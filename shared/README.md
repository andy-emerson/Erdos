# Shared harness tools

Generic tools used by more than one specification's `test/`, and measurements that belong to more than one specification. The tools are not specific to a number format; the partial-sharing baseline is the joint run of specifications 08 and 09.

- `tangle.py`: extracts the code from a literate benchmark project (Markdown chapters in `PROJECT/lit/`) into `PROJECT/build/` and `PROJECT/cbench/`. Usage: `python3 shared/tangle.py PROJECT` (default: the current directory). Used by [`01-number-system/test`](../01-number-system/test/) and [`03-q/test/q-bench`](../03-q/test/q-bench/).
- `runtests.py`: runs every `test_*` function in the named modules; run it from a harness's `build/` directory. Used by [`03-q/test/q-research`](../03-q/test/q-research/) and [`04-division-algebra-family/test`](../04-division-algebra-family/test/).
- `checklinks.py`: checks that every relative Markdown link and anchor in the repository resolves. Usage: `python3 shared/checklinks.py . [FOLDER_TO_SKIP ...]`.
- [`partial-sharing-baseline/`](partial-sharing-baseline/): the joint measurement of Designs 1 and 2 ([`08-common-divisors`](../08-common-divisors/), [`09-coprime-base`](../09-coprime-base/)) against one control: the shared integer module, the control and the ceiling, the harness, the fixed rule and the measured results (concluded).

The timing harness, data generators, metadata, verification and precision helpers that later specs reuse are part of the literate benchmarks they were written in (Exact Number System v0.1 and Q v0.3), so they stay there; later specs put those builds on the import path with their own `build/reuse.py`.
