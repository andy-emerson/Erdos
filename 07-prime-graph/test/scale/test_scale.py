"""Tests for the scalability stress test (scale.c, g_s.c, g_c.c, s_v3.c, scale_bench.py).

    python3 test_scale.py          (or: python3 -m unittest test_scale -v)

The binaries are driven through their script mode (commands on stdin).  Covered: the split procedure on small cases
(30 then 11/45 gives nodes 2, 3, 5; 30 -> 6*5 -> 2*3*5; weights carried over with exponents; the coarsest graph: no
split when a denominator is a power of a node), the dictionary's first sighting, reference counts and freeing,
the invariants after random sequences (S, S-lazy, C) with every result checked against fractions, the idle pass's
batch refinement, the batch-gcd coprime audit against pairwise gcds, the small workloads end to end (including the
leak check), and each check catching a wrong result or a broken invariant.
"""
import json
import math
import random
import subprocess
import tempfile
import unittest
from fractions import Fraction
from pathlib import Path

import scale_bench as sb

sb.build()
S, C = str(sb.binary("s")), str(sb.binary("c"))
P1, P2, P3, P4 = 1000003, 1000033, 1000037, 1000039  # primes above 2^16; any two multiply above 2^32
HARSH = [65537, 65539, 65543, 1000003, 1000033, 1000037, 1000039, 1000081, 16777259, 16777289]


def script(binary, lines, check=True):
    r = subprocess.run([binary, "script"], input="\n".join(lines) + "\n", capture_output=True, text=True, timeout=600)
    if check and r.returncode != 0:
        raise AssertionError(f"{binary} script failed: {r.stderr[-2000:]}")
    return r.stdout.splitlines()


def nodes(out):
    return [list(map(int, l.split()[1:])) for l in out if l.startswith("N")]


def edges(out):
    return [l.split()[1:] for l in out if l[0] in "ED"]


class TestSplits(unittest.TestCase):
    def test_30_then_45(self):
        out = script(S, ["enter single 1/30 7/30", "nodes", "edges 0", "enter single 11/45", "nodes", "edges 0",
                         "edges 2", "degree 3", "degree 5", "degree 2", "audit"])
        self.assertEqual(nodes(out), [[30], [2, 3, 5]])
        self.assertEqual(edges(out), [["30^1"], ["2^1", "3^1", "5^1"], ["3^2", "5^1"]])
        # degree and refcount (equal): 3 and 5 hold 1/30, 7/30, 11/45; 2 holds 1/30, 7/30
        self.assertEqual([l for l in out if l.startswith("G")], ["G 3 3", "G 3 3", "G 2 2"])
        self.assertEqual(out[-1], "A ok")

    def test_30_to_6_5_to_2_3_5(self):
        out = script(S, ["enter single 1/30", "enter single 1/5", "nodes", "edges 0", "enter single 1/2", "nodes",
                         "edges 0", "edges 1", "edges 2", "audit"])
        self.assertEqual(nodes(out), [[5, 6], [2, 3, 5]])
        self.assertEqual(sorted(edges(out)[0]), ["5^1", "6^1"])
        self.assertEqual(sorted(edges(out)[1]), ["2^1", "3^1", "5^1"])
        self.assertEqual(edges(out)[2], ["5^1"])
        self.assertEqual(edges(out)[3], ["2^1"])
        self.assertEqual(out[-1], "A ok")

    def test_weights_carry_over(self):
        # 900 = 30^2 first: one node 30 with weight 2; 1/2 splits 2 out: 900 = 2^2 * 15^2; 1/9 splits 15: 3^2 * 5^2
        out = script(S, ["enter single 1/900", "edges 0", "enter single 1/2", "nodes", "edges 0", "enter single 1/9",
                         "nodes", "edges 0", "edges 2", "audit"])
        e = edges(out)
        self.assertEqual(e[0], ["30^2"])
        self.assertEqual(nodes(out), [[2, 15], [2, 3, 5]])
        self.assertEqual(sorted(e[1]), ["15^2", "2^2"])
        self.assertEqual(sorted(e[2]), ["2^2", "3^2", "5^2"])
        self.assertEqual(e[3], ["3^2"])
        self.assertEqual(out[-1], "A ok")

    def test_unequal_exponents(self):
        # 12 = 2^2 * 3: node 12; 1/6 has ratios 1/2 (for 2) and 1 (for 3): split into 2 and 3, 12 = 2^2 * 3
        out = script(S, ["enter single 1/12", "edges 0", "enter single 5/6", "nodes", "edges 0", "edges 1", "audit"])
        self.assertEqual(edges(out)[0], ["12^1"])
        self.assertEqual(nodes(out), [[2, 3]])
        self.assertEqual(sorted(edges(out)[1]), ["2^2", "3^1"])
        self.assertEqual(sorted(edges(out)[2]), ["2^1", "3^1"])
        # 36 = 6^2 first: node 6 with weight 2; 1/4 = 2^2 has ratio 2 on 2 and 0 on 3: split
        out = script(S, ["enter single 1/36", "edges 0", "enter single 1/4", "nodes", "edges 0", "edges 1", "audit"])
        self.assertEqual(edges(out)[0], ["6^2"])
        self.assertEqual(nodes(out), [[2, 3]])
        self.assertEqual(sorted(edges(out)[1]), ["2^2", "3^2"])
        self.assertEqual(edges(out)[2], ["2^2"])
        self.assertEqual(out[-1], "A ok")

    def test_coarsest_no_split(self):
        # a power of a node, or the same denominator again, never splits it
        out = script(S, ["enter single 1/30", "enter single 1/900 1/30 7/27000", "nodes", "edges 1", "edges 3",
                         "stats", "audit"])
        self.assertEqual(nodes(out), [[30]])
        self.assertEqual(edges(out), [["30^2"], ["30^3"]])
        st = json.loads(out[-2])
        self.assertEqual(st["splits_entry"], 0)
        self.assertEqual(out[-1], "A ok")

    def test_first_sighting_large_primes(self):
        # rho mode: P1*P2 is factored at entry; first sighting: one node; P1 again splits it
        out = script(S, [f"enter single 1/{P1 * P2}", "nodes", f"enter single 1/{P1 * 3}", "nodes", "audit"])
        self.assertEqual(nodes(out), [[P1 * P2], [3, P1, P2]])
        self.assertEqual(out[-1], "A ok")

    def test_split_loop_updates_every_numerator(self):
        # many numerators on one node, including a dense array's D; the split reaches each, values unchanged
        vals = [Fraction(n, 30030) for n in (1, -17, 19, 23, 29, 31)]
        lines = ["enter single " + " ".join(f"{v.numerator}/{v.denominator}" for v in vals),
                 "enter array 1/30030 17/30030 19/30030",
                 "degree 30030", "enter single 1/7", "nodes", "degree 7", "degree 4290", "audit"]
        lines += [f"get {h}" for h in range(9)]
        out = script(S, lines)
        self.assertEqual([l for l in out if l.startswith("G")][0], "G 7 7")  # 6 singles + the array's D holder
        self.assertEqual(nodes(out)[0], [7, 4290])
        self.assertIn("G 8 8", out)  # + the new 1/7
        self.assertEqual(out[-10], "A ok")
        got = [Fraction(int(l.split()[1]), int(l.split()[2])) for l in out if l.startswith("V")]
        self.assertEqual(got, vals + [Fraction(1, 30030), Fraction(17, 30030), Fraction(19, 30030)])


class TestRefcounts(unittest.TestCase):
    def test_free_on_last_numerator(self):
        out = script(S, ["enter single 1/7", "degree 7", "enter single 3/14", "degree 7", "nodes", "del 0",
                         "degree 7", "nodes", "audit", "del 1", "nodes", "audit"])
        g = [l for l in out if l.startswith("G")]
        self.assertEqual(g, ["G 1 1", "G 2 2", "G 1 1"])
        self.assertEqual(nodes(out), [[2, 7], [2, 7], []])
        self.assertEqual([l for l in out if l.startswith("A")], ["A ok", "A ok"])

    def test_array_D_holder_and_compaction(self):
        vals = " ".join(f"{i}/7" for i in range(1, 101))
        lines = ["enter array " + vals, "degree 7"] + [f"del {h}" for h in range(80)] + ["degree 7", "audit"]
        lines += [f"get {h}" for h in range(80, 100)] + [f"del {h}" for h in range(80, 100)] + ["nodes", "audit"]
        out = script(S, lines)
        self.assertEqual([l for l in out if l.startswith("G")], ["G 1 1", "G 1 1"])  # one D holder
        got = [Fraction(int(l.split()[1]), int(l.split()[2])) for l in out if l.startswith("V")]
        self.assertEqual(got, [Fraction(i, 7) for i in range(81, 101)])
        self.assertEqual(nodes(out), [[]])
        self.assertEqual([l for l in out if l.startswith("A")], ["A ok", "A ok"])

    def test_results_hold_references(self):
        out = script(S, ["enter single 1/11 1/13", "add 0 1", "del 0", "del 1", "nodes", "audit"])
        self.assertEqual(out[1], "R 24 143")
        self.assertEqual(nodes(out), [[11, 13]])  # the stored result 24/143 still uses both
        self.assertEqual(out[-1], "A ok")

    def test_no_leak_after_delete_all(self):
        warm = ["enter array 1/6 1/35 2/77", "enter single 1/91", "add 0 3", "delall", "baseline"]
        work = ["enter array 1/30 7/45 1/1000003", f"enter single 1/{P1 * P2} 3/{P1 * 5}", "mul 0 3", "add 1 4",
                "enter single 1/2", "delall"]
        out = script(S, warm + work)
        self.assertEqual(out[-1], "L blocks=0 nodes=0 holders=0 pool_used=0 handles=0 arrays=0")


class TestInvariantsRandom(unittest.TestCase):
    def fuzz(self, binary, mode, seed, steps=12, large=False, harsh=False):
        rng = random.Random(seed)
        lines = [f"mode {mode}"]
        live, expect = [], {}
        nh, free = 0, []  # handle ids: fresh, or the last freed (the binary's free list is LIFO)
        primes = [P1, P2, P3, P4, 65537, 65539, 1000081]
        ops = []
        for step in range(steps):
            n = rng.randint(1, 12)
            fr = []
            for _ in range(n):
                if harsh and rng.random() < 0.6:  # up to three large primes, squares, a small cofactor
                    d = rng.choice([1, 2, 3, 4, 6, 9, 30])
                    for _ in range(rng.randint(1, 3)):
                        q = rng.choice(HARSH) ** rng.choice([1, 1, 2])
                        if d * q < 2**62:
                            d *= q
                elif large and rng.random() < 0.5:
                    d = rng.choice(primes) * rng.choice([1, 2, 3, 6, rng.choice(primes)])
                else:
                    d = rng.choice([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 30, 36, 45, 100, 210, 1000, 2310, 30030])
                fr.append((rng.randint(-50, 50), d))
            kind = rng.choice(["single", "array"])
            lines.append(f"enter {kind} " + " ".join(f"{a}/{b}" for a, b in fr))
            for a, b in fr:
                if free:
                    h = free.pop()
                else:
                    h, nh = nh, nh + 1
                expect[h] = Fraction(a, b)
                live.append(h)
            if mode == "lazy" or rng.random() < 0.6:  # lazy: the invariant holds once the idle pass has run
                lines.append("idle")
            for _ in range(rng.randint(0, 4)):
                x, y = rng.choice(live), rng.choice(live)
                op = rng.choice(["add", "mul"])
                lines.append(f"{op} {x} {y}")
                ops.append((op, expect[x], expect[y]))
            if len(live) > 3 and rng.random() < 0.5:
                h = live.pop(rng.randrange(len(live)))
                lines.append(f"del {h}")
                del expect[h]
                free.append(h)
            lines.append("idle")
            lines.append("audit")
        for h in live:
            lines.append(f"get {h}")
        lines.append("delall")
        self.last_lines = lines
        out = script(binary, lines)
        hs = [int(x) for l in out if l.startswith("H") for x in l.split()[1:]]
        self.assertEqual(len(hs), sum(len(l.split()) - 2 for l in lines if l.startswith("enter")))
        self.assertEqual([l for l in out if l.startswith("A") and l != "A ok"], [])
        rs = [l for l in out if l.startswith("R")]
        self.assertEqual(len(rs), len(ops))
        for (op, a, b), l in zip(ops, rs):
            rn, rd = map(int, l.split()[1:])
            x = a + b if op == "add" else a * b
            if rd == 0:  # the op code's contract: the exact result does not fit, or (add) the lcm reaches 2^63
                fits = -2**63 <= x.numerator < 2**63 and x.denominator < 2**63
                lcm_big = op == "add" and math.lcm(a.denominator, b.denominator) >= 2**63
                self.assertTrue(not fits or lcm_big, (op, a, b))
                continue
            self.assertEqual(math.gcd(abs(rn), rd), 1)
            self.assertEqual(Fraction(rn, rd), x, (op, a, b))
        vs = [l for l in out if l.startswith("V")]
        for h, l in zip(live, vs):
            vn, vd = map(int, l.split()[1:])
            self.assertEqual(Fraction(vn, vd), expect[h])
            self.assertEqual(math.gcd(abs(vn), vd), 1)
        self.assertTrue(out[-1].startswith("L "), out[-1])
        if binary == S:
            self.assertIn("nodes=0 holders=0 pool_used=0 handles=0 arrays=0", out[-1])
        return out

    def test_S_small(self):
        for seed in range(12):
            self.fuzz(S, "rho", seed)

    def test_S_large_primes(self):
        for seed in range(12):
            self.fuzz(S, "rho", 100 + seed, large=True)

    def test_S_lazy(self):
        for seed in range(12):
            self.fuzz(S, "lazy", 200 + seed, large=True)

    def test_S_harsh(self):
        for seed in range(8):
            self.fuzz(S, "rho", 400 + seed, steps=20, harsh=True)
            self.fuzz(S, "lazy", 500 + seed, steps=20, harsh=True)

    def test_C(self):
        for seed in range(6):
            self.fuzz(C, "rho", 300 + seed, large=True)

    def test_S_matches_C(self):
        lines = ["enter array 1/6 5/12 7/30 1/1000003", f"enter single 3/{P1 * P2} 1/{P1 * 7}", "add 0 1", "mul 2 4",
                 "add 3 5", "mul 4 5", "add 1 2"]
        rs = lambda o: [l for l in o if l.startswith("R")]
        self.assertEqual(rs(script(S, lines)), rs(script(C, lines)))
        self.assertEqual(rs(script(S, ["mode lazy"] + lines)), rs(script(C, lines)))


class TestLazyIdle(unittest.TestCase):
    def test_batch_refinement(self):
        out = script(S, ["mode lazy", f"enter single 1/{P1 * P2}", "nodes", "idle", "nodes", "stats",
                         f"enter single 5/{P1 * P3}", "nodes", "audit coprime", "idle", "nodes", "edges 0", "edges 1",
                         "audit", "get 0", "get 1"])
        n = nodes(out)
        self.assertEqual(n[0], [P1 * P2])
        self.assertEqual(n[1], [P1 * P2])  # no sharing: settles as one leftover, never factored
        st = json.loads([l for l in out if l.startswith("{")][0])
        self.assertEqual(st["rho_calls"], 0)
        self.assertEqual(st["atoms_not_prime"], 1)  # a leftover
        self.assertEqual(n[2], sorted([P1 * P2, P1 * P3]))
        self.assertTrue([l for l in out if l.startswith("A")][0].startswith("A FAIL nodes_not_coprime"))  # until idle
        self.assertEqual(n[3], [P1, P2, P3])
        self.assertEqual(sorted(edges(out)[0]), sorted([f"{P1}^1", f"{P2}^1"]))
        self.assertEqual(sorted(edges(out)[1]), sorted([f"{P1}^1", f"{P3}^1"]))
        self.assertIn("A ok", out)
        self.assertEqual(out[-2:], [f"V 1 {P1 * P2}", f"V 5 {P1 * P3}"])

    def test_piece_already_held_by_one_node(self):
        # a leftover b*c refined into a node {b, c}, then a pending b*c*d-like value: the shared pieces merge
        lines = ["mode lazy", f"enter single 1/{P1 * P2}", "idle", f"enter single 1/{P1 * 3}", "idle",
                 f"enter single 1/{P2 * P3}", f"enter single 1/{P3 * P4}", "idle", "audit", "nodes"]
        out = script(S, lines)
        self.assertIn("A ok", out)
        self.assertEqual(nodes(out)[-1], [3, P1, P2, P3, P4])

    def test_op_refines_a_leftover(self):
        # a leftover node shares part of a numerator in an op: refined by gcd, the result in lowest terms
        out = script(S, ["mode lazy", f"enter single 1/{P1 * P2}", "idle", f"enter single {P1 - 1}/{P1 * P2}",
                         "idle", "add 0 1", "nodes", "audit"])
        r = [l for l in out if l.startswith("R")][0].split()
        self.assertEqual(Fraction(int(r[1]), int(r[2])), Fraction(P1, P1 * P2))
        self.assertEqual(r[1:], ["1", str(P2)])
        self.assertEqual(nodes(out)[-1], [P1, P2])
        self.assertEqual(out[-1], "A ok")


class TestChecksCatch(unittest.TestCase):
    BASE = ["enter single 1/30 7/30 11/45 1/2 3/77", "enter array 1/6 1/10 1/15"]

    def corrupt(self, what, section="all", binary=S):
        out = script(binary, self.BASE + ["audit", f"corrupt {what}", f"audit {section}"])
        self.assertEqual(out[-3], "A ok")
        self.assertEqual(out[-2], "C 1")
        self.assertTrue(out[-1].startswith("A FAIL"), (what, out[-1]))
        return out[-1]

    def test_audits_catch(self):
        self.assertIn("coprime", self.corrupt("coprime", "coprime"))
        self.assertIn("coprime", self.corrupt("coprime", "coprimepairwise"))
        self.assertIn("position", self.corrupt("pos", "sym"))
        self.assertTrue(self.corrupt("list", "sym"))
        self.assertIn("refcount", self.corrupt("list", "rc"))
        self.assertIn("refcount", self.corrupt("rc", "rc"))
        self.assertIn("dict", self.corrupt("dict", "dict"))
        self.assertIn("den", self.corrupt("den", "den"))
        self.assertIn("coprime", self.corrupt("coprime", "coprime", binary=C))
        self.assertIn("den", self.corrupt("den", "den", binary=C))

    def test_leak_caught(self):
        warm = ["enter single 1/6 1/35", "delall", "baseline"]
        out = script(S, warm + ["enter single 1/30 1/7", "corrupt leak", "delall"])
        self.assertNotEqual(out[-1], "L blocks=0 nodes=0 holders=0 pool_used=0 handles=0 arrays=0")
        self.assertIn("holders=1", out[-1])
        out = script(S, warm + ["enter single 1/30 1/7", "corrupt leaklist", "delall"])
        self.assertTrue(out[-1].startswith("L blocks=1 "), out[-1])

    def test_wrong_result_caught(self):
        out = script(S, ["enter single 1/6 1/10", "corrupt result", "add 0 1"])
        rn, rd = map(int, out[-1].split()[1:])
        line = f"o + 1 6 1 10 {rn} {rd}"
        self.assertEqual(sb.check_line(line), "wrong_result")
        self.assertIsNone(sb.check_line("o + 1 6 1 10 4 15"))

    def test_check_line(self):
        self.assertIsNone(sb.check_line("o * 2 3 3 4 1 2"))
        self.assertEqual(sb.check_line("o * 2 3 3 4 2 4"), "not_lowest_terms")
        self.assertEqual(sb.check_line("o * 2 3 3 4 1 3"), "wrong_result")
        self.assertEqual(sb.check_line("o + 0 1 0 5 0 5"), "not_lowest_terms")
        self.assertEqual(sb.check_line("o + 1 2 1 3 0 0"), "ovf_false")
        big = 2 ** 62 + 1
        self.assertEqual(sb.check_line(f"o * {big} 1 4 1 0 0"), "ovf_true")
        self.assertIsNone(sb.check_line("v 2 4 1 2"))
        self.assertEqual(sb.check_line("v 2 4 2 4"), "stored_not_lowest_terms")
        self.assertEqual(sb.check_line("v 2 4 1 3"), "stored_wrong")
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "x.chk"
            p.write_text("o + 1 2 1 3 5 6\nv 1 2 1 2\no * 1 2 1 3 1 5\n")
            c = sb.check_file(p)
            self.assertEqual((c["ops"], c["values"], c["fail"]), (2, 1, 1))

    def test_verdicts_catch(self):
        g = {"steps": [{"fixed_ns": 100.0, "op_ns": 100.0}, {"fixed_ns": 250.0, "op_ns": 120.0}]}
        self.assertFalse(sb.growth_verdict(g)["pass"])
        g["steps"][1]["fixed_ns"] = 150.0
        self.assertTrue(sb.growth_verdict(g)["pass"])
        base = {"ns_per_event": 1.0, "values": 100, "mem": 1000}
        ch = {"steps": [dict(base, nodes_live=10), dict(base, nodes_live=15),
                        {"final": 1, "blocks_over_baseline": 0, "nodes_live": 0, "holders": 0, "pool_used": 0,
                         "handles_live": 0, "arrays": 0}]}
        self.assertTrue(sb.churn_verdict(ch)["pass"])
        ch["steps"][-1]["nodes_live"] = 3  # retained nodes: fails
        self.assertFalse(sb.churn_verdict(ch)["pass"])
        ch["steps"][-1]["nodes_live"] = 0
        ch["steps"][1]["nodes_live"] = 25  # nodes not bounded
        self.assertFalse(sb.churn_verdict(ch)["pass"])

    def test_batch_gcd_matches_pairwise(self):
        rng = random.Random(5)
        for t in range(20):
            primes = [1000003, 1000033, 1000037, 65537, 3, 5, 7, 2]
            vals = set()
            while len(vals) < rng.randint(2, 40):
                vals.add(rng.randint(2, 2**62))
            vals = list(vals)
            if t % 2:
                p = rng.choice(primes)
                vals[0] = vals[0] // p * p or p
                vals[-1] = vals[-1] // p * p or p
            out = script(S, ["cpcheck " + " ".join(map(str, vals))])
            batch, pair = map(int, out[0].split()[1:])
            truth = all(math.gcd(a, b) == 1 for i, a in enumerate(vals) for b in vals[i + 1:])
            self.assertEqual((batch, pair), (int(truth), int(truth)), vals)


class TestWorkloadsSmall(unittest.TestCase):
    def wl(self, binary, wl, **kw):
        with tempfile.TemporaryDirectory() as d:
            chk = Path(d) / "c.chk"
            args = [binary, wl, f"chk={chk}"] + [f"{k}={v}" for k, v in kw.items()]
            r = subprocess.run(args, capture_output=True, text=True, timeout=600)
            self.assertEqual(r.returncode, 0, r.stderr[-2000:])
            steps = [json.loads(l) for l in r.stdout.splitlines() if l.startswith("{")]
            c = sb.check_file(chk)
        self.assertEqual(c["fail"], 0, c)
        self.assertGreater(c["ops"] + c["values"], 0)
        for s in steps:
            self.assertIn(s.get("audit"), (None, "ok"), s)
        return steps

    def test_heavy(self):
        st = self.wl(S, "heavy", deg=3000, B=1000, nchk=300, nval=300)
        sp = [s for s in st if s.get("split", 0) >= 1]
        self.assertEqual([s["splits"] for s in sp], [1, 1])
        self.assertEqual(sp[0]["numerators"], 3000)  # the loop visits every numerator of 30030 once
        self.assertEqual(sp[1]["numerators"], 3000)  # 4290's list: the 3000 (1/7 went to node 7 only)
        st = self.wl(C, "heavy", deg=3000, B=1000, nchk=300, nval=300)
        self.assertEqual([s.get("numerators") for s in st if s.get("split", 0) >= 1], [0, 0])

    def test_chain(self):
        st = self.wl(S, "chain", deg=500, nchk=300, nval=300)
        sp = [s for s in st if s.get("split", 0) >= 1]
        self.assertEqual(len(sp), 14)
        self.assertTrue(all(s["splits"] == 1 for s in sp))
        self.assertEqual([s["numerators"] for s in sp], [500] * 14)
        self.assertEqual(sp[-1]["nodes_live"], 15)

    def test_growth(self):
        for b, kw in ((S, {}), (S, {"lazy": 1}), (C, {})):
            st = self.wl(b, "growth", N=20000, B=4000, M=2000, T=500, audit_every=1, nchk=300, nval=300, **kw)
            self.assertEqual(len(st), 5)
            self.assertGreater(st[-1]["nodes_live"], 300)

    def test_churn_no_leak(self):
        st = self.wl(S, "churn", L=3000, E=30000, b=200, step=10000, W=300, rate=3, Bi=1000, nchk=200, nval=200)
        fin = st[-1]
        self.assertEqual(fin.get("final"), 1)
        self.assertEqual((fin["blocks_over_baseline"], fin["nodes_live"], fin["holders"], fin["pool_used"],
                          fin["handles_live"], fin["arrays"]), (0, 0, 0, 0, 0, 0))
        self.assertGreater(st[-2]["nodes_freed"], 0)
        st = self.wl(S, "churn", L=3000, E=30000, b=200, step=10000, W=300, rate=3, Bi=1000, nchk=200, nval=200, lazy=1)
        self.assertEqual(st[-1]["nodes_live"], 0)
        st = self.wl(C, "churn", L=3000, E=30000, b=200, step=10000, W=300, rate=3, Bi=1000, nchk=200, nval=200)
        self.assertGreater(st[-1]["nodes_live"], 0)  # the control never frees nodes

    def test_distinct(self):
        for b, kw in ((S, {}), (S, {"lazy": 1}), (C, {})):
            st = self.wl(b, "distinct", ND=500, per=3, B=1000, M=2000, nchk=300, nval=300, **kw)
            self.assertEqual(st[-1]["ND"], 500)


class TestReport(unittest.TestCase):
    def test_report_renders(self):
        res = {"growth": [{"row": "S", "params": {}, "wall_s": 1.0, "rc": 0,
                           "check": {"ops": 1, "values": 1, "fail": 0, "ovf_true": 0, "ovf_false": 0, "reasons": {}},
                           "steps": [{"step": 0, "values": 10, "nodes_live": 3, "entry_ns_per_value": 1.0, "idle_ns": 0,
                                      "fixed_ns": 1.0, "fixed_add_ns": 1.0, "fixed_mul_ns": 1.0, "fixed_first_ns": 1.0,
                                      "op_ns": 1.0, "op_first_ns": 1.0, "fixed_ovf": 0, "op_ovf": 0, "mem": 1,
                                      "audit": "ok"}]}]}
        md = sb.report_md(res, {"date": "x", "cpu": "y", "quick": True}, {})
        self.assertIn("**pass**", md)
        self.assertIn("## 1. Growth: S", md)
        self.assertIn("fixed-sample ns per op", md)  # the ASCII plot without matplotlib files


if __name__ == "__main__":
    unittest.main()
