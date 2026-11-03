"""How round m3's harness builds the rows and the driver (rule-m3 item 2, carried over from rule-m2): one compiler, one
set of flags (gcc -std=c11 -O2, with round 3's warnings), the builds of each row differing only in -D settings, and the
unit, differential, allocation and sanitizer tests, and the TC gate's binary (m2's tc3gate_m2.c, unchanged). From
m2_libconf.py (unchanged, m2's).

The rows (spec v0.3 §16.11): TC (tcm.c, m1's), Hash-V-pack (m2's mh2.c, unchanged, built -DMH_GROW=0 -DMH_PACK=1, as m2
built it), and the factored rows (mh3.c, built -DMH3_COPRIME=0 or 1). The builds: timing, counting, test, and m2's
memory build (DECISIONS-m2 M2-D9; the timing build's flags with -DM2_MEM and the heap wrapper).

The pins (rule-m3 item 2): the 08 module, tc3.c and its headers, tcm.c's recorded diff (m1's); m1's sources at m1's
commit; and m2's sources and harness at m2's commit (e6505aa), so that the m1 and m2 runs stay reproducible.
"""
import hashlib
import json
from pathlib import Path

import m2_libconf as L2
import m3_common as C

LIBDIR = C.LIBDIR
SRC = LIBDIR / "src"
INCD = LIBDIR / "include"
V08 = LIBDIR / "vendor08"
TESTS = LIBDIR / "tests"
CC = L2.CC
CFLAGS = list(L2.CFLAGS)
SAN = list(L2.SAN)
INCLUDE = [INCD, V08, SRC, TESTS, C.HERE]
BUILDS = dict(L2.BUILDS)                                  # timing, counting, test, memory
WRAP = list(L2.WRAP)
LINK = {"memory": WRAP}                                   # the memory build's heap wrapper (bench_m3.c)

# the objects, and the -D a source needs beyond the build's
OBJECTS = {"tcm": (SRC / "tcm.c", []), "common": (V08 / "common.c", []), "m1common": (SRC / "m1common.c", []),
           "m2common": (SRC / "m2common.c", []), "m3common": (SRC / "m3common.c", []),
           "mhvp": (SRC / "mh2.c", ["-DMH_GROW=0", "-DMH_PACK=1"]),
           "mhpp": (SRC / "mh3.c", ["-DMH3_COPRIME=0"]), "mhpc": (SRC / "mh3.c", ["-DMH3_COPRIME=1"])}
COMMON = ("common", "m1common", "m2common", "m3common")
ROW_OBJECTS = {"TC": ("tcm",) + COMMON, "MHVP": ("mhvp",) + COMMON, "MHPP": ("mhpp",) + COMMON,
               "MHPC": ("mhpc",) + COMMON}
IMPL_DEFS = {"TC": ["-DIMPL_TC"], "MHVP": ["-DIMPL_HASH", "-DMH_GROW=0", "-DMH_PACK=1"],
             "MHPP": ["-DIMPL_FAC", "-DMH3_COPRIME=0"], "MHPC": ["-DIMPL_FAC", "-DMH3_COPRIME=1"]}
DRIVER = [C.HERE / "bench_m3.c", C.HERE / "api_m3.h"]
TC3GATE = C.HERE / "tc3gate_m2.c"                         # m2's, unchanged: the same job sets
UNIT = {"m3_unit": TESTS / "m3_unit.c", "m3_diff": TESTS / "m3_diff.c"}
UNIT_ALL = ("tcm", "mhvp", "mhpp", "mhpc") + COMMON
DIFF_SEEDS, DIFF_SEEDS_SAN = L2.DIFF_SEEDS, L2.DIFF_SEEDS_SAN
ALLOC_TEST = TESTS / "m3_alloc.c"
ALLOC_BUILDS = L2.ALLOC_BUILDS                            # timing, counting, memory

# the pins (rule-m3 item 2): the 08 module, tc3.c and its headers, tcm.c's recorded diff, and m1's sources: m2's
VENDOR08_SHA256 = L2.VENDOR08_SHA256
ROUND3_SHA256 = L2.ROUND3_SHA256
TCM_DIFF = L2.TCM_DIFF
vendor08_check = L2.vendor08_check
tc3_check = L2.tc3_check
tcm_diff_text = L2.tcm_diff_text
tcm_diff_check = L2.tcm_diff_check
M1_SOURCES_SHA256 = L2.M1_SOURCES_SHA256
m1_sources_check = L2.m1_sources_check

# m2's sources and harness, as m2 committed them (e6505aa, the m2 results): m3 links mh2.c (Hash-V-pack, unchanged:
# spec v0.3 §16.11) and m2common.c, includes m2.h and mh2.h, runs m2's tc3gate_m2.c and imports m2's Python modules;
# none of them may change (the m2 runs stay reproducible)
M2_COMMIT = "e6505aa"
M2_SOURCES_SHA256 = {
    "lib/src/mh2.c": "5b860542e173b648b557e7323d0caab26ed5d266d5fa66c9da1e7747c3b84f3d",
    "lib/src/m2common.c": "1ff8b6259c86c61743caaa99d7efdccf70c92084c813fb9c6fad8c05243ccb6a",
    "lib/include/m2.h": "d3e6e08c57bf0a8242436e36aaf4ff18581ac978318112cd6d558abc38c66b7d",
    "lib/include/mh2.h": "8e85320fc0320deb3bea6837d0058f92568965727bc2e3fed09d4c04c87607fb",
    "test/bench_m2.c": "8b87fd58afe18fb0c3198bbbcd343c37d7976b2b02c74ab79872304d2fee66aa",
    "test/api_m2.h": "57e8b02f46b6bfdcde350cc50f91d51c45cd0e70f7418a86309e60d0e1582bfa",
    "test/tc3gate_m2.c": "f4f6b8c5ac7712732a963f5f5f160743c96dc26a9113f4062446032e00aebe69",
    "lib/tests/m2t.h": "236c6dc086c00682bd1683be2bf2e9dd2a31a7a418a65bd29c5da1820ac576ee",
    "lib/tests/m2_unit.c": "2d78314d96956b77fff8b6a4da488ebc7864efb6a4aad791d5e8937ffdead9f3",
    "lib/tests/m2_diff.c": "8703df4e486dba1b3f25151b2c16e7505f67259eb64b69805e4400b3ea5f970d",
    "lib/tests/m2_alloc.c": "0b12bf1714548cc5cd198841ec847180db4f02e408c5d32cf3a2f68f1fbe0046",
    "test/m2_common.py": "84bbf10866ab557c2e7a1282b7c538a266a77f2b5f2b00897ee3fbc744d8ca6d",
    "test/m2_libconf.py": "f6d4193ffba5848ec607a7380d762fedd759bef0ab4e44f53f6958f61c203b10",
    "test/m2_model.py": "eda021e0c9c4347245855badc043496823098ff06d3573065b583fd0c2f4c770",
    "test/m2_gen.py": "849ea865d368d926105a285f9b685ab18fa72b1a169ee45fb3d3bb2b9f941ee3",
    "test/m2_checks.py": "00eabce580372f4e4c9c87d9c3d2ad705a2688599988cd64d9f8dc89e87eda61",
    "test/m2_runner.py": "883e756c68a4d499b524996a699204962358f6ce23fb02dbaec7d350d4b815af",
    "test/m2_analyze.py": "7dd066b14d0aa8b279f5bcc5a34279021247258b14e686ca12c166442517a86e",
    "test/test_m2_harness.py": "df29eb8b6679069150fa8903d694e60865beb03ff000cd01e898b5a58484e93b",
}

SHARED = [INCD / "m1.h", INCD / "m2.h", INCD / "m3.h", INCD / "r2.h", INCD / "r3.h", SRC / "r2int.h", SRC / "m1int.h",
          SRC / "m1common.c", SRC / "m2common.c", SRC / "m3common.c", V08 / "sk_internal.h", V08 / "common.c",
          V08 / "sk_common.h"] + DRIVER
OWN = {"TC": [SRC / "tcm.c", INCD / "tcm.h"], "MHVP": [SRC / "mh2.c", INCD / "mh2.h"],
       "MHPP": [SRC / "mh3.c", INCD / "mh3.h"], "MHPC": [SRC / "mh3.c", INCD / "mh3.h"]}


def m2_sources_check():
    """None, or the first of m2's sources and harness files whose sha256 differs from the pinned one."""
    for name, want in M2_SOURCES_SHA256.items():
        got = hashlib.sha256((C.ROOT / name).read_bytes()).hexdigest()
        if got != want:
            return f"{name}: sha256 {got} differs from m2's {want} (commit {M2_COMMIT})"
    return None


def code_hashes():
    """{impl: sha256 of its own files, the shared files, the compiler and the flags}: a result made with other code is
    stale (round 1's R-o)."""
    out = {}
    for impl in C.IMPLS:
        s = hashlib.sha256(json.dumps([CC, CFLAGS, BUILDS, IMPL_DEFS[impl], LINK,
                                       {k: v[1] for k, v in OBJECTS.items()}]).encode())
        for p in sorted(set(SHARED + OWN[impl]), key=str):
            s.update(Path(p).name.encode() + b"\0" + Path(p).read_bytes() + b"\0")
        out[impl] = s.hexdigest()
    s = hashlib.sha256()
    for p in [TC3GATE, SRC / "tc3.c", INCD / "tc.h"] + SHARED:
        s.update(Path(p).name.encode() + b"\0" + Path(p).read_bytes() + b"\0")
    out["TC3GATE"] = s.hexdigest()
    return out


def inc():
    return sum((["-I", str(p)] for p in INCLUDE), [])


def build_commands(bindir):
    """[(output, argv)] for every object, bench binary, the TC gate and test binary, in order."""
    bindir = Path(bindir)
    cmds = []
    for b, defs in BUILDS.items():
        od = bindir / "obj" / b
        for name, (src, extra) in OBJECTS.items():
            cmds.append((od / f"{name}.o", [CC] + CFLAGS + defs + extra + inc() + ["-c", str(src), "-o",
                                                                                  str(od / f"{name}.o")]))
        for impl in C.IMPLS:
            exe = bindir / f"bench-{impl}-{b}"
            cmds.append((exe, [CC] + CFLAGS + defs + IMPL_DEFS[impl] + inc() + [str(DRIVER[0])] +
                         [str(od / f"{o}.o") for o in ROW_OBJECTS[impl]] + LINK.get(b, []) + ["-o", str(exe)]))
    od = bindir / "obj" / "counting"                     # the TC gate: tc3.c itself, counting build
    cmds.append((bindir / "tc3gate_m2", [CC] + CFLAGS + BUILDS["counting"] + inc() + [str(TC3GATE)] +
                 [str(od / "common.o"), str(od / "m1common.o"), "-o", str(bindir / "tc3gate_m2")]))
    od = bindir / "obj" / "test"                         # unit and differential tests: the test build's objects
    for t, src in UNIT.items():
        exe = bindir / t
        cmds.append((exe, [CC] + CFLAGS + BUILDS["test"] + inc() + [str(src)] + [str(od / f"{o}.o") for o in UNIT_ALL] +
                     ["-o", str(exe)]))
    sd = bindir / "obj" / "asan"                         # and the same under the sanitizers
    for name, (src, extra) in OBJECTS.items():
        cmds.append((sd / f"{name}.o", [CC] + SAN + BUILDS["test"] + extra + inc() + ["-c", str(src), "-o",
                                                                                     str(sd / f"{name}.o")]))
    for t, src in UNIT.items():
        exe = bindir / f"{t}-asan"
        cmds.append((exe, [CC] + SAN + BUILDS["test"] + inc() + [str(src)] + [str(sd / f"{o}.o") for o in UNIT_ALL] +
                     ["-o", str(exe)]))
    for b in ALLOC_BUILDS:                               # the allocation check, in each measured build and the memory build
        od = bindir / "obj" / b
        exe = bindir / f"m3_alloc-{b}"
        cmds.append((exe, [CC] + CFLAGS + BUILDS[b] + inc() + [str(ALLOC_TEST)] + [str(od / f"{o}.o") for o in UNIT_ALL]
                     + WRAP + ["-o", str(exe)]))
    return cmds


def unit_commands(bindir, quick=False):
    """[(name, argv, env)] of step 2: the unit tests (with the large trial divisions of spec v0.3 §16.9 item 6, unless
    `quick`: the harness's own toy checks), the differential test, the allocation check, then the tests under the
    sanitizers."""
    bindir = Path(bindir)
    env = {"ASAN_OPTIONS": "detect_leaks=1"}
    q = ["--quick"] if quick else []
    return ([("m3_unit", [str(bindir / "m3_unit")] + q, None),
             ("m3_diff", [str(bindir / "m3_diff"), "--seeds", DIFF_SEEDS], None)] +
            [(f"m3_alloc-{b}", [str(bindir / f"m3_alloc-{b}")], None) for b in ALLOC_BUILDS] +
            [("m3_unit-asan", [str(bindir / "m3_unit-asan")] + q, env),
             ("m3_diff-asan", [str(bindir / "m3_diff-asan"), "--seeds", DIFF_SEEDS_SAN], env)])
