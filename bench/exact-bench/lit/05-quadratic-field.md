# 5. A symbolic column type: $\mathbb{Q}(\sqrt 2)$

The language design calls for symbolic values at the low level: $\sqrt2$ should stay $\sqrt2$ until it simplifies, so that $\sqrt2 \cdot \sqrt2$ is exactly $2$ rather than $2.0000000000000004$. General symbolic expressions vary in size, which would break the rule that a column holds one fixed-width type. The way around this is to fix an algebraic number field and make that the column type.

## 5.1 The field

$$\mathbb{Q}(\sqrt2) = \{\, a + b\sqrt2 : a, b \in \mathbb{Q} \,\}$$

is a field, and a two-dimensional vector space over $\mathbb{Q}$ with basis $\{1, \sqrt2\}$. Every element has a unique coordinate pair $(a, b)$, because $\sqrt2$ is irrational. So equality is coordinate equality, and "is this value rational?" is the test $b = 0$. Arithmetic is closed:

$$
\begin{aligned}
(a_1 + b_1\sqrt2) + (a_2 + b_2\sqrt2) &= (a_1 + a_2) + (b_1 + b_2)\sqrt2, \\
(a_1 + b_1\sqrt2)(a_2 + b_2\sqrt2) &= (a_1a_2 + 2b_1b_2) + (a_1b_2 + a_2b_1)\sqrt2, \\
(a + b\sqrt2)^{-1} &= \frac{a - b\sqrt2}{a^2 - 2b^2}.
\end{aligned}
$$

The denominator of the inverse is the field norm $N(a + b\sqrt2) = a^2 - 2b^2$. It is nonzero for every nonzero element, since $a^2 = 2b^2$ with $b \neq 0$ would make $\sqrt2 = |a/b|$ rational.

The column representation is a shared-denominator pair: integer vectors $p, q$ and a denominator $d$ with $x_i = (p_i + q_i\sqrt2)/d$. That is 16 bytes per value, and it is fixed width, so the column stays uniform.

This is the simplest case of a general construction. Any number field $\mathbb{Q}(\alpha)$ of degree $n$ gives an $n$-word fixed-width type with exact arithmetic, so the same design covers $\mathbb{Q}(\sqrt3)$, $\mathbb{Q}(i)$ for exact Gaussian rationals, or $\mathbb{Q}(\zeta_8)$ for exact 45° rotations.

## 5.2 Scalar arithmetic

The scalar class uses `Fraction` coordinates. It is used by the tests and as the reference for the kernel.

```python {#qfield scalar}
class QSqrt2:
    __slots__ = ("a", "b")

    def __init__(self, a, b=0):
        self.a, self.b = Fraction(a), Fraction(b)

    def __add__(self, o):
        o = lift(o)
        return QSqrt2(self.a + o.a, self.b + o.b)

    def __sub__(self, o):
        o = lift(o)
        return QSqrt2(self.a - o.a, self.b - o.b)

    def __mul__(self, o):
        o = lift(o)
        return QSqrt2(self.a * o.a + 2 * self.b * o.b, self.a * o.b + o.a * self.b)

    def norm(self):
        return self.a * self.a - 2 * self.b * self.b

    def inverse(self):
        n = self.norm()
        return QSqrt2(self.a / n, -self.b / n)

    def __truediv__(self, o):
        return self * lift(o).inverse()

    def __eq__(self, o):
        o = lift(o)
        return self.a == o.a and self.b == o.b

    def is_rational(self):
        return self.b == 0

    def __repr__(self):
        return f"({self.a} + {self.b}*sqrt2)"


def lift(x):
    return x if isinstance(x, QSqrt2) else QSqrt2(x)


SQRT2 = QSqrt2(0, 1)
```

## 5.3 The inner-product kernel

The benchmark operation is the inner product $\langle x, y \rangle = \sum_i x_i y_i$, the building block of matrix multiplication and least squares. With $x_i = (p_i + q_i\sqrt2)/d_x$ and $y_i = (r_i + s_i\sqrt2)/d_y$,

$$\langle x, y \rangle = \frac{1}{d_x d_y}\left[\ \sum_i (p_i r_i + 2 q_i s_i) \;+\; \sqrt2 \sum_i (p_i s_i + q_i r_i)\ \right].$$

Two integer sums, each over products. We use the same blocking idea as the column sum in Section 4.3. If every coordinate satisfies $|\cdot| < 2^{24}$, then each summand satisfies

$$|p_i r_i + 2 q_i s_i| < 3 \cdot 2^{48} < 2^{50},$$

and a block of 4096 summands totals less than $2^{62}$. No check is needed inside the block, so the loop vectorises. Totals of good blocks are combined with checked additions. Bad blocks, and the case where the combined totals overflow, fall back to exact Python integers per block, as in Section 4.3.

```python {#qfield dot}
DOT_LIMIT = 2**24


@njit
def dot_kernel(p, q, r, s):
    n = len(p)
    nblocks = (n + BLOCK - 1) // BLOCK
    block_a = np.zeros(nblocks, dtype=np.int64)
    block_b = np.zeros(nblocks, dtype=np.int64)
    bad = np.zeros(nblocks, dtype=np.bool_)
    A = np.int64(0)
    B = np.int64(0)
    total_ovf = False
    for blk in range(nblocks):
        acc_a = np.int64(0)
        acc_b = np.int64(0)
        flag = False
        for i in range(blk * BLOCK, min((blk + 1) * BLOCK, n)):
            flag |= (p[i] >= DOT_LIMIT) | (p[i] <= -DOT_LIMIT) | (q[i] >= DOT_LIMIT) | (q[i] <= -DOT_LIMIT)
            flag |= (r[i] >= DOT_LIMIT) | (r[i] <= -DOT_LIMIT) | (s[i] >= DOT_LIMIT) | (s[i] <= -DOT_LIMIT)
            acc_a += p[i] * r[i] + 2 * q[i] * s[i]
            acc_b += p[i] * s[i] + q[i] * r[i]
        block_a[blk] = acc_a
        block_b[blk] = acc_b
        bad[blk] = flag
        if not flag:
            A, o1 = add_ovf(A, acc_a)
            B, o2 = add_ovf(B, acc_b)
            total_ovf |= o1 | o2
    return A, B, total_ovf, block_a, block_b, bad


def qsqrt2_dot(p, q, dx, r, s, dy):
    A, B, total_ovf, block_a, block_b, bad = dot_kernel(p, q, r, s)
    if total_ovf:
        good = ~bad
        A = sum(map(int, block_a[good].tolist()))
        B = sum(map(int, block_b[good].tolist()))
    else:
        A, B = int(A), int(B)
    for blk in np.flatnonzero(bad):
        sl = slice(blk * BLOCK, (blk + 1) * BLOCK)
        P, Q, R, S = (list(map(int, v[sl].tolist())) for v in (p, q, r, s))
        A += sum(a * c + 2 * b * d for a, b, c, d in zip(P, Q, R, S))
        B += sum(a * d + b * c for a, b, c, d in zip(P, Q, R, S))
    return QSqrt2(Fraction(A, dx * dy), Fraction(B, dx * dy))
```

The bound is tested as two comparisons, $v \ge 2^{24}$ or $v \le -2^{24}$, rather than as $|v| \ge 2^{24}$. In two's complement $|{-2^{63}}|$ wraps around to $-2^{63}$, which would slip past an `abs` test. The comparison form has no such hole.

## 5.4 Measuring the float error against an irrational answer

The exact answer $A + B\sqrt2$ is irrational whenever $B \ne 0$, so it has no exact float or `Fraction` form to compare against. Instead we evaluate it in ball arithmetic with FLINT's `arb` type at 256 bits of precision. A ball is a midpoint with a rigorous radius, so the evaluation comes with a guaranteed error bound, far below anything binary64 can resolve. The float answer is then compared against the ball's midpoint.

```python {#qfield arb}
def qsqrt2_to_arb(x, prec=256):
    old = flint.ctx.prec
    flint.ctx.prec = prec
    try:
        a = flint.arb(x.a.numerator) / x.a.denominator
        b = flint.arb(x.b.numerator) / x.b.denominator
        return a + b * flint.arb(2).sqrt()
    finally:
        flint.ctx.prec = old


def qsqrt2_rel_error(approx, exact):
    old = flint.ctx.prec
    flint.ctx.prec = 256
    try:
        truth = qsqrt2_to_arb(exact)
        err = abs((flint.arb(float(approx)) - truth) / truth)
        return float(err.mid())
    finally:
        flint.ctx.prec = old
```

## 5.5 Float control

As in Section 4.7, the float control is allowed to reassociate so that it can vectorise like the integer kernel.

```python {#qfield float control}
@njit(fastmath={"reassoc", "contract"})
def f_dot(x, y):
    s = 0.0
    for i in range(len(x)):
        s += x[i] * y[i]
    return s
```

## 5.6 Correctness tests

The defining identities first: $\sqrt2 \cdot \sqrt2 = 2$ exactly and collapses to a rational, $(1 + \sqrt2)(\sqrt2 - 1) = 1$, and every nonzero element times its inverse is $1$. Then the kernel against the scalar class: on ordinary data; on large data where every block falls back; with a single $-2^{63}$ or a single oversized value in otherwise small data, where exactly one block must fall back; and on data where every block is good but the combined totals overflow.

```python {#tests qfield}
def test_qsqrt2_identities():
    assert SQRT2 * SQRT2 == 2 and (SQRT2 * SQRT2).is_rational()
    assert (QSqrt2(1, 1) * QSqrt2(-1, 1)) == 1
    rng = random.Random(30)
    for _ in range(500):
        x = QSqrt2(Fraction(rng.randint(-99, 99), rng.randint(1, 99)), Fraction(rng.randint(-99, 99), rng.randint(1, 99)))
        if x.a == 0 and x.b == 0:
            continue
        assert x * x.inverse() == 1


def qsqrt2_reference(p, q, dx, r, s, dy):
    want = QSqrt2(0)
    for i in range(len(p)):
        want = want + QSqrt2(Fraction(int(p[i]), dx), Fraction(int(q[i]), dx)) * QSqrt2(Fraction(int(r[i]), dy), Fraction(int(s[i]), dy))
    return want


def test_qsqrt2_dot():
    rng = random.Random(31)
    n = 9000
    for case in ("small", "big", "min", "one bad"):
        hi = 2**40 if case == "big" else 10**6
        p, q, r, s = (np.array([rng.randint(-hi, hi) for _ in range(n)], dtype=np.int64) for _ in range(4))
        if case == "min":
            p[17] = INT64_MIN
        if case == "one bad":
            q[BLOCK + 3] = 2**30
        if case in ("min", "one bad"):
            assert dot_kernel(p, q, r, s)[5].sum() == 1
        assert qsqrt2_dot(p, q, 1000, r, s, 100) == qsqrt2_reference(p, q, 1000, r, s, 100)


def test_qsqrt2_dot_total_overflow():
    n = 4 * BLOCK
    v = np.full(n, 2**24 - 1, dtype=np.int64)
    A, B, total_ovf, _, _, bad = dot_kernel(v, v, v, v)
    assert total_ovf and not bad.any()
    k = (2**24 - 1) ** 2
    assert qsqrt2_dot(v, v, 1, v, v, 1) == QSqrt2(3 * k * n, 2 * k * n)
```

```python {file=build/qfield.py}
from fractions import Fraction

import flint
import numpy as np
from numba import njit

from q64 import add_ovf
from columns import BLOCK


<<qfield scalar>>


<<qfield dot>>


<<qfield arb>>


<<qfield float control>>
```
