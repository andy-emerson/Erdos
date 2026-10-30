"""Constants of round 3's harness (research/rule-r3.md, spec v0.3): paths, rows, jobs, the fixed rule's numbers, the
pinned hashes, and the file formats. From round 2's r2_common.py (unchanged): every number carried over by rule-r3 is
round 2's, and the new ones are marked.

Nothing here is measured, and nothing here imports numpy: the runner imports this module, and keeps its own process
small so that nothing of its own runs, or grows, during timed phases (rule-r3 item 5; test/DECISIONS.md R2-H3). The
round-1 and round-2 harnesses are kept as they are and are not imported here.
"""
import hashlib
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent              # 10-interning/test
ROOT = HERE.parent                                   # 10-interning
LIBDIR = ROOT / "lib"
RULE_FILE = ROOT / "research" / "rule-r3.md"
BUILD = HERE / "build" / "r3"
RESULTS = HERE / "results-r3"                        # round 3 writes only here (rule-r3, Harness: the official data)

# The fixed rule (rule-r3.md as approved by the author, Oct 29, 2026, committed at 3267a47), with its revision of Oct
# 29, 2026 (spec v0.3 §17; before any measurement; re-pinned: the approved text's sha256 was 1820c273...85fad2). The
# harness refuses a measured run unless the file on disk has exactly this sha256, and while it holds the word
# "PROPOSED" (rule-r3.md, its header).
RULE_SHA256 = "f4671f1fbc53273a193e63fc1e1c13e116aa0d7d6a06647aadbf6890407f2cf9"
PROPOSAL_MARK = "PROPOSED"

# The official data (rule-r3, Harness: the official data): round 1's, the records of the two stopped round-2 runs,
# and round 2's complete run (results-r2/, the three-structure Q32 (retired), as committed at 82c73bf). Round 3 never
# writes or deletes there; the sha256 of every file is checked before and after the run.
RESULTS_STOPPED = HERE / "results-r2-stopped-193ae57"
RESULTS_STOPPED_2 = HERE / "results-r2-stopped-173fad1"
RESULTS_R2 = HERE / "results-r2"
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
    RESULTS_STOPPED: {
        "counters.csv": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "results.md": "9fc7a29a3b612041aedbe0e4e1ac90a9f65519343869ba8e9830fd52e06ae94b",
        "rounds.csv": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "run.log": "9e1637593170b619a4fa2b662104dfd7380eb5413d710b430ddd0c5d9b6dbff8",
        "state.json": "5185faa97d4ab4f31988ad0327748ba964d6f7d2a0562241a453795d3153ecb7",
        "verdicts.csv": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    },
    RESULTS_STOPPED_2: {
        "counters.csv": "327ded80c032d7a91557ac7347e6b06d6809b0fcae6d0ae4fc90fadbd4ea9435",
        "results.md": "5da5c00eafc30af0721d7410fe8126b7cc39eb3645b3c1e1b2a2a45f649ee166",
        "rounds.csv": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "run.log": "5db8b323ba1cc450b38f312833b8c50c60c0bdcdee63152b779474d57bc28280",
        "state.json": "98b54e33f93acf7482b927af3ff6320db52e040fdda8cc8bfdfd8032827e502b",
        "verdicts.csv": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    },
    RESULTS_R2: {
        "counters.csv": "327ded80c032d7a91557ac7347e6b06d6809b0fcae6d0ae4fc90fadbd4ea9435",
        "results.md": "04ec89dfb5eecdab8f5de4a134f2d0b6123a4be9a94af534175f4aec738ba958",
        "rounds.csv": "a335825620cedc8fdb1ea7376a7a8cc7bd3a6032a6e0ddf65baa050458c61e61",
        "run.log": "e38ff080b39407f797ee609c986365ca8671fe4d30442dc2ece678c1172cd1d5",
        "state.json": "5e696fee7b78c2ce379dc75d9e3bdf6ed7c7e08aab2c0ae93a6513c533f917d6",
        "verdicts.csv": "418ff4c1418af2f348fa2348d254ecc067f57454198fc9f497c70f718b7ed56b",
    },
}
# Round 1's workloads, pinned in its Q32 run's state.json (itself pinned above): round 3's must be byte-identical
# (rule-r3, Workloads).
ROUND1_STATE = HERE / "results-q32" / "state.json"

# ------------------------------------------------------------------------------------------------- the rule's numbers
# Every number below is carried over from round 2 (rule-r3 items 2-13), unless marked "new in round 3".
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
# item 5: the disturbance check (revision of Oct 29, 2026: new processes by process (TGID), not by thread; the fork
# counter is reported only; the host allowance and ceiling approved by the author, Oct 29, 2026)
DIST_OTHER_TICKS = 5                                  # any other process: more than 5 + ceil(1 x L) ticks over a block
DIST_OTHER_RATE = 1.0                                 #   of L seconds (a floor plus a rate: 1 tick/s, 1% of one core)
DIST_NEW_TICKS = 1                                    # a new process with at least 1 tick
HOST_BASE_TICKS, HOST_FACTOR = 5, 2                   # host allowance 5 + ceil(2 R_h L)
HOST_RATE_MIN_TICKS = 1                               # R_h: at least 1 tick over the longest pilot block
HOST_RATE_CEILING = 4.0                               # ticks per second; a host measured above it stops the run
# the runner's own ticks (second revision of Oct 29, 2026): recorded and reported per block, in total and per launch,
# not judged; one sanity limit: more than 1 tick per launch plus 2 over a block is a harness bug (approved by the
# author, Oct 29, 2026; the allowance of 2 for the block's own two snapshots is a minor fix found in review).
# Replaces the fixed limit of 2 ticks per block.
RUNNER_TICKS_PER_LAUNCH_MAX = 1.0
RUNNER_TICKS_BLOCK_ALLOWANCE = 2                      # fixed: the block's two snapshots, whatever its launches
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
# projection = elapsed + 5 min x max(1, P_rest / 0.5 min) + 42 P_idx + 36 P_prof
PILOT_REST_MIN, PILOT_NOMINAL_MIN = 5.0, 0.5
PILOT_IDX_FACTOR = 42                                 # new in round 3 (approved by the author, Oct 29, 2026)
PILOT_PROF_FACTOR = 36
PILOT_LIMIT_S = 22 * 60

# ------------------------------------------------------------------------------------------------- rows and jobs
# item 1: four rows; the null row is Q32-hash's binary. The rounds rotate over the rows in this order (item 4).
ROWS = ("TC", "Q32-index", "Q32-hash", "Q32-hash-null")
ROW_IMPL = {"TC": "TC", "Q32-index": "Q32I", "Q32-hash": "Q32H", "Q32-hash-null": "Q32H"}
IMPLS = ("TC", "Q32I", "Q32H")                         # the binaries: one per row of spec v0.3 §5
IMPL_ROWS = {"TC": ("TC",), "Q32I": ("Q32-index",), "Q32H": ("Q32-hash", "Q32-hash-null")}
IMPL_ROW = {"TC": "TC", "Q32I": "Q32-index", "Q32H": "Q32-hash"}
Q_IMPLS = ("Q32I", "Q32H")
NULL_ROW, NULL_OF = "Q32-hash-null", "Q32-hash"        # item 8
# item 7: the three pairings, (row, reference): the ratio is T(reference) / T(row), above 1 the row is faster
PAIRINGS = (("Q32-index", "TC"), ("Q32-hash", "TC"), ("Q32-hash", "Q32-index"))
TIME_MEASURES = ("creation", "operations")             # each with its own verdict and band (items 7-9)
REPORTED_TIMES = ("total", "read-out")                 # ratio and interval, no verdict
PROF_ROWS = ("Q32-index", "Q32-hash", "TC")            # item 10: the profile's order, repeated three times per job
Q32_CAPACITY = 32_768
SLOTS = 32_768                                         # Q32-hash's table (spec v0.3 §8)
TABLE_BYTES = 8 * SLOTS
PHI = 0x9E3779B97F4A7C15

# The 11 jobs of round 1 (rule.md; rule-r2, Workloads), numbered alike; round 2 runs those within Q32's capacity.
JOB_NAMES = {1: "MONEY-wide", 2: "MONEY-catalog", 3: "SMALL8", 4: "SMALL64", 5: "MOD-mixed", 6: "MOD-muldiv",
             7: "NEW-add", 8: "CH-money", 9: "CH-closed", 10: "CH-small", 11: "CH-mixed"}
ALL_JOBS = tuple(sorted(JOB_NAMES))
RUN_JOBS = (1, 2, 3, 4, 8, 9, 10, 11)                 # within capacity (rule-r3, Workloads)
OVER_CAPACITY_JOBS = (5, 6, 7)
STRATA = {"MONEY": (1, 2), "SMALL": (3, 4), "CHAINS": (8, 9, 10, 11)}
STRATA_ORDER = ("MONEY", "SMALL", "CHAINS")

# ------------------------------------------------------------------------------------------------- cases, counters
PHASES = ("entry", "add", "sub", "mul", "div")
OPNAME = {0: "enter", 1: "add", 2: "sub", 3: "mul", 4: "div"}
# the cases of spec v0.2 §9 and of the amendment of Oct 29, 2026 (v0.3 §17.3: the integer cases of addition and
# multiplication, and the zero-operand cases of every operation, appended), numbered as lib/include/r2.h (r2_case) in round 3's builds
CASES = ("add_zero", "add_cross", "add_den", "add_num", "add_general",
         "mul_one_x", "mul_one_y", "mul_both", "mul_den", "mul_num", "mul_cross_ad", "mul_cross_cb", "mul_general",
         "div_one_x", "div_one_y", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den", "div_general",
         "add_int_both", "add_int_x", "add_int_y", "mul_int_both", "mul_int_x", "mul_int_y", "add_zero_x", "add_zero_y",
         "mul_zero", "div_zero")
CASE = {n: i for i, n in enumerate(CASES)}
# per algorithm, in its order of checks (v0.3 §17.3)
CASES_OF = {"add": ("add_zero", "add_zero_x", "add_zero_y", "add_int_both", "add_int_x", "add_int_y", "add_cross",
                    "add_den", "add_num", "add_general"),
            "mul": ("mul_zero", "mul_one_x", "mul_one_y", "mul_int_both", "mul_both", "mul_den", "mul_num",
                    "mul_cross_ad", "mul_cross_cb", "mul_int_x", "mul_int_y", "mul_general"),
            "div": ("div_zero", "div_one_x", "div_one_y", "div_both", "div_cross_ad", "div_cross_cb", "div_num",
                    "div_den", "div_general")}
CASES_OF["sub"] = CASES_OF["add"]
assert sorted(sum((CASES_OF[k] for k in ("add", "mul", "div")), ())) == sorted(CASES)
NK = 15
KFIELDS = ("ops", "gcd", "gcd_work", "div", "mul", "add")
# the base counters (08's sk_count) that the driver prints, by these names (round 2's lookup counters are gone: no row
# has a lookup); round 3's storage counters (sctr lines, spec v0.3 §11.1)
BASE_FIELDS = ("gcd", "gcd_work", "div_exact", "mul", "add", "ev_overflows")
STORAGE_FIELDS = ("examined", "probes", "longest")
PERCALL_FIELDS = ("gcd", "gcd_work", "div_exact", "mul", "add")
# profile phases (spec §13.4 item 4), as r2.h numbers them; U is computed
PF_PHASES = ("F", "C", "A.gcd", "A.rest", "I", "S", "M")
JOB_PHASES = ("entry", "add", "sub", "mul", "div", "read-out")
PF_CASES = CASES + ("entry", "read")

# ------------------------------------------------------------------------------------------------- file formats
# workload job-NN.wl, reference job-NN.ref, read-out: as round 1 (test/common.py), read here without numpy.
#   workload: header (8s magic, 6 int32, 2 int64), then int64 op[n], int64 a[n], int64 b[n]
#   reference and read-out: int64 num[n], int64 den[n]
#   case file (round 2's format, written again by round 3) job-NN.case2: int8 per call, the case of spec v0.2 §9 or
#   v0.3 §17.3, numbered as CASES (-1 for an entry)
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
    """None, or why the rule on disk is not the fixed rule (rule-r3.md, its header)."""
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
    """True if path is, or is inside, a folder of the official data (round 1's, round 2's, or a stopped run's)."""
    p = Path(path).resolve()
    return any(p == q.resolve() or q.resolve() in p.parents for q in PROTECTED)


def home(v):
    """Q32-hash's hash function (spec v0.3 §8 item 3): the top 15 bits of v * phi mod 2^64."""
    return ((v * PHI) & 0xFFFFFFFFFFFFFFFF) >> 49
