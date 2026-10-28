"""Correctness tests for round 2 (round2_bench.py): V34, V6 and the 32-bit builds of V0', V1 and V5.

    python3 test_round2.py          (or: python3 -m unittest test_round2 -v)

Every new variant is run through its benchmark binary on random inputs for every job and checked against Python's
fractions, with the strict rules: outputs in lowest terms, overflows only where the 64-bit control V0' also reports
one (64-bit rows) or where the exact value does not fit the 32-bit range (32-bit rows), and each binary's structural
audit.  V6's invariant (pairwise coprime nodes, a consistent dictionary) is audited after random entries, including the
100 and 35 example.  Every check is fed a wrong result to show that it catches one.  The 32-bit build's overflow
detection is tested at the int32 boundaries.
"""
import os
import random
import tempfile
import unittest
from fractions import Fraction
from math import gcd, lcm
from pathlib import Path

import numpy as np
import sympy

import ablation_bench as ab
import pg_bench
import round2_bench as r2

r2.build()
NEW64 = ["v34-lcm", "v34-freq", "v6"]
NEW32 = ["v0p-32", "v1-32", "v5-32"]
JOBS = {"sum": "a", "add": "ab", "mul": "ab", "fma": "abc", "chain": "abc", "fma2": "abcef"}
LIM32 = 2**31

rng0 = random.Random(7)
MID = [int(sympy.nextprime(rng0.randint(2**16, 2**20))) for _ in range(12)]   # primes above 2^16 (leftover parts)
BIG = [int(sympy.nextprime(rng0.randint(2**40, 2**50))) for _ in range(3)]


def rand_frac(rng, kind):
    if kind == "small":
        return rng.randint(-10**6, 10**6), rng.randint(1, 12)
    if kind == "dec":
        return rng.randint(-10**4, 10**4), 1000
    if kind == "money":
        return rng.randint(-10**6, 10**6), 100
    if kind == "unrel":
        return rng.randint(-10**6, 10**6), rng.randint(1, 1000)
    if kind == "int":
        return rng.randint(-10**6, 10**6), 1
    if kind == "mixed90":
        return (rng.randint(-10**7, 10**7), 1000) if rng.random() < 0.9 else (rng.randint(-10**6, 10**6), rng.randint(1, 12))
    if kind == "s15":   # Part B's S30
        return rng.randint(-2**30 + 1, 2**30 - 1), rng.randint(1, 2**15)
    if kind == "leftover":  # V6: products of primes above 2^16 (unknown leftovers that share), times small primes
        d = rng.choice(MID) * rng.choice(MID) * rng.choice([1, 2, 3, 6, 10])
        return rng.randint(-10**6, 10**6) * rng.choice([1, 1, MID[0]]), d
    if kind == "bigprime":  # V6: a prime above 2^16 proven by the adaptive stop, or a 40-50-bit leftover prime
        return rng.randint(-10**4, 10**4), rng.choice([rng.choice(MID) * rng.choice([1, 3, 4]), rng.choice(BIG)])
    if kind == "coarse":  # V6: denominators whose primes are first seen together (multi-prime nodes)
        return rng.choice([1, 7, 11, 13, 49, -1, -77]), rng.choice([6, 6, 6, 36, 10, 15])
    if kind == "s32":   # fits the 32-bit build, products mostly fit
        return rng.randint(-2**12, 2**12), rng.randint(1, 60)
    if kind == "tiny":  # every job's result fits the 32-bit build
        return rng.randint(-1000, 1000), rng.randint(1, 12)
    raise ValueError(kind)


def make_inputs(d, names, cols):
    arrays = {k: (np.array([x for x, _ in cols[k]], dtype=np.int64), np.array([y for _, y in cols[k]], dtype=np.uint64))
              for k in names}
    pg_bench.write_inputs(d, arrays)
    return arrays


def run(v, job, d, K=3, reps=2, **env):
    e = dict(os.environ)
    for k in ("AB_AUDIT", "AB_CORRUPT", "AB_V6_DUMP", "AB_V3_POLICY"):
        e.pop(k, None)
    e.update({k: str(x) for k, x in env.items()})
    out = Path(d) / f"out-{v}-{job}.txt"
    st = r2.run_one(v, job, d, reps, K, out, env=e)
    lines = out.read_text().splitlines() if out.exists() and "unable" not in st else None
    if out.exists():
        out.unlink()
    return st, lines


def ovf_set(lines):
    return {i for i, l in enumerate(lines) if l == pg_bench.OVF_LINE}


def fits32(x):
    return -LIM32 <= x.numerator < LIM32 and x.denominator < LIM32


def path_fits32(job, F, i, K=3):
    """Whether every value the 32-bit build forms for element i fits: the operands, each reduced intermediate, and
    the common denominator of each add (formed before its reduction).  An overflow is genuine iff this is False."""
    vals = []

    def add(x, y):
        vals.append(Fraction(1, lcm(x.denominator, y.denominator)))
        vals.append(x + y)
        return x + y

    def mul(x, y):
        vals.append(x * y)
        return x * y

    if job == "sum":
        acc = Fraction(0)
        for x in F["a"]:
            vals.append(x)
            acc = add(acc, x)
    else:
        o = {k: F[k][i] for k in F}
        vals += list(o.values())
        if job == "add":
            add(o["a"], o["b"])
        elif job == "mul":
            mul(o["a"], o["b"])
        elif job == "fma":
            add(mul(o["a"], o["b"]), o["c"])
        elif job == "chain":
            x = o["a"]
            for _ in range(K):
                x = add(mul(x, o["b"]), o["c"])
        else:
            add(mul(add(mul(o["a"], o["b"]), o["c"]), o["e"]), o["f"])
    return all(fits32(v) for v in vals)


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.d = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()


# ------------------------------------------------------------------------------------------------- random correctness

class TestRandom(Base):
    def run_kind(self, kinds, seed, rows, n=300, allow_ovf=False):
        rng = random.Random(seed)
        cols = {k: [rand_frac(rng, rng.choice(kinds)) for _ in range(n)] for k in "abcef"}
        cols["e"] = [(x % 201 - 100, y) for x, y in cols["e"]]
        arrays = make_inputs(self.d, "abcef", cols)
        cd = self.d / "chain"
        chain_arrays = make_inputs(cd, "abc", dict(cols, b=[(x % 201 - 100, y) for x, y in cols["b"]]))
        for job in JOBS:
            d, arrs = (cd, chain_arrays) if job == "chain" else (self.d, arrays)
            exp = ab.expected(job, arrs, 3)
            st, ref = run("v0p", job, d)
            ref_ovf = ovf_set(ref) if ref else set()
            for v in rows:
                st, lines = run(v, job, d, AB_AUDIT=1)
                where = f"{v} {job} {kinds}"
                if "unable" in st:
                    self.assertEqual(job, "sum", where)
                    if v not in NEW32:  # the 32-bit build may be unable where the 64-bit control is not
                        self.assertIsNone(ref, f"{where}: unable where V0' was able")
                    continue
                self.assertEqual(st.get("audit"), "ok", where)
                ovs = ovf_set(lines)
                ok, msg = (r2.check_rows32 if v in NEW32 else ab.check_rows)(lines, exp, [i for i in range(len(exp)) if i not in ovs])
                self.assertTrue(ok, f"{where}: {msg}")
                self.assertEqual(ab.unreduced_pct(lines), 0.0, f"{where}: unreduced outputs")
                if ovs:
                    self.assertGreater(st.get("ovf"), 0, f"{where}: '0 0' without a counted overflow")
                if v in NEW32:  # each overflow genuine for int32: some value on the element's path does not fit
                    F = {k: pg_bench.fracs(arrs[k]) for k in JOBS[job]}
                    for i in ovs:
                        self.assertFalse(path_fits32(job, F, i), f"{where}: false overflow at {i}")
                    if job in ("add", "mul"):  # one op: an overflow is exactly a result that does not fit
                        for i in range(len(exp)):
                            self.assertEqual(i in ovs, not fits32(exp[i]) or not path_fits32(job, F, i), f"{where} {i}")
                elif job != "sum":
                    self.assertFalse(ovs - ref_ovf, f"{where}: overflowed where V0' did not: {sorted(ovs - ref_ovf)[:5]}")
                if not allow_ovf:
                    self.assertFalse(ovs, f"{where}: overflow on data that fits")

    def test_v34_and_v6(self):
        for kinds, seed in ((["dec"], 10), (["money"], 11), (["small"], 12), (["mixed90"], 13), (["int"], 14),
                            (["small", "dec", "int"], 15)):
            self.run_kind(kinds, seed, NEW64)
        for kinds, seed in ((["unrel"], 16), (["s15"], 17), (["small", "dec", "unrel", "s15", "int"], 18)):
            self.run_kind(kinds, seed, NEW64, allow_ovf=True)

    def test_v6_entries(self):
        """V6 on denominators that exercise the dictionary, the hash, leftovers and multi-prime nodes."""
        for kinds, seed in ((["coarse"], 20), (["leftover"], 21), (["bigprime"], 22),
                            (["coarse", "leftover", "bigprime", "small", "dec"], 23)):
            self.run_kind(kinds, seed, ["v6"], allow_ovf=True)

    def test_32bit(self):
        self.run_kind(["s32"], 30, NEW32, allow_ovf=True)
        self.run_kind(["small"], 31, NEW32, allow_ovf=True)   # numerators up to 10^6: many products overflow int32
        self.run_kind(["dec", "money"], 32, NEW32, allow_ovf=True)


# ------------------------------------------------------------------------------------------------- V6's invariant

def parse_edges(s):
    """'100:5^2*2^2;35:5^1*7^1' -> {100: {5: 2, 2: 2}, 35: {5: 1, 7: 1}}"""
    out = {}
    for item in s.split(";"):
        den, es = item.split(":")
        out[int(den)] = {int(x.split("^")[0]): int(x.split("^")[1]) for x in es.split("*")} if es else {}
    return out


class TestV6(Base):
    def dump(self, cols, job="sum"):
        make_inputs(self.d, JOBS[job], cols)
        st, lines = run("v6", job, self.d, reps=1, AB_V6_DUMP=1, AB_AUDIT=1)
        self.assertEqual(st["audit"], "ok")
        return st, lines

    def test_100_and_35(self):
        """The coarsest rule here: a node holding one prime is that prime, so 100 and 35 end as nodes 2, 5, 7 with
        100 = 2^2 * 5^2 and 35 = 5 * 7 (not node 4), in either order, after one split (5 out of 100's node)."""
        for order in ([100, 35], [35, 100]):
            st, lines = self.dump({"a": [(1, d) for d in order]})
            self.assertEqual(str(st["v6nodes"]), "2,5,7", order)
            E = parse_edges(st["v6edges"])
            self.assertEqual(E[100], {2: 2, 5: 2})
            self.assertEqual(E[35], {5: 1, 7: 1})
            self.assertEqual(st["v6_entry_splits"], 1)
            self.assertEqual(lines, ["27 700"])

    def test_first_sighting_does_not_split(self):
        """One denominator alone stays one node, even with several primes; a second sighting splits only that prime."""
        st, _ = self.dump({"a": [(1, 100), (7, 100), (-3, 100)]})
        self.assertEqual(str(st["v6nodes"]), "100")
        self.assertEqual(st["v6_entry_splits"], 0)
        st, _ = self.dump({"a": [(1, 2 * 9 * 5), (1, 7 * 9)]})  # 3 seen again: 90 -> 3 and 10; 63 -> 3^2 * 7
        self.assertEqual(str(st["v6nodes"]), "3,7,10")
        E = parse_edges(st["v6edges"])
        self.assertEqual(E[90], {3: 2, 10: 1})
        self.assertEqual(E[63], {3: 2, 7: 1})

    def test_reduction_splits_a_coarse_node(self):
        """A numerator sharing part of a multi-prime node splits it, at entry (fin) and in a product (core_mul)."""
        st, lines = self.dump({"a": [(1, 6), (5, 6), (3, 6)]})  # 3/6 = 1/2 splits node 6 at entry
        self.assertEqual(str(st["v6nodes"]), "2,3")
        self.assertEqual(lines, ["3 2"])
        make_inputs(self.d, "ab", {"a": [(1, 6), (5, 6)], "b": [(3, 1), (7, 1)]})
        st, lines = run("v6", "mul", self.d, reps=1, AB_AUDIT=1)
        self.assertEqual(st["audit"], "ok")
        self.assertEqual(lines, ["1 2", "35 6"])
        self.assertGreaterEqual(st["v6_op_splits"], 1)
        # a node of two primes above 2^16 (found by rho, kept together) split by a numerator sharing one of them
        p, q = MID[0], MID[1]
        st, lines = self.dump({"a": [(1, p * q), (p, p * q)]})
        self.assertEqual(lines, [str(Fraction(1, p * q) + Fraction(1, q)).replace("/", " ")])

    def test_big_primes_via_pg_factor(self):
        """p*q (both above 2^16: pg_factor's rho finds them; one node of two primes, in the hash), then 3p or 5q^2:
        the prime seen again is split out."""
        p, q = MID[2], MID[3]
        for order in ([p * q, 3 * p], [3 * p, p * q], [p * q, q * q * 5, 3 * p]):
            st, lines = self.dump({"a": [(1, d) for d in order]})
            self.assertTrue({str(p), str(q)} <= set(str(st["v6nodes"]).split(",")), (order, st["v6nodes"]))
            exp = sum(Fraction(1, d) for d in order)
            self.assertEqual(lines, [f"{exp.numerator} {exp.denominator}"])

    def test_invariant_after_random_entries(self):
        """Random entries of every kind; the audit checks pairwise coprime nodes, the dictionary, and every edge."""
        kinds = ["small", "unrel", "coarse", "leftover", "bigprime", "dec", "s15"]
        for seed in range(8):
            rng = random.Random(100 + seed)
            cols = {k: [rand_frac(rng, rng.choice(kinds)) for _ in range(400)] for k in "abc"}
            arrays = make_inputs(self.d, "abc", cols)
            for job in ("fma", "add"):
                st, lines = run("v6", job, self.d, reps=2, AB_AUDIT=1)
                self.assertEqual(st["audit"], "ok", (seed, job))
                exp = ab.expected(job, arrays)
                ovs = ovf_set(lines)
                self.assertTrue(ab.check_rows(lines, exp, [i for i in range(len(exp)) if i not in ovs])[0], (seed, job))
                self.assertEqual(ab.unreduced_pct(lines), 0.0)
            # every distinct input denominator is a power product of the live nodes, which are pairwise coprime
            st, _ = run("v6", "fma", self.d, reps=1, AB_V6_DUMP=1)
            nodes = [int(x) for x in str(st["v6nodes"]).split(",")]
            self.assertTrue(all(gcd(x, y) == 1 for i, x in enumerate(nodes) for y in nodes[i + 1:]), seed)

    def test_control_code_when_dictionary_idle(self):
        """Hooked copies run only while a node holds several primes (or after a split in the ops): on decimals the
        one fill-time split leaves prime nodes and the ops run the control's code; a lasting two-prime node keeps
        them hooked."""
        rng = random.Random(6)
        make_inputs(self.d, "abc", {k: [rand_frac(rng, "dec") for _ in range(2000)] for k in "abc"})
        st, _ = run("v6", "fma", self.d, reps=1)
        self.assertEqual(st["v6_multi"], 0)
        self.assertLess(st["v6_hooked"], 10)  # only up to the first split at entry
        p, q = MID[4], MID[5]
        make_inputs(self.d, "abc", {k: [(rng.choice([1, 3, -7]), p * q) for _ in range(500)] for k in "abc"})
        st, lines = run("v6", "fma", self.d, reps=1, AB_AUDIT=1)
        self.assertEqual((st["v6_multi"], st["audit"]), (1, "ok"))
        self.assertGreater(st["v6_hooked"], 1000)

    def test_s15_fits_int32(self):
        """Part B's S15: every add and mul of two operands fits the 32-bit build."""
        w = r2.sizes(True)["S15"]
        arrays = w["gen"](np.random.default_rng(3), 20000)
        for job in ("add", "mul"):
            self.assertTrue(all(fits32(x) for x in ab.expected(job, arrays)), job)

    def test_v6_matches_v0p_nodes_when_primes(self):
        """On small denominators every node ends prime, the same graph as V0' (W3-like data)."""
        rng = random.Random(5)
        make_inputs(self.d, "abc", {k: [rand_frac(rng, "small") for _ in range(500)] for k in "abc"})
        st6, l6 = run("v6", "fma", self.d, reps=1, AB_V6_DUMP=1)
        st0, l0 = run("v0p", "fma", self.d, reps=1)
        self.assertEqual(str(st6["v6nodes"]), "2,3,5,7,11")
        self.assertEqual(st6["v6_nodes"], st0["nodes"])
        self.assertEqual(l6, l0)


# ------------------------------------------------------------------------------------------------- checks catch wrong

class TestChecksCatchWrong(Base):
    def test_binary_corruption_caught(self):
        rng = random.Random(1)
        cols = {k: [rand_frac(rng, "tiny") for _ in range(200)] for k in "abc"}  # fits 32 bits: no overflow at all
        arrays = make_inputs(self.d, "abc", cols)
        exp = ab.expected("fma", arrays)
        for v in NEW64 + NEW32:
            judge = lambda lines: r2.judge(v, lines, exp, None, True)
            st, lines = run(v, "fma", self.d)
            ok, msg, _ = judge(lines)
            self.assertTrue(ok, f"{v}: {msg}")
            self.assertFalse(ovf_set(lines), v)
            st, bad = run(v, "fma", self.d, AB_CORRUPT="value")
            self.assertNotEqual(bad, lines)
            self.assertFalse(judge(bad)[0], f"{v}: a wrong value passed")
            st, bad = run(v, "fma", self.d, AB_CORRUPT="unreduced")
            ok, msg, _ = judge(bad)
            self.assertFalse(ok, f"{v}: an unreduced output passed")

    def test_audits_catch_corruption(self):
        rng = random.Random(2)
        cols = {k: [rand_frac(rng, rng.choice(["unrel", "small", "coarse", "leftover"])) for _ in range(300)] for k in "abc"}
        make_inputs(self.d, "abc", cols)
        tiny = self.d / "tiny"  # the 32-bit builds: data whose results fit, so the first output is not an overflow
        make_inputs(tiny, "abc", {k: [rand_frac(rng, "tiny") for _ in range(300)] for k in "abc"})
        cases = [("v34-lcm", "xi"), ("v34-freq", "xi"), ("v34-lcm", "coprime"), ("v6", "den"), ("v6", "coprime"), ("v6", "dict"),
                 ("v0p-32", "den"), ("v1-32", "den"), ("v5-32", "rc")]
        for v, how in cases:
            d = tiny if v in NEW32 else self.d
            st, _ = run(v, "fma", d, AB_AUDIT=1)
            self.assertEqual(st.get("audit"), "ok", v)
            st, _ = run(v, "fma", d, AB_AUDIT=1, AB_CORRUPT=how)
            self.assertTrue(str(st.get("audit")).startswith("FAIL"), f"{v}: the audit missed corruption '{how}'")

    def test_check32(self):
        exp = [Fraction(1, 3), Fraction(-5, 2), Fraction(2**40), Fraction(-2**31)]
        good = ["1 3", "-5 2", "0 0", f"{-2**31} 1"]
        ov = {}
        self.assertFalse(r2.check_rows32(good, exp, None, ov)[0])  # 1 of 4 overflowed: over 0.1%
        self.assertEqual(ov, {"n": 1, "fit": 0})
        self.assertTrue(r2.check_rows32(good, exp, [0, 1, 3], {})[0])
        self.assertFalse(r2.check_rows32(["1 3", "-5 2", "0 0", "0 0"], exp, [0, 1, 3], {})[0])     # -2^31 fits: false
        self.assertFalse(r2.check_rows32(["1 3", "-4 2", "0 0", f"{-2**31} 1"], exp, [0, 1, 3], {})[0])  # wrong value
        exp2 = [Fraction(2**31)]
        self.assertFalse(r2.check_rows32([f"{2**31} 1"], exp2, None, {})[0])                     # outside int32
        many = [Fraction(k, 7) for k in range(3000)] + [Fraction(2**33)]
        self.assertTrue(r2.check_rows32([f"{k} 7" for k in range(3000)] + ["0 0"], many, None, {})[0])  # one genuine
        ok, msg, extra = r2.judge("v1-32", ["2 6"], [Fraction(1, 3)], None, True)
        self.assertFalse(ok)
        self.assertIn("lowest terms", msg)

    def test_report(self):
        row = lambda ok, t, **last: {"median": {"entry": 1e6, "decomp": 0, "op": t, "red": 0, "total": t + 1e6},
                                     "ok": ok, "msg": "" if ok else "x", "unreduced": 0.0, "last": last, "out_dens": 1}
        cfg = {"date": "", "cpu": "", "rounds": 1, "reps": 1}
        res = {"workloads": {
            "W2": {"desc": "d", "n": 1, "K": None, "jobs": {"fma": {
                "v0p": row(True, 3e6, nodes=2), "v0p-rerun": row(True, 3.2e6), "v3-lcm": row(True, 1e6),
                "v34-lcm": row(True, 0.5e6), "v6": row(False, 1e6)}}},
            "S20": {"desc": "s", "n": 4, "K": None, "jobs": {"mul": {
                "v0p": row(True, 3e6), "v1-32": dict(row(False, 1e6), ovf_elems={"n": 2, "fit": 0})}}}}}
        md = r2.report_md(res, cfg).splitlines()
        self.assertTrue(any("v0p-rerun is the control's binary" in l and "0.95 to 0.95" in l for l in md))
        a = next(l for l in md if l.startswith("| W2 fma |"))
        cells = [c.strip() for c in a.split("|")]
        self.assertEqual(cells[2], "2.00")   # V3 lcm: 4/2
        self.assertEqual(cells[5], "2.67")   # V34 lcm: 4/1.5
        self.assertEqual(cells[7], "1.33")   # V3 / V34 (lcm): 2/1.5
        c = next(l for l in md if l.startswith("| W2 fma | fail | fail |"))  # failed V6 row: labelled in Part C
        self.assertIn("| S20 mul | not run | not run | not run | not run | fail | not run |", md)  # Part B ratios
        self.assertIn("| S20 mul | not run | fail | fail | not run | not run | not run / 50.00% / not run |", md)
        rows = [l for l in md if l.startswith("| v")]
        self.assertTrue(rows[0].startswith("| v0p |"))
        self.assertIn("| fail | fail |", next(l for l in rows if l.startswith("| v6 ")))
        res["workloads"]["W2"]["jobs"]["fma"]["v0p"] = {"unable": "overflow"}  # the control unable: labelled so
        md = r2.report_md(res, cfg).splitlines()
        self.assertTrue(next(l for l in md if l.startswith("| W2 fma |")).startswith("| W2 fma | unable | not run | not run | unable | not run | 1.33 |"))


# ------------------------------------------------------------------------------------------------- the 32-bit build

class Test32Overflow(Base):
    def case(self, job, pairs):
        cols = {"a": [p[0] for p in pairs], "b": [p[1] for p in pairs]}
        arrays = make_inputs(self.d, "ab", cols)
        exp = ab.expected(job, arrays)
        out = {}
        for v in NEW32:
            st, lines = run(v, job, self.d, AB_AUDIT=1)
            self.assertEqual(st["audit"], "ok", v)
            self.assertEqual(st["width"], 32, v)
            ovs = ovf_set(lines)
            for i, e in enumerate(exp):
                fits = -LIM32 <= e.numerator < LIM32 and e.denominator < LIM32
                if fits:
                    self.assertNotIn(i, ovs, f"{v} {job}: false overflow at {i} ({e})")
                    p, q = map(int, lines[i].split())
                    self.assertEqual(Fraction(p, q), e, f"{v} {job} {i}")
                    self.assertEqual(gcd(p, q), 1)
                else:
                    self.assertIn(i, ovs, f"{v} {job}: missed overflow at {i} ({e})")
            self.assertEqual(st["ovf"], len(ovs), v)
            out[v] = lines
        return exp, out

    def test_mul_boundaries(self):
        self.case("mul", [
            ((46340, 1), (46340, 1)),        # 2147395600: fits
            ((46341, 1), (46341, 1)),        # 2147488281: overflows
            ((-65536, 1), (32768, 1)),       # -2^31: fits (INT32_MIN)
            ((65536, 1), (32768, 1)),        # 2^31: overflows
            ((2**20, 3), (3, 2**20 + 1)),    # cross-cancelled: 2^20 / (2^20 + 1) fits
            ((1, 46341), (1, 46341)),        # denominator 2147488281: overflows
            ((1, 46340), (-1, 46340)),       # denominator 2147395600: fits
            ((2**30, 7), (2**30, 2**30)),    # 2^30/7 * 1: fits after cancellation
            ((0, 5), (2**30, 1)),
        ])

    def test_add_boundaries(self):
        self.case("add", [
            ((2**30, 1), (2**30 - 1, 1)),    # 2^31 - 1: fits
            ((2**30, 1), (2**30, 1)),        # 2^31: overflows
            ((-2**30, 1), (-2**30, 1)),      # -2^31: fits
            ((1, 65536), (1, 32768)),        # 3/65536: fits
            ((1, 46337), (1, 46349)),        # coprime primes: denominator above 2^31, overflows
            ((2**30, 3), (2**30, 6)),        # 2^29: the sum reduces back into range
            ((2**31 - 1, 2), (1, 2)),        # 2^30: fits
        ])

    def test_entry_outside_int32(self):
        """Inputs outside the 32-bit range are overflows at entry, never wrapped."""
        cols = {"a": [(2**31, 1), (5, 2**31), (-2**31, 1), (3, 2**31 - 1)], "b": [(1, 1)] * 4}
        arrays = make_inputs(self.d, "ab", cols)
        exp = ab.expected("mul", arrays)
        for v in NEW32:
            st, lines = run(v, "mul", self.d)
            self.assertEqual(lines[0], "0 0", v)
            self.assertEqual(lines[1], "0 0", v)
            self.assertEqual(lines[2], f"{-2**31} 1", v)
            self.assertEqual(lines[3], f"3 {2**31 - 1}", v)
            self.assertGreaterEqual(st["ovf"], 2, v)

    def test_matches_64bit_where_it_fits(self):
        rng = random.Random(9)
        cols = {k: [rand_frac(rng, "small") for _ in range(2000)] for k in "ab"}
        make_inputs(self.d, "ab", cols)
        for job in ("add", "mul"):
            _, ref = run("v0p", job, self.d)
            for v in NEW32:
                _, lines = run(v, job, self.d)
                diff = [i for i, (x, y) in enumerate(zip(lines, ref)) if x != y and x != pg_bench.OVF_LINE]
                self.assertFalse(diff, f"{v} {job}: differs from the 64-bit control at {diff[:5]}")


# ------------------------------------------------------------------------------------------------- V34 mechanics

class TestV34(Base):
    def test_exceptions_deferred_and_retries(self):
        """Exceptions stay unreduced until the final phase (the audit allows it, the outputs are reduced); overflowing
        unreduced graph ops are reduced and retried, never failed; outputs equal V3's."""
        rng = random.Random(41)
        cols = {}
        for k in "abc":
            col = []
            for _ in range(600):
                t = rng.random()
                if t < 0.6:
                    col.append((rng.randint(-10**7, 10**7), 1000))
                elif t < 0.85 or k == "b":
                    col.append((rng.randint(-10**6, 10**6) * 2**20, rng.choice([7, 11, 13, 641]) * 2**20))  # unreduced
                else:
                    col.append((rng.randint(2**29, 2**30), rng.choice([1, 2, 5])))
            cols[k] = col
        arrays = make_inputs(self.d, "abc", cols)
        for rule in ("lcm", "freq"):
            for job in ("fma", "sum", "chain"):
                exp = ab.expected(job, arrays, 3)
                st, lines = run(f"v34-{rule}", job, self.d, AB_AUDIT=1)
                if "unable" in st:  # a column sum beyond 64 bits: V3 must agree
                    self.assertIn("unable", run(f"v3-{rule}", job, self.d)[0], (rule, job))
                    continue
                self.assertEqual(st["audit"], "ok", (rule, job))
                self.assertEqual(st["policy"], rule)
                ovs = ovf_set(lines)
                self.assertTrue(ab.check_rows(lines, exp, [i for i in range(len(exp)) if i not in ovs])[0], (rule, job))
                self.assertEqual(ab.unreduced_pct(lines), 0.0, (rule, job))
                _, ref = run(f"v3-{rule}", job, self.d)
                self.assertEqual([l for i, l in enumerate(lines) if l != pg_bench.OVF_LINE],
                                 [l for i, l in enumerate(ref) if lines[i] != pg_bench.OVF_LINE], (rule, job))
                self.assertFalse(ovs - ovf_set(ref), (rule, job))
                if job == "fma":
                    self.assertGreater(st["exc_in"], 0, rule)
                    self.assertGreater(st["early"], 0, rule)


if __name__ == "__main__":
    unittest.main(verbosity=2)
