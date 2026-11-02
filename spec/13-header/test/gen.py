"""gen.py: spec 13 v0.2 §7's operands (timed trials), the job files read by src/bench.nim, and the exact values.

A job file is one row: "H13R" u32 kind (0 integers, 1 complex) u32 op u32 n, then n pairs (a, b). An operand is u32 d,
u64 m (A's and Control's form: magnitude m in direction d, 0..3 for 1, i, -1, -i), u64 p[4] (B's parts: integers P, N
in p[0], p[1]; complex the four slots). One-operand operations (neg, collapse) ignore b.

Timing rows, 1024 pairs each, from one seed (builder's readings, src/README.md Choices):
- magnitudes: bit length uniform 1..62 for add, sub and neg, so sums stay below 2^63 (Control); multiply a factor of
  L1 bits (uniform 1..62) and one of L2 bits (uniform 1..63-L1); exact division a = b q with q of L1 bits and b of L2
  bits, the same way: every product and quotient below 2^63;
- sign mixes: each operand negative with probability 0 ("pos"), 1/16 ("most") or 1/2 ("rand");
- magnitude-order mixes, for add, sub (and collapse): |a| >= |b| in every pair ("ge"), in 15/16 ("most") or in 1/2
  ("half"), the pair swapped to fit;
- complex: each operand on a random axis; for add and sub a nonzero pair across axes has one operand (chosen at
  random) replaced by a zero of its direction, before the order mix;
- B's operands enter collapsed. B's collapse rows collapse the B sums of the add rows' pairs (same mixes).
  python3 test/gen.py [out_dir]       writes the 72 timing rows (default build/work)
"""
import random
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SEED = 20261101
N = 1024
KINDS = ("int", "cx")
OPS = ("add", "sub", "neg", "mul", "div", "collapse")
SIGNS = {"pos": 0.0, "most": 1 / 16, "rand": 1 / 2}
ORDERS = {"ge": 1.0, "most": 15 / 16, "half": 1 / 2}
M64 = (1 << 64) - 1
MIX = 0x9E3779B97F4A7C15


# ------------------------------------------------------------------------------------------------ values
# A value is a pair of Python ints (re, im). An operand carries A's (d, m) and B's parts p (4 slots).
def value_of(d, m):
    return [(m, 0), (0, m), (-m, 0), (0, -m)][d]


def parts_of(kind, d, m):
    """B's collapsed parts"""
    p = [0, 0, 0, 0]
    if kind == "int":
        p[d >> 1] = m
    else:
        p[d] = m
    return p


def value_of_parts(kind, p):
    return (p[0] - p[1], 0) if kind == "int" else (p[0] - p[2], p[1] - p[3])


class Opnd:
    __slots__ = ("d", "m", "p")

    def __init__(self, d, m, p):
        self.d, self.m, self.p = d, m, p

    @staticmethod
    def collapsed(kind, d, m):
        return Opnd(d, m, parts_of(kind, d, m))

    def value(self, kind):
        return value_of_parts(kind, self.p)


ZERO = Opnd(0, 0, [0, 0, 0, 0])


def exact(kind, op, a, b):
    """the exact value of a op b, from the operands' values (B's parts give the same value as A's form)"""
    (ar, ai), (br, bi) = a.value(kind), b.value(kind)
    if op == "add":
        return (ar + br, ai + bi)
    if op == "sub":
        return (ar - br, ai - bi)
    if op == "neg":
        return (-ar, -ai)
    if op == "mul":
        return (ar * br - ai * bi, ar * bi + ai * br)
    if op == "div":
        n = br * br + bi * bi
        xr, xi = ar * br + ai * bi, ai * br - ar * bi      # a * conj(b)
        assert n and xr % n == 0 and xi % n == 0, "inexact division"
        return (xr // n, xi // n)
    if op == "collapse":
        return (ar, ai)
    raise ValueError(op)


def mix(h, x):
    return ((h ^ x) * MIX) & M64


def checksum(values):
    """the binary's checksum: per value (re < 0, |re|, im < 0, |im|)"""
    h = 0
    for re, im in values:
        for x in (int(re < 0), abs(re), int(im < 0), abs(im)):
            h = mix(h, x)
    return f"{h:016x}"


def write_job(path, kind, op, pairs):
    out = [b"H13R", struct.pack("<III", KINDS.index(kind), OPS.index(op), len(pairs))]
    for a, b in pairs:
        for x in (a, b):
            assert 0 <= x.d < 4 and 0 <= x.m <= M64 and all(0 <= y <= M64 for y in x.p), x
            out.append(struct.pack("<IQ4Q", x.d, x.m, *x.p))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"".join(out))


# ------------------------------------------------------------------------------------------------ the timing rows
def bits(rng, lo, hi):
    L = rng.randint(lo, hi)
    return rng.randint(1 << (L - 1), (1 << L) - 1), L


def direction(rng, kind, pneg):
    neg = rng.random() < pneg
    axis = rng.randint(0, 1) if kind == "cx" else 0
    return axis + 2 * neg


def draw(rng, kind, op, pneg, porder):
    """one pair for a timing row"""
    if op in ("add", "sub", "neg"):
        m1, _ = bits(rng, 1, 62)
        m2, _ = bits(rng, 1, 62)
    else:                                   # mul, div: a factor of L1 bits and one of L2 bits, L1 + L2 <= 63
        x, L1 = bits(rng, 1, 62)
        y, _ = bits(rng, 1, 63 - L1)
        m1, m2 = (x, y) if op == "mul" else (x * y, y)
    d1, d2 = direction(rng, kind, pneg), direction(rng, kind, pneg)
    if op in ("add", "sub") and kind == "cx" and (d1 ^ d2) & 1:
        if rng.random() < 0.5:
            m1 = 0
        else:
            m2 = 0
    if op in ("add", "sub") and (m1 >= m2) != (rng.random() < porder):
        m1, m2, d1, d2 = m2, m1, d2, d1
    return Opnd.collapsed(kind, d1, m1), Opnd.collapsed(kind, d2, m2)


def rows():
    """name -> (kind, op, pairs); the collapse rows from the add rows' pairs"""
    out = {}
    for kind in KINDS:
        for op in ("add", "sub", "neg", "mul", "div"):
            for s, pneg in SIGNS.items():
                for o, porder in (ORDERS.items() if op in ("add", "sub") else [(None, None)]):
                    name = f"{kind}-{op}-{s}" + (f"-{o}" if o else "")
                    rng = random.Random(f"{SEED}-{name}")
                    pairs = [draw(rng, kind, op, pneg, porder) for _ in range(N)]
                    if op == "neg":
                        pairs = [(a, ZERO) for a, _ in pairs]
                    out[name] = (kind, op, pairs)
                    if op == "add":
                        cp = []
                        for a, b in pairs:
                            p = [x + y for x, y in zip(a.p, b.p)]
                            v = value_of_parts(kind, p)
                            d, m = to_dm(v)
                            cp.append((Opnd(d, m, p), ZERO))
                        out[f"{kind}-collapse-{s}-{o}"] = (kind, "collapse", cp)
    return out


def to_dm(v):
    re, im = v
    assert re == 0 or im == 0
    if im > 0:
        return 1, im
    if im < 0:
        return 3, -im
    return (0, re) if re >= 0 else (2, -re)


def generate(work):
    """writes every row; returns name -> (path, kind, op, expected checksum)"""
    res = {}
    for name, (kind, op, pairs) in rows().items():
        path = work / f"{name}.bin"
        write_job(path, kind, op, pairs)
        res[name] = (path, kind, op, checksum(exact(kind, op, a, b) for a, b in pairs))
    return res


if __name__ == "__main__":
    work = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "build" / "work"
    r = generate(work)
    print(f"{len(r)} rows in {work}")
