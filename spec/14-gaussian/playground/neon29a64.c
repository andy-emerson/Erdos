/* Study 29: the NEON layer on real NEON intrinsics (included by neon29isa.c when A64 is defined).
 *
 * Purpose: compile the study's kernels for AArch64 (clang-18 --target=aarch64-linux-gnu -ffreestanding -O2 -DA64 -S)
 * and count the instructions and spills of each kernel's loop as the target would run them, with NEON's 32 vector
 * registers. Not run (no AArch64 machine or emulator here); the harness is compiled out.
 *
 * Every macro has the meaning of its x86 stand-in in neon29isa.c, in NEON's natural lane order ("lo" = lanes 0, 1).
 * V is uint32x4_t; reinterprets are free. Immediate shifts are written as register shifts by a constant splat,
 * which LLVM folds back to the immediate forms (and add + shift to usra/ssra), because clang's arm_neon.h defines
 * the immediate intrinsics as macros under the same names this layer uses.
 */
#include <arm_neon.h>
#include <stdint.h>
#include <stddef.h>
typedef int32_t i32; typedef uint32_t u32; typedef int64_t i64; typedef uint64_t u64;
typedef __int128 i128; typedef unsigned __int128 u128;
typedef uint32x4_t V;
typedef struct { V l, h; } P2;
#define NOINL __attribute__((noinline))
#define AI static inline __attribute__((always_inline))
/* the real intrinsics under names this layer does not redefine (a macro body that names another macro of this
 * layer would get the wrapper, with the wrong types) */
AI int32x4_t real_dup32(int32_t x) { return vdupq_n_s32(x); }
AI int64x2_t real_dup64(int64_t x) { return vdupq_n_s64(x); }
AI int32x4_t real_sshl32(int32x4_t a, int32x4_t k) { return vshlq_s32(a, k); }
AI int32x4_t real_neg32(int32x4_t a) { return vnegq_s32(a); }
AI int64x2_t real_neg64(int64x2_t a) { return vnegq_s64(a); }
AI int64x2_t real_add64(int64x2_t a, int64x2_t b) { return vaddq_s64(a, b); }
#define C_(e) (e)
#define M_(e) (e)
#define X_(e) (e)
#define BAR_(e) (e)
#define S32_(x) vreinterpretq_s32_u32(x)
#define S64_(x) vreinterpretq_s64_u32(x)
#define U64_(x) vreinterpretq_u64_u32(x)
#define U8_(x)  vreinterpretq_u8_u32(x)
#define U16_(x) vreinterpretq_u16_u32(x)
#define FS32_(x) vreinterpretq_u32_s32(x)
#define FS64_(x) vreinterpretq_u32_s64(x)
#define FU64_(x) vreinterpretq_u32_u64(x)
#define FU8_(x)  vreinterpretq_u32_u8(x)
#define FU16_(x) vreinterpretq_u32_u16(x)
#define SHCNT(s) FS64_(vdupq_n_s64(s))
#define vgetq_u64_(v, j) vgetq_lane_u64(U64_(v), j)

#define vld1q(p)            vreinterpretq_u32_u8(vld1q_u8((const uint8_t *)(p)))
#define vst1q(p, v)         vst1q_u8((uint8_t *)(p), vreinterpretq_u8_u32(v))
#define vdupq_n_s32(x)      vreinterpretq_u32_s32(vdupq_n_s32((int32_t)(x)))
#define vdupq_n_s64(x)      vreinterpretq_u32_s64(vdupq_n_s64((int64_t)(x)))
#define vzero()             vdupq_n_u32(0)
#define vaddq_s32(a, b)     vaddq_u32(a, b)
#define vsubq_s32(a, b)     vsubq_u32(a, b)
#define vaddq_s64(a, b)     FU64_(vaddq_u64(U64_(a), U64_(b)))
#define vsubq_s64(a, b)     FU64_(vsubq_u64(U64_(a), U64_(b)))
#define vnegq_s32(a)        FS32_(vnegq_s32(S32_(a)))
#define vnegq_s64(a)        FS64_(vnegq_s64(S64_(a)))
#define vabsq_s32(a)        FS32_(vabsq_s32(S32_(a)))
#define vabsq_s64(a)        FS64_(vabsq_s64(S64_(a)))
#define vandq(a, b)         vandq_u32(a, b)
#define vorrq(a, b)         vorrq_u32(a, b)
#define veorq(a, b)         veorq_u32(a, b)
#define vbicq(a, b)         vbicq_u32(a, b)
#define vmvnq(a)            vmvnq_u32(a)
/* immediate shifts as shifts by a constant splat (folded to shl/sshr/ushr #n) */
#define vshlq_n_s32(a, n)   vshlq_u32(a, real_dup32(n))
#define vshrq_n_s32(a, n)   FS32_(real_sshl32(S32_(a), real_dup32(-(n))))
#define vshrq_n_u32(a, n)   vshlq_u32(a, real_dup32(-(n)))
#define vshlq_n_s64(a, n)   FU64_(vshlq_u64(U64_(a), real_dup64(n)))
#define vshrq_n_s64(a, n)   FS64_(vshlq_s64(S64_(a), real_dup64(-(n))))
#define vshrq_n_u64(a, n)   FU64_(vshlq_u64(U64_(a), real_dup64(-(n))))
/* lane-count shifts: the x86 layer takes k >= 0 for right shifts; NEON shifts by -k */
#define vshlq_s32(a, k)     vshlq_u32(a, S32_(k))
#define vshrq_s32_v(a, k)   FS32_(real_sshl32(S32_(a), real_neg32(S32_(k))))
#define vshrq_u32_v(a, k)   vshlq_u32(a, real_neg32(S32_(k)))
#define vshlq_u64_v(a, k)   FU64_(vshlq_u64(U64_(a), S64_(k)))
#define vshrq_u64_v(a, k)   FU64_(vshlq_u64(U64_(a), real_neg64(S64_(k))))
#define vshlq_u64_sgn(a, k) FU64_(vshlq_u64(U64_(a), S64_(k)))
#define vshrq_u64_r(a, c)   FU64_(vshlq_u64(U64_(a), real_neg64(S64_(c))))
#define vshlq_u64_r(a, c)   FU64_(vshlq_u64(U64_(a), S64_(c)))
#define vshrq_s32_r(a, c)   FS32_(real_sshl32(S32_(a), real_dup32(-(int32_t)vgetq_lane_s64(S64_(c), 0))))
/* compares, select */
#define vceqq_s32(a, b)     vceqq_u32(a, b)
#define vceqzq_s32(a)       vceqzq_u32(a)
#define vcgtq_s32(a, b)     vcgtq_s32(S32_(a), S32_(b))
#define vcltq_s32(a, b)     vcltq_s32(S32_(a), S32_(b))
#define vcltzq_s32(a)       vcltzq_s32(S32_(a))
#define vceqq_s64(a, b)     FU64_(vceqq_u64(U64_(a), U64_(b)))
#define vceqzq_s64(a)       FU64_(vceqzq_u64(U64_(a)))
#define vcgtq_s64(a, b)     FU64_(vcgtq_s64(S64_(a), S64_(b)))
#define vcltq_s64(a, b)     FU64_(vcltq_s64(S64_(a), S64_(b)))
#define vcltzq_s64(a)       FU64_(vcltzq_s64(S64_(a)))
#define vcgtq_u32(a, b)     vcgtq_u32(a, b)
#define vcgtq_u64(a, b)     FU64_(vcgtq_u64(U64_(a), U64_(b)))
#define vcgeq_u64(a, b)     FU64_(vcgeq_u64(U64_(a), U64_(b)))
#define vtstq_s32(a, b)     vtstq_u32(a, b)
#define vminq_s32(a, b)     FS32_(vminq_s32(S32_(a), S32_(b)))
#define vmaxq_s32(a, b)     FS32_(vmaxq_s32(S32_(a), S32_(b)))
#define vminq_u32(a, b)     vminq_u32(a, b)
#define vmaxq_u32(a, b)     vmaxq_u32(a, b)
#define vbslq(m, a, b)      vbslq_u32(m, a, b)
#define vclzq_u32(a)        vclzq_u32(a)
#define vcntq_u8(a)         FU8_(vcntq_u8(U8_(a)))
#define vpaddlq_u8(a)       FU16_(vpaddlq_u8(U8_(a)))
#define vpaddlq_u16(a)      vpaddlq_u16(U16_(a))
#define vmulq_s32(a, b)     vmulq_u32(a, b)
#define vmlaq_s32(c, a, b)  vmlaq_u32(c, a, b)
#define vmlsq_s32(c, a, b)  vmlsq_u32(c, a, b)
#define vany(a)             (vmaxvq_u32(a) != 0)
#define vany_and(a, m)      (vmaxvq_u32(vandq_u32(a, m)) != 0)
/* widening multiplies, natural order */
#define vmull_lo_s32(a, b)      FS64_(vmull_s32(vget_low_s32(S32_(a)), vget_low_s32(S32_(b))))
#define vmull_high_s32(a, b)    FS64_(vmull_high_s32(S32_(a), S32_(b)))
#define vmull_lo_u32(a, b)      FU64_(vmull_u32(vget_low_u32(a), vget_low_u32(b)))
#define vmull_high_u32(a, b)    FU64_(vmull_high_u32(a, b))
#define vmlal_lo_s32(c, a, b)   FS64_(vmlal_s32(S64_(c), vget_low_s32(S32_(a)), vget_low_s32(S32_(b))))
#define vmlal_high_s32(c, a, b) FS64_(vmlal_high_s32(S64_(c), S32_(a), S32_(b)))
#define vmlsl_lo_s32(c, a, b)   FS64_(vmlsl_s32(S64_(c), vget_low_s32(S32_(a)), vget_low_s32(S32_(b))))
#define vmlsl_high_s32(c, a, b) FS64_(vmlsl_high_s32(S64_(c), S32_(a), S32_(b)))
#define vmlal_lo_u32(c, a, b)   FU64_(vmlal_u32(U64_(c), vget_low_u32(a), vget_low_u32(b)))
#define vmlal_high_u32(c, a, b) FU64_(vmlal_high_u32(U64_(c), a, b))
#define vmlsl_lo_u32(c, a, b)   FU64_(vmlsl_u32(U64_(c), vget_low_u32(a), vget_low_u32(b)))
#define vmlsl_high_u32(c, a, b) FU64_(vmlsl_high_u32(U64_(c), a, b))
#define vuzp1q_w(lo, hi)        vuzp1q_u32(lo, hi)
#define vuzp2q_w(lo, hi)        vuzp2q_u32(lo, hi)
#define vshrn_pair(lo, hi, n)   vshrn_high_n_u64(vshrn_n_u64(U64_(lo), n), U64_(hi), n)
#define vmovl_lo_u32(p)         FU64_(vmovl_u32(vget_low_u32(p)))
#define vmovl_high_u32(p)       FU64_(vmovl_high_u32(p))
#define vmovl_lo_s32(p)         FS64_(vmovl_s32(vget_low_s32(S32_(p))))
#define vmovl_high_s32(p)       FS64_(vmovl_high_s32(S32_(p)))
#define vshll32_lo(p)           FU64_(vshll_n_u32(vget_low_u32(p), 32))
#define vshll32_high(p)         FU64_(vshll_high_n_u32(p, 32))
#define vsubw_lo_u32(c, p)      FU64_(vsubw_u32(U64_(c), vget_low_u32(p)))
#define vsubw_high_u32(c, p)    FU64_(vsubw_high_u32(U64_(c), p))
#define vaddw_lo_u32(c, p)      FU64_(vaddw_u32(U64_(c), vget_low_u32(p)))
#define vaddw_high_u32(c, p)    FU64_(vaddw_high_u32(U64_(c), p))
#define vsraq_n_u64(c, a, n)    FU64_(vaddq_u64(U64_(c), vshlq_u64(U64_(a), real_dup64(-(n)))))   /* folded to usra */
#define vsraq_n_s64(c, a, n)    FS64_(real_add64(S64_(c), vshlq_s64(S64_(a), real_dup64(-(n)))))   /* folded to ssra */
#define vaddhn_pair(lo, hi, c)  FU32_HN_(vaddhn_high_u64(vaddhn_u64(U64_(lo), U64_(c)), U64_(hi), U64_(c)))
#define FU32_HN_(x)             (x)
#define vmovl_nat_lo(p)         vmovl_lo_s32(p)
#define vmovl_nat_high(p)       vmovl_high_s32(p)
#define vuzp1_nat(a, b)         vuzp1q_u32(a, b)
