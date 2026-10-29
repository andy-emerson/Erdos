"""Shared constants of the skeleton harness: the run plan of research/rule.md, paths, rows and file formats.

Nothing here is measured; gen.py, runner.py and analyze.py import it.
"""
import hashlib
import struct
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent              # 08-skeletons/test
ROOT = HERE.parent                                   # 08-skeletons
RULE_FILE = ROOT / "research" / "rule.md"
BUILD = HERE / "build"
RESULTS = HERE / "results"

# sha256 of research/rule.md as fixed by the author, Oct 28, 2026, with the revision of Oct 29, 2026 (Designs 2
# and 1 redefined before any measurement: D2 on demand, D1's pair set and sparse incidence rows; rule.md line 5). The
# harness refuses a measured run unless the file on disk has exactly this hash, so it holds the rule verbatim (rule.md,
# line 3).
RULE_SHA256 = "ff2b62379f9e0b7451066ceb413c119fccb73259201d2e5a4165e55d3bcb2c96"

MASTER_SEED = 20261028
BOOT = 10_000
BOOT_SEED = 20261028
ROUNDS = 12
PILOT_ROUNDS = 2
CALIB_MIN_NS = 50_000_000          # r: smallest power of two whose rep lasts >= 50 ms
ONE_REP_NS = 1_000_000_000         # calibration rep >= 1 s: 1 rep per invocation, else 3 (first discarded)
CAP_NS = 10_000_000_000            # a timed rep over 10 s: over cap at this size
COUNT_LIMIT_S = 600                # every counting run: 10 minutes
HARD_STOP_S = 4 * 3600             # the whole run
BAND_MIN = 0.05
FIXED_WIN, FIXED_LOSS, SEVERE = 0.90, 1.10, 4.0
TIMED_CORE, HARNESS_CORE = 1, 0
COUNT_REPEAT_JOBS = (3, 9, 14, 18, 19, 23)
PILOT_TIMED_JOBS = (3, 6, 9, 12, 15, 18)
PILOT_GATE_JOB = 20

# Idle-machine check before any timed run (test/DECISIONS.md R-p): the 1-minute load average must be at most
# IDLE_LOAD_MAX, and no other process may use more than IDLE_PROC_CPU_MAX of one core over a sample of IDLE_SAMPLE_S.
# If the machine is busy, the runner re-checks every IDLE_POLL_S for up to IDLE_WAIT_S, then refuses (RunStop).
IDLE_LOAD_MAX = 0.5
IDLE_PROC_CPU_MAX = 0.05
IDLE_SAMPLE_S = 1.0
IDLE_POLL_S = 15.0
IDLE_WAIT_S = 600.0

LEVELS = ("ALL", "P75", "P50", "P25", "NONE", "LG")
SIGMA = {"ALL": 0.5, "P75": 0.75, "P50": 0.5, "P25": 0.25, "NONE": 0.0, "LG": None}
MIXES = ("add", "muldiv", "mixed")

# Ops codes of the workload file (call kinds). Every call stores one value; its handle is its call index.
ENTER, ADD, SUB, MUL, DIV = 0, 1, 2, 3, 4
OPNAME = {ENTER: "enter", ADD: "add", SUB: "sub", MUL: "mul", DIV: "div"}

# Implementations (binaries) and rows. ctl-null is the ctl binary run as its own row.
IMPLS = ("ctl", "D1", "D2", "ceil")
ROW_IMPL = {"ctl": "ctl", "ctl-null": "ctl", "D1": "D1", "D2": "D2", "ceil": "ceil"}
DESIGN_ROWS = {"ctl": ("ctl", "ctl-null"), "D1": ("D1",), "D2": ("D2",), "ceil": ("ceil",)}


def job_table():
    """The 27 jobs of the run plan, numbered as in rule.md."""
    jobs = {}
    for l, level in enumerate(LEVELS):
        for k, mix in enumerate(MIXES):
            n = 1 + 3 * l + k
            D = 1 if level == "ALL" else 1000
            jobs[n] = dict(num=n, name=f"{level}-{mix}", family="matrix", level=level, mix=mix, D=D, V=10_000 // D,
                           B=20, width=64, nops=20_000, ceil=(n == 1), stratum=level)
    jobs[19] = dict(num=19, name="ARR-arriving", family="arrival", level="P50", mix="mixed", D=10_000, V=1, B=20,
                    width=64, nops=20_000, ceil=False, stratum=None)
    jobs[20] = dict(num=20, name="ARR-early-twin", family="arrival", level="P50", mix="mixed", D=10_000, V=1, B=20,
                    width=64, nops=20_000, ceil=False, stratum=None)
    ch = [(21, "CH-closed", None), (22, "CH-all", "ALL"), (23, "CH-open", "P50"), (24, "CH-muldiv", "P50"),
          (25, "CH-none", "NONE")]
    for n, name, level in ch:
        K = 32 if name in ("CH-closed", "CH-all") else 8
        mix = {"CH-closed": "addsub", "CH-all": "addsub", "CH-open": "mixed", "CH-muldiv": "muldiv",
               "CH-none": "mixed"}[name]
        jobs[n] = dict(num=n, name=name, family="chains", level=level, mix=mix, K=K, chains=1000,
                       D=(100 if n >= 23 else None), V=(100 if n >= 23 else None), B=(12 if n >= 23 else None),
                       width=64, nops=1000 * K, ceil=(n == 22), stratum="CHAINS")
    jobs[26] = dict(num=26, name="W32-P50-mixed", family="w32", level="P50", mix="mixed", D=10, V=1000, B=9,
                    width=32, nops=20_000, ceil=False, stratum=None, w64_twin=9)
    jobs[27] = dict(num=27, name="W32-ALL-mixed", family="w32", level="ALL", mix="mixed", D=1, V=10_000, B=9,
                    width=32, nops=20_000, ceil=False, stratum=None, w64_twin=3)
    return jobs


JOBS = job_table()
VERDICT_JOBS = tuple(range(1, 19)) + tuple(range(21, 26))
STRATA = {lvl: tuple(1 + 3 * l + k for k in range(3)) for l, lvl in enumerate(LEVELS)}
STRATA["CHAINS"] = (21, 22, 23, 24, 25)
STRATA_ORDER = ("ALL", "P75", "P50", "P25", "NONE", "LG", "CHAINS")


def rows_of(job):
    r = ["ctl", "ctl-null", "D1", "D2"]
    if JOBS[job]["ceil"]:
        r.append("ceil")
    return r


def rule_sha256(path=None):
    return hashlib.sha256(Path(path or RULE_FILE).read_bytes()).hexdigest()


# ------------------------------------------------------------------------------------------------- file formats
# workload  job-NN.wl : header (8s magic, 6 int32, 2 int64), then int64 op[ncalls], int64 a[ncalls], int64 b[ncalls].
#           op ENTER: a = numerator, b = denominator; otherwise a, b = handles (call indices) of the operands.
# reference job-NN.ref: int64 num[ncalls], int64 den[ncalls], the exact stored value of every call, lowest terms.
# read-out  (written by bench): int64 num[N], int64 den[N] in handle order.
WL_MAGIC = b"SKWLOAD1"
WL_HEAD = struct.Struct("<8s6i2q")


def write_workload(path, job, width, ceil_kind, nchains, K, n_in, op, a, b):
    with open(path, "wb") as f:
        f.write(WL_HEAD.pack(WL_MAGIC, job, width, ceil_kind, nchains, K, 0, len(op), n_in))
        for arr in (op, a, b):
            f.write(np.asarray(arr, dtype="<i8").tobytes())


def read_workload(path):
    data = Path(path).read_bytes()
    magic, job, width, ceil_kind, nchains, K, _, n, n_in = WL_HEAD.unpack_from(data, 0)
    assert magic == WL_MAGIC, path
    arr = np.frombuffer(data, dtype="<i8", offset=WL_HEAD.size).reshape(3, n)
    return dict(job=job, width=width, ceil_kind=ceil_kind, nchains=nchains, K=K, n=n, n_in=n_in,
                op=arr[0], a=arr[1], b=arr[2])


def write_pairs(path, num, den):
    with open(path, "wb") as f:
        f.write(np.asarray(num, dtype="<i8").tobytes())
        f.write(np.asarray(den, dtype="<i8").tobytes())


def read_pairs(path):
    arr = np.fromfile(path, dtype="<i8")
    n = len(arr) // 2
    return arr[:n], arr[n:]
