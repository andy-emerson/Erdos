# bits.nim: gcc's count-trailing/leading-zero builtins (the compiler's, no library). Undefined at 0, as the builtins.
proc builtinCtz64(x: uint64): cint {.importc: "__builtin_ctzll", nodecl.}
proc builtinClz64(x: uint64): cint {.importc: "__builtin_clzll", nodecl.}
proc builtinCtz32(x: uint32): cint {.importc: "__builtin_ctz", nodecl.}
proc builtinClz32(x: uint32): cint {.importc: "__builtin_clz", nodecl.}

template ctz*(x: uint64): int = int(builtinCtz64(x))
template clz*(x: uint64): int = int(builtinClz64(x))
template ctz*(x: uint32): int = int(builtinCtz32(x))
template clz*(x: uint32): int = int(builtinClz32(x))

proc subBorrow*(a, b: uint64; d: var uint64): uint64 {.inline.} =
  ## d = a - b (wrapping); returns 1 when it borrowed (a < b), from the subtraction itself
  var o: bool
  {.emit: [o, " = __builtin_sub_overflow(", a, ", ", b, ", &", d, ");"].}
  uint64(o)

# ------------------------------------------------------------------------------------------------ machine-level operations (round 6)
proc divExactInv*(a, b: uint64): uint64 {.inline.} =
  ## exact division (b divides a) with no divide instruction: shift out b's trailing zeros, invert b's odd part mod 2^64
  ## by Newton's iteration from a 5-bit start, doubling each step, then multiply. b = 0 gives no meaningful result.
  let s = ctz(b or (1'u64 shl 63))
  let o = b shr s
  var x = (3'u64 * o) xor 2   # correct to 5 bits for any odd o (a standard starting value)
  x = x * (2'u64 - o * x)    # 10 bits
  x = x * (2'u64 - o * x)    # 20
  x = x * (2'u64 - o * x)    # 40
  x = x * (2'u64 - o * x)    # 80
  (a shr s) * x

proc usubAsm*(a, b: uint64): uint64 {.inline.} =
  ## |a - b| from the subtraction's own borrow: sub sets the carry when a < b; sbb turns it into a mask (all ones or
  ## zeros); xor and sub with the mask negate the difference exactly when it wrapped.
  var r = a
  var m: uint64
  {.emit: ["__asm__(\"subq %2, %0\\n\\tsbbq %1, %1\\n\\txorq %1, %0\\n\\tsubq %1, %0\" : \"+r\"(", r, "), \"=&r\"(", m,
           ") : \"r\"(", b, ") : \"cc\");"].}
  r

# ------------------------------------------------------------------------------------------------ exact division methods (round 7)
template oddPart(b: uint64; s: var int): uint64 =
  s = ctz(b or (1'u64 shl 63))
  b shr s

proc divPar*(a, b: uint64): uint64 {.inline.} =
  ## the inverse by the parallel form: x0 (5 bits), y = 1 - o*x0, x = x0 (1+y)(1+y^2)(1+y^4)(1+y^8): o*x = 1 - y^16,
  ## correct to 80 bits. The squarings of y and the products into x do not wait on each other.
  var s: int
  let o = oddPart(b, s)
  let x0 = (3'u64 * o) xor 2
  var y = 1'u64 - o * x0
  var x = x0 * (1'u64 + y)
  y = y * y
  x = x * (1'u64 + y)
  y = y * y
  x = x * (1'u64 + y)
  y = y * y
  x = x * (1'u64 + y)
  (a shr s) * x

const invTable* = block:
  ## the inverse of each odd byte o mod 256, at index o shr 1 (128 bytes)
  var t: array[128, uint8]
  for i in 0 .. 127:
    let o = uint8(2 * i + 1)
    var x = o
    for k in 0 .. 3: x = x * (2'u8 - o * x)
    t[i] = x
  t

proc divTable*(a, b: uint64): uint64 {.inline.} =
  ## the inverse from an 8-bit table start, then 3 Newton steps (16, 32, 64 bits)
  var s: int
  let o = oddPart(b, s)
  var x = uint64(invTable[(o shr 1) and 127])
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  x = x * (2'u64 - o * x)
  (a shr s) * x

proc div32*(a, b: uint64): uint64 {.inline.} =
  ## special case: when a and b both fit 32 bits, the 32-bit hardware divide; otherwise the 64-bit one. b must be in
  ## the test: when a = 0, b can be any size, and a b whose low 32 bits are 0 would divide by zero
  if (a or b) < (1'u64 shl 32): uint64(uint32(a) div uint32(b)) else: a div b

proc divFp*(a, b: uint64): uint64 {.inline.} =
  ## special case: when a is below 2^53 (so is b), divide as doubles; the true quotient is a whole number below 2^53,
  ## so the correctly rounded double quotient is exact. Otherwise the 64-bit hardware divide.
  if a < (1'u64 shl 53): uint64(float64(a) / float64(b)) else: a div b

template inverseOf*(b: uint64; s: var int): uint64 =
  ## the shift and the inverse for b, computed once (the reuse method)
  block:
    let o = oddPart(b, s)
    var x = (3'u64 * o) xor 2
    x = x * (2'u64 - o * x)
    x = x * (2'u64 - o * x)
    x = x * (2'u64 - o * x)
    x = x * (2'u64 - o * x)
    x
