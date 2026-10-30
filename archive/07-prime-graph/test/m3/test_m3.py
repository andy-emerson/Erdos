"""Tests for the M3 peer comparison (m3_bench.py, m3_bench.c).

    python3 test_m3.py          (or: python3 -m unittest test_m3 -v)

1. Each check catches a wrong result: the checker is given right results and then results corrupted in every way it
   must reject (wrong numerator, wrong denominator, a right value not in lowest terms, a negative denominator, a
   missing result, a missing element, two results swapped), both on synthetic arrays and through the real output path
   of every contender (the driver corrupts its own output when M3_CORRUPT is set) on every job kind; FLINT running
   with more than one thread is rejected too.
2. Every contender gives exactly Python's fractions on small inputs of every workload and on hand-made inputs that
   need cancellation, sign handling and values near the 64-bit bounds.
3. The generator: every workload fits the M1 bounds; the reuse axes hold what rule 7c says (A: uses per value U with
   10 values per denominator; B: one use per value, V values per denominator); shared pairs share.
4. The statistics: the bootstrap, the verdict band (5% floor, widened by the null spread), the best library, n/a
   propagation, and the pay-back answer.
5. The rule guard: no measured run without the pre-registered rule; partial runs cannot write results/; --revise
   rewrites the rule only while no results exist.
"""
import os
import shutil
import subprocess
import tempfile
import unittest
from fractions import Fraction
from pathlib import Path

import numpy as np

import m3_bench as m3

m3.build()
CONTENDERS = ("pg", "pg-v3", "mpq", "fmpq", "fmpq-am", "fz")
CORRUPTIONS = {  # M3_CORRUPT mode: the reason the checker must give (a substring of its message)
    "num:3": "wrong value", "den:3": "wrong value", "unred:3": "not in lowest terms", "sign:3": "nonpositive",
    "ovf:3": "no result", "drop": "results, expected", "swap:3": "wrong value",
}


def run(con, job, d, extra=None, corrupt=None):
    out = Path(d) / f"out-{con}-{job}.bin"
    env = dict(os.environ)
    env.pop("M3_CORRUPT", None)
    if corrupt:
        env["M3_CORRUPT"] = corrupt
    st = m3.run_one(con, job, Path(d), 1, extra, out, env=env)
    if "unable" in st:
        return st, None
    got = m3.read_out(out)
    out.unlink()
    return st, got


def frac_arrays(fs):
    return (np.array([f.numerator for f in fs], dtype=np.int64), np.array([f.denominator for f in fs], dtype=np.int64))


class TmpDir(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix="m3test-"))

    def tearDown(self):
        shutil.rmtree(self.tmp, ignore_errors=True)


# ---------------------------------------------------------------------------------------------------- 1. checks

class CheckCatchesWrongResults(TmpDir):
    def setUp(self):
        super().setUp()
        self.exp = frac_arrays([Fraction(1, 2), Fraction(-7, 3), Fraction(0), Fraction(5), Fraction(2**62, 3),
                                Fraction(-1, 2**62 + 1)])

    def corrupted(self, how):
        n, d = self.exp[0].copy(), self.exp[1].copy()
        if how == "num":
            n[1] += 1
        elif how == "den":
            d[1] += 1
        elif how == "unred":
            n[0], d[0] = 2 * n[0], 2 * d[0]
        elif how == "unred-zero":   # 0/2 is 0, but not in lowest terms (0/1)
            d[2] = 2
        elif how == "sign":
            n[1], d[1] = -n[1], -d[1]
        elif how == "ovf":
            n[3], d[3] = 0, 0
        elif how == "short":
            n, d = n[:-1], d[:-1]
        elif how == "swap":
            n[[0, 1]], d[[0, 1]] = n[[1, 0]], d[[1, 0]]
        return n, d

    def test_right_results_pass(self):
        ok, msg = m3.check(self.exp, self.exp)
        self.assertTrue(ok, msg)

    def test_each_corruption_is_caught_with_its_reason(self):
        reasons = {"num": "wrong value", "den": "wrong value", "unred": "not in lowest terms",
                   "unred-zero": "not in lowest terms", "sign": "nonpositive", "ovf": "no result",
                   "short": "results, expected", "swap": "wrong value"}
        for how, why in reasons.items():
            with self.subTest(how=how):
                ok, msg = m3.check(self.corrupted(how), self.exp)
                self.assertFalse(ok)
                self.assertIn(why, msg)

    def test_flint_threads_must_be_one(self):
        self.assertTrue(m3.judge({"flint_threads": 1}, self.exp, self.exp)[0])
        ok, msg = m3.judge({"flint_threads": 2}, self.exp, self.exp)
        self.assertFalse(ok)
        self.assertIn("threads", msg)

    def test_driver_reports_flint_threads_one(self):
        out = subprocess.run([str(m3.BIN), "--version"], capture_output=True, text=True, check=True).stdout
        self.assertEqual(m3.parse_stats(out)["flint_threads"], 1)


class EveryContenderEveryJobKind(TmpDir):
    """Through the driver's real output path: each contender on each job kind passes clean, and each corruption of its
    output is caught by the harness."""

    @classmethod
    def setUpClass(cls):
        cls.tmpc = Path(tempfile.mkdtemp(prefix="m3jobs-"))
        W = m3.workloads(scale=0.001)
        cls.cases = []  # (dir, job, extra, expected)
        for wl in ("DEC", "MIX12", "G-mix4", "S16", "A-mid-U3", "B-pq-V6"):
            w = W[wl]
            data = w["gen"](m3.wrng(7, wl), w["n"])
            d = cls.tmpc / wl
            m3.write_inputs(d, data)
            for job in w["jobs"]:
                cls.cases.append((d, job, w.get("K", 0), m3.expected(job, data, w.get("K", 0)), w["kind"]))

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.tmpc, ignore_errors=True)

    def test_clean_runs_pass(self):
        kinds = set()
        for d, job, K, exp, kind in self.cases:
            for con in CONTENDERS:
                with self.subTest(case=d.name, job=job, con=con):
                    st, got = run(con, job, d, K)
                    if con == "fz" and kind == "ops":
                        self.assertIn("unable", st)
                        continue
                    ok, msg = m3.judge(st, got, exp)
                    self.assertTrue(ok, msg)
                    self.assertEqual(st["toolong"], 0)
                    for k in ("entry", "op", "exit", "total"):
                        self.assertGreaterEqual(st[k], 0)
                    self.assertLessEqual(st["total"], st["entry"] + st["op"] + st["exit"] + 10**9)
                    kinds.add(job)
        self.assertEqual(kinds, {"sum", "fma", "chain", "add", "mul", "mix"})

    def test_every_corruption_caught_for_every_contender(self):
        for d, job, K, exp, kind in self.cases:
            for con in CONTENDERS:
                if con == "fz" and kind == "ops":
                    continue
                for mode, why in CORRUPTIONS.items():
                    if job == "sum" and mode.startswith("swap"):
                        continue  # one result: nothing to swap
                    with self.subTest(case=d.name, job=job, con=con, mode=mode):
                        st, got = run(con, job, d, K, corrupt=mode)
                        ok, msg = m3.judge(st, got, exp)
                        self.assertFalse(ok, f"{mode} not caught")
                        self.assertIn(why, msg)


# ---------------------------------------------------------------------------------------------------- 2. exactness

class HandMadeInputs(TmpDir):
    """Cancellation to zero and to integers, signs, gcd > 1 inputs, and values near the 64-bit bounds."""

    def test_arrays(self):
        a = [(1, 6), (-3, 9), (5, 10), (0, 7), (2**62, 3), (-(2**62), 2**62 + 1), (6, -4), (7, 1), (1, 2**31 - 1)]
        b = [(2, 1), (3, 1), (-2, 1), (5, 3), (1, 2), (1, 1), (2, 3), (1, 7), (2**31 - 1, 1)]
        c = [(-1, 3), (1, 1), (1, 1), (1, 1), (0, 1), (2**62, 2**62 + 1), (1, 1), (0, 1), (-1, 1)]
        data = {k: (np.array([p for p, _ in v], dtype=np.int64), np.array([q for _, q in v], dtype=np.int64))
                for k, v in zip("abc", (a, b, c))}
        # entry accepts a negative denominator only through the sign in the numerator: normalise for the driver
        for k in data:
            num, den = data[k]
            data[k] = (np.where(den < 0, -num, num), np.abs(den))
        m3.write_inputs(self.tmp, data)
        exp_fma = m3.expected("fma", data)
        self.assertEqual(Fraction(int(exp_fma[0][0]), int(exp_fma[1][0])), Fraction(0))  # 1/6*2 - 1/3
        for con in CONTENDERS:
            with self.subTest(con=con):
                st, got = run(con, "fma", self.tmp)
                ok, msg = m3.judge(st, got, exp_fma)
                self.assertTrue(ok, msg)

    def test_single_values(self):
        vals = [(1, 6), (1, 3), (-5, 10), (1, 2), (2**62, 3), (1 - 2**62, 3), (1, 2**31 - 1), (2**31 - 1, 1), (0, 5),
                (7, 7), (3, 2**61)]
        v = (np.array([p for p, _ in vals], dtype=np.int64), np.array([q for _, q in vals], dtype=np.int64))
        ia = np.array([0, 2, 4, 6, 8, 9, 0], dtype=np.int32)
        for name, op, five in (("add", 0, 5), ("mul", 1, 10)):  # 2^62/3 + (1 - 2^62)/3 = 1/3; 2^62/3 * 3/2^61 = 2
            ib = np.array([1, 3, five, 7, 9, 9, 0], dtype=np.int32)
            data = {"v": v, "ops": {name: (ia, ib, np.full(len(ia), op, dtype=np.int32))}}
            m3.write_inputs(self.tmp, data)
            exp = m3.expected(name, data)
            for con in ("pg", "mpq", "fmpq"):
                with self.subTest(con=con, op=name):
                    st, got = run(con, name, self.tmp)
                    ok, msg = m3.judge(st, got, exp)
                    self.assertTrue(ok, msg)


class SmallWorkloadsAllContenders(TmpDir):
    def test_every_workload(self):
        W = m3.workloads(scale=0.001)
        for wl, w in W.items():
            data = w["gen"](m3.wrng(3, wl), w["n"])
            m3.check_inputs(data)
            d = self.tmp / wl
            m3.write_inputs(d, data)
            for job in w["jobs"]:
                exp = m3.expected(job, data, w.get("K", 0))
                for row in m3.rows_for(w):
                    if row == "pg-null":
                        continue
                    with self.subTest(wl=wl, job=job, row=row):
                        st, got = run(row, job, d, w.get("K", 0) if job == "chain" else None)
                        ok, msg = m3.judge(st, got, exp)
                        self.assertTrue(ok, msg)


# ---------------------------------------------------------------------------------------------------- 3. generator

class Generator(unittest.TestCase):
    def test_bounds_violation_is_caught(self):
        big = (np.array([2**40], dtype=np.int64), np.array([2**40 + 1], dtype=np.int64))
        data = {"a": big, "b": big, "c": (np.array([1], dtype=np.int64), np.array([1], dtype=np.int64))}
        with self.assertRaises(ValueError):
            m3.expected("fma", data)  # the product's numerator is 2^80

    def test_reuse_axes(self):
        """Axis A: each value used exactly U times, each denominator on exactly 10 values (the pool scales with the
        values).  Axis B: each value used once, each denominator on exactly V values.  pq: partners share a
        denominator, add only, 32-62 bits.  The number of operations is the same along an axis."""
        T = 1500
        for axis, (var, _, grid) in m3.AXES.items():
            for kind in ("mid", "pq"):
                for x in grid:
                    with self.subTest(axis=axis, kind=kind, x=x):
                        g = m3.reuse(m3.wrng(1, f"{axis}{kind}{x}"), T, axis, x, kind)
                        (ia, ib, op), = g["ops"].values()
                        self.assertEqual(len(ia), T)
                        U, vpd = (x, m3.VALUES_PER_DEN_A) if axis == "A" else (1, x)
                        num, den = g["v"]
                        n = len(num)
                        self.assertEqual(n, 2 * T // U)
                        self.assertTrue((np.bincount(np.concatenate([ia, ib]), minlength=n) == U).all())
                        _, per_den = np.unique(den, return_counts=True)
                        self.assertTrue((per_den == vpd).all())
                        self.assertEqual(len(per_den), n // vpd)
                        if kind == "pq":
                            self.assertTrue((den[ia] == den[ib]).all())
                            self.assertTrue((op == 0).all())
                            self.assertTrue(((den >= 2**32) & (den < 2**62)).all())
                        else:
                            self.assertEqual(set(op.tolist()), {0, 1})
                            self.assertTrue(((den >= 2) & (den < 2**20)).all())

    def test_shared_pairs_share(self):
        g = m3.pairs(m3.wrng(1, "sh"), 500, 30, 100)
        ia, ib, _ = g["ops"]["add"]
        self.assertTrue((g["v"][1][ia] == g["v"][1][ib]).all())

    def test_data_independent_of_selection(self):
        W1, W2 = m3.workloads(0.001), m3.workloads(0.001)
        a = W1["MIX90"]["gen"](m3.wrng(5, "MIX90"), 1000)
        b = W2["MIX90"]["gen"](m3.wrng(5, "MIX90"), 1000)
        self.assertTrue(all((a[k][0] == b[k][0]).all() and (a[k][1] == b[k][1]).all() for k in "abc"))

    def test_fz_only_with_a_shared_denominator(self):
        for wl, w in m3.workloads(0.001).items():
            if "fz" in m3.rows_for(w):
                data = w["gen"](m3.wrng(2, wl), w["n"])
                from math import lcm
                D = 1
                for k in "abc":
                    for q in set(data[k][1].tolist()):
                        D = lcm(D, q)
                self.assertLess(D, 2**63, wl)


# ---------------------------------------------------------------------------------------------------- 4. statistics

def row(totals, ok=True):
    return {"ok": ok, "msg": "", "rounds": [{"total": t, "entry": t // 2, "op": t // 2, "exit": 0} for t in totals]}


class Statistics(unittest.TestCase):
    def test_verdict_band(self):
        self.assertEqual(m3.verdict(1.06, 1.2, 0.05), "faster")
        self.assertEqual(m3.verdict(1.04, 1.2, 0.05), "tie")      # overlaps +5%
        self.assertEqual(m3.verdict(0.8, 0.94, 0.05), "slower")
        self.assertEqual(m3.verdict(0.8, 0.96, 0.05), "tie")      # overlaps -5%
        self.assertEqual(m3.verdict(1.1, 1.3, 0.12), "tie")       # inside a null spread wider than 5%

    def test_bootstrap_identical_rows(self):
        p, lo, hi = m3.ratio_ci([10, 11, 12] * 4, [10, 11, 12] * 4, np.random.default_rng(0), 500)
        self.assertEqual((p, lo, hi), (1.0, 1.0, 1.0))

    def test_analyse_best_library_and_na(self):
        rng = np.random.default_rng(1)
        base = (1000 + rng.integers(0, 20, 12)).tolist()
        res = {"workloads": {"X": {"jobs": {"fma": {
            "pg": row(base), "pg-null": row([t + 3 for t in base]),
            "mpq": row([3 * t for t in base]), "fmpq": row([2 * t for t in base]),
            "fz": row([t // 4 for t in base], ok=False)}}}}}
        a = m3.analyse(res, 0, 2000)["X fma"]
        self.assertEqual(a["best"], "fmpq")               # fz is faster but failed a check: not eligible
        self.assertEqual(a["cmp"]["fz"]["verdict"], "n/a")
        self.assertEqual(a["verdict"], "faster")
        self.assertAlmostEqual(a["cmp"]["fmpq"]["point"], 2.0, delta=0.01)
        # M1 failing makes every comparison n/a
        res["workloads"]["X"]["jobs"]["fma"]["pg"]["ok"] = False
        a = m3.analyse(res, 0, 2000)["X fma"]
        self.assertEqual(a["verdict"], "n/a")
        self.assertTrue(all(c["verdict"] == "n/a" for c in a["cmp"].values()))

    def test_noisy_null_widens_the_band(self):
        rng = np.random.default_rng(2)
        base = (1000 + rng.integers(0, 10, 12)).tolist()
        noisy = [t * (1.4 if i % 2 else 0.7) for i, t in enumerate(base)]
        res = {"workloads": {"X": {"jobs": {"sum": {
            "pg": row(base), "pg-null": row(noisy), "mpq": row([int(1.15 * t) for t in base])}}}}}
        a = m3.analyse(res, 0, 2000)["X sum"]
        self.assertGreater(a["h"], 0.15)
        self.assertEqual(a["verdict"], "tie")             # 15% faster, but inside the null spread

    def test_payback_per_axis_and_family(self):
        W, an = {}, {}
        verdicts = {"A-mid": ["slower", "tie", "faster", "faster", "faster"], "B-pq": ["slower"] * 5}
        for fam, vs in verdicts.items():
            for x, v in zip(m3.AXES[fam[0]][2], vs):
                W[f"{fam}-{x}"] = {"family": fam, "x": x}
                an[f"{fam}-{x} add"] = {"wl": f"{fam}-{x}", "verdict": v}
        A = m3.answers({"workloads": W}, an)["payback"]
        self.assertEqual((A["A-mid"]["faster_from"], A["A-mid"]["parity_from"], A["A-mid"]["axis"]), (10, 3, "A"))
        self.assertEqual((A["B-pq"]["faster_from"], A["B-pq"]["parity_from"], A["B-pq"]["axis"]), (None, None, "B"))
        self.assertTrue(A["A-mid"]["complete"] and A["B-pq"]["complete"])

    def test_payback(self):
        P = m3.payback
        self.assertEqual(P([(1, "slower"), (3, "tie"), (10, "faster"), (30, "faster"), (100, "faster")]), (10, 3))
        self.assertEqual(P([(1, "faster"), (3, "faster"), (10, "faster"), (30, "faster"), (100, "faster")]), (1, 1))
        self.assertEqual(P([(1, "slower"), (3, "faster"), (10, "slower"), (30, "faster"), (100, "faster")]), (30, 30))
        self.assertEqual(P([(1, "slower"), (3, "slower"), (10, "slower"), (30, "slower"), (100, "tie")]), (None, 100))
        self.assertEqual(P([(1, "faster"), (3, "faster"), (10, "n/a"), (30, "faster"), (100, "faster")]), (30, 30))
        self.assertEqual(P([(1, "slower")] * 4 + [(100, "slower")]), (None, None))


# ---------------------------------------------------------------------------------------------------- 5. rule guard

class RuleGuard(TmpDir):
    def test_rule_text(self):
        t = m3.rule_text()
        for s in ("12 interleaved rounds", "3 reps", "first rep (cold) is discarded", "10,000",
                  "h = max(5%, null spread)", "{1, 3, 10, 30, 100}", "{2, 6, 20, 60, 200}", "Axis A", "Axis B",
                  "best normal use", "V3", "faster", "slower", "tie", "best library"):
            self.assertIn(s, t)

    def test_measured_run_refuses_without_the_rule(self):
        saved = m3.RULE_FILE
        try:
            m3.RULE_FILE = self.tmp / "missing.md"
            self.assertEqual(m3.main([]), 2)
            m3.RULE_FILE = self.tmp / "other.md"
            m3.RULE_FILE.write_text(m3.rule_text(rounds=5))
            self.assertEqual(m3.main([]), 2)
        finally:
            m3.RULE_FILE = saved

    def test_partial_run_cannot_write_results(self):
        self.assertEqual(m3.main(["--only", "DEC"]), 2)
        self.assertEqual(m3.main(["--scale", "0.1"]), 2)

    def test_revise_rewrites_until_results_exist(self):
        saved = (m3.RULE_FILE, m3.RESULTS)
        try:
            m3.RULE_FILE, m3.RESULTS = self.tmp / "rule.md", self.tmp / "results"
            self.assertEqual(m3.main(["--write-rule"]), 0)
            m3.RULE_FILE.write_text(m3.RULE_FILE.read_text().replace(m3.rule_text(), "an older rule"))
            self.assertEqual(m3.main(["--revise", "revised after pre-run review"]), 0)
            text = m3.RULE_FILE.read_text()
            self.assertIn(m3.rule_text(), text)
            self.assertNotIn("an older rule", text)
            self.assertIn("**Revised", text)
            self.assertIn("revised after pre-run review", text)
            self.assertIn("revised after pre-run review", (m3.RESULTS / "m3.md").read_text())
            (m3.RESULTS / "m3.json").write_text("{}")
            self.assertEqual(m3.main(["--revise", "zz-second-revision"]), 2)
            self.assertNotIn("zz-second-revision", m3.RULE_FILE.read_text())
        finally:
            m3.RULE_FILE, m3.RESULTS = saved

    def test_write_rule_does_not_overwrite(self):
        saved = (m3.RULE_FILE, m3.RESULTS)
        try:
            m3.RULE_FILE, m3.RESULTS = self.tmp / "rule.md", self.tmp / "results"
            self.assertEqual(m3.main(["--write-rule"]), 0)
            text = m3.RULE_FILE.read_text()
            self.assertIn(m3.rule_text(), text)
            self.assertIn(m3.rule_text(), (m3.RESULTS / "m3.md").read_text())
            self.assertEqual(m3.main(["--write-rule"]), 0)
            self.assertEqual(m3.RULE_FILE.read_text(), text)
        finally:
            m3.RULE_FILE, m3.RESULTS = saved


if __name__ == "__main__":
    unittest.main(verbosity=2)
