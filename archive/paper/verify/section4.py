"""Checks Appendix B.2 of paper/v0.6.md, the chapter 4 part of the extension library (the matrices), as verify.py
checks Appendix A and section3.py checks B.1.

The appendix is in Nim, and nimlib.py checks it line for line and runs it on every call recorded here. The checks here
run on the Python reference: this harness's translation of the pseudocode of sections 4.2-4.6 (the substitutions of
verify.py and section3.py, with `SET A[i] = v` as an assignment, `DO f(x)` as a call on its own line, `RETURN` alone
as `return`, and ERROR raising ValueError), on top of the references of Appendix A and B.1; the arrays and matrices of
section 4.2, defined in prose, have fixed short forms. Then:

- every matrix built, added or multiplied is compared, cell by cell, with a plain reference matrix of
  fractions.Fraction, and every cell must be the stored value (lowest terms, positive denominator) that chapter 2's add
  and multiply give;
- every stored matrix must keep section 4.8's promise: its list holds exactly the different integers of its cells, each
  once, each used, with no unused entry; every handle is a position below W; its width is bits(W - 1) and holds every
  handle; equal handles mean equal integers;
- every build table is recorded as pack receives it: the smallest power of two from 4rc on in slots, at most half full,
  its handles slots below its size, its contents those of an independent linear-probing table with section 4.4's
  multiplicative home, and the list its used slots in slot order;
- put is checked against a Python dict on random sequences, with section 4.4's conditions (a) and (b) after every
  call, and the probe's wrap past the last slot;
- the index of section 4.3, pseudocode only, is translated here and checked, with its count of comparisons;
- every example and count of chapter 4 is reproduced;
- every `if` outcome (and `or` term) of B.2 must be reached, and every statement run. The outcome of each `if` is read
  by evaluating its condition when the line is reached, since a one-line `if c: A[i] = v` runs its body on its own line.
"""
import ast
import math
import random
import sys
from collections import Counter
from fractions import Fraction

import section3

FILENAME = "<v0.6.md, chapter 4: the Python reference>"
INDEX_FILENAME = "<v0.6.md, section 4.3 pseudocode>"
HEAD = ("unused = None\n"
        "phi = 0x9E3779B97F4A7C15\n"
        "Matrix = namedtuple('Matrix', 'rows columns width handles values')\n\n\n")
HEAD_LINES = 3
FUNCTIONS = ["array", "size", "matrix", "row_count", "column_count", "handles", "values",
             "cell", "entry", "home", "probe", "put", "table_size", "place", "positions", "copy_values", "rewrite", "bits", "pack",
             "place_all", "build", "add_cells", "add_matrices", "dot", "product_cells", "product_rows",
             "multiply_matrices"]
PSEUDOCODE_ONLY = ["search"]           # section 4.3, the index
# the arrays and matrices of section 4.2, defined in prose: their translation is fixed here
PROSE = {"array": "def array(n):\n    return [unused] * n\n",
         "size": "def size(A):\n    return len(A)\n",
         "matrix": "def matrix(r, c, w, H, L):\n    return Matrix(r, c, w, H, L)\n",
         "row_count": "def row_count(M):\n    return M[0]\n",
         "column_count": "def column_count(M):\n    return M[1]\n",
         "handles": "def handles(M):\n    return M[3]\n",
         "values": "def values(M):\n    return M[4]\n"}
PHI = 0x9E3779B97F4A7C15


# ------------------------------------------------------------------------------------------------------- the code
def load(path, ns_b, V):
    """The Python reference for sections 4.2-4.6, on top of Appendix A's and B.1's: (code, tree, line map, namespace)."""
    text, base = V.text_from(path, "\n### 4.2 ")
    numbered = V.parse_numbered(text, base)
    code, where = V.build_reference(FUNCTIONS, PROSE, numbered, lambda name, lines: translation(lines, V), HEAD)
    tree = ast.parse(code)
    ns = dict(ns_b)                      # Appendix A's and B.1's functions, used as they are
    exec(compile(code, FILENAME, "exec"), ns)
    return code, tree, where, ns


def chapter4_pseudocode(path, V):
    """{name: lines} of the pseudocode of sections 4.2-4.8 (the matrices; section 4.1's rows are section3's)."""
    text = path.read_text(encoding="utf-8").split("\n## Appendix A")[0]
    return V.parse_pseudocode(text.split("\n### 4.2 ")[1])


def translation(lines, V):
    return section3.errors(V.translate(lines))


# ------------------------------------------------------------------------------------------------------- tracing
class Tracer:
    """Traces B.2's frames: lines run, the outcome of every `if` (its condition evaluated as the line is reached) and of
    every `or` term; and, for each call of pack, the build table and the handles as pack receives them."""

    def __init__(self, br):
        self.br = br
        self.ran, self.outcomes, self.terms = Counter(), Counter(), Counter()
        self.compiled = {ln: compile(text, FILENAME, "eval") for ln, (_, text, _, _) in br.ifs.items()}
        self.packs = []

    def __enter__(self):
        sys.settrace(self.global_trace)
        return self

    def __exit__(self, *a):
        sys.settrace(None)

    def global_trace(self, frame, event, arg):
        if event != "call" or frame.f_code.co_filename != FILENAME:
            return None
        if frame.f_code.co_name == "pack":
            loc = frame.f_locals
            rec = {"r": loc["r"], "c": loc["c"], "T": list(loc["T"]), "H": list(loc["H"]), "H_obj": loc["H"], "result": None}
            self.packs.append(rec)
        else:
            rec = None

        def local(frame, event, arg):
            if event == "line":
                ln = frame.f_lineno
                self.ran[ln] += 1
                if ln in self.compiled:
                    env = dict(frame.f_globals)
                    env.update(frame.f_locals)
                    self.outcomes[(ln, bool(eval(self.compiled[ln], env)))] += 1
                    for i, (co, _) in enumerate(self.br.ifs[ln][3]):
                        if eval(co, env):
                            self.terms[(ln, i)] += 1
            elif event == "return" and rec is not None:
                rec["result"] = arg
                rec["H_after"] = list(frame.f_locals["H"])
            return local

        return local


def coverage(br, tr, where, V, check):
    missing = []
    print("\nBranch coverage of the Python reference for chapter 4, by pseudocode line of the paper (each `if`: times "
          "true / times false)")
    for ln in sorted(br.ifs):
        fn, text, _, terms = br.ifs[ln]
        at = V.where_text(where, ln)
        t, f = tr.outcomes[(ln, True)], tr.outcomes[(ln, False)]
        print(f"  {fn:18s} line {at}  if {text:68s} true {t:7d}  false {f:7d}")
        if not t:
            missing.append(f"{fn} line {at}: `if {text}` never true")
        if not f:
            missing.append(f"{fn} line {at}: `if {text}` never false")
        for i, (_, src) in enumerate(terms):
            n = tr.terms[(ln, i)]
            print(f"  {'':18s}            or-term {src:60s} true {n:7d}")
            if not n:
                missing.append(f"{fn} line {at}: or-term `{src}` never true")
    unrun = sorted(ln for ln in br.lines if not tr.ran[ln])
    for ln in unrun:
        missing.append(f"line {V.where_text(where, ln)} ({br.lines[ln]}) never ran")
    print(f"  statements run: {len(br.lines) - len(unrun)} of {len(br.lines)}")
    for m in missing:
        check(False, "chapter 4 coverage: " + m)


# ------------------------------------------------------------------------------------------------- references
def ref_width(m):
    return next(w for w in (8, 16, 32, 64) if m < 2 ** w)


def ref_home(v, n):
    """Section 4.4's multiplicative hash, independently: the top t of the 64 binary digits of |v| * phi mod 2^64, for
    n = 2^t slots."""
    t = n.bit_length() - 1
    assert n == 1 << t and t <= 64
    return ((abs(v) * PHI) % 2 ** 64) >> (64 - t)


def ref_size(rc):
    """The table's size: the smallest power of two from 4rc on."""
    n = 1
    while n < 4 * rc:
        n *= 2
    return n


def ref_table(ints, n, home=ref_home):
    """An independent linear-probing table of n slots: (slots, the slot of each integer put)."""
    T, slots = [None] * n, []
    for v in ints:
        s = home(v, n)
        while T[s] is not None and T[s] != v:
            s = (s + 1) % n
        T[s] = v
        slots.append(s)
    return T, slots


def integers(cells):
    """The integers of a matrix's cells, row by row, each numerator before its denominator."""
    return [u for x in cells for u in x]


def to_list(ns, cells):
    X = ns["empty"]
    for x in reversed(cells):
        X = ns["prepend"](x, X)
    return X


class Checker:
    def __init__(self, ns, tr, V, check):
        self.ns, self.tr, self.V, self.check = ns, tr, V, check
        self.counts = Counter()

    def stored(self, M, r, c, cells, what):
        """M keeps section 4.8's promise, and its cells are `cells` (stored values, row by row), exactly."""
        ns, check = self.ns, self.check
        ok = isinstance(M, ns["Matrix"]) and M[0] == r and M[1] == c
        if not check(ok, f"{what}: not an r x c Matrix: {M[:3] if isinstance(M, tuple) else M}"):
            return False
        H, L, w = ns["handles"](M), ns["values"](M), M[2]
        ints = integers(cells)
        W = len(L)
        ok = (len(H) == 2 * r * c and None not in L and len(set(L)) == W and set(L) == set(ints)
              and all(type(h) is int and 0 <= h < W for h in H) and set(H) == set(range(W)))
        check(ok, f"{what}: the list is not the different integers, each once and used, or a handle is not a position")
        check(w == ref_width(W - 1) and all(h < 2 ** w for h in H), f"{what}: width {w} for W = {W}")
        check(all((H[i] == H[j]) == (L[H[i]] == L[H[j]]) for i in range(len(H)) for j in range(i, min(len(H), i + 9))),
              f"{what}: equal handles do not mean equal integers")
        for k, x in enumerate(cells):
            got = ns["cell"](M, k)
            check(got == x and self.V.is_stored(got), f"{what}: cell {k} = {got}, expected {x}")
            i, j = divmod(k, c)
            check(ns["entry"](M, i, j) == x, f"{what}: entry({i}, {j})")
        # the build table, as pack received it: a power of two from 4rc on, at most half full, an independent table's slots
        rec = next((p for p in reversed(self.tr.packs) if p["result"] is M), None)
        if check(rec is not None, f"{what}: no pack recorded"):
            T, Hb = rec["T"], rec["H"]
            used = [s for s in range(len(T)) if T[s] is not None]
            check(len(T) == ref_size(r * c) and len(T) >= 4 * r * c and 2 * len(used) <= len(T),
                  f"{what}: table of {len(T)} slots, {len(used)} used")
            check(all(0 <= h < len(T) for h in Hb) and ref_width(len(T) - 1) >= w,
                  f"{what}: build handles not slots, or packing widened")
            rt, rslots = ref_table(ints, len(T))
            check(T == rt and Hb == rslots, f"{what}: the build table is not linear probing's")
            check(L == [T[s] for s in used] and len(used) == W, f"{what}: the list is not the used slots in order")
            check(rec["H_after"] == Hb and rec["H_obj"] is not H, f"{what}: pack changed the build handles, or kept them")
        self.counts["stored matrices checked"] += 1
        return True


def plain_add(X, Y):
    return [[a + b for a, b in zip(r, s)] for r, s in zip(X, Y)]


def plain_mul(X, Y):
    m = len(Y)
    return [[sum((X[i][t] * Y[t][j] for t in range(m)), Fraction(0)) for j in range(len(Y[0]))] for i in range(len(X))]


def as_cells(P):
    """A plain matrix of Fractions, as stored values row by row."""
    return [(f.numerator, f.denominator) for row in P for f in row]


def as_plain(cells, r, c):
    return [[Fraction(*cells[i * c + j]) for j in range(c)] for i in range(r)]


# ------------------------------------------------------------------------------------------------------- checks
def check_matrices(ns, ck, V, check):
    rng = random.Random(20261030 + 4)
    build, addm, mulm = ns["build"], ns["add_matrices"], ns["multiply_matrices"]

    def values(n, kind):
        if kind == "pool":                      # few values: much sharing
            pool = [V.reduced(rng.randrange(-6, 7), rng.randrange(1, 7)) for _ in range(rng.randrange(1, 5))]
            return [rng.choice(pool) for _ in range(n)]
        if kind == "small":                     # zero, +-1, both signs, small denominators
            return [V.reduced(rng.randrange(-12, 13), rng.randrange(1, 13)) for _ in range(n)]
        return [V.random_value(rng) for _ in range(n)]

    def make(r, c, kind):
        cells = values(r * c, kind)
        M = build(r, c, to_list(ns, cells))
        ck.stored(M, r, c, cells, f"build({r}, {c}) [{kind}]")
        return M, as_plain(cells, r, c)

    kinds = ("pool", "small", "random")
    # build: every shape up to 6 x 6, and some larger ones with many different integers (16-bit handles)
    for r in range(1, 7):
        for c in range(1, 7):
            for kind in kinds:
                make(r, c, kind)
                ck.counts["build"] += 1
    wide = 0
    for r, c in ((12, 12), (15, 15), (16, 16), (1, 40), (40, 1)):
        M, _ = make(r, c, "random")
        ck.counts["build"] += 1
        wide += M[2] == 16 and len(M[4]) > 256
    check(wide >= 1, "no build reached 16-bit handles")
    # empty dimensions stop with ERROR, both terms of the precondition
    for r, c in ((0, 1), (1, 0), (0, 0), (-1, 3)):
        check(section3.raises(build, r, c, ns["empty"]), f"build({r}, {c}) did not stop with ERROR")
        ck.counts["build (ERROR)"] += 1

    # addition and multiplication, against plain matrices of fractions, and cell for cell against chapter 2
    for _ in range(250):
        r, c, m = rng.randrange(1, 6), rng.randrange(1, 6), rng.randrange(1, 6)
        A, PA = make(r, m, rng.choice(kinds))
        B, PB = make(r, m, rng.choice(kinds))
        S = addm(A, B)
        want = as_cells(plain_add(PA, PB))
        ck.stored(S, r, m, want, "add_matrices")
        check(all(ns["cell"](S, k) == ns["add"](ns["cell"](A, k), ns["cell"](B, k)) for k in range(r * m)),
              "add_matrices: a cell is not chapter 2's add")
        ck.counts["add_matrices"] += 1
        C, PC = make(m, c, rng.choice(kinds))
        P = mulm(A, C)
        ck.stored(P, r, c, as_cells(plain_mul(PA, PC)), "multiply_matrices")
        ck.counts["multiply_matrices"] += 1
        # dimensions that differ stop with ERROR: rows, columns, inner
        D, _ = make(r + 1, m, "small")
        E, _ = make(r, m + 1, "small")
        F, _ = make(m + 1, c, "small")
        check(section3.raises(addm, A, D) and section3.raises(addm, A, E), "add_matrices: dimensions not checked")
        check(section3.raises(mulm, A, F), "multiply_matrices: inner dimensions not checked")
        ck.counts["dimension ERROR"] += 3

    # expressions: every intermediate is a packed matrix, used as an operand
    for _ in range(60):
        n = rng.randrange(1, 5)
        mats = [make(n, n, rng.choice(kinds)) for _ in range(4)]
        (A, PA), (B, PB), (C, PC), (D, PD) = mats
        sop = addm(mulm(A, B), mulm(C, D))
        ck.stored(sop, n, n, as_cells(plain_add(plain_mul(PA, PB), plain_mul(PC, PD))), "A.B + C.D")
        pos = mulm(addm(A, B), addm(C, D))
        ck.stored(pos, n, n, as_cells(plain_mul(plain_add(PA, PB), plain_add(PC, PD))), "(A + B).(C + D)")
        ck.counts["expressions"] += 2


def check_tables(ns, ck, V, check):
    """put, probe, home, table_size and bits, directly: section 4.4's conditions (a) and (b) after every put, against a
    Python dict."""
    rng = random.Random(4404)
    array, put, home, bits = ns["array"], ns["put"], ns["home"], ns["bits"]
    for _ in range(400):
        n = 2 ** rng.randrange(0, 7)                      # the table's size is a power of two (section 4.4)
        T = array(n)
        seen = {}
        for _ in range(rng.randrange(0, n // 2 + 1) * 2):
            if len(set(seen)) * 2 >= n:
                break
            v = rng.choice([rng.randrange(-40, 41), V.random_int(rng)] + list(seen)[:3])
            s = put(T, v)
            check(T[s] == v and (v not in seen or seen[v] == s), f"put({v}) = {s}")
            seen.setdefault(v, s)
            # (a): no integer in two slots; (b): every slot from the home forward to it is used
            used = [x for x in T if x is not None]
            check(len(used) == len(set(used)) == len(seen), "put: an integer in two slots")
            for u, su in seen.items():
                k = home(u, n)
                check(k == ref_home(u, n), f"home({u}, {n})")
                while k != su:
                    check(T[k] is not None, f"put: a gap between the home of {u} and its slot")
                    k = (k + 1) % n
            ck.counts["put"] += 1
    # the wrap past the last slot: 3 and 8 share the home 3, the last of 4 slots
    T = array(4)
    check(home(3, 4) == home(8, 4) == 3 and home(-3, 4) == 3, "4.4: 3 and 8 share the last slot's home in 4 slots")
    check(put(T, 3) == 3 and put(T, 8) == 0 and put(T, 8) == 0 and put(T, -3) == 1 and T == [8, -3, None, 3],
          f"put: the wrap past the last slot: {T}")
    check(home(-5, 16) == home(5, 16) and home(0, 16) == 0, "4.4: v and -v share a home")
    # the multiplicative hash: every n = 2^t up to 2^64, and values far past 64 bits (only the last 64 digits matter)
    for t in range(0, 65):
        for v in [0, 1, -1, 2, 3, 17, 2 ** 64 - 1, 2 ** 64, 2 ** 64 + 1] + [V.random_int(rng) for _ in range(30)]:
            k = home(v, 2 ** t)
            check(k == ref_home(v, 2 ** t) and 0 <= k < 2 ** t and k == home(abs(v) % 2 ** 64, 2 ** t),
                  f"home({v}, 2^{t}) = {k}")
            ck.counts["home"] += 1
    check(PHI % 2 == 1 and PHI == 11400714819323198485 and abs(PHI / 2 ** 64 - 0.618034) < 1e-6,
          "4.4: phi")
    from decimal import Decimal, getcontext
    getcontext().prec = 60
    check(int(Decimal(2 ** 64) / ((1 + Decimal(5).sqrt()) / 2)) == PHI, "4.4: phi is the whole part of 2^64 / golden ratio")
    check([home(v, 16) for v in range(1, 17)] == [9, 3, 13, 7, 1, 11, 5, 15, 8, 2, 12, 6, 0, 10, 4, 14]
          and [v % 16 for v in range(1, 17)] == list(range(1, 16)) + [0]
          and all(home(v, 16) == ((v * PHI) % 2 ** 64) >> 60 for v in range(1, 17)) and hex(PHI)[2] == "9",
          "4.4: the homes of 1 to 16 in 16 slots")
    # table_size: the smallest power of two from m on, less than 2m
    ts = ns["table_size"]
    for m in list(range(1, 3000)) + [40000, 2 ** 40 - 1, 2 ** 40, 2 ** 40 + 1]:
        n = ts(m, 1)
        check(n >= m and n < 2 * m and n & (n - 1) == 0 and (n == 1 or n // 2 < m), f"table_size({m}, 1) = {n}")
        ck.counts["table_size"] += 1
    check(ts(40000, 1) == 65536 and ts(16, 1) == 16, "4.4, 4.7: table sizes")
    for m, w in ((0, 8), (3, 8), (255, 8), (256, 16), (65535, 16), (65536, 32), (2 ** 32 - 1, 32), (2 ** 32, 64),
                 (2 ** 64 - 1, 64)):
        check(bits(m) == w and m < 2 ** w and (w == 8 or m >= 2 ** (w // 2 if w > 8 else 0)), f"bits({m}) = {bits(m)}")
        ck.counts["bits"] += 1
    check(all(bits(m) <= bits(m + 1) for m in range(0, 70000)), "bits gives a smaller width for a larger m")


def check_index(path, ns, V, check):
    """Section 4.3's index, pseudocode only: translated here."""
    pc = chapter4_pseudocode(path, V)
    ins = dict(ns)
    exec(compile(translation(pc["search"], V), INDEX_FILENAME, "exec"), ins)
    search = ins["search"]
    calls = [0]

    def counted(L, n, v, k):
        calls[0] += 1
        return search(L, n, v, k)

    ins["search"] = counted

    def index_build(ints):
        L, n, hs = ns["array"](len(ints)), 0, []
        for v in ints:
            k = counted(L, n, v, 0)
            if k == n:
                L[n] = v
                n += 1
            hs.append(k)
        return L[:n], hs

    L, hs = index_build([1, 2, 1, 3, 2, 3, 17, 2])
    check(L == [1, 2, 3, 17] and hs == [0, 1, 0, 2, 1, 2, 3, 1], f"4.3: the index of A: {L}, {hs}")
    rng = random.Random(4403)
    for _ in range(300):
        ints = [rng.randrange(-20, 21) for _ in range(rng.randrange(0, 40))]
        L, hs = index_build(ints)
        want = list(dict.fromkeys(ints))
        check(L == want and [want[h] for h in hs] == ints, "4.3: the index is not the arrival order")
    for N in range(0, 60):
        calls[0] = 0
        index_build(list(range(N)))
        comparisons = calls[0] - N          # each lookup's last call, k = n, compares nothing
        check(comparisons == N * (N - 1) // 2, f"4.3: {comparisons} comparisons for {N} different integers")
    check(20000 * 19999 // 2 == 199990000, "4.3: nearly 200 million")
    print("Section 4.3's index (pseudocode only, translated here): arrival order, the handles of A, N(N - 1)/2 "
          "comparisons")


def check_first_home(path, ns, V, check):
    """Section 4.4's first hash function, the remainder (pseudocode only; the library's home is the multiplicative one
    after it): translated here, and its example, the table of A with 17 probed from slot 1 to slot 4."""
    text = path.read_text(encoding="utf-8").split("\n### 4.4 ")[1].split("\n#### A multiplicative hash")[0]
    pc = V.parse_pseudocode(text)
    ins = dict(ns)
    exec(compile(translation(pc["home"], V), INDEX_FILENAME, "exec"), ins)
    home1 = ins["home"]
    check(all(home1(v, n) == abs(v) % n for n in range(1, 40) for v in range(-100, 101)), "4.4: the first home")
    check(home1(17, 16) == home1(1, 16) == 1 and home1(-5, 16) == home1(5, 16) == 5,
          "4.4: 1 and 17 share a home in 16 slots, and so do v and -v (the remainder)")
    T, slots = ref_table([1, 2, 1, 3, 2, 3, 17, 2], 16, home1)
    check(T[1:5] == [1, 2, 3, 17] and T.count(None) == 12 and slots == [1, 2, 1, 3, 2, 3, 4, 2],
          f"4.4: A's table with the remainder: {T}, {slots}")
    print("Section 4.4's first home, the remainder (pseudocode only, translated here): |v| mod n, and A's table "
          "(17 probed from slot 1 to slot 4)")


def check_examples(ns, ck, check):
    """The examples and counts of sections 4.2-4.7."""
    build, addm, mulm = ns["build"], ns["add_matrices"], ns["multiply_matrices"]
    A_cells = [(1, 2), (1, 3), (2, 3), (17, 2)]
    B_cells = [(1, 2), (2, 3), (1, 3), (1, 2)]
    check(integers(A_cells) == [1, 2, 1, 3, 2, 3, 17, 2] and len(set(integers(A_cells))) == 4, "4.2: A's integers")
    A = build(2, 2, to_list(ns, A_cells))
    rec = ck.tr.packs[-1]
    T = rec["T"]
    check(len(T) == 16 and [ref_home(v, 16) for v in (1, 2, 3, 17)] == [9, 3, 13, 8]
          and (T[3], T[8], T[9], T[13]) == (2, 17, 1, 3) and T.count(None) == 12
          and rec["H"] == [9, 3, 9, 13, 3, 13, 8, 3], f"4.4: A's table {T}, slots {rec['H']}")
    check(A[4] == [2, 17, 1, 3] and A[3] == [2, 0, 2, 3, 0, 3, 1, 0] and A[2] == 8 and ns["bits"](3) == 8,
          f"4.5: A stored as {A}")
    B = build(2, 2, to_list(ns, B_cells))
    S = addm(A, B)
    rec = ck.tr.packs[-1]
    check([ns["cell"](S, k) for k in range(4)] == [(1, 1), (1, 1), (1, 1), (9, 1)] and S[4] == [9, 1]
          and S[3] == [1, 1, 1, 1, 1, 1, 0, 1] and rec["T"][9] == 1 and rec["T"][8] == 9
          and ref_home(1, 16) == 9 and ref_home(9, 16) == 8, f"4.6: A + B = {S}")
    P = mulm(A, B)
    rec = ck.tr.packs[-1]
    ints = [13, 36, 1, 2, 19, 6, 169, 36]
    check([ns["cell"](P, k) for k in range(4)] == [(13, 36), (1, 2), (19, 6), (169, 36)]
          and integers([ns["cell"](P, k) for k in range(4)]) == ints
          and [ref_home(v, 16) for v in dict.fromkeys(ints)] == [0, 3, 9, 3, 11, 11, 7]
          and rec["H"] == [0, 3, 9, 4, 11, 12, 7, 3] and P[4] == [13, 36, 2, 169, 1, 19, 6], f"4.6: A . B = {P}")
    # dot sums from the left: s = 1/2 . 1/2 = 1/4, then 1/4 + 1/9 = 13/36
    check(ns["multiply"]((1, 2), (1, 2)) == (1, 4) and ns["dot"](A, B, 0, 0, 1, (1, 4)) == (13, 36)
          and ns["dot"](A, B, 0, 0, 2, (1, 4)) == (1, 4), "4.6: the first cell of A . B, summed from the left")
    # 4.7: bytes, with 4-byte integers
    rc = 100 * 100
    check(8 * rc == 80000, "4.7: plain")
    for W, hbits, total, pct in ((200, 8, 20800, 26), (5000, 16, 60000, 75), (20000, 16, 120000, 150)):
        h = hbits // 8
        check(ref_width(W - 1) == hbits and 4 * W + 2 * rc * h == total and round(100 * total / (8 * rc)) == pct,
              f"4.7: the row for W = {W}")
    for r, c in ((1, 1), (3, 5), (100, 100)):
        n = r * c
        for W in range(1, 2 * n + 1):
            check((4 * W + 2 * n * 1 < 8 * n) == (2 * W < 3 * n) and (4 * W + 2 * n * 2 < 8 * n) == (W < n)
                  and not (4 * W + 2 * n * 4 < 8 * n), f"4.7: break-even, W = {W}, rc = {n}")
    n = ref_size(rc)
    check(4 * rc == 40000 and n == 65536 == 2 ** 16 and ref_width(n - 1) == 16 and 4 * n == 262144
          and 2 * rc * 2 == 40000 and n * 2 == 131072 and ref_width(200 - 1) == 8 and 2 * rc * 1 == 20000
          and 262144 + 40000 + 131072 + 4 * 200 + 20000 == 454016 and 5.5 < 454016 / 80000 < 6
          and 4 * 200 + 20000 == 20800, "4.7: the build")
    print("The examples of chapter 4: A's integers, table, slots, list and handles; A + B and A . B; the byte counts "
          "of section 4.7")


def run(path, ns_b, a_filename, V, check):
    sys.setrecursionlimit(max(sys.getrecursionlimit(), 20000))
    code, tree, where, ns = load(path, ns_b, V)
    print(f"\nThe Python reference for chapter 4 (Appendix B.2): {len(code.splitlines())} lines, translated from the "
          f"pseudocode by this harness, defining unused, Matrix, {', '.join(FUNCTIONS)}, on top of Appendix A's and B.1's")
    pc = chapter4_pseudocode(path, V)
    check(sorted(pc) == sorted(set(FUNCTIONS) - set(PROSE) | set(PSEUDOCODE_ONLY)),
          f"sections 4.2-4.8 define {sorted(pc)}")
    print(f"  ({', '.join(PSEUDOCODE_ONLY)}: section 4.3's index, pseudocode only, translated by this harness)")
    api = V.nimlib.RECORDER.api(ns, V.FUNCTIONS + section3.FUNCTIONS + FUNCTIONS)
    br = V.Branches(code, section3.Funcs(tree))
    with Tracer(br) as tr:
        ck = Checker(api, tr, V, check)
        check_matrices(api, ck, V, check)
        check_tables(api, ck, V, check)
        with V.nimlib.traced():
            check_examples(api, ck, check)
    check_index(path, ns, V, check)
    check_first_home(path, ns, V, check)
    print("\nChapter 4 against plain matrices of fractions.Fraction (cells in lowest terms; lists, handles, widths "
          "and build tables as section 4.8 promises):")
    for k in sorted(ck.counts):
        print(f"  {k:36s} {ck.counts[k]:8d}")
    coverage(br, tr, where, V, check)
    return ns, ("B.2", code, FILENAME, HEAD_LINES)
