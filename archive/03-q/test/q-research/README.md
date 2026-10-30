# Q research measurements

Code and results for the research findings in [Q v0.3](../../v0.3.md) and the [plane variant note](../../research/plane-variant.md): LCF order keys (`lcf.py`, `lcfkern.py`, `lcf_bound.py`, `lcf_width.py`), quote notation's 2-adic arithmetic (`cbench/hensel.c`), and the plane variant (`plane.py`, `plane_format.py`, `cbench/cplx.c`).

It reuses `../q-bench/build` and `01-number-system/test/build`, which must be tangled first (see [`../README.md`](../README.md)). Then:

```
cd build
python3 -c "import research_bench; research_bench.build()"   # compiles cbench/hensel and cbench/cplx
python3 ../../../../shared/runtests.py test_research test_plane
python3 research_bench.py --quick
python3 research_bench.py
```

The C programs are compiled by `research_bench.py` on first use (`gcc -O3 -march=native`; GMP and FLINT headers required); the tests need them compiled. Every result is checked against an exact computation before it is reported. Results are written to `results/research.md` and `results/research.json`.
