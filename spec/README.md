# 11-matrix

The current specification: exact rational matrices. It compares a control (each cell a 64-bit numerator and denominator in lowest terms) with models that store each matrix's distinct whole numbers once, in a packed list that the cells refer to.

- Specs: `v0.1.md` (round m1) to `v0.4.md` (round m4), each amending the one before.
- `research/`: the measurement rules and research notes behind each round.
- `lib/`: the models' code. `test/`: the harness, tests and results of each round.

When this work is finished, this folder moves to `archive/11-matrix/`.
