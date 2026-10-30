# Integrated exact core vs NumPy

- **date**: 2026-10-25 23:48:07 CDT
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
| E1 regression | 2000 | NumPy float64 | 145 µs | 139 µs | 3% | 1.00x | 1.58e-11 |  | lstsq |
| E1 regression | 2000 | exact core | 1.45 ms | 1.39 ms | 2% | 9.98x | 0 (matches pure Python) |  | solver: FLINT |
| E1 regression | 2000 | pure Python exact | 98.8 ms | 97.9 ms | 1% | 681.80x | 0 (exact) |  |  |
| E2 table pipeline | 5000 | NumPy float64 | 86.5 µs | 82.3 µs | 4% | 1.00x | 5.41e-12 |  | worst over 151 outputs |
| E2 table pipeline | 5000 | exact core | 1.77 ms | 1.71 ms | 3% | 20.45x | 0 (matches pure Python) |  | revenue tier i64, converted tier i128 |
| E2 table pipeline | 5000 | pure Python exact | 44.1 ms | 42.9 ms | 5% | 509.26x | 0 (exact) |  |  |
| E3 complex pipeline | 4000 | NumPy float64 | 47.5 µs | 45.6 µs | 2% | 1.00x | 2.76e-15 |  | fft |
| E3 complex pipeline | 4000 | exact core | 263 µs | 254 µs | 3% | 5.53x | 0 (matches pure Python) |  | bin kinds ['one', 'two', 'one', 'two'] |
| E3 complex pipeline | 4000 | pure Python exact | 37.3 ms | 36.8 ms | 1% | 785.84x | 0 (exact) |  | Cx scalars |
| E4 solve, random rhs | 60 | NumPy float64 | 20.7 µs | 19.6 µs | 3% | 1.00x | 7.04e-13 |  |  |
| E4 solve, random rhs | 60 | exact core | 7.51 ms | 7.37 ms | 3% | 362.53x | 0 (matches FLINT) |  | solver: numeric-symbolic |
| E4 solve, random rhs | 60 | FLINT exact | 2.33 ms | 2.22 ms | 3% | 112.62x | 0 (exact) |  | existing exact baseline |
| E4 solve, planted answer | 60 | NumPy float64 | 20.7 µs | 19.7 µs | 3% | 1.00x | 4.12e-14 |  |  |
| E4 solve, planted answer | 60 | exact core | 530 µs | 502 µs | 6% | 25.66x | 0 (matches FLINT) |  | solver: numeric-symbolic |
| E4 solve, planted answer | 60 | FLINT exact | 595 µs | 579 µs | 2% | 28.81x | 0 (exact) |  | existing exact baseline |
