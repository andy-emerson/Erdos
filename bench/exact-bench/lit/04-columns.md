# 4. Exact columns

A table is a dictionary of columns, and every column holds one type. This chapter builds the numeric column types and the four column operations the benchmarks need: sum and mean, an elementwise fused multiply–add, group-by sum, and variance.

## 4.1 Two column representations

A **shared-denominator column** stores a vector $x \in \mathbb{Q}^n$ as an integer vector $p \in \mathbb{Z}^n$ and a single positive integer $d$, with $x_i = p_i / d$. A column of measured decimals recorded to $k$ places is naturally of this form with $d = 10^k$, and it costs 8 bytes per value, the same as binary64. Arithmetic on such a column is integer arithmetic on $p$, which compiles to the same vector instructions as float arithmetic.

A **Q64 column** stores a numerator and a denominator per element, 16 bytes per value. It is the fallback when elements genuinely need different denominators. Here that happens when an operation overflows on some elements and those elements must be rounded individually by $\rho$.

The rule is that every operation returns a shared-denominator column when it can, and promotes to a Q64 column only for the elements that need it. The benchmarks report when promotion happened.

```python {#columns types}
class SharedColumn:
    def __init__(self, nums, den):
        assert nums.dtype == np.int64 and 0 < den <= M
        self.nums, self.den = nums, den

    def __len__(self):
        return len(self.nums)

    def value(self, i):
        return Fraction(int(self.nums[i]), self.den)

    bytes_per_value = 8


class Q64Column:
    def __init__(self, nums, dens):
        assert nums.dtype == np.int64 and dens.dtype == np.int64
        self.nums, self.dens = nums, dens

    def __len__(self):
        return len(self.nums)

    def value(self, i):
        return Fraction(int(self.nums[i]), int(self.dens[i]))

    bytes_per_value = 16
```

## 4.2 Fast paths, limits, and fallbacks

Every kernel in this chapter has a compiled fast path that is valid only while the numerators stay below a bound. The bounds come from the requirement that no 64-bit intermediate can overflow, and each is derived where its kernel is defined. They are collected here because they decide when exact arithmetic stays fast:

| Kernel | Fast path requires | Benchmark data | Example of data past the limit |
|---|---|---|---|
| Sum, mean | $\lvert p_i \rvert < 2^{51} \approx 2.3 \times 10^{15}$ | $\le 10^7$ | rarely reached |
| Variance | $\lvert p_i \rvert < 2^{32} \approx 4.3 \times 10^{9}$ | $\le 10^7$, offset column $\approx 10^9$ | timestamps in milliseconds, $\approx 1.7 \times 10^{12}$ |
| $ab + c$ | every product and sum below $2^{62}$ | products $\le 10^{14}$ | six-place decimals of magnitude $10^4$: products $\approx 10^{20}$ |
| Group-by sum | group totals below $2^{63}$ | totals $\le 10^{11}$ | rarely reached |
| $\mathbb{Q}(\sqrt2)$ inner product | coordinates $\lvert \cdot \rvert < 2^{24} \approx 1.7 \times 10^{7}$ | $\le 10^6$ | six-place decimals of magnitude 17 or more |

When data crosses a limit, the affected work falls back to Python's unbounded integers, which is exact but roughly a hundred times slower. To keep that cliff local, the reductions (sum, variance, inner product) fall back **per block** of 4096 elements, not per column. One oversized value costs one slow block, not a slow column. The multiply–add falls back per element. Chapter 6 includes runs just past the variance and multiply–add limits, so the size of the cliff is measured rather than hidden.

A production version would replace the Python fallback with a compiled 128-bit path: a double-word accumulator, as in the variance kernel below. The fallback here is a placeholder for that, chosen because it is obviously correct.

## 4.3 Sum and mean

For a shared-denominator column,

$$\sum_i x_i = \frac{1}{d}\sum_i p_i, \qquad \bar x = \frac{\sum_i p_i}{n d}.$$

The work is one integer sum. The difficulty is doing it fast *and* detecting overflow. A per-element overflow test creates a dependency between consecutive additions, which prevents the compiler from vectorising the loop. So we work in blocks of $b = 4096$ elements and use a bound instead of a test inside each block. If every $|p_i| < 2^{51}$, then any partial sum within a block satisfies

$$\Bigl|\sum_{i \in \text{block}} p_i\Bigr| < 4096 \cdot 2^{51} = 2^{63},$$

so no partial sum can overflow. The inner loop is then a plain sum plus an elementwise comparison, both of which vectorise. Block totals from good blocks are combined with the checked addition from Section 3.5, one check per 4096 elements.

The kernel returns the combined total of the good blocks, whether that total overflowed, every block total, and a flag per block. The Python wrapper then handles three cases. If no block is bad and the total did not overflow, which is the normal case, the answer is the total. Bad blocks are summed with Python integers and added in. If the combined total overflowed, the good block totals are re-added in Python integers, a few thousand additions.

```python {#columns sum}
BLOCK = 4096
BLOCK_LIMIT = 2**51


@njit
def sum_kernel(nums):
    n = len(nums)
    nblocks = (n + BLOCK - 1) // BLOCK
    block_sums = np.zeros(nblocks, dtype=np.int64)
    bad = np.zeros(nblocks, dtype=np.bool_)
    total = np.int64(0)
    total_ovf = False
    for b in range(nblocks):
        acc = np.int64(0)
        flag = False
        for i in range(b * BLOCK, min((b + 1) * BLOCK, n)):
            v = nums[i]
            acc += v
            flag |= (v >= BLOCK_LIMIT) | (v <= -BLOCK_LIMIT)
        block_sums[b] = acc
        bad[b] = flag
        if not flag:
            total, o = add_ovf(total, acc)
            total_ovf |= o
    return total, total_ovf, block_sums, bad


def exact_int_sum(nums):
    total, total_ovf, block_sums, bad = sum_kernel(nums)
    if total_ovf:
        result = sum(int(s) for s, f in zip(block_sums.tolist(), bad.tolist()) if not f)
    else:
        result = int(total)
    for b in np.flatnonzero(bad):
        result += sum(map(int, nums[b * BLOCK:(b + 1) * BLOCK].tolist()))
    return result


def column_sum(col):
    return Fraction(exact_int_sum(col.nums), col.den)


def column_mean(col):
    return Fraction(exact_int_sum(col.nums), len(col) * col.den)
```

When `total_ovf` is set, the compiled `total` is garbage and is discarded. The block totals of good blocks are still exact, because each was bounded below $2^{63}$.

The results are returned as exact `Fraction`s. A scalar is one value, so its cost is negligible, and keeping it exact lets the benchmark measure the float error precisely. Storing it back into a table would apply $\rho$ once.

## 4.4 Variance in one pass with a split accumulator

The unbiased sample variance of $x_i = p_i/d$ is

$$s^2 = \frac{n \sum p_i^2 - \left(\sum p_i\right)^2}{n(n-1)\,d^2}.$$

In floating point this one-pass formula is notorious, because the two terms in the numerator can be nearly equal and cancel catastrophically. That is why NumPy uses a two-pass algorithm instead. In exact arithmetic cancellation is harmless, so the one-pass formula is the right choice: one sweep over the data computes both $\sum p_i$ and $\sum p_i^2$, with no intermediate mean.

The catch is size. With $|p_i|$ up to $10^9$, each square is up to $10^{18}$ and the sum of $10^7$ of them is about $10^{25}$, beyond 64 bits. We need a wider accumulator, and Numba, like WebAssembly, has no 128-bit integer. So we build one from two 64-bit words without any carry logic. If $|p_i| < 2^{32}$ then $p_i^2 < 2^{64}$ fits an unsigned word, and we split it as

$$p_i^2 = 2^{32} h_i + \ell_i, \qquad 0 \le h_i, \ell_i < 2^{32}.$$

Summing the $h_i$ and the $\ell_i$ separately, each total stays below $n \cdot 2^{32}$, which fits in an unsigned 64-bit word for any $n < 2^{32}$. The plain sum $\sum p_i$ is accumulated alongside in a signed word, and stays below $n \cdot 2^{32} < 2^{63}$ for $n < 2^{31}$. Then

$$\sum p_i^2 = 2^{32} \sum h_i + \sum \ell_i,$$

recombined exactly in Python. Each step in the loop is a multiply, a shift, a mask, and three adds, all independent across $i$, so the loop vectorises. Because these bounds hold for the whole column, the totals over good blocks can never overflow, and only the bad blocks, those containing an element with $|p_i| \ge 2^{32}$, need the Python fallback.

One trap: Numba promotes mixed `int64`/`uint64` arithmetic to `float64`, which would silently destroy exactness. Every quantity in the squares accumulator is therefore explicitly `uint64`.

```python {#columns variance}
SQ_LIMIT = 2**32
LOW_MASK = np.uint64(0xFFFFFFFF)
SHIFT = np.uint64(32)


@njit
def moments_kernel(nums):
    n = len(nums)
    nblocks = (n + BLOCK - 1) // BLOCK
    bad = np.zeros(nblocks, dtype=np.bool_)
    s1 = np.int64(0)
    hi = np.uint64(0)
    lo = np.uint64(0)
    for b in range(nblocks):
        a1 = np.int64(0)
        bh = np.uint64(0)
        bl = np.uint64(0)
        flag = False
        for i in range(b * BLOCK, min((b + 1) * BLOCK, n)):
            v = nums[i]
            flag |= (v >= SQ_LIMIT) | (v <= -SQ_LIMIT)
            a1 += v
            a = np.uint64(abs(v))
            sq = a * a
            bh += sq >> SHIFT
            bl += sq & LOW_MASK
        bad[b] = flag
        if not flag:
            s1 += a1
            hi += bh
            lo += bl
    return s1, hi, lo, bad


def exact_moments(nums):
    assert len(nums) < 2**31
    s1, hi, lo, bad = moments_kernel(nums)
    s1, s2 = int(s1), (int(hi) << 32) + int(lo)
    for b in np.flatnonzero(bad):
        block = list(map(int, nums[b * BLOCK:(b + 1) * BLOCK].tolist()))
        s1 += sum(block)
        s2 += sum(v * v for v in block)
    return s1, s2


def column_var(col):
    n = len(col)
    s1, s2 = exact_moments(col.nums)
    return Fraction(n * s2 - s1 * s1, n * (n - 1) * col.den**2)
```

## 4.5 Elementwise $a \cdot b + c$

For shared-denominator columns $a = p/d_a$, $b = q/d_b$, $c = r/d_c$, the result has denominator $D = \operatorname{lcm}(d_a d_b,\ d_c)$ and numerators

$$\frac{p_i q_i}{d_a d_b} + \frac{r_i}{d_c} = \frac{p_i q_i\, \sigma_{ab} + r_i\, \sigma_c}{D}, \qquad \sigma_{ab} = \frac{D}{d_a d_b},\quad \sigma_c = \frac{D}{d_c}.$$

With three-place decimals, $d_a = d_b = d_c = 10^3$, so $D = 10^6$, $\sigma_{ab} = 1$, and $\sigma_c = 10^3$. The case $\sigma_{ab} = 1$ is the common one, so the kernel has a separate loop for it that skips the redundant multiplication. Every product and sum uses the checked primitives. The kernel writes into a caller-supplied output buffer and returns only a count of overflowing elements, so the common case allocates nothing and carries no per-element flag array. NumPy gets the same courtesy in the benchmarks: it too writes into a preallocated buffer.

One asymmetry should be stated. The exact kernel computes $ab + c$ in a single fused loop, while NumPy's idiomatic `multiply` then `add` makes two passes over memory. That is how NumPy is normally used, but it gives the exact side a memory-traffic advantage. The Numba float control in Section 4.7 is also a single fused loop, so comparing against it removes the advantage.

If the count is nonzero, a second pass identifies the overflowing elements. Each of those is computed exactly with Python integers and rounded by $\rho$, and the column is promoted to Q64.

```python {#columns fma}
@njit(inline="always")
def fma_element(p, q, r, s_ab, s_c, unit):
    t, o1 = mul_ovf(p, q)
    o2 = False
    if not unit:
        t, o2 = mul_ovf(t, s_ab)
    u, o3 = mul_ovf(r, s_c)
    v, o4 = add_ovf(t, u)
    return v, o1 | o2 | o3 | o4


@njit
def fma_kernel(p, q, r, s_ab, s_c, out):
    bad = 0
    if s_ab == 1:
        for i in range(len(p)):
            v, o = fma_element(p[i], q[i], r[i], s_ab, s_c, True)
            out[i] = v
            bad += o
    else:
        for i in range(len(p)):
            v, o = fma_element(p[i], q[i], r[i], s_ab, s_c, False)
            out[i] = v
            bad += o
    return bad


@njit
def fma_flags(p, q, r, s_ab, s_c):
    flags = np.zeros(len(p), dtype=np.bool_)
    for i in range(len(p)):
        v, o = fma_element(p[i], q[i], r[i], s_ab, s_c, s_ab == 1)
        flags[i] = o
    return flags


def column_fma(a, b, c, out):
    D = math.lcm(a.den * b.den, c.den)
    if D > M:
        raise NotImplementedError("result denominator exceeds 64 bits")
    s_ab, s_c = D // (a.den * b.den), D // c.den
    bad = fma_kernel(a.nums, b.nums, c.nums, np.int64(s_ab), np.int64(s_c), out)
    if bad == 0:
        return SharedColumn(out, D)
    dens = np.full(len(out), D, dtype=np.int64)
    for i in np.flatnonzero(fma_flags(a.nums, b.nums, c.nums, np.int64(s_ab), np.int64(s_c))):
        exact = int(a.nums[i]) * int(b.nums[i]) * s_ab + int(c.nums[i]) * s_c
        out[i], dens[i] = best_approx(exact, D)
    return Q64Column(out, dens)
```

The `unit` argument is a compile-time constant in each loop, so the compiler deletes the unused branch.

## 4.6 Group-by sum

Given keys $k_i \in \{0, \dots, G - 1\}$, the group sums are $S_g = \frac{1}{d} \sum_{k_i = g} p_i$. The kernel is a scatter-add with a checked addition per element and a per-group overflow flag. A scatter-add does not vectorise for floats either, so the per-element check costs comparatively little here. Keys are validated inside the loop. Numba does not bounds-check array indexing, so an out-of-range key would otherwise corrupt memory silently, and NumPy's `bincount` performs the same validation. Any group that overflowed is recomputed exactly in Python and rounded by $\rho$.

```python {#columns groupby}
@njit
def groupby_kernel(keys, nums, groups):
    sums = np.zeros(groups, dtype=np.int64)
    bad = np.zeros(groups, dtype=np.bool_)
    for i in range(len(keys)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        s, o = add_ovf(sums[g], nums[i])
        sums[g] = s
        bad[g] |= o
    return sums, bad


def column_groupby_sum(keys, col, groups):
    sums, bad = groupby_kernel(keys, col.nums, groups)
    if not bad.any():
        return SharedColumn(sums, col.den)
    dens = np.full(groups, col.den, dtype=np.int64)
    for g in np.flatnonzero(bad):
        exact = sum(map(int, col.nums[keys == g].tolist()))
        sums[g], dens[g] = best_approx(exact, col.den)
    return Q64Column(sums, dens)
```

## 4.7 Compiled float controls

A benchmark that compares a Numba kernel with NumPy conflates two things: the arithmetic (exact integers versus floats) and the implementation (a Numba loop versus NumPy's C). To separate them, each column benchmark also runs the same loop under Numba with `float64`.

For the control to be fair it must be allowed to vectorise just as the integer kernels do. Integer addition is associative, so the compiler may reorder an integer sum freely. Float addition is not, and by default Numba must add floats in exactly the order written, a single dependent chain that cannot use vector lanes. The reductions below are therefore compiled with `fastmath={"reassoc", "contract"}`, which permits reordering and fused multiply–adds. That changes the rounding, so the controls' errors are not comparable with NumPy's pairwise summation. They are reported, but the controls are there for timing. The elementwise and scatter loops have no reduction, so they are compiled normally. The loops index the array explicitly rather than iterating over it: with this Numba version, `for v in x` was not vectorised even with `fastmath`, while `for i in range(len(x))` was, which halved the control's time in a probe.

The comparison then reads: if the exact kernel is slower than NumPy but close to the float control, the cost is Numba versus C, not exactness.

```python {#columns float controls}
FAST = {"reassoc", "contract"}


@njit(fastmath=FAST)
def f_sum(x):
    s = 0.0
    for i in range(len(x)):
        s += x[i]
    return s


@njit(fastmath=FAST)
def f_var(x):
    n = len(x)
    m = f_sum(x) / n
    s = 0.0
    for i in range(len(x)):
        d = x[i] - m
        s += d * d
    return s / (n - 1)


@njit
def f_fma(a, b, c, out):
    for i in range(len(a)):
        out[i] = a[i] * b[i] + c[i]


@njit
def f_groupby(keys, x, groups):
    sums = np.zeros(groups)
    for i in range(len(keys)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        sums[g] += x[i]
    return sums
```

## 4.8 Correctness tests

Each kernel is compared with a direct `Fraction` computation on small columns. Every test runs on ordinary values, where the fast path must be taken, and on values chosen to force each fallback. That way both paths are exercised and the fallback cannot hide a bug in the fast path. The fallback cases are:

- values around $2^{62}$, which make every block bad;
- columns of $2^{51} - 1$ spanning several blocks, where every block is good but the combined total overflows;
- a single oversized value in an otherwise ordinary column, where exactly one block is bad;
- values at the variance boundary, $\pm(2^{32} - 1)$, which must stay on the fast path, and $\pm 2^{32}$, which must not;
- for the multiply–add, values around $2^{40}$, so that products overflow 64 bits while results still lie inside the range of $\mathcal R_M$.

```python {#tests columns}
def small_column(rng, n, big, big_bits=62):
    hi = 2**big_bits if big else 10**7
    return SharedColumn(np.array([rng.randint(-hi, hi) for _ in range(n)], dtype=np.int64), 1000)


def check_reductions(col):
    n = len(col)
    xs = [col.value(i) for i in range(n)]
    mean = sum(xs) / n
    assert column_sum(col) == sum(xs)
    assert column_mean(col) == mean
    assert column_var(col) == sum((x - mean) ** 2 for x in xs) / (n - 1)


def test_sum_mean_var():
    rng = random.Random(20)
    for big in (False, True):
        for n in (2, 3, 4095, 4096, 4097, 10000):
            check_reductions(small_column(rng, n, big))


def test_reduction_fallback_paths():
    rng = random.Random(23)
    wide = SharedColumn(np.full(3 * BLOCK, 2**51 - 1, dtype=np.int64), 1)
    assert sum_kernel(wide.nums)[1] and not sum_kernel(wide.nums)[3].any()
    assert column_sum(wide) == 3 * BLOCK * (2**51 - 1)
    one_bad = small_column(rng, 3 * BLOCK, False)
    one_bad.nums[BLOCK + 5] = 2**61
    assert sum_kernel(one_bad.nums)[3].tolist() == [False, True, False]
    check_reductions(one_bad)
    edge = SharedColumn(np.array([2**32 - 1, -(2**32 - 1)] * 3000, dtype=np.int64), 1)
    assert not moments_kernel(edge.nums)[3].any()
    check_reductions(edge)
    past = SharedColumn(np.array([2**32, -(2**32), 7] * 2000, dtype=np.int64), 1)
    assert moments_kernel(past.nums)[3].all()
    check_reductions(past)


def test_fma():
    rng = random.Random(21)
    for big in (False, True):
        a, b, c = (small_column(rng, 3000, big, big_bits=40) for _ in range(3))
        for c_den in (1000, 10):
            c.den = c_den
            out = np.empty(3000, dtype=np.int64)
            res = column_fma(a, b, c, out)
            assert isinstance(res, Q64Column) == big
            for i in range(3000):
                exact = a.value(i) * b.value(i) + c.value(i)
                want = Fraction(*best_approx(exact.numerator, exact.denominator))
                assert res.value(i) == want, i
    a, b, c = (small_column(rng, 500, False) for _ in range(3))
    a.den, b.den, c.den = 4, 6, 9
    res = column_fma(a, b, c, np.empty(500, dtype=np.int64))
    assert all(res.value(i) == a.value(i) * b.value(i) + c.value(i) for i in range(500))


def test_groupby():
    rng = random.Random(22)
    for big in (False, True):
        col = small_column(rng, 5000, big)
        keys = np.array([rng.randrange(7) for _ in range(5000)], dtype=np.int64)
        res = column_groupby_sum(keys, col, 7)
        for g in range(7):
            exact = sum(col.value(i) for i in range(5000) if keys[i] == g)
            assert res.value(g) == Fraction(*best_approx(exact.numerator, exact.denominator))
    try:
        column_groupby_sum(np.array([0, 7], dtype=np.int64), small_column(rng, 2, False), 7)
        raise AssertionError("expected IndexError")
    except IndexError:
        pass
```

The last case in `test_fma` uses denominators $4$, $6$ and $9$, so that $D = \operatorname{lcm}(24, 9) = 72$ and $\sigma_{ab} = 3 \ne 1$, which exercises the general loop.

```python {file=build/columns.py}
import math
from fractions import Fraction

import numpy as np
from numba import njit

from q64 import M, add_ovf, mul_ovf, best_approx


<<columns types>>


<<columns sum>>


<<columns variance>>


<<columns fma>>


<<columns groupby>>


<<columns float controls>>
```
