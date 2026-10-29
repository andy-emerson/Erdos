"""Constants of round 2's harness (research/rule-r2.md, spec v0.2): paths, rows, jobs, the fixed rule's numbers, the
pinned hashes, and the file formats.

Nothing here is measured, and nothing here imports numpy: the runner imports this module, and keeps its own process
small so that nothing of its own runs, or grows, during timed phases (rule-r2 item 5; test/DECISIONS.md R2-H3). The
round-1 harness (common.py, runner.py, analyze.py) is kept as it is and is not imported here (spec v0.2 §16 item 2).
"""
import hashlib
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent              # 10-interning/test
ROOT = HERE.parent                                   # 10-interning
LIBDIR = ROOT / "lib"
RULE_FILE = ROOT / "research" / "rule-r2.md"
BUILD = HERE / "build" / "r2"
RESULTS = HERE / "results-r2"                        # round 2 writes only here (rule-r2, Harness: the official data)

# The fixed rule (rule-r2.md as approved by the author, Oct 29, 2026). The harness refuses a measured run unless the
# file on disk has exactly this sha256, and while it holds the word "PROPOSED" (rule-r2.md, "Fixing the rule").
RULE_SHA256 = "44baa5f1db601571d538116d37b1b8859fbf0eb99fe149baaa9ba655879e5f66"
PROPOSAL_MARK = "PROPOSED"

# The official data of round 1 (rule-r2, Harness: the official data). Round 2 never writes or deletes there; the sha256
# of every file is checked before and after the run.
PROTECTED = {
    HERE / "results": {
        "counters.csv": "690e2105b8cdfd99c0b425a8256997d7c62d270d53bf702258f9d31459defb5a",
        "results.md": "6d195328baf40a0c90696df7a595cf975db83b930ec4dafc7e7ca775d64b53ef",
        "rounds.csv": "f3dc7b2cb34e203b4f9925bd885ad33b32f1bde06ec875254fe69b9e415c92fc",
        "verdicts.csv": "79ddcd3890e532183e8b670065fde85911058fb06aaa06bd2ec6f6f84cf9576f",
    },
    HERE / "results-q64-b3b88b6": {
        "counters.csv": "690e2105b8cdfd99c0b425a8256997d7c62d270d53bf702258f9d31459defb5a",
        "results.md": "6d195328baf40a0c90696df7a595cf975db83b930ec4dafc7e7ca775d64b53ef",
        "rounds.csv": "f3dc7b2cb34e203b4f9925bd885ad33b32f1bde06ec875254fe69b9e415c92fc",
        "run.log": "9d8779f1c8db2de1dc13c49a3808d2d35ec4a0760c0a23814d02eaf02275c0ec",
        "state.json": "ce322948a2b68936e7470a6f89ffb66403bb936ee3368a05009c60050c9e5e0a",
        "verdicts.csv": "79ddcd3890e532183e8b670065fde85911058fb06aaa06bd2ec6f6f84cf9576f",
    },
    HERE / "results-q32": {
        "counters.csv": "befc17a8dc07587ebe1026d13c28e682b05769bba6d270426561593fb48e42c0",
        "q64_rerun_verdicts.csv": "d67d553c0162119c6e7ee564430b4640e803059e10c2b6975f9c4a19a094e90c",
        "results.md": "ae5a6667406be1983b8ea2f8d9ba909f2f75fbbdc29288a57256dcbb2d81c40d",
        "rounds.csv": "1349124af0f13a11e8e15965a3ec7e35dbc521e71d33a9b83529158ba7dda157",
        "run.log": "d09ef7ed7c8f56abe098ccdd9e440aaa70a3933e5e7e12925a2aad7022134c82",
        "state.json": "707e3327f4e279d436e6229d45a20039843c29c98a6b708894e8c885a7b201e3",
        "verdicts.csv": "25c12c4ee4cf62e6ff25293511de9782a45d1e27cac678f4893a1cdda77a4bcb",
    },
}
# Round 1's workloads, pinned in its Q32 run's state.json (itself pinned above): round 2's must be byte-identical
# (rule-r2, Workloads).
ROUND1_STATE = HERE / "results-q32" / "state.json"

# ------------------------------------------------------------------------------------------------- the rule's numbers
MASTER_SEED = 20261029
BOOT, BOOT_SEED = 10_000, 20261029                    # item 7
ROUNDS, PILOT_ROUNDS = 12, 2                          # item 4; Run plan step 3
CALIB_MIN_NS = 50_000_000                             # item 4: r, the smallest power of two with a rep >= 50 ms
ONE_REP_NS = 1_000_000_000                            # a calibration rep >= 1 s: 1 rep per invocation, else 3
CAP_NS = 10_000_000_000                               # a timed rep over 10 s: over cap
COUNT_LIMIT_S = 600                                   # item 2: every counting or memory run, 10 minutes
COUNT_REPEAT_JOBS = (3, 10)                           # item 2
HARD_STOP_S = 30 * 60                                 # the author's: 30 minutes
OPERATOR_TIMEOUT_MIN = 35                             # item 5: the operator's background command, at least 35 min
TIMED_CORE, HARNESS_CORE = 1, 0                       # item 4
# item 5: the disturbance check
DIST_OTHER_TICKS = 5                                  # any other process: more than 5 ticks over the block
DIST_NEW_TICKS = 1                                    # a new process with at least 1 tick
DIST_FORKS = 0                                        # more than 0 unexplained forks
HOST_BASE_TICKS, HOST_FACTOR = 5, 2                   # host allowance: 5 + ceil(2 R_h L)
HOST_RATE_MAX = 1.0                                   # ticks per second: a busy host stops the run after the pilot
RUNNER_TICKS_MAX = 2                                  # the runner's own ticks per block: more is a harness bug
# item 6: the idle machine
IDLE_SAMPLE_S = 2.0
IDLE_PROC_CPU_MAX = 0.05                              # no other process above 5% of one core over the sample
IDLE_CORE1_MAX = 0.02                                 # core 1 busy at most 2% of the sample
IDLE_POLL_S, IDLE_WAIT_S = 15.0, 600.0
# items 7-9
BAND_MIN = 0.05
FIXED_WIN, FIXED_LOSS, SEVERE = 0.90, 1.10, 2.0
# item 10 and spec §13.4: the profile
PROF_K = 32
PROF_SEED = 20261029
PROF_INVOCATIONS = 3
PROF_SLOWDOWN_MAX = 1.5
PROF_CASE_MIN_CALLS = 100
PROBE_BLOCKS, PROBE_PER_BLOCK = 1000, 1000
# Run plan step 3: the pilot
PILOT_JOBS = (1, 3, 10)
PILOT_PROF_JOB = 1
PILOT_REST_MIN, PILOT_NOMINAL_MIN = 5.0, 0.5          # projection = elapsed + 5 min x max(1, P / 0.5 min) + 36 P_prof
PILOT_PROF_FACTOR = 36
PILOT_LIMIT_S = 22 * 60

# ------------------------------------------------------------------------------------------------- rows and jobs
ROWS = ("Q32", "Q32-null", "TC")                      # item 1
ROW_IMPL = {"Q32": "Q32", "Q32-null": "Q32", "TC": "TC"}
IMPLS = ("Q32", "TC")
IMPL_ROWS = {"Q32": ("Q32", "Q32-null"), "TC": ("TC",)}
PROF_ROWS = ("Q32", "TC")
Q32_CAPACITY = 32_768

# The 11 jobs of round 1 (rule.md; rule-r2, Workloads), numbered alike; round 2 runs those within Q32's capacity.
JOB_NAMES = {1: "MONEY-wide", 2: "MONEY-catalog", 3: "SMALL8", 4: "SMALL64", 5: "MOD-mixed", 6: "MOD-muldiv",
             7: "NEW-add", 8: "CH-money", 9: "CH-closed", 10: "CH-small", 11: "CH-mixed"}
ALL_JOBS = tuple(sorted(JOB_NAMES))
RUN_JOBS = (1, 2, 3, 4, 8, 9, 10, 11)                 # within capacity (rule-r2, Workloads)
OVER_CAPACITY_JOBS = (5, 6, 7)
STRATA = {"MONEY": (1, 2), "SMALL": (3, 4), "CHAINS": (8, 9, 10, 11)}
STRATA_ORDER = ("MONEY", "SMALL", "CHAINS")

# ------------------------------------------------------------------------------------------------- cases, counters
PHASES = ("entry", "add", "sub", "mul", "div")
OPNAME = {0: "enter", 1: "add", 2: "sub", 3: "mul", 4: "div"}
# the cases of spec v0.2 §9, numbered as lib/include/r2.h (r2_case)
CASES = ("add_zero", "add_cross", "add_den", "add_num", "add_general",
         "mul_one_x", "mul_one_y", "mul_both", "mul_den", "mul_num", "mul_cross_ad", "mul_cross_cb", "mul_general",
         "div_one_x", "div_one_y", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den", "div_general")
CASE = {n: i for i, n in enumerate(CASES)}
CASES_OF = {"add": CASES[0:5], "sub": CASES[0:5], "mul": CASES[5:13], "div": CASES[13:21]}
NK = 15
KFIELDS = ("ops", "gcd", "gcd_work", "div", "mul", "add")
# the base counters (08's sk_count) and round 2's (r2_xcount) that the driver prints, by these names
BASE_FIELDS = ("gcd", "gcd_work", "div_exact", "mul", "add", "lookup_search", "lookup_probe", "ev_overflows")
PERCALL_FIELDS = ("gcd", "gcd_work", "div_exact", "mul", "add")
# profile phases (spec §13.4 item 4), as r2.h numbers them; U is computed
PF_PHASES = ("F", "C", "A.gcd", "A.rest", "I", "S", "M")
JOB_PHASES = ("entry", "add", "sub", "mul", "div", "read-out")
PF_CASES = CASES + ("entry", "read")

# ------------------------------------------------------------------------------------------------- file formats
# workload job-NN.wl, reference job-NN.ref, read-out: as round 1 (test/common.py), read here without numpy.
#   workload: header (8s magic, 6 int32, 2 int64), then int64 op[n], int64 a[n], int64 b[n]
#   reference and read-out: int64 num[n], int64 den[n]
#   case file (round 2) job-NN.case2: int8 per call, the case of spec v0.2 §9 (-1 for an entry)
WL_MAGIC = b"SKWLOAD1"
WL_HEAD = struct.Struct("<8s6i2q")


def wl_header(path):
    """(job, ncalls, n_in) of a workload file."""
    with open(path, "rb") as f:
        magic, job, width, fz, nch, K, _, n, n_in = WL_HEAD.unpack(f.read(WL_HEAD.size))
    assert magic == WL_MAGIC, path
    return job, n, n_in


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rule_check(path=None):
    """None, or why the rule on disk is not the fixed rule (rule-r2.md, "Fixing the rule")."""
    p = Path(path or RULE_FILE)
    text = p.read_text()
    if PROPOSAL_MARK in text:
        return f"{p.name} still holds the mark {PROPOSAL_MARK!r}: the rule is not fixed"
    got = sha256(p)
    if got != RULE_SHA256:
        return f"{p.name} has sha256 {got}, not the fixed rule's {RULE_SHA256}"
    return None


def archive_check():
    """None, or the first file of the official data that is missing or differs from its pinned sha256."""
    for folder, files in PROTECTED.items():
        for name, want in files.items():
            f = folder / name
            if not f.exists():
                return f"{f}: missing"
            got = sha256(f)
            if got != want:
                return f"{f}: sha256 {got} differs from the pinned {want}"
    return None


def protected(path):
    """True if path is, or is inside, a folder of round 1's official data."""
    p = Path(path).resolve()
    return any(p == q.resolve() or q.resolve() in p.parents for q in PROTECTED)


def lookup_slots(W):
    """S by the growth rule (spec v0.2 §7 item 2): the smallest power of two, at least 16, with 2W <= S."""
    S = 16
    while 2 * W > S:
        S *= 2
    return S
