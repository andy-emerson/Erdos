"""The exact model of spec v0.2 (11-matrix, round m2), in Python integers, for the generator's predicted figures (spec
v0.2 §15.7 item 2; rule-m2, Run plan step 0). It shares no code with the C library or the 08 module.

Two parts, since the arithmetic does not depend on how whole numbers are stored:
  - the arithmetic: m1's model, m1_model.Arith, used unchanged (v0.2 §15.1: "Unchanged: the arithmetic and its
    cases"): one Arith per input's entry and one per step, so that each matrix's counters are those charged to it (an
    input its entry, a result its step), and each step's cross tests, matches and cross reads are its own;
  - the stores: one matrix's table and cells in the order of entry, as spec v0.2 §15.2 and §15.4 change them: every
    table starts all empty, every whole number (0 and 1 included) is hashed and probed, W counts what is stored; fixed
    (T_fix slots), grow (from 16, doubling when W + 1 > T/2, every used slot from slot 0 reinserted and mapped, the
    fields written so far rewritten through the map), and pack (fixed's table as the build table (h), then the list of
    its used slots in slot order, every field rewritten to its position, the handle width shrunk to the smallest of 8,
    16, 32 that names W positions); results from 8 bits with every widening.
"""
import m1_model
import m2_common as C
from m1_model import EMPTY, LIM, Arith, Overflow, handle_width, t_fix_bits  # noqa: F401  (m1's, unchanged)


def pack_width(W):
    """w_h after the packing (v0.2 §15.4 item 3.4): the smallest of 8, 16, 32 with 2^(w - 1) >= W"""
    for wh in (8, 16, 32):
        if (1 << (wh - 1)) >= W:
            return wh
    raise ValueError("more than 2^31 whole numbers")


class Store:
    """One matrix's table (or list) and handle fields, for one sizing: "fixed", "grow" or "pack"."""

    def __init__(self, name, R, K, sizing, inp):
        self.name, self.R, self.K, self.sizing = name, R, K, sizing
        grow = sizing == "grow"
        self.wh = handle_width(R, K, grow)
        self.whb = self.wh
        self.t = 4 if grow else t_fix_bits(R, K)
        self.T = 1 << self.t
        self.w = 64 if inp else 8
        self.tab = [EMPTY] * self.T                       # no slot reserved (§15.2)
        self.W = 0
        self.fields = []
        self.packed = False
        self.list = None
        self.hb = None                                    # pack: (h) as the packing read it
        self.c = dict(found=0, new=0, probes=0, longest=0, widenings=0, copied=0, growths=0, reinserted=0, rprobes=0,
                      rewritten=0)
        self.growth_log = []
        self.pack_rec = None

    probe = staticmethod(m1_model.Store.probe)

    def count(self, found, n):
        self.c["found" if found else "new"] += 1
        self.c["probes"] += n
        self.c["longest"] = max(self.c["longest"], n)

    def store_whole(self, v):
        assert 0 <= v <= LIM and not self.packed
        s, n, f = self.probe(self.tab, self.T, self.t, v)
        if f:
            self.count(True, n)
            return s
        assert f is not None, "hash table full"
        if self.sizing == "grow" and self.W + 1 > self.T // 2:
            self.growth()
            s, n2, f = self.probe(self.tab, self.T, self.t, v)
            assert f is False
            n += n2
        need = v.bit_length() + 1
        if need > self.w:
            self.w = 16 if need <= 16 else 32 if need <= 32 else 64
            self.c["widenings"] += 1
            self.c["copied"] += self.T
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
        g = dict(name=self.name, fields=len(self.fields), W=self.W, T_old=T, reinserted=0, rprobes=0, rewritten=0,
                 bytes_new=T2 * self.w // 8, bytes_map=T * self.wh // 8)
        smap = {}
        for s in range(T):                                # from slot 0: every used slot (§15.2)
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
            self.fields[i] = 2 * smap[f >> 1] + (f & 1)
            g["rewritten"] += 1
        for key in ("reinserted", "rprobes", "rewritten"):
            self.c[key] += g[key]
        self.c["growths"] += 1
        self.growth_log.append(g)
        self.tab, self.T, self.t = nt, T2, t2

    def finish(self):
        """the end of the matrix's build: Hash-V-pack packs (§15.4 item 3); the other sizings keep their table"""
        assert len(self.fields) == 2 * self.R * self.K
        if self.sizing != "pack":
            return
        pos, lst = {}, []
        for s, v in enumerate(self.tab):                  # copy in slot order
            if v != EMPTY:
                pos[s] = len(lst)
                lst.append(v)
        assert len(lst) == self.W
        wb, wp, nf = self.wh, pack_width(self.W), len(self.fields)
        self.fields = [2 * pos[f >> 1] + (f & 1) for f in self.fields]
        self.pack_rec = dict(name=self.name, W=self.W, T=self.T, w=self.w, whb=wb, whp=wp, rewritten=nf,
                             bytes_list=self.W * self.w // 8, bytes_map=self.T * wb // 8,
                             bytes_table=self.T * self.w // 8, bytes_cells_build=nf * wb // 8,
                             bytes_cells_pack=nf * wp // 8)
        self.hb, self.list, self.tab = self.tab, lst, None
        self.whb, self.wh, self.packed = wb, wp, True

    def vals(self):
        return self.list if self.packed else self.tab

    def figures(self):
        """the stor line's figures (bench_m2.c)"""
        return dict(W=self.W, T=self.T, w=self.w, wh=self.wh, whb=self.whb, packed=int(self.packed),
                    nf=len(self.fields), bcells=2 * self.R * self.K * self.wh // 8,
                    bvals=(self.W if self.packed else self.T) * self.w // 8, **self.c)

    def value(self, cell):
        f, g = self.fields[2 * cell], self.fields[2 * cell + 1]
        v = self.vals()
        return (-v[f >> 1] if f & 1 else v[f >> 1], v[g >> 1])


# ---------------------------------------------------------------------------------------------------- a job
def predict(op, n, inputs):
    """Run the model over a job of set `op` ("add", "mul", "sop", "pos"): the inputs' entered pairs (as entered,
    row-major), in order. Returns every matrix's values (row-major), the counters of each matrix per phase, each
    result's cross tests, the cases, and per sizing ("fixed", "grow", "pack") every store's figures after its build and
    at its end, every growth and packing in order, and the sha256 of each table (or list), each matrix's handle fields
    and (pack) each build table (h)."""
    names_in = C.SET_INPUTS[op]
    arith, vals = {}, {}
    for name, X in zip(names_in, inputs):
        a = Arith()
        vals[name] = [a.enter(m, d) for m, d in X]
        arith[name] = a
    for kind, x, y, z in C.SET_STEPS[op]:
        a = Arith()
        if kind == "add":
            out = [a.addition(vals[x][i], vals[y][i], True)[0] for i in range(n * n)]
        else:
            out = []
            for i in range(n):
                for j in range(n):
                    s = (0, 1)
                    for k in range(n):
                        p, _ = a.multiply(vals[x][i * n + k], vals[y][k * n + j])
                        s, _ = a.addition(s, p, False)
                    out.append(s)
        vals[z] = out
        arith[z] = a
    ncase = {}
    for z in C.SET_MATRICES[op]:
        for k, v in arith[z].ncase.items():
            ncase[k] = ncase.get(k, 0) + v
    steps = {z: {"ncase": dict(arith[z].ncase), "cross": arith[z].cross, "cross_match": arith[z].cross_match,
                 "cross_reads": arith[z].cross_reads_V} for _, _, _, z in C.SET_STEPS[op]}
    sizings = {}
    for sizing in ("fixed", "grow", "pack"):
        S, built, growths, packs = {}, {}, [], []
        for name in C.SET_MATRICES[op]:                   # the order of the builds: the inputs, then the steps
            st = S[name] = Store(name, n, n, sizing, name in names_in)
            for m, d in vals[name]:
                st.store_cell(m, d)
            st.finish()
            growths += st.growth_log
            if st.pack_rec:
                packs.append(st.pack_rec)
            if name in names_in:
                built[name] = st.figures()
            assert [st.value(i) for i in range(n * n)] == vals[name]
        sizings[sizing] = {
            "stor_built": built, "stor_end": {k: S[k].figures() for k in S}, "growths": growths, "packs": packs,
            "tables": {k: C.sha_ints(S[k].vals()) for k in S}, "cells": {k: C.sha_ints(S[k].fields) for k in S},
            "hb": {k: C.sha_ints(S[k].hb) for k in S if S[k].hb is not None},
            "fill": {k: S[k].W / S[k].T for k in S},
        }
    for k in C.SET_MATRICES[op]:                          # pack's (h) is fixed's table, slot for slot (§15.7 item 2)
        assert sizings["pack"]["hb"][k] == sizings["fixed"]["tables"][k]
    return {"values": vals, "counters": {k: arith[k].per_phase() for k in arith}, "steps": steps, "ncase": ncase,
            "nops": C.nops(op, n), "sizings": sizings}
