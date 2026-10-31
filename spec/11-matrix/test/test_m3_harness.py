"""Tests of round m3's harness (not measurements; nothing here is timed or reported as a time). From test_m2_harness.py
(unchanged, m2's), with every test that applies carried over and adapted to this round's five rows and reduced grid,
and new ones for the factored rows.

    python3 test_m3_harness.py            the self-tests: the fixed rule and its pin; the 08 module, tc3.c, tcm.c's
                                          recorded diff, m1's and m2's sources and round 3's results; the places the
                                          harness may not write (m1's and m2's results, other m3 runs); the workloads
                                          of the four job sets on rule-m3's grid (add and mul equal to m1's, sop and pos
                                          equal to m2's); the model worked by hand (trial division in closed form
                                          against a plain loop, coprime bases, refinement, the factored arithmetic);
                                          tcm.c and mh2.c unchanged, mh3.c's store, pack and value arithmetic m2's line
                                          for line; the two factored rows from one source; schoolbook trial division;
                                          no handle test for 0 or 1; the allocations (only the named structures, (f),
                                          (q), (r) among them); the C suites; the binaries' exact checks on toy
                                          workloads of every set and on the splitting examples (every row, every list,
                                          build table, handle field, (f) and (q), the TC gate, call for call in its
                                          adapted form); the runner's untimed steps; statistics and the pooled
                                          bootstrap; the pilot's projection (rule-m3's form) and stops; fork/exec with
                                          the affinity set before exec; the kernel timer; the idle check; the
                                          disturbance check; the timed block's machinery with a stand-in driver (five
                                          rows, rotated; over cap); the runner's ticks per launch; the analysis (six
                                          pairings, the eight headline measures, over cap with its magnitude, the
                                          reported pairing, the bands, the factored rows' gcd work and factor work,
                                          strata without sop's MID, grid); and a toy run of each job set end to end
                                          (--toy; add with --pilot-only then --resume)
    python3 test_m3_harness.py --full     also every gate and counting run on the full-size workloads of every set
                                          (test and counting builds; untimed; long)
"""
import argparse
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
from math import gcd
from pathlib import Path

import m1_gen
import m2_gen
import m3_analyze as A
import m3_checks as K
import m3_common as C
import m3_gen
import m3_libconf as L
import m3_model
import m3_runner as RN

TB = C.HERE / "build" / "m3test"
BIN = TB / "bin"


def ns(**kw):
    d = dict(op="add", toy=True, jobs="", pilot_only=False, hard_stop_s=None, resume=False, rerun=None)
    d.update(kw)
    return argparse.Namespace(**d)


def build_stamp():
    """the rows' code hashes and the C test sources': a change to either rebuilds"""
    return json.dumps([L.code_hashes(), {p.name: C.sha256(p) for p in sorted(L.TESTS.iterdir()) if p.is_file()}])


def build_once():
    if (BIN / ".built").exists() and (BIN / ".built").read_text() == build_stamp():
        return
    BIN.mkdir(parents=True, exist_ok=True)
    for out, cmd in L.build_commands(BIN):
        out.parent.mkdir(parents=True, exist_ok=True)
        r = subprocess.run(cmd, capture_output=True, text=True)
        assert r.returncode == 0, f"{' '.join(cmd)}\n{r.stderr}"
    (BIN / ".built").write_text(build_stamp())


def toy_work(op):
    w = TB / f"work-{op}"
    if not (w / "gen.json").exists() or json.loads((w / "gen.json").read_text()).get("1", {}).get("factored") is None:
        m3_gen.generate(w, op, list(C.JOBS_OF[op]), toy=True)
    return w


def run(args, **kw):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True, **kw)


# ------------------------------------------------------------------------------------------------- the rule, pins
def test_rule_fixed_and_pinned():
    """the rule on disk is the pinned, approved one (committed at dcee959, no proposal mark); another text, or one with
    a proposal mark, is refused for a measured run of every job set"""
    text = C.RULE_FILE.read_text()
    assert C.RULE_SHA256 == "c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd"
    assert C.sha256(C.RULE_FILE) == C.RULE_SHA256, "rule-m3.md differs from the pinned hash"
    assert C.PROPOSAL_MARK not in text and C.rule_check() is None, C.rule_check()
    assert "**Fixed: approved by the author, Oct 30, 2026,**" in text
    r = run(["git", "log", "--format=%h", "-1", "--", str(C.RULE_FILE)], cwd=C.HERE)
    assert r.stdout.strip() == C.RULE_COMMIT == "dcee959", r.stdout      # committed once, unchanged since
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule-m3.md"
        p.write_text(text + " ")
        assert "sha256" in C.rule_check(p)
        p.write_text(text.replace("approved by the author, Oct 30, 2026", "PROPOSED", 1))
        assert "PROPOSED" in C.rule_check(p)
    for pin in ("0" * 64, None):
        saved, savedf = C.RULE_SHA256, C.RULE_FILE
        try:
            if pin:
                C.RULE_SHA256 = pin
            else:                                          # the approved text with a proposal mark put back
                tf = Path(tempfile.mkdtemp()) / "rule-m3.md"
                tf.write_text(text.replace("approved by the author, Oct 30, 2026", "PROPOSED", 1))
                C.RULE_FILE = tf
            for op in C.OPS:
                try:
                    RN.Runner(ns(op=op, toy=False)).preflight()
                except SystemExit as e:
                    assert "refusing a measured run" in str(e)
                else:
                    raise AssertionError("a measured run under another rule was not refused")
        finally:
            C.RULE_SHA256, C.RULE_FILE = saved, savedf


def test_pins_m1_m2():
    """the 08 module, tc3.c and its headers, tcm.c's recorded diff, m1's sources (at 6e9a85e), m2's sources and harness
    (at e6505aa) and round 3's results are as pinned; a change to any of them is caught and refused"""
    assert L.vendor08_check() is None and L.tc3_check() is None and L.tcm_diff_check() is None
    assert L.m1_sources_check() is None, L.m1_sources_check()
    assert L.m2_sources_check() is None, L.m2_sources_check()
    assert C.round3_check() is None, C.round3_check()
    for name in L.M1_SOURCES_SHA256:
        r = run(["git", "show", f"6e9a85e:11-matrix/lib/{name}"], cwd=C.HERE)
        assert r.returncode == 0 and r.stdout == (L.LIBDIR / name).read_text(), name
    for name in L.M2_SOURCES_SHA256:                     # as m2 committed them, at its results commit
        r = run(["git", "show", f"{L.M2_COMMIT}:11-matrix/{name}"], cwd=C.HERE)
        assert r.returncode == 0 and r.stdout == (C.ROOT / name).read_text(), name
    saved = C.ROOT
    with tempfile.TemporaryDirectory() as t:
        fake = Path(t)
        for name in L.M2_SOURCES_SHA256:
            (fake / name).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(C.ROOT / name, fake / name)
        try:
            C.ROOT = fake
            assert L.m2_sources_check() is None
            (fake / "lib/src/mh2.c").write_text((fake / "lib/src/mh2.c").read_text() + "\n")
            assert "mh2.c" in L.m2_sources_check()
        finally:
            C.ROOT = saved
    for op in C.OPS:                                     # the runner refuses when m1's or m2's sources changed
        for pins, key, why in ((L.M1_SOURCES_SHA256, "src/tcm.c", "m1's sources changed"),
                               (L.M2_SOURCES_SHA256, "lib/src/mh2.c", "m2's sources changed")):
            keep = dict(pins)
            try:
                pins[key] = "0" * 64
                try:
                    RN.Runner(ns(op=op)).preflight()
                except SystemExit as e:
                    assert why in str(e), str(e)
                else:
                    raise AssertionError(f"a run with {why} was not refused")
            finally:
                pins.clear()
                pins.update(keep)


def test_forbidden_places():
    """nothing is written anywhere in ../10-interning/ or in m1's or m2's results folders (runner, generator,
    analysis); a run writes in no other m3 run's folder; those folders are hashed before and after each run"""
    places = [C.ROUND3, C.ROUND3 / "test" / "results-r3", C.M1_RESULTS["add"], C.M1_RESULTS["mul"] / "x",
              C.M2_RESULTS["sop"], C.M2_RESULTS["pos"] / "x"]
    for p in places:
        for op in (None,) + C.OPS:
            assert C.forbidden(p, op), (p, op)
        r = RN.Runner(ns(toy=True))
        r.out = p
        try:
            r.preflight()
        except SystemExit as e:
            assert "10-interning" in str(e) or "m1's" in str(e) or "m2's" in str(e)
        else:
            raise AssertionError(f"the runner would write into {p}")
        for f in (lambda: A.report(p), lambda: m3_gen.generate(p, "add", [1], toy=True)):
            try:
                f()
            except SystemExit as e:
                assert "10-interning" in str(e) or "m1's" in str(e) or "m2's" in str(e)
            else:
                raise AssertionError(f"would write into {p}")
    for op in C.OPS:
        assert not C.forbidden(C.RESULTS[op], op) and C.RESULTS[op].name == f"results-m3-{op}"
        for other in C.OPS:
            if other != op:
                assert C.forbidden(C.RESULTS[other], op) and C.forbidden(C.RESULTS[other] / "x", op)
                r = RN.Runner(ns(op=op, toy=True))
                r.out = C.RESULTS[other]
                try:
                    r.preflight()
                except SystemExit as e:
                    assert f"m3 {other} run's folder" in str(e)
                else:
                    raise AssertionError(f"the {op} run would write into the {other} run's folder")
        prot = C.protected(op)
        assert set(prot) == {"m1-add", "m1-mul"} | {f"m2-{o}" for o in C.OPS} | {f"m3-{o}" for o in C.OPS if o != op}
    for k in C.OPS:
        assert {"results.md", "state.json"} <= set(C.folder_hashes(C.M2_RESULTS[k]))


def test_runner_imports_no_numpy():
    r = run([sys.executable, "-c", "import sys, m3_runner; print('numpy' in sys.modules, 'sympy' in sys.modules)"],
            cwd=C.HERE)
    assert r.stdout.strip() == "False False", r.stdout + r.stderr


def test_runner_arguments():
    """--op is required (add, mul, sop or pos; no option runs the four in one command); --hard-stop-s and --jobs are for
    toy runs; jobs outside the set's grid are refused; --resume is refused after the hard stop or the pilot's stop,
    once a timed round has run, and on another job set's state; --rerun TC, HASH or FAC"""
    r = run([sys.executable, str(C.HERE / "m3_runner.py"), "--toy"])
    assert r.returncode != 0 and "--op" in r.stderr
    for bad in ("div", "all"):
        r = run([sys.executable, str(C.HERE / "m3_runner.py"), "--op", bad, "--toy"])
        assert r.returncode != 0 and "invalid choice" in r.stderr
    r = run([sys.executable, str(C.HERE / "m3_runner.py"), "--help"])
    assert "--all" not in r.stdout and "--pilot-only" in r.stdout and "--resume" in r.stdout and "--toy" in r.stdout
    assert "{FAC,HASH,TC}" in r.stdout
    try:
        RN.Runner(ns(toy=False, hard_stop_s=1.0))
    except SystemExit as e:
        assert "toy" in str(e)
    else:
        raise AssertionError("--hard-stop-s accepted for a measured run")
    for op, bad in (("sop", "3"), ("add", "13"), ("pos", "16")):
        try:
            RN.Runner(ns(op=op, jobs=bad))
        except SystemExit as e:
            assert "grid" in str(e)
        else:
            raise AssertionError(f"job {bad} accepted in {op}")
    for op in C.OPS:
        r = RN.Runner(ns(op=op))
        assert r.out == C.TOY_BUILD / f"results-{op}" and r.wl == C.TOY_BUILD / f"work-{op}"
        r = RN.Runner(ns(op=op, toy=False))
        assert r.out == C.RESULTS[op] and r.jobs == list(C.JOBS_OF[op]) and r.hard_stop == 40 * 60
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for state, why in (({"op": "add", "hard_stopped": {"step": "x"}}, "hard stop"),
                           ({"op": "add", "pilot_stopped": ["x"]}, "stopped by the pilot"),
                           ({"op": "sop"}, "of the sop run"),
                           ({"op": "add", "records": [{"job": 1}]}, "--resume is refused")):
            r = RN.Runner(ns(resume=True))
            r.out, r.tmp, r.state_file = t / "out", t / "inv", t / "out" / "state.json"
            r.out.mkdir(exist_ok=True)
            r.state_file.write_text(json.dumps(dict(state, elapsed_saved=0)))
            r.log = lambda m: None
            saved = A.report
            A.report = lambda out: None
            try:
                r.start()
            except SystemExit as e:
                assert why in str(e), (why, str(e))
            else:
                raise AssertionError(f"--resume was not refused ({why})")
            finally:
                A.report = saved
                r.disarm()


# ------------------------------------------------------------------------------------------------- the workloads
def test_workloads():
    """rule-m3, Workloads: the grid n in {16, 32, 64} by rho in {1, 4, 16, 64}, jobs 1-12, and sop's 9 (no rho = 16),
    each job its rule-m1 number and seed; add and mul are m1's draw and sop and pos m2's, so step 0's workloads and
    references equal m1's and m2's recorded files by sha256; deterministic"""
    assert C.SIZES == (16, 32, 64) and C.RHOS == (1, 4, 16, 64) and C.TOY_SIZES == (4, 6, 8)
    assert C.JOBS_OF["add"] == C.JOBS_OF["mul"] == C.JOBS_OF["pos"] == tuple(range(1, 13))
    assert C.JOBS_OF["sop"] == (1, 2, 4, 5, 6, 8, 9, 10, 12)
    assert all(C.job_rho(j) != 16 for j in C.JOBS_OF["sop"])
    assert C.STRATA_OF["sop"] == ("NONE", "LOW", "HIGH") and C.STRATA_OF["add"] == ("NONE", "LOW", "MID", "HIGH")
    assert [C.job_n(j) for j in (1, 4, 5, 8, 9, 12)] == [16, 16, 32, 32, 64, 64]
    for op in C.OPS:
        for job in C.JOBS_OF[op]:
            assert C.job_seed(op, job) == 20261029 + {"add": 0, "mul": 100, "sop": 200, "pos": 300}[op] + job
    for op in ("add", "mul"):
        for job in (1, 2):
            assert m2_gen.workload(op, job)[2] == list(m1_gen.workload(op, job)[2:])
    with tempfile.TemporaryDirectory() as t:
        for op in C.OPS:                                     # step 0's files equal m1's (add, mul) or m2's (sop, pos)
            s = m3_gen.generate(Path(t) / op, op, [1, 4], toy=False)
            earlier = json.loads(((C.M1_RESULTS if op in ("add", "mul") else C.M2_RESULTS)[op] /
                                  "state.json").read_text())["gen"]
            for j in ("1", "4"):
                names = [f for f in s[j]["sha256"] if not f.endswith(".m3.json")]
                assert len(names) == (2 if op in ("add", "mul") else 4)
                for f in names:
                    assert s[j]["sha256"][f] == earlier[j]["sha256"][f], (op, j, f)
        s2 = m3_gen.generate(Path(t) / "again", "sop", [4], toy=False)
        s1 = json.loads((Path(t) / "sop" / "gen.json").read_text())
        assert s2["4"]["sha256"] == s1["4"]["sha256"]       # deterministic, byte for byte (predictions included)
        r = run([sys.executable, str(C.HERE / "m3_gen.py"), str(Path(t) / "x"), "--op", "sop", "--jobs", "3"])
        assert r.returncode != 0 and "grid" in r.stderr + r.stdout


# ------------------------------------------------------------------------------------------------- the model
def trial_loop(v):
    """§16.4 item 3's loop, plainly: (factorization, candidates, divisions with remainder, exact divisions)"""
    m, d, out, cand, divs, exact = v, 2, [], 0, 0, 0
    while d * d <= m:
        cand += 1
        e = 0
        while True:
            divs += 1
            if m % d:
                break
            m //= d
            exact += 1
            e += 1
        if e:
            out.append((d, e))
        d = 3 if d == 2 else d + 2
    if m > 1:
        out.append((m, 1))
    return out, cand, divs, exact


def test_model_worked_by_hand():
    """the model of v0.3 by hand: trial division's closed form against the plain loop (and sympy's primes); the
    coprime base by gcds alone (notation note §4's examples); pass 1 and 2 on {12, 18}, {15, 45}, {15, 225},
    {36, 6, 4}; a leftover split and reconciliation splits in small jobs; every value of every set on 1x1 matrices"""
    import random
    rng = random.Random(7)
    vals = [2, 3, 4, 9, 25, 97, 9409, 2 ** 20, 3 ** 13, 2 * 3 * 5 * 7 * 11 * 13, 1000003, 999983 * 2, 999983 ** 2,
            2 ** 31 - 1, 4 * 2147483647] + [rng.randrange(2, 10 ** 9) for _ in range(300)]
    for v in vals:
        f, c, d, e = trial_loop(v)
        assert f == m3_model.factor_primes(v), v
        assert m3_model.trial_counts(f) == (c, d, e), (v, m3_model.trial_counts(f), (c, d, e))
    assert m3_model.cbase([12, 18])[0] == [2, 3] and m3_model.cbase([15, 45])[0] == [3, 5]
    assert m3_model.cbase([15, 225])[0] == [15] and m3_model.cbase([6, 6])[0] == [6]
    S, ng, nd = m3_model.cbase([6, 10])
    assert S == [2, 3, 5] and ng >= 2 and nd == 2
    for nums, base in (([12, 18], {2, 3}), ([15, 45], {3, 5}), ([15, 225], {15}), ([36, 6, 4], {2, 3})):
        x = m3_model.FMat("A", [1] + nums, 64, [], True)
        x.factor()
        assert set(x.base) == base, (nums, x.base)
        for v, ent in zip(x.lst, x.fac):
            p = 1
            for q, k in ent:
                p *= q ** k
            assert p == v or (v < 2 and not ent)
        assert all(gcd(a, b) == 1 for i, a in enumerate(x.base) for b in x.base[i + 1:])
    # the leftover of the unit test: A all 1/6, B all 1: s = 1/6 + p = 1/6, t = 2, gcd(2, 6) = 2: 6 split in A
    p = m3_model.predict("mul", 2, [[(1, 6)] * 4, [(1, 1)] * 4])
    fa = p["factored"]["coprime"]["facc"]["C"]
    assert fa["lsplits"] >= 1 and fa["rc_splits"] == 0 and p["factored"]["coprime"]["nq"]["A"] == 2
    assert p["factored"]["primes"]["facc"]["C"]["lsplits"] == 0
    # reconciliation: A's numerator piece 6 against B's denominator 2, and the stale-D second pass
    p = m3_model.predict("mul", 2, [[(6, 1)] * 4, [(1, 2)] * 4])
    fa = p["factored"]["coprime"]["facc"]["C"]
    assert fa["rc_splits"] >= 1 and fa["rc_passes"] == 2 and fa["rebuilds"] >= 1
    p = m3_model.predict("add", 2, [[(1, 6)] * 4, [(1, 10), (1, 10), (3, 10), (1, 10)]])
    fa = p["factored"]["coprime"]["facc"]["C"]
    assert fa["rc_passes"] == 2 and p["factored"]["coprime"]["nq"] == {"A": 2, "B": 3, "C": 3}
    # 1x1 jobs of every set: the values, and the factored rows' figures
    p = m3_model.predict("add", 1, [[(1, 2)], [(-1, 2)]])
    assert p["values"]["C"] == [(0, 1)] and p["ncase"] == {"add_zero": 1}
    assert p["factored"]["primes"]["ffig_end"]["C"]["E"] == 0                     # 0 and 1: empty lists
    p = m3_model.predict("add", 1, [[(1, 4)], [(1, 12)]])                          # num, 9c: 1/4 + 1/12 = 1/3
    fa = p["factored"]["primes"]["facc"]["C"]
    assert p["values"]["C"] == [(1, 3)] and fa["num_eq"] == 1 and fa["merges"] == 1 and fa["tests"] == 2  # t = 4: 2^2
    assert fa["gchecked"] == 2 and fa["pgcd"] == 0
    fc = p["factored"]["coprime"]["facc"]["C"]
    assert fc["pgcd"] == 1 and fc["tests"] == 0 and p["factored"]["coprime"]["counters"]["C"]["add"]["gcd"] == 1
    assert p["factored"]["primes"]["counters"]["C"]["add"]["gcd"] == 0
    assert p["counters"]["C"]["add"]["gcd"] == 2                                  # TC: gcd(b, d), gcd(t, g)
    p = m3_model.predict("sop", 1, [[(3, 2)], [(4, 3)], [(1, 2)], [(1, 2)]])     # P = 2, Q = 1/4, R = 9/4
    assert p["values"]["R"] == [(9, 4)] and [f["name"] for f in p["factored"]["primes"]["factorings"]] == \
        ["A", "B", "C", "D", "P", "Q", "R"]
    p = m3_model.predict("pos", 1, [[(1, 2)], [(1, 2)], [(2, 3)], [(1, 3)]])     # S = 1, U = 1, R = 1
    assert p["values"]["R"] == [(1, 1)] and p["steps"]["R"]["ncase"] == {"mul_one_x": 1, "add_zero_x": 1}


# ------------------------------------------------------------------------------------------------- the sources
def c_function(src, name, prefix=""):
    """the text of a C function (by its name, with or without a prefix) from its line to its closing brace"""
    m = re.search(r"^[^\n;]*\b(" + re.escape(prefix + name) + "|" + re.escape(name) + r")\(([^)]*)\)[^;\n]*\{\n",
                  src, re.M)
    assert m, name
    i, depth = m.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(src[i], 0)
        i += 1
    return src[m.start():i]


def strip_audit(src):
    """the source without its #ifdef SK_AUDIT ... #endif blocks (the test build's audits and introspection)"""
    out, depth, skip = [], 0, 0
    for line in src.splitlines():
        t = line.strip()
        if t.startswith("#if"):
            depth += 1
            if skip == 0 and t == "#ifdef SK_AUDIT":
                skip = depth
                continue
        if skip:
            if t.startswith("#endif") and depth == skip:
                skip = 0
            if t.startswith("#endif"):
                depth -= 1
            elif t.startswith("#else") and depth == skip:
                skip = 0
            continue
        if t.startswith("#endif"):
            depth -= 1
        out.append(line)
    return "\n".join(out)


def guards(src, needle):
    """the #if lines enclosing the first line that holds needle, innermost last"""
    stack = []
    for line in src.splitlines():
        t = line.strip()
        if needle in line:
            return [s for s, _ in stack]
        if t.startswith("#if"):
            stack.append((t, False))
        elif t.startswith("#else") and stack:
            stack[-1] = (stack[-1][0] + " #else", True)
        elif t.startswith("#endif") and stack:
            stack.pop()
    raise AssertionError(needle)


VALUE_STEP = re.compile(r"\b(sk_add|sk_mul|sk_divexact|XCASE|KC|OVF|return store|rdset|SGN)\b")


def value_steps(body):
    """the value arithmetic of a body, in order: every 08 addition, multiplication and exact division, every case and
    known-answer count, every overflow step, read and store (the factored rows change only how gcds are obtained)"""
    return [re.sub(r"\s+", " ", l.strip()) for l in body.splitlines() if VALUE_STEP.search(l)]


def test_build_and_arithmetic_unchanged():
    """spec v0.3 §§16.4, 16.5, 16.11: tcm.c and mh2.c are m1's and m2's, unchanged; mh3.c's build table, store, widening,
    packing, entry, operands and comparisons are mh2.c's Hash-V-pack path line for line (only its own name in its
    messages); its value arithmetic (every case, 08 addition, multiplication and exact division, known-answer count,
    overflow step and store) is mh2.c's step for step, in add_body and mul_cells; only the gcds are obtained otherwise
    (no gcdp in an operation: Pack-coprime's per-piece gcd alone)"""
    assert L.m1_sources_check() is None and L.tcm_diff_check() is None and L.m2_sources_check() is None
    m2, m3 = (L.SRC / "mh2.c").read_text(), (L.SRC / "mh3.c").read_text()
    line = next(l for l in m2.splitlines() if l.startswith("static inline uint32_t home("))
    assert line in m3.splitlines()
    for name in ("slot_get", "slot_set", "fld_get", "fld_set", "table_new", "probe", "widen", "store_cell",
                 "store", "enter_cell", "operand", "rd1", "rdset", "peek", "begin", "compare_cells", "compare_values",
                 "count_store"):
        a = c_function(m2, name).replace("mh2: ", "mh3: ")
        assert a == c_function(m3, name), name
    a = c_function(m2, "pack").replace("mh2: ", "mh3: ")
    assert a == c_function(m3, "pack")
    sw2 = c_function(m2, "store_whole")
    sw2 = sw2[:sw2.index("#if MH_GROW")] + sw2[sw2.index("#endif", sw2.index("#if MH_GROW")) + len("#endif\n"):]
    assert sw2 == c_function(m3, "store_whole")
    for name in ("add_body", "mul_cells"):
        a, b = value_steps(c_function(m2, name)), value_steps(c_function(m3, name))
        assert a == b, (name, [x for x in zip(a, b) if x[0] != x[1]][:3])
        assert "gcdp(" not in c_function(m3, name)
    assert "gcdp(" in c_function(m3, "pgcd") and "gcdp(" in c_function(m3, "enter_cell")


def test_factored_rows_one_source():
    """spec v0.3 §16.11: the two factored rows are one source, mh3.c, built with MH3_COPRIME 0 (mhpp_) or 1 (mhpc_);
    Hash-V-pack is m2's mh2.c built as m2 built it (MH_GROW=0, MH_PACK=1); the rows and their binaries"""
    assert C.FAC_FLAGS == {"MHPP": 0, "MHPC": 1}
    assert L.OBJECTS["mhpp"] == (L.SRC / "mh3.c", ["-DMH3_COPRIME=0"])
    assert L.OBJECTS["mhpc"] == (L.SRC / "mh3.c", ["-DMH3_COPRIME=1"])
    assert L.OBJECTS["mhvp"] == (L.SRC / "mh2.c", ["-DMH_GROW=0", "-DMH_PACK=1"])
    assert L.IMPL_DEFS["MHVP"] == ["-DIMPL_HASH", "-DMH_GROW=0", "-DMH_PACK=1"]
    for cp, ok in ((0, True), (1, True), (None, False)):
        r = run(["gcc", "-std=c11", "-fsyntax-only"] + ([f"-DMH3_COPRIME={cp}"] if cp is not None else []) + L.inc() +
                [L.SRC / "mh3.c"])
        assert (r.returncode == 0) == ok, (cp, r.stderr[-300:])
    for g, p in ((0, 0), (1, 0)):                             # the driver refuses m2's other hash rows
        r = run(["gcc", "-std=c11", "-fsyntax-only", "-DIMPL_HASH", f"-DMH_GROW={g}", f"-DMH_PACK={p}"] + L.inc() +
                [L.DRIVER[0]])
        assert r.returncode != 0 and "Hash-V-pack only" in r.stderr
    assert C.ROWS == ("TC", "Hash-V-pack", "Pack-primes", "Pack-coprime", "Hash-V-pack-null")
    assert C.ROW_IMPL["Hash-V-pack-null"] == "MHVP" and C.ROWS[-1] == C.NULL_ROW and C.NULL_OF == "Hash-V-pack"
    assert set(L.BUILDS) == {"timing", "counting", "test", "memory"} and L.BUILDS["memory"] == ["-DM2_MEM"]


def test_schoolbook_trial_division():
    """spec v0.3 §16.4 item 3 (rule-m3, reviewer: trial division as written, unoptimized): candidates 2 then the odd
    numbers, stopping at d^2 > m; each "mod" one division with remainder, each "m / d" one exact division; the cofactor
    appended; no sieve, no prime table, no wheel, no other early exit; Pack-coprime's two passes by gcds and divisions"""
    src = strip_audit((L.SRC / "mh3.c").read_text())
    f = c_function(src, "factor")
    td = f[f.index("#if !MH3_COPRIME"):f.index("#else")]
    for t in ("uint64_t m = (uint64_t)v, d = 2;", "while (d * d <= m)", "if (m % d != 0) break;", "m = m / d;",
              "d = d == 2 ? 3 : d + 2;", "if (m > 1) fbuf_put(&b, k, (sk_int)m, 1);"):
        assert t in td, t
    assert td.count("while") == 1 and td.count("break") == 1 and "for (;;)" in td
    for bad in ("sieve", "prime[", "wheel", "sqrt", "isqrt", "% 6", "+= 4", "+= 6", "Miller", "rho"):
        assert bad not in src, bad
    cp = f[f.index("#else"):]
    assert "sk_gcd_raw(rest, q)" in cp and "cbase(S, &n, &ng, &nd)" in cp and "if (m % q) break;" in cp
    assert "factor(" not in c_function(src, "add_body") and "factor(" not in c_function(src, "mul_cells")


def test_no_reserved_slot_no_handle_test():
    """v0.2 §15.2 kept (v0.3 §16.1): no table reserves a slot or holds 0 or 1 in advance; no zero, integer or one test
    compares a handle; every store probes"""
    src = strip_audit((L.SRC / "mh3.c").read_text())
    tn = c_function(src, "table_new")
    assert "slot_set(tab, w, s, EMPTY)" in tn and tn.count("slot_set") == 1
    sw = c_function(src, "store_whole")
    assert "v == 0" not in sw and "v == 1" not in sw and sw.index("probe(") < sw.index("found == 1")
    cc = c_function(src, "compare_cells")
    for t in ("q.zx = o->v[0] == 0", "q.zy = o->v[2] == 0", "q.ix = o->v[1] == 1", "q.iy = o->v[3] == 1",
              "q.ox = six && o->v[0] == o->v[1]", "q.oy = six && o->v[2] == o->v[3]"):
        assert t in cc, t
    assert "x->W = 0;" in src[src.index("X *MH3F(create)"):src.index("void MH3F(destroy)")]


def test_only_named_structures():
    """spec v0.3 §16.3, "Nothing else exists": by the code, outside the test build's audits, the only allocations of
    mh3.c are each matrix's header, the cells (a), the build table (h) through table_new, the widened table (w), the
    list (l), the pack map (p) and the cells' shrinking realloc (Hash-V-pack's); (f)'s three arrays, grown by doubling
    and shrunk (also as (f')); and under MH3_COPRIME only (q), grown and shrunk, and (r)'s lists. The scratch of §16.3
    is local arrays: (t) of 15 entries (+1), the gcd list of 15 (+1), S of 32. By the allocations: the allocation check
    binaries (timing, counting and memory builds)."""
    alloc = re.compile(r"\b(sk_xmalloc|sk_xcalloc|sk_xrealloc|malloc|calloc|realloc|SK_RESERVE)\s*\(")

    def sites(body):
        got = []
        for line in body.splitlines():
            for m in alloc.finditer(line):
                j, depth = m.end(), 1
                while depth:
                    depth += {"(": 1, ")": -1}.get(line[j], 0)
                    j += 1
                got.append(line[m.start():j])
        return sorted(got)
    mh = (L.SRC / "mh3.c").read_text()
    assert sites(strip_audit(mh)) == sorted([
        "sk_xcalloc(1, sizeof *x)",                              # the header
        "sk_xmalloc((size_t)2 * R * K * (x->wh / 8))",           # (a) the cells
        "sk_xmalloc((size_t)T * (w / 8))",                       # (h): table_new
        "sk_xmalloc((size_t)x->T * (w2 / 8))",                   # (w) the widened table
        "sk_xmalloc((size_t)W * (w / 8))",                       # (l) the list
        "sk_xmalloc((size_t)T * (wb / 8))",                      # (p) the pack map
        "sk_xrealloc(x->cells, (size_t)nf * (wp / 8))",          # (a) shrunk in place
        "sk_xmalloc(sizeof(uint32_t) * ((size_t)W + 1))",        # (f) offsets (and (f'))
        "sk_xmalloc((size_t)cap * (w / 8))",                     # (f) factors
        "sk_xmalloc(cap)",                                       # (f) exponents
        "sk_xrealloc(b->fac, (size_t)c2 * (b->w / 8))",          # (f) doubled
        "sk_xrealloc(b->exp, c2)",
        "sk_xrealloc(b->fac, (size_t)b->E * (b->w / 8))",        # (f) shrunk
        "sk_xrealloc(b->exp, b->E)",
        "sk_xrealloc(x->piece, (size_t)c2 * (x->w / 8))",        # (q) doubled
        "sk_xrealloc(x->piece, (size_t)x->nq * (x->w / 8))",     # (q) shrunk
        "sk_xrealloc(D->v, sizeof(sk_int) * c2)"]), sites(strip_audit(mh))   # (r)
    for n in ("x->piece = sk_xrealloc(x->piece, (size_t)c2", "D->v = sk_xrealloc"):
        assert guards(mh, n) == ["#if MH3_COPRIME"], n
    assert "#define FMAX 15u" in mh and "#define SMAX 32u" in mh
    assert "sk_int q[FMAX + 1];" in mh and "gent g[FMAX + 1];" in mh and mh.count(" S[SMAX]") == 3
    assert mh.count("table_new(") == 2                       # its definition and create's (h)
    build_once()
    for b in L.ALLOC_BUILDS:
        p = run([BIN / f"m3_alloc-{b}"])
        assert p.returncode == 0 and " 0 failures" in p.stdout and f"({b} build)" in p.stdout, p.stdout + p.stderr
        for row in ("TC", "Hash-V-pack", "Pack-primes", "Pack-coprime"):
            assert f"  {row}: at most " in p.stdout
        assert " 0 factorings seen" in p.stdout.split("Hash-V-pack:")[1].split("\n")[0]


def test_c_suites():
    """the unit (with the large trial divisions), differential, allocation and sanitizer suites of the Run plan's
    step 2 (the runner's own commands)"""
    build_once()
    for name, cmd, env in L.unit_commands(BIN):
        p = run(cmd, env=dict(os.environ, **env) if env else None)
        assert p.returncode == 0 and " 0 failures" in p.stdout, (name, p.stdout[-500:] + p.stderr[-500:])
        if name == "m3_unit":
            for what in ("the largest prime below 2^63: 1 factors, 1518500250 candidates",
                         "2147483629 * 2147483647: 2 factors, 1073741815 candidates",
                         "refinement: {12, 18}, {15, 45}, {15, 225} (15 kept), {36, 6, 4}",
                         "reconciliation: a numerator piece split against a denominator piece, both directions",
                         "leftovers: a split inside a multiplication's sum", "sub-cases: 9a-c, 10a-c, 12a-c",
                         "the cost order, exhaustively", "widening: 8 -> 16 -> 32 -> 64",
                         "packing: every packed width (8, 16, 32)", "expressions: sop and pos",
                         "intermediates of every width", "overflow: 2^63, -2^63", "factorizations are caught"):
                assert what in p.stdout, what
        if name == "m3_diff":
            assert "obtained gcds checked against Euclid" in p.stdout and "leftover splits" in p.stdout


# ------------------------------------------------------------------------------------------------- the binaries
def gate(impl, op, w, job, t):
    p = run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out", t / f"gp-{impl}.bin",
             t / f"gd-{impl}.bin"])
    assert p.returncode == 0, p.stdout[-500:] + p.stderr
    assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
    for x in C.SET_INTERMEDIATES[op]:
        assert K.readout_check(Path(f"{t / 'g.out'}.{x}"), w / f"job-{job:02d}.{x}.ref") is None, (op, job, x)
    return p.stdout.splitlines(), (C.read_dump(t / f"gd-{impl}.bin") if (t / f"gd-{impl}.bin").exists() else None)


def check_job(op, w, job, t, mem=True):
    """one job, every row: the gate's and the counting run's exact checks; the TC gate; call for call (Hash-V-pack's
    record TC's; the factored rows' cases and core arithmetic TC's); the factored rows' (h), lists and handles
    Hash-V-pack's, and their store splits; the memory runs' stable bytes the model's"""
    pred = json.loads((w / f"job-{job:02d}.m3.json").read_text())["predicted"]
    dumps, stor, core = {}, {}, {}
    for impl in C.IMPLS:
        lines, dump = gate(impl, op, w, job, t)
        fails = K.exact_checks(impl, op, lines, pred, dump, test=True)
        assert not fails, (op, job, impl, fails[:5])
        dumps[impl] = dump
        assert bool(K.parse_packs(lines)) == (impl != "TC") and bool(K.parse_fac(lines)) == (impl in C.FAC_IMPLS)
        p = run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out", t / f"p-{impl}.bin"])
        assert p.returncode == 0 and not K.exact_checks(impl, op, p.stdout.splitlines(), pred, test=False)
        assert K.readout_check(t / "c.out", w / f"job-{job:02d}.ref") is None
        assert (t / f"p-{impl}.bin").read_bytes() == (t / f"gp-{impl}.bin").read_bytes()
        stor[impl] = (K.store_split(p.stdout.splitlines()), K.op_counts_no_gcd(p.stdout.splitlines()))
        core[impl] = K.percall_core_sha(t / f"p-{impl}.bin")
        if not mem:
            continue
        p = run([BIN / f"bench-{impl}-memory", "mem", w / f"job-{job:02d}.wl", t / "m.out"])
        mr = K.parse_memrun(p.stdout.splitlines())
        assert p.returncode == 0 and K.readout_check(t / "m.out", w / f"job-{job:02d}.ref") is None, p.stdout[-300:]
        n = C.job_n(job, True) if pred["nops"] < 4096 else int(round(len((w / f"job-{job:02d}.ref").read_bytes())
                                                                       / 16) ** 0.5)
        assert {f"built_{x}" for x in C.SET_INPUTS[op]} | {"after_creation", "op", "end"} <= set(mr)
        assert mr["op"]["cells"] == C.op_cells(op, n)
        assert mr["op"]["peak_acct"] >= mr["end"]["acct"] >= mr["after_creation"]["acct"] > 0
        if impl != "TC":                                       # stable bytes: the model's, exactly
            sz = pred["sizings"]["pack"]["stor_built"]
            for x in C.SET_INPUTS[op]:
                want = sz[x]["bcells"] + sz[x]["bvals"]
                if impl in C.FAC_IMPLS:
                    ff = pred["factored"][K.ROW_OF[impl]]["ffig_built"][x]
                    want += ff["bytes_f"] + ff["bytes_q"]
                assert mr[f"built_{x}"]["acct"] == want, (op, job, impl, x)
        else:
            assert mr["op"]["peak_acct"] == 16 * C.op_cells(op, n)
    p = run([BIN / "tc3gate_m2", w / f"job-{job:02d}.wl", t / "g3.out", t / "p3.bin"])
    assert p.returncode == 0 and K.readout_check(t / "g3.out", w / f"job-{job:02d}.ref") is None
    assert (t / "p3.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job)            # the TC gate
    assert (t / "p-MHVP.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job)        # call for call
    for i in C.FAC_IMPLS:                                                                     # adapted
        assert core[i] == core["TC"], (op, job, i)
        assert stor[i] == stor["MHVP"], (op, job, i)
        for x in C.SET_MATRICES[op]:
            for sec in ("hb", "tab", "cel"):
                assert dumps[i][f"{sec}{x}"] == dumps["MHVP"][f"{sec}{x}"], (op, job, i, sec, x)
    assert not dumps["TC"]


def test_gates_and_counts_toy():
    """every toy job of every set: each row's gate (test build) and counting run pass their exact checks against the
    model, every list, build table, handle field, (f) and (q); every intermediate against fractions; the TC gate;
    call for call (Hash-V-pack TC's exactly; the factored rows' cases and core arithmetic TC's); the factored rows'
    (h), lists, handles and store splits Hash-V-pack's; the memory runs' stable bytes the model's"""
    build_once()
    for op in C.OPS:
        w = toy_work(op)
        with tempfile.TemporaryDirectory() as t:
            for job in C.JOBS_OF[op]:
                check_job(op, w, job, Path(t))


def test_gates_on_the_splitting_examples():
    """the binaries against the model on the unit tests' splitting jobs (spec v0.3 §16.9 item 6): a leftover split
    inside a multiplication's sum, a numerator piece split against a denominator piece in each direction, and a
    reconciliation with a second pass: every count, (f) and (q) the model's"""
    build_once()
    jobs = {"mul": [[[(1, 6)] * 4, [(1, 1)] * 4], [[(6, 1)] * 4, [(1, 2)] * 4], [[(1, 2)] * 4, [(6, 1)] * 4]],
            "add": [[[(1, 6)] * 4, [(1, 10), (1, 10), (3, 10), (1, 10)]], [[(1, 6)] * 4, [(1, 6)] * 4]]}
    seen = set()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for op, js in jobs.items():
            for k, inputs in enumerate(js, 1):
                w = t / f"{op}{k}"
                w.mkdir()
                C.write_workload(w / "job-01.wl", op, 2, 1, inputs)
                want = m2_gen.reference(op, 2, inputs)
                C.write_pairs(w / "job-01.ref", want[C.SET_RESULT[op]])
                p = m3_model.predict(op, 2, inputs)
                (w / "job-01.m3.json").write_text(json.dumps({"predicted": {k2: v for k2, v in p.items()
                                                                            if k2 != "values"}}))
                fa = p["factored"]["coprime"]["facc"]["C"]
                seen |= {"leftover"} if fa["lsplits"] else set()
                seen |= {"reconcile"} if fa["rc_splits"] else set()
                seen |= {"two passes"} if fa["rc_passes"] >= 2 else set()
                check_job(op, w, 1, t, mem=False)
    assert seen == {"leftover", "reconcile", "two passes"}, seen


def test_exact_checks_catch_a_difference():
    build_once()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for op in ("mul", "sop"):
            w = toy_work(op)
            pred = json.loads((w / "job-06.m3.json").read_text())["predicted"]
            res = C.SET_RESULT[op]
            for impl in ("MHVP", "MHPP", "MHPC"):
                lines, dump = gate(impl, op, w, 6, t)
                assert not K.exact_checks(impl, op, lines, pred, dump, test=True)
                bad = json.loads(json.dumps(pred))
                bad["sizings"]["pack"]["stor_end"][res]["probes"] += 1
                assert any(f"store {res} end probes" in f for f in K.exact_checks(impl, op, lines, bad, test=True))
                bad = json.loads(json.dumps(pred))
                bad["steps"][res]["cross_match"] += 1
                assert any("operation counters" in f for f in K.exact_checks(impl, op, lines, bad, test=True))
                bad = json.loads(json.dumps(pred))
                bad["sizings"]["pack"]["hb"]["A"] = "0" * 64
                assert any("build table (h)" in f for f in K.exact_checks(impl, op, lines, bad, dump, test=True))
                if impl == "MHVP":
                    continue
                row = K.ROW_OF[impl]
                assert K.exact_checks(impl, op, lines, pred, dump, test=False)      # gchecked: the test build's
                for path, what in ((("factorings", 0, "E"), "factorings differ"),
                                   (("ffig_end", "A", "bytes_f"), "(f) and (q) figures of A end"),
                                   (("facc", res, "mentries"), "factored arithmetic differs"),
                                   (("counters", res, "mul" if op == "mul" else "add", "cmp"), f"{res} "),
                                   (("dumps", "B", "ff"), "B's factorization list (f)"),
                                   (("dumps", "C", "fq"), "C's coprime base (q)")):
                    bad = json.loads(json.dumps(pred))
                    x = bad["factored"][row]
                    for k in path[:-1]:
                        x = x[k]
                    x[path[-1]] = "0" * 64 if path[0] == "dumps" else x[path[-1]] + 1
                    assert any(what in f for f in K.exact_checks(impl, op, lines, bad, dump, test=True)), (impl, path)
                if impl == "MHPP":                                  # Pack-primes' lines are not Pack-coprime's
                    assert K.exact_checks("MHPC", op, lines, pred, dump, test=True)
            lines, _ = gate("TC", op, w, 6, t)                    # TC prints no store figures
            assert not K.exact_checks("TC", op, lines, pred) and K.exact_checks("TC", op, lines + ["stor end A W=2"], pred)
        ref = (toy_work("sop") / "job-06.ref").read_bytes()       # a read-out that differs in one cell is caught
        broken = bytearray(ref)
        broken[8 * 5] ^= 1
        (t / "x.out").write_bytes(bytes(broken))
        r = K.readout_check(t / "x.out", ref)
        assert r and r["cell"] == 5 and r["mismatches"] == 1


def test_untimed_steps_toy():
    """the runner's untimed steps end to end on toy jobs of every set, with the real binaries: step 0, step 1 (build,
    pins, tcm.c's diff, m1's and m2's sources, step 0's files), step 2 (unit, differential, allocation and sanitizer
    tests), step 4 (gates, the TC gate) and step 5 (counting and memory runs, the repeats on jobs 1 and 6, and the
    adapted call-for-call checks). The build is made once (sop), then reused. Nothing timed runs."""
    build_once()
    for op in ("sop", "add", "mul", "pos"):
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            r = RN.Runner(ns(op=op, jobs="1,6,10"))
            r.out, r.tmp = t / "out", t / "inv"
            r.out.mkdir()
            r.tmp.mkdir()
            r.state_file = r.out / "state.json"
            r.wl, r.bin = t / "work", (t / "bin" if op == "sop" else BIN)
            if op != "sop":
                r.build = lambda: None
            r.log = lambda m: None
            r.state = {"elapsed_before": 0.0, "op": op}
            r.step0()
            assert set(r.state["gen"]) == {"1", "6", "10"} and r.state["gen"]["6"]["name"] == f"{op}-6"
            r.t0 = time.monotonic()
            r.step1()
            assert (r.out / "tcm.diff").read_text() == L.tcm_diff_text()
            if op == "sop":
                r.unit_tests()
                ut = r.state["unit_tests"]
                assert set(ut) == {"m3_unit", "m3_diff", "m3_alloc-timing", "m3_alloc-counting", "m3_alloc-memory",
                                   "m3_unit-asan", "m3_diff-asan"} and all(v["rc"] == 0 for v in ut.values()), ut
            r.step4_gates()
            assert len(r.state["gates"]) == 12 and r.state["measuring"]
            assert all(r.state["gates"][f"TC:{j}"]["tc3gate"]["percall"] == "equal" for j in (1, 6, 10))
            r.step5_counts()
            assert threading.active_count() == 1
            for job in ("1", "6", "10"):
                chk = r.state["call_checks"][job]
                assert chk["result"] == "pass", chk
                assert chk["MHVP"]["G_ratio"] == 1.0 and chk["MHVP"]["gcd_work_check"] == "passed"
                for i in C.FAC_IMPLS:
                    assert chk[i]["call_for_call"].startswith("pass") and "reported" in chk[i]["gcd_work_check"]
                    assert chk[i]["G_ratio_by_part"]["entry"] == 1.0
                assert chk["MHPP"]["G_ratio_by_part"]["add" if op in ("add", "sop") else "mul"] in (0.0, None)
            for impl in C.IMPLS:
                for job in (1, 6):
                    assert r.state["counts"][f"{impl}:{job}"]["repeat"] == "identical"
                    assert r.state["mems"][f"{impl}:{job}"]["repeat"] == "identical"
                assert "repeat" not in r.state["counts"][f"{impl}:10"]
            assert not r.state.get("bugs")
            (r.wl / "job-06.ref").write_bytes(b"x")              # a changed step-0 file is refused at step 1
            r.build = lambda: None
            try:
                r.step1()
            except RN.RunStop as e:
                assert "step 0's files changed" in str(e)
            else:
                raise AssertionError("a changed workload file was not refused")


def test_step0_against_earlier():
    """rule-m3, Workloads: a measured add or mul run refuses step-0 files that differ from m1's recorded ones, and a sop
    or pos run those that differ from m2's; the predictions are this round's own; toy runs are not compared"""
    for op, folder in (("add", C.M1_RESULTS["add"]), ("sop", C.M2_RESULTS["sop"])):
        r = RN.Runner(ns(op=op, toy=False))
        gen = json.loads((folder / "state.json").read_text())["gen"]
        r.state = {"gen": {j: {"sha256": {k: v for k, v in m["sha256"].items() if not k.endswith((".m1.json",
                                                                                                  ".m2.json"))}}
                           for j, m in gen.items() if int(j) in C.JOBS_OF[op]}}
        r.state["gen"]["1"]["sha256"]["job-01.m3.json"] = "f" * 64          # not compared
        assert r.step0_vs_earlier() is None
        r.state["gen"]["4"]["sha256"]["job-04.ref"] = "0" * 64
        assert r.step0_vs_earlier() == ["job-04.ref"]
    r = RN.Runner(ns(op="mul", toy=True))
    r.state = {"gen": {"1": {"sha256": {"job-01.wl": "x", "job-01.ref": "y"}}}}
    assert r.step0_vs_earlier() is None                         # toy runs: not compared


# ------------------------------------------------------------------------------------------------- statistics, pilot
def test_statistics_and_verdicts():
    a = [10.0] * 12
    r, lo, hi = A.boot_ratio(a, a)
    assert r == lo == hi == 1.0
    assert A.time_verdict(1.2, 1.3, 0.05) == "win" and A.time_verdict(0.7, 0.8, 0.05) == "loss"
    assert A.time_verdict(0.97, 1.2, 0.05) == "tie"
    assert A.fixed_verdict(0.90) == "win" and A.fixed_verdict(1.10) == "loss" and A.fixed_verdict(1.0) == "tie"
    assert C.SEVERE == 2.0 and C.BAND_MIN == 0.05 and C.BOOT == 10_000 and C.BOOT_SEED == 20261029
    ref = [[20.0, 30.0]] * 12
    row = [[10.0, 15.0]] * 12
    assert A.boot_pooled(ref, row) == (2.0, 2.0, 2.0)
    assert A.judged("against TC", "build") and A.judged("factored against pack", "operation")
    assert not A.judged("coprime against primes", "build") and not A.judged("against TC", "total")
    assert len(C.PAIRINGS) == 6 and sum(1 for p in C.PAIRINGS if p[2] == "against TC") == 3
    assert C.PAIRINGS[-1] == ("Pack-coprime", "Pack-primes", C.REPORTED_KIND)
    assert ("Pack-primes", "Hash-V-pack", "factored against pack") in C.PAIRINGS
    assert ("Pack-coprime", "Hash-V-pack", "factored against pack") in C.PAIRINGS
    assert C.PEAK_MEASURE == {"add": 7, "mul": 8, "sop": None, "pos": None}
    reps = [(0, 100, 50, 10, (20, 20)), (1, 90, 60, 5, (15, 10)), (2, 95, 40, 9, (10, 30))]
    assert K.kept_minimum(reps, 2) == (45.0, 20.0, 2.5, (5.0, 5.0))
    # the adapted call-for-call record: gcds cleared, case and core kept
    import struct
    with tempfile.TemporaryDirectory() as t:
        a, b = Path(t) / "a", Path(t) / "b"
        a.write_bytes(struct.pack("<12h", 3, 2, 30, 1, 2, 3, 4, 0, 0, 0, 1, 1))
        b.write_bytes(struct.pack("<12h", 3, 0, 0, 1, 2, 3, 4, 1, 9, 0, 1, 1))
        assert K.percall_core_sha(a) == K.percall_core_sha(b)
        b.write_bytes(struct.pack("<12h", 3, 0, 0, 1, 2, 3, 4, 1, 9, 1, 1, 1))
        assert K.percall_core_sha(a) != K.percall_core_sha(b)


def test_pilot_projection_and_allowance():
    """rule-m3, Run plan step 3: elapsed + 1.2 [sum_c N_c (O_c + sum_rows (K + 6 P)) + 1/2 sum_c N_c sum_rows 4 t]"""
    blocks = {0: {"O": 3.0, "K": {"TC": 0.2, "X": 0.4}, "P": {"TC": 0.5, "X": 1.0, "N": 0.6}, "t": {"TC": 0.01, "X": 0.5}},
              2: {"O": 4.0, "K": {"TC": 0.3, "X": 11.0}, "P": {"TC": 0.7}, "t": {"TC": 0.02, "X": 11.0}}}
    first = 4 * (3.0 + 0.2 + 0.4 + 6 * (0.5 + 1.0 + 0.6)) + 3 * (4.0 + 0.3 + 11.0 + 6 * 0.7)
    second = 4 * 4 * (0.01 + 0.5) + 3 * 4 * (0.02 + 11.0)
    assert abs(K.pilot_projection(90, blocks, {0: 4, 2: 3}) - (90 + 1.2 * (first + second / 2))) < 1e-9
    assert C.PILOT_MARGIN == 1.2 and C.PILOT_JOBS == (1, 6, 10) and C.PILOT_ROUND_SCALE == 6
    assert C.PILOT_SIDE_RUNS == 4 and C.PILOT_SIDE_PARALLEL == 2
    assert all(j in C.JOBS_OF[op] for op in C.OPS for j in C.PILOT_JOBS)
    assert C.PILOT_LIMIT_S == 30 * 60 and C.HARD_STOP_S == 40 * 60 and C.OPERATOR_TIMEOUT_MIN == 45
    assert C.ROUNDS == 12 and C.PILOT_ROUNDS == 2 and C.COUNT_REPEAT_JOBS == (1, 6) and C.COUNT_LIMIT_S == 600
    assert C.CAP_NS == 10 * 10 ** 9
    assert K.host_allowance(0.5, 10) == 5 + 10 and K.host_allowance(0.0, 10) == 5 and C.HOST_RATE_CEILING == 4.0
    assert len(C.IMPLS) + C.PILOT_ROUNDS * len(C.ROWS) == 14 and len(C.IMPLS) + C.ROUNDS * len(C.ROWS) == 64


def pilot_runner(rates, P_extra=0.0, block_s=0.0, jobs="", op="add", cal_s=0.1, round_s=0.1, rep_ns=60_000_000):
    """a runner whose pilot blocks are stand-ins; rates: one host rate for every block, or a list of
    {host pid: (ticks, seconds)} per block, in block order; block_s: each stand-in block's wall time; every launch
    recorded with its wall time (cal_s, round_s) and each calibration with r = 4 and its rep time"""
    r = RN.Runner(ns(jobs=jobs, op=op))
    r.out = Path(tempfile.mkdtemp())
    r.state_file = r.out / "state.json"
    r.state = {"elapsed_before": 0.0, "disturbance_checks": []}
    r.t0 = time.monotonic() - P_extra
    r.save = lambda: None
    r.log = lambda m: None
    blocks = iter(rates) if isinstance(rates, list) else None
    seen = []

    def block(job, label, pilot=False, rows=None):
        assert pilot and label == "pilot" and rows is None
        seen.append(job)
        if blocks is None:
            hosts, sec = {"1": {"name": "init", "ticks": 0, "rate": rates}}, 1.6
        else:
            b = next(blocks)
            sec = max(s for _, s in b.values())
            hosts = {p: {"name": f"host{p}", "ticks": t, "rate": t / s} for p, (t, s) in b.items()}
        r.state["disturbance_checks"].append({"label": "pilot", "hosts": hosts, "seconds": sec})
        time.sleep(block_s)
        return [{"role": "calibration", "row": C.IMPL_ROW[i], "job": job, "r": 4, "reps": 3, "rep_ns": rep_ns,
                 "block": "pilot", "wall": cal_s} for i in C.IMPLS] + \
            [{"role": "round", "row": row, "job": job, "total": 100.0, "block": "pilot", "wall": round_s}
             for row in C.ROWS for _ in range(2)]
    r.timed_block = block
    r.seen = seen
    return r


def test_pilot_stops():
    r = pilot_runner(0.1)
    r.step3_pilot()                                       # goes on
    p = r.state["pilot"]
    assert r.seen == [1, 6, 10] and p["N"] == {"0": 4, "1": 4, "2": 4}
    b = p["blocks"]["0"]
    assert b["K"] == {C.IMPL_ROW[i]: 0.1 for i in C.IMPLS} and abs(b["P"]["Hash-V-pack-null"] - 0.2) < 1e-12
    assert b["t"]["TC"] == 60_000_000 / 4 / 1e9 and "Hash-V-pack-null" not in b["t"]
    blocks = {int(c): v for c, v in p["blocks"].items()}
    assert abs(p["projected_s"] - K.pilot_projection(p["elapsed_s"], blocks, {0: 4, 1: 4, 2: 4})) < 1e-6
    assert r.state["pilot_calibration"]["1:Hash-V-pack-null"]["shared_from"] == "Hash-V-pack"
    r = pilot_runner(0.1, op="sop")
    r.step3_pilot()
    assert r.state["pilot"]["N"] == {"0": 3, "1": 3, "2": 3}
    for rate in (1.25, 4.0):
        pilot_runner(rate).step3_pilot()
    # over the limit: a slow factored row (11 s per run) projects 1.2 x 1/2 x 4 x 4 x 11 x 2 rows past 30 minutes
    for rate, extra, bs, rep, why in ((4.5, 0, 0, 60_000_000, "ceiling"), (0.1, 29.8 * 60, 0.2, 60_000_000, "projected"),
                                      (0.1, 31 * 60, 0.0, 60_000_000, "projected"),
                                      (0.1, 0, 0.0, 4 * 400 * 10 ** 9, "projected")):
        r = pilot_runner(rate, extra, bs, rep_ns=rep)
        try:
            r.step3_pilot()
        except RN.RunStop as e:
            assert why in str(e), str(e)
            assert r.state["pilot_stopped"]
        else:
            raise AssertionError(f"the pilot did not stop the run ({why})")
    r = pilot_runner(0.1, jobs="2,3")                     # a toy run without jobs 1, 6, 10: its first job alone
    r.step3_pilot()
    assert r.seen == [2] and r.state["pilot"]["N"] == {"0": 2}


def test_host_allowance_from_pilot():
    """R_h is each host's highest rate over the pilot's three blocks, but at least 1 tick over the longest pilot block,
    and over a block of L seconds the allowance is 5 + ceil(2 R_h L)"""
    per_block = [{"91": (2, 1.6), "1": (1, 1.6), "7": (0, 1.6)}, {"91": (1, 1.6), "1": (0, 1.6), "7": (0, 1.6)},
                 {"91": (1, 2.5), "1": (0, 2.5), "7": (0, 2.5)}]
    r = pilot_runner(per_block)
    r.step3_pilot()
    assert r.state["host_rates"] == {"91": 1.25, "1": 0.625, "7": 0.4}, r.state["host_rates"]
    assert r.state["host_rates_measured"]["7"] == 0.0
    assert r.state["pilot"]["host_rates"] == r.state["host_rates"]
    assert r.allowance_for(10.0) == {91: 5 + 25, 1: 5 + 13, 7: 5 + 8}
    s0 = snap({91: ("claude", 1, 100, 50, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 0.0)
    ok = snap({91: ("claude", 1, 120, 60, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 10.0)   # 30 ticks
    over = snap({91: ("claude", 1, 121, 60, 0, 0, 1), 1: ("init", 0, 10, 10, 0, 0, 1)}, 0, 10.0)  # 31 ticks
    d = RN.disturbance(s0, ok, 4242, 0, {91, 1}, r.allowance_for(10.0))
    assert not d["disturbed"] and d["hosts"]["91"]["allowance"] == 30, d
    assert RN.disturbance(s0, over, 4242, 0, {91, 1}, r.allowance_for(10.0))["disturbed"]


# ------------------------------------------------------------------------------------------------- spawning, timer
def test_spawner_affinity_before_exec():
    sp = RN.Spawner()
    with tempfile.TemporaryDirectory() as t:
        out = Path(t) / "o.txt"
        rc = sp.run(["/bin/grep", "Cpus_allowed_list", "/proc/self/status"], out, C.TIMED_CORE)
        assert rc == 0 and out.read_text().split()[-1] == str(C.TIMED_CORE), out.read_text()
        rc = sp.run(["/bin/sleep", "0.2"], out, C.TIMED_CORE)
        assert rc == 0 and 0.2 <= sp.last_launch_s < 2.0, sp.last_launch_s
        rc = sp.run(["/nonexistent/binary"], out, C.TIMED_CORE)
        assert rc == 127
        assert sp.spawns == 3 and sp.child is None


def test_hard_stop_kernel_timer():
    r = RN.Runner(ns(hard_stop_s=0.3))
    r.state = {"elapsed_before": 0.0}
    r.t0 = time.monotonic()
    r.arm()
    t = time.monotonic()
    with tempfile.TemporaryDirectory() as d:
        try:
            r.spawner.run(["/bin/sleep", "5"], Path(d) / "o", C.TIMED_CORE)
        except RN.HardStop:
            pid = r.spawner.child
            r.reap()
            assert time.monotonic() - t < 2.0
            assert pid is not None and r.spawner.child is None
            try:
                os.kill(pid, 0)
                raise AssertionError("the child survived the hard stop")
            except ProcessLookupError:
                pass
        else:
            raise AssertionError("the kernel timer did not stop the run")
        finally:
            r.disarm()


# ------------------------------------------------------------------------------------------------- idle, disturbance
# round 3's, m1's and m2's tests, unchanged: rule-m3 item 5 carries the disturbance check over in full, and
# m3_runner uses m1_runner's code
def test_idle_probe():
    ok, d = RN.idle_probe(0.3)
    assert set(d) == {"busy", "core1", "load1"}
    p = subprocess.Popen(["taskset", "-c", str(C.TIMED_CORE), sys.executable, "-c",
                          "import time\nt=time.time()\nwhile time.time()-t<2: pass"])
    try:
        time.sleep(0.2)
        ok, d = RN.idle_probe(0.6)
        assert not ok and d["core1"] > C.IDLE_CORE1_MAX and any(b["pid"] == p.pid for b in d["busy"]), d
    finally:
        p.kill()
        p.wait()


def snap(procs, forks, t):
    return {"t": t, "forks": forks, "procs": procs}


def test_disturbance_synthetic():
    me, host, other, kt = 100, 50, 200, 300
    base = {me: ("python3", host, 10, 5, 99, 9, 1), host: ("claude", 1, 1, 1, 0, 0, 1),
            other: ("sshd", 1, 3, 3, 0, 0, 1), kt: ("kworker", 2, 0, 0, 0, 0, 1)}
    s0 = snap(base, 1000, 0.0)
    quiet = dict(base)
    quiet[me] = ("python3", host, 11, 6, 400, 30, 1)
    d = RN.disturbance(s0, snap(quiet, 1012, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["runner_ticks"] == 2 and d["forks_unattributed"] == 0
    busy = {**quiet, other: ("sshd", 1, 11, 11, 0, 0, 1)}
    d = RN.disturbance(s0, snap(busy, 1012, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and d["other_limit"] == 15
    edge = {**quiet, other: ("sshd", 1, 11, 10, 0, 0, 1)}
    assert not RN.disturbance(s0, snap(edge, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    short = {**quiet, other: ("sshd", 1, 5, 5, 0, 0, 1)}
    assert not RN.disturbance(s0, snap(short, 1012, 1.0), me, 12, {host}, {host: 7})["disturbed"]
    short = {**quiet, other: ("sshd", 1, 6, 7, 0, 0, 1)}
    assert RN.disturbance(s0, snap(short, 1012, 1.0), me, 12, {host}, {host: 7})["disturbed"]
    reaped = {**quiet, other: ("sshd", 1, 3, 3, 12, 4, 1)}
    assert RN.disturbance(s0, snap(reaped, 1013, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    new = {**quiet, 999: ("cron", 1, 1, 0, 0, 0, 5)}
    d = RN.disturbance(s0, snap(new, 1013, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and d["new"][0]["pid"] == 999
    d = RN.disturbance(s0, snap(quiet, 1013, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["forks_unattributed"] == 1
    hthreads = {**quiet, host: ("claude", 1, 3, 2, 0, 0, 1, 40)}
    d = RN.disturbance(snap({**base, host: base[host] + (12,)}, 1000, 0.0), snap(hthreads, 1512, 10.0), me, 12,
                       {host}, {host: 7})
    assert not d["disturbed"] and d["hosts"][str(host)]["threads"] == [12, 40] and d["forks_unattributed"] == 500
    hkid = {**quiet, host: ("claude", 1, 1, 1, 6, 2, 1)}
    d = RN.disturbance(s0, snap(hkid, 1013, 10.0), me, 12, {host}, {host: 7})
    assert d["disturbed"] and "over its allowance" in d["reasons"][0], d
    zero = {**quiet, 998: ("sleep", host, 0, 0, 0, 0, 5)}
    d = RN.disturbance(s0, snap(zero, 1013, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["new_quiet"][0]["pid"] == 998
    zkid = {**quiet, 997: ("sh", host, 0, 0, 1, 0, 5)}
    assert RN.disturbance(s0, snap(zkid, 1013, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    kth = {**quiet, 301: ("kworker/1", 2, 0, 1, 0, 0, 7)}
    d = RN.disturbance(s0, snap(kth, 1013, 10.0), me, 12, {host}, {host: 7})
    assert not d["disturbed"] and d["new_kthreads"] == 1
    hb = {**quiet, host: ("claude", 1, 6, 4, 0, 0, 1)}
    assert RN.disturbance(s0, snap(hb, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]
    d = RN.disturbance(s0, snap(hb, 1012, 10.0), me, 12, {host}, None)
    assert not d["disturbed"] and d["hosts"][str(host)]["rate"] == 0.8
    reused = {**quiet, other: ("sshd", 1, 0, 1, 0, 0, 9)}
    assert RN.disturbance(s0, snap(reused, 1012, 10.0), me, 12, {host}, {host: 7})["disturbed"]


BURN = "import time\nt=time.process_time()\nwhile time.process_time()-t<0.3: pass"


def test_existing_child_exit_charged_for_the_block_only():
    me, host, other, kid = 100, 50, 200, 210
    base = {me: ("python3", host, 10, 5, 0, 0, 1), host: ("claude", 1, 1, 1, 0, 0, 1),
            other: ("sshd", 1, 3, 3, 0, 0, 1), kid: ("worker", other, 40, 10, 0, 0, 2)}
    s0 = snap(base, 1000, 0.0)
    gone = {k: v for k, v in base.items() if k != kid}
    late = {**gone, other: ("sshd", 1, 3, 3, 42, 10, 1)}
    d = RN.disturbance(s0, snap(late, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["disturbed"] and d["credited"][str(other)]["ticks"] == 50, d
    busy = {**gone, other: ("sshd", 1, 3, 3, 60, 10, 1)}
    d = RN.disturbance(s0, snap(busy, 1000, 10.0), me, 0, {host}, {host: 7})
    assert d["disturbed"] and d["others"][0]["ticks"] == 20, d
    ign = {**gone, other: ("sshd", 1, 10, 3, 0, 0, 1)}
    d = RN.disturbance(s0, snap(ign, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["credited"] and not d["disturbed"]
    ign = {**gone, other: ("sshd", 1, 17, 5, 0, 0, 1)}
    assert RN.disturbance(s0, snap(ign, 1000, 10.0), me, 0, {host}, {host: 7})["disturbed"]
    hb = {**base, kid: ("worker", host, 40, 10, 0, 0, 2)}
    hl = {**gone, host: ("claude", 1, 2, 1, 42, 10, 1)}
    d = RN.disturbance(snap(hb, 1000, 0.0), snap(hl, 1000, 10.0), me, 0, {host}, {host: 7})
    assert not d["disturbed"] and d["hosts"][str(host)]["ticks"] == 3, d
    orphan = {k: v for k, v in gone.items() if k != other}
    orphan[1] = ("init", 0, 0, 0, 56, 0, 1)
    d = RN.disturbance(snap({**base, 1: ("init", 0, 0, 0, 0, 0, 1)}, 1000, 0.0), snap(orphan, 1000, 10.0), me, 0,
                       {host, 1}, {host: 7, 1: 7})
    assert d["disturbed"] and d["hosts"]["1"]["ticks"] == 50 and d["credited"] == {"1": {"name": "init", "ticks": 6}}, d
    parent = subprocess.Popen([sys.executable, "-c",
                               "import subprocess, sys, time\n"
                               f"subprocess.run([sys.executable, '-c', {BURN!r} + "
                               "'\\nprint(\"ready\", flush=True)\\ntime.sleep(1.0)'])\n"
                               "time.sleep(30)\n"], stdout=subprocess.PIPE, text=True)
    try:
        assert parent.stdout.readline().strip() == "ready"
        hosts = {p for p, _ in RN.ancestors()}
        s0 = RN.snapshot()
        kids = [p for p, st in s0["procs"].items() if st[1] == parent.pid]
        assert len(kids) == 1 and RN.ticks4(s0["procs"][kids[0]]) >= 20, s0["procs"].get(kids[0] if kids else 0)
        time.sleep(1.6)
        s1 = RN.snapshot()
        assert kids[0] not in s1["procs"]
        raw = RN.ticks4(s1["procs"][parent.pid]) - RN.ticks4(s0["procs"][parent.pid])
        assert raw > K.other_limit(s1["t"] - s0["t"]), raw
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert not [o for o in d["others"] if o["pid"] == parent.pid], d
        assert d["credited"][str(parent.pid)]["ticks"] == RN.ticks4(s0["procs"][kids[0]]), d
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts | {parent.pid},
                           {**{p: 10 ** 6 for p in hosts}, parent.pid: C.HOST_BASE_TICKS})
        assert d["hosts"][str(parent.pid)]["ticks"] <= C.HOST_BASE_TICKS and not d["disturbed"], d
    finally:
        parent.kill()
        parent.wait()


HELPER_THREADS = r"""
import threading, time, sys
time.sleep(0.5)
def burn():
    t = time.process_time()
    while time.process_time() - t < 0.004: pass
for k in range(60):
    th = threading.Thread(target=burn); th.start(); th.join()
time.sleep(30)
"""


def test_host_threads_do_not_disturb():
    host = subprocess.Popen([sys.executable, "-c", HELPER_THREADS])
    try:
        time.sleep(0.2)
        hosts = {p for p, _ in RN.ancestors()} | {host.pid}
        s0 = RN.snapshot()
        time.sleep(1.6)
        s1 = RN.snapshot()
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert not d["disturbed"], d
        assert d["forks_unattributed"] >= 60, d
        assert not [n for n in d["new"] + d["new_quiet"] if n["ppid"] == host.pid], d
        assert str(host.pid) in d["hosts"] and d["hosts"][str(host.pid)]["ticks"] >= 1, d
        tight = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {**{p: 10 ** 6 for p in hosts}, host.pid: 0})
        assert tight["disturbed"] and any(str(host.pid) in x for x in tight["reasons"]), tight
    finally:
        host.kill()
        host.wait()


def test_disturbance_catches_short_lived_process():
    helper = subprocess.Popen([sys.executable, "-c",
                               "import time, subprocess, sys\n"
                               "time.sleep(0.5)\n"
                               f"subprocess.run([sys.executable, '-c', {BURN!r}])\n"
                               "subprocess.run(['/bin/true'])\n"
                               "time.sleep(30)\n"])
    late = subprocess.Popen([sys.executable, "-c",
                             "import time, subprocess, sys\n"
                             "time.sleep(0.5)\n"
                             f"p = subprocess.Popen([sys.executable, '-c', {BURN!r} + '\\ntime.sleep(30)'])\n"
                             "print(p.pid, flush=True)\n"
                             "time.sleep(30)\n"], stdout=subprocess.PIPE, text=True)
    kid = None
    try:
        time.sleep(0.2)
        hosts = {p for p, _ in RN.ancestors()}
        s0 = RN.snapshot()
        time.sleep(1.6)
        s1 = RN.snapshot()
        kid = int(late.stdout.readline())
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts, {p: 10 ** 6 for p in hosts})
        assert d["disturbed"], d
        caught = [o for o in d["others"] if o["pid"] == helper.pid]
        assert caught and caught[0]["ticks"] > C.DIST_OTHER_TICKS, d
        assert any(n["pid"] == kid and n["ppid"] == late.pid for n in d["new"]), d
        d = RN.disturbance(s0, s1, os.getpid(), 0, hosts | {helper.pid}, {**{p: 10 ** 6 for p in hosts},
                                                                          helper.pid: C.HOST_BASE_TICKS})
        assert d["hosts"][str(helper.pid)]["ticks"] > C.HOST_BASE_TICKS, d
        assert any(f"[{helper.pid}]" in x and "allowance" in x for x in d["reasons"]), d
    finally:
        for p in (helper, late):
            p.kill()
            p.wait()
        if kid:
            try:
                os.kill(kid, signal.SIGKILL)
            except ProcessLookupError:
                pass


# ------------------------------------------------------------------------------------------------- the timed block
FAKE = r'''#!/usr/bin/env python3
# a stand-in for bench_m3 (harness test only): copies the reference as its read-out, prints made-up lines
import sys, os, shutil
mode, wl, ro = sys.argv[1], sys.argv[2], sys.argv[3]
row = os.path.basename(sys.argv[0]).split("-")[1]
cfg = os.environ.get("FAKE_CFG", "")
shutil.copy(wl[:-3] + ".ref", ro)
if "bug" in cfg and row == "TC":
    with open(ro, "r+b") as f:
        f.seek(0); b = f.read(1); f.seek(0); f.write(bytes([b[0] ^ 1]))
if mode == "calib":
    for r in (1, 2, 4):
        print(f"calib {r} {r * 20000000} 1 2 builds=3,4,5,6 row={row} cpu={r * 19} span={r * 21}")
    if "cap" in cfg and row == "TC":
        print("overcap 20000000000")
elif mode == "time":
    r, reps = int(sys.argv[4]), int(sys.argv[5])
    for i in range(reps):
        print(f"rep {i} {r * (1000 + i)} {r * (800 - i)} {r * (100 + i)} builds={r * 25},{r * (24 + i)},{r * 26},"
              f"{r * (25 - i)} row={row} cpu={r * (990 + i)} span={r * (1100 + i)}")
print(f"ok impl={row} n=6 set=sop ops=16")
'''


def fake_runner(tmp, cfg=""):
    w = toy_work("sop")
    r = RN.Runner(ns(op="sop", jobs="6"))
    r.out = tmp / "out"
    r.out.mkdir(parents=True)
    r.state_file = r.out / "state.json"
    r.tmp = tmp / "inv"
    r.wl = w
    r.state = {"elapsed_before": 0.0, "measuring": True}
    r.t0 = time.monotonic()
    r.hosts = {}
    r.idle_probe = lambda: (True, {"busy": [], "core1": 0.0, "load1": 0.0})
    fb = tmp / "fakebin"
    fb.mkdir()
    for impl in C.IMPLS:
        p = fb / f"bench-{impl}-timing"
        p.write_text(FAKE)
        p.chmod(0o755)
    r.bin = fb
    os.environ["FAKE_CFG"] = cfg
    return r


def test_timed_block_machinery():
    real = RN.disturbance
    seen = []

    def quiet(s0, s1, runner, spawns, hosts, allowance=None):
        d = real(s0, s1, runner, spawns, hosts, allowance)
        seen.append(d)
        d.update(disturbed=False, reasons=[], runner_ticks=0)
        return d
    RN.disturbance = quiet
    try:
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(6, "measured")
            cal = [x for x in recs if x["role"] == "calibration"]
            rnd = [x for x in recs if x["role"] == "round"]
            assert [x["row"] for x in cal] == list(C.ROWS[:4]) and cal[0]["r"] == 4 and cal[0]["reps"] == 3
            assert len(rnd) == 5 * C.ROUNDS and seen[-1]["spawns"] == 4 + 5 * C.ROUNDS == 64   # rule-m3 Run plan 6
            assert [x["row"] for x in rnd[:10]] == list(C.ROWS) + list(C.ROWS[1:]) + [C.ROWS[0]]  # rotated by one
            assert not any(x["row"] == C.NULL_ROW for x in cal)          # the null row takes Hash-V-pack's r
            x = rnd[0]
            # each quantity its own minimum of reps 2 and 3, over r = 4: total 1001, op 798, read 101, builds 25,25,26,23
            assert x["r"] == 4 and x["total"] == 1001 and x["operation"] == 798 and x["read"] == 101 and "bug" not in x
            assert x["builds"] == [25, 25, 26, 23]
            assert all(y["launch_s"] is not None and y["launch_s"] > 0 for y in recs)
            assert x["cpu_ns"] == 991 and x["span_ns"] == 1101 and x["cpu_share"] == 991 / 1101
            assert not list(r.tmp.glob("i*")), "the invocation files are removed after the block"
            assert threading.active_count() == 1
            prs = r.timed_block(6, "pilot", pilot=True)
            assert len([x for x in prs if x["role"] == "round"]) == 5 * C.PILOT_ROUNDS and seen[-1]["spawns"] == 14
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t), "cap")
            recs = r.timed_block(6, "measured")
            assert not any(x["row"] == "TC" for x in recs if x["role"] == "round")
            assert any(x.get("overcap") for x in recs if x["row"] == "TC")
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t), "bug")
            recs = r.timed_block(6, "measured")
            assert all("bug" in x for x in recs if x["row"] == "TC")
            assert not any("bug" in x for x in recs if x["row"] != "TC")
            for rec in recs:
                if "bug" in rec:
                    r.bug(C.ROW_IMPL[rec["row"]], 6, "round", rec["bug"])
            assert r.live_rows() == list(C.ROWS[1:])                     # TC's measured run stops (item 12)
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_block_rerun_once():
    real = RN.disturbance
    calls = []

    def flaky(s0, s1, runner, spawns, hosts, allowance=None):
        d = real(s0, s1, runner, spawns, hosts, allowance)
        calls.append(1)
        bad = len(calls) == 1
        d.update(disturbed=bad, reasons=["made up"] if bad else [], runner_ticks=0)
        return d
    try:
        RN.disturbance = flaky
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(6, "measured")
            assert len(calls) == 2 and len(r.state["discarded_blocks"]) == 1 and recs
        RN.disturbance = lambda *a, **k: dict(real(*a, **k), disturbed=True, reasons=["made up"], runner_ticks=0)
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            try:
                r.timed_block(6, "measured")
            except RN.RunStop as e:
                assert "disturbed twice" in str(e)
            else:
                raise AssertionError("a second disturbance did not stop the run")
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_runner_ticks_per_launch():
    assert C.RUNNER_TICKS_PER_LAUNCH_MAX == 1.0 and C.RUNNER_TICKS_BLOCK_ALLOWANCE == 2
    assert K.runner_ticks_check(66, 64) == (66 / 64, False) and K.runner_ticks_check(67, 64)[1]
    real = RN.disturbance

    def others_ignored(*a, **k):
        return dict(real(*a, **k), disturbed=False, reasons=[])
    try:
        RN.disturbance = others_ignored
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(6, "measured")
            d = r.state["disturbance_checks"][-1]
            assert recs and d["spawns"] == 64 and not d["runner_over_limit"], d
            assert "per launch" in (r.out / "run.log").read_text()
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            parse = r.quick_parse

            def busy(rec):
                c = time.process_time()
                while time.process_time() - c < 0.03:
                    pass
                return parse(rec)
            r.quick_parse = busy
            try:
                r.timed_block(6, "measured")
            except RN.RunStop as e:
                d = r.state["disturbance_checks"][-1]
                assert "harness bug" in str(e) and r.state["harness_bugs"] and d["runner_over_limit"], str(e)
            else:
                raise AssertionError("a runner above 1 tick per launch plus 2 was not a harness bug")
    finally:
        RN.disturbance = real
        os.environ.pop("FAKE_CFG", None)


def test_pool_is_joined():
    r = RN.Runner(ns())
    r.state = {"elapsed_before": 0.0}
    r.t0 = time.monotonic()
    res = r.pool([(k, (lambda core, k=k: (k, core))) for k in range(6)])
    assert sorted(res) == list(range(6)) and threading.active_count() == 1


# ------------------------------------------------------------------------------------------------- the analysis
# made-up times per run: the operation, each input's build, the read-out
TIMES = {"TC": (150.0, 20.0, 5.0), "Hash-V-pack": (100.0, 30.0, 6.0), "Hash-V-pack-null": (100.5, 30.1, 6.0),
         "Pack-primes": (300.0, 60.0, 6.0), "Pack-coprime": (110.0, 40.0, 6.0)}


def analysis_state(op="add"):
    """a finished run's state for job 1 (n = 16): made-up times, and memory figures (TC 16 bytes per cell; Hash-V-pack
    2-byte cells and a list of 282 entries; the factored rows with (f), and (q))"""
    nin = len(C.SET_INPUTS[op])
    recs = []
    for k in range(1, 13):
        for row, (o, b, rd) in TIMES.items():
            builds = [b + 0.01 * k + i for i in range(nin)]
            recs.append({"job": 1, "row": row, "role": "round", "round": k, "operation": o + 0.1 * k, "read": rd,
                         "builds": builds, "total": o + 0.1 * k + rd + sum(builds), "cpu_share": 0.99, "cpu_ns": o,
                         "span_ns": o})
    q = 256
    stable = {"TC": 16 * q, "MHVP": 2 * q + 8 * 282, "MHPP": 2 * q + 8 * 282 + 4 * 283 + 9 * 500,
              "MHPC": 2 * q + 8 * 282 + 4 * 283 + 9 * 480 + 8 * 70}
    peak = {"TC": 16, "MHVP": 25, "MHPP": 50, "MHPC": 52}
    cells_op = C.op_cells(op, 16)

    def memrun(i):
        m = {f"built_{x}": {"acct": stable[i], "heap": stable[i] + 300, "peak_acct": stable[i] * (k + 1),
                            "base_acct": stable[i] * k, "peak_heap": 0, "base_heap": 0, "cells": q}
             for k, x in enumerate(C.SET_INPUTS[op])}
        m["after_creation"] = {"acct": stable[i] * nin, "heap": stable[i] * nin + 600, "cells": q * nin}
        m["op"] = {"peak_acct": peak[i] * cells_op, "peak_heap": peak[i] * cells_op + 1000, "cells": cells_op}
        m["end"] = {"acct": stable[i] * (nin + 1), "heap": 1, "cells": q * (nin + 1)}
        return {"memrun": m}
    stor = {"end": {x: dict(W=282, T=1024, w=64, wh=16, whb=16, packed=1, nf=512, found=230, new=282, probes=600,
                            longest=5, widenings=0, copied=0, growths=0, reinserted=0, rprobes=0, rewritten=0,
                            bcells=512, bvals=2256) for x in C.SET_MATRICES[op]}}
    res = C.SET_RESULT[op]
    fac = [dict(name=x, W=282, E=500, nq=0, w=64, bytes_f=5632, bytes_q=0, td_cand=7000, td_divs=7500, td_exact=500,
                rgcd=0, rdiv=0, splits=0, sgcd=0, sdiv=0, p2tests=0) for x in C.SET_MATRICES[op]]
    facc = dict({k: 0 for k in C.FACC_FIELDS}, merges=900, mentries=1500, tests=321, gen_eq=7)
    counts = {f"{i}:1": {"counts": ["ctr A entry core gcd_work 500"], "stor": stor,
                         "opc": {res: dict(cross=1024, cross_match=40, cross_reads=900)},
                         "packs": [dict(name="A", W=282, whb=16, whp=16, bytes_list=2256, rewritten=512,
                                        bytes_map=2048)] if i != "TC" else [],
                         "fac": fac if i in C.FAC_IMPLS else [],
                         "ffig": {"end": {x: dict(W=282, E=500, nq=70 if i == "MHPC" else 0, w=64, factored=1,
                                                  bytes_f=5632, bytes_q=560 if i == "MHPC" else 0)
                                          for x in C.SET_MATRICES[op]}} if i in C.FAC_IMPLS else {},
                         "facc": {res: facc} if i in C.FAC_IMPLS else {}} for i in C.IMPLS}
    chk = {"G_TC": 500, "G_TC_by_phase": {"entry": 300, "add": 200}, "result": "pass",
           "MHVP": {"call_for_call": "pass", "G": 500, "G_ratio": 1.0, "gcd_work_check": "passed"},
           **{i: {"call_for_call": "pass (case and core arithmetic)", "G": 300, "G_ratio": 0.6,
                  "G_ratio_by_part": {"entry": 1.0, "add": 0.0, "mul": None},
                  "gcd_work_check": "reported, not checked (rule-m3 item 7)"} for i in C.FAC_IMPLS},
           "Pack-primes against Hash-V-pack": "pass", "Pack-coprime against Hash-V-pack": "pass"}
    cal = {f"1:{row}": {"r": 4, "reps": 1 if row == "Pack-primes" else 3, "rep_ns": 4 * 10 ** 8} for row in C.ROWS}
    st = {"started": "x", "finished": "y", "op": op, "jobs": [1], "elapsed_saved": 300, "rule_sha256": C.RULE_SHA256,
          "records": recs, "calibration": cal, "toy": False, "counts": counts,
          "mems": {f"{i}:1": memrun(i) for i in C.IMPLS}, "call_checks": {"1": chk},
          "gen": {"1": {"n": 16, "rho": 1, "s": 256, "nops": C.nops(op, 16),
                        "realized": {"s": 256, "distinct": {x: 256 for x in C.SET_MATRICES[op]},
                                     "W": {x: 282 for x in C.SET_MATRICES[op]}},
                        "ncase": {"add_general": 200}, "steps": {z: {"ncase": {"add_general": 200}, "cross": 1024,
                                                                      "cross_match": 40, "cross_reads": 900}
                                                                  for _, _, _, z in C.SET_STEPS[op]}}},
          "gates": {f"{i}:1": {} for i in C.IMPLS}, "round3_after": "unchanged", "protected_after": "unchanged",
          "pilot": {"jobs": [1, 6, 10], "walls_s": {"1": 6.0, "6": 7.0, "10": 9.0}, "N": {"0": 4, "1": 4, "2": 4},
                    "blocks": {"0": {"job": 1, "O": 2.0, "K": {"TC": 0.2}, "P": {"TC": 0.4}, "t": {"TC": 0.01},
                                     "wall": 6.0}},
                    "projected_s": 900.0, "limit_s": 1800, "host_rates": {"1": 0.5}},
          "disturbance_checks": [{"job": 1, "label": "measured", "kind": "rounds", "attempt": 1, "seconds": 25.0,
                                  "disturbed": False, "reasons": [], "runner_ticks": 9, "spawns": 64,
                                  "forks_rise": 64, "forks_unattributed": 0, "hosts": {}, "other_limit": 30}]}
    return st


def test_analysis():
    """a finished state gives, per pairing (six), per-measure verdicts: measure 1 pooled over the inputs and the
    operation measure, each with its own band from the null row (Hash-V-pack's copy); total and read-out reported;
    the stable bytes and, in add and mul, the peak with verdicts (sop and pos: the peak reported); the reported pairing
    (Pack-coprime against Pack-primes) with no verdict; Hash-V-pack's gcd-work check and the factored rows' reported
    gcd work beside their factor work; over cap with the calibration rep's time as the magnitude; strata (sop: no
    MID) and grid; the factored rows' counters; a stopped run gives no verdict"""
    import statistics
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for op in C.OPS:
            st = analysis_state(op)
            (t / "state.json").write_text(json.dumps(st))
            (t / "results.md").write_text("# head\n<!-- rule ends -->\nold tail\n")
            A.report(t)
            md = (t / "results.md").read_text()
            assert "old tail" not in md and md.startswith("# head")
            for row, ref, kind in C.PAIRINGS:
                assert f"### {row} against {ref} ({kind})" in md, (row, ref)
            n, what = C.OP_MEASURE[op]
            assert f"{n} {what}" in md and "6 stable bytes of one stored input matrix" in md
            sec = md[md.find("### Pack-primes against TC"):].split("\n\n")[1]
            nin = len(C.SET_INPUTS[op])
            b_tc = statistics.median([20.0 + 0.01 * k + i for k in range(1, 13) for i in range(nin)])
            b_pp = statistics.median([60.0 + 0.01 * k + i for k in range(1, 13) for i in range(nin)])
            assert f"| 1 (n 16, rho 1) | loss, {b_tc / b_pp:.3f}" in sec, (sec[:600], b_tc / b_pp)
            assert "| loss, 0.50" in sec and "(c)" in sec                       # Pack-primes ran 1 rep per invocation
            sp = (2 * 256 + 8 * 282 + 4 * 283 + 9 * 500) / (16 * 256)
            assert f"{A.fixed_verdict(sp)}, {sp:.3f} (heap" in sec, (sp, sec)
            if C.PEAK_MEASURE[op]:
                assert "loss, 3.125 (heap" in sec and "**severe memory loss**" in sec   # the peak, 50 / 16
            else:
                assert "reported, 3.125 (heap" in sec
            sec = md[md.find("### Pack-coprime against Pack-primes"):].split("\n\n")[1]
            assert "win" not in sec and "loss" not in sec and "tie" not in sec and "severe" not in sec
            assert sec.count("reported") >= 6
            sec = md[md.find("### Pack-coprime against Hash-V-pack"):].split("\n\n")[1]
            assert "loss, " in sec
            assert "1.000000 (passed), pass" in md and "0.6000 (1.0000, 0.0000, n/a), pass (case and core arithmetic)" in md
            nm = len(C.SET_MATRICES[op])                                        # factor work beside the gcd work
            assert f"| 1 | Pack-primes | 0.6000 | 900 | 1500 | 321 | 0 | {7000 * nm} | 0 | 0 | 0 | 0 / 0 / 0 |" in md
            strata = "| NONE | Pack-primes vs TC | stable |" in md
            assert strata and ("| MID |" in md) == (op != "sop")
            assert ("| NONE | Pack-primes vs TC | peak |" in md) == bool(C.PEAK_MEASURE[op])
            assert "| NONE | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 1 |" in md
            grid = md[md.find("Pack-primes against TC, 1 build time"):]
            assert f"| 16 | {b_tc / b_pp:.3f} | n/a | n/a | n/a |" in grid.split("\n\n")[1]
            assert "| 1 | A | 282 | 16 | 16 | 2256 | 512 | 2048 |" in md
            assert "| 1 | Pack-primes | A | 282 | 500 | 5632 | 0 | 0 | 7000 | 7500 | 500 |" in md
            assert f"| 1 | Pack-coprime | {C.SET_RESULT[op]} | 0 passes" in md and "900 (1500)" in md
            assert "results-m2-sop/results.md" in md and "| no | 9 | 64 |" in md
            v = list(csvrows(t / "verdicts.csv"))
            got = {(x["pairing"], x["measure"], x["verdict"]) for x in v}
            assert {("Pack-primes vs TC", "build", "loss"), ("Pack-primes vs TC", "total", "reported"),
                    ("Pack-coprime vs Pack-primes", "operation", "reported"),
                    ("Pack-coprime vs Pack-primes", "stable bytes", "reported"),
                    ("Hash-V-pack vs TC", "gcd work (check)", "passed"),
                    ("Pack-primes vs TC", "gcd work (reported)", "reported")} <= got, got
            assert len(list(csvrows(t / "rounds.csv"))) == 5 * 12
            f = list(csvrows(t / "factoring.csv"))
            assert {x["kind"] for x in f} == {"factoring", "end", "step"}
        st = analysis_state("sop")                            # one band per measure: a noisy null on the operation
        for x in st["records"]:
            if x["row"] == C.NULL_ROW:
                x["operation"] *= 1.3 if x["round"] % 2 else 0.8
        d = A.Data(st)
        hb, _ = A.null_h(d, 1, "build")
        ho, _ = A.null_h(d, 1, "operation")
        assert hb == C.BAND_MIN and ho > 0.1, (hb, ho)
        st2 = analysis_state()                                # over cap: both time measures, with the magnitude
        st2["calibration"]["1:Pack-primes"].update(overcap=True, rep_ns=14.2e9)
        st2["records"] = [x for x in st2["records"] if x["row"] != "Pack-primes"]
        d = A.Data(st2)
        for m in ("operation", "build"):
            x = A.time_measure(d, 1, "Pack-primes", "TC", m, "against TC")
            assert x["verdict"] == "over cap" and x["cap_rep_ns"] == {"Pack-primes": 14.2e9}
            assert A.tcell(x) == "over cap (calibration rep Pack-primes 14.2 s)"
        assert A.time_measure(d, 1, "Pack-primes", "TC", "total", "against TC")["verdict"] == "reported"
        assert A.time_measure(d, 1, "Pack-coprime", "Pack-primes", "operation", C.REPORTED_KIND)["verdict"] == \
            "reported"
        assert A.memory_measure(d, 1, "Pack-primes", "TC", "against TC", "stable")["verdict"] in ("win", "tie", "loss")
        (t / "state.json").write_text(json.dumps(st2))
        A.report(t)
        md = (t / "results.md").read_text()
        assert "over cap (calibration rep Pack-primes 14.2 s)" in md
        st["run_stopped"] = {"step": "step 4", "reason": "a gate failed"}
        (t / "state.json").write_text(json.dumps(st))
        A.report(t)
        md = (t / "results.md").read_text()
        assert "No verdict is reported" in md and "| 1 (n 16, rho 1) | loss" not in md


def csvrows(p):
    import csv
    with open(p) as f:
        yield from csv.DictReader(f)


# ------------------------------------------------------------------------------------------------- toy runs
def test_toy_runs_end_to_end():
    """the runner as the operator starts it, with --toy: the add run with --pilot-only and then --resume (steps 4-6
    after the pilot), and mul, sop and pos one after another, each in one go; each finishes, in its own toy folder,
    with every gate and call-for-call check passed and every pairing reported, and none touches another's folder. A
    harness test: nothing here is a measurement. (The disturbance check is the real one: another process busy on the
    machine stops a toy run as it would a measured one.)"""
    py = [sys.executable, str(C.HERE / "m3_runner.py")]
    out = {op: C.TOY_BUILD / f"results-{op}" for op in C.OPS}
    p = run(py + ["--op", "add", "--toy", "--jobs", "6", "--pilot-only"], timeout=900)
    assert p.returncode == 0 and "pilot only: stopping after step 3" in p.stdout, p.stdout[-1500:] + p.stderr[-1500:]
    st = json.loads((out["add"] / "state.json").read_text())
    assert "pilot" in st and not st.get("records") and not st.get("gates") and not st.get("finished")
    p = run(py + ["--op", "add", "--toy", "--jobs", "6", "--resume"], timeout=900)
    assert p.returncode == 0, p.stdout[-1500:] + p.stderr[-1500:]
    for op in ("mul", "sop", "pos"):
        p = run(py + ["--op", op, "--toy", "--jobs", "6"], timeout=900)
        assert p.returncode == 0, p.stdout[-1500:] + p.stderr[-1500:]
    for op in C.OPS:
        st = json.loads((out[op] / "state.json").read_text())
        assert st["op"] == op and st["toy"] and st.get("finished") and not st.get("run_stopped"), st.get("run_stopped")
        assert len(st["code_sessions"]) == (2 if op == "add" else 1)
        assert st["call_checks"]["6"]["result"] == "pass" and not st.get("bugs")
        assert all("fail" not in g for g in st["gates"].values()) and len(st["gates"]) == 4
        assert len([x for x in st["records"] if x["role"] == "round"]) == 5 * C.ROUNDS
        assert all(len(x["builds"]) == len(C.SET_INPUTS[op]) for x in st["records"] if x["role"] == "round")
        assert st["gen"]["6"]["name"] == f"{op}-6" and st["round3_after"] == "unchanged"
        assert st["protected_after"] == "unchanged"
        md = (out[op] / "results.md").read_text()
        assert "TOY RUN" in md and "<!-- rule ends -->" in md and "## Verdicts, per job" in md
        assert all(f"### {row} against {ref} ({kind})" in md for row, ref, kind in C.PAIRINGS)
    assert C.round3_check() is None


# ------------------------------------------------------------------------------------------------- full size
def full():
    """every gate and counting run on the full-size workloads of every set (untimed; Pack-primes' gates at n = 64 take
    seconds each, and the model a minute or so per set)"""
    build_once()
    for op in C.OPS:
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            w = t / "work"
            m3_gen.generate(w, op, list(C.JOBS_OF[op]))
            for job in C.JOBS_OF[op]:
                check_job(op, w, job, t, mem=False)
                print(f"  full size: {op}-{job} gates pass for the four rows, the TC gate, call for call", flush=True)


TESTS = [test_rule_fixed_and_pinned, test_pins_m1_m2, test_forbidden_places, test_runner_imports_no_numpy,
         test_runner_arguments, test_workloads, test_model_worked_by_hand, test_build_and_arithmetic_unchanged,
         test_factored_rows_one_source, test_schoolbook_trial_division, test_no_reserved_slot_no_handle_test,
         test_only_named_structures, test_c_suites, test_gates_and_counts_toy, test_gates_on_the_splitting_examples,
         test_exact_checks_catch_a_difference, test_untimed_steps_toy, test_step0_against_earlier,
         test_statistics_and_verdicts, test_pilot_projection_and_allowance, test_pilot_stops,
         test_host_allowance_from_pilot, test_spawner_affinity_before_exec, test_hard_stop_kernel_timer,
         test_idle_probe, test_disturbance_synthetic, test_existing_child_exit_charged_for_the_block_only,
         test_host_threads_do_not_disturb, test_disturbance_catches_short_lived_process, test_timed_block_machinery,
         test_block_rerun_once, test_runner_ticks_per_launch, test_pool_is_joined, test_analysis,
         test_toy_runs_end_to_end]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--full", action="store_true")
    ap.add_argument("--only", default="", help="run only the tests whose names contain this text")
    a = ap.parse_args()
    tests = [t for t in TESTS if a.only in t.__name__]
    failed = 0
    for t in tests:
        try:
            t()
            print(f"pass  {t.__name__}", flush=True)
        except Exception as e:
            failed += 1
            import traceback
            traceback.print_exc()
            print(f"FAIL  {t.__name__}: {e}", flush=True)
    if a.full:
        full()
    print(f"{len(tests) - failed} of {len(tests)} harness tests pass")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
