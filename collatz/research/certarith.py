"""Certified arithmetic for the cycle-length bounds (cycle-bounds.md).

Every real constant is returned as an integer interval [lo, hi] at scale 2^P,
meaning lo/2^P <= value <= hi/2^P.  Series are summed with separate lower and
upper roundings and an explicit tail bound, so the intervals are proven
enclosures (no floating point anywhere).  Ratios are returned as Fractions.
"""
from fractions import Fraction
from math import isqrt

P = 320  # bits


def _div_floor(a, b):
    return a // b


def _div_ceil(a, b):
    return -((-a) // b)


def atanh_pos(p, q, prec=P):
    """Enclosure of atanh(p/q) for 0 < p < q, at scale 2^prec."""
    assert 0 < p < q
    one = 1 << prec
    p2, q2 = p * p, q * q
    lo_pow = _div_floor(one * p, q)   # t^(2k+1), rounded down
    hi_pow = _div_ceil(one * p, q)    # rounded up
    lo = hi = 0
    k = 0
    while True:
        lo += _div_floor(lo_pow, 2 * k + 1)
        hi += _div_ceil(hi_pow, 2 * k + 1)
        k += 1
        lo_pow = _div_floor(lo_pow * p2, q2)
        hi_pow = _div_ceil(hi_pow * p2, q2)
        if hi_pow == 0 or hi_pow < 4:
            break
    # tail: sum_{j>=k} t^(2j+1)/(2j+1) <= t^(2k+1)/((2k+1)(1-t^2))
    hi += _div_ceil(hi_pow * q2, (2 * k + 1) * (q2 - p2)) + 1
    return lo, hi


def ln_rat(a, b, prec=P):
    """Enclosure of ln(a/b) for positive integers a, b: ln(a/b) = 2 atanh((a-b)/(a+b))."""
    num, den = a - b, a + b
    if num == 0:
        return 0, 0
    lo, hi = atanh_pos(abs(num), den, prec)
    if num > 0:
        return 2 * lo, 2 * hi
    return -2 * hi, -2 * lo


def atan_small(p, q, prec=P):
    """Enclosure of atan(p/q), 0 < p <= q (alternating series with tail bound)."""
    assert 0 < p <= q
    one = 1 << prec
    p2, q2 = p * p, q * q
    lo_pow = _div_floor(one * p, q)
    hi_pow = _div_ceil(one * p, q)
    lo = hi = 0
    k = 0
    while True:
        tl = _div_floor(lo_pow, 2 * k + 1)
        th = _div_ceil(hi_pow, 2 * k + 1)
        if k % 2 == 0:
            lo += tl
            hi += th
        else:
            lo -= th
            hi -= tl
        k += 1
        lo_pow = _div_floor(lo_pow * p2, q2)
        hi_pow = _div_ceil(hi_pow * p2, q2)
        if hi_pow < 4:
            break
    tail = _div_ceil(hi_pow, 2 * k + 1) + 1
    return lo - tail, hi + tail


def pi_iv(prec=P):
    """Machin: pi = 16 atan(1/5) - 4 atan(1/239)."""
    a = atan_small(1, 5, prec)
    b = atan_small(1, 239, prec)
    return 16 * a[0] - 4 * b[1], 16 * a[1] - 4 * b[0]


def arg_iv(a, b, prec=P):
    """Enclosure of arg(a+bi) in (-pi, pi], a+bi != 0."""
    pl, ph = pi_iv(prec)
    if b == 0:
        return (0, 0) if a > 0 else (pl, ph)
    if a == 0:
        return (pl // 2, ph // 2 + 1) if b > 0 else (-(ph // 2 + 1), -(pl // 2))
    x, y = abs(a), abs(b)
    if y <= x:
        t = atan_small(y, x, prec)
    else:  # atan(y/x) = pi/2 - atan(x/y)
        s = atan_small(x, y, prec)
        t = (pl // 2 - s[1], ph // 2 + 1 - s[0])
    # first-quadrant angle t; place by quadrant
    if a > 0 and b > 0:
        return t
    if a > 0 and b < 0:
        return -t[1], -t[0]
    if a < 0 and b > 0:
        return pl - t[1], ph - t[0]
    return t[0] - ph, t[1] - pl


def iv_frac(iv, prec=P):
    return Fraction(iv[0], 1 << prec), Fraction(iv[1], 1 << prec)


def sqrt_frac_iv(num, den, prec=P):
    """Enclosure of sqrt(num/den) at scale 2^prec."""
    x = (num << (2 * prec)) // den
    s = isqrt(x)
    return s, s + 1


def floor_frac(x):
    return x.numerator // x.denominator


def ceil_frac(x):
    return -((-x.numerator) // x.denominator)


def simplest_in(x, y):
    """Fraction with the smallest positive denominator in the closed interval [x, y]
    (Fractions, x <= y).  Standard continued-fraction recursion; returns the
    smallest denominator and, among those, the fraction found."""
    assert x <= y
    fl = floor_frac(x)
    if fl == x:
        return Fraction(fl)
    if fl + 1 <= y:
        return Fraction(fl + 1)
    r = simplest_in(1 / (y - fl), 1 / (x - fl))
    return fl + 1 / r


def min_denominator(x, y):
    return simplest_in(x, y).denominator


def contfrac(x, nterms):
    out = []
    for _ in range(nterms):
        a = floor_frac(x)
        out.append(a)
        if x == a:
            break
        x = 1 / (x - a)
    return out


def certified_cf(lo, hi, maxterms=60):
    """Partial quotients common to every real in [lo, hi] (exact Fractions)."""
    out = []
    a, b = lo, hi
    while len(out) < maxterms:
        fa, fb = floor_frac(a), floor_frac(b)
        if fa != fb or a == fa or b == fb:
            break
        out.append(fa)
        a, b = 1 / (b - fa), 1 / (a - fa)
    return out


def convergents(cf):
    p0, q0, p1, q1 = 1, 0, cf[0], 1
    res = [(p1, q1)]
    for a in cf[1:]:
        p0, q0, p1, q1 = p1, q1, a * p1 + p0, a * q1 + q0
        res.append((p1, q1))
    return res
