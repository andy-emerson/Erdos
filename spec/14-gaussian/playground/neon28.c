/* Study 28: NEON-shaped kernels on x86 (AVX-512VL in 128-bit registers as a NEON stand-in).
 *
 * Rules (decision 34, portable-isa.md): 128-bit registers; only operations NEON also has; no mask registers
 * (compare results are the masks); no 64-bit lane multiply; no three-input logic; no floating point.
 * Every vector operation below is a macro named after the NEON intrinsic it stands for; the body is the x86
 * stand-in. Built with -DCOUNT, each macro also counts one NEON instruction (nops; loads and stores in nmem),
 * which gives NEON instruction counts per element independent of how many x86 instructions the stand-in takes.
 *
 * Parts:
 *   M2  add, subtract, multiply of Gaussian arrays (separate re/im arrays, int32 halves), full-range overflow
 *       checks with a sticky flag (study 15), NEON-shaped.
 *   M3  Montgomery against Barrett for the Gaussian product mod n: widening forms and 32-bit high-half forms
 *       (vqdmulh, vqrdmulh; emulated here).
 *   M4  the (1+i)-ary binary gcd with ctz from the lane clz, interleaved 1 to 8 registers, against study 27's
 *       nbrd at 128 bits.
 *
 * Two gcc habits are blocked: logic ops pass through an empty asm (else gcc fuses them into vpternlog), and
 * compares and bsl are pinned to their VEX forms by inline asm (else gcc uses mask registers for values in
 * xmm16..31). After a build, scan the kernels' instructions against portable-isa.md.
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -o neon28 neon28.c
 *        (add -DCOUNT -o neon28c for the NEON instruction counts)
 * Run:   ./neon28 arith-check | arith-time | mod-check | mod-time | gcd-check | gcd-time | count
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int32_t i32; typedef uint32_t u32; typedef int64_t i64; typedef uint64_t u64;
typedef __int128 i128; typedef unsigned __int128 u128;
typedef __m128i V;
#define NOINL __attribute__((noinline))
#define AI static inline __attribute__((always_inline))

/* ======================= the NEON layer ======================= */
#ifdef COUNT
static u64 nops, nmem;
#define C_(e) (nops++, (e))
#define M_(e) (nmem++, (e))
#else
#define C_(e) (e)
#define M_(e) (e)
#endif
#define vld1q(p)            M_(_mm_loadu_si128((const V *)(p)))
#define vst1q(p, v)         M_(_mm_storeu_si128((V *)(p), v))
#define vdupq_n_s32(x)      _mm_set1_epi32((int)(x))            /* constants: hoisted, not counted */
#define vdupq_n_s64(x)      _mm_set1_epi64x((long long)(x))
#define vzero()             _mm_setzero_si128()
#define vaddq_s32(a, b)     C_(_mm_add_epi32(a, b))
#define vsubq_s32(a, b)     C_(_mm_sub_epi32(a, b))
#define vaddq_s64(a, b)     C_(_mm_add_epi64(a, b))
#define vsubq_s64(a, b)     C_(_mm_sub_epi64(a, b))
#define vnegq_s32(a)        C_(_mm_sub_epi32(_mm_setzero_si128(), a))
#define vabsq_s32(a)        C_(_mm_abs_epi32(a))
/* Logic ops pass through an empty asm so that gcc cannot fuse two of them into vpternlog (three-input logic,
 * which NEON lacks); the barrier costs nothing but that freedom. */
#define BAR_(e) ({ V r_ = (e); __asm__("" : "+v"(r_)); r_; })
#define vandq(a, b)         C_(BAR_(_mm_and_si128(a, b)))
#define vorrq(a, b)         C_(BAR_(_mm_or_si128(a, b)))
#define veorq(a, b)         C_(BAR_(_mm_xor_si128(a, b)))
#define vbicq(a, b)         C_(BAR_(_mm_andnot_si128(b, a)))     /* a AND NOT b */
#define vmvnq(a)            C_(BAR_(_mm_xor_si128(a, _mm_set1_epi32(-1))))
#define vshlq_n_s32(a, n)   C_(_mm_slli_epi32(a, n))
#define vshrq_n_s32(a, n)   C_(_mm_srai_epi32(a, n))
#define vshrq_n_u32(a, n)   C_(_mm_srli_epi32(a, n))
#define vshlq_n_s64(a, n)   C_(_mm_slli_epi64(a, n))
#define vshrq_n_s64(a, n)   C_(_mm_srai_epi64(a, n))
#define vshrq_n_u64(a, n)   C_(_mm_srli_epi64(a, n))
#define vshrq_n_u16(a, n)   C_(_mm_srli_epi16(a, n))
/* vshlq_s32/u32 with a lane count: NEON shifts right by a negative count. The kernels produce the count
 * that NEON would use at the same cost (see portable-isa.md note 1); here the right shifts take k >= 0. */
#define vshlq_s32(a, k)     C_(_mm_sllv_epi32(a, k))             /* left by k */
#define vshrq_s32_v(a, k)   C_(_mm_srav_epi32(a, k))             /* NEON: vshlq_s32(a, -k) */
#define vshrq_u32_v(a, k)   C_(_mm_srlv_epi32(a, k))             /* NEON: vshlq_u32(a, -k) */
#define vshrq_u64_v(a, k)   C_(_mm_srlv_epi64(a, k))             /* NEON: vshlq_u64(a, -k) */
/* Compares and selects are pinned to their VEX forms with inline asm. Left to itself, gcc turns them into
 * compares into mask registers and masked moves whenever values sit in xmm16..31 (the VEX forms reach only
 * xmm0..15), which breaks the no-mask-register rule. The pinned forms model NEON's one-instruction compare and
 * bsl, at the price of the 16-register limit on their operands (noted in portable-isa.md). */
#define ASM2_(ins, a, b) ({ V r_; __asm__(ins " %2, %1, %0" : "=x"(r_) : "x"(a), "x"(b)); r_; })
AI V cmpeq32_(V a, V b) { return ASM2_("vpcmpeqd", a, b); }
AI V cmpgt32_(V a, V b) { return ASM2_("vpcmpgtd", a, b); }     /* a > b */
AI V cmpeq64_(V a, V b) { return ASM2_("vpcmpeqq", a, b); }
AI V cmpgt64_(V a, V b) { return ASM2_("vpcmpgtq", a, b); }
AI V bsl_(V m, V a, V b) { V r_; __asm__("vpblendvb %3, %2, %1, %0" : "=x"(r_) : "x"(b), "x"(a), "x"(m)); return r_; }  /* m ? a : b */
#define vceqq_s32(a, b)     C_(cmpeq32_(a, b))
#define vceqzq_s32(a)       C_(cmpeq32_(a, _mm_setzero_si128()))
#define vcgtq_s32(a, b)     C_(cmpgt32_(a, b))
#define vcltq_s32(a, b)     C_(cmpgt32_(b, a))
#define vceqq_s64(a, b)     C_(cmpeq64_(a, b))
#define vcgtq_s64(a, b)     C_(cmpgt64_(a, b))
/* unsigned compare: one NEON op (cmhi); x86 has none without masks: xor the sign bit into both, pcmpgtd */
#define vcgtq_u32(a, b)     C_(cmpgt32_(BAR_(_mm_xor_si128(a, _mm_set1_epi32(INT32_MIN))), BAR_(_mm_xor_si128(b, _mm_set1_epi32(INT32_MIN)))))
/* vtst: one NEON op; x86: and, compare with zero, not */
#define vtstq_s32(a, b)     C_(BAR_(_mm_xor_si128(cmpeq32_(BAR_(_mm_and_si128(a, b)), _mm_setzero_si128()), _mm_set1_epi32(-1))))
#define vminq_s32(a, b)     C_(_mm_min_epi32(a, b))
#define vminq_u32(a, b)     C_(_mm_min_epu32(a, b))
#define vmaxq_u32(a, b)     C_(_mm_max_epu32(a, b))
#define vbslq(m, a, b)      C_(bsl_(m, a, b))                    /* m ? a : b, full-lane masks only */
#define vclzq_u32(a)        C_(_mm_lzcnt_epi32(a))
#define vqtbl1q_u8(t, i)    C_(_mm_shuffle_epi8(t, i))
#define vmulq_s32(a, b)     C_(_mm_mullo_epi32(a, b))
#define vmulq_s16(a, b)     C_(_mm_mullo_epi16(a, b))
#define vmlaq_s32(c, a, b)  C_(_mm_add_epi32(c, _mm_mullo_epi32(a, b)))
#define vmlsq_s32(c, a, b)  C_(_mm_sub_epi32(c, _mm_mullo_epi32(a, b)))
/* is any lane nonzero (for a branch): NEON umaxv + fmov + compare; x86 ptest. Counted as 2. */
AI int vany_(V a) { return !_mm_testz_si128(a, a); }
AI int vany_and_(V a, V m) { return !_mm_testz_si128(a, m); }
#define vany(a)             C_(C_(0) + vany_(a))
#define vany_and(a, m)      C_(C_(0) + vany_and_(a, m))
/* Widening multiply pairs. NEON: vmull_s32(vget_low) takes lanes 0, 1 and vmull_high_s32 lanes 2, 3, and
 * vuzp1q_s32 of the two products gives the low halves in lane order. x86: pmuldq takes lanes 0, 2; the "high"
 * stand-in shifts each operand right 32 (lanes 1, 3), and the narrowing stand-in re-interleaves even and odd.
 * Every use below is lane-order independent (the checks OR all lanes). */
#define ODD_(a)                 _mm_srli_epi64(a, 32)
#define vmull_lo_s32(a, b)      C_(_mm_mul_epi32(a, b))
#define vmull_high_s32(a, b)    C_(_mm_mul_epi32(ODD_(a), ODD_(b)))
#define vmull_lo_u32(a, b)      C_(_mm_mul_epu32(a, b))
#define vmull_high_u32(a, b)    C_(_mm_mul_epu32(ODD_(a), ODD_(b)))
#define vmlal_lo_s32(c, a, b)   C_(_mm_add_epi64(c, _mm_mul_epi32(a, b)))
#define vmlal_high_s32(c, a, b) C_(_mm_add_epi64(c, _mm_mul_epi32(ODD_(a), ODD_(b))))
#define vmlsl_lo_s32(c, a, b)   C_(_mm_sub_epi64(c, _mm_mul_epi32(a, b)))
#define vmlsl_high_s32(c, a, b) C_(_mm_sub_epi64(c, _mm_mul_epi32(ODD_(a), ODD_(b))))
#define vmlal_lo_u32(c, a, b)   C_(_mm_add_epi64(c, _mm_mul_epu32(a, b)))
#define vmlal_high_u32(c, a, b) C_(_mm_add_epi64(c, _mm_mul_epu32(ODD_(a), ODD_(b))))
#define vmlsl_lo_u32(c, a, b)   C_(_mm_sub_epi64(c, _mm_mul_epu32(a, b)))
#define vmlsl_high_u32(c, a, b) C_(_mm_sub_epi64(c, _mm_mul_epu32(ODD_(a), ODD_(b))))
#define vuzp1q_w(lo, hi)        C_(_mm_blend_epi32(lo, _mm_slli_epi64(hi, 32), 0xA))   /* low halves of a pair */
#define vuzp2q_w(lo, hi)        C_(_mm_blend_epi32(_mm_srli_epi64(lo, 32), hi, 0xA))   /* high halves of a pair */
/* vqmovn_s64 + vqmovn_high_s64 on a widening pair: saturating narrow, sets the sticky QC bit on saturation.
 * Stand-in: the value is the truncation where nothing saturates; QC is emulated in a variable. Counted as 2. */
#define QCSAT64_(t, qc)  ((qc) = BAR_(_mm_or_si128(qc, _mm_add_epi64(t, _mm_set1_epi64x(1LL << 31)))))
#define vqmovn_pair(lo, hi, qc) (C_(0), C_((QCSAT64_(lo, qc), QCSAT64_(hi, qc), _mm_blend_epi32(lo, _mm_slli_epi64(hi, 32), 0xA))))
#define QC_MASK64 _mm_set1_epi64x((long long)0xFFFFFFFF00000000ull)
/* vqaddq_s32 / vqsubq_s32: saturating; QC emulated as the OR of the overflow masks (sign bit) */
AI V qadd_(V a, V b, V *qc) { V r = _mm_add_epi32(a, b), m = _mm_srai_epi32(BAR_(_mm_andnot_si128(BAR_(_mm_xor_si128(a, b)), BAR_(_mm_xor_si128(a, r)))), 31);
    *qc = BAR_(_mm_or_si128(*qc, m)); return _mm_blendv_epi8(r, _mm_xor_si128(_mm_srai_epi32(a, 31), _mm_set1_epi32(INT32_MAX)), m); }
AI V qsub_(V a, V b, V *qc) { V r = _mm_sub_epi32(a, b), m = _mm_srai_epi32(BAR_(_mm_and_si128(BAR_(_mm_xor_si128(a, b)), BAR_(_mm_xor_si128(a, r)))), 31);
    *qc = BAR_(_mm_or_si128(*qc, m)); return _mm_blendv_epi8(r, _mm_xor_si128(_mm_srai_epi32(a, 31), _mm_set1_epi32(INT32_MAX)), m); }
#define vqaddq_s32(a, b, qc) C_(qadd_(a, b, &(qc)))
#define vqsubq_s32(a, b, qc) C_(qsub_(a, b, &(qc)))
/* vqdmulhq_s32: high half of 2ab (saturating only for a = b = -2^31, which the kernels never give).
 * No x86 equivalent: pmuldq on even and odd lanes, double, take the high dwords. */
AI V qdmulh_(V a, V b) {
    V e = _mm_mul_epi32(a, b), o = _mm_mul_epi32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32));
    return _mm_blend_epi32(_mm_srli_epi64(_mm_add_epi64(e, e), 32), _mm_add_epi64(o, o), 0xA);
}
/* vqrdmulhq_s32: (2ab + 2^31) >> 32 */
AI V qrdmulh_(V a, V b) {
    const V r = _mm_set1_epi64x(1LL << 31);
    V e = _mm_mul_epi32(a, b), o = _mm_mul_epi32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32));
    return _mm_blend_epi32(_mm_srli_epi64(_mm_add_epi64(_mm_add_epi64(e, e), r), 32), _mm_add_epi64(_mm_add_epi64(o, o), r), 0xA);
}
#define vqdmulhq_s32(a, b)  C_(qdmulh_(a, b))
#define vqrdmulhq_s32(a, b) C_(qrdmulh_(a, b))
/* vhsubq_s32: (a - b) >> 1 without overflow: (a >> 1) - (b >> 1) - (b AND NOT a AND 1) */
AI V hsub_(V a, V b) { return _mm_sub_epi32(_mm_sub_epi32(_mm_srai_epi32(a, 1), _mm_srai_epi32(b, 1)),
                                            BAR_(_mm_and_si128(BAR_(_mm_andnot_si128(a, b)), _mm_set1_epi32(1)))); }
#define vhsubq_s32(a, b)    C_(hsub_(a, b))

/* ======================= common: random numbers, clock ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i64 rr(i64 lim) { return (i64)(next() % (u64)(2 * lim + 1)) - lim; }
static u64 now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec; }
static int cmpu(const void *a, const void *b) { u64 x = *(const u64 *)a, y = *(const u64 *)b; return (x > y) - (x < y); }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

/* ======================= M2: add, subtract, multiply with overflow checks ======================= */
/* Layout (b): x = (ar, ai), y = (br, bi), result (cr, ci). N a multiple of 8 (two registers per pass).
 * Return: 1 when some result does not fit int32 (the sticky flag), else 0. The bare kernels return 0. */
#define AARGS const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
              i32 *restrict cr, i32 *restrict ci, size_t N
typedef int (*akf)(AARGS);
#define SIGNM vdupq_n_s32(INT32_MIN)

#define ABARE(NAME, OP)                                                                              \
NOINL int NAME(AARGS) {                                                                              \
    for (size_t i = 0; i < N; i += 8) {                                                              \
        vst1q(cr + i, OP(vld1q(ar + i), vld1q(br + i))); vst1q(ci + i, OP(vld1q(ai + i), vld1q(bi + i)));         \
        vst1q(cr + i + 4, OP(vld1q(ar + i + 4), vld1q(br + i + 4))); vst1q(ci + i + 4, OP(vld1q(ai + i + 4), vld1q(bi + i + 4))); \
    }                                                                                                \
    return 0;                                                                                        \
}
ABARE(add_bare, vaddq_s32)
ABARE(sub_bare, vsubq_s32)

/* study 15: add overflow (x XOR r) AND NOT (x XOR y), subtract (x XOR r) AND (x XOR y), at the sign bit */
#define OVA(x, y, r) vbicq(veorq(x, r), veorq(x, y))
#define OVS(x, y, r) vandq(veorq(x, r), veorq(x, y))
#define AXOR(NAME, OP, OVF)                                                                          \
NOINL int NAME(AARGS) {                                                                              \
    V ov = vzero();                                                                                  \
    for (size_t i = 0; i < N; i += 8) {                                                              \
        _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4) {                                    \
            V x = vld1q(ar + i + h), y = vld1q(br + i + h), u = vld1q(ai + i + h), v = vld1q(bi + i + h);  \
            V r = OP(x, y), q = OP(u, v);                                                            \
            vst1q(cr + i + h, r); vst1q(ci + i + h, q);                                              \
            ov = vorrq(ov, vorrq(OVF(x, y, r), OVF(u, v, q)));                                       \
        }                                                                                            \
    }                                                                                                \
    return vany_and(ov, SIGNM);                                                                      \
}
AXOR(add_xor, vaddq_s32, OVA)
AXOR(sub_xor, vsubq_s32, OVS)

/* saturating forms. (s) compare the saturated sum with the wrapped one: qadd, add, xor, or.
 * (qc) the saturating op alone; the hardware's sticky QC bit (FPSR) is the flag, read once per array.
 * On overflow the operation is redone at the next size from the inputs, so the saturated value is never used.
 * x86 has no 32-bit saturating add: these are emulated (correctness and NEON counts only, not timed). */
#define ASAT(NAME, QOP, OP)                                                                          \
NOINL int NAME(AARGS) {                                                                              \
    V ov = vzero(), qc = vzero();                                                                    \
    for (size_t i = 0; i < N; i += 8) {                                                              \
        _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4) {                                    \
            V x = vld1q(ar + i + h), y = vld1q(br + i + h), u = vld1q(ai + i + h), v = vld1q(bi + i + h);  \
            V r = OP(x, y), q = OP(u, v);                                                            \
            vst1q(cr + i + h, r); vst1q(ci + i + h, q);                                              \
            ov = vorrq(ov, vorrq(veorq(QOP(x, y, qc), r), veorq(QOP(u, v, qc), q)));                 \
        }                                                                                            \
    }                                                                                                \
    return vany(ov);                                                                                 \
}
ASAT(add_sat, vqaddq_s32, vaddq_s32)
ASAT(sub_sat, vqsubq_s32, vsubq_s32)
#define AQC(NAME, QOP)                                                                               \
NOINL int NAME(AARGS) {                                                                              \
    V qc = vzero();                                                                                  \
    for (size_t i = 0; i < N; i += 8) {                                                              \
        _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4) {                                    \
            vst1q(cr + i + h, QOP(vld1q(ar + i + h), vld1q(br + i + h), qc));                       \
            vst1q(ci + i + h, QOP(vld1q(ai + i + h), vld1q(bi + i + h), qc));                       \
        }                                                                                            \
    }                                                                                                \
    return vany_and(qc, SIGNM);    /* NEON: mrs FPSR, test QC (once per array) */                   \
}
AQC(add_qc, vqaddq_s32)
AQC(sub_qc, vqsubq_s32)

/* multiply, wrapping (reference): re = ac - bd, im = ad + bc mod 2^32 */
NOINL int mul_bare(AARGS) {
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        vst1q(cr + i, vmlsq_s32(vmulq_s32(a, c), b, d)); vst1q(ci + i, vmlaq_s32(vmulq_s32(a, d), b, c));
    }
    return 0;
}
/* the four 64-bit parts of a register of 4 products (NEON: smull, smlsl, smull2, smlsl2, ...) */
#define MUL_PARTS(a, b, c, d)                                                                        \
    V rl = vmlsl_lo_s32(vmull_lo_s32(a, c), b, d), rh = vmlsl_high_s32(vmull_high_s32(a, c), b, d);  \
    V il = vmlal_lo_s32(vmull_lo_s32(a, d), b, c), ih = vmlal_high_s32(vmull_high_s32(a, d), b, c);
/* (w) study 15's add form: a part v fits iff v + 2^31 has nothing in bits 32..63; OR, mask once per array.
 * The corner (-2^31)(-2^31) 2 = 2^63 wraps to -2^63 and is still caught. */
NOINL int mul_w(AARGS) {
    const V off = vdupq_n_s64(1LL << 31); V ov = vzero();
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        MUL_PARTS(a, b, c, d)
        ov = vorrq(ov, vorrq(vorrq(vaddq_s64(rl, off), vaddq_s64(rh, off)), vorrq(vaddq_s64(il, off), vaddq_s64(ih, off))));
        vst1q(cr + i, vuzp1q_w(rl, rh)); vst1q(ci + i, vuzp1q_w(il, ih));
    }
    return vany_and(ov, QC_MASK64);
}
/* (h) high halves against the sign of the low halves: uzp2, then hi XOR (lo >> 31) */
NOINL int mul_h(AARGS) {
    V ov = vzero();
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        MUL_PARTS(a, b, c, d)
        V re = vuzp1q_w(rl, rh), im = vuzp1q_w(il, ih);
        ov = vorrq(ov, vorrq(veorq(vuzp2q_w(rl, rh), vshrq_n_s32(re, 31)), veorq(vuzp2q_w(il, ih), vshrq_n_s32(im, 31))));
        vst1q(cr + i, re); vst1q(ci + i, im);
    }
    return vany(ov);
}
/* (qc) saturating narrow (sqxtn, sqxtn2) in place of uzp1; QC is the flag. Not timed (emulated). */
NOINL int mul_qc(AARGS) {
    V qc = vzero();
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        MUL_PARTS(a, b, c, d)
        vst1q(cr + i, vqmovn_pair(rl, rh, qc)); vst1q(ci + i, vqmovn_pair(il, ih, qc));
    }
    return vany_and(qc, QC_MASK64);
}

typedef struct { const char *name; akf f; int op; int checked; int timed; } akern;   /* op: 0 add, 1 sub, 2 mul */
static akern AK[] = {
    {"add_bare", add_bare, 0, 0, 1}, {"add_xor", add_xor, 0, 1, 1}, {"add_sat", add_sat, 0, 1, 0}, {"add_qc", add_qc, 0, 1, 0},
    {"sub_bare", sub_bare, 1, 0, 1}, {"sub_xor", sub_xor, 1, 1, 1}, {"sub_sat", sub_sat, 1, 1, 0}, {"sub_qc", sub_qc, 1, 1, 0},
    {"mul_bare", mul_bare, 2, 0, 1}, {"mul_w", mul_w, 2, 1, 1}, {"mul_h", mul_h, 2, 1, 1}, {"mul_qc", mul_qc, 2, 1, 0},
};
#define NAK (int)(sizeof AK / sizeof AK[0])

/* exact reference: true parts and whether they fit */
static void aref(int op, i64 a, i64 b, i64 c, i64 d, i64 *re, i64 *im) {
    if (op == 0) { *re = a + c; *im = b + d; }
    else if (op == 1) { *re = a - c; *im = b - d; }
    else { *re = a * c - b * d; *im = a * d + b * c; }        /* |.| <= 2^63; the corner 2^63 is handled below */
}
static int fits32(i128 v) { return v >= INT32_MIN && v <= INT32_MAX; }
static long afails, acases[NAK], aovf[NAK];
#define ABM 4096
static i32 XA[ABM + 8], XB[ABM + 8], XC[ABM + 8], XD[ABM + 8], YR[ABM + 8], YI[ABM + 8];
/* run every kernel on XA..XD (n elements, padded with zeros to a multiple of 8), compare values and flag */
static void acheck(size_t n) {
    size_t Np = (n + 7) / 8 * 8;
    for (size_t j = n; j < Np; j++) XA[j] = XB[j] = XC[j] = XD[j] = 0;
    for (int k = 0; k < NAK; k++) {
        int op = AK[k].op, fl = AK[k].f(XA, XB, XC, XD, YR, YI, Np), any = 0, valbad = 0;
        for (size_t j = 0; j < n; j++) {
            i128 re, im;
            if (op == 2) { re = (i128)XA[j] * XC[j] - (i128)XB[j] * XD[j]; im = (i128)XA[j] * XD[j] + (i128)XB[j] * XC[j]; }
            else { i64 r, q; aref(op, XA[j], XB[j], XC[j], XD[j], &r, &q); re = r; im = q; }
            int bad = !fits32(re) || !fits32(im); any |= bad;
            /* values: mod 2^32 always for the wrapping kernels; the saturating ones only where nothing overflowed */
            int sat = !strcmp(AK[k].name + 4, "sat") || !strcmp(AK[k].name + 4, "qc");
            if ((!sat || !bad) && (YR[j] != (i32)(u32)(u64)re || YI[j] != (i32)(u32)(u64)im)) valbad = 1;
        }
        acases[k] += n; aovf[k] += any;
        if (valbad || (AK[k].checked && fl != any)) if (afails++ < 10)
            printf("FAIL %s n=%zu flag %d want %d values %s (%d,%d)(%d,%d)\n", AK[k].name, n, fl, any, valbad ? "bad" : "ok", XA[0], XB[0], XC[0], XD[0]);
    }
}
/* one element alone among 8 (others zero), at position pos */
static void alone(i32 a, i32 b, i32 c, i32 d, int pos) {
    memset(XA, 0, 32); memset(XB, 0, 32); memset(XC, 0, 32); memset(XD, 0, 32);
    XA[pos] = a; XB[pos] = b; XC[pos] = c; XD[pos] = d; acheck(8);
}
static i32 AEDGE[64]; static int NAEDGE;
static void make_edges(void) {
    int n = 0; AEDGE[n++] = 0; AEDGE[n++] = 1; AEDGE[n++] = -1;
    int ks[4] = {14, 15, 30, 31};
    for (int t = 0; t < 4; t++) {
        i64 p = 1LL << ks[t];
        for (i64 v = p - 2; v <= p + 1; v++) if (v <= INT32_MAX) AEDGE[n++] = (i32)v;
        for (i64 v = -p - 1; v <= -p + 2; v++) if (v >= INT32_MIN) AEDGE[n++] = (i32)v;
    }
    NAEDGE = n;
}
static void do_arith_check(void) {
    make_edges();
    /* (1) every quadruple of edge values, each element alone, position rotating */
    long cnt = 0; int pos = 0;
    for (int p = 0; p < NAEDGE; p++) for (int q = 0; q < NAEDGE; q++) for (int r = 0; r < NAEDGE; r++) for (int s = 0; s < NAEDGE; s++) {
        alone(AEDGE[p], AEDGE[q], AEDGE[r], AEDGE[s], pos); pos = (pos + 1) & 7; cnt++;
    }
    printf("edges: %d values, every quadruple alone (%ld); failures so far %ld\n", NAEDGE, cnt, afails);
    /* (2) h = 8 style, exhaustive in the top bits: v = (t << S) | f, t every 8-bit two's complement pattern,
     * f one of 5 fills; every pair (x part, y part) for the real parts with the imaginary parts from an edge
     * pair cycling, then the same with the roles swapped. S = 24 for add and sub (the carry into bit 31),
     * S = 12 and 16 for mul (products from 2^24 to 2^46, across 2^31). */
    static const i32 FILL24[5] = {0, 1, 0x7FFFFF, 0x800000, 0xFFFFFF};
    int Ss[3] = {24, 12, 16};
    for (int si = 0; si < 3; si++) {
        int S = Ss[si]; static i32 H[1280]; int nh = 0;
        for (int t = -128; t < 128; t++) for (int f = 0; f < 5; f++) {
            i64 fill = FILL24[f] & ((1LL << S) - 1);
            if (f == 3) fill = 1LL << (S - 1);
            H[nh++] = (i32)(((i64)t << S) | fill);
        }
        long c2 = 0; int e = 0;
        for (int sw = 0; sw < 2; sw++)
            for (int x = 0; x < nh; x++) {
                size_t n = 0;
                for (int y = 0; y < nh; y++) {
                    i32 o1 = AEDGE[e % NAEDGE], o2 = AEDGE[(e / NAEDGE) % NAEDGE]; e += 7;
                    if (!sw) { XA[n] = H[x]; XC[n] = H[y]; XB[n] = o1; XD[n] = o2; }
                    else { XB[n] = H[x]; XD[n] = H[y]; XA[n] = o1; XC[n] = o2; }
                    n++; c2++;
                    if (n == 8) { /* per element flags: run each alone too, every 4th group */
                        acheck(8);
                        if ((y & 31) == 7) {
                            i32 g[4][8]; memcpy(g[0], XA, 32); memcpy(g[1], XB, 32); memcpy(g[2], XC, 32); memcpy(g[3], XD, 32);
                            for (int j = 0; j < 8; j++) alone(g[0][j], g[1][j], g[2][j], g[3][j], j);
                        }
                        n = 0;
                    }
                }
                if (n) acheck(n);
            }
        printf("top-bit exhaustive, S = %d: %d values, %ld element pairs (groups of 8, every 4th group also alone); failures so far %ld\n", S, nh, c2, afails);
    }
    /* (3) random arrays: 8 to 128 elements; mixes of full-range, near-boundary and small values, a planted bad one */
    long ca = 0;
    for (int it = 0; it < 200000; it++) {
        size_t n = 8 + next() % 121; int kind = (int)(next() % 4);
        for (size_t j = 0; j < n; j++) {
            i32 *p[4] = {XA + j, XB + j, XC + j, XD + j};
            for (int t = 0; t < 4; t++) {
                i64 v;
                if (kind == 0) v = (i32)next();
                else if (kind == 1) v = AEDGE[next() % NAEDGE] + rr(3);
                else if (kind == 2) v = rr(1 << 15);
                else v = rr(1LL << (next() % 31));
                if (v > INT32_MAX) v = INT32_MAX; if (v < INT32_MIN) v = INT32_MIN;
                *p[t] = (i32)v;
            }
        }
        if (kind >= 2 && (next() & 3) == 0) { size_t j = next() % n; XA[j] = (i32)next(); XC[j] = (i32)next(); }
        acheck(n); ca++;
    }
    printf("random arrays: %ld (8 to 128 elements)\n", ca);
    for (int k = 0; k < NAK; k++) printf("  %-9s elements %9ld, calls with an overflow %8ld\n", AK[k].name, acases[k] ? acases[k] : 0, aovf[k]);
    printf("TOTAL failures %ld\n", afails);
}

/* timing: L1, L2, memory; kernels of the timed set interleaved, order rotated, an untimed call first */
#define REPS 31
static void do_arith_time(size_t big) {
    size_t sizes[3] = {512, 16384, big}, inner[3] = {4096, 128, 1};
    const char *sn[3] = {"L1", "L2", "memory"};
    i32 *buf[6], *p[6];
    for (int j = 0; j < 6; j++) { buf[j] = aligned_alloc(4096, (big + 4096) * 4); p[j] = buf[j] + 144 * j; }   /* page offsets j*576 bytes */
    int tk[NAK], nt = 0; for (int k = 0; k < NAK; k++) if (AK[k].timed) tk[nt++] = k;
    printf("ns per element (one Gaussian integer); median [q1, q3] over %d reps\n", REPS);
    for (int s = 0; s < 3; s++) {
        size_t N = sizes[s];
        for (size_t j = 0; j < N; j++) for (int t = 0; t < 4; t++) p[t][j] = (i32)rr(1 << 14);   /* small: no overflow */
        static double t[NAK][REPS];
        for (int k = 0; k < nt; k++) AK[tk[k]].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < nt; j++) {
                int k = tk[(j + r) % nt];
                AK[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                u64 t0 = now_ns();
                for (size_t it = 0; it < inner[s]; it++) AK[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                t[k][r] = (double)(now_ns() - t0) / (double)(N * inner[s]);
            }
        printf("%s (N = %zu)\n", sn[s], N);
        for (int j = 0; j < nt; j++) { int k = tk[j]; qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-9s %.4f [%.4f, %.4f]\n", AK[k].name, t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
        fflush(stdout);
    }
}

/* ======================= M3: Montgomery against Barrett mod n, NEON-shaped ======================= */
/* Residues u32 in [0, n), separate arrays. Kernels:
 *   P  x y mod n (arrays a, c)                    S  a k mod n, k one shared residue
 *   G  Gaussian product of arrays (a + bi)(c + di) K  Gaussian array times one shared Gaussian (kc + kd i)
 * Versions:
 *   div    scalar t % n (reference)
 *   mon_w  Montgomery, R = 2^32, widening: t in 64-bit lanes (smull/smlsl), m = lo(t) n' (mul), t - m n (smlsl),
 *          high halves (uzp2), + n where negative. Odd n < 2^31. Inputs and outputs in Montgomery form.
 *   mon_h  Montgomery with the 32-bit high-half multiply: h = sqdmulh(a, b), m = (a b) n' (two mul, or one when b
 *          is shared: b n' precomputed), t = sqdmulh(m, n), r = hsub(h, t) = (a b - m n) / 2^32 in (-n, n).
 *          2ab and 2mn agree in their low 33 bits, so h - t is even and the halving is exact.
 *   bar_w  study 19's lane Barrett (q = ((t >> s) mu) >> 32 or >> 31, r < 3n or 4n, 64-bit correction). Any n.
 *   bar_h  Barrett with the high-half multiply, no conversions, any n:
 *          P (n <= 2^30): h = sqdmulh(a << (31 - L), b) = floor(ab / 2^L), q = sqdmulh(2h, mu2) = floor(h mu2 / 2^30),
 *            mu2 = floor(2^(L+30) / n); q_true - 3 <= q <= q_true, so r = ab - q n (mul, mls, mod 2^32) is in [0, 4n).
 *          S, K: Barrett multiplication by a shared k (Becker et al. 2021): k' = round(k 2^31 / n),
 *            q = sqrdmulh(a, k'), r = a k - q n (mul, mls) in (-n, n).
 *   bar_h30  K for n <= 2^30: the two quotients of a part added before one mls, r in (-2n, 2n).
 * Conversions: to Montgomery form (REDC(x (R^2 mod n))) and back (REDC(x)), widening and high-half. */
typedef struct {
    u32 n, L, mu, mu2, ninv, r2, r2n; u64 n2;
    u32 k, kc, kd;                    /* shared constants, standard form */
    u32 km, kcm, kdm;                 /* Montgomery form */
    u32 kmn, kcmn, kdmn;              /* Montgomery form times n' mod 2^32 (for mon_h) */
    u32 kb, kcb, kdb;                 /* Barrett: round(x 2^31 / n) */
} mod_t;
static u32 mont_of(u32 x, u32 n) { return (u32)(((u64)x << 32) % n); }
static u32 bconst(u32 x, u32 n) { return (u32)((((u64)x << 32) / n + 1) >> 1); }     /* round(x 2^31 / n) */
static void mod_set_consts(mod_t *M, u32 k, u32 kc, u32 kd) {
    u32 n = M->n; M->k = k; M->kc = kc; M->kd = kd;
    if (n & 1) { M->km = mont_of(k, n); M->kcm = mont_of(kc, n); M->kdm = mont_of(kd, n);
                 M->kmn = M->km * M->ninv; M->kcmn = M->kcm * M->ninv; M->kdmn = M->kdm * M->ninv; }
    M->kb = bconst(k, n); M->kcb = bconst(kc, n); M->kdb = bconst(kd, n);
}
static mod_t mod_make(u32 n) {
    mod_t M; memset(&M, 0, sizeof M); M.n = n; M.n2 = (u64)n * n;
    u32 L = 0; while ((1ull << L) < n) L++;
    M.L = L; M.mu = (u32)((1ull << (L + 31)) / n); M.mu2 = (u32)((1ull << (L + 30)) / n);
    if (n & 1) { u32 inv = n; for (int i = 0; i < 5; i++) inv *= 2 - n * inv; M.ninv = inv;
                 M.r2 = (u32)(((u128)1 << 64) % n); M.r2n = M.r2 * inv; }
    mod_set_consts(&M, 1 % n, 1 % n, 0);
    return M;
}
#define MARGS const u32 *restrict a, const u32 *restrict b, const u32 *restrict c, const u32 *restrict d, \
              u32 *restrict re, u32 *restrict im, size_t N, const mod_t *M
typedef void (*mkf)(MARGS);
/* range fixes (unsigned min): (-n, n) -> [0, n) and [0, 2n) -> [0, n) */
AI V FIXNEG(V r, V nv) { return vminq_u32(r, vaddq_s32(r, nv)); }
AI V FIXGE(V r, V nv) { return vminq_u32(r, vsubq_s32(r, nv)); }
/* shift-narrow by 31 of a widening pair (NEON: shrn, shrn2) */
#define vshrn31_w(lo, hi) (C_(0), C_(_mm_blend_epi32(_mm_srli_epi64(lo, 31), _mm_slli_epi64(hi, 1), 0xA)))

/* ---------- div (scalar reference) ---------- */
NOINL void P_div(MARGS) { u64 n = M->n; (void)b; (void)d; (void)im; for (size_t i = 0; i < N; i++) re[i] = (u32)((u64)a[i] * c[i] % n); }
NOINL void G_div(MARGS) { u64 n = M->n, n2 = M->n2;
    for (size_t i = 0; i < N; i++) { u64 A = a[i], B = b[i], C = c[i], D = d[i];
        re[i] = (u32)((A * C + (n2 - B * D)) % n); im[i] = (u32)((A * D + B * C) % n); } }

/* ---------- mon_w ---------- */
AI V monw_red_s(V tl, V th, V nv, V ni) {         /* signed t, |t| < n^2 (or < n 2^31) */
    V m = vmulq_s32(vuzp1q_w(tl, th), ni);
    return FIXNEG(vuzp2q_w(vmlsl_lo_s32(tl, m, nv), vmlsl_high_s32(th, m, nv)), nv);
}
AI V monw_red_u(V tl, V th, V nv, V ni) {         /* unsigned t < n 2^32 */
    V m = vmulq_s32(vuzp1q_w(tl, th), ni);
    return FIXNEG(vuzp2q_w(vmlsl_lo_u32(tl, m, nv), vmlsl_high_u32(th, m, nv)), nv);
}
AI V monw_p_(V x, V y, V nv, V ni) { return monw_red_u(vmull_lo_u32(x, y), vmull_high_u32(x, y), nv, ni); }
#define MONW_P(x, y) monw_p_(x, y, nv, ni)
#define MONW_G(A_, B_, C_x, D_, pr, pi) { V A = A_, B = B_, C = C_x, D = D_;                         \
    vst1q(pr, monw_red_s(vmlsl_lo_s32(vmull_lo_s32(A, C), B, D), vmlsl_high_s32(vmull_high_s32(A, C), B, D), nv, ni)); \
    vst1q(pi, monw_red_u(vmlal_lo_u32(vmull_lo_u32(A, D), B, C), vmlal_high_u32(vmull_high_u32(A, D), B, C), nv, ni)); }
NOINL void P_mon_w(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, MONW_P(vld1q(a + i), vld1q(c + i))); }
NOINL void S_mon_w(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv), k = vdupq_n_s32(M->km);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, MONW_P(vld1q(a + i), k)); }
NOINL void G_mon_w(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv);
    for (size_t i = 0; i < N; i += 4) MONW_G(vld1q(a + i), vld1q(b + i), vld1q(c + i), vld1q(d + i), re + i, im + i) }
NOINL void K_mon_w(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv), kc = vdupq_n_s32(M->kcm), kd = vdupq_n_s32(M->kdm);
    for (size_t i = 0; i < N; i += 4) MONW_G(vld1q(a + i), vld1q(b + i), kc, kd, re + i, im + i) }
NOINL void to_mon_w(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv), r2 = vdupq_n_s32(M->r2);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, MONW_P(vld1q(a + i), r2)); }
NOINL void from_mon_w(MARGS) {                     /* t = x < 2^32: r = 0 - hi(m n) */
    V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv);
    for (size_t i = 0; i < N; i += 4) { V x = vld1q(a + i), m = vmulq_s32(x, ni);
        vst1q(re + i, FIXNEG(vnegq_s32(vuzp2q_w(vmull_lo_u32(m, nv), vmull_high_u32(m, nv))), nv)); } }

/* ---------- mon_h ---------- */
AI V monh_(V x, V y, V ni, V nv) { return vhsubq_s32(vqdmulhq_s32(x, y), vqdmulhq_s32(vmulq_s32(vmulq_s32(x, y), ni), nv)); }   /* (-n, n) */
AI V monhk_(V x, V k, V kn, V nv) { return vhsubq_s32(vqdmulhq_s32(x, k), vqdmulhq_s32(vmulq_s32(x, kn), nv)); }               /* k n' given */
#define MONH(x, y)      monh_(x, y, ni, nv)
#define MONHK(x, k, kn) monhk_(x, k, kn, nv)
NOINL void P_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv);
    for (size_t i = 0; i < N; i += 4) { V x = vld1q(a + i), y = vld1q(c + i); vst1q(re + i, FIXNEG(MONH(x, y), nv)); } }
NOINL void S_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), k = vdupq_n_s32(M->km), kn = vdupq_n_s32(M->kmn);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, FIXNEG(MONHK(vld1q(a + i), k, kn), nv)); }
NOINL void G_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv);
    for (size_t i = 0; i < N; i += 4) {
        V A = vld1q(a + i), B = vld1q(b + i), C = vld1q(c + i), D = vld1q(d + i);
        V ac = FIXNEG(MONH(A, C), nv), bd = FIXNEG(MONH(B, D), nv), ad = FIXNEG(MONH(A, D), nv), bc = FIXNEG(MONH(B, C), nv);
        vst1q(re + i, FIXNEG(vsubq_s32(ac, bd), nv)); vst1q(im + i, FIXGE(vaddq_s32(ad, bc), nv));
    } }
NOINL void K_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), kc = vdupq_n_s32(M->kcm), kd = vdupq_n_s32(M->kdm);
    V kcn = vdupq_n_s32(M->kcmn), kdn = vdupq_n_s32(M->kdmn);
    for (size_t i = 0; i < N; i += 4) {
        V A = vld1q(a + i), B = vld1q(b + i);
        V ac = FIXNEG(MONHK(A, kc, kcn), nv), bd = FIXNEG(MONHK(B, kd, kdn), nv), ad = FIXNEG(MONHK(A, kd, kdn), nv), bc = FIXNEG(MONHK(B, kc, kcn), nv);
        vst1q(re + i, FIXNEG(vsubq_s32(ac, bd), nv)); vst1q(im + i, FIXGE(vaddq_s32(ad, bc), nv));
    } }
NOINL void to_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), r2 = vdupq_n_s32(M->r2), r2n = vdupq_n_s32(M->r2n);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, FIXNEG(MONHK(vld1q(a + i), r2, r2n), nv)); }
NOINL void from_mon_h(MARGS) { V nv = vdupq_n_s32(M->n), ni = vdupq_n_s32(M->ninv), z = vzero();   /* h = sqdmulh(x, 1) = 0 */
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, FIXNEG(vhsubq_s32(z, vqdmulhq_s32(vmulq_s32(vld1q(a + i), ni), nv)), nv)); }

/* ---------- bar_w (study 19 in NEON shape) ---------- */
/* t in a widening pair -> r in [0, n). sh: L - 1 (P) or L (G); the product form takes q = high half, the
 * Gaussian form q = (x mu) >> 31. Then subtract 2n where r >= 2n (64-bit), narrow, subtract n where r >= n. */
AI V barw_red_(V tl, V th, int Q31, V sh, V mu, V nv, V tn, V tnm1) {
    V x_ = vuzp1q_w(vshrq_u64_v(tl, sh), vshrq_u64_v(th, sh));
    V q_ = Q31 ? vshrn31_w(vmull_lo_u32(x_, mu), vmull_high_u32(x_, mu)) : vuzp2q_w(vmull_lo_u32(x_, mu), vmull_high_u32(x_, mu));
    V rl_ = vmlsl_lo_u32(tl, q_, nv), rh_ = vmlsl_high_u32(th, q_, nv);
    rl_ = vsubq_s64(rl_, vandq(vcgtq_s64(rl_, tnm1), tn)); rh_ = vsubq_s64(rh_, vandq(vcgtq_s64(rh_, tnm1), tn));
    return FIXGE(vuzp1q_w(rl_, rh_), nv);
}
#define BARW_RED(tl, th, Q31) barw_red_(tl, th, Q31, sh, mu, nv, tn, tnm1)
#define BARW_SETUP(shift) V nv = vdupq_n_s32(M->n), mu = vdupq_n_s32(M->mu), sh = vdupq_n_s64(shift);     \
    V tn = vdupq_n_s64(2 * (u64)M->n), tnm1 = vdupq_n_s64(2 * (u64)M->n - 1); (void)tn;
#define BARW_G(A_, B_, C_x, D_, pr, pi) { V A = A_, B = B_, C = C_x, D = D_;                         \
    vst1q(pr, BARW_RED(vmlsl_lo_u32(vmlal_lo_u32(n2, A, C), B, D), vmlsl_high_u32(vmlal_high_u32(n2, A, C), B, D), 1)); \
    vst1q(pi, BARW_RED(vmlal_lo_u32(vmull_lo_u32(A, D), B, C), vmlal_high_u32(vmull_high_u32(A, D), B, C), 1)); }
NOINL void P_bar_w(MARGS) { BARW_SETUP(M->L - 1)
    for (size_t i = 0; i < N; i += 4) { V x = vld1q(a + i), y = vld1q(c + i); vst1q(re + i, BARW_RED(vmull_lo_u32(x, y), vmull_high_u32(x, y), 0)); } }
NOINL void S_bar_w(MARGS) { BARW_SETUP(M->L - 1) V k = vdupq_n_s32(M->k);
    for (size_t i = 0; i < N; i += 4) { V x = vld1q(a + i); vst1q(re + i, BARW_RED(vmull_lo_u32(x, k), vmull_high_u32(x, k), 0)); } }
NOINL void G_bar_w(MARGS) { BARW_SETUP(M->L) V n2 = vdupq_n_s64(M->n2);
    for (size_t i = 0; i < N; i += 4) BARW_G(vld1q(a + i), vld1q(b + i), vld1q(c + i), vld1q(d + i), re + i, im + i) }
NOINL void K_bar_w(MARGS) { BARW_SETUP(M->L) V n2 = vdupq_n_s64(M->n2), kc = vdupq_n_s32(M->kc), kd = vdupq_n_s32(M->kd);
    for (size_t i = 0; i < N; i += 4) BARW_G(vld1q(a + i), vld1q(b + i), kc, kd, re + i, im + i) }

/* ---------- bar_h ---------- */
NOINL void P_bar_h(MARGS) {          /* n <= 2^30 */
    V nv = vdupq_n_s32(M->n), tnv = vdupq_n_s32(2 * M->n), s = vdupq_n_s32(31 - M->L), mu2 = vdupq_n_s32(M->mu2);
    for (size_t i = 0; i < N; i += 4) {
        V x = vld1q(a + i), y = vld1q(c + i);
        V h = vqdmulhq_s32(vshlq_s32(x, s), y);
        V q = vqdmulhq_s32(vaddq_s32(h, h), mu2);
        V r = vmlsq_s32(vmulq_s32(x, y), q, nv);                 /* [0, 4n) */
        vst1q(re + i, FIXGE(FIXGE(r, tnv), nv));
    } }
AI V barh_(V x, V k, V kb, V nv) { return vmlsq_s32(vmulq_s32(x, k), vqrdmulhq_s32(x, kb), nv); }   /* (-n, n) */
#define BARH(x, k, kb) barh_(x, k, kb, nv)
NOINL void S_bar_h(MARGS) { V nv = vdupq_n_s32(M->n), k = vdupq_n_s32(M->k), kb = vdupq_n_s32(M->kb);
    for (size_t i = 0; i < N; i += 4) vst1q(re + i, FIXNEG(BARH(vld1q(a + i), k, kb), nv)); }
NOINL void K_bar_h(MARGS) { V nv = vdupq_n_s32(M->n), kc = vdupq_n_s32(M->kc), kd = vdupq_n_s32(M->kd), kcb = vdupq_n_s32(M->kcb), kdb = vdupq_n_s32(M->kdb);
    for (size_t i = 0; i < N; i += 4) {
        V A = vld1q(a + i), B = vld1q(b + i);
        V ac = FIXNEG(BARH(A, kc, kcb), nv), bd = FIXNEG(BARH(B, kd, kdb), nv), ad = FIXNEG(BARH(A, kd, kdb), nv), bc = FIXNEG(BARH(B, kc, kcb), nv);
        vst1q(re + i, FIXNEG(vsubq_s32(ac, bd), nv)); vst1q(im + i, FIXGE(vaddq_s32(ad, bc), nv));
    } }
NOINL void K_bar_h30(MARGS) {        /* n <= 2^30: one mls per part, r in (-2n, 2n) */
    V nv = vdupq_n_s32(M->n), tnv = vdupq_n_s32(2 * M->n), kc = vdupq_n_s32(M->kc), kd = vdupq_n_s32(M->kd), kcb = vdupq_n_s32(M->kcb), kdb = vdupq_n_s32(M->kdb);
    for (size_t i = 0; i < N; i += 4) {
        V A = vld1q(a + i), B = vld1q(b + i);
        V qr = vsubq_s32(vqrdmulhq_s32(A, kcb), vqrdmulhq_s32(B, kdb)), qi = vaddq_s32(vqrdmulhq_s32(A, kdb), vqrdmulhq_s32(B, kcb));
        V rr_ = vmlsq_s32(vmlsq_s32(vmulq_s32(A, kc), B, kd), qr, nv), ri = vmlsq_s32(vmlaq_s32(vmulq_s32(A, kd), B, kc), qi, nv);
        vst1q(re + i, FIXGE(vminq_u32(rr_, vaddq_s32(rr_, tnv)), nv)); vst1q(im + i, FIXGE(vminq_u32(ri, vaddq_s32(ri, tnv)), nv));
    } }

/* ---------- table ---------- */
/* kind: 0 P, 1 S, 2 G, 3 K, 4 to, 5 from. mont: inputs/outputs in Montgomery form. lim30: n <= 2^30 only. */
typedef struct { const char *name; mkf f; int kind, mont, oddonly, lim30; } mkern;
static mkern MK[] = {
    {"P div", P_div, 0, 0, 0, 0}, {"P mon_w", P_mon_w, 0, 1, 1, 0}, {"P mon_h", P_mon_h, 0, 1, 1, 0},
    {"P bar_w", P_bar_w, 0, 0, 0, 0}, {"P bar_h", P_bar_h, 0, 0, 0, 1},
    {"S mon_w", S_mon_w, 1, 1, 1, 0}, {"S mon_h", S_mon_h, 1, 1, 1, 0}, {"S bar_w", S_bar_w, 1, 0, 0, 0}, {"S bar_h", S_bar_h, 1, 0, 0, 0},
    {"G div", G_div, 2, 0, 0, 0}, {"G mon_w", G_mon_w, 2, 1, 1, 0}, {"G mon_h", G_mon_h, 2, 1, 1, 0}, {"G bar_w", G_bar_w, 2, 0, 0, 0},
    {"K mon_w", K_mon_w, 3, 1, 1, 0}, {"K mon_h", K_mon_h, 3, 1, 1, 0}, {"K bar_w", K_bar_w, 3, 0, 0, 0},
    {"K bar_h", K_bar_h, 3, 0, 0, 0}, {"K bar_h30", K_bar_h30, 3, 0, 0, 1},
    {"to mon_w", to_mon_w, 4, 0, 1, 0}, {"from mon_w", from_mon_w, 5, 0, 1, 0}, {"to mon_h", to_mon_h, 4, 0, 1, 0}, {"from mon_h", from_mon_h, 5, 0, 1, 0},
};
#define NMK (int)(sizeof MK / sizeof MK[0])
static int mk_valid(int k, u32 n) { return (!MK[k].oddonly || (n & 1)) && (!MK[k].lim30 || n <= (1u << 30)); }

/* ---------- correctness ---------- */
#define MCH (1u << 16)
static u32 *QA, *QB, *QC, *QD, *QAm, *QBm, *QCm, *QDm, *QR, *QI;
static long mcases[NMK], mfails[NMK];
static size_t qfill; static int qkinds;           /* bit mask of the kinds checked by the current pass */
static u32 mtab[1024]; static u32 mtab_n;
static inline u32 MT(u32 x, u32 n) { return n == mtab_n ? mtab[x] : mont_of(x, n); }
static u32 *WR, *WI;
static void mcheck(size_t n0, const mod_t *M) {
    size_t Np = (n0 + 3) / 4 * 4; u32 n = M->n;
    if (n < 1024 && mtab_n != n && (n & 1)) { for (u32 x = 0; x < n; x++) mtab[x] = mont_of(x, n); mtab_n = n; }
    for (size_t j = n0; j < Np; j++) QA[j] = QB[j] = QC[j] = QD[j] = 0;
    if (n & 1) for (size_t j = 0; j < Np; j++) { QAm[j] = MT(QA[j], n); QBm[j] = MT(QB[j], n); QCm[j] = MT(QC[j], n); QDm[j] = MT(QD[j], n); }
    for (int kind = 0; kind < 6; kind++) {
        if (!(qkinds >> kind & 1)) continue;
        int any = 0; for (int k = 0; k < NMK; k++) any |= MK[k].kind == kind && mk_valid(k, n);
        if (!any) continue;
        for (size_t j = 0; j < n0; j++) {            /* expected, standard form */
            u64 A = QA[j], B = QB[j], C = QC[j], D = QD[j]; u32 wr, wi = 0;
            if (kind == 1) C = M->k;
            if (kind == 3) { C = M->kc; D = M->kd; }
            if (kind <= 1) wr = (u32)(A * C % n);
            else if (kind <= 3) { u64 t = A * C + (M->n2 - B * D); wr = (u32)(t % n); wi = (u32)((A * D + B * C) % n); }
            else if (kind == 4) wr = MT((u32)A, n);
            else wr = (u32)A;
            WR[j] = wr; WI[j] = wi;
        }
        for (int k = 0; k < NMK; k++) {
            if (MK[k].kind != kind || !mk_valid(k, n)) continue;
            int mt = MK[k].mont;
            const u32 *a = mt || kind == 5 ? QAm : QA, *b = mt ? QBm : QB, *c = mt ? QCm : QC, *d = mt ? QDm : QD;
            MK[k].f(a, b, c, d, QR, QI, Np, M);
            long bad = 0;
            for (size_t j = 0; j < n0; j++) {
                u32 wr = WR[j], wi = WI[j];
                if (mt) { wr = MT(wr, n); wi = MT(wi, n); }
                bad += QR[j] != wr || (kind >= 2 && kind <= 3 && QI[j] != wi);
            }
            mcases[k] += n0; mfails[k] += bad;
            if (bad && mfails[k] == bad) printf("FAIL %s n = %u: %ld of %zu\n", MK[k].name, n, bad, n0);
        }
    }
}
static void mpush(u32 A, u32 B, u32 C, u32 D, const mod_t *M) { QA[qfill] = A; QB[qfill] = B; QC[qfill] = C; QD[qfill] = D; if (++qfill == MCH) { mcheck(qfill, M); qfill = 0; } }
static void mflush(const mod_t *M) { if (qfill) mcheck(qfill, M); qfill = 0; }
static u32 mpick(u32 n) {                         /* 1/4 edge residues, 1/4 near the top, else uniform */
    u64 r = next();
    if ((r & 3) == 0) { u32 e[6] = {0, 1, 2, n - 1, n - 2, n / 2}; return e[(r >> 2) % 6] % n; }
    if ((r & 3) == 1) return n - 1 - (u32)((r >> 2) % (n < 1024 ? n : 1024));
    return (u32)((r >> 2) % n);
}
static void mreport(const char *title) {
    long f = 0; for (int k = 0; k < NMK; k++) f += mfails[k];
    printf("%s: failures %ld\n", title, f);
    for (int k = 0; k < NMK; k++) printf("  %-11s cases %11ld  failures %ld\n", MK[k].name, mcases[k], mfails[k]);
    memset(mcases, 0, sizeof mcases); memset(mfails, 0, sizeof mfails);
}
static void do_mod_check(void) {
    size_t sz = (MCH + 8) * 4;
    WR = malloc(sz); WI = malloc(sz); QA = malloc(sz); QB = malloc(sz); QC = malloc(sz); QD = malloc(sz); QAm = malloc(sz); QBm = malloc(sz); QCm = malloc(sz); QDm = malloc(sz); QR = malloc(sz); QI = malloc(sz);
    /* exhaustive: P (all a, c) and S (all a for every k), every n from 2 to 1023 (odd only for Montgomery);
     * G (all a, b, c, d) and K (all a, b for every (kc, kd)), every n from 2 to 63 */
    qkinds = 1 | 16 | 32;
    for (u32 n = 2; n <= 1023; n++) {
        mod_t M = mod_make(n);
        for (u32 x = 0; x < n; x++) for (u32 y = 0; y < n; y++) mpush(x, 0, y, 0, &M);
        mflush(&M);
    }
    qkinds = 2;
    for (u32 n = 2; n <= 1023; n++) {
        mod_t M = mod_make(n);
        for (u32 k = 0; k < n; k++) { mod_set_consts(&M, k, k, (k * 7 + 3) % n); for (u32 x = 0; x < n; x++) mpush(x, 0, 0, 0, &M); mflush(&M); }
    }
    for (u32 n = 2; n <= 63; n++) {
        mod_t M = mod_make(n); qkinds = 4;
        for (u32 A = 0; A < n; A++) for (u32 B = 0; B < n; B++) for (u32 C = 0; C < n; C++) for (u32 D = 0; D < n; D++) mpush(A, B, C, D, &M);
        mflush(&M);
        qkinds = 8;
        for (u32 kc = 0; kc < n; kc++) for (u32 kd = 0; kd < n; kd++) {
            mod_set_consts(&M, kc, kc, kd);
            for (u32 A = 0; A < n; A++) for (u32 B = 0; B < n; B++) mpush(A, B, 0, 0, &M);
            mflush(&M);
        }
    }
    mreport("exhaustive: P and S for every n in [2, 1023], G and K for every n in [2, 63] (S and K over every shared constant; conversions on every residue of the P pass)");
    /* random at large n; constants change every chunk */
    const u32 ns[] = {2147483647u, 2147483629u, 2147483645u, 2147483646u, 2147483648u - 2, 1431655767u, 1073741827u, 1073741825u,
                      1073741824u, 1073741823u, 1073741789u, 805306457u, 536870913u, 111546435u, 1000003u, 65537u, 65536u};
    qkinds = 63;
    for (unsigned t = 0; t < sizeof ns / sizeof *ns; t++) {
        u32 n = ns[t]; mod_t M = mod_make(n);
        for (int ch = 0; ch < 64; ch++) {
            mod_set_consts(&M, mpick(n), mpick(n), mpick(n));
            for (u32 j = 0; j < MCH; j++) mpush(mpick(n), mpick(n), mpick(n), mpick(n), &M);
            mflush(&M);
        }
    }
    mreport("random: 17 n from 65,536 to 2^31 - 1 (odd and even, prime and composite, both sides of 2^30), 4,194,304 cases per kernel per n");
}

/* ---------- timing ---------- */
static void do_mod_time(void) {
    const u32 ns[] = {2147483647u, 2147483645u, 1073741823u, 1000003u};
    size_t sizes[2] = {1024, 1u << 22}, inner[2] = {2048, 1};
    u32 *buf[10], *p[10];
    for (int j = 0; j < 10; j++) { buf[j] = aligned_alloc(4096, ((1u << 22) + 4096) * 4); p[j] = buf[j] + 144 * j; }
    printf("ns per element (a Gaussian element writes two residues); median [q1, q3] over %d reps; '-' not valid for this n\n", REPS);
    for (int s = 0; s < 2; s++) {
        size_t N = sizes[s];
        printf("\n%s, N = %zu\n%-11s", s ? "memory" : "L1", N, "kernel"); for (int t = 0; t < 4; t++) printf("  %26u", ns[t]); printf("\n");
        static double res[NMK][4][3];
        for (int t = 0; t < 4; t++) {
            u32 n = ns[t]; mod_t M = mod_make(n); mod_set_consts(&M, (u32)(next() % n), (u32)(next() % n), (u32)(next() % n));
            for (size_t j = 0; j < N; j++) for (int q = 0; q < 4; q++) { p[q][j] = (u32)(next() % n); p[q + 4][j] = mont_of(p[q][j], n); }
            static double tt[NMK][REPS];
            int vk[NMK], nv = 0; for (int k = 0; k < NMK; k++) if (mk_valid(k, n)) vk[nv++] = k;
#define RUNK(k) do { int mt_ = MK[k].mont || MK[k].kind == 5; MK[k].f(p[mt_ ? 4 : 0], p[mt_ ? 5 : 1], p[mt_ ? 6 : 2], p[mt_ ? 7 : 3], p[8], p[9], N, &M); } while (0)
            for (int j = 0; j < nv; j++) RUNK(vk[j]);
            for (int r = 0; r < REPS; r++)
                for (int j = 0; j < nv; j++) {
                    int k = vk[(j + r) % nv];
                    RUNK(k);
                    u64 t0 = now_ns();
                    for (size_t it = 0; it < inner[s]; it++) RUNK(k);
                    tt[k][r] = (double)(now_ns() - t0) / (double)(N * inner[s]);
                }
            for (int k = 0; k < NMK; k++) {
                if (!mk_valid(k, n)) { res[k][t][0] = -1; continue; }
                qsort(tt[k], REPS, sizeof(double), cmpd);
                res[k][t][0] = tt[k][REPS / 2]; res[k][t][1] = tt[k][REPS / 4]; res[k][t][2] = tt[k][3 * REPS / 4];
            }
        }
        for (int k = 0; k < NMK; k++) {
            printf("%-11s", MK[k].name);
            for (int t = 0; t < 4; t++) { char bb[64];
                if (res[k][t][0] < 0) snprintf(bb, sizeof bb, "-"); else snprintf(bb, sizeof bb, "%.3f [%.3f, %.3f]", res[k][t][0], res[k][t][1], res[k][t][2]);
                printf("  %26s", bb); }
            printf("\n");
        }
        fflush(stdout);
    }
}
static void mod_count(void) {
#ifdef COUNT
    enum { N = 1024 }; static u32 b[10][N]; u32 n = 1073741823u; mod_t M = mod_make(n); mod_set_consts(&M, 5, 7, 11);
    for (int j = 0; j < N; j++) for (int q = 0; q < 4; q++) { b[q][j] = (u32)(next() % n); b[q + 4][j] = mont_of(b[q][j], n); }
    printf("mod n kernels: NEON instructions per element (arithmetic / loads+stores)\n");
    for (int k = 0; k < NMK; k++) { if (!strncmp(MK[k].name + 2, "div", 3)) continue; nops = nmem = 0;
        int mt = MK[k].mont || MK[k].kind == 5; MK[k].f(b[mt ? 4 : 0], b[mt ? 5 : 1], b[mt ? 6 : 2], b[mt ? 7 : 3], b[8], b[9], N, &M);
        printf("  %-11s %6.3f / %5.3f\n", MK[k].name, (double)nops / N, (double)nmem / N); }
#endif
}

/* ======================= M4: the (1+i)-ary binary gcd in NEON-shaped lanes ======================= */
/* Study 26/27's algorithm (study 24's step) on arrays of independent pairs, int32 halves, full range; gcd
 * normalised to re > 0, im >= 0; pairs with a -2^31 half are set aside (guard) and recomputed exactly once per
 * array (fixup); bit 0 of the return value: some gcd does not fit (2^31). 4 pairs per 128-bit register, R
 * registers stepped together (R = 1, 2, 3, 4, 6, 8).
 * NEON shape: every select is vbsl with a full-lane mask (vtst for "bit set"), conditional negation is
 * bsl(m, -x, x) in place of psignd, shifts right by a lane count are vshl by the negated count.
 * Trailing zeros (the step shifts hx, hy right by k = ctz(hx OR hy)):
 *   clz   k = 31 - clz(x), x = o AND -o (neg, and, clz, sub: 4 ops; NEON's count -k = clz - 31 is the same sub). No branch.
 *   nbrd  study 27: if every lane has k < 4 (one any-lane test), k = tbl(T4, x); otherwise dbh (16-bit de Bruijn).
 *   nbrc  the same branch with clz as the rare path. */
#define GARGS const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
              i32 *restrict gr, i32 *restrict gi, size_t N
#define PASSG ar, ai, br, bi, gr, gi, N
#define HMAX 2147483647
#define IMIN INT32_MIN

/* ---------- exact scalar path for pairs with a -2^31 half (study 26) ---------- */
static inline int ctz64(u64 v) { return v ? __builtin_ctzll(v) : 64; }
static inline void normalise(i64 *px, i64 *py) {
    i64 x = *px, y = *py;
    if (y < 0 || (y == 0 && x < 0)) { x = -x; y = -y; }
    if (x <= 0 && y > 0) { i64 t = x; x = y; y = -t; }
    *px = x; *py = y;
}
static void pi64(i64 ax, i64 ay, i64 bx, i64 by, i64 *g1, i64 *g2) {
#define STRIP(x, y) { int e_ = ctz64((u64)((x) | (y))) & 63; x >>= e_; y >>= e_;                 \
    if ((x) & (y) & 1) { i64 px_ = x >> 1, py_ = y >> 1; x = px_ + py_ + 1; y = py_ - px_; } }
#define PRIM(x, y) { if (!((x) & 1)) { i64 t_ = x; x = y; y = -t_; } if (((x) - (y)) & 2) { x = -x; y = -y; } }
#define SZ(x, y) ((u64)((x) < 0 ? -(x) : (x)) + (u64)((y) < 0 ? -(y) : (y)))
    int za = !(ax | ay), zb = !(bx | by);
    int ea = ctz64((u64)(ax | ay)), eb = ctz64((u64)(bx | by));
    int va = za ? 1000 : 2 * ea + (int)((ax >> ea) & (ay >> ea) & 1);
    int vb = zb ? 1000 : 2 * eb + (int)((bx >> eb) & (by >> eb) & 1);
    int k = va < vb ? va : vb;
    STRIP(ax, ay) PRIM(ax, ay) STRIP(bx, by) PRIM(bx, by)
    if (za) { ax = bx; ay = by; }
    if (zb) { bx = ax; by = ay; }
    u64 sb = SZ(bx, by);
    if (SZ(ax, ay) < sb) { i64 t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; sb = SZ(bx, by); }
    while (ax != bx || ay != by) {
        i64 hx = (ax >> 1) - (bx >> 1), hy = (ay >> 1) - (by >> 1);
        STRIP(hx, hy) PRIM(hx, hy)
        u64 s = SZ(hx, hy);
        if (s >= sb) { ax = hx; ay = hy; } else { ax = bx; ay = by; bx = hx; by = hy; sb = s; }
    }
    i64 gx = k > 64 ? 0 : ax << (k >> 1), gy = k > 64 ? 0 : ay << (k >> 1);
    if (k & 1) { i64 t = gx - gy; gy = gx + gy; gx = t; }
    normalise(&gx, &gy); *g1 = gx; *g2 = gy;
}
NOINL u32 fixup(GARGS) {
    u32 ov = 0;
    for (size_t i = 0; i < N; i++) {
        if (ar[i] != IMIN && ai[i] != IMIN && br[i] != IMIN && bi[i] != IMIN) continue;
        i64 g1, g2; pi64(ar[i], ai[i], br[i], bi[i], &g1, &g2);
        ov |= g1 > HMAX;
        gr[i] = (i32)(u32)g1; gi[i] = (i32)(u32)g2;
    }
    return ov;
}

/* ---------- lane ctz forms ---------- */
#define ONE  vdupq_n_s32(1)
#define TBL16(...) _mm_setr_epi8(__VA_ARGS__)
AI V iso_(V o) { return vandq(o, vnegq_s32(o)); }
AI V ctz_clz(V o) { return vsubq_s32(vdupq_n_s32(31), vclzq_u32(iso_(o))); }
/* dbh (study 27): vmul.i16 by 0x09AF, shift each word right 12, one tbl; the word that held the bit is moved down
 * by s = 16 [k >= 16] (vshl by -s; -s comes from vtst and an and at the same cost as min and and) */
#define T16 TBL16(0, 1, 2, 5, 3, 9, 6, 11, 15, 4, 8, 10, 14, 7, 13, 12)
#define T4  TBL16(0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0)
AI V dbh_x(V x) {
    V s = vminq_u32(vandq(x, vdupq_n_s32(0xFFFF0000)), vdupq_n_s32(16));
    V t = vqtbl1q_u8(T16, vshrq_n_u16(vmulq_s16(x, vdupq_n_s32(0x09AF09AF)), 12));
    return vorrq(vshrq_u32_v(t, s), s);
}
AI V ctz_dbh(V o) { return dbh_x(iso_(o)); }
AI void sh_clz(V *hx, V *hy, V o) { V e = ctz_clz(o); *hx = vshrq_s32_v(*hx, e); *hy = vshrq_s32_v(*hy, e); }
AI void sh_nbrd(V *hx, V *hy, V o) {
    V x = iso_(o), e;
    if (__builtin_expect(!vany_and(x, vdupq_n_s32(0xFFFFFFF0)), 1)) e = vqtbl1q_u8(T4, x); else e = dbh_x(x);
    *hx = vshrq_s32_v(*hx, e); *hy = vshrq_s32_v(*hy, e);
}
AI void sh_nbrc(V *hx, V *hy, V o) {
    V x = iso_(o), e;
    if (__builtin_expect(!vany_and(x, vdupq_n_s32(0xFFFFFFF0)), 1)) e = vqtbl1q_u8(T4, x); else e = vsubq_s32(vdupq_n_s32(31), vclzq_u32(x));
    *hx = vshrq_s32_v(*hx, e); *hy = vshrq_s32_v(*hy, e);
}
#define ctz_nbrd ctz_dbh
#define ctz_nbrc ctz_clz

/* ---------- the step's pieces ---------- */
AI V sra1(V a) { return vshrq_n_s32(a, 1); }
AI void stripodd(V *px, V *py) {                  /* where both parts are odd: divide by 1+i */
    V x = *px, y = *py, m = vtstq_s32(vandq(x, y), ONE);
    V hx = sra1(x), hy = sra1(y);
    *px = vbslq(m, vsubq_s32(hx, vmvnq(hy)), x);
    *py = vbslq(m, vsubq_s32(hy, hx), y);
}
AI void primv(V *px, V *py) {                     /* make primary: x odd (rotate by i if not), then x - y = 1 mod 4 up to sign */
    V x = *px, y = *py, m = vtstq_s32(x, ONE);
    V nx = vbslq(m, x, y), ny = vbslq(m, y, vnegq_s32(x));
    V t = vtstq_s32(vsubq_s32(nx, ny), vdupq_n_s32(2));
    *px = vbslq(t, vnegq_s32(nx), nx); *py = vbslq(t, vnegq_s32(ny), ny);
}
AI V sizev(V x, V y) { return vaddq_s32(vabsq_s32(x), vabsq_s32(y)); }
AI void normstore(V gx, V gy, i32 *pr, i32 *pi) {
    const V zero = vzero();
    V nm = vorrq(vcltq_s32(gy, zero), vandq(vceqzq_s32(gy), vcltq_s32(gx, zero)));
    gx = vbslq(nm, vnegq_s32(gx), gx); gy = vbslq(nm, vnegq_s32(gy), gy);
    V rm = vandq(vcltq_s32(gx, ONE), vcgtq_s32(gy, zero));
    V rx = vbslq(rm, gy, gx); gy = vbslq(rm, vnegq_s32(gx), gy);
    vst1q(pr, rx); vst1q(pi, gy);
}
typedef struct { V ax, ay, bx, by, sb, k; } gst;
#define SETUP(G, OFF, CTZ) {                                                                             \
    V ax = vld1q(ar + i + (OFF)), ay = vld1q(ai + i + (OFF)), bx = vld1q(br + i + (OFF)), by = vld1q(bi + i + (OFF)); \
    { V mn = vdupq_n_s32(IMIN); V g = vorrq(vorrq(vceqq_s32(ax, mn), vceqq_s32(ay, mn)), vorrq(vceqq_s32(bx, mn), vceqq_s32(by, mn))); \
      ax = vbicq(ax, g); ay = vbicq(ay, g); bx = vbicq(bx, g); by = vbicq(by, g); sticky = vorrq(sticky, g); }   \
    V oa = vorrq(ax, ay), ob = vorrq(bx, by), za = vceqzq_s32(oa), zb = vceqzq_s32(ob);                  \
    V ea = CTZ(oa), eb = CTZ(ob);                                                                        \
    V va = vaddq_s32(vaddq_s32(ea, ea), vandq(vandq(vshrq_s32_v(ax, ea), vshrq_s32_v(ay, ea)), ONE));    \
    V vb = vaddq_s32(vaddq_s32(eb, eb), vandq(vandq(vshrq_s32_v(bx, eb), vshrq_s32_v(by, eb)), ONE));    \
    (G).k = vminq_s32(vbslq(za, vdupq_n_s32(1000), va), vbslq(zb, vdupq_n_s32(1000), vb));             \
    ax = vshrq_s32_v(ax, ea); ay = vshrq_s32_v(ay, ea); stripodd(&ax, &ay); primv(&ax, &ay);             \
    bx = vshrq_s32_v(bx, eb); by = vshrq_s32_v(by, eb); stripodd(&bx, &by); primv(&bx, &by);             \
    { V tx = ax, ty = ay; ax = vbslq(za, bx, ax); ay = vbslq(za, by, ay); bx = vbslq(zb, tx, bx); by = vbslq(zb, ty, by); } \
    V sa = sizev(ax, ay), sb = sizev(bx, by);                                                            \
    { V mn = vminq_u32(sa, sb), ge = vceqq_s32(mn, sb);                                                  \
      (G).ax = vbslq(ge, ax, bx); (G).ay = vbslq(ge, ay, by); (G).bx = vbslq(ge, bx, ax); (G).by = vbslq(ge, by, ay); (G).sb = mn; } }
#define STEP(G, SH) {                                                                                    \
    V hx = vsubq_s32(sra1((G).ax), sra1((G).bx)), hy = vsubq_s32(sra1((G).ay), sra1((G).by));          \
    V o = vorrq(hx, hy), z = vceqzq_s32(o);                                                              \
    SH(&hx, &hy, o); stripodd(&hx, &hy); primv(&hx, &hy);                                                \
    hx = vbslq(z, (G).bx, hx); hy = vbslq(z, (G).by, hy);                                                \
    V s = sizev(hx, hy), mn = vminq_u32(s, (G).sb), ge = vceqq_s32(mn, (G).sb);                          \
    V nbx = vbslq(ge, (G).bx, hx), nby = vbslq(ge, (G).by, hy);                                          \
    (G).ax = vbslq(ge, hx, (G).bx); (G).ay = vbslq(ge, hy, (G).by); (G).bx = nbx; (G).by = nby; (G).sb = mn; }
#define FINISH(G, OFF) {                                                                                 \
    V s2 = vshrq_n_s32((G).k, 1);                                                                        \
    V gx = vshlq_s32((G).ax, s2), gy = vshlq_s32((G).ay, s2), od = vtstq_s32((G).k, ONE);               \
    V tx = vsubq_s32(gx, gy), ty = vaddq_s32(gx, gy);                                                    \
    normstore(vbslq(od, tx, gx), vbslq(od, ty, gy), gr + i + (OFF), gi + i + (OFF)); }
#define DIFF(G) vorrq(veorq((G).ax, (G).bx), veorq((G).ay, (G).by))
static long gsteps;     /* register-steps, counted in the COUNT build only */
#ifdef COUNT
#define CSTEP gsteps++
#else
#define CSTEP (void)0
#endif
/* R registers stepped together until all are done; the tail (fewer than 4R pairs) one register at a time */
#define PI_R(name, R, CTZ, SH)                                                                          \
NOINL u32 name(GARGS) {                                                                                  \
    V sticky = vzero(); size_t i = 0;                                                                    \
    for (; i + 4 * (R) <= N; i += 4 * (R)) {                                                             \
        gst g[R];                                                                                        \
        _Pragma("GCC unroll 8") for (int r = 0; r < (R); r++) SETUP(g[r], 4 * r, CTZ)                    \
        while (1) {                                                                                      \
            V d = DIFF(g[0]);                                                                            \
            _Pragma("GCC unroll 8") for (int r = 1; r < (R); r++) d = vorrq(d, DIFF(g[r]));             \
            if (!vany(d)) break;                                                                         \
            _Pragma("GCC unroll 8") for (int r = 0; r < (R); r++) { STEP(g[r], SH) CSTEP; }              \
        }                                                                                                \
        _Pragma("GCC unroll 8") for (int r = 0; r < (R); r++) FINISH(g[r], 4 * r)                        \
    }                                                                                                    \
    for (; i < N; i += 4) { gst g0; SETUP(g0, 0, CTZ) while (vany(DIFF(g0))) { STEP(g0, SH) CSTEP; } FINISH(g0, 0) } \
    return vany(sticky) ? fixup(PASSG) : 0;                                                              \
}
#define FORMS_R(X, f) X(f, 1) X(f, 2) X(f, 3) X(f, 4) X(f, 6) X(f, 8)
#define MKG(f, R) PI_R(pi##R##_##f, R, ctz_##f, sh_##f)
FORMS_R(MKG, clz) FORMS_R(MKG, nbrd) FORMS_R(MKG, nbrc)
typedef u32 (*gkf)(GARGS);
typedef struct { const char *name; gkf f; } gkern;
#define ENTG(f, R) {"pi" #R "_" #f, pi##R##_##f},
static gkern GK[] = { FORMS_R(ENTG, clz) FORMS_R(ENTG, nbrd) FORMS_R(ENTG, nbrc) };
#define NGK (int)(sizeof GK / sizeof GK[0])

/* ---------- reference (study 6's Euclid in __int128, normalised) ---------- */
static i128 rdiv128(i128 x, i128 n) { i128 q = x / n, r = x % n; if (r < 0) { q--; r += n; } return q + (2 * r >= n); }
static void ref_gcd(i64 a1, i64 a2, i64 b1, i64 b2, i64 *g1, i64 *g2) {
    i128 x1 = a1, x2 = a2, y1 = b1, y2 = b2;
    while (y1 || y2) {
        i128 n = y1 * y1 + y2 * y2, p = x1 * y1 + x2 * y2, q = x2 * y1 - x1 * y2;
        i128 q1 = rdiv128(p, n), q2 = rdiv128(q, n);
        i128 r1 = x1 - (q1 * y1 - q2 * y2), r2 = x2 - (q1 * y2 + q2 * y1);
        x1 = y1; x2 = y2; y1 = r1; y2 = r2;
    }
    i64 u = (i64)x1, v = (i64)x2; normalise(&u, &v); *g1 = u; *g2 = v;
}
static const i32 EVM[] = {IMIN, IMIN + 1, 0, 1, -1, 2, -2, 3, HMAX, HMAX - 1, 1 << 30, -(1 << 30), (1 << 30) + 1,
                          -(1 << 30) - 1, 1 << 16, -(1 << 16), 46341, -46341, IMIN + 2, -3};
#define NEVM (int)(sizeof EVM / sizeof EVM[0])
static void gen(int kind, i32 *a1, i32 *a2, i32 *b1, i32 *b2) {      /* study 26's five kinds */
    if (kind == 0) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(HMAX); *b2 = rr(HMAX); return; }
    if (kind == 1) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(1 << 12); *b2 = rr(1 << 12); return; }
    if (kind == 4) {
        i32 *p[4] = {a1, a2, b1, b2};
        for (int j = 0; j < 4; j++) { int s = (int)(next() % 32); *p[j] = (next() & 3) == 0 ? IMIN : (i32)((u32)rr(HMAX) << (s & 31)); if (next() & 1) *p[j] &= ~((1u << (next() % 31)) - 1); }
        return;
    }
    int e = kind == 2 ? 4 + (int)(next() % 12) : 15;
    i64 g1 = rr(1LL << e), g2 = rr(1LL << e), lim = HMAX / (2LL << e);
    i64 u1 = rr(lim), u2 = rr(lim), v1 = rr(lim), v2 = rr(lim);
    *a1 = (i32)(g1 * u1 - g2 * u2); *a2 = (i32)(g1 * u2 + g2 * u1);
    *b1 = (i32)(g1 * v1 - g2 * v2); *b2 = (i32)(g1 * v2 + g2 * v1);
}

/* ---------- correctness ---------- */
static long gfails, gnov, gnst;
#define GBM (1 << 16)
static i32 GA1[GBM + 64], GA2[GBM + 64], GB1[GBM + 64], GB2[GBM + 64], GO[2][GBM + 64];
static void gcheck(size_t n) {
    size_t Np = (n + 3) / 4 * 4;
    for (size_t j = n; j < Np; j++) GA1[j] = GA2[j] = GB1[j] = GB2[j] = 0;
    static i64 W1[GBM + 64], W2[GBM + 64]; int anyov = 0, anymin = 0;
    for (size_t j = 0; j < n; j++) {
        ref_gcd(GA1[j], GA2[j], GB1[j], GB2[j], W1 + j, W2 + j); anyov |= W1[j] > HMAX;
        anymin |= GA1[j] == IMIN || GA2[j] == IMIN || GB1[j] == IMIN || GB2[j] == IMIN;
    }
    gnov += anyov; gnst += anymin;
    for (int k = 0; k < NGK; k++) {
        u32 fl = GK[k].f(GA1, GA2, GB1, GB2, GO[0], GO[1], Np);
        for (size_t j = 0; j < n; j++)
            if (GO[0][j] != (i32)(u32)W1[j] || GO[1][j] != (i32)(u32)W2[j])
                if (gfails++ < 10) printf("FAIL %s (%d,%d) (%d,%d): got (%d,%d) want (%lld,%lld)\n", GK[k].name, GA1[j], GA2[j], GB1[j], GB2[j],
                                          GO[0][j], GO[1][j], (long long)W1[j], (long long)W2[j]);
        if (fl != (u32)anyov) if (gfails++ < 10) printf("FAIL %s flag %u want %d\n", GK[k].name, fl, anyov);
    }
}
/* the fitted shift of each form against o >> ctz(o) and -o >> ctz(o), every nonzero o; pass 2 forces lane 0 to
 * a single bit so that a register mixes small and large k (both branches of nbrd and nbrc) */
static void shift_check(void) {
    long bad[3] = {0};
    for (int pass = 0; pass < 2; pass++)
        for (u64 base = 1; base < (1ull << 32); base += 4) {
            u32 o4[4]; for (int j = 0; j < 4; j++) o4[j] = (u32)(base + j);
            if (pass) o4[0] = 1u << (base >> 2 & 3);
            V o = vld1q(o4);
            for (int f = 0; f < 3; f++) {
                V hx = o, hy = vnegq_s32(o);
                if (f == 0) sh_clz(&hx, &hy, o); else if (f == 1) sh_nbrd(&hx, &hy, o); else sh_nbrc(&hx, &hy, o);
                i32 rx[4], ry[4]; vst1q(rx, hx); vst1q(ry, hy);
                for (int j = 0; j < 4; j++) { u32 v = o4[j]; if (!v) continue; int k = __builtin_ctz(v);
                    bad[f] += rx[j] != ((i32)v >> k) || ry[j] != ((i32)(0u - v) >> k); }
            }
        }
    long cb[2] = {0};    /* the setup's ctz values */
    for (u64 base = 1; base < (1ull << 32); base += 4) {
        V o = _mm_add_epi32(_mm_set1_epi32((int)(u32)base), _mm_setr_epi32(0, 1, 2, 3));
        i32 r0[4], r1[4]; vst1q(r0, ctz_clz(o)); vst1q(r1, ctz_dbh(o));
        for (int j = 0; j < 4; j++) { u32 v = (u32)(base + j); if (!v) continue; int k = __builtin_ctz(v); cb[0] += r0[j] != k; cb[1] += r1[j] != k; }
    }
    printf("shift by ctz, every nonzero o (two passes): wrong clz %ld, nbrd %ld, nbrc %ld; ctz values: clz %ld, dbh %ld\n", bad[0], bad[1], bad[2], cb[0], cb[1]);
}
static void do_gcd_check(void) {
    shift_check();
    for (int K = 3; K <= 6; K++) {
        i64 lo = -(1 << (K - 1)), hi = (1 << (K - 1)) - 1; long cnt = 0; size_t n = 0;
        for (i64 p = lo; p <= hi; p++) for (i64 q = lo; q <= hi; q++) for (i64 r = lo; r <= hi; r++) for (i64 s = lo; s <= hi; s++) {
            GA1[n] = (i32)p; GA2[n] = (i32)q; GB1[n] = (i32)r; GB2[n] = (i32)s; n++; cnt++;
            if (n == GBM) { gcheck(n); n = 0; }
        }
        if (n) gcheck(n);
        printf("K = %d: exhaustive, %ld pairs; failures so far %ld\n", K, cnt, gfails);
    }
    long cnt = 0; gnov = gnst = 0;
    for (int p = 0; p < NEVM; p++) for (int q = 0; q < NEVM; q++) for (int r = 0; r < NEVM; r++) for (int s = 0; s < NEVM; s++) {
        GA1[0] = EVM[p]; GA2[0] = EVM[q]; GB1[0] = EVM[r]; GB2[0] = EVM[s]; gcheck(1); cnt++;
    }
    printf("edges incl. -2^31: %ld pairs each alone; with a -2^31 half %ld, overflowing gcd %ld; failures so far %ld\n", cnt, gnst, gnov, gfails);
    gnov = gnst = 0;
    for (int it = 0; it < 200000; it++) { gen(4, GA1, GA2, GB1, GB2); gcheck(1); }
    printf("random -2^31 pairs alone: 200,000; with -2^31 %ld, overflowing %ld; failures so far %ld\n", gnst, gnov, gfails);
    long tot = 0;
    for (int it = 0; it < 40; it++) {
        for (int j = 0; j < GBM; j++) gen(j % 5 == 4 ? ((next() & 63) ? 0 : 4) : j % 5, GA1 + j, GA2 + j, GB1 + j, GB2 + j);
        gcheck(GBM - (it & 7)); tot += GBM - (it & 7);        /* lengths not a multiple of 4R: the tail loop runs */
    }
    printf("random: %ld pairs (five kinds)\nTOTAL failures %ld\n", tot, gfails);
}

/* ---------- timing ---------- */
static void do_gcd_time(void) {
    enum { N = 1056 };              /* a multiple of 4R for every R tried; 25 KB in all, in L1 */
    static i32 buf[6][N + 256];
    i32 *p[6]; for (int j = 0; j < 6; j++) p[j] = buf[j] + 16 * j;
    static const int kinds[2] = {0, 3};
    static const char *kn[2] = {"random", "large common factor 2^15"};
    printf("ns per gcd, N = %d pairs; median [q1, q3] over %d reps; kernels interleaved, order rotated\n", N, REPS);
    for (int c = 0; c < 2; c++) {
        for (int j = 0; j < N; j++) gen(kinds[c], p[0] + j, p[1] + j, p[2] + j, p[3] + j);
        static double t[NGK][REPS];
        for (int k = 0; k < NGK; k++) GK[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NGK; j++) {
                int k = (j + r) % NGK;
                GK[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                u64 t0 = now_ns();
                for (int it = 0; it < 32; it++) GK[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                t[k][r] = (double)(now_ns() - t0) / (32.0 * N);
            }
        printf("\n%s\n", kn[c]);
        for (int k = 0; k < NGK; k++) { qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-9s %7.2f [%7.2f, %7.2f]\n", GK[k].name, t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
        fflush(stdout);
    }
}
static void gcd_count(void) {
#ifdef COUNT
    enum { N = 1056 }; static i32 b[6][N];
    for (int c = 0; c < 2; c++) {
        for (int j = 0; j < N; j++) gen(c ? 3 : 0, b[0] + j, b[1] + j, b[2] + j, b[3] + j);
        printf("gcd (%s): NEON instructions per gcd (all / loads+stores), register-steps per register of 4\n", c ? "large common factor" : "random");
        for (int k = 0; k < NGK; k++) { nops = nmem = 0; gsteps = 0; GK[k].f(b[0], b[1], b[2], b[3], b[4], b[5], N);
            printf("  %-9s %7.1f / %4.2f   steps %.2f\n", GK[k].name, (double)nops / N, (double)nmem / N, (double)gsteps * 4 / N); }
    }
#endif
}

/* ======================= main ======================= */
static void do_count(void);
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "arith-check";
    if (!strcmp(mode, "arith-check")) do_arith_check();
    else if (!strcmp(mode, "arith-time")) do_arith_time(argc > 2 ? (size_t)atol(argv[2]) : (1u << 24));
    else if (!strcmp(mode, "mod-check")) do_mod_check();
    else if (!strcmp(mode, "mod-time")) do_mod_time();
    else if (!strcmp(mode, "gcd-check")) do_gcd_check();
    else if (!strcmp(mode, "gcd-time")) do_gcd_time();
    else if (!strcmp(mode, "count")) do_count();
    return 0;
}

/* ======================= NEON instruction counts (build with -DCOUNT) ======================= */
static void do_count(void) {
#ifdef COUNT
    enum { N = 1024 }; static i32 b[6][N];
    for (int j = 0; j < N; j++) for (int t = 0; t < 4; t++) b[t][j] = (i32)rr(1 << 14);
    printf("NEON instructions per element (arithmetic / loads+stores), from the macro counters\n");
    for (int k = 0; k < NAK; k++) { nops = nmem = 0; AK[k].f(b[0], b[1], b[2], b[3], b[4], b[5], N);
        printf("  %-9s %6.3f / %5.3f\n", AK[k].name, (double)nops / N, (double)nmem / N); }
    mod_count(); gcd_count();
#else
    printf("build with -DCOUNT\n");
#endif
}
