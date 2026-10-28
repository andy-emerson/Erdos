#!/usr/bin/env python3
"""A short parity check of the library against round three's R0 and D1 binaries.

    python3 bench/compare.py [--bin build/bench/pg_bench] [--rounds 3] [--only W2,N100] [--modes v3,v4]

Workloads (round three's data, test/ablation/build/r3-data, all the a*b + c job): W2 and W8 (decimal elementwise),
N100 and N10k (100 and 10,000 distinct denominators), P64 (two-large-prime denominators).  Each round runs every row
once, in an order rotated per round; each invocation runs --reps warm reps and reports the minimum of each phase
(entry, op, red: round three's driver format).  The table gives medians over rounds in ms and the ratio of R0's (and
D1's) total to the library's (above 1: the library is faster).  Every row's output is compared with R0's, line by
line.  This is a quick look, not round three's pre-registered rule: a few rounds, no bootstrap, no noise band.
"""
import argparse
import os
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent  # 07-prime-graph
R3 = ROOT / "test" / "ablation" / "build" / "r3"
DATA = ROOT / "test" / "ablation" / "build" / "r3-data"
WORKLOADS = {"W2": 3, "W8": 3, "N100": 3, "N10k": 1, "P64": 3}  # name: reps per invocation (R0 at N10k: seconds)


def run(args, env=None):
    r = subprocess.run(args, capture_output=True, text=True, env=env, timeout=3600)
    if r.returncode != 0:
        raise RuntimeError(f"{args[0]} failed ({r.returncode}): {r.stderr.strip()}")
    st = {}
    for tok in r.stdout.split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            st[k] = v
    return st


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default=str(HERE.parent / "build" / "bench" / "pg_bench"))
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--only", default=",".join(WORKLOADS))
    ap.add_argument("--modes", default="v3,v4", help="library rows: v3 (each op reduced), v4 (inside an expression)")
    a = ap.parse_args()
    for b in ("abl_r0", "abl_d1"):
        if not (R3 / b).exists():
            sys.exit(f"compare.py: {R3 / b} not found (round three's binaries)")
    tmp = Path(tempfile.mkdtemp(prefix="pg_compare_"))
    env = dict(os.environ, AB_V3_POLICY="lcm")
    rows = ["r0", "d1"] + [f"lib-{m}" for m in a.modes.split(",")]
    print(f"{'job':<10} {'row':<7} {'entry':>8} {'op':>8} {'red':>8} {'total':>8}  {'R0/row':>6} {'D1/row':>6}  check")
    for wl in a.only.split(","):
        d = DATA / wl
        reps = WORKLOADS.get(wl, 3)
        res = {r: [] for r in rows}
        for rnd in range(a.rounds):
            order = rows[rnd % len(rows):] + rows[:rnd % len(rows)]
            for r in order:
                out = tmp / f"{wl}-{r}.txt"
                if r.startswith("lib"):
                    st = run([a.bin, "fma", str(d), str(out), str(reps), r.split("-")[1]])
                else:
                    st = run([str(R3 / f"abl_{r}"), "fma", str(d), str(out), str(reps)], env)
                res[r].append(st)
        ref = (tmp / f"{wl}-r0.txt").read_bytes()
        med = lambda r, k: statistics.median(int(s[k]) for s in res[r]) / 1e6
        t0, t1 = med("r0", "total"), med("d1", "total")
        for r in rows:
            same = (tmp / f"{wl}-{r}.txt").read_bytes() == ref
            tot = med(r, "total")
            print(f"{wl + ' fma':<10} {r:<7} {med(r, 'entry'):8.2f} {med(r, 'op'):8.2f} {med(r, 'red'):8.2f} {tot:8.2f}"
                  f"  {t0 / tot:6.2f} {t1 / tot:6.2f}  {'same as R0' if same else 'DIFFERS from R0'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
