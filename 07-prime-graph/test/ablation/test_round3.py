"""Correctness tests for round 3 (round3_bench.py): D0-D3 on V3's layout, R1, and the harness's statistics.

    python3 test_round3.py          (or: python3 -m unittest test_round3 -v)

Every D row (both rules for D) runs through its binary on random inputs for every job and must give exactly R0's
(V3's) outputs, checked against Python's fractions with the strict rules (lowest terms; overflows only where R0
overflows) and with the coprime / dictionary / structure audit.  Targeted inputs drive the glue that re-expands edges
after a split (at the fill, in an op, in the final reduction, in a column sum), D1's block test at its boundaries
(same graph as D0), D2's leftover nodes and idle pass, and D3's product forest (same graph as D0).  Wrong results and
corrupted structures must be caught.  The existing rows must compile to the same machine code as at commit REF (the
round-2 sources), and the harness's bootstrap, verdicts, workloads and report are checked on synthetic data.
"""
import os
import random
import shutil
import subprocess
import tempfile
import unittest
from fractions import Fraction
from math import gcd
from pathlib import Path

import numpy as np
import sympy

import ablation_bench as ab
import pg_bench
import round2_bench as r2
import round3_bench as r3

r3.build()
D_ROWS = [f"d{k}-{p}" for p in r3.RULES for k in range(4)]
JOBS = {"sum": "a", "add": "ab", "mul": "ab", "fma": "abc", "chain": "abc", "fma2": "abcef"}
REF = "97f4cde"  # the commit with round 2's sources (before any round-3 edit)

rng0 = random.Random(11)
MID = [int(sympy.nextprime(rng0.randint(2**17, 2**20))) for _ in range(16)]   # large primes, products >= 2^34
BIG = [int(sympy.nextprime(rng0.randint(2**40, 2**50))) for _ in range(3)]


def rand_frac(rng, kind):
    if kind == "small":
        return rng.randint(-10**6, 10**6), rng.randint(1, 12)
    if kind == "dec":
        return rng.randint(-10**4, 10**4), 1000
    if kind == "unrel":
        return rng.randint(-10**6, 10**6), rng.randint(1, 1000)
    if kind == "int":
        return rng.randint(-10**6, 10**6), 1
    if kind == "coarse":    # primes first seen together: multi-prime nodes
        return rng.choice([1, 7, 11, 13, 49, -1, -77]), rng.choice([6, 6, 6, 36, 10, 15])
    if kind == "leftover":  # two large primes (a leftover of 2^34 or more), times small primes
        return rng.randint(-10**4, 10**4) * rng.choice([1, 1, MID[0]]), rng.choice(MID) * rng.choice(MID) * rng.choice([1, 2, 3])
    if kind == "bigprime":
        return rng.randint(-10**4, 10**4), rng.choice([rng.choice(MID) * rng.choice([1, 3, 4]), rng.choice(BIG)])
    if kind == "s15":
        return rng.randint(-2**30 + 1, 2**30 - 1), rng.randint(1, 2**15)
    raise ValueError(kind)


def make_inputs(d, names, cols):
    arrays = {k: (np.array([x for x, _ in cols[k]], dtype=np.int64), np.array([y for _, y in cols[k]], dtype=np.uint64))
              for k in names}
    pg_bench.write_inputs(Path(d), arrays)
    return arrays


def run(v, job, d, K=3, reps=1, **env):
    e = dict(os.environ)
    for k in ("AB_AUDIT", "AB_CORRUPT", "AB_V6_DUMP", "AB_V3_POLICY"):
        e.pop(k, None)
    e.update({k: str(x) for k, x in env.items()})
    out = Path(d) / f"out-{v}-{job}.txt"
    st = r3.run_one(v, job, d, reps, K, out, env=e)
    lines = out.read_text().splitlines() if out.exists() and "unable" not in st else None
    if out.exists():
        out.unlink()
    return st, lines


def ovf_set(lines):
    return {i for i, l in enumerate(lines) if l == pg_bench.OVF_LINE}


def frac_line(x):
    return f"{x.numerator} {x.denominator}"


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.d = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def check(self, v, job, arrays, K=3, reps=2, **env):
        """v's outputs equal the fractions (outside overflows), in lowest terms, audit ok; returns (stats, lines)."""
        st, lines = run(v, job, self.d, K=K, reps=reps, AB_AUDIT=1, **env)
        if "unable" in st:
            return st, None
        exp = ab.expected(job, arrays, K)
        ovs = ovf_set(lines)
        ok, msg = ab.check_rows(lines, exp, [i for i in range(len(exp)) if i not in ovs])
        self.assertTrue(ok, f"{v} {job}: {msg}")
        self.assertEqual(ab.unreduced_pct(lines), 0.0, f"{v} {job}: unreduced outputs")
        self.assertEqual(st.get("audit"), "ok", f"{v} {job}")
        return st, lines


# ------------------------------------------------------------------------------------------------- random correctness

class TestRandom(Base):
    def run_kind(self, kinds, seed, n=300):
        rng = random.Random(seed)
        cols = {k: [rand_frac(rng, rng.choice(kinds)) for _ in range(n)] for k in "abcef"}
        cols["e"] = [(x % 201 - 100, y) for x, y in cols["e"]]
        arrays = make_inputs(self.d, "abcef", cols)
        cd = self.d / "chain"
        chain_arrays = make_inputs(cd, "abc", dict(cols, b=[(x % 201 - 100, y) for x, y in cols["b"]]))
        for job in JOBS:
            d, arrs = (cd, chain_arrays) if job == "chain" else (self.d, arrays)
            single = job in ("add", "mul")
            for rule in (("plain",) if single else r3.RULES):
                ref_st, ref = run(r3.row_name("r0", rule), job, d)
                for base in ("r1", "d0", "d1", "d2", "d3"):
                    v = r3.row_name(base, rule)
                    where = f"{v} {job} {kinds}"
                    st, lines = run(v, job, d, reps=2, AB_AUDIT=1)
                    if "unable" in st or ref is None:
                        self.assertEqual("unable" in st, ref is None, f"{where}: unable differs from R0")
                        continue
                    exp = ab.expected(job, arrs, 3)
                    ovs = ovf_set(lines)
                    ok, msg = ab.check_rows(lines, exp, [i for i in range(len(exp)) if i not in ovs])
                    self.assertTrue(ok, f"{where}: {msg}")
                    self.assertEqual(ab.unreduced_pct(lines), 0.0, where)
                    self.assertEqual(st.get("audit"), "ok", where)
                    if base == "r1":  # V34 never overflows where V3 would not
                        self.assertFalse(ovs - ovf_set(ref), where)
                    else:             # D rows run V3's ops: the same outputs, overflows included
                        self.assertEqual(lines, ref, f"{where}: outputs differ from R0")
                    if ovs:
                        self.assertGreater(st.get("ovf"), 0, where)

    def test_fitting_kinds(self):
        for kinds, seed in ((["dec"], 1), (["small"], 2), (["int", "small", "dec"], 3), (["coarse"], 4)):
            self.run_kind(kinds, seed)

    def test_dictionary_kinds(self):
        for kinds, seed in ((["leftover"], 5), (["bigprime"], 6), (["unrel"], 7), (["s15"], 8),
                            (["coarse", "leftover", "bigprime", "small", "dec"], 9)):
            self.run_kind(kinds, seed)


# ------------------------------------------------------------------------------------------------- the V3-layout glue

class TestSplitGlue(Base):
    """Node 6 is first seen alone, so it is one node holding 2 and 3; a numerator sharing 2 or 3 splits it.  Each case
    puts the split where an array's edges were already written, so the re-expansion (d_fresh) must run."""

    def rows(self, rule):
        return [f"d{k}-{rule}" for k in range(4)]

    def test_split_at_fill(self):
        """freq: D = 7; 5/6 and 3/6 are exceptions, reduced at entry: 3/6 splits node 6 after 5/6's edges named it."""
        arrays = make_inputs(self.d, "a", {"a": [(1, 7)] * 5 + [(5, 6), (3, 6)]})
        for v in self.rows("freq"):
            st, lines = self.check(v, "sum", arrays, reps=1)
            self.assertEqual(lines, [frac_line(Fraction(5, 7) + Fraction(5, 6) + Fraction(1, 2))])
            self.assertGreaterEqual(st["v6_entry_splits"], 1, v)
            self.assertGreaterEqual(st["d_fresh"], 1, v)

    def test_split_in_op_then_reuse(self):
        """freq: R's exception 5/6 names node 6, then 1/6 * 3 splits it inside the op; the final reduction and the
        chain's next step re-expand."""
        cols = {"a": [(1, 7)] * 5 + [(5, 6), (1, 6)], "b": [(1, 1)] * 6 + [(3, 1)], "c": [(0, 1)] * 7}
        arrays = make_inputs(self.d, "abc", cols)
        for v in self.rows("freq"):
            for job in ("fma", "chain"):
                st, lines = self.check(v, job, arrays, K=3, reps=1)
                self.assertGreaterEqual(st["v6_op_splits"], 1, (v, job))
                self.assertGreaterEqual(st["d_fresh"], 1, (v, job))

    def test_split_in_final(self):
        """lcm: every element dense over D = 6 (not reduced at entry); the final reduction of 3/6 splits node 6 and
        the later elements are reduced over the re-expanded D."""
        cols = {"a": [(1, 6), (3, 6), (1, 6), (2, 6), (5, 6)], "b": [(1, 1)] * 5, "c": [(0, 1)] * 5}
        arrays = make_inputs(self.d, "abc", cols)
        for v in self.rows("lcm"):
            st, lines = self.check(v, "fma", arrays, reps=1)
            self.assertEqual(lines, ["1 6", "1 2", "1 6", "1 3", "5 6"])
            self.assertGreaterEqual(st["v6_op_splits"], 1, v)
            self.assertGreaterEqual(st["d_fresh"], 1, v)

    def test_split_in_column_sum(self):
        """lcm: D = 42; (2^62 + 1)/6 and -(2^62 - 3)/6 overflow when scaled, so they are exceptions (coprime to 6: no
        split at entry); their sum 4/6 splits node 6, so the dense part's edges (D's) are re-expanded before they
        combine."""
        x, y = 2**62 + 1, -(2**62 - 3)
        arrays = make_inputs(self.d, "a", {"a": [(1, 7), (1, 6), (x, 6), (y, 6)]})
        for v in self.rows("lcm"):
            st, lines = self.check(v, "sum", arrays, reps=1)
            self.assertEqual(lines, [frac_line(Fraction(1, 7) + Fraction(1, 6) + Fraction(x + y, 6))])
            self.assertEqual(st["exc_in"], 2, v)
            self.assertGreaterEqual(st["v6_op_splits"], 1, v)
            self.assertGreaterEqual(st["d_fresh"], 1, v)


# ------------------------------------------------------------------------------------------------- D1, D2, D3

def dump(test, v, job, cols):
    make_inputs(test.d, JOBS[job], cols)
    st, lines = run(v, job, test.d, reps=1, AB_V6_DUMP=1, AB_AUDIT=1, AB_V3_POLICY="lcm")
    test.assertEqual(st["audit"], "ok", v)
    return st, lines


class TestD1(Base):
    def dens(self):
        P = [65521, 65519, 65497, 251, 257, 3, 5, 7, 53, 59, 61]
        L = [65521 * 65519, 65521**2, 65519 * 65521 * 3, 3**39, 2**62, 4294967291, 4294967311, 4294967291 * 3,
             251 * 257, 251**2 * 257, 59 * 61 * 65521, 2 * 3 * 5 * 7 * 11 * 13 * 17 * 19 * 23 * 29 * 31 * 37 * 41 * 43,
             MID[0] * MID[1], MID[2]**2, MID[3] * 65521, BIG[0], 1]
        sp = list(sympy.primerange(3, 65536))
        for b in range(0, len(sp) - 9, 97):  # products around block boundaries: the stop is checked once per block
            L += [sp[b] * sp[b + 7], sp[b + 7] * sp[b + 8], sp[b]**2, sp[b + 8]**2 * 2]
        return P + L

    def test_same_graph_as_d0(self):
        """D1 finds the same primes in the same order as D0, so its graph (nodes and edges) and outputs are D0's."""
        dens = self.dens()
        rng = random.Random(3)
        for trial in range(4):
            order = dens[:] if trial == 0 else rng.sample(dens, len(dens))
            cols = {"a": [(rng.randint(-50, 50) or 1, d) for d in order], "b": [(1, 1)] * len(order),
                    "c": [(0, 1)] * len(order)}
            s0, l0 = dump(self, "d0-lcm", "fma", cols)
            s1, l1 = dump(self, "d1-lcm", "fma", cols)
            self.assertEqual(s1["v6nodes"], s0["v6nodes"], trial)
            self.assertEqual(s1["v6edges"], s0["v6edges"], trial)
            self.assertEqual(l1, l0, trial)
            arrays = make_inputs(self.d, "abc", cols)
            self.check("d1-freq", "fma", arrays, reps=1)

    def test_random_full_scans(self):
        """40-bit denominators with no factor below 2^16 (full scans, Miller-Rabin, rho) and random 31-bit ones."""
        rng = random.Random(4)
        dens = [int(sympy.nextprime(rng.randint(2**20, 2**28))) * int(sympy.nextprime(rng.randint(2**17, 2**20)))
                for _ in range(40)] + [rng.randint(2, 2**31) for _ in range(200)]
        cols = {"a": [(rng.randint(-99, 99) or 1, d) for d in dens], "b": [(1, 1)] * len(dens), "c": [(1, 1)] * len(dens)}
        s0, l0 = dump(self, "d0-lcm", "fma", cols)
        s1, l1 = dump(self, "d1-lcm", "fma", cols)
        self.assertEqual((s1["v6nodes"], s1["v6edges"], l1), (s0["v6nodes"], s0["v6edges"], l0))


class TestD2(Base):
    def run_dens(self, dens, expect=None):
        rng = random.Random(len(dens))
        cols = {"a": [(rng.randint(-9, 9) or 1, d) for d in dens], "b": [(rng.randint(-9, 9) or 1, d) for d in reversed(dens)]}
        arrays = make_inputs(self.d, "ab", cols)
        st, lines = self.check("d2", "add", arrays, reps=2)
        s0, l0 = run("d0", "add", self.d)
        self.assertEqual(lines, l0)
        for k, x in (expect or {}).items():
            self.assertEqual(st[k], x, (k, dens))
        return st

    def test_leftovers(self):
        p, q, r, s = MID[:4]
        P40 = int(sympy.nextprime(2**40 + 12345))
        # a leftover split by a later leftover's gcd: pq, then pr (p unknown until then); the parts p, q, r are
        # below 2^32, so they are prime nodes, not leftovers, and nothing is left for the idle pass
        self.run_dens([p * q, p * r], {"d2_leftovers": 1, "d2_leftover_splits": 1, "d2_idle_factored": 0})
        # a first-sighted large prime below 2^32 (7p: p is what the table leaves) splits the leftover pq
        self.run_dens([p * q, 7 * p], {"d2_leftovers": 1, "d2_leftover_splits": 1, "d2_idle_factored": 0})
        # p^2: the idle pass makes node p with weight 2; a 40-bit prime: a prime node; pq alone: one node of two primes
        st = self.run_dens([p * p, P40, q * r], {"d2_leftovers": 3, "d2_idle_factored": 3})
        self.assertEqual(st["v6_multi"], 1)
        self.assertGreater(st["decomp"], 0)  # the idle pass is timed in the decomposition slot
        # c sharing with two leftovers, and three large primes in one leftover
        self.run_dens([p * q, r * s, p * r, MID[4] * MID[5] * MID[6]], {"d2_leftover_splits": 2})
        # small primes with a leftover: D0 groups 6pq into one node; D2 keeps {2, 3} and the leftover apart
        x, y = MID[7], MID[8]
        self.run_dens([6 * x * y, 5])
        make_inputs(self.d, "ab", {"a": [(1, 6 * x * y), (1, 5)], "b": [(7, 5), (-1, 6 * x * y)]})  # no cancellation
        nodes = lambda v: sorted(int(t) for t in str(run(v, "add", self.d, AB_V6_DUMP=1)[0]["v6nodes"]).split(","))
        self.assertEqual(nodes("d0"), sorted([5, 6 * x * y]))   # the graph after entry
        self.assertEqual(nodes("d2"), sorted([5, 6, x * y]))

    def test_random_leftovers(self):
        rng = random.Random(8)
        for trial in range(5):
            dens = [rng.choice(MID) * rng.choice(MID) * rng.choice([1, 1, 2, 3, 5]) for _ in range(60)]
            dens += [rng.choice(MID) * rng.choice([7, 11, 1]) for _ in range(10)] + [rng.choice(BIG) for _ in range(3)]
            rng.shuffle(dens)
            st = self.run_dens(dens)
            self.assertGreater(st["d2_leftovers"], 0, trial)


class TestD3(Base):
    def test_same_graph_as_d0(self):
        """Hundreds of known large primes (several roots of up to 64 leaves), then denominators made of them: the
        product check finds exactly the primes D0's loop finds, so the graph and outputs are D0's."""
        rng = random.Random(9)
        ps = sorted({int(sympy.nextprime(rng.randint(2**17, 2**22))) for _ in range(300)})
        new = [int(sympy.nextprime(rng.randint(2**23, 2**26))) for _ in range(20)]
        dens = list(ps)  # each below 2^32: a prime after the table, a leaf of the forest
        dens += [rng.choice(ps) * rng.choice(ps) for _ in range(150)]                      # hits: both known
        dens += [rng.choice(ps) * rng.choice(new) for _ in range(60)]                      # one known, one new
        dens += [rng.choice(ps) * rng.choice(ps) * rng.choice(ps) for _ in range(40)]      # three known (2^51-2^66)
        dens = [d for d in dens if d < 2**63]
        dens += [rng.choice(new) * rng.choice(new) for _ in range(20)] + BIG                # misses: rho, MR
        dens += [rng.choice(ps)**2 * 3 for _ in range(10)]
        cols = {"a": [(rng.randint(-9, 9) or 1, d) for d in dens], "b": [(1, 1)] * len(dens), "c": [(0, 1)] * len(dens)}
        s0, l0 = dump(self, "d0-lcm", "fma", cols)
        s3, l3 = dump(self, "d3-lcm", "fma", cols)
        self.assertEqual((s3["v6nodes"], s3["v6edges"], l3), (s0["v6nodes"], s0["v6edges"], l0))
        self.assertGreater(s3["d3_hits"], 150)
        self.assertGreater(s3["d3_checks"], s3["d3_hits"])  # some misses
        self.assertGreater(s3["d3_roots"], 3)
        self.assertGreaterEqual(s3["d3_leaves"], 300)
        arrays = make_inputs(self.d, "abc", cols)
        for v in ("d3-lcm", "d3-freq"):
            self.check(v, "fma", arrays, reps=1)


# ------------------------------------------------------------------------------------------------- checks catch wrong

class TestChecksCatchWrong(Base):
    def test_binary_corruption_caught(self):
        rng = random.Random(1)
        cols = {k: [(rng.randint(-1000, 1000), rng.randint(1, 12)) for _ in range(200)] for k in "abc"}
        arrays = make_inputs(self.d, "abc", cols)
        exp = ab.expected("fma", arrays)
        for v in D_ROWS + ["r1-lcm", "r1-freq"]:
            st, lines = run(v, "fma", self.d)
            self.assertTrue(ab.judge(v, lines, exp, None, True)[0], v)
            st, bad = run(v, "fma", self.d, AB_CORRUPT="value")
            self.assertFalse(ab.judge(v, bad, exp, None, True)[0], f"{v}: a wrong value passed")
            st, bad = run(v, "fma", self.d, AB_CORRUPT="unreduced")
            self.assertFalse(ab.judge(v, bad, exp, None, True)[0], f"{v}: an unreduced output passed")

    def test_audits_catch_corruption(self):
        rng = random.Random(2)
        cols = {k: [rand_frac(rng, rng.choice(["unrel", "small", "coarse", "leftover"])) for _ in range(300)]
                for k in "abc"}
        make_inputs(self.d, "abc", cols)
        for v in D_ROWS:
            for how in ("coprime", "dict", "xi"):
                st, _ = run(v, "fma", self.d, AB_AUDIT=1)
                self.assertEqual(st.get("audit"), "ok", v)
                st, _ = run(v, "fma", self.d, AB_AUDIT=1, AB_CORRUPT=how)
                self.assertTrue(str(st.get("audit")).startswith("FAIL"), f"{v}: the audit missed '{how}'")


# ------------------------------------------------------------------------------------------------- machine code

def text_section(path):
    r = subprocess.run(["objcopy", "-O", "binary", "--only-section=.text", str(path), "/dev/stdout"],
                       capture_output=True, check=True)
    return r.stdout


class TestMachineCode(unittest.TestCase):
    """The rows that existed before round 3 compile to the same machine code (.text bytes) from the current sources
    as from REF's."""

    def test_existing_rows_unchanged(self):
        git = shutil.which("git")
        if not git or subprocess.run([git, "-C", str(r3.HERE), "cat-file", "-e", f"{REF}^{{commit}}"]).returncode:
            self.skipTest(f"git or commit {REF} not available")
        top = Path(subprocess.run([git, "-C", str(r3.HERE), "rev-parse", "--show-toplevel"], capture_output=True,
                                  text=True, check=True).stdout.strip())
        rel = r3.HERE.relative_to(top)
        variants = [("v0", "VAR_V0", []), ("v0p", "VAR_V0P", []), ("v1", "VAR_V1", []), ("v2", "VAR_V2", []),
                    ("v3", "VAR_V3", []), ("v4", "VAR_V4", []), ("v5", "VAR_V5", []), ("v34", "VAR_V34", []),
                    ("v6", "VAR_V6", []), ("v0p32", "VAR_V0P", ["-DAB_32"]), ("v1_32", "VAR_V1", ["-DAB_32"]),
                    ("v5_32", "VAR_V5", ["-DAB_32"])]
        with tempfile.TemporaryDirectory() as t:
            ref = Path(t) / "ref" / "abl"
            ref.mkdir(parents=True)
            for f in ("abl_bench.c", "ab_core.c", "ab_l1.c", "ab_v3.c", "ab_v5.c", "ab_v6.c"):
                (ref / f).write_bytes(subprocess.run([git, "-C", str(top), "show", f"{REF}:{rel / f}"],
                                                     capture_output=True, check=True).stdout)
            for f in ("pg.c", "pg.h"):
                (ref.parent / f).write_bytes(subprocess.run([git, "-C", str(top), "show", f"{REF}:{rel.parent / f}"],
                                                            capture_output=True, check=True).stdout)
            for name, macro, extra in variants:
                outs = []
                for src in (ref, r3.HERE):
                    o = Path(t) / f"{name}-{len(outs)}"
                    subprocess.run(["clang", *r3.CFLAGS, *extra, f"-DVARIANT={macro}", "-o", str(o),
                                    str(src / "abl_bench.c"), str(src.parent / "pg.c")], check=True)
                    outs.append(text_section(o))
                self.assertEqual(outs[0], outs[1], f"{name}: machine code differs from {REF}")


# ------------------------------------------------------------------------------------------------- the harness

def fake_row(name, totals, ok=True, decomp=0):
    rounds = [{"entry": t // 2, "decomp": decomp, "op": t - t // 2, "red": 0, "total": t} for t in totals]
    return {"name": name, "ok": ok, "msg": "" if ok else "x", "unreduced": 0.0, "rounds": rounds}


class TestHarness(unittest.TestCase):
    def test_row_names(self):
        self.assertEqual(r3.row_parts("r0-lcm-null"), ("r0", "lcm", True))
        self.assertEqual(r3.row_parts("d2-freq"), ("d2", "freq", False))
        self.assertEqual(r3.row_parts("r0-null"), ("r0", "plain", True))
        self.assertEqual(r3.row_parts("d3"), ("d3", "plain", False))
        self.assertEqual(len(r3.ARRAY_ROWS), 14)
        self.assertEqual(r3.SINGLE_ROWS, ["r0", "r0-null", "r1", "d0", "d1", "d2", "d3"])
        for v in r3.ARRAY_ROWS + r3.SINGLE_ROWS:
            self.assertEqual(r3.row_name(*r3.row_parts(v)), v)
        self.assertEqual(r3.row_env("d1-freq"), {"AB_V3_POLICY": "freq"})
        self.assertEqual(r3.row_env("d1"), {})

    def test_ratio_and_verdict(self):
        g = np.random.default_rng(0)
        p, lo, hi = r3.ratio_ci([100] * 12, [110] * 12, g, 500)
        self.assertAlmostEqual(p, 1.1)
        self.assertAlmostEqual(lo, 1.1)
        self.assertEqual(r3.verdict(lo, hi, 0.05), "better")
        self.assertEqual(r3.verdict(*r3.ratio_ci([100] * 12, [103] * 12, g, 500)[1:], 0.05), "tie")
        self.assertEqual(r3.verdict(*r3.ratio_ci([100] * 12, [80] * 12, g, 500)[1:], 0.05), "worse")
        self.assertEqual(r3.verdict(1.06, 1.20, 0.05), "better")
        self.assertEqual(r3.verdict(1.04, 1.20, 0.05), "tie")   # overlaps the band
        self.assertEqual(r3.verdict(1.06, 1.20, 0.10), "tie")   # a wider null band
        self.assertEqual(r3.verdict(0.80, 0.94, 0.05), "worse")
        tx = g.normal(100, 2, 12)
        p, lo, hi = r3.ratio_ci(tx, tx * 1.5, g, 500)
        self.assertAlmostEqual(p, 1.5)
        self.assertLess(lo, hi + 1e-12)

    def analysed(self, null_y):
        jr = {"r0-lcm": fake_row("r0-lcm", [100] * 12), "r0-lcm-null": fake_row("r0-lcm-null", null_y),
              "r1-lcm": fake_row("r1-lcm", [80] * 12), "d0-lcm": fake_row("d0-lcm", [100] * 12),
              "d1-lcm": fake_row("d1-lcm", [91] * 12), "d2-lcm": fake_row("d2-lcm", [120] * 12, decomp=7),
              "d3-lcm": fake_row("d3-lcm", [100] * 12, ok=False)}
        res = {"workloads": {"W2": {"desc": "d", "n": 1, "K": None, "aim": "existing", "single": False,
                                    "jobs": {"fma": jr}}}}
        return res, r3.analyse(res, 1, 500)["W2 fma lcm"]

    def test_analyse(self):
        res, a = self.analysed([100] * 12)
        self.assertEqual(a["h"], 0.05)
        c = a["cmp"]
        self.assertEqual((c["r1/r0"]["verdict"], round(c["r1/r0"]["point"], 3)), ("better", 1.25))
        self.assertEqual(c["d1/d0"]["verdict"], "better")        # 100 / 91 = 1.099
        self.assertEqual(c["d2/d0"]["verdict"], "worse")         # 100 / 120
        self.assertEqual((c["d3/d0"]["verdict"], c["d3/d0"]["why"]), ("n/a", "fail"))
        self.assertEqual(c["d0/r0"]["verdict"], "tie")
        # a noisy null widens the band: d1 (1.099) becomes a tie once h is above 10%
        res, a = self.analysed([100, 125] * 6)
        self.assertGreater(a["h"], 0.10)
        self.assertEqual(a["cmp"]["d1/d0"]["verdict"], "tie")
        self.assertEqual(r3.phases({"entry": 10, "decomp": 4, "op": 1, "red": 1, "total": 12}, "d2-lcm")["entry"], 6)
        self.assertEqual(r3.phases({"entry": 10, "decomp": 4, "op": 1, "red": 1, "total": 12}, "r0-lcm")["entry"], 10)

    def test_report(self):
        res, _ = self.analysed([100] * 12)
        an = r3.analyse(res, 1, 300)
        cfg = {"date": "", "cpu": "", "rounds": 12, "reps": 3, "boot": 300}
        md = r3.report_md(res, cfg, an)
        self.assertIn("**Decision rule", md)
        self.assertIn("over 300 bootstrap", md)
        self.assertIn("| W2 fma | lcm | existing | 1.25 [1.25, 1.25] | 5.0% | better |", md)
        self.assertIn("**Tally:** better 1, tie 0, worse 0.", md)
        self.assertIn("D3 better 0, tie 0, worse 0, not judged 1", md)
        self.assertIn("| d3-lcm |", md)
        self.assertIn("FAIL: x", md)

    def test_round2_data_unchanged(self):
        """W1-W10 and S8-S30d come out of the generator exactly as in round 2 (same seed, same order)."""
        seed = 5
        g2 = np.random.default_rng(seed)
        old = {wl: w["gen"](g2, w["n"]) for wl, w, _ in r2.plan(True)}
        g3 = np.random.default_rng(seed)
        new = {}
        for wl, w in r3.all_workloads(True).items():
            new[wl] = w["gen"](g3, w["n"])
        for wl, arrs in old.items():
            for k, (num, den) in arrs.items():
                self.assertTrue(np.array_equal(num, new[wl][k][0]) and np.array_equal(den, new[wl][k][1]), (wl, k))
        P = r3.plan(True)
        self.assertNotIn("W5", [p[0] for p in P])
        self.assertEqual({p[0] for p in P} >= {"W1", "W2", "W3", "W4", "W6", "W7", "W8", "W9", "W10", "S8", "S30d"}, True)

    def test_new_workloads(self):
        g = np.random.default_rng(1)
        W = r3.NEW_WORKLOADS
        n = 3000
        for wl in ("K8-dec", "K8-mix"):  # every value of the K = 8 chain fits 64 bits
            arrs = W[wl][3](g, n)
            for x in ab.expected("chain", arrs, 8):
                self.assertTrue(-2**63 <= x.numerator < 2**63 and x.denominator < 2**63, wl)
            self.assertTrue(all(abs(Fraction(int(a), int(b))) < 2 for a, b in zip(*arrs["b"])), wl)
        arrs = W["M75-12"][3](g, 20000)
        share = float(np.mean(arrs["a"][1] == 1000))
        self.assertAlmostEqual(share, 0.75, delta=0.02)
        arrs = W["U8"][3](g, 1000)
        self.assertEqual(len(set(arrs["a"][1].tolist())), 8)
        arrs = r3.sweep(g, 5000, 1000)
        dens = set(arrs["a"][1].tolist())
        self.assertEqual(len(dens), 1000)
        self.assertEqual(sum(d >= 2**32 for d in dens), 500)
        self.assertTrue(np.array_equal(arrs["a"][1], arrs["c"][1]))
        for pool in (64, 4000):
            arrs = r3.two_primes(g, 4000, pool, M=2000)
            dens = sorted(set(arrs["a"][1].tolist()))
            self.assertEqual(len(dens), 2000)
            primes = {}
            for d in dens:
                f = sympy.factorint(d)
                self.assertEqual(len(f), 2)
                self.assertTrue(all(2**17 <= p < 2**24 and e == 1 for p, e in f.items()))
                for p in f:
                    primes[p] = primes.get(p, 0) + 1
            self.assertLessEqual(len(primes), pool)
            if pool == 4000:
                self.assertEqual(set(primes.values()), {1})  # disjoint pairs: no prime shared
            else:
                self.assertGreater(min(primes.values()), 20)

    def test_end_to_end_quick(self):
        """A tiny run: the rule is written before the first measurement, verdicts and audits are reported."""
        with tempfile.TemporaryDirectory() as t:
            rc = r3.main(["--quick", "--rounds", "2", "--reps", "1", "--only", "W3,S8", "--out", t, "--boot", "200"])
            self.assertEqual(rc, 0)
            md = (Path(t) / "round3.md").read_text()
            import json
            res = json.loads((Path(t) / "round3.json").read_text())
            self.assertLessEqual(res["config"]["rule_fixed"], res["config"]["first_measurement"])
            self.assertIn("**Decision rule", md)
            self.assertEqual(set(res["analysis"]), {"W3 sum lcm", "W3 sum freq", "W3 fma lcm", "W3 fma freq", "S8 add plain",
                                                    "S8 mul plain"})
            self.assertTrue(all(x == "pass" for j in res["audits"].values() for r in j.values() for x in r.values()))


if __name__ == "__main__":
    unittest.main(verbosity=2)
