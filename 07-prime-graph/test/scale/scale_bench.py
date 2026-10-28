"""Scalability stress test for the prime-graph exact-rational design (research/idea.md, "Scalability requirements").

    python3 scale_bench.py              # the measured run: writes results/scale.md, results/scale.json, results/*.svg
    python3 scale_bench.py --quick      # a smoke run on small sizes, written to build/quick/ (not results/)
    python3 scale_bench.py --only growth,churn   # a subset of the sweeps

Rows:
  S       every requirement at once on V3 arrays: two-sided edges, splits by copy and loop, the seen-prime dictionary
          at entry (trial division, the leftover looked up whole, a bounded hot list of known large primes,
          Miller-Rabin, rho), reference-counted nodes freed at zero, values deletable (scale.c, g_s.c, s_v3.c)
  S-lazy  S with no Miller-Rabin or rho at entry: large leftovers become pending atoms, refined in the idle pass
          by a remainder tree against the settled large atoms and a batch gcd among themselves (GMP)
  C       the control (reference curve only): round two's V0' entry (gcd scan of every node, factoring of every
          non-prime node, edges by a scan) on the same V3 arrays, one-sided edges, no reference counts; a split is
          followed by a rewrite scan of every holder

Sweeps (each step is one row of a table; every step's sampled results are checked against fractions):
  1 growth    batches of 10^5 values up to 10^7, live nodes rising to about 10^5; per batch: entry, idle pass, and
              10^5 ops twice: a FIXED sample (the same operand pairs, from the first batch, every step) and a sample
              over ALL live values.  Pass: the fixed sample's time per op stays within 2x of the first batch.
  2 heavy     one node (30030) shared by 10^6..10^7 numerators, then 1/7 and 1/6 force two splits: time vs degree
  3 churn     10^6 live values, 10^7 events (delete a random value / insert); nodes and memory over time; at the
              end everything is deleted and the allocations are counted (the leak check)
  4 distinct  10^3, 10^4, 10^5 distinct denominators (10 values each): entry per distinct denominator, ops
  5 chain     a node over the first 15 primes shared by 10^4..10^6 numerators, split 14 times in a row

Checks: each sampled op result (o) must equal Fraction(a) op Fraction(b), in lowest terms; a reported overflow is
"true" if the exact result does not fit (numerator in int64, denominator below 2^63) and "false" otherwise (counted,
reported; the shared V0' op code detects overflow on the unreduced common denominator).  Each sampled stored value (v)
must equal the value entered, in lowest terms.  The binaries' audits (outside the timers): live nodes pairwise coprime
(batch gcd), every node equals its atoms and each atom maps to it in the dictionary, edge symmetry both ways with
list positions, reference count == list length and no live node unused, each holder's denominator == the product of
its edges, the array layout and handles.  Churn's final step must show no leak.
"""
import argparse
import json
import math
import os
import platform
import subprocess
import sys
import time
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build"
CFLAGS = ["-std=c2x", "-O3", "-march=native", "-Wall", "-Wextra", "-Wno-unused-function",
          "-Wno-invalid-feature-combination"]
SRC = ["scale.c", "s_core.c", "g_s.c", "g_c.c", "s_v3.c"]
LIM = 2 ** 63

ROWS = {"S": ("s", {}), "S-lazy": ("s", {"lazy": 1}), "C": ("c", {})}


def binary(v):
    return BUILD / f"scale_{v}"


def build(force=False):
    BUILD.mkdir(exist_ok=True)
    newest = max((HERE / f).stat().st_mtime for f in SRC)
    for v, macro in (("s", "VAR_S"), ("c", "VAR_C")):
        b = binary(v)
        if force or not b.exists() or b.stat().st_mtime < newest:
            subprocess.run(["clang", *CFLAGS, f"-D{macro}", "-o", str(b), str(HERE / "scale.c"), "-lgmp"], check=True)


# ---------------------------------------------------------------- checks

def check_line(line):
    """One chk line -> None if right, else a reason.  'ovf_true' / 'ovf_false' classify detected overflows."""
    p = line.split()
    if p[0] == "o":
        op = p[1]
        an, ad, bn, bd, rn, rd = map(int, p[2:8])
        a, b = Fraction(an, ad), Fraction(bn, bd)
        x = a + b if op == "+" else a * b
        if rd == 0:
            fits = -LIM <= x.numerator < LIM and x.denominator < LIM
            return "ovf_false" if fits else "ovf_true"
        if rd < 0:
            return "negative_den"
        if math.gcd(abs(rn), rd) != 1 or (rn == 0 and rd != 1):
            return "not_lowest_terms"
        if Fraction(rn, rd) != x:
            return "wrong_result"
        return None
    if p[0] == "v":
        on, od, sn, sd = map(int, p[1:5])
        if sd <= 0:
            return "stored_bad"
        if math.gcd(abs(sn), sd) != 1 or (sn == 0 and sd != 1):
            return "stored_not_lowest_terms"
        if Fraction(sn, sd) != Fraction(on, od):
            return "stored_wrong"
        return None
    return "unknown_line"


def check_file(path):
    """counts: checked, failures (by reason, wrong values only), ovf_true, ovf_false, first failures"""
    c = {"ops": 0, "values": 0, "fail": 0, "ovf_true": 0, "ovf_false": 0, "reasons": {}, "examples": []}
    if not Path(path).exists():
        return c
    with open(path) as f:
        for line in f:
            if not line.strip():
                continue
            c["ops" if line[0] == "o" else "values"] += 1
            r = check_line(line)
            if r in ("ovf_true", "ovf_false"):
                c[r] += 1
            elif r:
                c["fail"] += 1
                c["reasons"][r] = c["reasons"].get(r, 0) + 1
                if len(c["examples"]) < 5:
                    c["examples"].append(line.strip())
    return c


# ---------------------------------------------------------------- running

def run(row, wl, params, outdir, tag, budget=None, timeout=4 * 3600):
    v, extra = ROWS[row]
    chk = outdir / f"{tag}.chk"
    args = [str(binary(v)), wl, f"chk={chk}"] + [f"{k}={x}" for k, x in {**extra, **params}.items()]
    if budget:
        args.append(f"budget_s={budget}")
    t0 = time.time()
    r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
    steps = []
    for line in r.stdout.splitlines():
        line = line.strip()
        if line.startswith("{"):
            steps.append(json.loads(line))
    res = {"row": row, "wl": wl, "params": params, "steps": steps, "rc": r.returncode, "stderr": r.stderr[-2000:],
           "wall_s": time.time() - t0, "check": check_file(chk)}
    try:
        chk.unlink()  # chk files are large; the counts are kept
    except FileNotFoundError:
        pass
    print(f"  {row:7s} {wl:8s} {params} -> {len(steps)} steps, rc {r.returncode}, {res['wall_s']:.1f}s, "
          f"check fail {res['check']['fail']}", flush=True)
    return res


def plan(quick):
    if quick:
        return {
            "growth": {"N": 200000, "B": 20000, "M": 10000, "T": 5000, "audit_every": 2, "nchk": 1000, "nval": 500},
            "heavy": [{"deg": 20000, "B": 10000}, {"deg": 50000, "B": 10000}],
            "chain": [{"deg": 2000}, {"deg": 20000}],
            "churn": {"L": 20000, "E": 200000, "b": 500, "step": 20000, "W": 2000, "rate": 5, "Bi": 5000,
                      "nchk": 500, "nval": 500},
            "distinct": [{"ND": 200, "per": 5, "M": 5000}, {"ND": 2000, "per": 5, "M": 5000}],
            "budget": 60,
        }
    return {
        "growth": {"N": 10_000_000, "B": 100_000, "M": 100_000, "T": 100_000, "audit_every": 10, "nchk": 2000,
                   "nval": 1000},
        "heavy": [{"deg": d} for d in (1_000_000, 2_000_000, 5_000_000, 10_000_000)],
        "chain": [{"deg": d} for d in (10_000, 100_000, 1_000_000)],
        "churn": {"L": 1_000_000, "E": 10_000_000, "b": 1000, "step": 1_000_000, "W": 20000, "rate": 20,
                  "Bi": 10000, "nchk": 1000, "nval": 1000},
        "distinct": [{"ND": n, "per": 10} for n in (1000, 10_000, 100_000)],
        "budget": 600,
    }


# ---------------------------------------------------------------- verdicts

def ok_checks(res):
    return res["rc"] == 0 and res["check"]["fail"] == 0 and all(
        s.get("audit") in (None, "ok") for s in res["steps"])


def audits(res):
    a = [s.get("audit") for s in res["steps"] if s.get("audit") is not None]
    bad = [x for x in a if x != "ok"]
    return f"{len(a) - len(bad)}/{len(a)} ok" + (f" ({bad[0]})" if bad else "")


def growth_verdict(res):
    st = [s for s in res["steps"] if "fixed_ns" in s]
    if not st:
        return None
    f0, a0 = st[0]["fixed_ns"], st[0]["op_ns"]
    fmax = max(s["fixed_ns"] for s in st)
    amax = max(s["op_ns"] for s in st)
    tail = st[len(st) // 2:]
    fmed_tail = sorted(s["fixed_ns"] for s in tail)[len(tail) // 2]
    return {"fixed_first": f0, "fixed_max": fmax, "fixed_ratio": fmax / f0, "fixed_tail_median_ratio": fmed_tail / f0,
            "all_first": a0, "all_max": amax, "all_ratio": amax / a0, "pass": fmax / f0 <= 2.0,
            "steps": len(st), "stopped": any("stopped" in s for s in res["steps"])}


def churn_verdict(res):
    st = [s for s in res["steps"] if "ns_per_event" in s]
    fin = [s for s in res["steps"] if s.get("final")]
    if not st:
        return None
    n0, m0 = st[0]["nodes_live"], st[0]["mem"] / max(1, st[0]["values"])
    nmax = max(s["nodes_live"] for s in st)
    mmax = max(s["mem"] / max(1, s["values"]) for s in st)
    f = fin[0] if fin else {}
    leak = {k: f.get(k) for k in ("blocks_over_baseline", "nodes_live", "holders", "pool_used", "handles_live", "arrays")}
    no_leak = bool(fin) and all(v == 0 for v in leak.values())
    return {"nodes_first": n0, "nodes_max": nmax, "nodes_ratio": nmax / max(1, n0), "mem_per_value_first": m0,
            "mem_per_value_max": mmax, "mem_ratio": mmax / m0, "leak": leak, "no_leak": no_leak,
            "pass": nmax / max(1, n0) <= 2.0 and mmax / m0 <= 2.0 and no_leak,
            "stopped": any("stopped" in s for s in res["steps"])}


# ---------------------------------------------------------------- plots

def plots(res, outdir):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return {}
    files = {}
    colors = {"S": "#1f6fb4", "S-lazy": "#2a9d55", "C": "#c0392b"}

    def fig(name, title, xlabel, ylabel, series, logx=False, logy=False):
        f, ax = plt.subplots(figsize=(6.4, 3.8))
        for label, xs, ys, style in series:
            if xs:
                ax.plot(xs, ys, style, label=label, color=colors.get(label.split()[0], None), markersize=3)
        ax.set_title(title, fontsize=10)
        ax.set_xlabel(xlabel); ax.set_ylabel(ylabel)
        if logx: ax.set_xscale("log")
        if logy: ax.set_yscale("log")
        ax.grid(alpha=0.3)
        ax.legend(fontsize=8)
        f.tight_layout()
        p = outdir / f"{name}.svg"
        f.savefig(p)
        plt.close(f)
        files[name] = p.name

    g = {r["row"]: [s for s in r["steps"] if "fixed_ns" in s] for r in res.get("growth", [])}
    if g:
        fig("growth_ops", "Growth: time per op (fixed sample solid, all-values sample dashed)", "values", "ns per op",
            [(f"{k} fixed", [s["values"] for s in v], [s["fixed_ns"] for s in v], "-o") for k, v in g.items()] +
            [(f"{k} all", [s["values"] for s in v], [s["op_ns"] for s in v], "--") for k, v in g.items()])
        fig("growth_entry", "Growth: entry time per value against live nodes", "live nodes", "ns per value",
            [(k, [s["nodes_live"] for s in v], [s["entry_ns_per_value"] for s in v], "-o") for k, v in g.items()],
            logy=True)
    h = {}
    for r in res.get("heavy", []):
        for s in r["steps"]:
            if s.get("split", 0) >= 1:
                h.setdefault(f"{r['row']} split {s['split']}", []).append((s["deg"], s["split_ns"] or s["entry_ns"]))
    if h:
        fig("heavy_split", "Heavy split: time against the node's degree", "numerators on the node", "ns",
            [(k, [x for x, _ in v], [y for _, y in v], "-o") for k, v in h.items()], logx=True, logy=True)
    c = {r["row"]: [s for s in r["steps"] if "ns_per_event" in s] for r in res.get("churn", [])}
    if c:
        fig("churn_nodes", "Churn: live nodes over time", "events", "live nodes",
            [(k, [s["events"] for s in v], [s["nodes_live"] for s in v], "-o") for k, v in c.items()])
        fig("churn_mem", "Churn: representation memory over time", "events", "MB",
            [(k, [s["events"] for s in v], [s["mem"] / 1e6 for s in v], "-o") for k, v in c.items()])
    d = {}
    for r in res.get("distinct", []):
        for s in r["steps"]:
            if "entry_ns_per_distinct" in s:
                d.setdefault(r["row"], []).append((s["ND"], s["entry_ns_per_distinct"], s["op_ns"]))
    if d:
        fig("distinct_entry", "Many distinct denominators: entry per distinct denominator", "distinct denominators",
            "ns", [(k, [a for a, _, _ in v], [b for _, b, _ in v], "-o") for k, v in d.items()], logx=True, logy=True)
    ch = {}
    for r in res.get("chain", []):
        for s in r["steps"]:
            if s.get("split", 0) >= 1 and s.get("numerators"):
                ch.setdefault(f"{r['row']} deg {s['deg']}", []).append((s["split"], s["ns_per_numerator"]))
    if ch:
        fig("chain_split", "Deep chain: ns per numerator for each split in the chain", "split number",
            "ns per numerator", [(k, [a for a, _ in v], [b for _, b in v], "-o") for k, v in ch.items()])
    return files


def ascii_plot(xs, ys, width=50):
    if not ys:
        return ""
    m = max(ys) or 1
    return "\n".join(f"{x:>12} | {'#' * max(1, round(width * y / m))} {y:.1f}" for x, y in zip(xs, ys))


# ---------------------------------------------------------------- report

def fmt(x, d=1):
    if x is None:
        return "—"
    if isinstance(x, float):
        return f"{x:,.{d}f}"
    return f"{x:,}"


def chk_cell(r):
    c = r["check"]
    s = f"{c['ops']:,} ops, {c['values']:,} values: " + ("all right" if c["fail"] == 0 else f"**{c['fail']} wrong** {c['reasons']}")
    if c["ovf_true"] or c["ovf_false"]:
        s += f"; overflows {c['ovf_true']} true, {c['ovf_false']} false"
    return s


def report_md(res, cfg, files):
    L = [f"# Scalability stress test: S, S-lazy and C\n",
         f"Generated {cfg['date']} by `scale_bench.py`{' (--quick)' if cfg['quick'] else ''} on {cfg['cpu']}; "
         f"clang `{' '.join(CFLAGS[:3])}`, GMP for the product trees.  Spec: research/idea.md, "
         f"\"Scalability requirements\".  Rows: S (every requirement), S-lazy (S without Miller-Rabin or rho at "
         f"entry; leftovers refined in the idle pass), C (round two's V0' entry, reference only).  Every step's "
         f"sampled results were checked against `fractions`; audits run outside the timers.\n"]
    L.append("## Verdicts\n")
    L.append("| Sweep | Row | Criterion | Result | Checks |")
    L.append("| --- | --- | --- | --- | --- |")
    for r in res.get("growth", []):
        v = growth_verdict(r)
        if v:
            L.append(f"| growth | {r['row']} | fixed-sample op time within 2x of batch 1 | "
                     f"{'**pass**' if v['pass'] else '**fail**'}: max {v['fixed_ratio']:.2f}x "
                     f"({fmt(v['fixed_first'])} -> max {fmt(v['fixed_max'])} ns); all-values sample "
                     f"{v['all_ratio']:.2f}x{' (stopped by budget)' if v['stopped'] else ''} | "
                     f"{'ok' if ok_checks(r) else 'FAIL'}, audits {audits(r)} |")
    for r in res.get("churn", []):
        v = churn_verdict(r)
        if v:
            L.append(f"| churn | {r['row']} | nodes and memory per value within 2x of step 1; no leak | "
                     f"{'**pass**' if v['pass'] else '**fail**'}: nodes {v['nodes_ratio']:.2f}x, memory per value "
                     f"{v['mem_ratio']:.2f}x, leak check {'clean' if v['no_leak'] else v['leak']}"
                     f"{' (stopped by budget)' if v['stopped'] else ''} | "
                     f"{'ok' if ok_checks(r) else 'FAIL'}, audits {audits(r)} |")
    for wl in ("heavy", "chain", "distinct"):
        for r in res.get(wl, []):
            L.append(f"| {wl} | {r['row']} | {r['params']} | (see table) | {'ok' if ok_checks(r) else 'FAIL'}, "
                     f"audits {audits(r)} |")
    L.append("")

    for r in res.get("growth", []):
        L.append(f"## 1. Growth: {r['row']}\n")
        L.append(f"Parameters {r['params']}; wall {r['wall_s']:.0f} s; checks: {chk_cell(r)}.\n")
        L.append("| Step | Values | Live nodes | Entry ns/value | Idle ms | Fixed ns/op (add, mul) | All-values ns/op | "
                 "Ovf (fixed/all) | Splits (entry/op/idle) | Memory MB | Audit |")
        L.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for s in r["steps"]:
            if "fixed_ns" not in s:
                L.append(f"| stopped: {s.get('stopped')} at step {s.get('step')} | | | | | | | | | | |")
                continue
            L.append(f"| {s['step']} | {fmt(s['values'])} | {fmt(s['nodes_live'])} | {fmt(s['entry_ns_per_value'])} | "
                     f"{s['idle_ns'] / 1e6:.1f} | {fmt(s['fixed_ns'])} ({fmt(s['fixed_add_ns'])}, {fmt(s['fixed_mul_ns'])}) | "
                     f"{fmt(s['op_ns'])} | {s['fixed_ovf']}/{s['op_ovf']} | "
                     f"{s.get('splits_entry', 0)}/{s.get('splits_op', '—')}/{s.get('splits_idle', '—')} | "
                     f"{s['mem'] / 1e6:.0f} | {s['audit'] or ''} |")
        L.append("")
    if "growth_ops" in files:
        L.append(f"![growth ops]({files['growth_ops']})\n\n![growth entry]({files['growth_entry']})\n")
    elif res.get("growth"):  # no matplotlib: ASCII
        for r in res["growth"]:
            st = [s for s in r["steps"] if "fixed_ns" in s]
            L.append(f"```\n{r['row']}: fixed-sample ns per op by values\n"
                     + ascii_plot([s["values"] for s in st], [s["fixed_ns"] for s in st]) + "\n```\n")

    if res.get("heavy"):
        L.append("## 2. Heavy split\n")
        L.append("Node 30030 shared by `deg` single values (each its own numerator), then 1/7 (split 1: 7 out of 30030) "
                 "and 1/6 (split 2: 2·3 out of 4290).  C factors at entry, so it never holds 30030; its entry time for "
                 "the same values is shown for reference.\n")
        L.append("| Row | Degree | Build ns/value | Split | Entry ms | Split ms | Numerators looped | ns per numerator | "
                 "Memory MB | Audit | Checks |")
        L.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for r in res["heavy"]:
            b = next((s for s in r["steps"] if s.get("split") == 0), {})
            for s in r["steps"]:
                if s.get("split", 0) < 1:
                    continue
                L.append(f"| {r['row']} | {fmt(s['deg'])} | {fmt(b.get('build_ns_per_value'))} | {s['split']} (1/{s['den']}) | "
                         f"{s['entry_ns'] / 1e6:.2f} | {s['split_ns'] / 1e6:.2f} | {fmt(s['numerators'])} | "
                         f"{fmt(s['ns_per_numerator'], 2)} | {s['mem'] / 1e6:.0f} | {s.get('audit') or b.get('audit') or ''} | "
                         f"{'ok' if ok_checks(r) else 'FAIL'} |")
        L.append("")
        if "heavy_split" in files:
            L.append(f"![heavy split]({files['heavy_split']})\n")

    for r in res.get("churn", []):
        v = churn_verdict(r)
        L.append(f"## 3. Churn: {r['row']}\n")
        L.append(f"Parameters {r['params']}; wall {r['wall_s']:.0f} s; checks: {chk_cell(r)}.\n")
        L.append("| Step | Events | Live values | Live nodes | Node slots | ns/event | Memory MB | Bytes/value | "
                 "Blocks over baseline | Nodes freed | Audit |")
        L.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for s in r["steps"]:
            if "ns_per_event" not in s:
                continue
            L.append(f"| {s['step']} | {fmt(s['events'])} | {fmt(s['values'])} | {fmt(s['nodes_live'])} | "
                     f"{fmt(s['node_slots'])} | {fmt(s['ns_per_event'])} | {s['mem'] / 1e6:.0f} | "
                     f"{s['mem'] / max(1, s['values']):.0f} | {fmt(s['blocks_over_baseline'])} | "
                     f"{fmt(s.get('nodes_freed'))} | {s['audit']} |")
        if v:
            L.append(f"\nAfter deleting everything: {v['leak']} ({'no leak' if v['no_leak'] else 'LEAK or retained nodes'}).\n")
        if "churn_nodes" not in files:  # no matplotlib: ASCII
            L.append("```\nlive nodes by events\n" + ascii_plot([s["events"] for s in r["steps"] if "ns_per_event" in s],
                     [s["nodes_live"] for s in r["steps"] if "ns_per_event" in s]) + "\n```\n")
    if "churn_nodes" in files:
        L.append(f"![churn nodes]({files['churn_nodes']})\n\n![churn memory]({files['churn_mem']})\n")

    if res.get("distinct"):
        L.append("## 4. Many distinct denominators\n")
        L.append("| Row | Distinct | Values | Entry ns/value | Entry ns/distinct | Idle ms | ns/op (add, mul) | "
                 "Live nodes | Memory MB | Audit | Checks |")
        L.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
        for r in res["distinct"]:
            for s in r["steps"]:
                if "entry_ns_per_distinct" not in s:
                    L.append(f"| {r['row']} | {fmt(r['params']['ND'])} | stopped: {s.get('stopped')} | | | | | | | | |")
                    continue
                L.append(f"| {r['row']} | {fmt(s['ND'])} | {fmt(s['values'])} | {fmt(s['entry_ns_per_value'])} | "
                         f"{fmt(s['entry_ns_per_distinct'])} | {s['idle_ns'] / 1e6:.1f} | {fmt(s['op_ns'])} "
                         f"({fmt(s['op_add_ns'])}, {fmt(s['op_mul_ns'])}) | {fmt(s['nodes_live'])} | "
                         f"{s['mem'] / 1e6:.0f} | {s['audit']} | {'ok' if ok_checks(r) else 'FAIL'} |")
        L.append("")
        if "distinct_entry" in files:
            L.append(f"![distinct]({files['distinct_entry']})\n")

    if res.get("chain"):
        L.append("## 5. Deep refinement chain\n")
        L.append("A node over the first 15 primes (614889782588491410) shared by `deg` numerators; 1/2, 1/3, ..., 1/43 "
                 "then split one prime out at a time (14 splits of the same composite).\n")
        L.append("| Row | Degree | Split | Den | Split ms | Numerators | ns per numerator | Audit |")
        L.append("| --- | --- | --- | --- | --- | --- | --- | --- |")
        for r in res["chain"]:
            for s in r["steps"]:
                if s.get("split", 0) < 1:
                    continue
                L.append(f"| {r['row']} | {fmt(s['deg'])} | {s['split']} | {s['den']} | {s['split_ns'] / 1e6:.2f} | "
                         f"{fmt(s['numerators'])} | {fmt(s['ns_per_numerator'], 2)} | {s.get('audit') or ''} |")
        L.append("")
        if "chain_split" in files:
            L.append(f"![chain]({files['chain_split']})\n")

    L.append("## Notes on bias\n")
    L.append("- The all-values op sample touches memory spread over every value, so its time rises with the working "
             "set (cache and TLB misses), for C as much as for S; the fixed sample keeps the operands and their "
             "memory the same at every step, so it isolates the graph's size.  The pass rule uses the fixed sample.")
    L.append("- Op results are stored in a ring of holders (linked, so S pays for its two-sided edges on every "
             "result); operands are values entered by the workload, not earlier results.")
    L.append("- S's entry cost is dominated by rho on two-large-prime denominators (C finds those factors by its "
             "gcd scan instead); S-lazy moves that work to the idle pass, whose cost per batch grows with the number "
             "of settled large atoms (the remainder tree and the descents are linear in them).")
    L.append("- Heavy split and chain use single values (V3's all-exception arrays), so each value is its own "
             "numerator; a V3 array of 10^7 equal denominators would be one numerator on the node.")
    L.append("- Memory counts the representation's allocations (tables, pools, lists, arrays, handles), not the "
             "verification copies of the entered values or the GMP temporaries of audits and the idle pass.")
    return "\n".join(L) + "\n"


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--only", default="growth,heavy,churn,distinct,chain")
    ap.add_argument("--rows", default="S,S-lazy,C")
    ap.add_argument("--seed", type=int, default=1)
    a = ap.parse_args(argv)
    build()
    P = plan(a.quick)
    outdir = BUILD / "quick" if a.quick else HERE / "results"
    outdir.mkdir(parents=True, exist_ok=True)
    rows = a.rows.split(",")
    only = a.only.split(",")
    res = {}
    seed = {"seed": a.seed}
    budget = P["budget"]
    if "growth" in only:
        print("growth", flush=True)
        res["growth"] = [run(r, "growth", {**P["growth"], **seed}, outdir, f"growth-{r}",
                             budget=budget if r == "C" else None) for r in rows]
    if "heavy" in only:
        print("heavy", flush=True)
        res["heavy"] = [run(r, "heavy", {**p, **seed}, outdir, f"heavy-{r}-{p['deg']}")
                        for r in rows if r != "S-lazy" for p in P["heavy"]]
    if "chain" in only:
        print("chain", flush=True)
        res["chain"] = [run(r, "chain", {**p, **seed}, outdir, f"chain-{r}-{p['deg']}")
                        for r in rows if r != "S-lazy" for p in P["chain"]]
    if "churn" in only:
        print("churn", flush=True)
        res["churn"] = [run(r, "churn", {**P["churn"], **seed}, outdir, f"churn-{r}",
                            budget=budget if r == "C" else None) for r in rows]
    if "distinct" in only:
        print("distinct", flush=True)
        res["distinct"] = [run(r, "distinct", {**p, **seed}, outdir, f"distinct-{r}-{p['ND']}",
                               budget=budget if r == "C" else None) for r in rows for p in P["distinct"]]
    cpu = platform.processor() or platform.machine()
    try:
        for line in open("/proc/cpuinfo"):
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    cfg = {"date": time.strftime("%Y-%m-%d %H:%M"), "cpu": cpu, "quick": a.quick, "cflags": CFLAGS, "plan": P,
           "rows": rows}
    files = plots(res, outdir)
    md = report_md(res, cfg, files)
    (outdir / "scale.md").write_text(md)
    verdicts = {"growth": {r["row"]: growth_verdict(r) for r in res.get("growth", [])},
                "churn": {r["row"]: churn_verdict(r) for r in res.get("churn", [])}}
    (outdir / "scale.json").write_text(json.dumps({"config": cfg, "verdicts": verdicts, "results": res}, indent=1))
    print(f"wrote {outdir / 'scale.md'}")
    bad = [f"{wl}/{r['row']}" for wl, rs in res.items() for r in rs if not ok_checks(r)]
    if bad:
        print("CHECK FAILURES:", bad)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
