"""The paper's library against 10-interning's textbook control (TC), for correctness only: nothing is timed.

Round 2's own generator (10-interning/test/r2_gen.py) writes the small (toy) workloads of the jobs it runs into a
temporary directory; TC is built there from its own sources with round 2's own compiler commands
(r2_libconf.build_commands: the counting build only), and run once per job in its untimed `count` mode, as round 2's
self-tests run it. The paper's library runs the same workload: an entered pair p, q is stored as
reduce(rational(p, q)), and each operation is applied to the stored values of earlier calls. Every call's value must
be TC's, integer for integer, and the generator's exact reference.

Formats (10-interning/test/bench_r2.c): a workload is "SKWLOAD1", six int32, the number of calls and of entries as
int64, then int64 op[N], a[N], b[N] (op 0 enters a/b; 1 to 4 apply add, subtract, multiply, divide to the values of
calls a and b). A read-out, and the reference, are int64 num[N] then int64 den[N].
"""
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TEST = next((p for p in (ROOT / "10-interning" / "test", ROOT / "archive" / "10-interning" / "test") if p.is_dir()),
            ROOT / "10-interning" / "test")     # 10-interning now lives in archive/
ENTER, OPS = 0, {1: "add", 2: "subtract", 3: "multiply", 4: "divide"}


def read_workload(path):
    data = path.read_bytes()
    assert data[:8] == b"SKWLOAD1", path
    n, _ = struct.unpack_from("<qq", data, 8 + 24)
    off = 8 + 24 + 16
    op, a, b = (list(struct.unpack_from(f"<{n}q", data, off + 8 * n * i)) for i in range(3))
    return op, a, b


def read_pairs(path):
    data = path.read_bytes()
    n = len(data) // 16
    num = struct.unpack_from(f"<{n}q", data, 0)
    den = struct.unpack_from(f"<{n}q", data, 8 * n)
    return list(zip(num, den))


def run_paper(ns, op, a, b):
    vals = []
    for o, x, y in zip(op, a, b):
        if o == ENTER:
            vals.append(ns["reduce"](ns["rational"](x, y)))
        else:
            vals.append(ns[OPS[o]](vals[x], vals[y]))
    return vals


def compare(ns, check):
    sys.path.insert(0, str(TEST))
    import r2_common as C
    import r2_gen
    import r2_libconf as L

    print("\nAgainst 10-interning's textbook control (TC), untimed, on round 2's toy workloads:")
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        work, bindir = t / "work", t / "bin"
        r2_gen.generate(work, list(C.RUN_JOBS), toy=True)
        want = {bindir / "obj" / "counting" / f"{o}.o" for o in L.ROW_OBJECTS["TC"]} | {bindir / "bench-TC-counting"}
        for out, cmd in L.build_commands(bindir):
            if out in want:
                out.parent.mkdir(parents=True, exist_ok=True)
                r = subprocess.run(cmd, capture_output=True, text=True)
                if not check(r.returncode == 0, f"building TC: {' '.join(map(str, cmd))}\n{r.stderr}"):
                    return
        exe = bindir / "bench-TC-counting"
        for job in C.RUN_JOBS:
            wl, ref = work / f"job-{job:02d}.wl", work / f"job-{job:02d}.ref"
            out, percall = t / f"tc-{job:02d}.out", t / f"tc-{job:02d}.percall"
            r = subprocess.run([str(exe), "count", str(wl), str(out), str(percall)], capture_output=True, text=True)
            if not check(r.returncode == 0 and "ok impl=TC" in r.stdout, f"job {job}: TC failed: {r.stdout[-300:]}"
                                                                         f"{r.stderr[-300:]}"):
                continue
            op, a, b = read_workload(wl)
            tc, exact = read_pairs(out), read_pairs(ref)
            paper = run_paper(ns, op, a, b)
            diff_tc = [i for i, (p, q) in enumerate(zip(paper, tc)) if p != q]
            diff_ref = [i for i, (p, q) in enumerate(zip(paper, exact)) if p != q]
            check(len(paper) == len(tc) == len(exact), f"job {job}: lengths {len(paper)}, {len(tc)}, {len(exact)}")
            check(not diff_tc, f"job {job}: differs from TC at {len(diff_tc)} calls, first {diff_tc[:1]}")
            check(not diff_ref, f"job {job}: differs from the reference at {len(diff_ref)} calls")
            kinds = sorted({OPS[o] for o in op if o != ENTER})
            print(f"  job {job:2d} {C.JOB_NAMES[job]:14s} {len(op):4d} calls ({', '.join(kinds)}): "
                  f"{len(paper) - len(diff_tc)} of {len(tc)} identical to TC, "
                  f"{len(paper) - len(diff_ref)} of {len(exact)} to the reference")
