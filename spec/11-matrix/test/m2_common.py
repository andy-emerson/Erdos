"""Constants of round m2's harness (research/rule-m2.md, spec v0.2): paths, rows, job sets, the fixed rule's numbers, the
pinned hashes, and the file formats. From m1_common.py (unchanged, m1's): every number carried over by rule-m2 is
m1's, read from there, and the new ones are marked.

Nothing here is measured, and nothing here imports numpy: the runner imports this module, and keeps its own process
small so that nothing of its own runs, or grows, during timed phases (rule-m2 item 5, carried over).
"""
import hashlib
import struct
from pathlib import Path

import m1_common as M1

HERE = M1.HERE                                       # 11-matrix/test
ROOT = M1.ROOT                                       # 11-matrix
LIBDIR = M1.LIBDIR
RULE_FILE = ROOT / "research" / "rule-m2.md"
ROUND3 = M1.ROUND3                                   # round 3: read only

# The fixed rule (rule-m2.md as approved by the author, Oct 30, 2026, committed at 38f2d5d). The harness refuses a
# measured run unless the file on disk has exactly this sha256, and while it holds the mark of a proposal.
RULE_SHA256 = "e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200"
RULE_COMMIT = "38f2d5d"
PROPOSAL_MARK = M1.PROPOSAL_MARK

ROUND3_RESULTS = M1.ROUND3_RESULTS
ROUND3_RESULTS_SHA256 = M1.ROUND3_RESULTS_SHA256

# Four runs (rule-m2, The rows; Run plan): each its own results folder; the same binaries serve all four (item 2).
OPS = ("add", "mul", "sop", "pos")
RESULTS = {op: HERE / f"results-m2-{op}" for op in OPS}
M1_RESULTS = dict(M1.RESULTS)                        # m1's two runs: never written, checked before and after each run
BUILD = HERE / "build" / "m2"                        # binaries (shared by the four runs), work-<op>/
TOY_BUILD = HERE / "build" / "m2toy"

# ------------------------------------------------------------------------------------------------- the rule's numbers
# Carried over from rule-m1 (rule-m2 items 2, 4-6, 8, 9; Run plan), unless marked "new" (approved Oct 30, 2026).
MASTER_SEED = M1.MASTER_SEED
BOOT, BOOT_SEED = M1.BOOT, M1.BOOT_SEED
ROUNDS, PILOT_ROUNDS = M1.ROUNDS, M1.PILOT_ROUNDS
CALIB_MIN_NS, ONE_REP_NS, CAP_NS = M1.CALIB_MIN_NS, M1.ONE_REP_NS, M1.CAP_NS
COUNT_LIMIT_S, COUNT_REPEAT_JOBS = M1.COUNT_LIMIT_S, M1.COUNT_REPEAT_JOBS
HARD_STOP_S = M1.HARD_STOP_S
OPERATOR_TIMEOUT_MIN = M1.OPERATOR_TIMEOUT_MIN
TIMED_CORE, HARNESS_CORE = M1.TIMED_CORE, M1.HARNESS_CORE
DIST_OTHER_TICKS, DIST_OTHER_RATE, DIST_NEW_TICKS = M1.DIST_OTHER_TICKS, M1.DIST_OTHER_RATE, M1.DIST_NEW_TICKS
HOST_BASE_TICKS, HOST_FACTOR = M1.HOST_BASE_TICKS, M1.HOST_FACTOR
HOST_RATE_MIN_TICKS, HOST_RATE_CEILING = M1.HOST_RATE_MIN_TICKS, M1.HOST_RATE_CEILING
RUNNER_TICKS_PER_LAUNCH_MAX = M1.RUNNER_TICKS_PER_LAUNCH_MAX
RUNNER_TICKS_BLOCK_ALLOWANCE = M1.RUNNER_TICKS_BLOCK_ALLOWANCE
IDLE_SAMPLE_S, IDLE_PROC_CPU_MAX, IDLE_CORE1_MAX = M1.IDLE_SAMPLE_S, M1.IDLE_PROC_CPU_MAX, M1.IDLE_CORE1_MAX
IDLE_POLL_S, IDLE_WAIT_S = M1.IDLE_POLL_S, M1.IDLE_WAIT_S
BAND_MIN = M1.BAND_MIN
FIXED_WIN, FIXED_LOSS, SEVERE = M1.FIXED_WIN, M1.FIXED_LOSS, M1.SEVERE
PILOT_JOBS = M1.PILOT_JOBS
PILOT_P_SMALL, PILOT_P_LARGE = M1.PILOT_P_SMALL, M1.PILOT_P_LARGE
PILOT_SCALE, PILOT_MARGIN = M1.PILOT_SCALE, M1.PILOT_MARGIN
PILOT_LIMIT_S = M1.PILOT_LIMIT_S

# ------------------------------------------------------------------------------------------------- rows
# item 1 (new): five rows; the null row is Hash-V-fixed's binary and stays last. The rounds rotate in this order.
ROWS = ("TC", "Hash-V-fixed", "Hash-V-grow-map", "Hash-V-pack", "Hash-V-fixed-null")
IMPLS = ("TC", "MHVF", "MHVGM", "MHVP")                     # the binaries: one per build of spec v0.2 §15.3
ROW_IMPL = {"TC": "TC", "Hash-V-fixed": "MHVF", "Hash-V-grow-map": "MHVGM", "Hash-V-pack": "MHVP",
            "Hash-V-fixed-null": "MHVF"}
IMPL_ROW = {"TC": "TC", "MHVF": "Hash-V-fixed", "MHVGM": "Hash-V-grow-map", "MHVP": "Hash-V-pack"}
IMPL_ROWS = {"TC": ("TC",), "MHVF": ("Hash-V-fixed", "Hash-V-fixed-null"), "MHVGM": ("Hash-V-grow-map",),
             "MHVP": ("Hash-V-pack",)}
HASH_IMPLS = ("MHVF", "MHVGM", "MHVP")
HASH_FLAGS = {"MHVF": (0, 0), "MHVGM": (1, 0), "MHVP": (0, 1)}  # mh2.h: (MH_GROW, MH_PACK)
SIZING = {"MHVF": "fixed", "MHVGM": "grow", "MHVP": "pack"}
NULL_ROW, NULL_OF = "Hash-V-fixed-null", "Hash-V-fixed"        # item 8
RERUN = {"TC": ("TC",), "HASH": HASH_IMPLS}
# item 7 (new): six pairings (row, reference, kind); the ratio is T(reference) / T(row), above 1 the row is faster
PAIRINGS = (("Hash-V-fixed", "TC", "against TC"), ("Hash-V-grow-map", "TC", "against TC"),
            ("Hash-V-pack", "TC", "against TC"), ("Hash-V-pack", "Hash-V-fixed", "pack against fixed"),
            ("Hash-V-grow-map", "Hash-V-fixed", "grow-map against fixed"),
            ("Hash-V-pack", "Hash-V-grow-map", "pack against grow-map"))
REPORTED_KIND = "pack against grow-map"                     # item 7: reported, no verdict on any measure
GROW_T0 = M1.GROW_T0
PHI = M1.PHI

# ------------------------------------------------------------------------------------------------- the job sets
# rule-m2 (new): four sets, four runs; the matrices of a set, the steps, and the measures judged in its run
SET_INPUTS = {"add": ("A", "B"), "mul": ("A", "B"), "sop": ("A", "B", "C", "D"), "pos": ("A", "B", "C", "D")}
SET_STEPS = {"add": (("add", "A", "B", "C"),), "mul": (("mul", "A", "B", "C"),),
             "sop": (("mul", "A", "B", "P"), ("mul", "C", "D", "Q"), ("add", "P", "Q", "R")),
             "pos": (("add", "A", "B", "S"), ("add", "C", "D", "U"), ("mul", "S", "U", "R"))}
SET_RESULT = {"add": "C", "mul": "C", "sop": "R", "pos": "R"}
SET_INTERMEDIATES = {"add": (), "mul": (), "sop": ("P", "Q"), "pos": ("S", "U")}
SET_MATRICES = {s: SET_INPUTS[s] + SET_INTERMEDIATES[s] + (SET_RESULT[s],) for s in OPS}
OP_MEASURE = {"add": (2, "addition time"), "mul": (3, "multiplication time"), "sop": (4, "sum-of-products time"),
              "pos": (5, "product-of-sums time")}
PEAK_MEASURE = {"add": 7, "mul": 8, "sop": None, "pos": None}    # sop's and pos's peaks: reported, no verdict
SEED_OFFSET = {"add": 0, "mul": 100, "sop": 200, "pos": 300}

SIZES, RHOS, TOY_SIZES, JOBS = M1.SIZES, M1.RHOS, M1.TOY_SIZES, M1.JOBS
STRATA, STRATA_ORDER = M1.STRATA, M1.STRATA_ORDER
job_n, job_rho, job_s, stratum = M1.job_n, M1.job_rho, M1.job_s, M1.stratum


def job_seed(op, job):
    """job j: 20261029 + j (add), + 100 + j (mul), + 200 + j (sop), + 300 + j (pos) (rule-m2, Workloads)"""
    return MASTER_SEED + SEED_OFFSET[op] + job


def job_name(op, job):
    return f"{op}-{job}"


def nops(op, n):
    """the arithmetic operations of a job: n^2 additions per addition step, n^3 products and n^3 additions per
    multiplication step"""
    return sum(n * n if k == "add" else 2 * n ** 3 for k, _, _, _ in SET_STEPS[op])


def op_cells(op, n):
    """the operation's cells (spec v0.2 §15.6): every matrix that exists during it: 3 n^2 or 7 n^2"""
    return len(SET_MATRICES[op]) * n * n


# ------------------------------------------------------------------------------------------------- cases, counters
PHASES, CASES, CASE, CASES_OF, NK, KFIELDS, PERCALL_FIELDS = (M1.PHASES, M1.CASES, M1.CASE, M1.CASES_OF, M1.NK,
                                                              M1.KFIELDS, M1.PERCALL_FIELDS)
STORE_FIELDS = ("W", "T", "w", "wh", "whb", "packed", "nf", "found", "new", "probes", "longest", "widenings", "copied",
                "growths", "reinserted", "rprobes", "rewritten", "bcells", "bvals")
OPC_FIELDS = ("cross", "cross_match", "cross_reads")
PACK_FIELDS = ("W", "T", "w", "whb", "whp", "rewritten", "bytes_list", "bytes_map", "bytes_table", "bytes_cells_build",
               "bytes_cells_pack")
GROWTH_FIELDS = ("fields", "W", "T_old", "reinserted", "rprobes", "rewritten", "bytes_new", "bytes_map")

# ------------------------------------------------------------------------------------------------- file formats
#   add, mul: m1's workload, job-NN.wl, "M1WLOAD1" (m1_common.write_workload), written by m1_gen's own functions
#   sop, pos: job-NN.wl, "M2WLOAD1", int32 set (2 sop, 3 pos), int32 n, int32 job, int32 0, then int64 A[n^2][2],
#             B, C, D, the pairs (m, d) as entered, row-major
#   reference and read-out job-NN.ref: int64 num[n^2], int64 den[n^2], the result's cells row-major
#   intermediates' references (sop, pos) job-NN.<name>.ref, the same format
#   gate dump: sections of an 8-byte name, an int64 count and that many int64 values (bench_m2.c)
WL2_MAGIC = b"M2WLOAD1"
WL_HEAD = M1.WL_HEAD
SET_CODE = {"add": 0, "mul": 1, "sop": 2, "pos": 3}


def write_workload(path, op, n, job, mats):
    """mats: the inputs' pairs, in order"""
    if op in ("add", "mul"):
        return M1.write_workload(path, op, n, job, mats[0], mats[1])
    with open(path, "wb") as f:
        f.write(WL_HEAD.pack(WL2_MAGIC, SET_CODE[op], n, job, 0))
        for X in mats:
            f.write(struct.pack(f"<{2 * len(X)}q", *[v for p in X for v in p]))


def read_workload(path):
    """(op, n, job, [inputs' pairs]) of a workload file"""
    raw = Path(path).read_bytes()
    magic, code, n, job, _ = WL_HEAD.unpack(raw[:WL_HEAD.size])
    assert magic in (M1.WL_MAGIC, WL2_MAGIC), path
    op = OPS[code]
    k = len(SET_INPUTS[op])
    q = n * n
    v = struct.unpack(f"<{2 * k * q}q", raw[WL_HEAD.size:WL_HEAD.size + 16 * k * q])
    return op, n, job, [[(v[2 * x * q + 2 * i], v[2 * x * q + 2 * i + 1]) for i in range(q)] for x in range(k)]


write_pairs, read_dump, sha_ints, sha256, home = M1.write_pairs, M1.read_dump, M1.sha_ints, M1.sha256, M1.home


def rule_check(path=None):
    """None, or why the rule on disk is not the fixed rule (rule-m2.md, its header)."""
    p = Path(path or RULE_FILE)
    text = p.read_text()
    if PROPOSAL_MARK in text:
        return f"{p.name} still holds the mark {PROPOSAL_MARK!r}: the rule is not fixed"
    got = sha256(p)
    if got != RULE_SHA256:
        return f"{p.name} has sha256 {got}, not the fixed rule's {RULE_SHA256}"
    return None


round3_check = M1.round3_check
folder_hashes = M1.folder_hashes


def protected(op):
    """the folders a run of `op` checks before and after it (rule-m2, Harness): m1's two results folders, and every other
    m2 run's folder (an earlier m2 run's results)"""
    return {**{f"m1-{k}": v for k, v in M1_RESULTS.items()},
            **{f"m2-{k}": v for k, v in RESULTS.items() if k != op}}


def forbidden(path, op=None):
    """Why the harness may not write at path, or None (rule-m2, Harness): nowhere in ../10-interning/, nor in m1's results
    folders; and a run of `op` nowhere in another m2 run's folder."""
    p = Path(path).resolve()
    r3 = ROUND3.resolve()
    if p == r3 or r3 in p.parents:
        return f"{p} is in ../10-interning/ (round 3: read only)"
    for k, f in M1_RESULTS.items():
        f = f.resolve()
        if p == f or f in p.parents:
            return f"{p} is m1's {k} results folder (read only)"
    for k, f in RESULTS.items():
        f = f.resolve()
        if op is not None and k != op and (p == f or f in p.parents):
            return f"{p} is the m2 {k} run's folder (a run of {op} may not write there)"
    return None


def stamp_sha(data):
    return hashlib.sha256(data).hexdigest()
