# designb.nim: Design B (spec 13 §3), one magnitude per direction. A number is a pair (P, N) along +1 and -1, value
# P - N: a 16-bit header (P's class in the low byte, N's in the high byte, 3 spare bits 0 in each) and two words, P's
# then N's (two parallel lists beside the header list). Collapse is the only subtraction; division and gcd collapse
# into their working buffer, then follow A's direction rules. B is canonical only after collapse.
import std/strutils
import routines, pools

type BV* = object
  h*: uint16
  p*, n*: Word

template cP(h: uint16): int = int(h and 31)
template cN(h: uint16): int = int((h shr 8) and 31)

proc release*(h: uint16; p, n: Word) {.inline.} =
  let a = cP(h)
  let b = cN(h)
  if a != 0: slotRelease(a, p)
  if b != 0: slotRelease(b, n)

template join(fp, fn: (int, Word)): BV =
  BV(h: uint16(fp[0]) or (uint16(fn[0]) shl 8), p: fp[1], n: fn[1])

proc make*(pw, nw: openArray[Word]): BV =
  ## outside the operations: the parts P and N as words, lowest first (P allocated first)
  let fp = partMake(pw)
  let fn = partMake(nw)
  join(fp, fn)

# ------------------------------------------------------------------------------------------------ add, subtract
proc addParts(ca: int; wa: Word; cb: int; wb: Word; cc: int; wc: Word; cd: int; wd: Word): BV {.inline.} =
  ## (a + b, c + d): both result slots allocated first, then the pointers
  let la = partLen(ca, wa)
  let lb = partLen(cb, wb)
  let lc = partLen(cc, wc)
  let ld = partLen(cd, wd)
  var rp = resNew(imax(la, lb) + 1)
  var rn = resNew(imax(lc, ld) + 1)
  var xa = wa
  var xb = wb
  var xc = wc
  var xd = wd
  let lp = rAdd(resDst(rp), opnd(ca, wa, xa), la, opnd(cb, wb, xb), lb)
  let ln = rAdd(resDst(rn), opnd(cc, wc, xc), lc, opnd(cd, wd, xd), ld)
  let fp = finish(rp, lp)
  let fn = finish(rn, ln)
  join(fp, fn)

proc opAdd*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): BV {.noinline.} =
  ## (P1 + P2, N1 + N2)
  addParts(cP(h1), p1, cP(h2), p2, cN(h1), n1, cN(h2), n2)

proc opSub*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): BV {.noinline.} =
  ## the crossed parts, (P1 + N2, N1 + P2), copying nothing
  addParts(cP(h1), p1, cN(h2), n2, cN(h1), n1, cP(h2), p2)

# ------------------------------------------------------------------------------------------------ negate
proc opNeg*(h: uint16; p, n: Word): BV {.noinline.} =
  ## swap P and N, each part copied (no two numbers share a slot); N's copy first
  let fp = partCopy(cN(h), n)
  let fn = partCopy(cP(h), p)
  join(fp, fn)

# ------------------------------------------------------------------------------------------------ multiply
proc opMul*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): BV {.noinline.} =
  ## P = P1 P2 + N1 N2, N = P1 N2 + N1 P2, by multiply-accumulate; each part's bound imax(na + nb, nc + nd) + 1
  let a = cP(h1)
  let b = cN(h1)
  let c = cP(h2)
  let d = cN(h2)
  let lp1 = partLen(a, p1)
  let ln1 = partLen(b, n1)
  let lp2 = partLen(c, p2)
  let ln2 = partLen(d, n2)
  var rp = resNew(imax(lp1 + lp2, ln1 + ln2) + 1)
  var rn = resNew(imax(lp1 + ln2, ln1 + lp2) + 1)
  var xp1 = p1
  var xn1 = n1
  var xp2 = p2
  var xn2 = n2
  let P1 = opnd(a, p1, xp1)
  let N1 = opnd(b, n1, xn1)
  let P2 = opnd(c, p2, xp2)
  let N2 = opnd(d, n2, xn2)
  let dp = resDst(rp)
  var lp = rMac(dp, 0, P1, lp1, P2, lp2)
  lp = rMac(dp, lp, N1, ln1, N2, ln2)
  let dn = resDst(rn)
  var ln = rMac(dn, 0, P1, lp1, N2, ln2)
  ln = rMac(dn, ln, N1, ln1, P2, lp2)
  let fp = finish(rp, lp)
  let fn = finish(rn, ln)
  join(fp, fn)

# ------------------------------------------------------------------------------------------------ compare
proc opCmp*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): int {.noinline.} =
  ## P1 + N2 against P2 + N1, the two sums in one local (two halves)
  let a = cP(h1)
  let b = cN(h1)
  let c = cP(h2)
  let d = cN(h2)
  let lp1 = partLen(a, p1)
  let ln1 = partLen(b, n1)
  let lp2 = partLen(c, p2)
  let ln2 = partLen(d, n2)
  let s1 = imax(lp1, ln2) + 1
  let s2 = imax(lp2, ln1) + 1
  var xp1 = p1
  var xn1 = n1
  var xp2 = p2
  var xn2 = n2
  withScratch(s1 + s2, loc):
    let l1 = rAdd(loc, opnd(a, p1, xp1), lp1, opnd(d, n2, xn2), ln2)
    let l2 = rAdd(at(loc, s1), opnd(c, p2, xp2), lp2, opnd(b, n1, xn1), ln1)
    result = rCmp(loc, l1, at(loc, s1), l2)

# ------------------------------------------------------------------------------------------------ division, gcd
template loadCollapsed(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word;
                       s1, m1, b1, m2, b2: untyped) {.dirty.} =
  ## the working buffer: each operand's |P - N| written straight in, with its borrow (collapse)
  let a = cP(h1)
  let b = cN(h1)
  let c = cP(h2)
  let d = cN(h2)
  var xp1 = p1
  var xn1 = n1
  var xp2 = p2
  var xn2 = n2
  let lp1 = partLen(a, p1)
  let ln1 = partLen(b, n1)
  let lp2 = partLen(c, p2)
  let ln2 = partLen(d, n2)
  let s1 = imax(lp1, ln1)
  let s2 = imax(lp2, ln2)
  var b1, b2: Word

template collapseInto(buf: Words; s1, m1, b1, m2, b2: untyped) {.dirty.} =
  let m1 = rAbsdiff(buf, opnd(a, p1, xp1), lp1, opnd(b, n1, xn1), ln1, b1)
  let m2 = rAbsdiff(at(buf, s1), opnd(c, p2, xp2), lp2, opnd(d, n2, xn2), ln2, b2)

proc opDiv*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): BV {.noinline.} =
  ## collapse both into the buffer, divide, then A's rule: direction (b1 shl 7) - (b2 shl 7), 0 to P, 2 to N
  loadCollapsed(h1, p1, n1, h2, p2, n2, s1, m1, b1, m2, b2)
  var q: (int, Word)
  withScratch(s1 + s2, buf):
    collapseInto(buf, s1, m1, b1, m2, b2)
    var r = resNew(imax(m1 - m2 + 1, 0))
    let n = rDivCore(resDst(r), buf, 0, m1, s1, m2, m1, m2)
    q = finish(r, n)
  let dd = (uint8(b1 shl 7) - uint8(b2 shl 7)) and 0xC0
  if dd == 0: join(q, (0, 0'u64)) else: join((0, 0'u64), q)

proc opGcd*(h1: uint16; p1, n1: Word; h2: uint16; p2, n2: Word): BV {.noinline.} =
  ## collapse both into the buffer, gcd, to P; gcd(0, 0) an error
  loadCollapsed(h1, p1, n1, h2, p2, n2, s1, m1, b1, m2, b2)
  var g: (int, Word)
  withScratch(s1 + s2, buf):
    collapseInto(buf, s1, m1, b1, m2, b2)
    if m1 == 0 and m2 == 0: specError("gcd(0, 0)")
    var r = resNew(if m1 != 0 and m2 != 0: imin(m1, m2) else: imax(m1, m2))
    let n = rGcdCore(resDst(r), buf, 0, m1, s1, m2)
    g = finish(r, n)
  join(g, (0, 0'u64))

# ------------------------------------------------------------------------------------------------ readout (outside the clock)
proc partsOf*(h: uint16; p, n: Word): (seq[Word], seq[Word]) = (partWords(cP(h), p), partWords(cN(h), n))

proc dumpLine*(h: uint16; p, n: Word): string =
  ## header, the two words, and each part's whole slot ("-" for class 0), lowest first
  result = toHex(h).toLowerAscii & " " & toHex(p).toLowerAscii & " " & toHex(n).toLowerAscii
  for (c, w) in [(cP(h), p), (cN(h), n)]:
    if c == 0:
      result.add " -"
    else:
      var s: seq[string]
      for x in slotWords(c, w): s.add toHex(x).toLowerAscii
      result.add " " & s.join(",")
