# Findings: Design 1, common divisors

Oct 29, 2026 · **Concluded.** What Design 1 taught us. It draws on the measured run of the [partial-sharing baseline](../../partial-sharing-baseline/) ([results](../../partial-sharing-baseline/test/results/results.md), commit `f39d34e`), the [design note](design-note.md), the amendments of [spec v0.1](../v0.1.md) and the library's [decisions](../../partial-sharing-baseline/lib/DECISIONS.md). It adds no measurement. The joint write-up, with Design 2 side by side, is the baseline's [findings](../../partial-sharing-baseline/research/findings.md). Like the [rule](../../partial-sharing-baseline/research/rule.md) (item 11), it names no overall winner beyond what each measure shows.

## The design

Design 1 remembers the gcds it finds between pairs of denominators. Each denominator has a row of the divisors known to divide it, with their residuals. To add two values, it walks the two rows from the top to the largest divisor $c$ they share. If the pair has been compared before, $c$ is the gcd. If not, the gcd is $c \cdot \gcd(d_i / c,\ d_j / c)$, computed on the smaller residuals and then recorded. It was built as a **skeleton**, the plainest correct form, and measured against a **control** of plain reduced rationals (one gcd chain per operation) and against [Design 2](../../09-coprime-base/).

## How it changed before the measured run

The pilot (rule step 3; not a measurement) showed that Design 1's two dense structures do not survive many denominators. Both were made sparse by spec amendments of Oct 29, 2026, approved by the author before any measurement. Neither changes a stored value.

- **The pair table became a pair set.** The dense table, one bit for each pair of denominators, grows with the square of their number: at the roughly 210,000 denominators of a 200,000-operation run, it alone would have reached about 2.8 GB. It became a hash set of the pairs actually compared.
- **The incidence matrix became sparse rows.** The dense denominator-by-divisor bit matrix was 7–53% of Design 1's bytes per rational in the pilot. It became one sparse row per denominator, listing only the divisors known to divide it, with their residuals, under stable divisor ids, so that inserting a divisor shifts no row.

## Results against the control

23 verdict jobs of 20,000 operations, 12 interleaved rounds each, at width 64. Ratios are medians per stratum:
- **time** is T(ctl) / T(D1): above 1, Design 1 is faster;
- **gcd work** is G(D1) / G(ctl), the summed bit length of gcd operands: below 1, Design 1 does less;
- **memory** is B(D1) / B(ctl), used bytes per rational at the end of the job: below 1, Design 1 uses less.

| Stratum | Jobs | Time | Gcd work | Memory |
| --- | --- | ---: | ---: | ---: |
| ALL (one shared denominator) | 1–3 | 0.838 | 0.839 | 1.386 |
| P75 | 4–6 | 0.416 | 0.989 | 3.735 |
| P50 | 7–9 | 0.457 | 0.992 | 3.599 |
| P25 | 10–12 | 0.515 | 0.998 | 3.474 |
| NONE (pairwise coprime) | 13–15 | 0.570 | 0.999 | 3.416 |
| LG (large shared factors) | 16–18 | 0.479 | 0.992 | 3.532 |
| CHAINS (results as operands) | 21–25 | 0.529 | 0.946 | 2.156 |

- **Time.** Design 1 lost to the control on **every** job (23 losses). In the partial strata (P75, P50, P25, LG), it took 1.9–2.4 times the control's time; in ALL, about 1.2 times.
- **Gcd work.** It won on 4 jobs (ALL-muldiv 0.82, ALL-mixed 0.84, CH-closed 0.64, CH-all 0.66) and tied on the other 19. In the partial strata it did 0.989–0.999 of the control's gcd work: a tie in each.
- **Memory.** It won on 2 jobs (ALL-add 0.75, CH-all 0.75) and lost on 21. In the partial strata it used 3.4–3.7 times the control's memory, and raised the severe-memory-loss flag (ratio ≥ 4.0) on 4 jobs, one each in P75, P50, P25 and LG.

**Against Design 2** (T(D2) / T(D1), G(D1) / G(D2), B(D1) / B(D2)): Design 1 was faster on 15 of the 23 jobs, with medians of 1.21–1.54 in the partial strata, and did about half of Design 2's gcd work in P75 and P50 (0.484, 0.478). It was slower on all of NONE (0.779), and used more memory on all of P25 and NONE (1.166, 1.267). The full head-to-head table is in the baseline's [findings](../../partial-sharing-baseline/research/findings.md#head-to-head-design-1-against-design-2).

**Also reported.** Entering denominators in batches rather than all first changed nothing (job 19 against job 20: time 0.998, gcd work and end memory 1.000). At width 32, Design 1 won on gcd work (0.70 and 0.85) and still lost on time (0.64 and 0.85).

## Lessons

1. **At 64 bits, the bookkeeping costs more than the gcds it saves.** The merge of two rows, the pair-set probe, the binary search and the insertion of a new divisor all cost more than the gcd steps they avoid. Even where Design 1 won on gcd work, by up to a third, it lost on time.
2. **The chain of gcds saved almost no gcd work where it was meant to.** Partial sharing was the point, and there Design 1 did about 0.99 of the control's gcd work. A known common divisor shrinks the remaining gcd only a little at these sizes, and the second gcd, $\gcd(\text{sum}, g)$, remains. So **larger number sizes are unlikely to rescue it**: wider numbers make each gcd dearer, but a design that removes no gcd work has nothing to gain from that.
3. **Dense structures do not survive many denominators.** The pair table grows with the square of the number of denominators, and the incidence matrix with denominators times divisors. Both had to become sparse before a full run was possible, and even sparse, Design 1 used 3.4–3.7 times the control's memory with partial sharing.
4. **The control was very hard to beat.** Plain reduced rationals, with a same-denominator fast path, beat Design 1 on time on every job. Even the shared-denominator ceiling only tied the control on time.

These are results for a **skeleton**: the design note lists improvements, such as the index-valued pair table and the lazy parent link, none of which was measured. But lessons 1 and 2 hold for any improvement that keeps the same bookkeeping, since the bookkeeping, not the gcd, is what costs.

## Status

**Concluded.** Superseded by the active work on Design 3 in [`10-interning`](../../10-interning/): whole numbers interned and stored once, with index-equality shortcuts. The index-valued pair table, a deferred variant of Design 1, was not run.
