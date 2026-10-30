"""A model of spec v0.2 in exact Python integers, for the generator's predicted figures (spec §14.1 item 2): Q32's
whole-number list W, the case of every operation, every K counter of §13.1 (K-1 to K-15, with the arithmetic each
skipped), Q32's lookups (intern calls found and appended, lookup searches, K-Q1 and K-Q2 skips per case) and reads
(K-Q3), the comparisons, the zero results, and the base arithmetic of both rows, per phase.

It follows the pseudocode of §10 step by step on magnitudes, the tags and the whole-number list, and shares no code
with the C library or the 08 module. The comparisons are made on indices, which is the same as comparing magnitudes
because each whole number is stored once. The two rows make the same arithmetic (§14.1 item 3), so one model gives
both; the Q32-only figures (W, lookups, reads) are Q32's.

The skipped arithmetic of each K counter is measured against the same operation with every known-answer skip off
(lib/DECISIONS.md R2-D6): K-2's is that of case cross for +-1/1 -+ 1/1 and of case den otherwise; K-4 and K-6 skip four
divisions and the gcd gcd(t, 1); K-12 and K-15 count operations only; K-13 counts the signs applied by negation.
"""
from collections import defaultdict
from math import gcd

PH = {1: "add", 2: "sub", 3: "mul", 4: "div"}


def bits(x):
    return abs(x).bit_length()


def sgn(neg, v):
    return -v if neg else v


class Model:
    def __init__(self):
        self.W = []
        self.where = {}
        self.R = []                       # (neg, i, j)
        self.cnt = defaultdict(int)       # (phase, name) -> count
        self.ph = "entry"
        self.S = 16                       # the lookup's slots

    # ------------------------------------------------------------------------------------------------ counting
    def C(self, name, n=1):
        if n:
            self.cnt[(self.ph, name)] += n

    def K(self, k, field, n=1):
        self.C(f"k{k}_{field}", n)

    def g(self, p, q):
        """a gcd, counted as 08 counts it"""
        self.C("gcd")
        self.C("gcd_work", bits(max(abs(p), abs(q))))
        return gcd(p, q)

    def dv(self, p, q):
        self.C("div_exact")
        r, m = divmod(p, q)
        assert m == 0
        return r

    def mul(self, p, q):
        self.C("mul")
        return p * q

    def add(self, p, q):
        self.C("add")
        return p + q

    def sg(self, neg, v):
        """sgn(s, v): a sign applied by negation (K-13)"""
        self.K(13, "mul")
        return sgn(neg, v)

    def per_phase(self):
        out = {}
        for (ph, name), v in self.cnt.items():
            out.setdefault(ph, {})[name] = v
        return out

    # ------------------------------------------------------------------------------------------------ store
    def intern(self, v):
        self.C("lookup_search")                       # the search
        k = self.where.get(v)
        if k is not None:
            self.C("intern_found")
            return k
        k = len(self.W)
        assert k < 32768, "Q32 capacity exceeded"
        self.W.append(v)
        self.where[v] = k
        if 2 * (k + 1) > self.S:                      # growth: every index placed again, one search each
            self.S *= 2
            self.C("lookup_search", k + 1)
        self.C("intern_appended")
        return k

    def store(self, n, e, kase, hi=None, hj=None):
        """n / e with Q32's hints hi, hj (an index, or hj = "same": the numerator's)"""
        self.K(14, "ops")
        self.C("zero_results", n == 0)
        if n == 0:
            assert e == 1, "K-14: a zero result without the denominator 1"
        neg, mag = n < 0, abs(n)
        if hi is None:
            i = self.intern(mag)
        else:
            assert self.W[hi] == mag
            i = hi
            self.C("kq1_" + kase)
        if hj == "same":
            assert mag == 1 and e == 1
            j = i
            self.C("kq2_" + kase)
        elif hj is None:
            j = self.intern(e)
        else:
            assert self.W[hj] == e
            j = hj
            self.C("kq1_" + kase)
        self.R.append((neg, i, j))
        return len(self.R) - 1

    # ------------------------------------------------------------------------------------------------ calls
    def enter(self, m, d):
        self.ph = "entry"
        if d < 0:
            m, d = -m, -d
        g = self.g(m, d)
        if g == 1:                                                    # K-1
            self.K(1, "ops")
            self.K(1, "div", 2)
            return self.store(m, d, "entry")
        return self.store(self.dv(m, g), self.dv(d, g), "entry")

    def operate(self, code, x, y):
        """Returns (handle, case)."""
        self.ph = PH[code]
        sx, ix, jx = self.R[x]
        sy, iy, jy = self.R[y]
        if code == 2:
            sy = not sy
        self.idx = {"a": ix, "b": jx, "c": iy, "d": jy}
        self.got = set()                                              # indices read by this operation (K-Q3)
        if code == 4:
            if self.rd("c") == 0:
                raise ZeroDivisionError
        q = dict(den=jx == jy, num=ix == iy, ad=ix == jy, cb=iy == jx)
        if code >= 3:
            q.update(ox=ix == jx, oy=iy == jy)
            self.C("cmp", 6)
        else:
            self.C("cmp", 4)
        if code in (1, 2):
            h, k = self.addsub(sx, sy, q)
        elif code == 3:
            h, k = self.multiply(sx, sy, q)
        else:
            h, k = self.divide(sx, sy, q)
        self.C("reads_skipped", 4 - self.nread)
        return h, k

    def rd(self, *names):
        """read the whole numbers named (a, b, c, d), each distinct index once; returns their values"""
        out = []
        for nm in names:
            i = self.idx[nm]
            if i not in self.got:
                self.got.add(i)
                self.C("reads")
            out.append(self.W[i])
        return out[0] if len(out) == 1 else out

    @property
    def nread(self):
        return len(self.got)

    def case(self, kase):
        self.C("case_" + kase)
        if kase != "add_zero":
            self.K(13, "ops")
        return kase

    def addsub(self, sx, sy, q):
        if q["den"] and q["num"] and sx != sy:                         # zero [K-2]
            k = self.case("add_zero")
            self.K(2, "ops")
            if q["ad"] or q["cb"]:
                self.K(2, "mul", 3)
                self.K(2, "add", 1)
            else:
                d = self.W[self.idx["d"]]                             # (counting only; not a read)
                self.K(2, "add", 1)
                self.K(2, "gcd", 1)
                self.K(2, "gcd_work", bits(d))
                self.K(2, "div", 2)
            return self.store(0, 1, k), k
        if q["ad"] or q["cb"]:                                        # cross
            k = self.case("add_cross")
            a, b, c, d = self.rd("a", "b", "c", "d")
            p = self.mul(self.sg(sx, a), d)
            r = self.mul(self.sg(sy, c), b)
            t = self.add(p, r)
            return self.store(t, self.mul(b, d), k), k
        if q["den"]:                                                  # den
            k = self.case("add_den")
            a, c, d = self.rd("a", "c", "d")
            t = self.add(self.sg(sx, a), self.sg(sy, c))
            h = self.g(t, d)
            if h == 1:                                                # K-3, K-Q1
                self.K(3, "ops")
                self.K(3, "div", 2)
                return self.store(t, d, k, hj=self.idx["b"]), k
            n = self.dv(t, h)
            return self.store(n, self.dv(d, h), k), k
        if q["num"]:                                                  # num
            k = self.case("add_num")
            a, b, d = self.rd("a", "b", "d")
            g = self.g(b, d)
            if g == 1:                                                # K-4
                t = self.add(self.sg(sx, d), self.sg(sy, b))
                self.K(4, "ops")
                self.K(4, "div", 4)
                self.K(4, "gcd", 1)
                self.K(4, "gcd_work", bits(max(abs(t), 1)))
                n = self.mul(a, t)
                return self.store(n, self.mul(b, d), k), k
            bp, dp = self.dv(b, g), self.dv(d, g)
            t = self.add(self.sg(sx, dp), self.sg(sy, bp))
            h = self.g(t, g)
            if h == 1:                                                # K-5
                self.K(5, "ops")
                self.K(5, "div", 2)
                n = self.mul(a, t)
                return self.store(n, self.mul(bp, d), k), k
            tq, dq = self.dv(t, h), self.dv(d, h)
            n = self.mul(a, tq)
            return self.store(n, self.mul(bp, dq), k), k
        k = self.case("add_general")                                  # general
        a, b, c, d = self.rd("a", "b", "c", "d")
        g = self.g(b, d)
        if g == 1:                                                    # K-6
            p = self.mul(self.sg(sx, a), d)
            r = self.mul(self.sg(sy, c), b)
            t = self.add(p, r)
            e = self.mul(b, d)
            self.K(6, "ops")
            self.K(6, "div", 4)
            self.K(6, "gcd", 1)
            self.K(6, "gcd_work", bits(max(abs(t), 1)))
            return self.store(t, e, k), k
        bp, dp = self.dv(b, g), self.dv(d, g)
        p = self.mul(self.sg(sx, a), dp)
        r = self.mul(self.sg(sy, c), bp)
        t = self.add(p, r)
        h = self.g(t, g)
        if h == 1:                                                    # K-7
            self.K(7, "ops")
            self.K(7, "div", 2)
            return self.store(t, self.mul(bp, d), k), k
        n = self.dv(t, h)
        dq = self.dv(d, h)
        return self.store(n, self.mul(bp, dq), k), k

    def one(self, kase, flip, s, num, den):
        """cases one_x and one_y: the result is the operand whose whole numbers are num, den, with the sign s; its
        numerator is read only when the +-1 operand is -1 (flip)"""
        self.K(15, "ops")
        self.K(13, "mul")
        if flip:
            self.rd(num)
        n = sgn(s, self.W[self.idx[num]])
        return self.store(n, self.W[self.idx[den]], kase, hi=self.idx[num], hj=self.idx[den])

    def multiply(self, sx, sy, q):
        s = sx != sy
        if q["ox"]:
            k = self.case("mul_one_x")
            return self.one(k, sx, s, "c", "d"), k
        if q["oy"]:
            k = self.case("mul_one_y")
            return self.one(k, sy, s, "a", "b"), k
        if q["ad"] and q["cb"]:                                       # both [K-12, K-Q2]
            k = self.case("mul_both")
            self.K(12, "ops")
            return self.store(self.sg(s, 1), 1, k, hj="same"), k
        if q["den"]:
            k = self.case("mul_den")
            a, c, d = self.rd("a", "c", "d")
            n = self.mul(a, c)
            e = self.mul(d, d)
            return self.store(self.sg(s, n), e, k), k
        if q["num"]:
            k = self.case("mul_num")
            a, b, d = self.rd("a", "b", "d")
            n = self.mul(a, a)
            e = self.mul(b, d)
            return self.store(self.sg(s, n), e, k), k
        if q["ad"]:
            k = self.case("mul_cross_ad")
            b, c = self.rd("b", "c")
            h = self.g(c, b)
            if h == 1:                                                # K-8, K-Q1
                self.K(8, "ops")
                self.K(8, "div", 2)
                return self.store(self.sg(s, c), b, k, hi=self.idx["c"], hj=self.idx["b"]), k
            n = self.dv(c, h)
            return self.store(self.sg(s, n), self.dv(b, h), k), k
        if q["cb"]:
            k = self.case("mul_cross_cb")
            a, d = self.rd("a", "d")
            h = self.g(a, d)
            if h == 1:                                                # K-8, K-Q1
                self.K(8, "ops")
                self.K(8, "div", 2)
                return self.store(self.sg(s, a), d, k, hi=self.idx["a"], hj=self.idx["d"]), k
            n = self.dv(a, h)
            return self.store(self.sg(s, n), self.dv(d, h), k), k
        k = self.case("mul_general")
        a, b, c, d = self.rd("a", "b", "c", "d")
        g1, g2 = self.g(a, d), self.g(c, b)
        if g1 == 1:                                                   # K-9
            ap, dp = a, d
            self.K(9, "div", 2)
        else:
            ap, dp = self.dv(a, g1), self.dv(d, g1)
        if g2 == 1:
            cp, bp = c, b
            self.K(9, "div", 2)
        else:
            cp, bp = self.dv(c, g2), self.dv(b, g2)
        self.K(9, "ops", g1 == 1 or g2 == 1)
        n = self.mul(ap, cp)
        e = self.mul(bp, dp)
        return self.store(self.sg(s, n), e, k), k

    def divide(self, sx, sy, q):
        s = sx != sy
        if q["ox"]:                                                   # one_x: s*d/c [K-15, K-Q1]
            k = self.case("div_one_x")
            self.K(15, "ops")
            self.K(13, "mul")
            d, c = self.W[self.idx["d"]], self.W[self.idx["c"]]
            return self.store(sgn(s, d), c, k, hi=self.idx["d"], hj=self.idx["c"]), k
        if q["oy"]:
            k = self.case("div_one_y")
            return self.one(k, sy, s, "a", "b"), k
        if q["num"] and q["den"]:                                     # both [K-12, K-Q2]
            k = self.case("div_both")
            self.K(12, "ops")
            return self.store(self.sg(s, 1), 1, k, hj="same"), k
        if q["ad"]:
            k = self.case("div_cross_ad")
            a, b, c = self.rd("a", "b", "c")
            n = self.mul(a, a)
            e = self.mul(b, c)
            return self.store(self.sg(s, n), e, k), k
        if q["cb"]:
            k = self.case("div_cross_cb")
            a, d, c = self.rd("a", "d", "c")
            n = self.mul(a, d)
            e = self.mul(c, c)
            return self.store(self.sg(s, n), e, k), k
        if q["num"]:
            k = self.case("div_num")
            b, d = self.rd("b", "d")
            h = self.g(d, b)
            if h == 1:                                                # K-10, K-Q1
                self.K(10, "ops")
                self.K(10, "div", 2)
                return self.store(self.sg(s, d), b, k, hi=self.idx["d"], hj=self.idx["b"]), k
            n = self.dv(d, h)
            return self.store(self.sg(s, n), self.dv(b, h), k), k
        if q["den"]:
            k = self.case("div_den")
            a, c = self.rd("a", "c")
            h = self.g(a, c)
            if h == 1:                                                # K-10, K-Q1
                self.K(10, "ops")
                self.K(10, "div", 2)
                return self.store(self.sg(s, a), c, k, hi=self.idx["a"], hj=self.idx["c"]), k
            n = self.dv(a, h)
            return self.store(self.sg(s, n), self.dv(c, h), k), k
        k = self.case("div_general")
        a, b, c, d = self.rd("a", "b", "c", "d")
        g1, g2 = self.g(a, c), self.g(b, d)
        if g1 == 1:                                                   # K-11
            ap, cp = a, c
            self.K(11, "div", 2)
        else:
            ap, cp = self.dv(a, g1), self.dv(c, g1)
        if g2 == 1:
            bp, dp = b, d
            self.K(11, "div", 2)
        else:
            bp, dp = self.dv(b, g2), self.dv(d, g2)
        self.K(11, "ops", g1 == 1 or g2 == 1)
        n = self.mul(ap, dp)
        e = self.mul(bp, cp)
        return self.store(self.sg(s, n), e, k), k

    def value(self, h):
        neg, i, j = self.R[h]
        return (-self.W[i] if neg else self.W[i], self.W[j])


def predict(ops, a, b):
    """Run the model over the calls of a workload (op codes, operands); returns (values, cases, W after the last
    entry, W at the end, counters per phase, the whole-number list). Cases are indices into r2_common.CASES (-1 for an
    entry)."""
    import r2_common as C
    M = Model()
    cases, values = [], []
    last_entry = max(i for i, o in enumerate(ops) if o == 0)
    W_after_entry = None
    for i, o in enumerate(ops):
        if o == 0:
            M.enter(a[i], b[i])
            cases.append(-1)
        else:
            _, k = M.operate(o, a[i], b[i])
            cases.append(C.CASE[k])
        values.append(M.value(i))
        if i == last_entry:
            W_after_entry = len(M.W)
    return values, cases, W_after_entry, len(M.W), M.per_phase(), list(M.W)
