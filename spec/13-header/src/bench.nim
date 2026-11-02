# bench.nim: the driver for spec 13 v0.2 §7 (timed trials), one binary per build:
#   -d:bld=control   Control: int64, complex as a pair (re, im)
#   -d:bld=a         Design A: a header list (uint8) and a word list
#   -d:bld=abf       Design A's branch-free row
#   -d:bld=amul      Design A's multiplication row (signs by multiplying by 1 - 2b, no branch)
#   -d:bld=alg       Design A's header form (headers loaded sign-extended, the larger operand's direction, no branch)
#   -d:bld=anm       Design A's narrow mask row (borrow by mask, other branches kept)
#   -d:bld=b         Design B: a list of records of two words (four for complex)
# Not a build: it reads a job file (one row: a kind, an operation and its operand pairs), converts the operands to the
# build's stored form outside the clock, and runs the operation over them. Every result is stored; after the clock the
# results are checksummed over their values, (re < 0, |re|, im < 0, |im|), so every build computing the same values
# gives the same checksum, whatever the form (a zero's direction, B's uncollapsed parts).
#   dump <job> <out>            one pass; each result's stored form, one line each (the check)
#   sum <job>                   one pass; the checksum
#   calib <job> [min_ns]        r, the smallest power of two with a rep of at least min_ns (50 ms), cap 10 s
#   time <job> <r> <reps>       reps timed reps of r passes over the operands; then the checksum
#   bytes                       bytes per value: integers, complex
# Exit 3: an error of the spec (A: a sum across axes of two nonzero numbers). Exit 4: the build has no such operation
# (collapse is B's only).
import std/[strutils, os]

const bld {.strdefine.} = "a"
when bld == "control": import control
elif bld in ["a", "abf", "anm", "amul", "alg"]: import designa
elif bld == "b": import designb
else: {.error: "unknown build".}
import pseudo

const capNs = 10_000_000_000'i64
const opNames = ["add", "sub", "neg", "mul", "div", "collapse"]
const kindNames = ["int", "cx"]

template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

var gN: int
var gErr: Word        # abf: the sticky error word, ORed in after each pass

# ------------------------------------------------------------------------------------------------ stored forms and loops
template loopV(name: untyped; T: typedesc; A, B, R: untyped; call: untyped) =
  ## a value build (Control, B): one list of values per operand and for the results; the operation writes res[j]
  proc name(r: int) {.exportc, noinline.} =
    let a {.inject.} = A
    let b {.inject.} = B
    let res {.inject.} = R
    let n = gN
    for it in 0 ..< r:
      for j {.inject.} in 0 ..< n:
        call
      barrier()

when bld == "control":
  var gIA, gIB, gIR: ptr UncheckedArray[CI]
  var gCA, gCB, gCR: ptr UncheckedArray[CC]
  loopV(loop_int_add, CI, gIA, gIB, gIR, ciAdd(a[j], b[j], res[j]))
  loopV(loop_int_sub, CI, gIA, gIB, gIR, ciSub(a[j], b[j], res[j]))
  loopV(loop_int_neg, CI, gIA, gIB, gIR, ciNeg(a[j], res[j]))
  loopV(loop_int_mul, CI, gIA, gIB, gIR, ciMul(a[j], b[j], res[j]))
  loopV(loop_int_div, CI, gIA, gIB, gIR, ciDiv(a[j], b[j], res[j]))
  loopV(loop_cx_add, CC, gCA, gCB, gCR, ccAdd(a[j], b[j], res[j]))
  loopV(loop_cx_sub, CC, gCA, gCB, gCR, ccSub(a[j], b[j], res[j]))
  loopV(loop_cx_neg, CC, gCA, gCB, gCR, ccNeg(a[j], res[j]))
  loopV(loop_cx_mul, CC, gCA, gCB, gCR, ccMul(a[j], b[j], res[j]))
  loopV(loop_cx_div, CC, gCA, gCB, gCR, ccDiv(a[j], b[j], res[j]))
  const loops = [[loop_int_add, loop_int_sub, loop_int_neg, loop_int_mul, loop_int_div, nil],
                 [loop_cx_add, loop_cx_sub, loop_cx_neg, loop_cx_mul, loop_cx_div, nil]]
  var sI: array[3, seq[CI]]
  var sC: array[3, seq[CC]]
  proc toI(d: int; m: uint64): CI = (if d == 0: cast[int64](m) else: 0'i64 - cast[int64](m))
  proc toC(d: int; m: uint64): CC =
    let v = cast[int64](m)
    case d
    of 0: CC(re: v)
    of 1: CC(im: v)
    of 2: CC(re: 0'i64 - v)
    else: CC(im: 0'i64 - v)
  proc load(kind, d1: int; m1: uint64; p1: array[4, uint64]; d2: int; m2: uint64; p2: array[4, uint64]) =
    if kind == 0:
      sI[0].add toI(d1, m1)
      sI[1].add toI(d2, m2)
      sI[2].add 0
    else:
      sC[0].add toC(d1, m1)
      sC[1].add toC(d2, m2)
      sC[2].add CC()
  proc bindRow() =
    gIA = cast[ptr UncheckedArray[CI]](sI[0][0].addr); gIB = cast[ptr UncheckedArray[CI]](sI[1][0].addr)
    gIR = cast[ptr UncheckedArray[CI]](sI[2][0].addr)
    gCA = cast[ptr UncheckedArray[CC]](sC[0][0].addr); gCB = cast[ptr UncheckedArray[CC]](sC[1][0].addr)
    gCR = cast[ptr UncheckedArray[CC]](sC[2][0].addr)
  proc sgn(v: int64): (uint64, uint64) =
    (if v < 0: (1'u64, 0'u64 - cast[uint64](v)) else: (0'u64, cast[uint64](v)))
  proc value(kind, j: int): array[4, uint64] =
    if kind == 0:
      let (n, m) = sgn(sI[2][j])
      [n, m, 0, 0]
    else:
      let (n, m) = sgn(sC[2][j].re)
      let (n2, m2) = sgn(sC[2][j].im)
      [n, m, n2, m2]
  proc raw(kind, j: int): string =
    if kind == 0: $sI[2][j] else: $sC[2][j].re & " " & $sC[2][j].im
  const bytesInt = sizeof(CI)
  const bytesCx = sizeof(CC)

elif bld in ["a", "abf", "anm", "amul", "alg"]:
  var gAH, gBH, gRH: ptr UncheckedArray[uint8]
  var gAW, gBW, gRW: ptr UncheckedArray[Word]
  template loopA(name: untyped; one: static bool; call: untyped) =
    ## two parallel lists per operand and for the results
    proc name(r: int) {.exportc, noinline.} =
      let ah {.inject.} = gAH
      let aw {.inject.} = gAW
      let bh {.inject.} = gBH
      let bw {.inject.} = gBW
      let rh {.inject.} = gRH
      let rw {.inject.} = gRW
      let n = gN
      var err {.inject.} = 0'u64
      for it in 0 ..< r:
        for j {.inject.} in 0 ..< n:
          call
        barrier()
      gErr = gErr or err
  when bld == "a":
    loopA(loop_int_add, false, aAdd(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
    loopA(loop_int_sub, false, aSub(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
  elif bld == "amul":
    loopA(loop_int_add, false, aAddMul(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
    loopA(loop_int_sub, false, aSubMul(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
  elif bld == "alg":
    loopA(loop_int_add, false, aAddLg(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
    loopA(loop_int_sub, false, aSubLg(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
  elif bld == "anm":
    loopA(loop_int_add, false, aAddNM(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
    loopA(loop_int_sub, false, aSubNM(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
  else:
    loopA(loop_int_add, false, aAddBF(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
    loopA(loop_int_sub, false, aSubBF(ah[j], aw[j], bh[j], bw[j], err, rh[j], rw[j]))
  loopA(loop_int_neg, true, aNeg(ah[j], aw[j], rh[j], rw[j]))
  loopA(loop_int_mul, false, aMul(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
  loopA(loop_int_div, false, aDiv(ah[j], aw[j], bh[j], bw[j], rh[j], rw[j]))
  # A's code is the same for integers and complex numbers on the axes: the complex rows run the same loops
  const loops = [[loop_int_add, loop_int_sub, loop_int_neg, loop_int_mul, loop_int_div, nil],
                 [loop_int_add, loop_int_sub, loop_int_neg, loop_int_mul, loop_int_div, nil]]
  var sH: array[3, seq[uint8]]
  var sW: array[3, seq[Word]]
  proc load(kind, d1: int; m1: uint64; p1: array[4, uint64]; d2: int; m2: uint64; p2: array[4, uint64]) =
    sH[0].add uint8(d1 shl 6)
    sW[0].add m1
    sH[1].add uint8(d2 shl 6)
    sW[1].add m2
    sH[2].add 0xFF'u8          # never a header: every result is written
    sW[2].add 0
  proc bindRow() =
    gAH = cast[ptr UncheckedArray[uint8]](sH[0][0].addr); gBH = cast[ptr UncheckedArray[uint8]](sH[1][0].addr)
    gRH = cast[ptr UncheckedArray[uint8]](sH[2][0].addr)
    gAW = cast[ptr UncheckedArray[Word]](sW[0][0].addr); gBW = cast[ptr UncheckedArray[Word]](sW[1][0].addr)
    gRW = cast[ptr UncheckedArray[Word]](sW[2][0].addr)
  proc value(kind, j: int): array[4, uint64] =
    let m = sW[2][j]
    if m == 0: return [0'u64, 0, 0, 0]
    case int(sH[2][j] shr 6)
    of 0: [0'u64, m, 0, 0]
    of 1: [0'u64, 0, 0, m]
    of 2: [1'u64, m, 0, 0]
    else: [0'u64, 0, 1, m]
  proc raw(kind, j: int): string = toHex(sH[2][j]).toLowerAscii & " " & toHex(sW[2][j]).toLowerAscii
  const bytesInt = sizeof(uint8) + sizeof(Word)
  const bytesCx = sizeof(uint8) + sizeof(Word)

else:
  var gIA, gIB, gIR: ptr UncheckedArray[BI]
  var gCA, gCB, gCR: ptr UncheckedArray[BC]
  loopV(loop_int_add, BI, gIA, gIB, gIR, biAdd(a[j], b[j], res[j]))
  loopV(loop_int_sub, BI, gIA, gIB, gIR, biSub(a[j], b[j], res[j]))
  loopV(loop_int_neg, BI, gIA, gIB, gIR, biNeg(a[j], res[j]))
  loopV(loop_int_mul, BI, gIA, gIB, gIR, biMul(a[j], b[j], res[j]))
  loopV(loop_int_div, BI, gIA, gIB, gIR, biDiv(a[j], b[j], res[j]))
  loopV(loop_int_collapse, BI, gIA, gIB, gIR, biCollapse(a[j], res[j]))
  loopV(loop_cx_add, BC, gCA, gCB, gCR, bcAdd(a[j], b[j], res[j]))
  loopV(loop_cx_sub, BC, gCA, gCB, gCR, bcSub(a[j], b[j], res[j]))
  loopV(loop_cx_neg, BC, gCA, gCB, gCR, bcNeg(a[j], res[j]))
  loopV(loop_cx_mul, BC, gCA, gCB, gCR, bcMul(a[j], b[j], res[j]))
  loopV(loop_cx_div, BC, gCA, gCB, gCR, bcDiv(a[j], b[j], res[j]))
  loopV(loop_cx_collapse, BC, gCA, gCB, gCR, bcCollapse(a[j], res[j]))
  const loops = [[loop_int_add, loop_int_sub, loop_int_neg, loop_int_mul, loop_int_div, loop_int_collapse],
                 [loop_cx_add, loop_cx_sub, loop_cx_neg, loop_cx_mul, loop_cx_div, loop_cx_collapse]]
  var sI: array[3, seq[BI]]
  var sC: array[3, seq[BC]]
  proc load(kind, d1: int; m1: uint64; p1: array[4, uint64]; d2: int; m2: uint64; p2: array[4, uint64]) =
    if kind == 0:
      sI[0].add [p1[0], p1[1]]
      sI[1].add [p2[0], p2[1]]
      sI[2].add [0'u64, 0]
    else:
      sC[0].add p1
      sC[1].add p2
      sC[2].add [0'u64, 0, 0, 0]
  proc bindRow() =
    gIA = cast[ptr UncheckedArray[BI]](sI[0][0].addr); gIB = cast[ptr UncheckedArray[BI]](sI[1][0].addr)
    gIR = cast[ptr UncheckedArray[BI]](sI[2][0].addr)
    gCA = cast[ptr UncheckedArray[BC]](sC[0][0].addr); gCB = cast[ptr UncheckedArray[BC]](sC[1][0].addr)
    gCR = cast[ptr UncheckedArray[BC]](sC[2][0].addr)
  proc pn(p, n: uint64): (uint64, uint64) = (if p >= n: (0'u64, p - n) else: (1'u64, n - p))
  proc value(kind, j: int): array[4, uint64] =
    if kind == 0:
      let (s, m) = pn(sI[2][j][0], sI[2][j][1])
      [s, m, 0, 0]
    else:
      let v = sC[2][j]
      let (s, m) = pn(v[0], v[2])
      let (s2, m2) = pn(v[1], v[3])
      [s, m, s2, m2]
  proc raw(kind, j: int): string =
    var w: seq[string]
    if kind == 0:
      for x in sI[2][j]: w.add toHex(x).toLowerAscii
    else:
      for x in sC[2][j]: w.add toHex(x).toLowerAscii
    w.join(" ")
  const bytesInt = sizeof(BI)
  const bytesCx = sizeof(BC)

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
# "H13R" u32 kind (0 integers, 1 complex) u32 op (add sub neg mul div collapse) u32 n, then n pairs (a, b); an operand
# is u32 d (direction: 0..3, integers 0 or 2), u64 m (its magnitude: A's and Control's form), u64 p[4] (B's parts:
# integers P = p[0], N = p[1]; complex the four slots). One-operand operations ignore b.
proc readJob(path: string): (int, int, int) =
  let s = readFile(path)
  var p = 4
  proc u32(): int =
    result = int(cast[ptr uint32](unsafeAddr s[p])[])
    p += 4
  proc u64(): uint64 =
    result = cast[ptr uint64](unsafeAddr s[p])[]
    p += 8
  if s.len < 16 or s[0 ..< 4] != "H13R": quit "not a job file: " & path, 2
  let kind = u32()
  let op = u32()
  let n = u32()
  for j in 0 ..< n:
    var d, m: array[2, uint64]
    var ps: array[2, array[4, uint64]]
    for k in 0 .. 1:
      d[k] = uint64(u32())
      m[k] = u64()
      for i in 0 .. 3: ps[k][i] = u64()
    load(kind, int(d[0]), m[0], ps[0], int(d[1]), m[1], ps[1])
  (kind, op, n)

template mix(h: var uint64; x: uint64) = h = (h xor x) * 0x9E3779B97F4A7C15'u64

proc run(loop: proc (r: int) {.noinline.}; r: int) =
  loop(r)
  if gErr != 0:
    stderr.writeLine "error: sum across axes of two nonzero numbers (an expression)"
    quit 3

proc main() =
  let args = commandLineParams()
  if args.len < 1: quit "usage: bench dump|sum|calib|time|bytes <job> ...", 2
  let mode = args[0]
  if mode == "bytes":
    echo "bytes ", bld, " int ", bytesInt, " cx ", bytesCx
    return
  let (kind, op, n) = readJob(args[1])
  if n == 0: quit "empty job", 2
  let loop = loops[kind][op]
  if loop == nil:
    stderr.writeLine bld & " has no " & opNames[op]
    quit 4
  bindRow()
  gN = n
  proc sum(): uint64 =
    for j in 0 ..< n:
      for x in value(kind, j): mix(result, x)
  case mode
  of "dump":
    run(loop, 1)
    var f = open(args[2], fmWrite)
    for j in 0 ..< n: f.writeLine raw(kind, j)
    f.close()
  of "sum":
    run(loop, 1)
    echo "sum ", toHex(sum()).toLowerAscii
  of "calib":
    let minNs = (if args.len > 2: parseBiggestInt(args[2]) else: 50_000_000'i64)
    run(loop, 1)                                    # warm-up: every result present
    var r = 1
    while true:
      let c0 = ns(CLOCK_PROCESS_CPUTIME_ID)
      let t0 = ns(CLOCK_MONOTONIC)
      run(loop, r)
      let t = ns(CLOCK_MONOTONIC) - t0
      let c = ns(CLOCK_PROCESS_CPUTIME_ID) - c0
      if t >= minNs or t > capNs:
        echo "calib ", r, " ", t, " ", c, (if t > capNs: " overcap" else: "")
        break
      r *= 2
  of "time":
    let r = parseInt(args[2])
    let reps = parseInt(args[3])
    run(loop, 1)
    for i in 1 .. reps:
      let c0 = ns(CLOCK_PROCESS_CPUTIME_ID)
      let t0 = ns(CLOCK_MONOTONIC)
      run(loop, r)
      let t = ns(CLOCK_MONOTONIC) - t0
      let c = ns(CLOCK_PROCESS_CPUTIME_ID) - c0
      echo "rep ", i, " ", t, " ", c, (if t > capNs: " overcap" else: "")
      if t > capNs: break
    echo "sum ", toHex(sum()).toLowerAscii            # after the clock: every result checksummed
    echo "bytes ", (if kind == 0: bytesInt else: bytesCx)
  else:
    quit "unknown mode " & mode, 2

main()
