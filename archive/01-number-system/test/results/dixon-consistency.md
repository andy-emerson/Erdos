# Dixon on BLAS vs FLINT: consistency check

- **date**: 2026-10-25 22:22:59 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v153-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s); openblas 1 thread(s)
- **flint threads**: 1

## Summary

- **cases**: 30 (10 cells x 3 seeds)
- **variant A faster in**: 27 of 30 cases
- **range of per-case ratios**: 0.767 to 1.032

| n | geometric mean A / FLINT | 95% stratified CI | cells where A won all seeds | verdict |
|---|---|---|---|---|
| 200 | 1.005 | 0.995 to 1.014 | 0 of 2 | mixed |
| 400 | 0.882 | 0.880 to 0.884 | 2 of 2 | consistently faster |
| 600 | 0.832 | 0.828 to 0.835 | 2 of 2 | consistently faster |
| 800 | 0.819 | 0.815 to 0.823 | 2 of 2 | consistently faster |
| 1000 | 0.807 | 0.802 to 0.812 | 2 of 2 | consistently faster |

- **equal-weight grid average**: 0.866 (95% stratified CI 0.864 to 0.868)
- **2-place decimals to 1e2**: geometric mean 0.835
- **3-place decimals to 1e3**: geometric mean 0.898

## Cases

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| n=200, 2 places to 1e2 | seed 101 | FLINT | 39.1 ms | 38.7 ms | 3% | 1.00x |  |  |  |
| n=200, 2 places to 1e2 | seed 101 | A: Dixon on BLAS | 39.1 ms | 38.1 ms | 3% | 1.00x | 0 (matches FLINT) |  | 289 steps |
| n=200, 2 places to 1e2 | seed 202 | FLINT | 40.6 ms | 39.3 ms | 5% | 1.00x |  |  |  |
| n=200, 2 places to 1e2 | seed 202 | A: Dixon on BLAS | 39.6 ms | 38.9 ms | 2% | 0.97x | 0 (matches FLINT) |  | 289 steps |
| n=200, 2 places to 1e2 | seed 303 | FLINT | 39.3 ms | 38.9 ms | 1% | 1.00x |  |  |  |
| n=200, 2 places to 1e2 | seed 303 | A: Dixon on BLAS | 39.3 ms | 38.5 ms | 1% | 1.00x | 0 (matches FLINT) |  | 288 steps |
| n=200, 3 places to 1e3 | seed 101 | FLINT | 52.8 ms | 52.3 ms | 1% | 1.00x |  |  |  |
| n=200, 3 places to 1e3 | seed 101 | A: Dixon on BLAS | 54 ms | 53.3 ms | 1% | 1.02x | 0 (matches FLINT) |  | 406 steps |
| n=200, 3 places to 1e3 | seed 202 | FLINT | 54 ms | 53.5 ms | 0% | 1.00x |  |  |  |
| n=200, 3 places to 1e3 | seed 202 | A: Dixon on BLAS | 54 ms | 53.4 ms | 1% | 1.00x | 0 (matches FLINT) |  | 406 steps |
| n=200, 3 places to 1e3 | seed 303 | FLINT | 52.4 ms | 52 ms | 1% | 1.00x |  |  |  |
| n=200, 3 places to 1e3 | seed 303 | A: Dixon on BLAS | 54.1 ms | 52.4 ms | 2% | 1.03x | 0 (matches FLINT) |  | 406 steps |
| n=400, 2 places to 1e2 | seed 101 | FLINT | 290 ms | 287 ms | 2% | 1.00x |  |  |  |
| n=400, 2 places to 1e2 | seed 101 | A: Dixon on BLAS | 247 ms | 245 ms | 3% | 0.85x | 0 (matches FLINT) |  | 607 steps |
| n=400, 2 places to 1e2 | seed 202 | FLINT | 289 ms | 286 ms | 1% | 1.00x |  |  |  |
| n=400, 2 places to 1e2 | seed 202 | A: Dixon on BLAS | 247 ms | 244 ms | 2% | 0.86x | 0 (matches FLINT) |  | 607 steps |
| n=400, 2 places to 1e2 | seed 303 | FLINT | 292 ms | 291 ms | 4% | 1.00x |  |  |  |
| n=400, 2 places to 1e2 | seed 303 | A: Dixon on BLAS | 249 ms | 246 ms | 2% | 0.85x | 0 (matches FLINT) |  | 607 steps |
| n=400, 3 places to 1e3 | seed 101 | FLINT | 391 ms | 390 ms | 0% | 1.00x |  |  |  |
| n=400, 3 places to 1e3 | seed 101 | A: Dixon on BLAS | 356 ms | 353 ms | 1% | 0.91x | 0 (matches FLINT) |  | 847 steps |
| n=400, 3 places to 1e3 | seed 202 | FLINT | 393 ms | 389 ms | 2% | 1.00x |  |  |  |
| n=400, 3 places to 1e3 | seed 202 | A: Dixon on BLAS | 358 ms | 354 ms | 1% | 0.91x | 0 (matches FLINT) |  | 847 steps |
| n=400, 3 places to 1e3 | seed 303 | FLINT | 396 ms | 391 ms | 3% | 1.00x |  |  |  |
| n=400, 3 places to 1e3 | seed 303 | A: Dixon on BLAS | 361 ms | 355 ms | 1% | 0.91x | 0 (matches FLINT) |  | 847 steps |
| n=600, 2 places to 1e2 | seed 101 | FLINT | 993 ms | 956 ms | 1% | 1.00x |  |  |  |
| n=600, 2 places to 1e2 | seed 101 | A: Dixon on BLAS | 789 ms | 783 ms | 1% | 0.79x | 0 (matches FLINT) |  | 939 steps |
| n=600, 2 places to 1e2 | seed 202 | FLINT | 979 ms | 964 ms | 1% | 1.00x |  |  |  |
| n=600, 2 places to 1e2 | seed 202 | A: Dixon on BLAS | 782 ms | 774 ms | 0% | 0.80x | 0 (matches FLINT) |  | 939 steps |
| n=600, 2 places to 1e2 | seed 303 | FLINT | 988 ms | 960 ms | 2% | 1.00x |  |  |  |
| n=600, 2 places to 1e2 | seed 303 | A: Dixon on BLAS | 781 ms | 776 ms | 1% | 0.79x | 0 (matches FLINT) |  | 939 steps |
| n=600, 3 places to 1e3 | seed 101 | FLINT | 1.31 s | 1.3 s | 1% | 1.00x |  |  |  |
| n=600, 3 places to 1e3 | seed 101 | A: Dixon on BLAS | 1.14 s | 1.14 s | 1% | 0.88x | 0 (matches FLINT) |  | 1303 steps |
| n=600, 3 places to 1e3 | seed 202 | FLINT | 1.33 s | 1.3 s | 2% | 1.00x |  |  |  |
| n=600, 3 places to 1e3 | seed 202 | A: Dixon on BLAS | 1.15 s | 1.15 s | 1% | 0.87x | 0 (matches FLINT) |  | 1303 steps |
| n=600, 3 places to 1e3 | seed 303 | FLINT | 1.32 s | 1.31 s | 1% | 1.00x |  |  |  |
| n=600, 3 places to 1e3 | seed 303 | A: Dixon on BLAS | 1.14 s | 1.14 s | 1% | 0.87x | 0 (matches FLINT) |  | 1303 steps |
| n=800, 2 places to 1e2 | seed 101 | FLINT | 2.22 s | 2.19 s | 2% | 1.00x |  |  |  |
| n=800, 2 places to 1e2 | seed 101 | A: Dixon on BLAS | 1.74 s | 1.73 s | 0% | 0.78x | 0 (matches FLINT) |  | 1279 steps |
| n=800, 2 places to 1e2 | seed 202 | FLINT | 2.25 s | 2.21 s | 1% | 1.00x |  |  |  |
| n=800, 2 places to 1e2 | seed 202 | A: Dixon on BLAS | 1.74 s | 1.73 s | 2% | 0.77x | 0 (matches FLINT) |  | 1279 steps |
| n=800, 2 places to 1e2 | seed 303 | FLINT | 2.23 s | 2.21 s | 1% | 1.00x |  |  |  |
| n=800, 2 places to 1e2 | seed 303 | A: Dixon on BLAS | 1.75 s | 1.74 s | 1% | 0.79x | 0 (matches FLINT) |  | 1279 steps |
| n=800, 3 places to 1e3 | seed 101 | FLINT | 2.99 s | 2.98 s | 2% | 1.00x |  |  |  |
| n=800, 3 places to 1e3 | seed 101 | A: Dixon on BLAS | 2.55 s | 2.53 s | 1% | 0.85x | 0 (matches FLINT) |  | 1769 steps |
| n=800, 3 places to 1e3 | seed 202 | FLINT | 2.99 s | 2.95 s | 2% | 1.00x |  |  |  |
| n=800, 3 places to 1e3 | seed 202 | A: Dixon on BLAS | 2.58 s | 2.55 s | 1% | 0.87x | 0 (matches FLINT) |  | 1769 steps |
| n=800, 3 places to 1e3 | seed 303 | FLINT | 3 s | 2.98 s | 0% | 1.00x |  |  |  |
| n=800, 3 places to 1e3 | seed 303 | A: Dixon on BLAS | 2.58 s | 2.56 s | 2% | 0.86x | 0 (matches FLINT) |  | 1769 steps |
| n=1000, 2 places to 1e2 | seed 101 | FLINT | 4.39 s | 4.32 s | 2% | 1.00x |  |  |  |
| n=1000, 2 places to 1e2 | seed 101 | A: Dixon on BLAS | 3.37 s | 3.32 s | 2% | 0.77x | 0 (matches FLINT) |  | 1625 steps |
| n=1000, 2 places to 1e2 | seed 202 | FLINT | 4.31 s | 4.26 s | 1% | 1.00x |  |  |  |
| n=1000, 2 places to 1e2 | seed 202 | A: Dixon on BLAS | 3.34 s | 3.29 s | 2% | 0.78x | 0 (matches FLINT) |  | 1625 steps |
| n=1000, 2 places to 1e2 | seed 303 | FLINT | 4.26 s | 4.25 s | 0% | 1.00x |  |  |  |
| n=1000, 2 places to 1e2 | seed 303 | A: Dixon on BLAS | 3.31 s | 3.3 s | 1% | 0.78x | 0 (matches FLINT) |  | 1625 steps |
| n=1000, 3 places to 1e3 | seed 101 | FLINT | 5.79 s | 5.72 s | 1% | 1.00x |  |  |  |
| n=1000, 3 places to 1e3 | seed 101 | A: Dixon on BLAS | 4.91 s | 4.89 s | 0% | 0.85x | 0 (matches FLINT) |  | 2243 steps |
| n=1000, 3 places to 1e3 | seed 202 | FLINT | 5.85 s | 5.79 s | 1% | 1.00x |  |  |  |
| n=1000, 3 places to 1e3 | seed 202 | A: Dixon on BLAS | 4.86 s | 4.84 s | 1% | 0.83x | 0 (matches FLINT) |  | 2243 steps |
| n=1000, 3 places to 1e3 | seed 303 | FLINT | 5.81 s | 5.73 s | 1% | 1.00x |  |  |  |
| n=1000, 3 places to 1e3 | seed 303 | A: Dixon on BLAS | 4.91 s | 4.87 s | 0% | 0.85x | 0 (matches FLINT) |  | 2243 steps |
