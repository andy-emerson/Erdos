"""The runner of research/rule.md: order of work, pilot, gates, counting runs, calibration, 12 rounds, hard stop.

    python3 runner.py                      the measured run: results in test/results/ (refuses unless rule.md is the
                                           fixed rule, lib/ builds, and the machine has 2 cores)
    python3 runner.py --pilot-only         steps 1-3 (generation, unit tests, pilot with its projection), then stop
    python3 runner.py --resume             continue a run stopped by a failed gate, after the fix: re-runs the unit
                                           tests and every gate not passed, then goes on (refused once a timed round
                                           of step 6 has run); gate and counting results of other code (their
                                           code hash differs) are dropped and redone
    python3 runner.py --rerun D2           item 5: after a bug in a design is fixed under review, re-run that design
                                           in full (unit tests, its gates, counting runs, calibration, every round of
                                           every job), its earlier results discarded; ctl re-runs ctl and ctl-null;
                                           "all" re-runs every row (a fix to the shared integer routines); any
                                           design whose stored gate or counting results ran other code is re-run
                                           too, so a change to a shared file re-runs every row
    python3 runner.py --stub --toy [--jobs 1,3,19,20,22]
                                           harness test only: the stub of test/stub/ at toy size, in build/toy/

Order of work (rule.md): 1 generate workloads and references; 2 unit tests at widths 64 and 32; 3 pilot (timed, then
the gate of job 20) and its projection, stopping if it exceeds 4 hours; 4 gates, two at a time; 5 counting runs, two at
a time; 6 calibration and 12 rounds, job by job, one timed invocation at a time on core 1. The whole run has a hard
stop at 4 hours, checked between invocations and, by a timeout on every process, during them. Before any timed run
(the timed pilot, and each job's calibration and rounds) the machine must be idle: load average and other processes'
CPU use under the thresholds of common.py; if not, the runner waits, logging it, and after 10 minutes refuses (R-p).
"""
import argparse
import concurrent.futures as cf
import datetime
import hashlib
import json
import os
import queue
import shutil
import subprocess
import sys
import time
from pathlib import Path

import numpy as np

import common as C
import gen
import libconf


class HardStop(Exception):
    pass


class RunStop(Exception):
    """The run stops and is reported to the author (projection over 4 hours, failed gate, failed pilot check)."""


def _proc_cpu_ticks(exclude):
    """{pid: (name, utime + stime in clock ticks)} for every process in /proc, except the pids in exclude."""
    out = {}
    for d in os.listdir("/proc"):
        if not d.isdigit() or int(d) in exclude:
            continue
        try:
            raw = Path(f"/proc/{d}/stat").read_text()
        except OSError:
            continue
        name = raw[raw.find("(") + 1:raw.rfind(")")]
        f = raw[raw.rfind(")") + 2:].split()
        out[int(d)] = (name, int(f[11]) + int(f[12]))   # fields 14 and 15 of stat: utime, stime
    return out


def idle_probe(sample_s=None):
    """Is the machine idle?  Returns (idle, detail): the 1-minute load average against C.IDLE_LOAD_MAX, and every other
    process whose CPU use over a sample of sample_s seconds exceeds C.IDLE_PROC_CPU_MAX of one core.  The runner's own
    process is excluded; it has no child while it checks."""
    sample_s = C.IDLE_SAMPLE_S if sample_s is None else sample_s
    me = {os.getpid()}
    hz = os.sysconf("SC_CLK_TCK")
    a = _proc_cpu_ticks(me)
    time.sleep(sample_s)
    b = _proc_cpu_ticks(me)
    busy = []
    for pid, (name, t1) in b.items():
        t0 = a.get(pid, (name, t1))[1]                  # a process started during the sample counts from then on
        share = (t1 - t0) / hz / sample_s
        if share > C.IDLE_PROC_CPU_MAX:
            busy.append({"pid": pid, "name": name, "cpu": round(share, 3)})
    load1 = os.getloadavg()[0]
    idle = load1 <= C.IDLE_LOAD_MAX and not busy
    return idle, {"load1": round(load1, 2), "load_max": C.IDLE_LOAD_MAX, "busy": busy,
                  "proc_cpu_max": C.IDLE_PROC_CPU_MAX}


def stamp():
    return datetime.datetime.now().astimezone().isoformat(timespec="seconds")


def code_hashes(lib):
    """The code version of every implementation: {impl: sha256 of its own files, the shared files, the compiler and
    flags}, and the sha256 of the shared integer module alone (libconf.py). A shared file is in every impl's hash."""
    def h(paths, extra=""):
        s = hashlib.sha256(extra.encode())
        for p in sorted(set(paths), key=str):
            s.update(Path(p).name.encode() + b"\0" + Path(p).read_bytes() + b"\0")
        return s.hexdigest()
    flags = json.dumps([libconf.CC, libconf.CFLAGS, lib["defs"], lib["mode_defs"]])
    code = {impl: h(list(lib["shared_files"]) + list(lib["own_files"][impl]), flags) for impl in libconf.IMPL_DEFS}
    return code, h([lib["int_module"]])


class Runner:
    def __init__(self, a):
        self.a = a
        self.toy = a.toy
        self.lib = libconf.STUB if a.stub else libconf.LIB
        base = C.BUILD / "toy" if a.toy else C.HERE
        self.out = (base / "results") if a.toy else C.RESULTS
        self.wl = (C.BUILD / "toy" / "work") if a.toy else (C.BUILD / "work")
        self.bin = (C.BUILD / "toy" / "bin") if a.toy else (C.BUILD / "bin")
        self.tmp = (C.BUILD / "toy" / "readout") if a.toy else (C.BUILD / "readout")
        self.jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else sorted(C.JOBS)
        if 19 in self.jobs or 20 in self.jobs:          # the arrival pair runs together
            self.jobs = sorted(set(self.jobs) | {19, 20})
        self.hard_stop = C.HARD_STOP_S
        if a.hard_stop_s is not None:                   # harness tests of the stop paths only
            if not a.toy:
                raise SystemExit("--hard-stop-s is for toy runs only")
            self.hard_stop = a.hard_stop_s
        self.state_file = self.out / "state.json"
        self.state = None
        self.t0 = None
        self.code, self.code_int = code_hashes(self.lib)
        self.idle_probe = idle_probe                   # replaceable in the harness self-tests
        self.cores = queue.Queue()
        for c in (C.HARNESS_CORE, C.TIMED_CORE):
            self.cores.put(c)

    # ------------------------------------------------------------------------------------------- clock and state
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

    # ------------------------------------------------------------------------------------------- processes
    def proc(self, cmd, core, limit=None, where=""):
        """Run cmd pinned to core, with a timeout of min(limit, time left before the hard stop).
        Returns (CompletedProcess or None if the own limit passed, wall seconds)."""
        self.check_clock(where)
        rem = self.remaining()
        t = rem if limit is None else min(limit, rem)
        start = time.monotonic()
        try:
            r = subprocess.run(["taskset", "-c", str(core)] + [str(x) for x in cmd], capture_output=True, text=True,
                               timeout=max(t, 0.001))
        except subprocess.TimeoutExpired:
            if limit is not None and limit < rem:
                return None, time.monotonic() - start
            raise HardStop(where)
        return r, time.monotonic() - start

    def untimed(self, tasks):
        """Untimed work (steps 1-5 only), two processes at a time, one per core. tasks: list of (key, fn(core))."""
        def run(fn):
            core = self.cores.get()
            try:
                return fn(core)
            finally:
                self.cores.put(core)
        res = {}
        with cf.ThreadPoolExecutor(max_workers=2) as ex:
            futs = {ex.submit(run, fn): key for key, fn in tasks}
            stop = None
            for f in cf.as_completed(futs):
                try:
                    res[futs[f]] = f.result()
                except HardStop as e:
                    stop = e
            if stop:
                raise stop
        return res

    # ------------------------------------------------------------------------------------------- builds
    def build(self):
        self.bin.mkdir(parents=True, exist_ok=True)
        lib = self.lib
        inc = sum((["-I", str(p)] for p in lib["include"]), [])
        for impl in C.IMPLS:
            for mode in ("timing", "counting", "test"):
                for w in (64, 32):
                    if impl == "ceil" and w == 32:
                        continue
                    exe = self.bin / f"bench-{impl}-{mode}-w{w}"
                    cmd = ([libconf.CC] + libconf.CFLAGS + lib["defs"] + lib["mode_defs"][mode] +
                           libconf.width_defs(w) + [libconf.IMPL_DEFS[impl]] + inc +
                           [str(C.HERE / "bench.c")] + [str(s) for s in lib["sources"][impl]] + ["-o", str(exe)])
                    r = subprocess.run(cmd, capture_output=True, text=True)
                    if r.returncode:
                        raise RunStop(f"build failed: {' '.join(cmd)}\n{r.stderr}")
        units = []
        for i, srcs in enumerate(lib["unit_tests"]):
            for w in (64, 32):
                exe = self.bin / f"unit-{i}-w{w}"
                cmd = ([libconf.CC] + libconf.CFLAGS + lib["defs"] + lib["mode_defs"]["test"] + libconf.width_defs(w) +
                       inc + [str(s) for s in srcs] + ["-o", str(exe)])
                r = subprocess.run(cmd, capture_output=True, text=True)
                if r.returncode:
                    raise RunStop(f"unit-test build failed: {' '.join(cmd)}\n{r.stderr}")
                units.append(exe)
        if not units:
            raise RunStop("no unit tests are configured (libconf.py)")
        self.units = units

    def exe(self, impl, mode, job):
        return self.bin / f"bench-{impl}-{mode}-w{C.JOBS[job]['width']}"

    def wlpath(self, job):
        return self.wl / f"job-{job:02d}.wl"

    # ------------------------------------------------------------------------------------------- checks
    def check(self, job, readout):
        """Every stored value against the exact reference (rule item 5). None, or the first mismatch."""
        rn, rd = C.read_pairs(self.wl / f"job-{job:02d}.ref")
        try:
            on, od = C.read_pairs(readout)
        except OSError:
            return {"call": None, "what": "no read-out"}
        if len(on) != len(rn):
            return {"call": None, "what": f"read-out has {len(on)} values, reference {len(rn)}"}
        bad = np.nonzero((on != rn) | (od != rd))[0]
        if len(bad) == 0:
            return None
        i = int(bad[0])
        w = C.read_workload(self.wlpath(job))
        return {"call": i, "op": C.OPNAME[int(w["op"][i])], "got": f"{on[i]}/{od[i]}", "want": f"{rn[i]}/{rd[i]}",
                "mismatches": int(len(bad))}

    @staticmethod
    def parse_error(stdout):
        for line in stdout.splitlines():
            if line.startswith(("error ", "auditfail ")):
                return line
        return None

    # ------------------------------------------------------------------------------------------- idle machine
    def wait_idle(self, where):
        """Before any timed run (the timed pilot, and each job's calibration and rounds in step 6): the machine must be
        idle (R-p).  If it is busy, re-check every IDLE_POLL_S for up to IDLE_WAIT_S, logging each check, then refuse.
        The wait counts on the run clock.  Toy runs (harness tests) log the check and never wait or refuse."""
        t = time.monotonic()
        checks = self.state.setdefault("idle_checks", [])
        while True:
            self.check_clock(where)
            ok, d = self.idle_probe()
            rec = dict(d, where=where, time=stamp(), idle=ok, waited_s=round(time.monotonic() - t, 1))
            checks.append(rec)
            self.save()
            if ok:
                if rec["waited_s"] > 0:
                    self.log(f"machine idle before {where}, after waiting {rec['waited_s']:.0f} s")
                return
            what = f"load {d['load1']} (max {d['load_max']})" + (
                "; busy: " + ", ".join(f"{b['name']}[{b['pid']}] {b['cpu']:.0%}" for b in d["busy"]) if d["busy"]
                else "")
            if self.toy:
                self.log(f"machine busy before {where} ({what}); toy run: not waiting")
                return
            if time.monotonic() - t >= C.IDLE_WAIT_S:
                self.log(f"machine still busy before {where} after {C.IDLE_WAIT_S:.0f} s ({what}): refusing to time")
                raise RunStop(f"the machine is not idle before {where} ({what}); timed runs need an idle machine")
            self.log(f"machine busy before {where} ({what}); waiting {C.IDLE_POLL_S:.0f} s")
            time.sleep(C.IDLE_POLL_S)

    # ------------------------------------------------------------------------------------------- timed invocations
    def timed(self, job, row, kind, rnd=None, pos=None, r=None, reps=None):
        """One timed invocation on core 1; nothing else runs meanwhile. kind: calib or time."""
        impl = C.ROW_IMPL[row]
        ro = self.tmp / f"t-{job:02d}-{row}.out"
        if ro.exists():
            ro.unlink()
        cmd = [self.exe(impl, "timing", job), "calib" if kind == "calib" else "time", self.wlpath(job), ro]
        if kind != "calib":
            cmd += [r, reps]
        p, wall = self.proc(cmd, C.TIMED_CORE, where=f"step 6, job {job}, {row}, {kind} {rnd}")
        rec = {"job": job, "row": row, "kind": kind, "round": rnd, "pos": pos, "wall": wall, "rc": p.returncode}
        lines = p.stdout.splitlines()
        reps_out = [list(map(int, l.split()[1:])) for l in lines if l.startswith(("calib ", "rep "))]
        rec["overcap"] = any(l.startswith("overcap") for l in lines)
        rec["reps_out"] = reps_out
        err = self.parse_error(p.stdout)
        if p.returncode or err:
            rec["bug"] = {"what": err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"}
            return rec
        bad = self.check(job, ro)
        if bad:
            rec["bug"] = bad
            return rec
        if kind == "calib":
            last = reps_out[-1]
            rec["r"] = last[0]
            rec["rep_ns"] = last[1]                     # the calibration's rep (its whole duration, r runs)
            rec["reps"] = 1 if last[1] >= C.ONE_REP_NS else 3
        else:
            rec["r"], rec["reps"] = r, reps
            if not rec["overcap"]:
                kept = reps_out if reps == 1 else reps_out[1:]
                best = min(kept, key=lambda x: x[1])   # the rep of minimum total; its phases are reported with it
                rec["total"], rec["entry"], rec["ops"], rec["read"] = (best[1] / r, best[2] / r, best[3] / r,
                                                                       best[4] / r)
        if rec["overcap"]:
            last = reps_out[-1]
            rec["cap_rep_ns"] = last[1]
            rec["cap_time"] = last[1] / (last[0] if kind == "calib" else r)
        return rec

    def calibrate(self, jobs, store, live):
        """Calibration invocations (ctl, D1, D2, ceil), on core 1, before round 1; ctl-null takes ctl's."""
        cal = {}
        for job in jobs:
            for row in C.rows_of(job):
                if row == "ctl-null" or row not in live(job):
                    continue
                rec = self.timed(job, row, "calib")
                store.append(rec)
                cal[(job, row)] = rec
                if "bug" in rec:
                    self.bug(rec, "calibration")
                    continue
            if (job, "ctl") in cal and "bug" not in cal[(job, "ctl")]:
                cal[(job, "ctl-null")] = dict(cal[(job, "ctl")], row="ctl-null", shared_from="ctl")
        return cal

    def rounds(self, jobs, nrounds, cal, store, live, label):
        """Interleaved rounds: every row once per round, as its own process, order rotated by one per round."""
        order = [(j, row) for j in jobs for row in C.rows_of(j)]
        capped = set()
        for k in range(nrounds):
            rot = order[k % len(order):] + order[:k % len(order)]
            for pos, (job, row) in enumerate(rot):
                c = cal.get((job, row))
                if c is None or "bug" in c or c.get("overcap") or (job, row) in capped or row not in live(job):
                    continue
                rec = self.timed(job, row, "time", rnd=k + 1, pos=pos, r=c["r"], reps=c["reps"])
                rec["label"] = label
                store.append(rec)
                if "bug" in rec:
                    self.bug(rec, f"{label} round {k + 1}")
                elif rec["overcap"]:
                    capped.add((job, row))
                self.save()

    # ------------------------------------------------------------------------------------------- bugs (item 5)
    def bug(self, rec, where):
        design = C.ROW_IMPL[rec["row"]]
        entry = {"design": design, "row": rec["row"], "job": rec["job"], "where": where, "detail": rec["bug"],
                 "time": stamp()}
        if not self.state.get("measuring"):
            self.state.setdefault("pre_gate_failures", []).append(entry)
            self.save()
            raise RunStop(f"check failure before the gates ({where}): {entry}")
        self.state.setdefault("bugs", []).append(entry)
        if design not in self.state.setdefault("stopped_designs", []):
            self.state["stopped_designs"].append(design)
        self.log(f"BUG: {entry}; the measured run of {design} stops (rule item 5)")
        self.save()

    def live_rows(self, job):
        stopped = set(self.state.get("stopped_designs", []))
        return [r for r in C.rows_of(job) if C.ROW_IMPL[r] not in stopped]

    # ------------------------------------------------------------------------------------------- code versions
    def stamp_code(self, impl, res):
        res["code"], res["int"] = self.code[impl], self.code_int
        return res

    def is_stale(self, key, res):
        """A stored gate or counting result ran other code than the current (its own files, a shared file, or the
        shared integer module changed), or predates code hashes."""
        impl = key.split(":")[0]
        return res.get("code") != self.code[impl] or res.get("int") != self.code_int

    def drop_stale(self):
        """Drop every gate and counting result whose code hash differs from the current code; returns the
        implementations that had one. Called on --resume and --rerun before anything runs."""
        dropped, impls, int_changed = self.state.setdefault("dropped_stale", []), set(), False
        for kind in ("gates", "counts"):
            kept = {}
            for k, v in self.state.get(kind, {}).items():
                if self.is_stale(k, v):
                    impls.add(k.split(":")[0])
                    int_changed |= v.get("int") != self.code_int
                    dropped.append({"kind": kind, "key": k, "code": v.get("code"), "int": v.get("int"),
                                    "int_changed": v.get("int") != self.code_int, "time": stamp()})
                else:
                    kept[k] = v
            self.state[kind] = kept
        if impls:
            self.log(f"code changed since these results: dropped the stale gates and counting runs of {sorted(impls)}"
                     + (" (the shared integer module changed)" if int_changed else ""))
        return impls

    def plan_rerun(self, design):
        """The designs a --rerun re-runs in full: the one named, and every design with a stale result (a shared-file
        change makes every design stale). Their earlier results are discarded."""
        designs = set(C.IMPLS) if design == "all" else {design}
        stale = self.drop_stale()
        designs |= stale
        rows = {r for d in designs for r in C.DESIGN_ROWS[d]}
        prev = self.state.setdefault("reruns", [])
        prev.append({"design": design, "also_stale": sorted(stale - {design}), "time": stamp()})
        self.state["records"] = [r for r in self.state.get("records", []) if r["row"] not in rows]
        self.state["calibration"] = {k: v for k, v in self.state.get("calibration", {}).items()
                                     if k.split(":")[1] not in rows}
        for key in ("gates", "counts"):
            self.state[key] = {k: v for k, v in self.state.get(key, {}).items() if k.split(":")[0] not in designs}
        self.state["stopped_designs"] = [d for d in self.state.get("stopped_designs", []) if d not in designs]
        for b in self.state.get("bugs", []):
            if b["design"] in designs:
                b["rerun"] = prev[-1]["time"]
        return [d for d in C.IMPLS if d in designs]

    def gate_pairs(self, impls_only=None, skip_passed=True):
        done = self.state.setdefault("gates", {})
        pairs = []
        for job in self.constructible():
            for impl in self.impls_of(job, impls_only):
                k = f"{impl}:{job}"
                g = done.get(k)
                if skip_passed and g and "fail" not in g and not self.is_stale(k, g):
                    continue
                pairs.append((impl, job))
        return pairs

    # ------------------------------------------------------------------------------------------- untimed runs
    def gate(self, impl, job, core):
        ro = self.tmp / f"g-{job:02d}-{impl}.out"
        p, wall = self.proc([self.exe(impl, "test", job), "gate", self.wlpath(job), ro], core,
                            where=f"gate {impl} job {job}")
        res = self.stamp_code(impl, {"impl": impl, "job": job, "wall": wall, "rc": p.returncode})
        err = self.parse_error(p.stdout)
        if p.returncode or err:
            res["fail"] = err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"
        else:
            bad = self.check(job, ro)
            if bad:
                res["fail"] = f"check: {bad}"
            res["audits"] = next((l for l in p.stdout.splitlines() if l.startswith("audits")), "")
        return res

    def count(self, impl, job, core, rep=0):
        ro = self.tmp / f"c-{job:02d}-{impl}-{rep}.out"
        p, wall = self.proc([self.exe(impl, "counting", job), "count", self.wlpath(job), ro], core,
                            limit=C.COUNT_LIMIT_S, where=f"counting {impl} job {job}")
        res = self.stamp_code(impl, {"impl": impl, "job": job, "rep": rep, "wall": wall})
        if p is None:
            res["na"] = f"did not finish within the {C.COUNT_LIMIT_S // 60}-minute limit"
            return res
        err = self.parse_error(p.stdout)
        if p.returncode or err:
            res["bug"] = {"what": err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"}
            return res
        bad = self.check(job, ro)
        if bad:
            res["bug"] = bad
            return res
        res["counts"] = [l for l in p.stdout.splitlines() if l.startswith(("ctr ", "hist "))]
        res["mem"] = [l for l in p.stdout.splitlines() if l.startswith(("mem ", "memN "))]
        return res

    def impls_of(self, job, only=None):
        out = ["ctl", "D1", "D2"] + (["ceil"] if C.JOBS[job]["ceil"] else [])
        return [i for i in out if only is None or i in only]

    def unit_tests(self):
        res = {}
        for exe in self.units:
            p, wall = self.proc([exe], C.HARNESS_CORE, where="unit tests")
            res[exe.name] = {"rc": p.returncode, "out": (p.stdout + p.stderr)[-2000:]}
        self.state["unit_tests"] = res
        failed = [k for k, v in res.items() if v["rc"]]
        if failed:
            self.save()
            raise RunStop(f"unit tests failed: {failed}")
        self.log(f"unit tests pass: {sorted(res)}")

    def run_gates(self, pairs, label):
        tasks = [((impl, job), (lambda core, i=impl, j=job: self.gate(i, j, core))) for impl, job in pairs]
        res = self.untimed(tasks)
        gates = self.state.setdefault("gates", {})
        for (impl, job), r in res.items():
            gates[f"{impl}:{job}"] = r
        self.save()
        failed = {k: v["fail"] for k, v in res.items() if "fail" in v}
        if failed:
            raise RunStop(f"{label}: gates failed (fix the row, then --resume): {failed}")
        return res

    def run_counts(self, impls_only=None):
        tasks = []
        for job in self.jobs:
            for impl in self.impls_of(job, impls_only):
                tasks.append(((impl, job, 0), (lambda core, i=impl, j=job: self.count(i, j, core, 0))))
                if job in C.COUNT_REPEAT_JOBS:
                    tasks.append(((impl, job, 1), (lambda core, i=impl, j=job: self.count(i, j, core, 1))))
        res = self.untimed(tasks)
        counts = self.state.setdefault("counts", {})
        for (impl, job, rep), r in res.items():
            if rep == 0:
                counts[f"{impl}:{job}"] = r
        for (impl, job, rep), r in res.items():
            if rep == 1:
                first = res[(impl, job, 0)]
                if "counts" in r and "counts" in first and (r["counts"] != first["counts"] or r["mem"] != first["mem"]):
                    r["bug"] = {"what": "counting run repeated: counts differ (rule item 2)"}
                counts[f"{impl}:{job}"]["repeat"] = "identical" if "bug" not in r else r["bug"]["what"]
            if "bug" in r:
                self.bug(dict(row=impl, job=job, bug=r["bug"]), "counting run" + (" (repeat)" if rep else ""))
        self.save()

    # ------------------------------------------------------------------------------------------- results file
    def write_header(self):
        rule = C.RULE_FILE.read_text()
        head = ["# Skeleton baselines: results", "",
                f"Run started {stamp()}." + (" **TOY RUN with the stub: a harness test, not a measurement.**"
                                             if self.toy or self.a.stub else ""), "",
                "## The measurement rule (research/rule.md, verbatim, written before the first trial)", "",
                f"sha256 `{C.rule_sha256()}`", "", "<!-- rule begins -->", rule.rstrip("\n"), "<!-- rule ends -->", "",
                "## Calibration (written before each job's round 1)", "",
                "| job | row | r | calibration rep (ms, r runs) | reps per invocation |", "| --- | --- | --- | --- | --- |"]
        (self.out / "results.md").write_text("\n".join(head) + "\n")

    def append_calibration(self, cal):
        lines = []
        for (job, row), c in sorted(cal.items(), key=lambda x: (x[0][0], C.rows_of(x[0][0]).index(x[0][1]))):
            if "bug" in c:
                lines.append(f"| {job} | {row} | - | bug: see Bugs | - |")
            elif c.get("overcap"):
                lines.append(f"| {job} | {row} | {c['reps_out'][-1][0]} | over cap: {c['cap_rep_ns'] / 1e6:.1f} | - |")
            else:
                src = " (ctl's)" if c.get("shared_from") else ""
                lines.append(f"| {job} | {row} | {c['r']}{src} | {c['rep_ns'] / 1e6:.2f}{src} | {c['reps']}{src} |")
        with open(self.out / "results.md", "a") as f:
            f.write("\n".join(lines) + "\n")

    # ------------------------------------------------------------------------------------------- steps
    def preflight(self):
        if C.rule_sha256() != C.RULE_SHA256:
            msg = "research/rule.md differs from the fixed rule (sha256 mismatch): refusing to run"
            if not self.toy:
                raise SystemExit(msg)
            print("WARNING (toy run): " + msg)
        if not self.toy:
            if self.a.stub:
                raise SystemExit("the stub is for harness tests only (--toy): refusing a measured run with it")
            if len(os.sched_getaffinity(0)) < 2 or os.cpu_count() < 2:
                raise SystemExit("the rule needs 2 cores (item 4)")
            if self.a.jobs:
                raise SystemExit("a measured run runs every job of the run plan")
        os.sched_setaffinity(0, {C.HARNESS_CORE})

    def step1(self):
        self.log("step 1: workloads and exact references")
        if self.wl.exists():
            shutil.rmtree(self.wl)
        metas = gen.generate(self.wl, self.jobs, toy=self.toy)
        self.state["gen"] = {str(k): v for k, v in metas.items()}
        self.state["workload_sha256"] = {str(j): hashlib.sha256(self.wlpath(j).read_bytes()).hexdigest()
                                         for j in self.jobs if metas[j].get("constructible", True)}
        self.state["not_constructible"] = [j for j in self.jobs if not metas[j].get("constructible", True)]
        self.save()

    def constructible(self):
        return [j for j in self.jobs if j not in self.state.get("not_constructible", [])]

    def step3_pilot(self):
        self.log("step 3: pilot (timed part)")
        jobs = [j for j in C.PILOT_TIMED_JOBS if j in self.constructible()]
        store = self.state.setdefault("pilot_records", [])
        self.wait_idle("the timed pilot")
        t = time.monotonic()
        for job in jobs:
            cal = self.calibrate([job], store, self.live_rows)
            self.rounds([job], C.PILOT_ROUNDS, cal, store, self.live_rows, "pilot")
        timed_wall = time.monotonic() - t
        gate_wall = 0.0
        if C.PILOT_GATE_JOB in self.constructible():
            self.log("step 3: pilot (gate of job 20)")
            t = time.monotonic()
            self.run_gates([(i, C.PILOT_GATE_JOB) for i in self.impls_of(C.PILOT_GATE_JOB)], "gate pilot")
            gate_wall = time.monotonic() - t
            self.state["gate_pilot_passed"] = True
        r1, r2 = timed_wall / 120.0, gate_wall / 120.0
        proj = self.elapsed() + 35 * 60 * r1 + 10 * 60 * r2 + 3 * 60 * max(r1, r2)
        self.state["pilot"] = {"timed_wall_s": timed_wall, "gate_wall_s": gate_wall, "timed_ratio": r1,
                               "gate_ratio": r2, "elapsed_s": self.elapsed(), "projected_s": proj,
                               "discarded": "the pilot's results and calibration are discarded (rule step 3)"}
        self.save()
        self.log(f"pilot projection: {proj / 3600:.2f} h (timed pilot {timed_wall:.1f} s, gate pilot {gate_wall:.1f} s)")
        if proj > self.hard_stop:
            raise RunStop(f"projected total {proj / 3600:.2f} h exceeds {self.hard_stop / 3600:g} hours: the run stops and is reported to the "
                          "author before anything else runs")

    def step4_gates(self, impls_only=None, skip_passed=True):
        self.log("step 4: gates")
        self.run_gates(self.gate_pairs(impls_only, skip_passed), "step 4")
        self.state["measuring"] = True          # every gate passed: a check failure from here on is a bug (item 5)
        self.save()

    def step6_timed(self, rows_only=None, label="measured"):
        self.log("step 6: calibration and rounds")
        store = self.state.setdefault("records", [])
        cals = self.state.setdefault("calibration", {})

        def live(job):
            rows = self.live_rows(job)
            return [r for r in rows if rows_only is None or C.ROW_IMPL[r] in rows_only]

        groups = []
        for j in self.constructible():
            if j == 20:
                continue
            groups.append([19, 20] if j == 19 else [j])
        for jobs in groups:
            jobs = [j for j in jobs if j in self.constructible()]
            self.wait_idle(f"the calibration and rounds of jobs {jobs}")
            self.log(f"jobs {jobs}: calibration")
            cal = self.calibrate(jobs, store, live)
            for (job, row), c in cal.items():
                cals[f"{job}:{row}"] = c
            self.append_calibration(cal)
            self.save()
            self.log(f"jobs {jobs}: {C.ROUNDS} rounds")
            self.rounds(jobs, C.ROUNDS, cal, store, live, label)
        self.state["finished"] = stamp()
        self.save()

    # ------------------------------------------------------------------------------------------- top level
    def start(self):
        self.out.mkdir(parents=True, exist_ok=True)
        self.tmp.mkdir(parents=True, exist_ok=True)
        if self.a.resume or self.a.rerun:
            self.state = json.loads(self.state_file.read_text())
            if self.state.get("hard_stopped"):
                raise SystemExit("this run was stopped at 4 hours: nothing more runs until the author decides")
            self.state["elapsed_before"] = self.state.get("elapsed_saved", 0)
        else:
            for f in ("run.log", "state.json"):
                if (self.out / f).exists():
                    (self.out / f).unlink()
            self.state = {"started": stamp(), "elapsed_before": 0.0, "toy": self.toy, "stub": self.a.stub,
                          "jobs": self.jobs, "rule_sha256": C.rule_sha256(), "rule_text": C.RULE_FILE.read_text()}
            self.write_header()                      # the rule, verbatim, before the first trial
        self.state.setdefault("code_sessions", []).append({"time": stamp(), "code": self.code, "int": self.code_int})
        self.t0 = time.monotonic()                   # the clock runs from step 1 (accumulated over resumed sessions)
        if self.a.resume:
            self.drop_stale()                        # results of other code are dropped and redone (--rerun: plan_rerun)
            self.save()
        step = "build"
        try:
            self.build()
            if self.a.rerun:
                step = "re-run"
                self.do_rerun(self.a.rerun)
            elif self.a.resume:
                self.state.pop("run_stopped", None)
                if self.state.get("records"):
                    raise SystemExit("--resume is refused once a timed round has run; see --rerun")
                step = "step 2"
                self.unit_tests()
                if "pilot" not in self.state:
                    step = "step 3"
                    self.state["pilot_records"] = []
                    self.step3_pilot()
                step = "step 4"
                self.step4_gates()
                step = "step 5"
                self.log("step 5: counting runs")
                self.run_counts()
                step = "step 6"
                self.step6_timed()
            else:
                step = "step 1"
                self.step1()
                step = "step 2"
                self.log("step 2: unit tests (test build, widths 64 and 32)")
                self.unit_tests()
                step = "step 3"
                self.step3_pilot()
                if self.a.pilot_only:
                    self.log("pilot only: stopping after step 3")
                    return
                step = "step 4"
                self.step4_gates()
                step = "step 5"
                self.log("step 5: counting runs")
                self.run_counts()
                step = "step 6"
                self.step6_timed()
        except HardStop as e:
            self.state["hard_stopped"] = {"step": step, "where": str(e), "elapsed_s": self.elapsed()}
            self.save()
            self.log(f"STOPPED AT 4 HOURS in {step} ({e}); reporting what was done, with no verdicts")
        except RunStop as e:
            self.state["run_stopped"] = {"step": step, "reason": str(e), "elapsed_s": self.elapsed()}
            self.save()
            self.log(f"RUN STOPPED in {step}: {e}")
        finally:
            self.save()
            import analyze
            analyze.report(self.out)
            self.log(f"results: {self.out / 'results.md'}")

    def do_rerun(self, design):
        designs = self.plan_rerun(design)
        self.log(f"re-run of {designs} in full")
        self.save()
        self.unit_tests()
        self.step4_gates(impls_only=designs, skip_passed=False)
        self.log("step 5 (re-run): counting runs")
        self.run_counts(impls_only=designs)
        self.step6_timed(rows_only=designs, label="re-run")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--stub", action="store_true", help="use test/stub (harness tests only; needs --toy)")
    ap.add_argument("--toy", action="store_true", help="toy-size workloads in build/toy/ (harness tests only)")
    ap.add_argument("--jobs", default="", help="toy runs only: a subset of jobs")
    ap.add_argument("--pilot-only", action="store_true")
    ap.add_argument("--hard-stop-s", type=float, default=None, help="toy runs only: a shorter hard stop, to test it")
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--rerun", choices=("ctl", "D1", "D2", "ceil", "all"))
    a = ap.parse_args()
    r = Runner(a)
    r.preflight()
    r.start()


if __name__ == "__main__":
    main()
