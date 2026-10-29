"""Analysis of a run (research/rule.md items 6-12): results.md and CSVs, from OUTDIR/state.json.

    python3 analyze.py OUTDIR

Per job and measure only: time (paired bootstrap interval, band h from the null control L1 against L1-null), memory
B_used and B_heap (fixed bands, severe-memory-loss flag at 2.0, untracked-allocation flag), gcd work, multiplications
and lookups (fixed bands), per-case ratios (reported, no verdict), per-stratum counts and median ratios, and the
reported-only items of item 12: realized reuse (N, W, W/N, 2N/W after entry and at the end), memory against reuse,
counts per operation, the per-case table (gcds, gcd work, multiplications), L1-opt's tricks beside their predicted
figures with the share of L1's work each removes (O-1 to O-5), operation time alone, entry against operations with
store's share, the limitations (keeping every result; one row with every trick), gcd work against time (for the Q32
rows; L1's and L1-opt's is official at b3b88b6), and the chosen forms. There is no overall or combined verdict (item
11). A run that stopped (pilot projection, failed gate, hard stop) is reported with what it has and no verdict. Adapted
from 08's analyze.py (decisions A-a, A-b, A-g carry over).

The run after the revision of Oct 29, 2026 (the Q32 rows, an author-approved exception; rule.md, top): the verdicts
reported are the Q32 rows' (against their Q64 rows, ctl, fmpq, mpq, the best library and fz). The official Q64
verdicts stay those of commit b3b88b6, archived unchanged in test/results-q64-b3b88b6/; this run's Q64 rows serve only
the paired Q32-against-Q64 comparison and the band h of the same run (L1 against L1-null: h must come from the rounds it
bands), and every Q64 verdict that differs between the two runs is listed, without choosing. A report is never written
into the archive (common.PROTECTED).
"""
import csv
import io
import json
import subprocess
import sys
from pathlib import Path

import numpy as np

import common as C

NA = "n/a"
X_ROWS = ("L1", "L1-opt")                      # the Q64 rows: re-run, compared with b3b88b6 only
X_Q32 = C.Q32_ROWS                             # the Q32 rows: this run's verdicts
Y_ROWS = ("ctl", "fmpq", "mpq", "fz", "best")
OVER = "over capacity"
ARCHIVE_LINK = f"../{C.ARCHIVE.name}/results.md"     # from RESULTS (test/results-q32/) to the archived results
DESIGNED_AFTER = ("The Q32 rows were designed after the Q64 results of commit b3b88b6 had been seen (rule, revision "
                  "of Oct 29, 2026: an exception approved by the author to \"only while nothing has been measured\"). "
                  "The official Q64 verdicts, against ctl and the libraries and of L1-opt against L1, stay those of "
                  f"b3b88b6 ([archived results]({ARCHIVE_LINK})); the Q64 rows re-run here are used only for the "
                  "paired Q32-against-Q64 comparison and for the band h of the same run (L1 against L1-null, which must "
                  "come from the same rounds as the verdicts it bands), and every Q64 verdict that differs between the "
                  "two runs is listed below, without choosing between them")
CHAIN_NOTE = ("on this chain job every result is kept as a new element, which is not the libraries' normal use (spec "
              "§10 item 3; rule, Limitations)")
ONE_ROW_NOTE = ("L1-opt is timed with all its adopted tricks on together; the counters show each trick's effect on the "
                "work, but no trick's effect on time is measured alone (rule, Limitations: one row with every trick)")
# L1-opt's trick counters (spec §17.2), by trick, as bench.c names them
TRICKS = {
    "O-1": ["o1_skipped_" + c for c in C.O1_CASES],
    "O-2": ["o2_skipped0", "o2_skipped1"],
    "O-3": ["o3_gcd_skipped", "o3_gcd_work"],
    "O-4": ["o4_div_skipped", "o4_mul_skipped"],
    "O-5": ["o5_ops", "o5_add_skipped", "o5_gcd_skipped", "o5_gcd_work", "o5_div_skipped"],
    "O-6": ["o6_reads_avoided"],
    "O-7": ["o7_growths_avoided", "o7_bytes_avoided"],
}
PREDICTED_TRICKS = ("O-1", "O-2", "O-3", "O-4", "O-5")       # spec §17.3 item 2; O-6 and O-7 are reported only
# the share of L1's work each trick removes (rule item 12): {trick: [(what, L1 counter, L1-opt skip counters)]}
REMOVED = {
    "O-1": [("lookups", "lookup_search", TRICKS["O-1"])],
    "O-2": [("lookups", "lookup_search", TRICKS["O-2"])],
    "O-3": [("gcds", "gcd", ["o3_gcd_skipped"]), ("gcd work", "gcd_work", ["o3_gcd_work"])],
    "O-4": [("divisions", "div_exact", ["o4_div_skipped"]), ("multiplications", "mul", ["o4_mul_skipped"])],
    "O-5": [("gcds", "gcd", ["o5_gcd_skipped"]), ("gcd work", "gcd_work", ["o5_gcd_work"]),
            ("divisions", "div_exact", ["o5_div_skipped"]), ("additions", "add", ["o5_add_skipped"])],
}
# the per-operation counts of rule item 12 ("counts per operation"), as bench.c names them
OP_COUNTS = ("gcd", "gcd_work", "mul", "add", "div_exact", "lookup_search", "lookup_probe", "intern_found",
             "intern_appended", "idx_cmp")


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


def fmt(x, nd=3):
    return NA if x is None else (f"{x:.{nd}f}" if isinstance(x, float) else str(x))


# ---------------------------------------------------------------------------------------------------- data
class Data:
    def __init__(self, st):
        self.st = st
        self.stopped = set(st.get("stopped_impls", []))
        self.lib_na = set(st.get("library_na", []))
        self.over = {int(j): w for j, w in st.get("q32_over_capacity", {}).items()}
        self.rounds, self.capped = {}, {}
        for rec in st.get("records", []):
            if rec.get("label") == "pilot" or rec["kind"] != "time" or "bug" in rec:
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
        self.counts = {}
        for k, c in st.get("counts", {}).items():
            impl, job = k.split(":")
            self.counts[(int(job), impl)] = c
        self.mems = {}
        for k, c in st.get("mems", {}).items():
            impl, job = k.split(":")
            self.mems[(int(job), impl)] = c

    def unavailable(self, job, row):
        impl = C.ROW_IMPL[row]
        if row in C.Q32_ROWS and job in self.over:
            return f"{row}: {OVER} (W = {self.over[job]} > {C.Q32_CAPACITY})"
        if impl in self.stopped:
            return f"{row}: stopped by a bug (item 5), re-run pending"
        if f"{impl}:{job}" in self.lib_na:
            return f"{row}: n/a on this job (item 5)"
        return None

    def times(self, job, row, field="total"):
        why = self.unavailable(job, row)
        if why:
            return None, why
        if (job, row) in self.capped:
            return None, "over cap"
        rs = self.rounds.get((job, row), {})
        if len(rs) < C.ROUNDS:
            return None, f"{row}: {len(rs)} of {C.ROUNDS} rounds"
        return np.array([rs[k][field] for k in range(1, C.ROUNDS + 1)]), None

    def best_lib(self, job):
        t = {r: self.times(job, r)[0] for r in ("fmpq", "mpq")}
        t = {r: v for r, v in t.items() if v is not None}
        if not t:
            return None
        return min(t, key=lambda r: float(np.median(t[r])))

    def ctr(self, job, impl, name, phases=C.PHASES, part=None):
        """A counter summed over entry and operations (rule item 6: G, M and K), or None."""
        c = self.counts.get((job, impl))
        if c is None or "counts" not in c or impl in self.stopped:
            return None
        tot = 0
        for line in c["counts"]:
            t = line.split()
            if t[0] in ("ctr", "xctr") and t[3] == name and t[1] in phases and (part is None or t[2] == part):
                tot += int(t[4])
        return tot

    def mem_per(self, job, row, kind="used", when="end"):
        """Bytes per stored rational (rule item 6), or (None, reason)."""
        impl = C.ROW_IMPL[row]
        why = self.unavailable(job, row)
        if why:
            return None, why
        m = self.mems.get((job, impl))
        if m is None or "memrun" not in m:
            return None, (m or {}).get("na") or "no memory run"
        r = m["memrun"].get(when)
        if not r:
            return None, f"no {when} figure"
        n = r["stored"]
        if impl in C.LIBRARIES:
            if kind == "used":
                return (r["elsize"] * n + r["extra"] + r["hook_req"]) / n, None
            return (r["heap"] - r["elsize"] * (r["N"] - n)) / n, None
        return (r["used"] if kind == "used" else r["heap"]) / n, None

    def untracked(self, job, impl, when="end"):
        m = self.mems.get((job, impl))
        if impl not in C.LIBRARIES or m is None or "memrun" not in m or when not in m["memrun"]:
            return None
        r = m["memrun"][when]
        heap = r["heap"] - r["elsize"] * (r["N"] - r["stored"])
        allowed = r["elsize"] * r["stored"] + r["hook_usable"] + 16 * r["hook_blocks"] + 65536
        return heap > allowed


def null_h(d, job):
    a, wa = d.times(job, "L1")
    b, wb = d.times(job, "L1-null")
    if a is None or b is None:
        return None, None, wa or wb
    ratio, lo, hi = boot_ratio(b, a)
    spread = max(1 - lo, hi - 1, 0)
    return max(C.BAND_MIN, spread), (ratio, lo, hi), None


def ratio_entry(meas, ratio, verdict, extra=None):
    e = {"measure": meas, "ratio": ratio, "verdict": verdict}
    if extra:
        e.update(extra)
    return e


def compare(d, job, X, Y, h, field="total"):
    """Every measure of rule item 8 for X (L1, L1-opt, L1-Q32 or L1-opt-Q32) against Y (a row, or "best")."""
    out = []
    if X in X_Q32 and job in d.over:           # rule, revision (capacity): a finding, not n/a
        w = {"why": f"W = {d.over[job]} > {C.Q32_CAPACITY}", "W": d.over[job]}
        meas = ["time"] + ([] if field != "total" or Y == "best" else ["B_used", "B_heap"])
        return [ratio_entry(m, None, OVER, w) for m in meas]
    yrow = d.best_lib(job) if Y == "best" else Y
    # time
    if yrow is None:
        out.append(ratio_entry("time", None, NA, {"why": "no library row with 12 rounds"}))
    else:
        tx, wx = d.times(job, X, field)
        ty, wy = d.times(job, yrow, field)
        if (job, X) in d.capped:
            out.append(ratio_entry("time", None, f"{X} over cap", {"cap_time": d.capped[(job, X)].get("cap_time")}))
        elif (job, yrow) in d.capped:
            out.append(ratio_entry("time", None, f"{yrow} over cap"))
        elif tx is None or ty is None or h is None:
            out.append(ratio_entry("time", None, NA, {"why": wx or wy or "no null control"}))
        else:
            r, lo, hi = boot_ratio(ty, tx)
            out.append(ratio_entry("time", r, time_verdict(lo, hi, h), {"lo": lo, "hi": hi, "h": h, "vs": yrow}))
    if field != "total" or Y == "best":
        return out
    # memory
    for kind in ("used", "heap"):
        bx, wx = d.mem_per(job, X, kind)
        by, wy = d.mem_per(job, Y, kind)
        ex, _ = d.mem_per(job, X, kind, "after_entry")
        ey, _ = d.mem_per(job, Y, kind, "after_entry")
        if bx is None or by is None:
            out.append(ratio_entry(f"B_{kind}", None, NA, {"why": wx or wy}))
            continue
        r = bx / by
        e = ratio_entry(f"B_{kind}", r, fixed_verdict(r), {"after_entry": (ex / ey) if ex and ey else None,
                                                           "severe": r >= C.SEVERE})
        u = d.untracked(job, C.ROW_IMPL[Y])
        if u:
            e["untracked"] = True
        out.append(e)
    # gcd work, multiplications (against ctl; L1-opt also against L1), lookups (L1-opt against L1). The Q32 rows' equal
    # their Q64 rows' (spec §18.3, checked by the gates and counting runs): reported as checked, not compared.
    if X in X_ROWS and (Y == "ctl" or (X == "L1-opt" and Y == "L1")):
        ix, iy = C.ROW_IMPL[X], C.ROW_IMPL[Y]
        for meas, name in (("gcd work", "gcd_work"), ("multiplications", "mul")):
            gx, gy = d.ctr(job, ix, name), d.ctr(job, iy, name)
            if gx is None or gy is None or gy == 0:
                out.append(ratio_entry(meas, None, NA, {"why": "no counting run" if gy is None or gx is None
                                                        else "0 in the denominator", "x": gx, "y": gy}))
            else:
                out.append(ratio_entry(meas, gx / gy, fixed_verdict(gx / gy), {"x": gx, "y": gy}))
        if X == "L1-opt" and Y == "L1":
            kx, ky = d.ctr(job, "L1opt", "lookup_search"), d.ctr(job, "L1", "lookup_search")
            # the skipped lookups of each trick, beside the ratio (rule items 6 and 8)
            skips = {t: sum(d.ctr(job, "L1opt", n) or 0 for n in TRICKS[t]) for t in ("O-1", "O-2")}
            if kx is None or ky is None or ky == 0:
                out.append(ratio_entry("lookups", None, NA, {"skips": skips}))
            else:
                out.append(ratio_entry("lookups", kx / ky, fixed_verdict(kx / ky), {"x": kx, "y": ky, "skips": skips}))
    return out


# ---------------------------------------------------------------------------------------------------- report
def report(outdir):
    outdir = Path(outdir)
    if C.protected(outdir):                    # rule, revision of Oct 29, 2026: the official data are never rewritten
        raise SystemExit(f"{outdir} holds official data: refusing to write a report there")
    st = json.loads((outdir / "state.json").read_text())
    d = Data(st)
    md = (outdir / "results.md").read_text() if (outdir / "results.md").exists() else ""
    cut = md.find("\n## Run summary")
    if cut >= 0:
        md = md[:cut + 1]
    parts = [md.rstrip("\n"), "", run_section(st), pilot_section(st), workload_section(st)]
    finished = bool(st.get("finished")) and not st.get("hard_stopped") and not st.get("run_stopped")
    if finished:
        parts += verdict_sections(d, st, outdir)
    else:
        parts.append("## Verdicts\n\nNone: the run did not finish its 12 rounds (" +
                     ("stopped at the hard stop" if st.get("hard_stopped") else
                      "stopped: " + st["run_stopped"]["reason"][:300] if st.get("run_stopped") else
                      "only the pilot has run") + "). No verdict is reported from an unfinished run.\n")
    (outdir / "results.md").write_text("\n".join(parts) + "\n")


def run_section(st):
    L = ["## Run summary", ""]
    L.append(f"- started {st.get('started')}; elapsed (run's own clock) {st.get('elapsed_saved', 0) / 60:.1f} min")
    L.append(f"- libraries: {st.get('versions', NA)}")
    L.append(f"- 08 copies (lib/vendor08, commit {st.get('commit_08')}): sha256 checked before the run")
    if st.get("unit_tests"):
        for k, v in st["unit_tests"].items():
            last = v["out"].strip().splitlines()[-1] if v["out"].strip() else ""
            L.append(f"- unit tests {k}: {'pass' if v['rc'] == 0 else 'FAIL'} ({last})")
    if st.get("hard_stopped"):
        L.append(f"- **stopped** at the hard stop in {st['hard_stopped']['step']} ({st['hard_stopped']['where']})")
    if st.get("run_stopped"):
        L.append(f"- **run stopped** in {st['run_stopped']['step']}: {st['run_stopped']['reason'][:500]}")
    if st.get("finished"):
        L.append(f"- finished {st['finished']}")
    for b in st.get("bugs", []):
        L.append(f"- BUG: {b}")
    for b in st.get("pre_gate_failures", []):
        L.append(f"- check failure before the gates: {b}")
    gates = st.get("gates", {})
    if gates:
        ok = sum(1 for v in gates.values() if "fail" not in v)
        L.append(f"- gates: {ok} of {len(gates)} passed")
    return "\n".join(L) + "\n"


def pilot_section(st):
    p = st.get("pilot")
    if not p:
        return ""
    L = ["## Pilot (rule, Run plan step 3; discarded)", "",
         f"Jobs {p['jobs']}, every row, 2 rounds. Pilot wall time {p['wall_s']:.1f} s (factor {p['factor']:.2f}); "
         f"elapsed at the end of the pilot {p['elapsed_s'] / 60:.2f} min; **projected whole run "
         f"{p['projected_s'] / 60:.1f} min** (limit {p['limit_s'] / 60:g} min).", "",
         "Median per-run total over the pilot's rounds (ms), with the calibrated r and form:", "",
         "| job | " + " | ".join(C.rows_of(1)) + " |", "| --- |" + " --- |" * len(C.rows_of(1))]
    recs = {}
    for r in st.get("pilot_records", []):
        if r["kind"] == "time" and "total" in r:
            recs.setdefault((r["job"], r["row"]), []).append(r["total"])
    cal = st.get("pilot_calibration", {})
    for job in p["jobs"]:
        cells = []
        for row in C.rows_of(1):
            v = recs.get((job, row))
            c = cal.get(str(job), {}).get("r", {}).get(row if row != "L1-null" else "L1", {})
            if v:
                f = f" {c.get('form')}" if c.get("form") else ""
                cells.append(f"{np.median(v) / 1e6:.2f} (r={c.get('r')}{f})")
            else:
                cells.append("-")
        L.append(f"| {job} | " + " | ".join(cells) + " |")
    return "\n".join(L) + "\n"


def workload_section(st):
    g = st.get("gen", {})
    if not g:
        return ""
    L = ["## Workloads and realized reuse (rule item 12)", "",
         "N stored rationals, W distinct whole numbers, W/N whole numbers per rational, 2N/W places per whole number; "
         "each after entry and at the end.", "",
         "| job | name | N entry | W entry | W/N entry | 2N/W entry | N end | W end | W/N end | 2N/W end | "
         "result operand share |",
         "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for k in sorted(g, key=int):
        m = g[k]
        if not m.get("constructible", True):
            L.append(f"| {k} | {m['name']} | not constructible: {m['reason']} |")
            continue
        e, a = m["reuse"]["end"], m["reuse"]["after_entry"]
        L.append(f"| {k} | {m['name']} | {a['N']} | {a['W']} | {a['W_per_N']:.3f} | {a['places_per_whole']:.2f} | "
                 f"{e['N']} | {e['W']} | {e['W_per_N']:.3f} | {e['places_per_whole']:.2f} | "
                 f"{m['operands']['result_operand_share']:.3f} |")
    L += ["", "Share of operations meeting each case (spec §7), per operation:", ""]
    for k in sorted(g, key=int):
        m = g[k]
        if m.get("constructible", True):
            s = "; ".join(f"{ph}: " + ", ".join(f"{c.split('_', 1)[1]} {v:.3f}" for c, v in sh.items() if v)
                          for ph, sh in m["reuse"]["case_share"].items())
            L.append(f"- job {k} ({m['name']}): {s}")
    return "\n".join(L) + "\n"


def q32_pairs(job):
    """The Q32 rows' comparisons (rule item 1 and the revision of Oct 29, 2026): each against its Q64 row (paired),
    and against ctl, fmpq, mpq, the best library and fz."""
    ys = [Y for Y in Y_ROWS if Y != "fz" or C.JOBS[job]["fz"]]
    return [(X, C.Q32_PAIR[X]) for X in X_Q32] + [(X, Y) for X in X_Q32 for Y in ys]


def q64_pairs(job):
    """The Q64 comparisons of the run at b3b88b6, re-run here only to list the verdicts that changed."""
    return [(X, Y) for X in X_ROWS for Y in Y_ROWS if Y != "fz" or C.JOBS[job]["fz"]] + [("L1-opt", "L1")]


def csv_row(job, X, Y, e):
    return dict(job=job, X=X, Y=Y, measure=e["measure"], verdict=e["verdict"], ratio=e["ratio"], lo=e.get("lo"),
                hi=e.get("hi"), h=e.get("h"), after_entry=e.get("after_entry"), severe=e.get("severe"),
                untracked=e.get("untracked"))


def official_q64():
    """The official Q64 verdicts: verdicts.csv of commit b3b88b6, from the archive (test/results-q64-b3b88b6/, its
    sha256 checked), else from git at that commit. ({(job, X, Y, measure): row}, source), or (None, why)."""
    if C.archive_check() is None:
        text, src = (C.ARCHIVE / "verdicts.csv").read_text(), f"[{C.ARCHIVE.name}/verdicts.csv](../{C.ARCHIVE.name}/verdicts.csv)"
    else:
        p = subprocess.run(["git", "show", f"{C.OFFICIAL_Q64_COMMIT}:{C.OFFICIAL_Q64_VERDICTS}"], cwd=C.ROOT,
                           capture_output=True, text=True)
        if p.returncode:
            return None, f"neither the archive ({C.archive_check()}) nor git could be read"
        text, src = p.stdout, f"`git show {C.OFFICIAL_Q64_COMMIT}:{C.OFFICIAL_Q64_VERDICTS}`"
    return {(int(r["job"]), r["X"], r["Y"], r["measure"]): r for r in csv.DictReader(io.StringIO(text))}, src


def verdict_sections(d, st, outdir):
    jobs = sorted(int(j) for j in st.get("gen", {}) if st["gen"][j].get("constructible", True))
    rows_csv, rerun_csv, per_job = [], [], {}
    over = sorted(j for j in jobs if j in d.over)
    out = ["## Verdicts of the Q32 rows (rule items 8-10 and the revision of Oct 29, 2026; per measure only, no "
           "overall winner)", "",
           f"**{DESIGNED_AFTER}.**", "",
           f"*Over capacity:* {len(over)} of {len(jobs)} jobs " +
           (("(" + ", ".join(f"job {j}, W = {d.over[j]}" for j in over) + ")") if over else "") +
           f" have more than {C.Q32_CAPACITY:,} distinct whole numbers at the end; the Q32 rows do not run there, and "
           "each of their verdicts on those jobs is **over capacity**, a finding about the design at that width.", "",
           f"*Limitation:* {ONE_ROW_NOTE}.", ""]
    for job in jobs:
        h, null, why = null_h(d, job)
        per_job[job] = {"h": h, "null": null}
        out.append(f"#### Job {job} ({C.JOBS[job]['name']})")
        out.append("")
        out.append(f"Null control L1 against L1-null (this run): " +
                   (f"ratio {null[0]:.3f} [{null[1]:.3f}, {null[2]:.3f}], h = {h:.3f}" if null else f"n/a ({why})"))
        out.append("")
        if job in d.over:
            out.append(f"L1-Q32 and L1-opt-Q32: **over capacity** (W = {d.over[job]} > {C.Q32_CAPACITY:,}); "
                       "every Q32 verdict of this job is over capacity.")
            out.append("")
        out.append("| X | Y | measure | verdict | ratio | interval / after entry | flags |")
        out.append("| --- | --- | --- | --- | --- | --- | --- |")
        for X, Y in q32_pairs(job):
            for e in compare(d, job, X, Y, h):
                flags = []
                if e.get("severe"):
                    flags.append("severe memory loss")
                if e.get("untracked"):
                    flags.append("untracked allocation")
                if Y in ("fmpq", "mpq", "best") and C.JOBS[job]["family"] == "F5":
                    flags.append("chain: every result kept")
                if Y == "fz":
                    flags.append("ceiling gap")
                if Y in ("L1", "L1-opt"):
                    flags.append("paired Q32 against Q64")
                if Y == "best" and e.get("vs"):
                    flags.append(f"best library: {e['vs']}")
                iv = (f"[{e['lo']:.3f}, {e['hi']:.3f}], h {e['h']:.3f}" if "lo" in e else
                      e["why"] if e["verdict"] == OVER else
                      fmt(e.get("after_entry")) if "after_entry" in e else "")
                out.append(f"| {X} | {Y} | {e['measure']} | {e['verdict']} | {fmt(e['ratio'])} | {iv} | "
                           f"{', '.join(flags)} |")
                rows_csv.append(csv_row(job, X, Y, e))
        # operation time alone (item 12), with the job's h
        out.append("")
        ys = ("L1", "L1-opt", "ctl", "fmpq", "mpq") + (("fz",) if C.JOBS[job]["fz"] else ())
        pairs_ops = [(X, Y) for X in X_Q32 for Y in ys if Y not in X_ROWS or Y == C.Q32_PAIR[X]]
        out.append("Operation time alone (entry and read-out excluded; rule item 12, verdicts by item 8 with the "
                   "job's h): " + "; ".join(
                       f"{X} vs {Y} {e['verdict']} {fmt(e['ratio'])}" +
                       (f" [{e['lo']:.3f}, {e['hi']:.3f}]" if "lo" in e else "")
                       for X, Y in pairs_ops for e in compare(d, job, X, Y, h, field="ops")))
        if C.JOBS[job]["family"] == "F5":
            out.append("")
            out.append(f"*Limitation:* {CHAIN_NOTE}.")
        out.append("")
        for X, Y in q64_pairs(job):
            for e in compare(d, job, X, Y, h):
                rerun_csv.append(csv_row(job, X, Y, e))
    write_csv(outdir / "verdicts.csv", rows_csv)
    write_csv(outdir / "q64_rerun_verdicts.csv", rerun_csv)
    # rule item 10: the strata first, the per-job verdicts and magnitudes beneath
    head, per_job_lines = out[:8], out[8:]
    out = head + strata_section(rows_csv, d, jobs) + ["### Per job and measure, with magnitudes (rule item 8)", ""] + \
        per_job_lines
    out += gcd_vs_time_section(d, rows_csv, jobs)
    out += changed_q64_section(rerun_csv)
    out += model_section(d, st, jobs)
    out += reported_sections(d, st, jobs, outdir)
    out += ["## Limitations (rule, Limitations)", "",
            "- **Keeping every result** (spec §10 item 3): every row stores every result as a new element; on the "
            "chain jobs (8-11) this is not the libraries' normal use, so it is noted beside every chain verdict against "
            "fmpq, mpq and the best library.",
            f"- **One row with every trick:** {ONE_ROW_NOTE}.",
            f"- **Designed after the Q64 results:** {DESIGNED_AFTER}.", ""]
    return out


def gcd_vs_time_section(d, rows, jobs):
    """Rule item 12, for the Q32 rows: a job where the row removes gcd work or multiplications against ctl (fixed bands
    of item 8) but has no time win against ctl, at 64 bits, is a candidate for a wider-number test, where both cost
    more. The counts are the Q32 row's own counting run (equal to its Q64 row's, checked: spec §18.3 item 1), so they
    are reported here, not as verdicts; the time verdict is the Q32 row's against ctl in this run, with this run's h.
    L1's and L1-opt's table is official at b3b88b6 (the archive)."""
    out = ["## Gcd work against time, Q32 rows (rule item 12)", "",
           "A job where the Q32 row removes gcd work or multiplications against ctl (verdict win by the fixed bands of "
           "item 8) but has no time win against ctl is a **candidate for a wider-number test**, where both cost more. "
           "The counts are the row's own counting run, equal to its Q64 row's (checked, spec §18.3 item 1); the time "
           "verdict is this run's. L1's and L1-opt's table is official at b3b88b6 "
           f"([archived results]({ARCHIVE_LINK})). The Q32 rows are not compared with each other (rule item 1, "
           "revision), so L1-opt-Q32 against L1-Q32 has no time verdict and no row here.", "",
           "| job | X | Y | gcd work | multiplications | time | candidate |", "| --- | --- | --- | --- | --- | --- | --- |"]
    idx = {(r["job"], r["X"], r["Y"], r["measure"]): r for r in rows}
    cands = []
    for job in jobs:
        for X in X_Q32:
            Y = "ctl"
            if job in d.over:
                out.append(f"| {job} | {X} | {Y} | {OVER} | {OVER} | {OVER} | - |")
                continue
            ix = C.ROW_IMPL[X]
            cells, removes = [], False
            for name in ("gcd_work", "mul"):
                gx, gy = d.ctr(job, ix, name), d.ctr(job, "ctl", name)
                if gx is None or gy is None or gy == 0:
                    cells.append(NA)
                else:
                    v = fixed_verdict(gx / gy)
                    removes = removes or v == "win"
                    cells.append(f"{v} {fmt(gx / gy)}")
            t = idx.get((job, X, Y, "time"))
            tv = t["verdict"] if t else NA
            cand = removes and tv not in ("win", NA)
            if cand:
                cands.append(f"job {job} ({X} against {Y})")
            out.append(f"| {job} | {X} | {Y} | {cells[0]} | {cells[1]} | "
                       f"{(tv + ' ' + fmt(t['ratio'])) if t and t['ratio'] is not None else tv} | "
                       f"{'yes' if cand else ('n/a' if removes and tv == NA else 'no')} |")
    out += ["", "Candidates: " + ("; ".join(cands) if cands else "none") + ".", ""]
    return out


def changed_q64_section(rerun):
    """Rule, revision of Oct 29, 2026, safeguard 3: every Q64 verdict that differs between b3b88b6 and this run."""
    out = ["## Q64 verdicts that changed between the two runs (rule, revision of Oct 29, 2026)", "",
           f"The official Q64 verdicts are those of commit {C.OFFICIAL_Q64_COMMIT}. The Q64 rows were re-run here only "
           "for the paired comparison with the Q32 rows. Every (job, X, Y, measure) whose verdict differs between the "
           "two runs is listed, with both magnitudes; neither run is chosen over the other.", ""]
    off, src = official_q64()
    if off is None:
        return out + [f"n/a: the official verdicts of {C.OFFICIAL_Q64_COMMIT} could not be read: {src}.", ""]
    out += [f"Official verdicts read from {src} ([archived results]({ARCHIVE_LINK})).", ""]
    changed, same, missing = [], 0, 0
    for r in rerun:
        o = off.get((r["job"], r["X"], r["Y"], r["measure"]))
        if o is None:
            missing += 1
            continue
        if o["verdict"] == str(r["verdict"]):
            same += 1
        else:
            changed.append((r, o))
    out.append(f"{len(changed)} changed, {same} unchanged" + (f", {missing} not in {C.OFFICIAL_Q64_COMMIT}" if missing
                                                              else "") + ".")
    if changed:
        out += ["", f"| job | X | Y | measure | {C.OFFICIAL_Q64_COMMIT} verdict | ratio | this run's verdict | ratio |",
                "| --- | --- | --- | --- | --- | --- | --- | --- |"]
        for r, o in changed:
            ofr = float(o["ratio"]) if o["ratio"] not in ("", None) else None
            out.append(f"| {r['job']} | {r['X']} | {r['Y']} | {r['measure']} | {o['verdict']} | {fmt(ofr)} | "
                       f"{r['verdict']} | {fmt(r['ratio'])} |")
    return out + [""]


def model_section(d, st, jobs):
    """Rule, revision of Oct 29, 2026 (the model's prediction): bits per rational = H + E W/N, E = 64 + b_s S/W, with
    W from the generator and S from the growth rule, beside the measured B_used and B_heap. For B_used this is an
    accounting identity (it restates the memory report's sum), so its equality to the byte is a check of the report,
    not of the model; only B_heap tests anything."""
    out = ["## The model's bits per rational (rule, revision of Oct 29, 2026)", "",
           "bits per rational = H + E W/N, E = 64 + b_s S/W (H the rational's field bits; b_s the bits per lookup slot: "
           "L1 32, L1-opt 64, L1-Q32 16, L1-opt-Q32 48), with W from the generator and S from the growth rule, made "
           "before any run. **For B_used this is an accounting identity**: it restates the memory report's sum, so the "
           "\"identity\" column checks the report to the byte and tests nothing about the design. **Only B_heap "
           "tests anything:** it is measured by the C library's heap statistics, with capacity slack and allocator "
           "headers that the model leaves out.", "",
           "| job | when | row | W | N | S | S/W | E | predicted bits | B_used bits | identity | B_heap bits | "
           "B_heap / predicted |", "| --- |" * 1 + " --- |" * 12]
    g = st.get("gen", {})
    for job in jobs:
        m = g.get(str(job))
        if not m:
            continue
        for when, key in (("after entry", "after_entry"), ("end", "end")):
            W, N = m["reuse"][key]["W"], m["reuse"][key]["N"]
            S = C.lookup_slots(W)
            for impl in C.LAYER1:
                row = C.IMPL_ROWS[impl][0]
                E = 64 + C.SLOT_BITS[impl] * S / W
                pred = C.RAT_BITS[impl] + E * W / N
                if row in C.Q32_ROWS and job in d.over:
                    out.append(f"| {job} | {when} | {row} | {W} | {N} | {S} | {S / W:.3f} | {E:.1f} | {pred:.2f} | "
                               f"{OVER} | - | {OVER} | - |")
                    continue
                u, _ = d.mem_per(job, row, "used", key)
                hp, _ = d.mem_per(job, row, "heap", key)
                ident = NA if u is None else ("ok" if abs(8 * u - pred) < 1e-6 else "MISMATCH")
                out.append(f"| {job} | {when} | {row} | {W} | {N} | {S} | {S / W:.3f} | {E:.1f} | {pred:.2f} | "
                           f"{fmt(8 * u if u is not None else None, 2)} | {ident} | "
                           f"{fmt(8 * hp if hp is not None else None, 2)} | "
                           f"{fmt(8 * hp / pred if hp is not None else None)} |")
    return out + [""]


def strata_section(rows, d, jobs):
    over = [j for j in jobs if j in d.over]
    out = ["### Strata (rule item 10)", "",
           f"**Over-capacity jobs: {len(over)}** of {len(jobs)}" + (f" ({', '.join(map(str, over))})" if over else "") +
           ". **The Q32 strata cover fewer jobs** than the Q64 strata of b3b88b6: a stratum's over-capacity jobs are "
           "counted in the \"over capacity\" column and enter no other count or median.", "",
           "| stratum | jobs | over capacity | X | Y | measure | win | tie | loss | over cap | n/a | median ratio | "
           "severe flags |",
           "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for s in C.STRATA_ORDER:
        sj = [j for j in C.STRATA[s] if j in jobs]
        keys = sorted({(r["X"], r["Y"], r["measure"]) for r in rows if r["job"] in sj})
        for X, Y, meas in keys:
            rs = [r for r in rows if r["job"] in sj and (r["X"], r["Y"], r["measure"]) == (X, Y, meas)]
            cnt = {k: sum(1 for r in rs if r["verdict"] == k) for k in ("win", "tie", "loss")}
            oc = sum(1 for r in rs if r["verdict"] == OVER)
            cap = sum(1 for r in rs if "over cap" in str(r["verdict"]) and r["verdict"] != OVER)
            na = sum(1 for r in rs if r["verdict"] == NA)
            rat = [r["ratio"] for r in rs if r["ratio"] is not None]
            sev = sum(1 for r in rs if r.get("severe"))
            out.append(f"| {s} | {len(sj)} | {oc} | {X} | {Y} | {meas} | {cnt['win']} | {cnt['tie']} | {cnt['loss']} | "
                       f"{cap} | {na} | {fmt(float(np.median(rat)) if rat else None)} | {sev} |")
    return out + [""]


def reported_sections(d, st, jobs, outdir):
    out = ["## Also reported (rule item 12)", "",
           f"The time-dependent items of rule item 12 for L1 and L1-opt (gcd work against time) are official at commit "
           f"{C.OFFICIAL_Q64_COMMIT} ([archived results]({ARCHIVE_LINK})); the Q32 rows' are above. The counts below "
           "are deterministic and equal to that run's.", ""]
    # memory against reuse
    mrows = ("L1", "L1-opt", "L1-Q32", "L1-opt-Q32", "ctl", "fmpq", "mpq", "fz")
    out += ["### Memory against reuse (jobs by W/N at the end; bytes per stored rational, end)", "",
            "| job | W/N | " + " | ".join(f"{r} used / heap" for r in mrows) +
            " | L1 formula check |", "| --- | --- |" + " --- |" * (len(mrows) + 1)]
    g = st["gen"]
    for job in sorted(jobs, key=lambda j: g[str(j)]["reuse"]["end"]["W_per_N"]):
        cells = []
        for r in mrows:
            if r == "fz" and not C.JOBS[job]["fz"]:
                cells.append("-")
                continue
            if r in C.Q32_ROWS and job in d.over:
                cells.append(OVER)
                continue
            u, _ = d.mem_per(job, r, "used")
            hp, _ = d.mem_per(job, r, "heap")
            cells.append(f"{fmt(u, 1)} / {fmt(hp, 1)}")
        chk = NA
        c = d.counts.get((job, "L1"))
        if c and "mem" in c:
            mem = {l.split()[2]: int(l.split()[-2]) for l in c["mem"] if l.startswith("mem end")}
            N = next((int(l.split()[2]) for l in c["mem"] if l.startswith("memN end")), 0)
            W, S = mem.get("whole", 0) // 8, mem.get("lookup", 0) // 4
            u, _ = d.mem_per(job, "L1", "used")
            if N and u is not None:
                chk = "ok" if abs(u - (8 + (8 * W + 4 * S) / N)) < 1e-9 else "MISMATCH"
        out.append(f"| {job} | {g[str(job)]['reuse']['end']['W_per_N']:.3f} | " + " | ".join(cells) + f" | {chk} |")
    # per case
    out += ["", "### Per case (rule items 6, 12): operations, and gcds, gcd work and multiplications against ctl", "",
            "Each operation's core counts attributed to its predicted case (spec §14.1 item 3); add and subtract "
            "together. The `den` and `general` rows of L1 against ctl are 1 exactly. No verdict.", "",
            "| job | case | n | L1 gcds / ctl | ratio | L1 gcd work / ctl | ratio | L1 mul / ctl | ratio | "
            "L1-opt gcds / L1 | L1-opt gcd work / L1 | L1-opt mul / L1 |",
            "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]

    def rr(a, b):
        return "0 against 0" if a == 0 and b == 0 else (f"{a / b:.3f}" if b else NA)
    for job in jobs:
        pc = st.get("percase", {}).get(str(job))
        if not pc:
            continue
        for k in C.CASES:
            l, c, o = pc["L1"].get(k), pc["ctl"].get(k), pc["L1opt"].get(k)
            if not l:
                continue
            out.append(f"| {job} | {k} | {l['n']} | {l['gcd']} / {c['gcd']} | {rr(l['gcd'], c['gcd'])} | "
                       f"{l['gcd_work']} / {c['gcd_work']} | {rr(l['gcd_work'], c['gcd_work'])} | "
                       f"{l['mul']} / {c['mul']} | {rr(l['mul'], c['mul'])} | "
                       f"{o['gcd']} / {l['gcd']} ({rr(o['gcd'], l['gcd'])}) | "
                       f"{o['gcd_work']} / {l['gcd_work']} ({rr(o['gcd_work'], l['gcd_work'])}) | "
                       f"{o['mul']} / {l['mul']} ({rr(o['mul'], l['mul'])}) |")
    # tricks
    pred_all = st.get("predicted_tricks", {})
    out += ["", "### L1-opt's tricks per job (spec §17.2), with the predicted figures and the share of L1's work "
            "removed", "",
            "Each counter is summed over entry and the operations, beside the generator's prediction (spec §17.3 item 2; "
            "O-1 to O-5 must equal it exactly, by the gate; O-6 and O-7 are reported, not predicted). The share removed "
            "is the trick's skips over L1's count of the same work, entry and operations.", "",
            f"*Limitation:* {ONE_ROW_NOTE}.", "",
            "| job | trick | counters (predicted) | share of L1's work removed |", "| --- | --- | --- | --- |"]
    for job in jobs:
        if all(d.ctr(job, "L1opt", n) is None for t in TRICKS for n in TRICKS[t]):
            continue
        pred = {}
        for ph, dd in pred_all.get(str(job), {}).items():
            if ph in C.PHASES:
                for n, v in dd.items():
                    pred[n] = pred.get(n, 0) + v
        totals = {}
        for t, names in TRICKS.items():
            cells = []
            for n in names:
                v = d.ctr(job, "L1opt", n)
                p = f" ({pred.get(n, 0)})" if t in PREDICTED_TRICKS and str(job) in pred_all else ""
                cells.append(f"{n} {fmt(v)}{p}")
            rem = []
            for what, base, sk in REMOVED.get(t, []):
                b = d.ctr(job, "L1", base)
                v = sum(d.ctr(job, "L1opt", n) or 0 for n in sk)
                totals[what] = totals.get(what, 0) + v
                rem.append(f"{what} {v / b:.3f}" if b else f"{what} {NA} (L1 has 0)")
            out.append(f"| {job} | {t} | {', '.join(cells)} | {', '.join(rem) or '-'} |")
        tot = []
        for what, base in (("lookups", "lookup_search"), ("gcds", "gcd"), ("gcd work", "gcd_work"),
                           ("divisions", "div_exact"), ("multiplications", "mul"), ("additions", "add")):
            b = d.ctr(job, "L1", base)
            tot.append(f"{what} {totals.get(what, 0) / b:.3f}" if b else f"{what} {NA}")
        out.append(f"| {job} | all | | {', '.join(tot)} |")
    # entry against operations; store's share
    out += ["", "### Entry against operations; store's share (rule item 12)", "",
            "Per row, the share of the median total time spent in entry. For L1 and L1-opt, the share of gcd work and "
            "of lookups counted in `store` (entry and operations). `store` does no arithmetic (spec §6: it only "
            "interns the reduced numerator and denominator), so its share of gcd work is 0 by construction: every gcd, "
            "entry's reduction included, is counted in the core. Likewise every lookup is made by `store`'s intern, so its "
            "share of lookups is 1 by construction. Both are computed anyway, so a departure would show.",
            ""]
    for job in jobs:
        cells = []
        for row in ("L1", "L1-opt", "L1-Q32", "L1-opt-Q32", "ctl", "fmpq", "mpq") + \
                (("fz",) if C.JOBS[job]["fz"] else ()):
            e, _ = d.times(job, row, "entry")
            t, _ = d.times(job, row, "total")
            if e is not None and t is not None:
                cells.append(f"{row} {np.median(e) / np.median(t):.3f}")
        st_cells = []
        for impl in ("L1", "L1opt"):
            gs, gt = d.ctr(job, impl, "gcd_work", part="store"), d.ctr(job, impl, "gcd_work")
            ks, kt = d.ctr(job, impl, "lookup_search", part="store"), d.ctr(job, impl, "lookup_search")
            if gt is None or kt is None:
                continue
            g_txt = ("0 (by construction)" if gs == 0 else f"{gs / gt:.3f} (NOT 0: store did arithmetic)") \
                if gt else "0 against 0"
            k_txt = f"{ks / kt:.3f}" if kt else "0 against 0"
            st_cells.append(f"{C.IMPL_ROWS[impl][0]} store share of gcd work {g_txt}, of lookups {k_txt}")
        out.append(f"- job {job}: entry share " + ("; ".join(cells) or NA) +
                   ("; " + "; ".join(st_cells) if st_cells else ""))
    # counts per operation
    out += ["", "### Counts per operation (rule item 12)", "",
            "Totals per phase (entry, then each operation), core and store together; every counter per phase, core and "
            "store is in `counters.csv`.", "",
            "| job | row | phase | n | " + " | ".join(OP_COUNTS) + " | cases |",
            "| --- | --- | --- | --- |" + " --- |" * (len(OP_COUNTS) + 1)]
    for job in jobs:
        nph = st.get("gen", {}).get(str(job), {}).get("phase_counts")
        for impl in ("L1", "L1opt", "ctl"):          # the Q32 rows' equal their Q64 rows' (spec §18.3, checked)
            if d.counts.get((job, impl)) is None or "counts" not in d.counts[(job, impl)]:
                continue
            for ph in C.PHASES:
                vals = [d.ctr(job, impl, n, phases=(ph,)) for n in OP_COUNTS]
                cases = {k: d.ctr(job, impl, "case_" + k, phases=(ph,)) for k in C.CASES_OF.get(ph, ())}
                if not any(vals) and not any(cases.values()):
                    continue
                cs = ", ".join(f"{k.split('_', 1)[1]} {v}" for k, v in cases.items() if v) or "-"
                n = (nph or {}).get(ph, NA)
                out.append(f"| {job} | {C.IMPL_ROWS[impl][0]} | {ph} | {n} | " + " | ".join(fmt(v) for v in vals) +
                           f" | {cs} |")
    out += ["", "### Chosen forms (fmpq, mpq)", ""]
    for k, f in sorted(st.get("forms", {}).items(), key=lambda kv: (int(kv[0].split(":")[0]), kv[0])):
        out.append(f"- {k}: form {f['form']} (median A {f['median_A'] / 1e6:.3f} ms, B {f['median_B'] / 1e6:.3f} ms)")
    write_counts_csv(outdir, d, jobs)
    write_rounds_csv(outdir, st)
    return out + [""]


def write_csv(path, rows):
    if not rows:
        Path(path).write_text("")
        return
    keys = list(rows[0].keys())
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        for r in rows:
            w.writerow(r)


def write_rounds_csv(outdir, st):
    rows = [dict(job=r["job"], row=r["row"], round=r["round"], pos=r["pos"], r=r.get("r"), form=r.get("form"),
                 total=r.get("total"), entry=r.get("entry"), ops=r.get("ops"), read=r.get("read"),
                 overcap=r.get("overcap"), label=r.get("label"))
            for r in st.get("records", []) if r["kind"] == "time"]
    write_csv(outdir / "rounds.csv", rows)


def write_counts_csv(outdir, d, jobs):
    rows = []
    for job in jobs:
        for impl in C.COUNTED:
            c = d.counts.get((job, impl))
            if not c or "counts" not in c:
                continue
            for line in c["counts"]:
                t = line.split()
                rows.append(dict(job=job, impl=impl, kind=t[0], phase=t[1], part=t[2], name=t[3], value=int(t[4])))
    write_csv(outdir / "counters.csv", rows)


if __name__ == "__main__":
    report(sys.argv[1])
