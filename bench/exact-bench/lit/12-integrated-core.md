# 12. The integrated exact core

Chapters 3 to 11 tested ideas one at a time. This chapter combines the ones that were accepted into a single library, and measures it end to end against NumPy on four realistic tasks. The accepted pieces are:

| Piece | From | Role here |
|---|---|---|
| Exact rational columns | Ch. 4 | the main data type: integers over one shared denominator |
| Integer tiers: 64-bit, 128-bit, arbitrary, with demotion | Ch. 4, 7 | every column lives in the smallest tier that holds it exactly |
| Tagged complex numbers (sign + imaginary bit) | Ch. 10 | complex columns, one part or two, with per-column dispatch |
| Floating-slash tagged scalars | Ch. 8, 10 | the type of results that leave a column, such as sums and means |
| Numeric–symbolic solve with early stop, then Dixon on BLAS, then FLINT | Ch. 7, 9 | one `exact_solve` entry point |

Dropped ideas (the determinant shortcut, 8/16-bit formats, and factoring out 2s and 5s) are not used.

**What is and is not compiled.** The column kernels for the 64-bit and 128-bit tiers are compiled with Numba. The arbitrary-precision tier uses NumPy object arrays of Python integers, which is exact but interpreted. The floating-slash scalar is the Chapter 10 Python prototype: results are exact `Fraction`s classified into the word or big tier. It is not packed into compiled 128-bit words. So this chapter measures a realistic *library* built from these pieces, not the final compiled language.

## 12.1 Tiered rational columns

A `RatCol` is a vector of rationals $p_i / d$ sharing one denominator $d$. Its numerators live in exactly one tier:

- **`i64`:** an `int64` array, with $|p_i| \le 2^{63} - 1$, so negation never overflows;
- **`i128`:** two `uint64` arrays, high and low words of a two's-complement 128-bit integer, with $|p_i| \le 2^{127} - 1$;
- **`big`:** an object array of Python integers.

**Promotion.** Each operation runs in the higher of the two operands' tiers, the lowest tier that can hold both, with a compiled kernel that counts overflowing elements. If any element overflows, the operation is redone one tier up. The 64-bit kernels use Chapter 3's checked primitives. Multiplying two 64-bit columns always goes straight to 128 bits, since a 64×64 product always fits, and then tries to demote.

**Demotion.** After every operation that can widen a column (addition, subtraction, multiplication, scaling), a 128-bit result is checked: if every element fits 64 bits, it drops back to `i64`. A `big` result is classified from scratch. Columns entering the library are also checked: an input containing $-2^{63}$ goes straight to the 128-bit tier, because negating it in 64 bits would wrap. This is the rule settled earlier: a value lives in the smallest tier that holds it exactly, and never stays wide when it doesn't need to.

**Reducing the shared denominator.** After any operation that changes the denominator (multiplication, scaling, division), the column divides out $g = \gcd(d, p_1, \dots, p_n)$, computed with NumPy's vectorised `gcd` for the 64-bit tier. Smaller numerators are what make demotion possible.

### Compiled kernels

The 128-bit helpers, `magnitude`, `mul_u64`, `neg128`, `add128` and `smul128`, are reused from Chapter 7. New here are checked addition and scaling for both tiers, conversion from 64 to 128 bits, and the "fits in 64 bits" test. Signed 128-bit addition overflows exactly when both operands have the same sign and the result has the other. The kernel also rejects a result of exactly $-2^{127}$. It is a valid two's-complement value, but it lies outside the tier's symmetric range, and negating it would wrap silently. The independent review found that hole by constructing it. Scaling a 128-bit value by a 64-bit integer multiplies magnitudes as two $64 \times 64 \to 128$ partial products. It overflows if anything reaches bit 127.

```python {#core kernels}
ALL_ONES = np.uint64(0xFFFFFFFFFFFFFFFF)
MIN_HI = np.uint64(0x8000000000000000)
TOP = np.uint64(63)
I128_MAX = 2**127 - 1


@njit
def add_i64_kernel(a, b, negate_b, out):
    bad = 0
    for i in range(len(a)):
        y = -b[i] if negate_b else b[i]
        s, o = add_ovf(a[i], y)
        out[i] = s
        bad += o
    return bad


@njit
def scale_i64_kernel(a, s, out):
    bad = 0
    for i in range(len(a)):
        p, o = mul_ovf(a[i], s)
        out[i] = p
        bad += o
    return bad


@njit
def i64_to_i128_kernel(a, hi, lo):
    for i in range(len(a)):
        v = a[i]
        lo[i] = np.uint64(v)
        hi[i] = ALL_ONES if v < 0 else np.uint64(0)


@njit
def mul_i64_i128_kernel(a, b, hi, lo):
    for i in range(len(a)):
        hi[i], lo[i] = smul128(a[i], b[i])


@njit
def add_i128_kernel(ahi, alo, bhi, blo, negate_b, ohi, olo):
    bad = 0
    for i in range(len(ahi)):
        h, l = bhi[i], blo[i]
        if negate_b:
            h, l = neg128(h, l)
        rh, rl = add128(ahi[i], alo[i], h, l)
        sa, sb, sr = ahi[i] >> TOP, h >> TOP, rh >> TOP
        bad += ((sa == sb) and (sr != sa)) or (rh == MIN_HI and rl == np.uint64(0))
        ohi[i], olo[i] = rh, rl
    return bad


@njit
def scale_i128_kernel(hi, lo, s, ohi, olo):
    us, sneg = magnitude(s)
    bad = 0
    for i in range(len(hi)):
        neg = (hi[i] >> TOP) == np.uint64(1)
        mh, ml = (neg128(hi[i], lo[i]) if neg else (hi[i], lo[i]))
        p1h, p1l = mul_u64(ml, us)
        p2h, p2l = mul_u64(mh, us)
        mid = p1h + p2l
        carry = np.uint64(1) if mid < p1h else np.uint64(0)
        top = p2h + carry
        over = top != np.uint64(0) or (mid >> TOP) != np.uint64(0)
        rh, rl = mid, p1l
        if neg != sneg:
            rh, rl = neg128(rh, rl)
        ohi[i], olo[i] = rh, rl
        bad += over
    return bad


@njit
def fits64_kernel(hi, lo):
    lim = np.uint64(0x7FFFFFFFFFFFFFFF)
    for i in range(len(hi)):
        if hi[i] == np.uint64(0):
            if lo[i] > lim:
                return False
        elif hi[i] == ALL_ONES:
            if lo[i] <= lim + np.uint64(1):
                return False
        else:
            return False
    return True
```

### Conversions between tiers

```python {#core conversions}
def big_of_i128(hi, lo):
    h = hi.astype(object)
    out = (h << 64) | lo.astype(object)
    return np.where(hi >> TOP == np.uint64(1), out - (1 << 128), out)


def split_i128(obj):
    vals = [int(v) for v in obj]
    lo = np.array([v & 0xFFFFFFFFFFFFFFFF for v in vals], dtype=np.uint64)
    hi = np.array([(v >> 64) & 0xFFFFFFFFFFFFFFFF for v in vals], dtype=np.uint64)
    return hi, lo
```

### The column type

Addition and subtraction first bring both operands to the common denominator $\operatorname{lcm}(d_1, d_2)$ by integer scaling, which may itself promote. Then they add in the highest tier the two operands occupy. Group-by statistics use a compiled scatter kernel for the 64-bit tier, with per-group exact fallback. There is no compiled 128-bit group kernel yet, so a 128-bit column's group statistics go through Python integers. In E2 that one step is more than half of the exact core's time, and a compiled 128-bit scatter kernel is the obvious next optimisation.

```python {#core ratcol}
class RatCol:
    __slots__ = ("tier", "data", "den")

    def __init__(self, tier, data, den):
        self.tier, self.data, self.den = tier, data, den

    @staticmethod
    def decimals(nums, den):
        nums = np.ascontiguousarray(nums, dtype=np.int64)
        if len(nums) and nums.min() == -M - 1:
            return RatCol.from_big(nums.astype(object), den)
        return RatCol("i64", nums, den)

    @staticmethod
    def from_big(obj, den, reduce=True):
        obj = np.asarray(obj, dtype=object)
        top = max((abs(int(v)) for v in obj), default=0)
        if top <= M:
            col = RatCol("i64", np.array([int(v) for v in obj], dtype=np.int64), den)
        elif top <= I128_MAX:
            col = RatCol("i128", split_i128(obj), den)
        else:
            col = RatCol("big", obj, den)
        return col.reduce() if reduce else col

    def __len__(self):
        return len(self.data[0]) if self.tier == "i128" else len(self.data)

    def big(self):
        if self.tier == "i64":
            return self.data.astype(object)
        if self.tier == "i128":
            return big_of_i128(*self.data)
        return self.data

    def i128(self):
        if self.tier == "i128":
            return self.data
        n = len(self)
        hi, lo = np.empty(n, dtype=np.uint64), np.empty(n, dtype=np.uint64)
        i64_to_i128_kernel(self.data, hi, lo)
        return hi, lo

    def values(self):
        return [Fraction(int(v), self.den) for v in self.big()]

    def demote(self):
        if self.tier == "i128" and fits64_kernel(*self.data):
            return RatCol("i64", self.data[1].view(np.int64).copy(), self.den)
        if self.tier == "big":
            return RatCol.from_big(self.data, self.den)
        return self

    def reduce(self):
        if self.den == 1:
            return self
        if self.tier == "i64":
            g = math.gcd(int(np.gcd.reduce(np.abs(self.data))), self.den) if len(self) else self.den
        else:
            g = self.den
            for v in self.big():
                g = math.gcd(g, int(v))
                if g == 1:
                    break
        if g <= 1:
            return self
        if self.tier == "i64":
            return RatCol("i64", self.data // g, self.den // g)
        return RatCol.from_big(np.array([int(v) // g for v in self.big()], dtype=object), self.den // g)

    def scale_int(self, s):
        if s == 1:
            return self
        if self.tier == "i64" and abs(s) <= M:
            out = np.empty_like(self.data)
            if scale_i64_kernel(self.data, np.int64(s), out) == 0:
                return RatCol("i64", out, self.den)
        if self.tier in ("i64", "i128") and abs(s) <= M:
            hi, lo = self.i128()
            ohi, olo = np.empty_like(hi), np.empty_like(lo)
            if scale_i128_kernel(hi, lo, np.int64(s), ohi, olo) == 0:
                return RatCol("i128", (ohi, olo), self.den).demote()
        return RatCol.from_big(self.big() * s, self.den, reduce=False)

    def _aligned(self, o):
        D = math.lcm(self.den, o.den)
        return self.scale_int(D // self.den), o.scale_int(D // o.den), D

    def _addsub(self, o, negate):
        a, b, D = self._aligned(o)
        if a.tier == "i64" and b.tier == "i64":
            out = np.empty_like(a.data)
            if add_i64_kernel(a.data, b.data, negate, out) == 0:
                return RatCol("i64", out, D)
        if a.tier != "big" and b.tier != "big":
            (ah, al), (bh, bl) = a.i128(), b.i128()
            oh, ol = np.empty_like(ah), np.empty_like(al)
            if add_i128_kernel(ah, al, bh, bl, negate, oh, ol) == 0:
                return RatCol("i128", (oh, ol), D).demote()
        res = a.big() - b.big() if negate else a.big() + b.big()
        return RatCol.from_big(res, D)

    def __add__(self, o):
        return self._addsub(o, False)

    def __sub__(self, o):
        return self._addsub(o, True)

    def __mul__(self, o):
        D = self.den * o.den
        if self.tier == "i64" and o.tier == "i64":
            hi, lo = np.empty(len(self), dtype=np.uint64), np.empty(len(self), dtype=np.uint64)
            mul_i64_i128_kernel(self.data, o.data, hi, lo)
            return RatCol("i128", (hi, lo), D).demote().reduce()
        return RatCol.from_big(self.big() * o.big(), D)

    def scale(self, q):
        q = Fraction(q)
        c = self.scale_int(q.numerator)
        return RatCol(c.tier, c.data, self.den * q.denominator).reduce()

    def negate_where(self, mask):
        if not mask.any():
            return self
        if self.tier == "i64":
            return RatCol("i64", np.where(mask, -self.data, self.data), self.den)
        b = self.big()
        return RatCol.from_big(np.where(mask, -b, b), self.den)

    def zero_where(self, mask):
        if self.tier == "i64":
            return RatCol("i64", np.where(mask, 0, self.data), self.den)
        b = self.big()
        return RatCol.from_big(np.where(mask, 0, b), self.den)

    def is_zero(self):
        if self.tier == "i64":
            return self.data == 0
        if self.tier == "i128":
            return (self.data[0] == 0) & (self.data[1] == 0)
        return self.data == 0

    def take(self, mask):
        if self.tier == "i128":
            return RatCol("i128", (self.data[0][mask], self.data[1][mask]), self.den).demote()
        return RatCol(self.tier, self.data[mask], self.den).demote()

    def greater(self, q):
        q = Fraction(q)
        t = (q.numerator * self.den) // q.denominator
        if self.tier == "i64" and abs(t) <= M:
            return self.data > t
        return np.array([int(v) > t for v in self.big()], dtype=np.bool_)

    def total(self):
        if self.tier == "i64":
            total, ovf, block_sums, bad = sum_kernel(self.data)
            if not ovf and not bad.any():
                return Fraction(int(total), self.den)
        return Fraction(sum(int(v) for v in self.big()), self.den)

    def mean(self):
        return self.total() / len(self)

    def var(self):
        n = len(self)
        if self.tier == "i64":
            s1, hi, lo, bad = moments_kernel(self.data)
            s1, s2 = int(s1), (int(hi) << 32) + int(lo)
            if bad.any():
                w_hi, w_lo, q2, q1, q0 = moments128_kernel(self.data, bad)
                s1 += to_int128(w_hi, w_lo)
                s2 += (int(q2) << 128) | (int(q1) << 64) | int(q0)
        else:
            vals = [int(v) for v in self.big()]
            s1, s2 = sum(vals), sum(v * v for v in vals)
        return Fraction(n * s2 - s1 * s1, n * (n - 1) * self.den**2)

    def group_moments(self, keys, groups):
        counts = np.bincount(keys, minlength=groups)
        if self.tier == "i64":
            s1, q_hi, q_lo, bad = group_moments_kernel(keys, self.data, groups)
            s1 = [int(v) for v in s1]
            s2 = [(int(h) << 64) | int(l) for h, l in zip(q_hi, q_lo)]
            for g in np.flatnonzero(bad):
                vals = [int(v) for v in self.data[keys == g]]
                s1[g], s2[g] = sum(vals), sum(v * v for v in vals)
        else:
            big = self.big()
            s1, s2 = [0] * groups, [0] * groups
            for k, v in zip(keys.tolist(), big):
                v = int(v)
                s1[k] += v
                s2[k] += v * v
        return counts, s1, s2

    def group_sum(self, keys, groups):
        _, s1, _ = self.group_moments(keys, groups)
        return [Fraction(s, self.den) for s in s1]

    def group_mean_var(self, keys, groups):
        counts, s1, s2 = self.group_moments(keys, groups)
        out = []
        for n, a, b in zip(counts.tolist(), s1, s2):
            mean = Fraction(a, n * self.den) if n else None
            var = Fraction(n * b - a * a, n * (n - 1) * self.den**2) if n > 1 else None
            out.append((mean, var))
        return out
```

```python {#core group kernel}
@njit
def group_moments_kernel(keys, nums, groups):
    s1 = np.zeros(groups, dtype=np.int64)
    q_hi = np.zeros(groups, dtype=np.uint64)
    q_lo = np.zeros(groups, dtype=np.uint64)
    bad = np.zeros(groups, dtype=np.bool_)
    for i in range(len(keys)):
        g = keys[i]
        if g < 0 or g >= groups:
            raise IndexError("group key out of range")
        v = nums[i]
        a, o1 = add_ovf(s1[g], v)
        s1[g] = a
        u, _ = magnitude(v)
        h, l = mul_u64(u, u)
        lo = q_lo[g] + l
        carry = np.uint64(1) if lo < l else np.uint64(0)
        hi = q_hi[g] + h + carry
        bad[g] |= o1 | (hi < q_hi[g])
        q_lo[g], q_hi[g] = lo, hi
    return s1, q_hi, q_lo, bad
```

The group kernel accumulates each group's sum in a checked 64-bit word, and its sum of squares in a 128-bit accumulator, built from `mul_u64` for each square and a carry into the high word. A group whose sum overflows 64 bits, or whose sum of squares overflows 128 bits, is recomputed exactly in Python.

## 12.2 Complex columns with per-column dispatch

A `CxCol` is either **one-part**, a rational column plus a boolean array saying which elements are imaginary, or **two-part**, a real column and an imaginary column. This follows Chapter 10's design, with Chapter 10's branching problem solved two ways:

1. **Per-column dispatch.** The kind is decided once per column, not per element. A two-part column always runs the plain $(a+bi)(c+di)$ formula with no tests.
2. **Branch-free masks** for one-part columns. The quarter-turn law becomes array arithmetic: each element's turn count is its imaginary bit, the turns of a product add, the new imaginary bit is the low bit of the sum, and a sign flip happens where the high bit is set. Rotation by $i^k$ is the same with a constant $k$.

Sums that mix real and imaginary elements widen to two parts. Every result is narrowed back to one part when each element has at most one nonzero part.

```python {#core cxcol}
class CxCol:
    __slots__ = ("kind", "a", "b")

    def __init__(self, kind, a, b):
        self.kind, self.a, self.b = kind, a, b

    @staticmethod
    def real(col):
        return CxCol("one", col, np.zeros(len(col), dtype=np.bool_))

    def __len__(self):
        return len(self.a)

    def widen(self):
        if self.kind == "two":
            return self
        return CxCol("two", self.a.zero_where(self.b), self.a.zero_where(~self.b))

    def narrow(self):
        if self.kind == "one":
            return self
        rz, iz = self.a.is_zero(), self.b.is_zero()
        if not (rz | iz).all():
            return self
        return CxCol("one", self.a + self.b, ~iz)

    def rotate(self, k):
        k %= 4
        if self.kind == "one":
            turns = self.b.astype(np.int8) + k
            return CxCol("one", self.a.negate_where((turns >> 1) & 1 == 1), (turns & 1) == 1)
        re, im = self.a, self.b
        neg = np.ones(len(self), dtype=np.bool_)
        if k == 0:
            return self
        if k == 1:
            return CxCol("two", im.negate_where(neg), re)
        if k == 2:
            return CxCol("two", re.negate_where(neg), im.negate_where(neg))
        return CxCol("two", im, re.negate_where(neg))

    def __add__(self, o):
        if self.kind == "one" and o.kind == "one" and np.array_equal(self.b, o.b):
            return CxCol("one", self.a + o.a, self.b)
        x, y = self.widen(), o.widen()
        return CxCol("two", x.a + y.a, x.b + y.b).narrow()

    def __sub__(self, o):
        return self + o.rotate(2)

    def __mul__(self, o):
        if self.kind == "one" and o.kind == "one":
            turns = self.b.astype(np.int8) + o.b.astype(np.int8)
            return CxCol("one", (self.a * o.a).negate_where((turns >> 1) & 1 == 1), (turns & 1) == 1)
        x, y = self.widen(), o.widen()
        return CxCol("two", x.a * y.a - x.b * y.b, x.a * y.b + x.b * y.a).narrow()

    def abs2(self):
        if self.kind == "one":
            return self.a * self.a
        return self.a * self.a + self.b * self.b

    def total(self):
        if self.kind == "one":
            return Cx(self.a.zero_where(self.b).total(), self.a.zero_where(~self.b).total())
        return Cx(self.a.total(), self.b.total())

    def values(self):
        if self.kind == "one":
            return [Cx(0, v) if t else Cx(v) for v, t in zip(self.a.values(), self.b.tolist())]
        return [Cx(r, i) for r, i in zip(self.a.values(), self.b.values())]
```

## 12.3 Scalars and the solver

`greater(q)` compares a column with a rational threshold exactly: for an integer numerator $p$ and denominator $d > 0$, $p/d > q \iff p > \lfloor q d \rfloor$, so the test is a single integer comparison per element. Reductions (`total`, `mean`, `var`, the group statistics) return exact rationals, which the benchmarks wrap as Chapter 10 tagged floating-slash scalars (`Cx`), so their word-or-big tier is known.

`exact_solve` tries the numeric–symbolic solver with early stopping. If floats cannot make progress, or the entries are outside the float-exact range, it tries Dixon on BLAS with early stopping. Failing that, it uses FLINT, which accepts any integer matrix. Each solver either returns the exact answer or declines, so the chain always returns the exact answer.

```python {#core solve}
def as_int64(A):
    try:
        out = np.array(A, dtype=np.int64)
    except OverflowError:
        return None
    if out.size and (out.min() < -M or out.max() > M):
        return None
    return out


def exact_solve(N, b):
    Ni, bi = as_int64(N), as_int64(b)
    if Ni is not None and bi is not None:
        for name, solver in (("numeric-symbolic", numeric_solve), ("Dixon", dixon_solve)):
            try:
                nums, d, _ = solver(Ni, bi, True)
                return nums, d, name
            except NotApplicable:
                continue
    n = len(b)
    rows = [[int(v) for v in row] for row in np.asarray(N, dtype=object)]
    X = flint.fmpz_mat(rows).solve(flint.fmpz_mat(n, 1, [int(v) for v in b]))
    fr = [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(n)]
    d = math.lcm(*[f.denominator for f in fr])
    return [f.numerator * (d // f.denominator) for f in fr], d, "FLINT"
```

## 12.4 Correctness tests

- **Rational columns at every tier.** Random columns with numerators of 20, 62, 100 and 140 bits, so that every tier and every promotion path is reached. $+$, $-$, $\times$, scaling by a rational, total, mean and variance must equal `Fraction` arithmetic exactly.
- **Promotion paths and boundaries.** A 64-bit addition that overflows must promote to 128 bits and demote again after subtracting back. A column containing $-2^{63}$ must start in the 128-bit tier, since negating it in 64 bits would wrap. The boundary $\pm(2^{63} - 1)$ must be 64-bit while $-2^{63}$ is not. Scaling into the conservative band between $2^{62}$ and $2^{63}$ must demote back to 64 bits. A group whose 64-bit sum overflows must take the exact fallback.
- **128-bit edges.** Values at $\pm(2^{127} - 1)$ must promote to `big` when they step past the edge, including to exactly $-2^{127}$, and negating or doubling them must stay exact.
- **Demotion.** A product that needs 128 bits must demote back to 64 bits after dividing out the factor that caused it, and a `big` column must reclassify when its values shrink. Shared-denominator reduction must leave the values unchanged.
- **Group statistics** at 64-bit and big tiers, including groups that overflow and force the per-group fallback, must equal `Fraction` group sums, means and variances.
- **Complex columns.** One-part (mixed real and imaginary), two-part and real columns at three magnitudes. Every pairwise $+$, $-$, $\times$, every rotation $i^k$, and `abs2` must equal Chapter 10's scalar `Cx` arithmetic. Products of one-part columns must stay one-part, and sums that cancel the imaginary parts must narrow back.
- **The solver chain** must pick numeric–symbolic for an ordinary system, Dixon for the ill-conditioned bidiagonal matrix of Chapter 7, and FLINT for entries above $2^{20}$. All three answers must equal FLINT's.

```python {#tests core}
def rand_col(rng, n, bits, den):
    return RatCol.from_big(np.array([rng.randint(-(2**bits), 2**bits) for _ in range(n)], dtype=object), den)


def test_ratcol_all_tiers():
    rng = random.Random(100)
    seen = set()
    for bits in (20, 62, 100, 140):
        a, b = rand_col(rng, 300, bits, 1000), rand_col(rng, 300, bits, 7)
        va, vb = a.values(), b.values()
        seen.update({a.tier, (a * b).tier, (a + b).tier})
        assert (a + b).values() == [x + y for x, y in zip(va, vb)]
        assert (a - b).values() == [x - y for x, y in zip(va, vb)]
        assert (a * b).values() == [x * y for x, y in zip(va, vb)]
        assert a.scale(Fraction(-3, 5)).values() == [x * Fraction(-3, 5) for x in va]
        mean = sum(va) / len(va)
        assert a.total() == sum(va) and a.mean() == mean
        assert a.var() == sum((x - mean) ** 2 for x in va) / (len(va) - 1)
    assert seen == {"i64", "i128", "big"}


def test_i128_edges():
    top = 2**127 - 1
    a = RatCol.from_big(np.array([-top, top, -top], dtype=object), 1)
    one = RatCol.decimals(np.array([1, -1, 0], dtype=np.int64), 1)
    zero = RatCol.from_big(np.array([0, 0, 0], dtype=object), 1)
    c = a - one
    assert c.tier == "big" and c.values() == [-top - 1, top + 1, -top]
    assert (zero - c).values() == [top + 1, -top - 1, top]
    assert (a + a).values() == [2 * v for v in a.values()]
    assert a.scale(2).values() == [2 * v for v in a.values()] and a.scale(-1).values() == [-v for v in a.values()]


def test_promotion_paths_and_boundaries():
    e = 2**62
    a = RatCol.decimals(np.array([e, -e, 5], dtype=np.int64), 1)
    s = a + a
    assert s.tier == "i128" and s.values() == [2 * e, -2 * e, 10]
    assert (s - a).tier == "i64" and (s - a).values() == a.values()
    m = RatCol.decimals(np.array([-(2**63), 5], dtype=np.int64), 1)
    assert m.tier == "i128"
    assert (RatCol.decimals(np.array([5, 5], dtype=np.int64), 1) - m).values() == [5 + 2**63, 0]
    assert m.negate_where(np.array([True, False])).values() == [2**63, 5]
    edge = RatCol.from_big(np.array([2**63 - 1, -(2**63 - 1)], dtype=object), 1)
    assert edge.tier == "i64"
    over = RatCol.from_big(np.array([2**63 - 1, -(2**63)], dtype=object), 1)
    assert over.tier == "i128" and over.demote().tier == "i128"
    assert RatCol.decimals(np.array([2**61, 5], dtype=np.int64), 7).scale(3).tier == "i64"
    keys = np.array([0, 0, 0, 1, 1], dtype=np.int64)
    g = RatCol.decimals(np.array([e, e, -5, 3, 2**40], dtype=np.int64), 1)
    sums = g.group_sum(keys, 2)
    stats = g.group_mean_var(keys, 2)
    assert sums == [2 * e - 5, 3 + 2**40]
    xs = [Fraction(v) for v in (e, e, -5)]
    m0 = sum(xs) / 3
    assert stats[0] == (m0, sum((x - m0) ** 2 for x in xs) / 2)


def test_demotion_and_reduction():
    K = 2**40 + 3
    x = RatCol.decimals(np.array([K, -2 * K, 3 * K], dtype=np.int64), 1000)
    y = x * x
    assert y.tier == "i128"
    back = y.scale(Fraction(1, K * K))
    assert back.tier == "i64" and back.values() == [v * v / (K * K) for v in x.values()]
    big = RatCol.from_big(np.array([2**130, 2**131, 0], dtype=object), 1)
    assert big.tier == "big"
    small = big.scale(Fraction(1, 2**129))
    assert small.tier == "i64" and small.values() == [2, 4, 0] and small.den == 1
    z = RatCol.decimals(np.array([10, 20, 30], dtype=np.int64), 100).reduce()
    assert z.den == 10 and z.values() == [Fraction(1, 10), Fraction(1, 5), Fraction(3, 10)]


def test_group_statistics():
    rng = random.Random(101)
    for bits in (20, 40, 100):
        col = rand_col(rng, 2000, bits, 1000)
        keys = np.array([rng.randrange(9) for _ in range(2000)], dtype=np.int64)
        vals = col.values()
        stats = col.group_mean_var(keys, 9)
        sums = col.group_sum(keys, 9)
        for g in range(9):
            xs = [v for v, k in zip(vals, keys.tolist()) if k == g]
            m = sum(xs) / len(xs)
            assert sums[g] == sum(xs)
            assert stats[g] == (m, sum((x - m) ** 2 for x in xs) / (len(xs) - 1))


def test_complex_columns():
    rng = random.Random(102)
    n = 150
    units = [Cx(1), I, Cx(-1), Cx(0, -1)]
    for bits in (20, 62, 100):
        one = CxCol("one", rand_col(rng, n, bits, 1000), np.array([rng.random() < 0.5 for _ in range(n)]))
        two = CxCol("two", rand_col(rng, n, bits, 10), rand_col(rng, n, bits, 3))
        real = CxCol.real(rand_col(rng, n, bits, 7))
        for X in (one, two, real):
            for Y in (one, two, real):
                vx, vy = X.values(), Y.values()
                assert (X + Y).values() == [a + b for a, b in zip(vx, vy)]
                assert (X - Y).values() == [a - b for a, b in zip(vx, vy)]
                assert (X * Y).values() == [a * b for a, b in zip(vx, vy)]
            for k in range(4):
                assert X.rotate(k).values() == [v * units[k] for v in X.values()]
            assert X.abs2().values() == [v.re * v.re + v.im * v.im for v in X.values()]
        assert (one * one).kind == "one" and (real * one).kind == "one"
        assert ((two + two.rotate(2)).kind) == "one"


def test_solver_chain():
    rng = np.random.default_rng(103)
    N = rng.integers(-10**6, 10**6, size=(40, 40))
    b = rng.integers(-10**6, 10**6, size=40)
    B = np.eye(5, dtype=np.int64) - 2**19 * np.eye(5, k=1, dtype=np.int64)
    Big = rng.integers(-(2**40), 2**40, size=(10, 10))
    for A, rhs, method in ((N, b, "numeric-symbolic"), (B, np.arange(1, 6, dtype=np.int64), "Dixon"),
                           (Big, np.ones(10, dtype=np.int64), "FLINT")):
        nums, d, used = exact_solve(A, rhs)
        assert used == method
        X = flint.fmpz_mat(A.tolist()).solve(flint.fmpz_mat(len(rhs), 1, [int(v) for v in rhs]))
        assert [Fraction(v, d) for v in nums] == [Fraction(int(X[i, 0].p), int(X[i, 0].q)) for i in range(len(rhs))]
```

```python {file=build/core.py}
import math
from fractions import Fraction

import flint
import numpy as np
from numba import njit

from q64 import M, add_ovf, mul_ovf
from columns import sum_kernel, moments_kernel
from wide import magnitude, mul_u64, neg128, add128, smul128, moments128_kernel, to_int128
from tagged_complex import Cx
from fastsolve import NotApplicable, numeric_solve, dixon_solve


<<core kernels>>


<<core conversions>>


<<core ratcol>>


<<core group kernel>>


<<core cxcol>>


<<core solve>>
```

```python {file=build/test_core.py}
import random
from fractions import Fraction

import flint
import numpy as np

from core import RatCol, CxCol, exact_solve
from tagged_complex import Cx, I


<<tests core>>
```

## 12.5 End-to-end benchmarks

Four tasks, each written three ways: with **NumPy** floats, the baseline, written the careful way a NumPy user would; with the **exact core**; and with **pure Python exact arithmetic**, using `Fraction` or Chapter 10's `Cx`, which is what an exact computation looks like without this work. That third column fills the gap noted earlier, a measured "versus Python" figure. Every exact core result is checked against the pure Python result, and NumPy's error is measured against the exact answer.

**E1, linear regression from measured data.** $n = 20{,}000$ rows, 8 features of two-place decimals up to $10^2$, plus an intercept. The response is $y = X\beta^* + \varepsilon$ with three-place coefficients and five-place noise, so every value is an exact decimal. The exact core forms the normal equations $X^\top X \beta = X^\top y$ from column products and totals, scales them to integers, and calls `exact_solve`. The scaled entries are far above $2^{20}$, so in practice the chain always falls through to FLINT for this $9 \times 9$ system. E1 therefore measures the exact column arithmetic plus a tiny FLINT solve, not the numeric–symbolic solver. Both exact versions compute only the upper triangle of $X^\top X$ and mirror it. NumPy uses `lstsq`, the numerically careful route, which works through an orthogonal factorisation rather than forming $X^\top X$. Pure Python forms the normal equations with `Fraction` and solves them by Gaussian elimination in `Fraction`s.

**E2, a table statistics pipeline.** $10^5$ sales rows: price (two places), quantity (integer), discount (three places), region (50 groups), an exchange rate (six places), a tax rate (four places), and a temperature reading near $10^6$ (three places). The pipeline computes revenue $=$ price $\times$ quantity $\times (1 - \text{discount})$. It converts revenue as revenue $\times$ rate $\times (1 + \text{tax})$, whose numerators exceed 64 bits and so exercise the 128-bit tier. It then takes per-region totals of the converted revenue, the per-region mean and variance of temperature, and the total revenue of rows with revenue above 1000. NumPy uses `bincount` for group sums and two-pass group variance. Pure Python loops over rows with `Fraction`s, accumulating each group's exact sum and sum of squares in one pass, the same one-pass method as the exact core. NumPy's filter `rev > 1000` is computed in floats, so a row whose revenue is within rounding of 1000 could be classified differently, which does not occur in this data.

**E3, a complex signal pipeline.** $10^5$ real samples (three places) in blocks of four. Each block gets a 4-point DFT, whose twiddle factors are $\pm 1, \pm i$, so it is exact. Then the total power in each frequency bin, $\sum |X_k|^2$, and a cross-spectrum total $\sum X_1 X_2$, a genuinely complex quantity mixing a two-part and a one-part bin. In the exact core, $X_0$ and $X_2$ stay one-part (real) columns, while $X_1$ and $X_3$ widen to two parts. NumPy uses `fft` on each block, and computes power as $\operatorname{Re}^2 + \operatorname{Im}^2$ rather than squaring `abs`. Pure Python uses Chapter 10's `Cx` with the same butterfly the core uses ($x_0 - x_2$, $x_1 - x_3$ and rotations), not a 16-multiplication naive DFT.

**E4, linear systems.** The Chapter 7 systems at $n = 500$, with a random right-hand side and with a planted simple answer, solved by `exact_solve` and by NumPy's `solve`. A pure Python `Fraction` elimination at $n = 500$ would take hours, so here the existing-exact baseline is FLINT instead.

```python {#e2e bench}
def frac_gauss(A, b):
    n = len(b)
    M_ = [row[:] + [b[i]] for i, row in enumerate(A)]
    for c in range(n):
        piv = next(r for r in range(c, n) if M_[r][c] != 0)
        M_[c], M_[piv] = M_[piv], M_[c]
        for r in range(n):
            if r != c and M_[r][c] != 0:
                f = M_[r][c] / M_[c][c]
                M_[r] = [x - f * y for x, y in zip(M_[r], M_[c])]
    return [M_[i][n] / M_[i][i] for i in range(n)]


def max_rel(approx, exact):
    worst = 0.0
    for a, e in zip(approx, exact):
        e = Fraction(e)
        if e:
            worst = max(worst, float(abs(Fraction(float(a)) - e) / abs(e)))
    return worst


def e1_data(n, p, seed=110):
    rng = np.random.default_rng(seed)
    X = rng.integers(-10**4, 10**4 + 1, size=(n, p), dtype=np.int64)
    beta = rng.integers(-5000, 5001, size=p, dtype=np.int64)
    noise = rng.integers(-10**5, 10**5 + 1, size=n, dtype=np.int64)
    y = X @ beta + noise
    return X, y


def e1_core(X, y):
    n, p = X.shape
    cols = [RatCol.decimals(X[:, j], 100) for j in range(p)] + [RatCol.decimals(np.ones(n, dtype=np.int64), 1)]
    ycol = RatCol.decimals(y, 10**5)
    k = len(cols)
    G = [[None] * k for _ in range(k)]
    for i in range(k):
        for j in range(i, k):
            G[i][j] = G[j][i] = (cols[i] * cols[j]).total()
    r = [(c * ycol).total() for c in cols]
    L = math.lcm(*[v.denominator for row in G for v in row], *[v.denominator for v in r])
    Gi = [[int(v * L) for v in row] for row in G]
    ri = [int(v * L) for v in r]
    nums, d, method = exact_solve(Gi, ri)
    return [Fraction(v, d) for v in nums], method


def e1_python(X, y):
    n, p = X.shape
    cols = [[Fraction(int(v), 100) for v in X[:, j]] for j in range(p)] + [[Fraction(1)] * n]
    ys = [Fraction(int(v), 10**5) for v in y]
    k = len(cols)
    G = [[None] * k for _ in range(k)]
    for i in range(k):
        for j in range(i, k):
            G[i][j] = G[j][i] = sum((a * b for a, b in zip(cols[i], cols[j])), Fraction(0))
    r = [sum((a * b for a, b in zip(c, ys)), Fraction(0)) for c in cols]
    return frac_gauss(G, r)


def e1_numpy(X, y):
    A = np.column_stack([X / 100, np.ones(len(X))])
    return np.linalg.lstsq(A, y / 10**5, rcond=None)[0]


def e2_data(n, seed=111):
    rng = np.random.default_rng(seed)
    return dict(price=rng.integers(1, 10**6 + 1, size=n, dtype=np.int64),
                qty=rng.integers(1, 1001, size=n, dtype=np.int64),
                disc=rng.integers(0, 501, size=n, dtype=np.int64),
                region=rng.integers(0, 50, size=n, dtype=np.int64),
                fx=rng.integers(900000, 1100001, size=n, dtype=np.int64),
                tax=rng.integers(500, 2501, size=n, dtype=np.int64),
                temp=10**9 + rng.integers(-5000, 5001, size=n, dtype=np.int64))


def e2_core(D, groups=50):
    n = len(D["price"])
    one = RatCol.decimals(np.ones(n, dtype=np.int64), 1)
    price, qty = RatCol.decimals(D["price"], 100), RatCol.decimals(D["qty"], 1)
    disc, fx, tax = RatCol.decimals(D["disc"], 1000), RatCol.decimals(D["fx"], 10**6), RatCol.decimals(D["tax"], 10**4)
    rev = price * qty * (one - disc)
    conv = rev * fx * (one + tax)
    region_totals = conv.group_sum(D["region"], groups)
    temp_stats = RatCol.decimals(D["temp"], 1000).group_mean_var(D["region"], groups)
    big_rev = rev.take(rev.greater(1000)).total()
    out = region_totals + [m for m, _ in temp_stats] + [v for _, v in temp_stats] + [big_rev]
    return out, f"revenue tier {rev.tier}, converted tier {conv.tier}"


def e2_python(D, groups=50):
    tot = [Fraction(0)] * groups
    s1, s2, cnt = [Fraction(0)] * groups, [Fraction(0)] * groups, [0] * groups
    big_rev = Fraction(0)
    for pr, q, dc, g, f, tx, tp in zip(*(D[k].tolist() for k in ("price", "qty", "disc", "region", "fx", "tax", "temp"))):
        rev = Fraction(pr, 100) * q * (1 - Fraction(dc, 1000))
        tot[g] += rev * Fraction(f, 10**6) * (1 + Fraction(tx, 10**4))
        x = Fraction(tp, 1000)
        s1[g] += x
        s2[g] += x * x
        cnt[g] += 1
        if rev > 1000:
            big_rev += rev
    means = [a / n for a, n in zip(s1, cnt)]
    vars_ = [(n * b - a * a) / (n * (n - 1)) for a, b, n in zip(s1, s2, cnt)]
    return tot + means + vars_ + [big_rev]


def e2_numpy(D, groups=50):
    g = D["region"]
    rev = D["price"] / 100 * D["qty"] * (1 - D["disc"] / 1000)
    conv = rev * (D["fx"] / 10**6) * (1 + D["tax"] / 10**4)
    totals = np.bincount(g, weights=conv, minlength=groups)
    t = D["temp"] / 1000
    cnt = np.bincount(g, minlength=groups)
    mean = np.bincount(g, weights=t, minlength=groups) / cnt
    dev = t - mean[g]
    var = np.bincount(g, weights=dev * dev, minlength=groups) / (cnt - 1)
    big_rev = rev[rev > 1000].sum()
    return list(totals) + list(mean) + list(var) + [big_rev]


def e3_core(x):
    xs = [CxCol.real(RatCol.decimals(np.ascontiguousarray(x[j::4]), 1000)) for j in range(4)]
    x0, x1, x2, x3 = xs
    A, B = x0 - x2, x1 - x3
    X = [x0 + x1 + x2 + x3, A + B.rotate(3), x0 - x1 + x2 - x3, A + B.rotate(1)]
    power = [Xk.abs2().total() for Xk in X]
    cross = (X[1] * X[2]).total()
    return power + [cross.re, cross.im], f"bin kinds {[Xk.kind for Xk in X]}"


def e3_python(x):
    vals = [Fraction(int(v), 1000) for v in x]
    power = [Fraction(0)] * 4
    cross = Cx(0)
    minus_i = Cx(0, -1)
    for b in range(0, len(vals), 4):
        x0, x1, x2, x3 = (Cx(v) for v in vals[b:b + 4])
        A, B = x0 - x2, x1 - x3
        X = [x0 + x1 + x2 + x3, A + B * minus_i, x0 - x1 + x2 - x3, A + B * I]
        for k in range(4):
            power[k] += X[k].re * X[k].re + X[k].im * X[k].im
        cross = cross + X[1] * X[2]
    return power + [cross.re, cross.im]


def e3_numpy(x):
    F = np.fft.fft((x / 1000).reshape(-1, 4), axis=1)
    power = list((F.real ** 2 + F.imag ** 2).sum(axis=0))
    cross = (F[:, 1] * F[:, 2]).sum()
    return power + [cross.real, cross.imag]


def bench_e2e(report, sizes):
    X, y = e1_data(sizes["e1"], 8)
    beta, method = e1_core(X, y)
    ref = e1_python(X, y)
    res = measure({"NumPy float64": lambda: e1_numpy(X, y), "exact core": lambda: e1_core(X, y),
                   "pure Python exact": lambda: e1_python(X, y)}, gc_off=False)
    report.add_measured("E1 regression", sizes["e1"], res, "NumPy float64", **{
        "NumPy float64": dict(error=max_rel(e1_numpy(X, y), ref), note="lstsq"),
        "exact core": dict(error=verify(beta == ref, "E1", "0 (matches pure Python)"), note=f"solver: {method}"),
        "pure Python exact": dict(error="0 (exact)")})

    D = e2_data(sizes["e2"])
    out, tiers = e2_core(D)
    ref = e2_python(D)
    res = measure({"NumPy float64": lambda: e2_numpy(D), "exact core": lambda: e2_core(D),
                   "pure Python exact": lambda: e2_python(D)}, gc_off=False)
    report.add_measured("E2 table pipeline", sizes["e2"], res, "NumPy float64", **{
        "NumPy float64": dict(error=max_rel(e2_numpy(D), ref), note="worst over 151 outputs"),
        "exact core": dict(error=verify(out == ref, "E2", "0 (matches pure Python)"), note=tiers),
        "pure Python exact": dict(error="0 (exact)")})

    x = np.random.default_rng(112).integers(-10**6, 10**6 + 1, size=sizes["e3"], dtype=np.int64)
    out, kinds = e3_core(x)
    ref = e3_python(x)
    res = measure({"NumPy float64": lambda: e3_numpy(x), "exact core": lambda: e3_core(x),
                   "pure Python exact": lambda: e3_python(x)}, gc_off=False)
    report.add_measured("E3 complex pipeline", sizes["e3"], res, "NumPy float64", **{
        "NumPy float64": dict(error=max_rel(e3_numpy(x), ref), note="fft"),
        "exact core": dict(error=verify(out == ref, "E3", "0 (matches pure Python)"), note=kinds),
        "pure Python exact": dict(error="0 (exact)", note="Cx scalars")})

    n = sizes["e4"]
    N, _, _ = decimal_matrix(n, n, seed=12)
    for label, b in (("E4 solve, random rhs", np.ascontiguousarray(decimal_matrix(n, 1, seed=13)[0][:, 0])),
                     ("E4 solve, planted answer", N @ np.random.default_rng(17).integers(-1000, 1001, size=n))):
        Nf, bf = N.astype(np.float64), b.astype(np.float64)
        X_ = flint.fmpz_mat(N.tolist()).solve(flint.fmpz_mat(n, 1, b.tolist()))
        want = [Fraction(int(X_[i, 0].p), int(X_[i, 0].q)) for i in range(n)]
        nums, d, method = exact_solve(N, b)
        res = measure({"NumPy float64": lambda: np.linalg.solve(Nf, bf), "exact core": lambda: exact_solve(N, b),
                       "FLINT exact": lambda: flint.fmpz_mat(N.tolist()).solve(flint.fmpz_mat(n, 1, b.tolist()))},
                      gc_off=False)
        report.add_measured(label, n, res, "NumPy float64", **{
            "NumPy float64": dict(error=max_rel(np.linalg.solve(Nf, bf), want)),
            "exact core": dict(error=verify([Fraction(v, d) for v in nums] == want, label, "0 (matches FLINT)"),
                               note=f"solver: {method}"),
            "FLINT exact": dict(error="0 (exact)", note="existing exact baseline")})


def main():
    print("correctness tests")
    import test_core
    for name in sorted(n for n in dir(test_core) if n.startswith("test_")):
        t = time.perf_counter()
        getattr(test_core, name)()
        print(f"  pass  {name}  ({time.perf_counter() - t:.1f} s)", flush=True)
    quick = "--quick" in sys.argv
    sizes = dict(e1=2000, e2=5000, e3=4000, e4=60) if quick else dict(e1=20000, e2=10**5, e3=10**5, e4=500)
    meta = metadata()
    report = Report()
    bench_e2e(report, sizes)
    out = Path(__file__).resolve().parent.parent / "results"
    out.mkdir(exist_ok=True)
    suffix = "-quick" if quick else ""
    header = "\n".join(f"- **{k}**: {v}" for k, v in meta.items())
    (out / f"integrated{suffix}.md").write_text(f"# Integrated exact core vs NumPy\n\n{header}\n\n{report.markdown()}\n")
    (out / f"integrated{suffix}.json").write_text(json.dumps({"metadata": meta, "rows": report.raw()}, indent=1))
    print(report.markdown())


if __name__ == "__main__":
    main()
```

NumPy's errors are relative errors per output, against the pure Python exact result, with the worst over all nonzero outputs reported. Outputs whose exact value is zero are excluded, since a relative error is undefined there. All timings in this chapter leave the garbage collector on. For E2 that is 151 numbers: 50 converted totals, 50 means, 50 variances and one filtered total.

```python {file=build/run_integrated.py}
import math

from run_benchmarks import *
from run_benchmarks import metadata, verify

from core import RatCol, CxCol, exact_solve
from tagged_complex import Cx, I


<<e2e bench>>
```

