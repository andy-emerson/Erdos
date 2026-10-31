# divbench2.nim: exact division methods and their combinations against the default division, on native types.
# Every row divides two dividends by the same divisor (as a fraction's numerator and denominator are divided by their
# gcd); time is per division. Same timing as the builds: reps of at least 50 ms, 15 reps, median.
#   divbench2 <row> <set>    row: control table reuse d32 table_reuse table_d32 reuse_d32 all
#                            set: 32 (dividends below 2^32), 64 (below 2^64), 128 (below 2^128)
#   divbench2 check          every row against the exact quotient, on every set
# Control is the default division of the native type: uint64 `div` for sets 32 and 64; for set 128, the compiler's
# unsigned __int128 division. A quotient always fits 64 bits (divisor and quotient are each at most half the size).
#   table   inverse from the 8-bit table start (bits.invTable) and 3 Newton steps, computed for each division
#   reuse   inverse from the 5-bit start and 4 Newton steps, computed once per divisor and used for both dividends
#   d32     the 32-bit hardware divide when both dividends are below 2^32, otherwise the default division
# The combinations join these: table_reuse computes the table-start inverse once per divisor; table_d32 and reuse_d32
# take the 32-bit divide when it applies, otherwise the other method; all does the 32-bit divide when it applies,
# otherwise the table-start inverse once per divisor.
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

{.emit: """/*TYPESECTION*/
typedef unsigned __int128 u128;
""".}

# dividends as (lo, hi); hi = 0 in sets 32 and 64
var Alo, Ahi, A2lo, A2hi, B, Q, Q2, R, R2: array[N, uint64]
template barrier() = {.emit: "__asm__ __volatile__(\"\" ::: \"memory\");".}
proc opaque(x: uint64): uint64 {.inline.} =
  ## x, unchanged, but hidden from the compiler (no instructions), so a second inverse of the same divisor is computed
  ## again rather than merged with the first
  var y = x
  {.emit: ["__asm__(\"\" : \"+r\"(", y, "));"].}
  y

proc div128(lo, hi, b: uint64): uint64 {.inline.} =
  ## the compiler's own unsigned __int128 division (control for set 128); the quotient fits 64 bits
  var q: uint64
  {.emit: [q, " = (NU64)((((u128)", hi, " << 64) | ", lo, ") / (u128)", b, ");"].}
  q

template low64shr(lo, hi: uint64; s: int): uint64 =
  ## the low 64 bits of (hi:lo) >> s, 0 <= s < 64
  ((lo shr s) or ((hi shl 1) shl (63 - s)))           # no branch: shifting by 64 - s in two steps covers s = 0

proc tableInverse(b: uint64; s: var int): uint64 {.inline.} =
  s = ctz(b or (1'u64 shl 63))
  let o = b shr s
  var x = uint64(invTable[(o shr 1) and 127])
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x

proc newtonInverse(b: uint64; s: var int): uint64 {.inline.} =
  s = ctz(b or (1'u64 shl 63))
  let o = b shr s
  var x = (3'u64 * o) xor 2
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x

# ------------------------------------------------------------------------------------------------ the rows, one word
template loop64(name: untyped; body: untyped) =
  proc name(k: int) {.noinline, exportc.} =
    for it in 0 ..< k:
      for j {.inject.} in 0 ..< N:
        let a {.inject.} = Alo[j]
        let a2 {.inject.} = A2lo[j]
        let b {.inject.} = B[j]
        body
      barrier()

loop64(w_control):
  R[j] = a div b
  R2[j] = a2 div b
loop64(w_table):
  var s: int
  let x = tableInverse(b, s)
  R[j] = (a shr s) * x
  var s2: int
  let x2 = tableInverse(opaque(b), s2)
  R2[j] = (a2 shr s2) * x2
loop64(w_reuse):
  var s: int
  let x = newtonInverse(b, s)
  R[j] = (a shr s) * x
  R2[j] = (a2 shr s) * x
loop64(w_d32):
  if (a or a2 or b) < (1'u64 shl 32):
    R[j] = uint64(uint32(a) div uint32(b))
    R2[j] = uint64(uint32(a2) div uint32(b))
  else:
    R[j] = a div b
    R2[j] = a2 div b
loop64(w_table_reuse):
  var s: int
  let x = tableInverse(b, s)
  R[j] = (a shr s) * x
  R2[j] = (a2 shr s) * x
loop64(w_table_d32):
  if (a or a2 or b) < (1'u64 shl 32):
    R[j] = uint64(uint32(a) div uint32(b))
    R2[j] = uint64(uint32(a2) div uint32(b))
  else:
    var s: int
    let x = tableInverse(b, s)
    R[j] = (a shr s) * x
    var s2: int
    let x2 = tableInverse(opaque(b), s2)
    R2[j] = (a2 shr s2) * x2
loop64(w_reuse_d32):
  if (a or a2 or b) < (1'u64 shl 32):
    R[j] = uint64(uint32(a) div uint32(b))
    R2[j] = uint64(uint32(a2) div uint32(b))
  else:
    var s: int
    let x = newtonInverse(b, s)
    R[j] = (a shr s) * x
    R2[j] = (a2 shr s) * x
loop64(w_all):
  if (a or a2 or b) < (1'u64 shl 32):
    R[j] = uint64(uint32(a) div uint32(b))
    R2[j] = uint64(uint32(a2) div uint32(b))
  else:
    var s: int
    let x = tableInverse(b, s)
    R[j] = (a shr s) * x
    R2[j] = (a2 shr s) * x

# ------------------------------------------------------------------------------------------------ the rows, two words
template loop128(name: untyped; body: untyped) =
  proc name(k: int) {.noinline, exportc.} =
    for it in 0 ..< k:
      for j {.inject.} in 0 ..< N:
        let alo {.inject.} = Alo[j]
        let ahi {.inject.} = Ahi[j]
        let a2lo {.inject.} = A2lo[j]
        let a2hi {.inject.} = A2hi[j]
        let b {.inject.} = B[j]
        body
      barrier()

# d32 needs both dividends below 2^32; in this set they never are, so its rows fall back as written
loop128(d_control):
  R[j] = div128(alo, ahi, b)
  R2[j] = div128(a2lo, a2hi, b)
loop128(d_table):
  var s: int
  let x = tableInverse(b, s)
  R[j] = low64shr(alo, ahi, s) * x
  var s2: int
  let x2 = tableInverse(opaque(b), s2)
  R2[j] = low64shr(a2lo, a2hi, s2) * x2
loop128(d_reuse):
  var s: int
  let x = newtonInverse(b, s)
  R[j] = low64shr(alo, ahi, s) * x
  R2[j] = low64shr(a2lo, a2hi, s) * x
loop128(d_d32):
  if (ahi or a2hi or ((alo or a2lo or b) shr 32)) == 0:
    R[j] = uint64(uint32(alo) div uint32(b))
    R2[j] = uint64(uint32(a2lo) div uint32(b))
  else:
    R[j] = div128(alo, ahi, b)
    R2[j] = div128(a2lo, a2hi, b)
loop128(d_table_reuse):
  var s: int
  let x = tableInverse(b, s)
  R[j] = low64shr(alo, ahi, s) * x
  R2[j] = low64shr(a2lo, a2hi, s) * x
loop128(d_table_d32):
  if (ahi or a2hi or ((alo or a2lo or b) shr 32)) == 0:
    R[j] = uint64(uint32(alo) div uint32(b))
    R2[j] = uint64(uint32(a2lo) div uint32(b))
  else:
    var s: int
    let x = tableInverse(b, s)
    R[j] = low64shr(alo, ahi, s) * x
    var s2: int
    let x2 = tableInverse(opaque(b), s2)
    R2[j] = low64shr(a2lo, a2hi, s2) * x2
loop128(d_reuse_d32):
  if (ahi or a2hi or ((alo or a2lo or b) shr 32)) == 0:
    R[j] = uint64(uint32(alo) div uint32(b))
    R2[j] = uint64(uint32(a2lo) div uint32(b))
  else:
    var s: int
    let x = newtonInverse(b, s)
    R[j] = low64shr(alo, ahi, s) * x
    R2[j] = low64shr(a2lo, a2hi, s) * x
loop128(d_all):
  if (ahi or a2hi or ((alo or a2lo or b) shr 32)) == 0:
    R[j] = uint64(uint32(alo) div uint32(b))
    R2[j] = uint64(uint32(a2lo) div uint32(b))
  else:
    var s: int
    let x = tableInverse(b, s)
    R[j] = low64shr(alo, ahi, s) * x
    R2[j] = low64shr(a2lo, a2hi, s) * x

# ------------------------------------------------------------------------------------------------ operands
proc mul64(x, y: uint64; hi: var uint64): uint64 =
  var lo, h: uint64
  {.emit: ["u128 p_ = (u128)", x, " * ", y, "; ", lo, " = (NU64)p_; ", h, " = (NU64)(p_ >> 64);"].}
  hi = h
  lo

proc fill(set: int; seed: int64) =
  ## divisor and quotients each below 2^(set/2); some divisors even
  var rng = initRand(seed)
  let half = set div 2
  for j in 0 ..< N:
    let sh = rng.rand(3)
    let b = ((rng.next() shr (64 - half + 3)) or 1) shl sh
    let q = rng.next() shr (64 - half)
    let q2 = rng.next() shr (64 - half)
    B[j] = b
    Q[j] = q
    Q2[j] = q2
    Alo[j] = mul64(b, q, Ahi[j])
    A2lo[j] = mul64(b, q2, A2hi[j])

const rows = ["control", "table", "reuse", "d32", "table_reuse", "table_d32", "reuse_d32", "all"]
const loops64 = [w_control, w_table, w_reuse, w_d32, w_table_reuse, w_table_d32, w_reuse_d32, w_all]
const loops128 = [d_control, d_table, d_reuse, d_d32, d_table_reuse, d_table_d32, d_reuse_d32, d_all]

proc main() =
  let m = paramStr(1)
  if m == "check":
    var n = 0
    for set in [32, 64, 128]:
      for seed in 1 .. 30:
        fill(set, seed)
        for i in 0 ..< rows.len:
          (if set == 128: loops128[i] else: loops64[i])(1)
          for j in 0 ..< N:
            doAssert R[j] == Q[j] and R2[j] == Q2[j], rows[i] & " set " & $set
            n += 2
    # zero dividends with divisors of any size, including ones whose low 32 bits are 0 (the 32-bit divide must not
    # be used for them)
    for j in 0 ..< N:
      B[j] = (uint64(j mod 255) + 1) shl (32 + j mod 24)       # nonzero, below 2^63, low 32 bits 0
      Alo[j] = 0; Ahi[j] = 0; A2lo[j] = 0; A2hi[j] = 0
    for i in 0 ..< rows.len:
      for f in [loops64[i], loops128[i]]:
        f(1)
        for j in 0 ..< N:
          doAssert R[j] == 0 and R2[j] == 0, rows[i] & " zero dividends"
          n += 2
    echo "every row gives the exact quotient: ", n, " divisions (3 sets x 30 seeds x 8 rows x 2048, and zero dividends)"
    return
  let set = parseInt(paramStr(2))
  fill(set, 20261031)
  if m == "dump":                   # the operands, for the C (clang) and Rust controls: b alo ahi a2lo a2hi per line
    for j in 0 ..< N: echo B[j], " ", Alo[j], " ", Ahi[j], " ", A2lo[j], " ", A2hi[j]
    return
  let i = rows.find(m)
  doAssert i >= 0
  let f = (if set == 128: loops128[i] else: loops64[i])
  var k = 1
  while true:
    let t0 = now()
    f(k)
    if now() - t0 >= 50_000_000: break
    k *= 2
  var ts: seq[float]
  for r in 0 ..< 15:
    let t0 = now()
    f(k)
    ts.add float(now() - t0) / float(k * 2 * N)
  ts.sort()
  var h = 0'u64
  for j in 0 ..< N: h = (h xor R[j] xor (R2[j] shl 1)) * 0x9E3779B97F4A7C15'u64
  echo m, " ", set, " ", formatFloat(ts[7], ffDecimal, 4), " ", formatFloat((ts[14] - ts[0]) / ts[7], ffDecimal, 4), " ",
       toHex(h)

main()
