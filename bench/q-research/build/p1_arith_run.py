"""Runs cbench/p1_arith.c (phase one, array representations) and checks every result against Python integers."""
import random
import subprocess
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "p1_arith"
SRC = HERE.parent / "cbench" / "p1_arith.c"
REPS = {"bi": "_BitInt(W), compile-time width", "lc": "limbs, length fixed at compile time",
        "lr": "limbs, length set at run time (GMP mpn)", "rns": "residues: 2^64 plus primes",
        "mpz": "GMP mpz_t per element"}
OPS = ("add", "mul", "dot", "div1", "div2", "sort", "scan", "widen")
WIDTHS = (64, 128, 256, 512, 1024, 2048)


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["clang", "-std=c2x", "-O3", "-march=native", "-Wall", "-Wno-invalid-feature-combination",
                        "-o", str(BIN), str(SRC), "-lgmp"], check=True)


def _write(path, vals, W):
    L = W // 64
    mag = np.zeros((len(vals), L), dtype=np.uint64)
    sgn = np.zeros(len(vals), dtype=np.uint8)
    for i, v in enumerate(vals):
        sgn[i] = v < 0
        m = abs(v)
        assert m.bit_length() <= W
        for j in range(L):
            mag[i, j] = (m >> (64 * j)) & 0xFFFFFFFFFFFFFFFF
    mag.tofile(str(path) + ".mag")
    sgn.tofile(str(path) + ".sgn")


class WidthInputs:
    """Vectors that fill width W, as the narrowest-width rule makes them: a, b with W - 12 bits; e1 = 999 a and
    e2 = 1000 a (up to W - 2 bits); ha, hb with (W - 20)/2 - 1 bits stored at width max(64, W/2), whose
    products and dot product need width W. A few values are zero, +-1 and the largest magnitude."""

    def __init__(self, W, n, seed=0):
        rng = random.Random(seed * 7919 + W)
        WH = max(64, W // 2)
        hf, hh = W - 12, (W - 20) // 2 - 1

        def vec(bits, k):
            v = [rng.choice((-1, 1)) * rng.getrandbits(bits) for _ in range(n)]
            top = (1 << bits) - 1
            for i, x in enumerate((0, 1, -1, top, -top)):
                if i < n:
                    v[(i * (k + 3)) % n] = x
            return v
        self.a, self.b = vec(hf, 0), vec(hf, 1)
        self.ha, self.hb = vec(hh, 2), vec(hh, 3)
        if n > 8:
            self.a[6] = self.a[7]                      # a tie, for the stable sort
        self.e1 = [999 * x for x in self.a]
        self.e2 = [1000 * x for x in self.a]
        self.W, self.n = W, n
        self.dir = tempfile.TemporaryDirectory()
        for name in ("a", "b", "e1", "e2"):
            _write(Path(self.dir.name) / name, getattr(self, name), W)
        for name in ("ha", "hb"):
            _write(Path(self.dir.name) / name, getattr(self, name), WH)

    def expected(self, op):
        a, b, ha, hb = self.a, self.b, self.ha, self.hb
        if op == "add":
            return [x + y for x, y in zip(a, b)]
        if op == "mul":
            return [x * y for x, y in zip(ha, hb)]
        if op == "dot":
            return [sum(x * y for x, y in zip(ha, hb))]
        if op in ("div1", "div2"):
            return list(a)
        if op == "sort":
            return sorted(range(self.n), key=lambda i: a[i])          # Python's sort is stable
        if op == "scan":
            return [max(abs(x).bit_length() for x in a)]
        if op == "widen":
            return list(ha)

    def run(self, rep, op, reps=5):
        out = Path(self.dir.name) / f"{rep}-{op}.txt"
        r = subprocess.run([str(BIN), rep, op, str(self.W), str(self.n), str(reps), self.dir.name, str(out)],
                           capture_output=True, text=True)
        r.check_returncode()
        med, mn, w = r.stdout.split()
        got = [int(x) for x in out.read_text().split()]
        return float(med), float(mn), got


class ChainInputs:
    """x <- x b + c DEN^(k+1) from 3-place decimals, as in the family research."""

    def __init__(self, n, den=1000, seed=101, extremes=True):
        rng = np.random.default_rng(seed)
        self.x0 = rng.integers(-10**7, 10**7 + 1, size=n)
        self.cb = rng.integers(-10**4, 10**4 + 1, size=n)
        self.cc = rng.integers(-10**6, 10**6 + 1, size=n)
        if extremes:
            self.x0[:2] = [0, -(2**62)]
            self.cb[2] = 0
            self.cc[3] = 0
        self.n, self.den = n, den
        self.dir = tempfile.TemporaryDirectory()
        for name in ("x0", "cb", "cc"):
            getattr(self, name).astype(np.int64).tofile(Path(self.dir.name) / f"{name}.bin")
        self._cache = {0: [int(v) for v in self.x0]}

    def expected(self, K):
        k0 = max(k for k in self._cache if k <= K)
        x = self._cache[k0]
        cs = self.den ** (k0 + 1)
        b, c = [int(v) for v in self.cb], [int(v) for v in self.cc]
        for _ in range(k0, K):
            x = [xi * bi + ci * cs for xi, bi, ci in zip(x, b, c)]
            cs *= self.den
        self._cache[K] = x
        return x

    def run(self, rep, K, reps=3):
        out = Path(self.dir.name) / f"{rep}-chain-{K}.txt"
        r = subprocess.run([str(BIN), rep, "chain", "64", str(self.n), str(reps), self.dir.name, str(out), str(K),
                            str(self.den)], capture_output=True, text=True)
        if r.returncode == 3:
            return None, None, None, None
        r.check_returncode()
        med, mn, w = r.stdout.split()
        return float(med), float(mn), int(w), [int(x) for x in out.read_text().split()]


def steps_for_widths(inp, widths=(128, 256, 512, 1024, 2048)):
    """For each width, the largest step count whose proven bound still fits it (the rule in p1_arith.c)."""
    bx = max(abs(int(v)).bit_length() for v in inp.x0)
    bb = max(abs(int(v)).bit_length() for v in inp.cb)
    bc = max(abs(int(v)).bit_length() for v in inp.cc)
    out, pw, k = {}, 1, 0
    while True:
        pw *= inp.den
        nb = max(bx + bb, bc + pw.bit_length()) + 1
        if nb + 1 > widths[-1]:
            break
        bx, k = nb, k + 1
        for w in widths:
            if nb + 1 <= w and (w // 2 < nb + 1 or w == widths[0]):
                out[w] = k
    return out
