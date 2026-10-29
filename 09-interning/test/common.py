"""Shared constants of the layer-1 harness: the run plan of research/rule.md, paths, rows and file formats.

Nothing here is measured; gen.py, runner.py and analyze.py import it. Adapted from 08-skeletons/test/common.py.
"""
import hashlib
import struct
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent              # 09-interning/test
ROOT = HERE.parent                                   # 09-interning
RULE_FILE = ROOT / "research" / "rule.md"
BUILD = HERE / "build"
RESULTS = HERE / "results"

# sha256 of research/rule.md as fixed by the author, Oct 29, 2026 (every PROPOSED mark approved), with the procedural
# revision of the same day (the idle-machine check; rule.md, top). The harness refuses a measured run unless the file
# on disk has exactly this hash, so it holds the rule verbatim (rule.md, line 3).
RULE_SHA256 = "eb631a463d99f14dd278c08dc95a9075099ed8f012b906c9da7e195b5ccbd26d"

MASTER_SEED = 20261029
BOOT = 10_000
BOOT_SEED = 20261029
ROUNDS = 12
PILOT_ROUNDS = 2
CALIB_MIN_NS = 50_000_000          # r: smallest power of two whose rep lasts >= 50 ms
ONE_REP_NS = 1_000_000_000         # calibration rep >= 1 s: 1 rep per invocation, else 3 (first discarded)
CAP_NS = 10_000_000_000            # a timed rep over 10 s: over cap
COUNT_LIMIT_S = 600                # every counting or memory run: 10 minutes
HARD_STOP_S = 60 * 60              # the whole run: 60 minutes
PILOT_LIMIT_S = 45 * 60            # the pilot's projection limit
PILOT_REMAINING_S = 8.5 * 60       # the remaining work's upper estimate (rule, Run plan step 3)
PILOT_NOMINAL_S = 60.0             # the pilot's nominal wall time (1 minute)
BAND_MIN = 0.05
FIXED_WIN, FIXED_LOSS, SEVERE = 0.90, 1.10, 2.0
TIMED_CORE, HARNESS_CORE = 1, 0
COUNT_REPEAT_JOBS = (3, 5, 10)
PILOT_JOBS = (1, 3, 5, 7, 10)
FORM_CALIB_INVOCATIONS = 3         # per form, alternating A, B, A, B, A, B

# Idle-machine check before any timed run (test/DECISIONS.md R-13; rule.md, revision of Oct 29, 2026), as 08's R-p:
# the 1-minute load average must be at most IDLE_LOAD_MAX, and no other process may use more than IDLE_PROC_CPU_MAX of
# one core over a sample of IDLE_SAMPLE_S. If the machine is busy, the runner re-checks every IDLE_POLL_S for up to
# IDLE_WAIT_S, logging each check, then refuses (RunStop).
IDLE_LOAD_MAX = 0.5
IDLE_PROC_CPU_MAX = 0.05
IDLE_SAMPLE_S = 1.0
IDLE_POLL_S = 15.0
IDLE_WAIT_S = 600.0

# Ops codes of the workload file. Every call stores one value; its handle is its call index.
ENTER, ADD, SUB, MUL, DIV = 0, 1, 2, 3, 4
OPNAME = {ENTER: "enter", ADD: "add", SUB: "sub", MUL: "mul", DIV: "div"}
PHASES = ("entry", "add", "sub", "mul", "div")

# The cases of spec §7, numbered as lib/include/l1.h (l1_case).
CASES = ("add_cross", "add_den", "add_num", "add_general", "mul_both", "mul_den", "mul_num", "mul_cross_ad",
         "mul_cross_cb", "mul_general", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den",
         "div_general")
CASE = {n: i for i, n in enumerate(CASES)}
CASES_OF = {"add": CASES[0:4], "sub": CASES[0:4], "mul": CASES[4:10], "div": CASES[10:16]}
O1_CASES = ("add_den", "mul_cross_ad", "mul_cross_cb", "div_num", "div_den")

# Implementations (binaries) and rows. L1-null is the L1 binary run as its own row.
IMPLS = ("L1", "L1opt", "ctl", "fmpq", "mpq", "fz")
COUNTED = ("L1", "L1opt", "ctl")                    # counting build, test build, gates
LIBRARIES = ("fmpq", "mpq", "fz")
FORMED = ("fmpq", "mpq")                            # forms A and B, calibrated per job
ROW_IMPL = {"L1": "L1", "L1-opt": "L1opt", "L1-null": "L1", "ctl": "ctl", "fmpq": "fmpq", "mpq": "mpq", "fz": "fz"}
IMPL_ROWS = {"L1": ("L1", "L1-null"), "L1opt": ("L1-opt",), "ctl": ("ctl",), "fmpq": ("fmpq",), "mpq": ("mpq",),
             "fz": ("fz",)}


def job_table():
    """The 11 jobs of the run plan, numbered as in rule.md."""
    J = [
        (1, "MONEY-wide", "F1", "MONEY", "addsub", True),
        (2, "MONEY-catalog", "F1", "MONEY", "addsub", True),
        (3, "SMALL8", "F2", "SMALL", "muldiv", False),
        (4, "SMALL64", "F2", "SMALL", "muldiv", False),
        (5, "MOD-mixed", "F3", "MOD", "mixed", False),
        (6, "MOD-muldiv", "F3", "MOD", "muldiv", False),
        (7, "NEW-add", "F4", "NEW", "add", False),
        (8, "CH-money", "F5", "CHAINS", "addsub", True),
        (9, "CH-closed", "F5", "CHAINS", "addsub", False),
        (10, "CH-small", "F5", "CHAINS", "muldiv", False),
        (11, "CH-mixed", "F5", "CHAINS", "mixed", False),
    ]
    jobs = {}
    for n, name, fam, stratum, mix, fz in J:
        d = dict(num=n, name=name, family=fam, stratum=stratum, mix=mix, fz=fz, width=64)
        if fam == "F5":
            d.update(chains=1000, K=32 if n in (8, 9) else 8)
            d["nops"] = 1000 * d["K"]
        else:
            d["nops"] = 20_000
        jobs[n] = d
    return jobs


JOBS = job_table()
STRATA = {"MONEY": (1, 2), "SMALL": (3, 4), "MOD": (5, 6), "NEW": (7,), "CHAINS": (8, 9, 10, 11)}
STRATA_ORDER = ("MONEY", "SMALL", "MOD", "NEW", "CHAINS")


def rows_of(job):
    r = ["L1", "L1-opt", "L1-null", "ctl", "fmpq", "mpq"]
    if JOBS[job]["fz"]:
        r.append("fz")
    return r


def impls_of(job):
    return [i for i in IMPLS if i != "fz" or JOBS[job]["fz"]]


def rule_sha256(path=None):
    return hashlib.sha256(Path(path or RULE_FILE).read_bytes()).hexdigest()


# ------------------------------------------------------------------------------------------------- file formats
# workload  job-NN.wl  : header (8s magic, 6 int32, 2 int64), then int64 op[ncalls], int64 a[ncalls], int64 b[ncalls].
#           op ENTER: a = numerator, b = denominator (> 0); otherwise a, b = handles (call indices) of the operands.
#           Header int32s: job, width, fz (1 on the fz jobs), nchains, K, 0.  (08's format.)
# reference job-NN.ref : int64 num[ncalls], int64 den[ncalls], the exact stored value of every call, lowest terms.
# cases     job-NN.case: int8 per call, the case of spec §7 predicted from magnitudes (-1 for an entry).
# read-out  (bench)    : int64 num[N], int64 den[N] in handle order.
# per call  (bench count): int64[N][5]: the core counts gcd, gcd_work, div_exact, mul, add of each call.
# wholes    (bench gate) : int64 W[], the whole-number list in index order (L1, L1-opt).
WL_MAGIC = b"SKWLOAD1"
WL_HEAD = struct.Struct("<8s6i2q")
PERCALL_FIELDS = ("gcd", "gcd_work", "div_exact", "mul", "add")


def write_workload(path, job, width, fz, nchains, K, n_in, op, a, b):
    with open(path, "wb") as f:
        f.write(WL_HEAD.pack(WL_MAGIC, job, width, int(fz), nchains, K, 0, len(op), n_in))
        for arr in (op, a, b):
            f.write(np.asarray(arr, dtype="<i8").tobytes())


def read_workload(path):
    data = Path(path).read_bytes()
    magic, job, width, fz, nchains, K, _, n, n_in = WL_HEAD.unpack_from(data, 0)
    assert magic == WL_MAGIC, path
    arr = np.frombuffer(data, dtype="<i8", offset=WL_HEAD.size).reshape(3, n)
    return dict(job=job, width=width, fz=fz, nchains=nchains, K=K, n=n, n_in=n_in, op=arr[0], a=arr[1], b=arr[2])


def write_pairs(path, num, den):
    with open(path, "wb") as f:
        f.write(np.asarray(num, dtype="<i8").tobytes())
        f.write(np.asarray(den, dtype="<i8").tobytes())


def read_pairs(path):
    arr = np.fromfile(path, dtype="<i8")
    n = len(arr) // 2
    return arr[:n], arr[n:]
