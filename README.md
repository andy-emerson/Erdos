> **Milestone reached: Q32 run complete; repository reorganized** (Oct 29, 2026, 10:06 CDT)
>
> **Reorganization.** [`08-common-divisors`](08-common-divisors/) (Design 1), [`09-coprime-base`](09-coprime-base/) (Design 2), [`shared/partial-sharing-baseline`](shared/partial-sharing-baseline/) (their joint run), [`10-interning`](10-interning/) (Design 3, renamed from 09-interning; rule links changed, paths only, hash re-pinned).
>
> **Q32 run** (per measure only, no overall winner). Pilot projected 12.7 min (limit 45); run took 10.9 min; gates 60 of 60; no bugs; null spreads at most 0.013, so h = 5% on every job.
> - **Fit:** jobs 1–4 and 8–11 (1, 8, 9 near capacity). **Over capacity:** jobs 5, 6, 7 (W = 40,849, 40,798, 50,795).
> - Time is T(other)/T(Q32 row), above 1 means the Q32 row is faster; bytes are B_used(Q32 row)/B_used(other) per rational at the end, below 1 means it uses less. Ranges over the 8 fitting jobs:
>
> | | vs its Q64 row | vs ctl | vs fmpq | vs mpq |
> | --- | --- | --- | --- | --- |
> | **L1-Q32**, time | 1.00–1.03 (8 ties) | 0.66–0.89 (8 losses) | 0.73–0.83 (8 losses) | 1.56–2.15 (8 wins) |
> | **L1-Q32**, bytes | 0.51–0.66 (8 wins) | 0.26–1.02 (7 wins; tie on job 1) | 0.26–1.02 (7 wins; tie on job 1) | 0.07–0.31 (8 wins) |
> | **L1-opt-Q32**, time | 0.98–1.00 (8 ties) | 0.63–0.95 (7 losses; tie on job 3) | 0.69–0.86 (8 losses) | 1.51–2.30 (8 wins) |
> | **L1-opt-Q32**, bytes | 0.51–0.75 (8 wins) | 0.27–1.57 (win on job 3, tie on 2, 6 losses) | 0.27–1.57 (same as ctl) | 0.07–0.47 (8 wins) |
>
> - **Model vs B_heap:** B_used equals the model to the byte on every row and job (an accounting identity, so it tests the report only). B_heap / model at the end: L1-Q32 1.09–1.50 (highest on chains 10 and 11), L1-opt-Q32 1.04–1.14.
> - **Q64 verdicts changed between runs:** none (0 of 348). The official Q64 verdicts stay those of `b3b88b6`.
>
> **For you to confirm:** safeguard 2's wording now includes taking the band h from the re-run's own null control ([rule](10-interning/research/rule.md), top; marked pending).
>
> **Notes.** (1) In the original measured runs (08 at `f39d34e`, 09 Q64 at `b3b88b6`), a small waiting loop of the runner's own (a 20-second file check) kept running in the background through both timed phases. Its CPU use was negligible, the idle checks passed and the null spreads stayed small (at most 0.019 and 0.013), but strictly "nothing else running" was not true. Both runs can be repeated in about 22 minutes if you want fully clean timing. (2) The same applies to this Q32 run: my own wait loop (a file check every 20 s) ran in the background through its timed phases. Its idle checks passed and its null spreads are at most 0.013. (3) `shared/checklinks.py` now skips the verbatim rule copies inside results files. Those copies are pinned data and keep the old `08-skeletons` links. With that, 0 links are broken.
>
> **Results:** [results.md](10-interning/test/results-q32/results.md) · [verdicts.csv](10-interning/test/results-q32/verdicts.csv) · [Q64 re-run verdicts](10-interning/test/results-q32/q64_rerun_verdicts.csv) · [official Q64 archive](10-interning/test/results-q64-b3b88b6/results.md)

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
