# Overflow survey: replacing the mark with exact growing values in Control

Oct 30, 2026 · Research pass, read-only on the code. Nothing here is adopted. It surveys the ways to stop Control's
rationals failing past 64 bits, ranks them under [`../../RULES.md`](../../RULES.md), and names the structure each one
adds. Read: `v0.4.md` §17.9–17.10, `nim/src/arith.nim`, `nim/src/core.nim`, paper v0.6 §1.2 and §3.1 (the column's
deferred reduction). Measurements were made in a scratch Python script (`fractions.Fraction`), not in the repo's code.

## 0. Where Control stands

- A cell is `Ratio(n, e: int64)`, lowest terms, `e >= 1`. The mark is `(0, 0)`.
- `arith.nim` checks every `times` and `plus` with `__builtin_*_overflow` (and treats `-2^63` as overflow, as 08 does).
  An overflow makes the operation `return false`. **Those check sites are exactly where a promotion would go.**
- Control's arithmetic needs only five integer operations: add/subtract, multiply, compare, **gcd**, and **exact division**
  (`divExact`: the divisor is always known to divide). It never needs a quotient with a remainder. This shapes the
  big-integer design (§1.4).

## 1. Growing (arbitrary-precision) integers

### 1.1 Representation

| Choice | Standard options | For Control |
| --- | --- | --- |
| Limb size | 32-bit limbs with 64-bit products (portable; CPython uses 30-bit digits) or 64-bit limbs with a 128-bit product (GMP, FLINT; needs a `mulhi`, in C `unsigned __int128`, in Nim by `{.emit.}` or a 4×32-bit split) | **64-bit limbs.** With 32-bit limbs, a 90-bit value takes 3 limbs; with 64-bit limbs it takes 2. The one primitive needed is a 64×64→128 multiply plus add-with-carry, the same compiler builtins `core.nim` already imports. |
| Sign | sign–magnitude (GMP: the sign is in the size field) or two's complement | **Sign–magnitude.** The paper (§1.1) already assumes negation is O(1), and `arith.nim` already splits sign from magnitude (`Operand.neg`). |
| Normalization | no leading zero limbs; zero has length 0 | Required. It makes `length` a size test, and it makes `= 0`, `= 1` and `b == d` cheap (lengths differ ⇒ not equal, O(1)). |
| Layout | heap block `{len, cap, limbs[]}`, or inline for small values (§2) | A rational's numerator and denominator live in **one** heap block, `{sign, len_n, len_d, limbs_n..., limbs_d...}`. That is **one stored structure per rational** (a limb array), which RULES §4 allows. Separate blocks for the numerator and the denominator would be two. |

### 1.2 Schoolbook operations (Knuth, TAOCP 2, §4.3.1)

| Operation | Algorithm | Time (m, n limbs) |
| --- | --- | --- |
| add / subtract | carry/borrow loop; signs decide add or subtract of magnitudes, with a compare first | O(m + n) |
| multiply | schoolbook row-by-row, 64×64→128 inner step | O(mn) |
| compare | lengths first, then limbs from the top | O(1) when the lengths differ, else O(n) |
| divide (general) | Knuth's Algorithm D (normalize by a shift, estimate each quotient limb from the top two limbs, correct at most twice) | O((m − n + 1) n) |
| **exact divide** (the only division Control needs) | Jebelean / Montgomery-style: multiply by the inverse of the divisor's low limb mod 2^64, from the low end; no normalization, no correction step (GMP `mpz_divexact`) | O((m − n + 1) n), with a smaller constant and simpler code than Algorithm D |
| divide by one limb | a single pass, one 128/64 step per limb | O(m) |

### 1.3 gcd on big integers

| Method | What it is | Cost | Notes |
| --- | --- | --- | --- |
| Binary (Stein), multi-limb | the current gcd, with multi-limb shifts and subtracts | O(n²) bit steps, about n·w iterations of O(n) limb work | Simplest extension of what exists. Slow when the sizes are very different, which is Henrici's `gcd(t, g)`: t is big and g is small. |
| Euclid with big division | `u mod v` repeatedly | O(n²) | Needs Algorithm D, not just exact division. |
| **Lehmer** | run Euclid on the leading 64 bits to get a 2×2 cofactor matrix, then apply it to the full numbers once per about 30 quotient steps | O(n²) with roughly a w-fold smaller constant | The standard method for multi-limb gcds (Knuth §4.5.2; GMP below its HGCD threshold of hundreds of limbs). |
| Half-gcd (HGCD) | recursive, subquadratic | O(M(n) log n) | Pays only above thousands of bits. Out of scope. |
| **Mixed sizes** (special case) | when one operand fits one limb: one big-mod-small pass, then the existing 64-bit gcd | O(n) | **Named special case.** Condition: one operand is 1 limb. That is common here: g = gcd(b, d) is small and t is big. It is the earlier m3 survey's "one division first" item, made necessary. |

### 1.4 Temporaries and locals

A multiply or add needs scratch limbs for t = a·(d/g) + c·(b/g) and for the products. **One scratch arena per operation**
(one temporary), sized from the operands' lengths before the operation starts (for the result, at most m + n + 1 limbs per
integer), holds all the intermediates. Lehmer and exact division each need one local: a copy of the operands being
reduced. That fits RULES §4 if each intermediate is carved from the arena, not allocated separately.

## 2. Hybrid: small fast path, promote on overflow

| System | Small form | Big form | Promotion | Demotion |
| --- | --- | --- | --- | --- |
| CPython ≥ 3.12 `int` | "compact": one 30-bit digit, with a fast path | digit array | automatic | yes (canonical, by size) |
| Common Lisp fixnum/bignum | tagged immediate (62–61 bits) | heap bignum | automatic | yes: a bignum that fits is always returned as a fixnum |
| FLINT `fmpz` | inline `slong`, \|x\| ≤ 2^62 − 1; the second-highest bit tags a pointer | pointer to `mpz` | `_fmpz_promote` | `_fmpz_demote_val`; `_fmpz_is_canonical` checks that small ⇔ fits |
| GMP `mpq` | none (always `mpz` limbs, inline only in the sense of a small heap block) | — | — | — |
| Julia | `Rational{Int}` **throws** `OverflowError` (checked ops), the same as Control today; `Rational{BigInt}` is always big. There is no automatic promotion. | | | |

**For Control.** The current cell already has a free tag: `e >= 1` is a valid denominator, `e = 0` is the mark.
- **Tag.** Let `e <= 0` (or a reserved value) mean "n is a pointer to the rational's limb block". The mark test `d == 0`,
  already made on every operand read, becomes the **big test**. That is the same number of comparisons, and small values
  keep the full ±(2^63 − 1) range.
- **Per-rational promotion.** Numerator and denominator are big together, in one block. This is simpler than
  per-integer tagging and keeps one stored structure per rational. The mixed case (big numerator, small denominator) is
  common, but it costs only a 1-limb denominator inside the block.
- **Promotion.** At every `return false` in `arith.nim`, redo the **whole operation** on the big path from the
  operands. They are immutable, so this is exact and simple. Steps before the overflow are wasted; that is acceptable
  because the overflow is rare per step. The alternative, resuming mid-operation, adds a promoted copy of each partial
  result to every case. The case tests (`a == d`, `b == d`, etc.) on big values are compares, O(1) when the lengths
  differ.
- **Demotion (canonical form).** After reduction, a result that fits 64 bits (both parts within ±(2^63 − 1)) is stored
  small. The rule is "small ⇔ fits". It makes value equality a representation test (a small value never equals a big
  one), keeps Packed's hashing well-defined, and keeps RULES Optimization §3 ("match the control") decidable. It costs
  one length check per big result.
- **Cost.** On values that fit, nothing beyond the checks that exist now. On a big value: one allocation, plus
  multi-limb arithmetic. Memory: 16 B per cell as now, plus about 24–48 B per big rational (header + 2–4 limbs at the
  current sizes).
- **Combines with** everything below. This is how every production system ships big integers.

## 3. Wider fixed types (128-bit)

- **What it is.** `int128` numerators and denominators. In Nim this means `{.emit.}` of C's `__int128`, or a hand-written
  two-word type, which is already most of a 2-limb big integer.
- **Cost.** Every cell is 32 B, even the small ones. Multiply is about 3–4 instructions. Binary gcd takes about twice
  the iterations. A 128/128 division is a libgcc call (`__divti3`), tens of cycles.
- **Why it only delays.** It fixes this workload's single product at n = 64. Measured exact results reach 89 bits and
  intermediates about 120. But growth is linear in chain depth (§5: about +29 bits per further product), so sop/pos and
  larger n cross 127 bits again. General (non-smooth) denominators reach about 1,000 bits at n = 64. The overflow check
  stays, so the problem is not solved. v0.4 already "considered and withdrew" a 128-bit word.
- **Structure added.** None (fixed data). It is a partial measure only. A two-limb inline fast path inside §2 gets the
  same benefit without fixing the width.

## 4. Rational-specific techniques that limit growth or cost

| Technique | What it is | Exact? | Cost / effect | Structure | When it is the right special case |
| --- | --- | --- | --- | --- | --- |
| Lowest terms, gcd-first (Henrici) | already in place; GMP's manual gives the same rationale (several small gcds early are cheaper than one big gcd late) | yes | keeps every stored value minimal; no technique can store a result smaller than its reduced size | none | always (the baseline) |
| **Common denominators per row of A / column of B** ("cleared" multiply; FLINT `fmpq_mat_mul_cleared`) | r_i = lcm of row i's denominators, c_j = lcm of column j's; A′ = r_i·A, B′ = c_j·B are integer matrices; C_ij = (A′B′)_ij / (r_i c_j), reduced **once** per cell | yes | inner loop: n integer multiply-adds, **no gcd**; one gcd per cell at the end, plus O(n²) gcds to form the lcms. Turns about 2n² · n gcds into n² + O(n²). Entries grow by the lcm's size, so they still need big integers. | **temporary**: B′ (n² integers), plus one local for A′'s row (the existing row buffer). The denominator vectors r, c are a fixed handful per row/column, but they are n numbers, so they are a temporary vector. **This is more than one temporary; it needs discussion under RULES §4.** | Condition: denominators share factors, so the lcms stay small. **Ideal here**: every denominator is 7-smooth; measured row lcm ≤ 33 bits at n = 64. Bad for unrelated denominators: the lcm approaches the product of the denominators, about 15·n bits. |
| Common denominator per matrix (FLINT `get_fmpz_mat_matwise`) | one D for all entries (the paper's §3.1 column, made whole-matrix) | yes | the simplest form; add and sub become integer add-and-reduce-once; the lcm is larger than per-row lcms | stored (one D) if kept between operations, otherwise temporary | the same condition, more strongly; for 7-smooth inputs the matrix lcm is ≤ 112 bits whatever n is |
| **Delayed reduction in a dot product** (accumulate over a running lcm, reduce once) | s = Σ a_t c_t / (b_t d_t) held as an unreduced numerator over lcm(b_t d_t), with one gcd at the end | yes | saves Henrici's h = gcd(t, g) per term; still one gcd per term to grow the lcm; intermediates are not reduced. Without the lcm (raw product of denominators) it blows up as about 30·n bits. | none beyond the arithmetic's scratch | a weaker form of the cleared multiply; useful when there is no room for B′ |
| Fraction-free (Bareiss) elimination | division-free integer elimination with exact divisions; bounds growth to the Hadamard bound | yes | for determinants and solves, not for products or sums | none | not applicable to mul/add; note it for later operations |
| **Multi-modular + CRT** | clear denominators, compute A′B′ mod k word-size primes (k ≈ bits / 62), CRT back; the denominators r_i c_j are known, so **no rational reconstruction is needed** | yes, given a proven bound \|C′\| ≤ n·max\|A′\|·max\|B′\| | k·n³ word multiply-mods + O(n²k²) CRT; wins when entries are many limbs (FLINT's `fmpz_mat_mul_multi_mod`). At 2 limbs (k ≈ 2) it does not beat direct schoolbook. | one temporary residue matrix, reused per prime | large n with multi-limb entries; not for the current sizes |
| Rational reconstruction (Wang; Monagan's maximal-quotient variant) | recover a/b from x mod M by a half-extended Euclid | exact only with a proven size bound; early termination without a bound is heuristic and must be checked | needs a big gcd per entry | as multi-modular | when denominators are unknown (solves, inverses), not for products whose denominators are known |

## 5. Scale of the problem (measured)

Exact `Fraction` products C = AB on §17.9's draw (bits of the reduced result):

| Inputs | n | numerator bits, median / max | denominator bits, median / max | cells > 63 bits (incl. running sums) |
| --- | --- | --- | --- | --- |
| 7-smooth, unpooled | 16 / 32 / 64 | 28/47 · 35/61 · 40/60 | 20/36 · 24/43 · 28/45 | 0 |
| 7-smooth, distinct pool (ρ = 1) | 32 | 50 / 70 | 35 / 51 | 1–2% |
| 7-smooth, distinct pool (ρ = 1) | 64 | 71 / 88 | 50 / 67 | 89–93% (two seeds; matches the 92–100% reported) |
| 7-smooth, chained, n = 32 | AB, ABE, ABEG | max 61 → 89 → 118 | max 43 → 63 → 87 | 0 → 81% → 100% |
| uniform 1..32767 denominators | 16 / 32 / 64 | 331/362 · 590/655 · 1032/1199 | about the same as the numerator | all |

**Why.** A term a_t c_t / (b_t d_t) has at most 2B bits per part (B = 15). The reduced sum's denominator divides
L = lcm_t(b_t d_t), and its numerator is at most about log₂ n + 2B + bits(L).
- In general, L approaches the product of the distinct denominators, so it **grows linearly in n**, at about 16–19 bits
  per term measured.
- For 7-smooth inputs ≤ 32767, the exponents are capped (2^14, 3^9, 5^6, 7^5), so L ≤ 2^28·3^18·5^12·7^10 ≈ 2^112.
  That is **bounded independent of n**, with only the log n term growing. So one product here stays at about 2 limbs.
- Each further product in a chain (sop, pos) adds about 2B bits.

## 6. Ranking for Control

1. **Baseline (required): sign–magnitude big integers with 64-bit limbs, one heap block per rational, schoolbook
   add/sub/mul, exact division, multi-limb binary gcd.** Nothing else stores results that are inherently 70–1,200 bits.
2. **Standard first optimization: the hybrid small/big cell (§2).** Inline int64 pair when it fits, the tag in `e`,
   promotion at the existing overflow checks, canonical demotion. This keeps today's speed on small values.
3. Named special cases, measured one at a time: **mixed-size gcd** (one operand 1 limb); **division by one limb**;
   **Lehmer** once gcds on ≥ 2 limbs dominate.
4. **Cleared (row/column common-denominator) multiply**, under its condition (small lcms; true for 7-smooth inputs). It
   needs a RULES §4 discussion: B′ plus the r and c vectors.
5. Multi-modular / rational reconstruction: not at these sizes. 128-bit: not a solution.

## Sources

- D. E. Knuth, *TAOCP* Vol. 2, 3rd ed.: §4.3.1 (multiple-precision arithmetic, Algorithm D), §4.5.1 (fractions,
  Henrici's gcd-first rules), §4.5.2 (binary gcd, Lehmer).
- R. Brent, P. Zimmermann, *Modern Computer Arithmetic* (2010), ch. 1: limbs, exact division, gcd. https://arxiv.org/abs/1004.4710
- T. Jebelean, "An algorithm for exact division", *J. Symbolic Computation* 15 (1993).
- GMP manual, Rational Internals: https://gmplib.org/manual/Rational-Internals.html ; Subquadratic GCD:
  https://gmplib.org/manual/Subquadratic-GCD.html
- FLINT `fmpq_mat` (mul, mul_cleared, mul_direct, get_fmpz_mat_rowwise/colwise/matwise): https://flintlib.org/doc/fmpq_mat/index.html
- FLINT `fmpz` (inline small values, promote/demote, canonical form): https://flintlib.org/doc/_sources/fmpz.rst.txt
- CPython `longintrepr.h` (30-bit digits, compact ints): https://github.com/python/cpython/blob/101ddba62d91705149c73b2aad6aad3fe305d58f/Include/longintrepr.h
- Julia `base/rational.jl` (checked arithmetic, `OverflowError`): https://github.com/JuliaLang/julia/blob/7f0ee122d70af8835bfd4a69875f3202550f60df/base/rational.jl
- M. Monagan, "Maximal quotient rational reconstruction" (ISSAC 2004): https://cecm-www.dcr.sfu.ca/personal/monaganm/papers/MQIRR.pdf
- P. S. Wang, "A p-adic algorithm for univariate partial fractions", SYMSAC 1981 (rational reconstruction).
- E. H. Bareiss, "Sylvester's identity and multistep integer-preserving Gaussian elimination", *Math. Comp.* 22 (1968).
