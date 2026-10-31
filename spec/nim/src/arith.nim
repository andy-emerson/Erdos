## arith.nim: the arithmetic both models run, call for call (spec v0.1 §6; its cases and their order are v0.3 §17.3's,
## its pseudocode 10-interning v0.2 §10.1-10.2, gcd-first). One source for Control and Packed, so their arithmetic is
## the same by construction.
##
## Notation as the pseudocode: x = s_x a/b and y = s_y c/d, magnitudes in lowest terms. Every multiplication and
## addition is checked as 08 checks it (a result outside [-2^63 + 1, 2^63 - 1] overflows), step by step in the order
## tc3.c makes them (which is the pseudocode's written order, numerator before denominator, except that case num
## with h != 1 makes both divisions before its multiplications). An overflow makes the operation return false: the
## caller marks the cell (§17.10). The counts of the steps made before it stay counted ("up to the stop").

{.push raises: [].}

import core

type
  Operand* = object
    ## a rational as the arithmetic reads it: s a/b, a >= 0, b >= 1, in lowest terms
    a*, b*: int64
    neg*: bool
  Ratio* = object
    ## a result: n/e in lowest terms, e >= 1, zero as 0/1; or, in a cell, the mark 0/0
    n*, e*: int64

const mark* = Ratio(n: 0, e: 0)   ## §17.10 item 1: the whole numbers 0 and 0, the sign tag +

template zeroIsZeroOverOne(r: Ratio) =
  ## test build (K-14, 10-interning v0.2 §10.5): a zero result already has the denominator 1, never set by the store
  when auditing:
    if r.n == 0 and r.e != 1: skDie("K-14: a zero result without the denominator 1")

func operand*(r: Ratio): Operand {.inline.} =
  Operand(a: int64(mag(r.n)), b: r.e, neg: r.n < 0)

# The checked steps. Each computes its operands first (left to right), counts itself, then returns false from the
# operation if it overflows.
template times(p, q: int64): int64 =
  let x = p
  let y = q
  var product {.noinit.}: int64
  tally(cnt, muls, 1)
  if overflowsMul(x, y, product): return false
  product

template plus(p, q: int64): int64 =
  let x = p
  let y = q
  var total {.noinit.}: int64
  tally(cnt, adds, 1)
  if overflowsAdd(x, y, total): return false
  total

template compared(cnt: var Tally, n: int, bd, ac, ad, cb: bool) =
  ## the comparisons, all made before a case is chosen; the first four are the cross tests
  when counting:
    cnt.cmps += n
    cnt.matches += ord(bd) + ord(ac) + ord(ad) + ord(cb)

proc add*(x, y: Operand, r: var Ratio, cnt: var Tally): bool =
  ## r = x + y. Cases, in order: zero, zero_x, zero_y, int_both, int_x, int_y, cross, den, num, general.
  let (a, b, sx) = (x.a, x.b, x.neg)
  let (c, d, sy) = (y.a, y.b, y.neg)
  cnt.compared(8, b == d, a == c, a == d, c == b)       # and b = 1, d = 1, a = 0, c = 0
  if b == d and a == c and sx != sy:                    # zero: x + (-x)                          [K-2]
    cnt.taken addZero
    r = Ratio(n: 0, e: 1)
  elif a == 0:                                          # zero_x: the sum is y
    cnt.taken addZeroX
    r = Ratio(n: sgn(sy, c), e: d)
  elif c == 0:                                          # zero_y: the sum is x
    cnt.taken addZeroY
    r = Ratio(n: sgn(sx, a), e: b)
  elif b == 1 and d == 1:                               # int_both
    cnt.taken addIntBoth
    r = Ratio(n: plus(sgn(sx, a), sgn(sy, c)), e: 1)
  elif b == 1:                                          # int_x: gcd(s_x a d + s_y c, d) = gcd(c, d) = 1
    cnt.taken addIntX
    r = Ratio(n: plus(times(sgn(sx, a), d), sgn(sy, c)), e: d)
  elif d == 1:                                          # int_y
    cnt.taken addIntY
    r = Ratio(n: plus(sgn(sx, a), times(sgn(sy, c), b)), e: b)
  elif a == d or c == b:                                # cross: gcd(b, d) = 1
    cnt.taken addCross
    let t = plus(times(sgn(sx, a), d), times(sgn(sy, c), b))
    r = Ratio(n: t, e: times(b, d))
  elif b == d:                                          # den: gcd(b, d) = d
    cnt.taken addDen
    let t = plus(sgn(sx, a), sgn(sy, c))
    let h = cnt.gcdOf(t, d)
    if h == 1:                                          # [K-3]
      r = Ratio(n: t, e: d)
    else:
      r = Ratio(n: cnt.divExact(t, h), e: cnt.divExact(d, h))
  elif a == c:                                          # num: gcd(a, b d) = 1
    cnt.taken addNum
    let g = cnt.gcdOf(b, d)
    if g == 1:                                          # [K-4]
      let t = plus(sgn(sx, d), sgn(sy, b))
      r = Ratio(n: times(a, t), e: times(b, d))
    else:
      let b1 = cnt.divExact(b, g)
      let d1 = cnt.divExact(d, g)
      let t = plus(sgn(sx, d1), sgn(sy, b1))
      let h = cnt.gcdOf(t, g)
      if h == 1:                                        # [K-5]
        r = Ratio(n: times(a, t), e: times(b1, d))
      else:                                             # both divisions first, as tc3.c
        let t1 = cnt.divExact(t, h)
        let d2 = cnt.divExact(d, h)
        let n = times(a, t1)
        r = Ratio(n: n, e: times(b1, d2))
  else:                                                 # general
    cnt.taken addGeneral
    let g = cnt.gcdOf(b, d)
    if g == 1:                                          # [K-6]
      let t = plus(times(sgn(sx, a), d), times(sgn(sy, c), b))
      r = Ratio(n: t, e: times(b, d))
    else:
      let b1 = cnt.divExact(b, g)
      let d1 = cnt.divExact(d, g)
      let t = plus(times(sgn(sx, a), d1), times(sgn(sy, c), b1))
      let h = cnt.gcdOf(t, g)
      if h == 1:                                        # [K-7]
        r = Ratio(n: t, e: times(b1, d))
      else:
        let n = cnt.divExact(t, h)
        r = Ratio(n: n, e: times(b1, cnt.divExact(d, h)))
  zeroIsZeroOverOne(r)
  true

proc multiply*(x, y: Operand, r: var Ratio, cnt: var Tally): bool =
  ## r = x y, on magnitudes with the sign s = s_x s_y. Cases, in order: zero, one_x, one_y, int_both, both, den, num,
  ## cross_ad, cross_cb, int_x, int_y, general.
  let (a, b) = (x.a, x.b)
  let (c, d) = (y.a, y.b)
  let s = x.neg != y.neg                                # s = -1
  cnt.compared(10, b == d, a == c, a == d, c == b)      # and a = b, c = d, b = 1, d = 1, a = 0, c = 0
  if a == 0 or c == 0:                                  # zero
    cnt.taken mulZero
    r = Ratio(n: 0, e: 1)
  elif a == b:                                          # one_x: x = ±1                          [K-15]
    cnt.taken mulOneX
    r = Ratio(n: sgn(s, c), e: d)
  elif c == d:                                          # one_y: y = ±1                          [K-15]
    cnt.taken mulOneY
    r = Ratio(n: sgn(s, a), e: b)
  elif b == 1 and d == 1:                               # int_both
    cnt.taken mulIntBoth
    r = Ratio(n: sgn(s, times(a, c)), e: 1)
  elif a == d and c == b:                               # both: reciprocals                       [K-12]
    cnt.taken mulBoth
    r = Ratio(n: sgn(s, 1), e: 1)
  elif b == d:                                          # den: no gcd
    cnt.taken mulDen
    let n = sgn(s, times(a, c))
    r = Ratio(n: n, e: times(d, d))
  elif a == c:                                          # num (c = ±a): no gcd
    cnt.taken mulNum
    let n = sgn(s, times(a, a))
    r = Ratio(n: n, e: times(b, d))
  elif a == d:                                          # cross_ad: a cancels
    cnt.taken mulCrossAd
    let h = cnt.gcdOf(c, b)
    if h == 1:                                          # [K-8]
      r = Ratio(n: sgn(s, c), e: b)
    else:
      r = Ratio(n: sgn(s, cnt.divExact(c, h)), e: cnt.divExact(b, h))
  elif c == b:                                          # cross_cb: b cancels
    cnt.taken mulCrossCb
    let h = cnt.gcdOf(a, d)
    if h == 1:                                          # [K-8]
      r = Ratio(n: sgn(s, a), e: d)
    else:
      r = Ratio(n: sgn(s, cnt.divExact(a, h)), e: cnt.divExact(d, h))
  elif b == 1:                                          # int_x: gcd(c, b) = 1, one cross gcd
    cnt.taken mulIntX
    let h = cnt.gcdOf(a, d)
    var (a1, d1) = (a, d)
    if h != 1:                                          # else [K-8]
      a1 = cnt.divExact(a, h)
      d1 = cnt.divExact(d, h)
    r = Ratio(n: sgn(s, times(a1, c)), e: d1)
  elif d == 1:                                          # int_y
    cnt.taken mulIntY
    let h = cnt.gcdOf(c, b)
    var (c1, b1) = (c, b)
    if h != 1:                                          # else [K-8]
      c1 = cnt.divExact(c, h)
      b1 = cnt.divExact(b, h)
    r = Ratio(n: sgn(s, times(a, c1)), e: b1)
  else:                                                 # general
    cnt.taken mulGeneral
    let g1 = cnt.gcdOf(a, d)
    let g2 = cnt.gcdOf(c, b)
    var (a1, d1, c1, b1) = (a, d, c, b)
    if g1 != 1:                                         # else [K-9]
      a1 = cnt.divExact(a, g1)
      d1 = cnt.divExact(d, g1)
    if g2 != 1:                                         # else [K-9]
      c1 = cnt.divExact(c, g2)
      b1 = cnt.divExact(b, g2)
    let n = sgn(s, times(a1, c1))
    r = Ratio(n: n, e: times(b1, d1))
  zeroIsZeroOverOne(r)
  true

proc reduce*(m, d: int64, r: var Ratio, cnt: var Tally): SkStatus =
  ## entry (v0.2 §10.4): m/d in lowest terms, gcd-first, d made positive
  if m == int64Min or d == int64Min: return skEInval  # validity first, then d = 0, as tc3.c
  if d == 0: return skEDivZero
  let (m, d) = if d < 0: (-m, -d) else: (m, d)
  let g = cnt.gcdOf(m, d)
  if g == 1:                                            # [K-1]
    r = Ratio(n: m, e: d)
  else:
    r = Ratio(n: cnt.divExact(m, g), e: cnt.divExact(d, g))
  zeroIsZeroOverOne(r)
  skOk

{.pop.}
