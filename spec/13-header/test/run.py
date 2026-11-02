"""run.py: spec 13 v0.2 §7's timed trials, spec 12's method (spec 12 test/run.py). For each of the 72 rows of gen.py
(integers, then complex numbers; each operation under each sign mix and, for add, sub and collapse, each
magnitude-order mix; 1024 pairs each), one block:
- wait until the machine is idle (CPU at most 10% over 1 s, 1-minute load at most 1.5), retrying;
- each build pinned to core 1 with taskset: calibrate r so that one rep (r passes over the 1024 pairs) takes
  50-100 ms, then 15 reps; the median time per operation and the spread (max - min) / median;
- builds: Control, A, A's branch-free row, B, and the first of them again (Control; B for collapse, B's only), whose
  difference from the first is the noise: a difference counts only beyond max(5%, noise);
- every result is stored and checksummed after the clock (in the binary); a block's checksums must agree with each
  other and with gen.py's exact values;
- beside: bytes per value (Control 8, 16 complex; A 9; B 16, 32 complex) and, with --ir, callgrind's Ir per operation
  in the row's loop function.
The correctness check (check.py) runs first as a gate. Results: build/results/report.md and results.json (build/toy/
for --toy).

  python3 test/run.py --toy [--rows int-add-rand-half,cx-mul-pos]   a smoke run (2 ms reps, 3 of them): not a measurement
  python3 test/run.py [--ir]                                         the real trials: a quiet machine only
"""
import argparse
import json
import os
import re
import statistics
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen  # noqa: E402

HERE = gen.HERE
BIN = HERE / "build" / "bin"
BUILDS = ("control", "a", "abf", "anm", "amul", "b")
CORE = "1"
REPS, MIN_NS, BAND = 15, 50_000_000, 0.05
IDLE_CPU, IDLE_LOAD, IDLE_RETRY_S, IDLE_GIVEUP_S = 0.10, 1.5, 10, 600


def log(msg):
    print(f"{time.strftime('%H:%M:%S')} {msg}", flush=True)


def cpu_busy(sample_s=1.0):
    def read():
        v = [int(x) for x in Path("/proc/stat").read_text().split("\n")[0].split()[1:9]]
        return sum(v) - v[3] - v[4], sum(v)
    b0, t0 = read()
    time.sleep(sample_s)
    b1, t1 = read()
    return (b1 - b0) / max(t1 - t0, 1)


def wait_idle(toy):
    t = time.monotonic()
    while True:
        cpu, load = (0.0, 0.0) if toy else (cpu_busy(1.0), os.getloadavg()[0])
        if (cpu <= IDLE_CPU and load <= IDLE_LOAD) or toy:
            return {"cpu": round(cpu, 3), "load": load}
        if time.monotonic() - t > IDLE_GIVEUP_S:
            sys.exit(f"machine not idle for {IDLE_GIVEUP_S} s (cpu {cpu:.0%}, load {load})")
        log(f"busy (cpu {cpu:.0%}, load {load}); retrying in {IDLE_RETRY_S} s")
        time.sleep(IDLE_RETRY_S)


def build_of(row):
    return row[:-2] if row.endswith("-2") else row


def bench(row, *args):
    p = subprocess.run(["taskset", "-c", CORE, str(BIN / build_of(row)), *map(str, args)], capture_output=True,
                       text=True)
    return p.returncode, p.stdout.split("\n")


def time_cell(row, job, min_ns, reps):
    rc, out = bench(row, "calib", job, min_ns)
    if rc != 0:
        return {"trap": f"exit {rc}"}
    r = int(out[0].split()[1])
    rc, out = bench(row, "time", job, r, reps)
    if rc != 0:
        return {"trap": f"exit {rc}"}
    ts = [int(l.split()[2]) / (r * gen.N) for l in out if l.startswith("rep ")]
    med = statistics.median(ts)
    return {"r": r, "ns": ts, "median": med, "spread": (max(ts) - min(ts)) / med,
            "sum": next(l.split()[1] for l in out if l.startswith("sum ")),
            "bytes": int(next(l.split()[1] for l in out if l.startswith("bytes ")))}


def ir(b, job, fn, tmp):
    """callgrind Ir per operation, in the loop function only: the run with 2 passes less the run with 1, over N"""
    v = []
    for r in (1, 2):
        f = tmp / "cg.out"
        p = subprocess.run(["valgrind", "--tool=callgrind", f"--callgrind-out-file={f}", f"--toggle-collect={fn}",
                            str(BIN / b), "time", str(job), str(r), "1"], capture_output=True)
        if p.returncode != 0:
            return None
        v.append(int(re.search(r"^(?:summary|totals): (\d+)", f.read_text(), re.M).group(1)))
    return (v[1] - v[0]) / gen.N


def compare(t, ref, row, noise):
    """T(ref)/T(row): above 1 the row is faster; it counts only beyond max(5%, noise)"""
    if "median" not in t.get(ref, {}) or "median" not in t.get(row, {}):
        return {"ratio": None, "verdict": "trap"}
    r = t[ref]["median"] / t[row]["median"]
    h = max(BAND, noise)
    return {"ratio": r, "h": h, "verdict": "faster" if r > 1 + h else "slower" if r < 1 - h else "within noise"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--toy", action="store_true", help="2 ms reps, 3 of them, no idle wait: a smoke run")
    ap.add_argument("--rows", default="", help="comma-separated row names (default all 72)")
    ap.add_argument("--ir", action="store_true", help="callgrind Ir per operation beside the times")
    ap.add_argument("--no-check", action="store_true", help="skip the correctness gate (toy only)")
    a = ap.parse_args()
    min_ns, reps = (2_000_000, 3) if a.toy else (MIN_NS, REPS)
    out = HERE / "build" / ("toy" if a.toy else "results")
    out.mkdir(parents=True, exist_ok=True)
    t0 = time.monotonic()
    if not (a.toy and a.no_check):
        p = subprocess.run([sys.executable, str(HERE / "test" / "check.py")] + (["--quick"] if a.toy else []),
                           capture_output=True, text=True)
        log("check: " + p.stdout.strip().splitlines()[-1])
        if p.returncode != 0:
            print(p.stdout)
            sys.exit("correctness check failed")
    rows = gen.generate(HERE / "build" / "work")
    names = [r for r in a.rows.split(",") if r] or list(rows)
    timing, irs = {}, {}
    for name in names:
        path, kind, op, want = rows[name]
        builds = ("b",) if op == "collapse" else BUILDS
        order = builds + (builds[0] + "-2",)
        if a.ir:
            for b in builds:
                f = f"loop_{'int' if b in ('a', 'abf', 'anm', 'amul') else kind}_{op}"
                irs[f"{b}|{name}"] = ir(b, path, f, out)
        idle = wait_idle(a.toy)
        t = {row: time_cell(row, path, min_ns, reps) for row in order}
        m1, m2 = t[order[0]].get("median"), t[order[-1]].get("median")
        noise = abs(m1 - m2) / min(m1, m2) if m1 and m2 else None
        agree = {x.get("sum") for x in t.values()} == {want}
        timing[name] = {"kind": kind, "op": op, "idle": idle, "rows": t, "noise": noise, "sums_agree": agree}
        log(f"{name}: noise {noise if noise is None else round(noise, 4)}; " + ", ".join(
            f"{r} {x['median']:.4g} ns" if "median" in x else f"{r} {x['trap']}" for r, x in t.items())
            + ("" if agree else "; CHECKSUMS DIFFER"))
    pairs = []
    for name in names:
        tm = timing[name]
        if tm["op"] != "collapse":
            pairs += [dict(row_name=name, ref="control", row=b, **compare(tm["rows"], "control", b, tm["noise"] or 0))
                      for b in BUILDS[1:]]
    res = {"toy": a.toy, "n": gen.N, "reps": reps, "min_ns": min_ns, "timing": timing, "ir": irs, "pairings": pairs}
    (out / "results.json").write_text(json.dumps(res, indent=1))
    L = [f"# 13-header v0.2 §7 timed trials{' (TOY: not a measurement)' if a.toy else ''}", "",
         f"N = {gen.N} pairs per row, {reps} reps per cell; median time per operation. Bytes per value: Control 8 "
         "(16 complex), A 9, B 16 (32 complex).", "",
         "| row | build | ns/op | spread | Ir/op | bytes/value | sums |", "| --- | --- | --- | --- | --- | --- | --- |"]
    for name in names:
        tm = timing[name]
        for r, x in tm["rows"].items():
            irv = irs.get(f"{build_of(r)}|{name}")
            L.append(f"| {name} | {r} | " + (f"{x['median']:.4g} | {x['spread']:.1%}" if "median" in x
                                               else f"{x['trap']} | -")
                     + f" | {'-' if irv is None else round(irv, 1)} | {x.get('bytes', '-')} | "
                     + f"{'agree' if tm['sums_agree'] else 'DIFFER'} |")
    L += ["", "| row | build x Control | T(Control)/T(build) | band | time |", "| --- | --- | --- | --- | --- |"]
    for p in pairs:
        L.append(f"| {p['row_name']} | {p['row']} | {'-' if p['ratio'] is None else round(p['ratio'], 3)} | "
                 f"{'-' if p.get('h') is None else round(p['h'], 3)} | {p['verdict']} |")
    (out / "report.md").write_text("\n".join(L) + "\n")
    log(f"done in {time.monotonic() - t0:.0f} s: {out / 'report.md'}")


if __name__ == "__main__":
    main()
