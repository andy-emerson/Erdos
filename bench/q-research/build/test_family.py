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


def test_un_other_denominators_and_start_widths():
    """Chains with small, odd and 40-bit denominators, from every start width, against Fractions."""
    U.build()
    rng = np.random.default_rng(21)
    n = 120
    for den in (7, 1000, 2**40 + 15):
        a = rng.integers(-10**9, 10**9, size=n); b = rng.integers(-10**4, 10**4, size=n)
        c = rng.integers(-den, den, size=n)
        a[0] = -(2**63); b[1] = 0; c[2] = 0
        I = U.Inputs(a, b, c, den)
        Fr = lambda v: Fraction(int(v), den)
        x = [Fr(v) for v in a]
        for K in range(1, 41):
            x = [xi * Fr(bi) + Fr(ci) for xi, bi, ci in zip(x, b, c)]
            if K not in (1, 6, 15, 40):
                continue
            for m in ("un", "fl", "un@128", "fl@256", "un@1024", "fl@2048", "flc"):
                got = I.run(m, "chain", K, 1)[3]
                assert got == x or got is None, (den, K, m)
                if got is None:                                      # only past the cap
                    assert max(abs(v.numerator) for v in x).bit_length() + 1 > 2048 - 64, (den, K, m)


def test_un_and_quaternion_harnesses():
    U.build()
    rng = np.random.default_rng(9)
    n, den = 300, 1000
    a = rng.integers(-10**7, 10**7, size=n); b = rng.integers(-10**4, 10**4, size=n); c = rng.integers(-10**6, 10**6, size=n)
    a[:3] = [10**16, -(10**16), 2**62]
    I = U.Inputs(a, b, c, den)
    Fr = lambda v: Fraction(int(v), den)
    sq = [(Fr(x) * Fr(y)) ** 2 for x, y in zip(a, b)]
    for m in U.NAMES:
        if m == "flc":                                       # the pre-built U2048 runs chains only
            continue
        assert I.run(m, "sq", 0, 1)[3] == sq and I.run(m, "sumsq", 0, 1)[3] == [sum(sq)], m
    x = [Fr(v) for v in a]
    for _ in range(20):
        x = [xi * Fr(bi) + Fr(ci) for xi, bi, ci in zip(x, b, c)]
    for m in U.NAMES:
        assert I.run(m, "chain", 20, 1)[3] == x, m
    for K in (70, 140):                                      # through 1024 and 2048 bits
        x = [Fr(v) for v in a]
        for _ in range(K):
            x = [xi * Fr(bi) + Fr(ci) for xi, bi, ci in zip(x, b, c)]
        for m in U.FIXED:
            assert I.run(m, "chain", K, 1)[3] == x, (m, K)
    # past the 2048-bit cap: every fixed-width method reports it rather than wrapping
    for m in U.FIXED:
        assert I.run(m, "chain", 200, 1)[3] is None, m
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


# ---------------------------------------------------------------- the uniform header
def test_uniform_round_trips_every_algebra_and_word():
    import random
    import uniform as UN
    rng = random.Random(5)
    for dim in (1, 2, 4, 8):
        for word in UN.WORDS:
            _, _, _, D = UN.layout(word)
            for _ in range(300):
                s = rng.randrange(1, D)
                p = rng.getrandbits(s) | (1 << (s - 1))
                q = rng.randrange(1, 1 << min(D - s, 60)) if D - s > 0 else 1
                from math import gcd
                g = gcd(p, q); p, q = p // g, q // g
                if not UN.FA.fits(p, q, 8, word):
                    continue
                unit = (rng.choice((1, -1)), rng.randrange(dim))
                h, v = UN.encode(unit, p, q, word, dim)
                assert h < 256 and v < (1 << word)
                assert UN.decode(h, v, word) == (unit, p, q)
                assert UN.valid_for(unit, dim)


def test_uniform_one_decoder_and_nesting():
    """A value of a smaller algebra has the same bits as that value in every larger algebra: no re-encoding."""
    import uniform as UN
    for word in (16, 64, 256):
        for unit in ((1, 0), (-1, 0), (1, 1), (-1, 3)):
            enc = [UN.encode(unit, 5, 7, word, dim) for dim in (1, 2, 4, 8) if unit[1] < dim]
            assert len(set(enc)) == 1


def test_uniform_exhaustive_header_at_u16():
    """Every header byte with a fixed set of words decodes; axes past an algebra's dimension are flagged."""
    import uniform as UN
    for header in range(256):
        for value in (0, 1, 0x7FFF, 0x8000, 0xFFFF, 0x1234):
            unit, p, q = UN.decode(header, value, 16)
            if p:
                assert UN.decode(*UN.encode(unit, p, q, 16), 16) == (unit, p, q)
                assert UN.valid_for(unit, 8)
                assert UN.valid_for(unit, 4) == (unit[1] < 4)


def test_octonion_harness():
    """Octonion arrays against Fractions, including components whose products need 128 bits, and the refusal past
    128 bits; the reference table also satisfies |pq| = |p||q|."""
    import oct_run as Or
    Or.build()
    rng = np.random.default_rng(12)
    for den, bound, n in ((1000, 10**3, 200), (10**6, 10**6, 100)):
        mk = lambda: [rng.integers(-bound * den, bound * den + 1, size=n) for _ in range(8)]
        p, q = mk(), mk()
        p[3][0] = 2**62                                          # forces the 128-bit path
        want = Or.reference(p, q, den)
        nr = lambda x: sum(c * c for c in x)
        P = [tuple(Fraction(int(c[i]), den) for c in p) for i in range(n)]
        Qs = [tuple(Fraction(int(c[i]), den) for c in q) for i in range(n)]
        assert all(nr(m) == nr(x) * nr(y) for m, x, y in zip(want["mul"], P, Qs))
        J = Or.Inputs(p, q, den)
        for m in Or.NAMES:
            for op in ("add", "mul", "norm"):
                got = J.run(m, op, 1)[2]
                assert got == want[op] or (m == "i64c" and got is None) or (m == "vec" and got == "exceeds"
                                                                                and op == "norm"), (m, op)
    big = [np.full(4, 2**62 + 1, dtype=np.int64) for _ in range(8)]
    assert Or.Inputs(big, big, 1).run("vec", "mul", 1)[2] == "exceeds"


def test_gemm_exact():
    """Exact products against Python integers on small sizes, and against FLINT on extremes (+-(2^b - 1), zeros)."""
    import subprocess
    import tempfile
    import gemm_run as G
    G.build()
    with tempfile.TemporaryDirectory() as d:
        out = d + "/c.txt"
        for m in G.NAMES:
            for n, bits in ((1, 1), (5, 300), (17, 2048), (33, 64)):
                b, mm = G.binary_for(m)
                subprocess.run([str(b), mm, str(n), str(bits), "1", out], check=True, capture_output=True)
                vals = [int(x) for x in open(out + ".in").read().split()]
                A = [vals[i * n:(i + 1) * n] for i in range(n)]
                B = [vals[n * n + i * n:n * n + (i + 1) * n] for i in range(n)]
                C = [sum(A[i][k] * B[k][j] for k in range(n)) for i in range(n) for j in range(n)]
                got = open(out).read().split()
                assert len(got) == n * n
                if m == "fmpq":
                    assert all(Fraction(g) == Fraction(c, 10**6) for g, c in zip(got, C))
                else:
                    assert all(int(g) == c for g, c in zip(got, C)), (m, n, bits)
    for m in G.NAMES:                                          # the check catches a wrong entry, small and large n
        for n in (20, 200):
            b, mm = G.binary_for(m)
            r = subprocess.run([str(b), mm, str(n), "100", "1"], capture_output=True,
                               env=dict(__import__("os").environ, GEMM_CORRUPT="1"))
            assert r.returncode == 4, (m, n)
    for edge in (1, 2):
        for n, bits in ((1, 64), (7, 65), (64, 2048), (300, 63)):
            for m in ("mm", "mmg", "blas"):
                G.run(m, n, bits, reps=1, edge=edge)               # raises if the result differs from FLINT


def test_c_decoder_matches_family_decode():
    """The decoder timed in section F (cbench/decode.c) decodes exactly as family.decode, both layouts."""
    import subprocess
    import gemm_run as G
    subprocess.run(["gcc", "-O3", "-march=native", "-o", str(G.CB / "decode"), str(G.CB / "decode.c")], check=True)
    out = subprocess.run([str(G.CB / "decode"), "check", "20000"], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        h, v, s1, a1, p1, q1, s2, a2, p2, q2 = map(int, line.split())
        for dim, s, a, p, q in ((2, s1, a1, p1, q1), (8, s2, a2, p2, q2)):
            unit, P, Q = F.decode(h, v, dim, 64)
            assert (p == 0) if P == 0 else ((unit, P, Q) == ((s, a), p, q)), line


def test_uniform_dimension_wide_denominators_and_promotion():
    """encode refuses an axis outside the algebra; denominators far past 64 bits round-trip; a value that does not
    fit U16 fits U32 after promotion and decodes to the same number, for every algebra."""
    import random
    from math import gcd
    import uniform as UN
    for dim in (1, 2, 4):
        try:
            UN.encode((1, dim), 3, 5, 16, dim)
        except AssertionError:
            pass
        else:
            raise AssertionError("axis outside the algebra was accepted")
    rng = random.Random(8)
    for word in (256, 1024, 2048):
        _, _, _, D = UN.layout(word)
        for _ in range(100):
            sp = rng.randrange(1, D // 2)
            p = rng.getrandbits(sp) | 1
            q = rng.getrandbits(D - sp - 1) | (1 << (D - sp - 2)) | 1          # denominators of hundreds of bits
            g = gcd(p, q); p, q = p // g, q // g
            if not UN.FA.fits(p, q, 8, word):
                continue
            unit = (rng.choice((1, -1)), rng.randrange(8))
            assert UN.decode(*UN.encode(unit, p, q, word), word) == (unit, p, q)
    for dim in (1, 2, 4, 8):
        for p, q in ((70001, 3), (3, 70001), (2**19 + 1, 2**10 + 1)):
            unit = (-1, dim - 1)
            assert UN.FA.encode(unit, p, q, 8, 16) is None                 # too big for U16
            h, v = UN.encode(unit, p, q, 32, dim)
            assert UN.decode(h, v, 32) == (unit, p, q)


def test_octonion_128_bit_paths():
    """Components near 2^40: products and norms take the 128-bit path without refusing."""
    import oct_run as Or
    rng = np.random.default_rng(13)
    n, den = 80, 10**9
    mk = lambda: [rng.integers(-2**40, 2**40, size=n) for _ in range(8)]
    p, q = mk(), mk()
    want = Or.reference(p, q, den)
    J = Or.Inputs(p, q, den)
    for op in ("mul", "norm"):
        got = J.run("vec", op, 1)[2]
        assert got == want[op], op
