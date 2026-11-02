# designa.nim: Design A (spec 13 v0.2 §3). A number is an 8-bit header and a word, kept in two parallel lists (the
# driver's). Header bits 6-7 are the direction d (mod 4, for 1, i, -1, -i), bits 0-5 are 0; D = 0xC0. The same code
# serves integers (d in {0, 2}) and complex numbers on the axes. A zero keeps whatever direction its operation gives.
#
# Two rows: A (the notes' tree, with its direction and borrow branches) and A's branch-free row (§7: the direction and
# borrow branches replaced by masks, 0 - b, AND, XOR; no conditional moves). Negate, multiply and divide have no
# branches and are the same in both rows. Carries and borrows are values (a compare written as a value), never a flag.

type Word* = uint64
const D* = 0xC0'u8

proc specError*(msg: string) {.noinline, noreturn.} =
  ## an error of the spec (a sum across axes of two nonzero numbers, an expression): exit 3
  stderr.writeLine "error: " & msg
  quit 3

template opaque*(x: Word): Word =
  ## x, unchanged, hidden from gcc (no instructions): it cannot reason about the value's range, so it cannot turn the
  ## mask arithmetic that follows back into a branch on it (spec 12 divbench2's opaque)
  var y = x
  {.emit: ["__asm__(\"\" : \"+r\"(", y, "));"].}
  y

# ------------------------------------------------------------------------------------------------ add, the tree
proc aAdd*(h1: uint8; m1: Word; h2: uint8; m2: Word): (uint8, Word) {.inline.} =
  ## t = (h1 xor h2) and D. t = 0: m1 + m2 in direction h1 and D. t = 0x80: m1 - m2 and its borrow b, negated when
  ## b = 1, direction (h1 + (b shl 7)) and D. Otherwise (different axes) a zero operand takes the other's direction and
  ## the add runs as same-direction; two nonzero operands are an error. No other zero test.
  let t = (h1 xor h2) and D
  if t == 0:
    return (h1 and D, m1 + m2)
  if t == 0x80:
    let d = m1 - m2
    let b = Word(m1 < m2)                 # the borrow, as a value
    if b != 0:
      return ((h1 + 0x80'u8) and D, 0'u64 - d)
    return (h1 and D, d)
  var h = h1
  if m1 == 0: h = h2
  elif m2 != 0: specError("sum across axes of two nonzero numbers (an expression)")
  (h and D, m1 + m2)

proc aSub*(h1: uint8; m1: Word; h2: uint8; m2: Word): (uint8, Word) {.inline.} = aAdd(h1, m1, h2 xor 0x80'u8, m2)

# ------------------------------------------------------------------------------------------------ add, branch-free row
proc aAddBF*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word): (uint8, Word) {.inline.} =
  ## the tree's cases as masks, every step run for every input:
  ##   ax = 1 for different axes, o = 1 for opposite (t = 0x80);
  ##   on different axes with m1 = 0, h1's direction is replaced by h2's (mask sel); two nonzero operands set err
  ##   (sticky: the driver tests it after each pass, outside the operation);
  ##   r = m1 + (m2 xor s) + o with s = 0 - o: m1 + m2, or m1 - m2;
  ##   b = (m1 < m2) and o, the borrow; r = (r xor (0 - b)) + b negates it when b = 1; direction + (b shl 7).
  let t = Word((h1 xor h2) and D)
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
  (uint8((hd + (b shl 7)) and Word(D)), (r xor mk) + b)

proc aSubBF*(h1: uint8; m1: Word; h2: uint8; m2: Word; err: var Word): (uint8, Word) {.inline.} =
  aAddBF(h1, m1, h2 xor 0x80'u8, m2, err)

# ------------------------------------------------------------------------------------------------ negate, multiply, divide
proc aNeg*(h: uint8; m: Word): (uint8, Word) {.inline.} = (h xor 0x80'u8, m)
proc aMul*(h1: uint8; m1: Word; h2: uint8; m2: Word): (uint8, Word) {.inline.} = ((h1 + h2) and D, m1 * m2)
proc aDiv*(h1: uint8; m1: Word; h2: uint8; m2: Word): (uint8, Word) {.inline.} =
  ## m1 / m2 by the machine's 64-by-64-bit unsigned divide (m2 /= 0 divides m1), direction (h1 - h2) and D
  ((h1 - h2) and D, m1 div m2)
