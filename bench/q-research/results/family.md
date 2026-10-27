# Research for the division-algebra family

- **date**: 2026-10-26 20:24:26 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1
- **compiled**: section B (un.c, every method in it): clang 18 -O3 -march=native, for _BitInt; section C (quat.c): gcc 13 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1

## B. Fixed-width uN tiers (baseline GMP mpz_t numerators over the shared denominator, from C)

The uN vector, GMP mpz_t and FLINT fmpz all keep integers over the shared denominator and never reduce; GMP mpq_t reduces every operation, so its column measures rationals, not widths.

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| B (a*b)^2, none wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 128 µs | 124 µs | 3% | 0.13x | 0 (every element) |  | vector width 128 bits |
| B (a*b)^2, none wide | 100000 | GMP mpz_t numerators (C) | 966 µs | 910 µs | 6% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, none wide | 100000 | FLINT fmpz numerators (C) | 1.35 ms | 1.32 ms | 2% | 1.39x | 0 (every element) |  |  |
| B (a*b)^2, none wide | 100000 | GMP mpq_t (C) | 11 ms | 10.9 ms | 1% | 11.41x | 0 (every element) |  |  |
| B sum of (a*b)^2, none wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 145 µs | 143 µs | 1% | 0.12x | 0 (exact) |  | vector width 128 bits |
| B sum of (a*b)^2, none wide | 100000 | GMP mpz_t numerators (C) | 1.21 ms | 1.16 ms | 4% | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, none wide | 100000 | FLINT fmpz numerators (C) | 1.79 ms | 1.69 ms | 5% | 1.48x | 0 (exact) |  |  |
| B sum of (a*b)^2, none wide | 100000 | GMP mpq_t (C) | 24.3 ms | 24.1 ms | 1% | 20.12x | 0 (exact) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | uN vector (fixed widths 64-1024, compiled) | 588 µs | 532 µs | 10% | 0.10x | 0 (every element) |  | ends at width 128 bits |
| B growth chain x <- x*b + c, 3 steps | 100000 | GMP mpz_t numerators (C) | 5.66 ms | 5.49 ms | 3% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | FLINT fmpz numerators (C) | 3.64 ms | 3.6 ms | 1% | 0.64x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 3 steps | 100000 | GMP mpq_t (C) | 55.8 ms | 55.3 ms | 1% | 9.86x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | uN vector (fixed widths 64-1024, compiled) | 3.62 ms | 3.53 ms | 3% | 0.18x | 0 (every element) |  | ends at width 256 bits |
| B growth chain x <- x*b + c, 10 steps | 100000 | GMP mpz_t numerators (C) | 20.2 ms | 19.8 ms | 2% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | FLINT fmpz numerators (C) | 26.7 ms | 25.1 ms | 6% | 1.32x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 10 steps | 100000 | GMP mpq_t (C) | 192 ms | 187 ms | 2% | 9.47x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | uN vector (fixed widths 64-1024, compiled) | 24.7 ms | 24.4 ms | 1% | 0.41x | 0 (every element) |  | ends at width 512 bits |
| B growth chain x <- x*b + c, 25 steps | 100000 | GMP mpz_t numerators (C) | 59.9 ms | 59.2 ms | 1% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | FLINT fmpz numerators (C) | 95.3 ms | 88.3 ms | 7% | 1.59x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 25 steps | 100000 | GMP mpq_t (C) | 523 ms | 518 ms | 1% | 8.73x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | uN vector (fixed widths 64-1024, compiled) | 82.9 ms | 81.8 ms | 1% | 0.66x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 40 steps | 100000 | GMP mpz_t numerators (C) | 126 ms | 124 ms | 2% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | FLINT fmpz numerators (C) | 188 ms | 167 ms | 11% ⚠ | 1.49x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 40 steps | 100000 | GMP mpq_t (C) | 979 ms | 958 ms | 2% | 7.75x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | uN vector (fixed widths 64-1024, compiled) | 298 ms | 294 ms | 1% | 1.00x | 0 (every element) |  | ends at width 1024 bits |
| B growth chain x <- x*b + c, 70 steps | 100000 | GMP mpz_t numerators (C) | 299 ms | 291 ms | 3% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | FLINT fmpz numerators (C) | 392 ms | 385 ms | 2% | 1.31x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 70 steps | 100000 | GMP mpq_t (C) | 2.72 s | 2.48 s | 9% | 9.10x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 90 steps | 100000 | uN vector (fixed widths 64-1024, compiled) |  |  |  |  | exceeds the 1024-bit cap |  | detected before overflow; would continue in arbitrary precision |
| B growth chain x <- x*b + c, 90 steps | 100000 | GMP mpz_t numerators (C) | 447 ms | 439 ms | 2% | 1.00x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 90 steps | 100000 | FLINT fmpz numerators (C) | 577 ms | 551 ms | 5% | 1.29x | 0 (every element) |  |  |
| B growth chain x <- x*b + c, 90 steps | 100000 | GMP mpq_t (C) | 4 s | 3.65 s | 9% | 8.94x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 448 µs | 424 µs | 5% | 0.45x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, 1% wide | 100000 | GMP mpz_t numerators (C) | 996 µs | 944 µs | 5% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | FLINT fmpz numerators (C) | 1.39 ms | 1.35 ms | 3% | 1.40x | 0 (every element) |  |  |
| B (a*b)^2, 1% wide | 100000 | GMP mpq_t (C) | 11.2 ms | 10.9 ms | 2% | 11.25x | 0 (every element) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 477 µs | 459 µs | 4% | 0.39x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, 1% wide | 100000 | GMP mpz_t numerators (C) | 1.22 ms | 1.18 ms | 3% | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | FLINT fmpz numerators (C) | 1.89 ms | 1.71 ms | 10% | 1.56x | 0 (exact) |  |  |
| B sum of (a*b)^2, 1% wide | 100000 | GMP mpq_t (C) | 24.5 ms | 24.3 ms | 1% | 20.15x | 0 (exact) |  |  |
| B (a*b)^2, all wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 450 µs | 410 µs | 9% | 0.25x | 0 (every element) |  | vector width 256 bits |
| B (a*b)^2, all wide | 100000 | GMP mpz_t numerators (C) | 1.8 ms | 1.75 ms | 3% | 1.00x | 0 (every element) |  |  |
| B (a*b)^2, all wide | 100000 | FLINT fmpz numerators (C) | 2.19 ms | 2.13 ms | 3% | 1.22x | 0 (every element) |  |  |
| B (a*b)^2, all wide | 100000 | GMP mpq_t (C) | 9.99 ms | 9.61 ms | 4% | 5.54x | 0 (every element) |  |  |
| B sum of (a*b)^2, all wide | 100000 | uN vector (fixed widths 64-1024, compiled) | 459 µs | 452 µs | 2% | 0.21x | 0 (exact) |  | vector width 256 bits |
| B sum of (a*b)^2, all wide | 100000 | GMP mpz_t numerators (C) | 2.19 ms | 2.12 ms | 3% | 1.00x | 0 (exact) |  |  |
| B sum of (a*b)^2, all wide | 100000 | FLINT fmpz numerators (C) | 3.12 ms | 2.92 ms | 6% | 1.42x | 0 (exact) |  |  |
| B sum of (a*b)^2, all wide | 100000 | GMP mpq_t (C) | 23.3 ms | 23.1 ms | 1% | 10.64x | 0 (exact) |  |  |

## C. Quaternion arrays (baseline FLINT fmpz over one denominator per value, from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| C p + q, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 1.13 ms | 1.09 ms | 4% | 0.70x | 0 (every element) | 32 |  |
| C p + q, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 93.5 ms | 91.1 ms | 3% | 57.61x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 44.6 ms | 44.1 ms | 1% | 27.47x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 1.62 ms | 1.57 ms | 3% | 1.00x | 0 (every element) |  |  |
| C p + q, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 677 µs | 642 µs | 5% | 0.42x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 1.31 ms | 1.28 ms | 3% | 0.05x | 0 (every element) | 32 |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 578 ms | 572 ms | 1% | 22.12x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 394 ms | 387 ms | 2% | 15.10x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 26.1 ms | 18 ms | 31% ⚠ | 1.00x | 0 (every element) |  |  |
| C Hamilton product p q, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 1.48 ms | 1.46 ms | 1% | 0.06x | 0 (every element) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 457 µs | 453 µs | 1% | 0.20x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 95.4 ms | 92.3 ms | 3% | 42.17x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 82.8 ms | 82.5 ms | 0% | 36.63x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 2.26 ms | 2.17 ms | 4% | 1.00x | 0 (exact) |  |  |
| C sum of squared norms, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 728 µs | 621 µs | 15% ⚠ | 0.32x | 0 (exact) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | quaternion vector form (four int64 vectors, compiled) | 4.16 ms | 4.02 ms | 3% | 0.12x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | GMP mpq_t 4-tuples (C) | 1.04 s | 1.03 s | 1% | 30.29x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | FLINT fmpq 4-tuples (C) | 850 ms | 837 ms | 2% | 24.88x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | FLINT fmpz over one denominator per value (C) | 34.2 ms | 33.8 ms | 1% | 1.00x | 0 (every element) |  |  |
| C rotation q v q*, 3 places, components below 1000 | 200000 | C int64 4-tuples, overflow-checked | 2.97 ms | 2.91 ms | 2% | 0.09x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 546 µs | 536 µs | 2% | 0.69x | 0 (every element) | 32 |  |
| C p + q, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 47.6 ms | 47.4 ms | 0% | 60.49x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 33.4 ms | 33.2 ms | 1% | 42.42x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 787 µs | 772 µs | 2% | 1.00x | 0 (every element) |  |  |
| C p + q, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 337 µs | 329 µs | 2% | 0.43x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 798 µs | 759 µs | 5% | 0.04x | 0 (every element) | 64 |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 335 ms | 332 ms | 1% | 16.05x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 337 ms | 335 ms | 1% | 16.14x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 20.9 ms | 20.5 ms | 2% | 1.00x | 0 (every element) |  |  |
| C Hamilton product p q, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 722 µs | 700 µs | 3% | 0.03x | overflow detected |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 307 µs | 299 µs | 3% | 0.10x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 58.2 ms | 58.1 ms | 0% | 18.19x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 56 ms | 55.5 ms | 1% | 17.48x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 3.2 ms | 3.14 ms | 2% | 1.00x | 0 (exact) |  |  |
| C sum of squared norms, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 356 µs | 314 µs | 12% ⚠ | 0.11x | overflow detected |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | quaternion vector form (four int64 vectors, compiled) | 2.04 ms | 1.99 ms | 3% | 0.04x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | GMP mpq_t 4-tuples (C) | 681 ms | 677 ms | 1% | 14.30x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | FLINT fmpq 4-tuples (C) | 699 ms | 689 ms | 1% | 14.68x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | FLINT fmpz over one denominator per value (C) | 47.6 ms | 46.1 ms | 3% | 1.00x | 0 (every element) |  |  |
| C rotation q v q*, 6 places, components below 1e+06 | 100000 | C int64 4-tuples, overflow-checked | 1.49 ms | 1.44 ms | 3% | 0.03x | overflow detected |  |  |

## D. Precision of each member

| Member | Size | Unit bits | Slash bits (borrowed) | Data bits | Median relative error | Worst (beside 1) | Float significand to match: median | worst |
|---|---|---|---|---|---|---|---|---|
| reals | 136 | 1 | 7 (0) | 128 | 3.4e-39 | 2.7e-20 | 127 | 65 |
| complex | 72 | 2 | 6 (0) | 64 | 7.3e-20 | 1.2e-10 | 62 | 33 |
| quaternions | 40 | 3 | 5 (0) | 32 | 2.6e-10 | 7.6e-06 | 31 | 17 |
| octonions | 24 | 4 | 4 (0) | 16 | 1.8e-05 | 2.0e-03 | 15 | 9 |
| quaternions, promoted once | 72 | 3 | 6 (1) | 63 | 1.3e-19 | 2.3e-10 | 62 | 32 |
| quaternions, promoted twice | 136 | 3 | 7 (2) | 126 | 1.6e-38 | 5.4e-20 | 125 | 64 |
