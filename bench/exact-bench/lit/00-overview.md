# Exact arithmetic for numerical computing: a feasibility benchmark

## The question

The proposed language replaces binary floating point with exact arithmetic: rationals by default, symbolic algebraic values where needed, stored in typed columns of a Q-style table. The precision argument is easy to make. The performance argument is not. This project asks one question before any language is built:

> On the operations numerical users run every day, how close can exact arithmetic come to the speed of binary64 floats, and what does it buy in accuracy?

The answer is measured, not argued. Every benchmark pits an exact implementation against NumPy on the same data, on the same machine, interleaved in time, and reports speed, error, and memory side by side.

## What is being tested

Four ideas, each in its own chapter:

1. **Q64, a bounded rational** (Chapter 3). A numerator and a denominator in 64-bit words. Exact whenever the result fits. Otherwise rounded to the provably nearest representable rational using continued fractions, and reduced lazily, only when a bound is hit.
2. **Shared-denominator columns** (Chapter 4). A column of measured decimals $p_i / 10^k$ is stored as integers $p_i$ and one denominator. It costs 8 bytes per value, like a float, and column arithmetic becomes vectorised integer arithmetic.
3. **A fixed-width symbolic type** (Chapter 5). Columns over the number field $\mathbb{Q}(\sqrt2)$, where $\sqrt2 \cdot \sqrt2 = 2$ exactly, as a first instance of keeping algebraic values symbolic without breaking the uniform-column rule.
4. **Exact linear algebra** (Chapter 6). FLINT's multimodular and $p$-adic (Dixon) algorithms, used as they come.

## What is not being tested

- **The language itself.** Python, Numba and FLINT stand in for the eventual compiler and runtime. Numba compiles the column kernels through LLVM, which is roughly what an ahead-of-time compiler for the language would do, so those timings are a fair preview. The scalar recurrence tests run in the Python interpreter, and their absolute times are not.
- **Multithreading.** Everything is pinned to one thread (Section 6.1).
- **Transcendental functions.** $\exp$, $\log$ and trigonometric functions leave $\mathbb{Q}$ immediately, and they deserve their own study.

## Reading order

| File | Contents |
|---|---|
| `01-harness.md` | Timing statistics, error measurement, memory accounting |
| `02-data.md` | How the test data is generated, and why it is fair |
| `03-bounded-rational.md` | The Q64 type and its rounding rule, with a proof |
| `04-columns.md` | Column types and kernels: sum, variance, multiply–add, group-by |
| `05-quadratic-field.md` | Exact columns over $\mathbb{Q}(\sqrt2)$ |
| `06-benchmarks.md` | The ten benchmarks, their targets, and their checks |
| `07-speedups.md` | Four speed-up variants for linear algebra and the 64-bit cliff, each tested on its own |
| `08-number-formats.md` | Q64 versus an unsigned-denominator variant and a floating-slash format |
| `09-dixon-consistency.md` | Whether Dixon-on-BLAS beats FLINT consistently, across 30 cases |
| `10-tagged-complex.md` | Complex numbers whose parts carry a real-or-imaginary tag, like a second sign |
| `11-factored-25.md` | Keeping the powers of 2 and 5 as explicit exponents (tested, then dropped) |
| `12-integrated-core.md` | All accepted ideas combined into one library, benchmarked end to end against NumPy |

## Building and running

The Markdown files are the source. Code lives in named chunks inside them, and `tangle.py` assembles the chunks into runnable modules under `build/`. Never edit `build/` by hand.

```
pip install numpy numba python-flint scipy threadpoolctl
python3 tangle.py
python3 build/run_benchmarks.py --quick    # about a minute: checks the pipeline
python3 build/run_benchmarks.py            # the full run
python3 build/run_speedups.py              # the Chapter 7 variants (needs scipy)
python3 build/run_formats.py               # the Chapter 8 formats
python3 build/run_dixon_check.py           # the Chapter 9 consistency check
python3 build/run_tagged_complex.py        # the Chapter 10 complex prototype
python3 build/run_factored25.py            # the Chapter 11 experiment
python3 build/run_integrated.py            # the Chapter 12 integrated benchmarks
```

The runner executes every correctness test first and refuses to benchmark if any fails. Results go to `results/results.md`, a table with the machine's metadata, and `results/results.json`, every raw timing for reanalysis.

To check the conclusions on your own hardware, run the full benchmark there. Absolute times will differ. The ratios are the meaningful part.

## References

- D. W. Matula and P. Kornerup, "Foundations of Finite Precision Rational Arithmetic," *Computing Supplementum* 2, Springer, 1980, pp. 85–111. The fixed-slash and floating-slash systems and rounding by best rational approximation.
- P. Kornerup and D. W. Matula, *Finite Precision Number Systems and Arithmetic*, Cambridge University Press, 2010, chapter 9. The consolidated treatment.
- J. D. Dixon, "Exact solution of linear equations using p-adic expansions," *Numerische Mathematik* 40, 1982, pp. 137–141.
- The FLINT library, `fmpz_mat` and `fmpq_mat` modules, version 3.x, via python-flint 0.9. Its `fmpz_mat_solve` dispatches to Dixon lifting for a single right-hand side when $n > 15$.
- The `pyperf` documentation, on preferring the median and median absolute deviation to the mean and standard deviation for unstable timings.
- Python's `fractions.Fraction.limit_denominator`, whose source documents the convergent and semiconvergent characterisation of best approximations.
