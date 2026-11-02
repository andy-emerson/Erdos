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
