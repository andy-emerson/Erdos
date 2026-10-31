"""Analysis of one run of round m3 (research/rule-m3.md items 7-13, Run plan step 7): results.md and CSVs from
OUTDIR/state.json. From m2_analyze.py (unchanged, m2's), whose structure it keeps and whose pooled bootstrap of measure
1 (boot_pooled) it imports, and m1_analyze.py, whose statistics and formatting this imports and uses unchanged
(rule-m3, Harness: "Its ... bootstrap and stops are unchanged"): boot_ratio, time_verdict, fixed_verdict, fmt, median,
write_csv, runner_per_launch, runner_summary.

    python3 m3_analyze.py OUTDIR

Per job, pairing and measure only, for the six pairings of rule-m3 item 7 (three against TC; Pack-primes and
Pack-coprime against Hash-V-pack; Pack-coprime against Pack-primes, reported with no verdict on any measure), the run's
headline measures (spec v0.3 §16.10, v0.2 §15.6): measure 1, the build time of one input matrix (pooled); the operation
measure (2 addition, 3 multiplication, 4 sum of products, 5 product of sums); measure 6, the stable bytes of one stored
input ((f) and (q) included); the operation's peak (7 in add, 8 in mul; sop's and pos's reported, no verdict). Each time
measure has its own band h = max(5%, its null spread from Hash-V-pack against Hash-V-pack-null). A row over cap on a
job (its calibration rep past 10 s: no rounds) has the time verdict "over cap", with the calibration rep's time as the
magnitude (rule-m3 item 1); its memory verdicts stand. Gcd work: a check for Hash-V-pack (G(row)/G(TC) exactly 1);
reported, not checked, for the factored rows, per part, beside their factor work (item 7). Also reported, no verdict:
total and read-out time, per-matrix build times, the whole job's bytes after creation and at the end, the peak during
each input's build, and the counts of item 13 (per matrix: W, E, (f)'s bytes, |q|, trial-division candidates,
refinement gcds, splits, pass 2's tests; per step: reconciliation work, merges, tests, per-piece gcds, leftover splits
and the new sub-cases). The strata (sop: no MID) and the size x sharing grid, for the headline measures only. A run
that stopped is reported with what it has and no verdict. Never writes into ../10-interning/, m1's or m2's results
folders, or another m3 run's folder.
"""
import json
import sys
from pathlib import Path

import numpy as np

import m3_checks as K  # noqa: F401  (as m2_analyze imports m2_checks)
import m3_common as C
from m1_analyze import (boot_ratio, fixed_verdict, fmt, median, runner_per_launch,  # noqa: F401  (m1's, unchanged)
                        runner_summary, time_verdict, write_csv)
from m2_analyze import boot_pooled  # noqa: E402,F401  (m2's, unchanged)

NA = "n/a"
CAVEAT = ("the other rows' own noise is not measured: the null row is a copy of Hash-V-pack, and on this job a row of "
          "the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ "
          "(rule-m3 item 8, carried over)")
LIMITATIONS = [
    "**No n = 128 in any row or set, and no rho = 16 in sop.** The step from 64 to 128, where m2 saw the handle width "
    "change, is not measured.",
    "**Pack-primes is deliberately unoptimized.** Its times measure schoolbook trial division, by the author's design, "
    "not factoring in general. A fast factorizer is a later, separate change. The aim is a schoolbook baseline: find the "
    "weak points first, then optimize systematically.",
    "**Pack-coprime refines eagerly, per matrix**, the form 09 found costly: 58-161 times the control's gcd work there "
    "(09 findings, lesson 1). Its reconciliation, by contrast, is on demand and limited to denominator factors (v0.3 "
    "§16.7).",
    "**The factored rows keep TC's value arithmetic.** They measure only what factored gcds save and what factoring "
    "costs, not value shortcuts the factors would allow (v0.3 §16.5).",
    "**On this workload every denominator is {2, 3, 5}-smooth**, so the denominators' factorizations are short, and "
    "reconciliation is small. Workloads with richer denominators would change both.",
    "**The factoring time is not separated from the operation's time.** The counters separate the work.",
    "**Results are reported per run**, and not combined across runs or with m1 and m2.",
    "Carried over from rule-m2, without Hash-V-fixed and Hash-V-grow-map: textbook stores (open addressing with linear "
    "probing); Hash-V-pack's build table is a fixed table; the expression sets reuse one value distribution; 64 bits "
    "only; square matrices; the disturbance check cannot see every process.",
]
MEASURES_TIME = ("build", "operation")                    # each with its own verdict and band
REPORTED_TIMES = ("total", "read-out")
FIELD = {"operation": "operation", "total": "total", "read-out": "read"}


# ------------------------------------------------------------------------------------------------- data
class Data:
    def __init__(self, st):
        self.st = st
        self.op = st.get("op")
        self.rounds = {}                            # (job, row) -> {round: record}
        self.capped = {}
        for rec in st.get("records", []):
            key = (rec["job"], rec["row"])
            if rec.get("role") != "round" or "bug" in rec:
                continue
            if rec.get("overcap"):
                self.capped[key] = rec
            elif "total" in rec:
                self.rounds.setdefault(key, {})[rec["round"]] = rec
        self.cal = {}
        for k, c in st.get("calibration", {}).items():
            job, row = k.split(":")
            self.cal[(int(job), row)] = c
            if c.get("overcap"):
                self.capped[(int(job), row)] = c
        self.counts = {tuple(reversed(k.split(":"))): v for k, v in st.get("counts", {}).items()}
        self.mems = {tuple(reversed(k.split(":"))): v for k, v in st.get("mems", {}).items()}
        self.gen = st.get("gen", {})

    def count(self, job, impl):
        return self.counts.get((str(job), impl))

    def mem(self, job, impl):
        return (self.mems.get((str(job), impl)) or {}).get("memrun")

    def paired(self, job, x, y, measure):
        """the rounds both rows have: per round the measure's value (a list of samples for measure 1)"""
        a, b = self.rounds.get((job, x), {}), self.rounds.get((job, y), {})
        common = sorted(set(a) & set(b))
        if measure == "build":
            return [a[k]["builds"] for k in common], [b[k]["builds"] for k in common]
        return [a[k][FIELD[measure]] for k in common], [b[k][FIELD[measure]] for k in common]

    def one_rep(self, job, row):
        return (self.cal.get((job, row)) or {}).get("reps") == 1


def job_label(st, job):
    g = st.get("gen", {}).get(str(job), {})
    n, rho = g.get("n", C.job_n(job, bool(st.get("toy")))), g.get("rho", C.job_rho(job))
    return f"{job} (n {n}, rho {rho})"


def ratio_of(measure, a, b):
    """T(a)/T(b) and its interval, by the measure's own bootstrap"""
    return boot_pooled(a, b) if measure == "build" else boot_ratio(a, b)


# ------------------------------------------------------------------------------------------------- measures
def null_h(d, job, measure):
    """rule-m3 item 8: Hash-V-pack against Hash-V-pack-null on one time measure: h = max(5%, its null spread)."""
    a, b = d.paired(job, C.NULL_ROW, C.NULL_OF, measure)
    if len(a) < 2 or min(np.min(x) for x in b) <= 0:
        return None, None
    r, lo, hi = ratio_of(measure, a, b)
    return max(C.BAND_MIN, 1 - lo, hi - 1, 0.0), {"ratio": r, "lo": lo, "hi": hi}


def judged(kind, measure):
    """rule-m3 item 7: measure 1 and the operation measure get a verdict, except in the reported pairing"""
    return measure in MEASURES_TIME and kind != C.REPORTED_KIND


def time_measure(d, job, row, ref, measure, kind):
    """T(ref)/T(row); above 1 the row is faster (rule-m3 items 7-9). A row over cap (its calibration rep past 10 s, no
    rounds): "over cap", with the calibration rep's time as the magnitude (item 1)."""
    if (job, row) in d.capped or (job, ref) in d.capped:
        cap = {r: (d.cal.get((job, r)) or {}).get("rep_ns") for r in (row, ref) if (job, r) in d.capped}
        return {"verdict": "over cap" if judged(kind, measure) else "reported", "ratio": None, "overcap": True,
                "cap_rep_ns": cap}
    a, b = d.paired(job, ref, row, measure)
    if len(a) < 2 or min(np.min(x) for x in b) <= 0:
        return {"verdict": NA, "ratio": None, "why": "too few rounds"}
    r, lo, hi = ratio_of(measure, a, b)
    out = {"ratio": r, "lo": lo, "hi": hi, "rounds": len(a)}
    if judged(kind, measure):
        h, null = null_h(d, job, measure)
        out.update(h=h, null=null, verdict=time_verdict(lo, hi, h) if h is not None else NA)
        if d.one_rep(job, row) or d.one_rep(job, ref):
            out["caveat"] = CAVEAT
    else:
        out["verdict"] = "reported"
    return out


def stable_bytes(d, job, impl):
    """measure 6 (v0.2 §15.6): the inputs' stable accounted bytes summed over their cells summed; and the heap's"""
    m = d.mem(job, impl)
    if not m:
        return None
    xs = [m.get(f"built_{x}") for x in C.SET_INPUTS[d.op]]
    if any(x is None for x in xs):
        return None
    cells = sum(x["cells"] for x in xs)
    return {"acct": sum(x["acct"] for x in xs) / cells, "heap": sum(x["heap"] for x in xs) / cells}


def peak_bytes(d, job, impl):
    """measures 7 and 8 (sop's and pos's peaks reported): the operation's accounted peak over its cells; the heap's"""
    m = d.mem(job, impl)
    p = (m or {}).get("op")
    if not p or not p.get("cells"):
        return None
    return {"acct": p["peak_acct"] / p["cells"], "heap": p["peak_heap"] / p["cells"]}


def memory_measure(d, job, row, ref, kind, which):
    """bytes per cell, row over reference (below 1 the row is smaller); the verdict on the accounted figure, the heap's
    reported beside it. which: "stable" (measure 6) or "peak" (7, 8; sop and pos reported)"""
    f = stable_bytes if which == "stable" else peak_bytes
    q, t = f(d, job, C.ROW_IMPL[row]), f(d, job, C.ROW_IMPL[ref])
    out = {"row": q, "ref": t}
    out["ratio"] = q["acct"] / t["acct"] if q and t and t["acct"] else None
    out["heap_ratio"] = q["heap"] / t["heap"] if q and t and t["heap"] else None
    judge = kind != C.REPORTED_KIND and (which == "stable" or C.PEAK_MEASURE[d.op] is not None)
    r = out["ratio"]
    out["verdict"] = (fixed_verdict(r) if r is not None else NA) if judge else "reported"
    out["severe"] = judge and r is not None and r >= C.SEVERE
    return out


def gcd_check(d, job):
    chk = d.st.get("call_checks", {}).get(str(job))
    if not chk:
        return None
    nops = d.gen.get(str(job), {}).get("nops")
    out = {"G_TC": chk["G_TC"], "G_per_op": chk["G_TC"] / nops if nops else None, "result": chk.get("result")}
    for i in C.HASH_IMPLS:
        out[i] = chk.get(i)
    for i in C.FAC_IMPLS:
        k = f"{C.IMPL_ROW[i]} against Hash-V-pack"
        out[k] = chk.get(k)
    return out


def factor_work(d, job, impl):
    """a factored row's factor work over the job (counting run): its factorings' and its steps' counts, summed"""
    c = d.count(job, impl) or {}
    tot = {}
    for f in c.get("fac", []):
        for k, v in f.items():
            if k not in ("name", "W", "E", "nq", "w", "bytes_f", "bytes_q"):
                tot[k] = tot.get(k, 0) + v
    for z, a in (c.get("facc") or {}).items():
        for k, v in a.items():
            tot[k] = tot.get(k, 0) + v
    return tot


# ------------------------------------------------------------------------------------------------- report
def report(outdir):
    outdir = Path(outdir)
    bad = C.forbidden(outdir)
    if bad:
        raise SystemExit(f"no report is written there: {bad}")
    st = json.loads((outdir / "state.json").read_text())
    op = st.get("op")
    bad = C.forbidden(outdir, op)                      # never in another m2 run's folder
    if bad:
        raise SystemExit(f"no report is written there: {bad}")
    d = Data(st)
    head = (outdir / "results.md").read_text() if (outdir / "results.md").exists() else ""
    cut = head.find("<!-- rule ends -->")
    head = head[:cut + len("<!-- rule ends -->")] + "\n" if cut >= 0 else head
    md = [head]
    md += run_section(st)
    md += pilot_section(st)
    finished = bool(st.get("finished")) and not st.get("run_stopped") and not st.get("hard_stopped")
    jobs = list(st.get("jobs", C.JOBS_OF.get(op, ())))
    rows_csv = []
    if finished:
        md += verdict_sections(d, st, jobs, rows_csv)
    else:
        md += ["## Verdicts", "", "**No verdict is reported: the run did not finish** (" +
               (f"stopped at the hard stop in {st['hard_stopped']['step']}" if st.get("hard_stopped") else
                f"stopped in {st['run_stopped']['step']}: {st['run_stopped']['reason']}" if st.get("run_stopped") else
                "unfinished") + ").", ""]
    md += memory_reported(d, st, jobs)
    md += build_times_section(d, st, jobs)
    md += storage_section(d, st, jobs)
    md += reported_sections(d, st, jobs)
    md += ["## Limitations", ""] + [f"- {x}" for x in LIMITATIONS] + [""]
    (outdir / "results.md").write_text("\n".join(md) + "\n")
    write_csv(outdir / "verdicts.csv", rows_csv)
    write_csv(outdir / "rounds.csv", [dict({k: r.get(k) for k in ("job", "row", "round", "pos", "r", "reps", "total",
                                                                   "operation", "read", "overcap", "cpu_ns",
                                                                   "span_ns", "cpu_share")},
                                           **{f"build_{x}": (r.get("builds") or [None] * 4)[i]
                                              for i, x in enumerate(C.SET_INPUTS[op])})
                                      for r in st.get("records", []) if r.get("role") == "round"])
    write_csv(outdir / "counters.csv", counter_rows(st))
    write_csv(outdir / "factoring.csv", factoring_rows(st))


def counter_rows(st):
    out = []
    for key, c in sorted(st.get("counts", {}).items()):
        impl, job = key.split(":")
        for l in c.get("counts", []):
            t = l.split()
            out.append({"job": job, "row": C.IMPL_ROW[impl], "kind": t[0], "matrix": t[1], "phase": t[2],
                        "part": t[3], "name": t[4], "value": t[5]})
    return out


def factoring_rows(st):
    """the factored rows' counts of rule-m3 item 13: per matrix (each factoring; (f) and (q) at the matrix's end) and per
    step (the factored arithmetic and the reconciliation), from the counting runs"""
    out = []
    for key, c in sorted(st.get("counts", {}).items()):
        impl, job = key.split(":")
        if impl not in C.FAC_IMPLS:
            continue
        for f in c.get("fac", []):
            out.append(dict({"job": job, "row": C.IMPL_ROW[impl], "kind": "factoring", "matrix": f["name"]},
                            **{k: v for k, v in f.items() if k != "name"}))
        for m, f in (c.get("ffig") or {}).get("end", {}).items():
            out.append(dict({"job": job, "row": C.IMPL_ROW[impl], "kind": "end", "matrix": m}, **f))
        for z, a in (c.get("facc") or {}).items():
            out.append(dict({"job": job, "row": C.IMPL_ROW[impl], "kind": "step", "matrix": z}, **a))
    return out


def run_section(st):
    op = st.get("op")
    md = ["## The run", ""]
    status = ("finished " + st["finished"]) if st.get("finished") and not st.get("run_stopped") else \
        ("**stopped at the hard stop**" if st.get("hard_stopped") else
         ("**stopped**: " + st["run_stopped"]["reason"] if st.get("run_stopped") else "unfinished"))
    md += [f"- The {op} run (job set `{op}`: " + "; ".join(f"{z} = {x} {'+' if k == 'add' else 'x'} {y}"
                                                            for k, x, y, z in C.SET_STEPS.get(op, ())) + ")" +
           (" **(toy: a harness test, not a measurement)**" if st.get("toy") else "") + ".",
           f"- Started {st.get('started')}; {status}.",
           f"- Step 0 (before the clock): {st.get('step0_s', NA)} s; step 0's files: {st.get('step0_vs_earlier', NA)}.",
           f"- Run time {st.get('elapsed_saved', 0) / 60:.1f} minutes (hard stop {C.HARD_STOP_S // 60}).",
           f"- Rule sha256 `{st.get('rule_sha256')}`; round 3's results after the run: {st.get('round3_after', NA)}; "
           f"m1's and m2's results and the other m3 runs' folders after the run: {st.get('protected_after', NA)}.",
           "- Host processes (the runner's ancestors): " + ", ".join(f"{n}[{p}]" for p, n in st.get("hosts", {}).items())]
    for s in st.get("code_sessions", []):
        md.append(f"- Code at {s['time']}: " + ", ".join(f"{k} `{v[:12]}`" for k, v in s["code"].items()))
    ut = st.get("unit_tests", {})
    if ut:
        md.append("- Unit tests: " + "; ".join(f"{k}: " + (v["out"].strip().splitlines() or ["?"])[-1]
                                               for k, v in ut.items()))
    for b in st.get("bugs", []) + st.get("harness_bugs", []):
        md.append(f"- **Bug**: {b}")
    md.append("")
    return md


def pilot_section(st):
    p = st.get("pilot")
    if not p:
        return []
    md = ["## The pilot (discarded, except what it records for the run)", "",
          f"- Jobs {p['jobs']}, one per size; block wall times " +
          ", ".join(f"job {j} {w:.1f} s" for j, w in p["walls_s"].items()) + ".",
          "- Per size class c (its pilot job): O_c the block's checks, K the calibration launches, P the two round "
          "launches (0 over cap), t the time per run from the calibration; N_c the run's jobs of that size:"]
    for c, b in sorted(p.get("blocks", {}).items()):
        md.append(f"  - size {c} (job {b['job']}, N_c = {p['N'].get(c, 0)}): O {b['O']:.1f} s; " +
                  "; ".join(f"{r}: K {b['K'].get(r, 0):.2f} s, P {b['P'].get(r, 0):.2f} s, t {fmt(b['t'].get(r), 3)} s"
                            for r in C.ROWS if r in b["K"] or r in b["P"]))
    md += [f"- Projection elapsed + {C.PILOT_MARGIN:g} [sum_c N_c (O_c + sum_rows (K + {C.PILOT_ROUND_SCALE} P)) + "
           f"1/{C.PILOT_SIDE_PARALLEL} sum_c N_c sum_rows {C.PILOT_SIDE_RUNS} t] = {p['projected_s'] / 60:.1f} min "
           f"(limit {p['limit_s'] / 60:g}).",
           f"- Host rates R_h (ticks per second): {p['host_rates']}."]
    if st.get("pilot_stopped"):
        md.append(f"- **The pilot stopped the run**: {'; '.join(st['pilot_stopped'])}.")
    md.append("")
    return md


def tcell(t):
    if t.get("overcap"):
        return t["verdict"] + " (calibration rep " + ", ".join(f"{r} {fmt(v / 1e9 if v else None, 1)} s"
                                                              for r, v in t.get("cap_rep_ns", {}).items()) + ")"
    if t.get("ratio") is None:
        return t["verdict"]
    s = f"{t['verdict']}, {fmt(t['ratio'])} [{fmt(t.get('lo'))}, {fmt(t.get('hi'))}]"
    if "h" in t:
        s += f", h {fmt(t.get('h'))}"
    if t.get("caveat"):
        s += " (c)"
    return s


def mcell(b):
    s = f"{b['verdict']}, {fmt(b['ratio'])} (heap {fmt(b['heap_ratio'])})"
    return s + (" **severe memory loss**" if b["severe"] else "")


def measure_names(op):
    n, what = C.OP_MEASURE[op]
    pk = C.PEAK_MEASURE[op]
    return {"build": "1 build time of one input matrix", "operation": f"{n} {what}",
            "stable": "6 stable bytes of one stored input matrix",
            "peak": (f"{pk} peak bytes during the operation" if pk else "peak bytes during the operation (reported)")}


def verdict_sections(d, st, jobs, rows_csv):
    op = d.op
    names = measure_names(op)
    md = ["## Verdicts, per job, pairing and measure (no overall winner)", "",
          f"Headline measures of this run: {names['build']}; {names['operation']}; {names['stable']}; {names['peak']}. "
          "Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round "
          "(median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure "
          "has its own band h from Hash-V-pack against Hash-V-pack-null. Total and read-out time are reported without "
          "a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the "
          "heap's in parentheses; (f) and (q) included). Pack-coprime against Pack-primes is reported with no verdict on "
          "any measure. A row over cap runs no rounds: its time verdicts are over cap, with its calibration rep's time; "
          "its memory verdicts stand. Gcd work: a check for Hash-V-pack, reported for the factored rows. (c): " +
          CAVEAT + ".", ""]
    per = {}
    for row, ref, kind in C.PAIRINGS:
        md += [f"### {row} against {ref} ({kind})", "",
               f"| job | {names['build']} | {names['operation']} | total (reported) | read-out (reported) | "
               f"{names['stable']} | {names['peak']} |", "| --- | --- | --- | --- | --- | --- | --- |"]
        for job in jobs:
            t = {m: time_measure(d, job, row, ref, m, kind) for m in MEASURES_TIME + REPORTED_TIMES}
            ms = memory_measure(d, job, row, ref, kind, "stable")
            mp = memory_measure(d, job, row, ref, kind, "peak")
            per[(row, ref, job)] = (t, ms, mp)
            md.append(f"| {job_label(st, job)} | {tcell(t['build'])} | {tcell(t['operation'])} | {tcell(t['total'])} | "
                      f"{tcell(t['read-out'])} | {mcell(ms)} | {mcell(mp)} |")
            for m in MEASURES_TIME + REPORTED_TIMES:
                x = t[m]
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "kind": kind, "measure": m,
                                 "verdict": x["verdict"], "ratio": x.get("ratio"), "lo": x.get("lo"),
                                 "hi": x.get("hi"), "h": x.get("h"), "caveat": "(c)" if x.get("caveat") else ""})
            for mk, b in (("stable bytes", ms), ("peak bytes", mp)):
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "kind": kind, "measure": mk,
                                 "verdict": b["verdict"], "ratio": b["ratio"], "heap_ratio": b["heap_ratio"],
                                 "severe": b["severe"]})
        md.append("")
    md += ["### Gcd work and the call-for-call checks", "",
           "Hash-V-pack: G(row)/G(TC) MUST be exactly 1, and its per-operation records equal TC's, every step (a check, "
           "carried over). The factored rows (rule-m3 item 7, v0.3 §16.9 item 3): per operation the same case and the "
           "same exact divisions, multiplications and additions as TC (checked); their gcds are obtained otherwise, so "
           "G(row)/G(TC) is reported, not checked, per part (entry; the operations' additions and multiplications), "
           "beside their factor work; and each factored row takes Hash-V-pack's arithmetic counters (but the "
           "operations' gcds) and store splits (v0.3 §16.9 item 4).", "",
           "| job | G per operation (TC) | Hash-V-pack: G/G(TC), call for call | " +
           " | ".join(f"{C.IMPL_ROW[i]}: G/G(TC) (entry, add, mul), call for call | {C.IMPL_ROW[i]} vs Hash-V-pack"
                      for i in C.FAC_IMPLS) + " |",
           "| --- | --- | --- | " + " | ".join("--- | ---" for _ in C.FAC_IMPLS) + " |"]
    for job in jobs:
        g = gcd_check(d, job)
        if not g:
            md.append(f"| {job} | {NA} |" + " |" * (1 + 2 * len(C.FAC_IMPLS)))
            continue
        x = g.get("MHVP") or {}
        cells = [f"{fmt(x.get('G_ratio'), 6)} ({x.get('gcd_work_check')}), {x.get('call_for_call')}"]
        rows_csv.append({"job": job, "pairing": "Hash-V-pack vs TC", "kind": "against TC",
                         "measure": "gcd work (check)", "verdict": x.get("gcd_work_check", NA), "ratio": x.get("G_ratio")})
        for i in C.FAC_IMPLS:
            y = g.get(i) or {}
            bp = y.get("G_ratio_by_part") or {}
            cells.append(f"{fmt(y.get('G_ratio'), 4)} (" + ", ".join(fmt(bp.get(ph), 4) for ph in ("entry", "add", "mul"))
                         + f"), {y.get('call_for_call')}")
            cells.append(str(g.get(f"{C.IMPL_ROW[i]} against Hash-V-pack")))
            rows_csv.append({"job": job, "pairing": f"{C.IMPL_ROW[i]} vs TC", "kind": "against TC",
                             "measure": "gcd work (reported)", "verdict": "reported", "ratio": y.get("G_ratio")})
        md.append(f"| {job} | {fmt(g['G_per_op'], 2)} | " + " | ".join(cells) + " |")
    md += ["", "Factor work of the factored rows beside the gcd work (counting runs, the job's factorings and steps "
               "summed): merges and entries compared, divisibility tests, per-piece gcds, trial-division candidates, "
               "refinement gcds (pass 1 and the reconciliation).", "",
           "| job | row | G(row)/G(TC) | merges | entries compared | tests | per-piece gcds | trial-division candidates | "
           "pass-1 gcds | pass-2 tests | reconciliation gcds | splits (pass 1 / reconciliation / leftover) |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        g = gcd_check(d, job) or {}
        for i in C.FAC_IMPLS:
            w = factor_work(d, job, i)
            if not w:
                continue
            md.append(f"| {job} | {C.IMPL_ROW[i]} | {fmt((g.get(i) or {}).get('G_ratio'), 4)} | {w.get('merges', 0)} | "
                      f"{w.get('mentries', 0)} | {w.get('tests', 0)} | {w.get('pgcd', 0)} | {w.get('td_cand', 0)} | "
                      f"{w.get('rgcd', 0)} | {w.get('p2tests', 0)} | {w.get('rc_gcd', 0)} | {w.get('splits', 0)} / "
                      f"{w.get('rc_splits', 0)} / {w.get('lsplits', 0)} |")
    headline = ["build", "operation", "stable"] + (["peak"] if C.PEAK_MEASURE[op] else [])
    md += ["", "### Strata (counts of verdicts; no measure combined, no stratum scored)", "",
           "The strata are the sharing levels of the inputs (rule-m3 item 11): " +
           ", ".join(f"{s} (rho = {C.STRATA[s]})" for s in C.STRATA_OF[op]) +
           (" (sop has no MID)" if op == "sop" else "") + ". The run's headline measures only.", "",
           "| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe "
           "flags |", "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for s in C.STRATA_OF[op]:
        for row, ref, kind in C.PAIRINGS:
            js = [j for j in jobs if C.job_rho(j) == C.STRATA[s] and (row, ref, j) in per]
            for meas in headline:
                if meas in MEASURES_TIME:
                    xs = [per[(row, ref, j)][0][meas] for j in js]
                    sev = 0
                else:
                    xs = [per[(row, ref, j)][1 if meas == "stable" else 2] for j in js]
                    sev = sum(1 for x in xs if x["severe"])
                ratios = [x.get("ratio") for x in xs]
                vs = [x["verdict"] for x in xs]
                md.append(f"| {s} | {row} vs {ref} | {meas} | {vs.count('win')} | {vs.count('tie')} | {vs.count('loss')} | "
                          f"{vs.count('over cap')} | {vs.count(NA)} | {vs.count('reported')} | {fmt(median(ratios))} | "
                          f"{sev} |")
    md += ["", "### Size x sharing grid (ratios; presented, not judged)", ""]
    for row, ref, kind in C.PAIRINGS:
        for meas in headline:
            md += [f"{row} against {ref}, {names[meas]}:", "",
                   "| n | " + " | ".join(f"rho = {r}" for r in C.RHOS) + " |",
                   "| --- | " + " | ".join("---" for _ in C.RHOS) + " |"]
            for si, n in enumerate(C.TOY_SIZES if st.get("toy") else C.SIZES):
                cells = []
                for ri in range(len(C.RHOS)):
                    j = 4 * si + ri + 1
                    if (row, ref, j) not in per:
                        cells.append(NA)
                        continue
                    t, ms, mp = per[(row, ref, j)]
                    x = t[meas] if meas in MEASURES_TIME else (ms if meas == "stable" else mp)
                    cells.append(fmt(x.get("ratio")) if x.get("ratio") is not None else
                                 tcell(x) if x.get("overcap") else x["verdict"])
                md.append(f"| {n} | " + " | ".join(cells) + " |")
            md.append("")
    return md


def memory_reported(d, st, jobs):
    """rule-m3 item 7, reported with no verdict: the whole job's bytes after creation and at the end, the peak during
    each input's build, the operation's peak (sop, pos: reported); accounted (heap)"""
    op = d.op
    md = ["## Memory, reported (bytes per cell; accounted, heap in parentheses)", "",
          "From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and "
          "the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's "
          f"cells ({len(C.SET_MATRICES[op])} n^2); each input's stable bytes, and its build peak: the most the job's "
          "accounted total rose above its value before that input's create, during the build (its factoring "
          "included), over that input's cells. (f), (f'), (q) and (r) are accounted (v0.3 §16.10).", "",
          "| job | row | after creation | end | operation peak | " +
          " | ".join(f"{x}'s stable (build peak)" for x in C.SET_INPUTS[op]) + " |",
          "| --- | --- | --- | --- | --- | " + " | ".join("---" for _ in C.SET_INPUTS[op]) + " |"]
    for job in jobs:
        for impl in C.IMPLS:
            m = d.mem(job, impl)
            if not m:
                continue

            def pc(x, a, h, cells="cells"):
                return f"{fmt(x[a] / x[cells], 2)} ({fmt(x[h] / x[cells], 2)})" if x and x.get(cells) else NA
            built = []
            for x in C.SET_INPUTS[op]:
                b = m.get(f"built_{x}")
                built.append(f"{fmt(b['acct'] / b['cells'], 2)} ({fmt((b['peak_acct'] - b['base_acct']) / b['cells'], 2)})"
                             if b else NA)
            md.append(f"| {job} | {C.IMPL_ROW[impl]} | {pc(m.get('after_creation'), 'acct', 'heap')} | "
                      f"{pc(m.get('end'), 'acct', 'heap')} | {pc(m.get('op'), 'peak_acct', 'peak_heap')} | " +
                      " | ".join(built) + " |")
    md.append("")
    return md


def build_times_section(d, st, jobs):
    """rule-m3 item 7, reported: per-matrix build times (the median over rounds of each input's build time per run)"""
    op = d.op
    md = ["## Build time per input matrix (reported, no verdict)", "",
          "The median over rounds of each input matrix's build time per run, ns.", "",
          "| job | row | " + " | ".join(C.SET_INPUTS[op]) + " |", "| --- | --- | " +
          " | ".join("---" for _ in C.SET_INPUTS[op]) + " |"]
    for job in jobs:
        for row in C.ROWS:
            recs = list(d.rounds.get((job, row), {}).values())
            if not recs:
                continue
            md.append(f"| {job} | {row} | " + " | ".join(fmt(median([r["builds"][i] for r in recs]), 0)
                                                          for i in range(len(C.SET_INPUTS[op]))) + " |")
    md.append("")
    return md


def storage_section(d, st, jobs):
    """rule-m3 item 13: per job and matrix, at its end, from the counting runs (exact; the gates checked every figure
    against the model): Hash-V-pack's stores and packings (the factored rows' are the same), and the factored rows' (f)
    and (q), and their factorings"""
    md = ["## Stores, per job and matrix (rule-m3 item 13)", "",
          "From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is "
          "reserved: W counts the whole numbers stored. T is the build table's. The factored rows' stores, lists and "
          "packings are Hash-V-pack's (checked).", "",
          "| job | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings "
          "(slots copied) | cells bytes | list bytes |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        c = d.count(job, "MHVP")
        if not c or "stor" not in c:
            continue
        for r in C.SET_MATRICES[d.op]:
            s = c["stor"].get("end", {}).get(r)
            if not s:
                continue
            n = s["found"] + s["new"]
            md.append(f"| {job} | {r} | {s['W']} | {s['T']} | {s['W'] / s['T']:.1%} | {s['w']} | {s['wh']} | "
                      f"{s['found']} / {s['new']} | {fmt(s['probes'] / n if n else None, 2)} | {s['longest']} | "
                      f"{s['widenings']} ({s['copied']}) | {s['bcells']} | {s['bvals']} |")
    md += ["", "### Hash-V-pack's packings (from the counting runs)", "",
           "| job | matrix | W | w_h build | w_h packed | list bytes | cells rewritten | map bytes |",
           "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        c = d.count(job, "MHVP")
        for p in (c or {}).get("packs", []):
            md.append(f"| {job} | {p['name']} | {p['W']} | {p['whb']} | {p['whp']} | {p['bytes_list']} | "
                      f"{p['rewritten']} | {p['bytes_map']} |")
    md += ["", "### The factored rows' factorings (from the counting runs, each at its factoring)", "",
           "Pack-primes: trial-division candidates, divisions with remainder and exact divisions. Pack-coprime: pass 1's "
           "scan gcds, divisions and splits (with the splits' gcds), pass 2's divisibility tests. E: (f)'s entries; "
           "|q|: the base's pieces.", "",
           "| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes | candidates | divisions | exact | pass-1 gcds | "
           "pass-1 divisions | splits (gcds) | pass-2 tests |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        for i in C.FAC_IMPLS:
            for f in (d.count(job, i) or {}).get("fac", []):
                md.append(f"| {job} | {C.IMPL_ROW[i]} | {f['name']} | {f['W']} | {f['E']} | {f['bytes_f']} | {f['nq']} | "
                          f"{f['bytes_q']} | {f['td_cand']} | {f['td_divs']} | {f['td_exact']} | {f['rgcd']} | "
                          f"{f['rdiv']} | {f['splits']} ({f['sgcd']}) | {f['p2tests']} |")
    md += ["", "### The factored rows' (f) and (q) at each matrix's end (after every reconciliation)", "",
           "| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes |", "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        for i in C.FAC_IMPLS:
            for m, f in ((d.count(job, i) or {}).get("ffig") or {}).get("end", {}).items():
                md.append(f"| {job} | {C.IMPL_ROW[i]} | {m} | {f['W']} | {f['E']} | {f['bytes_f']} | {f['nq']} | "
                          f"{f['bytes_q']} |")
    md.append("")
    return md


def reported_sections(d, st, jobs):
    op = d.op
    md = ["## Also reported (rule-m3 item 13)", "", "### Realized sharing", "",
          "s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-pack's stores, from the "
          "model; the gates checked it).", "",
          "| job | n | rho | s | " + " | ".join(f"distinct {x}" for x in C.SET_MATRICES[op]) + " | " +
          " | ".join(f"W_{x}" for x in C.SET_MATRICES[op]) + " |",
          "| --- | --- | --- | --- | " + " | ".join("---" for _ in range(2 * len(C.SET_MATRICES[op]))) + " |"]
    for job in jobs:
        g = st.get("gen", {}).get(str(job))
        if not g:
            continue
        r = g.get("realized", {})
        md.append(f"| {job} | {g['n']} | {g['rho']} | {g['s']} | " +
                  " | ".join(str(r.get("distinct", {}).get(x)) for x in C.SET_MATRICES[op]) + " | " +
                  " | ".join(str(r.get("W", {}).get(x)) for x in C.SET_MATRICES[op]) + " |")
    md += ["", "### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.3 §16.9)", ""]
    for job in jobs:
        g = st.get("gen", {}).get(str(job))
        if not g or not g.get("steps"):
            continue
        for k, x, y, z in C.SET_STEPS[op]:
            s = g["steps"][z]
            nc = s["ncase"]
            parts = []
            for alg in ("add", "mul"):
                tot = sum(v for kk, v in nc.items() if kk.startswith(alg + "_"))
                if tot:
                    parts.append(f"{alg} ({tot}): " + ", ".join(f"{kk[len(alg) + 1:]} {nc[kk] / tot:.3f}"
                                                                for kk in C.CASES_OF[alg] if nc.get(kk)))
            c = d.count(job, "MHVP")
            o = (c or {}).get("opc", {}).get(z, {})
            se = (c or {}).get("stor", {}).get("end", {}).get(z, {})
            md.append(f"- job {job}, {z} = {x} {'+' if k == 'add' else 'x'} {y}: " + "; ".join(parts) +
                      f"; cross tests {o.get('cross')} (matched {o.get('cross_match')}, cross reads "
                      f"{o.get('cross_reads')}); {z}'s store W {se.get('W')}, found / new {se.get('found')} / "
                      f"{se.get('new')}")
    md += ["", "### Per step: the factored arithmetic (rule-m3 item 13; counting runs)", "",
           "Reconciliation (Pack-coprime): passes, gcds, splits, |D_L| and |D_R|, (f) rebuilds (entries written). The "
           "arithmetic: merges (entries compared), the temporaries' lists (comparisons forming them), divisibility "
           "tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, and the new sub-cases "
           "(9a-c num, 10a-c general, 12a-c multiplication's general).", "",
           "| job | row | step | reconciliation | merges (entries) | list comparisons | tests | per-piece gcds | factor "
           "multiplications | leftover splits | 9a / 9b / 9c | 10a / 10b / 10c | 12a / 12b / 12c |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        for i in C.FAC_IMPLS:
            for z, a in ((d.count(job, i) or {}).get("facc") or {}).items():
                md.append(f"| {job} | {C.IMPL_ROW[i]} | {z} | {a['rc_passes']} passes, {a['rc_gcd']} gcds, "
                          f"{a['rc_splits']} splits, D {a['rc_DL']} / {a['rc_DR']}, {a['rebuilds']} rebuilds "
                          f"({a['rentries']}) | {a['merges']} ({a['mentries']}) | {a['lwalk']} | {a['tests']} | "
                          f"{a['pgcd']} | {a['fmul']} | {a['lsplits']} | {a['num_cop']} / {a['num_uneq']} / "
                          f"{a['num_eq']} | {a['gen_cop']} / {a['gen_uneq']} / {a['gen_eq']} | {a['mgen_cop']} / "
                          f"{a['mgen_one']} / {a['mgen_two']} |")
    md += ["", "The gates checked every counter and store figure against the model (spec v0.3 §16.9 item 2), the TC "
               "gate included: " + ", ".join(f"{k} {'pass' if 'fail' not in v else 'FAIL'}"
                                             for k, v in st.get("gates", {}).items()), ""]
    md += ["### The idle and disturbance checks", "",
           "As round 3 and m1 (rule-m3 item 5, carried over in full): processes are told apart by process, not by "
           "thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). "
           "The runner's own ticks are reported in total and per launch; the one sanity limit is more than "
           f"{C.RUNNER_TICKS_PER_LAUNCH_MAX:g} tick per launch plus {C.RUNNER_TICKS_BLOCK_ALLOWANCE} over a block.", "",
           "| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks "
           "(reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | "
           "host ticks |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for x in st.get("disturbance_checks", []):
        hosts = ", ".join(f"{h['name']} {h['ticks']}" + (f"/{h['allowance']}" if "allowance" in h else "") +
                          (f" ({h['threads'][0]}->{h['threads'][1]} threads)" if "threads" in h else "")
                          for h in x["hosts"].values())
        quiet = ", ".join(f"{q['name']}[{q['pid']}]" for q in x.get("new_quiet", [])) or "none"
        cred = ", ".join(f"{c['name']}[{p}] {c['ticks']}" for p, c in x.get("credited", {}).items()) or "none"
        md.append(f"| {x['job']} | {x['label']} | {x['attempt']} | {x['seconds']} | "
                  f"{'yes: ' + '; '.join(x['reasons']) if x['disturbed'] else 'no'} | {x['runner_ticks']} | "
                  f"{x['spawns']} | {runner_per_launch(x)} | {x['forks_rise']} | {x.get('forks_unattributed')} | "
                  f"{quiet} | {x.get('other_limit', C.DIST_OTHER_TICKS)} | {cred} | {hosts} |")
    md += runner_summary(st)
    md += cpu_time_section(st)
    md += ["", f"Idle checks: {len(st.get('idle_checks', []))} (" +
           f"{sum(1 for c in st.get('idle_checks', []) if not c['idle'])} busy). Host rates R_h: "
           f"{st.get('host_rates', {})} (measured: {st.get('host_rates_measured', {})}; R_h is at least 1 tick over "
           f"the longest pilot block).", "",
           "### Earlier rounds, for reference only", "",
           "Round m2's runs, [`../results-m2-add/results.md`](../results-m2-add/results.md), "
           "[`../results-m2-mul/results.md`](../results-m2-mul/results.md), "
           "[`../results-m2-sop/results.md`](../results-m2-sop/results.md) and "
           "[`../results-m2-pos/results.md`](../results-m2-pos/results.md); m1's, "
           "[`../results-m1-add/results.md`](../results-m1-add/results.md) and "
           "[`../results-m1-mul/results.md`](../results-m1-mul/results.md); and round 3's, "
           "[`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They "
           "enter no measure here.", ""]
    return md


def cpu_time_section(st):
    """each timed rep's own process CPU time beside its time, reported only"""
    recs = [r for r in st.get("records", []) if r.get("role") == "round" and r.get("cpu_share") is not None]
    if not recs:
        return []
    md = ["", "### The measured process's own CPU time (reported only; enters no verdict)", "",
          "CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the "
          "same whole rep, for the rep of minimum total in each round.", "",
          "| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, "
          "median |", "| --- | --- | --- | --- | --- | --- | --- |"]
    for job in sorted({r["job"] for r in recs}):
        for row in C.ROWS:
            x = [r for r in recs if r["job"] == job and r["row"] == row]
            if x:
                md.append(f"| {job} | {row} | {len(x)} | {median([r['cpu_share'] for r in x]):.4f} | "
                          f"{min(r['cpu_share'] for r in x):.4f} | {median([r['cpu_ns'] for r in x]):.0f} | "
                          f"{median([r['total'] for r in x]):.0f} |")
    return md


if __name__ == "__main__":
    report(sys.argv[1])
