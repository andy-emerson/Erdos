"""gen.py: the operands of spec v0.1 §3, from one seeded generator (random.Random, seed 20261030), and the job files.

Jobs k in (1, 2, 4, 8, 16) words, N operations per operation (1024; the toy run fewer). Builder's readings (README):
- a class of k words is the bit lengths 64(k-1)+1 .. 64k, uniform; job 1 is 1 .. 63;
- each drawn quantity of l bits is 2^(l-1) with probability 1/16, else uniform among l-bit numbers;
- add, mul: independent pairs; subtract and absolute difference: add's pairs, each pair in random order, the same order
  for both;
- exact division: n from the class, b of max(1, n//2) bits, q of max(1, n - n//2) bits, a = bq;
- gcd: n from the class, g of max(1, n//2) bits, x and y of max(1, n - n//2) bits each, a = gx, b = gy.
"""
import random
import struct
import sys
from math import gcd
from pathlib import Path

SEED = 20261030
JOBS = (1, 2, 4, 8, 16)
N = 1024
OPS = ("add", "sub", "absdiff", "mul", "divexact", "gcd")
POW2_RATE = 1 / 16


def blen(rng, k):
    return rng.randint(1, 63) if k == 1 else rng.randint(64 * (k - 1) + 1, 64 * k)


def draw(rng, l):
    if rng.random() < POW2_RATE:
        return 1 << (l - 1)
    return (1 << (l - 1)) | rng.getrandbits(l - 1)


def job(rng, k, n):
    add = [(draw(rng, blen(rng, k)), draw(rng, blen(rng, k))) for _ in range(n)]
    swapped = [(b, a) if rng.random() < 0.5 else (a, b) for a, b in add]
    mul = [(draw(rng, blen(rng, k)), draw(rng, blen(rng, k))) for _ in range(n)]
    div = []
    for _ in range(n):
        m = blen(rng, k)
        b, q = draw(rng, max(1, m // 2)), draw(rng, max(1, m - m // 2))
        div.append((b * q, b))
    gc = []
    for _ in range(n):
        m = blen(rng, k)
        h = max(1, m - m // 2)
        g, x, y = draw(rng, max(1, m // 2)), draw(rng, h), draw(rng, h)
        gc.append((g * x, g * y))
    return {"add": add, "sub": swapped, "absdiff": swapped, "mul": mul, "divexact": div, "gcd": gc}


def exact(op, a, b):
    if op == "add":
        return a + b
    if op == "sub":
        return a - b
    if op == "absdiff":
        return abs(a - b)
    if op == "mul":
        return a * b
    if op == "divexact":
        assert a % b == 0
        return a // b
    return gcd(a, b)


def words(x):
    w = []
    while x:
        w.append(x & (2**64 - 1))
        x >>= 64
    return w


def write_job(path, k, pairs):
    n = len(pairs["add"])
    out = [b"INTJ", struct.pack("<II", k, n)]
    for op in OPS:
        for a, b in pairs[op]:
            for v in (a, b):
                w = words(v)
                out.append(struct.pack(f"<I{len(w)}Q", len(w), *w))
    Path(path).write_bytes(b"".join(out))


def read_job(path):
    s = Path(path).read_bytes()
    assert s[:4] == b"INTJ"
    k, n = struct.unpack_from("<II", s, 4)
    p, pairs = 12, {}
    for op in OPS:
        vs = []
        for _ in range(2 * n):
            (m,) = struct.unpack_from("<I", s, p)
            w = struct.unpack_from(f"<{m}Q", s, p + 4)
            p += 4 + 8 * m
            vs.append(sum(x << (64 * i) for i, x in enumerate(w)))
        pairs[op] = list(zip(vs[0::2], vs[1::2]))
    return k, pairs


def generate(folder, n=N, seed=SEED):
    """every job, in order, from one generator; returns {k: path}"""
    folder = Path(folder)
    folder.mkdir(parents=True, exist_ok=True)
    rng, paths = random.Random(seed), {}
    for k in JOBS:
        p = folder / f"job-{k}.bin"
        write_job(p, k, job(rng, k, n))
        paths[k] = p
    return paths


if __name__ == "__main__":
    folder = sys.argv[1] if len(sys.argv) > 1 else "build/work"
    n = int(sys.argv[2]) if len(sys.argv) > 2 else N
    for k, p in generate(folder, n).items():
        print(k, p)
