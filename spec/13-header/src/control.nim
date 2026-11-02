# control.nim: Control (spec 13 v0.2 §2). The machine's signed 64-bit integers, two's complement, with their own add,
# subtract, negate, multiply and signed divide. A complex number on the axes is a pair (re, im) of them, with the usual
# rules: multiply by four products; exact division of a pair by a number on an axis (coordinator's call).
# Operands are chosen so that nothing overflows (§1): every magnitude and every result's part is below 2^63.

type
  CI* = int64
  CC* = object
    re*, im*: int64

# ------------------------------------------------------------------------------------------------ integers
proc ciAdd*(a, b: CI): CI {.inline.} = a + b
proc ciSub*(a, b: CI): CI {.inline.} = a - b
proc ciNeg*(a: CI): CI {.inline.} = 0'i64 - a
proc ciMul*(a, b: CI): CI {.inline.} = a * b
proc ciDiv*(a, b: CI): CI {.inline.} = a div b        # the machine's signed divide (b divides a, b /= 0)

# ------------------------------------------------------------------------------------------------ complex, as a pair
proc ccAdd*(a, b: CC): CC {.inline.} = CC(re: a.re + b.re, im: a.im + b.im)
proc ccSub*(a, b: CC): CC {.inline.} = CC(re: a.re - b.re, im: a.im - b.im)
proc ccNeg*(a: CC): CC {.inline.} = CC(re: 0'i64 - a.re, im: 0'i64 - a.im)
proc ccMul*(a, b: CC): CC {.inline.} =
  ## the four products: (ar br - ai bi, ar bi + ai br)
  CC(re: a.re * b.re - a.im * b.im, im: a.re * b.im + a.im * b.re)
proc ccDiv*(a, b: CC): CC {.inline.} =
  ## a / b, b a nonzero number on an axis that divides a: by b.re when b is real, (ar + ai i) / (bi i) = ai / bi - (ar / bi) i
  ## when imaginary. The test of which axis b lies on is a branch.
  if b.im == 0: CC(re: a.re div b.re, im: a.im div b.re)
  else: CC(re: a.im div b.im, im: 0'i64 - a.re div b.im)
