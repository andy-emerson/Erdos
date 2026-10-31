# hybrid2.nim: Hybrid without shifts (round 4). A value below 2^63 is stored as itself, natively, with no allocation;
# a bigger value is a pointer to Growing's block with the top bit set (user-space pointers never use the top bit).
# Both operands native is one test, ((a or b) shr 63) == 0, then the plain machine operation; a result below 2^63 stays
# native, otherwise it becomes a Growing block. Any big operand goes to Growing's routine, and a result below 2^63
# comes back native. W = 64 only. Variants, each its own build:
#   -d:p2     the power-of-2 test, made on demand (no stored flag), in exact division and gcd (special cases)
#   -d:adsub  subtract is the absolute difference |a - b|
import bits, alloc, growing

when W != 64:
  {.error: "the Hybrid build needs W = 64".}

type Val* = uint64
const wordBuild* = false
const top = 1'u64 shl 63

template native(x: Val): bool = (x and top) == 0
template both(a, b: Val): bool = ((a or b) and top) == 0
template big(x: Val): Big = cast[Big](x and not top)
template mark(z: Big): Val = cast[Val](z) or top
template pow2(x: uint64): bool = x != 0 and (x and (x - 1)) == 0

proc fromBig(z: Big): Val {.inline.} =
  ## a result back from Growing: native when below 2^63 (the block is freed), else the block, marked
  if z.len == 0:
    release(z)
    return 0
  if z.len == 1 and z[1] < top:
    let v = z[1]
    release(z)
    return v
  mark(z)

proc toBig(w: openArray[uint64]): Val = mark(growing.fromWords(w))

proc release*(x: Val) {.inline.} =
  if not native(x): growing.release(big(x))

proc fromWords*(w: openArray[uint64]): Val =
  var n = w.len
  while n > 0 and w[n - 1] == 0: dec n
  if n == 0: return 0
  if n == 1 and w[0] < top: return w[0]
  toBig(w)

proc words*(x: Val): seq[uint64] =
  ## the value's words, lowest first (outside the clock: dump and checksum)
  if native(x):
    if x != 0: result.add x
  else:
    let b = big(x)
    for i in 1 .. b.len: result.add uint64(b[i])

template viaGrowing(a, b: Val; routine: untyped): Val =
  var r: Val
  block:
    var la {.noinit.}, lb {.noinit.}: array[2, Word]
    let ba = (if native(a): (la[0] = Word(if a == 0: 0 else: 1); la[1] = a; cast[Big](addr la[0])) else: big(a))
    let bb = (if native(b): (lb[0] = Word(if b == 0: 0 else: 1); lb[1] = b; cast[Big](addr lb[0])) else: big(b))
    r = fromBig(routine(ba, bb))
  r

proc opAdd*(a, b: Val): Val =
  if both(a, b):
    let s = a + b                       # two values below 2^63: the sum fits 64 bits
    if s < top: return s
    return toBig([s])
  viaGrowing(a, b, growing.opAdd)

proc opAbsdiff*(a, b: Val): Val =
  if both(a, b):
    return (if a >= b: a - b else: b - a)
  viaGrowing(a, b, growing.opAbsdiff)

proc opAbsdiff2*(a, b: Val): Val =
  if both(a, b):
    var d: uint64
    let m = 0'u64 - subBorrow(a, b, d)
    return (d xor m) - m
  viaGrowing(a, b, growing.opAbsdiff2)

when defined(adsub):
  proc opSub*(a, b: Val): Val = opAbsdiff(a, b)
else:
  proc opSub*(a, b: Val): Val =
    ## as Growing: when a < b it wraps over one word (mod 2^64)
    if both(a, b):
      let d = a - b
      if d < top: return d
      return toBig([d])
    viaGrowing(a, b, growing.opSub)

proc opMul*(a, b: Val): Val =
  if both(a, b):
    var lo, hi: uint64
    {.emit: ["unsigned __int128 p_ = (unsigned __int128)", a, " * ", b, "; ", lo, " = (NU64)p_; ", hi,
             " = (NU64)(p_ >> 64);"].}
    if hi == 0 and lo < top: return lo
    return toBig([lo, hi])
  viaGrowing(a, b, growing.opMul)

proc opDivexact*(a, b: Val): Val =
  if both(a, b):
    when defined(p2):
      if pow2(b): return a shr ctz(b)  # special case: dividing by a power of 2 is a shift
    return a div b
  viaGrowing(a, b, growing.opDivexact)

proc ctzVal(x: Val): int =
  ## trailing zero bits of a nonzero value
  if native(x): return ctz(x)
  let b = big(x)
  var i = 1
  while b[i] == 0: inc i
  (i - 1) * 64 + ctz(uint64(b[i]))

proc isPow2(x: Val): bool =
  if native(x): return pow2(x)
  let b = big(x)
  for i in 1 ..< b.len:
    if b[i] != 0: return false
  pow2(uint64(b[b.len]))

proc opGcd*(a, b: Val): Val =
  when defined(p2):
    # special case: when either operand is a power of 2, the gcd is 2 to the smaller count of trailing zeros
    if a != 0 and b != 0 and (isPow2(a) or isPow2(b)):
      let s = min(ctzVal(a), ctzVal(b))
      if s < 63: return 1'u64 shl s
      var w = newSeq[uint64](s div 64 + 1)
      w[s div 64] = 1'u64 shl (s mod 64)
      return toBig(w)
  if both(a, b):
    var x = a
    var y = b
    if x == 0: return y
    if y == 0: return x
    let s = ctz(x or y)
    y = y shr ctz(y)
    while x != 0:
      x = x shr ctz(x)
      if y > x: swap y, x
      x -= y
    return y shl s
  viaGrowing(a, b, growing.opGcd)

when defined(mem):
  proc acctOf*(x: Val): int =
    ## a native value is its 8-byte word, as Control's; a big one is its block, as Growing's
    if native(x): 8 else: alloc.acctOf(cast[pointer](big(x)))
  proc heapOf*(x: Val): int =
    if native(x): 0 else: alloc.heapOf(cast[pointer](big(x)))
