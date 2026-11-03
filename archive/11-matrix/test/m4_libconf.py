"""How the m4 build is made (test/DECISIONS-m4.md): Hash-V-pack-4 (lib/src/mh4.c, prefix mhv4_, label "Hash-V-pack-4") and
TC with the m4 gcd (lib/src/tcm.c unchanged, built against lib/vendor08/m4/sk_internal.h), beside the reference build
of the same sources as m3 builds them (mh2.c's Hash-V-pack, tcm.c, 08's gcd). One compiler and m3's flags and builds
(timing, counting, test, memory).

The sides:
  ref  the reference: 08's include path; bench_m3.c's TC and Hash-V-pack (mh2.c); and m2's and m3's own C suites
       (unit, differential, allocation, and the unit and differential tests under the sanitizers), unchanged;
  m4   the m4 build: lib/vendor08/m4 first on the include path, so every source that includes "sk_internal.h" from
       lib/src (tcm.c, mh4.c, mh2.c, m1/m2/m3common.c) gets the m4 gcd; bench_m4.c's TC and Hash-V-pack-4; and the m4
       suites (lib/tests/m4_*.c: TC, Hash-V-pack-4 and, as the reference row, mh2.c's Hash-V-pack).
vendor08/common.c includes "sk_internal.h" from its own directory, so it keeps 08's gcd on both sides; its one gcd
(sk_ctl_divide_pair) is on no path of TC or the hash rows.

Nothing here runs anything timed. Usage: python3 m4_libconf.py [--jobs N]  (builds everything into build/m4/)
"""
import argparse
import hashlib
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import m1_libconf as L1
import m2_libconf as L2
import m3_libconf as L3

HERE = Path(__file__).resolve().parent
LIBDIR = L3.LIBDIR
SRC, INCD, V08, TESTS = L3.SRC, L3.INCD, L3.V08, L3.TESTS
V08M4 = V08 / "m4"
OUT = HERE / "build" / "m4"
CC, CFLAGS, SAN, BUILDS, WRAP = L3.CC, L3.CFLAGS, L3.SAN, L3.BUILDS, L3.WRAP

# the pins: 08's (unchanged, L1's) and the m4 set (lib/vendor08/m4/SHA256SUMS): 08's files with sk_internal.h replaced
# by the m4 one. The m4 one is the lifted pin; 08's sk_internal.h keeps its own pin and is still built by every round.
VENDOR08_M4_SHA256 = {"../common.c": L1.VENDOR08_SHA256["common.c"], "../ctl.c": L1.VENDOR08_SHA256["ctl.c"],
                      "../sk_common.h": L1.VENDOR08_SHA256["sk_common.h"],
                      "../sk_ctl.h": L1.VENDOR08_SHA256["sk_ctl.h"],
                      "sk_internal.h": "cdfebe87ded6dcd3f53beb7f4a29beb8c23b0cf3437f57fb5a0372e2bbd80326"}


def pins_check():
    """None, or the first pin that does not hold: 08's (unchanged), m1's and m2's sources (mh2.c, tcm.c, ...), and the
    m4 set."""
    for chk in (L1.vendor08_check, L1.tc3_check, L1.tcm_diff_check, L2.m1_sources_check, L3.m2_sources_check):
        bad = chk()
        if bad:
            return bad
    for name, want in VENDOR08_M4_SHA256.items():
        got = hashlib.sha256((V08M4 / name).read_bytes()).hexdigest()
        if got != want:
            return f"lib/vendor08/m4/{name}: sha256 {got} differs from the m4 pin {want}"
    return None


HASHDEF = ["-DMH_GROW=0", "-DMH_PACK=1"]
COMMON = ("common", "m1common", "m2common", "m3common")
BASE_OBJ = {"tcm": (SRC / "tcm.c", []), "common": (V08 / "common.c", []), "m1common": (SRC / "m1common.c", []),
            "m2common": (SRC / "m2common.c", []), "m3common": (SRC / "m3common.c", []),
            "mhvp": (SRC / "mh2.c", HASHDEF)}
# side: (include path, objects, bench driver, bench rows {impl: (objects, -D)}, tests {name: (source, objects)},
#        allocation tests {name: (source, objects)})
SIDES = {
    "ref": ([INCD, V08, SRC, TESTS, HERE],
            dict(BASE_OBJ, mhvf=(SRC / "mh2.c", ["-DMH_GROW=0", "-DMH_PACK=0"]),
                 mhvgm=(SRC / "mh2.c", ["-DMH_GROW=1", "-DMH_PACK=0"]),
                 mhpp=(SRC / "mh3.c", ["-DMH3_COPRIME=0"]), mhpc=(SRC / "mh3.c", ["-DMH3_COPRIME=1"])),
            HERE / "bench_m3.c",
            {"TC": (("tcm",) + COMMON, ["-DIMPL_TC"]), "MHVP": (("mhvp",) + COMMON, ["-DIMPL_HASH"] + HASHDEF)},
            {"m3_unit": (TESTS / "m3_unit.c", ("tcm", "mhvp", "mhpp", "mhpc") + COMMON),
             "m3_diff": (TESTS / "m3_diff.c", ("tcm", "mhvp", "mhpp", "mhpc") + COMMON),
             "m2_unit": (TESTS / "m2_unit.c", ("tcm", "mhvf", "mhvgm", "mhvp", "common", "m1common", "m2common")),
             "m2_diff": (TESTS / "m2_diff.c", ("tcm", "mhvf", "mhvgm", "mhvp", "common", "m1common", "m2common"))},
            {"m3_alloc": (TESTS / "m3_alloc.c", ("tcm", "mhvp", "mhpp", "mhpc") + COMMON),
             "m2_alloc": (TESTS / "m2_alloc.c", ("tcm", "mhvf", "mhvgm", "mhvp", "common", "m1common", "m2common"))}),
    "m4": ([INCD, V08M4, V08, SRC, TESTS, HERE],
           dict(BASE_OBJ, mhv4=(SRC / "mh4.c", [])),
           HERE / "bench_m4.c",
           {"TC": (("tcm",) + COMMON, ["-DIMPL_TC"]), "MHV4": (("mhv4",) + COMMON, ["-DIMPL_HASH4"])},
           {"m4_unit": (TESTS / "m4_unit.c", ("tcm", "mhv4", "mhvp") + COMMON),
            "m4_diff": (TESTS / "m4_diff.c", ("tcm", "mhv4", "mhvp") + COMMON)},
           {"m4_alloc": (TESTS / "m4_alloc.c", ("tcm", "mhv4", "mhvp") + COMMON)}),
}
DIFF_SEEDS, DIFF_SEEDS_SAN = L3.DIFF_SEEDS, L3.DIFF_SEEDS_SAN


def inc(paths):
    return sum((["-I", str(p)] for p in paths), [])


def build_commands(out=OUT):
    """[(output, argv)] for both sides: objects, bench binaries, and the test binaries (and the sanitizer ones)."""
    cmds = []
    for side, (incl, objs, driver, bench, tests, allocs) in SIDES.items():
        d = out / side
        for b, defs in list(BUILDS.items()) + [("asan", None)]:
            od = d / "obj" / b
            fl = SAN + BUILDS["test"] if b == "asan" else CFLAGS + defs
            for name, (src, extra) in objs.items():
                cmds.append((od / f"{name}.o", [CC] + fl + extra + inc(incl) + ["-c", str(src), "-o", str(od / f"{name}.o")]))
            if b != "asan":
                for impl, (need, idefs) in bench.items():
                    exe = d / f"bench-{impl}-{b}"
                    cmds.append((exe, [CC] + fl + idefs + inc(incl) + [str(driver)] + [str(od / f"{o}.o") for o in need] +
                                 L3.LINK.get(b, []) + ["-o", str(exe)]))
            if b in ("test", "asan"):
                for t, (src, need) in tests.items():
                    exe = d / (t if b == "test" else f"{t}-asan")
                    cmds.append((exe, [CC] + fl + inc(incl) + [str(src)] + [str(od / f"{o}.o") for o in need] +
                                 ["-o", str(exe)]))
            if b in L3.ALLOC_BUILDS:
                for t, (src, need) in allocs.items():
                    exe = d / f"{t}-{b}"
                    cmds.append((exe, [CC] + fl + inc(incl) + [str(src)] + [str(od / f"{o}.o") for o in need] + WRAP +
                                 ["-o", str(exe)]))
    return cmds


def test_commands(out=OUT):
    """[(side, name, argv)]: every C suite of both sides, the sanitizer runs last"""
    out = Path(out)
    cmds = []
    for side, (_, _, _, _, tests, allocs) in SIDES.items():
        d = out / side
        for t in tests:
            cmds.append((side, t, [str(d / t)] + (["--seeds", DIFF_SEEDS] if t.endswith("_diff") else [])))
        for t in allocs:
            for b in L3.ALLOC_BUILDS:
                cmds.append((side, f"{t}-{b}", [str(d / f"{t}-{b}")]))
        for t in tests:
            cmds.append((side, f"{t}-asan", [str(d / f"{t}-asan")] +
                         (["--seeds", DIFF_SEEDS_SAN] if t.endswith("_diff") else [])))
    return cmds


def build(out=OUT, jobs=8):
    bad = pins_check()
    if bad:
        sys.exit(f"pin: {bad}")
    cmds = build_commands(out)
    objs = [c for c in cmds if c[0].suffix == ".o"]
    exes = [c for c in cmds if c[0].suffix != ".o"]

    def run(c):
        c[0].parent.mkdir(parents=True, exist_ok=True)
        p = subprocess.run(c[1], capture_output=True, text=True)
        return c, p

    with ThreadPoolExecutor(jobs) as ex:
        for stage in (objs, exes):
            for c, p in ex.map(run, stage):
                if p.returncode:
                    sys.exit(f"build failed: {' '.join(c[1])}\n{p.stderr}")
    print(f"built {len(cmds)} targets into {out}")


def run_tests(out=OUT):
    """every C suite of both sides, one at a time; returns the number that failed"""
    import os
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1")
    bad = 0
    for side, name, argv in test_commands(out):
        p = subprocess.run(argv, capture_output=True, text=True, env=env)
        last = (p.stdout.strip().splitlines() or ["(no output)"])[-1]
        print(f"{side:4s} {name:22s} {'ok  ' if p.returncode == 0 else 'FAIL'} {last}", flush=True)
        bad += p.returncode != 0
    return bad


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=8)
    ap.add_argument("--test", action="store_true", help="also run every C suite of both sides")
    a = ap.parse_args()
    build(jobs=a.jobs)
    if a.test:
        sys.exit(1 if run_tests() else 0)
