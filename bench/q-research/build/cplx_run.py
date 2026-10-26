"""Runs cbench/cplx.c and parses its results."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

BIN = Path(__file__).resolve().parent.parent / "cbench" / "cplx"
NAMES = {"mpq": "GMP mpq_t pairs (C)", "fmpq": "FLINT fmpq pairs (C)",
         "fmpzi": "FLINT fmpzi over one denominator (C)", "i64c": "C int64 pairs, overflow-checked"}


class Inputs:
    def __init__(self, xr, xi, yr, yi, wr, wi, den):
        self.dir = tempfile.TemporaryDirectory()
        for name, v in zip(("xr", "xi", "yr", "yi", "wr", "wi"), (xr, xi, yr, yi, wr, wi)):
            np.ascontiguousarray(v, dtype=np.int64).tofile(Path(self.dir.name) / f"{name}.bin")
        self.n, self.den = len(xr), int(den)

    def run(self, who, op, reps=5):
        out = Path(self.dir.name) / f"{who}-{op}.txt"
        r = subprocess.run([str(BIN), who, op, str(self.n), str(self.den), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True, check=True)
        med, mn = map(float, r.stdout.split())
        text = out.read_text().split()
        if text == ["overflow"]:
            return med, mn, None
        return med, mn, [(Fraction(a), Fraction(b)) for a, b in zip(text[::2], text[1::2])]
