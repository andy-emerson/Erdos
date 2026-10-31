"""The simulator of spec v0.4 §17 for the Nim build's checks (README.md): a Python prediction, written from the spec, of
every value, mark, count and Packed structure of one job, independent of the models.

- **Arithmetic and overflow** (§17.0, "its overflow rule"): the archived Control spec's cases (10-interning v0.3
  §17.3, on v0.2 §10.1-10.2's pseudocode), with 08's checks at every intermediate step, in the order the pseudocode
  writes them: a multiplication or addition whose result lies outside [-2^63 + 1, 2^63 - 1] overflows (-2^63
  included). Each operation's record is its case and the gcds, gcd work, exact divisions, multiplications and
  additions made, up to the step that overflowed.
- **Marks** (§17.10): addition tests both operand cells (left first) and passes a mark on; multiplication runs term by
  term, from the first product (§17.6), and stops at the first marked operand (passed on) or overflow (new).
- **Packed's structures** (§17.2-17.5): the bound 2rc, T_fix, the hash and linear probing, (l) in arrival order, (h)
  as positions, the widest value, the narrowed list width, the build and packed handle widths, and per store found,
  new, probes and the longest store.
- **Accounting** (§17.8, §17.11): the accounted running total of each model through the driver's call order, event
  by event, and so each memrun figure the memory build prints.
"""
from fractions import Fraction

LIM = 2 ** 63 - 1
PHI = 0x9E3779B97F4A7C15
M64 = 2 ** 64 - 1
MARK = (0, 0)

# r2.h's case numbers (r2_case_name's order)
CASE_NAMES = ["add_zero", "add_cross", "add_den", "add_num", "add_general", "mul_one_x", "mul_one_y", "mul_both",
              "mul_den", "mul_num", "mul_cross_ad", "mul_cross_cb", "mul_general", "div_one_x", "div_one_y",
              "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den", "div_general", "add_int_both",
              "add_int_x", "add_int_y", "mul_int_both", "mul_int_x", "mul_int_y", "add_zero_x", "add_zero_y",
              "mul_zero", "div_zero"]
CASE = {k: i for i, k in enumerate(CASE_NAMES)}


class Overflow(Exception):
    pass


class Op:
    """one arithmetic operation's counts"""
    __slots__ = ("case", "gcd", "work", "div", "mul", "add", "cmp", "match")

    def __init__(self):
        self.case, self.gcd, self.work, self.div, self.mul, self.add, self.cmp, self.match = 0, 0, 0, 0, 0, 0, 0, 0

    def record(self):
        return (self.case, self.gcd, self.work, self.div, self.mul, self.add)


def _gcd(p, q):
    p, q = abs(p), abs(q)
    while q:
        p, q = q, p % q
    return p


def gcd(k, p, q):
    k.gcd += 1
    k.work += max(abs(p), abs(q)).bit_length()
    return _gcd(p, q)


def div(k, p, q):
    k.div += 1
    assert p % q == 0
    return p // q


def mul(k, p, q):
    k.mul += 1
    r = p * q
    if r > LIM or r < -LIM:
        raise Overflow
    return r


def add(k, p, q):
    k.add += 1
    r = p + q
    if r > LIM or r < -LIM:
        raise Overflow
    return r


def sgn(neg, v):
    return -v if neg else v


def r_add(k, x, y):
    """x + y, x and y (a, b, neg); returns (n, e)"""
    a, b, sx = x
    c, d, sy = y
    k.cmp += 8
    k.match += (b == d) + (a == c) + (a == d) + (c == b)
    if b == d and a == c and sx != sy:
        k.case = CASE["add_zero"]
        return 0, 1
    if a == 0:
        k.case = CASE["add_zero_x"]
        return sgn(sy, c), d
    if c == 0:
        k.case = CASE["add_zero_y"]
        return sgn(sx, a), b
    if b == 1 and d == 1:
        k.case = CASE["add_int_both"]
        return add(k, sgn(sx, a), sgn(sy, c)), 1
    if b == 1:
        k.case = CASE["add_int_x"]
        return add(k, mul(k, sgn(sx, a), d), sgn(sy, c)), d
    if d == 1:
        k.case = CASE["add_int_y"]
        return add(k, sgn(sx, a), mul(k, sgn(sy, c), b)), b
    if a == d or c == b:
        k.case = CASE["add_cross"]
        t = add(k, mul(k, sgn(sx, a), d), mul(k, sgn(sy, c), b))
        return t, mul(k, b, d)
    if b == d:
        k.case = CASE["add_den"]
        t = add(k, sgn(sx, a), sgn(sy, c))
        h = gcd(k, t, d)
        if h == 1:
            return t, d
        return div(k, t, h), div(k, d, h)
    if a == c:
        k.case = CASE["add_num"]
        g = gcd(k, b, d)
        if g == 1:
            t = add(k, sgn(sx, d), sgn(sy, b))
            return mul(k, a, t), mul(k, b, d)
        b1, d1 = div(k, b, g), div(k, d, g)
        t = add(k, sgn(sx, d1), sgn(sy, b1))
        h = gcd(k, t, g)
        if h == 1:
            return mul(k, a, t), mul(k, b1, d)
        t1, d2 = div(k, t, h), div(k, d, h)       # both divisions first (tc3.c)
        return mul(k, a, t1), mul(k, b1, d2)
    k.case = CASE["add_general"]
    g = gcd(k, b, d)
    if g == 1:
        t = add(k, mul(k, sgn(sx, a), d), mul(k, sgn(sy, c), b))
        return t, mul(k, b, d)
    b1, d1 = div(k, b, g), div(k, d, g)
    t = add(k, mul(k, sgn(sx, a), d1), mul(k, sgn(sy, c), b1))
    h = gcd(k, t, g)
    if h == 1:
        return t, mul(k, b1, d)
    n = div(k, t, h)
    return n, mul(k, b1, div(k, d, h))


def r_mul(k, x, y):
    a, b, sx = x
    c, d, sy = y
    s = sx != sy
    k.cmp += 10
    k.match += (b == d) + (a == c) + (a == d) + (c == b)
    if a == 0 or c == 0:
        k.case = CASE["mul_zero"]
        return 0, 1
    if a == b:
        k.case = CASE["mul_one_x"]
        return sgn(s, c), d
    if c == d:
        k.case = CASE["mul_one_y"]
        return sgn(s, a), b
    if b == 1 and d == 1:
        k.case = CASE["mul_int_both"]
        return sgn(s, mul(k, a, c)), 1
    if a == d and c == b:
        k.case = CASE["mul_both"]
        return sgn(s, 1), 1
    if b == d:
        k.case = CASE["mul_den"]
        n = sgn(s, mul(k, a, c))
        return n, mul(k, d, d)
    if a == c:
        k.case = CASE["mul_num"]
        n = sgn(s, mul(k, a, a))
        return n, mul(k, b, d)
    if a == d:
        k.case = CASE["mul_cross_ad"]
        h = gcd(k, c, b)
        if h == 1:
            return sgn(s, c), b
        return sgn(s, div(k, c, h)), div(k, b, h)
    if c == b:
        k.case = CASE["mul_cross_cb"]
        h = gcd(k, a, d)
        if h == 1:
            return sgn(s, a), d
        return sgn(s, div(k, a, h)), div(k, d, h)
    if b == 1:
        k.case = CASE["mul_int_x"]
        h = gcd(k, a, d)
        a1, d1 = (a, d) if h == 1 else (div(k, a, h), div(k, d, h))
        return sgn(s, mul(k, a1, c)), d1
    if d == 1:
        k.case = CASE["mul_int_y"]
        h = gcd(k, c, b)
        c1, b1 = (c, b) if h == 1 else (div(k, c, h), div(k, b, h))
        return sgn(s, mul(k, a, c1)), b1
    k.case = CASE["mul_general"]
    g1 = gcd(k, a, d)
    g2 = gcd(k, c, b)
    a1, d1 = (a, d) if g1 == 1 else (div(k, a, g1), div(k, d, g1))
    c1, b1 = (c, b) if g2 == 1 else (div(k, c, g2), div(k, b, g2))
    n = sgn(s, mul(k, a1, c1))
    return n, mul(k, b1, d1)


def operand(p):
    return abs(p[0]), p[1], p[0] < 0


class Counts:
    """one matrix's counters as the driver prints them: per phase, 08's core counts, cmp and cases; Packed's cross
    tests; the marks"""

    def __init__(self):
        self.ctr = {}        # (phase, field) -> value
        self.cross = [0, 0, 0]
        self.marks = [0, 0, 0]   # new, passed, tests

    def put(self, phase, k, record):
        for f, v in (("gcd", k.gcd), ("gcd_work", k.work), ("div_exact", k.div), ("mul", k.mul), ("add", k.add)):
            self.ctr[(phase, "ctr", f)] = self.ctr.get((phase, "ctr", f), 0) + v
        if record:
            self.ctr[(phase, "xctr", "cmp")] = self.ctr.get((phase, "xctr", "cmp"), 0) + k.cmp
            c = "case_" + CASE_NAMES[k.case]
            self.ctr[(phase, "xctr", c)] = self.ctr.get((phase, "xctr", c), 0) + 1

    def lines(self, name):
        """the driver's ctr/xctr lines (every nonzero counter), as a set"""
        return {f"{kind} {name} {ph} core {f} {v}" for (ph, kind, f), v in self.ctr.items() if v}


def enter(cells):
    """an input's entry: each pair reduced gcd-first; (values, Counts)"""
    cnt, out = Counts(), []
    for m, d in cells:
        k = Op()
        if d < 0:
            m, d = -m, -d
        g = gcd(k, m, d)
        out.append((m, d) if g == 1 else (div(k, m, g), div(k, d, g)))
        cnt.put("entry", k, False)
    return out, cnt


class Records(list):
    """the per-operation record, and the index of the first operation that overflowed (None: none did)"""
    first = None


def first_overflow(records):
    if records.first is None:
        records.first = len(records) - 1


def madd(X, Y, records):
    cnt, out = Counts(), []
    for x, y in zip(X, Y):
        cnt.marks[2] += 2
        if x[1] == 0 or y[1] == 0:
            cnt.marks[1] += 1
            out.append(MARK)
            continue
        k = Op()
        try:
            r = r_add(k, operand(x), operand(y))
        except Overflow:
            r = None
        cnt.put("add", k, True)
        records.append(k.record())
        cnt.cross[0] += 4
        cnt.cross[1] += k.match
        cnt.cross[2] += (1 if x[0] in (x[1], -x[1]) else 2) + (1 if y[0] in (y[1], -y[1]) else 2)
        if r is None:
            cnt.marks[0] += 1
            first_overflow(records)
            out.append(MARK)
        else:
            out.append(r)
    return out, cnt


def mmul(X, Y, n, records):
    cnt, out = Counts(), []
    for i in range(n):
        for j in range(n):
            s = None
            for t in range(n):
                x, y = X[i * n + t], Y[t * n + j]
                cnt.marks[2] += 2
                if x[1] == 0 or y[1] == 0:
                    cnt.marks[1] += 1
                    s = MARK
                    break
                k = Op()
                try:
                    p = r_mul(k, operand(x), operand(y))
                except Overflow:
                    p = None
                cnt.put("mul", k, True)
                records.append(k.record())
                cnt.cross[0] += 4
                cnt.cross[1] += k.match
                cnt.cross[2] += (1 if x[0] in (x[1], -x[1]) else 2) + (1 if y[0] in (y[1], -y[1]) else 2)
                if p is None:
                    cnt.marks[0] += 1
                    first_overflow(records)
                    s = MARK
                    break
                if t == 0:
                    s = p
                    continue
                k = Op()
                try:
                    s = r_add(k, operand(s), operand(p))
                except Overflow:
                    s = None
                cnt.put("add", k, True)
                records.append(k.record())
                if s is None:
                    cnt.marks[0] += 1
                    first_overflow(records)
                    s = MARK
                    break
            out.append(s)
    return out, cnt


# ----------------------------------------------------------------------------------------- Packed's store

def build_width(tbits):
    return 8 if tbits <= 7 else 16 if tbits <= 15 else 32


def pack_width(W):
    return 8 if W <= 128 else 16 if W <= 32768 else 32


def list_width(bits):
    return 8 if bits <= 8 else 16 if bits <= 16 else 32 if bits <= 32 else 64


def store(cells):
    """Packed's build of a matrix whose cells (in order of entry) hold `cells`: (l), (h), the fields and figures"""
    rc = len(cells)
    tbits = (2 * rc - 1).bit_length()
    T = 1 << tbits
    whb = build_width(tbits)
    h = [-1] * T
    lst, fields = [], []
    found = new = probes = longest = 0
    widest = 1
    for m, d in cells:
        tag = 1 if m < 0 else 0
        for v, t in ((abs(m), tag), (d, 0)):
            s = ((v * PHI) & M64) >> (64 - tbits)
            p = 1
            while True:
                if h[s] == -1:
                    h[s] = len(lst)
                    lst.append(v)
                    widest = max(widest, v.bit_length() + 1)
                    k = h[s]
                    new += 1
                    break
                if lst[h[s]] == v:
                    k = h[s]
                    found += 1
                    break
                s = (s + 1) & (T - 1)
                p += 1
            probes += p
            longest = max(longest, p)
            fields.append(2 * k + t)
    W = len(lst)
    w, whp = list_width(widest), pack_width(W)
    return {"T": T, "tbits": tbits, "whb": whb, "whp": whp, "W": W, "w": w, "list": lst, "fields": fields,
            "h": [lst[k] if k >= 0 else -1 for k in h], "found": found, "new": new, "probes": probes,
            "longest": longest, "rc": rc}


def pack_line(name, st):
    rc = st["rc"]
    return (f"pack {name} W={st['W']} T={st['T']} w={st['w']} whb={st['whb']} whp={st['whp']} "
            f"rewritten={2 * rc if st['whp'] < st['whb'] else 0} bytes_list={st['W'] * st['w'] // 8} bytes_map=0 "
            f"bytes_table={st['T'] * st['whb'] // 8} bytes_cells_build={2 * rc * st['whb'] // 8} "
            f"bytes_cells_pack={2 * rc * st['whp'] // 8}")


def stor_line(when, name, st):
    rc = st["rc"]
    return (f"stor {when} {name} W={st['W']} T={st['T']} w={st['w']} wh={st['whp']} whb={st['whb']} packed=1 "
            f"nf={2 * rc} found={st['found']} new={st['new']} probes={st['probes']} longest={st['longest']} "
            f"widenings=0 copied=0 growths=0 reinserted=0 rprobes=0 rewritten=0 bcells={2 * rc * st['whp'] // 8} "
            f"bvals={st['W'] * st['w'] // 8}")


def packed_bytes(st):
    return 2 * st["rc"] * st["whp"] // 8 + st["W"] * st["w"] // 8


def build_bytes(st):
    """a matrix right after its create: cells at the build width, (h), and (l) at 2rc x 64 bits"""
    return 2 * st["rc"] * st["whb"] // 8 + st["T"] * st["whb"] // 8 + 16 * st["rc"]


# ----------------------------------------------------------------------------------------- one job

STEPS = {"add": (("add", "A", "B", "C"),), "mul": (("mul", "A", "B", "C"),),
         "sop": (("mul", "A", "B", "P"), ("mul", "C", "D", "Q"), ("add", "P", "Q", "R")),
         "pos": (("add", "A", "B", "S"), ("add", "C", "D", "U"), ("mul", "S", "U", "R"))}
INPUTS = {"add": "AB", "mul": "AB", "sop": "ABCD", "pos": "ABCD"}


def job(op, n, inputs):
    """every matrix's values, counts and (Packed) structures, the per-operation record, and the accounted figures"""
    V, K, ST, records = {}, {}, {}, Records()
    for name, cells in zip(INPUTS[op], inputs):
        V[name], K[name] = enter(cells)
    for kind, x, y, z in STEPS[op]:
        if kind == "add":
            V[z], K[z] = madd(V[x], V[y], records)
        else:
            V[z], K[z] = mmul(V[x], V[y], n, records)
    for name in V:
        ST[name] = store(V[name])
    return {"values": V, "counts": K, "stores": ST, "records": records, "first_overflow": records.first,
            "memory": memory(op, n, ST)}


def memory(op, n, ST):
    """the memory build's figures (memrun lines), by the accounting events of §17.11 (Packed) and the cells of every
    matrix alive (Control), through the driver's call order: each input's create and build; each step's result
    created just before its step, built (mul: the row buffer, n decoded cells of 24 bytes, allocated and freed inside
    it), packed; the intermediates freed after the last step"""
    q = n * n
    out = {"packed": {}, "control": {}}
    for model in ("packed", "control"):
        live, figs = 0, {}
        for name in INPUTS[op]:
            base = live
            if model == "packed":
                peak = live + build_bytes(ST[name])
                live += packed_bytes(ST[name])
            else:
                live += 16 * q
                peak = live
            figs[f"built_{name}"] = {"acct": live - base, "peak_acct": peak, "base_acct": base}
        figs["after_creation"] = {"acct": live}
        peak = live
        for kind, x, y, z in STEPS[op]:
            if model == "packed":
                live += build_bytes(ST[z])
                peak = max(peak, live)
                if kind == "mul":
                    peak = max(peak, live + 24 * n)
                live += packed_bytes(ST[z]) - build_bytes(ST[z])
            else:
                live += 16 * q
                peak = max(peak, live)
        if op in ("sop", "pos"):
            for x in STEPS[op][2][1:3]:
                live -= packed_bytes(ST[x]) if model == "packed" else 16 * q
        figs["op"] = {"peak_acct": peak}
        figs["end"] = {"acct": live}
        out[model] = figs
    return out


def exact(op, n, inputs):
    """the exact values by Python's fractions, independent of the arithmetic above"""
    F = {name: [Fraction(m, d) for m, d in X] for name, X in zip(INPUTS[op], inputs)}
    for kind, x, y, z in STEPS[op]:
        if kind == "add":
            F[z] = [a + b for a, b in zip(F[x], F[y])]
        else:
            F[z] = [sum((F[x][i * n + k] * F[y][k * n + j] for k in range(n)), Fraction(0))
                    for i in range(n) for j in range(n)]
    return {k: [(v.numerator, v.denominator) for v in X] for k, X in F.items()}
