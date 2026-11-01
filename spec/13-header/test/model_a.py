"""model_a.py: design A (spec 13 §2), a number = (8-bit header, word). Header bits 0-4 class, bit 5 spare, bits 6-7
direction d (1, i, -1, -i). Every operation follows §2's header arithmetic literally, on 8 bits (revised spec:
zero has no axis, negate copies, subtract flips h2's direction, results zero the rest of their slot)."""
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
        if n == 0:
            return 0, 0                            # zero is header 0
        return ((dirbits & 0xFF) | c), w

    # -------------------------------------------------------------------- outside the operations
    def make(self, v):
        c, w = part_from_int(self.pools, abs(v))
        if v == 0:
            return 0, 0
        return (0x80 if v < 0 else 0) | c, w

    def value(self, x):
        h, w = x
        m = part_value(self.pools, h & 31, w)
        d = (h >> 6) & 3
        if d == 0:
            return m
        if d == 2:
            return -m
        raise SpecError(f"direction {d} in an integer")

    def canonical(self, x):
        h, w = x
        if h > 0xFF:
            return "header wider than 8 bits"
        if h & 0x20:
            return "spare bit set"
        e = part_canonical(self.pools, h & 31, w)
        if e:
            return e
        if self._len(h, w) == 0 and (h, w) != (0, 0):
            return f"zero not header 0, word 0 (header {h:#x})"
        return None

    def release(self, x):
        COUNT["value_release"] += 1
        part_release(self.pools, x[0] & 31, x[1])

    # -------------------------------------------------------------------- operations
    @staticmethod
    def _zero(x):
        return x == (0, 0)

    def _copy(self, x):
        c, w = part_copy(self.pools, x[0] & 31, x[1])
        return (x[0] & ~31) | c, w

    def neg(self, x):
        if self._zero(x):
            return 0, 0                            # negating zero gives zero
        h, w = self._copy(x)
        return h ^ 0x80, w

    def add(self, x, y, flip=0):
        """flip = 0x80 is subtract: the add rules with h2 xor 0x80 in place of h2, copying nothing"""
        if self._zero(y):                          # zero has no axis: a copy of the other
            return self._copy(x)
        if self._zero(x):                          # (a subtract, a negated copy of the second)
            return self.neg(y) if flip else self._copy(y)
        (h1, w1), (h2, w2) = x, y
        h2 ^= flip
        d1, d2 = h1 & D, h2 & D
        n1, n2 = self._len(h1, w1), self._len(h2, w2)
        if d1 == d2:
            res = Res(self.pools, max(n1, n2) + 1)
            a, ao, n1 = self._ptr(h1, w1)
            b, bo, n2 = self._ptr(h2, w2)
            dst = res.dst(self.pools)
            n = r_add(dst, a, ao, n1, b, bo, n2)
            return self._done(res, dst, n, d1)
        if d1 ^ d2 == 0x80:
            res = Res(self.pools, max(n1, n2))
            a, ao, n1 = self._ptr(h1, w1)
            b, bo, n2 = self._ptr(h2, w2)
            dst = res.dst(self.pools)
            n, br = r_absdiff(dst, a, ao, n1, b, bo, n2)
            return self._done(res, dst, n, d1 + (br << 7))
        raise SpecError("sum across axes")

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
        (h1, w1), (h2, w2) = x, y
        d1, d2 = (h1 >> 6) & 3, (h2 >> 6) & 3
        if d1 != d2:
            return 1 if d1 == 0 else -1
        a, ao, n1 = self._ptr(h1, w1)
        b, bo, n2 = self._ptr(h2, w2)
        c = r_cmp(a, ao, n1, b, bo, n2)
        return c if d1 == 0 else -c
