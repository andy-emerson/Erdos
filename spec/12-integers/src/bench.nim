# bench.nim: the driver, one binary per build (-d:bld=control|pow2|even|growing|bytes). Not a build: it reads a job
# file, converts its operands to the build's values outside the clock, and runs one operation over them.
#   dump <job> <op> <out>       each result once, as hex (the correctness check)
#   calib <job> <op> [min_ns]   r, the smallest power of two with a rep of at least min_ns (50 ms), cap 10 s
#   time <job> <op> <r> <reps>  reps timed reps of r passes over the operands; then the results' checksum
#   mem <job>                   (memory build) bytes per value and peak bytes during the operation, accounted
import std/[strutils, os]

const bld {.strdefine.} = "control"
when bld == "control": import control
elif bld == "pow2": import pow2
elif bld == "even": import even
elif bld == "growing": import growing
elif bld == "bytes": import bytes
else: {.error: "unknown build".}
when defined(mem) and not wordBuild: import alloc

const ops = ["add", "sub", "absdiff", "mul", "divexact", "gcd"]
const capNs = 10_000_000_000'i64

# ------------------------------------------------------------------------------------------------ per build
when wordBuild:
  template release(v: Val) = discard
  proc hexOf(v: Val): string = toHex(v).toLowerAscii
  template mixh(h: var uint64; v: Val) = h = (h xor v) * 0x9E3779B97F4A7C15'u64
elif bld == "growing":
  proc hexOf(v: Val): string =
    if v.len == 0: return "0"
    for i in countdown(v.len, 1): result.add toHex(uint64(v[i]), sizeof(Word) * 2).toLowerAscii
  proc mixh(h: var uint64; v: Val) =
    for i in 0 .. v.len: h = (h xor uint64(v[i])) * 0x9E3779B97F4A7C15'u64
else:
  proc hexOf(v: Val): string =
    let (vl, L) = byteLen(v)
    if L == 0: return "0"
    for k in countdown(L - 1, 0): result.add toHex(v[vl + k]).toLowerAscii
  proc mixh(h: var uint64; v: Val) =
    let (vl, L) = byteLen(v)
    for k in 0 ..< vl + L: h = (h xor uint64(v[k])) * 0x9E3779B97F4A7C15'u64

# ------------------------------------------------------------------------------------------------ clocks
type Timespec {.importc: "struct timespec", header: "<time.h>".} = object
  tv_sec: clong
  tv_nsec: clong
var CLOCK_MONOTONIC {.importc, header: "<time.h>".}: cint
var CLOCK_PROCESS_CPUTIME_ID {.importc, header: "<time.h>".}: cint
proc clock_gettime(id: cint; ts: var Timespec): cint {.importc, header: "<time.h>".}
proc ns(id: cint): int64 =
  var t: Timespec
  discard clock_gettime(id, t)
  int64(t.tv_sec) * 1_000_000_000 + int64(t.tv_nsec)

# ------------------------------------------------------------------------------------------------ the job file
# "INTJ" u32 k u32 N, then for each of the six operations N pairs; a value is u32 nwords then nwords u64, lowest first
proc readJob(path: string): (int, int, seq[seq[seq[uint64]]]) =
  let s = readFile(path)
  var p = 0
  proc u32(): int =
    result = int(cast[ptr uint32](unsafeAddr s[p])[])
    p += 4
  proc u64(): uint64 =
    result = cast[ptr uint64](unsafeAddr s[p])[]
    p += 8
  doAssert s[0 ..< 4] == "INTJ"
  p = 4
  let k = u32()
  let n = u32()
  var vals: seq[seq[seq[uint64]]]
  for o in 0 ..< ops.len:
    var vs: seq[seq[uint64]]
    for j in 0 ..< 2 * n:
      let m = u32()
      var w = newSeq[uint64](m)
      for i in 0 ..< m: w[i] = u64()
      vs.add w
    vals.add vs
  (k, n, vals)

# ------------------------------------------------------------------------------------------------ the timed loops
var gA, gB, gR: ptr UncheckedArray[Val]
var gN: int

template mkLoop(pname, f: untyped) =
  proc pname(r: int) {.exportc, noinline.} =
    let a = gA
    let b = gB
    let res = gR
    let n = gN
    for it in 0 ..< r:
      for j in 0 ..< n:
        release(res[j])
        res[j] = f(a[j], b[j])
      {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

mkLoop(loop_add, opAdd)
mkLoop(loop_sub, opSub)
mkLoop(loop_absdiff, opAbsdiff)
mkLoop(loop_mul, opMul)
mkLoop(loop_divexact, opDivexact)
mkLoop(loop_gcd, opGcd)
const loops = [loop_add, loop_sub, loop_absdiff, loop_mul, loop_divexact, loop_gcd]

proc main() =
  let args = commandLineParams()
  let mode = args[0]
  let (_, n, vals) = readJob(args[1])
  var A, B, R: seq[seq[Val]]
  for o in 0 ..< ops.len:
    var a, b, r: seq[Val]
    for j in 0 ..< n:
      a.add fromWords(vals[o][2 * j])
      b.add fromWords(vals[o][2 * j + 1])
      r.add default(Val)
    A.add a
    B.add b
    R.add r
  proc select(o: int) =
    gA = cast[ptr UncheckedArray[Val]](addr A[o][0])
    gB = cast[ptr UncheckedArray[Val]](addr B[o][0])
    gR = cast[ptr UncheckedArray[Val]](addr R[o][0])
    gN = n
  if mode == "mem":
    when defined(mem):
      for o in 0 ..< ops.len:
        select(o)
        var bAcct, bHeap, pk, pkMax = 0
        for j in 0 ..< n:
          when wordBuild:
            R[o][j] = loops.len.Val      # nothing to run: the figures are the word's 8 bytes, no allocation
            bAcct += 3 * sizeof(Val)
            pk += sizeof(Val)
            pkMax = sizeof(Val)
          else:
            let base = live
            peak = live
            R[o][j] = case o
              of 0: opAdd(A[o][j], B[o][j])
              of 1: opSub(A[o][j], B[o][j])
              of 2: opAbsdiff(A[o][j], B[o][j])
              of 3: opMul(A[o][j], B[o][j])
              of 4: opDivexact(A[o][j], B[o][j])
              else: opGcd(A[o][j], B[o][j])
            let p = peak - base
            pk += p
            pkMax = max(pkMax, p)
            for v in [A[o][j], B[o][j], R[o][j]]:
              bAcct += acctOf(v)
              bHeap += heapOf(v)
        echo "mem ", ops[o], " ", formatFloat(bAcct / (3 * n), ffDecimal, 3), " ",
             formatFloat(bHeap / (3 * n), ffDecimal, 3), " ", formatFloat(pk / n, ffDecimal, 3), " ", pkMax,
             (when wordBuild: " trivial" else: "")
      return
    else:
      quit "mem needs the memory build", 2
  let o = ops.find(args[2])
  doAssert o >= 0
  select(o)
  let loop = loops[o]
  case mode
  of "dump":
    loop(1)
    var f = open(args[3], fmWrite)
    for j in 0 ..< n: f.writeLine hexOf(R[o][j])
    f.close()
  of "calib":
    let minNs = (if args.len > 3: parseBiggestInt(args[3]) else: 50_000_000'i64)
    loop(1)                                    # warm-up: every result present
    var r = 1
    while true:
      let c0 = ns(CLOCK_PROCESS_CPUTIME_ID)
      let t0 = ns(CLOCK_MONOTONIC)
      loop(r)
      let t = ns(CLOCK_MONOTONIC) - t0
      let c = ns(CLOCK_PROCESS_CPUTIME_ID) - c0
      if t >= minNs or t > capNs:
        echo "calib ", r, " ", t, " ", c, (if t > capNs: " overcap" else: "")
        break
      r *= 2
  of "time":
    let r = parseInt(args[3])
    let reps = parseInt(args[4])
    loop(1)
    for i in 1 .. reps:
      let c0 = ns(CLOCK_PROCESS_CPUTIME_ID)
      let t0 = ns(CLOCK_MONOTONIC)
      loop(r)
      let t = ns(CLOCK_MONOTONIC) - t0
      let c = ns(CLOCK_PROCESS_CPUTIME_ID) - c0
      echo "rep ", i, " ", t, " ", c, (if t > capNs: " overcap" else: "")
      if t > capNs: break
    var h = 0'u64                               # after the clock: every result checksummed
    for j in 0 ..< n: mixh(h, R[o][j])
    echo "sum ", toHex(h).toLowerAscii
  else:
    quit "unknown mode", 2

main()
