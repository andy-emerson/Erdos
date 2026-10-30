"""Tests of round m2's harness (not measurements; nothing here is timed or reported as a time). From test_m1_harness.py
(unchanged, m1's), with every test that applies carried over and adapted to this round's five rows and four job sets,
and new ones.

    python3 test_m2_harness.py            the self-tests: the fixed rule and its pin; the 08 module, tc3.c, tcm.c's
                                          recorded diff, m1's sources and round 3's results; the places the harness
                                          may not write (m1's results, other m2 runs); the workloads of the four job
                                          sets (add and mul equal to m1's); the model worked by hand (no reserved slot,
                                          growth from slot 0, the packing); tcm.c unchanged and mh2.c's arithmetic
                                          bodies m1's line for line; the three hash rows from one source; no handle
                                          test for 0 or 1; the allocations (only the named structures); the C suites;
                                          the binaries' exact checks on toy workloads of every set (every row, every
                                          table, list, build table and handle field, the TC gate, call for call, pack
                                          and grow-map against fixed); the runner's untimed steps; statistics and the
                                          pooled bootstrap; the pilot's projection and stops; fork/exec with the
                                          affinity set before exec; the kernel timer; the idle check; the disturbance
                                          check; the timed block's machinery with a stand-in driver (five rows,
                                          rotated; each input's build, the operation, the read-out; each quantity its
                                          own minimum); the runner's ticks per launch; the analysis (six pairings, the
                                          eight headline measures, the reported pairing, the bands, memory, strata and
                                          grid); and a toy run of each job set end to end (--toy; add with
                                          --pilot-only then --resume)
    python3 test_m2_harness.py --full     also every gate and counting run on the full-size workloads of every set
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
from pathlib import Path

import m1_gen
import m2_analyze as A
import m2_checks as K
import m2_common as C
import m2_gen
import m2_libconf as L
import m2_model
import m2_runner as RN

TB = C.HERE / "build" / "m2test"
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
    if not (w / "gen.json").exists() or json.loads((w / "gen.json").read_text()).get("1", {}).get("steps") is None:
        m2_gen.generate(w, op, list(C.JOBS), toy=True)
    return w


def run(args, **kw):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True, **kw)


# ------------------------------------------------------------------------------------------------- the rule, pins
def test_rule_fixed_and_pinned():
    """the rule on disk is the pinned, approved one (committed at 38f2d5d, no proposal mark); another text, or one with
    a proposal mark, is refused for a measured run of every job set"""
    text = C.RULE_FILE.read_text()
    assert C.RULE_SHA256 == "e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200"
    assert C.sha256(C.RULE_FILE) == C.RULE_SHA256, "rule-m2.md differs from the pinned hash"
    assert C.PROPOSAL_MARK not in text and C.rule_check() is None, C.rule_check()
    assert "**Fixed: approved by the author, Oct 30, 2026,**" in text
    r = run(["git", "log", "--format=%h", "-1", "--", str(C.RULE_FILE)], cwd=C.HERE)
    assert r.stdout.strip() == C.RULE_COMMIT == "38f2d5d", r.stdout      # committed once, unchanged since
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule-m2.md"
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
                tf = Path(tempfile.mkdtemp()) / "rule-m2.md"
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


def test_pins_and_m1():
    """the 08 module, tc3.c and its headers, tcm.c's recorded diff, m1's sources (tcm.c, mh.c, m1common.c, their
    headers) and round 3's results are as pinned; a change to any of them is caught"""
    assert L.vendor08_check() is None and L.tc3_check() is None and L.tcm_diff_check() is None
    assert L.m1_sources_check() is None, L.m1_sources_check()
    assert C.round3_check() is None, C.round3_check()
    for name in L.M1_SOURCES_SHA256:                     # as m1 committed them, at its last commit
        r = run(["git", "show", f"6e9a85e:11-matrix/lib/{name}"], cwd=C.HERE)
        assert r.returncode == 0 and r.stdout == (L.LIBDIR / name).read_text(), name
    with tempfile.TemporaryDirectory() as t:
        fake = Path(t)
        shutil.copytree(L.LIBDIR / "src", fake / "src")
        shutil.copytree(L.LIBDIR / "include", fake / "include")
        saved = L.LIBDIR
        try:
            L.LIBDIR = fake
            assert L.m1_sources_check() is None
            (fake / "src" / "mh.c").write_text((fake / "src" / "mh.c").read_text() + "\n")
            assert "mh.c" in L.m1_sources_check()
        finally:
            L.LIBDIR = saved
    for op in C.OPS:                                     # the runner refuses when m1's sources changed
        saved = dict(L.M1_SOURCES_SHA256)
        try:
            L.M1_SOURCES_SHA256["src/tcm.c"] = "0" * 64
            try:
                RN.Runner(ns(op=op)).preflight()
            except SystemExit as e:
                assert "m1's sources changed" in str(e)
            else:
                raise AssertionError("a changed m1 source was not refused")
        finally:
            L.M1_SOURCES_SHA256.clear()
            L.M1_SOURCES_SHA256.update(saved)


def test_forbidden_places():
    """nothing is written anywhere in ../10-interning/ or in m1's results folders (runner, generator, analysis); a run
    writes in no other m2 run's folder; m1's folders and the other m2 folders are hashed before and after each run"""
    places = [C.ROUND3, C.ROUND3 / "test" / "results-r3", C.M1_RESULTS["add"], C.M1_RESULTS["mul"] / "x"]
    for p in places:
        for op in (None,) + C.OPS:
            assert C.forbidden(p, op), (p, op)
        r = RN.Runner(ns(toy=True))
        r.out = p
        try:
            r.preflight()
        except SystemExit as e:
            assert "10-interning" in str(e) or "m1's" in str(e)
        else:
            raise AssertionError(f"the runner would write into {p}")
        for f in (lambda: A.report(p), lambda: m2_gen.generate(p, "add", [1], toy=True)):
            try:
                f()
            except SystemExit as e:
                assert "10-interning" in str(e) or "m1's" in str(e)
            else:
                raise AssertionError(f"would write into {p}")
    for op in C.OPS:
        assert not C.forbidden(C.RESULTS[op], op) and C.RESULTS[op].name == f"results-m2-{op}"
        for other in C.OPS:
            if other != op:
                assert C.forbidden(C.RESULTS[other], op) and C.forbidden(C.RESULTS[other] / "x", op)
                r = RN.Runner(ns(op=op, toy=True))
                r.out = C.RESULTS[other]
                try:
                    r.preflight()
                except SystemExit as e:
                    assert f"m2 {other} run's folder" in str(e)
                else:
                    raise AssertionError(f"the {op} run would write into the {other} run's folder")
        prot = C.protected(op)
        assert set(prot) == {"m1-add", "m1-mul"} | {f"m2-{o}" for o in C.OPS if o != op}
    for k in ("add", "mul"):
        assert {"results.md", "state.json"} <= set(C.folder_hashes(C.M1_RESULTS[k]))


def test_runner_imports_no_numpy():
    r = run([sys.executable, "-c", "import sys, m2_runner; print('numpy' in sys.modules)"], cwd=C.HERE)
    assert r.stdout.strip() == "False", r.stdout + r.stderr


def test_runner_arguments():
    """--op is required (add, mul, sop or pos; no option runs the four in one command); --hard-stop-s and --jobs are for
    toy runs; --resume is refused after the hard stop or the pilot's stop, once a timed round has run, and on another
    job set's state"""
    r = run([sys.executable, str(C.HERE / "m2_runner.py"), "--toy"])
    assert r.returncode != 0 and "--op" in r.stderr
    for bad in ("div", "all"):
        r = run([sys.executable, str(C.HERE / "m2_runner.py"), "--op", bad, "--toy"])
        assert r.returncode != 0 and "invalid choice" in r.stderr
    r = run([sys.executable, str(C.HERE / "m2_runner.py"), "--help"])
    assert "--all" not in r.stdout and "--pilot-only" in r.stdout and "--resume" in r.stdout and "--toy" in r.stdout
    try:
        RN.Runner(ns(toy=False, hard_stop_s=1.0))
    except SystemExit as e:
        assert "toy" in str(e)
    else:
        raise AssertionError("--hard-stop-s accepted for a measured run")
    for op in C.OPS:
        r = RN.Runner(ns(op=op))
        assert r.out == C.TOY_BUILD / f"results-{op}" and r.wl == C.TOY_BUILD / f"work-{op}"
        r = RN.Runner(ns(op=op, toy=False))
        assert r.out == C.RESULTS[op] and r.jobs == list(C.JOBS) and r.hard_stop == 40 * 60
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
    """rule-m2, Workloads: add and mul are m1's (m1_gen's own draw; the workloads and references equal m1's recorded
    files by sha256); sop and pos draw A, B, C, D as m1 draws A and B, from one stream in that order, seeded
    20261029 + 200 + j and + 300 + j, each with exactly s distinct values and its own pool; deterministic; the grid is
    m1's"""
    from fractions import Fraction
    assert C.SIZES == (16, 32, 64, 128) and C.RHOS == (1, 4, 16, 64) and C.JOBS == tuple(range(1, 17))
    for op in C.OPS:
        for job in C.JOBS:
            assert C.job_seed(op, job) == 20261029 + {"add": 0, "mul": 100, "sop": 200, "pos": 300}[op] + job
    for op in ("add", "mul"):
        for job in (1, 2, 3, 4):
            assert m2_gen.workload(op, job)[2] == list(m1_gen.workload(op, job)[2:])
    for op in ("sop", "pos"):
        for job in (1, 4, 6, 8):
            n, s, X = m2_gen.workload(op, job)
            assert (n, s) == (C.job_n(job), C.job_s(job)) and len(X) == 4
            for M in X:
                assert len(M) == n * n and all(-32767 <= m <= 32767 and d in m1_gen.DIVS720 for m, d in M)
                assert len({Fraction(m, d) for m, d in M}) == s
            assert len({tuple(M) for M in X}) == 4
            assert m2_gen.workload(op, job) == (n, s, X)
        assert m2_gen.workload("sop", 1)[2][0] != m2_gen.workload("pos", 1)[2][0]
    with tempfile.TemporaryDirectory() as t:
        for op in ("add", "mul"):                         # step 0's files equal m1's, by sha256 (rule-m2, Workloads)
            s = m2_gen.generate(Path(t) / op, op, [1, 4], toy=False)
            m1 = json.loads((C.M1_RESULTS[op] / "state.json").read_text())["gen"]
            for j in ("1", "4"):
                for f in (f"job-0{j}.wl", f"job-0{j}.ref"):
                    assert s[j]["sha256"][f] == m1[j]["sha256"][f], (op, j, f)
        s = m2_gen.generate(Path(t) / "sop", "sop", [4], toy=False)
        assert set(s["4"]["sha256"]) == {"job-04.wl", "job-04.ref", "job-04.m2.json", "job-04.P.ref", "job-04.Q.ref"}
        s2 = m2_gen.generate(Path(t) / "again", "sop", [4], toy=False)
        assert s2["4"]["sha256"] == s["4"]["sha256"]       # deterministic, byte for byte
        op, n, job, X = C.read_workload(Path(t) / "sop" / "job-04.wl")
        assert (op, n, job) == ("sop", 16, 4) and X == m2_gen.workload("sop", 4)[2]
        op, n, job, X = C.read_workload(Path(t) / "add" / "job-04.wl")
        assert (op, n, job) == ("add", 16, 4) and X == m2_gen.workload("add", 4)[2]


def test_model_worked_by_hand():
    """spec v0.2 §15.2 and §15.4 by hand: every table empty, 0 hashed like any value (home 0), W counting only what is
    stored; grow's first growth at the 9th new whole number, reinserting from slot 0; the packing: the list in slot
    order, every field mapped, the width shrunk; the pack width thresholds; small jobs of every set"""
    assert m2_model.pack_width(1) == 8 and m2_model.pack_width(128) == 8 and m2_model.pack_width(129) == 16
    assert m2_model.pack_width(32768) == 16 and m2_model.pack_width(32769) == 32
    for n, tfix, wh in ((16, 1024, 16), (32, 4096, 16), (64, 16384, 16), (128, 65536, 32)):
        assert 1 << m2_model.t_fix_bits(n, n) == tfix and m2_model.handle_width(n, n, False) == wh
    S = m2_model.Store("A", 4, 4, "grow", True)
    assert (S.T, S.W, S.w) == (16, 0, 64) and set(S.tab) == {-1}
    S.store_cell(0, 1)
    assert S.W == 2 and S.c["new"] == 2 and S.fields[0] == 2 * C.home(0, 4) and C.home(0, 4) == 0
    assert S.fields[1] == 2 * C.home(1, 4)
    for k in range(2, 8):                                   # W = 8 after 0, 1 and 2 .. 7
        S.store_cell(k, 1)
    assert S.W == 8 and S.T == 16 and S.c["growths"] == 0
    S.store_cell(-9, 1)                                     # 9, the 9th new whole number: a growth at its numerator
    g = S.growth_log[0]
    assert S.T == 32 and g["W"] == 8 and g["T_old"] == 16 and g["fields"] == 14 and g["reinserted"] == 8
    assert g["bytes_map"] == 16 * S.wh // 8 and g["rewritten"] == 14
    assert [S.value(i) for i in range(8)] == [(0, 1)] + [(k, 1) for k in range(2, 8)] + [(-9, 1)]
    P = m2_model.Store("A", 2, 2, "pack", True)
    F = m2_model.Store("A", 2, 2, "fixed", True)
    for m, d in ((3, 5), (-5, 3), (0, 1), (7, 1)):
        P.store_cell(m, d)
        F.store_cell(m, d)
    tab = list(P.tab)
    P.finish()
    F.finish()
    assert P.list == [v for v in tab if v != -1] and P.hb == tab == F.tab and P.packed and not F.packed
    assert P.pack_rec["W"] == 5 and P.pack_rec["whb"] == 8 and P.pack_rec["whp"] == 8 and P.wh == 8
    pos = {s: k for k, s in enumerate(s for s, v in enumerate(tab) if v != -1)}
    assert P.fields == [2 * pos[f >> 1] + (f & 1) for f in F.fields]
    assert [P.value(i) for i in range(4)] == [(3, 5), (-5, 3), (0, 1), (7, 1)]
    assert P.figures()["bvals"] == 5 * 8 and F.figures()["bvals"] == 16 * 8
    R = m2_model.Store("R", 2, 2, "pack", False)            # a result: 8 bits, widened by copy
    R.store_cell(1000, 1)
    assert R.w == 16 and R.c["widenings"] == 1
    # 1/2 + -1/2 (zero); every set on 1x1 matrices
    p = m2_model.predict("add", 1, [[(1, 2)], [(-1, 2)]])
    assert p["values"]["C"] == [(0, 1)] and p["ncase"] == {"add_zero": 1} and p["steps"]["C"]["cross_match"] == 2
    assert p["sizings"]["fixed"]["stor_end"]["C"]["W"] == 2           # 0 and 1, stored because the cell uses them
    p = m2_model.predict("sop", 1, [[(3, 2)], [(4, 3)], [(1, 2)], [(1, 2)]])  # P = 2, Q = 1/4, R = 9/4
    assert p["values"]["P"] == [(2, 1)] and p["values"]["Q"] == [(1, 4)] and p["values"]["R"] == [(9, 4)]
    assert p["nops"] == 2 + 2 + 1 and p["steps"]["R"]["ncase"] == {"add_int_x": 1}
    p = m2_model.predict("pos", 1, [[(1, 2)], [(1, 2)], [(2, 3)], [(1, 3)]])  # S = 1, U = 1, R = 1
    assert p["values"]["S"] == [(1, 1)] and p["values"]["R"] == [(1, 1)]
    assert p["steps"]["R"]["ncase"] == {"mul_one_x": 1, "add_zero_x": 1}
    assert [x["name"] for x in p["sizings"]["pack"]["packs"]] == ["A", "B", "C", "D", "S", "U", "R"]


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


def test_arithmetic_unchanged():
    """spec v0.2 §15.1 ("Unchanged: the arithmetic and its cases") and §15.8: tcm.c is m1's, unchanged (and so
    tc3.c's bodies, by m1's recorded diff); mh2.c's arithmetic bodies are mh.c's line for line (add_body, mul_cells,
    add_values; add_cells but its call of the cross tests, which no longer takes the handles)"""
    assert L.m1_sources_check() is None and L.tcm_diff_check() is None
    m1, m2 = (L.SRC / "mh.c").read_text(), (L.SRC / "mh2.c").read_text()
    for name in ("add_body", "mul_cells", "add_values", "store"):
        a, b = c_function(m1, name), c_function(m2, name)
        b = b.replace("compare_cells(c, &o, 1)", "compare_cells(c, &o, x, y, 1)")
        assert a == b, name
    a, b = c_function(m1, "add_cells"), c_function(m2, "add_cells")
    assert a.replace("compare_cells(c, &o, x, y, 0)", "compare_cells(c, &o, 0)") == b


def test_hash_rows_one_source():
    """spec v0.2 §§15.3, 15.8: the three hash rows are one source, mh2.c, built with only -D settings (fixed or grow,
    and the packing); packing builds on fixed only; the prefixes mhvf_, mhvgm_, mhvp_; the rows and their binaries"""
    assert C.HASH_FLAGS == {"MHVF": (0, 0), "MHVGM": (1, 0), "MHVP": (0, 1)}
    for i in C.HASH_IMPLS:
        src, defs = L.OBJECTS[i.lower()]
        assert src == L.SRC / "mh2.c" and defs == L.hash_defs(i)
        assert L.IMPL_DEFS[i] == ["-DIMPL_HASH"] + defs and L.ROW_OBJECTS[i] == (i.lower(), "common", "m1common",
                                                                                "m2common")
    assert L.OBJECTS["tcm"][0] == L.SRC / "tcm.c" and L.ROW_OBJECTS["TC"] == ("tcm", "common", "m1common", "m2common")
    for g, p, ok, prefix in ((0, 0, True, "mhvf_"), (1, 0, True, "mhvgm_"), (0, 1, True, "mhvp_"), (1, 1, False, "")):
        r = run(["gcc", "-std=c11", "-fsyntax-only", f"-DMH_GROW={g}", f"-DMH_PACK={p}"] + L.inc() + [L.SRC / "mh2.c"])
        assert (r.returncode == 0) == ok, (g, p, r.stderr[-300:])
    r = run(["gcc", "-std=c11", "-fsyntax-only"] + L.inc() + [L.SRC / "mh2.c"])
    assert r.returncode != 0
    assert C.ROWS == ("TC", "Hash-V-fixed", "Hash-V-grow-map", "Hash-V-pack", "Hash-V-fixed-null")
    assert C.ROW_IMPL["Hash-V-fixed-null"] == "MHVF" and C.ROWS[-1] == C.NULL_ROW
    assert set(L.BUILDS) == {"timing", "counting", "test", "memory"} and L.BUILDS["memory"] == ["-DM2_MEM"]
    assert L.LINK == {"memory": L.WRAP}


def test_no_reserved_slot_no_handle_test():
    """spec v0.2 §15.2 (rule-m2 item 12's new bugs): no table reserves a slot or holds 0 or 1 in advance; no zero,
    integer or one test compares a handle; every store probes"""
    src = strip_audit((L.SRC / "mh2.c").read_text())
    assert "H_ZERO" not in src and "H_ONE" not in src
    tn = c_function(src, "table_new")
    assert "slot_set(tab, w, s, EMPTY)" in tn and tn.count("slot_set") == 1
    sw = c_function(src, "store_whole")
    assert "v == 0" not in sw and "v == 1" not in sw and sw.index("probe(") < sw.index("found == 1")
    cc = c_function(src, "compare_cells")
    for t in ("q.zx = o->v[0] == 0", "q.zy = o->v[2] == 0", "q.ix = o->v[1] == 1", "q.iy = o->v[3] == 1",
              "q.ox = six && o->v[0] == o->v[1]", "q.oy = six && o->v[2] == o->v[3]"):
        assert t in cc, t
    assert "x.i" not in cc and "x.j" not in cc and "y.i" not in cc
    gr = c_function(src, "grow")
    assert "for (uint32_t s = 0; s < T; s++)" in gr and "s = 2" not in gr
    assert "x->W = 0;" in src[src.index("X *MHF(create)"):src.index("void MHF(destroy)")]


def test_only_named_structures():
    """spec v0.2 §15.3, "No other structure exists": by the code, outside the test build's audits, the only allocations
    are each matrix's header, the cells (a) and the table (b) ((h) in Hash-V-pack) through table_new, the widened table
    (w), and under their own settings only the slot map (r) and the grown table (n) (MH_GROW), and the list (l), the
    pack map (p) and the cells' shrinking realloc (MH_PACK). By the allocations: the allocation check binaries (timing,
    counting and memory builds)."""
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
    mh = (L.SRC / "mh2.c").read_text()
    assert sites(strip_audit(mh)) == sorted([
        "sk_xcalloc(1, sizeof *x)",                              # the header
        "sk_xmalloc((size_t)2 * R * K * (x->wh / 8))",           # (a) the cells
        "sk_xmalloc((size_t)T * (w / 8))",                       # (b), (h), and (n) in a growth: table_new
        "sk_xmalloc((size_t)x->T * (w2 / 8))",                   # (w) the widened table
        "sk_xmalloc((size_t)T * (x->wh / 8))",                   # (r) the slot map
        "sk_xmalloc((size_t)W * (w / 8))",                       # (l) the list
        "sk_xmalloc((size_t)T * (wb / 8))",                      # (p) the pack map
        "sk_xrealloc(x->cells, (size_t)nf * (wp / 8))"]), sites(strip_audit(mh))   # (a) shrunk in place
    assert guards(mh, "void *r = sk_xmalloc") == ["#if MH_GROW"]
    assert guards(mh, "void *nt = table_new(T2") == ["#if MH_GROW"]
    for n in ("void *list = sk_xmalloc", "void *p = sk_xmalloc", "x->cells = sk_xrealloc"):
        assert guards(mh, n)[0] == "#if MH_PACK", n
    assert mh.count("table_new(") == 3                       # its definition, create's (b) or (h), grow's (n)
    build_once()
    for b in L.ALLOC_BUILDS:
        p = run([BIN / f"m2_alloc-{b}"])
        assert p.returncode == 0 and " 0 failures" in p.stdout and f"({b} build)" in p.stdout, p.stdout + p.stderr
        for row in ("TC", "Hash-V-fixed", "Hash-V-grow-map", "Hash-V-pack"):
            assert f"  {row}: at most " in p.stdout
        assert "Hash-V-grow-map: at most" in p.stdout and " 0 packings seen" in p.stdout.split("Hash-V-grow-map")[1]


def test_c_suites():
    """the unit, differential, allocation and sanitizer suites of the Run plan's step 2 (the runner's own commands)"""
    build_once()
    for name, cmd, env in L.unit_commands(BIN):
        p = run(cmd, env=dict(os.environ, **env) if env else None)
        assert p.returncode == 0 and " 0 failures" in p.stdout, (name, p.stdout[-500:] + p.stderr[-500:])
        if name == "m2_unit":
            for what in ("zero, integer and +-1 cases by value, 0 and 1 at arbitrary slots",
                         "growth triggered by a denominator's store: in A, in B, in a result",
                         "a growth and a widening in one store of a result", "widening: 8 -> 16 -> 32 -> 64",
                         "packing: every packed width (8, 16, 32), every pair of widths in place",
                         "expressions: sop and pos against the exact reference", "intermediates of every width",
                         "w_h at the thresholds of both rules", "overflow: 2^63, -2^63",
                         "a packed handle >= W"):
                assert what in p.stdout, what


# ------------------------------------------------------------------------------------------------- the binaries
def gate(impl, op, w, job, t):
    p = run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out", t / f"gp-{impl}.bin",
             t / f"gd-{impl}.bin"])
    assert p.returncode == 0, p.stdout[-500:] + p.stderr
    assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
    for x in C.SET_INTERMEDIATES[op]:
        assert K.readout_check(Path(f"{t / 'g.out'}.{x}"), w / f"job-{job:02d}.{x}.ref") is None, (op, job, x)
    return p.stdout.splitlines(), (C.read_dump(t / f"gd-{impl}.bin") if (t / f"gd-{impl}.bin").exists() else None)


def test_gates_and_counts_toy():
    """every toy job of every set: each row's gate (test build) passes its exact checks against the model, every table,
    list, build table and handle field; every intermediate against fractions; the TC gate: tc3.c's per-operation
    record equal to tcm.c's, every step; the counting runs pass the same checks, and every hash row's record equals
    TC's; pack and grow-map have fixed's W and found/new; pack's build tables are fixed's tables; the memory runs read
    out the reference, their running totals agree with the reports, and the intermediates are gone at the end"""
    build_once()
    for op in C.OPS:
        w = toy_work(op)
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            for job in C.JOBS:
                pred = json.loads((w / f"job-{job:02d}.m2.json").read_text())["predicted"]
                dumps, stor = {}, {}
                for impl in C.IMPLS:
                    lines, dump = gate(impl, op, w, job, t)
                    fails = K.exact_checks(impl, op, lines, pred, dump)
                    assert not fails, (op, job, impl, fails[:5])
                    dumps[impl] = dump
                    assert bool(K.parse_growths(lines)) <= (impl == "MHVGM") and bool(K.parse_packs(lines)) == \
                        (impl == "MHVP")
                    p = run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                             t / f"p-{impl}.bin"])
                    assert p.returncode == 0 and not K.exact_checks(impl, op, p.stdout.splitlines(), pred)
                    assert K.readout_check(t / "c.out", w / f"job-{job:02d}.ref") is None
                    assert (t / f"p-{impl}.bin").read_bytes() == (t / f"gp-{impl}.bin").read_bytes()
                    stor[impl] = K.store_split(p.stdout.splitlines())
                    p = run([BIN / f"bench-{impl}-memory", "mem", w / f"job-{job:02d}.wl", t / "m.out"])
                    mr = K.parse_memrun(p.stdout.splitlines())
                    assert p.returncode == 0 and K.readout_check(t / "m.out", w / f"job-{job:02d}.ref") is None, \
                        p.stdout[-300:]
                    n = C.job_n(job, True)
                    assert {f"built_{x}" for x in C.SET_INPUTS[op]} | {"after_creation", "op", "end", "growth"} \
                        <= set(mr)
                    assert mr["op"]["cells"] == C.op_cells(op, n) and mr["end"]["cells"] == \
                        (len(C.SET_INPUTS[op]) + 1) * n * n
                    assert mr["op"]["peak_acct"] >= mr["end"]["acct"] >= mr["after_creation"]["acct"] > 0
                    assert (mr["growth"]["growths"] > 0) == (impl == "MHVGM" and bool(pred["sizings"]["grow"]
                                                                                       ["growths"]))
                    if impl != "TC":                              # stable bytes: the model's, exactly
                        sz = pred["sizings"][C.SIZING[impl]]["stor_built"]
                        for x in C.SET_INPUTS[op]:
                            assert mr[f"built_{x}"]["acct"] == sz[x]["bcells"] + sz[x]["bvals"]
                    else:
                        assert mr["op"]["peak_acct"] == 16 * C.op_cells(op, n)
                p = run([BIN / "tc3gate_m2", w / f"job-{job:02d}.wl", t / "g3.out", t / "p3.bin"])
                assert p.returncode == 0 and K.readout_check(t / "g3.out", w / f"job-{job:02d}.ref") is None
                assert (t / "p3.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job)   # the TC gate
                for i in C.HASH_IMPLS:                                                         # call for call
                    assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job, i)
                assert not dumps["TC"]
                for x in C.SET_MATRICES[op]:                       # pack's (h) is fixed's table, slot for slot
                    assert dumps["MHVP"][f"hb{x}"] == dumps["MHVF"][f"tab{x}"]
                assert stor["MHVF"] == stor["MHVGM"] == stor["MHVP"]


def test_exact_checks_catch_a_difference():
    build_once()
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for op in ("mul", "sop"):
            w = toy_work(op)
            pred = json.loads((w / "job-06.m2.json").read_text())["predicted"]
            res = C.SET_RESULT[op]
            for impl in ("MHVGM", "MHVP", "MHVF"):
                lines, dump = gate(impl, op, w, 6, t)
                assert not K.exact_checks(impl, op, lines, pred, dump)
                sz = C.SIZING[impl]
                bad = json.loads(json.dumps(pred))
                bad["counters"][res]["mul" if op == "mul" else "add"]["cmp"] += 1
                assert any(f"{res} " in f for f in K.exact_checks(impl, op, lines, bad))
                bad = json.loads(json.dumps(pred))
                bad["sizings"][sz]["stor_end"][res]["probes"] += 1
                assert any(f"store {res} end probes" in f for f in K.exact_checks(impl, op, lines, bad))
                bad = json.loads(json.dumps(pred))
                bad["steps"][res]["cross_match"] += 1
                assert any("operation counters" in f for f in K.exact_checks(impl, op, lines, bad))
                bad = json.loads(json.dumps(pred))
                bad["sizings"][sz]["tables"]["B"] = "0" * 64
                assert any("B's table" in f for f in K.exact_checks(impl, op, lines, bad, dump))
                bad = json.loads(json.dumps(pred))
                bad["sizings"][sz]["cells"]["A"] = "0" * 64
                assert any("A's handle fields" in f for f in K.exact_checks(impl, op, lines, bad, dump))
                assert K.exact_checks(impl, op, [l for l in lines if not l.startswith("stor ")], pred, dump)
                if impl == "MHVGM":
                    bad = json.loads(json.dumps(pred))
                    bad["sizings"]["grow"]["growths"][0]["fields"] += 1
                    assert any("growths differ" in f for f in K.exact_checks(impl, op, lines, bad))
                if impl == "MHVP":
                    bad = json.loads(json.dumps(pred))
                    bad["sizings"]["pack"]["packs"][0]["whp"] = 16
                    assert any("packings differ" in f for f in K.exact_checks(impl, op, lines, bad))
                    bad = json.loads(json.dumps(pred))
                    bad["sizings"]["pack"]["hb"]["A"] = "0" * 64
                    assert any("build table (h)" in f for f in K.exact_checks(impl, op, lines, bad, dump))
                else:
                    assert any("build table dumped" in f for f in
                               K.exact_checks(impl, op, lines, pred, dict(dump, hbA=b"")))
            lines, _ = gate("TC", op, w, 6, t)                    # TC prints no store figures
            assert not K.exact_checks("TC", op, lines, pred) and K.exact_checks("TC", op, lines + ["stor end A W=2"], pred)
            # fixed's lines are not pack's
            lines, dump = gate("MHVF", op, w, 6, t)
            assert K.exact_checks("MHVP", op, lines, pred, dump)
        ref = (toy_work("sop") / "job-06.ref").read_bytes()       # a read-out that differs in one cell is caught
        broken = bytearray(ref)
        broken[8 * 5] ^= 1
        (t / "x.out").write_bytes(bytes(broken))
        r = K.readout_check(t / "x.out", ref)
        assert r and r["cell"] == 5 and r["mismatches"] == 1


def test_untimed_steps_toy():
    """the runner's untimed steps end to end on toy jobs of every set, with the real binaries: step 0 (workloads,
    references, predictions), step 1 (build, pins, tcm.c's diff, m1's sources, step 0's files), step 2 (unit,
    differential, allocation and sanitizer tests), step 4 (gates, the TC gate) and step 5 (counting and memory runs,
    the repeats on jobs 1 and 6, and the call-for-call checks). The build is made once (sop), then reused. Nothing
    timed runs."""
    build_once()
    for op in ("sop", "add", "mul", "pos"):
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            r = RN.Runner(ns(op=op, jobs="1,6,11"))
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
            assert set(r.state["gen"]) == {"1", "6", "11"} and r.state["gen"]["6"]["name"] == f"{op}-6"
            r.t0 = time.monotonic()
            r.step1()
            assert (r.out / "tcm.diff").read_text() == L.tcm_diff_text()
            if op == "sop":
                r.unit_tests()
                ut = r.state["unit_tests"]
                assert set(ut) == {"m2_unit", "m2_diff", "m2_alloc-timing", "m2_alloc-counting", "m2_alloc-memory",
                                   "m2_unit-asan", "m2_diff-asan"} and all(v["rc"] == 0 for v in ut.values()), ut
            r.step4_gates()
            assert len(r.state["gates"]) == 12 and r.state["measuring"]
            assert all(r.state["gates"][f"TC:{j}"]["tc3gate"]["percall"] == "equal" for j in (1, 6, 11))
            r.step5_counts()
            assert threading.active_count() == 1
            for job in ("1", "6", "11"):
                chk = r.state["call_checks"][job]
                assert chk["result"] == "pass", chk
                assert all(chk[i]["G_ratio"] == 1.0 for i in C.HASH_IMPLS)
            for impl in C.IMPLS:
                for job in (1, 6):
                    assert r.state["counts"][f"{impl}:{job}"]["repeat"] == "identical"
                    assert r.state["mems"][f"{impl}:{job}"]["repeat"] == "identical"
                assert "repeat" not in r.state["counts"][f"{impl}:11"]
            assert not r.state.get("bugs")
            (r.wl / "job-06.ref").write_bytes(b"x")              # a changed step-0 file is refused at step 1
            r.build = lambda: None
            try:
                r.step1()
            except RN.RunStop as e:
                assert "step 0's files changed" in str(e)
            else:
                raise AssertionError("a changed workload file was not refused")


def test_step0_against_m1():
    """rule-m2, Workloads: a measured add or mul run refuses step-0 files that differ from m1's recorded ones"""
    r = RN.Runner(ns(op="add", toy=False))
    r.state = {"gen": json.loads((C.M1_RESULTS["add"] / "state.json").read_text())["gen"]}
    assert r.step0_vs_m1() is None
    r.state["gen"]["3"]["sha256"]["job-03.ref"] = "0" * 64
    assert r.step0_vs_m1() == ["job-03.ref"]
    r = RN.Runner(ns(op="sop", toy=False))
    r.state = {"gen": {}}
    assert r.step0_vs_m1() is None
    r = RN.Runner(ns(op="mul", toy=True))
    r.state = {"gen": {"1": {"sha256": {"job-01.wl": "x", "job-01.ref": "y"}}}}
    assert r.step0_vs_m1() is None                         # toy runs: not compared


# ------------------------------------------------------------------------------------------------- statistics, pilot
def test_statistics_and_verdicts():
    a = [10.0] * 12
    r, lo, hi = A.boot_ratio(a, a)
    assert r == lo == hi == 1.0
    assert A.time_verdict(1.2, 1.3, 0.05) == "win" and A.time_verdict(0.7, 0.8, 0.05) == "loss"
    assert A.time_verdict(0.97, 1.2, 0.05) == "tie"
    assert A.fixed_verdict(0.90) == "win" and A.fixed_verdict(1.10) == "loss" and A.fixed_verdict(1.0) == "tie"
    assert C.SEVERE == 2.0 and C.BAND_MIN == 0.05 and C.BOOT == 10_000 and C.BOOT_SEED == 20261029
    # measure 1: the median of all samples; the rounds resampled with their samples together
    ref = [[20.0, 30.0]] * 12
    row = [[10.0, 15.0]] * 12
    assert A.boot_pooled(ref, row) == (2.0, 2.0, 2.0)
    ref = [[10.0 + k, 30.0 + k] for k in range(12)]
    row = [[10.0, 30.0]] * 12
    r, lo, hi = A.boot_pooled(ref, row)
    import statistics
    assert abs(r - statistics.median([x for p in ref for x in p]) / 20.0) < 1e-12 and lo <= r <= hi
    assert A.judged("against TC", "build") and A.judged("pack against fixed", "operation")
    assert not A.judged("pack against grow-map", "build") and not A.judged("against TC", "total")
    assert len(C.PAIRINGS) == 6 and sum(1 for p in C.PAIRINGS if p[2] == "against TC") == 3
    assert C.PAIRINGS[-1] == ("Hash-V-pack", "Hash-V-grow-map", C.REPORTED_KIND)
    assert C.OP_MEASURE == {"add": (2, "addition time"), "mul": (3, "multiplication time"),
                            "sop": (4, "sum-of-products time"), "pos": (5, "product-of-sums time")}
    assert C.PEAK_MEASURE == {"add": 7, "mul": 8, "sop": None, "pos": None}
    reps = [(0, 100, 50, 10, (20, 20)), (1, 90, 60, 5, (15, 10)), (2, 95, 40, 9, (10, 30))]
    assert K.kept_minimum(reps, 2) == (45.0, 20.0, 2.5, (5.0, 5.0))   # each quantity its own minimum of reps 2, 3
    assert K.kept_minimum(reps[:1], 1) == (100, 50, 10, (20, 20))


def test_pilot_projection_and_allowance():
    # rule-m2, Run plan step 3 (carried over): elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6)
    assert K.pilot_projection(90, 8, 9, 4, 12) == 90 + 6 * (4 * 9 + 12 * 8)
    assert C.PILOT_SCALE == 5.0 and C.PILOT_MARGIN == 1.2 and C.PILOT_JOBS == (1, 6, 13)
    assert C.PILOT_LIMIT_S == 30 * 60 and C.HARD_STOP_S == 40 * 60 and C.OPERATOR_TIMEOUT_MIN == 45
    assert C.ROUNDS == 12 and C.PILOT_ROUNDS == 2 and C.COUNT_REPEAT_JOBS == (1, 6) and C.COUNT_LIMIT_S == 600
    assert K.host_allowance(0.5, 10) == 5 + 10 and K.host_allowance(0.0, 10) == 5 and C.HOST_RATE_CEILING == 4.0
    assert K.other_limit(1.0) == 6 and K.other_limit(10.0) == 15
    # 14 launches per pilot block (4 calibrations, 2 x 5 rounds) and 64 per measured block (4 + 12 x 5)
    assert len(C.IMPLS) + C.PILOT_ROUNDS * len(C.ROWS) == 14 and len(C.IMPLS) + C.ROUNDS * len(C.ROWS) == 64


def pilot_runner(rates, P_extra=0.0, block_s=0.0, jobs=""):
    """a runner whose pilot blocks are stand-ins; rates: one host rate for every block, or a list of
    {host pid: (ticks, seconds)} per block, in block order; block_s: each stand-in block's wall time"""
    r = RN.Runner(ns(jobs=jobs))
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
        return [{"role": "calibration", "row": C.IMPL_ROW[i], "job": job, "r": 4, "reps": 3, "block": "pilot"}
                for i in C.IMPLS] + [{"role": "round", "row": row, "job": job, "total": 100.0, "block": "pilot"}
                                     for row in C.ROWS for _ in range(2)]
    r.timed_block = block
    r.seen = seen
    return r


def test_pilot_stops():
    r = pilot_runner(0.1)
    r.step3_pilot()                                       # goes on
    p = r.state["pilot"]
    assert r.seen == [1, 6, 13] and p["N_128"] == 4 and p["N_rest"] == 12
    assert p["P_6_s"] == p["walls_s"]["6"] and p["P_13_s"] == p["walls_s"]["13"]
    assert abs(p["projected_s"] - K.pilot_projection(p["elapsed_s"], p["P_6_s"], p["P_13_s"], 4, 12)) < 1e-6
    assert r.state["pilot_calibration"]["1:Hash-V-fixed-null"]["shared_from"] == "Hash-V-fixed"
    for rate in (1.25, 4.0):
        pilot_runner(rate).step3_pilot()
    for rate, extra, bs, why in ((4.5, 0, 0, "ceiling"), (0.1, 29.8 * 60, 0.2, "projected"),
                                 (0.1, 31 * 60, 0.0, "projected")):
        r = pilot_runner(rate, extra, bs)
        try:
            r.step3_pilot()
        except RN.RunStop as e:
            assert why in str(e), str(e)
            assert r.state["pilot_stopped"]
        else:
            raise AssertionError(f"the pilot did not stop the run ({why})")
    r = pilot_runner(0.1, jobs="2,3")                     # a toy run without jobs 1, 6, 13: its first job alone
    r.step3_pilot()
    assert r.seen == [2] and r.state["pilot"]["N_128"] == 0 and r.state["pilot"]["N_rest"] == 2


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
# round 3's and m1's tests, unchanged: rule-m2 item 5 carries the disturbance check over in full, and m2_runner
# uses m1_runner's code
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
# a stand-in for bench_m2 (harness test only): copies the reference as its read-out, prints made-up lines
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
            assert len(rnd) == 5 * C.ROUNDS and seen[-1]["spawns"] == 4 + 5 * C.ROUNDS == 64   # rule-m2 Run plan 6
            assert [x["row"] for x in rnd[:10]] == list(C.ROWS) + list(C.ROWS[1:]) + [C.ROWS[0]]  # rotated by one
            assert not any(x["row"] == C.NULL_ROW for x in cal)          # the null row takes Hash-V-fixed's r
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
TIMES = {"TC": (150.0, 20.0, 5.0), "Hash-V-fixed": (100.0, 30.0, 6.0), "Hash-V-fixed-null": (100.5, 30.1, 6.0),
         "Hash-V-grow-map": (110.0, 40.0, 6.0), "Hash-V-pack": (105.0, 33.0, 4.0)}


def analysis_state(op="add"):
    """a finished run's state for job 1 (n = 16): made-up times, and memory figures as the spec's accounting gives
    them (TC 16 bytes per cell; Hash-V-fixed 4-byte cells and a 64-bit table of 1024 slots: 36; Hash-V-pack 2-byte
    cells and a list of 282 entries)"""
    nin = len(C.SET_INPUTS[op])
    recs = []
    for k in range(1, 13):
        for row, (o, b, rd) in TIMES.items():
            builds = [b + 0.01 * k + i for i in range(nin)]
            recs.append({"job": 1, "row": row, "role": "round", "round": k, "operation": o + 0.1 * k, "read": rd,
                         "builds": builds, "total": o + 0.1 * k + rd + sum(builds), "cpu_share": 0.99, "cpu_ns": o,
                         "span_ns": o})
    q = 256
    stable = {"TC": 16 * q, "MHVF": 4 * q + 8192, "MHVGM": 4 * q + 4096, "MHVP": 2 * q + 8 * 282}
    peak = {"TC": 16, "MHVF": 40, "MHVGM": 30, "MHVP": 25}
    cells_op = C.op_cells(op, 16)

    def memrun(i):
        m = {f"built_{x}": {"acct": stable[i], "heap": stable[i] + 300, "peak_acct": stable[i] * (k + 1),
                            "base_acct": stable[i] * k, "peak_heap": 0, "base_heap": 0, "cells": q}
             for k, x in enumerate(C.SET_INPUTS[op])}
        m["after_creation"] = {"acct": stable[i] * nin, "heap": stable[i] * nin + 600, "cells": q * nin}
        m["op"] = {"peak_acct": peak[i] * cells_op, "peak_heap": peak[i] * cells_op + 1000, "cells": cells_op}
        m["end"] = {"acct": stable[i] * (nin + 1), "heap": 1, "cells": q * (nin + 1)}
        m["growth"] = {"peak_acct": 30 * cells_op if i == "MHVGM" else 0, "peak_heap": 1,
                       "growths": 6 if i == "MHVGM" else 0}
        return {"memrun": m}
    stor = {"end": {x: dict(W=282, T=1024, w=64, wh=16, whb=16, packed=0, nf=512, found=230, new=282, probes=600,
                            longest=5, widenings=0, copied=0, growths=0, reinserted=0, rprobes=0, rewritten=0,
                            bcells=1024, bvals=8192) for x in C.SET_MATRICES[op]}}
    counts = {f"{i}:1": {"counts": [f"ctr A entry core gcd_work 500"], "stor": stor,
                         "opc": {C.SET_RESULT[op]: dict(cross=1024, cross_match=40, cross_reads=900)},
                         "packs": [dict(name="A", W=282, whb=16, whp=16, bytes_list=2256, rewritten=512,
                                        bytes_map=2048)] if i == "MHVP" else []} for i in C.IMPLS}
    chk = {"G_TC": 500, "result": "pass", **{i: {"call_for_call": "pass", "G": 500, "G_ratio": 1.0,
                                                  "gcd_work_check": "passed"} for i in C.HASH_IMPLS},
           "pack against fixed": "pass", "grow-map against fixed": "pass"}
    cal = {f"1:{row}": {"r": 4, "reps": 1 if row == "Hash-V-grow-map" else 3} for row in C.ROWS}
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
          "pilot": {"jobs": [1, 6, 13], "walls_s": {"1": 6.0, "6": 7.0, "13": 9.0}, "P_6_s": 7.0, "P_13_s": 9.0,
                    "N_128": 4, "N_rest": 12, "projected_s": 900.0, "limit_s": 1800, "host_rates": {"1": 0.5}},
          "disturbance_checks": [{"job": 1, "label": "measured", "kind": "rounds", "attempt": 1, "seconds": 25.0,
                                  "disturbed": False, "reasons": [], "runner_ticks": 9, "spawns": 64,
                                  "forks_rise": 64, "forks_unattributed": 0, "hosts": {}, "other_limit": 30}]}
    return st


def test_analysis():
    """a finished state gives, per pairing (six), per-measure verdicts: measure 1 pooled over the inputs and the
    operation measure, each with its own band from the null row; total and read-out reported; the stable bytes and,
    in add and mul, the peak with verdicts (sop and pos: the peak reported); the reported pairing with no verdict; the
    gcd-work and call-for-call checks; strata and grid; a stopped run gives no verdict"""
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
            sec = md[md.find("### Hash-V-fixed against TC"):].split("\n\n")[1]
            nin = len(C.SET_INPUTS[op])
            # measure 1: medians of all samples: TC's builds 20 + i + k/100, Hash-V-fixed's 30 + i + k/100
            b_tc = statistics.median([20.0 + 0.01 * k + i for k in range(1, 13) for i in range(nin)])
            b_vf = statistics.median([30.0 + 0.01 * k + i for k in range(1, 13) for i in range(nin)])
            assert f"| 1 (n 16, rho 1) | loss, {b_tc / b_vf:.3f}" in sec, (sec[:600], b_tc / b_vf)
            # the operation: (150 + k/10) / (100 + k/10), a win; stable bytes (1024 + 8192) / 4096 = 2.25, severe
            assert "| win, 1.49" in sec and "loss, 2.250 (heap 2.165) **severe memory loss**" in sec, sec
            if C.PEAK_MEASURE[op]:
                assert "loss, 2.500 (heap" in sec                          # the peak, 40 / 16, with a verdict
            else:
                assert "reported, 2.500 (heap" in sec                      # sop, pos: reported, no verdict
            sec = md[md.find("### Hash-V-grow-map against TC"):].split("\n\n")[1]
            assert "(c)" in sec                                            # grow-map ran 1 rep per invocation
            sec = md[md.find("### Hash-V-pack against Hash-V-grow-map"):].split("\n\n")[1]
            assert "win" not in sec and "loss" not in sec and "tie" not in sec and "severe" not in sec
            assert sec.count("reported") >= 6
            sec = md[md.find("### Hash-V-pack against Hash-V-fixed"):].split("\n\n")[1]
            assert "win, 0.300 (heap" in sec                               # (512 + 2256) / 9216 stable bytes
            assert "1.000000 (passed), pass" in md and "| pass | pass |" in md
            assert "| NONE | Hash-V-fixed vs TC | stable | 0 | 0 | 1 | 0 | 0 | 0 | 2.250 | 1 |" in md
            assert ("| NONE | Hash-V-fixed vs TC | peak |" in md) == bool(C.PEAK_MEASURE[op])
            assert "| NONE | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 1 |" in md
            assert "| LOW | Hash-V-fixed vs TC | build | 0 | 0 | 0 | 0 | 0 | 0 | n/a | 0 |" in md
            grid = md[md.find("Hash-V-fixed against TC, 1 build time"):]
            assert f"| 16 | {b_tc / b_vf:.3f} | n/a | n/a | n/a |" in grid.split("\n\n")[1]
            assert "| 1 | Hash-V-grow-map |" in md and "6 growths" in md
            assert "| 1 | A | 282 | 16 | 16 | 2256 | 512 | 2048 |" in md
            assert "results-m1-add/results.md" in md and "| no | 9 | 64 |" in md
            v = list(csvrows(t / "verdicts.csv"))
            got = {(x["pairing"], x["measure"], x["verdict"]) for x in v}
            assert {("Hash-V-fixed vs TC", "build", "loss"), ("Hash-V-fixed vs TC", "total", "reported"),
                    ("Hash-V-pack vs Hash-V-grow-map", "operation", "reported"),
                    ("Hash-V-pack vs Hash-V-grow-map", "stable bytes", "reported"),
                    ("Hash-V-pack vs Hash-V-fixed", "stable bytes", "win"),
                    ("Hash-V-grow-map vs TC", "gcd work (check)", "passed")} <= got, got
            assert len(list(csvrows(t / "rounds.csv"))) == 5 * 12
        # one band per measure: a noisy null on the operation widens h for the operation only
        st = analysis_state("sop")
        for x in st["records"]:
            if x["row"] == C.NULL_ROW:
                x["operation"] *= 1.3 if x["round"] % 2 else 0.8
        d = A.Data(st)
        hb, _ = A.null_h(d, 1, "build")
        ho, _ = A.null_h(d, 1, "operation")
        assert hb == C.BAND_MIN and ho > 0.1, (hb, ho)
        st2 = analysis_state()                                # over cap: both time measures of every pairing with it
        st2["calibration"]["1:TC"]["overcap"] = True
        d = A.Data(st2)
        assert A.time_measure(d, 1, "Hash-V-fixed", "TC", "operation", "against TC")["verdict"] == "over cap"
        assert A.time_measure(d, 1, "Hash-V-fixed", "TC", "build", "against TC")["verdict"] == "over cap"
        assert A.time_measure(d, 1, "Hash-V-fixed", "TC", "total", "against TC")["verdict"] == "reported"
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
    harness test: nothing here is a measurement."""
    py = [sys.executable, str(C.HERE / "m2_runner.py")]
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
    """every gate and counting run on the full-size workloads of every set (untimed; the model takes minutes at
    n = 128)"""
    build_once()
    for op in C.OPS:
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            w = t / "work"
            m2_gen.generate(w, op, list(C.JOBS))
            for job in C.JOBS:
                pred = json.loads((w / f"job-{job:02d}.m2.json").read_text())["predicted"]
                for impl in C.IMPLS:
                    lines, dump = gate(impl, op, w, job, t)
                    f = K.exact_checks(impl, op, lines, pred, dump)
                    assert not f, (op, job, impl, f[:5])
                    p = run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                             t / f"p-{impl}.bin"])
                    assert p.returncode == 0 and not K.exact_checks(impl, op, p.stdout.splitlines(), pred)
                p = run([BIN / "tc3gate_m2", w / f"job-{job:02d}.wl", t / "g3.out", t / "p3.bin"])
                assert p.returncode == 0 and (t / "p3.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
                for i in C.HASH_IMPLS:
                    assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
                print(f"  full size: {op}-{job} gates pass for the four rows, the TC gate, call for call", flush=True)


TESTS = [test_rule_fixed_and_pinned, test_pins_and_m1, test_forbidden_places, test_runner_imports_no_numpy,
         test_runner_arguments, test_workloads, test_model_worked_by_hand, test_arithmetic_unchanged,
         test_hash_rows_one_source, test_no_reserved_slot_no_handle_test, test_only_named_structures, test_c_suites,
         test_gates_and_counts_toy, test_exact_checks_catch_a_difference, test_untimed_steps_toy, test_step0_against_m1,
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
