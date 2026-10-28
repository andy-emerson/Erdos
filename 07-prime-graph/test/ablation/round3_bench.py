"""Round 3 of the prime-graph ablation: V3 + V4 against V3 (Q1), and three speed-ups of the dictionary entry (Q2).

    python3 round3_bench.py --rule      # print the decision rule (fixed before any measurement) and exit
    python3 round3_bench.py --quick     # a smoke run on small sizes, written to build/quick-r3/ (not results/)
    python3 round3_bench.py             # the measured run: writes results/round3.md and results/round3.json

Rows (all on V3's array layout; each runs both rules for D, lcm and freq, on the array jobs; single-value jobs have
no D, so one plain sub-row):

    R0  V3 with the old entry (gcd refinement against every live node, then pg_factor): control for Q1, reference for Q2
    R1  V3 + V4 (V34: deferred reduction)
    D0  V3 + V6's seen-prime dictionary entry, as it stands: control for Q2
    D1  D0 + a faster small-prime test (blocks of 8 exact-division tests, one multiply and one compare per prime)
    D2  D0 + no Miller-Rabin or rho at entry: large leftovers stay unfactored nodes until a timed idle pass
    D3  D0 + a product check of large leftovers against the known large primes (one remainder, one gcd)

plus the null control: R0 run again as its own row (the same binary), to size the noise band.

Q1: is R1 better than R0?  Q2: does each of D1-D3 beat D0, and does it reach parity with R0?  The verdict rule is
RULE below, written into the report (and into results/round3.md when a run starts) before the first measurement.

Method (as rounds 1-2): every round runs every row of a job once, in an order rotated per round; each invocation runs
--reps warm reps (each re-enters the values from the raw inputs and resets the node table) and reports the minimum of
each phase over them.  Every row is checked against Python's fractions with the strict rules (round 1 every element
and the unreduced rule; later rounds a sample; overflows ("0 0") counted, a row fails on any false overflow or over
0.1%), and after the timed rounds one untimed AB_AUDIT invocation per job audits R1 and D0-D3 (live nodes pairwise
coprime; for D0-D3 the dictionary and every edge as well, and V3's structural audit).

Workloads: W1-W4 and W6-W10 (ablation_bench.py) and S8-S30d (round2_bench.py), from the same seed in the same order as
round 2, so their data are identical; then the new ones in NEW_WORKLOADS (Q1: mixes, K = 8 chains, small unrelated
sets; Q2: a node-count sweep and two-large-prime denominators with varying sharing).
"""
import argparse
import json
import os
import platform
import random
import statistics
import subprocess
import sys
import time
from pathlib import Path

import numpy as np
import sympy

import ablation_bench as ab  # W1-W10, expected values, the strict checks, the report helpers
import pg_bench
import round2_bench as r2    # S8-S30d

HERE = ab.HERE
TEST = ab.TEST
BUILD = HERE / "build" / "r3"
CFLAGS = ab.CFLAGS
DEPS = ab.DEPS + [HERE / "ab_v6.c", HERE / "ab_d.c"]
SAMPLE = ab.SAMPLE
BOOT = 10_000      # bootstrap resamples
BAND = 0.05        # the fixed band: +-5%

BINARIES = {"r0": "VAR_V3", "r1": "VAR_V34", "d0": "VAR_D0", "d1": "VAR_D1", "d2": "VAR_D2", "d3": "VAR_D3"}
ROW_DESC = {
    "r0": "R0 = V3 with the old entry (V0' entry: each new denominator gcd-refined against every live node, then "
          "pg_factor on every node; no dictionary).  Control for Q1, reference for Q2",
    "r1": "R1 = V3 + V4 (round 2's V34: exceptions entered unreduced, V4's deferred ops on the graph path, one timed "
          "final reduction)",
    "d0": "D0 = V3 + V6's seen-prime dictionary as the entry (round 2's V6 entry code, unchanged).  Control for Q2",
    "d1": "D1 = D0 + a faster small-prime test: blocks of 8 branch-free exact-division tests (one multiply, one "
          "compare per prime; vectorised), the adaptive stop once per block; no second pass over the table before rho",
    "d2": "D2 = D0 + no Miller-Rabin or rho at entry: a large leftover is a leftover node, gcd-refined against the "
          "other leftover nodes; leftovers are factored in a timed idle pass (reported apart, included in total)",
    "d3": "D3 = D0 + a product check of large leftovers: a product forest of the known large primes; one remainder "
          "of the product and one gcd per new leftover, the forest searched only on a hit (replaces D0's exact "
          "division by every known large prime)",
}
NULL = "r0"  # the row run twice: <rule>-null is its second copy
RULES = ("lcm", "freq")
AUDITED = ("r1", "d0", "d1", "d2", "d3")


def rows_for(rule):
    """One rule's rows, in table order: R0, its null copy, R1, D0-D3."""
    return [row_name("r0", rule), row_name("r0", rule, True)] + [row_name(b, rule) for b in ("r1",) + AUDITED[1:]]


def row_parts(v):
    """'d1-freq' -> ('d1', 'freq', False); 'r0-lcm-null' -> ('r0', 'lcm', True); 'd2' -> ('d2', 'plain', False)."""
    p = v.split("-")
    null = p[-1] == "null"
    if null:
        p = p[:-1]
    return p[0], (p[1] if len(p) > 1 else "plain"), null


def row_name(base, rule, null=False):
    return base + ("" if rule == "plain" else f"-{rule}") + ("-null" if null else "")


ARRAY_ROWS = rows_for("lcm") + rows_for("freq")
SINGLE_ROWS = rows_for("plain")


def row_env(v):
    _, rule, _ = row_parts(v)
    return {"AB_V3_POLICY": rule} if rule != "plain" else {}


def binary(base):
    return BUILD / f"abl_{base}"


def build(force=False):
    BUILD.mkdir(parents=True, exist_ok=True)
    newest = max(d.stat().st_mtime for d in DEPS)
    for name, macro in BINARIES.items():
        b = binary(name)
        if force or not b.exists() or b.stat().st_mtime < newest:
            subprocess.run(["clang", *CFLAGS, f"-DVARIANT={macro}", "-o", str(b), str(HERE / "abl_bench.c"),
                            str(TEST / "pg.c")], check=True)


# ---------------------------------------------------------------------------------------------------- the rule

RULE = f"""\
**Decision rule (fixed before the run; the harness writes it into results/round3.md before the first measurement).**

1. Each job runs {{rounds}} interleaved rounds.  In every round each row of the job runs once, in an order rotated per
   round, so the rows of one round are paired.  A row's time in a round is its **total** (entry, idle pass, op and
   final reduction; the minimum over the invocation's warm reps).
2. A comparison "X against Y" is the ratio **T(Y) / T(X)**, where T is the median of the row's per-round totals.
   Above 1, X is faster.  Its 95% interval is the 2.5th-97.5th percentile of the same ratio over {{boot}} bootstrap
   resamples of the rounds (rounds drawn with replacement, the same draw for X and Y, so the pairing is kept; seeded).
3. **Null control:** R0 against its own second copy (the same binary run as another row) on the same job and rule.
   Its interval sizes the noise: the job's **null spread** is max(1 - lo, hi - 1) over the null interval [lo, hi].
4. **Band:** h = max({BAND:.0%}, null spread) for that job and rule; the band is [1 - h, 1 + h].
5. **Verdict per job:** **better** if the whole interval lies above 1 + h; **worse** if it lies below 1 - h; otherwise
   **tie** (the interval overlaps the band).  A comparison is not judged (**n/a**) if either row failed a check, was
   unable, or the null control is missing.
6. **Q1:** R1 against R0, per job and rule.  **Q2:** D1, D2, D3 each against D0 ("beats D0" = better), and D0-D3
   against R0 ("parity with R0" = tie or better).  Array jobs are judged under each rule (lcm, freq) separately;
   single-value jobs have one plain sub-row.  No other comparison carries a verdict; the tables show the rest for
   information.
"""


def rule_text(rounds, boot=BOOT):
    return RULE.replace("{rounds}", str(rounds)).replace("{boot}", f"{boot:,}")


# ---------------------------------------------------------------------------------------------------- workloads

def mixed(rng, n, p1000, dmax):
    """A share p1000 of three-place decimals in (-10^4, 10^4) over 1000; the rest numerators |n| < 10^6 over 1..dmax
    (ablation_bench.mixed90 with the share and the second set as parameters)."""
    one = rng.random(n) < p1000
    num = np.where(one, rng.integers(-10**7 + 1, 10**7, size=n, dtype=np.int64),
                   rng.integers(-10**6 + 1, 10**6, size=n, dtype=np.int64))
    den = np.where(one, 1000, rng.integers(1, dmax + 1, size=n, dtype=np.int64)).astype(np.uint64)
    return num, den


def chain_dec(rng, n):
    """K = 8 chain on decimals: x0, c three-place decimals in (-10, 10) (over 1000), b tenths in (-2, 2) (over 10).
    After 8 steps the denominator divides 1000 * 10^8 and |x| < 2^8 * 10 + 2^8 * 10: every value fits 64 bits."""
    return {"a": pg_bench.dec(rng, n, -10, 10), "b": (rng.integers(-19, 20, size=n, dtype=np.int64),
                                                      np.full(n, 10, dtype=np.uint64)),
            "c": pg_bench.dec(rng, n, -10, 10)}


def chain_mix(rng, n):
    """K = 8 chain on mixed denominators: x0, c numerators |n| < 10^4 over 1..12; b over 1..4 with |b| < 2.  The
    denominator stays below 27720 * 4^8 < 2^31 and |x| below 2^9 * 10^4, so every reduced value fits 64 bits."""
    bd = rng.integers(1, 5, size=n, dtype=np.int64)
    bn = np.array([int(rng.integers(-2 * d + 1, 2 * d)) for d in bd.tolist()], dtype=np.int64)
    side = lambda: (rng.integers(-10**4 + 1, 10**4, size=n, dtype=np.int64),
                    rng.integers(1, 13, size=n, dtype=np.int64).astype(np.uint64))
    return {"a": side(), "b": (bn, bd.astype(np.uint64)), "c": side()}


def from_set(rng, n, dens, nmax=10**6):
    """numerators |n| < nmax over denominators drawn uniformly from dens; every member used at least once."""
    dens = np.asarray(dens, dtype=np.uint64)
    pick = rng.integers(0, len(dens), size=n)
    pick[:min(n, len(dens))] = rng.permutation(len(dens))[:n]
    return rng.integers(-nmax + 1, nmax, size=n, dtype=np.int64), dens[pick]


def unrelated(rng, n, k):
    """k denominators drawn once (distinct) from 2..1000, then a, b, c over that set."""
    dens = rng.choice(np.arange(2, 1001), size=k, replace=False)
    return {x: from_set(rng, n, dens) for x in "abc"}


def shared_den_fma(rng, n, dens):
    """a*b + c with a over the given set, c over the same denominator as a, b over 1..12 with |num| < 100 (W7's b)."""
    num_a, den_a = from_set(rng, n, dens)
    return {"a": (num_a, den_a),
            "b": (rng.integers(-99, 100, size=n, dtype=np.int64), rng.integers(1, 13, size=n).astype(np.uint64)),
            "c": (rng.integers(-10**6 + 1, 10**6, size=n, dtype=np.int64), den_a.copy())}


def distinct(rng, k, lo, hi):
    """k distinct integers uniform in [lo, hi), in draw order (duplicates skipped)."""
    out, seen = [], set()
    while len(out) < k:
        for x in rng.integers(lo, hi, size=k).tolist():
            if x not in seen and len(out) < k:
                seen.add(x)
                out.append(x)
    return out


def sweep(rng, n, N):
    """N distinct denominators: half uniform in [2, 2^31) (leftovers below 2^32: prime after the table), half uniform
    in [2^32, 2^40) (leftovers of 2^32 or more: the known large primes, Miller-Rabin, rho)."""
    return shared_den_fma(rng, n, distinct(rng, N // 2, 2, 2**31) + distinct(rng, N - N // 2, 2**32, 2**40))


def two_primes(rng, n, pool, M=2000):
    """M distinct denominators p*q with p != q drawn from a pool of `pool` primes in [2^17, 2^24) (products of 34-48
    bits: leftovers of 2^32 or more).  The pool size sets the sharing: each prime is in about 2M/pool denominators
    (64: about 62; 4000: exactly one, the pairs disjoint)."""
    ps = set()
    while len(ps) < pool:
        ps.add(int(sympy.nextprime(int(rng.integers(2**17, 2**24)))))
    ps = sorted(ps)
    dens = set()
    if 2 * M <= pool:  # disjoint pairs: no prime shared
        perm = rng.permutation(pool)
        dens = {ps[perm[2 * i]] * ps[perm[2 * i + 1]] for i in range(M)}
    else:
        while len(dens) < M:
            i, j = rng.choice(pool, size=2, replace=False)
            dens.add(ps[i] * ps[j])
    return shared_den_fma(rng, n, sorted(dens))


NEW_WORKLOADS = {  # name: (aim, description, n, generator, jobs, extra)
    "M50-12": ("Q1", "50% denominator 1000 (three-place decimals in (-10^4, 10^4)), 50% denominators 1..12 (|num| < 10^6)",
               1_000_000, lambda r, n: {k: mixed(r, n, 0.50, 12) for k in "abc"}, ["sum", "fma"], {}),
    "M75-12": ("Q1", "75% denominator 1000, 25% denominators 1..12", 1_000_000,
               lambda r, n: {k: mixed(r, n, 0.75, 12) for k in "abc"}, ["sum", "fma"], {}),
    "M95-12": ("Q1", "95% denominator 1000, 5% denominators 1..12", 1_000_000,
               lambda r, n: {k: mixed(r, n, 0.95, 12) for k in "abc"}, ["sum", "fma"], {}),
    "M50-1k": ("Q1", "50% denominator 1000, 50% denominators 1..1000 (|num| < 10^6); no column sum (its common "
                     "denominator is far beyond 64 bits)", 200_000,
               lambda r, n: {k: mixed(r, n, 0.50, 1000) for k in "abc"}, ["fma"], {}),
    "M75-1k": ("Q1", "75% denominator 1000, 25% denominators 1..1000", 200_000,
               lambda r, n: {k: mixed(r, n, 0.75, 1000) for k in "abc"}, ["fma"], {}),
    "M95-1k": ("Q1", "95% denominator 1000, 5% denominators 1..1000", 200_000,
               lambda r, n: {k: mixed(r, n, 0.95, 1000) for k in "abc"}, ["fma"], {}),
    "K8-dec": ("Q1", "chain x <- x*b + c, K = 8: x0, c three-place decimals in (-10, 10), b tenths in (-2, 2)",
               1_000_000, chain_dec, ["chain"], {"K": 8}),
    "K8-mix": ("Q1", "chain x <- x*b + c, K = 8: x0, c over 1..12 (|num| < 10^4), b over 1..4 with |b| < 2",
               1_000_000, chain_mix, ["chain"], {"K": 8}),
    "U8": ("Q1", "unrelated denominators from a smaller set: 8 drawn once from 2..1000 (|num| < 10^6), a*b + c",
           200_000, lambda r, n: unrelated(r, n, 8), ["fma"], {}),
    "U32": ("Q1", "unrelated denominators: 32 drawn once from 2..1000, a*b + c", 200_000,
            lambda r, n: unrelated(r, n, 32), ["fma"], {}),
    "N100": ("Q2", "node-count sweep: 100 distinct denominators (half in [2, 2^31), half in [2^32, 2^40)); a*b + c, "
                   "c over a's denominator, b over 1..12", 200_000, lambda r, n: sweep(r, n, 100), ["fma"], {}),
    "N1k": ("Q2", "node-count sweep: 1,000 distinct denominators", 200_000, lambda r, n: sweep(r, n, 1000),
            ["fma"], {}),
    "N10k": ("Q2", "node-count sweep: 10,000 distinct denominators", 200_000, lambda r, n: sweep(r, n, 10_000),
             ["fma"], {"reps": 1}),
    "N30k": ("Q2", "node-count sweep: 30,000 distinct denominators", 200_000, lambda r, n: sweep(r, n, 30_000),
             ["fma"], {"reps": 1}),
    "P64": ("Q2", "2,000 distinct denominators p*q from a pool of 64 primes in [2^17, 2^24) (each prime in about 62 "
                  "denominators); a*b + c as N*", 200_000, lambda r, n: two_primes(r, n, 64), ["fma"], {}),
    "P256": ("Q2", "2,000 p*q from a pool of 256 primes (each in about 16)", 200_000,
             lambda r, n: two_primes(r, n, 256), ["fma"], {}),
    "P1024": ("Q2", "2,000 p*q from a pool of 1,024 primes (each in about 4)", 200_000,
              lambda r, n: two_primes(r, n, 1024), ["fma"], {}),
    "P4000": ("Q2", "2,000 p*q from 4,000 primes in disjoint pairs (no prime shared)", 200_000,
              lambda r, n: two_primes(r, n, 4000), ["fma"], {}),
}


def all_workloads(quick):
    """Every workload in generation order (the order fixes the data): ablation_bench's W1-W10 (W5 is generated, to
    keep the generator in step with round 2, but not run), round2_bench's S8-S30d, then NEW_WORKLOADS."""
    W = {}
    for wl, w in ab.workloads(quick).items():
        W[wl] = dict(w, aim="existing", single=False, run=wl != "W5")
    for wl, w in r2.sizes(quick).items():
        W[wl] = dict(w, aim="existing", single=True, run=True)
    s = (lambda n: max(n // 50, 1000)) if quick else (lambda n: n)
    for wl, (aim, desc, n, gen, jobs, extra) in NEW_WORKLOADS.items():
        W[wl] = dict(desc=desc, n=s(n), gen=gen, jobs=jobs, aim=aim, single=False, run=True, **extra)
    if quick:  # the sweep's distinct counts cannot exceed n
        for wl in ("N10k", "N30k"):
            n = W[wl]["n"]
            W[wl]["gen"] = (lambda N: lambda r, n: sweep(r, n, N))(min(n, {"N10k": 10_000, "N30k": 30_000}[wl]))
    return W


def plan(quick):
    """[(workload, spec, {job: [rows]})] for the workloads that run, in generation order."""
    out = []
    for wl, w in all_workloads(quick).items():
        if not w["run"]:
            continue
        rows = SINGLE_ROWS if w["single"] else ARRAY_ROWS
        out.append((wl, w, {job: list(rows) for job in w["jobs"]}))
    return out


# ---------------------------------------------------------------------------------------------------- running

def run_one(v, job, d, reps, K, out, env=None):
    base, _, _ = row_parts(v)
    e = dict(os.environ if env is None else env)
    for k, x in row_env(v).items():
        e.setdefault(k, x)
    args = [str(binary(base)), job, str(d), str(out), str(reps)] + ([str(K)] if job == "chain" else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=7200, env=e)
    if r.returncode == 3:
        return {"unable": r.stdout.strip().removeprefix("unable ")}
    if r.returncode != 0:
        raise RuntimeError(f"{v} {job} failed ({r.returncode}): {r.stderr.strip()}")
    return ab.parse_stats(r.stdout)


def phases(st, v):
    """entry (without the idle pass), idle, op, final, total in ns.  D2's idle pass runs in the driver's
    decomposition slot; for the other rows that slot is part of their entry (R0/R1: pg_factor on every node)."""
    base, _, _ = row_parts(v)
    idle = st["decomp"] if base == "d2" else 0
    return {"entry": st["entry"] - idle, "idle": idle, "decomp": st["decomp"] if base != "d2" else 0,
            "op": st["op"], "final": st["red"], "total": st["total"]}


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
    if lo > 1 + h:
        return "better"
    if hi < 1 - h:
        return "worse"
    return "tie"


def ok_row(cr):
    return cr is not None and "rounds" in cr and cr.get("ok") and cr["rounds"]


def totals(cr):
    return [phases(s, cr["name"])["total"] for s in cr["rounds"]]


COMPARISONS = [  # (question, X, Y): the ratio T(Y) / T(X); above 1, X is faster
    ("Q1", "r1", "r0"),
    ("Q2-D0", "d1", "d0"), ("Q2-D0", "d2", "d0"), ("Q2-D0", "d3", "d0"),
    ("Q2-R0", "d0", "r0"), ("Q2-R0", "d1", "r0"), ("Q2-R0", "d2", "r0"), ("Q2-R0", "d3", "r0"),
]


def analyse(res, seed, boot=BOOT):
    """Per job and rule: the null interval, the band, and every comparison with its interval and verdict."""
    rng = np.random.default_rng(seed)
    out = {}
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            rules = sorted({row_parts(v)[1] for v in jr}, key=lambda r: ("lcm", "freq", "plain").index(r))
            for rule in rules:
                key = f"{wl} {job} {rule}"
                a = {"wl": wl, "job": job, "rule": rule, "cmp": {}}
                x, y = jr.get(row_name(NULL, rule, True)), jr.get(row_name(NULL, rule))
                if ok_row(x) and ok_row(y):
                    p, lo, hi = ratio_ci(totals(x), totals(y), rng, boot)
                    a["null"] = {"point": p, "lo": lo, "hi": hi, "spread": max(1 - lo, hi - 1, 0.0)}
                    a["h"] = max(BAND, a["null"]["spread"])
                else:
                    a["null"], a["h"] = None, None
                for q, xb, yb in COMPARISONS:
                    cx, cy = jr.get(row_name(xb, rule)), jr.get(row_name(yb, rule))
                    c = {"q": q, "x": xb, "y": yb}
                    if cx is None or cy is None:
                        c["verdict"] = "not run"
                    elif not (ok_row(cx) and ok_row(cy)):
                        c["verdict"] = "n/a"
                        c["why"] = "unable" if "unable" in cx or "unable" in cy else "fail"
                    else:
                        c["point"], c["lo"], c["hi"] = ratio_ci(totals(cx), totals(cy), rng, boot)
                        c["verdict"] = verdict(c["lo"], c["hi"], a["h"]) if a["h"] is not None else "n/a"
                        if a["h"] is None:
                            c["why"] = "no null control"
                    a["cmp"][f"{xb}/{yb}"] = c
                out[key] = a
    return out


# ---------------------------------------------------------------------------------------------------- report

ms = ab.ms


def fmt_h(a):
    return "" if a["h"] is None else f"{a['h']:.1%}"


def fmt_ci(c):
    if "point" not in c:
        return c.get("why", c["verdict"])
    return f"{c['point']:.2f} [{c['lo']:.2f}, {c['hi']:.2f}]"


def tally(an, cmpkey, parity=False):
    t = {"better": 0, "tie": 0, "worse": 0, "n/a": 0}
    for a in an.values():
        c = a["cmp"].get(cmpkey)
        if c is None or c["verdict"] == "not run":
            continue
        t[c["verdict"] if c["verdict"] in t else "n/a"] += 1
    s = f"better {t['better']}, tie {t['tie']}, worse {t['worse']}" + (f", not judged {t['n/a']}" if t["n/a"] else "")
    if parity:
        s += f" (parity reached on {t['better'] + t['tie']} of {t['better'] + t['tie'] + t['worse']})"
    return s


def structure(v, last):
    if not last:
        return ""
    base, rule, _ = row_parts(v)
    s = ""
    if rule != "plain" or base in ("r0", "r1"):
        s = (f"D={last.get('D')}; exc {last.get('exc_in', 0):,} in, {last.get('exc_out', 0):,} out; "
             f"{last.get('dense_ovf', 0):,} dense ovf")
        if base == "r1":
            s += f"; {last.get('early', 0):,} early red."
    if base in ("r0", "r1"):
        return s + f"; {last.get('nodes', 0):,} nodes, {last.get('splits', 0):,} decomp splits"
    s += (f"; {last.get('v6_nodes', 0):,} nodes ({last.get('v6_multi', 0):,} multi-prime); splits "
          f"{last.get('v6_entry_splits', 0):,} entry / {last.get('v6_op_splits', 0):,} ops")
    if base == "d2":
        s += (f"; {last.get('d2_leftovers', 0):,} leftovers, {last.get('d2_gcds', 0):,} gcds, "
              f"{last.get('d2_idle_factored', 0):,} factored idle")
    if base == "d3":
        s += f"; {last.get('d3_checks', 0):,} checks, {last.get('d3_hits', 0):,} hits, {last.get('d3_leaves', 0):,} leaves"
    return s


def med(cr, k):
    v = [phases(s, cr["name"])[k] for s in cr["rounds"]]
    return statistics.median(v) if v else 0


def med_stat(cr, k):
    v = [s[k] for s in cr.get("rounds", []) if isinstance(s.get(k), int)]
    return statistics.median(v) if v else None


def summary_md(res, an):
    L = []
    nulls = [a["null"]["point"] for a in an.values() if a.get("null")]
    spreads = [a["null"]["spread"] for a in an.values() if a.get("null")]
    L += ["## Noise", ""]
    if nulls:
        wide = sum(s > BAND for s in spreads)
        L += [f"Null control (R0 against its own second copy) over {len(nulls)} job-rule pairs: point ratios "
              f"{min(nulls):.2f} to {max(nulls):.2f} (median {statistics.median(nulls):.2f}); null spread up to "
              f"{max(spreads):.1%}, above the {BAND:.0%} floor on {wide} of them (their band is the wider one).", ""]
    else:
        L += ["No null control ran: no verdicts.", ""]
    # Q1
    L += ["## Q1: is R1 (V3 + V4) better than R0 (V3)?", "",
          "R0 / R1 on total time (above 1, R1 is faster), its 95% interval, the band half-width h and the verdict.", "",
          f"**Tally:** {tally(an, 'r1/r0')}.", "",
          "| job | rule | aim | R0 / R1 [95%] | h | verdict |", "|---|---|---|---|---:|---|"]
    W = res["workloads"]
    for k, a in an.items():
        c = a["cmp"]["r1/r0"]
        L.append(f"| {a['wl']} {a['job']} | {a['rule']} | {W[a['wl']]['aim']} | {fmt_ci(c)} | "
                 f"{fmt_h(a)} | {c['verdict']} |")
    # Q2
    L += ["", "## Q2: do D1-D3 beat D0, and do they reach parity with R0?", "",
          "Left: D0 / Dk on total time (above 1, Dk is faster than D0).  Right: R0 / Dk (above 1, Dk is faster than "
          "R0; parity = tie or better).  Each cell: the ratio [95% interval], verdict.", "",
          f"**Beats D0:** D1 {tally(an, 'd1/d0')}; D2 {tally(an, 'd2/d0')}; D3 {tally(an, 'd3/d0')}.", "",
          f"**Against R0:** D0 {tally(an, 'd0/r0', True)}; D1 {tally(an, 'd1/r0', True)}; "
          f"D2 {tally(an, 'd2/r0', True)}; D3 {tally(an, 'd3/r0', True)}.", "",
          "| job | rule | aim | h | D0 / D1 | D0 / D2 | D0 / D3 | R0 / D0 | R0 / D1 | R0 / D2 | R0 / D3 |",
          "|---|---|---|---:|---|---|---|---|---|---|---|"]
    for k, a in an.items():
        cells = []
        for key in ("d1/d0", "d2/d0", "d3/d0", "d0/r0", "d1/r0", "d2/r0", "d3/r0"):
            c = a["cmp"][key]
            cells.append(fmt_ci(c) + ("" if "point" not in c else f" {c['verdict']}"))
        L.append(f"| {a['wl']} {a['job']} | {a['rule']} | {W[a['wl']]['aim']} | "
                 f"{fmt_h(a)} | " + " | ".join(cells) + " |")
    # entry breakdown
    L += ["", "## Entry breakdown (D rows against R0)", "",
          "Medians over rounds, ms.  **entry** excludes D2's idle pass; **idle** is D2's pass (factoring the leftover "
          "nodes), included in total.  R0's entry includes its gcd refinement and pg_factor on every node (**R0 "
          "decomp**).  **dict** is the dictionary passes, **fill** the rest of entry (D rows).  For array jobs the lcm "
          "rule's rows are shown (entry does not depend on the rule except through V3's choice of D, in the fill).", "",
          "| job | R0 entry | R0 decomp | D0 entry | D1 entry | D2 entry | D2 idle | D3 entry | D0 dict | D1 dict | "
          "D2 dict | D3 dict | D0 / D2 entry (no idle) | nodes R0 / D0 | D2 leftovers / gcds | D3 checks / hits |",
          "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---|"]
    for wl, w in W.items():
        for job, jr in w["jobs"].items():
            rule = "plain" if w.get("single") else "lcm"
            R = {b: jr.get(row_name(b, rule)) for b in ("r0", "d0", "d1", "d2", "d3")}
            if not all(ok_row(R[b]) for b in R):
                L.append(f"| {wl} {job} | " + " | ".join(["n/a"] * 15) + " |")
                continue
            d = lambda b, k: ms(med(R[b], k))
            dm = lambda b: ms(med_stat(R[b], "v6_dict_ns") or 0)
            l2, l3 = R["d2"]["rounds"][-1], R["d3"]["rounds"][-1]
            L.append(f"| {wl} {job} | {d('r0', 'entry')} | {d('r0', 'decomp')} | {d('d0', 'entry')} | "
                     f"{d('d1', 'entry')} | {d('d2', 'entry')} | {d('d2', 'idle')} | {d('d3', 'entry')} | {dm('d0')} | "
                     f"{dm('d1')} | {dm('d2')} | {dm('d3')} | {med(R['d0'], 'entry') / max(med(R['d2'], 'entry'), 1):.2f} | "
                     f"{R['r0']['rounds'][-1].get('nodes', 0):,} / {R['d0']['rounds'][-1].get('v6_nodes', 0):,} | "
                     f"{l2.get('d2_leftovers', 0):,} / {l2.get('d2_gcds', 0):,} | "
                     f"{l3.get('d3_checks', 0):,} / {l3.get('d3_hits', 0):,} |")
    # audits
    au = res.get("audits") or {}
    L += ["", "## Audits", "",
          "One untimed invocation per job and row with `AB_AUDIT=1` after the timed rounds.  R1: live nodes pairwise "
          "coprime, V3's structure.  D0-D3: every edge re-expanded onto live nodes, live nodes pairwise coprime, each "
          "node the product of its primes, the dictionary consistent, then V3's structure.", ""]
    if au:
        L += ["Array jobs: lcm / freq; single-value jobs: the plain row.", "",
              "| job | " + " | ".join(b.upper() for b in AUDITED) + " |", "|---|" + "---|" * len(AUDITED)]
        for wl in W:
            for job in W[wl]["jobs"]:
                a = au.get(wl, {}).get(job, {})
                if not a:
                    continue
                rules = ("plain",) if W[wl].get("single") else RULES
                L.append(f"| {wl} {job} | " + " | ".join(" / ".join(a.get(row_name(b, r), "not run") for r in rules)
                                                      for b in AUDITED) + " |")
    return L


def report_md(res, cfg, an):
    L = ["# Prime graph: ablation, round 3", "",
         f"Generated {cfg['date']} by `round3_bench.py` on {cfg['cpu']}; clang 18 `{' '.join(CFLAGS[:3])}`.  "
         f"{cfg['rounds']} interleaved rounds; each invocation runs {cfg['reps']} warm reps (fewer where a workload "
         "says so: N10k and N30k run 1) and reports the minimum of each phase.  Tables give medians over rounds; "
         "times in ms.", "",
         f"Rule fixed at {cfg.get('rule_fixed', '?')}; first measurement at {cfg.get('first_measurement', '?')}.", "",
         rule_text(cfg["rounds"], cfg["boot"]), "", "## Rows", ""] + [f"- **{b.upper()}**: {d}" for b, d in ROW_DESC.items()] + [
         "- **null**: R0 again, the same binary, as its own row (rows `r0-<rule>-null`).", "",
         "Array jobs run each row under both rules for V3's array denominator D (`-lcm`: the lcm of the array's "
         "denominators when below 2^63, else the most frequent; `-freq`: the most frequent).  Single-value jobs "
         "(S8-S30d) enter every value as an exception (no D), so they have one plain sub-row.", ""]
    L += summary_md(res, an)
    L += ["", "## Workloads", ""] + [f"- **{wl}** ({w['aim']}): {w['desc']}; n = {w['n']:,}"
                                     + (f", K = {w['K']}" if w.get("K") else "") + f"; jobs {', '.join(w['jobs'])}"
                                     for wl, w in res["workloads"].items()] + [""]
    L += ["Detail tables: **entry** excludes D2's idle pass, shown as **idle**; **decomp** is R0/R1's pg_factor stage "
          "(inside their entry); **final** is the timed final reduction; **total** is the minimum over reps of "
          "entry + idle + op + final in one rep; **R0 / x** is R0's total over the row's total under the same rule; "
          "**repr** is the measured peak of the representation's allocations; **unreduced %** (round 1, every output) "
          "fails a row above 0; overflowed elements (\"0 0\") are counted: a row fails if over 0.1% overflowed or any "
          "overflow was false; ‡ marks rows with some.", ""]
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            L += [f"### {wl} {job}: {w['desc']}", "", f"n = {w['n']:,}" + (f", K = {w['K']}" if w.get("K") else ""), "",
                  "| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | "
                  "structure | unreduced % | check |",
                  "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|"]
            for v, cr in jr.items():
                if "unable" in cr:
                    L.append(f"| {v} | unable: {cr['unable']} | | | | | | unable | | | | | | unable |")
                    continue
                base, rule, _ = row_parts(v)
                r0 = jr.get(row_name("r0", rule))
                rr = (f"{med(r0, 'total') / med(cr, 'total'):.2f}" if ok_row(r0) and ok_row(cr)
                      else ("fail" if not cr["ok"] or (r0 and not r0.get("ok")) else "n/a"))
                mark = " ‡" if (cr.get("ovf_elems") or {}).get("n") else ""
                last = cr["rounds"][-1] if cr["rounds"] else {}
                L.append(f"| {v} | {ms(med(cr, 'entry'))} | {ms(med(cr, 'idle'))} | {ms(med(cr, 'decomp'))} | "
                         f"{ms(med(cr, 'op'))} | {ms(med(cr, 'final'))} | {ms(med(cr, 'total'))} | {rr}{mark} | "
                         f"{last.get('maxrss_kb', 0) / 1024:.0f} | {last.get('rep_bytes', 0) / 2**20:.1f} | "
                         f"{cr.get('out_dens', '')} | {structure(v, last)} | {cr.get('unreduced') or 0:.2f} | "
                         f"{'ok' if cr['ok'] else 'FAIL: ' + cr['msg']} |")
            notes = []
            for v, cr in jr.items():
                oe = cr.get("ovf_elems") or {}
                if cr.get("rounds") and (cr["rounds"][-1].get("ovf") or oe.get("n")):
                    notes.append(f"{v}: overflows detected {cr['rounds'][-1].get('ovf', 0)} (outputs left overflowed: "
                                 f"{oe.get('n', 0)}, of which {oe.get('fit', 0)} have an exact value that fits 64 bits)")
            L += [""] + [f"{n}  " for n in notes] + [""]
    L += ["## Simplifications that may bias the results", ""] + [f"- {b}" for b in BIASES] + [""]
    return "\n".join(L)


BIASES = [
    "R0 and R1 are round 2's V3 and V34 binaries' code (rebuilt from the same sources; checked with objdump to be the "
    "same machine code, as are V0', V1, V2, V4, V5, V6 and the 32-bit builds).  The round-3 edits to shared files are "
    "preprocessor switches (AB_DICT, AB_V3L, VARIANT >= VAR_D0) and the new file ab_d.c.",
    "D0 is not round 2's V6: it is V6's entry on V3's layout.  V6's nodes need not be prime and splits kill nodes, so "
    "D0-D3 add one piece of glue V3 lacks: each array records the split count when its edges were last live, and an "
    "op re-expands its operands' edges (D and the exceptions) onto live nodes first if a split happened since; the "
    "final reduction does the same when it splits a node.  The cost is one compare per operand per op when nothing "
    "split, and it is in every D row alike.",
    "V3's dense elements are not reduced at entry, so V6's fill-time splits (a numerator sharing part of a multi-prime "
    "node) happen in D rows only for exceptions; dense elements meet such nodes first in the final reduction (timed).",
    "D1's faster test is the table in blocks of 8 exact-division tests, branch-free (clang vectorises it with "
    "-march=native), the stop checked once per block; a hit's exponent is then divided out as in D0.  D0 already "
    "used the multiply-by-inverse test (one multiply and one compare) but added a multiply and a branch per prime "
    "for the stop.  D1 also skips pg_factor's second pass over the table before rho (it calls a copy of pg.c's "
    "Miller-Rabin/rho stage directly), which only matters for composite large leftovers.  GMP-style packing was "
    "measured in isolation before this round (200,000 one-word denominators each, ns per denominator): full-table "
    "scans of 40-bit numbers without small factors 10,793 (D0's loop), 2,583 (blocks), 2,573 (blocks + n mod "
    "3*5*...*53 with the 15 tiny primes tested on the residue), 2,987 (32-bit lanes when n < 2^32); random 2..2^31: "
    "1,019 / 256 / 264 / 262; random 1..1000: 17.2 / 16.4 / 18.3 / 17.4.  Packing did not help one-word "
    "denominators, so D1 does not pack.",
    "D2 keeps large leftovers (2^32 or more after the table and the known large primes) as leftover nodes.  Kept "
    "coprime without factoring, that costs one gcd against every live leftover node per new leftover, and one exact "
    "division test against every leftover node per first-sighted large prime: linear in the leftovers, the cost "
    "D3's product check would remove (the two are separate rows by design).  A leftover is its own node, not "
    "grouped with the small primes first seen in the same denominator (D0 groups them), so D2's graph can be finer.",
    "D2's idle pass runs in the driver's decomposition slot, between entry's pass 1 and pass 2: the fill and the "
    "operations then see a factored graph, as in D0.  It factors every leftover node with Miller-Rabin, then "
    "pg_factor (D0's step 4: the same code), so its total work is D0's plus the gcd refinement; what can move is "
    "entry (without idle).  An idle pass deferred past the operations would need gcd-based hooked operations on "
    "unfactored nodes, which no row has.",
    "D3's forest holds every large prime (2^16 or more) entered in the dictionary, the same set D0's step 1b loops "
    "over, as perfect subtrees of up to 64 leaves (roots merged as leaves arrive); the remainders use a precomputed "
    "reciprocal of the leftover (Moller-Granlund), and roots are combined by modular products.  D0's step 1b is "
    "already one multiply and one compare per known prime with an early exit below 2^32, so the product check "
    "competes with a cheap loop, not with per-node gcds; it is not capped and never rebuilt (no prime leaves the "
    "dictionary).",
    "Entry is timed from the raw inputs and includes the dictionary passes, the fill (V3's choice of D, scaling, "
    "exception entry), and for R0 the gcd scan and pg_factor.  Per-rep resets (node tables, the dictionary's 256 KB "
    "direct-mapped array and hash, D2's leftover list, D3's forest) are outside the timer; table capacities carry "
    "over between warm reps, and the minimum over reps is kept.",
    "The verdict uses the median over 12 rounds of each row's per-invocation minimum.  The bootstrap resamples rounds, "
    "so it captures round-to-round noise but not effects shared by every round (machine, compiler, data).  The null "
    "band comes from one row (R0) per job and rule; a row much noisier than R0 could be called better or worse "
    "where a wider band would say tie.  With 12 rounds the median moves in discrete steps, so intervals can be "
    "lumpy; the 5% floor guards against reading those steps as effects.",
    "N10k and N30k run one rep per invocation (R0's entry there takes seconds: it gcd-refines each new denominator "
    "against every live node).  The sweep's denominators are half below 2^31 and half in [2^32, 2^40), so it mixes "
    "cheap and expensive leftovers; the P workloads isolate two-large-prime denominators.  Neither is drawn from "
    "real data.",
    "Workloads W1-W4, W6-W10 and S8-S30d are round 2's data (same seed, same generation order, W5 generated but not "
    "run).  The new workloads come after them from the same generator, so changing their order or sizes changes "
    "their data but not the old ones.",
    "Every rule of round 2's checks applies (fractions, unreduced, overflow).  The coprime audit runs once per job and "
    "row after the timed rounds, on the last rep's graph and arrays; it is not repeated every round.",
]


# ---------------------------------------------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rounds", type=int, default=12)
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--quick", action="store_true", help="small sizes; output to build/quick-r3/ unless --out")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--only", help="comma-separated workloads, e.g. W1,S30,P64")
    ap.add_argument("--rows", help="comma-separated base rows to run (r0,r1,d0,d1,d2,d3; the null runs with r0)")
    ap.add_argument("--seed", type=int, default=20261027)
    ap.add_argument("--boot", type=int, default=BOOT)
    ap.add_argument("--rule", action="store_true", help="print the decision rule and exit")
    ap.add_argument("--force-build", action="store_true")
    a = ap.parse_args(argv)
    if a.rule:
        print(rule_text(a.rounds, a.boot))
        return 0
    out = a.out or (HERE / "build" / "quick-r3" if a.quick else HERE / "results")
    out.mkdir(parents=True, exist_ok=True)
    rule_fixed = time.strftime("%Y-%m-%d %H:%M:%S")
    (out / "round3.md").write_text(f"# Prime graph: ablation, round 3 (run in progress)\n\nRule fixed at {rule_fixed}, "
                                   "before the first measurement.\n\n" + rule_text(a.rounds, a.boot) + "\n")
    build(a.force_build)
    W = all_workloads(a.quick)
    P = plan(a.quick)
    if a.only:
        P = [p for p in P if p[0] in a.only.split(",")]
    if a.rows:
        keep = set(a.rows.split(","))
        P = [(wl, w, {j: [v for v in rows if row_parts(v)[0] in keep] for j, rows in jobs.items()}) for wl, w, jobs in P]
    sel = {p[0] for p in P}
    data = HERE / "build" / ("r3-data-quick" if a.quick else "r3-data")
    rng = np.random.default_rng(a.seed)  # every workload generated in order, so the data do not depend on --only
    prep = {}
    for wl, w in W.items():
        t = time.time()
        arrays = w["gen"](rng, w["n"])
        if wl not in sel:
            continue
        d = data / wl
        pg_bench.write_inputs(d, arrays)
        prep[wl] = (d, {job: ab.expected(job, arrays, w.get("K", 0)) for job in w["jobs"]})
        print(f"{wl}: inputs and expected values in {time.time() - t:.1f} s", flush=True)
    res = {"workloads": {wl: {"desc": w["desc"], "n": w["n"], "K": w.get("K"), "aim": w["aim"],
                              "single": w["single"], "jobs": {}} for wl, w, _ in P}}
    raw = {}
    srng = random.Random(a.seed)
    failed = False
    first_measurement = time.strftime("%Y-%m-%d %H:%M:%S")
    for rnd in range(a.rounds):
        for wl, w, jobs in P:
            d, exps = prep[wl]
            reps = w.get("reps", a.reps)
            for job, rows in jobs.items():
                if not rows:
                    continue
                k = rnd % len(rows)
                for v in rows[k:] + rows[:k]:
                    o = data / wl / f"out-{v}-{job}.txt"
                    st = run_one(v, job, d, reps, w.get("K", 0), o)
                    r = raw.setdefault((wl, job, v), {"name": v, "rounds": [], "ok": True, "msg": "", "unreduced": None})
                    if "unable" in st:
                        r["unable"] = st["unable"]
                        continue
                    exp = exps[job]
                    lines = o.read_text().splitlines()
                    first = r["unreduced"] is None
                    idx = None if first or len(exp) <= SAMPLE else srng.sample(range(len(exp)), SAMPLE)
                    ok, msg, extra = ab.judge(v, lines, exp, idx, first)
                    r.update(extra)
                    if not ok and r["ok"]:
                        r["ok"], r["msg"] = False, f"round {rnd + 1}: {msg}"
                        failed = True
                        print(f"  CHECK FAILED {wl} {job} {v}: {msg}", flush=True)
                    r["rounds"].append(st)
                    o.unlink()
        print(f"round {rnd + 1}/{a.rounds} done ({time.strftime('%H:%M:%S')})", flush=True)
    audits = {}
    for wl, w, jobs in P:
        d, _ = prep[wl]
        for job, rows in jobs.items():
            for v in rows:
                if row_parts(v)[0] not in AUDITED or row_parts(v)[2]:
                    continue
                o = data / wl / f"audit-{v}-{job}.txt"
                st = run_one(v, job, d, 1, w.get("K", 0), o, env=dict(os.environ, AB_AUDIT="1"))
                ra = "unable" if "unable" in st else ("pass" if st.get("audit") == "ok" else f"fail: {st.get('audit')}")
                audits.setdefault(wl, {}).setdefault(job, {})[v] = ra
                if ra.startswith("fail"):
                    failed = True
                    print(f"  AUDIT FAILED {wl} {job} {v}: {st.get('audit')}", flush=True)
                if o.exists():
                    o.unlink()
    res["audits"] = audits
    for (wl, job, v), r in raw.items():
        jr = res["workloads"][wl]["jobs"].setdefault(job, {})
        jr[v] = ({"name": v, "unable": r["unable"]} if "unable" in r else
                 {"name": v, "ok": r["ok"], "msg": r["msg"], "unreduced": r["unreduced"], "ovf_elems": r.get("ovf_elems"),
                  "out_dens": r.get("out_dens"), "rounds": r["rounds"]})
    for wl, w, jobs in P:  # table order: the plan's
        for job, rows in jobs.items():
            jr = res["workloads"][wl]["jobs"].get(job, {})
            res["workloads"][wl]["jobs"][job] = {v: jr[v] for v in rows if v in jr}
    an = analyse(res, a.seed, a.boot)
    cpu = next((l.split(":", 1)[1].strip() for l in Path("/proc/cpuinfo").read_text().splitlines()
                if l.startswith("model name")), platform.processor())
    cfg = {"date": time.strftime("%Y-%m-%d %H:%M"), "cpu": cpu, "rounds": a.rounds, "reps": a.reps, "seed": a.seed,
           "boot": a.boot, "band": BAND, "quick": a.quick, "cflags": CFLAGS, "rows": ROW_DESC,
           "rule_fixed": rule_fixed, "first_measurement": first_measurement, "rule": rule_text(a.rounds, a.boot)}
    res["config"] = cfg
    res["analysis"] = an
    res["biases"] = BIASES
    (out / "round3.json").write_text(json.dumps(res, indent=1))
    (out / "round3.md").write_text(report_md(res, cfg, an))
    print(f"wrote {out / 'round3.md'} and {out / 'round3.json'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
