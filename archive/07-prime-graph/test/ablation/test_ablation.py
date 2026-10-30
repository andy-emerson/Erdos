"""Correctness tests for the ablation strategies (V0, V0', V1..V5), through the benchmark binaries themselves.

    python3 test_ablation.py          (or: python3 -m unittest test_ablation -v)

Every strategy is run on random inputs for every job and checked against Python's fractions; outputs must be in
lowest terms, and a strategy may report an overflow only on an element where V0 (the prototype) also does.  Every
check is also fed a deliberately wrong result to show that it catches one: the harness's value check, its unreduced
rule and overflow limits, and each strategy's structural audit (V5's reference counts, V3's exception list, the
per-value edge lists).  V5's reference counting and freeing, V3's exceptions and V4's early reductions are each
driven on inputs that exercise them.
"""
import os
import random
import tempfile
import unittest
from fractions import Fraction
from math import gcd, lcm
from pathlib import Path

import numpy as np

import ablation_bench as ab
import pg_bench

ab.build()
STRATS = list(ab.STRATEGIES)
JOBS = {"sum": "a", "add": "ab", "mul": "ab", "fma": "abc", "chain": "abc", "fma2": "abcef"}


def rand_frac(rng, kind):
    if kind == "small":
        return rng.randint(-10**6, 10**6), rng.randint(1, 12)
    if kind == "dec":
        return rng.randint(-10**4, 10**4), 1000
    if kind == "money":
        return rng.randint(-10**6, 10**6), 100
    if kind == "unrel":
        return rng.randint(-10**6, 10**6), rng.randint(1, 1000)
    if kind == "big":
        return rng.randint(-10**4, 10**4), rng.randint(1, 2**31)
    if kind == "int":
        return rng.randint(-10**6, 10**6), 1
    if kind == "wide":
        return rng.randint(-2**40, 2**40), rng.choice([2**20, 3**12, 6**10, 1000**3, 7 * 11 * 13 * 17 * 19 * 23])
    if kind == "mixed90":
        return (rng.randint(-10**7, 10**7), 1000) if rng.random() < 0.9 else (rng.randint(-10**6, 10**6), rng.randint(1, 12))
    raise ValueError(kind)


def make_inputs(d, names, cols):
    """cols: {name: [(num, den), ...]} -> d/{name}.bin"""
    arrays = {k: (np.array([x for x, _ in cols[k]], dtype=np.int64), np.array([y for _, y in cols[k]], dtype=np.uint64))
              for k in names}
    pg_bench.write_inputs(d, arrays)
    return arrays


def run(v, job, d, K=3, reps=2, **env):
    e = dict(os.environ)
    e.update({k: str(x) for k, x in env.items()})
    out = Path(d) / f"out-{v}-{job}.txt"
    st = ab.run_one(v, job, d, reps, K, out, env=e)
    lines = out.read_text().splitlines() if out.exists() and "unable" not in st else None
    if out.exists():
        out.unlink()
    return st, lines


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.d = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()


# ------------------------------------------------------------------------------------------------- checks catch wrong

class TestChecksCatchWrong(Base):
    def test_value_check(self):
        exp = [Fraction(1, 3), Fraction(-5, 2), Fraction(7)]
        self.assertTrue(ab.check_rows(["2 6", "-5 2", "7 1"], exp, None, {})[0])
        for bad in (["2 6", "-5 2", "8 1"], ["1 3", "5 -2", "7 1"], ["1 3", "-5 2"], ["1 3", "0 0", "7 1"],
                    ["1 3", "-5", "7 1"]):
            self.assertFalse(ab.check_rows(bad, exp, None, {})[0], bad)

    def test_overflow_limits(self):
        big = Fraction(1, 2**63 + 1)
        exp = [Fraction(k, 3) for k in range(2000)] + [big]
        lines = [f"{k} 3" for k in range(2000)] + ["0 0"]
        self.assertTrue(ab.check_rows(lines, exp, None, {})[0])                       # one genuine overflow: ok
        false = lines[:5] + ["0 0"] + lines[6:]
        self.assertFalse(ab.check_rows(false, exp, None, {})[0])                      # a false overflow fails
        self.assertFalse(ab.check_rows(["0 0"] * 3, [big] * 3, None, {})[0])          # over 0.1% fails

    def test_unreduced_rule(self):
        exp = [Fraction(1, 3), Fraction(1, 2)]
        ok, msg, extra = ab.judge("v1", ["1 3", "1 2"], exp, None, True)
        self.assertTrue(ok)
        ok, msg, extra = ab.judge("v1", ["2 6", "1 2"], exp, None, True)
        self.assertFalse(ok, "an unreduced output must fail the row")
        self.assertEqual(extra["unreduced"], 50.0)

    def test_binary_corruption_caught(self):
        rng = random.Random(1)
        cols = {k: [rand_frac(rng, "small") for _ in range(200)] for k in "abc"}
        arrays = make_inputs(self.d, "abc", cols)
        exp = ab.expected("fma", arrays)
        for v in STRATS:
            st, lines = run(v, "fma", self.d)
            self.assertTrue(ab.judge(v, lines, exp, None, True)[0], v)
            st, lines = run(v, "fma", self.d, AB_CORRUPT="value")
            self.assertFalse(ab.judge(v, lines, exp, None, True)[0], f"{v}: a wrong value passed")
            st, lines = run(v, "fma", self.d, AB_CORRUPT="unreduced")
            ok, msg, extra = ab.judge(v, lines, exp, None, True)
            self.assertFalse(ok, f"{v}: an unreduced output passed")
            self.assertIn("lowest terms", msg)

    def test_audits_catch_corruption(self):
        rng = random.Random(2)
        cols = {k: [rand_frac(rng, rng.choice(["unrel", "small"])) for _ in range(300)] for k in "abc"}
        make_inputs(self.d, "abc", cols)
        cases = {"v0p": "den", "v1": "den", "v2": "den", "v4": "den", "v3-lcm": "xi", "v3-freq": "xi", "v5": "rc"}
        for v, how in cases.items():
            st, _ = run(v, "fma", self.d, AB_AUDIT=1)
            self.assertEqual(st.get("audit"), "ok", v)
            st, _ = run(v, "fma", self.d, AB_AUDIT=1, AB_CORRUPT=how)
            self.assertTrue(str(st.get("audit")).startswith("FAIL"), f"{v}: the audit missed corruption '{how}'")

    def test_report_ratio_against_control(self):
        row = lambda ok, t: {"median": {"entry": 1e6, "decomp": 0, "op": t, "red": 0, "total": t + 1e6}, "ok": ok,
                             "msg": "" if ok else "x", "unreduced": 0.0, "last": {}, "out_dens": 1}
        cfg = {"date": "", "cpu": "", "rounds": 1, "reps": 1}
        res = {"workloads": {"W": {"desc": "d", "n": 1, "K": None,
                                   "jobs": {"sum": {"v0p": row(True, 3e6), "v0": row(True, 1e6), "v1": row(False, 1e6)}}}}}
        md = ab.report_md(res, cfg).splitlines()
        rows = [l for l in md if l.startswith("| v")]
        self.assertTrue(rows[0].startswith("| v0p"), "the control must come first")
        self.assertIn("| 2.00 | 3.00 |", next(l for l in rows if l.startswith("| v0 ")))  # V0' / x: 4/2, 3/1
        self.assertIn("| - | - |", next(l for l in rows if l.startswith("| v1 ")))        # failed row: no ratio
        res["workloads"]["W"]["jobs"]["sum"]["v0p"] = row(False, 3e6)
        md = ab.report_md(res, cfg).splitlines()
        self.assertIn("| - | - |", next(l for l in md if l.startswith("| v0 ")))         # failed control: no ratio
        self.assertTrue(any("V0' FAILED its check" in l for l in md))                     # ... and a note says why
        res["workloads"]["W"]["jobs"]["sum"]["v0p"] = {"unable": "overflow"}
        md = ab.report_md(res, cfg).splitlines()
        self.assertIn("| - | - |", next(l for l in md if l.startswith("| v0p")))         # unable: '-' in both cells
        self.assertTrue(any("V0' is unable here" in l for l in md))
        # the op ratio includes the final reduction: control op 3 + final 1 over row op 1 + final 1 = 2.00
        r = row(True, 3e6); r["median"]["red"] = 1e6
        x = row(True, 1e6); x["median"]["red"] = 1e6
        res["workloads"]["W"]["jobs"]["sum"] = {"v0p": r, "v4": x}
        md = ab.report_md(res, cfg).splitlines()
        cells = [c.strip() for c in next(l for l in md if l.startswith("| v4")).split("|")]
        self.assertEqual(cells[8], "2.00")  # columns: '', strategy, entry, decomp, op, final, total, total ratio, op ratio


# ------------------------------------------------------------------------------------------------- random correctness

def overflow_set(lines):
    return {i for i, l in enumerate(lines) if l == pg_bench.OVF_LINE}


class TestRandom(Base):
    def run_kind(self, kinds, seed, n=300, jobs=JOBS, allow_ovf=False):
        rng = random.Random(seed)
        cols = {k: [rand_frac(rng, rng.choice(kinds)) for _ in range(n)] for k in "abcef"}
        cols["e"] = [(x % 201 - 100, y) for x, y in cols["e"]]  # (a*b + c)*e stays mostly within 64 bits
        arrays = make_inputs(self.d, "abcef", cols)
        cd = self.d / "chain"  # chains multiply by b three times: b gets small numerators, as in test_pg's chains
        chain_cols = dict(cols, b=[(x % 201 - 100, y) for x, y in cols["b"]])
        chain_arrays = make_inputs(cd, "abc", chain_cols)
        for job in jobs:
            d, arrs = (cd, chain_arrays) if job == "chain" else (self.d, arrays)
            exp = ab.expected(job, arrs, 3)
            ref = None
            for v in ["v0"] + [s for s in STRATS if s != "v0"]:
                st, lines = run(v, job, d, AB_AUDIT=1)
                where = f"{v} {job} {kinds}"
                if "unable" in st:
                    self.assertEqual(job, "sum", where)
                    L = lcm(*[y for _, y in cols["a"]])
                    bound = sum(abs(x) * (L // y) for x, y in cols["a"])
                    self.assertTrue(L >= 2**63 or bound >= 2**63, f"{where}: unable on a sum that fits")
                    continue
                self.assertEqual(st.get("audit"), "ok", where)
                # every element not reported as an overflow is exact; reported overflows are counted by the binary,
                # and (elementwise) a strategy may report one only where the prototype V0 also does
                ovs = overflow_set(lines)
                ok, msg = ab.check_rows(lines, exp, [i for i in range(len(exp)) if i not in ovs])
                self.assertTrue(ok, f"{where}: {msg}")
                self.assertEqual(ab.unreduced_pct(lines), 0.0, f"{where}: unreduced outputs")
                if ovs:
                    self.assertGreater(st.get("ovf"), 0, f"{where}: '0 0' without a counted overflow")
                if v == "v0":
                    ref = ovs
                elif job != "sum":
                    self.assertFalse(ovs - ref, f"{where}: overflowed where V0 did not: {sorted(ovs - ref)[:5]}")
                if not allow_ovf:
                    self.assertFalse(ovs, f"{where}: overflow on data that fits")

    def test_one_scale(self):
        for kinds, seed in ((["dec"], 10), (["money"], 11), (["int"], 12)):
            self.run_kind(kinds, seed)

    def test_mixed_small(self):
        for kinds, seed in ((["small"], 20), (["mixed90"], 21), (["small", "dec", "int"], 22)):
            self.run_kind(kinds, seed)

    def test_unrelated_and_wide(self):  # overflows possible: each must be genuine and also V0's
        for kinds, seed in ((["unrel"], 30), (["big"], 31), (["wide"], 32), (["small", "dec", "unrel", "big", "int", "wide"], 33)):
            self.run_kind(kinds, seed, allow_ovf=True)


# ------------------------------------------------------------------------------------------------- strategy mechanics

class TestMechanics(Base):
    def test_v5_reference_counts(self):
        """Objects are shared, counted and freed: the audit recounts every reference after each job."""
        rng = random.Random(40)
        cols = {k: [rand_frac(rng, rng.choice(["small", "dec", "money", "int"])) for _ in range(500)] for k in "abcef"}
        cols["b"] = [(x % 201 - 100, y) for x, y in cols["b"]]  # keep the chain within 64 bits
        cols["e"] = [(x % 201 - 100, y) for x, y in cols["e"]]
        arrays = make_inputs(self.d, "abcef", cols)
        dec = {k: [rand_frac(rng, rng.choice(["dec", "money"])) for _ in range(500)] for k in "abc"}
        dec["b"] = [(x % 201 - 100, y) for x, y in dec["b"]]
        chain_arrays = make_inputs(self.d / "chain", "abc", dec)
        for job in ("sum", "add", "mul", "fma", "chain", "fma2"):
            d, arrs = (self.d / "chain", chain_arrays) if job == "chain" else (self.d, arrays)
            st, lines = run("v5", job, d, AB_AUDIT=1)
            self.assertEqual(st["audit"], "ok", job)
            ok, msg = ab.check_rows(lines, ab.expected(job, arrs, 3))
            self.assertTrue(ok, f"{job}: {msg}")
            self.assertLessEqual(st["objs_live"], st["objs_peak"], job)
            self.assertEqual(st["objs_made"] - st["objs_freed"], st["objs_live"], job)
            if job in ("sum", "fma", "chain", "fma2"):  # temporaries and overwritten slots are released and freed
                self.assertGreater(st["objs_freed"], 0, job)
        # sharing: a one-scale array holds far fewer objects than values
        cols = {k: [rand_frac(rng, "dec") for _ in range(2000)] for k in "abc"}
        make_inputs(self.d, "abc", cols)
        st, _ = run("v5", "fma", self.d, AB_AUDIT=1)
        self.assertLess(st["objs_peak"], 200)

    def test_v3_exceptions(self):
        """Exceptions from denominators beyond D and from scaled numerators that overflow; all-graph fallback."""
        rng = random.Random(41)
        cols = {}
        for k in "abc":  # mostly over 1000, some over unrelated denominators; a and c have some numerators near
            col = []     # 2^30 over 1, 2, 5, whose scaling to a large D overflows (exceptions), while a*b + c fits
            for _ in range(600):
                t = rng.random()
                if t < 0.7:
                    col.append((rng.randint(-10**7, 10**7), 1000))
                elif t < 0.9 or k == "b":
                    col.append((rng.randint(-10**6, 10**6), rng.choice([7, 11, 13, 999, 997, 641])))
                else:
                    col.append((rng.randint(2**29, 2**30), rng.choice([1, 2, 5])))
            cols[k] = col
        arrays = make_inputs(self.d, "abc", cols)
        for policy in ("lcm", "freq"):
            for job in ("fma",):
                st, lines = run("v3-lcm", job, self.d, AB_AUDIT=1, AB_V3_POLICY=policy)
                self.assertEqual(st["policy"], policy)
                self.assertEqual(st["audit"], "ok", (policy, job))
                exp = ab.expected(job, arrays, 3)
                ok, msg = ab.check_rows(lines, exp, None, {})
                self.assertTrue(ok, f"{policy} {job}: {msg}")
                self.assertEqual(ab.unreduced_pct(lines), 0.0)
                self.assertGreater(st["exc_in"], 0, (policy, job))
                if job == "fma":
                    self.assertGreater(st["exc_out"] + st["allslow"], 0, (policy, job))
        # freq policy on this data: D = 1000, so the unrelated denominators are exceptions
        st, _ = run("v3-lcm", "fma", self.d, AB_V3_POLICY="freq")
        self.assertEqual(st["D"], 1000)
        # the column sum with exceptions: one-scale data plus a few values over unrelated denominators
        cols = {"a": [(rng.randint(-10**7, 10**7), 1000) for _ in range(900)] +
                     [(rng.randint(-10**3, 10**3), rng.choice([7, 11, 13])) for _ in range(30)]}
        rng.shuffle(cols["a"])
        arrays = make_inputs(self.d, "a", cols)
        for policy in ("lcm", "freq"):
            st, lines = run("v3-lcm", "sum", self.d, AB_AUDIT=1, AB_V3_POLICY=policy)
            self.assertTrue(ab.check_rows(lines, ab.expected("sum", arrays))[0], policy)
            self.assertEqual(ab.unreduced_pct(lines), 0.0)
            if policy == "freq":
                self.assertEqual((st["D"], st["exc_in"]), (1000, 30))
        # a dense overflow: one-scale data whose products overflow int64 take the graph path, never fail falsely
        cols = {k: [(1000 * rng.randint(3 * 10**6, 4 * 10**6), 1000) for _ in range(300)] for k in "abc"}  # integers
        arrays = make_inputs(self.d, "abc", cols)
        st, lines = run("v3-lcm", "fma", self.d, AB_AUDIT=1)
        self.assertGreater(st["dense_ovf"], 0)
        self.assertTrue(ab.check_rows(lines, ab.expected("fma", arrays))[0])

    def test_v3_rows_and_D_after_entry(self):
        """The two declared V3 rows pick their rule; D is recorded after entry (not after a chain's swaps)."""
        self.assertEqual(ab.STRATEGIES["v3-lcm"][2], {"AB_V3_POLICY": "lcm"})
        self.assertEqual(ab.STRATEGIES["v3-freq"][2], {"AB_V3_POLICY": "freq"})
        rng = random.Random(46)
        cols = {k: [rand_frac(rng, "dec") for _ in range(300)] for k in "abc"}
        cols["b"] = [(x % 201 - 100, y) for x, y in cols["b"]]
        make_inputs(self.d, "abc", cols)
        for v in ("v3-lcm", "v3-freq"):
            st = ab.run_one(v, "chain", self.d, 1, 3, self.d / "o.txt")
            self.assertEqual(st["policy"], v.split("-")[1])
            self.assertEqual(st["D"], 1000, v)

    def test_v3_single_values_plain(self):
        rng = random.Random(42)
        cols = {k: [rand_frac(rng, "small") for _ in range(300)] for k in "ab"}
        arrays = make_inputs(self.d, "ab", cols)
        st, lines = run("v3-lcm", "add", self.d, AB_AUDIT=1)
        self.assertEqual(st["exc_in"], 600)  # every single value is an exception (plain), i.e. V0'
        self.assertTrue(ab.check_rows(lines, ab.expected("add", arrays))[0])

    def test_v4_early_reduction(self):
        """Unreduced products that overflow are reduced and retried, not failed."""
        rng = random.Random(43)
        cols = {k: [(rng.randint(1, 2**20) * 2**38, 2**40 * rng.choice([1, 3, 5])) for _ in range(300)] for k in "abc"}
        arrays = make_inputs(self.d, "abc", cols)
        for job in ("fma", "sum"):
            st, lines = run("v4", job, self.d, AB_AUDIT=1)
            self.assertTrue(ab.check_rows(lines, ab.expected(job, arrays, 3))[0], job)
            self.assertEqual(ab.unreduced_pct(lines), 0.0, job)
            if job != "sum":
                self.assertGreater(st["early"], 0, job)
                self.assertEqual(st["ovf"], 0, job)

    def test_v1_and_v2_paths(self):
        rng = random.Random(44)
        cols = {"a": [rand_frac(rng, "mixed90") for _ in range(3000)]}
        arrays = make_inputs(self.d, "a", cols)
        st, lines = run("v2", "sum", self.d)
        self.assertGreater(st["buckets"], 5)
        self.assertTrue(ab.check_rows(lines, ab.expected("sum", arrays))[0])
        st, lines = run("v1", "sum", self.d)
        self.assertTrue(ab.check_rows(lines, ab.expected("sum", arrays))[0])
        self.assertEqual(gcd(*map(int, lines[0].split())), 1)

    def test_v0_matches_prototype_bench(self):
        """V0 here is pg.c as bench.c runs it: same outputs as the prototype's own driver."""
        bench = ab.TEST / "build" / "pg_bench"
        if not bench.exists():
            self.skipTest("prototype benchmark binary not built")
        rng = random.Random(45)
        cols = {k: [rand_frac(rng, "small") for _ in range(400)] for k in "abc"}
        make_inputs(self.d, "abc", cols)
        _, mine = run("v0", "fma", self.d)
        out = self.d / "proto.txt"
        import subprocess
        subprocess.run([str(bench), "pg", "fma", str(self.d), str(out), "1"], check=True, capture_output=True)
        self.assertEqual(mine, out.read_text().splitlines())


if __name__ == "__main__":
    unittest.main(verbosity=2)
