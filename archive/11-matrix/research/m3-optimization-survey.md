# m3 optimization survey: standard techniques for the exact-rational matrix rows

Oct 30, 2026 · Research pass, read-only on the code. Nothing is proposed for adoption here. Each item is marked **(S)**, a
schoolbook implementation improvement with no new data structure, or **(N)**, one that adds a structure or feature (named
plainly; it needs the owner's approval). Overflow stays an error throughout: nothing below widens a type to avoid one.
Where an item would break a fixed rule (the 08 module unchanged by sha256, call-for-call identity with TC, spec v0.3's
algorithms as written), it says so.

## What the code already does (so it is not re-proposed)

- **gcd** (`vendor08/sk_internal.h`, `sk_gcd_raw`): Stein's binary gcd with `ctz`, the shared power of two factored out
  once, the loop written for conditional moves. This is already the standard fast form.
- **Rational add/mul** (`tc3.c`, `mh2.c` `add_body`/`mul_cells`): Henrici's method is already in place: add takes
  `g = gcd(b, d)`, `t = a(d/g) + c(b/g)`, then `h = gcd(t, g)` (not `gcd(t, bd/g)`); mul takes the two cross gcds
  `gcd(a, d)`, `gcd(c, b)` before multiplying. Integer, one, zero, equal-denominator and cross cases skip gcds.
- **Factoring** (`mh3.c` `factor`): each distinct whole number of a matrix's packed list is factored once, after the
  pack, by trial division over 2 and the odd numbers, stopping at d² > m; the cofactor left is prime. Operation steps
  never factor a new numerator; they test it only at the denominator's known factors.
- **Hash build** (`mh2.c`): table of T = next power of two ≥ 2rc + 2 slots (≤ 0.5 load), Fibonacci (golden-ratio
  multiplicative) hash on the top bits, linear probing; a result's table starts at 8-bit slots and is widened (whole-table
  copy) to 16/32/64 as values need; pack copies in slot order, rewrites handles, narrows the fields, frees (h) and (p).

## 1. gcd

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| ctz of the raw difference | In the Stein loop take `ctz(u - v)` (equal to `ctz(v - u)`) so the shift count does not wait for the min/abs select; shortens the loop's dependency chain by about one op. Lemire's and the Wikipedia version both do this. | 10–30% per gcd on 64-bit values. Touches `sk_internal.h`, so it needs the 08 module's sha256 pin lifted. | S |
| Hybrid: one division first when sizes differ | If one operand is much larger (bit lengths differ by more than about 8), do one `u % v` and then run Stein on the small pair. Binary gcd otherwise spends about one iteration per bit of the difference in size. That is exactly Henrici's `gcd(t, g)`, where t is large and g small. | Up to 2–4× on those calls; neutral elsewhere. Same 08-module caveat. | S |
| Early exit on 1 or equal | Return at once when either operand is 1, or when u == v. Reduced inputs make `gcd(x, 1)` common. | Small. Most such cases are already skipped by the K-n known-answer cases. | S |
| Lehmer / half-gcd | Process the leading word with single-precision cofactors. | None: it only pays for multi-word operands. Not applicable at 64 bits. | — |
| Henrici add/mul | Covered above, already done. | Already taken. | — |
| Exact division by modular inverse (Jebelean) | Replace `a / g` (known exact) by `a * g⁻¹ mod 2⁶⁴` after removing g's power of two; the inverse comes from about 5 Newton steps. | Little or none on current x86, where 64-bit `div` costs about 15–40 cycles, close to the inverse's cost. Not worth it. | S |

## 2. Rational matrix multiply (dot products)

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| Fraction-free / delayed reduction | Accumulate `Σ aᵢbᵢ` as an unreduced N/D, either with D = ∏ dᵢ or with D = running lcm (D ← D·(dᵢ/gcd(D,dᵢ))), and reduce once at the end of the dot product. | Saves about one `h = gcd(t, g)` and two exact divisions per term. But intermediates grow: with 64-bit words and overflow as an error, it **turns jobs that TC completes into overflow errors**. It also breaks call-for-call identity with TC. **Not recommended** under the constraints. The lcm form is the least harmful variant: D only exceeds the reduced denominator when numerators cancel factors. | S (algorithm change, no structure) |
| Integer fast path in the dot product | When all of a row's and a column's denominators are 1, the dot product is pure integer multiply-add with checked overflow. | Already covered per term by the int_both cases; a whole-row check adds little. | S |
| Common-denominator rows/columns (scale to integers, multiply, unscale) | Standard in FLINT `fmpq_mat_mul`: clear each row of A and each column of B to integers, multiply integer matrices, divide out. | Overflow-prone at 64 bits for the same reason as delayed reduction; needs per-row/column denominators. Report only. | N (per-row/col denominators) |
| Loop order / B transposed | i-k-j order or a transposed copy of B's handle fields for unit-stride reads. | Negligible for n ≤ 64: B's handle fields fit L1 (64·64·2·2 B = 16 KB). Changing the order also changes the sum order and so the overflow behaviour. | N (scratch copy) |

## 3. Factoring small integers (this is where Pack-primes and Pack-coprime lose 8–260×)

The dominant cost is `mh3.c` `factor`: plain trial division to √m. A result value with a large prime factor, or one that
is itself a prime near 2⁴⁰–2⁶², costs up to about 2³⁰ divisions.

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| Derive result factorizations from operand factorizations | A product's numerator a′c′ and every result denominator are built from operand values whose factors are known (minus the cancelled gcd, also known as a factor list). Only **sum numerators** t need fresh factoring. FLINT/PARI-style "factored integer" arithmetic works this way. | The largest gain available: it removes most of the factoring. mul and the denominators of add, sop and pos then need none, so the 8–260× should fall to the cost of factoring sum numerators only. Needs each stored cell's factor list carried from its step to the pack. | **N: per-result factor lists carried from the operation to the pack** (the temporaries' list (t) already exists; this keeps it for stored cells) |
| Deterministic Miller–Rabin before and during trial division | Test the cofactor for primality (bases {2, 325, 9375, 28178, 450775, 9780504, 1795265022} are deterministic below 2⁶⁴; bases {2, 7, 61} below 2³²). Stop at once when the cofactor is prime instead of dividing up to its root. | Removes the worst case: a prime cofactor of k bits costs about 7 modular exponentiations (~10³ multiplies) instead of 2^(k/2)/2 divisions. For 40–62-bit primes that is 10³–10⁶× on those values. For m < 2³² every product fits 64 bits. Above 2³² mulmod needs a 64×64→128 product, done as a schoolbook two-word multiply in 32-bit halves or with Montgomery reduction. That is internal exact arithmetic, not a widened result type, but the owner should rule on it. | S (algorithm) |
| Trial divide only to a bound, then classify the cofactor | Trial divide to about 2¹⁶ or ∛m. The cofactor c is then 1, a prime (by MR), or a product of at most two primes above the bound (when the bound ≥ ∛m), and only that case needs a splitter. | Bounds trial division at about 3·10³ candidates per value. | S |
| SQUFOF (Shanks) for the two-prime cofactor | Square-forms factorization works on numbers up to 2⁶²; Gower–Wagstaff give a version that needs only single-word arithmetic about √N in size, so no 128-bit products. About O(N¹ᐟ⁴) steps. | Splits a 2⁶² semiprime in about 10⁴–10⁵ steps rather than 2³⁰ divisions. **Best fit for "no wider types".** | S |
| Pollard rho (Brent's variant) with Montgomery mulmod | Standard for 64-bit splitting, with batched gcds (product of 100 differences mod n, then one gcd). | Similar to or faster than SQUFOF, but needs 64×64→128 mulmod as above. | S |
| Wheel mod 6 or mod 30 | Skip candidates divisible by 2, 3 (and 5): 1/3 or 4/15 of the integers instead of 1/2. | 1.5× (mod 6) or 1.9× (mod 30) on trial division. Smallest change on the list. | S |
| Fixed prime table + divisibility by multiplying with the inverse | Divide only by primes, from a constant table, testing `n·p⁻¹ mod 2⁶⁴ ≤ ⌊(2⁶⁴−1)/p⌋` (Granlund–Montgomery / Hacker's Delight) instead of `n % p`. No division instruction. | About 3–5× on the trial-division part over odd candidates with `%`. | **N: a constant table of primes and their inverses** (spec says "no lookup aid") |
| Remove known factors first | Before trial division, divide out the factors already known for this value (from the operands, or from the matrix's base). | Part of the "derive from operands" item. On its own it helps only when known factors are tested first. | S if it uses only the operands' lists; N if it needs a lookup |
| Reuse a value's factorization across matrices | A value already factored in an input's list (f) is factored again when it appears in a result. | Moderate: denominators repeat between inputs and results. | **N: a cross-matrix lookup (value → factor list)** |
| Factor lazily | Factor a result value only when a later step needs its factors, or only partially, at the primes the step needs. | Large if results are seldom reused. The spec makes result factoring operation time, so this changes what is measured. | **N: an "unfactored" state per list entry** |
| Already done: factor each distinct value once | Yes. `factor` runs once per entry of the packed list (l), not per cell. | — | — |

## 4. Coprime base (Pack-coprime)

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| Bernstein, "Factoring into coprimes in essentially linear time" (J. Algorithms 2005) | Builds a coprime base of a set by product and remainder trees, in about linear time in the total bit size. | **Not applicable** under the constraints: its products are multi-word, so it needs bignums or would overflow 64 bits. At our sizes (≤ 15 pieces per value, at most 32 in the working set) the quadratic absorb scan is the right algorithm anyway. | N (bignum trees) |
| Eager versus lazy refinement | Eager: refine the base as each value arrives (current, the 09 absorb scan). Lazy: keep values unrefined and split only when a gcd query meets two non-coprime pieces. | Lazy saves the gcds of pieces that never meet. Most pieces only meet within a step, so the gain is probably modest, and the bookkeeping is a new feature. | **N: deferred-refinement state** |
| Absorb-scan ordering and early exits | Stop when the remainder is 1 (done). Try first the pieces found most recently, or the largest. Skip `gcd` when `rest < q` and q is known prime-like… | Small (≤ 1.5×). The scan is not the dominant cost; factoring the results is (the same fix as §3, item 1, applies to the base). | S |
| Use gcd results from the base instead of fresh gcds | Already the design: gcds come from exponent merges. | — | — |

## 5. Hash interning and the pack (Hash-V-pack: 2.2× TC on add, 1.7× on build)

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| Initialise the table with `memset(0xFF)` | EMPTY is −1, all one-bits at every width, so `table_new` can be one `memset` instead of a per-slot loop through the width `switch`. | For add, the table (4rc slots) is touched mostly by init and pack, so this is one of the larger S wins on add and build: perhaps 10–25% of the overhead. | S |
| Width-specialised loops | `slot_get`, `slot_set`, `fld_get` and `fld_set` switch on the width at every access. Hoist the switch out of the probe, widen and pack loops: one loop body per width, generated by a macro. | 10–30% on probe and pack inner loops (the branch is predictable, but it blocks vectorising the pack and init loops). | S |
| Fewer widenings | A result's table starts at 8-bit slots and is copied in full on each widening (up to 3 copies of T slots). Alternatives: a one-pass maximum over the result's magnitudes is impossible before computing them, so either start at the operands' maximum width (a good predictor for add), or store into a 64-bit table and narrow at pack (pack already copies). | Up to 3T slot copies saved per result; noticeable on add. The "64-bit then narrow at pack" variant raises the peak memory of (h). | S |
| Pack: walk the cells, not the table | Pack copies all T slots, about 2× the 2rc cells at load ≤ 0.5. An alternative is to assign list positions in order of first appearance while walking the cells, using (p) as now. | Small; it changes the list order (slot order to entry order). | S |
| Reuse (h) and (p) across operations | Keep one scratch table and map per size and clear only the used slots (W of them), via the list, instead of malloc + full init + free each time. | Removes the O(T) init and the allocator calls per result. With memset already in, the remaining gain is small. | **N: a persistent scratch table (changes (h)'s lifetime)** |
| Sort + dedup instead of hashing | Collect the 2rc values with their cell indices, radix-sort (8 passes of 8 bits, or fewer for narrow values), dedup, write handles. | Hashing at ≤ 0.5 load is about 1.5–2.5 probes per value. Sorting costs O(n·passes) and is no faster at these sizes; it gives a sorted list for free, which helps a factored row's merges. Not a speed win for Hash-V-pack. | N (sort buffer of value+index pairs) |
| Hash function | Fibonacci hashing (top bits of v·φ·2⁶⁴) is the standard choice for integer keys and spreads consecutive integers well. A murmur fmix64 finaliser only helps against structured high bits. | None expected. | S |
| Load factor | T ≥ 2rc + 2 gives ≤ 0.5 load. With linear probing that means about 1.5 probes on a hit and 2.5 on a miss. A smaller table (e.g. 2rc rounded up) would raise the load to ≤ 1, and the "full" case would become reachable. | Keep as is. | — |
| Cache effects | Results' tables are narrow (8–16-bit slots), so the whole (h) fits L1/L2 for n ≤ 64 (4·64·64 slots × 2 B = 32 KB). Inputs at 64 bits: 128 KB, L2. | No action needed at n ≤ 64. | — |

## 6. Allocation

| Technique | What it is | Expected gain here | Mark |
| --- | --- | --- | --- |
| Scratch arena per run | One bump allocator for the temporaries of a build or step ((h), (p), the audit copies, mh3's (f′), (r), S, the merge lists), reset at the end of the step. | Removes per-op malloc/free. Small for Hash-V-pack (a few calls per result); larger for Pack-coprime, whose per-step lists and rebuilds allocate repeatedly. | **N: an arena (allocation scheme)** |
| Fixed-capacity stack buffers | mh3's factor lists are bounded (FMAX = 15, SMAX = 32), so their per-step scratch can live on the stack instead of the heap. | Small to moderate in Pack-coprime steps. No new structure, only the storage class. | S |
| `fbuf` doubling | The factor buffer grows by doubling from 2W entries, then shrinks. An upper bound of 15·W entries is known, but over-allocating it costs memory, which is measured. | Small. | S |
| `realloc` on narrowing | Already done once per pack. | — | — |

## Top 10, ranked by likely gain in this setting

1. **Derive result factorizations from the operands' factor lists; factor only sum numerators.** **(N: per-cell factor lists carried to the pack.)** It removes most of the factoring, which is the 8–260× in Pack-primes and Pack-coprime.
2. **Deterministic Miller–Rabin on the trial-division cofactor; stop as soon as it is prime.** (S.) It removes the 2^(k/2) worst case for large prime factors, by orders of magnitude on those values. Above 2³² it needs a two-word mulmod in 32-bit halves, for the owner to rule on.
3. **Bounded trial division (about 2¹⁶ or ∛m) plus SQUFOF to split the remaining two-prime cofactor.** (S.) It is single-word arithmetic and handles composite cofactors without √m work. Pollard–Brent rho is the alternative, at the price of 128-bit mulmod.
4. **Wheel mod 30 in trial division.** (S.) 1.9× on whatever trial division remains; a trivial change.
5. **Constant prime table with multiply-by-inverse divisibility tests.** **(N: a constant table of primes and inverses.)** 3–5× on trial division over the wheel.
6. **`memset(0xFF)` table init plus width-specialised loops in probe, widen and pack.** (S.) The main S lever on Hash-V-pack's 2.2× add and 1.7× build overhead.
7. **Fewer result-table widenings** (start at the operands' width, or build at 64 bits and narrow at pack). (S.) Saves up to 3 full-table copies per result.
8. **Hybrid gcd (one `%` when sizes differ) and ctz of the raw difference.** (S.) 10–30% per gcd overall, more on Henrici's `gcd(t, g)`. It helps all rows equally, TC included. It needs the 08 module's pin lifted.
9. **Reuse factorizations across matrices, or factor lazily.** **(N: a value → factor-list lookup, or an "unfactored" state.)** Moderate gain. Lazy factoring changes what the spec counts as operation time.
10. **Scratch arena / stack buffers for per-step temporaries**, mainly in Pack-coprime. (S for the stack buffers within the FMAX/SMAX bounds; **N: arena**.) Small to moderate.

Not recommended under the constraints: delayed or fraction-free reduction in dot products (it causes overflow errors that
TC does not have, and breaks call-for-call identity); Lehmer gcd (no gain at one word); Bernstein's product/remainder trees
(they need bignums); sort-based packing for Hash-V-pack (no speed gain).

## Sources

- Binary gcd, ctz variants: [Wikipedia, Binary GCD algorithm](https://en.wikipedia.org/wiki/Binary_GCD_algorithm);
  [Lemire, "Fastest way to compute the greatest common divisor"](https://eklausmeier.neocities.org/lemire/blog/2013/12-26-fastest-way-to-compute-the-greatest-common-divisor).
- Deterministic Miller–Rabin bases below 2⁶⁴: [arXiv 2108.04791](https://arxiv.org/pdf/2108.04791);
  [J. Kun, deterministic Miller–Rabin](https://jeremykun.com/2026/04/07/deterministic-miller-rabin).
- SQUFOF: [Gower & Wagstaff, "Square form factorization", Math. Comp. 2008](https://homes.cerias.purdue.edu/%7Essw/squfof.pdf);
  [Wikipedia, SQUFOF](https://en.wikipedia.org/wiki/Shanks%27s_square_forms_factorization).
- Coprime bases: [D. J. Bernstein, coprimes](https://cr.yp.to/coprimes.html);
  [Bernstein, "Detecting perfect powers by factoring into coprimes", Math. Comp. 2007](https://www.ams.org/journals/mcom/2007-76-257/S0025-5718-06-01837-0).
- From my own knowledge (not fetched): Henrici's rational add/mul (Knuth, TAOCP vol. 2 §4.5.1); Granlund–Montgomery,
  "Division by invariant integers using multiplication" (PLDI 1994) and Hacker's Delight §10-17 for the divisibility test;
  Jebelean's exact division (1993); Brent's variant of Pollard rho (1980); FLINT's `fmpq_mat_mul` clearing denominators.
