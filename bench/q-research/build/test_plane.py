import math
import random
from fractions import Fraction

import numpy as np

import reuse  # noqa: F401
import plane as P


def test_every_pattern_decodes_and_canonical_forms_are_unique():
    w = 8
    k = w.bit_length()
    seen = {}
    for header in range(1 << (2 * k + 2)):
        for word in range(1 << w):
            a, b, c = P.plane_decode(header, word, w)
            assert c >= 1
            z = (Fraction(a, c), Fraction(b, c))
            enc = P.plane_encode(a, b, c, w)
            assert enc is not None, "a decoded value always fits"
            assert P.plane_decode(*enc, w) == tuple(x // math.gcd(math.gcd(abs(a), abs(b)), c) for x in (a, b, c))
            seen.setdefault(enc, z)
            assert seen[enc] == z
    # distinct canonical encodings are distinct values
    assert len(set(seen.values())) == len(seen)


def test_round_trip_64():
    rng = random.Random(1)
    for _ in range(20000):
        a, b = rng.randrange(-2**30, 2**30), rng.randrange(-2**20, 2**20)
        c = rng.randrange(1, 2**12)
        enc = P.plane_encode(a, b, c, 64)
        g = math.gcd(math.gcd(abs(a), abs(b)), c)
        if enc is None:
            assert not P.plane_fits(a, b, c, 64)
            continue
        assert P.plane_decode(*enc, 64) == (a // g, b // g, c // g)


def ref(vals):
    return [(Fraction(r), Fraction(i)) for r, i in vals]


def test_plane_vectors_against_fractions():
    rng = np.random.default_rng(5)
    for n, bound, den in ((50, 10**6, 1000), (40, 2**40, 7), (30, 2**62, 1), (60, 10**3, 10**6)):
        ar, ai, br, bi = (rng.integers(-bound, bound, size=n) for _ in range(4))
        X, Y = P.PVec.decimals(ar, ai, den), P.PVec.decimals(br, bi, den * 3)
        x = [(Fraction(int(r), den), Fraction(int(i), den)) for r, i in zip(ar, ai)]
        y = [(Fraction(int(r), den * 3), Fraction(int(i), den * 3)) for r, i in zip(br, bi)]
        assert X.values() == x and Y.values() == y
        assert (X + Y).values() == [(p[0] + q[0], p[1] + q[1]) for p, q in zip(x, y)]
        assert (X - Y).values() == [(p[0] - q[0], p[1] - q[1]) for p, q in zip(x, y)]
        prod = [(p[0] * q[0] - p[1] * q[1], p[0] * q[1] + p[1] * q[0]) for p, q in zip(x, y)]
        assert (X * Y).values() == prod
        assert ((X * Y) * X).values() == [(p[0] * q[0] - p[1] * q[1], p[0] * q[1] + p[1] * q[0]) for p, q in zip(prod, x)]
        assert X.abs2().total() == sum(p[0] ** 2 + p[1] ** 2 for p in x)
        assert X.total2() == (sum(p[0] for p in x), sum(p[1] for p in x))
        g = math.gcd(X.den, *[int(v) for v in X.re.big()], *[int(v) for v in X.im.big()])
        assert g == 1, "plane vectors are reduced"


def test_axis_pair_matches_plane():
    rng = np.random.default_rng(6)
    for n, bound, den in ((40, 10**6, 1000), (30, 2**40, 7)):
        ar, ai, br, bi = (rng.integers(-bound, bound, size=n) for _ in range(4))
        X, Y = P.PVec.decimals(ar, ai, den), P.PVec.decimals(br, bi, den * 3)
        A, B = P.APair.decimals(ar, ai, den), P.APair.decimals(br, bi, den * 3)
        assert (A * B).values() == (X * Y).values()
        assert (A * B + A).values() == (X * Y + X).values()
        assert A.abs2().total() == X.abs2().total() and A.total2() == X.total2()
