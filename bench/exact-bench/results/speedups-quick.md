# Speed-up variants

- **date**: 2026-10-25 21:32:37 CDT
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
| S1 solve, random rhs | 50 | NumPy float64 | 15.3 µs | 14.6 µs | 6% | 0.01x | 1.76e-15 | 8 |  |
| S1 solve, random rhs | 50 | FLINT | 1.16 ms | 1.11 ms | 3% | 1.00x | 0 (exact) |  |  |
| S1 solve, random rhs | 50 | A: Dixon on BLAS | 2.42 ms | 2.33 ms | 4% | 2.09x | 0 (matches FLINT) |  | 94 steps |
| S1 solve, random rhs | 50 | A+B: Dixon on BLAS, early stop | 2.84 ms | 2.74 ms | 3% | 2.44x | 0 (matches FLINT) |  | 94 steps |
| S1 solve, random rhs | 50 | C: numeric-symbolic | 4.87 ms | 4.72 ms | 5% | 4.19x | 0 (matches FLINT) |  | 87 steps |
| S1 solve, random rhs | 50 | C+B: numeric-symbolic, early stop | 5.41 ms | 5.25 ms | 2% | 4.65x | 0 (matches FLINT) |  | 87 steps |
| S1 solve, random rhs | 100 | NumPy float64 | 50.7 µs | 49 µs | 3% | 0.01x | 1.43e-14 | 8 |  |
| S1 solve, random rhs | 100 | FLINT | 7.49 ms | 7.32 ms | 1% | 1.00x | 0 (exact) |  |  |
| S1 solve, random rhs | 100 | A: Dixon on BLAS | 9.73 ms | 9.37 ms | 3% | 1.30x | 0 (matches FLINT) |  | 195 steps |
| S1 solve, random rhs | 100 | A+B: Dixon on BLAS, early stop | 11.4 ms | 11.1 ms | 2% | 1.52x | 0 (matches FLINT) |  | 195 steps |
| S1 solve, random rhs | 100 | C: numeric-symbolic | 15.7 ms | 15.2 ms | 2% | 2.09x | 0 (matches FLINT) |  | 178 steps |
| S1 solve, random rhs | 100 | C+B: numeric-symbolic, early stop | 17 ms | 16.5 ms | 2% | 2.27x | 0 (matches FLINT) |  | 178 steps |
| S2 solve, planted answer | 50 | NumPy float64 | 14.7 µs | 14.4 µs | 2% | 0.06x | 4.22e-15 | 8 |  |
| S2 solve, planted answer | 50 | FLINT | 242 µs | 236 µs | 2% | 1.00x | 0 (exact) |  |  |
| S2 solve, planted answer | 50 | A: Dixon on BLAS | 1.65 ms | 1.56 ms | 4% | 6.79x | 0 (matches FLINT) |  | 94 steps |
| S2 solve, planted answer | 50 | A+B: Dixon on BLAS, early stop | 1.08 ms | 1.05 ms | 4% | 4.44x | 0 (matches FLINT) |  | 16 steps |
| S2 solve, planted answer | 50 | C: numeric-symbolic | 2.42 ms | 2.32 ms | 10% | 9.99x | 0 (matches FLINT) |  | 56 steps |
| S2 solve, planted answer | 50 | C+B: numeric-symbolic, early stop | 337 µs | 322 µs | 4% | 1.39x | 0 (matches FLINT) |  | 2 steps |
| S2 solve, planted answer | 100 | NumPy float64 | 51 µs | 48.9 µs | 1% | 0.04x | 5.54e-15 | 8 |  |
| S2 solve, planted answer | 100 | FLINT | 1.35 ms | 1.31 ms | 2% | 1.00x | 0 (exact) |  |  |
| S2 solve, planted answer | 100 | A: Dixon on BLAS | 5.89 ms | 5.75 ms | 2% | 4.37x | 0 (matches FLINT) |  | 195 steps |
| S2 solve, planted answer | 100 | A+B: Dixon on BLAS, early stop | 3.91 ms | 3.79 ms | 4% | 2.90x | 0 (matches FLINT) |  | 16 steps |
| S2 solve, planted answer | 100 | C: numeric-symbolic | 6.02 ms | 5.86 ms | 3% | 4.46x | 0 (matches FLINT) |  | 114 steps |
| S2 solve, planted answer | 100 | C+B: numeric-symbolic, early stop | 903 µs | 847 µs | 3% | 0.67x | 0 (matches FLINT) |  | 2 steps |
| S3 determinant | 50 | FLINT | 1.11 ms | 1.08 ms | 2% | 1.00x | 0 (exact) |  |  |
| S3 determinant | 50 | A: via Dixon on BLAS | 2.87 ms | 2.76 ms | 2% | 2.60x | 0 (matches FLINT) |  |  |
| S3 determinant | 50 | C: via numeric-symbolic | 5.33 ms | 5.18 ms | 3% | 4.83x | 0 (matches FLINT) |  |  |
| S3 determinant | 100 | FLINT | 4.97 ms | 4.86 ms | 2% | 1.00x | 0 (exact) |  |  |
| S3 determinant | 100 | A: via Dixon on BLAS | 12.3 ms | 12 ms | 2% | 2.47x | 0 (matches FLINT) |  |  |
| S3 determinant | 100 | C: via numeric-symbolic | 18.2 ms | 17.8 ms | 2% | 3.65x | 0 (matches FLINT) |  |  |
| S4a a*b+c, 6 places | 100000 | NumPy float64 | 102 µs | 99.2 µs | 3% | 1.00x | 2.70e-16 | 8 |  |
| S4a a*b+c, 6 places | 100000 | Python fallback (ch. 4) | 165 ms | 163 ms | 2% | 1612.58x | 4.15e-28 | 16 | rounded by rho |
| S4a a*b+c, 6 places | 100000 | D: 128-bit fallback | 277 µs | 264 µs | 3% | 2.71x | 0 (verified, all elements) | 16 | exact, 128-bit numerators |
| S4b variance, offset 1e7 | 100000 | NumPy float64 | 79.4 µs | 75.6 µs | 4% | 1.00x | 1.33e-11 | 8 |  |
| S4b variance, offset 1e7 | 100000 | Python fallback (ch. 4) | 7.08 ms | 6.94 ms | 2% | 89.18x | 0 (verified) | 8 |  |
| S4b variance, offset 1e7 | 100000 | D: 128-bit fallback | 303 µs | 294 µs | 2% | 3.82x | 0 (verified) | 8 |  |
