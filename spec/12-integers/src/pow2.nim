# pow2.nim: the Power-of-2 flag build (spec §1.2). Control's uint64; bit 63 means "is a power of 2"; values have 63
# bits. Every result recomputes the flag. The flag is used in multiply, exact division and gcd.
import bits, bingcd

type Val* = uint64
const wordBuild* = true
const mask = (1'u64 shl 63) - 1

template v(x: Val): uint64 = x and mask
template flagged(x: Val): bool = (x shr 63) != 0

proc mk(r0: uint64): Val {.inline.} =
  let r = r0 and mask
  r or (uint64(r != 0 and (r and (r - 1)) == 0) shl 63)

proc fromWords*(w: openArray[uint64]): Val = mk(if w.len > 0: w[0] else: 0'u64)   # the value mod 2^63

proc opAdd*(a, b: Val): Val {.inline.} = mk(v(a) + v(b))
proc opSub*(a, b: Val): Val {.inline.} = mk(v(a) - v(b))
proc opAbsdiff*(a, b: Val): Val {.inline.} = (if v(a) >= v(b): mk(v(a) - v(b)) else: mk(v(b) - v(a)))
proc opMul*(a, b: Val): Val {.inline.} =
  if flagged(a): mk(v(b) shl ctz(v(a)))
  elif flagged(b): mk(v(a) shl ctz(v(b)))
  else: mk(v(a) * v(b))
proc opDivexact*(a, b: Val): Val {.inline.} =
  if flagged(b): mk(v(a) shr ctz(v(b)))
  elif flagged(a): mk(1'u64 shl (ctz(v(a)) - ctz(v(b))))
  else: mk(v(a) div v(b))
proc opGcd*(a, b: Val): Val {.inline.} =
  if flagged(a) or flagged(b): mk(1'u64 shl min(ctz(v(a)), ctz(v(b))))
  else: mk(bingcd(v(a), v(b), false))
