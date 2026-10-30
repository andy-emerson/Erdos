# Prime graph: ablation, round 2

Generated 2026-10-27 22:41 by `round2_bench.py` on Intel(R) Xeon(R) Processor; clang 18 `-std=c2x -O3 -march=native`.  5 interleaved rounds; each invocation runs 3 reps in one warm process (every rep re-enters the values from the raw inputs and resets the node table) and reports the minimum of each phase over its reps.  Tables give the median over rounds.  Times in ms.

**Baseline: V0', the control (grouping removed).** **V0' / x (total)** is the control's total over the row's total, final reduction included; **V0' / x (op)** is the control's op + final over the row's op + final.  Above 1, the row is faster than the control.  V34 deliberately combines two ideas (V3 and V4); every other row is the control plus one idea, as in the first round.

**Same-code noise floor:** v0p-rerun is the control's binary run again as its own row, so its V0' / x (total) on the 24 jobs measures noise between identical code: 0.95 to 1.01 (median 1.00).  A ratio within that range of 1 counts as no difference.

## Part A: V3 + V4 (V34) against its parts

V0' / x (total).  The last two columns compare V34 with V3 under the same rule for D: V3 total / V34 total (above 1, the combination is faster than V3 alone).  Cells without a ratio say why: **not run** (the row was not run on this job), **unable** (the row or the control could not compute the result within its range: a column sum beyond 64 bits), **fail** (the row or the control failed a check; the detail table gives the reason).

| job | V3 lcm | V3 freq | V4 | V34 lcm | V34 freq | V3 / V34 (lcm) | V3 / V34 (freq) |
|---|---:|---:|---:|---:|---:|---:|---:|
| W1 sum | 11.11 | 11.83 | 2.38 | 12.12 | 12.53 | 1.09 | 1.06 |
| W2 fma | 3.40 | 3.38 | 1.47 | 3.44 | 3.45 | 1.01 | 1.02 |
| W3 sum | 14.61 | 1.79 | 1.41 | 14.10 | 2.26 | 0.96 | 1.27 |
| W3 fma | 1.52 | 0.87 | 0.98 | 1.43 | 0.94 | 0.94 | 1.08 |
| W4 sum | unable | unable | unable | unable | unable | unable | unable |
| W4 fma | 0.91 | 0.92 | 1.13 | 1.05 | 1.06 | 1.16 | 1.15 |
| W6 chain | 6.64 | 6.68 | 1.30 | 6.81 | 6.73 | 1.02 | 1.01 |
| W7 fma | 0.94 | 0.89 | 0.97 | 0.79 | 0.88 | 0.84 | 0.99 |
| W8 sum | 9.76 | 10.16 | 2.21 | 9.58 | 10.74 | 0.98 | 1.06 |
| W8 fma | 3.48 | 3.45 | 1.49 | 3.42 | 3.45 | 0.98 | 1.00 |
| W9 sum | 11.45 | 7.94 | 2.11 | 12.01 | 8.89 | 1.05 | 1.12 |
| W9 fma | 1.05 | 2.21 | 1.36 | 1.03 | 2.49 | 0.98 | 1.13 |
| W10 fma2 | 4.44 | 4.37 | 1.62 | 4.39 | 4.49 | 0.99 | 1.03 |

## Part B: single values by operand size

Against the 64-bit control on the same job.  32-bit rows are the int32 build (`-DAB_32`).  Cells without a ratio say why: **not run** (the row was not run on this job), **unable** (the row or the control could not compute the result within its range: a column sum beyond 64 bits), **fail** (the row or the control failed a check; the detail table gives the reason).

**V0' / x (total).**

| size, job | V1 | V5 | V6 | V0' 32-bit | V1 32-bit | V5 32-bit |
|---|---:|---:|---:|---:|---:|---:|
| S8 add | 0.98 | 1.11 | 0.92 | 1.06 | 1.03 | 1.20 |
| S8 mul | 1.02 | 1.10 | 0.93 | 1.04 | 1.03 | 1.15 |
| S15 add | 0.96 | 0.93 | 0.96 | 1.09 | 1.03 | 1.00 |
| S15 mul | 1.00 | 0.96 | 0.98 | 1.06 | 1.05 | 1.01 |
| S20 add | 0.95 | 0.60 | 0.98 | 1.09 | 1.02 | 0.70 |
| S20 mul | 1.01 | 0.68 | 0.98 | fail | fail | fail |
| S30 add | 0.98 | 0.70 | 3.53 | not run | not run | not run |
| S30 mul | 1.00 | 0.73 | 3.62 | not run | not run | not run |
| S30d add | 0.94 | 0.94 | 0.96 | not run | not run | not run |
| S30d mul | 1.01 | 0.91 | 0.97 | not run | not run | not run |

**V0' / x (op)**: op + final only, without entry (on S30 the control's total is almost all entry).

| size, job | V1 | V5 | V6 | V0' 32-bit | V1 32-bit | V5 32-bit |
|---|---:|---:|---:|---:|---:|---:|
| S8 add | 0.97 | 1.27 | 0.96 | 1.13 | 1.08 | 1.41 |
| S8 mul | 1.02 | 1.27 | 1.00 | 1.10 | 1.08 | 1.27 |
| S15 add | 0.93 | 0.90 | 0.98 | 1.16 | 1.04 | 0.96 |
| S15 mul | 1.00 | 0.94 | 0.99 | 1.11 | 1.11 | 0.98 |
| S20 add | 0.91 | 0.48 | 1.03 | 1.17 | 1.03 | 0.57 |
| S20 mul | 1.02 | 0.57 | 1.03 | fail | fail | fail |
| S30 add | 0.89 | 0.27 | 1.00 | not run | not run | not run |
| S30 mul | 1.01 | 0.28 | 0.99 | not run | not run | not run |
| S30d add | 0.90 | 0.94 | 1.00 | not run | not run | not run |
| S30d mul | 1.01 | 0.87 | 0.99 | not run | not run | not run |

**The same strategy at both widths** (64-bit time / 32-bit time; above 1, the 32-bit build is faster), and the share of the 32-bit outputs that overflowed int32 (each one genuine: its exact value does not fit; over 0.1% fails the strict rule).

| size, job | V0' 64 / 32 (total) | V1 64 / 32 (total) | V1 64 / 32 (op) | V5 64 / 32 (total) | V5 64 / 32 (op) | ovf V0' / V1 / V5 32-bit |
|---|---:|---:|---:|---:|---:|---|
| S8 add | 1.06 | 1.05 | 1.12 | 1.09 | 1.11 | 0.00% / 0.00% / 0.00% |
| S8 mul | 1.04 | 1.02 | 1.05 | 1.04 | 1.00 | 0.00% / 0.00% / 0.00% |
| S15 add | 1.09 | 1.07 | 1.13 | 1.07 | 1.06 | 0.00% / 0.00% / 0.00% |
| S15 mul | 1.06 | 1.05 | 1.10 | 1.06 | 1.05 | 0.00% / 0.00% / 0.00% |
| S20 add | 1.09 | 1.07 | 1.13 | 1.16 | 1.19 | 0.00% / 0.00% / 0.00% |
| S20 mul | fail | fail | fail | fail | fail | 86.28% / 86.28% / 86.28% |

## Part C: V6, the seen-prime dictionary

Entry is conversion from the raw inputs.  V0': the gcd scan of every live node per new denominator, pg_factor on every node, the edge scan, then the fill (each value reduced).  V6: the **dictionary passes** (pass 1: the shared per-element denominator hash; per distinct denominator, trial division, exact division by the large primes already in the dictionary, Miller-Rabin and rho on what remains, dictionary lookups and splits; pass 2: each distinct denominator's edges by lookups), then the **fill** (each value reduced, and the edges of elements filled before a fill-time split rewritten).  Nodes are the live nodes after the run; splits count node splits at entry and in the operations.  Cells without a ratio say why: **not run** (the row was not run on this job), **unable** (the row or the control could not compute the result within its range: a column sum beyond 64 bits), **fail** (the row or the control failed a check; the detail table gives the reason).

| job | V0' / V6 (total) | V0' / V6 (op) | entry V0' ms | entry V6 ms | V6 dictionary ms | V6 fill ms | V0' / V6 (entry) | nodes V0' | nodes V6 | V6 nodes of several primes | V6 splits (entry / ops) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| W1 sum | 0.98 | 0.98 | 23.62 | 24.00 | 1.13 | 22.82 | 0.98 | 2 | 2 | 0 | 1 / 0 |
| W2 fma | 0.99 | 1.01 | 69.88 | 72.40 | 3.37 | 68.98 | 0.97 | 2 | 2 | 0 | 1 / 0 |
| W3 sum | 0.98 | 1.00 | 23.09 | 24.35 | 2.51 | 21.80 | 0.95 | 5 | 5 | 0 | 0 / 0 |
| W3 fma | 0.97 | 0.99 | 68.59 | 72.17 | 7.73 | 64.42 | 0.95 | 5 | 5 | 0 | 0 / 0 |
| W4 sum | unable | unable | | | | | | | | | |
| W4 fma | 0.99 | 0.98 | 21.08 | 21.43 | 4.21 | 17.19 | 0.98 | 168 | 168 | 0 | 4 / 0 |
| W5 add | 0.98 | 1.00 | 44.54 | 46.92 | 5.11 | 41.82 | 0.95 | 5 | 5 | 0 | 0 / 0 |
| W5 mul | 0.96 | 1.00 | 44.54 | 47.32 | 5.23 | 42.08 | 0.94 | 5 | 5 | 0 | 0 / 0 |
| W6 chain | 1.00 | 1.01 | 69.87 | 71.53 | 3.25 | 68.22 | 0.98 | 2 | 2 | 0 | 1 / 0 |
| W7 fma | 0.88 | 0.87 | 25.88 | 29.18 | 16.77 | 12.38 | 0.89 | 125 | 101 | 24 | 17 / 0 |
| W8 sum | 0.98 | 0.98 | 22.25 | 22.87 | 1.14 | 21.71 | 0.97 | 2 | 2 | 0 | 2 / 0 |
| W8 fma | 0.99 | 1.02 | 66.01 | 68.52 | 3.34 | 65.13 | 0.96 | 2 | 2 | 0 | 2 / 0 |
| W9 sum | 0.97 | 0.98 | 24.52 | 25.98 | 2.09 | 23.83 | 0.94 | 5 | 5 | 0 | 1 / 0 |
| W9 fma | 0.97 | 1.00 | 73.97 | 77.49 | 6.07 | 71.12 | 0.95 | 5 | 5 | 0 | 1 / 0 |
| W10 fma2 | 0.99 | 1.01 | 110.31 | 114.17 | 5.60 | 108.55 | 0.97 | 2 | 2 | 0 | 1 / 0 |
| S8 add | 0.92 | 0.96 | 42.76 | 48.38 | 5.07 | 43.26 | 0.88 | 5 | 5 | 0 | 0 / 0 |
| S8 mul | 0.93 | 1.00 | 42.31 | 48.07 | 5.12 | 42.93 | 0.88 | 5 | 5 | 0 | 0 / 0 |
| S15 add | 0.96 | 0.98 | 57.06 | 60.53 | 7.83 | 52.40 | 0.94 | 54 | 54 | 0 | 2 / 0 |
| S15 mul | 0.98 | 0.99 | 57.17 | 59.41 | 7.81 | 51.60 | 0.96 | 54 | 54 | 0 | 2 / 0 |
| S20 add | 0.98 | 1.03 | 66.31 | 70.78 | 13.54 | 57.23 | 0.94 | 168 | 168 | 0 | 4 / 0 |
| S20 mul | 0.98 | 1.03 | 66.28 | 70.89 | 13.59 | 57.21 | 0.93 | 168 | 168 | 0 | 4 / 0 |
| S30 add | 3.53 | 1.00 | 554.79 | 103.13 | 19.29 | 83.53 | 5.38 | 3,512 | 3,512 | 0 | 15 / 0 |
| S30 mul | 3.62 | 0.99 | 555.75 | 103.31 | 19.28 | 83.95 | 5.38 | 3,512 | 3,512 | 0 | 15 / 0 |
| S30d add | 0.96 | 1.00 | 58.03 | 62.25 | 5.21 | 56.84 | 0.93 | 55 | 55 | 0 | 9 / 0 |
| S30d mul | 0.97 | 0.99 | 58.67 | 62.15 | 5.30 | 56.83 | 0.94 | 55 | 55 | 0 | 9 / 0 |

## Audits

One untimed invocation per job for V6 and V34 with `AB_AUDIT=1` after the timed rounds: the live nodes are pairwise coprime, plus each layout's structural audit (V6: the dictionary and every edge re-expanded onto live nodes; V34: D, the exception list).

| job | v34-lcm | v34-freq | v6 |
|---|---|---|---|
| W1 sum | pass | pass | pass |
| W2 fma | pass | pass | pass |
| W3 sum | pass | pass | pass |
| W3 fma | pass | pass | pass |
| W4 sum | unable | unable | unable |
| W4 fma | pass | pass | pass |
| W5 add | not run | not run | pass |
| W5 mul | not run | not run | pass |
| W6 chain | pass | pass | pass |
| W7 fma | pass | pass | pass |
| W8 sum | pass | pass | pass |
| W8 fma | pass | pass | pass |
| W9 sum | pass | pass | pass |
| W9 fma | pass | pass | pass |
| W10 fma2 | pass | pass | pass |
| S8 add | not run | not run | pass |
| S8 mul | not run | not run | pass |
| S15 add | not run | not run | pass |
| S15 mul | not run | not run | pass |
| S20 add | not run | not run | pass |
| S20 mul | not run | not run | pass |
| S30 add | not run | not run | pass |
| S30 mul | not run | not run | pass |
| S30d add | not run | not run | pass |
| S30d mul | not run | not run | pass |

## Rows

- `v0p`: V0' (control): grouping removed; every op merges its operands' edges, scales and reduces
- `v0p-rerun`: the control again: the same binary as v0p, run as its own row (the same-code noise floor)
- `v1`: V1 = V0' + per-value denominator check (equal: one integer op; one divides the other: one scaling)
- `v3-lcm`: V3 = V0' + array denominator D with sparse exceptions, D from the lcm (when below 2^63)
- `v3-freq`: V3 with D from the most frequent denominator
- `v4`: V4 = V0' + deferred reduction (overflow: reduce the operands and retry; one final reduction)
- `v34-lcm`: V34 = V0' + V3 (D from the lcm) + V4: exceptions entered unreduced; the graph path takes operands as they stand (a dense slot as n/D) with V4's ops (overflow: reduce and retry); results stored unreduced; one timed final reduction of dense elements and exceptions
- `v34-freq`: V34 with D from the most frequent denominator
- `v5`: V5 = V0' + interned denominator objects with reference counts
- `v6`: V6 = V0' + the seen-prime dictionary as the entry procedure (trial division, then the large primes already seen, then Miller-Rabin and rho on the rest; a prime splits its holder only when seen again; edges by dictionary lookups)
- `v0p-32`: V0' built with -DAB_32 (int32 numerators, denominators below 2^31, int64 intermediates)
- `v1-32`: V1 built with -DAB_32
- `v5-32`: V5 built with -DAB_32

Columns of the detail tables are those of `results/ablation.md`: **entry** includes decomposition (shown again as **decomp**; 0 for V6, which factors each distinct denominator as it enters, inside its dictionary pass); **final** is the timed final reduction (V3, V4, V34; 0 elsewhere); **total** is the minimum over reps of entry + op + final in one rep; **repr** is the measured peak of the representation's allocations; **unreduced %** is the share of outputs not in lowest terms (round 1, every output; any fails the row); overflowed elements ("0 0") are counted, not checked: a row fails if over 0.1% overflowed or any overflow was false; ‡ marks rows with some.

## W1 sum: decimal column: sum of three-place decimals in (-10^4, 10^4)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 23.62 | 0.00 | 33.75 | 0.00 | 57.55 | 1.00 | 1.00 | 49 | 31.5 | 1 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 23.62 | 0.00 | 33.78 | 0.00 | 57.54 | 1.00 | 1.00 | 49 | 31.5 | 1 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 4.04 | 0.00 | 1.10 | 0.00 | 5.18 | 11.11 | 30.79 | 29 | 11.5 | 1 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 3.79 | 0.00 | 1.07 | 0.00 | 4.86 | 11.83 | 31.44 | 29 | 11.5 | 1 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 6.13 | 0.00 | 18.02 | 0.00 | 24.20 | 2.38 | 1.87 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v34-lcm | 3.93 | 0.00 | 0.95 | 0.00 | 4.75 | 12.12 | 35.57 | 29 | 11.5 | 1 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 3.80 | 0.00 | 0.92 | 0.00 | 4.59 | 12.53 | 36.88 | 29 | 11.5 | 1 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 24.00 | 0.00 | 34.56 | 0.00 | 58.63 | 0.98 | 0.98 | 49 | 31.8 | 1 | 2 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## W2 fma: decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 69.88 | 0.00 | 67.07 | 0.00 | 136.95 | 1.00 | 1.00 | 169 | 122.1 | 49 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 69.83 | 0.00 | 67.10 | 0.00 | 137.20 | 1.00 | 1.00 | 168 | 122.1 | 49 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 15.10 | 0.00 | 2.42 | 22.80 | 40.33 | 3.40 | 2.66 | 105 | 57.3 | 49 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.33 | 0.00 | 2.39 | 22.88 | 40.55 | 3.38 | 2.65 | 105 | 57.3 | 49 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 18.86 | 0.00 | 45.03 | 29.12 | 93.00 | 1.47 | 0.90 | 170 | 122.1 | 49 | 0 early reductions | 0.00 | ok |
| v34-lcm | 15.26 | 0.00 | 2.36 | 22.32 | 39.86 | 3.44 | 2.72 | 105 | 57.3 | 49 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 15.10 | 0.00 | 2.43 | 22.25 | 39.74 | 3.45 | 2.72 | 105 | 57.3 | 49 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 72.40 | 0.00 | 66.52 | 0.00 | 138.91 | 0.99 | 1.01 | 169 | 122.4 | 49 | 2 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## W3 sum: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 23.09 | 0.00 | 52.41 | 0.00 | 75.46 | 1.00 | 1.00 | 45 | 31.5 | 1 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 22.95 | 0.00 | 52.25 | 0.00 | 75.31 | 1.00 | 1.00 | 45 | 31.5 | 1 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v3-lcm | 3.97 | 0.00 | 1.21 | 0.00 | 5.16 | 14.61 | 43.14 | 29 | 11.5 | 1 | input D after entry=27720 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.90 | 0.00 | 26.29 | 0.00 | 42.19 | 1.79 | 1.99 | 74 | 57.5 | 1 | input D after entry=12 (freq); 499,523 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 12.45 | 0.00 | 41.06 | 0.00 | 53.56 | 1.41 | 1.28 | 46 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v34-lcm | 4.11 | 0.00 | 1.22 | 0.00 | 5.35 | 14.10 | 42.84 | 29 | 11.5 | 1 | input D after entry=27720 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 11.80 | 0.00 | 21.51 | 0.00 | 33.33 | 2.26 | 2.44 | 74 | 57.5 | 1 | input D after entry=12 (freq); 499,523 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 24.35 | 0.00 | 52.49 | 0.00 | 77.07 | 0.98 | 1.00 | 45 | 31.8 | 1 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |


## W3 fma: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 68.59 | 0.00 | 80.05 | 0.00 | 149.57 | 1.00 | 1.00 | 156 | 122.1 | 166 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 68.98 | 0.00 | 80.33 | 0.00 | 151.17 | 0.99 | 1.00 | 156 | 122.1 | 166 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v3-lcm | 14.64 | 0.00 | 45.85 | 36.81 | 98.25 | 1.52 | 0.97 | 138 | 105.3 | 166 | input D after entry=27720 (lcm); 0 exceptions in, 300,698 out; 300,700 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 60.22 | 0.00 | 97.43 | 13.76 | 172.08 | 0.87 | 0.72 | 374 | 379.3 | 166 | input D after entry=12 (freq); 2,082,070 exceptions in, 669,944 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 37.30 | 0.00 | 77.23 | 37.17 | 152.42 | 0.98 | 0.70 | 162 | 129.7 | 166 | 0 early reductions | 0.00 | ok |
| v34-lcm | 14.82 | 0.00 | 43.17 | 46.02 | 104.54 | 1.43 | 0.90 | 138 | 105.3 | 166 | input D after entry=27720 (lcm); 0 exceptions in, 300,698 out; 300,700 dense overflows; 0 ops all-graph; 300,700 early reductions | 0.00 | ok |
| v34-freq | 38.08 | 0.00 | 84.31 | 37.49 | 159.76 | 0.94 | 0.66 | 393 | 379.3 | 166 | input D after entry=12 (freq); 2,082,070 exceptions in, 890,021 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 72.17 | 0.00 | 81.17 | 0.00 | 153.45 | 0.97 | 0.99 | 156 | 122.4 | 166 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |


## W4 sum: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v0p-rerun | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v3-lcm | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v3-freq | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v4 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v34-lcm | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v34-freq | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |
| v6 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | unable | unable | | | | | | unable |

V0' is unable here, so no ratios.  

## W4 fma: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 21.08 | 0.00 | 27.31 | 0.00 | 48.55 | 1.00 | 1.00 | 38 | 29.1 | 183438 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 20.87 | 0.00 | 27.50 | 0.00 | 48.37 | 1.00 | 0.99 | 38 | 29.1 | 183438 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v3-lcm | 22.37 | 0.00 | 29.39 | 1.57 | 53.56 | 0.91 | 0.88 | 101 | 103.6 | 183438 | input D after entry=213 (lcm); 598,438 exceptions in, 199,989 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 22.27 | 0.00 | 28.96 | 1.53 | 53.04 | 0.92 | 0.90 | 101 | 103.6 | 183438 | input D after entry=213 (freq); 598,438 exceptions in, 199,989 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 10.34 | 0.00 | 23.77 | 8.91 | 43.12 | 1.13 | 0.84 | 45 | 33.7 | 183438 | 0 early reductions | 0.00 | ok |
| v34-lcm | 11.92 | 0.00 | 25.11 | 9.01 | 46.16 | 1.05 | 0.80 | 101 | 103.6 | 183438 | input D after entry=213 (lcm); 598,438 exceptions in, 200,000 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 12.03 | 0.00 | 25.07 | 8.89 | 46.01 | 1.06 | 0.80 | 101 | 103.6 | 183438 | input D after entry=213 (freq); 598,438 exceptions in, 200,000 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 21.43 | 0.00 | 27.98 | 0.00 | 49.15 | 0.99 | 0.98 | 39 | 29.4 | 183438 | 168 nodes (0 holding several primes); splits 4 at entry, 0 in ops | 0.00 | ok |


## W5 add: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 44.54 | 0.00 | 43.18 | 0.00 | 88.08 | 1.00 | 1.00 | 112 | 90.6 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 44.61 | 0.00 | 42.89 | 0.00 | 87.50 | 1.01 | 1.01 | 112 | 90.6 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v6 | 46.92 | 0.00 | 43.23 | 0.00 | 90.23 | 0.98 | 1.00 | 113 | 90.9 | 42 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |


## W5 mul: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 44.54 | 0.00 | 40.95 | 0.00 | 85.49 | 1.00 | 1.00 | 111 | 90.6 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 45.06 | 0.00 | 41.33 | 0.00 | 87.32 | 0.98 | 0.99 | 111 | 90.6 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v6 | 47.32 | 0.00 | 40.81 | 0.00 | 88.68 | 0.96 | 1.00 | 112 | 90.9 | 59 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |


## W6 chain: growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2)

n = 1,000,000, K = 4

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 69.87 | 0.00 | 276.25 | 0.00 | 346.37 | 1.00 | 1.00 | 169 | 149.8 | 253 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 70.64 | 0.00 | 279.14 | 0.00 | 350.10 | 0.99 | 0.99 | 169 | 149.8 | 253 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 14.88 | 0.00 | 7.38 | 29.17 | 52.13 | 6.64 | 7.56 | 105 | 64.9 | 253 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 14.93 | 0.00 | 7.32 | 29.08 | 51.83 | 6.68 | 7.59 | 105 | 64.9 | 253 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 19.17 | 0.00 | 211.68 | 35.08 | 266.60 | 1.30 | 1.12 | 170 | 149.8 | 253 | 0 early reductions | 0.00 | ok |
| v34-lcm | 14.94 | 0.00 | 7.37 | 28.36 | 50.88 | 6.81 | 7.73 | 105 | 64.9 | 253 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 15.23 | 0.00 | 7.34 | 28.42 | 51.45 | 6.73 | 7.73 | 105 | 64.9 | 253 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 71.53 | 0.00 | 274.15 | 0.00 | 345.68 | 1.00 | 1.01 | 170 | 150.0 | 253 | 2 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## W7 fma: 32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12

n = 200,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 25.88 | 6.34 | 13.82 | 0.00 | 39.73 | 1.00 ‡ | 1.00 ‡ | 36 | 26.4 | 69474 | 125 nodes; 24 decomposition splits | 0.00 | ok |
| v0p-rerun | 27.90 | 6.48 | 14.02 | 0.00 | 41.68 | 0.95 ‡ | 0.99 ‡ | 36 | 26.4 | 69474 | 125 nodes; 24 decomposition splits | 0.00 | ok |
| v3-lcm | 23.09 | 6.33 | 17.69 | 1.52 | 42.43 | 0.94 ‡ | 0.72 ‡ | 83 | 80.9 | 69474 | input D after entry=9176664807820423 (lcm); 389,996 exceptions in, 200,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v3-freq | 26.89 | 6.35 | 16.38 | 1.51 | 44.76 | 0.89 ‡ | 0.77 ‡ | 96 | 103.9 | 69474 | input D after entry=9176664807820423 (freq); 556,396 exceptions in, 197,532 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 22.63 | 6.33 | 12.52 | 5.42 | 40.76 | 0.97 ‡ | 0.77 ‡ | 37 | 26.4 | 69474 | 161 early reductions | 0.00 | ok |
| v34-lcm | 22.21 | 6.34 | 22.86 | 5.47 | 50.32 | 0.79 ‡ | 0.49 ‡ | 83 | 80.9 | 69474 | input D after entry=9176664807820423 (lcm); 389,996 exceptions in, 200,000 out; 0 dense overflows; 1 ops all-graph; 138,420 early reductions | 0.00 | ok |
| v34-freq | 23.92 | 6.34 | 15.10 | 5.97 | 45.07 | 0.88 ‡ | 0.66 ‡ | 96 | 103.9 | 69474 | input D after entry=9176664807820423 (freq); 556,396 exceptions in, 199,187 out; 0 dense overflows; 0 ops all-graph; 213 early reductions | 0.00 | ok |
| v6 | 29.18 | 0.00 | 15.84 | 0.00 | 45.09 | 0.88 ‡ | 0.87 ‡ | 36 | 26.7 | 69474 | 101 nodes (24 holding several primes); splits 17 at entry, 0 in ops | 0.00 | ok |

v0p: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v0p-rerun: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v3-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v3-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v4: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v34-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v34-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  
v6: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits the 64-bit range)  

## W8 sum: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 22.25 | 0.00 | 31.84 | 0.00 | 54.23 | 1.00 | 1.00 | 48 | 31.5 | 1 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 22.27 | 0.00 | 32.17 | 0.00 | 54.57 | 0.99 | 0.99 | 48 | 31.5 | 1 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 4.29 | 0.00 | 1.21 | 0.00 | 5.56 | 9.76 | 26.31 | 29 | 11.5 | 1 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 4.06 | 0.00 | 1.14 | 0.00 | 5.34 | 10.16 | 27.99 | 29 | 11.5 | 1 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 6.26 | 0.00 | 18.13 | 0.00 | 24.57 | 2.21 | 1.76 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v34-lcm | 4.42 | 0.00 | 1.24 | 0.00 | 5.66 | 9.58 | 25.63 | 29 | 11.5 | 1 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 3.98 | 0.00 | 1.07 | 0.00 | 5.05 | 10.74 | 29.89 | 29 | 11.5 | 1 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 22.87 | 0.00 | 32.55 | 0.00 | 55.53 | 0.98 | 0.98 | 48 | 31.8 | 1 | 2 nodes (0 holding several primes); splits 2 at entry, 0 in ops | 0.00 | ok |


## W8 fma: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 66.01 | 0.00 | 69.25 | 0.00 | 135.35 | 1.00 | 1.00 | 166 | 122.1 | 25 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 66.21 | 0.00 | 68.85 | 0.00 | 135.03 | 1.00 | 1.01 | 166 | 122.1 | 25 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 15.06 | 0.00 | 2.37 | 21.44 | 38.90 | 3.48 | 2.91 | 105 | 57.3 | 25 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.16 | 0.00 | 2.49 | 21.48 | 39.18 | 3.45 | 2.89 | 105 | 57.3 | 25 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 19.27 | 0.00 | 43.58 | 27.63 | 90.70 | 1.49 | 0.97 | 170 | 122.1 | 25 | 0 early reductions | 0.00 | ok |
| v34-lcm | 15.82 | 0.00 | 2.50 | 21.11 | 39.57 | 3.42 | 2.93 | 105 | 57.3 | 25 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 15.61 | 0.00 | 2.45 | 21.07 | 39.20 | 3.45 | 2.94 | 105 | 57.3 | 25 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 68.52 | 0.00 | 68.14 | 0.00 | 136.78 | 0.99 | 1.02 | 167 | 122.4 | 25 | 2 nodes (0 holding several primes); splits 2 at entry, 0 in ops | 0.00 | ok |


## W9 sum: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 24.52 | 0.00 | 49.52 | 0.00 | 74.16 | 1.00 | 1.00 | 48 | 31.5 | 1 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 24.62 | 0.00 | 49.52 | 0.00 | 74.09 | 1.00 | 1.00 | 48 | 31.5 | 1 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v3-lcm | 5.05 | 0.00 | 1.35 | 0.00 | 6.48 | 11.45 | 36.77 | 29 | 11.5 | 1 | input D after entry=693000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 6.00 | 0.00 | 3.34 | 0.00 | 9.34 | 7.94 | 14.82 | 36 | 17.2 | 1 | input D after entry=1000 (freq); 49,755 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 8.54 | 0.00 | 26.69 | 0.00 | 35.11 | 2.11 | 1.86 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v34-lcm | 4.90 | 0.00 | 1.23 | 0.00 | 6.17 | 12.01 | 40.20 | 29 | 11.5 | 1 | input D after entry=693000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 5.38 | 0.00 | 2.97 | 0.00 | 8.34 | 8.89 | 16.65 | 36 | 17.2 | 1 | input D after entry=1000 (freq); 49,755 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 25.98 | 0.00 | 50.74 | 0.00 | 76.82 | 0.97 | 0.98 | 49 | 31.8 | 1 | 5 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## W9 fma: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 73.97 | 0.00 | 73.80 | 0.00 | 147.77 | 1.00 | 1.00 | 168 | 129.7 | 384 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 73.45 | 0.00 | 73.19 | 0.00 | 146.41 | 1.01 | 1.01 | 168 | 129.7 | 384 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v3-lcm | 17.14 | 0.00 | 99.95 | 23.18 | 140.50 | 1.05 | 0.60 | 185 | 153.3 | 384 | input D after entry=693000 (lcm); 0 exceptions in, 575,843 out; 575,954 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 19.75 | 0.00 | 22.22 | 24.57 | 66.99 | 2.21 | 1.58 | 129 | 86.0 | 384 | input D after entry=1000 (freq); 149,976 exceptions in, 96,079 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 26.31 | 0.00 | 50.69 | 30.92 | 108.56 | 1.36 | 0.90 | 170 | 129.7 | 384 | 0 early reductions | 0.00 | ok |
| v34-lcm | 16.90 | 0.00 | 92.58 | 33.42 | 143.80 | 1.03 | 0.59 | 185 | 153.3 | 384 | input D after entry=693000 (lcm); 0 exceptions in, 575,843 out; 575,954 dense overflows; 0 ops all-graph; 575,954 early reductions | 0.00 | ok |
| v34-freq | 19.37 | 0.00 | 12.70 | 27.65 | 59.34 | 2.49 | 1.83 | 133 | 97.5 | 384 | input D after entry=1000 (freq); 149,976 exceptions in, 142,621 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 77.49 | 0.00 | 74.17 | 0.00 | 151.74 | 0.97 | 1.00 | 168 | 130.0 | 384 | 5 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## W10 fma2: expression chain d = (a*b + c)*e + f on two-place decimals in (-100, 100)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 110.31 | 0.00 | 140.74 | 0.00 | 250.88 | 1.00 | 1.00 | 285 | 212.7 | 49 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 109.74 | 0.00 | 139.24 | 0.00 | 248.93 | 1.01 | 1.01 | 285 | 212.7 | 49 | 2 nodes; 1 decomposition splits | 0.00 | ok |
| v3-lcm | 25.40 | 0.00 | 5.00 | 26.04 | 56.53 | 4.44 | 4.53 | 166 | 87.8 | 49 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 25.13 | 0.00 | 5.06 | 26.04 | 57.35 | 4.37 | 4.52 | 166 | 87.8 | 49 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 31.62 | 0.00 | 90.50 | 32.13 | 154.92 | 1.62 | 1.15 | 291 | 212.7 | 49 | 0 early reductions | 0.00 | ok |
| v34-lcm | 25.85 | 0.00 | 4.98 | 25.68 | 57.11 | 4.39 | 4.59 | 166 | 87.8 | 49 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v34-freq | 25.08 | 0.00 | 5.01 | 25.60 | 55.89 | 4.49 | 4.60 | 166 | 87.8 | 49 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph; 0 early reductions | 0.00 | ok |
| v6 | 114.17 | 0.00 | 139.02 | 0.00 | 254.49 | 0.99 | 1.01 | 286 | 213.0 | 49 | 2 nodes (0 holding several primes); splits 1 at entry, 0 in ops | 0.00 | ok |


## S8 add: numerators |n| < 2^7, denominators 1..12

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 42.76 | 0.00 | 42.02 | 0.00 | 84.80 | 1.00 | 1.00 | 112 | 90.6 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 42.16 | 0.00 | 42.24 | 0.00 | 84.56 | 1.00 | 0.99 | 112 | 90.6 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 42.49 | 0.00 | 43.51 | 0.00 | 86.15 | 0.98 | 0.97 | 112 | 90.6 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 43.11 | 0.00 | 33.13 | 0.00 | 76.68 | 1.11 | 1.27 | 90 | 53.5 | 42 | peak 42 objects (0 freed) | 0.00 | ok |
| v6 | 48.38 | 0.00 | 43.77 | 0.00 | 92.39 | 0.92 | 0.96 | 113 | 90.9 | 42 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 42.64 | 0.00 | 37.18 | 0.00 | 80.21 | 1.06 | 1.13 | 90 | 67.8 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v1-32 | 42.76 | 0.00 | 38.98 | 0.00 | 82.04 | 1.03 | 1.08 | 90 | 67.8 | 42 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v5-32 | 40.39 | 0.00 | 29.88 | 0.00 | 70.64 | 1.20 | 1.41 | 79 | 42.1 | 42 | peak 42 objects (0 freed) | 0.00 | ok |


## S8 mul: numerators |n| < 2^7, denominators 1..12

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 42.31 | 0.00 | 40.52 | 0.00 | 83.05 | 1.00 | 1.00 | 111 | 90.6 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 42.41 | 0.00 | 40.49 | 0.00 | 83.12 | 1.00 | 1.00 | 111 | 90.6 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 42.20 | 0.00 | 39.59 | 0.00 | 81.71 | 1.02 | 1.02 | 111 | 90.6 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 43.13 | 0.00 | 31.97 | 0.00 | 75.28 | 1.10 | 1.27 | 90 | 53.5 | 59 | peak 59 objects (0 freed) | 0.00 | ok |
| v6 | 48.07 | 0.00 | 40.59 | 0.00 | 89.27 | 0.93 | 1.00 | 112 | 90.9 | 59 | 5 nodes (0 holding several primes); splits 0 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 42.58 | 0.00 | 36.75 | 0.00 | 79.74 | 1.04 | 1.10 | 89 | 67.8 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v1-32 | 42.54 | 0.00 | 37.56 | 0.00 | 80.31 | 1.03 | 1.08 | 89 | 67.8 | 59 | 5 nodes; 0 decomposition splits | 0.00 | ok |
| v5-32 | 40.26 | 0.00 | 31.85 | 0.00 | 72.25 | 1.15 | 1.27 | 79 | 42.1 | 59 | peak 59 objects (0 freed) | 0.00 | ok |


## S15 add: numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 57.06 | 0.00 | 60.74 | 0.00 | 117.86 | 1.00 | 1.00 | 127 | 98.3 | 14712 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 57.82 | 0.00 | 60.92 | 0.00 | 118.18 | 1.00 | 1.00 | 127 | 98.3 | 14712 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 56.98 | 0.00 | 65.52 | 0.00 | 122.48 | 0.96 | 0.93 | 127 | 98.3 | 14712 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 58.62 | 0.00 | 67.45 | 0.00 | 126.14 | 0.93 | 0.90 | 90 | 55.0 | 14712 | peak 14,712 objects (0 freed) | 0.00 | ok |
| v6 | 60.53 | 0.00 | 62.18 | 0.00 | 123.17 | 0.96 | 0.98 | 128 | 98.5 | 14712 | 54 nodes (0 holding several primes); splits 2 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 56.31 | 0.00 | 52.18 | 0.00 | 108.38 | 1.09 | 1.16 | 104 | 75.4 | 14712 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v1-32 | 56.36 | 0.00 | 58.17 | 0.00 | 114.62 | 1.03 | 1.04 | 104 | 75.4 | 14712 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v5-32 | 52.98 | 0.00 | 63.53 | 0.00 | 117.55 | 1.00 | 0.96 | 79 | 43.5 | 14712 | peak 14,712 objects (0 freed) | 0.00 | ok |


## S15 mul: numerators |n| < 2^15, denominators 1..256 (every add and mul fits int32)

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 57.17 | 0.00 | 54.65 | 0.00 | 111.82 | 1.00 | 1.00 | 126 | 98.3 | 17522 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 57.07 | 0.00 | 54.77 | 0.00 | 111.98 | 1.00 | 1.00 | 126 | 98.3 | 17522 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 57.18 | 0.00 | 54.49 | 0.00 | 111.87 | 1.00 | 1.00 | 126 | 98.3 | 17522 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 58.27 | 0.00 | 58.37 | 0.00 | 117.08 | 0.96 | 0.94 | 90 | 55.4 | 17522 | peak 17,522 objects (0 freed) | 0.00 | ok |
| v6 | 59.41 | 0.00 | 55.12 | 0.00 | 114.44 | 0.98 | 0.99 | 126 | 98.5 | 17522 | 54 nodes (0 holding several primes); splits 2 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 56.32 | 0.00 | 49.03 | 0.00 | 105.58 | 1.06 | 1.11 | 103 | 75.4 | 17522 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v1-32 | 56.46 | 0.00 | 49.42 | 0.00 | 106.20 | 1.05 | 1.11 | 103 | 75.4 | 17522 | 54 nodes; 0 decomposition splits | 0.00 | ok |
| v5-32 | 52.65 | 0.00 | 55.80 | 0.00 | 110.46 | 1.01 | 0.98 | 79 | 43.9 | 17522 | peak 17,522 objects (0 freed) | 0.00 | ok |


## S20 add: numerators |n| < 2^20, denominators 1..1000

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 66.31 | 0.00 | 67.43 | 0.00 | 133.73 | 1.00 | 1.00 | 131 | 98.3 | 172851 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 66.32 | 0.00 | 66.96 | 0.00 | 133.98 | 1.00 | 1.01 | 131 | 98.3 | 172851 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 66.15 | 0.00 | 74.33 | 0.00 | 140.71 | 0.95 | 0.91 | 131 | 98.3 | 172851 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 67.36 | 0.00 | 139.98 | 0.00 | 222.17 | 0.60 | 0.48 | 104 | 71.4 | 172851 | peak 172,851 objects (0 freed) | 0.00 | ok |
| v6 | 70.78 | 0.00 | 65.57 | 0.00 | 136.35 | 0.98 | 1.03 | 131 | 98.6 | 172851 | 168 nodes (0 holding several primes); splits 4 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 65.56 | 0.00 | 57.47 | 0.00 | 122.90 | 1.09 | 1.17 | 108 | 75.5 | 172851 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v1-32 | 65.47 | 0.00 | 65.68 | 0.00 | 131.08 | 1.02 | 1.03 | 108 | 75.5 | 172851 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v5-32 | 61.73 | 0.00 | 117.91 | 0.00 | 191.95 | 0.70 | 0.57 | 93 | 59.9 | 172851 | peak 172,851 objects (0 freed) | 0.00 | ok |


## S20 mul: numerators |n| < 2^20, denominators 1..1000

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 66.28 | 0.00 | 60.69 | 0.00 | 126.82 | 1.00 | 1.00 | 129 | 98.3 | 166664 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 66.44 | 0.00 | 60.08 | 0.00 | 127.34 | 1.00 | 1.01 | 129 | 98.3 | 166664 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 66.18 | 0.00 | 59.65 | 0.00 | 125.92 | 1.01 | 1.02 | 129 | 98.3 | 166664 | 168 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 67.41 | 0.00 | 105.76 | 0.00 | 187.79 | 0.68 | 0.57 | 104 | 70.8 | 166664 | peak 166,664 objects (0 freed) | 0.00 | ok |
| v6 | 70.89 | 0.00 | 58.83 | 0.00 | 129.98 | 0.98 | 1.03 | 130 | 98.6 | 166664 | 168 nodes (0 holding several primes); splits 4 at entry, 0 in ops | 0.00 | ok |
| v0p-32 | 65.32 | 0.00 | 52.78 | 0.00 | 118.44 | fail | fail | 93 | 67.8 | 30538 | 168 nodes; 0 decomposition splits | 0.00 | FAIL: round 1: 862781 of 1000000 elements overflowed (limit 0.1%) |
| v1-32 | 65.46 | 0.00 | 53.06 | 0.00 | 118.67 | fail | fail | 93 | 67.8 | 30538 | 168 nodes; 0 decomposition splits | 0.00 | FAIL: round 1: 862781 of 1000000 elements overflowed (limit 0.1%) |
| v5-32 | 61.82 | 0.00 | 57.07 | 0.00 | 121.04 | fail | fail | 79 | 45.1 | 30538 | peak 30,538 objects (0 freed) | 0.00 | FAIL: round 1: 862781 of 1000000 elements overflowed (limit 0.1%) |

v0p-32: overflows detected 862781 (outputs left overflowed: 862781, of which 0 have an exact value that fits the 32-bit range)  
v1-32: overflows detected 862781 (outputs left overflowed: 862781, of which 0 have an exact value that fits the 32-bit range)  
v5-32: overflows detected 862781 (outputs left overflowed: 862781, of which 0 have an exact value that fits the 32-bit range)  

## S30 add: numerators |n| < 2^30, denominators uniform in 1..2^15

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 554.79 | 0.10 | 75.45 | 0.00 | 630.47 | 1.00 | 1.00 | 162 | 132.1 | 983127 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 556.84 | 0.10 | 75.37 | 0.00 | 631.90 | 1.00 | 1.00 | 162 | 132.1 | 983127 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 556.32 | 0.10 | 84.60 | 0.00 | 640.92 | 0.98 | 0.89 | 162 | 132.1 | 983127 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 608.36 | 0.12 | 283.25 | 0.00 | 899.79 | 0.70 | 0.27 | 191 | 157.5 | 983127 | peak 1,009,090 objects (0 freed) | 0.00 | ok |
| v6 | 103.13 | 0.00 | 75.66 | 0.00 | 178.79 | 3.53 | 1.00 | 163 | 133.7 | 983127 | 3,512 nodes (0 holding several primes); splits 15 at entry, 0 in ops | 0.00 | ok |


## S30 mul: numerators |n| < 2^30, denominators uniform in 1..2^15

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 555.75 | 0.10 | 67.67 | 0.00 | 623.11 | 1.00 | 1.00 | 160 | 116.9 | 979925 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v0p-rerun | 553.98 | 0.10 | 67.63 | 0.00 | 620.79 | 1.00 | 1.00 | 160 | 116.9 | 979925 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v1 | 556.24 | 0.10 | 67.16 | 0.00 | 624.16 | 1.00 | 1.01 | 160 | 116.9 | 979925 | 3,512 nodes; 0 decomposition splits | 0.00 | ok |
| v5 | 608.97 | 0.12 | 241.11 | 0.00 | 852.85 | 0.73 | 0.28 | 191 | 157.0 | 979925 | peak 1,003,816 objects (0 freed) | 0.00 | ok |
| v6 | 103.31 | 0.00 | 68.32 | 0.00 | 171.90 | 3.62 | 0.99 | 162 | 118.4 | 979925 | 3,512 nodes (0 holding several primes); splits 15 at entry, 0 in ops | 0.00 | ok |


## S30d add: numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 58.03 | 0.00 | 72.99 | 0.00 | 130.95 | 1.00 | 1.00 | 154 | 113.5 | 18505 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 58.04 | 0.00 | 72.63 | 0.00 | 130.80 | 1.00 | 1.00 | 154 | 113.5 | 18505 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v1 | 57.99 | 0.00 | 81.09 | 0.00 | 139.23 | 0.94 | 0.90 | 154 | 113.5 | 18505 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v5 | 60.43 | 0.00 | 77.95 | 0.00 | 138.89 | 0.94 | 0.94 | 90 | 55.5 | 18505 | peak 18,520 objects (0 freed) | 0.00 | ok |
| v6 | 62.25 | 0.00 | 73.20 | 0.00 | 136.38 | 0.96 | 1.00 | 155 | 113.8 | 18505 | 55 nodes (0 holding several primes); splits 9 at entry, 0 in ops | 0.00 | ok |


## S30d mul: numerators |n| < 2^30, denominators from one fixed set of 50 in 2..2^15

n = 1,000,000

| row | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | structure | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 58.67 | 0.00 | 64.64 | 0.00 | 123.60 | 1.00 | 1.00 | 153 | 113.5 | 25935 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v0p-rerun | 58.38 | 0.00 | 65.37 | 0.00 | 123.55 | 1.00 | 0.99 | 153 | 113.5 | 25935 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v1 | 58.15 | 0.00 | 64.26 | 0.00 | 122.39 | 1.01 | 1.01 | 153 | 113.5 | 25935 | 55 nodes; 1 decomposition splits | 0.00 | ok |
| v5 | 60.60 | 0.00 | 74.11 | 0.00 | 136.09 | 0.91 | 0.87 | 90 | 56.1 | 25935 | peak 26,008 objects (0 freed) | 0.00 | ok |
| v6 | 62.15 | 0.00 | 65.00 | 0.00 | 127.21 | 0.97 | 0.99 | 154 | 113.8 | 25935 | 55 nodes (0 holding several primes); splits 9 at entry, 0 in ops | 0.00 | ok |


## Simplifications that may bias the results

- Everything in the first round's bias notes (results/ablation.md) still holds for the rows it covers: the shared entry hash, strict 'no caching', warm processes, layouts, V3's plain single values.  The V0', V1, V3, V4 and V5 binaries are built from the same sources; the round-2 edits are behind VARIANT / AB_32 switches, and those five compile to the same machine code as before (checked with objdump when the edits were made).
- The noise floor is an identical rerun of V0' (the same binary), not V2, because round 2 does not run V2.  It covers every job, column sums included.
- V34 (Part A) is V3's code with V4's deferred ops on the graph path.  A result stored while unreduced becomes an exception when its unreduced denominator does not divide the result's D, even if its reduced one would; the final reduction then also reduces every exception.  An overflow on the graph path reduces both operands and retries with the reducing op ('early reductions'), so V34 never overflows where V3 would not.
- Part B's sizes are chosen so that every add and mul of two operands fits int64 (S30: numerators below 2^46 and 2^60, denominators below 2^30), so the 64-bit rows should report no overflow.  S15 also fits int32 (below 2^24 and 2^30, denominators at most 2^16), so it is the size where the two widths compare cleanly.  The 32-bit build reads the same 64-bit input files and narrows at entry (a value outside int32 would be an overflow); its intermediates are int64.  S8 and S20 add fit int32 (0% overflowed in the smoke runs); S20 mul does not (products near 2^40: about 86% of its outputs overflow), so S20 mul's 32-bit rows fail the 0.1% rule and get no ratio; their times are for information only, since an overflowed element skips work.
- 32-bit rows are compared with the 64-bit control (V0' / x), and V1-32 and V5-32 also with their own 64-bit builds (V1, V5) on S8, S15 and S20.
- S30's large V0' / x (total) for V6 is entry only: the control gcd-refines each new distinct denominator against every live node and scans every node for each denominator's edges, a cost that grows with the number of distinct denominators (here capped at 2^15, all of 1..2^15 likely present).  The op ratio table shows the ops alone.  S30d (50 distinct denominators) is the same operand size without that cost.
- V6's coarsest rule: a node is the product of the primes first seen together in one denominator, stored as that product; a node holding one prime is stored as the prime with the power on the edge weight (100 then 35 give nodes 2, 5, 7 with 100 = 2^2 * 5^2).  A prime splits its holder only when seen again, so this is coarser than primes but finer than the coarsest coprime base in general (100 and 1000 give 2 and 5, not 10).
- V6 factors each distinct denominator in three steps: trial division by the primes below 2^16 with the adaptive stop (a copy of pg.c's table, built by the same sieve, because pg.c keeps it static and is left unchanged); then, for a leftover of 2^32 or more, exact division by the primes above 2^16 already in the dictionary; then Miller-Rabin on what remains, and pg_factor (rho) only if that is composite.  The dictionary so holds every prime, and step 3 (gcd refinement of unfactored leftovers) never triggers.  On W7, whose 32-62-bit denominators have no factor below 2^16, every distinct denominator still runs the whole table (6,542 primes); V0' runs pg_factor only on its refined nodes.  Before this order, V6 ran Miller-Rabin and rho on all of W7's distinct denominators, which was most of its W7 entry cost (about 50 ms of dictionary passes, now about 17).
- V6's nodes need not be prime.  While a live node holds several primes, or an operand may name a dead node (after a split during the ops), fin, core_add and core_mul run hooked copies (ab_v6.c) that test such nodes' known primes by exact division and split them on a proper common part; otherwise they run the control's code unchanged, after one test of a global flag.  A split during the fill re-expands every distinct denominator's edges at once, and the elements filled before it are rewritten at the end of the entry (timed).  W7's semiprime denominators stay nodes of two primes (their primes are never seen apart), so W7's ops stay on the hooked copies: each reduction and cancellation against such a node tests its known primes by exact division (no gcd).  In a full-size check before the measured run, V6's W7 op phase took about 15.7 ms against V0' 13.7 ms.
- The fill keeps one V6-only test per element (has a split happened since the previous element?), to know which elements to rewrite.
- V6's dictionary passes are timed as pass 1 (which includes the per-element denominator hash shared with V0') plus pass 2; the fill is the rest of entry.  The split of V0' entry is not measured (its code is unchanged).
- Per-rep resets run outside the timer for every row: pg_reset-like node table resets, and V6's v6_reset (a memset of the 256 KB direct-mapped dictionary and of the hash).
- Table capacities carry over between the warm reps of one invocation for every row (the node table, the arrays and their edge pools, V5's object blocks, V6's dictionary hash, pools and metadata): only the first rep pays for growing them, and the minimum over reps is reported.
- V6's 'nodes' are its live nodes after the run (splits in the operations included); V0' 'nodes' likewise.
