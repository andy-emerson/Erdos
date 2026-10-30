"""Ablation study of the light prime-graph prototype: what replaces grouping?

    python3 ablation_bench.py             # the measured run: writes results/ablation.md and results/ablation.json
    python3 ablation_bench.py --quick     # a smoke run on small sizes, written to build/quick/ (not results/)

The baseline is V0', the control: the prototype with grouping removed.  Every other strategy is V0' plus exactly one
idea (V1..V5), and V0 is the original prototype (pg.c, unchanged), whose one idea is grouping.  No strategy combines
two ideas.  Ratios are V0' / strategy: above 1, the strategy is faster than the control.

Workloads W1-W7 are pg_bench.py's, unchanged (same generators, sizes and seed handling); W8-W10 are new.  Each round
runs every strategy once per job, in an order rotated per round.  Each invocation runs --reps reps in one warm
process; every rep re-enters the values from the raw inputs and resets the node table, and the invocation reports
the minimum of each phase over its reps.  Tables give the medians over the rounds.  Round 1 checks every element
against Python's fractions and measures the share of outputs not in lowest terms; later rounds check a sample.
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

HERE = Path(__file__).resolve().parent
TEST = HERE.parent
sys.path.insert(0, str(TEST))
import pg_bench  # noqa: E402  (W1-W7, the input format, the fractions reference, the strict checks)

BUILD = HERE / "build"
CFLAGS = pg_bench.CFLAGS
DEPS = [HERE / f for f in ("abl_bench.c", "ab_core.c", "ab_l1.c", "ab_v3.c", "ab_v5.c")] + [TEST / "pg.c", TEST / "pg.h"]

BASE = "v0p"
# order of the tables: the control first, then the strategies.  Each entry: (C macro, description, environment).
# v3-lcm and v3-freq are the same V3 code with the two declared rules for choosing D (AB_V3_POLICY).
STRATEGIES = {
    "v0p": ("VAR_V0P", "V0' (control): grouping removed. Each value holds its numerator, cached denominator and inline "
                       "edge list; every op merges its operands' edges, scales and reduces; no group table, no "
                       "pair-plan cache", {}),
    "v0": ("VAR_V0", "V0: the prototype pg.c, unchanged (grouping: global group table, per-group pair-plan cache, "
                     "group-sum column sum)", {}),
    "v1": ("VAR_V1", "V1 = V0' + per-value denominator check: equal -> one integer op; one divides the other -> one "
                     "scaling; else the edge merge (mul: equal -> no merge)", {}),
    "v2": ("VAR_V2", "V2 = V0' + bucketing inside the column sum (hash by denominator, i128 checked accumulators; "
                     "each bucket reduced, then combined with the control's reducing graph adds); elementwise ops are V0'", {}),
    "v3-lcm": ("VAR_V3", "V3 = V0' + array denominator D with sparse exceptions, D from the lcm of the array's "
                         "denominators when below 2^63 (else the most frequent): dense checked int64 loops over D "
                         "(dense elements are scaled to D, not reduced), graph path for exceptions; D built at entry "
                         "(timed); outputs reduced in the timed final phase", {"AB_V3_POLICY": "lcm"}),
    "v3-freq": ("VAR_V3", "V3 as v3-lcm, D from the array's most frequent denominator (C = its edges; D = the lcm "
                          "of the denominators dividing it)", {"AB_V3_POLICY": "freq"}),
    "v4": ("VAR_V4", "V4 = V0' + deferred reduction: no reduction at entry or inside the expression; overflow -> "
                     "reduce operands and retry; one timed reduction of the results at the end", {}),
    "v5": ("VAR_V5", "V5 = V0' + interned denominator objects with real reference counts (pointer compare for "
                     "'same denominator?'; freed at count 0)", {}),
}
UNREDUCED_OK = set()  # strategies documented as giving unreduced results: none
SAMPLE = 100_000


def binary(v):
    return BUILD / f"abl_{v}"


def build(force=False):
    BUILD.mkdir(exist_ok=True)
    newest = max(d.stat().st_mtime for d in DEPS)
    for v, (macro, _, _) in STRATEGIES.items():
        b = binary(v)
        if force or not b.exists() or b.stat().st_mtime < newest:
            subprocess.run(["clang", *CFLAGS, f"-DVARIANT={macro}", "-o", str(b), str(HERE / "abl_bench.c"),
                            str(TEST / "pg.c")], check=True)


# ---------------------------------------------------------------------------------------------------- workloads

def dec2(rng, n, lo, hi):
    """Two-place decimals (money) in (lo, hi), entered as k/100."""
    return rng.integers(lo * 100 + 1, hi * 100, size=n, dtype=np.int64), np.full(n, 100, dtype=np.uint64)


def mixed90(rng, n):
    """90% three-place decimals in (-10^4, 10^4) over 1000; 10% numerators |n| < 10^6 over 1..12."""
    one = rng.random(n) < 0.9
    num = np.where(one, rng.integers(-10**7 + 1, 10**7, size=n, dtype=np.int64),
                   rng.integers(-10**6 + 1, 10**6, size=n, dtype=np.int64))
    den = np.where(one, 1000, rng.integers(1, 13, size=n, dtype=np.int64)).astype(np.uint64)
    return num, den


def workloads(quick):
    W = pg_bench.workloads(quick)  # W1-W7, unchanged
    s = (lambda n: max(n // 50, 1000)) if quick else (lambda n: n)
    W["W8"] = dict(desc="money: two-place decimals in (-10^5, 10^5)", n=s(1_000_000),
                   gen=lambda r, n: {k: dec2(r, n, -10**5, 10**5) for k in "abc"}, jobs=["sum", "fma"])
    W["W9"] = dict(desc="mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 "
                        "(numerators |n| < 10^6)", n=s(1_000_000),
                   gen=lambda r, n: {k: mixed90(r, n) for k in "abc"}, jobs=["sum", "fma"])
    W["W10"] = dict(desc="expression chain d = (a*b + c)*e + f on two-place decimals in (-100, 100)", n=s(1_000_000),
                    gen=lambda r, n: {k: dec2(r, n, -100, 100) for k in "abcef"}, jobs=["fma2"])
    return W


def expected(job, arrays, K=0):
    if job != "fma2":
        return pg_bench.expected(job, arrays, K)
    a, b, c, e, f = (pg_bench.fracs(arrays[k]) for k in "abcef")
    return [(x * y + z) * u + v for x, y, z, u, v in zip(a, b, c, e, f)]


# ---------------------------------------------------------------------------------------------------- checks

def check_rows(lines, exp, idx=None, ovf=None):
    """pg_bench.check: every checked line equals the expected fraction (den > 0); detected overflows ("0 0") are
    counted in ovf, and the row fails on any false overflow or on more than 0.1% overflowed."""
    return pg_bench.check(lines, exp, idx, ovf)


def unreduced_pct(lines):
    return pg_bench.unreduced_pct(lines)


def out_dens(lines):
    return len({l.split()[1] for l in lines if l != pg_bench.OVF_LINE})


def judge(v, lines, exp, idx, first):
    """(ok, msg, extras): the value check, plus (round 1) the unreduced rule: every output in lowest terms unless the
    strategy is documented otherwise."""
    ov = {}
    ok, msg = check_rows(lines, exp, idx, ov)
    extra = {}
    if first:
        extra = {"ovf_elems": ov, "unreduced": unreduced_pct(lines), "out_dens": out_dens(lines)}
        if ok and extra["unreduced"] > 0 and v not in UNREDUCED_OK:
            ok, msg = False, f"{extra['unreduced']:.4f}% of outputs not in lowest terms"
    return ok, msg, extra


def parse_stats(text):
    return {k: (int(v) if v.lstrip("-").isdigit() else v) for k, v in (t.split("=", 1) for t in text.split())}


def run_one(v, job, d, reps, K, out, env=None):
    """One invocation.  The strategy's own environment (v3-lcm / v3-freq: AB_V3_POLICY) applies unless env sets it."""
    e = dict(os.environ if env is None else env)
    for k, x in STRATEGIES[v][2].items():
        e.setdefault(k, x)
    args = [str(binary(v)), job, str(d), str(out), str(reps)] + ([str(K)] if job == "chain" else [])
    r = subprocess.run(args, capture_output=True, text=True, timeout=3600, env=e)
    if r.returncode == 3:
        return {"unable": r.stdout.strip().removeprefix("unable ")}
    if r.returncode != 0:
        raise RuntimeError(f"{v} {job} failed ({r.returncode}): {r.stderr.strip()}")
    return parse_stats(r.stdout)


# ---------------------------------------------------------------------------------------------------- report

def ms(ns):
    return f"{ns / 1e6:.2f}"


def ratio(x, y):
    return f"{x / y:.2f}" if y else "-"


def structure(v, last, extra):
    """The strategy's distinct denominators / groups, and what its one idea did."""
    if not last:
        return ""
    if v == "v0":
        return f"{last.get('groups', 0):,} groups"
    if v == "v5":
        return f"peak {last.get('objs_peak', 0):,} objects ({last.get('objs_freed', 0):,} freed)"
    if v == "v2":
        return f"{last.get('buckets', 0):,} buckets" if last.get("buckets") else "(no sum: = V0')"
    if v.startswith("v3"):
        return (f"input D after entry={last.get('D')} ({last.get('policy')}); {last.get('exc_in', 0):,} exceptions in, {last.get('exc_out', 0):,} out; "
                f"{last.get('dense_ovf', 0):,} dense overflows; {last.get('allslow', 0)} ops all-graph")
    if v == "v4":
        return f"{last.get('early', 0):,} early reductions"
    return ""


def noise_floor(res):
    """V2 runs the control's code everywhere except the column sum: its non-sum totals against V0' are the same-code spread."""
    r = []
    for w in res["workloads"].values():
        for job, jr in w["jobs"].items():
            b, x = jr.get(BASE, {}), jr.get("v2", {})
            if job != "sum" and "median" in b and "median" in x and b["ok"] and x["ok"]:
                r.append(b["median"]["total"] / x["median"]["total"])
    return r


def noise_floor_line(res):
    r = noise_floor(res)
    if not r:
        return "**Same-code noise floor:** not available (no non-sum job with both V0' and V2)."
    lo, hi = min(r), max(r)
    return (f"**Same-code noise floor:** V2 runs exactly the control's code outside the column sum, so its V0' / x (total) on "
            f"the {len(r)} non-sum jobs measures noise between identical code: {lo:.2f} to {hi:.2f} (median "
            f"{statistics.median(r):.2f}).  A ratio within that range of 1 counts as no difference.")


def report_md(res, cfg):
    L = ["# Prime graph: ablation of grouping", "",
         f"Generated {cfg['date']} by `ablation_bench.py` on {cfg['cpu']}; clang 18 `{' '.join(CFLAGS[:3])}`.  "
         f"{cfg['rounds']} interleaved rounds; each invocation runs {cfg['reps']} reps in one warm process (every rep "
         "re-enters the values from the raw inputs and resets the node table) and reports the minimum of each phase "
         "over its reps.  Tables give the median over rounds.  Times in ms.", "",
         "**Baseline: V0', the control (grouping removed).** Every other row is V0' plus exactly one idea; V0 is the "
         "original prototype, whose idea is grouping.  No row is meant to combine two ideas, with one exception that "
         "cannot be separated: V3's dense elements are scaled to the array denominator D and not reduced (a "
         "per-array canonical form), which is part of V4's idea too, so read V3 against V4 as well as V0'.  "
         "**V0' / x (total)** is the control's total over the row's total; **V0' / x (op)** is the control's "
         "op + final over the row's op + final.  Above 1, the strategy is faster than the control.", "",
         noise_floor_line(res), "",
         "Columns: **entry** is conversion from the raw (num, den) inputs, including decomposition (shown again as "
         "**decomp**) and the reduction of every entered value, except in V4 (values entered as given) and V3 "
         "(dense elements scaled to D unreduced; exceptions reduced), where it also includes building D; **op** is the operation; **final** is "
         "the timed final reduction to lowest terms (V3, V4; 0 elsewhere); **total** is the minimum over reps of "
         "entry + op + final within one rep.  **RSS** is the process's peak resident set (VmHWM; it includes the raw "
         "inputs and the output buffers, the same for every row of a job); **repr** is the bytes held by the strategy's own "
         "representation: for V0' and V1-V5 the measured peak of every allocation (arrays, edge pools, node table, "
         "objects, transient tables); for V0 an estimate after the run (node, group and denominator-map tables plus "
         "12 bytes per array element), not a measured peak.  **out dens** counts the distinct "
         "denominators among the outputs.  **unreduced %** is the share of outputs not in lowest terms (round 1, "
         "every output); a row fails if it is above 0, since no strategy is documented as unreduced.  Overflowed "
         "elements (\"0 0\") are counted, not checked: a row fails if over 0.1% overflowed or any overflow was "
         "false (the exact value fits 64 bits); ‡ marks rows with some.", ""]
    for v, (_, d, _) in STRATEGIES.items():
        L.append(f"- `{v}`: {d}")
    L.append("")
    for wl, w in res["workloads"].items():
        for job, jr in w["jobs"].items():
            L += [f"## {wl} {job}: {w['desc']}", "", f"n = {w['n']:,}" + (f", K = {w['K']}" if w.get("K") else ""), "",
                  "| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | "
                  "repr MiB | out dens | groups / objects / exceptions | unreduced % | check |",
                  "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|"]
            base = jr.get(BASE, {})
            base_ok = "median" in base and base["ok"]
            for v in STRATEGIES:
                cr = jr.get(v)
                if cr is None:
                    continue
                if "unable" in cr:
                    L.append(f"| {v} | unable: {cr['unable']} | | | | | - | - | | | | | | - |")
                    continue
                m, last = cr["median"], cr["last"]
                good = base_ok and cr["ok"]
                mark = " ‡" if (cr.get("ovf_elems") or {}).get("n") else ""
                rt = ratio(base["median"]["total"], m["total"]) + mark if good else "-"
                ro = ratio(base["median"]["op"] + base["median"]["red"], m["op"] + m["red"]) + mark if good else "-"
                L.append(f"| {v} | {ms(m['entry'])} | {ms(m['decomp'])} | {ms(m['op'])} | {ms(m['red'])} | "
                         f"{ms(m['total'])} | {rt} | {ro} | {last.get('maxrss_kb', 0) / 1024:.0f} | "
                         f"{last.get('rep_bytes', 0) / 2**20:.1f} | {cr.get('out_dens', '')} | "
                         f"{structure(v, last, cr)} | {cr['unreduced']:.2f} | "
                         f"{'ok' if cr['ok'] else 'FAIL: ' + cr['msg']} |")
            notes = []
            for v in STRATEGIES:
                cr = jr.get(v, {})
                if "last" in cr:
                    oe = cr.get("ovf_elems") or {}
                    if cr["last"].get("ovf") or oe.get("n"):
                        notes.append(f"{v}: overflows detected {cr['last'].get('ovf', 0)} (outputs left overflowed: "
                                     f"{oe.get('n', 0)}, of which {oe.get('fit', 0)} have an exact value that fits)")
            if BASE in jr and "unable" in jr[BASE]:
                notes.append("V0' is unable here, so no ratios.")
            elif BASE in jr and not jr[BASE].get("ok", False):
                notes.append(f"V0' FAILED its check here ({jr[BASE].get('msg')}), so no ratios.")
            L += [""] + [f"{n}.  " for n in notes] + [""]
    L += ["## Simplifications that may bias the results", ""] + [f"- {b}" for b in BIASES] + [""]
    return "\n".join(L)


BIASES = [
    "Entry is shared by V0' and V1-V5: a transient hash from each distinct input denominator to its edge list (new "
    "denominators gcd-refined against the live nodes exactly as pg_den_gid does, then decomposed with pg.c's "
    "pg_factor), freed when entry ends.  It plays the entry role of V0's denominator map, but nothing from it "
    "reaches the operations.  It favours inputs with few distinct denominators, as V0's map does.",
    "The control is strict about 'no caching': V0' and its strategies recompute the merge, the scalings and the "
    "reduction on every operation.  V0's pair-plan cache and 'weight lowered by one' cache are what grouping buys "
    "it on few-group data.",
    "V5's interning also removes duplicate edge lists: every value shares its denominator object's single edge list "
    "instead of carrying its own, which changes memory and cache behaviour as well as the equality test.",
    "V5's one idea includes the pointer comparison that the brief names as its 'same denominator?' check; this is "
    "the only same-denominator shortcut V5 has (no divisibility check), so it does not import V1.  Its product "
    "a*b inside a*b + c is a value and is interned like any other; its objects come from a free list (allocated in "
    "blocks of 1024 and kept for the process), so freeing is unlinking, not free().",
    "V3 cannot be separated from deferred reduction: its dense elements are numerators scaled to the array "
    "denominator D, never reduced until the final phase (a per-array canonical form).  Read V3 against V4 (deferred "
    "reduction alone) as well as against V0'.",
    "V3 has two declared rows for choosing D: v3-lcm (the lcm of the array's denominators when below 2^63, else the "
    "most frequent) and v3-freq (always the most frequent).  Scaling to a large lcm (W3: 27720; W9: 693000) makes "
    "the dense a*b overflow int64, so those elements fall back to the graph path.  V3 treats W5's single values as "
    "plain values (all exceptions), i.e. as V0'; and once an op's D would reach 2^63, or D is 1, every element takes "
    "the control's graph path, so V3 falls back to V0' with some overhead.",
    "V3 and V4 give lowest-terms outputs only after their final reduction, which is timed and included in total "
    "and in the op ratio (op + final).",
    "Same-code noise floor: V2 differs from V0' only in the column sum (bucketing; each bucket is reduced and the "
    "buckets are combined with the control's reducing adds, so V2 carries no deferred reduction).  Its other rows run the control's "
    "code, and their spread against V0' (stated above the tables) is the noise between identical code; differences "
    "within it count as no difference.",
    "The repr column is measured differently: for V0' and V1-V5 it is the real peak of every representation "
    "allocation; for V0 (pg.c, unchanged, so not instrumented) it is an estimate after the run from its table sizes "
    "and 12 bytes per array element.",
    "V4's overflow retry reduces the operands and repeats the op with the control's reducing form, so it never overflows "
    "where V0' would not; 'early reductions' counts the retries.",
    "Layouts differ by design: V0 arrays hold a numerator and a group id per element (12 bytes); V0', V1, V2, V4 hold "
    "numerator, denominator, edge count, edge offset and the edges (about 21 bytes + 4 per edge); V5 holds a "
    "numerator and an object pointer (16 bytes); V3 holds a dense numerator per element plus 96 bytes per "
    "exception.  W5's single values are pg_val structs in V0 and array elements elsewhere.",
    "The V0' family writes results in index order into a separate array, so a chain (W6) alternates two arrays where "
    "V0 works in place.",
    "V0's column sum reports 'unable' when a group sum or the combined common denominator overflows 64 bits; the "
    "control's sequential adds report it when a reduced partial sum does.  Both fail W4's sum.",
    "Warm processes: every rep re-enters the values from the raw inputs, but the reps run in one process, so "
    "allocator state and CPU caches are warm for every strategy alike, and the minimum over reps is reported.",
]


# ---------------------------------------------------------------------------------------------------- main

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rounds", type=int, default=5)
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--quick", action="store_true", help="small sizes; output to build/quick/ unless --out")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--only", help="comma-separated workloads, e.g. W1,W8")
    ap.add_argument("--strategies", help="comma-separated subset of " + ",".join(STRATEGIES))
    ap.add_argument("--seed", type=int, default=20261027)
    ap.add_argument("--force-build", action="store_true")
    a = ap.parse_args(argv)
    out = a.out or (BUILD / "quick" if a.quick else HERE / "results")
    out.mkdir(parents=True, exist_ok=True)
    build(a.force_build)
    W = workloads(a.quick)
    if a.only:
        W = {k: v for k, v in W.items() if k in a.only.split(",")}
    strategies = [v for v in STRATEGIES if not a.strategies or v in a.strategies.split(",")]
    data = BUILD / ("data-quick" if a.quick else "data")
    rng = np.random.default_rng(a.seed)
    prep = {}
    for wl, w in W.items():
        t = time.time()
        arrays = w["gen"](rng, w["n"])
        d = data / wl
        pg_bench.write_inputs(d, arrays)
        prep[wl] = (d, {job: expected(job, arrays, w.get("K", 0)) for job in w["jobs"]})
        print(f"{wl}: inputs and expected values in {time.time() - t:.1f} s", flush=True)
    res = {"baseline": BASE, "workloads": {wl: {"desc": w["desc"], "n": w["n"], "K": w.get("K"), "jobs": {}}
                                           for wl, w in W.items()}}
    raw = {}
    srng = random.Random(a.seed)
    failed = False
    for rnd in range(a.rounds):
        for wl, w in W.items():
            d, exps = prep[wl]
            for job in w["jobs"]:
                k = rnd % len(strategies)
                for v in strategies[k:] + strategies[:k]:
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
                        failed = True
                        print(f"  CHECK FAILED {wl} {job} {v}: {msg}", flush=True)
                    r["rounds"].append(st)
                    o.unlink()
        print(f"round {rnd + 1}/{a.rounds} done", flush=True)
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
           "quick": a.quick, "cflags": CFLAGS, "strategies": {v: d for v, (_, d, _) in STRATEGIES.items()},
           "noise_floor_v0p_over_v2_nonsum": noise_floor(res)}
    res["config"] = cfg
    res["biases"] = BIASES
    (out / "ablation.json").write_text(json.dumps(res, indent=1))
    (out / "ablation.md").write_text(report_md(res, cfg))
    print(f"wrote {out / 'ablation.md'} and {out / 'ablation.json'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
