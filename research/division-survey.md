# Integer-only division in SIMD lanes: survey

Scope: every integer-only way to divide inside vector lanes, for a divisor shared across the array and for a divisor that varies per lane, applied to Gaussian integers stored as int32 real and imaginary arrays. Two operations: exact division x / y when y divides x, and division with remainder with each quotient part rounded to the nearest integer. x86-64 with AVX2 as baseline and AVX-512 optional; ARM NEON/SVE and RISC-V V equivalents noted. No floating point anywhere, including estimates. Web survey only, November 2026. Items marked "not verified online" are cited from standard bibliographies or from memory; their pages were not fetched in this pass.

## 1. What the Gaussian operations actually need

For x = a + bi and y = c + di, the quotient is (ac + bd)/N + ((bc - ad)/N) i with N = c^2 + d^2. With int32 inputs both numerators and N lie in [-2^63, 2^63]; the extreme value 2^63 (all four inputs equal to -2^31) does not fit int64 but does fit uint64, so the natural working form is sign plus 64-bit unsigned magnitude.

The quotient is small. |x/y| <= |x| <= 2^31.5, and when y is not a unit |y| >= sqrt(2), so each quotient part fits in int32 except for the negation edge case (y a unit and a part equal to -2^31). The core kernel for rounded division is therefore a 64-bit by 64-bit unsigned division whose result has at most 33 bits. That bound matters: it caps bit-serial iteration counts (section 5) and lets a 32-bit reciprocal of the top half of N produce the quotient (section 3.3).

The remainder bound: if each quotient part is within 1/2 of the true rational part, N(r) <= N(y)/2 < N(y). Any tie rule works for the Euclidean property; the rule only needs to be fixed so results are deterministic (section 8). Truncation does not work: each fractional part in [0, 1) gives N(r)/N(y) < 2, not < 1.

## 2. Shared divisor: multiply by a precomputed reciprocal

The classic results, all integer-only:

- Granlund, T. and Montgomery, P. L., "Division by invariant integers using multiplication", PLDI 1994, [PDF](https://gmplib.org/~tege/divcnst-pldi94.pdf). Quotient = high half of x times a precomputed (N+1)-bit magic number, then a shift; signed and unsigned; section 9 covers exact division by multiplying with the inverse mod 2^N, and divisibility testing. Not verified online in this pass.
- Moller, N. and Granlund, T., "Improved division by invariant integers", IEEE Trans. Computers 60(2) (2011) 165-175, draft [PDF](https://www.lysator.liu.se/~nisse/archive/draft-division-paper.pdf). 2-by-1 division (two-word dividend, one normalized word divisor) with a one-word reciprocal, one multiply-high, one multiply-low and a cheap, mostly unlikely, correction. The reciprocal itself is computed integer-only from a small table plus Newton-like steps (GMP's `invert_limb`).
- Robison, A. D., "N-bit unsigned division via N-bit multiply-add", ARITH-17 (2005). Avoids the (N+1)-bit magic number by using a multiply-add. Not verified online.
- Warren, H. S., Hacker's Delight, 2nd ed. (2012), chapter 10: signed and unsigned magic numbers, exact division by constants (10-16), divisibility tests (10-17). Not verified online.
- Lemire, D., Kaser, O. and Kurz, N., "Faster remainder by direct computation: applications to compilers and software libraries", Software: Practice and Experience 49(6) (2019), [arXiv:1902.01961](https://arxiv.org/abs/1902.01961). Computes the remainder directly from the fractional part of x times the reciprocal ("fastmod"), and a one-multiply divisibility test. Not verified online in this pass.
- Lemire, D., Bartlett, C. and Kaser, O., "Integer division by constants: optimal bounds", Heliyon 7(6) (2021), [arXiv:2012.12369](https://arxiv.org/abs/2012.12369). Tight conditions on when a given magic number and shift are exact, including round-to-nearest variants.
- Barrett, P., "Implementing the RSA public key encryption algorithm on a standard digital signal processor", CRYPTO 1986. Barrett reduction is the same estimate-then-correct pattern used for remainders. Not verified online.
- Shoup-style precomputed multiplication (used in NTT code, e.g. Harvey, "Faster arithmetic for number-theoretic transforms", J. Symbolic Computation 60 (2014), [arXiv:1205.2926](https://arxiv.org/abs/1205.2926)): for x * w mod p with w fixed, precompute floor(w 2^k / p). Same idea, shared divisor. Not verified online.

Libraries that vectorize this:

- libdivide ([GitHub](https://github.com/ridiculousfish/libdivide)): 16, 32 and 64-bit, signed and unsigned, with SSE2, AVX2, AVX-512, NEON, SVE and SVE2 backends; branchfull and branchfree divider forms. Its README notes vector division is much faster at 32 bits than 64 bits on x86 because there is no vector 64x64 to 128 multiply.
- Agner Fog's Vector Class Library ([GitHub](https://github.com/vectorclass/version2), manual section on integer division): division of a vector by a run-time invariant divisor via `Divisor_i`/`Divisor_ui` objects, and by compile-time constants. Not verified online in this pass.
- Intel HEXL (Boemer et al., [arXiv:2103.16400](https://arxiv.org/pdf/2103.16400)) and Intel's AVX-512 IFMA modular multiplication guide ([Intel](https://www.intel.com/content/www/us/en/content-details/812656/intel-avx-512-fast-modular-multiplication-technique-technology-guide.html)): Barrett and Shoup reduction in 52-bit IFMA lanes with a per-modulus precomputed constant.

What breaks for our sizes on x86: N needs 63 to 64 bits and the numerators need 64, so a straight 64-bit magic number needs the high half of a 64x64 product. AVX2 and AVX-512 have no vector 64x64 high multiply; it costs four `vpmuludq` (32x32 to 64) plus adds and carries. AVX-512 IFMA (`vpmadd52luq`/`vpmadd52huq`) gives 52x52 to 104 bits, which helps if values are split into 52-bit pieces. ARM SVE2 has `UMULH`/`SMULH` on 64-bit elements and RISC-V V has `vmulhu`/`vmulh` at SEW=64, so the 64-bit form is direct there.

A Gaussian-specific shared form avoids the 64x64 product entirely: since the quotient real part is a(c/N) + b(d/N), precompute fixed-point C ~ c 2^k / N and D ~ d 2^k / N once per divisor and form the estimate as (aC + bD) >> k. With a and b int32 and C, D held as two 32-bit halves, this is four `vpmuldq` (signed 32x32 to 64) per part. The estimate need only be within one unit; the exact correction in section 3.4 does the rest.

## 3. Varying divisor: per-lane reciprocal by integer Newton iteration

### 3.1 The general recipe

Normalize the divisor (shift left by its leading-zero count), get a short seed for 1/N, refine with integer Newton steps v <- v + v(1 - Nv) in fixed point, multiply the dividend by v (high half), then fix the estimate with the exact remainder. Every step is integer multiply, shift, add.

- GMP `invert_limb` and Moller-Granlund 2011: 9- or 10-bit table seed, then two or three refinement steps to a full-word reciprocal.
- crypto-bigint `div_limb.rs` ([docs.rs source](https://docs.rs/crate/crypto-bigint/0.7.2/source/src/uint/div_limb.rs)): the same Moller-Granlund reciprocal made constant-time and table-free. The seed comes from a short constant-time shift-and-subtract division of the top 9 bits (10 bits in the 32-bit version), the comment noting this beat the table lookup; then refinement with only wrapping multiply, shift and select. This is close to a lane-ready recipe because every operation is branchless.
- Watt, S. M., "Efficient generic quotients using exact arithmetic", ISSAC 2023, [arXiv:2304.01753](https://arxiv.org/abs/2304.01753): "shifted inverse" Newton iteration done entirely in integers. Not verified online in this pass.

### 3.2 Normalization in lanes

Leading-zero count per lane: AVX-512CD `vplzcntd`/`vplzcntq`; NEON `CLZ` (8 to 32-bit elements, not 64); SVE `CLZ` (all sizes); RISC-V Zvbb `vclz`. AVX2 has none; integer-only emulation is a five-step compare-and-shift binary search, or nibble lookup with `vpshufb` combined across bytes. Variable shifts exist on AVX2 (`vpsllvd`, `vpsllvq`).

### 3.3 Sizing for our case

Because the quotient fits in 33 bits, the reciprocal does not need 64 bits. A workable shape: normalize N to Nn (top bit set), take Nh = top 32 bits of Nn (rounded up so the reciprocal underestimates), compute a 32-bit reciprocal of Nh with 32-bit lane arithmetic (`vpmuludq` gives the 64-bit products needed), estimate q from the top bits of the shifted numerator times that reciprocal, then correct. The relative error from truncating N to 32 bits is about 2^-31, so with q < 2^33 the estimate is off by a few units; one more refinement round (multiply the partial remainder by the reciprocal) or a short correction loop brings it to within one. This is the Moller-Granlund 2-by-1 structure at half width. Not found published in exactly this form for lanes; it is a design sketch.

### 3.4 Exact correction (common to all estimate methods)

Given an estimate q, compute r = num - q N exactly. Since the true remainder is small, wrapping 64-bit arithmetic gives r exactly. Then adjust q by plus or minus one while r is out of range, as branchless compare-and-blend steps with a fixed count chosen from the proven error bound. This is how every method above finishes, and it decouples correctness from the accuracy proof of the estimate: a too-weak estimate shows up as a failed final range check, which can be asserted in tests.

## 4. Exact division by the inverse modulo 2^k

When y divides x, x / y = x * y^-1 mod 2^k for odd y, provided the true quotient fits in k bits. No high multiply, no correction.

- Jebelean, T., "An algorithm for exact division", J. Symbolic Computation 15(2) (1993) 169-180 ([record](https://core.ac.uk/works/41337670)). Exact multi-precision division computed from the least significant end; the basis of GMP's `mpn_divexact`.
- Granlund and Montgomery 1994, section 9 (above): exact division by multiplying with the inverse mod 2^N, and the divisibility test: for odd d and unsigned x, d divides x iff (x * d^-1 mod 2^N) <= floor((2^N - 1)/d).
- Krandick, W. and Jebelean, T., "Bidirectional exact integer division", J. Symbolic Computation 21 (1996) 441-455. Computes the high half of the quotient from the top and the low half from the bottom. Not verified online.
- GMP `binvert_limb`: an 8-bit inverse from a 128-entry table indexed by (d/2) mod 128, then Newton steps. Not verified online in this pass.

Computing the inverse per lane (varying divisor) is cheap and fully vectorizable with low multiplies only. Newton for 2-adic inverses, x <- x(2 - dx), doubles correct bits each step. Seeds: x0 = d is correct to 3 bits (d^2 = 1 mod 8 for odd d); x0 = (3d) XOR 2 is correct to 5 bits (attributed to Montgomery). For 32 bits: 5, 10, 20, 40 bits, so three steps, six `vpmulld`. For 64 bits: four steps.

- Hurchalla, J., "An improved integer modular multiplicative inverse (modulo 2^w)", 2022, [arXiv:2204.04342](https://arxiv.org/abs/2204.04342). Rearranges the iteration to shorten the dependency chain and lower latency; same multiply count class.
- Dumas, J.-G., "On Newton-Raphson iteration for multiplicative inverses modulo prime powers", IEEE Trans. Computers 63(8) (2014), [arXiv:1209.6626](https://arxiv.org/abs/1209.6626). Not verified online in this pass.
- "On the inversion modulo a power of an integer", 2025, [arXiv:2506.02491](https://arxiv.org/abs/2506.02491). Generalizes to moduli b^k.

Even divisors: write y = 2^s y' with y' odd; if y divides x then 2^s divides x, so x / y = (x >> s) * y'^-1 mod 2^k. The shift is exact (arithmetic shift for signed). s comes from a trailing-zero count (via `x & -x` and a leading-zero count, or Zvbb `vctz`, SVE `RBIT`+`CLZ`).

Gaussian form, entirely in 32-bit lanes. For y = c + di, let k = min(ctz(c), ctz(d)) (treating ctz(0) as 32). Then y = 2^k y'' and, since y divides x, both parts of x are divisible by 2^k, so shift both. After this at least one of c'', d'' is odd. If both are odd, (1+i) divides y'' exactly once more: y''/(1+i) = ((c+d) + (d-c)i)/2, and the result has components of different parity, so no further factor of (1+i). Divide x by (1+i) the same way. Now N(y') is odd, and q = x' conj(y') N(y')^-1 with every product taken mod 2^32 using `vpmulld`. The result is the exact quotient whenever the true parts fit in int32, which section 1 shows is always, apart from the -2^31 negation edge. Cost: one 2-adic inverse (six multiplies), four multiplies for x' conj(y'), two for the final scaling, plus the 2-power stripping. No 64-bit arithmetic at all. If divisibility is not guaranteed, a 32-bit result proves nothing (q y = x mod 2^32 holds by construction), so verify with an exact check: compute q y with `vpmuldq` (32x32 to 64) and compare to x.

## 5. Bit-serial methods in lanes

Restoring and non-restoring shift-subtract division run fine in lanes as branchless loops: per quotient bit, shift the partial remainder, compare (or test sign), subtract or add, record the bit. Non-restoring needs only the sign of the partial remainder, about three to four vector ops per bit at the working width.

- Oberman, S. F. and Flynn, M. J., "Division algorithms and implementations", IEEE Trans. Computers 46(8) (1997) 833-854. Survey of digit recurrence (restoring, non-restoring, SRT), functional iteration (Newton, Goldschmidt) and table methods. Not verified online.
- Ercegovac, M. D. and Lang, T., Division and Square Root: Digit-Recurrence Algorithms and Implementations, Kluwer 1994, and Digital Arithmetic, Morgan Kaufmann 2004. SRT radix-4 with redundant quotient digits chosen from a small table of leading remainder and divisor bits. Not verified online.

For our case the iteration count is the quotient bit length, at most 33, and can be cut per vector to max over lanes of (bitlength(num) - bitlength(N) + 1). At 64-bit working width on AVX2 (four lanes per register, no unsigned 64-bit compare, so bias by XOR with the sign bit before `vpcmpgtq`), 33 iterations cost on the order of 130 to 170 instructions per four lanes. SRT radix-4 halves the iterations but its digit selection needs a small table lookup per step (`vpshufb` could serve), and redundant digits need a final conversion; in software the gain over radix-2 is unclear. The appeal of bit-serial is that it is trivially correct, branch-free, constant time and identical on every ISA; it is a good reference implementation and fallback.

## 6. Lookup tables in lanes

Tables help in two places: reciprocal seeds (section 3) and 2-adic inverse seeds (section 4). Direct quotient tables are only practical for tiny operands.

| ISA | In-register table | Gather |
|---|---|---|
| AVX2 | `vpshufb`: 16 bytes per 128-bit half | `vpgatherdd/qq`, slow, and slower still on Intel parts with the 2023 Gather Data Sampling microcode mitigation |
| AVX-512 | `vpermb` (VBMI) 64 bytes, `vpermi2b` 128 bytes, `vpermi2d` 32 dwords | `vpgatherdd/qq` |
| NEON | `TBL` up to 64 bytes | none |
| SVE | `TBL`, `TBX` | `LD1W` gather |
| RISC-V V | `vrgather` | indexed loads `vluxei` |

A 9- or 10-bit seed needs 512 to 1024 entries, beyond in-register tables, so it means gathers. The alternatives are a smaller in-register table with one more Newton step, or crypto-bigint's computed seed. For the 2-adic inverse, the 5-bit arithmetic seed (3d) XOR 2 makes a table unnecessary.

## 7. Hardware vector divide on other architectures

ARM. NEON has no integer divide. SVE has predicated `SDIV`, `UDIV`, `SDIVR`, `UDIVR` on 32- and 64-bit elements only. On cost: LLVM's Neoverse N2 scheduling model maps 32-bit SVE divides to a single V0 pipe with 12-cycle latency ([AArch64SchedNeoverseN2.td](https://llvm.googlesource.com/llvm-project/+/890c4bece26e005cd9fa5511fe0efa7307794de5/llvm/lib/Target/AArch64/AArch64SchedNeoverseN2.td)); scalar `SDIV` on Neoverse V2 is 12 cycles (32-bit) and 20 cycles (64-bit), not pipelined ([AArch64SchedNeoverseV2.td](https://llvm.googlesource.com/llvm-project/+/a58dcc5e08665f2d58a28c9d4510cf94de6ed3bf/llvm/lib/Target/AArch64/AArch64SchedNeoverseV2.td)). Arm's Software Optimization Guides for each core give the authoritative per-element throughput and should be checked per target; the vector divide is a single non-pipelined unit, so it is convenient rather than fast. SVE2 adds `UMULH`/`SMULH` at 64 bits, which makes the reciprocal methods direct.

RISC-V V. `vdiv`, `vdivu`, `vrem`, `vremu` exist for all element widths (8 to 64) in .vv and .vx forms. Timing is implementation-defined and typically iterative; LLVM's scheduling models for SiFive P600 ([PR 115038 thread](https://lists.llvm.org/pipermail/llvm-commits/Week-of-Mon-20241104/1495066.html)) and SpacemiT X60 ([commit](https://llvm.googlesource.com/llvm-project/+/8952225d88a5ce58a65c8b8695c610f4499d7181), [measurements PR 144730](https://llvm.googlesource.com/llvm-project/+/3516ad05dfd674d731487cb67bbfe48f7e1fda9c)) model divide latency scaling with element width and LMUL; we could not retrieve the numbers through our proxy. Camel-cdr's [rvv-bench](https://github.com/camel-cdr/rvv-bench) has an instruction cycle-count tool and per-core results. `vmulhu` at SEW=64 and Zvbb `vclz`/`vctz` make the reciprocal and 2-adic methods direct.

x86. No vector integer divide in any extension through AVX10. Intel SVML exposes `_mm256_div_epi32` and similar as library functions, not instructions; their implementation is undocumented and must be treated as possibly floating point, so it is excluded. Scalar `DIV r64` on recent Intel and AMD cores is roughly 10 to 20 cycles of latency with one division issued every several cycles (Agner Fog's [instruction tables](https://www.agner.org/optimize/instruction_tables.pdf), [uops.info](https://uops.info/table.html)); per-lane scalar division also pays extract and insert costs. Note also `vpmulld` (32x32 low) is two uops and about 10 cycles latency on Intel cores, versus about 5 for `vpmuludq`, and is cheap on Zen 4 (uops.info), which affects the 2-adic inverse cost on Intel.

## 8. GPUs

NVIDIA has no integer divide instruction. ptxas expands 32-bit unsigned division as: convert divisor to float (I2F), approximate reciprocal (MUFU.RCP, about 23 bits), convert back (F2I), estimate quotient with IMAD.HI, compute remainder with IMAD, then up to two correction steps; roughly 50 SASS instructions at 32 bits and 80 at 64 bits, the 64-bit and signed forms going through helper routines ([ptxas internals notes](https://grigoryevko.github.io/nvopen-tools/ptxas/intrinsics/math.html); NVIDIA patent US 8,938,485 "Integer division using floating-point reciprocal", [USPTO PDF](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/8938485)). AMD GPUs follow the same pattern in LLVM (`AMDGPUCodeGenPrepare::expandDivRem32` uses a float reciprocal seed then integer refinement; not verified online in this pass). Both use a floating-point seed, so the GPU recipe is excluded as written, but its structure (seed, integer multiply-high, exact remainder, bounded correction) is exactly section 3 with an integer seed substituted.

Multi-precision on GPU: Marchioro, Raahauge, Lovenskjold, Oancea and Watt, "On GPU implementation for multi-precision integer division", 2026, [arXiv:2606.06386](https://arxiv.org/abs/2606.06386): Newton iteration on the shifted inverse kept entirely in integers, data-parallel, CUDA, for 2^15 to 2^18 bit precisions; notes CGBN's division does not cover that range.

## 9. Vectorized multi-precision libraries

- GMP: `mpn_divexact` (Jebelean, Hensel/bdiv), `mpn_div_qr_1` with `invert_limb` (Moller-Granlund). Scalar, but the per-limb algorithms are the ones to port.
- Gueron, S. and Krasnov, V., "Accelerating big integer arithmetic using Intel IFMA extensions", ARITH 2016 ([record](https://cris.iucc.ac.il/en/publications/accelerating-big-integer-arithmetic-using-intel-ifma-extensions/)). Multiplication and Montgomery reduction in 52-bit IFMA lanes; no general division.
- Bos, J. W., Montgomery, P. L., Shumow, D. and Zaverucha, G. M., "Montgomery multiplication using vector instructions", SAC 2013. Not verified online.
- Montgomery's REDC itself is an exact division by 2^k of (x + mN), the same 2-adic idea as section 4.
- CGBN (NVIDIA Labs) and the paper in section 8 for GPU.

Vectorized bignum code consistently avoids general division: it uses Montgomery or Barrett with a precomputed per-modulus constant (shared divisor), and Newton on the inverse for the rare full division.

## 10. Rounding to nearest in integer-only division

With nonnegative n and N > 0: compute floor quotient q and remainder r = n - qN, then round up when 2r > N, and decide ties (2r = N) by rule. Since r < N <= 2^63, 2r fits in uint64. This avoids forming (2n + N) / (2N), which would need 65 bits. For signed numerators, round the magnitude and restore the sign: that gives ties away from zero, which is symmetric under negation and conjugation, a useful property for Gaussian code (q(conj x, conj y) = conj q(x, y), and multiplying x by a unit rotates q). Ties to even is the other common choice; it costs one extra AND on the low quotient bit. Floor-based rounding (ties up) breaks the symmetry. The gcd survey notes GAP's rule (nearest, ties towards the origin).

- Drane, T., Cheung, W.-c. and Constantinides, G., "Correctly rounded constant integer division via multiply-add", ISCAS 2012, [doi:10.1109/ISCAS.2012.6271461](https://doi.org/10.1109/ISCAS.2012.6271461). Round-to-nearest and other modes for division by a constant via one multiply-add. Not verified online beyond the DOI record.
- Lemire, Bartlett and Kaser 2021 (section 2) also treat rounding variants.
- C++ proposal P3724, "Integer division" ([R4](https://open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3724r4.html)): standard-library division with selectable rounding modes, including to nearest with tie rules, with reference implementations. Useful for naming and test vectors.

## 11. Gaussian or complex integer division in SIMD

We found no published SIMD implementation of Gaussian integer division. Libraries with Gaussian integers (FLINT `fmpzi`, PARI, GAP, Sage) are scalar; FLINT uses doubles for small operands (see gcd-survey.md), which we cannot. Collins's approximate-quotient idea (gcd-survey.md section 1.1) is the closest relative: a quotient from leading bits, corrected afterwards. The Gaussian structure itself helps in two ways found in this survey: removing (1+i) factors reduces exact division to an odd norm (section 4), and the quotient part a(c/N) + b(d/N) form gives a shared-divisor scheme with only 32x32 multiplies (section 2).

## 12. Options for our build

Shared divisor (one y for the whole array):

1. Exact division: the (1+i)-stripping plus 2-adic inverse of section 4, computed once; per element it is shifts and about six 32-bit low multiplies, no 64-bit work. Cheapest and simplest to prove.
2. Rounded division: precomputed fixed-point C and D (section 2), estimate with `vpmuldq`, exact remainder and a fixed number of branchless corrections, then the rounding comparison of section 10. All 32x32 to 64 multiplies, which AVX2 has.
3. Rounded division, alternative: libdivide-style 64-bit magic number for N applied to the 64-bit numerators. Correct and well documented, but the 64x64 high multiply is emulated on x86; better on SVE2 and RVV.
4. Scalar `div` per element: only as a reference.

Varying divisor (per-lane y):

1. Exact division: the same section 4 path with a per-lane 2-adic inverse (three Newton steps from the 5-bit seed). Everything stays in 32-bit lanes; verification, when needed, by a `vpmuldq` back-multiply.
2. Rounded division: per-lane normalized 32-bit reciprocal of the top of N (computed seed as in crypto-bigint, then integer Newton), quotient estimate, exact correction, rounding comparison (sections 3.3, 3.4, 10). Most work per lane but fully vectorized on AVX2, faster with AVX-512CD (`vplzcnt`) and IFMA.
3. Rounded division, fallback and reference: non-restoring bit-serial over at most 33 quotient bits (section 5). Branch-free, identical on all ISAs, easy to trust; slower by an estimated factor of a few.
4. Scalar `div` per lane: competitive at small vector widths given the extract/insert cost, and a good oracle for tests. On SVE, native `UDIV` on 64-bit elements is a direct per-lane equivalent; on RVV, `vdivu` at SEW=64, with speed that depends on the core.

Excluded: anything using a floating-point seed (SVML integer division, GPU-style MUFU recipes, float conversion tricks), per the exactness rule.
