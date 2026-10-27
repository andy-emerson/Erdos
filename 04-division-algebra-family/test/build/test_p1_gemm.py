"""Tests for cbench/p1_gemm.c (run from build/: python3 ../../../shared/runtests.py test_p1_gemm)."""
import tempfile
from pathlib import Path

import p1_gemm_run as P

_built = []


def _build():
    if not _built:
        P.build()
        _built.append(1)


def _read(path):
    return [int(x) for x in Path(path).read_text().split()]


def test_exact_against_python_integers():
    _build()
    with tempfile.TemporaryDirectory() as d:
        out = Path(d) / "c.txt"
        for method in P.METHODS:
            for n in (1, 5, 17, 33):
                for bits in (1, 64, 300, 2048):
                    for edge in (None, 1):
                        P.run(method, n, bits, reps=1, edge=edge, outfile=out)
                        c = _read(out)
                        ab = _read(str(out) + ".in")
                        a, b = ab[:n * n], ab[n * n:]
                        assert len(c) == n * n and len(b) == n * n
                        assert all(abs(x) < 2**bits for x in ab)
                        bt = [b[j::n] for j in range(n)]           # columns of B
                        for i in range(n):
                            row = a[i * n:(i + 1) * n]
                            for j in range(n):
                                assert c[i * n + j] == sum(x * y for x, y in zip(row, bt[j])), (method, n, bits, edge, i, j)


def test_extreme_inputs():
    _build()
    for method in P.METHODS:
        for edge in (1, 2):
            for n in (1, 7, 64, 300):
                for bits in (1, 63, 64, 65, 2048):
                    r = P.run(method, n, bits, reps=1, edge=edge)       # raises on a failed check
                    assert r["primes"] > 0


def test_corruption_is_detected():
    _build()
    for method in P.METHODS:
        for n in (20, 200):
            P.run(method, n, 64, reps=1)
            try:
                P.run(method, n, 64, reps=1, corrupt=True)
            except AssertionError:
                continue
            raise AssertionError(f"{method} n={n}: a corrupted product passed the check")
