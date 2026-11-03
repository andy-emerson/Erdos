/* Study 29: the NEON layer shared by neon29div.c, neon29mul.c and neon29shr.c (included, not compiled alone).
 *
 * Study 28's layer (neon28.c), unchanged, plus the operations studies 23 and 25 need in NEON shape:
 * 64-bit abs, the unsigned 64-bit compare (cmhs), shift right and accumulate (usra), widening subtract (usubw),
 * shift-left-long by 32 (shll), shift right narrow of a pair (shrn, shrn2), the 64-bit shift by a signed lane count
 * (ushl/sshl, left or right by the sign), and widening/narrowing between a packed 32-bit register and a pair of
 * 64-bit registers.
 *
 * Lane order. NEON's smull/umull take lanes 0, 1 and smull2/umull2 lanes 2, 3; uzp1/uzp2 of the two products give
 * the 32-bit halves back in lane order. The x86 stand-in takes even lanes (0, 2) for the "low" product and odd
 * lanes (1, 3) for the "high" one, and its uzp stand-in re-interleaves even and odd. Every macro below keeps that
 * one convention (a packed 32-bit register's even dwords go with the "lo" 64-bit register of a pair, odd dwords
 * with "hi"), so kernels written with these macros are correct on both, and count what NEON would issue.
 *
 * Rules as study 28: 128-bit registers, no mask registers, no 64-bit lane multiply, no three-input logic, no float.
 * scan_kernels() disassembles this binary and checks every function whose name starts with "k_". vpternlogd $0xff
 * (gcc's all-ones constant for xmm16..31, NEON movi #-1) and $0x55 (NOT, NEON mvn) are allowed and counted.
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
typedef struct { V l, h; } P2;          /* a pair of 64-bit-lane registers: 4 values, "lo" and "hi" halves */
#define NOINL __attribute__((noinline))
#define AI static inline __attribute__((always_inline))

/* ======================= the NEON layer (study 28) ======================= */
#ifdef COUNT
static u64 nops, nmem, nmul;
static void cnt_(void) { nops++; }      /* function calls: sequenced, so nested macro arguments count correctly */
static void cntm_(void) { nmem++; }
static void cntx_(void) { nmul++; nops++; }
#define C_(e) (cnt_(), (e))
#define M_(e) (cntm_(), (e))
#define X_(e) (cntx_(), (e))     /* a multiply (counted in both) */
#else
#define C_(e) (e)
#define M_(e) (e)
#define X_(e) (e)
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
#define vnegq_s64(a)        C_(_mm_sub_epi64(_mm_setzero_si128(), a))
#define vabsq_s32(a)        C_(_mm_abs_epi32(a))
#define vabsq_s64(a)        C_(_mm_abs_epi64(a))                 /* abs, 64-bit lanes: vpabsq (AVX-512VL) */
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
/* lane-count shifts. NEON shifts right by a negative count; kernels produce the count NEON would use at the same
 * cost (portable-isa.md note 1), and the stand-in takes the x86 right shift by k >= 0. */
#define vshlq_s32(a, k)     C_(_mm_sllv_epi32(a, k))             /* left by k */
#define vshrq_s32_v(a, k)   C_(_mm_srav_epi32(a, k))             /* NEON: vshlq_s32(a, -k) */
#define vshrq_u32_v(a, k)   C_(_mm_srlv_epi32(a, k))             /* NEON: vshlq_u32(a, -k) */
#define vshlq_u64_v(a, k)   C_(_mm_sllv_epi64(a, k))             /* left by k (0..63) */
#define vshrq_u64_v(a, k)   C_(_mm_srlv_epi64(a, k))             /* NEON: vshlq_u64(a, -k) */
/* NEON ushl with a signed count in each lane: left where k >= 0, right by -k where k < 0, one instruction.
 * x86 has no signed-count shift: both shifts (out-of-range counts give 0) and an OR. */
#define vshlq_u64_sgn(a, k) C_(BAR_(_mm_or_si128(_mm_sllv_epi64(a, k), _mm_srlv_epi64(a, _mm_sub_epi64(_mm_setzero_si128(), k)))))
/* shift by a count held in a register, the same for every lane (NEON: ushl by a dup'd count) */
#define vshrq_u64_r(a, c)   C_(_mm_srl_epi64(a, c))
#define vshlq_u64_r(a, c)   C_(_mm_sll_epi64(a, c))
#define vshrq_s32_r(a, c)   C_(_mm_sra_epi32(a, c))
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
#define vcltzq_s32(a)       C_(_mm_srai_epi32(a, 31))
#define vceqq_s64(a, b)     C_(cmpeq64_(a, b))
#define vceqzq_s64(a)       C_(cmpeq64_(a, _mm_setzero_si128()))
#define vcgtq_s64(a, b)     C_(cmpgt64_(a, b))
#define vcltq_s64(a, b)     C_(cmpgt64_(b, a))
#define vcltzq_s64(a)       C_(_mm_srai_epi64(a, 63))
/* unsigned compares: one NEON op (cmhi, cmhs); x86 has none without mask registers: bias both, pcmpgt (and NOT) */
#define BIAS32_(a) BAR_(_mm_xor_si128(a, _mm_set1_epi32(INT32_MIN)))
#define BIAS64_(a) BAR_(_mm_xor_si128(a, _mm_set1_epi64x(INT64_MIN)))
#define vcgtq_u32(a, b)     C_(cmpgt32_(BIAS32_(a), BIAS32_(b)))
#define vcgtq_u64(a, b)     C_(cmpgt64_(BIAS64_(a), BIAS64_(b)))
#define vcgeq_u64(a, b)     C_(BAR_(_mm_xor_si128(cmpgt64_(BIAS64_(b), BIAS64_(a)), _mm_set1_epi32(-1))))   /* cmhs */
#define vtstq_s32(a, b)     C_(BAR_(_mm_xor_si128(cmpeq32_(BAR_(_mm_and_si128(a, b)), _mm_setzero_si128()), _mm_set1_epi32(-1))))
#define vminq_s32(a, b)     C_(_mm_min_epi32(a, b))
#define vmaxq_s32(a, b)     C_(_mm_max_epi32(a, b))
#define vminq_u32(a, b)     C_(_mm_min_epu32(a, b))
#define vmaxq_u32(a, b)     C_(_mm_max_epu32(a, b))
#define vbslq(m, a, b)      C_(bsl_(m, a, b))                    /* m ? a : b, full-lane masks only */
#define vclzq_u32(a)        C_(_mm_lzcnt_epi32(a))
#define vcntq_u8(a)         C_(_mm_popcnt_epi8(a))
#define vpaddlq_u8(a)       C_(_mm_add_epi16(BAR_(_mm_and_si128(a, _mm_set1_epi16(0xFF))), _mm_srli_epi16(a, 8)))
#define vpaddlq_u16(a)      C_(_mm_add_epi32(BAR_(_mm_and_si128(a, _mm_set1_epi32(0xFFFF))), _mm_srli_epi32(a, 16)))
#define vqtbl1q_u8(t, i)    C_(_mm_shuffle_epi8(t, i))
#define vmulq_s32(a, b)     X_(_mm_mullo_epi32(a, b))
#define vmlaq_s32(c, a, b)  X_(_mm_add_epi32(c, _mm_mullo_epi32(a, b)))
#define vmlsq_s32(c, a, b)  X_(_mm_sub_epi32(c, _mm_mullo_epi32(a, b)))
AI int vany_(V a) { return !_mm_testz_si128(a, a); }
AI int vany_and_(V a, V m) { return !_mm_testz_si128(a, m); }
#define vany(a)             C_(C_(0) + vany_(a))                 /* umaxv + fmov + compare: counted as 2 */
#define vany_and(a, m)      C_(C_(0) + vany_and_(a, m))
/* widening multiplies: "lo" = even lanes on x86 (NEON lanes 0, 1), "high" = odd lanes (NEON lanes 2, 3) */
#define ODD_(a)                 _mm_srli_epi64(a, 32)
#define vmull_lo_s32(a, b)      X_(_mm_mul_epi32(a, b))
#define vmull_high_s32(a, b)    X_(_mm_mul_epi32(ODD_(a), ODD_(b)))
#define vmull_lo_u32(a, b)      X_(_mm_mul_epu32(a, b))
#define vmull_high_u32(a, b)    X_(_mm_mul_epu32(ODD_(a), ODD_(b)))
#define vmlal_lo_s32(c, a, b)   X_(_mm_add_epi64(c, _mm_mul_epi32(a, b)))
#define vmlal_high_s32(c, a, b) X_(_mm_add_epi64(c, _mm_mul_epi32(ODD_(a), ODD_(b))))
#define vmlsl_lo_s32(c, a, b)   X_(_mm_sub_epi64(c, _mm_mul_epi32(a, b)))
#define vmlsl_high_s32(c, a, b) X_(_mm_sub_epi64(c, _mm_mul_epi32(ODD_(a), ODD_(b))))
#define vmlal_lo_u32(c, a, b)   X_(_mm_add_epi64(c, _mm_mul_epu32(a, b)))
#define vmlal_high_u32(c, a, b) X_(_mm_add_epi64(c, _mm_mul_epu32(ODD_(a), ODD_(b))))
#define vmlsl_lo_u32(c, a, b)   X_(_mm_sub_epi64(c, _mm_mul_epu32(a, b)))
#define vmlsl_high_u32(c, a, b) X_(_mm_sub_epi64(c, _mm_mul_epu32(ODD_(a), ODD_(b))))
/* packing a pair back to 32-bit lanes, and widening a packed register to a pair */
#define vuzp1q_w(lo, hi)        C_(_mm_blend_epi32(lo, _mm_slli_epi64(hi, 32), 0xA))   /* low halves of a pair */
#define vuzp2q_w(lo, hi)        C_(_mm_blend_epi32(_mm_srli_epi64(lo, 32), hi, 0xA))   /* high halves of a pair */
#define vshrn_pair(lo, hi, n)   ((void)C_(0), C_(_mm_blend_epi32(_mm_srli_epi64(lo, n), _mm_slli_epi64(hi, 32 - (n)), 0xA)))  /* shrn, shrn2 */
#define vmovl_lo_u32(p)         C_(BAR_(_mm_and_si128(p, _mm_set1_epi64x(0xFFFFFFFFll))))  /* uxtl */
#define vmovl_high_u32(p)       C_(_mm_srli_epi64(p, 32))                                  /* uxtl2 */
#define vmovl_lo_s32(p)         C_(_mm_srai_epi64(_mm_slli_epi64(p, 32), 32))              /* sxtl */
#define vmovl_high_s32(p)       C_(_mm_srai_epi64(p, 32))                                  /* sxtl2 */
#define vshll32_lo(p)           C_(_mm_slli_epi64(p, 32))                                  /* shll #32 */
#define vshll32_high(p)         C_(BAR_(_mm_and_si128(p, _mm_set1_epi64x((long long)0xFFFFFFFF00000000ull))))  /* shll2 #32 */
#define vsubw_lo_u32(c, p)      C_(_mm_sub_epi64(c, BAR_(_mm_and_si128(p, _mm_set1_epi64x(0xFFFFFFFFll)))))  /* usubw */
#define vsubw_high_u32(c, p)    C_(_mm_sub_epi64(c, _mm_srli_epi64(p, 32)))                /* usubw2 */
#define vaddw_lo_u32(c, p)      C_(_mm_add_epi64(c, BAR_(_mm_and_si128(p, _mm_set1_epi64x(0xFFFFFFFFll)))))  /* uaddw */
#define vaddw_high_u32(c, p)    C_(_mm_add_epi64(c, _mm_srli_epi64(p, 32)))                /* uaddw2 */
#define vsraq_n_u64(c, a, n)    C_(_mm_add_epi64(c, _mm_srli_epi64(a, n)))                 /* usra */

/* ======================= common: random numbers, clock, statistics ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static u64 now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31

/* ======================= the instruction scan =======================
 * Disassemble this binary; for every function whose name starts with "k_", flag any instruction NEON lacks or the
 * rules forbid, and report per kernel: instructions, stack references (spills), and the stand-in instructions that
 * stand for one NEON op (vpblendvb for bsl, vpblendd for uzp/shrn, vpternlogq $0x55 for mvn). */
static const char *FORBID[] = {"%k0", "%k1", "%k2", "%k3", "%k4", "%k5", "%k6", "%k7", "ymm", "zmm", "vpmullq",
    "vpminsq", "vpmaxsq", "vpminuq", "vpmaxuq", "vpopcntd", "vpopcntq", "vplzcntq", "vpconflict", "vpcompress",
    "vpexpand", "vprol", "vpror", "vpshld", "vpshrd", "vpmadd52", "vpmaddwd", "vpmaddubsw", "vpsadbw", "vpmulhw",
    "vpmulhuw", "vpmulhrsw", "psign", "blendvps", "blendvpd", "vperm", "movmsk", "cvt", "vaddp", "vsubp", "vmulp",
    "vdivp", "vaddsd", "vaddss", "vmulsd", "vmulss", "vfmadd", "vsqrt", "vminp", "vmaxp", "vcmpp", "vpcmpq", "vpcmpd",
    "vpcmpuq", "vpcmpud", "vpblendm", 0};
static void scan_kernels(const char *exe) {
    char cmd[512]; snprintf(cmd, sizeof cmd, "objdump -d --no-show-raw-insn %s", exe);
    FILE *p = popen(cmd, "r"); if (!p) { printf("scan: objdump failed\n"); return; }
    char line[512], fn[256] = ""; int in = 0; long tot = 0, bad = 0, nk = 0, ins = 0, stk = 0, blv = 0, bld = 0, tln = 0, ones = 0;
    printf("instruction scan of %s (functions k_*): instructions, stack refs, vpblendvb (bsl), vpblendd (uzp/shrn), vpternlogq $0x55 (mvn)\n", exe);
#define FLUSH_() if (in) { printf("  %-22s %5ld %4ld %4ld %4ld %4ld\n", fn, ins, stk, blv, bld, tln); }
    while (fgets(line, sizeof line, p)) {
        char *lt = strchr(line, '<');
        if (line[0] != ' ' && lt && strstr(line, ">:")) {
            FLUSH_()
            char *gt = strchr(lt, '>'); size_t L = (size_t)(gt - lt - 1); if (L > 250) L = 250;
            memcpy(fn, lt + 1, L); fn[L] = 0; in = !strncmp(fn, "k_", 2); if (in) nk++;
            ins = stk = blv = bld = tln = 0; continue;
        }
        if (!in) continue;
        char *tab = strchr(line, '\t'); if (!tab) continue;
        char *ins_ = tab + 1; while (*ins_ == ' ') ins_++;
        if (!*ins_ || *ins_ == '\n') continue;
        ins++; tot++;
        if (strstr(ins_, "(%rsp)") && strstr(ins_, "xmm")) stk++;
        if (!strncmp(ins_, "vpblendvb", 9)) blv++;
        if (!strncmp(ins_, "vpblendd", 8)) bld++;
        if (!strncmp(ins_, "vpternlog", 9)) { if (strstr(ins_, "$0x55")) tln++; else if (strstr(ins_, "$0xff")) ones++; else { bad++; printf("  FORBIDDEN in %s: %s", fn, ins_); } }
        for (int j = 0; FORBID[j]; j++) if (strstr(ins_, FORBID[j])) { bad++; printf("  FORBIDDEN in %s: %s", fn, ins_); break; }
    }
    FLUSH_()
    pclose(p);
    printf("scan: %ld kernels, %ld instructions, %ld forbidden; vpternlogd $0xff (all-ones constant, NEON movi) %ld\n", nk, tot, bad, ones);
}
