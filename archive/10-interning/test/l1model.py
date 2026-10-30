"""A model of layer 1 in exact Python integers, for the generator's predicted figures (spec §14.1 items 1-2, §17.3
item 2): the whole-number list W, the case of every operation, and L1-opt's trick counters O-1 to O-5, per phase.

It follows the pseudocode of spec §6, §8 and §17.1 step by step on magnitudes and the order of the whole-number
list; it shares no code with the C library or the 08 module. Cases are chosen by comparing magnitudes (spec §7),
which is the same as comparing indices because each whole number is stored once.

A "known one" (O-4) is a value whose magnitude is known to be 1 without a comparison of values: a whole number at
the index of 1, a gcd that came out as 1 (or was skipped by O-3, or is the h = 1 of case num when g = 1), and the
quotients and products that inherit it (lib/DECISIONS.md, D-6).
"""
from collections import defaultdict
from math import gcd

PH = {1: "add", 2: "sub", 3: "mul", 4: "div"}


def bits(x):
    return abs(x).bit_length()


class Model:
    def __init__(self, opt):
        self.opt = opt
        self.W = []
        self.where = {}
        self.R = []                      # (neg, i, j)
        self.idx0 = self.idx1 = None
        self.cnt = defaultdict(int)      # (phase, name) -> count
        self.ph = "entry"

    # ------------------------------------------------------------------------------------------------ counting
    def C(self, name, n=1):
        self.cnt[(self.ph, name)] += n

    def per_phase(self, names):
        out = {}
        for (ph, name), v in self.cnt.items():
            if name in names and v:
                out.setdefault(ph, {})[name] = v
        return out

    # ------------------------------------------------------------------------------------------------ store
    def intern(self, v):
        k = self.where.get(v)
        if k is not None:
            self.C("intern_found")
            return k
        k = len(self.W)
        self.W.append(v)
        self.where[v] = k
        self.C("intern_appended")
        if self.opt:
            if v == 0:
                self.idx0 = k
            elif v == 1:
                self.idx1 = k
        return k

    def store(self, n, e, ki=None, kj=None, o1=None):
        if n == 0:
            e = 1
            self.C("zero_results")
        neg, mag = n < 0, abs(n)
        if self.opt:
            if ki is not None:
                assert self.W[ki] == mag
                self.C("o1_" + o1)
            elif mag == 0 and self.idx0 is not None:
                ki = self.idx0
                self.C("o2_0")
            elif mag == 1 and self.idx1 is not None:
                ki = self.idx1
                self.C("o2_1")
            else:
                ki = self.intern(mag)
            if kj is not None:
                assert self.W[kj] == e
                self.C("o1_" + o1)
            elif e == 1 and self.idx1 is not None:
                kj = self.idx1
                self.C("o2_1")
            else:
                kj = self.intern(e)
        else:
            ki = self.intern(mag)
            kj = self.intern(e)
        self.R.append((neg, ki, kj))
        return len(self.R) - 1

    # ------------------------------------------------------------------------------------------------ known ones
    def whole(self, i):
        return (self.W[i], self.opt and self.idx1 is not None and i == self.idx1)

    def gcdk(self, p, pi, q, qi):
        if self.opt and self.idx1 is not None and (pi == self.idx1 or qi == self.idx1):
            self.C("o3_gcd")
            self.C("o3_work", bits(max(abs(p), abs(q))))
            return (1, True)
        g = gcd(p, q)
        self.C("gcd")
        self.C("gcd_work", bits(max(abs(p), abs(q))))
        return (g, self.opt and g == 1)

    def dvx(self, x, h):
        if self.opt and h[1]:
            self.C("o4_div")
            return x
        self.C("div")
        q, r = divmod(x[0], h[0])
        assert r == 0
        return (q, False)

    def mulx(self, p, q):
        if self.opt and (p[1] or q[1]):
            self.C("o4_mul")
            if p[1]:
                return (-q[0] if p[0] < 0 else q[0], q[1])
            return (-p[0] if q[0] < 0 else p[0], p[1])
        self.C("mul")
        return (p[0] * q[0], False)

    def addv(self, p, q):
        self.C("add")
        return p + q

    # ------------------------------------------------------------------------------------------------ calls
    def enter(self, m, d):
        self.ph = "entry"
        if d < 0:
            m, d = -m, -d
        g = self.gcdk(m, None, d, None)
        n = self.dvx((m, False), g)
        e = self.dvx((d, False), g)
        return self.store(n[0], e[0])

    def operate(self, code, x, y):
        """Returns (handle, case)."""
        self.ph = PH[code]
        sx, ix, jx = self.R[x]
        sy, iy, jy = self.R[y]
        if code == 2:
            sy = not sy
        a, b, c, d = self.whole(ix), self.whole(jx), self.whole(iy), self.whole(jy)
        if code == 4 and c[0] == 0:
            raise ZeroDivisionError
        # the four comparisons of §7, on magnitudes
        e_den, e_num, e_ad, e_cb = b[0] == d[0], a[0] == c[0], a[0] == d[0], c[0] == b[0]
        self.C("idx_cmp", 4)
        if code in (1, 2):
            return self.addsub(sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb)
        if code == 3:
            return self.mul(sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb)
        return self.div(sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb)

    def addsub(self, sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb):
        mx = (-a[0] if sx else a[0], a[1])
        my = (-c[0] if sy else c[0], c[1])
        if e_ad or e_cb:
            p, r = self.mulx(mx, d), self.mulx(my, b)
            t = self.addv(p[0], r[0])
            e = self.mulx(b, d)
            return self.store(t, e[0]), "add_cross"
        if e_den:
            if self.opt and e_num and sx != sy:                      # O-5
                self.C("o5_ops")
                self.C("o5_add")
                self.C("o5_gcd")
                self.C("o5_work", bits(d[0]))
                self.C("o5_div", 2)
                return self.store(0, 1), "add_den"
            t = self.addv(mx[0], my[0])
            h = self.gcdk(t, None, d[0], jy)
            n = self.dvx((t, False), h)
            e = self.dvx(d, h)
            if self.opt and h[0] == 1:                               # O-1
                return self.store(n[0], e[0], None, jx, "add_den"), "add_den"
            return self.store(n[0], e[0]), "add_den"
        if e_num:
            g = self.gcdk(b[0], jx, d[0], jy)
            bp, dp = self.dvx(b, g), self.dvx(d, g)
            t = self.addv(-dp[0] if sx else dp[0], -bp[0] if sy else bp[0])
            if g[0] == 1:
                self.C("gen_g1")
                h = (1, self.opt)
            else:
                h = self.gcdk(t, None, g[0], None)
            tq = self.dvx((t, False), h)
            n = self.mulx(a, tq)
            dq = self.dvx(d, h)
            e = self.mulx(bp, dq)
            return self.store(n[0], e[0]), "add_num"
        g = self.gcdk(b[0], jx, d[0], jy)
        if g[0] == 1:
            self.C("gen_g1")
            p, r = self.mulx(mx, d), self.mulx(my, b)
            t = self.addv(p[0], r[0])
            e = self.mulx(b, d)
            return self.store(t, e[0]), "add_general"
        bp, dp = self.dvx(b, g), self.dvx(d, g)
        p, r = self.mulx(mx, dp), self.mulx(my, bp)
        t = self.addv(p[0], r[0])
        h = self.gcdk(t, None, g[0], None)
        n = self.dvx((t, False), h)
        dq = self.dvx(d, h)
        e = self.mulx(bp, dq)
        return self.store(n[0], e[0]), "add_general"

    def mul(self, sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb):
        neg = sx != sy
        mx = (-a[0] if sx else a[0], a[1])
        my = (-c[0] if sy else c[0], c[1])
        sg = (lambda v: -v) if neg else (lambda v: v)
        if e_ad and e_cb:
            return self.store(-1 if neg else 1, 1), "mul_both"
        if e_den:
            n = self.mulx(mx, my)
            e = self.mulx(d, d)
            return self.store(n[0], e[0]), "mul_den"
        if e_num:
            n = self.mulx(a, a)
            e = self.mulx(b, d)
            return self.store(sg(n[0]), e[0]), "mul_num"
        if e_ad:
            h = self.gcdk(c[0], iy, b[0], jx)
            n, e = self.dvx(c, h), self.dvx(b, h)
            if self.opt and h[0] == 1:
                return self.store(sg(n[0]), e[0], iy, jx, "mul_cross_ad"), "mul_cross_ad"
            return self.store(sg(n[0]), e[0]), "mul_cross_ad"
        if e_cb:
            h = self.gcdk(a[0], ix, d[0], jy)
            n, e = self.dvx(a, h), self.dvx(d, h)
            if self.opt and h[0] == 1:
                return self.store(sg(n[0]), e[0], ix, jy, "mul_cross_cb"), "mul_cross_cb"
            return self.store(sg(n[0]), e[0]), "mul_cross_cb"
        g1 = self.gcdk(mx[0], ix, d[0], jy)
        g2 = self.gcdk(my[0], iy, b[0], jx)
        q1, q2 = self.dvx(mx, g1), self.dvx(my, g2)
        n = self.mulx(q1, q2)
        r1, r2 = self.dvx(b, g2), self.dvx(d, g1)
        e = self.mulx(r1, r2)
        return self.store(n[0], e[0]), "mul_general"

    def div(self, sx, a, b, ix, jx, sy, c, d, iy, jy, e_den, e_num, e_ad, e_cb):
        neg = sx != sy
        sg = (lambda v: -v) if neg else (lambda v: v)
        if e_num and e_den:
            return self.store(-1 if neg else 1, 1), "div_both"
        if e_ad:
            n, e = self.mulx(a, a), self.mulx(b, c)
            return self.store(sg(n[0]), e[0]), "div_cross_ad"
        if e_cb:
            n, e = self.mulx(a, d), self.mulx(b, b)
            return self.store(sg(n[0]), e[0]), "div_cross_cb"
        if e_num:
            h = self.gcdk(d[0], jy, b[0], jx)
            n, e = self.dvx(d, h), self.dvx(b, h)
            if self.opt and h[0] == 1:
                return self.store(sg(n[0]), e[0], jy, jx, "div_num"), "div_num"
            return self.store(sg(n[0]), e[0]), "div_num"
        if e_den:
            h = self.gcdk(a[0], ix, c[0], iy)
            n, e = self.dvx(a, h), self.dvx(c, h)
            if self.opt and h[0] == 1:
                return self.store(sg(n[0]), e[0], ix, iy, "div_den"), "div_den"
            return self.store(sg(n[0]), e[0]), "div_den"
        g1 = self.gcdk(a[0], ix, c[0], iy)
        g2 = self.gcdk(b[0], jx, d[0], jy)
        q1, q2 = self.dvx(a, g1), self.dvx(d, g2)
        n = self.mulx(q1, q2)
        r1, r2 = self.dvx(b, g2), self.dvx(c, g1)
        e = self.mulx(r1, r2)
        return self.store(sg(n[0]), e[0]), "div_general"

    def value(self, h):
        neg, i, j = self.R[h]
        return (-self.W[i] if neg else self.W[i], self.W[j])


# The trick counters the generator predicts (spec §17.3 item 2), named as the C counters (bench.c's xctr lines).
TRICK_NAMES = {
    "o1_add_den": "o1_skipped_add_den", "o1_mul_cross_ad": "o1_skipped_mul_cross_ad",
    "o1_mul_cross_cb": "o1_skipped_mul_cross_cb", "o1_div_num": "o1_skipped_div_num",
    "o1_div_den": "o1_skipped_div_den", "o2_0": "o2_skipped0", "o2_1": "o2_skipped1",
    "o3_gcd": "o3_gcd_skipped", "o3_work": "o3_gcd_work", "o4_div": "o4_div_skipped", "o4_mul": "o4_mul_skipped",
    "o5_ops": "o5_ops", "o5_add": "o5_add_skipped", "o5_gcd": "o5_gcd_skipped", "o5_work": "o5_gcd_work",
    "o5_div": "o5_div_skipped",
}
# L1's own counts the model also gives (used by the harness tests, not by the gates)
BASE_NAMES = {"gcd": "gcd", "gcd_work": "gcd_work", "div": "div_exact", "mul": "mul", "add": "add"}
