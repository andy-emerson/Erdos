"""check.py: every build against exact answers computed here with Python's integers (a test of the builds, not a model).

Case sets, for integers and for complex numbers on the axes, for every operation:
- "ctl": every magnitude and every result part below 2^63, so Control can hold them: all four builds;
- "wide": magnitudes up to 2^64 - 1 with no result overflowing a word (A's magnitude, B's parts): A, A's branch-free
  row and B;
- "bun": B with uncollapsed operands (both parts nonzero), no part overflowing: B only.
Each set has edge cases (zeros in every direction, equal magnitudes, magnitudes near 2^32, 2^63 and 2^64, every
direction pair) and random ones (bit lengths uniform 0..64 or 0..63, zero included). For add and sub, complex
operands on different axes have a zero (two nonzero ones are the error cases below).

What is checked, per result:
- the value equals the exact value (all builds);
- A (both rows): header bits 0-5 are 0, and the header is the one §3's rules give (so a zero's direction too);
  the header form (alg) gives x's direction for two zeros on different axes (LargerRow.lean), so for alg any header
  with bits 0-5 clear is accepted on a zero sum or difference;
- B: the parts are exactly §4's: add, sub, neg and mul part by part; collapse with one part of each axis 0; division
  with the quotient in slot (d1 - d2) mod 4 of the collapsed operands' directions and the other slots 0.
Also: errors (A and A's branch-free row exit 3 on a sum or difference across axes of two nonzero numbers, alone or
after valid cases); each timing row's checksum in every build against the exact values' (gen.py); bytes per value.

  python3 test/check.py [--quick]     prints PASS or FAILED (exit 1)
"""
import argparse
import random
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen  # noqa: E402
from gen import Opnd, M64, exact, value_of_parts, to_dm  # noqa: E402

HERE = gen.HERE
BIN = HERE / "build" / "bin"
WORK = HERE / "build" / "check"
BUILDS = ("control", "a", "abf", "anm", "amul", "alg", "b")
D = 0xC0
E63 = [0, 1, 2, 3, 0xFFFFFFFF, 1 << 32, (1 << 32) + 1, (1 << 62) - 1, 1 << 62, (1 << 62) + 1, (1 << 63) - 2,
       (1 << 63) - 1]
E64 = E63 + [1 << 63, (1 << 63) + 1, M64 - (1 << 32), M64 - 1, M64]
LIM = {"ctl": 1 << 63, "wide": 1 << 64, "bun": 1 << 64}
SET_BUILDS = {"ctl": BUILDS, "wide": ("a", "abf", "anm", "amul", "alg", "b"), "bun": ("b",)}


# ------------------------------------------------------------------------------------------------ §3 and §4 as stated
def a_header(op, a, b):
    """the header §3 gives (h = d shl 6)"""
    h1, h2 = a.d << 6, b.d << 6
    if op == "sub":
        op, h2 = "add", h2 ^ 0x80
    if op == "add":
        t = (h1 ^ h2) & D
        if t == 0:
            return h1 & D
        if t == 0x80:
            return (h1 + ((a.m < b.m) << 7)) & D
        assert a.m == 0 or b.m == 0
        return (h2 if a.m == 0 else h1) & D
    if op == "neg":
        return h1 ^ 0x80
    if op == "mul":
        return (h1 + h2) & D
    if op == "div":
        return (h1 - h2) & D
    raise ValueError(op)


def b_parts(kind, op, a, b):
    """the parts §4 gives"""
    p, q = a.p, b.p
    if kind == "int":
        if op == "add":
            return [p[0] + q[0], p[1] + q[1]]
        if op == "sub":
            return [p[0] + q[1], p[1] + q[0]]
        if op == "neg":
            return [p[1], p[0]]
        if op == "mul":
            return [p[0] * q[0] + p[1] * q[1], p[0] * q[1] + p[1] * q[0]]
    else:
        if op == "add":
            return [x + y for x, y in zip(p, q)]
        if op == "sub":
            return [p[i] + q[(i + 2) % 4] for i in range(4)]
        if op == "neg":
            return [p[2], p[3], p[0], p[1]]
        if op == "mul":
            r = [0, 0, 0, 0]
            for i in range(4):
                for j in range(4):
                    r[(i + j) % 4] += p[i] * q[j]
            return r
    if op == "collapse":
        v = value_of_parts(kind, p)
        d, m = to_dm(v)
        return gen.parts_of(kind, d, m)[:2 if kind == "int" else 4]
    if op == "div":
        (d1, m1), (d2, m2) = to_dm(value_of_parts(kind, p)), to_dm(value_of_parts(kind, q))
        d = (d1 - d2) % 4
        r = gen.parts_of(kind, d, m1 // m2)
        return r[:2] if kind == "int" else r
    raise ValueError(op)


# ------------------------------------------------------------------------------------------------ the cases
def fits(kind, op, a, b, s):
    """no overflow in the builds the set is for"""
    lim = LIM[s]
    if a.m >= lim or b.m >= lim or any(x >= lim for x in a.p + b.p):
        return False
    if op == "div" and (b.m == 0 or a.m % b.m):
        return False
    v = exact(kind, op, a, b)
    if any(abs(x) >= lim for x in v):
        return False
    if op in ("add", "sub") and kind == "cx" and (a.d ^ b.d) & 1 and a.m and b.m:
        return False                        # an expression: the error cases
    if s == "ctl":
        return True
    if s == "wide":
        if op in ("add", "sub") and kind == "int" or op in ("add", "sub") and not ((a.d ^ b.d) & 1):
            hb = b.d ^ (2 if op == "sub" else 0)
            if a.d == hb and a.m + b.m > M64:
                return False
        if op == "mul" and a.m * b.m > M64:
            return False
    if op != "collapse" and op != "div":
        if any(x > M64 for x in b_parts(kind, op, a, b)):
            return False
    return True


def dirs(kind):
    return (0, 2) if kind == "int" else (0, 1, 2, 3)


def uncollapsed(kind, d, m, k1, k2):
    """B parts with value m i^d, each axis padded: (x + k, k)"""
    p = gen.parts_of(kind, d, m)
    if kind == "int":
        return [p[0] + k1, p[1] + k1, 0, 0]
    return [p[0] + k1, p[1] + k2, p[2] + k1, p[3] + k2]


def edge_cases(kind, op, s):
    E = E63 if s == "ctl" else E64
    out = []
    if s == "bun":
        pads = [1, 2, 0xFFFFFFFF, 1 << 62, (1 << 63) - 1, M64 - 3]
        vals = [0, 1, 3, 1 << 32, (1 << 62) - 1, 1 << 63, M64 - (1 << 62)]
        for d in dirs(kind):
            for m in vals:
                for k in pads:
                    if m + k <= M64:
                        x = uncollapsed(kind, d, m, k, k if kind == "cx" else 0)
                        out.append(Opnd(d, m, x))
        ops1 = out
        pairs = []
        rng = random.Random(f"bun-{kind}-{op}")
        if op in ("neg", "collapse"):
            pairs = [(x, gen.ZERO) for x in ops1]
        else:
            for x in ops1:
                for _ in range(12):
                    y = rng.choice(ops1)
                    if op == "div":
                        if y.m == 0:
                            continue
                        m, k = y.m * rng.choice([0, 1, 2, 3, 0xFFFF]), rng.choice(pads)
                        if m + k > M64:
                            continue
                        x = Opnd(x.d, m, uncollapsed(kind, x.d, m, k, k if kind == "cx" else 0))
                    pairs.append((x, y))
        return [(a, b) for a, b in pairs if fits(kind, op, a, b, s)]
    C = lambda d, m: Opnd.collapsed(kind, d, m)  # noqa: E731
    if op in ("neg", "collapse"):
        for d in dirs(kind):
            for m in E:
                out.append((C(d, m), gen.ZERO))
        if op == "collapse":                   # B's collapse: uncollapsed parts, all edge pairs
            out = []
            for x in E:
                for y in E:
                    p = [x, y, 0, 0] if kind == "int" else [x, 0, y, 0]
                    out.append((Opnd(*to_dm(value_of_parts(kind, p)), p), gen.ZERO))
                    if kind == "cx":
                        p = [0, x, 0, y]
                        out.append((Opnd(*to_dm(value_of_parts(kind, p)), p), gen.ZERO))
                        p = [x, x, y, x]          # imaginary axis zero, uncollapsed
                        out.append((Opnd(*to_dm(value_of_parts(kind, p)), p), gen.ZERO))
    elif op == "div":
        for d1 in dirs(kind):
            for d2 in dirs(kind):
                for bm in E:
                    for q in E:
                        if bm:
                            out.append((C(d1, bm * q), C(d2, bm)))
    else:
        for d1 in dirs(kind):
            for d2 in dirs(kind):
                for m1 in E:
                    for m2 in E:
                        out.append((C(d1, m1), C(d2, m2)))
    return [(a, b) for a, b in out if fits(kind, op, a, b, s)]


def random_cases(kind, op, s, n):
    rng = random.Random(f"{gen.SEED}-check-{kind}-{op}-{s}")
    top = 63 if s == "ctl" else 64

    def mag(lo=0, hi=top):
        L = rng.randint(lo, hi)
        return 0 if L == 0 else rng.randint(1 << (L - 1), (1 << L) - 1)

    out = []
    while len(out) < n:
        d1, d2 = rng.choice(dirs(kind)), rng.choice(dirs(kind))
        if op in ("mul", "div"):
            L = rng.randint(1, top)
            x, y = mag(0, L), mag(1, max(1, top - L))
            m1, m2 = (x, y) if op == "mul" else (x * y, y)
        else:
            m1, m2 = mag(), mag()
        if rng.random() < 1 / 16:
            m2 = m1                              # equal magnitudes
        if op in ("add", "sub", "collapse") and kind == "cx" and (d1 ^ d2) & 1 and m1 and m2:
            if rng.random() < 0.5:
                m1 = 0
            else:
                m2 = 0
        if s == "bun":
            def pad(d, m):
                k1, k2 = mag(1, 63), mag(1, 63) if kind == "cx" else 0
                return Opnd(d, m, uncollapsed(kind, d, m, k1, k2))
            a, b = pad(d1, m1), pad(d2, m2)
        else:
            a, b = Opnd.collapsed(kind, d1, m1), Opnd.collapsed(kind, d2, m2)
        if op in ("neg", "collapse"):
            b = gen.ZERO
        if op == "collapse" and s != "bun":
            p = [x + y for x, y in zip(a.p, Opnd.collapsed(kind, d2, m2).p)]
            if any(x > M64 for x in p):
                continue
            a = Opnd(*to_dm(value_of_parts(kind, p)), p)
        if fits(kind, op, a, b, s):
            out.append((a, b))
    return out


# ------------------------------------------------------------------------------------------------ running the builds
def run(b, *args):
    return subprocess.run([str(BIN / b), *map(str, args)], capture_output=True, text=True)


def parse(b, kind, line):
    t = line.split()
    if b == "control":
        return (int(t[0]), 0) if kind == "int" else (int(t[0]), int(t[1])), None
    if b in ("a", "abf", "anm", "amul", "alg"):
        h, m = int(t[0], 16), int(t[1], 16)
        return gen.value_of(h >> 6, m) if not h & 0x3F else (None, None), h
    p = [int(x, 16) for x in t]
    return value_of_parts(kind, p + [0, 0]), p


def check_set(kind, op, s, pairs, fails):
    path = WORK / f"{kind}-{op}-{s}.bin"
    gen.write_job(path, kind, op, pairs)
    n = 0
    for b in SET_BUILDS[s]:
        if op == "collapse" and b != "b":
            continue
        out = WORK / f"{kind}-{op}-{s}.{b}.txt"
        p = run(b, "dump", path, out)
        if p.returncode:
            fails.append(f"{b} {kind} {op} {s}: exit {p.returncode} {p.stderr.strip()}")
            continue
        lines = out.read_text().splitlines()
        if len(lines) != len(pairs):
            fails.append(f"{b} {kind} {op} {s}: {len(lines)} results for {len(pairs)} pairs")
            continue
        for (x, y), line in zip(pairs, lines):
            n += 1
            want = exact(kind, op, x, y)
            got, raw = parse(b, kind, line)
            bad = got != want
            if b in ("a", "abf", "anm", "amul") and not bad:
                bad = raw != a_header(op, x, y)
            if b == "alg" and not bad:
                # the header form gives x's direction for two zeros on different axes, where §3 gives y's
                # (lean/Header/LargerRow.lean, larger_add_eq): any header (bits 0-5 clear) is accepted for a zero sum
                zero_sum = op in ("add", "sub") and want == (0, 0)
                bad = raw & 0x3F != 0 if zero_sum else raw != a_header(op, x, y)
            if b == "b" and not bad:
                bad = raw != b_parts(kind, op, x, y)
            if bad:
                fails.append(f"{b} {kind} {op} {s}: a=(d{x.d} {x.m:#x} p{[hex(v) for v in x.p]}) "
                             f"b=(d{y.d} {y.m:#x} p{[hex(v) for v in y.p]}): got {line!r}, want value {want}")
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true", help="fewer random cases")
    a = ap.parse_args()
    nrand = 500 if a.quick else 4000
    WORK.mkdir(parents=True, exist_ok=True)
    fails, total = [], {}
    for kind in gen.KINDS:
        for op in gen.OPS:
            for s in ("ctl", "wide", "bun"):
                if op == "collapse" and s == "ctl":
                    continue                         # B's only: its sets are wide and bun
                pairs = edge_cases(kind, op, s)
                ne = len(pairs)
                pairs += random_cases(kind, op, s, nrand)
                n = check_set(kind, op, s, pairs, fails)
                total[(kind, op, s)] = (ne, len(pairs) - ne, n)
                print(f"{kind:3} {op:8} {s:4}: {ne:5} edge + {len(pairs) - ne:5} random cases, {n:6} results checked",
                      flush=True)
    # errors: two nonzero numbers across axes, alone and after valid cases
    rng = random.Random(1)
    nerr = 0
    for op in ("add", "sub"):
        for d1 in range(4):
            for d2 in range(4):
                if not (d1 ^ d2) & 1:
                    continue
                for lead in (0, 5):
                    pre = [(Opnd.collapsed("cx", 0, rng.randint(1, 99)), Opnd.collapsed("cx", 2, rng.randint(1, 99)))
                           for _ in range(lead)]
                    path = WORK / f"err-{op}-{d1}{d2}-{lead}.bin"
                    gen.write_job(path, "cx", op, pre + [(Opnd.collapsed("cx", d1, 7), Opnd.collapsed("cx", d2, 1))])
                    for b in ("a", "abf", "anm", "amul", "alg"):
                        nerr += 1
                        p = run(b, "dump", path, WORK / "err.txt")
                        if p.returncode != 3:
                            fails.append(f"{b} error case {op} d{d1} d{d2} lead {lead}: exit {p.returncode}, want 3")
    print(f"errors: {nerr} cases (A and A's branch-free row: exit 3)")
    # the timing rows: each build's checksum against the exact values'
    rows = gen.generate(HERE / "build" / "work")
    nsum = 0
    for name, (path, kind, op, want) in rows.items():
        for b in BUILDS:
            p = run(b, "sum", path)
            if op == "collapse" and b != "b":
                if p.returncode != 4:
                    fails.append(f"{b} {name}: exit {p.returncode}, want 4 (no collapse)")
                continue
            nsum += 1
            got = p.stdout.split()[1] if p.returncode == 0 else f"exit {p.returncode}"
            if got != want:
                fails.append(f"{b} {name}: checksum {got}, want {want}")
    print(f"timing rows: {len(rows)} rows, {nsum} checksums against the exact values")
    want = {"control": (8, 16), "a": (9, 9), "abf": (9, 9), "anm": (9, 9), "amul": (9, 9), "alg": (9, 9), "b": (16, 32)}
    for b in BUILDS:
        t = run(b, "bytes").stdout.split()
        if (int(t[3]), int(t[5])) != want[b]:
            fails.append(f"{b} bytes per value {t}, want {want[b]}")
    print("bytes per value: " + ", ".join(f"{b} {w[0]} ({w[1]} complex)" for b, w in want.items()))
    for f in fails[:40]:
        print("FAIL " + f)
    nres = sum(v[2] for v in total.values())
    print(f"{nres} results, {nerr} error cases, {nsum} checksums: " +
          ("PASS" if not fails else f"FAILED ({len(fails)} failures)"))
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
