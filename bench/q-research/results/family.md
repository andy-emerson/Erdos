# Research for the division-algebra family

- **date**: 2026-10-26 21:52:53 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1
- **compiled**: section B (un.c, every method in it): clang 18 -O3 -march=native, for _BitInt; sections C, E, G (quat.c, oct.c, gemm.c): gcc 13 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1; G uses OpenBLAS 0.3.34 (scipy-openblas32 wheel) under every method, one thread, and FLINT 3.0.1 configured with BLAS (build_flint_blas.sh) except the row marked without BLAS

## B. Fixed-width uN tiers (baseline GMP mpz_t numerators over the shared denominator, from C)

The uN vector, GMP mpz_t and FLINT fmpz all keep integers over the shared denominator and never reduce; GMP mpq_t reduces every operation, so its column measures rationals, not widths.

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| B (a*b)^2, none wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 196 µs | 190 µs | 3% | 0.20x | 0 (every element) |  | vector width 128 bits |
| B (a*b)^2, none wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 855 µs | 815 µs | 5% | 0.86x | 0 (every element) |  | vector width 128 bits |
| B (a*b)^2, none wide | 100000 | GMP mpz_t numerators (C) | 990 µs | 928 µs | 6% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, none wide | 100000 | FLINT fmpz numerators (C) | 1.42 ms | 1.35 ms | 5% | 1.43x | 0 (every element) |  |  |
| B (a*b)^2, none wide | 100000 | GMP mpq_t (C) | 12 ms | 11.5 ms | 4% | 12.10x | 0 (every element) |  |  |
| B sum of (a*b)^2, none wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 149 µs | 144 µs | 3% | 0.11x | 0 (exact) |  | vector width 128 bits |
| B sum of (a*b)^2, none wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 707 µs | 663 µs | 6% | 0.54x | 0 (exact) |  | vector width 128 bits |
| B sum of (a*b)^2, none wide | 100000 | GMP mpz_t numerators (C) | 1.31 ms | 1.15 ms | 12% ⚠ | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, none wide | 100000 | FLINT fmpz numerators (C) | 1.79 ms | 1.71 ms | 4% | 1.36x | 0 (exact) |  |  |
| B sum of (a*b)^2, none wide | 100000 | GMP mpq_t (C) | 25.3 ms | 24.3 ms | 4% | 19.31x | 0 (exact) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 551 µs | 544 µs | 1% | 0.10x | 0 (every element) |  | ends at width 128 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 4.28 ms | 4.23 ms | 1% | 0.76x | 0 (every element) |  | ends at width 128 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 81.8 ms | 80.1 ms | 2% | 14.57x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 84.6 ms | 83.9 ms | 1% | 15.06x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 79.2 ms | 78.4 ms | 1% | 14.11x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | GMP mpz_t numerators (C) | 5.61 ms | 5.49 ms | 2% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | FLINT fmpz numerators (C) | 3.63 ms | 3.57 ms | 2% | 0.65x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | GMP mpq_t (C) | 56.1 ms | 55.5 ms | 1% | 9.99x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 3.48 ms | 3.37 ms | 3% | 0.17x | 0 (every element) |  | ends at width 256 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 15.6 ms | 15.4 ms | 1% | 0.78x | 0 (every element) |  | ends at width 256 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 264 ms | 260 ms | 1% | 13.19x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 259 ms | 251 ms | 3% | 12.97x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 250 ms | 249 ms | 0% | 12.52x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | GMP mpz_t numerators (C) | 20 ms | 19.8 ms | 1% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | FLINT fmpz numerators (C) | 26.3 ms | 25.6 ms | 3% | 1.32x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | GMP mpq_t (C) | 190 ms | 187 ms | 2% | 9.51x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 25.7 ms | 25.2 ms | 2% | 0.43x | 0 (every element) |  | ends at width 512 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 52.2 ms | 52.1 ms | 0% | 0.87x | 0 (every element) |  | ends at width 512 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 694 ms | 660 ms | 5% | 11.53x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 604 ms | 601 ms | 1% | 10.03x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 598 ms | 590 ms | 1% | 9.93x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | GMP mpz_t numerators (C) | 60.2 ms | 59.5 ms | 1% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | FLINT fmpz numerators (C) | 93.1 ms | 87.8 ms | 6% | 1.55x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | GMP mpq_t (C) | 538 ms | 535 ms | 1% | 8.93x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 87.7 ms | 84 ms | 4% | 0.70x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 106 ms | 106 ms | 0% | 0.85x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 1.06 s | 1.04 s | 2% | 8.49x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 953 ms | 908 ms | 5% | 7.64x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 909 ms | 873 ms | 4% | 7.28x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | GMP mpz_t numerators (C) | 125 ms | 123 ms | 2% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | FLINT fmpz numerators (C) | 188 ms | 176 ms | 6% | 1.51x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | GMP mpq_t (C) | 1.14 s | 1.06 s | 7% | 9.11x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 323 ms | 314 ms | 3% | 1.04x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 259 ms | 256 ms | 1% | 0.83x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 1.87 s | 1.85 s | 1% | 6.02x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 1.51 s | 1.46 s | 3% | 4.86x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 1.42 s | 1.39 s | 2% | 4.57x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | GMP mpz_t numerators (C) | 310 ms | 298 ms | 4% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | FLINT fmpz numerators (C) | 478 ms | 393 ms | 18% ⚠ | 1.54x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | GMP mpq_t (C) | 3.12 s | 3.11 s | 1% | 10.06x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 100 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 1.02 s | 975 ms | 4% | 1.59x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 100 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 670 ms | 667 ms | 0% | 1.05x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 100 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 2.61 s | 2.57 s | 2% | 4.07x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 100 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 1.96 s | 1.88 s | 4% | 3.06x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 100 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 1.86 s | 1.85 s | 0% | 2.90x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 100 steps | 100000 | GMP mpz_t numerators (C) | 641 ms | 606 ms | 5% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 100 steps | 100000 | FLINT fmpz numerators (C) | 912 ms | 844 ms | 7% | 1.42x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 100 steps | 100000 | GMP mpq_t (C) | 5.09 s | 4.96 s | 3% | 7.94x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 140 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 1.93 s | 1.92 s | 0% | 1.65x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 140 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 1.09 s | 1.09 s | 1% | 0.94x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 140 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) | 3.47 s | 3.42 s | 1% | 2.98x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 140 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) | 2.43 s | 2.36 s | 3% | 2.08x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 140 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) | 2.27 s | 2.17 s | 5% | 1.95x | 0 (every element) |  | ends at width 2048 bits |
| B growth chain x <- x*b + c, 140 steps | 100000 | GMP mpz_t numerators (C) | 1.17 s | 1.02 s | 13% ⚠ | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 140 steps | 100000 | FLINT fmpz numerators (C) | 1.39 s | 1.28 s | 8% | 1.19x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 140 steps | 100000 | GMP mpq_t (C) | 8.03 s | 7.91 s | 2% | 6.89x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 200 steps | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) |  |  |  |  | exceeds the 2048-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 200 steps | 100000 | uN vector, fixed limb arrays with GMP mpn kernels |  |  |  |  | exceeds the 2048-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 200 steps | 100000 | pre-built 2048-bit _BitInt vector (starts at 2048, never widens) |  |  |  |  | exceeds the 2048-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 200 steps | 100000 | pre-built 2048-bit limb vector, GMP mpn (starts at 2048) |  |  |  |  | exceeds the 2048-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 200 steps | 100000 | pre-built U2048, compile-time length, inline loops (chain only) |  |  |  |  | exceeds the 2048-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 200 steps | 100000 | GMP mpz_t numerators (C) | 2.25 s | 2.22 s | 1% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 200 steps | 100000 | FLINT fmpz numerators (C) | 3.95 s | 3.82 s | 3% | 1.75x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 200 steps | 100000 | GMP mpq_t (C) | 13.9 s | 13.8 s | 1% | 6.16x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 530 µs | 470 µs | 11% ⚠ | 0.35x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, 1% wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 1.17 ms | 1.09 ms | 7% | 0.78x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, 1% wide | 100000 | GMP mpz_t numerators (C) | 1.51 ms | 1.38 ms | 8% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | FLINT fmpz numerators (C) | 1.41 ms | 1.37 ms | 3% | 0.94x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | GMP mpq_t (C) | 11.7 ms | 11.5 ms | 1% | 7.75x | 0 (every element) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 515 µs | 456 µs | 12% ⚠ | 0.42x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, 1% wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 720 µs | 681 µs | 5% | 0.58x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, 1% wide | 100000 | GMP mpz_t numerators (C) | 1.23 ms | 1.19 ms | 3% | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | FLINT fmpz numerators (C) | 1.86 ms | 1.78 ms | 4% | 1.51x | 0 (exact) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | GMP mpq_t (C) | 25 ms | 24.5 ms | 2% | 20.28x | 0 (exact) |  |  |
| B (a*b)^2, all wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 619 µs | 576 µs | 7% | 0.34x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, all wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 1.27 ms | 1.23 ms | 4% | 0.70x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, all wide | 100000 | GMP mpz_t numerators (C) | 1.81 ms | 1.76 ms | 3% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, all wide | 100000 | FLINT fmpz numerators (C) | 2.99 ms | 2.8 ms | 6% | 1.65x | 0 (every element) |  |  |
| B (a*b)^2, all wide | 100000 | GMP mpq_t (C) | 10.5 ms | 10.2 ms | 2% | 5.79x | 0 (every element) |  |  |
| B sum of (a*b)^2, all wide | 100000 | uN vector, _BitInt (fixed widths 64-2048, compiled) | 461 µs | 448 µs | 3% | 0.19x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, all wide | 100000 | uN vector, fixed limb arrays with GMP mpn kernels | 682 µs | 650 µs | 5% | 0.28x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, all wide | 100000 | GMP mpz_t numerators (C) | 2.4 ms | 2.22 ms | 7% | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, all wide | 100000 | FLINT fmpz numerators (C) | 3.05 ms | 2.94 ms | 4% | 1.27x | 0 (exact) |  |  |
| B sum of (a*b)^2, all wide | 100000 | GMP mpq_t (C) | 23.5 ms | 23.3 ms | 1% | 9.83x | 0 (exact) |  |  |

## C. Quaternion arrays (baseline FLINT fmpz over one denominator per value, from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| C p + q, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 1.22 ms | 1.12 ms | 8% | 0.70x | 0 (every element) | 32 |  |
| C p + q, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 93.1 ms | 92.6 ms | 1% | 53.44x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 44.3 ms | 43.9 ms | 1% | 25.41x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 1.74 ms | 1.56 ms | 11% ⚠ | 1.00x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 645 µs | 642 µs | 0% | 0.37x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 1.32 ms | 1.29 ms | 2% | 0.07x | 0 (every element) | 32 |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 592 ms | 576 ms | 3% | 33.13x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 393 ms | 389 ms | 1% | 22.00x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 17.9 ms | 17.7 ms | 1% | 1.00x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 1.46 ms | 1.41 ms | 3% | 0.08x | 0 (every element) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 538 µs | 500 µs | 7% | 0.24x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 94.3 ms | 93 ms | 1% | 41.95x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 82.9 ms | 82.3 ms | 1% | 36.88x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 2.25 ms | 2.17 ms | 3% | 1.00x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 728 µs | 597 µs | 18% ⚠ | 0.32x | 0 (exact) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 3.91 ms | 3.85 ms | 2% | 0.11x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 1.07 s | 1.03 s | 3% | 28.60x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 858 ms | 836 ms | 3% | 23.04x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 37.2 ms | 34.1 ms | 9% | 1.00x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 4.34 ms | 4.13 ms | 5% | 0.12x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 544 µs | 537 µs | 1% | 0.49x | 0 (every element) | 32 |  |
| C p + q, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 48.5 ms | 48 ms | 1% | 44.12x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 33.3 ms | 33.2 ms | 0% | 30.28x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 1.1 ms | 986 µs | 10% ⚠ | 1.00x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 327 µs | 324 µs | 1% | 0.30x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 804 µs | 753 µs | 6% | 0.04x | 0 (every element) | 64 |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 328 ms | 328 ms | 0% | 15.95x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 343 ms | 333 ms | 3% | 16.63x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 20.6 ms | 20.4 ms | 1% | 1.00x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 1.09 ms | 1.03 ms | 6% | 0.05x | overflow detected |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 357 µs | 320 µs | 10% ⚠ | 0.11x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 59.4 ms | 59 ms | 1% | 18.66x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 55.6 ms | 55.5 ms | 0% | 17.48x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 3.18 ms | 3.15 ms | 1% | 1.00x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 319 µs | 295 µs | 8% | 0.10x | overflow detected |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 2.02 ms | 1.93 ms | 4% | 0.04x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 666 ms | 663 ms | 0% | 14.49x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 684 ms | 681 ms | 0% | 14.89x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 45.9 ms | 45.9 ms | 0% | 1.00x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 1.52 ms | 1.49 ms | 2% | 0.03x | overflow detected |  |  |

## D. Precision of each member

| Member | Size | Unit bits | Slash bits (borrowed) | Data bits | Median relative error | Worst (beside 1) | Float significand to match: median | worst |
|---|---|---|---|---|---|---|---|---|
| reals | 136 | 1 | 7 (0) | 128 | 3.4e-39 | 2.7e-20 | 127 | 65 |
| complex | 72 | 2 | 6 (0) | 64 | 7.3e-20 | 1.2e-10 | 62 | 33 |
| quaternions | 40 | 3 | 5 (0) | 32 | 2.6e-10 | 7.6e-06 | 31 | 17 |
| octonions | 24 | 4 | 4 (0) | 16 | 1.8e-05 | 2.0e-03 | 15 | 9 |
| quaternions, promoted once | 72 | 3 | 6 (1) | 63 | 1.3e-19 | 2.3e-10 | 62 | 32 |
| quaternions, promoted twice | 136 | 3 | 7 (2) | 126 | 1.6e-38 | 5.4e-20 | 125 | 64 |

## E. Octonion arrays (baseline FLINT fmpz over one denominator per value, from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| E p + q, 3 places, components below 1000 | 100000 | octonion vector form (eight int64 vectors, compiled) | 1.12 ms | 1.08 ms | 3% | 0.38x | 0 (every element) |  |  |
| E p + q, 3 places, components below 1000 | 100000 | GMP mpq_t 8-tuples (C) | 89.2 ms | 88.6 ms | 1% | 30.59x | 0 (every element) |  |  |
| E p + q, 3 places, components below 1000 | 100000 | FLINT fmpz over one denominator per value (C) | 2.92 ms | 1.58 ms | 46% ⚠ | 1.00x | 0 (every element) |  |  |
| E p + q, 3 places, components below 1000 | 100000 | C int64 8-tuples, overflow-checked | 676 µs | 641 µs | 5% | 0.23x | 0 (every element) |  |  |
| E octonion product p q, 3 places, components below 1000 | 100000 | octonion vector form (eight int64 vectors, compiled) | 2.38 ms | 2.32 ms | 2% | 0.08x | 0 (every element) |  |  |
| E octonion product p q, 3 places, components below 1000 | 100000 | GMP mpq_t 8-tuples (C) | 1.24 s | 1.22 s | 2% | 39.21x | 0 (every element) |  |  |
| E octonion product p q, 3 places, components below 1000 | 100000 | FLINT fmpz over one denominator per value (C) | 31.6 ms | 31.2 ms | 1% | 1.00x | 0 (every element) |  |  |
| E octonion product p q, 3 places, components below 1000 | 100000 | C int64 8-tuples, overflow-checked | 3.41 ms | 3.34 ms | 2% | 0.11x | 0 (every element) |  |  |
| E sum of squared norms, 3 places, components below 1000 | 100000 | octonion vector form (eight int64 vectors, compiled) | 449 µs | 424 µs | 6% | 0.20x | 0 (exact) |  |  |
| E sum of squared norms, 3 places, components below 1000 | 100000 | GMP mpq_t 8-tuples (C) | 90.1 ms | 89.7 ms | 0% | 40.77x | 0 (exact) |  |  |
| E sum of squared norms, 3 places, components below 1000 | 100000 | FLINT fmpz over one denominator per value (C) | 2.21 ms | 2.15 ms | 2% | 1.00x | 0 (exact) |  |  |
| E sum of squared norms, 3 places, components below 1000 | 100000 | C int64 8-tuples, overflow-checked | 487 µs | 468 µs | 4% | 0.22x | 0 (exact) |  |  |
| E p + q, 6 places, components below 1e+06 | 50000 | octonion vector form (eight int64 vectors, compiled) | 547 µs | 546 µs | 0% | 0.69x | 0 (every element) |  |  |
| E p + q, 6 places, components below 1e+06 | 50000 | GMP mpq_t 8-tuples (C) | 48.5 ms | 47.2 ms | 3% | 61.17x | 0 (every element) |  |  |
| E p + q, 6 places, components below 1e+06 | 50000 | FLINT fmpz over one denominator per value (C) | 792 µs | 775 µs | 2% | 1.00x | 0 (every element) |  |  |
| E p + q, 6 places, components below 1e+06 | 50000 | C int64 8-tuples, overflow-checked | 352 µs | 338 µs | 4% | 0.44x | 0 (every element) |  |  |
| E octonion product p q, 6 places, components below 1e+06 | 50000 | octonion vector form (eight int64 vectors, compiled) | 1.39 ms | 1.34 ms | 4% | 0.03x | 0 (every element) |  |  |
| E octonion product p q, 6 places, components below 1e+06 | 50000 | GMP mpq_t 8-tuples (C) | 711 ms | 708 ms | 0% | 15.28x | 0 (every element) |  |  |
| E octonion product p q, 6 places, components below 1e+06 | 50000 | FLINT fmpz over one denominator per value (C) | 46.5 ms | 46.1 ms | 1% | 1.00x | 0 (every element) |  |  |
| E octonion product p q, 6 places, components below 1e+06 | 50000 | C int64 8-tuples, overflow-checked | 1.86 ms | 1.79 ms | 4% | 0.04x | overflow detected |  |  |
| E sum of squared norms, 6 places, components below 1e+06 | 50000 | octonion vector form (eight int64 vectors, compiled) | 304 µs | 301 µs | 1% | 0.09x | 0 (exact) |  |  |
| E sum of squared norms, 6 places, components below 1e+06 | 50000 | GMP mpq_t 8-tuples (C) | 59.5 ms | 58.4 ms | 2% | 17.16x | 0 (exact) |  |  |
| E sum of squared norms, 6 places, components below 1e+06 | 50000 | FLINT fmpz over one denominator per value (C) | 3.47 ms | 3.37 ms | 3% | 1.00x | 0 (exact) |  |  |
| E sum of squared norms, 6 places, components below 1e+06 | 50000 | C int64 8-tuples, overflow-checked | 255 µs | 242 µs | 5% | 0.07x | overflow detected |  |  |

## F. The uniform header (octonion layout for every algebra, U16 base) against the fitted header

| Algebra | Word | Size | Fitted: data bits (borrowed) | Uniform: data bits (borrowed) | Bits lost | Unit codes unused |
|---|---|---|---|---|---|---|
| real | U16 | 24 | not a member | 16 (0) | — | 14 of 16 |
| real | U32 | 40 | not a member | 31 (1) | — | 14 of 16 |
| real | U64 | 72 | not a member | 62 (2) | — | 14 of 16 |
| real | U128 | 136 | 128 (0) | 125 (3) | 3 | 14 of 16 |
| real | U256 | 264 | 255 (1) | 252 (4) | 3 | 14 of 16 |
| real | U512 | 520 | 510 (2) | 507 (5) | 3 | 14 of 16 |
| real | U1024 | 1032 | 1021 (3) | 1018 (6) | 3 | 14 of 16 |
| real | U2048 | 2056 | 2044 (4) | 2041 (7) | 3 | 14 of 16 |
| complex | U16 | 24 | not a member | 16 (0) | — | 12 of 16 |
| complex | U32 | 40 | not a member | 31 (1) | — | 12 of 16 |
| complex | U64 | 72 | 64 (0) | 62 (2) | 2 | 12 of 16 |
| complex | U128 | 136 | 127 (1) | 125 (3) | 2 | 12 of 16 |
| complex | U256 | 264 | 254 (2) | 252 (4) | 2 | 12 of 16 |
| complex | U512 | 520 | 509 (3) | 507 (5) | 2 | 12 of 16 |
| complex | U1024 | 1032 | 1020 (4) | 1018 (6) | 2 | 12 of 16 |
| complex | U2048 | 2056 | 2043 (5) | 2041 (7) | 2 | 12 of 16 |
| quaternion | U16 | 24 | not a member | 16 (0) | — | 8 of 16 |
| quaternion | U32 | 40 | 32 (0) | 31 (1) | 1 | 8 of 16 |
| quaternion | U64 | 72 | 63 (1) | 62 (2) | 1 | 8 of 16 |
| quaternion | U128 | 136 | 126 (2) | 125 (3) | 1 | 8 of 16 |
| quaternion | U256 | 264 | 253 (3) | 252 (4) | 1 | 8 of 16 |
| quaternion | U512 | 520 | 508 (4) | 507 (5) | 1 | 8 of 16 |
| quaternion | U1024 | 1032 | 1019 (5) | 1018 (6) | 1 | 8 of 16 |
| quaternion | U2048 | 2056 | 2042 (6) | 2041 (7) | 1 | 8 of 16 |
| octonion | U16 | 24 | 16 (0) | 16 (0) | 0 | 0 of 16 |
| octonion | U32 | 40 | 31 (1) | 31 (1) | 0 | 0 of 16 |
| octonion | U64 | 72 | 62 (2) | 62 (2) | 0 | 0 of 16 |
| octonion | U128 | 136 | 125 (3) | 125 (3) | 0 | 0 of 16 |
| octonion | U256 | 264 | 252 (4) | 252 (4) | 0 | 0 of 16 |
| octonion | U512 | 520 | 507 (5) | 507 (5) | 0 | 0 of 16 |
| octonion | U1024 | 1032 | 1018 (6) | 1018 (6) | 0 | 0 of 16 |
| octonion | U2048 | 2056 | 2041 (7) | 2041 (7) | 0 | 0 of 16 |

Decoding a U64 value, median ns per value over random patterns (C, gcc -O3, branch-free sign):

| Data | Fitted complex layout | Uniform layout | One decoder, layout given at run time |
|---|---|---|---|
| 4096 values, in cache | 1.20 | 1.43 | 1.82 |
| 10^7 values, from memory | 3.42 | 3.81 | 4.01 |

## G. Exact matrix products with wide entries (baseline FLINT fmpz_mat_mul with BLAS, from C; one thread)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| G 128 x 128 product, entries of 64 bits | 128 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 4.39 ms | 4.35 ms | 1% | 0.99x | 0 (every entry) |  | peak memory +2 MB |
| G 128 x 128 product, entries of 64 bits | 128 | FLINT fmpz_mat_mul, FLINT built with BLAS | 4.45 ms | 4.33 ms | 3% | 1.00x | 0 (every entry) |  | peak memory +3 MB |
| G 128 x 128 product, entries of 64 bits | 128 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 3.9 ms | 3.85 ms | 1% | 0.88x | 0 (every entry) |  | peak memory +4 MB |
| G 128 x 128 product, entries of 64 bits | 128 | FLINT fmpq_mat_mul (reduced rationals) | 7.74 ms | 7.39 ms | 5% | 1.74x | 0 (every entry) |  | peak memory +8 MB |
| G 128 x 128 product, entries of 64 bits | 128 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 8.42 ms | 8.24 ms | 2% | 1.89x | 0 (every entry) |  | 6 primes; reduce 3 ms, dgemm 1 ms, CRT 5 ms; peak memory +5 MB |
| G 128 x 128 product, entries of 64 bits | 128 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 3.21 ms | 3.16 ms | 1% | 0.72x | 0 (every entry) |  | 6 primes; reduce 2 ms, dgemm 0 ms, CRT 1 ms; peak memory +9 MB |
| G 128 x 128 product, entries of 64 bits | 128 | the same, everything timed: setup, fmpz to limbs and back, freeing | 4.01 ms | 3.89 ms | 3% | 0.90x | 0 (every entry) |  | 6 primes; reduce 2 ms, dgemm 0 ms, CRT 1 ms; peak memory +10 MB |
| G 128 x 128 product, entries of 256 bits | 128 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 15.7 ms | 15.7 ms | 0% | 1.00x | 0 (every entry) |  | peak memory +7 MB |
| G 128 x 128 product, entries of 256 bits | 128 | FLINT fmpz_mat_mul, FLINT built with BLAS | 15.7 ms | 15.7 ms | 1% | 1.00x | 0 (every entry) |  | peak memory +8 MB |
| G 128 x 128 product, entries of 256 bits | 128 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 15.4 ms | 15 ms | 3% | 0.98x | 0 (every entry) |  | peak memory +10 MB |
| G 128 x 128 product, entries of 256 bits | 128 | FLINT fmpq_mat_mul (reduced rationals) | 21.1 ms | 20.4 ms | 3% | 1.34x | 0 (every entry) |  | peak memory +17 MB |
| G 128 x 128 product, entries of 256 bits | 128 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 21.8 ms | 21.5 ms | 1% | 1.38x | 0 (every entry) |  | 22 primes; reduce 10 ms, dgemm 2 ms, CRT 9 ms; peak memory +13 MB |
| G 128 x 128 product, entries of 256 bits | 128 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 13.5 ms | 12.8 ms | 5% | 0.86x | 0 (every entry) |  | 22 primes; reduce 8 ms, dgemm 2 ms, CRT 4 ms; peak memory +24 MB |
| G 128 x 128 product, entries of 256 bits | 128 | the same, everything timed: setup, fmpz to limbs and back, freeing | 13.6 ms | 13.6 ms | 0% | 0.86x | 0 (every entry) |  | 22 primes; reduce 7 ms, dgemm 2 ms, CRT 4 ms; peak memory +28 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 84.3 ms | 83.8 ms | 1% | 0.94x | 0 (every entry) |  | peak memory +23 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | FLINT fmpz_mat_mul, FLINT built with BLAS | 89.5 ms | 88.3 ms | 1% | 1.00x | 0 (every entry) |  | peak memory +23 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 80.7 ms | 80.3 ms | 0% | 0.90x | 0 (every entry) |  | peak memory +30 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | FLINT fmpq_mat_mul (reduced rationals) | 106 ms | 101 ms | 5% | 1.18x | 0 (every entry) |  | peak memory +46 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 122 ms | 121 ms | 0% | 1.36x | 0 (every entry) |  | 86 primes; reduce 66 ms, dgemm 8 ms, CRT 47 ms; peak memory +43 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 64.4 ms | 64.3 ms | 0% | 0.72x | 0 (every entry) |  | 86 primes; reduce 36 ms, dgemm 8 ms, CRT 21 ms; peak memory +86 MB |
| G 128 x 128 product, entries of 1024 bits | 128 | the same, everything timed: setup, fmpz to limbs and back, freeing | 65.9 ms | 65.9 ms | 0% | 0.74x | 0 (every entry) |  | 86 primes; reduce 35 ms, dgemm 8 ms, CRT 21 ms; peak memory +102 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 222 ms | 222 ms | 0% | 0.95x | 0 (every entry) |  | peak memory +45 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | FLINT fmpz_mat_mul, FLINT built with BLAS | 234 ms | 232 ms | 0% | 1.00x | 0 (every entry) |  | peak memory +45 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 203 ms | 203 ms | 0% | 0.87x | 0 (every entry) |  | peak memory +56 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | FLINT fmpq_mat_mul (reduced rationals) | 245 ms | 242 ms | 1% | 1.05x | 0 (every entry) |  | peak memory +86 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 300 ms | 299 ms | 0% | 1.29x | 0 (every entry) |  | 172 primes; reduce 164 ms, dgemm 16 ms, CRT 120 ms; peak memory +83 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 152 ms | 151 ms | 1% | 0.65x | 0 (every entry) |  | 172 primes; reduce 77 ms, dgemm 16 ms, CRT 58 ms; peak memory +184 MB |
| G 128 x 128 product, entries of 2048 bits | 128 | the same, everything timed: setup, fmpz to limbs and back, freeing | 155 ms | 154 ms | 1% | 0.66x | 0 (every entry) |  | 172 primes; reduce 77 ms, dgemm 17 ms, CRT 57 ms; peak memory +216 MB |
| G 256 x 256 product, entries of 64 bits | 256 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 28.1 ms | 28 ms | 0% | 1.61x | 0 (every entry) |  | peak memory +9 MB |
| G 256 x 256 product, entries of 64 bits | 256 | FLINT fmpz_mat_mul, FLINT built with BLAS | 17.4 ms | 17 ms | 2% | 1.00x | 0 (every entry) |  | peak memory +12 MB |
| G 256 x 256 product, entries of 64 bits | 256 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 17.4 ms | 16.9 ms | 3% | 1.00x | 0 (every entry) |  | peak memory +12 MB |
| G 256 x 256 product, entries of 64 bits | 256 | FLINT fmpq_mat_mul (reduced rationals) | 58.8 ms | 57.1 ms | 3% | 3.37x | 0 (every entry) |  | peak memory +34 MB |
| G 256 x 256 product, entries of 64 bits | 256 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 24.5 ms | 24.4 ms | 0% | 1.41x | 0 (every entry) |  | 6 primes; reduce 8 ms, dgemm 4 ms, CRT 13 ms; peak memory +14 MB |
| G 256 x 256 product, entries of 64 bits | 256 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 14.8 ms | 14.8 ms | 0% | 0.85x | 0 (every entry) |  | 6 primes; reduce 8 ms, dgemm 3 ms, CRT 4 ms; peak memory +27 MB |
| G 256 x 256 product, entries of 64 bits | 256 | the same, everything timed: setup, fmpz to limbs and back, freeing | 17.8 ms | 17.4 ms | 2% | 1.02x | 0 (every entry) |  | 6 primes; reduce 8 ms, dgemm 3 ms, CRT 4 ms; peak memory +32 MB |
| G 256 x 256 product, entries of 256 bits | 256 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 93 ms | 92.9 ms | 0% | 0.98x | 0 (every entry) |  | peak memory +21 MB |
| G 256 x 256 product, entries of 256 bits | 256 | FLINT fmpz_mat_mul, FLINT built with BLAS | 95.3 ms | 94.9 ms | 0% | 1.00x | 0 (every entry) |  | peak memory +21 MB |
| G 256 x 256 product, entries of 256 bits | 256 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 62.7 ms | 62.5 ms | 0% | 0.66x | 0 (every entry) |  | peak memory +30 MB |
| G 256 x 256 product, entries of 256 bits | 256 | FLINT fmpq_mat_mul (reduced rationals) | 122 ms | 117 ms | 5% | 1.29x | 0 (every entry) |  | peak memory +58 MB |
| G 256 x 256 product, entries of 256 bits | 256 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 95.4 ms | 95.3 ms | 0% | 1.00x | 0 (every entry) |  | 23 primes; reduce 43 ms, dgemm 13 ms, CRT 39 ms; peak memory +43 MB |
| G 256 x 256 product, entries of 256 bits | 256 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 67 ms | 66.3 ms | 1% | 0.70x | 0 (every entry) |  | 23 primes; reduce 35 ms, dgemm 14 ms, CRT 19 ms; peak memory +89 MB |
| G 256 x 256 product, entries of 256 bits | 256 | the same, everything timed: setup, fmpz to limbs and back, freeing | 68.6 ms | 68.5 ms | 0% | 0.72x | 0 (every entry) |  | 23 primes; reduce 34 ms, dgemm 13 ms, CRT 18 ms; peak memory +106 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 451 ms | 450 ms | 0% | 0.94x | 0 (every entry) |  | peak memory +72 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | FLINT fmpz_mat_mul, FLINT built with BLAS | 479 ms | 477 ms | 0% | 1.00x | 0 (every entry) |  | peak memory +72 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 332 ms | 331 ms | 0% | 0.69x | 0 (every entry) |  | peak memory +97 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | FLINT fmpq_mat_mul (reduced rationals) | 522 ms | 502 ms | 4% | 1.09x | 0 (every entry) |  | peak memory +164 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 525 ms | 519 ms | 1% | 1.10x | 0 (every entry) |  | 88 primes; reduce 272 ms, dgemm 54 ms, CRT 197 ms; peak memory +152 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 283 ms | 280 ms | 1% | 0.59x | 0 (every entry) |  | 88 primes; reduce 148 ms, dgemm 55 ms, CRT 80 ms; peak memory +327 MB |
| G 256 x 256 product, entries of 1024 bits | 256 | the same, everything timed: setup, fmpz to limbs and back, freeing | 334 ms | 329 ms | 1% | 0.70x | 0 (every entry) |  | 88 primes; reduce 157 ms, dgemm 54 ms, CRT 110 ms; peak memory +371 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 1.18 s | 1.14 s | 4% | 1.00x | 0 (every entry) |  | peak memory +141 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | FLINT fmpz_mat_mul, FLINT built with BLAS | 1.19 s | 1.18 s | 0% | 1.00x | 0 (every entry) |  | peak memory +141 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 850 ms | 831 ms | 2% | 0.72x | 0 (every entry) |  | peak memory +187 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 1.31 s | 1.28 s | 2% | 1.10x | 0 (every entry) |  | 175 primes; reduce 690 ms, dgemm 105 ms, CRT 500 ms; peak memory +299 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 648 ms | 639 ms | 1% | 0.55x | 0 (every entry) |  | 175 primes; reduce 311 ms, dgemm 99 ms, CRT 238 ms; peak memory +660 MB |
| G 256 x 256 product, entries of 2048 bits | 256 | the same, everything timed: setup, fmpz to limbs and back, freeing | 841 ms | 834 ms | 1% | 0.71x | 0 (every entry) |  | 175 primes; reduce 379 ms, dgemm 127 ms, CRT 300 ms; peak memory +660 MB |
| G 512 x 512 product, entries of 64 bits | 512 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 185 ms | 183 ms | 1% | 2.09x | 0 (every entry) |  | peak memory +35 MB |
| G 512 x 512 product, entries of 64 bits | 512 | FLINT fmpz_mat_mul, FLINT built with BLAS | 88.4 ms | 87.9 ms | 1% | 1.00x | 0 (every entry) |  | peak memory +45 MB |
| G 512 x 512 product, entries of 64 bits | 512 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 93 ms | 88 ms | 5% | 1.05x | 0 (every entry) |  | peak memory +45 MB |
| G 512 x 512 product, entries of 64 bits | 512 | FLINT fmpq_mat_mul (reduced rationals) | 191 ms | 188 ms | 1% | 2.16x | 0 (every entry) |  | peak memory +135 MB |
| G 512 x 512 product, entries of 64 bits | 512 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 127 ms | 126 ms | 1% | 1.43x | 0 (every entry) |  | 7 primes; reduce 39 ms, dgemm 27 ms, CRT 62 ms; peak memory +58 MB |
| G 512 x 512 product, entries of 64 bits | 512 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 89.7 ms | 88.7 ms | 1% | 1.01x | 0 (every entry) |  | 7 primes; reduce 41 ms, dgemm 26 ms, CRT 22 ms; peak memory +117 MB |
| G 512 x 512 product, entries of 64 bits | 512 | the same, everything timed: setup, fmpz to limbs and back, freeing | 99.6 ms | 99.1 ms | 0% | 1.13x | 0 (every entry) |  | 7 primes; reduce 40 ms, dgemm 26 ms, CRT 22 ms; peak memory +141 MB |
| G 512 x 512 product, entries of 256 bits | 512 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 595 ms | 593 ms | 0% | 1.98x | 0 (every entry) |  | peak memory +83 MB |
| G 512 x 512 product, entries of 256 bits | 512 | FLINT fmpz_mat_mul, FLINT built with BLAS | 300 ms | 294 ms | 2% | 1.00x | 0 (every entry) |  | peak memory +114 MB |
| G 512 x 512 product, entries of 256 bits | 512 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 296 ms | 294 ms | 0% | 0.99x | 0 (every entry) |  | peak memory +114 MB |
| G 512 x 512 product, entries of 256 bits | 512 | FLINT fmpq_mat_mul (reduced rationals) | 412 ms | 412 ms | 0% | 1.37x | 0 (every entry) |  | peak memory +261 MB |
| G 512 x 512 product, entries of 256 bits | 512 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 416 ms | 413 ms | 1% | 1.38x | 0 (every entry) |  | 23 primes; reduce 175 ms, dgemm 86 ms, CRT 153 ms; peak memory +167 MB |
| G 512 x 512 product, entries of 256 bits | 512 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 305 ms | 304 ms | 0% | 1.01x | 0 (every entry) |  | 23 primes; reduce 141 ms, dgemm 86 ms, CRT 78 ms; peak memory +351 MB |
| G 512 x 512 product, entries of 256 bits | 512 | the same, everything timed: setup, fmpz to limbs and back, freeing | 351 ms | 339 ms | 3% | 1.17x | 0 (every entry) |  | 23 primes; reduce 143 ms, dgemm 95 ms, CRT 87 ms; peak memory +397 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 2.66 s | 2.66 s | 0% | 1.72x | 0 (every entry) |  | peak memory +287 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | FLINT fmpz_mat_mul, FLINT built with BLAS | 1.55 s | 1.55 s | 0% | 1.00x | 0 (every entry) |  | peak memory +384 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 1.55 s | 1.55 s | 0% | 1.00x | 0 (every entry) |  | peak memory +384 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 2.25 s | 2.25 s | 0% | 1.45x | 0 (every entry) |  | 90 primes; reduce 1112 ms, dgemm 338 ms, CRT 796 ms; peak memory +617 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 1.28 s | 1.24 s | 3% | 0.83x | 0 (every entry) |  | 90 primes; reduce 618 ms, dgemm 327 ms, CRT 329 ms; peak memory +1320 MB |
| G 512 x 512 product, entries of 1024 bits | 512 | the same, everything timed: setup, fmpz to limbs and back, freeing | 3.55 s | 1.66 s | 53% ⚠ | 2.29x | 0 (every entry) |  | 90 primes; reduce 756 ms, dgemm 381 ms, CRT 2338 ms; peak memory +1320 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | FLINT fmpz_mat_mul, FLINT built without BLAS (the system's) | 6.32 s | 6.24 s | 1% | 1.66x | 0 (every entry) |  | peak memory +562 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | FLINT fmpz_mat_mul, FLINT built with BLAS | 3.81 s | 3.77 s | 1% | 1.00x | 0 (every entry) |  | peak memory +745 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | FLINT fmpz_mat_mul_blas (FLINT's multimodular BLAS method) | 3.81 s | 3.77 s | 1% | 1.00x | 0 (every entry) |  | peak memory +746 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | multimodular OpenBLAS dgemm, FLINT reduction and CRT | 5.49 s | 5.48 s | 0% | 1.44x | 0 (every entry) |  | 179 primes; reduce 2781 ms, dgemm 711 ms, CRT 2019 ms; peak memory +1216 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | multimodular OpenBLAS dgemm, reduction and CRT as dgemms (uN limb layout) | 2.9 s | 2.89 s | 0% | 0.76x | 0 (every entry) |  | 179 primes; reduce 1275 ms, dgemm 659 ms, CRT 948 ms; peak memory +2622 MB |
| G 512 x 512 product, entries of 2048 bits | 512 | the same, everything timed: setup, fmpz to limbs and back, freeing | 4.93 s | 4.93 s | 0% | 1.29x | 0 (every entry) |  | 179 primes; reduce 1560 ms, dgemm 964 ms, CRT 2387 ms; peak memory +2622 MB |
