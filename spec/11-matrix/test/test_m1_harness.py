"""Tests of the matrix round's harness (not measurements; nothing here is timed or reported as a time). From round 3's
test_r3_harness.py (../../10-interning/test/, unchanged there), with every test that applies carried over and adapted
to this round's seven rows and two operations, and new ones.

    python3 test_m1_harness.py            the self-tests: the fixed rule and its pin; the 08 module, tc3.c, tcm.c's
                                          recorded diff and round 3's results; the places the harness may not write;
                                          the workloads of both job sets; the model worked by hand; tcm.c against
                                          tc3.c (only the operand and result interface changed); the five hash rows
                                          from one source; the allocations (only the named structures); the binaries'
                                          exact checks on toy workloads of both operations (every row, every table,
                                          handle field and tau, the TC gate, call for call, V/M, grow/fixed,
                                          map/lookup); the runner's untimed steps; the multiplication run's guard;
                                          statistics; the pilot's projection and stops; fork/exec with the affinity
                                          set before exec; the kernel timer; the idle check; the disturbance check;
                                          the timed block's machinery with a stand-in driver (seven rows, rotated;
                                          three parts and tau); the runner's ticks per launch; the analysis (nine
                                          pairings, map against lookup on creation only, the bands, memory, the growth
                                          peak, tau, strata and grid); and a toy run of each operation end to end
                                          (--toy, with --pilot-only then --resume)
    python3 test_m1_harness.py --full     also every gate and counting run on the full-size workloads of both job
                                          sets (test and counting builds; untimed)
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

import m1_analyze as A
import m1_checks as K
import m1_common as C
import m1_gen
import m1_libconf as L
import m1_model
import m1_runner as RN

TB = C.HERE / "build" / "m1test"
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
    if not (w / "gen.json").exists():
        m1_gen.generate(w, op, list(C.JOBS), toy=True)
    return w


def run(args, **kw):
    return subprocess.run([str(a) for a in args], capture_output=True, text=True, **kw)


# ------------------------------------------------------------------------------------------------- the rule, pins
def test_rule_fixed_and_pinned():
    """the rule on disk is the pinned, approved one (no PROPOSED mark); another text, or one with a PROPOSED mark, is
    refused for a measured run"""
    text = C.RULE_FILE.read_text()
    assert C.RULE_SHA256 == "8e951ac6597f099962aa77580653e799d101af6d7b3e30a51e1a6005ab7aeff7"
    assert C.sha256(C.RULE_FILE) == C.RULE_SHA256, "rule-m1.md differs from the pinned hash"
    assert C.PROPOSAL_MARK not in text and C.rule_check() is None, C.rule_check()
    assert "**Fixed: approved by the author, Oct 29, 2026**" in text
    assert "**Gcd work: a check, not a verdict.**" in text
    r = run(["git", "log", "--format=%h", "-1", "--", str(C.RULE_FILE)], cwd=C.HERE)
    assert r.stdout.strip() == "5ee4eaa", r.stdout                 # committed once, unchanged since
    with tempfile.TemporaryDirectory() as t:
        p = Path(t) / "rule-m1.md"
        p.write_text(text + " ")
        assert "sha256" in C.rule_check(p)
        p.write_text(text.replace("approved by the author, Oct 29, 2026", "PROPOSED", 1))
        assert "PROPOSED" in C.rule_check(p)
    for pin in ("0" * 64, None):
        saved, savedf = C.RULE_SHA256, C.RULE_FILE
        try:
            if pin:
                C.RULE_SHA256 = pin
            else:                                          # the approved text with a PROPOSED mark put back
                tf = Path(tempfile.mkdtemp()) / "rule-m1.md"
                tf.write_text(text.replace("approved by the author, Oct 29, 2026", "PROPOSED", 1))
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


def test_pins_and_round3():
    """the 08 module, tc3.c and its headers (here and in round 3's tree), tcm.c's recorded diff and round 3's results
    are as pinned; a change to any of them is caught"""
    assert L.vendor08_check() is None
    sums = dict(reversed(l.split()) for l in (L.V08 / "SHA256SUMS").read_text().splitlines())
    assert sums == L.VENDOR08_SHA256
    assert L.tc3_check() is None, L.tc3_check()
    for name in L.ROUND3_SHA256:                         # copied unchanged from round 3
        assert (L.LIBDIR / name).read_bytes() == (C.ROUND3 / "lib" / name).read_bytes(), name
    for f in L.VENDOR08_SHA256:
        assert (L.V08 / f).read_bytes() == (C.ROUND3 / "lib" / "vendor08" / f).read_bytes(), f
    assert L.tcm_diff_check() is None, L.tcm_diff_check()
    assert C.round3_check() is None, C.round3_check()
    with tempfile.TemporaryDirectory() as t:
        fake = Path(t)
        shutil.copytree(L.LIBDIR / "src", fake / "src")
        shutil.copytree(L.LIBDIR / "include", fake / "include")
        saved = (L.LIBDIR, L.SRC, L.TCM_DIFF)
        try:
            L.LIBDIR, L.SRC, L.TCM_DIFF = fake, fake / "src", fake / "src" / "tcm.diff"
            (fake / "src" / "tc3.c").write_bytes((saved[1] / "tc3.c").read_bytes() + b"\n")
            assert "tc3.c" in L.tc3_check()
            (fake / "src" / "tc3.c").write_bytes((saved[1] / "tc3.c").read_bytes())
            assert L.tc3_check() is None and L.tcm_diff_check() is None
            src = (fake / "src" / "tcm.c").read_text()
            (fake / "src" / "tcm.c").write_text(src.replace("h = gcdp(c, t, d);", "h = gcdp(c, d, t);", 1))
            assert "differs from the recorded" in L.tcm_diff_check()
            (fake / "src" / "tcm.diff").unlink()
            assert "missing" in L.tcm_diff_check()
        finally:
            L.LIBDIR, L.SRC, L.TCM_DIFF = saved
    saved = dict(C.ROUND3_RESULTS_SHA256)
    try:
        C.ROUND3_RESULTS_SHA256["results.md"] = "0" * 64
        assert "results.md" in C.round3_check()
    finally:
        C.ROUND3_RESULTS_SHA256.clear()
        C.ROUND3_RESULTS_SHA256.update(saved)
    assert sorted(p.name for p in C.ROUND3_RESULTS.iterdir()) == sorted(C.ROUND3_RESULTS_SHA256)


def test_forbidden_places():
    """nothing is written anywhere in ../10-interning/ (runner, generator, analysis); the multiplication run writes
    nowhere in the addition run's folder; the multiplication run (measured) refuses unless the addition run finished"""
    for p in (C.ROUND3, C.ROUND3 / "test" / "results-r3", C.ROUND3 / "test" / "build" / "x"):
        assert C.forbidden(p) and C.forbidden(p, "mul")
        r = RN.Runner(ns(toy=True))
        r.out = p
        try:
            r.preflight()
        except SystemExit as e:
            assert "10-interning" in str(e)
        else:
            raise AssertionError(f"the runner would write into {p}")
        for f in (lambda: A.report(p), lambda: m1_gen.generate(p, "add", [1], toy=True)):
            try:
                f()
            except SystemExit as e:
                assert "10-interning" in str(e)
            else:
                raise AssertionError(f"would write into {p}")
    a = C.RESULTS["add"]
    assert C.forbidden(a, "mul") and C.forbidden(a / "x", "mul") and not C.forbidden(a, "add")
    assert not C.forbidden(C.RESULTS["mul"], "mul") and C.RESULTS["mul"].name == "results-m1-mul"
    r = RN.Runner(ns(op="mul", toy=True))
    r.out = a
    try:
        r.preflight()
    except SystemExit as e:
        assert "addition run's folder" in str(e)
    else:
        raise AssertionError("the multiplication run would write into the addition run's folder")
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        assert "no addition run's state" in RN.addition_finished(t)
        st = {"op": "add", "toy": False, "finished": "x"}
        for bad, why in (({"toy": True}, "not a measured"), ({"op": "mul"}, "not a measured"),
                         ({"finished": None}, "did not finish"), ({"run_stopped": {"step": "4"}}, "did not finish"),
                         ({"hard_stopped": {"step": "6"}}, "did not finish")):
            (t / "state.json").write_text(json.dumps(dict(st, **bad)))
            assert why in RN.addition_finished(t), bad
        (t / "state.json").write_text(json.dumps(st))
        assert RN.addition_finished(t) is None
        saved = dict(C.RESULTS)
        try:
            C.RESULTS["add"] = t / "none"
            try:
                RN.Runner(ns(op="mul", toy=False)).preflight()
            except SystemExit as e:
                assert "refusing the multiplication run" in str(e), str(e)
            else:
                raise AssertionError("the multiplication run did not refuse without the addition run")
        finally:
            C.RESULTS.update(saved)


def test_runner_imports_no_numpy():
    r = run([sys.executable, "-c", "import sys, m1_runner; print('numpy' in sys.modules)"], cwd=C.HERE)
    assert r.stdout.strip() == "False", r.stdout + r.stderr


def test_runner_arguments():
    """--op is required (add or mul); --hard-stop-s and --jobs are for toy runs; --resume is refused after the hard
    stop or the pilot's stop, once a timed round has run, and on the other operation's state"""
    r = run([sys.executable, str(C.HERE / "m1_runner.py"), "--toy"])
    assert r.returncode != 0 and "--op" in r.stderr
    r = run([sys.executable, str(C.HERE / "m1_runner.py"), "--op", "div", "--toy"])
    assert r.returncode != 0 and "invalid choice" in r.stderr
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
                           ({"op": "mul"}, "of the mul run"),
                           ({"op": "add", "records": [{"job": 1}]}, "--resume is refused")):
            r = RN.Runner(ns(resume=True))
            r.out, r.tmp, r.state_file = t / "out", t / "inv", t / "out" / "state.json"
            r.out.mkdir(exist_ok=True)
            r.state_file.write_text(json.dumps(dict(state, elapsed_saved=0)))
            r.log = lambda m: None
            import m1_analyze
            saved = m1_analyze.report
            m1_analyze.report = lambda out: None
            try:
                r.start()
            except SystemExit as e:
                assert why in str(e), (why, str(e))
            else:
                raise AssertionError(f"--resume was not refused ({why})")
            finally:
                m1_analyze.report = saved
                r.disarm()


# ------------------------------------------------------------------------------------------------- the workloads
def test_workloads():
    """rule-m1, Workloads: the sweep n x rho, 16 jobs per set; job j seeded 20261029 + j (addition) or 20261029 + 100 + j
    (multiplication), A's values then B's from one stream; numerators in [-32767, 32767], denominators among the 30
    divisors of 720; each matrix exactly s = n^2 / rho distinct values, at least 4; A and B separate pools; the same
    sizes and sharing in both sets; deterministic"""
    from fractions import Fraction
    table = {1: (16, 1, 256), 2: (16, 4, 64), 3: (16, 16, 16), 4: (16, 64, 4), 5: (32, 1, 1024), 6: (32, 4, 256),
             7: (32, 16, 64), 8: (32, 64, 16), 9: (64, 1, 4096), 10: (64, 4, 1024), 11: (64, 16, 256),
             12: (64, 64, 64), 13: (128, 1, 16384), 14: (128, 4, 4096), 15: (128, 16, 1024), 16: (128, 64, 256)}
    assert m1_gen.DIVS720 == tuple(d for d in range(1, 721) if 720 % d == 0) and len(m1_gen.DIVS720) == 30
    for op in C.OPS:
        for job, (n, rho, s) in table.items():
            assert (C.job_n(job), C.job_rho(job), C.job_s(job)) == (n, rho, s)
            assert C.job_seed(op, job) == 20261029 + job + (100 if op == "mul" else 0)
            if n > 32:
                continue                                  # the large jobs: drawn in --full and in step 0
            n2, s2, A_, B_ = m1_gen.workload(op, job)
            assert (n2, s2) == (n, s) and len(A_) == len(B_) == n * n
            for X in (A_, B_):
                assert all(-32767 <= m <= 32767 and d in m1_gen.DIVS720 for m, d in X)
                assert len({Fraction(m, d) for m, d in X}) == s >= 4
            assert A_ != B_
            assert m1_gen.workload(op, job) == (n2, s2, A_, B_)
        assert m1_gen.workload("add", 1)[2] != m1_gen.workload("mul", 1)[2]
    assert C.stratum(1) == "NONE" and C.stratum(6) == "LOW" and C.stratum(11) == "MID" and C.stratum(16) == "HIGH"
    with tempfile.TemporaryDirectory() as t:
        s = m1_gen.generate(t, "add", [1, 4], toy=False)
        assert s["4"]["realized"]["distinct_A"] == 4 and s["1"]["nops"] == 256
        h = {k: v["sha256"] for k, v in s.items()}
        s2 = m1_gen.generate(Path(t) / "again", "add", [1, 4], toy=False)
        assert {k: v["sha256"] for k, v in s2.items()} == h       # deterministic, byte for byte
        op, n, job, A_, B_ = C.read_workload(Path(t) / "job-04.wl")
        assert (op, n, job) == ("add", 16, 4) and (A_, B_) == tuple(m1_gen.workload("add", 4)[2:])


def test_model_worked_by_hand():
    """the hash of spec section 5.3; T_fix and w_h of section 5.2's table; 0 and 1 at slots 0 and 1, found with no
    probe; grow's trigger W + 1 > T/2 from T0 = 16 with every whole number reinserted and every field written so far
    rewritten (lookup and map giving the same handle); tau's none; a small addition and multiplication"""
    phi = 0x9E3779B97F4A7C15
    assert C.home(1, 4) == phi >> 60 and C.home(0, 10) == 0 and C.home(2, 15) == ((2 * phi) % 2 ** 64) >> 49
    for n, K_, tfix, wh in ((16, 514, 1024, 16), (32, 2050, 4096, 16), (64, 8194, 16384, 16), (128, 32770, 65536, 32)):
        assert 2 * n * n + 2 == K_ and 1 << m1_model.t_fix_bits(n, n) == tfix
        assert m1_model.handle_width(n, n, False) == m1_model.handle_width(n, n, True) == wh
    assert m1_model.handle_width(7, 7, False) == 8 and m1_model.handle_width(8, 8, False) == 16
    assert m1_model.handle_width(5, 5, True) == 8 and m1_model.handle_width(6, 6, True) == 16
    S = m1_model.Store("A", 4, 4, True)
    assert (S.T, S.W, S.tab[0], S.tab[1], S.w) == (16, 2, 0, 1, 64)
    S.store_cell(0, 1)
    assert S.c["found"] == 2 and S.c["probes"] == 0 and S.fields == [0, 2]
    for k in range(2, 8):                                   # W = 8 after 6 new whole numbers: 2/1 .. 7/1
        S.store_cell(k, 1)
    assert S.W == 8 and S.T == 16 and S.c["growths"] == 0
    S.store_cell(-1, 9)                                     # the numerator 1: found; the denominator 9: a growth
    g = S.growth_log[0]
    assert S.T == 32 and g["W"] == 8 and g["T_old"] == 16 and g["fields"] == 15 and g["rewritten"] == 15
    assert g["reinserted"] == 6 and S.fields[14] == 2 * 1 + 1      # the numerator's field, written before the growth
    assert [S.value(i) for i in range(8)] == [(0, 1)] + [(k, 1) for k in range(2, 8)] + [(-1, 9)]
    C_ = m1_model.Store("C", 4, 4, False)
    C_.store_cell(1000, 1)
    assert C_.w == 16 and C_.c["widenings"] == 1 and C_.c["copied"] == C_.T
    C_.store_cell(1 << 40, 3)
    assert C_.w == 64 and C_.c["widenings"] == 2
    B_ = m1_model.Store("B", 4, 4, False)
    B_.store_cell(9, 11)
    A_ = m1_model.Store("A", 4, 4, False)
    A_.store_cell(9, 7)
    tau, cnt = m1_model.build_tau(A_, B_)
    none = (1 << A_.wh) - 1
    assert tau[0] == 0 and tau[1] == 1 and tau[B_.fields[0] >> 1] == A_.fields[0] >> 1 and \
        tau[B_.fields[1] >> 1] == none
    assert cnt["tau_found"] == 1 and cnt["tau_none"] == 1 and tau.count(none) == B_.T - 3
    # 1/2 + -1/2 (zero), 2/3 + 1/3 (den, h = 3); a cell of B whose 3 A lacks
    p = m1_model.predict("add", 1, [(1, 2)], [(-1, 2)])
    assert p["values"] == [(0, 1)] and p["ncase"] == {"add_zero": 1} and p["cross_match"] == 2
    p = m1_model.predict("add", 1, [(2, 3)], [(1, 3)])
    assert p["values"] == [(1, 1)] and p["ncase"] == {"add_den": 1} and p["counters"]["add"]["gcd"] == 1
    assert p["sizings"]["fixed"]["tau_counts"] == {"tau_found": 1, "tau_none": 0, "tau_probes": 1}
    # 3/2 x 4/3: a = d, so cross_ad, gcd(c, b) = 2: 2/1; then 0/1 + 2/1: zero_x (every cell's first addition)
    p = m1_model.predict("mul", 1, [(3, 2)], [(4, 3)])
    assert p["values"] == [(2, 1)] and p["ncase"] == {"mul_cross_ad": 1, "add_zero_x": 1} and p["nops"] == 2
    # [[1/2, 2], [0, 1]] [[3, 0], [1/4, 1]] = [[3/2 + 1/2, 0 + 2], [0 + 1/4, 0 + 1]]: products cross_ad (1/2 x 3),
    # cross_cb (2 x 1/4), zero (three), one_y (2 x 1), one_x (two); sums zero_x but 3/2 + 1/2 (den)
    p = m1_model.predict("mul", 2, [(1, 2), (2, 1), (0, 1), (1, 1)], [(3, 1), (0, 1), (1, 4), (1, 1)])
    assert p["values"] == [(2, 1), (2, 1), (1, 4), (1, 1)]
    assert p["ncase"] == {"mul_cross_ad": 1, "mul_cross_cb": 1, "mul_zero": 3, "mul_one_y": 1, "mul_one_x": 2,
                          "add_zero_x": 7, "add_den": 1}, p["ncase"]


# ------------------------------------------------------------------------------------------------- the sources
def c_function(src, name, prefix):
    """the text of a C function (by its name, with or without the row's prefix) from its line to its closing brace"""
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


def test_tcm_changes_only_the_interface():
    """spec section 4: tcm.c is tc3.c with its arithmetic function bodies unchanged: the comparisons, the cases and
    their order, the gcds, the exact divisions, multiplications and additions, the skips and the overflow steps, line
    for line; only the signatures (pairs in, a destination out), operand(), store(), begin() (its handle check replaced
    by the test build's assertion of valid operands) and the entry's destination change"""
    t3, tm = (L.SRC / "tc3.c").read_text(), (L.SRC / "tcm.c").read_text()
    for name in ("addsub", "mul", "div", "compare", "gcdp"):
        a, b = c_function(t3, name, "tc_"), c_function(tm, name, "tcm_")
        assert a.split("\n", 1)[1] == b.split("\n", 1)[1], name           # the body, line for line
    e3, em = c_function(t3, "enter", "tc_").split("\n"), c_function(tm, "enter", "tcm_").split("\n")
    assert [l for l in e3[1:] if l not in em] == [], "the entry's arithmetic changed"
    b = c_function(tm, "begin", "tcm_")
    assert "x >= c->N" not in b and "SK_INT_MIN" in b and "d < 1" in b and "sk_gcd_raw" in b and "sk_die" in b
    assert "#ifdef SK_AUDIT" in b
    st = c_function(tm, "store", "tcm_")
    assert "out->m = n;" in st and "out->d = e;" in st and "SK_RESERVE" not in st
    assert L.tcm_diff_check() is None


def test_hash_rows_one_source():
    """spec section 3: the five hash rows are one source, mh.c, built with only -D settings (V or M, fixed or grow,
    lookup or map); the map rewrite is built on Hash-V-grow only; the counts of structures are 6, 7, 6, 7, 7"""
    assert C.HASH_FLAGS == {"MHVF": (0, 0, 0), "MHMF": (1, 0, 0), "MHVG": (0, 1, 0), "MHMG": (1, 1, 0),
                            "MHVGM": (0, 1, 1)}
    for i in C.HASH_IMPLS:
        src, defs = L.OBJECTS[i.lower()]
        assert src == L.SRC / "mh.c" and defs == L.hash_defs(i)
        assert L.IMPL_DEFS[i] == ["-DIMPL_HASH"] + defs and L.ROW_OBJECTS[i] == (i.lower(), "common", "m1common")
    assert L.OBJECTS["tcm"][0] == L.SRC / "tcm.c" and L.ROW_OBJECTS["TC"] == ("tcm", "common", "m1common")
    for m, g, mp, ok in ((0, 0, 0, True), (1, 1, 0, True), (0, 1, 1, True), (1, 1, 1, False), (0, 0, 1, False),
                         (1, 0, 1, False)):
        r = run(["gcc", "-std=c11", "-fsyntax-only", f"-DMH_M={m}", f"-DMH_GROW={g}", f"-DMH_MAP={mp}"] + L.inc() +
                [L.SRC / "mh.c"])
        assert (r.returncode == 0) == ok, (m, g, mp, r.stderr[-300:])
    r = run(["gcc", "-std=c11", "-fsyntax-only", "-DSK_PROFILE"] + L.inc() + [L.SRC / "tcm.c"])
    assert r.returncode != 0 and "no profiling build" in r.stderr
    assert C.ROWS == ("TC", "Hash-V-fixed", "Hash-M-fixed", "Hash-V-grow", "Hash-M-grow", "Hash-V-grow-map",
                      "Hash-V-fixed-null") and C.ROW_IMPL["Hash-V-fixed-null"] == "MHVF"


def test_only_named_structures():
    """spec section 3, "Nothing else exists": by the code, outside the test build's audits, the only allocations are
    each matrix's header, the cells (a) and the table (b) (TC: its pairs), the widened table (w), and, under their own
    settings only, the slot map (r) (MH_MAP) and tau (t) (MH_M); the grown table (n) is made by the table's own
    constructor inside the growth (MH_GROW). By the allocations: the allocation check binaries (timing and counting
    builds), whose limits are the counts 3, 6, 7, 6, 7, 7 with the replacement tables."""
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
    mh = (L.SRC / "mh.c").read_text()
    assert sites(strip_audit(mh)) == sorted([
        "sk_xcalloc(1, sizeof *x)",                              # the header
        "sk_xmalloc((size_t)2 * R * K * (x->wh / 8))",           # (a) the cells
        "sk_xmalloc((size_t)T * (w / 8))",                       # (b), and (n) in a growth: table_new
        "sk_xmalloc((size_t)x->T * (w2 / 8))",                   # (w) the widened table
        "sk_xmalloc((size_t)T * (x->wh / 8))",                   # (r) the slot map
        "sk_xmalloc((size_t)b->T * (a->wh / 8))"]), sites(strip_audit(mh))    # (t) tau
    assert guards(mh, "void *r = sk_xmalloc") == ["#if MH_GROW", "#if MH_MAP"]
    assert guards(mh, "c->tau = sk_xmalloc") == ["#if MH_M"]
    assert guards(mh, "void *nt = table_new(T2") == ["#if MH_GROW"]
    assert mh.count("table_new(") == 3                       # its definition, create's (b), grow's (n)
    tm = (L.SRC / "tcm.c").read_text()
    assert sites(strip_audit(tm)) == sorted(["sk_xcalloc(1, sizeof *c)", "sk_xmalloc(sizeof(tpair) * (size_t)R * K)",
                                             "sk_xcalloc(1, sizeof *c->pf)"])
    assert guards(tm, "c->pf = sk_xcalloc") == ["#ifdef SK_PROFILE"]          # never built (m1.h refuses it)
    build_once()
    for b in L.ALLOC_BUILDS:
        p = run([BIN / f"m1_alloc-{b}"])
        assert p.returncode == 0 and " 0 failures" in p.stdout, p.stdout + p.stderr
        for row, lim in (("TC", 6), ("Hash-V-fixed", 10), ("Hash-M-fixed", 11), ("Hash-V-grow", 10),
                         ("Hash-M-grow", 11), ("Hash-V-grow-map", 11)):
            assert f"  {row}: at most " in p.stdout and f"(limit {lim})" in p.stdout.split(f"  {row}: ")[1].split("\n")[0]


def test_c_suites():
    """the unit, differential, allocation and sanitizer suites of rule-m1 Run plan step 2 (the runner's own commands)"""
    build_once()
    for name, cmd, env in L.unit_commands(BIN):
        p = run(cmd, env=dict(os.environ, **env) if env else None)
        assert p.returncode == 0 and " 0 failures" in p.stdout, (name, p.stdout[-500:] + p.stderr[-500:])
        if name == "m1_unit":
            for what in ("growth triggered by a denominator's store: in A, in B, in C",
                         "a growth and a widening in one store of C", "widening: 8 -> 16 -> 32 -> 64",
                         "tau: all matching, no match, 0 and 1", "w_h at the thresholds of both rules",
                         "overflow: 2^63, -2^63", "probing: home slots 0 and 1 passed over; a probe that wraps"):
                assert what in p.stdout, what


# ------------------------------------------------------------------------------------------------- the binaries
def gate(impl, w, job, t):
    p = run([BIN / f"bench-{impl}-test", "gate", w / f"job-{job:02d}.wl", t / "g.out", t / f"gp-{impl}.bin",
             t / f"gd-{impl}.bin"])
    assert p.returncode == 0, p.stdout[-500:] + p.stderr
    assert K.readout_check(t / "g.out", w / f"job-{job:02d}.ref") is None
    return p.stdout.splitlines(), (C.read_dump(t / f"gd-{impl}.bin") if (t / f"gd-{impl}.bin").exists() else None)


def test_gates_and_counts_toy():
    """every toy job of both sets: each row's gate (test build) passes its exact checks against the model, every
    table slot by slot, every handle field and tau position by position; the TC gate: tc3.c's per-operation record
    equal to tcm.c's; the counting runs pass the same checks, and every hash row's per-operation record equals TC's;
    V and M have the same tables and fields, grow the same found/new and W as fixed, map the same tables and fields as
    lookup; the memory runs read out the reference and report after creation, at the end and the growth peak"""
    build_once()
    for op in C.OPS:
        w = toy_work(op)
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            for job in C.JOBS:
                pred = json.loads((w / f"job-{job:02d}.m1.json").read_text())["predicted"]
                dumps, stor = {}, {}
                for impl in C.IMPLS:
                    lines, dump = gate(impl, w, job, t)
                    fails = K.exact_checks(impl, lines, pred, dump)
                    assert not fails, (op, job, impl, fails[:5])
                    dumps[impl] = dump
                    ngrow = len(K.parse_growths(lines))
                    assert (ngrow > 0) <= (impl in C.GROW_IMPLS)
                    p = run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                             t / f"p-{impl}.bin"])
                    assert p.returncode == 0 and not K.exact_checks(impl, p.stdout.splitlines(), pred)
                    assert K.readout_check(t / "c.out", w / f"job-{job:02d}.ref") is None
                    assert (t / f"p-{impl}.bin").read_bytes() == (t / f"gp-{impl}.bin").read_bytes()
                    stor[impl] = K.store_split(p.stdout.splitlines())
                    p = run([BIN / f"bench-{impl}-timing", "mem", w / f"job-{job:02d}.wl", t / "m.out"])
                    mr = K.parse_memrun(p.stdout.splitlines())
                    assert p.returncode == 0 and {"after_creation", "end", "peak"} <= set(mr), p.stdout
                    assert K.readout_check(t / "m.out", w / f"job-{job:02d}.ref") is None
                    assert mr["peak"]["growths"] == ngrow                  # the hook, inside every growth
                    assert (mr["peak"]["used"] > 0 and mr["peak"]["heap"] > 0) == (ngrow > 0)
                    assert mr["end"]["used"] >= mr["after_creation"]["used"] > 0
                p = run([BIN / "tc3gate", w / f"job-{job:02d}.wl", t / "g3.out", t / "p3.bin"])
                assert p.returncode == 0 and K.readout_check(t / "g3.out", w / f"job-{job:02d}.ref") is None
                assert (t / "p3.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job)   # the TC gate
                for i in C.HASH_IMPLS:                                                         # call for call
                    assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes(), (op, job, i)
                assert not dumps["TC"] and "tau" in dumps["MHMF"] and "tau" not in dumps["MHVF"]
                for v, m in (("MHVF", "MHMF"), ("MHVG", "MHMG")):
                    assert dumps[v] == {k: x for k, x in dumps[m].items() if k != "tau"}
                assert dumps["MHVGM"] == dumps["MHVG"]
                assert stor["MHVF"] == stor["MHMF"] == stor["MHVG"] == stor["MHMG"] == stor["MHVGM"]
            if op == "add":                                   # job 1 (16 distinct values per matrix) grows A, B and C
                roles = {g["role"] for g in K.parse_growths(gate("MHVG", w, 1, t)[0])}
                assert roles == {"A", "B", "C"}, roles


def test_exact_checks_catch_a_difference():
    build_once()
    w = toy_work("mul")
    pred = json.loads((w / "job-06.m1.json").read_text())["predicted"]
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        for impl in ("MHMG", "MHVGM", "MHVF"):
            lines, dump = gate(impl, w, 6, t)
            assert not K.exact_checks(impl, lines, pred, dump)
            sz = "grow" if impl in C.GROW_IMPLS else "fixed"
            bad = json.loads(json.dumps(pred))
            bad["counters"]["mul"]["k15_ops"] = bad["counters"]["mul"].get("k15_ops", 0) + 1
            assert K.exact_checks(impl, lines, bad)
            bad = json.loads(json.dumps(pred))
            bad["sizings"][sz]["stor_end"]["C"]["probes"] += 1
            bad["sizings"][sz]["stor_end_map"]["C"]["probes"] += 1
            assert any("store C end probes" in f for f in K.exact_checks(impl, lines, bad))
            bad = json.loads(json.dumps(pred))
            bad["cross_match"] += 1
            assert any("operation counters" in f for f in K.exact_checks(impl, lines, bad))
            bad = json.loads(json.dumps(pred))
            bad["sizings"][sz]["tables"]["B"] = "0" * 64
            assert any("B's table" in f for f in K.exact_checks(impl, lines, bad, dump))
            bad = json.loads(json.dumps(pred))
            bad["sizings"][sz]["cells"]["A"] = "0" * 64
            assert any("A's handle fields" in f for f in K.exact_checks(impl, lines, bad, dump))
            assert K.exact_checks(impl, [l for l in lines if not l.startswith("stor ")], pred, dump)
            if impl in C.GROW_IMPLS:
                bad = json.loads(json.dumps(pred))
                bad["sizings"]["grow"]["growths"][0]["fields"] += 1
                assert any("growths differ" in f for f in K.exact_checks(impl, lines, bad))
                assert any("growths differ" in f for f in K.exact_checks(impl, [l for l in lines if not l.startswith(
                    "growth ")], pred))
            if impl in C.M_IMPLS:
                bad = json.loads(json.dumps(pred))
                bad["sizings"][sz]["tau"] = "0" * 64
                assert any("tau differs" in f for f in K.exact_checks(impl, lines, bad, dump))
                bad = json.loads(json.dumps(pred))
                bad["sizings"][sz]["tau_counts"]["tau_none"] += 1
                assert K.exact_checks(impl, lines, bad)
                assert any("reads" in f for f in K.exact_checks(impl, lines + ["xctr mul core reads 1"], pred))
            else:
                assert any("tau" in f for f in K.exact_checks(impl, lines, pred, dict(dump, tau=b"")))
        # the lookup's growths as the map row's: the rewrite probes differ
        lines, dump = gate("MHVGM", w, 6, t)
        assert not K.exact_checks("MHVGM", lines, pred, dump)
        assert K.exact_checks("MHVG", lines, pred, dump)
        # TC prints no store figures
        lines, _ = gate("TC", w, 6, t)
        assert not K.exact_checks("TC", lines, pred) and K.exact_checks("TC", lines + ["stor end A W=2"], pred)
        # a read-out that differs in one cell is caught, with the cell
        ref = (w / "job-06.ref").read_bytes()
        broken = bytearray(ref)
        broken[8 * 5] ^= 1
        (t / "x.out").write_bytes(bytes(broken))
        r = K.readout_check(t / "x.out", ref)
        assert r and r["cell"] == 5 and r["mismatches"] == 1


def test_untimed_steps_toy():
    """the runner's untimed steps end to end on toy jobs of both sets, with the real binaries: step 0 (workloads,
    references, predictions), step 1 (build, pins, tcm.c's diff, step 0's files), step 2 (unit, differential,
    allocation and sanitizer tests), step 4 (gates, the TC gate) and step 5 (counting and memory runs, the repeats on
    jobs 1 and 6, and the call-for-call checks of every pairing). Nothing timed runs."""
    for op in C.OPS:
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            r = RN.Runner(ns(op=op, jobs="1,6,11"))
            r.out, r.tmp = t / "out", t / "inv"
            r.out.mkdir()
            r.tmp.mkdir()
            r.state_file = r.out / "state.json"
            r.wl, r.bin = t / "work", t / "bin"
            r.log = lambda m: None
            r.state = {"elapsed_before": 0.0, "op": op}
            r.step0()
            assert set(r.state["gen"]) == {"1", "6", "11"} and r.state["gen"]["6"]["name"] == f"{op}-6"
            r.t0 = time.monotonic()
            r.step1()
            assert (r.out / "tcm.diff").read_text() == L.tcm_diff_text()
            r.unit_tests()
            ut = r.state["unit_tests"]
            assert set(ut) == {"m1_unit", "m1_diff", "m1_alloc-timing", "m1_alloc-counting", "m1_unit-asan",
                               "m1_diff-asan"} and all(v["rc"] == 0 for v in ut.values()), ut
            r.step4_gates()
            assert len(r.state["gates"]) == 18 and r.state["measuring"]
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
            # a changed step-0 file is refused at step 1
            (r.wl / "job-06.ref").write_bytes(b"x")
            r.build = lambda: None
            try:
                r.step1()
            except RN.RunStop as e:
                assert "step 0's files changed" in str(e)
            else:
                raise AssertionError("a changed workload file was not refused")


# ------------------------------------------------------------------------------------------------- statistics, pilot
def test_statistics_and_verdicts():
    a = [10.0] * 12
    r, lo, hi = A.boot_ratio(a, a)
    assert r == lo == hi == 1.0
    r, lo, hi = A.boot_ratio([20.0] * 12, a)
    assert r == lo == hi == 2.0
    assert A.time_verdict(1.2, 1.3, 0.05) == "win" and A.time_verdict(0.7, 0.8, 0.05) == "loss"
    assert A.time_verdict(0.97, 1.2, 0.05) == "tie"
    assert A.fixed_verdict(0.90) == "win" and A.fixed_verdict(1.10) == "loss" and A.fixed_verdict(1.0) == "tie"
    assert C.SEVERE == 2.0 and C.BAND_MIN == 0.05 and C.BOOT == 10_000 and C.BOOT_SEED == 20261029
    assert A.judged("map against lookup", "creation") and not A.judged("map against lookup", "operation")
    assert A.judged("against TC", "operation") and not A.judged("against TC", "total")
    assert len(C.PAIRINGS) == 9 and sum(1 for p in C.PAIRINGS if p[2] == "against TC") == 4


def test_pilot_projection_and_allowance():
    # rule-m1, Run plan step 3: elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6)
    assert K.pilot_projection(90, 8, 9, 4, 12) == 90 + 6 * (4 * 9 + 12 * 8)
    assert abs(K.pilot_projection(150, 8, 28, 4, 12) - (150 + 6 * (112 + 96))) < 1e-9
    assert C.PILOT_SCALE == 5.0 and C.PILOT_MARGIN == 1.2 and C.PILOT_JOBS == (1, 6, 13)
    assert C.PILOT_LIMIT_S == 30 * 60 and C.HARD_STOP_S == 40 * 60 and C.OPERATOR_TIMEOUT_MIN == 45
    assert C.ROUNDS == 12 and C.PILOT_ROUNDS == 2 and C.COUNT_REPEAT_JOBS == (1, 6) and C.COUNT_LIMIT_S == 600
    assert K.host_allowance(0.5, 10) == 5 + 10 and K.host_allowance(0.0, 10) == 5
    assert K.host_allowance(1.25, 10) == 5 + 25 and C.HOST_RATE_CEILING == 4.0
    assert C.HOST_BASE_TICKS == 5 and C.HOST_FACTOR == 2 and C.HOST_RATE_MIN_TICKS == 1
    assert K.other_limit(1.0) == 6 and K.other_limit(10.0) == 15 and K.other_limit(0.3) == 6


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
    assert p["P_6_s"] == p["walls_s"]["6"] and p["P_13_s"] == p["walls_s"]["13"] and set(p["walls_s"]) == {"1", "6", "13"}
    assert abs(p["projected_s"] - K.pilot_projection(p["elapsed_s"], p["P_6_s"], p["P_13_s"], 4, 12)) < 1e-6
    assert r.state["pilot_calibration"]["1:Hash-V-fixed-null"]["shared_from"] == "Hash-V-fixed"
    for rate in (1.25, 4.0):                              # under, and at, the ceiling of 4 ticks per second: goes on
        pilot_runner(rate).step3_pilot()
    r = pilot_runner(0.0)                                 # no tick in 1.6 s: R_h is 1 tick over 1.6 s, and goes on
    r.step3_pilot()
    assert r.state["host_rates"] == {"1": 1 / 1.6} and r.state["host_rates_measured"] == {"1": 0.0}
    # blocks of 0.2 s: 6 x (4 x 0.2 + 12 x 0.2) = 19.2 s; with 29.8 minutes elapsed the projection passes 30 minutes
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
    r = pilot_runner(0.1, 29.8 * 60 - 25, 0.2)            # 25 s short of the limit before the 19.2 s: goes on
    r.step3_pilot()
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
# round 3's tests, unchanged: rule-m1 item 5 carries the disturbance check over in full, and m1_runner keeps its code
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
# a stand-in for bench_m1 (harness test only): copies the reference as its read-out, prints made-up lines
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
        print(f"calib {r} {r * 20000000} 1 2 3 1 row={row} cpu={r * 19} span={r * 21}")
    if "cap" in cfg and row == "TC":
        print("overcap 20000000000")
elif mode == "time":
    r, reps = int(sys.argv[4]), int(sys.argv[5])
    for i in range(reps):
        print(f"rep {i} {r * (1000 + i)} {r * 100} {r * 800} {r * (100 + i)} {r * 7} row={row} cpu={r * (990 + i)} "
              f"span={r * (1100 + i)}")
print(f"ok impl={row} n=4 op=add ops=16")
'''


def fake_runner(tmp, cfg=""):
    w = toy_work("add")
    r = RN.Runner(ns(jobs="6"))
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
            assert [x["row"] for x in cal] == list(C.ROWS[:6]) and cal[0]["r"] == 4 and cal[0]["reps"] == 3
            assert len(rnd) == 7 * C.ROUNDS and seen[-1]["spawns"] == 6 + 7 * C.ROUNDS == 90   # rule-m1 Run plan 6
            assert [x["row"] for x in rnd[:14]] == list(C.ROWS) + list(C.ROWS[1:]) + [C.ROWS[0]]  # rotated by one
            assert not any(x["row"] == C.NULL_ROW for x in cal)          # the null row takes Hash-V-fixed's r
            x = rnd[0]
            assert x["r"] == 4 and x["total"] == 1001 and x["read"] == 101 and "bug" not in x   # reps 2-3, minimum
            assert x["creation"] == 100 and x["operation"] == 800 and x["tau"] == 7
            assert all(y["launch_s"] is not None and y["launch_s"] > 0 for y in recs)
            assert x["cpu_ns"] == 991 and x["span_ns"] == 1101 and x["cpu_share"] == 991 / 1101
            assert x["reps_cpu"] == [(4 * 990, 4 * 1100), (4 * 991, 4 * 1101), (4 * 992, 4 * 1102)]
            assert cal[0]["reps_cpu"] == [(r * 19, r * 21) for r in (1, 2, 4)], cal[0]
            assert not list(r.tmp.glob("i*")), "the invocation files are removed after the block"
            assert threading.active_count() == 1
            prs = r.timed_block(6, "pilot", pilot=True)
            assert len([x for x in prs if x["role"] == "round"]) == 7 * C.PILOT_ROUNDS and seen[-1]["spawns"] == 20
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
            r.state["measuring"] = True
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
    assert K.runner_ticks_check(5, 38) == (5 / 38, False)
    assert K.runner_ticks_check(92, 90) == (92 / 90, False) and K.runner_ticks_check(93, 90)[1]
    assert K.runner_ticks_check(2, 0) == (None, False) and K.runner_ticks_check(3, 0)[1]
    assert K.runner_ticks_check(None, 38) == (None, False)
    me = 100
    s0 = snap({me: ("python3", 1, 10, 5, 0, 0, 1)}, 0, 0.0)
    for ticks, spawns, over in ((5, 90, False), (92, 90, False), (93, 90, True), (5, 2, True)):
        d = RN.disturbance(s0, snap({me: ("python3", 1, 10 + ticks, 5, 0, 0, 1)}, spawns, 8.0), me, spawns, set(), {})
        assert d["runner_ticks"] == ticks and d["runner_over_limit"] == over and not d["disturbed"], d
    real = RN.disturbance

    def others_ignored(*a, **k):
        return dict(real(*a, **k), disturbed=False, reasons=[])
    try:
        RN.disturbance = others_ignored
        with tempfile.TemporaryDirectory() as t:
            r = fake_runner(Path(t))
            recs = r.timed_block(6, "measured")
            d = r.state["disturbance_checks"][-1]
            assert recs and d["spawns"] == 90 and not d["runner_over_limit"], d
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
TIMES = {"TC": 150.0, "Hash-V-fixed": 100.0, "Hash-V-fixed-null": 100.5, "Hash-M-fixed": 120.0, "Hash-V-grow": 110.0,
         "Hash-M-grow": 130.0, "Hash-V-grow-map": 105.0}


def analysis_state(op="add"):
    """a finished run's state for job 1 (n = 16, 768 cells): made-up times, and memory figures as the spec's accounting
    gives them (TC 16 bytes per cell; Hash-V-fixed 36; grow tables of 256 slots)"""
    recs = []
    for k in range(1, 13):
        for row, tot in TIMES.items():
            recs.append({"job": 1, "row": row, "role": "round", "round": k, "total": tot + k * 0.1,
                         "creation": tot / 5, "operation": tot * 3 / 5 + k * 0.1, "read": tot / 5,
                         "tau": 10.0 if row in ("Hash-M-fixed", "Hash-M-grow") else 0.0, "cpu_share": 0.99,
                         "cpu_ns": tot, "span_ns": tot})

    def mem(cells, table, tau):
        out = []
        for when in ("after_creation", "end"):
            for r in C.ROLES:
                if cells:
                    out += [f"mem {when} {r} cells {cells} {cells}", f"mem {when} {r} table {table} {table}"]
                else:
                    out += [f"mem {when} {r} pairs 4096 4096"]
            out += [f"memtau {when} {tau if when == 'end' else 0}", f"memN {when} 768"]
        return out
    mems = {"TC": mem(0, 0, 0), "MHVF": mem(1024, 8192, 0), "MHMF": mem(1024, 8192, 2048),
            "MHVG": mem(1024, 2048, 0), "MHMG": mem(1024, 2048, 512), "MHVGM": mem(1024, 2048, 0)}
    stor = {w: {r: dict(W=100, T=1024, w=64, wh=16, nf=512, found=400, new=98, probes=600, longest=5, widenings=0,
                        copied=0, growths=0, reinserted=0, rprobes=0, rewritten=0, wprobes=0) for r in C.ROLES}
            for w in ("after_creation", "end")}
    opc = {"after_creation": {f: 0 for f in C.OPC_FIELDS},
           "end": dict(cross=1024, cross_match=40, cross_reads=900, tau_found=30, tau_none=60, tau_probes=150)}
    counts = {f"{i}:1": {"mem": mems[i], "counts": ["ctr add core gcd_work 500"] +
                         (["xctr add core reads 300"] if i in C.M_IMPLS else []), "stor": stor, "opc": opc}
              for i in C.IMPLS}
    heap = {"TC": 17, "MHVF": 38, "MHMF": 41, "MHVG": 14, "MHMG": 15, "MHVGM": 14}
    peak = {"MHVG": (20, 26), "MHMG": (21, 27), "MHVGM": (22, 26)}
    mems_ = {f"{i}:1": {"memrun": {"after_creation": {"heap": heap[i] * 700, "used": 1, "cells": 768},
                                   "end": {"heap": heap[i] * 768, "used": 1, "cells": 768},
                                   "peak": {"heap": peak.get(i, (0, 0))[1] * 768, "used": peak.get(i, (0, 0))[0] * 768,
                                            "growths": 6 if i in peak else 0, "cells": 768}}} for i in C.IMPLS}
    chk = {"G_TC": 500, "result": "pass", **{i: {"call_for_call": "pass", "G": 500, "G_ratio": 1.0,
                                                  "gcd_work_check": "passed"} for i in C.HASH_IMPLS},
           **{k: "pass" for k in ("M against V (fixed)", "M against V (grow)", "grow against fixed (V)",
                                  "grow against fixed (M)", "map against lookup")}}
    cal = {f"1:{row}": {"r": 4, "reps": 1 if row == "Hash-M-grow" else 3} for row in C.ROWS}
    st = {"started": "x", "finished": "y", "op": op, "jobs": [1], "elapsed_saved": 300, "rule_sha256": C.RULE_SHA256,
          "records": recs, "calibration": cal, "toy": False, "counts": counts, "mems": mems_,
          "call_checks": {"1": chk},
          "gen": {"1": {"n": 16, "rho": 1, "s": 256, "nops": 256,
                        "realized": {"s": 256, "distinct_A": 256, "distinct_B": 256, "distinct_C": 250,
                                     "W": {"fixed": {"A": 300, "B": 301, "C": 400}}},
                        "ncase": {"add_general": 200, "add_den": 56}}},
          "gates": {f"{i}:1": {} for i in C.IMPLS}, "round3_after": "unchanged",
          "pilot": {"jobs": [1, 6, 13], "walls_s": {"1": 6.0, "6": 7.0, "13": 9.0}, "P_6_s": 7.0, "P_13_s": 9.0,
                    "N_128": 4, "N_rest": 12, "projected_s": 900.0, "limit_s": 1800, "host_rates": {"1": 0.5}},
          "disturbance_checks": [{"job": 1, "label": "measured", "kind": "rounds", "attempt": 1, "seconds": 25.0,
                                  "disturbed": False, "reasons": [], "runner_ticks": 9, "spawns": 90,
                                  "forks_rise": 90, "forks_unattributed": 0, "hosts": {}, "other_limit": 30}]}
    return st


def test_analysis():
    """a finished state gives, per pairing (nine), per-measure verdicts (creation and operation, each with its own band
    from the null row; total and read-out reported; map against lookup judged on creation only), the memory verdicts
    (tau included; the severe flag), the gcd-work and call-for-call checks, the growth peak, tau's time and share, the
    strata and the grid; a stopped one gives no verdict"""
    with tempfile.TemporaryDirectory() as t:
        t = Path(t)
        st = analysis_state()
        (t / "state.json").write_text(json.dumps(st))
        (t / "results.md").write_text("# head\n<!-- rule ends -->\nold tail\n")
        A.report(t)
        md = (t / "results.md").read_text()
        assert "old tail" not in md and md.startswith("# head")
        for row, ref, kind in C.PAIRINGS:
            assert f"### {row} against {ref} ({kind})" in md, (row, ref)
        sec = md[md.find("### Hash-V-fixed against TC"):]
        # creation 30/20 = 1.5 win; operation (90 + k/10)/(60 + k/10); B_used 36/16 = 2.25 loss, severe
        assert "| 1 (n 16, rho 1) | win, 1.500 [1.500, 1.500], h 0.050 | win, 1.49" in sec, sec[:900]
        assert "loss, 2.250 (2.250) **severe memory loss**" in sec.split("\n\n")[1]
        sec = md[md.find("### Hash-M-fixed against TC"):]
        assert "loss, 2.417 (2.250) **severe memory loss**" in sec.split("\n\n")[1]   # tau's 2048 bytes at the end
        sec = md[md.find("### Hash-V-grow against TC"):]
        assert "win, 0.750 (0.750)" in sec.split("\n\n")[1]
        sec = md[md.find("### Hash-M-grow against TC"):].split("\n\n")[1]
        assert "(c)" in sec                                                   # Hash-M-grow ran 1 rep per invocation
        assert "(c)" not in md[md.find("### Hash-V-grow against TC"):].split("\n\n")[1]
        sec = md[md.find("### Hash-V-grow-map against Hash-V-grow"):].split("\n\n")[1]
        # creation 22/21 = 1.048: a tie within 5%; operation, total and memory reported only
        assert "| 1 (n 16, rho 1) | tie, 1.048 [1.048, 1.048], h 0.050 | reported, 1.04" in sec, sec
        assert "reported, 1.000 (1.000) | reported" in sec and "severe" not in sec
        assert "1.000000 (passed), pass" in md and "| pass, pass | pass, pass | pass |" in md
        assert "| 1 (n 16, rho 1) | 20.00, 26.00 (6) | 21.00, 27.00 (6) | 22.00, 26.00 (6) | 1.100, 1.000 |" in md
        import statistics
        shares = [fmt4(statistics.median([10 / (o + 0.1 * k) for k in range(1, 13)])) for o in (72, 78)]
        assert f"| 1 (n 16, rho 1) | 10 ({shares[0]}) | 10 ({shares[1]}) |" in md, shares   # tau / operation
        assert "| NONE | Hash-V-fixed vs TC | creation | 1 | 0 | 0 | 0 | 0 | 0 | 1.500 | 0 |" in md
        assert "| NONE | Hash-V-fixed vs TC | B_used | 0 | 0 | 1 | 0 | 0 | 0 | 2.250 | 1 |" in md
        assert "| NONE | Hash-V-grow-map vs Hash-V-grow | operation | 0 | 0 | 0 | 0 | 0 | 1 |" in md
        assert "| LOW | Hash-V-fixed vs TC | creation | 0 | 0 | 0 | 0 | 0 | 0 | n/a | 0 |" in md
        grid = md[md.find("Hash-V-fixed against TC, creation:"):]
        assert "| 16 | 1.500 | n/a | n/a | n/a |" in grid.split("\n\n")[1] and "| 128 | n/a |" in grid
        assert "| 1 | fixed | A | end | 100 | 1024 | 9.8% | 64 | 16 | 400 / 98 | 1.20 | 5 |" in md
        assert "| 1 | Hash-M-fixed | 30 | 60 | 150 | 2048 |" in md
        assert "| 1 | 256 | 1024 | 40 | 900 | 0 | 300 |" in md
        assert "| no | 9 | 90 | 0.100 | 90 |" in md and "results-r3/results.md" in md
        assert "results-m1-add/results.md" not in md
        v = list(csvrows(t / "verdicts.csv"))
        got = {(x["pairing"], x["measure"], x["verdict"]) for x in v}
        assert {("Hash-V-fixed vs TC", "creation", "win"), ("Hash-V-fixed vs TC", "total", "reported"),
                ("Hash-V-grow-map vs Hash-V-grow", "operation", "reported"),
                ("Hash-V-grow-map vs Hash-V-grow", "creation", "tie"),
                ("Hash-V-grow-map vs Hash-V-grow", "B_used", "reported"), ("Hash-V-fixed vs TC", "B_used", "loss"),
                ("Hash-M-fixed vs Hash-V-fixed", "operation", "loss"),
                ("Hash-V-grow vs Hash-V-fixed", "B_heap", "win"),
                ("Hash-M-grow vs TC", "gcd work (check)", "passed")} <= got, got
        assert len(list(csvrows(t / "rounds.csv"))) == 7 * 12
        # one band per measure: a noisy null on the operation part widens h for the operation only
        for x in st["records"]:
            if x["row"] == C.NULL_ROW:
                x["operation"] *= 1.3 if x["round"] % 2 else 0.8
        d = A.Data(st)
        hc, _ = A.null_h(d, 1, "creation")
        ho, _ = A.null_h(d, 1, "operation")
        assert hc == C.BAND_MIN and ho > 0.1, (hc, ho)
        # over cap: both time measures of every pairing with the row
        st2 = analysis_state()
        st2["calibration"]["1:TC"]["overcap"] = True
        d = A.Data(st2)
        assert A.time_measure(d, 1, "Hash-V-fixed", "TC", "operation", "against TC")["verdict"] == "over cap"
        assert A.time_measure(d, 1, "Hash-V-fixed", "TC", "total", "against TC")["verdict"] == "reported"
        # the multiplication report links the addition report
        (t / "state.json").write_text(json.dumps(analysis_state("mul")))
        A.report(t)
        assert "results-m1-add/results.md" in (t / "results.md").read_text()
        st["run_stopped"] = {"step": "step 4", "reason": "a gate failed"}
        (t / "state.json").write_text(json.dumps(st))
        A.report(t)
        md = (t / "results.md").read_text()
        assert "No verdict is reported" in md and "| 1 (n 16, rho 1) | win" not in md


def fmt4(x):
    return f"{x:.4f}"


def csvrows(p):
    import csv
    with open(p) as f:
        yield from csv.DictReader(f)


# ------------------------------------------------------------------------------------------------- toy runs
def test_toy_runs_end_to_end():
    """the runner as the operator starts it, with --toy: the addition run with --pilot-only and then --resume (steps
    4-6 after the pilot), and the multiplication run in one go; each finishes, in its own toy folder, with every gate
    and call-for-call check passed and every pairing reported. A harness test: nothing here is a measurement."""
    py = [sys.executable, str(C.HERE / "m1_runner.py")]
    out = {op: C.TOY_BUILD / f"results-{op}" for op in C.OPS}
    p = run(py + ["--op", "add", "--toy", "--jobs", "6", "--pilot-only"], timeout=900)
    assert p.returncode == 0 and "pilot only: stopping after step 3" in p.stdout, p.stdout[-1500:] + p.stderr[-1500:]
    st = json.loads((out["add"] / "state.json").read_text())
    assert "pilot" in st and not st.get("records") and not st.get("gates") and not st.get("finished")
    p = run(py + ["--op", "add", "--toy", "--jobs", "6", "--resume"], timeout=900)
    assert p.returncode == 0, p.stdout[-1500:] + p.stderr[-1500:]
    p = run(py + ["--op", "mul", "--toy", "--jobs", "6"], timeout=900)
    assert p.returncode == 0, p.stdout[-1500:] + p.stderr[-1500:]
    for op in C.OPS:
        st = json.loads((out[op] / "state.json").read_text())
        assert st["op"] == op and st["toy"] and st.get("finished") and not st.get("run_stopped"), st.get("run_stopped")
        assert len(st["code_sessions"]) == (2 if op == "add" else 1)
        assert st["call_checks"]["6"]["result"] == "pass" and not st.get("bugs")
        assert all("fail" not in g for g in st["gates"].values()) and len(st["gates"]) == 6
        assert len([x for x in st["records"] if x["role"] == "round"]) == 7 * C.ROUNDS
        assert st["gen"]["6"]["name"] == f"{op}-6" and st["round3_after"] == "unchanged"
        md = (out[op] / "results.md").read_text()
        assert "TOY RUN" in md and "<!-- rule ends -->" in md and "## Verdicts, per job" in md
        assert all(f"### {row} against {ref} ({kind})" in md for row, ref, kind in C.PAIRINGS)
    assert C.round3_check() is None


# ------------------------------------------------------------------------------------------------- full size
def full():
    """every gate and counting run on the full-size workloads of both sets (untimed); the multiplication set's model
    takes a few minutes at n = 128"""
    build_once()
    for op in C.OPS:
        with tempfile.TemporaryDirectory() as t:
            t = Path(t)
            w = t / "work"
            m1_gen.generate(w, op, list(C.JOBS))
            for job in C.JOBS:
                pred = json.loads((w / f"job-{job:02d}.m1.json").read_text())["predicted"]
                for impl in C.IMPLS:
                    lines, dump = gate(impl, w, job, t)
                    f = K.exact_checks(impl, lines, pred, dump)
                    assert not f, (op, job, impl, f[:5])
                    p = run([BIN / f"bench-{impl}-counting", "count", w / f"job-{job:02d}.wl", t / "c.out",
                             t / f"p-{impl}.bin"])
                    assert p.returncode == 0 and not K.exact_checks(impl, p.stdout.splitlines(), pred)
                p = run([BIN / "tc3gate", w / f"job-{job:02d}.wl", t / "g3.out", t / "p3.bin"])
                assert p.returncode == 0 and (t / "p3.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
                for i in C.HASH_IMPLS:
                    assert (t / f"p-{i}.bin").read_bytes() == (t / "p-TC.bin").read_bytes()
                print(f"  full size: {op}-{job} gates pass for the six rows, the TC gate, call for call")


TESTS = [test_rule_fixed_and_pinned, test_pins_and_round3, test_forbidden_places, test_runner_imports_no_numpy,
         test_runner_arguments, test_workloads, test_model_worked_by_hand, test_tcm_changes_only_the_interface,
         test_hash_rows_one_source, test_only_named_structures, test_c_suites, test_gates_and_counts_toy,
         test_exact_checks_catch_a_difference, test_untimed_steps_toy, test_statistics_and_verdicts,
         test_pilot_projection_and_allowance, test_pilot_stops, test_host_allowance_from_pilot,
         test_spawner_affinity_before_exec, test_hard_stop_kernel_timer, test_idle_probe, test_disturbance_synthetic,
         test_existing_child_exit_charged_for_the_block_only, test_host_threads_do_not_disturb,
         test_disturbance_catches_short_lived_process, test_timed_block_machinery, test_block_rerun_once,
         test_runner_ticks_per_launch, test_pool_is_joined, test_analysis, test_toy_runs_end_to_end]


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
