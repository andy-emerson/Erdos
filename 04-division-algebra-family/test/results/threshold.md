# The threshold test: 1024 bits, the complete small set against the complete large set

- **date**: 2026-10-27 07:04:01 CDT
- **cpu**: Intel(R) Xeon(R) Processor
- **cores_visible**: 2
- **python**: 3.13.17
- **numpy**: 2.5.3
- **numba**: 0.68.0
- **python-flint**: 0.9.0
- **platform**: Linux-6.18.44-fc-v165-x86_64-with-glibc2.39
- **native thread pools**: openblas 1 thread(s)
- **flint threads**: 1

### out of cache, n = 100000, values filling the width (1012 bits)

Ratio = small set / large set time (above 1: the large set is faster): geometric mean over 12 rounds, 95% bootstrap interval. Band for a verdict: ±5.0% (the larger of 5% and the null control, the small set timed twice: 1.003, 0.994–1.013). Kernels were chosen in a separate pilot.

| Operation | Small set (kernel) | Large set (kernel) | Ratio | 95% interval | Verdict |
|---|---|---|---|---|---|
| a + b | 1.51 ms (C) | 1.53 ms (GMP) | 0.975 | 0.951–0.997 | tie |
| a b (512-bit inputs) | 3.69 ms (GMP) | 3.61 ms (GMP) | 1.012 | 0.988–1.033 | tie |
| dot product | 4.43 ms (GMP) | 4.6 ms (GMP) | 0.936 | 0.850–1.006 | tie |
| exact / 999 | 1.81 ms (C, no unrolling) | 1.79 ms (C, no unrolling) | 1.021 | 0.982–1.067 | tie |
| exact / 1000 | 1.98 ms (C) | 2.28 ms (C) | 0.875 | 0.846–0.911 | small |
| sort | 51.8 ms (GMP) | 52 ms (C, no unrolling) | 0.996 | 0.988–1.003 | tie |
| bit-length scan | 341 µs (C) | 345 µs (C) | 0.984 | 0.967–1.002 | tie |
| widen 512 to 1024 | 927 µs (C, no unrolling) | 958 µs (GMP) | 0.954 | 0.934–0.971 | tie |
| chain steps at 1024 bits (30) | 67.3 ms (GMP) | 69.4 ms (GMP) | 0.975 | 0.965–0.984 | tie |
| **all operations, equal weights** | | | 0.969 | 0.953–0.982 | **tie** |
| **pipeline (mul, add, divide, add, divide, add, step, add, sort, scan)** | 71.4 ms | 74.1 ms | 0.964 | 0.912–1.023 | **tie** |

### out of cache, n = 100000, values spread over 513-1012 bits

Ratio = small set / large set time (above 1: the large set is faster): geometric mean over 12 rounds, 95% bootstrap interval. Band for a verdict: ±5.0% (the larger of 5% and the null control, the small set timed twice: 0.996, 0.986–1.006). Kernels were chosen in a separate pilot.

| Operation | Small set (kernel) | Large set (kernel) | Ratio | 95% interval | Verdict |
|---|---|---|---|---|---|
| a + b | 1.78 ms (GMP) | 2.11 ms (C) | 0.862 | 0.846–0.879 | small |
| a b (512-bit inputs) | 3.65 ms (GMP) | 3.66 ms (GMP) | 0.996 | 0.983–1.009 | tie |
| dot product | 4.38 ms (GMP) | 4.32 ms (GMP) | 1.005 | 0.980–1.033 | tie |
| exact / 999 | 1.78 ms (C, no unrolling) | 1.77 ms (C, no unrolling) | 0.987 | 0.941–1.029 | tie |
| exact / 1000 | 1.99 ms (C) | 2.26 ms (C) | 0.889 | 0.850–0.934 | small |
| sort | 43 ms (GMP) | 42.5 ms (C) | 1.003 | 0.998–1.008 | tie |
| bit-length scan | 352 µs (C) | 350 µs (C, no unrolling) | 0.984 | 0.957–1.008 | tie |
| widen 512 to 1024 | 944 µs (C, no unrolling) | 984 µs (C, no unrolling) | 0.953 | 0.931–0.976 | tie |
| chain steps at 1024 bits (30) | 67.7 ms (GMP) | 69.5 ms (GMP) | 0.982 | 0.972–0.994 | tie |
| **all operations, equal weights** | | | 0.961 | 0.952–0.969 | **tie** |
| **pipeline (mul, add, divide, add, divide, add, step, add, sort, scan)** | 66.6 ms | 69.2 ms | 0.959 | 0.941–0.985 | **tie** |

### in cache, n = 3000, values filling the width (1012 bits)

Ratio = small set / large set time (above 1: the large set is faster): geometric mean over 12 rounds, 95% bootstrap interval. Band for a verdict: ±5.0% (the larger of 5% and the null control, the small set timed twice: 1.011, 0.999–1.027). Kernels were chosen in a separate pilot. Each timed sample repeats the operation (inner counts: {'add': 60, 'mul': 20, 'dot': 17, 'div1': 34, 'div2': 31, 'sort': 8, 'scan': 1195, 'widen': 134}).

| Operation | Small set (kernel) | Large set (kernel) | Ratio | 95% interval | Verdict |
|---|---|---|---|---|---|
| a + b | 28.1 µs (C) | 28.5 µs (GMP) | 0.970 | 0.937–1.001 | tie |
| a b (512-bit inputs) | 105 µs (GMP) | 104 µs (GMP) | 1.040 | 0.982–1.131 | tie |
| dot product | 117 µs (GMP) | 120 µs (GMP) | 0.983 | 0.971–0.995 | tie |
| exact / 999 | 46.1 µs (C, no unrolling) | 46 µs (C) | 1.001 | 0.993–1.011 | tie |
| exact / 1000 | 52.1 µs (C) | 61.5 µs (C) | 0.852 | 0.842–0.863 | small |
| sort | 258 µs (GMP) | 258 µs (GMP) | 0.997 | 0.984–1.009 | tie |
| bit-length scan | 1.69 µs (C, no unrolling) | 1.65 µs (C) | 1.026 | 1.018–1.036 | tie |
| widen 512 to 1024 | 9.59 µs (C) | 14.4 µs (GMP) | 0.693 | 0.633–0.798 | small |
| chain steps at 1024 bits (30) | 1.97 ms (GMP) | 2.03 ms (GMP) | 0.955 | 0.926–0.976 | tie |
| **all operations, equal weights** | | | 0.940 | 0.926–0.956 | **tie** |
| **pipeline (mul, add, divide, add, divide, add, step, add, sort, scan)** | 719 µs | 754 µs | 1.009 | 0.936–1.142 | **tie** |

### in cache, n = 3000, values spread over 513-1012 bits

Ratio = small set / large set time (above 1: the large set is faster): geometric mean over 12 rounds, 95% bootstrap interval. Band for a verdict: ±5.0% (the larger of 5% and the null control, the small set timed twice: 0.987, 0.977–0.995). Kernels were chosen in a separate pilot. Each timed sample repeats the operation (inner counts: {'add': 44, 'mul': 19, 'dot': 17, 'div1': 33, 'div2': 32, 'sort': 6, 'scan': 1192, 'widen': 127}).

| Operation | Small set (kernel) | Large set (kernel) | Ratio | 95% interval | Verdict |
|---|---|---|---|---|---|
| a + b | 23.2 µs (C) | 35.9 µs (GMP) | 0.671 | 0.635–0.715 | small |
| a b (512-bit inputs) | 103 µs (GMP) | 104 µs (GMP) | 0.999 | 0.988–1.011 | tie |
| dot product | 117 µs (GMP) | 120 µs (GMP) | 0.973 | 0.964–0.982 | tie |
| exact / 999 | 46.1 µs (C, no unrolling) | 45.7 µs (C, no unrolling) | 1.012 | 1.003–1.022 | tie |
| exact / 1000 | 52 µs (C) | 61.2 µs (C) | 0.874 | 0.847–0.914 | small |
| sort | 340 µs (GMP) | 340 µs (GMP) | 0.997 | 0.990–1.003 | tie |
| bit-length scan | 1.69 µs (GMP) | 1.65 µs (GMP) | 1.037 | 1.024–1.052 | tie |
| widen 512 to 1024 | 9.51 µs (GMP) | 14.4 µs (GMP) | 0.669 | 0.659–0.684 | small |
| chain steps at 1024 bits (30) | 1.98 ms (GMP) | 2.04 ms (GMP) | 0.925 | 0.845–0.973 | tie |
| **all operations, equal weights** | | | 0.895 | 0.885–0.906 | **small** |
| **pipeline (mul, add, divide, add, divide, add, step, add, sort, scan)** | 822 µs | 837 µs | 0.972 | 0.939–1.000 | **tie** |

## Decision

The rule, fixed before the run: each regime and input set gives two verdicts (the equal-weight mean and the pipeline); a side wins a verdict only if its interval lies wholly beyond the band; 1024 belongs to a side if that side wins at least half of the verdicts and the other none; otherwise it is a tie.

Verdicts: tie, tie, tie, tie, tie, tie, small, tie. **A tie: at 1024 the two sets perform alike, so the threshold can be stated either way. For the phase-two runs the out-of-cache point estimate (0.963) puts 1024 with the small routines.**
