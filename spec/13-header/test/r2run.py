"""r2run.py: spec 13 round 2's timing harness (§5 c), spec 12's method (spec 12 test/run.py). For each block, a job x
operation (45 jobs: k in 1, 2, 4, 8, 16 x three sign mixes x three magnitude-order mixes; 7 operations), and the
growth workload:
- wait until the machine is idle (CPU at most 10% over 1 s, 1-minute load at most 1.5), retrying;
- each row pinned to core 1 with taskset: calibrate r so that one rep (r passes over the 1024 precomputed pairs, or
  over the 64 x 32 growth steps) takes 50-100 ms, then 15 reps; the median time per operation (per step for growth)
  and the spread (max - min) / median;
- rows: Control, A, A's branch-free row, B, and Control again (control-2); the noise is |Control - Control-2| / the
  smaller, and a difference counts only beyond max(5%, noise);
- every result is stored and checksummed after the clock (in the binary); rows of one block must agree, and agree
  with Python's exact values (r2gen.checksum), or the block is marked;
- beside: bytes per value (the -mem binaries) and, with --ir, callgrind's Ir per operation in loop_<op>.
The correctness check (r2check.py) runs first as a gate.

  python3 test/r2run.py --toy [--jobs k1-srand-orand] [--ops add,mul]   a smoke run, not a measurement
  python3 test/r2run.py [--ir]                                           the real round: a quiet machine only
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
import r2gen  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
BIN = HERE / "build" / "bin"
BUILDS = ("control", "a", "abf", "b")
ROWS = BUILDS + ("control-2",)
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


def exe(row):
    return BIN / ("control" if row == "control-2" else row)


def bench(row, job, op, *args):
    p = subprocess.run(["taskset", "-c", CORE, str(exe(row)), args[0], str(job), op, *map(str, args[1:])],
                       capture_output=True, text=True)
    return p.returncode, p.stdout.split("\n")


def time_cell(row, job, op, n, min_ns, reps):
    rc, out = bench(row, job, op, "calib", min_ns)
    if rc != 0:
        return {"trap": f"exit {rc}"}
    r = int(out[0].split()[1])
    rc, out = bench(row, job, op, "time", r, reps)
    if rc != 0:
        return {"trap": f"exit {rc}"}
    ts = [int(l.split()[2]) / (r * n) for l in out if l.startswith("rep ")]
    med = statistics.median(ts)
    return {"r": r, "ns": ts, "median": med, "spread": (max(ts) - min(ts)) / med,
            "sum": next(l.split()[1] for l in out if l.startswith("sum "))}


def memory(jobs, gpath):
    mem = {}
    for b in BUILDS:
        for name, path in list(jobs.items()) + [("growth", gpath)]:
            out = subprocess.run([str(BIN / f"{b}-mem"), "mem", str(path)], capture_output=True, text=True,
                                 check=True).stdout
            for t in (l.split() for l in out.splitlines()):
                if t[0] == "mem":
                    mem[f"{b}|{name}|{t[1]}"] = {"bpv_acct": float(t[2]), "bpv_heap": float(t[3])}
                elif t[0] == "poolbytes":
                    mem[f"{b}|{name}|poolbytes"] = int(t[1])
    return mem


def ir(b, job, op, n, tmp):
    """callgrind Ir per operation, in loop_<op> only: the run with 2 passes less the run with 1, over n"""
    v = []
    for r in (1, 2):
        f = tmp / "cg.out"
        p = subprocess.run(["valgrind", "--tool=callgrind", f"--callgrind-out-file={f}", f"--toggle-collect=loop_{op}",
                            str(BIN / b), "time", str(job), op, str(r), "1"], capture_output=True)
        if p.returncode != 0:
            return None
        v.append(int(re.search(r"^(?:summary|totals): (\d+)", f.read_text(), re.M).group(1)))
    return (v[1] - v[0]) / n


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
    ap.add_argument("--jobs", default="", help="comma-separated job names (default all), 'growth' for the workload")
    ap.add_argument("--ops", default="", help="comma-separated operations (default all)")
    ap.add_argument("--ir", action="store_true", help="callgrind Ir per operation beside the times")
    ap.add_argument("--no-check", action="store_true", help="skip the correctness gate (toy only)")
    a = ap.parse_args()
    min_ns, reps = (2_000_000, 3) if a.toy else (MIN_NS, REPS)
    out = HERE / "build" / "r2" / ("toy" if a.toy else "results")
    out.mkdir(parents=True, exist_ok=True)
    t0 = time.monotonic()
    if not (a.toy and a.no_check):
        p = subprocess.run([sys.executable, str(HERE / "test" / "r2check.py")] + (["--quick"] if a.toy else []),
                           capture_output=True, text=True)
        log("check: " + p.stdout.strip().splitlines()[-1])
        if p.returncode != 0:
            print(p.stdout)
            sys.exit("correctness check failed")
    work = HERE / "build" / "r2" / "work"
    tjobs, (gpath, x0, yz) = r2gen.generate(work)
    want_jobs = [j for j in a.jobs.split(",") if j] or list(tjobs) + ["growth"]
    want_ops = [o for o in a.ops.split(",") if o] or list(r2gen.OPS)
    blocks = [(j, o) for j in want_jobs if j != "growth" for o in want_ops] + \
             ([("growth", "growth")] if "growth" in want_jobs else [])
    jobs = {j: tjobs[j][0] for j in want_jobs if j != "growth"}
    mem = memory(jobs, gpath)
    log(f"memory: {len(mem)} cells")
    expect = {}
    for j, o in blocks:
        if j == "growth":
            expect[(j, o)] = r2gen.checksum("growth", r2gen.growth_exact(x0, yz))
        else:
            pairs = tjobs[j][1][o]
            expect[(j, o)] = r2gen.checksum(o, [r2gen.exact(o, x.v, y.v) for x, y in pairs])
    irs = {}
    if a.ir:
        for j, o in blocks:
            path, n = (gpath, len(yz)) if j == "growth" else (tjobs[j][0], len(tjobs[j][1][o]))
            for b in BUILDS:
                irs[f"{b}|{j}|{o}"] = ir(b, path, o, n, out)
        log(f"callgrind: {sum(v is not None for v in irs.values())} cells")
    timing = {}
    for j, o in blocks:
        path, n = (gpath, len(yz)) if j == "growth" else (tjobs[j][0], len(tjobs[j][1][o]))
        idle = wait_idle(a.toy)
        t = {row: time_cell(row, path, o, n, min_ns, reps) for row in ROWS}
        c1, c2 = t["control"].get("median"), t["control-2"].get("median")
        noise = abs(c1 - c2) / min(c1, c2) if c1 and c2 else None
        sums = {x.get("sum") for x in t.values()}
        agree = sums == {expect[(j, o)]}
        timing[f"{j}|{o}"] = {"idle": idle, "rows": t, "noise": noise, "sums_agree": agree}
        log(f"{j} {o}: noise {noise if noise is None else round(noise, 4)}; " + ", ".join(
            f"{r} {x['median']:.4g} ns" if "median" in x else f"{r} {x['trap']}" for r, x in t.items())
            + ("" if agree else "; CHECKSUMS DIFFER"))
    pairs = []
    for j, o in blocks:
        tm = timing[f"{j}|{o}"]
        pairs += [dict(job=j, op=o, ref="control", row=b, **compare(tm["rows"], "control", b, tm["noise"] or 0))
                  for b in BUILDS[1:]]
    for p in pairs:
        ma, mb = mem.get(f"control|{p['job']}|{p['op']}"), mem.get(f"{p['row']}|{p['job']}|{p['op']}")
        p["bytes_ratio"] = mb["bpv_acct"] / ma["bpv_acct"] if ma and mb else None
    res = {"toy": a.toy, "reps": reps, "min_ns": min_ns, "timing": timing, "memory": mem, "ir": irs,
           "pairings": pairs}
    (out / "results.json").write_text(json.dumps(res, indent=1))
    L = [f"# 13-header round 2 timing{' (TOY: not a measurement)' if a.toy else ''}", "",
         f"{reps} reps per cell; time per operation (growth: per step, one multiply and one add). Bytes per value: "
         "accounted (Control's malloc usable size beside).", "",
         "| job | op | row | ns/op | spread | Ir/op | bytes/value acct (heap) | sums |",
         "| --- | --- | --- | --- | --- | --- | --- | --- |"]
    for j, o in blocks:
        tm = timing[f"{j}|{o}"]
        for r in ROWS:
            x = tm["rows"][r]
            b = "control" if r == "control-2" else r
            m = mem.get(f"{b}|{j}|{o}")
            irv = irs.get(f"{b}|{j}|{o}")
            L.append(f"| {j} | {o} | {r} | " + (f"{x['median']:.4g} | {x['spread']:.1%}" if "median" in x
                                                 else f"{x['trap']} | -") +
                     f" | {'-' if irv is None else round(irv, 1)} | "
                     + (f"{m['bpv_acct']:.1f} ({m['bpv_heap']:.1f})" if m else "-")
                     + f" | {'agree' if tm['sums_agree'] else 'DIFFER'} |")
    L += ["", "| job | op | row x Control | T(Control)/T(row) | band | time | bytes ratio |",
          "| --- | --- | --- | --- | --- | --- | --- |"]
    for p in pairs:
        L.append(f"| {p['job']} | {p['op']} | {p['row']} | {'-' if p['ratio'] is None else round(p['ratio'], 3)} | "
                 f"{'-' if p.get('h') is None else round(p['h'], 3)} | {p['verdict']} | "
                 f"{'-' if p['bytes_ratio'] is None else round(p['bytes_ratio'], 3)} |")
    (out / "report.md").write_text("\n".join(L) + "\n")
    log(f"done in {time.monotonic() - t0:.0f} s: {out / 'report.md'}")


if __name__ == "__main__":
    main()
