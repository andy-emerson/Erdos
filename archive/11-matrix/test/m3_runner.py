"""The runner of research/rule-m3.md (round m3: TC, Hash-V-pack, Pack-primes, Pack-coprime and Hash-V-pack-null, spec
v0.3). From m2_runner.py (unchanged, m2's), whose structure it keeps, and m1_runner.py, whose process code it imports and
uses unchanged (rule-m3, Harness: "Its spawner, checks, calibration, bootstrap and stops are unchanged"): the fork/exec
spawner pinned before exec, the single-threaded runner during timed phases, the kernel timer for the hard stop, the
idle check, the disturbance check by process with the pilot's host allowance. What changes (rule-m3, Harness): the
rows of v0.3; the grid of 12 jobs, and sop's 9; the model with factorizations, trial-division counts and Pack-coprime's
simulation; the adapted call-for-call check; the pairings and the reported gcd work of the factored rows; the pilot's
jobs 1, 6 and 10, and its projection; the new counters and accounted structures; rule-m3's pin. A row whose
calibration rep passes 10 s runs no rounds on that job (over cap, item 1).

THE OPERATOR (rule-m3 item 5, carried over) starts each run ONCE, as its own command that returns only when it exits:
    python3 test/m3_runner.py --op add
then, after its exit notice, --op mul, --op sop and --op pos, in that order, without waiting for the author between
them (rule-m3, Run order); a run that stops does not stop the next one. Each is either in the foreground, or a
background command that notifies the operator when it exits (for Claude Code: the Bash tool with run_in_background and
a timeout of at least 45 minutes). Until that exit notice the operator runs NOTHING else on the machine: no polling or
waiting loop, no sleep, watch, tail -f, file check, Monitor or until-loop, no other tool call, build, test or edit, and
no detached process. There is no option that runs the four in one command (DECISIONS-m2 M2-H2, carried over).

    python3 m3_runner.py --op add|mul|sop|pos
                                           one run: results in test/results-m3-<op>/ (refuses unless rule-m3.md is the
                                           fixed rule, the 08 module and tc3.c are unchanged, tcm.c's diff is the
                                           recorded one, m1's and m2's sources are unchanged, round 3's results are
                                           unchanged, and the machine has 2 cores; never writes in m1's or m2's results
                                           folders or in another m3 run's folder, and checks them before and after)
    ... --pilot-only                       steps 0-3, then stop
    ... --resume                           continue a run stopped before step 6 (a failed gate, or --pilot-only):
                                           re-runs the unit tests and every gate not passed, then goes on
    ... --rerun TC|HASH|FAC                rule-m3 item 12: after a bug is fixed under review, re-run TC, every hash row
                                           and the null row, or the two factored rows, in full
    ... --toy [--jobs 1,6] [--hard-stop-s S]
                                           a harness test with toy workloads in test/build/m3toy/: never a measurement

The run plan (rule-m3, Run plan, as rule-m3's), once per run: 0 before the clock: workloads, references and
predictions, with their sha256 (add and mul: equal to m1's; sop and pos: equal to m2's); 1 build, the hashes of the 08
module, tc3.c and m1's and m2's sources, tcm.c's recorded diff, step 0's files checked; 2 unit tests (with the large
trial divisions, the allocation check and the sanitizers); 3 the pilot (jobs 1, 6, 10: calibration and 2 rounds), its
projection and stops; 4 gates (two at a time; the TC gate against tc3.c, every step); 5 counting and memory runs (two
at a time; the worker pool is then shut down and joined); 6 per job: the idle check, calibration (4 launches) and 12
rounds (60 launches, fewer for a row over cap), the disturbance check; 7 the analysis. The hard stop: 40 minutes from
step 1 to the end of step 6.
"""
import argparse
import concurrent.futures as cf
import json
import os
import queue
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

import m3_checks as K
import m3_common as C
import m3_libconf as L
# m1's process code, used unchanged (rule-m3 item 5 carries the disturbance check and the spawner over in full)
from m1_runner import (HardStop, RunStop, Spawner, ancestors, cpu_times, disturbance,  # noqa: F401
                       fork_counter, idle_probe, is_kthread, read_stat, snapshot, stamp, threads_of, ticks4)


class Runner:
    def __init__(self, a):
        self.a = a
        self.op = a.op
        self.toy = a.toy
        base = C.TOY_BUILD if a.toy else C.BUILD
        self.out = (base / f"results-{a.op}") if a.toy else C.RESULTS[a.op]
        self.wl = base / f"work-{a.op}"
        self.bin = base / "bin"
        self.tmp = base / f"inv-{a.op}"
        self.jobs = list(C.JOBS_OF[a.op]) if not a.jobs else [int(x) for x in a.jobs.split(",")]
        bad = [j for j in self.jobs if j not in C.JOBS_OF[a.op]]
        if bad:
            raise SystemExit(f"jobs {bad} are not in the {a.op} set's grid (rule-m3, Workloads)")
        self.hard_stop = C.HARD_STOP_S
        if a.hard_stop_s is not None:
            if not a.toy:
                raise SystemExit("--hard-stop-s is for toy runs only")
            self.hard_stop = a.hard_stop_s
        self.state_file = self.out / "state.json"
        self.state = None
        self.t0 = None
        self.code = L.code_hashes()
        self.spawner = Spawner()
        self.untimed_procs = set()
        self.lock = threading.Lock()
        self.idle_probe = idle_probe                          # replaceable in the harness tests
        self.seq = 0
        self.refs = {}
        self.hosts = {}
        self.protected_before = None

    # --------------------------------------------------------------------------------------- clock, state, log
    def elapsed(self):
        return self.state["elapsed_before"] + (time.monotonic() - self.t0)

    def remaining(self):
        return self.hard_stop - self.elapsed()

    def check_clock(self, where):
        if self.remaining() <= 0:
            raise HardStop(where)

    def save(self):
        self.state["elapsed_saved"] = self.elapsed() if self.t0 else self.state.get("elapsed_before", 0)
        tmp = self.state_file.with_suffix(".tmp")
        tmp.write_text(json.dumps(self.state, indent=0))
        tmp.replace(self.state_file)

    def log(self, msg):
        line = f"[{self.elapsed():8.1f}s] {msg}" if self.t0 else msg
        print(line, flush=True)
        with open(self.out / "run.log", "a") as f:
            f.write(line + "\n")

    # --------------------------------------------------------------------------------------- the kernel timer
    def arm(self):
        """setitimer(ITIMER_REAL) to the time left before the hard stop; its handler kills the child and ends the run."""
        signal.signal(signal.SIGALRM, self._alarm)
        signal.setitimer(signal.ITIMER_REAL, max(self.remaining(), 0.001))

    def disarm(self):
        signal.setitimer(signal.ITIMER_REAL, 0)

    def _alarm(self, signum, frame):
        if self.spawner.child:
            try:
                os.kill(self.spawner.child, signal.SIGKILL)
            except ProcessLookupError:
                pass
        for p in list(self.untimed_procs):
            try:
                p.kill()
            except Exception:
                pass
        raise HardStop(f"the {self.hard_stop / 60:g}-minute hard stop")

    def reap(self):
        if self.spawner.child:
            try:
                os.waitpid(self.spawner.child, 0)
            except ChildProcessError:
                pass
            self.spawner.child = None

    # --------------------------------------------------------------------------------------- files
    def exe(self, impl, build):
        return self.bin / f"bench-{impl}-{build}"

    def wlpath(self, job):
        return self.wl / f"job-{job:02d}.wl"

    def ref(self, job, name=None):
        key = (job, name)
        if key not in self.refs:
            self.refs[key] = (self.wl / (f"job-{job:02d}.ref" if not name else f"job-{job:02d}.{name}.ref")).read_bytes()
        return self.refs[key]

    def pred(self, job):
        return json.loads((self.wl / f"job-{job:02d}.m3.json").read_text())["predicted"]

    # --------------------------------------------------------------------------------------- untimed work
    def run_untimed(self, cmd, core, limit=None, env=None):
        """An untimed process (steps 1, 2, 4, 5), pinned to a core; returns (CompletedProcess or None if its own limit
        passed, wall seconds)."""
        self.check_clock("untimed work")
        rem = self.remaining()
        t = rem if limit is None else min(limit, rem)
        start = time.monotonic()
        e = dict(os.environ, **env) if env else None
        p = subprocess.Popen(["taskset", "-c", str(core)] + [str(x) for x in cmd], stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, text=True, env=e)
        with self.lock:
            self.untimed_procs.add(p)
        try:
            out, err = p.communicate(timeout=max(t, 0.001))
        except subprocess.TimeoutExpired:
            p.kill()
            p.communicate()
            if limit is not None and limit < rem:
                return None, time.monotonic() - start
            raise HardStop("untimed work")
        finally:
            with self.lock:
                self.untimed_procs.discard(p)
        return subprocess.CompletedProcess(p.args, p.returncode, out, err), time.monotonic() - start

    def pool(self, tasks):
        """Untimed work two at a time, one per core (rule-m3 item 3; Run plan steps 4-5). tasks: [(key, fn(core))].
        The pool is shut down and joined on return."""
        cores = queue.Queue()
        for c in (C.HARNESS_CORE, C.TIMED_CORE):
            cores.put(c)

        def run(fn):
            core = cores.get()
            try:
                return fn(core)
            finally:
                cores.put(core)
        res, stop = {}, None
        with cf.ThreadPoolExecutor(max_workers=2) as ex:
            futs = {ex.submit(run, fn): key for key, fn in tasks}
            for f in cf.as_completed(futs):
                try:
                    res[futs[f]] = f.result()
                except HardStop as e:
                    stop = e
        if stop:
            raise stop
        return res

    # --------------------------------------------------------------------------------------- step 0
    def step0(self):
        """Before the clock (rule-m3, Run plan step 0): the workloads, the fractions references (the result's and, in
        sop and pos, each intermediate's) and the model's predictions, untimed and outside the hard stop, and each
        file's sha256."""
        self.log("step 0 (before the clock): workloads, exact references and the model's predictions")
        t = time.monotonic()
        cmd = [sys.executable, str(C.HERE / "m3_gen.py"), str(self.wl), "--op", self.op,
               "--jobs", ",".join(map(str, self.jobs))] + (["--toy"] if self.toy else [])
        p = subprocess.run(cmd, capture_output=True, text=True)
        if p.returncode:
            raise RunStop(f"step 0: workload generation failed: {p.stdout[-2000:]}{p.stderr[-2000:]}")
        gen = json.loads((self.wl / "gen.json").read_text())
        self.state["gen"] = gen
        self.state["step0_s"] = round(time.monotonic() - t, 1)
        self.log(f"step 0 done in {self.state['step0_s']} s: " + ", ".join(m["name"] for m in gen.values()))
        self.save()

    def step0_vs_earlier(self):
        """rule-m3, Workloads: step 0's files (each job's workload and references) MUST equal m1's (add, mul) or m2's
        (sop, pos) by sha256, job for job, as those runs recorded them in their states. None, or the files that differ.
        Toy runs: not compared (m1 and m2 recorded no toy files; DECISIONS-m3 M3-H3)."""
        if self.toy:
            return None
        folder = C.M1_RESULTS[self.op] if self.op in ("add", "mul") else C.M2_RESULTS[self.op]
        try:
            earlier = json.loads((folder / "state.json").read_text())["gen"]
        except (OSError, ValueError, KeyError):
            return [f"{folder.name}'s state is missing"]
        bad = []
        for j, m in self.state["gen"].items():
            for name in m["sha256"]:
                if name.endswith(".m3.json"):
                    continue                                  # the predictions are this round's own (M3-H3)
                if earlier.get(j, {}).get("sha256", {}).get(name) != m["sha256"][name]:
                    bad.append(name)
        return bad or None

    # --------------------------------------------------------------------------------------- step 1
    def build(self):
        self.bin.mkdir(parents=True, exist_ok=True)
        for out, cmd in L.build_commands(self.bin):
            out.parent.mkdir(parents=True, exist_ok=True)
            p, _ = self.run_untimed(cmd, C.HARNESS_CORE)
            if p.returncode:
                raise RunStop(f"build failed: {' '.join(cmd)}\n{p.stderr}")

    def step1(self):
        self.log("step 1: build, hashes, tcm.c's diff, m1's and m2's sources, step 0's files")
        self.build()
        for check, what in ((L.vendor08_check, "the 08 module"), (L.tc3_check, "tc3.c or its headers"),
                            (L.tcm_diff_check, "tcm.c's diff"), (L.m1_sources_check, "m1's sources"),
                            (L.m2_sources_check, "m2's sources")):
            bad = check()
            if bad:
                raise RunStop(f"{what} changed: {bad}")
        (self.out / "tcm.diff").write_text(L.tcm_diff_text())    # the recorded diff (rule-m3 item 2)
        bad = []
        for j, m in self.state["gen"].items():
            for name, want in m["sha256"].items():
                if C.sha256(self.wl / name) != want:
                    bad.append(name)
        if bad:
            raise RunStop(f"step 0's files changed since they were written: {bad}")
        bad = self.step0_vs_earlier()
        earlier = "m1's" if self.op in ("add", "mul") else "m2's"
        if bad:
            raise RunStop(f"step 0's files differ from {earlier} (rule-m3, Workloads): {bad}")
        self.state["step0_vs_earlier"] = (f"equal to {earlier} by sha256" if not self.toy else "not compared (toy run)")
        self.state["binaries"] = self.code
        self.log(f"08 module, tc3.c, tcm.c's diff and m1's and m2's sources as pinned; step 0's files unchanged "
                 f"({self.state['step0_vs_earlier']})")
        self.save()

    # --------------------------------------------------------------------------------------- step 2
    def unit_tests(self):
        self.log("step 2: unit tests (with the large trial divisions" + (", skipped in a toy run" if self.toy else "") +
                 "), the differential test, the allocation check, and the tests under the sanitizers")
        res = {}
        for name, cmd, env in L.unit_commands(self.bin, quick=self.toy):
            p, wall = self.run_untimed(cmd, C.HARNESS_CORE, env=env)
            res[name] = {"rc": p.returncode, "out": (p.stdout + p.stderr)[-1500:], "wall": round(wall, 2)}
        self.state["unit_tests"] = res
        failed = [k for k, v in res.items() if v["rc"]]
        self.save()
        if failed:
            raise RunStop(f"unit tests failed: {failed}")
        self.log("unit tests pass: " + "; ".join(v["out"].strip().splitlines()[-1] for v in res.values()))

    # --------------------------------------------------------------------------------------- idle machine
    def wait_idle(self, where):
        """rule-m3 item 6 (carried over): before each timed block; re-check every 15 s, refuse after 10 minutes."""
        t = time.monotonic()
        checks = self.state.setdefault("idle_checks", [])
        while True:
            self.check_clock(where)
            ok, d = self.idle_probe()
            rec = dict(d, where=where, time=stamp(), idle=ok, waited_s=round(time.monotonic() - t, 1))
            checks.append(rec)
            if ok:
                self.log(f"machine idle before {where} (core 1 {d['core1']:.1%}, load {d['load1']})")
                return rec
            what = f"core 1 {d['core1']:.1%}" + ("; busy: " + ", ".join(f"{b['name']}[{b['pid']}] {b['cpu']:.0%}"
                                                                         for b in d["busy"]) if d["busy"] else "")
            if self.toy:
                self.log(f"machine busy before {where} ({what}); toy run: not waiting")
                return rec
            if time.monotonic() - t >= C.IDLE_WAIT_S:
                self.log(f"machine still busy before {where} after {C.IDLE_WAIT_S:.0f} s ({what}): refusing to time")
                raise RunStop(f"the machine is not idle before {where} ({what})")
            self.log(f"machine busy before {where} ({what}); re-checking in {C.IDLE_POLL_S:.0f} s")
            self.save()
            time.sleep(C.IDLE_POLL_S)

    # --------------------------------------------------------------------------------------- timed invocations
    def invoke(self, job, row, kind, args, block):
        """One timed invocation on core 1 (inside a timed block: nothing is written until after the block)."""
        self.check_clock(f"job {job}, {row}, {kind}")
        self.seq += 1
        out = self.tmp / f"i{self.seq:05d}.txt"
        ro = self.tmp / f"i{self.seq:05d}.out"
        impl = C.ROW_IMPL[row]
        t = time.monotonic()
        rc = self.spawner.run([self.exe(impl, "timing"), kind, self.wlpath(job), ro] + args, out, C.TIMED_CORE)
        return {"job": job, "row": row, "kind": kind, "args": [str(x) for x in args], "rc": rc, "out": str(out),
                "readout": str(ro), "wall": round(time.monotonic() - t, 4), "block": block,
                "launch_s": self.spawner.last_launch_s}

    def quick_parse(self, rec):
        """After waitpid, inside a block: only what the block needs next (r from a calibration; an over-cap rep)."""
        lines = Path(rec["out"]).read_text().splitlines() if rec["rc"] == 0 else []
        rec["overcap"] = any(l.startswith("overcap") for l in lines)
        if rec["kind"] == "calib" and rec["rc"] == 0:
            reps = K.parse_reps(lines, ("calib",))
            if reps:
                last = reps[-1]
                rec["r"], rec["rep_ns"] = last[0], last[1]
                rec["reps"] = 1 if last[1] >= C.ONE_REP_NS else 3
        return rec

    def finish(self, rec):
        """After the block: the full parse, the exact check of the read-out (rule-m3 item 12), the files removed. Each
        timed quantity (each input's build, the operation, the read-out, and the total) keeps its own minimum of the
        kept reps, over r (rule-m3 item 4)."""
        lines = Path(rec["out"]).read_text().splitlines() if Path(rec["out"]).exists() else []
        err = next((l for l in lines if l.startswith(("error ", "auditfail "))), None)
        if rec["rc"] or err:
            rec["bug"] = {"what": err or f"exit {rec['rc']}: {' '.join(lines)[-300:]}"}
        else:
            bad = K.readout_check(rec["readout"], self.ref(rec["job"]))
            if bad:
                rec["bug"] = bad
        rec["reps_cpu"] = K.parse_rep_cpu(lines, (rec["kind"] if rec["kind"] == "calib" else "rep",))
        if "bug" not in rec:
            if rec["kind"] == "time" and not rec.get("overcap"):
                r, reps = int(rec["args"][0]), int(rec["args"][1])
                out = K.parse_reps(lines, ("rep",))
                total, op, read, builds = K.kept_minimum(out, r)
                rec.update(r=r, reps=reps, total=total, operation=op, read=read, builds=list(builds),
                           reps_out=[list(x[:4]) + [list(x[4])] for x in out])
                kept = out if reps == 1 else out[1:]
                best = min(kept, key=lambda x: x[1])      # the process's CPU time: of the rep of minimum total
                cpu = rec["reps_cpu"]
                if len(cpu) == len(out) and cpu[out.index(best)] is not None:
                    c, s = cpu[out.index(best)]
                    rec.update(cpu_ns=c / r, span_ns=s / r, cpu_share=c / s if s else None)
            elif rec.get("overcap"):
                last = K.parse_reps(lines)[-1]
                rec["cap_rep_ns"] = last[1]
        for f in (rec["readout"], rec["out"]):
            try:
                os.unlink(f)
            except OSError:
                pass
        rec.pop("readout", None)
        rec.pop("out", None)
        return rec

    # --------------------------------------------------------------------------------------- a timed block
    def timed_block(self, job, label, pilot=False, rows=None):
        """rule-m3 items 4-6: the idle check, then one timed block (calibration and the rounds), then the disturbance
        check. A disturbed block is discarded and re-run once; a second disturbance stops the run."""
        rows = rows or list(C.ROWS)
        for attempt in (1, 2):
            where = f"{label} block of job {job}" + (" (re-run)" if attempt == 2 else "")
            idle = self.wait_idle(where)
            self.save()
            assert threading.active_count() == 1, "the runner must be single-threaded during timed phases"
            self.tmp.mkdir(parents=True, exist_ok=True)
            spawns0 = self.spawner.spawns
            s0 = snapshot()
            recs = self._block_body(job, label, rows)       # nothing is written in here
            s1 = snapshot()
            d = disturbance(s0, s1, os.getpid(), self.spawner.spawns - spawns0, set(self.hosts),
                            None if pilot else self.allowance_for(s1["t"] - s0["t"]))
            d.update(job=job, kind="rounds", label=label, attempt=attempt, time=stamp(), idle_check=idle["time"])
            for rec in recs:
                self.finish(rec)
            self.state.setdefault("disturbance_checks", []).append(d)
            runner = (f"runner {d['runner_ticks']} ticks over {d['spawns']} launches" +
                      (f", {d['runner_ticks_per_launch']:.3f} per launch" if d["runner_ticks_per_launch"] is not None
                       else ""))
            if d["runner_over_limit"]:
                self.state.setdefault("harness_bugs", []).append(
                    {"what": f"the runner used {d['runner_ticks']} ticks over {d['spawns']} launches in a timed block "
                             f"(more than {C.RUNNER_TICKS_PER_LAUNCH_MAX:g} per launch plus "
                             f"{C.RUNNER_TICKS_BLOCK_ALLOWANCE})", "job": job, "label": label, "time": stamp()})
                self.save()
                raise RunStop(f"harness bug: {runner} in the {label} block of job {job} (rule-m3 item 5: more than "
                              f"{C.RUNNER_TICKS_PER_LAUNCH_MAX:g} tick per launch plus "
                              f"{C.RUNNER_TICKS_BLOCK_ALLOWANCE} is a harness bug)")
            if not d["disturbed"]:
                self.log(f"job {job} {label}: block of {d['seconds']:.1f} s undisturbed ({runner}, forks "
                         f"{d['forks_rise']})")
                self.save()
                return recs
            self.log(f"job {job} {label}: block DISTURBED ({'; '.join(d['reasons'])}); discarded")
            self.state.setdefault("discarded_blocks", []).append({"job": job, "label": label, "records": recs})
            self.save()
        raise RunStop(f"job {job}: the {label} block was disturbed twice (rule-m3 item 5)")

    def _block_body(self, job, label, rows):
        recs, cal = [], {}
        for impl in C.IMPLS:                                    # calibration: the four builds; the null row takes
            if impl not in {C.ROW_IMPL[r] for r in rows}:       # Hash-V-pack's r
                continue
            rec = self.quick_parse(self.invoke(job, C.IMPL_ROW[impl], "calib", [], label))
            rec["role"] = "calibration"
            recs.append(rec)
            cal[impl] = rec
        capped = {r for r in rows if cal.get(C.ROW_IMPL[r], {}).get("overcap") or "r" not in cal.get(C.ROW_IMPL[r], {})}
        nr = C.PILOT_ROUNDS if label == "pilot" else C.ROUNDS
        order = [r for r in C.ROWS if r in rows]
        for k in range(nr):
            rot = order[k % len(order):] + order[:k % len(order)]   # rotated by one row per round
            for pos, row in enumerate(rot):
                if row in capped:
                    continue
                c = cal[C.ROW_IMPL[row]]
                rec = self.quick_parse(self.invoke(job, row, "time", [c["r"], c["reps"]], label))
                rec.update(role="round", round=k + 1, pos=pos)
                recs.append(rec)
                if rec["overcap"]:
                    capped.add(row)
        return recs

    # --------------------------------------------------------------------------------------- hosts
    def find_hosts(self):
        self.hosts = {pid: name for pid, name in ancestors()}
        self.state["hosts"] = {str(p): n for p, n in self.hosts.items()}

    def allowance_for(self, seconds):
        rates = self.state.get("host_rates", {})
        return {int(p): K.host_allowance(r, seconds) for p, r in rates.items()}

    # --------------------------------------------------------------------------------------- bugs (item 12)
    def bug(self, impl, job, where, detail):
        entry = {"impl": impl, "job": job, "where": where, "detail": detail, "time": stamp()}
        if not self.state.get("measuring"):
            self.state.setdefault("pre_gate_failures", []).append(entry)
            self.save()
            raise RunStop(f"check failure before the gates ({where}): {entry}")
        self.state.setdefault("bugs", []).append(entry)
        if impl not in self.state.setdefault("stopped_impls", []):
            self.state["stopped_impls"].append(impl)
        self.log(f"BUG: {entry}; the measured run of {impl} stops (rule-m3 item 12)")
        self.save()

    def live_rows(self):
        stopped = set(self.state.get("stopped_impls", []))
        only = self.state.get("rerun_only")
        return [r for r in C.ROWS if C.ROW_IMPL[r] not in stopped and (not only or C.ROW_IMPL[r] in only)]

    # --------------------------------------------------------------------------------------- step 3: the pilot
    def step3_pilot(self):
        self.log("step 3: pilot")
        jobs = [j for j in C.PILOT_JOBS if j in self.jobs] or self.jobs[:1]
        pcal = self.state.setdefault("pilot_calibration", {})
        walls = {}
        for job in jobs:
            t = time.monotonic()
            recs = self.timed_block(job, "pilot", pilot=True)     # its idle and disturbance checks included
            walls[job] = time.monotonic() - t
            for rec in recs:
                if "bug" in rec:
                    self.bug(C.ROW_IMPL[rec["row"]], job, "pilot", rec["bug"])
                if rec.get("role") == "calibration":
                    pcal[f"{job}:{rec['row']}"] = {k: rec.get(k) for k in ("r", "reps", "rep_ns", "overcap",
                                                                           "reps_cpu")}
                    if rec["row"] == C.NULL_OF:
                        pcal[f"{job}:{C.NULL_ROW}"] = dict(pcal[f"{job}:{C.NULL_OF}"], shared_from=C.NULL_OF)
            self.state.setdefault("pilot_records", []).extend(recs)
        measured, rates = K.host_rates([d for d in self.state.get("disturbance_checks", []) if d.get("label") == "pilot"])
        self.state["host_rates"] = rates
        self.state["host_rates_measured"] = measured
        # the projection (rule-m3, Run plan step 3): per size class c (its pilot job), O_c the block's checks (its wall
        # less its launches), K_row,c the row's calibration launch, P_row,c its two round launches (0 over cap), t_row,c
        # its time per run from its calibration; N_c the run's jobs of size c. In a toy run without a size's pilot job,
        # that size takes the largest pilot block's figures.
        blocks = {}
        for job in jobs:
            recs = [r for r in self.state["pilot_records"] if r["job"] == job and r.get("block") == "pilot"]
            K_, P_, t_ = {}, {}, {}
            for r in recs:
                if r.get("role") == "calibration":
                    K_[r["row"]] = K_.get(r["row"], 0.0) + r["wall"]
                    if r.get("r") and r.get("rep_ns"):
                        t_[r["row"]] = r["rep_ns"] / r["r"] / 1e9
                elif r.get("role") == "round":
                    P_[r["row"]] = P_.get(r["row"], 0.0) + r["wall"]
            launches = sum(r["wall"] for r in recs)
            blocks[C.job_size_index(job)] = {"job": job, "O": max(walls[job] - launches, 0.0), "K": K_, "P": P_,
                                             "t": t_, "wall": walls[job]}
        largest = blocks[max(blocks)]
        n_of = {}
        for j in self.jobs:
            n_of[C.job_size_index(j)] = n_of.get(C.job_size_index(j), 0) + 1
        use = {c: blocks.get(c, largest) for c in n_of}
        elapsed = self.elapsed()
        proj = K.pilot_projection(elapsed, use, n_of)
        self.state["pilot"] = {"jobs": jobs, "walls_s": {str(j): w for j, w in walls.items()},
                               "blocks": {str(c): b for c, b in blocks.items()}, "N": {str(c): v for c, v in n_of.items()},
                               "elapsed_s": elapsed, "projected_s": proj, "limit_s": C.PILOT_LIMIT_S,
                               "host_rates": rates, "host_rates_measured": measured,
                               "discarded": "the pilot's results are discarded (rule-m3, Run plan step 3)"}
        self.save()
        self.log("pilot: blocks " + ", ".join(f"job {b['job']} {b['wall']:.1f} s (checks {b['O']:.1f} s)"
                                             for b in blocks.values()) +
                 f"; projection {proj / 60:.1f} min (limit {C.PILOT_LIMIT_S / 60:g}); host rates {rates}")
        reasons = []
        if proj > C.PILOT_LIMIT_S:
            reasons.append(f"projected total {proj / 60:.1f} min exceeds {C.PILOT_LIMIT_S / 60:g} minutes")
        busy = {p: r for p, r in measured.items() if r > C.HOST_RATE_CEILING}
        if busy:
            reasons.append(f"host processes over the ceiling of {C.HOST_RATE_CEILING:g} ticks per second: {busy}")
        if reasons:
            self.state["pilot_stopped"] = reasons
            raise RunStop("the pilot stops the run, reported to the author: " + "; ".join(reasons))

    # --------------------------------------------------------------------------------------- step 4: gates
    def gate(self, impl, job, core):
        """spec v0.3 §16.9 items 1, 2 and 5 on one row and job (test build): the read-out and every intermediate's
        against fractions, the audits (every obtained gcd checked against Euclid inside the factored rows), every exact
        figure against the model, every step; for TC also the TC gate: tc3.c itself on the same operations, every step,
        its per-operation record equal to tcm.c's (rule-m3 item 3)."""
        ro = self.tmp / f"g-{job:02d}-{impl}.out"
        pc = self.tmp / f"gp-{job:02d}-{impl}.bin"
        dp = self.tmp / f"gd-{job:02d}-{impl}.bin"
        inter = [Path(f"{ro}.{x}") for x in C.SET_INTERMEDIATES[self.op]]
        p, wall = self.run_untimed([self.exe(impl, "test"), "gate", self.wlpath(job), ro, pc, dp], core)
        res = {"impl": impl, "job": job, "wall": round(wall, 2), "rc": p.returncode, "code": self.code[impl]}
        lines = p.stdout.splitlines()
        err = next((l for l in lines if l.startswith(("error ", "auditfail "))), None)
        fails = []
        if p.returncode or err:
            res["fail"] = err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"
        else:
            bad = K.readout_check(ro, self.ref(job))
            if bad:
                fails.append(f"read-out: {bad}")
            for x, f in zip(C.SET_INTERMEDIATES[self.op], inter):
                bad = K.readout_check(f, self.ref(job, x))
                if bad:
                    fails.append(f"intermediate {x}: {bad}")
            dump = C.read_dump(dp) if dp.exists() else None
            fails += K.exact_checks(impl, self.op, lines, self.pred(job), dump, test=True)
            res["percall_sha256"] = C.sha256(pc)
            if dump:
                res["dump_sha256"] = {k: C.stamp_sha(v) for k, v in dump.items()}
            if impl == "TC":                                     # the TC gate
                ro3 = self.tmp / f"g3-{job:02d}.out"
                pc3 = self.tmp / f"gp3-{job:02d}.bin"
                q, w3 = self.run_untimed([self.bin / "tc3gate_m2", self.wlpath(job), ro3, pc3], core)
                res["tc3gate"] = {"rc": q.returncode, "wall": round(w3, 2), "out": q.stdout.strip()[-200:]}
                if q.returncode:
                    fails.append(f"TC gate: tc3gate_m2 exit {q.returncode}: {q.stdout[-200:]} {q.stderr[-200:]}")
                else:
                    b3 = K.readout_check(ro3, self.ref(job))
                    if b3:
                        fails.append(f"TC gate: tc3.c's read-out {b3}")
                    same = C.sha256(pc3) == res["percall_sha256"]
                    res["tc3gate"]["percall"] = "equal" if same else "DIFFERENT"
                    if not same:
                        fails.append("TC gate: tcm.c's cases and gcd counts differ from tc3.c's, operation for "
                                     "operation")
                for f in (ro3, pc3):
                    if f.exists():
                        f.unlink()
            if fails:
                res["fail"] = "exact checks (spec v0.3 §16.9): " + "; ".join(fails[:10])
        for f in [ro, pc, dp] + inter:
            if f.exists():
                f.unlink()
        return res

    def step4_gates(self, impls=None):
        self.log("step 4: gates (two at a time)")
        gates = self.state.setdefault("gates", {})
        pairs = [(i, j) for j in self.jobs for i in (impls or C.IMPLS)
                 if not (gates.get(f"{i}:{j}", {}).get("code") == self.code[i] and "fail" not in gates[f"{i}:{j}"])]
        res = self.pool([((i, j), (lambda core, i=i, j=j: self.gate(i, j, core))) for i, j in pairs])
        for (i, j), r in res.items():
            gates[f"{i}:{j}"] = r
        # the factored rows against Hash-V-pack (spec v0.3 §16.9 items 2, 4): (h) and (l), and the handles, equal
        for j in self.jobs:
            for i in C.FAC_IMPLS:
                a, b = gates.get(f"{i}:{j}", {}), gates.get(f"MHVP:{j}", {})
                if "dump_sha256" in a and "dump_sha256" in b:
                    for x in C.SET_MATRICES[self.op]:
                        for sec in (f"hb{x}", f"tab{x}", f"cel{x}"):
                            if a["dump_sha256"].get(sec) != b["dump_sha256"].get(sec):
                                gates[f"{i}:{j}"]["fail"] = f"against Hash-V-pack: {x}'s {sec[:-len(x)]} differs"
        self.save()
        failed = {k: v["fail"] for k, v in gates.items() if "fail" in v}
        if failed:
            raise RunStop(f"step 4: gates failed (fix the row, then --resume): {failed}")
        self.state["measuring"] = True
        self.log(f"every gate passed ({len(gates)}), the TC gate included")
        self.save()

    # --------------------------------------------------------------------------------------- step 5
    def count(self, impl, job, core, rep):
        ro = self.tmp / f"c-{job:02d}-{impl}-{rep}.out"
        pc = self.tmp / f"p-{job:02d}-{impl}-{rep}.bin"
        p, wall = self.run_untimed([self.exe(impl, "counting"), "count", self.wlpath(job), ro, pc], core,
                                   limit=C.COUNT_LIMIT_S)
        res = {"impl": impl, "job": job, "rep": rep, "wall": round(wall, 2), "code": self.code[impl]}
        if p is None:
            res["na"] = f"did not finish within the {C.COUNT_LIMIT_S // 60}-minute limit"
            return res
        lines = p.stdout.splitlines()
        err = next((l for l in lines if l.startswith("error ")), None)
        if p.returncode or err:
            res["bug"] = {"what": err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"}
            return res
        bad = K.readout_check(ro, self.ref(job))
        if bad:
            res["bug"] = bad
            return res
        fails = K.exact_checks(impl, self.op, lines, self.pred(job), test=False)
        if fails:
            res["bug"] = {"what": "exact checks: " + "; ".join(fails[:5])}
        res["counts"] = [l for l in lines if l.startswith(("ctr ", "xctr "))]
        res["stor"] = K.parse_stor(lines)
        res["opc"] = K.parse_opc(lines)
        res["packs"] = K.parse_packs(lines)
        res["fac"] = K.parse_fac(lines)
        res["ffig"] = K.parse_ffig(lines)
        res["facc"] = K.parse_facc(lines)
        res["percall_sha256"] = C.sha256(pc)
        res["percall_core_sha256"] = K.percall_core_sha(pc)
        res["G"] = K.gcd_work(lines)
        res["G_by_phase"] = K.gcd_work_by_phase(lines)
        for f in (ro, pc):
            f.unlink()
        return res

    def mem(self, impl, job, core, rep):
        """the memory run (memory build, untimed): each input's stable bytes and build peak, the operation's peak, the
        figures after creation and at the end (spec v0.3 §16.10, v0.2 §15.6)"""
        ro = self.tmp / f"m-{job:02d}-{impl}-{rep}.out"
        p, wall = self.run_untimed([self.exe(impl, "memory"), "mem", self.wlpath(job), ro], core, limit=C.COUNT_LIMIT_S)
        res = {"impl": impl, "job": job, "rep": rep, "wall": round(wall, 2), "code": self.code[impl]}
        if p is None:
            res["na"] = f"did not finish within the {C.COUNT_LIMIT_S // 60}-minute limit"
            return res
        err = next((l for l in p.stdout.splitlines() if l.startswith("error ")), None)
        if p.returncode or err:
            res["bug"] = {"what": err or f"exit {p.returncode}: {p.stdout[-300:]} {p.stderr.strip()[:300]}"}
            return res
        bad = K.readout_check(ro, self.ref(job))
        if bad:
            res["bug"] = bad
            return res
        res["memrun"] = K.parse_memrun(p.stdout.splitlines())
        ro.unlink()
        return res

    def step5_counts(self, impls=None):
        self.log("step 5: counting runs and memory runs (two at a time)")
        tasks = []
        for job in self.jobs:
            for impl in (impls or C.IMPLS):
                for rep in ((0, 1) if job in C.COUNT_REPEAT_JOBS else (0,)):
                    tasks.append((("count", impl, job, rep), (lambda core, i=impl, j=job, r=rep: self.count(i, j, core, r))))
                    tasks.append((("mem", impl, job, rep), (lambda core, i=impl, j=job, r=rep: self.mem(i, j, core, r))))
        res = self.pool(tasks)                                 # shut down and joined on return
        counts, mems = self.state.setdefault("counts", {}), self.state.setdefault("mems", {})
        for (kind, impl, job, rep), r in sorted(res.items(), key=lambda x: x[0][3]):
            dst = counts if kind == "count" else mems
            key = f"{impl}:{job}"
            if rep == 0:
                dst[key] = r
            else:
                first = res[(kind, impl, job, 0)]
                keys = ("counts", "stor", "opc", "packs", "fac", "ffig", "facc", "percall_sha256") if kind == "count" \
                    else \
                    ("memrun",)
                same = all(r.get(k) == first.get(k) for k in keys)
                if "bug" not in r and "na" not in r and not same:
                    r["bug"] = {"what": f"{kind} run repeated: figures differ (rule-m3 item 2)"}
                dst[key]["repeat"] = "identical" if "bug" not in r else r["bug"]["what"]
            if "bug" in r:
                self.bug(impl, job, f"{kind} run" + (" (repeat)" if rep else ""), r["bug"])
        self.call_checks(counts)
        self.save()

    def call_checks(self, counts):
        """spec v0.3 §16.9 items 3-4 on the counting runs. Hash-V-pack: its per-operation record (case and core counts,
        every step) equal to TC's, and G(row) / G(TC) = 1 (carried over). The factored rows, the adapted check (§16.9
        item 3): per operation the same case and the same exact divisions, multiplications and additions as TC (the
        per-operation records with their gcds cleared, equal); their gcd counts and work are reported, G(row) / G(TC)
        per part beside their factor work, not compared; each factored row against Hash-V-pack (§16.9 item 4): the
        same arithmetic counters of every matrix but the operations' gcds, the same W and found/new per store."""
        checks = self.state.setdefault("call_checks", {})
        for job in self.jobs:
            c = {i: counts.get(f"{i}:{job}", {}) for i in C.IMPLS}
            if any("percall_sha256" not in v for v in c.values()):
                continue
            t = c["TC"]
            rec, bad = {"G_TC": t["G"], "G_TC_by_phase": t.get("G_by_phase")}, []
            same = c["MHVP"]["percall_sha256"] == t["percall_sha256"]
            g_ok = c["MHVP"]["G"] == t["G"]
            rec["MHVP"] = {"call_for_call": "pass" if same else "FAIL", "G": c["MHVP"]["G"],
                           "G_ratio": c["MHVP"]["G"] / t["G"] if t["G"] else None,
                           "gcd_work_check": "passed" if g_ok else "FAILED"}
            if not same or not g_ok:
                bad.append("Hash-V-pack against TC")
            for i in C.FAC_IMPLS:
                same = c[i]["percall_core_sha256"] == t["percall_core_sha256"]
                gp = c[i].get("G_by_phase") or {}
                tp = t.get("G_by_phase") or {}
                rec[i] = {"call_for_call": "pass (case and core arithmetic)" if same else "FAIL", "G": c[i]["G"],
                          "G_ratio": c[i]["G"] / t["G"] if t["G"] else None,
                          "G_ratio_by_part": {ph: (gp.get(ph, 0) / tp[ph] if tp.get(ph) else None)
                                              for ph in ("entry", "add", "mul")},
                          "gcd_work_check": "reported, not checked (rule-m3 item 7)"}
                if not same:
                    bad.append(f"{C.IMPL_ROW[i]} against TC")

            def split(i):
                return K.op_counts_no_gcd(c[i]["counts"]), {f"{w}:{r}": {k: c[i]["stor"][w][r][k]
                                                                         for k in ("W", "found", "new")}
                                                            for w in c[i]["stor"] for r in c[i]["stor"][w]}
            for i in C.FAC_IMPLS:
                what = f"{C.IMPL_ROW[i]} against Hash-V-pack"
                ok = split(i) == split("MHVP")
                rec[what] = "pass" if ok else "FAIL"
                if not ok:
                    bad.append(what)
            rec["result"] = "pass" if not bad else "FAIL: " + ", ".join(bad)
            checks[str(job)] = rec
            if bad:
                for impl in C.IMPLS:
                    self.bug(impl, job, "counting run, call for call",
                             {"what": f"{', '.join(bad)}: per-operation case or core arithmetic, or stores, differ "
                                      "(spec v0.3 §16.9 items 3-4)"})

    # --------------------------------------------------------------------------------------- step 6
    def step6_rounds(self):
        self.log("step 6: calibration and 12 rounds, one timed block per job")
        cal = self.state.setdefault("calibration", {})
        store = self.state.setdefault("records", [])
        for job in self.jobs:
            rows = self.live_rows()
            if not rows:
                break
            recs = self.timed_block(job, "measured", rows=rows)
            for rec in recs:
                if rec.get("role") == "calibration":
                    cal[f"{job}:{rec['row']}"] = {k: rec.get(k) for k in ("r", "reps", "rep_ns", "overcap", "bug",
                                                                          "reps_cpu")}
                    if rec["row"] == C.NULL_OF:
                        cal[f"{job}:{C.NULL_ROW}"] = dict(cal[f"{job}:{C.NULL_OF}"], shared_from=C.NULL_OF)
            for rec in recs:
                if "bug" in rec:
                    self.bug(C.ROW_IMPL[rec["row"]], job, f"{rec['role']} of job {job}", rec["bug"])
            stopped = set(self.state.get("stopped_impls", []))
            store.extend(r for r in recs if C.ROW_IMPL[r["row"]] not in stopped)
            self.save()

    # --------------------------------------------------------------------------------------- results file
    def write_header(self):
        rule = C.RULE_FILE.read_text()
        what = {"add": "matrix addition", "mul": "matrix multiplication", "sop": "the sum of products A B + C D",
                "pos": "the product of sums (A + B)(C + D)"}[self.op]
        head = [f"# Design 3 on matrices, round m3 (factored whole numbers), {what}: results", "",
                f"Run started {stamp()}." + (" **TOY RUN: a harness test, not a measurement.**" if self.toy else ""),
                "", "## The measurement rule (research/rule-m3.md, verbatim, written before the first trial)", "",
                f"sha256 `{C.sha256(C.RULE_FILE)}`", "", "<!-- rule begins -->", rule.rstrip("\n"), "<!-- rule ends -->",
                ""]
        (self.out / "results.md").write_text("\n".join(head) + "\n")

    # --------------------------------------------------------------------------------------- top level
    def preflight(self):
        bad = C.rule_check()
        if bad:
            if not self.toy:
                raise SystemExit(f"refusing a measured run: {bad}")
            print("WARNING (toy run): " + bad)
        for check, what in ((L.vendor08_check, "the 08 module changed (rule-m3 item 2)"),
                            (L.tc3_check, "tc3.c or its headers changed (rule-m3 item 2)"),
                            (L.tcm_diff_check, "tcm.c's diff from tc3.c is not the recorded one (rule-m3 item 2)"),
                            (L.m1_sources_check, "m1's sources changed (spec v0.3 §16.11)"),
                            (L.m2_sources_check, "m2's sources changed (spec v0.3 §16.11)")):
            bad = check()
            if bad:
                raise SystemExit(f"{what}: {bad}")
        bad = C.round3_check()
        if bad:
            raise SystemExit(f"round 3's results changed: {bad}")
        for p in (self.out, self.wl, self.bin, self.tmp):
            bad = C.forbidden(p, self.op)
            if bad:
                raise SystemExit(f"refusing to write there: {bad}")
        if not self.toy:
            if len(os.sched_getaffinity(0)) < 2 or (os.cpu_count() or 0) < 2:
                raise SystemExit("the rule needs 2 cores (rule-m3 item 4)")
            if self.a.jobs:
                raise SystemExit("a measured run runs every job of its set")
        self.protected_before = {k: C.folder_hashes(f) for k, f in C.protected(self.op).items()}
        os.sched_setaffinity(0, {C.HARNESS_CORE})

    def start(self):
        self.out.mkdir(parents=True, exist_ok=True)
        self.tmp.mkdir(parents=True, exist_ok=True)
        if self.a.resume or self.a.rerun:
            self.state = json.loads(self.state_file.read_text())
            for k, why in (("hard_stopped", "stopped at the hard stop"), ("pilot_stopped", "stopped by the pilot")):
                if self.state.get(k):
                    raise SystemExit(f"this run was {why}: nothing more runs until the author decides")
            if self.state.get("op") != self.op:
                raise SystemExit(f"the state in {self.out} is of the {self.state.get('op')} run")
            self.state["elapsed_before"] = self.state.get("elapsed_saved", 0)
        else:
            for f in ("run.log", "state.json"):
                if (self.out / f).exists():
                    (self.out / f).unlink()
            self.state = {"started": stamp(), "op": self.op, "elapsed_before": 0.0, "toy": self.toy, "jobs": self.jobs,
                          "rule_sha256": C.sha256(C.RULE_FILE), "rule_text": C.RULE_FILE.read_text(),
                          "vendor08_sha256": L.VENDOR08_SHA256, "round3_sha256": L.ROUND3_SHA256,
                          "m1_sources_sha256": L.M1_SOURCES_SHA256, "m2_sources_sha256": L.M2_SOURCES_SHA256,
                          "operator": {"timeout_min_at_least": C.OPERATOR_TIMEOUT_MIN,
                                       "instruction": "start once; run nothing else until the exit notice "
                                                      "(rule-m3 item 5)"}}
            self.write_header()
            self.step0()                                     # before the clock, outside the hard stop
        if self.protected_before is not None:
            self.state.setdefault("protected_before", []).append(self.protected_before)
        self.state.setdefault("code_sessions", []).append({"time": stamp(), "code": self.code})
        self.t0 = time.monotonic()                          # the run's clock, from step 1, summed over sessions
        self.find_hosts()
        step = "start"
        try:
            self.arm()
            if self.a.rerun:
                step = "re-run"
                self.do_rerun(self.a.rerun)
            elif self.a.resume:
                if self.state.get("records"):
                    raise SystemExit("--resume is refused once a timed round of step 6 has run; see --rerun")
                self.state.pop("run_stopped", None)
                step = "step 1"
                self.step1()
                step = "step 2"
                self.unit_tests()
                if "pilot" not in self.state:
                    step = "step 3"
                    self.state["pilot_records"] = []
                    self.step3_pilot()
                step = "steps 4-6"
                self.rest()
            else:
                step = "step 1"
                self.step1()
                step = "step 2"
                self.unit_tests()
                step = "step 3"
                self.step3_pilot()
                if self.a.pilot_only:
                    self.log("pilot only: stopping after step 3")
                    return
                step = "steps 4-6"
                self.rest()
            self.state["finished"] = stamp()
        except HardStop as e:
            self.reap()
            self.state["hard_stopped"] = {"step": step, "where": str(e), "elapsed_s": self.elapsed()}
            self.log(f"STOPPED AT {self.hard_stop / 60:g} MINUTES in {step} ({e}): reporting what was done, with no "
                     "verdicts")
        except RunStop as e:
            self.state["run_stopped"] = {"step": step, "reason": str(e), "elapsed_s": self.elapsed()}
            self.log(f"RUN STOPPED in {step}: {e}")
        finally:
            self.disarm()
            self.save()
            bad = C.round3_check()
            if bad:
                self.log(f"WARNING: round 3's results changed during the run: {bad}")
            self.state["round3_after"] = bad or "unchanged"
            if self.protected_before is not None:
                after = {k: C.folder_hashes(f) for k, f in C.protected(self.op).items()}
                changed = sorted(k for k in after if after[k] != self.protected_before.get(k))
                self.state["protected_after"] = "unchanged" if not changed else {"changed": changed}
                if changed:
                    self.log(f"WARNING: these results folders changed during the run: {changed}")
            self.save()
            import m3_analyze                              # step 7, after the last timed block
            m3_analyze.report(self.out)
            self.log(f"results: {self.out / 'results.md'}")

    def rest(self):
        self.step4_gates()
        self.step5_counts()
        assert threading.active_count() == 1, "the worker pool must be shut down before step 6 (rule-m3 item 5)"
        self.step6_rounds()

    def do_rerun(self, which):
        """rule-m3 item 12: the rows re-run in full after a fix under review; their earlier results are dropped."""
        impls = C.RERUN[which]
        rows = {r for i in impls for r in C.IMPL_ROWS[i]}
        self.state.setdefault("reruns", []).append({"which": which, "time": stamp()})
        for k in ("gates", "counts", "mems"):
            self.state[k] = {key: v for key, v in self.state.get(k, {}).items() if key.split(":")[0] not in impls}
        self.state["records"] = [r for r in self.state.get("records", []) if r["row"] not in rows]
        self.state["calibration"] = {k: v for k, v in self.state.get("calibration", {}).items()
                                     if k.split(":")[1] not in rows}
        self.state["stopped_impls"] = [i for i in self.state.get("stopped_impls", []) if i not in impls]
        for b in self.state.get("bugs", []):
            if b["impl"] in impls:
                b["rerun"] = stamp()
        self.state["rerun_only"] = list(impls)
        self.step1()
        self.unit_tests()
        self.step4_gates(list(impls))
        self.step5_counts(list(impls))
        assert threading.active_count() == 1
        self.step6_rounds()
        self.state.pop("rerun_only", None)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--op", choices=C.OPS, required=True, help="the job set: add, mul, sop or pos (one run each)")
    ap.add_argument("--toy", action="store_true", help="toy workloads in test/build/m3toy/ (harness tests only)")
    ap.add_argument("--jobs", default="", help="toy runs only: a subset of jobs")
    ap.add_argument("--pilot-only", action="store_true")
    ap.add_argument("--hard-stop-s", type=float, default=None, help="toy runs only: a shorter hard stop")
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--rerun", choices=sorted(C.RERUN))
    a = ap.parse_args()
    r = Runner(a)
    r.preflight()
    r.start()


if __name__ == "__main__":
    main()
