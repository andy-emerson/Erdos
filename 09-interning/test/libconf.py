"""How the harness builds the rows: one compiler, one set of flags (rule item 2), three builds (spec §4.6).

The scalar rows (L1, L1-opt, L1-Q32, L1-opt-Q32, ctl) are built from lib/ (src/l1.c and lib/vendor08/, the 08 module
and control copied unchanged); the Q32 rows are the same source with -DL1_Q32 (spec §18.2). The lookup gate's build
(L1g: L1 with the Q32 rows' parameterized lookup at 32-bit slots, spec §18.1 item 4) is a test build only, run in the
gates and never timed. L1-Q32's lookup and the gate's (src/plk.c) are compiled as their own translation unit, as
L1-Q64's (sk_lookup, vendor08/common.c) is, and called from l1.c across it (lib D-25). The library rows (fmpq, mpq, fz) are built from bench.c alone, linked with FLINT and GMP as
installed.
"""
import hashlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
LIBDIR = HERE.parent / "lib"
V08 = LIBDIR / "vendor08"

CC = "gcc"
CFLAGS = ["-O2", "-std=c11", "-Wall", "-Wextra"]          # rule item 2: one compiler, one set of flags, -O2
INCLUDE = [LIBDIR / "include", V08, HERE]
MODE_DEFS = {"timing": [], "counting": ["-DSK_COUNT"], "test": ["-DSK_COUNT", "-DSK_AUDIT"]}
IMPL_DEFS = {"L1": ["-DIMPL_L1"], "L1opt": ["-DIMPL_L1O", "-DL1_OPT"], "ctl": ["-DIMPL_CTL"],
             "L1q": ["-DIMPL_L1Q", "-DL1_Q32"], "L1oq": ["-DIMPL_L1OQ", "-DL1_OPT", "-DL1_Q32"],
             "L1g": ["-DIMPL_L1G", "-DL1_LKGATE"],
             "fmpq": ["-DIMPL_FMPQ"], "mpq": ["-DIMPL_MPQ"], "fz": ["-DIMPL_FZ"]}
SOURCES = {"L1": [LIBDIR / "src" / "l1.c", V08 / "common.c"],
           "L1opt": [LIBDIR / "src" / "l1.c", V08 / "common.c"],
           "L1q": [LIBDIR / "src" / "l1.c", LIBDIR / "src" / "plk.c", V08 / "common.c"],
           "L1oq": [LIBDIR / "src" / "l1.c", V08 / "common.c"],
           "L1g": [LIBDIR / "src" / "l1.c", LIBDIR / "src" / "plk.c", V08 / "common.c"],
           "ctl": [V08 / "ctl.c", V08 / "common.c"],
           "fmpq": [], "mpq": [], "fz": []}
LINK = {"fmpq": ["-lflint", "-lgmp"], "mpq": ["-lflint", "-lgmp"], "fz": ["-lflint", "-lgmp"]}
MODES = {"L1": ("timing", "counting", "test"), "L1opt": ("timing", "counting", "test"),
         "L1q": ("timing", "counting", "test"), "L1oq": ("timing", "counting", "test"), "L1g": ("test",),
         "ctl": ("timing", "counting", "test"), "fmpq": ("timing",), "mpq": ("timing",), "fz": ("timing",)}

# Unit tests (rule item 3, Order of work step 2): layer 1's (with the control where they apply), at both widths (the
# same tests compiled with -DT_Q32 drive the Q32 rows), the differential test at both widths, the Q32 test (the Q32
# rows against the Q64 rows, capacity, the lookup gate in miniature), all in the test build; and, for ctl, 08's own
# unit tests, built from 08 at the commit the copies were taken from.
UNIT_OBJECTS = {"l1.o": (LIBDIR / "src" / "l1.c", []), "l1o.o": (LIBDIR / "src" / "l1.c", ["-DL1_OPT"]),
                "l1q.o": (LIBDIR / "src" / "l1.c", ["-DL1_Q32"]),
                "l1oq.o": (LIBDIR / "src" / "l1.c", ["-DL1_OPT", "-DL1_Q32"]),
                "l1g.o": (LIBDIR / "src" / "l1.c", ["-DL1_LKGATE"]),
                "plkq.o": (LIBDIR / "src" / "plk.c", ["-DL1_Q32"]),
                "plkg.o": (LIBDIR / "src" / "plk.c", ["-DL1_LKGATE"]),
                "common.o": (V08 / "common.c", []), "ctl.o": (V08 / "ctl.c", [])}
UNIT_TESTS = {"unit-layer1": (LIBDIR / "tests" / "unit.c", []),
              "unit-layer1-q32": (LIBDIR / "tests" / "unit.c", ["-DT_Q32"]),
              "diff-layer1": (LIBDIR / "tests" / "diff.c", []),
              "diff-layer1-q32": (LIBDIR / "tests" / "diff.c", ["-DT_Q32"]),
              "q32-layer1": (LIBDIR / "tests" / "q32.c", [])}
DIFF_SEEDS = "200"
COMMIT_08 = "08bbfbe"
LIB08_SOURCES = ["src/common.c", "src/ctl.c", "src/d1.c", "src/d2.c", "src/ceil.c", "src/rows.c"]

# sha256 of the 08 files as copied from 08-skeletons at commit 08bbfbe (lib/vendor08/SHA256SUMS): spec §4.3 item 4,
# rule item 2. Any other content is refused. L1 (Q64) uses 08's lookup (sk_lookup, common.c) through these files.
VENDOR08_SHA256 = {
    "common.c": "8f1d177ec1c5cd5db9ce9927f1260354b8aaa216554788a72d89c3d097f58043",
    "ctl.c": "34bb98fd7e6a6e83bb92aa55912cd1263acb715bdb17e0b024e8b96a7f14d3e4",
    "sk_common.h": "f608a67b018792b9ee9fbe98d7589d2e6c4e0fef87bc04d4dc385962c7e83dab",
    "sk_ctl.h": "9414e8e10f126b4432eb61de09282a8268cc61f47c14c322a0f01cd56a730aa5",
    "sk_internal.h": "166eba893b7ecd288193ec2fd37ee303c44e3e20c77226beda76795a19b17854",
}
VENDOR08_PATH = {"common.c": "src/common.c", "ctl.c": "src/ctl.c", "sk_common.h": "include/sk_common.h",
                 "sk_ctl.h": "include/sk_ctl.h", "sk_internal.h": "src/sk_internal.h"}

# Code versions (08 harness R-o): a row's "code" hash covers its own files and every shared file, the compiler and the
# flags; the shared integer module is also hashed alone ("int").
DRIVER = [HERE / "bench.c", HERE / "api.h"]
INT_MODULE = V08 / "sk_internal.h"
SCALAR_SHARED = [V08 / "sk_internal.h", V08 / "common.c", V08 / "sk_common.h", V08 / "sk_ctl.h",
                 LIBDIR / "include" / "l1.h"]
SHARED = {"L1": DRIVER + SCALAR_SHARED, "L1opt": DRIVER + SCALAR_SHARED, "L1q": DRIVER + SCALAR_SHARED,
          "L1oq": DRIVER + SCALAR_SHARED, "L1g": DRIVER + SCALAR_SHARED, "ctl": DRIVER + SCALAR_SHARED,
          "fmpq": list(DRIVER), "mpq": list(DRIVER), "fz": list(DRIVER)}
PLK = [LIBDIR / "src" / "plk.c", LIBDIR / "src" / "plk.h"]
OWN = {"L1": [LIBDIR / "src" / "l1.c"], "L1opt": [LIBDIR / "src" / "l1.c"], "L1q": [LIBDIR / "src" / "l1.c"] + PLK,
       "L1oq": [LIBDIR / "src" / "l1.c"], "L1g": [LIBDIR / "src" / "l1.c"] + PLK, "ctl": [V08 / "ctl.c"],
       "fmpq": [], "mpq": [], "fz": []}


def vendor08_check():
    """None, or the first 08 file whose sha256 differs from the copy's recorded hash."""
    for name, want in VENDOR08_SHA256.items():
        got = hashlib.sha256((V08 / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/vendor08/{name}: sha256 {got} differs from 08's {want}"
    return None
