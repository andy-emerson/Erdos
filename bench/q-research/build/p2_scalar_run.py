"""Runs cbench/p2_scalar.c (phase two, single values) and checks results against Python Fractions."""
import random
import struct
import subprocess
import tempfile
import zlib
from fractions import Fraction
from math import gcd
from pathlib import Path

HERE = Path(__file__).resolve().parent
FORCE = False
BIN = HERE.parent / "cbench" / "p2_scalar"
SRC = HERE.parent / "cbench" / "p2_scalar.c"
REPS = {"fmt": "the packed format", "mpq": "GMP mpq_t", "fmpq": "FLINT fmpq"}
DATA = {64: 54, 128: 117}
OAXIS = ((0, 1, 2, 3, 4, 5, 6, 7), (1, 0, 3, 2, 5, 4, 7, 6), (2, 3, 0, 1, 6, 7, 4, 5), (3, 2, 1, 0, 7, 6, 5, 4),
         (4, 5, 6, 7, 0, 1, 2, 3), (5, 4, 7, 6, 1, 0, 3, 2), (6, 7, 4, 5, 2, 3, 0, 1), (7, 6, 5, 4, 3, 2, 1, 0))
OSIGN = ((1, 1, 1, 1, 1, 1, 1, 1), (1, -1, 1, -1, 1, -1, -1, 1), (1, -1, -1, 1, 1, 1, -1, -1),
         (1, 1, -1, -1, 1, -1, 1, -1), (1, -1, -1, -1, -1, 1, 1, 1), (1, 1, -1, 1, -1, -1, -1, 1),
         (1, 1, 1, -1, -1, 1, -1, -1), (1, -1, 1, 1, -1, -1, 1, -1))
M64 = (1 << 64) - 1


def flint_root():
    """The FLINT every phase-two binary links: FLINT_ROOT, else the native rebuild, else the BLAS build."""
    import os
    for r in (os.environ.get("FLINT_ROOT"), "/home/claude/flintblas/inst-native", "/home/claude/flintblas/inst"):
        if r and Path(r, "lib").exists():
            return r


def build():
    if FORCE or not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        root = flint_root()
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        f"-I{root}/include", "-o", str(BIN), str(SRC), f"-L{root}/lib", "-lflint", "-lgmp",
                        f"-Wl,-rpath,{root}/lib"], check=True)


class Inputs:
    """2n values with p and q - 1 together using at most (D - 1)/3 bits, so that sums (whose size can reach three
    operand sizes: numerator p1 q2 + p2 q1, denominator q1 q2) and products fit D bits; real values
    (axis 0) so that every library holds the same numbers."""

    def __init__(self, W, n, seed=0, fill="third"):
        rng = random.Random(zlib.crc32(f"{W} {n} {seed} {fill}".encode()))
        D = DATA[W]
        half = (D - 1) // 3 if fill == "third" else (D - 2) // 2      # "half": products (not sums) still fit
        vals = []
        while len(vals) < 2 * n:
            lp = rng.randint(1, half - 1)
            lq = rng.randint(0, half - lp)
            p = rng.getrandbits(lp) | (1 << (lp - 1))
            q = (rng.getrandbits(lq) | (1 << (lq - 1))) + 1 if lq else 1
            if gcd(p, q) == 1 and (q - 1).bit_length() == lq:
                vals.append((rng.choice((1, -1)), 0, p, q))
        vals[0] = (1, 0, 0, 1)
        if n > 3:
            vals[3] = vals[2]
        self.vals, self.W, self.n = vals, W, n
        self.dir = tempfile.TemporaryDirectory()
        self.path = Path(self.dir.name) / "in.bin"
        with open(self.path, "wb") as f:
            for s, a, p, q in vals:
                f.write(struct.pack("<bb6xQQQQ", s, a, p & M64, p >> 64, q & M64, q >> 64))

    def expected(self, op):
        F = lambda v: v[0] * Fraction(v[2], v[3])
        a, b = self.vals[:self.n], self.vals[self.n:]
        if op == "sort":
            return sorted(range(self.n), key=lambda i: F(a[i]))
        out = []
        for x, y in zip(a, b):
            r = F(x) + F(y) if op == "add" else F(x) * F(y)
            out.append((1 if r >= 0 else -1, 0, abs(r.numerator), r.denominator) if r else (1, 0, 0, 1))
        return out

    def run(self, rep, op, reps=5):
        out = Path(self.dir.name) / "out.txt"
        r = subprocess.run([str(BIN), rep, op, str(self.W), str(self.n), str(reps), str(self.path), str(out)],
                           capture_output=True, text=True, check=True)
        med, mn, fails = r.stdout.split()
        lines = out.read_text().split("\n")[:-1]
        got = [int(x) for x in lines] if op == "sort" else [tuple(int(t) for t in ln.split()) for ln in lines]
        return float(med), float(mn), int(fails), got
