# Phase one: the variants compared with each other

- **date**: 2026-10-26 23:28:48 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1
- **compiled**: p1_arith, p1_layout, p1_widen: clang 18 -O3 -march=native; p1_gemm: gcc 13 -O3 -march=native with AVX-512 IFMA; OpenBLAS 0.3.34 (scipy-openblas32), one thread; GMP 6.3.0

## S1. Scalar layouts

### Sizes and precision (slash bits at the top; the bottom order has the same sizes)

| Layout | Total bits | Unit bits | Data bits | Data bits per stored bit | Median relative error | Worst (beside 1) |
|---|---|---|---|---|---|---|
| tfit_top_16 | 24 | 2 | 16 | 0.667 | 1.75e-05 | 1.97e-03 |
| tfit_top_32 | 40 | 2 | 32 | 0.800 | 2.49e-10 | 7.63e-06 |
| tfit_top_64 | 72 | 2 | 64 | 0.889 | 5.83e-20 | 1.16e-10 |
| tfit_top_128 | 136 | 2 | 127 | 0.934 | 7.06e-39 | 5.42e-20 |
| tuni_top_16 | 24 | 4 | 16 | 0.667 | 1.75e-05 | 1.97e-03 |
| tuni_top_32 | 40 | 4 | 31 | 0.775 | 5.17e-10 | 1.53e-05 |
| tuni_top_64 | 72 | 4 | 62 | 0.861 | 2.75e-19 | 2.33e-10 |
| tuni_top_128 | 136 | 4 | 125 | 0.919 | 2.91e-38 | 1.08e-19 |
| pfit_top_16 | 16 | 2 | 10 | 0.625 | 1.07e-03 | 1.67e-02 |
| pfit_top_32 | 32 | 2 | 25 | 0.781 | 3.54e-08 | 1.22e-04 |
| pfit_top_64 | 64 | 2 | 56 | 0.875 | 1.54e-17 | 1.86e-09 |
| pfit_top_128 | 128 | 2 | 119 | 0.930 | 1.78e-36 | 8.67e-19 |
| puni_top_16 | 16 | 4 | 8 | 0.500 | 4.07e-03 | 3.57e-02 |
| puni_top_32 | 32 | 4 | 23 | 0.719 | 1.60e-07 | 2.44e-04 |
| puni_top_64 | 64 | 4 | 54 | 0.844 | 6.80e-17 | 3.73e-09 |
| puni_top_128 | 128 | 4 | 117 | 0.914 | 6.91e-36 | 1.73e-18 |

### Cost per value (ns, median), by storage

| Layout | Storage | Bytes per value | Decode | Encode | Multiply (decode both, multiply, reduce, encode) | Promote to the next size | Complex to quaternion, same size |
|---|---|---|---|---|---|---|---|
| tfit_top_16 | aos | 4 | 5.29 | 2.69 | 24.52 | 0.26 | 1.50 |
| tfit_top_16 | aosp | 3 | 4.97 | 2.77 | 24.68 | 0.26 | 1.50 |
| tfit_top_16 | soa | 3 | 5.41 | 3.02 | 26.10 | 0.26 | 1.50 |
| tfit_top_32 | aos | 8 | 2.19 | 2.75 | 40.31 | 0.63 | 0.42 |
| tfit_top_32 | aosp | 5 | 2.23 | 2.70 | 42.96 | 0.63 | 0.42 |
| tfit_top_32 | soa | 5 | 1.84 | 2.65 | 40.82 | 0.63 | 0.42 |
| tfit_top_64 | aos | 16 | 1.66 | 2.93 | 74.57 | 1.95 | 1.55 (27988 no longer fit) |
| tfit_top_64 | aosp | 9 | 1.63 | 2.72 | 75.55 | 1.95 | 1.55 (27988 no longer fit) |
| tfit_top_64 | soa | 9 | 1.74 | 2.64 | 73.01 | 1.95 | 1.55 (27988 no longer fit) |
| tfit_top_128 | aos | 32 | 6.66 | 7.33 | 142.86 | — | 9.86 (28010 no longer fit) |
| tfit_top_128 | aosp | 17 | 6.56 | 6.97 | 145.45 | — | 9.86 (28010 no longer fit) |
| tfit_top_128 | soa | 17 | 6.69 | 6.82 | 147.36 | — | 9.86 (28010 no longer fit) |
| tfit_bot_16 | aos | 4 | 6.54 | 2.79 | 23.82 | 0.26 | — |
| tfit_bot_16 | aosp | 3 | 6.40 | 2.63 | 24.54 | 0.26 | — |
| tfit_bot_16 | soa | 3 | 6.54 | 2.68 | 26.31 | 0.26 | — |
| tfit_bot_32 | aos | 8 | 2.82 | 4.63 | 39.96 | 0.61 | — |
| tfit_bot_32 | aosp | 5 | 2.27 | 4.45 | 42.25 | 0.61 | — |
| tfit_bot_32 | soa | 5 | 1.95 | 2.70 | 40.78 | 0.61 | — |
| tfit_bot_64 | aos | 16 | 1.63 | 2.81 | 74.72 | 2.03 | — |
| tfit_bot_64 | aosp | 9 | 1.68 | 2.76 | 75.66 | 2.03 | — |
| tfit_bot_64 | soa | 9 | 1.71 | 2.73 | 73.30 | 2.03 | — |
| tfit_bot_128 | aos | 32 | 6.81 | 7.33 | 145.67 | — | — |
| tfit_bot_128 | aosp | 17 | 6.83 | 7.34 | 146.99 | — | — |
| tfit_bot_128 | soa | 17 | 6.72 | 6.98 | 143.18 | — | — |
| tuni_top_16 | aos | 4 | 1.53 | 2.76 | 22.57 | 1.08 | 0.06 |
| tuni_top_16 | aosp | 3 | 1.98 | 2.65 | 23.00 | 1.08 | 0.06 |
| tuni_top_16 | soa | 3 | 1.51 | 4.38 | 22.42 | 1.08 | 0.06 |
| tuni_top_32 | aos | 8 | 2.00 | 2.97 | 41.31 | 0.80 | 0.14 |
| tuni_top_32 | aosp | 5 | 2.02 | 2.88 | 42.74 | 0.80 | 0.14 |
| tuni_top_32 | soa | 5 | 1.94 | 2.91 | 42.81 | 0.80 | 0.14 |
| tuni_top_64 | aos | 16 | 2.77 | 3.13 | 75.08 | 2.24 | 0.57 |
| tuni_top_64 | aosp | 9 | 1.84 | 3.00 | 75.46 | 2.24 | 0.57 |
| tuni_top_64 | soa | 9 | 1.80 | 2.95 | 75.40 | 2.24 | 0.57 |
| tuni_top_128 | aos | 32 | 5.10 | 7.03 | 143.83 | — | 1.15 |
| tuni_top_128 | aosp | 17 | 4.04 | 9.46 | 143.64 | — | 1.15 |
| tuni_top_128 | soa | 17 | 3.80 | 9.44 | 144.51 | — | 1.15 |
| tuni_bot_16 | aos | 4 | 1.53 | 2.72 | 21.94 | 1.17 | — |
| tuni_bot_16 | aosp | 3 | 1.97 | 2.62 | 22.83 | 1.17 | — |
| tuni_bot_16 | soa | 3 | 1.55 | 2.73 | 22.83 | 1.17 | — |
| tuni_bot_32 | aos | 8 | 2.04 | 2.97 | 40.86 | 0.87 | — |
| tuni_bot_32 | aosp | 5 | 2.06 | 3.06 | 42.62 | 0.87 | — |
| tuni_bot_32 | soa | 5 | 2.04 | 2.88 | 40.97 | 0.87 | — |
| tuni_bot_64 | aos | 16 | 2.57 | 3.09 | 76.23 | 2.39 | — |
| tuni_bot_64 | aosp | 9 | 2.04 | 3.15 | 76.22 | 2.39 | — |
| tuni_bot_64 | soa | 9 | 1.83 | 3.16 | 73.95 | 2.39 | — |
| tuni_bot_128 | aos | 32 | 5.27 | 7.07 | 143.73 | — | — |
| tuni_bot_128 | aosp | 17 | 3.61 | 7.21 | 143.74 | — | — |
| tuni_bot_128 | soa | 17 | 3.97 | 7.46 | 142.58 | — | — |
| pfit_top_16 | flat | 2 | 5.28 | 3.33 | 26.04 | 1.09 | 1.44 (28136 no longer fit) |
| pfit_top_32 | flat | 4 | 5.23 | 2.75 | 41.07 | 0.71 | 1.14 (27892 no longer fit) |
| pfit_top_64 | flat | 8 | 1.94 | 2.72 | 74.67 | 2.29 | 1.66 (28150 no longer fit) |
| pfit_top_128 | flat | 16 | 4.05 | 6.98 | 141.96 | — | 9.66 (27990 no longer fit) |
| pfit_bot_16 | flat | 2 | 5.46 | 2.71 | 26.10 | 1.12 | — |
| pfit_bot_32 | flat | 4 | 1.73 | 2.75 | 40.85 | 0.79 | — |
| pfit_bot_64 | flat | 8 | 1.95 | 2.75 | 74.94 | 2.41 | — |
| pfit_bot_128 | flat | 16 | 3.84 | 8.02 | 143.62 | — | — |
| puni_top_16 | flat | 2 | 1.59 | 2.94 | 23.45 | 1.05 | 0.06 |
| puni_top_32 | flat | 4 | 1.96 | 2.87 | 40.69 | 0.63 | 0.14 |
| puni_top_64 | flat | 8 | 1.66 | 2.78 | 74.36 | 2.21 | 0.56 |
| puni_top_128 | flat | 16 | 3.66 | 7.82 | 139.83 | — | 1.16 |
| puni_bot_16 | flat | 2 | 1.58 | 2.74 | 23.14 | 1.06 | — |
| puni_bot_32 | flat | 4 | 1.87 | 2.69 | 41.06 | 0.74 | — |
| puni_bot_64 | flat | 8 | 1.65 | 2.92 | 74.86 | 2.38 | — |
| puni_bot_128 | flat | 16 | 3.51 | 8.08 | 141.64 | — | — |

## S2. Array representations by width

Times for $n = 100000$ elements. In bold: the fastest, and any within 10% of it (a tie). At width 64 residues are a plain int64 vector.

| Width | Operation | bi | lc | lr | rns | mpz |
|---|---|---|---|---|---|---|
| 64 | a + b | **44.1 µs** | 71.3 µs | 868 µs | **48 µs** | 1.58 ms |
| 64 | a b (inputs at W/2; residues pre-widened) | 56.2 µs | 117 µs | 518 µs | **45.8 µs** | 521 µs |
| 64 | a b including widening the inputs | 56.2 µs | 117 µs | 518 µs | **45.8 µs** | 521 µs |
| 64 | dot product (same) | 21 µs | 402 µs | 1.05 ms | **18.6 µs** | 1.16 ms |
| 64 | dot product including widening the inputs | 21 µs | 402 µs | 1.05 ms | **18.6 µs** | 1.16 ms |
| 64 | exact / 999 | **18.3 µs** | 62.9 µs | 172 µs | **17.8 µs** | 489 µs |
| 64 | exact / 1000 | **19.1 µs** | 61.6 µs | 324 µs | 550 µs | 513 µs |
| 64 | sort | 13.1 ms | **10.8 ms** | 14.9 ms | **10.8 ms** | 44.2 ms |
| 64 | bit-length scan | 6.82 µs | **5.22 µs** | **5.22 µs** | 1.02 ms | 655 µs |
| 128 | a + b | **159 µs** | 603 µs | 954 µs | 239 µs | 1.22 ms |
| 128 | a b (inputs at W/2; residues pre-widened) | **111 µs** | **120 µs** | 797 µs | 422 µs | 498 µs |
| 128 | a b including widening the inputs | **111 µs** | **120 µs** | 797 µs | 3.29 ms | 498 µs |
| 128 | dot product (same) | **51.1 µs** | 430 µs | 1.1 ms | 967 µs | 1.22 ms |
| 128 | dot product including widening the inputs | **51.1 µs** | 430 µs | 1.1 ms | 3.84 ms | 1.22 ms |
| 128 | exact / 999 | **106 µs** | **115 µs** | 268 µs | 430 µs | 557 µs |
| 128 | exact / 1000 | **118 µs** | **113 µs** | 441 µs | 2.02 ms | 581 µs |
| 128 | sort | 17 ms | 21.1 ms | 18.6 ms | **14.2 ms** | 49.9 ms |
| 128 | bit-length scan | 81.8 µs | 32.7 µs | **22.7 µs** | 2.75 ms | 643 µs |
| 128 | widen one vector W/2 to W | **58.6 µs** | 82.9 µs | 392 µs | 1.43 ms | — |
| 256 | a + b | **326 µs** | 762 µs | 1.02 ms | 405 µs | 1.26 ms |
| 256 | a b (inputs at W/2; residues pre-widened) | 290 µs | **236 µs** | 918 µs | 783 µs | 681 µs |
| 256 | a b including widening the inputs | 290 µs | **236 µs** | 918 µs | 8.73 ms | 681 µs |
| 256 | dot product (same) | **420 µs** | 627 µs | 1.22 ms | 1.92 ms | 1.78 ms |
| 256 | dot product including widening the inputs | **420 µs** | 627 µs | 1.22 ms | 9.87 ms | 1.78 ms |
| 256 | exact / 999 | 327 µs | **224 µs** | 378 µs | 801 µs | 758 µs |
| 256 | exact / 1000 | 417 µs | **293 µs** | 957 µs | 4.23 ms | 788 µs |
| 256 | sort | 27 ms | 29.5 ms | 27 ms | **18.2 ms** | 50.9 ms |
| 256 | bit-length scan | 120 µs | **94.1 µs** | **101 µs** | 5.55 ms | 614 µs |
| 256 | widen one vector W/2 to W | **169 µs** | 192 µs | 380 µs | 3.97 ms | — |
| 512 | a + b | **658 µs** | 999 µs | 1.22 ms | 755 µs | 1.41 ms |
| 512 | a b (inputs at W/2; residues pre-widened) | 1.23 ms | **976 µs** | 1.71 ms | 1.55 ms | 1.39 ms |
| 512 | a b including widening the inputs | 1.23 ms | **976 µs** | 1.71 ms | 28.4 ms | 1.39 ms |
| 512 | dot product (same) | **1.16 ms** | **1.25 ms** | 1.85 ms | 3.82 ms | 2.43 ms |
| 512 | dot product including widening the inputs | **1.16 ms** | **1.25 ms** | 1.85 ms | 30.7 ms | 2.43 ms |
| 512 | exact / 999 | 1.29 ms | **616 µs** | 830 µs | 1.5 ms | 1.3 ms |
| 512 | exact / 1000 | 1.52 ms | **696 µs** | 1.26 ms | 12.3 ms | 1.33 ms |
| 512 | sort | 28.6 ms | 39.1 ms | 36.4 ms | **22.2 ms** | 59.8 ms |
| 512 | bit-length scan | 291 µs | **215 µs** | **217 µs** | 14.7 ms | 715 µs |
| 512 | widen one vector W/2 to W | **338 µs** | **354 µs** | 454 µs | 13.4 ms | — |
| 1024 | a + b | **1.42 ms** | 1.62 ms | 1.71 ms | **1.4 ms** | 1.91 ms |
| 1024 | a b (inputs at W/2; residues pre-widened) | 4.79 ms | 4.32 ms | **3.6 ms** | **3.92 ms** | **3.84 ms** |
| 1024 | a b including widening the inputs | 4.79 ms | 4.32 ms | **3.6 ms** | 101 ms | **3.84 ms** |
| 1024 | dot product (same) | **4.47 ms** | 5.3 ms | **4.09 ms** | 8.51 ms | 7.13 ms |
| 1024 | dot product including widening the inputs | **4.47 ms** | 5.3 ms | **4.09 ms** | 105 ms | 7.13 ms |
| 1024 | exact / 999 | 6.11 ms | **1.81 ms** | 2.16 ms | 5.12 ms | 2.68 ms |
| 1024 | exact / 1000 | 6.18 ms | **1.81 ms** | 2.86 ms | 44.3 ms | 3.79 ms |
| 1024 | sort | **43.5 ms** | 54.7 ms | 51.5 ms | 50.5 ms | 77 ms |
| 1024 | bit-length scan | 725 µs | **334 µs** | **334 µs** | 52.7 ms | 738 µs |
| 1024 | widen one vector W/2 to W | **668 µs** | **704 µs** | 792 µs | 48.5 ms | — |
| 2048 | a + b | 4.12 ms | 5.19 ms | 4.17 ms | **3.41 ms** | 4.35 ms |
| 2048 | a b (inputs at W/2; residues pre-widened) | 19.2 ms | 21.2 ms | 13 ms | **7.3 ms** | 13 ms |
| 2048 | a b including widening the inputs | 19.2 ms | 21.2 ms | **13 ms** | 378 ms | **13 ms** |
| 2048 | dot product (same) | 18.7 ms | 22.6 ms | **13.5 ms** | 16.3 ms | 15.6 ms |
| 2048 | dot product including widening the inputs | 18.7 ms | 22.6 ms | **13.5 ms** | 387 ms | 15.6 ms |
| 2048 | exact / 999 | 21.8 ms | 7.66 ms | **4.8 ms** | 10.4 ms | 5.6 ms |
| 2048 | exact / 1000 | 26.6 ms | **4.91 ms** | 7.29 ms | 197 ms | 6.27 ms |
| 2048 | sort | 61 ms | 62.2 ms | **51.2 ms** | 244 ms | 86.6 ms |
| 2048 | bit-length scan | 2.04 ms | **608 µs** | **617 µs** | 221 ms | 841 µs |
| 2048 | widen one vector W/2 to W | **1.32 ms** | **1.36 ms** | **1.37 ms** | 185 ms | — |

## S3. Growth chains through every width

$x \leftarrow x b + c \cdot 1000^{k+1}$ on $n = 100000$ 3-place decimals; each step count is the largest whose proven bound fits the width. Residues widen in steps of one 62-bit prime; the others double.

| Width reached | Steps | bi | lc | lr | rns | mpz | Residue capacity at the end |
|---|---|---|---|---|---|---|---|
| 128 | 6 | **1.12 ms** | 3.02 ms | 8.21 ms | 3.57 ms | 19.3 ms | 125 bits |
| 256 | 15 | **6.34 ms** | 11.9 ms | 23.2 ms | 22 ms | 33.1 ms | 311 bits |
| 512 | 32 | **41.2 ms** | **38.8 ms** | 60 ms | 90.9 ms | 78.9 ms | 559 bits |
| 1024 | 66 | 281 ms | **150 ms** | 182 ms | 472 ms | 189 ms | 1055 bits |
| 2048 | 134 | 1.72 s | 571 ms | 610 ms | 3.27 s | **495 ms** | 2047 bits |

At 146 steps every representation stopped at the 2048-bit cap before overflowing.

## S4. Where each operation flips to the run-time side

Ratio = fastest compile-time representation (_BitInt, compile-time limbs; residues at 64 bits) / fastest run-time one (run-time limbs, residues, mpz); above 1 means the run-time side is faster. Within 10% either way counts as a tie.

| Operation | 64 | 128 | 256 | 512 | 1024 | 2048 | Flips at |
|---|---|---|---|---|---|---|---|
| a + b | 0.05 | 0.67 | 0.80 | 0.87 | 1.01 | 1.21 | 2048 |
| exact / 999 | 0.10 | 0.40 | 0.59 | 0.74 | 0.84 | 1.60 | 2048 |
| exact / 1000 | 0.06 | 0.26 | 0.37 | 0.55 | 0.63 | 0.78 | never (to 2048) |
| sort | 0.73 | 1.20 | 1.49 | 1.29 | 0.86 | 1.19 | 2048 |
| bit-length scan | 1.00 | 1.44 | 0.93 | 0.99 | 1.00 | 0.98 | never (to 2048) |
| widen one vector W/2 to W | nan | 0.15 | 0.45 | 0.74 | 0.84 | 0.96 | never (to 2048) |
| a b including widening the inputs | 0.09 | 0.22 | 0.35 | 0.70 | 1.20 | 1.48 | 1024 |
| dot product including widening the inputs | 0.02 | 0.05 | 0.34 | 0.63 | 1.09 | 1.38 | 2048 |

## S5. The widening rule

$n = 1000000$; a share of the values is near $10^{16}$, so their squares need 256 bits.

| Share wide | Operation | Whole vector | Per element | Whole / per element |
|---|---|---|---|---|
| 0.0% | (ab)^2 | 1.65 ms | 1.93 ms | 0.86 |
| 0.0% | sum of (ab)^2 | 1.91 ms | 1.84 ms | 1.04 |
| 0.1% | (ab)^2 | 4.65 ms | 1.96 ms | 2.37 |
| 0.1% | sum of (ab)^2 | 4.69 ms | 1.88 ms | 2.49 |
| 1.0% | (ab)^2 | 4.44 ms | 2.3 ms | 1.93 |
| 1.0% | sum of (ab)^2 | 4.6 ms | 2.2 ms | 2.09 |
| 10.0% | (ab)^2 | 4.41 ms | 3.37 ms | 1.31 |
| 10.0% | sum of (ab)^2 | 4.7 ms | 3.35 ms | 1.40 |
| 100.0% | (ab)^2 | 4.44 ms | 6.34 ms | 0.70 |
| 100.0% | sum of (ab)^2 | 4.79 ms | 6.57 ms | 0.73 |

## S6. Exact matrix products

Peak memory is the rise above the inputs; part of it is shared by every method (input limbs, the limb output and the copy made for the check, estimated in the last column), so the method's own working memory is roughly the difference.

| Size | Entry bits | Method | Median | Reduce | Product | Reconstruct | Primes | Peak memory | Shared part |
|---|---|---|---|---|---|---|---|---|---|
| 128 | 64 | mmg | 1.24 ms | 184 µs | 436 µs | 635 µs | 6 | +8 MB | ~2 MB |
| 128 | 64 | mmgc | 1.28 ms | 200 µs | 465 µs | 628 µs | 6 | +5 MB | ~2 MB |
| 128 | 64 | mmi | 1.09 ms | 189 µs | 268 µs | 599 µs | 3 | +4 MB | ~2 MB |
| 128 | 256 | mmg | 5.05 ms | 1.26 ms | 1.55 ms | 2.18 ms | 22 | +24 MB | ~4 MB |
| 128 | 256 | mmgc | 4.29 ms | 856 µs | 1.61 ms | 1.75 ms | 22 | +12 MB | ~4 MB |
| 128 | 256 | mmi | 3.56 ms | 913 µs | 1.01 ms | 1.58 ms | 11 | +10 MB | ~4 MB |
| 128 | 1024 | mmg | 28.7 ms | 9.27 ms | 6.45 ms | 13 ms | 86 | +86 MB | ~13 MB |
| 128 | 1024 | mmgc | 26.9 ms | 9.47 ms | 7.12 ms | 10.3 ms | 86 | +30 MB | ~13 MB |
| 128 | 1024 | mmi | 12.8 ms | 4.64 ms | 3.82 ms | 4.43 ms | 42 | +31 MB | ~13 MB |
| 128 | 2048 | mmg | 85.1 ms | 30.7 ms | 14.3 ms | 39.6 ms | 172 | +184 MB | ~25 MB |
| 128 | 2048 | mmgc | 77.3 ms | 33 ms | 13 ms | 31.2 ms | 172 | +54 MB | ~25 MB |
| 128 | 2048 | mmi | 30.9 ms | 11.8 ms | 8.37 ms | 10.8 ms | 83 | +59 MB | ~25 MB |
| 256 | 64 | mmg | 6.52 ms | 1.09 ms | 2.75 ms | 2.69 ms | 6 | +28 MB | ~6 MB |
| 256 | 64 | mmgc | 6.58 ms | 719 µs | 3.26 ms | 2.58 ms | 6 | +14 MB | ~6 MB |
| 256 | 64 | mmi | 5.14 ms | 745 µs | 2 ms | 2.41 ms | 3 | +13 MB | ~6 MB |
| 256 | 256 | mmg | 30.1 ms | 8.15 ms | 11.3 ms | 10.7 ms | 23 | +89 MB | ~15 MB |
| 256 | 256 | mmgc | 22.4 ms | 3.71 ms | 11.4 ms | 7.35 ms | 23 | +37 MB | ~15 MB |
| 256 | 256 | mmi | 17.6 ms | 3.97 ms | 7.58 ms | 6.03 ms | 11 | +30 MB | ~15 MB |
| 256 | 1024 | mmg | 171 ms | 53 ms | 49.2 ms | 68.6 ms | 88 | +327 MB | ~51 MB |
| 256 | 1024 | mmgc | 128 ms | 36.5 ms | 43.8 ms | 43.6 ms | 88 | +95 MB | ~51 MB |
| 256 | 1024 | mmi | 75.4 ms | 22.2 ms | 34.6 ms | 20.7 ms | 42 | +98 MB | ~51 MB |
| 256 | 2048 | mmg | 412 ms | 130 ms | 100 ms | 182 ms | 175 | +660 MB | ~99 MB |
| 256 | 2048 | mmgc | 355 ms | 140 ms | 86.6 ms | 127 ms | 175 | +167 MB | ~99 MB |
| 256 | 2048 | mmi | 159 ms | 47.8 ms | 61.8 ms | 47.6 ms | 83 | +189 MB | ~99 MB |
| 512 | 64 | mmg | 47.3 ms | 8.51 ms | 23.2 ms | 15.7 ms | 7 | +119 MB | ~25 MB |
| 512 | 64 | mmgc | 41.4 ms | 3.65 ms | 25.9 ms | 11.6 ms | 7 | +56 MB | ~25 MB |
| 512 | 64 | mmi | 29.7 ms | 3.81 ms | 16.6 ms | 9.38 ms | 3 | +47 MB | ~25 MB |
| 512 | 256 | mmg | 187 ms | 45.1 ms | 84.4 ms | 56.8 ms | 23 | +353 MB | ~61 MB |
| 512 | 256 | mmgc | 129 ms | 13.7 ms | 84.2 ms | 30.3 ms | 23 | +141 MB | ~61 MB |
| 512 | 256 | mmi | 115 ms | 17.3 ms | 71.2 ms | 25.6 ms | 11 | +115 MB | ~61 MB |
| 512 | 1024 | mmg | 848 ms | 212 ms | 342 ms | 293 ms | 90 | +1322 MB | ~205 MB |
| 512 | 1024 | mmgc | 704 ms | 170 ms | 347 ms | 191 ms | 90 | +372 MB | ~205 MB |
| 512 | 1024 | mmi | 403 ms | 81.8 ms | 243 ms | 77.9 ms | 42 | +385 MB | ~205 MB |
| 512 | 2048 | mmg | 1.91 s | 512 ms | 654 ms | 727 ms | 179 | +2624 MB | ~397 MB |
| 512 | 2048 | mmgc | 1.85 s | 616 ms | 702 ms | 554 ms | 179 | +655 MB | ~397 MB |
| 512 | 2048 | mmi | 896 ms | 208 ms | 470 ms | 185 ms | 83 | +742 MB | ~397 MB |
