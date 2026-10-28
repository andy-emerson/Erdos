# Prime graph: ablation of grouping

Generated 2026-10-27 21:25 by `ablation_bench.py` on Intel(R) Xeon(R) Processor; clang 18 `-std=c2x -O3 -march=native`.  5 interleaved rounds; each invocation runs 3 reps in one warm process (every rep re-enters the values from the raw inputs and resets the node table) and reports the minimum of each phase over its reps.  Tables give the median over rounds.  Times in ms.

**Baseline: V0', the control (grouping removed).** Every other row is V0' plus exactly one idea; V0 is the original prototype, whose idea is grouping.  No row is meant to combine two ideas, with one exception that cannot be separated: V3's dense elements are scaled to the array denominator D and not reduced (a per-array canonical form), which is part of V4's idea too, so read V3 against V4 as well as V0'.  **V0' / x (total)** is the control's total over the row's total; **V0' / x (op)** is the control's op + final over the row's op + final.  Above 1, the strategy is faster than the control.

**Same-code noise floor:** V2 runs exactly the control's code outside the column sum, so its V0' / x (total) on the 10 non-sum jobs measures noise between identical code: 0.98 to 1.00 (median 0.99).  A ratio within that range of 1 counts as no difference.

Columns: **entry** is conversion from the raw (num, den) inputs, including decomposition (shown again as **decomp**) and the reduction of every entered value, except in V4 (values entered as given) and V3 (dense elements scaled to D unreduced; exceptions reduced), where it also includes building D; **op** is the operation; **final** is the timed final reduction to lowest terms (V3, V4; 0 elsewhere); **total** is the minimum over reps of entry + op + final within one rep.  **RSS** is the process's peak resident set (VmHWM; it includes the raw inputs and the output buffers, the same for every row of a job); **repr** is the bytes held by the strategy's own representation: for V0' and V1-V5 the measured peak of every allocation (arrays, edge pools, node table, objects, transient tables); for V0 an estimate after the run (node, group and denominator-map tables plus 12 bytes per array element), not a measured peak.  **out dens** counts the distinct denominators among the outputs.  **unreduced %** is the share of outputs not in lowest terms (round 1, every output); a row fails if it is above 0, since no strategy is documented as unreduced.  Overflowed elements ("0 0") are counted, not checked: a row fails if over 0.1% overflowed or any overflow was false (the exact value fits 64 bits); ‡ marks rows with some.

- `v0p`: V0' (control): grouping removed. Each value holds its numerator, cached denominator and inline edge list; every op merges its operands' edges, scales and reduces; no group table, no pair-plan cache
- `v0`: V0: the prototype pg.c, unchanged (grouping: global group table, per-group pair-plan cache, group-sum column sum)
- `v1`: V1 = V0' + per-value denominator check: equal -> one integer op; one divides the other -> one scaling; else the edge merge (mul: equal -> no merge)
- `v2`: V2 = V0' + bucketing inside the column sum (hash by denominator, i128 checked accumulators; each bucket reduced, then combined with the control's reducing graph adds); elementwise ops are V0'
- `v3-lcm`: V3 = V0' + array denominator D with sparse exceptions, D from the lcm of the array's denominators when below 2^63 (else the most frequent): dense checked int64 loops over D (dense elements are scaled to D, not reduced), graph path for exceptions; D built at entry (timed); outputs reduced in the timed final phase
- `v3-freq`: V3 as v3-lcm, D from the array's most frequent denominator (C = its edges; D = the lcm of the denominators dividing it)
- `v4`: V4 = V0' + deferred reduction: no reduction at entry or inside the expression; overflow -> reduce operands and retry; one timed reduction of the results at the end
- `v5`: V5 = V0' + interned denominator objects with real reference counts (pointer compare for 'same denominator?'; freed at count 0)

## W1 sum: decimal column: sum of three-place decimals in (-10^4, 10^4)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 23.43 | 0.00 | 33.70 | 0.00 | 57.33 | 1.00 | 1.00 | 49 | 31.5 | 1 |  | 0.00 | ok |
| v0 | 23.59 | 0.00 | 1.90 | 0.00 | 25.78 | 2.22 | 17.71 | 29 | 11.5 | 1 | 16 groups | 0.00 | ok |
| v1 | 23.33 | 0.00 | 29.18 | 0.00 | 52.72 | 1.09 | 1.15 | 49 | 31.5 | 1 |  | 0.00 | ok |
| v2 | 24.01 | 0.00 | 4.50 | 0.00 | 28.49 | 2.01 | 7.49 | 49 | 31.5 | 1 | 16 buckets | 0.00 | ok |
| v3-lcm | 4.30 | 0.00 | 0.98 | 0.00 | 5.20 | 11.03 | 34.49 | 29 | 11.5 | 1 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 3.53 | 0.00 | 0.87 | 0.00 | 4.41 | 13.00 | 38.90 | 29 | 11.5 | 1 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 6.15 | 0.00 | 18.11 | 0.00 | 24.34 | 2.36 | 1.86 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v5 | 23.77 | 0.00 | 29.25 | 0.00 | 52.93 | 1.08 | 1.15 | 37 | 19.2 | 1 | peak 16 objects (0 freed) | 0.00 | ok |


## W2 fma: decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 70.42 | 0.00 | 67.26 | 0.00 | 137.70 | 1.00 | 1.00 | 168 | 122.1 | 49 |  | 0.00 | ok |
| v0 | 72.48 | 0.00 | 27.03 | 0.00 | 99.83 | 1.38 | 2.49 | 96 | 45.8 | 49 | 49 groups | 0.00 | ok |
| v1 | 69.94 | 0.00 | 55.48 | 0.00 | 125.66 | 1.10 | 1.21 | 169 | 122.1 | 49 |  | 0.00 | ok |
| v2 | 71.87 | 0.00 | 66.36 | 0.00 | 138.34 | 1.00 | 1.01 | 169 | 122.1 | 49 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 15.03 | 0.00 | 2.47 | 22.71 | 40.28 | 3.42 | 2.67 | 105 | 57.3 | 49 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.62 | 0.00 | 2.51 | 23.09 | 41.31 | 3.33 | 2.63 | 105 | 57.3 | 49 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 19.26 | 0.00 | 44.80 | 29.05 | 93.36 | 1.47 | 0.91 | 170 | 122.1 | 49 | 0 early reductions | 0.00 | ok |
| v5 | 72.10 | 0.00 | 50.50 | 0.00 | 122.79 | 1.12 | 1.33 | 121 | 72.6 | 49 | peak 49 objects (21 freed) | 0.00 | ok |


## W3 sum: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 23.25 | 0.00 | 52.20 | 0.00 | 75.59 | 1.00 | 1.00 | 45 | 31.5 | 1 |  | 0.00 | ok |
| v0 | 23.56 | 0.00 | 1.92 | 0.00 | 25.52 | 2.96 | 27.18 | 29 | 11.5 | 1 | 20 groups | 0.00 | ok |
| v1 | 22.89 | 0.00 | 30.75 | 0.00 | 53.48 | 1.41 | 1.70 | 45 | 31.5 | 1 |  | 0.00 | ok |
| v2 | 23.49 | 0.00 | 6.26 | 0.00 | 29.85 | 2.53 | 8.34 | 45 | 31.5 | 1 | 12 buckets | 0.00 | ok |
| v3-lcm | 3.95 | 0.00 | 1.20 | 0.00 | 5.09 | 14.84 | 43.32 | 29 | 11.5 | 1 | input D after entry=27720 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.92 | 0.00 | 26.47 | 0.00 | 42.39 | 1.78 | 1.97 | 74 | 57.5 | 1 | input D after entry=12 (freq); 499,523 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 12.45 | 0.00 | 41.05 | 0.00 | 53.38 | 1.42 | 1.27 | 46 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v5 | 23.24 | 0.00 | 51.05 | 0.00 | 74.44 | 1.02 | 1.02 | 37 | 19.2 | 1 | peak 14 objects (292,483 freed) | 0.00 | ok |


## W3 fma: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 68.87 | 0.00 | 80.56 | 0.00 | 149.64 | 1.00 | 1.00 | 156 | 122.1 | 166 |  | 0.00 | ok |
| v0 | 70.83 | 0.00 | 53.51 | 0.00 | 124.34 | 1.20 | 1.51 | 96 | 45.8 | 166 | 166 groups | 0.00 | ok |
| v1 | 69.44 | 0.00 | 77.32 | 0.00 | 146.77 | 1.02 | 1.04 | 156 | 122.1 | 166 |  | 0.00 | ok |
| v2 | 71.37 | 0.00 | 79.64 | 0.00 | 151.32 | 0.99 | 1.01 | 156 | 122.1 | 166 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 14.92 | 0.00 | 45.60 | 37.01 | 97.73 | 1.53 | 0.98 | 138 | 105.3 | 166 | input D after entry=27720 (lcm); 0 exceptions in, 300,698 out; 300,700 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 59.26 | 0.00 | 97.43 | 13.75 | 170.73 | 0.88 | 0.72 | 374 | 379.3 | 166 | input D after entry=12 (freq); 2,082,070 exceptions in, 669,944 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 37.01 | 0.00 | 75.93 | 37.14 | 150.09 | 1.00 | 0.71 | 162 | 129.7 | 166 | 0 early reductions | 0.00 | ok |
| v5 | 70.18 | 0.00 | 66.81 | 0.00 | 137.33 | 1.09 | 1.21 | 121 | 72.6 | 166 | peak 166 objects (20 freed) | 0.00 | ok |


## W4 sum: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v0 | unable: common denominator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v1 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v2 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v3-lcm | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v3-freq | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v4 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |
| v5 | unable: common denominator or numerator overflows 64 bits (1 overflows detected) | | | | | - | - | | | | | | - |

V0' is unable here, so no ratios..  

## W4 fma: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 20.79 | 0.00 | 27.22 | 0.00 | 48.15 | 1.00 | 1.00 | 38 | 29.1 | 183438 |  | 0.00 | ok |
| v0 | 17.13 | 0.01 | 105.89 | 0.00 | 123.07 | 0.39 | 0.26 | 144 | 101.2 | 183438 | 274,488 groups | 0.00 | ok |
| v1 | 20.88 | 0.00 | 27.55 | 0.00 | 48.55 | 0.99 | 0.99 | 38 | 29.1 | 183438 |  | 0.00 | ok |
| v2 | 21.42 | 0.00 | 27.17 | 0.00 | 48.49 | 0.99 | 1.00 | 38 | 29.1 | 183438 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 22.50 | 0.00 | 29.02 | 1.59 | 53.26 | 0.90 | 0.89 | 101 | 103.6 | 183438 | input D after entry=213 (lcm); 598,438 exceptions in, 199,989 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 22.31 | 0.00 | 29.01 | 1.61 | 52.99 | 0.91 | 0.89 | 101 | 103.6 | 183438 | input D after entry=213 (freq); 598,438 exceptions in, 199,989 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 10.42 | 0.00 | 24.14 | 8.97 | 43.62 | 1.10 | 0.82 | 45 | 33.7 | 183438 | 0 early reductions | 0.00 | ok |
| v5 | 21.60 | 0.00 | 56.96 | 0.00 | 79.71 | 0.60 | 0.48 | 45 | 33.5 | 183438 | peak 183,756 objects (138,157 freed) | 0.00 | ok |


## W5 add: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 44.63 | 0.00 | 42.38 | 0.00 | 87.40 | 1.00 | 1.00 | 112 | 90.6 | 42 |  | 0.00 | ok |
| v0 | 90.77 | 16.73 | 36.98 | 0.00 | 128.68 | 0.68 | 1.15 | 331 | 297.6 | 42 | 42 groups | 0.00 | ok |
| v1 | 44.75 | 0.00 | 43.03 | 0.00 | 88.09 | 0.99 | 0.98 | 112 | 90.6 | 42 |  | 0.00 | ok |
| v2 | 45.64 | 0.00 | 43.34 | 0.00 | 89.26 | 0.98 | 0.98 | 112 | 90.6 | 42 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 49.07 | 0.00 | 46.73 | 7.56 | 103.12 | 0.85 | 0.78 | 374 | 321.8 | 42 | input D after entry=1 (lcm); 2,000,000 exceptions in, 1,000,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v3-freq | 48.60 | 0.00 | 46.59 | 7.47 | 102.81 | 0.85 | 0.78 | 374 | 321.8 | 42 | input D after entry=1 (freq); 2,000,000 exceptions in, 1,000,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v4 | 24.73 | 0.00 | 41.84 | 27.28 | 93.66 | 0.93 | 0.61 | 117 | 90.6 | 42 | 0 early reductions | 0.00 | ok |
| v5 | 45.49 | 0.00 | 33.34 | 0.00 | 78.93 | 1.11 | 1.27 | 90 | 53.5 | 42 | peak 42 objects (0 freed) | 0.00 | ok |


## W5 mul: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 44.47 | 0.00 | 40.68 | 0.00 | 85.34 | 1.00 | 1.00 | 111 | 90.6 | 59 |  | 0.00 | ok |
| v0 | 89.94 | 16.54 | 43.40 | 0.00 | 133.51 | 0.64 | 0.94 | 331 | 297.6 | 59 | 59 groups | 0.00 | ok |
| v1 | 44.67 | 0.00 | 40.01 | 0.00 | 85.17 | 1.00 | 1.02 | 111 | 90.6 | 59 |  | 0.00 | ok |
| v2 | 46.08 | 0.00 | 40.78 | 0.00 | 87.21 | 0.98 | 1.00 | 111 | 90.6 | 59 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 48.17 | 0.00 | 44.25 | 7.38 | 100.22 | 0.85 | 0.79 | 374 | 321.8 | 59 | input D after entry=1 (lcm); 2,000,000 exceptions in, 1,000,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v3-freq | 48.13 | 0.00 | 44.49 | 7.44 | 100.05 | 0.85 | 0.78 | 374 | 321.8 | 59 | input D after entry=1 (freq); 2,000,000 exceptions in, 1,000,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v4 | 24.91 | 0.00 | 33.89 | 32.11 | 91.01 | 0.94 | 0.62 | 117 | 90.6 | 59 | 0 early reductions | 0.00 | ok |
| v5 | 46.25 | 0.00 | 32.39 | 0.00 | 78.73 | 1.08 | 1.26 | 90 | 53.5 | 59 | peak 59 objects (0 freed) | 0.00 | ok |


## W6 chain: growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2)

n = 1,000,000, K = 4

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 70.60 | 0.00 | 278.15 | 0.00 | 349.11 | 1.00 | 1.00 | 169 | 149.8 | 253 |  | 0.00 | ok |
| v0 | 72.89 | 0.00 | 98.46 | 0.00 | 171.64 | 2.03 | 2.83 | 88 | 45.8 | 253 | 256 groups | 0.00 | ok |
| v1 | 70.28 | 0.00 | 209.40 | 0.00 | 278.97 | 1.25 | 1.33 | 169 | 149.8 | 253 |  | 0.00 | ok |
| v2 | 72.46 | 0.00 | 275.82 | 0.00 | 348.44 | 1.00 | 1.01 | 169 | 149.8 | 253 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 15.23 | 0.00 | 7.26 | 28.96 | 52.06 | 6.71 | 7.68 | 105 | 64.9 | 253 | input D after entry=1000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 14.71 | 0.00 | 7.29 | 29.18 | 51.03 | 6.84 | 7.63 | 105 | 64.9 | 253 | input D after entry=1000 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 19.24 | 0.00 | 214.01 | 35.18 | 269.03 | 1.30 | 1.12 | 170 | 149.8 | 253 | 0 early reductions | 0.00 | ok |
| v5 | 71.98 | 0.00 | 211.59 | 0.00 | 283.47 | 1.23 | 1.31 | 128 | 87.9 | 253 | peak 256 objects (96 freed) | 0.00 | ok |


## W7 fma: 32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12

n = 200,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 26.06 | 6.36 | 13.82 | 0.00 | 39.87 | 1.00 ‡ | 1.00 ‡ | 36 | 26.4 | 69474 |  | 0.00 | ok |
| v0 | 23.94 | 6.48 | 33.40 | 0.00 | 57.45 | 0.69 ‡ | 0.41 ‡ | 53 | 32.2 | 69474 | 75,800 groups | 0.00 | ok |
| v1 | 25.59 | 6.35 | 13.92 | 0.00 | 39.67 | 1.01 ‡ | 0.99 ‡ | 36 | 26.4 | 69474 |  | 0.00 | ok |
| v2 | 26.00 | 6.36 | 13.66 | 0.00 | 40.17 | 0.99 ‡ | 1.01 ‡ | 36 | 26.4 | 69474 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 23.27 | 6.34 | 17.69 | 1.41 | 42.61 | 0.94 ‡ | 0.72 ‡ | 83 | 80.9 | 69474 | input D after entry=9176664807820423 (lcm); 389,996 exceptions in, 200,000 out; 0 dense overflows; 1 ops all-graph | 0.00 | ok |
| v3-freq | 27.11 | 6.31 | 16.48 | 1.55 | 45.22 | 0.88 ‡ | 0.77 ‡ | 96 | 103.9 | 69474 | input D after entry=9176664807820423 (freq); 556,396 exceptions in, 197,532 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 22.73 | 6.34 | 12.42 | 5.39 | 40.68 | 0.98 ‡ | 0.78 ‡ | 37 | 26.4 | 69474 | 161 early reductions | 0.00 | ok |
| v5 | 26.23 | 6.34 | 20.49 | 0.00 | 46.90 | 0.85 ‡ | 0.67 ‡ | 34 | 22.3 | 69474 | peak 69,488 objects (15,194 freed) | 0.00 | ok |

v0p: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v0: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v1: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v2: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v3-lcm: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v3-freq: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v4: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  
v5: overflows detected 61 (outputs left overflowed: 61, of which 0 have an exact value that fits).  

## W8 sum: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 22.24 | 0.00 | 32.02 | 0.00 | 55.14 | 1.00 | 1.00 | 48 | 31.5 | 1 |  | 0.00 | ok |
| v0 | 22.26 | 0.00 | 1.71 | 0.00 | 24.14 | 2.28 | 18.75 | 29 | 11.5 | 1 | 9 groups | 0.00 | ok |
| v1 | 22.17 | 0.00 | 27.88 | 0.00 | 50.07 | 1.10 | 1.15 | 48 | 31.5 | 1 |  | 0.00 | ok |
| v2 | 22.72 | 0.00 | 5.06 | 0.00 | 27.77 | 1.99 | 6.33 | 48 | 31.5 | 1 | 9 buckets | 0.00 | ok |
| v3-lcm | 3.89 | 0.00 | 0.94 | 0.00 | 4.78 | 11.55 | 34.18 | 29 | 11.5 | 1 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 3.55 | 0.00 | 0.79 | 0.00 | 4.33 | 12.75 | 40.34 | 29 | 11.5 | 1 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 6.28 | 0.00 | 18.34 | 0.00 | 25.17 | 2.19 | 1.75 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v5 | 22.26 | 0.00 | 27.34 | 0.00 | 49.70 | 1.11 | 1.17 | 37 | 19.2 | 1 | peak 9 objects (0 freed) | 0.00 | ok |


## W8 fma: money: two-place decimals in (-10^5, 10^5)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 66.55 | 0.00 | 69.62 | 0.00 | 135.95 | 1.00 | 1.00 | 166 | 122.1 | 25 |  | 0.00 | ok |
| v0 | 67.81 | 0.00 | 31.56 | 0.00 | 99.43 | 1.37 | 2.21 | 95 | 45.8 | 25 | 25 groups | 0.00 | ok |
| v1 | 66.52 | 0.00 | 61.46 | 0.00 | 128.04 | 1.06 | 1.13 | 166 | 122.1 | 25 |  | 0.00 | ok |
| v2 | 68.92 | 0.00 | 68.67 | 0.00 | 139.04 | 0.98 | 1.01 | 166 | 122.1 | 25 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 15.05 | 0.00 | 2.50 | 21.35 | 38.92 | 3.49 | 2.92 | 105 | 57.3 | 25 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 15.49 | 0.00 | 2.43 | 21.47 | 39.94 | 3.40 | 2.91 | 105 | 57.3 | 25 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 19.37 | 0.00 | 43.31 | 27.59 | 90.74 | 1.50 | 0.98 | 170 | 122.1 | 25 | 0 early reductions | 0.00 | ok |
| v5 | 67.60 | 0.00 | 52.67 | 0.00 | 120.83 | 1.13 | 1.32 | 121 | 72.6 | 25 | peak 25 objects (26 freed) | 0.00 | ok |


## W9 sum: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 24.71 | 0.00 | 49.20 | 0.00 | 74.66 | 1.00 | 1.00 | 48 | 31.5 | 1 |  | 0.00 | ok |
| v0 | 24.73 | 0.00 | 2.07 | 0.00 | 26.88 | 2.78 | 23.73 | 29 | 11.5 | 1 | 29 groups | 0.00 | ok |
| v1 | 24.79 | 0.00 | 43.94 | 0.00 | 68.97 | 1.08 | 1.12 | 48 | 31.5 | 1 |  | 0.00 | ok |
| v2 | 24.88 | 0.00 | 4.62 | 0.00 | 29.50 | 2.53 | 10.65 | 48 | 31.5 | 1 | 22 buckets | 0.00 | ok |
| v3-lcm | 5.07 | 0.00 | 1.36 | 0.00 | 6.44 | 11.59 | 36.27 | 29 | 11.5 | 1 | input D after entry=693000 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 5.90 | 0.00 | 3.30 | 0.00 | 9.19 | 8.13 | 14.92 | 36 | 17.2 | 1 | input D after entry=1000 (freq); 49,755 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 8.59 | 0.00 | 26.64 | 0.00 | 35.26 | 2.12 | 1.85 | 49 | 31.5 | 1 | 0 early reductions | 0.00 | ok |
| v5 | 25.18 | 0.00 | 50.22 | 0.00 | 75.55 | 0.99 | 0.98 | 37 | 19.2 | 1 | peak 24 objects (718,012 freed) | 0.00 | ok |


## W9 fma: mixed: 90% denominator 1000 (three-place decimals in (-10^4, 10^4)), 10% denominators 1..12 (numerators |n| < 10^6)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 74.22 | 0.00 | 73.76 | 0.00 | 148.40 | 1.00 | 1.00 | 168 | 129.7 | 384 |  | 0.00 | ok |
| v0 | 74.72 | 0.00 | 35.48 | 0.00 | 110.21 | 1.35 | 2.08 | 97 | 45.9 | 384 | 385 groups | 0.00 | ok |
| v1 | 73.69 | 0.00 | 61.95 | 0.00 | 135.12 | 1.10 | 1.19 | 168 | 129.7 | 384 |  | 0.00 | ok |
| v2 | 75.70 | 0.00 | 72.77 | 0.00 | 148.26 | 1.00 | 1.01 | 168 | 129.7 | 384 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 17.55 | 0.00 | 100.78 | 23.50 | 142.15 | 1.04 | 0.59 | 185 | 153.3 | 384 | input D after entry=693000 (lcm); 0 exceptions in, 575,843 out; 575,954 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 20.54 | 0.00 | 22.54 | 24.51 | 67.53 | 2.20 | 1.57 | 129 | 86.0 | 384 | input D after entry=1000 (freq); 149,976 exceptions in, 96,079 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 26.19 | 0.00 | 51.38 | 30.87 | 108.49 | 1.37 | 0.90 | 170 | 129.7 | 384 | 0 early reductions | 0.00 | ok |
| v5 | 75.19 | 0.00 | 60.12 | 0.00 | 134.97 | 1.10 | 1.23 | 121 | 72.6 | 384 | peak 385 objects (323 freed) | 0.00 | ok |


## W10 fma2: expression chain d = (a*b + c)*e + f on two-place decimals in (-100, 100)

n = 1,000,000

| strategy | entry | decomp | op | final | total | V0' / x (total) | V0' / x (op) | RSS MiB | repr MiB | out dens | groups / objects / exceptions | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|---:|---|
| v0p | 110.84 | 0.00 | 139.67 | 0.00 | 250.52 | 1.00 | 1.00 | 285 | 212.7 | 49 |  | 0.00 | ok |
| v0 | 113.94 | 0.00 | 61.41 | 0.00 | 175.94 | 1.42 | 2.27 | 161 | 80.2 | 49 | 49 groups | 0.00 | ok |
| v1 | 110.06 | 0.00 | 115.13 | 0.00 | 225.66 | 1.11 | 1.21 | 285 | 212.7 | 49 |  | 0.00 | ok |
| v2 | 115.03 | 0.00 | 137.56 | 0.00 | 252.75 | 0.99 | 1.02 | 285 | 212.7 | 49 | (no sum: = V0') | 0.00 | ok |
| v3-lcm | 26.40 | 0.00 | 5.27 | 26.09 | 57.54 | 4.35 | 4.45 | 166 | 87.8 | 49 | input D after entry=100 (lcm); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v3-freq | 25.32 | 0.00 | 5.10 | 26.04 | 57.53 | 4.35 | 4.49 | 166 | 87.8 | 49 | input D after entry=100 (freq); 0 exceptions in, 0 out; 0 dense overflows; 0 ops all-graph | 0.00 | ok |
| v4 | 32.05 | 0.00 | 91.30 | 32.10 | 155.37 | 1.61 | 1.13 | 291 | 212.7 | 49 | 0 early reductions | 0.00 | ok |
| v5 | 113.14 | 0.00 | 107.67 | 0.00 | 220.82 | 1.13 | 1.30 | 205 | 126.0 | 49 | peak 49 objects (24 freed) | 0.00 | ok |


## Simplifications that may bias the results

- Entry is shared by V0' and V1-V5: a transient hash from each distinct input denominator to its edge list (new denominators gcd-refined against the live nodes exactly as pg_den_gid does, then decomposed with pg.c's pg_factor), freed when entry ends.  It plays the entry role of V0's denominator map, but nothing from it reaches the operations.  It favours inputs with few distinct denominators, as V0's map does.
- The control is strict about 'no caching': V0' and its strategies recompute the merge, the scalings and the reduction on every operation.  V0's pair-plan cache and 'weight lowered by one' cache are what grouping buys it on few-group data.
- V5's interning also removes duplicate edge lists: every value shares its denominator object's single edge list instead of carrying its own, which changes memory and cache behaviour as well as the equality test.
- V5's one idea includes the pointer comparison that the brief names as its 'same denominator?' check; this is the only same-denominator shortcut V5 has (no divisibility check), so it does not import V1.  Its product a*b inside a*b + c is a value and is interned like any other; its objects come from a free list (allocated in blocks of 1024 and kept for the process), so freeing is unlinking, not free().
- V3 cannot be separated from deferred reduction: its dense elements are numerators scaled to the array denominator D, never reduced until the final phase (a per-array canonical form).  Read V3 against V4 (deferred reduction alone) as well as against V0'.
- V3 has two declared rows for choosing D: v3-lcm (the lcm of the array's denominators when below 2^63, else the most frequent) and v3-freq (always the most frequent).  Scaling to a large lcm (W3: 27720; W9: 693000) makes the dense a*b overflow int64, so those elements fall back to the graph path.  V3 treats W5's single values as plain values (all exceptions), i.e. as V0'; and once an op's D would reach 2^63, or D is 1, every element takes the control's graph path, so V3 falls back to V0' with some overhead.
- V3 and V4 give lowest-terms outputs only after their final reduction, which is timed and included in total and in the op ratio (op + final).
- Same-code noise floor: V2 differs from V0' only in the column sum (bucketing; each bucket is reduced and the buckets are combined with the control's reducing adds, so V2 carries no deferred reduction).  Its other rows run the control's code, and their spread against V0' (stated above the tables) is the noise between identical code; differences within it count as no difference.
- The repr column is measured differently: for V0' and V1-V5 it is the real peak of every representation allocation; for V0 (pg.c, unchanged, so not instrumented) it is an estimate after the run from its table sizes and 12 bytes per array element.
- V4's overflow retry reduces the operands and repeats the op with the control's reducing form, so it never overflows where V0' would not; 'early reductions' counts the retries.
- Layouts differ by design: V0 arrays hold a numerator and a group id per element (12 bytes); V0', V1, V2, V4 hold numerator, denominator, edge count, edge offset and the edges (about 21 bytes + 4 per edge); V5 holds a numerator and an object pointer (16 bytes); V3 holds a dense numerator per element plus 96 bytes per exception.  W5's single values are pg_val structs in V0 and array elements elsewhere.
- The V0' family writes results in index order into a separate array, so a chain (W6) alternates two arrays where V0 works in place.
- V0's column sum reports 'unable' when a group sum or the combined common denominator overflows 64 bits; the control's sequential adds report it when a reduced partial sum does.  Both fail W4's sum.
- Warm processes: every rep re-enters the values from the raw inputs, but the reps run in one process, so allocator state and CPU caches are warm for every strategy alike, and the minimum over reps is reported.
