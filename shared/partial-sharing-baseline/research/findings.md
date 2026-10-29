# Findings: partial sharing by common divisors and by a coprime base

Oct 29, 2026 · **Concluded.** A short account of what the joint measurement of Designs 1 and 2 taught us. Each design's own write-up is in its folder: [Design 1](../../../08-common-divisors/research/findings.md), [Design 2](../../../09-coprime-base/research/findings.md). It draws on the measured run ([results](../test/results/results.md), commit `f39d34e`), the [pilot finding on eager refinement](../../../09-coprime-base/research/eager-d2-finding.md), the [design note](design-note.md), the amendments of the joint [spec v0.1](../v0.1.md) and the library and harness decisions ([lib](../lib/DECISIONS.md), [test](../test/DECISIONS.md)). It adds no measurement. Like the [rule](rule.md) (item 11), it names no overall winner beyond what each measure shows.

## The question

Most pairs of denominators share some factors but not all. Both designs try to exploit that middle ground (**partial sharing**) inside one class of 64-bit rationals:
- **Design 1 (common divisors)** remembers the gcds found between pairs of denominators. A common divisor already known for a new pair shrinks the gcd still to be computed.
- **Design 2 (the coprime base)** writes each denominator over a shared set of pairwise coprime pieces, so that what two denominators share can be read from their rows ([notation](../../../09-coprime-base/research/coprime-base-notation.md)).

Both were built as **skeletons**: the plainest correct form of each design, with every optimization left out on purpose. They were measured against a **control** (plain reduced rationals, one gcd chain per operation) and, where every value shares one denominator, a **ceiling** (one shared denominator for the whole vector).

## How the designs changed before the measured run

The pilot (rule step 3; not a measurement) exposed three problems. Each was fixed by a spec amendment of Oct 29, 2026, approved by the author before any measurement. None changes a stored value.

- **Design 2: from eager to on-demand refinement.** As first specified, Design 2 refined every new denominator, entered or a result, against the whole base as soon as it was stored: up to one gcd per piece of the base for each new denominator. The base grew to 2,400–3,200 pieces, mostly private parts of single denominators that never met a partner. Design 2 computed 4–11 million gcds per mixed job against the control's 31–47 thousand (58–161 times the control's gcd work) and ran 55–137 times slower. It now refines **on demand**: a row holds its known pieces and an unrefined remainder, entry computes no gcd against the base, and two denominators are refined against each other only when they meet in an addition, subtraction or division, and only as far as that meeting needs. On the same jobs this gave 31–164 thousand gcds and bases of 3–64 pieces ([finding](../../../09-coprime-base/research/eager-d2-finding.md)).
- **Design 1: a sparse pair table.** The dense pair table, one bit for each pair of denominators, grows with the square of their number: at the roughly 210,000 denominators of a 200,000-operation run it alone would have reached about 2.8 GB. It became a hash set of the pairs actually compared.
- **Design 1: a sparse incidence matrix.** The dense denominator-by-divisor bit matrix, 7–53% of Design 1's bytes per rational in the pilot, became one sparse row per denominator listing only the divisors known to divide it, with their residuals.

## What was measured

27 jobs of 20,000 operations, 12 interleaved rounds each, at width 64 (two of them at width 32). The 23 verdict jobs fall into seven strata: **ALL** (one shared denominator), **P75**, **P50** and **P25** (denominators drawn to share factors with decreasing likelihood), **NONE** (pairwise coprime), **LG** (large shared factors) and **CHAINS** (results fed back as operands). The measures are kept apart:
- **time**: T(ctl) / T(X); above 1, X is faster;
- **gcd work**: G(X) / G(ctl), the summed bit length of gcd operands; below 1, X does less;
- **memory**: B(X) / B(ctl), used bytes per rational at the end of the job; below 1, X uses less.

Every stored value matched the exact reference, all 83 gates passed, the repeated counting runs were identical, and no bug appeared after the gates. The null control's spread was at most 0.007 on every job, so the time band was ±5% throughout.

## Results against the control

Median ratio per stratum (jobs 1–18 and 21–25):

| Stratum | D1 time | D2 time | D1 gcd work | D2 gcd work | D1 memory | D2 memory |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ALL | 0.838 | 0.863 | 0.839 | 0.876 | 1.386 | 1.358 |
| P75 | 0.416 | 0.271 | 0.989 | 2.045 | 3.735 | 4.284 |
| P50 | 0.457 | 0.305 | 0.992 | 2.075 | 3.599 | 3.763 |
| P25 | 0.515 | 0.426 | 0.998 | 1.636 | 3.474 | 3.132 |
| NONE | 0.570 | 0.732 | 0.999 | 1.001 | 3.416 | 2.695 |
| LG | 0.479 | 0.362 | 0.992 | 1.737 | 3.532 | 3.502 |
| CHAINS | 0.529 | 0.458 | 0.946 | 1.212 | 2.156 | 1.760 |

Verdicts over the 23 jobs:
- **Time.** Both designs lost to the control on **every** job (23 losses each). In the partial strata (P75, P50, P25, LG), Design 1 took 1.9–2.4 times the control's time and Design 2 2.3–3.7 times; in ALL, about 1.2 times.
- **Gcd work.** Design 1 won on 4 jobs (ALL-muldiv 0.82, ALL-mixed 0.84, CH-closed 0.64, CH-all 0.66) and tied on the other 19; in the partial strata it did 0.989–0.999 of the control's gcd work. Design 2 won on 2 jobs (ALL-muldiv 0.69, ALL-mixed 0.88), tied on 5 and lost on 16; in the partial strata it did 1.6–2.1 times the control's gcd work.
- **Memory.** Design 1 won on 2 jobs (ALL-add 0.75, CH-all 0.75) and lost on 21. Design 2 won on 3 (ALL-add 0.75, CH-closed 0.77, CH-all 0.75) and lost on 20. In the partial strata both used 3.1–4.3 times the control's memory. The severe-memory-loss flag (ratio ≥ 4.0) was raised 4 times for Design 1 (once each in P75, P50, P25 and LG) and 3 times for Design 2 (twice in P75, once in P50).

## Head to head (Design 1 against Design 2)

Time is T(D2) / T(D1), gcd work G(D1) / G(D2), memory B(D1) / B(D2); a time ratio above 1, or a gcd or memory ratio below 1, favours Design 1.

| Stratum | time | gcd work | memory |
| --- | ---: | ---: | ---: |
| ALL | 1.030 | 0.971 | 1.021 |
| P75 | 1.535 | 0.484 | 0.918 |
| P50 | 1.461 | 0.478 | 1.023 |
| P25 | 1.209 | 0.610 | 1.166 |
| NONE | 0.779 | 0.998 | 1.267 |
| LG | 1.276 | 0.571 | 1.089 |
| CHAINS | 1.143 | 0.709 | 1.022 |

- **Time.** Design 1 was faster on 15 jobs (all of P75, P50, P25 and LG, and 3 chains), tied on 3 and slower on 5 (all of NONE, ALL-muldiv and CH-none).
- **Gcd work.** Design 1 did less on 16 jobs, about half of Design 2's in P75 and P50; it tied on 6 and did more on 1 (ALL-muldiv, 1.20).
- **Memory.** Mostly a tie (14 jobs). Design 1 used more on 8 (all of P25 and NONE, CH-closed and CH-none) and less on 1.

Where denominators share factors partially, Design 1 was the cheaper of the two on time and gcd work. With no sharing at all, Design 2 was the cheaper on time and memory. Both stayed behind the control.

## Other observations

- **The ceiling only ties the control on time.** On the two jobs where every value shares one denominator, the ceiling tied the control on time (1.051 on ALL-add, 0.950 on CH-all) while using half its memory. Even perfect sharing, with no gcd at all on ALL-add, buys no measurable time at 64 bits.
- **Arrival order does not matter.** Entering denominators in batches (job 19) gave the same time, gcd work and end memory as entering them all first (job 20), for every row (time ratios 0.998–1.002).
- **Width 32 changes the order but not the time verdicts.** Both designs won on gcd work at width 32 (0.70–0.85) and still lost on time (0.60–0.90).
- **Gcd work saved did not become time saved.** Every job where a design won on gcd work (ALL-muldiv, ALL-mixed, CH-closed, CH-all and both W32 jobs) was still a time loss.

## Lessons

1. **At 64 bits, the bookkeeping costs more than the gcds it saves.** A 64-bit gcd is cheap. Lookups, hash probes, row merges, piece scans and the extra memory traffic of both designs cost more than the gcd steps they avoid. Even where a design won on gcd work, it lost on time.
2. **Neither design reduced gcd work where it was meant to.** Partial sharing was the point. There, Design 1 did about 0.99 of the control's gcd work, a tie in every partial stratum, and Design 2 did 1.6–2.1 times as much. The only gcd savings came with full sharing (ALL, closed chains), which the control's same-denominator fast path and the ceiling already serve. So **larger number sizes are unlikely to rescue these designs**: wider numbers make each gcd dearer, but a design that removes no gcd work has nothing to gain from that.
3. **Eager completeness was the flaw in Design 2's first form.** Keeping the base pairwise coprime across all denominators, at once, did work that almost no operation needed. The fix gave a principle that carries over: **no work in advance, no work in the background; each operation computes only what it needs.**
4. **Dense structures do not survive many denominators.** Design 1's pair table grows with the square of the number of denominators, and its incidence matrix with denominators times divisors. Both had to become sparse before a full run was possible.
5. **The control was very hard to beat.** Plain reduced rationals, with one gcd chain per operation and a same-denominator fast path, won or tied on time on every verdict job against every other row, the ceiling included. A new design has to beat it on time, not only on gcd counts.

These are results for **skeletons**: the design note lists improvements for each design, and none were measured. But lessons 1 and 2 hold for any improvement that keeps the same bookkeeping, since the bookkeeping, not the gcd, is what costs.

## Status

**Concluded.** Superseded by the active work on Design 3 in [`09-interning`](../../../09-interning/) (to be renumbered 10): whole numbers interned and stored once, with index-equality shortcuts in place of partial sharing. The index-valued pair table, a deferred variant of Design 1 ([rule](rule.md), "Deferred"), was not run.
