# 7. Four speed-ups, tested one at a time

The first run left two problems. Exact linear algebra was 100 to 350 times slower than NumPy, and data past the 64-bit fast paths fell off a cliff into Python. This chapter tests four candidate fixes. Each is a separate switch, so its effect can be measured on its own:

| Variant | What changes | Problems it is tested on |
|---|---|---|
| **A. BLAS lifting** | Dixon's $p$-adic lifting, with every heavy step done as float BLAS operations that are provably exact | solve and determinant, random data |
| **B. Early termination** | stop lifting as soon as a candidate answer passes an exact check, instead of at the worst-case bound | solve, random and planted-answer data, applied to both A and C |
| **C. Numeric–symbolic solving** | float LU guesses each correction, residuals are computed exactly, and the exact answer is reconstructed and checked | solve, random and planted-answer data |
| **D. 128-bit fallback** | past the 64-bit limits, compiled double-word arithmetic replaces the Python fallback | the two past-limit column rows |

Every variant returns the **exact** answer. None of them is a "probably right" method. Variants A and C without early termination stop at a proven bound, and with early termination they stop only after an exact verification. Where a variant cannot apply, for example because the matrix is too ill-conditioned for floats to make progress, it says so and declines, rather than returning anything inexact.

One fact found while building these shapes the reading of the results. FLINT's own Dixon solver, `_fmpq_mat_solve_dixon`, already contains an early-termination check: its source tracks whether "lifting has stabilised". That python-flint 0.9's `fmpz_mat.solve` reaches this routine for a single right-hand side is inferred, not verified: FLINT's C dispatcher does so for $n > 15$, and in a probe the timing matched `fmpq_mat.solve(algorithm='dixon')`. So FLINT is not a "no early termination" baseline. On problems with simple answers, it already benefits from idea B.

## 7.1 When float arithmetic is exact integer arithmetic

All of A and C rests on one fact. A binary64 float represents every integer of magnitude at most $2^{53}$ exactly, and a sum or product of such integers is computed exactly whenever every partial result stays within $2^{53}$. A matrix–vector product $Mv$ with $|M_{ij}| < \alpha$ and $|v_j| < \beta$ has partial sums below $n\alpha\beta$. So

$$n\,\alpha\,\beta \le 2^{53} \quad\Longrightarrow\quad \texttt{M @ v} \text{ in BLAS is the exact integer product.}$$

BLAS may reorder the summation however it likes. That is harmless here, because every reordering of an exact integer sum gives the same exact integer. This lets exact integer linear algebra run on the most optimised code on the machine.

The variants require $|A_{ij}| < 2^{20}$, $n \le 2048$ and $|b_i| < 2^{52}$, which covers the benchmark matrices: three-place decimals up to $10^3$ give numerators up to $10^6 < 2^{20}$. Inputs outside that range raise `NotApplicable`, and a caller would use FLINT instead. The range test compares against the minimum and maximum, not the absolute value, because in two's complement $|{-2^{63}}|$ wraps to $-2^{63}$ and would pass an `abs` test. The independent review caught exactly that hole in a first draft, which returned a wrong answer for $b_1 = -2^{63}$.

```python {#fast common}
FLOAT_EXACT = 2**53
ENTRY_LIMIT = 2**20
RHS_LIMIT = 2**52
MAX_N = 2048


class NotApplicable(Exception):
    pass


def check_input(A, b):
    n = A.shape[0]
    if A.shape != (n, n) or b.shape != (n,) or A.dtype != np.int64 or b.dtype != np.int64 or n == 0:
        raise NotApplicable("need a nonempty square int64 matrix and an int64 vector")
    if n > MAX_N or A.min() <= -ENTRY_LIMIT or A.max() >= ENTRY_LIMIT or b.min() <= -RHS_LIMIT or b.max() >= RHS_LIMIT:
        raise NotApplicable("entries or size outside the float-exact range")


def prime_below(m):
    q = int(m) - 1
    while not flint.fmpz(q).is_prime():
        q -= 1
    return q


def exact_check(FA, nums, d, b):
    n = len(nums)
    return FA * flint.fmpz_mat(n, 1, nums) == flint.fmpz_mat(n, 1, [d * int(v) for v in b])
```

`exact_check` confirms $A \cdot \text{nums} = d \cdot b$ in FLINT's exact integers. That is the definition of $x = \text{nums}/d$ being the solution, so passing it is a proof, not a test.

### Bounds on the answer

To stop at a proven point we need bounds on the size of the answer. By Cramer's rule, $x_j = \det A_j / \det A$, where $A_j$ is $A$ with column $j$ replaced by $b$. Hadamard's inequality bounds a determinant by the product of its column norms, or of its row norms. So the common denominator of $x$ is at most $H = \min\bigl(\prod_i \lVert a_{i\cdot} \rVert,\ \prod_j \lVert a_{\cdot j} \rVert\bigr)$, and every numerator is at most

$$N = \frac{\prod_k \lVert a_{\cdot k} \rVert \;\cdot\; \lVert b \rVert}{\min_k \lVert a_{\cdot k} \rVert}.$$

We compute $\log_2$ of both in floating point and add one bit of slack, which covers the floating-point error in the logarithms many times over.

```python {#fast bounds}
def log2_bounds(A, b):
    Af = A.astype(np.float64)
    with np.errstate(divide="ignore"):
        row = 0.5 * np.log2((Af * Af).sum(axis=1))
        col = 0.5 * np.log2((Af * Af).sum(axis=0))
    if not np.isfinite(row).all() or not np.isfinite(col).all():
        raise ZeroDivisionError("singular matrix: zero row or column")
    log_den = min(row.sum(), col.sum()) + 1
    log_b = 0.5 * math.log2(max(1.0, float((b.astype(np.float64) ** 2).sum())))
    log_num = col.sum() + log_b - col.min() + 1
    return log_num, log_den
```

## 7.2 Variant A: Dixon lifting on BLAS

Dixon's method, as described earlier, solves $Ax = b$ by computing $x$ as a $p$-adic number, one base-$p$ digit vector per step:

$$x^{(i)} = C r^{(i)} \bmod p, \qquad r^{(i+1)} = \frac{r^{(i)} - A x^{(i)}}{p}, \qquad r^{(0)} = b,$$

where $C = A^{-1} \bmod p$ is computed once. Then $x \equiv \sum_i x^{(i)} p^i \pmod{p^k}$, and rational reconstruction recovers $x$ from that residue.

Each step costs two matrix–vector products, and the trick is to make both of them exact BLAS operations. Choose the prime $p$ so that

$$n p^2 \le 2^{53} \quad \text{and} \quad n \cdot 2^{20} \cdot p \le 2^{53}.$$

The first condition makes $C \cdot (r \bmod p)$ exact, since both factors are below $p$. The second makes $A x^{(i)}$ exact, since $|A| < 2^{20}$ and $0 \le x^{(i)} < p$. For $n = 1000$ this gives $p \approx 2^{21.5}$, so each step yields about 21 bits of the answer. The residual stays small. Since $|r^{(i+1)}| \le |r^{(i)}|/p + n \cdot 2^{20}$, it never exceeds about $\max(|b|, 2n \cdot 2^{20})$ and lives comfortably in `int64`. The one-off inverse uses FLINT's `nmod_mat`, and if $A$ happens to be singular modulo $p$, the next prime down is tried, up to five primes. A matrix singular modulo five consecutive primes near $2^{21}$ is almost certainly singular, and is reported as such. A caller that needs certainty either way can hand it to FLINT, as the determinant code does.

```python {#fast dixon setup}
def dixon_inverse(A):
    n = A.shape[0]
    limit = min(math.isqrt(FLOAT_EXACT // n), FLOAT_EXACT // (n * ENTRY_LIMIT))
    p = prime_below(limit)
    rows = A.tolist()
    for _ in range(5):
        try:
            inv = flint.nmod_mat(rows, p).inv()
        except ZeroDivisionError:
            p = prime_below(p)
            continue
        return p, np.array([int(v) for v in inv.entries()], dtype=np.float64).reshape(n, n)
    raise ZeroDivisionError("matrix singular modulo 5 primes")
```

That an inverse modulo $p$ exists also proves $\det A \not\equiv 0 \pmod p$, so $A$ is nonsingular. That fact matters for the proven stopping rule below.

**Assembling the digits.** After $k$ steps the residue is $X = \sum_{i<k} x^{(i)} p^i$, a vector of integers with tens of thousands of bits. Building it by Horner's rule would cost $k$ big-integer operations per entry. Instead, pairs of digits are combined first in `int64`, since $p^2 < 2^{43}$, and then pairs of pairs in a balanced tree of Python big integers. Fast multiplication then makes the total cost roughly $O(M(\text{size}) \log k)$ per entry.

```python {#fast combine}
def combine_low_first(leaves):
    level = list(leaves)
    while len(level) > 1:
        nxt = [(level[i][0] + level[i + 1][0] * level[i][1], level[i][1] * level[i + 1][1])
               for i in range(0, len(level) - 1, 2)]
        if len(level) % 2:
            nxt.append(level[-1])
        level = nxt
    return level[0]


def combine_high_first(leaves):
    level = list(leaves)
    while len(level) > 1:
        nxt = [(level[i][0] * level[i + 1][1] + level[i + 1][0], level[i][1] * level[i + 1][1])
               for i in range(0, len(level) - 1, 2)]
        if len(level) % 2:
            nxt.append(level[-1])
        level = nxt
    return level[0]


def padic_leaves(digits, p):
    leaves = [((digits[i] + digits[i + 1] * p).astype(object), p * p) for i in range(0, len(digits) - 1, 2)]
    if len(digits) % 2:
        leaves.append((digits[-1].astype(object), p))
    return leaves
```

Each leaf is a pair (values, weight). For $p$-adic digits the earlier digit is the less significant one, so a merge computes $L + R \cdot w_L$. Chapter 7.3 needs the opposite order, and `combine_high_first` computes $L \cdot w_R + R$.

**Rational reconstruction.** Given $u \equiv a/q \pmod P$ with $|a| \le N$, $0 < q \le D$ and $2ND < P$, the fraction $a/q$ is unique and is found by running the extended Euclidean algorithm on $(P, u)$ until the remainder drops to $N$ or below. That is Wang's algorithm. Reconstructing every one of $n$ entries this way would cost $n$ long Euclid runs. But the entries of $x$ share a common denominator, so we reconstruct one entry, take its denominator $d$, and for every other entry compute $d\,u_j \bmod P$ directly. If the symmetric residue is at most $N$ in magnitude, it is the numerator. If not, that entry's own reconstruction gives an extra factor $e$, and $d$ becomes $de$. Typically only the first entry needs Euclid.

```python {#fast ratrecon}
def ratrecon(u, P, N, D):
    r0, r1 = P, u % P
    t0, t1 = 0, 1
    while r1 > N:
        q = r0 // r1
        r0, r1 = r1, r0 - q * r1
        t0, t1 = t1, t0 - q * t1
    if t1 == 0 or abs(t1) > D:
        return None
    if t1 < 0:
        r1, t1 = -r1, -t1
    if math.gcd(r1, t1) != 1:
        return None
    return r1, t1


def reconstruct_padic(X, P, N, D):
    first = ratrecon(int(X[0]), P, N, D)
    if first is None:
        return None
    d = first[1]
    nums = [0] * len(X)
    j = 0
    while j < len(X):
        y = (int(X[j]) * d) % P
        if y > P // 2:
            y -= P
        if abs(y) <= N:
            nums[j] = y
            j += 1
            continue
        extra = ratrecon(y, P, N, D)
        if extra is None or d * extra[1] > D:
            return None
        d *= extra[1]
        for i in range(j):
            nums[i] *= extra[1]
    return nums, d
```

**Two stopping rules.** With `early=False`, lifting runs to the proven bound, $p^k > 2ND$ with $N, D$ from Section 7.1. Then reconstruction is guaranteed to succeed and the result is guaranteed correct. We run `exact_check` once anyway. It costs one exact matrix–vector product, and it turns any remaining hole in the exactness argument from a silent wrong answer into an error. Both stopping rules of both solvers therefore end with the same exact check, which also keeps the comparison between them even. With `early=True` (variant B), every time the precision reached, $k \log_2 p$ bits, passes a checkpoint of $64, 128, 256, \dots$ bits, we attempt reconstruction with balanced bounds $N = D = \lfloor\sqrt{(P-1)/2}\rfloor$ and run `exact_check`. Because the checkpoints double, the work spent on failed attempts is at most a constant factor of the final attempt's. If no checkpoint verifies, lifting continues to the proven bound and the guaranteed reconstruction is used. So early termination can only make the answer arrive sooner, never wrong.

```python {#fast dixon}
def dixon_solve(A, b, early):
    check_input(A, b)
    log_num, log_den = log2_bounds(A, b)
    p, C = dixon_inverse(A)
    Af = A.astype(np.float64)
    k_full = math.ceil((log_num + log_den + 4) / math.log2(p))
    FA = flint.fmpz_mat(A.tolist()) if early else None
    r = b.copy()
    pending, X, P = [], None, 1
    k, next_check = 0, 64
    bits_per_step = math.log2(p)
    while True:
        x = np.mod(C @ np.mod(r, p).astype(np.float64), p)
        r = (r - (Af @ x).astype(np.int64)) // p
        pending.append(x.astype(np.int64))
        k += 1
        final = k >= k_full
        if not (final or (early and k * bits_per_step >= next_check)):
            continue
        val, weight = combine_low_first(padic_leaves(pending, p))
        X = val if X is None else X + val * P
        P *= weight
        pending = []
        if final:
            res = reconstruct_padic(X, P, 2 ** math.ceil(log_num), 2 ** math.ceil(log_den))
            if res is None or not exact_check(flint.fmpz_mat(A.tolist()) if FA is None else FA, res[0], res[1], b):
                raise ArithmeticError("no exact solution at the proven bound")
            return res + (k,)
        bound = math.isqrt((P - 1) // 2)
        res = reconstruct_padic(X, P, bound, bound)
        if res is not None and exact_check(FA, res[0], res[1], b):
            return res + (k,)
        next_check *= 2
```

The `+ 4` in `k_full` absorbs the rounding of the two bounds up to powers of two and the factor 2 in $2ND < P$. The function returns the numerators, the common denominator, and the number of lifting steps taken.

## 7.3 Variant C: numeric–symbolic solving

This is the method of Wan (2006), in the form of iterative refinement with exact residuals. Floats are used only to *guess* corrections. Everything that determines the answer is exact.

Keep integers $X$ (a vector), $D$ (a power of two) and $r$ (the residual), maintaining the invariant

$$A X = D\, b - r.$$

Initially $X = 0$, $D = 1$, $r = b$. Each step solves $A y \approx r$ in floating point using one float LU factorisation computed up front, picks a scale $2^s$, and rounds: $c = \operatorname{round}(2^s y)$. Then it updates

$$X \leftarrow 2^s X + c, \qquad D \leftarrow 2^s D, \qquad r \leftarrow 2^s r - A c,$$

which preserves the invariant exactly, whatever the quality of $y$. If $y$ is accurate, then $2^s r - Ac$ is small, the residual stays bounded, and $X/D$ gains $s$ correct bits per step, because

$$x - \frac{X}{D} = \frac{A^{-1} r}{D}.$$

**Computing $Ac$ exactly.** The entries of $c$ have up to about 61 bits, far beyond the float-exact range. So $c$ is split into three 21-bit limbs, $c = 2^{42} c_2 + 2^{21} c_1 + c_0$. Then $A c_0$, $A c_1$, $A c_2$ are each exact BLAS products, since $n \cdot 2^{20} \cdot 2^{21} \le 2^{52}$ for $n \le 2048$, and all three go into one matrix–matrix call that reads $A$ from memory once. They are recombined in `int64`, where the arithmetic wraps modulo $2^{64}$. The new residual $2^s r - Ac$ is therefore computed exactly *modulo* $2^{64}$. It is the true residual whenever its true magnitude is below $2^{63}$. A fourth column computes $A c$ approximately in floating point. If that estimate of the new residual is below $2^{60}$, its error being far smaller than $2^{59}$, the wrapped result is the true value. Otherwise the step is retried with a smaller $s$. This is the same pattern as Chapter 3: a cheap float test that can only err on the side of caution.

**Choosing $s$.** The scale adapts. If the new residual came out large, the next $s$ is smaller, and if small, larger, targeting a residual of about $2^{31}$. It is also capped so that $2^s r$ and $c$ fit in 61 bits. If the float solve is too inaccurate to gain at least 4 bits per step, for 8 steps in a row, the matrix is too ill-conditioned for this method, and it raises `NotApplicable`.

**Recovering the exact answer.** $X/D$ approximates $x$ with error $E/D$, where $E \ge \lVert A^{-1} r \rVert_\infty$. If $x_j = a/q$ with $q \le Q$ and $D > 2Q^2E$, then $|x_j - X_j/D| < 1/(2q^2)$, and by Legendre's theorem $a/q$ is a convergent of the continued fraction of $X_j/D$. Moreover, the next convergent's denominator exceeds $Q$, so $a/q$ is precisely the last convergent with denominator at most $Q$. The common-denominator trick from 7.2 again means only one full continued fraction is usually needed.

The two stopping rules mirror variant A:

- **Proven:** stop when $D > 2Q^2E$ with $Q = 2^{\lceil \log_2 H \rceil}$ and the rigorous bound $E = n \cdot H \cdot \lVert r \rVert_\infty$. Here $\lVert A^{-1} \rVert_\infty \le n H$, since the entries of the adjugate are minors bounded by $H$ and $|\det A| \ge 1$. This bound is pessimistic, about $3\log_2 H$ bits against Dixon's $2\log_2 H$.
- **Early (variant B):** at the same checkpoints as Dixon, $64, 128, 256, \dots$ bits of precision, estimate $E$ from one more float solve, attempt reconstruction, and accept only if `exact_check` passes.

Neither proven path stops early when the residual happens to reach zero, although that would also be exact: if $r = 0$ the invariant gives $x = X/D$ directly. That is deliberate. A zero-residual exit is itself a form of output sensitivity, and leaving it out keeps variant B the only difference between the two stopping rules. On planted-answer problems, the gap between a proven variant and its early-stopping twin therefore measures all of B's benefit, including this case.

The proven path also runs `exact_check` once, at the end. The float LU never certifies that $A$ is nonsingular, and for a singular $A$ the reconstruction would otherwise return a vector that solves nothing. The check costs one exact matrix–vector product.

```python {#fast numeric}
LIMB = 21
LIMB_MASK = (1 << LIMB) - 1


def cf_denominator(num, den, qmax):
    num = abs(num)
    q0, q1 = 1, 0
    while den:
        a = num // den
        q2 = a * q1 + q0
        if q2 > qmax:
            break
        q0, q1 = q1, q2
        num, den = den, num - a * den
    return max(q1, 1)


def reconstruct_real(X, D, qmax, E):
    d = cf_denominator(int(X[0]), D, qmax)
    nums = [0] * len(X)
    j = 0
    while j < len(X):
        v = int(X[j]) * d
        num = (2 * v + D) // (2 * D)
        if abs(v - num * D) <= d * E:
            nums[j] = num
            j += 1
            continue
        e = cf_denominator(v, D, max(1, qmax // d))
        if e == 1:
            return None
        d *= e
        for i in range(j):
            nums[i] *= e
    return nums, d


def refine_step(Af, lu, r, s):
    y = scipy.linalg.lu_solve(lu, r.astype(np.float64), check_finite=False)
    if not np.isfinite(y).all():
        raise ZeroDivisionError("singular matrix")
    rbits = int(np.abs(r).max()).bit_length()
    while True:
        s = min(s, 61 - rbits, int(60 - math.log2(float(np.abs(y).max()) + 1.0)))
        if s < 1:
            raise NotApplicable("float solve makes no progress: matrix too ill-conditioned")
        c = np.rint(y * 2.0**s).astype(np.int64)
        limbs = np.column_stack([c & LIMB_MASK, (c >> LIMB) & LIMB_MASK, c >> (2 * LIMB), c]).astype(np.float64)
        M = Af @ limbs
        Ac = (M[:, 2].astype(np.int64) << (2 * LIMB)) + (M[:, 1].astype(np.int64) << LIMB) + M[:, 0].astype(np.int64)
        estimate = r.astype(np.float64) * 2.0**s - M[:, 3]
        if float(np.abs(estimate).max()) < 2.0**60:
            return c, (r << s) - Ac, s
        s -= 4


def numeric_solve(A, b, early):
    check_input(A, b)
    n = A.shape[0]
    log_num, log_den = log2_bounds(A, b)
    Af = A.astype(np.float64)
    lu = scipy.linalg.lu_factor(Af, check_finite=False)
    FA = flint.fmpz_mat(A.tolist())
    r = b.copy()
    pending, X, shift = [], None, 0
    s, slow_steps, next_check, steps = 20, 0, 64, 0
    while True:
        c, r, used = refine_step(Af, lu, r, s)
        pending.append((c.astype(object), 1 << used))
        shift += used
        steps += 1
        slow_steps = slow_steps + 1 if used < 4 else 0
        if slow_steps >= 8:
            raise NotApplicable("float solve makes no progress: matrix too ill-conditioned")
        rmax = int(np.abs(r).max())
        s = max(1, used + 31 - rmax.bit_length())
        proven = shift >= 3 * math.ceil(log_den) + math.ceil(math.log2(n)) + rmax.bit_length() + 2
        if not (proven or (early and shift >= next_check)):
            continue
        val, weight = combine_high_first(pending)
        X = val if X is None else X * weight + val
        pending = []
        D = 1 << shift
        if proven:
            E = n * (1 << math.ceil(log_den)) * (rmax + 1)
            res = reconstruct_real(X, D, 1 << math.ceil(log_den), E)
            if res is None or not exact_check(FA, res[0], res[1], b):
                raise ArithmeticError("no exact solution at the proven bound: matrix singular?")
            return res + (steps,)
        y = scipy.linalg.lu_solve(lu, r.astype(np.float64), check_finite=False)
        E = int(float(np.abs(y).max())) + 2
        res = reconstruct_real(X, D, math.isqrt(D // (2 * E)), E)
        if res is not None and exact_check(FA, res[0], res[1], b):
            return res + (steps,)
        next_check *= 2
```

The `FA` matrix is built in both modes, because both end with an exact check. Building it is part of the measured cost.

## 7.4 Variant A applied to determinants

A solve gives the determinant almost for free, up to a small factor. With $x = \text{nums}/d$ in lowest common terms, Cramer's rule says $d$ divides $\det A$. So $\det A = d \cdot s$ for an integer $s$ with $|s| \le H/d$. The cofactor $s$ is found modulo 62-bit primes $q$, from $\det A \bmod q$ divided by $d \bmod q$, and combined by the Chinese remainder theorem until the product of the primes exceeds $2H/d$. That gives a proven exact determinant. FLINT's own `fmpz_mat_det` uses this same divisor idea internally, so this variant isolates what the BLAS-based solve contributes.

The catch, visible already in a probe, is that Hadamard's bound overestimates $|\det A|$ for random matrices by roughly $\tfrac{n}{2}\log_2 e \approx 0.72n$ bits. Proving $s$ therefore takes about $0.72n/62$ primes, each needing an $O(n^3)$ modular determinant. The benchmark measures whether the faster solve pays for that. The solve uses early termination: any solution that passes the exact check is the true solution, so its common denominator is exactly the $d$ that divides $\det A$, and nothing is lost by stopping early. The first 62-bit modular determinant is computed before the solve. If it is zero, the matrix is probably singular, and it goes straight to FLINT rather than wasting a solve. Any matrix a solver declines or cannot handle also goes to FLINT, so the function always returns the exact determinant.

```python {#fast det}
def det_via_solve(A, solver, seed=0):
    n = A.shape[0]
    rows = A.tolist()
    q = prime_below(2**62)
    det_q = int(flint.nmod_mat(rows, q).det())
    if det_q == 0:
        return int(flint.fmpz_mat(rows).det())
    b = np.random.default_rng(seed).integers(-(2**20), 2**20, size=n, dtype=np.int64)
    try:
        _, d, _ = solver(A, b, True)
        _, log_den = log2_bounds(A, b)
    except (ZeroDivisionError, NotApplicable, ArithmeticError):
        return int(flint.fmpz_mat(rows).det())
    bound = 2 * 2 ** max(0, math.ceil(log_den - math.log2(d) + 1))
    s, modulus = 0, 1
    while modulus <= bound:
        if d % q:
            s_q = det_q * pow(d, -1, q) % q
            s += modulus * ((s_q - s) * pow(modulus, -1, q) % q)
            modulus *= q
        q = prime_below(q)
        det_q = int(flint.nmod_mat(rows, q).det())
    if s > modulus // 2:
        s -= modulus
    return d * s
```

## 7.5 Variant D: the 128-bit fallback

When a column computation outgrows 64 bits, the fallback in Chapter 4 drops into Python's unbounded integers. Variant D keeps it compiled by working with numbers made of two 64-bit words. Think of it as doing arithmetic in base $2^{64}$ with two-digit numbers. The building blocks, all in unsigned 64-bit arithmetic under Numba:

- **$64 \times 64 \to 128$ multiplication**, by splitting each operand into 32-bit halves $a = 2^{32}a_1 + a_0$ and summing the four partial products with their carries. That is schoolbook multiplication in base $2^{32}$, which never overflows a 64-bit word.
- **128-bit addition**: add the low words, detect the carry as "the sum came out smaller than an addend", and add it into the high words.
- **Negation** in two's complement: complement both words and add one, carrying into the high word when the low word wraps to zero.
- **Magnitude of an `int64`**, computed as $-(v+1) + 1$ in unsigned arithmetic so that $-2^{63}$ is handled without overflow.

**Multiply–add.** $p_i q_i + r_i \sigma_c$ with every operand in `int64` is below $2^{127}$ in magnitude, so it always fits a signed 128-bit result. The fallback therefore stays **exact**. Instead of rounding the overflowing elements with $\rho$, as Chapter 4 does, the result column widens to 128-bit numerators over the shared denominator, 16 bytes per value. The variant handles $\sigma_{ab} = 1$, the case benchmarked. For $\sigma_{ab} \ne 1$ the triple product $p_i q_i \sigma_{ab}$ can need more than 128 bits, and rather than fall back to rounding, the function declines with an error.

**Variance.** Only the bad blocks, those containing an element with $|p_i| \ge 2^{32}$, go to the wide kernel. It accumulates $\sum p_i$ in a signed 128-bit word and $\sum p_i^2$ in three unsigned 64-bit words, a 192-bit accumulator. With $|p_i| < 2^{63}$ and $n < 2^{31}$, these hold up to $2^{94}$ and $2^{157}$ respectively, so neither can overflow.

```python {#wide kernels}
U32 = np.uint64(0xFFFFFFFF)
S32 = np.uint64(32)
ONE = np.uint64(1)
ZERO = np.uint64(0)


@njit(inline="always")
def magnitude(v):
    if v >= 0:
        return np.uint64(v), False
    return np.uint64(-(v + 1)) + ONE, True


@njit(inline="always")
def mul_u64(a, b):
    a0, a1 = a & U32, a >> S32
    b0, b1 = b & U32, b >> S32
    p00, p01, p10, p11 = a0 * b0, a0 * b1, a1 * b0, a1 * b1
    mid = (p00 >> S32) + (p01 & U32) + (p10 & U32)
    lo = (p00 & U32) | (mid << S32)
    hi = p11 + (p01 >> S32) + (p10 >> S32) + (mid >> S32)
    return hi, lo


@njit(inline="always")
def neg128(hi, lo):
    lo = ~lo + ONE
    hi = ~hi + (ONE if lo == ZERO else ZERO)
    return hi, lo


@njit(inline="always")
def add128(ahi, alo, bhi, blo):
    lo = alo + blo
    hi = ahi + bhi + (ONE if lo < alo else ZERO)
    return hi, lo


@njit(inline="always")
def smul128(a, b):
    ua, na = magnitude(a)
    ub, nb = magnitude(b)
    hi, lo = mul_u64(ua, ub)
    if na != nb:
        hi, lo = neg128(hi, lo)
    return hi, lo


@njit
def fma128_kernel(p, q, r, s_c, out_hi, out_lo):
    for i in range(len(p)):
        thi, tlo = smul128(p[i], q[i])
        uhi, ulo = smul128(r[i], s_c)
        out_hi[i], out_lo[i] = add128(thi, tlo, uhi, ulo)


@njit
def moments128_kernel(nums, bad):
    s_hi, s_lo = ZERO, ZERO
    q2, q1, q0 = ZERO, ZERO, ZERO
    n = len(nums)
    for blk in range(len(bad)):
        if not bad[blk]:
            continue
        for i in range(blk * BLOCK, min((blk + 1) * BLOCK, n)):
            v = nums[i]
            u, negative = magnitude(v)
            vhi, vlo = (ZERO, u) if not negative else neg128(ZERO, u)
            s_hi, s_lo = add128(s_hi, s_lo, vhi, vlo)
            hi, lo = mul_u64(u, u)
            t0 = q0 + lo
            c0 = ONE if t0 < q0 else ZERO
            t1 = q1 + hi
            c1 = ONE if t1 < q1 else ZERO
            t1c = t1 + c0
            c1 += ONE if t1c < t1 else ZERO
            q0, q1, q2 = t0, t1c, q2 + c1
    return s_hi, s_lo, q2, q1, q0


def to_int128(hi, lo):
    v = (int(hi) << 64) | int(lo)
    return v - (1 << 128) if v >> 127 else v
```

The column functions mirror Chapter 4's, swapping in the wide kernels where Chapter 4 would call Python.

```python {#wide columns}
class WideColumn:
    def __init__(self, hi, lo, den):
        self.hi, self.lo, self.den = hi, lo, den

    def __len__(self):
        return len(self.hi)

    def value(self, i):
        return Fraction(to_int128(self.hi[i], self.lo[i]), self.den)

    bytes_per_value = 16


def column_fma_wide(a, b, c, out):
    D = math.lcm(a.den * b.den, c.den)
    if D > M:
        raise NotImplementedError("result denominator exceeds 64 bits")
    s_ab, s_c = D // (a.den * b.den), D // c.den
    bad = fma_kernel(a.nums, b.nums, c.nums, np.int64(s_ab), np.int64(s_c), out)
    if bad == 0:
        return SharedColumn(out, D)
    if s_ab != 1:
        raise NotImplementedError("wide multiply-add implemented for sigma_ab = 1 only")
    hi = np.empty(len(out), dtype=np.uint64)
    lo = np.empty(len(out), dtype=np.uint64)
    fma128_kernel(a.nums, b.nums, c.nums, np.int64(s_c), hi, lo)
    return WideColumn(hi, lo, D)


def column_var_wide(col):
    n = len(col)
    assert n < 2**31
    s1, hi, lo, bad = moments_kernel(col.nums)
    s1, s2 = int(s1), (int(hi) << 32) + int(lo)
    if bad.any():
        w_hi, w_lo, q2, q1, q0 = moments128_kernel(col.nums, bad)
        s1 += to_int128(w_hi, w_lo)
        s2 += (int(q2) << 128) | (int(q1) << 64) | int(q0)
    return Fraction(n * s2 - s1 * s1, n * (n - 1) * col.den**2)
```

When the fast multiply–add kernel reports overflow, the whole column is recomputed wide rather than only the overflowing elements. A column is one type, and mixing 64-bit and 128-bit numerators would break that. The wide kernel is not vectorised: its carry tests are branches. It is measured as is.

## 7.6 Correctness tests

**The solvers against FLINT.** Random systems of several sizes, including $n = 1$ and $2$. Each of the four solver configurations must return exactly FLINT's answer. Systems with planted integer answers must return those answers, and for $n \ge 30$ the early-stopping configurations must take fewer than half the steps of the proven ones, which shows that variant B actually triggers. A matrix with a zero row must raise rather than return.

**Declining rather than guessing.** Four kinds of awkward input, none of which may produce a wrong answer:

- An integer-scaled Hilbert matrix, $\operatorname{lcm}(1, \dots, 15) \cdot H_8$, with entries below $2^{19}$ and condition number about $10^{10}$. All four configurations must return the exact answer. In probes the numeric solver coped with condition numbers up to about $10^{15}$.
- A $5 \times 5$ upper-bidiagonal matrix with $1$ on the diagonal and $-2^{19}$ above it. Its determinant is $1$ but its inverse has entries up to $2^{76}$, so floats cannot represent the corrections. The numeric solver must decline with `NotApplicable`, and Dixon must still return the exact answer.
- A rank-deficient matrix with no zero row, in which one row is a combination of three others, and a matrix with a zero row. Every configuration must raise an error, never return a vector.
- Inputs outside the float-exact range, including entries equal to $-2^{63}$, a right-hand side of exactly $-2^{52}$, and an empty system. All must raise `NotApplicable`.

**Lifting-prime edge case.** A nonsingular $2 \times 2$ matrix whose determinant is exactly the first prime Dixon tries, so the modular inverse fails and the retry path runs. It must still return FLINT's answer.

**Near the size limits.** $n = 2048$ with entries up to $2^{19}$ and a planted answer, where the limb bound $n \cdot 2^{20} \cdot 2^{21} \le 2^{52}$ and the Dixon prime bound are tightest. Only the early-stopping configurations run here. The proven ones would need thousands of lifting steps and add nothing to the check.

**Determinants** of random matrices for $n$ from 1 to 40 must equal FLINT's for both solvers, as must the determinants of a matrix with a repeated row and of a rank-deficient matrix with no repeated row, which are 0.

**The 128-bit kernels** against Python integers on random operands of every magnitude, plus the edge values $0$, $\pm 1$, $\pm(2^{63} - 1)$ and $-2^{63}$. Then the wide column functions against `Fraction` arithmetic, including a column where only one block is bad.

```python {#tests speedups}
def random_system(rng, n, planted=False):
    A = np.array([[rng.randint(-10**6, 10**6) for _ in range(n)] for _ in range(n)], dtype=np.int64)
    if planted:
        m = [rng.randint(-1000, 1000) for _ in range(n)]
        b = np.array([sum(int(A[i, j]) * m[j] for j in range(n)) for i in range(n)], dtype=np.int64)
        return A, b, [Fraction(v) for v in m]
    b = np.array([rng.randint(-10**6, 10**6) for _ in range(n)], dtype=np.int64)
    X = flint.fmpz_mat(A.tolist()).solve(flint.fmpz_mat(n, 1, b.tolist()))
    return A, b, [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(n)]


SOLVERS = [(dixon_solve, False), (dixon_solve, True), (numeric_solve, False), (numeric_solve, True)]


def test_solvers_match_flint():
    rng = random.Random(50)
    for n in (1, 2, 3, 7, 30, 80):
        for planted in (False, True):
            A, b, want = random_system(rng, n, planted)
            steps = {}
            for solver, early in SOLVERS:
                nums, d, steps[solver, early] = solver(A, b, early)
                assert [Fraction(v, d) for v in nums] == want, (solver.__name__, early, n, planted)
            if planted and n >= 30:
                for solver in (dixon_solve, numeric_solve):
                    assert steps[solver, True] < steps[solver, False] / 2, (solver.__name__, steps)


def expect_error(fn, *errors):
    try:
        fn()
    except errors:
        return
    raise AssertionError(f"expected one of {errors}")


def test_solvers_decline_rather_than_guess():
    n = 8
    L = math.lcm(*range(1, 2 * n))
    H = np.array([[L // (i + j + 1) for j in range(n)] for i in range(n)], dtype=np.int64)
    b = H @ np.ones(n, dtype=np.int64)
    for solver, early in SOLVERS:
        nums, d, _ = solver(H, b, early)
        assert [Fraction(v, d) for v in nums] == [1] * n
    K = 2**19
    B = np.eye(5, dtype=np.int64) - K * np.eye(5, k=1, dtype=np.int64)
    rhs = np.arange(1, 6, dtype=np.int64)
    want = flint.fmpz_mat(B.tolist()).solve(flint.fmpz_mat(5, 1, rhs.tolist()))
    for early in (False, True):
        expect_error(lambda: numeric_solve(B, rhs, early), NotApplicable)
        nums, d, _ = dixon_solve(B, rhs, early)
        assert all(Fraction(nums[i], d) == Fraction(int(want[i, 0].p), int(want[i, 0].q)) for i in range(5))
    rng = random.Random(54)
    S = np.array([[rng.randint(-1000, 1000) for _ in range(6)] for _ in range(6)], dtype=np.int64)
    S[5] = 3 * S[0] - 7 * S[1] + S[2]
    for solver, early in SOLVERS:
        expect_error(lambda: solver(S, np.ones(6, dtype=np.int64), early), ZeroDivisionError, NotApplicable, ArithmeticError)
    Z = np.zeros((3, 3), dtype=np.int64)
    Z[0, 0] = Z[1, 1] = 1
    for solver, early in SOLVERS:
        expect_error(lambda: solver(Z, np.ones(3, dtype=np.int64), early), ZeroDivisionError)
    good = np.array([[5, 1], [2, 3]], dtype=np.int64)
    cases = [(np.full((2, 2), 2**21, dtype=np.int64), np.ones(2, dtype=np.int64)),
             (good, np.array([INT64_MIN, 1], dtype=np.int64)),
             (np.array([[INT64_MIN, 1], [2, 3]], dtype=np.int64), np.ones(2, dtype=np.int64)),
             (good, np.array([-(2**52), 1], dtype=np.int64)),
             (np.zeros((0, 0), dtype=np.int64), np.zeros(0, dtype=np.int64))]
    for A, rhs in cases:
        for solver, early in SOLVERS:
            expect_error(lambda: solver(A, rhs, early), NotApplicable)


def test_dixon_singular_mod_first_prime():
    n = 2
    p = prime_below(min(math.isqrt(2**53 // n), 2**53 // (n * 2**20)))
    a = 2**19 - 1
    d = -(-p // a)
    A = np.array([[a, 1], [a * d - p, d]], dtype=np.int64)
    assert int(flint.fmpz_mat(A.tolist()).det()) == p and np.abs(A).max() < 2**20
    rhs = np.array([3, -5], dtype=np.int64)
    want = flint.fmpz_mat(A.tolist()).solve(flint.fmpz_mat(2, 1, rhs.tolist()))
    for early in (False, True):
        nums, den, _ = dixon_solve(A, rhs, early)
        assert all(Fraction(nums[i], den) == Fraction(int(want[i, 0].p), int(want[i, 0].q)) for i in range(2))


def test_large_n_near_limits():
    n = 2048
    rng = np.random.default_rng(55)
    A = rng.integers(-(2**19), 2**19, size=(n, n), dtype=np.int64)
    m = rng.integers(-3, 4, size=n, dtype=np.int64)
    rhs = A @ m
    for solver in (dixon_solve, numeric_solve):
        nums, d, _ = solver(A, rhs, True)
        assert d == 1 and nums == m.tolist(), solver.__name__


def test_det_via_solve():
    rng = random.Random(51)
    for n in list(range(1, 12)) + [25, 40]:
        A = np.array([[rng.randint(-10**6, 10**6) for _ in range(n)] for _ in range(n)], dtype=np.int64)
        want = int(flint.fmpz_mat(A.tolist()).det())
        for solver in (dixon_solve, numeric_solve):
            assert det_via_solve(A, solver) == want, (n, solver.__name__)
    A = np.array([[rng.randint(-99, 99) for _ in range(5)] for _ in range(5)], dtype=np.int64)
    A[4] = A[0]
    for solver in (dixon_solve, numeric_solve):
        assert det_via_solve(A, solver) == 0
        S = np.array([[rng.randint(-1000, 1000) for _ in range(10)] for _ in range(10)], dtype=np.int64)
        S[9] = 3 * S[0] - 7 * S[1] + S[2]
        assert det_via_solve(S, solver) == 0


def test_wide_kernels():
    rng = random.Random(52)
    edges = [0, 1, -1, M, -M, INT64_MIN]
    pick = lambda: rng.choice(edges) if rng.random() < 0.1 else rng.randint(INT64_MIN, M)
    n = 5000
    p, q, r = (np.array([pick() for _ in range(n)], dtype=np.int64) for _ in range(3))
    for s_c in (1, 1000, 10**6):
        hi, lo = np.empty(n, dtype=np.uint64), np.empty(n, dtype=np.uint64)
        fma128_kernel(p, q, r, np.int64(s_c), hi, lo)
        assert all(to_int128(hi[i], lo[i]) == int(p[i]) * int(q[i]) + int(r[i]) * s_c for i in range(n))
    bad = np.ones((n + BLOCK - 1) // BLOCK, dtype=np.bool_)
    s_hi, s_lo, q2, q1, q0 = moments128_kernel(p, bad)
    xs = list(map(int, p.tolist()))
    assert to_int128(s_hi, s_lo) == sum(xs)
    assert (int(q2) << 128) | (int(q1) << 64) | int(q0) == sum(v * v for v in xs)


def test_wide_columns():
    rng = random.Random(53)
    cols = [SharedColumn(np.array([rng.randint(-10**10, 10**10) for _ in range(3000)], dtype=np.int64), 10**6)
            for _ in range(3)]
    res = column_fma_wide(*cols, np.empty(3000, dtype=np.int64))
    assert isinstance(res, WideColumn)
    assert all(res.value(i) == cols[0].value(i) * cols[1].value(i) + cols[2].value(i) for i in range(3000))
    nums = np.array([rng.randint(-10**7, 10**7) for _ in range(3 * BLOCK)], dtype=np.int64)
    nums[BLOCK + 9] = 2**62
    col = SharedColumn(nums, 1000)
    assert column_var_wide(col) == column_var(col)
```

```python {file=build/fastsolve.py}
import math

import flint
import numpy as np
import scipy.linalg


<<fast common>>


<<fast bounds>>


<<fast dixon setup>>


<<fast combine>>


<<fast ratrecon>>


<<fast dixon>>


<<fast numeric>>


<<fast det>>
```

```python {file=build/wide.py}
import math
from fractions import Fraction

import numpy as np
from numba import njit

from q64 import M
from columns import BLOCK, SharedColumn, fma_kernel, column_fma, moments_kernel


<<wide kernels>>


<<wide columns>>
```

```python {file=build/test_speedups.py}
import math
import random
from fractions import Fraction

import flint
import numpy as np

from q64 import M, INT64_MIN
from columns import BLOCK, SharedColumn, column_var
from fastsolve import NotApplicable, dixon_solve, numeric_solve, det_via_solve, prime_below
from wide import WideColumn, fma128_kernel, moments128_kernel, to_int128, column_fma_wide, column_var_wide


<<tests speedups>>
```

## 7.7 The benchmarks

Each variant is compared with the method it would replace: FLINT for linear algebra, the Chapter 4 Python fallback for the wide rows. All contenders start from the same NumPy `int64` arrays, so FLINT's timing includes building its matrices from them, just as the variants' timings include their own conversions to FLINT and float formats. In Chapter 6 FLINT's conversion was excluded and reported separately; here it is included because the variants cannot avoid theirs. NumPy appears in every table for context. The same harness, interleaving and fairness settings as Chapter 6 apply. All answers are verified against FLINT outside the timed region.

**Solve, random right-hand side (S1).** The Chapter 6 problems: three-place decimal matrices and right-hand sides, $n = 100, 500, 1000$. The exact answers are huge, about 1 KB per entry at $n = 1000$. Dixon's proven bound is tight for such answers, so for variant A early termination cannot stop meaningfully sooner, and what it costs is measured. Variant C's proven bound is looser, about $3\log_2 H$ bits against the roughly $2\log_2 H$ actually needed, so for C early termination can help even here.

**Solve, planted answer (S2).** The same matrices, with $b = Am$ for a vector $m$ of integers in $[-1000, 1000]$, so the exact answer is small. This models a system whose true solution is simple, such as a consistent set of measurements, and it is where early termination should pay.

**Determinant (S3).** The Chapter 6 matrices, $n = 100, 500, 1000$, with the determinant obtained from each solver as in Section 7.4.

**Wide fallback (S4).** The two past-limit rows from Chapter 6, at $10^6$ values: six-place multiply–add, and variance at offset $10^7$. For the multiply–add the comparison is not like-for-like in output: the Chapter 4 path rounds every element by $\rho$, while variant D returns the exact 128-bit values. D does strictly more for the user, so the speed ratio understates its advantage, if anything.

```python {#speed solve}
def flint_solution(FA, b):
    n = len(b)
    X = FA.solve(flint.fmpz_mat(n, 1, b.tolist()))
    return [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(n)]


def bench_solve_variants(report, n, planted):
    N, den, Af = decimal_matrix(n, n, seed=12)
    if planted:
        m = np.random.default_rng(17).integers(-1000, 1001, size=n, dtype=np.int64)
        b = N @ m
        label = "S2 solve, planted answer"
    else:
        b = decimal_matrix(n, 1, seed=13)[0][:, 0]
        label = "S1 solve, random rhs"
    b = np.ascontiguousarray(b)
    FN, FB = flint.fmpz_mat(N.tolist()), flint.fmpz_mat(n, 1, b.tolist())
    want = flint_solution(FN, b)
    Nf, bf = N.astype(np.float64), b.astype(np.float64)
    contenders = {
        "NumPy float64": lambda: np.linalg.solve(Nf, bf),
        "FLINT": lambda: flint.fmpz_mat(N.tolist()).solve(flint.fmpz_mat(n, 1, b.tolist())),
        "A: Dixon on BLAS": lambda: dixon_solve(N, b, False),
        "A+B: Dixon on BLAS, early stop": lambda: dixon_solve(N, b, True),
        "C: numeric-symbolic": lambda: numeric_solve(N, b, False),
        "C+B: numeric-symbolic, early stop": lambda: numeric_solve(N, b, True),
    }
    res = measure(contenders)
    extra = {"NumPy float64": dict(error=norm_rel_error(np.linalg.solve(Nf, bf), want), bytes_per_value=8),
             "FLINT": dict(error="0 (exact)")}
    for name, fn in contenders.items():
        if name in extra:
            continue
        nums, d, steps = fn()
        ok = [Fraction(v, d) for v in nums] == want
        extra[name] = dict(error=verify(ok, f"{label} {name}", "0 (matches FLINT)"), note=f"{steps} steps")
    report.add_measured(label, n, res, "FLINT", **extra)
```

The NumPy contender solves the integer system $N x = b$ directly, which is the same system the exact solvers see, so its error is measured against the same exact answer.

```python {#speed det}
def bench_det_variants(report, n):
    N, den, Af = decimal_matrix(n, n, seed=14)
    FN = flint.fmpz_mat(N.tolist())
    want = int(FN.det())
    contenders = {
        "FLINT": lambda: flint.fmpz_mat(N.tolist()).det(),
        "A: via Dixon on BLAS": lambda: det_via_solve(N, dixon_solve),
        "C: via numeric-symbolic": lambda: det_via_solve(N, numeric_solve),
    }
    res = measure(contenders)
    extra = {"FLINT": dict(error="0 (exact)")}
    for name in ("A: via Dixon on BLAS", "C: via numeric-symbolic"):
        extra[name] = dict(error=verify(contenders[name]() == want, f"det {name}", "0 (matches FLINT)"))
    report.add_measured("S3 determinant", n, res, "FLINT", **extra)
```

```python {#speed wide}
def bench_wide(report, n):
    (pa, den, a), (pb, _, b), (pc, _, c) = (decimal_column(n, 6, 10**4, seed=s) for s in (5, 6, 7))
    A, B, C = SharedColumn(pa, den), SharedColumn(pb, den), SharedColumn(pc, den)
    buf_np, buf_old, buf_new = np.empty(n), np.empty(n, dtype=np.int64), np.empty(n, dtype=np.int64)

    def run_numpy():
        np.multiply(a, b, out=buf_np)
        np.add(buf_np, c, out=buf_np)

    res = measure({"NumPy float64": run_numpy,
                   "Python fallback (ch. 4)": lambda: column_fma(A, B, C, buf_old),
                   "D: 128-bit fallback": lambda: column_fma_wide(A, B, C, buf_new)})
    wide = column_fma_wide(A, B, C, buf_new)
    P, Q, R = (list(map(int, v.tolist())) for v in (pa, pb, pc))
    ok = isinstance(wide, WideColumn) and all(
        to_int128(h, l) == x * y + z * den for h, l, x, y, z in zip(wide.hi.tolist(), wide.lo.tolist(), P, Q, R))
    idx = random.Random(42).sample(range(n), 20000)
    exact = [Fraction(P[i] * Q[i], den * den) + Fraction(R[i], den) for i in idx]
    old = column_fma(A, B, C, buf_old)
    run_numpy()
    report.add_measured("S4a a*b+c, 6 places", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=norm_rel_error([buf_np[i] for i in idx], exact), bytes_per_value=8),
        "Python fallback (ch. 4)": dict(error=exact_norm_err([old.value(i) for i in idx], exact), bytes_per_value=16,
                                        note="rounded by rho"),
        "D: 128-bit fallback": dict(error=verify(ok, "wide fma", "0 (verified, all elements)"), bytes_per_value=16,
                                    note="exact, 128-bit numerators"),
    })
    nums, vden, xf = offset_column(n, offset=10**7)
    col = SharedColumn(nums, vden)
    want = exact_two_pass_var(nums, vden)
    res = measure({"NumPy float64": lambda: xf.var(ddof=1), "Python fallback (ch. 4)": lambda: column_var(col),
                   "D: 128-bit fallback": lambda: column_var_wide(col)})
    report.add_measured("S4b variance, offset 1e7", n, res, "NumPy float64", **{
        "NumPy float64": dict(error=rel_error(xf.var(ddof=1), want), bytes_per_value=8),
        "Python fallback (ch. 4)": dict(error=verify(column_var(col) == want, "var old"), bytes_per_value=8),
        "D: 128-bit fallback": dict(error=verify(column_var_wide(col) == want, "var wide"), bytes_per_value=8),
    })
```

```python {#speed main}
def main():
    quick = "--quick" in sys.argv
    print("correctness tests")
    import test_speedups
    for name in sorted(n for n in dir(test_speedups) if n.startswith("test_")):
        t = time.perf_counter()
        getattr(test_speedups, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    meta = metadata()
    report = Report()
    sizes = (50, 100) if quick else (100, 500, 1000)
    steps = [*[(f"S1 n={n}", lambda n=n: bench_solve_variants(report, n, False)) for n in sizes],
             *[(f"S2 n={n}", lambda n=n: bench_solve_variants(report, n, True)) for n in sizes],
             *[(f"S3 n={n}", lambda n=n: bench_det_variants(report, n)) for n in sizes],
             ("S4 wide", lambda: bench_wide(report, 10**5 if quick else 10**6))]
    for name, step in steps:
        t = time.perf_counter()
        step()
        print(f"  done  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / f"speedups{suffix}.md").write_text(f"# Speed-up variants\n\n{header}\n\n{report.markdown()}\n")
    (out / f"speedups{suffix}.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

The speed-up runner reuses Chapter 6's setup, helpers and metadata by importing them, so the thread pinning happens before NumPy loads, exactly as in the main run.

```python {file=build/run_speedups.py}
from run_benchmarks import *
from run_benchmarks import exact_two_pass_var, exact_norm_err, metadata, verify

from fastsolve import dixon_solve, numeric_solve, det_via_solve
from wide import WideColumn, column_fma_wide, column_var_wide, to_int128


<<speed solve>>


<<speed det>>


<<speed wide>>


<<speed main>>
```
