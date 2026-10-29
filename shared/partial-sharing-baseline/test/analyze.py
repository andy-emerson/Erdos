"""Analysis of a run (research/rule.md items 6-12): results.md and machine-readable CSVs, from OUTDIR/state.json.

    python3 analyze.py OUTDIR

Per job and measure only: time (paired bootstrap interval, band h from the null control), gcd work and memory (fixed
bands, severe-memory-loss flag), D1 against D2 head to head, per-stratum counts and median ratios, and the reported-only
items of item 12. There is no overall or combined verdict (item 11). A run stopped at 4 hours, or stopped before the
measured rounds, is reported with what it has and no verdict.
"""
import csv
import json
import sys
from pathlib import Path

import numpy as np

import common as C

NA = "n/a"


# ---------------------------------------------------------------------------------------------------- statistics
def boot_ratio(a, b):
    """median(a) / median(b) and its 95% interval over BOOT paired resamples of the rounds (same draw for both)."""
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
    if ratio <= C.FIXED_WIN:
        return "win"
    if ratio >= C.FIXED_LOSS:
        return "loss"
    return "tie"


# ---------------------------------------------------------------------------------------------------- data
class Data:
    def __init__(self, st):
        self.st = st
        self.stopped = set(st.get("stopped_designs", []))
        self.rounds = {}          # (job, row) -> {round: rec}
        self.capped = {}          # (job, row) -> rec of the rep that passed the cap
        for rec in st.get("records", []):
            if rec.get("label") == "pilot" or rec["kind"] == "calib" or "bug" in rec:
                continue
            key = (rec["job"], rec["row"])
            if rec.get("overcap"):
                self.capped[key] = rec
            else:
                self.rounds.setdefault(key, {})[rec["round"]] = rec
        for k, c in st.get("calibration", {}).items():
            job, row = k.split(":")
            if c.get("overcap"):
                self.capped[(int(job), row)] = c
                if row == "ctl":
                    self.capped[(int(job), "ctl-null")] = c
        self.counts = {}
        for k, c in st.get("counts", {}).items():
            impl, job = k.split(":")
            self.counts[(int(job), impl)] = c

    def times(self, job, row, field="total"):
        """The row's per-round times (12 rounds), or (None, reason)."""
        if C.ROW_IMPL[row] in self.stopped:
            return None, f"{row}: stopped by a bug (item 5), re-run pending"
        if (job, row) in self.capped:
            return None, "over cap"
        rs = self.rounds.get((job, row), {})
        if len(rs) < C.ROUNDS:
            return None, f"{row}: {len(rs)} of {C.ROUNDS} rounds"
        return np.array([rs[k][field] for k in range(1, C.ROUNDS + 1)]), None

    def cap_time(self, job, row):
        c = self.capped.get((job, row))
        return c.get("cap_time") if c else None

    def gcdwork(self, job, impl, phases=("entry", "add", "sub", "mul", "div"), part=None):
        """G: the sum of gcd_work over the job's entry and operations (rule item 6); the ceiling's read-out phase,
        if the library reports one, is not part of G."""
        c = self.counts.get((job, impl))
        if c is None or "counts" not in c or impl in self.stopped:
            return None, (c or {}).get("na") or ("bug" if c and "bug" in c else "no counting run")
        tot = 0
        for line in c["counts"]:
            t = line.split()
            if t[0] == "ctr" and t[3] == "gcd_work" and (phases is None or t[1] in phases) and \
                    (part is None or t[2] == part):
                tot += int(t[4])
        return tot, None

    def memory(self, job, impl, when="end", kind="used"):
        c = self.counts.get((job, impl))
        if c is None or "mem" not in c or impl in self.stopped:
            return None, (c or {}).get("na") or "no counting run"
        tot, n = 0, None
        for line in c["mem"]:
            t = line.split()
            if t[0] == "memN" and t[1] == when:
                n = int(t[2])
            elif t[0] == "mem" and t[1] == when:
                tot += int(t[-2] if kind == "used" else t[-1])
        if not n:
            return None, "N = 0"
        return tot / n, None


# ---------------------------------------------------------------------------------------------------- comparisons
def null_h(d, job):
    a, why = d.times(job, "ctl")
    b, why2 = d.times(job, "ctl-null")
    if a is None or b is None:
        return None, None, why or why2
    r, lo, hi = boot_ratio(a, b)
    spread = max(1 - lo, hi - 1, 0)
    return max(C.BAND_MIN, spread), (r, lo, hi, spread), None


def compare(d, job, X, Y, h, field="total", h2h=False):
    """X against Y (Y the control, or D2 head to head) on one job: a dict per measure."""
    out = {}
    # time: T(Y) / T(X)
    tx, wx = d.times(job, X, field)
    ty, wy = d.times(job, Y, field)
    if h2h and wx == "over cap" and wy == "over cap":
        out["time"] = dict(verdict=NA, note="both over cap")
    elif wx == "over cap":
        out["time"] = dict(verdict=f"{X} over cap" if h2h else "over cap", cap_time=d.cap_time(job, X))
    elif h2h and wy == "over cap":
        out["time"] = dict(verdict=f"{Y} over cap", cap_time=d.cap_time(job, Y))
    elif tx is None or ty is None or h is None:
        out["time"] = dict(verdict=NA, note=wx or wy or "no null control")
    else:
        r, lo, hi = boot_ratio(ty, tx)
        out["time"] = dict(ratio=r, lo=lo, hi=hi, h=h, verdict=time_verdict(lo, hi, h))
    if field != "total":
        return out
    ix, iy = C.ROW_IMPL[X], C.ROW_IMPL[Y]
    gx, wx = d.gcdwork(job, ix)
    gy, wy = d.gcdwork(job, iy)
    if gx is None or gy is None:
        out["gcd"] = dict(verdict=NA, note=wx or wy)
    elif gy == 0:
        out["gcd"] = dict(verdict=NA, note=f"G({Y}) = 0")
    else:
        out["gcd"] = dict(ratio=gx / gy, verdict=fixed_verdict(gx / gy))
    bx, wx = d.memory(job, ix)
    by, wy = d.memory(job, iy)
    ex, _ = d.memory(job, ix, "after_entry")
    ey, _ = d.memory(job, iy, "after_entry")
    if bx is None or by is None:
        out["mem"] = dict(verdict=NA, note=wx or wy)
    else:
        r = bx / by
        out["mem"] = dict(ratio=r, entry_ratio=(ex / ey if ex and ey else None), verdict=fixed_verdict(r),
                          flag=r >= C.SEVERE)
    return out


def fmt(x, nd=3):
    return NA if x is None else f"{x:.{nd}f}"


def magnitude(m, meas):
    if meas == "time":
        if "ratio" in m:
            return f"{m['ratio']:.3f} [{m['lo']:.3f}, {m['hi']:.3f}], h = {m['h']:.3f}"
        if m.get("cap_time") is not None:
            return f"rep past the cap: {m['cap_time'] / 1e9:.2f} s per run"
        return m.get("note", "")
    if meas == "gcd":
        return f"{m['ratio']:.3f}" if "ratio" in m else m.get("note", "")
    if "ratio" in m:
        s = f"end {m['ratio']:.3f}, after entry {fmt(m.get('entry_ratio'))}"
        return s + (" **severe-memory-loss flag**" if m.get("flag") else "")
    return m.get("note", "")


# ---------------------------------------------------------------------------------------------------- report
COMPS = (("D1 vs ctl", "D1", "ctl", False), ("D2 vs ctl", "D2", "ctl", False), ("D1 vs D2", "D1", "D2", True))
MEAS = (("time", "Time"), ("gcd", "Gcd work"), ("mem", "Memory (end of job)"))


def report(outdir):
    outdir = Path(outdir)
    st = json.loads((outdir / "state.json").read_text())
    d = Data(st)
    md = []
    rule = st.get("rule_text") or C.RULE_FILE.read_text()   # the rule as verified at the start of the run
    status = "finished" if st.get("finished") else "in progress"
    if st.get("hard_stopped"):
        hs = st["hard_stopped"]
        status = f"**stopped at 4 hours**, in {hs['step']} ({hs['where']}); no verdict is reported from a stopped run"
    elif st.get("run_stopped"):
        rs = st["run_stopped"]
        status = f"**run stopped** in {rs['step']}, reported to the author: {rs['reason']}"
    toy = st.get("toy") or st.get("stub")
    md += ["# Skeleton baselines: results", ""]
    if toy:
        md += ["**TOY RUN with the stub (test/stub/): a test of the harness, not a measurement. Nothing here is a "
               "result about the skeletons.**", ""]
    md += [f"Run started {st['started']}; status: {status}. Wall time on the run clock: "
           f"{st.get('elapsed_saved', 0) / 60:.1f} min.", "",
           "Per measure only: **this report names no overall winner and combines no measures** (rule item 11).", "",
           "## The measurement rule (research/rule.md, verbatim, written before the first trial)", "",
           f"sha256 `{st['rule_sha256']}`" + ("" if st["rule_sha256"] in (C.RULE_SHA256, C.RULE_SHA256_AT_RUN) else
                                               " (**differs from the fixed rule**)"),
           "", "<!-- rule begins -->", rule.rstrip("\n"), "<!-- rule ends -->", ""]
    md += calibration_section(st)
    md += run_section(st)
    md += workload_section(st)
    rows_csv = write_rounds_csv(outdir, st)
    write_counts_csv(outdir, d)
    verdict_ok = st.get("finished") and not st.get("hard_stopped") and not st.get("run_stopped")
    if verdict_ok:
        md += verdict_sections(d, st, outdir)
    else:
        md += ["## Verdicts", "", "None: the run did not finish its measured rounds (see status).", ""]
        for f in ("verdicts.csv", "strata.csv", "reported.csv"):
            (outdir / f).unlink(missing_ok=True)
    md += detail_sections(d, st)
    (outdir / "results.md").write_text("\n".join(md) + "\n")
    return rows_csv


def calibration_section(st):
    md = ["## Calibration (fixed for all 12 rounds; enters no measure)", "",
          "| job | row | r | calibration rep, r runs (ms) | reps per invocation |", "| --- | --- | --- | --- | --- |"]
    cal = st.get("calibration", {})
    for k in sorted(cal, key=lambda k: (int(k.split(":")[0]), C.rows_of(int(k.split(":")[0])).index(k.split(":")[1]))):
        c = cal[k]
        job, row = k.split(":")
        sh = " (ctl's)" if c.get("shared_from") else ""
        if "bug" in c:
            md.append(f"| {job} | {row} | - | bug (see Bugs) | - |")
        elif c.get("overcap"):
            md.append(f"| {job} | {row} | {c['reps_out'][-1][0]} | over cap: {c['cap_rep_ns'] / 1e6:.1f} | - |")
        else:
            md.append(f"| {job} | {row} | {c['r']}{sh} | {c['rep_ns'] / 1e6:.2f}{sh} | {c['reps']}{sh} |")
    return md + [""]


def run_section(st):
    md = ["## Run", ""]
    p = st.get("pilot")
    if p:
        md.append(f"- **Pilot** (discarded): timed part {p['timed_wall_s']:.1f} s, gate pilot (job 20) "
                  f"{p['gate_wall_s']:.1f} s; projected total {p['projected_s'] / 3600:.2f} h "
                  f"(limit 4 h).")
    u = st.get("unit_tests", {})
    if u:
        md.append("- **Unit tests** (test build, widths 64 and 32): " +
                  ", ".join(f"{k} {'pass' if v['rc'] == 0 else 'FAIL'}" for k, v in sorted(u.items())))
    g = st.get("gates", {})
    if g:
        fails = [k for k, v in g.items() if "fail" in v]
        md.append(f"- **Gates:** {len(g) - len(fails)} of {len(g)} passed" +
                  (f"; failed: {', '.join(sorted(fails))}" if fails else "") + ".")
    cnt = st.get("counts", {})
    if cnt:
        na = [f"{k} ({v['na']})" for k, v in cnt.items() if "na" in v]
        rep = [f"{k}: {v['repeat']}" for k, v in sorted(cnt.items()) if "repeat" in v]
        md.append(f"- **Counting runs:** {len(cnt)}; n/a: {', '.join(na) if na else 'none'}. Repeated (jobs "
                  f"{', '.join(map(str, C.COUNT_REPEAT_JOBS))}): " + ("; ".join(rep) if rep else "none run") + ".")
    if st.get("not_constructible"):
        md.append(f"- **Not constructible:** jobs {st['not_constructible']} (n/a for every row, in no stratum).")
    md += ["", "### Bugs after the gates (item 5)", ""]
    bugs = st.get("bugs", [])
    if not bugs:
        md.append("None.")
    for b in bugs:
        md.append(f"- {b['design']} ({b['row']}), job {b['job']}, {b['where']}: {json.dumps(b['detail'])}"
                  + (f"; re-run {b['rerun']}" if b.get("rerun") else "; **re-run pending**"))
    if st.get("pre_gate_failures"):
        md += ["", "Check failures before the gates: " + "; ".join(json.dumps(x) for x in st["pre_gate_failures"])]
    return md + [""]


def workload_section(st):
    md = ["## Workloads and realized sharing", "",
          "Realized sharing over 10,000 random pairs of distinct entered denominators: fraction with gcd > 1; mean "
          "bits(gcd) over those pairs; mean bits(gcd) / bits(min) over all pairs. Result operands (G3): realized share "
          "of operands that are earlier results, and distinct result denominators used as operands.", "",
          "| job | label | D | V | gcd > 1 | mean gcd bits | mean rel. | result operands | distinct result dens | "
          "notes |", "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for j in sorted(st.get("gen", {}), key=int):
        m = st["gen"][j]
        if not m.get("constructible", True):
            md.append(f"| {j} | {m['name']} | | | | | | | | **not constructible**: {m.get('reason')} |")
            continue
        s, o = m.get("sharing", {}), m.get("operands", {})
        notes = []
        for k in ("redraws_total", "redraws_max", "lg_failures", "restarts", "g6_redraws"):
            if k in m:
                notes.append(f"{k} {m[k]}")
        md.append(f"| {j} | {m['name']} | {m.get('D', m.get('D_entered', ''))} | {m.get('V', '')} | "
                  f"{fmt(s.get('frac_gcd_gt1'), 4)} | {fmt(s.get('mean_gcd_bits'), 2)} | "
                  f"{fmt(s.get('mean_rel_gcd_bits'), 3)} | {fmt(o.get('result_operand_share'), 3)} | "
                  f"{o.get('distinct_result_dens_as_operands', '')} | {', '.join(notes)} |")
    return md + [""]


def verdict_sections(d, st, outdir):
    jobs = [int(j) for j in st["jobs"] if int(j) not in st.get("not_constructible", [])]
    per = {}
    vrows = []
    hinfo = {}
    for job in jobs:
        h, nul, why = null_h(d, job)
        hinfo[job] = (h, nul, why)
        per[job] = {}
        for name, X, Y, h2h in COMPS:
            per[job][name] = compare(d, job, X, Y, h, h2h=h2h)
        if C.JOBS[job]["ceil"]:
            per[job]["ceil vs ctl"] = compare(d, job, "ceil", "ctl", h)
        for name, res in per[job].items():
            for meas, m in res.items():
                vrows.append(dict(job=job, name=C.JOBS[job]["name"], comparison=name, measure=meas,
                                  ratio=m.get("ratio"), lo=m.get("lo"), hi=m.get("hi"), h=m.get("h"),
                                  entry_ratio=m.get("entry_ratio"), verdict=m["verdict"],
                                  flag=m.get("flag", ""), note=m.get("note", "")))
    md = ["## Per-stratum results (item 10)", "",
          "Verdict jobs only (1-18, 21-25). For each comparison and measure: wins, ties, losses, over-cap results "
          "(and n/a), the median ratio over the stratum's jobs that have one, and severe-memory-loss flags. Time "
          "ratio T(ctl)/T(X) (head to head T(D2)/T(D1)); gcd work G(X)/G(ctl) (G(D1)/G(D2)); memory B(X)/B(ctl) "
          "(B(D1)/B(D2)), end of job. A win in D1 vs D2 is a win for D1. Bugs are listed under Run.", ""]
    srows = []
    for s in C.STRATA_ORDER:
        sj = [j for j in C.STRATA[s] if j in jobs]
        md += [f"### {s} (jobs {', '.join(map(str, sj)) or 'none run'})", "",
               "| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |",
               "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
        for name, *_ in COMPS:
            for meas, label in MEAS:
                ms = [per[j][name][meas] for j in sj]
                cnt = {k: sum(1 for m in ms if m["verdict"] == k) for k in ("win", "tie", "loss")}
                oc = sum(1 for m in ms if "over cap" in m["verdict"])
                na = sum(1 for m in ms if m["verdict"] == NA)
                rs = [m["ratio"] for m in ms if "ratio" in m]
                med = float(np.median(rs)) if rs else None
                flags = sum(1 for m in ms if m.get("flag"))
                md.append(f"| {name} | {label} | {cnt['win']} | {cnt['tie']} | {cnt['loss']} | {oc} | {na} | "
                          f"{fmt(med)} | {flags if meas == 'mem' else ''} |")
                srows.append(dict(stratum=s, comparison=name, measure=meas, win=cnt["win"], tie=cnt["tie"],
                                  loss=cnt["loss"], over_cap=oc, na=na, median_ratio=med,
                                  flags=flags if meas == "mem" else ""))
        md.append("")
    md += ["## Per-job verdicts and magnitudes (items 7-9)", "",
           "Every verdict with its magnitude. Time: ratio [95% interval], h. Memory: B ratio at the end (the verdict) "
           "and after entry.", ""]
    for job in jobs:
        h, nul, why = hinfo[job]
        nl = (f"null control T(ctl)/T(ctl-null) {nul[0]:.3f} [{nul[1]:.3f}, {nul[2]:.3f}], spread {nul[3]:.3f}, "
              f"h = {h:.3f}") if nul else f"null control n/a ({why})"
        lbl = C.JOBS[job]["name"]
        md += [f"### Job {job}: {lbl}", "", nl, "", "| comparison | measure | verdict | magnitude |",
               "| --- | --- | --- | --- |"]
        for name, res in per[job].items():
            for meas, label in MEAS:
                m = res[meas]
                md.append(f"| {name} | {label} | {m['verdict']} | {magnitude(m, meas)} |")
        md.append("")
    rep_rows = []
    md += reported_sections(d, st, jobs, per, hinfo, rep_rows, vrows)
    write_csv(outdir / "verdicts.csv", vrows)
    write_csv(outdir / "strata.csv", srows)
    write_csv(outdir / "reported.csv", rep_rows)
    return md


def reported_sections(d, st, jobs, per, hinfo, rep_rows, vrows):
    md = ["## Also reported (item 12)", ""]
    # ceiling gap
    md += ["### Ceiling gap", "", "T(ceil)/T(X) with its interval; B(X)/B(ceil) (deterministic).", "",
           "| job | X | T(ceil)/T(X) | B(X)/B(ceil), end | after entry |", "| --- | --- | --- | --- | --- |"]
    for job in [j for j in jobs if C.JOBS[j]["ceil"]]:
        for X in ("ctl", "D1", "D2"):
            tc, w1 = d.times(job, "ceil")
            tx, w2 = d.times(job, X)
            t = boot_ratio(tc, tx) if tc is not None and tx is not None else None
            bx, _ = d.memory(job, C.ROW_IMPL[X])
            bc, _ = d.memory(job, "ceil")
            ex, _ = d.memory(job, C.ROW_IMPL[X], "after_entry")
            ec, _ = d.memory(job, "ceil", "after_entry")
            tr = f"{t[0]:.3f} [{t[1]:.3f}, {t[2]:.3f}]" if t else f"n/a ({w1 or w2})"
            br = bx / bc if bx and bc else None
            er = ex / ec if ex and ec else None
            md.append(f"| {job} | {X} | {tr} | {fmt(br)} | {fmt(er)} |")
            rep_rows.append(dict(item="ceiling gap", job=job, row=X, time_ratio=t[0] if t else None,
                                 lo=t[1] if t else None, hi=t[2] if t else None, mem_ratio=br, mem_entry_ratio=er))
    # operation time alone
    md += ["", "### Operation time alone (entry excluded), by item 8 with the job's h", "",
           "| job | comparison | verdict | magnitude |", "| --- | --- | --- | --- |"]
    for job in jobs:
        h = hinfo[job][0]
        for name, X, Y, h2h in COMPS:
            m = compare(d, job, X, Y, h, field="ops", h2h=h2h)["time"]
            md.append(f"| {job} | {name} | {m['verdict']} | {magnitude(m, 'time')} |")
            vrows.append(dict(job=job, name=C.JOBS[job]["name"], comparison=name, measure="ops-time",
                              ratio=m.get("ratio"), lo=m.get("lo"), hi=m.get("hi"), h=m.get("h"), entry_ratio=None,
                              verdict=m["verdict"], flag="", note=m.get("note", "")))
    # arrival
    if 19 in jobs and 20 in jobs:
        md += ["", "### Arrival (job 19 arriving, job 20 its early twin)", "",
               "Verdicts of items 8-9 on each are in the per-job sections. Per row: T(19)/T(20) with the paired "
               "bootstrap interval over the shared rounds, G(19)/G(20), B(19)/B(20) at the end and after entry.", "",
               "| row | T(19)/T(20) | G(19)/G(20) | B(19)/B(20) end | after entry |", "| --- | --- | --- | --- | --- |"]
        for row in ("ctl", "ctl-null", "D1", "D2"):
            a, w1 = d.times(19, row)
            b, w2 = d.times(20, row)
            t = boot_ratio(a, b) if a is not None and b is not None else None
            impl = C.ROW_IMPL[row]
            g1, _ = d.gcdwork(19, impl)
            g2, _ = d.gcdwork(20, impl)
            b1, _ = d.memory(19, impl)
            b2, _ = d.memory(20, impl)
            e1, _ = d.memory(19, impl, "after_entry")
            e2, _ = d.memory(20, impl, "after_entry")
            gr = g1 / g2 if g1 is not None and g2 else None
            br = b1 / b2 if b1 and b2 else None
            er = e1 / e2 if e1 and e2 else None
            tr = f"{t[0]:.3f} [{t[1]:.3f}, {t[2]:.3f}]" if t else f"n/a ({w1 or w2})"
            md.append(f"| {row} | {tr} | {fmt(gr)} | {fmt(br)} | {fmt(er)} |")
            rep_rows.append(dict(item="arrival 19/20", job=19, row=row, time_ratio=t[0] if t else None,
                                 lo=t[1] if t else None, hi=t[2] if t else None, gcd_ratio=gr, mem_ratio=br,
                                 mem_entry_ratio=er))
    # width 32
    w32 = [j for j in (26, 27) if j in jobs]
    if w32:
        md += ["", "### Width 32", "", "Verdicts of items 8-9 are in the per-job sections. Ordering of ctl, D1 and D2 "
               "on each measure (smallest first: time T, gcd work G, memory B at the end), against the width-64 "
               "matrix job of the same sharing level and mix.", "",
               "| W32 job | W64 job | measure | W32 order | W64 order | match |", "| --- | --- | --- | --- | --- | --- |"]
        for j in w32:
            t = C.JOBS[j]["w64_twin"]
            for meas in ("time", "gcd", "mem"):
                o32, o64 = order(d, j, meas), order(d, t, meas)
                match = NA if o32 is None or o64 is None else ("yes" if o32 == o64 else "no")
                md.append(f"| {j} | {t} | {meas} | {' < '.join(o32) if o32 else NA} | "
                          f"{' < '.join(o64) if o64 else NA} | {match} |")
                rep_rows.append(dict(item="w32 ordering", job=j, row=meas, note=f"{o32} vs {o64}: {match}"))
    # entry against operations
    md += ["", "### Entry against operations", "",
           "Per design and job: share of total time in entry (median entry / median total), share of gcd work in "
           "entry, and within operations the share of gcd work in `store` (the harness cannot time inside a call, "
           "so `store`'s share is given for gcd work only).", "",
           "| job | row | entry share of time | entry share of gcd work | store share of operations' gcd work |",
           "| --- | --- | --- | --- | --- |"]
    for job in jobs:
        for row in ("ctl", "D1", "D2"):
            te, _ = d.times(job, row, "entry")
            tt, _ = d.times(job, row, "total")
            ts = float(np.median(te) / np.median(tt)) if te is not None and tt is not None else None
            g, _ = d.gcdwork(job, row)
            ge, _ = d.gcdwork(job, row, phases=("entry",))
            go, _ = d.gcdwork(job, row, phases=("add", "sub", "mul", "div"))
            gs, _ = d.gcdwork(job, row, phases=("add", "sub", "mul", "div"), part="store")
            es = ge / g if g else None
            ss = gs / go if go else None
            md.append(f"| {job} | {row} | {fmt(ts)} | {fmt(es)} | {fmt(ss)} |")
            rep_rows.append(dict(item="entry vs operations", job=job, row=row, time_ratio=ts, gcd_ratio=es,
                                 note=f"store share of operations' gcd work {ss}"))
    # results as operands
    md += ["", "### Results as operands", "", "Given per job in the Workloads table above (G3 realized).", ""]
    # gcd work against time
    md += ["### Gcd work against time", "",
           "Jobs where a design wins on gcd work against the control but not on time: candidates for wider-number "
           "tests, where gcds cost more.", ""]
    cands = []
    for job in jobs:
        for name in ("D1 vs ctl", "D2 vs ctl"):
            m = per[job][name]
            if m["gcd"]["verdict"] == "win" and m["time"]["verdict"] != "win":
                cands.append(f"- job {job} ({C.JOBS[job]['name']}), {name.split()[0]}: gcd work "
                             f"{m['gcd']['ratio']:.3f}, time {m['time']['verdict']} ({magnitude(m['time'], 'time')})")
    md += (cands or ["None."]) + [""]
    return md


def order(d, job, meas):
    vals = {}
    for row in ("ctl", "D1", "D2"):
        if meas == "time":
            t, _ = d.times(job, row)
            v = float(np.median(t)) if t is not None else None
        elif meas == "gcd":
            v, _ = d.gcdwork(job, row)
        else:
            v, _ = d.memory(job, row)
        if v is None:
            return None
        vals[row] = v
    return sorted(vals, key=lambda r: vals[r])


def detail_sections(d, st):
    md = ["## Per job: times, memory and counters", "",
          "Times are medians over the rounds, per run of the job, in ms (entry, operations, read-out, total). "
          "Memory: used and allocated bytes per rational after entry and at the end, and the per-structure breakdown "
          "(bytes). Every counter of spec §12.1, per phase and part, is in counters.csv.", ""]
    for j in sorted(map(int, st.get("jobs", []))):
        if j in st.get("not_constructible", []):
            continue
        md += [f"### Job {j}: {C.JOBS[j]['name']}", "",
               "| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | "
               "gcd work | structures (end: used/alloc) |", "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
        for row in C.rows_of(j):
            ts = []
            for f in ("entry", "ops", "read", "total"):
                t, why = d.times(j, row, f)
                ts.append(f"{np.median(t) / 1e6:.3f}" if t is not None else (why or NA))
            impl = C.ROW_IMPL[row]
            ea, _ = d.memory(j, impl, "after_entry")
            ue, _ = d.memory(j, impl, "end")
            ae, _ = d.memory(j, impl, "end", "alloc")
            g, _ = d.gcdwork(j, impl)
            c = d.counts.get((j, impl), {})
            structs = "; ".join(f"{' '.join(l.split()[2:-2])} {l.split()[-2]}/{l.split()[-1]}"
                                for l in c.get("mem", []) if l.startswith("mem end"))
            md.append(f"| {row} | {' | '.join(ts)} | {fmt(ea, 1)} | {fmt(ue, 1)} | {fmt(ae, 1)} | "
                      f"{g if g is not None else NA} | {structs} |")
        md.append("")
    return md


def write_csv(path, rows):
    keys = []
    for r in rows:
        for k in r:
            if k not in keys:
                keys.append(k)
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        for r in rows:
            w.writerow({k: ("" if v is None else v) for k, v in r.items()})


def write_rounds_csv(outdir, st):
    rows = []
    for rec in st.get("records", []) + st.get("pilot_records", []):
        rows.append(dict(label=rec.get("label", "calibration"), job=rec["job"], row=rec["row"], kind=rec["kind"],
                         round=rec.get("round"), position=rec.get("pos"), r=rec.get("r"), reps=rec.get("reps"),
                         total_ns=rec.get("total"), entry_ns=rec.get("entry"), ops_ns=rec.get("ops"),
                         read_ns=rec.get("read"), over_cap=rec.get("overcap"), cap_time_ns=rec.get("cap_time"),
                         bug=json.dumps(rec["bug"]) if "bug" in rec else "", reps_out=json.dumps(rec.get("reps_out"))))
    write_csv(outdir / "rounds.csv", rows)
    return rows


def write_counts_csv(outdir, d):
    crow, mrow = [], []
    for (job, impl), c in sorted(d.counts.items()):
        for l in c.get("counts", []):
            t = l.split()
            crow.append(dict(job=job, impl=impl, kind=t[0], phase=t[1], part=t[2], name=t[3], value=int(t[4])))
        for l in c.get("mem", []):
            t = l.split()
            if t[0] == "memN":
                mrow.append(dict(job=job, impl=impl, when=t[1], structure="N", used=int(t[2]), alloc=""))
            else:
                mrow.append(dict(job=job, impl=impl, when=t[1], structure=" ".join(t[2:-2]), used=int(t[-2]),
                                 alloc=int(t[-1])))
    write_csv(outdir / "counters.csv", crow)
    write_csv(outdir / "memory.csv", mrow)
    jrows = []
    for j, m in sorted(d.st.get("gen", {}).items(), key=lambda x: int(x[0])):
        s, o = m.get("sharing", {}), m.get("operands", {})
        jrows.append(dict(job=j, name=m["name"], stratum=C.JOBS[int(j)]["stratum"], constructible=m.get("constructible"),
                          D=m.get("D"), V=m.get("V"), frac_gcd_gt1=s.get("frac_gcd_gt1"),
                          mean_gcd_bits=s.get("mean_gcd_bits"), mean_rel_gcd_bits=s.get("mean_rel_gcd_bits"),
                          result_operand_share=o.get("result_operand_share"),
                          distinct_result_dens_as_operands=o.get("distinct_result_dens_as_operands"),
                          redraws_total=m.get("redraws_total"), restarts=m.get("restarts")))
    write_csv(outdir / "jobs.csv", jrows)


if __name__ == "__main__":
    report(sys.argv[1])
