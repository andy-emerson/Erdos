# hybrid.nim: the Hybrid build (round 3). Growing, except that a value that fits in 63 bits is stored natively, in the
# word itself, with no allocation: the word holds (v shl 1) or 1. A word with the low bit 0 is a pointer to Growing's
# block (malloc's blocks are aligned, so their low bit is 0). Small operands are computed natively, with the machine's
# overflow detection; a result that fits 63 bits stays native, otherwise it becomes a Growing block. Any big operand
# sends the operation to Growing's routine (a small operand is lent as a one-word block on the stack), and a result
# that fits 63 bits comes back native. W = 64 only.
import bits, alloc, growing

when W != 64:
  {.error: "the Hybrid build needs W = 64".}

type Val* = uint64
const wordBuild* = false
const lim = 1'u64 shl 63

template small(x: Val): bool = (x and 1) == 1
template sv(x: Val): uint64 = x shr 1
template mkSmall(v: uint64): Val = (v shl 1) or 1
template big(x: Val): Big = cast[Big](x)

proc fromBig(z: Big): Val {.inline.} =
  ## a result back from Growing: native when it fits 63 bits (the block is freed), else the block itself
  if z.len == 0:
    release(z)
    return mkSmall(0)
  if z.len == 1 and z[1] < lim:
    let v = z[1]
    release(z)
    return mkSmall(v)
  cast[Val](z)

proc toBig(v: uint64): Big =
  ## a native value as a new block
  let w = [v]
  growing.fromWords(w)

proc release*(x: Val) {.inline.} =
  if x != 0 and not small(x): growing.release(big(x))

proc fromWords*(w: openArray[uint64]): Val =
  var n = w.len
  while n > 0 and w[n - 1] == 0: dec n
  if n == 0: return mkSmall(0)
  if n == 1 and w[0] < lim: return mkSmall(w[0])
  cast[Val](growing.fromWords(w))

proc words*(x: Val): seq[uint64] =
  ## the value's words, lowest first (outside the clock: dump and checksum)
  if small(x):
    if sv(x) != 0: result.add sv(x)
  else:
    let b = big(x)
    for i in 1 .. b.len: result.add uint64(b[i])

template viaGrowing(a, b: Val; routine: untyped): Val =
  var r: Val
  block:
    var la {.noinit.}, lb {.noinit.}: array[2, Word]
    let ba = (if small(a): (la[0] = Word(if sv(a) == 0: 0 else: 1); la[1] = sv(a); cast[Big](addr la[0])) else: big(a))
    let bb = (if small(b): (lb[0] = Word(if sv(b) == 0: 0 else: 1); lb[1] = sv(b); cast[Big](addr lb[0])) else: big(b))
    r = fromBig(routine(ba, bb))
  r

proc opAdd*(a, b: Val): Val =
  if small(a) and small(b):
    let s = sv(a) + sv(b)               # two 63-bit values: the sum fits 64 bits
    if s < lim: return mkSmall(s)
    return cast[Val](toBig(s))
  viaGrowing(a, b, growing.opAdd)

proc opSub*(a, b: Val): Val =
  ## as Growing: when a < b it wraps over one word (mod 2^64)
  if small(a) and small(b):
    let d = sv(a) - sv(b)
    if d < lim: return mkSmall(d)
    return cast[Val](toBig(d))
  viaGrowing(a, b, growing.opSub)

proc opAbsdiff*(a, b: Val): Val =
  if small(a) and small(b):
    return mkSmall(if sv(a) >= sv(b): sv(a) - sv(b) else: sv(b) - sv(a))
  viaGrowing(a, b, growing.opAbsdiff)

proc opAbsdiff2*(a, b: Val): Val =
  if small(a) and small(b):
    var d: uint64
    let m = 0'u64 - subBorrow(sv(a), sv(b), d)
    return mkSmall((d xor m) - m)
  viaGrowing(a, b, growing.opAbsdiff2)

proc opMul*(a, b: Val): Val =
  if small(a) and small(b):
    let x = sv(a)
    let y = sv(b)
    var lo, hi: uint64
    {.emit: ["unsigned __int128 p_ = (unsigned __int128)", x, " * ", y, "; ", lo, " = (NU64)p_; ", hi,
             " = (NU64)(p_ >> 64);"].}
    if hi == 0 and lo < lim: return mkSmall(lo)
    let w = [lo, hi]
    return cast[Val](growing.fromWords(w))
  viaGrowing(a, b, growing.opMul)

proc opDivexact*(a, b: Val): Val =
  if small(a) and small(b):
    return mkSmall(sv(a) div sv(b))
  viaGrowing(a, b, growing.opDivexact)

proc opGcd*(a, b: Val): Val =
  if small(a) and small(b):
    var x = sv(a)
    var y = sv(b)
    if x == 0: return mkSmall(y)
    if y == 0: return mkSmall(x)
    let s = ctz(x or y)
    y = y shr ctz(y)
    while x != 0:
      x = x shr ctz(x)
      if y > x: swap y, x
      x -= y
    return mkSmall(y shl s)
  viaGrowing(a, b, growing.opGcd)

when defined(mem):
  proc acctOf*(x: Val): int =
    ## a native value is its 8-byte word, as Control's; a big one is its block, as Growing's
    if small(x): 8 else: alloc.acctOf(cast[pointer](x))
  proc heapOf*(x: Val): int =
    if small(x): 0 else: alloc.heapOf(cast[pointer](x))
