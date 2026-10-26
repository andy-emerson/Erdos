# Speed-up variants

- **date**: 2026-10-25 21:56:39 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v153-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s); openblas 1 thread(s)
- **flint threads**: 1

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| S1 solve, random rhs | 100 | NumPy float64 | 50.5 µs | 48.9 µs | 3% | 0.01x | 1.43e-14 | 8 |  |
| S1 solve, random rhs | 100 | FLINT | 8.02 ms | 7.89 ms | 1% | 1.00x | 0 (exact) |  |  |
| S1 solve, random rhs | 100 | A: Dixon on BLAS | 10.6 ms | 10.3 ms | 2% | 1.32x | 0 (matches FLINT) |  | 195 steps |
| S1 solve, random rhs | 100 | A+B: Dixon on BLAS, early stop | 12.8 ms | 12.6 ms | 2% | 1.60x | 0 (matches FLINT) |  | 195 steps |
| S1 solve, random rhs | 100 | C: numeric-symbolic | 15.4 ms | 15.1 ms | 3% | 1.92x | 0 (matches FLINT) |  | 178 steps |
| S1 solve, random rhs | 100 | C+B: numeric-symbolic, early stop | 17 ms | 16.6 ms | 2% | 2.12x | 0 (matches FLINT) |  | 178 steps |
| S1 solve, random rhs | 500 | NumPy float64 | 3 ms | 2.9 ms | 4% | 0.00x | 2.24e-13 | 8 |  |
| S1 solve, random rhs | 500 | FLINT | 780 ms | 775 ms | 0% | 1.00x | 0 (exact) |  |  |
| S1 solve, random rhs | 500 | A: Dixon on BLAS | 703 ms | 702 ms | 0% | 0.90x | 0 (matches FLINT) |  | 1074 steps |
| S1 solve, random rhs | 500 | A+B: Dixon on BLAS, early stop | 742 ms | 739 ms | 1% | 0.95x | 0 (matches FLINT) |  | 1074 steps |
| S1 solve, random rhs | 500 | C: numeric-symbolic | 570 ms | 565 ms | 0% | 0.73x | 0 (matches FLINT) |  | 960 steps |
| S1 solve, random rhs | 500 | C+B: numeric-symbolic, early stop | 569 ms | 566 ms | 1% | 0.73x | 0 (matches FLINT) |  | 887 steps |
| S1 solve, random rhs | 1000 | NumPy float64 | 17.2 ms | 16.3 ms | 14% ⚠ | 0.00x | 1.05e-13 | 8 |  |
| S1 solve, random rhs | 1000 | FLINT | 5.69 s | 5.59 s | 0% | 1.00x | 0 (exact) |  |  |
| S1 solve, random rhs | 1000 | A: Dixon on BLAS | 4.85 s | 4.68 s | 1% | 0.85x | 0 (matches FLINT) |  | 2243 steps |
| S1 solve, random rhs | 1000 | A+B: Dixon on BLAS, early stop | 5.03 s | 4.97 s | 2% | 0.88x | 0 (matches FLINT) |  | 2243 steps |
| S1 solve, random rhs | 1000 | C: numeric-symbolic | 3.88 s | 3.68 s | 6% | 0.68x | 0 (matches FLINT) |  | 1998 steps |
| S1 solve, random rhs | 1000 | C+B: numeric-symbolic, early stop | 3.61 s | 3.45 s | 5% | 0.64x | 0 (matches FLINT) |  | 1809 steps |
| S2 solve, planted answer | 100 | NumPy float64 | 51.5 µs | 49.8 µs | 3% | 0.03x | 5.54e-15 | 8 |  |
| S2 solve, planted answer | 100 | FLINT | 2.05 ms | 1.99 ms | 5% | 1.00x | 0 (exact) |  |  |
| S2 solve, planted answer | 100 | A: Dixon on BLAS | 7.02 ms | 6.77 ms | 4% | 3.43x | 0 (matches FLINT) |  | 195 steps |
| S2 solve, planted answer | 100 | A+B: Dixon on BLAS, early stop | 3.98 ms | 3.8 ms | 4% | 1.95x | 0 (matches FLINT) |  | 3 steps |
| S2 solve, planted answer | 100 | C: numeric-symbolic | 6.23 ms | 5.95 ms | 3% | 3.04x | 0 (matches FLINT) |  | 114 steps |
| S2 solve, planted answer | 100 | C+B: numeric-symbolic, early stop | 948 µs | 905 µs | 3% | 0.46x | 0 (matches FLINT) |  | 2 steps |
| S2 solve, planted answer | 500 | NumPy float64 | 3.01 ms | 2.78 ms | 5% | 0.03x | 1.01e-12 | 8 |  |
| S2 solve, planted answer | 500 | FLINT | 117 ms | 116 ms | 1% | 1.00x | 0 (exact) |  |  |
| S2 solve, planted answer | 500 | A: Dixon on BLAS | 379 ms | 372 ms | 1% | 3.24x | 0 (matches FLINT) |  | 1074 steps |
| S2 solve, planted answer | 500 | A+B: Dixon on BLAS, early stop | 137 ms | 134 ms | 2% | 1.18x | 0 (matches FLINT) |  | 3 steps |
| S2 solve, planted answer | 500 | C: numeric-symbolic | 214 ms | 210 ms | 2% | 1.83x | 0 (matches FLINT) |  | 592 steps |
| S2 solve, planted answer | 500 | C+B: numeric-symbolic, early stop | 21.1 ms | 20.8 ms | 3% | 0.18x | 0 (matches FLINT) |  | 2 steps |
| S2 solve, planted answer | 1000 | NumPy float64 | 16.5 ms | 15.6 ms | 4% | 0.02x | 3.99e-13 | 8 |  |
| S2 solve, planted answer | 1000 | FLINT | 765 ms | 754 ms | 0% | 1.00x | 0 (exact) |  |  |
| S2 solve, planted answer | 1000 | A: Dixon on BLAS | 2.47 s | 2.33 s | 4% | 3.22x | 0 (matches FLINT) |  | 2243 steps |
| S2 solve, planted answer | 1000 | A+B: Dixon on BLAS, early stop | 725 ms | 706 ms | 1% | 0.95x | 0 (matches FLINT) |  | 3 steps |
| S2 solve, planted answer | 1000 | C: numeric-symbolic | 1.57 s | 1.4 s | 12% ⚠ | 2.05x | 0 (matches FLINT) |  | 1208 steps |
| S2 solve, planted answer | 1000 | C+B: numeric-symbolic, early stop | 91.2 ms | 88.2 ms | 1% | 0.12x | 0 (matches FLINT) |  | 2 steps |
| S3 determinant | 100 | FLINT | 5.65 ms | 5.46 ms | 2% | 1.00x | 0 (exact) |  |  |
| S3 determinant | 100 | A: via Dixon on BLAS | 16.3 ms | 16 ms | 1% | 2.89x | 0 (matches FLINT) |  |  |
| S3 determinant | 100 | C: via numeric-symbolic | 21 ms | 20.6 ms | 2% | 3.72x | 0 (matches FLINT) |  |  |
| S3 determinant | 500 | FLINT | 543 ms | 539 ms | 0% | 1.00x | 0 (exact) |  |  |
| S3 determinant | 500 | A: via Dixon on BLAS | 1.04 s | 1.03 s | 2% | 1.91x | 0 (matches FLINT) |  |  |
| S3 determinant | 500 | C: via numeric-symbolic | 867 ms | 864 ms | 0% | 1.60x | 0 (matches FLINT) |  |  |
| S3 determinant | 1000 | FLINT | 4.92 s | 4.77 s | 5% | 1.00x | 0 (exact) |  |  |
| S3 determinant | 1000 | A: via Dixon on BLAS | 8.34 s | 8.24 s | 1% | 1.69x | 0 (matches FLINT) |  |  |
| S3 determinant | 1000 | C: via numeric-symbolic | 6.81 s | 6.71 s | 3% | 1.38x | 0 (matches FLINT) |  |  |
| S4a a*b+c, 6 places | 1000000 | NumPy float64 | 2.03 ms | 1.81 ms | 3% | 1.00x | 2.65e-16 | 8 |  |
| S4a a*b+c, 6 places | 1000000 | Python fallback (ch. 4) | 1.72 s | 1.66 s | 2% | 845.33x | 1.68e-28 | 16 | rounded by rho |
| S4a a*b+c, 6 places | 1000000 | D: 128-bit fallback | 3.62 ms | 3.33 ms | 4% | 1.78x | 0 (verified, all elements) | 16 | exact, 128-bit numerators |
| S4b variance, offset 1e7 | 1000000 | NumPy float64 | 2.67 ms | 2.6 ms | 3% | 1.00x | 9.72e-12 | 8 |  |
| S4b variance, offset 1e7 | 1000000 | Python fallback (ch. 4) | 71.1 ms | 70.1 ms | 1% | 26.66x | 0 (verified) | 8 |  |
| S4b variance, offset 1e7 | 1000000 | D: 128-bit fallback | 5.33 ms | 4.19 ms | 14% ⚠ | 2.00x | 0 (verified) | 8 |  |
