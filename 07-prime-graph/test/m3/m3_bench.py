"""M3 peer comparison: the accepted M1 prime-graph library (lib/) against GMP and FLINT on the same exact jobs.

    python3 m3_bench.py --rule          # print the decision rule and exit
    python3 m3_bench.py --write-rule    # fix the rule (dated) in research/m3-rule.md and results/m3.md; once
    python3 m3_bench.py --plan          # list the workloads, rows and invocation count, and exit
    python3 m3_bench.py --quick         # smoke run at 1/50 size, 2 rounds, written to build/quick/ (not results/)
    python3 m3_bench.py                 # the measured run: refuses to start unless research/m3-rule.md holds the
                                        # rule; writes results/m3.md and results/m3.json

Contenders (rows), one process per invocation, every result checked against Python's fractions:

    pg       the M1 library (lib/build/libprimegraph.a, linked as built): its expression API for array jobs
             (pg_expr_begin ... pg_expr_end), single-value calls for the operation-list jobs
    pg-null  the same binary and job again as its own row: the null control
    mpq      GMP mpq_t per element
    fmpq     FLINT fmpq per element; a*b + c in the faster of two call forms (mul + add, or set + addmul), picked per
             job by a calibration run before the rounds
    fz       FLINT fmpz numerators over one shared denominator, where the job's inputs have one that fits 63 bits
             (decimals, money, denominators 1..12, the 90/10 mix, the chains); its reduction of the results is timed

FLINT's fmpq_mat / fmpz_mat are not used: no workload here is matrix-shaped (every job is elementwise, a column sum or
single values), so a matrix form would only re-express the same vector loops.

Times: entry (conversion from the raw int64 inputs), op (the job), exit (every result read out in lowest terms as two
int64), total = entry + op + exit of one rep, minimum over the invocation's warm reps.  Verdicts use total only.
"""
import argparse
import json
import hashlib
import os
import platform
import random
import statistics
import subprocess
import sys
import time
import zlib
from fractions import Fraction
from pathlib import Path

import numpy as np
import sympy

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent                      # 07-prime-graph
LIB = ROOT / "lib"
PGLIB = LIB / "build" / "libprimegraph.a"      # the accepted M1 build (make), linked as is; lib/ is never modified
BUILD = HERE / "build"
BIN = BUILD / "m3_bench"
SRC = HERE / "m3_bench.c"
RULE_FILE = ROOT / "research" / "m3-rule.md"
RESULTS = HERE / "results"
CFLAGS = ["-std=c2x", "-O3", "-march=native", "-D_GNU_SOURCE", "-Wall", "-Wextra", "-Wno-invalid-feature-combination"]
ROUNDS = 12
REPS = 2
BOOT = 10_000
BAND = 0.05
SEED = 20261028
U_GRID = (1, 3, 10, 30, 100)
LIBS = ("mpq", "fmpq", "fz")
I64 = 2**63

ROW_DESC = {
    "pg": "the M1 library, lib/build/libprimegraph.a as built by `make` (gcc, -O3 -march=native): array jobs through "
          "pg_array_from_fracs and pg_array_sum / pg_array_fma inside pg_expr_begin / pg_expr_end (a chain is one "
          "expression, each intermediate released inside it), results read by pg_array_to_fracs / pg_value_get; "
          "single values through pg_value_new, pg_add / pg_mul and pg_value_get; a new graph per rep (created inside "
          "the timed entry, destroyed untimed)",
    "pg-null": "the same binary and job as pg, run as its own row (the null control)",
    "mpq": "GMP mpq_t per element: mpq_set_si + mpq_canonicalize at entry, mpq_add / mpq_mul, mpz_get_si at exit",
    "fmpq": "FLINT fmpq per element: fmpq_set_si at entry (canonicalising), fmpq_add / fmpq_mul, a*b + c in the "
            "calibrated form, fmpz_get_si at exit",
    "fz": "FLINT fmpz numerators over one shared denominator D = lcm of every input denominator (computed from the "
          "data at entry, each distinct small denominator once), numerators scaled to D; a*b + c as (a*b + c*D) / D^2; "
          "chains over D^(k+1); exit reduces each result with fmpq_set_fmpz_frac (timed)",
}

# ---------------------------------------------------------------------------------------------------- the rule

RULE = """\
**Decision rule for M3 (fixed before any measurement: in research/m3-rule.md, and in the header of results/m3.md; the
harness refuses a measured run unless research/m3-rule.md holds this text).**

1. **Rounds.**  Each job runs {rounds} interleaved rounds.  In every round each row of the job (M1, M1's null copy, and
   each library contender) runs once, as its own process, in an order rotated by one row per round, so the rows of a
   round are paired.  A row's time in a round is its **total**: entry (conversion from the raw int64 inputs; for M1
   including creating its graph) + operation + exit (every result read out in lowest terms as two int64), the
   minimum over the invocation's warm reps of one rep's sum.  Entry is reported separately and is included in the total.
2. **Checks.**  Every result of every invocation is compared with the exact value computed by Python's `fractions`:
   the numerator and the positive denominator in lowest terms must both be equal.  A row that fails any check in any
   round, or is unable, is **n/a** for that job.
3. **Ratio.**  For a contender C, the ratio is **T(C) / T(M1)**, T being the median of the row's per-round totals; above
   1, M1 is faster.  Its 95% interval [lo, hi] is the 2.5th-97.5th percentile of the same ratio over {boot} bootstrap
   resamples of the rounds (rounds drawn with replacement, the same draw for both rows, so the pairing is kept; seeded).
4. **Null control.**  M1 against its own second copy on the same job, by 3.  The job's **null spread** is
   max(1 - lo, hi - 1, 0) over that interval; the band half-width is **h = max(5%, null spread)**.
5. **Verdict per job and contender:** **faster** (M1 faster) if lo > 1 + h; **slower** (M1 slower) if hi < 1 - h;
   otherwise **tie** (the interval overlaps the band [1 - h, 1 + h]).  **n/a** if either row is n/a or the null control is.
6. **Best library.**  Per job, the best library is the library row (mpq, fmpq, fz where it runs) that passed every
   check with the lowest T.  The job's verdict is M1's verdict against the best library.
7. **Summary answers.**  (a) **Where is M1 faster than the best library:** the jobs whose verdict is faster.
   (b) **Where is it slower:** the jobs whose verdict is slower.  (Ties and n/a are listed too.)
   (c) **Pay-back of entry:** in each reuse family (R1k, Rpq), the jobs differ only in U, the uses per value (each value
   is an operand of exactly U operations; same number of operations), over the grid U = {grid}.  M1's entry cost **pays
   back from** the smallest U in the grid at which the verdict is faster at that U and at every larger U of the grid;
   it reaches **parity from** the smallest such U with tie or faster.  If the verdict at U = {umax} is not faster
   (resp. tie or faster), the answer is "not within {umax} uses".  An n/a job breaks the run of U's above it.
8. **Calibration.**  Before the rounds, a separate calibration (3 invocations of each form, alternating) picks per job
   the faster FLINT fmpq form of a*b + c (fmpq_mul + fmpq_add, or fmpq_set + fmpq_addmul) by median total.  Its times
   are reported but carry no verdict.  No other comparison carries a verdict.
"""


def rule_text(rounds=ROUNDS, boot=BOOT):
    return (RULE.replace("{rounds}", str(rounds)).replace("{boot}", f"{boot:,}")
            .replace("{grid}", "{" + ", ".join(map(str, U_GRID)) + "}").replace("{umax}", str(U_GRID[-1])))


def header_lines(date, note):
    return ["# M3 peer comparison: M1 against GMP and FLINT", "",
            f"Rule fixed {date} (research/m3-rule.md).  {note}", "", rule_text(), "",
            f"**Scope limit.**  Every input, intermediate and result of every workload fits the M1 bounds (numerators in "
            f"[-2^63, 2^63), denominators below 2^63); the generator checks this for each workload.  GMP and FLINT are "
            f"arbitrary precision and pay for that generality here; beyond 64 bits M1 reports overflow, so these "
            f"results say nothing about larger values.", ""]


def rule_file_text(date):
    W = workloads()
    plan = []
    for wl, w in W.items():
        plan.append(f"- **{wl}** ({w['group']}): {w['desc']}; {'n' if w['kind'] == 'array' else 'operations'} = "
                    f"{w['n']:,}; jobs {', '.join(w['jobs'])}" + (f"; K = {w['K']}" if w.get("K") else "")
                    + f"; rows {', '.join(rows_for(w))}")
    return "\n".join([
        "# M3 peer comparison: pre-registered decision rule", "",
        f"{date} · written by `test/m3/m3_bench.py --write-rule` before any measurement of M3.  The measured run "
        "refuses to start unless this file contains the rule below verbatim.", "",
        rule_text(), "",
        "## Run plan", "",
        f"{ROUNDS} rounds, {REPS} warm reps per invocation, seed {SEED}, {BOOT:,} bootstrap resamples, band floor "
        f"{BAND:.0%}.  Rows: {', '.join(ROW_DESC)} (fz only where the inputs have a shared denominator below 2^63).  "
        "Workloads:", ""] + plan + [""])


# ---------------------------------------------------------------------------------------------------- workloads

def wrng(seed, name):
    """Each workload's own generator, so the data do not depend on --only or on the order of the workloads."""
    return np.random.default_rng([seed, zlib.crc32(name.encode())])


def ints(rng, lo, hi, n):
    return rng.integers(lo, hi, size=n, dtype=np.int64)


def dec(rng, n, lo, hi, places=3):
    """Decimals with `places` places in (lo, hi), entered as k / 10^places."""
    s = 10**places
    return ints(rng, lo * s + 1, hi * s, n), np.full(n, s, dtype=np.int64)


def small(rng, n, dmax, nmax=10**6):
    return ints(rng, -nmax + 1, nmax, n), ints(rng, 1, dmax + 1, n)


def mix(rng, n, p1000, dmax):
    """A share p1000 of three-place decimals in (-10^4, 10^4) over 1000; the rest |num| < 10^6 over 1..dmax."""
    one = rng.random(n) < p1000
    num = np.where(one, ints(rng, -10**7 + 1, 10**7, n), ints(rng, -10**6 + 1, 10**6, n))
    den = np.where(one, 1000, ints(rng, 1, dmax + 1, n))
    return num.astype(np.int64), den.astype(np.int64)


def chain_dec(rng, n):
    """x0, c three-place decimals in (-10, 10); b tenths in (-2, 2)."""
    return {"a": dec(rng, n, -10, 10), "b": (ints(rng, -19, 20, n), np.full(n, 10, dtype=np.int64)),
            "c": dec(rng, n, -10, 10)}


def chain_mix(rng, n):
    """x0, c: |num| < 10^4 over 1..12; b over 1..4 with |b| < 2 (denominators grow by up to 4 per step)."""
    bd = ints(rng, 1, 5, n)
    bn = np.floor((rng.random(n) * 2 - 1) * (2 * bd - 1) + 0.5).astype(np.int64)
    return {"a": small(rng, n, 12, 10**4), "b": (bn, bd), "c": small(rng, n, 12, 10**4)}


def distinct(rng, k, lo, hi):
    out, seen = [], set()
    while len(out) < k:
        for x in rng.integers(lo, hi, size=k).tolist():
            if x not in seen and len(out) < k:
                seen.add(x)
                out.append(x)
    return out


def shared_den_fma(rng, n, dens, bmax_den=12, bmax_num=100):
    """a over the given denominators (each used at least once), c over a's denominator, b small: a*b + c."""
    dens = np.asarray(dens, dtype=np.int64)
    pick = rng.integers(0, len(dens), size=n)
    pick[:min(n, len(dens))] = rng.permutation(len(dens))[:n]
    da = dens[pick]
    return {"a": (ints(rng, -10**6 + 1, 10**6, n), da),
            "b": (ints(rng, -bmax_num + 1, bmax_num, n), ints(rng, 1, bmax_den + 1, n)),
            "c": (ints(rng, -10**6 + 1, 10**6, n), da.copy())}


def sweep(rng, n, N):
    """N distinct denominators: half uniform in [2, 2^31), half in [2^32, 2^40) (round three's node-count sweep)."""
    return shared_den_fma(rng, n, distinct(rng, N // 2, 2, 2**31) + distinct(rng, N - N // 2, 2**32, 2**40))


def prime_pool(rng, k, lo, hi):
    ps = set()
    while len(ps) < k:
        ps.add(int(sympy.nextprime(int(rng.integers(lo, hi)))))
    return sorted(ps)


def pq_dens(rng, pool, M, lo, hi):
    """M distinct products p*q (p != q) of primes in [lo, hi) from a pool of `pool` primes; pool >= 2M: disjoint pairs
    (no prime shared), else each prime is in about 2M / pool denominators."""
    ps = prime_pool(rng, pool, lo, hi)
    if 2 * M <= pool:
        perm = rng.permutation(pool)
        return [ps[perm[2 * i]] * ps[perm[2 * i + 1]] for i in range(M)]
    dens = set()
    while len(dens) < M:
        i, j = rng.choice(pool, size=2, replace=False)
        dens.add(ps[i] * ps[j])
    return sorted(dens)


def two_primes(rng, n, pool, M=2000):
    """a over M distinct p*q, primes in [2^16, 2^30) (32-60 bits), c over a's denominator, b over 1..4 with |num| < 8:
    results up to 62 bits."""
    return shared_den_fma(rng, n, pq_dens(rng, pool, min(M, n), 2**16, 2**30), bmax_den=4, bmax_num=8)


def pairs(rng, n, bits, shared_pool=0):
    """Single-value pairs (a_i, b_i), |num| < 2^bits, denominators in [2^(bits-1), 2^bits): independent, or (shared_pool
    > 0) b_i over a_i's denominator, drawn from a pool of that many."""
    lo, hi = 2**(bits - 1), 2**bits
    if shared_pool:
        pool = np.asarray(distinct(rng, shared_pool, lo, hi), dtype=np.int64)
        da = pool[rng.integers(0, shared_pool, size=n)]
        db = da.copy()
    else:
        da, db = ints(rng, lo, hi, n), ints(rng, lo, hi, n)
    num = np.concatenate([ints(rng, -hi + 1, hi, n), ints(rng, -hi + 1, hi, n)])
    v = (num, np.concatenate([da, db]))
    ia = np.arange(n, dtype=np.int32)
    ib = ia + np.int32(n)
    return {"v": v, "ops": {"add": (ia, ib, np.zeros(n, np.int32)), "mul": (ia, ib, np.ones(n, np.int32))}}


def reuse(rng, T, U, family, pool=None):
    """T operations over 2T/U values, each value an operand of exactly U of them.  R1k: |num| < 10^6 over 1..1000,
    add and mul alternately, partners at random.  Rpq: |num| < 10^6 over p*q from `pool` (32-62 bits), add only,
    partners over the same denominator (values come in pairs over one denominator)."""
    n = 2 * T // U
    slots = np.repeat(np.arange(n, dtype=np.int32), U)
    if family == "R1k":
        v = small(rng, n, 1000)
        slots = slots[rng.permutation(len(slots))]
        op = (np.arange(T) % 2).astype(np.int32)
    else:
        dpair = np.asarray(pool, dtype=np.int64)[rng.integers(0, len(pool), size=n // 2)]
        den = np.repeat(dpair, 2)
        v = (ints(rng, -10**6 + 1, 10**6, n), den)
        order = np.lexsort((rng.random(len(slots)), den[slots]))  # grouped by denominator, shuffled inside groups
        slots = slots[order]
        op = np.zeros(T, np.int32)
    ia, ib = slots[0::2].copy(), slots[1::2].copy()
    perm = rng.permutation(T)
    return {"v": v, "ops": {"mix" if family == "R1k" else "add": (ia[perm], ib[perm], op)}}


def workloads(scale=1.0, seed=SEED):
    """name -> spec, in report order.  n is the array length (array jobs) or the number of operations (ops jobs)."""
    s = lambda n: max(int(n * scale), 1000)
    T = max(300, int(300_000 * scale) // 300 * 300)  # divisible by every U of the grid
    W = {}

    def add(name, group, kind, desc, n, gen, jobs, fz=False, **extra):
        W[name] = dict(group=group, kind=kind, desc=desc, n=n, gen=gen, jobs=jobs, fz=fz, **extra)

    add("DEC", "decimal columns", "array", "three-place decimals in (-10^4, 10^4)", s(1_000_000),
        lambda r, n: {k: dec(r, n, -10_000, 10_000) for k in "abc"}, ["sum", "fma"], fz=True)
    add("CH-dec", "decimal columns", "array", "expression chain x <- x*b + c, K = 8: x0, c three-place decimals in "
        "(-10, 10), b tenths in (-2, 2)", s(500_000), chain_dec, ["chain"], fz=True, K=8)
    add("MONEY", "money", "array", "two-place amounts in (-10^5, 10^5)", s(1_000_000),
        lambda r, n: {k: dec(r, n, -100_000, 100_000, 2) for k in "abc"}, ["sum", "fma"], fz=True)
    add("MIX12", "mixed small", "array", "|num| < 10^6 over denominators 1..12", s(1_000_000),
        lambda r, n: {k: small(r, n, 12) for k in "abc"}, ["sum", "fma"], fz=True)
    add("MIX90", "mixed 90/10", "array", "90% three-place decimals in (-10^4, 10^4) over 1000, 10% |num| < 10^6 over "
        "1..12", s(1_000_000), lambda r, n: {k: mix(r, n, 0.90, 12) for k in "abc"}, ["sum", "fma"], fz=True)
    add("U1k", "unrelated", "array", "|num| < 10^6 over denominators 1..1000 (no column sum: its common denominator "
        "is far beyond 64 bits)", s(200_000), lambda r, n: {k: small(r, n, 1000) for k in "abc"}, ["fma"])
    for b in (8, 16, 24, 30):
        add(f"S{b}", "single values", "ops", f"independent pairs, |num| < 2^{b}, denominators in [2^{b - 1}, 2^{b})",
            s(500_000), (lambda b: lambda r, n: pairs(r, n, b))(b), ["add", "mul"])
    add("SH30", "single values", "ops", "pairs over one shared denominator: |num| < 2^30, denominators from a pool of "
        "1,000 in [2^29, 2^30)", s(500_000), lambda r, n: pairs(r, n, 30, 1000), ["add", "mul"])
    add("P-sh", "two large primes", "array", "a*b + c, a over 2,000 distinct p*q (primes in [2^16, 2^30): 32-60 bits) "
        "from a pool of 64 primes (each prime in about 62 denominators), c over a's denominator, b over 1..4",
        s(200_000), lambda r, n: two_primes(r, n, 64), ["fma"])
    add("P-ind", "two large primes", "array", "as P-sh, the 2,000 p*q from 4,000 primes in disjoint pairs (no prime "
        "shared)", s(200_000), lambda r, n: two_primes(r, n, 4000), ["fma"])
    add("N100", "many denominators", "array", "a*b + c, a over 100 distinct denominators (half in [2, 2^31), half in "
        "[2^32, 2^40)), c over a's denominator, b over 1..12", s(200_000), lambda r, n: sweep(r, n, 100), ["fma"])
    add("N10k", "many denominators", "array", "as N100 with 10,000 distinct denominators", s(200_000),
        lambda r, n: sweep(r, n, min(10_000, n)), ["fma"])
    for K in (4, 8):
        add(f"G-mix{K}", "growth chains", "array", f"growth chain x <- x*b + c, K = {K}: x0, c |num| < 10^4 over 1..12, "
            "b over 1..4 with |b| < 2 (denominators grow up to 4x per step)", s(500_000), chain_mix, ["chain"],
            fz=True, K=K)
    pq_pool = pq_dens(wrng(seed, "Rpq-pool"), 64, 2000, 2**16, 2**31)
    for fam, desc in (("R1k", "|num| < 10^6 over 1..1000; add and mul alternately, partners at random"),
                      ("Rpq", "|num| < 10^6 over 2,000 p*q (primes in [2^16, 2^31) from a pool of 64: 32-62 bits); "
                              "add of two values over the same denominator")):
        for U in U_GRID:
            add(f"{fam}-U{U}", "reuse", "ops", f"{T:,} operations over {2 * T // U:,} values, each value an operand of "
                f"{U} operation{'s' if U > 1 else ''} ({'entry-heavy' if U == 1 else 'reuse-heavy'}): {desc}", T,
                (lambda fam, U: lambda r, n: reuse(r, n, U, fam, pq_pool))(fam, U),
                ["mix" if fam == "R1k" else "add"], family=fam, U=U)
    return W


def rows_for(w):
    return ["pg", "pg-null", "mpq", "fmpq"] + (["fz"] if w["fz"] and w["kind"] == "array" else [])


# ---------------------------------------------------------------------------------------------------- expected values

def fits(f):
    return -I64 <= f.numerator < I64 and f.denominator < I64


def fracs(arr):
    num, den = arr
    return [Fraction(a, b) for a, b in zip(num.tolist(), den.tolist())]


def to_np(fs, what):
    """Exact fractions -> (num, den) int64 arrays; raises if one is beyond the M1 bounds (a workload design error)."""
    for i, f in enumerate(fs):
        if not fits(f):
            raise ValueError(f"{what}: element {i} = {f} is beyond the M1 bounds")
    return (np.array([f.numerator for f in fs], dtype=np.int64), np.array([f.denominator for f in fs], dtype=np.int64))


def expected(job, data, K=0):
    """The exact results by Python's fractions, with every intermediate checked against the M1 bounds."""
    if job in ("add", "mul", "mix"):
        v = fracs(data["v"])
        ia, ib, op = data["ops"][job]
        return to_np([v[i] * v[j] if o else v[i] + v[j] for i, j, o in zip(ia.tolist(), ib.tolist(), op.tolist())], job)
    a = fracs(data["a"])
    if job == "sum":
        return to_np([sum(a, Fraction(0))], job)
    b, c = fracs(data["b"]), fracs(data["c"])
    x = a
    for _ in range(1 if job == "fma" else K):
        p = [u * v for u, v in zip(x, b)]
        to_np(p, f"{job}: a product")
        x = [u + w for u, w in zip(p, c)]
        to_np(x, f"{job}: a step")
    return to_np(x, job)


def check_inputs(data):
    for k, arr in data.items():
        if k == "ops":
            continue
        num, den = arr
        if not ((den > 0).all() and len(num) == len(den)):
            raise ValueError(f"input {k}: a denominator is not positive")


def write_inputs(d, data):
    d.mkdir(parents=True, exist_ok=True)
    for k, arr in data.items():
        if k == "ops":
            for name, (ia, ib, op) in arr.items():
                with open(d / f"ops-{name}.bin", "wb") as f:
                    f.write(np.int64(len(ia)).tobytes())
                    for x in (ia, ib, op):
                        f.write(np.ascontiguousarray(x, dtype=np.int32).tobytes())
            continue
        num, den = arr
        with open(d / f"{k}.bin", "wb") as f:
            f.write(np.int64(len(num)).tobytes())
            f.write(np.ascontiguousarray(num, dtype=np.int64).tobytes())
            f.write(np.ascontiguousarray(den, dtype=np.int64).tobytes())


# ---------------------------------------------------------------------------------------------------- build and run

def flint_root():
    for r in (os.environ.get("FLINT_ROOT"), "/home/claude/flintblas/inst-native", "/home/claude/flintblas/inst"):
        if r and Path(r, "lib").exists():
            return r
    raise SystemExit("no FLINT install found")


def lib_stale():
    """Sources of lib/ newer than its static library (the harness never rebuilds lib/)."""
    t = PGLIB.stat().st_mtime
    return [str(p.relative_to(LIB)) for p in list((LIB / "src").glob("*")) + [LIB / "include" / "primegraph.h"]
            if p.stat().st_mtime > t]


def build(force=False):
    BUILD.mkdir(parents=True, exist_ok=True)
    if not PGLIB.exists():
        raise SystemExit(f"{PGLIB} is missing: build it in lib/ with make (this harness does not modify lib/)")
    deps = [SRC, PGLIB, LIB / "include" / "primegraph.h"]
    if force or not BIN.exists() or BIN.stat().st_mtime < max(d.stat().st_mtime for d in deps):
        root = flint_root()
        subprocess.run(["clang", *CFLAGS, f"-I{LIB / 'include'}", f"-I{root}/include", "-o", str(BIN), str(SRC),
                        str(PGLIB), f"-L{root}/lib", "-lflint", "-lgmp", "-lpthread", f"-Wl,-rpath,{root}/lib"],
                       check=True)
    return BIN


def parse_stats(text):
    out = {}
    for t in text.split():
        k, v = t.split("=", 1)
        try:
            out[k] = int(v)
        except ValueError:
            out[k] = v
    return out


def contender_of(row, calib, key):
    if row.startswith("pg"):
        return "pg"
    if row == "fmpq":
        return calib.get(key, "fmpq")
    return row


def run_one(con, job, d, reps, extra, out, env=None):
    """One invocation; returns its statistics, or {"unable": why}."""
    job_arg = "ops" if job in ("add", "mul", "mix") else job
    args = [str(BIN), con, job_arg, str(d), str(out), str(reps)]
    if job_arg == "ops":
        args.append(job)
    elif job == "chain":
        args.append(str(extra))
    r = subprocess.run(args, capture_output=True, text=True, timeout=7200, env=env)
    if r.returncode == 3:
        return {"unable": r.stdout.strip().removeprefix("unable ")}
    if r.returncode != 0:
        raise RuntimeError(f"{con} {job} failed ({r.returncode}): {r.stderr.strip()}")
    return parse_stats(r.stdout)


def read_out(path):
    raw = np.fromfile(path, dtype=np.int64)
    n = int(raw[0])
    if len(raw) != 1 + 2 * n:
        raise ValueError(f"{path}: malformed output ({len(raw)} words for {n} results)")
    return raw[1:1 + n], raw[1 + n:]


def check(got, exp):
    """(ok, message).  Every element: the numerator and the positive denominator must equal the expected value in
    lowest terms.  The message names the first wrong element and why: no result (denominator 0: an error, or a value
    beyond 64 bits), a nonpositive denominator, a right value not in lowest terms, or a wrong value."""
    gn, gd = got
    en, ed = exp
    if len(gn) != len(en) or len(gd) != len(ed):
        return False, f"{len(gn):,} results, expected {len(en):,}"
    bad = np.flatnonzero((gn != en) | (gd != ed))
    if not len(bad):
        return True, f"{len(en):,} checked"
    i = int(bad[0])
    p, q, e = int(gn[i]), int(gd[i]), Fraction(int(en[i]), int(ed[i]))
    if q == 0:
        why = "no result (an error, or beyond 64 bits)"
    elif q < 0:
        why = "nonpositive denominator"
    elif Fraction(p, q) == e:
        why = "right value, not in lowest terms"
    else:
        why = "wrong value"
    return False, f"{len(bad):,} wrong; first: element {i}: got {p}/{q}, expected {e} ({why})"


def judge(st, got, exp):
    """One invocation: its results (check) and its run conditions (FLINT single-threaded)."""
    if st.get("flint_threads") != 1:
        return False, f"FLINT ran with {st.get('flint_threads')} threads"
    return check(got, exp)


# ---------------------------------------------------------------------------------------------------- statistics

def ratio_ci(tx, ty, rng, boot=BOOT):
    """T(Y) / T(X) with T the median over rounds, and its 95% paired bootstrap interval (rounds resampled)."""
    tx, ty = np.asarray(tx, float), np.asarray(ty, float)
    k = min(len(tx), len(ty))
    tx, ty = tx[:k], ty[:k]
    point = float(np.median(ty) / np.median(tx))
    idx = rng.integers(0, k, size=(boot, k))
    r = np.median(ty[idx], axis=1) / np.median(tx[idx], axis=1)
    lo, hi = np.percentile(r, [2.5, 97.5])
    return point, float(lo), float(hi)


def verdict(lo, hi, h):
    """Rule 5: M1 faster / slower / tie, from the interval of T(contender) / T(M1) and the band half-width h."""
    if lo > 1 + h:
        return "faster"
    if hi < 1 - h:
        return "slower"
    return "tie"


def ok_row(cr):
    return cr is not None and "unable" not in cr and cr.get("ok") and cr.get("rounds")


def totals(cr):
    return [s["total"] for s in cr["rounds"]]


def med(cr, k):
    v = [s[k] for s in cr.get("rounds", [])]
    return statistics.median(v) if v else 0


def analyse(res, seed, boot=BOOT):
    """Per job: the null interval and band, every library against M1, the best library and the job's verdict."""
    rng = np.random.default_rng(seed)
    out = {}
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            a = {"wl": wl, "job": job, "cmp": {}}
            pg, null = jr.get("pg"), jr.get("pg-null")
            if ok_row(pg) and ok_row(null):
                p, lo, hi = ratio_ci(totals(pg), totals(null), rng, boot)
                a["null"] = {"point": p, "lo": lo, "hi": hi, "spread": max(1 - lo, hi - 1, 0.0)}
                a["h"] = max(BAND, a["null"]["spread"])
            else:
                a["null"], a["h"] = None, None
            for lib in LIBS:
                if lib not in jr:
                    continue
                cr = jr[lib]
                c = {}
                if not (ok_row(pg) and ok_row(cr)):
                    c["verdict"] = "n/a"
                    c["why"] = "unable" if "unable" in cr or (pg and "unable" in pg) else "failed a check"
                else:
                    c["point"], c["lo"], c["hi"] = ratio_ci(totals(pg), totals(cr), rng, boot)
                    if a["h"] is None:
                        c["verdict"], c["why"] = "n/a", "no null control"
                    else:
                        c["verdict"] = verdict(c["lo"], c["hi"], a["h"])
                a["cmp"][lib] = c
            passing = [lib for lib in a["cmp"] if ok_row(jr[lib])]
            a["best"] = min(passing, key=lambda lib: med(jr[lib], "total")) if passing else None
            a["verdict"] = a["cmp"][a["best"]]["verdict"] if a["best"] and ok_row(pg) else "n/a"
            out[f"{wl} {job}"] = a
    return out


def payback(entries):
    """Rule 7c over [(U, verdict)] sorted by U: (pays back from, parity from), each the smallest U from which every
    larger U of the grid satisfies it, or None ("not within the grid")."""
    def from_(good):
        first = None
        for U, v in reversed(entries):
            if not good(v):
                break
            first = U
        return first
    return from_(lambda v: v == "faster"), from_(lambda v: v in ("faster", "tie"))


def answers(res, an):
    keys = list(an)
    by = lambda v: [k for k in keys if an[k]["verdict"] == v]
    out = {"faster": by("faster"), "slower": by("slower"), "tie": by("tie"), "n/a": by("n/a"), "payback": {}}
    for fam in ("R1k", "Rpq"):
        ent = sorted((res["workloads"][a["wl"]]["U"], a["verdict"]) for a in an.values()
                     if res["workloads"][a["wl"]].get("family") == fam)
        if ent:
            f, p = payback(ent)
            out["payback"][fam] = {"entries": ent, "faster_from": f, "parity_from": p,
                                   "complete": [U for U, _ in ent] == list(U_GRID)}
    return out


# ---------------------------------------------------------------------------------------------------- report

def ms(ns):
    return f"{ns / 1e6:.2f}"


def fmt_ci(c):
    if "point" not in c:
        return c.get("why", c["verdict"])
    return f"{c['point']:.2f} [{c['lo']:.2f}, {c['hi']:.2f}]"


def answers_md(res, an):
    A = answers(res, an)
    lst = lambda xs: ", ".join(xs) if xs else "none"
    L = ["## Answers (rule 7)", "",
         f"Over {len(an)} jobs.  M1 against the best library (rule 6):", "",
         f"- **(a) M1 faster:** {lst(A['faster'])}.",
         f"- **(b) M1 slower:** {lst(A['slower'])}.",
         f"- Tie: {lst(A['tie'])}.",
         f"- n/a: {lst(A['n/a'])}.", "",
         "**(c) Pay-back of M1's entry cost** (uses per value U; verdict against the best library at each U):", "",
         "| family | " + " | ".join(f"U = {U}" for U in U_GRID) + " | pays back from | parity from |",
         "|---|" + "---|" * len(U_GRID) + "---|---|"]
    for fam, p in A["payback"].items():
        v = dict(p["entries"])
        nf = lambda x: f"U = {x}" if x is not None else f"not within {U_GRID[-1]} uses"
        L.append(f"| {fam} | " + " | ".join(v.get(U, "not run") for U in U_GRID) + f" | **{nf(p['faster_from'])}** | "
                 f"{nf(p['parity_from'])}" + ("" if p["complete"] else " (grid incomplete)") + " |")
    return L + [""]


def report_md(res, cfg, an):
    W = res["workloads"]
    L = header_lines(cfg["rule_date"], f"This run started at {cfg['started']} and measured first at "
                     f"{cfg['first_measurement']}; it ended at {cfg['date']}.")
    L += [f"Machine: {cfg['cpu']}, {cfg['ncpu']} CPUs; load average {cfg['load_start']} at the start, per-round values "
          f"in the JSON (max 1-minute load during the rounds: {cfg['load_max']:.2f}).  Driver: clang 18 "
          f"`{' '.join(CFLAGS[:4])}`.  M1: `lib/build/libprimegraph.a` ({cfg['lib_compiler']}, sha256 "
          f"{cfg['lib_sha256'][:16]}...).  GMP {cfg['gmp']}; FLINT {cfg['flint']} ({cfg['flint_root']}), "
          f"**FLINT threads: {cfg['flint_threads']}** (flint_set_num_threads(1) in every invocation, and read back).  "
          f"{cfg['rounds']} rounds, {cfg['reps']} warm reps per invocation, seed {cfg['seed']}, {cfg['boot']:,} "
          f"bootstrap resamples.", ""]
    if cfg.get("quick"):
        L += [f"**Smoke run (scale {cfg['scale']}, {cfg['rounds']} rounds): not a measurement; verdicts are "
              "illustrative only.**", ""]
    L += ["## Rows", ""] + [f"- **{r}**: {d}" for r, d in ROW_DESC.items()] + [
        "", "FLINT's fmpq_mat / fmpz_mat (optional contender 5) are not used: no workload is matrix-shaped.", ""]
    if res.get("calibration"):
        L += ["FLINT a*b + c form, picked by the calibration (median total ms; not reported results): "
              + "; ".join(f"{k}: mul+add {ms(v['fmpq'])}, set+addmul {ms(v['fmpq-am'])} -> {v['chosen']}"
                          for k, v in res["calibration"].items()), ""]
    L += answers_md(res, an)
    nulls = [a["null"] for a in an.values() if a.get("null")]
    L += ["## Noise", ""]
    if nulls:
        sp = [x["spread"] for x in nulls]
        L += [f"Null control (M1 against its own second copy) over {len(nulls)} jobs: point ratios "
              f"{min(x['point'] for x in nulls):.2f} to {max(x['point'] for x in nulls):.2f}; null spread up to "
              f"{max(sp):.1%}, above the {BAND:.0%} floor on {sum(s > BAND for s in sp)} jobs (their band is wider).", ""]
    L += ["## Verdicts per job", "",
          "Ratios are T(contender) / T(M1) on total time with the 95% interval; above 1, M1 is faster.  h is the band "
          "half-width (rule 4).  The verdict is against the best library (rule 6).", "",
          "| job | M1 total ms | M1 entry ms | best library | best / M1 [95%] | h | verdict | mpq / M1 | fmpq / M1 | "
          "fz / M1 |", "|---|---:|---:|---|---|---:|---|---|---|---|"]
    for k, a in an.items():
        jr = W[a["wl"]]["jobs"][a["job"]]
        pg = jr.get("pg", {})
        best = a["best"]
        cell = lambda lib: (fmt_ci(a["cmp"][lib]) + ("" if "point" not in a["cmp"][lib] else f" {a['cmp'][lib]['verdict']}")
                            if lib in a["cmp"] else "-")
        L.append(f"| {k} | {ms(med(pg, 'total')) if ok_row(pg) else 'n/a'} | {ms(med(pg, 'entry')) if ok_row(pg) else ''} | "
                 f"{best or 'none'} | {fmt_ci(a['cmp'][best]) if best else ''} | "
                 f"{'' if a['h'] is None else format(a['h'], '.1%')} | **{a['verdict']}** | {cell('mpq')} | {cell('fmpq')} | "
                 f"{cell('fz')} |")
    # reuse families: phases
    L += ["", "## Entry against reuse", "",
          "Medians over rounds, ms: entry / op / exit / total.  Every job of a family has the same number of operations; "
          "U is the uses per value.", "",
          "| job | M1 | mpq | fmpq | M1 entry share | best / M1 [95%] | verdict |", "|---|---|---|---|---:|---|---|"]
    for k, a in an.items():
        w = W[a["wl"]]
        if not w.get("family"):
            continue
        jr = w["jobs"][a["job"]]
        ph = lambda r: (" / ".join(ms(med(jr[r], x)) for x in ("entry", "op", "exit", "total")) if ok_row(jr.get(r)) else "n/a")
        pg = jr.get("pg")
        share = f"{med(pg, 'entry') / med(pg, 'total'):.0%}" if ok_row(pg) and med(pg, "total") else ""
        L.append(f"| {k} | {ph('pg')} | {ph('mpq')} | {ph('fmpq')} | {share} | "
                 f"{fmt_ci(a['cmp'][a['best']]) if a['best'] else ''} | {a['verdict']} |")
    L += ["", "## Workloads", ""] + [
        f"- **{wl}** ({w['group']}): {w['desc']}; {'n' if w['kind'] == 'array' else 'operations'} = {w['n']:,}"
        + (f", K = {w['K']}" if w.get("K") else "") + f"; jobs {', '.join(w['jobs'])}" for wl, w in W.items()] + [""]
    L += ["## Detail", "",
          "Medians over rounds, ms.  Phase columns are each the median of per-invocation minima, so they need not add "
          "up to total.  x / M1 is the row's median total over M1's.", ""]
    for wl, w in W.items():
        for job, jr in w["jobs"].items():
            L += [f"### {wl} {job}", "", "| row | entry | op | exit | total | x / M1 | check | notes |",
                  "|---|---:|---:|---:|---:|---:|---|---|"]
            pg = jr.get("pg")
            for r, cr in jr.items():
                if "unable" in cr:
                    L.append(f"| {r} | | | | | | unable: {cr['unable']} | |")
                    continue
                last = cr["rounds"][-1] if cr["rounds"] else {}
                notes = ""
                if r.startswith("pg") and last:
                    notes = (f"nodes {last.get('nodes', 0):,}; rho {last.get('rho', 0):,}; exc out "
                             f"{last.get('exc_out', 0):,}; D {last.get('D', 0)}; V4 retries {last.get('v4_retries', 0):,}; "
                             f"peak {last.get('mem_peak', 0) / 2**20:.1f} MiB")
                elif r == "fz":
                    notes = f"D: {last.get('Dbits', '?')} bits"
                elif r == "fmpq":
                    notes = cr.get("form", "")
                rr = f"{med(cr, 'total') / med(pg, 'total'):.2f}" if ok_row(pg) and ok_row(cr) and med(pg, "total") else "n/a"
                L.append(f"| {r} | {ms(med(cr, 'entry'))} | {ms(med(cr, 'op'))} | {ms(med(cr, 'exit'))} | "
                         f"{ms(med(cr, 'total'))} | {rr} | {'ok' if cr['ok'] else 'FAIL: ' + cr['msg']} | {notes} |")
            L.append("")
    L += ["## Simplifications that may bias the results", ""] + [f"- {b}" for b in BIASES] + [""]
    return "\n".join(L)


BIASES = [
    "Scope: every value fits 64 bits by construction (checked by the generator).  GMP and FLINT handle any size and "
    "carry that generality's cost (and FLINT's fmpz keeps values below 2^62 inline, so most of these values never "
    "allocate in FLINT, while GMP's always use limbs).  Nothing here measures M1 beyond its bounds, where it reports "
    "overflow.",
    "Allocation: GMP and FLINT objects are initialised once before the reps and reused (as a caller keeping its "
    "vectors would), so their timed loops never allocate a fresh object; M1 creates a new graph in every rep's timed "
    "entry (with its 576 KiB factor cache) and allocates its arrays and result values inside the timed phases.  "
    "malloc keeps freed memory for every contender (mallopt), so later reps reuse pages.  Releases, mpq_clear and "
    "pg_graph_destroy are not timed.  This favours the libraries.",
    "Single-value jobs: M1 makes every result a new value (a holder linked into its nodes); GMP and FLINT write each "
    "result into a preallocated slot.  The operation-list indirection (operands by index) is the same for all.",
    "Best library: the minimum over two or three library medians is biased low (a lucky library row is chosen more "
    "often), which favours the libraries in the job verdict; each library's own interval is in the table.",
    "FLINT fmpq's a*b + c form is calibrated per job; GMP's mpq has one form (mpq_mul + mpq_add).  The fz contender "
    "knows the job's shape: it computes the lcm of the inputs' denominators at entry (each distinct denominator below "
    "2^16 once) and runs only where that lcm is below 2^63; it reduces every result at exit (timed).",
    "M1 runs every array job through its expression API (V4: deferred reduction, one reduction at pg_expr_end), as "
    "asked.  The library's README measures V4 slower than per-operation reduction (V3) on data like N100, where many "
    "elements are exceptions; V3 is not run here.",
    "Compilers: M1 is linked as `make` built it (gcc 13.3, -O3 -march=native); the driver and its inline loops are "
    "clang 18 -O3 -march=native; GMP is the distribution's build (CPU-dispatched assembly); FLINT is the native "
    "rebuild in /home/claude/flintblas/inst-native.  Different compilers can move a library's times by a few percent.",
    "Warm processes: the reps of an invocation run in one process with warm caches for every contender, and the "
    "minimum over reps is kept; reading the inputs and writing the outputs are not timed.  M1's factor cache and "
    "dictionary start empty in every rep (a new graph), so no rep benefits from an earlier one's factoring.",
    "Checks compare every result of every invocation (all rounds) with Python's fractions, as int64 pairs; a "
    "contender's output passes only if equal in lowest terms with a positive denominator.",
    "Statistics: the bootstrap resamples rounds, so it captures round-to-round noise, not effects shared by every "
    "round (machine, compiler, data).  The null band comes from M1's row only, so a library row much noisier than M1 "
    "could be called faster or slower where a wider band would say tie.  About 100 comparisons are made without a "
    "multiple-comparison correction.  With 12 rounds the median moves in discrete steps.",
    "The machine has 2 CPUs and other work may run on it (another agent tests in test/fault/); the 1-minute load "
    "average is recorded at the start of each round.  Rows of a round are paired and interleaved, so slow-downs "
    "that last several invocations hit all rows of a job alike, but short bursts do not.",
    "Workloads are synthetic: uniform random numerators and denominators from the stated sets.  Reuse jobs pair "
    "values at random (R1k) or within a denominator (Rpq); operations of R1k alternate add and mul.  U counts operand "
    "uses, so U = 1 enters two values per operation.",
]


# ---------------------------------------------------------------------------------------------------- main

def load_avg():
    try:
        return float(Path("/proc/loadavg").read_text().split()[0])
    except OSError:
        return -1.0


def calibrate(P, prep, reps, n_each, calib_out):
    """Rule 8: the faster fmpq form of a*b + c per job (fma and chain), by median total over n_each invocations each."""
    choice = {}
    for wl, w, jobs in P:
        for job in jobs:
            if job not in ("fma", "chain") or "fmpq" not in jobs[job]:
                continue
            d = prep[wl]["dir"]
            t = {"fmpq": [], "fmpq-am": []}
            for i in range(n_each):
                for f in (("fmpq", "fmpq-am") if i % 2 == 0 else ("fmpq-am", "fmpq")):
                    o = d / f"calib-{f}-{job}.bin"
                    st = run_one(f, job, d, reps, w.get("K", 0), o)
                    ok, msg = judge(st, read_out(o), prep[wl]["exp"][job])
                    o.unlink()
                    if not ok:
                        raise RuntimeError(f"calibration {wl} {job} {f}: {msg}")
                    t[f].append(st["total"])
            m = {f: statistics.median(v) for f, v in t.items()}
            chosen = min(m, key=m.get)
            choice[f"{wl} {job}"] = chosen
            calib_out[f"{wl} {job}"] = {**m, "chosen": "set+addmul" if chosen == "fmpq-am" else "mul+add"}
    return choice


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rounds", type=int, default=None, help=f"default {ROUNDS} (quick: 2)")
    ap.add_argument("--reps", type=int, default=REPS)
    ap.add_argument("--boot", type=int, default=BOOT)
    ap.add_argument("--seed", type=int, default=SEED)
    ap.add_argument("--quick", action="store_true", help="scale 0.02 and 2 rounds; output to build/quick/ unless --out")
    ap.add_argument("--scale", type=float, help="size factor (not with a measured run)")
    ap.add_argument("--out", type=Path, help="write here instead (not a measured run)")
    ap.add_argument("--only", help="comma-separated workloads, e.g. DEC,R1k-U1")
    ap.add_argument("--rule", action="store_true", help="print the decision rule and exit")
    ap.add_argument("--write-rule", action="store_true", help=f"fix the rule in {RULE_FILE} and results/m3.md")
    ap.add_argument("--plan", action="store_true", help="list workloads, rows and invocations, and exit")
    ap.add_argument("--force-build", action="store_true")
    a = ap.parse_args(argv)
    scale = a.scale if a.scale is not None else (0.02 if a.quick else 1.0)
    rounds = a.rounds if a.rounds is not None else (2 if a.quick else ROUNDS)
    if a.rule:
        print(rule_text())
        return 0
    if a.write_rule:
        if RULE_FILE.exists():
            print(f"{RULE_FILE} exists; not overwritten")
            return 0 if rule_text() in RULE_FILE.read_text() else 1
        date = time.strftime("%b %d, %Y %H:%M %Z").strip()
        RULE_FILE.write_text(rule_file_text(date))
        RESULTS.mkdir(parents=True, exist_ok=True)
        (RESULTS / "m3.md").write_text("\n".join(header_lines(date, "The measured run has not started; this file "
                                                              "holds the rule only.")) + "\n")
        print(f"wrote {RULE_FILE} and {RESULTS / 'm3.md'}")
        return 0
    W = workloads(scale, a.seed)
    sel = set(a.only.split(",")) if a.only else set(W)
    unknown = sel - set(W)
    if unknown:
        raise SystemExit(f"unknown workloads: {', '.join(sorted(unknown))}")
    P = [(wl, w, {job: rows_for(w) for job in w["jobs"]}) for wl, w in W.items() if wl in sel]
    if a.plan:
        inv = sum(len(r) for _, _, jobs in P for r in jobs.values())
        for wl, w, jobs in P:
            print(f"{wl:10s} {w['kind']:5s} n={w['n']:>9,}  jobs {','.join(jobs)}  rows {','.join(rows_for(w))}")
        print(f"{len(P)} workloads, {sum(len(j) for _, _, j in P)} jobs, {inv} invocations per round, "
              f"{inv * rounds} in {rounds} rounds")
        return 0
    measured = not a.quick and a.out is None and a.scale is None and not a.only
    if measured:
        if not RULE_FILE.exists() or rule_text(rounds, a.boot) not in RULE_FILE.read_text():
            print(f"refusing the measured run: {RULE_FILE} is missing or does not hold this rule (run --write-rule "
                  "first; the measured run uses the default --rounds and --boot)")
            return 2
        stale = lib_stale()
        if stale:
            print(f"refusing the measured run: lib/ sources newer than {PGLIB}: {', '.join(stale)}")
            return 2
    elif not (a.quick or a.out is not None):
        print("a partial run (--scale or --only) needs --quick or --out, so it cannot overwrite results/")
        return 2
    out = a.out or (BUILD / "quick" if a.quick else RESULTS)
    out.mkdir(parents=True, exist_ok=True)
    started = time.strftime("%Y-%m-%d %H:%M:%S")
    rule_date = "(not fixed: smoke run)"
    if RULE_FILE.exists():
        first = RULE_FILE.read_text().split("\n")
        rule_date = next((l.split(" · ")[0] for l in first if " · written by" in l), rule_date)
    (out / "m3.md").write_text("\n".join(header_lines(rule_date, f"This run started at {started} (in progress).")) + "\n")
    build(a.force_build)
    ver = parse_stats(subprocess.run([str(BIN), "--version"], capture_output=True, text=True, check=True).stdout)
    data = BUILD / ("data" if measured else f"data-s{scale:g}")
    prep = {}
    for wl, w, jobs in P:
        t = time.time()
        g = w["gen"](wrng(a.seed, wl), w["n"])
        check_inputs(g)
        d = data / wl
        write_inputs(d, g)
        prep[wl] = {"dir": d, "exp": {job: expected(job, g, w.get("K", 0)) for job in jobs}}
        print(f"{wl}: inputs and expected values in {time.time() - t:.1f} s", flush=True)
    calib = {}
    choice = calibrate(P, prep, a.reps, 1 if a.quick else 3, calib)
    res = {"workloads": {wl: {k: v for k, v in w.items() if k != "gen"} | {"jobs": {}} for wl, w, _ in P},
           "calibration": calib}
    raw, loads = {}, []
    failed = False
    first_measurement = time.strftime("%Y-%m-%d %H:%M:%S")
    for rnd in range(rounds):
        loads.append(load_avg())
        for wl, w, jobs in P:
            d, exps = prep[wl]["dir"], prep[wl]["exp"]
            for job, rows in jobs.items():
                k = rnd % len(rows)
                for r in rows[k:] + rows[:k]:
                    con = contender_of(r, choice, f"{wl} {job}")
                    o = d / f"out-{r}-{job}.bin"
                    st = run_one(con, job, d, a.reps, w.get("K", 0), o)
                    cr = raw.setdefault((wl, job, r), {"rounds": [], "ok": True, "msg": ""})
                    if r == "fmpq":
                        cr["form"] = "set+addmul" if con == "fmpq-am" else ("mul+add" if job in ("fma", "chain") else "")
                    if "unable" in st:
                        cr["unable"] = st["unable"]
                        continue
                    ok, msg = judge(st, read_out(o), exps[job])
                    if not ok and cr["ok"]:
                        cr["ok"], cr["msg"] = False, f"round {rnd + 1}: {msg}"
                        failed = True
                        print(f"  CHECK FAILED {wl} {job} {r}: {msg}", flush=True)
                    cr["rounds"].append(st)
                    o.unlink()
        print(f"round {rnd + 1}/{rounds} done ({time.strftime('%H:%M:%S')}, load {loads[-1]:.2f})", flush=True)
    for wl, w, jobs in P:
        for job, rows in jobs.items():
            res["workloads"][wl]["jobs"][job] = {r: raw[(wl, job, r)] for r in rows if (wl, job, r) in raw}
    an = analyse(res, a.seed, a.boot)
    cpu = next((l.split(":", 1)[1].strip() for l in Path("/proc/cpuinfo").read_text().splitlines()
                if l.startswith("model name")), platform.processor())
    comment = subprocess.run(["readelf", "-p", ".comment", str(PGLIB)], capture_output=True, text=True).stdout
    lib_cc = sorted({l.split("]", 1)[1].strip() for l in comment.splitlines() if "]" in l and ("GCC" in l or "clang" in l)})
    cfg = {"date": time.strftime("%Y-%m-%d %H:%M:%S"), "started": started, "first_measurement": first_measurement,
           "rule_date": rule_date, "cpu": cpu, "ncpu": os.cpu_count(), "rounds": rounds, "reps": a.reps,
           "seed": a.seed, "boot": a.boot, "band": BAND, "scale": scale, "quick": not measured, "cflags": CFLAGS,
           "gmp": ver.get("gmp"), "flint": ver.get("flint"), "flint_threads": ver.get("flint_threads"),
           "flint_root": flint_root(), "lib_sha256": hashlib.sha256(PGLIB.read_bytes()).hexdigest(),
           "lib_compiler": "; ".join(lib_cc) or "unknown", "load_start": loads[0] if loads else None,
           "load_rounds": loads, "load_max": max(loads) if loads else 0.0, "rule": rule_text(rounds, a.boot)}
    res["config"] = cfg
    res["analysis"] = an
    res["answers"] = answers(res, an)
    res["biases"] = BIASES
    (out / "m3.json").write_text(json.dumps(res, indent=1, default=lambda x: x.tolist() if hasattr(x, "tolist") else str(x)))
    (out / "m3.md").write_text(report_md(res, cfg, an))
    print(f"wrote {out / 'm3.md'} and {out / 'm3.json'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
