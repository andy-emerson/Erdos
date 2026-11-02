# designb.nim: Design B (spec 13 v0.2 §4). An integer is a pair (P, N), value P - N, two words. A complex number is four
# words (m0, m1, m2, m3) along 1, i, -1, -i. No header. Each value is a record of its words, kept in one list (the
# driver's). Collapse is the only place a magnitude is subtracted; its borrow is a value and the one branch.

type
  Word* = uint64
  BI* = array[2, Word]     # (P, N)
  BC* = array[4, Word]     # (m0, m1, m2, m3)

# ------------------------------------------------------------------------------------------------ integers
proc biAdd*(a, b: BI): BI {.inline.} = [a[0] + b[0], a[1] + b[1]]
proc biSub*(a, b: BI): BI {.inline.} = [a[0] + b[1], a[1] + b[0]]        # add the negation, by crossed parts
proc biNeg*(a: BI): BI {.inline.} = [a[1], a[0]]
proc biMul*(a, b: BI): BI {.inline.} =
  [a[0] * b[0] + a[1] * b[1], a[0] * b[1] + a[1] * b[0]]

proc absd(p, n: Word): (Word, Word) {.inline.} =
  ## |p - n| and the borrow b of p - n, as in A's add: the difference, negated when b = 1
  let d = p - n
  let b = Word(p < n)
  if b != 0: (0'u64 - d, b) else: (d, b)

proc biCollapse*(a: BI): BI {.inline.} =
  ## |P - N| from the subtraction and its borrow; to P when the borrow is 0, else to N; the other part 0
  let d = a[0] - a[1]
  let b = Word(a[0] < a[1])
  if b != 0: [0'u64, 0'u64 - d] else: [d, 0'u64]

proc biDiv*(a, b: BI): BI {.inline.} =
  ## collapse both, divide the magnitudes (64-by-64-bit unsigned divide), A's direction rule: d = (d1 - d2) mod 4 with
  ## d = 2 * borrow; the quotient is stored in part d / 2, the other part 0
  let (m1, b1) = absd(a[0], a[1])
  let (m2, b2) = absd(b[0], b[1])
  let d = ((b1 shl 1) - (b2 shl 1)) and 3
  result = [0'u64, 0'u64]
  result[d shr 1] = m1 div m2

# ------------------------------------------------------------------------------------------------ complex
proc bcAdd*(a, b: BC): BC {.inline.} = [a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]]
proc bcSub*(a, b: BC): BC {.inline.} = [a[0] + b[2], a[1] + b[3], a[2] + b[0], a[3] + b[1]]
proc bcNeg*(a: BC): BC {.inline.} = [a[2], a[3], a[0], a[1]]            # every part turned by 2
proc bcMul*(a, b: BC): BC {.inline.} =
  ## sixteen products, a part along d1 times a part along d2 into slot (d1 + d2) mod 4
  [a[0] * b[0] + a[1] * b[3] + a[2] * b[2] + a[3] * b[1],
   a[0] * b[1] + a[1] * b[0] + a[2] * b[3] + a[3] * b[2],
   a[0] * b[2] + a[1] * b[1] + a[2] * b[0] + a[3] * b[3],
   a[0] * b[3] + a[1] * b[2] + a[2] * b[1] + a[3] * b[0]]

proc bcCollapse*(a: BC): BC {.inline.} =
  ## each axis separately: (m0, m2) and (m1, m3) as an integer's (P, N)
  let r = biCollapse([a[0], a[2]])
  let i = biCollapse([a[1], a[3]])
  [r[0], i[0], r[1], i[1]]

proc bcDiv*(a, b: BC): BC {.inline.} =
  ## collapse both, divide the magnitudes, A's direction rule. A value on an axis has at most one nonzero axis after
  ## collapse: the imaginary one when its magnitude is nonzero (a compare written as a value, ax), else the real one.
  ## Direction: real 2 br, imaginary 1 + 2 bi, chosen by the mask 0 - ax. The quotient is stored in slot (d1 - d2) mod 4.
  let (r1, br1) = absd(a[0], a[2])
  let (i1, bi1) = absd(a[1], a[3])
  let (r2, br2) = absd(b[0], b[2])
  let (i2, bi2) = absd(b[1], b[3])
  let ax1 = Word(i1 != 0)
  let ax2 = Word(i2 != 0)
  let d1 = (br1 shl 1) xor (((br1 shl 1) xor (1 + (bi1 shl 1))) and (0'u64 - ax1))
  let d2 = (br2 shl 1) xor (((br2 shl 1) xor (1 + (bi2 shl 1))) and (0'u64 - ax2))
  result = [0'u64, 0'u64, 0'u64, 0'u64]
  result[(d1 - d2) and 3] = (r1 or i1) div (r2 or i2)
