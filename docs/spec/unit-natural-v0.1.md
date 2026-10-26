# Unit–Natural Number System Specification v0.1

Oct 26, 2026 · Andrew Emerson

This is an alternative design for the language's number system, not a revision of the [Exact Number System Specification v0.1](number-system-v0.1.md). That spec packs a sign bit, an imaginary bit and a 7-bit slash into one 128-bit word. This one builds numbers from two small types that each mean something on their own, and pairs them. Both specs are kept so the designs can be weighed against each other.

## Purpose and scope

This spec defines what a number is, how it is stored, how arithmetic on it behaves, and how columns of numbers are laid out. Every value is exact unless the program explicitly asks for rounding.

In scope: the Natural, the Unit, the pair they form, special values, arithmetic semantics, rounding on request, columns and complex columns.

Out of scope for this version: the representation of values too large for one pair (see Values larger than one pair, which specifies their behaviour so programs stay exact), syntax, the type checker, transcendental functions, algebraic fields beyond the Gaussian rationals, and I/O formats. Linear algebra uses the solver chain of the first spec unchanged.

## Design principles

The principles of the first spec carry over, in the same priority order: exact by default; deterministic everywhere; smallest representation that fits; one type per column; decline rather than guess. One is added:

6. **Composition over packing.** A number is built from small types that each mean something alone, rather than from fields packed into a word. A type used alone means exactly what it means inside a pair, and a pair's behaviour follows from its parts rather than from special cases.

## The Natural

A **Natural** is an unsigned 64-bit integer, $0 \le N \le 2^{64}-1$. All 64 bits are digits; there are no header or flag bits. On its own it is the language's natural-number type with traditional arithmetic: its zero is the ordinary zero, and dividing by it is an error. Its operations are native on every target (add, subtract and compare in one instruction; 64×64→128-bit multiply in one or two).

## The Unit

A **Unit** is one byte: a 2-bit **turn** $t$ in the high bits and a 6-bit **slash** $s$ in the low bits.

| Bits | Field | Meaning |
| --- | --- | --- |
| 7–6 | turn $t$ | the complex unit $i^t$: 0 is $1$, 1 is $i$, 2 is $-1$, 3 is $-i$ |
| 5–0 | slash $s$ | where the slash falls in a paired Natural: the numerator is its top $s+1$ digits (see The pair); 63 means no denominator |

Standing alone, a Unit is a pair whose numerator and denominator default to 1. With slash 63 the slash sits after the last digit, so there is no denominator and the value is $i^t\cdot\infty$; any other slash gives $i^t$. So a Unit is one of eight values: the four complex units and the four directed infinities. Its most common use is as a sign, but it is the natural representation of the complex units, and infinity falls out of it. A Unit alone is never zero: zero is a Natural.

**Unit algebra.** Multiplication adds turns modulo 4 and is infinite if either factor is. Negation adds 2 to the turn and conjugation negates it. The reciprocal of a unit is a unit; the reciprocal of an infinity is a zero, which needs a Natural. Addition is not defined on Units alone: $1 + i$ is not a Unit. Slashes 0 to 62 all mean magnitude 1 for a Unit alone, so the canonical byte uses slash 0, and there are exactly eight distinct Units.

## The pair

A **pair** is a Unit and a Natural. The slash counts numerator digits: for $0 \le s \le 62$ and $N \ne 0$,

$$
p = \left\lfloor N / 2^{63-s} \right\rfloor, \qquad q = N \bmod 2^{63-s}, \qquad x = i^{t}\,\frac{p}{q},
$$

so the numerator is the top $s+1$ bits and the denominator the remaining $63-s$. All 64 slash values are used: 63 splits, and slash 63, the slash after the last digit.

| Natural | Slash 0–62 | Slash 63 |
| --- | --- | --- |
| $N = 0$ | zero, carrying the Unit's direction | $0/0$: **undefined** |
| $N \ne 0$ | $i^t \cdot p/q$ | $i^t\cdot\infty$ |

**What fits.** A value $p/q$ in lowest terms fits one pair exactly when

$$
\operatorname{bits}(p) \le 63 \quad\text{and}\quad \operatorname{bits}(p) + \operatorname{bits}(q) \le 64 .
$$

The largest finite value is the integer $2^{63}-1 \approx 9.2\times10^{18}$, the same as the largest positive 64-bit signed integer, and the smallest positive value is $1/(2^{63}-1) \approx 1.1\times10^{-19}$.

**Canonical form.** Each value has exactly one canonical word:

| Value | Unit | Natural |
| --- | --- | --- |
| finite $i^t\,p/q$, $\gcd(p,q)=1$ | turn $t$, slash $\operatorname{bits}(p)-1$ | $p\cdot 2^{64-\operatorname{bits}(p)} + q$ (the numerator's leading 1 in bit 63) |
| zero in direction $i^t$ | turn $t$, slash 0 | 0 |
| infinity in direction $i^t$ | turn $t$, slash 63 | $2^{63}$ |
| undefined | turn 0, slash 63 | 0 |

A decoder rejects every other word, except that the turn bits of undefined carry no meaning and are ignored.

**Axis.** A pair lies on one axis: real for even turns, imaginary for odd turns. A general complex value is two pairs (see Complex columns).

## Arithmetic semantics

Pair arithmetic follows from the Unit. Results are exact and canonical; results that do not fit one pair are promoted (Values larger than one pair) and demoted again when they fit.

**Products and quotients** add turns and multiply magnitudes. A zero keeps the direction the arithmetic gives it, so

$$
\frac{1}{i^u\cdot 0} = i^{-u}\cdot\infty, \qquad \frac{1}{i^u\cdot\infty} = i^{-u}\cdot 0, \qquad \frac{x}{i^u\cdot 0} = i^{t-u}\cdot\infty \text{ for nonzero finite } x = i^t\,p/q .
$$

Division by zero needs no special rule: it moves the slash to 63. Nothing is guessed, and the reciprocal is its own inverse even at infinity: $1/(1/(-\infty)) = -\infty$.

| Operation | Result |
| --- | --- |
| $0\cdot\infty$, $0/0$, $\infty/\infty$ | undefined |
| $\infty_t + \infty_u$ | $\infty_t$ if $t = u$, otherwise undefined |
| $\infty_t + x$, $x$ finite | $\infty_t$ |
| $0 + x$, $x$ nonzero | $x$ |
| $0_t + 0_u$ | IEEE 754's rule on four directions: $0_t$ if $t = u$; otherwise the positive zero of a shared axis ($+0$ or $+i\cdot0$); otherwise $+0$ |
| $x - x$ (exact cancellation) | the positive zero on $x$'s axis |
| $x + y$, nonzero, same axis | signed magnitudes add along the axis |
| $x + y$, nonzero, different axes | a complex value of two pairs |
| $-x$, $\bar x$ | turn $+2$; turn negated |
| anything with undefined | undefined |

**Equality and order.** All zeros compare equal; their direction matters only for what they produce in products and quotients. Everything else compares by turn and exact magnitude. Real values, including $\pm\infty$, are totally ordered; imaginary and undefined values are not ordered. Hashing agrees with equality, including with ordinary integers and fractions.

**Floats** entering the language convert to their exact rational value, but almost no binary64 value fits one pair: 0.1 as a double is a 108-bit fraction (Evidence). Decimal text should therefore be parsed directly to rationals, where "0.1" is simply $1/10$. Converting back is an explicit, correctly rounded `to_float(x)`.

## Rounding on request

Rounding happens only when asked for: `round(x)` returns the value that fits one pair and is nearest to $x$, with ties to the smaller denominator and then toward zero. Magnitudes at or beyond $(2^{63}-1)+\tfrac12$ round to the infinity in $x$'s direction; magnitudes too small to represent round to the zero in $x$'s direction. The nearest value is found by continued fractions, with the algorithm and proof of the first spec, which hold for any fit rule closed under taking smaller numerators and denominators, as this one is. There is no per-value rounded flag; whether a value was rounded is a property of the code that produced it.

## Columns

A table is a dictionary of columns, each of one element type. Two layouts were built and benchmarked (Evidence).

### Shared-denominator columns

Element $i$ is $i^{t_i}\, m_i / d$: a **turn byte** and a **Natural magnitude** per element, and one positive denominator $d$ for the whole column. Zero is a magnitude of 0 with its turn, exactly as in the pair.

| Tier | Per element | Magnitude range |
| --- | --- | --- |
| 64-bit | turn byte + one Natural (9 bytes) | $0 \le m \le 2^{64}-1$ |
| 128-bit | turn byte + two Naturals (17 bytes) | $0 \le m \le 2^{128}-1$ |
| arbitrary | turn byte + an unbounded natural | unbounded |

The ranges are symmetric, so there is no most-negative special case, and the 64-bit tier holds one more bit than a signed 64-bit integer. An operation runs in the higher operand tier and is redone one tier up on overflow; results demote when every element fits again; and after every arithmetic operation the common factor of the denominator and all magnitudes is divided out, stopping as soon as the running gcd reaches 1. Sums are accumulated per turn in 128-bit words, which cannot overflow for fewer than $2^{64}$ elements, with a vectorised 64-bit fast path that a bitwise OR of the magnitudes proves safe; additions select between $a+b$ and $|a-b|$ without branching. A shared-denominator column holds finite values and zeros; it has no per-element slash, so it cannot hold an infinity or undefined.

### Own-slash columns

Element $i$ is a full pair: 9 bytes, each element with its own numerator and denominator. Such a column holds every value a pair can, including directed zeros, infinities and undefined, which are ordinary words, so no side bitmap is needed. Each elementwise result is put in lowest terms individually; reductions run over the least common multiple of the element denominators. Elements that do not fit one pair are promoted individually.

### Complex columns

Both layouts carry a turn per element, so a column whose elements are each real or imaginary is already complex: the imaginary tag is part of the Unit rather than a separate mask. A **one-part** complex column is one column with any turns; a **two-part** column is a real-part column and an imaginary-part column. Multiplying one-part columns adds turns. Adding them widens to two parts only if some element adds a real to an imaginary value, and a two-part result narrows back when no element has both parts nonzero. Two-part products use the schoolbook formula, except that an element with one part zero multiplies by the Unit rules, so infinities keep their direction. Rotation by a quarter turn and conjugation change only the turn bytes.

## Values larger than one pair

Exactness requires that a value too large for one pair goes somewhere larger. This version specifies the behaviour, not the representation:

- a result that does not fit one pair is **promoted** to an exact value of unbounded size, never rounded;
- a promoted value is **demoted** back to a pair as soon as it fits;
- promotion is invisible to the program: the value's type is unchanged.

All 64 slash values have meanings, so the marker for "this value continues" must live outside the pair, for example in a column's metadata or a type-level tag. The prototype keeps promoted elements exactly in a side table at interpreter speed, so benchmark times for workloads that promote often measure that stand-in, not a designed extension. How often promotion happens is measured (Evidence), because it decides how much the extension matters.

## Determinism and platform requirements

As in the first spec: exact results are bit-identical on every platform, the iteration order of unordered results is defined, and results never depend on thread count. The Natural needs only native 64-bit unsigned operations and a 64×64→128-bit multiply, which every target has; WebAssembly lowers the 128-bit product to a short sequence of 64-bit operations. Turn arithmetic is byte-wise and vectorises on every SIMD target.

## Evidence

A prototype of both column layouts was built and benchmarked against NumPy's binary64 arithmetic, with every exact result checked against an independent exact computation. It was reviewed three times by independent reviewers before the final run. Timings are medians from one shared 2-core cloud machine, single-threaded; the ratio is time relative to NumPy, so below 1 is faster than NumPy. The exact contenders have zero error in every row.

**Column operations** ($10^7$ values unless stated; 3-place decimals):

| Operation | NumPy error | Shared denominator | Own slash |
| --- | --- | --- | --- |
| sum | 4.9e-16 | 1.4× | 21× |
| mean | 5.4e-16 | 1.2× | 18× |
| $a\cdot b + c$ | 2.6e-16 | 3.6× | 537×, 9.1% of elements promoted |
| $a\cdot b + c$, 6 places ($10^6$) | 2.6e-16 | 5.6× (128-bit tier) | 5,146×, 99.99% promoted |
| $a\cdot b + c$, 2 places, no overflow | 2.3e-16 | 4.7× | 51×, nothing promoted |
| group-by sum, 1,000 groups | 7.3e-15 | 1.2× | 3.8× |
| variance | 2.3e-17 | **0.24×** | 3.8× |
| variance, offset $10^6$ | 2.8e-13 | **0.20×** | 3.2× |
| variance, offset $10^7$ ($10^6$) | 9.7e-12 | 1.2× | 3.5× |

**Integrated pipelines** (the pipelines of the first spec's Chapter 12):

| Pipeline | NumPy worst error | Shared denominator | Own slash | Pure-Python exact |
| --- | --- | --- | --- | --- |
| E1 least-squares regression, 20,000 rows | 4.6e-12 | 1.6× | 45× | 562× |
| E2 table pipeline, 100,000 rows | 1.2e-12 | 9.7× | 723×, 99.98% of converted revenue promoted | 316× |
| E3 complex 4-point DFT, 100,000 values | 4.6e-15 | 1.6× | 23× | 556× |
| E4 exact solve, $n = 500$, planted answer | 6.4e-10 | — | 7.4×, all 500 entries fit one pair | — |
| E4 exact solve, $n = 500$, random | 4.8e-12 | — | 223×, no entry fits one pair | — |

**Precision and range.** Explicit rounding to one pair has a median relative error of about $7\times10^{-20}$ at every magnitude in range, against about $4\times10^{-17}$ for binary64: roughly three more decimal digits than a double, and about 17 fewer than the first spec's 120-bit word. The range is $1.1\times10^{-19}$ to $9.2\times10^{18}$; anything outside it is promoted, never lost.

**What fits one pair exactly:**

| Values | Fit |
| --- | --- |
| 3-place decimals below $10^6$ | 100% |
| products of two such decimals | 10.8% |
| binary64 values, converted exactly | 0.003% |

**What the evidence says:**

1. **Shared-denominator columns on Naturals are the practical layout.** They stay exact while running at 1.2× to 5.6× NumPy's time on column operations and 1.6× to 9.7× on the pipelines, and variance is 4 to 5× *faster* than NumPy, because the sums of squares are exact integer sums with no cancellation to guard against. Their 64-bit tier holds one more bit than a signed integer, and their sums never leave compiled code.
2. **Own-slash columns pay for per-element denominators.** Even with nothing promoted they were 18× to 51× NumPy's time, because every element is put in lowest terms with its own gcd. They are the only layout that holds infinities and undefined per element.
3. **The extension for larger values is the main open question.** 64-bit pairs overflow on 1 in 9 products of ordinary decimals, on nearly all 6-place products, in the table pipeline's chained products, and on every exact solution of a random dense system. The very large own-slash times in those rows measure the interpreter-speed stand-in, not a designed extension.
4. **Input should be parsed as decimal text, not through floats.** Almost no double fits one pair exactly (0.1 as a double is a 108-bit fraction), while every 3-place decimal does.

For orientation only: the first spec's integrated prototype reported 5.7× NumPy on E1, 16.5× on E2 and 1.8× on E3. The shared-denominator layout here measured 1.6×, 9.7× and 1.6× on the same pipelines. The column-operation rows are not like for like: the first spec's $a\cdot b+c$ used one fused kernel and its 6-place case rounded rather than staying exact.

The benchmark itself is a literate program (prose and code chapters tangled into a runnable build) with its own tests; the tables above summarise its full results.

## Open decisions

- [ ] **The extension for values larger than one pair.** This is now the most important open question, not an edge case: 64-bit pairs overflow on about 1 in 9 products of two 3-place decimals, on almost every product of 6-place decimals, and on every exact solution of a random dense system (Evidence). Its marker must live outside the pair. Candidates: a column-level tier flag (as shared-denominator columns already have), a per-element side bitmap, or a type-level tag.
- [ ] **Which column layout is the default.** Shared-denominator columns were within small factors of NumPy, and faster on variance; own-slash columns were 4× to 50× slower even without overflow, because every element pays for its own gcd. Own-slash columns are still the only layout that can hold infinities and undefined per element. One option: shared-denominator by default, own-slash when a column needs special values or mixes unrelated denominators.
- [ ] **Undirected infinity.** Every nonzero Natural at slash 63 means infinity, but only $2^{63}$ is used, so spare patterns exist. If the single point at infinity of complex analysis is ever wanted (for example as the result of $1/0$ for a plain Natural lifted into a pair), it fits there without a new code.
- [ ] **Strict mode.** Whether programs can ask for an error, rather than an infinity or undefined, when a pair divides by zero.
- [ ] **Narrowing complex columns.** A widen-and-narrow round trip keeps every value but returns an imaginary zero as the real $+0$. Decide whether that is acceptable or whether two-part columns should remember the direction of a zero element.
- [ ] **Confirm the evidence on target hardware.** All figures come from one shared 2-core cloud machine; rerun on the machines the language will target.

## Considered and dropped

| Idea | Why it was dropped |
| --- | --- |
| Slash 0 meaning zero | It spent a slash value on something the Natural can say by itself (a Natural of 0) and cost the split with a 63-bit numerator, so the largest integer was $2^{62}-1$. |
| Zero as a Unit value | Zero is a fact about the digits, so it belongs to the Natural. A Unit alone is a unit or an infinity. |
| A separate bitmap for undefined | $0/0$ already has a word: slash 63 with a Natural of 0. |
| Discarding a pair zero's direction | Then $1/(1/(-\infty)) = +\infty$: the direction of the reciprocal had to be invented. Keeping the Unit makes products and quotients fall out of the Unit rules. |
| Division by a pair zero as an error, or as undefined | Neither is needed: dividing by zero moves the slash to 63, and the zero's Unit gives the direction. (A plain Natural keeps the traditional error.) |
| A 128-bit Natural | Its 129 split points plus the special values need an 8-bit slash, so the Unit would be 10 bits and no longer a byte. With a 64-bit Natural, 2 turn bits and a 6-bit slash make exactly one byte. |
| A cache of quotients in the reductions | Measured about 1.1× faster on 3-place data and up to 1.8× slower on 6-place data. |
