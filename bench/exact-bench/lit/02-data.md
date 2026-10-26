# 2. Test data

The case for exact arithmetic rests on a claim about data: values that come from measurement are finite decimals, hence rational, hence exactly representable as $p/10^k$. The test data is built to match that claim honestly, without tilting the field.

## 2.1 Measured decimals

A measured quantity recorded to $k$ decimal places with magnitude at most $m$ is a number

$$x = \frac{p}{10^k}, \qquad p \in \mathbb{Z},\ |p| \le m \cdot 10^k.$$

We generate $p$ uniformly in that range with a fixed seed. The exact side stores $p$ and the shared denominator $10^k$. The float side stores $\operatorname{fl}(p / 10^k)$, the binary64 number nearest to $x$.

That float is exactly what a user gets by parsing the decimal string. Both $p$ and $10^k$ are integers below $2^{53}$, so each converts to binary64 without error, and IEEE 754 division is correctly rounded. So `p / 10**k` in NumPy and `float("123.456")` agree to the bit, and neither side is handicapped by a careless conversion. The cost of converting is excluded from every timing on both sides, because a real system pays it once at load time.

The defaults, $k = 3$ and $m = 10^4$, give numerators up to $10^7 < 2^{24}$, comfortably inside `int64` and typical of sensor readings, prices, and lab data.

```python {#data decimal column}
def decimal_column(n, digits=3, magnitude=10**4, seed=0):
    rng = np.random.default_rng(seed)
    den = 10**digits
    bound = magnitude * den
    nums = rng.integers(-bound, bound + 1, size=n, dtype=np.int64)
    return nums, den, nums / den
```

## 2.2 A hard case for floats: a large offset

Variance is computed from differences from the mean. When every value sits close to a large common offset, say temperatures in kelvin to the millikelvin, or timestamps, the float computation of $\sum x_i^2 - \tfrac{1}{n}(\sum x_i)^2$ cancels catastrophically, and even the better two-pass algorithm loses digits to the representation error of each $x_i$. We include such a column, values $10^6 + u$ with $u$ a three-place decimal in $[-1, 1]$, because it is a realistic shape and because it is exactly where an exact system should win. It is labelled as such in the results, not mixed in with the ordinary case.

```python {#data offset column}
def offset_column(n, digits=3, offset=10**6, noise=1, seed=1):
    rng = np.random.default_rng(seed)
    den = 10**digits
    nums = offset * den + rng.integers(-noise * den, noise * den + 1, size=n, dtype=np.int64)
    return nums, den, nums / den
```

## 2.3 Group keys

Keys are integers in $\{0, \dots, G-1\}$, drawn uniformly. Uniform keys are the neutral choice. Skewed keys would change cache behaviour, but equally for both sides.

```python {#data keys}
def group_keys(n, groups, seed=2):
    rng = np.random.default_rng(seed)
    return rng.integers(0, groups, size=n, dtype=np.int64)
```

## 2.4 Matrices of measured decimals

A matrix of measured decimals is $A = N / 10^k$ with $N$ an integer matrix. For linear algebra the shared denominator factors out entirely: $Ax = b$ with $b = M/10^k$ is the same system as $Nx = M$. FLINT works on $N$ and $M$ directly, and NumPy works on $\operatorname{fl}(N/10^k)$ and $\operatorname{fl}(M/10^k)$, so both solve the same mathematical problem from their natural representation.

Uniform random matrices are well conditioned with high probability. They are the friendly case for floats, which is deliberate: the linear algebra benchmarks measure speed under ordinary conditions, and the Hilbert matrix in Section 2.5 measures what happens when conditioning is bad.

```python {#data matrix}
def decimal_matrix(rows, cols, digits=3, magnitude=10**3, seed=3):
    rng = np.random.default_rng(seed)
    den = 10**digits
    bound = magnitude * den
    nums = rng.integers(-bound, bound + 1, size=(rows, cols), dtype=np.int64)
    return nums, den, nums / den
```

## 2.5 The Hilbert matrix

The Hilbert matrix $H_{ij} = 1/(i + j - 1)$ is the classic ill-conditioned example. Its condition number grows like $e^{3.5n}$, so binary64 loses all significant digits around $n = 12$. Its entries are not finite decimals, so the float version carries a representation error in every entry even before any arithmetic. That is realistic too, since rational coefficients like $1/3$ arise constantly in discretised models.

We choose the right-hand side $b = H\mathbf{1}$, computed exactly, so the true solution is the all-ones vector and the error is easy to read. The float versions of $H$ and $b$ are made by converting each exact entry through Python's `Fraction`, whose conversion to float is correctly rounded, so each float entry is the nearest binary64 to the exact value.

```python {#data hilbert}
def hilbert_system(n):
    H = flint.fmpq_mat.hilbert(n, n)
    b = H * flint.fmpq_mat(n, 1, [1] * n)
    Hf = np.array([[float(Fraction(int(H[i, j].p), int(H[i, j].q))) for j in range(n)] for i in range(n)])
    bf = np.array([float(Fraction(int(b[i, 0].p), int(b[i, 0].q))) for i in range(n)])
    return H, b, Hf, bf
```

## 2.6 Elements of $\mathbb{Q}(\sqrt 2)$

For the symbolic chapter we need columns of numbers $a + b\sqrt2$ with $a, b$ measured decimals. We draw $a$ and $b$ independently with a shared denominator. The float side stores $\operatorname{fl}(a + b\sqrt2)$, computed as `a + b * sqrt(2)` in binary64, which is what a user of floats would write.

```python {#data qsqrt2}
def qsqrt2_column(n, digits=3, magnitude=10**3, seed=4):
    rng = np.random.default_rng(seed)
    den = 10**digits
    bound = magnitude * den
    a = rng.integers(-bound, bound + 1, size=n, dtype=np.int64)
    b = rng.integers(-bound, bound + 1, size=n, dtype=np.int64)
    return a, b, den, a / den + (b / den) * np.sqrt(2.0)
```

```python {file=build/data.py}
from fractions import Fraction

import numpy as np
import flint


<<data decimal column>>


<<data offset column>>


<<data keys>>


<<data matrix>>


<<data hilbert>>


<<data qsqrt2>>
```
