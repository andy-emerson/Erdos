"""model_b.py: design B (spec 13 §3), a number = (16-bit header, P's word, N's word), value P - N. Header: P's class in
bits 0-4, N's in bits 8-12, bits 5-7 and 13-15 spare. Collapse is the only subtraction. Revised spec: negate copies, subtract adds crossed parts,
results zero the rest of their slot, gcd's bound for a zero operand, gcd(0, 0) an error."""
from words import (COUNT, SpecError, Res, part_copy, part_len, part_ptr, part_value, part_canonical, part_release,
                   part_from_int, Dst, r_add, r_mac, r_absdiff, r_cmp, r_div_core, r_gcd_core, POISON)


class B:
    def __init__(self, pools):
        self.pools = pools
        self.breach = []

    # -------------------------------------------------------------------- helpers
    @staticmethod
    def parts(x):
        h, wp, wn = x
        return (h & 31, wp), ((h >> 8) & 31, wn)

    @staticmethod
    def join(p, n):
        return p[0] | (n[0] << 8), p[1], n[1]

    def _len(self, part):
        return part_len(self.pools, *part)

    def _ptr(self, part):
        return part_ptr(self.pools, *part)

    def _fin(self, res, dst, n):
        if dst.breach:
            self.breach.append(dst.breach)
        if res.c:
            dst.zero_rest(n)
        return res.finish(self.pools, n)

    def _fin2(self, rp, dp, lp, rn, dn, ln):
        """both parts' rests zeroed before either moves down: a move-down can grow a pool and move the other's slot"""
        for res, dst, n in ((rp, dp, lp), (rn, dn, ln)):
            if dst.breach:
                self.breach.append(dst.breach)
            if res.c:
                dst.zero_rest(n)
        return self.join(rp.finish(self.pools, lp), rn.finish(self.pools, ln))

    # -------------------------------------------------------------------- outside the operations
    def make(self, p, n=0):
        return self.join(part_from_int(self.pools, p), part_from_int(self.pools, n))

    def make_int(self, v):
        return self.make(v, 0) if v >= 0 else self.make(0, -v)

    def value(self, x):
        p, n = self.parts(x)
        return part_value(self.pools, *p) - part_value(self.pools, *n)

    def canonical(self, x, collapsed=False):
        h = x[0]
        if h > 0xFFFF:
            return "header wider than 16 bits"
        if h & 0xE0E0:
            return "spare bit set"
        p, n = self.parts(x)
        for name, part in (("P", p), ("N", n)):
            e = part_canonical(self.pools, *part)
            if e:
                return f"{name}: {e}"
        if collapsed and p != (0, 0) and n != (0, 0):
            return "collapsed result with both parts nonzero"
        return None

    def release(self, x):
        COUNT["value_release"] += 1
        p, n = self.parts(x)
        part_release(self.pools, *p)
        part_release(self.pools, *n)

    # -------------------------------------------------------------------- operations
    def neg(self, x):
        """swap P and N, each part copied (§1: no two numbers share a slot)"""
        p, n = self.parts(x)
        return self.join(part_copy(self.pools, *n), part_copy(self.pools, *p))

    def _add(self, p1, p2, n1, n2):
        """(p1 + p2, n1 + n2), both result slots allocated first"""
        rp = Res(self.pools, max(self._len(p1), self._len(p2)) + 1)
        rn = Res(self.pools, max(self._len(n1), self._len(n2)) + 1)
        dp, dn = rp.dst(self.pools), rn.dst(self.pools)
        a, ao, la = self._ptr(p1)
        b, bo, lb = self._ptr(p2)
        lp = r_add(dp, a, ao, la, b, bo, lb)
        a, ao, la = self._ptr(n1)
        b, bo, lb = self._ptr(n2)
        ln = r_add(dn, a, ao, la, b, bo, lb)
        return self._fin2(rp, dp, lp, rn, dn, ln)

    def add(self, x, y):
        (p1, n1), (p2, n2) = self.parts(x), self.parts(y)
        return self._add(p1, p2, n1, n2)

    def sub(self, x, y):
        """the crossed parts, P1 + N2 and N1 + P2, copying nothing"""
        (p1, n1), (p2, n2) = self.parts(x), self.parts(y)
        return self._add(p1, n2, n1, p2)

    def _mac2(self, dst, a, b, c, d):
        """dst = a*b + c*d by two multiply-accumulates"""
        x, xo, nx = self._ptr(a)
        y, yo, ny = self._ptr(b)
        n = r_mac(dst, 0, x, xo, nx, y, yo, ny)
        x, xo, nx = self._ptr(c)
        y, yo, ny = self._ptr(d)
        return r_mac(dst, n, x, xo, nx, y, yo, ny)

    def mul(self, x, y):
        (p1, n1), (p2, n2) = self.parts(x), self.parts(y)
        L = {k: self._len(v) for k, v in (("p1", p1), ("n1", n1), ("p2", p2), ("n2", n2))}
        rp = Res(self.pools, max(L["p1"] + L["p2"], L["n1"] + L["n2"]) + 1)
        rn = Res(self.pools, max(L["p1"] + L["n2"], L["n1"] + L["p2"]) + 1)
        dp, dn = rp.dst(self.pools), rn.dst(self.pools)
        lp = self._mac2(dp, p1, p2, n1, n2)
        ln = self._mac2(dn, p1, n2, n1, p2)
        return self._fin2(rp, dp, lp, rn, dn, ln)

    def collapse(self, x):
        """|P - N| with borrow b: to P when b = 0, else to N; the other part 0"""
        p, n = self.parts(x)
        res = Res(self.pools, max(self._len(p), self._len(n)))
        dst = res.dst(self.pools)
        a, ao, la = self._ptr(p)
        b, bo, lb = self._ptr(n)
        m, br = r_absdiff(dst, a, ao, la, b, bo, lb)
        r = self._fin(res, dst, m)
        return self.join(r, (0, 0)) if br == 0 else self.join((0, 0), r)

    def cmp(self, x, y):
        """P1 + N2 against P2 + N1, in one local (two halves)"""
        (p1, n1), (p2, n2) = self.parts(x), self.parts(y)
        s1 = max(self._len(p1), self._len(n2)) + 1
        s2 = max(self._len(p2), self._len(n1)) + 1
        loc = [POISON] * (s1 + s2)
        a, ao, la = self._ptr(p1)
        b, bo, lb = self._ptr(n2)
        d1 = Dst(loc, 0, s1, s1)
        l1 = r_add(d1, a, ao, la, b, bo, lb)
        a, ao, la = self._ptr(p2)
        b, bo, lb = self._ptr(n1)
        d2 = Dst(loc, s1, s2, s2)
        l2 = r_add(d2, a, ao, la, b, bo, lb)
        return r_cmp(loc, 0, l1, loc, s1, l2)

    def _load(self, x, y):
        """the working buffer, each operand's |P - N| written straight in; returns buf, n1, b1, offset of y, n2, b2"""
        (p1, n1), (p2, n2) = self.parts(x), self.parts(y)
        s1 = max(self._len(p1), self._len(n1))
        s2 = max(self._len(p2), self._len(n2))
        buf = [POISON] * max(s1 + s2, 1)
        out = []
        for (p, n), off, s in (((p1, n1), 0, s1), ((p2, n2), s1, s2)):
            a, ao, la = self._ptr(p)
            b, bo, lb = self._ptr(n)
            d = Dst(buf, off, s, s)
            m, br = r_absdiff(d, a, ao, la, b, bo, lb)
            if d.breach:
                self.breach.append(d.breach)
            out.append((m, br))
        return buf, out[0], s1, out[1]

    def div(self, x, y):
        buf, (m1, b1), yo, (m2, b2) = self._load(x, y)
        res = Res(self.pools, max(m1 - m2 + 1, 0))
        dst = res.dst(self.pools)
        n = r_div_core(dst, buf, 0, m1, yo, m2, m1, m2)
        q = self._fin(res, dst, n)
        d = ((b1 << 7) - (b2 << 7)) & 0xC0           # A's rule: (h1 and D) - (h2 and D)
        return self.join(q, (0, 0)) if d == 0 else self.join((0, 0), q)

    def gcd(self, x, y):
        buf, (m1, _), yo, (m2, _) = self._load(x, y)
        if m1 == 0 and m2 == 0:
            raise SpecError("gcd(0, 0)")
        res = Res(self.pools, min(m1, m2) if m1 and m2 else max(m1, m2))
        dst = res.dst(self.pools)
        n = r_gcd_core(dst, buf, 0, m1, yo, m2)
        return self.join(self._fin(res, dst, n), (0, 0))
