# The turn-byte vector form

The v0.1 vector layout: per element a turn byte and a U64 magnitude, over one denominator. Compiled with Numba; every constant that meets a U64 is a `np.uint64`, because Numba converts mixed signed and unsigned 64-bit arithmetic to floating point.

## Word helpers

```python {file=build/kern.py}
import numpy as np
from numba import njit

import reuse  # noqa: F401
from wide import mul_u64

U = np.uint64
ZERO, ONE = U(0), U(1)


# ---------------------------------------------------------------- word helpers

@njit(inline="always")
def bitlen(x):
    n = 0
    if x >> U(32):
        x >>= U(32)
        n += 32
    if x >> U(16):
        x >>= U(16)
        n += 16
    if x >> U(8):
        x >>= U(8)
        n += 8
    if x >> U(4):
        x >>= U(4)
        n += 4
    if x >> U(2):
        x >>= U(2)
        n += 2
    if x >> ONE:
        x >>= ONE
        n += 1
    return n + (1 if x != ZERO else 0)


@njit(inline="always")
def gcd64(a, b):
    while b != ZERO:
        a, b = b, a % b
    return a


@njit(inline="always")
def add192(a2, a1, a0, hi, lo):
    s0 = a0 + lo
    c0 = ONE if s0 < a0 else ZERO
    s1 = a1 + hi
    c1 = ONE if s1 < a1 else ZERO
    s1c = s1 + c0
    c1 += ONE if s1c < s1 else ZERO
    return a2 + c1, s1c, s0


```

## Kernels

Addition is sign-magnitude addition written branch-free (both $a+b$ and $|a-b|$ are computed and the right one selected with masks), because with random signs a branch per element mispredicts about half the time. Products go to a 128-bit tier unless the bit lengths of the operands prove they fit one word, in which case `nat_mul64` multiplies directly. Sums run in 64-bit words with an OR of the magnitudes that proves afterwards whether they were exact; otherwise a carrying kernel reruns.

```python {file=build/kern.py}
# ------------------------------------------- the vector form: turns and U64 magnitudes over one denominator

@njit
def nat_from_decimals(nums, ot, om):
    for i in range(len(nums)):
        v = nums[i]
        if v < 0:
            ot[i] = 2
            om[i] = U(-(v + 1)) + ONE
        else:
            ot[i] = 0
            om[i] = U(v)


@njit
def nat_gcd(m, start):
    g = start
    for i in range(len(m)):
        if g == ONE:
            return g
        if m[i] != ZERO:
            g = gcd64(m[i], g)
    return g


@njit
def nat_div(m, g, out):
    for i in range(len(m)):
        out[i] = m[i] // g


@njit
def nat_scale(m, s, out):
    bad = 0
    for i in range(len(m)):
        h, l = mul_u64(m[i], s)
        out[i] = l
        bad += 1 if h != ZERO else 0
    return bad


@njit
def nat_scale128(hi, lo, s, ohi, olo):
    bad = 0
    for i in range(len(hi)):
        h1, l1 = mul_u64(lo[i], s)
        h2, l2 = mul_u64(hi[i], s)
        mid = h1 + l2
        carry = ONE if mid < h1 else ZERO
        bad += 1 if (h2 + carry) != ZERO else 0
        ohi[i], olo[i] = mid, l1
    return bad


@njit(inline="always")
def pick(cond, a, b):
    """cond ? a : b on unsigned words, without a branch (cond is 0 or 1)."""
    return b ^ ((ZERO - cond) & (a ^ b))


@njit
def nat_add(ta, ma, tb, mb, negate_b, ot, om):
    """Sign-magnitude addition of two turn/magnitude columns, written branch-free: with random signs a branch
    per element mispredicts about half the time. Both a+b and |a-b| are computed and the right one is selected.
    Returns (overflow count, cross-axis count); cross-axis elements are left unspecified for the caller."""
    ovf = 0
    cross = 0
    shift = U(2) if negate_b else ZERO
    for i in range(len(ma)):
        a, b = ma[i], mb[i]
        x = U(ta[i])
        y = (U(tb[i]) + shift) & U(3)
        same = U(x == y)
        diff = ONE - same
        s = a + b
        ovf += int(same & U(s < a))
        lt = U(a < b)
        d = pick(lt, b - a, a - b)                                   # |a - b|
        td = pick(lt, y, x)                                          # direction of the larger
        td = pick(U(d == ZERO) & diff, ZERO, td)                     # exact cancellation: zero, turn 0
        m = pick(diff, d, s)
        t = pick(diff, td, x)
        t = pick(U(a == ZERO) & U(b == ZERO), ZERO, t)              # two zeros: zero, turn 0
        cross += int(U(a != ZERO) & U(b != ZERO) & ((x ^ y) & ONE))
        ot[i] = np.uint8(t)
        om[i] = m
    return ovf, cross


@njit
def nat_add128(ta, ahi, alo, tb, bhi, blo, negate_b, ot, ohi, olo):
    ovf = 0
    cross = 0
    for i in range(len(ahi)):
        x = np.int64(ta[i])
        y = (np.int64(tb[i]) + (2 if negate_b else 0)) & 3
        az = ahi[i] == ZERO and alo[i] == ZERO
        bz = bhi[i] == ZERO and blo[i] == ZERO
        if az and bz:
            ot[i], ohi[i], olo[i] = 0, ZERO, ZERO
        elif az:
            ot[i], ohi[i], olo[i] = y, bhi[i], blo[i]
        elif bz:
            ot[i], ohi[i], olo[i] = x, ahi[i], alo[i]
        elif (x ^ y) & 1:
            cross += 1
        elif x == y:
            lo = alo[i] + blo[i]
            c = ONE if lo < alo[i] else ZERO
            hi = ahi[i] + bhi[i]
            o1 = hi < ahi[i]
            hic = hi + c
            o2 = hic < hi
            ovf += 1 if (o1 or o2) else 0
            ot[i], ohi[i], olo[i] = x, hic, lo
        else:
            a_ge = ahi[i] > bhi[i] or (ahi[i] == bhi[i] and alo[i] >= blo[i])
            if a_ge:
                uh, ul, vh, vl, t = ahi[i], alo[i], bhi[i], blo[i], x
            else:
                uh, ul, vh, vl, t = bhi[i], blo[i], ahi[i], alo[i], y
            lo = ul - vl
            borrow = ONE if ul < vl else ZERO
            hi = uh - vh - borrow
            if hi == ZERO and lo == ZERO:
                t = 0
            ot[i], ohi[i], olo[i] = t, hi, lo
    return ovf, cross


@njit
def nat_or(m):
    """OR of all magnitudes: bitlen(OR) bounds every magnitude's length."""
    h = ZERO
    for i in range(len(m)):
        h |= m[i]
    return h


@njit
def nat_mul64(ta, ma, tb, mb, ot, om):
    """Products known to fit one word (the caller has checked the bit lengths)."""
    for i in range(len(ma)):
        p = ma[i] * mb[i]
        ot[i] = np.uint8(0 if p == ZERO else (np.int64(ta[i]) + np.int64(tb[i])) & 3)
        om[i] = p


@njit
def nat_mul(ta, ma, tb, mb, ot, ohi, olo):
    for i in range(len(ma)):
        ot[i] = 0 if (ma[i] == ZERO or mb[i] == ZERO) else (np.int64(ta[i]) + np.int64(tb[i])) & 3
        ohi[i], olo[i] = mul_u64(ma[i], mb[i])


LOW32 = U(0xFFFFFFFF)
S32 = U(32)


@njit
def nat_sums_fast(t, m):
    """Per-turn sums in plain 64-bit words, plus the OR of all magnitudes.
    The sums are exact whenever bitlen(OR) + bitlen(n) <= 64; the caller checks."""
    s0, s1, s2, s3, hb = ZERO, ZERO, ZERO, ZERO, ZERO
    for i in range(len(m)):
        a = m[i]
        j = U(t[i])
        hb |= a
        s0 += a & (ZERO - U(j == U(0)))
        s1 += a & (ZERO - U(j == U(1)))
        s2 += a & (ZERO - U(j == U(2)))
        s3 += a & (ZERO - U(j == U(3)))
    return s0, s1, s2, s3, hb


@njit
def nat_turn_sums_wide(t, m):
    acc = np.zeros((4, 2), dtype=np.uint64)
    for i in range(len(m)):
        j = t[i]
        a = m[i]
        lo = acc[j, 1] + a
        acc[j, 0] += ONE if lo < a else ZERO
        acc[j, 1] = lo
    return acc


@njit
def nat_moments_fast(t, m):
    """Real-axis sums, sum of squares split into 32-bit halves, OR of magnitudes, and OR of imaginary magnitudes.
    Exact whenever OR < 2**32 and n < 2**31; the caller checks."""
    sp, sn, hb, odd, qh, ql = ZERO, ZERO, ZERO, ZERO, ZERO, ZERO
    for i in range(len(m)):
        a = m[i]
        j = U(t[i])
        hb |= a
        odd |= a & (ZERO - (j & ONE))
        neg = ZERO - ((j >> ONE) & ONE)
        sn += a & neg
        sp += a & ~neg
        q = a * a
        qh += q >> S32
        ql += q & LOW32
    return sp, sn, hb, odd, qh, ql


@njit
def nat_moments_wide(t, m):
    s = np.zeros((4, 2), dtype=np.uint64)
    sq = np.zeros(3, dtype=np.uint64)
    for i in range(len(m)):
        j = t[i]
        a = m[i]
        lo = s[j, 1] + a
        s[j, 0] += ONE if lo < a else ZERO
        s[j, 1] = lo
        h2, l2 = mul_u64(a, a)
        sq[0], sq[1], sq[2] = add192(sq[0], sq[1], sq[2], h2, l2)
    return s, sq


@njit
def nat_group_moments_fast(keys, t, m, groups):
    """Per-group sums of positive and negative magnitudes, squares split into 32-bit halves,
    the OR of all magnitudes and the OR of imaginary magnitudes. Exact whenever OR < 2**32 and n < 2**31;
    the sums alone are exact whenever bitlen(OR) + bitlen(n) <= 64. The caller checks."""
    pos = np.zeros(groups, dtype=np.uint64)
    neg = np.zeros(groups, dtype=np.uint64)
    qh = np.zeros(groups, dtype=np.uint64)
    ql = np.zeros(groups, dtype=np.uint64)
    hb, odd = ZERO, ZERO
    for i in range(len(m)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        a = m[i]
        j = U(t[i])
        hb |= a
        odd |= a & (ZERO - (j & ONE))
        if j >= U(2):
            neg[g] += a
        else:
            pos[g] += a
        q = a * a
        qh[g] += q >> S32
        ql[g] += q & LOW32
    return pos, neg, qh, ql, hb, odd


@njit
def nat_group_moments(keys, t, m, groups):
    s = np.zeros((groups, 4, 2), dtype=np.uint64)
    sq = np.zeros((groups, 3), dtype=np.uint64)
    for i in range(len(m)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        j = t[i]
        a = m[i]
        lo = s[g, j, 1] + a
        s[g, j, 0] += ONE if lo < a else ZERO
        s[g, j, 1] = lo
        h2, l2 = mul_u64(a, a)
        sq[g, 0], sq[g, 1], sq[g, 2] = add192(sq[g, 0], sq[g, 1], sq[g, 2], h2, l2)
    return s, sq


@njit
def nat_greater(t, m, T_neg, T, out):
    """out[i] = signed(t[i], m[i]) > (-1)^T_neg * T for a real column."""
    for i in range(len(m)):
        a = m[i]
        neg = t[i] == 2 and a != ZERO
        if T_neg:
            out[i] = (not neg) or a < T
        else:
            out[i] = (not neg) and a > T
```

## The turn-byte vector

An operation runs in the higher operand tier (n64, n128, big) and is redone one tier up on overflow; results demote when every element fits; after arithmetic the common factor of the denominator and all magnitudes is divided out, stopping as soon as the running gcd reaches 1.

```python {file=build/qvec.py}
import math
from fractions import Fraction

import numpy as np

import kern
from q72 import Q, CrossAxis

N64_MAX = 2**64 - 1
N128_MAX = 2**128 - 1
SIGN_OF_TURN = (1, 1, -1, -1)


def _py_add(ta, ma, tb, mb, negate):
    """Element rule shared with the kernels, on Python ints (any size)."""
    ot, om = np.empty(len(ma), np.uint8), np.empty(len(ma), dtype=object)
    for i, (x, a, y, b) in enumerate(zip(ta.tolist(), ma, tb.tolist(), mb)):
        a, b = int(a), int(b)
        y = (y + (2 if negate else 0)) & 3
        if a == 0 and b == 0:
            ot[i], om[i] = 0, 0
        elif a == 0:
            ot[i], om[i] = y, b
        elif b == 0:
            ot[i], om[i] = x, a
        elif (x ^ y) & 1:
            raise CrossAxis("real plus imaginary")
        elif x == y:
            ot[i], om[i] = x, a + b
        elif a >= b:
            ot[i], om[i] = (x if a != b else 0), a - b
        else:
            ot[i], om[i] = y, b - a
    return ot, om


class QVec:
    """The Q72 vector form: element i is i**turn[i] * mag[i] / den, with one denominator for the vector.
    A zero element has magnitude 0 and turn 0.

    tier "n64": mag is uint64; "n128": mag is (hi, lo) uint64 words; "big": mag is Python ints.
    """

    __slots__ = ("tier", "turn", "mag", "den")

    def __init__(self, tier, turn, mag, den):
        self.tier, self.turn, self.mag, self.den = tier, turn, mag, int(den)
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
        nums = np.ascontiguousarray(nums, dtype=np.int64)
        t, m = np.empty(len(nums), np.uint8), np.empty(len(nums), np.uint64)
        kern.nat_from_decimals(nums, t, m)
        return QVec("n64", t, m, den).reduce()

    @staticmethod
    def from_parts(turn, mags, den, reduce=True):
        mags = [int(v) for v in mags]
        top = max(mags, default=0)
        turn = np.array(turn, dtype=np.uint8)
        turn[np.array([v == 0 for v in mags], dtype=np.bool_)] = 0  # zero has no direction
        if top <= N64_MAX:
            col = QVec("n64", turn, np.array(mags, dtype=np.uint64), den)
        elif top <= N128_MAX:
            hi = np.array([v >> 64 for v in mags], dtype=np.uint64)
            lo = np.array([v & N64_MAX for v in mags], dtype=np.uint64)
            col = QVec("n128", turn, (hi, lo), den)
        else:
            col = QVec("big", turn, np.array(mags, dtype=object), den)
        return col.reduce() if reduce else col

    def __len__(self):
        return len(self.turn)

    def big(self):
        if self.tier == "n64":
            return self.mag.astype(object)
        if self.tier == "n128":
            hi, lo = self.mag
            return (hi.astype(object) << 64) | lo.astype(object)
        return self.mag

    def n128(self):
        if self.tier == "n128":
            return self.mag
        return np.zeros(len(self), np.uint64), self.mag

    def magnitude(self, i):
        if self.tier == "n64":
            return int(self.mag[i])
        if self.tier == "n128":
            return (int(self.mag[0][i]) << 64) | int(self.mag[1][i])
        return int(self.mag[i])

    def num(self, i):
        return Q(int(self.turn[i]), Fraction(self.magnitude(i), self.den))

    def values(self):
        return [self.num(i) for i in range(len(self))]

    def demote(self):
        if self.tier == "n128" and not self.mag[0].any():
            return QVec("n64", self.turn, self.mag[1].copy(), self.den)
        if self.tier == "big":
            return QVec.from_parts(self.turn, self.mag, self.den, reduce=False)
        return self

    def reduce(self):
        if self.den == 1:
            return self
        if self.tier == "n64" and self.den <= N64_MAX:
            g = int(kern.nat_gcd(self.mag, np.uint64(self.den)))
            if g <= 1:
                return self
            out = np.empty_like(self.mag)
            kern.nat_div(self.mag, np.uint64(g), out)
            return QVec("n64", self.turn, out, self.den // g)
        g = self.den
        for i in range(len(self)):
            g = math.gcd(g, self.magnitude(i))
            if g == 1:
                return self
        return QVec.from_parts(self.turn, [int(v) // g for v in self.big()], self.den // g, reduce=False)

    def scale_int(self, s):
        if s == 1:
            return self
        if self.tier == "n64" and s <= N64_MAX:
            out = np.empty_like(self.mag)
            if kern.nat_scale(self.mag, np.uint64(s), out) == 0:
                return QVec("n64", self.turn, out, self.den)
        if self.tier in ("n64", "n128") and s <= N64_MAX:
            hi, lo = self.n128()
            ohi, olo = np.empty_like(hi), np.empty_like(lo)
            if kern.nat_scale128(hi, lo, np.uint64(s), ohi, olo) == 0:
                return QVec("n128", self.turn, (ohi, olo), self.den).demote()
        return QVec.from_parts(self.turn, self.big() * s, self.den, reduce=False)

    def _aligned(self, o):
        D = math.lcm(self.den, o.den)
        return self.scale_int(D // self.den), o.scale_int(D // o.den), D

    def _check_len(self, o):
        if len(self) != len(o):
            raise ValueError(f"columns of different lengths: {len(self)} and {len(o)}")

    def _addsub(self, o, negate):
        self._check_len(o)
        a, b, D = self._aligned(o)
        n = len(a)
        if a.tier == "n64" and b.tier == "n64":
            ot, om = np.empty(n, np.uint8), np.empty(n, np.uint64)
            ovf, cross = kern.nat_add(a.turn, a.mag, b.turn, b.mag, negate, ot, om)
            if cross:
                raise CrossAxis(f"{cross} elements add a real to an imaginary value")
            if not ovf:
                return QVec("n64", ot, om, D).reduce()
        if a.tier != "big" and b.tier != "big":
            (ah, al), (bh, bl) = a.n128(), b.n128()
            ot, oh, ol = np.empty(n, np.uint8), np.empty(n, np.uint64), np.empty(n, np.uint64)
            ovf, cross = kern.nat_add128(a.turn, ah, al, b.turn, bh, bl, negate, ot, oh, ol)
            if cross:
                raise CrossAxis(f"{cross} elements add a real to an imaginary value")
            if not ovf:
                return QVec("n128", ot, (oh, ol), D).demote().reduce()
        ot, om = _py_add(a.turn, a.big(), b.turn, b.big(), negate)
        return QVec.from_parts(ot, om, D)

    def __add__(self, o):
        return self._addsub(o, False)

    def __sub__(self, o):
        return self._addsub(o, True)

    def __mul__(self, o):
        self._check_len(o)
        D = self.den * o.den
        n = len(self)
        if self.tier == "n64" and o.tier == "n64":
            if int(kern.nat_or(self.mag)).bit_length() + int(kern.nat_or(o.mag)).bit_length() <= 64:
                ot, om = np.empty(n, np.uint8), np.empty(n, np.uint64)
                kern.nat_mul64(self.turn, self.mag, o.turn, o.mag, ot, om)
                return QVec("n64", ot, om, D).reduce()
            ot, hi, lo = np.empty(n, np.uint8), np.empty(n, np.uint64), np.empty(n, np.uint64)
            kern.nat_mul(self.turn, self.mag, o.turn, o.mag, ot, hi, lo)
            return QVec("n128", ot, (hi, lo), D).demote().reduce()
        mags = self.big() * o.big()
        turn = np.where(mags == 0, 0, (self.turn.astype(np.int64) + o.turn) & 3).astype(np.uint8)
        return QVec.from_parts(turn, mags, D)

    def rotate(self, k):
        k %= 4
        if k == 0:
            return self
        return QVec(self.tier, np.where(self.is_zero(), np.uint8(0), (self.turn + np.uint8(k)) & np.uint8(3)),
                    self.mag, self.den)

    def conj(self):
        return QVec(self.tier, (np.uint8(4) - self.turn) & np.uint8(3), self.mag, self.den)

    def turns(self):
        return self.turn

    def is_zero(self):
        if self.tier == "n64":
            return self.mag == 0
        if self.tier == "n128":
            return (self.mag[0] == 0) & (self.mag[1] == 0)
        return self.mag == 0

    def zero_where(self, mask):
        turn = np.where(mask, np.uint8(0), self.turn)
        if self.tier == "n64":
            return QVec("n64", turn, np.where(mask, np.uint64(0), self.mag), self.den)
        if self.tier == "n128":
            z = np.uint64(0)
            return QVec("n128", turn, (np.where(mask, z, self.mag[0]), np.where(mask, z, self.mag[1])),
                          self.den).demote()
        return QVec.from_parts(turn, np.where(mask, 0, self.mag), self.den, reduce=False)

    def select(self, mask, other):
        """Elementwise: other where mask, else self (over the common denominator)."""
        self._check_len(other)
        a, b, D = self._aligned(other)
        if a.tier == "n64" and b.tier == "n64":
            return QVec("n64", np.where(mask, b.turn, a.turn), np.where(mask, b.mag, a.mag), D).reduce()
        return QVec.from_parts(np.where(mask, b.turn, a.turn), np.where(mask, b.big(), a.big()), D)

    def real_part(self):
        return self.zero_where((self.turn & 1) == 1)

    def imag_part(self):
        return self.rotate(3).zero_where((self.turn & 1) == 0)

    def _signed_parts(self):
        re = im = 0
        for t, m in zip(self.turn.tolist(), self.big()):
            v = int(m) * SIGN_OF_TURN[t]
            if t & 1:
                im += v
            else:
                re += v
        return re, im

    def total2(self):
        if self.tier == "n64":
            *S, hb = kern.nat_sums_fast(self.turn, self.mag)
            if int(hb).bit_length() + len(self).bit_length() > 64:
                acc = kern.nat_turn_sums_wide(self.turn, self.mag)
                S = [(int(acc[t, 0]) << 64) | int(acc[t, 1]) for t in range(4)]
            S = [int(v) for v in S]
            re, im = S[0] - S[2], S[1] - S[3]
        else:
            re, im = self._signed_parts()
        return Fraction(re, self.den), Fraction(im, self.den)

    def total(self):
        re, im = self.total2()
        if im:
            raise ValueError("column has an imaginary total; use total2()")
        return re

    def mean(self):
        return self.total() / len(self)

    def _check_real(self):
        if np.any(((self.turn & 1) == 1) & ~self.is_zero()):
            raise TypeError("variance of an imaginary column")

    def var(self):
        n = len(self)
        if n < 2:
            raise ValueError("the sample variance needs at least two values")
        if self.tier == "n64":
            sp, sn, hb, odd, qh, ql = kern.nat_moments_fast(self.turn, self.mag)
            if odd:
                raise TypeError("variance of an imaginary column")
            if int(hb) < 2**32 and n < 2**31:
                s1, s2 = int(sp) - int(sn), (int(qh) << 32) + int(ql)
            else:
                s, sq = kern.nat_moments_wide(self.turn, self.mag)
                s1 = ((int(s[0, 0]) << 64) | int(s[0, 1])) - ((int(s[2, 0]) << 64) | int(s[2, 1]))
                s2 = (int(sq[0]) << 128) | (int(sq[1]) << 64) | int(sq[2])
        else:
            self._check_real()
            vals = [int(m) * SIGN_OF_TURN[t] for t, m in zip(self.turn.tolist(), self.big())]
            s1, s2 = sum(vals), sum(v * v for v in vals)
        return Fraction(n * s2 - s1 * s1, n * (n - 1) * self.den ** 2)

    def _group_words(self, keys, groups, need_squares):
        """Integer per-group sums (and sums of squares) of the signed magnitudes, or None for the Python path."""
        if len(keys) != len(self):
            raise ValueError(f"{len(keys)} keys for {len(self)} rows")
        if self.tier != "n64":
            return None
        n = len(self)
        pos, neg, qh, ql, hb, odd = kern.nat_group_moments_fast(keys, self.turn, self.mag, groups)
        if odd:
            raise TypeError("group statistics of an imaginary column")
        if need_squares and int(hb) < 2**32 and n < 2**31:
            return [int(a) - int(b) for a, b in zip(pos, neg)], [(int(h) << 32) + int(l) for h, l in zip(qh, ql)]
        if not need_squares and int(hb).bit_length() + n.bit_length() <= 64:
            return [int(a) - int(b) for a, b in zip(pos, neg)], None
        s, sq = kern.nat_group_moments(keys, self.turn, self.mag, groups)
        s1 = [((int(s[g, 0, 0]) << 64) | int(s[g, 0, 1])) - ((int(s[g, 2, 0]) << 64) | int(s[g, 2, 1]))
              for g in range(groups)]
        s2 = [(int(sq[g, 0]) << 128) | (int(sq[g, 1]) << 64) | int(sq[g, 2]) for g in range(groups)]
        return s1, s2

    def _group_python(self, keys, groups):
        self._check_real()
        s1, s2 = [0] * groups, [0] * groups
        for k, t, m in zip(keys.tolist(), self.turn.tolist(), self.big()):
            if not 0 <= k < groups:
                raise IndexError("group key out of range")
            v = int(m) * SIGN_OF_TURN[t]
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
        T = (q.numerator * self.den) // q.denominator
        if self.tier == "n64" and abs(T) <= N64_MAX:
            out = np.empty(len(self), dtype=np.bool_)
            kern.nat_greater(self.turn, self.mag, T < 0, np.uint64(abs(T)), out)
            return out
        return np.array([int(m) * SIGN_OF_TURN[t] > T for t, m in zip(self.turn.tolist(), self.big())],
                        dtype=np.bool_)

    def take(self, mask):
        if self.tier == "n128":
            mag = (self.mag[0][mask], self.mag[1][mask])
        else:
            mag = self.mag[mask]
        return QVec(self.tier, self.turn[mask], mag, self.den).demote()
```
