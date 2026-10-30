# Prime graph: ablation, round 3

Generated 2026-10-28 01:09 by `round3_bench.py` on Intel(R) Xeon(R) Processor; clang 18 `-std=c2x -O3 -march=native`.  12 interleaved rounds; each invocation runs 2 warm reps (fewer where a workload says so: N10k and N30k run 1) and reports the minimum of each phase.  Tables give medians over rounds; times in ms.

Rule pre-registered in `research/round3-rule.md`; this run started at 2026-10-28 00:02:31 and measured first at 2026-10-28 00:04:57.

**Decision rule (pre-registered in research/round3-rule.md before the measured run; the harness refuses to start a
measured run unless that file holds this text, and writes it into results/round3.md before the first measurement).**

1. Each job runs 12 interleaved rounds.  In every round each row of the job runs once, in an order rotated per
   round, so the rows of one round are paired.  A row's time in a round is its **total** (entry, idle pass, op and
   final reduction; the minimum over the invocation's warm reps).
2. A comparison "X against Y" is the ratio **T(Y) / T(X)**, where T is the median of the row's per-round totals.
   Above 1, X is faster.  Its 95% interval [lo, hi] is the 2.5th-97.5th percentile of the same ratio over 10,000
   bootstrap resamples of the rounds (rounds drawn with replacement, the same draw for X and Y, so the pairing is
   kept; seeded).
3. **Null control:** R0 against its own second copy (the same binary run as another row) on the same job and rule.
   Its interval sizes the noise: the job's **null spread** is max(1 - lo, hi - 1) over the null interval.
4. **Band:** h = max(5%, null spread) for that job and rule; the band is [1 - h, 1 + h].
5. **Per job and rule** (array jobs: lcm and freq separately; single-value jobs: one plain sub-row; the Q2-only
   workloads N* and P*: lcm only):
   - **better** if lo > 1 + h; **worse** if hi < 1 - h; otherwise **tie** (the interval overlaps the band);
   - **parity** (X at parity with Y, or faster) if lo >= 1 - h;
   - **n/a** (not judged) if either row failed a check or was unable, or the null control is missing.
6. **Aims.** Each workload is aimed at Q1 (M*, K8-*, U*), at Q2 (N*, P*), or at neither (the existing W* and S*).
7. **Q1, the overall answer** (R1 against R0, over the job-rule pairs of the Q1-aimed workloads): **better** if R1 is
   better on at least one and worse on none; **worse** if it is worse on at least one and better on none; otherwise
   **mixed** (including all ties).
8. **Q2, the overall answers, for each of D1, D2, D3:**
   (a) **against D0** (Dk against D0 over the job-rule pairs of the Q2-aimed workloads), by the rule of 7:
       **better**, **worse** or **mixed**;
   (b) **parity with R0**: **yes** if Dk is at parity with R0 (5) on every judged job-rule pair of the workloads not
       aimed at Q2 (the existing ones and the Q1-aimed ones), else **no**, with the pairs that fail listed.
   n/a pairs are left out and counted.  No other comparison carries a verdict; the tables show the rest (D0 against
   R0, all workloads in every comparison) for information.


## Rows

- **R0**: R0 = V3 with the old entry (V0' entry: each new denominator gcd-refined against every live node, then pg_factor on every node; no dictionary).  Control for Q1, reference for Q2
- **R1**: R1 = V3 + V4 (round 2's V34: exceptions entered unreduced, V4's deferred ops on the graph path, one timed final reduction)
- **D0**: D0 = V3 + V6's seen-prime dictionary as the entry (round 2's V6 entry code, unchanged).  Control for Q2
- **D1**: D1 = D0 with the small-prime table in branch-free blocks of 8: the same multiply-by-inverse test per prime (D0 already uses it), but 8 primes per block without branches (vectorised), the adaptive stop once per block instead of a multiply and a branch per prime; everything else (pg_factor before rho) as D0
- **D2**: D2 = D0 + no rho at entry: Miller-Rabin stays; a composite large leftover is only appended (deduplicated by hash); the timed idle pass (reported apart, included in total) runs a batch gcd over the leftovers (product and remainder tree), divides pieces still composite by the known large primes, sends only what is still composite to rho, and merges their primes through the dictionary
- **D3**: D3 = D0 + a product check of large leftovers: a product forest of the known large primes; one remainder of the product and one gcd per new leftover, the forest searched only on a hit (replaces D0's exact division by every known large prime)
- **null**: R0 again, the same binary, as its own row (rows `r0-<rule>-null`).

Array jobs run each row under both rules for V3's array denominator D (`-lcm`: the lcm of the array's denominators when below 2^63, else the most frequent; `-freq`: the most frequent).  Single-value jobs (S8-S30d) enter every value as an exception (no D), so they have one plain sub-row.

## Answers (rule 7 and 8)

**Q1: is R1 better than R0?**  **better** over 26 job-rule pairs of the Q1-aimed workloads.  Better on: M50-12 sum freq, M50-12 fma freq, M75-12 sum freq, M75-12 fma freq, M50-1k fma lcm, M50-1k fma freq, M75-1k fma lcm, M75-1k fma freq, M95-1k fma lcm, M95-1k fma freq, K8-mix chain freq, U8 fma lcm, U8 fma freq, U32 fma freq.  Worse on: none.

**Q2:**

| row | (a) against D0, Q2-aimed workloads | better on | worse on | (b) parity with R0, other workloads | pairs without parity |
|---|---|---|---|---|---|
| D1 | **better** (8 pairs) | N10k fma lcm, N30k fma lcm, P64 fma lcm, P256 fma lcm, P1024 fma lcm, P4000 fma lcm | none | **no** (60 judged) | W1 sum lcm, W2 fma lcm, W2 fma freq, W3 sum freq, W3 fma lcm, W6 chain lcm, W6 chain freq, W7 fma lcm, W7 fma freq, W8 sum lcm, W8 fma lcm, W8 fma freq, W9 sum freq, W10 fma2 lcm, W10 fma2 freq, S8 mul plain, S15 add plain, S20 add plain, S30d add plain, S30d mul plain, M75-12 sum freq, M95-12 sum lcm, M95-12 sum freq, K8-dec chain freq, K8-mix chain lcm |
| D2 | **mixed** (8 pairs) | none | none | **no** (60 judged) | W2 fma lcm, W2 fma freq, W3 sum freq, W6 chain lcm, W6 chain freq, W7 fma lcm, W7 fma freq, W8 fma lcm, W8 fma freq, W10 fma2 lcm, W10 fma2 freq, S8 mul plain, S30d add plain, S30d mul plain, M95-12 sum lcm, M95-12 fma freq, K8-dec chain lcm, K8-dec chain freq, K8-mix chain lcm |
| D3 | **worse** (8 pairs) | none | N30k fma lcm, P256 fma lcm, P1024 fma lcm | **no** (60 judged) | W1 sum lcm, W2 fma lcm, W2 fma freq, W6 chain lcm, W6 chain freq, W7 fma lcm, W7 fma freq, W8 fma lcm, W8 fma freq, W9 sum freq, W10 fma2 lcm, W10 fma2 freq, S8 add plain, S8 mul plain, S30d add plain, S30d mul plain, M50-12 sum freq, M75-12 sum freq, M95-12 sum lcm, K8-dec chain freq, K8-mix chain lcm |

## Noise

Null control (R0 against its own second copy) over 68 job-rule pairs: point ratios 0.94 to 1.11 (median 1.00); null spread up to 18.7%, above the 5% floor on 18 of them (their band is the wider one).

## Q1: is R1 (V3 + V4) better than R0 (V3)?

R0 / R1 on total time (above 1, R1 is faster), its 95% interval, the band half-width h and the verdict.

**Per-pair counts, all workloads (information; the answer above uses the Q1-aimed ones):** better 24, tie 35, worse 1.

| job | rule | aim | R0 / R1 [95%] | h | verdict |
|---|---|---|---|---:|---|
| W1 sum | lcm | existing | 0.88 [0.76, 1.00] | 18.4% | tie |
| W1 sum | freq | existing | 0.99 [0.92, 1.13] | 15.3% | tie |
| W2 fma | lcm | existing | 1.01 [0.98, 1.05] | 5.0% | tie |
| W2 fma | freq | existing | 1.00 [0.98, 1.02] | 5.0% | tie |
| W3 sum | lcm | existing | 1.17 [1.01, 1.29] | 15.8% | tie |
| W3 sum | freq | existing | 1.28 [1.26, 1.30] | 5.0% | better |
| W3 fma | lcm | existing | 0.94 [0.89, 0.97] | 5.0% | tie |
| W3 fma | freq | existing | 1.07 [1.05, 1.09] | 5.0% | better |
| W4 fma | lcm | existing | 1.18 [1.14, 1.29] | 11.5% | better |
| W4 fma | freq | existing | 1.17 [1.14, 1.21] | 5.0% | better |
| W6 chain | lcm | existing | 1.02 [0.97, 1.04] | 5.0% | tie |
| W6 chain | freq | existing | 1.02 [1.00, 1.03] | 5.0% | tie |
| W7 fma | lcm | existing | 0.85 [0.84, 0.86] | 5.0% | worse |
| W7 fma | freq | existing | 1.02 [1.00, 1.06] | 5.0% | tie |
| W8 sum | lcm | existing | 0.97 [0.86, 1.08] | 13.8% | tie |
| W8 sum | freq | existing | 0.95 [0.81, 1.16] | 18.7% | tie |
| W8 fma | lcm | existing | 0.99 [0.97, 1.03] | 5.0% | tie |
| W8 fma | freq | existing | 1.00 [0.97, 1.02] | 5.0% | tie |
| W9 sum | lcm | existing | 1.06 [0.99, 1.11] | 12.2% | tie |
| W9 sum | freq | existing | 1.05 [0.97, 1.10] | 7.4% | tie |
| W9 fma | lcm | existing | 0.98 [0.97, 0.99] | 5.0% | tie |
| W9 fma | freq | existing | 1.12 [1.09, 1.19] | 6.4% | better |
| W10 fma2 | lcm | existing | 1.00 [0.99, 1.03] | 5.7% | tie |
| W10 fma2 | freq | existing | 1.01 [0.98, 1.04] | 5.0% | tie |
| S8 add | plain | existing | 1.02 [0.99, 1.04] | 5.0% | tie |
| S8 mul | plain | existing | 1.01 [0.99, 1.05] | 5.0% | tie |
| S15 add | plain | existing | 1.07 [1.05, 1.08] | 5.0% | better |
| S15 mul | plain | existing | 1.08 [1.06, 1.10] | 5.0% | better |
| S20 add | plain | existing | 1.07 [1.03, 1.09] | 5.0% | tie |
| S20 mul | plain | existing | 1.10 [1.09, 1.13] | 5.0% | better |
| S30 add | plain | existing | 1.05 [1.04, 1.08] | 5.0% | tie |
| S30 mul | plain | existing | 1.06 [1.04, 1.07] | 5.0% | tie |
| S30d add | plain | existing | 1.15 [1.13, 1.17] | 5.0% | better |
| S30d mul | plain | existing | 1.18 [1.17, 1.20] | 5.0% | better |
| M50-12 sum | lcm | Q1 | 1.14 [0.98, 1.23] | 11.8% | tie |
| M50-12 sum | freq | Q1 | 1.16 [1.13, 1.24] | 8.6% | better |
| M50-12 fma | lcm | Q1 | 0.99 [0.95, 1.01] | 5.0% | tie |
| M50-12 fma | freq | Q1 | 1.31 [1.29, 1.33] | 5.0% | better |
| M75-12 sum | lcm | Q1 | 1.11 [1.07, 1.19] | 9.1% | tie |
| M75-12 sum | freq | Q1 | 1.09 [1.06, 1.13] | 5.0% | better |
| M75-12 fma | lcm | Q1 | 1.01 [1.00, 1.02] | 5.0% | tie |
| M75-12 fma | freq | Q1 | 1.15 [1.12, 1.18] | 5.0% | better |
| M95-12 sum | lcm | Q1 | 1.00 [0.89, 1.12] | 7.1% | tie |
| M95-12 sum | freq | Q1 | 1.10 [0.96, 1.20] | 14.1% | tie |
| M95-12 fma | lcm | Q1 | 0.96 [0.94, 0.97] | 5.0% | tie |
| M95-12 fma | freq | Q1 | 1.06 [1.04, 1.10] | 5.0% | tie |
| M50-1k fma | lcm | Q1 | 1.23 [1.21, 1.25] | 5.0% | better |
| M50-1k fma | freq | Q1 | 1.25 [1.22, 1.27] | 5.0% | better |
| M75-1k fma | lcm | Q1 | 1.25 [1.19, 1.27] | 5.0% | better |
| M75-1k fma | freq | Q1 | 1.25 [1.24, 1.28] | 5.0% | better |
| M95-1k fma | lcm | Q1 | 1.14 [1.10, 1.19] | 6.4% | better |
| M95-1k fma | freq | Q1 | 1.13 [1.09, 1.15] | 6.2% | better |
| K8-dec chain | lcm | Q1 | 1.00 [0.97, 1.02] | 5.0% | tie |
| K8-dec chain | freq | Q1 | 1.00 [0.98, 1.02] | 5.0% | tie |
| K8-mix chain | lcm | Q1 | 1.02 [1.00, 1.03] | 5.0% | tie |
| K8-mix chain | freq | Q1 | 1.15 [1.14, 1.18] | 5.0% | better |
| U8 fma | lcm | Q1 | 1.29 [1.27, 1.32] | 5.0% | better |
| U8 fma | freq | Q1 | 1.35 [1.33, 1.41] | 5.3% | better |
| U32 fma | lcm | Q1 | 1.15 [1.03, 1.17] | 5.0% | tie |
| U32 fma | freq | Q1 | 1.17 [1.16, 1.19] | 5.0% | better |
| N100 fma | lcm | Q2 | not run | 5.0% | not run |
| N1k fma | lcm | Q2 | not run | 5.0% | not run |
| N10k fma | lcm | Q2 | not run | 5.0% | not run |
| N30k fma | lcm | Q2 | not run | 5.0% | not run |
| P64 fma | lcm | Q2 | not run | 5.0% | not run |
| P256 fma | lcm | Q2 | not run | 5.0% | not run |
| P1024 fma | lcm | Q2 | not run | 5.0% | not run |
| P4000 fma | lcm | Q2 | not run | 5.0% | not run |

## Q2: do D1-D3 beat D0, and do they reach parity with R0?

Left: D0 / Dk on total time (above 1, Dk is faster than D0).  Right: R0 / Dk (above 1, Dk is faster than R0; parity = lo >= 1 - h, marked when the verdict is a tie).  Each cell: the ratio [95% interval], verdict.

Per-pair counts over all workloads, for information (the answers above use the aimed subsets).

**Against D0:** D1 better 8, tie 60, worse 0; D2 better 0, tie 68, worse 0; D3 better 0, tie 65, worse 3.

**Against R0:** D0 better 12, tie 45, worse 11 (parity, lo >= 1 - h, on 42 of 68); D1 better 13, tie 45, worse 10 (parity, lo >= 1 - h, on 41 of 68); D2 better 9, tie 48, worse 11 (parity, lo >= 1 - h, on 46 of 68); D3 better 11, tie 44, worse 13 (parity, lo >= 1 - h, on 44 of 68).

| job | rule | aim | h | D0 / D1 | D0 / D2 | D0 / D3 | R0 / D0 | R0 / D1 | R0 / D2 | R0 / D3 |
|---|---|---|---:|---|---|---|---|---|---|---|
| W1 sum | lcm | existing | 18.4% | 1.09 [0.90, 1.14] tie | 1.08 [0.88, 1.15] tie | 1.01 [0.79, 1.12] tie | 0.89 [0.83, 1.05] tie (parity) | 0.97 [0.78, 1.06] tie | 0.96 [0.83, 1.05] tie (parity) | 0.89 [0.80, 1.00] tie |
| W1 sum | freq | existing | 15.3% | 1.02 [0.95, 1.15] tie | 1.12 [1.05, 1.25] tie | 1.11 [1.01, 1.23] tie | 1.02 [0.91, 1.20] tie (parity) | 1.04 [0.96, 1.25] tie (parity) | 1.14 [1.07, 1.35] tie (parity) | 1.13 [1.02, 1.33] tie (parity) |
| W2 fma | lcm | existing | 5.0% | 1.00 [0.98, 1.02] tie | 1.01 [0.97, 1.02] tie | 1.01 [0.99, 1.03] tie | 0.89 [0.88, 0.92] worse | 0.89 [0.87, 0.92] worse | 0.89 [0.88, 0.93] worse | 0.90 [0.88, 0.93] worse |
| W2 fma | freq | existing | 5.0% | 1.00 [0.96, 1.04] tie | 0.99 [0.98, 1.03] tie | 0.99 [0.97, 1.03] tie | 0.90 [0.86, 0.91] worse | 0.90 [0.87, 0.90] worse | 0.89 [0.88, 0.91] worse | 0.89 [0.87, 0.91] worse |
| W3 sum | lcm | existing | 15.8% | 1.07 [0.99, 1.15] tie | 1.15 [1.01, 1.22] tie | 1.18 [0.98, 1.28] tie | 1.12 [1.04, 1.25] tie (parity) | 1.20 [1.09, 1.29] tie (parity) | 1.29 [1.11, 1.36] tie (parity) | 1.33 [1.10, 1.41] tie (parity) |
| W3 sum | freq | existing | 5.0% | 0.97 [0.96, 0.99] tie | 0.99 [0.97, 1.01] tie | 1.01 [1.00, 1.02] tie | 0.97 [0.96, 0.99] tie (parity) | 0.94 [0.93, 0.97] tie | 0.96 [0.93, 0.99] tie | 0.98 [0.97, 1.00] tie (parity) |
| W3 fma | lcm | existing | 5.0% | 0.99 [0.91, 1.04] tie | 1.00 [0.98, 1.03] tie | 1.01 [0.96, 1.04] tie | 1.00 [0.96, 1.03] tie (parity) | 0.99 [0.90, 1.04] tie | 0.99 [0.97, 1.03] tie (parity) | 1.00 [0.96, 1.03] tie (parity) |
| W3 fma | freq | existing | 5.0% | 1.00 [0.98, 1.02] tie | 0.98 [0.96, 1.01] tie | 0.98 [0.94, 1.01] tie | 1.02 [1.00, 1.03] tie (parity) | 1.01 [0.99, 1.03] tie (parity) | 1.00 [0.97, 1.02] tie (parity) | 1.00 [0.96, 1.01] tie (parity) |
| W4 fma | lcm | existing | 11.5% | 1.00 [0.97, 1.01] tie | 1.00 [0.98, 1.01] tie | 1.00 [0.98, 1.02] tie | 1.05 [1.02, 1.15] tie (parity) | 1.04 [1.00, 1.15] tie (parity) | 1.04 [1.01, 1.14] tie (parity) | 1.05 [1.01, 1.15] tie (parity) |
| W4 fma | freq | existing | 5.0% | 1.00 [0.98, 1.02] tie | 0.99 [0.98, 1.00] tie | 0.99 [0.98, 1.01] tie | 1.06 [1.03, 1.09] tie (parity) | 1.05 [1.03, 1.09] tie (parity) | 1.04 [1.01, 1.08] tie (parity) | 1.05 [1.02, 1.09] tie (parity) |
| W6 chain | lcm | existing | 5.0% | 1.01 [0.96, 1.04] tie | 1.01 [0.97, 1.04] tie | 1.01 [0.97, 1.02] tie | 0.91 [0.88, 0.95] tie | 0.91 [0.88, 0.94] worse | 0.91 [0.88, 0.94] worse | 0.92 [0.87, 0.94] worse |
| W6 chain | freq | existing | 5.0% | 1.02 [0.99, 1.04] tie | 1.01 [0.98, 1.04] tie | 1.01 [0.98, 1.04] tie | 0.92 [0.90, 0.94] worse | 0.93 [0.91, 0.96] tie | 0.92 [0.90, 0.95] tie | 0.93 [0.90, 0.95] worse |
| W7 fma | lcm | existing | 5.0% | 1.35 [1.34, 1.38] better | 0.97 [0.96, 0.99] tie | 0.95 [0.94, 0.97] tie | 0.66 [0.65, 0.67] worse | 0.89 [0.88, 0.90] worse | 0.64 [0.63, 0.65] worse | 0.63 [0.62, 0.64] worse |
| W7 fma | freq | existing | 5.0% | 1.33 [1.30, 1.37] better | 0.98 [0.97, 1.00] tie | 0.95 [0.90, 0.98] tie | 0.69 [0.67, 0.71] worse | 0.92 [0.89, 0.95] worse | 0.68 [0.66, 0.69] worse | 0.65 [0.62, 0.67] worse |
| W8 sum | lcm | existing | 13.8% | 0.98 [0.86, 1.06] tie | 1.07 [0.94, 1.19] tie | 1.03 [0.88, 1.19] tie | 0.93 [0.83, 1.05] tie | 0.91 [0.82, 1.02] tie | 0.99 [0.89, 1.14] tie (parity) | 0.96 [0.86, 1.08] tie (parity) |
| W8 sum | freq | existing | 18.7% | 0.99 [0.93, 1.11] tie | 1.07 [0.98, 1.16] tie | 1.07 [0.97, 1.19] tie | 0.99 [0.83, 1.16] tie (parity) | 0.98 [0.86, 1.19] tie (parity) | 1.06 [0.91, 1.21] tie (parity) | 1.05 [0.95, 1.18] tie (parity) |
| W8 fma | lcm | existing | 5.0% | 1.00 [0.98, 1.01] tie | 1.01 [0.98, 1.03] tie | 1.01 [0.99, 1.02] tie | 0.89 [0.86, 0.93] worse | 0.89 [0.86, 0.92] worse | 0.90 [0.86, 0.94] worse | 0.90 [0.87, 0.93] worse |
| W8 fma | freq | existing | 5.0% | 0.99 [0.97, 1.04] tie | 0.99 [0.97, 1.02] tie | 1.00 [0.95, 1.03] tie | 0.91 [0.87, 0.93] worse | 0.90 [0.87, 0.93] worse | 0.90 [0.87, 0.92] worse | 0.91 [0.85, 0.92] worse |
| W9 sum | lcm | existing | 12.2% | 1.02 [0.96, 1.08] tie | 1.04 [0.91, 1.12] tie | 1.03 [0.96, 1.10] tie | 1.03 [0.97, 1.09] tie (parity) | 1.05 [0.98, 1.13] tie (parity) | 1.07 [0.95, 1.14] tie (parity) | 1.06 [0.99, 1.12] tie (parity) |
| W9 sum | freq | existing | 7.4% | 1.00 [0.93, 1.07] tie | 1.04 [0.97, 1.09] tie | 1.02 [0.93, 1.11] tie | 0.94 [0.87, 0.99] tie | 0.93 [0.88, 1.01] tie | 0.97 [0.94, 1.00] tie (parity) | 0.96 [0.89, 1.01] tie |
| W9 fma | lcm | existing | 5.0% | 0.98 [0.95, 1.01] tie | 1.00 [0.99, 1.01] tie | 0.99 [0.98, 1.00] tie | 1.02 [1.01, 1.03] tie (parity) | 1.00 [0.97, 1.03] tie (parity) | 1.02 [1.01, 1.03] tie (parity) | 1.01 [1.00, 1.02] tie (parity) |
| W9 fma | freq | existing | 6.4% | 1.00 [0.99, 1.02] tie | 0.99 [0.98, 1.02] tie | 0.99 [0.98, 1.02] tie | 0.98 [0.95, 1.04] tie (parity) | 0.98 [0.95, 1.03] tie (parity) | 0.97 [0.95, 1.03] tie (parity) | 0.97 [0.95, 1.04] tie (parity) |
| W10 fma2 | lcm | existing | 5.7% | 1.00 [0.97, 1.02] tie | 1.00 [0.98, 1.01] tie | 1.00 [0.98, 1.02] tie | 0.93 [0.92, 0.96] tie | 0.93 [0.92, 0.95] tie | 0.93 [0.92, 0.96] tie | 0.93 [0.91, 0.95] tie |
| W10 fma2 | freq | existing | 5.0% | 1.00 [0.98, 1.02] tie | 1.02 [1.00, 1.04] tie | 1.00 [0.98, 1.01] tie | 0.94 [0.92, 0.96] tie | 0.93 [0.91, 0.96] tie | 0.95 [0.94, 0.98] tie | 0.93 [0.91, 0.96] tie |
| S8 add | plain | existing | 5.0% | 1.02 [1.00, 1.04] tie | 1.01 [0.99, 1.02] tie | 1.00 [0.97, 1.02] tie | 0.96 [0.95, 0.99] tie | 0.98 [0.97, 1.01] tie (parity) | 0.97 [0.96, 1.00] tie (parity) | 0.97 [0.95, 0.99] tie |
| S8 mul | plain | existing | 5.0% | 1.01 [0.98, 1.02] tie | 1.00 [0.99, 1.01] tie | 0.99 [0.97, 1.01] tie | 0.96 [0.94, 1.00] tie | 0.97 [0.94, 1.00] tie | 0.96 [0.94, 1.00] tie | 0.96 [0.93, 0.99] tie |
| S15 add | plain | existing | 5.0% | 0.98 [0.93, 1.00] tie | 1.00 [0.98, 1.01] tie | 0.99 [0.97, 1.01] tie | 1.00 [0.98, 1.01] tie (parity) | 0.97 [0.93, 1.01] tie | 0.99 [0.99, 1.01] tie (parity) | 0.99 [0.96, 1.00] tie (parity) |
| S15 mul | plain | existing | 5.0% | 1.00 [0.95, 1.02] tie | 1.01 [0.99, 1.02] tie | 1.00 [0.98, 1.02] tie | 1.00 [0.99, 1.02] tie (parity) | 1.01 [0.95, 1.02] tie (parity) | 1.01 [1.00, 1.02] tie (parity) | 1.00 [0.98, 1.02] tie (parity) |
| S20 add | plain | existing | 5.0% | 1.00 [0.94, 1.00] tie | 0.99 [0.96, 1.00] tie | 0.99 [0.98, 1.00] tie | 1.01 [0.98, 1.02] tie (parity) | 1.00 [0.92, 1.02] tie | 0.99 [0.97, 1.00] tie (parity) | 0.99 [0.98, 1.01] tie (parity) |
| S20 mul | plain | existing | 5.0% | 1.00 [0.98, 1.03] tie | 1.02 [1.00, 1.05] tie | 1.03 [1.00, 1.07] tie | 1.00 [0.97, 1.02] tie (parity) | 1.00 [0.98, 1.02] tie (parity) | 1.02 [1.01, 1.04] tie (parity) | 1.03 [1.01, 1.05] tie (parity) |
| S30 add | plain | existing | 5.0% | 1.01 [0.99, 1.04] tie | 0.99 [0.96, 1.02] tie | 0.99 [0.98, 1.03] tie | 3.39 [3.30, 3.48] better | 3.42 [3.38, 3.52] better | 3.36 [3.27, 3.46] better | 3.38 [3.33, 3.50] better |
| S30 mul | plain | existing | 5.0% | 1.00 [0.96, 1.04] tie | 0.99 [0.98, 1.03] tie | 1.01 [0.99, 1.05] tie | 3.55 [3.41, 3.60] better | 3.54 [3.41, 3.62] better | 3.52 [3.48, 3.57] better | 3.58 [3.55, 3.61] better |
| S30d add | plain | existing | 5.0% | 0.98 [0.97, 1.01] tie | 0.96 [0.94, 1.00] tie | 0.98 [0.97, 1.01] tie | 0.93 [0.90, 0.94] worse | 0.91 [0.89, 0.92] worse | 0.89 [0.87, 0.93] worse | 0.91 [0.89, 0.93] worse |
| S30d mul | plain | existing | 5.0% | 1.00 [0.99, 1.03] tie | 1.00 [0.98, 1.04] tie | 1.00 [0.95, 1.03] tie | 0.94 [0.91, 0.95] tie | 0.94 [0.92, 0.95] tie | 0.94 [0.93, 0.96] tie | 0.94 [0.89, 0.96] tie |
| M50-12 sum | lcm | Q1 | 11.8% | 1.02 [0.92, 1.08] tie | 1.05 [0.97, 1.09] tie | 1.00 [0.85, 1.14] tie | 1.19 [0.99, 1.26] tie (parity) | 1.21 [0.93, 1.30] tie (parity) | 1.25 [0.99, 1.32] tie (parity) | 1.19 [0.90, 1.30] tie (parity) |
| M50-12 sum | freq | Q1 | 8.6% | 0.99 [0.98, 1.08] tie | 1.01 [0.97, 1.09] tie | 0.99 [0.93, 1.09] tie | 0.98 [0.91, 1.07] tie | 0.97 [0.94, 1.07] tie (parity) | 0.98 [0.94, 1.08] tie (parity) | 0.97 [0.89, 1.07] tie |
| M50-12 fma | lcm | Q1 | 5.0% | 1.00 [0.99, 1.08] tie | 0.99 [0.99, 1.08] tie | 0.99 [0.98, 1.07] tie | 1.04 [0.96, 1.05] tie (parity) | 1.04 [1.03, 1.06] tie (parity) | 1.03 [1.03, 1.04] tie (parity) | 1.03 [1.02, 1.04] tie (parity) |
| M50-12 fma | freq | Q1 | 5.0% | 1.01 [1.00, 1.10] tie | 1.00 [0.99, 1.08] tie | 1.00 [0.96, 1.08] tie | 1.01 [0.93, 1.03] tie | 1.03 [1.01, 1.04] tie (parity) | 1.02 [0.98, 1.03] tie (parity) | 1.01 [0.97, 1.03] tie (parity) |
| M75-12 sum | lcm | Q1 | 9.1% | 0.99 [0.95, 1.08] tie | 1.01 [0.96, 1.13] tie | 0.99 [0.93, 1.08] tie | 1.08 [1.03, 1.12] tie (parity) | 1.08 [1.03, 1.17] tie (parity) | 1.09 [1.03, 1.23] tie (parity) | 1.07 [0.99, 1.15] tie (parity) |
| M75-12 sum | freq | Q1 | 5.0% | 1.00 [0.99, 1.04] tie | 1.01 [1.00, 1.06] tie | 1.01 [0.96, 1.06] tie | 0.96 [0.92, 0.99] tie | 0.96 [0.93, 0.99] tie | 0.97 [0.95, 1.00] tie (parity) | 0.97 [0.91, 1.00] tie |
| M75-12 fma | lcm | Q1 | 5.0% | 1.01 [1.00, 1.02] tie | 1.00 [0.98, 1.01] tie | 0.98 [0.97, 1.00] tie | 1.03 [1.01, 1.04] tie (parity) | 1.04 [1.03, 1.05] tie (parity) | 1.03 [1.00, 1.04] tie (parity) | 1.02 [1.00, 1.03] tie (parity) |
| M75-12 fma | freq | Q1 | 5.0% | 1.03 [1.01, 1.05] tie | 1.01 [1.00, 1.03] tie | 1.01 [1.00, 1.04] tie | 0.97 [0.95, 0.99] tie | 1.00 [0.99, 1.02] tie (parity) | 0.98 [0.97, 1.00] tie (parity) | 0.98 [0.97, 1.01] tie (parity) |
| M95-12 sum | lcm | Q1 | 7.1% | 1.04 [0.92, 1.10] tie | 1.11 [0.94, 1.15] tie | 1.06 [0.87, 1.07] tie | 0.97 [0.95, 1.10] tie (parity) | 1.02 [0.90, 1.11] tie | 1.08 [0.91, 1.14] tie | 1.03 [0.86, 1.09] tie |
| M95-12 sum | freq | Q1 | 14.1% | 0.99 [0.90, 1.03] tie | 1.00 [0.93, 1.04] tie | 1.00 [0.93, 1.05] tie | 1.04 [0.92, 1.14] tie (parity) | 1.03 [0.85, 1.15] tie | 1.03 [0.88, 1.15] tie (parity) | 1.03 [0.88, 1.17] tie (parity) |
| M95-12 fma | lcm | Q1 | 5.0% | 1.00 [0.99, 1.01] tie | 1.00 [0.99, 1.01] tie | 1.00 [0.98, 1.01] tie | 1.01 [0.99, 1.02] tie (parity) | 1.00 [0.99, 1.02] tie (parity) | 1.00 [0.99, 1.01] tie (parity) | 1.00 [0.99, 1.01] tie (parity) |
| M95-12 fma | freq | Q1 | 5.0% | 0.99 [0.98, 1.00] tie | 0.98 [0.96, 1.00] tie | 0.99 [0.97, 1.01] tie | 0.97 [0.96, 1.00] tie (parity) | 0.97 [0.95, 0.99] tie (parity) | 0.95 [0.92, 0.98] tie | 0.97 [0.95, 0.98] tie (parity) |
| M50-1k fma | lcm | Q1 | 5.0% | 1.00 [0.98, 1.01] tie | 0.99 [0.97, 1.00] tie | 1.01 [0.98, 1.01] tie | 1.08 [1.07, 1.09] better | 1.08 [1.06, 1.09] better | 1.07 [1.06, 1.09] better | 1.08 [1.07, 1.09] better |
| M50-1k fma | freq | Q1 | 5.0% | 1.00 [0.98, 1.02] tie | 0.98 [0.97, 1.00] tie | 0.98 [0.97, 1.01] tie | 1.09 [1.06, 1.10] better | 1.09 [1.06, 1.10] better | 1.07 [1.05, 1.08] tie (parity) | 1.07 [1.06, 1.09] better |
| M75-1k fma | lcm | Q1 | 5.0% | 1.00 [0.99, 1.01] tie | 0.99 [0.98, 1.00] tie | 0.99 [0.99, 1.01] tie | 1.08 [1.07, 1.10] better | 1.08 [1.06, 1.10] better | 1.07 [1.05, 1.09] better | 1.08 [1.07, 1.10] better |
| M75-1k fma | freq | Q1 | 5.0% | 1.00 [0.97, 1.01] tie | 0.99 [0.95, 1.01] tie | 0.99 [0.99, 1.01] tie | 1.09 [1.07, 1.11] better | 1.09 [1.06, 1.11] better | 1.08 [1.04, 1.11] tie (parity) | 1.08 [1.06, 1.10] better |
| M95-1k fma | lcm | Q1 | 6.4% | 1.01 [0.99, 1.14] tie | 1.00 [0.98, 1.12] tie | 1.00 [0.97, 1.13] tie | 1.04 [0.93, 1.09] tie | 1.05 [1.03, 1.10] tie (parity) | 1.03 [1.01, 1.09] tie (parity) | 1.04 [1.00, 1.10] tie (parity) |
| M95-1k fma | freq | Q1 | 6.2% | 1.00 [0.96, 1.05] tie | 1.01 [0.99, 1.07] tie | 1.00 [0.97, 1.05] tie | 1.01 [0.97, 1.04] tie (parity) | 1.01 [0.98, 1.03] tie (parity) | 1.03 [1.01, 1.04] tie (parity) | 1.02 [0.99, 1.04] tie (parity) |
| K8-dec chain | lcm | Q1 | 5.0% | 1.01 [0.97, 1.05] tie | 0.99 [0.97, 1.02] tie | 1.00 [0.96, 1.05] tie | 0.98 [0.94, 1.01] tie | 0.99 [0.96, 1.01] tie (parity) | 0.97 [0.95, 1.00] tie | 0.98 [0.95, 1.00] tie (parity) |
| K8-dec chain | freq | Q1 | 5.0% | 1.00 [0.97, 1.01] tie | 0.99 [0.96, 1.00] tie | 0.98 [0.93, 1.00] tie | 0.97 [0.95, 1.00] tie (parity) | 0.97 [0.94, 0.99] tie | 0.96 [0.94, 0.98] tie | 0.96 [0.91, 0.97] tie |
| K8-mix chain | lcm | Q1 | 5.0% | 0.99 [0.97, 1.01] tie | 0.97 [0.95, 1.00] tie | 0.98 [0.97, 1.00] tie | 0.95 [0.92, 0.97] tie | 0.94 [0.92, 0.96] tie | 0.92 [0.89, 0.95] tie | 0.93 [0.91, 0.94] worse |
| K8-mix chain | freq | Q1 | 5.0% | 1.00 [0.97, 1.00] tie | 0.99 [0.97, 1.01] tie | 0.99 [0.97, 1.00] tie | 1.07 [1.05, 1.10] better | 1.06 [1.04, 1.09] tie (parity) | 1.06 [1.04, 1.08] tie (parity) | 1.05 [1.03, 1.09] tie (parity) |
| U8 fma | lcm | Q1 | 5.0% | 1.00 [0.97, 1.02] tie | 1.00 [0.98, 1.02] tie | 1.00 [0.96, 1.03] tie | 1.03 [1.01, 1.06] tie (parity) | 1.03 [1.00, 1.05] tie (parity) | 1.03 [1.01, 1.05] tie (parity) | 1.03 [1.01, 1.06] tie (parity) |
| U8 fma | freq | Q1 | 5.3% | 1.03 [0.99, 1.06] tie | 1.00 [0.97, 1.03] tie | 1.02 [0.99, 1.05] tie | 1.04 [1.01, 1.09] tie (parity) | 1.07 [1.05, 1.10] better | 1.04 [1.02, 1.08] tie (parity) | 1.06 [1.04, 1.11] tie (parity) |
| U32 fma | lcm | Q1 | 5.0% | 1.01 [0.99, 1.04] tie | 0.99 [0.97, 1.02] tie | 1.00 [0.98, 1.04] tie | 1.04 [1.01, 1.06] tie (parity) | 1.05 [1.04, 1.07] tie (parity) | 1.02 [1.02, 1.03] tie (parity) | 1.04 [1.03, 1.06] tie (parity) |
| U32 fma | freq | Q1 | 5.0% | 1.00 [0.98, 1.01] tie | 0.98 [0.94, 0.99] tie | 0.99 [0.98, 1.01] tie | 1.06 [1.04, 1.07] tie (parity) | 1.06 [1.03, 1.07] tie (parity) | 1.03 [1.00, 1.05] tie (parity) | 1.05 [1.04, 1.07] tie (parity) |
| N100 fma | lcm | Q2 | 5.0% | 1.00 [0.96, 1.05] tie | 1.02 [0.97, 1.05] tie | 1.03 [1.00, 1.04] tie | 0.68 [0.66, 0.70] worse | 0.68 [0.66, 0.70] worse | 0.69 [0.67, 0.71] worse | 0.70 [0.68, 0.71] worse |
| N1k fma | lcm | Q2 | 5.0% | 1.03 [0.99, 1.05] tie | 1.00 [0.98, 1.02] tie | 1.00 [0.98, 1.02] tie | 1.16 [1.14, 1.18] better | 1.19 [1.16, 1.20] better | 1.16 [1.15, 1.17] better | 1.15 [1.14, 1.19] better |
| N10k fma | lcm | Q2 | 5.0% | 1.19 [1.17, 1.21] better | 1.00 [0.98, 1.02] tie | 0.95 [0.93, 0.97] tie | 19.27 [18.86, 19.61] better | 22.88 [22.55, 23.38] better | 19.23 [18.96, 19.44] better | 18.28 [17.85, 18.55] better |
| N30k fma | lcm | Q2 | 5.0% | 1.36 [1.32, 1.38] better | 0.98 [0.95, 1.00] tie | 0.78 [0.77, 0.80] worse | 82.79 [81.51, 83.15] better | 112.19 [109.41, 113.01] better | 81.37 [79.06, 81.59] better | 64.93 [64.07, 65.45] better |
| P64 fma | lcm | Q2 | 5.0% | 1.43 [1.41, 1.46] better | 0.96 [0.95, 0.97] tie | 0.95 [0.93, 0.96] tie | 0.64 [0.62, 0.65] worse | 0.92 [0.89, 0.94] worse | 0.61 [0.60, 0.62] worse | 0.61 [0.59, 0.62] worse |
| P256 fma | lcm | Q2 | 5.0% | 1.41 [1.38, 1.46] better | 0.95 [0.93, 0.96] tie | 0.92 [0.91, 0.94] worse | 0.88 [0.86, 0.89] worse | 1.24 [1.21, 1.27] better | 0.83 [0.81, 0.84] worse | 0.81 [0.79, 0.82] worse |
| P1024 fma | lcm | Q2 | 5.0% | 1.29 [1.26, 1.31] better | 0.94 [0.93, 0.95] tie | 0.90 [0.88, 0.91] worse | 1.40 [1.39, 1.41] better | 1.80 [1.77, 1.84] better | 1.32 [1.30, 1.34] better | 1.26 [1.23, 1.28] better |
| P4000 fma | lcm | Q2 | 5.0% | 1.10 [1.09, 1.12] better | 0.97 [0.95, 0.99] tie | 0.98 [0.97, 0.99] tie | 1.82 [1.79, 1.84] better | 2.00 [1.97, 2.02] better | 1.76 [1.73, 1.79] better | 1.78 [1.77, 1.79] better |

## Entry breakdown (D rows against R0)

Medians over rounds, ms.  **entry** excludes D2's idle pass; **idle** is D2's pass (factoring the leftover nodes), included in total.  R0's entry includes its gcd refinement and pg_factor on every node (**R0 decomp**).  **dict** is the dictionary passes, **fill** the rest of entry (D rows).  For array jobs the lcm rule's rows are shown (entry does not depend on the rule except through V3's choice of D, in the fill).

| job | R0 entry | R0 decomp | D0 entry | D1 entry | D2 entry | D2 idle | D3 entry | D0 dict | D1 dict | D2 dict | D3 dict | D0 / D2 entry (no idle) | nodes R0 / D0 | D2 leftovers / shared / rho | D3 checks / hits |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---|---|
| W1 sum | 3.41 | 0.00 | 3.85 | 3.62 | 3.62 | 0.00 | 3.77 | 0.63 | 0.61 | 0.68 | 0.77 | 1.06 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| W2 fma | 17.30 | 0.00 | 17.41 | 17.64 | 17.20 | 0.00 | 17.43 | 5.57 | 5.77 | 5.61 | 5.54 | 1.01 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| W3 sum | 4.27 | 0.00 | 3.88 | 3.83 | 3.51 | 0.00 | 3.52 | 2.30 | 2.29 | 2.17 | 2.13 | 1.11 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| W3 fma | 17.86 | 0.00 | 18.26 | 18.71 | 18.03 | 0.00 | 17.95 | 10.61 | 10.82 | 10.29 | 10.04 | 1.01 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| W4 fma | 22.99 | 0.00 | 22.66 | 22.45 | 22.76 | 0.00 | 22.27 | 4.56 | 4.50 | 4.58 | 4.47 | 1.00 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| W6 chain | 17.47 | 0.00 | 17.69 | 17.17 | 17.28 | 0.00 | 17.70 | 5.72 | 5.65 | 5.73 | 5.85 | 1.02 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| W7 fma | 23.26 | 6.38 | 42.05 | 25.07 | 35.02 | 8.59 | 45.48 | 32.55 | 16.21 | 25.92 | 36.52 | 1.20 | 125 / 101 | 2,104 / 2,080 / 41 | 2,111 / 2,063 |
| W8 sum | 3.54 | 0.00 | 3.84 | 3.94 | 3.62 | 0.00 | 3.75 | 0.76 | 0.82 | 0.69 | 0.73 | 1.06 | 2 / 1 | 0 / 0 / 0 | 0 / 0 |
| W8 fma | 17.59 | 0.00 | 17.86 | 17.69 | 16.73 | 0.00 | 17.31 | 5.78 | 5.76 | 5.40 | 5.80 | 1.07 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| W9 sum | 4.69 | 0.00 | 4.84 | 4.79 | 4.60 | 0.00 | 4.74 | 1.93 | 1.92 | 1.83 | 1.92 | 1.05 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| W9 fma | 17.23 | 0.00 | 17.85 | 17.75 | 17.70 | 0.00 | 18.01 | 6.51 | 6.40 | 6.48 | 6.56 | 1.01 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| W10 fma2 | 30.30 | 0.00 | 30.02 | 30.02 | 30.14 | 0.00 | 29.86 | 9.99 | 10.17 | 10.11 | 10.10 | 1.00 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| S8 add | 45.68 | 0.00 | 49.41 | 48.40 | 48.97 | 0.00 | 49.10 | 5.10 | 5.08 | 5.07 | 5.00 | 1.01 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| S8 mul | 45.24 | 0.00 | 48.91 | 48.17 | 48.60 | 0.00 | 49.03 | 5.06 | 5.10 | 5.04 | 5.02 | 1.01 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| S15 add | 62.12 | 0.00 | 60.00 | 60.25 | 59.55 | 0.00 | 60.21 | 7.73 | 7.81 | 7.66 | 7.61 | 1.01 | 54 / 54 | 0 / 0 / 0 | 0 / 0 |
| S15 mul | 61.72 | 0.00 | 60.35 | 60.13 | 59.69 | 0.00 | 60.49 | 7.69 | 7.81 | 7.68 | 7.60 | 1.01 | 54 / 54 | 0 / 0 / 0 | 0 / 0 |
| S20 add | 71.38 | 0.00 | 71.21 | 71.26 | 71.34 | 0.00 | 70.99 | 13.40 | 13.33 | 13.41 | 13.14 | 1.00 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| S20 mul | 71.74 | 0.00 | 72.38 | 72.13 | 71.08 | 0.00 | 70.24 | 13.45 | 13.23 | 13.41 | 13.14 | 1.02 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| S30 add | 564.15 | 0.10 | 101.96 | 100.16 | 101.63 | 0.00 | 100.55 | 19.34 | 18.99 | 19.43 | 19.06 | 1.00 | 3,512 / 3,512 | 0 / 0 / 0 | 0 / 0 |
| S30 mul | 564.74 | 0.10 | 102.29 | 101.49 | 102.42 | 0.00 | 100.82 | 19.21 | 19.04 | 19.50 | 19.27 | 1.00 | 3,512 / 3,512 | 0 / 0 / 0 | 0 / 0 |
| S30d add | 63.60 | 0.00 | 74.68 | 75.88 | 76.87 | 0.00 | 74.47 | 5.45 | 5.72 | 5.46 | 5.29 | 0.97 | 55 / 55 | 0 / 0 / 0 | 0 / 0 |
| S30d mul | 63.76 | 0.00 | 74.56 | 74.85 | 74.69 | 0.00 | 74.87 | 5.44 | 5.47 | 5.25 | 5.27 | 1.00 | 55 / 55 | 0 / 0 / 0 | 0 / 0 |
| M50-12 sum | 6.08 | 0.00 | 5.33 | 5.28 | 5.11 | 0.00 | 5.30 | 3.38 | 3.27 | 3.22 | 3.26 | 1.04 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M50-12 fma | 21.33 | 0.00 | 20.76 | 20.34 | 19.78 | 0.00 | 20.28 | 10.80 | 10.61 | 10.50 | 10.51 | 1.05 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M75-12 sum | 6.12 | 0.00 | 6.00 | 5.88 | 5.81 | 0.00 | 5.97 | 3.27 | 3.28 | 3.21 | 3.32 | 1.03 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M75-12 fma | 21.43 | 0.00 | 21.38 | 21.13 | 21.01 | 0.00 | 21.35 | 10.65 | 10.44 | 10.33 | 10.59 | 1.02 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M95-12 sum | 4.07 | 0.00 | 4.23 | 4.15 | 3.92 | 0.00 | 4.15 | 1.25 | 1.25 | 1.21 | 1.24 | 1.08 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M95-12 fma | 16.13 | 0.00 | 16.09 | 16.07 | 15.55 | 0.00 | 16.29 | 4.68 | 4.67 | 4.55 | 4.58 | 1.03 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| M50-1k fma | 17.26 | 0.00 | 15.39 | 15.51 | 15.66 | 0.00 | 15.28 | 3.51 | 3.44 | 3.52 | 3.45 | 0.98 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| M75-1k fma | 11.08 | 0.00 | 10.60 | 10.39 | 10.52 | 0.00 | 10.49 | 3.29 | 3.27 | 3.27 | 3.31 | 1.01 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| M95-1k fma | 5.70 | 0.00 | 4.93 | 4.88 | 4.89 | 0.00 | 4.84 | 1.65 | 1.62 | 1.66 | 1.61 | 1.01 | 168 / 168 | 0 / 0 / 0 | 0 / 0 |
| K8-dec chain | 17.39 | 0.00 | 16.22 | 16.44 | 15.93 | 0.00 | 15.79 | 4.65 | 4.76 | 4.50 | 4.49 | 1.02 | 2 / 2 | 0 / 0 / 0 | 0 / 0 |
| K8-mix chain | 20.27 | 0.00 | 21.11 | 20.34 | 20.89 | 0.00 | 20.43 | 12.53 | 12.10 | 12.14 | 11.93 | 1.01 | 5 / 5 | 0 / 0 / 0 | 0 / 0 |
| U8 fma | 19.04 | 0.00 | 20.64 | 20.64 | 20.84 | 0.00 | 20.26 | 2.24 | 2.18 | 2.13 | 2.11 | 0.99 | 11 / 11 | 0 / 0 / 0 | 0 / 0 |
| U32 fma | 18.39 | 0.00 | 18.49 | 18.28 | 18.89 | 0.00 | 18.27 | 2.40 | 2.43 | 2.40 | 2.40 | 0.98 | 27 / 27 | 0 / 0 / 0 | 0 / 0 |
| N100 fma | 15.59 | 0.12 | 24.79 | 24.44 | 24.71 | 0.00 | 24.68 | 2.62 | 2.43 | 2.61 | 2.57 | 1.00 | 184 / 177 | 0 / 0 / 0 | 10 / 0 |
| N1k fma | 49.70 | 1.44 | 31.27 | 29.46 | 30.94 | 0.50 | 31.18 | 7.94 | 6.10 | 7.56 | 7.57 | 1.01 | 1,321 / 1,155 | 23 / 0 / 23 | 106 / 1 |
| N10k fma | 2423.10 | 14.27 | 84.57 | 64.66 | 80.62 | 4.58 | 91.28 | 44.86 | 24.86 | 41.13 | 51.32 | 1.05 | 10,271 / 9,603 | 215 / 2 / 205 | 1,265 / 8 |
| N30k fma | 18316.99 | 38.83 | 181.58 | 123.10 | 171.39 | 14.25 | 239.53 | 139.19 | 80.06 | 130.23 | 194.22 | 1.06 | 27,022 / 26,168 | 532 / 28 / 454 | 3,697 / 74 |
| P64 fma | 16.29 | 0.02 | 34.60 | 19.07 | 34.19 | 3.11 | 38.12 | 27.69 | 12.28 | 27.36 | 31.22 | 1.01 | 69 / 69 | 2,000 / 2,000 / 16 | 1,999 / 1,984 |
| P256 fma | 30.69 | 0.06 | 37.38 | 21.47 | 34.69 | 5.74 | 41.90 | 30.35 | 14.64 | 27.56 | 35.05 | 1.08 | 261 / 261 | 2,000 / 2,000 / 60 | 1,999 / 1,940 |
| P1024 fma | 79.12 | 0.28 | 49.00 | 33.42 | 37.47 | 15.39 | 56.99 | 39.85 | 24.39 | 28.25 | 47.32 | 1.31 | 1,000 / 998 | 2,000 / 1,998 / 230 | 1,999 / 1,754 |
| P4000 fma | 277.35 | 102.16 | 140.41 | 125.54 | 36.37 | 108.37 | 144.35 | 130.96 | 116.04 | 26.57 | 134.97 | 3.86 | 4,005 / 2,005 | 2,000 / 0 / 2,000 | 1,999 / 0 |

## Audits

One untimed invocation per job and row with `AB_AUDIT=1` after the timed rounds.  R1: live nodes pairwise coprime, V3's structure.  D0-D3: every edge re-expanded onto live nodes, live nodes pairwise coprime, each node the product of its primes, the dictionary consistent, then V3's structure.

Array jobs: lcm / freq; single-value jobs: the plain row.

| job | R1 | D0 | D1 | D2 | D3 |
|---|---|---|---|---|---|
| W1 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W2 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W3 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W3 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W4 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W6 chain | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W7 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W8 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W8 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W9 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W9 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| W10 fma2 | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| S8 add | pass | pass | pass | pass | pass |
| S8 mul | pass | pass | pass | pass | pass |
| S15 add | pass | pass | pass | pass | pass |
| S15 mul | pass | pass | pass | pass | pass |
| S20 add | pass | pass | pass | pass | pass |
| S20 mul | pass | pass | pass | pass | pass |
| S30 add | pass | pass | pass | pass | pass |
| S30 mul | pass | pass | pass | pass | pass |
| S30d add | pass | pass | pass | pass | pass |
| S30d mul | pass | pass | pass | pass | pass |
| M50-12 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M50-12 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M75-12 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M75-12 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M95-12 sum | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M95-12 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M50-1k fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M75-1k fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| M95-1k fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| K8-dec chain | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| K8-mix chain | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| U8 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| U32 fma | pass / pass | pass / pass | pass / pass | pass / pass | pass / pass |
| N100 fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| N1k fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| N10k fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| N30k fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| P64 fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| P256 fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| P1024 fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |
| P4000 fma | not run / not run | pass / not run | pass / not run | pass / not run | pass / not run |

## Workloads

- **W1** (existing): decimal column: sum of three-place decimals in (-10^4, 10^4); n = 1,000,000; jobs sum
- **W2** (existing): decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4); n = 1,000,000; jobs fma
- **W3** (existing): mixed small denominators 1..12, numerators |n| < 10^6; n = 1,000,000; jobs sum, fma
- **W4** (existing): unrelated denominators 1..1000, numerators |n| < 10^6; n = 200,000; jobs fma
- **W6** (existing): growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2); n = 1,000,000, K = 4; jobs chain
- **W7** (existing): 32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12; n = 200,000; jobs fma
- **W8** (existing): money: two-place decimals in (-10^5, 10^5); n = 1,000,000; jobs sum, fma
- **W9** (existing): mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6); n = 1,000,000; jobs sum, fma
- **W10** (existing): expression chain d = (a*b + c)*e + f on two-place decimals in (-100, 100); n = 1,000,000; jobs fma2
- **S8** (existing): numerators |n| < 2^7, denominators 1..12; n = 1,000,000; jobs add, mul
- **S15** (existing): numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32); n = 1,000,000; jobs add, mul
- **S20** (existing): numerators |n| < 2^20, denominators 1..1000; n = 1,000,000; jobs add, mul
- **S30** (existing): numerators |n| < 2^30, denominators uniform in 1..2^15; n = 1,000,000; jobs add, mul
- **S30d** (existing): numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15; n = 1,000,000; jobs add, mul
- **M50-12** (Q1): 50% denominator 1000 (three-place decimals in (-10^4, 10^4)), 50% denominators 1..12 (|num| < 10^6); n = 1,000,000; jobs sum, fma
- **M75-12** (Q1): 75% denominator 1000, 25% denominators 1..12; n = 1,000,000; jobs sum, fma
- **M95-12** (Q1): 95% denominator 1000, 5% denominators 1..12; n = 1,000,000; jobs sum, fma
- **M50-1k** (Q1): 50% denominator 1000, 50% denominators 1..1000 (|num| < 10^6); no column sum (its common denominator is far beyond 64 bits); n = 200,000; jobs fma
- **M75-1k** (Q1): 75% denominator 1000, 25% denominators 1..1000; n = 200,000; jobs fma
- **M95-1k** (Q1): 95% denominator 1000, 5% denominators 1..1000; n = 200,000; jobs fma
- **K8-dec** (Q1): chain x <- x*b + c, K = 8: x0, c three-place decimals in (-10, 10), b tenths in (-2, 2); n = 1,000,000, K = 8; jobs chain
- **K8-mix** (Q1): chain x <- x*b + c, K = 8: x0, c over 1..12 (|num| < 10^4), b over 1..4 with |b| < 2; n = 1,000,000, K = 8; jobs chain
- **U8** (Q1): unrelated denominators from a smaller set: 8 drawn once from 2..1000 (|num| < 10^6), a*b + c; n = 200,000; jobs fma
- **U32** (Q1): unrelated denominators: 32 drawn once from 2..1000, a*b + c; n = 200,000; jobs fma
- **N100** (Q2): node-count sweep: 100 distinct denominators (half in [2, 2^31), half in [2^32, 2^40)); a*b + c, c over a's denominator, b over 1..12; n = 200,000; jobs fma
- **N1k** (Q2): node-count sweep: 1,000 distinct denominators; n = 200,000; jobs fma
- **N10k** (Q2): node-count sweep: 10,000 distinct denominators; n = 200,000; jobs fma
- **N30k** (Q2): node-count sweep: 30,000 distinct denominators; n = 200,000; jobs fma
- **P64** (Q2): 2,000 distinct denominators p*q from a pool of 64 primes in [2^17, 2^24) (each prime in about 62 denominators); a*b + c as N*; n = 200,000; jobs fma
- **P256** (Q2): 2,000 p*q from a pool of 256 primes (each in about 16); n = 200,000; jobs fma
- **P1024** (Q2): 2,000 p*q from a pool of 1,024 primes (each in about 4); n = 200,000; jobs fma
- **P4000** (Q2): 2,000 p*q from 4,000 primes in disjoint pairs (no prime shared); n = 200,000; jobs fma

Detail tables: **entry** excludes D2's idle pass, shown as **idle**; **decomp** is R0/R1's pg_factor stage (inside their entry); **final** is the timed final reduction; **total** is the minimum over reps of entry + idle + op + final in one rep; **R0 / x** is R0's total over the row's total under the same rule; **repr** is the measured peak of the representation's allocations; **unreduced %** (round 1, every output) fails a row above 0; overflowed elements ("0 0") are counted: a row fails if over 0.1% overflowed or any overflow was false; ‡ marks rows with some.

### W1 sum: decimal column: sum of three-place decimals in (-10^4, 10^4)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 3.41 | 0.00 | 0.00 | 0.74 | 0.00 | 4.18 | 1.00 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 3.50 | 0.00 | 0.00 | 0.88 | 0.00 | 4.35 | 0.96 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 3.97 | 0.00 | 0.00 | 0.76 | 0.00 | 4.77 | 0.88 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 3.85 | 0.00 | 0.00 | 0.77 | 0.00 | 4.70 | 0.89 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-lcm | 3.62 | 0.00 | 0.00 | 0.72 | 0.00 | 4.31 | 0.97 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-lcm | 3.62 | 0.00 | 0.00 | 0.80 | 0.00 | 4.36 | 0.96 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 3.77 | 0.00 | 0.00 | 0.92 | 0.00 | 4.67 | 0.89 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 3.86 | 0.00 | 0.00 | 0.91 | 0.00 | 4.88 | 1.00 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 3.90 | 0.00 | 0.00 | 0.96 | 0.00 | 4.81 | 1.01 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 3.95 | 0.00 | 0.00 | 0.90 | 0.00 | 4.93 | 0.99 | 29 | 11.5 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 3.92 | 0.00 | 0.00 | 0.79 | 0.00 | 4.79 | 1.02 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-freq | 3.83 | 0.00 | 0.00 | 0.77 | 0.00 | 4.68 | 1.04 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-freq | 3.52 | 0.00 | 0.00 | 0.75 | 0.00 | 4.28 | 1.14 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 3.66 | 0.00 | 0.00 | 0.66 | 0.00 | 4.31 | 1.13 | 30 | 11.8 | 1 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W2 fma: decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.30 | 0.00 | 0.00 | 2.42 | 22.92 | 42.84 | 1.00 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 17.50 | 0.00 | 0.00 | 2.41 | 22.99 | 43.29 | 0.99 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 17.60 | 0.00 | 0.00 | 2.39 | 22.48 | 42.61 | 1.01 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 17.41 | 0.00 | 0.00 | 2.46 | 28.04 | 48.16 | 0.89 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-lcm | 17.64 | 0.00 | 0.00 | 2.51 | 28.15 | 48.29 | 0.89 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-lcm | 17.20 | 0.00 | 0.00 | 2.49 | 28.18 | 47.92 | 0.89 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 17.43 | 0.00 | 0.00 | 2.41 | 27.86 | 47.82 | 0.90 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 16.92 | 0.00 | 0.00 | 2.51 | 23.02 | 42.52 | 1.00 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 17.70 | 0.00 | 0.00 | 2.50 | 22.99 | 43.22 | 0.98 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 17.41 | 0.00 | 0.00 | 2.47 | 22.59 | 42.62 | 1.00 | 105 | 57.3 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 17.10 | 0.00 | 0.00 | 2.42 | 27.98 | 47.29 | 0.90 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-freq | 17.09 | 0.00 | 0.00 | 2.47 | 27.95 | 47.38 | 0.90 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-freq | 17.21 | 0.00 | 0.00 | 2.45 | 27.88 | 47.64 | 0.89 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 16.96 | 0.00 | 0.00 | 2.43 | 28.12 | 47.77 | 0.89 | 106 | 57.5 | 49 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W3 sum: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 4.27 | 0.00 | 0.00 | 1.30 | 0.00 | 5.58 | 1.00 | 29 | 11.5 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 3.90 | 0.00 | 0.00 | 1.14 | 0.00 | 5.04 | 1.11 | 29 | 11.5 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 3.85 | 0.00 | 0.00 | 0.92 | 0.00 | 4.78 | 1.17 | 29 | 11.5 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 3.88 | 0.00 | 0.00 | 0.95 | 0.00 | 4.96 | 1.12 | 30 | 11.8 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 3.83 | 0.00 | 0.00 | 0.80 | 0.00 | 4.65 | 1.20 | 30 | 11.8 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 3.51 | 0.00 | 0.00 | 0.72 | 0.00 | 4.31 | 1.29 | 30 | 11.8 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 3.52 | 0.00 | 0.00 | 0.70 | 0.00 | 4.20 | 1.33 | 30 | 11.8 | 1 | D=27720; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 15.92 | 0.00 | 0.00 | 26.39 | 0.00 | 42.18 | 1.00 | 74 | 57.5 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 16.00 | 0.00 | 0.00 | 26.53 | 0.00 | 42.51 | 0.99 | 74 | 57.5 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 11.68 | 0.00 | 0.00 | 21.28 | 0.00 | 33.00 | 1.28 | 74 | 57.5 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 16.39 | 0.00 | 0.00 | 26.92 | 0.00 | 43.36 | 0.97 | 75 | 57.8 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 16.67 | 0.00 | 0.00 | 27.80 | 0.00 | 44.71 | 0.94 | 75 | 57.8 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 16.66 | 0.00 | 0.00 | 26.99 | 0.00 | 43.88 | 0.96 | 75 | 57.8 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 16.11 | 0.00 | 0.00 | 26.70 | 0.00 | 42.85 | 0.98 | 75 | 57.8 | 1 | D=12; exc 499,523 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W3 fma: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.86 | 0.00 | 0.00 | 45.93 | 36.82 | 101.60 | 1.00 | 138 | 105.3 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 18.20 | 0.00 | 0.00 | 45.96 | 37.09 | 100.97 | 1.01 | 138 | 105.3 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 17.86 | 0.00 | 0.00 | 43.81 | 46.15 | 107.68 | 0.94 | 138 | 105.3 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 300,700 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 18.26 | 0.00 | 0.00 | 44.49 | 38.82 | 101.90 | 1.00 | 139 | 105.5 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 18.71 | 0.00 | 0.00 | 44.65 | 39.07 | 102.68 | 0.99 | 139 | 105.5 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 18.03 | 0.00 | 0.00 | 44.56 | 38.93 | 102.16 | 0.99 | 139 | 105.5 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 17.95 | 0.00 | 0.00 | 44.14 | 39.11 | 101.38 | 1.00 | 139 | 105.5 | 166 | D=27720; exc 0 in, 300,698 out; 300,700 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 58.84 | 0.00 | 0.00 | 97.42 | 13.63 | 171.05 | 1.00 | 374 | 379.3 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 59.67 | 0.00 | 0.00 | 98.25 | 13.75 | 172.93 | 0.99 | 374 | 379.3 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 37.58 | 0.00 | 0.00 | 83.84 | 37.65 | 159.67 | 1.07 | 393 | 379.3 | 166 | D=12; exc 2,082,070 in, 890,021 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 60.07 | 0.00 | 0.00 | 93.08 | 14.49 | 168.38 | 1.02 | 375 | 379.5 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 60.85 | 0.00 | 0.00 | 93.70 | 14.62 | 169.12 | 1.01 | 375 | 379.5 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 60.80 | 0.00 | 0.00 | 94.88 | 14.71 | 170.96 | 1.00 | 375 | 379.5 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 60.15 | 0.00 | 0.00 | 95.88 | 14.43 | 171.60 | 1.00 | 375 | 379.5 | 166 | D=12; exc 2,082,070 in, 669,944 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W4 fma: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 22.99 | 0.00 | 0.00 | 29.76 | 1.56 | 54.44 | 1.00 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 22.81 | 0.00 | 0.00 | 29.07 | 1.56 | 53.76 | 1.01 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 12.09 | 0.00 | 0.00 | 25.25 | 8.82 | 46.28 | 1.18 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 200,000 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 22.66 | 0.00 | 0.00 | 27.61 | 1.59 | 51.97 | 1.05 | 95 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1-lcm | 22.45 | 0.00 | 0.00 | 27.82 | 1.59 | 52.15 | 1.04 | 96 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2-lcm | 22.76 | 0.00 | 0.00 | 27.81 | 1.56 | 52.22 | 1.04 | 96 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 22.27 | 0.00 | 0.00 | 28.14 | 1.54 | 51.86 | 1.05 | 95 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 22.65 | 0.00 | 0.00 | 29.54 | 1.59 | 54.14 | 1.00 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 22.50 | 0.00 | 0.00 | 29.02 | 1.56 | 53.59 | 1.01 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 12.07 | 0.00 | 0.00 | 25.20 | 8.86 | 46.35 | 1.17 | 101 | 103.6 | 183438 | D=213; exc 598,438 in, 200,000 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 22.39 | 0.00 | 0.00 | 27.39 | 1.53 | 51.29 | 1.06 | 95 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1-freq | 22.19 | 0.00 | 0.00 | 27.57 | 1.60 | 51.34 | 1.05 | 96 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2-freq | 22.67 | 0.00 | 0.00 | 27.67 | 1.61 | 51.95 | 1.04 | 96 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 22.13 | 0.00 | 0.00 | 27.82 | 1.62 | 51.60 | 1.05 | 95 | 103.9 | 183438 | D=213; exc 598,438 in, 199,989 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W6 chain: growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2)

n = 1,000,000, K = 4

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.47 | 0.00 | 0.00 | 6.97 | 29.24 | 53.74 | 1.00 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 17.45 | 0.00 | 0.00 | 7.32 | 29.18 | 54.01 | 0.99 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 17.19 | 0.00 | 0.00 | 7.27 | 28.43 | 52.93 | 1.02 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 17.69 | 0.00 | 0.00 | 7.56 | 33.67 | 59.25 | 0.91 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-lcm | 17.17 | 0.00 | 0.00 | 7.22 | 33.87 | 58.85 | 0.91 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-lcm | 17.28 | 0.00 | 0.00 | 7.35 | 33.75 | 58.75 | 0.91 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 17.70 | 0.00 | 0.00 | 7.57 | 33.72 | 58.67 | 0.92 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 17.52 | 0.00 | 0.00 | 7.12 | 29.16 | 53.99 | 1.00 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 17.37 | 0.00 | 0.00 | 7.06 | 29.17 | 53.49 | 1.01 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 17.30 | 0.00 | 0.00 | 7.35 | 28.38 | 53.09 | 1.02 | 105 | 64.9 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 17.56 | 0.00 | 0.00 | 7.33 | 33.69 | 59.00 | 0.92 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-freq | 17.30 | 0.00 | 0.00 | 7.21 | 33.33 | 57.91 | 0.93 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-freq | 16.80 | 0.00 | 0.00 | 7.18 | 33.85 | 58.65 | 0.92 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 17.20 | 0.00 | 0.00 | 7.34 | 33.88 | 58.37 | 0.93 | 106 | 65.2 | 253 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W7 fma: 32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 23.26 | 0.00 | 6.38 | 17.63 | 1.48 | 42.56 | 1.00 ‡ | 83 | 80.9 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 125 nodes, 24 decomp splits | 0.00 | ok |
| r0-lcm-null | 23.31 | 0.00 | 6.41 | 17.57 | 1.47 | 42.51 | 1.00 ‡ | 83 | 80.9 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 125 nodes, 24 decomp splits | 0.00 | ok |
| r1-lcm | 22.15 | 0.00 | 6.35 | 22.76 | 5.32 | 50.24 | 0.85 ‡ | 83 | 80.9 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 138,420 early red.; 125 nodes, 24 decomp splits | 0.00 | ok |
| d0-lcm | 42.05 | 0.00 | 0.00 | 20.93 | 1.60 | 64.56 | 0.66 ‡ | 77 | 81.2 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d1-lcm | 25.07 | 0.00 | 0.00 | 21.05 | 1.53 | 47.71 | 0.89 ‡ | 78 | 81.2 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d2-lcm | 35.02 | 8.59 | 0.00 | 21.06 | 1.52 | 66.23 | 0.64 ‡ | 85 | 81.4 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops; 2,104 leftovers (2,080 shared), 41 pieces to rho | 0.00 | ok |
| d3-lcm | 45.48 | 0.00 | 0.00 | 20.76 | 1.58 | 67.84 | 0.63 ‡ | 77 | 81.3 | 69474 | D=9176664807820423; exc 389,996 in, 200,000 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops; 2,111 checks, 2,063 hits, 120 leaves | 0.00 | ok |
| r0-freq | 27.26 | 0.00 | 6.40 | 16.51 | 1.49 | 45.57 | 1.00 ‡ | 96 | 103.9 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 125 nodes, 24 decomp splits | 0.00 | ok |
| r0-freq-null | 27.27 | 0.00 | 6.39 | 16.40 | 1.54 | 45.27 | 1.01 ‡ | 96 | 103.9 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 125 nodes, 24 decomp splits | 0.00 | ok |
| r1-freq | 23.74 | 0.00 | 6.39 | 14.87 | 5.75 | 44.55 | 1.02 ‡ | 96 | 103.9 | 69474 | D=9176664807820423; exc 556,396 in, 199,187 out; 0 dense ovf; 213 early red.; 125 nodes, 24 decomp splits | 0.00 | ok |
| d0-freq | 45.64 | 0.00 | 0.00 | 18.24 | 1.58 | 65.82 | 0.69 ‡ | 99 | 104.2 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d1-freq | 29.19 | 0.00 | 0.00 | 18.83 | 1.71 | 49.53 | 0.92 ‡ | 99 | 104.2 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d2-freq | 39.23 | 8.69 | 0.00 | 17.95 | 1.65 | 67.50 | 0.68 ‡ | 99 | 104.4 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops; 2,104 leftovers (2,080 shared), 41 pieces to rho | 0.00 | ok |
| d3-freq | 49.77 | 0.00 | 0.00 | 17.97 | 1.59 | 69.59 | 0.65 ‡ | 99 | 104.3 | 69474 | D=9176664807820423; exc 556,396 in, 197,532 out; 0 dense ovf; 101 nodes (24 multi-prime); splits 17 entry / 0 ops; 2,111 checks, 2,063 hits, 120 leaves | 0.00 | ok |

r0-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
r0-lcm-null: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
r1-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d0-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d1-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d2-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d3-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
r0-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
r0-freq-null: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
r1-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d0-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d1-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d2-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  
d3-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits 64 bits)  

### W8 sum: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 3.54 | 0.00 | 0.00 | 0.81 | 0.00 | 4.32 | 1.00 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 3.73 | 0.00 | 0.00 | 0.88 | 0.00 | 4.57 | 0.94 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 3.69 | 0.00 | 0.00 | 0.77 | 0.00 | 4.43 | 0.97 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 3.84 | 0.00 | 0.00 | 0.82 | 0.00 | 4.64 | 0.93 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 3.94 | 0.00 | 0.00 | 0.79 | 0.00 | 4.73 | 0.91 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 3.62 | 0.00 | 0.00 | 0.73 | 0.00 | 4.35 | 0.99 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 3.75 | 0.00 | 0.00 | 0.76 | 0.00 | 4.51 | 0.96 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 3.78 | 0.00 | 0.00 | 0.87 | 0.00 | 4.58 | 1.00 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 3.86 | 0.00 | 0.00 | 0.76 | 0.00 | 4.71 | 0.97 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 3.82 | 0.00 | 0.00 | 0.84 | 0.00 | 4.80 | 0.95 | 29 | 11.5 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 3.80 | 0.00 | 0.00 | 0.78 | 0.00 | 4.63 | 0.99 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 3.77 | 0.00 | 0.00 | 0.88 | 0.00 | 4.70 | 0.98 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 3.63 | 0.00 | 0.00 | 0.71 | 0.00 | 4.34 | 1.06 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 3.61 | 0.00 | 0.00 | 0.74 | 0.00 | 4.35 | 1.05 | 30 | 11.8 | 1 | D=100; exc 0 in, 0 out; 0 dense ovf; 1 nodes (1 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W8 fma: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.59 | 0.00 | 0.00 | 2.41 | 21.39 | 41.74 | 1.00 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 18.21 | 0.00 | 0.00 | 2.56 | 21.69 | 42.42 | 0.98 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 18.19 | 0.00 | 0.00 | 2.45 | 21.24 | 42.19 | 0.99 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 17.86 | 0.00 | 0.00 | 2.43 | 26.70 | 46.93 | 0.89 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-lcm | 17.69 | 0.00 | 0.00 | 2.47 | 26.71 | 47.05 | 0.89 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-lcm | 16.73 | 0.00 | 0.00 | 2.53 | 26.73 | 46.53 | 0.90 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 17.31 | 0.00 | 0.00 | 2.45 | 26.72 | 46.55 | 0.90 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 17.74 | 0.00 | 0.00 | 2.45 | 21.54 | 42.10 | 1.00 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 17.22 | 0.00 | 0.00 | 2.41 | 21.38 | 41.19 | 1.02 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 18.05 | 0.00 | 0.00 | 2.49 | 21.41 | 41.96 | 1.00 | 105 | 57.3 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 17.33 | 0.00 | 0.00 | 2.46 | 26.85 | 46.39 | 0.91 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-freq | 17.17 | 0.00 | 0.00 | 2.43 | 26.87 | 46.65 | 0.90 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-freq | 17.25 | 0.00 | 0.00 | 2.42 | 26.82 | 46.90 | 0.90 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 17.32 | 0.00 | 0.00 | 2.52 | 26.76 | 46.41 | 0.91 | 106 | 57.5 | 25 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W9 sum: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 4.69 | 0.00 | 0.00 | 1.15 | 0.00 | 5.89 | 1.00 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 4.54 | 0.00 | 0.00 | 1.05 | 0.00 | 5.59 | 1.05 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 4.63 | 0.00 | 0.00 | 0.94 | 0.00 | 5.57 | 1.06 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 4.84 | 0.00 | 0.00 | 0.92 | 0.00 | 5.71 | 1.03 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 4.79 | 0.00 | 0.00 | 0.75 | 0.00 | 5.59 | 1.05 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 4.60 | 0.00 | 0.00 | 0.80 | 0.00 | 5.50 | 1.07 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 4.74 | 0.00 | 0.00 | 0.82 | 0.00 | 5.56 | 1.06 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 5.75 | 0.00 | 0.00 | 3.02 | 0.00 | 8.75 | 1.00 | 36 | 17.2 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 5.75 | 0.00 | 0.00 | 3.02 | 0.00 | 8.74 | 1.00 | 36 | 17.2 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 5.42 | 0.00 | 0.00 | 2.92 | 0.00 | 8.35 | 1.05 | 36 | 17.2 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 6.25 | 0.00 | 0.00 | 3.10 | 0.00 | 9.36 | 0.94 | 37 | 17.5 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 6.17 | 0.00 | 0.00 | 3.16 | 0.00 | 9.39 | 0.93 | 37 | 17.5 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 5.98 | 0.00 | 0.00 | 3.07 | 0.00 | 8.99 | 0.97 | 37 | 17.5 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 6.01 | 0.00 | 0.00 | 3.04 | 0.00 | 9.16 | 0.96 | 37 | 17.5 | 1 | D=1000; exc 49,755 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W9 fma: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.23 | 0.00 | 0.00 | 100.14 | 22.58 | 140.57 | 1.00 | 185 | 153.3 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 17.61 | 0.00 | 0.00 | 100.50 | 22.81 | 140.87 | 1.00 | 185 | 153.3 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 17.11 | 0.00 | 0.00 | 92.64 | 33.00 | 143.05 | 0.98 | 185 | 153.3 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 575,954 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 17.85 | 0.00 | 0.00 | 95.49 | 24.34 | 137.43 | 1.02 | 185 | 153.5 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 17.75 | 0.00 | 0.00 | 96.01 | 25.04 | 140.01 | 1.00 | 185 | 153.5 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 17.70 | 0.00 | 0.00 | 94.92 | 25.08 | 137.92 | 1.02 | 185 | 153.5 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 18.01 | 0.00 | 0.00 | 95.62 | 24.71 | 138.99 | 1.01 | 185 | 153.5 | 384 | D=693000; exc 0 in, 575,843 out; 575,954 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 22.41 | 0.00 | 0.00 | 22.56 | 24.38 | 69.52 | 1.00 | 129 | 86.0 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 22.38 | 0.00 | 0.00 | 22.31 | 24.34 | 69.03 | 1.01 | 129 | 86.0 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 21.40 | 0.00 | 0.00 | 12.60 | 27.86 | 62.17 | 1.12 | 133 | 97.5 | 384 | D=1000; exc 149,976 in, 142,621 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 23.16 | 0.00 | 0.00 | 20.82 | 27.24 | 71.24 | 0.98 | 130 | 86.3 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 22.96 | 0.00 | 0.00 | 20.84 | 27.15 | 71.23 | 0.98 | 130 | 86.3 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 23.01 | 0.00 | 0.00 | 21.12 | 27.47 | 71.84 | 0.97 | 130 | 86.3 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 23.01 | 0.00 | 0.00 | 20.86 | 27.20 | 71.72 | 0.97 | 130 | 86.3 | 384 | D=1000; exc 149,976 in, 96,079 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### W10 fma2: expression chain d = (a*b + c)*e + f on two-place decimals in (-100, 100)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 30.30 | 0.00 | 0.00 | 4.98 | 25.97 | 61.75 | 1.00 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 31.35 | 0.00 | 0.00 | 5.05 | 26.02 | 63.31 | 0.98 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 30.77 | 0.00 | 0.00 | 5.00 | 25.70 | 61.46 | 1.00 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 30.02 | 0.00 | 0.00 | 5.03 | 30.94 | 66.09 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-lcm | 30.02 | 0.00 | 0.00 | 5.01 | 30.88 | 66.25 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-lcm | 30.14 | 0.00 | 0.00 | 5.06 | 31.06 | 66.32 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 29.86 | 0.00 | 0.00 | 5.01 | 31.00 | 66.21 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 30.98 | 0.00 | 0.00 | 4.89 | 25.91 | 62.10 | 1.00 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 30.89 | 0.00 | 0.00 | 4.96 | 26.04 | 61.92 | 1.00 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 30.65 | 0.00 | 0.00 | 4.98 | 25.63 | 61.77 | 1.01 | 166 | 87.8 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 30.32 | 0.00 | 0.00 | 4.99 | 30.79 | 66.37 | 0.94 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d1-freq | 30.49 | 0.00 | 0.00 | 5.03 | 30.86 | 66.61 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops | 0.00 | ok |
| d2-freq | 29.01 | 0.00 | 0.00 | 5.02 | 31.15 | 65.39 | 0.95 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 29.83 | 0.00 | 0.00 | 5.08 | 31.30 | 66.51 | 0.93 | 167 | 88.1 | 49 | D=100; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 0 entry / 1 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S8 add: numerators |n| < 2^7, denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 45.68 | 0.00 | 0.00 | 46.71 | 7.55 | 100.59 | 1.00 | 374 | 321.8 | 42 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 45.49 | 0.00 | 0.00 | 46.89 | 7.32 | 99.89 | 1.01 | 374 | 321.8 | 42 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 26.32 | 0.00 | 0.00 | 44.86 | 27.47 | 98.98 | 1.02 | 374 | 321.8 | 42 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 49.41 | 0.00 | 0.00 | 46.25 | 7.86 | 104.33 | 0.96 | 374 | 322.1 | 42 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1 | 48.40 | 0.00 | 0.00 | 45.99 | 7.67 | 102.19 | 0.98 | 374 | 322.1 | 42 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2 | 48.97 | 0.00 | 0.00 | 46.51 | 7.73 | 103.43 | 0.97 | 374 | 322.1 | 42 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 49.10 | 0.00 | 0.00 | 47.47 | 7.72 | 104.19 | 0.97 | 374 | 322.1 | 42 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S8 mul: numerators |n| < 2^7, denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 45.24 | 0.00 | 0.00 | 43.90 | 7.65 | 97.45 | 1.00 | 374 | 321.8 | 59 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 44.99 | 0.00 | 0.00 | 43.66 | 7.59 | 96.83 | 1.01 | 374 | 321.8 | 59 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 26.49 | 0.00 | 0.00 | 38.22 | 31.09 | 96.09 | 1.01 | 374 | 321.8 | 59 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 48.91 | 0.00 | 0.00 | 44.64 | 8.01 | 101.38 | 0.96 | 374 | 322.1 | 59 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1 | 48.17 | 0.00 | 0.00 | 45.12 | 7.98 | 100.79 | 0.97 | 374 | 322.1 | 59 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2 | 48.60 | 0.00 | 0.00 | 44.76 | 7.86 | 101.46 | 0.96 | 374 | 322.1 | 59 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 49.03 | 0.00 | 0.00 | 44.61 | 7.96 | 101.91 | 0.96 | 374 | 322.1 | 59 | ; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S15 add: numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 62.12 | 0.00 | 0.00 | 63.58 | 7.69 | 133.05 | 1.00 | 374 | 321.8 | 14712 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 54 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 62.22 | 0.00 | 0.00 | 63.59 | 7.67 | 133.57 | 1.00 | 374 | 321.8 | 14712 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 54 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 30.21 | 0.00 | 0.00 | 59.90 | 34.00 | 124.89 | 1.07 | 374 | 321.8 | 14712 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 54 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 60.00 | 0.00 | 0.00 | 65.00 | 7.85 | 133.49 | 1.00 | 371 | 322.1 | 14712 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d1 | 60.25 | 0.00 | 0.00 | 65.70 | 8.19 | 136.85 | 0.97 | 371 | 322.1 | 14712 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d2 | 59.55 | 0.00 | 0.00 | 66.31 | 8.13 | 134.10 | 0.99 | 371 | 322.1 | 14712 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 60.21 | 0.00 | 0.00 | 66.25 | 7.98 | 134.78 | 0.99 | 371 | 322.1 | 14712 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S15 mul: numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 61.72 | 0.00 | 0.00 | 58.01 | 7.68 | 127.52 | 1.00 | 374 | 321.8 | 17522 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 54 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 62.45 | 0.00 | 0.00 | 58.25 | 7.79 | 128.94 | 0.99 | 374 | 321.8 | 17522 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 54 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 31.07 | 0.00 | 0.00 | 45.30 | 40.34 | 117.71 | 1.08 | 374 | 321.8 | 17522 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 54 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 60.35 | 0.00 | 0.00 | 58.41 | 7.99 | 127.38 | 1.00 | 371 | 322.1 | 17522 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d1 | 60.13 | 0.00 | 0.00 | 58.37 | 8.07 | 126.83 | 1.01 | 371 | 322.1 | 17522 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d2 | 59.69 | 0.00 | 0.00 | 58.29 | 8.15 | 126.09 | 1.01 | 371 | 322.1 | 17522 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 60.49 | 0.00 | 0.00 | 58.18 | 8.22 | 127.33 | 1.00 | 371 | 322.1 | 17522 | ; 54 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S20 add: numerators |n| < 2^20, denominators 1..1000

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 71.38 | 0.00 | 0.00 | 70.34 | 8.01 | 149.98 | 1.00 | 372 | 321.9 | 172851 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 72.26 | 0.00 | 0.00 | 70.48 | 8.02 | 152.18 | 0.99 | 372 | 321.9 | 172851 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 37.29 | 0.00 | 0.00 | 65.41 | 37.58 | 140.59 | 1.07 | 372 | 321.9 | 172851 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 71.21 | 0.00 | 0.00 | 70.04 | 8.26 | 149.19 | 1.01 | 371 | 322.2 | 172851 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1 | 71.26 | 0.00 | 0.00 | 70.39 | 8.27 | 149.89 | 1.00 | 372 | 322.2 | 172851 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2 | 71.34 | 0.00 | 0.00 | 71.75 | 8.06 | 151.34 | 0.99 | 372 | 322.2 | 172851 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 70.99 | 0.00 | 0.00 | 71.62 | 8.03 | 150.87 | 0.99 | 371 | 322.2 | 172851 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S20 mul: numerators |n| < 2^20, denominators 1..1000

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 71.74 | 0.00 | 0.00 | 63.58 | 8.13 | 143.07 | 1.00 | 372 | 321.9 | 166664 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 70.69 | 0.00 | 0.00 | 63.63 | 7.75 | 143.40 | 1.00 | 372 | 321.9 | 166664 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 37.04 | 0.00 | 0.00 | 48.42 | 42.96 | 129.56 | 1.10 | 372 | 321.9 | 166664 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 72.38 | 0.00 | 0.00 | 61.53 | 8.04 | 142.91 | 1.00 | 371 | 322.2 | 166664 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1 | 72.13 | 0.00 | 0.00 | 62.76 | 8.21 | 143.39 | 1.00 | 372 | 322.2 | 166664 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2 | 71.08 | 0.00 | 0.00 | 61.31 | 8.07 | 140.05 | 1.02 | 372 | 322.2 | 166664 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 70.24 | 0.00 | 0.00 | 60.87 | 7.79 | 138.77 | 1.03 | 371 | 322.2 | 166664 | ; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S30 add: numerators |n| < 2^30, denominators uniform in 1..2^15

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 564.15 | 0.00 | 0.10 | 78.63 | 7.68 | 651.12 | 1.00 | 388 | 325.2 | 983127 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 567.36 | 0.00 | 0.10 | 77.86 | 7.55 | 653.06 | 1.00 | 388 | 325.2 | 983127 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 511.82 | 0.00 | 0.10 | 72.72 | 35.50 | 620.44 | 1.05 | 388 | 325.2 | 983127 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 101.96 | 0.00 | 0.00 | 81.25 | 7.91 | 191.80 | 3.39 | 378 | 326.7 | 983127 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops | 0.00 | ok |
| d1 | 100.16 | 0.00 | 0.00 | 81.58 | 7.97 | 190.24 | 3.42 | 378 | 326.7 | 983127 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops | 0.00 | ok |
| d2 | 101.63 | 0.00 | 0.00 | 83.43 | 8.02 | 193.54 | 3.36 | 378 | 326.7 | 983127 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 100.55 | 0.00 | 0.00 | 83.40 | 8.09 | 192.89 | 3.38 | 378 | 326.7 | 983127 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S30 mul: numerators |n| < 2^30, denominators uniform in 1..2^15

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 564.74 | 0.00 | 0.10 | 70.13 | 7.74 | 642.93 | 1.00 | 388 | 325.2 | 979925 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| r0-null | 562.39 | 0.00 | 0.10 | 70.26 | 7.65 | 641.22 | 1.00 | 388 | 325.2 | 979925 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| r1 | 512.18 | 0.00 | 0.10 | 51.52 | 42.03 | 608.02 | 1.06 | 388 | 325.2 | 979925 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 3,512 nodes, 0 decomp splits | 0.00 | ok |
| d0 | 102.29 | 0.00 | 0.00 | 70.29 | 7.89 | 181.23 | 3.55 | 378 | 326.7 | 979925 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops | 0.00 | ok |
| d1 | 101.49 | 0.00 | 0.00 | 71.67 | 7.99 | 181.75 | 3.54 | 378 | 326.7 | 979925 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops | 0.00 | ok |
| d2 | 102.42 | 0.00 | 0.00 | 70.58 | 8.19 | 182.61 | 3.52 | 378 | 326.7 | 979925 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 100.82 | 0.00 | 0.00 | 70.41 | 8.02 | 179.64 | 3.58 | 378 | 326.7 | 979925 | ; 3,512 nodes (0 multi-prime); splits 15 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S30d add: numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 63.60 | 0.00 | 0.00 | 75.02 | 7.84 | 146.95 | 1.00 | 374 | 321.8 | 18505 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 55 nodes, 1 decomp splits | 0.00 | ok |
| r0-null | 63.65 | 0.00 | 0.00 | 76.28 | 7.64 | 148.15 | 0.99 | 374 | 321.8 | 18505 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 55 nodes, 1 decomp splits | 0.00 | ok |
| r1 | 26.21 | 0.00 | 0.00 | 65.26 | 35.92 | 128.26 | 1.15 | 374 | 321.8 | 18505 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 55 nodes, 1 decomp splits | 0.00 | ok |
| d0 | 74.68 | 0.00 | 0.00 | 76.20 | 7.87 | 158.79 | 0.93 | 374 | 322.1 | 18505 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops | 0.00 | ok |
| d1 | 75.88 | 0.00 | 0.00 | 77.38 | 8.00 | 162.02 | 0.91 | 374 | 322.1 | 18505 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops | 0.00 | ok |
| d2 | 76.87 | 0.00 | 0.00 | 79.80 | 7.87 | 165.88 | 0.89 | 374 | 322.1 | 18505 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 74.47 | 0.00 | 0.00 | 78.60 | 8.04 | 161.42 | 0.91 | 374 | 322.1 | 18505 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### S30d mul: numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0 | 63.76 | 0.00 | 0.00 | 68.38 | 7.76 | 140.06 | 1.00 | 374 | 321.8 | 25935 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 55 nodes, 1 decomp splits | 0.00 | ok |
| r0-null | 63.86 | 0.00 | 0.00 | 67.85 | 7.63 | 140.27 | 1.00 | 374 | 321.8 | 25935 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 55 nodes, 1 decomp splits | 0.00 | ok |
| r1 | 26.13 | 0.00 | 0.00 | 50.08 | 42.49 | 118.96 | 1.18 | 374 | 321.8 | 25935 | D=1; exc 2,000,000 in, 1,000,000 out; 0 dense ovf; 0 early red.; 55 nodes, 1 decomp splits | 0.00 | ok |
| d0 | 74.56 | 0.00 | 0.00 | 66.79 | 7.89 | 149.12 | 0.94 | 374 | 322.1 | 25935 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops | 0.00 | ok |
| d1 | 74.85 | 0.00 | 0.00 | 66.76 | 7.75 | 149.26 | 0.94 | 374 | 322.1 | 25935 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops | 0.00 | ok |
| d2 | 74.69 | 0.00 | 0.00 | 66.25 | 7.92 | 149.38 | 0.94 | 374 | 322.1 | 25935 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3 | 74.87 | 0.00 | 0.00 | 66.19 | 7.97 | 149.02 | 0.94 | 374 | 322.1 | 25935 | ; 55 nodes (0 multi-prime); splits 9 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M50-12 sum: 50% denominator 1000 (three-place decimals in (-10^4, 10^4)), 50% denominators 1..12 (|num| < 10^6)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 6.08 | 0.00 | 0.00 | 1.19 | 0.00 | 7.26 | 1.00 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 5.74 | 0.00 | 0.00 | 1.25 | 0.00 | 6.90 | 1.05 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 5.59 | 0.00 | 0.00 | 0.80 | 0.00 | 6.35 | 1.14 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 5.33 | 0.00 | 0.00 | 0.78 | 0.00 | 6.10 | 1.19 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 5.28 | 0.00 | 0.00 | 0.70 | 0.00 | 5.99 | 1.21 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 5.11 | 0.00 | 0.00 | 0.75 | 0.00 | 5.83 | 1.25 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 5.30 | 0.00 | 0.00 | 0.82 | 0.00 | 6.09 | 1.19 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 12.38 | 0.00 | 0.00 | 11.63 | 0.00 | 23.93 | 1.00 | 53 | 34.5 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 12.45 | 0.00 | 0.00 | 11.39 | 0.00 | 23.94 | 1.00 | 53 | 34.5 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 10.07 | 0.00 | 0.00 | 10.63 | 0.00 | 20.71 | 1.16 | 53 | 34.5 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 12.59 | 0.00 | 0.00 | 11.76 | 0.00 | 24.49 | 0.98 | 54 | 34.8 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 12.45 | 0.00 | 0.00 | 12.07 | 0.00 | 24.64 | 0.97 | 54 | 34.8 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 12.47 | 0.00 | 0.00 | 11.83 | 0.00 | 24.30 | 0.98 | 54 | 34.8 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 12.58 | 0.00 | 0.00 | 12.00 | 0.00 | 24.73 | 0.97 | 54 | 34.8 | 1 | D=1000; exc 250,039 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M50-12 fma: 50% denominator 1000 (three-place decimals in (-10^4, 10^4)), 50% denominators 1..12 (|num| < 10^6)

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 21.33 | 0.00 | 0.00 | 160.29 | 15.14 | 196.82 | 1.00 | 208 | 153.3 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 21.33 | 0.00 | 0.00 | 160.40 | 15.09 | 196.55 | 1.00 | 208 | 153.3 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 21.01 | 0.00 | 0.00 | 134.72 | 41.28 | 198.55 | 0.99 | 208 | 153.3 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 838,963 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 20.76 | 0.00 | 0.00 | 153.74 | 15.28 | 189.47 | 1.04 | 208 | 153.5 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 20.34 | 0.00 | 0.00 | 153.06 | 15.00 | 189.80 | 1.04 | 208 | 153.5 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 19.78 | 0.00 | 0.00 | 154.29 | 15.49 | 190.52 | 1.03 | 208 | 153.5 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 20.28 | 0.00 | 0.00 | 154.99 | 15.37 | 191.19 | 1.03 | 208 | 153.5 | 429 | D=693000; exc 0 in, 838,764 out; 838,963 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 43.62 | 0.00 | 0.00 | 83.57 | 22.41 | 150.10 | 1.00 | 210 | 172.3 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 43.49 | 0.00 | 0.00 | 83.14 | 22.48 | 149.52 | 1.00 | 210 | 172.3 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 32.62 | 0.00 | 0.00 | 42.28 | 39.25 | 114.39 | 1.31 | 249 | 218.3 | 429 | D=1000; exc 751,085 in, 578,612 out; 0 dense ovf; 1 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 45.19 | 0.00 | 0.00 | 78.95 | 23.88 | 148.43 | 1.01 | 211 | 172.5 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 43.49 | 0.00 | 0.00 | 78.70 | 24.38 | 146.42 | 1.03 | 211 | 172.5 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 43.26 | 0.00 | 0.00 | 79.76 | 24.21 | 147.85 | 1.02 | 211 | 172.5 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 43.66 | 0.00 | 0.00 | 80.44 | 23.99 | 148.33 | 1.01 | 211 | 172.5 | 429 | D=1000; exc 751,085 in, 415,306 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M75-12 sum: 75% denominator 1000, 25% denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 6.12 | 0.00 | 0.00 | 1.29 | 0.00 | 7.42 | 1.00 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 5.96 | 0.00 | 0.00 | 1.16 | 0.00 | 7.21 | 1.03 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 5.71 | 0.00 | 0.00 | 0.88 | 0.00 | 6.66 | 1.11 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 6.00 | 0.00 | 0.00 | 0.89 | 0.00 | 6.84 | 1.08 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 5.88 | 0.00 | 0.00 | 0.93 | 0.00 | 6.88 | 1.08 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 5.81 | 0.00 | 0.00 | 0.91 | 0.00 | 6.80 | 1.09 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 5.97 | 0.00 | 0.00 | 0.85 | 0.00 | 6.92 | 1.07 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 8.94 | 0.00 | 0.00 | 6.24 | 0.00 | 15.24 | 1.00 | 43 | 23.0 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 9.07 | 0.00 | 0.00 | 6.25 | 0.00 | 15.41 | 0.99 | 43 | 23.0 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 8.09 | 0.00 | 0.00 | 5.86 | 0.00 | 13.92 | 1.09 | 43 | 23.0 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 9.57 | 0.00 | 0.00 | 6.37 | 0.00 | 15.91 | 0.96 | 43 | 23.3 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 9.28 | 0.00 | 0.00 | 6.61 | 0.00 | 15.95 | 0.96 | 43 | 23.3 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 9.37 | 0.00 | 0.00 | 6.32 | 0.00 | 15.73 | 0.97 | 43 | 23.3 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 9.37 | 0.00 | 0.00 | 6.41 | 0.00 | 15.79 | 0.97 | 43 | 23.3 | 1 | D=1000; exc 124,779 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M75-12 fma: 75% denominator 1000, 25% denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 21.43 | 0.00 | 0.00 | 126.52 | 19.48 | 167.98 | 1.00 | 195 | 153.3 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 21.42 | 0.00 | 0.00 | 126.21 | 19.58 | 167.04 | 1.01 | 195 | 153.3 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 20.93 | 0.00 | 0.00 | 110.38 | 34.69 | 166.31 | 1.01 | 195 | 153.3 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 688,899 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 21.38 | 0.00 | 0.00 | 120.15 | 20.76 | 162.52 | 1.03 | 195 | 153.5 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 21.13 | 0.00 | 0.00 | 119.73 | 20.66 | 161.58 | 1.04 | 195 | 153.5 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 21.01 | 0.00 | 0.00 | 121.36 | 21.06 | 163.34 | 1.03 | 195 | 153.5 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 21.35 | 0.00 | 0.00 | 123.36 | 20.94 | 165.41 | 1.02 | 195 | 153.5 | 427 | D=693000; exc 0 in, 688,714 out; 688,899 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 31.78 | 0.00 | 0.00 | 49.90 | 23.97 | 105.91 | 1.00 | 159 | 114.8 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 32.61 | 0.00 | 0.00 | 50.39 | 24.00 | 107.08 | 0.99 | 159 | 114.8 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 32.02 | 0.00 | 0.00 | 26.50 | 33.68 | 92.26 | 1.15 | 171 | 137.8 | 427 | D=1000; exc 374,613 in, 329,891 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 34.01 | 0.00 | 0.00 | 48.34 | 26.11 | 108.99 | 0.97 | 160 | 115.0 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 33.07 | 0.00 | 0.00 | 47.01 | 26.07 | 106.02 | 1.00 | 160 | 115.0 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 33.24 | 0.00 | 0.00 | 47.88 | 26.64 | 107.67 | 0.98 | 160 | 115.0 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 33.24 | 0.00 | 0.00 | 47.94 | 25.96 | 107.68 | 0.98 | 160 | 115.0 | 427 | D=1000; exc 374,613 in, 226,543 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M95-12 sum: 95% denominator 1000, 5% denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 4.07 | 0.00 | 0.00 | 0.98 | 0.00 | 5.06 | 1.00 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 4.03 | 0.00 | 0.00 | 0.92 | 0.00 | 4.97 | 1.02 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 4.14 | 0.00 | 0.00 | 0.89 | 0.00 | 5.04 | 1.00 | 29 | 11.5 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 4.23 | 0.00 | 0.00 | 0.95 | 0.00 | 5.20 | 0.97 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 4.15 | 0.00 | 0.00 | 0.77 | 0.00 | 4.98 | 1.02 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 3.92 | 0.00 | 0.00 | 0.75 | 0.00 | 4.67 | 1.08 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 4.15 | 0.00 | 0.00 | 0.81 | 0.00 | 4.92 | 1.03 | 30 | 11.8 | 1 | D=693000; exc 0 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 4.66 | 0.00 | 0.00 | 2.03 | 0.00 | 7.14 | 1.00 | 31 | 14.4 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 4.83 | 0.00 | 0.00 | 2.13 | 0.00 | 6.99 | 1.02 | 31 | 14.4 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 4.58 | 0.00 | 0.00 | 1.89 | 0.00 | 6.51 | 1.10 | 31 | 14.4 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 4.92 | 0.00 | 0.00 | 1.99 | 0.00 | 6.90 | 1.04 | 32 | 14.6 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 4.88 | 0.00 | 0.00 | 2.10 | 0.00 | 6.95 | 1.03 | 32 | 14.6 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 4.97 | 0.00 | 0.00 | 2.06 | 0.00 | 6.91 | 1.03 | 32 | 14.6 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 4.88 | 0.00 | 0.00 | 1.97 | 0.00 | 6.93 | 1.03 | 32 | 14.6 | 1 | D=1000; exc 24,960 in, 0 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M95-12 fma: 95% denominator 1000, 5% denominators 1..12

n = 1,000,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 16.13 | 0.00 | 0.00 | 87.39 | 23.97 | 128.08 | 1.00 | 181 | 153.3 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 15.78 | 0.00 | 0.00 | 87.26 | 24.13 | 127.25 | 1.01 | 181 | 153.3 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 16.23 | 0.00 | 0.00 | 83.54 | 32.99 | 133.87 | 0.96 | 181 | 153.3 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 534,743 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 16.09 | 0.00 | 0.00 | 84.58 | 26.09 | 127.36 | 1.01 | 182 | 153.5 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 16.07 | 0.00 | 0.00 | 84.57 | 26.71 | 127.64 | 1.00 | 182 | 153.5 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 15.55 | 0.00 | 0.00 | 85.26 | 26.94 | 127.84 | 1.00 | 182 | 153.5 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 16.29 | 0.00 | 0.00 | 85.28 | 25.79 | 127.96 | 1.00 | 182 | 153.5 | 347 | D=693000; exc 0 in, 534,665 out; 534,743 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 19.05 | 0.00 | 0.00 | 14.28 | 24.40 | 57.89 | 1.00 | 122 | 71.6 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 19.09 | 0.00 | 0.00 | 14.31 | 24.20 | 57.60 | 1.01 | 122 | 71.6 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 18.54 | 0.00 | 0.00 | 9.01 | 26.63 | 54.40 | 1.06 | 124 | 77.4 | 347 | D=1000; exc 75,498 in, 73,600 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 19.34 | 0.00 | 0.00 | 12.65 | 27.04 | 59.61 | 0.97 | 122 | 71.9 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 20.04 | 0.00 | 0.00 | 13.13 | 26.98 | 59.93 | 0.97 | 122 | 71.9 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 19.71 | 0.00 | 0.00 | 13.18 | 27.25 | 60.63 | 0.95 | 122 | 71.9 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 19.20 | 0.00 | 0.00 | 13.09 | 27.37 | 59.99 | 0.97 | 122 | 71.9 | 347 | D=1000; exc 75,498 in, 49,119 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M50-1k fma: 50% denominator 1000, 50% denominators 1..1000 (|num| < 10^6); no column sum (its common denominator is far beyond 64 bits)

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.26 | 0.00 | 0.00 | 30.88 | 2.30 | 50.45 | 1.00 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 17.44 | 0.00 | 0.00 | 31.12 | 2.18 | 50.50 | 1.00 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 11.91 | 0.00 | 0.00 | 19.98 | 8.99 | 41.05 | 1.23 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 173,183 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 15.39 | 0.00 | 0.00 | 28.91 | 2.31 | 46.85 | 1.08 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1-lcm | 15.51 | 0.00 | 0.00 | 28.94 | 2.36 | 46.83 | 1.08 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2-lcm | 15.66 | 0.00 | 0.00 | 29.01 | 2.39 | 47.10 | 1.07 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 15.28 | 0.00 | 0.00 | 29.09 | 2.33 | 46.61 | 1.08 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 17.27 | 0.00 | 0.00 | 31.22 | 2.20 | 50.70 | 1.00 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 17.32 | 0.00 | 0.00 | 31.48 | 2.26 | 50.88 | 1.00 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 11.86 | 0.00 | 0.00 | 19.88 | 8.89 | 40.67 | 1.25 | 65 | 69.1 | 83090 | D=1000; exc 294,682 in, 173,183 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 15.33 | 0.00 | 0.00 | 28.70 | 2.34 | 46.48 | 1.09 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d1-freq | 15.53 | 0.00 | 0.00 | 28.69 | 2.34 | 46.54 | 1.09 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops | 0.00 | ok |
| d2-freq | 15.70 | 0.00 | 0.00 | 29.20 | 2.35 | 47.41 | 1.07 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 15.23 | 0.00 | 0.00 | 29.65 | 2.33 | 47.36 | 1.07 | 66 | 69.4 | 83090 | D=1000; exc 294,682 in, 169,912 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 4 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M75-1k fma: 75% denominator 1000, 25% denominators 1..1000

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 11.08 | 0.00 | 0.00 | 18.83 | 3.48 | 33.39 | 1.00 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 10.99 | 0.00 | 0.00 | 18.76 | 3.43 | 33.11 | 1.01 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 8.50 | 0.00 | 0.00 | 10.83 | 7.46 | 26.81 | 1.25 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 113,793 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 10.60 | 0.00 | 0.00 | 16.59 | 3.74 | 30.85 | 1.08 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d1-lcm | 10.39 | 0.00 | 0.00 | 16.64 | 3.69 | 30.85 | 1.08 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d2-lcm | 10.52 | 0.00 | 0.00 | 16.75 | 3.79 | 31.18 | 1.07 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 10.49 | 0.00 | 0.00 | 16.74 | 3.74 | 31.01 | 1.08 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 10.99 | 0.00 | 0.00 | 18.99 | 3.46 | 33.60 | 1.00 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 11.00 | 0.00 | 0.00 | 18.70 | 3.48 | 33.05 | 1.02 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 8.46 | 0.00 | 0.00 | 10.88 | 7.38 | 26.83 | 1.25 | 47 | 40.3 | 34304 | D=1000; exc 147,513 in, 113,793 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 10.46 | 0.00 | 0.00 | 16.63 | 3.76 | 30.80 | 1.09 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d1-freq | 10.36 | 0.00 | 0.00 | 16.70 | 3.73 | 30.76 | 1.09 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops | 0.00 | ok |
| d2-freq | 10.45 | 0.00 | 0.00 | 16.80 | 3.75 | 31.04 | 1.08 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 10.36 | 0.00 | 0.00 | 16.82 | 3.76 | 31.02 | 1.08 | 47 | 40.6 | 34304 | D=1000; exc 147,513 in, 110,364 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 2 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### M95-1k fma: 95% denominator 1000, 5% denominators 1..1000

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 5.70 | 0.00 | 0.00 | 4.57 | 4.32 | 14.54 | 1.00 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 5.58 | 0.00 | 0.00 | 4.57 | 4.26 | 14.36 | 1.01 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 5.05 | 0.00 | 0.00 | 2.71 | 5.07 | 12.79 | 1.14 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 28,152 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 4.93 | 0.00 | 0.00 | 4.27 | 4.79 | 14.03 | 1.04 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d1-lcm | 4.88 | 0.00 | 0.00 | 4.16 | 4.78 | 13.83 | 1.05 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d2-lcm | 4.89 | 0.00 | 0.00 | 4.28 | 4.84 | 14.05 | 1.03 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 4.84 | 0.00 | 0.00 | 4.25 | 4.88 | 14.04 | 1.04 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 5.55 | 0.00 | 0.00 | 4.54 | 4.24 | 14.35 | 1.00 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 5.55 | 0.00 | 0.00 | 4.58 | 4.32 | 14.44 | 0.99 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 4.98 | 0.00 | 0.00 | 2.64 | 5.05 | 12.72 | 1.13 | 29 | 18.8 | 8191 | D=1000; exc 29,665 in, 28,152 out; 0 dense ovf; 0 early red.; 168 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 5.04 | 0.00 | 0.00 | 4.19 | 4.80 | 14.16 | 1.01 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d1-freq | 4.96 | 0.00 | 0.00 | 4.31 | 4.85 | 14.19 | 1.01 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d2-freq | 4.81 | 0.00 | 0.00 | 4.27 | 4.84 | 13.96 | 1.03 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 4.90 | 0.00 | 0.00 | 4.27 | 4.88 | 14.11 | 1.02 | 29 | 19.1 | 8191 | D=1000; exc 29,665 in, 27,017 out; 0 dense ovf; 168 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### K8-dec chain: chain x <- x*b + c, K = 8: x0, c three-place decimals in (-10, 10), b tenths in (-2, 2)

n = 1,000,000, K = 8

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 17.39 | 0.00 | 0.00 | 12.22 | 27.86 | 58.01 | 1.00 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 17.52 | 0.00 | 0.00 | 12.59 | 27.60 | 57.87 | 1.00 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 17.92 | 0.00 | 0.00 | 12.73 | 27.24 | 58.00 | 1.00 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 16.22 | 0.00 | 0.00 | 12.53 | 30.10 | 59.18 | 0.98 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 16.44 | 0.00 | 0.00 | 12.27 | 30.22 | 58.63 | 0.99 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 15.93 | 0.00 | 0.00 | 13.01 | 30.74 | 59.59 | 0.97 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 15.79 | 0.00 | 0.00 | 12.67 | 30.38 | 58.91 | 0.98 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 17.19 | 0.00 | 0.00 | 12.58 | 27.72 | 57.56 | 1.00 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 17.35 | 0.00 | 0.00 | 12.84 | 27.94 | 58.90 | 0.98 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 17.94 | 0.00 | 0.00 | 12.43 | 27.19 | 57.64 | 1.00 | 105 | 64.9 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 0 early red.; 2 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 16.49 | 0.00 | 0.00 | 12.35 | 29.95 | 59.18 | 0.97 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-freq | 16.15 | 0.00 | 0.00 | 12.91 | 30.37 | 59.31 | 0.97 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-freq | 16.06 | 0.00 | 0.00 | 12.97 | 30.74 | 60.05 | 0.96 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 16.18 | 0.00 | 0.00 | 12.99 | 30.81 | 60.13 | 0.96 | 110 | 65.2 | 130 | D=1000; exc 0 in, 0 out; 0 dense ovf; 2 nodes (0 multi-prime); splits 1 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### K8-mix chain: chain x <- x*b + c, K = 8: x0, c over 1..12 (|num| < 10^4), b over 1..4 with |b| < 2

n = 1,000,000, K = 8

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 20.27 | 0.00 | 0.00 | 13.28 | 52.39 | 86.07 | 1.00 | 109 | 65.1 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 20.48 | 0.00 | 0.00 | 13.26 | 52.22 | 86.38 | 1.00 | 109 | 65.1 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 20.16 | 0.00 | 0.00 | 13.11 | 51.70 | 84.68 | 1.02 | 109 | 65.1 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 1,136 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 21.11 | 0.00 | 0.00 | 12.89 | 56.92 | 90.78 | 0.95 | 110 | 65.4 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-lcm | 20.34 | 0.00 | 0.00 | 12.84 | 57.96 | 91.40 | 0.94 | 110 | 65.4 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-lcm | 20.89 | 0.00 | 0.00 | 13.43 | 58.31 | 93.12 | 0.92 | 110 | 65.4 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 20.43 | 0.00 | 0.00 | 12.99 | 58.80 | 92.94 | 0.93 | 110 | 65.4 | 431 | D=27720; exc 0 in, 1,134 out; 1,136 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 63.52 | 0.00 | 0.00 | 663.70 | 17.77 | 744.93 | 1.00 | 362 | 386.9 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 63.66 | 0.00 | 0.00 | 673.27 | 17.93 | 755.95 | 0.99 | 362 | 386.9 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 38.12 | 0.00 | 0.00 | 568.89 | 36.66 | 645.50 | 1.15 | 385 | 386.9 | 431 | D=3; exc 1,997,796 in, 833,055 out; 0 dense ovf; 0 early red.; 5 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 64.84 | 0.00 | 0.00 | 613.10 | 19.17 | 698.97 | 1.07 | 363 | 387.2 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d1-freq | 64.05 | 0.00 | 0.00 | 616.14 | 19.31 | 700.81 | 1.06 | 363 | 387.2 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops | 0.00 | ok |
| d2-freq | 64.44 | 0.00 | 0.00 | 621.27 | 19.36 | 705.34 | 1.06 | 363 | 387.2 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 63.81 | 0.00 | 0.00 | 623.21 | 19.40 | 707.18 | 1.05 | 363 | 387.2 | 431 | D=3; exc 1,997,796 in, 570,842 out; 0 dense ovf; 5 nodes (0 multi-prime); splits 0 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### U8 fma: unrelated denominators from a smaller set: 8 drawn once from 2..1000 (|num| < 10^6), a*b + c

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 19.04 | 0.00 | 0.00 | 26.91 | 1.59 | 47.63 | 1.00 | 95 | 103.5 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes, 1 decomp splits | 0.00 | ok |
| r0-lcm-null | 19.15 | 0.00 | 0.00 | 27.01 | 1.57 | 48.09 | 0.99 | 95 | 103.5 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes, 1 decomp splits | 0.00 | ok |
| r1-lcm | 9.10 | 0.00 | 0.00 | 19.46 | 8.23 | 36.83 | 1.29 | 95 | 103.5 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 544 early red.; 11 nodes, 1 decomp splits | 0.00 | ok |
| d0-lcm | 20.64 | 0.00 | 0.00 | 23.70 | 1.61 | 46.06 | 1.03 | 95 | 103.8 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d1-lcm | 20.64 | 0.00 | 0.00 | 23.67 | 1.59 | 46.13 | 1.03 | 95 | 103.8 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d2-lcm | 20.84 | 0.00 | 0.00 | 23.80 | 1.57 | 46.08 | 1.03 | 95 | 103.8 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 20.26 | 0.00 | 0.00 | 23.86 | 1.57 | 46.17 | 1.03 | 95 | 103.8 | 2752 | D=5635209559922748720; exc 599,455 in, 200,000 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 17.80 | 0.00 | 0.00 | 29.90 | 1.85 | 49.53 | 1.00 | 88 | 103.5 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes, 1 decomp splits | 0.00 | ok |
| r0-freq-null | 17.71 | 0.00 | 0.00 | 29.84 | 1.77 | 49.11 | 1.01 | 88 | 103.5 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes, 1 decomp splits | 0.00 | ok |
| r1-freq | 8.55 | 0.00 | 0.00 | 19.64 | 8.42 | 36.65 | 1.35 | 88 | 103.5 | 2752 | D=976; exc 524,319 in, 197,675 out; 0 dense ovf; 0 early red.; 11 nodes, 1 decomp splits | 0.00 | ok |
| d0-freq | 18.95 | 0.00 | 0.00 | 26.22 | 1.83 | 47.71 | 1.04 | 89 | 103.8 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d1-freq | 18.66 | 0.00 | 0.00 | 26.06 | 1.74 | 46.37 | 1.07 | 89 | 103.8 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops | 0.00 | ok |
| d2-freq | 19.34 | 0.00 | 0.00 | 26.49 | 1.80 | 47.68 | 1.04 | 89 | 103.8 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 18.61 | 0.00 | 0.00 | 26.26 | 1.81 | 46.86 | 1.06 | 89 | 103.8 | 2752 | D=976; exc 524,319 in, 197,215 out; 0 dense ovf; 11 nodes (0 multi-prime); splits 3 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### U32 fma: unrelated denominators: 32 drawn once from 2..1000, a*b + c

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 18.39 | 0.00 | 0.00 | 32.23 | 1.63 | 52.34 | 1.00 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 18.53 | 0.00 | 0.00 | 32.86 | 1.55 | 53.15 | 0.98 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes, 0 decomp splits | 0.00 | ok |
| r1-lcm | 9.02 | 0.00 | 0.00 | 26.60 | 9.98 | 45.60 | 1.15 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,887 out; 0 dense ovf; 0 early red.; 27 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 18.49 | 0.00 | 0.00 | 30.07 | 1.67 | 50.48 | 1.04 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops | 0.00 | ok |
| d1-lcm | 18.28 | 0.00 | 0.00 | 29.90 | 1.64 | 50.03 | 1.05 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops | 0.00 | ok |
| d2-lcm | 18.89 | 0.00 | 0.00 | 30.41 | 1.60 | 51.11 | 1.02 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 18.27 | 0.00 | 0.00 | 30.62 | 1.69 | 50.38 | 1.04 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |
| r0-freq | 18.56 | 0.00 | 0.00 | 32.16 | 1.54 | 52.60 | 1.00 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes, 0 decomp splits | 0.00 | ok |
| r0-freq-null | 18.40 | 0.00 | 0.00 | 32.23 | 1.63 | 52.42 | 1.00 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes, 0 decomp splits | 0.00 | ok |
| r1-freq | 8.80 | 0.00 | 0.00 | 26.22 | 9.89 | 44.82 | 1.17 | 93 | 103.5 | 55161 | D=973; exc 580,654 in, 199,887 out; 0 dense ovf; 0 early red.; 27 nodes, 0 decomp splits | 0.00 | ok |
| d0-freq | 18.15 | 0.00 | 0.00 | 29.92 | 1.66 | 49.71 | 1.06 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops | 0.00 | ok |
| d1-freq | 18.24 | 0.00 | 0.00 | 29.86 | 1.61 | 49.66 | 1.06 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops | 0.00 | ok |
| d2-freq | 18.84 | 0.00 | 0.00 | 30.48 | 1.65 | 50.95 | 1.03 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-freq | 18.02 | 0.00 | 0.00 | 30.55 | 1.68 | 50.08 | 1.05 | 94 | 103.8 | 55161 | D=973; exc 580,654 in, 199,609 out; 0 dense ovf; 27 nodes (0 multi-prime); splits 5 entry / 0 ops; 0 checks, 0 hits, 0 leaves | 0.00 | ok |


### N100 fma: node-count sweep: 100 distinct denominators (half in [2, 2^31), half in [2^32, 2^40)); a*b + c, c over a's denominator, b over 1..12

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 15.59 | 0.00 | 0.12 | 25.45 | 1.77 | 42.87 | 1.00 | 77 | 80.5 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 184 nodes, 45 decomp splits | 0.00 | ok |
| r0-lcm-null | 15.51 | 0.00 | 0.12 | 25.43 | 1.80 | 42.76 | 1.00 | 77 | 80.5 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 184 nodes, 45 decomp splits | 0.00 | ok |
| d0-lcm | 24.79 | 0.00 | 0.00 | 30.95 | 6.89 | 63.25 | 0.68 | 77 | 80.8 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 177 nodes (7 multi-prime); splits 71 entry / 7 ops | 0.00 | ok |
| d1-lcm | 24.44 | 0.00 | 0.00 | 30.84 | 7.02 | 63.00 | 0.68 | 77 | 80.8 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 177 nodes (7 multi-prime); splits 71 entry / 7 ops | 0.00 | ok |
| d2-lcm | 24.71 | 0.00 | 0.00 | 30.40 | 6.75 | 62.03 | 0.69 | 77 | 80.8 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 177 nodes (7 multi-prime); splits 71 entry / 7 ops; 0 leftovers (0 shared), 0 pieces to rho | 0.00 | ok |
| d3-lcm | 24.68 | 0.00 | 0.00 | 30.53 | 6.41 | 61.55 | 0.70 | 77 | 80.8 | 4530 | D=552573341; exc 395,772 in, 197,886 out; 0 dense ovf; 177 nodes (7 multi-prime); splits 71 entry / 7 ops; 10 checks, 0 hits, 71 leaves | 0.00 | ok |


### N1k fma: node-count sweep: 1,000 distinct denominators

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 49.70 | 0.00 | 1.44 | 26.75 | 1.55 | 78.22 | 1.00 | 84 | 80.6 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,321 nodes, 206 decomp splits | 0.00 | ok |
| r0-lcm-null | 49.70 | 0.00 | 1.43 | 26.69 | 1.63 | 78.10 | 1.00 | 84 | 80.6 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,321 nodes, 206 decomp splits | 0.00 | ok |
| d0-lcm | 31.27 | 0.00 | 0.00 | 31.56 | 4.74 | 67.64 | 1.16 | 77 | 81.0 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,155 nodes (166 multi-prime); splits 168 entry / 17 ops | 0.00 | ok |
| d1-lcm | 29.46 | 0.00 | 0.00 | 31.58 | 4.74 | 65.96 | 1.19 | 77 | 81.0 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,155 nodes (166 multi-prime); splits 168 entry / 17 ops | 0.00 | ok |
| d2-lcm | 30.94 | 0.50 | 0.00 | 31.44 | 4.61 | 67.34 | 1.16 | 78 | 81.0 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,155 nodes (166 multi-prime); splits 168 entry / 17 ops; 23 leftovers (0 shared), 23 pieces to rho | 0.00 | ok |
| d3-lcm | 31.18 | 0.00 | 0.00 | 31.50 | 4.52 | 67.74 | 1.15 | 84 | 81.1 | 29486 | D=774406593; exc 399,514 in, 199,757 out; 0 dense ovf; 1,155 nodes (166 multi-prime); splits 168 entry / 17 ops; 106 checks, 1 hits, 753 leaves | 0.00 | ok |


### N10k fma: node-count sweep: 10,000 distinct denominators

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 2423.10 | 0.00 | 14.27 | 34.45 | 1.61 | 2459.65 | 1.00 | 85 | 78.7 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 10,271 nodes, 671 decomp splits | 0.00 | ok |
| r0-lcm-null | 2428.30 | 0.00 | 14.30 | 34.66 | 1.62 | 2465.61 | 1.00 | 85 | 78.7 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 10,271 nodes, 671 decomp splits | 0.00 | ok |
| d0-lcm | 84.57 | 0.00 | 0.00 | 37.76 | 4.84 | 127.65 | 19.27 | 81 | 80.4 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 9,603 nodes (668 multi-prime); splits 650 entry / 1 ops | 0.00 | ok |
| d1-lcm | 64.66 | 0.00 | 0.00 | 37.81 | 4.98 | 107.51 | 22.88 | 81 | 80.4 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 9,603 nodes (668 multi-prime); splits 650 entry / 1 ops | 0.00 | ok |
| d2-lcm | 80.62 | 4.58 | 0.00 | 37.69 | 5.01 | 127.94 | 19.23 | 81 | 80.4 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 9,603 nodes (668 multi-prime); splits 644 entry / 1 ops; 215 leftovers (2 shared), 205 pieces to rho | 0.00 | ok |
| d3-lcm | 91.28 | 0.00 | 0.00 | 38.18 | 4.87 | 134.53 | 18.28 | 81 | 81.0 | 106277 | D=692852680; exc 399,924 in, 199,959 out; 0 dense ovf; 9,603 nodes (668 multi-prime); splits 650 entry / 1 ops; 1,265 checks, 8 hits, 7,715 leaves | 0.00 | ok |


### N30k fma: node-count sweep: 30,000 distinct denominators

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 18316.99 | 0.00 | 38.83 | 35.46 | 1.68 | 18353.97 | 1.00 | 78 | 79.2 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 27,022 nodes, 854 decomp splits | 0.00 | ok |
| r0-lcm-null | 18300.28 | 0.00 | 38.91 | 36.05 | 1.68 | 18337.57 | 1.00 | 78 | 79.2 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 27,022 nodes, 854 decomp splits | 0.00 | ok |
| d0-lcm | 181.58 | 0.00 | 0.00 | 38.98 | 1.65 | 221.69 | 82.79 | 85 | 83.2 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 26,168 nodes (854 multi-prime); splits 1,248 entry / 0 ops | 0.00 | ok |
| d1-lcm | 123.10 | 0.00 | 0.00 | 38.94 | 1.71 | 163.59 | 112.19 | 86 | 83.2 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 26,168 nodes (854 multi-prime); splits 1,248 entry / 0 ops | 0.00 | ok |
| d2-lcm | 171.39 | 14.25 | 0.00 | 38.81 | 1.72 | 225.56 | 81.37 | 86 | 83.3 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 26,168 nodes (854 multi-prime); splits 1,203 entry / 0 ops; 532 leftovers (28 shared), 454 pieces to rho | 0.00 | ok |
| d3-lcm | 239.53 | 0.00 | 0.00 | 40.64 | 1.66 | 282.66 | 64.93 | 93 | 85.5 | 149306 | D=1465987792; exc 399,964 in, 199,982 out; 0 dense ovf; 26,168 nodes (854 multi-prime); splits 1,248 entry / 0 ops; 3,697 checks, 74 hits, 22,584 leaves | 0.00 | ok |


### P64 fma: 2,000 distinct denominators p*q from a pool of 64 primes in [2^17, 2^24) (each prime in about 62 denominators); a*b + c as N*

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 16.29 | 0.00 | 0.02 | 15.48 | 1.50 | 33.32 | 1.00 | 84 | 80.7 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 16.30 | 0.00 | 0.02 | 15.43 | 1.54 | 33.14 | 1.01 | 84 | 80.7 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 34.60 | 0.00 | 0.00 | 15.95 | 1.58 | 52.08 | 0.64 | 84 | 81.0 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes (0 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d1-lcm | 19.07 | 0.00 | 0.00 | 15.64 | 1.50 | 36.35 | 0.92 | 84 | 81.0 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes (0 multi-prime); splits 17 entry / 0 ops | 0.00 | ok |
| d2-lcm | 34.19 | 3.11 | 0.00 | 15.64 | 1.48 | 54.22 | 0.61 | 85 | 81.1 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes (0 multi-prime); splits 17 entry / 0 ops; 2,000 leftovers (2,000 shared), 16 pieces to rho | 0.00 | ok |
| d3-lcm | 38.12 | 0.00 | 0.00 | 15.30 | 1.44 | 54.79 | 0.61 | 84 | 81.0 | 22085 | D=56090911180541; exc 399,732 in, 199,866 out; 0 dense ovf; 69 nodes (0 multi-prime); splits 17 entry / 0 ops; 1,999 checks, 1,984 hits, 64 leaves | 0.00 | ok |


### P256 fma: 2,000 p*q from a pool of 256 primes (each in about 16)

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 30.69 | 0.00 | 0.06 | 15.61 | 1.55 | 48.18 | 1.00 | 84 | 80.7 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes, 0 decomp splits | 0.00 | ok |
| r0-lcm-null | 30.70 | 0.00 | 0.06 | 15.45 | 1.50 | 47.73 | 1.01 | 84 | 80.7 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes, 0 decomp splits | 0.00 | ok |
| d0-lcm | 37.38 | 0.00 | 0.00 | 15.90 | 1.59 | 54.87 | 0.88 | 84 | 81.0 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes (0 multi-prime); splits 61 entry / 0 ops | 0.00 | ok |
| d1-lcm | 21.47 | 0.00 | 0.00 | 15.71 | 1.52 | 38.94 | 1.24 | 84 | 81.0 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes (0 multi-prime); splits 61 entry / 0 ops | 0.00 | ok |
| d2-lcm | 34.69 | 5.74 | 0.00 | 15.92 | 1.65 | 57.88 | 0.83 | 85 | 81.1 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes (0 multi-prime); splits 61 entry / 0 ops; 2,000 leftovers (2,000 shared), 60 pieces to rho | 0.00 | ok |
| d3-lcm | 41.90 | 0.00 | 0.00 | 15.56 | 1.60 | 59.39 | 0.81 | 84 | 81.0 | 22065 | D=60379112161987; exc 399,734 in, 199,867 out; 0 dense ovf; 261 nodes (0 multi-prime); splits 61 entry / 0 ops; 1,999 checks, 1,940 hits, 256 leaves | 0.00 | ok |


### P1024 fma: 2,000 p*q from a pool of 1,024 primes (each in about 4)

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 79.12 | 0.00 | 0.28 | 15.63 | 1.58 | 96.28 | 1.00 | 84 | 80.7 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 1,000 nodes, 2 decomp splits | 0.00 | ok |
| r0-lcm-null | 79.14 | 0.00 | 0.28 | 15.65 | 1.58 | 96.52 | 1.00 | 84 | 80.7 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 1,000 nodes, 2 decomp splits | 0.00 | ok |
| d0-lcm | 49.00 | 0.00 | 0.00 | 18.24 | 1.58 | 68.86 | 1.40 | 84 | 81.1 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 998 nodes (2 multi-prime); splits 244 entry / 0 ops | 0.00 | ok |
| d1-lcm | 33.42 | 0.00 | 0.00 | 18.32 | 1.54 | 53.43 | 1.80 | 85 | 81.1 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 998 nodes (2 multi-prime); splits 244 entry / 0 ops | 0.00 | ok |
| d2-lcm | 37.47 | 15.39 | 0.00 | 18.42 | 1.58 | 73.18 | 1.32 | 79 | 81.2 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 998 nodes (2 multi-prime); splits 244 entry / 0 ops; 2,000 leftovers (1,998 shared), 230 pieces to rho | 0.00 | ok |
| d3-lcm | 56.99 | 0.00 | 0.00 | 18.14 | 1.58 | 76.66 | 1.26 | 85 | 81.2 | 22059 | D=75826431688639; exc 399,730 in, 199,865 out; 0 dense ovf; 998 nodes (2 multi-prime); splits 244 entry / 0 ops; 1,999 checks, 1,754 hits, 995 leaves | 0.00 | ok |


### P4000 fma: 2,000 p*q from 4,000 primes in disjoint pairs (no prime shared)

n = 200,000

| row | entry | idle | decomp | op | final | total | R0 / x | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| r0-lcm | 277.35 | 0.00 | 102.16 | 15.93 | 1.46 | 294.86 | 1.00 | 84 | 80.9 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 4,005 nodes, 2,000 decomp splits | 0.00 | ok |
| r0-lcm-null | 277.47 | 0.00 | 101.91 | 15.87 | 1.55 | 295.22 | 1.00 | 84 | 80.9 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 4,005 nodes, 2,000 decomp splits | 0.00 | ok |
| d0-lcm | 140.41 | 0.00 | 0.00 | 20.13 | 1.66 | 162.23 | 1.82 | 85 | 81.3 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 2,005 nodes (2,000 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d1-lcm | 125.54 | 0.00 | 0.00 | 20.42 | 1.69 | 147.29 | 2.00 | 85 | 81.3 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 2,005 nodes (2,000 multi-prime); splits 1 entry / 0 ops | 0.00 | ok |
| d2-lcm | 36.37 | 108.37 | 0.00 | 20.25 | 1.65 | 167.15 | 1.76 | 85 | 81.4 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 2,005 nodes (2,000 multi-prime); splits 1 entry / 0 ops; 2,000 leftovers (0 shared), 2,000 pieces to rho | 0.00 | ok |
| d3-lcm | 144.35 | 0.00 | 0.00 | 19.94 | 1.61 | 165.91 | 1.78 | 79 | 81.6 | 22106 | D=174941392179871; exc 399,714 in, 199,857 out; 0 dense ovf; 2,005 nodes (2,000 multi-prime); splits 1 entry / 0 ops; 1,999 checks, 0 hits, 4,000 leaves | 0.00 | ok |


## Simplifications that may bias the results

- R0 and R1 are round 2's V3 and V34 binaries' code (rebuilt from the same sources; checked with objdump to be the same machine code, as are V0', V1, V2, V4, V5, V6 and the 32-bit builds; test_round3.py checks it against commit 97f4cde).  The round-3 edits to shared files are preprocessor switches (AB_DICT, AB_V3L, VARIANT >= VAR_D0) and the new file ab_d.c.
- D0 is not round 2's V6: it is V6's entry on V3's layout.  V6's nodes need not be prime and splits kill nodes, so D0-D3 add one piece of glue V3 lacks: each array records the split count when its edges were last live, and an op re-expands its operands' edges (D and the exceptions) onto live nodes first if a split happened since; the final reduction does the same when it splits a node.  The cost is one compare per operand per op when nothing split, and it is in every D row alike.
- V3's dense elements are not reduced at entry, so V6's fill-time splits (a numerator sharing part of a multi-prime node) happen in D rows only for exceptions; dense elements meet such nodes first in the final reduction (timed).
- D1 changes one thing: the small-prime table is tested in branch-free blocks of 8 (clang vectorises the block with -march=native), the adaptive stop checked once per block, one branch per block.  The per-prime test is D0's (multiply by the inverse, compare); D0 adds a multiply and a branch per prime for the stop.  A composite large leftover still goes to pg_factor, whose second pass over the table D1 keeps, as D0 does.  GMP-style packing was measured in isolation before this round (200,000 one-word denominators each, ns per denominator): full-table scans of 40-bit numbers without small factors 10,793 (D0's loop), 2,583 (blocks), 2,573 (blocks + n mod 3*5*...*53 with the 15 tiny primes tested on the residue), 2,987 (32-bit lanes when n < 2^32); random 2..2^31: 1,019 / 256 / 264 / 262; random 1..1000: 17.2 / 16.4 / 18.3 / 17.4.  Packing did not help one-word denominators, so D1 does not pack.
- D2 drops only rho at entry: Miller-Rabin stays, so a prime leftover is entered as in D0, and only a composite leftover is deferred, appended once per value (a hash), with no gcd at entry.  Its primes are unknown until the idle pass, which runs in the driver's decomposition slot between entry's pass 1 and pass 2, so the fill and the operations see a factored graph, as in D0.  The idle pass does a batch gcd over the deferred leftovers (GMP: product tree, remainder tree modulo the squares; GMP's own allocations are not in the repr column), splits each leftover that shares into gcd and cofactor, divides a piece still composite by the large primes known so far (D0's step 1b loop; primes merged earlier in the pass count, so a leftover whose primes are all shared, gcd = itself, is still resolved without rho once another leftover has revealed them), sends only what is still composite to rho (pg_factor, D0's code: its table pass included), and merges each leftover's primes through the dictionary with D0's rule, in entry order.  D2 saves rho where shared primes resolve a leftover; it pays the batch gcd always.  A leftover's primes are grouped among themselves, not with the small primes first seen in the same denominator (D0 groups them), so D2's graph can be finer.
- D3's forest holds every large prime (2^16 or more) entered in the dictionary, the same set D0's step 1b loops over, as perfect subtrees of up to 64 leaves (roots merged as leaves arrive); the remainders use a precomputed reciprocal of the leftover (Moller-Granlund), and roots are combined by modular products.  D0's step 1b is already one multiply and one compare per known prime with an early exit below 2^32, so the product check competes with a cheap loop, not with per-node gcds; it is not capped and never rebuilt (no prime leaves the dictionary).
- Entry is timed from the raw inputs and includes the dictionary passes, the fill (V3's choice of D, scaling, exception entry), and for R0 the gcd scan and pg_factor.  Per-rep resets (node tables, the dictionary's 256 KB direct-mapped array and hash, D2's leftover list and hash, D3's forest) are outside the timer; table capacities carry over between warm reps, and the minimum over reps is kept.
- Phase columns are minima over an invocation's reps taken phase by phase, possibly from different reps, and the detail tables' entry for D2 is (minimum entry including idle) - (minimum idle): a difference of minima from possibly different reps, so it can understate or overstate D2's entry alone, and the phases need not add up to total (the minimum over reps of one rep's sum).  The verdicts use total only.
- The band is conservative: h is at least 5% and widens to the null spread, so small real differences are called ties.  The report makes many comparisons (8 per job-rule pair, about 90 pairs) without a multiple-comparison correction, so some per-pair 'better' or 'worse' calls are expected by chance; the overall answers (rule 7-8) turn one 'worse' into 'mixed' or 'no', which makes them sensitive to such a call.  The bootstrap resamples rounds, so it captures round-to-round noise but not effects shared by every round (machine, compiler, data); the null band comes from one row (R0) per job and rule, so a row much noisier than R0 could be called better or worse where a wider band would say tie; with 12 rounds the median moves in discrete steps.
- Run length: 12 rounds, 2 reps per invocation (1 for N10k and N30k, where R0's entry takes seconds: it gcd-refines each new denominator against every live node).  The Q2-only workloads (N*, P*) run the lcm rule only and no R1; W4 sum is dropped (unable for every row).  The sweep's denominators are half below 2^31 and half in [2^32, 2^40); the P workloads isolate two-large-prime denominators.  Neither is drawn from real data.
- Workloads W1-W4, W6-W10 and S8-S30d are round 2's data (same seed, same generation order, W5 generated but not run).  The new workloads come after them from the same generator, so changing their order or sizes changes their data but not the old ones.
- Every rule of round 2's checks applies to the timed invocations (fractions, unreduced, overflow).  The audit invocations (AB_AUDIT, one per job and row after the timed rounds, on the last rep's graph and arrays) check the structure only: their outputs are not checked against fractions.
