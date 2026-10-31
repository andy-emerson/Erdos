"""run.py: the timing harness. For each job x operation (one block):
- wait until the machine is idle (CPU usage and load low), retrying;
- for each row, pinned to core 1 with taskset: calibrate r so that one rep (r passes over the 1024 precomputed
  operands) takes 50-100 ms, then 15 reps; the median time per operation and the spread (max - min) / median.
Rows: the five builds, and Control again as a sixth row; the noise is the difference of Control's two medians, and a
difference between builds counts only if it is larger than max(5%, that noise). Every result is stored and
checksummed after the clock (in the binary). Beside the times: the memory build's bytes (spec §4) and callgrind Ir.

  python3 test/run.py --toy     tiny and fast (N = 32, 3 reps, 2 ms reps), not a measurement
  python3 test/run.py           the real round: quiet machine and the author's go-ahead only
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
import check  # noqa: E402
import gen  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
BIN = HERE / "build" / "bin"
BUILDS = check.BUILDS
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
        cpu, load = cpu_busy(0.2 if toy else 1.0), os.getloadavg()[0]
        if (cpu <= IDLE_CPU and load <= IDLE_LOAD) or toy:
            return {"cpu": round(cpu, 3), "load": load}
        if time.monotonic() - t > IDLE_GIVEUP_S:
            sys.exit(f"machine not idle for {IDLE_GIVEUP_S} s (cpu {cpu:.0%}, load {load})")
        log(f"busy (cpu {cpu:.0%}, load {load}); retrying in {IDLE_RETRY_S} s")
        time.sleep(IDLE_RETRY_S)


def exe(row, k):
    b = "control" if row == "control-2" else row
    return BIN / (f"{b}-j1" if k == 1 else b)


def bench(row, k, op, work, *args):
    p = subprocess.run(["taskset", "-c", CORE, str(exe(row, k)), args[0], str(work / f"job-{k}.bin"), op,
                        *map(str, args[1:])], capture_output=True, text=True)
    return p.returncode, p.stdout.split("\n")


def time_cell(row, k, op, work, n, min_ns, reps):
    rc, out = bench(row, k, op, work, "calib", min_ns)
    if rc != 0:
        return {"trap": f"exit {rc}"}
    r = int(out[0].split()[1])
    rc, out = bench(row, k, op, work, "time", r, reps)
    ts = [int(l.split()[2]) / (r * n) for l in out if l.startswith("rep ")]
    med = statistics.median(ts)
    return {"r": r, "ns": ts, "median": med, "spread": (max(ts) - min(ts)) / med,
            "sum": next(l.split()[1] for l in out if l.startswith("sum "))}


def memory(work):
    mem = {}
    for b in BUILDS:
        for k in gen.JOBS:
            out = subprocess.run([str(BIN / f"{b}-mem"), "mem", str(work / f"job-{k}.bin")], capture_output=True,
                                 text=True, check=True).stdout
            for t in (l.split() for l in out.splitlines()):
                mem[f"{b}|{k}|{t[1]}"] = {"bpv_acct": float(t[2]), "bpv_heap": float(t[3]),
                                          "peak_acct": float(t[4])}
    return mem


def ir(b, k, op, work, n, tmp):
    """callgrind Ir per operation, in loop_<op> only: the run with 2 passes less the run with 1, over N"""
    v = []
    for r in (1, 2):
        f = tmp / "cg.out"
        p = subprocess.run(["valgrind", "--tool=callgrind", f"--callgrind-out-file={f}", f"--toggle-collect=loop_{op}",
                            str(exe(b, k)), "time", str(work / f"job-{k}.bin"), op, str(r), "1"], capture_output=True)
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
    ap.add_argument("--toy", action="store_true")
    a = ap.parse_args()
    n, min_ns, reps = (32, 2_000_000, 3) if a.toy else (gen.N, MIN_NS, REPS)
    out = HERE / "build" / ("toy" if a.toy else "results")
    work = out / "work"
    out.mkdir(parents=True, exist_ok=True)
    t0 = time.monotonic()
    gen.generate(work, n)
    cells = check.check(work, BIN, out / "check")
    for line in check.summary(cells):
        log("check: " + line)
    if any(c["failures"] for c in cells):
        sys.exit("correctness check failed")
    inexact = {f"{c['build']}|{c['job']}|{c['op']}": ("trap" if c.get("trap") else c["inexact"])
               for c in cells if c["W"] in (None, 64)}
    mem = memory(work)
    log(f"memory: {len(mem)} cells")
    irs = {f"{b}|{k}|{op}": (None if inexact[f"{b}|{k}|{op}"] == "trap" else ir(b, k, op, work, n, out))
           for b in BUILDS for k in gen.JOBS for op in gen.OPS}
    log(f"callgrind: {sum(v is not None for v in irs.values())} cells")
    timing = {}
    for k in gen.JOBS:
        for op in gen.OPS:
            idle = wait_idle(a.toy)
            t = {row: time_cell(row, k, op, work, n, min_ns, reps) for row in ROWS}
            c1, c2 = t["control"].get("median"), t["control-2"].get("median")
            noise = abs(c1 - c2) / min(c1, c2) if c1 and c2 else None
            timing[f"{k}|{op}"] = {"idle": idle, "rows": t, "noise": noise}
            log(f"job {k} {op}: idle {idle}; noise {noise if noise is None else round(noise, 4)}; " + ", ".join(
                f"{r} {x['median']:.3g} ns" if "median" in x else f"{r} {x['trap']}" for r, x in t.items()))
    pairs = []
    for k in gen.JOBS:
        for op in gen.OPS:
            tm = timing[f"{k}|{op}"]
            if k == 1:
                pairs += [dict(kind="build x Control", job=k, op=op, ref="control", row=b,
                               **compare(tm["rows"], "control", b, tm["noise"] or 0)) for b in BUILDS[1:]]
            else:
                pairs.append(dict(kind="Byte encoding x Growing", job=k, op=op, ref="growing", row="bytes",
                                  **compare(tm["rows"], "growing", "bytes", tm["noise"] or 0)))
    for b in BUILDS:
        for k in gen.JOBS:
            s, d = timing[f"{k}|sub"], timing[f"{k}|absdiff"]
            ts = {"sub": s["rows"][b], "absdiff": d["rows"][b]}
            pairs.append(dict(kind="absdiff x sub", job=k, op="-", ref="sub", row=f"{b} absdiff",
                              **compare(ts, "sub", "absdiff", max(s["noise"] or 0, d["noise"] or 0))))
    for p in pairs:
        ref = f"{p['ref'] if p['kind'] != 'absdiff x sub' else p['row'].split()[0]}"
        if p["kind"] == "absdiff x sub":
            ma, mb = mem[f"{ref}|{p['job']}|sub"], mem[f"{ref}|{p['job']}|absdiff"]
        else:
            ma, mb = mem[f"{p['ref']}|{p['job']}|{p['op']}"], mem[f"{p['row']}|{p['job']}|{p['op']}"]
        p["bytes_ratio"] = mb["bpv_acct"] / ma["bpv_acct"]
        p["peak_ratio"] = mb["peak_acct"] / ma["peak_acct"]
    res = {"toy": a.toy, "n": n, "reps": reps, "timing": timing, "memory": mem, "ir": irs, "inexact": inexact,
           "pairings": pairs}
    (out / "results.json").write_text(json.dumps(res, indent=1))
    L = [f"# 12-integers timing{' (TOY: not a measurement)' if a.toy else ''}", "",
         f"N = {n}, {reps} reps per cell. Word builds: 8 bytes per value and 8 peak, no allocation (trivial).", "",
         "| build | job | op | ns/op | spread | Ir/op | inexact | bytes/value acct (heap) | peak acct |",
         "| --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for k in gen.JOBS:
        for op in gen.OPS:
            for b in ROWS:
                x = timing[f"{k}|{op}"]["rows"][b]
                key = f"{'control' if b == 'control-2' else b}|{k}|{op}"
                m = mem[key]
                L.append(f"| {b} | {k} | {op} | " + (f"{x['median']:.3g} | {x['spread']:.1%}" if "median" in x
                                                     else "trap | -") +
                         f" | {irs[key] if irs[key] is None else round(irs[key], 1)} | {inexact[key]} | "
                         f"{m['bpv_acct']:.1f} ({m['bpv_heap']:.1f}) | {m['peak_acct']:.1f} |")
    L += ["", "| pairing | job | op | ref | row | T(ref)/T(row) | band | time | bytes ratio | peak ratio |",
          "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"]
    for p in pairs:
        L.append(f"| {p['kind']} | {p['job']} | {p['op']} | {p['ref']} | {p['row']} | "
                 f"{'-' if p['ratio'] is None else round(p['ratio'], 3)} | "
                 f"{'-' if p.get('h') is None else round(p['h'], 3)} | {p['verdict']} | "
                 f"{p['bytes_ratio']:.3f} | {p['peak_ratio']:.3f} |")
    (out / "report.md").write_text("\n".join(L) + "\n")
    log(f"done in {time.monotonic() - t0:.0f} s: {out / 'report.md'}")


if __name__ == "__main__":
    main()
