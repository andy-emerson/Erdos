# Phase one: the variants compared with each other

Oct 27, 2026 · Andrew Emerson

This note reports the first of two research phases before the next specification. Phase one compares the format's design variants **only with each other**, to find the best version of the format. GMP, FLINT and other libraries are not contenders here; they come in phase two, after the decisions below are made. Where variants trade off, this note reports the trade-off and leaves the decision to the discussion.

The variants come from the register agreed in the last discussion (A–L), and the conditional routines from the list of where each condition is expected to flip.

The code is in [`bench/q-research`](../../bench/q-research/): `phase1_bench.py`, `p1_layout.c`, `p1_arith.c`, `p1_widen.c` and `p1_gemm.c`, with their runners and tests. The results are in `bench/q-research/results/phase1.md`.

Every timed result was checked:
- **S1** against an independent reference coder in Python;
- **S2, S3 and S5** against Python integers;
- **S6** against FLINT's classical product (128 × 128 and below) and a random-vector (Freivalds) check at every size.

Tests confirm that each check catches a deliberately wrong result. An independent review, briefed to look for tests that could only confirm what we expected, came before the measured run. It found three serious problems, all fixed:
- **Residue credit:** residues were being credited with widening done outside the timed region.
- **Noise read as findings:** differences within noise were reported as findings. A 10% tie band now applies throughout.
- **Uneven tuning:** the matrix variants had uneven hand-tuning, which was levelled before measuring.

One thread on one machine (2-core Xeon with AVX-512); medians of 3–7 runs.

## S1. Value layouts (A, B, C)

The layouts compared:
- **Tuple, header byte + word:** fitted or uniform header, borrowed slash bits at the top or the bottom. Stored three ways:
  - a padded struct;
  - a packed struct;
  - header bytes and words in separate arrays.
- **Packed power-of-two words:** fitted or uniform header, slash bits at the top or the bottom.

The fitted example is complex numbers (2 unit bits); the uniform header has 4 unit bits.

**Sizes and precision.**

| Layout | Size | Data bits | Data bits per stored bit | Median relative error |
| --- | --- | --- | --- | --- |
| tuple, fitted, U32 | 40 | 32 | 0.80 | $2.5 \times 10^{-10}$ |
| tuple, uniform, U32 | 40 | 31 | 0.78 | $5.2 \times 10^{-10}$ |
| packed, fitted, 32 | 32 | 25 | 0.78 | $3.5 \times 10^{-8}$ |
| packed, uniform, 32 | 32 | 23 | 0.72 | $1.6 \times 10^{-7}$ |
| tuple, fitted, U64 | 72 | 64 | 0.89 | $5.8 \times 10^{-20}$ |
| tuple, uniform, U64 | 72 | 62 | 0.86 | $2.8 \times 10^{-19}$ |
| packed, fitted, 64 | 64 | 56 | 0.88 | $1.5 \times 10^{-17}$ |
| packed, uniform, 64 | 64 | 54 | 0.84 | $6.8 \times 10^{-17}$ |
| tuple, uniform, U128 | 136 | 125 | 0.92 | $2.9 \times 10^{-38}$ |
| packed, uniform, 128 | 128 | 117 | 0.91 | $6.9 \times 10^{-36}$ |

At 16 bits the packed uniform layout has 8 data bits (0.50 per stored bit). Its tuple counterpart has 16 data bits in 24 (0.67).

**Costs, ns per value, median.** Rows are U64 / 64-bit, which is typical.

| Layout | Bytes per value | Decode | Encode | Multiply in the format | Promote to the next size | Complex → quaternion, same size |
| --- | --- | --- | --- | --- | --- | --- |
| tuple, fitted, separate arrays | 9 | 1.7 | 2.6 | 73 | 2.0 | 1.6; every value using all 64 data bits no longer fits |
| tuple, uniform, separate arrays | 9 | 1.8 | 3.0 | 75 | 2.2 | 0.6 (a copy: the bits are identical) |
| packed, fitted | 8 | 1.9 | 2.7 | 75 | 2.3 | 1.7; every value using all 56 data bits no longer fits |
| packed, uniform | 8 | 1.7 | 2.8 | 74 | 2.2 | 0.6 (a copy) |

**Findings.**
- **No layout is faster.** Decoding, encoding, multiplying in the format and promoting cost the same within noise for all four layouts at every size. The multiply is dominated by the gcd, which is the same work in every layout.
  - Decode times scatter from 1.5 to 5.3 ns at 16 and 32 bits. Those differences track whether the compiler vectorised a particular loop, not the layout. The same holds for slash bits at the top against the bottom, and for the three tuple storages: medians within 2%.
- **Packing saves memory:** 8 bytes against 9 at 64 bits, and 16 against 17 at 128, with the same per-bit precision. In arrays it also avoids padding: a natural struct of a header and a U64 takes 16 bytes. The packed form is the only one that is a machine type.
- **The uniform header is what protects exactness across algebras.** Moving a full-precision complex value into the quaternions at the same size fails in both fitted layouts for every value that uses all its data bits, 14% of these inputs, because the quaternion layout has one fewer data bit. In both uniform layouts it is a plain copy and nothing is lost.
- **Precision costs, all fixed and small:**
  - 2 data bits for the uniform header against fitted (complex);
  - 7–8 data bits for packed against the tuple with the same word, which is 8 bits larger, so the precision per stored bit is about the same;
  - at 16 bits, packed uniform keeps only 8 data bits.

## S2. Array representations by width (G, H)

The vector form at widths 64 to 2,048 bits, $10^5$ elements, five representations:
- `_BitInt(W)`: the compiler's fixed-width integers;
- compile-time limbs: the length fixed when compiling, hand-written loops;
- run-time limbs: the length set at run time, GMP's `mpn` routines;
- residues: a 2⁶⁴ modulus plus 62-bit primes, so that a 64-bit vector is exactly an int64 vector;
- GMP `mpz_t` per element.

Data fills its width, as the narrowest-width rule makes it. Products and dot products take inputs stored at $W/2$ and produce width $W$.

Residues cannot widen without base extension. So their product inputs are stored already extended, and a separate row adds that cost. The table shows the fastest representation for each case; full tables, with every representation, are in the results file.

| Operation | 64–128 bits | 256–512 bits | 1,024 bits | 2,048 bits |
| --- | --- | --- | --- | --- |
| add | `_BitInt` | `_BitInt` | `_BitInt`, residues (tie) | residues (1.2× over the rest) |
| multiply, including widening | `_BitInt`, compile-time limbs (int64 residues at 64) | compile-time limbs | run-time limbs, mpz (1.2×) | run-time limbs, mpz (1.5×) |
| dot product, including widening | `_BitInt` | `_BitInt`, compile-time limbs | run-time limbs, `_BitInt` (tie) | run-time limbs (1.4×) |
| exact ÷ 999 | `_BitInt`, compile-time limbs | compile-time limbs | compile-time limbs | run-time limbs (1.6×) |
| exact ÷ 1000 | `_BitInt`, compile-time limbs | compile-time limbs | compile-time limbs | compile-time limbs |
| sort | residues (int64 at 64) | residues | `_BitInt` | run-time limbs (1.2×) |
| bit-length scan | limbs | limbs | limbs | limbs |
| widen $W/2 \to W$ | `_BitInt` | `_BitInt`, limbs | `_BitInt`, limbs | all positional (tie) |

**Residues.**
- **Arithmetic is their strength.** With inputs already extended they have the fastest product at 2,048 bits (7.3 ms against 13 ms) and the fastest addition.
- **Everything that needs the actual number costs them heavily:**
  - growing a vector by base extension: 185 ms against 1.4 ms at 2,048 bits;
  - exact division by an even number: 197 ms against 4.9 ms;
  - sorting: 244 ms against 51 ms;
  - finding bit lengths: 221 ms against 0.6 ms.
- **Including widening, they never win a product.** The review found that replacing the slow parts of this base extension gained only 10–25%. The cost is quadratic in the number of moduli, so this is a property of residues, not of this implementation.
- **One structural advantage:** residues widen in steps of one 62-bit prime instead of doubling (see S3).

**`_BitInt`** is the fastest representation for most operations up to 512 bits. Above 512 it falls behind because it multiplies at the full width even when one factor is a single word: C has no narrower multiply for it. Compile-time limbs avoid that.

**Run-time against compile-time limbs.** Below 1,024 bits compile-time limbs win. At 1,024–2,048 bits run-time limbs win products, by 1.2× to 1.6×, because GMP's routines switch to faster multiplication algorithms.

## S3. Growth chains through every width (I)

$x \leftarrow x b + c \cdot 1000^{k+1}$ on $10^5$ 3-place decimals. Each step count is the largest whose proven bound fits the width, and every representation follows the same widening rule and cap.

| Width reached | Steps | `_BitInt` | compile-time limbs | run-time limbs | residues | mpz |
| --- | --- | --- | --- | --- | --- | --- |
| 128 | 6 | **1.1 ms** | 3.0 ms | 8.2 ms | 3.6 ms | 19 ms |
| 256 | 15 | **6.3 ms** | 12 ms | 23 ms | 22 ms | 33 ms |
| 512 | 32 | **41 ms** | **39 ms** | 60 ms | 91 ms | 79 ms |
| 1024 | 66 | 281 ms | **150 ms** | 182 ms | 472 ms | 189 ms |
| 2048 | 134 | 1.72 s | 571 ms | 610 ms | 3.27 s | **495 ms** |

At 146 steps every representation stopped at the 2,048-bit cap before overflowing.

**Findings.**
- **Below 512 bits the compiler's fixed widths win clearly.** `_BitInt` is 5–17× faster than mpz at 128 and 256 bits. At 512 bits compile-time limbs tie with `_BitInt`.
- **At 1,024 bits compile-time limbs win.** At 2,048 bits the run-time side catches up: mpz is 1.15× faster than compile-time limbs, and run-time limbs tie with them.
- **Residues are slowest past 256 bits,** because growth means base extension at every new modulus. They do keep a tighter capacity: 1,055 bits where the others jumped to 1,024 and then 2,048.

## S4. Where each condition flips (I, the threshold n*)

Ratio: the fastest compile-time representation over the fastest run-time one. Above 1, the run-time side is faster; within 10% counts as a tie.

| Operation | 128 | 256 | 512 | 1024 | 2048 | Flips at |
| --- | --- | --- | --- | --- | --- | --- |
| add | 0.67 | 0.80 | 0.87 | 1.01 | 1.21 | 2048 |
| multiply, including widening | 0.22 | 0.35 | 0.70 | 1.20 | 1.48 | 1024 |
| dot product, including widening | 0.05 | 0.34 | 0.63 | 1.09 | 1.38 | 2048 (tie at 1024) |
| exact ÷ 999 | 0.40 | 0.59 | 0.74 | 0.84 | 1.60 | 2048 |
| exact ÷ 1000 | 0.26 | 0.37 | 0.55 | 0.63 | 0.78 | not by 2048 |
| sort | 1.20 | 1.49 | 1.29 | 0.86 | 1.19 | noisy; residues win sorting at 128–512 |
| bit-length scan | 1.44 | 0.93 | 0.99 | 1.00 | 0.98 | tie throughout |
| growth chain (S3) | — | — | — | compile-time | ties or run-time | 2048 |

**Findings.**
- **The conditions cluster.** Multiplication flips at 1,024 bits, and addition, the dot product, division by odd numbers and growth chains at 2,048. Division by even numbers and scanning have not flipped by 2,048. Nothing flips below 1,024.
- **So the single threshold is supported, and the data places it between 1,024 and 2,048 bits.**
  - At 1,024, only multiplication (by 1.2×) prefers the run-time side.
  - At 2,048, most operations do, but by modest margins (1.2× to 1.6×).
- **The run-time side's lead comes from GMP's multiplication algorithms,** not from run-time lengths as such. Below the threshold, compile-time lengths are never worse.

## S5. The widening rule (F)

Whole-vector widening against per-element promotion, on $10^6$ values, with a share of the values needing 256 bits.

| Share needing 256 bits | 0% | 0.1% | 1% | 10% | 100% |
| --- | --- | --- | --- | --- | --- |
| whole vector / per element, $(ab)^2$ | 0.86 | 2.4 | 1.9 | 1.3 | 0.70 |
| the same, sum of $(ab)^2$ | 1.04 | 2.5 | 2.1 | 1.4 | 0.73 |

Above 1, per element is faster.

**Findings.**
- **Per-element promotion wins when a few values are wide:** about 2.4× at 0.1%, 2× at 1%, 1.3× at 10%.
- **Whole-vector widening wins when all values are wide (1.4×), and slightly when none are (1.2× on squares; a tie on sums),** because it has no per-element check or second pass.
- **The crossover lies between 10% and 100% wide values.** This is itself a conditional routine (it depends on the share of wide values, not on width), so the choice is between:
  - a fixed rule;
  - a rule that picks by the share, which one scan already measures.

## S6. Exact matrix products (K)

Three variants of the multimodular product:
- **`mmg`:** float `dgemm` for every phase, 16-bit digits, about 26-bit primes;
- **`mmgc`:** the same in bounded memory;
- **`mmi`:** an AVX-512 IFMA integer kernel with BLIS-style packing and 50-bit primes, so about half as many primes.

The per-residue support code was vectorised to the same standard in all three before measuring.

| Size | Entry bits | `mmg` | `mmgc` | `mmi` | `mmg` / `mmi` | Peak memory: `mmg` / `mmgc` / `mmi` |
| --- | --- | --- | --- | --- | --- | --- |
| 128 | 64 | 1.24 ms | 1.28 ms | 1.09 ms | 1.1× | +8 / +5 / +4 MB |
| 256 | 256 | 30 ms | 22 ms | 18 ms | 1.7× | +89 / +37 / +30 MB |
| 256 | 2048 | 412 ms | 355 ms | 159 ms | 2.6× | +660 / +167 / +189 MB |
| 512 | 256 | 187 ms | 129 ms | 115 ms | 1.6× | +353 / +141 / +115 MB |
| 512 | 1024 | 848 ms | 704 ms | 403 ms | 2.1× | +1,322 / +372 / +385 MB |
| 512 | 2048 | 1.91 s | 1.85 s | 0.90 s | 2.1× | +2,624 / +655 / +742 MB |

About 400 MB of the 512 × 2,048 memory figures is shared by every method: the inputs, the output, and the copy made for the check.

**Findings.**
- **The IFMA kernel is the clear winner:** 2.1× to 2.6× faster than `mmg` at 1,024 and 2,048 bits, and 1.1× to 1.7× at smaller widths. Most of the gain comes from needing half as many primes, which halves the reductions and reconstructions that dominate the time.
- **Bounded memory costs nothing:**
  - **Speed:** `mmgc` is as fast as `mmg` or faster, since smaller blocks stay in cache.
  - **Memory:** it uses 3–4× less, and less than a quarter of `mmg`'s working memory once the shared part is subtracted. `mmi` is similar.
- **The kernel needs AVX-512 IFMA.** That is present on recent Intel server and some client CPUs and on AMD Zen 4 and later, but not on Apple silicon or older x86, where `mmgc` is the variant to use. This is a portability condition, not a size condition.

## Decisions for discussion

**Leaning clear from the data** (still for confirmation):
1. **Uniform header (B):** it is the only choice that keeps exactness when moving between algebras, at a fixed cost of 1–3 bits.
2. **Below the threshold, compile-time widths (G):** `_BitInt` up to 512 bits and compile-time limbs at 512–1,024. Or compile-time limbs throughout, which never lose badly: one kernel style against two.
3. **The threshold n\* (I):** between 1,024 and 2,048. Everything below 1,024 prefers compile-time code. At 2,048 the run-time side, which uses GMP's multiplication algorithms, wins most operations by 1.2× to 1.6×.
4. **Matrix products (K):** the IFMA kernel where the CPU has it, the bounded-memory float version otherwise.

**Trade-offs to decide:**
1. **Packed against tuple (A).**
   - **Packed gives:** the conventional power-of-two sizes; one byte less per value; no padding; a machine type.
   - **Packed costs:** 7–8 fewer data bits than the tuple with the same word, which is 8 bits larger. At 16 bits it leaves only 8 data bits.
   - **Speed:** neither is faster.
2. **Residues above the threshold (H).** They win addition, products when no widening is needed, and sorting at mid widths. They lose badly wherever a value's magnitude matters: growth, even division, scans and, above 512 bits, sorting.
   - **One option:** a representation for vectors that stay at one width through long runs of products and sums (matrix-like work).
   - **The other:** drop them in favour of run-time limbs.
3. **The widening rule (F).** Whole vector, per element, or chosen by the share of wide values, measured in the scan the vector form already does.
4. **Slash bits at the top or the bottom (C).** There is no cost difference. Top keeps one contiguous bit string in field order; bottom has no advantage that was measured.

**Deferred, as agreed:**
- int8 Ozaki II matrix products;
- a Strassen layer, which only matters beyond this machine's memory;
- sparse methods, which are better treated as phase-two operation rows.

**Caveats:**
- **One machine.** This is one machine, with one thread and one compiler. The compile-time against run-time results depend on the compiler's code for `_BitInt` and on GMP's tuning for this CPU.
- **Residue implementation.** Base extension is scalar here. A vectorised version would narrow, though by the review's measurement not close, the residues' gap on growth.
