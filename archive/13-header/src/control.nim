# control.nim: Control (spec 13 v0.2 §2). The machine's signed 64-bit integers, two's complement, with their own add,
# subtract, negate, multiply and signed divide. A complex number on the axes is a pair (re, im) of them, with the usual
# rules: multiply by four products; exact division of a pair by a number on an axis (coordinator's call).
# Operands are chosen so that nothing overflows (§1): every magnitude and every result's part is below 2^63.
# Each operation writes its result's words one by one into r (no struct copy, which gcc would do with vector moves).

type
  CI* = int64
  CC* = object
    re*, im*: int64

# ------------------------------------------------------------------------------------------------ integers
proc ciAdd*(a, b: CI; r: var CI) {.inline.} = r = a + b
proc ciSub*(a, b: CI; r: var CI) {.inline.} = r = a - b
proc ciNeg*(a: CI; r: var CI) {.inline.} = r = 0'i64 - a
proc ciMul*(a, b: CI; r: var CI) {.inline.} = r = a * b
proc ciDiv*(a, b: CI; r: var CI) {.inline.} = r = a div b    # the machine's signed divide (b /= 0 divides a)

# ------------------------------------------------------------------------------------------------ complex, as a pair
proc ccAdd*(a, b: CC; r: var CC) {.inline.} =
  r.re = a.re + b.re
  r.im = a.im + b.im
proc ccSub*(a, b: CC; r: var CC) {.inline.} =
  r.re = a.re - b.re
  r.im = a.im - b.im
proc ccNeg*(a: CC; r: var CC) {.inline.} =
  r.re = 0'i64 - a.re
  r.im = 0'i64 - a.im
proc ccMul*(a, b: CC; r: var CC) {.inline.} =
  ## the four products: (ar br - ai bi, ar bi + ai br)
  let re = a.re * b.re - a.im * b.im
  let im = a.re * b.im + a.im * b.re
  r.re = re
  r.im = im
proc ccDiv*(a, b: CC; r: var CC) {.inline.} =
  ## a / b, b a nonzero number on an axis that divides a: by b.re when b is real; when b = bi i,
  ## (ar + ai i) / (bi i) = ai / bi - (ar / bi) i. Which axis b lies on is a branch.
  if b.im == 0:
    let re = a.re div b.re
    let im = a.im div b.re
    r.re = re
    r.im = im
  else:
    let re = a.im div b.im
    let im = 0'i64 - a.re div b.im
    r.re = re
    r.im = im
