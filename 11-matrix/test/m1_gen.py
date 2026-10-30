"""The matrix round's workloads, exact references and predicted figures (rule-m1, Workloads and Run plan step 0; spec
§10 items 1-2). New in this round.

    python3 m1_gen.py OUTDIR --op add|mul [--jobs 1,6] [--toy]

Job j of the addition set is seeded 20261029 + j, of the multiplication set 20261029 + 100 + j; each draws A's values,
then B's, from one random.Random stream. A value: a numerator uniform in [-32767, 32767] and a denominator uniform among
the 30 divisors of 720 (the pair written as drawn; the value it names is reduced, and the entry reduces the pair
gcd-first, spec §8). Sharing: a pool of s = n^2 / rho distinct values per matrix (a draw whose value is already in the
pool is drawn again); each pool value is placed once, the other cells are drawn uniformly from the pool, then the cells
are shuffled, so each matrix has exactly s distinct values. A and B draw separate pools. This writes:
    OUTDIR/job-NN.wl        the workload (m1_common.write_workload)
    OUTDIR/job-NN.ref       C = A + B or C = A B by Python's fractions, row-major (the read-out's format)
    OUTDIR/job-NN.m1.json   the model's predicted figures (m1_model.predict), and the realized sharing
    OUTDIR/gen.json         every job's summary, with each file's sha256 (rule-m1, Run plan step 0)
The runner calls this as its own process, so that the runner never imports more than it needs.
"""
import argparse
import json
import random
import sys
from fractions import Fraction
from pathlib import Path

import m1_common as C
import m1_model

DIVS720 = tuple(d for d in range(1, 721) if 720 % d == 0)
assert len(DIVS720) == 30


def draw_matrix(rng, n, s):
    """n^2 cells (m, d) as drawn, with exactly s distinct values"""
    pool, seen = [], set()
    while len(pool) < s:
        m, d = rng.randint(-32767, 32767), rng.choice(DIVS720)
        v = Fraction(m, d)
        if v in seen:
            continue
        seen.add(v)
        pool.append((m, d))
    cells = pool + [rng.choice(pool) for _ in range(n * n - s)]
    rng.shuffle(cells)
    return cells


def workload(op, job, toy=False):
    n, s = C.job_n(job, toy), C.job_s(job, toy)
    rng = random.Random(C.job_seed(op, job))
    A = draw_matrix(rng, n, s)
    B = draw_matrix(rng, n, s)
    return n, s, A, B


def reference(op, n, A, B):
    FA = [Fraction(m, d) for m, d in A]
    FB = [Fraction(m, d) for m, d in B]
    if op == "add":
        out = [x + y for x, y in zip(FA, FB)]
    else:
        out = []
        for i in range(n):
            row = FA[i * n:(i + 1) * n]
            for j in range(n):
                s = Fraction(0)
                for k in range(n):
                    s += row[k] * FB[k * n + j]
                out.append(s)
    return [(v.numerator, v.denominator) for v in out]


def gen_job(op, job, out, toy):
    n, s, A, B = workload(op, job, toy)
    wl, ref, js = out / f"job-{job:02d}.wl", out / f"job-{job:02d}.ref", out / f"job-{job:02d}.m1.json"
    C.write_workload(wl, op, n, job, A, B)
    want = reference(op, n, A, B)
    C.write_pairs(ref, want)
    p = m1_model.predict(op, n, A, B)
    assert p["values"] == want, f"{op} job {job}: the model's values differ from the exact reference"
    for v in want:                                       # every value of C fits 64 bits (rule-m1, Workloads)
        assert abs(v[0]) <= m1_model.LIM and v[1] <= m1_model.LIM
    realized = {"s": s, "distinct_A": len({Fraction(m, d) for m, d in A}),
                "distinct_B": len({Fraction(m, d) for m, d in B}), "distinct_C": len(set(want)),
                "W": {sz: {r: p["sizings"][sz]["stor_end"][r]["W"] for r in C.ROLES} for sz in ("fixed",)}}
    assert realized["distinct_A"] == s and realized["distinct_B"] == s
    meta = {"op": op, "job": job, "name": C.job_name(op, job), "n": n, "rho": C.job_rho(job), "s": s, "toy": toy,
            "seed": C.job_seed(op, job), "nops": p["nops"], "realized": realized,
            "predicted": {k: v for k, v in p.items() if k != "values"}}
    js.write_text(json.dumps(meta, indent=0))
    return {"op": op, "job": job, "name": meta["name"], "n": n, "rho": meta["rho"], "s": s, "nops": p["nops"],
            "toy": toy, "sha256": {f.name: C.sha256(f) for f in (wl, ref, js)}, "realized": realized,
            "ncase": p["ncase"]}


def generate(out, op, jobs, toy=False):
    out = Path(out)
    bad = C.forbidden(out)
    if bad:
        raise SystemExit(f"refusing to write: {bad}")
    out.mkdir(parents=True, exist_ok=True)
    summary = {}
    for job in sorted(jobs):
        summary[str(job)] = gen_job(op, job, out, toy)
    (out / "gen.json").write_text(json.dumps(summary, indent=1))
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--op", choices=C.OPS, required=True)
    ap.add_argument("--jobs", default="")
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else list(C.JOBS)
    s = generate(a.out, a.op, jobs, a.toy)
    for j, m in s.items():
        print(m["name"], f"n={m['n']} rho={m['rho']} s={m['s']} ops={m['nops']}", "ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
