# The layout experiment

The three layouts, timed on identical work. The packed kernels read each byte of turns once for its four elements, handling whole bytes first and the remainder after, so that the comparison measures the layout rather than the loop. Each kernel does the whole job of its layout: the turn layouts check that every element is real before summing, and write turns and magnitudes back, while all three rely on the same caller-side proof (bit lengths of the operands) that results fit one word. Only the storage of the sign and the imaginary bit differs.

```python {file=build/layouts.py}
"""The layout experiment: the same operations on the same vectors, in three layouts of the vector form.

- turn bytes: a byte per element holding the turn (sign and imaginary bits), and a uint64 magnitude;
- packed turns: the turns packed four to a byte, and a uint64 magnitude;
- signed: a two's complement int64 per element, with the axis stored once for the vector.

Every kernel does the full job of its layout: the turn layouts check that every element is real before
summing, and write turns and magnitudes back; all three rely on the same caller-side proof (bit lengths of the
operands) that results fit one word. Only the layout differs."""
import numpy as np
from numba import njit

U, I = np.uint64, np.int64
ZERO, ONE = U(0), U(1)
LOW32, S32 = U(0xFFFFFFFF), U(32)


# ---------------------------------------------------------------- conversions from int64 numerators

@njit
def to_turn_bytes(x, t, m):
    for i in range(len(x)):
        s = U(x[i] >> I(63))
        m[i] = (U(x[i]) ^ s) - s
        t[i] = np.uint8(s & U(2))


@njit
def to_packed(x, tp, m):
    """Four turns to a byte, each byte written once."""
    n = len(x)
    for j in range(len(tp)):
        acc = ZERO
        for q in range(4):
            i = 4 * j + q
            if i < n:
                s = U(x[i] >> I(63))
                m[i] = (U(x[i]) ^ s) - s
                acc |= (s & U(2)) << U(2 * q)
        tp[j] = np.uint8(acc)


# ---------------------------------------------------------------- sum (real vectors)

@njit
def sum_turn_bytes(t, m):
    acc, odd = ZERO, ZERO
    for i in range(len(m)):
        j = U(t[i])
        odd |= m[i] & (ZERO - (j & ONE))
        s = ZERO - ((j >> ONE) & ONE)
        acc += (m[i] ^ s) - s
    return I(acc), odd


@njit(inline="always")
def _sum_step(t, a, acc, odd):
    odd |= a & (ZERO - (t & ONE))
    s = ZERO - ((t >> ONE) & ONE)
    return acc + ((a ^ s) - s), odd


@njit
def sum_packed(tp, m):
    """Each byte of turns read once for its four elements; whole bytes first, then the remainder."""
    acc, odd = ZERO, ZERO
    n = len(m)
    full = n >> 2
    for j in range(full):
        b = U(tp[j])
        i = 4 * j
        acc, odd = _sum_step(b & U(3), m[i], acc, odd)
        acc, odd = _sum_step((b >> U(2)) & U(3), m[i + 1], acc, odd)
        acc, odd = _sum_step((b >> U(4)) & U(3), m[i + 2], acc, odd)
        acc, odd = _sum_step((b >> U(6)) & U(3), m[i + 3], acc, odd)
    for i in range(4 * full, n):
        acc, odd = _sum_step((U(tp[i >> 2]) >> U(2 * (i & 3))) & U(3), m[i], acc, odd)
    return I(acc), odd


@njit
def sum_signed(x):
    acc = ZERO
    for i in range(len(x)):
        acc += U(x[i])
    return I(acc)


# ---------------------------------------------------------------- a*b + c*k (results known to fit)

@njit
def fma_turn_bytes(ta, ma, tb, mb, tc, mc, k, ot, om):
    odd = ZERO
    for i in range(len(ma)):
        x, y, z = U(ta[i]), U(tb[i]), U(tc[i])
        odd |= (x | y | z) & ONE
        sp = ZERO - (((x + y) >> ONE) & ONE)
        sc = ZERO - ((z >> ONE) & ONE)
        r = ((ma[i] * mb[i]) ^ sp) - sp + ((mc[i] * k) ^ sc) - sc
        s = U(I(r) >> I(63))
        om[i] = (r ^ s) - s
        ot[i] = np.uint8(s & U(2))
    return odd


@njit
def fma_packed(ta, ma, tb, mb, tc, mc, k, ot, om):
    """Each input byte of turns read once and each output byte written once, per four elements."""
    odd = ZERO
    n = len(ma)
    for j in range(len(ot)):
        ba, bb, bc = U(ta[j]), U(tb[j]), U(tc[j])
        odd |= (ba | bb | bc) & U(0x55)
        acc = ZERO
        for q in range(4):
            i = 4 * j + q
            if i < n:
                sh = U(2 * q)
                x, y, z = (ba >> sh) & U(3), (bb >> sh) & U(3), (bc >> sh) & U(3)
                sp = ZERO - (((x + y) >> ONE) & ONE)
                sc = ZERO - ((z >> ONE) & ONE)
                r = ((ma[i] * mb[i]) ^ sp) - sp + ((mc[i] * k) ^ sc) - sc
                s = U(I(r) >> I(63))
                om[i] = (r ^ s) - s
                acc |= (s & U(2)) << sh
        ot[j] = np.uint8(acc)
    return odd


@njit
def fma_signed(a, b, c, k, out):
    kk = U(k)
    for i in range(len(a)):
        out[i] = I(U(a[i]) * U(b[i]) + U(c[i]) * kk)


# ---------------------------------------------------------------- group sums and moments (real vectors)

@njit
def group_turn_bytes(keys, t, m, groups):
    s = np.zeros(groups, dtype=np.uint64)
    odd = ZERO
    for i in range(len(m)):
        j = U(t[i])
        odd |= m[i] & (ZERO - (j & ONE))
        sg = ZERO - ((j >> ONE) & ONE)
        s[keys[i]] += (m[i] ^ sg) - sg
    return s, odd


@njit
def group_packed(keys, tp, m, groups):
    s = np.zeros(groups, dtype=np.uint64)
    odd = ZERO
    n = len(m)
    full = n >> 2
    for j in range(full):
        b = U(tp[j])
        for q in range(4):
            i = 4 * j + q
            t = (b >> U(2 * q)) & U(3)
            odd |= m[i] & (ZERO - (t & ONE))
            sg = ZERO - ((t >> ONE) & ONE)
            s[keys[i]] += (m[i] ^ sg) - sg
    for i in range(4 * full, n):
        t = (U(tp[i >> 2]) >> U(2 * (i & 3))) & U(3)
        odd |= m[i] & (ZERO - (t & ONE))
        sg = ZERO - ((t >> ONE) & ONE)
        s[keys[i]] += (m[i] ^ sg) - sg
    return s, odd


@njit
def group_signed(keys, x, groups):
    s = np.zeros(groups, dtype=np.uint64)
    for i in range(len(x)):
        s[keys[i]] += U(x[i])
    return s


@njit
def moments_turn_bytes(t, m):
    acc, odd, qh, ql = ZERO, ZERO, ZERO, ZERO
    for i in range(len(m)):
        j = U(t[i])
        a = m[i]
        odd |= a & (ZERO - (j & ONE))
        s = ZERO - ((j >> ONE) & ONE)
        acc += (a ^ s) - s
        q = a * a
        qh += q >> S32
        ql += q & LOW32
    return I(acc), odd, qh, ql


@njit
def moments_packed(tp, m):
    acc, odd, qh, ql = ZERO, ZERO, ZERO, ZERO
    n = len(m)
    full = n >> 2
    for j in range(full):
        b = U(tp[j])
        for q in range(4):
            i = 4 * j + q
            t = (b >> U(2 * q)) & U(3)
            a = m[i]
            acc, odd = _sum_step(t, a, acc, odd)
            sq = a * a
            qh += sq >> S32
            ql += sq & LOW32
    for i in range(4 * full, n):
        t = (U(tp[i >> 2]) >> U(2 * (i & 3))) & U(3)
        a = m[i]
        acc, odd = _sum_step(t, a, acc, odd)
        sq = a * a
        qh += sq >> S32
        ql += sq & LOW32
    return I(acc), odd, qh, ql


@njit
def moments_signed(x):
    acc, qh, ql = ZERO, ZERO, ZERO
    for i in range(len(x)):
        acc += U(x[i])
        q = U(x[i]) * U(x[i])
        qh += q >> S32
        ql += q & LOW32
    return I(acc), qh, ql


# ---------------------------------------------------------------- the three layouts as Python objects

class Layout:
    """One real vector of int64 numerators, held in one of the three layouts."""

    def __init__(self, kind, x):
        self.kind, self.n = kind, len(x)
        x = np.ascontiguousarray(x, dtype=np.int64)
        if kind == "signed":
            self.x = x.copy()
        elif kind == "turn bytes":
            self.t, self.m = np.empty(len(x), np.uint8), np.empty(len(x), np.uint64)
            to_turn_bytes(x, self.t, self.m)
        else:
            self.t, self.m = np.zeros((len(x) + 3) // 4, np.uint8), np.empty(len(x), np.uint64)
            to_packed(x, self.t, self.m)

    def nbytes(self):
        if self.kind == "signed":
            return self.x.nbytes
        return self.t.nbytes + self.m.nbytes

    def signed(self):
        """Back to int64 numerators, to check results."""
        if self.kind == "signed":
            return self.x
        if self.kind == "turn bytes":
            t = self.t
        else:
            t = (self.t.repeat(4)[: self.n] >> (2 * (np.arange(self.n) % 4)).astype(np.uint8)) & np.uint8(3)
        mag = self.m.view(np.int64)
        return np.where(t == 2, -mag, mag)

    def sum(self):
        if self.kind == "signed":
            return sum_signed(self.x)
        s, odd = (sum_turn_bytes if self.kind == "turn bytes" else sum_packed)(self.t, self.m)
        assert odd == 0
        return s

    def fma(self, b, c, k):
        out = Layout.__new__(Layout)
        out.kind, out.n = self.kind, self.n
        if self.kind == "signed":
            out.x = np.empty(self.n, np.int64)
            fma_signed(self.x, b.x, c.x, k, out.x)
            return out
        out.m = np.empty(self.n, np.uint64)
        if self.kind == "turn bytes":
            out.t = np.empty(self.n, np.uint8)
            odd = fma_turn_bytes(self.t, self.m, b.t, b.m, c.t, c.m, np.uint64(k), out.t, out.m)
        else:
            out.t = np.empty((self.n + 3) // 4, np.uint8)
            odd = fma_packed(self.t, self.m, b.t, b.m, c.t, c.m, np.uint64(k), out.t, out.m)
        assert odd == 0
        return out

    def group_sum(self, keys, groups):
        if self.kind == "signed":
            return group_signed(keys, self.x, groups)
        s, odd = (group_turn_bytes if self.kind == "turn bytes" else group_packed)(keys, self.t, self.m, groups)
        assert odd == 0
        return s

    def moments(self):
        if self.kind == "signed":
            return moments_signed(self.x)
        s, odd, qh, ql = (moments_turn_bytes if self.kind == "turn bytes" else moments_packed)(self.t, self.m)
        assert odd == 0
        return s, qh, ql


KINDS = ("turn bytes", "packed turns", "signed")
```
