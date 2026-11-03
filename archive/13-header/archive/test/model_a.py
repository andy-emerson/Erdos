"""model_a.py: design A (spec 13 §2), a number = (8-bit header, word). Header bits 0-4 class, bit 5 spare, bits 6-7
direction d (1, i, -1, -i). Every operation follows §2 literally, on 8 bits: a zero keeps whatever direction its
operation gives (no step sets it); add branches on t = (h1 xor h2) and D; subtract is add with h2 xor 0x80; negate
copies; results zero the rest of their slot."""
from words import (COUNT, SpecError, Res, part_len, part_copy, part_ptr, part_value, part_canonical, part_release,
                   part_from_int,
                   r_add, r_mac, r_absdiff, r_cmp, r_div_core, r_gcd_core, POISON)

D = 0xC0


class A:
    def __init__(self, pools):
        self.pools = pools
        self.breach = []          # routine bound breaches seen in the current operation

    # -------------------------------------------------------------------- helpers
    def _len(self, h, w):
        return part_len(self.pools, h & 31, w)

    def _ptr(self, h, w):
        return part_ptr(self.pools, h & 31, w)

    def _done(self, res, dst, n, dirbits):
        if dst.breach:
            self.breach.append(dst.breach)
        if res.c:
            dst.zero_rest(n)
        c, w = res.finish(self.pools, n)
        return ((dirbits & 0xFF) | c), w

    # -------------------------------------------------------------------- outside the operations
    def make(self, v, d=None):
        """v an int; a zero takes direction d (0 or 2), default 0"""
        c, w = part_from_int(self.pools, abs(v))
        if v == 0:
            return ((d or 0) << 6), 0
        return (0x80 if v < 0 else 0) | c, w

    def value(self, x):
        h, w = x
        m = part_value(self.pools, h & 31, w)
        d = (h >> 6) & 3
        if m == 0:
            return 0                               # any zero equals any zero
        if d == 0:
            return m
        if d == 2:
            return -m
        raise SpecError(f"direction {d} in an integer")

    def canonical(self, x):
        """the smallest class and spare bit 0; a zero in any direction"""
        h, w = x
        if h > 0xFF:
            return "header wider than 8 bits"
        if h & 0x20:
            return "spare bit set"
        return part_canonical(self.pools, h & 31, w)

    def release(self, x):
        COUNT["value_release"] += 1
        part_release(self.pools, x[0] & 31, x[1])

    # -------------------------------------------------------------------- operations
    def _copy(self, x):
        c, w = part_copy(self.pools, x[0] & 31, x[1])
        return (x[0] & ~31) | c, w

    def neg(self, x):
        """h xor 0x80, zero too; a pool number's words copied into a new slot"""
        h, w = self._copy(x)
        return h ^ 0x80, w

    def add(self, x, y, flip=0):
        """flip = 0x80 is subtract: add with h2 xor 0x80, copying nothing. No zero test but the different-axes one."""
        (h1, w1), (h2, w2) = x, y
        h2 ^= flip
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        t = (h1 ^ h2) & D
        if t in (0x40, 0xC0):                      # different axes
            if n1 == 0:
                h1 = (h1 & ~D) | (h2 & D)          # it takes h2's direction
            elif n2 == 0:
                pass                               # h1's stays
            else:
                raise SpecError("sum across axes (an expression)")
            t = 0
        if t == 0:                                 # same: m1 + m2, direction h1 and D
            res = Res(self.pools, max(n1, n2) + 1)
            a, ao, n1 = self._ptr(h1, w1)
            b, bo, n2 = self._ptr(h2, w2)
            dst = res.dst(self.pools)
            n = r_add(dst, a, ao, n1, b, bo, n2)
            return self._done(res, dst, n, h1 & D)
        res = Res(self.pools, max(n1, n2))         # opposite: |m1 - m2|, direction (h1 and D) + (b shl 7)
        a, ao, n1 = self._ptr(h1, w1)
        b, bo, n2 = self._ptr(h2, w2)
        dst = res.dst(self.pools)
        n, br = r_absdiff(dst, a, ao, n1, b, bo, n2)
        return self._done(res, dst, n, (h1 & D) + (br << 7))

    def sub(self, x, y):
        return self.add(x, y, 0x80)

    def mul(self, x, y):
        (h1, w1), (h2, w2) = x, y
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        res = Res(self.pools, n1 + n2)
        a, ao, n1 = self._ptr(h1, w1)
        b, bo, n2 = self._ptr(h2, w2)
        dst = res.dst(self.pools)
        n = r_mac(dst, 0, a, ao, n1, b, bo, n2)
        return self._done(res, dst, n, (h1 & D) + (h2 & D))

    def _load(self, x, y):
        """Growing's working buffer: the two operands copied in, a then b"""
        a, ao, n1 = self._ptr(*x)
        b, bo, n2 = self._ptr(*y)
        buf = [POISON] * max(n1 + n2, 1)
        for i in range(n1):
            buf[i] = a[ao + i]
        for i in range(n2):
            buf[n1 + i] = b[bo + i]
        return buf, n1, n2

    def div(self, x, y):
        (h1, w1), (h2, w2) = x, y
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        res = Res(self.pools, max(n1 - n2 + 1, 0))
        buf, n1, n2 = self._load(x, y)
        dst = res.dst(self.pools)
        n = r_div_core(dst, buf, 0, n1, n1, n2, n1, n2)
        return self._done(res, dst, n, (h1 & D) - (h2 & D))

    def gcd(self, x, y):
        (h1, w1), (h2, w2) = x, y
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        if n1 == 0 and n2 == 0:
            raise SpecError("gcd(0, 0)")
        res = Res(self.pools, min(n1, n2) if n1 and n2 else max(n1, n2))
        buf, n1, n2 = self._load(x, y)
        dst = res.dst(self.pools)
        n = r_gcd_core(dst, buf, 0, n1, n1, n2)
        return self._done(res, dst, n, 0)

    def cmp(self, x, y):
        """zero first (two zeros equal; one zero, the other's sign decides, 1 and i positive), then opposite
        directions (the positive one larger), then same direction (magnitudes, reversed when negative); different
        axes are not ordered"""
        (h1, w1), (h2, w2) = x, y
        d1, d2 = (h1 >> 6) & 3, (h2 >> 6) & 3
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        if n1 == 0 and n2 == 0:
            return 0
        if n1 == 0:
            return -1 if d2 in (0, 1) else 1
        if n2 == 0:
            return 1 if d1 in (0, 1) else -1
        t = (d1 ^ d2)
        if t == 2:
            return 1 if d1 in (0, 1) else -1
        if t != 0:
            raise SpecError("compare across axes")
        a, ao, n1 = self._ptr(h1, w1)
        b, bo, n2 = self._ptr(h2, w2)
        c = r_cmp(a, ao, n1, b, bo, n2)
        return c if d1 in (0, 1) else -c
