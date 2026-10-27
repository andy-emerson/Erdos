import math
import random
from fractions import Fraction

import numpy as np

import reuse  # noqa: F401
import family as F
import quat_run as Qr
import un_run as U


def test_tables_nest_and_close():
    for dim in (1, 2, 4, 8):
        us = F.units(dim)
        for u in us:
            for v in us:
                assert F.unit_mul(u, v, dim) in us, "closed"
        for small in (1, 2, 4):
            if small < dim:
                for x in range(small):
                    for y in range(small):
                        assert F.basis_mul(x, y, dim) == F.basis_mul(x, y, small), "each table is inside the next"
    # complex: i*i = -1; quaternions: the Hamilton table
    assert F.basis_mul(1, 1, 2) == (-1, 0)
    for x in range(4):
        for y in range(4):
            assert F.basis_mul(x, y, 4) == (Qr.SIGN[x][y], Qr.AXIS[x][y]), (x, y)


def test_associativity_and_inverses():
    for dim in (1, 2, 4, 8):
        us = F.units(dim)
        bad = sum(F.unit_mul(F.unit_mul(a, b, dim), c, dim) != F.unit_mul(a, F.unit_mul(b, c, dim), dim)
                  for a in us for b in us for c in us)
        assert (bad == 0) == (dim <= 4), (dim, bad)
        # alternative: x(xy) = (xx)y even for octonions, and every unit has an inverse
        for a in us:
            for b in us:
                assert F.unit_mul(a, F.unit_mul(a, b, dim), dim) == F.unit_mul(F.unit_mul(a, a, dim), b, dim)
            assert any(F.unit_mul(a, b, dim) == (1, 0) for b in us)
        if dim == 4:
            assert F.basis_mul(1, 2, 4) == (1, 3) and F.basis_mul(2, 1, 4) == (-1, 3), "ij = k, ji = -k"


def test_layouts():
    assert [F.layout(d, w) for d, w in F.ALGEBRAS.values()] == [(1, 7, 0, 128), (2, 6, 0, 64), (3, 5, 0, 32), (4, 4, 0, 16)]
    assert F.layout(4, 64) == (3, 5, 1, 63) and F.layout(4, 128) == (3, 5, 2, 126) and F.layout(4, 256) == (3, 5, 3, 253)
    assert F.layout(2, 128) == (2, 6, 1, 127) and F.layout(8, 32) == (4, 4, 1, 31)


def test_every_pattern_decodes_small_words():
    """With a 4-bit header, check every pattern of small words: every pattern decodes, and canonical
    encodings round-trip and are unique, including promoted words with borrowed slash bits."""
    for dim, word in ((1, 8), (1, 16), (2, 4), (2, 8), (2, 16), (4, 2), (4, 4), (4, 8)):
        U_, h, b, D = F.layout(dim, word, header_bits=4)
        seen = {}
        for header in range(16):
            for value in range(1 << word):
                unit, p, q = F.decode(header, value, dim, word, header_bits=4)
                g = math.gcd(p, q)
                enc = F.encode(unit, p // g, q // g, dim, word, header_bits=4)
                assert enc is not None
                assert F.decode(*enc, dim, word, header_bits=4) == (unit if p else (1, 0), p // g, q // g)
                v = F.value_of(unit, p // g, q // g) if p else (Fraction(0), 0)
                assert seen.setdefault(enc, v) == v
        assert len(set(seen.values())) == len(seen)


def test_round_trip_full_widths():
    rng = random.Random(4)
    for dim, base in F.ALGEBRAS.values():
        for word in (base, 2 * base, 4 * base):
            _, _, _, D = F.layout(dim, word)
            for _ in range(2000):
                bp = rng.randint(1, D)
                p = rng.getrandbits(bp) | (1 << (bp - 1))
                q = rng.randint(1, 1 << max(0, D - bp))
                g = math.gcd(p, q)
                p, q = p // g, q // g
                unit = (rng.choice((1, -1)), rng.randrange(dim))
                enc = F.encode(unit, p, q, dim, word)
                if enc is None:
                    assert not F.fits(p, q, dim, word)
                    continue
                assert enc[0] < 256 and enc[1] < (1 << word)
                assert F.decode(*enc, dim, word) == (unit, p, q)


def test_un_and_quaternion_harnesses():
    rng = np.random.default_rng(9)
    n, den = 300, 1000
    a = rng.integers(-10**7, 10**7, size=n); b = rng.integers(-10**4, 10**4, size=n); c = rng.integers(-10**6, 10**6, size=n)
    a[:3] = [10**16, -(10**16), 2**62]
    I = U.Inputs(a, b, c, den)
    Fr = lambda v: Fraction(int(v), den)
    sq = [(Fr(x) * Fr(y)) ** 2 for x, y in zip(a, b)]
    for m in U.NAMES:
        assert I.run(m, "sq", 0, 1)[3] == sq and I.run(m, "sumsq", 0, 1)[3] == [sum(sq)]
    x = [Fr(v) for v in a]
    for _ in range(20):
        x = [xi * Fr(bi) + Fr(ci) for xi, bi, ci in zip(x, b, c)]
    for m in U.NAMES:
        assert I.run(m, "chain", 20, 1)[3] == x, m
    # past the cap: the uN method reports it rather than wrapping
    x = [Fr(v) for v in a]
    assert I.run("un", "chain", 90, 1)[3] is None
    mk = lambda: [rng.integers(-10**6, 10**6, size=n) for _ in range(4)]
    p, q, v = mk(), mk(), mk()
    v[0][:] = 0
    J = Qr.Inputs(p, q, v, den)
    want = Qr.reference(p, q, v, den)
    for m in Qr.NAMES:
        for op in ("add", "mul", "norm", "rot"):
            got = J.run(m, op, 1)[2]
            assert got == want[op] or (m == "i64c" and got is None and op == "rot"), (m, op)


def _mul(x, y, dim):
    """General elements (lists of integer coordinates) multiplied through the basis table."""
    out = [0] * dim
    for i, a in enumerate(x):
        if a:
            for j, b in enumerate(y):
                if b:
                    s, z = F.basis_mul(i, j, dim)
                    out[z] += s * a * b
    return out


def test_alternativity_on_general_elements():
    """Octonions are alternative, x(xy) = (xx)y, for all elements; sedenions (dimension 16) are not."""
    rng = random.Random(12)
    for dim, expect in ((4, True), (8, True), (16, False)):
        ok = True
        for _ in range(60):
            x = [rng.randint(-5, 5) for _ in range(dim)]
            y = [rng.randint(-5, 5) for _ in range(dim)]
            ok &= _mul(x, _mul(x, y, dim), dim) == _mul(_mul(x, x, dim), y, dim)
        assert ok == expect, dim


def test_quaternion_vector_refuses_instead_of_wrapping():
    n, den = 8, 1
    M = 2**63 - 1
    p = [np.full(n, M), np.full(n, M), np.full(n, M), np.full(n, -M)]
    q = [np.full(n, M)] * 4
    v = [np.zeros(n, np.int64)] + [np.full(n, 3)] * 3
    J = Qr.Inputs(p, q, v, den)
    assert J.run("vec", "mul", 1)[2] == "exceeds"
    big = [np.full(n, 2**62)] * 4
    assert Qr.Inputs(big, big, v, den).run("vec", "norm", 1)[2] == "exceeds"
    near = [np.full(n, 2**63 - 1)] * 4
    one = [np.ones(n, np.int64)] * 4
    K = Qr.Inputs(near, one, v, den)
    assert K.run("vec", "add", 1)[2] == Qr.reference(near, one, v, den)["add"], "a wide sum goes to 128 bits"
