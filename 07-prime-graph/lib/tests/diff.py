#!/usr/bin/env python3
"""Randomized differential test of primegraph against Python's fractions.

    python3 tests/diff.py --bin build/diff [--rounds N] [--seed S] [--audit] [--fuzz SECONDS] [--keep FILE]

Each round generates a random script (tests/diff.c documents the commands), runs it through the C driver and checks
every output line against an exact model kept with fractions.Fraction:

  - every value and every array element, and every error code (overflow is expected exactly when the exact result
    does not fit: numerator in [-2^63, 2^63 - 1], denominator below 2^63; pg_array_fma also when a*b does not fit);
  - the layout chosen at entry (D by the provisional default rule, the exception count, I4's lowering of D) for
    arrays made from fractions and from values, and the deferred flag inside expressions;
  - releases and churn: when the script releases everything, the graph must hold no node, holder or edge;
  - with --audit the driver runs pg_audit after every command (the debug build also audits inside the library).

--fuzz SECONDS runs rounds with fresh seeds until the time is up (the long-running fuzz option); a failing script is
written to --keep (default build/diff-fail.txt) for replay with:  build/diff < build/diff-fail.txt
"""
import argparse
import math
import os
import random
import subprocess
import sys
import time
from fractions import Fraction as F

LO, HI, DMAX = -2**63, 2**63 - 1, 2**63 - 1
SMALLP = [2, 3, 5, 7, 11, 13]
LARGEP = [65537, 1000003, 1000033, 2147483647, 2147483693, 2147484679, 4294967311, 1099511627791,
          2305843009213693951, 9223372036854775783]  # primes (checked in check_primes)
LENGTHS = [1, 6, 23, 64, 64, 300]


def check_primes():
    for p in LARGEP:
        if p < 2 or any(pow(a, p - 1, p) != 1 for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)):
            sys.exit(f"diff.py: {p} is not prime")


def fits(x):
    return LO <= x.numerator <= HI and x.denominator <= DMAX


def lcm_all(xs):
    out = 1
    for x in xs:
        out = out * x // math.gcd(out, x)
    return out


class Gen:
    def __init__(self, seed):
        self.r = random.Random(seed)

    def den(self):
        r = self.r
        k = r.random()
        if k < 0.005:
            return 0
        if k < 0.25:
            d = r.randint(1, 12)
        elif k < 0.40:
            d = 10 ** r.randint(0, 6)
        elif k < 0.65:  # products of shared small primes: multi-atom nodes, ratio-class splits
            d = 1
            for p in SMALLP:
                if r.random() < 0.45:
                    d *= p ** r.randint(1, 3)
        elif k < 0.80:  # large primes, two-large-prime products (rho), times a small part
            ps = r.sample(LARGEP, r.randint(1, 2))
            d = 1
            for p in ps:
                if d * p <= DMAX:
                    d *= p
            m = r.choice([1, 1, 2, 6, 10, 30])
            if d * m <= DMAX:
                d *= m
        elif k < 0.92:
            d = r.randint(1, 2 ** r.randint(1, 63) - 1)
        else:
            d = r.choice([DMAX, 2**62, 3**39, 2**31 * 3**19, 1000003**3, 999999999989 * 3])
        if r.random() < 0.1:
            d = -d
        return d

    def num(self, d):
        r = self.r
        k = r.random()
        if k < 0.25:
            return r.randint(-100, 100)
        if k < 0.40:
            return r.randint(-10**6, 10**6)
        if k < 0.65:  # shares part of the denominator: partial cancellation (the g < N case)
            f = 1
            for p in SMALLP + LARGEP[:4]:
                if d and abs(d) % p == 0 and r.random() < 0.6:
                    f *= p
            n = f * r.randint(-1000, 1000)
            return max(LO, min(HI, n))
        if k < 0.85:
            return r.randint(-2**62, 2**62)
        return r.choice([LO, HI, LO + 1, 2**62, -2**62, 0, 1, -1])

    def frac(self):
        d = self.den()
        return self.num(d), d


class Program:
    """A script and, per command, the check of its output line."""

    def __init__(self, seed, length):
        self.g = Gen(seed)
        self.r = self.g.r
        self.cmds, self.checks = [], []
        self.vals = {}     # slot -> Fraction
        self.arrs = {}     # slot -> (list of Fraction, deferred at creation)
        self.depth = 0
        for _ in range(length):
            self.step()
        self.finish()

    def emit(self, cmd, check):
        self.cmds.append(cmd)
        self.checks.append(check)

    # ------------------------------------------------------------------ expected lines
    def want_value(self, dst, x, err=None):
        """x: the exact result (or None with err set).  Sets the model slot on success."""
        if err is None and not fits(x):
            err = 1
        if err is not None:
            return ("E", err)
        self.vals[dst] = x
        return ("=", x.numerator, x.denominator)

    def value_cmd(self, cmd, dst, x, err=None):
        self.emit(cmd, self.want_value(dst, x, err))

    def array_result(self, dst, elems, err=None, layout=None):
        """An array made by entry (layout given: never deferred) or by an operation (deferred inside an expression)."""
        if err is not None:
            return ("E", err)
        self.arrs[dst] = elems
        return ("a", layout, 1 if self.depth > 0 and layout is None else 0)

    # ------------------------------------------------------------------ commands
    def slot(self, pool, n):
        return self.r.choice(sorted(pool)) if pool and self.r.random() < 0.9 else self.r.randrange(n)

    def vslot(self):
        return self.r.choice(sorted(self.vals)) if self.vals else None

    def step(self):
        r = self.r
        k = r.random()
        if k < 0.16 or not self.vals:
            dst = r.randrange(64)
            n, d = self.g.frac()
            self.value_cmd(f"V {dst} {n} {d}", dst, None if d == 0 else F(n, d), 2 if d == 0 else None)
        elif k < 0.19:
            dst = r.randrange(64)
            n = self.g.num(1)
            self.value_cmd(f"I {dst} {n}", dst, F(n))
        elif k < 0.23:
            dst = r.randrange(64)
            self.text_cmd(dst)
        elif k < 0.45:
            a, b = self.vslot(), self.vslot()
            op = r.choice("+-*/")
            dst = r.randrange(64)
            x, y = self.vals[a], self.vals[b]
            if op == "/" and y == 0:
                self.value_cmd(f"B / {dst} {a} {b}", dst, None, 2)
            else:
                z = x + y if op == "+" else x - y if op == "-" else x * y if op == "*" else x / y
                self.value_cmd(f"B {op} {dst} {a} {b}", dst, z)
        elif k < 0.48:
            a, dst = self.vslot(), r.randrange(64)
            if r.random() < 0.5:
                self.value_cmd(f"N {dst} {a}", dst, -self.vals[a])
            else:
                self.value_cmd(f"C {dst} {a}", dst, self.vals[a])
        elif k < 0.53:
            a = self.vslot()
            self.emit(f"R {a}", ("ok",))
            del self.vals[a]
        elif k < 0.58:
            a, b = self.vslot(), self.vslot()
            c = r.random()
            x, y = self.vals[a], self.vals[b]
            if c < 0.4:
                self.emit(f"Q {a} {b}", ("c", (x > y) - (x < y)))
            elif c < 0.7:
                self.emit(f"G {a}", ("s", (x > 0) - (x < 0), int(x == 0)))
            else:
                t = str(x.numerator) if x.denominator == 1 else f"{x.numerator}/{x.denominator}"
                self.emit(f"T {a}", ("t", t))
        elif k < 0.68:
            self.array_from_fracs()
        elif k < 0.71:
            self.array_from_values()
        elif k < 0.82:
            self.array_op()
        elif k < 0.86:
            self.array_reduce()
        elif k < 0.92:
            self.array_read()
        elif k < 0.95 and self.arrs:
            a = r.choice(sorted(self.arrs))
            self.emit(f"AR {a}", ("ok",))
            del self.arrs[a]
        elif k < 0.98:
            if self.depth < 2 and r.random() < 0.6:
                self.depth += 1
                self.emit("EB", ("ok",))
            elif self.depth == 0:
                self.emit("EE", ("E", 3))
            else:
                self.expr_end()
        else:
            self.emit("AU", ("ok",))

    def expr_end(self):
        self.depth -= 1
        self.emit("EE", ("ok",))

    def text_cmd(self, dst):
        r = self.r
        c = r.random()
        if c < 0.6:
            n, d = self.g.frac()
            s = f"{n}/{d}" if r.random() < 0.8 else f"{n}"
            if r.random() < 0.15:  # beyond 64 bits, reduced by gcd first
                m = r.choice([2, 3, 2**10, 10**6])
                n, d = n * m, d * m
                s = f"{n}/{d}"
            x = None
            if "/" in s:
                nn, dd = s.split("/")
                if int(dd) == 0:
                    self.value_cmd(f"S {dst} {s}", dst, None, 2)
                    return
                if len(nn.lstrip("+-")) > 38 or len(dd.lstrip("+-")) > 38:
                    self.value_cmd(f"S {dst} {s}", dst, None, 3)
                    return
                x = F(int(nn), int(dd))
            else:
                x = F(int(s))
            self.value_cmd(f"S {dst} {s}", dst, x)
        else:
            s = r.choice(["1/", "/3", "x", "1/2/3", "--4", "1.5", "+", "0x1", "1/-+2", "12a"])
            self.value_cmd(f"S {dst} {s}", dst, None, 3)

    # ------------------------------------------------------------------ arrays
    def array_from_fracs(self):
        r = self.r
        dst = r.randrange(16)
        n = r.choice(LENGTHS)
        mode = r.random()
        if mode < 0.35:  # one scale (decimals)
            d = 10 ** r.randint(1, 4)
            fr = [(r.randint(-10**7, 10**7), d) for _ in range(n)]
        elif mode < 0.6:  # mostly one denominator, some others
            base = self.g.den() or 7
            fr = [(self.g.num(base), base) if r.random() < 0.8 else self.g.frac() for _ in range(n)]
        else:
            fr = [self.g.frac() for _ in range(n)]
        fr = [(a, b if b != 0 or r.random() < 0.3 else 1) for a, b in fr]
        cmd = f"AF {dst} {n} " + " ".join(f"{a} {b}" for a, b in fr)
        if any(b == 0 for _, b in fr):
            self.emit(cmd, ("E", 2))
            return
        xs = [F(a, b) for a, b in fr]
        if not all(fits(x) for x in xs):
            self.emit(cmd, ("E", 1))
            return
        # the layout: D by the default rule over |den| as given, dense where it divides and fits, then I4
        mags = [abs(b) for _, b in fr]
        L = lcm_all(mags)
        if L < 2**63:
            D0 = L
        else:
            cnt, best, bc = {}, 1, 0
            for m in mags:
                cnt[m] = cnt.get(m, 0) + 1
            for m in dict.fromkeys(mags):  # first appearance order, strictly greater wins
                if cnt[m] > bc:
                    bc, best = cnt[m], m
            D0 = best
        dense = []
        for a, b in fr:
            t = -a if b < 0 else a
            s = D0 // abs(b) if D0 % abs(b) == 0 else 0
            if s and LO <= t <= HI and LO <= t * s <= HI:
                dense.append(t * s)
        D = D0 // math.gcd(D0, *dense) if dense else 1
        self.emit(cmd, self.array_result(dst, xs, layout=(D, n - len(dense))))

    def array_from_values(self):
        r = self.r
        if not self.vals:
            return
        dst = r.randrange(16)
        n = r.choice(LENGTHS)
        slots = [r.choice(sorted(self.vals)) for _ in range(n)]
        xs = [self.vals[s] for s in slots]
        dens = [x.denominator for x in xs]
        L = lcm_all(dens)
        if L < 2**63:
            D0 = L
        else:
            cnt, best, bc = {}, 1, 0
            for d in dens:  # the first to reach a strictly greater count wins
                cnt[d] = cnt.get(d, 0) + 1
                if cnt[d] > bc:
                    bc, best = cnt[d], d
            D0 = best
        dense = []
        for x in xs:
            if D0 % x.denominator == 0 and LO <= x.numerator * (D0 // x.denominator) <= HI:
                dense.append(x.numerator * (D0 // x.denominator))
        D = D0 // math.gcd(D0, *dense) if dense else 1
        cmd = f"AV {dst} {n} " + " ".join(map(str, slots))
        self.emit(cmd, self.array_result(dst, xs, layout=(D, n - len(dense))))

    def pick_arrays(self, k):
        by_len = {}
        for s, xs in self.arrs.items():
            by_len.setdefault(len(xs), []).append(s)
        groups = [v for v in by_len.values()]
        if not groups:
            return None
        grp = self.r.choice(groups)
        return [self.r.choice(grp) for _ in range(k)]

    def array_op(self):
        r = self.r
        op = r.choice("+-*/f")
        ss = self.pick_arrays(3 if op == "f" else 2)
        if not ss:
            return self.array_from_fracs()
        dst = r.randrange(16)
        A = [self.arrs[s] for s in ss]
        cmd = f"AO {op} {dst} " + " ".join(map(str, ss))
        out, err = [], None
        for i in range(len(A[0])):
            x, y = A[0][i], A[1][i]
            if op == "/" and y == 0:
                err = 2
                break
            if op == "f":
                t = x * y
                if not fits(t):
                    err = 1
                    break
                z = t + A[2][i]
            else:
                z = x + y if op == "+" else x - y if op == "-" else x * y if op == "*" else x / y
            if not fits(z):
                err = 1
                break
            out.append(z)
        self.emit(cmd, self.array_result(dst, out, err=err))

    def array_reduce(self):
        r = self.r
        dst = r.randrange(64)
        if r.random() < 0.5:
            ss = self.pick_arrays(1)
            if not ss:
                return
            xs = self.arrs[ss[0]]
            self.value_cmd(f"AS {dst} {ss[0]}", dst, sum(xs, F(0)))
        else:
            ss = self.pick_arrays(2)
            if not ss:
                return
            a, b = self.arrs[ss[0]], self.arrs[ss[1]]
            self.value_cmd(f"AD {dst} {ss[0]} {ss[1]}", dst, sum((x * y for x, y in zip(a, b)), F(0)))

    def array_read(self):
        r = self.r
        if not self.arrs:
            return
        a = r.choice(sorted(self.arrs))
        xs = self.arrs[a]
        c = r.random()
        if c < 0.4:
            i = r.randrange(len(xs) + (1 if r.random() < 0.1 else 0))
            if i >= len(xs):
                self.emit(f"AG {a} {i}", ("E", 3))
            else:
                self.emit(f"AG {a} {i}", ("=", xs[i].numerator, xs[i].denominator))
        elif c < 0.7:
            i = r.randrange(len(xs))
            dst = r.randrange(64)
            self.value_cmd(f"AGV {dst} {a} {i}", dst, xs[i])
        else:
            self.emit(f"AX {a}", ("x", xs))

    def finish(self):
        while self.depth:
            self.expr_end()
        self.emit("AU", ("ok",))
        for a in sorted(self.arrs):
            self.emit(f"AR {a}", ("ok",))
        for v in sorted(self.vals):
            self.emit(f"R {v}", ("ok",))
        self.arrs.clear()
        self.vals.clear()
        self.emit("ST", ("st0",))
        self.emit("AU", ("ok",))
        self.emit("END", None)


def check_line(want, got):
    """None if the output line matches the expectation, else a message."""
    f = got.split()
    kind = want[0]
    if kind == "E":
        return None if f == ["E", str(want[1])] else f"want error {want[1]}"
    if kind == "=":
        return None if f == ["=", str(want[1]), str(want[2])] else f"want = {want[1]} {want[2]}"
    if kind == "ok":
        return None if f == ["ok"] else "want ok"
    if kind == "c":
        return None if f == ["c", str(want[1])] else f"want c {want[1]}"
    if kind == "s":
        return None if f == ["s", str(want[1]), str(want[2])] else f"want s {want[1]} {want[2]}"
    if kind == "t":
        return None if f == ["t", want[1]] else f"want t {want[1]}"
    if kind == "a":
        if len(f) != 4 or f[0] != "a":
            return "want an array"
        layout, deferred = want[1], want[2]
        if int(f[3]) != deferred:
            return f"want deferred {deferred}"
        if layout is not None and (int(f[1]), int(f[2])) != layout:
            return f"want layout D={layout[0]} exceptions={layout[1]}"
        return None
    if kind == "x":
        xs = want[1]
        exp = ["x"] + [str(v) for x in xs for v in (x.numerator, x.denominator)]
        return None if f == exp else "want elements " + " ".join(exp[1:])[:300]
    if kind == "st0":
        if len(f) < 6 or f[0] != "st" or any(int(v) != 0 for v in f[1:6]):
            return "want an empty graph (nodes holders values arrays edges = 0)"
        return None
    return f"unknown expectation {kind}"


def run_round(binary, seed, length, audit, keep):
    p = Program(seed, length)
    script = "\n".join(p.cmds) + "\n"
    args = [binary] + (["--audit"] if audit else [])
    env = dict(os.environ)
    res = subprocess.run(args, input=script, capture_output=True, text=True, env=env)
    lines = res.stdout.splitlines()
    err = None
    stats = None
    for i, (cmd, want) in enumerate(zip(p.cmds, p.checks)):
        if want is None:
            break
        if i >= len(lines):
            err = f"command {i + 1} ({cmd[:120]}): no output (exit {res.returncode}): {res.stderr.strip()[-500:]}"
            break
        if lines[i].startswith("AUDIT"):
            err = f"command {i + 1} ({cmd[:120]}): {lines[i]}"
            break
        m = check_line(want, lines[i])
        if m:
            err = f"command {i + 1} ({cmd[:200]}): got '{lines[i][:300]}', {m}"
            break
        if want[0] == "st0":
            stats = lines[i]
    if err is None and res.returncode != 0:
        err = f"exit status {res.returncode}: {res.stderr.strip()[-1000:]}"
    if err:
        os.makedirs(os.path.dirname(keep) or ".", exist_ok=True)
        with open(keep, "w") as fh:
            fh.write(script)
    kinds = {}
    for w in p.checks:
        if w:
            key = w[0] + (str(w[1]) if w[0] == "E" else "")
            kinds[key] = kinds.get(key, 0) + 1
    return err, len(p.cmds), stats, kinds


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--bin", default="build/diff")
    ap.add_argument("--rounds", type=int, default=40)
    ap.add_argument("--length", type=int, default=400, help="commands per round")
    ap.add_argument("--seed", type=int, default=20261028)
    ap.add_argument("--audit", action="store_true", help="pg_audit after every command")
    ap.add_argument("--fuzz", type=float, default=0, help="run fresh seeds for this many seconds")
    ap.add_argument("--keep", default="build/diff-fail.txt")
    a = ap.parse_args()
    check_primes()
    t0 = time.time()
    n = cmds = 0
    cov = [0] * 5  # splits at entry, splits in reductions, restarts, V4 retries, rho calls
    kinds = {}
    seed = a.seed if not a.fuzz else int(time.time() * 1000) % 2**32
    while True:
        err, k, st, ks = run_round(a.bin, seed, a.length if n % 10 else 4 * a.length, a.audit, a.keep)
        n += 1
        cmds += k
        if st:
            cov = [c + int(v) for c, v in zip(cov, st.split()[6:11])]
        for key, v in ks.items():
            kinds[key] = kinds.get(key, 0) + v
        if err:
            print(f"diff: FAIL (seed {seed}, round {n}): {err}\n  script kept in {a.keep}")
            return 1
        seed += 1
        if a.fuzz:
            if time.time() - t0 > a.fuzz:
                break
        elif n >= a.rounds:
            break
    print(f"diff: {n} rounds, {cmds} commands, all match Python's fractions"
          f"{' (audited after every command)' if a.audit else ''} [{time.time() - t0:.1f} s]")
    print(f"  coverage: {cov[0]} entry splits, {cov[1]} reduction splits, {cov[2]} restarts, {cov[3]} V4 retries, "
          f"{cov[4]} rho calls; expected errors: {kinds.get('E1', 0)} overflow, {kinds.get('E2', 0)} divzero, "
          f"{kinds.get('E3', 0)} invalid; {kinds.get('a', 0)} arrays")
    return 0


if __name__ == "__main__":
    sys.exit(main())
