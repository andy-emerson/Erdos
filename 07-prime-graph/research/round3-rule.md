# Round three: pre-registered decision rule

Oct 27, 2026 · written by `test/ablation/round3_bench.py --write-rule` before any measured run of round three.  The measured run refuses to start unless this file contains the rule below verbatim.

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


## Run plan

12 rounds, 2 reps per invocation unless noted, seed 20261027, 10,000 bootstrap resamples.  Rows: R0 and its null copy, R1, D0-D3, under the lcm and freq rules on array jobs (single-value jobs: one plain sub-row; Q2-aimed workloads: lcm only, no R1).  Workloads (aim): 

- W1 (existing): sum; n = 1,000,000
- W2 (existing): fma; n = 1,000,000
- W3 (existing): sum, fma; n = 1,000,000
- W4 (existing): fma; n = 200,000
- W6 (existing): chain; n = 1,000,000
- W7 (existing): fma; n = 200,000
- W8 (existing): sum, fma; n = 1,000,000
- W9 (existing): sum, fma; n = 1,000,000
- W10 (existing): fma2; n = 1,000,000
- S8 (existing): add, mul; n = 1,000,000
- S15 (existing): add, mul; n = 1,000,000
- S20 (existing): add, mul; n = 1,000,000
- S30 (existing): add, mul; n = 1,000,000
- S30d (existing): add, mul; n = 1,000,000
- M50-12 (Q1): sum, fma; n = 1,000,000
- M75-12 (Q1): sum, fma; n = 1,000,000
- M95-12 (Q1): sum, fma; n = 1,000,000
- M50-1k (Q1): fma; n = 200,000
- M75-1k (Q1): fma; n = 200,000
- M95-1k (Q1): fma; n = 200,000
- K8-dec (Q1): chain; n = 1,000,000
- K8-mix (Q1): chain; n = 1,000,000
- U8 (Q1): fma; n = 200,000
- U32 (Q1): fma; n = 200,000
- N100 (Q2): fma; n = 200,000
- N1k (Q2): fma; n = 200,000
- N10k (Q2): fma; n = 200,000; 1 rep
- N30k (Q2): fma; n = 200,000; 1 rep
- P64 (Q2): fma; n = 200,000
- P256 (Q2): fma; n = 200,000
- P1024 (Q2): fma; n = 200,000
- P4000 (Q2): fma; n = 200,000
