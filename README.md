> **Milestone reached: matrix round m2 complete (packed build; four runs)** (Oct 30, 2026)
>
> All four runs finished with no stop and 0 disturbed blocks: add 5.1 min, mul 7.4, sum of products 11.2, product of sums 8.2 (hard stop 40). Rule sha256 `e340d32b…`. Gcd work check passed on all 64 jobs. No row pre-stores 0 or 1. Cost as a multiple of the control (1.00 = control, lower is better): **median over 16 jobs [range]**; verdicts per job are in the results.
>
> | measure | Hash-V-fixed | Hash-V-grow-map | **Hash-V-pack** |
> | --- | --- | --- | --- |
> | build one matrix (time) | 1.4–1.5× | 1.7–2.0× | 1.6–1.9× |
> | addition (time) | 1.89× | 2.38× | 2.17× |
> | multiplication (time) | 1.12× | 1.12× | 1.12× |
> | sum of products (time) | 1.12× | 1.12× | 1.12× |
> | product of sums (time) | 1.11× | 1.12× | 1.11× |
> | stored matrix (bytes) | 2.25× [2.25–2.50] | 0.55× [0.28–2.25] | **0.29× [0.14–0.80], win on 16/16** |
> | peak, addition | 2.08× | 0.86× | 0.92× [0.68–1.28] |
> | peak, multiplication | 2.33× | 1.67× | 1.35× [1.18–1.71] |
> | peak, sum of products (reported) | 2.29× | 1.57× | 0.91× |
> | peak, product of sums (reported) | 2.00× | 1.17× | 0.83× |
>
> **Headline:** the packed build stores every matrix in 14–80% of the control's memory (median 29%) and has the lowest peaks in multiplication and both expressions, at the same operation speed as the fixed table in multiplication and both expressions (ties on all 48 jobs). It is slower to build (pack vs fixed 0.79–0.87) and in addition (0.86). No version beats the control on speed. Minor, recorded: each timed part takes its own minimum of reps 2 and 3 (as rule-m2 says; m1 used the rep with the smallest total), applied to every row alike.
>
> **Results:** [add](11-matrix/test/results-m2-add/results.md) · [mul](11-matrix/test/results-m2-mul/results.md) · [sum of products](11-matrix/test/results-m2-sop/results.md) · [product of sums](11-matrix/test/results-m2-pos/results.md)

# Erdos

Specifications for an exact number system, one folder per specification, numbered in the order they were designed. Each holds its versions (`vX.Y.md`), its research notes (`research/`) and its benchmark harness, tests and results (`test/`).

**Active now:** the concept paper ([`paper`](paper/), v0.4 current), Design 3 ([`10-interning`](10-interning/)), and next the 2-3-5 design (11, not yet created).

- [`01-number-system`](01-number-system/): the Exact Number System: what a number is in the language, how it is stored, how arithmetic behaves, and how linear systems are solved exactly. Versions: [v0.1](01-number-system/v0.1.md).
- [`02-unit-natural`](02-unit-natural/): the Unit–Natural number system, an alternative design built from a 64-bit Natural and a one-byte Unit. Versions: [v0.1](02-unit-natural/v0.1.md).
- [`03-q`](03-q/): Q, Gaussian-unit rational formats (Q2, Q24, Q40, Q72, Q144) with a shared-denominator vector form; research on LCF order keys, quote notation and the plane variant. Versions: [v0.1](03-q/v0.1.md) (as Q72), [v0.2](03-q/v0.2.md), [v0.3](03-q/v0.3.md).
- [`04-division-algebra-family`](04-division-algebra-family/): a family over the division algebras (reals, complex numbers, quaternions, octonions). Research only; no spec text (see its [README](04-division-algebra-family/README.md)).
- [`05-exact`](05-exact/): `Exact<uW>`, unsigned fixed-width rationals with the slash outside the word, and their vector form. Its research is in 04 (see [research](05-exact/research/README.md)). Versions: [v0.1](05-exact/v0.1.md).
- [`06-alg`](06-alg/): the units (sign and i) as symbols acting on magnitudes. No spec yet; research: [Clifford generator count](06-alg/research/generators.md) (a direction explored and set aside).
- [`07-prime-graph`](07-prime-graph/): exact rationals in a weighted bipartite graph of numerators and denominator primes, with lazy decomposition; the vector form becomes the case of values with identical denominators. Versions: [v0.1](07-prime-graph/v0.1.md), [v0.2](07-prime-graph/v0.2.md) (same design, rewritten for readability), [v0.3](07-prime-graph/v0.3.md) (pure specification; consistency section added).
- [`08-common-divisors`](08-common-divisors/): Design 1 of partial sharing: denominators remember the gcds found between them (sparse rows of known divisors, a set of compared pairs), so a known common divisor shortens the next gcd. **Concluded:** it lost to the control on time on every verdict job ([findings](08-common-divisors/research/findings.md)). Versions: [v0.1](08-common-divisors/v0.1.md).
- [`09-coprime-base`](09-coprime-base/): Design 2 of partial sharing: denominators written over a shared coprime base, refined on demand when two of them meet. **Concluded:** it lost to the control on time on every verdict job ([findings](09-coprime-base/research/findings.md)). Versions: [v0.1](09-coprime-base/v0.1.md).
- [`10-interning`](10-interning/): **active.** Design 3: whole numbers interned and stored once, with index-equality shortcuts. Versions: [v0.1](10-interning/v0.1.md).
- `11` (not yet created): **next.** The 2-3-5 design.
- [`paper`](paper/): **active.** The concept paper explaining the idea (problem, lowest terms, common denominators, exact sharing of whole numbers), independent of any spec. Drafts: [v0.1](paper/v0.1.md), [v0.2](paper/v0.2.md), [v0.3](paper/v0.3.md), [v0.4](paper/v0.4.md) (current).
- [`shared`](shared/): generic harness tools used by several specs (the literate-source tangler and the test runner), and the [partial-sharing baseline](shared/partial-sharing-baseline/): the joint run of 08 and 09 against one control, with its rule, harness and results.
