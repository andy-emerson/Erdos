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
