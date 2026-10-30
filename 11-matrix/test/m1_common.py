"""Constants of the matrix round's harness (research/rule-m1.md, spec v0.1): paths, rows, jobs, the fixed rule's numbers,
the pinned hashes, and the file formats. From round 3's r3_common.py (../../10-interning/test/, unchanged there): every
number carried over by rule-m1 is round 3's, and the new ones are marked.

Nothing here is measured, and nothing here imports numpy: the runner imports this module, and keeps its own process
small so that nothing of its own runs, or grows, during timed phases (rule-m1 item 5, carried over from rule-r3).
"""
import hashlib
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent              # 11-matrix/test
ROOT = HERE.parent                                   # 11-matrix
LIBDIR = ROOT / "lib"
RULE_FILE = ROOT / "research" / "rule-m1.md"
ROUND3 = ROOT.parent / "10-interning"                # round 3: read only (spec §1; rule-m1, Harness)

# The fixed rule (rule-m1.md as approved by the author, Oct 29, 2026, committed at 5ee4eaa). The harness refuses a
# measured run unless the file on disk has exactly this sha256, and while it holds the word "PROPOSED" (rule-m1.md,
# its header: "while any mark of a proposal awaiting approval remains in it", as round 3).
RULE_SHA256 = "8e951ac6597f099962aa77580653e799d101af6d7b3e30a51e1a6005ab7aeff7"
PROPOSAL_MARK = "PROPOSED"

# Round 3's results (rule-m1, Harness): their sha256 is checked before and after each run, and nothing is ever written
# anywhere in ../10-interning/.
ROUND3_RESULTS = ROUND3 / "test" / "results-r3"
ROUND3_RESULTS_SHA256 = {
    "counters.csv": "3ae6149a7c76dfcdba672d42afe395de0f97ed797d28bd7b27faf8cfb6fdac8a",
    "results.md": "bd92ecdbb57172c92c7d4ce177364559ade4bc0b57147b40b462d8a35d8551e0",
    "rounds.csv": "e6f53ede60c91416a3e2b57ae863c80c1d5db3c9a812b994bcba66ce51c1e24d",
    "run.log": "41dbb4f6442d385cca96930a968350b8855cb5de549954fd49ef719c051268b9",
    "speed_vs_fill.png": "61fd40d15dee4f4f2b1ed8eb88fcca47de7a78b04b0f7f196f424b08528c7f1f",
    "state.json": "9c9d02516d2d4d8863d76c6cd01e20a58920d7ba49fa7c42c2379dddfbce21af",
    "verdicts.csv": "824baa8e4caf1b7e312e36c388d8f6e462b7375f3fc59704ed73ed9dd466b468",
}

# Two runs (rule-m1, Run plan): each its own results folder; the same binaries serve both (rule-m1 item 2).
OPS = ("add", "mul")
RESULTS = {"add": HERE / "results-m1-add", "mul": HERE / "results-m1-mul"}
BUILD = HERE / "build" / "m1"                        # binaries (shared by both runs), work-add/, work-mul/
TOY_BUILD = HERE / "build" / "m1toy"

# ------------------------------------------------------------------------------------------------- the rule's numbers
# Carried over from rule-r3 (rule-m1 items 4-6, 8, 9), unless marked "new" (approved by the author, Oct 29, 2026).
MASTER_SEED = 20261029                                # Workloads
BOOT, BOOT_SEED = 10_000, 20261029                    # item 7
ROUNDS, PILOT_ROUNDS = 12, 2                          # item 4; Run plan step 3
CALIB_MIN_NS = 50_000_000                             # item 4: r, the smallest power of two with a rep >= 50 ms
ONE_REP_NS = 1_000_000_000                            # a calibration rep >= 1 s: 1 rep per invocation, else 3
CAP_NS = 10_000_000_000                               # a timed rep over 10 s: over cap
COUNT_LIMIT_S = 600                                   # item 2: every counting or memory run, 10 minutes
COUNT_REPEAT_JOBS = (1, 6)                            # item 2 (new): a small and a middle job
HARD_STOP_S = 40 * 60                                 # new: 40 minutes per run
OPERATOR_TIMEOUT_MIN = 45                             # item 5 (new): the hard stop plus 5
TIMED_CORE, HARNESS_CORE = 1, 0                       # item 4
DIST_OTHER_TICKS = 5                                  # item 5: any other process: more than 5 + ceil(1 x L) ticks
DIST_OTHER_RATE = 1.0
DIST_NEW_TICKS = 1                                    # a new process with at least 1 tick
HOST_BASE_TICKS, HOST_FACTOR = 5, 2                   # host allowance 5 + ceil(2 R_h L)
HOST_RATE_MIN_TICKS = 1                               # R_h: at least 1 tick over the longest pilot block
HOST_RATE_CEILING = 4.0                               # ticks per second; a host measured above it stops the run
RUNNER_TICKS_PER_LAUNCH_MAX = 1.0                     # the runner's sanity limit: 1 tick per launch plus 2 per block
RUNNER_TICKS_BLOCK_ALLOWANCE = 2
IDLE_SAMPLE_S = 2.0                                   # item 6
IDLE_PROC_CPU_MAX = 0.05
IDLE_CORE1_MAX = 0.02
IDLE_POLL_S, IDLE_WAIT_S = 15.0, 600.0
BAND_MIN = 0.05                                       # items 8-9
FIXED_WIN, FIXED_LOSS, SEVERE = 0.90, 1.10, 2.0
# Run plan step 3 (new): the pilot on jobs 1, 6 and 13; projection = elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6)
PILOT_JOBS = (1, 6, 13)
PILOT_P_SMALL, PILOT_P_LARGE = 6, 13
PILOT_SCALE, PILOT_MARGIN = 5.0, 1.2
PILOT_LIMIT_S = 30 * 60

# ------------------------------------------------------------------------------------------------- rows
# item 1: seven rows; the null row is Hash-V-fixed's binary. The rounds rotate over the rows in this order (item 4).
ROWS = ("TC", "Hash-V-fixed", "Hash-M-fixed", "Hash-V-grow", "Hash-M-grow", "Hash-V-grow-map", "Hash-V-fixed-null")
IMPLS = ("TC", "MHVF", "MHMF", "MHVG", "MHMG", "MHVGM")       # the binaries: one per build of spec §3
ROW_IMPL = {"TC": "TC", "Hash-V-fixed": "MHVF", "Hash-M-fixed": "MHMF", "Hash-V-grow": "MHVG",
            "Hash-M-grow": "MHMG", "Hash-V-grow-map": "MHVGM", "Hash-V-fixed-null": "MHVF"}
IMPL_ROW = {"TC": "TC", "MHVF": "Hash-V-fixed", "MHMF": "Hash-M-fixed", "MHVG": "Hash-V-grow", "MHMG": "Hash-M-grow",
            "MHVGM": "Hash-V-grow-map"}
IMPL_ROWS = {"TC": ("TC",), "MHVF": ("Hash-V-fixed", "Hash-V-fixed-null"), "MHMF": ("Hash-M-fixed",),
             "MHVG": ("Hash-V-grow",), "MHMG": ("Hash-M-grow",), "MHVGM": ("Hash-V-grow-map",)}
HASH_IMPLS = ("MHVF", "MHMF", "MHVG", "MHMG", "MHVGM")
# the settings of each hash binary (mh.h): (MH_M, MH_GROW, MH_MAP)
HASH_FLAGS = {"MHVF": (0, 0, 0), "MHMF": (1, 0, 0), "MHVG": (0, 1, 0), "MHMG": (1, 1, 0), "MHVGM": (0, 1, 1)}
M_IMPLS = ("MHMF", "MHMG")
GROW_IMPLS = ("MHVG", "MHMG", "MHVGM")
NULL_ROW, NULL_OF = "Hash-V-fixed-null", "Hash-V-fixed"      # item 8
# --rerun (rule-m1 item 12): a fix to tcm.c re-runs TC; a fix to the hash source every hash row and the null row
RERUN = {"TC": ("TC",), "HASH": HASH_IMPLS}
# item 7: the nine pairings (row, reference, kind); the ratio is T(reference) / T(row), above 1 the row is faster
PAIRINGS = (("Hash-V-fixed", "TC", "against TC"), ("Hash-M-fixed", "TC", "against TC"),
            ("Hash-V-grow", "TC", "against TC"), ("Hash-M-grow", "TC", "against TC"),
            ("Hash-M-fixed", "Hash-V-fixed", "M against V"), ("Hash-M-grow", "Hash-V-grow", "M against V"),
            ("Hash-V-grow", "Hash-V-fixed", "grow against fixed"), ("Hash-M-grow", "Hash-M-fixed", "grow against fixed"),
            ("Hash-V-grow-map", "Hash-V-grow", "map against lookup"))
MAP_PAIRING = ("Hash-V-grow-map", "Hash-V-grow")
TIME_MEASURES = ("creation", "operation")              # each with its own verdict and band (items 7-9)
REPORTED_TIMES = ("total", "read-out")                 # ratio and interval, no verdict
GROW_T0 = 16
PHI = 0x9E3779B97F4A7C15

# ------------------------------------------------------------------------------------------------- jobs (Workloads)
SIZES = (16, 32, 64, 128)
RHOS = (1, 4, 16, 64)
TOY_SIZES = (4, 6, 8, 12)                              # harness tests only (DECISIONS M1-H9)
JOBS = tuple(range(1, 17))
STRATA = {"NONE": 1, "LOW": 4, "MID": 16, "HIGH": 64}  # item 11: the sharing level rho
STRATA_ORDER = ("NONE", "LOW", "MID", "HIGH")


def job_n(job, toy=False):
    return (TOY_SIZES if toy else SIZES)[(job - 1) // 4]


def job_rho(job):
    return RHOS[(job - 1) % 4]


def job_s(job, toy=False):
    """s = n^2 / rho distinct values per matrix (toy: at least 2)"""
    n = job_n(job, toy)
    return max(2, n * n // job_rho(job)) if toy else n * n // job_rho(job)


def job_seed(op, job):
    """addition job j: 20261029 + j; multiplication job j: 20261029 + 100 + j (rule-m1, Workloads)"""
    return MASTER_SEED + job + (100 if op == "mul" else 0)


def job_name(op, job):
    return f"{op}-{job}"


def stratum(job):
    return {v: k for k, v in STRATA.items()}[job_rho(job)]


# ------------------------------------------------------------------------------------------------- cases, counters
PHASES = ("entry", "add", "sub", "mul", "div")
# the cases, numbered as lib/include/r2.h (r2_case) in round 3's builds (read with r3.h)
CASES = ("add_zero", "add_cross", "add_den", "add_num", "add_general",
         "mul_one_x", "mul_one_y", "mul_both", "mul_den", "mul_num", "mul_cross_ad", "mul_cross_cb", "mul_general",
         "div_one_x", "div_one_y", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den", "div_general",
         "add_int_both", "add_int_x", "add_int_y", "mul_int_both", "mul_int_x", "mul_int_y", "add_zero_x", "add_zero_y",
         "mul_zero", "div_zero")
CASE = {n: i for i, n in enumerate(CASES)}
CASES_OF = {"add": ("add_zero", "add_zero_x", "add_zero_y", "add_int_both", "add_int_x", "add_int_y", "add_cross",
                    "add_den", "add_num", "add_general"),
            "mul": ("mul_zero", "mul_one_x", "mul_one_y", "mul_int_both", "mul_both", "mul_den", "mul_num",
                    "mul_cross_ad", "mul_cross_cb", "mul_int_x", "mul_int_y", "mul_general")}
NK = 15
KFIELDS = ("ops", "gcd", "gcd_work", "div", "mul", "add")
PERCALL_FIELDS = ("case", "gcd", "gcd_work", "div_exact", "mul", "add")   # int16 each (m1.h)
STORE_FIELDS = ("W", "T", "w", "wh", "nf", "found", "new", "probes", "longest", "widenings", "copied", "growths",
                "reinserted", "rprobes", "rewritten", "wprobes")
OPC_FIELDS = ("cross", "cross_match", "cross_reads", "tau_found", "tau_none", "tau_probes")
ROLES = ("A", "B", "C")

# ------------------------------------------------------------------------------------------------- file formats
#   workload job-NN.wl: "M1WLOAD1", int32 op (0 add, 1 mul), int32 n, int32 job, int32 0, then int64 A[n^2][2] and
#   int64 B[n^2][2], the pairs (m, d) as entered, row-major
#   reference and read-out job-NN.ref: int64 num[n^2], int64 den[n^2], C's cells row-major
#   gate dump: sections of an 8-byte name, an int64 count and that many int64 values (bench_m1.c)
WL_MAGIC = b"M1WLOAD1"
WL_HEAD = struct.Struct("<8s4i")


def write_workload(path, op, n, job, A, B):
    with open(path, "wb") as f:
        f.write(WL_HEAD.pack(WL_MAGIC, 0 if op == "add" else 1, n, job, 0))
        for X in (A, B):
            f.write(struct.pack(f"<{2 * len(X)}q", *[v for p in X for v in p]))


def read_workload(path):
    """(op, n, job, A, B) of a workload file, A and B lists of (m, d) as entered"""
    raw = Path(path).read_bytes()
    magic, opk, n, job, _ = WL_HEAD.unpack(raw[:WL_HEAD.size])
    assert magic == WL_MAGIC, path
    q = n * n
    v = struct.unpack(f"<{4 * q}q", raw[WL_HEAD.size:WL_HEAD.size + 32 * q])
    A = [(v[2 * i], v[2 * i + 1]) for i in range(q)]
    B = [(v[2 * q + 2 * i], v[2 * q + 2 * i + 1]) for i in range(q)]
    return ("mul" if opk else "add"), n, job, A, B


def write_pairs(path, pairs):
    with open(path, "wb") as f:
        f.write(struct.pack(f"<{len(pairs)}q", *[p[0] for p in pairs]))
        f.write(struct.pack(f"<{len(pairs)}q", *[p[1] for p in pairs]))


def read_dump(path):
    """{name: raw bytes of its int64 values} from a gate dump"""
    raw, out, i = Path(path).read_bytes(), {}, 0
    while i < len(raw):
        name = raw[i:i + 8].rstrip(b"\0").decode()
        (cnt,) = struct.unpack("<q", raw[i + 8:i + 16])
        out[name] = raw[i + 16:i + 16 + 8 * cnt]
        i += 16 + 8 * cnt
    return out


def sha_ints(values):
    """the sha256 of a list of int64 values as the driver writes them (a dump section)"""
    return hashlib.sha256(struct.pack(f"<{len(values)}q", *values)).hexdigest()


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def rule_check(path=None):
    """None, or why the rule on disk is not the fixed rule (rule-m1.md, its header)."""
    p = Path(path or RULE_FILE)
    text = p.read_text()
    if PROPOSAL_MARK in text:
        return f"{p.name} still holds the mark {PROPOSAL_MARK!r}: the rule is not fixed"
    got = sha256(p)
    if got != RULE_SHA256:
        return f"{p.name} has sha256 {got}, not the fixed rule's {RULE_SHA256}"
    return None


def round3_check():
    """None, or the first file of round 3's results that is missing or differs from its pinned sha256."""
    for name, want in ROUND3_RESULTS_SHA256.items():
        f = ROUND3_RESULTS / name
        if not f.exists():
            return f"{f}: missing"
        got = sha256(f)
        if got != want:
            return f"{f}: sha256 {got} differs from the pinned {want}"
    return None


def folder_hashes(folder):
    """{name: sha256} of every file directly in a folder (the addition run's results, checked by the multiplication
    run before and after)"""
    folder = Path(folder)
    return {p.name: sha256(p) for p in sorted(folder.iterdir()) if p.is_file()} if folder.exists() else {}


def forbidden(path, op=None):
    """Why the harness may not write at path, or None: nowhere in ../10-interning/; and the multiplication run
    nowhere in the addition run's folder (rule-m1, Harness)."""
    p = Path(path).resolve()
    r3 = ROUND3.resolve()
    if p == r3 or r3 in p.parents:
        return f"{p} is in ../10-interning/ (round 3: read only)"
    if op == "mul":
        a = RESULTS["add"].resolve()
        if p == a or a in p.parents:
            return f"{p} is the addition run's folder (the multiplication run may not write there)"
    return None


def home(v, t):
    """the hash of spec §5.3: the top t bits of v * phi mod 2^64"""
    return ((v * PHI) & 0xFFFFFFFFFFFFFFFF) >> (64 - t)
