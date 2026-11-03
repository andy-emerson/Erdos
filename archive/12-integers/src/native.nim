# native.nim: our machine-level operations side by side with Nim's own, on native uint64 (round 6). Control is the
# type's built-in operation. Each pair runs over the same 1024 operand pairs, reps calibrated to at least 50 ms, 15
# reps, the median per operation; every result is checksummed after the clock.
#   div_nim   a div b                          div_ours  exact division by inverse (bits.divExactInv)
#   sub_nim   a - b                            sub_ours  |a - b| from the subtraction's borrow (bits.usubAsm)
# The division pairs are exact (a = b * q, with b and q below 2^32); the subtraction pairs are random, in either order.
# Also: a check that the two wrap-around ways of multiplying directions (mod 4) agree with (s + t) mod 4.
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

var da, db, sa, sb, r: array[N, uint64]
template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}

proc divNim(k: int) {.noinline.} =
  for it in 0 ..< k:
    for j in 0 ..< N: r[j] = da[j] div db[j]
    barrier()
proc divOurs(k: int) {.noinline.} =
  for it in 0 ..< k:
    for j in 0 ..< N: r[j] = divExactInv(da[j], db[j])
    barrier()
proc subNim(k: int) {.noinline.} =
  for it in 0 ..< k:
    for j in 0 ..< N: r[j] = sa[j] - sb[j]
    barrier()
proc subOurs(k: int) {.noinline.} =
  for it in 0 ..< k:
    for j in 0 ..< N: r[j] = usubAsm(sa[j], sb[j])
    barrier()

proc directionsCheck(): string =
  ## all 16 pairs: a byte that wraps (low 2 bits) and a word's top 2 bits (carry falls off) against (s + t) mod 4
  for s in 0'u64 .. 3:
    for t in 0'u64 .. 3:
      let want = (s + t) mod 4
      var byteR = uint8(s)
      for k in 0 ..< 70: byteR += uint8(t)          # many wraps past 256
      let byteWant = (s + 70 * t) mod 4
      doAssert (byteR and 3) == uint8(byteWant), "byte"
      let top = (s shl 62) + (t shl 62)
      doAssert (top shr 62) == want, "top"
  "directions mod 4: byte wrap and top-field wrap agree with (s + t) mod 4 on all 16 pairs"

proc main() =
  let name = paramStr(1)
  if name == "check":
    echo directionsCheck()
    var rng = initRand(7)
    for k in 0 ..< 1_000_000:
      let x = rng.next() shr 1
      let y = rng.next() shr 1
      doAssert usubAsm(x, y) == (if x >= y: x - y else: y - x)
    var rng2 = initRand(9)
    for k in 0 ..< 1_000_000:
      let b = (rng2.next() shr 32) or 1
      let q = rng2.next() shr 32
      doAssert divExactInv(b * q, b) == q
      let b2 = ((b shr 16) or 1) shl rng2.rand(15)   # an even divisor; the product stays below 2^64, so it is exact
      doAssert divExactInv(b2 * q, b2) == q
    echo "unsigned subtraction: 1 million random pairs; exact division: 2 million exact pairs (odd and even divisors): all passed"
    return
  var rng = initRand(20261031)
  for j in 0 ..< N:
    let b = (rng.next() shr 32) or 1
    let q = rng.next() shr 32
    da[j] = b * q
    db[j] = b
    sa[j] = rng.next()
    sb[j] = rng.next()
  let f = case name
    of "div_nim": divNim
    of "div_ours": divOurs
    of "sub_nim": subNim
    of "sub_ours": subOurs
    else: quit "unknown"
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
    ts.add float(now() - t0) / float(k * N)
  ts.sort()
  var h = 0'u64
  for j in 0 ..< N: h = (h xor r[j]) * 0x9E3779B97F4A7C15'u64
  echo name, " ", formatFloat(ts[7], ffDecimal, 4), " ", formatFloat((ts[14] - ts[0]) / ts[7], ffDecimal, 4), " ", toHex(h)

main()
