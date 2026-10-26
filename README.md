# Erdos
## Documents

- [Exact Number System Specification v0.1](docs/spec/number-system-v0.1.md): what a number is in the language, how it is stored, how arithmetic behaves, and how linear systems are solved exactly.
- [Unit–Natural Number System Specification v0.1](docs/spec/unit-natural-v0.1.md): an alternative design that builds numbers from a 64-bit Natural and a one-byte Unit (the complex units, with infinity), benchmarked against NumPy.
- [Q: Gaussian-Unit Rational Formats, Specification v0.3](docs/spec/q-v0.3.md): the Q family (Q2, Q24, Q40, Q72, Q144) of exact rationals on the real or imaginary axis, with a shared-denominator vector form for arrays on one scale, measured against GMP and FLINT from C, checked int64, and workloads where it does not win.
- [Q: Gaussian-Unit Rational Formats, Specification v0.2](docs/spec/q-v0.2.md): the Q family (Q2, Q24, Q40, Q72, Q144) of exact rationals on the real or imaginary axis, with a normative two's complement vector form, measured against GMP, FLINT and Python's exact types. Superseded by v0.3.
- [Q72: Gaussian-Unit Rational Format Specification v0.1](docs/spec/q72-v0.1.md): a data-type standard for exact axis-aligned complex rationals, a one-byte Gaussian unit with an unmodified 64-bit word, with its ladder of widths and vector form, measured against NumPy, GMP and FLINT. Superseded by v0.2.

## Benchmarks

- [`bench/`](bench/): literate benchmark sources and results for the specifications.
