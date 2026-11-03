"""The workloads of spec v0.4 §17.9, for the Nim build's checks (README.md):

    python3 gen.py OUTDIR --op add|mul|sop|pos [--jobs 1,4,5,8,9,12]

Every whole number is drawn the same way, numerators and denominators alike: start at 1; for each prime p in the order
2, 3, 5, 7, multiply by p while randrange(p) == 0; reject a value above 32767 and draw it again at once, from the same
stream. A numerator takes a random sign from randrange(2) (0: +, 1: -). For each rational: the numerator's magnitude,
its sign, then the denominator's magnitude. Pools as rule-m1's (m1_gen.draw_matrix): s = n^2 / rho distinct values
per matrix (a rational whose value is already in the pool is discarded and a whole new one drawn), each pool value
once, then n^2 - s cells by choice(pool), then shuffle(cells); the job's matrices in order (A, B; A, B, C, D) from one
random.Random stream seeded as before (m2_common.job_seed). Pairs are written as drawn; the entry reduces them, as
before (m1_gen). The grid, sizes and seeds are m3's (m2_common). This writes, per job:
    OUTDIR/job-NN.wl      the workload (m2_common.write_workload: the driver's format)
    OUTDIR/job-NN.ref.json  every matrix built by an operation (the result; in sop and pos each intermediate too), by
                            Python's fractions, exact (values past 64 bits included: those cells must be marked),
                            {name: [[num, den], ...]} row-major
    OUTDIR/gen.json       each job's n, rho, s, seed, the rationals drawn per matrix, and each file's sha256
"""
import argparse
import json
import random
import sys
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / "test"))
import m2_common as C  # noqa: E402  (the harness's grid, seeds and file formats, unchanged)

PYTHON = "3.13.17"   # §17.9: the generator's Python is pinned; randrange, choice and shuffle depend on the version
PRIMES = (2, 3, 5, 7)
LIMIT = 32767


def magnitude(rng):
    """§17.9 steps 1-3: a 7-smooth whole number at most 32767"""
    while True:
        v = 1
        for p in PRIMES:
            while rng.randrange(p) == 0:
                v *= p
        if v <= LIMIT:
            return v


def rational(rng):
    """§17.9 step 4 and the order of calls: the numerator's magnitude, its sign, the denominator's magnitude"""
    m = magnitude(rng)
    if rng.randrange(2) == 1:
        m = -m
    return m, magnitude(rng)


def draw_matrix(rng, n, s):
    """n^2 cells (m, d) as drawn, with exactly s distinct values (rule-m1's pools, m1_gen.draw_matrix's order)"""
    pool, seen, drawn = [], set(), 0
    while len(pool) < s:
        m, d = rational(rng)
        drawn += 1
        v = Fraction(m, d)
        if v in seen:
            continue
        seen.add(v)
        pool.append((m, d))
    cells = pool + [rng.choice(pool) for _ in range(n * n - s)]
    rng.shuffle(cells)
    return cells, drawn


def workload(op, job):
    n, s = C.job_n(job), C.job_s(job)
    rng = random.Random(C.job_seed(op, job))
    mats, drawn = [], []
    for _ in C.SET_INPUTS[op]:
        cells, k = draw_matrix(rng, n, s)
        mats.append(cells)
        drawn.append(k)
    return n, s, mats, drawn


def reference(op, n, inputs):
    """every matrix's exact value by Python's fractions, {name: [(num, den)]}"""
    F = {name: [Fraction(m, d) for m, d in X] for name, X in zip(C.SET_INPUTS[op], inputs)}
    for kind, x, y, z in C.SET_STEPS[op]:
        if kind == "add":
            F[z] = [a + b for a, b in zip(F[x], F[y])]
        else:
            out = []
            for i in range(n):
                row = F[x][i * n:(i + 1) * n]
                for j in range(n):
                    out.append(sum((row[k] * F[y][k * n + j] for k in range(n)), Fraction(0)))
            F[z] = out
    return {k: [(v.numerator, v.denominator) for v in X] for k, X in F.items()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--op", choices=C.OPS, required=True)
    ap.add_argument("--jobs", default="1,4,5,8,9,12")
    a = ap.parse_args()
    if sys.version.split()[0] != PYTHON:
        sys.exit(f"the generator's Python is pinned to {PYTHON} (§17.9); this is {sys.version.split()[0]}")
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    summary = {}
    for job in [int(j) for j in a.jobs.split(",")]:
        n, s, inputs, drawn = workload(a.op, job)
        wl, ref = out / f"job-{job:02d}.wl", out / f"job-{job:02d}.ref.json"
        C.write_workload(wl, a.op, n, job, inputs)
        want = reference(a.op, n, inputs)
        built = C.SET_INTERMEDIATES[a.op] + (C.SET_RESULT[a.op],)
        ref.write_text(json.dumps({x: want[x] for x in built}))
        files = [wl, ref]
        summary[str(job)] = {"op": a.op, "job": job, "n": n, "rho": C.job_rho(job), "s": s,
                             "seed": C.job_seed(a.op, job), "drawn": drawn,
                             "sha256": {f.name: C.sha256(f) for f in files}}
        print(f"{a.op}-{job} n={n} rho={C.job_rho(job)} s={s} drawn={drawn}")
    (out / "gen.json").write_text(json.dumps(summary, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
