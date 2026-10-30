"""Tests of round 3's harness (not measurements; nothing here is timed or reported as a time). From round 2's
test_r2_harness.py (unchanged), with every test carried over and adapted to round 3's rows, and new ones.

    python3 test_r3_harness.py            the self-tests: the fixed rule and its pin, the 08 module and tc.c, the
                                          official data (rounds 1 and 2), the workloads, the model against round 2's,
                                          the two Q32 rows' sources against round 2's q32.c (arithmetic identical,
                                          only storage replaced) and their allocations (only the named structures),
                                          the binaries' exact checks on toy workloads (the three rows; Q32-index's
                                          list and Q32-hash's table; the call-for-call checks of every pairing), the
                                          full table, the profiling build's results and accounting (create's batch
                                          figure), the slowdown limit, statistics, the pilot's projection with P_idx
                                          and its stops, fork/exec with the affinity set before exec and the readings
                                          around it, the kernel timer, the idle check, the disturbance check, the
                                          timed block's machinery with a stand-in driver (four rows, rotated; create
                                          in the creation part), the runner's ticks per launch, and the analysis (three
                                          pairings, two time measures with their own bands, storage, speed against
                                          fill)
    python3 test_r3_harness.py --full     also every gate and counting run on the full-size workloads of the 8 jobs
                                          (test and counting builds; untimed): every counter and storage figure
                                          against the prediction, the read-outs, and the call-for-call checks
"""
import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import r3_analyze as A
import r3_checks as K
import r3_common as C
import r3_gen
import r3_libconf as L
import r3_runner as RN

TB = C.HERE / "build" / "r3test"
BIN = TB / "bin"


def ns(**kw):
    d = dict(toy=True, jobs="", pilot_only=False, hard_stop_s=None, resume=False, rerun=None)
    d.update(kw)
    return argparse.Namespace(**d)


def build_once():
    if (BIN / ".built").exists() and (BIN / ".built").read_text() == json.dumps(L.code_hashes()):
        return
    BIN.mkdir(parents=True, exist_ok=True)
    for out, cmd in L.build_commands(BIN):
        out.parent.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(cmd, capture_output=True, text=True)
        assert r.returncode == 0, f"{' '.join(cmd)}\n{r.stderr}"
    (BIN / ".built").write_text(json.dumps(L.code_hashes()))


def toy_work():
    w = TB / "work"
    if not (w / "gen.json").exists():
        r3_gen.generate(w, list(C.RUN_JOBS), toy=True)
    return w


# ------------------------------------------------------------------------------------------------- the rule, data
def test_rule_fixed_and_pinned():
    """the rule on disk is the pinned, approved one (no PROPOSED mark); a rule with a PROPOSED mark, or another text,
    is refused for a measured run"""
    text = C.RULE_FILE.read_text()
    assert C.sha256(C.RULE_FILE) == C.RULE_SHA256, "rule-r3.md differs from the pinned hash"
    assert C.PROPOSAL_MARK not in text and C.rule_check() is None, C.rule_check()
    assert "**Fixed: approved by the author, Oct 29, 2026**" in text
    assert "**Gcd work: a check, not a verdict** (carried over)" in text
    assert "**The factor 42 (approved by the author, Oct 29, 2026)**" in text
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule-r3.md"
        p.write_text(text + " ")
        assert "sha256" in C.rule_check(p)
        p.write_text(text.replace("approved by the author, Oct 29, 2026", "PROPOSED", 1))
        assert "PROPOSED" in C.rule_check(p)
    for pin in ("0" * 64, None):
        saved, savedf = C.RULE_SHA256, C.RULE_FILE
        try:
            if pin:
                C.RULE_SHA256 = pin
            else:                                          # the approved text with a PROPOSED mark put back
                tf = Path(tempfile.mkdtemp()) / "rule-r3.md"
                tf.write_text(text.replace("approved by the author, Oct 29, 2026", "PROPOSED", 1))
                C.RULE_FILE = tf
            try:
                RN.Runner(ns(toy=False)).preflight()
            except SystemExit as e:
                assert "refusing a measured run" in str(e)
            else:
                raise AssertionError("a measured run under another rule was not refused")
        finally:
            C.RULE_SHA256, C.RULE_FILE = saved, savedf
def test_vendor08_and_official_data():
    assert L.vendor08_check() is None
    assert L.tc_check() is None, L.tc_check()            # tc.c and tc.h as at round 2 (rule-r3 item 2)
    with tempfile.TemporaryDirectory() as t:             # a changed tc.c is refused
        saved = L.LIBDIR
        try:
            fake = Path(t)
            (fake / "src").mkdir()
            (fake / "include").mkdir()
            (fake / "src" / "tc.c").write_bytes((saved / "src" / "tc.c").read_bytes() + b"\n")
            (fake / "include" / "tc.h").write_bytes((saved / "include" / "tc.h").read_bytes())
            L.LIBDIR = fake
            assert "tc.c" in L.tc_check()
        finally:
            L.LIBDIR = saved
    sums = dict(reversed(l.split()) for l in (L.V08 / "SHA256SUMS").read_text().splitlines())
    assert sums == L.VENDOR08_SHA256
    assert C.archive_check() is None, C.archive_check()
    for d in C.PROTECTED:
        assert C.protected(d) and C.protected(d / "results.md")
        try:
            RN.Runner(ns(toy=True)).__class__  # the runner refuses to start there
            r = RN.Runner(ns(toy=True))
            r.out = d
            r.start()
        except SystemExit as e:
            assert "official data" in str(e)
        else:
            raise AssertionError(f"the runner would write into {d}")
        try:
            A.report(d)
        except SystemExit as e:
            assert "official data" in str(e)
        else:
            raise AssertionError(f"the analysis would write into {d}")
    assert C.RESULTS_STOPPED in C.PROTECTED and C.RESULTS_STOPPED_2 in C.PROTECTED   # both stopped runs' records
    assert C.RESULTS_R2 in C.PROTECTED and len(C.PROTECTED[C.RESULTS_R2]) == 6       # round 2's complete run
    assert sorted(p.name for p in C.RESULTS_R2.iterdir()) == sorted(C.PROTECTED[C.RESULTS_R2])
    assert not C.protected(C.RESULTS) and C.RESULTS.name == "results-r3"


def test_workloads_identical_to_round1():
    with tempfile.TemporaryDirectory() as t:
        s = r3_gen.generate(t, [3, 10, 5])
        assert s["3"]["identical_to_round1"] and s["10"]["identical_to_round1"] and s["5"]["identical_to_round1"]
        assert s["5"]["over_capacity"] and not s["3"]["over_capacity"]


def test_runner_imports_no_numpy():
    r = subprocess.run([sys.executable, "-c", "import sys, r3_runner; print('numpy' in sys.modules)"], cwd=C.HERE,
                       capture_output=True, text=True)
    assert r.stdout.strip() == "False", r.stdout + r.stderr


# ------------------------------------------------------------------------------------------------- the binaries
def test_gates_and_counts_toy():
    """every toy job: the test build's gate of each row passes its exact checks against the model (Q32-index's list,
    Q32-hash's table, the storage figures); the counting runs of each Q32 row have TC's arithmetic, call for call, and
    its gcd work; the two Q32 rows' stores are equal, call for call; the memory runs read out the reference"""
    build_once()
    w = toy_work()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for job in C.RUN_JOBS:
            pred = json.loads((w / f"job-{job:02d}.r3.json").read_text())["predicted"]
            G, counts = {}, {}
            for impl in C.IMPLS:
                p = subprocess.run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out",
                                    t / "w.bin"], capture_output=True, text=True)
                assert p.returncode == 0, p.stdout[-500:] + p.stderr
                assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
                ws = C.sha256(t / "w.bin") if impl in C.Q_IMPLS else None
                if impl == "Q32H":
                    assert (t / "w.bin").stat().st_size == 8 * C.SLOTS
                fails = K.exact_checks(impl, p.stdout.splitlines(), pred, ws)
                assert not fails, (job, impl, fails[:5])
                p = subprocess.run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                                    t / f"p-{impl}.bin", t / f"s-{impl}.bin"], capture_output=True, text=True)
                assert p.returncode == 0
                G[impl] = K.gcd_work(p.stdout.splitlines())
                counts[impl] = {k: v for k, v in K.parse_counts(p.stdout.splitlines()).items()
                                if k[1] not in K.STORAGE_OF}
                p = subprocess.run([BIN / f"bench-{impl}-timing", "mem", w / f"job-{job:02d}.wl", t / "m.out"],
                                   capture_output=True, text=True)
                assert p.returncode == 0 and "memrun end" in p.stdout
                assert K.readout_check(t / "m.out", w / f"job-{job:02d}.ref") is None
            for i in C.Q_IMPLS:
                assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (job, i)
                assert G[i] == G["TC"] > 0
            assert (t / "s-Q32I.bin").read_bytes() == (t / "s-Q32H.bin").read_bytes(), job
            assert any((t / "s-Q32I.bin").read_bytes()) and not any((t / "s-TC.bin").read_bytes())
            assert counts["Q32I"] == counts["Q32H"], job
def test_exact_checks_catch_a_difference():
    build_once()
    w = toy_work()
    pred = json.loads((w / "job-03.r3.json").read_text())["predicted"]
    with tempfile.TemporaryDirectory() as t:
        for impl in C.Q_IMPLS:
            p = subprocess.run([BIN / f"bench-{impl}-test", "gate", w / "job-03.wl", Path(t) / "g.out",
                                Path(t) / "w.bin"], capture_output=True, text=True)
            lines = p.stdout.splitlines()
            ws = C.sha256(Path(t) / "w.bin")
            assert not K.exact_checks(impl, lines, pred, ws)
            bad = json.loads(json.dumps(pred))
            bad["counters"]["mul"]["k15_ops"] = bad["counters"]["mul"].get("k15_ops", 0) + 1
            assert K.exact_checks(impl, lines, bad)
            assert K.exact_checks(impl, lines, dict(pred, W_end=pred["W_end"] + 1))
            own = "examined" if impl == "Q32I" else "probes"
            bad = json.loads(json.dumps(pred))
            bad["counters"]["entry"][own] += 1                  # the row's own storage counter, per phase
            assert any(own in f for f in K.exact_checks(impl, lines, bad))
            bad = json.loads(json.dumps(pred))
            bad["stor_end"][own] += 1                           # and its total at the end
            assert any("storage end" in f for f in K.exact_checks(impl, lines, bad))
            assert K.exact_checks(impl, lines, pred, "0" * 64)  # the list, or the table, differs
            assert K.exact_checks(impl, [l for l in lines if not l.startswith("stor ")], pred, ws)
        # each row's storage counters are its own: a probe count in Q32-index is a failure
        p = subprocess.run([BIN / "bench-Q32I-test", "gate", w / "job-03.wl", Path(t) / "g.out", Path(t) / "w.bin"],
                           capture_output=True, text=True)
        lines = p.stdout.splitlines() + ["sctr add store probes 1"]
        assert any("probes" in f for f in K.exact_checks("Q32I", lines, pred, C.sha256(Path(t) / "w.bin")))
        # a read-out that differs in one value is caught, with the call
        ref = (w / "job-03.ref").read_bytes()
        broken = bytearray(ref)
        broken[8 * 5] ^= 1
        (Path(t) / "x.out").write_bytes(bytes(broken))
        r = K.readout_check(Path(t) / "x.out", ref)
        assert r and r["call"] == 5 and r["mismatches"] == 1
def test_profiling_build():
    """The profiling build changes no result (its read-out is the reference), samples about one call in K with random
    gaps, attributes every sampled call to a job phase and case, charges every growth to M, and reports create's batch
    time; the slowdown limit flags an invocation over 1.5. No time is reported."""
    build_once()
    w = toy_work()
    with tempfile.TemporaryDirectory() as t:
        for impl in C.IMPLS:
            p = subprocess.run([BIN / f"bench-{impl}-profiling", "prof", w / "job-03.wl", Path(t) / "p.out", "2", "3",
                                "8", str(C.PROF_SEED)], capture_output=True, text=True)
            assert p.returncode == 0, p.stdout[-300:] + p.stderr
            assert K.readout_check(Path(t) / "p.out", w / "job-03.ref") is None
            pr = K.parse_prof(p.stdout.splitlines())
            assert pr["K"] == 8 and pr["probe_cost"] > 0 and pr["tsc_hz"] > 1e8
            n = C.wl_header(w / "job-03.wl")[1]
            for i, rep in pr["reps"].items():
                calls = 2 * 2 * n                           # r = 2 runs; each run: n calls and n read-outs
                assert abs(rep["sampled"] - calls / 8) < 0.25 * calls / 8, (rep["sampled"], calls / 8)
                assert sum(c for c, o in rep["calls"].values()) == rep["sampled"]
                for (jp, k), (c, outer) in rep["calls"].items():
                    inner = sum(v[0] for (j2, k2, ph), v in rep["cyc"].items() if (j2, k2) == (jp, k))
                    assert outer >= inner > 0
                assert any(k == "read" for (jp, k) in rep["calls"]) and any(k == "entry" for (jp, k) in rep["calls"])
                assert rep["m"], "growths are charged to M"
                assert 0 < rep["create"] < rep["entry"]          # create's batch figure, inside the creation part
            # every phase the row has, seen; TC has no I
            seen = {ph for rep in pr["reps"].values() for (jp, k, ph) in rep["cyc"]}
            assert {0, 1, 2, 3, 5} <= seen and ((4 in seen) == (impl in C.Q_IMPLS))
        for b in ("timing", "counting", "test"):                # not built in the other builds
            p = subprocess.run([BIN / f"bench-Q32H-{b}", "prof", w / "job-03.wl", Path(t) / "p.out", "1", "1", "8",
                                "1"], capture_output=True, text=True)
            assert p.returncode != 0
    s, ok = K.slowdown(1.6, 1.0)
    assert s == 1.6 and not ok
    s, ok = K.slowdown(1.5, 1.0)
    assert ok
    assert C.PROF_SLOWDOWN_MAX == 1.5 and C.PROF_K == 32 and C.PROF_INVOCATIONS == 3
def test_statistics_and_verdicts():
    a = [10.0] * 12
    r, lo, hi = A.boot_ratio(a, a)
    assert r == lo == hi == 1.0
    r, lo, hi = A.boot_ratio([20.0] * 12, a)
    assert r == lo == hi == 2.0
    assert A.time_verdict(1.2, 1.3, 0.05) == "win" and A.time_verdict(0.7, 0.8, 0.05) == "loss"
    assert A.time_verdict(0.97, 1.2, 0.05) == "tie"
    assert A.fixed_verdict(0.90) == "win" and A.fixed_verdict(1.10) == "loss" and A.fixed_verdict(1.0) == "tie"
    assert C.SEVERE == 2.0 and C.BAND_MIN == 0.05 and C.BOOT == 10_000 and C.BOOT_SEED == 20261029


def test_pilot_projection_and_allowance():
    assert K.pilot_projection(60, 30, 0, 1) == 60 + 300 + 36
    assert K.pilot_projection(60, 60, 0, 1) == 60 + 600 + 36
    assert K.pilot_projection(60, 30, 2, 1) == 60 + 300 + 84 + 36       # 42 x P_idx (rule-r3, Run plan step 3)
    assert C.PILOT_IDX_FACTOR == 42 and C.PILOT_PROF_FACTOR == 36
    assert K.host_allowance(0.5, 10) == 5 + 10 and K.host_allowance(0.0, 10) == 5
    assert K.host_allowance(1.25, 10) == 5 + 25 and C.HOST_RATE_CEILING == 4.0
    assert C.HOST_BASE_TICKS == 5 and C.HOST_FACTOR == 2 and C.HOST_RATE_MIN_TICKS == 1
    assert K.other_limit(1.0) == 6 and K.other_limit(10.0) == 15 and K.other_limit(0.3) == 6
    assert C.PILOT_LIMIT_S == 22 * 60 and C.HARD_STOP_S == 30 * 60 and C.PILOT_JOBS == (1, 3, 10)
def pilot_runner(rates, slow, P_extra=0.0, idx_s=0.01):
    """a runner whose pilot blocks are stand-ins; rates: one host rate for every block, or a list of
    {host pid: (ticks, seconds)} per block, in block order; idx_s: the readings around fork and waitpid of each
    Q32-index launch (calibration and rounds)"""
    r = RN.Runner(ns(jobs="1,3,10"))
    r.out = Path(tempfile.mkdtemp())
    r.state_file = r.out / "state.json"
    r.state = {"elapsed_before": 0.0, "disturbance_checks": []}
    r.t0 = time.monotonic() - P_extra
    r.save = lambda: None
    r.log = lambda m: None

    blocks = iter(rates) if isinstance(rates, list) else None

    def block(job, kind, label, pilot=False, rows=None):
        assert pilot and label == "pilot"
        if blocks is None:
            hosts, sec = {"1": {"name": "init", "ticks": 0, "rate": rates}}, 1.6
        else:
            b = next(blocks)
            hosts = {p: {"name": f"host{p}", "ticks": t, "rate": t / sec} for p, (t, sec) in b.items()}
            sec = max(sec for _, sec in b.values())
        r.state["disturbance_checks"].append({"label": "pilot", "hosts": hosts, "seconds": sec})

        def ls(row):
            return idx_s if row == "Q32-index" else 0.001
        if kind == "rounds":
            return [{"role": "calibration", "row": C.IMPL_ROW[i], "job": job, "r": 4, "reps": 3, "block": "pilot",
                     "launch_s": ls(C.IMPL_ROW[i])} for i in C.IMPLS] + \
                   [{"role": "round", "row": row, "job": job, "total": 100.0, "block": "pilot", "launch_s": ls(row)}
                    for row in C.ROWS for _ in range(2)]
        return [{"role": "profile", "row": row, "job": job, "total": 100.0 * slow, "block": "pilot"}
                for row in C.PROF_ROWS]
    r.timed_block = block
    return r
def test_pilot_stops():
    r = pilot_runner(0.1, 1.2)
    r.step3_pilot()                                       # goes on
    assert set(r.state["pilot"]["slowdown"]) == set(C.PROF_ROWS)
    assert r.state["pilot"]["slowdown"]["Q32-index"]["slowdown"] == 1.2
    # P_idx: Q32-index's launches in the round blocks, calibration included: 3 jobs x (1 + 2) launches x 0.01 s
    assert abs(r.state["pilot"]["P_idx_s"] - 9 * 0.01) < 1e-9, r.state["pilot"]
    assert r.state["pilot"]["P_rest_s"] == max(r.state["pilot"]["P_s"] - r.state["pilot"]["P_idx_s"], 0.0)
    r = pilot_runner(1.25, 1.2)                           # a host rate under the ceiling: goes on
    r.step3_pilot()
    r = pilot_runner(4.0, 1.2)                            # at the ceiling of 4 ticks per second: goes on
    r.step3_pilot()
    r = pilot_runner(0.0, 1.2)                            # no tick in 1.6 s: R_h is 1 tick over 1.6 s, and goes on
    r.step3_pilot()
    assert r.state["host_rates"] == {"1": 1 / 1.6} and r.state["host_rates_measured"] == {"1": 0.0}
    # Q32-index's launches of 4 s each: 9 x 4 = 36 s, x 42 = 25.2 min > 22 min: the projection stops the run
    for rates, slow, extra, idx, why in ((4.5, 1.2, 0, 0.01, "ceiling"), (0.1, 1.6, 0, 0.01, "slowdown"),
                                         (0.1, 1.2, 20 * 60, 0.01, "projected"), (0.1, 1.2, 0, 4.0, "projected")):
        r = pilot_runner(rates, slow, extra, idx)
        try:
            r.step3_pilot()
        except RN.RunStop as e:
            assert why in str(e), str(e)
            assert r.state["pilot_stopped"]
        else:
            raise AssertionError(f"the pilot did not stop the run ({why})")
def test_host_allowance_from_pilot():
    """the host allowance is computed from the pilot's data: R_h is each host's highest rate over the pilot's blocks
    (three rounds blocks and the profiling block), but at least 1 tick over the longest pilot block, and over a block
    of L seconds the allowance is 5 + ceil(2 R_h L)"""
    per_block = [{"91": (2, 1.6), "1": (1, 1.6), "7": (0, 1.6)}, {"91": (1, 1.6), "1": (0, 1.6), "7": (0, 1.6)},
                 {"91": (0, 1.5), "1": (0, 1.5), "7": (0, 1.5)}, {"91": (1, 2.5), "1": (0, 2.5), "7": (0, 2.5)}]
    r = pilot_runner(per_block, 1.2)
    r.step3_pilot()                                       # all under the ceiling: goes on
    assert r.state["host_rates"] == {"91": 1.25, "1": 0.625, "7": 0.4}, r.state["host_rates"]   # 7: 1 tick / 2.5 s
    assert r.state["host_rates_measured"]["7"] == 0.0
    assert r.state["pilot"]["host_rates"] == r.state["host_rates"]
    assert r.allowance_for(10.0) == {91: 5 + 25, 1: 5 + 13, 7: 5 + 8}
    assert r.allowance_for(2.0) == {91: 5 + 5, 1: 5 + 3, 7: 5 + 2}
    s0 = snap({91: ("claude", 1, 100, 50, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 0.0)
    ok = snap({91: ("claude", 1, 120, 60, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 10.0)   # 30 ticks
    over = snap({91: ("claude", 1, 121, 60, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 10.0)  # 31 ticks
    d = RN.disturbance(s0, ok, 4242, 0, {91, 1}, r.allowance_for(10.0))
    assert not d["disturbed"] and d["hosts"]["91"]["allowance"] == 30, d
    assert RN.disturbance(s0, over, 4242, 0, {91, 1}, r.allowance_for(10.0))["disturbed"]


# ------------------------------------------------------------------------------------------------- round 3's rows
ARITH_FUNCS = ("rd", "rdset", "gcdp", "store", "enter", "operand", "begin", "compare", "addsub", "other", "one", "mul",
               "div", "read")
NEW_FUNCS = ("other",)   # added by the amendment of Oct 29, 2026 (spec v0.3 §17.3): not in round 2's q32.c


def c_function(src, name, prefix):
    """the text of a C function (by its name, with or without the row's prefix) from its line to its closing brace"""
    import re
    m = re.search(r"^[^\n;]*\b(" + re.escape(prefix + name) + "|" + re.escape(name) + r")\(([^)]*)\)[^;\n]*\{\n",
                  src, re.M)
    assert m, name
    i, depth = m.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(src[i], 0)
        i += 1
    return src[m.start():i]


def strip_audit(src):
    """the source without its #ifdef SK_AUDIT ... #endif blocks (the test build's audits and introspection)"""
    out, depth, skip = [], 0, 0
    for line in src.splitlines():
        t = line.strip()
        if t.startswith("#if"):
            depth += 1
            if skip == 0 and t == "#ifdef SK_AUDIT":
                skip = depth
                continue
        if skip:
            if t.startswith("#endif") and depth == skip:
                skip = 0
            if t.startswith("#endif"):
                depth -= 1
            elif t.startswith("#else") and depth == skip:
                skip = 0
            continue
        if t.startswith("#endif"):
            depth -= 1
        out.append(line)
    return "\n".join(out)


def test_rows_identical_outside_storage():
    """spec v0.3 §5, §16 reviewer item 4, as amended Oct 29, 2026 (§17): q32i.c and q32h.c are the same text outside
    whole-number storage (every function of the arithmetic, the cases, the known-answer skips and the store, but the
    store's two test-build hint checks in Q32-hash: a slot, not an index below W), and both add to round 2's q32.c
    only the amendment's cases and tests; round 2's q32.c, plk.c and tc.c are unchanged (retired, or round 2's record),
    and neither new row includes plk.h. TC (tc3.c) is round 2's tc.c with the same new cases."""
    import difflib
    src = L.SRC
    qi, qh = (src / "q32i.c").read_text(), (src / "q32h.c").read_text()
    for f, new in (("q32i.c", qi), ("q32h.c", qh)):
        assert "plk.h" not in new and "l1q_plk" not in new and "lookup" not in strip_audit(new).lower().replace(
            "no lookup", "")
    for name in ARITH_FUNCS:
        assert c_function(qi, name, "q32i_") == c_function(qh.replace("q32h_", "q32i_"), name, "q32i_"), name
    for name in ("add", "sub"):
        line = next(l for l in qi.splitlines() if l.startswith(f"sk_status q32i_{name}("))
        assert line.replace("q32i_", "q32h_") in qh.splitlines(), name
    sa, sb = c_function(qi, "store_", "q32i_"), c_function(qh, "store_", "q32h_")
    diff = [(x, y) for x, y in zip(sa.splitlines(), sb.splitlines()) if x != y]
    assert len(sa.splitlines()) == len(sb.splitlines()) and len(diff) == 2, diff
    assert all("c->nW" in x and "SLOTS" in y for x, y in diff), diff
    # against round 2's q32.c: only lines added, and each added line belongs to the amendment (a new case, the integer
    # and zero tests, the fixed handles); no line of round 2's arithmetic is removed but the comparison count, the
    # struct of comparisons, division's zero test (by handle) and K-2's note for 1/1 - 1/1 (now int_both)
    old = (src / "q32.c").read_text().replace("q32_", "q32i_")
    removed = []
    for name in (f for f in ARITH_FUNCS if f not in NEW_FUNCS):
        a_, b_ = c_function(old, name, "q32i_").splitlines(), c_function(qi, name, "q32i_").splitlines()
        removed += [l[1:].strip() for l in difflib.unified_diff(a_, b_, lineterm="", n=0)
                    if l.startswith("-") and not l.startswith("---")]
    allowed = ("XC(c, cmp, six ? 6 : 4);", "if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = false;",
               "if (v[2] == 0) return SK_EDIVZERO;", "KC(c, 2, mul, 3);",
               "if (q.ad || q.cb) { /* +-1/1 -+ 1/1: without K-2, case cross */")
    assert all(l in allowed for l in removed), [l for l in removed if l not in allowed]
    tc2, tc3 = (src / "tc.c").read_text(), (src / "tc3.c").read_text()
    for name in ("store", "enter", "operand", "begin", "read"):
        assert c_function(tc2, name, "tc_") == c_function(tc3, name, "tc_"), name
    for f in ("q32.c", "plk.c", "plk.h", "tc.c"):              # unchanged since round 2's run (82c73bf)
        r = subprocess.run(["git", "diff", "--quiet", "82c73bf", "--", str(src / f)], cwd=src)
        assert r.returncode == 0, f


def test_only_named_structures():
    """spec v0.3 §6 item 5: each row holds its named structures and nothing else. By the code: outside the test
    build's audits, the only allocations are the class header, Q32-index's two lists (by SK_RESERVE), Q32-hash's table
    (once, in create) and its rational list, and the profiling build's state; TC is round 2's. By the allocations:
    the allocation check (r3_alloc) in the timing, counting and profiling builds."""
    import re
    alloc = re.compile(r"\b(sk_xmalloc|sk_xcalloc|sk_xrealloc|malloc|calloc|realloc|SK_RESERVE)\s*\(")
    want = {"q32i.c": ["sk_xcalloc(1, sizeof *c)", "SK_RESERVE(c->W, c->nW, c->capW, 16)",
                       "SK_RESERVE(c->W, c->nW, c->capW, 16)",          # in create: 0 and 1 (v0.3 §17.2)
                       "SK_RESERVE(c->R, c->N, c->capR, 16)", "sk_xcalloc(1, sizeof *c->pf)"],
            "q32h.c": ["sk_xcalloc(1, sizeof *c)", "sk_xmalloc(sizeof(sk_int) * SLOTS)",
                       "SK_RESERVE(c->R, c->N, c->capR, 16)", "sk_xcalloc(1, sizeof *c->pf)"]}
    for f, calls in want.items():
        body = strip_audit((L.SRC / f).read_text())
        got = []
        for line in body.splitlines():
            for m in alloc.finditer(line):
                j, depth = m.end(), 1
                while depth:
                    depth += {"(": 1, ")": -1}.get(line[j], 0)
                    j += 1
                got.append(line[m.start():j])
        assert sorted(got) == sorted(calls), (f, got)
        assert body.count("c->W = sk_xmalloc") == (1 if f == "q32h.c" else 0)
        assert "sk_xrealloc(c->W" not in body
    build_once()
    for b in L.ALLOC_BUILDS:
        p = subprocess.run([BIN / f"r3_alloc-{b}"], capture_output=True, text=True)
        assert p.returncode == 0 and " 0 failures" in p.stdout, p.stdout + p.stderr


def test_full_table_and_capacity():
    """spec v0.3 §8 item 5, §7 item 5: a workload that stores a 32,769th whole number aborts every build of both Q32
    rows with its message (the gate reports it as an error), while 32,768 succeed and fill Q32-hash's table; TC runs
    it. Built here as a workload file of round 1's format."""
    import struct
    build_once()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for n_wholes, ok in ((32768, True), (32769, False)):
            # 0/1, then k/1 for k = 2 .. n - 1: the whole numbers 0, 1, 2, ..., n - 1
            ms = [0] + list(range(2, n_wholes))
            n = len(ms)
            wl = t / f"cap{n_wholes}.wl"
            with open(wl, "wb") as f:
                f.write(C.WL_HEAD.pack(C.WL_MAGIC, 0, 64, 0, 0, 0, 0, n, n))
                f.write(struct.pack(f"<{n}q", *([0] * n)))
                f.write(struct.pack(f"<{n}q", *ms))
                f.write(struct.pack(f"<{n}q", *([1] * n)))
            for impl in C.IMPLS:
                for b, mode in (("timing", "mem"), ("counting", "count"), ("test", "gate")):
                    args = [BIN / f"bench-{impl}-{b}", mode, wl, t / "o.out"] + (
                        [t / "p.bin"] if mode == "count" else [t / "w.bin"] if mode == "gate" else [])
                    p = subprocess.run(args, capture_output=True, text=True)
                    if ok or impl == "TC":
                        assert p.returncode == 0, (impl, b, p.stderr[-300:])
                    else:
                        msg = "capacity of 32,768" if impl == "Q32I" else "hash table overflow"
                        assert p.returncode != 0 and msg in p.stderr, (impl, b, p.returncode, p.stderr[-300:])
                if ok and impl == "Q32H":
                    assert (t / "w.bin").stat().st_size == 8 * C.SLOTS
                    tab = struct.unpack(f"<{C.SLOTS}q", (t / "w.bin").read_bytes())
                    assert -1 not in tab and sorted(tab) == list(range(C.SLOTS))   # every slot used


def test_model_matches_round2_and_simulates_both_stores():
    """the model (r3_model) keeps round 2's values and whole numbers (r3_gen checks every job against r2_model, with 0
    and 1 first), and its two stores agree: the list in arrival order after 0 and 1, the table with the hash of spec
    v0.3 §8 (h(1) worked by hand) and 0 and 1 at slots 0 and 1 (§17.2), found with no probe"""
    import r3_model
    assert C.home(1) == (0x9E3779B97F4A7C15 >> 49) and C.home(0) == 0
    assert C.home(2) == ((2 * 0x9E3779B97F4A7C15) % 2 ** 64) >> 49
    ops, a, b = [0, 0, 1, 3], [1, 5, 0, 0], [6, 6, 1, 2]       # 1/6, 5/6, x + y = 1, x * (x + y)
    p = r3_model.predict(ops, a, b)
    assert p["values"] == [(1, 6), (5, 6), (1, 1), (1, 6)]
    assert p["list"] == [0, 1, 6, 5] and p["W_end"] == 4
    assert p["table"][0] == 0 and p["table"][1] == 1
    assert sorted(v for v in p["table"] if v >= 0) == [0, 1, 5, 6] and p["table"][C.home(6)] == 6
    s = p["stor_end"]
    # stores: 1 (found, no probe) 6 (new) | 5 (new) 6 (found) | 1/1, den h = 6: 1, 1 found, no probe | 1/6 * 1/1:
    # one_y, the operand's handles
    assert s["new"] == 2 and s["found"] == 4 and s["probes"] == 3
    assert s["found"] + s["new"] == sum(
        v for ph in p["counters"].values() for k, v in ph.items() if k in ("intern_found", "intern_appended"))
    assert s["examined"] == sum(ph.get("examined", 0) for ph in p["counters"].values())
    # the new cases: 2 + 3/5 (int_x, no gcd), 3/5 * 2 (int_y, one gcd), 0 * 3/5 (zero), 3/5 / 2 (general: division
    # has no integer case), 0 + 3/5 and 3/5 - 0 (zero_x, zero_y: no arithmetic)
    p = r3_model.predict([0, 0, 0, 0, 1, 3, 3, 4, 1, 2], [2, 3, 0, 0, 0, 1, 2, 1, 2, 1], [1, 5, 1, 1, 1, 0, 1, 0, 1, 2])
    assert p["values"][4:] == [(13, 5), (6, 5), (0, 1), (3, 10), (3, 5), (3, 5)]
    assert [C.CASES[k] for k in p["cases"][4:]] == ["add_int_x", "mul_int_y", "mul_zero", "div_general", "add_zero_x",
                                                    "add_zero_y"]
    assert p["counters"]["add"].get("gcd", 0) == 0 and p["counters"]["mul"]["gcd"] == 1


def test_untimed_steps_toy():
    """the runner's untimed steps end to end on toy jobs, with the real binaries: step 1 (build, workloads,
    predictions), step 2 (unit, differential, allocation and sanitizer tests), step 4 (gates) and step 5 (counting and
    memory runs, the repeats, and the call-for-call checks of every pairing). Nothing timed runs."""
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        r = RN.Runner(ns(jobs="3,10"))
        r.out, r.tmp = t / "out", t / "inv"
        r.out.mkdir()
        r.tmp.mkdir()
        r.state_file = r.out / "state.json"
        r.wl, r.bin = t / "work", t / "bin"
        r.state = {"elapsed_before": 0.0}
        r.t0 = time.monotonic()
        r.step1()
        r.unit_tests()
        ut = r.state["unit_tests"]
        assert set(ut) == {"r3_unit", "r3_diff", "r3_alloc-timing", "r3_alloc-counting", "r3_alloc-profiling",
                           "r3_unit-asan", "r3_diff-asan"} and all(v["rc"] == 0 for v in ut.values()), ut
        r.step4_gates()
        assert len(r.state["gates"]) == 6 and r.state["measuring"]
        r.step5_counts()
        assert threading.active_count() == 1
        for job in ("3", "10"):
            chk = r.state["call_checks"][job]
            assert chk["result"] == "pass", chk
            assert chk["Q32I"]["G_ratio"] == chk["Q32H"]["G_ratio"] == 1.0
        for impl in C.IMPLS:
            assert r.state["counts"][f"{impl}:3"]["repeat"] == "identical"
            assert r.state["mems"][f"{impl}:10"]["repeat"] == "identical"
        assert not r.state.get("bugs")
        st = r.state["counts"]["Q32H:3"]["stor"]["end"]
        assert st["new"] > 0 and st["probes"] >= st["new"]      # 0 and 1 are found with no probe (spec v0.3 §17.2)


# ------------------------------------------------------------------------------------------------- spawning, timer
def test_spawner_affinity_before_exec():
    sp = RN.Spawner()
    with tempfile.TemporaryDirectory() as t:
        out = Path(t) / "o.txt"
        rc = sp.run(["/bin/grep", "Cpus_allowed_list", "/proc/self/status"], out, C.TIMED_CORE)
        assert rc == 0 and out.read_text().split()[-1] == str(C.TIMED_CORE), out.read_text()
        rc = sp.run(["/bin/sleep", "0.2"], out, C.TIMED_CORE)   # the readings around fork and waitpid (P_idx)
        assert rc == 0 and 0.2 <= sp.last_launch_s < 2.0, sp.last_launch_s
        rc = sp.run(["/nonexistent/binary"], out, C.TIMED_CORE)
        assert rc == 127
        assert sp.spawns == 3 and sp.child is None


def test_hard_stop_kernel_timer():
    r = RN.Runner(ns(hard_stop_s=0.3))
    r.state = {"elapsed_before": 0.0}
    r.t0 = time.monotonic()
    r.arm()
    t = time.monotonic()
    with tempfile.TemporaryDirectory() as d:
        try:
            r.spawner.run(["/bin/sleep", "5"], Path(d) / "o", C.TIMED_CORE)
        except RN.HardStop:
            pid = r.spawner.child
            r.reap()
            assert time.monotonic() - t < 2.0
            assert pid is not None and r.spawner.child is None
            try:
                os.kill(pid, 0)
                raise AssertionError("the child survived the hard stop")
            except ProcessLookupError:
                pass
        else:
            raise AssertionError("the kernel timer did not stop the run")
        finally:
            r.disarm()


# ------------------------------------------------------------------------------------------------- idle, disturbance
def test_idle_probe():
    ok, d = RN.idle_probe(0.3)
    assert set(d) == {"busy", "core1", "load1"}
    p = subprocess.Popen(["taskset", "-c", str(C.TIMED_CORE), sys.executable, "-c",
                          "import time\nt=time.time()\nwhile time.time()-t<2: pass"])
    try:
        time.sleep(0.2)
        ok, d = RN.idle_probe(0.6)
        assert not ok and d["core1"] > C.IDLE_CORE1_MAX and any(b["pid"] == p.pid for b in d["busy"]), d
    finally:
        p.kill()
        p.wait()


def snap(procs, forks, t):
    return {"t": t, "forks": forks, "procs": procs}


def test_disturbance_synthetic():
    me, host, other, kt = 100, 50, 200, 300
    base = {me: ("python3", host, 10, 5, 99, 9, 1), host: ("claude", 1, 1, 1, 0, 0, 1),
            other: ("sshd", 1, 3, 3, 0, 0, 1), kt: ("kworker", 2, 0, 0, 0, 0, 1)}
    s0 = snap(base, 1000, 0.0)
    quiet = dict(base)
    quiet[me] = ("python3", host, 11, 6, 400, 30, 1)          # the runner: 2 ticks; its children's are timed
    d = RN.disturbance(s0, snap(quiet, 1012, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["runner_ticks"] == 2 and d["forks_unattributed"] == 0
    busy = {**quiet, other: ("sshd", 1, 11, 11, 0, 0, 1)}  # 16 ticks over 10 s: over 5 + ceil(1 x 10) = 15
    d = RN.disturbance(s0, snap(busy, 1012, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and d["other_limit"] == 15
    edge = {**quiet, other: ("sshd", 1, 11, 10, 0, 0, 1)}  # 15 ticks over 10 s: within
    assert not RN.disturbance(s0, snap(edge, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    short = {**quiet, other: ("sshd", 1, 5, 5, 0, 0, 1)}   # 4 ticks over 1 s: within 6; 7 ticks: over
    assert not RN.disturbance(s0, snap(short, 1012, 1.0), me, 12, {host}, {host: 7})["disturbed"]
    short = {**quiet, other: ("sshd", 1, 6, 7, 0, 0, 1)}
    assert RN.disturbance(s0, snap(short, 1012, 1.0), me, 12, {host}, {host: 7})["disturbed"]
    reaped = {**quiet, other: ("sshd", 1, 3, 3, 12, 4, 1)}  # a waited-for child's 16 ticks in cutime + cstime
    assert RN.disturbance(s0, snap(reaped, 1013, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    new = {**quiet, 999: ("cron", 1, 1, 0, 0, 0, 5)}
    d = RN.disturbance(s0, snap(new, 1013, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and d["new"][0]["pid"] == 999
    d = RN.disturbance(s0, snap(quiet, 1013, 10.0), me, 12, {host}, {host: 7})   # a fork no snapshot saw: a thread,
    assert not d["disturbed"] and d["forks_unattributed"] == 1                     # or a process with no CPU time
    hthreads = {**quiet, host: ("claude", 1, 3, 2, 0, 0, 1, 40)}            # the host: 40 threads, 3 ticks, 500 forks
    d = RN.disturbance(snap({**base, host: base[host] + (12,)}, 1000, 0.0), snap(hthreads, 1512, 10.0), me, 12,
                       {host}, {host: 7})
    assert not d["disturbed"] and d["hosts"][str(host)]["threads"] == [12, 40] and d["forks_unattributed"] == 500
    hkid = {**quiet, host: ("claude", 1, 1, 1, 6, 2, 1)}                    # the host reaped a child of 8 ticks
    d = RN.disturbance(s0, snap(hkid, 1013, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and "over its allowance" in d["reasons"][0], d
    zero = {**quiet, 998: ("sleep", host, 0, 0, 0, 0, 5)}                  # a new process under one tick: reported
    d = RN.disturbance(s0, snap(zero, 1013, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["new_quiet"][0]["pid"] == 998
    zkid = {**quiet, 997: ("sh", host, 0, 0, 1, 0, 5)}                     # a new process that reaped a child
    assert RN.disturbance(s0, snap(zkid, 1013, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    kth = {**quiet, 301: ("kworker/1", 2, 0, 1, 0, 0, 7)}                   # a new kernel thread: explained
    d = RN.disturbance(s0, snap(kth, 1013, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["new_kthreads"] == 1
    hb = {**quiet, host: ("claude", 1, 6, 4, 0, 0, 1)}                       # the host: 8 ticks over 7
    assert RN.disturbance(s0, snap(hb, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    d = RN.disturbance(s0, snap(hb, 1012, 10.0), me, 12, {host}, None)             # the pilot records, no flag
    assert not d["disturbed"] and d["hosts"][str(host)]["rate"] == 0.8
    reused = {**quiet, other: ("sshd", 1, 0, 1, 0, 0, 9)}                   # a pid reused (another start time)
    assert RN.disturbance(s0, snap(reused, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]


def test_existing_child_exit_charged_for_the_block_only():
    """review fix (b): a child that existed before the block and is reaped during it adds its whole lifetime's ticks
    to its reaper's cutime + cstime; only its ticks during the block are charged (its ticks at the first snapshot are
    credited back to its parent, at most the rise of the parent's cutime + cstime)"""
    me, host, other, kid = 100, 50, 200, 210
    base = {me: ("python3", host, 10, 5, 0, 0, 1), host: ("claude", 1, 1, 1, 0, 0, 1),
            other: ("sshd", 1, 3, 3, 0, 0, 1), kid: ("worker", other, 40, 10, 0, 0, 2)}       # the child: 50 ticks
    s0 = snap(base, 1000, 0.0)
    gone = {k: v for k, v in base.items() if k != kid}
    late = {**gone, other: ("sshd", 1, 3, 3, 42, 10, 1)}      # reaped: 52 ticks in cutime + cstime, 2 in the block
    d = RN.disturbance(s0, snap(late, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["disturbed"] and d["credited"][str(other)]["ticks"] == 50, d
    busy = {**gone, other: ("sshd", 1, 3, 3, 60, 10, 1)}      # 20 ticks in the block: over 15
    d = RN.disturbance(s0, snap(busy, 1000, 10.0), me, 0, {host}, {host: 7})
    assert d["disturbed"] and d["others"][0]["ticks"] == 20, d
    ign = {**gone, other: ("sshd", 1, 10, 3, 0, 0, 1)}        # the parent ignores SIGCHLD: no cutime, nothing credited
    d = RN.disturbance(s0, snap(ign, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["credited"] and not d["disturbed"]
    ign = {**gone, other: ("sshd", 1, 17, 5, 0, 0, 1)}        # its own 16 ticks: over 15, nothing taken off
    assert RN.disturbance(s0, snap(ign, 1000, 10.0), me, 0, {host}, {host: 7})["disturbed"]
    hb = {**base, kid: ("worker", host, 40, 10, 0, 0, 2)}     # a host's child, reaped by the host in the block
    hl = {**gone, host: ("claude", 1, 2, 1, 42, 10, 1)}       # 1 + 2 ticks in the block, within 7
    d = RN.disturbance(snap(hb, 1000, 0.0), snap(hl, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["disturbed"] and d["hosts"][str(host)]["ticks"] == 3, d
    orphan = {k: v for k, v in gone.items() if k != other}     # the parent exited too: the orphan's reaper, process 1,
    orphan[1] = ("init", 0, 0, 0, 56, 0, 1)                    # is charged its whole lifetime; only the parent's own
    d = RN.disturbance(snap({**base, 1: ("init", 0, 0, 0, 0, 0, 1)}, 1000, 0.0), snap(orphan, 1000, 10.0), me, 0,
                       {host, 1}, {host: 7, 1: 7})                 # 6 ticks before the block, as init's child, go off
    assert d["disturbed"] and d["hosts"]["1"]["ticks"] == 50 and d["credited"] == {"1": {"name": "init", "ticks": 6}}, d
    # a real child, alive and busy before the block (about 30 ticks), that exits and is reaped during it
    parent = subprocess.Popen([sys.executable, "-c",
                               "import subprocess, sys, time\n"
                               f"subprocess.run([sys.executable, '-c', {BURN!r} + "
                               "'\\nprint(\"ready\", flush=True)\\ntime.sleep(1.0)'])\n"
                               "time.sleep(30)\n"], stdout=subprocess.PIPE, text=True)
    try:
        assert parent.stdout.readline().strip() == "ready"
        hosts = {p for p, _ in RN.ancestors()}
        s0 = RN.snapshot()
        kids = [p for p, st in s0["procs"].items() if st[1] == parent.pid]
        assert len(kids) == 1 and RN.ticks4(s0["procs"][kids[0]]) >= 20, s0["procs"].get(kids[0] if kids else 0)
        time.sleep(1.6)                                          # the child exits and is reaped in here
        s1 = RN.snapshot()
        assert kids[0] not in s1["procs"]
        raw = RN.ticks4(s1["procs"][parent.pid]) - RN.ticks4(s0["procs"][parent.pid])
        assert raw > K.other_limit(s1["t"] - s0["t"]), raw            # charged in full, it would flag
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert not [o for o in d["others"] if o["pid"] == parent.pid], d
        assert d["credited"][str(parent.pid)]["ticks"] == RN.ticks4(s0["procs"][kids[0]]), d
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts | {parent.pid},     # as a host, with the floor alone
                           {**{p: 10 ** 6 for p in hosts}, parent.pid: C.HOST_BASE_TICKS})
        assert d["hosts"][str(parent.pid)]["ticks"] <= C.HOST_BASE_TICKS and not d["disturbed"], d
    finally:
        parent.kill()
        parent.wait()


HELPER_THREADS = r"""
import threading, time, sys
time.sleep(0.5)                                    # after the first snapshot
def burn():
    t = time.process_time()
    while time.process_time() - t < 0.004: pass
for k in range(60):                                # 60 short-lived threads, each gone before the second snapshot
    th = threading.Thread(target=burn); th.start(); th.join()
time.sleep(30)
"""


def test_host_threads_do_not_disturb():
    """A host process that creates many short-lived threads within the block does not disturb it: its threads are
    its own (their CPU time is in its utime + stime, against its allowance), and the fork counter, which counts
    them, is reported only (before the revision this block counted dozens of unexplained forks)."""
    host = subprocess.Popen([sys.executable, "-c", HELPER_THREADS])
    try:
        time.sleep(0.2)
        hosts = {p for p, _ in RN.ancestors()} | {host.pid}
        s0 = RN.snapshot()
        time.sleep(1.6)
        s1 = RN.snapshot()
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert not d["disturbed"], d
        assert d["forks_unattributed"] >= 60, d          # the threads, in the fork counter: reported, no flag
        assert not [n for n in d["new"] + d["new_quiet"] if n["ppid"] == host.pid], d   # no thread is a process
        assert str(host.pid) in d["hosts"] and d["hosts"][str(host.pid)]["ticks"] >= 1, d   # charged to the host
        tight = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {**{p: 10 ** 6 for p in hosts}, host.pid: 0})
        assert tight["disturbed"] and any(str(host.pid) in x for x in tight["reasons"]), tight   # but counted
    finally:
        host.kill()
        host.wait()


BURN = "import time\nt=time.process_time()\nwhile time.process_time()-t<0.3: pass"


def test_disturbance_catches_short_lived_process():
    """A genuinely new outside process is caught, even a short-lived one, deliberately started within the block: one
    that is alive (or a zombie) at the second snapshot, as a new process; one that started and exited, by its CPU
    time in the cutime + cstime of the process that reaped it, against that process's limit (another process's 5
    ticks; a host's allowance)."""
    helper = subprocess.Popen([sys.executable, "-c",              # an outside process: its child starts and exits
                               "import time, subprocess, sys\n"
                               "time.sleep(0.5)\n"
                               f"subprocess.run([sys.executable, '-c', {BURN!r}])\n"
                               "subprocess.run(['/bin/true'])\n"
                               "time.sleep(30)\n"])
    late = subprocess.Popen([sys.executable, "-c",                # an outside process starting a child kept alive
                             "import time, subprocess, sys\n"
                             "time.sleep(0.5)\n"
                             f"p = subprocess.Popen([sys.executable, '-c', {BURN!r} + '\\ntime.sleep(30)'])\n"
                             "print(p.pid, flush=True)\n"
                             "time.sleep(30)\n"], stdout=subprocess.PIPE, text=True)
    kid = None
    try:
        time.sleep(0.2)
        hosts = {p for p, _ in RN.ancestors()}
        s0 = RN.snapshot()
        time.sleep(1.6)                                      # the processes start (and the first exits) in here
        s1 = RN.snapshot()
        kid = int(late.stdout.readline())
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert d["disturbed"], d
        caught = [o for o in d["others"] if o["pid"] == helper.pid]
        assert caught and caught[0]["ticks"] > C.DIST_OTHER_TICKS, d  # the exited child's ticks, in its reaper's cutime
        assert any(n["pid"] == kid and n["ppid"] == late.pid for n in d["new"]), d     # the new process, alive
        # the same exited child, when its reaper is a host: charged to the host's allowance
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts | {helper.pid}, {**{p: 10 ** 6 for p in hosts},
                                                                          helper.pid: C.HOST_BASE_TICKS})
        assert d["hosts"][str(helper.pid)]["ticks"] > C.HOST_BASE_TICKS, d
        assert any(f"[{helper.pid}]" in x and "allowance" in x for x in d["reasons"]), d
    finally:
        for p in (helper, late):
            p.kill()
            p.wait()
        if kid:
            try:
                os.kill(kid, signal.SIGKILL)
            except ProcessLookupError:
                pass


# ------------------------------------------------------------------------------------------------- the timed block
FAKE = r'''#!/usr/bin/env python3
# a stand-in for bench_r3 (harness test only): copies the reference as its read-out, prints made-up lines
import sys, os, shutil
mode, wl, ro = sys.argv[1], sys.argv[2], sys.argv[3]
row = os.path.basename(sys.argv[0]).split("-")[1]
cfg = os.environ.get("FAKE_CFG", "")
shutil.copy(wl[:-3] + ".ref", ro)
if "bug" in cfg and row == "TC":
    with open(ro, "r+b") as f:
        f.seek(0); b = f.read(1); f.seek(0); f.write(bytes([b[0] ^ 1]))
if mode == "calib":
    for r in (1, 2, 4):
        print(f"calib {r} {r * 20000000} 1 2 3 1 row={row} cpu={r * 19} span={r * 21}")
    if "cap" in cfg and row == "TC":
        print("overcap 20000000000")
elif mode == "time":
    r, reps = int(sys.argv[4]), int(sys.argv[5])
    for i in range(reps):
        print(f"rep {i} {r * (1000 + i)} {r * 100} {r * 800} {r * (100 + i)} {r * 7} row={row} cpu={r * (990 + i)} "
              f"span={r * (1100 + i)}")
elif mode == "prof":
    r = int(sys.argv[4])
    print("probe_cost 30.0"); print("tsc_hz 2000000000.0"); print("prof_k 32")
    for i in range(3):
        print(f"prep {i} {r * 1200} {r * 120} {r * 960} {r * 120} sampled=10 row={row} create={r * 9}")
        print(f"pfcalls {i} 1 add_den 10 10000")
        for ph in range(6):
            print(f"pf {i} 1 add_den {ph} 1000 10")
        print(f"pfm {i} 1 500 2")
print(f"ok impl={row} calls=0")
'''


def fake_runner(tmp, cfg=""):
    w = toy_work()
    r = RN.Runner(ns(jobs="3"))
    r.out = tmp / "out"
    r.out.mkdir(parents=True)
    r.state_file = r.out / "state.json"
    r.tmp = tmp / "inv"
    r.wl = w
    r.state = {"elapsed_before": 0.0, "measuring": True}
    r.t0 = time.monotonic()
    r.hosts = {}
    r.idle_probe = lambda: (True, {"busy": [], "core1": 0.0, "load1": 0.0})
    fb = tmp / "fakebin"
    fb.mkdir()
    for impl in C.IMPLS:
        for b in ("timing", "profiling"):
            p = fb / f"bench-{impl}-{b}"
            p.write_text(FAKE)
            p.chmod(0o755)
    r.bin = fb
    os.environ["FAKE_CFG"] = cfg
    return r


def test_timed_block_machinery():
    real = RN.disturbance
    seen = []

    def quiet(s0, s1, runner, spawns, hosts, allowance=None):
        d = real(s0, s1, runner, spawns, hosts, allowance)
        seen.append(d)
        d.update(disturbed=False, reasons=[], runner_ticks=0)
        return d
    RN.disturbance = quiet
    try:
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(3, "rounds", "measured")
            cal = [x for x in recs if x["role"] == "calibration"]
            rnd = [x for x in recs if x["role"] == "round"]
            assert [x["row"] for x in cal] == ["TC", "Q32-index", "Q32-hash"] and cal[0]["r"] == 4 and cal[0]["reps"] == 3
            assert len(rnd) == 4 * C.ROUNDS and seen[-1]["spawns"] == 3 + 4 * C.ROUNDS   # 51 launches (rule-r3)
            assert [x["row"] for x in rnd[:8]] == ["TC", "Q32-index", "Q32-hash", "Q32-hash-null",
                                                  "Q32-index", "Q32-hash", "Q32-hash-null", "TC"]  # rotated by one
            x = rnd[0]
            assert x["r"] == 4 and x["total"] == 1001 and x["read"] == 101 and "bug" not in x   # reps 2-3, minimum
            assert x["creation"] == 100 and x["ops"] == 800 and x["create"] == 7              # create, on its own
            assert all(y["launch_s"] is not None and y["launch_s"] > 0 for y in recs)       # fork..waitpid readings
            assert x["cpu_ns"] == 991 and x["span_ns"] == 1101 and x["cpu_share"] == 991 / 1101   # the kept rep's
            assert x["reps_cpu"] == [(4 * 990, 4 * 1100), (4 * 991, 4 * 1101), (4 * 992, 4 * 1102)]   # every rep's
            assert cal[0]["reps_cpu"] == [(r * 19, r * 21) for r in (1, 2, 4)], cal[0]          # calibration reps too
            assert not list(r.tmp.glob("i*")), "the invocation files are removed after the block"
            assert all(threading.active_count() == 1 for _ in [0])
            r.state["calibration"] = {f"3:{row}": {"r": 4} for row in C.ROWS}
            precs = r.timed_block(3, "prof", "profile")
            assert [x["row"] for x in precs] == ["Q32-index", "Q32-hash", "TC"] * 3
            assert precs[0]["total"] == 1200.0 and precs[0]["profile"]["sampled"] == 10       # reps 2-3, minimum
            assert precs[0]["create"] == 9.0 and precs[0]["creation"] == 120.0
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t), "cap")
            recs = r.timed_block(3, "rounds", "measured")
            assert not any(x["row"] == "TC" for x in recs if x["role"] == "round")             # TC over cap
            assert any(x.get("overcap") for x in recs if x["row"] == "TC")
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t), "bug")
            recs = r.timed_block(3, "rounds", "measured")
            assert all("bug" in x for x in recs if x["row"] == "TC")
            assert not any("bug" in x for x in recs if x["row"] != "TC")
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_block_rerun_once():
    real = RN.disturbance
    calls = []

    def flaky(s0, s1, runner, spawns, hosts, allowance=None):
        d = real(s0, s1, runner, spawns, hosts, allowance)
        calls.append(1)
        bad = len(calls) == 1
        d.update(disturbed=bad, reasons=["made up"] if bad else [], runner_ticks=0)
        return d
    try:
        RN.disturbance = flaky
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(10, "rounds", "measured")
            assert len(calls) == 2 and len(r.state["discarded_blocks"]) == 1 and recs
        RN.disturbance = lambda *a, **k: dict(real(*a, **k), disturbed=True, reasons=["made up"], runner_ticks=0)
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            try:
                r.timed_block(10, "rounds", "measured")
            except RN.RunStop as e:
                assert "disturbed twice" in str(e)
            else:
                raise AssertionError("a second disturbance did not stop the run")
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_runner_ticks_per_launch():
    """rule-r2 item 5 (second revision of Oct 29, 2026): the runner's ticks are recorded and reported, in total and per
    launch, not judged; the one sanity limit is more than 1 tick per launch plus 2 over a block (a fixed allowance
    for the block's own two snapshots; minor fix found in review). The stopped run's
    measured block (5 ticks over 38 launches, against the old fixed limit of 2) now passes; a block with many launches
    at the runner's normal cost passes; a 2-launch block at 3 or 4 ticks passes; an artificially busy runner (above
    1 tick per launch plus 2) stops the run as a
    harness bug; the per-launch figures are in the record, the log and the report. Nothing is timed: the stand-in
    driver runs in place of bench_r3."""
    assert C.RUNNER_TICKS_PER_LAUNCH_MAX == 1.0 and C.RUNNER_TICKS_BLOCK_ALLOWANCE == 2
    assert not hasattr(C, "RUNNER_TICKS_MAX")
    assert K.runner_ticks_check(5, 38) == (5 / 38, False)        # the stopped block of 173fad1
    assert K.runner_ticks_check(40, 38) == (40 / 38, False)      # exactly 1 per launch plus 2: within
    assert K.runner_ticks_check(41, 38)[1]                       # above 1 per launch plus 2: a harness bug
    assert K.runner_ticks_check(3, 2) == (1.5, False) and K.runner_ticks_check(4, 2) == (2.0, False)  # 2-launch block
    assert K.runner_ticks_check(5, 2)[1]
    assert K.runner_ticks_check(2, 0) == (None, False) and K.runner_ticks_check(3, 0)[1]   # no launch: the 2 only
    assert K.runner_ticks_check(None, 38) == (None, False)
    me = 100
    s0 = snap({me: ("python3", 1, 10, 5, 0, 0, 1)}, 0, 0.0)
    for ticks, spawns, over in ((5, 38, False), (500, 600, False), (3, 2, False), (41, 38, True), (5, 2, True)):
        d = RN.disturbance(s0, snap({me: ("python3", 1, 10 + ticks, 5, 0, 0, 1)}, spawns, 8.0), me, spawns, set(), {})
        assert d["runner_ticks"] == ticks and d["runner_ticks_per_launch"] == ticks / spawns, d
        assert d["runner_over_limit"] == over and not d["disturbed"], d          # never a disturbance of the timings
    real = RN.disturbance

    def others_ignored(*a, **k):                     # the runner's real ticks; other processes are not this test's
        return dict(real(*a, **k), disturbed=False, reasons=[])
    try:
        RN.disturbance = others_ignored
        with tempfile.TemporaryDirectory() as t:         # many launches at the runner's normal cost: passes
            r = fake_runner(Path(t))
            recs = r.timed_block(3, "rounds", "measured")
            d = r.state["disturbance_checks"][-1]
            assert recs and d["spawns"] == 3 + 4 * C.ROUNDS and not d["runner_over_limit"], d
            assert d["runner_ticks_per_launch"] == d["runner_ticks"] / d["spawns"] <= 1.0, d
            assert "per launch" in (r.out / "run.log").read_text()
        with tempfile.TemporaryDirectory() as t:         # an artificially busy runner: about 3 ticks per launch
            r = fake_runner(Path(t))
            parse = r.quick_parse

            def busy(rec):
                c = time.process_time()
                while time.process_time() - c < 0.03:
                    pass
                return parse(rec)
            r.quick_parse = busy
            try:
                r.timed_block(3, "rounds", "measured")
            except RN.RunStop as e:
                d = r.state["disturbance_checks"][-1]
                assert "harness bug" in str(e) and "per launch" in str(e) and r.state["harness_bugs"], str(e)
                assert d["runner_over_limit"] and d["runner_ticks"] > d["spawns"] + 2, d
                assert d["spawns"] == 3 + 4 * C.ROUNDS, d
            else:
                raise AssertionError("a runner above 1 tick per launch plus 2 was not a harness bug")
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_pool_is_joined():
    r = RN.Runner(ns())
    r.state = {"elapsed_before": 0.0}
    r.t0 = time.monotonic()
    res = r.pool([(k, (lambda core, k=k: (k, core))) for k in range(6)])
    assert sorted(res) == list(range(6)) and threading.active_count() == 1


# ------------------------------------------------------------------------------------------------- the analysis
def test_analysis():
    """a finished state gives per-pairing, per-measure verdicts (creation and operations, each with its own band from
    the null row; total and read-out reported), the gcd-work and call-for-call checks, create's share, the storage
    figures, speed against fill with its plot, and the profile; a stopped one gives no verdict"""
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        recs = []
        # TC 150 per run; Q32-index 300 (slower); Q32-hash 100 and its null copy 100.5 (h: the 5% floor)
        for k in range(1, 13):
            for row, tot in (("TC", 150.0), ("Q32-index", 300.0), ("Q32-hash", 100.0), ("Q32-hash-null", 100.5)):
                recs.append({"job": 3, "row": row, "role": "round", "round": k, "total": tot + k * 0.1,
                             "creation": tot / 5, "ops": tot * 3 / 5 + k * 0.1, "read": tot / 5,
                             "create": 1.0 if row != "Q32-hash" and row != "Q32-hash-null" else 5.0})
        memq = ["mem end whole numbers 2800 4096", "mem end rationals 4000 4096", "memN end 1000",
                "mem after_entry whole numbers 64 128", "mem after_entry rationals 400 512", "memN after_entry 100"]
        memh = ["mem end hash table 262144 262144", "mem end rationals 4000 4096", "memN end 1000",
                "mem after_entry hash table 262144 262144", "mem after_entry rationals 400 512", "memN after_entry 100"]
        memt = ["mem end pairs 16000 16384", "memN end 1000", "mem after_entry pairs 1600 2048", "memN after_entry 100"]
        stor = {"after_entry": {"found": 150, "new": 50, "examined": 3000, "probes": 220, "longest": 3},
                "end": {"found": 1500, "new": 350, "examined": 90000, "probes": 2000, "longest": 9}}
        prof = {"total": 1200, "entry": 120, "ops": 960, "read": 120, "sampled": 10, "create": 8,
                "cyc": [[1, "add_den", ph, 1000, 10] for ph in range(6)], "calls": [[1, "add_den", 10, 10000]],
                "m": [[1, 500, 2]]}
        st = {"started": "x", "finished": "y", "jobs": [3], "elapsed_saved": 300, "rule_sha256": C.RULE_SHA256,
              "records": recs, "calibration": {}, "toy": True,
              "counts": {"Q32I:3": {"mem": memq, "counts": ["ctr add core gcd_work 500", "sctr add store examined 9"],
                                    "stor": stor},
                         "Q32H:3": {"mem": memh, "counts": ["ctr add core gcd_work 500", "sctr add store probes 9"],
                                    "stor": stor},
                         "TC:3": {"mem": memt, "counts": ["ctr add core gcd_work 500"]}},
              "mems": {"Q32I:3": {"memrun": {"end": {"heap": 9000, "stored": 1000},
                                             "after_entry": {"heap": 600, "stored": 100}}},
                       "Q32H:3": {"memrun": {"end": {"heap": 270000, "stored": 1000},
                                             "after_entry": {"heap": 263000, "stored": 100}}},
                       "TC:3": {"memrun": {"end": {"heap": 17000, "stored": 1000},
                                           "after_entry": {"heap": 1700, "stored": 100}}}},
              "call_checks": {"3": {"G_TC": 500, "result": "pass",
                                    "Q32I": {"call_for_call": "pass", "G": 500, "G_ratio": 1.0,
                                             "gcd_work_check": "passed"},
                                    "Q32H": {"call_for_call": "pass", "G": 500, "G_ratio": 1.0,
                                             "gcd_work_check": "passed"},
                                    "Q32I_vs_Q32H": {"stores_call_for_call": "pass", "counters": "pass",
                                                     "found_new": "pass"}}},
              "gen": {"3": {"n": 1000, "phase_counts": {"entry": 100, "add": 900}}},
              "profiles": [{"job": 3, "row": row, "profile": prof, "r": 4, "tsc_hz": 2e9, "probe_cost": 30.0, "K": 32,
                            "total": 300.0, "creation": 30.0, "ops": 240.0, "read": 30.0, "create": 2.0,
                            "slowdown": 1.1, "within_limit": True} for row in C.PROF_ROWS for _ in range(3)]}
        st["disturbance_checks"] = [{"job": 3, "label": "measured", "kind": "rounds", "attempt": 1, "seconds": 8.4,
                                     "disturbed": False, "reasons": [], "runner_ticks": 5, "spawns": 51,
                                     "forks_rise": 51, "forks_unattributed": 0, "hosts": {}, "other_limit": 14},
                                    {"job": 3, "label": "profile", "kind": "prof", "attempt": 1, "seconds": 1.0,
                                     "disturbed": False, "reasons": [], "runner_ticks": 1, "spawns": 9,
                                     "forks_rise": 9, "forks_unattributed": 0, "hosts": {}, "other_limit": 6}]
        (t / "state.json").write_text(json.dumps(st))
        (t / "results.md").write_text("# head\n<!-- rule ends -->\nold tail\n")
        A.report(t)
        md = (t / "results.md").read_text()
        assert "old tail" not in md
        assert "### Q32-index against TC" in md and "### Q32-hash against TC" in md
        assert "### Q32-hash against Q32-index" in md
        # Q32-hash against TC: creation 30/20 = 1.5 win, operations 1.5 win; Q32-index against TC: losses, caveat (c)
        assert "| 3 SMALL8 | win, 1.500 [1.500, 1.500], h 0.050 | win, 1.49" in md, md[md.find("### Q32-hash against TC"):][:800]
        assert "| 3 SMALL8 | loss, 0.500 [0.500, 0.500], h 0.050 (c)" in md
        assert "| no | 5 | 51 | 0.098 | 51 |" in md and "| no | 1 | 9 | 0.111 | 9 |" in md   # per block, per launch
        assert "Q32-index's own noise is not measured" in md and "indicative only" in md
        assert "1.000000 (passed)" in md and "pass, pass, pass" in md
        assert "| 3 SMALL8 | end | 350 | 1.1% | 1500 / 350 | 1.08 | 9 | 48.6 |" in md, md[md.find("## Storage"):][:900]
        assert "## `create`, on its own" in md and "5 (0.2500)" in md and "1 (0.0333)" in md
        assert (t / "speed_vs_fill.png").exists() and "![Speed against fill](speed_vs_fill.png)" in md
        assert "three-structure Q32 (retired)" in md
        v = list(csvrows(t / "verdicts.csv"))
        got = {(x["pairing"], x["measure"], x["verdict"]) for x in v}
        assert {("Q32-hash vs TC", "creation", "win"), ("Q32-hash vs TC", "operations", "win"),
                ("Q32-index vs TC", "operations", "loss"), ("Q32-hash vs Q32-index", "operations", "win"),
                ("Q32-hash vs TC", "total", "reported"), ("Q32-hash vs TC", "B_used", "loss"),
                ("Q32-index vs TC", "B_used", "win"), ("Q32-hash vs TC", "gcd work (check)", "passed")} <= got, got
        # one band per measure: a noisy null on operations widens h for operations only
        for x in st["records"]:
            if x["row"] == "Q32-hash-null":
                x["ops"] *= 1.3 if x["round"] % 2 else 0.8
        (t / "state.json").write_text(json.dumps(st))
        d = A.Data(st)
        hc, _ = A.null_h(d, 3, "creation")
        ho, _ = A.null_h(d, 3, "operations")
        assert hc == C.BAND_MIN and ho > 0.1, (hc, ho)
        st["run_stopped"] = {"step": "step 4", "reason": "a gate failed"}
        (t / "state.json").write_text(json.dumps(st))
        A.report(t)
        md = (t / "results.md").read_text()
        assert "No verdict is reported" in md and "| 3 SMALL8 | win" not in md
def test_profile_m_per_run():
    """the driver sums the growth cycles over a rep's r runs, so M per operation divides by r as the batch does: a
    synthetic profile with r = 4 whose growths take 800 cycles in all over 4 runs of 100 add calls at 2 GHz gives
    800 / 4 / 100 = 2 cycles = 1 ns per operation, the same as r = 1 with 200 cycles (unfixed, r = 4 gives 4 ns)"""
    def rec(r, mcyc):
        prof = {"total": 1200 * r, "entry": 120 * r, "ops": 960 * r, "read": 120 * r, "sampled": 10 * r,
                "create": 8 * r, "cyc": [[1, "add_den", ph, 1000 * r, 10 * r] for ph in range(6)],
                "calls": [[1, "add_den", 10 * r, 10000 * r]], "m": [[1, mcyc, 2 * r]]}
        return {"profile": prof, "r": r, "tsc_hz": 2e9, "probe_cost": 30.0, "K": 32,
                "total": 1200.0, "creation": 120.0, "ops": 960.0, "read": 120.0, "create": 20.0}
    pc = {"entry": 10, "add": 100}
    f4, f1 = A.profile_figures(rec(4, 800), pc), A.profile_figures(rec(1, 200), pc)
    for f in (f4, f1):
        assert abs(f["parts"]["operations"]["M_ns_per_op"] - 1.0) < 1e-9, f["parts"]["operations"]
        assert abs(f["parts"]["operations"]["batch_ns_per_op"] - 9.6) < 1e-9
        assert abs(f["parts"]["entry"]["batch_ns_per_op"] - (120.0 - 20.0) / 10) < 1e-9   # create left out of entry
        assert f["create_ns"] == 20.0
    assert f4["parts"]["operations"]["est_ns_per_op"] == f1["parts"]["operations"]["est_ns_per_op"]
def csvrows(p):
    import csv
    with open(p) as f:
        yield from csv.DictReader(f)


# ------------------------------------------------------------------------------------------------- full size
def full():
    """every gate and counting run on the full-size workloads (untimed)"""
    build_once()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        s = r3_gen.generate(t / "work", list(C.ALL_JOBS))
        assert all(m["identical_to_round1"] for m in s.values())
        for job in C.RUN_JOBS:
            w = t / "work"
            pred = json.loads((w / f"job-{job:02d}.r3.json").read_text())["predicted"]
            for impl in C.IMPLS:
                p = subprocess.run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out",
                                    t / "w.bin"], capture_output=True, text=True)
                assert p.returncode == 0, p.stdout[-300:] + p.stderr[-300:]
                assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
                f = K.exact_checks(impl, p.stdout.splitlines(), pred,
                                   C.sha256(t / "w.bin") if impl in C.Q_IMPLS else None)
                assert not f, (job, impl, f[:5])
                p = subprocess.run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                                    t / f"p-{impl}.bin", t / f"s-{impl}.bin"], capture_output=True, text=True)
                assert p.returncode == 0
            for i in C.Q_IMPLS:
                assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
            assert (t / "s-Q32I.bin").read_bytes() == (t / "s-Q32H.bin").read_bytes()
            print(f"  full size: job {job} gates pass for the three rows; each Q32 row equal to TC call for call; "
                  f"the two Q32 rows' stores equal call for call")
TESTS = [test_rule_fixed_and_pinned, test_vendor08_and_official_data, test_workloads_identical_to_round1,
         test_runner_imports_no_numpy, test_rows_identical_outside_storage, test_only_named_structures,
         test_model_matches_round2_and_simulates_both_stores, test_gates_and_counts_toy,
         test_exact_checks_catch_a_difference, test_full_table_and_capacity, test_untimed_steps_toy,
         test_profiling_build, test_statistics_and_verdicts, test_pilot_projection_and_allowance, test_pilot_stops,
         test_spawner_affinity_before_exec, test_hard_stop_kernel_timer, test_idle_probe, test_disturbance_synthetic,
         test_host_threads_do_not_disturb, test_disturbance_catches_short_lived_process, test_host_allowance_from_pilot,
         test_existing_child_exit_charged_for_the_block_only,
         test_timed_block_machinery, test_block_rerun_once, test_runner_ticks_per_launch,
         test_pool_is_joined, test_analysis, test_profile_m_per_run]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--full", action="store_true")
    a = ap.parse_args()
    failed = 0
    for t in TESTS:
        try:
            t()
            print(f"pass  {t.__name__}")
        except Exception as e:
            failed += 1
            import traceback
            traceback.print_exc()
            print(f"FAIL  {t.__name__}: {e}")
    if a.full:
        full()
    print(f"{len(TESTS) - failed} of {len(TESTS)} harness tests pass")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
