"""Runs cbench/p1_layout.c (phase one, scalar layouts) and checks every result against the reference coder in
gen_p1_layouts.py."""
import random
import struct
import subprocess
import zlib
import tempfile
from fractions import Fraction
from math import gcd
from pathlib import Path

import gen_p1_layouts as G

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "p1_layout"
SRC = HERE.parent / "cbench" / "p1_layout.c"
LAYOUTS = {L["name"]: L for L in G.layouts()}
QUAT = {f"{k}fitq_top_{w}": dict(kind=k, hdr="fit", dim=4, order="top",
                                  **(G.tuple_layout(4, w) if k == "t" else G.packed_layout(4, w)))
        for k in ("t", "p") for w in G.WORDS}
OAXIS = ((0, 1, 2, 3, 4, 5, 6, 7), (1, 0, 3, 2, 5, 4, 7, 6), (2, 3, 0, 1, 6, 7, 4, 5), (3, 2, 1, 0, 7, 6, 5, 4),
         (4, 5, 6, 7, 0, 1, 2, 3), (5, 4, 7, 6, 1, 0, 3, 2), (6, 7, 4, 5, 2, 3, 0, 1), (7, 6, 5, 4, 3, 2, 1, 0))
OSIGN = ((1, 1, 1, 1, 1, 1, 1, 1), (1, -1, 1, -1, 1, -1, -1, 1), (1, -1, -1, 1, 1, 1, -1, -1),
         (1, 1, -1, -1, 1, -1, 1, -1), (1, -1, -1, -1, -1, 1, 1, 1), (1, 1, -1, 1, -1, -1, -1, 1),
         (1, 1, 1, -1, -1, 1, -1, -1), (1, -1, 1, 1, -1, -1, 1, -1))
M64 = (1 << 64) - 1


def build():
    G.main()
    subprocess.run(["clang", "-O3", "-march=native", "-Wall", "-Wno-shift-count-overflow", "-Wno-unused-function",
                    "-Wno-invalid-feature-combination", "-o", str(BIN), str(SRC)], check=True)


def rand_value(rng, D, fill, dim=2):
    """sign, axis < dim, p, q coprime, with bit lengths of p and q - 1 adding to `fill` (at most D)."""
    if rng.random() < 0.02:
        return 1, 0, 0, 1
    while True:
        lp = rng.randint(1, fill)
        lq = fill - lp
        p = rng.getrandbits(lp) | (1 << (lp - 1))
        q = (rng.getrandbits(lq) | (1 << (lq - 1))) + 1 if lq else 1
        if lq and (q - 1).bit_length() != lq:
            continue
        if gcd(p, q) == 1 and p.bit_length() + (q - 1).bit_length() <= D:
            return rng.choice((1, -1)), rng.randrange(dim), p, q


class Inputs:
    def __init__(self, name, n, op, seed=0):
        L = LAYOUTS[name]
        key = f"{L['word']} {op} {seed}" if op == "mul" else f"{name} {op} {seed}"   # mul: the same operands per word size
        rng = random.Random(zlib.crc32(key.encode()))   # stable across processes
        D = L["D"]
        # products: the same operand sizes for every layout of a word size (half the smallest data width among
        # them), so that the multiply-and-reduce work is identical and only decoding and encoding differ
        dmin = min(x["D"] for x in LAYOUTS.values() if x["word"] == L["word"])
        fill = (lambda: rng.randint(1, max(1, dmin // 2))) if op == "mul" else (lambda: rng.randint(max(1, D - 6), D))
        self.vals = [rand_value(rng, D, fill()) for _ in range(n)]
        self.name, self.L, self.n, self.op = name, L, n, op
        self.dir = tempfile.TemporaryDirectory()
        self.path = Path(self.dir.name) / "in.bin"
        with open(self.path, "wb") as f:
            for s, a, p, q in self.vals:
                f.write(struct.pack("<bb6xQQQQ", s, a, p & M64, p >> 64, q & M64, q >> 64))

    def run(self, storage, reps=5):
        out = Path(self.dir.name) / "out.txt"
        r = subprocess.run([str(BIN), self.name, storage, self.op, str(self.n), str(reps), str(self.path), str(out)],
                           capture_output=True, text=True)
        r.check_returncode()
        med, mn, fails, bpv = r.stdout.split()
        lines = out.read_text().split("\n")[:-1]
        return float(med), float(mn), int(fails), int(bpv), lines

    def check(self, storage, lines, fails):
        """True if the C output is exactly what the reference coder gives."""
        L, op = self.L, self.op
        parse = lambda ln: (int(ln.split()[0]), int(ln.split()[1], 16))
        if op == "dec":
            c = 0
            for s, a, p, q in self.vals:
                if p == 0:
                    s, a, q = 1, 0, 1
                c = (c * 31 + (p & M64) + 3 * (p >> 64) + 5 * (q & M64) + 7 * (q >> 64) + 11 * ((a * 7 + s) & M64)) & M64
            return int(lines[0]) == c
        if op == "enc":
            return fails == 0 and all(parse(ln) == tuple(G.ref_encode(L, *v)) for ln, v in zip(lines, self.vals))
        if op == "mul":
            n2 = self.n // 2
            for i, ln in enumerate(lines):
                (s1, a1, p1, q1), (s2, a2, p2, q2) = self.vals[i], self.vals[i + n2]
                fr = Fraction(p1 * p2, q1 * q2)
                want = (s1 * s2 * OSIGN[a1][a2], OAXIS[a1][a2], fr.numerator, fr.denominator) if fr else (1, 0, 0, 1)
                if G.ref_decode(L, *parse(ln)) != want:
                    return False
            return fails == 0
        if op in ("promo", "move"):
            if op == "move" and L["hdr"] == "uni":
                return [parse(ln) for ln in lines] == [tuple(G.ref_encode(L, *v)) for v in self.vals] and fails == 0
            T = target(self.name, op)
            nf = 0
            for ln, (s, a, p, q) in zip(lines, self.vals):
                want = (s, a, p, q) if p else (1, 0, 0, 1)
                if G.ref_encode(T, *want) is None:
                    nf += 1
                    continue
                if G.ref_decode(T, *parse(ln)) != want:
                    return False
            return nf == fails
        return False


def target(name, op):
    kind, order, w = name.split("_")
    if op == "promo":
        return LAYOUTS[f"{kind}_{order}_{2 * int(w)}"]
    return QUAT[f"{kind[0]}fitq_top_{w}"]


def storages(name):
    return ("aos", "aosp", "soa") if name.startswith("t") else ("flat",)
