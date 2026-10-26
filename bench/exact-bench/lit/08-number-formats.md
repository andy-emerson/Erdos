# 8. Three bounded-rational formats

Chapter 3 fixed one format: a signed 64-bit numerator over a 64-bit denominator, both bounded by $2^{63} - 1$. This chapter compares it with two alternatives that use the same 128 bits differently. All three are pure integer formats: a value is always a numerator and a denominator, never a float.

| Format | Representable set | Storage |
|---|---|---|
| **Q64**, fixed-slash 63/63 (Chapter 3) | $\lvert p \rvert \le 2^{63} - 1,\ 1 \le q \le 2^{63} - 1$ | `int64` + `int64` |
| **Q64u**, fixed-slash 63/64 | $\lvert p \rvert \le 2^{63} - 1,\ 1 \le q \le 2^{64} - 1$ | `int64` + `uint64` |
| **FS120**, floating-slash | $\operatorname{bits}(\lvert p \rvert) + \operatorname{bits}(q) \le 120$ | sign + 7-bit slash position + 120 bits |

**Q64u.** Only one sign is needed per rational, so the denominator can be unsigned. That doubles the largest denominator, which is one extra bit. It only helps where the denominator bound is the one that binds, which is for $|x| < 1$. There, a generic value is approximated with error about $1/q^2$ for $q$ near the bound, so doubling the bound should cut errors by about a factor of 4. For $|x| > 1$ the numerator bound, still $2^{63} - 1$, binds first, the extra denominator bit is unusable, and Q64u gives exactly the same results as Q64.

**FS120.** Matula and Kornerup's *floating-slash* format lets each value split a fixed bit budget between numerator and denominator however it needs. A 128-bit word spends 1 bit on the sign and 7 bits recording where the slash sits, between bit positions 0 and 127, which leaves 120 bits for $|p|$ and $q$ together. This is slightly conservative. Matula and Kornerup's formats can also drop the leading 1 bit of each integer, since it is implied, gaining about two more bits. Without that refinement the comparison near 1 slightly understates floating-slash. Near $|x| \approx 1$, numerator and denominator have similar sizes, about 60 bits each, against 63 each in Q64, so FS120 is expected to be *slightly less* precise there. The gain is at large and small magnitudes. Q64 can represent $10^{15}$ only with denominators below about $2^{13}$, while FS120 can give it a denominator of about 35 bits (an 85-bit numerator over a 35-bit denominator fills the 120-bit budget). So FS120 trades a few bits near 1 for roughly uniform relative precision across a huge range, from about $2^{-119}$ to $2^{119}$.

## 8.1 One rounding routine for every format

Each format is a set $\mathcal F$ of fractions defined by a predicate on $(|p|, q)$ that is **downward closed**: if $(a, b)$ is allowed then so is every $(a', b')$ with $a' \le a$ and $b' \le b$. Both a box ($|p| \le N$, $q \le D$) and a bit budget ($\operatorname{bits}(|p|) + \operatorname{bits}(q) \le B$) have this property.

The proof in Section 3.2 used only that property. Along the continued-fraction descent toward $x$, numerators and denominators increase, so the convergents and semiconvergents that are allowed form an initial segment. Any fraction strictly between two Farey neighbours dominates their mediant componentwise, so if the mediant is excluded, everything between the neighbours is excluded. The nearest element of $\mathcal F$ to $x$ is therefore still one of two candidates: the last allowed convergent $h_j/k_j$, or the semiconvergent $(t h_j + h_{j-1})/(t k_j + k_{j-1})$ with the largest allowed $t$.

For a box, Chapter 3 found that $t$ by division. For a general downward-closed set, allowedness is monotone in $t$, so a binary search over $0 \le t < a_{j+1}$ finds it in $O(\log a_{j+1})$ predicate tests. The tie rules are unchanged: toward the smaller denominator, then toward zero. In fact, with only these two candidates, a tie always resolves to the convergent. When $t = 0$ the semiconvergent is the previous convergent, which is strictly farther from $x$. When $t \ge 1$ it has the larger denominator. The explicit tie test in the code is therefore never decisive, but it states the rule rather than relying on that argument.

```python {#formats rounding}
class FormatRangeError(ArithmeticError):
    pass


def round_to_format(p, q, allowed, max_value):
    if q == 0:
        raise ZeroDivisionError("zero denominator")
    if q < 0:
        p, q = -p, -q
    sign = -1 if p < 0 else 1
    p = abs(p)
    g = math.gcd(p, q)
    p, q = p // g, q // g
    if allowed(p, q):
        return sign * p, q
    if p > max_value * q:
        raise FormatRangeError("value outside the representable range")
    h2, k2, h1, k1 = 0, 1, 1, 0
    num, den = p, q
    a = 0
    while den:
        a = num // den
        h, k = a * h1 + h2, a * k1 + k2
        if not allowed(h, k):
            break
        h2, k2, h1, k1 = h1, k1, h, k
        num, den = den, num - a * den
    else:
        return sign * h1, k1
    lo, hi = 0, a - 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if allowed(mid * h1 + h2, mid * k1 + k2):
            lo = mid
        else:
            hi = mid - 1
    hs, ks = lo * h1 + h2, lo * k1 + k2
    if ks == 0:
        return sign * h1, k1
    d1 = abs(h1 * q - k1 * p) * ks
    ds = abs(hs * q - ks * p) * k1
    if ds < d1 or (ds == d1 and (ks, hs) < (k1, h1)):
        return sign * hs, ks
    return sign * h1, k1
```

The binary search starts at $t = 0$, which is always allowed, since $(h_{j-1}, k_{j-1})$ is an earlier convergent or the initial $(1, 0)$, and ends below $a_{j+1}$, which we know is not allowed.

## 8.2 The formats

```python {#formats definitions}
class Format:
    def __init__(self, name, allowed, max_value, storage_bytes):
        self.name, self.allowed, self.max_value, self.storage_bytes = name, allowed, max_value, storage_bytes

    def round(self, p, q):
        return round_to_format(p, q, self.allowed, self.max_value)


N63 = 2**63 - 1
D64 = 2**64 - 1
FS_BITS = 120

Q64_FMT = Format("Q64 (63/63)", lambda a, b: a <= N63 and b <= N63, N63, 16)
Q64U_FMT = Format("Q64u (63/64)", lambda a, b: a <= N63 and b <= D64, N63, 16)
FS120_FMT = Format("FS120 floating-slash", lambda a, b: a.bit_length() + b.bit_length() <= FS_BITS,
                   2**(FS_BITS - 1) - 1, 16)
FORMATS = [Q64_FMT, Q64U_FMT, FS120_FMT]
```

`max_value` is the largest representable magnitude: $2^{63} - 1$ for the fixed-slash formats, and $2^{119} - 1$ for FS120, a 119-bit numerator over the 1-bit denominator $1$.

## 8.3 A scalar class for any format

This mirrors Chapter 3's Q64 class, including lazy reduction and the same call structure, so that timings differ only in the format's own costs. `make_scalar` builds one class per format.

```python {#formats scalar}
def make_scalar(fmt):
    allowed, rnd = fmt.allowed, fmt.round

    class Scalar:
        __slots__ = ("n", "d")
        format = fmt

        def __init__(self, n, d=1):
            if d < 0:
                n, d = -n, -d
            if d == 0:
                raise ZeroDivisionError("zero denominator")
            if not allowed(abs(n), d):
                n, d = rnd(n, d)
            self.n, self.d = n, d

        @staticmethod
        def coerce(x):
            if isinstance(x, Scalar):
                return x
            if isinstance(x, int):
                return Scalar(x)
            if isinstance(x, Fraction):
                return Scalar(x.numerator, x.denominator)
            return NotImplemented

        def __add__(self, other):
            o = Scalar.coerce(other)
            if o is NotImplemented:
                return o
            return Scalar(self.n * o.d + o.n * self.d, self.d * o.d)

        __radd__ = __add__

        def __sub__(self, other):
            o = Scalar.coerce(other)
            if o is NotImplemented:
                return o
            return Scalar(self.n * o.d - o.n * self.d, self.d * o.d)

        def __rsub__(self, other):
            o = Scalar.coerce(other)
            if o is NotImplemented:
                return o
            return o - self

        def __mul__(self, other):
            o = Scalar.coerce(other)
            if o is NotImplemented:
                return o
            return Scalar(self.n * o.n, self.d * o.d)

        __rmul__ = __mul__

        def __truediv__(self, other):
            o = Scalar.coerce(other)
            if o is NotImplemented:
                return o
            return Scalar(self.n * o.d, self.d * o.n)

        def fraction(self):
            return Fraction(self.n, self.d)

    Scalar.__name__ = fmt.name
    return Scalar
```

## 8.4 What a 64-bit word costs here

These classes run in Python, with unbounded integers, so the formats differ in time only through the cost of rounding. A compiled implementation would differ more:

- **Q64u** makes every cross product a signed-times-unsigned multiplication, which Chapter 4 showed is a trap in Numba, where mixed `int64`/`uint64` arithmetic silently becomes float. That costs care, but no extra instructions.
- **FS120** packs both integers into one 128-bit word whose split varies per value. Every operation must unpack, which costs two shifts and a mask, compute with up to 120-bit operands, and repack. On 64-bit hardware that is double-word arithmetic throughout, roughly the cost of the 128-bit fallback in Chapter 7, plus variable shifts.

The benchmarks below therefore measure **precision**, which these Python classes measure exactly, and report time only as a secondary figure. Even that figure is not purely the format's cost: the generic class tests membership through a function call on every operation, where Chapter 3's class uses inline comparisons. The Q64 row here reproduces Chapter 3's recurrence errors exactly, so precision is consistent between the two implementations.

## 8.5 Correctness tests

**Brute force on small budgets.** For a box with tiny bounds and for a floating-slash budget of 6 bits, enumerate every allowed fraction and compare `round_to_format` against an exhaustive nearest search on every $p/q$ with small $q$. This checks the generalised proof, the binary search, and the tie rules.

**Agreement with Chapter 3.** For the Q64 format, `round_to_format` must agree with Chapter 3's `best_approx` on random large inputs.

**Format properties.** Q64u must accept denominators above $2^{63}$, FS120 must accept unbalanced values such as $1/2^{100}$ that the fixed formats cannot hold, and the exact edges of each budget are pinned: $1/2^{118}$ uses exactly 120 bits and fits, $1/2^{119}$ uses 121 and does not, and each format's largest integer fits while the next one up raises a range error. That last check matters because `round_to_format` relies on `max_value` being exactly the largest representable magnitude. If it were too large, values just above the true range would silently saturate instead of raising. Also, and every rounded result, including results of arithmetic, must lie in its format. Closeness is checked by the brute-force test, not here.

```python {#tests formats}
def brute_format(x, allowed, nmax, dmax):
    best = None
    for q in range(1, dmax + 1):
        for p in range(-nmax, nmax + 1):
            if not allowed(abs(p), q):
                continue
            r = Fraction(p, q)
            key = (abs(r - x), q, abs(p))
            if best is None or key < best[0]:
                best = (key, r)
    return best[1]


def test_round_to_format_brute_force():
    shapes = [(lambda a, b: a <= 5 and b <= 7, 5, 5, 7),
              (lambda a, b: a <= 9 and b <= 3, 9, 9, 3),
              (lambda a, b: a.bit_length() + b.bit_length() <= 6, 31, 31, 31)]
    for allowed, max_value, nmax, dmax in shapes:
        for q in range(1, 30):
            for p in range(-max_value * q, max_value * q + 1):
                got = Fraction(*round_to_format(p, q, allowed, max_value))
                assert got == brute_format(Fraction(p, q), allowed, nmax, dmax), (p, q)


def test_q64_format_matches_chapter3():
    rng = random.Random(60)
    for _ in range(3000):
        e = rng.choice([64, 80, 126, 200])
        p, q = rng.randint(-(2**e), 2**e), rng.randint(1, 2**e)
        if abs(Fraction(p, q)) > N63:
            continue
        assert Q64_FMT.round(p, q) == best_approx(p, q), (p, q)


def test_format_properties():
    assert Q64U_FMT.round(1, 2**64 - 1) == (1, 2**64 - 1)
    assert Q64_FMT.round(1, 2**64 - 1) == (0, 1)
    assert FS120_FMT.round(1, 2**100) == (1, 2**100)
    assert Q64_FMT.round(1, 2**100) == (0, 1)
    assert FS120_FMT.round(1, 2**118) == (1, 2**118)
    assert FS120_FMT.round(1, 2**119) != (1, 2**119)
    assert FS120_FMT.round(2**119 - 1, 1) == (2**119 - 1, 1)
    assert Q64U_FMT.round(N63, 2**64 - 1) == (N63, 2**64 - 1)
    for fmt in FORMATS:
        assert fmt.allowed(fmt.max_value, 1) and not fmt.allowed(fmt.max_value + 1, 1)
        try:
            fmt.round(fmt.max_value + 1, 1)
            raise AssertionError("expected FormatRangeError")
        except FormatRangeError:
            pass
    rng = random.Random(61)
    for fmt in FORMATS:
        for _ in range(500):
            p, q = rng.randint(-(2**200), 2**200), rng.randint(1, 2**200)
            if abs(Fraction(p, q)) > fmt.max_value:
                continue
            a, b = fmt.round(p, q)
            assert fmt.allowed(abs(a), b)
            S = make_scalar(fmt)
            x = S(p, q) * S(3, 7) + S(1, 11)
            assert fmt.allowed(abs(x.n), x.d)
```

```python {file=build/formats.py}
import math
from fractions import Fraction


<<formats rounding>>


<<formats definitions>>


<<formats scalar>>
```

## 8.6 The benchmarks

**P1, precision across magnitudes.** For each magnitude $m \in \{10^{-12}, 10^{-6}, 10^{-1}, 1, 10^{6}, 10^{12}, 10^{18}\}$, draw 2,000 exact rationals uniformly in $[m, 10m)$, as $x = m\,(1 + 9u)$ with $u = a/2^{256}$ for a random 256-bit integer $a$. Each $x$ is then essentially a "random real" of one known order of magnitude, so each row sits in one regime: $10^{-1}$ lies wholly below 1, where the denominator bound binds, and $1$ lies wholly above. Round each to every format, and to binary64 through Python's correctly rounded `Fraction` to `float` conversion. Report the median relative error, and the maximum in the note. The time column is the time to round all 2,000 values. Some draws at $10^{18}$ exceed the range of the fixed-slash formats, about $9.2 \times 10^{18}$. Rounding those raises a range error, which the timed code catches and counts, so the time includes attempting them, and the note reports how many were out of range. The runner first does a 20-value smoke pass of P1, so a crash in any branch surfaces in seconds rather than after the full run.

**P2, the recurrences of Chapter 6.** Euler decay, the logistic map, and Newton's method for $\sqrt2$, run with each format's scalar class, with errors against the same 4096-bit ball references.

```python {#formats bench}
def safe_round(fmt, x):
    try:
        return fmt.round(x.numerator, x.denominator)
    except FormatRangeError:
        return None


def bench_precision(report, count=2000):
    rng = random.Random(62)
    for exp in (-12, -6, -1, 0, 6, 12, 18):
        scale = Fraction(10) ** exp
        xs = [scale * (1 + 9 * Fraction(rng.getrandbits(256), 2**256)) for _ in range(count)]
        contenders = {"float64": lambda xs=xs: [float(x) for x in xs]}
        for fmt in FORMATS:
            contenders[fmt.name] = lambda xs=xs, fmt=fmt: [safe_round(fmt, x) for x in xs]
        res = measure(contenders)
        extra = {}
        errors = [abs(Fraction(float(x)) - x) / x for x in xs]
        extra["float64"] = dict(error=float(statistics.median(errors)), bytes_per_value=8,
                                note=f"max {float(max(errors)):.2e}")
        for fmt in FORMATS:
            errs, out = [], 0
            for x in xs:
                r = safe_round(fmt, x)
                if r is None:
                    out += 1
                else:
                    errs.append(abs(Fraction(*r) - x) / x)
            note = f"max {float(max(errs)):.2e}" if errs else ""
            if out:
                note += f"; {out}/{count} out of range"
            extra[fmt.name] = dict(error=float(statistics.median(errs)) if errs else "all out of range",
                                   bytes_per_value=fmt.storage_bytes, note=note)
        report.add_measured("P1 rounding precision", f"|x| ~ 1e{exp}", res, "float64", **extra)


def bench_format_recurrences(report):
    cases = [("P2a Euler decay", lambda S: euler(S(1), S(1, 100), 1000), lambda: euler(1.0, 0.01, 1000),
              lambda: euler(flint.arb(1), flint.arb(1) / 100, 1000)),
             ("P2b logistic r=3.2", lambda S: logistic(S(16, 5), S(1, 5), S(1), 1000), lambda: logistic(3.2, 0.2, 1.0, 1000),
              lambda: logistic(flint.arb(16) / 5, flint.arb(1) / 5, flint.arb(1), 1000)),
             ("P2c Newton sqrt(2)", lambda S: newton_sqrt2(S(1), S(2), 8), lambda: newton_sqrt2(1.0, 2.0, 8),
              lambda: flint.arb(2).sqrt())]
    classes = [make_scalar(fmt) for fmt in FORMATS]
    for label, run, run_float, ref_fn in cases:
        ref = arb_ref(ref_fn)
        contenders = {"Python float": run_float}
        for S in classes:
            contenders[S.__name__] = lambda S=S: run(S)
        res = measure(contenders, gc_off=False)
        extra = {"Python float": dict(error=err_vs_arb(run_float(), ref), bytes_per_value=8)}
        for S in classes:
            x = run(S)
            extra[S.__name__] = dict(error=err_vs_arb(x.fraction(), ref), bytes_per_value=16)
        report.add_measured(label, "", res, "Python float", **extra)


def main():
    print("correctness tests")
    import test_formats
    for name in sorted(n for n in dir(test_formats) if n.startswith("test_")):
        t = time.perf_counter()
        getattr(test_formats, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    meta = metadata()
    smoke = Report()
    bench_precision(smoke, count=20)
    report = Report()
    bench_precision(report)
    print("  done  P1", flush=True)
    bench_format_recurrences(report)
    print("  done  P2", flush=True)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / "formats.md").write_text(f"# Number formats\n\n{header}\n\n{report.markdown()}\n")
    (out / "formats.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

```python {file=build/test_formats.py}
import random
from fractions import Fraction

from q64 import best_approx
from formats import round_to_format, make_scalar, FORMATS, FormatRangeError, Q64_FMT, Q64U_FMT, FS120_FMT, N63


<<tests formats>>
```

```python {file=build/run_formats.py}
import statistics

from run_benchmarks import *
from run_benchmarks import euler, logistic, newton_sqrt2, arb_ref, err_vs_arb, metadata

from formats import FORMATS, FormatRangeError, make_scalar


<<formats bench>>
```
