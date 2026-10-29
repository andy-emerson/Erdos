> **Milestone reached: round 2 run complete (textbook Q32 vs textbook control)** (Oct 29, 2026, 17:45 CDT)
>
> The round 2 run of [`10-interning`](10-interning/) finished in **2.5 min** (hard stop 30), pilot and full run, with no stop: 20 timed blocks, 0 disturbed, 0 busy idle checks, 0 harness bugs. Rule sha256 `3ad56ffe…`. Jobs 5–7 are over Q32's capacity and not run. Per measure only, **no overall winner**.
>
> **Time**, T(TC)/T(Q32) [interval], null band h 0.050; below 1, Q32 is slower. **Bytes per rational** at the end (B_used Q32 / TC, ratio).
>
> | job | time | B_used | B_heap ratio |
> | --- | --- | --- | --- |
> | 1 MONEY-wide | loss 0.655 [0.652, 0.665] | 16.33 / 16.00, tie 1.021 | tie 1.012 |
> | 2 MONEY-catalog | loss 0.650 [0.648, 0.660] | 10.29 / 16.00, win 0.643 | win 0.636 |
> | 3 SMALL8 | loss 0.730 [0.723, 0.734] | 4.16 / 16.00, win 0.260 | win 0.270 |
> | 4 SMALL64 | loss 0.661 [0.657, 0.665] | 14.06 / 16.00, win 0.879 | tie 1.009 |
> | 8 CH-money | loss 0.645 [0.643, 0.649] | 12.43 / 16.00, win 0.777 | win 0.631 |
> | 9 CH-closed | loss 0.684 [0.674, 0.692] | 12.53 / 16.00, win 0.783 | win 0.631 |
> | 10 CH-small | loss 0.610 [0.607, 0.615] | 12.51 / 16.00, win 0.782 | win 0.634 |
> | 11 CH-mixed | loss 0.620 [0.616, 0.628] | 13.28 / 16.00, win 0.830 | win 0.634 |
>
> **Gcd work check:** passed on all 8 jobs (G(Q32)/G(TC) = 1.000000; call for call equal).
>
> **Profile (indicative only).** Q32's extra time in the operations is 21–46 ns/op: mostly **interning lookup (I) 10–28**, operand fetch (F) 8–10, record write (S) 4–6, growth (M) 0–5, shortcut checks (C) 0–2 and unattributed −1 to 5; gcd and other arithmetic are 2–7 ns *lower* in Q32. At entry the extra 12–25 ns is mostly I (9–16) and S (4–6). Read-out differs by 0.5 ns.
>
> **Process CPU time cross-check:** median CPU share 0.998–1.000 for every job and row (lowest 0.950, job 9 Q32); CPU and total ns per run agree.
>
> **Runner ticks:** 48 over 378 launches, 0.127 per launch; highest block 0.333 (limit: more than 1 per launch plus 2). Reported only.
>
> **Flags:** no severe flags. Q32's B_used is 2% above TC's on job 1 (W/N ≈ 1, no reuse), a tie.
>
> Paper v0.5 (sections 1–2 and Appendix A) is published ([paper/v0.5.md](paper/v0.5.md)).
>
> **Results:** [results.md](10-interning/test/results-r2/results.md) · [verdicts.csv](10-interning/test/results-r2/verdicts.csv) · [run.log](10-interning/test/results-r2/run.log) · [rule](10-interning/research/rule-r2.md) · [DECISIONS](10-interning/test/DECISIONS.md)

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
