# The Q72 vector form against the exact tools

- **date**: 2026-10-26 16:26:31 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s); openblas 1 thread(s)
- **flint threads**: 1
- **decimal**: libmpdec 4.0.0, precision 80, Inexact trapped
- **gmpy2**: 2.3.2
- **compiled baselines**: gcc -O3 -march=native; GMP 6.3.0, FLINT 3.0.1 (system packages)

## A. Column operations (baseline GMP mpq_t from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| A1 sum | 1000000 | GMP mpq | 110 ms | 108 ms | 1% | 1.14x | 0 (exact) |  | object array |
| A1 sum | 1000000 | FLINT fmpq | 106 ms | 104 ms | 3% | 1.10x | 0 (exact) |  | object array |
| A1 sum | 1000000 | Python Fraction | 419 ms | 413 ms | 0% | 4.33x | 0 (exact) |  | object array |
| A1 sum | 1000000 | Python decimal | 48.4 ms | 45.9 ms | 3% | 0.50x | 0 (exact) |  | object array |
| A1 sum | 1000000 | GMP mpz numerators | 27.1 ms | 26.8 ms | 1% | 0.28x | 0 (exact) |  | object array of numerators |
| A1 sum | 1000000 | NumPy int64 fixed point | 331 µs | 320 µs | 2% | 0.00x | 0 (exact) | 8 | int64 numerators, unchecked |
| A1 sum | 1000000 | Q72 vector, turn bytes | 571 µs | 524 µs | 8% | 0.01x | 0 (exact) | 9 | tier n64 |
| A1 sum | 1000000 | Q72 vector, signed | 431 µs | 412 µs | 6% | 0.00x | 0 (exact) | 8 | tier s64 |
| A1 sum | 1000000 | GMP mpq_t (C) | 96.7 ms | 96.1 ms | 1% | 1.00x | 0 (exact) |  | compiled; rationals |
| A1 sum | 1000000 | FLINT fmpq (C) | 62.8 ms | 62.5 ms | 0% | 0.65x | 0 (exact) |  | compiled; rationals |
| A1 sum | 1000000 | GMP mpz_t numerators (C) | 9.95 ms | 9.93 ms | 0% | 0.10x | 0 (exact) |  | compiled; integers over one denominator |
| A1 sum | 1000000 | FLINT fmpz numerators (C) | 2.17 ms | 2.07 ms | 4% | 0.02x | 0 (exact) |  | compiled; integers over one denominator |
| A1 sum | 1000000 | C int64, unchecked | 312 µs | 311 µs | 0% | 0.00x | 0 (exact) | 8 | compiled; integers over one denominator |
| A1 sum | 1000000 | C int64, overflow-checked | 517 µs | 513 µs | 1% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A1 sum | 1000000 | C int64, checked, widening to int128 | 507 µs | 505 µs | 0% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A2 mean | 1000000 | GMP mpq | 109 ms | 108 ms | 1% |  | 0 (exact) |  | object array |
| A2 mean | 1000000 | FLINT fmpq | 105 ms | 103 ms | 4% |  | 0 (exact) |  | object array |
| A2 mean | 1000000 | Python Fraction | 415 ms | 411 ms | 4% |  | 0 (exact) |  | object array |
| A2 mean | 1000000 | Python decimal | 50.7 ms | 47.5 ms | 7% |  | 0 (exact) |  | object array |
| A2 mean | 1000000 | GMP mpz numerators | 26.8 ms | 26.1 ms | 2% |  | 0 (exact) |  | object array of numerators |
| A2 mean | 1000000 | NumPy int64 fixed point | 375 µs | 325 µs | 13% ⚠ |  | 0 (exact) | 8 | int64 numerators, unchecked |
| A2 mean | 1000000 | Q72 vector, turn bytes | 512 µs | 448 µs | 16% ⚠ |  | 0 (exact) | 9 | tier n64 |
| A2 mean | 1000000 | Q72 vector, signed | 458 µs | 417 µs | 10% ⚠ |  | 0 (exact) | 8 | tier s64 |
| A3 a*b+c, 3 places | 1000000 | GMP mpq | 441 ms | 434 ms | 3% | 2.25x | 0 (every element) |  | object array |
| A3 a*b+c, 3 places | 1000000 | FLINT fmpq | 266 ms | 265 ms | 3% | 1.36x | 0 (every element) |  | object array |
| A3 a*b+c, 3 places | 1000000 | Python Fraction | 1.76 s | 1.74 s | 0% | 8.98x | 0 (every element) |  | object array |
| A3 a*b+c, 3 places | 1000000 | Python decimal | 226 ms | 218 ms | 2% | 1.15x | 0 (every element) |  | object array |
| A3 a*b+c, 3 places | 1000000 | GMP mpz numerators | 221 ms | 218 ms | 1% | 1.13x | 0 (every element) |  | object array of numerators |
| A3 a*b+c, 3 places | 1000000 | NumPy int64 fixed point | 4 ms | 3.38 ms | 18% ⚠ | 0.02x | 0 (every element) | 8 | int64 numerators, unchecked |
| A3 a*b+c, 3 places | 1000000 | Q72 vector, turn bytes | 6.22 ms | 5.4 ms | 4% | 0.03x | 0 (every element) | 9 | tier n64 |
| A3 a*b+c, 3 places | 1000000 | Q72 vector, signed | 5.75 ms | 4.97 ms | 2% | 0.03x | 0 (every element) | 8 | tier s64 |
| A3 a*b+c, 3 places | 1000000 | GMP mpq_t (C) | 196 ms | 195 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| A3 a*b+c, 3 places | 1000000 | FLINT fmpq (C) | 156 ms | 155 ms | 1% | 0.80x | 0 (every element) |  | compiled; rationals |
| A3 a*b+c, 3 places | 1000000 | GMP mpz_t numerators (C) | 16.9 ms | 16.7 ms | 1% | 0.09x | 0 (every element) |  | compiled; integers over one denominator |
| A3 a*b+c, 3 places | 1000000 | FLINT fmpz numerators (C) | 16 ms | 14.3 ms | 11% ⚠ | 0.08x | 0 (every element) |  | compiled; integers over one denominator |
| A3 a*b+c, 3 places | 1000000 | C int64, unchecked | 1.08 ms | 1.07 ms | 1% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A3 a*b+c, 3 places | 1000000 | C int64, overflow-checked | 1.16 ms | 1.1 ms | 5% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A3 a*b+c, 3 places | 1000000 | C int64, checked, widening to int128 | 1.51 ms | 1.33 ms | 12% ⚠ | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A4 a*b+c, 6 places | 1000000 | GMP mpq | 450 ms | 443 ms | 0% | 1.97x | 0 (every element) |  | object array |
| A4 a*b+c, 6 places | 1000000 | FLINT fmpq | 350 ms | 346 ms | 1% | 1.53x | 0 (every element) |  | object array |
| A4 a*b+c, 6 places | 1000000 | Python Fraction | 1.99 s | 1.98 s | 2% | 8.75x | 0 (every element) |  | object array |
| A4 a*b+c, 6 places | 1000000 | Python decimal | 223 ms | 218 ms | 2% | 0.98x | 0 (every element) |  | object array |
| A4 a*b+c, 6 places | 1000000 | GMP mpz numerators | 226 ms | 225 ms | 0% | 0.99x | 0 (every element) |  | object array of numerators |
| A4 a*b+c, 6 places | 1000000 | Q72 vector, turn bytes | 15.7 ms | 15.6 ms | 3% | 0.07x | 0 (every element) | 17 | tier n128 |
| A4 a*b+c, 6 places | 1000000 | Q72 vector, signed | 9.32 ms | 8.62 ms | 1% | 0.04x | 0 (every element) | 16 | tier s128 |
| A4 a*b+c, 6 places | 1000000 | GMP mpq_t (C) | 228 ms | 228 ms | 0% | 1.00x | 0 (every element) |  | compiled; rationals |
| A4 a*b+c, 6 places | 1000000 | FLINT fmpq (C) | 207 ms | 207 ms | 0% | 0.91x | 0 (every element) |  | compiled; rationals |
| A4 a*b+c, 6 places | 1000000 | GMP mpz_t numerators (C) | 17.1 ms | 16.8 ms | 2% | 0.07x | 0 (every element) |  | compiled; integers over one denominator |
| A4 a*b+c, 6 places | 1000000 | FLINT fmpz numerators (C) | 34 ms | 33.7 ms | 1% | 0.15x | 0 (every element) |  | compiled; integers over one denominator |
| A4 a*b+c, 6 places | 1000000 | C int64, unchecked | 1.28 ms | 1.22 ms | 5% | 0.01x | wrong result | 8 | compiled; nothing checks (int64 wrapped) |
| A4 a*b+c, 6 places | 1000000 | C int64, overflow-checked | 1.32 ms | 1.31 ms | 1% | 0.01x | overflow detected |  | result discarded; would need a wider type |
| A4 a*b+c, 6 places | 1000000 | C int64, checked, widening to int128 | 2.88 ms | 2.83 ms | 2% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A4 a*b+c, 6 places | 1000000 | NumPy int64 fixed point |  |  |  |  |  |  | not run: a*b overflows int64 |
| A5 group-by sum | 1000000, G=1000 | GMP mpq | 180 ms | 179 ms | 1% | 1.68x | 0 (exact) |  | loop over object array |
| A5 group-by sum | 1000000, G=1000 | FLINT fmpq | 179 ms | 178 ms | 2% | 1.67x | 0 (exact) |  | loop over object array |
| A5 group-by sum | 1000000, G=1000 | Python Fraction | 555 ms | 553 ms | 1% | 5.17x | 0 (exact) |  | loop over object array |
| A5 group-by sum | 1000000, G=1000 | Python decimal | 117 ms | 115 ms | 2% | 1.09x | 0 (exact) |  | loop over object array |
| A5 group-by sum | 1000000, G=1000 | GMP mpz numerators | 84.7 ms | 83.1 ms | 1% | 0.79x | 0 (exact) |  | loop over object array |
| A5 group-by sum | 1000000, G=1000 | NumPy int64 fixed point | 1.7 ms | 1.66 ms | 2% | 0.02x | 0 (exact) |  | np.add.at, unchecked |
| A5 group-by sum | 1000000, G=1000 | Q72 vector, turn bytes | 2.21 ms | 2.11 ms | 4% | 0.02x | 0 (exact) |  | integer numerators over one denominator |
| A5 group-by sum | 1000000, G=1000 | Q72 vector, signed | 1.38 ms | 1.34 ms | 4% | 0.01x | 0 (exact) |  | integer numerators over one denominator |
| A5 group-by sum | 1000000, G=1000 | GMP mpq_t (C) | 107 ms | 106 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| A5 group-by sum | 1000000, G=1000 | FLINT fmpq (C) | 62.3 ms | 62.1 ms | 0% | 0.58x | 0 (every element) |  | compiled; rationals |
| A5 group-by sum | 1000000, G=1000 | GMP mpz_t numerators (C) | 12.5 ms | 11.3 ms | 10% | 0.12x | 0 (every element) |  | compiled; integers over one denominator |
| A5 group-by sum | 1000000, G=1000 | FLINT fmpz numerators (C) | 2.87 ms | 2.28 ms | 20% ⚠ | 0.03x | 0 (every element) |  | compiled; integers over one denominator |
| A5 group-by sum | 1000000, G=1000 | C int64, unchecked | 742 µs | 736 µs | 1% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A5 group-by sum | 1000000, G=1000 | C int64, overflow-checked | 776 µs | 767 µs | 1% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A5 group-by sum | 1000000, G=1000 | C int64, checked, widening to int128 | 780 µs | 777 µs | 0% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| A6 variance | 1000000 | GMP mpq | 357 ms | 350 ms | 2% | 1.70x | 0 (exact) |  | sums of x and x^2, one exact division |
| A6 variance | 1000000 | FLINT fmpq | 329 ms | 326 ms | 3% | 1.57x | 0 (exact) |  | sums of x and x^2, one exact division |
| A6 variance | 1000000 | Python Fraction | 1.68 s | 1.66 s | 2% | 8.00x | 0 (exact) |  | sums of x and x^2, one exact division |
| A6 variance | 1000000 | Python decimal | 244 ms | 236 ms | 2% | 1.16x | 0 (exact) |  | sums of x and x^2, one exact division |
| A6 variance | 1000000 | GMP mpz numerators | 117 ms | 114 ms | 3% | 0.55x | 0 (exact) |  | sums of x and x^2, one exact division |
| A6 variance | 1000000 | Q72 vector, turn bytes | 727 µs | 704 µs | 10% ⚠ | 0.00x | 0 (exact) |  |  |
| A6 variance | 1000000 | Q72 vector, signed | 631 µs | 599 µs | 5% | 0.00x | 0 (exact) |  |  |
| A6 variance | 1000000 | GMP mpq_t (C) | 210 ms | 209 ms | 1% | 1.00x | 0 (exact) |  | compiled; rationals |
| A6 variance | 1000000 | FLINT fmpq (C) | 167 ms | 167 ms | 0% | 0.80x | 0 (exact) |  | compiled; rationals |
| A6 variance | 1000000 | GMP mpz_t numerators (C) | 17.4 ms | 17.2 ms | 1% | 0.08x | 0 (exact) |  | compiled; integers over one denominator |
| A6 variance | 1000000 | FLINT fmpz numerators (C) | 9.45 ms | 9.21 ms | 3% | 0.04x | 0 (exact) |  | compiled; integers over one denominator |
| A6 variance | 1000000 | C int64, unchecked | 664 µs | 646 µs | 3% | 0.00x | 0 (exact) | 8 | compiled; integers over one denominator |
| A6 variance | 1000000 | C int64, overflow-checked | 1.67 ms | 1.61 ms | 4% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A6 variance | 1000000 | C int64, checked, widening to int128 | 1.7 ms | 1.6 ms | 6% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A6 variance | 1000000 | NumPy int64 fixed point |  |  |  |  |  |  | not run: the sum of squares reaches 1e+20, past int64 |
| A7 variance, values near 1e6 | 1000000 | GMP mpq | 336 ms | 331 ms | 1% | 1.65x | 0 (exact) |  | sums of x and x^2, one exact division |
| A7 variance, values near 1e6 | 1000000 | FLINT fmpq | 333 ms | 332 ms | 1% | 1.64x | 0 (exact) |  | sums of x and x^2, one exact division |
| A7 variance, values near 1e6 | 1000000 | Python Fraction | 1.64 s | 1.63 s | 1% | 8.04x | 0 (exact) |  | sums of x and x^2, one exact division |
| A7 variance, values near 1e6 | 1000000 | Python decimal | 217 ms | 209 ms | 5% | 1.06x | 0 (exact) |  | sums of x and x^2, one exact division |
| A7 variance, values near 1e6 | 1000000 | GMP mpz numerators | 100 ms | 97.8 ms | 2% | 0.49x | 0 (exact) |  | sums of x and x^2, one exact division |
| A7 variance, values near 1e6 | 1000000 | Q72 vector, turn bytes | 683 µs | 594 µs | 36% ⚠ | 0.00x | 0 (exact) |  |  |
| A7 variance, values near 1e6 | 1000000 | Q72 vector, signed | 605 µs | 409 µs | 6% | 0.00x | 0 (exact) |  |  |
| A7 variance, values near 1e6 | 1000000 | GMP mpq_t (C) | 203 ms | 203 ms | 0% | 1.00x | 0 (exact) |  | compiled; rationals |
| A7 variance, values near 1e6 | 1000000 | FLINT fmpq (C) | 174 ms | 174 ms | 0% | 0.86x | 0 (exact) |  | compiled; rationals |
| A7 variance, values near 1e6 | 1000000 | GMP mpz_t numerators (C) | 11.6 ms | 11.5 ms | 0% | 0.06x | 0 (exact) |  | compiled; integers over one denominator |
| A7 variance, values near 1e6 | 1000000 | FLINT fmpz numerators (C) | 10.4 ms | 10.4 ms | 1% | 0.05x | 0 (exact) |  | compiled; integers over one denominator |
| A7 variance, values near 1e6 | 1000000 | C int64, unchecked | 571 µs | 563 µs | 1% | 0.00x | 0 (exact) | 8 | compiled; integers over one denominator |
| A7 variance, values near 1e6 | 1000000 | C int64, overflow-checked | 1.5 ms | 1.4 ms | 6% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A7 variance, values near 1e6 | 1000000 | C int64, checked, widening to int128 | 1.47 ms | 1.45 ms | 1% | 0.01x | 0 (exact) | 8 | compiled; integers over one denominator |
| A7 variance, values near 1e6 | 1000000 | NumPy int64 fixed point |  |  |  |  |  |  | not run: the sum of squares reaches 1e+24, past int64 |

## U. Workloads that do not favour the vector form (baseline GMP mpq_t from C)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| U1 unrelated denominators 1..1000, sum | 100000 | Q72 vector, signed, including conversion | 121 ms | 120 ms | 1% | 5.15x | 0 (verified) |  | least common multiple and rescaling, then the operation |
| U1 unrelated denominators 1..1000, sum | 100000 | Q72 vector, signed | 6.29 ms | 5.95 ms | 3% | 0.27x | 0 (verified) |  | conversion excluded; common denominator 1438 bits; tier big |
| U1 unrelated denominators 1..1000, sum | 100000 | GMP mpq | 35.6 ms | 34.8 ms | 2% | 1.51x | 0 (verified) |  | object array |
| U1 unrelated denominators 1..1000, sum | 100000 | GMP mpq_t (C) | 23.6 ms | 23.3 ms | 1% | 1.00x | 0 (exact) |  | compiled; rationals |
| U1 unrelated denominators 1..1000, sum | 100000 | FLINT fmpq (C) | 25.1 ms | 24.8 ms | 1% | 1.06x | 0 (exact) |  | compiled; rationals |
| U1 unrelated denominators 1..1000, fma | 100000 | Q72 vector, signed, including conversion | 1.42 s | 1.4 s | 1% | 77.59x | 0 (verified) |  | least common multiple and rescaling, then the operation |
| U1 unrelated denominators 1..1000, fma | 100000 | Q72 vector, signed | 1.06 s | 1.05 s | 1% | 57.60x | 0 (verified) |  | conversion excluded; common denominator 1438 bits; tier big |
| U1 unrelated denominators 1..1000, fma | 100000 | GMP mpq | 41.2 ms | 41.1 ms | 0% | 2.25x | 0 (verified) |  | object array |
| U1 unrelated denominators 1..1000, fma | 100000 | GMP mpq_t (C) | 18.3 ms | 18.2 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| U1 unrelated denominators 1..1000, fma | 100000 | FLINT fmpq (C) | 12.5 ms | 12.4 ms | 1% | 0.68x | 0 (every element) |  | compiled; rationals |
| U1 unrelated denominators 1..1000, conversion | 100000 | conversion into the vector form | 114 ms | 111 ms | 3% |  | 0 (exact) |  | least common multiple, then scaling every numerator |
| U2 mixed small denominators 1..12, sum | 100000 | Q72 vector, signed, including conversion | 17.5 ms | 17.1 ms | 1% | 1.92x | 0 (verified) |  | least common multiple and rescaling, then the operation |
| U2 mixed small denominators 1..12, sum | 100000 | Q72 vector, signed | 12.1 µs | 11.3 µs | 4% | 0.00x | 0 (verified) |  | conversion excluded; common denominator 15 bits; tier s64 |
| U2 mixed small denominators 1..12, sum | 100000 | GMP mpq | 9.39 ms | 8.98 ms | 3% | 1.03x | 0 (verified) |  | object array |
| U2 mixed small denominators 1..12, sum | 100000 | GMP mpq_t (C) | 9.12 ms | 8.94 ms | 2% | 1.00x | 0 (exact) |  | compiled; rationals |
| U2 mixed small denominators 1..12, sum | 100000 | FLINT fmpq (C) | 4.82 ms | 4.76 ms | 1% | 0.53x | 0 (exact) |  | compiled; rationals |
| U2 mixed small denominators 1..12, fma | 100000 | Q72 vector, signed, including conversion | 52.5 ms | 51.2 ms | 2% | 3.47x | 0 (verified) |  | least common multiple and rescaling, then the operation |
| U2 mixed small denominators 1..12, fma | 100000 | Q72 vector, signed | 940 µs | 790 µs | 10% | 0.06x | 0 (verified) |  | conversion excluded; common denominator 15 bits; tier s64 |
| U2 mixed small denominators 1..12, fma | 100000 | GMP mpq | 36.9 ms | 35.9 ms | 2% | 2.44x | 0 (verified) |  | object array |
| U2 mixed small denominators 1..12, fma | 100000 | GMP mpq_t (C) | 15.1 ms | 15 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| U2 mixed small denominators 1..12, fma | 100000 | FLINT fmpq (C) | 9.23 ms | 9 ms | 2% | 0.61x | 0 (every element) |  | compiled; rationals |
| U2 mixed small denominators 1..12, conversion | 100000 | conversion into the vector form | 17.4 ms | 16.9 ms | 1% |  | 0 (exact) |  | least common multiple, then scaling every numerator |
| U3 growth chain, 10 steps of x*b + c | 20000 | Q72 vector, signed | 49.3 ms | 48.8 ms | 1% | 1.29x | 0 (verified) |  | ends in tier big, denominator 110 bits |
| U3 growth chain, 10 steps of x*b + c | 20000 | GMP mpq_t (C) | 38.1 ms | 37.8 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| U3 growth chain, 10 steps of x*b + c | 20000 | FLINT fmpq (C) | 45.9 ms | 44.6 ms | 3% | 1.20x | 0 (every element) |  | compiled; rationals |
| U4 a*b+c, wide elements: none | 100000 | Q72 vector, signed | 371 µs | 307 µs | 17% ⚠ | 0.02x | 0 (every element) |  | result tier s64 |
| U4 a*b+c, wide elements: none | 100000 | GMP mpq_t (C) | 19.8 ms | 19.3 ms | 2% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 a*b+c, wide elements: none | 100000 | FLINT fmpz numerators (C) | 1.41 ms | 1.35 ms | 4% | 0.07x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: none | 100000 | C int64, overflow-checked | 107 µs | 103 µs | 4% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| U4 a*b+c, wide elements: none | 100000 | C int64, checked, widening to int128 | 104 µs | 104 µs | 0% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: none | 100000 | Q72 vector, signed | 532 µs | 480 µs | 8% | 0.04x | 0 (every element) |  | result tier s128 |
| U4 (a*b)^2, wide elements: none | 100000 | GMP mpq_t (C) | 12.8 ms | 12.7 ms | 1% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 (a*b)^2, wide elements: none | 100000 | GMP mpz_t numerators (C) | 1.06 ms | 1.01 ms | 4% | 0.08x | 0 (every element) |  | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: none | 100000 | FLINT fmpz numerators (C) | 2.95 ms | 2.91 ms | 2% | 0.23x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: one element | 100000 | Q72 vector, signed | 835 µs | 790 µs | 5% | 0.04x | 0 (every element) |  | result tier s128 |
| U4 a*b+c, wide elements: one element | 100000 | GMP mpq_t (C) | 20 ms | 19.5 ms | 3% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 a*b+c, wide elements: one element | 100000 | FLINT fmpz numerators (C) | 1.44 ms | 1.38 ms | 4% | 0.07x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: one element | 100000 | C int64, overflow-checked | 116 µs | 116 µs | 0% | 0.01x | overflow detected |  | result discarded; would need a wider type |
| U4 a*b+c, wide elements: one element | 100000 | C int64, checked, widening to int128 | 301 µs | 288 µs | 4% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: one element | 100000 | Q72 vector, signed | 22.7 ms | 22.3 ms | 2% | 1.65x | 0 (every element) |  | result tier big |
| U4 (a*b)^2, wide elements: one element | 100000 | GMP mpq_t (C) | 13.7 ms | 12.9 ms | 6% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 (a*b)^2, wide elements: one element | 100000 | GMP mpz_t numerators (C) | 2.1 ms | 2.08 ms | 1% | 0.15x | 0 (every element) |  | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: one element | 100000 | FLINT fmpz numerators (C) | 4.05 ms | 3.99 ms | 1% | 0.29x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: 1% | 100000 | Q72 vector, signed | 722 µs | 696 µs | 3% | 0.03x | 0 (every element) |  | result tier s128 |
| U4 a*b+c, wide elements: 1% | 100000 | GMP mpq_t (C) | 27.8 ms | 21.4 ms | 23% ⚠ | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 a*b+c, wide elements: 1% | 100000 | FLINT fmpz numerators (C) | 1.48 ms | 1.42 ms | 4% | 0.05x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: 1% | 100000 | C int64, overflow-checked | 122 µs | 119 µs | 3% | 0.00x | overflow detected |  | result discarded; would need a wider type |
| U4 a*b+c, wide elements: 1% | 100000 | C int64, checked, widening to int128 | 307 µs | 288 µs | 6% | 0.01x | 0 (every element) | 8 | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: 1% | 100000 | Q72 vector, signed | 22.1 ms | 21.9 ms | 1% | 1.64x | 0 (every element) |  | result tier big |
| U4 (a*b)^2, wide elements: 1% | 100000 | GMP mpq_t (C) | 13.5 ms | 13.2 ms | 2% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 (a*b)^2, wide elements: 1% | 100000 | GMP mpz_t numerators (C) | 1.07 ms | 1.06 ms | 1% | 0.08x | 0 (every element) |  | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: 1% | 100000 | FLINT fmpz numerators (C) | 3.01 ms | 2.93 ms | 3% | 0.22x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: all | 100000 | Q72 vector, signed | 651 µs | 597 µs | 7% | 0.03x | 0 (every element) |  | result tier s128 |
| U4 a*b+c, wide elements: all | 100000 | GMP mpq_t (C) | 18.8 ms | 18.5 ms | 2% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 a*b+c, wide elements: all | 100000 | FLINT fmpz numerators (C) | 3.36 ms | 3.29 ms | 2% | 0.18x | 0 (every element) |  | compiled; integers over one denominator |
| U4 a*b+c, wide elements: all | 100000 | C int64, overflow-checked | 134 µs | 117 µs | 12% ⚠ | 0.01x | overflow detected |  | result discarded; would need a wider type |
| U4 a*b+c, wide elements: all | 100000 | C int64, checked, widening to int128 | 309 µs | 293 µs | 5% | 0.02x | 0 (every element) | 8 | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: all | 100000 | Q72 vector, signed | 24.6 ms | 22.8 ms | 6% | 2.24x | 0 (every element) |  | result tier big |
| U4 (a*b)^2, wide elements: all | 100000 | GMP mpq_t (C) | 11 ms | 10.5 ms | 4% | 1.00x | 0 (every element) |  | compiled; rationals |
| U4 (a*b)^2, wide elements: all | 100000 | GMP mpz_t numerators (C) | 1.24 ms | 1.15 ms | 7% | 0.11x | 0 (every element) |  | compiled; integers over one denominator |
| U4 (a*b)^2, wide elements: all | 100000 | FLINT fmpz numerators (C) | 1.57 ms | 1.54 ms | 2% | 0.14x | 0 (every element) |  | compiled; integers over one denominator |

## Summary: the signed vector form against the compiled contenders

| Benchmark | Size | vs GMP mpq_t (C) | vs FLINT fmpz numerators (C) | vs C int64, overflow-checked | vs C int64, checked, widening to int128 |
|---|---|---|---|---|---|
| A1 sum | 1000000 | 224x faster | 5.03x faster | 1.2x faster | 1.18x faster |
| A2 mean | 1000000 |  |  |  |  |
| A3 a*b+c, 3 places | 1000000 | 34.1x faster | 2.78x faster | 4.98x slower | 3.82x slower |
| A4 a*b+c, 6 places | 1000000 | 24.4x faster | 3.64x faster | 7.06x slower | 3.24x slower |
| A5 group-by sum | 1000000, G=1000 | 77.8x faster | 2.08x faster | 1.78x slower | 1.77x slower |
| A6 variance | 1000000 | 333x faster | 15x faster | 2.64x faster | 2.69x faster |
| A7 variance, values near 1e6 | 1000000 | 336x faster | 17.3x faster | 2.47x faster | 2.43x faster |
| U1 unrelated denominators 1..1000, sum | 100000 | 5.15x slower |  |  |  |
| U1 unrelated denominators 1..1000, fma | 100000 | 77.6x slower |  |  |  |
| U2 mixed small denominators 1..12, sum | 100000 | 1.92x slower |  |  |  |
| U2 mixed small denominators 1..12, fma | 100000 | 3.47x slower |  |  |  |
| U3 growth chain, 10 steps of x*b + c | 20000 | 1.29x slower |  |  |  |
| U4 a*b+c, wide elements: none | 100000 | 53.3x faster | 3.81x faster | 3.46x slower | 3.56x slower |
| U4 (a*b)^2, wide elements: none | 100000 | 24x faster | 5.54x faster |  |  |
| U4 a*b+c, wide elements: one element | 100000 | 24x faster | 1.73x faster | 7.18x slower | 2.78x slower |
| U4 (a*b)^2, wide elements: one element | 100000 | 1.65x slower | 5.61x slower |  |  |
| U4 a*b+c, wide elements: 1% | 100000 | 38.5x faster | 2.04x faster | 5.92x slower | 2.35x slower |
| U4 (a*b)^2, wide elements: 1% | 100000 | 1.64x slower | 7.34x slower |  |  |
| U4 a*b+c, wide elements: all | 100000 | 28.8x faster | 5.16x faster | 4.86x slower | 2.11x slower |
| U4 (a*b)^2, wide elements: all | 100000 | 2.24x slower | 15.7x slower |  |  |

Where a contender reported an overflow or gave a wrong answer, its time is still shown in the tables above; this summary compares times only.

## B. Matrix product (baseline FLINT fmpq_mat)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| B1 matrix product | 500, 3 places | FLINT fmpq_mat | 75.8 ms | 74.7 ms | 2% | 1.00x | 0 (verified) |  |  |
| B1 matrix product | 500, 3 places | FLINT fmpz_mat (numerators) | 46.2 ms | 45.2 ms | 1% | 0.61x | 0 (exact) |  | shared denominator applied separately |
| B1 matrix product | 500, 3 places | NumPy int64 matmul (numerators) | 59.2 ms | 58.3 ms | 2% | 0.78x | 0 (verified) |  | no BLAS for integers; unchecked |
| B1 matrix product | 500, 3 places | NumPy float64 matmul (numerators, unchecked) | 3.77 ms | 3.49 ms | 5% | 0.05x | 0 (exact here) |  | bound below 2^53, so exact; nothing checks it |
| B1 matrix product | 500, 3 places | exact float-BLAS product (numerators) | 12.6 ms | 12.2 ms | 2% | 0.17x | 0 (equals FLINT) |  | one float GEMM; exact by bound |
| B1 matrix product | 1000, 3 places | FLINT fmpq_mat | 475 ms | 473 ms | 1% | 1.00x | 0 (verified) |  |  |
| B1 matrix product | 1000, 3 places | FLINT fmpz_mat (numerators) | 345 ms | 345 ms | 0% | 0.73x | 0 (exact) |  | shared denominator applied separately |
| B1 matrix product | 1000, 3 places | NumPy int64 matmul (numerators) | 807 ms | 784 ms | 3% | 1.70x | 0 (verified) |  | no BLAS for integers; unchecked |
| B1 matrix product | 1000, 3 places | NumPy float64 matmul (numerators, unchecked) | 24.9 ms | 23.5 ms | 3% | 0.05x | 0 (exact here) |  | bound below 2^53, so exact; nothing checks it |
| B1 matrix product | 1000, 3 places | exact float-BLAS product (numerators) | 60.4 ms | 59.8 ms | 1% | 0.13x | 0 (equals FLINT) |  | one float GEMM; exact by bound |
| B2 matrix product, 6 places | 500, 6 places | FLINT fmpq_mat | 130 ms | 122 ms | 5% | 1.00x | 0 (verified) |  |  |
| B2 matrix product, 6 places | 500, 6 places | FLINT fmpz_mat (numerators) | 69.5 ms | 67.2 ms | 3% | 0.54x | 0 (exact) |  | shared denominator applied separately |
| B2 matrix product, 6 places | 500, 6 places | NumPy float64 matmul (numerators, unchecked) | 3.75 ms | 3.6 ms | 7% | 0.03x | wrong: rounding |  | bound past 2^53; nothing checks it |
| B2 matrix product, 6 places | 500, 6 places | exact float-BLAS product (numerators) | 28.2 ms | 26 ms | 3% | 0.22x | 0 (equals FLINT) |  | split into limbs, 128-bit result; exact by bound |
| B2 matrix product, 6 places | 500, 6 places | NumPy int64 matmul (numerators) |  |  |  |  |  |  | not run: overflows int64 |
| B3 matrix product, 9 places | 500, 9 places | FLINT fmpq_mat | 159 ms | 154 ms | 3% | 1.00x | 0 (verified) |  |  |
| B3 matrix product, 9 places | 500, 9 places | FLINT fmpz_mat (numerators) | 73.8 ms | 70.5 ms | 2% | 0.46x | 0 (exact) |  | shared denominator applied separately |
| B3 matrix product, 9 places | 500, 9 places | NumPy float64 matmul (numerators, unchecked) | 3.76 ms | 3.5 ms | 9% | 0.02x | wrong: rounding |  | bound past 2^53; nothing checks it |
| B3 matrix product, 9 places | 500, 9 places | exact float-BLAS product (numerators) | 27 ms | 25.7 ms | 6% | 0.17x | 0 (equals FLINT) |  | split into limbs, 128-bit result; exact by bound |
| B3 matrix product, 9 places | 500, 9 places | NumPy int64 matmul (numerators) |  |  |  |  |  |  | not run: overflows int64 |

## C. Linear solve (baseline FLINT fmpq_mat)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| C1 solve, random right-hand side | 100 | FLINT fmpq_mat | 7.91 ms | 7.7 ms | 1% | 1.00x | 0 (verified) |  |  |
| C1 solve, random right-hand side | 100 | FLINT fmpz_mat | 7.58 ms | 7.38 ms | 2% | 0.96x | 0 (exact) |  | Dixon p-adic |
| C1 solve, random right-hand side | 100 | exact solver chain (v0.1) | 19.5 ms | 19.2 ms | 2% | 2.47x | 0 (equals FLINT) |  | numeric-symbolic, Dixon, FLINT; used: numeric-symbolic; 0/100 entries fit one Q72 |
| C2 solve, planted integer answer | 100 | FLINT fmpq_mat | 1.65 ms | 1.62 ms | 2% | 1.00x | 0 (verified) |  |  |
| C2 solve, planted integer answer | 100 | FLINT fmpz_mat | 1.38 ms | 1.34 ms | 3% | 0.83x | 0 (exact) |  | Dixon p-adic |
| C2 solve, planted integer answer | 100 | exact solver chain (v0.1) | 1.44 ms | 1.39 ms | 4% | 0.87x | 0 (equals FLINT) |  | numeric-symbolic, Dixon, FLINT; used: numeric-symbolic; 100/100 entries fit one Q72 |
| C1 solve, random right-hand side | 500 | FLINT fmpq_mat | 790 ms | 782 ms | 1% | 1.00x | 0 (verified) |  |  |
| C1 solve, random right-hand side | 500 | FLINT fmpz_mat | 782 ms | 775 ms | 0% | 0.99x | 0 (exact) |  | Dixon p-adic |
| C1 solve, random right-hand side | 500 | exact solver chain (v0.1) | 661 ms | 649 ms | 2% | 0.84x | 0 (equals FLINT) |  | numeric-symbolic, Dixon, FLINT; used: numeric-symbolic; 0/500 entries fit one Q72 |
| C2 solve, planted integer answer | 500 | FLINT fmpq_mat | 110 ms | 107 ms | 2% | 1.00x | 0 (verified) |  |  |
| C2 solve, planted integer answer | 500 | FLINT fmpz_mat | 103 ms | 100 ms | 1% | 0.94x | 0 (exact) |  | Dixon p-adic |
| C2 solve, planted integer answer | 500 | exact solver chain (v0.1) | 22.7 ms | 21.4 ms | 3% | 0.21x | 0 (equals FLINT) |  | numeric-symbolic, Dixon, FLINT; used: numeric-symbolic; 500/500 entries fit one Q72 |

## D. Pipelines (baseline GMP mpq)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| D1 regression | 20000 | GMP mpq | 310 ms | 308 ms | 6% | 1.00x | 0 (matches reference) |  | loop over scalars |
| D1 regression | 20000 | Python Fraction | 997 ms | 996 ms | 2% | 3.22x | 0 (matches reference) |  | loop over scalars |
| D1 regression | 20000 | Q72 vector, turn bytes | 2.3 ms | 2.27 ms | 9% | 0.01x | 0 (matches reference) |  | solver: FLINT; 0/9 coefficients fit one Q72 |
| D1 regression | 20000 | Q72 vector, signed | 1.53 ms | 1.49 ms | 8% | 0.00x | 0 (matches reference) |  | solver: FLINT; 0/9 coefficients fit one Q72 |
| D2 table pipeline | 100000 | GMP mpq | 236 ms | 235 ms | 2% | 1.00x | 0 (matches reference) |  | loop over scalars |
| D2 table pipeline | 100000 | Python Fraction | 835 ms | 815 ms | 1% | 3.54x | 0 (matches reference) |  | loop over scalars |
| D2 table pipeline | 100000 | Q72 vector, turn bytes | 27.3 ms | 26.5 ms | 1% | 0.12x | 0 (matches reference) |  | revenue: tier n64; converted: tier n128 |
| D2 table pipeline | 100000 | Q72 vector, signed | 22.1 ms | 21.4 ms | 0% | 0.09x | 0 (matches reference) |  | revenue: tier s64; converted: tier s128 |
| D3 complex pipeline | 100000 | GMP mpq | 116 ms | 115 ms | 1% | 1.00x | 0 (matches reference) |  | loop over scalars |
| D3 complex pipeline | 100000 | Python Fraction | 426 ms | 422 ms | 1% | 3.68x | 0 (matches reference) |  | loop over scalars |
| D3 complex pipeline | 100000 | Q72 vector, turn bytes | 2.67 ms | 2.43 ms | 13% ⚠ | 0.02x | 0 (matches reference) |  | bin kinds ['one', 'two', 'one', 'two'] |
| D3 complex pipeline | 100000 | Q72 vector, signed | 1.18 ms | 1.04 ms | 14% ⚠ | 0.01x | 0 (matches reference) |  | bin kinds ['one', 'two', 'one', 'two'] |

## L. Layouts of the vector form (baseline turn bytes)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| L1 convert from int64 numerators | 10000000 | turn bytes | 20 ms | 17.7 ms | 5% | 1.00x | 0 (exact) | 9 |  |
| L1 convert from int64 numerators | 10000000 | packed turns | 20.2 ms | 18.3 ms | 5% | 1.01x | 0 (exact) | 8.25 |  |
| L1 convert from int64 numerators | 10000000 | signed | 20.7 ms | 17.6 ms | 5% | 1.04x | 0 (exact) | 8 |  |
| L2 sum | 10000000 | turn bytes | 10.2 ms | 9.75 ms | 1% | 1.00x | 0 (exact) | 9 |  |
| L2 sum | 10000000 | packed turns | 6.8 ms | 6.13 ms | 5% | 0.67x | 0 (exact) | 8.25 |  |
| L2 sum | 10000000 | signed | 6.18 ms | 6.06 ms | 1% | 0.61x | 0 (exact) | 8 |  |
| L3 a*b+c | 10000000 | turn bytes | 34.3 ms | 33.6 ms | 1% | 1.00x | 0 (exact) | 9 |  |
| L3 a*b+c | 10000000 | packed turns | 34.1 ms | 33.7 ms | 2% | 1.00x | 0 (exact) | 8.25 |  |
| L3 a*b+c | 10000000 | signed | 29.4 ms | 28.8 ms | 1% | 0.86x | 0 (exact) | 8 |  |
| L4 group-by sum, G=1000 | 10000000 | turn bytes | 19.1 ms | 16.8 ms | 16% ⚠ | 1.00x | 0 (exact) | 9 |  |
| L4 group-by sum, G=1000 | 10000000 | packed turns | 25.5 ms | 21.9 ms | 16% ⚠ | 1.34x | 0 (exact) | 8.25 |  |
| L4 group-by sum, G=1000 | 10000000 | signed | 12.5 ms | 9.77 ms | 14% ⚠ | 0.66x | 0 (exact) | 8 |  |
| L5 sum and sum of squares | 10000000 | turn bytes | 7.56 ms | 6.12 ms | 17% ⚠ | 1.00x | 0 (exact) | 9 |  |
| L5 sum and sum of squares | 10000000 | packed turns | 5.81 ms | 4.83 ms | 11% ⚠ | 0.77x | 0 (exact) | 8.25 |  |
| L5 sum and sum of squares | 10000000 | signed | 4.67 ms | 3.87 ms | 18% ⚠ | 0.62x | 0 (exact) | 8 |  |
| L6 exact division by a common factor | 10000000 | divide instruction | 43 ms | 42.1 ms | 2% | 1.00x | 0 (exact) |  | unsigned divide of the magnitude, sign restored |
| L6 exact division by a common factor | 10000000 | multiply by 2-adic inverse | 16.1 ms | 15.7 ms | 7% | 0.37x | 0 (exact) |  | quote notation's division |

## E. The cost of exactness (NumPy float64 for context)

| Benchmark | Size | Contender | Median | Min | Spread | vs baseline | Rel. error | Bytes/value | Note |
|---|---|---|---|---|---|---|---|---|---|
| E1 sum | 1000000 | NumPy float64 | 416 µs | 407 µs | 14% ⚠ | 1.00x |  |  | rounds |
| E1 sum | 1000000 | Q72 vector, signed | 330 µs | 310 µs | 10% | 0.79x | 0 (verified) |  |  |
| E2 a*b+c | 1000000 | NumPy float64 | 2.34 ms | 2.07 ms | 29% ⚠ | 1.00x |  |  | rounds |
| E2 a*b+c | 1000000 | Q72 vector, signed | 5 ms | 4.47 ms | 23% ⚠ | 2.13x | 0 (verified) |  |  |
| E3 variance | 1000000 | NumPy float64 | 2.79 ms | 2.52 ms | 11% ⚠ | 1.00x |  |  | rounds |
| E3 variance | 1000000 | Q72 vector, signed | 637 µs | 595 µs | 9% | 0.23x | 0 (verified) |  |  |
| E4 matrix product 1000, 6 places | 1000 | NumPy float64 | 23.9 ms | 23 ms | 3% | 1.00x |  |  | rounds |
| E4 matrix product 1000, 6 places | 1000 | exact float-BLAS product | 151 ms | 147 ms | 2% | 6.34x | 0 (verified) |  |  |

## F. Precision and exactness

| Magnitude | float64 median | Q72 median | float64 worst (sampled) | Q72 worst (sampled) | Q72 worst in [10^k/10, 10^k*10] |
|---|---|---|---|---|---|
| 1e-18 | 3.7e-17 | 6.4e-20 | 1.1e-16 | some samples out of range | 5.0e-01 (beside 1/9223372036854775808) (gap to zero, at the bottom of the range) |
| 1e-15 | 4.2e-17 | 6.5e-20 | 1.1e-16 | 2.3e-18 | 1.4e-17 (beside 1/140737488355328) |
| 1e-12 | 4.0e-17 | 6.6e-20 | 1.1e-16 | 1.3e-17 | 4.4e-16 (beside 1/137438953472) |
| 1e-9 | 4.1e-17 | 5.9e-20 | 1.1e-16 | 8.2e-18 | 1.4e-14 (beside 1/134217728) |
| 1e-6 | 3.6e-17 | 5.7e-20 | 1.0e-16 | 3.8e-18 | 4.5e-13 (beside 1/131072) |
| 1e-3 | 3.5e-17 | 7.0e-20 | 9.9e-17 | 8.1e-18 | 1.5e-11 (beside 1/128) |
| 1e0 | 3.5e-17 | 6.0e-20 | 1.0e-16 | 4.1e-18 | 1.2e-10 (beside 1) |
| 1e3 | 4.0e-17 | 6.8e-20 | 1.1e-16 | 8.2e-18 | 1.5e-11 (beside 104) |
| 1e6 | 3.7e-17 | 6.8e-20 | 1.0e-16 | 4.3e-17 | 4.5e-13 (beside 110592) |
| 1e9 | 3.7e-17 | 6.3e-20 | 1.1e-16 | 1.2e-17 | 1.4e-14 (beside 109051904) |
| 1e12 | 3.7e-17 | 6.1e-20 | 1.1e-16 | 1.2e-17 | 4.4e-16 (beside 107374182400) |
| 1e15 | 4.2e-17 | 6.4e-20 | 1.0e-16 | 3.4e-18 | 1.4e-17 (beside 109951162777600) |
| 1e18 | 4.1e-17 | 4.9e-20 | 1.0e-16 | some samples out of range | 4.8e-19 (beside 130604389193744384) |

| To match Q72's ... | needs a float significand of | an IEEE-style float of (with an 8-bit exponent) |
|---|---|---|
| typical (median) precision, 6.4e-20 | 63 bits | 71 bits |
| worst-case precision, 1.2e-10 | 33 bits | 41 bits |

Q72 is 72 bits. Its range is 2^-63 to 2^64 - 1 (about 1.1e-19 to 1.8e19); float64's is about 1e-308 to 1e308.

| Values | exact in binary64 | exact in decimal64 | exact in Q72 |
|---|---|---|---|
| money, 2 places, below 10^9 | 3.89% | 100% | 100% |
| measurements, 3 places, below 10^6 | 0.79% | 100% | 100% |
| measurements, 6 places, below 10^6 | 0.01% | 100% | 100% |
| products of two 3-place measurements | 0.045% | 16.3% | 10.4% |
| ratios a/b of integers up to 1000 | 2.5% | 5.1% | 100% |
| means of 7 three-place measurements | 0.12% | 14.2% | 100% |
| binary64 values in [0, 1) times 10^k, k in -6..6 | 100% | 0% | 0% |
