# Prime graph, light prototype: results

Generated 2026-10-27 20:07 by `pg_bench.py` on Intel(R) Xeon(R) Processor; clang 18 `-std=c2x -O3 -march=native`; FLINT 18.0.1.  5 interleaved rounds; each invocation runs 3 reps in one warm process (every rep re-enters the values from the raw inputs; pg also resets its graph) and reports the minimum of each phase over its reps.  Tables give the median over rounds.  Times in ms.

Columns: **entry** is conversion from the raw (num, den) inputs (pg: values entered, equal denominators merged, new ones gcd-refined, then every value reduced by divisibility tests after decomposition; vec/fz: the common denominator computed up front plus scaling; mpq: `mpq_set_si` + `mpq_canonicalize`; fmpq: `fmpq_set_si`); **decomp** is pg's explicit decomposition pass (entry cost, shown separately); **op** is the operation; **red** is fz's timed reduction of its results to lowest terms; **total** is the minimum over reps of entry + decomp + op within one rep (red excluded), so it can exceed the sum of the column minima.  **unreduced %** is the share of output values not in lowest terms, measured by the harness on every output of round 1.  Ratios are contender / pg (above 1: pg is faster); a ratio is '-' when either row failed its check.  † marks contenders whose outputs are unreduced (vec, pgu); ‡ marks ratios of a row (pg or pgu) that detected overflows on some elements: those elements are counted, not checked, and were not finished; a row fails if over 0.1% overflowed or any overflow was false.

- `pg`: pg, the prototype: reduces at entry (inside its timed entry), results in lowest terms
- `pgu`: pg variant, documented unreduced: no entry reduction, uniform arrays stay one group (no per-element cancellation), same-group adds skip reduction
- `vec`: vector form: checked int64 over one shared denominator (unreduced results)
- `mpq`: GMP mpq_t per element (mpq_set_si + mpq_canonicalize at entry)
- `fmpq`: FLINT fmpq per element (fmpq_set_si at entry); a*b + c in the calibrated faster form
- `fz`: FLINT fmpz over one shared denominator; 'red' is its timed reduction of the results to lowest terms (fmpq_set_fmpz_frac), not included in total

FLINT a*b + c form, chosen on a separate calibration run (op ms, not reported results): W2 fma: mul+add 155.64, set+addmul 158.53 -> mul+add; W3 fma: mul+add 74.18, set+addmul 77.23 -> mul+add; W4 fma: mul+add 24.47, set+addmul 25.21 -> mul+add; W7 fma: mul+add 38.04, set+addmul 38.62 -> mul+add

## W1 sum: decimal column: sum of three-place decimals in (-10^4, 10^4)

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 23.04 | 0.00 | 1.03 | 0.00 | 24.17 | 1.00 | 1.00 | 0.00 | ok |
| pgu † | 0.98 | 0.00 | 0.55 | 0.00 | 1.54 | 0.53 † | 0.06 † | 0.00 | ok |
| vec † | 1.48 | 0.00 | 0.69 | 0.00 | 2.12 | 0.66 † | 0.09 † | 100.00 | ok |
| mpq | 57.76 | 0.00 | 91.09 | 0.00 | 149.40 | 88.31 | 6.18 | 0.00 | ok |
| fmpq | 42.69 | 0.00 | 63.85 | 0.00 | 106.55 | 61.91 | 4.41 | 0.00 | ok |
| fz | 9.29 | 0.00 | 1.72 | 0.00 | 11.02 | 1.67 | 0.46 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 16; node + group + denominator-map memory 0.1 MiB.  
pgu: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 5; node + group + denominator-map memory 0.1 MiB.  

## W2 fma: decimals elementwise: a*b + c, three-place decimals in (-10^4, 10^4)

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 71.96 | 0.00 | 26.41 | 0.00 | 98.37 | 1.00 | 1.00 | 0.00 | ok |
| pgu † | 4.01 | 0.00 | 1.32 | 0.00 | 5.22 | 0.05 † | 0.05 † | 83.98 | ok |
| vec † | 5.29 | 0.00 | 1.76 | 0.00 | 6.91 | 0.07 † | 0.07 † | 83.98 | ok |
| mpq | 172.99 | 0.00 | 196.49 | 0.00 | 370.61 | 7.44 | 3.77 | 0.00 | ok |
| fmpq | 128.11 | 0.00 | 155.17 | 0.00 | 283.71 | 5.88 | 2.88 | 0.00 | ok |
| fz | 31.38 | 0.00 | 9.86 | 42.57 | 41.25 | 0.37 | 0.42 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 49; node + group + denominator-map memory 0.1 MiB.  
pgu: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 3; node + group + denominator-map memory 0.1 MiB.  

## W3 sum: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 22.87 | 0.00 | 1.03 | 0.00 | 23.95 | 1.00 | 1.00 | 0.00 | ok |
| vec † | 1.45 | 0.00 | 0.66 | 0.00 | 2.12 | 0.64 † | 0.09 † | 100.00 | ok |
| mpq | 46.33 | 0.00 | 73.30 | 0.00 | 119.65 | 71.22 | 5.00 | 0.00 | ok |
| fmpq | 30.51 | 0.00 | 50.16 | 0.00 | 80.85 | 48.73 | 3.38 | 0.00 | ok |
| fz | 9.37 | 0.00 | 1.79 | 0.00 | 11.16 | 1.74 | 0.47 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 0; live nodes 5; peak groups 20; node + group + denominator-map memory 0.1 MiB.  

## W3 fma: mixed small denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 69.90 | 0.00 | 53.17 | 0.00 | 123.29 | 1.00 | 1.00 | 0.00 | ok |
| vec | unable: numerator overflows int64 | | | | | | | | - |
| mpq | 138.70 | 0.00 | 151.04 | 0.00 | 289.79 | 2.84 | 2.35 | 0.00 | ok |
| fmpq | 91.63 | 0.00 | 74.39 | 0.00 | 167.05 | 1.40 | 1.35 | 0.00 | ok |
| fz | 31.56 | 0.00 | 20.21 | 44.51 | 51.70 | 0.38 | 0.42 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 0; live nodes 5; peak groups 166; node + group + denominator-map memory 0.1 MiB.  

## W4 sum: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | unable: common denominator overflows 64 bits (1 overflows detected) | | | | | | | | - |
| vec | unable: shared denominator overflows 64 bits | | | | | | | | - |
| mpq | 10.80 | 0.00 | 46.52 | 0.00 | 57.41 | - | - | 0.00 | ok |
| fmpq | 7.71 | 0.00 | 49.98 | 0.00 | 57.75 | - | - | 0.00 | ok |
| fz | 5.09 | 0.00 | 5.11 | 0.01 | 10.20 | - | - | 0.00 | ok |

pg: unable: common denominator overflows 64 bits (1 overflows detected).  

## W4 fma: unrelated denominators 1..1000, numerators |n| < 10^6

n = 200,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 16.93 | 0.01 | 68.35 | 0.00 | 85.33 | 1.00 | 1.00 | 0.00 | ok |
| vec | unable: shared denominator overflows 64 bits | | | | | | | | - |
| mpq | 32.60 | 0.00 | 36.93 | 0.00 | 69.46 | 0.54 | 0.81 | 0.00 | ok |
| fmpq | 23.25 | 0.00 | 24.54 | 0.00 | 47.95 | 0.36 | 0.56 | 0.00 | ok |
| fz | 17.02 | 0.00 | 119.26 | 108.68 | 135.88 | 1.75 | 1.59 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.01 ms, nodes split by it 0; live nodes 168; peak groups 274,488; node + group + denominator-map memory 92.0 MiB.  

## W5 add: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 71.01 | 14.03 | 34.67 | 0.00 | 120.17 | 1.00 | 1.00 | 0.00 | ok |
| vec † | 3.24 | 0.00 | 0.97 | 0.00 | 4.21 | 0.03 † | 0.04 † | 100.00 | ok |
| mpq | 93.16 | 0.00 | 60.86 | 0.00 | 154.75 | 1.76 | 1.29 | 0.00 | ok |
| fmpq | 60.87 | 0.00 | 26.49 | 0.00 | 87.24 | 0.76 | 0.73 | 0.00 | ok |
| fz | 20.05 | 0.00 | 3.27 | 30.35 | 23.87 | 0.09 | 0.20 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 14.03 ms, nodes split by it 0; live nodes 5; peak groups 42; node + group + denominator-map memory 0.1 MiB.  

## W5 mul: single values: independent pairs, denominators 1..12, numerators |n| < 10^6

n = 1,000,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 70.63 | 14.31 | 43.58 | 0.00 | 128.31 | 1.00 | 1.00 | 0.00 | ok |
| vec | unable: numerator overflows int64 | | | | | | | | - |
| mpq | 93.52 | 0.00 | 92.04 | 0.00 | 185.18 | 2.11 | 1.44 | 0.00 | ok |
| fmpq | 60.96 | 0.00 | 47.03 | 0.00 | 108.02 | 1.08 | 0.84 | 0.00 | ok |
| fz | 21.25 | 0.00 | 12.30 | 42.07 | 33.60 | 0.28 | 0.26 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 14.31 ms, nodes split by it 0; live nodes 5; peak groups 59; node + group + denominator-map memory 0.1 MiB.  

## W6 chain: growth chain x <- x*b + c, K=4; x0, c decimals in (-10, 10), b decimals in (-2, 2)

n = 1,000,000, K = 4

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 71.21 | 0.00 | 98.22 | 0.00 | 169.34 | 1.00 | 1.00 | 0.00 | ok |
| pgu † | 3.97 | 0.00 | 4.93 | 0.00 | 9.05 | 0.05 † | 0.05 † | 84.00 | ok |
| vec † | 6.40 | 0.00 | 4.62 | 0.00 | 11.09 | 0.05 † | 0.07 † | 84.00 | ok |
| mpq | 138.71 | 0.00 | 726.61 | 0.00 | 865.79 | 7.40 | 5.11 | 0.00 | ok |
| fmpq | 89.96 | 0.00 | 670.24 | 0.00 | 763.98 | 6.82 | 4.51 | 0.00 | ok |
| fz | 32.70 | 0.00 | 46.69 | 82.19 | 79.40 | 0.48 | 0.47 | 0.00 | ok |

pg: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 256; node + group + denominator-map memory 0.1 MiB.  
pgu: overflows detected 0 (output elements left as overflowed: 0, of which 0 have an exact value that fits 64 bits); decomposition 0.00 ms, nodes split by it 1; live nodes 2; peak groups 6; node + group + denominator-map memory 0.1 MiB.  

## W7 fma: 32-62-bit denominators (products of shared 16-20-bit primes, 24 semiprimes of two 24-29-bit primes, 8 primes of 40-58 bits): a*b + c, b with denominators 1..12

n = 200,000

| contender | entry | decomp | op | red | total | op / pg | total / pg | unreduced % | check |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---|
| pg | 17.38 | 6.46 | 24.23 | 0.00 | 48.10 | 1.00 ‡ | 1.00 ‡ | 0.00 | ok |
| vec | unable: shared denominator overflows 64 bits | | | | | | | | - |
| mpq | 28.81 | 0.00 | 41.18 | 0.00 | 70.02 | 1.70 | 1.46 | 0.00 | ok |
| fmpq | 34.88 | 0.00 | 37.85 | 0.00 | 72.71 | 1.56 | 1.51 | 0.00 | ok |
| fz | 197.80 | 0.00 | 540.33 | 229.08 | 737.96 | 22.30 | 15.34 | 0.00 | ok |

pg: overflows detected 61 (output elements left as overflowed: 61, of which 0 have an exact value that fits 64 bits); decomposition 6.46 ms, nodes split by it 24; live nodes 125; peak groups 75,800; node + group + denominator-map memory 23.0 MiB.  

## Simplifications that may bias the results

- Reduction: pg reduces every value at entry (after decomposition, inside its timed entry), and its add, sub, same-group add and mul cancel by divisibility tests on prime nodes, so its outputs are in lowest terms; the harness's unreduced % column verifies this per output.  Column sums add integers within each group without reducing, then reduce once while combining the group sums.  pgu and vec are unreduced by design (†), and fz's reduction is timed separately as 'red'.
- Pair cache: pg caches a plan per pair of group ids (common-denominator group, scale factors, merged edges) in a 2^16-entry direct-mapped cache, and caches 'weight lowered by one' transitions per group.  Few distinct groups (W1-W3, W5, W6) favour pg; W4 and W7 have many and miss often.
- pg's group table only grows (no collection); its memory (reported per job) is not charged to any op.
- Array layout: pg arrays are one numerator array plus one group id per element, not physically separate per-group vectors; only an array whose elements all share one group takes the vector-form loops (pgu).
- One missing speed-up: add tests every node of the result for cancellation, not only nodes where both weights are equal (the only ones that can cancel when the inputs are reduced).  This makes pg slower, not faster.
- vec and fz are given the common denominator of all inputs; vec reports 'unable' rather than widening, and pg reports 'unable' when a column sum's common denominator overflows 64 bits.
- Entry for pg merges equal denominators through a hash and gcd-refines each new one against every live node (a linear scan); inputs with few distinct denominators make that cheap.  For small denominators the refinement already splits nodes into primes, so that cost lands in entry and decomposition looks almost free.
- Inline edge lists of single values are rewritten after a split in one sync pass at the end of decomposition (ops read edges through the group id, so they are never stale where used).
- Warm processes: every rep re-enters the values from the raw inputs, but the reps run in one process, so allocator state and CPU caches are warm for every contender alike, and the minimum over reps is reported.
- An overflow inside a one-group (pgu) array loop poisons the whole result array.
