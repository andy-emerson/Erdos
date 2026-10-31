# hybrid3.nim: Hybrid-2 with absolute-difference subtraction, its native path inlined (round 5). Two builds:
#   hyb3   one-word values: native below 2^63, else a marked pointer to Growing's block (as hybrid2.nim)
#   hyb3w  (-d:wide) two-word values: native below 2^127, else a marked pointer. A product of two values that each fit
#          one word is native whatever its size, so multiply never allocates for them; the cost is 16 bytes a value.
# Each operation's native path is an inline test plus the machine operation; anything else is a separate, out-of-line
# call to Growing's routine, whose result comes back native when it fits. Subtract is the absolute difference.
import bits, alloc, growing

when W != 64:
  {.error: "the Hybrid build needs W = 64".}

const wordBuild* = false
const top = 1'u64 shl 63

when not defined(wide):
  # ------------------------------------------------------------------------------------------------ one word
  type Val* = uint64

  template native(x: Val): bool = (x and top) == 0
  template both(a, b: Val): bool = ((a or b) and top) == 0
  template big(x: Val): Big = cast[Big](x and not top)
  template mark(z: Big): Val = cast[Val](z) or top

  proc fromBig(z: Big): Val =
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
    if native(x):
      if x != 0: result.add x
    else:
      let b = big(x)
      for i in 1 .. b.len: result.add uint64(b[i])

  template lend(x: Val; buf: var array[2, Word]): Big =
    (if native(x): (buf[0] = Word(if x == 0: 0 else: 1); buf[1] = x; cast[Big](addr buf[0])) else: big(x))

  template slow(name, routine: untyped) =
    proc name(a, b: Val): Val {.noinline.} =
      var la {.noinit.}, lb {.noinit.}: array[2, Word]
      fromBig(routine(lend(a, la), lend(b, lb)))

  slow(slowAdd, growing.opAdd)
  slow(slowAbsdiff, growing.opAbsdiff)
  slow(slowAbsdiff2, growing.opAbsdiff2)
  slow(slowMul, growing.opMul)
  slow(slowDivexact, growing.opDivexact)
  slow(slowGcd, growing.opGcd)

  proc mulOverflow(lo, hi: uint64): Val {.noinline.} = toBig([lo, hi])
  proc addOverflow(s: uint64): Val {.noinline.} = toBig([s])

  proc opAdd*(a, b: Val): Val {.inline.} =
    if both(a, b):
      let s = a + b
      if s < top: return s
      return addOverflow(s)
    slowAdd(a, b)

  proc opAbsdiff*(a, b: Val): Val {.inline.} =
    if both(a, b): return (if a >= b: a - b else: b - a)
    slowAbsdiff(a, b)

  proc opAbsdiff2*(a, b: Val): Val {.inline.} =
    if both(a, b):
      var d: uint64
      let m = 0'u64 - subBorrow(a, b, d)
      return (d xor m) - m
    slowAbsdiff2(a, b)

  proc opSub*(a, b: Val): Val {.inline.} = opAbsdiff(a, b)

  proc opMul*(a, b: Val): Val {.inline.} =
    if both(a, b):
      var lo, hi: uint64
      {.emit: ["unsigned __int128 p_ = (unsigned __int128)", a, " * ", b, "; ", lo, " = (NU64)p_; ", hi,
               " = (NU64)(p_ >> 64);"].}
      if hi == 0 and lo < top: return lo
      return mulOverflow(lo, hi)
    slowMul(a, b)

  proc opDivexact*(a, b: Val): Val {.inline.} =
    if both(a, b): return a div b
    slowDivexact(a, b)

  proc gcdWord(a, b: uint64): uint64 =
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
    y shl s

  proc opGcd*(a, b: Val): Val {.inline.} =
    if both(a, b): return gcdWord(a, b)
    slowGcd(a, b)

  when defined(mem):
    proc acctOf*(x: Val): int =
      if native(x): 8 else: alloc.acctOf(cast[pointer](big(x)))
    proc heapOf*(x: Val): int =
      if native(x): 0 else: alloc.heapOf(cast[pointer](big(x)))

else:
  # ------------------------------------------------------------------------------------------------ two words
  type Val* = object
    lo*, hi*: uint64          # native: the value hi * 2^64 + lo, hi below 2^63; big: hi = top, lo the block

  {.emit: """/*TYPESECTION*/
typedef unsigned __int128 u128;
static inline u128 mk128(NU64 hi, NU64 lo) { return ((u128)hi << 64) | lo; }
static inline int ctz128(u128 x) { NU64 lo = (NU64)x; return lo ? __builtin_ctzll(lo) : 64 + __builtin_ctzll((NU64)(x >> 64)); }
static inline u128 gcd128(u128 x, u128 y) {
  if (x == 0) return y;
  if (y == 0) return x;
  int s = ctz128(x | y);
  y >>= ctz128(y);
  while (x != 0) { x >>= ctz128(x); if (y > x) { u128 t = x; x = y; y = t; } x -= y; }
  return y << s;
}
""".}

  template native(x: Val): bool = x.hi < top
  template both(a, b: Val): bool = (a.hi or b.hi) < top
  template big(x: Val): Big = cast[Big](x.lo)
  template mark(z: Big): Val = Val(lo: cast[uint64](z), hi: top)
  template nat(h, l: uint64): Val = Val(lo: l, hi: h)

  proc fromBig(z: Big): Val =
    if z.len == 0:
      release(z)
      return nat(0, 0)
    if z.len == 1:
      let v = z[1]
      release(z)
      return nat(0, v)
    if z.len == 2 and z[2] < top:
      let v = nat(z[2], z[1])
      release(z)
      return v
    mark(z)

  proc toBig(w: openArray[uint64]): Val = mark(growing.fromWords(w))

  proc release*(x: Val) {.inline.} =
    if not native(x): growing.release(big(x))

  proc fromWords*(w: openArray[uint64]): Val =
    var n = w.len
    while n > 0 and w[n - 1] == 0: dec n
    if n == 0: return nat(0, 0)
    if n == 1: return nat(0, w[0])
    if n == 2 and w[1] < top: return nat(w[1], w[0])
    toBig(w)

  proc words*(x: Val): seq[uint64] =
    if native(x):
      if x.hi != 0: result = @[x.lo, x.hi]
      elif x.lo != 0: result = @[x.lo]
    else:
      let b = big(x)
      for i in 1 .. b.len: result.add uint64(b[i])

  template lend(x: Val; buf: var array[3, Word]): Big =
    (if native(x):
       (buf[0] = Word(if x.hi != 0: 2 elif x.lo != 0: 1 else: 0); buf[1] = x.lo; buf[2] = x.hi;
        cast[Big](addr buf[0]))
     else: big(x))

  template slow(name, routine: untyped) =
    proc name(a, b: Val): Val {.noinline.} =
      var la {.noinit.}, lb {.noinit.}: array[3, Word]
      fromBig(routine(lend(a, la), lend(b, lb)))

  slow(slowAdd, growing.opAdd)
  slow(slowAbsdiff, growing.opAbsdiff)
  slow(slowAbsdiff2, growing.opAbsdiff2)
  slow(slowMul, growing.opMul)
  slow(slowDivexact, growing.opDivexact)
  slow(slowGcd, growing.opGcd)

  proc overflow2(lo, hi, c: uint64): Val {.noinline.} = toBig([lo, hi, c])

  proc opAdd*(a, b: Val): Val {.inline.} =
    if both(a, b):
      var lo, hi: uint64
      {.emit: ["unsigned __int128 s_ = (((unsigned __int128)", a.hi, " << 64) | ", a.lo, ") + (((unsigned __int128)", b.hi, " << 64) | ", b.lo, "); ", lo, " = (NU64)s_; ", hi,
               " = (NU64)(s_ >> 64);"].}
      if hi < top: return nat(hi, lo)
      return overflow2(lo, hi, 0)
    slowAdd(a, b)

  proc opAbsdiff*(a, b: Val): Val {.inline.} =
    if both(a, b):
      var lo, hi: uint64
      {.emit: ["unsigned __int128 x_ = (((unsigned __int128)", a.hi, " << 64) | ", a.lo, "), y_ = (((unsigned __int128)", b.hi, " << 64) | ", b.lo,
               "); unsigned __int128 d_ = x_ >= y_ ? x_ - y_ : y_ - x_; ", lo, " = (NU64)d_; ", hi, " = (NU64)(d_ >> 64);"].}
      return nat(hi, lo)
    slowAbsdiff(a, b)

  proc opAbsdiff2*(a, b: Val): Val {.inline.} =
    if both(a, b):
      var lo, hi: uint64
      {.emit: ["unsigned __int128 x_ = (((unsigned __int128)", a.hi, " << 64) | ", a.lo, "), y_ = (((unsigned __int128)", b.hi, " << 64) | ", b.lo,
               "), d_; int o_ = __builtin_sub_overflow(x_, y_, &d_); unsigned __int128 m_ = (unsigned __int128)0 - (unsigned __int128)o_; d_ = (d_ ^ m_) - m_; ",
               lo, " = (NU64)d_; ", hi, " = (NU64)(d_ >> 64);"].}
      return nat(hi, lo)
    slowAbsdiff2(a, b)

  proc opSub*(a, b: Val): Val {.inline.} = opAbsdiff(a, b)

  proc opMul*(a, b: Val): Val {.inline.} =
    if (a.hi or b.hi) == 0:
      # both fit one word: the product fits two words, and is native below 2^127
      var lo, hi: uint64
      {.emit: ["unsigned __int128 p_ = (unsigned __int128)", a.lo, " * ", b.lo, "; ", lo, " = (NU64)p_; ", hi, " = (NU64)(p_ >> 64);"].}
      if hi < top: return nat(hi, lo)
      return overflow2(lo, hi, 0)
    slowMul(a, b)

  proc opDivexact*(a, b: Val): Val {.inline.} =
    if both(a, b):
      if (a.hi or b.hi) == 0: return nat(0, a.lo div b.lo)
      var lo, hi: uint64
      {.emit: ["unsigned __int128 q_ = (((unsigned __int128)", a.hi, " << 64) | ", a.lo, ") / (((unsigned __int128)", b.hi, " << 64) | ", b.lo, "); ", lo, " = (NU64)q_; ", hi,
               " = (NU64)(q_ >> 64);"].}
      return nat(hi, lo)
    slowDivexact(a, b)

  proc gcdWide(a, b: Val): Val =
    var lo, hi: uint64
    {.emit: ["u128 g_ = gcd128(mk128(", a.hi, ", ", a.lo, "), mk128(", b.hi, ", ", b.lo, ")); ", lo, " = (NU64)g_; ",
             hi, " = (NU64)(g_ >> 64);"].}
    nat(hi, lo)

  proc gcdWord(a, b: uint64): uint64 =
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
    y shl s

  proc opGcd*(a, b: Val): Val {.inline.} =
    if (a.hi or b.hi) == 0: return nat(0, gcdWord(a.lo, b.lo))   # both fit one word: the word gcd
    if both(a, b): return gcdWide(a, b)
    slowGcd(a, b)

  when defined(mem):
    proc acctOf*(x: Val): int =
      if native(x): 16 else: alloc.acctOf(cast[pointer](big(x)))
    proc heapOf*(x: Val): int =
      if native(x): 0 else: alloc.heapOf(cast[pointer](big(x)))
