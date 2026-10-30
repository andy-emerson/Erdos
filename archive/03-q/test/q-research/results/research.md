# Research measurements

- **date**: 2026-10-26 16:37:12 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s); openblas 1 thread(s)
- **flint threads**: 1
- **compiled**: gcc -O3 -march=native; GMP 6.3.0, FLINT 3.0.1

## R1. LCF order keys

Longest key for any Q72 magnitude (bits, without the sign bit):

| Quotient code | longest found by search | proven upper bound |
|---|---|---|
| lexibinary (LCF's code) | 129 (at 18446744073709551615/1) | 131 |
| delta code | 101 (at 4224250756/2986996355) | 115 |

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| R1 sort Q72 values | 1000000 | LCF keys (128-bit), then numpy lexsort | 206 ms | 205 ms | 1% | 0.55x | 0 (exact order) |  | keys: 16 bytes per value; encoding included |
| R1 sort Q72 values | 1000000 | LCF keys (128-bit), then the same merge sort | 261 ms | 256 ms | 1% | 0.70x | 0 (exact order) |  | the merge sort of the exact method, on keys |
| R1 sort Q72 values | 1000000 | exact merge sort, cross-multiplied comparisons | 373 ms | 327 ms | 12% ⚠ | 1.00x | 0 (exact order) |  |  |
| R1 sort Q72 values | 1000000 | float64 sort, then exact sort of equal floats (floats given) | 121 ms | 121 ms | 5% | 0.32x | 0 (exact order) |  | correctly rounded floats computed before timing; float(p)/float(q) would not be monotonic |
| R1 sort Q72 values | 1000000 | float64 sort, then exact sort of equal floats (including conversion) | 257 ms | 254 ms | 2% | 0.69x | 0 (exact order) |  | conversion by Python's exact Fraction-to-float |
| R1 sort Q72 values | 1000000 | GMP mpq, Python sort | 1.17 s | 1.15 s | 1% | 3.14x | 0 (exact order) |  |  |
| R1 encode keys | 1000000 | LCF keys, encoding only | 87.3 ms | 86.9 ms | 1% |  | 0 (exact) |  |  |

## R2. Quote notation: 2-adic arithmetic against deferred and per-step reduction

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| R2 x <- x*b + c, 1 step, unrelated denominators 1..1000 | 100000 | GMP mpq_t, reduced every step (C) | 34.3 ms | 34.2 ms | 0% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 1 step, unrelated denominators 1..1000 | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 8.8 ms | 8.79 ms | 0% | 0.26x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 1 step, unrelated denominators 1..1000 | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 31.5 ms | 31.2 ms | 1% | 0.92x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 2 steps, unrelated denominators 1..1000 | 100000 | GMP mpq_t, reduced every step (C) | 54.6 ms | 54.4 ms | 0% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 2 steps, unrelated denominators 1..1000 | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 11.7 ms | 11.6 ms | 0% | 0.21x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 2 steps, unrelated denominators 1..1000 | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 41.1 ms | 41.1 ms | 0% | 0.75x | 0 (every element) |  | 5365 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 3 steps, unrelated denominators 1..1000 | 100000 | GMP mpq_t, reduced every step (C) | 77.7 ms | 76.4 ms | 2% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 3 steps, unrelated denominators 1..1000 | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 15.4 ms | 15.4 ms | 1% | 0.20x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 3 steps, unrelated denominators 1..1000 | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 84.3 ms | 77.9 ms | 8% | 1.08x | 0 (every element) |  | 97278 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 4 steps, unrelated denominators 1..1000 | 100000 | GMP mpq_t, reduced every step (C) | 98.8 ms | 98.5 ms | 0% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 4 steps, unrelated denominators 1..1000 | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 68.8 ms | 68.4 ms | 1% | 0.70x | 0 (every element) |  | 57110 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 4 steps, unrelated denominators 1..1000 | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 101 ms | 101 ms | 0% | 1.02x | 0 (every element) |  | 99954 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 1 step, decimals, 3 places | 100000 | GMP mpq_t, reduced every step (C) | 37.6 ms | 37.2 ms | 1% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 1 step, decimals, 3 places | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 7.63 ms | 7.55 ms | 1% | 0.20x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 1 step, decimals, 3 places | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 23 ms | 23 ms | 0% | 0.61x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 2 steps, decimals, 3 places | 100000 | GMP mpq_t, reduced every step (C) | 55.7 ms | 55.4 ms | 1% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 2 steps, decimals, 3 places | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 10.9 ms | 10.8 ms | 1% | 0.20x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 2 steps, decimals, 3 places | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 31.6 ms | 31.2 ms | 1% | 0.57x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 3 steps, decimals, 3 places | 100000 | GMP mpq_t, reduced every step (C) | 74.8 ms | 73.8 ms | 1% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 3 steps, decimals, 3 places | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 17 ms | 16.8 ms | 1% | 0.23x | 0 (every element) |  | 0 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 3 steps, decimals, 3 places | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 76.3 ms | 76.1 ms | 0% | 1.02x | 0 (every element) |  | 99985 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 4 steps, decimals, 3 places | 100000 | GMP mpq_t, reduced every step (C) | 93.1 ms | 92.9 ms | 0% | 1.00x | 0 (every element) |  |  |
| R2 x <- x*b + c, 4 steps, decimals, 3 places | 100000 | 128-bit numerator and denominator, gcd at the end (C) | 96.4 ms | 93.7 ms | 3% | 1.03x | 0 (every element) |  | 99126 of 100000 elements redone with GMP |
| R2 x <- x*b + c, 4 steps, decimals, 3 places | 100000 | 2-adic residues mod 2^128, reconstructed at the end (C) | 95.7 ms | 95.5 ms | 0% | 1.03x | 0 (every element) |  | 100000 of 100000 elements redone with GMP |

## R3. The plane variant

Format: share of values that fit, and bits needed

| Values | P80 (16 + 64 bits) | 2 x Q40 (80 bits) | P152 (24 + 128 bits) | 2 x Q72 (144 bits) | median bits, plane | median bits, pair |
|---|---|---|---|---|---|---|
| complex decimals, 3 places, parts below 10^3 | 100% | 100% | 100% | 100% | 47 | 53 |
| complex decimals, 6 places, parts below 10^6 | 0% | 0% | 100% | 100% | 97 | 113 |
| products of two 3-place complex decimals | 0.01% | 0% | 100% | 100% | 94 | 110 |
| quotients of two 3-place complex decimals | 0.005% | 0% | 100% | 2.4% | 113 | 151 |
| quotients of Gaussian integers below 1000 | 100% | 19.7% | 100% | 100% | 53 | 72 |
| quotients of Gaussian integers below 10^6 | 0.01% | 0% | 100% | 2.71% | 113 | 151 |
| products of three Gaussian-integer quotients below 100 | 3.94% | 0.25% | 100% | 59.2% | 92 | 123 |

Speed (baseline GMP mpq_t pairs from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| R3 complex mul, 3 places, parts below 1000 | 1000000 | plane vector (A + Bi)/D | 11.6 ms | 10.5 ms | 8% | 0.02x | 0 (verified) | 16 | tier s64 |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | axis pair, written like the plane vector | 12.5 ms | 10.3 ms | 10% ⚠ | 0.02x | 0 (verified) | 16 | same code as the plane vector; each part keeps its own denominator |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | pair of axis vectors (CxCol) | 12.7 ms | 11.4 ms | 15% ⚠ | 0.02x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step; tiers s64, s64 |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | GMP mpq_t pairs (C) | 609 ms | 608 ms | 0% | 1.00x | 0 (every element) |  |  |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | FLINT fmpq pairs (C) | 423 ms | 418 ms | 1% | 0.70x | 0 (every element) |  |  |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | FLINT fmpzi over one denominator (C) | 13.8 ms | 13.5 ms | 2% | 0.02x | 0 (every element) |  |  |
| R3 complex mul, 3 places, parts below 1000 | 1000000 | C int64 pairs, overflow-checked | 4.93 ms | 4.64 ms | 6% | 0.01x | 0 (every element) |  |  |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | plane vector (A + Bi)/D | 16.3 ms | 14.6 ms | 3% | 0.02x | 0 (verified) | 16 | tier s64 |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | axis pair, written like the plane vector | 16.8 ms | 14.7 ms | 4% | 0.02x | 0 (verified) | 16 | same code as the plane vector; each part keeps its own denominator |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | pair of axis vectors (CxCol) | 19 ms | 15.4 ms | 3% | 0.02x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step; tiers s64, s64 |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | GMP mpq_t pairs (C) | 798 ms | 797 ms | 0% | 1.00x | 0 (every element) |  |  |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | FLINT fmpq pairs (C) | 587 ms | 581 ms | 1% | 0.74x | 0 (every element) |  |  |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | FLINT fmpzi over one denominator (C) | 19.6 ms | 19.4 ms | 1% | 0.02x | 0 (every element) |  |  |
| R3 complex fma, 3 places, parts below 1000 | 1000000 | C int64 pairs, overflow-checked | 5.27 ms | 4.77 ms | 9% | 0.01x | 0 (every element) |  |  |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | plane vector (A + Bi)/D | 11.8 ms | 11.2 ms | 6% | 0.01x | 0 (verified) |  |  |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | axis pair, written like the plane vector | 12.5 ms | 11.7 ms | 3% | 0.02x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | pair of axis vectors (CxCol) | 13.6 ms | 12.9 ms | 5% | 0.02x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | GMP mpq_t pairs (C) | 832 ms | 826 ms | 1% | 1.00x | 0 (exact) |  |  |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | FLINT fmpq pairs (C) | 595 ms | 591 ms | 1% | 0.71x | 0 (exact) |  |  |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | FLINT fmpzi over one denominator (C) | 17.1 ms | 16.8 ms | 2% | 0.02x | 0 (exact) |  |  |
| R3 complex dot, 3 places, parts below 1000 | 1000000 | C int64 pairs, overflow-checked | 2.18 ms | 1.97 ms | 10% | 0.00x | 0 (exact) |  |  |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | plane vector (A + Bi)/D | 5.55 ms | 5.02 ms | 23% ⚠ | 0.03x | 0 (verified) |  |  |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | axis pair, written like the plane vector | 6.1 ms | 4.72 ms | 8% | 0.03x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | pair of axis vectors (CxCol) | 5.95 ms | 4.8 ms | 12% ⚠ | 0.03x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | GMP mpq_t pairs (C) | 221 ms | 218 ms | 1% | 1.00x | 0 (exact) |  |  |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | FLINT fmpq pairs (C) | 211 ms | 211 ms | 0% | 0.96x | 0 (exact) |  |  |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | FLINT fmpzi over one denominator (C) | 6.33 ms | 6.16 ms | 3% | 0.03x | 0 (exact) |  |  |
| R3 complex abs2, 3 places, parts below 1000 | 1000000 | C int64 pairs, overflow-checked | 2.84 ms | 2.53 ms | 11% ⚠ | 0.01x | 0 (exact) |  |  |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | plane vector (A + Bi)/D | 4.47 ms | 4.3 ms | 5% | 0.03x | 0 (verified) |  | tier s128 |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | axis pair, written like the plane vector | 4.73 ms | 4.5 ms | 2% | 0.03x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | pair of axis vectors (CxCol) | 5.1 ms | 4.39 ms | 3% | 0.03x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step; tiers s128, s128 |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | GMP mpq_t pairs (C) | 172 ms | 172 ms | 0% | 1.00x | 0 (every element) |  |  |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | FLINT fmpq pairs (C) | 175 ms | 175 ms | 0% | 1.02x | 0 (every element) |  |  |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | FLINT fmpzi over one denominator (C) | 3.87 ms | 3.8 ms | 2% | 0.02x | 0 (every element) |  |  |
| R3 complex mul, 6 places, parts below 1e+06 | 250000 | C int64 pairs, overflow-checked | 774 µs | 739 µs | 5% | 0.00x | overflow detected |  |  |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | plane vector (A + Bi)/D | 6.47 ms | 6 ms | 10% ⚠ | 0.03x | 0 (verified) |  | tier s128 |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | axis pair, written like the plane vector | 6.94 ms | 6.64 ms | 7% | 0.03x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | pair of axis vectors (CxCol) | 6.93 ms | 6.59 ms | 5% | 0.03x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step; tiers s128, s128 |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | GMP mpq_t pairs (C) | 232 ms | 229 ms | 1% | 1.00x | 0 (every element) |  |  |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | FLINT fmpq pairs (C) | 233 ms | 233 ms | 0% | 1.00x | 0 (every element) |  |  |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | FLINT fmpzi over one denominator (C) | 10.9 ms | 10.7 ms | 2% | 0.05x | 0 (every element) |  |  |
| R3 complex fma, 6 places, parts below 1e+06 | 250000 | C int64 pairs, overflow-checked | 1.11 ms | 1.02 ms | 9% | 0.00x | overflow detected |  |  |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | plane vector (A + Bi)/D | 4.85 ms | 4.47 ms | 8% | 0.02x | 0 (verified) |  |  |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | axis pair, written like the plane vector | 5.11 ms | 4.7 ms | 11% ⚠ | 0.02x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | pair of axis vectors (CxCol) | 5.19 ms | 4.63 ms | 15% ⚠ | 0.02x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | GMP mpq_t pairs (C) | 244 ms | 238 ms | 2% | 1.00x | 0 (exact) |  |  |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | FLINT fmpq pairs (C) | 243 ms | 242 ms | 0% | 0.99x | 0 (exact) |  |  |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | FLINT fmpzi over one denominator (C) | 8.28 ms | 8.2 ms | 1% | 0.03x | 0 (exact) |  |  |
| R3 complex dot, 6 places, parts below 1e+06 | 250000 | C int64 pairs, overflow-checked | 561 µs | 501 µs | 11% ⚠ | 0.00x | overflow detected |  |  |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | plane vector (A + Bi)/D | 2.43 ms | 2.18 ms | 16% ⚠ | 0.03x | 0 (verified) |  |  |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | axis pair, written like the plane vector | 2.47 ms | 2.2 ms | 14% ⚠ | 0.04x | 0 (verified) |  | same code as the plane vector; each part keeps its own denominator |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | pair of axis vectors (CxCol) | 2.42 ms | 2.19 ms | 15% ⚠ | 0.03x | 0 (verified) |  | the benchmark's complex columns, which reduce after every step |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | GMP mpq_t pairs (C) | 69.6 ms | 68.9 ms | 1% | 1.00x | 0 (exact) |  |  |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | FLINT fmpq pairs (C) | 70.7 ms | 70 ms | 1% | 1.02x | 0 (exact) |  |  |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | FLINT fmpzi over one denominator (C) | 4.06 ms | 4.02 ms | 1% | 0.06x | 0 (exact) |  |  |
| R3 complex abs2, 6 places, parts below 1e+06 | 250000 | C int64 pairs, overflow-checked | 279 µs | 274 µs | 2% | 0.00x | overflow detected |  |  |
