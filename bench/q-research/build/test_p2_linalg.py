"""Tests for cbench/p2_linalg.c (run: python3 runtests.py test_p2_linalg).

Every op and contender at small sizes and every width, with random, edge (0, +-1, +-max) and all-extreme entries,
is checked against Python integers and Fractions computed from the inputs the binary writes; a corrupted result
must fail the binary's own check."""
import tempfile
from fractions import Fraction
from pathlib import Path

import p2_linalg_run as P

_built = []


def _build():
    if not _built:
        P.build()
        _built.append(1)


def _expected(op, n, dA, dB, A, B):
    """the exact result as Fractions (mv, mm, spmv); None for solve (checked by substitution)"""
    if op == "mv":
        return [Fraction(sum(A[i * n + j] * B[j] for j in range(n)), dA * dB) for i in range(n)]
    if op == "spmv":
        rp, cols, vals = A
        return [Fraction(sum(vals[t] * B[cols[t]] for t in range(rp[i], rp[i + 1])), dA * dB) for i in range(n)]
    if op == "mm":
        return [Fraction(sum(A[i * n + k] * B[k * n + j] for k in range(n)), dA * dB)
                for i in range(n) for j in range(n)]
    return None


def _check(path, op, contender, n, W, edge):
    op2, n2, W2, k, dA, dB, A, B, res = P.read_out(path)
    tag = (op, contender, n, W, edge)
    assert (op2, n2, W2) == (op, n, W), tag
    assert all(d > 0 for _, d in res), tag
    got = [Fraction(a, d) for a, d in res]
    vals = A[2] if op == "spmv" else A
    # entry sizes as documented: exactly ENT bits (random), or the edge values
    log2 = lambda x: (x - 1).bit_length()
    ent = W // 8 if op == "solve" else (W - log2(k if op == "spmv" else n)) // 2 - 1
    assert max(abs(x) for x in vals + B) <= 2**ent - 1, tag
    if edge == 2:
        assert all(abs(x) == 2**ent - 1 for x in B) and dA == dB == 1, tag
    if op == "solve":
        assert len(got) == n, tag
        for i in range(n):
            assert sum(Fraction(A[i * n + j], dA) * got[j] for j in range(n)) == Fraction(B[i], dB), (tag, i)
        if contender in ("ours", "gmp_mpz", "flint_fmpz"):      # one shared denominator, in lowest terms
            from math import gcd
            assert len({d for _, d in res}) == 1, tag
            g = res[0][1]
            for a, _ in res:
                g = gcd(g, a)
            assert g == 1, tag
    else:
        exp = _expected(op, n, dA, dB, A, B)
        assert got == exp, tag
        if op != "solve" and edge != 2:          # the results really need about W bits
            assert max(abs(x.numerator * (dA * dB) // x.denominator) for x in exp).bit_length() <= W - 2, tag


def _sizes(op):
    if op == "spmv":
        return ((1, 1), (40, 5), (64, 64))
    if op == "solve":
        return ((1, None), (3, None), (9, None), (17, None))      # 17: FLINT's Dixon paths (n > 15)
    return ((1, None), (5, None), (17, None))


def test_exact_against_python():
    _build()
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "r.txt"
        for op in P.OPS:
            for contender in P.CONTENDERS:
                for n, k in _sizes(op):
                    for W in P.WIDTHS:
                        for edge in (None, 1, 2):
                            if contender == "gmp_mpq" and op == "solve" and n > 9 and W > 512:
                                continue                     # slow in Python's Fractions, not in C
                            P.run(op, contender, n, W, reps=1, edge=edge, k=k, outfile=out)
                            _check(out, op, contender, n, W, edge)


def test_variants():
    """ours mv/spmv forced to run-time limbs; ours solve with a lane dropped per prime batch (a varying prime set);
    ours mm at a size where the IFMA kernel's blocking is exercised"""
    _build()
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "r.txt"
        for W in (128, 2048):
            for op, k in (("mv", None), ("spmv", 5)):
                P.run(op, "ours", 40, W, reps=1, rt=True, k=k, edge=1, outfile=out)
                _check(out, op, "ours", 40, W, 1)
            for edge in (None, 1, 2):
                P.run("solve", "ours", 12, W, reps=1, edge=edge, drop=True, outfile=out)
                _check(out, "solve", "ours", 12, W, edge)
        for W in (128, 1024):
            P.run("mm", "ours", 40, W, reps=1, outfile=out)
            _check(out, "mm", "ours", 40, W, None)
    # larger sizes, checked by the binary itself (FLINT classical reference, Freivalds, A x == b)
    for op, n in (("mv", 200), ("mm", 140), ("spmv", 2000), ("solve", 40)):
        for W in (128, 2048):
            for contender in ("ours", "gmp_mpz", "flint_fmpz", "flint_fmpq"):
                P.run(op, contender, n, W, reps=1)


def test_corruption_is_detected():
    _build()
    for op, n in (("mv", 20), ("mm", 20), ("mm", 140), ("spmv", 300), ("solve", 10)):
        for contender in P.CONTENDERS:
            P.run(op, contender, n, 256, reps=1)
            try:
                P.run(op, contender, n, 256, reps=1, corrupt=True)
            except AssertionError:
                continue
            raise AssertionError(f"{op} {contender} n={n}: a corrupted result passed the check")


def test_flint_methods():
    """every FLINT routine the bench may choose (P.METHODS), exact against Python, at sizes on both sides of FLINT's
    cutoffs (n = 17 > 15 for the solve dispatch), random and edge entries"""
    _build()
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "r.txt"
        for (contender, op), methods in P.METHODS.items():
            for m in methods:
                for n, k in ((3, None), (17, None)) if op != "spmv" else ((40, 5),):
                    for W in (128, 2048):
                        for edge in (None, 1):
                            r = P.run(op, contender, n, W, reps=1, edge=edge, k=k, method=m, outfile=out)
                            assert r["method"] == m, (contender, op, m, r)
                            _check(out, op, contender, n, W, edge)
    try:
        P.run("mm", "flint_fmpz", 3, 128, reps=1, method="nonsense")
    except RuntimeError:
        pass
    else:
        raise AssertionError("an unknown method was accepted")


def test_timed_setup_repeats():
    """ours mm and solve now rebuild their primes and CRT constants inside every run: several reps must agree and
    verify (the last run is the one checked), including with dropped lanes and edge inputs"""
    _build()
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "r.txt"
        for W in (128, 1024, 2048):
            r = P.run("mm", "ours", 20, W, reps=3, outfile=out)
            assert r["primes"] > 0
            _check(out, "mm", "ours", 20, W, None)
            for drop in (False, True):
                r = P.run("solve", "ours", 10, W, reps=3, edge=1, drop=drop, outfile=out)
                assert r["primes"] > 0
                _check(out, "solve", "ours", 10, W, 1)
