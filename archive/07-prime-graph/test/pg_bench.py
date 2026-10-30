"""Light benchmark of the prime-graph prototype (pg.c) against GMP mpq_t, FLINT fmpq, the checked int64 vector form
and FLINT fmpz over a shared denominator.  Every result is checked against Python's fractions.

    python3 pg_bench.py                 # the measured run: writes results/pg-light.md and results/pg-light.json
    python3 pg_bench.py --quick         # a smoke run on small sizes, written to build/quick/ (not results/)

Each round runs every contender once per job, in an order rotated per round.  Each invocation runs --reps reps in
one warm process; every rep re-enters the values from the raw inputs (pg also resets its graph), and the invocation
reports the minimum of each phase over its reps.  The tables give the medians over the rounds.  Round 1 checks
every element and measures the share of outputs not in lowest terms; later rounds check a random sample.
Before the rounds, a separate calibration run picks the faster FLINT form of a*b + c (fmpq_mul + fmpq_add, or
fmpq_set + fmpq_addmul) per job; the calibration timings are not reported as results.
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
from fractions import Fraction
from math import gcd
from pathlib import Path

import numpy as np
import sympy

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
BIN = BUILD / "pg_bench"
SRCS = [HERE / "bench.c", HERE / "pg.c"]
DEPS = SRCS + [HERE / "pg.h"]
CFLAGS = ["-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination"]
CONTENDERS = {"pg": "pg, the prototype: reduces at entry (inside its timed entry), results in lowest terms",
              "pgu": "pg variant, documented unreduced: no entry reduction, uniform arrays stay one group "
                     "(no per-element cancellation), same-group adds skip reduction",
              "vec": "vector form: checked int64 over one shared denominator (unreduced results)",
              "mpq": "GMP mpq_t per element (mpq_set_si + mpq_canonicalize at entry)",
              "fmpq": "FLINT fmpq per element (fmpq_set_si at entry); a*b + c in the calibrated faster form",
              "fz": "FLINT fmpz over one shared denominator; 'red' is its timed reduction of the results to lowest "
                    "terms (fmpq_set_fmpz_frac), not included in total"}
UNREDUCED = {"vec", "pgu"}  # their ratios are footnoted: their outputs are not in lowest terms
SAMPLE = 100_000


def flint_root():
    """FLINT_ROOT, else the native rebuild, else the BLAS build (as in 04's p2_elem_run.py)."""
    for r in (os.environ.get("FLINT_ROOT"), "/home/claude/flintblas/inst-native", "/home/claude/flintblas/inst"):
        if r and Path(r, "lib").exists():
            return r
    raise SystemExit("no FLINT install found")


def build(force=False):
    BUILD.mkdir(exist_ok=True)
    if force or not BIN.exists() or BIN.stat().st_mtime < max(d.stat().st_mtime for d in DEPS):
        root = flint_root()
        subprocess.run(["clang", *CFLAGS, f"-I{root}/include", "-o", str(BIN), *map(str, SRCS), f"-L{root}/lib",
                        "-lflint", "-lgmp", f"-Wl,-rpath,{root}/lib"], check=True)
    return BIN


# ---------------------------------------------------------------------------------------------------- workloads

def dec(rng, n, lo, hi):
    """Three-place decimals in (lo, hi), entered as k/1000."""
    return rng.integers(lo * 1000 + 1, hi * 1000, size=n, dtype=np.int64), np.full(n, 1000, dtype=np.uint64)


def mixed(rng, n, dmax, nmax):
    return (rng.integers(-nmax + 1, nmax, size=n, dtype=np.int64),
            rng.integers(1, dmax + 1, size=n, dtype=np.int64).astype(np.uint64))


def workloads(quick):
    s = (lambda n: max(n // 50, 1000)) if quick else (lambda n: n)
    return {
        "W1": dict(desc="decimal column: sum of three-place decimals in (-10^4, 10^4)", n=s(1_000_000),
                   gen=lambda r, n: {"a": dec(r, n, -10_000, 10_000)}, jobs=["sum"]),
        "W2": dict(desc="decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4)", n=s(1_000_000),
                   gen=lambda r, n: {k: dec(r, n, -10_000, 10_000) for k in "abc"}, jobs=["fma"]),
        "W3": dict(desc="mixed small denominators 1..12, numerators |n| < 10^6", n=s(1_000_000),
                   gen=lambda r, n: {k: mixed(r, n, 12, 10**6) for k in "abc"}, jobs=["sum", "fma"]),
        "W4": dict(desc="unrelated denominators 1..1000, numerators |n| < 10^6", n=s(200_000),
                   gen=lambda r, n: {k: mixed(r, n, 1000, 10**6) for k in "abc"}, jobs=["sum", "fma"]),
        "W5": dict(desc="single values: independent pairs, denominators 1..12, numerators |n| < 10^6", n=s(1_000_000),
                   gen=lambda r, n: {k: mixed(r, n, 12, 10**6) for k in "ab"}, jobs=["add", "mul"]),
        "W6": dict(desc="growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2)",
                   n=s(1_000_000), K=4,
                   gen=lambda r, n: {"a": dec(r, n, -10, 10), "b": dec(r, n, -2, 2), "c": dec(r, n, -10, 10)},
                   jobs=["chain"]),
        "W7": dict(desc="32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit "
                        "primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12", n=s(200_000),
                   gen=w7, jobs=["fma"]),
    }


def w7(rng, n):
    """Denominators that reach decomposition stages 3 (Miller-Rabin on 40-58-bit primes) and 4 (Pollard rho on
    semiprimes whose factors exceed 2^16); a and c share factors so that most a*b + c fit 64 bits."""
    nxt = lambda lo, hi: int(sympy.nextprime(int(rng.integers(lo, hi))))
    small = [nxt(2**16, 2**20) for _ in range(64)]
    big = [nxt(2**40, 2**58) for _ in range(8)]
    semi = [nxt(2**24, 2**29) * nxt(2**24, 2**29) for _ in range(24)]
    kind = rng.integers(0, 10, n).tolist()
    ijk = rng.integers(0, 64, (n, 3)).tolist()
    coin = rng.integers(0, 2, n).tolist()
    pick = rng.integers(0, 24, n).tolist()
    da, dc = [], []
    for t, (i, j, k), c, q in zip(kind, ijk, coin, pick):
        if t < 6:  # p*q, and c over p*r or over the same p*q
            d = small[i] * small[j]
            da.append(d); dc.append(small[i] * small[k] if c else d)
        elif t < 8:
            da.append(big[q % 8]); dc.append(big[q % 8])
        else:
            da.append(semi[q]); dc.append(semi[q])
    num = lambda m: rng.integers(-m + 1, m, size=n, dtype=np.int64)
    return {"a": (num(10**6), np.array(da, dtype=np.uint64)),
            "b": (num(100), rng.integers(1, 13, size=n, dtype=np.int64).astype(np.uint64)),
            "c": (num(10**6), np.array(dc, dtype=np.uint64))}


def contenders_for(wl, job):
    c = ["pg", "vec", "mpq", "fmpq", "fz"]
    if wl in ("W1", "W2", "W6"):  # all inputs share one denominator: the documented-unreduced uniform variant
        c.insert(1, "pgu")
    return c


def write_inputs(d, arrays):
    d.mkdir(parents=True, exist_ok=True)
    for k, (num, den) in arrays.items():
        with open(d / f"{k}.bin", "wb") as f:
            f.write(np.int64(len(num)).tobytes())
            f.write(np.ascontiguousarray(num, dtype=np.int64).tobytes())
            f.write(np.ascontiguousarray(den, dtype=np.uint64).tobytes())


def fracs(arr):
    num, den = arr
    return [Fraction(int(a), int(b)) for a, b in zip(num.tolist(), den.tolist())]


def expected(job, arrays, K=0):
    """The exact answer, computed with Python's fractions."""
    a = fracs(arrays["a"])
    if job == "sum":
        return [sum(a, Fraction(0))]
    b = fracs(arrays["b"])
    if job == "add":
        return [x + y for x, y in zip(a, b)]
    if job == "mul":
        return [x * y for x, y in zip(a, b)]
    c = fracs(arrays["c"])
    if job == "fma":
        return [x * y + z for x, y, z in zip(a, b, c)]
    if job == "chain":
        x = a
        for _ in range(K):
            x = [u * v + w for u, v, w in zip(x, b, c)]
        return x
    raise ValueError(job)


# ---------------------------------------------------------------------------------------------------- checking

OVF_LINE = "0 0"  # how pg writes a value whose overflow it detected
MAX_OVF = 0.001   # a row with a larger share of detected overflows fails


def check(lines, exp, idx=None, ovf=None):
    """(ok, message): every checked line "num den" must equal the expected fraction, with den > 0.  If ovf is a
    dict, a detected-overflow line (pg's "0 0") is counted there instead of failing: ovf["n"] counts them and
    ovf["fit"] those whose exact value would have fit pg's range (int64 numerator, denominator below 2^63).  The
    row still fails if any overflow was false (the value fit) or if more than MAX_OVF of the checked elements
    overflowed."""
    if len(lines) != len(exp):
        return False, f"{len(lines)} results, expected {len(exp)}"
    idx = range(len(exp)) if idx is None else idx
    n = 0
    for i in idx:
        if ovf is not None and lines[i] == OVF_LINE:
            e = exp[i]
            ovf["n"] = ovf.get("n", 0) + 1
            ovf["fit"] = ovf.get("fit", 0) + (-2**63 <= e.numerator < 2**63 and e.denominator < 2**63)
            continue
        parts = lines[i].split()
        if len(parts) != 2:
            return False, f"element {i}: malformed {lines[i]!r}"
        p, q = int(parts[0]), int(parts[1])
        e = exp[i]
        if q <= 0 or p * e.denominator != e.numerator * q:
            return False, f"element {i}: got {p}/{q}, expected {e}"
        n += 1
    if ovf:
        k, total = ovf.get("n", 0), len(idx)
        if ovf.get("fit", 0):
            return False, f"{ovf['fit']} false overflows: the exact value fits int64 / den < 2^63"
        if k > MAX_OVF * total:
            return False, f"{k} of {total} elements overflowed (limit {MAX_OVF:.1%})"
    return True, f"{n} checked"


def unreduced_pct(lines):
    """Share of "num den" lines not in lowest terms (gcd > 1, or a nonpositive denominator)."""
    bad = 0
    lines = [l for l in lines if l != OVF_LINE]  # detected overflows are reported separately
    for l in lines:
        p, q = l.split()
        p, q = int(p), int(q)
        bad += q <= 0 or gcd(p, q) != 1
    return 100.0 * bad / max(len(lines), 1)


def parse_stats(text):
    return {k: (int(v) if v.isdigit() else v) for k, v in (t.split("=", 1) for t in text.split())}


def run_one(binary, con, job, d, reps, K, out):
    args = [str(binary), con, job, str(d), str(out), str(reps)] + ([str(K)] if job == "chain" else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=1800)
    if r.returncode == 3:
        return {"unable": r.stdout.strip().removeprefix("unable ")}
    if r.returncode != 0:
        raise RuntimeError(f"{con} {job} failed ({r.returncode}): {r.stderr.strip()}")
    return parse_stats(r.stdout)


# ---------------------------------------------------------------------------------------------------- report

def ms(ns):
    return f"{ns / 1e6:.2f}"


def ratio(x, y):
    return f"{x / y:.2f}" if y else "-"


def report_md(res, cfg):
    L = ["# Prime graph, light prototype: results", "",
         f"Generated {cfg['date']} by `pg_bench.py` on {cfg['cpu']}; clang 18 `{' '.join(CFLAGS[:3])}`; "
         f"FLINT {cfg['flint']}.  {cfg['rounds']} interleaved rounds; each invocation runs {cfg['reps']} reps in one "
         "warm process (every rep re-enters the values from the raw inputs; pg also resets its graph) and reports "
         "the minimum of each phase over its reps.  Tables give the median over rounds.  Times in ms.", "",
         "Columns: **entry** is conversion from the raw (num, den) inputs (pg: values entered, equal denominators "
         "merged, new ones gcd-refined, then every value reduced by divisibility tests after decomposition; "
         "vec/fz: the common denominator computed up front plus scaling; mpq: `mpq_set_si` + `mpq_canonicalize`; "
         "fmpq: `fmpq_set_si`); **decomp** is pg's explicit decomposition pass (entry cost, shown separately); "
         "**op** is the operation; **red** is fz's timed reduction of its results to lowest terms; **total** is the "
         "minimum over reps of entry + decomp + op within one rep (red excluded), so it can exceed the sum of the "
         "column minima.  **unreduced %** is the share of output values not in lowest terms, measured by the "
         "harness on every output of round 1.  Ratios are contender / pg (above 1: pg is faster); a ratio is '-' "
         "when either row failed its check.  † marks contenders whose outputs are unreduced (vec, pgu); ‡ marks "
         "ratios of a row (pg or pgu) that detected overflows on some elements: those elements are counted, not "
         "checked, and were not finished; a row fails if over 0.1% overflowed or any overflow was false.", ""]
    for c, d in CONTENDERS.items():
        L.append(f"- `{c}`: {d}")
    if res.get("calibration"):
        L += ["", "FLINT a*b + c form, chosen on a separate calibration run (op ms, not reported results): "
              + "; ".join(f"{k}: mul+add {ms(v['fmpq'])}, set+addmul {ms(v['fmpq-am'])} -> {v['chosen']}"
                          for k, v in res["calibration"].items())]
    L.append("")
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            L += [f"## {wl} {job}: {w['desc']}", "", f"n = {w['n']:,}" + (f", K = {w['K']}" if w.get("K") else ""), "",
                  "| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |",
                  "|---|---:|---:|---:|---:|---:|---:|---:|---:|---|"]
            pg = jr.get("pg", {})
            pg_ok = "median" in pg and pg["ok"]
            for c, cr in jr.items():
                if "unable" in cr:
                    L.append(f"| {c} | unable: {cr['unable']} | | | | | | | | - |")
                    continue
                m = cr["median"]
                good = pg_ok and cr["ok"]
                mark = " †" if c in UNREDUCED else ""
                own = (cr.get("ovf_elems") or {}).get("n", 0)  # this contender's own detected overflows
                rm = mark + (" ‡" if own else "")
                ro = ratio(m["op"], pg["median"]["op"]) + rm if good else "-"
                rt = ratio(m["total"], pg["median"]["total"]) + rm if good else "-"
                L.append(f"| {c}{mark} | {ms(m['conv'])} | {ms(m['decomp'])} | {ms(m['op'])} | {ms(m['red'])} | "
                         f"{ms(m['total'])} | {ro} | {rt} | {cr['unreduced']:.2f} | "
                         f"{'ok' if cr['ok'] else 'FAIL: ' + cr['msg']} |")
            L.append("")
            for c in ("pg", "pgu"):
                if c in jr and "last" in jr[c]:
                    p = jr[c]["last"]
                    oe = jr[c].get("ovf_elems") or {}
                    L.append(f"{c}: overflows detected {p['ovf']} (output elements left as overflowed: "
                             f"{oe.get('n', 0)}, of which {oe.get('fit', 0)} have an exact value that fits 64 bits); decomposition {ms(jr[c]['median']['decomp'])} ms, "
                             f"nodes split by it {p['splits']}; live nodes {p['nodes']}; peak groups {p['groups']:,}; "
                             f"node + group + denominator-map memory {p['table_bytes'] / 2**20:.1f} MiB.  ")
                elif c in jr and "unable" in jr[c]:
                    L.append(f"{c}: unable: {jr[c]['unable']}.  ")
            L.append("")
    L += ["## Simplifications that may bias the results", ""] + [f"- {b}" for b in BIASES] + [""]
    return "\n".join(L)


BIASES = [
    "Reduction: pg reduces every value at entry (after decomposition, inside its timed entry), and its add, sub, "
    "same-group add and mul cancel by divisibility tests on prime nodes, so its outputs are in lowest terms; the "
    "harness's unreduced % column verifies this per output.  Column sums add integers within each group without "
    "reducing, then reduce once while combining the group sums.  pgu and vec are unreduced by design (†), and "
    "fz's reduction is timed separately as 'red'.",
    "Pair cache: pg caches a plan per pair of group ids (common-denominator group, scale factors, merged edges) in a "
    "2^16-entry direct-mapped cache, and caches 'weight lowered by one' transitions per group.  Few distinct groups "
    "(W1-W3, W5, W6) favour pg; W4 and W7 have many and miss often.",
    "pg's group table only grows (no collection); its memory (reported per job) is not charged to any op.",
    "Array layout: pg arrays are one numerator array plus one group id per element, not physically separate "
    "per-group vectors; only an array whose elements all share one group takes the vector-form loops (pgu).",
    "One missing speed-up: add tests every node of the result for cancellation, not only nodes where both weights "
    "are equal (the only ones that can cancel when the inputs are reduced).  This makes pg slower, not faster.",
    "vec and fz are given the common denominator of all inputs; vec reports 'unable' rather than widening, and pg "
    "reports 'unable' when a column sum's common denominator overflows 64 bits.",
    "Entry for pg merges equal denominators through a hash and gcd-refines each new one against every live node (a "
    "linear scan); inputs with few distinct denominators make that cheap.  For small denominators the refinement "
    "already splits nodes into primes, so that cost lands in entry and decomposition looks almost free.",
    "Inline edge lists of single values are rewritten after a split in one sync pass at the end of decomposition "
    "(ops read edges through the group id, so they are never stale where used).",
    "Warm processes: every rep re-enters the values from the raw inputs, but the reps run in one process, so "
    "allocator state and CPU caches are warm for every contender alike, and the minimum over reps is reported.",
    "An overflow inside a one-group (pgu) array loop poisons the whole result array.",
]


# ---------------------------------------------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rounds", type=int, default=5)
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--quick", action="store_true", help="small sizes; output to build/quick/ unless --out")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--only", help="comma-separated workloads, e.g. W1,W3")
    ap.add_argument("--seed", type=int, default=20261027)
    ap.add_argument("--force-build", action="store_true")
    a = ap.parse_args(argv)
    out = a.out or (BUILD / "quick" if a.quick else HERE / "results")
    out.mkdir(parents=True, exist_ok=True)
    binary = build(a.force_build)
    W = workloads(a.quick)
    if a.only:
        W = {k: v for k, v in W.items() if k in a.only.split(",")}
    data = BUILD / ("data-quick" if a.quick else "data")
    rng = np.random.default_rng(a.seed)
    prep = {}
    for wl, w in W.items():
        t = time.time()
        arrays = w["gen"](rng, w["n"])
        d = data / wl
        write_inputs(d, arrays)
        prep[wl] = (d, {job: expected(job, arrays, w.get("K", 0)) for job in w["jobs"]})
        print(f"{wl}: inputs and expected values in {time.time() - t:.1f} s", flush=True)
    res = {"workloads": {wl: {"desc": w["desc"], "n": w["n"], "K": w.get("K"), "jobs": {}} for wl, w in W.items()}}
    # calibration (separate from the reported rounds): the faster FLINT form of a*b + c, per job
    binname, calib = {}, {}
    for wl, w in W.items():
        for job in w["jobs"]:
            if job != "fma":
                continue
            o = data / wl / "calib.txt"
            t = {f: run_one(binary, f, job, prep[wl][0], a.reps, 0, o)["op"] for f in ("fmpq", "fmpq-am")}
            o.unlink()
            chosen = min(t, key=t.get)
            binname[(wl, job)] = chosen
            calib[f"{wl} {job}"] = {**t, "chosen": "set+addmul" if chosen == "fmpq-am" else "mul+add"}
    res["calibration"] = calib
    raw = {}
    srng = random.Random(a.seed)
    failed = False
    for rnd in range(a.rounds):
        for wl, w in W.items():
            d, exps = prep[wl]
            for job in w["jobs"]:
                cons = contenders_for(wl, job)
                k = rnd % len(cons)
                for con in cons[k:] + cons[:k]:
                    o = data / wl / f"out-{con}-{job}.txt"
                    b = binname.get((wl, job), con) if con == "fmpq" else con
                    st = run_one(binary, b, job, d, a.reps, w.get("K", 0), o)
                    r = raw.setdefault((wl, job, con), {"rounds": [], "ok": True, "msg": "", "unreduced": None})
                    if "unable" in st:
                        r["unable"] = st["unable"]
                        continue
                    exp = exps[job]
                    lines = o.read_text().splitlines()
                    idx = None if rnd == 0 or len(exp) <= SAMPLE else srng.sample(range(len(exp)), SAMPLE)
                    ov = {} if con in ("pg", "pgu") else None
                    ok, msg = check(lines, exp, idx, ov)
                    if rnd == 0 and ov is not None:
                        r["ovf_elems"] = ov
                    if r["unreduced"] is None:
                        r["unreduced"] = unreduced_pct(lines)
                    if not ok:
                        r["ok"], r["msg"] = False, f"round {rnd + 1}: {msg}"
                        failed = True
                        print(f"  CHECK FAILED {wl} {job} {con}: {msg}", flush=True)
                    r["rounds"].append(st)
                    o.unlink()
        print(f"round {rnd + 1}/{a.rounds} done", flush=True)
    for (wl, job, con), r in raw.items():
        jr = res["workloads"][wl]["jobs"].setdefault(job, {})
        if "unable" in r:
            jr[con] = {"unable": r["unable"]}
            continue
        med = {k: statistics.median(s[k] for s in r["rounds"]) for k in ("conv", "decomp", "op", "red", "total")}
        jr[con] = {"median": med, "ok": r["ok"], "msg": r["msg"], "unreduced": r["unreduced"],
                   "ovf_elems": r.get("ovf_elems"),
                   "rounds": r["rounds"], "last": r["rounds"][-1]}
    try:
        fl = Path(flint_root(), "lib").resolve()
        flint = next((p.name.removeprefix("libflint.so.") for p in fl.glob("libflint.so.*.*")), "?")
    except Exception:
        flint = "?"
    cpu = next((l.split(":", 1)[1].strip() for l in Path("/proc/cpuinfo").read_text().splitlines()
                if l.startswith("model name")), platform.processor())
    cfg = {"date": time.strftime("%Y-%m-%d %H:%M"), "cpu": cpu, "flint": flint, "rounds": a.rounds, "reps": a.reps,
           "seed": a.seed, "quick": a.quick, "cflags": CFLAGS, "flint_root": flint_root()}
    res["config"] = cfg
    res["biases"] = BIASES
    (out / "pg-light.json").write_text(json.dumps(res, indent=1))
    (out / "pg-light.md").write_text(report_md(res, cfg))
    print(f"wrote {out / 'pg-light.md'} and {out / 'pg-light.json'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
