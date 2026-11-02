# designa.nim: Design A (spec 13 v0.2 §3). A number is an 8-bit header and a word, kept in two parallel lists (the
# driver's). Header bits 6-7 are the direction d (mod 4, for 1, i, -1, -i), bits 0-5 are 0, so no rule masks with D = 0xC0 (the
# author's change of Nov 1, 2026; lean/Header/HeaderByte.lean). The same code
# serves integers (d in {0, 2}) and complex numbers on the axes. A zero keeps whatever direction its operation gives.
#
# Two rows: A (the notes' tree, with its direction and borrow branches) and A's branch-free row (§7: the direction and
# borrow branches replaced by masks, 0 - b, AND, XOR; no conditional moves). Negate, multiply and divide have no
# branches and are the same in both rows. Carries and borrows are values (a compare written as a value), never a flag.

import pseudo
export pseudo


# Each operation writes its result's header and word into rh and rw (the result lists' entries).

# ------------------------------------------------------------------------------------------------ add, the tree
proc aAdd*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  ## t = h1 xor h2. t = 0: m1 + m2 in direction h1. t = 0x80: m1 - m2 and its borrow b, negated when
  ## b = 1, direction h1 + (b shl 7). Otherwise (different axes) a zero operand takes the other's direction and
  ## the add runs as same-direction; two nonzero operands are an error. No other zero test.
  let t = h1 xor h2
  if t == 0:
    rh = h1
    rw = m1 + m2
  elif t == 0x80:
    let d = m1 - m2
    let b = Word(m1 < m2)                 # the borrow, as a value
    if b != 0:
      rh = h1 + 0x80'u8
      rw = 0'u64 - d
    else:
      rh = h1
      rw = d
  else:
    var h = h1
    if m1 == 0: h = h2
    elif m2 != 0: specError()
    rh = h
    rw = m1 + m2

proc aSub*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  aAdd(h1, m1, h2 xor 0x80'u8, m2, rh, rw)

# ------------------------------------------------------------------------------------------------ add, narrow mask row
proc aAddNM*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  ## the tree, with its direction and axis branches, but no borrow branch: on opposite directions the borrow b
  ## negates the difference by the mask, (d xor (0 - b)) + b, and turns the direction by h1 + (b shl 7).
  let t = h1 xor h2
  if t == 0:
    rh = h1
    rw = m1 + m2
  elif t == 0x80:
    let d = m1 - m2
    let b = Word(m1 < m2)                 # the borrow, as a value
    rh = h1 + (uint8(b) shl 7)
    rw = (d xor (0'u64 - b)) + b
  else:
    var h = h1
    if m1 == 0: h = h2
    elif m2 != 0: specError()
    rh = h
    rw = m1 + m2

proc aSubNM*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  aAddNM(h1, m1, h2 xor 0x80'u8, m2, rh, rw)

# ------------------------------------------------------------------------------------------------ add, branch-free row
proc aAddBF*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  ## the tree's cases as masks, every step run for every input:
  ##   ax = 1 for different axes, o = 1 for opposite (t = 0x80);
  ##   on different axes with m1 = 0, h1's direction is replaced by h2's (mask sel); two nonzero operands set err
  ##   (sticky: the driver tests it after each pass, outside the operation);
  ##   r = m1 + (m2 xor s) + o with s = 0 - o: m1 + m2, or m1 - m2;
  ##   b = (m1 < m2) and o, the borrow; r = (r xor (0 - b)) + b negates it when b = 1; direction + (b shl 7).
  let t = Word(h1 xor h2)
  let ax = (t shr 6) and 1
  let o = (t shr 7) and (ax xor 1)
  let z1 = Word(m1 == 0)
  let nz2 = Word(m2 != 0)
  err = err or (ax and (z1 xor 1) and nz2)
  let sel = 0'u64 - (ax and z1)
  let hd = Word(h1) xor ((Word(h1) xor Word(h2)) and sel)
  let s = 0'u64 - o
  let r = m1 + (m2 xor s) + o
  let b = Word(m1 < m2) and o
  let mk = 0'u64 - b
  rh = uint8(hd) + (uint8(b) shl 7)
  rw = (r xor mk) + b

# ------------------------------------------------------------------------------------------------ add, multiplication row
proc aAddMul*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  ## the notes' exponent rule as code, no branch (the author's request, Nov 1, 2026). Signs are applied by multiplying
  ## by 1 - 2k, which is 1 or -1 on the clock, and the cross-axis choice of direction by multiplying by a 0/1 flag:
  ##   ax = 1 for different axes, o = 1 for opposite directions on one axis;
  ##   r = m1 + m2 * (1 - 2o): m1 + m2, or m1 - m2;
  ##   b = (m1 < m2) and o, the borrow; magnitude r * (1 - 2b);
  ##   direction h1 + k * (h2 - h1) with k = ax and (m1 = 0), then + (b shl 7);
  ##   two nonzero operands on different axes set err (sticky, as in the branch-free row).
  let t = Word(h1 xor h2)
  let ax = (t shr 6) and 1
  let o = (t shr 7) and (ax xor 1)
  let z1 = Word(m1 == 0)
  let nz2 = Word(m2 != 0)
  err = err or (ax and (z1 xor 1) and nz2)
  let r = m1 + m2 * opaque(1'u64 - 2'u64 * o)   # opaque: gcc keeps the multiply (no cmov, no select)
  let b = Word(m1 < m2) and o
  rw = r * opaque(1'u64 - 2'u64 * b)
  let k = opaque(ax and z1)
  let hd = Word(h1) + k * (Word(h2) - Word(h1))
  rh = uint8(hd and 0xFF) + (uint8(b) shl 7)

proc aSubMul*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  aAddMul(h1, m1, h2 xor 0x80'u8, m2, err, rh, rw)

# ------------------------------------------------------------------------------------------------ add, header form (larger)
proc aAddLgX(g1: Word; m1: Word; g2: Word; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  ## the header form's add (prior-art.md, "Header form"; lean/Header/LargerRow.lean, ProgLg.lean), no branch. g1, g2
  ## are the headers loaded sign-extended (bit 7 fills bits 8-63):
  ##   t = g1 xor g2, s = t sar 63 (all ones when the sign bits differ);
  ##   r = m1 + (m2 xor s) - s: m1 + m2, or m1 - m2; L = 0 - (m1 < m2); b = s and L, the borrow;
  ##   magnitude (r xor b) - b; direction g1 xor (t and L) (the larger operand's), stored as a byte;
  ##   err = (t shr 6) and (m1 != 0) and (m2 != 0), two nonzero operands on different axes (sticky, as in abf).
  ## Two zeros on different axes keep x's direction, where the tree gives y's (both are zero; LargerRow.lean).
  let t = g1 xor g2
  let s = cast[Word](ashr(cast[int64](t), 63))
  let r = m1 + (m2 xor s) - s
  let L = 0'u64 - opaque(Word(m1 < m2))   # opaque: gcc keeps the compare as a value (no cmov, no sbb)
  let b = s and L
  rw = (r xor b) - b
  rh = uint8((g1 xor (t and L)) and 0xFF)
  err = err or ((t shr 6) and Word(m1 != 0) and Word(m2 != 0))

template sx8(h: uint8): Word = cast[Word](int64(cast[int8](h)))   # sign-extending byte load (movsx, ldrsb, lb)

proc aAddLg*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  aAddLgX(sx8(h1), m1, sx8(h2), m2, err, rh, rw)

proc aSubLg*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  ## h2 flipped by xor -128 on the sign-extended register (bits 8-63 stay equal to bit 7)
  aAddLgX(sx8(h1), m1, sx8(h2) xor 0xFFFF_FFFF_FFFF_FF80'u64, m2, err, rh, rw)

proc aSubBF*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word; rh: var uint8; rw: var Word) {.inline.} =
  aAddBF(h1, m1, h2 xor 0x80'u8, m2, err, rh, rw)

# ------------------------------------------------------------------------------------------------ negate, multiply, divide
proc aNeg*(h: uint8; m: Word; rh: var uint8; rw: var Word) {.inline.} =
  rh = h xor 0x80'u8
  rw = m
proc aMul*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  rh = h1 + h2
  rw = m1 * m2
proc aDiv*(h1: uint8; m1: Word; h2: uint8; m2: Word; rh: var uint8; rw: var Word) {.inline.} =
  ## m1 / m2 by the machine's 64-by-64-bit unsigned divide (m2 /= 0 divides m1), direction h1 - h2
  rh = h1 - h2
  rw = m1 div m2
