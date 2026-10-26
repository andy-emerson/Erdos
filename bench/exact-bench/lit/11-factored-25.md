# 11. Pulling out the 2s and 5s

Measured decimals are $p/10^k$, and $10 = 2 \cdot 5$. This chapter tests a representation that keeps the powers of 2 and 5 as explicit exponents, separate from the rest of the number:

$$x = \frac{m}{d} \cdot 2^{a} \cdot 5^{b}, \qquad \gcd(m, 10) = \gcd(d, 10) = \gcd(m, d) = 1,\ d > 0,\ a, b \in \mathbb{Z}.$$

It is the partial form of the prime-exponent idea. Storing *every* prime's exponent makes addition need factoring. Storing just two primes' exponents, with an ordinary coprime fraction for the rest, keeps addition cheap, since it needs only shifts, multiplications by powers of 5, and stripping. Multiplication, division and scaling by powers of 10 become exponent arithmetic. The question is how much that gains on multiplicative work and costs on additive work.

## 11.1 Normal form

The representation is unique. Given any nonzero rational, strip every factor of 2 and 5 from its reduced numerator and denominator into $a$ and $b$. Zero is $m = 0$ with $d = 1$ and $a = b = 0$.

Stripping 2s is cheap: the number of trailing zero bits of $n$ is $\operatorname{bitlen}(n \wedge -n) - 1$. Stripping 5s takes one division per factor found, plus one more that fails. To make that faster for numbers with many factors of 5, we first try dividing by $5^{16}$ repeatedly, then by $5$.

```python {#f25 normal}
FIVE16 = 5**16


def strip2(n):
    if n == 0:
        return 0, 0
    k = (n & -n).bit_length() - 1
    return n >> k, k


def strip5(n):
    if n == 0:
        return 0, 0
    k = 0
    while n % FIVE16 == 0:
        n //= FIVE16
        k += 16
    while n % 5 == 0:
        n //= 5
        k += 1
    return n, k


def normalize(num, den, a, b):
    if den == 0:
        raise ZeroDivisionError("zero denominator")
    if num == 0:
        return 0, 1, 0, 0
    if den < 0:
        num, den = -num, -den
    num, k = strip2(num)
    a += k
    den, k = strip2(den)
    a -= k
    num, k = strip5(num)
    b += k
    den, k = strip5(den)
    b -= k
    g = math.gcd(num, den)
    return num // g, den // g, a, b
```

`strip2` on a negative $n$ works because $n \wedge -n$ is still the lowest set bit in two's complement.

## 11.2 Arithmetic

**Multiplication and division** never create new factors of 2 or 5 in $m$ or $d$, because products of numbers coprime to 10 are coprime to 10. So they need no stripping, only the cancellation that keeps $m/d$ reduced:

$$\left(\tfrac{m_1}{d_1} 2^{a_1} 5^{b_1}\right)\left(\tfrac{m_2}{d_2} 2^{a_2} 5^{b_2}\right) = \tfrac{m_1 m_2}{d_1 d_2}\, 2^{a_1 + a_2} 5^{b_1 + b_2}.$$

We cross-cancel, $g_1 = \gcd(m_1, d_2)$ and $g_2 = \gcd(m_2, d_1)$, before multiplying, as Python's `Fraction` does, so the comparison is like for like.

**Scaling by $10^k$**, or by any $2^i 5^j$, touches only the exponents. No big-integer work at all.

**Addition** must align the exponents. With $\alpha = \min(a_1, a_2)$ and $\beta = \min(b_1, b_2)$,

$$x_1 + x_2 = \frac{m_1 d_2\, 2^{a_1 - \alpha} 5^{b_1 - \beta} + m_2 d_1\, 2^{a_2 - \alpha} 5^{b_2 - \beta}}{d_1 d_2}\, 2^{\alpha} 5^{\beta},$$

after which the sum may again contain factors of 2 or 5, and is re-normalised. The powers of 2 are shifts. The powers of 5 are multiplications by a precomputable constant. The stripping step is the price.

```python {#f25 value}
class D25:
    __slots__ = ("m", "d", "a", "b")

    def __init__(self, num=0, den=1, a=0, b=0, normal=False):
        if normal:
            self.m, self.d, self.a, self.b = num, den, a, b
        else:
            self.m, self.d, self.a, self.b = normalize(num, den, a, b)

    @classmethod
    def from_fraction(cls, x):
        x = Fraction(x)
        return cls(x.numerator, x.denominator)

    @classmethod
    def decimal(cls, p, k):
        return cls(p, 1, -k, -k)

    def fraction(self):
        v = Fraction(self.m, self.d)
        v *= Fraction(2) ** self.a if self.a >= 0 else Fraction(1, 2 ** -self.a)
        v *= Fraction(5) ** self.b if self.b >= 0 else Fraction(1, 5 ** -self.b)
        return v

    def scale(self, i, j):
        if self.m == 0:
            return self
        return type(self)(self.m, self.d, self.a + i, self.b + j, normal=True)

    def __mul__(self, o):
        if self.m == 0 or o.m == 0:
            return type(self)()
        g1, g2 = math.gcd(self.m, o.d), math.gcd(o.m, self.d)
        return type(self)((self.m // g1) * (o.m // g2), (self.d // g2) * (o.d // g1),
                   self.a + o.a, self.b + o.b, normal=True)

    def __truediv__(self, o):
        if o.m == 0:
            raise ZeroDivisionError("division by zero")
        if self.m == 0:
            return type(self)()
        g1, g2 = math.gcd(self.m, o.m), math.gcd(o.d, self.d)
        num, den = (self.m // g1) * (o.d // g2), (self.d // g2) * (o.m // g1)
        if den < 0:
            num, den = -num, -den
        return type(self)(num, den, self.a - o.a, self.b - o.b, normal=True)

    def __add__(self, o):
        if self.m == 0:
            return o
        if o.m == 0:
            return self
        alpha, beta = min(self.a, o.a), min(self.b, o.b)
        t1 = (self.m * o.d) << (self.a - alpha)
        t2 = (o.m * self.d) << (o.a - alpha)
        if self.b > beta:
            t1 *= 5 ** (self.b - beta)
        if o.b > beta:
            t2 *= 5 ** (o.b - beta)
        return type(self)(t1 + t2, self.d * o.d, alpha, beta)

    def __neg__(self):
        return type(self)(-self.m, self.d, self.a, self.b, normal=True)

    def __sub__(self, o):
        return self + (-o)

    def __eq__(self, o):
        if not isinstance(o, D25):
            return NotImplemented
        return (self.m, self.d, self.a, self.b) == (o.m, o.d, o.a, o.b)

    def __hash__(self):
        return hash((self.m, self.d, self.a, self.b))

    def size_bits(self):
        return abs(self.m).bit_length() + self.d.bit_length() + abs(self.a).bit_length() + abs(self.b).bit_length() + 2
```

Because the normal form is unique, equality is a comparison of the four fields. `size_bits` counts the bits a value needs: numerator, denominator, and the two exponents with one sign bit each. `Fraction` sizes are counted as numerator plus denominator bits. Neither count includes the numerator's sign or any length fields, so for tiny values the difference between the two counts is partly bookkeeping. For values of tens of bits it is real. That shows whether pulling out 2s and 5s actually keeps numbers smaller, which decides how often they would outgrow a 64-bit word.

## 11.3 Two variants: 2s only, and lazy cleanup

The first quick run showed `D25` paying on addition. There are two candidate causes. Every sum is re-normalised, which strips 2s, which is cheap, but also strips 5s, which takes at least one big-integer division by 5 per sum even when there is nothing to strip. And aligning exponents multiplies by powers of 5. Two variants test each cause separately:

- **`D2`, 2s only.** $x = \frac{m}{d} \cdot 2^{a}$ with $m, d$ odd and coprime. Normalising is just counting trailing zero bits and shifting, so there is no division at all. Powers of 5 stay inside $m$ and $d$, so scaling by 10 becomes "shift the exponent and multiply by 5", and decimals keep their factors of 5 in the denominator.
- **`D25L`, lazy cleanup.** The same representation as `D25`, but addition skips stripping and the $\gcd$. A sum is left in any form with the right value, and it is normalised only when a canonical form is needed: comparison, or output. This is the same deferral that Q64 uses for its $\gcd$ (Chapter 3). The cost is that values can carry common factors between operations, so they may grow.

```python {#f25 variants}
def normalize2(num, den, a):
    if den == 0:
        raise ZeroDivisionError("zero denominator")
    if num == 0:
        return 0, 1, 0
    if den < 0:
        num, den = -num, -den
    num, k = strip2(num)
    a += k
    den, k = strip2(den)
    a -= k
    g = math.gcd(num, den)
    return num // g, den // g, a


class D2:
    __slots__ = ("m", "d", "a")

    def __init__(self, num=0, den=1, a=0, normal=False):
        if normal:
            self.m, self.d, self.a = num, den, a
        else:
            self.m, self.d, self.a = normalize2(num, den, a)

    @classmethod
    def decimal(cls, p, k):
        return cls(p, 5**k, -k) if k >= 0 else cls(p * 5 ** -k, 1, -k)

    def fraction(self):
        v = Fraction(self.m, self.d)
        return v * 2**self.a if self.a >= 0 else v / 2 ** -self.a

    def scale(self, i, j):
        if self.m == 0:
            return self
        if j >= 0:
            return type(self)(self.m * 5**j, self.d, self.a + i)
        return type(self)(self.m, self.d * 5 ** -j, self.a + i)

    def __mul__(self, o):
        if self.m == 0 or o.m == 0:
            return type(self)()
        g1, g2 = math.gcd(self.m, o.d), math.gcd(o.m, self.d)
        return type(self)((self.m // g1) * (o.m // g2), (self.d // g2) * (o.d // g1), self.a + o.a, normal=True)

    def __truediv__(self, o):
        if o.m == 0:
            raise ZeroDivisionError("division by zero")
        if self.m == 0:
            return type(self)()
        g1, g2 = math.gcd(self.m, o.m), math.gcd(o.d, self.d)
        num, den = (self.m // g1) * (o.d // g2), (self.d // g2) * (o.m // g1)
        if den < 0:
            num, den = -num, -den
        return type(self)(num, den, self.a - o.a, normal=True)

    def __add__(self, o):
        if self.m == 0:
            return o
        if o.m == 0:
            return self
        alpha = min(self.a, o.a)
        t1 = (self.m * o.d) << (self.a - alpha)
        t2 = (o.m * self.d) << (o.a - alpha)
        return type(self)(t1 + t2, self.d * o.d, alpha)

    def __neg__(self):
        return type(self)(-self.m, self.d, self.a, normal=True)

    def __sub__(self, o):
        return self + (-o)

    def size_bits(self):
        return abs(self.m).bit_length() + self.d.bit_length() + abs(self.a).bit_length() + 1


class D25L(D25):
    __slots__ = ()

    def __add__(self, o):
        if self.m == 0:
            return o
        if o.m == 0:
            return self
        alpha, beta = min(self.a, o.a), min(self.b, o.b)
        t1 = (self.m * o.d) << (self.a - alpha)
        t2 = (o.m * self.d) << (o.a - alpha)
        if self.b > beta:
            t1 *= 5 ** (self.b - beta)
        if o.b > beta:
            t2 *= 5 ** (o.b - beta)
        return D25L(t1 + t2, self.d * o.d, alpha, beta, normal=True)

    def normalized(self):
        return D25(self.m, self.d, self.a, self.b)
```

`D25L` inherits every method except addition. `D25`'s methods construct their results with `type(self)`, so a lazy value stays lazy through multiplication, division and scaling, with no extra object built. `D25L` keeps the value semantics of `D25` exactly: $x = \frac{m}{d} 2^a 5^b$ for whatever $m, d, a, b$ it holds. Multiplication's cross-cancellation and division remain correct without the normal form, because they use only that identity. `fraction()` is inherited and computes the value from the fields, so it needs no normal form either. Equality on lazy values must go through `normalized()` or `fraction()`, since two lazy representations of one value can differ.

## 11.4 A faster 2s-only addition

Because $m$ and $d$ are always odd, the 2s-only addition has structure that `D2` ignores. Write the two operands as $\frac{m_1}{d_1} 2^{a_1}$ and $\frac{m_2}{d_2} 2^{a_2}$, and let $\alpha = \min(a_1, a_2)$.

1. **Unequal exponents give an odd sum.** The aligned numerators are $m_1 d_2 \cdot 2^{a_1 - \alpha}$ and $m_2 d_1 \cdot 2^{a_2 - \alpha}$. Products of odd numbers are odd, so if $a_1 \ne a_2$ exactly one of the two terms is even, and odd plus even is odd. There is nothing to strip.
2. **Equal exponents give an even sum,** odd plus odd, unless it is zero. Stripping is then one "count trailing zeros" and one shift, and the count is at least 1.
3. **Alignment is a shift,** never a multiplication.
4. **The denominator $d_1 d_2$ is odd,** so no factor of 2 can enter it and it never needs stripping.
5. **Equal denominators skip the cross-multiplication.** When $d_1 = d_2$, which happens often for values of one column (in a running sum of three-place decimals, about two additions in three, since a decimal's denominator here is $5^3$, $5^2$, $5$ or $1$ depending on its numerator), the sum is $(m_1 2^{a_1 - \alpha} + m_2 2^{a_2 - \alpha})/d_1$, with no products at all.
6. **Use the lcm of the denominators, not their product.** With $g = \gcd(d_1, d_2)$ the sum is taken over $\operatorname{lcm}(d_1, d_2) = d_1 d_2 / g$, and the final reduction only needs $\gcd(s, g)$ rather than $\gcd(s, d_1 d_2)$. This is Knuth's method, and Python's `Fraction` uses it too. The $\gcd$ of two small denominators is far cheaper than one involving the numerator.

What remains is the small $\gcd$ that keeps $m/d$ reduced, the same cost a plain `Fraction` pays. `D2F` implements tricks 1 to 6. `D2FL` keeps the lcm denominator but skips the final $\gcd(s, g)$, so it defers only that last reduction.

```python {#f25 fast2}
class D2F(D2):
    __slots__ = ()

    def _sum(self, o):
        g = 1 if self.d == o.d else math.gcd(self.d, o.d)
        if self.d == o.d:
            t1, t2 = self.m, o.m
        else:
            t1, t2 = self.m * (o.d // g), o.m * (self.d // g)
        a1, a2 = self.a, o.a
        if a1 == a2:
            s = t1 + t2
            if s:
                k = (s & -s).bit_length() - 1
                s, a1 = s >> k, a1 + k
            a = a1
        elif a1 < a2:
            s, a = t1 + (t2 << (a2 - a1)), a1
        else:
            s, a = (t1 << (a1 - a2)) + t2, a2
        return s, g, a

    def __add__(self, o):
        if self.m == 0:
            return o
        if o.m == 0:
            return self
        s, g, a = self._sum(o)
        if s == 0:
            return type(self)()
        if self.d == o.d:
            g2 = math.gcd(s, self.d)
            return type(self)(s // g2, self.d // g2, a, normal=True)
        g2 = math.gcd(s, g)
        return type(self)(s // g2, (self.d // g) * (o.d // g2), a, normal=True)


class D2FL(D2F):
    __slots__ = ()

    def __add__(self, o):
        if self.m == 0:
            return o
        if o.m == 0:
            return self
        s, g, a = self._sum(o)
        if s == 0:
            return D2FL()
        den = self.d if self.d == o.d else (self.d // g) * o.d
        return D2FL(s, den, a, normal=True)

    def normalized(self):
        return D2(self.m, self.d, self.a)
```

`D2F` results are in `D2`'s normal form. `D2FL` sums keep $m$ odd and $d$ odd, so the power of two is always exact, but $m/d$ may share an odd factor until `normalized()`.

**What deferral costs.** A first draft of `D2FL` skipped *every* $\gcd$ in addition and multiplied the denominators. The independent review measured the result. In a running sum of three-place decimals, the denominator became the product of every term's denominator: 634 bits after 100 terms and 12,822 after 2,000. Each addition then multiplied ever-longer numbers, so the sum took quadratic time, 2.8× slower than `Fraction` at 2,500 terms and 12.8× at 20,000. Deferring the $\gcd$ is safe for Q64 only because its 64-bit bound forces a reduction regularly. Unbounded rationals have no such backstop. The current `D2FL` keeps the lcm denominator, so a running sum of decimals stays over at most $5^3$. What it defers, the common factor between the sum and $g$, is bounded by $g$. Even so, deferral has no theoretical guarantee in general. Cross-cancellation in a product also never removes a factor shared *within* one lazy operand.

## 11.5 Correctness tests

- **Normal form:** random rationals, including powers of 2 and 5 of both signs, round-trip through `D25` and back exactly. The stored $m$ and $d$ are coprime to 10 and to each other, $d > 0$, and zero is unique.
- **Arithmetic:** random chains of $+$, $-$, $\times$, $\div$ and `scale` on random operands, including decimals and values with large exponents, match `Fraction` exactly.
- **Stripping:** `strip5` on $5^{k} \cdot u$ for $k$ up to 100 recovers $k$ and $u$, including $k$ that is a multiple of 16 and negative $n$.
- **The variants:** the same random operation chains through `D2` and `D25L` must match `Fraction` exactly. In a 500-term running sum of mixed-scale decimals, every variant's denominator must stay within 40 bits, which catches unbounded deferral. For `D2F` and `D2FL`, sums must match `Fraction`, keep $m$ and $d$ odd, and be in `D2` normal form directly (`D2F`) or after `normalized()` (`D2FL`). A run of sums sharing one denominator exercises the equal-denominator path. `D2` values must stay in normal form ($m, d$ odd and coprime). `D25L` values must normalise to exactly the `D25` result.

```python {#tests f25}
def rand_rational(rng):
    kind = rng.randrange(4)
    if kind == 0:
        return Fraction(rng.randint(-10**8, 10**8), 10 ** rng.randint(0, 6))
    if kind == 1:
        return Fraction(rng.choice([-1, 1]) * 2 ** rng.randint(0, 80) * 5 ** rng.randint(0, 40),
                        2 ** rng.randint(0, 80) * 5 ** rng.randint(0, 40) * rng.choice([1, 3, 7, 21]))
    if kind == 2:
        return Fraction(rng.randint(-(2**90), 2**90), rng.randint(1, 2**90))
    return Fraction(0)


def check_normal(v):
    assert v.d > 0 and math.gcd(v.m, v.d) == 1
    if v.m == 0:
        assert (v.d, v.a, v.b) == (1, 0, 0)
    else:
        assert v.m % 2 and v.m % 5 and v.d % 2 and v.d % 5


def test_normal_form_roundtrip():
    rng = random.Random(80)
    for _ in range(4000):
        x = rand_rational(rng)
        v = D25.from_fraction(x)
        check_normal(v)
        assert v.fraction() == x
    assert D25.decimal(12500, 3).fraction() == Fraction(25, 2)
    assert D25.from_fraction(0) == D25()
    v = D25(3, -7)
    check_normal(v)
    assert v.fraction() == Fraction(-3, 7)


def test_arithmetic_matches_fraction():
    rng = random.Random(81)
    for _ in range(4000):
        x, y = rand_rational(rng), rand_rational(rng)
        u, v = D25.from_fraction(x), D25.from_fraction(y)
        op = rng.randrange(5)
        if op == 0:
            got, want = u + v, x + y
        elif op == 1:
            got, want = u - v, x - y
        elif op == 2:
            got, want = u * v, x * y
        elif op == 3:
            if y == 0:
                continue
            got, want = u / v, x / y
        else:
            i, j = rng.randint(-30, 30), rng.randint(-30, 30)
            got = u.scale(i, j)
            want = x * Fraction(2) ** i * Fraction(5) ** j
        check_normal(got)
        assert got.fraction() == want, (x, y, op)


def test_variants_match_fraction():
    rng = random.Random(83)
    for _ in range(3000):
        x, y = rand_rational(rng), rand_rational(rng)
        op = rng.randrange(4)
        if op == 3 and y == 0:
            continue
        want = [x + y, x - y, x * y, x / y if y else None][op]
        for C in (D2, D25L):
            u, v = C(x.numerator, x.denominator), C(y.numerator, y.denominator)
            got = [u + v, u - v, u * v, u / v if y else None][op]
            assert got.fraction() == want, (C.__name__, x, y, op)
            if C is D2:
                assert got.d > 0 and (got.m == 0 or (got.m % 2 and got.d % 2 and math.gcd(got.m, got.d) == 1))
            else:
                ref = [D25.from_fraction(x) + D25.from_fraction(y), D25.from_fraction(x) - D25.from_fraction(y),
                       D25.from_fraction(x) * D25.from_fraction(y), D25.from_fraction(x) / D25.from_fraction(y) if y else None][op]
                assert got.normalized() == ref
    for C in (D2F, D2FL):
        for _ in range(3000):
            x, y = rand_rational(rng), rand_rational(rng)
            u, v = C(x.numerator, x.denominator), C(y.numerator, y.denominator)
            s = u + v
            assert s.fraction() == x + y and s.d % 2 == 1
            if s.m:
                assert s.m % 2 == 1
            ref = D2((x + y).numerator, (x + y).denominator)
            n = s if C is D2F else s.normalized()
            assert (n.m, n.d, n.a) == (ref.m, ref.d, ref.a)
            assert (u * v).fraction() == x * y
            if y:
                assert (u / v).fraction() == x / y
        same = [C(rng.randint(-10**6, 10**6), 7, rng.randint(-5, 5)) for _ in range(200)]
        acc = C()
        for v in same:
            acc = acc + v
        assert acc.fraction() == sum((v.fraction() for v in same), Fraction(0))
    for C in (D2, D25L, D2F, D2FL):
        acc = C.decimal(0, 0)
        total = Fraction(0)
        for _ in range(500):
            p, k = rng.randint(-10**7, 10**7), rng.randint(-6, 6)
            acc = acc + C.decimal(p, k)
            total += Fraction(p, 10**k) if k >= 0 else Fraction(p * 10 ** -k)
            assert acc.d.bit_length() <= 40, (C.__name__, acc.d.bit_length())
        assert acc.fraction() == total
        if C is D2FL:
            n = acc.normalized()
            ref = D2(total.numerator, total.denominator)
            assert (n.m, n.d, n.a) == (ref.m, ref.d, ref.a)


def test_strip5():
    rng = random.Random(82)
    for k in list(range(0, 101)) + [16, 32, 48]:
        u = rng.choice([1, 3, 7, 11, 2**40 + 1]) * rng.choice([1, -1])
        assert strip5(u * 5**k) == (u, k)
    assert strip5(0) == (0, 0)
```

```python {file=build/factored25.py}
import math
from fractions import Fraction


<<f25 normal>>


<<f25 value>>


<<f25 variants>>


<<f25 fast2>>
```

```python {file=build/test_factored25.py}
import math
import random
from fractions import Fraction

from factored25 import D25, D2, D25L, D2F, D2FL, strip5


<<tests f25>>
```

## 11.6 The benchmark

Five workloads, each run with Python's `Fraction` and with the five factored types `D25`, `D2`, `D25L`, `D2F` and `D2FL`, both pure Python, so the comparison is between the two representations under the same interpreter. The times are not a forecast of compiled speed. Every result is checked against the `Fraction` result. Alongside the time, the table reports the mean and maximum `size_bits` of the results, against the bits of the equivalent reduced `Fraction`.

1. **F1, unit conversions.** $10^4$ measured three-place decimals, each multiplied by $1000$, divided by $3600$, multiplied by $1.609344$ (miles to km, exact), and divided by $10^6$, with all four constants built once outside the loop on both sides. **F1b** folds the four into one constant on both sides, as a compiler could.
2. **F2, repeated halving.** Each of $10^4$ decimals is halved 40 times, then multiplied by $5^{20}$ and divided by $1000$, in three versions so that effort is matched:
   - **F2a**, 40 general divisions by 2 on both sides, then a general multiplication and division;
   - **F2b**, the same 40 steps, but the factored types take their exponent shortcut for each literal power, as a compiler would, while `Fraction` has no such shortcut and divides;
   - **F2c**, both sides fold the whole sequence into one precomputed constant: `Fraction` multiplies by $5^{20}/(2^{40} \cdot 1000)$, and `D25` applies one `scale`.

   F2b against F2a shows what the shortcut is worth. F2c shows what remains when a compiler can fold constants on both sides.
3. **F3, sums of decimals.** Sum $10^5$ measured three-place decimals sequentially. Additive, with all values sharing $10^{-3}$, so exponent alignment is trivial, but `D25` re-strips every sum. `D2F` and `D2FL` strip only when exponents are equal.
4. **F4, sums of mixed scales.** Sum $10^5$ decimals with random scales from $10^{-6}$ to $10^{6}$, so alignment really is needed.
5. **F5, a mixed formula.** For each pair of decimals, $(x + y) \cdot x / 4 - y / 10$, a typical small expression mixing all four operations.

```python {#f25 bench}
def decimals(n, seed, places=3, spread=False):
    rng = random.Random(seed)
    out = []
    for _ in range(n):
        p = rng.randint(-10**7, 10**7) or 1
        k = places if not spread else rng.randint(-6, 6)
        out.append((p, k))
    return out


def as_fraction(p, k):
    return Fraction(p, 10**k) if k >= 0 else Fraction(p * 10 ** -k)


def halve_steps(x, two, k5_20, k1000):
    for _ in range(40):
        x = x / two
    return x * k5_20 / k1000


def halve_scale(x):
    for _ in range(40):
        x = x.scale(-1, 0)
    return x.scale(0, 20).scale(-3, -3)


def running_sum(values, zero):
    acc = zero
    for v in values:
        acc = acc + v
    return acc


def build(n_small, n_big):
    small = decimals(n_small, 90)
    big = decimals(n_big, 91)
    spread = decimals(n_big, 92, spread=True)
    pairs = list(zip(decimals(n_small, 93), decimals(n_small, 94)))
    F = Fraction
    f1000, f3600, fmile, f1e6, two, k520, four, ten = (F(1000), F(3600), F(1609344, 10**6), F(10**6),
                                                     F(2), F(5**20), F(4), F(10))
    fconv = f1000 / f3600 * fmile / f1e6
    folded = Fraction(5**20, 2**40 * 1000)
    mk = as_fraction
    work = {
        "Fraction": {
            "F1 unit conversions": lambda: [mk(p, k) * f1000 / f3600 * fmile / f1e6 for p, k in small],
            "F1b unit conversion folded into one constant": lambda: [mk(p, k) * fconv for p, k in small],
            "F2a 40 halvings, general ops": lambda: [halve_steps(mk(p, k), two, k520, f1000) for p, k in small],
            "F2b 40 halvings, exponent shortcut": lambda: [halve_steps(mk(p, k), two, k520, f1000) for p, k in small],
            "F2c halvings folded into one constant": lambda: [mk(p, k) * folded for p, k in small],
            "F3 sum of 3-place decimals": lambda: [running_sum((mk(p, k) for p, k in big), F(0))],
            "F4 sum of mixed scales": lambda: [running_sum((mk(p, k) for p, k in spread), F(0))],
            "F5 mixed formula": lambda: [(mk(*x) + mk(*y)) * mk(*x) / four - mk(*y) / ten for x, y in pairs],
        }}
    for name, C in (("2s and 5s factored", D25), ("2s only", D2), ("2s and 5s, lazy cleanup", D25L),
                    ("2s only, fast add", D2F), ("2s only, fast add, lazy gcd", D2FL)):
        d = C.decimal
        c1000, c3600, cmile, c1e6, c2, c520, c4, c10 = (d(1000, 0), d(3600, 0), d(1609344, 6), d(1, -6),
                                                       d(2, 0), d(5**20, 0), d(4, 0), d(10, 0))
        cconv = c1000 / c3600 * cmile / c1e6
        work[name] = {
            "F1 unit conversions": lambda d=d, a=c1000, b=c3600, c=cmile, e=c1e6: [d(p, k) * a / b * c / e for p, k in small],
            "F1b unit conversion folded into one constant": lambda d=d, cc=cconv: [d(p, k) * cc for p, k in small],
            "F2a 40 halvings, general ops": lambda d=d, a=c2, b=c520, c=c1000: [halve_steps(d(p, k), a, b, c) for p, k in small],
            "F2b 40 halvings, exponent shortcut": lambda d=d: [halve_scale(d(p, k)) for p, k in small],
            "F2c halvings folded into one constant": lambda d=d: [d(p, k).scale(-43, 17) for p, k in small],
            "F3 sum of 3-place decimals": lambda d=d: [running_sum((d(p, k) for p, k in big), d(0, 0))],
            "F4 sum of mixed scales": lambda d=d: [running_sum((d(p, k) for p, k in spread), d(0, 0))],
            "F5 mixed formula": lambda d=d, a=c4, b=c10: [(d(*x) + d(*y)) * d(*x) / a - d(*y) / b for x, y in pairs],
        }
    return work


def frac_bits(x):
    return abs(x.numerator).bit_length() + x.denominator.bit_length()


def bench_f25(report, n_small, n_big):
    work = build(n_small, n_big)
    for label in work["Fraction"]:
        res = measure({name: w[label] for name, w in work.items()}, gc_off=False)
        fo = work["Fraction"][label]()
        size = n_big if label.startswith(("F3", "F4")) else n_small
        fb = [frac_bits(f) for f in fo]
        extra = {"Fraction": dict(error="0 (exact)", note=f"result bits mean {statistics.mean(fb):.0f}, max {max(fb)}")}
        for name, w in work.items():
            if name == "Fraction":
                continue
            out = w[label]()
            ok = len(out) == len(fo) and all(x.fraction() == f for x, f in zip(out, fo))
            bits = [x.size_bits() for x in out]
            extra[name] = dict(error=verify(ok, f"{label} {name}", "0 (matches Fraction)"),
                               note=f"result bits mean {statistics.mean(bits):.0f}, max {max(bits)}")
        report.add_measured(label, size, res, "Fraction", **extra)


def main():
    print("correctness tests")
    import test_factored25
    for name in sorted(n for n in dir(test_factored25) if n.startswith("test_")):
        t = time.perf_counter()
        getattr(test_factored25, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    quick = "--quick" in sys.argv
    meta = metadata()
    report = Report()
    bench_f25(report, 500 if quick else 10**4, 5000 if quick else 10**5)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / f"factored25{suffix}.md").write_text(f"# Factoring out 2s and 5s\n\n{header}\n\n{report.markdown()}\n")
    (out / f"factored25{suffix}.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

F1 and F3 to F5 use only general operations on both sides. Only F2b uses `D25`'s exponent shortcut, and it is labelled as such. An independent review found that an earlier draft folded the 40 halvings into a single shortcut on the `D25` side only, which made `D25` look 20× faster than a like-for-like comparison supports.

```python {file=build/run_factored25.py}
import statistics

from run_benchmarks import *
from run_benchmarks import metadata, verify

from factored25 import D25, D2, D25L, D2F, D2FL


<<f25 bench>>
```
