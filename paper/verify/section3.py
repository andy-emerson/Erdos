"""Checks Appendix B of paper/v0.5.md, the section 3 library in Python, as verify.py checks Appendix A.

Appendix B's one ```python block is read from the paper and run on top of Appendix A's functions, which it uses and does
not repeat. Each function must be the line-for-line translation of its pseudocode in sections 3.1 and 3.2 (Appendix
A's substitutions, with ERROR raising ValueError); every function is checked against fractions.Fraction, including
signs, zero, values that cannot join, empty columns, sums that reduce to zero and raw entries; every `if` outcome (and
any `or` term) of Appendix B must be reached; and every formula and example of section 3 is reproduced, with the gcd and reduce calls it
claims. `join` and `take` are defined twice in the pseudocode, for a column (3.1) and for a row (3.2); in Appendix B each is one
function that runs the column definition under `if isinstance(g, Column):` and the row definition after it.
"""
import ast
import math
import random
import re
import sys
from collections import Counter
from fractions import Fraction

HEADING = "## Appendix B. The section 3 library in Python"
FILENAME = "<v0.5.md, Appendix B>"
FUNCTIONS = ["prepend", "first", "rest", "shared", "entries",
             "column", "row", "join", "take", "tally", "sum_", "product"]
HEAD = ["from collections import namedtuple", "empty = None",
        "Column = namedtuple('Column', 'shared entries')", "Row = namedtuple('Row', 'shared entries')"]
# the lists, columns and rows of section 3.1, defined in prose: their translation is fixed (Appendix B's own text)
PROSE = {"prepend": "def prepend(u, L):\n    return (u, L)\n",
         "first": "def first(L):\n    return L[0]\n",
         "rest": "def rest(L):\n    return L[1]\n",
         "shared": "def shared(g):\n    return g[0]\n",
         "entries": "def entries(g):\n    return g[1]\n"}
PYNAME = {"sum": "sum_"}           # `sum` is a Python built-in
BOTH = ("join", "take")            # defined for a column (3.1) and for a row (3.2), dispatched in Appendix B


def extract(path):
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
    if len(blocks) != 1:
        sys.exit(f"{path.name}: expected one python block in Appendix B, found {len(blocks)}")
    return blocks[0]


def load(path, ns_a):
    first_line, code = extract(path)
    tree = ast.parse(code)
    head = [ast.unparse(n) for n in tree.body[:len(HEAD)]]
    names = [n.name for n in tree.body[len(HEAD):] if isinstance(n, ast.FunctionDef)]
    if head != HEAD or names != FUNCTIONS or len(tree.body) != len(FUNCTIONS) + len(HEAD):
        sys.exit(f"Appendix B must be {HEAD} and then define exactly {FUNCTIONS} in this order; "
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
    """{name: lines} of the pseudocode of section 3.1, and of section 3.2, read separately (join and take are in both)."""
    text = path.read_text(encoding="utf-8").split("\n## Appendix A")[0].split("\n## 3. ")[1]
    s31, s32 = text.split("\n### 3.2 ")
    return V.parse_pseudocode(s31), V.parse_pseudocode(s32)


def body(lines, V, name, kind):
    """The translated body of one definition, with its column or row renamed g and `pair` written as the kind."""
    py = V.translate(lines).replace("raise ZeroDivisionError", "raise ValueError").replace("pair(", kind + "(")
    out = py.splitlines()[1:]
    return [re.sub(r"\b" + name + r"\b", "g", l) for l in out]


def wanted(fn, pc31, pc32, V):
    if fn in BOTH:
        params = V.translate(pc31[fn]).splitlines()[0].split("(", 1)[1].split(", ", 1)[1]
        lines = [f"def {fn}(g, {params}", "    if isinstance(g, Column):"]
        lines += ["    " + l for l in body(pc31[fn], V, "c", "Column")]
        lines += body(pc32[fn], V, "r", "Row")
        return "\n".join(lines) + "\n", f"{len(pc31[fn])} + {len(pc32[fn])} lines of pseudocode (3.1, 3.2)"
    src = {PYNAME.get(k, k): (v, "Column") for k, v in pc31.items()}
    src.update({PYNAME.get(k, k): (v, "Row") for k, v in pc32.items()})
    if fn not in src:
        return None, None
    lines, kind = src[fn]
    py = V.translate(lines).replace("raise ZeroDivisionError", "raise ValueError").replace("pair(", kind + "(")
    for k, v in PYNAME.items():
        py = re.sub(r"\bdef " + k + r"\(", "def " + v + "(", py)
    return py, f"{len(lines)} lines of pseudocode"


def run(path, ns_a, a_filename, V, check):
    code, tree, first_line, ns = load(path, ns_a)
    print(f"\nAppendix B of {path.name}: {len(code.splitlines())} lines of Python from line {first_line} of the "
          f"paper, defining empty, {', '.join(FUNCTIONS)}, on top of Appendix A\n")

    # ---------------------------------------------------------------------------- line for line against section 3
    pc31, pc32 = section_pseudocode(path, V)
    print("Line for line against the pseudocode of section 3:")
    for fn in Funcs(tree).body:
        mine = ast.get_source_segment(code, fn) + "\n"
        if fn.name in PROSE:
            want, where = PROSE[fn.name], "section 3.1 (prose)"
        else:
            want, where = wanted(fn.name, pc31, pc32, V)
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

    br = V.Branches(code, Funcs(tree))
    with Tracer(br, a_filename) as tr:
        check_functions(ns, tr, V, check)
        check_examples(ns, tr, check)
    coverage(br, tr, first_line, check)


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


def check_functions(ns, tr, V, check):
    F, is_stored = V.F, V.is_stored
    rational = ns["rational"]
    Column, Row = ns["Column"], ns["Row"]
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

    # ------------------------------------------------------------------------------------------------ section 3.2
    check(raises(ns["row"], 0), "row(0) did not stop with ERROR")
    counts["row (ERROR)"] += 1
    numerators = list(range(-12, 13)) + [V.random_int(rng) for _ in range(30)]
    for a in [a for a in numerators if a != 0]:
        G0 = ns["row"](a)
        check(G0 == (a, None) and isinstance(G0, Row), f"row({a}) = {G0}")
        counts["row"] += 1
        G = G0
        for x in values + [V.reduced(a, abs(V.random_int(rng)) or 1) for _ in range(30)]:
            if x[0] == a:
                G2, n = tr.count(ns["join"], G, x)
                check(isinstance(G2, Row) and G2[0] == a and G2[1][0] == x[1] and G2[1][1] is G[1]
                      and not n["gcd"] and not n["reduce"], f"join(row of {a}, {x}) = {G2}")
                G = G2
                counts["join (row)"] += 1
            else:
                check(raises(ns["join"], G, x), f"{x} joined the row of {a}")
                counts["join (row, ERROR)"] += 1
        ents = as_list(G[1])
        for b in ents:
            r, n = tr.count(ns["take"], G, b)
            check(is_stored(r) and r == (a, b) and not n["gcd"] and not n["reduce"],
                  f"take(row of {a}, {b}) = {r}")
            counts["take (row)"] += 1
            for d in ents:
                r, n = tr.count(ns["product"], G, b, d)
                want = Fraction(a, b) * Fraction(a, d)
                check(is_stored(r) and F(r) == want and not n["gcd"] and not n["reduce"],
                      f"product({a}; {b}, {d}) = {r}, expected {want}")
                check(r == ns["multiply"]((a, b), (a, d)), "product differs from section 2.7's multiply")
                # what stays: only in the row of 1, as the entry b·d; the row of -1 sends it to the row of 1
                check((r[0] == a) == (a == 1), f"row of {a}: the product {r} stays/leaves wrongly")
                if a in (1, -1):
                    check(r == (1, b * d), f"unit numerator {a}: {r}")
                counts["product"] += 1
    # raw entries, placed directly on the caller's promise: b > 0, and gcd(a, b) = 1 unless a = ±1 (then always so)
    for b in range(1, 200):
        for a in (1, -1):
            check(math.gcd(a, b) == 1 and is_stored(ns["take"](Row(a, None), b)), f"{a}/{b}")
    for a in [a for a in numerators if a != 0]:
        raw = [b for b in list(range(1, 40)) + [abs(V.random_int(rng)) + 1 for _ in range(10)] if math.gcd(a, b) == 1]
        G = ns["row"](a)
        for b in raw:
            G = Row(ns["shared"](G), ns["prepend"](b, ns["entries"](G)))
        for b in raw[:12]:
            for d in raw[:12]:
                r, n = tr.count(ns["product"], G, b, d)
                check(is_stored(r) and F(r) == Fraction(a, b) * Fraction(a, d) and not n["gcd"],
                      f"raw entries: product({a}; {b}, {d}) = {r}")
                counts["product (raw entries)"] += 1
    print("\nAppendix B against fractions.Fraction (taken-out values and row values: lowest terms, "
          "positive denominator):")
    for k in sorted(counts):
        print(f"  {k:36s} {counts[k]:8d} calls")


def check_examples(ns, tr, check):
    """Every formula and example of section 3, with the calls it claims."""
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

    # 3.2
    g1, g3 = ns["row"](1), ns["row"](3)
    check(ns["product"](g1, 6, 6) == (1, 36), "3.2: two sixes")
    check(ns["product"](g3, 4, 5) == (9, 20), "3.2: 3/4 * 3/5")
    check(raises(ns["join"], g3, (2, 5)), "3.2: 2/5 joined the row of 3")
    check(ns["divide"]((1, 2), (1, 4)) == (2, 1) and Fraction(1, 2) / Fraction(1, 4) == Fraction(4, 2),
          "3.2: 1/2 / 1/4")
    check(ns["product"](ns["row"](-1), 2, 3) == (1, 6), "3.2: row of -1")
    # scaling a denominator breaks lowest terms unless c = a (no scaling); c = -a fails only on the sign
    for a in [k for k in range(-24, 25) if k]:
        for c in [k for k in range(-24, 25) if k and a % k == 0]:
            for d in range(1, 15):
                if math.gcd(c, d) != 1:
                    continue
                den = d * (a // c)
                low = den > 0 and math.gcd(a, den) == 1
                check(low == (c == a), f"3.2: scaling {c}/{d} to the numerator {a}")
                check((den < 0) == (a // c < 0) and Fraction(a, den) == Fraction(c, d), "3.2: scaling")
                if c == -a:
                    check(den == -d and math.gcd(a, den) == 1, f"3.2: {c}/{d} scaled to {a} fails only on the sign")
    # the sum of two values of a row is section 2.6's equal-numerators case, with a gcd
    _, n = tr.count(add, (1, 6), (1, 10))
    check(n["gcd"] >= 1, "3.2: 1/6 + 1/10 should need a gcd")
    print("Section 3's examples: money and minutes, 1/2 as 50/100, 1/3 and 2/5 refused, the column of cents "
          "(4 additions and 1 gcd against 10 gcds), 1/36, 9/20, 1/2 / 1/4 = 2, scaling refused")


def coverage(br, tr, first_line, check):
    at = first_line - 1
    missing = []
    print("\nAppendix B branch coverage (each `if`: times true / times false)")
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
        check(False, "Appendix B coverage: " + m)
