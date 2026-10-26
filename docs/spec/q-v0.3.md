# Q: Gaussian-Unit Rational Formats, Specification v0.3

Oct 26, 2026 · Andrew Emerson

This spec defines a family of data types, not a language feature: fixed-width binary formats for exact rationals on the real or the imaginary axis, specified the way a standards body specifies a floating-point format. It revises [v0.2](q-v0.2.md). The formats and the vector form are unchanged; v0.3 states precisely what the family is for, describes its contribution as a combination of known techniques, and replaces the evidence with measurements against compiled exact libraries, including workloads where Q does not win.

Changes from v0.2:

- **Scope of the claim.** The vector form speeds up large arrays of exact values that share a denominator or a common scale. It is not a faster general rational arithmetic, and the spec now says where it loses.
- **Contribution.** The prior-art section lists what each component builds on; what the spec adds is the combination, and the small format details not found elsewhere.
- **Evidence.** GMP and FLINT are now measured from C, not only through Python; int64 arithmetic in C is measured unchecked, checked, and widening to 128 bits; new workloads have unrelated denominators, growing denominators, and values too wide for the fast tiers. Every table names its data and denominator structure.
- **Reproduction.** The benchmark code is published in this repository.

## Purpose and scope

The formats are for computation that must be exact. A conforming implementation never rounds, so these formats compete with exact libraries, not with floating point.

**What the vector form is for.** Its speed comes from one property of the data: the values in an array share a denominator, or a common scale such as a fixed number of decimal places, and their numerators fit machine words. Then exact arithmetic on the array is integer arithmetic on dense arrays, with overflow checked and widened rather than assumed away, and Q runs close to hand-written fixed-width integer code. Typical data of this kind are decimal measurements and money, statistics and regression on bounded decimal inputs, exact reference pipelines in signal processing, deterministic simulation state, and matrices of decimals. Where values have unrelated denominators, where denominators grow through long chains of operations, or where results outgrow 128 bits, the advantage disappears, and general rational libraries are as fast or faster (see Evidence).

In scope: the unit Q2, the use of an unmodified unsigned integer as a numerator and a denominator, the family of widths, canonical encodings, arithmetic semantics, and the vector form for arrays.

Out of scope: rounding, syntax and language bindings, transcendental functions, the internal representation of arbitrary-precision values, and I/O formats.

## Notation

For a positive integer $n$, $\operatorname{bits}(n)$ is its length in binary, $\lfloor \log_2 n \rfloor + 1$, and $\operatorname{bits}(0) = 0$. A U$w$ is an unsigned integer of $w$ bits, numbered $w-1$ (most significant) down to 0. In a name Q$n$, $n$ counts the bits that carry meaning.

## Q2: the Gaussian unit

**Q2** is two bits, a sign bit and an imaginary bit. Read together as a 2-bit number they are the **turn** $t$, the number of quarter turns: $t = 0$ is $1$, $1$ is $i$, $2$ is $-1$, $3$ is $-i$. The four units form a cyclic group, so multiplying units is adding turns modulo 4: $i \cdot i$ is $01 + 01 = 10$, which is $-1$. Negation adds 2 (flips the sign bit); conjugation negates the turn; the reciprocal of $i^t$ is $i^{-t}$. The bits also read independently: the sign bit alone is $-1$, the imaginary bit alone is $i$, both are $-i$.

A Q2 is never zero. Stored alone it occupies a byte, the smallest unit a machine addresses, in the byte's top two bits with the other six clear, the way a boolean occupies a byte. Arrays of Q2 may pack four to a byte.

## Q72

A **Q72** is a byte and a U64, written $(g, u)$. The byte's top two bits are a Q2, the turn $t$; its low six bits are the **slash** $s$. The U64 is the standard unsigned 64-bit integer, unmodified; the slash tells how to read it:

$$
p = \left\lfloor \frac{u}{2^{63-s}} \right\rfloor, \qquad q = \left(u \bmod 2^{63-s}\right) + 1, \qquad x = i^t \cdot \frac{p}{q}.
$$

| Byte bits | Field | Meaning |
| --- | --- | --- |
| 7 | sign | the Q2 |
| 6 | imaginary | |
| 5–0 | slash $s$ | the numerator is the top $s+1$ bits of the U64 |

The numerator $p$ is the top $s+1$ bits of $u$. The remaining $63-s$ bits store the **denominator minus one**, so a field of zeros means $q = 1$ and no pattern has a zero denominator. At slash 63 there is no field: the whole word is the numerator, and the value is an integer.

Examples: $1/2$ has $p = 1$, $q = 2$, slash 0, and $u = 2^{63} + 1$. The integer 10 has $p = 1010_2$; slash 3 puts it in the top four bits with a zero field, so $u = 10 \cdot 2^{60}$ (slash 63 with $u = 10$ is a non-canonical encoding of the same value). $-3i/4$ has turn 3, $p = 11_2$, slash 1, and $u = 3 \cdot 2^{62} + 3$.

**Every pattern is a value.** All $2^{72}$ bit patterns decode; there are no reserved codes, infinities or not-a-number values.

**Zero.** When the numerator bits are all zero, $p = 0$ and the value is zero, whatever the rest of the pattern holds. Zero comes from the U64, never from the Q2. Zero has no direction: $0 \cdot i = 0$, and $-0$ is $0$.

**Range.** A nonzero magnitude $p/q$ in lowest terms is representable exactly when

$$
\operatorname{bits}(p) + \operatorname{bits}(q - 1) \le 64.
$$

The largest magnitude is the integer $2^{64} - 1$; the smallest positive magnitude is $1/2^{63}$.

### Canonical form

The canonical encoding of a nonzero value has $p$ and $q$ coprime and $s = \operatorname{bits}(p) - 1$, so the numerator's leading 1 is bit 63 of the U64. The canonical encoding of zero has every bit clear. Every value has exactly one canonical encoding, operations return canonical results, and equality of values is equality of canonical encodings. Non-canonical patterns are valid inputs and decode to their values.

## Arithmetic semantics

Every operation is exact, and results are canonical.

- **Multiplication** adds turns and multiplies magnitudes. Division multiplies by the reciprocal $i^{-t} q/p$.
- **Division by zero** is an error. There is no infinite or undefined result to return.
- **Addition and subtraction** are defined between values on the same axis: two reals, or two imaginaries. A real plus an imaginary is a complex number with both parts, which is two values (see Complex values). Zero is on every axis.
- **Comparison.** Real values are totally ordered. Imaginary values are compared for equality only.
- **Conjugation** negates the turn and **negation** adds 2 to it; neither changes the magnitude.

### Exactness

A result that does not fit is promoted to the next width, and from the top of the family to arbitrary precision. A conforming implementation never rounds, and has no rounding mode or inexact flag. An approximation is a new value that a program asks for explicitly, through a function outside this spec.

### Complex values

A complex value with both parts nonzero is a pair, real part first: $(a, b)$ means $a + b$ where $a$ is real and $b$ imaginary. Its operations follow from the parts: $(a, b)(c, d) = (ac + bd, ad + bc)$, where every product of two imaginaries is real and lands in the first part.

## The family

The layout scales with the word. A Q$n$ is a unit-and-slash tag and a U$w$; the tag holds the Q2 in its top two bits and a slash of $\log_2 w$ bits in its low bits.

| Format | Tag | Word | Largest integer | Smallest positive |
| --- | --- | --- | --- | --- |
| Q2 | — | — | the four units $\pm 1$, $\pm i$ | — |
| Q24 | 8 bits: Q2, 2 reserved, 4-bit slash | U16 | $2^{16}-1$ | $2^{-15}$ |
| Q40 | 8 bits: Q2, 1 reserved, 5-bit slash | U32 | $2^{32}-1$ | $2^{-31}$ |
| Q72 | 8 bits: Q2, 6-bit slash | U64 | $2^{64}-1$ | $2^{-63}$ |
| Q144 | 16 bits: Q2, 7 reserved, 7-bit slash | U128 | $2^{128}-1$ | $2^{-127}$ |

Reserved bits are zero in canonical form and ignored on decode. Beyond Q144, values are arbitrary-precision exact rationals with the same turn, whose representation this spec does not fix. Promotion is by value: a result is stored in the narrowest format that holds it.

## The vector form

An array of Q values is stored for computation in the **vector form**: one shared denominator $d \ge 1$, one axis $a$ for the whole vector (0 real, 1 imaginary), and per element a **two's complement** integer $x_i$. Element $i$ is

$$
i^{a} \cdot \frac{x_i}{d}.
$$

The sign of each element is part of its integer, as in quote notation, where $-1$ is $\ldots 1111$; only the axis, one bit, is stored for the vector. Elements are 64-bit integers by default. When any element needs more, the whole vector widens to 128-bit integers, then to arbitrary precision; it narrows again when every element fits.

**Canonical form.** The denominator and the elements share no common factor: $\gcd(d, x_1, \ldots, x_n) = 1$. An all-zero vector has $d = 1$ and lies on both axes.

**Conversion.** An array of Q72 values on one axis converts to the vector form over the least common multiple of their denominators, the sign of each element coming from its turn. An array with both real and imaginary elements is a **complex array**: a real vector and an imaginary vector. A vector converts back by reducing each element.

**Operations.** Equality, ordering and results are those of the values; the layout changes only speed.

- Two vectors are aligned to a common denominator, after which addition and subtraction are integer addition and subtraction, with no gcd. A real vector plus an imaginary vector, both nonzero, is a complex array.
- Multiplication is integer multiplication over $d \cdot d'$; the axes add, so two imaginary vectors give a negated real vector.
- Sums, sums of squares and group sums are integer sums.
- **Overflow is detected, never wrapped.** An implementation proves a result fits (from the operands' bit lengths, or from the sign bits after a two's complement add), and otherwise recomputes in the wider tier.
- After arithmetic, the vector is reduced to canonical form by the common factor $g$ of $d$ and all elements. The division is exact, so it may be done as quote notation does it: shift out the power of two in $g$, then multiply by the inverse of its odd part modulo $2^{64}$. This is exact for negative elements too.

This is the arithmetic of integers over a shared denominator, with overflow caught and widened rather than assumed away, and with results kept in lowest terms.

**Matrix product.** A matrix in the vector form is an integer matrix over one denominator, so its product is an integer product over $d_A d_B$. When $\max|A| \cdot \max|B| \cdot n < 2^{53}$, a single float64 matrix product is exact. Otherwise each matrix splits into signed limbs of $b$ bits with $n \cdot 2^{2b} \le 2^{53}$, each pair of limbs is one exact float product, and the limbs recombine in 64-bit, 128-bit or arbitrary-precision integers, as the bound allows.

**Why this layout.** Three layouts were measured on identical work:
- a turn byte and an unsigned magnitude per element (9 bytes), the v0.1 layout;
- turns packed four to a byte (8.25 bytes);
- two's complement with the axis per vector (8 bytes).

The two's complement layout was fastest or tied at every operation (see Evidence).

Arrays may also be stored element by element as Q72 values, which is the natural form for interchange and for arrays whose elements have unrelated denominators. Q72 v0.1 measured it, and it is not recommended for computation.

## Determinism and platform requirements

Results are bit-identical on every platform: arithmetic is exact and canonical form is unique. Implementations need only 64-bit integer arithmetic and a 64×64→128-bit multiply.

## Prior art

The magnitude of a Q value is a **floating-slash rational**, the format Matula and Kornerup defined and analysed ("Finite Precision Rational Arithmetic: Slash Number Systems", *IEEE Trans. Computers*, 1985; *Finite Precision Number Systems and Arithmetic*, Cambridge, 2010). Their formats store a numerator and denominator whose boundary moves, round by best rational approximation, and are arranged in multi-tier hierarchies; the Q family is such a hierarchy. Their Integrated Rational Arithmetic Unit (1981) computes all four operations on floating-slash operands with one Euclidean-algorithm unit.

Two other representations of the rationals were considered and not adopted, because the vector form depends on explicit integer numerators:

- **LCF** (Kornerup and Matula, *J. Universal Computer Science*, 1995) encodes a rational as the bit string of its continued fraction. Comparing two encodings as bit strings compares the values, and adjacent strings are neighbouring fractions. Arithmetic is bit-serial, not word-parallel.
- **Quote notation** (Hehner and Horspool, *SIAM J. Computing*, 1979) writes a rational as an eventually repeating 2-adic expansion, which generalises two's complement: $-1$ is $\ldots 1111$. Addition, subtraction and multiplication use the integer algorithms, and division needs no gcd. Its length depends on the denominator: $1/947$ needs 947 bits.

Two of quote notation's ideas are used here without changing the format. The signed vector layout keeps each element's sign in two's complement. And exact division by a known factor, the step that reduces a vector after arithmetic, multiplies by the 2-adic inverse of the factor's odd part instead of dividing; this is Jebelean's exact division (1993), which GMP uses for the same purpose. The exact solver's p-adic lifting (Dixon, 1982) is the same arithmetic again.

Kahan argued that pure-imaginary values deserve their own type, so that arithmetic mixing real and imaginary values does not invent a zero real part; C99's `_Imaginary` type (Annex G) follows him. The Q family puts the axis of every value into the type.

For exact computation, the standards are GMP (`mpq`, `mpz`), FLINT (`fmpq`, `fmpq_mat`, `fmpz_mat`) and Python's `fractions` and `decimal` (libmpdec). Exact matrix products on floating-point hardware by error-free splitting follow Ozaki, Ogita, Rump and Oishi (2008).

### Contribution

Each component has prior art:

| Component | Prior art |
| --- | --- |
| a numerator and denominator in one word, with a movable boundary | floating-slash rationals (Matula and Kornerup) |
| a tier hierarchy of widths | their multi-tier hierarchies |
| a type that knows a value is imaginary | Kahan; C99 `_Imaginary` |
| integers over a shared denominator | fixed-point and scaled-integer arithmetic |
| native-word arithmetic with checked promotion to wider integers | small-integer fast paths in bignum libraries (FLINT's `fmpz` stores small values inline) |
| exact division by a known factor via a 2-adic inverse | Jebelean's exact division; GMP |
| exact matrix products on floating-point hardware under proven bounds | Ozaki, Ogita, Rump and Oishi |

What Q contributes is the combination: a compact exact-rational scalar format and a dense shared-denominator vector form, held together by one model of exactness, canonical form, layout and promotion, so that arrays of exact rationals get the performance techniques long available to integer, fixed-point and limb-based arithmetic. Two details of the format did not appear in the literature reviewed: the quarter-turn reading of the sign and imaginary bits on a floating-slash rational, and the denominator stored minus one, which makes every bit pattern a value. Whether the combination is novel would need a fuller review; its value rests on the measurements below.

## Evidence

### How the measurements were made

Q72 and the vector form were implemented as a prototype in Python with compiled (Numba) kernels. The vector form uses the two's complement layout. Its tiers are 64-bit integers, then 128-bit, then Python integers; the last tier is a stand-in, so results that need it say little about a designed arbitrary-precision tier. Only Q72 was built.

Contenders, all exact except where noted:
- **From C** (gcc `-O3 -march=native`, inputs built before timing, outputs preallocated and reused, median of repetitions after a warm-up):
  - GMP `mpq_t` and FLINT `fmpq` rationals;
  - GMP `mpz_t` and FLINT `fmpz` integers over the shared denominator (FLINT stores small integers inline, which makes it the closest existing analogue of the vector form);
  - int64 integers over the shared denominator in three versions: unchecked; with every overflow checked; and checked with the whole operation redone in 128-bit integers on overflow, which is the vector form's own strategy, compiled.
- **From Python:** GMP, FLINT, `fractions` and `decimal` (80 digits, every rounding trapped) as object arrays, GMP integers over the shared denominator, and NumPy int64 over the shared denominator.
- **NumPy float64**, which rounds, appears only as context.

Every result was checked against an independent exact computation, every element for elementwise operations, and 29 correctness tests ran first. Independent reviews of the code and of the fairness of each comparison came before the measured runs, and their findings were fixed. The machine was one core of an Intel Xeon; BLAS and FLINT were single-threaded.

**Workloads.** Every table below names its data. The columns differ in three ways that decide the result:

| Workload | Denominators | Values | Promotion |
| --- | --- | --- | --- |
| A. columns of decimals | one per column: $10^3$ or $10^6$ | below $10^4$ (numerators below $10^7$ or $10^{10}$) | none; 128-bit products for 6 places |
| U1. unrelated fractions | per element, 1 to 1000; common denominator about 1,400 bits | numerators below $10^6$ | the whole vector is in the widest tier |
| U2. mixed small denominators | per element, 1 to 12; common denominator 15 bits | numerators below $10^6$ | none |
| U3. a growth chain, ten steps of $x \leftarrow x b + c$ | grows $10^3$-fold each step | 3-place decimals | to the widest tier by the end |
| U4. a few wide values | one per column | none, one, 1% or all elements near $10^{16}$ | 128-bit, or the widest tier for $(ab)^2$ |

### Where the vector form wins: arrays on one scale

$10^6$ decimals, compiled contenders, time relative to the signed vector form (above 1 means the vector form is faster):

| Operation | GMP `mpq_t` | FLINT `fmpz` numerators | GMP `mpz_t` numerators | int64, checked | int64, checked, widening |
| --- | --- | --- | --- | --- | --- |
| sum | 224 | 5.0 | 23 | 1.2 | 1.2 |
| $a \cdot b + c$, 3 places | 34 | 2.8 | 2.9 | 0.20 | 0.26 |
| $a \cdot b + c$, 6 places | 24 | 3.6 | 1.8 | overflow | 0.31 |
| group-by sum, 1000 groups | 78 | 2.1 | 9.1 | 0.56 | 0.56 |
| variance | 333 | 15 | 28 | 2.6 | 2.7 |
| variance, values near $10^6$ | 336 | 17 | 19 | 2.5 | 2.4 |

The tools a C programmer would reach for compare as follows:
- **GMP rationals** (`mpq_t`) are 24× to 340× slower, and calling them from C instead of Python saves at most a factor of 2.3. Their cost is the gcd in every operation, not the interpreter.
- **FLINT's integers over the shared denominator**, the closest existing technique, are 2× to 17× slower.
- **Checked int64 in C** is the real comparison. The vector form matches it on sums, is 2.5× faster on variance, and takes 1.8× to 5× its time on group sums and $a \cdot b + c$. A dedicated compiled implementation should close much of that gap; the variance lead comes from proving the bound once per vector instead of checking every addition.
- **Unchecked int64** was wrong on the 6-place products.

From Python, the vector form was 48× to 570× faster than GMP rationals, 24× to 390× faster than `decimal`, and 210× to 2,700× faster than `fractions`. Python's `decimal` is the fastest of the Python object types for decimal data.

### Where it loses

Relative to GMP `mpq_t` from C, with the conversion into the vector form (the common denominator and rescaling) included:

| Workload | Vector form |
| --- | --- |
| U1 unrelated denominators: sum | 5.2× slower |
| U1 unrelated denominators: $a \cdot b + c$ | 78× slower |
| U2 mixed small denominators: sum | 1.9× slower |
| U2 mixed small denominators: $a \cdot b + c$ | 3.5× slower |
| U3 growth chain | 1.3× slower |
| U4 $(ab)^2$ with one, 1% or all elements wide | 1.6× to 2.2× slower (and 5.6× to 16× slower than FLINT's integers) |

- **U1, unrelated denominators.** The common denominator of a column of unrelated fractions has over a thousand bits, so every element lives in the widest tier. Converting into the vector form takes several times longer than GMP takes for the whole operation.
- **U2, mixed small denominators.** Once in the vector form, the work is fast (the sum takes 12 µs against GMP's 9 ms), but converting into it dominates. Data that arrives on a common scale avoids that cost.
- **U3, growth chain.** Chains whose denominators grow lose the shared-denominator advantage step by step.
- **U4, one wide element.** One wide element widens the whole vector. To 128 bits that costs about 2× and the vector form still leads: it is 24× faster than GMP and 1.7× faster than FLINT's integers, which promote element by element. Past 128 bits the prototype falls to Python integers and loses. A compiled arbitrary-precision tier would narrow that gap, but the whole-vector widening rule itself costs more than per-element promotion whenever only a few values are wide.

### Matrix product and solve

Decimal matrices in the vector form multiply as integer matrices over one denominator. The exact float-BLAS product was 3.7× to 5.7× faster than FLINT `fmpz_mat` at 3 places, 2.5× to 2.7× at 6 and 9 places (where it splits into limbs), and 4.6× to 7.9× faster than `fmpq_mat`. A plain float64 product of the numerators is 2.4× to 3.4× faster again at 3 places, where it happens to be exact, and was wrong at 6 and 9 places; nothing in it checks the bound that the exact product checks.

The exact solver chain from earlier work is not specific to Q. It took 0.21× FLINT's time with a planted integer answer ($n = 500$, every entry fitting one Q72), and 0.84× to 2.5× with a random right-hand side, where no entry fits.

### Pipelines

A regression on 20,000 rows, a 100,000-row table pipeline with currency conversion and grouping, and a 100,000-point complex DFT stage, against the same pipelines written as loops over GMP `mpq` scalars from Python: the vector form was 200×, 11× and 98× faster. The table pipeline's converted amounts need 128-bit integers.

### The layout of the vector form

Fused kernels doing identical work on $10^7$ elements, time relative to a turn byte per element:

| Operation | packed turns | two's complement |
| --- | --- | --- |
| convert from int64 numerators | 1.01 | 1.04 |
| sum | 0.67 | **0.61** |
| $a \cdot b + c$ | 1.00 | **0.86** |
| group-by sum | 1.34 | **0.66** |
| sum and sum of squares | 0.77 | **0.62** |
| bytes per element | 8.25 | **8** |

Exact division by a common factor was 2.7× faster by multiplying with the 2-adic inverse than with the divide instruction.

### Precision

Rounding is not part of the format. It was measured only to describe how finely Q72 covers the real line.

| To match Q72's… | a float needs a significand of | so an IEEE-style format of |
| --- | --- | --- |
| typical precision (median relative error $6.4 \times 10^{-20}$) | 63 bits | 71 bits |
| worst-case precision ($1.2 \times 10^{-10}$, beside the integer 1) | 33 bits | 41 bits |

The format sizes assume the 8-bit exponent needed to cover Q72's range. Typical precision is about $6 \times 10^{-20}$ at every magnitude, against float64's $4 \times 10^{-17}$. The worst case is at magnitude 1 and improves toward either end of the range; at the bottom the gap is to zero, as with float underflow.

### Exactness

How often common values are exactly representable (20,000 samples each):

| Values | binary64 | decimal64 | Q72 |
| --- | --- | --- | --- |
| money, 2 places, below $10^9$ | 3.9% | 100% | 100% |
| measurements, 3 or 6 places, below $10^6$ | under 1% | 100% | 100% |
| ratios $a/b$ of integers up to 1000 | 2.5% | 5.1% | 100% |
| means of 7 three-place measurements | 0.12% | 14% | 100% |
| products of two 3-place measurements | 0.05% | 16% | 10% |
| binary64 values | 100% | 0% | 0% |

Products of large decimals often need more than 64 bits and are promoted.

### The cost of exactness

For context only, against NumPy float64, which rounds, on $10^6$ values:
- the sum takes 0.79× NumPy's time;
- $a \cdot b + c$ takes 2.1×;
- the variance takes 0.23×;
- a 1000×1000 product of 6-place decimals takes 6.3×.

### Summary

- **The niche.** For large arrays on one scale, such as decimal columns, the vector form is exact at speeds near native fixed-width integer code:
  - it matches checked int64 in C on sums and is 2.5× faster on variance;
  - it is within 5× of checked int64 on elementwise products;
  - it is 2× to 17× faster than FLINT's integers over the same denominator;
  - it is 24× to 340× faster than GMP rationals.
- **Outside the niche it is not faster.** It loses with unrelated or mixed denominators once conversion counts, with growing denominators, and past 128 bits.
- **The matrix product** is 2.5× to 7.9× faster than FLINT.
- **Layout and division.** Two's complement with one axis per vector is the fastest layout, and quote notation's exact division is 2.7× faster than dividing.
- **Precision and exactness.** Q72 is exact for decimals and ratios of small integers, and its precision matches a 71-bit float typically and a 41-bit float in the worst case.

### Reproducing

The benchmark code is in [`bench/`](../../bench/) as literate sources that generate the code. It needs the v0.1 benchmark ([`bench/exact-bench`](../../bench/exact-bench/)) and this one ([`bench/q-bench`](../../bench/q-bench/)), Python with NumPy, Numba, python-flint and gmpy2, and GMP and FLINT development headers for the compiled baselines. The results of the runs reported here are in `bench/q-bench/results/`.

## Open decisions

- **Reserved bits.** Q24, Q40 and Q144 leave bits of the tag unused. This spec makes them zero and ignored. They could instead carry a format tag, so that a stream of mixed widths describes itself.
- **Errors in arrays.** Division by zero is an error for a scalar. For an array operation it is open whether the whole operation fails, or the failing elements are reported while the rest complete.
- **Ordering of imaginary values.** Imaginaries compare for equality only. Ordering them by coefficient ($2i > i$) would help sorting, at the cost of an order that does not extend to complex values.
- **Arbitrary precision.** Above Q144 this spec fixes the semantics but not the representation; an interchange encoding is open.
- **Approximation.** A separate, explicit best-approximation function, returning the nearest Q72 to a given rational, is natural to standardise alongside this spec. The Evidence section measures what it would give. It is not part of the format.

## Directions for research

These are explorations that might improve implementations or add features without changing the formats.

- **Ordered keys from LCF.** Measured; see Research findings below.
- **Deferred reduction for unrelated denominators.** Measured as part of the quote-notation research; see Research findings below. Fixed-width numerator and denominator pairs with the gcd deferred to the end are the open lead: an array form for the data the vector form handles worst.
- **Mixed-axis vectors.** The vector form keeps one axis per vector, so an array mixing real and imaginary elements is two vectors. A bitmap of imaginary bits, one per element, would let one vector hold both. Packing turns four to a byte was measured and did not pay: it was slower than two's complement at every operation except one tie. Whether a bitmap pays for mixed data is open.
- **A plane variant.** $(a + bi)/c$ with three integers sharing one word covers the whole complex plane and is closed under all four operations (the Gaussian rationals, $\mathbb{Q}(i)$). Two slashes are needed, so each part gets about 20 bits of a 64-bit word; it becomes plausible at 144 bits. The vector form already holds complex arrays as a real and an imaginary vector over one denominator, which is this idea for arrays. The comparison would be against FLINT's exact Gaussian and number-field types.

## Research findings

These measurements explore two of the directions above without changing the formats. The plane variant is evaluated in a separate [research note](../research/plane-variant.md): it ties the axis family on speed and array memory, and its one advantage is compact storage of single complex quotients. The code is in [`bench/q-research`](../../bench/q-research/), and every result was checked against an exact computation.

### LCF: order-preserving keys

**Question.** Can every Q value have a fixed-width key whose order as a plain unsigned integer (or as bytes, compared most significant first) is the numeric order, so that systems that only compare bytes, such as database indexes, radix and external sorts, and storage engines, can order Q values without understanding the format?

**Construction.** Following Kornerup and Matula's LCF, the key of a magnitude $p/q$ is its continued fraction $[a_0; a_1, \ldots, a_n]$:
- each quotient is written in an order-preserving, self-delimiting code;
- the codes at odd positions are complemented;
- the expansion ends with an infinite quotient, written as all ones (complemented at odd positions);
- a sign bit leads, and a negative value takes the complement of its magnitude's key.

LCF's own quotient code (a unary length prefix, $2\,\mathrm{bits}(a) - 1$ bits) needs up to 131 bits for Q72, just too long for 128. A delta code (the bit length in LCF's code, then the remaining bits) needs at most 115 bits. That is a proven upper bound, from continuant growth over all Q72 magnitudes; the longest found by search is 101 bits. So every Q72 value has a 128-bit key, a sign bit and 127 bits of code, with equal values getting equal keys and order preserved exactly. Brute-force tests and fuzzing of extreme values confirm this, including zero, negatives and $2^{64} - 1$.

**Cost.** Encoding takes about 87 ns per value (compiled). Sorting $10^6$ mixed Q72 values:

| Method | Time |
| --- | --- |
| keys, then NumPy's sort | 206 ms |
| keys, then a merge sort | 261 ms |
| the same merge sort, comparing values exactly by 128-bit cross-multiplication | 373 ms |
| float64 sort with exact fixing of ties, correctly rounded floats given | 121 ms |
| the same, including the conversion to correctly rounded floats | 257 ms |
| GMP `mpq` objects, Python's sort | 1.17 s |

**Finding.** Keys do not make sorting much faster: with the same sort they gain 1.4× over exact comparison. Their value is interoperability. Any system that compares bytes can index and order Q values correctly, at 16 bytes per key against 9 for the value. Within a vector the question does not arise, because values over one denominator order as integers.

A standard order-key encoding derived from Q72 is a candidate addition to the spec, alongside the format rather than in it.

### Quote notation: 2-adic arithmetic and deferred reduction

Two of quote notation's ideas are already in v0.2: two's complement signs in the vector form, and exact division by a 2-adic inverse (2.7× faster than dividing). This research asked whether its representation of rationals could give a fast path where the vector form fails, for elementwise work on values with unrelated denominators.

**Method.** Elementwise chains $x \leftarrow x b + c$ of 1 to 4 steps on $10^5$ elements, compiled, in three versions:
1. GMP `mpq_t`, reduced every step;
2. a 128-bit numerator and denominator per element, with the gcd deferred to the end;
3. quote notation's representation: a residue modulo $2^{128}$ with a power-of-two exponent, so that adding and multiplying are single integer operations, then rational reconstruction at the end.

Versions 2 and 3 carry exact bounds on each element's size. When a bound fails, the element is redone with GMP, so no result can be wrong.

| Steps | unrelated denominators 1–1000: 2-adic | deferred pairs | decimals, 3 places: 2-adic | deferred pairs |
| --- | --- | --- | --- | --- |
| 1 | 0.92× | **0.26×** | 0.61× | **0.20×** |
| 2 | 0.75× | **0.21×** | 0.57× | **0.20×** |
| 3 | 1.08× (97% fall back) | **0.20×** | 1.02× (all fall back) | **0.23×** |
| 4 | 1.02× (all fall back) | 0.70× (57% fall back) | 1.03× (all fall back) | 1.03× (99% fall back) |

Time is relative to GMP.

**Finding.** The 2-adic representation does not pay. Rational reconstruction costs more than the gcd it replaces, and a residue modulo $2^{128}$ can only be reconstructed while numerator and denominator together stay under about 127 bits, half what two separate 128-bit words hold. So it falls back sooner.

The plain alternative, unreduced fixed-width pairs with one gcd at the end, was 4–5× faster than GMP for up to three steps. That is the case where the vector form is weakest: it was 78× slower than GMP on unrelated denominators. An array of fixed-width numerator–denominator pairs with deferred reduction, beside the vector form, is the lead this research leaves. Quote notation's p-adic arithmetic remains in use where it fits best, in the exact solver's Dixon lifting.
