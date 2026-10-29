"""The workload generator of research/rule.md (Workloads, G1-G6), and the exact references.

    python3 gen.py OUTDIR [--jobs 1,3,19] [--toy]

Master seed 20261028; job n uses random.Random(20261028 + n), except job 20 (the early twin), which is built from
job 19's workload. For every job it writes, before any measured run:
    OUTDIR/job-NN.wl    the workload (calls: entries and operations, operands as handles), common.py format
    OUTDIR/job-NN.ref   the exact value of every stored value (Python fractions), lowest terms
    OUTDIR/job-NN.json  parameters, redraws, LG failures, constructibility, chain restarts, realized sharing
                        (10,000 random pairs of distinct entered denominators) and realized result operands (G3)
--toy scales every job down (10,000 inputs -> 200, 20,000 operations -> 400, 1,000 chains -> 20) to test the harness
only; toy workloads are never measured.
"""
import argparse
import json
import math
import random
import sys
from fractions import Fraction
from pathlib import Path

import numpy as np

import common as C

POOL12 = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
L_CLOSED = 720_720
REDRAW_LIMIT = 1000          # one denominator reaching this many redraws: job not constructible
G6_REDRAWS = 16


class NotConstructible(Exception):
    pass


def bits(x):
    return abs(int(x)).bit_length()


def is_prime(n):
    if n < 2:
        return False
    for p in POOL12:
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in POOL12:                       # deterministic for n < 3.3e24
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


# ---------------------------------------------------------------------------------------------------- G1
def draw_shared(rng, sigma, B):
    """d = s * u: s from the pool up to min(round(sigma B), B - 6) bits; u of a = B - bits(s) bits, no factor <= 37."""
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


def draw_prime(rng, B):
    while True:
        x = rng.randrange(2 ** (B - 1), 2 ** B)
        if is_prime(x):
            return x


K_PRIMES = tuple(p for p in range(41, 128) if is_prime(p))


def lg_pool(B=20):
    """The 57 shared parts of LG and each one's admissible private parts (primes above 127 of a = B - bits(s) bits)."""
    prods = sorted(p * q for i, p in enumerate(K_PRIMES) for q in K_PRIMES[i + 1:] if p * q < 2 ** 12)
    pool = list(K_PRIMES) + prods
    assert len(K_PRIMES) == 19 and len(prods) == 38 and len(pool) == 57
    lists = {}
    for s in pool:
        a = B - bits(s)
        lists[s] = [u for u in range(max(128, 2 ** (a - 1)), 2 ** a) if is_prime(u)]
    if B == 20:
        sizes = {s: len(v) for s, v in lists.items()}
        assert all(sizes[s] == {6: 872, 7: 464, 11: 43, 12: 23}[bits(s)] for s in pool), sizes
        assert sum(sizes.values()) == 12_198
    return pool, lists


def den(rng, D, level, B):
    """D distinct denominators; returns (dens, info)."""
    info = {"redraws_total": 0, "redraws_max": 0}
    if level == "ALL":
        return [draw_shared(rng, 0.5, B)], info
    if level == "LG":
        pool, lists = lg_pool(B)
        base, extra = divmod(D, len(pool))
        plus = set(rng.sample(range(len(pool)), extra))
        dens, failures = [], 0
        for i, s in enumerate(pool):
            k = base + (1 if i in plus else 0)
            if k > len(lists[s]):
                failures += k - len(lists[s])
                k = len(lists[s])
            dens += [s * u for u in rng.sample(lists[s], k)]
        info["lg_failures"] = failures
        if failures:
            raise RuntimeError(f"LG construction failures: {failures} (a generator bug; generation stops)")
        rng.shuffle(dens)
        assert len(set(dens)) == D
        return dens, info
    sigma = C.SIGMA[level]
    draw = (lambda: draw_prime(rng, B)) if sigma == 0 else (lambda: draw_shared(rng, sigma, B))
    dens, seen = [], set()
    for _ in range(D):
        k = 0
        while True:
            d = draw()
            if d not in seen:
                break
            k += 1
            if k >= REDRAW_LIMIT:
                raise NotConstructible(f"a denominator reached {REDRAW_LIMIT} redraws")
        seen.add(d)
        dens.append(d)
        info["redraws_total"] += k
        info["redraws_max"] = max(info["redraws_max"], k)
    return dens, info


# ---------------------------------------------------------------------------------------------------- G2
def vals(rng, dens, V, B):
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


# ---------------------------------------------------------------------------------------------------- G3
def pick_op(rng, mix):
    if mix == "add":
        return C.ADD
    if mix == "muldiv":
        return (C.MUL, C.DIV)[rng.randrange(2)]
    if mix == "addsub":
        return (C.ADD, C.SUB)[rng.randrange(2)]
    return (C.ADD, C.SUB, C.MUL, C.DIV)[rng.randrange(4)]


def ops(rng, n, mix, n_in):
    """Operations as (code, ref1, ref2), refs ('E', i) for entered value i or ('R', t) for operation t's result."""
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


# ---------------------------------------------------------------------------------------------------- assembly
class Builder:
    """Collects calls in order; every call stores one value, whose handle is its call index."""

    def __init__(self, width):
        self.width = width
        self.op, self.a, self.b, self.val = [], [], [], []
        self.bound = (2 ** 31 - 1) ** 2 if width == 64 else (2 ** 15 - 1) ** 2
        self.vmax = 2 ** (width - 1) - 1

    def enter(self, m, d):
        self.op.append(C.ENTER)
        self.a.append(m)
        self.b.append(d)
        v = Fraction(m, d)
        self.val.append(v)
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


def emit(job, B_, path, ceil_kind=0, nchains=0, K=0):
    n_in = sum(1 for o in B_.op if o == C.ENTER)
    C.write_workload(path / f"job-{job:02d}.wl", job, B_.width, ceil_kind, nchains, K, n_in, B_.op, B_.a, B_.b)
    C.write_pairs(path / f"job-{job:02d}.ref", [v.numerator for v in B_.val], [v.denominator for v in B_.val])


def operand_stats(B_):
    """G3 realized: share of operands that are earlier results; distinct result denominators used as operands."""
    is_res = [o != C.ENTER for o in B_.op]
    entered = {B_.val[h].denominator for h, o in enumerate(B_.op) if o == C.ENTER}
    total = res = 0
    res_dens = set()
    for h, o in enumerate(B_.op):
        if o == C.ENTER:
            continue
        for x in (B_.a[h], B_.b[h]):
            total += 1
            if is_res[x]:
                res += 1
                res_dens.add(B_.val[x].denominator)
    return {"operands": total, "result_operands": res, "result_operand_share": res / total if total else None,
            "distinct_result_dens_as_operands": len(res_dens),
            "of_which_not_entered_dens": len(res_dens - entered)}


def realized_sharing(rng, dens, pairs=10_000):
    if len(dens) < 2:
        return {"pairs": 0, "note": "fewer than two distinct denominators: no pairs"}
    shared = gsum = 0
    rel = 0.0
    for _ in range(pairs):
        i, j = rng.sample(range(len(dens)), 2)
        g = math.gcd(dens[i], dens[j])
        if g > 1:
            shared += 1
            gsum += bits(g)
        rel += bits(g) / bits(min(dens[i], dens[j]))
    return {"pairs": pairs, "frac_gcd_gt1": shared / pairs, "mean_gcd_bits": gsum / shared if shared else None,
            "mean_rel_gcd_bits": rel / pairs}


def scaled(job, toy):
    """Sizes for a job: (n_in, nops, nchains, D)."""
    j = C.JOBS[job]
    if not toy:
        return 10_000, j["nops"], j.get("chains", 0), j["D"]
    D = j["D"]
    if D is not None and D > 1:
        D = max(min(D, 10), round(D / 50))
    return 200, (400 if j["family"] != "chains" else 20 * j["K"]), (20 if j["family"] == "chains" else 0), D


def gen_matrix(job, out, toy):
    j = C.JOBS[job]
    rng = random.Random(C.MASTER_SEED + job)
    n_in, nops, _, D = scaled(job, toy)
    V = n_in // D
    meta = {"job": job, "name": j["name"], "seed": C.MASTER_SEED + job, "D": D, "V": V, "B": j["B"],
            "n_in": n_in, "nops": nops, "width": j["width"]}
    dens, info = den(rng, D, j["level"], j["B"])
    meta.update(info)
    vs = vals(rng, dens, V, j["B"])
    os_ = ops(rng, nops, j["mix"], n_in)
    b = Builder(j["width"])
    for m, d in vs:
        b.enter(m, d)
    for code, (k1, i1), (k2, i2) in os_:
        h1 = i1 if k1 == "E" else n_in + i1
        h2 = i2 if k2 == "E" else n_in + i2
        b.operate(code, h1, h2)
    meta["sharing"] = realized_sharing(rng, dens)
    meta["operands"] = operand_stats(b)
    emit(job, b, out, ceil_kind=(1 if j["ceil"] else 0))
    return meta


def gen_arrival(out, toy):
    """Job 19 (arriving) and job 20 (its early twin, built from job 19's workload)."""
    j = C.JOBS[19]
    rng = random.Random(C.MASTER_SEED + 19)
    n_in, nops, _, D = scaled(19, toy)
    V = n_in // D
    meta = {"job": 19, "name": j["name"], "seed": C.MASTER_SEED + 19, "D": D, "V": V, "B": 20, "n_in": n_in,
            "nops": nops, "width": 64}
    dens, info = den(rng, D, "P50", 20)
    meta.update(info)
    vs = vals(rng, dens, V, 20)
    # batches: denominators in order of first appearance, 20 equal batches, each with all its values
    order, seen = [], set()
    for m, d in vs:
        if d not in seen:
            seen.add(d)
            order.append(d)
    per = D // 20
    batch_of = {d: i // per for i, d in enumerate(order)}
    batches = [[] for _ in range(20)]               # entered value ids per batch, in entry (shuffled) order
    entry_seq = []
    for t in range(20):
        for i, (m, d) in enumerate(vs):
            if batch_of[d] == t:
                batches[t].append(len(entry_seq))
                entry_seq.append((m, d))
    seg = nops // 20
    opl, two_input = [], []
    for t in range(20):
        upto = [i for bt in batches[:t + 1] for i in bt]
        for _ in range(seg):
            code = pick_op(rng, "mixed")
            if opl and rng.random() < 0.5:
                opl.append((t, code, ("R", rng.choice(two_input)), ("E", rng.choice(batches[t]))))
            else:
                opl.append((t, code, ("E", rng.choice(batches[t])), ("E", rng.choice(upto))))
                two_input.append(len(opl) - 1)
    metas = {}
    for job, early in ((19, False), (20, True)):
        b = Builder(64)
        ehandle, rhandle = {}, {}
        if early:
            for i, (m, d) in enumerate(entry_seq):
                ehandle[i] = b.enter(m, d)
        done = -1
        for k, (t, code, r1, r2) in enumerate(opl):
            if not early and t > done:
                for i in batches[t]:
                    ehandle[i] = b.enter(*entry_seq[i])
                done = t
            hs = [ehandle[i] if kind == "E" else rhandle[i] for kind, i in (r1, r2)]
            rhandle[k] = b.operate(code, *hs)
        m = dict(meta, job=job, name=C.JOBS[job]["name"])
        if early:
            m["built_from"] = "job 19's workload: the same batches, all entered first, then the same operations"
        m["operands"] = operand_stats(b)
        emit(job, b, out)
        metas[job] = m
    metas[19]["sharing"] = metas[20]["sharing"] = realized_sharing(rng, dens)
    return metas


def gen_chains(job, out, toy):
    j = C.JOBS[job]
    rng = random.Random(C.MASTER_SEED + job)
    n_in, nops, nch, D = scaled(job, toy)
    K = j["K"]
    meta = {"job": job, "name": j["name"], "seed": C.MASTER_SEED + job, "n_in": n_in, "chains": nch, "K": K,
            "width": 64}
    dens = None
    if j["name"] in ("CH-closed", "CH-all"):
        if j["name"] == "CH-closed":
            divs = [d for d in range(1, L_CLOSED + 1) if L_CLOSED % d == 0]
            assert len(divs) == 240
        else:
            divs = [draw_shared(rng, 0.5, 24)]
        vs = []
        for _ in range(n_in):
            d = rng.choice(divs)
            while True:
                m = rng.randint(-(d - 1), d - 1)
                if math.gcd(m, d) == 1:
                    break
            vs.append((m, d))
        dens = sorted({d for _, d in vs})
        meta["D_entered"] = len(dens)
        redraw = False
    else:
        V = n_in // D
        level = "NONE" if j["name"] == "CH-none" else "P50"
        dens, info = den(rng, D, level, 12)
        meta.update(info, D=D, V=V, B=12)
        vs = vals(rng, dens, V, 12)
        redraw = True
    b = Builder(64)
    hin = [b.enter(m, d) for m, d in vs]
    x = [hin[rng.randrange(n_in)] for _ in range(nch)]
    lim = 2 ** 31
    restarts = redraws = 0
    for k in range(K):
        for c in range(nch):
            code = pick_op(rng, j["mix"])
            tries = 0
            while True:
                hy = hin[rng.randrange(n_in)]
                v = apply(code, b.val[x[c]], b.val[hy])
                if v is not None and abs(v.numerator) < lim and v.denominator < lim:
                    break
                if not redraw:
                    raise RuntimeError(f"{j['name']}: a chain value left the bound without a redraw rule")
                tries += 1
                redraws += 1
                if tries > G6_REDRAWS:
                    x[c] = hin[rng.randrange(n_in)]
                    restarts += 1
                    tries = 0
            x[c] = b.operate(code, x[c], hy)
    meta["g6_redraws"] = redraws
    meta["restarts"] = restarts
    meta["nops"] = nch * K
    meta["sharing"] = realized_sharing(rng, dens)
    meta["operands"] = operand_stats(b)
    emit(job, b, out, ceil_kind=(2 if j["ceil"] else 0), nchains=nch, K=K)
    return meta


def generate(out, jobs, toy=False):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    metas = {}
    for job in sorted(jobs):
        if job == 20 and 19 in metas:
            continue
        fam = C.JOBS[job]["family"]
        try:
            if fam in ("matrix", "w32"):
                metas[job] = gen_matrix(job, out, toy)
            elif fam == "arrival":
                metas.update(gen_arrival(out, toy))
            else:
                metas[job] = gen_chains(job, out, toy)
        except NotConstructible as e:
            metas[job] = {"job": job, "name": C.JOBS[job]["name"], "constructible": False, "reason": str(e)}
            if fam == "arrival":
                metas[20] = dict(metas[job], job=20, name=C.JOBS[20]["name"])
    for job, m in metas.items():
        m.setdefault("constructible", True)
        m["toy"] = toy
        (out / f"job-{job:02d}.json").write_text(json.dumps(m, indent=1))
    return {j: metas[j] for j in jobs if j in metas}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--jobs", default="")
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else sorted(C.JOBS)
    for j, m in generate(a.out, jobs, a.toy).items():
        print(j, m["name"], json.dumps(m.get("sharing")), json.dumps(m.get("operands")))


if __name__ == "__main__":
    sys.exit(main())
