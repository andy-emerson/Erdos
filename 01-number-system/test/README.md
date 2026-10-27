# Exact Number System benchmark

The benchmark behind [the Exact Number System v0.1](../v0.1.md) (the `exact-bench` suite, Chapters 1–12 in the spec). It is a literate program: the source is the Markdown chapters in [`lit/`](lit/) (start with [`00-overview.md`](lit/00-overview.md)), and the repository's tangler writes the code into `build/`. Never edit `build/` by hand. Results of the reported runs are in [`results/`](results/).

Its harness, data generators and solver chain are reused by the Q benchmark ([`03-q/test`](../../03-q/test/)) and the division-algebra research ([`04-division-algebra-family/test`](../../04-division-algebra-family/test/)), so tangle this one first.

## Running

From this directory:

```
pip install numpy numba python-flint scipy threadpoolctl
python3 ../../shared/tangle.py
python3 build/run_benchmarks.py --quick    # about a minute: checks the pipeline
python3 build/run_benchmarks.py            # the full run
```

The other runners (`run_speedups.py`, `run_formats.py`, `run_dixon_check.py`, `run_tagged_complex.py`, `run_factored25.py`, `run_integrated.py`) are listed in the overview. Every runner executes the correctness tests first and stops if any fails. Results are written to `results/`.
