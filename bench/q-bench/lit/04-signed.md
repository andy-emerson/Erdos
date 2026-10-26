# The signed vector form

The signed layout stores each element as a two's complement int64 and the axis once per vector: element $i$ is $i^{a}\,x_i/d$ for the vector's axis $a$. The sign is part of the integer, as in quote notation, where $-1$ is the 2-adic expansion $\ldots 1111$.

## Kernels

All arithmetic is done on the bit patterns as `uint64`, which wraps modulo $2^{64}$ exactly as two's complement does, and overflow is read from the sign bits ($a+b$ overflows when $a$ and $b$ agree in sign and the sum does not), so nothing relies on signed overflow. The 128-bit tier holds a signed high word and an unsigned low word. Exact division comes in two forms: the (unsigned) divide instruction on magnitudes, and quote notation's method, which shifts out the power of two and multiplies by the inverse of the odd part modulo $2^{64}$; in two's complement that is exact for negative values too, because the quotient is known to be an integer in range.

```python {file=build/skern.py}
"""Kernels for the signed vector form: two's complement integers over one denominator, one axis per vector.

Arithmetic is done on the bit patterns as uint64, which wraps modulo 2**64 exactly as two's complement does,
and overflow is detected from the sign bits, so no step relies on signed overflow behaviour."""
import numpy as np
from numba import njit

import reuse  # noqa: F401
from wide import mul_u64
from kern import add192, gcd64

U, I = np.uint64, np.int64
ZERO, ONE = U(0), U(1)
S63 = U(63)
LOW32 = U(0xFFFFFFFF)
S32 = U(32)


@njit(inline="always")
def uabs(x):
    """|x| as uint64, branch-free (|-2**63| = 2**63)."""
    s = U(x >> I(63))
    return (U(x) ^ s) - s


@njit
def s_or_abs(x):
    h = ZERO
    for i in range(len(x)):
        h |= uabs(x[i])
    return h


@njit
def s_add(a, b, negate_b, out):
    """out = a + b (or a - b) in int64; returns the number of elements that overflowed (left unspecified)."""
    ovf = ZERO
    for i in range(len(a)):
        ua, ub = U(a[i]), U(b[i])
        if negate_b:
            s = ua - ub
            ovf += ((ua ^ ub) & (ua ^ s)) >> S63
        else:
            s = ua + ub
            ovf += (~(ua ^ ub) & (ua ^ s)) >> S63
        out[i] = I(s)
    return ovf


@njit
def s_add128(ahi, alo, bhi, blo, negate_b, ohi, olo):
    """Signed 128-bit add or subtract, each value as (hi int64, lo uint64); returns the overflow count."""
    ovf = ZERO
    for i in range(len(alo)):
        xh, xl = U(ahi[i]), alo[i]
        yh, yl = U(bhi[i]), blo[i]
        if negate_b:
            yl = ~yl + ONE
            yh = ~yh + (ONE if yl == ZERO else ZERO)
            ovf += ONE if (bhi[i] == I(-2**63) and blo[i] == ZERO) else ZERO
        lo = xl + yl
        c = ONE if lo < xl else ZERO
        hi = xh + yh + c
        ovf += (~(xh ^ yh) & (xh ^ hi)) >> S63
        ohi[i], olo[i] = I(hi), lo
    return ovf


@njit
def s_mul64(a, b, out):
    """Products known to fit int64 (the caller has checked bit lengths)."""
    for i in range(len(a)):
        out[i] = I(U(a[i]) * U(b[i]))


@njit
def s_mul128(a, b, ohi, olo):
    """Signed 128-bit products of int64 values."""
    for i in range(len(a)):
        hi, lo = mul_u64(uabs(a[i]), uabs(b[i]))
        if (a[i] < 0) != (b[i] < 0):
            lo = ~lo + ONE
            hi = ~hi + (ONE if lo == ZERO else ZERO)
        ohi[i], olo[i] = I(hi), lo


@njit
def s_scale128(hi, lo, s, ohi, olo):
    """Signed 128-bit values times a positive uint64 s; returns the overflow count."""
    bad = ZERO
    for i in range(len(lo)):
        neg = hi[i] < 0
        h, l = U(hi[i]), lo[i]
        if neg:
            l = ~l + ONE
            h = ~h + (ONE if l == ZERO else ZERO)
        h1, l1 = mul_u64(l, s)
        h2, l2 = mul_u64(h, s)
        mid = h1 + l2
        carry = ONE if mid < h1 else ZERO
        bad += ONE if ((h2 + carry) != ZERO or (mid >> S63) != ZERO) else ZERO
        if neg:
            l1 = ~l1 + ONE
            mid = ~mid + (ONE if l1 == ZERO else ZERO)
        ohi[i], olo[i] = I(mid), l1
    return bad


@njit
def s_fits64(hi, lo):
    """True when every signed 128-bit value is a sign-extended int64."""
    for i in range(len(lo)):
        if hi[i] != I(lo[i]) >> I(63):
            return False
    return True


@njit
def s_gcd(x, start):
    g = start
    for i in range(len(x)):
        if g == ONE:
            return g
        a = uabs(x[i])
        if a != ZERO:
            g = gcd64(a, g)
    return g


@njit
def s_div_hardware(x, g, out):
    """Exact division by g with the (unsigned) divide instruction: divide the magnitude, restore the sign."""
    for i in range(len(x)):
        s = U(x[i] >> I(63))
        q = ((U(x[i]) ^ s) - s) // g
        out[i] = I((q ^ s) - s)


@njit
def s_div_inverse(x, k, inv, out):
    """Exact division by g = 2**k * odd, the way quote notation divides: shift out 2**k (exact, so an
    arithmetic shift), then multiply by the inverse of the odd part modulo 2**64. In two's complement this is
    exact for negative values too, because the quotient is known to be an integer in range."""
    kk = I(k)
    for i in range(len(x)):
        out[i] = I(U(x[i] >> kk) * inv)


@njit
def s_sums_fast(x):
    """Wrapping 64-bit sum and the OR of magnitudes; exact when bitlen(OR) + bitlen(n) <= 63."""
    acc, h = ZERO, ZERO
    for i in range(len(x)):
        acc += U(x[i])
        h |= uabs(x[i])
    return I(acc), h


@njit
def s_sum_wide(x):
    """Signed 128-bit sum: (hi, lo) as uint64 words."""
    hi, lo = ZERO, ZERO
    for i in range(len(x)):
        v = U(x[i])
        n = lo + v
        hi += (ONE if n < lo else ZERO) + U(x[i] >> I(63))
        lo = n
    return hi, lo


@njit
def s_sum128(xh, xl):
    """Signed sum of signed 128-bit values, in a 192-bit accumulator: (top, hi, lo) as uint64 words."""
    t, hi, lo = ZERO, ZERO, ZERO
    for i in range(len(xl)):
        sign = U(xh[i] >> I(63))
        n = lo + xl[i]
        c = ONE if n < lo else ZERO
        lo = n
        h1 = hi + U(xh[i])
        c2 = ONE if h1 < hi else ZERO
        h2 = h1 + c
        c2 += ONE if h2 < h1 else ZERO
        hi = h2
        t += c2 + sign
    return t, hi, lo


@njit
def s_moments_fast(x):
    """Sum, sum of squares split into 32-bit halves, and the OR of magnitudes.
    Exact whenever OR < 2**32 and n < 2**31; the caller checks."""
    acc, h, qh, ql = ZERO, ZERO, ZERO, ZERO
    for i in range(len(x)):
        a = uabs(x[i])
        acc += U(x[i])
        h |= a
        q = a * a
        qh += q >> S32
        ql += q & LOW32
    return I(acc), h, qh, ql


@njit
def s_moments_wide(x):
    shi, slo = ZERO, ZERO
    sq = np.zeros(3, dtype=np.uint64)
    for i in range(len(x)):
        v = U(x[i])
        n = slo + v
        shi += (ONE if n < slo else ZERO) + U(x[i] >> I(63))
        slo = n
        h2, l2 = mul_u64(uabs(x[i]), uabs(x[i]))
        sq[0], sq[1], sq[2] = add192(sq[0], sq[1], sq[2], h2, l2)
    return shi, slo, sq


@njit
def s_group_moments_fast(keys, x, groups):
    """Per-group wrapping sums, squares in 32-bit halves, and the OR of magnitudes (exact as above)."""
    s = np.zeros(groups, dtype=np.uint64)
    qh = np.zeros(groups, dtype=np.uint64)
    ql = np.zeros(groups, dtype=np.uint64)
    h = ZERO
    for i in range(len(x)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        a = uabs(x[i])
        h |= a
        s[g] += U(x[i])
        q = a * a
        qh[g] += q >> S32
        ql[g] += q & LOW32
    return s, qh, ql, h


@njit
def s_group_sums_fast(keys, x, groups):
    s = np.zeros(groups, dtype=np.uint64)
    h = ZERO
    for i in range(len(x)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        h |= uabs(x[i])
        s[g] += U(x[i])
    return s, h


@njit
def s_group_moments(keys, x, groups):
    s = np.zeros((groups, 2), dtype=np.uint64)
    sq = np.zeros((groups, 3), dtype=np.uint64)
    for i in range(len(x)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        v = U(x[i])
        n = s[g, 1] + v
        s[g, 0] += (ONE if n < s[g, 1] else ZERO) + U(x[i] >> I(63))
        s[g, 1] = n
        a = uabs(x[i])
        h2, l2 = mul_u64(a, a)
        sq[g, 0], sq[g, 1], sq[g, 2] = add192(sq[g, 0], sq[g, 1], sq[g, 2], h2, l2)
    return s, sq
```

## The signed vector

The same tiers and rules as the turn-byte vector (s64, s128, big; redo one tier up on overflow; demote; reduce by the common factor). Construction from decimal numerators is a copy. Multiplying by $i$ turns the axis, and negates the values when the turn passes $-1$. Adding a real vector to an imaginary one raises `CrossAxis` unless one of them is all zero; a complex array is a real vector and an imaginary vector.

```python {file=build/svec.py}
import math
from fractions import Fraction

import numpy as np

import skern as K
from q72 import Q, CrossAxis

I64_MIN, I64_MAX = -2**63, 2**63 - 1
I128_MIN, I128_MAX = -2**127, 2**127 - 1
MASK64 = 2**64 - 1

# How the vector reduces by a common factor: "inverse" multiplies by the 2-adic inverse of the odd part,
# "hardware" uses the divide instruction. Both are exact; the benchmark compares them.
DIVISION = "inverse"


def _to_int(hi, lo):
    """A signed 128-bit value from (hi as a signed or unsigned word, lo unsigned) Python ints."""
    v = ((int(hi) & MASK64) << 64) | int(lo)
    return v - (1 << 128) if v >> 127 else v


def exact_divide(x, g):
    """x // g for an int64 array known to be divisible by g (1 < g < 2**63)."""
    out = np.empty_like(x)
    if DIVISION == "inverse":
        k = (g & -g).bit_length() - 1
        odd = g >> k
        K.s_div_inverse(x, k, np.uint64(pow(odd, -1, 2**64)), out)
    else:
        K.s_div_hardware(x, np.uint64(g), out)
    return out


class SVec:
    """The signed vector form: element i is i**axis * val[i] / den. The values are two's complement integers,
    so the sign is part of each value; the axis (0 real, 1 imaginary) is one bit for the whole vector.
    A complex array is two vectors, a real part and an imaginary part.

    tier "s64": val is int64; "s128": val is (hi int64, lo uint64); "big": val is Python ints."""

    __slots__ = ("tier", "val", "den", "axis")

    def __init__(self, tier, val, den, axis=0):
        self.tier, self.val, self.den, self.axis = tier, val, int(den), axis & 1
        if self.den < 1:
            raise ValueError("the denominator must be a positive integer")

    @staticmethod
    def decimals(nums, den):
        den = int(den)
        if den < 1:
            raise ValueError("the denominator must be a positive integer")
        nums = np.asarray(nums)
        if nums.dtype.kind not in "iu":
            raise TypeError("numerators must be integers; the vector form never rounds")
        return SVec("s64", np.array(nums, dtype=np.int64), den).reduce()

    @staticmethod
    def from_ints(vals, den, axis=0, reduce=True):
        vals = [int(v) for v in vals]
        lo_, hi_ = min(vals, default=0), max(vals, default=0)
        if I64_MIN <= lo_ and hi_ <= I64_MAX:
            col = SVec("s64", np.array(vals, dtype=np.int64), den, axis)
        elif I128_MIN <= lo_ and hi_ <= I128_MAX:
            col = SVec("s128", (np.array([v >> 64 for v in vals], dtype=np.int64),
                                np.array([v & MASK64 for v in vals], dtype=np.uint64)), den, axis)
        else:
            col = SVec("big", np.array(vals, dtype=object), den, axis)
        return col.reduce() if reduce else col

    def __len__(self):
        return len(self.val[1]) if self.tier == "s128" else len(self.val)

    def big(self):
        if self.tier == "s64":
            return self.val.astype(object)
        if self.tier == "s128":
            hi, lo = self.val
            return (hi.astype(object) << 64) + lo.astype(object)
        return self.val

    def value(self, i):
        if self.tier == "s128":
            return (int(self.val[0][i]) << 64) + int(self.val[1][i])
        return int(self.val[i])

    def num(self, i):
        return Q.of(Fraction(self.value(i), self.den), self.axis)

    def values(self):
        return [self.num(i) for i in range(len(self))]

    def _like(self, tier, val, den=None, axis=None):
        return SVec(tier, val, self.den if den is None else den, self.axis if axis is None else axis)

    def demote(self):
        if self.tier == "s128" and K.s_fits64(*self.val):
            return self._like("s64", self.val[1].view(np.int64).copy())
        if self.tier == "big":
            return SVec.from_ints(self.val, self.den, self.axis, reduce=False)
        return self

    def reduce(self):
        if self.den == 1:
            return self
        if self.tier == "s64" and self.den <= I64_MAX:
            g = int(K.s_gcd(self.val, np.uint64(self.den)))
            if g <= 1:
                return self
            return self._like("s64", exact_divide(self.val, g), self.den // g)
        g = self.den
        for i in range(len(self)):
            g = math.gcd(g, self.value(i))
            if g == 1:
                return self
        return SVec.from_ints([v // g for v in self.big()], self.den // g, self.axis, reduce=False)

    def scale_int(self, s):
        if s == 1:
            return self
        if self.tier == "s64" and int(K.s_or_abs(self.val)).bit_length() + s.bit_length() <= 63:
            return self._like("s64", self.val * np.int64(s))
        if self.tier in ("s64", "s128") and s < 2**64:
            hi, lo = self._128()
            ohi, olo = np.empty_like(hi), np.empty_like(lo)
            if K.s_scale128(hi, lo, np.uint64(s), ohi, olo) == 0:
                return self._like("s128", (ohi, olo)).demote()
        return SVec.from_ints(self.big() * s, self.den, self.axis, reduce=False)

    def _128(self):
        if self.tier == "s128":
            return self.val
        return self.val >> np.int64(63), self.val.view(np.uint64)

    def _aligned(self, o):
        D = math.lcm(self.den, o.den)
        return self.scale_int(D // self.den), o.scale_int(D // o.den), D

    def _check_len(self, o):
        if len(self) != len(o):
            raise ValueError(f"columns of different lengths: {len(self)} and {len(o)}")

    def all_zero(self):
        if self.tier == "s128":
            return not (self.val[0].any() or self.val[1].any())
        return not self.val.any()

    def _addsub(self, o, negate):
        self._check_len(o)
        if self.axis != o.axis:
            if o.all_zero():
                return self
            if self.all_zero():
                return o.rotate(2) if negate else o
            raise CrossAxis("a real vector plus an imaginary vector is a complex array of two vectors")
        a, b, D = self._aligned(o)
        if a.tier == "s64" and b.tier == "s64":
            out = np.empty_like(a.val)
            if K.s_add(a.val, b.val, negate, out) == 0:
                return SVec("s64", out, D, self.axis).reduce()
        if a.tier != "big" and b.tier != "big":
            (ah, al), (bh, bl) = a._128(), b._128()
            oh, ol = np.empty_like(ah), np.empty_like(al)
            if K.s_add128(ah, al, bh, bl, negate, oh, ol) == 0:
                return SVec("s128", (oh, ol), D, self.axis).demote().reduce()
        vals = a.big() - b.big() if negate else a.big() + b.big()
        return SVec.from_ints(vals, D, self.axis)

    def __add__(self, o):
        return self._addsub(o, False)

    def __sub__(self, o):
        return self._addsub(o, True)

    def __mul__(self, o):
        self._check_len(o)
        D = self.den * o.den
        # i**a * i**b = i**(a+b): two imaginaries make a negative real
        axis, negate = (self.axis + o.axis) & 1, self.axis & o.axis
        if self.tier == "s64" and o.tier == "s64":
            if int(K.s_or_abs(self.val)).bit_length() + int(K.s_or_abs(o.val)).bit_length() <= 63:
                out = np.empty_like(self.val)
                K.s_mul64(self.val, o.val, out)
                col = SVec("s64", out, D, axis)
            else:
                hi, lo = np.empty(len(self), np.int64), np.empty(len(self), np.uint64)
                K.s_mul128(self.val, o.val, hi, lo)
                col = SVec("s128", (hi, lo), D, axis).demote()
            return (col.rotate(2) if negate else col).reduce()
        vals = self.big() * o.big()
        return SVec.from_ints(-vals if negate else vals, D, axis)

    def negate(self):
        if self.tier == "s64" and not np.any(self.val == np.int64(I64_MIN)):
            return self._like("s64", -self.val)
        return SVec.from_ints(-self.big(), self.den, self.axis, reduce=False)

    def rotate(self, k):
        """Times i**k: the axis turns, and the values change sign when the turn passes -1."""
        k %= 4
        if k == 0:
            return self
        # i**axis * i**k = i**(axis+k); write it as i**((axis+k) mod 2) * (-1)**((axis+k) div 2)
        t = self.axis + k
        out = self._like(self.tier, self.val, axis=t & 1)
        return out.negate() if (t >> 1) & 1 else out

    def conj(self):
        return self.negate() if self.axis else self

    def is_zero(self):
        if self.tier == "s128":
            return (self.val[0] == 0) & (self.val[1] == 0)
        return self.val == 0

    def zero_where(self, mask):
        if self.tier == "s64":
            return self._like("s64", np.where(mask, np.int64(0), self.val))
        if self.tier == "s128":
            return self._like("s128", (np.where(mask, np.int64(0), self.val[0]),
                                       np.where(mask, np.uint64(0), self.val[1]))).demote()
        return SVec.from_ints(np.where(mask, 0, self.val), self.den, self.axis, reduce=False)

    def select(self, mask, other):
        self._check_len(other)
        if self.axis != other.axis:
            if other.all_zero():
                other = SVec("s64", np.zeros(len(other), np.int64), 1, self.axis)
            elif self.all_zero():
                self = SVec("s64", np.zeros(len(self), np.int64), 1, other.axis)
            else:
                raise CrossAxis("select between a real and an imaginary vector")
        a, b, D = self._aligned(other)
        if a.tier == "s64" and b.tier == "s64":
            return SVec("s64", np.where(mask, b.val, a.val), D, self.axis).reduce()
        return SVec.from_ints(np.where(mask, b.big(), a.big()), D, self.axis)

    def real_part(self):
        return self if self.axis == 0 else self._zeros(0)

    def imag_part(self):
        """The imaginary part as a real vector of coefficients."""
        return self.rotate(3) if self.axis == 1 else self._zeros(0)

    def _zeros(self, axis):
        return SVec("s64", np.zeros(len(self), np.int64), 1, axis)

    def _sum(self):
        if self.tier == "s64":
            s, h = K.s_sums_fast(self.val)
            if int(h).bit_length() + len(self).bit_length() <= 63:
                return int(s)
            hi, lo = K.s_sum_wide(self.val)
            return _to_int(hi, lo)
        if self.tier == "s128":
            t, hi, lo = K.s_sum128(*self.val)
            v = (int(t) << 128) | (int(hi) << 64) | int(lo)
            return v - (1 << 192) if v >> 191 else v
        return sum(self.val)

    def total2(self):
        s = Fraction(self._sum(), self.den)
        return (s, Fraction(0)) if self.axis == 0 else (Fraction(0), s)

    def total(self):
        if self.axis and not self.all_zero():
            raise ValueError("column has an imaginary total; use total2()")
        return Fraction(self._sum(), self.den) if self.axis == 0 else Fraction(0)

    def mean(self):
        return self.total() / len(self)

    def _check_real(self):
        if self.axis and not self.all_zero():
            raise TypeError("statistics of an imaginary column")

    def var(self):
        n = len(self)
        if n < 2:
            raise ValueError("the sample variance needs at least two values")
        self._check_real()
        if self.tier == "s64":
            s, h, qh, ql = K.s_moments_fast(self.val)
            if int(h) < 2**32 and n < 2**31:
                s1, s2 = int(s), (int(qh) << 32) + int(ql)
            else:
                shi, slo, sq = K.s_moments_wide(self.val)
                s1 = _to_int(shi, slo)
                s2 = (int(sq[0]) << 128) | (int(sq[1]) << 64) | int(sq[2])
        else:
            vals = self.big()
            s1, s2 = int(sum(vals)), int(sum(v * v for v in vals))
        return Fraction(n * s2 - s1 * s1, n * (n - 1) * self.den ** 2)

    def _group_words(self, keys, groups, need_squares):
        if len(keys) != len(self):
            raise ValueError(f"{len(keys)} keys for {len(self)} rows")
        self._check_real()
        if self.tier != "s64":
            return None
        n = len(self)
        if not need_squares:
            s, h = K.s_group_sums_fast(keys, self.val, groups)
            if int(h).bit_length() + n.bit_length() <= 63:
                return [int(v) - (1 << 64) if v >> 63 else int(v) for v in s.tolist()], None
        else:
            s, qh, ql, h = K.s_group_moments_fast(keys, self.val, groups)
            if int(h) < 2**32 and n < 2**31:
                return ([int(v) - (1 << 64) if v >> 63 else int(v) for v in s.tolist()],
                        [(int(a) << 32) + int(b) for a, b in zip(qh, ql)])
        s, sq = K.s_group_moments(keys, self.val, groups)
        s1 = [_to_int(s[g, 0], s[g, 1]) for g in range(groups)]
        s2 = [(int(sq[g, 0]) << 128) | (int(sq[g, 1]) << 64) | int(sq[g, 2]) for g in range(groups)]
        return s1, s2

    def _group_python(self, keys, groups):
        s1, s2 = [0] * groups, [0] * groups
        for k, v in zip(keys.tolist(), self.big()):
            if not 0 <= k < groups:
                raise IndexError("group key out of range")
            v = int(v)
            s1[k] += v
            s2[k] += v * v
        return s1, s2

    def group_moments(self, keys, groups):
        keys = np.ascontiguousarray(keys, dtype=np.int64)
        r = self._group_words(keys, groups, True)
        s1, s2 = r if r is not None else self._group_python(keys, groups)
        counts = np.bincount(keys, minlength=groups)
        d = self.den
        return counts, [Fraction(a, d) for a in s1], [Fraction(b, d * d) for b in s2]

    def group_numerators(self, keys, groups):
        """Per-group sums as integer numerators over self.den (the raw result, before building fractions)."""
        keys = np.ascontiguousarray(keys, dtype=np.int64)
        r = self._group_words(keys, groups, False)
        return (r[0] if r is not None else self._group_python(keys, groups)[0]), self.den

    def group_sum(self, keys, groups):
        keys = np.ascontiguousarray(keys, dtype=np.int64)
        r = self._group_words(keys, groups, False)
        s1 = r[0] if r is not None else self._group_python(keys, groups)[0]
        return [Fraction(a, self.den) for a in s1]

    def group_mean_var(self, keys, groups):
        counts, F1, F2 = self.group_moments(keys, groups)
        out = []
        for n, a, b in zip(counts.tolist(), F1, F2):
            mean = a / n if n else None
            var = (n * b - a * a) / (n * (n - 1)) if n > 1 else None
            out.append((mean, var))
        return out

    def greater(self, q):
        self._check_real()
        q = Fraction(q)
        T = (q.numerator * self.den) // q.denominator   # x/den > q  <=>  x > floor(q*den)
        if self.tier == "s64":
            if T >= I64_MAX:
                return np.zeros(len(self), dtype=np.bool_)
            if T < I64_MIN:
                return np.ones(len(self), dtype=np.bool_)
            return self.val > np.int64(T)
        return np.array([v > T for v in self.big()], dtype=np.bool_)

    def take(self, mask):
        if self.tier == "s128":
            return self._like("s128", (self.val[0][mask], self.val[1][mask])).demote()
        return self._like(self.tier, self.val[mask]).demote()
```
