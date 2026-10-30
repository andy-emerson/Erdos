"""Analysis of one run of the matrix round (research/rule-m1.md items 7-13, Run plan step 7): results.md and CSVs from
OUTDIR/state.json. From round 3's r3_analyze.py (../../10-interning/test/, unchanged there); round 2's statistics and
formatting, which round 3 imported from r2_analyze.py, are copied here unchanged (boot_ratio, time_verdict,
fixed_verdict, fmt, median, write_csv, runner_per_launch, runner_summary), so that nothing is imported, or compiled,
from ../10-interning/ (DECISIONS M1-H7).

    python3 m1_analyze.py OUTDIR

Per job, pairing and measure only, for the nine pairings of rule-m1 item 7 (four against TC; M against V within each
sizing; grow against fixed within V and within M; map against lookup): creation time and operation time, each with its
own verdict (T(reference)/T(row), paired bootstrap interval, band h = max(5%, that measure's null spread from
Hash-V-fixed against Hash-V-fixed-null)); total and read-out time with ratio and interval and no verdict; bytes per cell
accounted (B_used: tables counted in full, tau included) and on the heap (B_heap), each with its verdict and magnitude;
gcd work as a check, not a verdict (exactly 1 for every hash row). Map against lookup has a verdict on creation only:
its operation and total time and its end memory are reported, and its growth peak is a ratio without a verdict. Also:
the growth peak of every grow row, tau's build time and its share of the operation part, the strata (counts of
verdicts; no overall winner), the size x sharing grid of ratios (presented, not judged), and item 13's reported items.
A run that stopped is reported with what it has and no verdict. Never writes into ../10-interning/, and the
multiplication run's report never into the addition run's folder.
"""
import csv
import io
import json
import statistics
import sys
from pathlib import Path

import numpy as np

import m1_checks as K
import m1_common as C

NA = "n/a"
FIELD = {"creation": "creation", "operation": "operation", "total": "total", "read-out": "read"}
CAVEAT = ("the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of "
          "the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ "
          "(rule-m1 item 8)")
LIMITATIONS = [
    "**Textbook only.** No row is optimized: plain open addressing with linear probing; growth by doubling.",
    "**The index store is not tested** in this round (the author's decision).",
    "**Fixed's fill is low and not controlled:** its tables are sized from the dimensions (T >= 2n^2 + 2), and it "
    "loses on memory by design. **Grow has a fill limit** of one half (spec section 5.6), which the author rejected for "
    "round 3's fixed table.",
    "**The map method is tested on V only;** the rewrite method and V/M are taken to act on different parts (creation "
    "and operation), though C's growths put a little of the rewrite into the operation part.",
    "**Addition is floor-dominated:** its runs take milliseconds, so its time ratios rest on reps of about 50 ms, made "
    "of many runs; the null band shows how far that holds.",
    "**Two runs, not one:** the runs are separate in time, so machine conditions can differ between them; their results "
    "are not combined.",
    "**64 bits only.** A value past 64 bits is an error in every build; the sweep stays far below.",
    "**Square matrices, one value distribution.** A and B have the same sharing level.",
    "Every other limitation of round 3 that applies (the disturbance check cannot see every process; a child of a "
    "process that ignores SIGCHLD is not charged).",
]


# ------------------------------------------------------------------------------------------------- statistics
# round 2's (r2_analyze.py), unchanged
def boot_ratio(a, b):
    """median(a) / median(b) and its 95% interval over BOOT paired resamples of the rounds (the same draw for both)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    R = len(a)
    rng = np.random.default_rng(C.BOOT_SEED)
    idx = rng.integers(0, R, size=(C.BOOT, R))
    ra = np.median(a[idx], axis=1) / np.median(b[idx], axis=1)
    lo, hi = np.percentile(ra, [2.5, 97.5])
    return float(np.median(a) / np.median(b)), float(lo), float(hi)


def time_verdict(lo, hi, h):
    if lo > 1 + h:
        return "win"
    if hi < 1 - h:
        return "loss"
    return "tie"


def fixed_verdict(ratio):
    """bytes: row over reference; at most 0.90 the row wins, at least 1.10 it loses (rule-m1 item 9)"""
    if ratio <= C.FIXED_WIN:
        return "win"
    if ratio >= C.FIXED_LOSS:
        return "loss"
    return "tie"


def fmt(x, nd=3):
    if x is None:
        return NA
    if isinstance(x, float):
        return f"{x:.{nd}f}"
    return str(x)


def median(v):
    v = [x for x in v if x is not None]
    return statistics.median(v) if v else None


def write_csv(path, rows):
    if not rows:
        path.write_text("")
        return
    keys = list(rows[0].keys())
    for r in rows[1:]:
        for k in r:
            if k not in keys:
                keys.append(k)
    s = io.StringIO()
    w = csv.DictWriter(s, fieldnames=keys)
    w.writeheader()
    for r in rows:
        w.writerow(r)
    path.write_text(s.getvalue())


def runner_per_launch(x):
    t, n = x.get("runner_ticks"), x.get("spawns")
    return f"{t / n:.3f}" if t is not None and n else "n/a"


def runner_summary(st):
    xs = [x for x in st.get("disturbance_checks", []) if x.get("runner_ticks") is not None]
    if not xs:
        return []
    ticks, launches = sum(x["runner_ticks"] for x in xs), sum(x["spawns"] for x in xs)
    per = [x["runner_ticks"] / x["spawns"] for x in xs if x["spawns"]]
    return ["", f"The runner's own ticks over the {len(xs)} blocks: {ticks} ticks over {launches} launches, "
                f"{ticks / launches if launches else float('nan'):.3f} per launch; the highest block "
                f"{max(per) if per else float('nan'):.3f} per launch (sanity limit: more than "
                f"{C.RUNNER_TICKS_PER_LAUNCH_MAX:g} per launch plus {C.RUNNER_TICKS_BLOCK_ALLOWANCE} per block). "
                f"Reported only: they fall between reps, not in them."]


# ------------------------------------------------------------------------------------------------- data
class Data:
    def __init__(self, st):
        self.st = st
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
        return self.mems.get((str(job), impl))

    def paired(self, job, x, y, field="total"):
        a, b = self.rounds.get((job, x), {}), self.rounds.get((job, y), {})
        common = sorted(set(a) & set(b))
        return [a[k][field] for k in common], [b[k][field] for k in common]

    def median_of(self, job, row, field):
        return median([r.get(field) for r in self.rounds.get((job, row), {}).values()])

    def one_rep(self, job, row):
        return (self.cal.get((job, row)) or {}).get("reps") == 1


def job_label(st, job):
    g = st.get("gen", {}).get(str(job), {})
    n, rho = g.get("n", C.job_n(job, bool(st.get("toy")))), g.get("rho", C.job_rho(job))
    return f"{job} (n {n}, rho {rho})"


# ------------------------------------------------------------------------------------------------- measures
def null_h(d, job, measure):
    """rule-m1 item 8: Hash-V-fixed against Hash-V-fixed-null on one time measure: h = max(5%, its null spread)."""
    a, b = d.paired(job, C.NULL_ROW, C.NULL_OF, FIELD[measure])
    if len(a) < 2 or min(b) <= 0:
        return None, None
    r, lo, hi = boot_ratio(a, b)
    return max(C.BAND_MIN, 1 - lo, hi - 1, 0.0), {"ratio": r, "lo": lo, "hi": hi}


def judged(kind, measure):
    """rule-m1 item 7: creation and operation get a verdict, but map against lookup is judged on creation only"""
    if measure not in C.TIME_MEASURES:
        return False
    return not (kind == "map against lookup" and measure != "creation")


def time_measure(d, job, row, ref, measure, kind):
    """T(ref)/T(row) on one part; above 1 the row is faster (rule-m1 items 7-9)."""
    if (job, row) in d.capped or (job, ref) in d.capped:
        return {"verdict": "over cap" if judged(kind, measure) else "reported", "ratio": None, "overcap": True}
    a, b = d.paired(job, ref, row, FIELD[measure])
    if len(a) < 2 or min(b) <= 0:
        return {"verdict": NA, "ratio": None, "why": "too few rounds"}
    r, lo, hi = boot_ratio(a, b)
    out = {"ratio": r, "lo": lo, "hi": hi, "rounds": len(a)}
    if judged(kind, measure):
        h, null = null_h(d, job, measure)
        out.update(h=h, null=null, verdict=time_verdict(lo, hi, h) if h is not None else NA)
        if d.one_rep(job, row) or d.one_rep(job, ref):
            out["caveat"] = CAVEAT
    else:
        out["verdict"] = "reported"
    return out


def bytes_used(d, job, impl, when):
    """the accounted bytes per cell (spec section 9): every structure's report, tau included, over 3 n^2 cells"""
    c = d.count(job, impl)
    if not c or "mem" not in c:
        return None
    m = K.parse_mem(c["mem"]).get(when)
    if not m or not m.get("N"):
        return None
    return (sum(u for u, _ in m["items"].values()) + m.get("tau", 0)) / m["N"]


def bytes_heap(d, job, impl, when):
    m = d.mem(job, impl)
    if not m or "memrun" not in m or when not in m["memrun"]:
        return None
    r = m["memrun"][when]
    return r["heap"] / r["cells"] if r.get("cells") else None


def memory_measure(d, job, row, ref, kind, mk):
    """bytes per cell, row over reference (below 1 the row is smaller); verdicts on the end figures. Map against
    lookup: reported only (its end memory is equal by construction, rule-m1 item 7)."""
    f = bytes_used if mk == "B_used" else bytes_heap
    ri, fi = C.ROW_IMPL[row], C.ROW_IMPL[ref]
    out = {}
    for when in ("after_creation", "end"):
        q, t = f(d, job, ri, when), f(d, job, fi, when)
        out[when] = {"row": q, "ref": t, "ratio": q / t if q is not None and t else None}
    r = out["end"]["ratio"]
    if kind == "map against lookup":
        out["verdict"] = "reported"
    else:
        out["verdict"] = fixed_verdict(r) if r is not None else NA
    out["severe"] = r is not None and r >= C.SEVERE and kind != "map against lookup"
    return out


def growth_peak(d, job, impl):
    """rule-m1 item 7: the most bytes, accounted and heap, at any growth, per cell (the memory run's peak line)"""
    m = d.mem(job, impl)
    p = (m or {}).get("memrun", {}).get("peak")
    if not p or not p.get("cells"):
        return None
    return {"used": p["used"] / p["cells"], "heap": p["heap"] / p["cells"], "growths": p.get("growths")}


def gcd_check(d, job):
    chk = d.st.get("call_checks", {}).get(str(job))
    if not chk:
        return None
    nops = d.gen.get(str(job), {}).get("nops")
    out = {"G_TC": chk["G_TC"], "G_per_op": chk["G_TC"] / nops if nops else None, "result": chk.get("result")}
    for i in C.HASH_IMPLS:
        out[i] = chk.get(i)
    for k in ("M against V (fixed)", "M against V (grow)", "grow against fixed (V)", "grow against fixed (M)",
              "map against lookup"):
        out[k] = chk.get(k)
    return out


def tau_time(d, job, row):
    """tau's build time per run (the median over rounds) and its share of the operation part (the median of the
    per-round shares); reported, no verdict (rule-m1 item 4)"""
    recs = list(d.rounds.get((job, row), {}).values())
    t = median([r.get("tau") for r in recs])
    sh = median([r["tau"] / r["operation"] for r in recs if r.get("tau") is not None and r.get("operation")])
    return t, sh


# ------------------------------------------------------------------------------------------------- report
def report(outdir):
    outdir = Path(outdir)
    st = json.loads((outdir / "state.json").read_text())
    op = st.get("op")
    bad = C.forbidden(outdir, op)
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
    jobs = [j for j in st.get("jobs", C.JOBS)]
    rows_csv = []
    if finished:
        md += verdict_sections(d, st, jobs, rows_csv)
        md += peak_section(d, st, jobs)
        md += tau_section(d, st, jobs)
    else:
        md += ["## Verdicts", "", "**No verdict is reported: the run did not finish** (" +
               (f"stopped at the hard stop in {st['hard_stopped']['step']}" if st.get("hard_stopped") else
                f"stopped in {st['run_stopped']['step']}: {st['run_stopped']['reason']}" if st.get("run_stopped") else
                "unfinished") + ").", ""]
    md += storage_section(d, st, jobs)
    md += reported_sections(d, st, jobs)
    md += ["## Limitations", ""] + [f"- {x}" for x in LIMITATIONS] + [""]
    (outdir / "results.md").write_text("\n".join(md) + "\n")
    write_csv(outdir / "verdicts.csv", rows_csv)
    write_csv(outdir / "rounds.csv", [{k: r.get(k) for k in ("job", "row", "round", "pos", "r", "reps", "total",
                                                              "creation", "operation", "read", "tau", "overcap",
                                                              "cpu_ns", "span_ns", "cpu_share")}
                                      for r in st.get("records", []) if r.get("role") == "round"])
    write_csv(outdir / "counters.csv", counter_rows(st))


def counter_rows(st):
    out = []
    for key, c in sorted(st.get("counts", {}).items()):
        impl, job = key.split(":")
        for l in c.get("counts", []):
            t = l.split()
            out.append({"job": job, "row": C.IMPL_ROW[impl], "kind": t[0], "phase": t[1], "part": t[2], "name": t[3],
                        "value": t[4]})
    return out


def run_section(st):
    op = st.get("op")
    md = ["## The run", ""]
    status = ("finished " + st["finished"]) if st.get("finished") and not st.get("run_stopped") else \
        ("**stopped at the hard stop**" if st.get("hard_stopped") else
         ("**stopped**: " + st["run_stopped"]["reason"] if st.get("run_stopped") else "unfinished"))
    md += [f"- The {'addition' if op == 'add' else 'multiplication'} run (job set `{op}`)" +
           (" **(toy: a harness test, not a measurement)**" if st.get("toy") else "") + ".",
           f"- Started {st.get('started')}; {status}.",
           f"- Step 0 (before the clock): {st.get('step0_s', NA)} s.",
           f"- Run time {st.get('elapsed_saved', 0) / 60:.1f} minutes (hard stop {C.HARD_STOP_S // 60}).",
           f"- Rule sha256 `{st.get('rule_sha256')}`; round 3's results after the run: {st.get('round3_after', NA)}.",
           "- Host processes (the runner's ancestors): " + ", ".join(f"{n}[{p}]" for p, n in st.get("hosts", {}).items())]
    if op == "mul":
        md.append(f"- Binaries: {st.get('binaries_vs_addition', NA)}; the addition run's results after this run: "
                  f"{st.get('addition_results_after', NA)}.")
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
          f"- Jobs {p['jobs']}; block wall times " + ", ".join(f"job {j} {w:.1f} s" for j, w in p["walls_s"].items()) +
          f"; P_6 = {p['P_6_s']:.1f} s, P_13 = {p['P_13_s']:.1f} s; N_128 = {p['N_128']}, N_rest = {p['N_rest']}.",
          f"- Projection elapsed + {C.PILOT_MARGIN:g} x {C.PILOT_SCALE:g} x (N_128 P_13 + N_rest P_6) = "
          f"{p['projected_s'] / 60:.1f} min (limit {p['limit_s'] / 60:g}).",
          f"- Host rates R_h (ticks per second): {p['host_rates']}."]
    if st.get("pilot_stopped"):
        md.append(f"- **The pilot stopped the run**: {'; '.join(st['pilot_stopped'])}.")
    md.append("")
    return md


def tcell(t):
    if t.get("ratio") is None:
        return t["verdict"]
    s = f"{t['verdict']}, {fmt(t['ratio'])} [{fmt(t.get('lo'))}, {fmt(t.get('hi'))}]"
    if "h" in t:
        s += f", h {fmt(t.get('h'))}"
    if t.get("caveat"):
        s += " (c)"
    return s


def mcell(b):
    s = f"{b['verdict']}, {fmt(b['end']['ratio'])} ({fmt(b['after_creation']['ratio'])})"
    return s + (" **severe memory loss**" if b["severe"] else "")


def verdict_sections(d, st, jobs, rows_csv):
    md = ["## Verdicts, per job, pairing and measure (no overall winner)", "",
          "Time: T(reference)/T(row), above 1 the row is faster; creation and operation each have their own band h "
          "from Hash-V-fixed against Hash-V-fixed-null on that part. Total and read-out time are reported without a "
          "verdict. Bytes per cell: row over reference, below 1 the row is smaller (end of the operation; after "
          "creation in parentheses). Map against lookup is judged on creation only. Gcd work is a check, not a "
          "verdict. (c): " + CAVEAT + ".", ""]
    per = {}
    for row, ref, kind in C.PAIRINGS:
        md += [f"### {row} against {ref} ({kind})", "",
               "| job | creation: verdict, ratio [lo, hi], h | operation: verdict, ratio [lo, hi], h | total "
               "(reported) | read-out (reported) | B_used (end): verdict, ratio (after creation) | B_heap (end): "
               "verdict, ratio (after creation) |", "| --- | --- | --- | --- | --- | --- | --- |"]
        for job in jobs:
            t = {m: time_measure(d, job, row, ref, m, kind) for m in C.TIME_MEASURES + C.REPORTED_TIMES}
            bu = memory_measure(d, job, row, ref, kind, "B_used")
            bh = memory_measure(d, job, row, ref, kind, "B_heap")
            per[(row, ref, job)] = (t, bu, bh)
            md.append(f"| {job_label(st, job)} | {tcell(t['creation'])} | {tcell(t['operation'])} | "
                      f"{tcell(t['total'])} | {tcell(t['read-out'])} | {mcell(bu)} | {mcell(bh)} |")
            for m in C.TIME_MEASURES + C.REPORTED_TIMES:
                x = t[m]
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "kind": kind, "measure": m,
                                 "verdict": x["verdict"], "ratio": x.get("ratio"), "lo": x.get("lo"),
                                 "hi": x.get("hi"), "h": x.get("h"), "caveat": "(c)" if x.get("caveat") else ""})
            for mk, b in (("B_used", bu), ("B_heap", bh)):
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "kind": kind, "measure": mk,
                                 "verdict": b["verdict"], "ratio": b["end"]["ratio"],
                                 "after_creation": b["after_creation"]["ratio"], "severe": b["severe"]})
        md.append("")
    md += ["### Gcd work (a check, not a verdict) and the call-for-call checks", "",
           "G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m1 item 7); the counting runs' per-operation "
           "records of every hash row equal TC's, and V against M, grow against fixed and map against lookup take the "
           "same cases, core counts and store splits (spec section 10 items 3-6).", "",
           "| job | G per operation | " + " | ".join(f"G({C.IMPL_ROW[i]})/G(TC), call for call" for i in C.HASH_IMPLS) +
           " | M vs V (fixed, grow) | grow vs fixed (V, M) | map vs lookup |",
           "| --- | --- | " + " | ".join("---" for _ in C.HASH_IMPLS) + " | --- | --- | --- |"]
    for job in jobs:
        g = gcd_check(d, job)
        if not g:
            md.append(f"| {job} | {NA} |" + " |" * (len(C.HASH_IMPLS) + 3))
            continue
        cells = []
        for i in C.HASH_IMPLS:
            x = g.get(i) or {}
            cells.append(f"{fmt(x.get('G_ratio'), 6)} ({x.get('gcd_work_check')}), {x.get('call_for_call')}")
            rows_csv.append({"job": job, "pairing": f"{C.IMPL_ROW[i]} vs TC", "kind": "against TC",
                             "measure": "gcd work (check)", "verdict": x.get("gcd_work_check", NA),
                             "ratio": x.get("G_ratio")})
        md.append(f"| {job} | {fmt(g['G_per_op'], 2)} | " + " | ".join(cells) +
                  f" | {g.get('M against V (fixed)')}, {g.get('M against V (grow)')} | "
                  f"{g.get('grow against fixed (V)')}, {g.get('grow against fixed (M)')} | {g.get('map against lookup')} |")
    md += ["", "### Strata (counts of verdicts; no measure combined, no stratum scored)", "",
           "The strata are the sharing levels of A and B (rule-m1 item 11): " +
           ", ".join(f"{s} (rho = {C.STRATA[s]})" for s in C.STRATA_ORDER) + ".", "",
           "| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe "
           "flags |", "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for s in C.STRATA_ORDER:
        for row, ref, kind in C.PAIRINGS:
            js = [j for j in jobs if C.job_rho(j) == C.STRATA[s] and (row, ref, j) in per]
            for meas in ("creation", "operation", "B_used", "B_heap"):
                if meas in C.TIME_MEASURES:
                    xs = [per[(row, ref, j)][0][meas] for j in js]
                    ratios = [x.get("ratio") for x in xs]
                    sev = 0
                else:
                    xs = [per[(row, ref, j)][1 if meas == "B_used" else 2] for j in js]
                    ratios = [x["end"]["ratio"] for x in xs]
                    sev = sum(1 for x in xs if x["severe"])
                vs = [x["verdict"] for x in xs]
                md.append(f"| {s} | {row} vs {ref} | {meas} | {vs.count('win')} | {vs.count('tie')} | "
                          f"{vs.count('loss')} | {vs.count('over cap')} | {vs.count(NA)} | {vs.count('reported')} | "
                          f"{fmt(median(ratios))} | {sev} |")
    md += ["", "### Size x sharing grid (ratios T(reference)/T(row); presented, not judged)", ""]
    for row, ref, kind in C.PAIRINGS:
        for meas in C.TIME_MEASURES:
            md += [f"{row} against {ref}, {meas}:", "",
                   "| n | " + " | ".join(f"rho = {r}" for r in C.RHOS) + " |",
                   "| --- | " + " | ".join("---" for _ in C.RHOS) + " |"]
            for si, n in enumerate(C.TOY_SIZES if st.get("toy") else C.SIZES):
                cells = []
                for ri in range(len(C.RHOS)):
                    j = 4 * si + ri + 1
                    x = per.get((row, ref, j), ({},))[0].get(meas) if (row, ref, j) in per else None
                    cells.append(fmt(x.get("ratio")) if x and x.get("ratio") is not None else
                                 (x["verdict"] if x else NA))
                md.append(f"| {n} | " + " | ".join(cells) + " |")
            md.append("")
    return md


def peak_section(d, st, jobs):
    """rule-m1 item 7: the growth peak, accounted and heap, per cell, for every grow row; and as a ratio in the map
    against lookup pairing, no verdict"""
    md = ["## The growth peak (grow rows; reported, no verdict)", "",
          "The most bytes per cell at any growth: accounted (every matrix's cells and table, the old table included, "
          "plus the grown table (n), the slot map (r) in Hash-V-grow-map, and tau) and heap (`mallinfo2`, read inside "
          "the growth just before the old table is freed), over 3n^2 cells; and Hash-V-grow-map over Hash-V-grow.", "",
          "| job | " + " | ".join(f"{C.IMPL_ROW[i]} accounted, heap (growths)" for i in C.GROW_IMPLS) +
          " | map / lookup: accounted, heap |", "| --- | " + " | ".join("---" for _ in C.GROW_IMPLS) + " | --- |"]
    for job in jobs:
        p = {i: growth_peak(d, job, i) for i in C.GROW_IMPLS}
        cells = [f"{fmt(x['used'], 2)}, {fmt(x['heap'], 2)} ({x['growths']})" if x else NA for x in p.values()]
        m, l = p["MHVGM"], p["MHVG"]
        ratio = f"{fmt(m['used'] / l['used'])}, {fmt(m['heap'] / l['heap'])}" if m and l and l["used"] and l["heap"] \
            else NA
        md.append(f"| {job_label(st, job)} | " + " | ".join(cells) + f" | {ratio} |")
    md.append("")
    return md


def tau_section(d, st, jobs):
    md = ["## tau's build time (M rows; reported, no verdict)", "",
          "The median over rounds of tau's build time per run, and its share of the operation part (the median of the "
          "per-round shares). tau is built, used and freed inside the operation part (spec section 8).", "",
          "| job | " + " | ".join(f"{C.IMPL_ROW[i]} tau ns (share)" for i in C.M_IMPLS) + " |",
          "| --- | " + " | ".join("---" for _ in C.M_IMPLS) + " |"]
    for job in jobs:
        cells = []
        for i in C.M_IMPLS:
            t, sh = tau_time(d, job, C.IMPL_ROW[i])
            cells.append(f"{fmt(t, 0)} ({fmt(sh, 4)})" if t is not None else NA)
        md.append(f"| {job_label(st, job)} | " + " | ".join(cells) + " |")
    md.append("")
    return md


def storage_section(d, st, jobs):
    """rule-m1 item 13: per job and store, after creation and at the end, from the counting runs (exact; the gates
    checked every figure against the model). V and M have the same stores (spec section 10 item 4) and the map row
    lookup's but its rewrite probes (item 6), so one hash row per sizing is shown, with the map row's rewrite probes."""
    md = ["## Stores, per job (rule-m1 item 13)", "",
          "From the counting runs. Fixed: Hash-V-fixed (Hash-M-fixed has the same stores). Grow: Hash-V-grow "
          "(Hash-M-grow the same; Hash-V-grow-map the same but no rewrite probe). W counts 0 and 1. Probes per store "
          "over every whole-number store (0 and 1 found with no probe).", "",
          "| job | sizing | store | when | W | T | fill | w | w_h | stores (found / new) | probes per store | longest "
          "| widenings (slots copied) | growths | reinserted (probes) | fields rewritten | rewrite probes |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        for sizing, impl in (("fixed", "MHVF"), ("grow", "MHVG")):
            c = d.count(job, impl)
            if not c or "stor" not in c:
                continue
            for r in C.ROLES:
                for when in ("after_creation", "end"):
                    s = c["stor"].get(when, {}).get(r)
                    if not s:
                        continue
                    n = s["found"] + s["new"]
                    md.append(f"| {job} | {sizing} | {r} | {when.replace('_', ' ')} | {s['W']} | {s['T']} | "
                              f"{s['W'] / s['T']:.1%} | {s['w']} | {s['wh']} | {s['found']} / {s['new']} | "
                              f"{fmt(s['probes'] / n if n else None, 2)} | {s['longest']} | {s['widenings']} "
                              f"({s['copied']}) | {s['growths']} | {s['reinserted']} ({s['rprobes']}) | "
                              f"{s['rewritten']} | {s['wprobes']} |")
    md += ["", "### tau's finds (M rows; from the counting runs)", "",
           "| job | row | found | none | probes | position bytes |", "| --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        for impl in C.M_IMPLS:
            c = d.count(job, impl)
            if not c or "opc" not in c:
                continue
            o = c["opc"].get("end", {})
            tb = K.parse_mem(c["mem"]).get("end", {}).get("tau") if c.get("mem") else None
            md.append(f"| {job} | {C.IMPL_ROW[impl]} | {o.get('tau_found')} | {o.get('tau_none')} | "
                      f"{o.get('tau_probes')} | {tb} |")
    md.append("")
    return md


def reported_sections(d, st, jobs):
    op = st.get("op")
    md = ["## Also reported (rule-m1 item 13)", "", "### Realized sharing", "",
          "s: distinct values per matrix of A and B, as drawn; W per store (0 and 1 included) at the end, from the "
          "model (the gates checked it).", "",
          "| job | n | rho | s | distinct A | distinct B | distinct C | W_A | W_B | W_C |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        g = st.get("gen", {}).get(str(job))
        if not g:
            continue
        r = g.get("realized", {})
        W = r.get("W", {}).get("fixed", {})
        md.append(f"| {job} | {g['n']} | {g['rho']} | {g['s']} | {r.get('distinct_A')} | {r.get('distinct_B')} | "
                  f"{r.get('distinct_C')} | {W.get('A')} | {W.get('B')} | {W.get('C')} |")
    md += ["", "### Cases per operation (every row takes the same; spec section 10 items 3-4)", ""]
    for job in jobs:
        g = st.get("gen", {}).get(str(job))
        if not g or not g.get("ncase"):
            continue
        nc = g["ncase"]
        parts = []
        for alg in ("add", "mul"):
            tot = sum(v for k, v in nc.items() if k.startswith(alg + "_"))
            if tot:
                parts.append(f"{alg} ({tot}): " + ", ".join(f"{k[len(alg) + 1:]} {nc[k] / tot:.3f}"
                                                            for k in C.CASES_OF[alg] if nc.get(k)))
        md.append(f"- job {job}: " + "; ".join(parts))
    md += ["", "### Cross tests per operation (counting runs)", "",
           "Four cross tests per operation on two cells (spec section 6); V reads the values (cross reads), M compares "
           "handles through tau and reads only what a case needs (K-Q3 reads).", "",
           "| job | operations on cells | cross tests | matched | V cross reads | V K-Q3 reads | M K-Q3 reads |",
           "| --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        v, m = d.count(job, "MHVF"), d.count(job, "MHMF")
        if not v or not m or "opc" not in v:
            continue
        o = v["opc"].get("end", {})
        n = st.get("gen", {}).get(str(job), {}).get("n")
        cells = n * n if op == "add" else n ** 3 if n else None
        rv = sum(x for (ph, nm), x in K.parse_counts(v["counts"]).items() if nm == "reads")
        rm = sum(x for (ph, nm), x in K.parse_counts(m["counts"]).items() if nm == "reads")
        md.append(f"| {job} | {cells} | {o.get('cross')} | {o.get('cross_match')} | {o.get('cross_reads')} | {rv} | "
                  f"{rm} |")
    md += ["", "### Bytes per cell (accounted, at the end; tau included)", "",
           "| job | " + " | ".join(C.IMPL_ROW[i] for i in C.IMPLS) + " |",
           "| --- | " + " | ".join("---" for _ in C.IMPLS) + " |"]
    for job in jobs:
        md.append(f"| {job} | " + " | ".join(fmt(bytes_used(d, job, i, "end"), 2) for i in C.IMPLS) + " |")
    md += ["", "The gates checked every counter and store figure against the model (spec section 10 item 2), the TC "
               "gate included: " + ", ".join(f"{k} {'pass' if 'fail' not in v else 'FAIL'}"
                                             for k, v in st.get("gates", {}).items()), ""]
    md += ["### The idle and disturbance checks", "",
           "As round 3 (rule-m1 item 5, carried over in full): processes are told apart by process, not by thread; the "
           "fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's "
           "own ticks are reported in total and per launch; the one sanity limit is more than "
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
           "### Round 3, for reference only", "",
           "Round 3's results (Design 3 on streams of single operations) are in "
           "[`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They "
           "enter no measure here.", ""]
    if op == "mul":
        md += ["The addition run's report, for reference only: [`../results-m1-add/results.md`]"
               "(../results-m1-add/results.md). Nothing measured there enters a measure here.", ""]
    return md


def cpu_time_section(st):
    """rule-m1 item 4 (as rule-r3): each timed rep's own process CPU time beside its time, reported only"""
    recs = [r for r in st.get("records", []) if r.get("role") == "round" and r.get("cpu_share") is not None]
    if not recs:
        return []
    md = ["", "### The measured process's own CPU time (reported only; enters no verdict)", "",
          "CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the "
          "same whole rep, for the rep kept in each round.", "",
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
