# Round two: V3 + V4, single values by size, and the seen-prime dictionary

Oct 27, 2026 · Andrew Emerson

This follows [round one](ablation-grouping.md). The baseline is still V0′, the control with grouping removed. Ratios are V0′ / strategy on total time; above 1, the strategy is faster. The noise floor (the control rerun as its own row) was 0.95–1.01. The full tables are in [`test/ablation/results/round2.md`](../test/ablation/results/round2.md). Independent reviews came before the run, and three rounds of fixes were verified. Every result was checked against Python's `fractions`, and every coprime-invariant audit passed.

## A. V3 + V4 together (V34)

| Job | V3 (lcm / freq) | V4 | V34 (lcm / freq) |
| --- | --- | --- | --- |
| W1 decimals, sum | 11.1 / 11.8 | 2.38 | 12.1 / 12.5 |
| W3 denominators 1–12, sum | 14.6 / 1.79 | 1.41 | 14.1 / 2.26 |
| W4 unrelated, a·b + c | 0.91 / 0.92 | 1.13 | 1.05 / 1.06 |
| W6 chain | 6.6 / 6.7 | 1.30 | 6.8 / 6.7 |
| W7 large shared primes | 0.94 / 0.89 | 0.97 | 0.79 / 0.88 |
| W9 90% one denominator, a·b + c | 1.05 / 2.21 | 1.36 | 1.03 / 2.49 |
| W10 (a·b + c)·e + f | 4.4 / 4.4 | 1.62 | 4.4 / 4.5 |

(The other jobs are in the full tables, with the same pattern.)

**Finding:** the combination behaves almost exactly like V3. Adding V4 helps where V3 was weakest:
- unrelated denominators, where it goes from 0.91 to 1.05 and no longer loses;
- the most-frequent rule on mixed data (W3 sum 1.79 to 2.26, W9 a·b + c 2.21 to 2.49).

It hurts once: W7 with the lcm rule, 0.94 to 0.79. Elsewhere it is within noise of V3. The choice of D still matters more than adding V4.

## B. Single values by operand size

V0′ / x on total time; the operation-only ratios are in the full tables.

| Size | V1 | V5 | V6 | 32-bit control (64-bit / 32-bit) |
| --- | --- | --- | --- | --- |
| S8 (7-bit numerators, denominators 1–12) | 0.98–1.02 | **1.10–1.11** | 0.92–0.93 | 1.04–1.06 |
| S15 | 0.96–1.00 | 0.93–0.96 | 0.96–0.98 | 1.06–1.09 |
| S20 | 0.95–1.01 | 0.60–0.68 | 0.98 | 1.09 (add; mul overflows 32 bits) |
| S30 (random denominators up to 2¹⁵) | 0.98–1.00 | 0.70–0.73 | **3.5–3.6** | — |
| S30d (50 shared denominators) | 0.94–1.01 | 0.91–0.94 | 0.96–0.97 | — |

**Findings:**
- **V1 does nothing** at any size. It is within noise or slightly worse, so it is the control with more work, as you suspected.
- **V5 helps only the smallest values** (S8, 1.10; 1.15–1.41 in 32 bits) and hurts as the number of distinct denominators grows (0.60–0.73), because interning costs more than it saves.
- **32-bit words are 4–9% faster than 64-bit words** on total time, and 5–17% on operation time, where the values fit.

## C. The seen-prime dictionary (V6)

| Situation | V0′ / V6 (total) | Why |
| --- | --- | --- |
| few distinct denominators (W1–W6, W8–W10, S8–S20, S30d) | 0.92–1.00 | the dictionary has almost nothing to do and costs a little |
| large denominators of two large primes (W7) | 0.88 | 24 unshared two-prime nodes stay composite, and their checks cost more |
| **many distinct denominators** (S30: 3,512 nodes) | **3.5–3.6** (entry 5.4×) | the control's entry compares each new denominator with every node; the dictionary looks it up |

**Finding:** the dictionary is a **scalability** idea, not a speed-up for small graphs. It costs 0–8% when the graph is small, and wins by more as the graph grows: the control's entry grows with the square of the number of nodes, while the dictionary's stays flat. That is exactly the requirement recorded in the [idea note](idea.md#scalability-requirements-agreed-oct-27). With 3,512 nodes it is already 3.5× faster overall. The small-graph cost and the W7 cost are the targets of the [shared-factor prior art](prior-art-3-sharing.md): a faster small-prime test, and no Pollard's rho at entry.

## Where this leaves the strategies

- **Arrays:** V3, or V34, which adds safety on unrelated denominators. The rule for choosing D is the open item.
- **Single values:** nothing tested beats the control at realistic sizes. V5 helps only tiny values. Smaller words (32-bit) help modestly.
- **Entry:** the dictionary, for scalability, with the prior-art speed-ups to remove its small-graph cost.
- **Dropped:** V1, grouping (V0), V2 (V3 covers it), and V5 except possibly for tiny values.
