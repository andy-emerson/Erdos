# Findings: Design 2, the coprime base

Oct 29, 2026 · **Concluded.** What Design 2 taught us. It draws on the measured run of the [partial-sharing baseline](../../shared/partial-sharing-baseline/) ([results](../../shared/partial-sharing-baseline/test/results/results.md), commit `f39d34e`), the [pilot finding on eager refinement](eager-d2-finding.md), the [design note](design-note.md), the amendments of [spec v0.1](../v0.1.md) and the library's [decisions](../../shared/partial-sharing-baseline/lib/DECISIONS.md). It adds no measurement. The joint write-up, with Design 1 side by side, is the baseline's [findings](../../shared/partial-sharing-baseline/research/findings.md). Like the [rule](../../shared/partial-sharing-baseline/research/rule.md) (item 11), it names no overall winner beyond what each measure shows.

## The design

Design 2 writes each denominator over a shared set of pairwise coprime **pieces** (a coprime base; [notation](coprime-base-notation.md)): a known part, as (piece, valuation) entries in its row, times a remainder not yet refined. Once two denominators have been refined against each other, their gcd and residuals are read from the rows, piece by piece, and only the pieces of equal valuation can cancel in a sum. It was built as a **skeleton**, the plainest correct form, and measured against a **control** of plain reduced rationals (one gcd chain per operation) and against [Design 1](../../08-common-divisors/).

## From eager to on-demand refinement

As first specified, Design 2 refined every new denominator, entered or a result, against the whole base as soon as it was stored, to keep every row complete at all times. That cost up to one gcd per piece of the base for each new denominator. In the pilot (rule step 3; not a measurement):
- the base grew to 2,400–3,200 pieces, mostly private parts of single denominators that never met a partner;
- Design 2 computed 4–11 million gcds per mixed job against the control's 31–47 thousand, 58–161 times the control's gcd work;
- it ran 55–137 times slower than the control.

About 99% of those gcds were the refinement scan; the operations themselves needed about as many as the control ([finding](eager-d2-finding.md)).

By a spec amendment of Oct 29, 2026, approved by the author before any measurement, Design 2 now refines **on demand**. A row holds its known pieces and an unrefined remainder. Entry computes no gcd against the base. Two denominators are refined against each other only when they meet in an addition, a subtraction or a division, and only as far as that meeting needs, with an extra refinement step that the prototype showed to be necessary (without it, $3/4 + 1/8$ is stored as $14/16$). On the same pilot jobs this gave 31–164 thousand gcds, 1.0–3.5 times the control's, and bases of 3–64 pieces. No stored value changed.

## Results against the control

23 verdict jobs of 20,000 operations, 12 interleaved rounds each, at width 64. Ratios are medians per stratum:
- **time** is T(ctl) / T(D2): above 1, Design 2 is faster;
- **gcd work** is G(D2) / G(ctl), the summed bit length of gcd operands: below 1, Design 2 does less;
- **memory** is B(D2) / B(ctl), used bytes per rational at the end of the job: below 1, Design 2 uses less.

| Stratum | Jobs | Time | Gcd work | Memory |
| --- | --- | ---: | ---: | ---: |
| ALL (one shared denominator) | 1–3 | 0.863 | 0.876 | 1.358 |
| P75 | 4–6 | 0.271 | 2.045 | 4.284 |
| P50 | 7–9 | 0.305 | 2.075 | 3.763 |
| P25 | 10–12 | 0.426 | 1.636 | 3.132 |
| NONE (pairwise coprime) | 13–15 | 0.732 | 1.001 | 2.695 |
| LG (large shared factors) | 16–18 | 0.362 | 1.737 | 3.502 |
| CHAINS (results as operands) | 21–25 | 0.458 | 1.212 | 1.760 |

- **Time.** Design 2 lost to the control on **every** job (23 losses). In the partial strata (P75, P50, P25, LG), it took 2.3–3.7 times the control's time; in ALL, about 1.2 times.
- **Gcd work.** It won on 2 jobs (ALL-muldiv 0.69, ALL-mixed 0.88), tied on 5 and lost on 16. In the partial strata it did **1.6–2.1 times** the control's gcd work.
- **Memory.** It won on 3 jobs (ALL-add 0.75, CH-closed 0.77, CH-all 0.75) and lost on 20. In the partial strata it used 3.1–4.3 times the control's memory, and raised the severe-memory-loss flag (ratio ≥ 4.0) on 3 jobs, two in P75 and one in P50.

**Against Design 1** (T(D2) / T(D1), G(D1) / G(D2), B(D1) / B(D2)): Design 2 was slower on 15 of the 23 jobs, with medians of 1.21–1.54 in Design 1's favour in the partial strata, and did about twice Design 1's gcd work in P75 and P50. It was faster on all of NONE (0.779) and used less memory on all of P25 and NONE (1.166, 1.267). The full head-to-head table is in the baseline's [findings](../../shared/partial-sharing-baseline/research/findings.md#head-to-head-design-1-against-design-2).

**Also reported.** Entering denominators in batches rather than all first changed nothing (job 19 against job 20: time 1.001, gcd work and end memory 1.000). At width 32, Design 2 won on gcd work (0.76 and 0.77) and still lost on time (0.60 and 0.90).

## Lessons

1. **Eager completeness was the flaw in the first form.** Keeping the base pairwise coprime across all denominators, at once, did work that almost no operation needed: it cost 58–161 times the control's gcd work. The fix gives a principle that carries over: **no work in advance, no work in the background; each operation computes only what it needs.**
2. **Even on demand, Design 2 did more gcd work than the control where sharing is partial.** A meeting makes one gcd for each piece tested against a remainder, plus the gcd of the two remainders, and a sum then makes one for each piece of equal valuation; the control's addition makes two (design note, cost table). Design 2 did 1.6–2.1 times the control's gcd work in the partial strata. **Larger number sizes are therefore unlikely to rescue it**: wider numbers make each gcd dearer, and Design 2 makes more of them.
3. **At 64 bits, the bookkeeping costs more than the gcds.** Merging rows, multiplying out residuals from pieces on every addition, splitting pieces and rewriting rows all cost time. Even where Design 2 saved gcd work (ALL, by up to a third), it lost on time.
4. **The control was very hard to beat.** Plain reduced rationals, with a same-denominator fast path, beat Design 2 on time on every job. Even the shared-denominator ceiling only tied the control on time.

These are results for a **skeleton**: the design note lists improvements, such as an index limiting which pieces a remainder is tested against, none of which was measured. But lessons 2 and 3 hold for any improvement that keeps the same structure, since the structure makes more gcds, not fewer, and its bookkeeping is what costs.

## Status

**Concluded.** Superseded by the active work on Design 3 in [`09-interning`](../../09-interning/) (to be renumbered 10): whole numbers interned and stored once, with index-equality shortcuts.
