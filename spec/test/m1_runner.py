"""The runner of research/rule-m1.md (the matrix round: TC and the five hash rows, spec v0.1). From round 3's
r3_runner.py (../../10-interning/test/, unchanged there), with round 3's whole timing method: the fork/exec spawner
pinned before exec, the single-threaded runner during timed phases, the kernel timer for the hard stop, the idle
check, the disturbance check by process with the pilot's host allowance, calibration, the 12 interleaved rounds and the
stops. What changes: two runs, one per operation (--op add, then --op mul after the author has seen the addition
results), each with its own job set, pilot, hard stop (40 minutes) and results folder; seven rows (TC, the five hash
rows and Hash-V-fixed-null, Hash-V-fixed's binary); step 0, untimed and outside the hard stop (workloads, references,
predictions); the TC gate against tc3.c; the call-for-call, V/M, grow/fixed and map/lookup checks; the pilot on jobs 1,
6 and 13 with its projection; no profile.

THE OPERATOR (rule-m1 item 5, carried over) starts it ONCE per run, with one command that returns only when it exits:
    python3 test/m1_runner.py --op add
either in the foreground, or as a background command that notifies the operator when it exits (for Claude Code: the
Bash tool with run_in_background and a timeout of at least 45 minutes). Until that exit notice the operator runs
NOTHING else on the machine: no polling or waiting loop, no sleep, watch, tail -f, file check, Monitor or until-loop,
no other tool call, build, test or edit, and no detached process.

    python3 m1_runner.py --op add          the addition run: results in test/results-m1-add/ (refuses unless
                                           rule-m1.md is the fixed rule, the 08 module and tc3.c are unchanged,
                                           tcm.c's diff is the recorded one, round 3's results are unchanged, and the
                                           machine has 2 cores)
    python3 m1_runner.py --op mul          the multiplication run: results in test/results-m1-mul/ (also refuses
                                           unless test/results-m1-add/ holds a finished addition run, refuses to
                                           write in the addition run's folder, and checks its files before and after)
    ... --pilot-only                       steps 0-3, then stop
    ... --resume                           continue a run stopped before step 6 (a failed gate, or --pilot-only):
                                           re-runs the unit tests and every gate not passed, then goes on
    ... --rerun TC|HASH                    rule-m1 item 12: after a bug is fixed under review, re-run TC, or every
                                           hash row and the null row, in full (gates, counting and memory runs,
                                           calibration and every round)
    ... --toy [--jobs 1,6] [--hard-stop-s S]
                                           a harness test with toy workloads in test/build/m1toy/: never a measurement

The run plan (rule-m1, Run plan), once per run: 0 before the clock: workloads, references and predictions, with their
sha256; 1 build, the hashes of the 08 module and tc3.c, tcm.c's recorded diff, step 0's files checked; 2 unit tests
(with the allocation check and the sanitizers); 3 the pilot (jobs 1, 6, 13: calibration and 2 rounds), its projection
and stops; 4 gates (two at a time; the TC gate against tc3.c); 5 counting and memory runs (two at a time; the worker
pool is then shut down and joined); 6 per job: the idle check, calibration (6 launches) and 12 rounds (84 launches),
the disturbance check; 7 the analysis. The hard stop: 40 minutes from step 1 to the end of step 6.
"""
import argparse
import concurrent.futures as cf
import datetime
import json
import os
import queue
import shutil
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

import m1_checks as K
import m1_common as C
import m1_libconf as L


class HardStop(Exception):
    pass


class RunStop(Exception):
    """The run stops and is reported to the author."""


def stamp():
    return datetime.datetime.now().astimezone().isoformat(timespec="seconds")


# ------------------------------------------------------------------------------------------------------- /proc
# (round 3's, unchanged: rule-m1 item 5 carries the disturbance check over in full)
def read_stat(pid):
    """(name, ppid, utime, stime, cutime, cstime, starttime, num_threads) of a process from /proc/<pid>/stat, or None."""
    try:
        raw = Path(f"/proc/{pid}/stat").read_text()
    except OSError:
        return None
    name = raw[raw.find("(") + 1:raw.rfind(")")]
    f = raw[raw.rfind(")") + 2:].split()
    return (name, int(f[1]), int(f[11]), int(f[12]), int(f[13]), int(f[14]), int(f[19]), int(f[17]))


def fork_counter():
    """The kernel's fork counter (the "processes" line of /proc/stat). Reported only."""
    for line in Path("/proc/stat").read_text().splitlines():
        if line.startswith("processes "):
            return int(line.split()[1])
    raise RuntimeError("/proc/stat has no processes line")


def cpu_times(cpu):
    """(busy, total) jiffies of one CPU from /proc/stat (busy: total less idle and iowait)."""
    for line in Path("/proc/stat").read_text().splitlines():
        if line.startswith(f"cpu{cpu} "):
            v = [int(x) for x in line.split()[1:]]
            total = sum(v[:8])
            return total - v[3] - v[4], total
    raise RuntimeError(f"/proc/stat has no cpu{cpu} line")


def snapshot():
    """Every process's (name, ppid, utime, stime, cutime, cstime, starttime), and the fork counter, with the time."""
    procs = {}
    for d in os.listdir("/proc"):
        if d.isdigit():
            s = read_stat(int(d))
            if s is not None:
                procs[int(d)] = s
    return {"t": time.monotonic(), "forks": fork_counter(), "procs": procs}


def ancestors(pid=None):
    """The runner's ancestors, following PPid to process 1: [(pid, name)] (the host processes)."""
    out, p = [], os.getppid() if pid is None else pid
    while p > 0:
        s = read_stat(p)
        if s is None:
            break
        out.append((p, s[0]))
        if p == 1:
            break
        p = s[1]
    return out


def is_kthread(pid, st):
    return pid == 2 or st[1] == 2


def ticks4(st):
    return st[2] + st[3] + st[4] + st[5]


def threads_of(st):
    return st[7] if len(st) > 7 else None


def disturbance(s0, s1, runner, spawns, hosts, allowance=None):
    """rule-m1 item 5 (rule-r3 item 5, carried over in full): the check after a timed block, from its two snapshots;
    round 3's function, unchanged. Processes are told apart by process, never by thread; the runner's own ticks are
    reported; a host is held to its allowance (none in the pilot); any other process present in both snapshots to
    5 + ceil(1 x L) ticks; a new process to under 1 tick; a child reaped in the block is charged for the block only;
    kernel threads never flag; the fork counter is reported only."""
    L_s = max(s1["t"] - s0["t"], 1e-9)
    p0, p1 = s0["procs"], s1["procs"]
    other_limit = K.other_limit(L_s)
    rec = {"seconds": round(L_s, 3), "runner_ticks": None, "kthreads": {}, "hosts": {}, "others": [], "new": [],
           "new_quiet": [], "forks_rise": s1["forks"] - s0["forks"], "spawns": spawns, "new_kthreads": 0,
           "other_limit": other_limit, "credited": {}, "reasons": []}
    if runner in p0 and runner in p1:
        rec["runner_ticks"] = (p1[runner][2] + p1[runner][3]) - (p0[runner][2] + p0[runner][3])
    rec["runner_ticks_per_launch"], rec["runner_over_limit"] = K.runner_ticks_check(rec["runner_ticks"], spawns)

    def same(pid):
        return pid in p0 and pid in p1 and p0[pid][6] == p1[pid][6]
    exited = {}
    for pid, st in p0.items():
        if same(pid) or is_kthread(pid, st) or not same(st[1]) or st[1] == runner:
            continue
        exited[st[1]] = exited.get(st[1], 0) + ticks4(st)
    for pid, st in p1.items():
        if pid == runner:
            continue
        before = p0.get(pid) if same(pid) else None
        d = ticks4(st) - (ticks4(before) if before else 0)
        if is_kthread(pid, st):
            if before is None:
                rec["new_kthreads"] += 1
            if d:
                rec["kthreads"][str(pid)] = {"name": st[0], "ticks": d}
            continue
        if before is not None and pid in exited:
            credit = min(exited[pid], max((st[4] + st[5]) - (before[4] + before[5]), 0))
            if credit:
                d -= credit
                rec["credited"][str(pid)] = {"name": st[0], "ticks": credit}
        if pid in hosts and before is not None:
            h = {"name": st[0], "ticks": d, "rate": d / L_s}
            if threads_of(before) is not None and threads_of(st) is not None:
                h["threads"] = [threads_of(before), threads_of(st)]
            if allowance is not None:
                h["allowance"] = allowance.get(pid, C.HOST_BASE_TICKS)
                if d > h["allowance"]:
                    rec["reasons"].append(f"host process {st[0]}[{pid}] used {d} ticks, over its allowance "
                                          f"{h['allowance']}")
            rec["hosts"][str(pid)] = h
            continue
        if before is None:
            if d >= C.DIST_NEW_TICKS:
                rec["new"].append({"pid": pid, "name": st[0], "ppid": st[1], "ticks": d})
                rec["reasons"].append(f"new process {st[0]}[{pid}] (parent {st[1]}) used {d} ticks")
            else:
                rec["new_quiet"].append({"pid": pid, "name": st[0], "ppid": st[1]})
        elif d > other_limit:
            rec["others"].append({"pid": pid, "name": st[0], "ticks": d})
            rec["reasons"].append(f"process {st[0]}[{pid}] used {d} ticks (over {other_limit})")
    rec["forks_unattributed"] = rec["forks_rise"] - spawns - rec["new_kthreads"]
    rec["disturbed"] = bool(rec["reasons"])
    return rec


def idle_probe(sample_s=None, me=None):
    """rule-m1 item 6 (carried over): over a sample of 2 s, no process outside the runner used more than 5% of one
    core, and core 1 was busy at most 2%. Returns (idle, detail)."""
    sample_s = C.IDLE_SAMPLE_S if sample_s is None else sample_s
    me = os.getpid() if me is None else me
    hz = os.sysconf("SC_CLK_TCK")
    a = {pid: (st[0], st[2] + st[3]) for pid, st in snapshot()["procs"].items() if pid != me}
    c0 = cpu_times(C.TIMED_CORE)
    time.sleep(sample_s)
    b = {pid: (st[0], st[2] + st[3]) for pid, st in snapshot()["procs"].items() if pid != me}
    c1 = cpu_times(C.TIMED_CORE)
    busy = []
    for pid, (name, t1) in b.items():
        t0 = a.get(pid, (name, t1))[1]
        share = (t1 - t0) / hz / sample_s
        if share > C.IDLE_PROC_CPU_MAX:
            busy.append({"pid": pid, "name": name, "cpu": round(share, 3)})
    core1 = (c1[0] - c0[0]) / max(c1[1] - c0[1], 1)
    idle = not busy and core1 <= C.IDLE_CORE1_MAX
    return idle, {"busy": busy, "core1": round(core1, 4), "load1": round(os.getloadavg()[0], 2)}


# ------------------------------------------------------------------------------------------------------- spawning
class Spawner:
    """rule-m1 item 5 (carried over): fork, then in the child pin to the core with sched_setaffinity, redirect stdout
    and stderr to the output file with dup2, and exec (exit 127 if exec fails); the parent waits with one blocking
    waitpid. Round 3's, unchanged."""

    def __init__(self):
        self.child = None
        self.spawns = 0
        self.last_launch_s = None

    def run(self, argv, out_path, core):
        argv = [str(x) for x in argv]
        t0 = time.monotonic()
        pid = os.fork()
        if pid == 0:
            try:
                os.sched_setaffinity(0, {core})
                fd = os.open(str(out_path), os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o644)
                os.dup2(fd, 1)
                os.dup2(fd, 2)
                os.execv(argv[0], argv)
            finally:
                os._exit(127)
        self.child = pid
        self.spawns += 1
        _, status = os.waitpid(pid, 0)
        self.last_launch_s = time.monotonic() - t0
        self.child = None
        return os.waitstatus_to_exitcode(status)


def addition_finished(folder=None):
    """None, or why the addition run's folder does not hold a finished addition run (measured, not toy, not stopped):
    the multiplication run comes after it (spec v0.1 section 0 item 13). Whether the author has seen its results is
    the operator's to confirm; the harness checks only that they exist."""
    p = Path(folder or C.RESULTS["add"]) / "state.json"
    try:
        st = json.loads(p.read_text())
    except (OSError, ValueError):
        return f"no addition run's state in {p.parent}"
    if st.get("op") != "add" or st.get("toy"):
        return f"{p} is not a measured addition run"
    if not st.get("finished") or st.get("run_stopped") or st.get("hard_stopped"):
        return f"the addition run in {p.parent} did not finish"
    return None


# ------------------------------------------------------------------------------------------------------- the runner
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
        self.jobs = list(C.JOBS) if not a.jobs else [int(x) for x in a.jobs.split(",")]
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
        self.add_hashes = None

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

    def ref(self, job):
        if job not in self.refs:
            self.refs[job] = (self.wl / f"job-{job:02d}.ref").read_bytes()
        return self.refs[job]

    def pred(self, job):
        return json.loads((self.wl / f"job-{job:02d}.m1.json").read_text())["predicted"]

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
        """Untimed work two at a time, one per core (rule-m1 item 3; Run plan steps 4-5). tasks: [(key, fn(core))].
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
        """Before the clock (rule-m1, Run plan step 0): the workloads, the fractions references and the model's
        predictions, untimed and outside the hard stop, and each file's sha256."""
        self.log("step 0 (before the clock): workloads, exact references and the model's predictions")
        t = time.monotonic()
        cmd = [sys.executable, str(C.HERE / "m1_gen.py"), str(self.wl), "--op", self.op,
               "--jobs", ",".join(map(str, self.jobs))] + (["--toy"] if self.toy else [])
        p = subprocess.run(cmd, capture_output=True, text=True)
        if p.returncode:
            raise RunStop(f"step 0: workload generation failed: {p.stdout[-2000:]}{p.stderr[-2000:]}")
        gen = json.loads((self.wl / "gen.json").read_text())
        self.state["gen"] = gen
        self.state["step0_s"] = round(time.monotonic() - t, 1)
        self.log(f"step 0 done in {self.state['step0_s']} s: " + ", ".join(m["name"] for m in gen.values()))
        self.save()

    # --------------------------------------------------------------------------------------- step 1
    def build(self):
        self.bin.mkdir(parents=True, exist_ok=True)
        for out, cmd in L.build_commands(self.bin):
            out.parent.mkdir(parents=True, exist_ok=True)
            p, _ = self.run_untimed(cmd, C.HARNESS_CORE)
            if p.returncode:
                raise RunStop(f"build failed: {' '.join(cmd)}\n{p.stderr}")

    def step1(self):
        self.log("step 1: build, hashes, tcm.c's diff, step 0's files")
        self.build()
        for check, what in ((L.vendor08_check, "the 08 module"), (L.tc3_check, "tc3.c or its headers"),
                            (L.tcm_diff_check, "tcm.c's diff")):
            bad = check()
            if bad:
                raise RunStop(f"{what} changed: {bad}")
        (self.out / "tcm.diff").write_text(L.tcm_diff_text())    # the recorded diff (rule-m1 item 2)
        bad = []
        for j, m in self.state["gen"].items():
            for name, want in m["sha256"].items():
                if C.sha256(self.wl / name) != want:
                    bad.append(name)
        if bad:
            raise RunStop(f"step 0's files changed since they were written: {bad}")
        self.state["binaries"] = self.code
        if self.op == "mul" and not self.toy:                  # the multiplication run checks the addition run's binaries
            try:
                add = json.loads((C.RESULTS["add"] / "state.json").read_text())
                same = add.get("binaries") == self.code
                self.state["binaries_vs_addition"] = "the addition run's" if same else \
                    "DIFFERENT from the addition run's (recorded: a revision of the code, rule-m1 item 12)"
            except OSError:
                self.state["binaries_vs_addition"] = "the addition run's state is missing: not compared"
            self.log(f"binaries: {self.state['binaries_vs_addition']}")
        self.log("08 module, tc3.c and tcm.c's diff as pinned; step 0's files unchanged")
        self.save()

    # --------------------------------------------------------------------------------------- step 2
    def unit_tests(self):
        self.log("step 2: unit tests, the differential test, the allocation check, and the tests under the sanitizers")
        res = {}
        for name, cmd, env in L.unit_commands(self.bin):
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
        """rule-m1 item 6: before each timed block; re-check every 15 s, refuse after 10 minutes."""
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
        """After the block: the full parse, the exact check of the read-out (rule-m1 item 12), the files removed."""
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
                kept = out if reps == 1 else out[1:]
                best = min(kept, key=lambda x: x[1])         # the rep of minimum total; its parts go with it
                rec.update(r=r, reps=reps, total=best[1] / r, creation=best[2] / r, operation=best[3] / r,
                           read=best[4] / r, tau=best[5] / r, reps_out=out)
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
        """rule-m1 items 4-6: the idle check, then one timed block (calibration and the rounds), then the disturbance
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
                raise RunStop(f"harness bug: {runner} in the {label} block of job {job} (rule-m1 item 5: more than "
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
        raise RunStop(f"job {job}: the {label} block was disturbed twice (rule-m1 item 5)")

    def _block_body(self, job, label, rows):
        recs, cal = [], {}
        for impl in C.IMPLS:                                    # calibration: the six builds; the null row takes
            if impl not in {C.ROW_IMPL[r] for r in rows}:       # Hash-V-fixed's r
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
        self.log(f"BUG: {entry}; the measured run of {impl} stops (rule-m1 item 12)")
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
        # P_6 and P_13: the pilot blocks of jobs 6 and 13 (in a toy run without them: the longest pilot block)
        longest = max(walls.values())
        p_small = walls.get(C.PILOT_P_SMALL, longest)
        p_large = walls.get(C.PILOT_P_LARGE, longest)
        n_large = sum(1 for j in self.jobs if j >= 13)
        n_rest = len(self.jobs) - n_large
        elapsed = self.elapsed()
        proj = K.pilot_projection(elapsed, p_small, p_large, n_large, n_rest)
        self.state["pilot"] = {"jobs": jobs, "walls_s": {str(j): w for j, w in walls.items()}, "P_6_s": p_small,
                               "P_13_s": p_large, "N_128": n_large, "N_rest": n_rest, "elapsed_s": elapsed,
                               "projected_s": proj, "limit_s": C.PILOT_LIMIT_S, "host_rates": rates,
                               "host_rates_measured": measured,
                               "discarded": "the pilot's results are discarded (rule-m1, Run plan step 3)"}
        self.save()
        self.log(f"pilot: P_6 {p_small:.1f} s, P_13 {p_large:.1f} s; projection {proj / 60:.1f} min (limit "
                 f"{C.PILOT_LIMIT_S / 60:g}); host rates {rates}")
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
        """spec §10 items 1-2 and 7 on one row and job (test build): the read-out, the audits, every exact figure
        against the model; for TC also the TC gate: tc3.c itself on the same operations, its per-operation record
        equal to tcm.c's (rule-m1 item 3)."""
        ro = self.tmp / f"g-{job:02d}-{impl}.out"
        pc = self.tmp / f"gp-{job:02d}-{impl}.bin"
        dp = self.tmp / f"gd-{job:02d}-{impl}.bin"
        p, wall = self.run_untimed([self.exe(impl, "test"), "gate", self.wlpath(job), ro, pc, dp], core)
        res = {"impl": impl, "job": job, "wall": round(wall, 2), "rc": p.returncode, "code": self.code[impl]}
        lines = p.stdout.splitlines()
        err = next((l for l in lines if l.startswith(("error ", "auditfail "))), None)
        if p.returncode or err:
            res["fail"] = err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"
            return res
        bad = K.readout_check(ro, self.ref(job))
        if bad:
            res["fail"] = f"read-out: {bad}"
            return res
        dump = C.read_dump(dp) if dp.exists() else None
        fails = K.exact_checks(impl, lines, self.pred(job), dump)
        res["percall_sha256"] = C.sha256(pc)
        if dump:
            res["dump_sha256"] = {k: C.hashlib.sha256(v).hexdigest() for k, v in dump.items()}
        if impl == "TC":                                         # the TC gate (spec §4)
            ro3 = self.tmp / f"g3-{job:02d}.out"
            pc3 = self.tmp / f"gp3-{job:02d}.bin"
            q, w3 = self.run_untimed([self.bin / "tc3gate", self.wlpath(job), ro3, pc3], core)
            res["tc3gate"] = {"rc": q.returncode, "wall": round(w3, 2), "out": q.stdout.strip()[-200:]}
            if q.returncode:
                fails.append(f"TC gate: tc3gate exit {q.returncode}: {q.stdout[-200:]} {q.stderr[-200:]}")
            else:
                b3 = K.readout_check(ro3, self.ref(job))
                if b3:
                    fails.append(f"TC gate: tc3.c's read-out {b3}")
                same = C.sha256(pc3) == res["percall_sha256"]
                res["tc3gate"]["percall"] = "equal" if same else "DIFFERENT"
                if not same:
                    fails.append("TC gate: tcm.c's cases and gcd counts differ from tc3.c's, operation for operation")
            for f in (ro3, pc3):
                if f.exists():
                    f.unlink()
        if fails:
            res["fail"] = "exact checks (spec §10): " + "; ".join(fails[:10])
        for f in (ro, pc, dp):
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
        # map against lookup (spec §10 item 6): the same tables and cells, from the gates' dumps
        for j in self.jobs:
            a, b = gates.get(f"MHVGM:{j}", {}), gates.get(f"MHVG:{j}", {})
            if "dump_sha256" in a and "dump_sha256" in b and a["dump_sha256"] != b["dump_sha256"]:
                gates[f"MHVGM:{j}"]["fail"] = "map against lookup: the tables or cells differ (spec §10 item 6)"
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
        fails = K.exact_checks(impl, lines, self.pred(job))
        if fails:
            res["bug"] = {"what": "exact checks: " + "; ".join(fails[:5])}
        res["counts"] = [l for l in lines if l.startswith(("ctr ", "xctr "))]
        res["mem"] = [l for l in lines if l.startswith(("mem ", "memtau ", "memN "))]
        res["stor"] = K.parse_stor(lines)
        res["opc"] = K.parse_opc(lines)
        res["growths"] = K.parse_growths(lines)
        res["percall_sha256"] = C.sha256(pc)
        res["G"] = K.gcd_work(lines)
        for f in (ro, pc):
            f.unlink()
        return res

    def mem(self, impl, job, core, rep):
        ro = self.tmp / f"m-{job:02d}-{impl}-{rep}.out"
        p, wall = self.run_untimed([self.exe(impl, "timing"), "mem", self.wlpath(job), ro], core, limit=C.COUNT_LIMIT_S)
        res = {"impl": impl, "job": job, "rep": rep, "wall": round(wall, 2), "code": self.code[impl]}
        if p is None:
            res["na"] = f"did not finish within the {C.COUNT_LIMIT_S // 60}-minute limit"
            return res
        if p.returncode:
            res["bug"] = {"what": f"exit {p.returncode}: {p.stdout[-300:]} {p.stderr.strip()[:300]}"}
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
                keys = ("counts", "mem", "stor", "opc", "growths", "percall_sha256") if kind == "count" else ("memrun",)
                same = all(r.get(k) == first.get(k) for k in keys)
                if kind == "mem" and not same:                  # the heap figures, every field but none
                    same = r.get("memrun") == first.get("memrun")
                if "bug" not in r and "na" not in r and not same:
                    r["bug"] = {"what": f"{kind} run repeated: figures differ (rule-m1 item 2)"}
                dst[key]["repeat"] = "identical" if "bug" not in r else r["bug"]["what"]
            if "bug" in r:
                self.bug(impl, job, f"{kind} run" + (" (repeat)" if rep else ""), r["bug"])
        self.call_checks(counts)
        self.save()

    def call_checks(self, counts):
        """spec §10 items 3-6 on the counting runs: every hash row's per-operation record (case and core counts) equal
        to TC's, and G(row) / G(TC) = 1; V against M within a sizing, grow against fixed within V and M: the same
        cases, core counts and store splits (found, new, W); map against lookup: the same store figures but the rewrite
        probes, and the same growths."""
        checks = self.state.setdefault("call_checks", {})
        for job in self.jobs:
            c = {i: counts.get(f"{i}:{job}", {}) for i in C.IMPLS}
            if any("percall_sha256" not in v for v in c.values()):
                continue
            t = c["TC"]
            rec, bad = {"G_TC": t["G"]}, []
            for i in C.HASH_IMPLS:
                same = c[i]["percall_sha256"] == t["percall_sha256"]
                g_ok = c[i]["G"] == t["G"]
                rec[i] = {"call_for_call": "pass" if same else "FAIL", "G": c[i]["G"],
                          "G_ratio": c[i]["G"] / t["G"] if t["G"] else None, "gcd_work_check": "passed" if g_ok else
                          "FAILED"}
                if not same or not g_ok:
                    bad.append(f"{C.IMPL_ROW[i]} against TC")

            def split(i):
                lines = c[i]["counts"]
                return K.arith_counts(lines), {f"{w}:{r}": {k: c[i]["stor"][w][r][k] for k in ("W", "found", "new")}
                                               for w in c[i]["stor"] for r in c[i]["stor"][w]}
            for a, b, what in (("MHMF", "MHVF", "M against V (fixed)"), ("MHMG", "MHVG", "M against V (grow)"),
                               ("MHVG", "MHVF", "grow against fixed (V)"), ("MHMG", "MHMF", "grow against fixed (M)")):
                ok = split(a) == split(b)
                rec[what] = "pass" if ok else "FAIL"
                if not ok:
                    bad.append(what)
            sm, sl = c["MHVGM"]["stor"], c["MHVG"]["stor"]
            strip = lambda s: {w: {r: {k: v for k, v in s[w][r].items() if k != "wprobes"} for r in s[w]} for w in s}
            gm = [{k: v for k, v in g.items() if k not in ("wprobes", "bytes_map")} for g in c["MHVGM"]["growths"]]
            gl = [{k: v for k, v in g.items() if k not in ("wprobes", "bytes_map")} for g in c["MHVG"]["growths"]]
            ok = strip(sm) == strip(sl) and gm == gl and K.arith_counts(c["MHVGM"]["counts"]) == \
                K.arith_counts(c["MHVG"]["counts"])
            rec["map against lookup"] = "pass" if ok else "FAIL"
            if not ok:
                bad.append("map against lookup")
            rec["result"] = "pass" if not bad else "FAIL: " + ", ".join(bad)
            checks[str(job)] = rec
            if bad:
                for impl in C.IMPLS:
                    self.bug(impl, job, "counting run, call for call",
                             {"what": f"{', '.join(bad)}: per-operation arithmetic, cases or stores differ "
                                      "(spec §10 items 3-6)"})

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
        head = [f"# Design 3 on matrix {'addition' if self.op == 'add' else 'multiplication'}: results", "",
                f"Run started {stamp()}." + (" **TOY RUN: a harness test, not a measurement.**" if self.toy else ""),
                "", "## The measurement rule (research/rule-m1.md, verbatim, written before the first trial)", "",
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
        for check, what in ((L.vendor08_check, "the 08 module changed (rule-m1 item 2)"),
                            (L.tc3_check, "tc3.c or its headers changed (spec §4, rule-m1 item 2)"),
                            (L.tcm_diff_check, "tcm.c's diff from tc3.c is not the recorded one (rule-m1 item 2)")):
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
                raise SystemExit("the rule needs 2 cores (rule-m1 item 4)")
            if self.a.jobs:
                raise SystemExit("a measured run runs every job of its set")
            if self.op == "mul":
                bad = addition_finished()
                if bad:
                    raise SystemExit(f"refusing the multiplication run: {bad} (spec v0.1 section 0 item 13: the "
                                     "addition run first; DECISIONS M1-H4)")
                self.add_hashes = C.folder_hashes(C.RESULTS["add"])
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
                          "operator": {"timeout_min_at_least": C.OPERATOR_TIMEOUT_MIN,
                                       "instruction": "start once; run nothing else until the exit notice "
                                                      "(rule-m1 item 5)"}}
            if self.add_hashes is not None:
                self.state["addition_results_before"] = self.add_hashes
            self.write_header()
            self.step0()                                     # before the clock, outside the hard stop
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
            if self.add_hashes is not None:
                after = C.folder_hashes(C.RESULTS["add"])
                self.state["addition_results_after"] = "unchanged" if after == self.add_hashes else after
                if after != self.add_hashes:
                    self.log("WARNING: the addition run's results changed during the multiplication run")
            self.save()
            import m1_analyze                              # step 7, after the last timed block
            m1_analyze.report(self.out)
            self.log(f"results: {self.out / 'results.md'}")

    def rest(self):
        self.step4_gates()
        self.step5_counts()
        assert threading.active_count() == 1, "the worker pool must be shut down before step 6 (rule-m1 item 5)"
        self.step6_rounds()

    def do_rerun(self, which):
        """rule-m1 item 12: the rows re-run in full after a fix under review; their earlier results are dropped."""
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
    ap.add_argument("--op", choices=C.OPS, required=True, help="the addition run, or the multiplication run")
    ap.add_argument("--toy", action="store_true", help="toy workloads in test/build/m1toy/ (harness tests only)")
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
