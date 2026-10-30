"""Analysis of a round-3 run (research/rule-r3.md items 7-13, Run plan step 8): results.md, CSVs and the plot of speed
against fill, from OUTDIR/state.json. From round 2's r2_analyze.py (unchanged; its statistics are imported from it).

    python3 r3_analyze.py OUTDIR

Per job, pairing and measure only, for the three pairings (Q32-index against TC, Q32-hash against TC, Q32-hash against
Q32-index): creation time and operation time, each with its own verdict (T(reference)/T(row), paired bootstrap
interval, band h = max(5%, that measure's null spread from Q32-hash against Q32-hash-null)); total and read-out time
with ratio and interval and no verdict; bytes per rational accounted (B_used: the table counted in full) and on the heap
(B_heap), each with its verdict and magnitude; gcd work as a check, not a verdict (exactly 1 for both Q32 rows);
`create`'s own time and its share of the creation part, reported; the profile, reported and not judged; the strata,
with no overall winner; and item 13: Q32-hash's fill, probes per store and longest store, Q32-index's entries examined
per search, speed against fill (a table and a plot, descriptive), and round 2's reported items. A run that stopped is
reported with what it has and no verdict. Never writes into the official data (rounds 1 and 2).
"""
import json
import sys
from pathlib import Path

import r2_analyze as A2                    # round 2's statistics and formatting, unchanged: boot_ratio, verdicts, fmt
import r3_checks as K
import r3_common as C

NA = A2.NA
boot_ratio, time_verdict, fixed_verdict, fmt, median, write_csv, diff = (
    A2.boot_ratio, A2.time_verdict, A2.fixed_verdict, A2.fmt, A2.median, A2.write_csv, A2.diff)

FIELD = {"creation": "creation", "operations": "ops", "total": "total", "read-out": "read"}
CAVEAT = ("Q32-index's own noise is not measured: the null row is a copy of Q32-hash, and on jobs where Q32-index's "
          "calibration rep lasts at least 1 s it runs 1 rep per invocation (rule-r3 item 8).")
LIMITATIONS = [
    "**Textbook only.** No row is optimized. In particular, Q32-index's search is the plain linear search, and "
    "Q32-hash's table is plain open addressing with linear probing (rule-r3, Limitations).",
    "**Fill is not controlled.** Q32-hash's fill varies across jobs only as the jobs' W does, together with every "
    "other property of the jobs. The speed-against-fill table is descriptive. A sweep at target fills is for a later "
    "round (spec v0.3 §0 item 3).",
    "**Q32-index's cost grows with W.** Its time per store is roughly proportional to the list's length, so its "
    "verdicts depend on each job's size, not only on its reuse.",
    "**Every result is kept;** chains do not update in place. Arithmetic is 64-bit only. The profile perturbs what it "
    "measures. Jobs 5-7 are not run. The disturbance check cannot see every process. All of these are as round 2.",
    "**Round 2's Q32 is retired, not superseded.** Its results measured a three-structure design and stay on record "
    "under that label.",
]


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
        return median([r[field] for r in self.rounds.get((job, row), {}).values() if r.get(field) is not None])


# ------------------------------------------------------------------------------------------------- measures
def null_h(d, job, measure):
    """rule-r3 item 8: Q32-hash against Q32-hash-null on one time measure: h = max(5%, its null spread)."""
    a, b = d.paired(job, C.NULL_ROW, C.NULL_OF, FIELD[measure])
    if len(a) < 2 or min(b) <= 0:
        return None, None
    r, lo, hi = boot_ratio(a, b)
    return max(C.BAND_MIN, 1 - lo, hi - 1, 0.0), {"ratio": r, "lo": lo, "hi": hi}


def time_measure(d, job, row, ref, measure):
    """rule-r3 items 7-9: T(ref)/T(row) on one part; above 1 the row is faster. creation and operations get a verdict
    with their own band; total and read-out are reported with ratio and interval only."""
    if (job, row) in d.capped or (job, ref) in d.capped:
        return {"verdict": "over cap", "ratio": None}
    a, b = d.paired(job, ref, row, FIELD[measure])
    if len(a) < 2 or min(b) <= 0:
        return {"verdict": NA, "ratio": None, "why": "too few rounds"}
    r, lo, hi = boot_ratio(a, b)
    out = {"ratio": r, "lo": lo, "hi": hi, "rounds": len(a)}
    if measure in C.TIME_MEASURES:
        h, null = null_h(d, job, measure)
        out.update(h=h, null=null, verdict=time_verdict(lo, hi, h) if h is not None else NA)
        if "Q32-index" in (row, ref):
            out["caveat"] = CAVEAT
    else:
        out["verdict"] = "reported"
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


def memory_measure(d, job, row, ref, kind):
    """bytes per rational, row over reference (below 1 the row is smaller); verdicts on the end figures"""
    f = bytes_used if kind == "B_used" else bytes_heap
    ri, fi = C.ROW_IMPL[row], C.ROW_IMPL[ref]
    out = {}
    for when in ("after_entry", "end"):
        q, t = f(d, job, ri, when), f(d, job, fi, when)
        out[when] = {"row": q, "ref": t, "ratio": q / t if q is not None and t else None}
    r = out["end"]["ratio"]
    out["verdict"] = fixed_verdict(r) if r is not None else NA
    out["severe"] = r is not None and r >= C.SEVERE
    return out


def gcd_check(d, job):
    chk = d.st.get("call_checks", {}).get(str(job))
    if not chk:
        return None
    n = d.gen.get(str(job), {}).get("n")
    out = {"G_TC": chk["G_TC"], "G_per_op": chk["G_TC"] / n if n else None, "result": chk.get("result")}
    for i in C.Q_IMPLS:
        out[i] = chk.get(i)
    out["Q32I_vs_Q32H"] = chk.get("Q32I_vs_Q32H")
    return out


def storage(d, job):
    """rule-r3 item 13: Q32-hash's fill, probes per store and longest store; Q32-index's entries examined per search,
    with the found/new split; after entry and at the end, from the counting runs."""
    out = {}
    for impl in C.Q_IMPLS:
        c = d.count(job, impl)
        if not c or "stor" not in c:
            continue
        for when in ("after_entry", "end"):
            s = c["stor"].get(when)
            if not s:
                continue
            n = s["found"] + s["new"]
            W = s["new"]
            e = out.setdefault(when, {})
            e.update(found=s["found"], new=s["new"], W=W, fill=W / C.SLOTS)
            if impl == "Q32I":
                e["examined_per_search"] = s["examined"] / n if n else None
                e["examined"] = s["examined"]
            else:
                e["probes_per_store"] = s["probes"] / n if n else None
                e["longest"] = s["longest"]
    return out


def calls_of(st, job):
    pc = st.get("gen", {}).get(str(job), {}).get("phase_counts", {})
    n_e = pc.get("entry", 0)
    n_o = sum(pc.get(x, 0) for x in ("add", "sub", "mul", "div"))
    return n_e, n_o


# ------------------------------------------------------------------------------------------------- the profile
def profile_figures(rec, phase_counts):
    """One profiling invocation (its kept rep), as round 2's (r2_analyze.profile_figures), with the creation part in
    place of entry; create's batch time per run is reported beside it, and is not part of M (spec v0.3 §11.4)."""
    p = rec["profile"]
    hz, cost, r = rec["tsc_hz"], rec["probe_cost"], rec["r"]
    ns = 1e9 / hz
    cyc, segs = {}, {}
    for jp, k, ph, c, s in p["cyc"]:
        cyc[(jp, k, ph)] = c
        segs[(jp, k, ph)] = s
    calls = {(jp, k): (n, o) for jp, k, n, o in p["calls"]}
    m = {jp: (c, n) for jp, c, n in p["m"]}
    names = C.PF_PHASES[:6] + ("U",)

    def phases(sel):
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
                    continue
                raw[names[ph]] += c
                cor[names[ph]] += max(0.0, c - segs.get((jp, k, ph), 0) * cost)
            u = outer - inner
            raw["U"] += max(0, u)
            cor["U"] += max(0.0, u - 2 * n * cost)
        return raw, cor, ncalls

    def shares(raw, cor):
        tr, tc = sum(raw.values()), sum(cor.values())
        return ({n: raw[n] / tr if tr else None for n in names}, {n: cor[n] / tc if tc else None for n in names})

    n_e = phase_counts.get("entry", 0)
    # the creation part's batch time holds create; the per-entry figures are taken without it
    parts = {"entry": ([0], n_e, rec["creation"] - rec.get("create", 0.0)),
             "operations": ([1, 2, 3, 4], sum(phase_counts.get(x, 0) for x in ("add", "sub", "mul", "div")), rec["ops"]),
             "read-out": ([5], sum(phase_counts.values()), rec["read"])}
    out = {"parts": {}, "job_phases": {}, "cases": {}, "probe_cost": cost, "K": rec.get("K"),
           "sampled": p["sampled"], "slowdown": rec.get("slowdown"), "create_ns": rec.get("create")}
    for part, (jps, nops, batch) in parts.items():
        sel = [key for key in calls if key[0] in jps]
        raw, cor, ncalls = phases(sel)
        sr, sc = shares(raw, cor)
        per_op = batch / nops if nops else None
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
                "share_corrected": sc, "sampled_calls": ncalls, "mean_ns": sum(cor.values()) * ns / ncalls}
    return out


def profile_summary(st, job, row):
    """the median of each figure over the row's profiling invocations on the job (rule-r3 item 10)"""
    recs = [r for r in st.get("profiles", []) if r["job"] == job and r["row"] == row]
    pc = st.get("gen", {}).get(str(job), {}).get("phase_counts", {})
    figs = [profile_figures(r, pc) for r in recs if "profile" in r]
    slow = [{"slowdown": r.get("slowdown"), "within": r.get("within_limit"), "withheld": r.get("withheld"),
             "create_ns": r.get("create")} for r in recs]
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
    med["create_ns"] = median([f["create_ns"] for f in figs])
    return {"slowdowns": slow, "figures": med, "invocations": len(figs)}


# ------------------------------------------------------------------------------------------------- report
def report(outdir):
    outdir = Path(outdir)
    if C.protected(outdir):
        raise SystemExit(f"{outdir} holds official data (rounds 1 and 2): no report is written there")
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
        md += create_section(d, st, jobs)
        md += profile_section(st, jobs)
    else:
        md += ["## Verdicts", "", "**No verdict is reported: the run did not finish** (" +
               (f"stopped at the hard stop in {st['hard_stopped']['step']}" if st.get("hard_stopped") else
                f"stopped in {st['run_stopped']['step']}: {st['run_stopped']['reason']}" if st.get("run_stopped") else
                "unfinished") + ").", ""]
    md += storage_section(d, st, jobs, outdir, finished)
    md += reported_sections(d, st, jobs)
    md += ["## Limitations", ""] + [f"- {x}" for x in LIMITATIONS] + [""]
    (outdir / "results.md").write_text("\n".join(md) + "\n")
    write_csv(outdir / "verdicts.csv", rows_csv)
    write_csv(outdir / "rounds.csv", [{k: r.get(k) for k in ("job", "row", "round", "pos", "r", "reps", "total",
                                                              "creation", "ops", "read", "create", "overcap", "cpu_ns",
                                                              "span_ns", "cpu_share")}
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
        md.append("- **Over capacity for both Q32 rows** (not run): " +
                  ", ".join(f"job {j} ({C.JOB_NAMES[int(j)]}, W = {w})" for j, w in over.items()))
    for b in st.get("bugs", []) + st.get("harness_bugs", []):
        md.append(f"- **Bug**: {b}")
    md.append("")
    return md


def pilot_section(st):
    p = st.get("pilot")
    if not p:
        return []
    md = ["## The pilot (discarded, except what it records for the run)", "",
          f"- Jobs {p['jobs']}, profiling run on job {p['prof_job']}; P = {p['P_s']:.1f} s (P_idx = "
          f"{p.get('P_idx_s', 0):.1f} s, P_rest = {p.get('P_rest_s', 0):.1f} s), P_prof = {p['P_prof_s']:.1f} s; "
          f"projection {p['projected_s'] / 60:.1f} min (limit {p['limit_s'] / 60:g}).",
          f"- Host rates R_h (ticks per second): {p['host_rates']}.",
          f"- Profiling slowdown: " + ", ".join(f"{r} {s['slowdown']:.3f}" for r, s in p["slowdown"].items()) + "."]
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


def verdict_sections(d, st, jobs, rows_csv):
    md = ["## Verdicts, per job, pairing and measure (no overall winner)", "",
          "Time: T(reference)/T(row), above 1 the row is faster; each time measure has its own band h from Q32-hash "
          "against Q32-hash-null on that measure. Total and read-out time are reported without a verdict. Bytes: row "
          "over reference, below 1 the row is smaller. Gcd work is a check, not a verdict. (c): " + CAVEAT, ""]
    per = {}
    for row, ref in C.PAIRINGS:
        md += [f"### {row} against {ref}", "",
               "| job | creation: verdict, ratio [lo, hi], h | operations: verdict, ratio [lo, hi], h | total "
               "(reported) | read-out (reported) | B_used (end): verdict, ratio (after entry) | B_heap (end): verdict, "
               "ratio (after entry) |", "| --- | --- | --- | --- | --- | --- | --- |"]
        for job in jobs:
            t = {m: time_measure(d, job, row, ref, m) for m in C.TIME_MEASURES + C.REPORTED_TIMES}
            bu, bh = memory_measure(d, job, row, ref, "B_used"), memory_measure(d, job, row, ref, "B_heap")
            per[(row, ref, job)] = (t, bu, bh)
            bus = f"{bu['verdict']}, {fmt(bu['end']['ratio'])} ({fmt(bu['after_entry']['ratio'])})" + \
                (" **severe memory loss**" if bu["severe"] else "")
            bhs = f"{bh['verdict']}, {fmt(bh['end']['ratio'])} ({fmt(bh['after_entry']['ratio'])})" + \
                (" **severe memory loss**" if bh["severe"] else "")
            md.append(f"| {job} {C.JOB_NAMES[job]} | {tcell(t['creation'])} | {tcell(t['operations'])} | "
                      f"{tcell(t['total'])} | {tcell(t['read-out'])} | {bus} | {bhs} |")
            for m in C.TIME_MEASURES + C.REPORTED_TIMES:
                x = t[m]
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "measure": m, "verdict": x["verdict"],
                                 "ratio": x.get("ratio"), "lo": x.get("lo"), "hi": x.get("hi"), "h": x.get("h"),
                                 "caveat": "Q32-index noise not measured" if x.get("caveat") else ""})
            for kind, b in (("B_used", bu), ("B_heap", bh)):
                rows_csv.append({"job": job, "pairing": f"{row} vs {ref}", "measure": kind, "verdict": b["verdict"],
                                 "ratio": b["end"]["ratio"], "after_entry": b["after_entry"]["ratio"],
                                 "severe": b["severe"]})
        md.append("")
    md += ["### Gcd work (a check, not a verdict) and the call-for-call checks", "",
           "| job | G per operation | G(Q32-index)/G(TC) | G(Q32-hash)/G(TC) | Q32-index vs TC call for call | Q32-hash vs "
           "TC call for call | Q32-index vs Q32-hash: stores call for call, counters, found/new |",
           "| --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        g = gcd_check(d, job)
        if not g:
            md.append(f"| {job} | {NA} | | | | | |")
            continue
        qi, qh, qq = g.get("Q32I") or {}, g.get("Q32H") or {}, g.get("Q32I_vs_Q32H") or {}
        md.append(f"| {job} | {fmt(g['G_per_op'], 2)} | {fmt(qi.get('G_ratio'), 6)} ({qi.get('gcd_work_check')}) | "
                  f"{fmt(qh.get('G_ratio'), 6)} ({qh.get('gcd_work_check')}) | {qi.get('call_for_call')} | "
                  f"{qh.get('call_for_call')} | {qq.get('stores_call_for_call')}, {qq.get('counters')}, "
                  f"{qq.get('found_new')} |")
        for i in C.Q_IMPLS:
            rows_csv.append({"job": job, "pairing": f"{C.IMPL_ROW[i]} vs TC", "measure": "gcd work (check)",
                             "verdict": (g.get(i) or {}).get("gcd_work_check", NA),
                             "ratio": (g.get(i) or {}).get("G_ratio")})
    md += ["", "### Strata (counts of verdicts; no measure combined, no stratum scored)", "",
           "| stratum | pairing | measure | wins | ties | losses | over cap | n/a | median ratio | severe flags |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for s in C.STRATA_ORDER:
        for row, ref in C.PAIRINGS:
            js = [j for j in C.STRATA[s] if (row, ref, j) in per]
            for meas in ("creation", "operations", "B_used", "B_heap"):
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
                          f"{vs.count('loss')} | {vs.count('over cap')} | {vs.count(NA)} | {fmt(median(ratios))} | "
                          f"{sev} |")
    md.append("")
    return md


def create_section(d, st, jobs):
    """rule-r3 item 4: create's own time, per run and as a share of the creation part; reported, no verdict"""
    md = ["## `create`, on its own (reported, not judged)", "",
          "The median over rounds of `create`'s time per run, and its share of the creation part (median of the "
          "per-round shares). `create` is timed inside the creation part (spec v0.3 §6 item 4).", "",
          "| job | " + " | ".join(f"{r} create ns (share)" for r in C.ROWS) + " |",
          "| --- | " + " | ".join("---" for _ in C.ROWS) + " |"]
    for job in jobs:
        cells = []
        for row in C.ROWS:
            recs = list(d.rounds.get((job, row), {}).values())
            cr = median([r.get("create") for r in recs])
            sh = median([r["create"] / r["creation"] for r in recs if r.get("create") is not None and r.get("creation")])
            cells.append(f"{fmt(cr, 0)} ({fmt(sh, 4)})" if cr is not None else NA)
        md.append(f"| {job} {C.JOB_NAMES[job]} | " + " | ".join(cells) + " |")
    md.append("")
    return md


def profile_section(st, jobs):
    md = ["## The profile (reported, not judged)", "",
          "Batch times per operation are close to the timing build's; the phase figures are not. **The per-phase "
          "differences between rows are indicative only**: the fences of a sampled call stop one phase's latency from "
          "overlapping the next phase's work (spec v0.2 §13.4 item 9). I is Q32-index's linear search and append, and "
          "Q32-hash's hashing and probing; M is list growth only; `create` is its own batch figure, per run (spec v0.3 "
          "§11.4). The entry part's figures leave `create` out.", ""]
    names = C.PF_PHASES[:6] + ("U", "M")
    diffs = (("Q32-index", "TC"), ("Q32-hash", "TC"), ("Q32-hash", "Q32-index"))
    for job in jobs:
        sums = {row: profile_summary(st, job, row) for row in C.PROF_ROWS}
        md += [f"### Job {job} ({C.JOB_NAMES[job]})", ""]
        for row, s in sums.items():
            sl = ", ".join(("over the slowdown limit " if x.get("withheld") else "") + fmt(x["slowdown"])
                           for x in s["slowdowns"])
            f = s["figures"]
            md.append(f"- {row}: slowdowns {sl or NA}" + (f"; probe cost {f['probe_cost']:.1f} cycles, K = {f['K']}, "
                                                          f"sampled calls {f['sampled']}; create {fmt(f['create_ns'], 0)} "
                                                          f"ns per run" if f else ""))
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
            for a, b in diffs:
                q, t = sums[a]["figures"]["parts"][part], sums[b]["figures"]["parts"][part]
                md.append(f"| {part} | {a} - {b} (indicative only) | "
                          f"{fmt(diff(q['batch_ns_per_op'], t['batch_ns_per_op']), 1)} | " +
                          " | ".join("" for _ in names[:-1]) + " | " +
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


def storage_section(d, st, jobs, outdir, finished):
    """rule-r3 item 13: storage per job, after entry and at the end; speed against fill (a table and a plot)"""
    md = ["## Storage, per job (rule-r3 item 13)", "",
          "From the counting runs (exact; the gates checked every figure against the model). Q32-hash: fill W/32,768, "
          "average probes per store, longest store. Q32-index: average entries examined per search, with the searches "
          "split into found and new (appended).", "",
          "| job | when | W | fill | stores (found / new) | probes per store | longest store | entries examined per "
          "search |", "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        s = storage(d, job)
        for when in ("after_entry", "end"):
            e = s.get(when)
            if not e:
                continue
            md.append(f"| {job} {C.JOB_NAMES[job]} | {when.replace('_', ' ')} | {e['W']} | {e['fill']:.1%} | "
                      f"{e['found']} / {e['new']} | {fmt(e.get('probes_per_store'), 2)} | {e.get('longest', NA)} | "
                      f"{fmt(e.get('examined_per_search'), 1)} |")
    md += ["", "### Speed against fill (presented, not judged)", "",
           "Jobs sorted by Q32-hash's fill at the end. Per row, the median over rounds of the creation part per entering "
           "call and of the operations part per operation (ns), and each Q32 row's ratio T(TC)/T(row) on the same part. "
           "The jobs differ in more than fill (their operations, cases and reuse differ too), so the curve is "
           "descriptive. A dedicated fill sweep is left to a later round (spec v0.3 §0 item 3).", "",
           "| job | fill (end) | probes per store | entries examined per search | " +
           " | ".join(f"{r} creation ns/call" for r in ("TC", "Q32-index", "Q32-hash")) + " | " +
           " | ".join(f"{r} ops ns/op" for r in ("TC", "Q32-index", "Q32-hash")) +
           " | Q32-index / TC creation, ops | Q32-hash / TC creation, ops |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    pts = []
    order = sorted((j for j in jobs if storage(d, j).get("end")), key=lambda j: storage(d, j)["end"]["fill"])
    for job in order:
        e = storage(d, job)["end"]
        n_e, n_o = calls_of(st, job)
        cr = {r: d.median_of(job, r, "creation") for r in ("TC", "Q32-index", "Q32-hash")}
        op = {r: d.median_of(job, r, "ops") for r in ("TC", "Q32-index", "Q32-hash")}
        cpc = {r: cr[r] / n_e if cr[r] is not None and n_e else None for r in cr}
        opo = {r: op[r] / n_o if op[r] is not None and n_o else None for r in op}

        def ratio(r, x):
            return x["TC"] / x[r] if x["TC"] is not None and x[r] else None
        md.append(f"| {job} | {e['fill']:.1%} | {fmt(e.get('probes_per_store'), 2)} | "
                  f"{fmt(e.get('examined_per_search'), 1)} | " + " | ".join(fmt(cpc[r], 1) for r in cpc) + " | " +
                  " | ".join(fmt(opo[r], 1) for r in opo) + f" | {fmt(ratio('Q32-index', cpc))}, "
                  f"{fmt(ratio('Q32-index', opo))} | {fmt(ratio('Q32-hash', cpc))}, {fmt(ratio('Q32-hash', opo))} |")
        pts.append((job, e["fill"], cpc, opo))
    png = plot_fill(pts, outdir) if finished and pts and all(p[2]["TC"] for p in pts) else None
    md += ["", f"![Speed against fill]({png})" if png else "(No plot: the run has no timed rounds for every job.)", ""]
    return md


def plot_fill(pts, outdir):
    """the plot of item 13: ns per call against Q32-hash's fill, creation and operations, each row; descriptive"""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        return None
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))
    for ax, k, title in ((axes[0], 2, "creation, ns per entering call"), (axes[1], 3, "operations, ns per operation")):
        for row, mk in (("TC", "o"), ("Q32-index", "s"), ("Q32-hash", "^")):
            xs = [p[1] for p in pts if p[k][row] is not None]
            ys = [p[k][row] for p in pts if p[k][row] is not None]
            ax.plot(xs, ys, marker=mk, label=row)
            for p in pts:
                if p[k][row] is not None and row == "Q32-hash":
                    ax.annotate(str(p[0]), (p[1], p[k][row]), textcoords="offset points", xytext=(4, 4), fontsize=8)
        ax.set_yscale("log")
        ax.set_xlabel("Q32-hash fill at the end (W / 32,768)")
        ax.set_title(title)
        ax.grid(True, alpha=0.3)
    axes[0].legend()
    fig.suptitle("Speed against fill across the jobs (descriptive; jobs differ in more than fill)")
    fig.tight_layout()
    name = "speed_vs_fill.png"
    fig.savefig(Path(outdir) / name, dpi=110)
    plt.close(fig)
    return name


def reported_sections(d, st, jobs):
    md = ["## Also reported (rule-r3 item 13, as rule-r2)", "", "### Realized reuse", "",
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
    md += ["", "### Counts per operation (counting build; equal between the rows except the Q32 rows' own)", "",
           "| job | row | gcd | gcd work | mul | add | exact div | found | new | reads | entries examined | probes |",
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
            md.append(f"| {job} | {C.IMPL_ROW[impl]} | " + " | ".join(f"{tot.get(x, 0) / n:.3f}" for x in
                      ("gcd", "gcd_work", "mul", "add", "div_exact", "intern_found", "intern_appended", "reads",
                       "examined", "probes")) + " |")
    md += ["", "### The known-answer skips (counting build, with the generator's prediction from the gates)", "",
           "| job | skip | operations | gcds | gcd work | divisions | multiplications | additions |",
           "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for job in jobs:
        c = d.count(job, "Q32I")
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
        md.append(f"| {job} | K-Q1, K-Q2 | stores skipped {kq} ({fmt(kq / lk if lk else None)} of stores) | "
                  f"reads made {tot.get('reads', 0)}, skipped {tot.get('reads_skipped', 0)} (K-Q3) | | | | |")
    md += ["", "The gates checked every counter above against the generator's prediction (spec v0.3 §12.1 item 2): "
               + ", ".join(f"{k} {'pass' if 'fail' not in v else 'FAIL'}" for k, v in st.get("gates", {}).items()), ""]
    md += ["### Memory against reuse (jobs sorted by W/N at the end)", "",
           "| job | W/N | " + " | ".join(f"B_used {C.IMPL_ROW[i]}" for i in C.IMPLS) + " | " +
           " | ".join(f"B_heap {C.IMPL_ROW[i]}" for i in C.IMPLS) + " |", "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    order = sorted((j for j in jobs if load_reuse(st, j)), key=lambda j: load_reuse(st, j)["end"]["W_per_N"])
    for job in order:
        md.append(f"| {job} | {load_reuse(st, job)['end']['W_per_N']:.3f} | " +
                  " | ".join(fmt(bytes_used(d, job, i, 'end'), 2) for i in C.IMPLS) + " | " +
                  " | ".join(fmt(bytes_heap(d, job, i, 'end'), 2) for i in C.IMPLS) + " |")
    md += ["", "### The idle and disturbance checks", "",
           "As round 2 (rule-r3 item 5, carried over in full): processes are told apart by process, not by thread; the "
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
        md.append(f"| {x['job']} | {x['label']} {x['kind']} | {x['attempt']} | {x['seconds']} | "
                  f"{'yes: ' + '; '.join(x['reasons']) if x['disturbed'] else 'no'} | {x['runner_ticks']} | "
                  f"{x['spawns']} | {A2.runner_per_launch(x)} | {x['forks_rise']} | {x.get('forks_unattributed')} | "
                  f"{quiet} | {x.get('other_limit', C.DIST_OTHER_TICKS)} | {cred} | {hosts} |")
    md += A2.runner_summary(st)
    md += cpu_time_section(st)
    md += ["", f"Idle checks: {len(st.get('idle_checks', []))} (" +
           f"{sum(1 for c in st.get('idle_checks', []) if not c['idle'])} busy). Host rates R_h: "
           f"{st.get('host_rates', {})} (measured: {st.get('host_rates_measured', {})}; R_h is at least 1 tick over "
           f"the longest pilot block).", "",
           "### Round 2, for reference only", "",
           "Round 2's results, the **three-structure Q32 (retired)** against TC, are in "
           "[`../results-r2/results.md`](../results-r2/results.md). They enter no round-3 measure.", ""]
    return md


def cpu_time_section(st):
    """rule-r3 item 4 (as rule-r2, revision of Oct 29, 2026): each timed rep's own process CPU time beside its time,
    reported only; per job and row over the rounds' kept reps (round 2's table, with round 3's rows)."""
    recs = [r for r in st.get("records", []) if r.get("role") == "round" and r.get("cpu_share") is not None]
    if not recs:
        return []
    md = ["", "### The measured process's own CPU time (reported only; enters no verdict)", "",
          "CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the "
          "same whole rep (create and destroy included), for the rep kept in each round.", "",
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
    """the realized reuse of a job, from the generator's file beside the workloads (build/r3/work)"""
    key = (bool(st.get("toy")), job)
    if key in _REUSE:
        return _REUSE[key]
    base = C.HERE / "build" / ("r3toy" if st.get("toy") else "r3") / "work" / f"job-{job:02d}.r3.json"
    try:
        _REUSE[key] = json.loads(base.read_text()).get("reuse")
    except OSError:
        _REUSE[key] = None
    return _REUSE[key]


if __name__ == "__main__":
    report(sys.argv[1])
