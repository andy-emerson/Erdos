"""How the harness builds the rows: one compiler, one set of flags (rule item 2), three builds (spec §4.6), two widths.

LIB describes lib/, the library under test. It is a placeholder until lib/README.md says how lib/ is laid out
(headers, sources, build settings, unit tests); reconcile it, and api.h, with that README. STUB describes test/stub/,
used only to test the harness end to end (runner.py --stub, never a measured run).
"""
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIBDIR = HERE.parent / "lib"

CC = "gcc"
CFLAGS = ["-O2", "-std=c11", "-Wall", "-Wextra"]          # rule item 2: one compiler, one set of flags, -O2
IMPL_DEFS = {"ctl": "-DIMPL_CTL", "D1": "-DIMPL_D1", "D2": "-DIMPL_D2", "ceil": "-DIMPL_CEIL"}


def width_defs(w):
    return [f"-DSK_WIDTH={w}"]


STUB = {
    "name": "stub",
    "include": [HERE],
    "defs": ["-DSK_USE_STUB"],
    "mode_defs": {"timing": [], "counting": ["-DSK_COUNTING"], "test": ["-DSK_COUNTING", "-DSK_AUDIT"]},
    "sources": {impl: [HERE / "stub" / "sk_stub.c"] for impl in IMPL_DEFS},
    # unit tests (rule, Order of work, step 2): sources compiled in the test build at each width, then run
    "unit_tests": [[HERE / "stub" / "unit.c", HERE / "stub" / "sk_stub.c"]],
}

# lib/ as laid out on Oct 28, 2026 (lib/Makefile, lib/include/*.h): one library of six sources, built here from
# source with the harness's one set of flags; build settings SK_WIDTH, SK_COUNT, SK_AUDIT. Unit tests: lib/tests/unit.c
# (lib's own `make test` also runs a differential test; the rule asks for the unit tests).
LIB_SRC = [LIBDIR / "src" / f for f in ("common.c", "ctl.c", "d1.c", "d2.c", "ceil.c", "rows.c")]
LIB = {
    "name": "lib",
    "include": [LIBDIR / "include", LIBDIR / "tests", HERE],
    "defs": [],
    "mode_defs": {"timing": [], "counting": ["-DSK_COUNT"], "test": ["-DSK_COUNT", "-DSK_AUDIT"]},
    "sources": {impl: LIB_SRC for impl in IMPL_DEFS},
    "unit_tests": [[LIBDIR / "tests" / "unit.c"] + LIB_SRC],
}

# Code versions (runner.py, code_hashes): every gate and counting result stores the hash of the code it ran. A row's
# "code" hash covers its own files and every shared file (the shared integer module, the lookup and the control's
# divide in common.c, the common headers, the harness driver, the compiler and flags), so a change to a shared file
# changes every row's hash. The shared integer module (spec §4.3) is also hashed alone, as "int".
DRIVER = [HERE / "bench.c", HERE / "api.h"]
LIB["int_module"] = LIBDIR / "src" / "sk_internal.h"
LIB["shared_files"] = DRIVER + [LIBDIR / "src" / f for f in ("sk_internal.h", "common.c", "rows.c")] + \
    [LIBDIR / "include" / f for f in ("sk_common.h", "skeletons.h")]
LIB["own_files"] = {impl: [LIBDIR / "src" / f"{n}.c", LIBDIR / "include" / f"sk_{n}.h"]
                    for impl, n in (("ctl", "ctl"), ("D1", "d1"), ("D2", "d2"), ("ceil", "ceil"))}
STUB["int_module"] = HERE / "stub" / "sk_stub.c"
STUB["shared_files"] = DRIVER + [HERE / "stub" / "sk_stub.c", HERE / "stub" / "sk.h"]
STUB["own_files"] = {impl: [] for impl in IMPL_DEFS}
