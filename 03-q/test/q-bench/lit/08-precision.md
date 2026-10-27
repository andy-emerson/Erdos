# Precision and exactness

Rounding is not part of the format: a value that does not fit is promoted. It is measured only to characterise how finely Q72 covers the reals.

- **Typical precision**: random reals at each magnitude, rounded to the nearest Q72 and to binary64.
- **Worst-case precision**: the worst rounding error at a magnitude is half the gap beside its simplest fractions. `neighbours` finds those gaps exactly (they are Farey neighbours), and the scan covers every simple fraction times a power of two in the window $[10^k/10, 10^k \cdot 10]$. At the bottom of the range the gap is to zero, as with float underflow; that row is labelled and excluded from the bits-to-match figure.
- **Bits to match**: the smallest float significand whose median, and whose worst-case, relative error is at least as good, and the size of an IEEE-style format with that significand and the 8-bit exponent Q72's range needs.
- **Exactness**: how often common kinds of value are exactly representable in binary64, decimal64 and Q72.

```python {file=build/precision.py}
"""Precision measurements. Rounding is not part of the Q72 specification; it is measured here only to compare how
finely Q72 values cover the reals with how finely floating-point formats of a given size do."""
import math
import random
from fractions import Fraction

import numpy as np

import reuse  # noqa: F401
from formats import round_to_format
from q72 import fits

WORD = 64
LARGEST = 2**WORD - 1
SMALLEST = Fraction(1, 2**(WORD - 1))


def round_q72(x, word=WORD):
    """Nearest value that fits one pair (ties: smaller denominator, then toward zero); None outside the range."""
    x = Fraction(x)
    m = abs(x)
    largest = 2**word - 1
    if m > largest:
        return None
    if fits(m.numerator, m.denominator, word):
        r = m
    else:
        p, q = round_to_format(m.numerator, m.denominator, lambda a, b: fits(a, b, word), largest)
        r = Fraction(p, q)
    return r if x >= 0 else -r


def round_float(x, p):
    """x rounded to nearest-even with a p-bit significand and unbounded exponent."""
    x = Fraction(x)
    if x == 0:
        return x
    m = abs(x)
    e = m.numerator.bit_length() - m.denominator.bit_length()
    if Fraction(2) ** e > m:
        e -= 1
    scale = Fraction(2) ** (p - 1 - e)
    r = round(m * scale) / scale
    return r if x > 0 else -r


def random_reals(magnitude_exp10, samples, rng):
    return [Fraction(rng.getrandbits(400) | 1, rng.getrandbits(400) | 1) * Fraction(10) ** magnitude_exp10
            for _ in range(samples)]


def typical_errors(samples=300, seed=31, exps=range(-18, 19, 3)):
    """Median and worst relative rounding error at random points of each magnitude: float64 vs Q72."""
    rng = random.Random(seed)
    rows = []
    for k in exps:
        q_err, f_err = [], []
        for x in random_reals(k, samples, rng):
            r = round_q72(x)
            q_err.append(float(abs(r - x) / x) if r is not None and r != 0 else math.inf)
            f_err.append(float(abs(Fraction(float(x)) - x) / x))
        rows.append(dict(exp=k, q72_median=float(np.median(q_err)), q72_max=float(np.max(q_err)),
                         f64_median=float(np.median(f_err)), f64_max=float(np.max(f_err))))
    return rows


def neighbours(a, b, word=WORD):
    """The predecessor and successor of a/b (a >= 1, coprime) among values that fit one Q72.

    The set of fitting magnitudes is closed under taking smaller numerators and denominators, so the neighbours of
    a/b are Farey neighbours: c/d with b*c - a*d = +-1. Along each family d = d0 + k*b, c = c0 + k*a the gap
    1/(b*d) shrinks as k grows, so the neighbour is the member with the largest k that still fits."""
    out = []
    for sign in (1, -1):
        if b == 1:
            d0, c0 = 1, a + sign
        else:
            d0 = (-sign * pow(a, -1, b)) % b
            c0 = (sign + a * d0) // b
        lo, hi = 0, 1
        while c0 + hi * a >= 1 and fits(c0 + hi * a, d0 + hi * b, word):
            hi *= 2
        while hi - lo > 1:
            mid = (lo + hi) // 2
            if fits(c0 + mid * a, d0 + mid * b, word):
                lo = mid
            else:
                hi = mid
        c, d = c0 + lo * a, d0 + lo * b
        out.append(Fraction(c, d) if c >= 0 and d >= 1 and fits(c, d, word) else None)
    pred, succ = out[1], out[0]
    return pred, succ


def worst_relative_halfgap(a, b, word=WORD):
    """Largest relative rounding error just beside a/b: half the larger gap to a neighbour, over a/b."""
    x = Fraction(a, b)
    pred, succ = neighbours(a, b, word)
    gaps = [x - pred] if pred is not None else []
    if succ is not None:
        gaps.append(succ - x)
    return max(gaps) / 2 / x


def worst_case_profile(exps=range(-18, 19, 3), simple=40, word=WORD):
    """Worst relative half-gap at each magnitude 10**k, over the window [10**k / 10, 10**k * 10].

    Gaps are largest beside the simplest fractions, and multiplying by a power of two moves a fraction across
    magnitudes without making it more complicated. So the scan covers every (a/b) * 2**e in the window with a, b at
    most `simple`, plus the fractions with a or b at most `simple` that sit closest to 10**k."""
    rows = []
    for k in exps:
        target = Fraction(10) ** k
        lo_w, hi_w = target / 10, target * 10
        cands = set()
        for small in range(1, simple + 1):
            for a in (math.floor(target * small), math.ceil(target * small)):
                cands.add(Fraction(a, small))
            for b in (math.floor(small / target), math.ceil(small / target)):
                if b >= 1:
                    cands.add(Fraction(small, b))
        for a in range(1, simple + 1):
            for b in range(1, simple + 1):
                if math.gcd(a, b) != 1:
                    continue
                base = Fraction(a, b)
                e = math.floor(math.log2(lo_w / base)) - 1
                while True:
                    x = base * Fraction(2) ** e
                    if x > hi_w:
                        break
                    if x >= lo_w:
                        cands.add(x)
                    e += 1
        worst, at = Fraction(0), None
        for x in cands:
            a, b = x.numerator, x.denominator
            if a < 1 or not lo_w <= x <= hi_w or not fits(a, b, word):
                continue
            w = worst_relative_halfgap(a, b, word)
            if w > worst:
                worst, at = w, x
        rows.append(dict(exp=k, worst=float(worst), at=str(at)))
    return rows


def float_bits_to_match(err):
    """Smallest significand p (bits, including the hidden bit) whose worst-case relative rounding error 2**-p is
    at most err, and the total size of an IEEE-style format with that significand and an 8-bit exponent field,
    which is what Q72's range (2**-63 to 2**64, 128 binades) needs."""
    p = math.ceil(-math.log2(err))
    return p, 1 + 8 + (p - 1)


def float_median_bits(target_median, seed=41, samples=400):
    """Smallest significand p whose median relative rounding error on random reals is at most target_median."""
    rng = random.Random(seed)
    xs = random_reals(0, samples, rng)
    for p in range(20, 120):
        med = float(np.median([float(abs(round_float(x, p) - x) / x) for x in xs]))
        if med <= target_median:
            return p, 1 + 8 + (p - 1)
    return None, None


# ---------------------------------------------------------------- which values are exact

def exact_binary64(x):
    x = Fraction(x)
    try:
        return Fraction(float(x)) == x
    except OverflowError:
        return False


def exact_decimal64(x):
    """IEEE 754 decimal64: 16 significant decimal digits, exponents -398..369."""
    x = Fraction(x)
    if x == 0:
        return True
    q = x.denominator
    for f in (2, 5):
        while q % f == 0:
            q //= f
    if q != 1:
        return False
    k = 0
    while (x * 10**k).denominator != 1:
        k += 1
    m = abs((x * 10**k).numerator)
    while m % 10 == 0 and m:
        m //= 10
        k -= 1
    e = -k
    # a coefficient below 10**16 may be padded with trailing zeros, so the largest exponent is 369 + 16 - digits
    return m < 10**16 and -398 <= e <= 369 + 16 - len(str(m))


def exact_q72(x):
    x = Fraction(x)
    return fits(abs(x.numerator), x.denominator)


def exactness_rates(n=20000, seed=33):
    rng = random.Random(seed)

    def dec(places, bound):
        return Fraction(rng.randint(-bound * 10**places, bound * 10**places), 10**places)

    cases = {
        "money, 2 places, below 10^9": lambda: dec(2, 10**9),
        "measurements, 3 places, below 10^6": lambda: dec(3, 10**6),
        "measurements, 6 places, below 10^6": lambda: dec(6, 10**6),
        "products of two 3-place measurements": lambda: dec(3, 10**6) * dec(3, 10**6),
        "ratios a/b of integers up to 1000": lambda: Fraction(rng.randint(1, 1000), rng.randint(1, 1000)),
        "means of 7 three-place measurements": lambda: sum((dec(3, 10**6) for _ in range(7)), Fraction(0)) / 7,
        "binary64 values in [0, 1) times 10^k, k in -6..6": lambda: Fraction(rng.random() * 10.0 ** rng.randint(-6, 6)),
    }
    rows = []
    for name, gen in cases.items():
        xs = [gen() for _ in range(n)]
        rows.append(dict(values=name, binary64=sum(map(exact_binary64, xs)) / n,
                         decimal64=sum(map(exact_decimal64, xs)) / n, q72=sum(map(exact_q72, xs)) / n))
    return rows
```
