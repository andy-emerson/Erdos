"""The exact model of spec v0.1 (11-matrix), in Python integers, for the generator's predicted figures (spec §10 item 2;
rule-m1, Run plan step 0). It shares no code with the C library or the 08 module.

Two parts, since the arithmetic does not depend on how whole numbers are stored (spec §10 items 3-6):
  - Arith: every operation of a job on values: tc3.c's cases in v0.3 §17.3's order, every counter of the counting build
    (gcds and gcd work as 08 counts them, exact divisions, multiplications, additions, the comparisons, every
    known-answer counter K-1 to K-15, the zero results, the cases), per phase; and per row kind the reads: V's cross
    reads (a, b, c, d, each distinct handle of one matrix once, §7.1) and M's K-Q3 reads (the whole numbers each case
    needs, each distinct handle of one matrix once, §6.1 item 2). A comparison of two handles of one matrix is a
    comparison of values, since each whole number is stored once in its matrix's table; a cross test by V or by M is
    true exactly when the values are equal. Every intermediate is checked against 64 bits (an overflow, or -2^63, is an
    error, as the 08 module).
  - Store: one matrix's table and cells, in the order of entry (spec §5.7): fixed (T_fix slots) or grow (from 16,
    doubling when W + 1 > T/2, every whole number reinserted in slot order, the handle fields written so far rewritten
    in one pass; the lookup method's probes counted, the map method's map sized), C's width from 8 bits with every
    widening; per store the found/new split, probes and the longest store; then tau (M rows) from B's final table and
    A's.
The model's Store keeps the table as a list and the handle fields as a list, exactly as the row does (slot = handle).
"""
from collections import defaultdict
from math import gcd

import m1_common as C

LIM = (1 << 63) - 1
EMPTY = -1

# the whole numbers each case needs (bit 0 a, 1 b, 2 c, 3 d), as mh.c's rdset masks (= the values tc3.c's body uses)
NEED_ADD = {"add_zero": 0, "add_zero_x": 0xC, "add_zero_y": 0x3, "add_int_both": 0x5, "add_int_x": 0xD,
            "add_int_y": 0x7, "add_cross": 0xF, "add_den": 0xD, "add_num": 0xB, "add_general": 0xF}
NEED_MUL = {"mul_zero": 0, "mul_one_x": 0xC, "mul_one_y": 0x3, "mul_int_both": 0x5, "mul_both": 0, "mul_den": 0xD,
            "mul_num": 0xB, "mul_cross_ad": 0x6, "mul_cross_cb": 0x9, "mul_int_x": 0xD, "mul_int_y": 0x7,
            "mul_general": 0xF}


class Overflow(Exception):
    pass


def bits(x):
    return abs(x).bit_length()


def distinct_reads(mask, a, b, c, d):
    """slots read for the positions in mask, each distinct handle of one matrix once (a with b, c with d)"""
    n = 0
    for lo, hi, x, y in ((0, 1, a, b), (2, 3, c, d)):
        need_lo, need_hi = (mask >> lo) & 1, (mask >> hi) & 1
        n += 1 if need_lo and need_hi and x == y else need_lo + need_hi
    return n


class Arith:
    def __init__(self):
        self.cnt = defaultdict(int)       # (phase, name) -> count, every row's arithmetic counters
        self.reads_M = defaultdict(int)   # phase -> K-Q3 reads (M rows; V and TC make none)
        self.cross = 0
        self.cross_match = 0
        self.cross_reads_V = 0
        self.ph = "entry"
        self.ncase = defaultdict(int)

    # ------------------------------------------------------------------------------------------------ counting
    def C(self, name, n=1):
        if n:
            self.cnt[(self.ph, name)] += n

    def K(self, k, field, n=1):
        if n:
            self.cnt[(self.ph, f"k{k}_{field}")] += n

    def g(self, p, q):
        self.C("gcd")
        self.C("gcd_work", bits(max(abs(p), abs(q))))
        return gcd(p, q)

    def dv(self, p, q):
        self.C("div_exact")
        r, m = divmod(p, q)
        assert m == 0
        return r

    def chk(self, r, step):
        if r > LIM or r < -LIM:
            raise Overflow(step)
        return r

    def mul(self, p, q, step="mul"):
        self.C("mul")
        return self.chk(p * q, step)

    def add(self, p, q, step="add"):
        self.C("add")
        return self.chk(p + q, step)

    def sg(self, neg, v):
        """sgn(s, v): a sign applied by negation (K-13)"""
        self.K(13, "mul")
        return -v if neg else v

    def case(self, kase):
        self.C("case_" + kase)
        self.ncase[kase] += 1
        if kase not in ("add_zero", "mul_zero"):
            self.K(13, "ops")
        return kase

    def store(self, n, e):
        """the arithmetic's store (K-14): every result, to a cell or a local pair"""
        self.K(14, "ops")
        self.C("zero_results", n == 0)
        assert not (n == 0 and e != 1)
        return (n, e)

    # ------------------------------------------------------------------------------------------------ entry
    def enter(self, m, d):
        self.ph = "entry"
        if d < 0:
            m, d = -m, -d
        g = self.g(m, d)
        if g == 1:                                                    # K-1
            self.K(1, "ops")
            self.K(1, "div", 2)
            return self.store(m, d)
        return self.store(self.dv(m, g), self.dv(d, g))

    # ------------------------------------------------------------------------------------------------ reads
    def reads(self, kase, need, a, b, c, d, cells):
        """the reads of one operation on two cells (cells=True): V's cross reads; M's K-Q3 reads for the case"""
        if cells:
            self.reads_M[self.ph] += distinct_reads(need[kase], a, b, c, d)

    # ------------------------------------------------------------------------------------------------ addition
    def addition(self, x, y, cells):
        """x + y on pairs (m, d); cells: x a cell of A and y one of B (cross tests, reads); else two local pairs"""
        self.ph = "add"
        a, b, xneg = abs(x[0]), x[1], x[0] < 0
        c, d, yneg = abs(y[0]), y[1], y[0] < 0
        q_den, q_num, q_ad, q_cb = b == d, a == c, a == d, c == b
        q_ix, q_iy, q_zx, q_zy = b == 1, d == 1, a == 0, c == 0
        self.C("cmp", 8)
        if cells:
            self.cross += 4
            self.cross_match += q_den + q_num + q_ad + q_cb
            self.cross_reads_V += (1 if a == b else 2) + (1 if c == d else 2)
        if q_den and q_num and xneg != yneg:                          # zero [K-2]
            k = self.case("add_zero")
            self.K(2, "ops")
            if q_zx:
                pass
            elif q_ix:
                self.K(2, "add", 1)
            else:
                self.K(2, "add", 1)
                self.K(2, "gcd", 1)
                self.K(2, "gcd_work", bits(d))
                self.K(2, "div", 2)
            r = self.store(0, 1)
        elif q_zx:                                                    # zero_x
            k = self.case("add_zero_x")
            r = self.store(self.sg(yneg, c), d)
        elif q_zy:                                                    # zero_y
            k = self.case("add_zero_y")
            r = self.store(self.sg(xneg, a), b)
        elif q_ix and q_iy:                                           # int_both
            k = self.case("add_int_both")
            t = self.add(self.sg(xneg, a), self.sg(yneg, c))
            r = self.store(t, 1)
        elif q_ix:                                                    # int_x
            k = self.case("add_int_x")
            p = self.mul(self.sg(xneg, a), d)
            t = self.add(p, self.sg(yneg, c))
            r = self.store(t, d)
        elif q_iy:                                                    # int_y
            k = self.case("add_int_y")
            rr = self.mul(self.sg(yneg, c), b)
            t = self.add(self.sg(xneg, a), rr)
            r = self.store(t, b)
        elif q_ad or q_cb:                                            # cross
            k = self.case("add_cross")
            p = self.mul(self.sg(xneg, a), d)
            rr = self.mul(self.sg(yneg, c), b)
            t = self.add(p, rr)
            e = self.mul(b, d)
            r = self.store(t, e)
        elif q_den:                                                   # den
            k = self.case("add_den")
            t = self.add(self.sg(xneg, a), self.sg(yneg, c))
            h = self.g(t, d)
            if h == 1:                                                # K-3
                self.K(3, "ops")
                self.K(3, "div", 2)
                r = self.store(t, d)
            else:
                n = self.dv(t, h)
                r = self.store(n, self.dv(d, h))
        elif q_num:                                                   # num
            k = self.case("add_num")
            g = self.g(b, d)
            if g == 1:                                                # K-4
                t = self.add(self.sg(xneg, d), self.sg(yneg, b))
                self.K(4, "ops")
                self.K(4, "div", 4)
                self.K(4, "gcd", 1)
                self.K(4, "gcd_work", bits(max(abs(t), 1)))
                n = self.mul(a, t)
                r = self.store(n, self.mul(b, d))
            else:
                bp, dp = self.dv(b, g), self.dv(d, g)
                t = self.add(self.sg(xneg, dp), self.sg(yneg, bp))
                h = self.g(t, g)
                if h == 1:                                            # K-5
                    self.K(5, "ops")
                    self.K(5, "div", 2)
                    n = self.mul(a, t)
                    r = self.store(n, self.mul(bp, d))
                else:
                    tq, dq = self.dv(t, h), self.dv(d, h)
                    n = self.mul(a, tq)
                    r = self.store(n, self.mul(bp, dq))
        else:                                                         # general
            k = self.case("add_general")
            g = self.g(b, d)
            if g == 1:                                                # K-6
                p = self.mul(self.sg(xneg, a), d)
                rr = self.mul(self.sg(yneg, c), b)
                t = self.add(p, rr)
                e = self.mul(b, d)
                self.K(6, "ops")
                self.K(6, "div", 4)
                self.K(6, "gcd", 1)
                self.K(6, "gcd_work", bits(max(abs(t), 1)))
                r = self.store(t, e)
            else:
                bp, dp = self.dv(b, g), self.dv(d, g)
                p = self.mul(self.sg(xneg, a), dp)
                rr = self.mul(self.sg(yneg, c), bp)
                t = self.add(p, rr)
                h = self.g(t, g)
                if h == 1:                                            # K-7
                    self.K(7, "ops")
                    self.K(7, "div", 2)
                    r = self.store(t, self.mul(bp, d))
                else:
                    n = self.dv(t, h)
                    dq = self.dv(d, h)
                    r = self.store(n, self.mul(bp, dq))
        self.reads(k, NEED_ADD, a, b, c, d, cells)
        return r, k

    # ------------------------------------------------------------------------------------------------ multiplication
    def multiply(self, x, y):
        """x * y, x a cell of A and y one of B"""
        self.ph = "mul"
        a, b, xneg = abs(x[0]), x[1], x[0] < 0
        c, d, yneg = abs(y[0]), y[1], y[0] < 0
        q_den, q_num, q_ad, q_cb = b == d, a == c, a == d, c == b
        q_ox, q_oy = a == b, c == d
        q_ix, q_iy, q_zx, q_zy = b == 1, d == 1, a == 0, c == 0
        self.C("cmp", 10)
        self.cross += 4
        self.cross_match += q_den + q_num + q_ad + q_cb
        self.cross_reads_V += (1 if a == b else 2) + (1 if c == d else 2)
        s = xneg != yneg
        if q_zx or q_zy:
            k = self.case("mul_zero")
            r = self.store(0, 1)
        elif q_ox:                                                    # one_x [K-15]
            k = self.case("mul_one_x")
            self.K(15, "ops")
            r = self.store(self.sg(s, c), d)
        elif q_oy:                                                    # one_y [K-15]
            k = self.case("mul_one_y")
            self.K(15, "ops")
            r = self.store(self.sg(s, a), b)
        elif q_ix and q_iy:                                           # int_both
            k = self.case("mul_int_both")
            n = self.mul(a, c)
            r = self.store(self.sg(s, n), 1)
        elif q_ad and q_cb:                                           # both [K-12]
            k = self.case("mul_both")
            self.K(12, "ops")
            r = self.store(self.sg(s, 1), 1)
        elif q_den:
            k = self.case("mul_den")
            n = self.mul(a, c)
            e = self.mul(d, d)
            r = self.store(self.sg(s, n), e)
        elif q_num:
            k = self.case("mul_num")
            n = self.mul(a, a)
            e = self.mul(b, d)
            r = self.store(self.sg(s, n), e)
        elif q_ad:
            k = self.case("mul_cross_ad")
            h = self.g(c, b)
            if h == 1:                                                # K-8
                self.K(8, "ops")
                self.K(8, "div", 2)
                r = self.store(self.sg(s, c), b)
            else:
                n = self.dv(c, h)
                r = self.store(self.sg(s, n), self.dv(b, h))
        elif q_cb:
            k = self.case("mul_cross_cb")
            h = self.g(a, d)
            if h == 1:                                                # K-8
                self.K(8, "ops")
                self.K(8, "div", 2)
                r = self.store(self.sg(s, a), d)
            else:
                n = self.dv(a, h)
                r = self.store(self.sg(s, n), self.dv(d, h))
        elif q_ix:                                                    # int_x: gcd(a, d)
            k = self.case("mul_int_x")
            h = self.g(a, d)
            if h == 1:                                                # K-8
                self.K(8, "ops")
                self.K(8, "div", 2)
                n = self.mul(a, c)
                r = self.store(self.sg(s, n), d)
            else:
                ap = self.dv(a, h)
                e = self.dv(d, h)
                n = self.mul(ap, c)
                r = self.store(self.sg(s, n), e)
        elif q_iy:                                                    # int_y: gcd(c, b)
            k = self.case("mul_int_y")
            h = self.g(c, b)
            if h == 1:                                                # K-8
                self.K(8, "ops")
                self.K(8, "div", 2)
                n = self.mul(a, c)
                r = self.store(self.sg(s, n), b)
            else:
                cp = self.dv(c, h)
                e = self.dv(b, h)
                n = self.mul(a, cp)
                r = self.store(self.sg(s, n), e)
        else:
            k = self.case("mul_general")
            g1, g2 = self.g(a, d), self.g(c, b)
            if g1 == 1:                                               # K-9
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
            r = self.store(self.sg(s, n), e)
        self.reads(k, NEED_MUL, a, b, c, d, True)
        return r, k

    def per_phase(self):
        out = {}
        for (ph, name), v in self.cnt.items():
            out.setdefault(ph, {})[name] = v
        return out


# ---------------------------------------------------------------------------------------------------- the stores
def t_fix_bits(R, K):
    kb = 2 * R * K + 2
    t = 0
    while (1 << t) < kb:
        t += 1
    return t


def handle_width(R, K, grow):
    """w_h (spec §5.2): the smallest of 8, 16, 32 with w_h - 1 >= log2 T_fix (fixed) or log2 T_fix + 1 (grow)"""
    need = t_fix_bits(R, K) + (1 if grow else 0)
    for wh in (8, 16, 32):
        if wh - 1 >= need:
            return wh
    raise ValueError("dimensions beyond 32-bit handles")


class Store:
    """One matrix's table and handle fields (spec §5), fixed or grow; the growths' rewrite probes are the lookup
    method's (the map method makes none and the same tables and fields)."""

    def __init__(self, role, R, K, grow):
        self.role, self.R, self.K, self.grow = role, R, K, grow
        self.wh = handle_width(R, K, grow)
        self.t = 4 if grow else t_fix_bits(R, K)
        self.T = 1 << self.t
        self.w = 8 if role == "C" else 64
        self.tab = [EMPTY] * self.T
        self.tab[0], self.tab[1] = 0, 1
        self.W = 2
        self.fields = []
        self.c = dict(found=0, new=0, probes=0, longest=0, widenings=0, copied=0, growths=0, reinserted=0,
                      rprobes=0, rewritten=0, wprobes=0)
        self.growth_log = []

    @staticmethod
    def probe(tab, T, t, v):
        """(slot, probes, found): found True (v), False (an empty slot), or None (T slots read, neither: the table
        is full and lacks v; an error for a store, \"none\" for tau's find)"""
        s = C.home(v, t)
        for n in range(1, T + 1):
            e = tab[s]
            if e == v:
                return s, n, True
            if e == EMPTY:
                return s, n, False
            s = (s + 1) & (T - 1)
        return s, T, None

    def count(self, found, n):
        self.c["found" if found else "new"] += 1
        self.c["probes"] += n
        self.c["longest"] = max(self.c["longest"], n)

    def store_whole(self, v):
        assert 0 <= v <= LIM
        if v in (0, 1):
            self.count(True, 0)
            return v
        s, n, f = self.probe(self.tab, self.T, self.t, v)
        if f:
            self.count(True, n)
            return s
        assert f is not None, "hash table full"
        if self.grow and self.W + 1 > self.T // 2:
            self.growth()
            s, n2, f = self.probe(self.tab, self.T, self.t, v)
            assert f is False
            n += n2
        need = v.bit_length() + 1
        if need > self.w:
            w2 = 16 if need <= 16 else 32 if need <= 32 else 64
            self.w = w2
            self.c["widenings"] += 1
            self.c["copied"] += self.T
        assert s > 1
        self.tab[s] = v
        self.W += 1
        self.count(False, n)
        return s

    def store_cell(self, m, d):
        i = self.store_whole(abs(m))
        self.fields.append(2 * i + (1 if m < 0 else 0))
        j = self.store_whole(d)
        self.fields.append(2 * j)

    def growth(self):
        T, T2, t2 = self.T, 2 * self.T, self.t + 1
        nt = [EMPTY] * T2
        nt[0], nt[1] = 0, 1
        g = dict(role=self.role, fields=len(self.fields), W=self.W, T_old=T, reinserted=0, rprobes=0, rewritten=0,
                 wprobes=0, bytes_new=T2 * self.w // 8, bytes_map=T * self.wh // 8)
        smap = {0: 0, 1: 1}
        for s in range(2, T):
            v = self.tab[s]
            if v == EMPTY:
                continue
            s2, n, f = self.probe(nt, T2, t2, v)
            assert f is False
            nt[s2] = v
            smap[s] = s2
            g["reinserted"] += 1
            g["rprobes"] += n
        for i, f in enumerate(self.fields):
            k = f >> 1
            v = self.tab[k]
            if v in (0, 1):
                k2 = v
            else:
                k2, n, found = self.probe(nt, T2, t2, v)
                assert found
                g["wprobes"] += n
            assert k2 == smap[k]                                      # lookup and map give the same handle
            self.fields[i] = 2 * k2 + (f & 1)
            g["rewritten"] += 1
        for key in ("reinserted", "rprobes", "rewritten", "wprobes"):
            self.c[key] += g[key]
        self.c["growths"] += 1
        self.growth_log.append(g)
        self.tab, self.T, self.t = nt, T2, t2

    def figures(self, method=None):
        """the stor line's figures (bench_m1.c), for the lookup rows, or the map row (method "map": no rewrite probes)"""
        f = dict(W=self.W, T=self.T, w=self.w, wh=self.wh, nf=len(self.fields), **self.c)
        if method == "map":
            f["wprobes"] = 0
        return f

    def value(self, cell):
        f, g = self.fields[2 * cell], self.fields[2 * cell + 1]
        v = self.tab[f >> 1]
        return (-v if f & 1 else v, self.tab[g >> 1])


def build_tau(A, B):
    """tau (spec §7.2): one position per slot of B's table: A's slot of the same whole number, or none (2^wh_A - 1);
    0 and 1 map to 0 and 1; each other whole number of B is a find in A (probes counted)."""
    none = (1 << A.wh) - 1
    tau, cnt = [], dict(tau_found=0, tau_none=0, tau_probes=0)
    for v in B.tab:
        if v == EMPTY:
            tau.append(none)
        elif v in (0, 1):
            tau.append(v)
        else:
            s, n, f = Store.probe(A.tab, A.T, A.t, v)
            cnt["tau_probes"] += n
            if f:
                cnt["tau_found"] += 1
                tau.append(s)
            else:
                cnt["tau_none"] += 1
                tau.append(none)
    return tau, cnt


# ---------------------------------------------------------------------------------------------------- a job
def predict(op, n, A_in, B_in):
    """Run the model over a job: the entered pairs of A and B (as entered, row-major), and the operation ("add" or
    "mul"). Returns the values of C (row-major), the arithmetic counters per phase (every row), M's K-Q3 reads per
    phase, the cross tests, and per sizing ("fixed", "grow") the stores' figures after creation and at the end, every
    growth, the sha256 of each table and each matrix's handle fields, and tau's (M rows) with its finds."""
    M = Arith()
    ent = {"A": [M.enter(m, d) for m, d in A_in], "B": [M.enter(m, d) for m, d in B_in]}
    q = n * n
    C_vals = []
    if op == "add":
        for i in range(q):
            r, _ = M.addition(ent["A"][i], ent["B"][i], True)
            C_vals.append(r)
    else:
        for i in range(n):
            for j in range(n):
                s = (0, 1)
                for k in range(n):
                    p, _ = M.multiply(ent["A"][i * n + k], ent["B"][k * n + j])
                    s, _ = M.addition(s, p, False)
                C_vals.append(s)
    nops = q if op == "add" else 2 * n ** 3
    sizings = {}
    for sizing in ("fixed", "grow"):
        g = sizing == "grow"
        S = {r: Store(r, n, n, g) for r in C.ROLES}
        for r in ("A", "B"):
            for m, d in ent[r]:
                S[r].store_cell(m, d)
        after = {r: S[r].figures() for r in C.ROLES}
        after_map = {r: S[r].figures("map") for r in C.ROLES}
        tau, tcnt = build_tau(S["A"], S["B"])
        for m, d in C_vals:
            S["C"].store_cell(m, d)
        for r in ("A", "B"):
            assert [S[r].value(i) for i in range(q)] == ent[r]
        assert [S["C"].value(i) for i in range(q)] == C_vals
        sizings[sizing] = {
            "stor_after_creation": after, "stor_end": {r: S[r].figures() for r in C.ROLES},
            "stor_after_creation_map": after_map, "stor_end_map": {r: S[r].figures("map") for r in C.ROLES},
            "growths": [x for r in C.ROLES for x in S[r].growth_log],
            "tables": {r: C.sha_ints(S[r].tab) for r in C.ROLES},
            "cells": {r: C.sha_ints(S[r].fields) for r in C.ROLES},
            "tau": C.sha_ints(tau), "tau_counts": tcnt,
            "fill": {r: S[r].W / S[r].T for r in C.ROLES},
            "bytes": {r: {"cells": 2 * q * S[r].wh // 8, "table": S[r].T * S[r].w // 8} for r in C.ROLES},
            "bytes_after_creation": {r: {"cells": 2 * q * S[r].wh // 8,
                                         "table": after[r]["T"] * after[r]["w"] // 8} for r in C.ROLES},
            "tau_bytes": len(tau) * S["A"].wh // 8,
        }
    return {"values": C_vals, "counters": M.per_phase(), "reads_M": dict(M.reads_M), "cross": M.cross,
            "cross_match": M.cross_match, "cross_reads_V": M.cross_reads_V, "ncase": dict(M.ncase), "nops": nops,
            "sizings": sizings}
