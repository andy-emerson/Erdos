"""r2check.py: spec 13 round 2's correctness check. Every build (Control, A, A's branch-free row, B) against round 1's
Python reference models (model_a.py, model_b.py, words.py), run here at W = 64, and against Python's exact values.

1. Check jobs (build/r2/check): edge-case operands (zeros of each direction, equal magnitudes, unequal lengths, all-ones
   words whose carries leave the top word, B's parts uncollapsed), random operands (up to 8 words, zeros and equal
   magnitudes at 1/16, half of B's operands uncollapsed), and, for A and Control, operands in all four directions.
   Each build's binary runs each operation twice over the job (`dump`: two passes, so results are released and their
   slots reused) and prints every result raw. The model replays the same steps from empty pools (the same operands
   made in the same order, the same two passes), so the check is exact:
   - A and A's branch-free row: header (class, spare bit, direction, a zero's direction too), word (the inline
     magnitude or the slot index) and the whole slot, all equal to model A's;
   - B: header, both words and both slots equal to model B's;
   - Control: direction bits equal to model A's, class bits 0, the block's length its real length, magnitude equal;
   - every value equal to Python's (any zero equal to any zero); compare's result equal;
   - after everything is released, every slot of every pool is on its free list exactly once (the binary checks).
2. Errors: gcd(0, 0) in every direction (and B's uncollapsed zeros), and, for A and Control, sums and compares across
   axes: the binary must stop with the spec error (exit 3).
3. Timing jobs and growth: every build's checksum (`sum`) equals the one computed from Python's exact values, so every
   row computes the same thing; the growth workload's results are also replayed in the models, as in 1.
  python3 test/r2check.py [--quick]      (after src/build.sh; --quick skips the timing jobs' checksums)
"""
import argparse
import os
import random
import subprocess
import sys
import time
from math import gcd
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import words  # noqa: E402

# round 1's models at W = 64: words.py reads W, M, POISON and MAX_SLOTS as module globals at call time
words.W = 64
words.M = (1 << 64) - 1
words.POISON = words.M
words.MAX_SLOTS = words.M
import model_a  # noqa: E402
import model_b  # noqa: E402
model_a.POISON = model_b.POISON = words.M
from words import Pools, part_from_int, cls  # noqa: E402
import r2gen  # noqa: E402
from r2gen import Val, OPS, exact, checksum, M64  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
BIN = Path(os.environ.get("R2BIN", HERE / "build" / "bin"))
BUILDS = ("control", "a", "abf", "b")
AXES_BUILDS = ("control", "a", "abf")


# ------------------------------------------------------------------------------------------------ check jobs
def edge_mags():
    rng = random.Random(7)
    return [0, 1, 2, 3, 1 << 63, M64, 1 << 64, (1 << 64) + 1, (1 << 128) - 1, 1 << 128, (1 << 192) - 1,
            (1 << 512) - 1, (1 << 1024) - 1, 1 << 1023, (1 << 1024) - (1 << 960), rng.getrandbits(192) | 1 << 191,
            rng.getrandbits(1024) | 1 << 1023]


BASES = (0, M64, (1 << 128) - 1, 0, (1 << 512) - 1, 0, 12345)


def with_base(v, i):
    """B's parts for v, uncollapsed by a base drawn from BASES by index i"""
    base = BASES[i % len(BASES)]
    return Val(v.m, v.d, v.p + base, v.n + base)


def edge_job():
    vals = []
    for m in edge_mags():
        vals += [Val(m, 0), Val(m, 2)]                       # zeros in directions 0 and 2
    small = [v for v in vals if v.m < 1 << 192]
    pairs = {}
    i = 0
    for op in OPS:
        lst = []
        if op == "neg":
            src = [(a, Val(0)) for a in vals]
        elif op == "div":
            src = [(Val(b.m * q.m, (b.d + q.d) % 4), b) for b in vals if b.m for q in vals]
        elif op == "gcd":
            src = [(a, b) for a in vals for b in (vals if a.m < 1 << 192 else small) if a.m or b.m]
        else:
            src = [(a, b) for a in vals for b in vals]
        for a, b in src:
            i += 1
            lst.append((with_base(a, i), with_base(b, i + 3)) if i % 2 else (a, b))
        pairs[op] = lst
    return pairs


def rand_mag(rng, lo=1, hi=8, zero=True):
    if zero and rng.random() < 1 / 16:
        return 0
    n = rng.randint(lo, max(lo, hi))
    return rng.randint(1 << (64 * (n - 1)), (1 << (64 * n)) - 1)


def rand_job(n=4096, seed=20261102):
    rng = random.Random(seed)
    sgn = lambda m: Val(m, 2 if rng.random() < 0.5 else 0)    # noqa: E731  zeros take 0 or 2 too

    def unc(v):
        if rng.random() < 0.5:
            base = rand_mag(rng)
            return Val(v.m, v.d, v.p + base, v.n + base)
        return v
    pairs = {}
    for op in OPS:
        lst = []
        for _ in range(n):
            if op == "div":
                b = rand_mag(rng, zero=False)
                q = 1 if rng.random() < 1 / 16 else rand_mag(rng, 1, 9 - (b.bit_length() + 63) // 64)
                a, b = sgn(b * q), sgn(b)
            elif op == "gcd" and rng.random() < 0.5:
                g = rand_mag(rng, 1, 4, zero=False)
                h = 8 - (g.bit_length() + 63) // 64
                a, b = sgn(g * rand_mag(rng, 1, h)), sgn(g * rand_mag(rng, 1, h))
            else:
                a = sgn(rand_mag(rng))
                b = sgn(a.m) if rng.random() < 1 / 16 else sgn(rand_mag(rng))
            if op == "gcd" and a.m == 0 and b.m == 0:
                b = sgn(1)
            if op == "neg":
                b = Val(0)
            lst.append((unc(a), unc(b)))
        pairs[op] = lst
    return pairs


def axes_job():
    """A and Control only: every direction, zeros included; sums and compares only where they are defined"""
    vals = [Val(m, d) for m in (0, 1, 5, M64, 1 << 64, (1 << 130) + 7) for d in range(4)]
    allp = [(a, b) for a in vals for b in vals]
    same_axis = lambda a, b: (a.d ^ b.d) & 1 == 0 or a.m == 0 or b.m == 0    # noqa: E731
    return {"add": [p for p in allp if same_axis(*p)], "sub": [p for p in allp if same_axis(*p)],
            "neg": [(a, Val(0)) for a in vals], "mul": allp,
            "div": [(Val(b.m * q.m, (b.d + q.d) % 4), b) for b in vals if b.m for q in vals],
            "gcd": [p for p in allp if p[0].m or p[1].m], "cmp": [p for p in allp if same_axis(*p)]}


def error_cases():
    """(name, op, a, b, builds): each must stop with the spec error"""
    z0, z2 = Val(0, 0), Val(0, 2)
    out = [(f"gcd00-{a.d}{b.d}", "gcd", a, b, BUILDS) for a in (z0, z2) for b in (z0, z2)]
    out.append(("gcd00-B-uncollapsed", "gcd", Val(0, 0, 5, 5), Val(0, 0, M64, M64), ("b",)))
    for op in ("add", "sub", "cmp"):
        out.append((f"{op}-axes", op, Val(5, 0), Val(3, 1), AXES_BUILDS))
        out.append((f"{op}-axes-big", op, Val(1 << 200, 2), Val(M64, 3), AXES_BUILDS))
    return out


# ------------------------------------------------------------------------------------------------ the models, replayed
def replay(design, pairs, op, passes=2):
    """the binary's dump, step for step: every operation's operands made in order, then `passes` passes of op"""
    pools = Pools()
    m = model_a.A(pools) if design == "A" else model_b.B(pools)

    def mk(v):
        if design == "A":
            c, w = part_from_int(pools, v.m)
            return (v.d << 6) | c, w
        return m.make(v.p, v.n)
    made = {o: [(mk(a), mk(b)) for a, b in pairs[o]] for o in OPS}
    res = [None] * len(made[op])
    for _ in range(passes):
        for j, (x, y) in enumerate(made[op]):
            if res[j] is not None and op != "cmp":
                m.release(res[j])
            res[j] = (m.neg(x) if op == "neg" else getattr(m, op)(x, y))
    return m, pools, res


def replay_growth(design, x0, yz, passes=2):
    pools = Pools()
    m = model_a.A(pools) if design == "A" else model_b.B(pools)
    def mk(v):
        if design == "A":
            c, w = part_from_int(pools, v.m)
            return (v.d << 6) | c, w
        return m.make(v.p, v.n)
    X0 = [mk(v) for v in x0]
    YZ = []
    for y, z in yz:
        YZ.append((mk(y), mk(z)))
    steps = len(yz) // len(x0)
    res = [None] * len(x0)
    for _ in range(passes):
        for c in range(len(x0)):
            if res[c] is not None:
                m.release(res[c])
            x = X0[c]
            for s in range(steps):
                y, z = YZ[c * steps + s]
                t = m.mul(x, y)
                if s > 0:
                    m.release(x)
                x = m.add(t, z)
                m.release(t)
            res[c] = x
    return m, pools, res


def slot_hex(pools, c, w):
    mem, off = pools.ptr(c, w)
    return ",".join(f"{x:016x}" for x in mem[off:off + (1 << c)])


def line_a(pools, r):
    h, w = r
    s = f"{h:02x} {w:016x}"
    return s + (" " + slot_hex(pools, h & 31, w) if h & 31 else "")


def line_b(pools, r):
    h, p, n = r
    s = f"{h:04x} {p:016x} {n:016x}"
    for c, w in ((h & 31, p), ((h >> 8) & 31, n)):
        s += " " + (slot_hex(pools, c, w) if c else "-")
    return s


def value_of_line(build, line):
    """(value, problem) from a dump line, with the canonical checks that need no model"""
    t = line.split()
    if build == "control":
        h, n = int(t[0], 16), int(t[1])
        ws = [] if t[2] == "-" else [int(x, 16) for x in t[2].split(",")]
        m = sum(x << (64 * i) for i, x in enumerate(ws))
        prob = None
        if h & 0x3F:
            prob = "class or spare bits set in Control's header"
        elif len(ws) != n or (n and ws[-1] == 0):
            prob = "block length not the real length"
        return (h >> 6, m), prob
    if build in ("a", "abf"):
        h, w = int(t[0], 16), int(t[1], 16)
        c = h & 31
        if c == 0:
            m = w
            n = 1 if w else 0
        else:
            ws = [int(x, 16) for x in t[2].split(",")]
            m = sum(x << (64 * i) for i, x in enumerate(ws))
            n = len(words_of(m))
        prob = "spare bit set" if h & 0x20 else (None if cls(n) == c else f"class {c} holds {n} words")
        return (h >> 6, m), prob
    h = int(t[0], 16)
    parts = []
    prob = "spare bits set" if h & 0xE0E0 else None
    for c, s, w in ((h & 31, t[3], int(t[1], 16)), ((h >> 8) & 31, t[4], int(t[2], 16))):
        if c == 0:
            parts.append(w)
            n = 1 if w else 0
        else:
            ws = [int(x, 16) for x in s.split(",")]
            parts.append(sum(x << (64 * i) for i, x in enumerate(ws)))
            n = len(words_of(parts[-1]))
        if cls(n) != c:
            prob = prob or f"part class {c} holds {n} words"
    return (None, parts[0] - parts[1]), prob


def words_of(x):
    return r2gen.words(x)


def unit(d, m):
    """m * i^d as a Gaussian integer (re, im), exactly"""
    return [(m, 0), (0, m), (-m, 0), (0, -m)][d % 4]


def int_of(dv):
    """(direction, magnitude), or (None, value) from B, to (re, im)"""
    d, m = dv
    return (m, 0) if d is None else unit(d, m)


def ref(op, a, b):
    """the exact result as (re, im), or compare's int; in four directions for the axes job"""
    va, vb = unit(a.d, a.m), unit(b.d, b.m)
    if op == "add":
        return (va[0] + vb[0], va[1] + vb[1])
    if op == "sub":
        return (va[0] - vb[0], va[1] - vb[1])
    if op == "neg":
        return (-va[0], -va[1])
    if op == "mul":
        return (va[0] * vb[0] - va[1] * vb[1], va[0] * vb[1] + va[1] * vb[0])
    if op == "div":
        assert b.m and a.m % b.m == 0
        return unit(a.d - b.d, a.m // b.m)
    if op == "gcd":
        return (gcd(a.m, b.m), 0)
    sa = a.m if a.d in (0, 1) else -a.m              # compare along the shared axis, 1 and i positive
    sb = b.m if b.d in (0, 1) else -b.m
    return (sa > sb) - (sa < sb)


# ------------------------------------------------------------------------------------------------ running a binary
def run(build, mode, job, op, out=None):
    args = [str(BIN / build), mode, str(job), op] + ([str(out)] if out else [])
    return subprocess.run(args, capture_output=True, text=True)


def check_dump(build, job, pairs, op, tmp, models, stats):
    """one build x job x operation against the replayed model and Python"""
    if not pairs[op]:
        return
    out = tmp / f"{build}-{job.stem}-{op}.txt"
    p = run(build, "dump", job, op, out)
    key = f"{job.stem}|{op}"
    st = stats.setdefault(build, {})
    cell = st.setdefault(key, {"n": 0, "fail": 0, "first": None})
    if p.returncode != 0:
        cell["fail"] += 1
        cell["first"] = f"exit {p.returncode}: {p.stderr.strip()}"
        return
    lines = out.read_text().splitlines()
    design = "B" if build == "b" else "A"
    mk = (job.stem, op, design)
    if mk not in models:
        models[mk] = replay(design, pairs, op)
    _, pools, res = models[mk]
    if lines[-1] != "pools ok" and not (build == "control" and lines[-1] == "pools -"):
        cell["fail"] += 1
        cell["first"] = cell["first"] or lines[-1]
    for j, ((a, b), line, r) in enumerate(zip(pairs[op], lines, res)):
        cell["n"] += 1
        want = ref(op, a, b)
        prob = None
        if op == "cmp":
            if int(line) != want or int(line) != r:
                prob = f"compare {line}, model {r}, exact {want}"
        else:
            dv, prob = value_of_line(build, line)
            got = int_of(dv)
            if got != want:
                prob = prob or f"value {got!r}, exact {want!r}"
            if build in ("a", "abf"):
                ml = line_a(pools, r)
                if line != ml:
                    prob = prob or f"representation {line[:80]} != model {ml[:80]}"
            elif build == "b":
                ml = line_b(pools, r)
                if line != ml:
                    prob = prob or f"representation {line[:80]} != model {ml[:80]}"
            else:
                if dv[0] != r[0] >> 6:
                    prob = prob or f"direction {dv[0]} != model {r[0] >> 6}"
        if prob:
            cell["fail"] += 1
            if cell["first"] is None:
                cell["first"] = f"pair {j} ({a}, {b}): {prob}"


def write_check_jobs(d):
    d.mkdir(parents=True, exist_ok=True)
    jobs = {"edge": edge_job(), "random": rand_job(), "axes": axes_job()}
    paths = {}
    for name, pairs in jobs.items():
        r2gen.write_job(d / f"{name}.bin", 0, pairs)
        paths[name] = d / f"{name}.bin"
    return jobs, paths


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true", help="skip the timing jobs' checksums")
    a = ap.parse_args()
    t0 = time.monotonic()
    d = HERE / "build" / "r2" / "check"
    jobs, paths = write_check_jobs(d)
    failed = False
    stats, models = {}, {}
    # 1. check jobs against the models
    for name, pairs in jobs.items():
        for b in (AXES_BUILDS if name == "axes" else BUILDS):
            for op in OPS:
                check_dump(b, paths[name], pairs, op, d, models, stats)
        print(f"{time.monotonic() - t0:6.1f}s  {name}: " + ", ".join(
            f"{b} {sum(c['n'] for k, c in stats[b].items() if k.startswith(name + '|'))} results "
            f"{sum(c['fail'] for k, c in stats[b].items() if k.startswith(name + '|'))} failures"
            for b in (AXES_BUILDS if name == "axes" else BUILDS)), flush=True)
    for b, st in stats.items():
        for k, c in st.items():
            if c["fail"]:
                failed = True
                print(f"FAIL {b} {k}: {c['fail']} of {c['n']}; first: {c['first']}")
    # 2. errors
    ne = 0
    for name, op, x, y, builds in error_cases():
        pairs = {o: [] for o in OPS}
        pairs[op] = [(x, y)]
        f = d / f"err-{name}.bin"
        r2gen.write_job(f, 0, pairs)
        for b in builds:
            p = run(b, "dump", f, op, d / "err.txt")
            ne += 1
            if p.returncode != 3 or "spec error" not in p.stderr:
                failed = True
                print(f"FAIL error {name} {b}: exit {p.returncode} {p.stderr.strip()[:100]}")
    print(f"{time.monotonic() - t0:6.1f}s  errors: {ne} cases, each stopped with the spec error"
          if not failed else "errors checked")
    # 2b. B's second part when the first part's move-down grows its pool (fresh pools; exact values only: round 1's
    # model B keeps the second part's pointer across that growth and gives a wrong value here, README)
    x = Val((1 << 256) - 16 - ((1 << 128) - 1), 0, (1 << 256) - 16, (1 << 128) - 1)   # P 4 words (pool 2's one slot)
    y = Val((1 << 128) - 2, 2, 1, (1 << 128) - 1)                                     # P 1 word, N 2 words
    for op, yy in (("add", y), ("sub", Val(y.m, 2 - y.d, y.n, y.p))):
        pairs = {o: [] for o in OPS}
        pairs[op] = [(x, yy)]
        f = d / f"growpart-{op}.bin"
        r2gen.write_job(f, 0, pairs)
        out = d / f"growpart-{op}.txt"
        p = run("b", "dump", f, op, out)
        lines = out.read_text().splitlines() if p.returncode == 0 else [f"exit {p.returncode}", ""]
        dv, prob = value_of_line("b", lines[0])
        want = ref(op, x, yy)
        _, pools, res = replay("B", pairs, op, passes=1)
        mv = model_b.B(pools).value(res[0])
        if int_of(dv) != want or prob or lines[-1] != "pools ok":
            failed = True
            print(f"FAIL B part pointer {op}: {int_of(dv)} != {want} {prob or ''} {lines[-1]}")
        else:
            print(f"{time.monotonic() - t0:6.1f}s  B {op}, second part across its pool's growth: exact"
                  + ("" if mv == want[0] else f" (round 1's model B gives {mv - want[0]:+#x} off here)"))
    # 3. growth: models and checksums; timing jobs: checksums
    work = HERE / "build" / "r2" / "work"
    tjobs, (gpath, x0, yz) = r2gen.generate(work)
    gwant = checksum("growth", r2gen.growth_exact(x0, yz))
    gexact = r2gen.growth_exact(x0, yz)
    for b in BUILDS:
        p = run(b, "sum", gpath, "growth")
        got = p.stdout.split()[-1] if p.returncode == 0 and p.stdout else f"exit {p.returncode}"
        if got != gwant:
            failed = True
            print(f"FAIL growth sum {b}: {got} != {gwant}")
        out = d / f"growth-{b}.txt"
        p = run(b, "dump", gpath, "growth", out)
        lines = out.read_text().splitlines() if p.returncode == 0 else [f"exit {p.returncode}"]
        design = "B" if b == "b" else "A"
        if ("growth", design) not in models:
            models[("growth", design)] = replay_growth(design, x0, yz)
        _, pools, res = models[("growth", design)]
        bad = 0
        for c, (line, r) in enumerate(zip(lines, res)):
            dv, prob = value_of_line(b, line)
            if int_of(dv) != (gexact[c], 0):
                prob = prob or "value"
            if b in ("a", "abf") and line != line_a(pools, r):
                prob = prob or "representation"
            if b == "b" and line != line_b(pools, r):
                prob = prob or "representation"
            if b == "control" and dv[0] != r[0] >> 6:
                prob = prob or "direction"
            if prob:
                bad += 1
                if bad == 1:
                    print(f"FAIL growth {b} chain {c}: {prob}")
        if lines[-1] not in ("pools ok", "pools -"):
            bad += 1
            print(f"FAIL growth {b}: {lines[-1]}")
        failed |= bool(bad)
    print(f"{time.monotonic() - t0:6.1f}s  growth: {len(x0)} chains x {len(yz) // len(x0)} steps, sums and models")
    if not a.quick:
        n = bad = 0
        for name, (path, pairs) in tjobs.items():
            for op in OPS:
                want = checksum(op, [exact(op, x.v, y.v) for x, y in pairs[op]])
                for b in BUILDS:
                    p = run(b, "sum", path, op)
                    got = p.stdout.split()[-1] if p.returncode == 0 and p.stdout else f"exit {p.returncode}"
                    n += 1
                    if got != want:
                        bad += 1
                        print(f"FAIL sum {name} {op} {b}: {got} != {want}")
        failed |= bool(bad)
        print(f"{time.monotonic() - t0:6.1f}s  timing jobs: {n} checksums ({len(tjobs)} jobs x {len(OPS)} operations "
              f"x {len(BUILDS)} builds), {bad} differ from Python's")
    print("PASS" if not failed else "FAILED", f"in {time.monotonic() - t0:.0f} s")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
