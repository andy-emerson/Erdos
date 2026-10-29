"""The workload generator of research/rule.md (Workloads, F1-F5), the exact references, and the predicted figures.

    python3 gen.py OUTDIR [--jobs 1,3] [--toy]

Master seed 20261029; job n uses random.Random(20261029 + n). For every job it writes, before any measured run:
    OUTDIR/job-NN.wl    the workload (calls: entries and operations, operands as handles), common.py format
    OUTDIR/job-NN.ref   the exact value of every stored value (Python fractions), lowest terms
    OUTDIR/job-NN.case  the case of spec §7 of every operation, from magnitudes (int8; -1 for an entry)
    OUTDIR/job-NN.json  parameters, realized reuse (rule item 12) and the predicted figures: W after entry and at the
                        end (spec §14.1 item 1), the count of each case per phase (item 2), and L1-opt's trick
                        counters O-1 to O-5 per phase (spec §17.3 item 2)
--toy scales every job down (10,000 inputs -> 200, 20,000 operations -> 400, 1,000 chains -> 20) to test the harness
only; toy workloads are never measured.

The 08 generator (shared/partial-sharing-baseline/test/gen.py, then 08-skeletons) is the starting point: draw_shared, den, vals, the G3 operations and the
Builder are copied from it (its decisions G-a, G-c, G-d, G-h carry over); 08 is not imported.
"""
import argparse
import json
import math
import random
import sys
from collections import Counter
from fractions import Fraction
from pathlib import Path

import numpy as np

import common as C
import l1model

POOL12 = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
L_CLOSED = 720_720
REDRAW_LIMIT = 1000


class NotConstructible(Exception):
    pass


def bits(x):
    return abs(int(x)).bit_length()


# ---------------------------------------------------------------------------------------------------- 08 G1, G2
def draw_shared(rng, sigma, B):
    """08 G1: d = s * u: s from the pool up to min(round(sigma B), B - 6) bits; u of a = B - bits(s) bits, no factor
    <= 37."""
    cap = min(round(sigma * B), B - 6)
    s = 1
    while True:
        p = rng.choice(POOL12)
        if bits(s * p) > cap:
            break
        s *= p
    a = B - bits(s)
    assert a >= 6
    while True:
        u = rng.randrange(2 ** (a - 1), 2 ** a)
        if all(u % p for p in POOL12):
            return s * u


def den_p50(rng, D, B=20):
    """08 G1 at P50 (sigma = 0.5): D distinct whole numbers; a repeat is redrawn (08 G-d)."""
    info = {"redraws_total": 0, "redraws_max": 0}
    dens, seen = [], set()
    for _ in range(D):
        k = 0
        while True:
            d = draw_shared(rng, 0.5, B)
            if d not in seen:
                break
            k += 1
            if k >= REDRAW_LIMIT:
                raise NotConstructible(f"a whole number reached {REDRAW_LIMIT} redraws")
        seen.add(d)
        dens.append(d)
        info["redraws_total"] += k
        info["redraws_max"] = max(info["redraws_max"], k)
    return dens, info


def vals(rng, dens, V, B):
    """08 G2: V values per denominator, numerators uniform in [-(2^B - 1), 2^B - 1], nonzero and coprime."""
    lim = 2 ** B - 1
    out = []
    for d in dens:
        for _ in range(V):
            while True:
                m = rng.randint(-lim, lim)
                if m != 0 and math.gcd(m, d) == 1:
                    break
            out.append((m, d))
    rng.shuffle(out)
    return out


# ---------------------------------------------------------------------------------------------------- 08 G3
def pick_op(rng, mix):
    """A mix names which operations are drawn, each with equal probability."""
    if mix == "add":
        return C.ADD
    if mix == "addsub":
        return (C.ADD, C.SUB)[rng.randrange(2)]
    if mix == "muldiv":
        return (C.MUL, C.DIV)[rng.randrange(2)]
    return (C.ADD, C.SUB, C.MUL, C.DIV)[rng.randrange(4)]


def ops(rng, n, mix, n_in):
    """08 G3 with rho = 1/2: (code, ref1, ref2), refs ('E', i) for entered value i or ('R', t) for operation t."""
    out, two_input = [], []
    for t in range(n):
        code = pick_op(rng, mix)
        if t > 0 and rng.random() < 0.5:
            out.append((code, ("R", rng.choice(two_input)), ("E", rng.randrange(n_in))))
        else:
            out.append((code, ("E", rng.randrange(n_in)), ("E", rng.randrange(n_in))))
            two_input.append(t)
    return out


def apply(code, x, y):
    if code == C.ADD:
        return x + y
    if code == C.SUB:
        return x - y
    if code == C.MUL:
        return x * y
    if y == 0:
        return None
    return x / y


def H(x):
    return max(abs(x.numerator), x.denominator)


class Builder:
    """Collects calls in order; every call stores one value, whose handle is its call index (08 harness)."""

    def __init__(self):
        self.op, self.a, self.b, self.val = [], [], [], []
        self.bound = (2 ** 31 - 1) ** 2                  # the 08 operand rule (rule, Size bound; 08 G-h)
        self.vmax = 2 ** 63 - 1

    def enter(self, m, d):
        assert d > 0 and abs(m) <= self.vmax and d <= self.vmax
        self.op.append(C.ENTER)
        self.a.append(m)
        self.b.append(d)
        self.val.append(Fraction(m, d))
        return len(self.op) - 1

    def operate(self, code, hx, hy):
        x, y = self.val[hx], self.val[hy]
        if H(x) * H(y) > self.bound:
            raise RuntimeError(f"operand rule broken at call {len(self.op)}: H(x) H(y) = {H(x) * H(y)}")
        v = apply(code, x, y)
        assert v is not None, "division by zero in a workload"
        assert abs(v.numerator) <= self.vmax and v.denominator <= self.vmax
        self.op.append(code)
        self.a.append(hx)
        self.b.append(hy)
        self.val.append(v)
        return len(self.op) - 1


def operand_stats(B_):
    """Realized share of operands that are earlier results (rule item 12)."""
    is_res = [o != C.ENTER for o in B_.op]
    total = res = 0
    for h, o in enumerate(B_.op):
        if o == C.ENTER:
            continue
        for x in (B_.a[h], B_.b[h]):
            total += 1
            res += is_res[x]
    return {"operands": total, "result_operands": res, "result_operand_share": res / total if total else None}


# ---------------------------------------------------------------------------------------------------- inputs
def sizes(job, toy):
    """(n_in, nops, nchains)."""
    j = C.JOBS[job]
    if not toy:
        return 10_000, j["nops"], j.get("chains", 0)
    if j["family"] == "F5":
        return 200, 20 * j["K"], 20
    return 200, 400, 0


def inputs(job, rng, n_in, meta):
    """The entered values (m, d), d > 0, as given to every row (money unreduced, over 100)."""
    name = C.JOBS[job]["name"]
    if name == "MONEY-wide":
        out = []
        for _ in range(n_in):
            while True:
                m = rng.randint(-(10 ** 7 - 1), 10 ** 7 - 1)
                if m != 0:
                    break
            out.append((m, 100))
        return out
    if name in ("MONEY-catalog", "CH-money"):
        cat = rng.sample(range(1, 10 ** 5), 200)
        meta["catalog"] = len(cat)
        return [(rng.choice(cat) * (1 if rng.randrange(2) else -1), 100) for _ in range(n_in)]
    if name in ("SMALL8", "SMALL64"):
        k = 8 if name == "SMALL8" else 64
        S = [1] + rng.sample(range(2, 2 ** 15), k - 1)
        meta["k"] = k
        out = []
        for _ in range(n_in):
            while True:
                a, b = rng.choice(S), rng.choice(S)
                if math.gcd(a, b) == 1:
                    break
            out.append((a if rng.randrange(2) else -a, b))
        return out
    if name in ("MOD-mixed", "MOD-muldiv"):
        P, info = den_p50(rng, 1000, 20)
        meta.update(pool=len(P), **info)
        out = []
        for _ in range(n_in):
            while True:
                a, b = rng.choice(P), rng.choice(P)
                if a != b and math.gcd(a, b) == 1:
                    break
            out.append((a if rng.randrange(2) else -a, b))
        return out
    if name == "NEW-add":
        D = 1000 if n_in == 10_000 else max(10, n_in // 10)
        dens, info = den_p50(rng, D, 20)
        meta.update(D=D, V=n_in // D, B=20, **info)
        return vals(rng, dens, n_in // D, 20)
    if name == "CH-closed":
        divs = [d for d in range(1, L_CLOSED + 1) if L_CLOSED % d == 0]
        assert len(divs) == 240
        out = []
        for _ in range(n_in):
            d = rng.choice(divs)
            while True:                                  # 08 G-f: |m| < d, coprime to d (d = 1 gives 0)
                m = rng.randint(-(d - 1), d - 1)
                if math.gcd(m, d) == 1:
                    break
            out.append((m, d))
        return out
    if name in ("CH-small", "CH-mixed"):
        out = []
        for _ in range(n_in):
            while True:
                a, b = rng.randint(1, 63), rng.randint(1, 63)
                if math.gcd(a, b) == 1:
                    break
            out.append((a if rng.randrange(2) else -a, b))
        return out
    raise ValueError(name)


# ---------------------------------------------------------------------------------------------------- predictions
def predict(b, n_in):
    """Run the model of L1 and of L1-opt over the calls: values (checked against the reference), cases, W, and the
    trick counters per phase."""
    L, O = l1model.Model(False), l1model.Model(True)
    cases = np.full(len(b.op), -1, dtype=np.int8)
    W_after_entry = None
    last_entry = max(i for i, o in enumerate(b.op) if o == C.ENTER)
    for i, o in enumerate(b.op):
        if o == C.ENTER:
            L.enter(b.a[i], b.b[i])
            O.enter(b.a[i], b.b[i])
        else:
            h, k = L.operate(o, b.a[i], b.b[i])
            h2, k2 = O.operate(o, b.a[i], b.b[i])
            assert k == k2
            cases[i] = C.CASE[k]
        v = b.val[i]
        assert L.value(i) == (v.numerator, v.denominator) == O.value(i), f"model value differs at call {i}"
        if i == last_entry:
            W_after_entry = len(L.W)
    assert L.W == O.W
    case_counts = {}
    for i, o in enumerate(b.op):
        if o != C.ENTER:
            ph = C.OPNAME[o]
            case_counts.setdefault(ph, Counter())[C.CASES[cases[i]]] += 1
    tricks = O.per_phase(set(l1model.TRICK_NAMES))
    tricks = {ph: {l1model.TRICK_NAMES[k]: v for k, v in d.items()} for ph, d in tricks.items()}
    base = L.per_phase(set(l1model.BASE_NAMES))
    base = {ph: {l1model.BASE_NAMES[k]: v for k, v in d.items()} for ph, d in base.items()}
    return cases, {"W_after_entry": W_after_entry, "W_end": len(L.W),
                   "case_counts": {ph: dict(c) for ph, c in case_counts.items()},
                   "tricks": tricks, "l1_base": base}


def distinct_wholes(vals_):
    s = set()
    for v in vals_:
        s.add(abs(v.numerator))
        s.add(v.denominator)
    return len(s)


def reuse(b, n_in, cases):
    """Realized reuse (rule item 12), from the reference alone."""
    N_e, N = n_in, len(b.op)
    W_e, W = distinct_wholes(b.val[:n_in]), distinct_wholes(b.val)
    share = {}
    for ph in ("add", "sub", "mul", "div"):
        idx = [i for i, o in enumerate(b.op) if o != C.ENTER and C.OPNAME[o] == ph]
        if idx:
            cnt = Counter(C.CASES[cases[i]] for i in idx)
            share[ph] = {k: cnt.get(k, 0) / len(idx) for k in C.CASES_OF[ph]}
    return {"after_entry": {"N": N_e, "W": W_e, "W_per_N": W_e / N_e, "places_per_whole": 2 * N_e / W_e},
            "end": {"N": N, "W": W, "W_per_N": W / N, "places_per_whole": 2 * N / W},
            "case_share": share}


# ---------------------------------------------------------------------------------------------------- jobs
def gen_job(job, out, toy):
    j = C.JOBS[job]
    rng = random.Random(C.MASTER_SEED + job)
    n_in, nops, nch = sizes(job, toy)
    meta = {"job": job, "name": j["name"], "seed": C.MASTER_SEED + job, "n_in": n_in, "mix": j["mix"], "toy": toy}
    vs = inputs(job, rng, n_in, meta)
    b = Builder()
    hin = [b.enter(m, d) for m, d in vs]
    if j["family"] != "F5":
        for code, (k1, i1), (k2, i2) in ops(rng, nops, j["mix"], n_in):
            h1 = i1 if k1 == "E" else n_in + i1
            h2 = i2 if k2 == "E" else n_in + i2
            b.operate(code, h1, h2)
        meta["nops"] = nops
    else:                                               # chains, round-robin, bounded by construction: no redraw
        K = j["K"]
        x = [hin[rng.randrange(n_in)] for _ in range(nch)]
        for k in range(K):
            for c in range(nch):
                code = pick_op(rng, j["mix"])
                hy = hin[rng.randrange(n_in)]
                x[c] = b.operate(code, x[c], hy)
        meta.update(chains=nch, K=K, nops=nch * K)
    for i, o in enumerate(b.op):                        # spec §10 item 4: no division by zero
        if o == C.DIV:
            assert b.val[b.b[i]] != 0
    cases, pred = predict(b, n_in)
    meta["predicted"] = pred
    meta["reuse"] = reuse(b, n_in, cases)
    assert meta["reuse"]["after_entry"]["W"] == pred["W_after_entry"]      # spec §14.1 item 1, from the reference
    assert meta["reuse"]["end"]["W"] == pred["W_end"]
    meta["operands"] = operand_stats(b)
    cnt = Counter(C.OPNAME[o] for o in b.op)                # calls per phase, for the counts per operation (item 12)
    meta["phase_counts"] = {("entry" if k == "enter" else k): v for k, v in cnt.items()}
    meta["H_max_bits"] = max(bits(H(v)) for v in b.val)
    C.write_workload(out / f"job-{job:02d}.wl", job, 64, j["fz"], nch, j.get("K", 0), n_in, b.op, b.a, b.b)
    C.write_pairs(out / f"job-{job:02d}.ref", [v.numerator for v in b.val], [v.denominator for v in b.val])
    (out / f"job-{job:02d}.case").write_bytes(cases.tobytes())
    return meta


def generate(out, jobs, toy=False):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    metas = {}
    for job in sorted(jobs):
        try:
            metas[job] = gen_job(job, out, toy)
        except NotConstructible as e:
            metas[job] = {"job": job, "name": C.JOBS[job]["name"], "constructible": False, "reason": str(e)}
    for job, m in metas.items():
        m.setdefault("constructible", True)
        (out / f"job-{job:02d}.json").write_text(json.dumps(m, indent=1))
    return metas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--jobs", default="")
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else sorted(C.JOBS)
    for j, m in generate(a.out, jobs, a.toy).items():
        if m.get("constructible"):
            r = m["reuse"]
            print(j, m["name"], f"W/N end {r['end']['W_per_N']:.3f}", json.dumps(m["predicted"]["case_counts"]),
                  json.dumps(m["operands"]))
        else:
            print(j, m["name"], "not constructible:", m["reason"])


if __name__ == "__main__":
    sys.exit(main())
