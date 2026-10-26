# Integrated exact core vs NumPy

- **date**: 2026-10-25 23:48:36 CDT
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
| E1 regression | 20000 | NumPy float64 | 1.59 ms | 1.42 ms | 1% | 1.00x | 4.56e-12 |  | lstsq |
| E1 regression | 20000 | exact core | 9.05 ms | 8.68 ms | 4% | 5.71x | 0 (matches pure Python) |  | solver: FLINT |
| E1 regression | 20000 | pure Python exact | 978 ms | 976 ms | 0% | 616.61x | 0 (exact) |  |  |
| E2 table pipeline | 100000 | NumPy float64 | 2.03 ms | 2 ms | 7% | 1.00x | 1.23e-12 |  | worst over 151 outputs |
| E2 table pipeline | 100000 | exact core | 33.4 ms | 33.1 ms | 2% | 16.50x | 0 (matches pure Python) |  | revenue tier i64, converted tier i128 |
| E2 table pipeline | 100000 | pure Python exact | 890 ms | 848 ms | 1% | 439.06x | 0 (exact) |  |  |
| E3 complex pipeline | 100000 | NumPy float64 | 1.29 ms | 1.27 ms | 1% | 1.00x | 4.59e-15 |  | fft |
| E3 complex pipeline | 100000 | exact core | 2.36 ms | 2.3 ms | 11% ⚠ | 1.83x | 0 (matches pure Python) |  | bin kinds ['one', 'two', 'one', 'two'] |
| E3 complex pipeline | 100000 | pure Python exact | 931 ms | 922 ms | 2% | 720.35x | 0 (exact) |  | Cx scalars |
| E4 solve, random rhs | 500 | NumPy float64 | 3.23 ms | 3.17 ms | 5% | 1.00x | 4.78e-12 |  |  |
| E4 solve, random rhs | 500 | exact core | 554 ms | 539 ms | 4% | 171.28x | 0 (matches FLINT) |  | solver: numeric-symbolic |
| E4 solve, random rhs | 500 | FLINT exact | 764 ms | 751 ms | 3% | 236.26x | 0 (exact) |  | existing exact baseline |
| E4 solve, planted answer | 500 | NumPy float64 | 2.88 ms | 2.75 ms | 4% | 1.00x | 6.35e-10 |  |  |
| E4 solve, planted answer | 500 | exact core | 20.9 ms | 20.4 ms | 2% | 7.25x | 0 (matches FLINT) |  | solver: numeric-symbolic |
| E4 solve, planted answer | 500 | FLINT exact | 115 ms | 115 ms | 1% | 40.02x | 0 (exact) |  | existing exact baseline |
