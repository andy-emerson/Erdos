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
#define vandq(a, b)         C_(_mm_and_si128(a, b))
#define vorrq(a, b)         C_(_mm_or_si128(a, b))
#define veorq(a, b)         C_(_mm_xor_si128(a, b))
#define vbicq(a, b)         C_(_mm_andnot_si128(b, a))           /* a AND NOT b */
#define vmvnq(a)            C_(_mm_xor_si128(a, _mm_set1_epi32(-1)))
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
#define vceqq_s32(a, b)     C_(_mm_cmpeq_epi32(a, b))
#define vceqzq_s32(a)       C_(_mm_cmpeq_epi32(a, _mm_setzero_si128()))
#define vcgtq_s32(a, b)     C_(_mm_cmpgt_epi32(a, b))
#define vcltq_s32(a, b)     C_(_mm_cmpgt_epi32(b, a))
#define vceqq_s64(a, b)     C_(_mm_cmpeq_epi64(a, b))
#define vcgtq_s64(a, b)     C_(_mm_cmpgt_epi64(a, b))
/* unsigned compare: one NEON op (cmhi); x86 has none without masks: xor the sign bit into both, pcmpgtd */
#define vcgtq_u32(a, b)     C_(_mm_cmpgt_epi32(_mm_xor_si128(a, _mm_set1_epi32(INT32_MIN)), _mm_xor_si128(b, _mm_set1_epi32(INT32_MIN))))
#define vtstq_s32(a, b)     C_(_mm_xor_si128(_mm_cmpeq_epi32(_mm_and_si128(a, b), _mm_setzero_si128()), _mm_set1_epi32(-1)))
#define vminq_s32(a, b)     C_(_mm_min_epi32(a, b))
#define vminq_u32(a, b)     C_(_mm_min_epu32(a, b))
#define vmaxq_u32(a, b)     C_(_mm_max_epu32(a, b))
#define vbslq(m, a, b)      C_(_mm_blendv_epi8(b, a, m))         /* m ? a : b, full-lane masks only */
#define vclzq_u32(a)        C_(_mm_lzcnt_epi32(a))
#define vqtbl1q_u8(t, i)    C_(_mm_shuffle_epi8(t, i))
#define vmulq_s32(a, b)     C_(_mm_mullo_epi32(a, b))
#define vmulq_s16(a, b)     C_(_mm_mullo_epi16(a, b))
#define vmlaq_s32(c, a, b)  C_(_mm_add_epi32(c, _mm_mullo_epi32(a, b)))
#define vmlsq_s32(c, a, b)  C_(_mm_sub_epi32(c, _mm_mullo_epi32(a, b)))
/* is any lane nonzero (for a branch): NEON umaxv + fmov + compare; x86 ptest. Counted as 2. */
#define vany(a)             C_(C_(0) + !_mm_testz_si128(a, a))
#define vany_and(a, m)      C_(C_(0) + !_mm_testz_si128(a, m))
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
#define QCSAT64_(t, qc)  ((qc) = _mm_or_si128(qc, _mm_add_epi64(t, _mm_set1_epi64x(1LL << 31))))
#define vqmovn_pair(lo, hi, qc) (C_(0), C_((QCSAT64_(lo, qc), QCSAT64_(hi, qc), _mm_blend_epi32(lo, _mm_slli_epi64(hi, 32), 0xA))))
#define QC_MASK64 _mm_set1_epi64x((long long)0xFFFFFFFF00000000ull)
/* vqaddq_s32 / vqsubq_s32: saturating; QC emulated as the OR of the overflow masks (sign bit) */
AI V qadd_(V a, V b, V *qc) { V r = _mm_add_epi32(a, b), m = _mm_srai_epi32(_mm_andnot_si128(_mm_xor_si128(a, b), _mm_xor_si128(a, r)), 31);
    *qc = _mm_or_si128(*qc, m); return _mm_blendv_epi8(r, _mm_xor_si128(_mm_srai_epi32(a, 31), _mm_set1_epi32(INT32_MAX)), m); }
AI V qsub_(V a, V b, V *qc) { V r = _mm_sub_epi32(a, b), m = _mm_srai_epi32(_mm_and_si128(_mm_xor_si128(a, b), _mm_xor_si128(a, r)), 31);
    *qc = _mm_or_si128(*qc, m); return _mm_blendv_epi8(r, _mm_xor_si128(_mm_srai_epi32(a, 31), _mm_set1_epi32(INT32_MAX)), m); }
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
#define vhsubq_s32(a, b)    C_(_mm_sub_epi32(_mm_sub_epi32(_mm_srai_epi32(a, 1), _mm_srai_epi32(b, 1)), \
                               _mm_and_si128(_mm_andnot_si128(a, b), _mm_set1_epi32(1))))

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

/* ======================= main ======================= */
static void do_count(void);
static void do_mod_check(void); static void do_mod_time(void);
static void do_gcd_check(void); static void do_gcd_time(void);
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
#else
    printf("build with -DCOUNT\n");
#endif
}
static void do_mod_check(void) {} static void do_mod_time(void) {}
static void do_gcd_check(void) {} static void do_gcd_time(void) {}
