# Q research measurements

Code and results for the research findings in [Q v0.3](../../docs/spec/q-v0.3.md): LCF order keys (`lcf.py`, `lcfkern.py`, `lcf_bound.py`), quote notation's 2-adic arithmetic (`cbench/hensel.c`), and the plane variant (`plane.py`, `plane_format.py`, `cbench/cplx.c`).

It reuses `../q-bench/build` and `../exact-bench/build`, which must be tangled first (see [`../README.md`](../README.md)). Then:

```
cd build
python3 runtests.py test_research test_plane
python3 research_bench.py --quick
python3 research_bench.py
```

The C programs are compiled on first use (`gcc -O3 -march=native`; GMP and FLINT headers required). Every result is checked against an exact computation before it is reported. Results are written to `results/research.md` and `results/research.json`.

## The division-algebra family

Code and results for the [family research note](../../docs/research/division-algebra-family.md): unit tables and the header with borrowed slash bits (`family.py`, tested in `test_family.py`), fixed-width uN vector tiers (`cbench/un.c`, compiled with clang for `_BitInt`), quaternion arrays (`cbench/quat.c`), and the harness `family_bench.py`. Prior-art notes are in `prior-art-family.md`.

```
cd build
python3 runtests.py test_family
python3 family_bench.py --quick
python3 family_bench.py
```

Results are written to `results/family.md` and `results/family.json`.

The second round adds:
- **uN tiers to 2,048 bits:** `_BitInt` and fixed limb arrays on GMP's `mpn` routines, plus ready-made 2,048-bit vectors.
- **Octonion arrays:** `cbench/oct.c`, with the product written out by `gen_oct_table.py`.
- **The uniform header:** `uniform.py`, and the decoder timing in `cbench/decode.c`.
- **Exact wide-entry matrix products:** `cbench/gemm.c`, multimodular OpenBLAS `dgemm` against FLINT.

Section G needs FLINT 3.0.1 built with BLAS. `build_flint_blas.sh [DIR]` builds it, with OpenBLAS from the scipy-openblas32 wheel; set `FLINT_BLAS_ROOT` to DIR if it is not `/home/claude/flintblas`. `un.c` needs clang 18 or later for `_BitInt`.

## Phase one: the variants compared with each other

Code and results for the [phase one note](../../docs/research/phase1-variants.md). It covers:
- **Value layouts (S1):** `gen_p1_layouts.py`, `cbench/p1_layout.c`.
- **Array representations by width and growth chains (S2–S4):** `cbench/p1_arith.c`, compiled with clang for `_BitInt`.
- **The widening rule (S5):** `cbench/p1_widen.c`.
- **Exact matrix products (S6):** `cbench/p1_gemm.c`, built against OpenBLAS from `build_flint_blas.sh`. Its `mmi` method needs AVX-512 IFMA.

```
cd build
python3 runtests.py test_phase1 test_p1_gemm
python3 phase1_bench.py --quick
python3 phase1_bench.py
```

Results are written to `results/phase1.md` and `results/phase1.json`.
