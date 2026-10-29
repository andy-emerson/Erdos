> **Major issue: needs your decision** (Oct 29, 2026, 13:13 CDT)
>
> **What happened.** The round 2 measured run of [`10-interning`](10-interning/) (textbook Q32 vs textbook control) stopped in **step 3, the pilot**, after 26.7 s, on its first timed block (job 1, calibration and 2 rounds). The block was **disturbed twice**: 2 unexplained forks, then 1 on the re-run. The rule allows 0 (rule-r2 item 5), and a second disturbance of one job stops the run. I did not retry. **No verdicts**, and nothing past the pilot ran: no gates, counting, memory, rounds or profile.
>
> **What came before passed.** Build, workloads byte-identical to round 1, and unit tests, the differential test and both under the sanitizers (0 failures).
>
> **The record.** No other process used a tick, and no new process was seen (`others` and `new` are empty). The runner used 1 tick per block. The fork counter rose 10 and 9 against 8 spawns, with 0 new kernel threads. Host ticks over the blocks (1.6 s each): `claude` (the session process hosting the run) 2, then 1, about 1.25 and 0.62 ticks/s; `process_api` (pid 1) 1, then 0. These host ticks do not flag in the pilot, but the first block's `claude` rate is above the 1 tick/s that stops a run after the pilot (item 5.3). The record cannot name the forker. My guess is short-lived threads of a host process: the counter counts threads too, and those threads ran for no recorded tick. This is a guess, not checked.
>
> **Options.**
> 1. **Re-run unchanged** in the same setup. Cheap, but likely to stop the same way, and even a clean pilot may trip the busy-host stop, since `claude` was at about 1.25 ticks/s.
> 2. **Run from a quieter host:** start `python3 test/r2_runner.py` yourself in a plain terminal with no Claude session open, so the only hosts are a shell blocked in `wait` and pid 1.
> 3. **Revise the rule** (dated, at the top): for example, explain forks by the host processes' own new threads (`/proc/<pid>/task`), or give the hosts a small fork allowance measured in the pilot like their ticks. The harness pin would then be updated under review.
>
> **Still open, apart from the run.** Paper v0.5 sections 1–2 are drafted ([paper/v0.5.md](paper/v0.5.md)). For you to confirm: **R2-H7** in [test/DECISIONS.md](10-interning/test/DECISIONS.md): estimated times are reported only per part (entry, operations, read-out), as the spec gives them.
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
