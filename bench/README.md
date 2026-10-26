# Benchmarks

Literate benchmark sources for the specifications in [`docs/spec`](../docs/spec). Each project is a set of Markdown chapters in `lit/`; `tangle.py` extracts the code from them into `build/` (and, for the compiled baselines, `cbench/`).

| Project | For | Results |
| --- | --- | --- |
| [`exact-bench`](exact-bench) | [Exact Number System v0.1](../docs/spec/number-system-v0.1.md); its harness, data generators and solver chain are reused by `q-bench` | `exact-bench/results/` |
| [`q-bench`](q-bench) | [Q v0.3](../docs/spec/q-v0.3.md) | `q-bench/results/q.md` (the run reported in the spec) |
| [`q-research`](q-research) | the research findings in [Q v0.3](../docs/spec/q-v0.3.md) | `q-research/results/research.md` |

## Running

```
apt-get install libgmp-dev libflint-dev          # for the compiled GMP and FLINT baselines
pip install numpy numba python-flint gmpy2 scipy threadpoolctl
cd bench/exact-bench && python3 tangle.py && cd ../q-bench && python3 tangle.py
python3 build/run_bench.py --quick                # a few minutes
python3 build/run_bench.py                        # about an hour
```

`q-bench` expects `exact-bench` beside it, tangled. The runner executes the correctness tests first and stops if any fails, and checks every exact result against an independent exact computation before reporting it. Results are written to `results/`.
