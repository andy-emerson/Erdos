# The Q family, v0.3: measurements

## The question

This project measures the Q family of exact number formats, as specified in v0.3, against the tools people who need exact arithmetic actually use: GMP and FLINT called from C, the same libraries and Python's `fractions` and `decimal` called from Python, and integers over a shared denominator (GMP, FLINT, and int64 with and without overflow checks). Rounding formats are not contenders; NumPy float64 appears once, for context. Workloads include the vector form's best case, decimal columns with one denominator, and cases that do not favour it: unrelated denominators, growing denominators, and a few wide values in an otherwise narrow vector.

## The family in brief

- **Q2** is the Gaussian unit: a sign bit and an imaginary bit, read together as a quarter-turn count ($00 = 1$, $01 = i$, $10 = -1$, $11 = -i$). Stored alone it takes a byte.
- **Q72** is a byte and an unmodified U64. The byte's top two bits are a Q2; its low six bits are the slash $s$. The numerator is the top $s+1$ bits of the U64, and the other $63-s$ bits hold the denominator minus one. Every pattern is a value; zero is a numerator of 0; there are no infinities, and division by zero is an error.
- The ladder is Q24, Q40, Q72 and Q144, then arbitrary precision. Values that do not fit are promoted, never rounded. Only Q72 is implemented here.

## What is measured

Arrays use the **vector form**: one shared denominator per vector. v0.2 asks which layout of it is fastest, and builds three:

- **turn bytes**: a byte per element holding the turn, and a U64 magnitude (the v0.1 layout);
- **packed turns**: the turns packed four to a byte, and a U64 magnitude;
- **signed**: a two's complement int64 per element, with the axis (real or imaginary) stored once per vector, so a complex array is two vectors.

Turn bytes and signed are complete implementations (every operation, three magnitude tiers, complex columns). All three layouts also get fused kernels that do identical work with the same overflow proof, so the layout itself can be timed in isolation. The section also times exact division by a common factor two ways: with the divide instruction, and by multiplying by a 2-adic inverse, the idea behind Hehner and Horspool's quote notation.

## Reading order

| File | Contents |
|---|---|
| `00-overview.md` | This file |
| `01-reuse.md` | What is imported from the earlier benchmarks |
| `02-q.md` | Q2, the Q72 encoding, and exact scalar arithmetic |
| `03-turn-bytes.md` | The turn-byte vector form: kernels and vector |
| `04-signed.md` | The signed vector form: kernels and vector |
| `05-complex.md` | Complex columns over either vector form |
| `06-layouts.md` | The layout experiment |
| `07-gemm.md` | An exact matrix product shaped for the vector form |
| `08-precision.md` | Precision and exactness measurements |
| `09-tests.md` | Correctness tests |
| `10-benchmarks.md` | The benchmarks |
| `11-compiled.md` | Compiled baselines: GMP and FLINT from C |

## Building and running

The v0.1 project (`01-number-system/test` in this repository) must be tangled first. From this directory:

```
apt-get install libgmp-dev libflint-dev     # for the compiled baselines
pip install numpy numba python-flint gmpy2 scipy threadpoolctl
python3 ../../../shared/tangle.py ../../../01-number-system/test
python3 ../../../shared/tangle.py
python3 build/run_bench.py --quick
python3 build/run_bench.py
```

The runner executes every correctness test first and stops if any fails. Every exact result is checked against an independent exact computation (every element, for elementwise operations) before it is reported. Results go to `results/q.md` and `results/q.json`.
