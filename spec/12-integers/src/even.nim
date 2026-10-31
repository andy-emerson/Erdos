# even.nim: the Even flag build (spec §1.3). As pow2.nim, with bit 63 meaning "is even". The flag is used only in gcd,
# to skip the shared-power step when an operand is odd.
import bits, bingcd

type Val* = uint64
const wordBuild* = true
const mask = (1'u64 shl 63) - 1

template v(x: Val): uint64 = x and mask
template flagged(x: Val): bool = (x shr 63) != 0

proc mk(r0: uint64): Val {.inline.} =
  let r = r0 and mask
  r or (uint64((r and 1) == 0) shl 63)

proc fromWords*(w: openArray[uint64]): Val = mk(if w.len > 0: w[0] else: 0'u64)   # the value mod 2^63

proc opAdd*(a, b: Val): Val {.inline.} = mk(v(a) + v(b))
proc opSub*(a, b: Val): Val {.inline.} = mk(v(a) - v(b))
proc opAbsdiff*(a, b: Val): Val {.inline.} = (if v(a) >= v(b): mk(v(a) - v(b)) else: mk(v(b) - v(a)))
proc opAbsdiff2*(a, b: Val): Val {.inline.} =
  var d: uint64
  let m = 0'u64 - subBorrow(v(a), v(b), d)
  mk((d xor m) - m)
proc opMul*(a, b: Val): Val {.inline.} = mk(v(a) * v(b))
proc opDivexact*(a, b: Val): Val {.inline.} = mk(v(a) div v(b))
proc opGcd*(a, b: Val): Val {.inline.} = mk(bingcd(v(a), v(b), true, flagged(a) and flagged(b)))
