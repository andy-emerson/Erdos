"""Checks the Python library of paper/v0.5.md, Appendix A.

The code is read from the appendix's code block itself, so what is tested is exactly what the paper shows: if the two
drift apart, this fails. Each function must also be the mechanical translation of its final pseudocode in sections 1-2,
line for line (the appendix's own substitutions: `·` is `*`, `/` is `//`, `b′` is `b_prime`, and so on). Every function is checked against Python's fractions.Fraction. Branch coverage of every `if`
in the appendix is recorded and reported, and every outcome must be reached. The worked computation of section 2.9 is
reproduced call for call, with its intermediate values and the branches it takes. Finally the library is compared,
untimed, with the textbook control of 10-interning on round 2's small (toy) workloads (control.py).

    python3 paper/verify/verify.py                  everything
    python3 paper/verify/verify.py --no-control     without the comparison with the control (needs no compiler)
"""
import argparse
import ast
import math
import random
import sys
from collections import Counter
from fractions import Fraction
from pathlib import Path

sys.dont_write_bytecode = True    # no __pycache__ beside the paper (control.py is imported)

HERE = Path(__file__).resolve().parent
PAPER = HERE.parent / "v0.5.md"
HEADING = "## Appendix A. The library in Python"
FILENAME = "<v0.5.md, Appendix A>"
FUNCTIONS = ["rational", "numerator", "denominator", "mod", "gcd", "reduce", "add", "subtract", "multiply", "divide"]
OPERATIONS = ("add", "subtract", "multiply", "divide")
FAILURES = []


def check(cond, what):
    if not cond:
        FAILURES.append(what)
        if len(FAILURES) <= 20:
            print("  FAIL:", what)
    return cond


# --------------------------------------------------------------------------------------------- the code, from the paper
def extract(path):
    """The one ```python block of Appendix A, and the line of the paper where it starts."""
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
        sys.exit(f"{path.name}: expected one python block in Appendix A, found {len(blocks)}")
    return blocks[0]


def load(path):
    first_line, code = extract(path)
    tree = ast.parse(code)
    names = [n.name for n in tree.body if isinstance(n, ast.FunctionDef)]
    if names != FUNCTIONS or len(tree.body) != len(FUNCTIONS):
        sys.exit(f"Appendix A must define exactly {FUNCTIONS} in this order and nothing else; it defines {names}")
    ns = {}
    exec(compile(code, FILENAME, "exec"), ns)
    return code, tree, first_line, ns


# ------------------------------------------------------------------------- the translation, against the pseudocode
# The appendix's substitutions, applied mechanically to the paper's pseudocode. Order matters: "≠" and "=" before the
# words, "/" before nothing else makes "//".
NAMES = {"′": "_prime", "₊": "_plus", "₋": "_minus", "₁": "1", "₂": "2", "₃": "3", "₄": "4", "₅": "5", "₆": "6"}
# rational, numerator and denominator (section 1.2) and mod (section 2.3) are defined in prose, not in pseudocode:
# their translation is fixed
PROSE = {"rational": "def rational(p, q):\n    return (p, q)\n",
         "numerator": "def numerator(x):\n    return x[0]\n",
         "denominator": "def denominator(x):\n    return x[1]\n",
         "mod": "def mod(u, v):\n    return u % v\n"}


def pseudocode(path):
    """{name: lines} of each function's last pseudocode definition before Appendix A (its final form)."""
    return parse_pseudocode(path.read_text(encoding="utf-8").split("\n## Appendix A")[0])


def parse_pseudocode(text):
    """{name: lines} of each function's last pseudocode definition in text."""
    out, cur = {}, None
    inside = False
    for line in text.split("\n"):
        if line.strip() == "```text":
            inside, cur = True, None
            continue
        if inside and line.strip() == "```":
            inside, cur = False, None
            continue
        if not inside:
            continue
        if line.startswith("FUNCTION "):
            cur = line.split()[1].split("(")[0]
            out[cur] = [line]
        elif cur and line.startswith("    "):
            out[cur].append(line)
        elif cur and not line.strip():
            continue
        else:
            cur = None
    return out


def expr(e):
    e = e.replace("·", "*").replace("−", "-").replace("/", "//")
    for k, v in NAMES.items():
        e = e.replace(k, v)
    return e


def cond(c):
    c = expr(c).replace("≠", "!=").replace(" = ", " == ").replace(" AND ", " and ").replace(" OR ", " or ")
    return c


def translate(lines):
    py = []
    for line in lines:
        ind = line[:len(line) - len(line.lstrip())]
        s = line.strip()
        if s.startswith("FUNCTION "):
            py.append(ind + "def " + s[len("FUNCTION "):] + ":")
        elif s.startswith("LET "):
            name, value = s[4:].split(" = ", 1)
            py.append(ind + expr(name) + " = " + expr(value))
        elif s.startswith("IF "):
            c, then = s[3:].split(" THEN", 1)
            then = then.strip()
            if not then:
                py.append(ind + "if " + cond(c) + ":")
            elif then == "ERROR":
                py.append(ind + "if " + cond(c) + ": raise ZeroDivisionError")
            else:
                assert then.startswith("RETURN "), line
                py.append(ind + "if " + cond(c) + ": return " + expr(then[7:]))
        elif s.startswith("RETURN "):
            py.append(ind + "return " + expr(s[7:]))
        else:
            raise ValueError(f"untranslatable pseudocode line: {line!r}")
    return "\n".join(py) + "\n"


def check_literal(path, code, tree):
    """Each function of the appendix is the mechanical translation of its final pseudocode: the same statements, in
    the same order, one line of Python per line of pseudocode."""
    pc = pseudocode(path)
    print("Line for line against the final pseudocode of sections 1-2:")
    for fn in tree.body:
        mine = ast.get_source_segment(code, fn) + "\n"
        if fn.name in PROSE:
            want, where = PROSE[fn.name], "section 2.3 (prose)" if fn.name == "mod" else "section 1.2 (prose)"
        elif fn.name in pc:
            want, where = translate(pc[fn.name]), f"{len(pc[fn.name])} lines of pseudocode"
        else:
            check(False, f"{fn.name}: no pseudocode found in the paper")
            continue
        same_tree = ast.dump(ast.parse(mine)) == ast.dump(ast.parse(want))
        same_lines = len(mine.strip().splitlines()) == len(want.strip().splitlines())
        ok = check(same_tree and same_lines, f"{fn.name}: the appendix is not the translation of its pseudocode")
        if not ok:
            for a, b in zip(mine.splitlines() + [""] * 99, want.splitlines() + [""] * 99):
                if a.strip() != b.strip():
                    print(f"    appendix:    {a.strip()}\n    translation: {b.strip()}")
                    break
        print(f"  {fn.name:12s} {'identical' if ok else 'DIFFERS'} ({where})")
    print()


# ------------------------------------------------------------------------------------------------ coverage and tracing
class Branches:
    """Every `if` of the appendix: its condition's text, the line it is on, where its true outcome goes (the same
    line for a one-line `if ...: return`, else the first line of its block), and its top-level `or` terms."""

    def __init__(self, code, tree):
        self.ifs = {}          # line -> (function, condition text, true target, [or-terms as code objects, text])
        self.lines = {}        # line -> function, for every statement
        for fn in tree.body:
            for node in ast.walk(fn):
                if isinstance(node, ast.stmt) and node is not fn:
                    self.lines[node.lineno] = fn.name
                if isinstance(node, ast.If):
                    body = node.body[0].lineno
                    target = "exit" if body == node.lineno else body
                    terms = []
                    if isinstance(node.test, ast.BoolOp) and isinstance(node.test.op, ast.Or):
                        for t in node.test.values:
                            src = ast.get_source_segment(code, t)
                            terms.append((compile(src, FILENAME, "eval"), src))
                    self.ifs[node.lineno] = (fn.name, ast.get_source_segment(code, node.test), target, terms)


class Tracer:
    """sys.settrace over the appendix's frames only. It records, per frame, the lines run in order (a frame's exit
    as "exit"), the call tree with arguments and results, and the locals at return; and, over the whole run, the
    arcs between lines and which `or` terms were ever true."""

    def __init__(self, br):
        self.br = br
        self.arcs = {"all": Counter(), "ops": Counter()}
        self.terms = Counter()
        self.phase = "all"
        self.log = None        # a list of call records, when recording calls
        self.depth = 0
        self.op_depth = 0      # > 0 while inside one of the four operations
        self.reduce_in_ops = 0

    def __enter__(self):
        sys.settrace(self.global_trace)
        return self

    def __exit__(self, *a):
        sys.settrace(None)

    def global_trace(self, frame, event, arg):
        if frame.f_code.co_filename != FILENAME or event != "call":
            return None
        name = frame.f_code.co_name
        code = frame.f_code
        rec = {"name": name, "depth": self.depth, "args": tuple(frame.f_locals[v] for v in
                                                                 code.co_varnames[:code.co_argcount]),
               "lines": [], "result": None, "locals": None}
        if self.log is not None:
            self.log.append(rec)
        if name == "reduce" and self.op_depth:
            self.reduce_in_ops += 1
        is_op = name in OPERATIONS
        self.depth += 1
        self.op_depth += is_op
        last = [None]
        arcs = (self.arcs["all"], self.arcs["ops"] if self.op_depth else None)

        def local(frame, event, arg):
            if event == "line":
                ln = frame.f_lineno
                for c in arcs:
                    if c is not None and last[0] is not None:
                        c[(last[0], ln)] += 1
                for c in arcs:
                    if c is not None:
                        c[(ln, None)] += 1           # the line itself ran
                info = self.br.ifs.get(ln)
                if info and info[3]:
                    env = dict(frame.f_locals)
                    for i, (co, _) in enumerate(info[3]):
                        if eval(co, {}, env):
                            self.terms[(ln, i)] += 1
                rec["lines"].append(ln)
                last[0] = ln
            elif event == "return":
                for c in arcs:
                    if c is not None:
                        c[(last[0], "exit")] += 1
                rec["lines"].append("exit")
                rec["result"] = arg
                rec["locals"] = dict(frame.f_locals)
                self.depth -= 1
                self.op_depth -= is_op
            return local

        return local

    def outcomes(self, lines):
        """[(condition text, True/False)] for the ifs of one frame's line sequence."""
        out = []
        for here, nxt in zip(lines, lines[1:]):
            info = self.br.ifs.get(here)
            if info:
                out.append((info[1], nxt == info[2]))
        return out


def coverage_report(br, tr, first_line):
    """Prints the branch coverage, by line of the paper; returns the outcomes never reached."""
    at = first_line - 1
    missing, reached_ops_missing = [], []
    print("\nBranch coverage (each `if`: times true / times false; [ops] = also reached from inside the four "
          "operations)")
    for ln in sorted(br.ifs):
        fn, text, target, terms = br.ifs[ln]
        res = {}
        for phase in ("all", "ops"):
            arcs = tr.arcs[phase]
            t = arcs[(ln, target)]
            f = sum(n for (a, b), n in arcs.items() if a == ln and b is not None and b != target)
            res[phase] = (t, f)
        t, f = res["all"]
        ot, of = res["ops"]
        tag = "" if fn in ("rational", "numerator", "denominator", "mod") else (
            "  [ops: both]" if ot and of else f"  [ops: {'true only' if ot else 'false only' if of else 'never'}]")
        print(f"  {fn:9s} line {at + ln:4d}  if {text:55s} true {t:7d}  false {f:7d}{tag}")
        if not t:
            missing.append(f"{fn} line {at + ln}: `if {text}` never true")
        if not f:
            missing.append(f"{fn} line {at + ln}: `if {text}` never false")
        if ot == 0 or of == 0:
            reached_ops_missing.append((fn, text, "true" if ot == 0 else "false"))
        for i, (_, src) in enumerate(terms):
            n = tr.terms[(ln, i)]
            print(f"  {'':9s}            or-term {src:47s} true {n:7d}")
            if not n:
                missing.append(f"{fn} line {at + ln}: or-term `{src}` never true")
    ran = {ln for (ln, b) in tr.arcs["all"] if b is None}
    unrun = sorted(set(br.lines) - ran)
    n_out = 2 * len(br.ifs)
    n_terms = sum(len(v[3]) for v in br.ifs.values())
    lost_terms = sum(1 for m in missing if "or-term" in m)
    lost_out = len(missing) - lost_terms
    print(f"\n  if outcomes reached: {n_out - lost_out} of {n_out};  or-terms true: {n_terms - lost_terms} of "
          f"{n_terms};  statements run: {len(br.lines) - len(unrun)} of {len(br.lines)}")
    if reached_ops_missing:
        print("  outcomes not reached from inside the four operations (reached only by direct calls; no operation calls reduce):")
        for fn, text, which in reached_ops_missing:
            print(f"    {fn}: `if {text}` {which}")
    for ln in unrun:
        missing.append(f"line {at + ln} ({br.lines[ln]}) never ran")
    return missing


# ---------------------------------------------------------------------------------------------------------- the checks
def is_stored(r):
    return (isinstance(r, tuple) and len(r) == 2 and all(type(v) is int for v in r) and r[1] > 0
            and math.gcd(r[0], r[1]) == 1)


def F(x):
    return Fraction(x[0], x[1])


def reduced(p, q):
    f = Fraction(p, q)
    return (f.numerator, f.denominator)


def grid(m):
    """Every stored value with |numerator| <= m and 1 <= denominator <= m: zero, +-1, both signs."""
    return [(p, q) for q in range(1, m + 1) for p in range(-m, m + 1) if math.gcd(p, q) == 1]


def random_int(rng):
    k = rng.choice([4, 8, 16, 32, 64, 65, 128, 200])
    return rng.randrange(-(1 << k), 1 << k)


def random_value(rng):
    while True:
        q = abs(random_int(rng)) or 1
        p = random_int(rng)
        if rng.random() < 0.05:
            p = rng.choice([0, 1, -1])
        if rng.random() < 0.05:
            q = 1
        return reduced(p, q)


def related_pair(rng):
    """Two stored values with one of the coincidences the special cases look for, built at random sizes (the
    coincidence can be lost when the second value is reduced; the pair is then an ordinary one)."""
    a, b = random_value(rng)
    k = rng.randrange(11)
    m = abs(random_int(rng)) + 1
    s = rng.choice([1, -1])
    c, d = [(random_int(rng), b),            # shared denominator
            (a, m), (-a, m),                  # equal, opposite numerators
            (s * b, m),                       # cross match, opposite cross match
            (m * s, abs(a) or 1),             # cross match other way (a = d or a = -d)
            (s * b, abs(a) or 1),             # reciprocals, opposite reciprocals
            (random_int(rng), b * m),         # denominators with a common factor
            (a * m, b * m + 1),               # a numerator with a factor of the other's
            (b, a * s or 1),
            (random_int(rng), 1),             # an integer operand
            (a * m, 1)][k]                    # an integer sharing a factor with the other numerator
    if d == 0:
        d = 1
    return (a, b), reduced(c, d)


def check_functions(ns, tr):
    rational, numerator, denominator = ns["rational"], ns["numerator"], ns["denominator"]
    gcd, reduce_ = ns["gcd"], ns["reduce"]
    ops = {"add": (ns["add"], lambda x, y: x + y), "subtract": (ns["subtract"], lambda x, y: x - y),
           "multiply": (ns["multiply"], lambda x, y: x * y), "divide": (ns["divide"], lambda x, y: x / y)}
    rng = random.Random(20261029)
    counts = Counter()

    # rational, numerator, denominator: pairing only, nothing computed
    for p, q in [(3, 4), (-3, 4), (3, -4), (-3, -4), (0, 5), (6, 4), (1 << 100, -(1 << 70))]:
        x = rational(p, q)
        check(numerator(x) == p and denominator(x) == q, f"rational({p}, {q}) does not keep the pair")
        counts["rational, numerator, denominator"] += 1

    # mod: the remainder of section 2.3, for u >= 0 and v > 0
    mod = ns["mod"]
    pairs = [(u, v) for u in range(0, 61) for v in range(1, 31)]
    pairs += [(abs(random_int(rng)), abs(random_int(rng)) or 1) for _ in range(3000)]
    for u, v in pairs:
        r = mod(u, v)
        check(type(r) is int and 0 <= r < v and (u - r) % v == 0, f"mod({u}, {v}) = {r}")
        counts["mod"] += 1

    # gcd: every sign combination and zero, then random values
    pairs = [(u, v) for u in range(-30, 31) for v in range(-30, 31) if (u, v) != (0, 0)]
    pairs += [(random_int(rng), random_int(rng)) for _ in range(3000)]
    pairs += [(m * random_int(rng), m * random_int(rng)) for m in (abs(random_int(rng)) + 1 for _ in range(1000))]
    for u, v in pairs:
        if (u, v) == (0, 0):
            continue
        g = gcd(u, v)
        check(g == math.gcd(u, v) and g > 0, f"gcd({u}, {v}) = {g}")
        counts["gcd"] += 1

    # reduce: any pair with a nonzero denominator, every sign combination
    pairs = [(p, q) for p in range(-30, 31) for q in range(-30, 31) if q]
    pairs += [(random_int(rng), random_int(rng) or 1) for _ in range(3000)]
    pairs += [(m * random_int(rng), m * (random_int(rng) or 1)) for m in (abs(random_int(rng)) + 1
                                                                          for _ in range(1000))]
    for p, q in pairs:
        r = reduce_(rational(p, q))
        check(is_stored(r) and r == reduced(p, q), f"reduce({p}, {q}) = {r}")
        counts["reduce"] += 1

    # the four operations, on stored values only (their promise), with no call to reduce from inside them
    small = grid(12)
    cases = [(x, y) for x in small for y in small]
    cases += [(random_value(rng), random_value(rng)) for _ in range(4000)]
    cases += [related_pair(rng) for _ in range(8000)]
    cases += [(y, x) for x, y in cases[-8000:]]
    tr.reduce_in_ops = 0
    for x, y in cases:
        for name, (fn, exact) in ops.items():
            if name == "divide" and y[0] == 0:
                continue
            r = fn(x, y)
            ok = is_stored(r) and F(r) == exact(F(x), F(y))
            check(ok, f"{name}({x}, {y}) = {r}, expected {exact(F(x), F(y))}")
            counts[name] += 1

    # division by zero raises an error, whatever the dividend
    for x in small + [random_value(rng) for _ in range(200)]:
        try:
            r = ops["divide"][0](x, rational(0, 1))
            check(False, f"divide({x}, 0/1) returned {r} instead of raising")
        except ZeroDivisionError:
            counts["divide by zero"] += 1
    check(tr.reduce_in_ops == 0, f"an operation called reduce {tr.reduce_in_ops} times (section 2.9: none does)")

    print("Checked against fractions.Fraction (results stored: lowest terms, positive denominator):")
    for k in ["rational, numerator, denominator"] + FUNCTIONS[3:] + ["divide by zero"]:
        print(f"  {k:34s} {counts[k]:8d} calls")


# ------------------------------------------------------------------------------------------------- section 2.9's trace
def check_trace(ns, tr):
    """The worked computation of section 2.9: z = reduce(rational(-44, 30)); r = subtract(x, y); w = divide(r, z),
    with x = 5/6 and y = 1/10. Every call of reduce, gcd and the operations, in order, with its arguments and result;
    the branches each takes; and its intermediate values."""
    rational = ns["rational"]
    x, y = rational(5, 6), rational(1, 10)
    tr.log = []
    z = ns["reduce"](rational(-44, 30))
    r = ns["subtract"](x, y)
    w = ns["divide"](r, z)
    log = [e for e in tr.log if e["name"] not in ("rational", "numerator", "denominator", "mod")]
    tr.log = None

    G = "gcd"
    expected_calls = [
        # entry
        (0, "reduce", ((-44, 30),), (-22, 15)),
        (1, G, (-44, 30), 2), (2, G, (44, 30), 2), (3, G, (30, 14), 2), (4, G, (14, 2), 2), (5, G, (2, 0), 2),
        # subtraction and addition
        (0, "subtract", ((5, 6), (1, 10)), (11, 15)),
        (1, "add", ((5, 6), (-1, 10)), (11, 15)),
        (2, G, (6, 10), 2), (3, G, (10, 6), 2), (4, G, (6, 4), 2), (5, G, (4, 2), 2), (6, G, (2, 0), 2),
        (2, G, (22, 2), 2), (3, G, (2, 0), 2),
        # division and multiplication
        (0, "divide", ((11, 15), (-22, 15)), (-1, 2)),
        (1, "multiply", ((11, 15), (-15, 22)), (-1, 2)),
        (2, G, (11, 22), 11), (3, G, (22, 11), 11), (4, G, (11, 0), 11),
    ]
    got_calls = [(e["depth"], e["name"], e["args"], e["result"]) for e in log]
    ok = check(got_calls == expected_calls, "section 2.9: the calls differ")
    if not ok:
        for a, b in zip(got_calls + [None] * 40, expected_calls + [None] * 40):
            if a != b:
                print("    got", a, "\n    expected", b)
                break
    n = Counter(e["name"] for e in log)
    check(all(n[k] == 1 for k in ("reduce", "subtract", "add", "divide", "multiply")),
          f"section 2.9: reduce and each operation must run exactly once, ran {dict(n)}")

    no = False
    expected_branches = {
        "reduce": ([("b < 0", no), ("g == 1", no)],
                   dict(a=-44, b=30, g=2, p=-22, q=15)),
        "subtract": ([], dict(c=1, d=10)),
        "add": ([("a == 0", no), ("c == 0", no), ("b == 1 and d == 1", no), ("b == 1", no), ("d == 1", no),
                 ("c == -a and d == b", no),
                 ("d == a or d == -a or b == c or b == -c", no), ("d == b", no), ("g == 1", no),
                 ("c == a", no), ("c == -a", no), ("h == 1", no)],
                dict(a=5, b=6, c=-1, d=10, g=2, b_prime=3, d_prime=5, t=22, h=2, p=11, q=15)),
        "divide": ([("numerator(y) == 0", no), ("c < 0", True)], dict(c=-22, d=15)),
        "multiply": ([("a == 0", no), ("c == 0", no), ("a == 1 and b == 1", no), ("c == 1 and d == 1", no),
                      ("a == -1 and b == 1", no), ("c == -1 and d == 1", no),
                      ("b == 1 and d == 1", no),
                      ("(a == d and c == b) or (a == -d and c == -b)", no),
                      ("(a == d and c == -b) or (a == -d and c == b)", no),
                      ("d == b", no), ("c == a", no), ("c == -a", no), ("b == 1", no), ("d == 1", no),
                      ("c == b", no), ("a == d", no), ("c == -b", True), ("k3 == 1", no)],
                     dict(a=11, b=15, c=-15, d=22, k3=11)),
    }
    print("\nSection 2.9, the worked computation:")
    for e in log:
        if e["name"] == "gcd":
            continue
        want_br, want_vals = expected_branches[e["name"]]
        got_br = tr.outcomes(e["lines"])
        check(got_br == want_br, f"section 2.9, {e['name']}: branches {got_br}, expected {want_br}")
        vals = {k: e["locals"].get(k) for k in want_vals}
        check(vals == want_vals, f"section 2.9, {e['name']}: values {vals}, expected {want_vals}")
        taken = next((c for c, t in got_br if t), None)
        print(f"  {e['name']:9s} {e['args']} -> {e['result']};  {len(got_br)} checks"
              f"{', taken: if ' + taken if taken else ', none taken'};  "
              + ", ".join(f"{k} = {v}" for k, v in vals.items()))
    check(z == (-22, 15) and r == (11, 15) and w == (-1, 2), f"section 2.9: z, r, w = {z}, {r}, {w}")
    check(F(w) == (Fraction(5, 6) - Fraction(1, 10)) / Fraction(-44, 30), "section 2.9: w has the wrong value")


# ------------------------------------------------------------------------------------- special cases ordered by cost
# Section 1.6: special cases are ordered by the cost of their result, cheapest first; where two results cost the same,
# the cheaper test goes first. Each special case is a top-level `if` on a, b, c and d only; the harness knows each
# one's cost, and the paper's code gives the order. The cost of a result is counted in multiplications in section 1
# and in gcds in section 2; a test costs 1 against constants and 2 when it compares the operands.
S1 = {"add": ({"a == 0": (0, 1), "c == 0": (0, 1), "b == 1 and d == 1": (0, 1), "b == 1": (1, 1), "d == 1": (1, 1),
               "d == b": (0, 2), "c == a": (2, 2), "c == -a": (2, 2)}, lambda a, b, c, d: 3),
      "multiply": ({"a == 0": (0, 1), "c == 0": (0, 1), "a == 1 and b == 1": (0, 1), "c == 1 and d == 1": (0, 1),
                    "a == -1 and b == 1": (0, 1), "c == -1 and d == 1": (0, 1), "b == 1 and d == 1": (1, 1),
                    "(a == d and c == b) or (a == -d and c == -b)": (0, 2),
                    "(a == d and c == -b) or (a == -d and c == b)": (0, 2),
                    "c == b": (0, 2), "a == d": (0, 2), "c == -b": (0, 2), "a == -d": (0, 2),
                    "b == 1": (1, 1), "d == 1": (1, 1)}, lambda a, b, c, d: 2)}
_g2 = lambda a, b, c, d: 1 if math.gcd(b, d) == 1 else 2     # section 2.6: the second gcd is skipped when g = 1
S2 = {"add": ({"a == 0": (0, 1), "c == 0": (0, 1), "b == 1 and d == 1": (0, 1), "b == 1": (0, 1), "d == 1": (0, 1),
               "c == -a and d == b": (0, 2), "d == a or d == -a or b == c or b == -c": (0, 2), "d == b": (1, 2),
               "c == a": (_g2, 2), "c == -a": (_g2, 2)}, _g2),
      "multiply": ({**S1["multiply"][0], "b == 1 and d == 1": (0, 1), "d == b": (0, 2), "c == a": (0, 2),
                    "c == -a": (0, 2), "b == 1": (1, 1), "d == 1": (1, 1),
                    "c == b": (1, 2), "a == d": (1, 2), "c == -b": (1, 2), "a == -d": (1, 2)}, lambda a, b, c, d: 2)}


def special_cases(fn_tree, code):
    out = []
    for node in fn_tree.body:
        if isinstance(node, ast.If):
            names = {n.id for n in ast.walk(node.test) if isinstance(n, ast.Name)}
            if names <= set("abcd"):
                out.append(ast.get_source_segment(code, node.test))
    return out


def check_order(path):
    """Exhaustively, for |p|, q <= 15: the case each input reaches gives a result no dearer than any other case that
    holds for it, and among the cases whose results cost the same it has the cheapest test."""
    text = path.read_text(encoding="utf-8")
    sec1 = parse_pseudocode(text.split("\n## 2. ")[0])
    sec2 = pseudocode(path)
    vals1 = [(p, q) for q in range(1, 16) for p in range(-15, 16)]           # section 1: any positive denominator
    vals2 = [v for v in vals1 if math.gcd(*v) == 1]                         # section 2: reduced
    print("\nSpecial cases ordered by the cost of their result (section 1.6), every pair with |p|, q <= 15:")
    for sec, pc, table, vals in (("1", sec1, S1, vals1), ("2", sec2, S2, vals2)):
        for name, (costs, general) in table.items():
            src = translate(pc[name])
            conds = special_cases(ast.parse(src).body[0], src)
            if not check(sorted(conds) == sorted(costs), f"section {sec} {name}: special cases {conds}"):
                continue
            compiled = [(t, compile(t, "<cond>", "eval")) for t in conds]
            n = bad = 0
            for a, b in vals:
                for c, d in vals:
                    env = {"a": a, "b": b, "c": c, "d": d}
                    held = [t for t, co in compiled if eval(co, {}, env)]

                    def cost(t):
                        k = costs[t][0]
                        return k(a, b, c, d) if callable(k) else k
                    n += 1
                    if not held:
                        continue
                    r = held[0]
                    best = min(min(cost(t) for t in held), general(a, b, c, d))
                    tie = min(costs[t][1] for t in held if cost(t) == cost(r))
                    if cost(r) > best or costs[r][1] > tie:
                        bad += 1
                        check(False, f"section {sec} {name}({a}/{b}, {c}/{d}) reaches `{r}`, but {held} hold")
            print(f"  section {sec} {name:9s} {len(conds):2d} special cases, {n:7d} pairs, {bad} out of order")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--no-control", action="store_true", help="skip the comparison with 10-interning's control")
    ap.add_argument("--paper", type=Path, default=PAPER, help="the paper to read (default: %(default)s)")
    a = ap.parse_args()
    code, tree, first_line, ns = load(a.paper)
    print(f"Appendix A of {a.paper.name}: {len(code.splitlines())} lines of Python from line {first_line} of the paper, "
          f"defining {', '.join(FUNCTIONS)}\n")
    check_literal(a.paper, code, tree)
    check_order(a.paper)
    br = Branches(code, tree)
    with Tracer(br) as tr:
        check_functions(ns, tr)
        check_trace(ns, tr)
    missing = coverage_report(br, tr, first_line)
    for m in missing:
        check(False, "coverage: " + m)
    import section3
    section3.run(a.paper, ns, FILENAME, sys.modules[__name__], check)
    if not a.no_control:
        import control
        control.compare(ns, check)
    print()
    if FAILURES:
        print(f"FAILED: {len(FAILURES)} check(s)")
        return 1
    print("ALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
