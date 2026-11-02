# designcpl.nim: the complement layout (complement-research.md). A number is one 64-bit word:
#   bit 63 sign start s, bit 62 axis a (G = 2^62), bits 0-61 count c; value (c - 2^62 s) * i^a.
# On each axis (s, c) is a 63-bit two's-complement integer, -2^62 .. 2^62 - 1, with the axis bit between its sign bit and
# the rest. The same code serves integers (a = 0) and complex numbers on the axes. Every word is a valid number; the two
# zeros are 0 and G (0 and 0i), and either is accepted where the result is zero.
#
# The formulas are the research's, as the author chose them (Nov 2, 2026):
#   full add       "Branch-free, no selects": 11, plus 5 for the expression flag e (sticky, err |= e, as in abf/alg);
#                  the bridge b = z1 + (w2 OR G), not the re-add;
#   subtract       negate w2 (4), then the full add (16): 20;
#   negate         z = w AND G; d = z - w; r = (d AND NOT G) OR z: 4;
#   multiply       18: sign-extend both (4 + 4), m for i*i = -1 (3), (x1 xor m) - m (2), the product (1), the fix (1 + 3);
#   exact divide   18: the same with m from (w2 AND NOT w1) (a2 = 1, a1 = 0: v / (iu) = -i v/u) applied to the quotient.
# No branches and no selects. opaque (pseudo.nim) keeps gcc from turning the zero test's mask into a cmov or a branch.

import pseudo
export pseudo

const
  G = 1'u64 shl 62          # the axis bit
  NG = not G                # sign and count

template sar(x: Word; k: static int): Word = cast[Word](ashr(cast[int64](x), k))

# ------------------------------------------------------------------------------------------------ negate (4)
proc cNeg*(w: Word; r: var Word) {.inline.} =
  ## d = -(sign and count), the borrow passing through the axis column; the axis written back. -(-2^62) wraps to -2^62.
  let z = w and G
  let d = z - w
  r = (d and NG) or z

# ------------------------------------------------------------------------------------------------ full add (11 + 5)
proc cAddW(w1, w2: Word; err: var Word): Word {.inline.} =
  ## same axis: the sum (wrapping as a 63-bit integer); different axes, one operand zero: the other operand;
  ## two nonzero operands on different axes: e = 1, ORed into err (the driver exits 3 after the pass).
  let t = w1 xor w2                                   # 1
  let z1 = w1 and NG                                  # 1
  let b = z1 + (w2 or G)                              # 2  a bridge on every input (axis column 0 + 1)
  let Z1 = 0'u64 - opaque(Word(z1 == 0))              # 2  all ones when w1 is a zero (opaque: no cmov, no branch)
  let src = w1 xor (t and Z1)                         # 2  w2 if w1 is a zero, else w1
  result = b xor ((b xor src) and G)                  # 3  the axis written back from src
  let z2 = w2 and NG                                  # 1
  err = err or (((t and not Z1) shr 62) and Word(z2 != 0))   # 4  (bit 0: different axes, w1 and w2 nonzero)

proc cAdd*(w1, w2: Word; err: var Word; r: var Word) {.inline.} =
  r = cAddW(w1, w2, err)

proc cSub*(w1, w2: Word; err: var Word; r: var Word) {.inline.} =
  ## w1 + (-w2): the 4-step negate, then the full add (20)
  let z = w2 and G
  let n2 = ((z - w2) and NG) or z
  r = cAddW(w1, n2, err)

# ------------------------------------------------------------------------------------------------ multiply, divide (18)
template sext(w: Word): Word =
  ## 4: copies s into bit 62, so the word is the 64-bit two's complement of the axis value
  w xor ((w xor (w shr 1)) and G)

proc cMul*(w1, w2: Word; r: var Word) {.inline.} =
  ## correct whenever the product fits (i * i = -1); axis a1 xor a2
  let x1 = sext(w1)                                   # 4
  let x2 = sext(w2)                                   # 4
  let m = sar((w1 and w2) shl 1, 63)                  # 3  all ones when both are imaginary
  let p = ((x1 xor m) - m) * x2                       # 3
  r = p xor ((p xor w1 xor w2) and G)                 # 4  (w1 xor w2 gives a1 xor a2 at bit 62)

proc cDiv*(w1, w2: Word; r: var Word) {.inline.} =
  ## exact quotient by the signed divide on the sign-extended values (>= -2^62, so no trap); v / (iu) = -i v/u
  let x1 = sext(w1)                                   # 4
  let x2 = sext(w2)                                   # 4
  let m = sar((w2 and not w1) shl 1, 63)              # 3  all ones when a2 = 1 and a1 = 0
  let q = cast[Word](cast[int64](x1) div cast[int64](x2))   # 1
  let qm = (q xor m) - m                              # 2
  r = qm xor ((qm xor w1 xor w2) and G)               # 4

# ------------------------------------------------------------------------------------------------ the layout
proc cEncode*(axis: int; v: int64): Word =
  ## v on the axis (0 real, 1 imaginary); v in -2^62 .. 2^62 - 1
  let u = cast[Word](v)
  ((u shr 62) and 1) shl 63 or (Word(axis) shl 62) or (u and (G - 1))

proc cDecode*(w: Word): (int, int64) =
  ## (axis, value)
  (int((w shr 62) and 1), int64(w and (G - 1)) - (int64(w shr 63) shl 62))
