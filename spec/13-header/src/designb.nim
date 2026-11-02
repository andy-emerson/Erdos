# designb.nim: Design B (spec 13 v0.2 §4). An integer is a pair (P, N), value P - N, two words. A complex number is four
# words (m0, m1, m2, m3) along 1, i, -1, -i. No header. Each value is a record of its words, kept in one list (the
# driver's). Collapse is the only place a magnitude is subtracted; its borrow is a value, and the one branch.
# Each operation writes its result's words one by one into r (no array copy, which gcc would do with vector moves).
import pseudo
export pseudo

type
  BI* = array[2, Word]     # (P, N)
  BC* = array[4, Word]     # (m0, m1, m2, m3)

# ------------------------------------------------------------------------------------------------ integers
proc biAdd*(a, b: BI; r: var BI) {.inline.} =
  let p = a[0] + b[0]
  let n = a[1] + b[1]
  r[0] = p
  r[1] = n
proc biSub*(a, b: BI; r: var BI) {.inline.} =
  ## add the negation, by crossed parts
  let p = a[0] + b[1]
  let n = a[1] + b[0]
  r[0] = p
  r[1] = n
proc biNeg*(a: BI; r: var BI) {.inline.} =
  let p = a[1]
  let n = a[0]
  r[0] = p
  r[1] = n
proc biMul*(a, b: BI; r: var BI) {.inline.} =
  let p = a[0] * b[0] + a[1] * b[1]
  let n = a[0] * b[1] + a[1] * b[0]
  r[0] = p
  r[1] = n

proc absd(p, n: Word): (Word, Word) {.inline.} =
  ## |p - n| and the borrow b of p - n, as in A's add: the difference, negated when b = 1
  let d = p - n
  let b = Word(p < n)
  if b != 0: (0'u64 - d, b) else: (d, b)

proc collapse2(p, n: Word; rp, rn: var Word) {.inline.} =
  ## |P - N| from the subtraction and its borrow; to P when the borrow is 0, else to N; the other part 0
  let d = p - n
  let b = Word(p < n)
  if b != 0:
    rp = 0
    rn = 0'u64 - d
  else:
    rp = d
    rn = 0

proc biCollapse*(a: BI; r: var BI) {.inline.} =
  let p = a[0]
  let n = a[1]
  collapse2(p, n, r[0], r[1])

proc biDiv*(a, b: BI; r: var BI) {.inline.} =
  ## collapse both, divide the magnitudes (64-by-64-bit unsigned divide), A's direction rule: d = (d1 - d2) mod 4 with
  ## d = 2 * borrow; the quotient is stored in part d / 2 (an indexed store), after both parts are zeroed
  let (m1, b1) = absd(a[0], a[1])
  let (m2, b2) = absd(b[0], b[1])
  let d = ((b1 shl 1) - (b2 shl 1)) and 3
  let q = m1 div m2
  r[0] = 0
  r[1] = 0
  r[d shr 1] = q

# ------------------------------------------------------------------------------------------------ complex
proc bcAdd*(a, b: BC; r: var BC) {.inline.} =
  let x0 = a[0] + b[0]
  let x1 = a[1] + b[1]
  let x2 = a[2] + b[2]
  let x3 = a[3] + b[3]
  r[0] = x0
  r[1] = x1
  r[2] = x2
  r[3] = x3
proc bcSub*(a, b: BC; r: var BC) {.inline.} =
  let x0 = a[0] + b[2]
  let x1 = a[1] + b[3]
  let x2 = a[2] + b[0]
  let x3 = a[3] + b[1]
  r[0] = x0
  r[1] = x1
  r[2] = x2
  r[3] = x3
proc bcNeg*(a: BC; r: var BC) {.inline.} =
  ## every part turned by 2
  let x0 = a[2]
  let x1 = a[3]
  let x2 = a[0]
  let x3 = a[1]
  r[0] = x0
  r[1] = x1
  r[2] = x2
  r[3] = x3
proc bcMul*(a, b: BC; r: var BC) {.inline.} =
  ## sixteen products, a part along d1 times a part along d2 into slot (d1 + d2) mod 4
  let x0 = a[0] * b[0] + a[1] * b[3] + a[2] * b[2] + a[3] * b[1]
  let x1 = a[0] * b[1] + a[1] * b[0] + a[2] * b[3] + a[3] * b[2]
  let x2 = a[0] * b[2] + a[1] * b[1] + a[2] * b[0] + a[3] * b[3]
  let x3 = a[0] * b[3] + a[1] * b[2] + a[2] * b[1] + a[3] * b[0]
  r[0] = x0
  r[1] = x1
  r[2] = x2
  r[3] = x3

proc bcCollapse*(a: BC; r: var BC) {.inline.} =
  ## each axis separately: (m0, m2) and (m1, m3) as an integer's (P, N)
  let x0 = a[0]
  let x1 = a[1]
  let x2 = a[2]
  let x3 = a[3]
  collapse2(x0, x2, r[0], r[2])
  collapse2(x1, x3, r[1], r[3])

proc bcDiv*(a, b: BC; r: var BC) {.inline.} =
  ## collapse both, divide the magnitudes, A's direction rule. A value on an axis has at most one nonzero axis after
  ## collapse: the imaginary one when its magnitude is nonzero (a compare written as a value, ax), else the real one.
  ## Direction: real 2 br, imaginary 1 + 2 bi, chosen by the mask 0 - ax; the magnitude is the OR of the two (one is
  ## 0). The quotient is stored in slot (d1 - d2) mod 4 (an indexed store), after all four slots are zeroed.
  let (r1, br1) = absd(a[0], a[2])
  let (i1, bi1) = absd(a[1], a[3])
  let (r2, br2) = absd(b[0], b[2])
  let (i2, bi2) = absd(b[1], b[3])
  let ax1 = opaque(Word(i1 != 0))
  let ax2 = opaque(Word(i2 != 0))
  let d1 = (br1 shl 1) xor (((br1 shl 1) xor (1 + (bi1 shl 1))) and (0'u64 - ax1))
  let d2 = (br2 shl 1) xor (((br2 shl 1) xor (1 + (bi2 shl 1))) and (0'u64 - ax2))
  let q = (r1 or i1) div (r2 or i2)
  r[0] = 0
  r[1] = 0
  r[2] = 0
  r[3] = 0
  r[(d1 - d2) and 3] = q
