# control.nim: Control (spec 13 §5): spec 12's Growing (§1.4), given A's header list. A number is A's 8-bit header,
# whose class bits are unused (0) and whose direction bits are A's, and a word that points to Growing's block: one
# malloc'd block per value, a header word (the length) then the words, no leading zero words, immutable.
# The magnitudes use the shared routines (§1), each writing into a block malloc'd at its bound, as Growing's do; the
# directions follow A's rules (§2), so that Control differs from A only in where the magnitude lives.
import std/strutils
import routines, alloc

const D = 0xC0'u8

type
  AV* = object
    h*: uint8
    w*: Word
  Big = ptr UncheckedArray[Word]       # [0] the length n, [1 .. n] the words

template big(w: Word): Big = cast[Big](w)
template blen(w: Word): int = int(big(w)[0])
template bwd(w: Word): Words = cast[Words](addr big(w)[1])

proc newBig(n: int): Big {.inline.} =
  result = cast[Big](xalloc((n + 1) * sizeof(Word)))
  result[0] = Word(n)

template val(b: Big; n: int; dir: uint8): AV =
  b[0] = Word(n)
  AV(h: dir and D, w: cast[Word](b))

proc release*(h: uint8; w: Word) {.inline.} = xfree(cast[pointer](w))

proc make*(d: int; m: openArray[Word]): AV =
  let b = newBig(m.len)
  for i in 0 ..< m.len: b[i + 1] = m[i]
  AV(h: uint8(d shl 6), w: cast[Word](b))

# ------------------------------------------------------------------------------------------------ add, subtract
proc addC(h1in: uint8; w1: Word; h2: uint8; w2: Word): AV {.inline.} =
  ## A's tree (designa.nim) over Growing's blocks
  var h1 = h1in
  let n1 = blen(w1)
  let n2 = blen(w2)
  var t = (h1 xor h2) and D
  if (t and 0x40) != 0:                     # different axes
    if n1 == 0: h1 = (h1 and not D) or (h2 and D)
    elif n2 == 0: discard
    else: specError("sum across axes (an expression)")
    t = 0
  if t == 0:
    let r = newBig(imax(n1, n2) + 1)
    let n = rAdd(cast[Words](addr r[1]), bwd(w1), n1, bwd(w2), n2)
    return val(r, n, h1 and D)
  let r = newBig(imax(n1, n2))
  var br: Word
  let n = rAbsdiff(cast[Words](addr r[1]), bwd(w1), n1, bwd(w2), n2, br)
  val(r, n, (h1 and D) + uint8(br shl 7))

proc opAdd*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} = addC(h1, w1, h2, w2)
proc opSub*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} = addC(h1, w1, h2 xor 0x80, w2)

proc opNeg*(h: uint8; w: Word): AV {.noinline.} =
  ## h xor 0x80; the block copied (values are immutable and released one by one)
  let n = blen(w)
  let r = newBig(n)
  let s = bwd(w)
  for i in 0 ..< n: r[i + 1] = s[i]
  AV(h: h xor 0x80, w: cast[Word](r))

proc opMul*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  let n1 = blen(w1)
  let n2 = blen(w2)
  let r = newBig(n1 + n2)
  let n = rMac(cast[Words](addr r[1]), 0, bwd(w1), n1, bwd(w2), n2)
  val(r, n, (h1 and D) + (h2 and D))

proc opDiv*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  let n1 = blen(w1)
  let n2 = blen(w2)
  let r = newBig(imax(n1 - n2 + 1, 0))
  var n: int
  withScratch(n1 + n2, buf):
    let x = bwd(w1)
    let y = bwd(w2)
    for i in 0 ..< n1: buf[i] = x[i]
    for i in 0 ..< n2: buf[n1 + i] = y[i]
    n = rDivCore(cast[Words](addr r[1]), buf, 0, n1, n1, n2, n1, n2)
  val(r, n, (h1 and D) - (h2 and D))

proc opGcd*(h1: uint8; w1: Word; h2: uint8; w2: Word): AV {.noinline.} =
  let n1 = blen(w1)
  let n2 = blen(w2)
  if n1 == 0 and n2 == 0: specError("gcd(0, 0)")
  let r = newBig(if n1 != 0 and n2 != 0: imin(n1, n2) else: imax(n1, n2))
  var n: int
  withScratch(n1 + n2, buf):
    let x = bwd(w1)
    let y = bwd(w2)
    for i in 0 ..< n1: buf[i] = x[i]
    for i in 0 ..< n2: buf[n1 + i] = y[i]
    n = rGcdCore(cast[Words](addr r[1]), buf, 0, n1, n1, n2)
  val(r, n, 0)

proc opCmp*(h1: uint8; w1: Word; h2: uint8; w2: Word): int {.noinline.} =
  ## A's compare (designa.nim)
  let d1 = h1 shr 6
  let d2 = h2 shr 6
  let n1 = blen(w1)
  let n2 = blen(w2)
  if n1 == 0 and n2 == 0: return 0
  if n1 == 0: return (if (d2 and 2) == 0: -1 else: 1)
  if n2 == 0: return (if (d1 and 2) == 0: 1 else: -1)
  let t = d1 xor d2
  if t == 2: return (if (d1 and 2) == 0: 1 else: -1)
  if t != 0: specError("compare across axes")
  let c = rCmp(bwd(w1), n1, bwd(w2), n2)
  if (d1 and 2) == 0: c else: -c

# ------------------------------------------------------------------------------------------------ readout (outside the clock)
proc words*(h: uint8; w: Word): seq[Word] =
  for i in 0 ..< blen(w): result.add bwd(w)[i]
proc negative*(h: uint8; w: Word): bool = (h and 0x80) != 0 and blen(w) != 0
proc dumpLine*(h: uint8; w: Word): string =
  ## header, the block's length, its words (lowest first)
  var s: seq[string]
  for x in words(h, w): s.add toHex(x).toLowerAscii
  toHex(h).toLowerAscii & " " & $blen(w) & " " & (if s.len == 0: "-" else: s.join(","))
when defined(mem):
  proc blockBytes*(w: Word): (int, int) = (acctOf(cast[pointer](w)), heapOf(cast[pointer](w)))
