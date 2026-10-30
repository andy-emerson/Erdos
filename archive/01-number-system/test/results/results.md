# Benchmark results

- **date**: 2026-10-25 21:04:42 CDT
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
| 1a sum | 10000000 | NumPy float64 | 3.81 ms | 3.68 ms | 35% ⚠ | 1.00x | 4.93e-16 | 8 |  |
| 1a sum | 10000000 | exact (shared den) | 4.78 ms | 4.68 ms | 1% | 1.26x | 0 (verified) | 8 |  |
| 1a sum | 10000000 | Numba float64 | 3.46 ms | 3.33 ms | 5% | 0.91x | 2.52e-14 | 8 | timing control, reassociated |
| 1b mean | 10000000 | NumPy float64 | 4.81 ms | 3.44 ms | 34% ⚠ | 1.00x | 5.44e-16 | 8 |  |
| 1b mean | 10000000 | exact (shared den) | 4.8 ms | 4.64 ms | 5% | 1.00x | 0 (verified) | 8 |  |
| 1b mean | 10000000 | Numba float64 | 3.3 ms | 3.09 ms | 11% ⚠ | 0.69x | 2.53e-14 | 8 | timing control, reassociated |
| 2a a*b+c | 10000000 | NumPy float64 | 18.9 ms | 17.6 ms | 4% | 1.00x | 2.56e-16 | 8 | error over 20k sampled elements |
| 2a a*b+c | 10000000 | exact (shared den) | 16.2 ms | 15.8 ms | 1% | 0.86x | 0 (verified) | 8 |  |
| 2a a*b+c | 10000000 | Numba float64 | 16.2 ms | 15.8 ms | 2% | 0.86x | 2.56e-16 | 8 | timing control |
| 2b a*b+c, 6 places | 1000000 | NumPy float64 | 2.05 ms | 1.37 ms | 21% ⚠ | 1.00x | 2.60e-16 | 8 | error over 20k sampled elements |
| 2b a*b+c, 6 places | 1000000 | exact (shared den) | 1.68 s | 1.68 s | 1% | 822.52x | 6.54e-28 | 16 | past fast-path limit: all elements rounded by rho; error over 20k sampled elements |
| 2b a*b+c, 6 places | 1000000 | Numba float64 | 1.72 ms | 1.13 ms | 26% ⚠ | 0.84x | 2.60e-16 | 8 | timing control |
| 3 group-by sum | 10000000, G=1000 | NumPy float64 | 19.4 ms | 18.1 ms | 4% | 1.00x | 7.33e-15 | 8 |  |
| 3 group-by sum | 10000000, G=1000 | exact (shared den) | 34.6 ms | 28.1 ms | 8% | 1.79x | 0 (verified) | 8 |  |
| 3 group-by sum | 10000000, G=1000 | Numba float64 | 11 ms | 10.4 ms | 4% | 0.57x | 7.33e-15 | 8 | timing control |
| 4a variance | 10000000 | NumPy float64 | 29.5 ms | 26.1 ms | 6% | 1.00x | 2.27e-17 | 8 |  |
| 4a variance | 10000000 | exact (shared den) | 9.88 ms | 8.98 ms | 8% | 0.34x | 0 (verified) | 8 |  |
| 4a variance | 10000000 | Numba float64 | 8.82 ms | 7.91 ms | 14% ⚠ | 0.30x | 3.04e-15 | 8 | timing control, reassociated |
| 4b variance, offset 1e6 | 10000000 | NumPy float64 | 27.7 ms | 25.8 ms | 11% ⚠ | 1.00x | 2.75e-13 | 8 |  |
| 4b variance, offset 1e6 | 10000000 | exact (shared den) | 7.23 ms | 6.58 ms | 9% | 0.26x | 0 (verified) | 8 |  |
| 4b variance, offset 1e6 | 10000000 | Numba float64 | 8.47 ms | 7.04 ms | 12% ⚠ | 0.31x | 1.21e-12 | 8 | timing control, reassociated |
| 4c variance, offset 1e7 | 1000000 | NumPy float64 | 1.73 ms | 1.51 ms | 10% ⚠ | 1.00x | 9.72e-12 | 8 |  |
| 4c variance, offset 1e7 | 1000000 | exact (shared den) | 70 ms | 69.1 ms | 1% | 40.40x | 0 (verified) | 8 | past fast-path limit: every block falls back to Python |
| 4c variance, offset 1e7 | 1000000 | Numba float64 | 582 µs | 542 µs | 9% | 0.34x | 9.68e-12 | 8 | timing control, reassociated |
| 5 matmul | 500 | NumPy float64 | 3.23 ms | 3.03 ms | 6% | 1.00x | 8.51e-16 | 8 |  |
| 5 matmul | 500 | FLINT exact | 43.7 ms | 43.4 ms | 0% | 13.53x | 0 (verified, Freivalds x3) | 5.9 | stored integer per entry, den shared; conversion 0.03 s excluded |
| 5 matmul | 2000 | NumPy float64 | 173 ms | 167 ms | 4% | 1.00x | 9.10e-16 | 8 |  |
| 5 matmul | 2000 | FLINT exact | 2.53 s | 2.49 s | 2% | 14.60x | 0 (verified, Freivalds x3) | 6 | stored integer per entry, den shared; conversion 0.54 s excluded |
| 6 solve Ax=b | 100 | NumPy float64 | 50.9 µs | 48.3 µs | 5% | 1.00x | 7.28e-15 | 8 |  |
| 6 solve Ax=b | 100 | FLINT exact (Dixon) | 7.45 ms | 7.29 ms | 2% | 146.43x | 0 (verified) | 543.6 | conversion 0.00 s excluded |
| 6 solve Ax=b | 500 | NumPy float64 | 2.76 ms | 2.67 ms | 9% | 1.00x | 1.04e-13 | 8 |  |
| 6 solve Ax=b | 500 | FLINT exact (Dixon) | 723 ms | 719 ms | 0% | 262.48x | 0 (verified) | 2861.8 | conversion 0.02 s excluded |
| 6 solve Ax=b | 1000 | NumPy float64 | 16.1 ms | 14.8 ms | 6% | 1.00x | 1.71e-13 | 8 |  |
| 6 solve Ax=b | 1000 | FLINT exact (Dixon) | 5.58 s | 5.43 s | 2% | 347.25x | 0 (verified) | 5848.4 | conversion 0.06 s excluded |
| 7 determinant | 100 | NumPy float64 (slogdet) | 48.4 µs | 46.5 µs | 2% | 1.00x | 8.08e-14 | 16 | sign + log|det| |
| 7 determinant | 100 | FLINT exact | 4.97 ms | 4.84 ms | 2% | 102.81x | 0 (checked mod 3 primes) | 396 | one value; conversion 0.00 s excluded |
| 7 determinant | 500 | NumPy float64 (slogdet) | 2.61 ms | 2.51 ms | 5% | 1.00x | 1.23e-12 | 16 | sign + log|det| |
| 7 determinant | 500 | FLINT exact | 545 ms | 538 ms | 1% | 208.48x | 0 (checked mod 3 primes) | 2054 | one value; conversion 0.02 s excluded |
| 7 determinant | 1000 | NumPy float64 (slogdet) | 15 ms | 14.5 ms | 3% | 1.00x | 4.23e-12 | 16 | sign + log|det| |
| 7 determinant | 1000 | FLINT exact | 4.77 s | 4.74 s | 1% | 318.76x | 0 (checked mod 3 primes) | 4171 | one value; conversion 0.06 s excluded |
| 8a Euler decay | 1000 | Python float | 17.4 µs | 16.4 µs | 6% | 1.00x | 1.05e-15 | 8 |  |
| 8a Euler decay | 1000 | float in Python class | 399 µs | 385 µs | 3% | 22.97x | 1.05e-15 | 8 | interpreter-overhead control |
| 8a Euler decay | 1000 | Q64 | 13.9 ms | 13.7 ms | 2% | 802.26x | 2.10e-32 | 16 | rounded 1978/2000 ops, inexact 1932; 34.9x vs class control |
| 8a Euler decay | 1000 | Fraction (unbounded) | 31.2 ms | 30.9 ms | 1% | 1798.88x | 0 (exact) | 1660 |  |
| 8b logistic r=3.2 | 1000 | Python float | 21.7 µs | 20.9 µs | 2% | 1.00x | 9.72e-17 | 8 |  |
| 8b logistic r=3.2 | 1000 | float in Python class | 567 µs | 555 µs | 2% | 26.09x | 9.72e-17 | 8 | interpreter-overhead control |
| 8b logistic r=3.2 | 1000 | Q64 | 18.3 ms | 18 ms | 2% | 842.33x | 1.35e-38 | 16 | rounded 1986/3000 ops, inexact 1974; 32.3x vs class control |
| 8c Newton sqrt(2) | 8 | Python float | 278 ns | 271 ns | 2% | 1.00x | 8.87e-17 | 8 |  |
| 8c Newton sqrt(2) | 8 | float in Python class | 4.83 µs | 4.69 µs | 3% | 17.39x | 8.87e-17 | 8 | interpreter-overhead control |
| 8c Newton sqrt(2) | 8 | Q64 | 35.2 µs | 34.3 µs | 2% | 126.91x | 1.06e-38 | 16 | rounded 5/24 ops, inexact 3; 7.3x vs class control |
| 8c Newton sqrt(2) | 8 | Fraction (unbounded) | 10.6 µs | 10.4 µs | 3% | 38.22x | 2.09e-196 | 82 |  |
| 8b logistic r=3.2 | 15 | Fraction (unbounded) |  |  |  |  |  | 38042 | 15 steps only; size doubles each step |
| 9 Hilbert solve | 10 | NumPy float64 | 3.82 µs | 3.63 µs | 3% | 1.00x | 5.85e-04 | 8 |  |
| 9 Hilbert solve | 10 | FLINT exact | 6.68 µs | 6.47 µs | 2% | 1.75x | 0 (verified) | 1 |  |
| 9 Hilbert solve | 20 | NumPy float64 | 5.46 µs | 5.29 µs | 3% | 1.00x | 1.38e+01 | 8 |  |
| 9 Hilbert solve | 20 | FLINT exact | 47.7 µs | 46.2 µs | 2% | 8.73x | 0 (verified) | 1 |  |
| 9 Hilbert solve | 50 | NumPy float64 | 14.8 µs | 14.3 µs | 5% | 1.00x | 3.93e+01 | 8 |  |
| 9 Hilbert solve | 50 | FLINT exact | 620 µs | 588 µs | 5% | 41.99x | 0 (verified) | 1 |  |
| 10 dot in Q(sqrt2) | 10000000 | NumPy float64 | 8.8 ms | 7.06 ms | 9% | 1.00x | 1.19e-14 | 8 |  |
| 10 dot in Q(sqrt2) | 10000000 | exact Q(sqrt2) | 21.2 ms | 20.5 ms | 2% | 2.41x | 0 (verified) | 16 |  |
| 10 dot in Q(sqrt2) | 10000000 | Numba float64 | 8.09 ms | 6.4 ms | 21% ⚠ | 0.92x | 6.71e-15 | 8 | timing control, reassociated |
