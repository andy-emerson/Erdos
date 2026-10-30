"""How round m2's harness builds the rows and the driver (rule-m2 item 2, carried over from rule-m1): one compiler, one
set of flags (gcc -std=c11 -O2, with round 3's warnings), the builds of each row differing only in -D settings, and the
unit, differential, allocation and sanitizer tests, and the TC gate's binary. From m1_libconf.py (unchanged, m1's).

The builds: timing, counting and test (rule-m2 item 2), and the memory build (spec v0.2 §15.6, rule-m2 Harness: "the
memory build tracks the peak"; "the timing build carries neither"): the timing build's flags with -DM2_MEM, linked with
the allocation wrapper. It serves the memory runs only and is never timed (DECISIONS-m2 M2-D10).

The pins (rule-m2 items 2 and 12): m1's, unchanged: the 08 module, tc3.c and its round-3 headers, and tcm.c's recorded
diff from tc3.c (tcm.c is unchanged from m1; spec v0.2 §15.8).
"""
import hashlib
import json
from pathlib import Path

import m1_libconf as L1
import m2_common as C

LIBDIR = C.LIBDIR
SRC = LIBDIR / "src"
INCD = LIBDIR / "include"
V08 = LIBDIR / "vendor08"
TESTS = LIBDIR / "tests"
CC = L1.CC
CFLAGS = list(L1.CFLAGS)
SAN = list(L1.SAN)
INCLUDE = [INCD, V08, SRC, TESTS, C.HERE]
BUILDS = {"timing": [], "counting": ["-DSK_COUNT"], "test": ["-DSK_COUNT", "-DSK_AUDIT"], "memory": ["-DM2_MEM"]}
WRAP = list(L1.WRAP)
LINK = {"memory": WRAP}                                   # the memory build's heap wrapper (bench_m2.c)


def hash_defs(impl):
    g, p = C.HASH_FLAGS[impl]
    return [f"-DMH_GROW={g}", f"-DMH_PACK={p}"]


# the objects, and the -D a source needs beyond the build's
OBJECTS = {"tcm": (SRC / "tcm.c", []), "common": (V08 / "common.c", []), "m1common": (SRC / "m1common.c", []),
           "m2common": (SRC / "m2common.c", [])}
for _i in C.HASH_IMPLS:
    OBJECTS[_i.lower()] = (SRC / "mh2.c", hash_defs(_i))
ROW_OBJECTS = {"TC": ("tcm", "common", "m1common", "m2common")}
for _i in C.HASH_IMPLS:
    ROW_OBJECTS[_i] = (_i.lower(), "common", "m1common", "m2common")
IMPL_DEFS = {"TC": ["-DIMPL_TC"]}
for _i in C.HASH_IMPLS:
    IMPL_DEFS[_i] = ["-DIMPL_HASH"] + hash_defs(_i)
DRIVER = [C.HERE / "bench_m2.c", C.HERE / "api_m2.h"]
TC3GATE = C.HERE / "tc3gate_m2.c"
UNIT = {"m2_unit": TESTS / "m2_unit.c", "m2_diff": TESTS / "m2_diff.c"}
UNIT_ALL = ("tcm",) + tuple(i.lower() for i in C.HASH_IMPLS) + ("common", "m1common", "m2common")
DIFF_SEEDS, DIFF_SEEDS_SAN = "3000", "600"
ALLOC_TEST = TESTS / "m2_alloc.c"
ALLOC_BUILDS = ("timing", "counting", "memory")

# the pins: m1's (rule-m2 item 2: "the 08 module and tc3.c unchanged, their sha256 checked before every run; tcm.c's
# recorded diff checked")
VENDOR08_SHA256 = L1.VENDOR08_SHA256
ROUND3_SHA256 = L1.ROUND3_SHA256
TCM_DIFF = L1.TCM_DIFF
vendor08_check = L1.vendor08_check
tc3_check = L1.tc3_check
tcm_diff_text = L1.tcm_diff_text
tcm_diff_check = L1.tcm_diff_check

# tcm.c and m1's sources, as m1 committed them (6e9a85e): m2 links tcm.c and m1common.c and includes m1.h and m1int.h;
# none of them may change (spec v0.2 §15.8; the m1 runs stay reproducible)
M1_SOURCES_SHA256 = {
    "src/tcm.c": "f85d56c428397ba0781ad06e1f4926356c1fe64f7a642259c5d5bf9e19b5b811",
    "src/mh.c": "9131c05c8044133ea1a197111844b939bf21d80c0fc53f17bec2faabbc8eaa4b",
    "src/m1common.c": "a822a7f8ac8e256f7ce153ba0fb423a89814686d0c293e81bfee62eea0a4707c",
    "src/m1int.h": "990dba19e460110bb55e6f852dc6aabc782e0fc1eba9047b6fcf1ff5783cdf1e",
    "src/tcm.diff": "8ad1ca6683d453a37e1a67e422d48fd84f16ae2b9d2217d2f537f24f101423fd",
    "include/m1.h": "12b17e9cbcde6e3b3c5c0a0b0fe97eeefa23122d816ae633bb378db45a174abf",
    "include/mh.h": "43ac77d688397d38f009fe51d5ed3a575311381c003e92b9de230621e7411be9",
    "include/tcm.h": "19ec26964104212fe4f4088abf6ce276fd5828f03e8ad739bbeabb00dbeb8659",
}

SHARED = [INCD / "m1.h", INCD / "m2.h", INCD / "r2.h", INCD / "r3.h", SRC / "r2int.h", SRC / "m1int.h",
          SRC / "m1common.c", SRC / "m2common.c", V08 / "sk_internal.h", V08 / "common.c", V08 / "sk_common.h"] + DRIVER
OWN = {"TC": [SRC / "tcm.c", INCD / "tcm.h"]}
for _i in C.HASH_IMPLS:
    OWN[_i] = [SRC / "mh2.c", INCD / "mh2.h"]


def m1_sources_check():
    """None, or the first of m1's sources (those m2 builds with, and mh.c) whose sha256 differs from the pinned one."""
    for name, want in M1_SOURCES_SHA256.items():
        got = hashlib.sha256((LIBDIR / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/{name}: sha256 {got} differs from m1's {want}"
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
        exe = bindir / f"m2_alloc-{b}"
        cmds.append((exe, [CC] + CFLAGS + BUILDS[b] + inc() + [str(ALLOC_TEST)] + [str(od / f"{o}.o") for o in UNIT_ALL]
                     + WRAP + ["-o", str(exe)]))
    return cmds


def unit_commands(bindir):
    """[(name, argv, env)] of step 2: the unit and differential tests, the allocation check, then the tests under the
    sanitizers."""
    bindir = Path(bindir)
    env = {"ASAN_OPTIONS": "detect_leaks=1"}
    return ([("m2_unit", [str(bindir / "m2_unit")], None),
             ("m2_diff", [str(bindir / "m2_diff"), "--seeds", DIFF_SEEDS], None)] +
            [(f"m2_alloc-{b}", [str(bindir / f"m2_alloc-{b}")], None) for b in ALLOC_BUILDS] +
            [("m2_unit-asan", [str(bindir / "m2_unit-asan")], env),
             ("m2_diff-asan", [str(bindir / "m2_diff-asan"), "--seeds", DIFF_SEEDS_SAN], env)])
