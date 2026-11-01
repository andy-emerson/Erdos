# designa.nim: Design A (spec 13 §2), magnitude and direction. A number is an 8-bit header (bits 0-4 class, bit 5
# spare 0, bits 6-7 direction d mod 4 for 1, i, -1, -i) and a word, kept in two parallel lists (one stored structure).
# The mathematics is book/notes-designs-a-b.md, Design A. A zero keeps whatever direction its operation gives.
#
# -d:branchfree builds A's branch-free row (§5 c): add's and compare's sign and borrow branches replaced by masks
# (0 - b, AND, XOR); loops over lengths, the class structure and the axis check kept. Everything else is the same.
import std/strutils
import routines, pools

const D = 0xC0'u8

type AV* = object
  h*: uint8
  w*: Word

template cl(h: uint8): int = int(h and 31)

proc release*(h: uint8; w: Word) {.inline.} =
  let c = cl(h)
  if c != 0: slotRelease(c, w)

proc make*(d: int; m: openArray[Word]): AV =
  ## outside the operations: magnitude m (words, lowest first) in direction d
  let (c, w) = partMake(m)
  AV(h: uint8(d shl 6) or uint8(c), w: w)

template done(r: var Res; n: int; dir: uint8): AV =
  let f = finish(r, n)
  AV(h: (dir and D) or uint8(f[0]), w: f[1])

# ------------------------------------------------------------------------------------------------ add, subtract
when not defined(branchfree):
  proc addA(h1in: uint8; w1: Word; h2: uint8; w2: Word): AV {.inline.} =
    ## the notes' tree, on t = (h1 xor h2) and D: same, m1 + m2 in direction h1 and D; opposite, |m1 - m2| with its
    ## borrow b, direction (h1 and D) + (b shl 7); different axes, a zero takes the other's direction and the add runs
    ## as same-direction, two nonzero operands an error (an expression). No other zero test.
    var h1 = h1in
    let c1 = cl(h1)
    let c2 = cl(h2)
    let n1 = partLen(c1, w1)
    let n2 = partLen(c2, w2)
    var t = (h1 xor h2) and D
    if (t and 0x40) != 0:                     # different axes
      if n1 == 0: h1 = (h1 and not D) or (h2 and D)
      elif n2 == 0: discard
      else: specError("sum across axes (an expression)")
      t = 0
    var l1 = w1
    var l2 = w2
    if t == 0:                                # same direction
      var r = resNew(imax(n1, n2) + 1)
      let x = opnd(c1, w1, l1)
      let y = opnd(c2, w2, l2)
      let n = rAdd(resDst(r), x, n1, y, n2)
      return done(r, n, h1 and D)
    var r = resNew(imax(n1, n2))               # opposite
    let x = opnd(c1, w1, l1)
    let y = opnd(c2, w2, l2)
    var br: Word
    let n = rAbsdiff(resDst(r), x, n1, y, n2, br)
    done(r, n, (h1 and D) + uint8(br shl 7))

else:
  proc addA(h1in: uint8; w1: Word; h2: uint8; w2: Word): AV {.inline.} =
    ## the branch-free row: same and opposite in one loop, r = m1 + (m2 xor s) + o over max words, with o = 1 for
    ## opposite and s = 0 - o; the carry out is the top word (same) or 1 - b (opposite); then the negation on the
    ## borrow as a mask pass, m = 0 - b, (r xor m) + b. The top word's store goes to a sink when opposite (an address
    ## mask), since the slot holds only max words then.
    var h1 = h1in
    let c1 = cl(h1)
    let c2 = cl(h2)
    let n1 = partLen(c1, w1)
    let n2 = partLen(c2, w2)
    var t = (h1 xor h2) and D
    if (t and 0x40) != 0:                     # different axes: the axis branch is kept (never taken by integers)
      if n1 == 0: h1 = (h1 and not D) or (h2 and D)
      elif n2 == 0: discard
      else: specError("sum across axes (an expression)")
      t = 0
    let o = Word(t shr 7)                     # 1: opposite
    let s = 0'u64 - o
    let mx = imax(n1, n2)
    let mn = imin(n1, n2)
    var r = resNew(mx + 1 - int(o))
    var l1 = w1
    var l2 = w2
    let x = opnd(c1, w1, l1)
    let y = opnd(c2, w2, l2)
    let d = resDst(r)
    var c = o
    for i in 0 ..< mn:
      let u = x[i] + c
      let k1 = Word(u < c)
      let z = u + (y[i] xor s)
      d[i] = z
      c = k1 + Word(z < u)
    if n1 >= n2:
      for i in mn ..< n1:
        let u = x[i] + c
        let k1 = Word(u < c)
        let z = u + s
        d[i] = z
        c = k1 + Word(z < u)
    else:
      for i in mn ..< n2:
        let z = (y[i] xor s) + c
        d[i] = z
        c = Word(z < c)
    var sink: Word
    let tp = cast[ptr Word]((cast[uint](addr d[mx]) and not uint(s)) or (cast[uint](addr sink) and uint(s)))
    tp[] = c
    let b = (c xor 1) and o                   # the borrow (opposite only)
    let m = 0'u64 - b
    var cc = b
    for i in 0 ..< mx:
      let z = (d[i] xor m) + cc
      cc = Word(z < cc)
      d[i] = z
    let n = norm(d, mx + 1 - int(o))
    done(r, n, (h1 and D) + uint8(b shl 7))

proc opAdd*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} = addA(h1, w1, h2, w2)
proc opSub*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  ## add with h2 xor 0x80, copying nothing
  addA(h1, w1, h2 xor 0x80, w2)

# ------------------------------------------------------------------------------------------------ negate
proc opNeg*(h: uint8; w: Word): AV {.noinline.} =
  ## h xor 0x80, zero too; a pool number's slot copied into a new slot
  let (c, j) = partCopy(cl(h), w)
  AV(h: ((h and not 31'u8) or uint8(c)) xor 0x80, w: j)

# ------------------------------------------------------------------------------------------------ multiply
proc opMul*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  ## m1 * m2 by multiply-accumulate into r = 0 (bound n1 + n2), direction (h1 and D) + (h2 and D)
  let c1 = cl(h1)
  let c2 = cl(h2)
  let n1 = partLen(c1, w1)
  let n2 = partLen(c2, w2)
  var r = resNew(n1 + n2)
  var l1 = w1
  var l2 = w2
  let x = opnd(c1, w1, l1)
  let y = opnd(c2, w2, l2)
  let n = rMac(resDst(r), 0, x, n1, y, n2)
  done(r, n, (h1 and D) + (h2 and D))

# ------------------------------------------------------------------------------------------------ exact division, gcd
template loadBuf(buf: Words; c1: int; w1: Word; n1: int; c2: int; w2: Word; n2: int) =
  ## Growing's working buffer: the two operands copied in, a then b
  var l1 = w1
  var l2 = w2
  let x = opnd(c1, w1, l1)
  let y = opnd(c2, w2, l2)
  for i in 0 ..< n1: buf[i] = x[i]
  for i in 0 ..< n2: buf[n1 + i] = y[i]

proc opDiv*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  ## m1 / m2 (m2 nonzero, dividing m1), direction (h1 and D) - (h2 and D)
  let c1 = cl(h1)
  let c2 = cl(h2)
  let n1 = partLen(c1, w1)
  let n2 = partLen(c2, w2)
  var r = resNew(imax(n1 - n2 + 1, 0))
  var n: int
  withScratch(n1 + n2, buf):
    loadBuf(buf, c1, w1, n1, c2, w2, n2)
    n = rDivCore(resDst(r), buf, 0, n1, n1, n2, n1, n2)
  done(r, n, (h1 and D) - (h2 and D))

proc opGcd*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  ## gcd(m1, m2), direction 0; bound imin(n1, n2), or the nonzero operand's length; gcd(0, 0) an error
  let c1 = cl(h1)
  let c2 = cl(h2)
  let n1 = partLen(c1, w1)
  let n2 = partLen(c2, w2)
  if n1 == 0 and n2 == 0: specError("gcd(0, 0)")
  var r = resNew(if n1 != 0 and n2 != 0: imin(n1, n2) else: imax(n1, n2))
  var n: int
  withScratch(n1 + n2, buf):
    loadBuf(buf, c1, w1, n1, c2, w2, n2)
    n = rGcdCore(resDst(r), buf, 0, n1, n1, n2)
  done(r, n, 0)

# ------------------------------------------------------------------------------------------------ compare
when not defined(branchfree):
  proc opCmp*(h1: uint8; w1: Word; h2: uint8; w2: Word): int {.noinline.} =
    ## ordered along the shared axis, 1 and i positive; zero lies on both. Two zeros equal; one zero, the other's
    ## sign decides; opposite directions, the positive one first; same direction, magnitudes, reversed when negative.
    ## Different axes are not ordered (an error).
    let c1 = cl(h1)
    let c2 = cl(h2)
    let d1 = h1 shr 6
    let d2 = h2 shr 6
    let n1 = partLen(c1, w1)
    let n2 = partLen(c2, w2)
    if n1 == 0 and n2 == 0: return 0
    if n1 == 0: return (if (d2 and 2) == 0: -1 else: 1)
    if n2 == 0: return (if (d1 and 2) == 0: 1 else: -1)
    let t = d1 xor d2
    if t == 2: return (if (d1 and 2) == 0: 1 else: -1)
    if t != 0: specError("compare across axes")
    var l1 = w1
    var l2 = w2
    let c = rCmp(opnd(c1, w1, l1), n1, opnd(c2, w2, l2), n2)
    if (d1 and 2) == 0: c else: -c
else:
  proc opCmp*(h1: uint8; w1: Word; h2: uint8; w2: Word): int {.noinline.} =
    ## the branch-free row: each operand's sign as a value, sg = nz * (1 - 2 neg) in {-1, 0, 1}; the result is
    ## (sg1 > sg2) - (sg1 < sg2), plus, masked by sg1 = sg2, the magnitude order negated by a mask when negative.
    ## The axis check is kept as a branch (never taken by integers); the magnitude compare's loop is kept.
    let c1 = cl(h1)
    let c2 = cl(h2)
    let d1 = int(h1 shr 6)
    let d2 = int(h2 shr 6)
    let n1 = partLen(c1, w1)
    let n2 = partLen(c2, w2)
    let nz1 = int(n1 != 0)
    let nz2 = int(n2 != 0)
    if (((d1 xor d2) and 1) and nz1 and nz2) != 0: specError("compare across axes")
    let ng1 = (d1 shr 1) and 1
    let ng2 = (d2 shr 1) and 1
    let sg1 = nz1 - 2 * (ng1 and nz1)
    let sg2 = nz2 - 2 * (ng2 and nz2)
    var l1 = w1
    var l2 = w2
    let c = rCmp(opnd(c1, w1, l1), n1, opnd(c2, w2, l2), n2)
    let nm = 0 - ng1                       # all ones when negative
    let cs = (c xor nm) - nm
    let dd = sg1 - sg2
    let same = not ashr(dd or (0 - dd), 63)  # all ones when sg1 = sg2 (a shift, not a compare: gcc would select)
    (int(sg1 > sg2) - int(sg1 < sg2)) + (cs and same)

# ------------------------------------------------------------------------------------------------ readout (outside the clock)
proc words*(h: uint8; w: Word): seq[Word] = partWords(cl(h), w)
proc negative*(h: uint8; w: Word): bool = (h and 0x80) != 0 and partLen(cl(h), w) != 0
proc dumpLine*(h: uint8; w: Word): string =
  ## header, word, and the whole slot when there is one (lowest first)
  result = toHex(h).toLowerAscii & " " & toHex(w).toLowerAscii
  if cl(h) != 0:
    var s: seq[string]
    for x in slotWords(cl(h), w): s.add toHex(x).toLowerAscii
    result.add " " & s.join(",")
