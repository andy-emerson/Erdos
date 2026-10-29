"""The runner of research/rule-r2.md (round 2: Q32 and TC, spec v0.2).

THE OPERATOR (rule-r2 item 5) starts it ONCE, with one command that returns only when it exits:
    python3 test/r2_runner.py
either in the foreground, or as a background command that notifies the operator when it exits (for Claude Code: the
Bash tool with run_in_background and a timeout of at least 35 minutes). Until that exit notice the operator runs
NOTHING else on the machine: no polling or waiting loop, no sleep, watch, tail -f, file check, Monitor or until-loop,
no other tool call, build, test or edit, and no detached process.

    python3 r2_runner.py                 the measured run: results in test/results-r2/ (refuses unless rule-r2.md is the
                                         fixed rule, the 08 module is unchanged, round 1's official data is unchanged,
                                         and the machine has 2 cores)
    python3 r2_runner.py --pilot-only    steps 1-3, then stop
    python3 r2_runner.py --resume        continue a run stopped before step 6 (a failed gate, or --pilot-only): re-runs
                                         the unit tests and every gate not passed, then goes on
    python3 r2_runner.py --rerun Q32     rule-r2 item 12: after a bug is fixed under review, re-run that row in full
                                         (gates, counting and memory runs, calibration and every round, its profile);
                                         Q32 re-runs Q32 and Q32-null
    python3 r2_runner.py --toy [--jobs 1,3] [--hard-stop-s S]
                                         a harness test with toy workloads in test/build/r2toy/: never a measurement

The run plan (rule-r2, Run plan): 1 build, workloads, references and predictions; 2 unit tests (with the sanitizers);
3 the pilot (jobs 1, 3, 10: calibration and 2 rounds, then a profiling run on job 1), with its projection and stops;
4 gates; 5 counting and memory runs (two at a time; the worker pool is then shut down and joined); 6 per job: the idle
check, calibration and 12 rounds, the disturbance check; 7 per job: the idle check, the profile runs, the disturbance
check; 8 the analysis.

During timed phases (rule-r2 item 5) the runner is single-threaded; it starts each timed invocation by fork and exec
(pinned to core 1 before exec, its output redirected to a file), waits with one blocking waitpid, writes nothing between
the start and the end of an invocation, and enforces the 30-minute hard stop with a kernel timer (setitimer). Before
and after each timed block it snapshots every process's ticks and the kernel's fork counter; a disturbed block is
re-run once. It never imports numpy (test/DECISIONS.md R2-H3).
"""
import argparse
import concurrent.futures as cf
import datetime
import json
import math
import os
import queue
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path

import r2_checks as K
import r2_common as C
import r2_libconf as L


class HardStop(Exception):
    pass


class RunStop(Exception):
    """The run stops and is reported to the author."""


def stamp():
    return datetime.datetime.now().astimezone().isoformat(timespec="seconds")


# ------------------------------------------------------------------------------------------------------- /proc
def read_stat(pid):
    """(name, ppid, utime, stime, cutime, cstime, starttime) of a process from /proc/<pid>/stat (fields 4, 14-17 and
    22), or None."""
    try:
        raw = Path(f"/proc/{pid}/stat").read_text()
    except OSError:
        return None
    name = raw[raw.find("(") + 1:raw.rfind(")")]
    f = raw[raw.rfind(")") + 2:].split()
    return (name, int(f[1]), int(f[11]), int(f[12]), int(f[13]), int(f[14]), int(f[19]))


def fork_counter():
    """The kernel's fork counter: the "processes" line of /proc/stat (every process and thread created since boot)."""
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
    """The runner's ancestors, following PPid to process 1: [(pid, name)] (the host processes, rule-r2 item 5)."""
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


def disturbance(s0, s1, runner, spawns, hosts, allowance=None):
    """rule-r2 item 5, the check after a timed block, from its two snapshots. runner: the runner's pid; spawns: the
    timed invocations it started in the block; hosts: the host processes' pids; allowance: {pid: ticks} after the
    pilot, None in the pilot (host ticks are recorded and do not flag). Returns the record, with "disturbed" and the
    reasons."""
    L_s = max(s1["t"] - s0["t"], 1e-9)
    p0, p1 = s0["procs"], s1["procs"]
    rec = {"seconds": round(L_s, 3), "runner_ticks": None, "kthreads": {}, "hosts": {}, "others": [], "new": [],
           "forks_rise": s1["forks"] - s0["forks"], "spawns": spawns, "new_kthreads": 0, "reasons": []}
    if runner in p0 and runner in p1:                       # the runner: its own ticks only (its children are timed)
        rec["runner_ticks"] = (p1[runner][2] + p1[runner][3]) - (p0[runner][2] + p0[runner][3])
    for pid, st in p1.items():
        if pid == runner:
            continue
        before = p0.get(pid)
        if before is not None and before[6] != st[6]:
            before = None                                   # a reused pid (another start time): a new process
        d = ticks4(st) - (ticks4(before) if before else 0)
        if is_kthread(pid, st):
            if before is None:
                rec["new_kthreads"] += 1
            if d:
                rec["kthreads"][str(pid)] = {"name": st[0], "ticks": d}
            continue
        if pid in hosts:
            rec["hosts"][str(pid)] = {"name": st[0], "ticks": d, "rate": d / L_s}
            if allowance is not None and d > allowance.get(pid, C.HOST_BASE_TICKS):
                rec["reasons"].append(f"host process {st[0]}[{pid}] used {d} ticks, over its allowance "
                                      f"{allowance.get(pid, C.HOST_BASE_TICKS)}")
            continue
        if before is None:
            if d >= C.DIST_NEW_TICKS:
                rec["new"].append({"pid": pid, "name": st[0], "ticks": d})
                rec["reasons"].append(f"new process {st[0]}[{pid}] used {d} ticks")
        elif d > C.DIST_OTHER_TICKS:
            rec["others"].append({"pid": pid, "name": st[0], "ticks": d})
            rec["reasons"].append(f"process {st[0]}[{pid}] used {d} ticks (over {C.DIST_OTHER_TICKS})")
    rec["unexplained_forks"] = rec["forks_rise"] - spawns - rec["new_kthreads"]
    if rec["unexplained_forks"] > C.DIST_FORKS:
        rec["reasons"].append(f"{rec['unexplained_forks']} unexplained forks")
    rec["disturbed"] = bool(rec["reasons"])
    return rec


def idle_probe(sample_s=None, me=None):
    """rule-r2 item 6: over a sample of 2 s, no process outside the runner used more than 5% of one core, and core 1
    was busy at most 2%. Returns (idle, detail); the load average is recorded and does not gate."""
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
    """rule-r2 item 5.2-5.3: fork, then in the child pin to the core with sched_setaffinity, redirect stdout and stderr
    to the output file with dup2, and exec (exit 127 if exec fails); the parent waits with one blocking waitpid."""

    def __init__(self):
        self.child = None
        self.spawns = 0

    def run(self, argv, out_path, core):
        argv = [str(x) for x in argv]
        pid = os.fork()
        if pid == 0:                                         # the child: no Python code of the runner after exec
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
        _, status = os.waitpid(pid, 0)                       # blocking, no timeout, no polling
        self.child = None
        return os.waitstatus_to_exitcode(status)


# ------------------------------------------------------------------------------------------------------- the runner
class Runner:
    def __init__(self, a):
        self.a = a
        self.toy = a.toy
        base = C.HERE / "build" / ("r2toy" if a.toy else "r2")
        self.out = (base / "results") if a.toy else C.RESULTS
        self.wl = base / "work"
        self.bin = base / "bin"
        self.tmp = base / "inv"
        self.jobs = list(C.RUN_JOBS) if not a.jobs else [int(x) for x in a.jobs.split(",")]
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
        self.pending_log = []
        self.seq = 0
        self.refs = {}

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
        """rule-r2 item 5.4: setitimer(ITIMER_REAL) to the time left before the hard stop; its handler kills the child
        and ends the run. It runs no code until it fires."""
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
        return json.loads((self.wl / f"job-{job:02d}.r2.json").read_text())["predicted"]

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
        """Untimed work two at a time, one per core (rule-r2 item 3; Run plan steps 4-5). tasks: [(key, fn(core))].
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

    # --------------------------------------------------------------------------------------- step 1
    def build(self):
        self.bin.mkdir(parents=True, exist_ok=True)
        for out, cmd in L.build_commands(self.bin):
            out.parent.mkdir(parents=True, exist_ok=True)
            p, _ = self.run_untimed(cmd, C.HARNESS_CORE)
            if p.returncode:
                raise RunStop(f"build failed: {' '.join(cmd)}\n{p.stderr}")

    def step1(self):
        self.log("step 1: build, workloads, exact references and predicted figures")
        self.build()
        jobs = sorted(set(self.jobs) | (set() if self.toy else set(C.OVER_CAPACITY_JOBS)))
        cmd = [sys.executable, str(C.HERE / "r2_gen.py"), str(self.wl), "--jobs", ",".join(map(str, jobs))]
        if self.toy:
            cmd.append("--toy")
        p, _ = self.run_untimed(cmd, C.HARNESS_CORE)
        if p.returncode:
            raise RunStop(f"workload generation failed: {p.stdout[-2000:]}{p.stderr[-2000:]}")
        gen = json.loads((self.wl / "gen.json").read_text())
        self.state["gen"] = gen
        if not self.toy:
            bad = [j for j, m in gen.items() if not m.get("identical_to_round1")]
            if bad:
                raise RunStop(f"workloads differ from round 1's (rule-r2, Workloads): jobs {bad}")
        over = [int(j) for j, m in gen.items() if m.get("over_capacity")]
        if any(j in self.jobs for j in over):
            raise RunStop(f"jobs over Q32's capacity in the run plan: {over}")
        self.state["over_capacity"] = {j: gen[j]["W_end_round1_model"] for j in gen if gen[j].get("over_capacity")}
        self.log("workloads byte-identical to round 1's" if not self.toy else "toy workloads")
        self.save()

    # --------------------------------------------------------------------------------------- step 2
    def unit_tests(self):
        self.log("step 2: unit tests, the differential test, and both under the sanitizers (test build)")
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
        """rule-r2 item 6: before each timed block; re-check every 15 s, refuse after 10 minutes."""
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
        build = "profiling" if kind == "prof" else "timing"
        t = time.monotonic()
        rc = self.spawner.run([self.exe(impl, build), "calib" if kind == "calib" else kind, self.wlpath(job), ro] +
                              args, out, C.TIMED_CORE)
        rec = {"job": job, "row": row, "kind": kind, "args": [str(x) for x in args], "rc": rc, "out": str(out),
               "readout": str(ro), "wall": round(time.monotonic() - t, 4), "block": block}
        return rec

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
        """After the block: the full parse, the exact check of the read-out (rule-r2 item 12), the files removed."""
        lines = Path(rec["out"]).read_text().splitlines() if Path(rec["out"]).exists() else []
        err = next((l for l in lines if l.startswith(("error ", "auditfail "))), None)
        if rec["rc"] or err:
            rec["bug"] = {"what": err or f"exit {rec['rc']}: {' '.join(lines)[-300:]}"}
        else:
            bad = K.readout_check(rec["readout"], self.ref(rec["job"]))
            if bad:
                rec["bug"] = bad
        if "bug" not in rec:
            if rec["kind"] == "time" and not rec.get("overcap"):
                r, reps = int(rec["args"][0]), int(rec["args"][1])
                out = K.parse_reps(lines, ("rep",))
                kept = out if reps == 1 else out[1:]
                best = min(kept, key=lambda x: x[1])         # the rep of minimum total; its parts go with it
                rec.update(r=r, reps=reps, total=best[1] / r, entry=best[2] / r, ops=best[3] / r, read=best[4] / r,
                           reps_out=out)
            elif rec["kind"] == "prof":
                r = int(rec["args"][0])
                p = K.parse_prof(lines)
                reps = sorted(p["reps"])
                kept = min(reps[1:] or reps, key=lambda i: p["reps"][i]["total"])
                rp = p["reps"][kept]
                rec.update(r=r, probe_cost=p.get("probe_cost"), tsc_hz=p.get("tsc_hz"), K=p.get("K"), kept_rep=kept,
                           total=rp["total"] / r, entry=rp["entry"] / r, ops=rp["ops"] / r, read=rp["read"] / r,
                           profile=K.pack_prof(rp), rep_totals=[p["reps"][i]["total"] / r for i in reps])
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
    def timed_block(self, job, kind, label, pilot=False, rows=None):
        """rule-r2 items 4-6: the idle check, then one timed block (kind "rounds": calibration and the rounds; kind
        "prof": the profile runs), then the disturbance check. A disturbed block is discarded and re-run once; a
        second disturbance stops the run. Returns the block's records."""
        rows = rows or list(C.ROWS)
        for attempt in (1, 2):
            where = f"{label} block of job {job}" + (" (re-run)" if attempt == 2 else "")
            idle = self.wait_idle(where)
            self.save()
            assert threading.active_count() == 1, "the runner must be single-threaded during timed phases"
            self.tmp.mkdir(parents=True, exist_ok=True)
            spawns0 = self.spawner.spawns
            s0 = snapshot()
            recs = self._block_body(job, kind, label, rows)       # nothing is written in here
            s1 = snapshot()
            d = disturbance(s0, s1, os.getpid(), self.spawner.spawns - spawns0, set(self.hosts),
                            None if pilot else self.allowance_for(s1["t"] - s0["t"]))
            d.update(job=job, kind=kind, label=label, attempt=attempt, time=stamp(), idle_check=idle["time"])
            for rec in recs:
                self.finish(rec)
            self.state.setdefault("disturbance_checks", []).append(d)
            if d["runner_ticks"] is not None and d["runner_ticks"] > C.RUNNER_TICKS_MAX:
                self.state.setdefault("harness_bugs", []).append(
                    {"what": f"the runner used {d['runner_ticks']} ticks in a timed block (more than "
                             f"{C.RUNNER_TICKS_MAX})", "job": job, "label": label, "time": stamp()})
                self.save()
                raise RunStop(f"harness bug: the runner used {d['runner_ticks']} ticks in the {label} block of job "
                              f"{job} (rule-r2 item 5: more than {C.RUNNER_TICKS_MAX} is a harness bug)")
            if not d["disturbed"]:
                self.log(f"job {job} {label}: block of {d['seconds']:.1f} s undisturbed (runner {d['runner_ticks']} "
                         f"ticks, {d['spawns']} spawns, forks {d['forks_rise']})")
                self.save()
                return recs
            self.log(f"job {job} {label}: block DISTURBED ({'; '.join(d['reasons'])}); discarded")
            self.state.setdefault("discarded_blocks", []).append({"job": job, "label": label, "records": recs})
            self.save()
        raise RunStop(f"job {job}: the {label} block was disturbed twice (rule-r2 item 5)")

    def _block_body(self, job, kind, label, rows):
        recs = []
        if kind == "rounds":
            cal = {}
            for impl in C.IMPLS:                                # calibration: Q32 and TC; Q32-null takes Q32's r
                if impl not in {C.ROW_IMPL[r] for r in rows}:
                    continue
                rec = self.quick_parse(self.invoke(job, impl, "calib", [], label))
                rec["role"] = "calibration"
                recs.append(rec)
                cal[impl] = rec
            capped = {r for r in rows if cal.get(C.ROW_IMPL[r], {}).get("overcap") or "r" not in cal.get(C.ROW_IMPL[r], {})}
            nr = C.PILOT_ROUNDS if label == "pilot" else C.ROUNDS
            order = [r for r in C.ROWS if r in rows]
            for k in range(nr):
                rot = order[k % len(order):] + order[:k % len(order)]
                for pos, row in enumerate(rot):
                    if row in capped:
                        continue
                    c = cal[C.ROW_IMPL[row]]
                    rec = self.quick_parse(self.invoke(job, row, "time", [c["r"], c["reps"]], label))
                    rec.update(role="round", round=k + 1, pos=pos)
                    recs.append(rec)
                    if rec["overcap"]:
                        capped.add(row)
        else:                                                  # the profile: Q32, TC, Q32, TC, ...
            n = 1 if label == "pilot" else C.PROF_INVOCATIONS
            for k in range(n):
                for row in C.PROF_ROWS:
                    if row not in rows:
                        continue
                    r = self.prof_r(job, row, label)
                    if r is None:
                        continue
                    rec = self.invoke(job, row, "prof", [r, 3, C.PROF_K, C.PROF_SEED], label)
                    rec.update(role="profile", invocation=k + 1)
                    recs.append(rec)
        return recs

    def prof_r(self, job, row, label):
        cal = self.state.get("pilot_calibration" if label == "pilot" else "calibration", {}).get(f"{job}:{row}")
        return None if not cal or cal.get("overcap") or "r" not in cal else cal["r"]

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
        self.log(f"BUG: {entry}; the measured run of {impl} stops (rule-r2 item 12)")
        self.save()

    def live_rows(self):
        stopped = set(self.state.get("stopped_impls", []))
        only = self.state.get("rerun_only")
        return [r for r in C.ROWS if C.ROW_IMPL[r] not in stopped and (not only or C.ROW_IMPL[r] in only)]

    # --------------------------------------------------------------------------------------- step 3: the pilot
    def step3_pilot(self):
        self.log("step 3: pilot")
        jobs = [j for j in C.PILOT_JOBS if j in self.jobs] or self.jobs[:1]
        t = time.monotonic()
        pcal = self.state.setdefault("pilot_calibration", {})
        blocks = []
        for job in jobs:
            recs = self.timed_block(job, "rounds", "pilot", pilot=True)
            for rec in recs:
                if "bug" in rec:
                    self.bug(C.ROW_IMPL[rec["row"]], job, "pilot", rec["bug"])
                if rec.get("role") == "calibration":
                    pcal[f"{job}:{rec['row']}"] = {k: rec.get(k) for k in ("r", "reps", "rep_ns", "overcap")}
                    if rec["row"] == "Q32":
                        pcal[f"{job}:Q32-null"] = dict(pcal[f"{job}:Q32"], shared_from="Q32")
            self.state.setdefault("pilot_records", []).extend(recs)
            blocks.append(job)
        P = time.monotonic() - t
        pj = C.PILOT_PROF_JOB if C.PILOT_PROF_JOB in jobs else jobs[0]
        t = time.monotonic()
        precs = self.timed_block(pj, "prof", "pilot", pilot=True)
        P_prof = time.monotonic() - t                         # with its idle check: a more conservative stop (R2-H8a)
        for rec in precs:
            if "bug" in rec:
                self.bug(C.ROW_IMPL[rec["row"]], pj, "pilot profiling run", rec["bug"])
        self.state["pilot_records"].extend(precs)
        # the host rates R_h: the highest rate over the pilot's blocks
        rates = {}
        for d in self.state.get("disturbance_checks", []):
            if d.get("label") == "pilot":
                for pid, h in d["hosts"].items():
                    rates[pid] = max(rates.get(pid, 0.0), h["rate"])
        self.state["host_rates"] = rates
        # the profiling run's slowdown, against the pilot rounds' median total
        slow = {}
        for rec in precs:
            tot = sorted(r["total"] for r in self.state["pilot_records"]
                         if r.get("role") == "round" and r["job"] == pj and r["row"] == rec["row"] and "total" in r)
            if tot and "total" in rec:
                med = tot[len(tot) // 2] if len(tot) % 2 else (tot[len(tot) // 2 - 1] + tot[len(tot) // 2]) / 2
                s, ok = K.slowdown(rec["total"], med)
                slow[rec["row"]] = {"slowdown": s, "within": ok}
        proj = K.pilot_projection(self.elapsed(), P, P_prof)
        self.state["pilot"] = {"jobs": jobs, "prof_job": pj, "P_s": P, "P_prof_s": P_prof, "elapsed_s": self.elapsed(),
                               "projected_s": proj, "limit_s": C.PILOT_LIMIT_S, "host_rates": rates, "slowdown": slow,
                               "discarded": "the pilot's results are discarded (rule-r2, Run plan step 3)"}
        self.save()
        self.log(f"pilot: P {P:.1f} s, P_prof {P_prof:.1f} s, projection {proj / 60:.1f} min (limit "
                 f"{C.PILOT_LIMIT_S / 60:g}); host rates {rates}; profiling slowdown {slow}")
        reasons = []
        if proj > C.PILOT_LIMIT_S:
            reasons.append(f"projected total {proj / 60:.1f} min exceeds {C.PILOT_LIMIT_S / 60:g} minutes")
        busy = {p: r for p, r in rates.items() if r > C.HOST_RATE_MAX}
        if busy:
            reasons.append(f"host processes over {C.HOST_RATE_MAX} tick per second: {busy}")
        over = {r: s["slowdown"] for r, s in slow.items() if not s["within"]}
        if over:
            reasons.append(f"profiling slowdown over {C.PROF_SLOWDOWN_MAX}: {over}")
        if len(slow) < len([r for r in C.PROF_ROWS if r in self.live_rows()]):
            reasons.append("the pilot's profiling run gave no slowdown for every row")
        if reasons:
            self.state["pilot_stopped"] = reasons
            raise RunStop("the pilot stops the run, reported to the author: " + "; ".join(reasons))

    # --------------------------------------------------------------------------------------- step 4: gates
    def gate(self, impl, job, core):
        ro = self.tmp / f"g-{job:02d}-{impl}.out"
        wf = self.tmp / f"w-{job:02d}-{impl}.bin"
        p, wall = self.run_untimed([self.exe(impl, "test"), "gate", self.wlpath(job), ro, wf], core)
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
        res["audits"] = next((l for l in lines if l.startswith("audits")), "")
        ws = C.sha256(wf) if impl == "Q32" else None
        fails = K.exact_checks(impl, lines, self.pred(job), ws)
        if fails:
            res["fail"] = "exact checks (spec v0.2 §14.1): " + "; ".join(fails[:10])
        for f in (ro, wf):
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
        self.save()
        failed = {k: v["fail"] for k, v in gates.items() if "fail" in v}
        if failed:
            raise RunStop(f"step 4: gates failed (fix the row, then --resume): {failed}")
        self.state["measuring"] = True
        self.log(f"every gate passed ({len(gates)})")
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
        res["counts"] = [l for l in lines if l.startswith(("ctr ", "xctr "))]
        res["mem"] = [l for l in lines if l.startswith(("mem ", "memN "))]
        res["percall_sha256"] = C.sha256(pc)
        res["G"] = K.gcd_work(lines)
        ro.unlink()
        return res

    def mem(self, impl, job, core, rep):
        ro = self.tmp / f"m-{job:02d}-{impl}-{rep}.out"
        p, wall = self.run_untimed([self.exe(impl, "timing"), "mem", self.wlpath(job), ro], core,
                                   limit=C.COUNT_LIMIT_S)
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
        res["memrun"] = {}
        for l in p.stdout.splitlines():
            if l.startswith("memrun "):
                t = l.split()
                res["memrun"][t[1]] = {k: int(v) for k, v in (x.split("=") for x in t[2:])}
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
        res = self.pool(tasks)                                 # shut down and joined on return (item 5.1)
        counts, mems = self.state.setdefault("counts", {}), self.state.setdefault("mems", {})
        for (kind, impl, job, rep), r in sorted(res.items(), key=lambda x: x[0][3]):
            dst = counts if kind == "count" else mems
            key = f"{impl}:{job}"
            if rep == 0:
                dst[key] = r
            else:
                first = res[(kind, impl, job, 0)]
                same = (r.get("counts") == first.get("counts") and r.get("mem") == first.get("mem") and
                        r.get("percall_sha256") == first.get("percall_sha256")) if kind == "count" else \
                    r.get("memrun") == first.get("memrun")
                if "bug" not in r and "na" not in r and not same:
                    r["bug"] = {"what": f"{kind} run repeated: figures differ (rule-r2 item 2)"}
                dst[key]["repeat"] = "identical" if "bug" not in r else r["bug"]["what"]
            if "bug" in r:
                self.bug(impl, job, f"{kind} run" + (" (repeat)" if rep else ""), r["bug"])
        # spec §14.1 item 3: Q32's arithmetic equals TC's, call for call; and so G(Q32) / G(TC) = 1 (rule-r2 item 7)
        checks = self.state.setdefault("call_checks", {})
        for job in self.jobs:
            a, b = counts.get(f"Q32:{job}", {}), counts.get(f"TC:{job}", {})
            if "percall_sha256" not in a or "percall_sha256" not in b:
                continue
            pa, pb = self.tmp / f"p-{job:02d}-Q32-0.bin", self.tmp / f"p-{job:02d}-TC-0.bin"
            same = pa.read_bytes() == pb.read_bytes()
            g_ok = a["G"] == b["G"]
            checks[str(job)] = {"call_for_call": "pass" if same else "FAIL", "G_Q32": a["G"], "G_TC": b["G"],
                                "G_ratio": a["G"] / b["G"] if b["G"] else None, "gcd_work_check": "passed" if g_ok
                                else "FAILED"}
            if not same or not g_ok:
                for impl in C.IMPLS:
                    self.bug(impl, job, "counting run, Q32 against TC call for call",
                             {"what": "per-call arithmetic or gcd work differs between Q32 and TC (spec §14.1 item 3)"})
        self.save()

    # --------------------------------------------------------------------------------------- steps 6-7
    def step6_rounds(self):
        self.log("step 6: calibration and 12 rounds, one timed block per job")
        cal = self.state.setdefault("calibration", {})
        store = self.state.setdefault("records", [])
        for job in self.jobs:
            rows = self.live_rows()
            if not rows:
                break
            recs = self.timed_block(job, "rounds", "measured", rows=rows)
            for rec in recs:
                if rec.get("role") == "calibration":
                    cal[f"{job}:{rec['row']}"] = {k: rec.get(k) for k in ("r", "reps", "rep_ns", "overcap", "bug")}
                    if rec["row"] == "Q32":
                        cal[f"{job}:Q32-null"] = dict(cal[f"{job}:Q32"], shared_from="Q32")
            for rec in recs:
                if "bug" in rec:
                    self.bug(C.ROW_IMPL[rec["row"]], job, f"{rec['role']} of job {job}", rec["bug"])
            stopped = set(self.state.get("stopped_impls", []))
            store.extend(r for r in recs if C.ROW_IMPL[r["row"]] not in stopped)
            self.save()

    def step7_profile(self):
        self.log("step 7: the profile runs, one timed block per job")
        store = self.state.setdefault("profiles", [])
        for job in self.jobs:
            rows = [r for r in self.live_rows() if r in C.PROF_ROWS]
            if not rows:
                break
            recs = self.timed_block(job, "prof", "profile", rows=rows)
            for rec in recs:
                if "bug" in rec:
                    self.bug(C.ROW_IMPL[rec["row"]], job, f"profile run of job {job}", rec["bug"])
                    continue
                tot = sorted(r["total"] for r in self.state["records"]
                             if r.get("role") == "round" and r["job"] == job and r["row"] == rec["row"] and "total" in r)
                if tot:
                    med = tot[len(tot) // 2] if len(tot) % 2 else (tot[len(tot) // 2 - 1] + tot[len(tot) // 2]) / 2
                    rec["slowdown"], rec["within_limit"] = K.slowdown(rec["total"], med)
                    if not rec["within_limit"]:
                        rec["withheld"] = "over the slowdown limit: phase figures withheld (rule-r2 item 10)"
                        rec.pop("profile", None)
                store.append(rec)
            self.save()

    # --------------------------------------------------------------------------------------- results file
    def write_header(self):
        rule = C.RULE_FILE.read_text()
        head = ["# Design 3, round 2: results", "",
                f"Run started {stamp()}." + (" **TOY RUN: a harness test, not a measurement.**" if self.toy else ""),
                "", "## The measurement rule (research/rule-r2.md, verbatim, written before the first trial)", "",
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
        bad = L.vendor08_check()
        if bad:
            raise SystemExit(f"the 08 module changed (spec v0.2 §5): {bad}")
        bad = C.archive_check()
        if bad:
            raise SystemExit(f"round 1's official data changed: {bad}")
        if C.protected(self.out):
            raise SystemExit(f"{self.out} holds round 1's official data: refusing to write there")
        if not self.toy:
            if len(os.sched_getaffinity(0)) < 2 or (os.cpu_count() or 0) < 2:
                raise SystemExit("the rule needs 2 cores (rule-r2 item 4)")
            if self.a.jobs:
                raise SystemExit("a measured run runs every job of the run plan")
        os.sched_setaffinity(0, {C.HARNESS_CORE})

    def start(self):
        if C.protected(self.out):
            raise SystemExit(f"{self.out} holds round 1's official data: refusing to write there")
        self.out.mkdir(parents=True, exist_ok=True)
        self.tmp.mkdir(parents=True, exist_ok=True)
        if self.a.resume or self.a.rerun:
            self.state = json.loads(self.state_file.read_text())
            for k, why in (("hard_stopped", "stopped at the hard stop"), ("pilot_stopped", "stopped by the pilot")):
                if self.state.get(k):
                    raise SystemExit(f"this run was {why}: nothing more runs until the author decides")
            self.state["elapsed_before"] = self.state.get("elapsed_saved", 0)
        else:
            for f in ("run.log", "state.json"):
                if (self.out / f).exists():
                    (self.out / f).unlink()
            self.state = {"started": stamp(), "elapsed_before": 0.0, "toy": self.toy, "jobs": self.jobs,
                          "rule_sha256": C.sha256(C.RULE_FILE), "rule_text": C.RULE_FILE.read_text(),
                          "vendor08_sha256": L.VENDOR08_SHA256, "operator": {
                              "timeout_min_at_least": C.OPERATOR_TIMEOUT_MIN,
                              "instruction": "start once; run nothing else until the exit notice (rule-r2 item 5)"}}
            self.write_header()
        self.state.setdefault("code_sessions", []).append({"time": stamp(), "code": self.code})
        self.t0 = time.monotonic()                          # the run's clock, summed over sessions
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
                step = "step 1 (build)"
                self.build()
                step = "step 2"
                self.unit_tests()
                if "pilot" not in self.state:
                    step = "step 3"
                    self.state["pilot_records"] = []
                    self.step3_pilot()
                step = "steps 4-7"
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
                step = "steps 4-7"
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
            bad = C.archive_check()
            if bad:
                self.log(f"WARNING: round 1's official data changed during the run: {bad}")
            self.state["archive_after"] = bad or "unchanged"
            self.save()
            import r2_analyze                              # step 8, after the last timed block
            r2_analyze.report(self.out)
            self.log(f"results: {self.out / 'results.md'}")

    def rest(self):
        self.step4_gates()
        self.step5_counts()
        assert threading.active_count() == 1, "the worker pool must be shut down before step 6 (rule-r2 item 5.1)"
        self.step6_rounds()
        self.step7_profile()

    def do_rerun(self, impl):
        """rule-r2 item 12: the row re-run in full after a fix under review; its earlier results are dropped."""
        rows = set(C.IMPL_ROWS[impl])
        self.state.setdefault("reruns", []).append({"impl": impl, "time": stamp()})
        for k in ("gates", "counts", "mems"):
            self.state[k] = {key: v for key, v in self.state.get(k, {}).items() if not key.startswith(impl + ":")}
        self.state["records"] = [r for r in self.state.get("records", []) if r["row"] not in rows]
        self.state["profiles"] = [r for r in self.state.get("profiles", []) if r["row"] not in rows]
        self.state["calibration"] = {k: v for k, v in self.state.get("calibration", {}).items()
                                     if k.split(":")[1] not in rows}
        self.state["stopped_impls"] = [i for i in self.state.get("stopped_impls", []) if i != impl]
        for b in self.state.get("bugs", []):
            if b["impl"] == impl:
                b["rerun"] = stamp()
        self.state["rerun_only"] = [impl]
        self.build()
        self.unit_tests()
        self.step4_gates([impl])
        self.step5_counts([impl])
        assert threading.active_count() == 1
        self.step6_rounds()
        self.step7_profile()
        self.state.pop("rerun_only", None)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--toy", action="store_true", help="toy workloads in test/build/r2toy/ (harness tests only)")
    ap.add_argument("--jobs", default="", help="toy runs only: a subset of jobs")
    ap.add_argument("--pilot-only", action="store_true")
    ap.add_argument("--hard-stop-s", type=float, default=None, help="toy runs only: a shorter hard stop")
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--rerun", choices=C.IMPLS)
    a = ap.parse_args()
    r = Runner(a)
    r.preflight()
    r.start()


if __name__ == "__main__":
    main()
