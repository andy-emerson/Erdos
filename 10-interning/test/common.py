"""Shared constants of the layer-1 harness: the run plan of research/rule.md, paths, rows and file formats.

Nothing here is measured; gen.py, runner.py and analyze.py import it. Adapted from shared/partial-sharing-baseline/test/common.py (then 08-skeletons).
"""
import hashlib
import struct
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent              # 10-interning/test
ROOT = HERE.parent                                   # 10-interning
RULE_FILE = ROOT / "research" / "rule.md"
BUILD = HERE / "build"
# Results folders (rule, revision of Oct 29, 2026: the official data). The run after the revision (the Q32 rows) writes
# only to RESULTS. The measured run of b3b88b6 is kept in ARCHIVE: a byte-for-byte copy of test/results/ as that run
# left it (results.md, verdicts.csv, rounds.csv and counters.csv as committed at b3b88b6, and the raw state.json and
# run.log), committed with the code. The runner refuses to write into ARCHIVE or the original test/results/
# (PROTECTED), and refuses any run while a file of ARCHIVE differs from the sha256 recorded here.
RESULTS = HERE / "results-q32"
ARCHIVE = HERE / "results-q64-b3b88b6"
PROTECTED = (ARCHIVE, HERE / "results")
ARCHIVE_SHA256 = {
    "counters.csv": "690e2105b8cdfd99c0b425a8256997d7c62d270d53bf702258f9d31459defb5a",
    "results.md": "6d195328baf40a0c90696df7a595cf975db83b930ec4dafc7e7ca775d64b53ef",
    "rounds.csv": "f3dc7b2cb34e203b4f9925bd885ad33b32f1bde06ec875254fe69b9e415c92fc",
    "run.log": "9d8779f1c8db2de1dc13c49a3808d2d35ec4a0760c0a23814d02eaf02275c0ec",
    "state.json": "ce322948a2b68936e7470a6f89ffb66403bb936ee3368a05009c60050c9e5e0a",
    "verdicts.csv": "79ddcd3890e532183e8b670065fde85911058fb06aaa06bd2ec6f6f84cf9576f",
}

# sha256 of research/rule.md as fixed by the author, Oct 29, 2026 (every PROPOSED mark approved), with the procedural
# revision of the same day (the idle-machine check) and the revision after the measured run of b3b88b6 (the Q32 rows,
# an exception approved by the author; rule.md, top), re-pinned after that revision's review (safeguard 2 worded with
# the band h, the Q32 lookup's own translation unit, the archived official data and test/results-q32/, the pilot's
# projection updated, gcd work against time for the Q32 rows), and re-pinned for the wording of the whole run's
# non-normative estimate with the Q32 rows (about 11 to 20 minutes; Run plan), and re-pinned after the Q32 run for the
# rename to 10-interning (paths only: links to shared/partial-sharing-baseline; rule.md, top). RULE_SHA256_AT_Q32_RUN is
# the hash the Q32 run of test/results-q32/ was made under. The harness refuses a measured run unless the file on disk has
# exactly this hash, so it holds the rule verbatim (rule.md, line 3).
RULE_SHA256 = "de2049725b26821910a5b985fd229a284855ac585c97cbff0be7ef47ec3dca50"
RULE_SHA256_AT_Q32_RUN = "99f3c189c3393bf0e89bf163faf7354d2d472c7b1e5c317a0e006a9ba7d063a1"

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
# The pilot's projection (rule, Run plan step 3, as revised Oct 29, 2026 for the Q32 rows; test/DECISIONS.md R-19):
# the remaining work's upper estimate, 8.5 minutes before the revision (gates, counting and memory runs 2.5;
# calibration and rounds 6), scaled by the work each part gains: gates 33 -> 60 runs (16 Q32, 11 lookup gate), 1.5 ->
# 2.7 min; counting and memory runs 115 -> 155 (repeats on jobs 3, 5 and 10 included), 1 -> 1.4 min; timed invocations
# per round and r-calibrations 69 -> 85, 6 -> 7.4 min: 11.5 minutes. The pilot's nominal wall time scales with its own invocations per round, 31 -> 37 (Q32 rows on pilot jobs 1,
# 3 and 10): 1 -> 1.2 minutes. The 45-minute limit and the 60-minute hard stop are unchanged.
PILOT_REMAINING_S = 11.5 * 60      # the remaining work's upper estimate (rule, Run plan step 3, revised)
PILOT_NOMINAL_S = 72.0             # the pilot's nominal wall time (1.2 minutes, revised)
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

# Implementations (binaries) and rows. L1-null is the L1 binary run as its own row. L1 and L1-opt are the Q64 rows
# (spec §18: "L1-Q64" and "L1-opt-Q64" where both widths are meant); L1q and L1oq are the Q32 rows, run only on the
# jobs within their capacity. L1g is the lookup gate's build (spec §18.1 item 4): gates only, no row, never timed.
IMPLS = ("L1", "L1opt", "L1q", "L1oq", "ctl", "fmpq", "mpq", "fz", "L1g")
COUNTED = ("L1", "L1opt", "L1q", "L1oq", "ctl")    # counting build, test build, gates
LAYER1 = ("L1", "L1opt", "L1q", "L1oq")
Q32_IMPLS = ("L1q", "L1oq")
Q64_OF = {"L1q": "L1", "L1oq": "L1opt"}            # each Q32 row's Q64 row (spec §18.3 item 1)
OPT_IMPLS = ("L1opt", "L1oq")
BASE_OF = {"L1opt": "L1", "L1oq": "L1q"}           # each L1-opt row's L1 (spec §17.3)
GATE_ONLY = ("L1g",)
LIBRARIES = ("fmpq", "mpq", "fz")
FORMED = ("fmpq", "mpq")                            # forms A and B, calibrated per job
ROW_IMPL = {"L1": "L1", "L1-opt": "L1opt", "L1-null": "L1", "L1-Q32": "L1q", "L1-opt-Q32": "L1oq", "ctl": "ctl",
            "fmpq": "fmpq", "mpq": "mpq", "fz": "fz"}
IMPL_ROWS = {"L1": ("L1", "L1-null"), "L1opt": ("L1-opt",), "L1q": ("L1-Q32",), "L1oq": ("L1-opt-Q32",),
             "ctl": ("ctl",), "fmpq": ("fmpq",), "mpq": ("mpq",), "fz": ("fz",), "L1g": ()}
Q32_ROWS = ("L1-Q32", "L1-opt-Q32")
Q32_PAIR = {"L1-Q32": "L1", "L1-opt-Q32": "L1-opt"}  # the paired Q32-against-Q64 comparison (rule, revision)

# Q32 capacity (spec §18.1 item 2): 32,768 whole numbers, at indices 0 to 32,767. A Q32 row runs only on the jobs
# whose generator-predicted W at the end is at most this (rule, revision of Oct 29, 2026: capacity).
Q32_CAPACITY = 32_768
# The measured run whose Q64 results stay official (rule, revision of Oct 29, 2026, safeguard 1): its verdicts are
# read from the archive (checked against ARCHIVE_SHA256), or else from git at that commit.
OFFICIAL_Q64_COMMIT = "b3b88b6"
OFFICIAL_Q64_VERDICTS = "09-interning/test/results/verdicts.csv"     # its path at that commit (folder renamed 10-interning later)
# The model's bits per lookup slot b_s and per rational H (rule, revision: the model's prediction).
SLOT_BITS = {"L1": 32, "L1opt": 64, "L1q": 16, "L1oq": 48}
RAT_BITS = {"L1": 64, "L1opt": 64, "L1q": 32, "L1oq": 32}


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


def rows_of(job, q32=True):
    """The rows of a job (rule item 1); q32 False on a job over the Q32 capacity."""
    r = ["L1", "L1-opt", "L1-null"] + (list(Q32_ROWS) if q32 else []) + ["ctl", "fmpq", "mpq"]
    if JOBS[job]["fz"]:
        r.append("fz")
    return r


def impls_of(job, q32=True):
    return [i for i in IMPLS if (i != "fz" or JOBS[job]["fz"]) and (q32 or i not in Q32_IMPLS)
            and i not in GATE_ONLY]


def lookup_slots(W):
    """S by the growth rule (spec §5 item 3): the smallest power of two, at least 16, with 2W <= S."""
    S = 16
    while 2 * W > S:
        S *= 2
    return S


def archive_check():
    """None, or the first file of the b3b88b6 archive that is missing or differs from its recorded sha256."""
    for name, want in ARCHIVE_SHA256.items():
        f = ARCHIVE / name
        if not f.exists():
            return f"{f}: missing"
        got = hashlib.sha256(f.read_bytes()).hexdigest()
        if got != want:
            return f"{f}: sha256 {got} differs from the archived {want}"
    return None


def protected(path):
    """True if path is, or is inside, a folder of official data that no run may write (PROTECTED)."""
    p = Path(path).resolve()
    return any(p == q.resolve() or q.resolve() in p.parents for q in PROTECTED)


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
