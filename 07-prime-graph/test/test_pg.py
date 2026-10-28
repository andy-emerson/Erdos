"""Correctness tests for the prime-graph prototype (pg.c), through ctypes.

    python3 test_pg.py          (or: python3 -m unittest test_pg -v)

Each check is a plain function that raises AssertionError; every check also has a test feeding it a deliberately
wrong result, to show that it catches one.  Needs sympy.
"""
import ctypes as C
import random
import subprocess
import unittest
from fractions import Fraction
from math import gcd, prod
from pathlib import Path

import sympy

import pg_bench

HERE = Path(__file__).resolve().parent
LIB = HERE / "build" / "libpg.so"
BAD = 0xFFFFFFFF
I64MAX, I64MIN = 2**63 - 1, -2**63


MAXE = 16


class Val(C.Structure):
    _fields_ = [("num", C.c_int64), ("den", C.c_uint64), ("gid", C.c_uint32), ("ne", C.c_uint8),
                ("w", C.c_uint8 * MAXE), ("node", C.c_uint32 * MAXE)]


class Arr(C.Structure):
    _fields_ = [("n", C.c_size_t), ("num", C.POINTER(C.c_int64)), ("gid", C.POINTER(C.c_uint32)),
                ("uniform", C.c_int), ("ugid", C.c_uint32)]


class Node(C.Structure):
    _fields_ = [("v", C.c_uint64), ("inv", C.c_uint64), ("lim", C.c_uint64), ("s", C.c_uint8), ("prime", C.c_uint8),
                ("alive", C.c_uint8)]


class Group(C.Structure):
    _fields_ = [("den", C.c_uint64), ("node", C.c_uint32 * MAXE), ("w", C.c_uint8 * MAXE), ("ne", C.c_uint8),
                ("dirty", C.c_uint8), ("dec", C.c_uint32 * MAXE)]


def load():
    LIB.parent.mkdir(exist_ok=True)
    src = [HERE / "pg.c", HERE / "pg.h"]
    if not LIB.exists() or LIB.stat().st_mtime < max(s.stat().st_mtime for s in src):
        subprocess.run(["clang", *pg_bench.CFLAGS, "-shared", "-fPIC", "-o", str(LIB), str(HERE / "pg.c")], check=True)
    L = C.CDLL(str(LIB))
    sig = {"pg_init": ([], None), "pg_reset": ([], None), "pg_den_gid": ([C.c_uint64], C.c_uint32),
           "pg_from": ([C.c_int64, C.c_uint64], Val), "pg_add": ([Val, Val], Val), "pg_sub": ([Val, Val], Val),
           "pg_mul": ([Val, Val], Val), "pg_cmp": ([Val, Val], C.c_int), "pg_reduce": ([Val], Val),
           "pg_factor": ([C.c_uint64, C.POINTER(C.c_uint64), C.POINTER(C.c_uint32)], C.c_int),
           "pg_is_prime": ([C.c_uint64], C.c_int), "pg_decompose_node": ([C.c_uint32], C.c_uint32),
           "pg_decompose_all": ([], C.c_uint32), "pg_sync_pools": ([], None), "pg_register_pool": ([C.POINTER(Val), C.c_size_t], None),
           "pg_arr_alloc": ([C.POINTER(Arr), C.c_size_t], None), "pg_arr_free": ([C.POINTER(Arr)], None),
           "pg_arr_enter": ([C.POINTER(Arr), C.POINTER(C.c_int64), C.POINTER(C.c_uint64)], None),
           "pg_arr_reduce": ([C.POINTER(Arr)], None), "pg_arr_add": ([C.POINTER(Arr)] * 3, None), "pg_arr_sub": ([C.POINTER(Arr)] * 3, None),
           "pg_arr_mul": ([C.POINTER(Arr)] * 3, None), "pg_arr_fma": ([C.POINTER(Arr)] * 4, None),
           "pg_arr_sum": ([C.POINTER(Arr)], Val), "pg_arr_get": ([C.POINTER(Arr), C.c_size_t], Val)}
    for name, (a, r) in sig.items():
        f = getattr(L, name)
        f.argtypes, f.restype = a, r
    L.pg_init()
    return L


L = load()
G = {n: t.in_dll(L, n) for n, t in [("pg_overflows", C.c_uint64), ("pg_abort_on_overflow", C.c_int),
                                    ("pg_keep_uniform", C.c_int), ("pg_use_cache", C.c_int),
                                    ("pg_nnodes", C.c_uint32), ("pg_ngroups", C.c_uint32)]}


def overflows():
    return G["pg_overflows"].value


def snapshot():
    """(nodes, groups) as plain Python: nodes[id] = (v, prime, alive); groups[g] = (den, [(node, w), ...])."""
    np_, ng = G["pg_nnodes"].value, G["pg_ngroups"].value
    nodes_p = C.POINTER(Node).in_dll(L, "pg_nodes")
    groups_p = C.POINTER(Group).in_dll(L, "pg_groups")
    nodes = [(nodes_p[i].v, nodes_p[i].prime, nodes_p[i].alive) for i in range(np_)]
    groups = [(groups_p[g].den, [(groups_p[g].node[k], groups_p[g].w[k]) for k in range(groups_p[g].ne)])
              for g in range(ng)]
    return nodes, groups


def factor(n):
    p = (C.c_uint64 * 64)()
    e = (C.c_uint32 * 64)()
    k = L.pg_factor(n, p, e)
    return [(p[i], e[i]) for i in range(k)]


# ------------------------------------------------------------------------------------------------- the checks

def check_factorization(n, got):
    """got must be n's factorisation: ascending distinct primes, positive exponents, product n (as sympy says)."""
    assert all(e >= 1 for _, e in got), f"{n}: zero exponent in {got}"
    assert [p for p, _ in got] == sorted({p for p, _ in got}), f"{n}: primes not ascending and distinct: {got}"
    assert all(sympy.isprime(p) for p, _ in got), f"{n}: a composite factor in {got}"
    assert prod(p**e for p, e in got) == n, f"{n}: product of {got} is not n"
    assert dict(got) == sympy.factorint(n), f"{n}: {got} differs from sympy"


def check_graph(nodes, groups):
    """The coprime invariant and the group bookkeeping."""
    live = [i for i, (v, _, a) in enumerate(nodes) if a]
    for i in live:
        v, pr, _ = nodes[i]
        assert v > 1, f"node {i} has value {v}"
        if pr:
            assert sympy.isprime(v), f"node {i} = {v} flagged prime but composite"
    for x in range(len(live)):
        for y in range(x + 1, len(live)):
            a, b = nodes[live[x]][0], nodes[live[y]][0]
            assert gcd(a, b) == 1, f"nodes {live[x]} = {a} and {live[y]} = {b} share {gcd(a, b)}"
    dens = set()
    for g, (den, edges) in enumerate(groups):
        assert len(edges) <= MAXE, f"group {g} has {len(edges)} edges"
        assert [n for n, _ in edges] == sorted({n for n, _ in edges}), f"group {g} edges unsorted: {edges}"
        for n, w in edges:
            assert w >= 1, f"group {g}: zero weight"
            assert nodes[n][2], f"group {g} uses dead node {n}"
        assert den == prod(nodes[n][0] ** w for n, w in edges), f"group {g}: den {den} != product of {edges}"
        assert den not in dens, f"two groups with denominator {den}"
        dens.add(den)


def check_value(v, want, groups=None):
    """A pg value must be good, equal want, and carry its group's denominator (and edges, when groups is given)."""
    assert v.gid != BAD, f"bad value where {want} was expected"
    assert v.den > 0, "nonpositive denominator"
    assert Fraction(v.num, v.den) == want, f"got {v.num}/{v.den}, expected {want}"
    if groups is not None:
        den, edges = groups[v.gid]
        assert v.den == den, f"cached den {v.den} != group den {den}"
        assert [(v.node[k], v.w[k]) for k in range(v.ne)] == edges, "inline edges differ from the group's"


def check_reduced(v):
    """A good value in lowest terms."""
    assert v.gid != BAD and v.den > 0, "bad value"
    assert gcd(v.num, v.den) == 1, f"{v.num}/{v.den} is not in lowest terms"


def value_or_overflow(v, want, before):
    """An operation either gives the exact answer or reports an overflow; it never returns a wrong value."""
    if v.gid == BAD:
        assert overflows() > before, "bad value without an overflow being counted"
    else:
        check_value(v, want)


# ------------------------------------------------------------------------------------------------- data

def rand_frac(rng, kind):
    if kind == "small":
        return rng.randint(-10**6, 10**6), rng.randint(1, 12)
    if kind == "dec":
        return rng.randint(-10**7, 10**7), 1000
    if kind == "unrel":
        return rng.randint(-10**6, 10**6), rng.randint(1, 1000)
    if kind == "big":
        return rng.randint(-10**4, 10**4), rng.randint(1, 2**31)
    if kind == "int":
        return rng.randint(-10**9, 10**9), 1
    if kind == "wide":
        return rng.randint(-2**40, 2**40), rng.choice([2**20, 3**12, 6**10, 1000**3, 7 * 11 * 13 * 17 * 19 * 23])
    raise ValueError(kind)


KINDS = ["small", "dec", "unrel", "big", "int", "wide"]


class PG(unittest.TestCase):
    def setUp(self):
        L.pg_reset()
        G["pg_overflows"].value = 0
        G["pg_abort_on_overflow"].value = 0
        G["pg_keep_uniform"].value = 0
        G["pg_use_cache"].value = 1


class TestChecksCatchWrong(PG):
    """Every check rejects a deliberately wrong result (and accepts the right one)."""

    def test_factorization_check(self):
        n = 2**3 * 3 * 65537**2
        good = factor(n)
        check_factorization(n, good)
        for bad in ([(2, 3), (3, 1)], [(2, 4), (3, 1), (65537, 2)], [(2, 3), (3 * 65537, 1), (65537, 1)],
                    [(3, 1), (2, 3), (65537, 2)], [(2, 3), (3, 1), (65537, 2), (5, 0)]):
            with self.assertRaises(AssertionError):
                check_factorization(n, bad)

    def test_graph_check(self):
        for d in (12, 8, 45, 1000, 77):
            L.pg_den_gid(d)
        nodes, groups = snapshot()
        check_graph(nodes, groups)
        broken = [list(nodes) + [(6, 0, 1)], groups]                       # a node sharing a factor
        self.assertIn(77, [v for v, _, a in nodes if a])                 # entered unsplit, so not known prime
        flagged = [[(v, 1, a) for v, p, a in nodes], groups]               # a composite flagged prime
        wrongden = [nodes, [(d + 1, e) if e else (d, e) for d, e in groups]]
        deadedge = [[(v, p, 0) for v, p, a in nodes], groups]
        for n, g in (broken, flagged, wrongden, deadedge):
            with self.assertRaises(AssertionError):
                check_graph(n, g)

    def test_value_check(self):
        a, b = L.pg_from(1, 6), L.pg_from(1, 4)
        r = L.pg_add(a, b)
        _, groups = snapshot()
        check_value(r, Fraction(5, 12), groups)
        r2 = Val.from_buffer_copy(r); r2.num += 1
        r3 = Val.from_buffer_copy(r); r3.den *= 2
        r4 = Val.from_buffer_copy(r); r4.gid = BAD
        r5 = Val.from_buffer_copy(r); r5.w[0] += 1
        for bad in (r2, r3, r4):
            with self.assertRaises(AssertionError):
                check_value(bad, Fraction(5, 12))
        with self.assertRaises(AssertionError):
            check_value(r5, Fraction(5, 12), groups)
        with self.assertRaises(AssertionError):
            value_or_overflow(r4, Fraction(5, 12), overflows())  # bad, but no overflow counted

    def test_reduced_check(self):
        v = L.pg_from(2, 4)  # entered as given: unreduced
        L.pg_decompose_all()
        check_reduced(L.pg_reduce(v))
        with self.assertRaises(AssertionError):
            check_reduced(v)
        self.assertEqual(pg_bench.unreduced_pct(["2 4", "1 3", "0 1", "0 5"]), 50.0)

    def test_report_ratio_dash_on_failure(self):
        row = lambda ok: {"median": {"conv": 1e6, "decomp": 0, "op": 2e6, "red": 0, "total": 3e6}, "ok": ok,
                          "msg": "" if ok else "x", "unreduced": 0.0}
        for pg_ok, mpq_ok in ((False, True), (True, False)):
            res = {"workloads": {"W": {"desc": "d", "n": 1, "K": None,
                                       "jobs": {"sum": {"pg": row(pg_ok), "mpq": row(mpq_ok)}}}}}
            md = pg_bench.report_md(res, {"date": "", "cpu": "", "flint": "", "rounds": 1, "reps": 1})
            mpq_line = next(l for l in md.splitlines() if l.startswith("| mpq"))
            self.assertIn("| - | - |", mpq_line)
        res["workloads"]["W"]["jobs"]["sum"] = {"pg": row(True), "mpq": row(True)}
        md = pg_bench.report_md(res, {"date": "", "cpu": "", "flint": "", "rounds": 1, "reps": 1})
        self.assertIn("| 1.00 | 1.00 |", next(l for l in md.splitlines() if l.startswith("| mpq")))

    def test_harness_overflow_limits(self):
        big = Fraction(1, 2**63 + 1)  # its denominator does not fit: a genuine overflow
        exp = [Fraction(k, 3) for k in range(2000)] + [big]
        lines = [f"{k} 3" for k in range(2000)] + ["0 0"]
        ov = {}
        self.assertTrue(pg_bench.check(lines, exp, None, ov)[0])  # 1 genuine overflow in 2001: within 0.1%
        self.assertEqual(ov, {"n": 1, "fit": 0})
        self.assertFalse(pg_bench.check(lines, exp)[0])            # without overflow accounting, "0 0" fails
        false = lines[:5] + ["0 0"] + lines[6:]                    # element 5 = 5/3 fits: a false overflow
        self.assertFalse(pg_bench.check(false, exp, None, {})[0])
        many = [f"{k} 3" for k in range(10)] + ["0 0"] * 2         # 2 genuine overflows in 12: over 0.1%
        self.assertFalse(pg_bench.check(many, [Fraction(k, 3) for k in range(10)] + [big] * 2, None, {})[0])
        allovf = ["0 0"] * 5
        self.assertFalse(pg_bench.check(allovf, [big] * 5, None, {})[0])

    def test_harness_check(self):
        exp = [Fraction(1, 3), Fraction(-5, 2), Fraction(7)]
        self.assertTrue(pg_bench.check(["2 6", "-5 2", "7 1"], exp)[0])
        for bad in (["2 6", "-5 2", "8 1"], ["1 3", "5 -2", "7 1"], ["1 3", "-5 2"], ["1 3", "0 0", "7 1"],
                    ["1 3", "-5", "7 1"]):
            self.assertFalse(pg_bench.check(bad, exp)[0], bad)
        self.assertFalse(pg_bench.check(["1 3", "-5 2", "6 1"], exp, [2])[0])


class TestDecomposition(PG):
    def test_factor_against_sympy(self):
        rng = random.Random(1)
        cases = [1, 2, 3, 4, 2**63, 2**64 - 1, 2**64 - 59, 65521**2, 65537**2, 4294967291 * 4294967279,
                 3215031751, 3825123056546413051, 341550071728321, 561, 1105, 999999999989 * 3, 1000]
        for _ in range(300):
            cases.append(rng.getrandbits(64) | 1)
            cases.append(rng.getrandbits(rng.randint(2, 64)) + 1)
            cases.append(sympy.randprime(2**31, 2**32) * sympy.randprime(2**31, 2**32))
            cases.append(prod(sympy.randprime(2**16, 2**21) for _ in range(3)))
            cases.append(sympy.randprime(2**16, 2**21) ** 2 * rng.randint(1, 1000))
            cases.append(prod(rng.choice([2, 3, 5, 7, 11, 13]) for _ in range(rng.randint(1, 20))))
        for n in cases:
            check_factorization(n, factor(n))

    def test_miller_rabin_against_sympy(self):
        rng = random.Random(2)
        cases = [0, 1, 2, 3, 4, 3215031751, 3825123056546413051, 318665857834031151167461, 2**61 - 1, 2**64 - 59]
        cases = [c for c in cases if c < 2**64] + [rng.getrandbits(64) for _ in range(3000)]
        cases += [sympy.randprime(2**40, 2**64) for _ in range(200)]
        for n in cases:
            self.assertEqual(bool(L.pg_is_prime(n)), sympy.isprime(n), n)

    def test_decompose_splits_and_rewrites(self):
        rng = random.Random(3)
        pool = (Val * 400)()
        L.pg_register_pool(pool, 400)
        want = []
        for i in range(400):
            n, d = rng.randint(-10**6, 10**6), rng.choice([360, 1000, 2**40, 65537 * 3, 2147483647 * 2147483629,
                                                          rng.randint(2, 2**62)])
            pool[i] = L.pg_from(n, d)
            want.append(Fraction(n, d))
        check_graph(*snapshot())
        L.pg_decompose_all()
        nodes, groups = snapshot()
        check_graph(nodes, groups)
        self.assertTrue(all(p for _, p, a in nodes if a), "a live node is not known prime after decomposition")
        for v, w in zip(pool, want):
            check_value(v, w, groups)  # values unchanged, inline edges rewritten


class TestCoprimeInvariant(PG):
    def test_random_entry(self):
        rng = random.Random(4)
        p = sympy.randprime(2**40, 2**41)
        dens = [7 * p, 11 * p, 12, 18, 8, 1000, 999, 2**62, 3**39, 6, 30, 210 * 11]
        dens += [rng.randint(2, 5000) for _ in range(300)]
        dens += [rng.randint(2, 2**62) for _ in range(100)]
        dens += [rng.choice([2, 3, 5, 7]) ** rng.randint(1, 20) * rng.randint(1, 100) for _ in range(100)]
        for i, d in enumerate(dens):
            self.assertNotEqual(L.pg_den_gid(d), BAD, d)
            if i % 50 == 0 or i < 12:
                check_graph(*snapshot())
        nodes, groups = snapshot()
        check_graph(nodes, groups)
        self.assertIn(p, [v for v, _, a in nodes if a], "gcd refinement did not isolate the shared large factor")
        L.pg_decompose_all()
        check_graph(*snapshot())


class TestOps(PG):
    def run_ops(self, decompose, cache, kinds, n=400, seed=5):
        G["pg_use_cache"].value = cache
        rng = random.Random(seed)
        xs = [rand_frac(rng, rng.choice(kinds)) for _ in range(n)]
        ys = [rand_frac(rng, rng.choice(kinds)) for _ in range(n)]
        va, vb = (Val * n)(), (Val * n)()
        L.pg_register_pool(va, n)  # later entries may split nodes: pooled values get their edges rewritten
        L.pg_register_pool(vb, n)
        for i in range(n):
            va[i], vb[i] = L.pg_from(*xs[i]), L.pg_from(*ys[i])
        if decompose:
            L.pg_decompose_all()  # splits, then syncs the pools
            for i in range(n):    # entry reduction, as the benchmark does it
                va[i], vb[i] = L.pg_reduce(va[i]), L.pg_reduce(vb[i])
        else:
            L.pg_sync_pools()
        _, groups = snapshot()
        for a, b, x, y in zip(va, vb, xs, ys):
            fa, fb = Fraction(*x), Fraction(*y)
            check_value(a, fa, groups)
            for op, want in ((L.pg_add, fa + fb), (L.pg_sub, fa - fb), (L.pg_mul, fa * fb)):
                before = overflows()
                r = op(a, b)
                value_or_overflow(r, want, before)
                if decompose and r.gid != BAD:
                    check_reduced(r)  # reduced inputs over prime nodes give results in lowest terms
            self.assertEqual(L.pg_cmp(a, b), (fa > fb) - (fa < fb))
            r = L.pg_reduce(a)
            check_value(r, fa)
        check_graph(*snapshot())

    def test_small_no_overflow(self):
        for dec in (0, 1):
            for cache in (0, 1):
                self.setUp()
                self.run_ops(dec, cache, ["small", "dec", "int"])
                self.assertEqual(overflows(), 0)

    def test_all_kinds(self):
        for dec in (0, 1):
            for cache in (0, 1):
                self.setUp()
                self.run_ops(dec, cache, KINDS, n=600, seed=6 + dec)

    def test_chains_of_ops(self):
        rng = random.Random(7)
        for cache in (0, 1):
            self.setUp()
            G["pg_use_cache"].value = cache
            x, fx = L.pg_from(1, 1), Fraction(1)
            for i in range(3000):
                n, d = rand_frac(rng, rng.choice(["small", "dec"]))
                if i == 1500:
                    L.pg_decompose_all()
                y, fy = L.pg_from(n % 1000, d), Fraction(n % 1000, d)
                op = rng.choice(["add", "sub", "mul"]) if abs(fx.numerator) < 10**6 else "sub"
                before = overflows()
                r = {"add": L.pg_add, "sub": L.pg_sub, "mul": L.pg_mul}[op](x, y)
                want = {"add": fx + fy, "sub": fx - fy, "mul": fx * fy}[op]
                value_or_overflow(r, want, before)
                if r.gid == BAD:
                    x, fx = L.pg_from(1, 1), Fraction(1)
                else:
                    x, fx = r, want
            check_graph(*snapshot())

    def test_mul_cancels(self):
        r = L.pg_mul(L.pg_from(10, 21), L.pg_from(7, 4))  # 10/21 * 7/4 = 5/6
        L.pg_decompose_all()
        r = L.pg_mul(L.pg_from(10, 21), L.pg_from(7, 4))
        check_value(r, Fraction(5, 6))
        self.assertEqual((r.num, r.den), (5, 6))
        s = L.pg_add(L.pg_from(1, 6), L.pg_from(1, 3))  # 1/2 after reduction by divisibility tests
        self.assertEqual((s.num, s.den), (1, 2))


class TestOverflow(PG):
    def test_detected_never_wraps(self):
        cases = [((I64MAX, 1), (1, 1), "add"), ((I64MIN, 1), (1, 1), "sub"), ((I64MAX, 3), (I64MAX, 5), "add"),
                 ((2**40, 1), (2**40, 1), "mul"), ((1, 2**62), (1, 3), "mul"), ((1, 2**40), (1, 3**30), "add"),
                 ((I64MAX, 7), (1, 11), "add")]
        for x, y, op in cases:
            self.setUp()
            a, b = L.pg_from(*x), L.pg_from(*y)
            f = {"add": L.pg_add, "sub": L.pg_sub, "mul": L.pg_mul}[op]
            want = {"add": Fraction(*x) + Fraction(*y), "sub": Fraction(*x) - Fraction(*y),
                    "mul": Fraction(*x) * Fraction(*y)}[op]
            r = f(a, b)
            self.assertEqual(r.gid, BAD, (x, y, op))
            self.assertGreater(overflows(), 0)
            self.assertEqual(L.pg_add(r, a).gid, BAD, "a bad value must poison")
        self.setUp()
        self.assertEqual(L.pg_den_gid(2**63), BAD)
        self.assertEqual(overflows(), 1)

    def test_uniform_array_overflow_poisons(self):
        a = make_arr([I64MAX, 1], [1, 1]); b = make_arr([1, 1], [1, 1]); r = empty_arr(2)
        L.pg_arr_add(C.byref(r), C.byref(a), C.byref(b))  # pg: per element
        self.assertGreater(overflows(), 0)
        self.assertEqual(L.pg_arr_get(C.byref(r), 0).gid, BAD)
        check_value(L.pg_arr_get(C.byref(r), 1), Fraction(2))
        G["pg_keep_uniform"].value = 1                     # pgu: the uniform loop poisons the whole result
        L.pg_arr_add(C.byref(r), C.byref(a), C.byref(b))
        self.assertEqual(L.pg_arr_get(C.byref(r), 1).gid, BAD)


class TestWide(PG):
    def test_nine_primes(self):
        """The W4 element that needed 9 primes: denominators 561 = 3*11*17, 910 = 2*5*7*13, 551 = 19*29."""
        for dec in (0, 1):
            self.setUp()
            a, b, c = L.pg_from(1, 561), L.pg_from(1, 910), L.pg_from(1, 551)
            if dec:
                L.pg_decompose_all()
                a, b, c = (L.pg_reduce(L.pg_from(*x)) for x in ((1, 561), (1, 910), (1, 551)))
            r = L.pg_add(L.pg_mul(a, b), c)
            check_value(r, Fraction(1, 561) * Fraction(1, 910) + Fraction(1, 551))
            if dec:
                self.assertEqual(r.ne, 9)
                check_reduced(r)
            A, B, Cc = make_arr([1], [561]), make_arr([1], [910]), make_arr([1], [551])
            R_ = empty_arr(1)
            L.pg_arr_fma(C.byref(R_), C.byref(A), C.byref(B), C.byref(Cc))
            check_value(L.pg_arr_get(C.byref(R_), 0), Fraction(1, 561) * Fraction(1, 910) + Fraction(1, 551))
            self.assertEqual(overflows(), 0)
            check_graph(*snapshot())

    def test_sum_overflow_reported(self):
        rng = random.Random(9)
        a = make_arr([rng.randint(1, 10**6) for _ in range(3000)], [rng.randint(1, 1000) for _ in range(3000)])
        L.pg_decompose_all()
        self.assertEqual(L.pg_arr_sum(C.byref(a)).gid, BAD)
        self.assertGreater(overflows(), 0)


class TestReducedOutputs(PG):
    def test_arrays_reduced(self):
        rng = random.Random(10)
        n = 2000
        for kind in ("dec", "small", "unrel"):
            self.setUp()
            data = [[rand_frac(rng, kind) for _ in range(n)] for _ in range(3)]
            if kind == "dec":
                data = [[(x // 1000, d) for x, d in col] for col in data]
            A, B, Cc = (make_arr([x for x, _ in col], [d for _, d in col]) for col in data)
            L.pg_decompose_all()
            for X in (A, B, Cc):
                L.pg_arr_reduce(C.byref(X))
            fa, fb, fc = ([Fraction(x, d) for x, d in col] for col in data)
            for v, w in zip(arr_vals(A), fa):
                check_value(v, w); check_reduced(v)
            for fn in (L.pg_arr_add, L.pg_arr_sub, L.pg_arr_mul):
                R_ = empty_arr(n)
                fn(C.byref(R_), C.byref(A), C.byref(B))
                for v in arr_vals(R_):
                    check_reduced(v)
            R_ = empty_arr(n)
            L.pg_arr_fma(C.byref(R_), C.byref(A), C.byref(B), C.byref(Cc))
            for v, w in zip(arr_vals(R_), [x * y + z for x, y, z in zip(fa, fb, fc)]):
                check_value(v, w); check_reduced(v)
            AA = empty_arr(n)
            L.pg_arr_add(C.byref(AA), C.byref(A), C.byref(A))  # same group everywhere: integer add, then tests
            for v, w in zip(arr_vals(AA), fa):
                check_value(v, 2 * w); check_reduced(v)
            if kind != "unrel":
                s = L.pg_arr_sum(C.byref(A))
                check_value(s, sum(fa, Fraction(0))); check_reduced(s)
            self.assertEqual(overflows(), 0)


def make_arr(nums, dens):
    a = Arr()
    L.pg_arr_alloc(C.byref(a), len(nums))
    L.pg_arr_enter(C.byref(a), (C.c_int64 * len(nums))(*nums), (C.c_uint64 * len(dens))(*dens))
    return a


def empty_arr(n):
    a = Arr()
    L.pg_arr_alloc(C.byref(a), n)
    return a


def arr_vals(a):
    return [L.pg_arr_get(C.byref(a), i) for i in range(a.n)]


class TestArrays(PG):
    def gen(self, rng, kind, n):
        xs = [rand_frac(rng, kind) for _ in range(n)]
        return [x for x, _ in xs], [d for _, d in xs]

    def test_elementwise_and_sum(self):
        rng = random.Random(8)
        n = 2000
        for kind in ("dec", "small", "unrel"):
            for keep in (0, 1):
                for dec in (0, 1):
                    self.setUp()
                    G["pg_keep_uniform"].value = keep
                    data = [self.gen(rng, kind, n) for _ in range(3)]
                    if kind == "dec":
                        data = [([v // 1000 for v in nums], dens) for nums, dens in data]  # keep a*b+c in 64 bits
                    A, B, Cc = (make_arr(*d) for d in data)
                    if dec:
                        L.pg_decompose_all()
                    fa, fb, fc = ([Fraction(x, y) for x, y in zip(*d)] for d in data)
                    for fn, want in ((L.pg_arr_add, [x + y for x, y in zip(fa, fb)]),
                                     (L.pg_arr_sub, [x - y for x, y in zip(fa, fb)]),
                                     (L.pg_arr_mul, [x * y for x, y in zip(fa, fb)])):
                        R = empty_arr(n)
                        fn(C.byref(R), C.byref(A), C.byref(B))
                        for v, w in zip(arr_vals(R), want):
                            check_value(v, w)
                    R = empty_arr(n)
                    L.pg_arr_fma(C.byref(R), C.byref(A), C.byref(B), C.byref(Cc))
                    for v, w in zip(arr_vals(R), [x * y + z for x, y, z in zip(fa, fb, fc)]):
                        check_value(v, w)
                    if kind != "unrel":
                        check_value(L.pg_arr_sum(C.byref(A)), sum(fa, Fraction(0)))
                    L.pg_arr_fma(C.byref(A), C.byref(A), C.byref(B), C.byref(Cc))  # in place
                    for v, w in zip(arr_vals(A), [x * y + z for x, y, z in zip(fa, fb, fc)]):
                        check_value(v, w)
                    self.assertEqual(overflows(), 0, (kind, keep, dec))
                    check_graph(*snapshot())

    def test_sum_check_catches_wrong(self):
        a = make_arr([1, 2, 3], [2, 3, 4])
        s = L.pg_arr_sum(C.byref(a))
        check_value(s, Fraction(1, 2) + Fraction(2, 3) + Fraction(3, 4))
        with self.assertRaises(AssertionError):
            check_value(s, Fraction(1, 2) + Fraction(2, 3) + Fraction(3, 5))


if __name__ == "__main__":
    unittest.main(verbosity=2)
