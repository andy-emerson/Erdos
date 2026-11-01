"""round1.py: spec 13 §4, round 1 (correctness). Runs designs A and B on every listed set and operation, and the
value-only four-direction check, and writes out/r1.json and a summary.

Each pair: operands built (test setup), the operation, then the checks:
- value: the model's own word-by-word readout equals Python's (B: P - N);
- canonical: smallest class, zero as class 0 word 0 (A: header 0), spare bits 0; B: every part, and collapsed results
  (division, gcd, and the output collapse run on every B result) have one part zero;
- bound: no routine wrote past its §1 bound;
- release: after the operands, the result (and B's output collapse) are released, every slot is on its pool's free list;
- error: the operation raised (overflow past a destination, class above 31, an index too wide for a word, or a Python
  fault such as reading past memory).
After a failing pair the pools are reset, so one fault does not cascade.

Modes: "literal" is the spec as written (unwritten memory holds all ones). "zeroed" is a diagnostic, not the spec:
every allocation zeroes its slot, to see what fails besides stale words.
Usage: python3 round1.py [random pairs per operation, default 100000]"""
import json
import math
import random
import sys
import time
from multiprocessing import Pool
from pathlib import Path

import words
from words import Pools
from model_a import A
from model_b import B
import axes

OPS = ("add", "sub", "neg", "mul", "div", "gcd", "cmp")
OUT = Path(__file__).resolve().parent / "out"


def ref(op, a, b):
    return {"add": lambda: a + b, "sub": lambda: a - b, "neg": lambda: -a, "mul": lambda: a * b,
            "div": lambda: a // b, "gcd": lambda: math.gcd(a, b), "cmp": lambda: (a > b) - (a < b)}[op]()


# ------------------------------------------------------------------------------------------------ one pair
def run_pair(design, op, xa, xb, pools):
    """xa, xb: A ints; B (P, N) pairs. Returns a list of (kind, detail, got)."""
    m = A(pools) if design == "A" else B(pools)
    mk = (lambda v: m.make(v)) if design == "A" else (lambda v: m.make(*v))
    val = (lambda v: v) if design == "A" else (lambda v: v[0] - v[1])
    a, b = val(xa), (val(xb) if xb is not None else None)
    want = ref(op, a, b)
    fails = []
    try:
        x = mk(xa)
        y = mk(xb) if xb is not None else None
        r = getattr(m, op)(x, y) if y is not None else m.neg(x)
        if op == "cmp":
            if r != want:
                fails.append(("value", "", r))
        else:
            got = m.value(r)
            if got != want:
                fails.append(("value", "", got))
            if design == "A":
                e = m.canonical(r)
            else:
                e = m.canonical(r, collapsed=op in ("div", "gcd"))
            if e:
                fails.append(("canonical", e, r))
            if design == "B":                       # output collapses
                c = m.collapse(r)
                g = m.value(c)
                if g != want:
                    fails.append(("collapse value", "", g))
                e = m.canonical(c, collapsed=True)
                if e:
                    fails.append(("collapse canonical", e, c))
                m.release(c)
            m.release(r)
        if m.breach:
            fails.append(("bound", "; ".join(m.breach), None))
        m.release(x)
        if y is not None:
            m.release(y)
        e = pools.check()
        if e:
            fails.append(("release", e, None))
    except Exception as ex:                         # noqa: BLE001
        fails.append(("error", f"{type(ex).__name__}: {ex}", None))
    return want, fails


# ------------------------------------------------------------------------------------------------ the sets
def signed8(op):
    vs = range(-255, 256)
    if op == "neg":
        return [(a, None) for a in vs]
    if op == "div":
        return [(a, b) for b in vs if b for a in vs if a % b == 0]
    return [(a, b) for a in vs for b in vs]


def unc4(op):
    vs = [(p, n) for p in range(16) for n in range(16)]
    if op == "neg":
        return [(x, None) for x in vs]
    if op == "div":
        return [(x, y) for y in vs if y[0] != y[1] for x in vs if (x[0] - x[1]) % (y[0] - y[1]) == 0]
    return [(x, y) for x in vs for y in vs]


def mag(rng, lo=1, hi=8, zero=True):
    """a magnitude of lo..hi words (top word nonzero); zero at 1/16"""
    if zero and rng.random() < 1 / 16:
        return 0
    n = rng.randint(lo, max(lo, hi))
    return rng.randint(1 << (4 * (n - 1)), (1 << (4 * n)) - 1)


def nwords(v):
    return (abs(v).bit_length() + 3) // 4


def sgn(rng, v):
    return -v if rng.random() < 0.5 else v


def rand_a(op, rng):
    if op == "div":
        b = sgn(rng, mag(rng, zero=False))
        q = 1 if rng.random() < 1 / 16 else mag(rng, 1, 8 - nwords(b) + 1)
        return sgn(rng, q) * b, b
    if op == "gcd" and rng.random() < 0.5:
        g = mag(rng, 1, 4, zero=False)
        h = 8 - nwords(g)
        return sgn(rng, g * mag(rng, 1, h)), sgn(rng, g * mag(rng, 1, h))
    a = sgn(rng, mag(rng))
    if op == "neg":
        return a, None
    b = sgn(rng, abs(a)) if rng.random() < 1 / 16 else sgn(rng, mag(rng))
    return a, b


def rand_bpart(rng):
    p = mag(rng)
    n = p if rng.random() < 1 / 16 else mag(rng)
    return p, n


def as_parts(rng, v):
    """an uncollapsed (P, N) with P - N = v, the base drawn on its own"""
    base = mag(rng)
    return (base + v, base) if v >= 0 else (base, base - v)


def rand_b(op, rng):
    if op == "div":
        while True:
            y = rand_bpart(rng)
            if y[0] != y[1]:
                break
        b = y[0] - y[1]
        q = 1 if rng.random() < 1 / 16 else mag(rng, 1, 8 - nwords(b) + 1)
        return as_parts(rng, sgn(rng, q) * b), y
    if op == "gcd" and rng.random() < 0.5:
        g = mag(rng, 1, 4, zero=False)
        h = 8 - nwords(g)
        return as_parts(rng, sgn(rng, g * mag(rng, 1, h))), as_parts(rng, sgn(rng, g * mag(rng, 1, h)))
    x = rand_bpart(rng)
    if op == "neg":
        return x, None
    y = (x if rng.random() < 0.5 else x[::-1]) if rng.random() < 1 / 16 else rand_bpart(rng)
    return x, y


def cases(design, setname, op, nrand, seed):
    if setname == "signed8":
        cs = signed8(op)
        if design == "B":
            cs = [((a, 0) if a >= 0 else (0, -a), None if b is None else ((b, 0) if b >= 0 else (0, -b)))
                  for a, b in cs]
        return cs
    if setname == "unc4":
        return unc4(op)
    rng = random.Random(seed)
    f = rand_a if design == "A" else rand_b
    return [f(op, rng) for _ in range(nrand)]


def size(c):
    flat = []
    for v in c:
        if v is None:
            continue
        flat += list(v) if isinstance(v, tuple) else [v]
    return max(abs(v).bit_length() for v in flat), sum(abs(v) for v in flat)


# ------------------------------------------------------------------------------------------------ one task
def task(args):
    design, setname, op, mode, nrand, seed = args
    words.ZERO_SLOTS = mode == "zeroed"
    t0 = time.time()
    pools = Pools()
    stats = {"design": design, "set": setname, "op": op, "mode": mode, "n": 0, "pairs_failed": 0, "kinds": {},
             "max_slots": 0}
    for c in cases(design, setname, op, nrand, seed):
        stats["n"] += 1
        want, fails = run_pair(design, op, c[0], c[1], pools)
        stats["max_slots"] = max(stats["max_slots"], pools.max_slots())
        if fails:
            stats["pairs_failed"] += 1
            pools = Pools()
            for kind, detail, got in fails:
                k = stats["kinds"].setdefault(kind, {"count": 0, "first": None, "min": None})
                k["count"] += 1
                ex = {"inputs": c, "expected": want, "got": got, "detail": detail}
                if k["first"] is None:
                    k["first"] = ex
                if k["min"] is None or size(c) < size(k["min"]["inputs"]):
                    k["min"] = ex
    # does the minimal example fail from empty pools?
    for kind, k in stats["kinds"].items():
        c = k["min"]["inputs"]
        _, f = run_pair(design, op, c[0], c[1], Pools())
        k["min"]["fresh_pools"] = [x[0] for x in f]
    stats["seconds"] = round(time.time() - t0, 1)
    return stats


def main():
    nrand = int(sys.argv[1]) if len(sys.argv) > 1 else 100000
    modes = sys.argv[2].split(",") if len(sys.argv) > 2 else ["literal", "zeroed"]
    tasks = []
    for mode in modes:
        for design, sets in (("A", ("signed8", "random")), ("B", ("signed8", "unc4", "random"))):
            for s in sets:
                for i, op in enumerate(OPS):
                    tasks.append((design, s, op, mode, nrand, 20261101 + 100 * i + (design == "B")))
    t0 = time.time()
    with Pool(2) as p:
        res = list(p.imap_unordered(task, tasks))
        ax = axes.run(nrand, 20261101)
    OUT.mkdir(exist_ok=True)
    res.sort(key=lambda s: (s["mode"], s["design"], s["set"], OPS.index(s["op"])))
    (OUT / "r1.json").write_text(json.dumps({"tasks": res, "axes": ax, "nrand": nrand,
                                             "seconds": round(time.time() - t0)}, indent=1, default=str))
    for s in res:
        ks = ", ".join(f"{k} {v['count']}" for k, v in s["kinds"].items()) or "pass"
        print(f"{s['mode']:7} {s['design']} {s['set']:7} {s['op']:4} n={s['n']:7} fail={s['pairs_failed']:7}"
              f"  slots<={s['max_slots']:2} {s['seconds']:6}s  {ks}")
    for k, v in ax.items():
        print(f"axes {k:28} n={v['n']:7} fail={v['fail']}" + (f"  e.g. {v['example']}" if v["fail"] else ""))
    print(f"total {round(time.time() - t0)} s")


if __name__ == "__main__":
    main()
