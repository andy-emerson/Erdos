"""Runs cbench/un.c (fixed-width uN tiers against GMP and FLINT) and parses its results."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "un"
SRC = HERE.parent / "cbench" / "un.c"
NAMES = {"un": "uN vector (fixed widths 64-1024, compiled)", "mpz": "GMP mpz_t numerators (C)",
         "fmpz": "FLINT fmpz numerators (C)", "mpq": "GMP mpq_t (C)"}


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        "-o", str(BIN), str(SRC), "-lflint", "-lgmp"], check=True)


class Inputs:
    def __init__(self, a, b, c, den):
        self.dir = tempfile.TemporaryDirectory()
        for name, x in zip("abc", (a, b, c)):
            np.ascontiguousarray(x, dtype=np.int64).tofile(Path(self.dir.name) / f"{name}.bin")
        self.n, self.den = len(a), int(den)

    def run(self, method, op, K=0, reps=5):
        out = Path(self.dir.name) / f"{method}-{op}-{K}.txt"
        r = subprocess.run([str(BIN), method, op, str(self.n), str(self.den), str(K), str(reps), self.dir.name,
                            str(out)], capture_output=True, text=True, check=True)
        med, mn, width = r.stdout.split()
        text = out.read_text().split()
        return float(med), float(mn), int(width), (None if text == ["exceeds"] else [Fraction(s) for s in text])
