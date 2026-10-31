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
