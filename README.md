> **Milestone reached: matrix runs complete (11-matrix, addition and multiplication)** (Oct 30, 2026, 00:57 CDT)
>
> Both runs finished with no stop and 0 disturbed blocks (addition 6.7 min, multiplication 11.5 min; hard stop 40). Rule sha256 `8e951ac6…`. Gcd work check passed on all 32 jobs. Per measure only, **no overall winner**. Time: T(TC)/T(row), below 1 = row slower. Bytes: row/TC, below 1 = row smaller.
>
> | row vs TC | addition: operation | addition: memory | multiplication: operation | multiplication: memory |
> | --- | --- | --- | --- | --- |
> | Hash-V-fixed | loss 16/16, 0.39–0.59 | loss 16/16, 1.92–2.17 | loss 16/16, 0.84–0.91 | loss 16/16, 2.25–2.50 |
> | Hash-M-fixed | loss 16/16, 0.33–0.53 | loss 16/16, 2.08–2.50 | loss 16/16, 0.84–0.90 | loss 16/16, 2.42–2.83 |
> | Hash-V-grow | loss 16/16, 0.24–0.51 | **win 10**, tie 3, loss 3; 0.31–1.92 | loss 16/16, 0.82–0.91 | tie 7, loss 9; 0.94–2.25 |
> | Hash-M-grow | loss 16/16, 0.21–0.50 | **win 10**, tie 2, loss 4; 0.32–2.08 | loss 16/16, 0.81–0.90 | tie 6, loss 10; 0.94–2.42 |
>
> Creation is slower in every row (0.17–0.77). Translation (M) vs direct comparison (V): no gain (tie in multiplication, slower in addition). Map vs lookup rewrite: map faster on creation on 9 of 16 (addition) and 6 of 16 (multiplication). In multiplication, C's values are nearly all distinct, so grow cannot win on memory there.
>
> **Results:** [addition](11-matrix/test/results-m1-add/results.md) · [multiplication](11-matrix/test/results-m1-mul/results.md)

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
