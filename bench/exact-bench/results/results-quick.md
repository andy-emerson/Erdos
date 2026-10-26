# Benchmark results

- **date**: 2026-10-25 21:03:16 CDT
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
| 1a sum | 100000 | NumPy float64 | 18.9 µs | 18.3 µs | 3% | 1.00x | 9.74e-17 | 8 |  |
| 1a sum | 100000 | exact (shared den) | 40.9 µs | 39.5 µs | 2% | 2.16x | 0 (verified) | 8 |  |
| 1a sum | 100000 | Numba float64 | 53.2 µs | 52.4 µs | 2% | 2.81x | 1.59e-14 | 8 | timing control, reassociated |
| 1b mean | 100000 | NumPy float64 | 19.8 µs | 19.3 µs | 2% | 1.00x | 4.63e-17 | 8 |  |
| 1b mean | 100000 | exact (shared den) | 41 µs | 39.5 µs | 2% | 2.07x | 0 (verified) | 8 |  |
| 1b mean | 100000 | Numba float64 | 53 µs | 51.7 µs | 1% | 2.68x | 1.59e-14 | 8 | timing control, reassociated |
| 2a a*b+c | 100000 | NumPy float64 | 100 µs | 96.1 µs | 3% | 1.00x | 2.61e-16 | 8 | error over 20k sampled elements |
| 2a a*b+c | 100000 | exact (shared den) | 110 µs | 108 µs | 2% | 1.10x | 0 (verified) | 8 |  |
| 2a a*b+c | 100000 | Numba float64 | 98.4 µs | 90.5 µs | 10% ⚠ | 0.98x | 2.61e-16 | 8 | timing control |
| 2b a*b+c, 6 places | 10000 | NumPy float64 | 4.26 µs | 4.13 µs | 4% | 1.00x | 2.59e-16 | 8 | error over 20k sampled elements |
| 2b a*b+c, 6 places | 10000 | exact (shared den) | 16.3 ms | 16.1 ms | 1% | 3822.72x | 4.76e-29 | 16 | past fast-path limit: all elements rounded by rho; error over 20k sampled elements |
| 2b a*b+c, 6 places | 10000 | Numba float64 | 2.99 µs | 2.91 µs | 2% | 0.70x | 2.59e-16 | 8 | timing control |
| 3 group-by sum | 100000, G=1000 | NumPy float64 | 99.5 µs | 96.5 µs | 3% | 1.00x | 5.45e-16 | 8 |  |
| 3 group-by sum | 100000, G=1000 | exact (shared den) | 108 µs | 105 µs | 2% | 1.08x | 0 (verified) | 8 |  |
| 3 group-by sum | 100000, G=1000 | Numba float64 | 48 µs | 46.3 µs | 3% | 0.48x | 5.45e-16 | 8 | timing control |
| 4a variance | 100000 | NumPy float64 | 86 µs | 81.7 µs | 4% | 1.00x | 2.83e-17 | 8 |  |
| 4a variance | 100000 | exact (shared den) | 60.6 µs | 59 µs | 3% | 0.70x | 0 (verified) | 8 |  |
| 4a variance | 100000 | Numba float64 | 186 µs | 182 µs | 2% | 2.17x | 1.64e-14 | 8 | timing control, reassociated |
| 4b variance, offset 1e6 | 100000 | NumPy float64 | 85.7 µs | 81.4 µs | 5% | 1.00x | 5.00e-13 | 8 |  |
| 4b variance, offset 1e6 | 100000 | exact (shared den) | 61 µs | 60 µs | 2% | 0.71x | 0 (verified) | 8 |  |
| 4b variance, offset 1e6 | 100000 | Numba float64 | 187 µs | 181 µs | 4% | 2.18x | 4.53e-13 | 8 | timing control, reassociated |
| 4c variance, offset 1e7 | 10000 | NumPy float64 | 11.7 µs | 11.2 µs | 2% | 1.00x | 2.38e-12 | 8 |  |
| 4c variance, offset 1e7 | 10000 | exact (shared den) | 762 µs | 732 µs | 3% | 65.32x | 0 (verified) | 8 | past fast-path limit: every block falls back to Python |
| 4c variance, offset 1e7 | 10000 | Numba float64 | 18.7 µs | 18.5 µs | 2% | 1.60x | 2.37e-12 | 8 | timing control, reassociated |
| 5 matmul | 50 | NumPy float64 | 3.84 µs | 3.62 µs | 4% | 1.00x | 3.84e-16 | 8 |  |
| 5 matmul | 50 | FLINT exact | 66.6 µs | 61.7 µs | 5% | 17.36x | 0 (verified, Freivalds x3) | 5.8 | stored integer per entry, den shared; conversion 0.00 s excluded |
| 5 matmul | 100 | NumPy float64 | 22.3 µs | 20.4 µs | 12% ⚠ | 1.00x | 5.84e-16 | 8 |  |
| 5 matmul | 100 | FLINT exact | 420 µs | 398 µs | 2% | 18.83x | 0 (verified, Freivalds x3) | 5.9 | stored integer per entry, den shared; conversion 0.00 s excluded |
| 6 solve Ax=b | 20 | NumPy float64 | 5.99 µs | 5.52 µs | 7% | 1.00x | 7.13e-16 | 8 |  |
| 6 solve Ax=b | 20 | FLINT exact (Dixon) | 152 µs | 146 µs | 3% | 25.37x | 0 (verified) | 103.8 | conversion 0.00 s excluded |
| 6 solve Ax=b | 50 | NumPy float64 | 15.3 µs | 14.5 µs | 3% | 1.00x | 3.55e-15 | 8 |  |
| 6 solve Ax=b | 50 | FLINT exact (Dixon) | 1.17 ms | 1.12 ms | 7% | 76.53x | 0 (verified) | 265.8 | conversion 0.00 s excluded |
| 7 determinant | 20 | NumPy float64 (slogdet) | 4.82 µs | 4.47 µs | 6% | 1.00x | 3.07e-14 | 16 | sign + log|det| |
| 7 determinant | 20 | FLINT exact | 113 µs | 110 µs | 2% | 23.46x | 0 (checked mod 3 primes) | 77 | one value; conversion 0.00 s excluded |
| 7 determinant | 50 | NumPy float64 (slogdet) | 13.4 µs | 13 µs | 2% | 1.00x | 1.03e-13 | 16 | sign + log|det| |
| 7 determinant | 50 | FLINT exact | 1.12 ms | 1.08 ms | 2% | 83.69x | 0 (checked mod 3 primes) | 195 | one value; conversion 0.00 s excluded |
| 8a Euler decay | 1000 | Python float | 17.1 µs | 16.2 µs | 4% | 1.00x | 1.05e-15 | 8 |  |
| 8a Euler decay | 1000 | float in Python class | 355 µs | 346 µs | 2% | 20.68x | 1.05e-15 | 8 | interpreter-overhead control |
| 8a Euler decay | 1000 | Q64 | 14.2 ms | 14 ms | 1% | 826.26x | 2.10e-32 | 16 | rounded 1978/2000 ops, inexact 1932; 40.0x vs class control |
| 8a Euler decay | 1000 | Fraction (unbounded) | 31.2 ms | 30.9 ms | 2% | 1817.48x | 0 (exact) | 1660 |  |
| 8b logistic r=3.2 | 1000 | Python float | 21.2 µs | 19.9 µs | 5% | 1.00x | 9.72e-17 | 8 |  |
| 8b logistic r=3.2 | 1000 | float in Python class | 542 µs | 519 µs | 2% | 25.57x | 9.72e-17 | 8 | interpreter-overhead control |
| 8b logistic r=3.2 | 1000 | Q64 | 18.3 ms | 18.1 ms | 1% | 865.60x | 1.35e-38 | 16 | rounded 1986/3000 ops, inexact 1974; 33.8x vs class control |
| 8c Newton sqrt(2) | 8 | Python float | 279 ns | 265 ns | 5% | 1.00x | 8.87e-17 | 8 |  |
| 8c Newton sqrt(2) | 8 | float in Python class | 4.62 µs | 4.49 µs | 2% | 16.55x | 8.87e-17 | 8 | interpreter-overhead control |
| 8c Newton sqrt(2) | 8 | Q64 | 36 µs | 34.5 µs | 3% | 128.81x | 1.06e-38 | 16 | rounded 5/24 ops, inexact 3; 7.8x vs class control |
| 8c Newton sqrt(2) | 8 | Fraction (unbounded) | 10.6 µs | 10.2 µs | 1% | 37.89x | 2.09e-196 | 82 |  |
| 8b logistic r=3.2 | 15 | Fraction (unbounded) |  |  |  |  |  | 38042 | 15 steps only; size doubles each step |
| 9 Hilbert solve | 10 | NumPy float64 | 3.71 µs | 3.62 µs | 4% | 1.00x | 5.85e-04 | 8 |  |
| 9 Hilbert solve | 10 | FLINT exact | 6.65 µs | 6.5 µs | 2% | 1.79x | 0 (verified) | 1 |  |
| 9 Hilbert solve | 20 | NumPy float64 | 5.5 µs | 5.2 µs | 7% | 1.00x | 1.38e+01 | 8 |  |
| 9 Hilbert solve | 20 | FLINT exact | 49 µs | 46.8 µs | 4% | 8.92x | 0 (verified) | 1 |  |
| 9 Hilbert solve | 50 | NumPy float64 | 14.7 µs | 14.1 µs | 5% | 1.00x | 3.93e+01 | 8 |  |
| 9 Hilbert solve | 50 | FLINT exact | 582 µs | 550 µs | 6% | 39.47x | 0 (verified) | 1 |  |
| 10 dot in Q(sqrt2) | 100000 | NumPy float64 | 12.8 µs | 12.4 µs | 3% | 1.00x | 2.63e-15 | 8 |  |
| 10 dot in Q(sqrt2) | 100000 | exact Q(sqrt2) | 197 µs | 186 µs | 3% | 15.39x | 0 (verified) | 16 |  |
| 10 dot in Q(sqrt2) | 100000 | Numba float64 | 14.4 µs | 14.2 µs | 2% | 1.13x | 1.18e-15 | 8 | timing control, reassociated |
