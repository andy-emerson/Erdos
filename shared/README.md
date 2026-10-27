# Shared harness tools

Generic tools used by more than one specification's `test/`. Nothing here is specific to a number format.

- `tangle.py`: extracts the code from a literate benchmark project (Markdown chapters in `PROJECT/lit/`) into `PROJECT/build/` and `PROJECT/cbench/`. Usage: `python3 shared/tangle.py PROJECT` (default: the current directory). Used by [`01-number-system/test`](../01-number-system/test/) and [`03-q/test/q-bench`](../03-q/test/q-bench/).
- `runtests.py`: runs every `test_*` function in the named modules; run it from a harness's `build/` directory. Used by [`03-q/test/q-research`](../03-q/test/q-research/) and [`04-division-algebra-family/test`](../04-division-algebra-family/test/).

The timing harness, data generators, metadata, verification and precision helpers that later specs reuse are part of the literate benchmarks they were written in (Exact Number System v0.1 and Q v0.3), so they stay there; later specs put those builds on the import path with their own `build/reuse.py`.
