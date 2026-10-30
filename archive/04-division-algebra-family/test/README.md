# Division-algebra family: research code

Code and results for the research notes in [`../research`](../research/). Everything here is checked against an exact computation before it is reported, and the tests confirm each check catches a deliberately wrong result.

Some harnesses reuse the Exact Number System benchmark ([`01-number-system/test`](../../01-number-system/test/)) and the Q benchmark ([`03-q/test/q-bench`](../../03-q/test/q-bench/)) through `build/reuse.py`; tangle both first (see [`03-q/test/README.md`](../../03-q/test/README.md)). Tests run with the shared runner, from `build/`. The C programs in `cbench/` are compiled on first use, by each runner's `build()` (the harnesses call these); a test that reports a missing binary in `cbench/` needs its harness run once with `--quick`, or the runner's `build()` called, for example `python3 -c "import quat_run; quat_run.build()"`.

## Requirements

- GMP and FLINT development headers; clang 18 or later (for `_BitInt` in `un.c` and `p1_arith.c`).
- **FLINT 3.0.1 built with BLAS** for the matrix products and the library comparison. `build/build_flint_blas.sh [DIR]` builds it, with OpenBLAS from the scipy-openblas32 wheel, into `DIR` (default `/home/claude/flintblas`); set `FLINT_BLAS_ROOT` to DIR if it is elsewhere. The phase-two results used a rebuild with `CFLAGS="-O3 -march=native"` installed to `DIR/inst-native`; set `FLINT_ROOT` to point elsewhere.
- `p1_gemm.c`'s `mmi` method needs AVX-512 IFMA.

## The family ([note](../research/division-algebra-family.md); prior art in [`prior-art-family.md`](../research/prior-art-family.md))

Unit tables and the header with borrowed slash bits (`family.py`), fixed-width uN vector tiers to 2,048 bits (`cbench/un.c`), quaternion arrays (`cbench/quat.c`), octonion arrays (`cbench/oct.c`, with the product written out by `gen_oct_table.py`), the uniform header (`uniform.py`, decoder timing in `cbench/decode.c`), and exact wide-entry matrix products (`cbench/gemm.c`, multimodular OpenBLAS `dgemm` against FLINT). Results: `results/family.md`, `results/family.json`.

```
cd build
python3 ../../../shared/runtests.py test_family
python3 family_bench.py --quick
python3 family_bench.py
```

## Phase one: the variants compared with each other ([note](../research/phase1-variants.md))

Value layouts (S1: `gen_p1_layouts.py`, `cbench/p1_layout.c`), array representations by width and growth chains (S2–S4: `cbench/p1_arith.c`), the widening rule (S5: `cbench/p1_widen.c`) and exact matrix products (S6: `cbench/p1_gemm.c`). Results: `results/phase1.md`, `results/phase1.json`.

```
cd build
python3 ../../../shared/runtests.py test_phase1 test_p1_gemm
python3 phase1_bench.py --quick
python3 phase1_bench.py
```

## The threshold, storage granularity and the library comparison ([note](../research/threshold-storage-libraries.md))

The threshold test at 1024 bits (`threshold_bench.py`, `cbench/p1_thresh.c`), storage granularity (`storage_bench.py`, `cbench/p1_storage.c`), and the format against GMP and FLINT (phase two: `p2_bench.py`, `cbench/p2_elem.c`, `cbench/p2_scalar.c`, `cbench/p2_linalg.c`). Results: `results/threshold.*`, `results/storage.*`, `results/phase2.*`.

```
cd build
python3 ../../../shared/runtests.py test_phase1 test_p1_storage test_p2_linalg
python3 threshold_bench.py --quick && python3 threshold_bench.py
python3 storage_bench.py --quick && python3 storage_bench.py
python3 p2_bench.py --quick && python3 p2_bench.py
```
