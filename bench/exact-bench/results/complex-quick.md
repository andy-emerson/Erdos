# Tagged complex numbers

- **date**: 2026-10-25 22:41:45 CDT
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
| W1 real data | 4000 | pairs (reference) | 11.5 ms | 11.3 ms | 1% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W1 real data | 4000 | tagged | 5.05 ms | 4.81 ms | 2% | 0.44x | 0 (matches pairs) | 16 | fixed-width column of width 1; 1.00 parts per value if stored variable-width |
| W1 real data | 4000 | NumPy complex128 | 1.87 ms | 1.82 ms | 2% | 0.16x | 1.93e-16 | 16 | includes conversion from exact values |
| W2 rotation by i | 4000 | pairs (reference) | 23.1 ms | 22.6 ms | 3% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W2 rotation by i | 4000 | tagged | 11.3 ms | 10.9 ms | 4% | 0.49x | 0 (matches pairs) | 16 | fixed-width column of width 1; 1.00 parts per value if stored variable-width |
| W2 rotation by i | 4000 | NumPy complex128 | 1.93 ms | 1.88 ms | 2% | 0.08x | 2.01e-16 | 16 | includes conversion from exact values |
| W3 4-point DFT of real data | 4000 | pairs (reference) | 67.6 ms | 66.8 ms | 1% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W3 4-point DFT of real data | 4000 | tagged | 42.4 ms | 41.8 ms | 1% | 0.63x | 0 (matches pairs) | 32 | fixed-width column of width 2; 1.50 parts per value if stored variable-width |
| W3 4-point DFT of real data | 4000 | NumPy complex128 | 966 µs | 940 µs | 3% | 0.01x | 1.54e-16 | 16 | includes conversion from exact values |
| W4 general complex products | 4000 | pairs (reference) | 14.8 ms | 14.5 ms | 2% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W4 general complex products | 4000 | tagged | 17.3 ms | 16.9 ms | 2% | 1.16x | 0 (matches pairs) | 32 | fixed-width column of width 2; 2.00 parts per value if stored variable-width |
| W4 general complex products | 4000 | NumPy complex128 | 2.01 ms | 1.95 ms | 3% | 0.14x | 1.56e-16 | 16 | includes conversion from exact values |
