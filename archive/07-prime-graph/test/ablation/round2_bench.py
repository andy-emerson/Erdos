"""Round 2 of the prime-graph ablation: a deliberate combination, single values by size, and the seen-prime dictionary.

    python3 round2_bench.py             # the measured run: writes results/round2.md and results/round2.json
    python3 round2_bench.py --quick     # a smoke run on small sizes, written to build/quick-r2/ (not results/)

The baseline is still V0', the control (grouping removed).  Ratios are V0' / x on the total time, final reduction
included: above 1, the row is faster than the control.  The same-code noise floor comes from an identical rerun of V0'
(row v0p-rerun: the same binary, run again as its own row in every round).

  Part A  V34 = V0' + V3's array denominator with exceptions + V4's deferred reduction (also for the exceptions and
          the graph path), both rules for D, with V3 (both rules) and V4 rerun on the same jobs: W1-W4, W6-W10.
  Part B  single values at four operand sizes (S8, S20, S30, S30d; add and mul, independent pairs) for V0', V1, V5,
          and a 32-bit build (-DAB_32: int32 numerators, denominators below 2^31, int64 intermediates) of V0', V1, V5
          on S8 and S20.
  Part C  V6 = V0' + the seen-prime dictionary as the entry procedure, on every job: W1-W10 and Part B's sizes.

Workloads W1-W10 are ablation_bench.py's (pg_bench.py's W1-W7, unchanged), generated from the same seed.  The method
is ablation_bench.py's: interleaved rounds with the order rotated per round, --reps warm reps per invocation (each
re-enters the values from the raw inputs and resets the node table; the minimum of each phase is kept), medians over
rounds.  Every row is checked against Python's fractions with the existing strict rules: round 1 checks every element
and fails a row with any output not in lowest terms; later rounds check a sample; a detected overflow ("0 0") is
counted, and a row fails if any overflow was false (its exact value fits the build's range) or over 0.1% overflowed.
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
from pathlib import Path

import numpy as np

import ablation_bench as ab  # W1-W10, expected values, the strict checks, the report helpers
import pg_bench

HERE = ab.HERE
TEST = ab.TEST
BUILD = HERE / "build" / "r2"
CFLAGS = ab.CFLAGS
DEPS = ab.DEPS + [HERE / "ab_v6.c"]
BASE = "v0p"

# binaries: name -> (VARIANT macro, extra flags)
BINARIES = {
    "v0p": ("VAR_V0P", []), "v1": ("VAR_V1", []), "v3": ("VAR_V3", []), "v4": ("VAR_V4", []), "v5": ("VAR_V5", []),
    "v34": ("VAR_V34", []), "v6": ("VAR_V6", []),
    "v0p_32": ("VAR_V0P", ["-DAB_32"]), "v1_32": ("VAR_V1", ["-DAB_32"]), "v5_32": ("VAR_V5", ["-DAB_32"]),
}
# rows, in table order: name -> (binary, description, environment, width)
STRATEGIES = {
    "v0p": ("v0p", "V0' (control): grouping removed; every op merges its operands' edges, scales and reduces", {}, 64),
    "v0p-rerun": ("v0p", "the control again: the same binary as v0p, run as its own row (the same-code noise floor)",
                  {}, 64),
    "v1": ("v1", "V1 = V0' + per-value denominator check (equal: one integer op; one divides the other: one scaling)",
           {}, 64),
    "v3-lcm": ("v3", "V3 = V0' + array denominator D with sparse exceptions, D from the lcm (when below 2^63)",
               {"AB_V3_POLICY": "lcm"}, 64),
    "v3-freq": ("v3", "V3 with D from the most frequent denominator", {"AB_V3_POLICY": "freq"}, 64),
    "v4": ("v4", "V4 = V0' + deferred reduction (overflow: reduce the operands and retry; one final reduction)", {}, 64),
    "v34-lcm": ("v34", "V34 = V0' + V3 (D from the lcm) + V4: exceptions entered unreduced; the graph path takes "
                       "operands as they stand (a dense slot as n/D) with V4's ops (overflow: reduce and retry); "
                       "results stored unreduced; one timed final reduction of dense elements and exceptions",
                {"AB_V3_POLICY": "lcm"}, 64),
    "v34-freq": ("v34", "V34 with D from the most frequent denominator", {"AB_V3_POLICY": "freq"}, 64),
    "v5": ("v5", "V5 = V0' + interned denominator objects with reference counts", {}, 64),
    "v6": ("v6", "V6 = V0' + the seen-prime dictionary as the entry procedure (trial division, then the large "
                 "primes already seen, then Miller-Rabin and rho on the rest; a prime splits its holder only when "
                 "seen again; edges by dictionary lookups)",
           {}, 64),
    "v0p-32": ("v0p_32", "V0' built with -DAB_32 (int32 numerators, denominators below 2^31, int64 intermediates)",
               {}, 32),
    "v1-32": ("v1_32", "V1 built with -DAB_32", {}, 32),
    "v5-32": ("v5_32", "V5 built with -DAB_32", {}, 32),
}
PART_A = ["v0p", "v0p-rerun", "v3-lcm", "v3-freq", "v4", "v34-lcm", "v34-freq", "v6"]
W5_ROWS = ["v0p", "v0p-rerun", "v6"]
PART_B = ["v0p", "v0p-rerun", "v1", "v5", "v6"]
PART_B32 = ["v0p-32", "v1-32", "v5-32"]
SAMPLE = ab.SAMPLE


def binary(name):
    return BUILD / f"abl_{name}"


def build(force=False):
    BUILD.mkdir(parents=True, exist_ok=True)
    newest = max(d.stat().st_mtime for d in DEPS)
    for name, (macro, extra) in BINARIES.items():
        b = binary(name)
        if force or not b.exists() or b.stat().st_mtime < newest:
            subprocess.run(["clang", *CFLAGS, *extra, f"-DVARIANT={macro}", "-o", str(b), str(HERE / "abl_bench.c"),
                            str(TEST / "pg.c")], check=True)


# ---------------------------------------------------------------------------------------------------- workloads

def sizes(quick):
    """Part B: independent pairs at five operand sizes.  Bounds for one op on two operands: S15 fits int32
    (|a.num * b.den + b.num * a.den| < 2^24, |a.num * b.num| < 2^30, denominators at most 2^16); S30 fits int64
    (< 2^46 and < 2^60, denominators below 2^30)."""
    n = 20_000 if quick else 1_000_000

    def pairs(nmax, dens):
        return lambda r, n: {k: (r.integers(-nmax + 1, nmax, size=n, dtype=np.int64), dens(r, n).astype(np.uint64))
                             for k in "ab"}

    def s30d(r, n):  # one fixed set of 50 denominators, drawn once from the run's generator, shared by a and b
        D = r.choice(np.arange(2, 2**15 + 1, dtype=np.int64), size=50, replace=False).astype(np.uint64)
        return {k: (r.integers(-2**30 + 1, 2**30, size=n, dtype=np.int64), D[r.integers(0, 50, size=n)])
                for k in "ab"}

    return {
        "S8": dict(desc="numerators |n| < 2^7, denominators 1..12", n=n,
                   gen=pairs(2**7, lambda r, n: r.integers(1, 13, size=n)), jobs=["add", "mul"], w32=True),
        "S15": dict(desc="numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32)", n=n,
                    gen=pairs(2**15, lambda r, n: r.integers(1, 257, size=n)), jobs=["add", "mul"], w32=True),
        "S20": dict(desc="numerators |n| < 2^20, denominators 1..1000", n=n,
                    gen=pairs(2**20, lambda r, n: r.integers(1, 1001, size=n)), jobs=["add", "mul"], w32=True),
        "S30": dict(desc="numerators |n| < 2^30, denominators uniform in 1..2^15", n=n,
                    gen=pairs(2**30, lambda r, n: r.integers(1, 2**15 + 1, size=n)), jobs=["add", "mul"], w32=False),
        "S30d": dict(desc="numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15", n=n,
                     gen=s30d, jobs=["add", "mul"], w32=False),
    }


def plan(quick):
    """[(workload, spec, {job: [rows]})] in run order."""
    out = []
    for wl, w in ab.workloads(quick).items():
        rows = W5_ROWS if wl == "W5" else PART_A
        out.append((wl, w, {job: list(rows) for job in w["jobs"]}))
    for wl, w in sizes(quick).items():
        rows = PART_B + (PART_B32 if w["w32"] else [])
        out.append((wl, w, {job: list(rows) for job in w["jobs"]}))
    return out


# ---------------------------------------------------------------------------------------------------- checks

def check_rows32(lines, exp, idx=None, ovf=None):
    """pg_bench.check for the 32-bit build: the same value check, with the overflow rule on the 32-bit range (an
    overflow is false if the exact value has an int32 numerator and a denominator below 2^31), the same 0.1% limit,
    and every output inside the 32-bit range."""
    if len(lines) != len(exp):
        return False, f"{len(lines)} results, expected {len(exp)}"
    idx = range(len(exp)) if idx is None else idx
    o = {"n": 0, "fit": 0}
    rest = []
    for i in idx:
        if lines[i] == pg_bench.OVF_LINE:
            e = exp[i]
            o["n"] += 1
            o["fit"] += -2**31 <= e.numerator < 2**31 and e.denominator < 2**31
        else:
            rest.append(i)
    if ovf is not None:
        ovf.update(o)
    ok, msg = pg_bench.check(lines, exp, rest)
    if not ok:
        return ok, msg
    for i in rest:
        p, q = map(int, lines[i].split())
        if not (-2**31 <= p < 2**31 and 0 < q < 2**31):
            return False, f"element {i}: {p}/{q} outside the 32-bit range"
    if o["fit"]:
        return False, f"{o['fit']} false overflows: the exact value fits int32 / den < 2^31"
    if o["n"] > pg_bench.MAX_OVF * len(idx):
        return False, f"{o['n']} of {len(idx)} elements overflowed (limit {pg_bench.MAX_OVF:.1%})"
    return True, f"{len(rest)} checked"


def judge(v, lines, exp, idx, first):
    """ablation_bench.judge (value check, overflow rule, and in round 1 the unreduced rule), with the 32-bit range for
    the 32-bit rows."""
    if STRATEGIES[v][3] == 64:
        return ab.judge(v, lines, exp, idx, first)
    ov = {}
    ok, msg = check_rows32(lines, exp, idx, ov)
    extra = {}
    if first:
        extra = {"ovf_elems": ov, "unreduced": ab.unreduced_pct(lines), "out_dens": ab.out_dens(lines)}
        if ok and extra["unreduced"] > 0:
            ok, msg = False, f"{extra['unreduced']:.4f}% of outputs not in lowest terms"
    return ok, msg, extra


def run_one(v, job, d, reps, K, out, env=None):
    b, _, senv, _ = STRATEGIES[v]
    e = dict(os.environ if env is None else env)
    for k, x in senv.items():
        e.setdefault(k, x)
    args = [str(binary(b)), job, str(d), str(out), str(reps)] + ([str(K)] if job == "chain" else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=3600, env=e)
    if r.returncode == 3:
        return {"unable": r.stdout.strip().removeprefix("unable ")}
    if r.returncode != 0:
        raise RuntimeError(f"{v} {job} failed ({r.returncode}): {r.stderr.strip()}")
    return ab.parse_stats(r.stdout)


# ---------------------------------------------------------------------------------------------------- report

ms, ratio = ab.ms, ab.ratio


def structure(v, last):
    if not last:
        return ""
    if v.startswith("v34"):
        return ab.structure("v3-" + v.split("-")[1], last, {}) + f"; {last.get('early', 0):,} early reductions"
    if v == "v6":
        return (f"{last.get('v6_nodes', 0):,} nodes ({last.get('v6_multi', 0):,} holding several primes); splits "
                f"{last.get('v6_entry_splits', 0):,} at entry, {last.get('v6_op_splits', 0):,} in ops")
    if v in ("v0p", "v0p-rerun", "v1", "v0p-32", "v1-32"):
        return f"{last.get('nodes', 0):,} nodes; {last.get('splits', 0):,} decomposition splits"
    return ab.structure(v.removesuffix("-32"), last, {})


def ok_row(cr):
    return cr is not None and "median" in cr and cr["ok"]


def tot_ratio(jr, v, base=BASE):
    b, x = jr.get(base), jr.get(v)
    return b["median"]["total"] / x["median"]["total"] if ok_row(b) and ok_row(x) else None


def why_not(jr, v, base=BASE):
    """The label of a ratio cell that has no ratio: 'not run', 'unable' or 'fail' (row first, then its base)."""
    for r in (jr.get(v), jr.get(base)):
        if r is None:
            return "not run"
        if "unable" in r:
            return "unable"
        if not r.get("ok", False):
            return "fail"
    return None


def rcell(jr, v, base=BASE, key="total"):
    """V0' / x (key 'total'), or the op ratio (op + final, key 'op'); a label when there is no ratio."""
    w = why_not(jr, v, base)
    if w:
        return w
    b, x = jr[base]["median"], jr[v]["median"]
    if key == "op":
        return f"{(b['op'] + b['red']) / (x['op'] + x['red']):.2f}"
    return f"{b['total'] / x['total']:.2f}"


def noise_floor(res):
    r = []
    for w in res["workloads"].values():
        for jr in w["jobs"].values():
            x = tot_ratio(jr, "v0p-rerun")
            if x is not None:
                r.append(x)
    return r


def noise_floor_line(res):
    r = noise_floor(res)
    if not r:
        return "**Same-code noise floor:** not available (no job with both v0p and v0p-rerun checked)."
    return (f"**Same-code noise floor:** v0p-rerun is the control's binary run again as its own row, so its "
            f"V0' / x (total) on the {len(r)} jobs measures noise between identical code: {min(r):.2f} to "
            f"{max(r):.2f} (median {statistics.median(r):.2f}).  A ratio within that range of 1 counts as no difference.")


def med_stat(cr, k):
    v = [s[k] for s in cr.get("rounds", []) if isinstance(s.get(k), int)]
    return statistics.median(v) if v else None


LABELS = ("Cells without a ratio say why: **not run** (the row was not run on this job), **unable** (the row or the "
          "control could not compute the result within its range: a column sum beyond 64 bits), **fail** (the row "
          "or the control failed a check; the detail table gives the reason).")


def summary_md(res):
    L = []
    W = res["workloads"]
    # Part A
    L += ["## Part A: V3 + V4 (V34) against its parts", "",
          "V0' / x (total).  The last two columns compare V34 with V3 under the same rule for D: V3 total / V34 total "
          "(above 1, the combination is faster than V3 alone).  " + LABELS, "",
          "| job | V3 lcm | V3 freq | V4 | V34 lcm | V34 freq | V3 / V34 (lcm) | V3 / V34 (freq) |",
          "|---|---:|---:|---:|---:|---:|---:|---:|"]
    for wl, w in W.items():
        if wl.startswith("S") or wl == "W5":
            continue
        for job, jr in w["jobs"].items():
            c = [rcell(jr, v) for v in ("v3-lcm", "v3-freq", "v4", "v34-lcm", "v34-freq")]
            c += [rcell(jr, "v34-" + p, "v3-" + p) for p in ("lcm", "freq")]
            L.append(f"| {wl} {job} | " + " | ".join(c) + " |")
    # Part B
    sizes_ = [(wl, w) for wl, w in W.items() if wl.startswith("S")]
    rows = ("v1", "v5", "v6", "v0p-32", "v1-32", "v5-32")
    head = "| size, job | V1 | V5 | V6 | V0' 32-bit | V1 32-bit | V5 32-bit |"
    L += ["", "## Part B: single values by operand size", "",
          "Against the 64-bit control on the same job.  32-bit rows are the int32 build (`-DAB_32`).  " + LABELS, "",
          "**V0' / x (total).**", "", head, "|---|---:|---:|---:|---:|---:|---:|"]
    for wl, w in sizes_:
        for job, jr in w["jobs"].items():
            L.append(f"| {wl} {job} | " + " | ".join(rcell(jr, v) for v in rows) + " |")
    L += ["", "**V0' / x (op)**: op + final only, without entry (on S30 the control's total is almost all entry).", "",
          head, "|---|---:|---:|---:|---:|---:|---:|"]
    for wl, w in sizes_:
        for job, jr in w["jobs"].items():
            L.append(f"| {wl} {job} | " + " | ".join(rcell(jr, v, key="op") for v in rows) + " |")
    L += ["", "**The same strategy at both widths** (64-bit time / 32-bit time; above 1, the 32-bit build is faster), "
          "and the share of the 32-bit outputs that overflowed int32 (each one genuine: its exact value does not fit; "
          "over 0.1% fails the strict rule).", "",
          "| size, job | V0' 64 / 32 (total) | V1 64 / 32 (total) | V1 64 / 32 (op) | V5 64 / 32 (total) | "
          "V5 64 / 32 (op) | ovf V0' / V1 / V5 32-bit |", "|---|---:|---:|---:|---:|---:|---|"]
    for wl, w in sizes_:
        for job, jr in w["jobs"].items():
            if not any(v in jr for v in ("v0p-32", "v1-32", "v5-32")):
                continue
            ov = []
            for v in ("v0p-32", "v1-32", "v5-32"):
                cr = jr.get(v)
                ov.append(f"{100 * (cr.get('ovf_elems') or {}).get('n', 0) / w['n']:.2f}%" if cr and "median" in cr
                          else why_not(jr, v) or "")
            L.append(f"| {wl} {job} | {rcell(jr, 'v0p-32')} | {rcell(jr, 'v1-32', 'v1')} | "
                     f"{rcell(jr, 'v1-32', 'v1', 'op')} | {rcell(jr, 'v5-32', 'v5')} | {rcell(jr, 'v5-32', 'v5', 'op')} | "
                     + " / ".join(ov) + " |")
    # Part C
    L += ["", "## Part C: V6, the seen-prime dictionary", "",
          "Entry is conversion from the raw inputs.  V0': the gcd scan of every live node per new denominator, "
          "pg_factor on every node, the edge scan, then the fill (each value reduced).  V6: the **dictionary passes** "
          "(pass 1: the shared per-element denominator hash; per distinct denominator, trial division, exact "
          "division by the large primes already in the dictionary, Miller-Rabin and rho on what remains, dictionary "
          "lookups and splits; pass 2: each distinct denominator's edges by lookups), then the **fill** (each value "
          "reduced, and the edges of elements filled before a fill-time split rewritten).  Nodes are the live nodes "
          "after the run; splits count node splits at entry and in the operations.  " + LABELS, "",
          "| job | V0' / V6 (total) | V0' / V6 (op) | entry V0' ms | entry V6 ms | V6 dictionary ms | V6 fill ms | "
          "V0' / V6 (entry) | nodes V0' | nodes V6 | V6 nodes of several primes | V6 splits (entry / ops) |",
          "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|"]
    for wl, w in W.items():
        for job, jr in w["jobs"].items():
            b, x = jr.get(BASE), jr.get("v6")
            if x is None:
                continue
            why = why_not(jr, "v6")
            if why:
                L.append(f"| {wl} {job} | {why} | {why} | | | | | | | | | |")
                continue
            bl, xl = b["last"], x["last"]
            dct, fil = med_stat(x, "v6_dict_ns"), med_stat(x, "v6_fill_ns")
            L.append(f"| {wl} {job} | {rcell(jr, 'v6')} | {rcell(jr, 'v6', key='op')} | {ms(b['median']['entry'])} | "
                     f"{ms(x['median']['entry'])} | {ms(dct) if dct is not None else 'not run'} | "
                     f"{ms(fil) if fil is not None else 'not run'} | {b['median']['entry'] / x['median']['entry']:.2f} | "
                     f"{bl.get('nodes', 0):,} | {xl.get('v6_nodes', 0):,} | {xl.get('v6_multi', 0):,} | "
                     f"{xl.get('v6_entry_splits', 0):,} / {xl.get('v6_op_splits', 0):,} |")
    # Audits
    au = res.get("audits") or {}
    L += ["", "## Audits", "",
          "One untimed invocation per job for V6 and V34 with `AB_AUDIT=1` after the timed rounds: the live nodes are "
          "pairwise coprime, plus each layout's structural audit (V6: the dictionary and every edge re-expanded onto "
          "live nodes; V34: D, the exception list).", "",
          "| job | v34-lcm | v34-freq | v6 |", "|---|---|---|---|"]
    for wl in W:
        for job in W[wl]["jobs"]:
            a = au.get(wl, {}).get(job, {})
            if a:
                L.append(f"| {wl} {job} | " + " | ".join(a.get(v, "not run") for v in ("v34-lcm", "v34-freq", "v6")) + " |")
    return L


def report_md(res, cfg):
    L = ["# Prime graph: ablation, round 2", "",
         f"Generated {cfg['date']} by `round2_bench.py` on {cfg['cpu']}; clang 18 `{' '.join(CFLAGS[:3])}`.  "
         f"{cfg['rounds']} interleaved rounds; each invocation runs {cfg['reps']} reps in one warm process (every rep "
         "re-enters the values from the raw inputs and resets the node table) and reports the minimum of each phase "
         "over its reps.  Tables give the median over rounds.  Times in ms.", "",
         "**Baseline: V0', the control (grouping removed).** **V0' / x (total)** is the control's total over the row's "
         "total, final reduction included; **V0' / x (op)** is the control's op + final over the row's op + final.  "
         "Above 1, the row is faster than the control.  V34 deliberately combines two ideas (V3 and V4); every other "
         "row is the control plus one idea, as in the first round.", "",
         noise_floor_line(res), ""]
    L += summary_md(res)
    L += ["", "## Rows", ""] + [f"- `{v}`: {d}" for v, (_, d, _, _) in STRATEGIES.items()] + [""]
    L += ["Columns of the detail tables are those of `results/ablation.md`: **entry** includes decomposition (shown "
          "again as **decomp**; 0 for V6, which factors each distinct denominator as it enters, inside its "
          "dictionary pass); **final** is the timed final reduction (V3, V4, V34; 0 elsewhere); **total** is the minimum over "
          "reps of entry + op + final in one rep; **repr** is the measured peak of the representation's allocations; "
          "**unreduced %** is the share of outputs not in lowest terms (round 1, every output; any fails the row); "
          "overflowed elements (\"0 0\") are counted, not checked: a row fails if over 0.1% overflowed or any "
          "overflow was false; ‡ marks rows with some.", ""]
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            L += [f"## {wl} {job}: {w['desc']}", "", f"n = {w['n']:,}" + (f", K = {w['K']}" if w.get("K") else ""), "",
                  "| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | "
                  "out dens | structure | unreduced % | check |",
                  "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|"]
            base = jr.get(BASE, {})
            for v in STRATEGIES:
                cr = jr.get(v)
                if cr is None:
                    continue
                if "unable" in cr:
                    L.append(f"| {v} | unable: {cr['unable']} | | | | | unable | unable | | | | | | unable |")
                    continue
                m, last = cr["median"], cr["last"]
                mark = " ‡" if (cr.get("ovf_elems") or {}).get("n") else ""
                rt = rcell(jr, v) + ("" if why_not(jr, v) else mark)
                ro = rcell(jr, v, key="op") + ("" if why_not(jr, v) else mark)
                L.append(f"| {v} | {ms(m['entry'])} | {ms(m['decomp'])} | {ms(m['op'])} | {ms(m['red'])} | "
                         f"{ms(m['total'])} | {rt} | {ro} | {last.get('maxrss_kb', 0) / 1024:.0f} | "
                         f"{last.get('rep_bytes', 0) / 2**20:.1f} | {cr.get('out_dens', '')} | {structure(v, last)} | "
                         f"{cr['unreduced']:.2f} | {'ok' if cr['ok'] else 'FAIL: ' + cr['msg']} |")
            notes = []
            for v in STRATEGIES:
                cr = jr.get(v, {})
                if "last" in cr:
                    oe = cr.get("ovf_elems") or {}
                    if cr["last"].get("ovf") or oe.get("n"):
                        notes.append(f"{v}: overflows detected {cr['last'].get('ovf', 0)} (outputs left overflowed: "
                                     f"{oe.get('n', 0)}, of which {oe.get('fit', 0)} have an exact value that fits "
                                     f"the {STRATEGIES[v][3]}-bit range)")
            if "unable" in base:
                notes.append("V0' is unable here, so no ratios.")
            elif base and not base.get("ok", False):
                notes.append(f"V0' FAILED its check here ({base.get('msg')}), so no ratios.")
            L += [""] + [f"{n}  " for n in notes] + [""]
    L += ["## Simplifications that may bias the results", ""] + [f"- {b}" for b in BIASES] + [""]
    return "\n".join(L)


BIASES = [
    "Everything in the first round's bias notes (results/ablation.md) still holds for the rows it covers: the shared "
    "entry hash, strict 'no caching', warm processes, layouts, V3's plain single values.  The V0', V1, V3, V4 and V5 "
    "binaries are built from the same sources; the round-2 edits are behind VARIANT / AB_32 switches, and those five "
    "compile to the same machine code as before (checked with objdump when the edits were made).",
    "The noise floor is an identical rerun of V0' (the same binary), not V2, because round 2 does not run V2.  It "
    "covers every job, column sums included.",
    "V34 (Part A) is V3's code with V4's deferred ops on the graph path.  A result stored while unreduced becomes an "
    "exception when its unreduced denominator does not divide the result's D, even if its reduced one would; the final "
    "reduction then also reduces every exception.  An overflow on the graph path reduces both operands and retries "
    "with the reducing op ('early reductions'), so V34 never overflows where V3 would not.",
    "Part B's sizes are chosen so that every add and mul of two operands fits int64 (S30: numerators below 2^46 and "
    "2^60, denominators below 2^30), so the 64-bit rows should report no overflow.  S15 also fits int32 (below 2^24 "
    "and 2^30, denominators at most 2^16), so it is the size where the two widths compare cleanly.  The 32-bit "
    "build reads the same 64-bit input files and narrows at entry (a value outside int32 would be an overflow); its "
    "intermediates are int64.  S8 and S20 add fit int32 (0% overflowed in the smoke runs); S20 mul does not "
    "(products near 2^40: about 86% of its outputs overflow), so S20 mul's 32-bit rows fail the 0.1% rule and get "
    "no ratio; their times are for information only, since an overflowed element skips work.",
    "32-bit rows are compared with the 64-bit control (V0' / x), and V1-32 and V5-32 also with their own 64-bit "
    "builds (V1, V5) on S8, S15 and S20.",
    "S30's large V0' / x (total) for V6 is entry only: the control gcd-refines each new distinct denominator against "
    "every live node and scans every node for each denominator's edges, a cost that grows with the number of "
    "distinct denominators (here capped at 2^15, all of 1..2^15 likely present).  The op ratio table shows the ops "
    "alone.  S30d (50 distinct denominators) is the same operand size without that cost.",
    "V6's coarsest rule: a node is the product of the primes first seen together in one denominator, stored as that "
    "product; a node holding one prime is stored as the prime with the power on the edge weight (100 then 35 give "
    "nodes 2, 5, 7 with 100 = 2^2 * 5^2).  A prime splits its holder only when seen again, so this is coarser than "
    "primes but finer than the coarsest coprime base in general (100 and 1000 give 2 and 5, not 10).",
    "V6 factors each distinct denominator in three steps: trial division by the primes below 2^16 with the adaptive "
    "stop (a copy of pg.c's table, built by the same sieve, because pg.c keeps it static and is left unchanged); "
    "then, for a leftover of 2^32 or more, exact division by the primes above 2^16 already in the dictionary; then "
    "Miller-Rabin on what remains, and pg_factor (rho) only if that is composite.  The dictionary so holds every "
    "prime, and step 3 (gcd refinement of unfactored leftovers) never triggers.  On W7, whose 32-62-bit "
    "denominators have no factor below 2^16, every distinct denominator still runs the whole table (6,542 "
    "primes); V0' runs pg_factor only on its refined nodes.  Before this order, V6 ran Miller-Rabin and rho on all "
    "of W7's distinct denominators, which was most of its W7 entry cost (about 50 ms of dictionary passes, "
    "now about 17).",
    "V6's nodes need not be prime.  While a live node holds several primes, or an operand may name a dead node "
    "(after a split during the ops), fin, core_add and core_mul run hooked copies (ab_v6.c) that test such nodes' "
    "known primes by exact division and split them on a proper common part; otherwise they run the control's code unchanged, after one "
    "test of a global flag.  A split during the fill re-expands every distinct denominator's edges at once, and the "
    "elements filled before it are rewritten at the end of the entry (timed).  W7's semiprime denominators stay "
    "nodes of two primes (their primes are never seen apart), so W7's ops stay on the hooked copies: each "
    "reduction and cancellation against such a node tests its known primes by exact division (no gcd).  In a "
    "full-size check before the measured run, V6's W7 op phase took about 15.7 ms against V0' 13.7 ms.",
    "The fill keeps one V6-only test per element (has a split happened since the previous element?), to know which "
    "elements to rewrite.",
    "V6's dictionary passes are timed as pass 1 (which includes the per-element denominator hash shared with V0') "
    "plus pass 2; the fill is the rest of entry.  The split of V0' entry is not measured (its code is unchanged).",
    "Per-rep resets run outside the timer for every row: pg_reset-like node table resets, and V6's v6_reset (a "
    "memset of the 256 KB direct-mapped dictionary and of the hash).",
    "Table capacities carry over between the warm reps of one invocation for every row (the node table, the arrays "
    "and their edge pools, V5's object blocks, V6's dictionary hash, pools and metadata): only the first rep pays "
    "for growing them, and the minimum over reps is reported.",
    "V6's 'nodes' are its live nodes after the run (splits in the operations included); V0' 'nodes' likewise.",
]


# ---------------------------------------------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rounds", type=int, default=5)
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--quick", action="store_true", help="small sizes; output to build/quick-r2/ unless --out")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--only", help="comma-separated workloads, e.g. W1,S30")
    ap.add_argument("--rows", help="comma-separated subset of " + ",".join(STRATEGIES))
    ap.add_argument("--seed", type=int, default=20261027)
    ap.add_argument("--force-build", action="store_true")
    a = ap.parse_args(argv)
    out = a.out or (HERE / "build" / "quick-r2" if a.quick else HERE / "results")
    out.mkdir(parents=True, exist_ok=True)
    build(a.force_build)
    P = plan(a.quick)
    if a.only:
        P = [p for p in P if p[0] in a.only.split(",")]
    if a.rows:
        keep = a.rows.split(",")
        P = [(wl, w, {j: [v for v in rows if v in keep] for j, rows in jobs.items()}) for wl, w, jobs in P]
    data = HERE / "build" / ("r2-data-quick" if a.quick else "r2-data")
    rng = np.random.default_rng(a.seed)  # the same seed and order as ablation_bench.py for W1-W10, then Part B
    prep = {}
    for wl, w, _ in P:
        t = time.time()
        arrays = w["gen"](rng, w["n"])
        d = data / wl
        pg_bench.write_inputs(d, arrays)
        prep[wl] = (d, {job: ab.expected(job, arrays, w.get("K", 0)) for job in w["jobs"]})
        print(f"{wl}: inputs and expected values in {time.time() - t:.1f} s", flush=True)
    res = {"baseline": BASE, "workloads": {wl: {"desc": w["desc"], "n": w["n"], "K": w.get("K"), "jobs": {}}
                                           for wl, w, _ in P}}
    raw = {}
    srng = random.Random(a.seed)
    failed = False
    for rnd in range(a.rounds):
        for wl, w, jobs in P:
            d, exps = prep[wl]
            for job, rows in jobs.items():
                if not rows:
                    continue
                k = rnd % len(rows)
                for v in rows[k:] + rows[:k]:
                    o = data / wl / f"out-{v}-{job}.txt"
                    st = run_one(v, job, d, a.reps, w.get("K", 0), o)
                    r = raw.setdefault((wl, job, v), {"rounds": [], "ok": True, "msg": "", "unreduced": None})
                    if "unable" in st:
                        r["unable"] = st["unable"]
                        continue
                    exp = exps[job]
                    lines = o.read_text().splitlines()
                    first = r["unreduced"] is None
                    idx = None if first or len(exp) <= SAMPLE else srng.sample(range(len(exp)), SAMPLE)
                    ok, msg, extra = judge(v, lines, exp, idx, first)
                    r.update(extra)
                    if not ok and r["ok"]:
                        r["ok"], r["msg"] = False, f"round {rnd + 1}: {msg}"
                        if STRATEGIES[v][3] == 64 or "overflowed (limit" not in msg:
                            failed = True  # a 32-bit row failing only the 0.1% overflow limit is a finding, not an error
                        print(f"  CHECK FAILED {wl} {job} {v}: {msg}", flush=True)
                    r["rounds"].append(st)
                    o.unlink()
        print(f"round {rnd + 1}/{a.rounds} done", flush=True)
    audits = {}  # one untimed AB_AUDIT invocation per job for V6 and V34 (node invariant + structural audit)
    for wl, w, jobs in P:
        d, _ = prep[wl]
        for job, rows in jobs.items():
            for v in rows:
                if v not in ("v6", "v34-lcm", "v34-freq"):
                    continue
                o = data / wl / f"audit-{v}-{job}.txt"
                e = dict(os.environ, AB_AUDIT="1")
                st = run_one(v, job, d, 1, w.get("K", 0), o, env=e)
                res_a = "unable" if "unable" in st else ("pass" if st.get("audit") == "ok" else f"fail: {st.get('audit')}")
                audits.setdefault(wl, {}).setdefault(job, {})[v] = res_a
                if res_a.startswith("fail"):
                    failed = True
                    print(f"  AUDIT FAILED {wl} {job} {v}: {st.get('audit')}", flush=True)
                if o.exists():
                    o.unlink()
    res["audits"] = audits
    for (wl, job, v), r in raw.items():
        jr = res["workloads"][wl]["jobs"].setdefault(job, {})
        if "unable" in r:
            jr[v] = {"unable": r["unable"]}
            continue
        med = {k: statistics.median(s[k] for s in r["rounds"]) for k in ("entry", "decomp", "op", "red", "total")}
        jr[v] = {"median": med, "ok": r["ok"], "msg": r["msg"], "unreduced": r["unreduced"],
                 "ovf_elems": r.get("ovf_elems"), "out_dens": r.get("out_dens"),
                 "rounds": r["rounds"], "last": r["rounds"][-1]}
    for w in res["workloads"].values():  # table order: the control first
        for job in w["jobs"]:
            w["jobs"][job] = {v: w["jobs"][job][v] for v in STRATEGIES if v in w["jobs"][job]}
    cpu = next((l.split(":", 1)[1].strip() for l in Path("/proc/cpuinfo").read_text().splitlines()
                if l.startswith("model name")), platform.processor())
    cfg = {"date": time.strftime("%Y-%m-%d %H:%M"), "cpu": cpu, "rounds": a.rounds, "reps": a.reps, "seed": a.seed,
           "quick": a.quick, "cflags": CFLAGS, "rows": {v: d for v, (_, d, _, _) in STRATEGIES.items()},
           "noise_floor_v0p_over_rerun": noise_floor(res)}
    res["config"] = cfg
    res["biases"] = BIASES
    (out / "round2.json").write_text(json.dumps(res, indent=1))
    (out / "round2.md").write_text(report_md(res, cfg))
    print(f"wrote {out / 'round2.md'} and {out / 'round2.json'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
