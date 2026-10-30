"""Analysis of a round-2 run (research/rule-r2.md items 7-13, Run plan step 8): results.md and CSVs from
OUTDIR/state.json.

    python3 r2_analyze.py OUTDIR

Per job and measure only, Q32 against TC: time (T(TC)/T(Q32), paired bootstrap interval, band h from Q32 against
Q32-null), bytes per rational accounted (B_used) and on the heap (B_heap), each with its verdict and magnitude; gcd work
as a check, not a verdict (G(Q32)/G(TC), exactly 1, with G per operation); the profile, reported and not judged (the
slowdown of every invocation, batch times per operation, each phase's share raw and corrected and its estimated
nanoseconds per operation, per job phase and per case, and Q32 - TC per phase, indicative only); the strata, with no
overall winner; and the reported items of item 13. A run that stopped (the pilot, a gate, the hard stop) is reported
with what it has and no verdict. Never writes into the official data (round 1's, and the stopped round-2 run's).
"""
import csv
import io
import json
import statistics
import sys
from pathlib import Path

import numpy as np

import r2_checks as K
import r2_common as C

NA = "n/a"
LIMITATIONS = [
    "**Textbook only.** Neither row is optimized (spec v0.2 §0, decision 5). The verdicts compare two plain baselines; "
    "they do not compare the designs at their best.",
    "**Every result is kept**, in both rows, as in round 1. Chains do not update in place.",
    "**64-bit arithmetic only.** At this width a gcd is cheap, so time differences come mostly from structure. The gcd "
    "work is equal by construction, and is reported to make that visible.",
    "**The profile perturbs what it measures** (spec v0.2 §13.4 items 8 and 9). Its fences stop the overlap between "
    "phases, so its shares and its per-phase differences between the rows are indicative only.",
    "**Jobs 5-7 are not run.** Round 2 says nothing about the low-reuse jobs beyond Q32's capacity.",
]


# ------------------------------------------------------------------------------------------------- statistics
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
    """bytes: Q32 over TC; at most 0.90 Q32 wins, at least 1.10 it loses (rule-r2 item 9)"""
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
        for k, c in st.get("calibration", {}).items():
            job, row = k.split(":")
            if c.get("overcap"):
                self.capped[(int(job), row)] = c
        self.counts = {tuple(reversed(k.split(":"))): v for k, v in st.get("counts", {}).items()}
        self.mems = {tuple(reversed(k.split(":"))): v for k, v in st.get("mems", {}).items()}
        self.stopped = set(st.get("stopped_impls", []))
        self.gen = st.get("gen", {})

    def count(self, job, impl):
        return self.counts.get((str(job), impl))

    def mem(self, job, impl):
        return self.mems.get((str(job), impl))

    def paired(self, job, x, y, field="total"):
        a, b = self.rounds.get((job, x), {}), self.rounds.get((job, y), {})
        common = sorted(set(a) & set(b))
        return [a[k][field] for k in common], [b[k][field] for k in common]


def null_h(d, job):
    a, b = d.paired(job, "Q32-null", "Q32")
    if len(a) < 2:
        return None, None
    r, lo, hi = boot_ratio(a, b)
    return max(C.BAND_MIN, 1 - lo, hi - 1, 0.0), {"ratio": r, "lo": lo, "hi": hi}


def time_measure(d, job):
    """item 7: T(TC)/T(Q32); above 1 Q32 is faster"""
    if (job, "Q32") in d.capped or (job, "TC") in d.capped:
        return {"verdict": "over cap", "ratio": None}
    tc, q = d.paired(job, "TC", "Q32")
    if len(tc) < 2:
        return {"verdict": NA, "ratio": None, "why": "too few rounds"}
    h, null = null_h(d, job)
    r, lo, hi = boot_ratio(tc, q)
    out = {"ratio": r, "lo": lo, "hi": hi, "h": h, "null": null, "rounds": len(tc),
           "verdict": time_verdict(lo, hi, h) if h is not None else NA}
    for part in ("entry", "ops", "read"):
        a, b = d.paired(job, "TC", "Q32", part)
        if len(a) >= 2 and min(b) > 0:
            pr, plo, phi = boot_ratio(a, b)
            out[part] = {"ratio": pr, "lo": plo, "hi": phi}
    return out


def bytes_used(d, job, impl, when):
    c = d.count(job, impl)
    if not c or "mem" not in c:
        return None
    m = K.parse_mem(c["mem"]).get(when)
    if not m or not m.get("N"):
        return None
    return sum(u for u, _ in m["items"].values()) / m["N"]


def bytes_heap(d, job, impl, when):
    m = d.mem(job, impl)
    if not m or "memrun" not in m or when not in m["memrun"]:
        return None
    r = m["memrun"][when]
    return r["heap"] / r["stored"]


def memory_measure(d, job, kind):
    f = bytes_used if kind == "B_used" else bytes_heap
    out = {}
    for when in ("after_entry", "end"):
        q, t = f(d, job, "Q32", when), f(d, job, "TC", when)
        out[when] = {"Q32": q, "TC": t, "ratio": q / t if q is not None and t else None}
    r = out["end"]["ratio"]
    out["verdict"] = fixed_verdict(r) if r is not None else NA
    out["severe"] = r is not None and r >= C.SEVERE
    return out


def gcd_check(d, job):
    chk = d.st.get("call_checks", {}).get(str(job))
    if not chk:
        return None
    gen = d.gen.get(str(job), {})
    n = gen.get("n")
    return dict(chk, G_per_op=chk["G_Q32"] / n if n else None)


# ------------------------------------------------------------------------------------------------- the profile
def profile_figures(rec, phase_counts):
    """One profiling invocation (its kept rep): per part (entry, operations, read-out) the batch time per operation,
    M per operation, and each phase's share (raw and corrected) and estimated ns per operation; per job phase and per
    case, the shares and the sampled calls' mean corrected time. Spec v0.2 §13.4 items 5-7."""
    p = rec["profile"]
    hz, cost, r = rec["tsc_hz"], rec["probe_cost"], rec["r"]
    ns = 1e9 / hz
    cyc, segs = {}, {}
    for jp, k, ph, c, s in p["cyc"]:
        cyc[(jp, k, ph)] = c
        segs[(jp, k, ph)] = s
    calls = {(jp, k): (n, o) for jp, k, n, o in p["calls"]}
    m = {jp: (c, n) for jp, c, n in p["m"]}
    names = C.PF_PHASES[:6] + ("U",)                         # F, C, A.gcd, A.rest, I, S, U (M apart)

    def phases(sel):
        """raw and corrected cycles per phase over the sampled calls of the (jp, case) keys in sel"""
        raw, cor = {n: 0.0 for n in names}, {n: 0.0 for n in names}
        ncalls = 0
        for (jp, k) in sel:
            n, outer = calls[(jp, k)]
            ncalls += n
            inner = 0
            for ph in range(7):
                c = cyc.get((jp, k, ph), 0)
                inner += c
                if ph == 6:
                    continue                                   # M: reported in full from every growth
                raw[names[ph]] += c
                cor[names[ph]] += max(0.0, c - segs.get((jp, k, ph), 0) * cost)
            u = outer - inner
            raw["U"] += max(0, u)
            cor["U"] += max(0.0, u - 2 * n * cost)               # the outer pair's two intervals
        return raw, cor, ncalls

    def shares(raw, cor):
        tr, tc = sum(raw.values()), sum(cor.values())
        return ({n: raw[n] / tr if tr else None for n in names}, {n: cor[n] / tc if tc else None for n in names})

    parts = {"entry": ([0], phase_counts.get("entry", 0), rec["entry"]),
             "operations": ([1, 2, 3, 4], sum(phase_counts.get(x, 0) for x in ("add", "sub", "mul", "div")), rec["ops"]),
             "read-out": ([5], sum(phase_counts.values()), rec["read"])}
    out = {"parts": {}, "job_phases": {}, "cases": {}, "probe_cost": cost, "K": rec.get("K"),
           "sampled": p["sampled"], "slowdown": rec.get("slowdown")}
    for part, (jps, nops, batch) in parts.items():
        sel = [key for key in calls if key[0] in jps]
        raw, cor, ncalls = phases(sel)
        sr, sc = shares(raw, cor)
        per_op = batch / nops if nops else None
        # the driver sums the growth cycles over the rep's r runs (bench_r2.c, prof_add); the batch figures are
        # already per run (r2_runner.quick_parse), so M is divided by r as well
        m_ns = sum(m.get(jp, (0, 0))[0] for jp in jps) * ns / (nops * r) if nops else None
        est = {n: (sc[n] * (per_op - m_ns) if sc[n] is not None and per_op is not None else None) for n in names}
        est["M"] = m_ns
        out["parts"][part] = {"batch_ns_per_op": per_op, "M_ns_per_op": m_ns, "share_raw": sr, "share_corrected": sc,
                              "est_ns_per_op": est, "sampled_calls": ncalls}
    for jp in range(6):
        sel = [key for key in calls if key[0] == jp]
        if not sel:
            continue
        raw, cor, ncalls = phases(sel)
        sr, sc = shares(raw, cor)
        out["job_phases"][C.JOB_PHASES[jp]] = {"share_raw": sr, "share_corrected": sc, "sampled_calls": ncalls}
    for key in calls:
        raw, cor, ncalls = phases([key])
        if ncalls >= C.PROF_CASE_MIN_CALLS:
            sr, sc = shares(raw, cor)
            out["cases"][f"{C.JOB_PHASES[key[0]]}/{key[1]}"] = {
                "share_corrected": sc, "sampled_calls": ncalls,
                "mean_ns": sum(cor.values()) * ns / ncalls}
    return out


def profile_summary(st, job, row):
    """the median of each figure over the row's profiling invocations on the job (rule-r2 item 10)"""
    recs = [r for r in st.get("profiles", []) if r["job"] == job and r["row"] == row]
    pc = st.get("gen", {}).get(str(job), {}).get("phase_counts", {})
    figs = [profile_figures(r, pc) for r in recs if "profile" in r]
    slow = [{"slowdown": r.get("slowdown"), "within": r.get("within_limit"), "withheld": r.get("withheld")}
            for r in recs]
    if not figs:
        return {"slowdowns": slow, "figures": None}
    med = {"parts": {}}
    for part in figs[0]["parts"]:
        P = [f["parts"][part] for f in figs]
        med["parts"][part] = {
            "batch_ns_per_op": median([x["batch_ns_per_op"] for x in P]),
            "M_ns_per_op": median([x["M_ns_per_op"] for x in P]),
            "share_raw": {n: median([x["share_raw"][n] for x in P]) for n in P[0]["share_raw"]},
            "share_corrected": {n: median([x["share_corrected"][n] for x in P]) for n in P[0]["share_corrected"]},
            "est_ns_per_op": {n: median([x["est_ns_per_op"][n] for x in P]) for n in P[0]["est_ns_per_op"]},
            "sampled_calls": median([x["sampled_calls"] for x in P])}
    med["job_phases"] = {}
    for jp in figs[0]["job_phases"]:
        P = [f["job_phases"][jp] for f in figs if jp in f["job_phases"]]
        med["job_phases"][jp] = {"share_corrected": {n: median([x["share_corrected"][n] for x in P])
                                                     for n in P[0]["share_corrected"]},
                                 "sampled_calls": median([x["sampled_calls"] for x in P])}
    med["cases"] = {}
    for k in figs[0]["cases"]:
        P = [f["cases"][k] for f in figs if k in f["cases"]]
        med["cases"][k] = {"share_corrected": {n: median([x["share_corrected"][n] for x in P])
                                               for n in P[0]["share_corrected"]},
                           "mean_ns": median([x["mean_ns"] for x in P]),
                           "sampled_calls": median([x["sampled_calls"] for x in P])}
    med["probe_cost"] = median([f["probe_cost"] for f in figs])
    med["K"] = figs[0]["K"]
    med["sampled"] = median([f["sampled"] for f in figs])
    return {"slowdowns": slow, "figures": med, "invocations": len(figs)}


# ------------------------------------------------------------------------------------------------- report
def report(outdir):
    outdir = Path(outdir)
    if C.protected(outdir):
        raise SystemExit(f"{outdir} holds official data (round 1's, or the stopped round-2 run's): no report is written there")
    st = json.loads((outdir / "state.json").read_text())
    d = Data(st)
    head = (outdir / "results.md").read_text() if (outdir / "results.md").exists() else ""
    cut = head.find("<!-- rule ends -->")
    head = head[:cut + len("<!-- rule ends -->")] + "\n" if cut >= 0 else head
    md = [head]
    md += run_section(st)
    md += pilot_section(st)
    finished = bool(st.get("finished")) and not st.get("run_stopped") and not st.get("hard_stopped")
    jobs = [j for j in st.get("jobs", C.RUN_JOBS)]
    rows_csv = []
    if finished:
        md += verdict_sections(d, st, jobs, rows_csv)
        md += profile_section(st, jobs)
    else:
        md += ["## Verdicts", "", "**No verdict is reported: the run did not finish** (" +
               (f"stopped at the hard stop in {st['hard_stopped']['step']}" if st.get("hard_stopped") else
                f"stopped in {st['run_stopped']['step']}: {st['run_stopped']['reason']}" if st.get("run_stopped") else
                "unfinished") + ").", ""]
    md += reported_sections(d, st, jobs)
    md += ["## Limitations", ""] + [f"- {x}" for x in LIMITATIONS] + [""]
    (outdir / "results.md").write_text("\n".join(md) + "\n")
    write_csv(outdir / "verdicts.csv", rows_csv)
    write_csv(outdir / "rounds.csv", [{k: r.get(k) for k in ("job", "row", "round", "pos", "r", "reps", "total", "entry",
                                                              "ops", "read", "overcap", "cpu_ns", "span_ns",
                                                              "cpu_share")}
                                      for r in st.get("records", []) if r.get("role") == "round"])
    write_csv(outdir / "counters.csv", counter_rows(st))


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


def counter_rows(st):
    out = []
    for key, c in sorted(st.get("counts", {}).items()):
        impl, job = key.split(":")
        for l in c.get("counts", []):
            t = l.split()
            out.append({"job": job, "row": impl, "kind": t[0], "phase": t[1], "part": t[2], "name": t[3],
                        "value": t[4]})
    return out


def run_section(st):
    md = ["## The run", ""]
    status = ("finished " + st["finished"]) if st.get("finished") and not st.get("run_stopped") else \
        ("**stopped at the hard stop**" if st.get("hard_stopped") else
         ("**stopped**: " + st["run_stopped"]["reason"] if st.get("run_stopped") else "unfinished"))
    md += [f"- Started {st.get('started')}; {status}.",
           f"- Run time {st.get('elapsed_saved', 0) / 60:.1f} minutes (hard stop {C.HARD_STOP_S // 60}).",
           f"- Rule sha256 `{st.get('rule_sha256')}`; the official data after the run: "
           f"{st.get('archive_after', NA)}.",
           f"- Host processes (the runner's ancestors): " + ", ".join(f"{n}[{p}]" for p, n in st.get("hosts", {}).items())]
    for s in st.get("code_sessions", []):
        md.append(f"- Code at {s['time']}: " + ", ".join(f"{k} `{v[:12]}`" for k, v in s["code"].items()))
    ut = st.get("unit_tests", {})
    if ut:
        md.append("- Unit tests: " + "; ".join(f"{k}: " + (v["out"].strip().splitlines() or ["?"])[-1]
                                               for k, v in ut.items()))
    over = st.get("over_capacity", {})
    if over:
        md.append("- **Over capacity for Q32** (not run): " + ", ".join(f"job {j} ({C.JOB_NAMES[int(j)]}, W = {w})"
                                                                       for j, w in over.items()))
    for b in st.get("bugs", []) + st.get("harness_bugs", []):
        md.append(f"- **Bug**: {b}")
    md.append("")
    return md


def pilot_section(st):
    p = st.get("pilot")
    if not p:
        return []
    md = ["## The pilot (discarded, except what it records for the run)", "",
          f"- Jobs {p['jobs']}, profiling run on job {p['prof_job']}; P = {p['P_s']:.1f} s, P_prof = "
          f"{p['P_prof_s']:.1f} s; projection {p['projected_s'] / 60:.1f} min (limit {p['limit_s'] / 60:g}).",
          f"- Host rates R_h (ticks per second): {p['host_rates']}.",
          f"- Profiling slowdown: " + ", ".join(f"{r} {s['slowdown']:.3f}" for r, s in p["slowdown"].items()) + "."]
    if st.get("pilot_stopped"):
        md.append(f"- **The pilot stopped the run**: {'; '.join(st['pilot_stopped'])}.")
    md.append("")
    return md


def verdict_sections(d, st, jobs, rows_csv):
    md = ["## Verdicts, per job and measure (Q32 against TC; no overall winner)", "",
          "Time: T(TC)/T(Q32), above 1 Q32 is faster; bytes: Q32 over TC, below 1 Q32 is smaller. Gcd work is a "
          "check, not a verdict.", "",
          "| job | time: verdict, ratio [lo, hi], h | entry / ops / read-out ratios | B_used (end): verdict, ratio "
          "(after entry) | B_heap (end): verdict, ratio (after entry) | gcd work check |",
          "| --- | --- | --- | --- | --- | --- |"]
    per = {}
    for job in jobs:
        t = time_measure(d, job)
        bu, bh, g = memory_measure(d, job, "B_used"), memory_measure(d, job, "B_heap"), gcd_check(d, job)
        per[job] = (t, bu, bh)
        tt = (f"{t['verdict']}, {fmt(t['ratio'])} [{fmt(t.get('lo'))}, {fmt(t.get('hi'))}], h {fmt(t.get('h'))}"
              if t.get("ratio") is not None else t["verdict"])
        parts = " / ".join(fmt(t.get(p, {}).get("ratio")) for p in ("entry", "ops", "read"))
        bus = f"{bu['verdict']}, {fmt(bu['end']['ratio'])} ({fmt(bu['after_entry']['ratio'])})" + \
            (" **severe memory loss**" if bu["severe"] else "")
        bhs = f"{bh['verdict']}, {fmt(bh['end']['ratio'])} ({fmt(bh['after_entry']['ratio'])})" + \
            (" **severe memory loss**" if bh["severe"] else "")
        gs = (f"{g['gcd_work_check']} (G(Q32)/G(TC) = {fmt(g['G_ratio'], 6)}, G per operation {fmt(g['G_per_op'], 2)}; "
              f"call for call: {g['call_for_call']})") if g else NA
        md.append(f"| {job} {C.JOB_NAMES[job]} | {tt} | {parts} | {bus} | {bhs} | {gs} |")
        rows_csv.append({"job": job, "measure": "time", "verdict": t["verdict"], "ratio": t.get("ratio"),
                         "lo": t.get("lo"), "hi": t.get("hi"), "h": t.get("h")})
        rows_csv.append({"job": job, "measure": "B_used", "verdict": bu["verdict"], "ratio": bu["end"]["ratio"],
                         "after_entry": bu["after_entry"]["ratio"], "severe": bu["severe"]})
        rows_csv.append({"job": job, "measure": "B_heap", "verdict": bh["verdict"], "ratio": bh["end"]["ratio"],
                         "after_entry": bh["after_entry"]["ratio"], "severe": bh["severe"]})
        rows_csv.append({"job": job, "measure": "gcd work (check)", "verdict": g["gcd_work_check"] if g else NA,
                         "ratio": g["G_ratio"] if g else None})
    md += ["", "### Strata (counts of verdicts; no measure combined, no stratum scored)", "",
           "| stratum | measure | wins | ties | losses | over cap | n/a | median ratio | severe flags |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for s in C.STRATA_ORDER:
        js = [j for j in C.STRATA[s] if j in per]
        for mi, meas in enumerate(("time", "B_used", "B_heap")):
            vs = [per[j][mi]["verdict"] for j in js]
            ratios = [per[j][0]["ratio"] if mi == 0 else per[j][mi]["end"]["ratio"] for j in js]
            sev = sum(1 for j in js if mi and per[j][mi]["severe"])
            md.append(f"| {s} | {meas} | {vs.count('win')} | {vs.count('tie')} | {vs.count('loss')} | "
                      f"{vs.count('over cap')} | {vs.count(NA)} | {fmt(median(ratios))} | {sev} |")
    md.append("")
    return md


def profile_section(st, jobs):
    md = ["## The profile (reported, not judged)", "",
          "Batch times per operation are close to the timing build's; the phase figures are not. **Q32 - TC per "
          "phase is indicative only**: the fences of a sampled call stop one phase's latency from overlapping the next "
          "phase's work (spec v0.2 §13.4 item 9).", ""]
    names = C.PF_PHASES[:6] + ("U", "M")
    for job in jobs:
        sums = {row: profile_summary(st, job, row) for row in C.PROF_ROWS}
        md += [f"### Job {job} ({C.JOB_NAMES[job]})", ""]
        for row, s in sums.items():
            sl = ", ".join(("over the slowdown limit " if x.get("withheld") else "") + fmt(x["slowdown"])
                           for x in s["slowdowns"])
            f = s["figures"]
            md.append(f"- {row}: slowdowns {sl or NA}" + (f"; probe cost {f['probe_cost']:.1f} cycles, K = {f['K']}, "
                                                          f"sampled calls {f['sampled']}" if f else ""))
        if not all(s["figures"] for s in sums.values()):
            md.append("")
            continue
        md += ["", "| part | row | batch ns/op | " + " | ".join(f"{n} share (raw / corr.)" for n in names[:-1]) +
               " | " + " | ".join(f"{n} ns/op" for n in names) + " |",
               "| --- | --- | --- | " + " | ".join("---" for _ in names[:-1]) + " | " + " | ".join("---" for _ in names) +
               " |"]
        for part in ("entry", "operations", "read-out"):
            for row in C.PROF_ROWS:
                P = sums[row]["figures"]["parts"][part]
                md.append(f"| {part} | {row} | {fmt(P['batch_ns_per_op'], 1)} | " +
                          " | ".join(f"{fmt(P['share_raw'][n])} / {fmt(P['share_corrected'][n])}" for n in names[:-1])
                          + " | " + " | ".join(fmt(P["est_ns_per_op"][n], 1) for n in names) + " |")
            q, t = sums["Q32"]["figures"]["parts"][part], sums["TC"]["figures"]["parts"][part]
            md.append(f"| {part} | Q32 - TC (indicative only) | {fmt(diff(q['batch_ns_per_op'], t['batch_ns_per_op']), 1)}"
                      f" | " + " | ".join("" for _ in names[:-1]) + " | " +
                      " | ".join(fmt(diff(q["est_ns_per_op"][n], t["est_ns_per_op"][n]), 1) for n in names) + " |")
        md += ["", "Per job phase and per case (corrected shares; cases with at least "
                   f"{C.PROF_CASE_MIN_CALLS} sampled calls):", ""]
        for row in C.PROF_ROWS:
            f = sums[row]["figures"]
            for jp, v in f["job_phases"].items():
                md.append(f"- {row} {jp} ({fmt(v['sampled_calls'])} sampled): " +
                          ", ".join(f"{n} {fmt(v['share_corrected'][n])}" for n in names[:-1]))
            for k, v in sorted(f["cases"].items()):
                md.append(f"  - {row} {k} ({fmt(v['sampled_calls'])} sampled, mean {fmt(v['mean_ns'], 1)} ns): " +
                          ", ".join(f"{n} {fmt(v['share_corrected'][n])}" for n in names[:-1]))
        md.append("")
    return md


def diff(a, b):
    return None if a is None or b is None else a - b


def reported_sections(d, st, jobs):
    md = ["## Also reported (rule-r2 item 13)", "", "### Realized reuse", "",
          "| job | N after entry | W | W/N | 2N/W | N at the end | W | W/N | 2N/W | result operands |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        r = load_reuse(st, job)
        if not r:
            continue
        a, e = r["after_entry"], r["end"]
        md.append(f"| {job} | {a['N']} | {a['W']} | {a['W_per_N']:.3f} | {a['places_per_whole']:.2f} | {e['N']} | "
                  f"{e['W']} | {e['W_per_N']:.3f} | {e['places_per_whole']:.2f} | "
                  f"{fmt(r['operands']['result_operand_share'])} |")
    md += ["", "Case shares per algorithm:", ""]
    for job in jobs:
        r = load_reuse(st, job)
        if r:
            md.append(f"- job {job}: " + "; ".join(f"{ph}: " + ", ".join(f"{k} {v:.3f}" for k, v in sh.items() if v)
                                                    for ph, sh in r["case_share"].items()))
    md += ["", "### Counts per operation (counting build; equal between the rows except Q32's own)", "",
           "| job | row | gcd | gcd work | mul | add | exact div | lookup searches | probes | found | appended | reads |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        n = st.get("gen", {}).get(str(job), {}).get("n")
        for impl in C.IMPLS:
            c = d.count(job, impl)
            if not c or "counts" not in c or not n:
                continue
            tot = {}
            for (ph, nm), v in K.parse_counts(c["counts"]).items():
                tot[nm] = tot.get(nm, 0) + v
            md.append(f"| {job} | {impl} | " + " | ".join(f"{tot.get(x, 0) / n:.3f}" for x in
                      ("gcd", "gcd_work", "mul", "add", "div_exact", "lookup_search", "lookup_probe", "intern_found",
                       "intern_appended", "reads")) + " |")
    md += ["", "### The known-answer skips (counting build, with the generator's prediction from the gates)", "",
           "| job | skip | operations | gcds | gcd work | divisions | multiplications | additions |",
           "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        c = d.count(job, "Q32")
        if not c or "counts" not in c:
            continue
        tot = {}
        for (ph, nm), v in K.parse_counts(c["counts"]).items():
            tot[nm] = tot.get(nm, 0) + v
        base = {f: tot.get(f, 0) for f in ("gcd", "gcd_work", "div_exact", "mul", "add")}
        for k in range(1, C.NK + 1):
            v = {f: tot.get(f"k{k}_{f}", 0) for f in C.KFIELDS}
            if not any(v.values()):
                continue

            def share(skip, made):
                return f"{skip} ({skip / (skip + made):.1%})" if skip and skip + made else str(skip)
            md.append(f"| {job} | K-{k} | {v['ops']} | {share(v['gcd'], base['gcd'])} | "
                      f"{share(v['gcd_work'], base['gcd_work'])} | {share(v['div'], base['div_exact'])} | "
                      f"{share(v['mul'], base['mul'])} | {share(v['add'], base['add'])} |")
        kq = sum(v for nm, v in tot.items() if nm.startswith(("kq1_", "kq2_")))
        lk = kq + tot.get("intern_found", 0) + tot.get("intern_appended", 0)
        md.append(f"| {job} | K-Q1, K-Q2 | lookups skipped {kq} ({fmt(kq / lk if lk else None)} of lookups) | "
                  f"reads made {tot.get('reads', 0)}, skipped {tot.get('reads_skipped', 0)} (K-Q3) | | | | |")
    md += ["", "The gates checked every counter above against the generator's prediction (spec v0.2 §14.1 item 2): "
               + ", ".join(f"{k} {'pass' if 'fail' not in v else 'FAIL'}" for k, v in st.get("gates", {}).items()), ""]
    md += ["### Memory against reuse (jobs sorted by W/N at the end)", "",
           "| job | W/N | B_used Q32 | B_used TC | B_heap Q32 | B_heap TC |", "| --- | --- | --- | --- | --- | --- |"]
    order = sorted((j for j in jobs if load_reuse(st, j)), key=lambda j: load_reuse(st, j)["end"]["W_per_N"])
    for job in order:
        md.append(f"| {job} | {load_reuse(st, job)['end']['W_per_N']:.3f} | {fmt(bytes_used(d, job, 'Q32', 'end'), 2)} | "
                  f"{fmt(bytes_used(d, job, 'TC', 'end'), 2)} | {fmt(bytes_heap(d, job, 'Q32', 'end'), 2)} | "
                  f"{fmt(bytes_heap(d, job, 'TC', 'end'), 2)} |")
    md += ["", "### The idle and disturbance checks", "",
           "Processes are told apart by process (thread-group id), not by thread; the fork counter counts threads too "
           "and is reported only (rule-r2 item 5, revision of Oct 29, 2026). Host ticks are given as ticks / allowance "
           "(none in the pilot) and threads before -> after. Other processes may use 5 + ceil(1 x L) ticks over a block "
           "of L seconds (the limit column). Credited: the ticks, before the block, of children that existed at its "
           "start and were reaped during it, taken off their parent's rise.", "",
           "The runner's own ticks (utime + stime) are reported in total and per launch (a launch: one timed invocation "
           "it started in the block), not judged: each rep is timed inside the measured program, and the runner is "
           "blocked in waitpid while it runs, so its ticks fall between reps. The one sanity limit is more than "
           f"{C.RUNNER_TICKS_PER_LAUNCH_MAX:g} tick per launch plus {C.RUNNER_TICKS_BLOCK_ALLOWANCE} over a block (a fixed "
           "allowance for the block's own two snapshots), a harness bug (rule-r2 item 5, "
           "second revision of Oct 29, 2026).", "",
           "| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks "
           "(reported) | forks less "
           "spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for x in st.get("disturbance_checks", []):
        hosts = ", ".join(f"{h['name']} {h['ticks']}" + (f"/{h['allowance']}" if "allowance" in h else "") +
                          (f" ({h['threads'][0]}->{h['threads'][1]} threads)" if "threads" in h else "")
                          for h in x["hosts"].values())
        unattr = x.get("forks_unattributed", x.get("unexplained_forks"))
        quiet = ", ".join(f"{q['name']}[{q['pid']}]" for q in x.get("new_quiet", [])) or "none"
        cred = ", ".join(f"{c['name']}[{p}] {c['ticks']}" for p, c in x.get("credited", {}).items()) or "none"
        md.append(f"| {x['job']} | {x['label']} {x['kind']} | {x['attempt']} | {x['seconds']} | "
                  f"{'yes: ' + '; '.join(x['reasons']) if x['disturbed'] else 'no'} | {x['runner_ticks']} | "
                  f"{x['spawns']} | {runner_per_launch(x)} | {x['forks_rise']} | {unattr} | {quiet} | {x.get('other_limit', C.DIST_OTHER_TICKS)} | "
                  f"{cred} | {hosts} |")
    md += runner_summary(st)
    md += cpu_time_section(st)
    md += ["", f"Idle checks: {len(st.get('idle_checks', []))} (" +
           f"{sum(1 for c in st.get('idle_checks', []) if not c['idle'])} busy). Host rates R_h: "
           f"{st.get('host_rates', {})} (measured: {st.get('host_rates_measured', {})}; R_h is at least 1 tick over "
           f"the longest pilot block).", "",
           "### Round 1, for reference only", "",
           "Round 1's verdicts per job are in [`../results-q64-b3b88b6/results.md`](../results-q64-b3b88b6/results.md) "
           "(Q64, official) and [`../results-q32/results.md`](../results-q32/results.md) (the Q32 run). Round 1 enters "
           "no round-2 measure.", ""]
    return md


def runner_per_launch(x):
    """the runner's ticks per launch in a block (rule-r2 item 5, second revision): reported, n/a without a launch"""
    t, n = x.get("runner_ticks"), x.get("spawns")
    return f"{t / n:.3f}" if t is not None and n else "n/a"


def runner_summary(st):
    """rule-r2 items 5 and 13 (second revision of Oct 29, 2026): the runner's ticks over every block, in total and per
    launch, and the highest block's figure against the sanity limit. Reported only."""
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


def cpu_time_section(st):
    """rule-r2 item 4 (revision of Oct 29, 2026): each timed rep's own process CPU time beside its time, reported only.
    Per job and row, over the rounds' kept reps: the median and the lowest CPU share (CPU time over the CLOCK_MONOTONIC
    span of the same whole rep), and the median CPU time per run beside the median total."""
    recs = [r for r in st.get("records", []) if r.get("role") == "round" and r.get("cpu_share") is not None]
    if not recs:
        return []
    md = ["", "### The measured process's own CPU time (reported only; enters no verdict)", "",
          "CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the "
          "same whole rep (create and destroy included), for the rep kept in each round. A share well below 1 means "
          "the process lost its core during the rep.", "",
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


_REUSE = {}


def load_reuse(st, job):
    """the realized reuse of a job, from the generator's file beside the workloads (build/r2/work)"""
    if job in _REUSE:
        return _REUSE[job]
    base = C.HERE / "build" / ("r2toy" if st.get("toy") else "r2") / "work" / f"job-{job:02d}.r2.json"
    try:
        _REUSE[job] = json.loads(base.read_text()).get("reuse")
    except OSError:
        _REUSE[job] = None
    return _REUSE[job]


if __name__ == "__main__":
    report(sys.argv[1])
