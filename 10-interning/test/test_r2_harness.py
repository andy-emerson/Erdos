"""Tests of round 2's harness (not measurements; nothing here is timed or reported as a time).

    python3 test_r2_harness.py            the self-tests: the fixed rule and its pin, the 08 module, the official data,
                                          the workloads, the binaries' exact checks on toy workloads, the profiling
                                          build's results and accounting, the slowdown limit, statistics, the pilot's
                                          projection and stops, fork/exec with the affinity set before exec, the kernel
                                          timer, the idle check, the disturbance check (rule-r2 item 5, revision of Oct
                                          29, 2026: synthetic; a host's short-lived threads do not disturb; a
                                          deliberately started short-lived outside process does; the host allowance
                                          computed from the pilot's data), the timed block's machinery with a stand-in
                                          driver (re-run once, the harness-bug limit, over cap, a bug, the CPU time per
                                          rep), and the analysis
    python3 test_r2_harness.py --full     also every gate and counting run on the full-size workloads of the 8 jobs
                                          (test and counting builds; untimed): every counter against the prediction,
                                          the read-outs, and Q32 against TC call for call
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

import r2_analyze as A
import r2_checks as K
import r2_common as C
import r2_gen
import r2_libconf as L
import r2_runner as RN

TB = C.HERE / "build" / "r2test"
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
        r2_gen.generate(w, list(C.RUN_JOBS), toy=True)
    return w


# ------------------------------------------------------------------------------------------------- the rule, data
def test_rule_fixed_and_pinned():
    """the rule on disk is the pinned one; while it holds a PROPOSED mark (as the revision of Oct 29, 2026 did before the
    author) a measured run is refused for that reason alone; once the marks are approved the pin must be renewed"""
    text = C.RULE_FILE.read_text()
    assert C.sha256(C.RULE_FILE) == C.RULE_SHA256, "rule-r2.md differs from the pinned hash"
    if C.PROPOSAL_MARK in text:
        assert "still holds the mark" in C.rule_check(), C.rule_check()
        try:
            RN.Runner(ns(toy=False)).preflight()
        except SystemExit as e:
            assert "refusing a measured run" in str(e) and C.PROPOSAL_MARK in str(e)
        else:
            raise AssertionError("a measured run under a rule with PROPOSED marks was not refused")
        text = text.replace(C.PROPOSAL_MARK, "approved by the author, Oct 29, 2026")   # as if approved
    else:
        assert C.rule_check() is None, C.rule_check()
    assert "approved by the author, Oct 29, 2026" in text
    assert "Gcd work: a check, not a verdict** (approved by the author, Oct 29, 2026)" in text
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule-r2.md"
        p.write_text(text + " ")
        assert "sha256" in C.rule_check(p)
        p.write_text(text.replace("approved by the author, Oct 29, 2026", "PROPOSED", 1))
        assert "PROPOSED" in C.rule_check(p)
    saved = C.RULE_SHA256
    try:
        C.RULE_SHA256 = "0" * 64
        try:
            RN.Runner(ns(toy=False)).preflight()
        except SystemExit as e:
            assert "refusing a measured run" in str(e)
        else:
            raise AssertionError("a measured run under another rule was not refused")
    finally:
        C.RULE_SHA256 = saved


def test_vendor08_and_official_data():
    assert L.vendor08_check() is None
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
    assert not C.protected(C.RESULTS)


def test_workloads_identical_to_round1():
    with tempfile.TemporaryDirectory() as t:
        s = r2_gen.generate(t, [3, 10, 5])
        assert s["3"]["identical_to_round1"] and s["10"]["identical_to_round1"] and s["5"]["identical_to_round1"]
        assert s["5"]["over_capacity"] and not s["3"]["over_capacity"]


def test_runner_imports_no_numpy():
    r = subprocess.run([sys.executable, "-c", "import sys, r2_runner; print('numpy' in sys.modules)"], cwd=C.HERE,
                       capture_output=True, text=True)
    assert r.stdout.strip() == "False", r.stdout + r.stderr


# ------------------------------------------------------------------------------------------------- the binaries
def test_gates_and_counts_toy():
    """every toy job: the test build's gate passes its exact checks against the model; the counting runs of Q32 and
    TC have equal arithmetic, call for call, and equal gcd work"""
    build_once()
    w = toy_work()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for job in C.RUN_JOBS:
            pred = json.loads((w / f"job-{job:02d}.r2.json").read_text())["predicted"]
            G = {}
            for impl in C.IMPLS:
                p = subprocess.run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out",
                                    t / "w.bin"], capture_output=True, text=True)
                assert p.returncode == 0, p.stdout[-500:] + p.stderr
                assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
                fails = K.exact_checks(impl, p.stdout.splitlines(), pred, C.sha256(t / "w.bin") if impl == "Q32" else None)
                assert not fails, (job, impl, fails[:5])
                p = subprocess.run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                                    t / f"p-{impl}.bin"], capture_output=True, text=True)
                assert p.returncode == 0
                G[impl] = K.gcd_work(p.stdout.splitlines())
                p = subprocess.run([BIN / f"bench-{impl}-timing", "mem", w / f"job-{job:02d}.wl", t / "m.out"],
                                   capture_output=True, text=True)
                assert p.returncode == 0 and "memrun end" in p.stdout
                assert K.readout_check(t / "m.out", w / f"job-{job:02d}.ref") is None
            assert (t / "p-Q32.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), job
            assert G["Q32"] == G["TC"] > 0


def test_exact_checks_catch_a_difference():
    build_once()
    w = toy_work()
    pred = json.loads((w / "job-03.r2.json").read_text())["predicted"]
    with tempfile.TemporaryDirectory() as t:
        p = subprocess.run([BIN / "bench-Q32-test", "gate", w / "job-03.wl", Path(t) / "g.out", Path(t) / "w.bin"],
                           capture_output=True, text=True)
        lines = p.stdout.splitlines()
        assert not K.exact_checks("Q32", lines, pred, C.sha256(Path(t) / "w.bin"))
        bad = json.loads(json.dumps(pred))
        bad["counters"]["mul"]["k15_ops"] = bad["counters"]["mul"].get("k15_ops", 0) + 1
        assert K.exact_checks("Q32", lines, bad)
        bad = dict(pred, W_end=pred["W_end"] + 1)
        assert K.exact_checks("Q32", lines, bad)
        # a read-out that differs in one value is caught, with the call
        ref = (w / "job-03.ref").read_bytes()
        broken = bytearray(ref)
        broken[8 * 5] ^= 1
        (Path(t) / "x.out").write_bytes(bytes(broken))
        r = K.readout_check(Path(t) / "x.out", ref)
        assert r and r["call"] == 5 and r["mismatches"] == 1


def test_profiling_build():
    """The profiling build changes no result (its read-out is the reference), samples about one call in K with random
    gaps, attributes every sampled call to a job phase and case, and charges every growth to M; the slowdown limit
    flags an invocation over 1.5. No time is reported."""
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
                if impl == "Q32":
                    assert rep["m"], "growths are charged to M"
            # every phase the row has, seen; TC has no I
            seen = {ph for rep in pr["reps"].values() for (jp, k, ph) in rep["cyc"]}
            assert {0, 1, 2, 3, 5} <= seen and ((4 in seen) == (impl == "Q32"))
        for b in ("timing", "counting", "test"):                # not built in the other builds
            p = subprocess.run([BIN / f"bench-Q32-{b}", "prof", w / "job-03.wl", Path(t) / "p.out", "1", "1", "8", "1"],
                               capture_output=True, text=True)
            assert p.returncode != 0
    s, ok = K.slowdown(1.6, 1.0)
    assert s == 1.6 and not ok
    s, ok = K.slowdown(1.5, 1.0)
    assert ok
    assert C.PROF_SLOWDOWN_MAX == 1.5 and C.PROF_K == 32 and C.PROF_INVOCATIONS == 3


# ------------------------------------------------------------------------------------------------- statistics
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
    assert K.pilot_projection(60, 30, 1) == 60 + 300 + 36
    assert K.pilot_projection(60, 60, 1) == 60 + 600 + 36
    assert K.host_allowance(0.5, 10) == 5 + 10 and K.host_allowance(0.0, 10) == 5
    assert K.host_allowance(1.25, 10) == 5 + 25 and C.HOST_RATE_CEILING == 4.0           # approved Oct 29, 2026
    assert C.HOST_BASE_TICKS == 5 and C.HOST_FACTOR == 2 and C.HOST_RATE_MIN_TICKS == 1
    assert K.other_limit(1.0) == 6 and K.other_limit(10.0) == 15 and K.other_limit(0.3) == 6
    assert C.PILOT_LIMIT_S == 22 * 60 and C.HARD_STOP_S == 30 * 60 and C.PILOT_JOBS == (1, 3, 10)


def pilot_runner(rates, slow, P_extra=0.0):
    """a runner whose pilot blocks are stand-ins; rates: one host rate for every block, or a list of
    {host pid: (ticks, seconds)} per block, in block order"""
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
        if kind == "rounds":
            return [{"role": "calibration", "row": "Q32", "job": job, "r": 4, "reps": 3},
                    {"role": "calibration", "row": "TC", "job": job, "r": 4, "reps": 3}] + \
                   [{"role": "round", "row": row, "job": job, "total": 100.0} for row in C.ROWS for _ in range(2)]
        return [{"role": "profile", "row": row, "job": job, "total": 100.0 * slow} for row in C.PROF_ROWS]
    r.timed_block = block
    return r


def test_pilot_stops():
    r = pilot_runner(0.1, 1.2)
    r.step3_pilot()                                       # goes on
    assert r.state["pilot"]["slowdown"]["Q32"]["slowdown"] == 1.2
    r = pilot_runner(1.25, 1.2)                           # the stopped run's host rate: under the ceiling, goes on
    r.step3_pilot()
    r = pilot_runner(4.0, 1.2)                            # at the ceiling of 4 ticks per second: goes on
    r.step3_pilot()
    r = pilot_runner(0.0, 1.2)                            # no tick in 1.6 s: R_h is 1 tick over 1.6 s, and goes on
    r.step3_pilot()
    assert r.state["host_rates"] == {"1": 1 / 1.6} and r.state["host_rates_measured"] == {"1": 0.0}
    for rates, slow, extra, why in ((4.5, 1.2, 0, "ceiling"), (0.1, 1.6, 0, "slowdown"),
                                    (0.1, 1.2, 20 * 60, "projected")):
        r = pilot_runner(rates, slow, extra)
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


# ------------------------------------------------------------------------------------------------- spawning, timer
def test_spawner_affinity_before_exec():
    sp = RN.Spawner()
    with tempfile.TemporaryDirectory() as t:
        out = Path(t) / "o.txt"
        rc = sp.run(["/bin/grep", "Cpus_allowed_list", "/proc/self/status"], out, C.TIMED_CORE)
        assert rc == 0 and out.read_text().split()[-1] == str(C.TIMED_CORE), out.read_text()
        rc = sp.run(["/nonexistent/binary"], out, C.TIMED_CORE)
        assert rc == 127
        assert sp.spawns == 2 and sp.child is None


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
# a stand-in for bench_r2 (harness test only): copies the reference as its read-out, prints made-up lines
import sys, os, shutil
mode, wl, ro = sys.argv[1], sys.argv[2], sys.argv[3]
row = "Q32" if "Q32" in sys.argv[0] else "TC"
cfg = os.environ.get("FAKE_CFG", "")
shutil.copy(wl[:-3] + ".ref", ro)
if "bug" in cfg and row == "TC":
    with open(ro, "r+b") as f:
        f.seek(0); b = f.read(1); f.seek(0); f.write(bytes([b[0] ^ 1]))
if mode == "calib":
    for r in (1, 2, 4):
        print(f"calib {r} {r * 20000000} 1 2 3 row={row} cpu={r * 19} span={r * 21}")
    if "cap" in cfg and row == "TC":
        print("overcap 20000000000")
elif mode == "time":
    r, reps = int(sys.argv[4]), int(sys.argv[5])
    for i in range(reps):
        print(f"rep {i} {r * (1000 + i)} {r * 100} {r * 800} {r * (100 + i)} row={row} cpu={r * (990 + i)} "
              f"span={r * (1100 + i)}")
elif mode == "prof":
    r = int(sys.argv[4])
    print("probe_cost 30.0"); print("tsc_hz 2000000000.0"); print("prof_k 32")
    for i in range(3):
        print(f"prep {i} {r * 1200} {r * 120} {r * 960} {r * 120} sampled=10 row={row}")
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
            assert [x["row"] for x in cal] == ["Q32", "TC"] and cal[0]["r"] == 4 and cal[0]["reps"] == 3
            assert len(rnd) == 3 * C.ROUNDS and seen[-1]["spawns"] == 2 + 3 * C.ROUNDS
            assert [x["row"] for x in rnd[:6]] == ["Q32", "Q32-null", "TC", "Q32-null", "TC", "Q32"]  # rotated
            x = rnd[0]
            assert x["r"] == 4 and x["total"] == 1001 and x["read"] == 101 and "bug" not in x   # reps 2-3, minimum
            assert x["cpu_ns"] == 991 and x["span_ns"] == 1101 and x["cpu_share"] == 991 / 1101   # the kept rep's
            assert x["reps_cpu"] == [(4 * 990, 4 * 1100), (4 * 991, 4 * 1101), (4 * 992, 4 * 1102)]   # every rep's
            assert cal[0]["reps_cpu"] == [(r * 19, r * 21) for r in (1, 2, 4)], cal[0]          # calibration reps too
            assert not list(r.tmp.glob("i*")), "the invocation files are removed after the block"
            assert all(threading.active_count() == 1 for _ in [0])
            r.state["calibration"] = {"3:Q32": {"r": 4}, "3:TC": {"r": 4}}
            precs = r.timed_block(3, "prof", "profile")
            assert [x["row"] for x in precs] == ["Q32", "TC"] * 3
            assert precs[0]["total"] == 1200.0 and precs[0]["profile"]["sampled"] == 10       # reps 2-3, minimum
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


def test_block_rerun_once_and_harness_bug():
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
        RN.disturbance = lambda *a, **k: dict(real(*a, **k), disturbed=False, reasons=[], runner_ticks=3)
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            try:
                r.timed_block(10, "rounds", "measured")
            except RN.RunStop as e:
                assert "harness bug" in str(e) and r.state["harness_bugs"]
            else:
                raise AssertionError("3 runner ticks in a block were not a harness bug")
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
    """a finished state gives per-measure verdicts, the gcd-work check and the profile; a stopped one gives none"""
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        recs = []
        for k in range(1, 13):
            for row, tot in (("Q32", 100.0), ("Q32-null", 100.5), ("TC", 150.0)):
                recs.append({"job": 3, "row": row, "role": "round", "round": k, "total": tot + k * 0.1,
                             "entry": 10.0, "ops": tot - 20, "read": 10.0})
        mem = ["mem end whole numbers 2800 4096", "mem end lookup 1024 1024", "mem end rationals 4000 4096",
               "memN end 1000", "mem after_entry whole numbers 64 128", "mem after_entry lookup 32 32",
               "mem after_entry rationals 400 512", "memN after_entry 100"]
        prof = {"total": 1200, "entry": 120, "ops": 960, "read": 120, "sampled": 10,
                "cyc": [[1, "add_den", ph, 1000, 10] for ph in range(6)], "calls": [[1, "add_den", 10, 10000]],
                "m": [[1, 500, 2]]}
        st = {"started": "x", "finished": "y", "jobs": [3], "elapsed_saved": 300, "rule_sha256": C.RULE_SHA256,
              "records": recs, "calibration": {}, "toy": True,
              "counts": {"Q32:3": {"mem": mem, "counts": ["ctr add core gcd_work 500"]},
                         "TC:3": {"mem": ["mem end pairs 16000 16384", "memN end 1000",
                                          "mem after_entry pairs 1600 2048", "memN after_entry 100"],
                                  "counts": ["ctr add core gcd_work 500"]}},
              "mems": {"Q32:3": {"memrun": {"end": {"heap": 9000, "stored": 1000},
                                            "after_entry": {"heap": 600, "stored": 100}}},
                       "TC:3": {"memrun": {"end": {"heap": 17000, "stored": 1000},
                                           "after_entry": {"heap": 1700, "stored": 100}}}},
              "call_checks": {"3": {"call_for_call": "pass", "G_Q32": 500, "G_TC": 500, "G_ratio": 1.0,
                                    "gcd_work_check": "passed"}},
              "gen": {"3": {"n": 1000, "phase_counts": {"entry": 100, "add": 900}}},
              "profiles": [{"job": 3, "row": row, "profile": prof, "r": 4, "tsc_hz": 2e9, "probe_cost": 30.0, "K": 32,
                            "total": 300.0, "entry": 30.0, "ops": 240.0, "read": 30.0, "slowdown": 1.1,
                            "within_limit": True} for row in C.PROF_ROWS for _ in range(3)]}
        (t / "state.json").write_text(json.dumps(st))
        (t / "results.md").write_text("# head\n<!-- rule ends -->\nold tail\n")
        A.report(t)
        md = (t / "results.md").read_text()
        assert "old tail" not in md and "| 3 SMALL8 | win, 1.49" in md, md[:3000]
        assert "passed (G(Q32)/G(TC) = 1.000000" in md and "indicative only" in md
        v = list(csvrows(t / "verdicts.csv"))
        assert {(x["measure"], x["verdict"]) for x in v} >= {("time", "win"), ("B_used", "win"), ("B_heap", "win"),
                                                             ("gcd work (check)", "passed")}
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
                "cyc": [[1, "add_den", ph, 1000 * r, 10 * r] for ph in range(6)],
                "calls": [[1, "add_den", 10 * r, 10000 * r]], "m": [[1, mcyc, 2 * r]]}
        return {"profile": prof, "r": r, "tsc_hz": 2e9, "probe_cost": 30.0, "K": 32,
                "total": 1200.0, "entry": 120.0, "ops": 960.0, "read": 120.0}
    pc = {"entry": 10, "add": 100}
    f4, f1 = A.profile_figures(rec(4, 800), pc), A.profile_figures(rec(1, 200), pc)
    for f in (f4, f1):
        assert abs(f["parts"]["operations"]["M_ns_per_op"] - 1.0) < 1e-9, f["parts"]["operations"]
        assert abs(f["parts"]["operations"]["batch_ns_per_op"] - 9.6) < 1e-9
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
        s = r2_gen.generate(t / "work", list(C.ALL_JOBS))
        assert all(m["identical_to_round1"] for m in s.values())
        for job in C.RUN_JOBS:
            w = t / "work"
            pred = json.loads((w / f"job-{job:02d}.r2.json").read_text())["predicted"]
            for impl in C.IMPLS:
                p = subprocess.run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out",
                                    t / "w.bin"], capture_output=True, text=True)
                assert p.returncode == 0
                assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
                f = K.exact_checks(impl, p.stdout.splitlines(), pred, C.sha256(t / "w.bin") if impl == "Q32" else None)
                assert not f, (job, impl, f[:5])
                p = subprocess.run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                                    t / f"p-{impl}.bin"], capture_output=True, text=True)
                assert p.returncode == 0
            assert (t / "p-Q32.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
            print(f"  full size: job {job} gates pass, Q32 and TC equal call for call")


TESTS = [test_rule_fixed_and_pinned, test_vendor08_and_official_data, test_workloads_identical_to_round1,
         test_runner_imports_no_numpy, test_gates_and_counts_toy, test_exact_checks_catch_a_difference,
         test_profiling_build, test_statistics_and_verdicts, test_pilot_projection_and_allowance, test_pilot_stops,
         test_spawner_affinity_before_exec, test_hard_stop_kernel_timer, test_idle_probe, test_disturbance_synthetic,
         test_host_threads_do_not_disturb, test_disturbance_catches_short_lived_process, test_host_allowance_from_pilot,
         test_existing_child_exit_charged_for_the_block_only,
         test_timed_block_machinery, test_block_rerun_once_and_harness_bug,
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
