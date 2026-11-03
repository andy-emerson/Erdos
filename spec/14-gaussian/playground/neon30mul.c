/* Study 30, part A: the grown-size (int64-half) Gaussian multiply with the exact overflow check, reshaped for
 * vector lanes (neon30mul.c). Includes neon29mul.c (study 29's kernels s_bare, s_chk, k_bare, k_sgn, k_bnd).
 *
 * The pieces layout. Each int64 half x is stored as two int32 arrays: hi = x >> 32 (signed), lo = x mod 2^32.
 * That is the int64 array split into its two words; no other change of meaning (the size stays int64).
 * A Gaussian array at the grown size is then 4 arrays (re hi, re lo, im hi, im lo).
 *
 * The product. Signed-low pieces: h' = hi + (lo >> 31, logical) (one usra), l = lo read as signed, so
 * x = h' 2^32 + l with h' in [-2^31, 2^31] (2^31 only at the edge x >= 2^63 - 2^31, where h' wraps to -2^31).
 *   re: HH = ah'ch' - bh'dh', M1 = ah'cl - bh'dl, M2 = al ch' - bl dh', LL = al cl - bl dl (smull/smlsl, 2 per pair)
 *       re = HH 2^64 + (M1 + M2) 2^32 + LL.
 *       Z0 = lo(M1) + lo(M2) + floor(LL / 2^32)          (uzp1 x2, uaddl, uaddl2, then ssra)
 *       out: lo = low32(LL) (uzp1), hi = low32(Z0) (uzp1)   - the standard pieces of re mod 2^64
 *       K  = HH + (M1 >> 32) + (M2 >> 32) + round(Z0 / 2^32)   (ssra, ssra, srsra) = floor((re + 2^63) / 2^64),
 *       computed mod 2^64; |K| <= 2^63 so the result fits int64 iff K = 0.
 *   im: the same with sums. With every h' != -2^31 (below), HH', M1', M2' stay inside int64; LL' = al dl + bl cl can
 *       reach 2^63 (all four lows -2^31), so floor(LL'/2^32) is taken from LL' + 2^63 - 2^32 read unsigned (usra):
 *       Zi = Zsum' + floor(LL'/2^32) + 2^31 (one add of 1), K' += Zi >> 32 (usra), out hi = low32(Zi) xor 2^31.
 *   edges: a running smin of the four h' registers. If it reaches -2^31 anywhere in the array (x within 2^31 of
 *       +-2^63), the array is redone by the exact scalar kernel (one branch per array, as the sticky flag's).
 *   flag: K values ORed into one register, tested once per array.
 *
 * Kernels:
 *   s_chk  (study 29, int64 layout) scalar mul + smulh; the control
 *   k_sgn  (study 29, int64 layout) the vector check being replaced
 *   k_v2   int64 layout, the new algorithm: pieces by uzp1/uzp2/usra on load, outputs rebuilt by sli
 *   k_pc   pieces layout, the new algorithm
 *   s_pc   pieces layout, scalar (to show what the layout costs scalar code)
 *   k_pcb  pieces layout, bare (wrapping, no check), for reference
 * Side costs (pieces against int64 layout): k_padd / k_add64 (checked add), k_ppro / k_pro64 (promotion from
 * int32 halves), k_pshr / k_shr64 (rule B shrink test of one array).
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -o neon30mul neon30mul.c
 *        (-DCOUNT counts; -DFAULT30=k faults)
 *        clang-18 --target=aarch64-linux-gnu -ffreestanding -O2 -DA64 -S -o neon30mul.s neon30mul.c
 * Run:   ./neon30mul check | quick | time | count | scan
 */
#define NEON29_NO_MAIN
#include "neon29mul.c"
#ifndef FAULT30
#define FAULT30 0
#endif
/* faults: 1 K without the srsra term; 2 im without the +1; 3 no edge fallback; 4 ssra in place of usra for LL';
 *         5 h' without the usra; 6 Zsum dropping lo(M2) */

/* ---- new layer operations (NEON intrinsic named; x86 stand-in) ---- */
#ifndef A64
#define vsraq_n_u32_31(h, l)   C_(_mm_add_epi32(h, _mm_srli_epi32(l, 31)))                   /* usra .4s #31 */
/* srsra .2d #32: acc + ((x + 2^31) >> 32), exact on NEON; the stand-in is exact when x + 2^31 does not overflow */
#define vrsraq_n_s64_32(c, x)  C_(_mm_add_epi64(c, _mm_srai_epi64(_mm_add_epi64(x, _mm_set1_epi64x(1ll << 31)), 32)))
/* sli .2d #32: low 32 bits of l kept, high 32 bits from the low 32 bits of h */
#define vsliq_n_u64_32(l, h)   C_(_mm_blend_epi32(l, _mm_slli_epi64(h, 32), 0xA))
/* lo(M1) + lo(M2) as a pair: NEON uzp1, uzp1, uaddl, uaddl2 (4 per two pairs); x86: and, and, add per register */
#define LOSUM_(m1, m2) BAR_(_mm_add_epi64(BAR_(_mm_and_si128(m1, _mm_set1_epi64x(0xFFFFFFFFll))), BAR_(_mm_and_si128(m2, _mm_set1_epi64x(0xFFFFFFFFll)))))
#define LOSUM2(M1, M2, Z) ((void)C_(0), (void)C_(0), (void)C_(0), (void)C_(0), (Z).l = LOSUM_((M1).l, (M2).l), (Z).h = LOSUM_((M1).h, (M2).h))
#else
#define vsraq_n_u32_31(h, l)   vsraq_n_u32(h, l, 31)
#define vrsraq_n_s64_32(c, x)  FS64_(vrsraq_n_s64(S64_(c), S64_(x), 32))
#define vsliq_n_u64_32(l, h)   FU64_(vsliq_n_u64(U64_(l), U64_(h), 32))
#define LOSUM2(M1, M2, Z) do { V u1_ = vuzp1q_u32((M1).l, (M1).h), u2_ = vuzp1q_u32((M2).l, (M2).h); \
    (Z).l = FU64_(vaddl_u32(vget_low_u32(u1_), vget_low_u32(u2_))); (Z).h = FU64_(vaddl_high_u32(u1_, u2_)); } while (0)
#endif

#define C32_(x) vdupq_n_s32(x)
#define PARGS const i32 *restrict ah, const i32 *restrict al, const i32 *restrict bh, const i32 *restrict bl, \
              const i32 *restrict ch, const i32 *restrict cl, const i32 *restrict dh, const i32 *restrict dl, \
              i32 *restrict rh, i32 *restrict rl, i32 *restrict ih, i32 *restrict il, size_t N
#define PPASS ah, al, bh, bl, ch, cl, dh, dl, rh, rl, ih, il, N
typedef int (*pf)(PARGS);

/* ======================= scalar, pieces layout ======================= */
#define J64_(h, l) ((i64)(((u64)(u32)(h) << 32) | (u32)(l)))
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_pc(PARGS) {
    u64 o = 0;
#ifdef A64
#pragma clang loop vectorize(disable) interleave(disable)
#endif
    for (size_t i = 0; i < N; i++) {
        i64 a = J64_(ah[i], al[i]), b = J64_(bh[i], bl[i]), c = J64_(ch[i], cl[i]), d = J64_(dh[i], dl[i]);
        i128 re = (i128)a * c - (i128)b * d;
        i128 im = (i128)((u128)((i128)a * d) + (u128)((i128)b * c));
        i64 rl_ = (i64)re, il_ = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl_ >> 63)) | ((u64)(im >> 64) ^ (u64)(il_ >> 63));
        rh[i] = (i32)(rl_ >> 32); rl[i] = (i32)rl_; ih[i] = (i32)(il_ >> 32); il[i] = (i32)il_;
    }
    return o != 0;
}

/* ======================= the reshaped product, one pass of 4 elements =======================
 * inputs: signed-low pieces (packed, natural order); outputs: the standard pieces of re and im (packed), and
 * returns the OR of the four K registers. */
AI V pc_core(V ahp, V al_, V bhp, V bl_, V chp, V cl_, V dhp, V dl_, V *orh, V *orl, V *oih, V *oil, P2 *Rout, P2 *Iout) {
    const V offL = C64_((long long)0x7FFFFFFF00000000ull), one64 = C64_(1), top = C32_(INT32_MIN);
    P2 HH, M1, M2, LL, Z, K, HI, N1, N2, LI, Zi, Ki;
    HH.l = vmlsl_lo_s32(vmull_lo_s32(ahp, chp), bhp, dhp); HH.h = vmlsl_high_s32(vmull_high_s32(ahp, chp), bhp, dhp);
    M1.l = vmlsl_lo_s32(vmull_lo_s32(ahp, cl_), bhp, dl_); M1.h = vmlsl_high_s32(vmull_high_s32(ahp, cl_), bhp, dl_);
    M2.l = vmlsl_lo_s32(vmull_lo_s32(al_, chp), bl_, dhp); M2.h = vmlsl_high_s32(vmull_high_s32(al_, chp), bl_, dhp);
    LL.l = vmlsl_lo_s32(vmull_lo_s32(al_, cl_), bl_, dl_); LL.h = vmlsl_high_s32(vmull_high_s32(al_, cl_), bl_, dl_);
#if FAULT30 == 6
    LOSUM2(M1, M1, Z);
#else
    LOSUM2(M1, M2, Z);
#endif
    Z.l = vsraq_n_s64(Z.l, LL.l, 32); Z.h = vsraq_n_s64(Z.h, LL.h, 32);
    K.l = vsraq_n_s64(vsraq_n_s64(HH.l, M1.l, 32), M2.l, 32); K.h = vsraq_n_s64(vsraq_n_s64(HH.h, M1.h, 32), M2.h, 32);
#if FAULT30 != 1
    K.l = vrsraq_n_s64_32(K.l, Z.l); K.h = vrsraq_n_s64_32(K.h, Z.h);
#endif
    HI.l = vmlal_lo_s32(vmull_lo_s32(ahp, dhp), bhp, chp); HI.h = vmlal_high_s32(vmull_high_s32(ahp, dhp), bhp, chp);
    N1.l = vmlal_lo_s32(vmull_lo_s32(ahp, dl_), bhp, cl_); N1.h = vmlal_high_s32(vmull_high_s32(ahp, dl_), bhp, cl_);
    N2.l = vmlal_lo_s32(vmull_lo_s32(al_, dhp), bl_, chp); N2.h = vmlal_high_s32(vmull_high_s32(al_, dhp), bl_, chp);
    LI.l = vmlal_lo_s32(vmull_lo_s32(al_, dl_), bl_, cl_); LI.h = vmlal_high_s32(vmull_high_s32(al_, dl_), bl_, cl_);
    LOSUM2(N1, N2, Zi);
#if FAULT30 == 4
    Zi.l = vsraq_n_s64(Zi.l, LI.l, 32); Zi.h = vsraq_n_s64(Zi.h, LI.h, 32);
    Zi.l = vaddq_s64(Zi.l, C64_(1ll << 31)); Zi.h = vaddq_s64(Zi.h, C64_(1ll << 31)); (void)offL; (void)one64;
#else
    Zi.l = vsraq_n_u64(Zi.l, vaddq_s64(LI.l, offL), 32); Zi.h = vsraq_n_u64(Zi.h, vaddq_s64(LI.h, offL), 32);
#if FAULT30 != 2
    Zi.l = vaddq_s64(Zi.l, one64); Zi.h = vaddq_s64(Zi.h, one64);
#endif
#endif
    Ki.l = vsraq_n_s64(vsraq_n_s64(HI.l, N1.l, 32), N2.l, 32); Ki.h = vsraq_n_s64(vsraq_n_s64(HI.h, N1.h, 32), N2.h, 32);
    Ki.l = vsraq_n_u64(Ki.l, Zi.l, 32); Ki.h = vsraq_n_u64(Ki.h, Zi.h, 32);
    if (orl) {                                  /* packed pieces out */
        *orl = vuzp1q_w(LL.l, LL.h); *orh = vuzp1q_w(Z.l, Z.h);
        *oil = vuzp1q_w(LI.l, LI.h); *oih = veorq(vuzp1q_w(Zi.l, Zi.h), top);
    } else {                                    /* int64 pairs out: sli */
        Rout->l = vsliq_n_u64_32(LL.l, Z.l); Rout->h = vsliq_n_u64_32(LL.h, Z.h);
        V xl = veorq(Zi.l, C64_(1ll << 31)), xh = veorq(Zi.h, C64_(1ll << 31));
        Iout->l = vsliq_n_u64_32(LI.l, xl); Iout->h = vsliq_n_u64_32(LI.h, xh);
    }
    return vorrq(vorrq(K.l, K.h), vorrq(Ki.l, Ki.h));
}
#if FAULT30 == 5
#define HP_(h, l) (h)
#else
#define HP_(h, l) vsraq_n_u32_31(h, l)
#endif
static long fallbacks;
NOINL int k_pc(PARGS) {
    V o = vzero(), mn = C32_(INT32_MAX);
    for (size_t i = 0; i < N; i += 4) {
        V a0 = vld1q(al + i), b0 = vld1q(bl + i), c0 = vld1q(cl + i), d0 = vld1q(dl + i);
        V a1 = HP_(vld1q(ah + i), a0), b1 = HP_(vld1q(bh + i), b0), c1 = HP_(vld1q(ch + i), c0), d1 = HP_(vld1q(dh + i), d0);
        mn = vminq_s32(vminq_s32(mn, a1), vminq_s32(b1, vminq_s32(c1, d1)));
        V xrh, xrl, xih, xil;
        o = vorrq(o, pc_core(a1, a0, b1, b0, c1, c0, d1, d0, &xrh, &xrl, &xih, &xil, 0, 0));
        vst1q(rh + i, xrh); vst1q(rl + i, xrl); vst1q(ih + i, xih); vst1q(il + i, xil);
    }
#if FAULT30 != 3
    if (vany(vceqq_s32(mn, C32_(INT32_MIN)))) { fallbacks++; return s_pc(PPASS); }
#endif
    return vany(o);
}
NOINL int k_v2(ARGS) {
    V o = vzero(), mn = C32_(INT32_MAX);
    for (size_t i = 0; i < N; i += 4) {
        P2 a = LD2(ar, i), b = LD2(ai, i), c = LD2(br, i), d = LD2(bi, i), re, im;
        V a0 = vuzp1q_w(a.l, a.h), b0 = vuzp1q_w(b.l, b.h), c0 = vuzp1q_w(c.l, c.h), d0 = vuzp1q_w(d.l, d.h);
        V a1 = HP_(vuzp2q_w(a.l, a.h), a0), b1 = HP_(vuzp2q_w(b.l, b.h), b0);
        V c1 = HP_(vuzp2q_w(c.l, c.h), c0), d1 = HP_(vuzp2q_w(d.l, d.h), d0);
        mn = vminq_s32(vminq_s32(mn, a1), vminq_s32(b1, vminq_s32(c1, d1)));
        o = vorrq(o, pc_core(a1, a0, b1, b0, c1, c0, d1, d0, 0, 0, 0, 0, &re, &im));
        ST2(cr, i, re); ST2(ci, i, im);
    }
#if FAULT30 != 3
    if (vany(vceqq_s32(mn, C32_(INT32_MIN)))) { fallbacks++; return s_chk(ar, ai, br, bi, cr, ci, N); }
#endif
    return vany(o);
}
/* bare product in the pieces layout: lo = low32(LL) by mul/mls, hi = low32(mid + hi32(LL)) */
NOINL int k_pcb(PARGS) {
    for (size_t i = 0; i < N; i += 4) {
        V a0 = vld1q(al + i), b0 = vld1q(bl + i), c0 = vld1q(cl + i), d0 = vld1q(dl + i);
        V a1 = vld1q(ah + i), b1 = vld1q(bh + i), c1 = vld1q(ch + i), d1 = vld1q(dh + i);
        V mr = vmlsq_s32(vmlsq_s32(vmlaq_s32(vmulq_s32(a1, c0), a0, c1), b1, d0), b0, d1);
        V mi = vmlaq_s32(vmlaq_s32(vmlaq_s32(vmulq_s32(a1, d0), a0, d1), b1, c0), b0, c1);
        P2 L, I;
        L.l = vmlsl_lo_u32(vmull_lo_u32(a0, c0), b0, d0); L.h = vmlsl_high_u32(vmull_high_u32(a0, c0), b0, d0);
        I.l = vmlal_lo_u32(vmull_lo_u32(a0, d0), b0, c0); I.h = vmlal_high_u32(vmull_high_u32(a0, d0), b0, c0);
        vst1q(rl + i, vuzp1q_w(L.l, L.h)); vst1q(rh + i, vaddq_s32(mr, vuzp2q_w(L.l, L.h)));
        vst1q(il + i, vuzp1q_w(I.l, I.h)); vst1q(ih + i, vaddq_s32(mi, vuzp2q_w(I.l, I.h)));
    }
    return 0;
}

/* ======================= side costs ======================= */
/* checked add, one Gaussian array pair: pieces (carry by cmhi, V rule on the high word) against int64 */
#ifndef A64
#define vcgtq_u32_(a, b) vcgtq_u32(a, b)
#else
#define vcgtq_u32_(a, b) vcgtq_u32(a, b)
#endif
NOINL int k_padd(PARGS) {
    V o = vzero();
    for (size_t i = 0; i < N; i += 4) {
        V xl = vld1q(al + i), yl = vld1q(cl + i), xh = vld1q(ah + i), yh = vld1q(ch + i);
        V sl = vaddq_s32(xl, yl), sh = vsubq_s32(vaddq_s32(xh, yh), vcgtq_u32_(xl, sl));
        o = vorrq(o, vbicq(veorq(xh, sh), veorq(xh, yh)));
        vst1q(rl + i, sl); vst1q(rh + i, sh);
        V ul = vld1q(bl + i), vl = vld1q(dl + i), uh = vld1q(bh + i), vh = vld1q(dh + i);
        V tl = vaddq_s32(ul, vl), th = vsubq_s32(vaddq_s32(uh, vh), vcgtq_u32_(ul, tl));
        o = vorrq(o, vbicq(veorq(uh, th), veorq(uh, vh)));
        vst1q(il + i, tl); vst1q(ih + i, th);
    }
    return vany(vcltzq_s32(o));
}
NOINL int k_add64(ARGS) {
    V o = vzero();
    for (size_t i = 0; i < N; i += 2) {
        V x = vld1q(ar + i), y = vld1q(br + i), s = vaddq_s64(x, y);
        o = vorrq(o, vbicq(veorq(x, s), veorq(x, y))); vst1q(cr + i, s);
        V u = vld1q(ai + i), v = vld1q(bi + i), t = vaddq_s64(u, v);
        o = vorrq(o, vbicq(veorq(u, t), veorq(u, v))); vst1q(ci + i, t);
    }
    return vany(vcltzq_s64(o));
}
/* promotion of one int32 array to the grown size: pieces (lo = x, hi = x >> 31) against int64 (sxtl, sxtl2) */
NOINL void k_ppro(const i32 *restrict x, i32 *restrict h, i32 *restrict l, size_t N) {
    for (size_t i = 0; i < N; i += 4) { V v = vld1q(x + i); vst1q(l + i, v); vst1q(h + i, vshrq_n_s32(v, 31)); }
}
#ifndef A64
#define SXTL_LO(v) C_(_mm_cvtepi32_epi64(v))
#define SXTL_HI(v) C_(_mm_cvtepi32_epi64(_mm_unpackhi_epi64(v, v)))
#else
#define SXTL_LO(v) FS64_(vmovl_s32(vget_low_s32(S32_(v))))
#define SXTL_HI(v) FS64_(vmovl_high_s32(S32_(v)))
#endif
NOINL void k_pro64(const i32 *restrict x, i64 *restrict y, size_t N) {
    for (size_t i = 0; i < N; i += 4) { V v = vld1q(x + i); vst1q(y + i, SXTL_LO(v)); vst1q(y + i + 2, SXTL_HI(v)); }
}
/* rule B shrink test of one array (does every value lie in [-2^30, 2^30)?):
 * pieces: hi + (lo >> 31) must be 0 (usra, orr) and lo + 2^30 must have bit 31 clear (add, orr);
 * int64: x + 2^30 must lie in [0, 2^31) (add, orr), as study 29's fused test */
NOINL int k_pshr(const i32 *restrict h, const i32 *restrict l, size_t N) {
    V o1 = vzero(), o2 = vzero(); const V b30 = C32_(1 << 30);
    for (size_t i = 0; i < N; i += 4) { V lo = vld1q(l + i); o1 = vorrq(o1, vsraq_n_u32_31(vld1q(h + i), lo)); o2 = vorrq(o2, vaddq_s32(lo, b30)); }
    return !vany(o1) && !vany(vcltzq_s32(o2));
}
NOINL int k_shr64(const i64 *restrict x, size_t N) {
    V o = vzero(); const V b30 = C64_(1ll << 30);
    for (size_t i = 0; i < N; i += 2) o = vorrq(o, vaddq_s64(vld1q(x + i), b30));
    return !vany(vandq(o, C64_(~0x7FFFFFFFll)));
}

#ifndef A64
/* ======================= harness ======================= */
/* study 23/29's edges and random values (neon29mul.c's harness, copied: it is compiled out with NEON29_NO_MAIN) */
static i64 E[160]; static int NE;
static void add_edge(i128 v) {
    if (v < -((i128)1 << 63) || v >= ((i128)1 << 63)) return;
    for (int j = 0; j < NE; j++) if (E[j] == (i64)v) return;
    E[NE++] = (i64)v;
}
static void make_edges(void) {
    add_edge(0); add_edge(1); add_edge(-1);
    const int bits[] = {31, 32, 62, 63};
    for (int k = 0; k < 4; k++) { i128 p = (i128)1 << bits[k]; for (int d = -2; d <= 1; d++) { add_edge(p + d); add_edge(-p + d); } }
    i128 L = ((i128)1 << 63) - ((i128)1 << 31);
    for (int d = -1; d <= 1; d++) { add_edge(L + d); add_edge(-L + d); }
}
static i64 rval(void) {
    u64 k = next() % 100;
    if (k < 15) return E[next() % NE];
    if (k < 25) return (i64)((u64)E[next() % NE] + (next() % 9) - 4);
    int b = (int)(next() % 65);
    i64 v = b ? (i64)(next() >> (64 - b)) : 0;
    if (b == 64) return v;
    if (next() & 1) v = -v - (i64)(next() & 1);
    return v;
}
static int fitsum(i128 x, i128 y) { i128 lo = -((i128)1 << 63), hi = ((i128)1 << 63) - 1; return y >= lo - x && y <= hi - x; }
static i64 sat(i128 v) { if (v >= ((i128)1 << 63)) return INT64_MAX; if (v < -((i128)1 << 63)) return INT64_MIN; return (i64)v; }
static i32 PH[4][256], PL[4][256], OH[2][256], OL[2][256];
static void split(const i64 *x, i32 *h, i32 *l, size_t n) { for (size_t j = 0; j < n; j++) { h[j] = (i32)(x[j] >> 32); l[j] = (i32)x[j]; } }
typedef struct { const char *name; int lay; void *f; } kern30;   /* lay 0: int64 (kf), 1: pieces (pf) */
static kern30 K30[] = { {"s_chk", 0, (void *)s_chk}, {"k_sgn", 0, (void *)k_sgn}, {"k_v2", 0, (void *)k_v2},
                        {"k_pc", 1, (void *)k_pc}, {"s_pc", 1, (void *)s_pc} };
#define NK30 (int)(sizeof K30 / sizeof K30[0])
static long f30, c30, kf30[8], fl30[8][2];
static int call30(int k, const i64 *A, const i64 *B, const i64 *Cc, const i64 *D, i64 *R, i64 *I, size_t n) {
    if (!K30[k].lay) return ((kf)K30[k].f)(A, B, Cc, D, R, I, n);
    split(A, PH[0], PL[0], n); split(B, PH[1], PL[1], n); split(Cc, PH[2], PL[2], n); split(D, PH[3], PL[3], n);
    int f = ((pf)K30[k].f)(PH[0], PL[0], PH[1], PL[1], PH[2], PL[2], PH[3], PL[3], OH[0], OL[0], OH[1], OL[1], n);
    for (size_t j = 0; j < n; j++) { R[j] = J64_(OH[0][j], OL[0][j]); I[j] = J64_(OH[1][j], OL[1][j]); }
    return f;
}
static void chk30(const i64 *A, const i64 *B, const i64 *Cc, const i64 *D, size_t n) {
    static i64 R[256], I[256];
    int want = 0;
    for (size_t j = 0; j < n; j++) { i128 a = A[j], b = B[j], c = Cc[j], d = D[j];
        if (!fitsum(a * c, -(b * d)) || !fitsum(a * d, b * c)) want = 1; }
    for (int k = 0; k < NK30; k++) {
        memset(R, 0x5A, sizeof R); memset(I, 0x5A, sizeof I);
        int f = call30(k, A, B, Cc, D, R, I, n);
        for (size_t j = 0; j < n; j++) {
            i128 a = A[j], b = B[j], c = Cc[j], d = D[j];
            u64 er = (u64)(a * c) - (u64)(b * d), ei = (u64)(a * d) + (u64)(b * c);
            if ((u64)R[j] != er || (u64)I[j] != ei) { kf30[k]++; if (f30++ < 10) printf("FAIL value %s j=%zu a=%lld b=%lld c=%lld d=%lld\n", K30[k].name, j, (long long)A[j], (long long)B[j], (long long)Cc[j], (long long)D[j]); }
        }
        if (f != want) { kf30[k]++; if (f30++ < 10) printf("FAIL flag %s got %d want %d (a=%lld b=%lld c=%lld d=%lld)\n", K30[k].name, f, want, (long long)A[0], (long long)B[0], (long long)Cc[0], (long long)D[0]); }
        fl30[k][f]++; c30++;
    }
}
static void do_check30(int quick) {
    make_edges();
    /* extra edges for the pieces: around 2^63 - 2^31 (h' wraps), lows of -2^31, and the all -2^31 lows */
    i128 extra[] = { ((i128)1 << 63) - ((i128)1 << 31) - 2, -((i128)1 << 63) + ((i128)1 << 31), -((i128)1 << 63) + ((i128)1 << 31) - 1,
                     ((i128)1 << 32) * 3 + ((i128)1 << 31), -((i128)1 << 32) * 3 - ((i128)1 << 31), ((i128)1 << 31) * 5,
                     (i128)(i64)0x7FFFFFFE80000000ll, (i128)(i64)0x7FFFFFFE7FFFFFFFll, (i128)(i64)0x8000000180000000ll };
    for (unsigned j = 0; j < sizeof extra / sizeof extra[0]; j++) add_edge(extra[j]);
    printf("edge values: %d\n", NE);
    static i64 A[256], B[256], Cc[256], D[256];
    long cnt = 0;
    for (int p = 0; p < NE; p++) for (int q = 0; q < NE; q++) for (int r = 0; r < NE; r++) for (int s = 0; s < NE; s++) {
        if (quick && (cnt++ % 7)) continue;
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = (p + q + r + s) & 3; A[l] = E[p]; B[l] = E[q]; Cc[l] = E[r]; D[l] = E[s];
        chk30(A, B, Cc, D, 4); if (!quick) cnt++;
    }
    printf("edge quadruples: %ld%s, failures so far %ld\n", cnt, quick ? " (every 7th)" : "", f30);
    for (int j = 0; j < 4; j++) A[j] = B[j] = Cc[j] = D[j] = INT64_MIN;
    chk30(A, B, Cc, D, 4);
    /* every combination of low pieces in {-2^31, -2^31+1, -1, 0, 2^31-1} with high pieces in a small set: hits LL' = 2^63 */
    {   static const i32 LV[] = {INT32_MIN, INT32_MIN + 1, -1, 0, 1, INT32_MAX}, HV[] = {0, -1, 1, 2, -2, 0x3FFFFFFF, -0x40000000, 0x7FFFFFFE, -0x7FFFFFFF};
        long ce = 0; int nl = 6, nh = 9;
        for (int x = 0; x < nl * nh; x++) for (int y = 0; y < nl * nh; y++) for (int z = 0; z < nl * nh; z++) for (int w = 0; w < nl; w++) {
            if (quick && (ce++ % 5)) continue;
            int l = (x + y + z + w) & 3;
            memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
            A[l] = J64_(HV[x / nl], LV[x % nl]); B[l] = J64_(HV[y / nl], LV[y % nl]); Cc[l] = J64_(HV[z / nl], LV[z % nl]);
            D[l] = J64_(HV[(x + 3 * y + 5 * z + w) % nh], LV[w]);
            chk30(A, B, Cc, D, 4); if (!quick) ce++;
        }
        printf("piece grid: %ld quadruples, failures so far %ld\n", ce, f30);
    }
    long rnd = 0;
    for (int it = 0; it < (quick ? 400000 : 4000000); it++) {
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = it & 3; A[l] = rval(); B[l] = rval(); Cc[l] = rval(); D[l] = rval();
        if (it % 4 == 1) { i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = 1; B[l] = bb; D[l] = sat(((i128)A[l] * Cc[l] - T) / bb); }
        else if (it % 4 == 2) { i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = -1; B[l] = bb; Cc[l] = sat((T - (i128)A[l] * D[l]) / bb); }
        chk30(A, B, Cc, D, 4); rnd++;
    }
    printf("random single elements: %ld, failures so far %ld\n", rnd, f30);
    long arr = 0;
    for (int it = 0; it < (quick ? 20000 : 200000); it++) {
        size_t n = 4 * (1 + next() % 32); int sm = (int)(next() % 3);
        for (size_t j = 0; j < n; j++) {
            if (sm == 0) { A[j] = rval(); B[j] = rval(); Cc[j] = rval(); D[j] = rval(); }
            else { int w = 20 + (int)(next() % 43);
                A[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); B[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1));
                Cc[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); D[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); }
        }
        if (sm == 2) A[next() % n] = rval();
        chk30(A, B, Cc, D, n); arr++;
    }
    printf("random arrays: %ld\nkernel calls %ld, failures %ld, edge fallbacks %ld\n", arr, c30, f30, fallbacks);
    for (int k = 0; k < NK30; k++) printf("  %-6s failures %ld, flag clear %ld, set %ld\n", K30[k].name, kf30[k], fl30[k][0], fl30[k][1]);
    /* side kernels: add, promotion, shrink test against exact arithmetic */
    long sf = 0;
    for (int it = 0; it < (quick ? 20000 : 200000); it++) {
        size_t n = 4 * (1 + next() % 32); int w = (int)(next() % 4);
        for (size_t j = 0; j < n; j++) { A[j] = w == 3 ? rval() : (i64)(next() >> (34 - 2 * w)) >> 1; B[j] = rval(); Cc[j] = w == 3 ? rval() : -(i64)(next() >> (34 - 2 * w)) >> 1; D[j] = rval(); }
        int want = 0; for (size_t j = 0; j < n; j++) if (!fitsum(A[j], Cc[j]) || !fitsum(B[j], D[j])) want = 1;
        static i64 R[256], I[256];
        int f1 = k_add64(A, B, Cc, D, R, I, n);
        for (size_t j = 0; j < n; j++) if ((u64)R[j] != (u64)A[j] + (u64)Cc[j] || (u64)I[j] != (u64)B[j] + (u64)D[j]) sf++;
        split(A, PH[0], PL[0], n); split(B, PH[1], PL[1], n); split(Cc, PH[2], PL[2], n); split(D, PH[3], PL[3], n);
        int f2 = k_padd(PH[0], PL[0], PH[1], PL[1], PH[2], PL[2], PH[3], PL[3], OH[0], OL[0], OH[1], OL[1], n);
        for (size_t j = 0; j < n; j++) if ((u64)J64_(OH[0][j], OL[0][j]) != (u64)A[j] + (u64)Cc[j] || (u64)J64_(OH[1][j], OL[1][j]) != (u64)B[j] + (u64)D[j]) sf++;
        sf += (f1 != want) + (f2 != want);
        int ws = 1; for (size_t j = 0; j < n; j++) if (A[j] < -(1ll << 30) || A[j] >= (1ll << 30)) ws = 0;
        sf += (k_shr64(A, n) != ws) + (k_pshr(PH[0], PL[0], n) != ws);
        static i32 x32[256]; for (size_t j = 0; j < n; j++) x32[j] = (i32)next();
        k_pro64(x32, R, n); k_ppro(x32, OH[0], OL[0], n);
        for (size_t j = 0; j < n; j++) sf += (R[j] != x32[j]) + (J64_(OH[0][j], OL[0][j]) != x32[j]);
    }
    printf("side kernels (add, shrink test, promotion), both layouts: failures %ld\n", sf);
}

/* ======================= timing ======================= */
static volatile int sink30;
static void do_time30(void) {
    const size_t sizes[3] = {512, 16384, (size_t)1 << 24}, inner[3] = {4096, 128, 1};
    static const char *pn[3] = {"L1", "L2", "memory"};
    size_t big = sizes[2];
    char *b64[6]; i32 *bp[12];
    for (int j = 0; j < 6; j++) b64[j] = (char *)aligned_alloc(4096, big * 8 + 4096) + 576 * j;
    for (int j = 0; j < 12; j++) bp[j] = (i32 *)((char *)aligned_alloc(4096, big * 4 + 8192) + 320 * j);
    enum { NT = 6 };
    static const char *nm[NT] = {"s_chk", "k_sgn", "k_v2", "k_pc", "s_pc", "k_pcb"};
    printf("ns per element (one Gaussian product); median [q1, q3] over %d reps; kernels interleaved, order rotated\n", REPS);
    for (int p = 0; p < 3; p++) {
        size_t N = sizes[p];
        for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) {
            i64 v = (i64)(next() >> 2) - (1LL << 61); ((i64 *)b64[j])[i] = v; bp[2 * j][i] = (i32)(v >> 32); bp[2 * j + 1][i] = (i32)v; }
        static double t[NT][REPS];
#define CALL30(k) ((k) == 0 ? s_chk((i64 *)b64[0], (i64 *)b64[1], (i64 *)b64[2], (i64 *)b64[3], (i64 *)b64[4], (i64 *)b64[5], N) : \
                   (k) == 1 ? k_sgn((i64 *)b64[0], (i64 *)b64[1], (i64 *)b64[2], (i64 *)b64[3], (i64 *)b64[4], (i64 *)b64[5], N) : \
                   (k) == 2 ? k_v2((i64 *)b64[0], (i64 *)b64[1], (i64 *)b64[2], (i64 *)b64[3], (i64 *)b64[4], (i64 *)b64[5], N) : \
                   (k) == 3 ? k_pc(bp[0], bp[1], bp[2], bp[3], bp[4], bp[5], bp[6], bp[7], bp[8], bp[9], bp[10], bp[11], N) : \
                   (k) == 4 ? s_pc(bp[0], bp[1], bp[2], bp[3], bp[4], bp[5], bp[6], bp[7], bp[8], bp[9], bp[10], bp[11], N) : \
                              k_pcb(bp[0], bp[1], bp[2], bp[3], bp[4], bp[5], bp[6], bp[7], bp[8], bp[9], bp[10], bp[11], N))
        for (int k = 0; k < NT; k++) sink30 += CALL30(k);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NT; j++) {
                int k = (j + r) % NT;
                sink30 += CALL30(k);
                u64 t0 = now_ns();
                for (size_t it = 0; it < inner[p]; it++) sink30 += CALL30(k);
                t[k][r] = (double)(now_ns() - t0) / (double)(N * inner[p]);
            }
        printf("\n%s, N = %zu\n", pn[p], N);
        for (int k = 0; k < NT; k++) { qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-6s %.4f [%.4f, %.4f]\n", nm[k], t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
        fflush(stdout);
    }
    /* side kernels, L1 */
    {
        size_t N = 512; static double t[6][REPS];
        static const char *sn[6] = {"k_add64", "k_padd", "k_pro64", "k_ppro", "k_shr64", "k_pshr"};
        for (int r = 0; r < REPS; r++) for (int j = 0; j < 6; j++) {
            int k = (j + r) % 6; u64 t0 = now_ns();
            for (int it = 0; it < 4096; it++) {
                if (k == 0) sink30 += k_add64((i64 *)b64[0], (i64 *)b64[1], (i64 *)b64[2], (i64 *)b64[3], (i64 *)b64[4], (i64 *)b64[5], N);
                else if (k == 1) sink30 += k_padd(bp[0], bp[1], bp[2], bp[3], bp[4], bp[5], bp[6], bp[7], bp[8], bp[9], bp[10], bp[11], N);
                else if (k == 2) k_pro64(bp[0], (i64 *)b64[4], N);
                else if (k == 3) k_ppro(bp[0], bp[8], bp[9], N);
                else if (k == 4) sink30 += k_shr64((i64 *)b64[0], N);
                else sink30 += k_pshr(bp[0], bp[1], N);
            }
            t[k][r] = (double)(now_ns() - t0) / (double)(N * 4096);
        }
        printf("\nside kernels, L1, N = 512 (add: one Gaussian add; promotion and shrink test: one int64 value)\n");
        for (int k = 0; k < 6; k++) { qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-8s %.4f [%.4f, %.4f]\n", sn[k], t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
    }
}
static void do_count30(void) {
#ifdef COUNT
    enum { N = 1024 }; static i64 b[6][N]; static i32 p[12][N];
    for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) { b[j][i] = (i64)(next() >> 34) - (1LL << 29); p[2 * j][i] = (i32)(b[j][i] >> 32); p[2 * j + 1][i] = (i32)b[j][i]; }
#define CNT(name, call) do { nops = nmem = nmul = 0; call; printf("  %-8s %6.2f / %5.2f / %4.2f\n", name, (double)nops / N, (double)nmul / N, (double)nmem / N); } while (0)
    printf("NEON instructions per element (all / multiplies / loads+stores)\n");
    CNT("k_sgn", k_sgn(b[0], b[1], b[2], b[3], b[4], b[5], N));
    CNT("k_v2", k_v2(b[0], b[1], b[2], b[3], b[4], b[5], N));
    CNT("k_pc", k_pc(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], N));
    CNT("k_pcb", k_pcb(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], N));
    CNT("k_add64", k_add64(b[0], b[1], b[2], b[3], b[4], b[5], N));
    CNT("k_padd", k_padd(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], N));
    CNT("k_pro64", k_pro64(p[0], b[4], N));
    CNT("k_ppro", k_ppro(p[0], p[8], p[9], N));
    CNT("k_shr64", k_shr64(b[0], N));
    CNT("k_pshr", k_pshr(p[0], p[1], N));
#else
    printf("build with -DCOUNT\n");
#endif
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check30(0);
    else if (!strcmp(mode, "quick")) do_check30(1);
    else if (!strcmp(mode, "count")) do_count30();
    else if (!strcmp(mode, "scan")) scan_kernels(argv[0]);
    else do_time30();
    return 0;
}
#endif
