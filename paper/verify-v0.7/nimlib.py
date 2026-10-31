"""The Nim appendices of paper/v0.7.md: read, compared line for line with the pseudocode, compiled, and run on every call
the Python checks make.

The paper's library appendices (A; B.1 and B.2) are in Nim. This module:

1. reads the one ```nim block of Appendix A (chapters 1-3: the integers, the quantities of chapter 2, the rationals) and the two of Appendix B (B.1, B.2) from the paper;
2. checks that each is exactly its fixed head (the types and constants, which the pseudocode does not state) followed by
   its functions in order, each one either a fixed short form (the primitives the paper defines in prose) or the
   mechanical translation of its last pseudocode definition (NIM_RULES below), with the signature fixed here, since
   the pseudocode states no types;
3. records every call that the Python checks (verify.py, section3.py, section4.py, control.py) make of the library's
   functions, with its arguments and the Python reference's result, where the Python reference is the harness's own
   translation of the same pseudocode (Python stays the checking harness);
4. compiles the Nim code exactly as the paper shows it, with a driver generated from its signatures, and replays every
   call whose arguments fit Nim's 64-bit integers; each result must equal the reference's, value for value, or be an
   int64 overflow that an int64 model of the pseudocode also reaches (the model: the same Python translation with every
   integer checked to stay in 64 bits, and home's word arithmetic modulo 2^64);
5. compiles an instrumented copy (a counter at every `if`, every `or` term and every statement, and a call trace) and
   reports branch and statement coverage of the Nim code by line of the paper; every outcome must be reached;
6. traces a sample of calls in both languages and compares the call trees: every call, its arguments and result, and
   the outcome of every `if` it evaluates.
"""
import ast
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from collections import Counter, defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
HEADINGS = {"A": "## Appendix A. The library in Nim", "B": "## Appendix B. The extension library in Nim"}
INT_MIN, INT_MAX = -(1 << 63), (1 << 63) - 1
MASK64 = (1 << 64) - 1
PHI = 0x9E3779B97F4A7C15
INDENT = "  "                                    # Nim's two spaces a level; the pseudocode's four
# The pinned compiler, the one spec/11-matrix/nim pins: Nim 2.2.14 from the GitHub tag v2.2.14, built by its build_all.sh
PINNED = {"version": "2.2.14", "git": "c5c2337564e347bb8252a510c79a74817a670231",
          "sha256": "c844c0fd004a76b1c1a6bc9b2e16a7878b375aba9dd582a29f66e667a17d0441"}

# ------------------------------------------------------------------------------------------------- the fixed parts
# What the pseudocode does not state: the types, the constants, and the primitives defined in prose. Each part of the
# appendix is HEAD, then its functions in ORDER, separated by one blank line.
HEAD = {
    "A": "type\n"
         "  Quantity = object\n"
         "    m: int64\n"
         "    s: uint8\n"
         "  Rational = object\n"
         "    p, q: int64",
    "B.1": "type\n"
           "  List[V] = ref object\n"
           "    head: V\n"
           "    tail: List[V]\n"
           "  Column = object\n"
           "    s: int64\n"
           "    L: List[int64]\n"
           "  Decimal = object\n"
           "    u: int64\n"
           "    k: uint8\n"
           "  DecimalOrRational = object\n"
           "    case is_decimal: bool\n"
           "    of true: d: Decimal\n"
           "    of false: r: Rational\n"
           "\n"
           "const empty = nil\n"
           "\n"
           "converter either(x: Decimal): DecimalOrRational =\n"
           "  return DecimalOrRational(is_decimal: true, d: x)\n"
           "\n"
           "converter either(x: Rational): DecimalOrRational =\n"
           "  return DecimalOrRational(is_decimal: false, r: x)",
    "B.2": "import std/sequtils\n"
           "\n"
           "const unused = low(int64)\n"
           "const φ = 0x9E3779B97F4A7C15'u64\n"
           "\n"
           "type Matrix = object\n"
           "  r, c, w: int64\n"
           "  H, L: seq[int64]",
}
ORDER = {
    "A": ["mod", "gcd", "quantity", "magnitude", "direction", "times", "opposite", "negate", "add_quantities",
          "subtract_quantities", "multiply_quantities", "rational", "numerator", "denominator", "reduce", "add", "subtract",
          "multiply", "divide"],
    "B.1": ["prepend", "first", "rest", "pair", "shared", "entries", "column", "join", "take", "tally", "sum",
            "decimal", "scale", "numerator", "power", "add_decimals", "subtract_decimals", "multiply_decimals",
            "to_rational", "strip", "places", "to_decimal", "divide_decimals"],
    "B.2": ["array", "size", "matrix", "row_count", "column_count", "handles", "values", "cell", "entry", "high_word",
            "home", "probe", "put", "table_size", "place", "positions", "copy_values", "rewrite", "bits", "pack",
            "place_all", "build", "add_cells", "add_matrices", "dot", "product_cells", "product_rows",
            "multiply_matrices"],
}
PROSE = {
    "A": {"quantity": "proc quantity(m, s: int64): Quantity =\n  return Quantity(m: m, s: uint8(s))",
          "magnitude": "proc magnitude(x: Quantity): int64 =\n  return x.m",
          "direction": "proc direction(x: Quantity): int64 =\n  return int64(x.s)",
          "rational": "proc rational(p, q: int64): Rational =\n  return Rational(p: p, q: q)",
          "numerator": "proc numerator(x: Rational): int64 =\n  return x.p",
          "denominator": "proc denominator(x: Rational): int64 =\n  return x.q"},
    "B.1": {"prepend": "proc prepend[V](u: V, L: List[V]): List[V] =\n  return List[V](head: u, tail: L)",
            "first": "proc first[V](L: List[V]): V =\n  return L.head",
            "rest": "proc rest[V](L: List[V]): List[V] =\n  return L.tail",
            "pair": "proc pair(s: int64, L: List[int64]): Column =\n  return Column(s: s, L: L)",
            "shared": "proc shared(g: Column): int64 =\n  return g.s",
            "entries": "proc entries(g: Column): List[int64] =\n  return g.L",
            "decimal": "proc decimal(u, k: int64): Decimal =\n  return Decimal(u: u, k: uint8(k))",
            "scale": "proc scale(x: Decimal): int64 =\n  return int64(x.k)",
            "numerator": "proc numerator(x: Decimal): int64 =\n  return x.u"},
    "B.2": {"array": "proc array(n: int64): seq[int64] =\n  return newSeqWith(n, unused)",
            "size": "proc size(A: seq[int64]): int64 =\n  return A.len",
            "matrix": "proc matrix(r, c, w: int64, H, L: sink seq[int64]): Matrix =\n"
                      "  return Matrix(r: r, c: c, w: w, H: H, L: L)",
            "row_count": "proc row_count(M: Matrix): int64 =\n  return M.r",
            "column_count": "proc column_count(M: Matrix): int64 =\n  return M.c",
            "handles": "proc handles(M: Matrix): lent seq[int64] =\n  return M.H",
            "values": "proc values(M: Matrix): lent seq[int64] =\n  return M.L",
            # (w · n) >> 64 of home, for w and n below 2^64: the top 64 of the 128 binary digits of the product,
            # from 32-bit halves, since Nim's integers stop at 64 bits
            "high_word": "proc high_word(w: uint64, n: int64): int64 =\n"
                         "  let a = w shr 32\n"
                         "  let b = w and 0xFFFF_FFFF'u64\n"
                         "  let c = uint64(n) shr 32\n"
                         "  let d = uint64(n) and 0xFFFF_FFFF'u64\n"
                         "  let carry = (((b * d) shr 32) + ((a * d) and 0xFFFF_FFFF'u64) + ((b * c) and 0xFFFF_FFFF'u64)) shr 32\n"
                         "  return int64((a * c) + ((a * d) shr 32) + ((b * c) shr 32) + carry)"},
}
# The signatures of the functions translated from pseudocode: their types, which the pseudocode does not state.
SIGNATURE = {
    "mod": "proc `mod`(u, v: int64): int64 =",
    "gcd": "proc gcd(u, v: int64): int64 =",
    "times": "proc times(s, t: int64): int64 =",
    "opposite": "proc opposite(s: int64): int64 =",
    "negate": "proc negate(x: Quantity): Quantity =",
    "add_quantities": "proc add_quantities(x, y: Quantity): Quantity =",
    "subtract_quantities": "proc subtract_quantities(x, y: Quantity): Quantity =",
    "multiply_quantities": "proc multiply_quantities(x, y: Quantity): Quantity =",
    "reduce": "proc reduce(x: Rational): Rational =",
    "add": "proc add(x, y: Rational): Rational =",
    "subtract": "proc subtract(x, y: Rational): Rational =",
    "multiply": "proc multiply(x, y: Rational): Rational =",
    "divide": "proc divide(x, y: Rational): Rational =",
    "column": "proc column(q: int64): Column =",
    "join": "proc join(c: Column, x: Rational): Column =",
    "take": "proc take(c: Column, k: int64): Rational =",
    "tally": "proc tally(L: List[int64]): int64 =",
    "sum": "proc sum(c: Column): Rational =",
    "power": "proc power(k: int64): int64 =",
    "add_decimals": "proc add_decimals(x, y: Decimal): Decimal =",
    "subtract_decimals": "proc subtract_decimals(x, y: Decimal): Decimal =",
    "multiply_decimals": "proc multiply_decimals(x, y: Decimal): Decimal =",
    "to_rational": "proc to_rational(x: Decimal): Rational =",
    "strip": "proc strip(q, f: int64): int64 =",
    "places": "proc places(q, k: int64): int64 =",
    "to_decimal": "proc to_decimal(r: Rational): DecimalOrRational =",
    "divide_decimals": "proc divide_decimals(x, y: Decimal): DecimalOrRational =",
    "cell": "proc cell(M: Matrix, k: int64): Rational =",
    "entry": "proc entry(M: Matrix, i, j: int64): Rational =",
    "home": "proc home(v, n: int64): int64 =",
    "probe": "proc probe(T: seq[int64], v, s: int64): int64 =",
    "put": "proc put(T: var seq[int64], v: int64): int64 =",
    "table_size": "proc table_size(m, s: int64): int64 =",
    "place": "proc place(T, H: var seq[int64], k: int64, x: Rational) =",
    "positions": "proc positions(T: seq[int64], P: var seq[int64], s, k: int64): int64 =",
    "copy_values": "proc copy_values(T, P: seq[int64], L: var seq[int64], s: int64) =",
    "rewrite": "proc rewrite(H, P: seq[int64], G: var seq[int64], i: int64) =",
    "bits": "proc bits(m: int64): int64 =",
    "pack": "proc pack(r, c: int64, T, H: seq[int64]): Matrix =",
    "place_all": "proc place_all(T, H: var seq[int64], k: int64, X: List[Rational]) =",
    "build": "proc build(r, c: int64, X: List[Rational]): Matrix =",
    "add_cells": "proc add_cells(A, B: Matrix, T, H: var seq[int64], k: int64) =",
    "add_matrices": "proc add_matrices(A, B: Matrix): Matrix =",
    "dot": "proc dot(A, B: Matrix, i, j, t: int64, s: Rational): Rational =",
    "product_cells": "proc product_cells(A, B: Matrix, T, H: var seq[int64], i, j: int64) =",
    "product_rows": "proc product_rows(A, B: Matrix, T, H: var seq[int64], i: int64) =",
    "multiply_matrices": "proc multiply_matrices(A, B: Matrix): Matrix =",
}
# The one function whose arithmetic is on 64-bit words, not on the library's integers: home's two lines (section 5.4)
WORDS = {"(v · φ) AND (2⁶⁴ − 1)": "(uint64(v) * φ) and high(uint64)",
         "(w · n) >> 64": "high_word(w, n)"}
ERROR_MESSAGE = {"zero": "division by zero", "scale": "scale above 18", "other": "precondition"}
NOT_TRACED = {"high_word", "either"}             # no Python counterpart: a word helper, and the converters

NIM_RULES = """`LET` is `let`, or `var` for an array made by `array(n)`, which is changed in place; `IF c THEN s` is
`if c: s`; `RETURN` is `return`; `SET A[i] = v` is `A[i] = v`; `DO f(x)` is `f(x)`; `·` is `*`, `−` is `-`, `/` is `div`,
`=` in a condition is `==` and `≠` is `!=`; `AND` and `OR` are `and` and `or`, both between conditions and, for `AND`,
between integers; `>>` and `<<` are `shr` and `shl`; `mod(u, v)` is the call `` `mod`(u, v) `` of the paper's own mod;
`ERROR` raises ValueError("division by zero") for a zero divisor, ValueError("scale above 18") for a scale above 18, and
ValueError("precondition") otherwise; names are kept, `b′`, `s₊`, `k₁` and `φ` among them; home's two word lines are
WORDS; indentation is two spaces a level."""


# ----------------------------------------------------------------------------------------------- reading the paper
def extract(path):
    """{part: (first line, code)} for A, B.1 and B.2: the ```nim blocks under the two appendix headings."""
    lines = path.read_text(encoding="utf-8").split("\n")
    out = {}
    for key, heading in HEADINGS.items():
        starts = [i for i, l in enumerate(lines) if l.strip() == heading]
        if len(starts) != 1:
            sys.exit(f"{path.name}: expected one heading {heading!r}, found {len(starts)}")
        end = next((i for i in range(starts[0] + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
        blocks, i = [], starts[0] + 1
        while i < end:
            if lines[i].strip() == "```nim":
                j = next(k for k in range(i + 1, end) if lines[k].strip() == "```")
                blocks.append((i + 2, "\n".join(lines[i + 1:j])))
                i = j
            elif lines[i].strip().startswith("```python"):
                sys.exit(f"{path.name}: {heading} still has a python block")
            i += 1
        want = 1 if key == "A" else 2
        if len(blocks) != want:
            sys.exit(f"{path.name}: expected {want} nim block(s) under {heading!r}, found {len(blocks)}")
        if key == "A":
            out["A"] = blocks[0]
        else:
            out["B.1"], out["B.2"] = blocks
    return out


def routines(code):
    """[(name, header line index, end index)] of the top-level procs and converters of a block, in order."""
    lines = code.split("\n")
    heads = [i for i, l in enumerate(lines) if l.startswith(("proc ", "converter "))]
    out = []
    for n, i in enumerate(heads):
        j = heads[n + 1] if n + 1 < len(heads) else len(lines)
        while j > i and not lines[j - 1].strip():
            j -= 1
        name = re.match(r"(?:proc|converter) `?(\w+)`?", lines[i]).group(1)
        out.append((name, i, j))
    return out


# ------------------------------------------------------------------------------------------- the Nim translation
def nim_expr(e):
    for k, v in WORDS.items():
        e = e.replace(k, v)
    e = e.replace("·", "*").replace("−", "-")
    e = re.sub(r"\s*/\s*", " div ", e)
    e = e.replace(">>", "shr").replace("<<", "shl").replace(" AND ", " and ").replace(" OR ", " or ")
    e = re.sub(r"\bmod\(", "`mod`(", e)
    return e


def nim_cond(c):
    return nim_expr(c).replace("≠", "!=").replace(" = ", " == ")


def nim_error(c):
    kind = "zero" if "numerator(y) = 0" in c else "scale" if "> 18" in c else "other"
    return f'raise newException(ValueError, "{ERROR_MESSAGE[kind]}")'


def nim_statement(s, line):
    if s == "RETURN":
        return "return"
    if s.startswith("RETURN "):
        return "return " + nim_expr(s[7:])
    if s.startswith("SET "):
        name, value = s[4:].split(" = ", 1)
        return nim_expr(name) + " = " + nim_expr(value)
    if s.startswith("DO "):
        return nim_expr(s[3:])
    raise ValueError(f"untranslatable pseudocode line: {line!r}")


def nim_translate(name, lines):
    out = [SIGNATURE[name]]
    for line in lines[1:]:
        level = (len(line) - len(line.lstrip())) // 4
        ind, s = INDENT * level, line.strip()
        if s.startswith("LET "):
            var, value = s[4:].split(" = ", 1)
            kw = "var" if value.startswith("array(") else "let"
            out.append(f"{ind}{kw} {nim_expr(var)} = {nim_expr(value)}")
        elif s.startswith("IF "):
            c, then = s[3:].split(" THEN", 1)
            then = then.strip()
            head = f"{ind}if {nim_cond(c)}:"
            if not then:
                out.append(head)
            elif then == "ERROR":
                out.append(f"{head} {nim_error(c)}")
            else:
                out.append(f"{head} {nim_statement(then, line)}")
        else:
            out.append(ind + nim_statement(s, line))
    return "\n".join(out)


def expected(part, pseudo):
    """The text the part must be: (head, [(name, text, where)]); where is 'prose' or the pseudocode's line count."""
    funcs = []
    for name in ORDER[part]:
        if name in PROSE[part]:
            funcs.append((name, PROSE[part][name], "prose"))
        elif name in pseudo:
            funcs.append((name, nim_translate(name, pseudo[name]), f"{len(pseudo[name])} lines of pseudocode"))
        else:
            funcs.append((name, None, "no pseudocode found"))
    return HEAD[part], funcs


def expected_text(part, pseudo):
    head, funcs = expected(part, pseudo)
    return "\n\n".join([head] + [t or "" for _, t, _ in funcs])


def check_literal(blocks, pseudo, check):
    """Each part is its head and then exactly the translation of each function, line for line."""
    print("Line for line against the pseudocode (Nim; the rules are nimlib.NIM_RULES):")
    for part in ("A", "B.1", "B.2"):
        first, code = blocks[part]
        head, funcs = expected(part, pseudo[part])
        got = [l.rstrip() for l in code.split("\n")]
        rs = routines(code)
        names = [n for n, _, _ in rs if n != "either"]
        if not check(names == ORDER[part], f"Appendix {part}: defines {names}, expected {ORDER[part]}"):
            continue
        first_proc = next(i for _, i, _ in rs if got[i].startswith("proc "))
        got_head = "\n".join(got[:first_proc]).rstrip()
        check(got_head == head, f"Appendix {part}: the head (types and constants) is not the fixed one")
        print(f"  Appendix {part} (from line {first} of the paper): head {'identical' if got_head == head else 'DIFFERS'}")
        k, all_same = 0, got_head == head
        for name, i, j in rs:
            if name == "either":
                continue
            mine = "\n".join(got[i:j])
            want_name, want, where = funcs[k]
            k += 1
            ok = check(want is not None and mine == want, f"Appendix {part}, {name}: not the translation of its "
                                                          f"{'prose definition' if where == 'prose' else 'pseudocode'}")
            all_same = all_same and ok
            if not ok and want is not None:
                for a, b in zip(mine.split("\n") + [""] * 99, want.split("\n") + [""] * 99):
                    if a != b:
                        print(f"    appendix:    {a}\n    translation: {b}")
                        break
            print(f"    {name:20s} {'identical' if ok else 'DIFFERS'} ({where}{', fixed' if where == 'prose' else ''})")
        whole = "\n".join(got).rstrip()
        if all_same:
            check(whole == expected_text(part, pseudo[part]), f"Appendix {part}: the blank lines between the functions "
                                                               f"are not one each")
    print()


# --------------------------------------------------------------------------------------------- signatures, kinds
def parse_signature(header):
    """('name', generic?, [(param, type, var?)], return type or None) of a `proc` header."""
    m = re.match(r"proc `?(\w+)`?(\[V\])?\((.*)\)(?:: (.+?))? =$", header)
    name, generic, params, ret = m.group(1), bool(m.group(2)), m.group(3), m.group(4)
    groups, depth, cur = [], 0, ""
    for ch in params:
        depth += ch == "["
        depth -= ch == "]"
        if ch == "," and depth == 0:
            groups.append(cur.strip())
            cur = ""
        else:
            cur += ch
    groups.append(cur.strip())
    out, pending = [], []
    for g in groups:
        if ":" in g:
            n, t = (x.strip() for x in g.split(":", 1))
            is_var = t.startswith("var ")
            t = t[4:] if is_var else t
            t = t[5:] if t.startswith("sink ") else t
            for p in pending + [n]:
                out.append((p, t, is_var))
            pending = []
        else:
            pending.append(g)
    return name, generic, out, ret


def entries_for(blocks):
    """Every callable instance of the Nim library: {key: (name, [(param, type, var?)], return type)}; a generic
    function is instantiated for lists of integers and of stored values."""
    out = {}
    for part in ("A", "B.1", "B.2"):
        for line in blocks[part][1].split("\n"):
            if not line.startswith("proc "):
                continue
            name, generic, params, ret = parse_signature(line)
            if name in NOT_TRACED:
                continue
            for V in (("int64", "Rational") if generic else (None,)):
                sub = (lambda t: t.replace("List[V]", f"List[{V}]") if t == "List[V]" else V if t == "V" else t) \
                    if V else (lambda t: t)
                ps = [(p, sub(t), v) for p, t, v in params]
                key = name if not generic else f"{name}.{V}"
                if key in out:                      # an overload: numerator of a decimal
                    key = f"{name}.{ps[0][1]}"
                out[key] = (name, ps, sub(ret) if ret else None)
    return out


class Unrepresentable(Exception):
    """An argument that Nim's types cannot hold (an integer beyond 64 bits, a section 5.1 row, ...)."""


def kind(v):
    if v is None:
        return "List"
    if type(v) is bool:
        raise Unrepresentable("bool")
    if isinstance(v, int):
        return "int64"
    if isinstance(v, list):
        return "seq[int64]"
    t = type(v).__name__
    if t in ("Quantity", "Decimal", "Column", "Matrix"):
        return t
    if isinstance(v, tuple) and len(v) == 2 and t == "tuple":
        if isinstance(v[0], int) and isinstance(v[1], int):
            return "Rational"
        if v[1] is None or (isinstance(v[1], tuple) and type(v[1]).__name__ == "tuple"):
            return "List[%s]" % kind(v[0])
    raise Unrepresentable(t)


def fits(t, k):
    return t == k or (k == "List" and t.startswith("List["))


def ser_int(v):
    if not INT_MIN <= v <= INT_MAX:
        raise Unrepresentable("an integer beyond 64 bits")
    return str(v)


def ser(v):
    """A value in the exchange format (the driver's parse and ser are its Nim side)."""
    if v is None:
        return "L 0"
    if type(v) is bool:
        raise Unrepresentable("bool")
    if isinstance(v, int):
        return ser_int(v)
    if isinstance(v, list):
        return " ".join(["A", str(len(v))] + ["_" if x is None else ser_int(x) for x in v])
    t = type(v).__name__
    if t == "Quantity":
        if not 0 <= v[1] <= 255:
            raise Unrepresentable("a direction beyond 8 bits")
        return f"Q {ser_int(v[0])} {v[1]}"
    if t == "Decimal":
        if not 0 <= v[1] <= 255:
            raise Unrepresentable("a scale beyond 8 bits")
        return f"D {ser_int(v[0])} {v[1]}"
    if t == "Column":
        return f"C {ser_int(v[0])} {ser(v[1])}"
    if t == "Matrix":
        return f"M {ser_int(v[0])} {ser_int(v[1])} {ser_int(v[2])} {ser(v[3])} {ser(v[4])}"
    if t == "tuple" and len(v) == 2:
        if isinstance(v[0], int) and isinstance(v[1], int):
            return f"R {ser_int(v[0])} {ser_int(v[1])}"
        items, L = [], v
        while L is not None:
            items.append(ser(L[0]))
            L = L[1]
        return " ".join(["L", str(len(items))] + items)
    raise Unrepresentable(t)


def ser_result(v, ret):
    if ret is None:
        return "V"
    if v is None and ret.startswith("List"):
        return "L 0"
    return ser(v)


def error_text(e):
    if isinstance(e, ZeroDivisionError):
        return "E " + ERROR_MESSAGE["zero"]
    if isinstance(e, OverflowError):
        return "E " + ERROR_MESSAGE["scale"]
    return "E " + ERROR_MESSAGE["other"]


class Parser:
    """The exchange format back into Python values, with the integers and kinds of pair of a given namespace."""

    def __init__(self, ns, integer=int):
        self.ns, self.int = ns, integer

    def parse(self, text):
        self.toks, self.i = text.split(), 0
        out = []
        while self.i < len(self.toks):
            out.append(self.value())
        return out

    def tok(self):
        self.i += 1
        return self.toks[self.i - 1]

    def value(self):
        t = self.tok()
        I = self.int
        if t == "R":
            return (I(int(self.tok())), I(int(self.tok())))
        if t == "Q":
            return self.ns["Quantity"](I(int(self.tok())), I(int(self.tok())))
        if t == "D":
            return self.ns["Decimal"](I(int(self.tok())), I(int(self.tok())))
        if t == "C":
            s = I(int(self.tok()))
            return self.ns["Column"](s, self.value())
        if t == "L":
            items = [self.value() for _ in range(int(self.tok()))]
            L = None
            for x in reversed(items):
                L = (x, L)
            return L
        if t == "A":
            return [None if x == "_" else I(int(x)) for x in (self.tok() for _ in range(int(self.tok())))]
        if t == "M":
            r, c, w = (I(int(self.tok())) for _ in range(3))
            return self.ns["Matrix"](r, c, w, self.value(), self.value())
        return I(int(t))


# ------------------------------------------------------------------------------------------------ the recorder
class Recorder:
    """Wraps the Python reference's functions for the checks: every call the checks make (never the library's calls of
    its own functions) is recorded with its arguments and result, in the exchange format."""

    def __init__(self, entries):
        self.by_name = defaultdict(list)
        for key, (name, ps, ret) in entries.items():
            self.by_name[name].append((key, ps, ret))
        self.calls = []                 # (key, args text, expected text, traced?)
        self.skipped = Counter()        # name -> calls Nim's types cannot hold
        self.trace_all = False          # set around the paper's examples: every call is traced

    def wrap(self, name, fn):
        nim_name = "sum" if name == "sum_" else name
        rec = self

        def wrapper(*args):
            try:
                kinds = [kind(a) for a in args]
                entry = next(((k, ps, ret) for k, ps, ret in rec.by_name[nim_name]
                              if len(ps) == len(args) and all(fits(t, kk) for (_, t, _), kk in zip(ps, kinds))), None)
                if entry is None:
                    raise Unrepresentable("no Nim signature")
                before = " ".join(ser(a) for a in args)
            except Unrepresentable:
                rec.skipped[nim_name] += 1
                return fn(*args)
            key, ps, ret = entry
            try:
                r = fn(*args)
            except (ZeroDivisionError, OverflowError, ValueError) as e:
                rec.calls.append((key, before, error_text(e), False))
                raise
            try:
                out = [ser_result(r, ret)] + [ser(a) for a, (_, _, v) in zip(args, ps) if v]
                rec.calls.append((key, before, " | ".join(out), "example" if rec.trace_all else False))
            except Unrepresentable:
                rec.calls.append((key, before, "BIG", False))     # a result beyond 64 bits: Nim must overflow
            return r

        wrapper.__name__ = name
        wrapper.__wrapped__ = fn
        return wrapper

    def api(self, ns, names):
        """A copy of ns for the checks, with the library's functions in names wrapped."""
        out = dict(ns)
        for n in names:
            out[n] = self.wrap(n, ns[n])
        return out


RECORDER = None


class traced:
    """Every call the checks make inside this block is traced in both languages (the paper's examples)."""

    def __enter__(self):
        if RECORDER is not None:
            RECORDER.trace_all = True

    def __exit__(self, *a):
        if RECORDER is not None:
            RECORDER.trace_all = False


# ---------------------------------------------------------------------------------------------- instrumentation
def split_condition(rest):
    """`cond: tail` -> (cond, tail): the colon that ends the condition is the first one outside brackets."""
    depth = 0
    for i, ch in enumerate(rest):
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == ":" and depth == 0:
            return rest[:i], rest[i + 1:]
    raise ValueError(rest)


def or_terms(cond):
    depth, terms, cur = 0, [], ""
    i = 0
    while i < len(cond):
        ch = cond[i]
        depth += ch in "(["
        depth -= ch in ")]"
        if depth == 0 and cond.startswith(" or ", i):
            terms.append(cur)
            cur, i = "", i + 4
            continue
        cur += ch
        i += 1
    return terms + [cur] if terms else []


def instrument(blocks):
    """The instrumented library (coverage and call traces, with the exchange format's ser included after each part's
    types), and the table of what it counts: {id: (part, paper line, function, kind, text, offset in function)}."""
    out = ["include rt_cov"]
    info = {}
    nid = 0
    for part, include in (("A", "ser_a"), ("B.1", "ser_b1"), ("B.2", "ser_b2")):
        first, code = blocks[part]
        lines = code.split("\n")
        rs = routines(code)
        head_end = rs[0][1] if rs else len(lines)
        # the head, up to the converters, which are routines and counted
        conv = [r for r in rs if r[0] == "either"]
        stop = conv[0][1] if conv else head_end
        out += lines[:stop]
        out.append(f"include {include}")
        idx = stop
        for name, i, j in rs:
            out += lines[idx:i]                                   # blank lines between routines
            header = lines[i]
            out.append(header)
            if header.startswith("proc ") and name not in NOT_TRACED:
                _, _, params, ret = parse_signature(header)
                args = ", ".join(f"ser({p})" for p, _, _ in params) or ""
                out.append(f'  if tracing: trace_call("{name}", @[{args}])')
                out.append("  defer: trace_ret(result)" if ret else "  defer: trace_ret_void()")
            for k in range(i + 1, j):
                line = lines[k]
                if not line.strip():
                    out.append(line)
                    continue
                ind = line[:len(line) - len(line.lstrip())]
                s = line.strip()
                paper = first + k
                if s.startswith("if "):
                    cond, tail = split_condition(s[3:])
                    nid += 1
                    cid = nid
                    terms = or_terms(cond)
                    info[cid] = (part, paper, name, "if", cond, k - i)
                    if terms:
                        tids = []
                        for t in terms:
                            nid += 1
                            info[nid] = (part, paper, name, "term", t.strip(), k - i)
                            tids.append(nid)
                        cond2 = f"terms(@[{', '.join(map(str, tids))}], @[{', '.join(terms)}])"
                    else:
                        cond2 = cond
                    out.append(f"{ind}if cov({cid}, {cond2}):{tail}")
                else:
                    nid += 1
                    info[nid] = (part, paper, name, "stmt", s, k - i)
                    out.append(f"{ind}mark({nid})")
                    out.append(line)
            idx = j
        out += lines[idx:]
        out.append("")
    return f"const NCOV = {nid + 1}\n" + "\n".join(out) + "\n", info


# ---------------------------------------------------------------------------------------------- the driver
def nim_parse_call(t):
    return {"int64": "p.int64v()", "Quantity": "p.quantityv()", "Rational": "p.rationalv()", "Decimal": "p.decimalv()", "Column": "p.columnv()",
            "List[int64]": "p.listv(int64)", "List[Rational]": "p.listv(Rational)", "seq[int64]": "p.seqv()",
            "Matrix": "p.matrixv()"}[t]


def driver(entries):
    """The dispatch: one case per callable instance, which parses its arguments, calls it and writes the result,
    then the arrays it changed."""
    out = ["proc dispatch(key: string, p: var Parser): string =", "  case key"]
    for key, (name, ps, ret) in sorted(entries.items()):
        out.append(f'  of "{key}":')
        for i, (pn, t, v) in enumerate(ps):          # numbered: Nim ignores underscores, so a_s would be `as`
            out.append(f"    var arg{i} = {nim_parse_call(t)}")
        call = ("`mod`" if name == "mod" else name) + "(" + ", ".join(f"arg{i}" for i in range(len(ps))) + ")"
        after = "".join(f' & " | " & ser(arg{i})' for i, (pn, _, v) in enumerate(ps) if v)
        if ret:
            out.append(f"    result = ser({call}){after}")
        else:
            out.append(f"    {call}")
            out.append(f'    result = "V"{after}')
    out.append('  else: result = "E unknown " & key')
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------------------------- the int64 model
class Int64Overflow(Exception):
    pass


class InexactDivision(Exception):
    pass


def _c(v):
    v = int(v)
    if not INT_MIN <= v <= INT_MAX:
        raise Int64Overflow
    return I64(v)


class I64(int):
    """An integer of the int64 model: every result must fit 64 bits, and every `/` must be exact."""
    __slots__ = ()

    def __new__(cls, v):
        if not INT_MIN <= v <= INT_MAX:
            raise Int64Overflow
        return int.__new__(cls, v)

    def _op(f):
        def op(a, b):
            if not isinstance(b, int):
                return NotImplemented        # a list's repetition, [unused] * n
            return _c(f(int(a), int(b)))
        return op

    __add__ = __radd__ = _op(lambda a, b: a + b)
    __sub__ = _op(lambda a, b: a - b)
    __rsub__ = _op(lambda a, b: b - a)
    __mul__ = __rmul__ = _op(lambda a, b: a * b)
    __and__ = __rand__ = _op(lambda a, b: a & b)
    __rshift__ = _op(lambda a, b: a >> b)
    __lshift__ = _op(lambda a, b: a << b)

    def __neg__(a):
        return _c(-int(a))

    def __floordiv__(a, b):
        if not isinstance(b, int):
            return NotImplemented
        if int(a) % int(b):
            raise InexactDivision(f"{int(a)} / {int(b)}")
        return _c(int(a) // int(b))

    def __rfloordiv__(a, b):
        return I64.__floordiv__(I64(b), a)


class _Literals(ast.NodeTransformer):
    def visit_Constant(self, node):
        if type(node.value) is int:
            return ast.Call(ast.Name("I64", ast.Load()), [node], [])
        return node


def model_namespace(parts):
    """The int64 model: the Python reference's code, every integer an I64, and home on 64-bit words as Nim's is."""
    ns = {"I64": I64}
    for name, code, filename, head_lines in parts:
        tree = ast.parse(code)
        body = tree.body[head_lines:]
        tree.body = tree.body[:head_lines] + [_Literals().visit(n) for n in body]
        ast.fix_missing_locations(tree)
        exec(compile(tree, filename + " (int64 model)", "exec"), ns)

    def home(v, n):
        if v < 0:
            return home(-v, n)
        w = (int(v) * PHI) & MASK64
        return I64((w * int(n)) >> 64)

    ns["home"] = home
    return ns


# ---------------------------------------------------------------------------------------------- building and running
RT_COV = r'''
var tracing* = false
var covT = newSeq[int](NCOV)
var covF = newSeq[int](NCOV)
var depth = 0
var traceOut: seq[string]

proc cov(id: int, b: bool): bool =
  if b: inc covT[id] else: inc covF[id]
  if tracing: traceOut.add("#B " & $id & " " & (if b: "1" else: "0"))
  return b

proc terms(ids: seq[int], ts: seq[bool]): bool =
  result = false
  for i, t in ts:
    if t:
      inc covT[ids[i]]
      result = true

proc mark(id: int) =
  inc covT[id]

proc trace_call(name: string, args: seq[string]) =
  var s = "#C " & $depth & " " & name
  for a in args: s.add(" ; " & a)
  traceOut.add(s)
  inc depth

template trace_ret(x: untyped) =
  if tracing:
    dec depth
    traceOut.add("#R " & $depth & " " & ser(x))

template trace_ret_void() =
  if tracing:
    dec depth
    traceOut.add("#R " & $depth & " V")
'''

SER_A = r'''
proc ser(x: int64): string = $x
proc ser(x: Quantity): string = "Q " & $x.m & " " & $x.s
proc ser(x: Rational): string = "R " & $x.p & " " & $x.q
'''
SER_B1 = r'''
proc ser[V](L: List[V]): string =
  var items: seq[string]
  var n = L
  while n != nil:
    items.add(ser(n.head))
    n = n.tail
  result = "L " & $items.len
  for s in items: result.add(" " & s)
proc ser(x: Decimal): string = "D " & $x.u & " " & $x.k
proc ser(x: Column): string = "C " & $x.s & " " & ser(x.L)
proc ser(x: DecimalOrRational): string = (if x.is_decimal: ser(x.d) else: ser(x.r))
'''
SER_B2 = r'''
proc ser(A: seq[int64]): string =
  result = "A " & $A.len
  for x in A: result.add(if x == unused: " _" else: " " & $x)
proc ser(M: Matrix): string = "M " & $M.r & " " & $M.c & " " & $M.w & " " & ser(M.H) & " " & ser(M.L)
'''
PARSE = r'''
import std/strutils

type Parser = object
  toks: seq[string]
  i: int

proc tok(p: var Parser): string =
  result = p.toks[p.i]
  inc p.i

proc int64v(p: var Parser): int64 = parseBiggestInt(p.tok())
proc rationalv(p: var Parser): Rational =
  doAssert p.tok() == "R"
  let a = p.int64v()
  let b = p.int64v()
  return Rational(p: a, q: b)
proc quantityv(p: var Parser): Quantity =
  doAssert p.tok() == "Q"
  let m = p.int64v()
  let s = p.int64v()
  return Quantity(m: m, s: uint8(s))
proc decimalv(p: var Parser): Decimal =
  doAssert p.tok() == "D"
  let u = p.int64v()
  let k = p.int64v()
  return Decimal(u: u, k: uint8(k))
proc listv(p: var Parser, V: typedesc): List[V] =
  doAssert p.tok() == "L"
  let n = parseInt(p.tok())
  var items: seq[V]
  for i in 0 ..< n:
    when V is int64: items.add(p.int64v()) else: items.add(p.rationalv())
  var L: List[V] = nil                 # (not `result = List[V](..., tail: result)`, which ARC makes a cycle)
  for i in countdown(n - 1, 0):
    let tail = L
    L = List[V](head: items[i], tail: tail)
  return L
proc columnv(p: var Parser): Column =
  doAssert p.tok() == "C"
  let s = p.int64v()
  return Column(s: s, L: p.listv(int64))
proc seqv(p: var Parser): seq[int64] =
  doAssert p.tok() == "A"
  let n = parseInt(p.tok())
  for i in 0 ..< n:
    let t = p.tok()
    result.add(if t == "_": unused else: parseBiggestInt(t))
proc matrixv(p: var Parser): Matrix =
  doAssert p.tok() == "M"
  let r = p.int64v()
  let c = p.int64v()
  let w = p.int64v()
  let H = p.seqv()
  let L = p.seqv()
  return Matrix(r: r, c: c, w: w, H: H, L: L)
'''
MAIN = r'''
proc main() =
  let input = open(paramStr(1))
  let output = open(paramStr(2), fmWrite)
  var line: string
  while input.readLine(line):
    let f = line.split(' ', 3)
    var p = Parser(toks: (if f.len > 3: f[3].split(' ') else: @[]), i: 0)
    var res: string
    when defined(instrumented):
      tracing = f[1] == "T"
      depth = 0
      traceOut.setLen(0)
    try:
      res = dispatch(f[2], p)
    except ValueError as e:
      res = "E " & e.msg
    except OverflowDefect:
      res = "E overflow"
    except Defect as e:
      res = "E defect " & $e.name & ": " & e.msg
    when defined(instrumented):
      tracing = false
      for s in traceOut: output.writeLine(f[0] & " " & s)
    output.writeLine(f[0] & " " & res)
  when defined(instrumented):
    for i in 1 ..< NCOV: output.writeLine("COV " & $i & " " & $covT[i] & " " & $covF[i])
  output.close()

main()
'''


def find_nim(explicit):
    for cand in (explicit, os.environ.get("NIM"), shutil.which("nim")):
        if cand and Path(cand).is_file():
            return cand
    return None


def compile_nim(nim, src, exe, cache, defines=()):
    cmd = [nim, "c", "--hints:off", "--warnings:off", "-d:release", "--mm:arc", "--overflowChecks:on",
           "--rangeChecks:on", "--boundChecks:on",
           *[f"-d:{d}" for d in defines], f"--nimcache:{cache}", f"-o:{exe}", str(src)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.returncode == 0, (r.stdout + r.stderr)[-3000:], cmd


# ---------------------------------------------------------------------------------------------- the checks
def canonical_tree(events):
    """Call events -> a canonical text: each frame as `name(args) -> result`, its `if` outcomes in order, and its
    children, indented. The order of outcomes relative to the calls a condition makes is not compared."""
    out = []
    stack = []

    def close():
        f = stack.pop()
        lines = [f"{'  ' * f['depth']}{f['name']}({' ; '.join(f['args'])}) -> {f['result']}"
                 f"  [{' '.join(f['branches'])}]"] + f["children"]
        if stack:
            stack[-1]["children"].extend(lines)
        else:
            out.extend(lines)

    for ev in events:
        if ev[0] == "C":
            stack.append({"depth": len(stack), "name": ev[1], "args": ev[2], "branches": [], "children": [],
                          "result": None})
        elif ev[0] == "B":
            stack[-1]["branches"].append(ev[1])
        elif ev[0] == "R":
            stack[-1]["result"] = ev[1]
            close()
    return "\n".join(out)


class PyTracer:
    """The Python reference's call trees, in the Nim trace's terms: the frames of the reference's code, their
    arguments and results in the exchange format, and the outcome of each `if` (by function and line offset)."""

    def __init__(self, sources):
        self.files = {}          # filename -> {line: (function, offset, compiled condition or None)}
        for filename, code in sources:
            tree = ast.parse(code)
            table = {}
            for fn in tree.body:
                if not isinstance(fn, ast.FunctionDef):
                    continue
                for node in ast.walk(fn):
                    if isinstance(node, ast.If):
                        src = ast.get_source_segment(code, node.test)
                        table[node.lineno] = ("sum" if fn.name == "sum_" else fn.name, node.lineno - fn.lineno,
                                              compile(src, "<cond>", "eval"))
            self.files[filename] = table

    def run(self, fn, args):
        events = []

        def glob(frame, event, arg):
            if event != "call" or frame.f_code.co_filename not in self.files:
                return None
            table = self.files[frame.f_code.co_filename]
            co = frame.f_code
            name = "sum" if co.co_name == "sum_" else co.co_name
            events.append(("C", name, [ser(frame.f_locals[v]) for v in co.co_varnames[:co.co_argcount]]))
            ret_kind = RET_KINDS.get(name)

            def local(frame, event, arg):
                if event == "line" and frame.f_lineno in table:
                    fname, off, c = table[frame.f_lineno]
                    env = dict(frame.f_globals)
                    env.update(frame.f_locals)
                    events.append(("B", f"{fname}:{off}={int(bool(eval(c, env)))}"))
                elif event == "return":
                    events.append(("R", ser_result(arg, ret_kind)))
                return local
            return local

        sys.settrace(glob)
        try:
            fn(*args)
        finally:
            sys.settrace(None)
        return canonical_tree(events)


RET_KINDS = {}


def run(path, blocks, pseudo, py_parts, lib_ns, check, nim):
    """Compile the paper's Nim, replay RECORDER's calls, and report. py_parts: [(name, code, filename, head
    lines)] of the Python reference; lib_ns: its namespace (the functions themselves, unwrapped)."""
    rec = RECORDER
    entries = entries_for(blocks)
    ENTRIES_CACHE.update(entries)
    for key, (name, ps, ret) in entries.items():
        RET_KINDS.setdefault(name, ret)
    print(f"\nThe Nim appendices, compiled and run ({nim}):")
    ver = subprocess.run([nim, "--version"], capture_output=True, text=True).stdout.split("\n")
    git = next((l.split(":", 1)[1].strip() for l in ver if l.startswith("git hash")), "?")
    import hashlib
    sha = hashlib.sha256(Path(nim).read_bytes()).hexdigest()
    pinned = git == PINNED["git"] and sha == PINNED["sha256"]
    print(f"  {ver[0]}; git hash {git}; sha256 {sha[:16]}...: "
          + ("the pinned build" if pinned else f"NOT the pinned build (Nim {PINNED['version']}, {PINNED['git'][:7]}, "
                                                f"sha256 {PINNED['sha256'][:16]}...; see README.md)"))
    keep = os.environ.get("NIMAPPX_KEEP")          # a directory to keep the build in, for debugging the harness
    with tempfile.TemporaryDirectory(prefix="nimappx-") as t:
        t = Path(keep) if keep else Path(t)
        t.mkdir(parents=True, exist_ok=True)
        lib = "\n\n".join(blocks[p][1] for p in ("A", "B.1", "B.2")) + "\n"
        (t / "lib.nim").write_text(lib, encoding="utf-8")
        cov_lib, info = instrument(blocks)
        (t / "lib_cov.nim").write_text(cov_lib, encoding="utf-8")
        (t / "rt_cov.nim").write_text(RT_COV, encoding="utf-8")
        for n, s in (("ser_a", SER_A), ("ser_b1", SER_B1), ("ser_b2", SER_B2), ("parse", PARSE)):
            (t / f"{n}.nim").write_text(s, encoding="utf-8")
        disp = driver(entries)
        (t / "driver.nim").write_text("include lib\ninclude ser_a\ninclude ser_b1\ninclude ser_b2\ninclude parse\n"
                                      "import std/os\n" + disp + MAIN, encoding="utf-8")
        (t / "driver_cov.nim").write_text("include lib_cov\ninclude parse\nimport std/os\n" + disp + MAIN,
                                          encoding="utf-8")
        t0 = time.time()
        ok1, log1, cmd = compile_nim(nim, t / "driver.nim", t / "driver", t / "cache")
        if not check(ok1, f"Nim: the paper's code does not compile:\n{log1}"):
            return
        ok2, log2, _ = compile_nim(nim, t / "driver_cov.nim", t / "driver_cov", t / "cache_cov", ["instrumented"])
        if not check(ok2, f"Nim: the instrumented code does not compile:\n{log2}"):
            return
        print(f"  compiled the paper's code as it is, and an instrumented copy, in {time.time() - t0:.0f} s "
              f"(nim c -d:release --mm:arc: overflow, range and index checks on)")

        # calls: sample one in 97 of the small, error-free ones for tracing, besides the paper's examples
        lines, traced = [], 0
        for n, (key, args, want, tr) in enumerate(rec.calls):
            if not tr and n % 97 == 0 and len(args) < 400 and not want.startswith(("E", "BIG")):
                tr = "sample"
            if tr:
                traced += 1
                rec.calls[n] = (key, args, want, tr)
            lines.append(f"{n} {'T' if tr else '-'} {key} {args}")
        (t / "calls.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
        t0 = time.time()
        for exe, out in (("driver", "out.txt"), ("driver_cov", "out_cov.txt")):
            r = subprocess.run([str(t / exe), str(t / "calls.txt"), str(t / out)], capture_output=True, text=True)
            if not check(r.returncode == 0, f"Nim: {exe} failed ({r.returncode}): {r.stderr[-2000:]}"
                                            f"{r.stdout[-2000:]}"):
                return
        print(f"  replayed {len(rec.calls)} calls in both builds in {time.time() - t0:.0f} s; "
              f"{sum(rec.skipped.values())} calls not sent, since Nim's 64-bit types cannot hold their arguments")
        results = {}
        for line in (t / "out.txt").read_text(encoding="utf-8").split("\n"):
            if line:
                n, res = line.split(" ", 1)
                results[int(n)] = res
        cov_results, traces, covcounts = {}, defaultdict(list), {}
        for line in (t / "out_cov.txt").read_text(encoding="utf-8").split("\n"):
            if not line:
                continue
            if line.startswith("COV "):
                _, i, tt, ff = line.split()
                covcounts[int(i)] = (int(tt), int(ff))
                continue
            n, res = line.split(" ", 1)
            if res.startswith("#"):
                traces[int(n)].append(res)
            else:
                cov_results[int(n)] = res
        compare_values(rec, results, cov_results, py_parts, lib_ns, check)
        compare_traces(rec, traces, results, info, py_parts, lib_ns, check)
        coverage(info, covcounts, check)


def compare_values(rec, results, cov_results, py_parts, lib_ns, check):
    model = model_namespace(py_parts)
    mparse = Parser(model, I64)
    stats = defaultdict(Counter)
    bad = 0
    for n, (key, args, want, _) in enumerate(rec.calls):
        got = results.get(n)
        name = key.split(".")[0]
        s = stats[name]
        if got != cov_results.get(n):
            check(False, f"Nim call {n} {key}({args[:200]}): the instrumented build gives {cov_results.get(n)!r}, "
                         f"the paper's code {got!r}")
        # the int64 model on every call: it must agree with Nim, and with the reference where it does not overflow
        try:
            margs = mparse.parse(args)
            mres = model[("sum_" if name == "sum" else name)](*margs)
            entry = RET_KINDS.get(name)
            mout = ser_result(mres, entry)
            var_after = [ser(a) for a, (_, _, v) in zip(margs, ENTRIES_CACHE[key][1]) if v]
            model_out = " | ".join([mout] + var_after)
        except Int64Overflow:
            model_out = "E overflow"
        except InexactDivision as e:
            model_out = f"E inexact division {e}"
            check(False, f"{key}({args[:200]}): a `/` that is not exact: {e}")
        except (ZeroDivisionError, OverflowError, ValueError) as e:
            model_out = error_text(e)
        if got == want:
            s["equal" if not want.startswith("E") else "equal (ERROR)"] += 1
            ok = model_out == want
        elif got == "E overflow" and model_out == "E overflow":
            s["int64 overflow, as the model's"] += 1
            ok = True
        else:
            ok = False
        if not ok:
            bad += 1
            check(False, f"Nim {key}({args[:300]}) = {got!r}; reference {want!r}; int64 model {model_out!r}")
    print("  Every call against the Python reference (value for value) and the int64 model (the same pseudocode with "
          "every integer held to 64 bits):")
    for name in sorted(stats):
        s = stats[name]
        print(f"    {name:20s} " + ", ".join(f"{v} {k}" for k, v in sorted(s.items())))
    tot = Counter()
    for s in stats.values():
        tot.update(s)
    print(f"    {'all':20s} " + ", ".join(f"{v} {k}" for k, v in sorted(tot.items()))
          + f"; {bad} differ")
    if rec.skipped:
        print("    not sent (arguments beyond Nim's 64-bit types): "
              + ", ".join(f"{k} {v}" for k, v in sorted(rec.skipped.items())))


ENTRIES_CACHE = {}


def compare_traces(rec, traces, results, info, py_parts, lib_ns, check):
    """The traced calls that complete in both languages (not an ERROR, nor an int64 overflow in Nim)."""
    tracer = PyTracer([(filename, code) for _, code, filename, _ in py_parts])
    parse = Parser(lib_ns)
    n_ok, n_all, frames = Counter(), Counter(), 0
    for n, (key, args, want, tr) in enumerate(rec.calls):
        if not tr or want.startswith(("E", "BIG")) or results.get(n, "E").startswith("E"):
            continue
        n_all[tr] += 1
        events = []
        for ev in traces.get(n, []):
            kind_, rest = ev[1], ev[3:]
            if kind_ == "C":
                parts = rest.split(" ; ")
                _, name = parts[0].split(" ", 1)
                events.append(("C", name, parts[1:]))
            elif kind_ == "B":
                i, o = rest.split()
                part, paper, fname, _, _, off = info[int(i)]
                events.append(("B", f"{fname}:{off}={o}"))
            elif kind_ == "R":
                _, res = rest.split(" ", 1)
                events.append(("R", res))
        nim_tree = canonical_tree(events)
        name = key.split(".")[0]
        py_tree = tracer.run(lib_ns["sum_" if name == "sum" else name], parse.parse(args))
        same = nim_tree == py_tree
        n_ok[tr] += same
        frames += nim_tree.count("\n") + 1
        if not check(same, f"Nim call {key}({args[:200]}): its call tree differs from the Python reference's"):
            for a, b in zip(nim_tree.split("\n") + ["(end)"] * 5, py_tree.split("\n") + ["(end)"] * 5):
                if a != b:
                    print(f"    Nim:    {a[:300]}\n    Python: {b[:300]}")
                    break
    print(f"  Call trees, against the Python reference's (every call, its arguments and result, and every `if` "
          f"outcome; {frames} frames in all): the paper's examples, {n_ok['example']} of {n_all['example']} calls "
          f"identical; a sample of one call in 97, {n_ok['sample']} of {n_all['sample']} identical")


def coverage(info, counts, check):
    print("\n  Nim branch coverage, by line of the paper (each `if`: times true / times false; each `or` term: times "
          "true):")
    missing = []
    n_if = n_term = n_stmt = 0
    lost = Counter()
    for i in sorted(info):
        part, paper, fname, k, text, _ = info[i]
        t, f = counts.get(i, (0, 0))
        if k == "if":
            n_if += 1
            print(f"    {part:4s} {fname:18s} line {paper:4d}  if {text:62s} true {t:7d}  false {f:7d}")
            if not t:
                missing.append(f"Appendix {part}, {fname}, line {paper}: `if {text}` never true")
            if not f:
                missing.append(f"Appendix {part}, {fname}, line {paper}: `if {text}` never false")
            lost["if"] += (not t) + (not f)
        elif k == "term":
            n_term += 1
            print(f"    {'':4s} {'':18s}            or-term {text:57s} true {t:7d}")
            if not t:
                missing.append(f"Appendix {part}, {fname}, line {paper}: or-term `{text}` never true")
                lost["term"] += 1
        else:
            n_stmt += 1
            if not t:
                missing.append(f"Appendix {part}, {fname}, line {paper}: `{text}` never ran")
                lost["stmt"] += 1
    print(f"    if outcomes reached: {2 * n_if - lost['if']} of {2 * n_if};  or-terms true: {n_term - lost['term']} of "
          f"{n_term};  other statements run: {n_stmt - lost['stmt']} of {n_stmt}")
    for m in missing:
        check(False, "Nim coverage: " + m)
