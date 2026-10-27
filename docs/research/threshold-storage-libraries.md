# The threshold, storage granularity, and the format against GMP and FLINT

Oct 27, 2026 · Andrew Emerson

This note reports the research agreed after [phase one](phase1-variants.md):
1. **The threshold test:** at exactly 1024 bits, the complete set of small routines against the complete set of large ones, to settle whether 1024 is the last small size or the first large one.
2. **Storage granularity:** how vectors are stored between operations, in powers of two, multiples of 64 bits or multiples of 8 bits.
3. **The library comparison:** the format, built as settled, against GMP and FLINT, one table per width, rows by operation.

The code is in [`bench/q-research`](../../bench/q-research/) (`threshold_bench.py` with `p1_thresh.c`; `storage_bench.py` with `p1_storage.c`; `p2_bench.py` with `p2_elem.c`, `p2_scalar.c` and `p2_linalg.c`). The results are in `results/threshold.md`, `results/storage.md` and `results/phase2.md`.

Every timed result was checked:
- against Python integers or fractions;
- for large matrix products, against FLINT and a random-vector (Freivalds) check;
- for linear solves, by substituting the answer back.

Tests confirm each check catches a deliberately wrong result. Independent reviews came before each measured run, five in all, and every finding was fixed or is disclosed below. The most consequential were:
- **Assembly mistaken for compile-time gains:** an earlier threshold test credited the large set with what was really GMP's assembly against plain C.
- **Untimed setup:** the format's prime tables were left out of its timing in the matrix product and the solve.
- **FLINT built weaker:** FLINT was compiled without native flags; it was rebuilt with `-O3 -march=native` and BLAS.
- **Free layout switching:** the format was allowed to switch number layouts between operations at no cost.

One thread on one machine (2-core Xeon with AVX-512 and IFMA).

## 1. The threshold test

**Setup.** At 1024 bits, two complete sets of routines on the same sign-and-magnitude layout:
- **The small set:** the vector's length (16 limbs) is a compile-time constant.
- **The large set:** the length is read at run time, opaque to the compiler.

Each set may use hand-written C loops (with or without the compiler's unrolling, which helped some loops and hurt others) or GMP's `mpn` routines. GMP's routines are assembly that takes the length as an argument either way, except compare, zero test and negate, which are inline C and do benefit from a constant length. Each set's kernel per operation was chosen in a separate pilot run.

**What was measured:**
- **Operations:** add, multiply (512-bit inputs), dot product, exact division by 999 and by 1000, sort, bit-length scan, widening 512 → 1024, and 30 chain steps that stay at 1024 bits.
- **A pipeline:** the operations in sequence, each result feeding the next.
- **Two regimes:** $10^5$ values (out of cache) and 3,000 (in cache, each sample repeated to at least 2 ms).
- **Two input sets:** values filling the width, and values spread over 513–1012 bits.
- **Rounds:** 12 interleaved rounds, the two sets paired within each round, 95% bootstrap intervals, and a null control (the small set timed twice) to size the noise band.

**The rule, fixed before the run.** Each regime and input set gives two verdicts: the equal-weight mean over operations, and the pipeline. A side wins a verdict only if its interval lies wholly beyond the band of ±5% (or the null control's spread, if larger). 1024 belongs to a side if that side wins at least half of the eight verdicts and the other wins none; otherwise it is a tie.

| Regime and inputs | All operations, small / large | Pipeline, small / large |
| --- | --- | --- |
| out of cache, values filling the width | 0.969 (0.953–0.982), tie | 0.964 (0.912–1.023), tie |
| out of cache, values spread | 0.961 (0.952–0.969), tie | 0.959 (0.941–0.985), tie |
| in cache, values filling the width | 0.940 (0.926–0.956), tie | 1.009 (0.936–1.142), tie |
| in cache, values spread | 0.895 (0.885–0.906), **small** | 0.972 (0.939–1.000), tie |

Ratios below 1 mean the small set is faster.

**Result: a tie, leaning small.**
- **No single operation favoured the large set** in any regime.
- **The small set won outright on** exact division by 1000 (12–15%), addition with spread values (14–33%) and widening in cache (31–33%).
- **Everything else was within 5%.** At 1024 bits the large set's only possible advantage, GMP's multiplication algorithms, is not yet in play: both sets multiply 8 × 8 limbs by schoolbook.

So 1024 is the last size for the small routines, by a margin of about 3–4%. The threshold can be stated as "compile-time lengths up to and including 1024 bits".

## 2. Storage granularity

**Setup.** Vectors are stored between operations in one of three granularities:
- **pow2:** the next power-of-two width;
- **limb:** a multiple of 64 bits;
- **byte:** a multiple of 8 bits, packed and accessed in place.

Arithmetic is always on 64-bit limbs with the same kernels. The value sizes are spread evenly within each limb count (18 sizes from 80 to 2032 bits), plus the exact boundaries as controls, in cache and beyond L2. There were 12 rounds with a null control (limb against itself), and growth chains under seven policies at three growth rates.

**Speed, ratio to limb** (geometric mean over the 18 sizes; below 1 is faster):

| Operation | pow2, in cache | byte, in cache | pow2, beyond L2 | byte, beyond L2 |
| --- | --- | --- | --- | --- |
| add | 1.05 | 0.99 | 1.06 | 1.04 |
| multiply | 1.00 | 1.12 | 1.00 | 1.13 |
| sort | 1.00 | 0.97 | 1.01 | 1.00 |
| bit-length scan | 1.02 | 1.24 | 1.05 | 1.10 |
| widen | 1.03 | 1.10 | 1.03 | 1.03 |
| all operations | 1.02 | 1.08 | 1.03 | 1.06 |

The null control stayed within 1.00 ± 0.01.

**Memory** (magnitude bytes per value over sizes 1–2048 bits):

| Granularity | Average waste above 64 bits | Worst waste |
| --- | --- | --- |
| pow2 | 24.9% | 1023 bits |
| limb | 4.9% | 63 bits |
| byte | 0.6% | 7 bits |

**Growth chains** (128 steps, about 1300 bits at the end; time relative to limb):

| Policy | Time | Re-stores | Bytes per value at the end |
| --- | --- | --- | --- |
| pow2 | 0.97 | 5 | 256 |
| limb | 1.00 | 20 | 168 |
| byte (exact) | 1.29 | 128 | 163 |
| byte + 1.5× slack | 0.97 | 8 | 231 |
| limb + 1.5× slack | **0.93** | 4 | 192 |

**Findings.**
- **64-bit granularity is the fastest storage.** Byte saves only about 4% of memory over it (0.6% against 4.9% waste) but costs 6–8% in speed. Most of that cost comes from:
  - multiplying with a partial top limb (12–13% slower);
  - bit-length scans in cache (24% slower, up to 2× at some sizes).
- **Powers of two waste a quarter of the memory on average** and are 2–3% slower, from moving the padding.
- **Growth needs slack.** Growing to exactly the size needed re-copies the vector at almost every step: 17–29% slower for bytes. Growing geometrically (1.5×) removes that. Limb storage with 1.5× slack was the fastest policy, or tied for fastest, at all three growth rates:

  | Growth per step | Limb + slack, relative to exact limbs |
  | --- | --- |
  | about 7 bits | 7% faster |
  | about 14 bits | 8% faster |
  | about 27 bits | 17% faster |

  Powers of two, which also grow geometrically, came close.

**So the measured best is:** multiples of 64 bits in storage, grown with geometric slack. Byte storage is a memory-for-speed trade of about 4% against 6–8%. Powers of two have no advantage except convention.

## 3. The format against GMP and FLINT

**Setup.** Every contender does the same exact job its own best way.
- **The format, as settled:**
  - **Vectors:** sign-and-magnitude limbs over one shared denominator per vector.
  - **Up to 1024 bits:** compile-time routines, or GMP's kernels where faster. The routine is chosen per operation on a calibration run, and the time comes from a separate run.
  - **Above 1024 bits:** run-time limbs with GMP's kernels.
  - **Single values:** the packed layout, with the unit field in the low bits and the slash field at the top.
  - **Matrix products:** the IFMA kernel.
  - **Solves:** a multimodular Cramer method on IFMA residues.
- **The libraries:**
  - **GMP:** `mpz_t` over a shared denominator (a careful GMP user's version of the same idea), and `mpq_t` per element.
  - **FLINT 3.0.1:** `fmpz` over a shared denominator, `fmpq` per element, and its matrix types.

FLINT was rebuilt with `-O3 -march=native` and BLAS, and its routine for each matrix operation (`mul`, `blas`, `multi_mod`, `fft`; Dixon, multimodular and fraction-free solvers) was chosen on a calibration run, the same rule as the format. Every binary links the same FLINT.

### Element by element

Vectors of $10^5$ values over a shared denominator of 1000 (so 3-place decimals), filling each width. Each cell is the best library (the fastest of the four) over the format; above 1, the format is faster. Times are medians over 7 interleaved rounds. All times, with every contender, are in `results/phase2.md`.

| Operation | 128 bits | 256 | 512 | 1024 | 2048 |
| --- | --- | --- | --- | --- | --- |
| a + b | 1.98 | 1.55 | 1.50 | 1.25 | 1.09 |
| a b | 4.48 | 2.89 | 1.81 | 1.03 | 1.01 |
| dot product | 2.82 | 2.90 | 1.80 | 1.20 | 1.06 |
| exact ÷ 999 | 5.12 | 3.33 | 2.36 | 1.52 | 1.23 |
| exact ÷ 1000 | 4.80 | 2.93 | 2.09 | 1.79 | 1.05 |
| sort | 2.57 | 2.05 | 1.65 | 1.52 | 1.42 |
| bit-length scan | 25 | 6.05 | 3.06 | 1.92 | 1.10 |
| growth chain to the width | 3.86 | 2.96 | 2.11 | 1.29 | 0.95 |
| decimal text in | 1.51 | 1.14 | 1.06 | 1.09 | 1.00 |
| decimal text out | 1.49 | 1.28 | 1.32 | 1.28 | 1.21 |

The best library was always GMP or FLINT over a shared denominator, the libraries' version of the same idea: GMP in 43 rows, FLINT in 7. Per-element rationals (`mpq_t`, `fmpq`) were 2–70× slower than the format on arithmetic, and 1.1–1.9× slower on decimal text.

**Findings.**
- **The format is fastest or tied in every row at every width.** The single value below 1 is the growth chain at 2048 bits (0.95), within the tie band.
- **The lead is largest at small widths and narrows steadily:**
  - 1.5–5× at 128–256 bits, where compile-time routines avoid per-element dispatch;
  - 1.0–1.8× at 1024;
  - near parity (1.0–1.4×) at 2048, where the format runs GMP's own kernels.

  This is the threshold result seen from outside: above 1024 bits the format's advantage is its layout (one denominator, dense storage), not its arithmetic.
- **Sorting keeps a lead at every width** (1.4–2.6×). One shared denominator means comparing numerators directly.
- **Decimal text is near parity at large widths.** Converting between bases is the same algorithm in every contender.
- **Two of the format's routines were improved during this round,** at 2048 bits: exact division by 1000 and the chain step now use GMP's exact-division and fused multiply-add kernels. Both kernels were already in the threshold test's large set, so the threshold verdict is unaffected. Before the change those two cells were 0.78 and 0.81.
- **Three rounds were not enough.** A first measurement with 3 rounds was unstable for some cells on both sides, so the section was rerun with 7.

### Single values

$10^6$ independent values per operation, each with its own denominator. The ratio is the best library over the format; above 1, the format is faster.

| Size | Operation | Operand bits | The packed format | GMP `mpq_t` | FLINT `fmpq` | Best library / the format |
| --- | --- | --- | --- | --- | --- | --- |
| 64 | add | 17 | 31.7 ms | 72 ms | 30.1 ms | 0.95 |
| 64 | multiply | 17 / 26 | 55.7 / 68.8 ms | 93 / 104 ms | 41.3 / 55.0 ms | 0.74 / 0.80 |
| 64 | sort | 17 / 26 | 242 / 249 ms | 1.81 / 1.90 s | 328 / 351 ms | 1.35 / 1.41 |
| 128 | add | 38 | 50.2 ms | 78.1 ms | 43.0 ms | 0.86 |
| 128 | multiply | 38 / 57 | 89.5 / 118 ms | 113 / 125 ms | 71.3 / 100 ms | 0.80 / 0.85 |
| 128 | sort | 38 / 57 | 448 / 458 ms | 1.94 / 1.91 s | 354 / 376 ms | 0.79 / 0.82 |

**For single values, FLINT is faster at arithmetic** (by 5–35%), and the format is 1.1–7.5× faster than GMP rationals.
- **Why FLINT wins arithmetic:** FLINT keeps small values inline in machine words and uses the same algorithms (Henrici's); the format pays to decode and re-encode its packed layout on every operation.
- **Sorting:** the format wins at 64 bits (1.35–1.41×) and loses at 128 (0.79–0.82), where its comparisons need 256-bit products.
- **Where the format's advantage lies:** the packed layout's gain is storage (8 bytes against FLINT's 16), not scalar speed. The vector form, which shares one denominator, is where the format's speed comes from.

### Linear algebra

Best library over the format (above 1: the format is faster). FLINT's routine is the faster on a calibration run.

| Operation | Size | 128 bits | 256 | 512 | 1024 | 2048 |
| --- | --- | --- | --- | --- | --- | --- |
| matrix-vector | 512 | 3.72 | 2.69 | 2.32 | 1.37 | 1.03 |
| dense product | 128 | 1.63 | 2.29 | 3.36 | 4.41 | 5.44 |
| dense product | 256 | 1.47 | 2.52 | 2.90 | 3.76 | 4.39 |
| sparse matrix-vector (1% nonzero) | 10,000 | 10.0 | 6.90 | 2.56 | 1.24 | 1.05 |
| linear solve | 32 | 2.88 | 3.47 | 4.77 | 5.99 | 6.47 |
| linear solve | 64 | 2.67 | 2.29 | 3.26 | 5.81 | 6.33 |

For solve, the width is the bit size of the solution's entries; the matrix entries have an eighth of it.

**Findings.**
- **The format is faster in every linear-algebra cell.**
  - **Dense matrix products** are 1.5–5.4× faster than FLINT's best routine (FLINT chose BLAS from 512 bits). The lead grows with the entries' width, because the IFMA kernel's 50-bit primes halve the work that dominates.
  - **Solves** are 2.3–6.5× faster than FLINT's fastest solver.
  - **Matrix-vector and sparse products** win by 3.7–10× at small widths, where compile-time limbs avoid per-element dispatch. The lead narrows to a tie at 2048 bits, where both run GMP's multiplication.
- **The solve's lead may narrow for larger systems.** FLINT's Dixon lifting scales better with n than the format's multimodular method, so the lead could narrow or reverse for much larger systems than the 32 and 64 tested.
- **Memory is comparable,** within a few MB in most cells, and lower than FLINT's for products and sparse matrices at large widths.

### What was posed in the format's favour, and other caveats

- **Problem framing:**
  - The vector operations assume one shared denominator, which the libraries' per-element rationals do not need.
  - Exact division is by a factor known to divide.
  - Decimal text has exactly three places.
  - The bit-length scan is defined only for shared-denominator forms, and stops after the top limb because inputs fill their width.
  - Widths are fixed in advance by proven bounds, so no overflow handling is timed.
- **Above the threshold the format uses GMP's own kernels,** so at 2048 bits the comparison is between layouts, not arithmetic.
- **GMP has no matrix routines,** so its matrix product and solve are hand-written.
- **Single-value inputs are small** (17–57 bits), so that sums and products fit in the packed sizes.
- **Single values and linear algebra had fewer rounds:** single values were measured over 3 interleaved rounds, and linear algebra once, each cell the median of 5 repetitions in one process. Differences within about 5–10% there are not meaningful. The element-by-element section had 7 rounds.
- **One machine, one thread.** The IFMA kernel requires AVX-512 IFMA; on other CPUs the format's matrix products would use the slower float kernel from phase one.

## Reopen triggers

- **The threshold:** rerun the threshold test whenever the routines that currently decide it improve. At 1024 bits that is the small set. Above it, it is GMP's multiplication, which starts to matter as widths grow. (The kernels added to the 2048-bit routines during this round were already in the threshold test's large set, so they do not reopen it.)
- **Storage:** revisit byte granularity if memory becomes the binding constraint, or if a multiply that handles a partial top limb without the extra additions removes its speed cost.

## For discussion

- **Separate specifications.** The proposal discussed during this round:
  - **Exact fixed-width rationals:** the vector form and the packed scalar, with the threshold, storage and growth policy measured here.
  - **The hypercomplex unit:** an 8-bit type, generic `Alg<T>` over any number type, in a sparse form (one unit and one T) and a dense form (one T per axis).
  - **Its spare bits:** the options for its 4 spare bits.
- **Single values.** The packed scalar loses arithmetic speed to FLINT's inline small values. Is its storage advantage enough, or should the scalar format change?
- **The large-solve risk.** Test larger systems against Dixon lifting before claiming the solve generally.
