# Phase two: the format against GMP and FLINT

- **date**: 2026-10-27 07:36:05 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1
- **compiled**: p2_elem, p2_scalar: clang 18 -O3 -march=native; GMP 6.3.0, FLINT 3.0.1

## E. Element by element, $n = 100000$, by width

### 128 bits

The format: sign and magnitude limbs, compile-time or run-time routines (GMP's kernels), whichever was faster on a calibration run (noted). Times: median over 7 interleaved rounds. Ratio = best library / the format (above 1: the format is faster). Bold: the fastest, unless another is within 5% (a tie).

| Operation | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz, shared denominator | FLINT fmpq | Best library / the format | the format, two's complement _BitInt (for information) |
|---|---|---|---|---|---|---|---|
| a + b | **614 µs** (compile-time limbs) | 1.22 ms | 10.6 ms | 1.65 ms | 11.3 ms | 1.98 | 161 µs |
| a b | **120 µs** (compile-time limbs) | 537 µs | 8.58 ms | 887 µs | 12.1 ms | 4.48 | 110 µs |
| dot product | **426 µs** (compile-time limbs) | 1.2 ms | 18.8 ms | 1.4 ms | 25.6 ms | 2.82 | 55.8 µs |
| divide by 999 (exact) | **114 µs** (compile-time limbs) | 582 µs | 4.9 ms | 802 µs | 6.38 ms | 5.12 | 100 µs |
| divide by 1000 (exact) | **123 µs** (compile-time limbs) | 592 µs | 7.24 ms | 819 µs | 7.29 ms | 4.80 | 117 µs |
| sort | **19.6 ms** (run-time limbs, GMP kernels) | 50.4 ms | 73.6 ms | 58.4 ms | 105 ms | 2.57 | 17.8 ms |
| bit-length scan (shared denominators only) | **22.9 µs** (compile-time limbs) | 575 µs | — | 775 µs | — | 25.09 | 56.4 µs |
| decimal text in | **5.15 ms** (limbs) | 8.37 ms | 12.1 ms | 7.77 ms | 11.7 ms | 1.51 | 6.37 ms |
| decimal text out | **5.16 ms** (limbs) | 7.67 ms | 9.59 ms | 7.95 ms | 9.97 ms | 1.49 | 5.22 ms |
| growth chain to this width (6 steps) | **3.11 ms** (compile-time limbs) | 14.1 ms | 116 ms | 12 ms | 134 ms | 3.86 | 1.27 ms |

### 256 bits

The format: sign and magnitude limbs, compile-time or run-time routines (GMP's kernels), whichever was faster on a calibration run (noted). Times: median over 7 interleaved rounds. Ratio = best library / the format (above 1: the format is faster). Bold: the fastest, unless another is within 5% (a tie).

| Operation | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz, shared denominator | FLINT fmpq | Best library / the format | the format, two's complement _BitInt (for information) |
|---|---|---|---|---|---|---|---|
| a + b | **818 µs** (compile-time limbs) | 1.27 ms | 11.3 ms | 1.73 ms | 11.6 ms | 1.55 | 325 µs |
| a b | **246 µs** (compile-time limbs) | 710 µs | 9.07 ms | 737 µs | 9.72 ms | 2.89 | 295 µs |
| dot product | **600 µs** (compile-time limbs) | 1.74 ms | 20.7 ms | 2.27 ms | 24.6 ms | 2.90 | 275 µs |
| divide by 999 (exact) | **245 µs** (compile-time limbs) | 818 µs | 5.27 ms | 1.03 ms | 6.72 ms | 3.33 | 305 µs |
| divide by 1000 (exact) | **286 µs** (compile-time limbs) | 837 µs | 7.99 ms | 1.06 ms | 7.82 ms | 2.93 | 402 µs |
| sort | **29.6 ms** (run-time limbs, GMP kernels) | 60.6 ms | 83 ms | 65.8 ms | 110 ms | 2.05 | 30.4 ms |
| bit-length scan (shared denominators only) | **97.8 µs** (compile-time limbs) | 592 µs | — | 781 µs | — | 6.05 | 144 µs |
| decimal text in | **11.6 ms** (limbs) | 13.5 ms | 18.5 ms | 13.2 ms | 18.4 ms | 1.14 | 12 ms |
| decimal text out | **9.54 ms** (limbs) | 12.2 ms | 14.2 ms | 12.5 ms | 14.6 ms | 1.28 | 9.81 ms |
| growth chain to this width (15 steps) | **12.1 ms** (compile-time limbs) | 36 ms | 307 ms | 47.2 ms | 449 ms | 2.96 | 7.11 ms |

### 512 bits

The format: sign and magnitude limbs, compile-time or run-time routines (GMP's kernels), whichever was faster on a calibration run (noted). Times: median over 7 interleaved rounds. Ratio = best library / the format (above 1: the format is faster). Bold: the fastest, unless another is within 5% (a tie).

| Operation | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz, shared denominator | FLINT fmpq | Best library / the format | the format, two's complement _BitInt (for information) |
|---|---|---|---|---|---|---|---|
| a + b | **974 µs** (compile-time limbs) | 1.47 ms | 12.8 ms | 1.9 ms | 12.6 ms | 1.50 | 674 µs |
| a b | **745 µs** (compile-time limbs) | 1.35 ms | 9.9 ms | 1.55 ms | 10.9 ms | 1.81 | 1.26 ms |
| dot product | **1.28 ms** (compile-time limbs) | 2.3 ms | 23.4 ms | 2.83 ms | 26.6 ms | 1.80 | 1.24 ms |
| divide by 999 (exact) | **558 µs** (compile-time limbs) | 1.32 ms | 7.02 ms | 1.55 ms | 7.31 ms | 2.36 | 1.34 ms |
| divide by 1000 (exact) | **642 µs** (compile-time limbs) | 1.34 ms | 9.09 ms | 1.6 ms | 9.25 ms | 2.09 | 1.54 ms |
| sort | **43.9 ms** (run-time limbs, GMP kernels) | 72.2 ms | 94.5 ms | 78.4 ms | 117 ms | 1.65 | 34.6 ms |
| bit-length scan (shared denominators only) | **213 µs** (compile-time limbs) | 653 µs | — | 813 µs | — | 3.06 | 303 µs |
| decimal text in | **21.3 ms** (limbs) | 23.9 ms | 30.2 ms | 22.6 ms | 30.2 ms | 1.06 | 22.1 ms |
| decimal text out | **19.5 ms** (limbs) | 25.9 ms | 28.6 ms | 26.4 ms | 28.9 ms | 1.32 | 19.6 ms |
| growth chain to this width (32 steps) | **39.9 ms** (compile-time limbs) | 84.1 ms | 749 ms | 147 ms | 1.21 s | 2.11 | 38.9 ms |

### 1024 bits

The format: sign and magnitude limbs, compile-time or run-time routines (GMP's kernels), whichever was faster on a calibration run (noted). Times: median over 7 interleaved rounds. Ratio = best library / the format (above 1: the format is faster). Bold: the fastest, unless another is within 5% (a tie).

| Operation | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz, shared denominator | FLINT fmpq | Best library / the format | the format, two's complement _BitInt (for information) |
|---|---|---|---|---|---|---|---|
| a + b | **1.53 ms** (compile-time limbs) | 1.92 ms | 15.7 ms | 2.37 ms | 15 ms | 1.25 | 1.42 ms |
| a b | 3.57 ms (run-time limbs, GMP kernels) | 3.68 ms | 14.1 ms | 3.81 ms | 15 ms | 1.03 | 4.93 ms |
| dot product | **4.08 ms** (run-time limbs, GMP kernels) | 4.89 ms | 29.7 ms | 4.91 ms | 32.6 ms | 1.20 | 4.65 ms |
| divide by 999 (exact) | **1.72 ms** (compile-time limbs) | 2.62 ms | 11.9 ms | 2.97 ms | 11.5 ms | 1.52 | 5.52 ms |
| divide by 1000 (exact) | **1.64 ms** (compile-time limbs) | 2.94 ms | 11.8 ms | 3.05 ms | 13.1 ms | 1.79 | 6.21 ms |
| sort | **56.8 ms** (run-time limbs, GMP kernels) | 86.4 ms | 115 ms | 93 ms | 141 ms | 1.52 | 49.6 ms |
| bit-length scan (shared denominators only) | **381 µs** (run-time limbs, GMP kernels) | 732 µs | — | 910 µs | — | 1.92 | 753 µs |
| decimal text in | **43.8 ms** (limbs) | 48.5 ms | 57.1 ms | 47.7 ms | 60.4 ms | 1.09 | 48.1 ms |
| decimal text out | **52.2 ms** (limbs) | 66.5 ms | 71.1 ms | 66.8 ms | 70.1 ms | 1.28 | 52.7 ms |
| growth chain to this width (66 steps) | **152 ms** (compile-time limbs) | 196 ms | 1.86 s | 391 ms | 2.78 s | 1.29 | 247 ms |

### 2048 bits

The format: sign and magnitude limbs, run-time limbs with GMP's kernels. Times: median over 7 interleaved rounds. Ratio = best library / the format (above 1: the format is faster). Bold: the fastest, unless another is within 5% (a tie).

| Operation | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz, shared denominator | FLINT fmpq | Best library / the format |
|---|---|---|---|---|---|---|
| a + b | **2.9 ms** (run-time limbs, GMP kernels) | 3.16 ms | 20.4 ms | 3.3 ms | 20.4 ms | 1.09 |
| a b | 13 ms (run-time limbs, GMP kernels) | 13.4 ms | 26.6 ms | 13 ms | 28.6 ms | 1.01 |
| dot product | **13.4 ms** (run-time limbs, GMP kernels) | 14.2 ms | 45.3 ms | 14.3 ms | 50.4 ms | 1.06 |
| divide by 999 (exact) | **4.71 ms** (run-time limbs, GMP kernels) | 5.8 ms | 17.3 ms | 6.14 ms | 18.3 ms | 1.23 |
| divide by 1000 (exact) | **5.57 ms** (run-time limbs, GMP kernels) | 5.86 ms | 14.2 ms | 6.39 ms | 18.9 ms | 1.05 |
| sort | **67.8 ms** (run-time limbs, GMP kernels) | 96 ms | 172 ms | 99.7 ms | 179 ms | 1.42 |
| bit-length scan (shared denominators only) | **793 µs** (run-time limbs, GMP kernels) | 875 µs | — | 1.01 ms | — | 1.10 |
| decimal text in | 105 ms (limbs) | 108 ms | 118 ms | 105 ms | 125 ms | 1.00 |
| decimal text out | **129 ms** (limbs) | 156 ms | 161 ms | 157 ms | 161 ms | 1.21 |
| growth chain to this width (134 steps) | 524 ms (run-time limbs, GMP kernels) | **496 ms** | 7.63 s | 1.16 s | 9.87 s | 0.95 |

## S. Single values

$n = 1000000$ independent values per operation, each with its own denominator; median over 3 rounds. Operand sizes: a third of the data bits (17 bits at 64, 38 at 128), so that sums fit; and half (26 and 57 bits) for products and sorting.

| Size | Operands | Operation | the packed format | GMP mpq_t | FLINT fmpq | Best library / the format |
|---|---|---|---|---|---|---|
| 64 | third | add | 31.7 ms | 72 ms | 30.1 ms | 0.95 |
| 64 | third | mul | 55.7 ms | 93.1 ms | 41.3 ms | 0.74 |
| 64 | third | sort | 242 ms | 1.81 s | 328 ms | 1.35 |
| 64 | half | mul | 68.8 ms | 104 ms | 55 ms | 0.80 |
| 64 | half | sort | 249 ms | 1.9 s | 351 ms | 1.41 |
| 128 | third | add | 50.2 ms | 78.1 ms | 43 ms | 0.86 |
| 128 | third | mul | 89.5 ms | 113 ms | 71.3 ms | 0.80 |
| 128 | third | sort | 448 ms | 1.94 s | 354 ms | 0.79 |
| 128 | half | mul | 118 ms | 125 ms | 100 ms | 0.85 |
| 128 | half | sort | 458 ms | 1.91 s | 376 ms | 0.82 |

## L. Linear algebra

Ratio = best library / the format (above 1: the format is faster); peak memory is the rise during the run. GMP rationals are omitted where one run takes tens of seconds (dense product at 256, solve at 64). FLINT's routine is chosen per cell on a calibration run (listed under each table); FLINT install: /home/claude/flintblas/inst-native.

### matrix-vector, n = 512

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **293 µs** | 3.42 ms | 79.8 ms | 1.09 ms | 10.4 ms | 3.72 | +2 / +2 MB |
| 256 | **945 µs** | 4.57 ms | 82.5 ms | 2.54 ms | 17.1 ms | 2.69 | +4 / +15 MB |
| 512 | **2.85 ms** | 6.61 ms | 92.5 ms | 7.33 ms | 20.5 ms | 2.32 | +8 / +16 MB |
| 1024 | **8.92 ms** | 12.2 ms | 110 ms | 13.1 ms | 28.2 ms | 1.37 | +16 / +24 MB |
| 2048 | **34.5 ms** | 35.6 ms | 155 ms | 37.1 ms | 56.3 ms | 1.03 | +32 / +40 MB |

Routines: 128: the format compile-time limbs, FLINT fmpz_mat, shared denominator mat, FLINT fmpq_mat cleared; 256: the format compile-time limbs, FLINT fmpz_mat, shared denominator mat, FLINT fmpq_mat cleared; 512: the format compile-time limbs, FLINT fmpz_mat, shared denominator vec, FLINT fmpq_mat cleared; 1024: the format assembly kernels, FLINT fmpz_mat, shared denominator vec, FLINT fmpq_mat mul; 2048: the format run-time limbs, FLINT fmpz_mat, shared denominator dot, FLINT fmpq_mat cleared.

### dense matrix product, n = 128

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **1.07 ms** | 26 ms | 629 ms | 1.75 ms | 4.95 ms | 1.63 | +3 / +1 MB |
| 256 | **1.95 ms** | 36.7 ms | 639 ms | 4.46 ms | 8.26 ms | 2.29 | +4 / +3 MB |
| 512 | **3.43 ms** | 47.8 ms | 713 ms | 11.5 ms | 19.1 ms | 3.36 | +6 / +10 MB |
| 1024 | **6.16 ms** | 94.3 ms | 862 ms | 27.2 ms | 38.7 ms | 4.41 | +11 / +16 MB |
| 2048 | **12.9 ms** | 280 ms | 1.22 s | 70.1 ms | 88.7 ms | 5.44 | +21 / +30 MB |

Routines: 128: the format IFMA kernel, FLINT fmpz_mat, shared denominator mul, FLINT fmpq_mat cleared; 256: the format IFMA kernel, FLINT fmpz_mat, shared denominator mul, FLINT fmpq_mat cleared; 512: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat cleared; 1024: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat mul; 2048: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat cleared.

### dense matrix product, n = 256

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **5.36 ms** | 211 ms | — | 7.89 ms | 20.8 ms | 1.47 | +9 / +6 MB |
| 256 | **10.5 ms** | 300 ms | — | 26.5 ms | 46 ms | 2.52 | +14 / +24 MB |
| 512 | **17.7 ms** | 390 ms | — | 51.5 ms | 103 ms | 2.90 | +23 / +38 MB |
| 1024 | **32.4 ms** | 758 ms | — | 122 ms | 210 ms | 3.76 | +42 / +64 MB |
| 2048 | **69.4 ms** | 2.31 s | — | 304 ms | 468 ms | 4.39 | +78 / +118 MB |

Routines: 128: the format IFMA kernel, FLINT fmpz_mat, shared denominator mul, FLINT fmpq_mat cleared; 256: the format IFMA kernel, FLINT fmpz_mat, shared denominator mul, FLINT fmpq_mat mul; 512: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat cleared; 1024: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat cleared; 2048: the format IFMA kernel, FLINT fmpz_mat, shared denominator blas, FLINT fmpq_mat mul.

### sparse matrix-vector (1% nonzero), n = 10000

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **1.49 ms** | 14.9 ms | 324 ms | 16.6 ms | 370 ms | 10.02 | +9 / +47 MB |
| 256 | **3.22 ms** | 22.2 ms | 334 ms | 29.4 ms | 401 ms | 6.90 | +17 / +47 MB |
| 512 | **11.7 ms** | 30 ms | 367 ms | 39.2 ms | 414 ms | 2.56 | +32 / +63 MB |
| 1024 | **46.3 ms** | 57.4 ms | 447 ms | 66.5 ms | 508 ms | 1.24 | +64 / +94 MB |
| 2048 | **146 ms** | 152 ms | 627 ms | 161 ms | 682 ms | 1.05 | +127 / +157 MB |

Routines: 128: the format compile-time limbs, FLINT fmpz_mat, shared denominator addmul, FLINT fmpq_mat addmul; 256: the format compile-time limbs, FLINT fmpz_mat, shared denominator addmul, FLINT fmpq_mat addmul; 512: the format compile-time limbs, FLINT fmpz_mat, shared denominator addmul, FLINT fmpq_mat addmul; 1024: the format assembly kernels, FLINT fmpz_mat, shared denominator addmul, FLINT fmpq_mat addmul; 2048: the format run-time limbs, FLINT fmpz_mat, shared denominator addmul, FLINT fmpq_mat addmul.

### linear solve, n = 32

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **124 µs** | 652 µs | 17.7 ms | 357 µs | 487 µs | 2.88 | +1 / +1 MB |
| 256 | **187 µs** | 1.1 ms | 36.4 ms | 647 µs | 794 µs | 3.47 | +1 / +1 MB |
| 512 | **407 µs** | 2.53 ms | 78.4 ms | 1.94 ms | 1.98 ms | 4.77 | +1 / +1 MB |
| 1024 | **798 µs** | 7 ms | 176 ms | 4.78 ms | 5.63 ms | 5.99 | +1 / +1 MB |
| 2048 | **1.7 ms** | 21.3 ms | 410 ms | 14.2 ms | 11 ms | 6.47 | +1 / +1 MB |

Routines: 128: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator solve, FLINT fmpq_mat solve; 256: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator q_solve, FLINT fmpq_mat dixon; 512: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator solve, FLINT fmpq_mat solve; 1024: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator q_dixon, FLINT fmpq_mat dixon; 2048: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator dixon, FLINT fmpq_mat solve.

### linear solve, n = 64

| Width | the format | GMP mpz_t, shared denominator | GMP mpq_t | FLINT fmpz_mat, shared denominator | FLINT fmpq_mat | Best library / the format | Memory: the format / best library |
|---|---|---|---|---|---|---|---|
| 128 | **697 µs** | 8.07 ms | — | 1.86 ms | 2.06 ms | 2.67 | +1 / +1 MB |
| 256 | **1.37 ms** | 18.3 ms | — | 3.13 ms | 3.78 ms | 2.29 | +1 / +1 MB |
| 512 | **2.66 ms** | 51.6 ms | — | 8.69 ms | 10.2 ms | 3.26 | +1 / +1 MB |
| 1024 | **5.23 ms** | 161 ms | — | 30.4 ms | 34 ms | 5.81 | +2 / +2 MB |
| 2048 | **11.2 ms** | 500 ms | — | 71 ms | 73.1 ms | 6.33 | +4 / +3 MB |

Routines: 128: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator q_solve, FLINT fmpq_mat dixon; 256: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator dixon, FLINT fmpq_mat solve; 512: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator dixon, FLINT fmpq_mat dixon; 1024: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator q_solve, FLINT fmpq_mat solve; 2048: the format multimodular Cramer, IFMA, FLINT fmpz_mat, shared denominator q_solve, FLINT fmpq_mat solve.

