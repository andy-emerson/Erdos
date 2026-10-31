"""Checks chapter 2 of paper/v0.7.md, the quantities: a magnitude (an unsigned integer of chapter 1) with a direction.

The chapter's last pseudocode, that of section 2.3 (the directions as two-bit counters) with negate and
subtract_quantities of section 2.1 and multiply_quantities of section 2.2, is library code: it is part of Appendix A,
and verify.py builds its Python reference with the rest of Appendix A's. The checks of that reference are here
(check_library), and run through verify.py's recorder and tracer, so that every call is replayed in Nim and every `if`
outcome must be reached.

The earlier forms are pseudocode only, superseded in the chapter: section 2.1's (the directions 1 and -1, written as
marks) and section 2.2's (the multiplication table of the four directions 1, i, -1, -i). They are translated here, as
verify.py translates the library, with the directions as Python numbers: 1 and -1, and i as 1j, so that -i is -1j
(check_pseudocode). Everything is checked against exact arithmetic on the four directions: a quantity with magnitude m
and direction d is the number d * m.
"""
import random
from collections import Counter

FUNCTIONS = ["quantity", "magnitude", "direction", "times", "opposite", "negate", "add_quantities",
             "subtract_quantities", "multiply_quantities"]
EARLY_FILENAME = "<v0.7.md, sections 2.1-2.2: pseudocode only>"
# what each section's pseudocode defines (its last definition of each name within the section)
DEFINES = {"2.1": ["opposite", "negate", "add_quantities", "subtract_quantities"],
           "2.2": ["times", "multiply_quantities", "opposite", "add_quantities"],
           "2.3": ["times", "opposite", "add_quantities"]}
# section 2.3's counters: the direction each stands for
VALUE = {0: 1, 1: 1j, 2: -1, 3: -1j}
COUNTER = {v: k for k, v in VALUE.items()}
# the primitives of section 2.1, defined in prose, and the direction i of section 2.2, for the pseudocode-only forms
PRIMS = ("from collections import namedtuple\n"
         "Quantity = namedtuple('Quantity', 'magnitude direction')\n"
         "i = 1j\n\n\n"
         "def quantity(m, s):\n    return Quantity(m, s)\n\n\n"
         "def magnitude(x):\n    return x[0]\n\n\n"
         "def direction(x):\n    return x[1]\n\n\n")


def section_text(path, sec):
    text = path.read_text(encoding="utf-8").split("\n## Appendix A")[0]
    return text.split(f"\n### {sec} ")[1].split("\n### ")[0].split("\n## ")[0]


def check_pseudocode(path, V, check):
    """Sections 2.1 and 2.2, pseudocode only: translated here and checked; returns their namespaces."""
    pcs = {sec: V.parse_pseudocode(section_text(path, sec)) for sec in DEFINES}
    for sec, names in DEFINES.items():
        check(sorted(pcs[sec]) == sorted(names), f"section {sec} defines {sorted(pcs[sec])}, expected {sorted(names)}")
    # the chapter's names are not defined again after chapter 2, so section 2.3's forms are the library's
    text = path.read_text(encoding="utf-8").split("\n## Appendix A")[0]
    later = V.parse_pseudocode(text.split("\n## 3. ")[1])
    check(not set(later) & set(FUNCTIONS), f"chapters 3-5 define {sorted(set(later) & set(FUNCTIONS))} again")

    def code(*secs):
        out = PRIMS
        for sec in secs:
            for name in DEFINES[sec]:
                out += V.errors(V.translate(pcs[sec][name])) + "\n\n"
        return out
    ns21, ns22 = {}, {}
    exec(compile(code("2.1"), EARLY_FILENAME, "exec"), ns21)
    exec(compile(code("2.1", "2.2"), EARLY_FILENAME, "exec"), ns22)   # 2.2's opposite and add replace 2.1's
    counts = Counter()

    def stored(x, dirs):
        return (type(x).__name__ == "Quantity" and type(x[0]) is int and x[0] >= 0 and x[1] in dirs
                and (x[0] > 0 or x[1] == 1))

    # ------------------------------------------------------------------ section 2.1: the directions 1 and -1
    ns = ns21
    Q = ns["quantity"]
    real = [Q(m, s) for m in range(0, 16) for s in (1, -1) if m or s == 1]
    check(ns["opposite"](1) == -1 and ns["opposite"](-1) == 1, "2.1: opposite")
    # every integer has exactly one stored quantity
    for u in range(-40, 41):
        forms = [x for x in [Q(m, s) for m in range(0, 41) for s in (1, -1) if m or s == 1] if x[1] * x[0] == u]
        check(forms == [Q(abs(u), 1 if u >= 0 else -1)], f"2.1: the stored quantities of {u}: {forms}")
        counts["2.1 stored forms"] += 1
    for x in real:
        r = ns["negate"](x)
        check(stored(r, (1, -1)) and r[1] * r[0] == -(x[1] * x[0]), f"2.1: negate({x}) = {r}")
        for y in real:
            for name, f in (("add_quantities", lambda a, b: a + b), ("subtract_quantities", lambda a, b: a - b)):
                r = ns[name](x, y)
                check(stored(r, (1, -1)) and r[1] * r[0] == f(x[1] * x[0], y[1] * y[0]), f"2.1: {name}({x}, {y}) = {r}")
                counts[f"2.1 {name}"] += 1
    check(ns["add_quantities"](Q(3, 1), Q(5, -1)) == Q(2, -1), "2.1: 3 + (-5) = -2")
    check(ns["subtract_quantities"](Q(3, 1), Q(5, 1)) == Q(2, -1), "2.1: 3 - 5 = -2")

    # ------------------------------------------------------------------ section 2.2: the table of the complex axes
    ns = ns22
    Q = ns["quantity"]
    dirs = (1, 1j, -1, -1j)
    for s in dirs:
        for t in dirs:
            check(ns["times"](s, t) == s * t, f"2.2: the table gives {s} * {t} = {ns['times'](s, t)}")
            counts["2.2 table entries"] += 1
        check(ns["opposite"](s) == -s, f"2.2: opposite({s})")
    check(all(ns["times"](s, t) == s * t in (1, -1) for s in (1, -1) for t in (1, -1)), "2.2: the rule of signs")
    check(ns["times"](1j, 1j) == -1, "2.2: i * i = -1")
    grid = [Q(m, s) for m in range(0, 9) for s in dirs if m or s == 1]
    for x in grid:
        vx = x[1] * x[0]
        r = ns["negate"](x)
        check(stored(r, dirs) and r[1] * r[0] == -vx, f"2.2: negate({x}) = {r}")
        for y in grid:
            vy = y[1] * y[0]
            r = ns["multiply_quantities"](x, y)
            check(stored(r, dirs) and r[1] * r[0] == vx * vy, f"2.2: multiply_quantities({x}, {y}) = {r}")
            counts["2.2 multiply_quantities"] += 1
            for name, f, yy in (("add_quantities", lambda a, b: a + b, y),
                                ("subtract_quantities", lambda a, b: a - b, Q(y[0], -y[1]) if y[0] else y)):
                one_axis = x[0] == 0 or yy[0] == 0 or yy[1] in (x[1], -x[1])
                try:
                    r = ns[name](x, y)
                    check(one_axis and stored(r, dirs) and r[1] * r[0] == f(vx, vy), f"2.2: {name}({x}, {y}) = {r}")
                except ValueError:
                    check(not one_axis, f"2.2: {name}({x}, {y}) stopped with ERROR")
                counts[f"2.2 {name}"] += 1
    check(ns["multiply_quantities"](Q(3, 1j), Q(4, 1j)) == Q(12, -1), "2.2: 3i * 4i = -12")
    check(ns["add_quantities"](Q(0, 1), Q(4, 1j)) == Q(4, 1j), "2.2: 0 + 4i = 4i")
    for f, y in (("add_quantities", Q(4, 1j)), ("subtract_quantities", Q(4, 1j))):
        try:
            ns[f](Q(3, 1), y)
            check(False, f"2.2: {f}(3, 4i) did not stop with ERROR")
        except ValueError:
            pass
    # the real quantities are closed: a computation that starts with them never reaches i
    rng = random.Random(20261030)
    pool = [Q(m, s) for m in range(0, 6) for s in (1, -1) if m or s == 1]
    for _ in range(3000):
        x, y = rng.choice(pool), rng.choice(pool)
        op = rng.choice(["add_quantities", "subtract_quantities", "multiply_quantities", "negate"])
        r = ns[op](x) if op == "negate" else ns[op](x, y)
        check(stored(r, (1, -1)), f"2.2: {op} of real quantities gave {r}")
        if r[0] < 10 ** 6:
            pool.append(r)
        counts["2.2 real closure (steps)"] += 1
    print("\nChapter 2's earlier forms, pseudocode only (section 2.1: the directions 1 and -1; section 2.2: the table of "
          "1, i, -1, -i), against exact arithmetic on the directions:")
    for k, v in counts.items():
        print(f"  {k:34s} {v:8d}")
    return ns22


def check_library(api, ns22, V, check):
    """Section 2.3's final forms, the library: the counters of 0 to 3, through the recorder (replayed in Nim)."""
    Q = api["quantity"]
    counts = Counter()
    nim = V.nimlib

    def value(x):
        """x's value exactly, as the integer pair (real part, imaginary part): no float, whatever the magnitude"""
        d = VALUE[x[1]]
        return (int(d.real) * x[0], int(d.imag) * x[0])

    def plus(a, b):
        return (a[0] + b[0], a[1] + b[1])

    def minus(a, b):
        return (a[0] - b[0], a[1] - b[1])

    def times(a, b):
        return (a[0] * b[0] - a[1] * b[1], a[0] * b[1] + a[1] * b[0])

    def neg(a):
        return (-a[0], -a[1])

    def stored(x):
        return (type(x).__name__ == "Quantity" and type(x[0]) is int and x[0] >= 0 and x[1] in VALUE
                and (x[0] > 0 or x[1] == 0))

    # the counter against the table of section 2.2, entry for entry; u AND 3 is mod(u, 4)
    for s in range(4):
        for t in range(4):
            r = api["times"](s, t)
            check(r == COUNTER[ns22["times"](VALUE[s], VALUE[t])] == (s + t) % 4, f"2.3: times({s}, {t}) = {r}")
            counts["times (the 16 entries)"] += 1
        r = api["opposite"](s)
        check(VALUE[r] == -VALUE[s] and r == COUNTER[ns22["opposite"](VALUE[s])], f"2.3: opposite({s}) = {r}")
        counts["opposite"] += 1
        for t in range(4):
            same = VALUE[t] in (VALUE[s], -VALUE[s])
            check(same == ((s & 1) == (t & 1)), f"2.3: the axis of {s} and {t}")
    check(all(eval(V.expr("u AND 3"), {"u": u}) == u % 4 for u in range(0, 300)), "2.3: u AND 3 is mod(u, 4)")
    # section 2.3's examples, through the paper's translation of AND: 4 is 100 and 6 is 110 in base 2
    check([bin(4), bin(6), bin(2)] == ["0b100", "0b110", "0b10"]
          and eval(V.expr("4 AND 3")) == 0 and eval(V.expr("6 AND 3")) == 2
          and eval(V.expr("(s + t) AND 3"), {"s": 1, "t": 3}) == 0 and eval(V.expr("(s + t) AND 3"), {"s": 3, "t": 3}) == 2,
          "2.3: the examples 4 AND 3 = 0 and 6 AND 3 = 2")

    rng = random.Random(20261031)
    grid = [Q(m, s) for m in range(0, 13) for s in range(4) if m or s == 0]
    big = [Q(m, rng.randrange(4)) for m in (2 ** 31, 2 ** 32 + 1, 2 ** 62, 2 ** 63 - 1)]
    big += [Q(abs(V.random_int(rng)) or 1, rng.randrange(4)) for _ in range(400)]
    cases = [(x, y) for x in grid for y in grid] + [(rng.choice(big + grid), rng.choice(big + grid)) for _ in range(4000)]
    for x in grid + big:
        r = api["negate"](x)
        check(stored(r) and value(r) == neg(value(x)), f"2.3: negate({x}) = {r}")
        counts["negate"] += 1
    for x, y in cases:
        r = api["multiply_quantities"](x, y)
        check(stored(r) and value(r) == times(value(x), value(y)), f"2.3: multiply_quantities({x}, {y}) = {r}")
        counts["multiply_quantities"] += 1
        for name, f, yy in (("add_quantities", plus, y),
                            ("subtract_quantities", minus, Q(y[0], (y[1] + 2) % 4) if y[0] else y)):
            one_axis = x[0] == 0 or yy[0] == 0 or (x[1] & 1) == (yy[1] & 1)
            try:
                r = api[name](x, y)
                check(one_axis and stored(r) and value(r) == f(value(x), value(y)), f"2.3: {name}({x}, {y}) = {r}")
            except ValueError:
                check(not one_axis, f"2.3: {name}({x}, {y}) stopped with ERROR")
                counts[f"{name} (ERROR, different axes)"] += 1
            counts[name] += 1
    # the real quantities, the counters 0 and 2, are closed, and never stop with ERROR
    pool = [Q(m, s) for m in range(0, 6) for s in (0, 2) if m or s == 0]
    for _ in range(3000):
        x, y = rng.choice(pool), rng.choice(pool)
        op = rng.choice(["add_quantities", "subtract_quantities", "multiply_quantities", "negate"])
        r = api[op](x) if op == "negate" else api[op](x, y)
        check(stored(r) and r[1] in (0, 2), f"2.3: {op} of real quantities gave {r}")
        if r[0] < 10 ** 6:
            pool.append(r)
        counts["real closure (steps)"] += 1

    # the chapter's examples, traced in Nim too
    with nim.traced():
        check(api["add_quantities"](Q(3, 0), Q(5, 2)) == Q(2, 2), "2.1: 3 + (-5) = -2")
        check(api["subtract_quantities"](Q(3, 0), Q(5, 0)) == Q(2, 2), "2.1: 3 - 5 = -2")
        check(api["multiply_quantities"](Q(3, 1), Q(4, 1)) == Q(12, 2), "2.2: 3i * 4i = -12")
        check(api["add_quantities"](Q(0, 0), Q(4, 1)) == Q(4, 1), "2.2: 0 + 4i = 4i")
        check(api["times"](1, 3) == 0 and api["times"](3, 3) == 2, "2.3: i * (-i) = 1, (-i) * (-i) = -1")
        for name in ("add_quantities", "subtract_quantities"):
            try:
                api[name](Q(3, 0), Q(4, 1))
                check(False, f"2.2: {name}(3, 4i) did not stop with ERROR")
            except ValueError:
                pass
    print("\nChapter 2's library (section 2.3's counters) against exact arithmetic on the directions (results stored: "
          "magnitude >= 0, zero only with the direction 0):")
    for k, v in counts.items():
        print(f"  {k:42s} {v:8d} calls")
    print("The examples of chapter 2: 3 + (-5) = -2, 3 - 5 = -2, 3i * 4i = -12, 0 + 4i = 4i, 3 + 4i and 3 - 4i stop with "
          "ERROR, i * (-i) = 1, (-i) * (-i) = -1, 4 AND 3 = 0, 6 AND 3 = 2")
