# Factoring out 2s and 5s

- **date**: 2026-10-25 23:21:05 CDT
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
| F1 unit conversions | 10000 | Fraction | 20.5 ms | 20.2 ms | 1% | 1.00x | 0 (exact) |  | result bits mean 72, max 76 |
| F1 unit conversions | 10000 | 2s and 5s factored | 22.6 ms | 22.3 ms | 1% | 1.10x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1 unit conversions | 10000 | 2s only | 21.2 ms | 20.8 ms | 1% | 1.03x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1 unit conversions | 10000 | 2s and 5s, lazy cleanup | 22.7 ms | 22.5 ms | 1% | 1.11x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1 unit conversions | 10000 | 2s only, fast add | 21.3 ms | 20.9 ms | 1% | 1.04x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1 unit conversions | 10000 | 2s only, fast add, lazy gcd | 21.4 ms | 21 ms | 1% | 1.04x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 10000 | Fraction | 8.28 ms | 8.14 ms | 1% | 1.00x | 0 (exact) |  | result bits mean 72, max 76 |
| F1b unit conversion folded into one constant | 10000 | 2s and 5s factored | 10.6 ms | 10.3 ms | 1% | 1.27x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1b unit conversion folded into one constant | 10000 | 2s only | 9.49 ms | 9.38 ms | 2% | 1.15x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 10000 | 2s and 5s, lazy cleanup | 10.5 ms | 10.3 ms | 2% | 1.27x | 0 (matches Fraction) |  | result bits mean 42, max 45 |
| F1b unit conversion folded into one constant | 10000 | 2s only, fast add | 9.52 ms | 9.33 ms | 1% | 1.15x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F1b unit conversion folded into one constant | 10000 | 2s only, fast add, lazy gcd | 9.61 ms | 9.47 ms | 2% | 1.16x | 0 (matches Fraction) |  | result bits mean 68, max 72 |
| F2a 40 halvings, general ops | 10000 | Fraction | 148 ms | 145 ms | 3% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2a 40 halvings, general ops | 10000 | 2s and 5s factored | 166 ms | 162 ms | 2% | 1.12x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2a 40 halvings, general ops | 10000 | 2s only | 153 ms | 152 ms | 1% | 1.03x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2a 40 halvings, general ops | 10000 | 2s and 5s, lazy cleanup | 165 ms | 162 ms | 1% | 1.12x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2a 40 halvings, general ops | 10000 | 2s only, fast add | 154 ms | 150 ms | 2% | 1.04x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2a 40 halvings, general ops | 10000 | 2s only, fast add, lazy gcd | 155 ms | 153 ms | 2% | 1.04x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 10000 | Fraction | 153 ms | 146 ms | 4% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2b 40 halvings, exponent shortcut | 10000 | 2s and 5s factored | 100 ms | 98.6 ms | 1% | 0.65x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2b 40 halvings, exponent shortcut | 10000 | 2s only | 200 ms | 198 ms | 1% | 1.31x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 10000 | 2s and 5s, lazy cleanup | 101 ms | 99.1 ms | 3% | 0.66x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2b 40 halvings, exponent shortcut | 10000 | 2s only, fast add | 200 ms | 199 ms | 2% | 1.30x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2b 40 halvings, exponent shortcut | 10000 | 2s only, fast add, lazy gcd | 202 ms | 199 ms | 4% | 1.31x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 10000 | Fraction | 8.74 ms | 8.5 ms | 3% | 1.00x | 0 (exact) |  | result bits mean 100, max 103 |
| F2c halvings folded into one constant | 10000 | 2s and 5s factored | 8.86 ms | 8.43 ms | 5% | 1.01x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2c halvings folded into one constant | 10000 | 2s only | 11.3 ms | 10.9 ms | 3% | 1.29x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 10000 | 2s and 5s, lazy cleanup | 8.84 ms | 8.52 ms | 3% | 1.01x | 0 (matches Fraction) |  | result bits mean 34, max 37 |
| F2c halvings folded into one constant | 10000 | 2s only, fast add | 11.3 ms | 11 ms | 4% | 1.30x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F2c halvings folded into one constant | 10000 | 2s only, fast add, lazy gcd | 11.3 ms | 11 ms | 2% | 1.29x | 0 (matches Fraction) |  | result bits mean 62, max 64 |
| F3 sum of 3-place decimals | 100000 | Fraction | 80.3 ms | 78.3 ms | 1% | 1.00x | 0 (exact) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 100000 | 2s and 5s factored | 140 ms | 138 ms | 3% | 1.74x | 0 (matches Fraction) |  | result bits mean 36, max 36 |
| F3 sum of 3-place decimals | 100000 | 2s only | 111 ms | 110 ms | 2% | 1.38x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 100000 | 2s and 5s, lazy cleanup | 104 ms | 101 ms | 2% | 1.29x | 0 (matches Fraction) |  | result bits mean 36, max 36 |
| F3 sum of 3-place decimals | 100000 | 2s only, fast add | 100 ms | 98.4 ms | 2% | 1.24x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F3 sum of 3-place decimals | 100000 | 2s only, fast add, lazy gcd | 91.2 ms | 89.9 ms | 1% | 1.13x | 0 (matches Fraction) |  | result bits mean 39, max 39 |
| F4 sum of mixed scales | 100000 | Fraction | 77.1 ms | 76.5 ms | 1% | 1.00x | 0 (exact) |  | result bits mean 88, max 88 |
| F4 sum of mixed scales | 100000 | 2s and 5s factored | 167 ms | 166 ms | 1% | 2.16x | 0 (matches Fraction) |  | result bits mean 78, max 78 |
| F4 sum of mixed scales | 100000 | 2s only | 124 ms | 123 ms | 1% | 1.60x | 0 (matches Fraction) |  | result bits mean 87, max 87 |
| F4 sum of mixed scales | 100000 | 2s and 5s, lazy cleanup | 113 ms | 112 ms | 1% | 1.47x | 0 (matches Fraction) |  | result bits mean 79, max 79 |
| F4 sum of mixed scales | 100000 | 2s only, fast add | 116 ms | 115 ms | 1% | 1.50x | 0 (matches Fraction) |  | result bits mean 87, max 87 |
| F4 sum of mixed scales | 100000 | 2s only, fast add, lazy gcd | 106 ms | 105 ms | 1% | 1.38x | 0 (matches Fraction) |  | result bits mean 87, max 87 |
| F5 mixed formula | 10000 | Fraction | 38 ms | 37.1 ms | 1% | 1.00x | 0 (exact) |  | result bits mean 60, max 70 |
| F5 mixed formula | 10000 | 2s and 5s factored | 64.6 ms | 63 ms | 2% | 1.70x | 0 (matches Fraction) |  | result bits mean 50, max 58 |
| F5 mixed formula | 10000 | 2s only | 52.6 ms | 51.6 ms | 3% | 1.39x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
| F5 mixed formula | 10000 | 2s and 5s, lazy cleanup | 51.7 ms | 51 ms | 2% | 1.36x | 0 (matches Fraction) |  | result bits mean 52, max 58 |
| F5 mixed formula | 10000 | 2s only, fast add | 47.8 ms | 47.1 ms | 2% | 1.26x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
| F5 mixed formula | 10000 | 2s only, fast add, lazy gcd | 46.8 ms | 45.8 ms | 4% | 1.23x | 0 (matches Fraction) |  | result bits mean 58, max 67 |
