"""Runs cbench/p1_widen.c (phase one, the widening rule) and checks results against Python integers."""
import subprocess
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "p1_widen"
SRC = HERE.parent / "cbench" / "p1_widen.c"
RULES = {"whole": "whole-vector widening", "elem": "per-element promotion"}
SHARES = (0.0, 0.001, 0.01, 0.1, 1.0)


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        "-o", str(BIN), str(SRC), "-lgmp"], check=True)


class Inputs:
    """3-place decimals below 10^4 (a) and below 10 (b), with a share of a near 10^16, whose squares need 256 bits."""

    def __init__(self, n, share, seed=0):
        rng = np.random.default_rng(seed)
        self.a = rng.integers(-10**7, 10**7 + 1, size=n)
        self.b = rng.integers(-10**4, 10**4 + 1, size=n)
        k = int(round(share * n))
        if k:
            i = rng.choice(n, size=k, replace=False)
            self.a[i] = rng.integers(10**15, 10**16, size=k) * np.where(rng.random(k) < 0.5, -1, 1)
        self.n, self.k = n, k
        self.dir = tempfile.TemporaryDirectory()
        self.a.astype(np.int64).tofile(Path(self.dir.name) / "a.bin")
        self.b.astype(np.int64).tofile(Path(self.dir.name) / "b.bin")

    def expected(self, op):
        sq = [(int(x) * int(y)) ** 2 for x, y in zip(self.a, self.b)]
        return [sum(sq)] if op == "sumsq" else sq

    def run(self, rule, op, reps=7):
        out = Path(self.dir.name) / f"{rule}-{op}.txt"
        r = subprocess.run([str(BIN), rule, op, str(self.n), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True, check=True)
        med, mn, wide = r.stdout.split()
        return float(med), float(mn), int(wide), [int(x) for x in out.read_text().split()]
