# divbench.nim: exact division on native uint64, by each method, against Nim's own `div` (control). Same timing as
# the builds: reps of at least 50 ms, 15 reps, the median per division; results checksummed after the clock.
#   divbench <method> <set>     method: nim newton par table d32 fp pair_nim pair_reuse
#                               set: small (a < 2^32), medium (a < 2^53), large (a < 2^64)
#   divbench check              every method against the exact quotient, on every set
# pair_nim and pair_reuse divide two numbers by the same divisor (as a fraction's numerator and denominator are divided
# by their gcd): Nim's two divides, or one inverse used twice. Their time is per division.
import std/[os, strutils, random, algorithm]
import bits

const N = 1024
type Timespec {.importc: "struct timespec", header: "<time.h>".} = object
  tv_sec: clong
  tv_nsec: clong
var CLOCK_MONOTONIC {.importc, header: "<time.h>".}: cint
proc clock_gettime(id: cint; ts: var Timespec): cint {.importc, header: "<time.h>".}
proc now(): int64 =
  var t: Timespec
  discard clock_gettime(CLOCK_MONOTONIC, t)
  int64(t.tv_sec) * 1_000_000_000 + int64(t.tv_nsec)

var A, A2, B, Q, Q2, R, R2: array[N, uint64]
template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

template mk(name, f: untyped) =
  proc name(k: int) {.noinline, exportc.} =
    for it in 0 ..< k:
      for j in 0 ..< N: R[j] = f(A[j], B[j])
      barrier()

template nimDiv(a, b: uint64): uint64 = a div b
mk(run_nim, nimDiv)
mk(run_newton, divExactInv)
mk(run_par, divPar)
mk(run_table, divTable)
mk(run_d32, div32)
mk(run_fp, divFp)

proc run_pair_nim(k: int) {.noinline, exportc.} =
  for it in 0 ..< k:
    for j in 0 ..< N:
      R[j] = A[j] div B[j]
      R2[j] = A2[j] div B[j]
    barrier()

proc run_pair_reuse(k: int) {.noinline, exportc.} =
  for it in 0 ..< k:
    for j in 0 ..< N:
      var s: int
      let x = inverseOf(B[j], s)
      R[j] = (A[j] shr s) * x
      R2[j] = (A2[j] shr s) * x
    barrier()

proc fill(set: string; seed: int64) =
  var rng = initRand(seed)
  let (bb, qb) = (case set
    of "small": (16, 16)
    of "medium": (26, 27)
    of "large": (32, 32)
    else: quit "unknown set")
  for j in 0 ..< N:
    let sh = rng.rand(3)                                       # some even divisors: b = odd part shifted left
    let bs = ((rng.next() shr (64 - bb + 3)) or 1) shl sh     # below 2^bb
    Q[j] = rng.next() shr (64 - qb)
    Q2[j] = rng.next() shr (64 - qb)
    B[j] = bs
    A[j] = bs * Q[j]
    A2[j] = bs * Q2[j]

proc main() =
  let m = paramStr(1)
  if m == "check":
    for set in ["small", "medium", "large"]:
      for seed in 1 .. 50:
        fill(set, seed)
        for j in 0 ..< N:
          let a = A[j]
          let b = B[j]
          let q = Q[j]
          doAssert a div b == q
          doAssert divExactInv(a, b) == q, "newton"
          doAssert divPar(a, b) == q, "par"
          doAssert divTable(a, b) == q, "table"
          doAssert div32(a, b) == q, "d32"
          doAssert divFp(a, b) == q, "fp"
          var s: int
          let x = inverseOf(b, s)
          doAssert (a shr s) * x == q and (A2[j] shr s) * x == Q2[j], "reuse"
    for k in 0 ..< 64:                      # zero dividend, divisors of any size, some with their low 32 bits 0
      let b = (uint64(k) + 1) shl (k mod 33 + 31)
      doAssert div32(0, b) == 0 and divTable(0, b) == 0 and divExactInv(0, b) == 0, "zero dividend"
    echo "every method gives the exact quotient on 153,600 divisions (3 sets x 50 seeds x 1024), and on zero dividends"
    return
  fill(paramStr(2), 20261031)
  let f = case m
    of "nim": run_nim
    of "newton": run_newton
    of "par": run_par
    of "table": run_table
    of "d32": run_d32
    of "fp": run_fp
    of "pair_nim": run_pair_nim
    of "pair_reuse": run_pair_reuse
    else: quit "unknown method"
  let per = (if m.startsWith("pair"): 2 * N else: N)
  var k = 1
  while true:
    let t0 = now()
    f(k)
    if now() - t0 >= 50_000_000: break
    k *= 2
  var ts: seq[float]
  for i in 0 ..< 15:
    let t0 = now()
    f(k)
    ts.add float(now() - t0) / float(k * per)
  ts.sort()
  var h = 0'u64
  for j in 0 ..< N: h = (h xor R[j] xor (R2[j] shl 1)) * 0x9E3779B97F4A7C15'u64
  echo m, " ", formatFloat(ts[7], ffDecimal, 4), " ", formatFloat((ts[14] - ts[0]) / ts[7], ffDecimal, 4), " ", toHex(h)

main()
