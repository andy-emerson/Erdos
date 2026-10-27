# Tagged complex numbers

- **date**: 2026-10-25 22:47:24 CDT
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
| W1 real data | 100000 | pairs (reference) | 316 ms | 310 ms | 2% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W1 real data | 100000 | tagged | 138 ms | 136 ms | 1% | 0.44x | 0 (matches pairs) | 16 | fixed-width column of width 1; 1.00 parts per value if stored variable-width |
| W1 real data | 100000 | NumPy complex128 | 189 µs | 174 µs | 11% ⚠ | 0.00x | 2.26e-16 | 16 | arithmetic only, on pre-converted arrays; error is max-norm |
| W2 rotation by i | 100000 | pairs (reference) | 611 ms | 606 ms | 3% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W2 rotation by i | 100000 | tagged | 288 ms | 287 ms | 0% | 0.47x | 0 (matches pairs) | 16 | fixed-width column of width 1; 1.00 parts per value if stored variable-width |
| W2 rotation by i | 100000 | NumPy complex128 | 267 µs | 260 µs | 2% | 0.00x | 2.42e-16 | 16 | arithmetic only, on pre-converted arrays; error is max-norm |
| W3 4-point DFT of real data | 100000 | pairs (reference) | 1.7 s | 1.69 s | 5% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W3 4-point DFT of real data | 100000 | tagged | 1.06 s | 1.05 s | 1% | 0.62x | 0 (matches pairs) | 32 | fixed-width column of width 2; 1.50 parts per value if stored variable-width |
| W3 4-point DFT of real data | 100000 | NumPy complex128 | 378 µs | 367 µs | 6% | 0.00x | 1.58e-16 | 16 | arithmetic only, on pre-converted arrays; error is max-norm |
| W4 general complex products | 100000 | pairs (reference) | 410 ms | 396 ms | 3% | 1.00x | 0 (exact) | 32 | 2 words per value |
| W4 general complex products | 100000 | tagged | 472 ms | 462 ms | 2% | 1.15x | 0 (matches pairs) | 32 | fixed-width column of width 2; 2.00 parts per value if stored variable-width |
| W4 general complex products | 100000 | NumPy complex128 | 185 µs | 174 µs | 13% ⚠ | 0.00x | 1.99e-16 | 16 | arithmetic only, on pre-converted arrays; error is max-norm |
