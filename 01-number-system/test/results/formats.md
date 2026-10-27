# Number formats

- **date**: 2026-10-25 22:09:10 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v153-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| P1 rounding precision | |x| ~ 1e-12 | float64 | 363 µs | 351 µs | 2% | 1.00x | 3.95e-17 | 8 | max 1.06e-16 |
| P1 rounding precision | |x| ~ 1e-12 | Q64 (63/63) | 13.5 ms | 13.3 ms | 1% | 37.07x | 1.96e-27 | 16 | max 3.12e-24 |
| P1 rounding precision | |x| ~ 1e-12 | Q64u (63/64) | 13.7 ms | 13.6 ms | 1% | 37.82x | 5.03e-28 | 16 | max 2.25e-25 |
| P1 rounding precision | |x| ~ 1e-12 | FS120 floating-slash | 17.5 ms | 17.4 ms | 1% | 48.22x | 9.55e-37 | 16 | max 4.61e-34 |
| P1 rounding precision | |x| ~ 1e-6 | float64 | 385 µs | 376 µs | 3% | 1.00x | 3.73e-17 | 8 | max 1.09e-16 |
| P1 rounding precision | |x| ~ 1e-6 | Q64 (63/63) | 18.9 ms | 18.6 ms | 1% | 49.22x | 1.97e-33 | 16 | max 1.32e-30 |
| P1 rounding precision | |x| ~ 1e-6 | Q64u (63/64) | 19.1 ms | 18.8 ms | 1% | 49.64x | 5.15e-34 | 16 | max 7.99e-31 |
| P1 rounding precision | |x| ~ 1e-6 | FS120 floating-slash | 20.4 ms | 20.1 ms | 1% | 53.03x | 9.88e-37 | 16 | max 1.42e-33 |
| P1 rounding precision | |x| ~ 1e-1 | float64 | 364 µs | 351 µs | 3% | 1.00x | 3.71e-17 | 8 | max 1.09e-16 |
| P1 rounding precision | |x| ~ 1e-1 | Q64 (63/63) | 23.9 ms | 23.1 ms | 3% | 65.76x | 2.05e-38 | 16 | max 1.37e-35 |
| P1 rounding precision | |x| ~ 1e-1 | Q64u (63/64) | 23.9 ms | 23.3 ms | 4% | 65.64x | 8.22e-39 | 16 | max 5.75e-36 |
| P1 rounding precision | |x| ~ 1e-1 | FS120 floating-slash | 22.6 ms | 21.9 ms | 2% | 62.12x | 9.23e-37 | 16 | max 9.89e-34 |
| P1 rounding precision | |x| ~ 1e0 | float64 | 366 µs | 352 µs | 4% | 1.00x | 3.94e-17 | 8 | max 1.07e-16 |
| P1 rounding precision | |x| ~ 1e0 | Q64 (63/63) | 23.6 ms | 22.7 ms | 11% ⚠ | 64.47x | 4.93e-38 | 16 | max 2.35e-35 |
| P1 rounding precision | |x| ~ 1e0 | Q64u (63/64) | 23.6 ms | 22.9 ms | 7% | 64.32x | 4.93e-38 | 16 | max 2.35e-35 |
| P1 rounding precision | |x| ~ 1e0 | FS120 floating-slash | 22.4 ms | 22.2 ms | 2% | 61.27x | 9.37e-37 | 16 | max 3.40e-33 |
| P1 rounding precision | |x| ~ 1e6 | float64 | 365 µs | 344 µs | 1% | 1.00x | 3.98e-17 | 8 | max 1.09e-16 |
| P1 rounding precision | |x| ~ 1e6 | Q64 (63/63) | 17.7 ms | 17.4 ms | 2% | 48.55x | 4.72e-32 | 16 | max 1.42e-29 |
| P1 rounding precision | |x| ~ 1e6 | Q64u (63/64) | 17.8 ms | 17.4 ms | 2% | 48.73x | 4.72e-32 | 16 | max 1.42e-29 |
| P1 rounding precision | |x| ~ 1e6 | FS120 floating-slash | 19.8 ms | 19.5 ms | 1% | 54.33x | 9.41e-37 | 16 | max 4.25e-34 |
| P1 rounding precision | |x| ~ 1e12 | float64 | 370 µs | 358 µs | 4% | 1.00x | 3.78e-17 | 8 | max 1.09e-16 |
| P1 rounding precision | |x| ~ 1e12 | Q64 (63/63) | 12.2 ms | 12.1 ms | 1% | 33.00x | 5.01e-26 | 16 | max 6.38e-23 |
| P1 rounding precision | |x| ~ 1e12 | Q64u (63/64) | 12.2 ms | 12 ms | 2% | 33.04x | 5.01e-26 | 16 | max 6.38e-23 |
| P1 rounding precision | |x| ~ 1e12 | FS120 floating-slash | 17.3 ms | 16.8 ms | 2% | 46.90x | 9.37e-37 | 16 | max 5.93e-34 |
| P1 rounding precision | |x| ~ 1e18 | float64 | 358 µs | 347 µs | 2% | 1.00x | 3.73e-17 | 8 | max 1.07e-16 |
| P1 rounding precision | |x| ~ 1e18 | Q64 (63/63) | 5.76 ms | 5.57 ms | 3% | 16.08x | 2.78e-20 | 16 | max 1.05e-19; 176/2000 out of range |
| P1 rounding precision | |x| ~ 1e18 | Q64u (63/64) | 5.82 ms | 5.57 ms | 3% | 16.27x | 2.78e-20 | 16 | max 1.05e-19; 176/2000 out of range |
| P1 rounding precision | |x| ~ 1e18 | FS120 floating-slash | 14.3 ms | 14 ms | 2% | 40.09x | 9.31e-37 | 16 | max 1.88e-33 |
| P2a Euler decay |  | Python float | 17.2 µs | 16.3 µs | 3% | 1.00x | 1.05e-15 | 8 |  |
| P2a Euler decay |  | Q64 (63/63) | 18.4 ms | 18.1 ms | 2% | 1071.12x | 2.10e-32 | 16 |  |
| P2a Euler decay |  | Q64u (63/64) | 18.5 ms | 18 ms | 1% | 1077.00x | 1.06e-32 | 16 |  |
| P2a Euler decay |  | FS120 floating-slash | 17 ms | 16.7 ms | 1% | 988.67x | 1.61e-34 | 16 |  |
| P2b logistic r=3.2 |  | Python float | 21.7 µs | 20.6 µs | 4% | 1.00x | 9.72e-17 | 8 |  |
| P2b logistic r=3.2 |  | Q64 (63/63) | 22.8 ms | 22.4 ms | 1% | 1047.48x | 1.35e-38 | 16 |  |
| P2b logistic r=3.2 |  | Q64u (63/64) | 22.7 ms | 22.4 ms | 2% | 1047.09x | 1.35e-38 | 16 |  |
| P2b logistic r=3.2 |  | FS120 floating-slash | 12.2 ms | 11.9 ms | 3% | 559.72x | 1.48e-36 | 16 |  |
| P2c Newton sqrt(2) |  | Python float | 277 ns | 263 ns | 7% | 1.00x | 8.87e-17 | 8 |  |
| P2c Newton sqrt(2) |  | Q64 (63/63) | 43.3 µs | 42.1 µs | 2% | 155.98x | 1.06e-38 | 16 |  |
| P2c Newton sqrt(2) |  | Q64u (63/64) | 42.6 µs | 41.1 µs | 3% | 153.74x | 1.06e-38 | 16 |  |
| P2c Newton sqrt(2) |  | FS120 floating-slash | 72.9 µs | 70.4 µs | 1% | 263.06x | 2.09e-36 | 16 |  |
