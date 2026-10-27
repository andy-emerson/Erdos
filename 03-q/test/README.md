# Q benchmarks and research code

Two projects, both for [Q v0.3](../v0.3.md):

| Project | What | Results |
| --- | --- | --- |
| [`q-bench`](q-bench/) | the measurements in the spec's Evidence section, as a literate program (chapters in `q-bench/lit/`, starting with [`00-overview.md`](q-bench/lit/00-overview.md)) | `q-bench/results/q.md` (the run reported in the spec) |
| [`q-research`](q-research/) | the spec's research findings (LCF order keys, quote notation) and the [plane variant](../research/plane-variant.md) | `q-research/results/research.md` |

The benchmark code was first published with v0.3; the runs behind v0.1 and v0.2 are not in this repository.

## Running

`q-bench` reuses the Exact Number System benchmark ([`01-number-system/test`](../../01-number-system/test/)), which must be tangled first. From this directory:

```
apt-get install libgmp-dev libflint-dev          # for the compiled GMP and FLINT baselines
pip install numpy numba python-flint gmpy2 scipy threadpoolctl
python3 ../../shared/tangle.py ../../01-number-system/test
python3 ../../shared/tangle.py q-bench
cd q-bench
python3 build/run_bench.py --quick                # a few minutes
python3 build/run_bench.py                        # about an hour
```

The runner executes the correctness tests first and stops if any fails, and checks every exact result against an independent exact computation before reporting it. Results are written to `q-bench/results/`. For `q-research`, see its [README](q-research/README.md).
