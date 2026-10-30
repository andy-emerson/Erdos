# Round three: V4 on V3, and three fixes to the seen-prime dictionary

Oct 28, 2026 · Andrew Emerson

This follows [round two](ablation-round2.md). The rule was fixed and recorded before the first measurement ([round3-rule.md](round3-rule.md)):
- 12 interleaved rounds;
- paired bootstrap 95% intervals;
- an R0 null copy to size the noise band;
- verdicts of better, worse or tie, with ties counted whenever the interval overlaps the band of ±5% or the null spread, if larger.

The full tables are in [`test/ablation/results/round3.md`](../test/ablation/results/round3.md). Independent reviews came before the run. Every result was checked against Python's `fractions`, and every audit passed. The run took 67 minutes.

The rows:

| Row | What it is |
| --- | --- |
| R0 | V3 (array shared denominator with sparse exceptions); control for Q1 |
| R1 | R0 + V4 (deferred reduction) |
| D0 | R0 + the naive seen-prime dictionary at entry; control for Q2 |
| D1 | D0 + branch-free small-prime tests in blocks of 8 |
| D2 | D0 + no Pollard rho at entry (composite leftovers factored by batch gcd and rho in a timed idle pass) |
| D3 | D0 + a product check for large leftovers |

## Q1: is V3 + V4 better than V3? **Yes,** by the pre-registered rule.

Over all 60 judged pairs, R1 was better on 24, tied on 35 and worse on 1.
- **Better** (7–35% faster): mixed denominators (M50, M75, M95 with denominators 1–1000: 1.13–1.25), unrelated denominators (U8 1.29–1.35, U32 1.15–1.17, W4 1.17–1.18), the most-frequent rule on mixed data (W3 1.07–1.28, W9 1.12, K8-mix 1.15), and single values sharing denominators (S15–S30d, 1.07–1.18).
- **Tied:** every one-scale decimal workload (W1, W2, W6, W8, W10, K8-dec). V4 adds nothing there, and costs nothing.
- **Worse:** once, W7 with the lcm rule (0.85).

**Keep V4 on top of V3.**

## Q2: do the dictionary fixes help, and do they reach parity with V3 alone?

| Fix | Against the naive dictionary (D0) | Parity with V3 alone (R0) on the other workloads |
| --- | --- | --- |
| D1, branch-free small-prime tests | **better** on every Q2 workload: 1.10–1.43 | no |
| D2, no rho at entry | **no difference** (all ties) | no |
| D3, product check | **worse** (0.78–0.92 on three workloads) | no |

**Keep D1. Drop D2 and D3.** D2's deferral saved entry time on P4000 (entry 36 ms against 140), but its idle pass spent it all again (108 ms). D3's product check costs more than the cheap exact-division loop it replaces.

**The dictionary against V3 alone,** with D1 as the best fix:

| Situation | V3 / dictionary (above 1: the dictionary is faster) |
| --- | --- |
| many distinct denominators: N10k, N30k | **23×, 112×** |
| N1k, P1024, P4000 | 1.19, 1.80, 2.00 |
| decimal elementwise and chains (W2, W6, W8 fma) | 0.89–0.93 |
| two-large-prime denominators: W7, P64 | 0.89–0.92 (the naive dictionary was 0.64–0.69) |
| few distinct denominators of medium size: N100 | 0.68 |
| everything else | parity |

So the dictionary is a large win at scale: 23× at 10,000 distinct denominators and 112× at 30,000, because V3's entry grows with the square of the node count. D1 brings it much closer on large-prime data. **It still misses parity in two places,** and the entry breakdown locates both:
1. **Decimal elementwise work (about −10%).** Entry times are equal (17.3 ms against 17.4 on W2), so the loss is in the operations, not the dictionary. The D rows' operation path costs about 10% more than R0's on the same data, which looks like an implementation cost in how the D rows hook V3's operations, not part of the idea. This is the first thing to find and fix.
2. **About 100 distinct medium denominators (N100, 0.68).** Here entry itself is slower (24.8 ms against 15.6): trial division of each new denominator costs more than a gcd scan over only about 180 nodes.

## Where this leaves the design

- **Arrays:** V3 + V4.
- **Entry:** the dictionary, with branch-free small-prime tests (D1).
- **Next:** find the about 10% operation cost in the D rows, and cheapen entry for small node counts. Both are needed to reach parity everywhere, the bar set for keeping the dictionary without a conditional routine.
