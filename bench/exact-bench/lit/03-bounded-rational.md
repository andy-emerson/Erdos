# 3. Bounded 64-bit rationals

This chapter defines the scalar number type, which we call **Q64**: a pair of 64-bit integers that represents a rational number exactly whenever it can, and otherwise rounds to the provably nearest representable rational.

## 3.1 The representable set

Fix $M = 2^{63} - 1$, the largest `int64`. The representable set is

$$\mathcal R_M = \left\{ \tfrac{p}{q} \;:\; p, q \in \mathbb{Z},\ |p| \le M,\ 1 \le q \le M \right\}.$$

This is what Matula and Kornerup called a *fixed-slash* system: both the numerator and the denominator get a fixed budget of bits, and the "slash" between them sits in a fixed place. Their alternative, *floating-slash*, shares one budget between the two and lets the boundary move. Fixed-slash is simpler and maps directly onto two machine words, so we start there.

Unlike binary floats, $\mathcal R_M$ is not evenly spaced in relative terms. For $|x| < 1$ the denominator bound is the one that binds, and $\mathcal R_M \cap [0,1]$ is the Farey sequence of order $M$. There, the neighbours of a fraction $a/b$ are at distance roughly $1/(bM)$. So the gaps range from about $1/M$, next to fractions with tiny denominators such as $0$, $1/2$ and $1$, down to about $1/M^2$ among fractions with denominators near $M$. Simple fractions are surrounded by the widest gaps, which is the reverse of what a naive picture suggests. For $|x| > 1$ the numerator bound takes over and the grid coarsens with magnitude. Near $|x| \approx M/2$, for example, the only representable values have denominators $1$ or $2$, so the spacing is about $1/2$. Large magnitudes are where a fixed-slash system is weakest, and where floats, which keep 53 bits of relative precision at every magnitude, are strongest. Floats make the opposite bet: every value gets the same 53 bits of relative precision. Which bet pays off depends on the data, and the stress tests in Chapter 6 probe exactly this.

## 3.2 Rounding: the nearest representable rational

When an exact result $x = p/q$ is not in $\mathcal R_M$, we round it to

$$\rho(x) = \operatorname*{arg\,min}_{r \in \mathcal R_M} |r - x|,$$

breaking ties toward the smaller denominator, the same convention as Python's `Fraction.limit_denominator`, and, when the two candidates have the same denominator, toward the smaller absolute numerator, that is, toward zero. The second rule only matters when both candidates are integers: for example $x = -1/2$ with $M = 1$ lies midway between $-1$ and $0$, and rounds to $0$. Two properties make $\rho$ the right rounding rule:

1. **It is exact when it can be.** If $x \in \mathcal R_M$ then $\rho(x) = x$, so a single rounding routine handles both reduction and approximation. This is the property discussed earlier: there is no need for a separate GCD step, because the routine returns the reduced exact value whenever it fits.
2. **It is deterministic**, a pure function of the exact value $x$. The result does not depend on how $x$ happened to be represented, reduced or not.

### Computing $\rho$ by continued fractions

Assume $x > 0$. The negative case follows by symmetry, since $\mathcal R_M$ is symmetric about zero. Write $x = [a_0; a_1, a_2, \dots]$ with convergents $h_j / k_j$, which satisfy

$$h_j = a_j h_{j-1} + h_{j-2}, \qquad k_j = a_j k_{j-1} + k_{j-2},$$

with $h_{-2} = 0$, $h_{-1} = 1$, $k_{-2} = 1$, $k_{-1} = 0$. Both $h_j$ and $k_j$ increase with $j$. Let $j$ be the last index whose convergent fits both bounds, and let

$$t = \max\{\, t' \ge 0 : t' h_j + h_{j-1} \le M \ \text{and}\ t' k_j + k_{j-1} \le M \,\}.$$

**Claim.** $\rho(x)$ is whichever of $h_j / k_j$ and $s = (t h_j + h_{j-1}) / (t k_j + k_{j-1})$ is closer to $x$.

*Proof sketch.* The semiconvergents $(t' h_j + h_{j-1})/(t' k_j + k_{j-1})$ for $t' = 0, 1, \dots, a_{j+1}$ move monotonically from $h_{j-1}/k_{j-1}$ to $h_{j+1}/k_{j+1}$, all on the opposite side of $x$ from $h_j/k_j$, with $x$ lying between $h_j/k_j$ and $h_{j+1}/k_{j+1}$. Because $h_{j+1}/k_{j+1}$ violates a bound, $t < a_{j+1}$, so $s$ and $h_j/k_j$ bracket $x$. They are also Farey neighbours: $|h_j (t k_j + k_{j-1}) - k_j (t h_j + h_{j-1})| = |h_j k_{j-1} - k_j h_{j-1}| = 1$. Any fraction strictly between two Farey neighbours $a/b < c/d$ has numerator at least $a + c$ and denominator at least $b + d$. The smallest such fraction is the mediant, which here is the semiconvergent at $t + 1$, and that violates a bound by the maximality of $t$. So no element of $\mathcal R_M$ lies strictly between the two candidates, and the nearest element to $x$ must be one of them. $\square$

This is the classical denominator-only argument, as in Python's `limit_denominator` and the treatment in Matula and Kornerup, extended to a numerator bound. The extension works because both bounds are monotone along the continued-fraction descent, so the feasible region is still closed under taking smaller convergents. Section 3.6 checks the claim against brute force.

The number of iterations is the length of the continued fraction, which for a $b$-bit input is at most about $b / \log_2 \varphi \approx 1.44\, b$ by Lamé's theorem, where $\varphi$ is the golden ratio. The inputs to $\rho$ are results of an operation on two Q64 values, such as the cross products in an addition, so they have up to about 128 bits. That is at most about 185 iterations, and far fewer for typical inputs.

The comparison of the two candidates is done in exact integer arithmetic:

$$\left|\tfrac{h}{k} - \tfrac{p}{q}\right| = \frac{|hq - kp|}{kq},$$

so comparing two distances means cross-multiplying by the candidate denominators.

```python {#q64 best approx}
class Q64RangeError(ArithmeticError):
    pass


def best_approx(p, q, bound=M):
    if q == 0:
        raise ZeroDivisionError("zero denominator")
    if q < 0:
        p, q = -p, -q
    sign = -1 if p < 0 else 1
    p = abs(p)
    g = math.gcd(p, q)
    p, q = p // g, q // g
    if p <= bound and q <= bound:
        return sign * p, q
    if p > bound * q:
        raise Q64RangeError(f"|x| exceeds {bound}")
    h2, k2, h1, k1 = 0, 1, 1, 0
    num, den = p, q
    while den:
        a = num // den
        h, k = a * h1 + h2, a * k1 + k2
        if h > bound or k > bound:
            break
        h2, k2, h1, k1 = h1, k1, h, k
        num, den = den, num - a * den
    else:
        return sign * h1, k1
    t_num = (bound - h2) // h1 if h1 else bound
    t_den = (bound - k2) // k1
    t = min(t_num, t_den)
    hs, ks = t * h1 + h2, t * k1 + k2
    if ks == 0:
        return sign * h1, k1
    d1 = abs(h1 * q - k1 * p) * ks
    ds = abs(hs * q - ks * p) * k1
    if ds < d1 or (ds == d1 and (ks, hs) < (k1, h1)):
        return sign * hs, ks
    return sign * h1, k1
```

Two guards deserve a word. If $x > M$, nothing in $\mathcal R_M$ lies above it, and we raise rather than silently saturate. This is the analogue of a float overflowing to infinity, made loud. When $h_j = 0$, which happens when $x < 1$ and $j = 0$, the numerator imposes no constraint on $t$. The loop's `else` branch can only be reached if the continued fraction terminates inside the bounds, which the earlier reduced-fits check rules out, but it is kept as a defensive return.

## 3.3 Lazy reduction

A Q64 value is a pair $(p, q)$ that is **not necessarily in lowest terms**. Arithmetic uses the textbook formulas

$$\frac{a}{b} \pm \frac{c}{d} = \frac{ad \pm cb}{bd}, \qquad \frac{a}{b}\cdot\frac{c}{d} = \frac{ac}{bd}, \qquad \frac{a}{b} \Big/ \frac{c}{d} = \frac{ad}{bc},$$

with no GCD. If the result fits in $\mathcal R_M$ as written, it is stored unreduced. Only when a component would exceed $M$ do we call $\rho$, which reduces and rounds in one step. Because $\rho$ reduces before checking the bounds, an unreduced representation never causes a loss of exactness. It can only cause $\rho$ to be invoked earlier than strictly necessary, which costs time, not accuracy.

Since representations are not canonical, equality and ordering compare by cross-multiplication, $a/b = c/d \iff ad = cb$, which is exact in unbounded integer arithmetic.

## 3.4 Scalar Q64 in Python

The scalar class is used where values flow through a loop one at a time: the recurrence stress tests in Chapter 6, and the correctness tests. Python integers are unbounded, so "overflow" here means "a component exceeds $M$", detected by comparison. The class can count two events, which the stress tests report. Counting is switched off while timing, so the timed code pays only for one test of a global flag per operation, and the counts come from a separate untimed run:

- `rounded`: the exact result did not fit as written, so $\rho$ was called;
- `inexact`: $\rho$ actually changed the value, meaning reduction alone was not enough.

```python {#q64 scalar}
STATS = {"ops": 0, "rounded": 0, "inexact": 0}
COUNTING = False


def reset_stats():
    for key in STATS:
        STATS[key] = 0


def set_counting(on):
    global COUNTING
    COUNTING = on


class Q64:
    __slots__ = ("n", "d")

    def __init__(self, n, d=1):
        if d < 0:
            n, d = -n, -d
        if d == 0:
            raise ZeroDivisionError("zero denominator")
        if abs(n) > M or d > M:
            rn, rd = best_approx(n, d)
            if COUNTING:
                STATS["rounded"] += 1
                STATS["inexact"] += rn * d != n * rd
            n, d = rn, rd
        self.n, self.d = n, d

    @staticmethod
    def coerce(x):
        if isinstance(x, Q64):
            return x
        if isinstance(x, int):
            return Q64(x)
        if isinstance(x, Fraction):
            return Q64(x.numerator, x.denominator)
        return NotImplemented

    def __add__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING:
            STATS["ops"] += 1
        return Q64(self.n * o.d + o.n * self.d, self.d * o.d)

    __radd__ = __add__

    def __sub__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING:
            STATS["ops"] += 1
        return Q64(self.n * o.d - o.n * self.d, self.d * o.d)

    def __rsub__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        return o - self

    def __mul__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING:
            STATS["ops"] += 1
        return Q64(self.n * o.n, self.d * o.d)

    __rmul__ = __mul__

    def __truediv__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING:
            STATS["ops"] += 1
        return Q64(self.n * o.d, self.d * o.n)

    def __rtruediv__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        return o / self

    def __neg__(self):
        return Q64(-self.n, self.d)

    def __eq__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        return self.n * o.d == o.n * self.d

    def __lt__(self, other):
        o = Q64.coerce(other)
        if o is NotImplemented:
            return o
        return self.n * o.d < o.n * self.d

    def __hash__(self):
        return hash(Fraction(self.n, self.d))

    def fraction(self):
        return Fraction(self.n, self.d)

    def __float__(self):
        return self.n / self.d

    def __repr__(self):
        return f"Q64({self.n}, {self.d})"
```

`__lt__` and `__gt__` are all the stress tests need. Python derives `>` from `__lt__` with the arguments swapped.

## 3.5 Overflow detection in compiled kernels

The column kernels in Chapter 4 run under Numba, where `int64` arithmetic wraps modulo $2^{64}$ silently, exactly like the hardware. Exactness requires that every wrap be detected. Two primitives do this.

**Addition.** For $s = a + b$ computed with wraparound, overflow occurred exactly when $a$ and $b$ have the same sign and $s$ has the other sign. In two's complement that is the sign bit of $(a \oplus s) \wedge (b \oplus s)$. We also reject $s = -2^{63}$, which is a valid `int64` but lies outside $\mathcal R_M$, whose numerators are bounded by $M = 2^{63} - 1$ in absolute value.

**Multiplication.** Without a 128-bit type we cannot see the high half of $ab$ directly. Instead we use a floating-point pre-check. Let $u = 2^{-53}$ be the unit roundoff. Converting $a$ and $b$ to binary64 and multiplying gives an estimate $E = \operatorname{fl}(\operatorname{fl}(a)\operatorname{fl}(b)) = ab(1+\delta_1)(1+\delta_2)(1+\delta_3)$ with each $|\delta_i| \le u$, so $|E| \ge |ab|(1-u)^3 \ge |ab|(1 - 3u)$. If $|E| < 2^{62}$, then $|ab| < 2^{62}/(1 - 3u) < 2^{63} - 1$, so the integer product cannot overflow. Otherwise we report overflow. This test is **conservative**. Products between $2^{62}$ and $2^{63}$ are flagged even though they would fit. That is safe, since a flagged element goes to the exact slow path, which returns the exact value whenever it fits. It costs a little speed in a narrow band and never costs exactness.

```python {#q64 kernels}
SAFE_PRODUCT = 2.0**62
INT64_MIN = -(2**63)


@njit(inline="always")
def add_ovf(a, b):
    s = a + b
    return s, ((a ^ s) & (b ^ s)) < 0 or s == INT64_MIN


@njit(inline="always")
def mul_ovf(a, b):
    if abs(float(a) * float(b)) < SAFE_PRODUCT:
        return a * b, False
    return np.int64(0), True
```

## 3.6 Correctness tests

**Brute force for $\rho$.** For small bounds $B$ we enumerate $\mathcal R_B$ directly and compare `best_approx` against an exhaustive nearest-element search on random inputs, including inputs with large numerators and denominators. This validates the claim in Section 3.2, including the numerator bound. Random inputs with large denominators almost never land exactly on a tie, so a second test enumerates every $p/q$ with $q < 40$ and $|p/q| \le B$ for $B \le 8$, which contains every tie of both kinds.

```python {#tests q64 brute}
def brute_nearest(x, bound):
    best = None
    for q in range(1, bound + 1):
        for p in range(-bound, bound + 1):
            r = Fraction(p, q)
            key = (abs(r - x), q, abs(p))
            if best is None or key < best[0]:
                best = (key, r)
    return best[1]


def test_best_approx_brute_force():
    rng = random.Random(10)
    for bound in [1, 2, 3, 5, 8, 13, 21]:
        for _ in range(300):
            q = rng.randint(1, 10**6)
            p = rng.randint(-bound * q, bound * q)
            x = Fraction(p, q)
            got = Fraction(*best_approx(p, q, bound))
            assert got == brute_nearest(x, bound), (p, q, bound, got)


def test_best_approx_exhaustive_small():
    for bound in range(1, 9):
        for q in range(1, 40):
            for p in range(-bound * q, bound * q + 1):
                got = Fraction(*best_approx(p, q, bound))
                assert got == brute_nearest(Fraction(p, q), bound), (p, q, bound, got)
```

**Exactness, range, and the ties.** Anything whose reduced form fits must come back unchanged, unreduced inputs included. Values beyond $M$ must raise. With $B = 2$ the positive elements of $\mathcal R_2$ are $0, \tfrac12, 1, 2$. So $x = 1/4$ is equidistant from $0/1$ and $1/2$ and must round to $0/1$, and $x = 3/4$ is equidistant from $1/2$ and $1/1$ and must round to $1/1$, the smaller denominator in each case.

```python {#tests q64 exact}
def test_best_approx_exact_when_fits():
    rng = random.Random(11)
    for _ in range(2000):
        p, q = rng.randint(-M, M), rng.randint(1, M)
        k = rng.randint(1, 10**6)
        assert Fraction(*best_approx(p * k, q * k)) == Fraction(p, q)


def test_best_approx_range_and_ties():
    try:
        best_approx(M + 1, 1)
        raise AssertionError("expected Q64RangeError")
    except Q64RangeError:
        pass
    assert best_approx(1, 4, 2) == (0, 1)
    assert best_approx(-1, 4, 2) == (0, 1)
    assert best_approx(3, 4, 2) == (1, 1)
    assert best_approx(355 * 10**30, 113 * 10**30, 400) == (355, 113)
    assert best_approx(314159265358979, 10**14, 400) == (355, 113)
```

**The kernels against unbounded integers.** Random operands are drawn at several magnitudes, plus the edge values $0$, $\pm 1$, $\pm M$, and $-2^{63}$. Whenever a primitive reports no overflow, its result must equal the exact integer result and lie in $[-M, M]$. Whenever the exact result lies in $[-M, M]$, addition must not report overflow. Multiplication may report overflow only above $2^{62}$, and must never do so below $2^{61}$.

```python {#tests q64 kernels}
def test_overflow_primitives():
    rng = random.Random(12)
    edges = [0, 1, -1, M, -M, INT64_MIN, 2**31, -(2**31), 2**62, -(2**62)]
    values = edges + [rng.randint(-(2**e), 2**e) for e in (10, 31, 32, 40, 62, 63) for _ in range(200)]
    values = [v for v in values if INT64_MIN <= v <= M]
    pairs = [(a, b) for a in edges for b in edges] + [(rng.choice(values), rng.choice(values)) for _ in range(20000)]
    for a, b in pairs:
        s, ovf = add_ovf(np.int64(a), np.int64(b))
        if not ovf:
            assert int(s) == a + b and abs(a + b) <= M, (a, b)
        if abs(a + b) <= M:
            assert not ovf, (a, b)
        p, ovf = mul_ovf(np.int64(a), np.int64(b))
        if not ovf:
            assert int(p) == a * b and abs(a * b) <= M, (a, b)
        if abs(a * b) < 2**61:
            assert not ovf, (a, b)
```

**Scalar arithmetic.** For random operands, every Q64 operation must equal $\rho$ of the exact `Fraction` result. When no rounding occurred it must equal the exact result itself.

```python {#tests q64 scalar}
def test_q64_matches_rounded_exact():
    rng = random.Random(13)
    ops = [operator.add, operator.sub, operator.mul, operator.truediv]
    for _ in range(5000):
        e = rng.choice([8, 31, 40, 62])
        a = Q64(rng.randint(-(2**e), 2**e), rng.randint(1, 2**e))
        b = Q64(rng.randint(-(2**e), 2**e) or 1, rng.randint(1, 2**e))
        op = rng.choice(ops)
        exact = op(a.fraction(), b.fraction())
        if abs(exact) > M:
            continue
        got = op(a, b).fraction()
        assert got == Fraction(*best_approx(exact.numerator, exact.denominator)), (a, b, op)
```

```python {file=build/q64.py}
import math
from fractions import Fraction

import numpy as np
from numba import njit

M = 2**63 - 1


<<q64 best approx>>


<<q64 scalar>>


<<q64 kernels>>
```
