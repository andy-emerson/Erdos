## gcdtest.nim: the untimed tests of §17.12 for the gcd (§17.7), on the models' own routine (src/gcd.nim, test build):
## a zero operand; the special case (one remainder first, 6a) on either side, at size differences of 8 bits and 9
## bits, with the path itself counted (src/gcd.nim's test-build counter): not taken at 8, taken at 9; a zero
## remainder; and random pairs. Every value against Euclid's algorithm and against 08's own gcd (sk_gcd_raw of
## lib/vendor08/sk_internal.h, called as C). Built and run by test/run_tests.sh.

import ../src/core

proc euclid(p, q: int64): int64 =
  var (u, v) = (mag(p), mag(q))
  while v != 0: (u, v) = (v, u mod v)
  cast[int64](u)

var failures, checks = 0

proc check(p, q: int64, why: string, remainders = -1) =
  ## remainders >= 0: the times each of gcd(p, q) and gcd(q, p) must take the remainder-first path
  inc checks
  let before = remainderFirst
  let g1 = gcd(p, q)
  let mid = remainderFirst
  let g2 = gcd(q, p)
  if remainders >= 0 and (mid - before != remainders or remainderFirst - mid != remainders):
    inc failures
    echo "FAIL ", why, ": gcd(", p, ", ", q, ") took the remainder-first path ", mid - before, " and ",
      remainderFirst - mid, " times, not ", remainders
  if g1 != g2: inc failures
  let want = euclid(p, q)
  if gcd(p, q) != want or gcd08(p, q) != want or gcd(q, p) != want:
    inc failures
    echo "FAIL ", why, ": gcd(", p, ", ", q, ") = ", gcd(p, q), ", 08's ", gcd08(p, q), ", Euclid ", want

# a zero operand
check(0, 0, "both zero")
check(0, 12, "u = 0")
check(35, 0, "v = 0")
check(0, -(1'i64 shl 62), "u = 0, v negative")
# the special case: bit lengths differing by 8 (binary method only) and by 9 (one remainder first), on either side
for (u, v) in [(0x1FFFF'i64, 0x1FF'i64), (0x1FFFF'i64, 0x100'i64), (0x10000'i64, 0x1FF'i64)]:
  # bit lengths 17 and 9: a difference of 8
  doAssert bitLen(uint64(u)) - bitLen(uint64(v)) == 8
  check(u, v, "difference 8, u larger", remainders = 0)
  check(v, u, "difference 8, v larger", remainders = 0)
for (u, v) in [(0x3FFFF'i64, 0x1FF'i64), (0x20000'i64, 0x100'i64), (0x30003'i64, 0x1FB'i64)]:
  doAssert bitLen(uint64(u)) - bitLen(uint64(v)) == 9
  check(u, v, "difference 9, u larger", remainders = 1)
  check(v, u, "difference 9, v larger", remainders = 1)
# a zero remainder in the special case: the smaller divides the larger
check(5'i64 shl 20, 5, "zero remainder, u larger", remainders = 1)
check(7, 7'i64 * 3 shl 30, "zero remainder, v larger", remainders = 1)
check(int64.high, 1, "2^63 - 1 and 1")
check(-int64.high, 3, "a negative operand")
# random pairs, of every size difference
var x = 0x2545F4914F6CDD1D'u64
proc next(): uint64 =
  x = x xor (x shl 13); x = x xor (x shr 7); x = x xor (x shl 17); x
for i in 0 ..< 200000:
  let a = int64(next() shr (1 + next() mod 63))
  let b = int64(next() shr (1 + next() mod 63))
  let g = int64(next() shr (40 + next() mod 23)) + 1
  check(a, b, "random")
  if a <= int64.high div g and b <= int64.high div g: check(a * g, b * g, "random, common factor")
echo "gcd: ", checks, " checks, ", failures, " failures"
quit(if failures == 0: 0 else: 1)
