"""r2gen.py: spec 13 round 2's operands (§5 c), from seeded generators, and the job files read by src/bench.nim.

Timing jobs: k in (1, 2, 4, 8, 16) words, N = 1024 pairs per operation, under each sign mix and each magnitude-order
mix (3 x 3 jobs per k). Builder's readings (src/README.md, Choices):
- a k-word magnitude has a bit length uniform in 64(k-1)+1 .. 64k (spec 12's class; job 1 is 1 .. 64);
- add, sub, neg, mul, cmp: each operand drawn on its own; division a = bq with b of max(1, n//2) bits and q of
  max(1, n - n//2) bits, n drawn from the class; gcd a = gx, b = gy, g of max(1, n//2) bits, x and y of
  max(1, n - n//2) bits (spec 12 §3);
- sign mixes: "pos" every operand positive, "most" each operand negative with probability 1/16, "rand" 1/2;
- magnitude-order mixes: "a" |a| >= |b| in every pair, "most" in 15/16 of the pairs, "rand" in 1/2 (the pair is
  swapped to fit); division keeps a = bq (|a| >= |b| always) and negate has one operand, so the mix leaves them as
  drawn;
- the magnitudes are the same in all nine jobs of a k: only the signs and the order change.
Growth workload: 64 chains of 32 steps of x <- x*y + z; x0, y and z one word each (bit length uniform 1 .. 64),
each sign 1/2.

A value in a job file: u32 direction d, then three word lists (u32 count, that many u64, lowest first): the
magnitude (A and Control), and B's parts P and N (collapsed in the timing jobs; the check also gives uncollapsed ones).
  python3 test/r2gen.py [out_dir]     writes the timing jobs and the growth job (default build/r2/work)
"""
import random
import struct
import sys
from math import gcd
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SEED = 20261101
JOBS = (1, 2, 4, 8, 16)
N = 1024
OPS = ("add", "sub", "neg", "mul", "div", "gcd", "cmp")
SIGNS = {"pos": 0.0, "most": 1 / 16, "rand": 1 / 2}
ORDERS = {"a": 1.0, "most": 15 / 16, "rand": 1 / 2}
CHAINS, STEPS = 64, 32
M64 = (1 << 64) - 1


# ------------------------------------------------------------------------------------------------ values
class Val:
    """a signed value for the job file: magnitude m, direction d (0 or 2 for integers; 1 or 3 only in the axes check),
    and B's parts (p, n) with p - n = the value"""
    __slots__ = ("m", "d", "p", "n")

    def __init__(self, m, d=0, p=None, n=None):
        self.m, self.d = m, d
        if p is None:
            p, n = (m, 0) if d == 0 else (0, m)
        self.p, self.n = p, n

    @property
    def v(self):
        """the integer value (directions 0 and 2)"""
        return -self.m if self.d == 2 else self.m

    def __repr__(self):
        return f"Val({self.m:#x}, d={self.d}, p={self.p:#x}, n={self.n:#x})"


def words(x):
    w = []
    while x:
        w.append(x & M64)
        x >>= 64
    return w


def pack_value(v):
    out = [struct.pack("<I", v.d)]
    for x in (v.m, v.p, v.n):
        w = words(x)
        out.append(struct.pack(f"<I{len(w)}Q", len(w), *w))
    return b"".join(out)


def write_job(path, k, pairs):
    """pairs: {op: [(a, b)]}, b a Val (zero for negate)"""
    out = [b"HJ13", struct.pack("<II", k, len(OPS))]
    for op in OPS:
        out.append(struct.pack("<I", len(pairs[op])))
        for a, b in pairs[op]:
            out.append(pack_value(a))
            out.append(pack_value(b))
    Path(path).write_bytes(b"".join(out))


def write_growth(path, x0, yz):
    out = [b"HG13", struct.pack("<II", len(x0), len(yz) // len(x0))]
    out += [pack_value(v) for v in x0]
    for y, z in yz:
        out += [pack_value(y), pack_value(z)]
    Path(path).write_bytes(b"".join(out))


# ------------------------------------------------------------------------------------------------ exact results
def exact(op, a, b):
    """the reference (Python integers): a, b ints"""
    if op == "add":
        return a + b
    if op == "sub":
        return a - b
    if op == "neg":
        return -a
    if op == "mul":
        return a * b
    if op == "div":
        assert b != 0 and a % b == 0
        return a // b
    if op == "gcd":
        return gcd(a, b)
    return (a > b) - (a < b)


MIX = 0x9E3779B97F4A7C15


def checksum(op, results):
    """bench.nim's checksum: over values (negative, length, words); compare's results as r + 1"""
    h = 0
    for r in results:
        if op == "cmp":
            xs = [(r + 1) & M64]
        else:
            w = words(abs(r))
            xs = [int(r < 0), len(w)] + w
        for x in xs:
            h = ((h ^ x) * MIX) & M64
    return f"{h:016x}"


# ------------------------------------------------------------------------------------------------ timing jobs
def blen(rng, k):
    return rng.randint(1, 64) if k == 1 else rng.randint(64 * (k - 1) + 1, 64 * k)


def draw(rng, l):
    return (1 << (l - 1)) | rng.getrandbits(l - 1)


def base_pairs(rng, k, n):
    """magnitude pairs per operation"""
    ps = {}
    for op in ("add", "sub", "neg", "mul", "cmp"):
        ps[op] = [(draw(rng, blen(rng, k)), draw(rng, blen(rng, k))) for _ in range(n)]
    ps["neg"] = [(a, 0) for a, _ in ps["neg"]]
    div = []
    for _ in range(n):
        m = blen(rng, k)
        b, q = draw(rng, max(1, m // 2)), draw(rng, max(1, m - m // 2))
        div.append((b * q, b))
    ps["div"] = div
    gc = []
    for _ in range(n):
        m = blen(rng, k)
        h = max(1, m - m // 2)
        g, x, y = draw(rng, max(1, m // 2)), draw(rng, h), draw(rng, h)
        gc.append((g * x, g * y))
    ps["gcd"] = gc
    return ps


def mixed(ps, k, sname, oname):
    """the sign and order mix applied to the base magnitudes, from their own seeded generator"""
    rng = random.Random(f"{SEED}-{k}-{sname}-{oname}")
    pneg, pfirst = SIGNS[sname], ORDERS[oname]
    out = {}
    for op in OPS:
        lst = []
        for a, b in ps[op]:
            if op not in ("div", "neg"):
                first = rng.random() < pfirst
                if (a >= b) != first and a != b:
                    a, b = b, a
            da = 2 if rng.random() < pneg else 0
            db = 2 if rng.random() < pneg else 0
            if op == "neg":
                lst.append((Val(a, da), Val(0)))
            else:
                lst.append((Val(a, da), Val(b, db)))
        out[op] = lst
    return out


def job_names():
    return [f"k{k}-s{s}-o{o}" for k in JOBS for s in SIGNS for o in ORDERS]


def generate(work, n=N):
    """every timing job and the growth job; returns {name: (path, pairs)} and the growth (path, x0, yz)"""
    work = Path(work)
    work.mkdir(parents=True, exist_ok=True)
    rng = random.Random(SEED)
    jobs = {}
    for k in JOBS:
        ps = base_pairs(rng, k, n)
        for s in SIGNS:
            for o in ORDERS:
                name = f"k{k}-s{s}-o{o}"
                pairs = mixed(ps, k, s, o)
                write_job(work / f"{name}.bin", k, pairs)
                jobs[name] = (work / f"{name}.bin", pairs)
    g = random.Random(SEED + 1)
    one = lambda: Val(draw(g, g.randint(1, 64)), 2 if g.random() < 0.5 else 0)  # noqa: E731
    x0 = [one() for _ in range(CHAINS)]
    yz = [(one(), one()) for _ in range(CHAINS * STEPS)]
    write_growth(work / "growth.bin", x0, yz)
    return jobs, (work / "growth.bin", x0, yz)


def growth_exact(x0, yz):
    res = []
    steps = len(yz) // len(x0)
    for c, x in enumerate(x0):
        v = x.v
        for s in range(steps):
            y, z = yz[c * steps + s]
            v = v * y.v + z.v
        res.append(v)
    return res


if __name__ == "__main__":
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "build" / "r2" / "work"
    jobs, _ = generate(out)
    print(f"{len(jobs)} jobs and the growth job in {out}")
