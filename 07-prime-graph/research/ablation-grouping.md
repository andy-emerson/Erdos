# Replacing grouping: each strategy alone

Oct 27, 2026 · Andrew Emerson

The light prototype's grouping (a remembered table of values with identical denominators) helped on one-scale arrays but exploded on unrelated denominators and slowed single values ([first results](light-results.md)). This study removes grouping and adds back one idea at a time. There are no combinations; those are for discussion after this. The full tables are in [`test/ablation/results/ablation.md`](../test/ablation/results/ablation.md) and the code is in [`test/ablation/`](../test/ablation/). An independent review came before the run, and its fixes were re-verified. Every result was checked against Python's `fractions`, and every final result is in lowest terms.

**Baseline: V0′, the control.** Grouping is removed, and every operation works on its operands' subgraphs. Each strategy is V0′ plus one idea:

| Strategy | Idea |
| --- | --- |
| V0 | grouping (the original prototype) |
| V1 | near-free per-value check: equal denominators mean one integer op; one dividing the other means one scaling |
| V2 | bucketing by denominator during column sums |
| V3 | array-level shared denominator D with sparse exceptions; two declared choices of D (lcm, most frequent) |
| V4 | reduction deferred to the end of the expression |
| V5 | shared denominator objects (interned, reference counted) |

V3 can't be separated from deferred reduction, because a shared array denominator is canonical per array, not per value. It should be read against V4 as well.

**Noise floor:** code identical to the control measured 0.98–1.00, so differences within ±2% count as no difference.

## Results

Speed-up over the control, V0′ time / strategy time (total, including entry and final reduction). Above 1, the strategy is faster.

| Workload | V0 grouping | V1 check | V2 buckets | V3 (lcm / freq) | V4 deferred | V5 objects |
| --- | --- | --- | --- | --- | --- | --- |
| W1 decimals, sum | 2.22 | 1.09 | 2.01 | **11.0 / 13.0** | 2.36 | 1.08 |
| W2 decimals, a·b + c | 1.38 | 1.10 | = | **3.42 / 3.33** | 1.47 | 1.12 |
| W3 denominators 1–12, sum | 2.96 | 1.41 | 2.53 | **14.8** / 1.78 | 1.42 | 1.02 |
| W3 denominators 1–12, a·b + c | 1.20 | 1.02 | = | **1.53** / 0.88 | 1.00 | 1.09 |
| W4 unrelated, a·b + c | 0.39 | 0.99 | = | 0.90 / 0.91 | **1.10** | 0.60 |
| W5 single values, add | 0.68 | 0.99 | = | 0.85 / 0.85 | 0.93 | **1.11** |
| W5 single values, mul | 0.64 | 1.00 | = | 0.85 / 0.85 | 0.94 | **1.08** |
| W6 growth chain | 2.03 | 1.25 | = | **6.71 / 6.84** | 1.30 | 1.23 |
| W7 large shared primes, a·b + c | 0.69 | 1.01 | = | 0.94 / 0.88 | 0.98 | 0.85 |
| W8 money, sum | 2.28 | 1.10 | 1.99 | **11.6 / 12.8** | 2.19 | 1.11 |
| W8 money, a·b + c | 1.37 | 1.06 | = | **3.49 / 3.40** | 1.50 | 1.13 |
| W9 90% one denominator, sum | 2.78 | 1.08 | 2.53 | **11.6** / 8.13 | 2.12 | 0.99 |
| W9 90% one denominator, a·b + c | 1.35 | 1.10 | = | 1.04 / **2.20** | 1.37 | 1.10 |
| W10 (a·b + c)·e + f | 1.42 | 1.11 | = | **4.35 / 4.35** | 1.61 | 1.13 |

"=" means the strategy runs the control's code for that job: V2 only changes column sums. The W4 column sum is beyond 64 bits for every strategy.

## Findings

1. **V3, a shared array denominator with sparse exceptions, is far ahead on its own ground.**
   - It is 11–15× faster than the control on one-scale and mostly-one-scale sums, 3.3–6.8× on elementwise work and chains, and 4.35× on the expression chain.
   - It beats V4 by a wide margin everywhere, so the gain is not just its built-in deferred reduction.
   - It loses a little where nothing is shared: single values 0.85, unrelated denominators 0.90, large shared primes 0.88–0.94.
2. **V3's choice of D decides its result on mixed data.** The lcm wins when the lcm is small (W3 sum 14.8 against 1.78). The most frequent denominator wins when the lcm makes products overflow (W9 a·b + c, 2.20 against 1.04). A rule that picks between them, or a D that adapts, is needed.
3. **V4, deferred reduction, helps every array workload by 1.3–2.4×.** It is the only strategy that helps unrelated denominators (1.10), and it costs 6–7% on single values.
4. **V2, bucketing, doubles column sums (2.0–2.5×)** and nothing else. V3 does sums much better.
5. **V5, shared objects, is the only strategy that helps single values (1.08–1.11),** and it uses less memory. It loses on many distinct denominators (W4 0.60, W7 0.85).
6. **V1, the near-free check, gives a small, safe gain (1.0–1.4×) and never loses.** The per-value check was already cheap in the control. The control's cost lies in entry and reduction, not in the check.
7. **Grouping's benefit on arrays was real (1.2–3×), but V3 delivers 4–5× more of it, and grouping is the worst strategy where nothing is shared** (0.39–0.69).
8. **Entry is still the control's largest cost** (about 23 ms of 57 on W1), and the time goes to reducing each value. V3 and V4 cut entry to 4–6 ms because they don't reduce per value. The seen-prime dictionary, the next proposed test, targets the rest of entry.

## For discussion: combinations

These results suggest candidates. They are not decided.
- V3 for arrays, with a rule for D.
- V4 everywhere, inside expressions.
- V5 or V1 for single values.
- V2 is made redundant by V3.
- Grouping can be dropped.

These ideas act on different shapes (arrays, expressions, single values), so they look compatible, but that needs testing.
