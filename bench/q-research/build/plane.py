"""The plane variant: one value (a + b i)/c holding a whole Gaussian rational, three integers sharing one word.

Format. A header holds two sign bits (for a and b) and two cut positions; the word holds |a| in its top bits,
|b| next, and c - 1 in the remaining bits, so that, as in Q72, a zero field means c = 1 and every pattern is a value.
For a w-bit word each cut takes bits(w) bits (a field may be empty: a = 0 or b = 0), so the header is
2 + 2*bits(w) bits: 16 for a 64-bit word (P80), 18 for a 128-bit word, rounded to 24 (P152).
Canonical form: gcd(a, b, c) = 1, and each of |a| and |b| takes exactly bits(|a|) and bits(|b|) bits.
A value fits when bits(|a|) + bits(|b|) + bits(c - 1) <= w.

Vectors. A plane vector is two int64 (or wider) integer vectors over one shared denominator: element i is
(A[i] + B[i] i) / D. It is built on the signed vector form (svec.SVec) for each part, kept on one denominator."""
import math
from fractions import Fraction

import numpy as np

import reuse  # noqa: F401
from svec import SVec
import skern as K


# ---------------------------------------------------------------- the scalar format

def plane_fits(a, b, c, w):
    g = math.gcd(math.gcd(abs(a), abs(b)), c)
    a, b, c = a // g, b // g, c // g
    return abs(a).bit_length() + abs(b).bit_length() + (c - 1).bit_length() <= w


def plane_encode(a, b, c, w=64):
    """(header, word) of (a + b i)/c in canonical form, or None if it does not fit."""
    g = math.gcd(math.gcd(abs(a), abs(b)), c)
    a, b, c = a // g, b // g, c // g
    la, lb, lc = abs(a).bit_length(), abs(b).bit_length(), (c - 1).bit_length()
    if la + lb + lc > w:
        return None
    k = w.bit_length()                      # bits per cut
    cut1, cut2 = la, la + lb
    header = ((a < 0) << (2 * k + 1)) | ((b < 0) << (2 * k)) | (cut1 << k) | cut2
    word = c - 1
    if la:
        word |= abs(a) << (w - la)
    if lb:
        word |= abs(b) << (w - cut2)
    return header, word


def plane_decode(header, word, w=64):
    k = w.bit_length()
    sa, sb = (header >> (2 * k + 1)) & 1, (header >> (2 * k)) & 1
    cut1, cut2 = (header >> k) & ((1 << k) - 1), header & ((1 << k) - 1)
    cut1, cut2 = min(cut1, w), min(max(cut2, cut1), w)       # every header decodes
    a = word >> (w - cut1) if cut1 else 0
    b = (word >> (w - cut2)) & ((1 << (cut2 - cut1)) - 1) if cut2 > cut1 else 0
    c = (word & ((1 << (w - cut2)) - 1)) + 1 if cut2 < w else 1
    return (-a if sa else a), (-b if sb else b), c


def q_fits(x, word):
    """A real or imaginary part as one Q value with the given word (Q40: 32, Q72: 64)."""
    x = Fraction(x)
    if x == 0:
        return True
    return abs(x.numerator).bit_length() + (x.denominator - 1).bit_length() <= word


def pair_fits(z, word):
    re, im = z
    return q_fits(re, word) and q_fits(im, word)


def as_plane(z):
    re, im = Fraction(z[0]), Fraction(z[1])
    c = math.lcm(re.denominator, im.denominator)
    return int(re * c), int(im * c), c


# ---------------------------------------------------------------- plane vectors

class PVec:
    """Elements (A[i] + B[i] i) / D: a real-part and an imaginary-part signed vector on one denominator."""

    __slots__ = ("re", "im")

    def __init__(self, re, im):
        if re.den != im.den:
            D = math.lcm(re.den, im.den)
            re, im = re.scale_int(D // re.den), im.scale_int(D // im.den)
            re, im = SVec(re.tier, re.val, D, 0), SVec(im.tier, im.val, D, 0)
        self.re, self.im = re, im

    @staticmethod
    def decimals(re_nums, im_nums, den):
        return PVec(SVec("s64", np.array(re_nums, dtype=np.int64), den),
                    SVec("s64", np.array(im_nums, dtype=np.int64), den)).reduce()

    @property
    def den(self):
        return self.re.den

    def __len__(self):
        return len(self.re)

    def reduce(self):
        """Divide out the common factor of D and every element of both parts."""
        D = self.den
        if D == 1:
            return self
        g = D
        for part in (self.re, self.im):
            if part.tier == "s64" and g <= 2**63 - 1:
                g = int(K.s_gcd(part.val, np.uint64(g)))
            else:                                  # wide tiers: stop as soon as the running gcd is 1
                for i in range(len(part)):
                    g = math.gcd(g, part.value(i))
                    if g == 1:
                        break
            if g == 1:
                return self
        return PVec(_div(self.re, g, D // g), _div(self.im, g, D // g))

    def _parts_aligned(self, o):
        D = math.lcm(self.den, o.den)
        s, t = D // self.den, D // o.den
        return (_scale(self.re, s), _scale(self.im, s), _scale(o.re, t), _scale(o.im, t), D)

    def __add__(self, o):
        a, b, c, d, D = self._parts_aligned(o)
        return PVec(_raw(a + c, D), _raw(b + d, D)).reduce()

    def __sub__(self, o):
        a, b, c, d, D = self._parts_aligned(o)
        return PVec(_raw(a - c, D), _raw(b - d, D)).reduce()

    def __mul__(self, o):
        """(A + Bi)(C + Di) = (AC - BD) + (AD + BC) i over the product of the denominators: four integer
        products and two sums, all elementwise on integer vectors."""
        A, B, C, D_ = (_unit(x) for x in (self.re, self.im, o.re, o.im))
        den = self.den * o.den
        return PVec(_raw(A * C - B * D_, den), _raw(A * D_ + B * C, den)).reduce()

    def abs2(self):
        A, B = _unit(self.re), _unit(self.im)
        return _raw(A * A + B * B, self.den * self.den)

    def total2(self):
        return (Fraction(self.re._sum(), self.den), Fraction(self.im._sum(), self.den))

    def values(self):
        D = self.den
        return [(Fraction(a, D), Fraction(b, D)) for a, b in zip(self.re.big(), self.im.big())]


def _unit(v):
    """The integers of a part as a vector over 1 (so products do not multiply denominators twice)."""
    return SVec(v.tier, v.val, 1, 0)


def _raw(v, den):
    return SVec(v.tier, v.val, den, 0)


def _scale(v, s):
    return _unit(v).scale_int(s) if s != 1 else _unit(v)


def _div(v, g, den):
    from svec import exact_divide
    if v.tier == "s64":
        return SVec("s64", exact_divide(v.val, g), den, 0)
    return SVec.from_ints([int(x) // g for x in v.big()], den, 0, reduce=False)


class APair:
    """A complex array as a real-part and an imaginary-part signed vector, each on its own denominator, written
    exactly like PVec (unreduced products, one reduction per part at the end), so that the two differ only in
    whether the parts share a denominator."""

    __slots__ = ("re", "im")

    def __init__(self, re, im):
        self.re, self.im = re, im

    @staticmethod
    def decimals(re_nums, im_nums, den):
        return APair(SVec.decimals(re_nums, den), SVec.decimals(im_nums, den))

    def __len__(self):
        return len(self.re)

    def __add__(self, o):
        return APair(self.re + o.re, self.im + o.im)

    def __mul__(self, o):
        """re = ar*br - ai*bi over the lcm of (dar dbr, dai dbi); im = ar*bi + ai*br likewise."""
        def prod(x, y):
            return _unit(x) * _unit(y), x.den * y.den

        (p1, d1), (p2, d2) = prod(self.re, o.re), prod(self.im, o.im)
        (p3, d3), (p4, d4) = prod(self.re, o.im), prod(self.im, o.re)
        L1, L2 = math.lcm(d1, d2), math.lcm(d3, d4)
        re = _raw(_scale(p1, L1 // d1) - _scale(p2, L1 // d2), L1).reduce()
        im = _raw(_scale(p3, L2 // d3) + _scale(p4, L2 // d4), L2).reduce()
        return APair(re, im)

    def abs2(self):
        a = _unit(self.re) * _unit(self.re)
        b = _unit(self.im) * _unit(self.im)
        d1, d2 = self.re.den ** 2, self.im.den ** 2
        L = math.lcm(d1, d2)
        return _raw(_scale(a, L // d1) + _scale(b, L // d2), L).reduce()

    def total2(self):
        return (Fraction(self.re._sum(), self.re.den), Fraction(self.im._sum(), self.im.den))

    def values(self):
        return [(Fraction(a, self.re.den), Fraction(b, self.im.den)) for a, b in zip(self.re.big(), self.im.big())]
