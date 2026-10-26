# 6. The benchmarks

Ten benchmarks in four groups. Each states what is measured, what the baseline is, what counts as passing, and how the exact result is independently checked. An exact contender's error is reported as `0 (verified)` only when every value was checked, or the check is a randomised proof with negligible failure probability. Checks that only sample say so and give the sample size. If any check fails, the run stops rather than reporting a number.

The targets come from the original plan. They are judgement calls about what users would accept in exchange for exact answers, not established thresholds.

| # | Benchmark | Target |
|---|---|---|
| 1 | Column sum and mean, $10^7$ values | within 2× of NumPy |
| 2 | Elementwise $a b + c$ | within 2× |
| 3 | Group-by sum, 1000 groups | within 2× |
| 4 | Sample variance | within 3×, exact |
| 5 | Matrix multiply, $n = 500, 2000$ | within 5× |
| 6 | Solve $Ax = b$, $n = 100, 500, 1000$ | within 10× |
| 7 | Determinant, same sizes | within 10× |
| 8 | Scalar recurrences | bounded denominators, or under 20× |
| 9 | Hilbert solve | correct answer in reasonable time |
| 10 | Inner product in $\mathbb{Q}(\sqrt2)$ | within 2× (added test of the symbolic type) |

Benchmarks 2 and 4 each have an extra row with data just past the fast-path limits of Section 4.2, to measure the cost of falling back rather than hide it. Those rows use $10^6$ values, not $10^7$, because the fallback is slow.

## 6.1 Fairness settings

**Threads.** NumPy's BLAS will use every core it can find, while FLINT and our Numba kernels run on one. Comparing four cores against one measures the core count, not the arithmetic. So the run pins the BLAS and OpenMP thread pools to one thread before NumPy is imported, and sets FLINT to one thread. The pinning is not just asserted: the metadata records the thread count each native library actually reports, via `threadpoolctl`. Multithreaded comparisons are worth doing later, but separately.

**Conversion.** Building FLINT matrices from NumPy arrays, and building float arrays from decimal numerators, happens once at load time in a real system. It is excluded from all timings, and the matrix conversion time is reported in the note column so it stays visible.

**Equal work.** Wherever NumPy can write into a preallocated buffer, it does, matching the exact kernels. NumPy's `var` uses a two-pass algorithm, and its `sum` uses pairwise summation. Those are its normal, more accurate defaults, so it is compared as users actually run it.

```python {#bench setup}
import os

for var in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ[var] = "1"

import json
import platform
import random
import sys
import time
from fractions import Fraction
from pathlib import Path

import numpy as np
import flint
import numba
from threadpoolctl import threadpool_info

flint.ctx.threads = 1

from harness import Report, measure, rel_error, norm_rel_error, rational_bytes
from data import decimal_column, offset_column, group_keys, decimal_matrix, hilbert_system, qsqrt2_column
from q64 import Q64, STATS, reset_stats, set_counting
from columns import (SharedColumn, Q64Column, column_sum, column_mean, column_var, column_fma, column_groupby_sum,
                     f_sum, f_var, f_fma, f_groupby)
from qfield import QSqrt2, qsqrt2_dot, qsqrt2_rel_error, f_dot
```

Four small helpers: one turns a FLINT `fmpq` into a `Fraction`, one raises if an independent check fails, one finds the primes used to check determinants, and one measures the error of a list of exact rationals, such as rounded Q64 values, against the true values without passing through floats.

```python {#bench helpers}
def frac(x):
    return Fraction(int(x.p), int(x.q))


def verify(ok, what, label="0 (verified)"):
    if not ok:
        raise SystemExit(f"verification failed: {what}")
    return label


def check_primes(k, start=2**60):
    primes, n = [], start
    while len(primes) < k:
        n += 1
        if flint.fmpz(n).is_prime():
            primes.append(n)
    return primes


def exact_norm_err(approx, exact):
    scale = max(abs(e) for e in exact)
    return float(max(abs(a - e) for a, e in zip(approx, exact)) / scale)
```

## 6.2 Group 1: columns of measured decimals

**Benchmark 1, sum and mean.** The exact side sums the numerators (Section 4.3). The check recomputes the integer sum with Python's unbounded integers, over every element.

```python {#bench sum mean}
def bench_sum_mean(report, n):
    nums, den, xf = decimal_column(n)
    col = SharedColumn(nums, den)
    exact_sum = Fraction(sum(map(int, nums.tolist())), den)
    exact_mean = exact_sum / n
    for label, exact_fn, np_fn, nb_fn, exact in (
        ("1a sum", lambda: column_sum(col), lambda: xf.sum(), lambda: f_sum(xf), exact_sum),
        ("1b mean", lambda: column_mean(col), lambda: xf.mean(), lambda: f_sum(xf) / n, exact_mean),
    ):
        res = measure({"NumPy float64": np_fn, "exact (shared den)": exact_fn, "Numba float64": nb_fn})
        report.add_measured(label, n, res, "NumPy float64", **{
            "NumPy float64": dict(error=rel_error(np_fn(), exact), bytes_per_value=8),
            "exact (shared den)": dict(error=verify(exact_fn() == exact, label), bytes_per_value=8),
            "Numba float64": dict(error=rel_error(nb_fn(), exact), bytes_per_value=8, note="timing control, reassociated"),
        })
```

**Benchmark 2, elementwise $ab + c$.** Three independent decimal columns. Every implementation writes into its own preallocated buffer. The check recomputes every element as $p_i q_i + 1000\, r_i$ with NumPy's own `int64` arithmetic, which cannot overflow here because $|p_i q_i| \le 10^{14}$ (the code asserts the bound), and compares the whole array. The NumPy error is measured against exact values over 20,000 randomly sampled elements, since computing $10^7$ exact errors in `Fraction` arithmetic would dominate the run without changing the figure.

The extra row, **2b**, uses six-place decimals of magnitude up to $10^4$. Products of numerators reach $10^{20}$, past $2^{62}$, so every element takes the Python fallback and is rounded by $\rho$. The result is no longer exact. Its true denominator is $10^{12}$ and its numerator up to $10^{20}$, which does not fit in $\mathcal R_M$. So that row reports the rounding error of Q64 against the true value, over 20,000 sampled elements, rather than `0`.

```python {#bench fma}
def bench_fma(report, n, digits=3, magnitude=10**4, label="2a a*b+c"):
    (pa, den, a), (pb, _, b), (pc, _, c) = (decimal_column(n, digits, magnitude, seed=s) for s in (5, 6, 7))
    A, B, C = SharedColumn(pa, den), SharedColumn(pb, den), SharedColumn(pc, den)
    buf_np, buf_nb, buf_ex = np.empty(n), np.empty(n), np.empty(n, dtype=np.int64)

    def run_numpy():
        np.multiply(a, b, out=buf_np)
        np.add(buf_np, c, out=buf_np)

    res = measure({"NumPy float64": run_numpy, "exact (shared den)": lambda: column_fma(A, B, C, buf_ex),
                   "Numba float64": lambda: f_fma(a, b, c, buf_nb)})
    out = column_fma(A, B, C, buf_ex)
    run_numpy()
    f_fma(a, b, c, buf_nb)
    idx = random.Random(40).sample(range(n), min(n, 20000))
    exact = [Fraction(int(pa[i]) * int(pb[i]), den * den) + Fraction(int(pc[i]), den) for i in idx]
    if isinstance(out, SharedColumn):
        bound = int(np.abs(pa).max()) * int(np.abs(pb).max()) + int(np.abs(pc).max()) * den
        assert bound < 2**62 and out.den == den * den
        exact_err = verify(np.array_equal(out.nums, pa * pb + pc * den), "fma")
        note = ""
    else:
        exact_err = exact_norm_err([out.value(i) for i in idx], exact)
        note = "past fast-path limit: all elements rounded by rho; error over 20k sampled elements"
    report.add_measured(label, n, res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error([buf_np[i] for i in idx], exact),
                              bytes_per_value=8, note="error over 20k sampled elements"),
        "exact (shared den)": dict(error=exact_err, bytes_per_value=out.bytes_per_value, note=note),
        "Numba float64": dict(error=norm_rel_error([buf_nb[i] for i in idx], exact),
                              bytes_per_value=8, note="timing control"),
    })
```

**Benchmark 3, group-by sum.** One thousand uniform groups. NumPy's idiom is `bincount` with weights. The check sums each group's numerators with `np.add.at` in `int64`, which cannot overflow at these sizes, since $10^7 \cdot 10^7 < 2^{63}$, and which shares no code with our kernel.

```python {#bench groupby}
def bench_groupby(report, n, groups=1000):
    nums, den, xf = decimal_column(n, seed=8)
    keys = group_keys(n, groups)
    col = SharedColumn(nums, den)
    res = measure({"NumPy float64": lambda: np.bincount(keys, weights=xf, minlength=groups),
                   "exact (shared den)": lambda: column_groupby_sum(keys, col, groups),
                   "Numba float64": lambda: f_groupby(keys, xf, groups)})
    ref = np.zeros(groups, dtype=np.int64)
    np.add.at(ref, keys, nums)
    exact = [Fraction(int(v), den) for v in ref]
    out = column_groupby_sum(keys, col, groups)
    ok = isinstance(out, SharedColumn) and all(out.value(g) == exact[g] for g in range(groups))
    report.add_measured("3 group-by sum", f"{n}, G={groups}", res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error(np.bincount(keys, weights=xf, minlength=groups), exact), bytes_per_value=8),
        "exact (shared den)": dict(error=verify(ok, "groupby"), bytes_per_value=8),
        "Numba float64": dict(error=norm_rel_error(f_groupby(keys, xf, groups), exact), bytes_per_value=8, note="timing control"),
    })
```

**Benchmark 4, variance.** Run on the ordinary decimal column and on the large-offset column from Section 2.2. The check is the textbook two-pass definition evaluated in exact integer arithmetic, $\frac{1}{n-1}\sum (x_i - \bar x)^2$, a different formula from the kernel's one-pass identity.

The extra row, **4c**, moves the offset from $10^6$ to $10^7$ with three decimal places, so numerators reach $10^{10}$, past the $2^{32}$ limit of the split accumulator. Every block falls back to Python. The answer stays exact. Only the speed changes.

```python {#bench variance}
def exact_two_pass_var(nums, den):
    xs = list(map(int, nums.tolist()))
    n = len(xs)
    total = sum(xs)
    return Fraction(sum((n * x - total) ** 2 for x in xs), n * n * (n - 1) * den * den)


def bench_variance(report, n, past_limit_n):
    cases = (("4a variance", n, decimal_column(n, seed=9), ""),
             ("4b variance, offset 1e6", n, offset_column(n), ""),
             ("4c variance, offset 1e7", past_limit_n, offset_column(past_limit_n, offset=10**7),
              "past fast-path limit: every block falls back to Python"))
    for label, size, (nums, den, xf), note in cases:
        col = SharedColumn(nums, den)
        exact = exact_two_pass_var(nums, den)
        res = measure({"NumPy float64": lambda: xf.var(ddof=1), "exact (shared den)": lambda: column_var(col),
                       "Numba float64": lambda: f_var(xf)})
        report.add_measured(label, size, res, "NumPy float64", **{
            "NumPy float64": dict(error=rel_error(xf.var(ddof=1), exact), bytes_per_value=8),
            "exact (shared den)": dict(error=verify(column_var(col) == exact, label), bytes_per_value=8, note=note),
            "Numba float64": dict(error=rel_error(f_var(xf), exact), bytes_per_value=8, note="timing control, reassociated"),
        })
```

The check expands $\sum (x_i - \bar x)^2$ with $x_i = p_i/d$ and $\bar x = P/(nd)$ where $P = \sum p_i$. Then $x_i - \bar x = (n p_i - P)/(nd)$, which gives the expression in the code without any division inside the loop.

## 6.3 Group 2: linear algebra

All three benchmarks use matrices of three-place decimals, $A = N/10^3$ with $|N_{ij}| \le 10^6$. FLINT works on the integer matrix $N$ and NumPy on $\operatorname{fl}(N/10^3)$. The exact result for $A$ is then recovered by scaling: $AB = NB'/10^6$ for a product, the solution of $Ax = b$ is the solution of $Nx = M$, and $\det A = \det N / 10^{3n}$.

**Benchmark 5, matrix multiply.** FLINT's `fmpz_mat` product against NumPy's `@` (OpenBLAS `dgemm`). The check is Freivalds' algorithm: for a random vector $r$, test $C r = N_1 (N_2 r)$ exactly. That costs $O(n^2)$ instead of $O(n^3)$. If $C \neq N_1 N_2$, a random $r$ with entries drawn from $2^{62}$ values exposes the difference with probability at least $1 - 2^{-62}$, so three independent trials leave a failure probability below $2^{-186}$. The NumPy error is measured over 2,000 random entries. The bytes-per-value figure is the size of the integer FLINT actually stores per entry, with the denominator $10^6$ shared by the whole matrix.

```python {#bench matmul}
def bench_matmul(report, n):
    N1, den, A1 = decimal_matrix(n, n, seed=10)
    N2, _, A2 = decimal_matrix(n, n, seed=11)
    t = time.perf_counter()
    F1, F2 = flint.fmpz_mat(N1.tolist()), flint.fmpz_mat(N2.tolist())
    conv = time.perf_counter() - t
    res = measure({"NumPy float64": lambda: A1 @ A2, "FLINT exact": lambda: F1 * F2})
    C, Cf = F1 * F2, A1 @ A2
    rng = random.Random(41)
    ok = True
    for _ in range(3):
        r = flint.fmpz_mat(n, 1, [rng.randrange(2**62) for _ in range(n)])
        ok = ok and C * r == F1 * (F2 * r)
    sample = [(rng.randrange(n), rng.randrange(n)) for _ in range(2000)]
    exact = [Fraction(int(C[i, j]), den * den) for i, j in sample]
    stored = sum((abs(int(C[i, j])).bit_length() + 1 + 7) // 8 for i, j in sample) / len(sample)
    report.add_measured("5 matmul", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error([Cf[i, j] for i, j in sample], exact), bytes_per_value=8),
        "FLINT exact": dict(error=verify(ok, "matmul", "0 (verified, Freivalds x3)"), bytes_per_value=round(stored, 1),
                            note=f"stored integer per entry, den shared; conversion {conv:.2f} s excluded"),
    })
```

**Benchmark 6, solve.** FLINT's `fmpz_mat.solve`. For a single right-hand side and $n > 15$, FLINT dispatches to Dixon $p$-adic lifting, as its source shows. The check multiplies back: $NX = M$ in exact rational arithmetic. The bytes-per-value figure is the average size of an entry of the exact solution, which shows how large exact answers get. By Cramer's rule each entry is a ratio of $n \times n$ determinants, so its size grows roughly like $n$ times the entry size.

```python {#bench solve}
def bench_solve(report, n):
    N, den, A = decimal_matrix(n, n, seed=12)
    Mb, _, b = decimal_matrix(n, 1, seed=13)
    t = time.perf_counter()
    FN, FM = flint.fmpz_mat(N.tolist()), flint.fmpz_mat(Mb.tolist())
    conv = time.perf_counter() - t
    res = measure({"NumPy float64": lambda: np.linalg.solve(A, b), "FLINT exact (Dixon)": lambda: FN.solve(FM)})
    X = FN.solve(FM)
    ok = flint.fmpq_mat(FN) * X == flint.fmpq_mat(FM)
    exact = [frac(X[i, 0]) for i in range(n)]
    report.add_measured("6 solve Ax=b", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error(np.linalg.solve(A, b)[:, 0], exact), bytes_per_value=8),
        "FLINT exact (Dixon)": dict(error=verify(ok, "solve"), bytes_per_value=round(sum(map(rational_bytes, exact)) / n, 1),
                                    note=f"conversion {conv:.2f} s excluded"),
    })
```

**Benchmark 7, determinant.** For $n \ge 100$ the determinant of $A$ is around $10^{300}$ or more, beyond the range of binary64, so `np.linalg.det` returns infinity. A float user would call `slogdet`, which returns the sign and $\log|\det A|$, so that is the baseline. Its relative error in the determinant itself is

$$\varepsilon = \left| e^{\,\widehat{\ell} - \ell} - 1 \right|, \qquad \ell = \log|\det N| - n \log 10^3,$$

evaluated in 256-bit ball arithmetic. The check reduces $N$ modulo three 60-bit primes and compares the determinant of each reduction with $\det N \bmod p$. That check is not independent of FLINT, since it runs FLINT's own `nmod_mat` determinant code, but it uses a different algorithm path, and primes chosen here rather than by FLINT. A wrong determinant passes all three only if it agrees with the true one modulo a product of about $2^{180}$.

```python {#bench det}
def bench_det(report, n):
    N, den, A = decimal_matrix(n, n, seed=14)
    t = time.perf_counter()
    FN = flint.fmpz_mat(N.tolist())
    conv = time.perf_counter() - t
    res = measure({"NumPy float64 (slogdet)": lambda: np.linalg.slogdet(A), "FLINT exact": lambda: FN.det()})
    D = FN.det()
    rows = N.tolist()
    ok = all(int(flint.nmod_mat(rows, p).det()) == int(D) % p for p in check_primes(3))
    sign, logabs = np.linalg.slogdet(A)
    flint.ctx.prec = 256
    ell = flint.arb(abs(int(D))).log() - n * flint.arb(den).log()
    err = float(abs((flint.arb(float(logabs)) - ell).exp() - 1).mid())
    flint.ctx.prec = 53
    sign_ok = (sign > 0) == (int(D) > 0)
    report.add_measured("7 determinant", n, res, "NumPy float64 (slogdet)", **{
        "NumPy float64 (slogdet)": dict(error=err if sign_ok else "sign wrong", bytes_per_value=16,
                                        note="sign + log|det|"),
        "FLINT exact": dict(error=verify(ok, "det", "0 (checked mod 3 primes)"),
                            bytes_per_value=rational_bytes(Fraction(int(D), den**n)),
                            note=f"one value; conversion {conv:.2f} s excluded"),
    })
```

## 6.4 Group 3: stress tests

These are where exactness is expected to struggle, because the denominators of an exact iteration grow without bound. Three scalar recurrences, each run as a plain Python loop with several number types: built-in floats as the baseline, Q64 from Chapter 3, unbounded `Fraction`s where feasible, and a 4096-bit ball reference for the errors.

These rows do **not** isolate the cost of the arithmetic. A built-in float operation is a single C call, while each Q64 operation runs Python code: argument coercion, a constructor, a bounds test, and often a call to $\rho$. `Fraction` is also pure Python. So the ratio against built-in floats is dominated by interpreter overhead, and the "under 20×" target cannot be judged from it. To separate the two effects, a fourth contender wraps a float in a Python class with the same call structure as Q64: the same coercion, the same constructor checks, the same flag test. The ratio of Q64 to that control is the closest this setup gets to the cost of the arithmetic itself, and it is given in the note column. A compiled Q64 needs a 128-bit rounding path, which is the main thing to build next if these numbers look promising.

The garbage collector stays on for these rows (Section 1.3), because every operation allocates. Q64's event counting is switched off while timing, and the counts in the table come from one separate untimed run.

1. **Euler decay.** $y' = -y$, $y(0) = 1$, step $h = 1/100$, 1000 steps. The update is $y_{k+1} = (1 - h)\, y_k$, so the exact answer is $(99/100)^{1000}$, whose denominator has 2001 digits. This is the simplest possible case of exponential denominator growth.
2. **Logistic map.** $x_{k+1} = r x_k (1 - x_k)$ with $r = 16/5$ and $x_0 = 1/5$, 1000 steps. At $r = 3.2$ the map has an attracting 2-cycle, so errors are damped rather than amplified, and the comparison measures rounding, not chaos. Exact iteration doubles the degree every step, so after about 30 steps an exact `Fraction` would need more bits than there are atoms in a laptop. The Fraction loop is therefore run for 15 steps only, to show the growth, and reported separately.
3. **Newton's method for $\sqrt2$.** $x_{k+1} = (x_k + 2/x_k)/2$ from $x_0 = 1$, 8 steps. The target is irrational, so Q64 can only approach the best element of $\mathcal R_M$ near $\sqrt2$. Best approximations to $\sqrt2$ with denominators near $M$ are accurate to about $1/M^2 \approx 10^{-38}$, so this tests whether rounding by $\rho$ actually reaches that.

```python {#bench float class}
class PyFloat:
    __slots__ = ("v",)

    def __init__(self, v):
        if v != v:
            raise ValueError("nan")
        if abs(v) > 1.7e308:
            raise OverflowError("overflow")
        self.v = v

    @staticmethod
    def coerce(x):
        if isinstance(x, PyFloat):
            return x
        if isinstance(x, (int, float)):
            return PyFloat(float(x))
        return NotImplemented

    def __add__(self, other):
        o = PyFloat.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING_CONTROL:
            CONTROL_OPS[0] += 1
        return PyFloat(self.v + o.v)

    __radd__ = __add__

    def __sub__(self, other):
        o = PyFloat.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING_CONTROL:
            CONTROL_OPS[0] += 1
        return PyFloat(self.v - o.v)

    def __rsub__(self, other):
        o = PyFloat.coerce(other)
        if o is NotImplemented:
            return o
        return o - self

    def __mul__(self, other):
        o = PyFloat.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING_CONTROL:
            CONTROL_OPS[0] += 1
        return PyFloat(self.v * o.v)

    __rmul__ = __mul__

    def __truediv__(self, other):
        o = PyFloat.coerce(other)
        if o is NotImplemented:
            return o
        if COUNTING_CONTROL:
            CONTROL_OPS[0] += 1
        return PyFloat(self.v / o.v)

    def __float__(self):
        return self.v


COUNTING_CONTROL = False
CONTROL_OPS = [0]
```

The constructor's two checks stand in for Q64's sign, zero and bounds tests, and the flag test, which is always false while timing, mirrors Q64's counting test, so the control pays the same interpreter costs and only the arithmetic differs.

```python {#bench recurrences}
def euler(one, h, steps):
    y = one
    for _ in range(steps):
        y = y - h * y
    return y


def logistic(r, x0, one, steps):
    x = x0
    for _ in range(steps):
        x = r * x * (one - x)
    return x


def newton_sqrt2(one, two, steps):
    x = one
    for _ in range(steps):
        x = (x + two / x) / two
    return x


def arb_ref(fn, prec=4096):
    flint.ctx.prec = prec
    try:
        v = fn()
        assert float(v.rad()) < 1e-60 * max(1.0, abs(float(v.mid()))), "reference ball too wide"
        return v
    finally:
        flint.ctx.prec = 53


def err_vs_arb(x, ref):
    flint.ctx.prec = 4096
    try:
        if isinstance(x, Q64):
            v = flint.arb(x.n) / x.d
        elif isinstance(x, Fraction):
            v = flint.arb(x.numerator) / x.denominator
        else:
            v = flint.arb(float(x))
        return float(abs((v - ref) / ref).mid())
    finally:
        flint.ctx.prec = 53


def q64_counts(fn):
    reset_stats()
    set_counting(True)
    try:
        x = fn()
    finally:
        set_counting(False)
    return x, f"rounded {STATS['rounded']}/{STATS['ops']} ops, inexact {STATS['inexact']}"


def bench_recurrences(report):
    cases = [
        ("8a Euler decay", 1000,
         lambda: euler(1.0, 0.01, 1000),
         lambda: euler(PyFloat(1.0), PyFloat(0.01), 1000),
         lambda: euler(Q64(1), Q64(1, 100), 1000),
         lambda: euler(Fraction(1), Fraction(1, 100), 1000),
         lambda: euler(flint.arb(1), flint.arb(1) / 100, 1000)),
        ("8b logistic r=3.2", 1000,
         lambda: logistic(3.2, 0.2, 1.0, 1000),
         lambda: logistic(PyFloat(3.2), PyFloat(0.2), PyFloat(1.0), 1000),
         lambda: logistic(Q64(16, 5), Q64(1, 5), Q64(1), 1000),
         None,
         lambda: logistic(flint.arb(16) / 5, flint.arb(1) / 5, flint.arb(1), 1000)),
        ("8c Newton sqrt(2)", 8,
         lambda: newton_sqrt2(1.0, 2.0, 8),
         lambda: newton_sqrt2(PyFloat(1.0), PyFloat(2.0), 8),
         lambda: newton_sqrt2(Q64(1), Q64(2), 8),
         lambda: newton_sqrt2(Fraction(1), Fraction(2), 8),
         lambda: flint.arb(2).sqrt()),
    ]
    for label, steps, f_float, f_class, f_q64, f_frac, f_ref in cases:
        ref = arb_ref(f_ref)
        contenders = {"Python float": f_float, "float in Python class": f_class, "Q64": f_q64}
        if f_frac:
            contenders["Fraction (unbounded)"] = f_frac
        res = measure(contenders, gc_off=False)
        q, counts = q64_counts(f_q64)
        vs_class = res["Q64"].median / res["float in Python class"].median
        extra = {
            "Python float": dict(error=err_vs_arb(f_float(), ref), bytes_per_value=8),
            "float in Python class": dict(error=err_vs_arb(f_class(), ref), bytes_per_value=8,
                                          note="interpreter-overhead control"),
            "Q64": dict(error=err_vs_arb(q, ref), bytes_per_value=16, note=f"{counts}; {vs_class:.1f}x vs class control"),
        }
        if f_frac:
            fx = f_frac()
            extra["Fraction (unbounded)"] = dict(error=err_vs_arb(fx, ref), bytes_per_value=rational_bytes(fx))
        report.add_measured(label, steps, res, "Python float", **extra)
    x = Fraction(1, 5)
    for _ in range(15):
        x = Fraction(16, 5) * x * (1 - x)
    report.add("8b logistic r=3.2", 15, "Fraction (unbounded)", None, bytes_per_value=rational_bytes(x),
               note="15 steps only; size doubles each step")
```

An exact `Fraction` that matches the reference to within the ball radius has an error far below $10^{-1000}$, which prints as `0 (exact)`.

**Benchmark 9, the Hilbert matrix.** $H\mathbf x = H\mathbf 1$ for $n = 10, 20, 50$. FLINT solves over $\mathbb{Q}$ directly. The check is that the exact answer is the all-ones vector.

```python {#bench hilbert}
def bench_hilbert(report, n):
    H, b, Hf, bf = hilbert_system(n)
    res = measure({"NumPy float64": lambda: np.linalg.solve(Hf, bf), "FLINT exact": lambda: H.solve(b)})
    X = H.solve(b)
    exact = [frac(X[i, 0]) for i in range(n)]
    report.add_measured("9 Hilbert solve", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error(np.linalg.solve(Hf, bf), [1] * n), bytes_per_value=8),
        "FLINT exact": dict(error=verify(all(x == 1 for x in exact), "hilbert"),
                            bytes_per_value=sum(map(rational_bytes, exact)) / n),
    })
```

## 6.5 Group 4: the symbolic type

**Benchmark 10, inner product in $\mathbb{Q}(\sqrt2)$.** Two columns of $10^7$ elements $a + b\sqrt2$ with three-place decimal coordinates. NumPy computes `np.dot` on the float values (BLAS `ddot`). The exact side uses the kernel of Section 5.3. The check recomputes both coordinate sums with Python integers over all elements. The float error is measured against a 256-bit ball evaluation of the exact answer.

```python {#bench qsqrt2}
def bench_qsqrt2(report, n):
    p, q, dx, xf = qsqrt2_column(n, seed=15)
    r, s, dy, yf = qsqrt2_column(n, seed=16)
    res = measure({"NumPy float64": lambda: np.dot(xf, yf), "exact Q(sqrt2)": lambda: qsqrt2_dot(p, q, dx, r, s, dy),
                   "Numba float64": lambda: f_dot(xf, yf)})
    got = qsqrt2_dot(p, q, dx, r, s, dy)
    P, Q, R, S = (list(map(int, v.tolist())) for v in (p, q, r, s))
    A = sum(a * c + 2 * b * d for a, b, c, d in zip(P, Q, R, S))
    B = sum(a * d + b * c for a, b, c, d in zip(P, Q, R, S))
    ok = got == QSqrt2(Fraction(A, dx * dy), Fraction(B, dx * dy))
    report.add_measured("10 dot in Q(sqrt2)", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=qsqrt2_rel_error(np.dot(xf, yf), got), bytes_per_value=8),
        "exact Q(sqrt2)": dict(error=verify(ok, "qsqrt2 dot"), bytes_per_value=16),
        "Numba float64": dict(error=qsqrt2_rel_error(f_dot(xf, yf), got), bytes_per_value=8, note="timing control, reassociated"),
    })
```

## 6.6 Running everything

The runner first executes every correctness test from Chapters 3 to 5 and stops if any fails. It then records the machine, the library versions and the thread pools, runs the benchmarks, and writes `results/results.md` with the table and `results/results.json` with every raw timing. `--quick` shrinks every size so the whole pipeline can be checked in a minute.

```python {#bench metadata}
def metadata():
    cpu = "unknown"
    try:
        for line in Path("/proc/cpuinfo").read_text().splitlines():
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    pools = "; ".join(f"{p.get('internal_api')} {p.get('num_threads')} thread(s)" for p in threadpool_info())
    return {
        "date": time.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "cpu": cpu,
        "cores_visible": os.cpu_count(),
        "python": sys.version.split()[0],
        "numpy": np.__version__,
        "numba": numba.__version__,
        "python-flint": flint.__version__,
        "platform": platform.platform(),
        "native thread pools": pools or "none reported",
        "flint threads": flint.ctx.threads,
    }
```

```python {#bench main}
def run_tests():
    import test_correctness
    names = sorted(n for n in dir(test_correctness) if n.startswith("test_"))
    for name in names:
        t = time.perf_counter()
        getattr(test_correctness, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)")


def main():
    quick = "--quick" in sys.argv
    print("correctness tests")
    run_tests()
    meta = metadata()
    report = Report()
    col_n = 10**5 if quick else 10**7
    past_n = 10**4 if quick else 10**6
    steps = [
        ("1 sum and mean", lambda: bench_sum_mean(report, col_n)),
        ("2a a*b+c", lambda: bench_fma(report, col_n)),
        ("2b a*b+c past limit", lambda: bench_fma(report, past_n, digits=6, label="2b a*b+c, 6 places")),
        ("3 group-by", lambda: bench_groupby(report, col_n)),
        ("4 variance", lambda: bench_variance(report, col_n, past_n)),
        *[(f"5 matmul {n}", lambda n=n: bench_matmul(report, n)) for n in ((50, 100) if quick else (500, 2000))],
        *[(f"6 solve {n}", lambda n=n: bench_solve(report, n)) for n in ((20, 50) if quick else (100, 500, 1000))],
        *[(f"7 det {n}", lambda n=n: bench_det(report, n)) for n in ((20, 50) if quick else (100, 500, 1000))],
        ("8 recurrences", lambda: bench_recurrences(report)),
        *[(f"9 Hilbert {n}", lambda n=n: bench_hilbert(report, n)) for n in (10, 20, 50)],
        ("10 Q(sqrt2) dot", lambda: bench_qsqrt2(report, col_n)),
    ]
    for name, step in steps:
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / f"results{suffix}.md").write_text(f"# Benchmark results\n\n{header}\n\n{report.markdown()}\n")
    (out / f"results{suffix}.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

The tests from every chapter are gathered into one module.

```python {file=build/test_correctness.py}
import operator
import random
from fractions import Fraction

import numpy as np

from q64 import M, INT64_MIN, best_approx, Q64, Q64RangeError, add_ovf, mul_ovf
from columns import (BLOCK, SharedColumn, Q64Column, column_sum, column_mean, column_var, column_fma,
                     column_groupby_sum, sum_kernel, moments_kernel)
from qfield import QSqrt2, SQRT2, qsqrt2_dot, dot_kernel


<<tests q64 brute>>


<<tests q64 exact>>


<<tests q64 kernels>>


<<tests q64 scalar>>


<<tests columns>>


<<tests qfield>>
```

```python {file=build/run_benchmarks.py}
<<bench setup>>


<<bench helpers>>


<<bench sum mean>>


<<bench fma>>


<<bench groupby>>


<<bench variance>>


<<bench matmul>>


<<bench solve>>


<<bench det>>


<<bench float class>>


<<bench recurrences>>


<<bench hilbert>>


<<bench qsqrt2>>


<<bench metadata>>


<<bench main>>
```
