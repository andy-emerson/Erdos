"""How the harness builds the rows: one compiler, one set of flags (rule item 2), three builds (spec §4.6).

The scalar rows (L1, L1-opt, ctl) are built from lib/ (src/l1.c and lib/vendor08/, the 08 module and control copied
unchanged); the library rows (fmpq, mpq, fz) from bench.c alone, linked with FLINT and GMP as installed.
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
             "fmpq": ["-DIMPL_FMPQ"], "mpq": ["-DIMPL_MPQ"], "fz": ["-DIMPL_FZ"]}
SOURCES = {"L1": [LIBDIR / "src" / "l1.c", V08 / "common.c"],
           "L1opt": [LIBDIR / "src" / "l1.c", V08 / "common.c"],
           "ctl": [V08 / "ctl.c", V08 / "common.c"],
           "fmpq": [], "mpq": [], "fz": []}
LINK = {"fmpq": ["-lflint", "-lgmp"], "mpq": ["-lflint", "-lgmp"], "fz": ["-lflint", "-lgmp"]}
MODES = {"L1": ("timing", "counting", "test"), "L1opt": ("timing", "counting", "test"),
         "ctl": ("timing", "counting", "test"), "fmpq": ("timing",), "mpq": ("timing",), "fz": ("timing",)}

# Unit tests (rule item 3, Order of work step 2): layer 1's (with the control where they apply) and the differential
# test, in the test build; and, for ctl, 08's own unit tests, built from 08 at the commit the copies were taken from.
UNIT_OBJECTS = {"l1.o": (LIBDIR / "src" / "l1.c", []), "l1o.o": (LIBDIR / "src" / "l1.c", ["-DL1_OPT"]),
                "common.o": (V08 / "common.c", []), "ctl.o": (V08 / "ctl.c", [])}
UNIT_TESTS = {"unit-layer1": LIBDIR / "tests" / "unit.c", "diff-layer1": LIBDIR / "tests" / "diff.c"}
DIFF_SEEDS = "200"
COMMIT_08 = "08bbfbe"
LIB08_SOURCES = ["src/common.c", "src/ctl.c", "src/d1.c", "src/d2.c", "src/ceil.c", "src/rows.c"]

# sha256 of the 08 files as copied from 08-skeletons at commit 08bbfbe (lib/vendor08/SHA256SUMS): spec §4.3 item 4,
# rule item 2. Any other content is refused.
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
SHARED = {"L1": DRIVER + SCALAR_SHARED, "L1opt": DRIVER + SCALAR_SHARED, "ctl": DRIVER + SCALAR_SHARED,
          "fmpq": list(DRIVER), "mpq": list(DRIVER), "fz": list(DRIVER)}
OWN = {"L1": [LIBDIR / "src" / "l1.c"], "L1opt": [LIBDIR / "src" / "l1.c"], "ctl": [V08 / "ctl.c"],
       "fmpq": [], "mpq": [], "fz": []}


def vendor08_check():
    """None, or the first 08 file whose sha256 differs from the copy's recorded hash."""
    for name, want in VENDOR08_SHA256.items():
        got = hashlib.sha256((V08 / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/vendor08/{name}: sha256 {got} differs from 08's {want}"
    return None
