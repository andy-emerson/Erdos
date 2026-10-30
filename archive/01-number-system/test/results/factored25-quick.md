# Factoring out 2s and 5s

- **date**: 2026-10-25 23:20:35 CDT
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
| F1 unit conversions | 500 | Fraction | 1.03 ms | 973 µs | 4% | 1.00x | 0 (exact) |  | result bits mean 72, max 76 |
| F1 unit conversions | 500 | 2s and 5s factored | 1.13 ms | 1.09 ms | 4% | 1.10x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1 unit conversions | 500 | 2s only | 1.06 ms | 1.02 ms | 2% | 1.02x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1 unit conversions | 500 | 2s and 5s, lazy cleanup | 1.15 ms | 1.11 ms | 2% | 1.12x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1 unit conversions | 500 | 2s only, fast add | 1.06 ms | 1.02 ms | 2% | 1.02x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1 unit conversions | 500 | 2s only, fast add, lazy gcd | 1.07 ms | 1.02 ms | 3% | 1.04x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 500 | Fraction | 399 µs | 383 µs | 4% | 1.00x | 0 (exact) |  | result bits mean 72, max 76 |
| F1b unit conversion folded into one constant | 500 | 2s and 5s factored | 528 µs | 500 µs | 3% | 1.32x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1b unit conversion folded into one constant | 500 | 2s only | 461 µs | 450 µs | 2% | 1.16x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 500 | 2s and 5s, lazy cleanup | 526 µs | 499 µs | 3% | 1.32x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1b unit conversion folded into one constant | 500 | 2s only, fast add | 464 µs | 445 µs | 2% | 1.16x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 500 | 2s only, fast add, lazy gcd | 465 µs | 453 µs | 1% | 1.16x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F2a 40 halvings, general ops | 500 | Fraction | 7.63 ms | 7.29 ms | 6% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2a 40 halvings, general ops | 500 | 2s and 5s factored | 8.27 ms | 7.95 ms | 2% | 1.08x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2a 40 halvings, general ops | 500 | 2s only | 7.63 ms | 7.32 ms | 3% | 1.00x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2a 40 halvings, general ops | 500 | 2s and 5s, lazy cleanup | 8.22 ms | 7.94 ms | 2% | 1.08x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2a 40 halvings, general ops | 500 | 2s only, fast add | 7.71 ms | 7.45 ms | 3% | 1.01x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2a 40 halvings, general ops | 500 | 2s only, fast add, lazy gcd | 7.77 ms | 7.47 ms | 4% | 1.02x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 500 | Fraction | 7.46 ms | 7.26 ms | 3% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2b 40 halvings, exponent shortcut | 500 | 2s and 5s factored | 5.13 ms | 4.93 ms | 5% | 0.69x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2b 40 halvings, exponent shortcut | 500 | 2s only | 10.1 ms | 9.76 ms | 2% | 1.35x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 500 | 2s and 5s, lazy cleanup | 5.1 ms | 4.93 ms | 4% | 0.68x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2b 40 halvings, exponent shortcut | 500 | 2s only, fast add | 10.2 ms | 9.77 ms | 2% | 1.36x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 500 | 2s only, fast add, lazy gcd | 10.2 ms | 9.89 ms | 3% | 1.36x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 500 | Fraction | 420 µs | 403 µs | 2% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2c halvings folded into one constant | 500 | 2s and 5s factored | 428 µs | 410 µs | 4% | 1.02x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2c halvings folded into one constant | 500 | 2s only | 556 µs | 531 µs | 5% | 1.32x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 500 | 2s and 5s, lazy cleanup | 427 µs | 403 µs | 4% | 1.02x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2c halvings folded into one constant | 500 | 2s only, fast add | 553 µs | 532 µs | 5% | 1.32x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 500 | 2s only, fast add, lazy gcd | 560 µs | 533 µs | 3% | 1.33x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F3 sum of 3-place decimals | 5000 | Fraction | 4.25 ms | 4.05 ms | 4% | 1.00x | 0 (exact) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 5000 | 2s and 5s factored | 7.05 ms | 6.89 ms | 3% | 1.66x | 0 (matches Fraction) |  | result bits mean 36, max 36 |
| F3 sum of 3-place decimals | 5000 | 2s only | 5.72 ms | 5.57 ms | 3% | 1.35x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 5000 | 2s and 5s, lazy cleanup | 5.33 ms | 5.1 ms | 5% | 1.26x | 0 (matches Fraction) |  | result bits mean 36, max 36 |
| F3 sum of 3-place decimals | 5000 | 2s only, fast add | 5.1 ms | 4.82 ms | 4% | 1.20x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 5000 | 2s only, fast add, lazy gcd | 4.6 ms | 4.43 ms | 2% | 1.08x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F4 sum of mixed scales | 5000 | Fraction | 4.35 ms | 4.17 ms | 2% | 1.00x | 0 (exact) |  | result bits mean 87, max 87 |
| F4 sum of mixed scales | 5000 | 2s and 5s factored | 8.71 ms | 8.5 ms | 2% | 2.00x | 0 (matches Fraction) |  | result bits mean 76, max 76 |
| F4 sum of mixed scales | 5000 | 2s only | 6.51 ms | 6.28 ms | 2% | 1.50x | 0 (matches Fraction) |  | result bits mean 85, max 85 |
| F4 sum of mixed scales | 5000 | 2s and 5s, lazy cleanup | 5.99 ms | 5.87 ms | 2% | 1.38x | 0 (matches Fraction) |  | result bits mean 76, max 76 |
| F4 sum of mixed scales | 5000 | 2s only, fast add | 6.09 ms | 5.9 ms | 2% | 1.40x | 0 (matches Fraction) |  | result bits mean 85, max 85 |
| F4 sum of mixed scales | 5000 | 2s only, fast add, lazy gcd | 5.57 ms | 5.39 ms | 2% | 1.28x | 0 (matches Fraction) |  | result bits mean 85, max 85 |
| F5 mixed formula | 500 | Fraction | 1.96 ms | 1.88 ms | 3% | 1.00x | 0 (exact) |  | result bits mean 60, max 70 |
| F5 mixed formula | 500 | 2s and 5s factored | 3.35 ms | 3.13 ms | 2% | 1.71x | 0 (matches Fraction) |  | result bits mean 50, max 58 |
| F5 mixed formula | 500 | 2s only | 2.69 ms | 2.62 ms | 3% | 1.37x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
| F5 mixed formula | 500 | 2s and 5s, lazy cleanup | 2.69 ms | 2.58 ms | 3% | 1.37x | 0 (matches Fraction) |  | result bits mean 52, max 58 |
| F5 mixed formula | 500 | 2s only, fast add | 2.51 ms | 2.38 ms | 3% | 1.28x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
| F5 mixed formula | 500 | 2s only, fast add, lazy gcd | 2.41 ms | 2.33 ms | 2% | 1.23x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
