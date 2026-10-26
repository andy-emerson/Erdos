# Tests

Every test compares against an independent reference: Python's `Fraction`, complex arithmetic on pairs of Fractions, FLINT, NumPy integers, or a brute-force search.

- **Encoding.** Every pattern of a 16-bit word decodes and every value has one canonical encoding; boundary values, random round trips and the Q2 bit order.
- **Arithmetic.** Random scalar products, quotients and sums against complex Fraction arithmetic; zero, division by zero and ordering.
- **Vectors.** Both full layouts, with data at the int64 extremes and denominators up to $2^{64}$ and beyond, against `Fraction` and against the scalar `Q`; every tier and overflow path reached; complex columns; exact division by both methods, including negative values and the int64 extremes; the three fused layouts against NumPy.
- **The exact product.** Against FLINT, with every recombination path reached.
- **Precision code.** Neighbours, rounding and the per-magnitude worst case against brute force on a 10-bit word.

```python {file=build/test_scalar.py}
import itertools
import math
import random
from fractions import Fraction

from q2 import Q2, turn_of
from q72 import Q, CrossAxis, fits, encode, decode, is_canonical, U64_MAX


# ---------------------------------------------------------------- Q2

def test_g8_bit_order_and_group():
    assert [turn_of(b) for b in (0x00, 0x40, 0x80, 0xC0)] == [0, 1, 2, 3], "bit 7 sign, bit 6 imaginary"
    assert Q2.from_byte(0x80).negative and not Q2.from_byte(0x80).imaginary
    assert Q2.from_byte(0x40).imaginary and not Q2.from_byte(0x40).negative
    for a, b in itertools.product(range(4), repeat=2):
        assert (Q2(a) * Q2(b)).turn == (a + b) % 4
        assert (Q2(a) / Q2(b)).turn == (a - b) % 4
    i = Q2(1)
    assert i * i == -Q2(0) and i.conj() == Q2(3) and i.reciprocal() == Q2(3)
    assert Q2.from_byte(0b10_101010) == Q2.from_byte(0b10_000000), "the slash means nothing in a Q2 alone"


def complex_value(turn, p, q):
    v = Fraction(p, q) * (-1 if turn >= 2 else 1)
    return (v, Fraction(0)) if turn % 2 == 0 else (Fraction(0), v)


def test_every_pattern_decodes_small_word():
    """On a 16-bit word with a 4-bit slash, every (unit, slash, word) decodes, and canonical words are unique."""
    seen = {}
    for code in range(256):
        if code & 0x30:
            continue
        for u in range(1 << 16):
            t, p, q = decode(code, u, 16)
            assert q >= 1
            if is_canonical(code, u, 16):
                key = complex_value(t, p, q)
                assert key not in seen, (code, u, seen.get(key))
                seen[key] = (code, u)
    for (re, im), (code, u) in itertools.islice(seen.items(), 0, None, 997):
        x = re if im == 0 else im
        turn = (2 if x < 0 else 0) + (1 if im else 0)
        assert encode(turn, abs(x).numerator, abs(x).denominator, 16) == (code, u)


def test_encode_boundaries():
    assert fits(U64_MAX, 1) and not fits(2**64, 1), "slash 63: the whole word is the numerator, q = 1"
    assert fits(1, 2**63) and not fits(1, 2**63 + 1), "slash 0: one numerator bit, q - 1 in 63 bits"
    assert fits(2**31 - 1, 2**33) and not fits(2**32 - 1, 2**32 + 2)
    cases = [(1, 1), (U64_MAX, 1), (1, 2**63), (2**63 - 1, 2), (5, 2**60 + 1), (7, 9), (2**39 + 1, 2**24 - 3)]
    for p, q in cases:
        assert math.gcd(p, q) == 1 and fits(p, q)
        for turn in range(4):
            code, u = encode(turn, p, q)
            assert decode(code, u) == (turn, p, q) and is_canonical(code, u)
            assert u >> 63 == 1 and code & 63 == p.bit_length() - 1 and code >> 6 == turn
    assert encode(3, 0, 1) == (0, 0) and decode(0, 0) == (0, 0, 1)
    for code in (0x00, 0x7F, 0xC5):
        assert decode(code, 0)[1] == 0, "a U64 of 0 is zero whatever the Q2 holds"
    assert not is_canonical(0xC0, 0), "zero's canonical Q2 is 0"
    assert decode(63, 5) == (0, 5, 1), "slash 63 is an integer"
    assert decode(0, 1) == (0, 0, 1), "slash 0 with a zero top bit is zero (0/2), not canonical"
    for p, q in [(2, 4), (6, 9)]:
        try:
            encode(0, p, q)
        except ValueError:
            continue
        raise AssertionError((p, q))


def test_word_roundtrip_random():
    rng = random.Random(7)
    for _ in range(20000):
        bp = rng.randint(1, 64)
        p = rng.randrange(1 << (bp - 1), 1 << bp)
        bq = rng.randint(0, 64 - bp)
        q = rng.randrange(1 << bq) + 1 if bq else 1
        g = math.gcd(p, q)
        x = Q.of(Fraction(p // g, q // g), rng.randrange(4))
        w = x.word()
        assert w is not None and Q.from_word(*w) == x and is_canonical(*w)


def rand_q(rng):
    v = Fraction(rng.randint(1, 10**rng.randint(1, 25)), rng.randint(1, 10**rng.randint(1, 25)))
    return Q.of(v, rng.choice([0, 1, 2, 3]))


def test_arithmetic_matches_complex_fractions():
    rng = random.Random(11)
    for _ in range(20000):
        a, b = rand_q(rng), rand_q(rng)
        (ar, ai), (br, bi) = a.parts(), b.parts()
        assert (a * b).parts() == (ar * br - ai * bi, ar * bi + ai * br)
        den = br * br + bi * bi
        assert (a / b).parts() == ((ar * br + ai * bi) / den, (ai * br - ar * bi) / den)
        try:
            s = a + b
        except CrossAxis:
            assert a.axis != b.axis
            continue
        assert s.parts() == (ar + br, ai + bi) and (a - b).parts() == (ar - br, ai - bi)


def test_zero_and_division():
    z = Q.of(0)
    assert z.turn == 0 and (Q.of(-3) * z).turn == 0 and (Q.imag(5) - Q.imag(5)).turn == 0
    assert (Q.imag(5) - Q.imag(5)).word() == (0, 0) and z + Q.imag(2) == Q.imag(2)
    for x in (Q.of(1), z, Q.imag(-2)):
        try:
            x / z
        except ZeroDivisionError:
            continue
        raise AssertionError("division by zero is an error")
    assert Q.of(3) == 3 and len({Q.of(3), 3, Fraction(3)}) == 1 and Q.of(0) == 0
    assert Q.of(1) != None and Q.of(1) != 1.5  # noqa: E711
    try:
        Q.of(1) + Q.imag(1)
    except CrossAxis:
        pass
    else:
        raise AssertionError("a real plus an imaginary is two pairs")


def test_ordering():
    xs = [Q.of(-10**30), Q.of(Fraction(-1, 3)), Q.of(0), Q.of(Fraction(1, 10**30)), Q.of(5)]
    for a, b in zip(xs, xs[1:]):
        assert a < b
    try:
        Q.imag(1) < Q.of(1)
    except TypeError:
        pass
    else:
        raise AssertionError("imaginary values are not ordered")
```

```python {file=build/test_columns.py}
import math
import random
from fractions import Fraction

import numpy as np
import flint

import kern
from q72 import Q, CrossAxis
from qvec import QVec
from svec import SVec
import svec as SV
import skern
from cxcol import CxCol
from gemm import exact_gemm, limb_bits, Int128Matrix

KINDS = (QVec, SVec)
INT64_MIN = -(2**63)


def fr(col):
    return [v.fraction() for v in col.values()]


def col_of(C, qs):
    """A vector holding the scalar values qs (for SVec, all on one axis)."""
    D = math.lcm(1, *[q.mag.denominator for q in qs])
    if C is QVec:
        return QVec.from_parts(np.array([q.turn for q in qs], np.uint8), [(q.mag * D).numerator for q in qs], D)
    axes = {q.axis for q in qs if q.mag}
    assert len(axes) <= 1
    axis = axes.pop() if axes else 0
    return SVec.from_ints([(sum(q.parts()) * D / (1 if axis == 0 else 1)).numerator if axis == 0
                           else (q.parts()[1] * D).numerator for q in qs], D, axis)


def cx(C, vals, axes):
    """A complex column of values v on the given axes: one QVec, or real and imaginary SVecs."""
    qs = [Q.of(v, t) for v, t in zip(vals, axes)]
    if C is QVec:
        return CxCol("one", col_of(QVec, qs))
    re = col_of(SVec, [q if q.axis == 0 else Q.of(0) for q in qs])
    im = col_of(SVec, [Q.of(q.parts()[1]) for q in qs])
    return CxCol("two", re, im).narrow()


def data(n, seed, bound=10**7, extremes=True):
    rng = np.random.default_rng(seed)
    x = rng.integers(-bound, bound + 1, size=n, dtype=np.int64)
    x[rng.random(n) < 0.05] = 0
    if extremes:
        x[:6] = [INT64_MIN, 2**63 - 1, -(2**62), 2**62 - 1, 1, -1]
    return x


DENS = (1, 7, 1000, 10**6, 2**61 - 1, 2**63 - 1, 2**64 - 1, 10**18 + 9)


def test_decimals_roundtrip_and_canonical():
    for C in KINDS:
        for den in DENS:
            x = data(500, den % 97)
            col = C.decimals(x, den)
            assert fr(col) == [Fraction(int(v), den) for v in x], (C.__name__, den)
            g = math.gcd(den, *[int(v) for v in x])
            assert col.den == den // g, "construction reduces by the common factor"


def check_ops(C, x, dx, y, dy):
    A, B = C.decimals(x, dx), C.decimals(y, dy)
    fa = [Fraction(int(v), dx) for v in x]
    fb = [Fraction(int(v), dy) for v in y]
    assert fr(A + B) == [a + b for a, b in zip(fa, fb)]
    assert fr(A - B) == [a - b for a, b in zip(fa, fb)]
    P = A * B
    assert fr(P) == [a * b for a, b in zip(fa, fb)]
    assert fr(P * B - A) == [a * b * b - a for a, b in zip(fa, fb)]
    assert A.total() == sum(fa) and P.total() == sum(a * b for a, b in zip(fa, fb))
    n = len(fa)
    m = sum(fa) / n
    assert A.mean() == m and A.var() == sum((a - m) ** 2 for a in fa) / (n - 1)


def test_elementwise_ops_small_and_overflowing():
    cases = [(data(400, 1, 10**4, False), 1000, data(400, 2, 10**4, False), 100),
             (data(400, 3, 10**9, False), 10**6, data(400, 4, 10**9, False), 7),
             (data(400, 5), 1, data(400, 6), 1),
             (data(400, 7), 10**6, data(400, 8), 2**61 - 1),
             (data(400, 9), 3, data(400, 10), 10**18 + 9)]
    for C in KINDS:
        for x, dx, y, dy in cases:
            check_ops(C, x, dx, y, dy)


def test_paths_are_reached():
    x, y = data(300, 11), data(300, 12)
    N = QVec.decimals(x, 1) * QVec.decimals(y, 1)
    assert N.tier == "n128" and (N * N).tier == "big" and ((N * N) - (N * N)).tier == "n64"
    big = QVec.from_parts(np.zeros(10, np.uint8), [2**64 - 1] * 10, 1)
    assert (big + big).tier == "n128" and (big + big).total() == 20 * (2**64 - 1)
    *_, hb = kern.nat_sums_fast(big.turn, big.mag)
    assert int(hb).bit_length() + len(big).bit_length() > 64, "the vector sum takes the carrying kernel"
    small = QVec.decimals(np.array([3, -4, 5]), 1)
    assert (small * small).tier == "n64", "products known to fit one word skip the 128-bit kernel"
    S = SVec.decimals(x, 1) * SVec.decimals(y, 1)
    assert S.tier == "s128" and fr(S) == [Fraction(int(a) * int(b)) for a, b in zip(x, y)]
    assert (S * S).tier == "big" and ((S * S) - (S * S)).tier == "s64"
    assert (SVec.decimals(np.array([3, -4, 5]), 1) * SVec.decimals(np.array([3, -4, 5]), 1)).tier == "s64"
    top = SVec.from_ints([2**63 - 1] * 9 + [-(2**63)], 1)
    assert (top + top).tier == "s128" and (top + top).total() == 2 * (9 * (2**63 - 1) - 2**63)
    assert (top - top.rotate(2)).tier == "s128"
    s, h = skern.s_sums_fast(top.val)
    assert int(h).bit_length() + len(top).bit_length() > 63, "the signed sum takes the 128-bit kernel"
    w = SVec.from_ints([2**126, -(2**126), 5], 1)
    assert w.tier == "s128" and (w + w).tier == "big" and (w + w).total() == 10
    assert w.scale_int(3).tier == "big" and fr(w.scale_int(3)) == [3 * 2**126, -3 * 2**126, 15]
    assert SVec.from_ints([2**70, -3], 1).scale_int(5).tier == "s128"
    assert SVec.from_ints([2**70, -3], 1).total() == 2**70 - 3
    assert SVec.from_ints([-(2**63), 1], 1).negate().tier == "s128", "negating -2**63 widens"


def test_exact_division_methods_agree():
    rng = np.random.default_rng(40)
    for g in (3, 10, 1000, 2**10, 3 * 2**20, 10**9 + 7, 2**62 - 57):
        q = rng.integers(-(2**63 // g), (2**63 - 1) // g, size=5000, dtype=np.int64)
        q[:2] = [-(2**63 // g), (2**63 - 1) // g]
        x = q * np.int64(g)
        outs = []
        for method in ("inverse", "hardware"):
            SV.DIVISION = method
            outs.append(SV.exact_divide(x, g))
        SV.DIVISION = "inverse"
        assert np.array_equal(outs[0], q) and np.array_equal(outs[1], q), g
    for method in ("inverse", "hardware"):
        SV.DIVISION = method
        col = SVec.decimals(np.array([-1500, 2500, 0, -(2**62)]), 500)
        assert col.den == 125 and fr(col) == [-3, 5, 0, Fraction(-(2**62), 500)]
    SV.DIVISION = "inverse"


def test_reductions_and_groups():
    rng = np.random.default_rng(15)
    for C in KINDS:
        for x, den in ((data(5000, 16, 10**7, False), 1000), (data(3000, 17), 10**6),
                       (10**10 + data(4000, 18, 10**4, False), 1000)):
            col = C.decimals(x, den)
            f = [Fraction(int(v), den) for v in x]
            keys = rng.integers(0, 37, size=len(x))
            sums = col.group_sum(keys, 37)
            mv = col.group_mean_var(keys, 37)
            for g in range(37):
                xs = [v for v, k in zip(f, keys) if k == g]
                m = sum(xs, Fraction(0)) / len(xs)
                assert sums[g] == sum(xs, Fraction(0))
                assert mv[g] == (m, sum((v - m) ** 2 for v in xs) / (len(xs) - 1))
            n = len(f)
            m = sum(f) / n
            assert col.var() == sum((v - m) ** 2 for v in f) / (n - 1)
        for bad in (lambda: C.decimals(np.array([1, 2, 3]), 1).group_sum(np.array([0, 1, 3]), 3),):
            try:
                bad()
            except IndexError:
                continue
            raise AssertionError("out-of-range group key")


def test_greater_and_take():
    x = data(2000, 19)
    for C in KINDS:
        for den in (1, 1000, 2**61 - 1):
            col = C.decimals(x, den)
            f = [Fraction(int(v), den) for v in x]
            for q in (0, Fraction(1, 3), Fraction(-7, 2), 10**5, -(10**30), 10**30, Fraction(2**70 + 1, 3)):
                mask = col.greater(q)
                assert mask.tolist() == [v > q for v in f], (C.__name__, den, q)
                assert fr(col.take(mask)) == [v for v in f if v > q]
        try:
            C.decimals(np.array([1, 2]), 1).rotate(1).greater(0)
        except TypeError:
            pass
        else:
            raise AssertionError("imaginary values are not ordered")


def ref_mul(a, b):
    return (a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0])


def test_complex_columns():
    rng = random.Random(5)
    for trial in range(40):
        n = rng.randint(1, 30)
        big = trial % 3 == 0
        bound = 2**62 if big else 10**6
        av = [Fraction(rng.randrange(-bound, bound), rng.choice([1, 10, 1000, 7])) for _ in range(n)]
        bv = [Fraction(rng.randrange(-bound, bound), rng.choice([1, 10, 1000, 7])) for _ in range(n)]
        at, bt = [rng.randrange(2) for _ in range(n)], [rng.randrange(2) for _ in range(n)]
        ra = [(v, 0) if t == 0 else (0, v) for v, t in zip(av, at)]
        rb = [(v, 0) if t == 0 else (0, v) for v, t in zip(bv, bt)]
        for C in KINDS:
            A, B = cx(C, av, at), cx(C, bv, bt)
            add = [(x[0] + y[0], x[1] + y[1]) for x, y in zip(ra, rb)]
            for X, want in ((A + B, add), (A - B, [(x[0] - y[0], x[1] - y[1]) for x, y in zip(ra, rb)]),
                            (A * B, [ref_mul(x, y) for x, y in zip(ra, rb)]),
                            ((A + B) * (A - B), [ref_mul(s, (x[0] - y[0], x[1] - y[1])) for s, x, y in zip(add, ra, rb)]),
                            ((A + B).rotate(trial), [[(s[0], s[1]), (-s[1], s[0]), (-s[0], -s[1]), (s[1], -s[0])][trial % 4] for s in add])):
                assert [tuple(map(Fraction, g)) for g in X.values()] == want, C.__name__
                assert tuple(X.total2()) == (sum(w[0] for w in want), sum(w[1] for w in want))
                assert X.abs2().total2() == (sum(w[0] ** 2 + w[1] ** 2 for w in want), 0)
    for C in KINDS:
        a = C.decimals(np.array([1, 2]), 1)
        try:
            a + a.rotate(1)
        except CrossAxis:
            continue
        raise AssertionError("a column refuses real plus imaginary")


def test_zeros_have_no_direction_and_canonical_words():
    for C in KINDS:
        x = C.decimals(np.array([4, -4, 0, 9]), 1)
        z = x - x
        assert all(v.turn == 0 and v.mag == 0 for v in z.values())
        assert all(v.turn == 0 for v in (x.rotate(1) - x.rotate(1)).values())
        p = C.decimals(np.array([0, 0, -3]), 1) * C.decimals(np.array([-5, 7, 0]), 1).rotate(1)
        assert all(v.turn == 0 for v in p.values()), "a zero product has turn 0"
        if C is QVec:
            assert [int(t) for t in z.turn] == [0, 0, 0, 0] and [int(t) for t in p.turn] == [0, 0, 0]
        else:
            assert z.val.tolist() == [0, 0, 0, 0] and p.val.tolist() == [0, 0, 0]
            assert (x.rotate(1) + (x - x)).values() == x.rotate(1).values(), "a zero vector is on every axis"


def test_lengths_and_edges():
    for C in KINDS:
        a, b = C.decimals(np.arange(1, 9), 1), C.decimals(np.array([2, 3]), 1)
        for op in (lambda: a * b, lambda: a + b, lambda: a - b, lambda: a.group_sum(np.array([0, 1]), 2)):
            try:
                op()
            except ValueError:
                continue
            raise AssertionError(C.__name__)
        for den in (0, -10):
            try:
                C.decimals(np.array([5]), den)
            except ValueError:
                continue
            raise AssertionError(C.__name__)
        try:
            C.decimals(np.array([7]), 3).var()
        except ValueError:
            pass
        else:
            raise AssertionError("the sample variance of one value")


def test_random_vectors_against_q():
    rng = random.Random(3)

    def rv():
        if rng.random() < 0.1:
            return Fraction(0)
        kind = rng.random()
        if kind < 0.4:
            return Fraction(rng.randrange(1, 1 << rng.randint(1, 66)), rng.randrange(1, 1 << rng.randint(1, 66)))
        if kind < 0.7:
            return Fraction(rng.randrange(1, 10**rng.randint(1, 19)), 10**rng.randint(0, 18))
        return Fraction(rng.choice([2**63 - 1, 2**63, 2**64 - 1, 1, 3]), rng.choice([1, 2**63, 2**63 + 1, 2, 3]))

    for trial in range(150):
        n = rng.randint(1, 40)
        for C in KINDS:
            if C is QVec:
                turns_a = [rng.choice([0, 2]) if rng.random() < 0.7 else rng.randrange(4) for _ in range(n)]
                turns_b = [rng.choice([0, 2]) if rng.random() < 0.7 else rng.randrange(4) for _ in range(n)]
            else:
                ax_a, ax_b = rng.randrange(2), rng.randrange(2)
                turns_a = [ax_a + rng.choice([0, 2]) for _ in range(n)]
                turns_b = [ax_b + rng.choice([0, 2]) for _ in range(n)]
            A = [Q(t, rv()) for t in turns_a]
            B = [Q(t, rv()) for t in turns_b]
            CA, CB = col_of(C, A), col_of(C, B)
            for name, op in (("mul", lambda a, b: a * b), ("add", lambda a, b: a + b), ("sub", lambda a, b: a - b)):
                want = []
                for a, b in zip(A, B):
                    try:
                        want.append(op(a, b))
                    except CrossAxis:
                        want.append(CrossAxis)
                # one axis per signed vector: a real vector plus an imaginary one is two vectors, unless one is zero
                vector_cross = (C is SVec and name != "mul" and CA.axis != CB.axis
                                and not CA.all_zero() and not CB.all_zero())
                try:
                    out = op(CA, CB)
                except CrossAxis:
                    assert CrossAxis in want or vector_cross, C.__name__
                    continue
                assert CrossAxis not in want and not vector_cross
                got = out.values()
                assert got == want and [g.turn for g in got] == [w.turn for w in want], C.__name__
            for k in range(4):
                assert CA.rotate(k).values() == [Q(a.turn + k, a.mag) for a in A]
            assert CA.total2() == (sum(a.parts()[0] for a in A), sum(a.parts()[1] for a in A))


def test_exact_gemm():
    rng = np.random.default_rng(31)
    kinds = set()
    for n, bound in ((3, 10), (50, 10**6), (200, 10**6), (64, 2**40), (40, 2**62), (17, 2**63 - 1)):
        A = rng.integers(-bound, bound, size=(n + 3, n), dtype=np.int64)
        B = rng.integers(-bound, bound, size=(n, n - 1 if n > 1 else 1), dtype=np.int64)
        if bound >= 2**62:
            A[0, 0] = INT64_MIN
        want = flint.fmpz_mat(A.tolist()) * flint.fmpz_mat(B.tolist())
        got = exact_gemm(A, B)
        rows = got.to_python() if isinstance(got, Int128Matrix) else [[int(v) for v in row] for row in got.tolist()]
        assert rows == [[int(want[i, j]) for j in range(want.ncols())] for i in range(want.nrows())], (n, bound)
        kind = "i128" if isinstance(got, Int128Matrix) else "obj" if got.dtype == object else "i64"
        bnd = int(np.abs(A.astype(object)).max()) * int(np.abs(B.astype(object)).max()) * n
        assert kind == ("i64" if bnd < 2**62 else "i128" if bnd < 2**126 else "obj"), (n, bound, kind)
        kinds.add(kind)
    assert kinds == {"i64", "i128", "obj"}, "every recombination path is reached"
    assert limb_bits(500) == 22 and limb_bits(2**20) == 16


def test_select_from_parts_and_huge_denominators():
    v = QVec.from_parts(np.array([1, 3, 2, 0], np.uint8), [0, 5, 0, 7], 3)
    assert [int(t) for t in v.turn] == [0, 3, 0, 0], "from_parts gives zeros turn 0"
    for tier_mag in ([1, 2, 3, 4], [2**64 - 1, 1, 2**70, 5]):
        a = QVec.from_parts(np.array([0, 2, 0, 2], np.uint8), tier_mag, 7)
        b = QVec.decimals(np.array([10, -20, 30, -40]), 100)
        mask = np.array([True, False, False, True])
        got = a.select(mask, b)
        want = [y if m else x for x, y, m in zip(a.values(), b.values(), mask)]
        assert got.values() == want
    a = SVec.from_ints([1, -2, 3, 2**70], 7)
    b = SVec.decimals(np.array([10, -20, 30, -40]), 100)
    mask = np.array([True, False, False, True])
    assert a.select(mask, b).values() == [y if m else x for x, y, m in zip(a.values(), b.values(), mask)]
    den = 3 * 2**64 + 1
    for C in KINDS:
        col = C.decimals(np.array([1, -5, 0, 2**62]), den)
        assert [v.fraction() for v in col.values()] == [Fraction(1, den), Fraction(-5, den), 0, Fraction(2**62, den)]
        assert (col + col).total() == Fraction(2 * (2**62 - 4), den)


def test_layouts_agree():
    import layouts as L
    rng = np.random.default_rng(50)
    for n in (1, 5, 1003):
        a, b, c = (rng.integers(-10**7, 10**7 + 1, size=n) for _ in range(3))
        a[: min(n, 3)] = 0
        keys = rng.integers(0, 13, size=n)
        want_fma = a * b + c * 1000
        g = np.zeros(13, dtype=np.int64)
        np.add.at(g, keys, a)
        for kind in L.KINDS:
            A, B, C = (L.Layout(kind, v) for v in (a, b, c))
            assert np.array_equal(A.signed(), a), kind
            assert A.sum() == a.sum()
            assert np.array_equal(A.fma(B, C, 1000).signed(), want_fma), kind
            assert np.array_equal(A.group_sum(keys, 13).view(np.int64), g), kind
            s, qh, ql = A.moments()
            assert s == a.sum() and (int(qh) << 32) + int(ql) == int((a.astype(object) ** 2).sum()), kind
        assert L.Layout("packed turns", a).nbytes() == 8 * n + (n + 3) // 4


def test_signed_vector_edges():
    from svec import SVec
    big = SVec.from_ints([2**130, -5, 0, 7], 3)
    mid = SVec.from_ints([2**70, -(2**70), 1, 0], 3)
    small = SVec.decimals(np.array([1, 2, 3, 4]), 3)
    mask = np.array([True, False, True, False])
    for a, b in ((big, mid), (mid, small), (small, big)):
        got = a.select(mask, b)
        assert got.values() == [y if m else x for x, y, m in zip(a.values(), b.values(), mask)]
        z = a.zero_where(mask)
        assert z.values() == [Q.of(0) if m else x for x, m in zip(a.values(), mask)]
    zero_im = SVec.decimals(np.zeros(4, np.int64), 1).rotate(1)
    assert small.select(mask, zero_im).values() == [Q.of(0) if m else x for x, m in zip(small.values(), mask)]
    try:
        small.select(mask, small.rotate(1))
    except CrossAxis:
        pass
    else:
        raise AssertionError("select between nonzero real and imaginary vectors")
    for C in KINDS:
        for bad in (np.array([1.5, 2.0]), [0.1]):
            try:
                C.decimals(bad, 10)
            except TypeError:
                continue
            raise AssertionError("float numerators are refused, not truncated")


def test_gemm_degenerate():
    A = np.zeros((3, 4), np.int64)
    B = np.arange(8, dtype=np.int64).reshape(4, 2)
    assert exact_gemm(A, B).tolist() == [[0, 0]] * 3
    assert exact_gemm(np.zeros((2, 0), np.int64), np.zeros((0, 3), np.int64)).tolist() == [[0, 0, 0]] * 2


def test_bench_checks_reject_wrong_answers():
    import run_bench as R
    nums = np.array([3, -4, 5], np.int64)
    exact = nums.astype(object) * 10
    for C in KINDS:
        col = C.decimals(nums, 1)
        assert R.full_check(col, exact, 10)
        assert not R.full_check(col, exact + np.array([0, 1, 0], dtype=object), 10)
        assert not R.full_check(col.rotate(1), exact, 10), "an imaginary vector is not a real answer"
    objs = R.objects(R.GMP, nums, 1)
    assert R.objects_check(objs, exact, 10) and not R.objects_check(objs, exact * 2, 10)


def test_layouts_at_the_proven_limit():
    import layouts as L
    a = np.array([2**30 - 1, -(2**30 - 1), 0, 1, -1, 2**29], np.int64)
    b = np.array([-(2**30 - 1), -(2**30 - 1), 5, 0, -1, 2**30 - 1], np.int64)
    c = np.array([2**40, -(2**40), 0, -1, 0, 3], np.int64)
    want = a.astype(object) * b.astype(object) + c.astype(object) * 1000
    for kind in L.KINDS:
        A, B_, C_ = (L.Layout(kind, v) for v in (a, b, c))
        assert list(A.fma(B_, C_, 1000).signed().astype(object)) == list(want), kind
        s, qh, ql = A.moments()
        assert int(s) == int(a.sum()) and (int(qh) << 32) + int(ql) == sum(int(v) ** 2 for v in a), kind
```

```python {file=build/test_precision.py}
import bisect
import math
import random
from fractions import Fraction

import precision as P
from q72 import fits


def all_fitting(word):
    out = {Fraction(0)}
    for p in range(1, 1 << word):
        for q in range(1, (1 << (word - p.bit_length())) + 1):
            if math.gcd(p, q) == 1 and fits(p, q, word):
                out.add(Fraction(p, q))
    return sorted(out)


def test_neighbours_match_brute_force_small_word():
    word = 10
    table = all_fitting(word)
    for i in range(1, len(table) - 1, 7):
        x = table[i]
        pred, succ = P.neighbours(x.numerator, x.denominator, word)
        assert pred == table[i - 1] and succ == table[i + 1], (x, pred, succ, table[i - 1], table[i + 1])
    assert P.neighbours(table[-1].numerator, table[-1].denominator, word)[1] is None, "nothing above the largest"


def test_rounding_matches_brute_force_small_word():
    word = 10
    table = all_fitting(word)
    rng = random.Random(5)
    xs = [Fraction(rng.randint(1, 10**6), rng.randint(1, 10**6)) * rng.choice([1, 7, 1000]) for _ in range(400)]
    xs += [(a + b) / 2 for a, b in zip(table, table[1:])][::11]
    for x in xs:
        r = P.round_q72(x, word)
        if x > 2**word - 1:
            assert r is None
            continue
        i = bisect.bisect_left(table, x)
        best = min(table[max(0, i - 2):i + 2], key=lambda c: (abs(c - x), c.denominator, c))
        assert r == best, (x, r, best)


def test_round_float_and_formats():
    assert P.round_float(Fraction(1, 3), 53) == Fraction(1 / 3)
    for x in (Fraction(2, 7), Fraction(10**20 + 3, 9), Fraction(-5, 11)):
        assert P.round_float(x, 53) == Fraction(float(x))
    assert P.round_float(Fraction(3), 1) == 4 and P.round_float(Fraction(5), 2) == 4, "ties to even"
    assert P.exact_decimal64(Fraction(1, 10)) and not P.exact_decimal64(Fraction(1, 3))
    assert P.exact_decimal64(Fraction(10**16 - 1)) and not P.exact_decimal64(Fraction(10**16 + 1))
    assert P.exact_binary64(Fraction(1, 2)) and not P.exact_binary64(Fraction(1, 10))
    assert P.exact_q72(Fraction(1, 3)) and not P.exact_q72(Fraction(1, 2**64))
    p, total = P.float_bits_to_match(2.0**-52)
    assert (p, total) == (52, 60)


def test_worst_case_profile_matches_brute_force_small_word():
    """Per-window worst half-gap on a 10-bit word, against every fitting value in the window."""
    word = 10
    table = all_fitting(word)
    rows = P.worst_case_profile(exps=range(-2, 3), word=word)
    for row in rows:
        target = Fraction(10) ** row["exp"]
        lo, hi = target / 10, target * 10
        best = Fraction(0)
        for i in range(1, len(table)):
            x = table[i]
            if not lo <= x <= hi:
                continue
            gaps = [x - table[i - 1]] + ([table[i + 1] - x] if i + 1 < len(table) else [])
            best = max(best, max(gaps) / 2 / x)
        assert math.isclose(row["worst"], float(best), rel_tol=1e-12), (row, float(best))


def test_bits_to_match_and_rates():
    rng = random.Random(9)
    for _ in range(200):
        err = 2.0 ** -rng.uniform(5, 100)
        p, total = P.float_bits_to_match(err)
        assert 2.0 ** -p <= err < 2.0 ** -(p - 1), "smallest significand whose bound 2**-p meets err"
        assert total == 1 + 8 + p - 1
    target = 1e-12
    p, _ = P.float_median_bits(target)
    xs = P.random_reals(0, 400, random.Random(41))
    med = lambda q: sorted(float(abs(P.round_float(x, q) - x) / x) for x in xs)[200]
    assert med(p) <= target * 1.0000001 and med(p - 1) > target
    assert P.exact_decimal64(Fraction(10) ** 384) and not P.exact_decimal64(Fraction(10) ** 385)
    assert P.exact_decimal64(Fraction(1, 10**398)) and not P.exact_decimal64(Fraction(1, 10**399))
    rates = {r["values"]: r for r in P.exactness_rates(n=300)}
    assert rates["money, 2 places, below 10^9"]["q72"] == 1.0
    assert rates["ratios a/b of integers up to 1000"]["q72"] == 1.0
    assert rates["binary64 values in [0, 1) times 10^k, k in -6..6"]["binary64"] == 1.0
    typ = P.typical_errors(samples=40, exps=[0])[0]
    assert typ["q72_median"] < typ["f64_median"]
```
