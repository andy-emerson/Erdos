"""run.py: the Nim pilot's comparison, C Control against its Nim port (REPORT.md). Nothing timed.

    python3 run.py [--jobs 1,2,...] [--ops add,mul]

For each of m3's add and mul jobs (spec/test/build/m3/work-<op>/job-NN.wl, made by m3_gen.py):
  1. values: one run of each timing binary (`bench time <wl> <ro> 1 1`); the read-outs must be identical byte for byte,
     to each other and to the job's exact reference (job-NN.ref, Python's fractions);
  2. instruction counts: the same run under valgrind --tool=callgrind, one run at a time; inclusive Ir of the API
     functions, as DECISIONS-m4.md measured them: build = (enter + the inputs' share of create) per input,
     op = madd/mmul + the result's share of create, create split evenly per call;
  3. memory: one run of each memory binary (`bench mem`): the accounted and the heap peaks (memrun lines).
Writes results/ir.csv, results/mem.csv, results/values.csv beside this file.
"""
import argparse
import csv
import re
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
WORK = HERE.parent / "test" / "build" / "m3"
B = HERE / "build"
ROWS = {"C": B / "c" / "bench-TC", "Nim": B / "nim" / "bench-TCnim",
        "Nim-exc": B / "nimexc" / "bench-TCnim", "Nim-O3": B / "nim3" / "bench-TCnim",
        "Nim-if": B / "nimif" / "bench-TCnim"}
MEMROWS = ("C", "Nim")
OUT = HERE / "results"
API = re.compile(r"\s*([\d,]+) \([^)]*\)\s+\S*?:?(tcm_(?:create|enter|madd|mmul))(?: \[|$)")


def ir(exe, wl):
    with tempfile.TemporaryDirectory() as t:
        cg = Path(t) / "cg"
        subprocess.run(["valgrind", "--tool=callgrind", f"--callgrind-out-file={cg}", str(exe), "time", str(wl),
                        str(Path(t) / "ro"), "1", "1"], capture_output=True, check=True)
        ann = subprocess.run(["callgrind_annotate", "--threshold=100", "--inclusive=yes", str(cg)],
                             capture_output=True, text=True, check=True).stdout
    total = int(re.search(r"([\d,]+) \(100\.0%\)\s+PROGRAM TOTALS", ann).group(1).replace(",", ""))
    f = {}
    for ln in ann.splitlines():
        m = API.match(ln)
        if m:
            k = m.group(2).split("_", 1)[1]
            f[k] = f.get(k, 0) + int(m.group(1).replace(",", ""))
    cper = f["create"] / 3          # two inputs and one result
    return total, (f["enter"] + 2 * cper) / 2, f.get("madd", 0) + f.get("mmul", 0) + cper


def mem(exe, wl):
    with tempfile.TemporaryDirectory() as t:
        out = subprocess.run([str(exe), "mem", str(wl), str(Path(t) / "ro")], capture_output=True, text=True,
                             check=True).stdout
    r = {}
    for ln in out.splitlines():
        if ln.startswith("memrun "):
            w = ln.split()
            kv = dict(x.split("=") for x in w[2:])
            r[w[1]] = kv
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", default="1,2,3,4,5,6,7,8,9,10,11,12")
    ap.add_argument("--ops", default="add,mul")
    a = ap.parse_args()
    jobs = [int(x) for x in a.jobs.split(",")]
    ops = a.ops.split(",")
    OUT.mkdir(exist_ok=True)
    with open(OUT / "values.csv", "w", newline="") as fv, open(OUT / "ir.csv", "w", newline="") as fi, \
            open(OUT / "mem.csv", "w", newline="") as fm:
        wv, wi, wm = csv.writer(fv), csv.writer(fi), csv.writer(fm)
        wv.writerow(["op", "job", "n", "cells", "identical_to_ref"] + [f"{r}_equals_C" for r in ROWS if r != "C"])
        wi.writerow(["op", "job", "n", "row", "total", "build_per_input", "op_ir"])
        wm.writerow(["op", "job", "n", "row", "build_peak_acct", "build_peak_heap", "op_peak_acct", "op_peak_heap",
                     "end_acct", "end_heap"])
        for op in ops:
            for job in jobs:
                wl = WORK / f"work-{op}" / f"job-{job:02d}.wl"
                ref = (WORK / f"work-{op}" / f"job-{job:02d}.ref").read_bytes()
                n = int((len(ref) // 16) ** 0.5)
                # 1. values
                ro = {}
                with tempfile.TemporaryDirectory() as t:
                    for r, exe in ROWS.items():
                        p = Path(t) / r
                        out = subprocess.run([f"{exe}-timing", "time", str(wl), str(p), "1", "1"], capture_output=True,
                                             text=True, check=True).stdout
                        assert out.rstrip().splitlines()[-1].startswith("ok "), out
                        ro[r] = p.read_bytes()
                same = [ro[r] == ro["C"] for r in ROWS if r != "C"]
                wv.writerow([op, job, n, n * n, ro["C"] == ref] + same)
                assert ro["C"] == ref and all(same), f"{op} job {job}: values differ"
                # 2. instruction counts, one run at a time
                for r, exe in ROWS.items():
                    tot, bld, opir = ir(f"{exe}-timing", wl)
                    wi.writerow([op, job, n, r, tot, round(bld), round(opir)])
                    print(op, job, r, tot, round(bld), round(opir), flush=True)
                # 3. memory
                for r in MEMROWS:
                    m = mem(f"{ROWS[r]}-memory", wl)
                    bpa = max(int(m[f"built_{x}"]["peak_acct"]) - int(m[f"built_{x}"]["base_acct"]) for x in "AB")
                    bph = max(int(m[f"built_{x}"]["peak_heap"]) - int(m[f"built_{x}"]["base_heap"]) for x in "AB")
                    wm.writerow([op, job, n, r, bpa, bph, m["op"]["peak_acct"], m["op"]["peak_heap"],
                                 m["end"]["acct"], m["end"]["heap"]])
                fv.flush(), fi.flush(), fm.flush()
    print("done")


if __name__ == "__main__":
    main()
