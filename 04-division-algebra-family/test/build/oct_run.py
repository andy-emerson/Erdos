"""Runs cbench/oct.c (exact octonion arrays) and parses its results. The reference product is computed from
family.basis_mul on Fractions, independently of the generated C table."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

import family as FA
import gen_oct_table

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "oct"
SRC = HERE.parent / "cbench" / "oct.c"
NAMES = {"vec": "octonion vector form (eight int64 vectors, compiled)", "mpq": "GMP mpq_t 8-tuples (C)",
         "fmpz": "FLINT fmpz over one denominator per value (C)", "i64c": "C int64 8-tuples, overflow-checked"}


def build():
    gen_oct_table.main()
    subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(BIN), str(SRC), "-lflint", "-lgmp"], check=True)


def omul(x, y):
    r = [Fraction(0)] * 8
    for a in range(8):
        for b in range(8):
            s, z = FA.basis_mul(a, b, 8)
            r[z] += s * x[a] * y[b]
    return tuple(r)


class Inputs:
    def __init__(self, p, q, den):
        self.dir = tempfile.TemporaryDirectory()
        for prefix, comps in (("p", p), ("q", q)):
            for c, x in enumerate(comps):
                np.ascontiguousarray(x, dtype=np.int64).tofile(Path(self.dir.name) / f"{prefix}{c}.bin")
        self.n, self.den = len(p[0]), int(den)

    def run(self, method, op, reps=5):
        out = Path(self.dir.name) / f"{method}-{op}.txt"
        r = subprocess.run([str(BIN), method, op, str(self.n), str(self.den), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True)
        if r.returncode == 3:
            return None, None, "exceeds"
        r.check_returncode()
        med, mn = map(float, r.stdout.split())
        text = out.read_text().split()
        if text == ["overflow"]:
            return med, mn, None
        if op == "norm":
            return med, mn, [Fraction(text[0])]
        return med, mn, [tuple(Fraction(s) for s in text[i:i + 8]) for i in range(0, len(text), 8)]


def reference(p, q, den):
    F = lambda comps: [tuple(Fraction(int(c[i]), den) for c in comps) for i in range(len(comps[0]))]
    P, Qs = F(p), F(q)
    return {"add": [tuple(a + b for a, b in zip(x, y)) for x, y in zip(P, Qs)],
            "mul": [omul(x, y) for x, y in zip(P, Qs)],
            "norm": [sum(sum(c * c for c in x) for x in P)]}
