"""Checks Appendix B.1 of paper/v0.6.md, the chapter 3 part of the extension library in Python, as verify.py checks
Appendix A; and the rows of section 4.1, which are pseudocode only, from a translation made here.

Appendix B's first ```python block (B.1) is read from the paper and run on top of Appendix A's functions, which it uses
and does not repeat. Each function must be the line-for-line translation of its pseudocode in chapter 3 (sections
3.1-3.9) (Appendix A's substitutions, with ERROR raising ZeroDivisionError for a zero divisor, OverflowError for a scale
above 18 and ValueError otherwise); every function is checked against fractions.Fraction, including signs, zero, values
that cannot join, empty columns, sums that reduce to zero, raw entries, decimals at every scale, scale overflow and
quotients that leave the decimals; every `if` outcome (and any `or` term) of B.1 must be reached; and every formula and
example of chapters 1 (the lcm) and 3 is reproduced, with the gcd and reduce calls it claims. The rows of section 4.1 are
not library code (the author's rule: no unused library code), so they are not in Appendix B; this harness translates
their pseudocode itself, with `pair` as a `Row`, and runs the same checks on it as before.
"""
import ast
import math
import random
import re
import sys
from collections import Counter
from fractions import Fraction

HEADING = "## Appendix B. The extension library in Python"          # v0.6, build 4b: B.1 decimals, B.2 matrices
FILENAME = "<v0.6.md, Appendix B.1>"
ROWS_FILENAME = "<v0.6.md, section 4.1 pseudocode>"
FUNCTIONS = ["prepend", "first", "rest", "shared", "entries",
             "column", "join", "take", "tally", "sum_",
             "decimal", "scale", "power", "add_decimals", "subtract_decimals", "multiply_decimals",
             "to_rational", "strip", "places", "to_decimal", "divide_decimals"]
HEAD = ["from collections import namedtuple", "empty = None",
        "Column = namedtuple('Column', 'shared entries')",
        "Decimal = namedtuple('Decimal', 'numerator scale')"]
ROW_FUNCTIONS = ["row", "join", "product", "take"]     # section 4.1, pseudocode only
LIMIT = 18                         # the largest scale, for 64-bit numerators (section 3.4)
# the lists, columns and rows of section 3.1, defined in prose: their translation is fixed (Appendix B's own text)
PROSE = {"prepend": "def prepend(u, L):\n    return (u, L)\n",
         "first": "def first(L):\n    return L[0]\n",
         "rest": "def rest(L):\n    return L[1]\n",
         "shared": "def shared(g):\n    return g[0]\n",
         "entries": "def entries(g):\n    return g[1]\n",
         # the decimals of section 3.4, defined in prose
         "decimal": "def decimal(u, k):\n    return Decimal(u, k)\n",
         "scale": "def scale(x):\n    return x[1]\n"}
PYNAME = {"sum": "sum_"}           # `sum` is a Python built-in


def extract(path, part=0):
    """Appendix B has two ```python blocks, B.1 (part 0) and B.2 (part 1); the one asked for, and its first line."""
    lines = path.read_text(encoding="utf-8").split("\n")
    starts = [i for i, l in enumerate(lines) if l.strip() == HEADING]
    if len(starts) != 1:
        sys.exit(f"{path.name}: expected one heading {HEADING!r}, found {len(starts)}")
    end = next((i for i in range(starts[0] + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
    blocks, i = [], starts[0] + 1
    while i < end:
        if lines[i].strip() == "```python":
            j = next(k for k in range(i + 1, end) if lines[k].strip() == "```")
            blocks.append((i + 2, "\n".join(lines[i + 1:j]) + "\n"))
            i = j
        i += 1
    if len(blocks) != 2:
        sys.exit(f"{path.name}: expected two python blocks in Appendix B (B.1 and B.2), found {len(blocks)}")
    return blocks[part]


def load(path, ns_a):
    first_line, code = extract(path)
    tree = ast.parse(code)
    head = [ast.unparse(n) for n in tree.body[:len(HEAD)]]
    names = [n.name for n in tree.body[len(HEAD):] if isinstance(n, ast.FunctionDef)]
    if head != HEAD or names != FUNCTIONS or len(tree.body) != len(FUNCTIONS) + len(HEAD):
        sys.exit(f"Appendix B.1 must be {HEAD} and then define exactly {FUNCTIONS} in this order; "
                 f"it defines {names}")
    ns = dict(ns_a)                      # Appendix A's functions, used as they are
    exec(compile(code, FILENAME, "exec"), ns)
    return code, tree, first_line, ns


class Funcs:
    """Appendix B's function definitions only, for verify.Branches."""
    def __init__(self, tree):
        self.body = [n for n in tree.body if isinstance(n, ast.FunctionDef)]


class Tracer:
    """Traces Appendix B's frames line by line (arcs and `or` terms, for coverage), and counts calls of Appendix A's
    reduce, and of gcd from outside gcd (one per gcd computed, not per step of Euclid's algorithm)."""

    def __init__(self, br, a_filename):
        self.br, self.a_filename = br, a_filename
        self.arcs, self.terms, self.calls = Counter(), Counter(), Counter()

    def __enter__(self):
        sys.settrace(self.global_trace)
        return self

    def __exit__(self, *a):
        sys.settrace(None)

    def global_trace(self, frame, event, arg):
        if event != "call":
            return None
        fn = frame.f_code.co_filename
        name = frame.f_code.co_name
        if fn == self.a_filename:
            if name == "reduce" and frame.f_back.f_code.co_name != "reduce":
                self.calls["reduce"] += 1
            if name == "gcd" and frame.f_back.f_code.co_name != "gcd":
                self.calls["gcd"] += 1
            return None
        if fn != FILENAME:
            return None
        self.calls[name] += 1
        last = [None]

        def local(frame, event, arg):
            if event == "line":
                ln = frame.f_lineno
                if last[0] is not None:
                    self.arcs[(last[0], ln)] += 1
                self.arcs[(ln, None)] += 1
                info = self.br.ifs.get(ln)
                if info and info[3]:
                    env = dict(frame.f_globals)
                    env.update(frame.f_locals)
                    for i, (co, _) in enumerate(info[3]):
                        if eval(co, env):
                            self.terms[(ln, i)] += 1
                last[0] = ln
            elif event == "return":
                self.arcs[(last[0], "exit")] += 1
            return local

        return local

    def count(self, f, *args):
        """f(*args), with the calls it makes of reduce, gcd and Appendix B's functions."""
        before = Counter(self.calls)
        r = f(*args)
        return r, self.calls - before


def section_pseudocode(path, V):
    """{name: lines} of the pseudocode of chapter 3, and of section 4.1 (the rows), read separately (join and take are
    in both)."""
    text = path.read_text(encoding="utf-8").split("\n## Appendix A")[0]
    ch3 = text.split("\n## 3. ")[1].split("\n## 4. ")[0]
    s41 = text.split("\n### 4.1 ")[1].split("\n### 4.2 ")[0]
    return V.parse_pseudocode(ch3), V.parse_pseudocode(s41)


def errors(py):
    """ERROR, which V.translate writes as ZeroDivisionError: kept for a zero divisor, OverflowError for a scale above
    the limit, ValueError otherwise (Appendix B's text)."""
    out = []
    for l in py.splitlines():
        if "raise ZeroDivisionError" in l and "numerator(y) == 0" not in l:
            l = l.replace("ZeroDivisionError", "OverflowError" if f"> {LIMIT}" in l else "ValueError")
        out.append(l)
    return "\n".join(out) + "\n"


def wanted(fn, pc3, V, kind="Column"):
    src = {PYNAME.get(k, k): v for k, v in pc3.items()}
    if fn not in src:
        return None, None
    lines = src[fn]
    py = errors(V.translate(lines)).replace("pair(", kind + "(")
    for k, v in PYNAME.items():
        py = re.sub(r"\bdef " + k + r"\(", "def " + v + "(", py)
    return py, f"{len(lines)} lines of pseudocode"


def row_namespace(ns, pc41, V):
    """The rows of section 4.1, pseudocode only: translated here, as Appendix B would, with `pair` as a Row, and run on
    top of B.1 (in a namespace of their own, since their join and take are not the columns')."""
    rns = dict(ns)
    exec("from collections import namedtuple\nRow = namedtuple('Row', 'shared entries')\n", rns)
    for fn in ROW_FUNCTIONS:
        py, _ = wanted(fn, pc41, V, "Row")
        exec(compile(py, ROWS_FILENAME, "exec"), rns)
    return rns


def run(path, ns_a, a_filename, V, check):
    code, tree, first_line, ns = load(path, ns_a)
    print(f"\nAppendix B.1 of {path.name}: {len(code.splitlines())} lines of Python from line {first_line} of the "
          f"paper, defining empty, {', '.join(FUNCTIONS)}, on top of Appendix A\n")

    # ---------------------------------------------------------------------------------- line for line against chapter 3
    pc3, pc41 = section_pseudocode(path, V)
    check(sorted(pc41) == sorted(ROW_FUNCTIONS), f"section 4.1 defines {sorted(pc41)}")
    print("Line for line against the pseudocode of chapter 3:")
    for fn in Funcs(tree).body:
        mine = ast.get_source_segment(code, fn) + "\n"
        if fn.name in PROSE:
            want, where = PROSE[fn.name], "section 3.4 (prose)" if fn.name in ("decimal", "scale") else "section 3.1 (prose)"
        else:
            want, where = wanted(fn.name, pc3, V)
            if want is None:
                check(False, f"{fn.name}: no pseudocode found in the paper")
                continue
        same = (ast.dump(ast.parse(mine)) == ast.dump(ast.parse(want))
                and len(mine.strip().splitlines()) == len(want.strip().splitlines()))
        ok = check(same, f"{fn.name}: Appendix B is not the translation of its pseudocode")
        if not ok:
            for a, b in zip(mine.splitlines() + [""] * 99, want.splitlines() + [""] * 99):
                if a.strip() != b.strip():
                    print(f"    appendix:    {a.strip()}\n    translation: {b.strip()}")
                    break
        print(f"  {fn.name:28s} {'identical' if ok else 'DIFFERS'} ({where})")

    rns = row_namespace(ns, pc41, V)
    print(f"  (section 4.1's rows, {', '.join(ROW_FUNCTIONS)}: pseudocode only, translated by this harness)")
    br = V.Branches(code, Funcs(tree))
    with Tracer(br, a_filename) as tr:
        check_functions(ns, rns, tr, V, check)
        check_decimals(ns, tr, V, check)
        check_examples(ns, rns, tr, check)
        check_decimal_examples(ns, tr, check)
    coverage(br, tr, first_line, check)
    return ns


def raises(f, *args):
    try:
        f(*args)
    except ValueError:
        return True
    return False


def as_list(L):
    out = []
    while L is not None:
        out.append(L[0])
        L = L[1]
    return out


def check_functions(ns, rns, tr, V, check):
    F, is_stored = V.F, V.is_stored
    rational = ns["rational"]
    Column, Row = ns["Column"], rns["Row"]
    rng = random.Random(20261030)
    counts = Counter()
    small = V.grid(12)

    # lists, columns and rows: pairing only
    L = ns["prepend"](3, ns["prepend"](-4, ns["empty"]))
    check(as_list(L) == [3, -4] and ns["first"](L) == 3 and ns["rest"](ns["rest"](L)) is None
          and ns["empty"] is None, "lists do not pair as section 3.1 says")
    for G in (Column(100, L), Row(100, L)):
        check(ns["shared"](G) == 100 and ns["entries"](G) is L, "columns and rows do not pair as section 3.1 says")
    check(not isinstance(Row(1, None), Column) and Column(1, None) == Row(1, None) == (1, None),
          "a Column and a Row are pairs of two kinds")
    counts["lists, columns and rows"] += 1

    # ------------------------------------------------------------------------------------------------ section 3.1
    for D in range(-5, 1):
        check(raises(ns["column"], D), f"column({D}) did not stop with ERROR")
        counts["column (ERROR)"] += 1
    Ds = list(range(1, 61)) + [100, 360, 3600, 2 ** 64] + [abs(V.random_int(rng)) + 1 for _ in range(40)]
    values = small + [V.random_value(rng) for _ in range(300)]
    for D in Ds:
        G0 = ns["column"](D)
        check(G0 == (D, None) and isinstance(G0, Column), f"column({D}) = {G0}")
        counts["column"] += 1
        G = G0
        joined = []
        for x in values + [rational(rng.randrange(-50, 50), D)]:
            x = V.reduced(*x)
            if D % x[1] == 0:
                G2, n = tr.count(ns["join"], G, x)
                ok = (isinstance(G2, Column) and G2[0] == D and G2[1][1] is G[1]
                      and Fraction(G2[1][0], D) == F(x) and n["gcd"] == 0)
                check(ok, f"join(column over {D}, {x}) = entry {G2[1][0]}")
                joined.append(F(x))
                G = G2
                counts["join (column)"] += 1
            else:
                check(raises(ns["join"], G, x), f"{x} joined the column over {D}")
                counts["join (column, ERROR)"] += 1
        # raw entries, signs and zeros, placed as they are
        raw = [rng.randrange(-10 ** 6, 10 ** 6) for _ in range(rng.randrange(0, 40))]
        for u in raw:
            G = Column(D, ns["prepend"](u, ns["entries"](G)))
        want = sum(joined, Fraction(0)) + Fraction(sum(raw), D)
        r, n = tr.count(ns["sum_"], G)
        check(is_stored(r) and F(r) == want, f"sum over {D}: {r}, expected {want}")
        check(n["reduce"] <= 1 and n["tally"] == len(as_list(G[1])) + 1 and n["take"] == 1,
              f"sum over {D}: not one deferred reduction ({dict(n)})")
        s = sum(as_list(G[1]))
        check(ns["tally"](G[1]) == s, f"tally over {D}")
        biggest = max([abs(e) for e in as_list(G[1])] + [1])
        check(abs(s).bit_length() <= biggest.bit_length() + math.ceil(math.log2(max(len(as_list(G[1])), 1))),
              "a sum grew by more than log2 N bits")
        counts["sum_"] += 1
        # an empty column, and a column whose entries cancel
        check(ns["sum_"](G0) == (0, 1), f"empty column over {D}: sum {ns['sum_'](G0)}")
        Gz = G0
        for u in raw + [-u for u in raw] + [0]:
            Gz = Column(D, ns["prepend"](u, ns["entries"](Gz)))
        check(ns["sum_"](Gz) == (0, 1), f"cancelling column over {D}: sum {ns['sum_'](Gz)}")
        counts["sum_ (zero)"] += 2

    for u in range(-200, 201):
        for D in list(range(1, 40)) + [100, 2 ** 70]:
            r, n = tr.count(ns["take"], Column(D, None), u)
            check(is_stored(r) and F(r) == Fraction(u, D), f"take(column over {D}, {u}) = {r}")
            check(n["reduce"] == (0 if u == 0 or D == 1 else 1), f"take {u}/{D}: reduce ran {n['reduce']} times")
            counts["take (column)"] += 1
    for _ in range(300):
        us = [V.random_int(rng) for _ in range(rng.randrange(0, 30))]
        Lu = None
        for u in us:
            Lu = ns["prepend"](u, Lu)
        check(ns["tally"](Lu) == sum(us), "tally")
        counts["tally"] += 1

    # ------------------------------------------------------------------------------------------------ section 4.1
    # (pseudocode only: rns holds this harness's translation of the rows, with their own join and take)
    check(raises(rns["row"], 0), "row(0) did not stop with ERROR")
    counts["row (ERROR)"] += 1
    numerators = list(range(-12, 13)) + [V.random_int(rng) for _ in range(30)]
    for a in [a for a in numerators if a != 0]:
        G0 = rns["row"](a)
        check(G0 == (a, None) and isinstance(G0, Row), f"row({a}) = {G0}")
        counts["row"] += 1
        G = G0
        for x in values + [V.reduced(a, abs(V.random_int(rng)) or 1) for _ in range(30)]:
            if x[0] == a:
                G2, n = tr.count(rns["join"], G, x)
                check(isinstance(G2, Row) and G2[0] == a and G2[1][0] == x[1] and G2[1][1] is G[1]
                      and not n["gcd"] and not n["reduce"], f"join(row of {a}, {x}) = {G2}")
                G = G2
                counts["join (row)"] += 1
            else:
                check(raises(rns["join"], G, x), f"{x} joined the row of {a}")
                counts["join (row, ERROR)"] += 1
        ents = as_list(G[1])
        for b in ents:
            r, n = tr.count(rns["take"], G, b)
            check(is_stored(r) and r == (a, b) and not n["gcd"] and not n["reduce"],
                  f"take(row of {a}, {b}) = {r}")
            counts["take (row)"] += 1
            for d in ents:
                r, n = tr.count(rns["product"], G, b, d)
                want = Fraction(a, b) * Fraction(a, d)
                check(is_stored(r) and F(r) == want and not n["gcd"] and not n["reduce"],
                      f"product({a}; {b}, {d}) = {r}, expected {want}")
                check(r == ns["multiply"]((a, b), (a, d)), "product differs from section 2.10's multiply")
                # what stays: only in the row of 1, as the entry b·d; the row of -1 sends it to the row of 1
                check((r[0] == a) == (a == 1), f"row of {a}: the product {r} stays/leaves wrongly")
                if a in (1, -1):
                    check(r == (1, b * d), f"unit numerator {a}: {r}")
                counts["product"] += 1
    # raw entries, placed directly on the caller's promise: b > 0, and gcd(a, b) = 1 unless a = ±1 (then always so)
    for b in range(1, 200):
        for a in (1, -1):
            check(math.gcd(a, b) == 1 and is_stored(rns["take"](Row(a, None), b)), f"{a}/{b}")
    for a in [a for a in numerators if a != 0]:
        raw = [b for b in list(range(1, 40)) + [abs(V.random_int(rng)) + 1 for _ in range(10)] if math.gcd(a, b) == 1]
        G = rns["row"](a)
        for b in raw:
            G = Row(rns["shared"](G), rns["prepend"](b, rns["entries"](G)))
        for b in raw[:12]:
            for d in raw[:12]:
                r, n = tr.count(rns["product"], G, b, d)
                check(is_stored(r) and F(r) == Fraction(a, b) * Fraction(a, d) and not n["gcd"],
                      f"raw entries: product({a}; {b}, {d}) = {r}")
                counts["product (raw entries)"] += 1
    print("\nAppendix B.1, and section 4.1's rows, against fractions.Fraction (taken-out values and row values: "
          "lowest terms, positive denominator):")
    for k in sorted(counts):
        print(f"  {k:36s} {counts[k]:8d} calls")


def check_examples(ns, rns, tr, check):
    """Every formula and example of section 3.1 and of the rows of section 4.1, with the calls it claims."""
    rational, reduce_, add = ns["rational"], ns["reduce"], ns["add"]
    Column = ns["Column"]
    # 3.1 storage and promise
    check(Fraction(310, 100) == Fraction("3.10") and Fraction(45, 100) == Fraction("0.45")
          and Fraction(25, 100) == Fraction("0.25") and Fraction(90, 60) == Fraction(3, 2), "3.1: money, minutes")
    g100 = ns["column"](100)
    check(ns["join"](g100, (1, 2))[1][0] == 50 and math.gcd(50, 100) == 50, "3.1: 1/2 is 50/100")
    check(raises(ns["join"], g100, (1, 3)), "3.1: 1/3 joined the column over 100")
    check(Fraction(1, 6) + Fraction(1, 6) == Fraction(1, 3) == Fraction(2, 6), "3.1: 1/6 + 1/6")
    # divisibility is needed: a reduced a/b equals some u/D only if b divides D
    for b in range(1, 30):
        for a in range(-20, 21):
            if math.gcd(a, b) != 1:
                continue
            for D in range(1, 60):
                exists = (Fraction(a, b) * D).denominator == 1
                check(exists == (D % b == 0), f"3.1: {a}/{b} over {D}")
    # the worked example: cents, with a refund
    gm = g100
    for u in (25, -120, 45, 310):
        gm = Column(100, ns["prepend"](u, ns["entries"](gm)))
    r, n = tr.count(ns["sum_"], gm)
    check(r == (13, 5) and n["tally"] == 5 and n["gcd"] == 1 and n["reduce"] == 1
          and math.gcd(260, 100) == 20 and 310 + 45 - 120 + 25 == 260,
          f"3.1 example: sum {r}, calls {dict(n)}")
    entered, n_entry = [], Counter()
    for p in (310, 45, -120, 25):
        x, n = tr.count(reduce_, rational(p, 100))
        entered.append(x)
        n_entry += n
    check(entered == [(31, 10), (9, 20), (-6, 5), (1, 4)] and n_entry["gcd"] == 4, f"3.1 example: entry {entered}")
    acc, gcds = entered[0], []
    for x in entered[1:]:
        acc, n = tr.count(add, acc, x)
        gcds.append(n["gcd"])
    check(acc == (13, 5) and gcds == [2, 2, 2], f"3.1 example: section 2 gives {acc} with gcds {gcds}")
    check(4 + sum(gcds) == 10, "3.1 example: ten gcds")
    check(ns["sum_"](Column(100, ns["prepend"](45, ns["prepend"](-45, None)))) == (0, 1), "3.1: 45 and -45")

    # 4.1
    g1, g3 = rns["row"](1), rns["row"](3)
    check(rns["product"](g1, 6, 6) == (1, 36), "4.1: two sixes")
    check(rns["product"](g3, 4, 5) == (9, 20), "4.1: 3/4 * 3/5")
    check(raises(rns["join"], g3, (2, 5)), "4.1: 2/5 joined the row of 3")
    check(ns["divide"]((1, 2), (1, 4)) == (2, 1) and Fraction(1, 2) / Fraction(1, 4) == Fraction(4, 2),
          "4.1: 1/2 / 1/4")
    check(rns["product"](rns["row"](-1), 2, 3) == (1, 6), "4.1: row of -1")
    # scaling a denominator breaks lowest terms unless c = a (no scaling); c = -a fails only on the sign
    for a in [k for k in range(-24, 25) if k]:
        for c in [k for k in range(-24, 25) if k and a % k == 0]:
            for d in range(1, 15):
                if math.gcd(c, d) != 1:
                    continue
                den = d * (a // c)
                low = den > 0 and math.gcd(a, den) == 1
                check(low == (c == a), f"4.1: scaling {c}/{d} to the numerator {a}")
                check((den < 0) == (a // c < 0) and Fraction(a, den) == Fraction(c, d), "4.1: scaling")
                if c == -a:
                    check(den == -d and math.gcd(a, den) == 1, f"4.1: {c}/{d} scaled to {a} fails only on the sign")
    # the sum of two values of a row is section 2.9's equal-numerators case, with a gcd
    _, n = tr.count(add, (1, 6), (1, 10))
    check(n["gcd"] >= 1, "4.1: 1/6 + 1/10 should need a gcd")
    print("The examples of sections 3 and 4: money and minutes, 1/2 as 50/100, 1/3 and 2/5 refused, the column of cents "
          "(4 additions and 1 gcd against 10 gcds), 1/36, 9/20, 1/2 / 1/4 = 2, scaling refused")


def raises_as(exc, f, *args):
    try:
        f(*args)
    except exc:
        return True
    return False


def two_five(q):
    """(q has no prime factors but 2 and 5, the smallest k with q dividing 10^k), by plain division."""
    i = j = 0
    while q % 2 == 0:
        q, i = q // 2, i + 1
    while q % 5 == 0:
        q, j = q // 5, j + 1
    return q == 1, max(i, j)


def check_decimals(ns, tr, V, check):
    """Sections 3.4-3.8: every decimal function against fractions.Fraction."""
    Decimal, dec, num, scale = ns["Decimal"], ns["decimal"], ns["numerator"], ns["scale"]
    rng = random.Random(20261031)
    counts = Counter()

    def val(x):
        return Fraction(x[0], 10 ** x[1])

    def is_dec(r, k=None):
        return (isinstance(r, Decimal) and type(r[0]) is int and type(r[1]) is int and 0 <= r[1] <= LIMIT
                and (k is None or r[1] == k))

    def is_rat(r):                     # a stored value of section 2, not a decimal
        return not isinstance(r, Decimal) and V.is_stored(r)

    # decimal, scale and numerator: pairing only
    for u, k in [(75, 2), (-3, 0), (0, LIMIT), (1 << 70, 5)]:
        x = dec(u, k)
        check(isinstance(x, Decimal) and num(x) == u and scale(x) == k, f"decimal({u}, {k}) = {x}")
        counts["decimal, scale, numerator"] += 1
    for k in range(0, 60):
        check(ns["power"](k) == 10 ** k, f"power({k})")
        counts["power"] += 1

    # strip and places
    for q in range(1, 3001):
        for f in range(2, 13):
            w = ns["strip"](q, f)
            e = q // w
            ok = q % w == 0 and w % f != 0
            while ok and e % f == 0:
                e //= f
            check(ok and e == 1, f"strip({q}, {f}) = {w}")
            counts["strip"] += 1
    qs = list(range(1, 2001)) + [2 ** i * 5 ** j for i in range(0, 25) for j in range(0, 25)]
    for q in qs:
        for k in (0, 1, 2, 5, 18, 19, 20):
            want = k if k > LIMIT else next((j for j in range(k, LIMIT + 1) if 10 ** j % q == 0), LIMIT + 1)
            check(ns["places"](q, k) == want, f"places({q}, {k}) = {ns['places'](q, k)}, expected {want}")
            counts["places"] += 1

    # decimals: every scale, small numerators with both signs and zero, and random large numerators
    grid = [dec(u, k) for k in range(0, LIMIT + 1) for u in (-11, -10, -3, -1, 0, 1, 2, 7, 10, 100)]
    rand = [dec(V.random_int(rng), rng.randrange(0, LIMIT + 1)) for _ in range(400)]
    pairs = [(x, y) for x in grid for y in grid] + [(rng.choice(rand), rng.choice(rand + grid)) for _ in range(4000)]
    for x, y in pairs:
        k, m = x[1], y[1]
        r = ns["add_decimals"](x, y)
        check(is_dec(r, max(k, m)) and val(r) == val(x) + val(y), f"add_decimals({x}, {y}) = {r}")
        r = ns["subtract_decimals"](x, y)
        check(is_dec(r, max(k, m)) and val(r) == val(x) - val(y), f"subtract_decimals({x}, {y}) = {r}")
        if k + m > LIMIT:
            check(raises_as(OverflowError, ns["multiply_decimals"], x, y), f"multiply_decimals({x}, {y}): no overflow")
            counts["multiply_decimals (scale overflow)"] += 1
        else:
            r = ns["multiply_decimals"](x, y)
            check(is_dec(r, k + m) and val(r) == val(x) * val(y), f"multiply_decimals({x}, {y}) = {r}")
            counts["multiply_decimals"] += 1
        counts["add_decimals, subtract_decimals"] += 1
    # no gcd in addition, subtraction or multiplication
    for x, y in pairs[:3000]:
        for f in ("add_decimals", "subtract_decimals", "multiply_decimals"):
            if f == "multiply_decimals" and x[1] + y[1] > LIMIT:
                continue
            _, n = tr.count(ns[f], x, y)
            check(not n["gcd"] and not n["reduce"], f"{f}({x}, {y}) computed a gcd")

    # to_rational: the reduced value, with reduce skipped for 0 and for scale 0
    for x in grid + rand:
        r, n = tr.count(ns["to_rational"], x)
        check(is_rat(r) and V.F(r) == val(x), f"to_rational({x}) = {r}")
        check(n["reduce"] == (0 if x[0] == 0 or x[1] == 0 else 1), f"to_rational({x}): reduce ran {n['reduce']} times")
        counts["to_rational"] += 1

    # to_decimal: a decimal at the smallest scale, the rational itself, or scale overflow
    rats = [V.reduced(p, q) for q in range(1, 1201) for p in (-7, -1, 0, 1, 3, 10)]
    rats += [V.reduced(p, 2 ** i * 5 ** j) for i in range(0, 24) for j in range(0, 24) for p in (1, -3, 7)]
    rats += [V.random_value(rng) for _ in range(500)]
    rats += [V.reduced(V.random_int(rng), 2 ** rng.randrange(0, 30) * 5 ** rng.randrange(0, 30)) for _ in range(500)]
    for r in rats:
        ok, kmin = two_five(r[1])
        if not ok:
            got = ns["to_decimal"](r)
            check(got is r, f"to_decimal({r}) = {got}, expected the rational itself")
            counts["to_decimal (not a decimal)"] += 1
        elif kmin > LIMIT:
            check(raises_as(OverflowError, ns["to_decimal"], r), f"to_decimal({r}): no scale overflow")
            counts["to_decimal (scale overflow)"] += 1
        else:
            got = ns["to_decimal"](r)
            check(is_dec(got, kmin) and val(got) == V.F(r), f"to_decimal({r}) = {got}")
            counts["to_decimal"] += 1

    # divide_decimals: exact; a decimal at the smallest scale, a stored rational, scale overflow, or a zero divisor
    dpairs = [(x, y) for x in grid[::3] for y in grid] + [(rng.choice(rand + grid), rng.choice(rand + grid))
                                                       for _ in range(3000)]
    dpairs += [(dec(1, 0), dec(2 ** i, 0)) for i in range(0, 25)] + [(dec(1, 0), dec(5 ** j, 0)) for j in range(0, 25)]
    for x, y in dpairs:
        if y[0] == 0:
            check(raises_as(ZeroDivisionError, ns["divide_decimals"], x, y), f"divide_decimals({x}, {y}): no error")
            counts["divide_decimals (zero divisor)"] += 1
            continue
        exact = val(x) / val(y)
        ok, kmin = two_five(exact.denominator)
        if ok and kmin > LIMIT:
            check(raises_as(OverflowError, ns["divide_decimals"], x, y), f"divide_decimals({x}, {y}): no overflow")
            counts["divide_decimals (scale overflow)"] += 1
            continue
        r = ns["divide_decimals"](x, y)
        if ok:
            check(is_dec(r, kmin) and val(r) == exact, f"divide_decimals({x}, {y}) = {r}")
            counts["divide_decimals (decimal)"] += 1
        else:
            check(is_rat(r) and V.F(r) == exact, f"divide_decimals({x}, {y}) = {r}")
            counts["divide_decimals (rational)"] += 1

    # section 3.8's rule: dividing by y keeps every decimal a decimal exactly when y's reduced numerator is 2^i 5^j
    xs = [dec(u, k) for k in (0, 1, 2) for u in range(-20, 21)]
    for y in [dec(u, k) for k in (0, 1, 2) for u in range(-30, 31) if u]:
        stays = two_five(abs(val(y).numerator))[0]
        left = False
        for x in xs:
            try:
                r = ns["divide_decimals"](x, y)
            except OverflowError:
                continue
            left = left or not isinstance(r, Decimal)
        check(stays == (not left), f"3.8 rule: dividing by {y}")
        if not stays:
            check(not isinstance(ns["divide_decimals"](dec(1, 0), y), Decimal), f"3.8 rule: 1 / {y} stays a decimal")
        counts["3.8 rule (divisors)"] += 1

    print("\nThe decimals of sections 3.4-3.8 against fractions.Fraction (decimals at the stated scale, rationals "
          "reduced):")
    for k in sorted(counts):
        print(f"  {k:36s} {counts[k]:8d} calls")


def check_decimal_examples(ns, tr, check):
    """The lcm of section 1.5, the aside of section 2.8, and the examples and claims of sections 3.2-3.8."""
    dec, Decimal, Column = ns["decimal"], ns["Decimal"], ns["Column"]

    def lcm(u, v):                      # the smallest positive common multiple, by search
        return next(m for m in range(1, abs(u * v) + 1) if m % u == 0 and m % v == 0)

    # 1.5
    check(lcm(4, 6) == 12 and lcm(-4, 6) == 12 and lcm(6, 10) == (6 // math.gcd(6, 10)) * 10 == 30, "1.5: examples")
    for u in [k for k in range(-30, 31) if k]:
        for v in [k for k in range(-30, 31) if k]:
            m = lcm(u, v)
            check(m * math.gcd(u, v) == abs(u * v) and m == (abs(u) // math.gcd(u, v)) * abs(v), f"1.5: lcm({u}, {v})")
            check(all(M % m == 0 for M in range(-3 * m, 3 * m + 1) if M % u == 0 and M % v == 0),
                  f"1.5: a common multiple of {u} and {v} is not a multiple of the lcm")
    for B in range(1, 13):
        for i in range(0, 7):
            for j in range(i, 7):
                check(math.gcd(B ** i, B ** j) == B ** i and math.lcm(B ** i, B ** j) == B ** j, f"1.5: powers of {B}")
    check(math.gcd(100, 10000) == 100 and math.lcm(100, 10000) == 10000, "1.5: 100 and 10 000")
    # 2.8
    check(math.gcd(3, 10) == 1 and math.gcd(6, 10) == 2 and Fraction(1, 6) + Fraction(1, 10) == Fraction(8, 30),
          "2.8: 1/6 + 1/10 over 30")

    # 3.2: the lcm two at a time, and the sum of section 2.6 over it
    rng = random.Random(7)
    for _ in range(300):
        bs = [rng.choice([-1, 1]) * rng.randrange(1, 60) for _ in range(rng.randrange(1, 7))]
        L = abs(bs[0])
        for b in bs[1:]:
            L = (L // math.gcd(L, b)) * abs(b)
        check(L == math.lcm(*bs), f"3.2: the lcm of {bs}")
    steps, L = [], 2
    for b in (3, 4, 5, 6):
        L = (L // math.gcd(L, b)) * b
        steps.append(L)
    check(steps == [6, 12, 60, 60], f"3.2: lcm steps {steps}")
    G = ns["column"](60)
    for b in (2, 3, 4, 5, 6):
        G = ns["join"](G, (1, b))
    check(as_list(G[1]) == [10, 12, 15, 20, 30], f"3.2: entries {as_list(G[1])}")
    r, n = tr.count(ns["sum_"], G)
    check(r == (29, 20) and n["gcd"] == 1 and n["reduce"] == 1 and math.gcd(87, 60) == 3
          and Fraction(29, 20) == sum(Fraction(1, b) for b in range(2, 7)), f"3.2 example: {r}, {dict(n)}")
    L13 = math.lcm(*range(2, 14))
    G = ns["column"](L13)
    for b in range(2, 14):
        G = ns["join"](G, (1, b))
    s = ns["tally"](G[1])
    check(L13 == 360360 and s == 785633 and math.gcd(s, L13) == 1 and ns["sum_"](G) == (785633, 360360)
          and Fraction(785633, 360360) == sum(Fraction(1, b) for b in range(2, 14)), "3.2: up to 1/13")

    # 3.4
    check(10 ** 18 <= 2 ** 63 - 1 < 10 ** 19 and 2 ** 63 - 1 == 9223372036854775807, "3.4: 18 for 64 bits")
    check(10 ** 9 <= 2 ** 31 - 1 < 10 ** 10, "3.4: 9 for 32 bits")
    check(Fraction(75, 100) == Fraction("0.75") and Fraction(310, 100) == Fraction("3.10")
          and Fraction(375, 100) == Fraction("3.75") and 5 * 10 ** 6 == 5000000, "3.3, 3.4: examples")
    # 3.5
    check(ns["add_decimals"](dec(310, 2), dec(45, 2)) == (355, 2), "3.5: 3.10 + 0.45")
    check(ns["add_decimals"](dec(310, 2), dec(35, 3)) == (3135, 3), "3.5: 3.10 + 0.035")
    check(ns["multiply_decimals"](dec(310, 2), dec(35, 3)) == (10850, 5)
          and Fraction(10850, 10 ** 5) == Fraction("3.10") * Fraction("0.035"), "3.5: 3.10 * 0.035")
    check(raises_as(OverflowError, ns["multiply_decimals"], dec(1, 9), dec(1, 10)), "3.5: scale overflow example")
    # 3.6: section 2.9's dividing out first, on powers of 10, is the alignment
    for k in range(0, 19):
        for m in range(k + 1, 19):
            b, d = 10 ** k, 10 ** m
            g = math.gcd(b, d)
            bp, dp = b // g, d // g
            for u, v in ((7, -3), (-12, 5), (1, 1)):
                t = u * dp + v * bp
                check(g == 10 ** k and bp == 1 and dp == 10 ** (m - k) and bp * d == 10 ** m
                      and ns["add_decimals"](dec(u, k), dec(v, m)) == (t, m), f"3.6: scales {k}, {m}")
    # 3.7
    r, n = tr.count(ns["to_rational"], dec(50, 2))
    check(r == (1, 2) and n["gcd"] == 1 and math.gcd(50, 100) == 50 and ns["to_rational"](dec(5, 1)) == (1, 2),
          "3.7: 0.50 and 0.5")
    # 3.8
    check([two_five(q)[0] for q in (1, 8, 20, 125, 3, 6)] == [True] * 4 + [False] * 2, "3.8: 1, 8, 20, 125; 3, 6")
    check(all(Fraction(1, 3) * 10 ** k % 1 != 0 for k in range(0, 40)), "3.8: 1/3 is not a decimal")
    r = ns["divide_decimals"](dec(1, 0), dec(8, 0))
    check(isinstance(r, Decimal) and r == (125, 3) and ns["places"](8, 0) == 3
          and [10 ** k % 8 for k in range(4)] == [1, 2, 4, 0], "3.8: 1 / 8")
    r = ns["divide_decimals"](dec(7, 0), dec(2, 0))
    check(isinstance(r, Decimal) and r == (35, 1), "3.8: 7 / 2")
    r = ns["divide_decimals"](dec(1, 0), dec(3, 1))
    check(not isinstance(r, Decimal) and r == (10, 3), "3.8: 1 / 0.3")
    r = ns["divide_decimals"](dec(6, 1), dec(3, 1))
    check(isinstance(r, Decimal) and r == (2, 0), "3.8: 0.6 / 0.3")
    check(raises_as(OverflowError, ns["divide_decimals"], dec(1, 0), dec(2 ** 19, 0)), "3.8: 1 / 2^19 overflows")
    print("The examples of chapters 1 and 3: lcm(4, 6), lcm * gcd = |u v|, powers of one base, 1/2 + ... + 1/6 over "
          "60 (and up to 1/13 over 360 360), 18 and 9 as the largest scales, 3.10 + 0.45, 3.10 + 0.035, 3.10 * 0.035, "
          "scale overflow, alignment as section 2.9, 0.50 as 1/2, 1 / 8, 7 / 2, 1 / 0.3, 0.6 / 0.3")


def coverage(br, tr, first_line, check):
    at = first_line - 1
    missing = []
    print("\nAppendix B.1 branch coverage (each `if`: times true / times false)")
    for ln in sorted(br.ifs):
        fn, text, target, terms = br.ifs[ln]
        t = tr.arcs[(ln, target)]
        f = sum(n for (a, b), n in tr.arcs.items() if a == ln and b is not None and b != target)
        print(f"  {fn:28s} line {at + ln:4d}  if {text:30s} true {t:7d}  false {f:7d}")
        if not t:
            missing.append(f"{fn} line {at + ln}: `if {text}` never true")
        if not f:
            missing.append(f"{fn} line {at + ln}: `if {text}` never false")
        for i, (_, src) in enumerate(terms):
            n = tr.terms[(ln, i)]
            print(f"  {'':28s}            or-term {src:22s} true {n:7d}")
            if not n:
                missing.append(f"{fn} line {at + ln}: or-term `{src}` never true")
    ran = {ln for (ln, b) in tr.arcs if b is None}
    unrun = sorted(set(br.lines) - ran)
    for ln in unrun:
        missing.append(f"line {at + ln} ({br.lines[ln]}) never ran")
    print(f"  statements run: {len(br.lines) - len(unrun)} of {len(br.lines)}")
    for m in missing:
        check(False, "Appendix B.1 coverage: " + m)
