# Portable instruction list: NEON and its x86 stand-in (Study 28)

Decision 34: the build targets ARM64 NEON. The test machine is x86 with AVX-512, used as a stand-in. Test builds use 128-bit registers (xmm, through AVX-512VL where needed) and only the operations below. Correctness is tested in full on x86. Timings are read for ranking only.

**Rules.**
- 128-bit registers only: 4 lanes of 32 bits, or 2 of 64.
- No AVX-512 mask registers (k0 to k7). A compare gives a full-lane mask (every bit 0 or every bit 1), as NEON's compares do, and selects use that mask.
- No 64-bit lane multiply (`vpmullq`). Only the widening 32 × 32 → 64.
- No three-input logic (`vpternlog`).
- No floating point.
- No operation that NEON lacks, even when x86 has it in one instruction (list at the end).

In neon28.c every operation is a macro named after the NEON intrinsic, so the kernels read as NEON code. The macro body is the x86 stand-in.

## Shared operations

32-bit lanes unless stated. "Ops" is the number of x86 instructions in the stand-in.

| operation | NEON intrinsic | x86 stand-in (xmm) | ops |
|---|---|---|---|
| load, store 128 bits | `vld1q_s32`, `vst1q_s32` | `_mm_loadu_si128`, `_mm_storeu_si128` | 1 |
| broadcast a constant | `vdupq_n_s32`, `vdupq_n_s64` | `_mm_set1_epi32`, `_mm_set1_epi64x` (hoisted) | 0 in loops |
| add, subtract (32, 64) | `vaddq_s32`, `vsubq_s32`, `vaddq_s64`, `vsubq_s64` | `_mm_add_epi32`, `_mm_sub_epi32`, `_mm_add_epi64`, `_mm_sub_epi64` | 1 |
| negate | `vnegq_s32`, `vnegq_s64` | subtract from zero | 1 |
| absolute value | `vabsq_s32` | `_mm_abs_epi32` | 1 |
| and, or, xor | `vandq_s32`, `vorrq_s32`, `veorq_s32` | `_mm_and_si128`, `_mm_or_si128`, `_mm_xor_si128` | 1 |
| and-not, a AND NOT b | `vbicq_s32(a, b)` | `_mm_andnot_si128(b, a)` | 1 |
| not | `vmvnq_s32` | xor with all ones | 1 |
| shift by an immediate, 32-bit | `vshlq_n_s32`, `vshrq_n_s32`, `vshrq_n_u32` | `_mm_slli_epi32`, `_mm_srai_epi32`, `_mm_srli_epi32` | 1 |
| shift by an immediate, 64-bit (arithmetic too) | `vshlq_n_s64`, `vshrq_n_s64`, `vshrq_n_u64` | `_mm_slli_epi64`, `_mm_srai_epi64` (AVX-512VL), `_mm_srli_epi64` | 1 |
| shift each lane by its own count | `vshlq_s32` (signed count; negative shifts right, arithmetic), `vshlq_u32` (logical) | `_mm_sllv_epi32`, `_mm_srav_epi32`, `_mm_srlv_epi32` | 1 (see note 1) |
| compare equal, greater (signed, 32 and 64) | `vceqq_s32`, `vcgtq_s32`, `vcltq_s32`, `vceqq_s64`, `vcgtq_s64` | `_mm_cmpeq_epi32`, `_mm_cmpgt_epi32`, `_mm_cmpeq_epi64`, `_mm_cmpgt_epi64` | 1 |
| compare against zero | `vceqzq_s32`, `vcltzq_s32` | compare with zero; `_mm_srai_epi32(x, 31)` for x < 0 | 1 |
| min, max (signed and unsigned, 32-bit) | `vminq_s32`, `vminq_u32`, `vmaxq_s32`, `vmaxq_u32` | `_mm_min_epi32`, `_mm_min_epu32`, `_mm_max_epi32`, `_mm_max_epu32` | 1 |
| select by a full-lane mask | `vbslq_s32(m, a, b)` | `_mm_blendv_epi8(b, a, m)` | 1 (see note 2) |
| swap the two 32-bit lanes of each 64-bit lane | `vrev64q_s32` | `_mm_shuffle_epi32(x, 0xB1)` | 1 |
| even lanes of two registers, odd lanes of two registers | `vuzp1q_s32`, `vuzp2q_s32` | `_mm_shuffle_ps(a, b, 0x88)`, `_mm_shuffle_ps(a, b, 0xDD)` (cast) | 1 |
| interleave low halves, high halves | `vzip1q_s32`, `vzip2q_s32` | `_mm_unpacklo_epi32`, `_mm_unpackhi_epi32` | 1 |
| interleave 64-bit halves | `vzip1q_s64`, `vzip2q_s64` | `_mm_unpacklo_epi64`, `_mm_unpackhi_epi64` | 1 |
| byte rotate across two registers | `vextq_s32` | `_mm_alignr_epi8` | 1 |
| widening multiply, lanes 0 and 1 → two 64-bit products | `vmull_s32(vget_low)`, `vmull_u32` | `_mm_mul_epi32`, `_mm_mul_epu32` on the even lanes (note 3) | 1 |
| low 32 bits of a 32 × 32 product | `vmulq_s32` | `_mm_mullo_epi32` | 1 (note 4) |
| 16-bit low multiply | `vmulq_s16` | `_mm_mullo_epi16` | 1 |
| narrow 64 → 32 (truncate) | `vmovn_s64` | `_mm_cvtepi64_epi32` (AVX-512VL), or `vuzp1q_s32` of two registers | 1 |
| widen 32 → 64, low half | `vmovl_s32` | `_mm_cvtepi32_epi64` | 1 |
| count leading zeros per lane | `vclzq_u32` | `_mm_lzcnt_epi32` (AVX-512CD + VL) | 1 (note 5) |
| byte popcount | `vcntq_u8` | `_mm_popcnt_epi8` (BITALG) | 1 |
| 16-entry byte table, index ≥ 16 gives 0 | `vqtbl1q_u8` | `_mm_shuffle_epi8` | 1 (note 6) |
| "is the register all zero" (for a branch) | `vmaxvq_u32` then compare to 0 in a general register | `_mm_testz_si128` | 1 (note 7) |

## Notes

1. **Variable shifts.** NEON has one shift instruction per signedness, with the direction in the count's sign. A right shift by k needs the count −k. Where a kernel computes k from a count of zeros it can produce −k at the same cost (for example clz − 31 in place of 31 − clz), so the stand-in uses the x86 right shift by k and the op count is the same. Counts of 32 or more: x86 gives 0 (logical) or the sign (arithmetic); NEON gives the same.
2. **Select.** `vbsl` takes any bitwise mask. The stand-in is the byte blend, which selects per byte by each byte's top bit, so it is correct only for full-lane masks, as NEON's compares give. Code that relies on x86's sign-bit-only lane blend (`blendv_ps` with a mask that has only bit 31 set, as studies 24 to 27 did) fails the tests here. NEON's `bsl` overwrites the mask register; a copy may be needed (not counted). On this core `vpblendvb` xmm is 1 to 3 µops depending on the port model, against 1 for `bsl`.
3. **Widening multiply, the upper half.** NEON's `vmull_s32` takes lanes 0 and 1, and `vmull_high_s32` lanes 2 and 3, with no setup. x86's `pmuldq` takes lanes 0 and 2. The stand-in for the upper half is a 64-bit right shift by 32 of each operand, then `pmuldq` on lanes 1 and 3: 1 multiply plus up to 2 shifts (shared when an operand is reused). The products then come out in even and odd order on x86 (lanes 0, 2 and 1, 3) and in natural order on NEON (0, 1 and 2, 3). Packing the low halves back is `vuzp1q_s32` on NEON; on x86 the stand-in for the even/odd order is a shift and a blend, or `shufps` and a `pshufd`. These extra x86 ops are flagged where counted.
4. **The 32-bit low multiply** (`pmulld`) is 2 µops and 10 cycles on this core; NEON's `mul` is one instruction. Rankings that trade it against other work are uncertain.
5. **clz.** `vplzcntd` exists here only through AVX-512CD; it is one instruction on both.
6. **Table lookup.** `pshufb` zeroes a byte when the index has bit 7 set and otherwise reads the low 4 bits; `tbl` zeroes it for any index ≥ 16. For indices 0 to 15 and for 0x80 to 0xFF the two agree. Kernels use only those indices.
7. **The all-zero test.** x86 `ptest` sets flags directly. NEON needs a reduction across lanes (`umaxv`, a few cycles), a move to a general register and a compare and branch. Loops that test once per step (the gcd's loop exit, a per-step branch like study 27's nbrd) pay more on NEON than here. Rankings between a branched and a branch-free form are uncertain for that reason.

## NEON operations with no direct x86 equivalent

The substitute used here, and what it means for rankings. Rankings that rest on these are marked uncertain in studies.md.

| NEON | what it does | x86 substitute | ops (NEON 1) |
|---|---|---|---|
| `vmull_high_s32`, `vmull_high_u32` | widening multiply of lanes 2 and 3 | shift both operands right 32 in 64-bit lanes, `pmuldq` (odd lanes) | 1 to 3 |
| `vmlal_s32`, `vmlsl_s32` (and `_high`) | widening multiply-add into 64-bit lanes | `pmuldq`, then `paddq` or `psubq` | 2 |
| `vqdmulhq_s32` | high half of 2ab, saturating (32-bit high-half multiply) | `pmuldq` even and odd (with shifts), shift, combine | 5 to 6 |
| `vqrdmulhq_s32` | the same, rounded | as above with a rounding add | 6 to 7 |
| `vmlaq_s32`, `vmlsq_s32` | multiply-add, low 32 bits | `pmulld`, then add | 2 |
| `vqtbl2q_u8`, `vqtbl3q_u8`, `vqtbl4q_u8` | 32, 48, 64-byte table | one `pshufb` per 16 bytes with the index offset, ORed (or AVX-512VBMI `vpermt2b` for 32 bytes, which wraps indices instead of zeroing) | 2 to 7 |
| `vqtbx1q_u8` | lookup that keeps the old byte when out of range | `pshufb`, compare, blend | 3 |
| `vqaddq_s32`, `vqsubq_s32` | saturating 32-bit add, subtract | none; not used (study 15's xor test is used instead) | |
| `vtstq_s32` | mask where a AND b ≠ 0 | and, compare equal to zero, not | 3 (2 when the inverted mask suffices) |
| `vornq_s32` | a OR NOT b | not, or | 2 |
| `vcgeq_s32`, `vcleq_s32` | greater or equal | `pcmpgtd` with operands swapped, then not | 2 (1 when the inverted mask suffices) |
| `vcgtq_u32`, `vcgeq_u32` | unsigned compare | xor the sign bit into both, `pcmpgtd`; or `pminud` and `pcmpeqd` | 2 to 3 |
| `vcgtq_u64` | unsigned 64-bit compare | xor the sign bit, `pcmpgtq` | 3 |
| `vtrn1q_s32`, `vtrn2q_s32` | transpose pairs | shift and blend | 2 |
| `vsliq_n_s32`, `vsriq_n_s32` | shift and insert | shift, and, or (or blend) | 2 to 3 |
| `vsraq_n_s32` | shift right and accumulate | shift, add | 2 |
| `vshrn_n_s64`, `vaddhn_s64` | shift (or add) and narrow | shift, then a shuffle | 2 to 3 |
| `vmovl_high_s32` | widen lanes 2 and 3 | `pshufd`, `pmovsxdq` | 2 |
| `vpaddq_s32` | pairwise add of adjacent lanes | `phaddd` (3 µops) | 1 (3 µops) |
| `vaddvq_s32`, `vmaxvq_u32` | reduce across lanes | shuffles and adds; for the zero test, `ptest` (note 7) | 3 to 4 |
| `vld2q_s32`, `vst2q_s32` | load or store with de-interleave | load, then `shufps` twice | 3 |
| `vabdq_s32` | absolute difference | subtract both ways and max, or subtract and abs (with overflow care) | 2 to 3 |

## x86 operations not allowed here (NEON lacks them)

| x86 | why it is out | what is used instead |
|---|---|---|
| mask registers, masked operations, compares into k | rule (NEON compares give full-lane masks) | compare masks and `vbsl` |
| `vpternlogd` | rule (no three-input logic) | two-input logic |
| `vpmullq` | rule (no 64-bit lane multiply) | widening 32 × 32 → 64 |
| `vpminsq`, `vpmaxsq`, `vpminuq`, `vpmaxuq` | NEON has no 64-bit min or max | compare and select |
| `vpopcntd`, `vpopcntq` | NEON counts bits per byte only | `vcntq_u8`, then pairwise adds |
| `vplzcntq` | NEON's clz has no 64-bit lanes | |
| `vpconflictd`, `vpcompressd`, `vpexpandd` | no NEON form | |
| `vprold`, `vprolvd` (rotate) | no NEON rotate | shift and insert |
| `vpshldv`, `vpshrdv` (funnel shifts) | no NEON form | |
| `vpmadd52luq` (IFMA) | no NEON form | |
| `vpmaddwd`, `vpmaddubsw`, `vpsadbw` | no single NEON form (smull and pairwise add) | |
| `vpmulhw`, `vpmulhuw`, `vpmulhrsw` used as a plain high half | NEON's 16-bit high multiply doubles (`vqdmulhq_s16`) | only where the doubling is accounted for |
| `psignd` | no NEON form | xor with a mask, then subtract (2) |
| `blendv_ps` relying on the sign bit only | NEON's select is bitwise | full-lane masks |
| `pblendw`, `vpblendd` with an immediate | no NEON form for an arbitrary lane pattern | `vuzp`, `vzip`, `vtrn`, `vbsl` with a constant mask |
| `vpermd`, `vpermt2d` as general permutes | NEON uses `tbl` on bytes | `vqtbl1q_u8` or zips |
| 256 and 512-bit registers | rule | 128-bit only |
| floating point | rule | |
