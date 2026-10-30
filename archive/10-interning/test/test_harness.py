"""Tests of the harness (not measurements).

    python3 test_harness.py              the quick tests (rule refusal, 08 copies, generator, statistics, code hashes,
                                         the pilot's stop rule, the official data protected, the Q32 rows' gcd work
                                         against time, the idle-machine check, the hard stop, the randomized
                                         differential test of every row binary against Python fractions)
    python3 test_harness.py --full DIR   also the full-size check of the generator's predictions: every job's gate
                                         run (test build) of L1, L1-opt (and, on the jobs within capacity, L1-Q32 and
                                         L1-opt-Q32) against the predicted W, cases and trick counters, L1-opt against
                                         L1, each Q32 row against its Q64 row (gate and counting runs), the lookup gate
                                         on every job, and the counting runs' cases den and general against ctl;
                                         workloads written to DIR (a scratch directory, not the run's). Untimed.
"""
import argparse
import json
import random
import shutil
import subprocess
import sys
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

import analyze
import common as C
import gen
import libconf as L
import runner


def ns(**kw):
    d = dict(toy=True, jobs="", pilot_only=False, hard_stop_s=None, resume=False, rerun=None)
    d.update(kw)
    return argparse.Namespace(**d)


def test_rule_refusal():
    r = runner.Runner(ns(toy=False))
    saved = C.RULE_SHA256
    try:
        C.RULE_SHA256 = "0" * 64
        try:
            r.preflight()
        except SystemExit as e:
            assert "rule.md differs" in str(e)
        else:
            raise AssertionError("a measured run with another rule was not refused")
    finally:
        C.RULE_SHA256 = saved
    assert C.rule_sha256() == C.RULE_SHA256, "rule.md on disk is not the fixed rule"


def test_vendor08():
    assert L.vendor08_check() is None
    sums = dict(reversed(l.split()) for l in (L.V08 / "SHA256SUMS").read_text().splitlines())
    assert sums == L.VENDOR08_SHA256


def test_generator_deterministic():
    with tempfile.TemporaryDirectory() as a, tempfile.TemporaryDirectory() as b:
        gen.generate(a, [3, 9], toy=True)
        gen.generate(b, [3, 9], toy=True)
        for f in ("job-03.wl", "job-03.ref", "job-03.case", "job-09.wl", "job-09.json"):
            assert (Path(a) / f).read_bytes() == (Path(b) / f).read_bytes(), f


def test_bands():
    assert analyze.fixed_verdict(0.90) == "win" and analyze.fixed_verdict(1.10) == "loss"
    assert analyze.fixed_verdict(0.95) == "tie"
    assert analyze.time_verdict(1.2, 1.3, 0.05) == "win" and analyze.time_verdict(0.7, 0.8, 0.05) == "loss"
    assert analyze.time_verdict(0.97, 1.2, 0.05) == "tie"
    assert C.SEVERE == 2.0 and C.HARD_STOP_S == 3600 and C.PILOT_LIMIT_S == 2700


def test_bootstrap_paired():
    a = np.array([10.0] * 12)
    r, lo, hi = analyze.boot_ratio(a, a)
    assert r == lo == hi == 1.0
    b = a * 2
    r, lo, hi = analyze.boot_ratio(b, a)
    assert r == lo == hi == 2.0


def test_code_hashes_invalidate():
    code, ci = runner.code_hashes()
    assert len(set(code.values())) == len(code)          # every row its own hash
    assert code["L1"] != code["L1opt"]                   # flags differ
    saved = L.OWN["L1"]
    try:
        tmp = Path(tempfile.mkdtemp()) / "l1.c"
        tmp.write_text((L.LIBDIR / "src" / "l1.c").read_text() + "\n/* changed */\n")
        L.OWN["L1"] = [tmp]
        code2, _ = runner.code_hashes()
        assert code2["L1"] != code["L1"] and code2["ctl"] == code["ctl"]
    finally:
        L.OWN["L1"] = saved
    r = runner.Runner(ns())
    r.state = {"gates": {"L1:1": {"code": "old", "int": ci}, "ctl:1": {"code": code["ctl"], "int": ci}},
               "counts": {}, "mems": {}, "elapsed_before": 0}
    r.t0 = None
    r.out = Path(tempfile.mkdtemp())
    stale = r.drop_stale()
    assert stale == {"L1"} and "ctl:1" in r.state["gates"] and "L1:1" not in r.state["gates"]


def test_pilot_stop_rule():
    """The projection: elapsed + 11.5 min x max(1, pilot wall / 1.2 min) (revised for the Q32 rows); over 45 minutes
    the run stops."""
    r = runner.Runner(ns(jobs="10"))
    r.state = {"elapsed_before": 30 * 60.0, "not_constructible": []}
    r.t0 = runner.time.monotonic()
    r.out = Path(tempfile.mkdtemp())
    r.save = lambda: None
    r.log = lambda m: None
    r.calibrate = lambda job, store, live: ({}, {})
    r.rounds = lambda *a, **k: runner.time.sleep(0.01)
    r.idle_probe = lambda: (True, {"load1": 0.0, "load_max": C.IDLE_LOAD_MAX, "busy": [],
                                   "proc_cpu_max": C.IDLE_PROC_CPU_MAX})
    r.jobs = [10]
    saved = C.PILOT_NOMINAL_S
    try:
        C.PILOT_NOMINAL_S = 0.001                        # a pilot 10x+ longer than nominal: 11.5 min x factor
        try:
            r.step3_pilot()
        except runner.RunStop as e:
            assert "exceeds 45 minutes" in str(e)
        else:
            raise AssertionError("the pilot's projection did not stop the run")
    finally:
        C.PILOT_NOMINAL_S = saved
    r.state = {"elapsed_before": 0.0, "not_constructible": []}
    r.t0 = runner.time.monotonic()
    r.step3_pilot()                                      # 11.5 minutes: goes on
    assert abs(r.state["pilot"]["projected_s"] - C.PILOT_REMAINING_S) < 5
    assert (C.PILOT_REMAINING_S, C.PILOT_NOMINAL_S, C.PILOT_LIMIT_S) == (11.5 * 60, 72.0, 45 * 60)


def test_official_data_protected():
    """Rule, revision of Oct 29, 2026 (the official data): the run of b3b88b6 is archived unchanged, the run after the
    revision writes to test/results-q32/ only, and neither the runner nor the analysis writes into the archive or the
    original test/results/."""
    assert C.archive_check() is None, C.archive_check()
    # the folder was 09-interning at b3b88b6 (renamed 10-interning later): the path below is the one at that commit
    for name in ("counters.csv", "results.md", "rounds.csv", "verdicts.csv"):     # as committed at b3b88b6
        p = subprocess.run(["git", "show", f"{C.OFFICIAL_Q64_COMMIT}:09-interning/test/results/{name}"], cwd=C.ROOT,
                           capture_output=True)
        if p.returncode == 0:
            assert p.stdout == (C.ARCHIVE / name).read_bytes(), name
    assert C.RESULTS == C.HERE / "results-q32" and not C.protected(C.RESULTS) and not C.protected(C.BUILD / "toy")
    for q in C.PROTECTED:
        assert C.protected(q) and C.protected(q / "results.md")
    r = runner.Runner(ns(toy=False))
    assert r.out == C.RESULTS
    r.out = C.ARCHIVE
    try:
        r.preflight()
    except SystemExit as e:
        assert "official data" in str(e), e
    else:
        raise AssertionError("a run writing into the archive was not refused")
    # start() checks write-protection before it creates the output folder
    r.out = C.ARCHIVE / "never-created"
    try:
        r.start()
    except SystemExit as e:
        assert "official data" in str(e), e
    else:
        raise AssertionError("start() into the archive was not refused")
    assert not r.out.exists(), "start() created a folder inside the archive before refusing"
    for q in C.PROTECTED:
        try:
            analyze.report(q)
        except SystemExit as e:
            assert "official data" in str(e)
        else:
            raise AssertionError(f"a report into {q} was not refused")
    off, src = analyze.official_q64()
    assert off and C.ARCHIVE.name in src
    assert C.archive_check() is None


def test_q32_gcd_vs_time():
    """Rule item 12 for the Q32 rows: gcd work and multiplications against ctl from the row's own counts, beside its
    time verdict against ctl; over-capacity jobs reported as such."""
    lines = ["ctr add core gcd_work 50", "ctr add core mul 80"]
    st = {"q32_over_capacity": {"5": 40849},
          "counts": {"L1q:1": {"counts": lines}, "L1oq:1": {"counts": lines},
                     "ctl:1": {"counts": ["ctr add core gcd_work 100", "ctr add core mul 100"]}}}
    d = analyze.Data(st)
    rows = [dict(job=1, X="L1-Q32", Y="ctl", measure="time", verdict="tie", ratio=1.0),
            dict(job=1, X="L1-opt-Q32", Y="ctl", measure="time", verdict="win", ratio=1.5)]
    out = "\n".join(analyze.gcd_vs_time_section(d, rows, [1, 5]))
    assert "| 1 | L1-Q32 | ctl | win 0.500 | win 0.800 | tie 1.000 | yes |" in out, out
    assert "| 1 | L1-opt-Q32 | ctl | win 0.500 | win 0.800 | win 1.500 | no |" in out, out
    assert f"| 5 | L1-Q32 | ctl | {analyze.OVER} |" in out
    assert "Candidates: job 1 (L1-Q32 against ctl)." in out


def test_idle_check():
    """R-13: a process spinning on a core makes the machine busy; a busy machine is waited on, logged and then refused
    before any timed run; an idle one passes at once; a toy run logs and goes on. (The probe is injected for the wait
    logic, so this test does not depend on the machine being idle.)"""
    spin = subprocess.Popen([sys.executable, "-c", "while True: pass"])
    try:
        runner.time.sleep(0.3)
        idle, d = runner.idle_probe(sample_s=0.5)
        assert not idle and any(b["pid"] == spin.pid for b in d["busy"]), d
    finally:
        spin.kill()
        spin.wait()
    with tempfile.TemporaryDirectory() as t:
        def fresh(toy):
            r = runner.Runner(ns(toy=toy))
            r.out = Path(t)
            r.state_file = r.out / "state.json"
            r.state = {"elapsed_before": 0}
            r.t0 = runner.time.monotonic()
            return r
        busy = (False, {"load1": 3.0, "load_max": C.IDLE_LOAD_MAX, "busy": [{"pid": 1, "name": "x", "cpu": 1.0}],
                        "proc_cpu_max": C.IDLE_PROC_CPU_MAX})
        old = (C.IDLE_WAIT_S, C.IDLE_POLL_S)
        C.IDLE_WAIT_S, C.IDLE_POLL_S = 0.05, 0.01
        try:
            r = fresh(False)
            r.idle_probe = lambda: busy
            try:
                r.wait_idle("the pilot")
                raise AssertionError("a busy machine was not refused")
            except runner.RunStop as e:
                assert "not idle" in str(e)
            assert len(r.state["idle_checks"]) >= 2 and not r.state["idle_checks"][-1]["idle"]
            assert "refusing to time" in (Path(t) / "run.log").read_text()
            seq = [busy, (True, dict(busy[1], load1=0.1, busy=[]))]
            r = fresh(False)
            r.idle_probe = lambda: seq.pop(0)
            r.wait_idle("the calibration and rounds of job 3")      # waits once, then passes
            assert [c["idle"] for c in r.state["idle_checks"]] == [False, True]
            r = fresh(True)                                         # toy runs log and go on
            r.idle_probe = lambda: busy
            r.wait_idle("the pilot")
            assert "toy run: not waiting" in (Path(t) / "run.log").read_text()
        finally:
            C.IDLE_WAIT_S, C.IDLE_POLL_S = old
    assert (C.IDLE_LOAD_MAX, C.IDLE_PROC_CPU_MAX, C.IDLE_WAIT_S) == (0.5, 0.05, 600.0)


def test_hard_stop():
    """A toy run with a 3-second hard stop: stopped, recorded, no verdict."""
    out = C.BUILD / "toy" / "results"
    p = subprocess.run([sys.executable, "runner.py", "--toy", "--jobs", "3", "--hard-stop-s", "3"], cwd=C.HERE,
                       capture_output=True, text=True, timeout=600)
    st = json.loads((out / "state.json").read_text())
    assert st.get("hard_stopped"), p.stdout[-2000:]
    assert "No verdict is reported" in (out / "results.md").read_text()


# ------------------------------------------------------------------------------------------------ differential
def build_all(bindir):
    """Every row binary in every build it has, as the runner builds them."""
    r = runner.Runner(ns())
    r.bin = Path(bindir)
    r.build()
    return r


def random_workload(rng, shape):
    """Entries then operations on earlier values, within the 08 operand rule, every op, zeros and repeats."""
    b = gen.Builder()
    n_in = rng.randint(20, 120)
    for _ in range(n_in):
        if shape == 0:
            m, d = rng.randint(-6, 6), rng.randint(1, 6)
        elif shape == 1:
            m, d = rng.randint(-10 ** 6, 10 ** 6), rng.randint(1, 10 ** 6)
        else:
            m, d = rng.randint(-10 ** 4, 10 ** 4) * 3, 100
        b.enter(m, d)
    for _ in range(rng.randint(50, 400)):
        for _ in range(100):
            code = rng.choice((C.ADD, C.SUB, C.MUL, C.DIV)) if shape != 2 else rng.choice((C.ADD, C.SUB))
            x = rng.randrange(len(b.op))
            y = x if rng.random() < 0.15 else rng.randrange(len(b.op))
            if code == C.DIV and b.val[y] == 0:
                continue
            if gen.H(b.val[x]) * gen.H(b.val[y]) > b.bound:
                continue
            b.operate(code, x, y)
            break
    return b, n_in


def test_differential_all_rows():
    """Every row (L1, L1-opt, L1-Q32, L1-opt-Q32, ctl in their three builds; the lookup gate's build; fmpq and mpq in
    forms A and B; fz on the money shape) against Python fractions, on random workloads; also W against the distinct
    magnitudes, and the predicted cases and trick counters in the gate; each Q32 row's gate and counting runs against
    its Q64 row's (spec §18.3 item 1), and the lookup gate's against L1's (spec §18.1 item 4)."""
    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        r = build_all(td / "bin")
        rng = random.Random(20261029)
        nw = 0
        for t in range(36):
            shape = t % 3
            b, n_in = random_workload(rng, shape)
            wl = td / "w.wl"
            C.write_workload(wl, 99, 64, shape == 2, 0, 0, n_in, b.op, b.a, b.b)
            refn = np.array([v.numerator for v in b.val], dtype=np.int64)
            refd = np.array([v.denominator for v in b.val], dtype=np.int64)
            cases, pred = gen.predict(b, n_in)
            runs = []
            for impl in ("L1", "L1opt", "L1q", "L1oq", "ctl"):
                runs += [([r.exe(impl, "timing"), "time", wl, td / "o", 1, 1], impl),
                         ([r.exe(impl, "counting"), "count", wl, td / "o", td / f"p-{impl}"], impl),
                         ([r.exe(impl, "test"), "gate", wl, td / "o", td / f"w-{impl}"], impl),
                         ([r.exe(impl, "timing"), "mem", wl, td / "o"], impl)]
            runs.append(([r.exe("L1g", "test"), "gate", wl, td / "o", td / "w-L1g"], "L1g"))
            for impl in ("fmpq", "mpq"):
                for form in "AB":
                    runs.append(([r.exe(impl, "timing"), "time", wl, td / "o", 1, 1, form], impl))
                runs.append(([r.exe(impl, "timing"), "mem", wl, td / "o"], impl))
            if shape == 2:
                runs += [([r.exe("fz", "timing"), "time", wl, td / "o", 1, 1], "fz"),
                         ([r.exe("fz", "timing"), "mem", wl, td / "o"], "fz")]
            lines = {}
            for cmd, impl in runs:
                p = subprocess.run([str(x) for x in cmd], capture_output=True, text=True)
                assert p.returncode == 0, (cmd, p.stdout[-500:], p.stderr[-500:])
                on, od = C.read_pairs(td / "o")
                bad = np.nonzero((on != refn) | (od != refd))[0]
                assert len(bad) == 0, (cmd, int(bad[0]), on[bad[0]], od[bad[0]], b.val[bad[0]])
                if cmd[1] in ("gate", "count"):
                    lines[(cmd[1], impl)] = [l for l in p.stdout.splitlines() if l.startswith(("ctr ", "xctr "))]
                if cmd[1] == "gate" and impl in C.LAYER1 + C.GATE_ONLY:
                    wl_ = {l.split()[1]: int(l.split()[2]) for l in p.stdout.splitlines() if l.startswith("wholes ")}
                    assert wl_["end"] == pred["W_end"] == gen.distinct_wholes(b.val)
                    tot, _ = r.parse_counts(p.stdout.splitlines())
                    for ph, cc in pred["case_counts"].items():
                        for k, v in cc.items():
                            assert tot.get((ph, "case_" + k), 0) == v
                    if impl in C.OPT_IMPLS:
                        for ph, d in pred["tricks"].items():
                            for k, v in d.items():
                                assert tot.get((ph, k), 0) == v, (ph, k, tot.get((ph, k)), v)
                    else:                                # the model's own L1 counts, an extra check
                        for ph, d in pred["l1_base"].items():
                            for k, v in d.items():
                                assert tot.get((ph, k), 0) == v, (ph, k, tot.get((ph, k)), v)
                nw += 1
            # spec §18.3 item 1: each Q32 row's counters equal its Q64 row's (O-7's bytes halved), with the same
            # whole-number list and per-call counts; spec §18.1 item 4: the lookup gate's equal L1's
            for q32, q64 in C.Q64_OF.items():
                for mode in ("gate", "count"):
                    assert runner.Runner.scaled_o7(lines[(mode, q64)], 1, 2) == lines[(mode, q32)], (q32, mode)
                assert (td / f"w-{q32}").read_bytes() == (td / f"w-{q64}").read_bytes()
                assert (td / f"p-{q32}").read_bytes() == (td / f"p-{q64}").read_bytes()
            assert lines[("gate", "L1g")] == lines[("gate", "L1")]
            assert (td / "w-L1g").read_bytes() == (td / "w-L1").read_bytes()
        print(f"  differential: {nw} invocations, every stored value equal to the exact reference; Q32 rows equal "
              f"to their Q64 rows; the lookup gate's counts equal L1's")


# ------------------------------------------------------------------------------------------------ full size
def full_predictions(outdir):
    """The generator's predicted figures matched exactly at full size, outside any run: gate runs of L1 and L1-opt
    (W, cases, trick counters; L1-opt against L1) and counting runs (cases den and general against ctl)."""
    outdir = Path(outdir)
    if outdir.exists():
        shutil.rmtree(outdir)
    r = runner.Runner(ns())
    r.bin, r.wl, r.tmp, r.out = outdir / "bin", outdir / "work", outdir / "readout", outdir
    r.tmp.mkdir(parents=True)
    r.build()
    metas = gen.generate(r.wl, sorted(C.JOBS))
    over = {str(j): m["predicted"]["W_end"] for j, m in metas.items() if m["predicted"]["W_end"] > C.Q32_CAPACITY}
    r.state = {"gates": {}, "counts": {}, "elapsed_before": 0.0, "q32_over_capacity": over}
    r.t0 = runner.time.monotonic()
    summary = []
    for job in sorted(C.JOBS):
        for impl in C.COUNTED + C.GATE_ONLY:
            if impl in C.Q32_IMPLS and not r.q32_fits(job):
                continue
            g = r.gate(impl, job, C.HARNESS_CORE)
            assert "fail" not in g, (impl, job, g.get("fail"))
            r.state["gates"][f"{impl}:{job}"] = g
            if impl in C.GATE_ONLY:
                continue
            c = r.count(impl, job, C.HARNESS_CORE)
            assert "bug" not in c and "na" not in c, (impl, job, c)
            r.state["counts"][f"{impl}:{job}"] = c
        for opt in C.OPT_IMPLS:
            if opt in C.Q32_IMPLS and not r.q32_fits(job):
                continue
            fails = r.against_l1(job, opt)
            assert not fails, (job, opt, fails)
        if r.q32_fits(job):
            for q32 in C.Q32_IMPLS:
                for key in ("gates", "counts"):
                    fails = r.against_q64(job, q32, counts_key=key)
                    assert not fails, (job, q32, key, fails)
        fails = r.lookup_gate(job)                       # spec §18.1 item 4, on every job
        assert not fails, (job, "lookup gate", fails)
        fails, sums = r.percase_check(job)
        assert not fails, (job, fails)
        p = metas[job]["predicted"]
        tr = sum(v for d in p["tricks"].values() for k, v in d.items() if k.startswith(("o1_", "o2_")))
        probes = sum(int(l.split()[4]) for l in r.state["gates"][f"L1g:{job}"]["counts"]
                     if l.split()[3] == "lookup_probe")
        summary.append(f"job {job}: W {p['W_after_entry']}/{p['W_end']}, cases {sum(sum(d.values()) for d in p['case_counts'].values())}, "
                       f"skipped lookups {tr}; lookup gate: {probes} probes, equal to sk_lookup's; Q32 rows: " +
                       (f"over capacity (W {p['W_end']})" if not r.q32_fits(job) else "equal to Q64"))
    print("  full-size predictions matched exactly:\n    " + "\n    ".join(summary))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--full", default=None)
    a = ap.parse_args()
    tests = [test_rule_refusal, test_vendor08, test_generator_deterministic, test_bands, test_bootstrap_paired,
             test_code_hashes_invalidate, test_pilot_stop_rule, test_official_data_protected, test_q32_gcd_vs_time,
             test_idle_check, test_differential_all_rows,
             test_hard_stop]
    for t in tests:
        t()
        print(f"ok {t.__name__}")
    if a.full:
        full_predictions(a.full)
        print("ok full_predictions")


if __name__ == "__main__":
    main()
