"""A model of spec v0.3 in exact Python integers, for the generator's predicted figures (spec v0.3 §12.1 item 2).

It is round 2's model (r2_model.py, unchanged and imported: the cases, every K counter, the reads, the comparisons, the
zero results and the base arithmetic, which v0.3 keeps for all three rows, call for call) with the whole-number store
replaced by the two stores of round 3, simulated exactly and side by side, since both Q32 rows call intern on the
same values in the same order (spec v0.3 §10 item 3):
  - Q32-index (§7): a list in arrival order searched top to bottom. A value at index k costs k + 1 entries examined;
    a new value costs W (the whole list), then is appended. Each distinct whole number is stored once, so the search's
    result is the value's position, kept here in a dict (the model's own bookkeeping, not the row's structure).
  - Q32-hash (§8): a table of 32,768 slots, -1 empty; the home slot h(v) is the top 15 bits of v * phi mod 2^64, and
    linear probing goes to the next slot, wrapping. Probes: the slots read by one store, the home slot included.
The model's own handles are the list's indices; a comparison of handles is a comparison of values in either row,
since each whole number is stored once in each (spec v0.3 §3, `same`).

Round 2's lookup counter (lookup_search) is not counted: no round-3 row has a lookup.

Amendment of Oct 29, 2026 (spec v0.3 §17): both stores hold 0 and 1 from creation, at handles 0 and 1 (W counts
them); Q32-hash finds 0 and 1 by its first test, with no probe, and its probing passes over slots 0 and 1. The
operations take the integer cases of addition and multiplication and the zero-operand cases of every operation
(§17.3), in §17.3's order, with the two integer tests and the two zero tests among the comparisons. Division has no
integer case. Round 2's
model is imported for its counting, store and reads; addsub, multiply and divide are this module's, written out
whole from round 2's with the new cases added.
"""
import r2_model
import r3_common as C
from r2_model import bits, sgn

EMPTY = -1


class Model(r2_model.Model):
    def __init__(self):
        super().__init__()
        self.T = [EMPTY] * C.SLOTS            # Q32-hash's table
        self.slot = {}                        # value -> its slot (for the model's checks only)
        self.tot = {"found": 0, "new": 0, "examined": 0, "probes": 0, "longest": 0}
        for v in (0, 1):                      # stored by create, at handles 0 and 1 (§17.2); no counter moves
            self.W.append(v)
            self.where[v] = v
            self.T[v] = v
            self.slot[v] = v

    def longest(self, n):
        key = (self.ph, "longest")
        if n > self.cnt.get(key, 0):
            self.cnt[key] = n
        self.tot["longest"] = max(self.tot["longest"], n)

    def intern(self, v):
        k = self.where.get(v)
        # Q32-index: the entries examined by the search
        ex = k + 1 if k is not None else len(self.W)
        self.C("examined", ex)
        self.tot["examined"] += ex
        # Q32-hash: 0 and 1 found at slots 0 and 1 with no probe; any other value, the probe sequence from its home
        if v in (0, 1):
            s, n = v, 0
        else:
            s, n = C.home(v), 1
            while self.T[s] != v and self.T[s] != EMPTY:
                s = (s + 1) % C.SLOTS
                n += 1
                assert n <= C.SLOTS, "Q32-hash: the table is full"
            assert s > 1, "Q32-hash: a value placed in slot 0 or 1"
        self.C("probes", n)
        self.tot["probes"] += n
        self.longest(n)
        if k is not None:
            assert self.T[s] == v and self.slot[v] == s, "the two stores disagree: found in the list, not the table"
            self.C("intern_found")
            self.tot["found"] += 1
            return k
        assert self.T[s] == EMPTY, "the two stores disagree: new to the list, found in the table"
        k = len(self.W)
        assert k < C.Q32_CAPACITY, "Q32 capacity exceeded"
        self.W.append(v)
        self.where[v] = k
        self.T[s] = v
        self.slot[v] = s
        self.C("intern_appended")
        self.tot["new"] += 1
        return k


    # ------------------------------------------------------------------------------------------------ calls (§17.3)
    def operate(self, code, x, y):
        """Returns (handle, case). As round 2's, with the integer tests (b = 1, d = 1: handle 1) and the zero tests
        (a = 0, c = 0: handle 0) among the comparisons."""
        self.ph = r2_model.PH[code]
        sx, ix, jx = self.R[x]
        sy, iy, jy = self.R[y]
        if code == 2:
            sy = not sy
        self.idx = {"a": ix, "b": jx, "c": iy, "d": jy}
        self.got = set()
        if code == 4:
            self.rd("c")                                              # c read before the comparisons
            if iy == 0:                                               # the zero test, by handle
                raise ZeroDivisionError
        q = dict(den=jx == jy, num=ix == iy, ad=ix == jy, cb=iy == jx, ix=jx == 1, iy=jy == 1, zx=ix == 0, zy=iy == 0)
        if code >= 3:
            q.update(ox=ix == jx, oy=iy == jy)
            self.C("cmp", 10)
        else:
            self.C("cmp", 8)
        if code in (1, 2):
            h, k = self.addsub(sx, sy, q)
        elif code == 3:
            h, k = self.multiply(sx, sy, q)
        else:
            h, k = self.divide(sx, sy, q)
        self.C("reads_skipped", 4 - self.nread)
        return h, k

    def case(self, kase):
        self.C("case_" + kase)
        if kase not in ("add_zero", "mul_zero", "div_zero"):
            self.K(13, "ops")
        return kase

    def addsub(self, sx, sy, q):
        if q["den"] and q["num"] and sx != sy:                         # zero [K-2]
            k = self.case("add_zero")
            self.K(2, "ops")
            if q["zx"]:                                               # 0 - 0 without K-2: zero_x, nothing
                pass
            elif q["ix"]:                                             # without K-2: int_both, one addition
                self.K(2, "add", 1)
            else:                                                     # without K-2: den
                d = self.W[self.idx["d"]]
                self.K(2, "add", 1)
                self.K(2, "gcd", 1)
                self.K(2, "gcd_work", bits(d))
                self.K(2, "div", 2)
            return self.store(0, 1, k), k
        if q["zx"]:                                                   # zero_x: s_y * y, no arithmetic, y's handles
            k = self.case("add_zero_x")
            return self.other(k, sy and self.idx["c"] != 0, "c", "d"), k
        if q["zy"]:                                                   # zero_y: x
            k = self.case("add_zero_y")
            return self.other(k, sx and self.idx["a"] != 0, "a", "b"), k
        if q["ix"] and q["iy"]:                                       # int_both
            k = self.case("add_int_both")
            a, c = self.rd("a", "c")
            t = self.add(self.sg(sx, a), self.sg(sy, c))
            return self.store(t, 1, k, hj=self.idx["b"]), k
        if q["ix"]:                                                   # int_x
            k = self.case("add_int_x")
            a, c, d = self.rd("a", "c", "d")
            p = self.mul(self.sg(sx, a), d)
            t = self.add(p, self.sg(sy, c))
            return self.store(t, d, k, hj=self.idx["d"]), k
        if q["iy"]:                                                   # int_y
            k = self.case("add_int_y")
            a, b, c = self.rd("a", "b", "c")
            r = self.mul(self.sg(sy, c), b)
            t = self.add(self.sg(sx, a), r)
            return self.store(t, b, k, hj=self.idx["b"]), k
        return super().addsub(sx, sy, q)                              # cross, den, num, general: round 2's

    def multiply(self, sx, sy, q):
        s = sx != sy
        if q["zx"] or q["zy"]:                                        # zero
            k = self.case("mul_zero")
            return self.store(0, 1, k), k
        if q["ox"]:
            k = self.case("mul_one_x")
            return self.one(k, sx, s, "c", "d"), k
        if q["oy"]:
            k = self.case("mul_one_y")
            return self.one(k, sy, s, "a", "b"), k
        if q["ix"] and q["iy"]:                                       # int_both
            k = self.case("mul_int_both")
            a, c = self.rd("a", "c")
            n = self.mul(a, c)
            return self.store(self.sg(s, n), 1, k, hj=self.idx["b"]), k
        if q["ad"] and q["cb"]:                                       # both [K-12, K-Q2]
            k = self.case("mul_both")
            self.K(12, "ops")
            return self.store(self.sg(s, 1), 1, k, hj="same"), k
        if not (q["den"] or q["num"] or q["ad"] or q["cb"]):
            if q["ix"]:                                               # int_x: gcd(a, d)
                k = self.case("mul_int_x")
                a, c, d = self.rd("a", "c", "d")
                h = self.g(a, d)
                if h == 1:                                            # K-8, K-Q1
                    self.K(8, "ops")
                    self.K(8, "div", 2)
                    n = self.mul(a, c)
                    return self.store(self.sg(s, n), d, k, hj=self.idx["d"]), k
                ap = self.dv(a, h)
                e = self.dv(d, h)
                n = self.mul(ap, c)
                return self.store(self.sg(s, n), e, k), k
            if q["iy"]:                                               # int_y: gcd(c, b)
                k = self.case("mul_int_y")
                a, b, c = self.rd("a", "b", "c")
                h = self.g(c, b)
                if h == 1:                                            # K-8, K-Q1
                    self.K(8, "ops")
                    self.K(8, "div", 2)
                    n = self.mul(a, c)
                    return self.store(self.sg(s, n), b, k, hj=self.idx["b"]), k
                cp = self.dv(c, h)
                e = self.dv(b, h)
                n = self.mul(a, cp)
                return self.store(self.sg(s, n), e, k), k
        q = dict(q, ox=False, oy=False)                               # den, num, cross_ad, cross_cb, general: round 2's
        return super().multiply(sx, sy, q)

    def divide(self, sx, sy, q):
        s = sx != sy
        if q["zx"]:                                                   # zero: a zero dividend
            k = self.case("div_zero")
            return self.store(0, 1, k), k
        return super().divide(sx, sy, q)                              # one_x .. general: round 2's (no integer case)

    def other(self, kase, neg, num, den):
        """addition's zero_x and zero_y: the other operand with the tag neg, its handles as hints; no read (the hints
        and K-13 on), one sign applied (K-13)"""
        self.K(13, "mul")
        i, j = self.idx[num], self.idx[den]
        n = sgn(neg, self.W[i])
        return self.store(n, self.W[j], kase, hi=i, hj=j)


def predict(ops, a, b):
    """Run the model over the calls of a workload (op codes, operands). Returns a dict: the values, the cases (indices
    into r3_common.CASES, -1 for an entry), W after the last entry and at the end, the counters per phase, Q32-index's
    list in index order, Q32-hash's table slot by slot, and the storage totals after the last entry and at the end."""
    M = Model()
    cases, values = [], []
    last_entry = max(i for i, o in enumerate(ops) if o == 0)
    W_after_entry, stor_after_entry = None, None
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
            stor_after_entry = dict(M.tot)
    return {"values": values, "cases": cases, "W_after_entry": W_after_entry, "W_end": len(M.W),
            "counters": M.per_phase(), "list": list(M.W), "table": list(M.T),
            "stor_after_entry": stor_after_entry, "stor_end": dict(M.tot)}
