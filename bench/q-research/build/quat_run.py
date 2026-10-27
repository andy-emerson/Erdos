"""Runs cbench/quat.c (exact quaternion arrays) and parses its results."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "quat"
SRC = HERE.parent / "cbench" / "quat.c"
NAMES = {"vec": "quaternion vector form (four int64 vectors, compiled)", "mpq": "GMP mpq_t 4-tuples (C)",
         "fmpq": "FLINT fmpq 4-tuples (C)", "fmpz": "FLINT fmpz over one denominator per value (C)",
         "i64c": "C int64 4-tuples, overflow-checked"}

# e_x e_y = SIGN[x][y] e_AXIS[x][y], axes 1, i, j, k
AXIS = ((0, 1, 2, 3), (1, 0, 3, 2), (2, 3, 0, 1), (3, 2, 1, 0))
SIGN = ((1, 1, 1, 1), (1, -1, 1, -1), (1, -1, -1, 1), (1, 1, -1, -1))


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(BIN), str(SRC), "-lflint", "-lgmp"], check=True)


def hamilton(x, y):
    """Reference Hamilton product on 4-tuples of Fractions, written out independently of the table."""
    a1, b1, c1, d1 = x
    a2, b2, c2, d2 = y
    return (a1 * a2 - b1 * b2 - c1 * c2 - d1 * d2, a1 * b2 + b1 * a2 + c1 * d2 - d1 * c2,
            a1 * c2 - b1 * d2 + c1 * a2 + d1 * b2, a1 * d2 + b1 * c2 - c1 * b2 + d1 * a2)


def conj(x):
    return (x[0], -x[1], -x[2], -x[3])


class Inputs:
    def __init__(self, p, q, v, den):
        self.dir = tempfile.TemporaryDirectory()
        for prefix, comps in (("p", p), ("q", q), ("v", v)):
            for name, x in zip("abcd", comps):
                np.ascontiguousarray(x, dtype=np.int64).tofile(Path(self.dir.name) / f"{prefix}{name}.bin")
        self.n, self.den = len(p[0]), int(den)

    def run(self, method, op, reps=5):
        out = Path(self.dir.name) / f"{method}-{op}.txt"
        r = subprocess.run([str(BIN), method, op, str(self.n), str(self.den), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True)
        if r.returncode == 3:                    # the vector form refused: the result needs more than 128 bits
            return None, None, "exceeds"
        r.check_returncode()
        med, mn = map(float, r.stdout.split())
        text = out.read_text().split()
        if text == ["overflow"]:
            return med, mn, None
        if op == "norm":
            return med, mn, [Fraction(text[0])]
        return med, mn, [tuple(Fraction(s) for s in text[i:i + 4]) for i in range(0, len(text), 4)]


def reference(p, q, v, den):
    F = lambda comps: [tuple(Fraction(int(c[i]), den) for c in comps) for i in range(len(comps[0]))]
    P, Qs, Vs = F(p), F(q), F(v)
    return {"add": [tuple(a + b for a, b in zip(x, y)) for x, y in zip(P, Qs)],
            "mul": [hamilton(x, y) for x, y in zip(P, Qs)],
            "norm": [sum(sum(c * c for c in x) for x in P)],
            "rot": [hamilton(hamilton(x, w), conj(x)) for x, w in zip(P, Vs)]}
