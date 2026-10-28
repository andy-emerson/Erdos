"""workload.py: random workloads with an exact oracle, trial jobs for the C driver, and outcome classification.

The workload generator is lib/tests/diff.py's (copied, not imported, so lib/ stays untouched), without its text,
audit and statistics commands, and followed by a readout of every live value and array, so that a corrupted value
that the workload never printed still meets the oracle.  The oracle is Python's fractions.Fraction.
"""
import math
import random
from fractions import Fraction as F

LO, HI, DMAX = -2**63, 2**63 - 1, 2**63 - 1
SMALLP = [2, 3, 5, 7, 11, 13]
LARGEP = [65537, 1000003, 1000033, 2147483647, 2147483693, 2147484679, 4294967311, 1099511627791,
          2305843009213693951, 9223372036854775783]
LENGTHS = [1, 6, 23, 64, 64, 300]

CLASSES = [1, 2, 3, 4, 5, 6, 7, 8, 9]
CLASS_NAMES = {1: "F1 drop list entry", 2: "F2 stray list entry", 3: "F3 wrong node in factorization",
               4: "F4 wrong exponent in factorization (O2: the shared record)", 5: "F5 wrong list position",
               6: "F6 wrong reference count", 7: "F7 wrong dictionary entry", 8: "F8 interrupted refinement",
               9: "F9 single bit flip"}
INDEX_CLASSES = [1, 2, 5, 6, 7]   # faults in the derived index: a cross-check can see them
TRUTH_CLASSES = [3, 4, 8]         # corruptions of the truth itself (F8: a half-applied refinement)
HW_CLASSES = [9]
OUTCOMES = ["detected_repaired", "detected_not_repaired", "silent_wrong", "crash", "benign", "spurious_error"]


def undetected(outcome, sub):
    """silent wrong, a crash without a diagnostic, or benign with the graph left inconsistent"""
    return outcome in ("silent_wrong", "crash") or (outcome == "benign" and sub == "latent inconsistency left")


def detected(outcome):
    return outcome in ("detected_repaired", "detected_not_repaired")


def repair_induced(outcome, sub):
    return outcome == "silent_wrong" and sub == "after a repair"


def fits(x):
    return LO <= x.numerator <= HI and x.denominator <= DMAX


def lcm_all(xs):
    out = 1
    for x in xs:
        out = out * x // math.gcd(out, x)
    return out


class Gen:
    def __init__(self, r):
        self.r = r

    def den(self):
        r = self.r
        k = r.random()
        if k < 0.005:
            return 0
        if k < 0.25:
            d = r.randint(1, 12)
        elif k < 0.40:
            d = 10 ** r.randint(0, 6)
        elif k < 0.65:
            d = 1
            for p in SMALLP:
                if r.random() < 0.45:
                    d *= p ** r.randint(1, 3)
        elif k < 0.80:
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
        if r.random() < 0.02:
            d = LO
        return d

    def num(self, d):
        r = self.r
        k = r.random()
        if k < 0.25:
            return r.randint(-100, 100)
        if k < 0.40:
            return r.randint(-10**6, 10**6)
        if k < 0.65:
            f = 1
            for p in SMALLP + LARGEP[:4]:
                if d and abs(d) % p == 0 and r.random() < 0.6:
                    f *= p
            return max(LO, min(HI, f * r.randint(-1000, 1000)))
        if k < 0.85:
            return r.randint(-2**62, 2**62)
        return r.choice([LO, HI, LO + 1, 2**62, -2**62, 0, 1, -1])

    def frac(self):
        d = self.den()
        return self.num(d), d


def exp_line(want):
    """diff.py's expectation tuple -> the driver's expectation line."""
    k = want[0]
    if k == "E":
        return f"X E {want[1]}"
    if k == "=":
        return f"X = {want[1]} {want[2]}"
    if k == "=?":
        return f"A = {want[1]} {want[2]}|E 1"
    if k == "any":
        return "I"
    if k == "ok":
        return "X ok"
    if k == "c":
        return f"X c {want[1]}"
    if k == "s":
        return f"X s {want[1]} {want[2]}"
    if k == "a":
        return "P a "  # an array was made; its layout is representation, not a result
    if k == "x":
        return "X x" + "".join(f" {x.numerator} {x.denominator}" for x in want[1])
    raise ValueError(k)


class Workload:
    """A random program of single values and arrays (entries, arithmetic, releases, refinements, array expressions,
    sums and dot products) with the exact expected output of every command, then a readout of every live result."""

    def __init__(self, seed, length):
        self.r = random.Random(seed)
        self.g = Gen(self.r)
        self.cmds, self.exps = [], []
        self.vals, self.arrs, self.depth = {}, {}, 0
        for _ in range(length):
            self.step()
        while self.depth:
            self.depth -= 1
            self.emit("EE", ("ok",))
        self.workend = len(self.cmds)
        for v in sorted(self.vals):
            x = self.vals[v]
            self.emit(f"P {v}", ("=", x.numerator, x.denominator))
        for a in sorted(self.arrs):
            self.emit(f"AX {a}", ("x", self.arrs[a]))

    def emit(self, cmd, want):
        self.cmds.append(cmd)
        self.exps.append(exp_line(want))

    def want_value(self, dst, x, err=None):
        if err is None and not fits(x):
            err = 1
        if err is not None:
            return ("E", err)
        self.vals[dst] = x
        return ("=", x.numerator, x.denominator)

    def value_cmd(self, cmd, dst, x, err=None):
        self.emit(cmd, self.want_value(dst, x, err))

    def array_result(self, dst, elems, err=None):
        if err is not None:
            return ("E", err)
        self.arrs[dst] = elems
        return ("a", None, 0)

    def vslot(self):
        return self.r.choice(sorted(self.vals)) if self.vals else None

    def step(self):
        r = self.r
        k = r.random()
        if k < 0.18 or not self.vals:
            dst = r.randrange(64)
            n, d = self.g.frac()
            self.value_cmd(f"V {dst} {n} {d}", dst, None if d == 0 else F(n, d), 2 if d == 0 else None)
        elif k < 0.21:
            dst = r.randrange(64)
            n = self.g.num(1)
            self.value_cmd(f"I {dst} {n}", dst, F(n))
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
            x, y = self.vals[a], self.vals[b]
            if r.random() < 0.5:
                self.emit(f"Q {a} {b}", ("c", (x > y) - (x < y)))
            else:
                self.emit(f"G {a}", ("s", (x > 0) - (x < 0), int(x == 0)))
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
        else:
            if self.depth < 2 and r.random() < 0.6:
                self.depth += 1
                self.emit("EB", ("ok",))
            elif self.depth == 0:
                self.emit("EE", ("E", 3))
            else:
                self.depth -= 1
                self.emit("EE", ("ok",))

    def array_from_fracs(self):
        r = self.r
        dst = r.randrange(16)
        n = r.choice(LENGTHS)
        mode = r.random()
        if mode < 0.12:
            fr = []
            while len(fr) < n:
                d = r.choice([p for p in LARGEP if p < 2**41] + [1000003 * 7, 2**40 + 15, 3**25])
                a_ = r.randint(1, 2**40)
                fr += [(a_, d), (-a_, d)] if len(fr) + 1 < n else [(r.randint(-9, 9), r.randint(1, 12))]
            r.shuffle(fr)
        elif mode < 0.35:
            d = 10 ** r.randint(1, 4)
            fr = [(r.randint(-10**7, 10**7), d) for _ in range(n)]
        elif mode < 0.6:
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
        self.emit(cmd, self.array_result(dst, xs))

    def array_from_values(self):
        r = self.r
        if not self.vals:
            return
        dst = r.randrange(16)
        n = r.choice(LENGTHS)
        slots = [r.choice(sorted(self.vals)) for _ in range(n)]
        self.emit(f"AV {dst} {n} " + " ".join(map(str, slots)), self.array_result(dst, [self.vals[s] for s in slots]))

    def pick_arrays(self, k):
        by_len = {}
        for s, xs in self.arrs.items():
            by_len.setdefault(len(xs), []).append(s)
        if not by_len:
            return None
        grp = self.r.choice(list(by_len.values()))
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

    def reduction_cmd(self, cmd, dst, terms):
        x = sum(terms, F(0))
        groups = {}
        for t in terms:
            groups[t.denominator] = groups.get(t.denominator, F(0)) + t
        L = lcm_all([d for d, v in groups.items() if v != 0])
        if fits(x) and L.bit_length() > 16384:
            self.emit(cmd, ("=?", x.numerator, x.denominator))
            self.vals.pop(dst, None)
            self.emit(f"R {dst}", ("any",))
            return
        self.value_cmd(cmd, dst, x)

    def array_reduce(self):
        r = self.r
        dst = r.randrange(64)
        if r.random() < 0.5:
            ss = self.pick_arrays(1)
            if ss:
                self.reduction_cmd(f"AS {dst} {ss[0]}", dst, list(self.arrs[ss[0]]))
        else:
            ss = self.pick_arrays(2)
            if ss:
                a, b = self.arrs[ss[0]], self.arrs[ss[1]]
                self.reduction_cmd(f"AD {dst} {ss[0]} {ss[1]}", dst, [x * y for x, y in zip(a, b)])

    def array_read(self):
        r = self.r
        if not self.arrs:
            return
        a = r.choice(sorted(self.arrs))
        xs = self.arrs[a]
        c = r.random()
        if c < 0.4:
            i = r.randrange(len(xs))
            self.emit(f"AG {a} {i}", ("=", xs[i].numerator, xs[i].denominator))
        elif c < 0.7:
            i = r.randrange(len(xs))
            dst = r.randrange(64)
            self.value_cmd(f"AGV {dst} {a} {i}", dst, xs[i])
        else:
            self.emit(f"AX {a}", ("x", xs))


def job_text(w, K, trials):
    """The driver's input: trials are (id, cls, moment, seed, phase)."""
    out = [f"K {K}", f"WORKEND {w.workend}", f"CMDS {len(w.cmds)}"] + w.cmds + [f"EXP {len(w.exps)}"] + w.exps
    out.append(f"TRIALS {len(trials)}")
    out += [f"{i} {c} {m} {s} {p}" for i, c, m, s, p in trials]
    return "\n".join(out) + "\n"


def parse_record(line):
    """A driver 'T' line -> dict."""
    f = line.rstrip("\n").split("\t")
    d = {}
    for kv in f[1:]:
        k, _, v = kv.partition("=")
        d[k] = v
    for k in ("id", "cls", "exit", "injected", "inj_cmd", "halt", "nverify", "nrepair", "repair_cmd", "mismatch_cmd",
              "mismatch_kind", "completed", "latent", "done", "nosplit", "in_check", "nroll"):
        if k in d and d[k] not in ("",):
            d[k] = int(d[k])
    d["sig"] = int(d["sig"]) if d.get("sig") else 0
    return d


SIGABRT, SIGSEGV, SIGALRM = 6, 11, 14
EXIT_HALT = 42


def classify(d):
    """(outcome, subtype) for a trial record; outcome None if the fault was not injected or does not apply."""
    if d.get("nosplit"):
        return None, "no refinement in the workload"
    if d.get("injected") == -1:
        return None, "not applicable"
    if d.get("injected") != 1:
        if d.get("sig") or d.get("exit") not in (0, None):
            return "crash", "before injection"
        return None, "no target"
    if d["mismatch_cmd"] >= 0:
        if d["mismatch_kind"] == 2:
            return "spurious_error", "an error status where a result was expected"
        sub = "after a repair" if d["nrepair"] > 0 else "no check fired first" if d["nverify"] == 0 else "checks passed"
        return "silent_wrong", sub
    if d["sig"]:
        if d.get("in_check") == 1 and d["halt"] == 0:
            return "crash", "inside a check" + (" (hang)" if d["sig"] == SIGALRM else f" (signal {d['sig']})")
        if d["sig"] == SIGABRT and d["halt"] == 2:
            return "detected_not_repaired", "internal diagnostic: " + d.get("halt_msg", "")[:60]
        if d["sig"] == SIGALRM:
            return "crash", "hang (timeout)"
        if d["sig"] == SIGABRT:
            return "crash", "abort without a diagnostic"
        return "crash", f"signal {d['sig']}"
    if d["exit"] == EXIT_HALT and d["halt"] == 1:
        return "detected_not_repaired", "production check"
    if d["exit"] != 0 or not d["completed"]:
        return "crash", f"exit {d['exit']}"
    if d["nrepair"] > 0:
        return "detected_repaired", "latent inconsistency left" if d["latent"] else "consistent"
    return "benign", "latent inconsistency left" if d["latent"] else "no trace"
