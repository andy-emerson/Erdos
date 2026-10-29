"""The runner of research/rule.md: order of work, pilot with its projection, gates with the exact checks, counting
and memory runs, calibration (forms and r), 12 rounds, the time cap and the 60-minute hard stop.

    python3 runner.py                      the measured run: results in test/results-q32/ (refuses unless rule.md is
                                           the fixed rule, the 08 copies are unchanged, the archive of b3b88b6 is
                                           unchanged, and the machine has 2 cores)
    python3 runner.py --pilot-only         steps 1-3 (generation, unit tests, pilot with its projection), then stop
    python3 runner.py --resume             continue a run stopped by a failed gate or after --pilot-only: re-runs the
                                           unit tests and every gate not passed, then goes on (refused once a timed
                                           round of step 6 has run); results of other code are dropped and redone
    python3 runner.py --rerun L1           item 5: after a bug is fixed under review, re-run that implementation in
                                           full (unit tests, gates, counting and memory runs, calibration, every
                                           round); L1 re-runs L1 and L1-null; "all" re-runs every row; any
                                           implementation whose stored gate or counting results ran other code is
                                           re-run too (a change to l1.c re-runs L1, L1-opt, L1-Q32, L1-opt-Q32 and
                                           the lookup gate)

Rows (rule item 1 and its revision of Oct 29, 2026): L1, L1-opt, L1-null, ctl, fmpq, mpq, fz where marked, and L1-Q32
and L1-opt-Q32 (implementations L1q and L1oq) on the jobs whose predicted W at the end is within the Q32 capacity of
32,768. The gates also run the lookup gate (L1g, spec §18.1 item 4) on every job, and check each Q32 row against its
Q64 row (spec §18.3 item 1).
    python3 runner.py --toy [--jobs 1,3] [--hard-stop-s S]
                                           harness test only: toy-size workloads in build/toy/

Results (rule, revision of Oct 29, 2026: the official data): this run writes only to test/results-q32/. The measured
run of b3b88b6 is archived, unchanged, in test/results-q64-b3b88b6/ (with the original test/results/); the runner never
writes, deletes or overwrites a file there, refuses to start if any archived file differs from its recorded sha256, and
checks it again at the end.

Before any timed run (the pilot, and each job's calibration and rounds) the machine must be idle: load average and other
processes' CPU use under the thresholds of common.py; if not, the runner waits, logging it, and after 10 minutes refuses
(R-13, as 08's R-p). The hard-stop clock starts at the build, just before step 1 (R-14).

Adapted from 08-skeletons/test/runner.py (its decisions R-a to R-c, R-g, R-i to R-o carry over; test/DECISIONS.md).
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
import time
from pathlib import Path

import numpy as np

import common as C
import gen
import libconf as L


class HardStop(Exception):
    pass


class RunStop(Exception):
    """The run stops and is reported to the author (projection over 45 minutes, failed gate, failed pilot check)."""


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
    process is excluded; it has no child while it checks.  (As 08's R-p.)"""
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


def code_hashes():
    """{impl: sha256 of its own files, its shared files, the compiler and flags}, and the sha256 of the shared integer
    module alone (08 harness R-o)."""
    def h(paths, extra=""):
        s = hashlib.sha256(extra.encode())
        for p in sorted(set(paths), key=str):
            s.update(Path(p).name.encode() + b"\0" + Path(p).read_bytes() + b"\0")
        return s.hexdigest()
    code = {}
    for impl in C.IMPLS:
        flags = json.dumps([L.CC, L.CFLAGS, L.MODE_DEFS, L.IMPL_DEFS[impl], L.LINK.get(impl, [])])
        code[impl] = h(L.SHARED[impl] + L.OWN[impl], flags)
    return code, h([L.INT_MODULE])


class Runner:
    def __init__(self, a):
        self.a = a
        self.toy = a.toy
        base = C.BUILD / "toy" if a.toy else C.BUILD
        self.out = (base / "results") if a.toy else C.RESULTS
        self.wl = base / "work"
        self.bin = base / "bin"
        self.tmp = base / "readout"
        self.jobs = [int(x) for x in a.jobs.split(",")] if a.jobs else sorted(C.JOBS)
        self.hard_stop = C.HARD_STOP_S
        if a.hard_stop_s is not None:
            if not a.toy:
                raise SystemExit("--hard-stop-s is for toy runs only")
            self.hard_stop = a.hard_stop_s
        self.state_file = self.out / "state.json"
        self.state = None
        self.t0 = None
        self.code, self.code_int = code_hashes()
        self.idle_probe = idle_probe                   # replaceable in the harness tests
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
        Returns (CompletedProcess, or None if its own limit passed; wall seconds)."""
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
        """Untimed work (steps 2, 4, 5), two processes at a time, one per core. tasks: list of (key, fn(core))."""
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
        inc = sum((["-I", str(p)] for p in L.INCLUDE), [])
        for impl in C.IMPLS:
            for mode in L.MODES[impl]:
                exe = self.bin / f"bench-{impl}-{mode}"
                cmd = ([L.CC] + L.CFLAGS + L.MODE_DEFS[mode] + L.IMPL_DEFS[impl] + inc + [str(C.HERE / "bench.c")] +
                       [str(s) for s in L.SOURCES[impl]] + ["-o", str(exe)] + L.LINK.get(impl, []))
                r = subprocess.run(cmd, capture_output=True, text=True)
                if r.returncode:
                    raise RunStop(f"build failed: {' '.join(cmd)}\n{r.stderr}")
        self.units = []
        objs = []
        for oname, (src, defs) in L.UNIT_OBJECTS.items():            # l1.c twice: L1, and L1-opt (-DL1_OPT)
            o = self.bin / f"unit-{oname}"
            cmd = [L.CC] + L.CFLAGS + L.MODE_DEFS["test"] + defs + inc + ["-c", str(src), "-o", str(o)]
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                raise RunStop(f"unit-test build failed: {' '.join(cmd)}\n{r.stderr}")
            objs.append(str(o))
        for name, (src, tdefs) in L.UNIT_TESTS.items():
            exe = self.bin / name
            cmd = [L.CC] + L.CFLAGS + L.MODE_DEFS["test"] + tdefs + inc + ["-I", str(L.LIBDIR / "tests"), str(src)] + \
                objs + ["-o", str(exe)]
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                raise RunStop(f"unit-test build failed: {' '.join(cmd)}\n{r.stderr}")
            self.units.append((name, [exe] + (["--seeds", L.DIFF_SEEDS] if name.startswith("diff") else [])))
        # 08's own unit tests, for ctl: 08's lib at the commit the copies were taken from, extracted read-only
        lib08 = self.bin.parent / "lib08"
        if lib08.exists():
            shutil.rmtree(lib08)
        lib08.mkdir(parents=True)
        root = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=C.ROOT, capture_output=True, text=True)
        if root.returncode:
            raise RunStop("08's unit tests: not in a git repository")
        arch = subprocess.run(f"git archive {L.COMMIT_08} 08-skeletons/lib | tar -x -C {lib08}", shell=True,
                              cwd=root.stdout.strip(), capture_output=True, text=True)
        if arch.returncode:
            raise RunStop(f"08's unit tests: git archive failed: {arch.stderr}")
        d08 = lib08 / "08-skeletons" / "lib"
        for name, rel in L.VENDOR08_PATH.items():
            if hashlib.sha256((d08 / rel).read_bytes()).hexdigest() != L.VENDOR08_SHA256[name]:
                raise RunStop(f"08 at {L.COMMIT_08}: {rel} differs from lib/vendor08/{name}")
        exe = self.bin / "unit-08"
        cmd = [L.CC] + L.CFLAGS + L.MODE_DEFS["test"] + ["-I", str(d08 / "include"), "-I", str(d08 / "tests"),
                                                         str(d08 / "tests" / "unit.c")] + \
            [str(d08 / s) for s in L.LIB08_SOURCES] + ["-o", str(exe)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            raise RunStop(f"08 unit-test build failed: {r.stderr}")
        self.units.append(("unit-08", [exe]))
        v = subprocess.run([str(self.bin / "bench-fmpq-timing"), "version"], capture_output=True, text=True)
        self.versions = v.stdout.strip()

    def exe(self, impl, mode):
        return self.bin / f"bench-{impl}-{mode}"

    def wlpath(self, job):
        return self.wl / f"job-{job:02d}.wl"

    def meta(self, job):
        return json.loads((self.wl / f"job-{job:02d}.json").read_text())

    def q32_fits(self, job):
        """Rule, revision of Oct 29, 2026 (capacity): a Q32 row runs only on the jobs whose generator-predicted W at the
        end is at most the Q32 capacity, 32,768."""
        return str(job) not in self.state.get("q32_over_capacity", {})

    def rows_of(self, job):
        return C.rows_of(job, q32=self.q32_fits(job))

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

    @staticmethod
    def parse_counts(lines):
        """{(phase, name): total over core and store} from ctr/xctr lines, and {(phase, part, name): value}."""
        tot, full = {}, {}
        for l in lines:
            t = l.split()
            if t[0] in ("ctr", "xctr"):
                tot[(t[1], t[3])] = tot.get((t[1], t[3]), 0) + int(t[4])
                full[(t[1], t[2], t[3])] = int(t[4])
        return tot, full

    def exact_checks(self, impl, job, stdout):
        """Spec §14.1 items 1-2 and, for L1-opt and L1-opt-Q32, §17.3 item 2: W after entry and at the end, the count of
        every case per phase, and the predicted trick counters, each equal to the generator's figure. Returns a list
        of failures."""
        pred = self.meta(job)["predicted"]
        fails = []
        wl = {l.split()[1]: int(l.split()[2]) for l in stdout.splitlines() if l.startswith("wholes ")}
        if wl.get("after_entry") != pred["W_after_entry"]:
            fails.append(f"W after entry {wl.get('after_entry')} != predicted {pred['W_after_entry']}")
        if wl.get("end") != pred["W_end"]:
            fails.append(f"W at the end {wl.get('end')} != predicted {pred['W_end']}")
        tot, _ = self.parse_counts(stdout.splitlines())
        for ph in ("add", "sub", "mul", "div"):
            want = pred["case_counts"].get(ph, {})
            for k in C.CASES_OF[ph]:
                got = tot.get((ph, "case_" + k), 0)
                if got != want.get(k, 0):
                    fails.append(f"{ph} case {k}: {got} != predicted {want.get(k, 0)}")
        if impl in C.OPT_IMPLS:
            for ph in C.PHASES:
                want = pred["tricks"].get(ph, {})
                names = set(want) | {n for (p, n) in tot if p == ph and n.startswith(("o1_", "o2_", "o3_", "o4_", "o5_"))}
                for n in sorted(names):
                    if tot.get((ph, n), 0) != want.get(n, 0):
                        fails.append(f"{ph} {n}: {tot.get((ph, n), 0)} != predicted {want.get(n, 0)}")
        return fails

    def same_run(self, job, base, impl, what):
        """The gate runs of impl and base on job: the same stored values and the same whole-number list."""
        fails = []
        ra, rb = self.tmp / f"g-{job:02d}-{base}.out", self.tmp / f"g-{job:02d}-{impl}.out"
        if ra.read_bytes() != rb.read_bytes():
            fails.append(f"stored values differ from {what}'s")
        if (self.tmp / f"w-{job:02d}-{base}.bin").read_bytes() != (self.tmp / f"w-{job:02d}-{impl}.bin").read_bytes():
            fails.append(f"whole-number list differs from {what}'s")
        return fails

    def against_l1(self, job, opt="L1opt"):
        """Spec §17.3 items 1 and 3 on the gate runs: L1-opt's (or L1-opt-Q32's) stored values and whole-number list
        equal to L1's (L1-Q32's); its found plus skipped lookups equal to L1's found, appended equal, and its arithmetic
        equal to L1's less its counted skips. Returns a list of failures."""
        base = C.BASE_OF[opt]
        g = self.state["gates"]
        a, b = g.get(f"{base}:{job}"), g.get(f"{opt}:{job}")
        if not a or not b or "fail" in a or "fail" in b:
            return [f"{base} or {opt} has no passed gate"]
        fails = self.same_run(job, base, opt, base)
        ta, tb = {}, {}
        for src, dst in ((a["counts"], ta), (b["counts"], tb)):
            for (ph, n), v in self.parse_counts(src)[0].items():
                if ph != "read":
                    dst[n] = dst.get(n, 0) + v
        sk = sum(v for n, v in tb.items() if n.startswith(("o1_skipped", "o2_skipped")))
        rel = [("intern_found", ta.get("intern_found", 0) - sk, "found + skipped"),
               ("intern_appended", ta.get("intern_appended", 0), "appended"),
               ("lookup_search", ta.get("lookup_search", 0) - sk, "lookups"),
               ("gcd", ta.get("gcd", 0) - tb.get("o3_gcd_skipped", 0) - tb.get("o5_gcd_skipped", 0), "gcds"),
               ("gcd_work", ta.get("gcd_work", 0) - tb.get("o3_gcd_work", 0) - tb.get("o5_gcd_work", 0), "gcd work"),
               ("div_exact", ta.get("div_exact", 0) - tb.get("o4_div_skipped", 0) - tb.get("o5_div_skipped", 0),
                "divisions"),
               ("mul", ta.get("mul", 0) - tb.get("o4_mul_skipped", 0), "multiplications"),
               ("add", ta.get("add", 0) - tb.get("o5_add_skipped", 0), "additions")]
        for name, want, what in rel:
            if tb.get(name, 0) != want:
                fails.append(f"{what}: {opt} {tb.get(name, 0)} != {base} less skips {want}")
        return fails

    @staticmethod
    def scaled_o7(lines, num, den):
        """Counter lines with O-7's bytes avoided (rational-list bytes) scaled by num / den; every other line as is."""
        out = []
        for l in lines:
            t = l.split()
            if t[0] == "xctr" and t[3] == "o7_bytes_avoided":
                t[4] = str(int(t[4]) // den * num)
            out.append(" ".join(t))
        return out

    def against_q64(self, job, q32, counts_key="gates"):
        """Spec §18.3 item 1: a Q32 row against its Q64 row on a job within capacity: the same stored values and
        whole-number list (gate runs), and every counter equal, line for line, except O-7's bytes avoided, which are
        rational-list bytes and so half (4 bytes per rational against 8). Returns a list of failures."""
        q64 = C.Q64_OF[q32]
        src = self.state[counts_key]
        a, b = src.get(f"{q64}:{job}"), src.get(f"{q32}:{job}")
        if not a or not b or "counts" not in a or "counts" not in b:
            return [f"{q64} or {q32} has no result to compare"]
        fails = self.same_run(job, q64, q32, q64) if counts_key == "gates" else []
        if self.scaled_o7(a["counts"], 1, 2) != list(b["counts"]):
            fails.append(f"counters differ from {q64}'s")
        if counts_key == "counts" and a.get("percall_sha256") != b.get("percall_sha256"):
            fails.append(f"per-call core counts differ from {q64}'s")
        return fails

    def lookup_gate(self, job):
        """Spec §18.1 item 4: the parameterized lookup built at 32-bit slots (L1g, L1 with that lookup) gives, on every
        job, the probe counts of L1, whose lookup is 08's sk_lookup: every counter line equal (lookup searches and
        probes per phase and part included), and the same stored values and whole-number list. Failures, or []."""
        g = self.state["gates"]
        a, b = g.get(f"L1:{job}"), g.get(f"L1g:{job}")
        if not a or not b or "fail" in a or "fail" in b:
            return ["L1 or the lookup gate's build has no passed gate run"]
        fails = self.same_run(job, "L1", "L1g", "L1")
        if a["counts"] != b["counts"]:
            diff = [x for x in b["counts"] if x not in set(a["counts"])][:5]
            fails.append(f"counters differ from L1's (sk_lookup): {diff}")
        return fails

    def percase_check(self, job):
        """Spec §14.1 item 3, on the counting runs: per operation, L1's core gcds, gcd work, divisions, multiplications
        and additions summed over the operations of cases den and general equal ctl's. Also the per-case sums of
        L1, L1-opt and ctl, for rule items 6 and 12. Returns (failures, per-case table)."""
        cases = np.frombuffer((self.wl / f"job-{job:02d}.case").read_bytes(), dtype=np.int8)
        sums = {}
        for impl in ("L1", "L1opt", "ctl"):        # the Q32 rows' per-call counts equal their Q64 rows' (spec §18.3)
            p = self.tmp / f"p-{job:02d}-{impl}-0.bin"
            arr = np.fromfile(p, dtype="<i8").reshape(-1, 5)
            per = {}
            for k in range(len(C.CASES)):
                sel = cases == k
                if sel.any():
                    per[C.CASES[k]] = dict(n=int(sel.sum()), **{f: int(arr[sel, i].sum())
                                                                for i, f in enumerate(C.PERCALL_FIELDS)})
            sums[impl] = per
        fails = []
        for k, v in sums["L1"].items():
            if k.endswith(("_den", "_general")):
                c = sums["ctl"].get(k)
                if {f: v[f] for f in C.PERCALL_FIELDS} != {f: c[f] for f in C.PERCALL_FIELDS}:
                    fails.append(f"case {k}: L1 {v} != ctl {c}")
        return fails, sums

    # ------------------------------------------------------------------------------------------- idle machine
    def wait_idle(self, where):
        """Before any timed run (the pilot, and each job's calibration and rounds in step 6): the machine must be idle
        (R-13; rule.md, revision of Oct 29, 2026).  If it is busy, re-check every IDLE_POLL_S for up to IDLE_WAIT_S,
        logging each check, then refuse.  The wait counts on the run clock.  Toy runs (harness tests) log the check and
        never wait or refuse."""
        t = time.monotonic()
        checks = self.state.setdefault("idle_checks", [])
        while True:
            self.check_clock(where)
            ok, d = self.idle_probe()
            rec = dict(d, where=where, time=stamp(), idle=ok, waited_s=round(time.monotonic() - t, 1))
            checks.append(rec)
            self.save()
            if ok:
                self.log(f"machine idle before {where} (load {d['load1']}, max {d['load_max']})" +
                         (f", after waiting {rec['waited_s']:.0f} s" if rec["waited_s"] > 0 else ""))
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
    def timed(self, job, row, kind, rnd=None, pos=None, r=None, reps=None, form=None):
        """One timed invocation on core 1; nothing else runs meanwhile. kind: calib, time or form."""
        impl = C.ROW_IMPL[row]
        ro = self.tmp / f"t-{job:02d}-{row}.out"
        if ro.exists():
            ro.unlink()
        cmd = [self.exe(impl, "timing"), "calib" if kind == "calib" else "time", self.wlpath(job), ro]
        if kind != "calib":
            cmd += [r, reps]
        if form:
            cmd += [form]
        p, wall = self.proc(cmd, C.TIMED_CORE, where=f"job {job}, {row}, {kind} {rnd or ''}")
        rec = {"job": job, "row": row, "kind": kind, "round": rnd, "pos": pos, "wall": wall, "rc": p.returncode,
               "form": form}
        lines = p.stdout.splitlines()
        reps_out = [[int(x) for x in l.split()[1:6]] for l in lines if l.startswith(("calib ", "rep "))]
        rec["overcap"] = any(l.startswith("overcap") for l in lines)
        rec["reps_out"] = reps_out
        vl = [l for l in lines if l.startswith(("calib ", "rep ")) and "gmp=" in l]
        if vl:
            rec["versions"] = " ".join(x for x in vl[-1].split() if "=" in x)
            if "threads=1" not in rec["versions"]:
                rec["bug"] = {"what": f"FLINT is not single-threaded: {rec['versions']}"}
                return rec
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
            rec["rep_ns"] = last[1]
            rec["reps"] = 1 if last[1] >= C.ONE_REP_NS else 3
        else:
            rec["r"], rec["reps"] = r, reps
            if not rec["overcap"]:
                kept = reps_out if reps == 1 else reps_out[1:]
                best = min(kept, key=lambda x: x[1])   # R-b: the rep of minimum total; its phases go with it
                rec["total"], rec["entry"], rec["ops"], rec["read"] = (best[1] / r, best[2] / r, best[3] / r,
                                                                       best[4] / r)
        if rec["overcap"]:
            last = reps_out[-1]
            rec["cap_rep_ns"] = last[1]
            rec["cap_time"] = last[1] / (last[0] if kind == "calib" else r)
        return rec

    def calibrate(self, job, store, live):
        """Rule item 4, immediately before the job's round 1, on core 1: the form of fmpq and mpq (3 invocations of each
        form, alternating A, B, A, B, A, B, at r = 1 with 3 reps, the first discarded; the lower median total wins),
        then r for L1, L1-opt, ctl, fmpq, mpq and fz. L1-null takes L1's."""
        cal, forms = {}, {}
        rows = live(job)
        for row in C.FORMED:
            if row not in rows:
                continue
            tots = {"A": [], "B": []}
            for k in range(C.FORM_CALIB_INVOCATIONS):
                for form in ("A", "B"):
                    rec = self.timed(job, row, "form", r=1, reps=3, form=form)
                    store.append(rec)
                    if "bug" in rec:
                        self.bug(rec, "form calibration")
                        break
                    if rec["overcap"]:
                        tots[form].append(float("inf"))
                    else:
                        tots[form].append(rec["total"])
                if row not in live(job):
                    break
            if row not in live(job):
                continue
            med = {f: float(np.median(v)) for f, v in tots.items()}
            forms[row] = {"form": "A" if med["A"] <= med["B"] else "B", "median_A": med["A"], "median_B": med["B"],
                          "totals": tots}
        for row in rows:
            if row == "L1-null":
                continue
            if row in C.FORMED and row not in forms:
                continue
            form = forms[row]["form"] if row in forms else None
            rec = self.timed(job, row, "calib", form=form)
            store.append(rec)
            cal[(job, row)] = rec
            if "bug" in rec:
                self.bug(rec, "calibration")
        if (job, "L1") in cal and "bug" not in cal[(job, "L1")]:
            cal[(job, "L1-null")] = dict(cal[(job, "L1")], row="L1-null", shared_from="L1")
        return cal, forms

    def rounds(self, job, nrounds, cal, store, live, label):
        """Interleaved rounds: every row once per round, as its own process, order rotated by one per round."""
        order = self.rows_of(job)
        capped = set()
        for k in range(nrounds):
            rot = order[k % len(order):] + order[:k % len(order)]
            for pos, row in enumerate(rot):
                c = cal.get((job, row))
                if c is None or "bug" in c or c.get("overcap") or row in capped or row not in live(job):
                    continue
                rec = self.timed(job, row, "time", rnd=k + 1, pos=pos, r=c["r"], reps=c["reps"], form=c.get("form"))
                rec["label"] = label
                store.append(rec)
                if "bug" in rec:
                    self.bug(rec, f"{label} round {k + 1}")
                elif rec["overcap"]:
                    capped.add(row)
                self.save()

    # ------------------------------------------------------------------------------------------- bugs (item 5)
    def bug(self, rec, where):
        impl = C.ROW_IMPL.get(rec["row"], rec["row"])
        entry = {"impl": impl, "row": rec["row"], "job": rec["job"], "where": where, "detail": rec["bug"],
                 "time": stamp()}
        if not self.state.get("measuring"):
            self.state.setdefault("pre_gate_failures", []).append(entry)
            self.save()
            raise RunStop(f"check failure before the gates ({where}): {entry}")
        self.state.setdefault("bugs", []).append(entry)
        if impl in C.LIBRARIES:
            # rule item 5: the driver is reviewed first; until then the row is n/a on this job (D-r5)
            self.state.setdefault("library_na", []).append(f"{impl}:{rec['job']}")
            self.log(f"BUG (library row): {entry}; {impl} is n/a on job {rec['job']} pending review of the driver")
        else:
            if impl not in self.state.setdefault("stopped_impls", []):
                self.state["stopped_impls"].append(impl)
            self.log(f"BUG: {entry}; the measured run of {impl} stops (rule item 5)")
        self.save()

    def live_rows(self, job):
        stopped = set(self.state.get("stopped_impls", []))
        na = set(self.state.get("library_na", []))
        return [r for r in self.rows_of(job) if C.ROW_IMPL[r] not in stopped and f"{C.ROW_IMPL[r]}:{job}" not in na]

    # ------------------------------------------------------------------------------------------- code versions
    def stamp_code(self, impl, res):
        res["code"], res["int"] = self.code[impl], self.code_int
        return res

    def is_stale(self, key, res):
        impl = key.split(":")[0]
        return res.get("code") != self.code[impl] or res.get("int") != self.code_int

    def drop_stale(self):
        """Drop every gate, counting and memory result whose code hash differs from the current code; returns the
        implementations that had one."""
        dropped, impls = self.state.setdefault("dropped_stale", []), set()
        for kind in ("gates", "counts", "mems"):
            kept = {}
            for k, v in self.state.get(kind, {}).items():
                if self.is_stale(k, v):
                    impls.add(k.split(":")[0])
                    dropped.append({"kind": kind, "key": k, "code": v.get("code"), "int": v.get("int"),
                                    "int_changed": v.get("int") != self.code_int, "time": stamp()})
                else:
                    kept[k] = v
            self.state[kind] = kept
        if impls:
            self.log(f"code changed since these results: dropped the stale gates, counting and memory runs of "
                     f"{sorted(impls)}")
        return impls

    def plan_rerun(self, impl):
        impls = set(C.IMPLS) if impl == "all" else {impl}
        stale = self.drop_stale()
        impls |= stale
        rows = {r for i in impls for r in C.IMPL_ROWS[i]}
        prev = self.state.setdefault("reruns", [])
        prev.append({"impl": impl, "also_stale": sorted(stale - {impl}), "time": stamp()})
        self.state["records"] = [r for r in self.state.get("records", []) if r["row"] not in rows]
        self.state["calibration"] = {k: v for k, v in self.state.get("calibration", {}).items()
                                     if k.split(":")[1] not in rows}
        for key in ("gates", "counts", "mems"):
            self.state[key] = {k: v for k, v in self.state.get(key, {}).items() if k.split(":")[0] not in impls}
        self.state["stopped_impls"] = [d for d in self.state.get("stopped_impls", []) if d not in impls]
        self.state["library_na"] = [k for k in self.state.get("library_na", []) if k.split(":")[0] not in impls]
        for b in self.state.get("bugs", []):
            if b["impl"] in impls:
                b["rerun"] = prev[-1]["time"]
        return [i for i in C.IMPLS if i in impls]

    # ------------------------------------------------------------------------------------------- untimed runs
    def gate(self, impl, job, core):
        ro = self.tmp / f"g-{job:02d}-{impl}.out"
        wf = self.tmp / f"w-{job:02d}-{impl}.bin"
        p, wall = self.proc([self.exe(impl, "test"), "gate", self.wlpath(job), ro, wf], core,
                            where=f"gate {impl} job {job}")
        res = self.stamp_code(impl, {"impl": impl, "job": job, "wall": wall, "rc": p.returncode})
        err = self.parse_error(p.stdout)
        if p.returncode or err:
            res["fail"] = err or f"exit {p.returncode}: {p.stderr.strip()[:300]}"
            return res
        bad = self.check(job, ro)
        if bad:
            res["fail"] = f"check: {bad}"
            return res
        res["audits"] = next((l for l in p.stdout.splitlines() if l.startswith("audits")), "")
        res["counts"] = [l for l in p.stdout.splitlines() if l.startswith(("ctr ", "xctr "))]
        if impl in C.LAYER1 or impl in C.GATE_ONLY:
            fails = self.exact_checks(impl, job, p.stdout)
            if fails:
                res["fail"] = "exact checks (spec §14.1, §17.3): " + "; ".join(fails[:10])
        return res

    def count(self, impl, job, core, rep=0):
        ro = self.tmp / f"c-{job:02d}-{impl}-{rep}.out"
        pc = self.tmp / f"p-{job:02d}-{impl}-{rep}.bin"
        p, wall = self.proc([self.exe(impl, "counting"), "count", self.wlpath(job), ro, pc], core,
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
        res["counts"] = [l for l in p.stdout.splitlines() if l.startswith(("ctr ", "xctr ", "hist "))]
        res["mem"] = [l for l in p.stdout.splitlines() if l.startswith(("mem ", "memN "))]
        res["percall_sha256"] = hashlib.sha256(pc.read_bytes()).hexdigest()
        return res

    def mem(self, impl, job, core, rep=0):
        ro = self.tmp / f"m-{job:02d}-{impl}-{rep}.out"
        p, wall = self.proc([self.exe(impl, "timing"), "mem", self.wlpath(job), ro], core,
                            limit=C.COUNT_LIMIT_S, where=f"memory run {impl} job {job}")
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
        res["memrun"] = {}
        for l in p.stdout.splitlines():
            if l.startswith("memrun "):
                t = l.split()
                res["memrun"][t[1]] = {k: int(v) for k, v in (x.split("=") for x in t[2:])}
        return res

    def unit_tests(self):
        res = {}
        for name, cmd in self.units:
            p, wall = self.proc(cmd, C.HARNESS_CORE, where="unit tests")
            res[name] = {"rc": p.returncode, "out": (p.stdout + p.stderr)[-2000:], "wall": wall}
        self.state["unit_tests"] = res
        failed = [k for k, v in res.items() if v["rc"]]
        if failed:
            self.save()
            raise RunStop(f"unit tests failed: {failed}")
        self.log("unit tests pass: " + "; ".join(f"{k}: {v['out'].strip().splitlines()[-1]}" for k, v in res.items()
                                                 if v["out"].strip()))

    def gate_pairs(self, impls_only=None, skip_passed=True):
        """The gate runs: every counted row on every job (the Q32 rows on the jobs within capacity), and the lookup
        gate's build on every job (spec §18.1 item 4)."""
        done = self.state.setdefault("gates", {})
        pairs = []
        for job in self.constructible():
            for impl in C.COUNTED + C.GATE_ONLY:
                if impls_only is not None and impl not in impls_only:
                    continue
                if impl in C.Q32_IMPLS and not self.q32_fits(job):
                    continue
                k = f"{impl}:{job}"
                g = done.get(k)
                if skip_passed and g and "fail" not in g and not self.is_stale(k, g):
                    continue
                pairs.append((impl, job))
        return pairs

    def run_gates(self, pairs, label):
        tasks = [((impl, job), (lambda core, i=impl, j=job: self.gate(i, j, core))) for impl, job in pairs]
        res = self.untimed(tasks)
        gates = self.state.setdefault("gates", {})
        for (impl, job), r in res.items():
            gates[f"{impl}:{job}"] = r
        # once both gates of a job have passed: L1-opt against L1, and L1-opt-Q32 against L1-Q32 (spec §17.3 items 1,
        # 3); each Q32 row against its Q64 row (spec §18.3 item 1); the lookup gate against L1 (spec §18.1 item 4)
        def passed(k):
            return k in gates and "fail" not in gates[k]
        for job in sorted({j for _, j in pairs}):
            for opt in C.OPT_IMPLS:
                k, kb = f"{opt}:{job}", f"{C.BASE_OF[opt]}:{job}"
                if passed(k) and passed(kb):
                    fails = self.against_l1(job, opt)
                    if fails:
                        gates[k]["fail"] = f"against {C.BASE_OF[opt]} (spec §17.3): " + "; ".join(fails)
                    else:
                        gates[k]["against_l1"] = "pass"
            for q32 in C.Q32_IMPLS:
                k, kb = f"{q32}:{job}", f"{C.Q64_OF[q32]}:{job}"
                if passed(k) and passed(kb):
                    fails = self.against_q64(job, q32)
                    if fails:
                        gates[k]["fail"] = f"against {C.Q64_OF[q32]} (spec §18.3): " + "; ".join(fails)
                    else:
                        gates[k]["against_q64"] = "pass"
            k = f"L1g:{job}"
            if passed(k) and passed(f"L1:{job}"):
                fails = self.lookup_gate(job)
                if fails:
                    gates[k]["fail"] = "lookup gate (spec §18.1 item 4): " + "; ".join(fails)
                else:
                    gates[k]["lookup_gate"] = "pass: every counter, probes included, equal to L1's (sk_lookup)"
        self.save()
        failed = {k: v["fail"] for k, v in gates.items() if "fail" in v and tuple(k.split(":")) in
                  {(i, str(j)) for i, j in pairs}}
        if failed:
            raise RunStop(f"{label}: gates failed (fix the row, then --resume): {failed}")
        return res

    def run_counts(self, impls_only=None):
        """Counting runs (L1, L1-opt, ctl) and memory runs (every implementation), two at a time; on jobs 3, 5 and 10
        each is repeated and must give identical figures (rule item 2)."""
        tasks = []
        for job in self.constructible():
            for impl in C.impls_of(job, q32=self.q32_fits(job)):
                if impls_only is not None and impl not in impls_only:
                    continue
                reps = (0, 1) if job in C.COUNT_REPEAT_JOBS else (0,)
                for rep in reps:
                    if impl in C.COUNTED:
                        tasks.append((("count", impl, job, rep), (lambda core, i=impl, j=job, r=rep: self.count(i, j, core, r))))
                    tasks.append((("mem", impl, job, rep), (lambda core, i=impl, j=job, r=rep: self.mem(i, j, core, r))))
        res = self.untimed(tasks)
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
                    r["bug"] = {"what": f"{kind} run repeated: figures differ (rule item 2)"}
                dst[key]["repeat"] = "identical" if "bug" not in r else r["bug"]["what"]
            if "bug" in r:
                self.bug(dict(row=C.IMPL_ROWS[impl][0], job=job, bug=r["bug"]),
                         f"{kind} run" + (" (repeat)" if rep else ""))
        # spec §18.3 item 1: each Q32 row's counters and per-call core counts equal its Q64 row's (O-7's bytes halved)
        for job in self.constructible():
            for q32 in C.Q32_IMPLS:
                if "counts" in counts.get(f"{q32}:{job}", {}) and "counts" in counts.get(f"{C.Q64_OF[q32]}:{job}", {}):
                    fails = self.against_q64(job, q32, counts_key="counts")
                    if fails:
                        self.bug(dict(row=C.IMPL_ROWS[q32][0], job=job,
                                      bug={"what": "spec §18.3 item 1: " + "; ".join(fails)}),
                                 f"counting run, {q32} against {C.Q64_OF[q32]}")
                    else:
                        counts[f"{q32}:{job}"]["against_q64"] = "pass"
        # spec §14.1 item 3: on cases den and general, L1's arithmetic equals ctl's
        percase = self.state.setdefault("percase", {})
        for job in self.constructible():
            if all("counts" in counts.get(f"{i}:{job}", {}) for i in ("L1", "L1opt", "ctl")):
                fails, sums = self.percase_check(job)
                percase[str(job)] = sums
                if fails:
                    self.bug(dict(row="L1", job=job, bug={"what": "spec §14.1 item 3: " + "; ".join(fails)}),
                             "counting run, cases den and general against ctl")
        self.save()

    # ------------------------------------------------------------------------------------------- results file
    def write_header(self):
        rule = C.RULE_FILE.read_text()
        head = ["# Design 3, layer 1: results", "",
                f"Run started {stamp()}." + (" **TOY RUN: a harness test, not a measurement.**" if self.toy else ""),
                "", "## The measurement rule (research/rule.md, verbatim, written before the first trial)", "",
                f"sha256 `{C.rule_sha256()}`", "", "<!-- rule begins -->", rule.rstrip("\n"), "<!-- rule ends -->", ""]
        (self.out / "results.md").write_text("\n".join(head) + "\n")

    def append_calibration(self, job, cal, forms):
        lines = [f"### Calibration, job {job} ({C.JOBS[job]['name']})", ""]
        for row, f in forms.items():
            lines.append(f"- {row}: form {f['form']} (median totals: A {f['median_A'] / 1e6:.3f} ms, "
                         f"B {f['median_B'] / 1e6:.3f} ms)")
        lines += ["", "| row | form | r | calibration rep (ms, r runs) | reps per invocation |",
                  "| --- | --- | --- | --- | --- |"]
        for (j, row), c in sorted(cal.items(), key=lambda x: self.rows_of(job).index(x[0][1])):
            if "bug" in c:
                lines.append(f"| {row} | - | - | bug: see Bugs | - |")
            elif c.get("overcap"):
                lines.append(f"| {row} | {c.get('form') or '-'} | {c['reps_out'][-1][0]} | over cap: "
                             f"{c['cap_rep_ns'] / 1e6:.1f} | - |")
            else:
                src = " (L1's)" if c.get("shared_from") else ""
                lines.append(f"| {row} | {c.get('form') or '-'} | {c['r']}{src} | {c['rep_ns'] / 1e6:.2f}{src} | "
                             f"{c['reps']}{src} |")
        with open(self.out / "results.md", "a") as f:
            f.write("\n".join(lines) + "\n\n")

    # ------------------------------------------------------------------------------------------- steps
    def preflight(self):
        if C.rule_sha256() != C.RULE_SHA256:
            msg = "research/rule.md differs from the fixed rule (sha256 mismatch): refusing to run"
            if not self.toy:
                raise SystemExit(msg)
            print("WARNING (toy run): " + msg)
        bad = L.vendor08_check()
        if bad:
            raise SystemExit(f"the 08 module or control changed (spec §4.3 item 4): {bad}")
        bad = C.archive_check()
        if bad:
            raise SystemExit(f"the archive of the official Q64 run ({C.OFFICIAL_Q64_COMMIT}) changed: {bad}")
        if C.protected(self.out):
            raise SystemExit(f"{self.out} holds official data (rule, revision of Oct 29, 2026): refusing to write there")
        if not self.toy:
            if len(os.sched_getaffinity(0)) < 2 or os.cpu_count() < 2:
                raise SystemExit("the rule needs 2 cores (item 4)")
            if self.a.jobs:
                raise SystemExit("a measured run runs every job of the run plan")
        os.sched_setaffinity(0, {C.HARNESS_CORE})

    def step1(self):
        self.log("step 1: workloads, exact references and predicted figures")
        if self.wl.exists():
            shutil.rmtree(self.wl)
        metas = gen.generate(self.wl, self.jobs, toy=self.toy)
        self.state["gen"] = {str(k): {kk: vv for kk, vv in v.items() if kk != "predicted"} for k, v in metas.items()}
        # the predicted trick figures, reported beside L1-opt's counters (rule item 12; spec §17.3 item 2)
        self.state["predicted_tricks"] = {str(k): v["predicted"]["tricks"] for k, v in metas.items()
                                          if "predicted" in v}
        self.state["workload_sha256"] = {str(j): hashlib.sha256(self.wlpath(j).read_bytes()).hexdigest()
                                         for j in self.jobs if metas[j].get("constructible", True)}
        self.state["not_constructible"] = [j for j in self.jobs if not metas[j].get("constructible", True)]
        # rule, revision of Oct 29, 2026 (capacity): the jobs whose predicted W at the end exceeds the Q32 capacity;
        # the Q32 rows are reported there as over capacity, with W, and do not run
        self.state["q32_over_capacity"] = {str(j): metas[j]["predicted"]["W_end"] for j in self.jobs
                                           if metas[j].get("constructible", True)
                                           and metas[j]["predicted"]["W_end"] > C.Q32_CAPACITY}
        if self.state["q32_over_capacity"]:
            self.log("Q32 rows over capacity (W at the end > 32,768) on jobs " +
                     ", ".join(f"{j} (W {w})" for j, w in self.state["q32_over_capacity"].items()))
        self.save()

    def constructible(self):
        return [j for j in self.jobs if j not in self.state.get("not_constructible", [])]

    def step3_pilot(self):
        """Rule, Run plan step 3: jobs 1, 3, 5, 7 and 10, every row, timing build, form and r calibration and 2 rounds,
        one timed invocation at a time on core 1. Its results and calibration are discarded."""
        self.log("step 3: pilot")
        jobs = [j for j in C.PILOT_JOBS if j in self.constructible()] if not self.toy else \
            [j for j in self.constructible() if j in C.PILOT_JOBS] or self.constructible()[:2]
        store = self.state.setdefault("pilot_records", [])
        pcal = self.state.setdefault("pilot_calibration", {})
        self.wait_idle("the pilot")                  # before the pilot's wall time starts (R-13)
        t = time.monotonic()
        for job in jobs:
            cal, forms = self.calibrate(job, store, self.live_rows)
            pcal[str(job)] = {"forms": forms, "r": {row: {k: c.get(k) for k in ("r", "reps", "rep_ns", "form", "overcap")}
                                                    for (_, row), c in cal.items()}}
            self.rounds(job, C.PILOT_ROUNDS, cal, store, self.live_rows, "pilot")
        wall = time.monotonic() - t
        factor = max(1.0, wall / C.PILOT_NOMINAL_S)
        proj = self.elapsed() + C.PILOT_REMAINING_S * factor
        self.state["pilot"] = {"jobs": jobs, "wall_s": wall, "factor": factor, "elapsed_s": self.elapsed(),
                               "projected_s": proj, "limit_s": C.PILOT_LIMIT_S,
                               "discarded": "the pilot's results and calibration are discarded (rule step 3)"}
        self.save()
        self.log(f"pilot projection: {proj / 60:.1f} min (pilot {wall:.1f} s, factor {factor:.2f}, "
                 f"elapsed {self.elapsed() / 60:.1f} min; limit {C.PILOT_LIMIT_S / 60:g} min)")
        if proj > C.PILOT_LIMIT_S:
            raise RunStop(f"projected total {proj / 60:.1f} min exceeds {C.PILOT_LIMIT_S / 60:g} minutes: the run "
                          "stops and is reported to the author before anything else runs")

    def step4_gates(self, impls_only=None, skip_passed=True):
        self.log("step 4: gates")
        self.run_gates(self.gate_pairs(impls_only, skip_passed), "step 4")
        self.state["measuring"] = True          # every gate passed: a check failure from here on is a bug (item 5)
        self.save()

    def step6_timed(self, rows_only=None, label="measured"):
        self.log("step 6: calibration and rounds")
        store = self.state.setdefault("records", [])
        cals = self.state.setdefault("calibration", {})
        forms_all = self.state.setdefault("forms", {})

        def live(job):
            rows = self.live_rows(job)
            return [r for r in rows if rows_only is None or C.ROW_IMPL[r] in rows_only]

        for job in self.constructible():
            self.wait_idle(f"the calibration and rounds of job {job}")
            self.log(f"job {job}: calibration")
            cal, forms = self.calibrate(job, store, live)
            for (j, row), c in cal.items():
                cals[f"{j}:{row}"] = c
            for row, f in forms.items():
                forms_all[f"{job}:{row}"] = f
            self.append_calibration(job, cal, forms)
            self.save()
            self.log(f"job {job}: {C.ROUNDS} rounds")
            self.rounds(job, C.ROUNDS, cal, store, live, label)
        self.state["finished"] = stamp()
        self.save()

    # ------------------------------------------------------------------------------------------- top level
    def start(self):
        # write-protection first, before anything is created: never make or touch a folder of official data
        # (checked in preflight too; start() must not rely on it having run)
        if C.protected(self.out):
            raise SystemExit(f"{self.out} holds official data (rule, revision of Oct 29, 2026): refusing to write there")
        self.out.mkdir(parents=True, exist_ok=True)
        self.tmp.mkdir(parents=True, exist_ok=True)
        if self.a.resume or self.a.rerun:
            self.state = json.loads(self.state_file.read_text())
            if self.state.get("hard_stopped"):
                raise SystemExit("this run was stopped at 60 minutes: nothing more runs until the author decides")
            if self.state.get("pilot_stopped"):
                raise SystemExit("the pilot's projection stopped this run: nothing more runs until the author decides")
            self.state["elapsed_before"] = self.state.get("elapsed_saved", 0)
        else:
            for f in ("run.log", "state.json"):
                if (self.out / f).exists():
                    (self.out / f).unlink()
            self.state = {"started": stamp(), "elapsed_before": 0.0, "toy": self.toy, "jobs": self.jobs,
                          "rule_sha256": C.rule_sha256(), "rule_text": C.RULE_FILE.read_text(),
                          "vendor08_sha256": L.VENDOR08_SHA256, "commit_08": L.COMMIT_08}
            self.write_header()                      # the rule, verbatim, before the first trial
        self.state.setdefault("code_sessions", []).append({"time": stamp(), "code": self.code, "int": self.code_int})
        self.t0 = time.monotonic()                   # the clock runs from the build, just before step 1
                                                     # (R-14; accumulated over sessions, R-m)
        if self.a.resume:
            self.drop_stale()
            self.save()
        step = "build"
        try:
            self.build()
            self.state["versions"] = self.versions
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
                self.rest()
            else:
                step = "step 1"
                self.step1()
                step = "step 2"
                self.log("step 2: unit tests (test build)")
                self.unit_tests()
                step = "step 3"
                self.step3_pilot()
                if self.a.pilot_only:
                    self.log("pilot only: stopping after step 3")
                    return
                self.rest()
        except HardStop as e:
            self.state["hard_stopped"] = {"step": step, "where": str(e), "elapsed_s": self.elapsed()}
            self.save()
            self.log(f"STOPPED AT {self.hard_stop / 60:g} MINUTES in {step} ({e}); reporting what was done, "
                     "with no verdicts")
        except RunStop as e:
            self.state["run_stopped"] = {"step": step, "reason": str(e), "elapsed_s": self.elapsed()}
            if step == "step 3" and "projected" in str(e):
                self.state["pilot_stopped"] = True
            self.save()
            self.log(f"RUN STOPPED in {step}: {e}")
        finally:
            self.save()
            import analyze
            analyze.report(self.out)
            self.log(f"results: {self.out / 'results.md'}")
            bad = C.archive_check()
            if bad:
                self.log(f"WARNING: the archive of {C.OFFICIAL_Q64_COMMIT} changed during the run: {bad}")

    def rest(self):
        self.step4_gates()
        self.log("step 5: counting runs and memory runs")
        self.run_counts()
        self.step6_timed()

    def do_rerun(self, impl):
        impls = self.plan_rerun(impl)
        self.log(f"re-run of {impls} in full")
        self.save()
        self.unit_tests()
        counted = [i for i in impls if i in C.COUNTED + C.GATE_ONLY]
        if counted:
            self.step4_gates(impls_only=counted, skip_passed=False)
        self.log("step 5 (re-run): counting runs and memory runs")
        self.run_counts(impls_only=impls)
        self.step6_timed(rows_only=impls, label="re-run")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--toy", action="store_true", help="toy-size workloads in build/toy/ (harness tests only)")
    ap.add_argument("--jobs", default="", help="toy runs only: a subset of jobs")
    ap.add_argument("--pilot-only", action="store_true")
    ap.add_argument("--hard-stop-s", type=float, default=None, help="toy runs only: a shorter hard stop, to test it")
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--rerun", choices=C.IMPLS + ("all",))
    a = ap.parse_args()
    r = Runner(a)
    r.preflight()
    r.start()


if __name__ == "__main__":
    main()
