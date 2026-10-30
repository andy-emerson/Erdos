"""How the matrix round's harness builds the rows and the driver (rule-m1 item 2): one compiler, one set of flags (gcc
-std=c11 -O2, with round 3's warnings), three builds of each row (timing, counting, test) differing only in -D settings,
and the unit, differential, allocation and sanitizer tests, and the TC gate's binary. From round 3's r3_libconf.py.

The pins (rule-m1 items 2 and 12): the 08 module (lib/vendor08/, copied unchanged from round 3, whose hashes are
round 1's), tc3.c (copied unchanged; also checked at ../10-interning/lib/src/tc3.c), the round-3 headers it needs
(copied unchanged), and tcm.c's diff from tc3.c, recorded in lib/src/tcm.diff and checked byte for byte.
"""
import difflib
import hashlib
import json
from pathlib import Path

import m1_common as C

LIBDIR = C.LIBDIR
SRC = LIBDIR / "src"
INCD = LIBDIR / "include"
V08 = LIBDIR / "vendor08"
TESTS = LIBDIR / "tests"
CC = "gcc"
CFLAGS = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O2"]
SAN = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g", "-fsanitize=address,undefined",
       "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
INCLUDE = [INCD, V08, SRC, TESTS, C.HERE]
BUILDS = {"timing": [], "counting": ["-DSK_COUNT"], "test": ["-DSK_COUNT", "-DSK_AUDIT"]}   # no profiling build (M-9)


def hash_defs(impl):
    m, g, mp = C.HASH_FLAGS[impl]
    return [f"-DMH_M={m}", f"-DMH_GROW={g}", f"-DMH_MAP={mp}"]


# the objects, and the -D a source needs beyond the build's
OBJECTS = {"tcm": (SRC / "tcm.c", []), "common": (V08 / "common.c", []), "m1common": (SRC / "m1common.c", [])}
for _i in C.HASH_IMPLS:
    OBJECTS[_i.lower()] = (SRC / "mh.c", hash_defs(_i))
ROW_OBJECTS = {"TC": ("tcm", "common", "m1common")}
for _i in C.HASH_IMPLS:
    ROW_OBJECTS[_i] = (_i.lower(), "common", "m1common")
IMPL_DEFS = {"TC": ["-DIMPL_TC"]}
for _i in C.HASH_IMPLS:
    IMPL_DEFS[_i] = ["-DIMPL_HASH"] + hash_defs(_i)
DRIVER = [C.HERE / "bench_m1.c", C.HERE / "api_m1.h"]
TC3GATE = C.HERE / "tc3gate.c"
UNIT = {"m1_unit": TESTS / "m1_unit.c", "m1_diff": TESTS / "m1_diff.c"}
UNIT_ALL = ("tcm",) + tuple(i.lower() for i in C.HASH_IMPLS) + ("common", "m1common")
DIFF_SEEDS, DIFF_SEEDS_SAN = "3000", "600"
ALLOC_TEST = TESTS / "m1_alloc.c"
ALLOC_BUILDS = ("timing", "counting")
WRAP = ["-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free"]

VENDOR08_SHA256 = {
    "common.c": "8f1d177ec1c5cd5db9ce9927f1260354b8aaa216554788a72d89c3d097f58043",
    "ctl.c": "34bb98fd7e6a6e83bb92aa55912cd1263acb715bdb17e0b024e8b96a7f14d3e4",
    "sk_common.h": "f608a67b018792b9ee9fbe98d7589d2e6c4e0fef87bc04d4dc385962c7e83dab",
    "sk_ctl.h": "9414e8e10f126b4432eb61de09282a8268cc61f47c14c322a0f01cd56a730aa5",
    "sk_internal.h": "166eba893b7ecd288193ec2fd37ee303c44e3e20c77226beda76795a19b17854",
}

# tc3.c, the base of tcm.c's diff (spec §4), and the round-3 headers it is built with: copied unchanged from
# ../10-interning/lib (as committed at 80d06ee); each checked here and in round 3's tree
ROUND3_SHA256 = {
    "src/tc3.c": "7956bc5f9ebc72f4e12b604ec1a5ab743d3ed75602cb075d6fb902035013494b",
    "src/r2int.h": "1e2980608d211af26356c6a8b29e5e9ee991032a8207c22f22c1a6b7316502f6",
    "include/r2.h": "3a587d51ca4e6aeea59cb16e38b6fd5ed57eee976b875904435cb2cac1cf90e2",
    "include/r3.h": "5916b8f6989ae8e88803d583a714d348cd65b0eda888358f4db3f71f6decc265",
    "include/tc.h": "180ba7ff2567b4e1a7741c485d5384fb36ca604a4685c358d186204cb5584389",
}
TCM_DIFF = SRC / "tcm.diff"

SHARED = [INCD / "m1.h", INCD / "r2.h", INCD / "r3.h", SRC / "r2int.h", SRC / "m1int.h", SRC / "m1common.c",
          V08 / "sk_internal.h", V08 / "common.c", V08 / "sk_common.h"] + DRIVER
OWN = {"TC": [SRC / "tcm.c", INCD / "tcm.h"]}
for _i in C.HASH_IMPLS:
    OWN[_i] = [SRC / "mh.c", INCD / "mh.h"]


def vendor08_check():
    """None, or the first 08 file whose sha256 differs from the recorded one (rule-m1 item 2)."""
    for name, want in VENDOR08_SHA256.items():
        got = hashlib.sha256((V08 / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/vendor08/{name}: sha256 {got} differs from 08's {want}"
    return None


def tc3_check():
    """None, or the first of tc3.c and its headers whose sha256 differs from round 3's, here or in ../10-interning."""
    for name, want in ROUND3_SHA256.items():
        for base in (LIBDIR, C.ROUND3 / "lib"):
            got = hashlib.sha256((base / name).read_bytes()).hexdigest()
            if got != want:
                return f"{base / name}: sha256 {got} differs from round 3's {want}"
    return None


def tcm_diff_text():
    """tcm.c's diff from tc3.c (unified, 3 lines of context), as recorded in lib/src/tcm.diff"""
    a = (SRC / "tc3.c").read_text().splitlines(keepends=True)
    b = (SRC / "tcm.c").read_text().splitlines(keepends=True)
    return "".join(difflib.unified_diff(a, b, "lib/src/tc3.c", "lib/src/tcm.c"))


def tcm_diff_check():
    """None, or why tcm.c's diff from tc3.c is not the recorded, reviewed one (rule-m1 item 2)."""
    if not TCM_DIFF.exists():
        return "lib/src/tcm.diff (tcm.c's recorded diff from tc3.c) is missing"
    if tcm_diff_text() != TCM_DIFF.read_text():
        return "tcm.c's diff from tc3.c differs from the recorded lib/src/tcm.diff"
    return None


def code_hashes():
    """{impl: sha256 of its own files, the shared files, the compiler and the flags}: a result made with other code is
    stale (round 1's R-o)."""
    out = {}
    for impl in C.IMPLS:
        s = hashlib.sha256(json.dumps([CC, CFLAGS, BUILDS, IMPL_DEFS[impl],
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
                         [str(od / f"{o}.o") for o in ROW_OBJECTS[impl]] + ["-o", str(exe)]))
    od = bindir / "obj" / "counting"                     # the TC gate: tc3.c itself, counting build
    cmds.append((bindir / "tc3gate", [CC] + CFLAGS + BUILDS["counting"] + inc() + [str(TC3GATE)] +
                 [str(od / "common.o"), str(od / "m1common.o"), "-o", str(bindir / "tc3gate")]))
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
    for b in ALLOC_BUILDS:                               # the allocation check, in each measured build
        od = bindir / "obj" / b
        exe = bindir / f"m1_alloc-{b}"
        cmds.append((exe, [CC] + CFLAGS + BUILDS[b] + inc() + [str(ALLOC_TEST)] + [str(od / f"{o}.o") for o in UNIT_ALL]
                     + WRAP + ["-o", str(exe)]))
    return cmds


def unit_commands(bindir):
    """[(name, argv, env)] of step 2: the unit and differential tests, the allocation check, then the tests under the
    sanitizers."""
    bindir = Path(bindir)
    env = {"ASAN_OPTIONS": "detect_leaks=1"}
    return ([("m1_unit", [str(bindir / "m1_unit")], None),
             ("m1_diff", [str(bindir / "m1_diff"), "--seeds", DIFF_SEEDS], None)] +
            [(f"m1_alloc-{b}", [str(bindir / f"m1_alloc-{b}")], None) for b in ALLOC_BUILDS] +
            [("m1_unit-asan", [str(bindir / "m1_unit-asan")], env),
             ("m1_diff-asan", [str(bindir / "m1_diff-asan"), "--seeds", DIFF_SEEDS_SAN], env)])
