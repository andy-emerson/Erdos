"""How round 2's harness builds the rows and the driver (rule-r2 item 2; spec v0.2 §6 item 4): one compiler, one set
of flags (gcc -std=c11 -O2, with lib/Makefile's warnings), four builds of each row differing only in -D settings, and
the unit, differential and sanitizer tests. Each source is compiled to its own object (Q32's lookup, src/plk.c, with
-DL1_Q32, unchanged and never inlined into q32.c: spec R2-2), then linked with the driver bench_r2.c.

The 08 module's hashes are round 1's (libconf.VENDOR08_SHA256, lib/vendor08/SHA256SUMS), read here without importing
round 1's harness.
"""
import hashlib
import json
from pathlib import Path

import r2_common as C

LIBDIR = C.LIBDIR
SRC = LIBDIR / "src"
V08 = LIBDIR / "vendor08"
CC = "gcc"
CFLAGS = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O2"]     # lib/Makefile's CSTD, WARN and OPT
SAN = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g", "-fsanitize=address,undefined",
       "-fno-omit-frame-pointer", "-fno-sanitize-recover=all"]
INCLUDE = [LIBDIR / "include", V08, SRC, C.HERE]
BUILDS = {"timing": [], "counting": ["-DSK_COUNT"], "test": ["-DSK_COUNT", "-DSK_AUDIT"], "profiling": ["-DSK_PROFILE"]}
# the objects of each row, and the -D a source needs beyond the build's
OBJECTS = {"q32": (SRC / "q32.c", []), "plk": (SRC / "plk.c", ["-DL1_Q32"]), "tc": (SRC / "tc.c", []),
           "r2prof": (SRC / "r2prof.c", []), "common": (V08 / "common.c", [])}
ROW_OBJECTS = {"Q32": ("q32", "plk", "r2prof", "common"), "TC": ("tc", "r2prof", "common")}
IMPL_DEFS = {"Q32": ["-DIMPL_Q32"], "TC": ["-DIMPL_TC"]}
DRIVER = [C.HERE / "bench_r2.c", C.HERE / "api_r2.h"]
TESTS = {"r2_unit": LIBDIR / "tests" / "r2_unit.c", "r2_diff": LIBDIR / "tests" / "r2_diff.c"}
DIFF_SEEDS, DIFF_SEEDS_SAN = "300", "60"
UNIT_ALL = ("q32", "plk", "tc", "r2prof", "common")

VENDOR08_SHA256 = {
    "common.c": "8f1d177ec1c5cd5db9ce9927f1260354b8aaa216554788a72d89c3d097f58043",
    "ctl.c": "34bb98fd7e6a6e83bb92aa55912cd1263acb715bdb17e0b024e8b96a7f14d3e4",
    "sk_common.h": "f608a67b018792b9ee9fbe98d7589d2e6c4e0fef87bc04d4dc385962c7e83dab",
    "sk_ctl.h": "9414e8e10f126b4432eb61de09282a8268cc61f47c14c322a0f01cd56a730aa5",
    "sk_internal.h": "166eba893b7ecd288193ec2fd37ee303c44e3e20c77226beda76795a19b17854",
}

SHARED = [LIBDIR / "include" / "r2.h", SRC / "r2int.h", SRC / "r2prof.c", V08 / "sk_internal.h", V08 / "common.c",
          V08 / "sk_common.h"] + DRIVER
OWN = {"Q32": [SRC / "q32.c", SRC / "plk.c", SRC / "plk.h", LIBDIR / "include" / "q32.h"],
       "TC": [SRC / "tc.c", LIBDIR / "include" / "tc.h"]}


def vendor08_check():
    """None, or the first 08 file whose sha256 differs from round 1's recorded hash (spec v0.2 §5)."""
    for name, want in VENDOR08_SHA256.items():
        got = hashlib.sha256((V08 / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/vendor08/{name}: sha256 {got} differs from 08's {want}"
    return None


def code_hashes():
    """{impl: sha256 of its own files, the shared files, the compiler and the flags}: a result made with other code is
    stale (round 1's R-o)."""
    out = {}
    for impl in C.IMPLS:
        s = hashlib.sha256(json.dumps([CC, CFLAGS, BUILDS, IMPL_DEFS[impl], {k: v[1] for k, v in OBJECTS.items()}])
                           .encode())
        for p in sorted(set(SHARED + OWN[impl]), key=str):
            s.update(Path(p).name.encode() + b"\0" + Path(p).read_bytes() + b"\0")
        out[impl] = s.hexdigest()
    return out


def inc():
    return sum((["-I", str(p)] for p in INCLUDE), [])


def build_commands(bindir):
    """[(output, argv)] for every object, bench binary and test binary, in order."""
    bindir = Path(bindir)
    cmds = []
    for b, defs in BUILDS.items():
        od = bindir / "obj" / b
        for name, (src, extra) in OBJECTS.items():
            cmds.append((od / f"{name}.o", [CC] + CFLAGS + defs + extra + inc() + ["-c", str(src), "-o",
                                                                                  str(od / f"{name}.o")]))
        for impl in C.IMPLS:
            exe = bindir / f"bench-{impl}-{b}"
            cmds.append((exe, [CC] + CFLAGS + defs + IMPL_DEFS[impl] + inc() + [str(C.HERE / "bench_r2.c")] +
                         [str(od / f"{o}.o") for o in ROW_OBJECTS[impl]] + ["-o", str(exe)]))
    # unit and differential tests: the test build's objects; and the same under the sanitizers
    od = bindir / "obj" / "test"
    for t, src in TESTS.items():
        exe = bindir / t
        cmds.append((exe, [CC] + CFLAGS + BUILDS["test"] + inc() + ["-I", str(LIBDIR / "tests"), str(src)] +
                     [str(od / f"{o}.o") for o in UNIT_ALL] + ["-o", str(exe)]))
    sd = bindir / "obj" / "asan"
    for name, (src, extra) in OBJECTS.items():
        cmds.append((sd / f"{name}.o", [CC] + SAN + BUILDS["test"] + extra + inc() + ["-c", str(src), "-o",
                                                                                     str(sd / f"{name}.o")]))
    for t, src in TESTS.items():
        exe = bindir / f"{t}-asan"
        cmds.append((exe, [CC] + SAN + BUILDS["test"] + inc() + ["-I", str(LIBDIR / "tests"), str(src)] +
                     [str(sd / f"{o}.o") for o in UNIT_ALL] + ["-o", str(exe)]))
    return cmds


def unit_commands(bindir):
    """[(name, argv, env)] of step 2: the unit and differential tests, then the same under the sanitizers."""
    bindir = Path(bindir)
    env = {"ASAN_OPTIONS": "detect_leaks=1"}
    return [("r2_unit", [str(bindir / "r2_unit")], None),
            ("r2_diff", [str(bindir / "r2_diff"), "--seeds", DIFF_SEEDS], None),
            ("r2_unit-asan", [str(bindir / "r2_unit-asan")], env),
            ("r2_diff-asan", [str(bindir / "r2_diff-asan"), "--seeds", DIFF_SEEDS_SAN], env)]
