# bench.nim: the round-2 driver (spec 13 §5 c), one binary per build: -d:bld=control | a | abf (A's branch-free row,
# with -d:branchfree) | b. Not a build: it reads a job file, makes its operands outside the clock (in each build's
# stored form: A and Control a header list and a word list, B a header list and two word lists), and runs one
# operation over them, each result stored and checksummed after the clock.
#   dump <job> <op> <out>        two passes; the second pass's results, raw, one line each, then a pools check
#   sum <job> <op>               one pass; the checksum of the results' values (the check)
#   calib <job> <op> [min_ns]    r, the smallest power of two with a rep of at least min_ns (50 ms), cap 10 s
#   time <job> <op> <r> <reps>   reps timed reps of r passes over the operands; then the results' checksum
#   mem <job>                    bytes per value, accounted, per operation (the -mem binary for Control)
# Operations: add sub neg mul div gcd cmp, and growth (its own job file: chains of x <- x*y + z, intermediates
# released). The checksum is over values (sign, length, words), so every build's agrees for the same job.
import std/[strutils, os]
import routines

const bld {.strdefine.} = "a"
when bld == "control":
  import control
elif bld in ["a", "abf"]:
  when (bld == "abf") != defined(branchfree): {.error: "abf needs -d:branchfree, a must not have it".}
  import designa, pools
elif bld == "b":
  import designb, pools
else:
  {.error: "unknown build".}

const isB = bld == "b"
const usesPools = bld != "control"
const ops = ["add", "sub", "neg", "mul", "div", "gcd", "cmp"]
const capNs = 10_000_000_000'i64

when isB:
  type
    Hd = uint16
    V = BV
else:
  type
    Hd = uint8
    V = AV

# ------------------------------------------------------------------------------------------------ the stored lists
type Store = object
  h: seq[Hd]
  w: seq[Word]    # A, Control: the word; B: P's word
  n: seq[Word]    # B: N's word

var gAH, gBH, gRH: ptr UncheckedArray[Hd]
var gAW, gBW, gRW, gAN, gBN, gRN: ptr UncheckedArray[Word]
var gRC: ptr UncheckedArray[int8]
var gN: int

template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

when isB:
  template call2(f, j): untyped = f(gAH[j], gAW[j], gAN[j], gBH[j], gBW[j], gBN[j])
  template call1(f, j): untyped = f(gAH[j], gAW[j], gAN[j])
  template put(j: int; v: V) =
    gRH[j] = v.h
    gRW[j] = v.p
    gRN[j] = v.n
  template relR(j: int) = release(gRH[j], gRW[j], gRN[j])
  template relV(v: V) = release(v.h, v.p, v.n)
  template mulV(a, b: V): V = opMul(a.h, a.p, a.n, b.h, b.p, b.n)
  template addV(a, b: V): V = opAdd(a.h, a.p, a.n, b.h, b.p, b.n)
  template mk(d: int; m, p, n: seq[Word]): V = make(p, n)
  template at(s: Store; j: int): V = BV(h: s.h[j], p: s.w[j], n: s.n[j])
else:
  template call2(f, j): untyped = f(gAH[j], gAW[j], gBH[j], gBW[j])
  template call1(f, j): untyped = f(gAH[j], gAW[j])
  template put(j: int; v: V) =
    gRH[j] = v.h
    gRW[j] = v.w
  template relR(j: int) = release(gRH[j], gRW[j])
  template relV(v: V) = release(v.h, v.w)
  template mulV(a, b: V): V = opMul(a.h, a.w, b.h, b.w)
  template addV(a, b: V): V = opAdd(a.h, a.w, b.h, b.w)
  template mk(d: int; m, p, n: seq[Word]): V = make(d, m)
  template at(s: Store; j: int): V = AV(h: s.h[j], w: s.w[j])

proc add(s: var Store; v: V) =
  s.h.add v.h
  when isB:
    s.w.add v.p
    s.n.add v.n
  else:
    s.w.add v.w

# ------------------------------------------------------------------------------------------------ the timed loops
template mkLoop2(pname, f: untyped) =
  proc pname(r: int) {.exportc, noinline.} =
    let n = gN
    for it in 0 ..< r:
      for j in 0 ..< n:
        relR(j)
        let v = call2(f, j)
        put(j, v)
      barrier()

mkLoop2(loop_add, opAdd)
mkLoop2(loop_sub, opSub)
mkLoop2(loop_mul, opMul)
mkLoop2(loop_div, opDiv)
mkLoop2(loop_gcd, opGcd)

proc loop_neg(r: int) {.exportc, noinline.} =
  let n = gN
  for it in 0 ..< r:
    for j in 0 ..< n:
      relR(j)
      let v = call1(opNeg, j)
      put(j, v)
    barrier()

proc loop_cmp(r: int) {.exportc, noinline.} =
  let n = gN
  for it in 0 ..< r:
    for j in 0 ..< n:
      gRC[j] = int8(call2(opCmp, j))
    barrier()

const loops = [loop_add, loop_sub, loop_neg, loop_mul, loop_div, loop_gcd, loop_cmp]

# the growth workload: 64 chains of 32 steps of x <- x*y + z; each intermediate released once used
var gX0, gY, gZ, gRes: seq[V]
var gChains, gSteps: int

proc loop_growth(r: int) {.exportc, noinline.} =
  for it in 0 ..< r:
    for c in 0 ..< gChains:
      relV(gRes[c])
      var x = gX0[c]
      for s in 0 ..< gSteps:
        let k = c * gSteps + s
        let t = mulV(x, gY[k])
        if s > 0: relV(x)
        x = addV(t, gZ[k])
        relV(t)
      gRes[c] = x
    barrier()

# ------------------------------------------------------------------------------------------------ values (outside the clock)
proc valueOf(v: V): (bool, seq[Word]) =
  ## the value as (negative, magnitude words at their real length)
  when isB:
    let (p, n) = partsOf(v.h, v.p, v.n)
    let m = max(p.len, n.len)
    var r = newSeq[Word](max(m, 1))
    var pp = p
    var nn = n
    pp.add 0
    nn.add 0
    var br: Word
    let l = rAbsdiff(cast[Words](addr r[0]), cast[Words](addr pp[0]), p.len, cast[Words](addr nn[0]), n.len, br)
    r.setLen(l)
    (br != 0, r)
  else:
    (negative(v.h, v.w), words(v.h, v.w))

template mix(h: var uint64; x: uint64) = h = (h xor x) * 0x9E3779B97F4A7C15'u64

proc mixVal(h: var uint64; v: V) =
  let (neg, m) = valueOf(v)
  mix(h, uint64(neg))
  mix(h, uint64(m.len))
  for x in m: mix(h, x)

proc dumpOf(v: V): string =
  when isB: dumpLine(v.h, v.p, v.n) else: dumpLine(v.h, v.w)

proc bytesOf(v: V): (int, int) =
  ## bytes per value: the stored lists' share (header and words) plus the slots, or Control's block (accounted;
  ## malloc's usable size beside)
  when bld == "control":
    when defined(mem):
      let (a, h) = blockBytes(v.w)
      (1 + 8 + a, 1 + 8 + h)
    else:
      quit "mem needs the -mem binary for Control", 2
  elif isB:
    var s = 2 + 16
    for c in [int(v.h and 31), int((v.h shr 8) and 31)]:
      if c != 0: s += (1 shl c) * sizeof(Word)
    (s, s)
  else:
    let c = int(v.h and 31)
    let s = 1 + 8 + (if c != 0: (1 shl c) * sizeof(Word) else: 0)
    (s, s)

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

# ------------------------------------------------------------------------------------------------ job files
# "HJ13" u32 k u32 nops, then per operation (add sub neg mul div gcd cmp) u32 n and n pairs (a, b); "HG13" u32 chains
# u32 steps, then x0 for each chain, then (y, z) for each chain and step. A value: u32 d (direction), then three word
# lists (u32 count, that many u64, lowest first): the magnitude (A, Control), P and N (B).
type Reader = object
  s: string
  p: int

proc u32(r: var Reader): int =
  result = int(cast[ptr uint32](unsafeAddr r.s[r.p])[])
  r.p += 4

proc wl(r: var Reader): seq[Word] =
  let m = r.u32()
  for i in 0 ..< m:
    result.add cast[ptr uint64](unsafeAddr r.s[r.p])[]
    r.p += 8

proc value(r: var Reader): V =
  let d = r.u32()
  let m = r.wl()
  let p = r.wl()
  let n = r.wl()
  mk(d, m, p, n)

proc timeLoop(loop: proc (r: int) {.noinline.}; mode: string; args: seq[string]; n: int; sum: proc (): uint64) =
  case mode
  of "sum":
    loop(1)
    echo "sum ", toHex(sum()).toLowerAscii
  of "calib":
    let minNs = (if args.len > 3: parseBiggestInt(args[3]) else: 50_000_000'i64)
    loop(1)                                     # warm-up: every result present
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
    echo "sum ", toHex(sum()).toLowerAscii      # after the clock: every result checksummed
  else:
    quit "unknown mode " & mode, 2

proc poolLine(): string =
  when usesPools:
    let e = poolsCheck()
    (if e == "": "pools ok" else: "pools bad " & e)
  else:
    "pools -"

# ------------------------------------------------------------------------------------------------ main
proc growthMain(mode: string; args: seq[string]) =
  var rd = Reader(s: readFile(args[1]))
  doAssert rd.s[0 ..< 4] == "HG13"
  rd.p = 4
  gChains = rd.u32()
  gSteps = rd.u32()
  for c in 0 ..< gChains: gX0.add rd.value()
  for k in 0 ..< gChains * gSteps:
    gY.add rd.value()
    gZ.add rd.value()
  gRes = newSeq[V](gChains)
  proc sum(): uint64 =
    for c in 0 ..< gChains: mixVal(result, gRes[c])
  case mode
  of "dump":
    loop_growth(2)
    var f = open(args[3], fmWrite)
    for c in 0 ..< gChains: f.writeLine dumpOf(gRes[c])
    for v in gRes: relV(v)
    for v in gX0: relV(v)
    for v in gY: relV(v)
    for v in gZ: relV(v)
    f.writeLine poolLine()
    f.close()
  of "mem":
    loop_growth(1)
    var a, h = 0
    for v in gRes:
      let (x, y) = bytesOf(v)
      a += x
      h += y
    echo "mem growth ", formatFloat(a / gChains, ffDecimal, 3), " ", formatFloat(h / gChains, ffDecimal, 3)
    when usesPools: echo "poolbytes ", poolBytes()
  else:
    timeLoop(loop_growth, mode, args, gChains * gSteps, sum)

proc main() =
  let args = commandLineParams()
  if args.len < 2: quit "usage: bench dump|sum|calib|time|mem <job> [op ...]", 2
  let mode = args[0]
  if (args.len > 2 and args[2] == "growth") or (mode == "mem" and readFile(args[1])[0 ..< 4] == "HG13"):
    growthMain(mode, args)
    return
  var rd = Reader(s: readFile(args[1]))
  doAssert rd.s[0 ..< 4] == "HJ13"
  rd.p = 4
  discard rd.u32()                          # k
  let nops = rd.u32()
  doAssert nops == ops.len
  var A, B, R: seq[Store]
  var RC: seq[seq[int8]]
  var cnt: seq[int]
  for o in 0 ..< nops:
    let n = rd.u32()
    cnt.add n
    var a, b: Store
    var r = Store(h: newSeq[Hd](n), w: newSeq[Word](n), n: newSeq[Word](n))
    for j in 0 ..< n:
      a.add rd.value()
      b.add rd.value()
    A.add a
    B.add b
    R.add r
    RC.add newSeq[int8](n)
  proc select(o: int) =
    gN = cnt[o]
    if gN == 0: return
    gAH = cast[ptr UncheckedArray[Hd]](addr A[o].h[0])
    gBH = cast[ptr UncheckedArray[Hd]](addr B[o].h[0])
    gRH = cast[ptr UncheckedArray[Hd]](addr R[o].h[0])
    gAW = cast[ptr UncheckedArray[Word]](addr A[o].w[0])
    gBW = cast[ptr UncheckedArray[Word]](addr B[o].w[0])
    gRW = cast[ptr UncheckedArray[Word]](addr R[o].w[0])
    when isB:
      gAN = cast[ptr UncheckedArray[Word]](addr A[o].n[0])
      gBN = cast[ptr UncheckedArray[Word]](addr B[o].n[0])
      gRN = cast[ptr UncheckedArray[Word]](addr R[o].n[0])
    gRC = cast[ptr UncheckedArray[int8]](addr RC[o][0])
  if mode == "mem":
    for o in 0 ..< nops:                     # every operation once, all results kept
      select(o)
      if gN > 0: loops[o](1)
    # per operation: the mean over its operands and results (compare's result is not a value)
    for o in 0 ..< nops:
      select(o)
      if gN == 0: continue
      var a, h, m = 0
      for j in 0 ..< gN:
        var all = @[at(A[o], j)]
        if ops[o] != "neg": all.add at(B[o], j)
        if ops[o] != "cmp": all.add at(R[o], j)
        for v in all:
          let (x, y) = bytesOf(v)
          a += x
          h += y
          inc m
      echo "mem ", ops[o], " ", formatFloat(a / m, ffDecimal, 3), " ", formatFloat(h / m, ffDecimal, 3)
    when usesPools: echo "poolbytes ", poolBytes()
    return
  let o = ops.find(args[2])
  if o < 0: quit "unknown operation " & args[2], 2
  select(o)
  let n = gN
  if mode == "dump":
    loops[o](2)
    var f = open(args[3], fmWrite)
    for j in 0 ..< n:
      f.writeLine(if ops[o] == "cmp": $RC[o][j] else: dumpOf(at(R[o], j)))
    for q in 0 ..< nops:                       # release everything: every slot must be free again
      for j in 0 ..< cnt[q]:
        if ops[q] != "cmp": relV(at(R[q], j))
        relV(at(A[q], j))
        relV(at(B[q], j))
    f.writeLine poolLine()
    f.close()
    return
  proc sum(): uint64 =
    for j in 0 ..< n:
      if ops[o] == "cmp": mix(result, uint64(int64(RC[o][j]) + 1))
      else: mixVal(result, at(R[o], j))
  timeLoop(loops[o], mode, args, n, sum)

main()
