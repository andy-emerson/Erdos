> **Major issue: needs your decision** (Oct 29, 2026, 16:38 CDT)
>
> **What happened.** The round 2 measured run of [`10-interning`](10-interning/) (textbook Q32 vs textbook control) **stopped in step 6** after 47.4 s, at the end of its first measured block (job 1, calibration and 12 rounds, 8.4 s, 38 timed invocations). The runner itself used **5 ticks** in that block. The rule says more than 2 is a harness bug (rule-r2 item 5), and the harness stops the run on one (R2-H5). I did not retry. **No verdicts.** No rounds of jobs 2–11 and no profile ran.
>
> **What passed.** The build, workloads byte-identical to round 1, unit, differential and sanitizer tests (0 failures), and the **pilot**: 4 blocks, all undisturbed. The projection was 7.1 min (limit 22). The highest host rate was 0.62 ticks/s (ceiling 4). Profiling slowdowns were Q32 1.23 and TC 1.25 (limit 1.5). Then all 16 gates, and the counting and memory runs.
>
> **The stopping block was otherwise clean.** No other process used a tick, no new process appeared, and every host stayed within its allowance (`claude` used 1 tick against 15). The fork counter rose 38, matching the runner's 38 spawns. The block was not flagged as disturbed.
>
> **Likely cause (a guess, not checked).** The runner's fixed limit of 2 ticks per block does not scale with the block. The pilot blocks had 2–8 spawns and the runner used 0–1 ticks. This block had 38 spawns and it used 5, about 0.13 ticks per spawn (fork, `waitpid`, output reads). The runner runs on core 0, and the timed child on core 1.
>
> **Options.**
> 1. **Treat it as a harness bug** (rule-r2 item 12): cut the runner's per-spawn cost (for example, a smaller process to fork from), review the fix, and re-run. The fix may not get a 12-round block under 2 ticks.
> 2. **Revise the rule** (dated, at the top): scale the runner's limit with the block, like the other limits. Examples: 2 + ⌈spawns / 8⌉, or a floor plus a rate per second, set from this run's record. Then update the harness pin under review and re-run.
> 3. **Run from a quieter host or a faster machine:** this only helps if the per-spawn cost falls, so it is unlikely to fix the cause alone.
>
> **Also:** paper v0.5 sections 1–2 and Appendix A are published ([paper/v0.5.md](paper/v0.5.md)). Still open for you: **R2-H7**, and the rate of 1 tick/s for other processes (R2-H15 (c)), in [test/DECISIONS.md](10-interning/test/DECISIONS.md).
>
> **Results (stopped run):** [results.md](10-interning/test/results-r2/results.md) · [run.log](10-interning/test/results-r2/run.log) · [state.json](10-interning/test/results-r2/state.json)

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
