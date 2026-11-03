"""Constants of round m3's harness (research/rule-m3.md, spec v0.3): paths, rows, job sets and the reduced grid, the fixed
rule's numbers, the pinned hashes, and the file formats. From m2_common.py (unchanged, m2's): every number carried over
by rule-m3 is m2's (and through it m1's), read from there, and the new ones are marked.

Nothing here is measured, and nothing here imports numpy: the runner imports this module, and keeps its own process
small so that nothing of its own runs, or grows, during timed phases (rule-m3 item 5, carried over).
"""
import hashlib
from pathlib import Path

import m1_common as M1
import m2_common as M2

HERE = M2.HERE                                       # 11-matrix/test
ROOT = M2.ROOT                                       # 11-matrix
LIBDIR = M2.LIBDIR
RULE_FILE = ROOT / "research" / "rule-m3.md"
ROUND3 = M2.ROUND3                                   # round 3: read only

# The fixed rule (rule-m3.md as approved by the author, Oct 30, 2026, committed at dcee959). The harness refuses a
# measured run unless the file on disk has exactly this sha256, and while it holds the mark of a proposal.
RULE_SHA256 = "c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd"
RULE_COMMIT = "dcee959"
PROPOSAL_MARK = M2.PROPOSAL_MARK

ROUND3_RESULTS = M2.ROUND3_RESULTS
ROUND3_RESULTS_SHA256 = M2.ROUND3_RESULTS_SHA256

# Four runs (rule-m3, The rows; Run plan): each its own results folder; the same binaries serve all four (item 2).
OPS = M2.OPS                                         # add, mul, sop, pos
RESULTS = {op: HERE / f"results-m3-{op}" for op in OPS}
M1_RESULTS = dict(M2.M1_RESULTS)                     # m1's two runs and m2's four: never written, checked before and
M2_RESULTS = dict(M2.RESULTS)                        # after each run
BUILD = HERE / "build" / "m3"                        # binaries (shared by the four runs), work-<op>/
TOY_BUILD = HERE / "build" / "m3toy"

# ------------------------------------------------------------------------------------------------- the rule's numbers
# Carried over from rule-m2 (and through it rule-m1), unless marked "new" (approved Oct 30, 2026).
MASTER_SEED = M2.MASTER_SEED
BOOT, BOOT_SEED = M2.BOOT, M2.BOOT_SEED
ROUNDS, PILOT_ROUNDS = M2.ROUNDS, M2.PILOT_ROUNDS
CALIB_MIN_NS, ONE_REP_NS, CAP_NS = M2.CALIB_MIN_NS, M2.ONE_REP_NS, M2.CAP_NS
COUNT_LIMIT_S, COUNT_REPEAT_JOBS = M2.COUNT_LIMIT_S, M2.COUNT_REPEAT_JOBS
HARD_STOP_S = M2.HARD_STOP_S
OPERATOR_TIMEOUT_MIN = M2.OPERATOR_TIMEOUT_MIN
TIMED_CORE, HARNESS_CORE = M2.TIMED_CORE, M2.HARNESS_CORE
DIST_OTHER_TICKS, DIST_OTHER_RATE, DIST_NEW_TICKS = M2.DIST_OTHER_TICKS, M2.DIST_OTHER_RATE, M2.DIST_NEW_TICKS
HOST_BASE_TICKS, HOST_FACTOR = M2.HOST_BASE_TICKS, M2.HOST_FACTOR
HOST_RATE_MIN_TICKS, HOST_RATE_CEILING = M2.HOST_RATE_MIN_TICKS, M2.HOST_RATE_CEILING
RUNNER_TICKS_PER_LAUNCH_MAX = M2.RUNNER_TICKS_PER_LAUNCH_MAX
RUNNER_TICKS_BLOCK_ALLOWANCE = M2.RUNNER_TICKS_BLOCK_ALLOWANCE
IDLE_SAMPLE_S, IDLE_PROC_CPU_MAX, IDLE_CORE1_MAX = M2.IDLE_SAMPLE_S, M2.IDLE_PROC_CPU_MAX, M2.IDLE_CORE1_MAX
IDLE_POLL_S, IDLE_WAIT_S = M2.IDLE_POLL_S, M2.IDLE_WAIT_S
BAND_MIN = M2.BAND_MIN
FIXED_WIN, FIXED_LOSS, SEVERE = M2.FIXED_WIN, M2.FIXED_LOSS, M2.SEVERE
PILOT_LIMIT_S = M2.PILOT_LIMIT_S
# Run plan step 3 (new): the pilot on jobs 1, 6 and 10, one per size; the projection
#   elapsed + 1.2 [ sum_c N_c (O_c + sum_rows (K_row,c + 6 P_row,c)) + 1/2 sum_c N_c sum_rows 4 t_row,c ]
PILOT_JOBS = (1, 6, 10)
PILOT_MARGIN = 1.2                                   # the bracket's factor
PILOT_ROUND_SCALE = 6                                # 12 rounds over the pilot's 2
PILOT_SIDE_RUNS = 4                                  # a gate, a counting and a memory run, taken as 4 runs
PILOT_SIDE_PARALLEL = 2                              # two at a time: the 1/2

# ------------------------------------------------------------------------------------------------- rows
# item 1 (new): five rows; the null row is Hash-V-pack's binary and stays last. The rounds rotate in this order.
ROWS = ("TC", "Hash-V-pack", "Pack-primes", "Pack-coprime", "Hash-V-pack-null")
IMPLS = ("TC", "MHVP", "MHPP", "MHPC")                      # the binaries: one per build of spec v0.3 §16.1
ROW_IMPL = {"TC": "TC", "Hash-V-pack": "MHVP", "Pack-primes": "MHPP", "Pack-coprime": "MHPC",
            "Hash-V-pack-null": "MHVP"}
IMPL_ROW = {"TC": "TC", "MHVP": "Hash-V-pack", "MHPP": "Pack-primes", "MHPC": "Pack-coprime"}
IMPL_ROWS = {"TC": ("TC",), "MHVP": ("Hash-V-pack", "Hash-V-pack-null"), "MHPP": ("Pack-primes",),
             "MHPC": ("Pack-coprime",)}
HASH_IMPLS = ("MHVP", "MHPP", "MHPC")
FAC_IMPLS = ("MHPP", "MHPC")
FAC_FLAGS = {"MHPP": 0, "MHPC": 1}                          # mh3.h: MH3_COPRIME
NULL_ROW, NULL_OF = "Hash-V-pack-null", "Hash-V-pack"      # item 8
RERUN = {"TC": ("TC",), "HASH": HASH_IMPLS, "FAC": FAC_IMPLS}
# item 7 (new): six pairings (row, reference, kind); the ratio is T(reference) / T(row), above 1 the row is faster
PAIRINGS = (("Hash-V-pack", "TC", "against TC"), ("Pack-primes", "TC", "against TC"),
            ("Pack-coprime", "TC", "against TC"), ("Pack-primes", "Hash-V-pack", "factored against pack"),
            ("Pack-coprime", "Hash-V-pack", "factored against pack"),
            ("Pack-coprime", "Pack-primes", "coprime against primes"))
REPORTED_KIND = "coprime against primes"                   # item 7: reported, no verdict on any measure

# ------------------------------------------------------------------------------------------------- the job sets
SET_INPUTS, SET_STEPS, SET_RESULT = M2.SET_INPUTS, M2.SET_STEPS, M2.SET_RESULT
SET_INTERMEDIATES, SET_MATRICES = M2.SET_INTERMEDIATES, M2.SET_MATRICES
OP_MEASURE, PEAK_MEASURE, SEED_OFFSET = M2.OP_MEASURE, M2.PEAK_MEASURE, M2.SEED_OFFSET

# rule-m3, Workloads (new): n in {16, 32, 64} by rho in {1, 4, 16, 64}, jobs 1-12 numbered as rule-m1; sop without
# rho = 16 (jobs 3, 7, 11). Every job keeps its rule-m1 number and seed.
SIZES = M1.SIZES[:3]                                 # 16, 32, 64
TOY_SIZES = M1.TOY_SIZES[:3]                         # harness tests only: 4, 6, 8
RHOS = M1.RHOS
JOBS_OF = {"add": tuple(range(1, 13)), "mul": tuple(range(1, 13)), "pos": tuple(range(1, 13)),
           "sop": (1, 2, 4, 5, 6, 8, 9, 10, 12)}
STRATA = M1.STRATA
STRATA_OF = {op: tuple(s for s in M1.STRATA_ORDER if any(M1.job_rho(j) == STRATA[s] for j in JOBS_OF[op]))
             for op in OPS}                          # sop: NONE, LOW, HIGH (item 11: no MID)
job_n, job_rho, job_s, stratum = M1.job_n, M1.job_rho, M1.job_s, M1.stratum
job_seed, job_name, nops, op_cells = M2.job_seed, M2.job_name, M2.nops, M2.op_cells


def job_size_index(job):
    """the size class of a job: 0 (n = 16), 1 (32), 2 (64)"""
    return (job - 1) // 4


# ------------------------------------------------------------------------------------------------- cases, counters
PHASES, CASES, CASE, CASES_OF, NK, KFIELDS, PERCALL_FIELDS = (M2.PHASES, M2.CASES, M2.CASE, M2.CASES_OF, M2.NK,
                                                              M2.KFIELDS, M2.PERCALL_FIELDS)
STORE_FIELDS = M2.STORE_FIELDS
OPC_FIELDS = M2.OPC_FIELDS
PACK_FIELDS = M2.PACK_FIELDS
# the factored rows' lines (bench_m3.c, api_m3.h)
FAC_FIELDS = ("W", "E", "nq", "w", "bytes_f", "bytes_q", "td_cand", "td_divs", "td_exact", "rgcd", "rdiv", "splits",
              "sgcd", "sdiv", "p2tests")
FFIG_FIELDS = ("W", "E", "nq", "w", "factored", "bytes_f", "bytes_q")
SUBCASES = ("num_cop", "num_uneq", "num_eq", "gen_cop", "gen_uneq", "gen_eq", "mgen_cop", "mgen_one", "mgen_two")
FACC_FIELDS = ("merges", "mentries", "lwalk", "tests", "pgcd", "fmul", "lsplits", "gchecked", "zero_reads",
               "rc_passes", "rc_gcd", "rc_splits", "rc_DL", "rc_DR", "sp_gcd", "sp_div", "rebuilds",
               "rentries") + SUBCASES

# ------------------------------------------------------------------------------------------------- file formats
# As m2's (m2_common): workloads job-NN.wl, references job-NN.ref and job-NN.<X>.ref; predictions job-NN.m3.json; the
# gate dump: m2's sections, and ff<X>, fq<X> for the factored rows (bench_m3.c).
write_workload, read_workload = M2.write_workload, M2.read_workload
write_pairs, read_dump, sha_ints, sha256, home = M2.write_pairs, M2.read_dump, M2.sha_ints, M2.sha256, M2.home


def rule_check(path=None):
    """None, or why the rule on disk is not the fixed rule (rule-m3.md, its header)."""
    p = Path(path or RULE_FILE)
    text = p.read_text()
    if PROPOSAL_MARK in text:
        return f"{p.name} still holds the mark {PROPOSAL_MARK!r}: the rule is not fixed"
    got = sha256(p)
    if got != RULE_SHA256:
        return f"{p.name} has sha256 {got}, not the fixed rule's {RULE_SHA256}"
    return None


round3_check = M2.round3_check
folder_hashes = M2.folder_hashes


def protected(op):
    """the folders a run of `op` checks before and after it (rule-m3, Harness): m1's and m2's results folders, and every
    other m3 run's folder"""
    return {**{f"m1-{k}": v for k, v in M1_RESULTS.items()},
            **{f"m2-{k}": v for k, v in M2_RESULTS.items()},
            **{f"m3-{k}": v for k, v in RESULTS.items() if k != op}}


def forbidden(path, op=None):
    """Why the harness may not write at path, or None (rule-m3, Harness): nowhere in ../10-interning/, nor in m1's or m2's
    results folders; and a run of `op` nowhere in another m3 run's folder."""
    p = Path(path).resolve()
    r3 = ROUND3.resolve()
    if p == r3 or r3 in p.parents:
        return f"{p} is in ../10-interning/ (round 3: read only)"
    for tag, folders in (("m1", M1_RESULTS), ("m2", M2_RESULTS)):
        for k, f in folders.items():
            f = f.resolve()
            if p == f or f in p.parents:
                return f"{p} is {tag}'s {k} results folder (read only)"
    for k, f in RESULTS.items():
        f = f.resolve()
        if op is not None and k != op and (p == f or f in p.parents):
            return f"{p} is the m3 {k} run's folder (a run of {op} may not write there)"
    return None


def stamp_sha(data):
    return hashlib.sha256(data).hexdigest()
