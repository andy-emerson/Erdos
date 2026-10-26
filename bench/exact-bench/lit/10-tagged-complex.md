# 10. Tagged complex numbers

This chapter prototypes a complex-number design in which every stored number carries **two signs**: the usual positive-or-negative sign, and a second bit saying whether it is real or imaginary. A complex value is then one or two tagged parts:

- a real number $a$ is one part, tagged real;
- an imaginary number $bi$ is one part, tagged imaginary;
- a general complex number $a + bi$ is two parts, one of each.

Zero parts are simply absent. So $3i$ takes one word, not two, and a column of purely real or purely imaginary values costs one word per value.

## 10.1 The quarter-turn law

The two bits together select one of the four complex units, $\{1, i, -1, -i\}$, which are the quarter-turn rotations $i^k$ for $k = 0, 1, 2, 3$. Write a one-part value as $i^k m$ with a magnitude $m \ge 0$. Then

$$\left(i^{k_1} m_1\right)\left(i^{k_2} m_2\right) = i^{\,(k_1 + k_2) \bmod 4}\, m_1 m_2.$$

Multiplying two one-part values multiplies their magnitudes and *adds their turn counts mod 4*. In bit terms, with $s$ the sign bit and $t$ the imaginary bit, $k = 2s + t$, so the product's bits are

$$t = t_1 \oplus t_2, \qquad s = s_1 \oplus s_2 \oplus (t_1 \wedge t_2).$$

The last term is $i \cdot i = -1$: two imaginary parts give a real part with the sign flipped. Division follows from $1/i^k = i^{-k}$. So one-part values are closed under multiplication, division and negation, and those operations never need a second *part*. A part can still outgrow one word, which moves it to the big tier of Section 10.3, but it stays a single part. Only adding a real part to an imaginary part creates a two-part value.

## 10.2 Bit layout

Each part is one 128-bit word, laid out as Chapter 8's floating-slash format with one payload bit given to the new tag:

| Field | Bits | Contents |
|---|---|---|
| sign | 1 | 0 positive, 1 negative |
| imaginary | 1 | 0 real, 1 imaginary |
| slash position | 7 | number of payload bits used by the numerator, $1 \dots 118$ |
| payload | 119 | numerator bits, then denominator bits |

So a part fits in a word when $\operatorname{bits}(|p|) + \operatorname{bits}(q) \le 119$. A stored part is never zero, since zero parts are absent, so the numerator uses at least 1 bit, and the denominator always uses at least 1, so the slash position runs from 1 to 118. The 7-bit field has 128 codes, so 10 are unused: 0 and 119 to 127. They are reserved for special values such as undefined or infinity, and `decode_part` rejects them explicitly. Chapter 8's hidden-bit refinement, which drops the leading 1 of each integer, is not applied.

```python {#cx layout}
PAYLOAD_BITS = 119
SLASH_SHIFT = PAYLOAD_BITS
IMAG_SHIFT = SLASH_SHIFT + 7
SIGN_SHIFT = IMAG_SHIFT + 1


def fits_word(num, den):
    return abs(num).bit_length() + den.bit_length() <= PAYLOAD_BITS


def encode_part(value, imaginary):
    num, den = value.numerator, value.denominator
    if num == 0 or not fits_word(num, den):
        raise ValueError("encode needs a nonzero value that fits one word")
    a = abs(num)
    slash = a.bit_length()
    word = (a << (PAYLOAD_BITS - slash)) | den
    return (int(num < 0) << SIGN_SHIFT) | (int(imaginary) << IMAG_SHIFT) | (slash << SLASH_SHIFT) | word


def decode_part(word):
    sign = (word >> SIGN_SHIFT) & 1
    imaginary = bool((word >> IMAG_SHIFT) & 1)
    slash = (word >> SLASH_SHIFT) & 0x7F
    if not 1 <= slash <= PAYLOAD_BITS - 1:
        raise ValueError(f"reserved slash code {slash}")
    payload = word & ((1 << PAYLOAD_BITS) - 1)
    width = PAYLOAD_BITS - slash
    num, den = payload >> width, payload & ((1 << width) - 1)
    if den == 0 or num.bit_length() != slash:
        raise ValueError("non-canonical word")
    return Fraction(-num if sign else num, den), imaginary
```

The numerator occupies the top `slash` bits of the payload and the denominator the low bits. Because `slash` is the numerator's exact bit length, decoding needs no other information.

**Multiplying words directly.** `mul_words` implements the quarter-turn law on the bit fields themselves. It reads the sign and imaginary bits and the unsigned magnitudes straight out of the two words, combines the bits with $t = t_1 \oplus t_2$ and $s = s_1 \oplus s_2 \oplus (t_1 \wedge t_2)$, multiplies and reduces the magnitudes, and packs the result. It never forms a signed number, so the bit rule is the only source of the result's sign and kind. If the product's magnitude does not fit a word, it returns `None`, and the caller would promote that part to the big tier.

```python {#cx mulwords}
def fields(word):
    s = (word >> SIGN_SHIFT) & 1
    t = (word >> IMAG_SHIFT) & 1
    slash = (word >> SLASH_SHIFT) & 0x7F
    payload = word & ((1 << PAYLOAD_BITS) - 1)
    width = PAYLOAD_BITS - slash
    return s, t, payload >> width, payload & ((1 << width) - 1)


def mul_words(w1, w2):
    s1, t1, p1, q1 = fields(w1)
    s2, t2, p2, q2 = fields(w2)
    t = t1 ^ t2
    s = s1 ^ s2 ^ (t1 & t2)
    p, q = p1 * p2, q1 * q2
    g = math.gcd(p, q)
    p, q = p // g, q // g
    if not fits_word(p, q):
        return None
    slash = p.bit_length()
    return (s << SIGN_SHIFT) | (t << IMAG_SHIFT) | (slash << SLASH_SHIFT) | (p << (PAYLOAD_BITS - slash)) | q
```

## 10.3 Tiers and demotion

Exactness is the default. A part whose numerator and denominator do not fit the 119-bit payload is not rounded. In the design, it moves to the **big** tier, an unbounded rational, and moves back to a single word as soon as a later result fits again. A part's tier is a pure function of its current value, so demotion is automatic and immediate.

**What this prototype actually implements.** Every part is held as a Python `Fraction`, whatever its tier. `tiers()` *computes* which tier each part belongs to, and `encode_part`/`mul_words` show that word-tier parts really do fit the layout and multiply correctly at the bit level. But values are not physically stored as words. The same holds for column widths in Section 10.5. So the tier and width tests check the classification rules, and exactness across a tier change comes from `Fraction`. Building real word storage is a job for the compiled implementation, where it would also be measurable.

Bounded precision is available only on request. `rounded(budget)` rounds each part to the nearest value fitting the budget, with Chapter 8's rounding routine, and it is the only operation that can lose exactness. A part whose magnitude exceeds the largest representable value for that budget, $2^{\text{budget}-1} - 1$, raises `FormatRangeError` rather than being clamped.

## 10.4 The value type

`Cx` stores its real and imaginary parts as exact `Fraction`s, with $0$ meaning "absent". The tagged view, `parts()`, lists only the nonzero parts. That is what gets stored, and its length is the number of words a value occupies.

Arithmetic takes the one-part fast paths whenever it can. A one-part times one-part product is one rational multiplication, with the result placed by the quarter-turn law of Section 10.1. In `Cx` the sign travels inside the `Fraction`. The word-level version that combines the bits explicitly is `mul_words` in Section 10.2, and the tests check the two against each other. The general case expands $(a + bi)(c + di)$, which skips any product involving an absent part. Division by a one-part value uses $1/(i^k m) = i^{-k}/m$. Division by a two-part value multiplies by the conjugate over the norm $c^2 + d^2$.

```python {#cx value}
class Cx:
    __slots__ = ("re", "im")

    def __init__(self, re=0, im=0):
        self.re, self.im = Fraction(re), Fraction(im)

    @staticmethod
    def lift(x):
        return x if isinstance(x, Cx) else Cx(x)

    def parts(self):
        return [(v, imag) for v, imag in ((self.re, False), (self.im, True)) if v != 0]

    def words(self):
        return len(self.parts())

    def tiers(self):
        return ["word" if fits_word(v.numerator, v.denominator) else "big" for v, _ in self.parts()]

    def __add__(self, o):
        o = Cx.lift(o)
        return Cx(self.re + o.re, self.im + o.im)

    __radd__ = __add__

    def __sub__(self, o):
        o = Cx.lift(o)
        return Cx(self.re - o.re, self.im - o.im)

    def __rsub__(self, o):
        return Cx.lift(o) - self

    def __neg__(self):
        return Cx(-self.re, -self.im)

    def __mul__(self, o):
        o = Cx.lift(o)
        a, b, c, d = self.re, self.im, o.re, o.im
        if not b and not d:
            return Cx(a * c, 0)
        if not a and not c:
            return Cx(-(b * d), 0)
        if not b and not c:
            return Cx(0, a * d)
        if not a and not d:
            return Cx(0, b * c)
        re = (a * c if a and c else 0) - (b * d if b and d else 0)
        im = (a * d if a and d else 0) + (b * c if b and c else 0)
        return Cx(re, im)

    __rmul__ = __mul__

    def __truediv__(self, o):
        o = Cx.lift(o)
        c, d = o.re, o.im
        if not c and not d:
            raise ZeroDivisionError("complex division by zero")
        if not d:
            return Cx(self.re / c, self.im / c)
        if not c:
            return Cx(self.im / d, -self.re / d)
        n = c * c + d * d
        return self * Cx(c / n, -d / n)

    def __rtruediv__(self, o):
        return Cx.lift(o) / self

    def __eq__(self, o):
        o = Cx.lift(o)
        return self.re == o.re and self.im == o.im

    def __hash__(self):
        return hash(self.re) if not self.im else hash((self.re, self.im))

    def rounded(self, budget=PAYLOAD_BITS):
        allowed = lambda a, b: a.bit_length() + b.bit_length() <= budget
        top = 2 ** (budget - 1) - 1

        def r(v):
            if v == 0:
                return Fraction(0)
            return Fraction(*round_to_format(v.numerator, v.denominator, allowed, top))

        return Cx(r(self.re), r(self.im))

    def __repr__(self):
        return f"Cx({self.re}, {self.im})"


I = Cx(0, 1)
```

The four one-part branches of `__mul__` are the quarter-turn law written out. Real times real is real. Imaginary times imaginary is real and negated. Mixed products are imaginary. `Fraction` carries the sign, so the sign rule $s = s_1 \oplus s_2 \oplus (t_1 \wedge t_2)$ appears as the minus sign on $b \cdot d$.

## 10.5 Columns

A column is fixed width: every element occupies the same number of words. Two widths are needed:

- **one-part columns**, one word per element, each element tagged real or imaginary independently;
- **two-part columns**, two words per element, real then imaginary, where a zero part is stored as the value $0$.

The rule is the same one used for integer overflow in Chapters 4 and 7. A column takes the narrowest width that holds every element. An operation whose result needs two parts for some element widens the whole column. A result whose elements all fit in one part narrows it again. In this prototype, a column's width is computed whenever a column is built, which includes the result of every column operation. As with tiers, the width is a classification of `Fraction`-backed values, not a change of physical storage.

```python {#cx columns}
class CxColumn:
    def __init__(self, values):
        self.values = [Cx.lift(v) for v in values]
        self.width = 2 if any(v.re and v.im for v in self.values) else 1

    def __len__(self):
        return len(self.values)

    def storage_words(self):
        return self.width * len(self.values)

    def map2(self, other, op):
        return CxColumn([op(a, b) for a, b in zip(self.values, other.values)])
```

`map2` rebuilds the result as a new column, so its width is recomputed from the results. That is the narrowing rule.

## 10.6 The reference: plain pairs

To check the tagged type exactly and to time it fairly, a reference implementation stores every complex value as a pair of `Fraction`s and always uses the textbook formulas. It has no shortcuts, no absent parts, and no tags: four multiplications per product, always.

```python {#cx reference}
class Pair:
    __slots__ = ("re", "im")

    def __init__(self, re=0, im=0):
        self.re, self.im = Fraction(re), Fraction(im)

    def __add__(self, o):
        return Pair(self.re + o.re, self.im + o.im)

    def __sub__(self, o):
        return Pair(self.re - o.re, self.im - o.im)

    def __mul__(self, o):
        return Pair(self.re * o.re - self.im * o.im, self.re * o.im + self.im * o.re)

    def __truediv__(self, o):
        n = o.re * o.re + o.im * o.im
        return Pair((self.re * o.re + self.im * o.im) / n, (self.im * o.re - self.re * o.im) / n)

    def as_cx(self):
        return Cx(self.re, self.im)
```

## 10.7 Correctness tests

- **The quarter-turn law,** exhaustively over all four units for both operands and several magnitudes, against the pair reference. Then the word-level `mul_words`, which combines the sign and imaginary bits itself, must agree with `Cx` multiplication on random one-part values: decoding its result must give the `Cx` product. That test fails if the bit rule is wrong in any way, for example with $i \cdot i = +1$ or with the sign and imaginary fields swapped in one place but not the other.
- **Random arithmetic,** chains of $+$, $-$, $\times$, $\div$ on random one- and two-part values with large and small coordinates, checked exactly against the pair reference.
- **Canonical shapes:** $i \cdot i = -1$ is one real part, $(1 + i) - i$ is one real part, zero has no parts, and $i^4 = 1$.
- **Layout:** encoding round-trips for random parts. The 119-bit edge fits, and 120 bits does not, on both the denominator side and the integer side. All four sign and kind combinations survive the round trip. Words with a reserved slash code, a zero denominator, or a numerator that doesn't match its slash position are rejected.
- **Tiers:** a product that outgrows 119 bits moves to the big tier, stays exact, and returns to a word after dividing back.
- **Columns:** a column of mixed one-part values has width 1. Adding a column that creates two-part values widens it, and subtracting it again narrows it back.
- **Rounding on request:** `rounded()` gives a value that fits, and equals Chapter 8's rounding at the default budget. Independently, at a 6-bit budget, `rounded()` is checked against a brute-force nearest search over every representable value, so nearest-ness is tested through `Cx` itself. Out-of-range parts must raise rather than clamp.

```python {#tests cx}
def rand_frac(rng, bits):
    return Fraction(rng.randint(-(2**bits), 2**bits), rng.randint(1, 2**bits))


def rand_cx(rng, bits):
    shape = rng.randrange(4)
    re = rand_frac(rng, bits) if shape in (1, 3) else 0
    im = rand_frac(rng, bits) if shape in (2, 3) else 0
    return Cx(re, im)


def test_quarter_turn_law():
    units = [Cx(1), I, Cx(-1), Cx(0, -1)]
    for m1 in (Fraction(3, 7), Fraction(2**60 + 1, 3)):
        for m2 in (Fraction(5, 11), Fraction(1, 2**50)):
            for k1, u1 in enumerate(units):
                for k2, u2 in enumerate(units):
                    x, y = u1 * m1, u2 * m2
                    got = x * y
                    want = units[(k1 + k2) % 4] * (m1 * m2)
                    assert got == want and got.words() == 1
                    assert got == Pair(x.re, x.im).__mul__(Pair(y.re, y.im)).as_cx()


def test_mul_words_implements_bit_rule():
    rng = random.Random(74)
    for _ in range(4000):
        x = Cx(rand_frac(rng, 28)) if rng.random() < 0.5 else Cx(0, rand_frac(rng, 28))
        y = Cx(rand_frac(rng, 28)) if rng.random() < 0.5 else Cx(0, rand_frac(rng, 28))
        if x == 0 or y == 0:
            continue
        w = mul_words(encode_part(*x.parts()[0]), encode_part(*y.parts()[0]))
        v, imag = decode_part(w)
        assert (Cx(0, v) if imag else Cx(v)) == x * y
    huge = encode_part(Fraction(2**100 + 1, 3), False)
    assert mul_words(huge, huge) is None


def test_random_arithmetic_matches_pairs():
    rng = random.Random(70)
    ops = [lambda a, b: a + b, lambda a, b: a - b, lambda a, b: a * b, lambda a, b: a / b]
    for _ in range(3000):
        bits = rng.choice([4, 40, 130])
        x, y = rand_cx(rng, bits), rand_cx(rng, bits)
        if y == 0:
            continue
        op = rng.choice(ops)
        assert op(x, y) == op(Pair(x.re, x.im), Pair(y.re, y.im)).as_cx()


def test_canonical_shapes():
    assert 1 - I == Cx(1, -1) and 1 / I == Cx(0, -1) and Fraction(1, 2) * I == Cx(0, Fraction(1, 2))
    assert hash(Cx(1)) == hash(1) and {Cx(3): "x"}[3] == "x"
    assert I * I == -1 and (I * I).parts() == [(Fraction(-1), False)]
    assert (Cx(1, 1) - I).words() == 1
    assert Cx(0).words() == 0 and Cx(0).parts() == []
    assert I * I * I * I == 1


def test_layout_roundtrip_and_edges():
    rng = random.Random(71)
    for _ in range(2000):
        v = rand_frac(rng, rng.choice([1, 20, 55]))
        if v == 0 or not fits_word(v.numerator, v.denominator):
            continue
        for imag in (False, True):
            assert decode_part(encode_part(v, imag)) == (v, imag)
            assert encode_part(v, imag) < 2**128
    edge = Fraction(1, 2**117)
    assert fits_word(edge.numerator, edge.denominator)
    assert decode_part(encode_part(-edge, True)) == (-edge, True)
    assert not fits_word(1, 2**118)
    big = Fraction(2**118 - 1)
    assert decode_part(encode_part(big, False)) == (big, False)
    assert not fits_word(2**118, 1)
    for bad in (0, 1 << SIGN_SHIFT, 120 << (PAYLOAD_BITS), (5 << PAYLOAD_BITS) | 1):
        try:
            decode_part(bad)
            raise AssertionError("expected ValueError")
        except ValueError:
            pass


def test_tiers_promote_and_demote():
    x = Cx(Fraction(2**70 + 3, 7), Fraction(5, 2**40))
    assert x.tiers() == ["word", "word"]
    y = x * x
    assert "big" in y.tiers()
    z = y / x
    assert z == x and z.tiers() == ["word", "word"]


def test_columns_widen_and_narrow():
    col = CxColumn([Cx(1), Cx(0, 2), Cx(-3), Cx(0, Fraction(1, 5))])
    assert col.width == 1 and col.storage_words() == 4
    shift = CxColumn([I, Cx(1), I, Cx(1)])
    wide = col.map2(shift, lambda a, b: a + b)
    assert wide.width == 2
    back = wide.map2(shift, lambda a, b: a - b)
    assert back.width == 1 and back.values == col.values


def test_rounding_small_budget_brute_force():
    budget = 6
    allowed = lambda a, b: a.bit_length() + b.bit_length() <= budget
    grid = [Fraction(p, q) for q in range(1, 32) for p in range(-31, 32) if allowed(abs(p), q)]
    for q in range(1, 25):
        for p in range(-31 * q, 31 * q + 1):
            v = Fraction(p, q)
            if abs(v) > 2 ** (budget - 1) - 1:
                continue
            want = min(grid, key=lambda r: (abs(r - v), r.denominator, abs(r.numerator)))
            got = Cx(v, v).rounded(budget)
            assert got.re == want and got.im == want, (p, q)
    try:
        Cx(Fraction(2**130, 3)).rounded()
        raise AssertionError("expected FormatRangeError")
    except FormatRangeError:
        pass


def test_rounding_on_request():
    rng = random.Random(72)
    for _ in range(300):
        x = Cx(rand_frac(rng, 200), rand_frac(rng, 200))
        r = x.rounded()
        for v, _ in r.parts():
            assert fits_word(v.numerator, v.denominator)
        allowed = lambda a, b: a.bit_length() + b.bit_length() <= PAYLOAD_BITS
        for part, got in ((x.re, r.re), (x.im, r.im)):
            if part:
                assert got == Fraction(*round_to_format(part.numerator, part.denominator, allowed, 2**118 - 1))
```

```python {file=build/tagged_complex.py}
import math
from fractions import Fraction

from formats import round_to_format


<<cx layout>>


<<cx mulwords>>


<<cx value>>


<<cx columns>>


<<cx reference>>
```

```python {file=build/test_tagged_complex.py}
import random
from fractions import Fraction

from formats import round_to_format, FormatRangeError
from tagged_complex import (Cx, I, Pair, CxColumn, encode_part, decode_part, fits_word, mul_words,
                            PAYLOAD_BITS, SIGN_SHIFT, IMAG_SHIFT)


<<tests cx>>
```

## 10.8 The benchmark

Four workloads of $10^5$ values each, chosen to span the range from "tags help a lot" to "tags cannot help":

- **W1, real data.** Measured three-place decimals, multiplied elementwise by a second real column. Every value is one real part.
- **W2, rotation by $i$.** The W1 data multiplied by $i$, then by a column of random one-part values, each real or imaginary. Every value stays one part: the quarter-turn case.
- **W3, a 4-point DFT of real data.** Each block $x_0, \dots, x_3$ maps to $X_k = \sum_j x_j\, i^{-jk}$. The twiddle factors are the four units, so every twiddle multiplication is a quarter turn. $X_0$ and $X_2$ are real, while $X_1$ and $X_3$ are genuinely complex, so the output is half one-part and half two-part.
- **W4, general complex arithmetic.** Products of random two-part values. No part is ever absent, so tags cannot help.

For each workload: the time for the tagged type and for the pair reference, the storage per value (the bytes column is the fixed-width column after the narrowing rule; the note adds the average number of parts per value, which is what a variable-width encoding would store), and an exact check that the tagged results equal the pair results. NumPy `complex128` is shown for context. Its inputs are converted to floats before timing, so its time is the arithmetic alone, and its error is the max-norm error against the exact answer, scaled by the largest coordinate.

Both exact implementations are pure Python with `Fraction` arithmetic, and the comparison between them is not perfectly even. The tagged multiply spends extra branch tests deciding which parts are absent, which the pair reference never does. On W4, where no part is ever absent, those tests are pure overhead, and they are why tagged is slower there. Elsewhere they pay for themselves by skipping multiplications. The storage figures describe the designed 128-bit word layout, since this prototype holds `Fraction`s, so they compare the two designs rather than measure memory. The tagged figure is reported only after checking that every result part is word-tier. None of these times forecast compiled speed.

```python {#cx bench}
def workload_data(n, seed):
    rng = random.Random(seed)
    dec = lambda: Fraction(rng.randint(-10**7, 10**7), 1000)
    one = lambda: Cx(dec()) if rng.random() < 0.5 else Cx(0, dec())
    two = lambda: Cx(dec(), dec())
    return dec, one, two


def cx_to_np(xs):
    return np.array([complex(float(x.re), float(x.im)) for x in xs])


def max_rel_err(approx, exact):
    scale = max(max(abs(e.re), abs(e.im)) for e in exact)
    worst = 0
    for a, e in zip(approx, exact):
        worst = max(worst, abs(Fraction(a.real) - e.re), abs(Fraction(a.imag) - e.im))
    return float(worst / scale)


def dft4(blocks, mul):
    out = []
    for x0, x1, x2, x3 in blocks:
        tw = [[1, 1, 1, 1], [1, -1j, -1, 1j], [1, -1, 1, -1], [1, 1j, -1, -1j]]
        for k in range(4):
            acc = None
            for j, xj in enumerate((x0, x1, x2, x3)):
                term = mul(xj, tw[k][j])
                acc = term if acc is None else acc + term
            out.append(acc)
    return out


UNIT = {1: Cx(1), -1: Cx(-1), 1j: I, -1j: Cx(0, -1)}
UNIT_PAIR = {k: Pair(v.re, v.im) for k, v in UNIT.items()}


def bench_cx(report, n):
    dec, one, two = workload_data(n, 73)
    a = [Cx(dec()) for _ in range(n)]
    b = [Cx(dec()) for _ in range(n)]
    r = [one() for _ in range(n)]
    g = [two() for _ in range(n)]
    h = [two() for _ in range(n)]
    P = lambda xs: [Pair(x.re, x.im) for x in xs]
    pa, pb, pr, pg, ph = P(a), P(b), P(r), P(g), P(h)
    na, nb, nr, ng, nh = (cx_to_np(xs) for xs in (a, b, r, g, h))
    pair_i = UNIT_PAIR[1j]
    work = {
        "W1 real data": (lambda: [x * y for x, y in zip(a, b)],
                         lambda: [x * y for x, y in zip(pa, pb)],
                         lambda: na * nb),
        "W2 rotation by i": (lambda: [(x * I) * y for x, y in zip(a, r)],
                             lambda: [(x * pair_i) * y for x, y in zip(pa, pr)],
                             lambda: na * 1j * nr),
        "W3 4-point DFT of real data": (lambda: dft4(zip(*[iter(a)] * 4), lambda x, t: x * UNIT[t]),
                                        lambda: dft4(zip(*[iter(pa)] * 4), lambda x, t: x * UNIT_PAIR[t]),
                                        lambda: np.fft.fft(na.reshape(-1, 4), axis=1).ravel()),
        "W4 general complex products": (lambda: [x * y for x, y in zip(g, h)],
                                        lambda: [x * y for x, y in zip(pg, ph)],
                                        lambda: ng * nh),
    }
    for label, (tagged, pairs, numpy_fn) in work.items():
        res = measure({"pairs (reference)": pairs, "tagged": tagged, "NumPy complex128": numpy_fn})
        t_out, p_out = tagged(), pairs()
        ok = all(t == p.as_cx() for t, p in zip(t_out, p_out))
        words = sum(x.words() for x in t_out) / len(t_out)
        width = CxColumn(t_out).width
        all_word_tier = all(tier == "word" for x in t_out for tier in x.tiers())
        report.add_measured(label, n, res, "pairs (reference)", **{
            "pairs (reference)": dict(error="0 (exact)", bytes_per_value=32, note="2 words per value"),
            "tagged": dict(error=verify(ok, label, "0 (matches pairs)"),
                           bytes_per_value=16 * width if all_word_tier else None,
                           note=f"fixed-width column of width {width}; {words:.2f} parts per value if stored variable-width"
                                + ("" if all_word_tier else "; some parts big-tier, so bytes not reported")),
            "NumPy complex128": dict(error=max_rel_err(numpy_fn(), t_out), bytes_per_value=16,
                                     note="arithmetic only, on pre-converted arrays; error is max-norm"),
        })


def main():
    print("correctness tests")
    import test_tagged_complex
    for name in sorted(n for n in dir(test_tagged_complex) if n.startswith("test_")):
        t = time.perf_counter()
        getattr(test_tagged_complex, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    quick = "--quick" in sys.argv
    meta = metadata()
    report = Report()
    bench_cx(report, 4000 if quick else 10**5)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / f"complex{suffix}.md").write_text(f"# Tagged complex numbers\n\n{header}\n\n{report.markdown()}\n")
    (out / f"complex{suffix}.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

For W3, NumPy uses its FFT on each block of four, which computes the same transform with the same sign convention, $X_k = \sum_j x_j e^{-2\pi i jk/4}$.

```python {file=build/run_tagged_complex.py}
from run_benchmarks import *
from run_benchmarks import metadata, verify

from tagged_complex import Cx, I, Pair, CxColumn


<<cx bench>>
```
