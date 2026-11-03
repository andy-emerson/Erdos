"""Instruction counts (callgrind Ir) of the Nim build against the C build (README.md, "Against C"), one run at a time,
on m3's workloads: `bench time <wl> <ro> 1 1` (the timing build) under valgrind --tool=callgrind, with the inclusive
Ir of the API functions, by test/DECISIONS-m4.md's method: build (per input) = (enter + the inputs' share of create)
/ inputs; op = madd + mmul + the results' share of create; create split evenly per call. No wall clock.

    python3 ir.py [--ops add,mul,sop,pos] [--jobs 2,6,10] [--out FILE]

Rows: Nim Control (NCTL) / C TC with the m4 gcd (TC), Nim Packed (NPK) / C mh4 (MHV4).
"""
import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
NIM = HERE.parent / "build" / "bin"
CB = HERE.parents[1] / "test" / "build" / "m4" / "m4"
WORK = HERE.parents[1] / "test" / "build" / "m3"
ROWS = (("NCTL", NIM, "nctl_"), ("TC", CB, "tcm_"), ("NPK", NIM, "npk_"), ("MHV4", CB, "mhv4_"))
NIN = {"add": 2, "mul": 2, "sop": 4, "pos": 4}
NSTEP = {"add": 1, "mul": 1, "sop": 3, "pos": 3}


def inclusive(exe, wl, prefix):
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "cg.out"
        subprocess.run(["valgrind", "--tool=callgrind", f"--callgrind-out-file={out}", str(exe), "time", str(wl),
                        str(Path(tmp) / "ro"), "1", "1"], check=True, capture_output=True)
        txt = subprocess.run(["callgrind_annotate", "--inclusive=yes", "--threshold=100", str(out)],
                             check=True, capture_output=True, text=True).stdout
    ir = {}
    for ln in txt.splitlines():
        m = re.match(r"\s*([\d,]+)\s+\(\s*[\d.]+%\)\s+\S*:(\w+)\b", ln) or re.match(r"\s*([\d,]+)\s+\S*:(\w+)\b", ln)
        if m and m.group(2).startswith(prefix):
            ir[m.group(2)[len(prefix):]] = ir.get(m.group(2)[len(prefix):], 0) + int(m.group(1).replace(",", ""))
    return ir


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ops", default="add,mul,sop,pos")
    ap.add_argument("--jobs", default="2,6,10")
    ap.add_argument("--out")
    a = ap.parse_args()
    res = {}
    for op in a.ops.split(","):
        for job in [int(j) for j in a.jobs.split(",")]:
            wl = WORK / f"work-{op}" / f"job-{job:02d}.wl"
            if not wl.exists():
                continue
            r = {}
            for row, d, prefix in ROWS:
                ir = inclusive(d / f"bench-{row}-timing", wl, prefix)
                k, s = NIN[op], NSTEP[op]
                create = ir.get("create", 0) / (k + s)
                r[row] = {"build": (ir.get("enter", 0) + k * create) / k,
                          "op": ir.get("madd", 0) + ir.get("mmul", 0) + s * create, "raw": ir}
            res[f"{op}-{job}"] = r
            line = f"{op}-{job}:"
            for nim, c in (("NCTL", "TC"), ("NPK", "MHV4")):
                line += (f"  {nim}/{c} build {r[nim]['build'] / r[c]['build']:.3f} op {r[nim]['op'] / r[c]['op']:.3f}"
                         f" ({r[nim]['op']:,.0f} / {r[c]['op']:,.0f})")
            print(line, flush=True)
    if a.out:
        Path(a.out).write_text(json.dumps(res, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
