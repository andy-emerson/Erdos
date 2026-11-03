/* Study 29, milestone 2: study 23's int64-half Gaussian multiply in NEON shape on the x86 stand-in (neon29mul.c)
 *
 * re = ac - bd, im = ad + bc with int64 halves, separate arrays; each kernel writes the low 64 bits and (checked
 * kernels) returns 1 iff some true re or im does not fit int64. Only the widening 32 x 32 -> 64 multiply and the
 * 32-bit low multiply (no 64-bit lane multiply, as on NEON). 4 elements per pass: two 128-bit registers per array.
 *
 * NEON shape (AVX2 form in brackets):
 *   pieces    two registers of a give lo(a) = uzp1, hi(a) = uzp2 packed in 4 lanes (2 ops per 4 values)
 *             [pmuludq reads the low dword in place; hi by a shift per register]
 *             signed pieces: hi = addhn(a, 2^31) (add and narrow the high half, 2 ops per 4 values) [add, shift]
 *   products  smull/smull2 and smlal/smlsl(2) fuse each column's two products and their sum or difference [mul, mul, add]
 *   splits    re's columns (differences inside int64): floor(m / 2^32) by ssra (shift-right-accumulate, signed),
 *             lo(m) by and; im's columns (sums that can reach 2^63) keep study 23's offsets, then usra [xor, srl, blend]
 *   borrows   (unsigned pieces) unsigned 64-bit compare cmhi, one op [bias xor x2, pcmpgtq]
 *   bare      the 2^32 column needs only 32 bits: mul, mla, mls on packed lanes (4 per 4 elements), shll, add
 *             [4 more pmuludq per register]; k_barew keeps the widening form for comparison
 *   flag      the k test ORed into a register, tested once per array [the same, ptest]
 *
 * Kernels: s_bare, s_chk (scalar, study 20/23), k_bare, k_barew, k_sgn (signed pieces), k_uns (unsigned pieces),
 * k_bnd (k_bare + per-array bound p + q <= 61, else k_sgn). N a multiple of 4.
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -o neon29mul neon29mul.c
 *        (-DCOUNT counts; -DFAULT=k faults; add -ffixed-xmm16 ... -ffixed-xmm31 for a 16-register build)
 * Run:   ./neon29mul check | quick | time [Nmem] | count | scan
 * neon29shr.c includes this file with NEON29_NO_MAIN defined, for the kernels only.
 */
#include "neon29isa.c"
#ifndef FAULT
#define FAULT 0
#endif
/* faults: 1 drop the e_a terms of k_sgn's re correction; 2 offB = offA on k_sgn's im; 3 drop k_uns's re borrow;
 * 4 logical (not arithmetic) split of re's m1 in k_sgn; 5 addhn without the 2^31 offset; 6 k_bare's mid column
 * missing b_l d_h */

#define ARGS const i64 *restrict ar, const i64 *restrict ai, const i64 *restrict br, const i64 *restrict bi, \
             i64 *restrict cr, i64 *restrict ci, size_t N
typedef int (*kf)(ARGS);

/* ======================= scalar (study 20) ======================= */
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_bare(ARGS) {
    for (size_t i = 0; i < N; i++) {
        u64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        cr[i] = (i64)(a * c - b * d); ci[i] = (i64)(a * d + b * c);
    }
    return 0;
}
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_chk(ARGS) {
    u64 o = 0;
    for (size_t i = 0; i < N; i++) {
        i64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        i128 re = (i128)a * c - (i128)b * d;
        i128 im = (i128)((u128)((i128)a * d) + (u128)((i128)b * c));
        i64 rl = (i64)re, il = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl >> 63)) | ((u64)(im >> 64) ^ (u64)(il >> 63));
        cr[i] = rl; ci[i] = il;
    }
    return o != 0;
}

/* ======================= NEON-shaped helpers ======================= */
#define C64_(x) vdupq_n_s64(x)
#define LOW_ C64_(0xFFFFFFFFll)
#ifndef A64
/* addhn, addhn2: high halves of (lo + c), (hi + c), packed (lo's in the "lo" lanes) */
#define vaddhn_pair(lo, hi, c) ((void)C_(0), C_(_mm_blend_epi32(_mm_srli_epi64(_mm_add_epi64(lo, c), 32), _mm_add_epi64(hi, c), 0xA)))
/* ssra: c + (a >> n) arithmetic, 64-bit */
#define vsraq_n_s64(c, a, n) C_(_mm_add_epi64(c, _mm_srai_epi64(a, n)))
#endif
#define LD2(p, i) ((P2){vld1q((p) + (i)), vld1q((p) + (i) + 2)})
#define ST2(p, i, v) (vst1q((p) + (i), (v).l), vst1q((p) + (i) + 2, (v).h))

/* the bare product: lo columns by widening, the 2^32 column in 32 bits */
AI void bare4(V al, V ah, V bl, V bh, V cl, V ch, V dl, V dh, P2 *re, P2 *im) {
    V mr = vmlsq_s32(vmlsq_s32(vmlaq_s32(vmulq_s32(ah, cl), al, ch), bh, dl), bl, dh);
#if FAULT == 6
    V mi = vmlaq_s32(vmlaq_s32(vmlaq_s32(vmulq_s32(ah, dl), al, dh), bh, cl), bl, bl);
#else
    V mi = vmlaq_s32(vmlaq_s32(vmlaq_s32(vmulq_s32(ah, dl), al, dh), bh, cl), bl, ch);
#endif
    re->l = vaddq_s64(vmlsl_lo_u32(vmull_lo_u32(al, cl), bl, dl), vshll32_lo(mr));
    re->h = vaddq_s64(vmlsl_high_u32(vmull_high_u32(al, cl), bl, dl), vshll32_high(mr));
    im->l = vaddq_s64(vmlal_lo_u32(vmull_lo_u32(al, dl), bl, cl), vshll32_lo(mi));
    im->h = vaddq_s64(vmlal_high_u32(vmull_high_u32(al, dl), bl, cl), vshll32_high(mi));
}
NOINL int k_bare(ARGS) {
    for (size_t i = 0; i < N; i += 4) {
        P2 a = LD2(ar, i), b = LD2(ai, i), c = LD2(br, i), d = LD2(bi, i), re, im;
        bare4(vuzp1q_w(a.l, a.h), vuzp2q_w(a.l, a.h), vuzp1q_w(b.l, b.h), vuzp2q_w(b.l, b.h),
              vuzp1q_w(c.l, c.h), vuzp2q_w(c.l, c.h), vuzp1q_w(d.l, d.h), vuzp2q_w(d.l, d.h), &re, &im);
        ST2(cr, i, re); ST2(ci, i, im);
    }
    return 0;
}
/* study 23's bare form, widening throughout: mid = hi(a) lo(c) + lo(a) hi(c) - ... in 64 bits, then << 32 */
NOINL int k_barew(ARGS) {
    for (size_t i = 0; i < N; i += 4) {
        P2 a = LD2(ar, i), b = LD2(ai, i), c = LD2(br, i), d = LD2(bi, i), re, im;
        V al = vuzp1q_w(a.l, a.h), ah = vuzp2q_w(a.l, a.h), bl = vuzp1q_w(b.l, b.h), bh = vuzp2q_w(b.l, b.h);
        V cl = vuzp1q_w(c.l, c.h), ch = vuzp2q_w(c.l, c.h), dl = vuzp1q_w(d.l, d.h), dh = vuzp2q_w(d.l, d.h);
        V ml = vmlsl_lo_u32(vmlsl_lo_u32(vmlal_lo_u32(vmull_lo_u32(ah, cl), al, ch), bh, dl), bl, dh);
        V mh = vmlsl_high_u32(vmlsl_high_u32(vmlal_high_u32(vmull_high_u32(ah, cl), al, ch), bh, dl), bl, dh);
        V nl = vmlal_lo_u32(vmlal_lo_u32(vmlal_lo_u32(vmull_lo_u32(ah, dl), al, dh), bh, cl), bl, ch);
        V nh = vmlal_high_u32(vmlal_high_u32(vmlal_high_u32(vmull_high_u32(ah, dl), al, dh), bh, cl), bl, ch);
        re.l = vaddq_s64(vmlsl_lo_u32(vmull_lo_u32(al, cl), bl, dl), vshlq_n_s64(ml, 32));
        re.h = vaddq_s64(vmlsl_high_u32(vmull_high_u32(al, cl), bl, dl), vshlq_n_s64(mh, 32));
        im.l = vaddq_s64(vmlal_lo_u32(vmull_lo_u32(al, dl), bl, cl), vshlq_n_s64(nl, 32));
        im.h = vaddq_s64(vmlal_high_u32(vmull_high_u32(al, dl), bl, cl), vshlq_n_s64(nh, 32));
        ST2(cr, i, re); ST2(ci, i, im);
    }
    return 0;
}

/* ======================= signed pieces (study 23's v_sgn) =======================
 * x = xh 2^32 + xl, xl the low dword as signed, xh = hi32(x + 2^31) (addhn). That covers [-2^63, 2^63 - 2^31); an
 * edge value x >= 2^63 - 2^31 is read as x - 2^64, which adds e_a c + e_c a to k for a product ac (masked adds).
 * k = floor((v + 2^63) / 2^64), mod 2^64; the result fits iff k = 0.
 * re: columns hh, m1, m2, ll are differences inside int64, so k = hh + floor(m1/2^32) + floor(m2/2^32)
 *     + ((lo(m1) + lo(m2) + ((ll + 2^63) >> 32)) >> 32): two ssra, two and, add, usra, usra.
 * im: columns are sums in [-2^63 + 2^32, 2^63] (2^63 wraps), so study 23's offsets: t1 = m1 + offA, t2 = m2 + offB,
 *     T = ll + offA (offA = 2^63 - 2^32, offB = offA + 1), then k + 2^32 - 2 = hh + t1 >> 32 + t2 >> 32
 *     + ((lo(t1) + lo(t2) + T >> 32) >> 32). */
AI void sgn_half(V ahs, V bhs, V chs, V dhs, V als, V bls, V cls, V dls, int high, V *re, V *im, V *kr, V *ki) {
    (void)0;
#define ML_(c, a, b) (high ? vmlal_high_s32(c, a, b) : vmlal_lo_s32(c, a, b))
#define MS_(c, a, b) (high ? vmlsl_high_s32(c, a, b) : vmlsl_lo_s32(c, a, b))
#define MU_(a, b)    (high ? vmull_high_s32(a, b) : vmull_lo_s32(a, b))
    const V sign = C64_(INT64_MIN), offA = C64_(0x7FFFFFFF00000000LL), offB = C64_(0x7FFFFFFF00000001LL);
    {
        V hh = MS_(MU_(ahs, chs), bhs, dhs), m1 = MS_(MU_(ahs, cls), bhs, dls);
        V m2 = MS_(MU_(als, chs), bls, dhs), ll = MS_(MU_(als, cls), bls, dls);
        *re = vaddq_s64(ll, vshlq_n_s64(vaddq_s64(m1, m2), 32));
#if FAULT == 4
        V k = vsraq_n_s64(vsraq_n_u64(hh, m1, 32), m2, 32);
#else
        V k = vsraq_n_s64(vsraq_n_s64(hh, m1, 32), m2, 32);
#endif
        V z = vsraq_n_u64(vaddq_s64(vandq(m1, LOW_), vandq(m2, LOW_)), veorq(ll, sign), 32);
        *kr = vsraq_n_u64(k, z, 32);
    }
    {
        V hh = ML_(MU_(ahs, dhs), bhs, chs), m1 = ML_(MU_(ahs, dls), bhs, cls);
        V m2 = ML_(MU_(als, dhs), bls, chs), ll = ML_(MU_(als, dls), bls, cls);
        *im = vaddq_s64(ll, vshlq_n_s64(vaddq_s64(m1, m2), 32));
#if FAULT == 2
        V t1 = vaddq_s64(m1, offA), t2 = vaddq_s64(m2, offA); (void)offB;
#else
        V t1 = vaddq_s64(m1, offA), t2 = vaddq_s64(m2, offB);
#endif
        V z = vsraq_n_u64(vaddq_s64(vandq(t1, LOW_), vandq(t2, LOW_)), vaddq_s64(ll, offA), 32);
        *ki = vsraq_n_u64(vsraq_n_u64(vsraq_n_u64(hh, t1, 32), t2, 32), z, 32);
    }
#undef ML_
#undef MS_
#undef MU_
}
/* one pass of 4 elements: stores re, im and returns the k test (nonzero: some result does not fit) */
AI V sgn_pass(P2 a, P2 b, P2 c, P2 d, P2 *re, P2 *im) {
#if FAULT == 5
    const V off31 = C64_(0);
#else
    const V off31 = C64_(1LL << 31);
#endif
    const V edge = C64_(0x7FFFFFFF7FFFFFFFLL), kim = C64_((1LL << 32) - 2);
    P2 kr, ki;
    V als = vuzp1q_w(a.l, a.h), bls = vuzp1q_w(b.l, b.h), cls = vuzp1q_w(c.l, c.h), dls = vuzp1q_w(d.l, d.h);
    V ahs = vaddhn_pair(a.l, a.h, off31), bhs = vaddhn_pair(b.l, b.h, off31);
    V chs = vaddhn_pair(c.l, c.h, off31), dhs = vaddhn_pair(d.l, d.h, off31);
    sgn_half(ahs, bhs, chs, dhs, als, bls, cls, dls, 0, &re->l, &im->l, &kr.l, &ki.l);
    sgn_half(ahs, bhs, chs, dhs, als, bls, cls, dls, 1, &re->h, &im->h, &kr.h, &ki.h);
    /* edge corrections (mod 2^64) */
    P2 ea = {vcgtq_s64(a.l, edge), vcgtq_s64(a.h, edge)}, eb = {vcgtq_s64(b.l, edge), vcgtq_s64(b.h, edge)};
    P2 ec = {vcgtq_s64(c.l, edge), vcgtq_s64(c.h, edge)}, ed = {vcgtq_s64(d.l, edge), vcgtq_s64(d.h, edge)};
#if FAULT == 1
#define EA_(x, h) vzero()
#else
#define EA_(x, h) vandq(x.h, ea.h)
#endif
#define KCORR(h)                                                                                                    \
    kr.h = vaddq_s64(kr.h, vsubq_s64(vaddq_s64(EA_(c, h), vandq(a.h, ec.h)), vaddq_s64(vandq(d.h, eb.h), vandq(b.h, ed.h)))); \
    ki.h = vaddq_s64(ki.h, vsubq_s64(vaddq_s64(vaddq_s64(vandq(d.h, ea.h), vandq(a.h, ed.h)), vaddq_s64(vandq(c.h, eb.h), vandq(b.h, ec.h))), kim));
    KCORR(l) KCORR(h)
#undef KCORR
#undef EA_
    return vorrq(vorrq(kr.l, ki.l), vorrq(kr.h, ki.h));
}
NOINL int k_sgn(ARGS) {
    V o = vzero();
    for (size_t i = 0; i < N; i += 4) {
        P2 re, im;
        o = vorrq(o, sgn_pass(LD2(ar, i), LD2(ai, i), LD2(br, i), LD2(bi, i), &re, &im));
        ST2(cr, i, re); ST2(ci, i, im);
    }
    return vany(o);
}

/* ======================= unsigned pieces (study 23's v_uns) =======================
 * x read as unsigned (x + 2^64 when x < 0): k gets -(s_a c + s_c a) per product (s = sign masks). Each middle product
 * is split on its own; the low products' borrow (re) and carry (im) by the unsigned compare cmhi. */
AI void uns_half(V ah, V bh, V ch, V dh, V al, V bl, V cl, V dl, int high, V *re, V *im, V *kr, V *ki) {
#define UL_(c, a, b) (high ? vmlal_high_u32(c, a, b) : vmlal_lo_u32(c, a, b))
#define US_(c, a, b) (high ? vmlsl_high_u32(c, a, b) : vmlsl_lo_u32(c, a, b))
#define UU_(a, b)    (high ? vmull_high_u32(a, b) : vmull_lo_u32(a, b))
    const V kre = C64_((1LL << 31) + (1LL << 33)), kimc = C64_(1LL << 31);
    {
        V p = UU_(al, cl), q = UU_(bl, dl), D = vsubq_s64(p, q);
#if FAULT == 3
        V bw = vzero();
#else
        V bw = vcgtq_u64(q, p);                                 /* -1 where p < q */
#endif
        V hh = US_(UU_(ah, ch), bh, dh);
        V m1 = UU_(ah, cl), m2 = UU_(al, ch), m3 = UU_(bh, dl), m4 = UU_(bl, dh);
        V L = vsubq_s64(vaddq_s64(vandq(m1, LOW_), vandq(m2, LOW_)), vaddq_s64(vandq(m3, LOW_), vandq(m4, LOW_)));
        V H = vsubq_s64(vsraq_n_u64(vshrq_n_u64(m1, 32), m2, 32), vsraq_n_u64(vshrq_n_u64(m3, 32), m4, 32));
        *re = vaddq_s64(D, vshlq_n_s64(L, 32));
        V z = vshrq_n_u64(vaddq_s64(vsraq_n_u64(L, D, 32), kre), 32);   /* carry + 2 */
        *kr = vaddq_s64(vaddq_s64(hh, H), vaddq_s64(bw, z));           /* k + 2, before the sign corrections */
    }
    {
        V p = UU_(al, dl), q = UU_(bl, cl), D = vaddq_s64(p, q);
        V cy = vcgtq_u64(p, D);                                 /* -1 where p + q carried */
        V hh = UL_(UU_(ah, dh), bh, ch);
        V m1 = UU_(ah, dl), m2 = UU_(al, dh), m3 = UU_(bh, cl), m4 = UU_(bl, ch);
        V L = vaddq_s64(vaddq_s64(vandq(m1, LOW_), vandq(m2, LOW_)), vaddq_s64(vandq(m3, LOW_), vandq(m4, LOW_)));
        V H = vaddq_s64(vsraq_n_u64(vshrq_n_u64(m1, 32), m2, 32), vsraq_n_u64(vshrq_n_u64(m3, 32), m4, 32));
        *im = vaddq_s64(D, vshlq_n_s64(L, 32));
        V z = vshrq_n_u64(vaddq_s64(vsraq_n_u64(L, D, 32), kimc), 32);
        *ki = vsubq_s64(vaddq_s64(hh, H), vsubq_s64(cy, z));          /* k, before the sign corrections */
    }
#undef UL_
#undef US_
#undef UU_
}
NOINL int k_uns(ARGS) {
    const V two = C64_(2);
    V o = vzero();
    for (size_t i = 0; i < N; i += 4) {
        P2 a = LD2(ar, i), b = LD2(ai, i), c = LD2(br, i), d = LD2(bi, i), re, im, kr, ki;
        V al = vuzp1q_w(a.l, a.h), ah = vuzp2q_w(a.l, a.h), bl = vuzp1q_w(b.l, b.h), bh = vuzp2q_w(b.l, b.h);
        V cl = vuzp1q_w(c.l, c.h), ch = vuzp2q_w(c.l, c.h), dl = vuzp1q_w(d.l, d.h), dh = vuzp2q_w(d.l, d.h);
        uns_half(ah, bh, ch, dh, al, bl, cl, dl, 0, &re.l, &im.l, &kr.l, &ki.l);
        uns_half(ah, bh, ch, dh, al, bl, cl, dl, 1, &re.h, &im.h, &kr.h, &ki.h);
        ST2(cr, i, re); ST2(ci, i, im);
        P2 sa = {vcltzq_s64(a.l), vcltzq_s64(a.h)}, sb = {vcltzq_s64(b.l), vcltzq_s64(b.h)};
        P2 sc = {vcltzq_s64(c.l), vcltzq_s64(c.h)}, sd = {vcltzq_s64(d.l), vcltzq_s64(d.h)};
#define UCORR(h)                                                                                                    \
        kr.h = vsubq_s64(kr.h, vaddq_s64(vsubq_s64(vaddq_s64(vandq(c.h, sa.h), vandq(a.h, sc.h)), vaddq_s64(vandq(d.h, sb.h), vandq(b.h, sd.h))), two)); \
        ki.h = vsubq_s64(ki.h, vaddq_s64(vaddq_s64(vandq(d.h, sa.h), vandq(a.h, sd.h)), vaddq_s64(vandq(c.h, sb.h), vandq(b.h, sc.h))));
        UCORR(l) UCORR(h)
        o = vorrq(o, vorrq(vorrq(kr.l, ki.l), vorrq(kr.h, ki.h)));
    }
    return vany(o);
}

/* ======================= bare + per-array bound, exact fallback (study 23's v_bnd) ======================= */
static long bnd_fast;
static int bitlen(V s) { u64 x = vgetq_u64_(s, 0) | vgetq_u64_(s, 1); return x ? 64 - __builtin_clzll(x) : 0; }
NOINL int k_bnd(ARGS) {
    V sx = vzero(), sy = vzero();
    for (size_t i = 0; i < N; i += 4) {
        P2 a = LD2(ar, i), b = LD2(ai, i), c = LD2(br, i), d = LD2(bi, i), re, im;
        bare4(vuzp1q_w(a.l, a.h), vuzp2q_w(a.l, a.h), vuzp1q_w(b.l, b.h), vuzp2q_w(b.l, b.h),
              vuzp1q_w(c.l, c.h), vuzp2q_w(c.l, c.h), vuzp1q_w(d.l, d.h), vuzp2q_w(d.l, d.h), &re, &im);
        ST2(cr, i, re); ST2(ci, i, im);
        /* |x| - [x < 0] = x XOR (x >> 63) */
        sx = vorrq(sx, vorrq(vorrq(veorq(a.l, vcltzq_s64(a.l)), veorq(a.h, vcltzq_s64(a.h))), vorrq(veorq(b.l, vcltzq_s64(b.l)), veorq(b.h, vcltzq_s64(b.h)))));
        sy = vorrq(sy, vorrq(vorrq(veorq(c.l, vcltzq_s64(c.l)), veorq(c.h, vcltzq_s64(c.h))), vorrq(veorq(d.l, vcltzq_s64(d.l)), veorq(d.h, vcltzq_s64(d.h)))));
    }
    if (bitlen(sx) + bitlen(sy) <= 61) { bnd_fast++; return 0; }
    return k_sgn(ar, ai, br, bi, cr, ci, N);
}

#if !defined(NEON29_NO_MAIN) && !defined(A64)
typedef struct { const char *name; int chk; kf f; } kern;
static kern KT[] = {
    {"s_bare", 0, s_bare}, {"s_chk", 1, s_chk}, {"k_bare", 0, k_bare}, {"k_barew", 0, k_barew},
    {"k_uns", 1, k_uns}, {"k_sgn", 1, k_sgn}, {"k_bnd", 1, k_bnd},
};
#define NKT (int)(sizeof KT / sizeof KT[0])

/* ======================= correctness (mul64.c's harness) ======================= */
static i64 E[128]; static int NE;
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
static long fails, cases, nflag[16][2], nnear[2], kfails[16];
static int near(i128 x, i128 y) {
    i128 B = (i128)1 << 63;
    if (x > 0 && y > 0 && (x > B + 8 || y > B + 8)) return 0;
    if (x < 0 && y < 0 && (x < -B - 8 || y < -B - 8)) return 0;
    i128 v = x + y;
    return (v >= B - 4 && v <= B + 3) || (v >= -B - 4 && v <= -B + 3);
}
static int fitsum(i128 x, i128 y) { i128 lo = -((i128)1 << 63), hi = ((i128)1 << 63) - 1; return y >= lo - x && y <= hi - x; }
static void check_arr(const i64 *A, const i64 *B, const i64 *Cc, const i64 *D, size_t N) {
    static i64 outr[256], outi[256];
    int want = 0;
    for (size_t j = 0; j < N; j++) {
        i128 a = A[j], b = B[j], c = Cc[j], d = D[j];
        if (!fitsum(a * c, -(b * d)) || !fitsum(a * d, b * c)) want = 1;
        if (near(a * c, -(b * d)) || near(a * d, b * c)) nnear[fitsum(a * c, -(b * d)) && fitsum(a * d, b * c)]++;
    }
    for (int k = 0; k < NKT; k++) {
        memset(outr, 0x5A, sizeof outr); memset(outi, 0x5A, sizeof outi);
        int f = KT[k].f(A, B, Cc, D, outr, outi, N);
        for (size_t j = 0; j < N; j++) {
            i128 a = A[j], b = B[j], c = Cc[j], d = D[j];
            u64 er = (u64)(a * c) - (u64)(b * d), ei = (u64)(a * d) + (u64)(b * c);
            if ((u64)outr[j] != er || (u64)outi[j] != ei) { kfails[k]++;
                if (fails++ < 10) printf("FAIL value %s j=%zu a=%lld b=%lld c=%lld d=%lld\n", KT[k].name, j, (long long)A[j], (long long)B[j], (long long)Cc[j], (long long)D[j]); }
        }
        if (KT[k].chk) {
            if (f != want) { kfails[k]++; if (fails++ < 10) printf("FAIL flag %s got %d want %d (a=%lld b=%lld c=%lld d=%lld)\n", KT[k].name, f, want,
                                                    (long long)A[0], (long long)B[0], (long long)Cc[0], (long long)D[0]); }
            nflag[k][f]++;
        }
        cases++;
    }
}
static i64 sat(i128 v) { if (v >= ((i128)1 << 63)) return INT64_MAX; if (v < -((i128)1 << 63)) return INT64_MIN; return (i64)v; }
static void do_check(int quick) {
    make_edges();
    printf("edge values: %d\n", NE);
    static i64 A[256], B[256], Cc[256], D[256];
    long cnt = 0;
    for (int p = 0; p < NE; p++) for (int q = 0; q < NE; q++) for (int r = 0; r < NE; r++) for (int s = 0; s < NE; s++) {
        if (quick && (cnt++ % 7)) continue;
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = (p + q + r + s) & 3;
        A[l] = E[p]; B[l] = E[q]; Cc[l] = E[r]; D[l] = E[s];
        check_arr(A, B, Cc, D, 4);
        if (!quick) cnt++;
    }
    printf("edge quadruples: %ld%s\n", cnt, quick ? " (every 7th checked)" : "");
    for (int j = 0; j < 4; j++) A[j] = B[j] = Cc[j] = D[j] = INT64_MIN;
    check_arr(A, B, Cc, D, 4);
    long rnd = 0;
    for (int it = 0; it < (quick ? 400000 : 4000000); it++) {
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = it & 3;
        A[l] = rval(); B[l] = rval(); Cc[l] = rval(); D[l] = rval();
        if (it % 4 == 1) {
            i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = 1;
            B[l] = bb; D[l] = sat(((i128)A[l] * Cc[l] - T) / bb);
        } else if (it % 4 == 2) {
            i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = -1;
            B[l] = bb; Cc[l] = sat((T - (i128)A[l] * D[l]) / bb);
        }
        check_arr(A, B, Cc, D, 4);
        rnd++;
    }
    printf("random single elements: %ld (half aimed at the int64 boundary)\n", rnd);
    long arr = 0;
    for (int it = 0; it < (quick ? 20000 : 200000); it++) {
        size_t N = 4 * (1 + next() % 32);
        int sm = (int)(next() % 3);
        for (size_t j = 0; j < N; j++) {
            if (sm == 0) { A[j] = rval(); B[j] = rval(); Cc[j] = rval(); D[j] = rval(); }
            else { int w = 20 + (int)(next() % 11);
                A[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); B[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1));
                Cc[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); D[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); }
        }
        if (sm == 2) A[next() % N] = rval();
        check_arr(A, B, Cc, D, N);
        arr++;
    }
    printf("random arrays: %ld (length 4..128)\n", arr);
    printf("elements whose re or im is within 4 of the boundary: %ld fit, %ld overflow\n", nnear[1], nnear[0]);
    printf("kernel calls %ld, failures %ld\n", cases, fails);
    for (int k = 0; k < NKT; k++) if (kfails[k]) printf("  %-8s %ld failures\n", KT[k].name, kfails[k]);
    printf("k_bnd settled by the bound: %ld calls (the rest fell back to k_sgn)\n", bnd_fast);
    for (int k = 0; k < NKT; k++) if (KT[k].chk) printf("  %-8s flag clear %ld, set %ld\n", KT[k].name, nflag[k][0], nflag[k][1]);
}

/* ======================= timing (mul64.c's harness) ======================= */
static volatile int sink;
static void do_time(size_t bigN) {
    char *buf[6];
    for (int j = 0; j < 6; j++) buf[j] = (char *)aligned_alloc(4096, bigN * 8 + 4096) + 576 * j;
    const size_t sizes[3] = {512, 16384, bigN}, inner[3] = {4096, 128, 1};
    static const char *pn[3] = {"L1", "L2", "memory"};
    printf("ns per element (one Gaussian product); median [q1, q3] over %d reps; kernels interleaved, order rotated\n", REPS);
    for (int p = 0; p < 3; p++) {
        size_t N = sizes[p];
        for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) ((i64 *)buf[j])[i] = (i64)(next() >> 34) - (1LL << 29);
        memset(buf[4], 0, N * 8); memset(buf[5], 0, N * 8);
        static double t[NKT][REPS];
#define CALL(k) KT[k].f((i64 *)buf[0], (i64 *)buf[1], (i64 *)buf[2], (i64 *)buf[3], (i64 *)buf[4], (i64 *)buf[5], N)
        for (int k = 0; k < NKT; k++) sink += CALL(k);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NKT; j++) {
                int k = (j + r) % NKT;
                sink += CALL(k);
                u64 t0 = now_ns();
                for (size_t it = 0; it < inner[p]; it++) sink += CALL(k);
                t[k][r] = (double)(now_ns() - t0) / (double)(N * inner[p]);
            }
        printf("\n%s, N = %zu (%zu KB per array)\n", pn[p], N, N * 8 / 1024);
        for (int k = 0; k < NKT; k++) { qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-8s %.4f [%.4f, %.4f]\n", KT[k].name, t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
        fflush(stdout);
    }
}
static void do_count(void) {
#ifdef COUNT
    enum { N = 1024 }; static i64 b[6][N];
    for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) b[j][i] = (i64)(next() >> 34) - (1LL << 29);
    printf("NEON instructions per element (all / multiplies / loads+stores)\n");
    for (int k = 2; k < NKT; k++) { nops = nmem = nmul = 0; KT[k].f(b[0], b[1], b[2], b[3], b[4], b[5], N);
        printf("  %-8s %6.2f / %5.2f / %4.2f\n", KT[k].name, (double)nops / N, (double)nmul / N, (double)nmem / N); }
#else
    printf("build with -DCOUNT\n");
#endif
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check(0);
    else if (!strcmp(mode, "quick")) do_check(1);
    else if (!strcmp(mode, "count")) do_count();
    else if (!strcmp(mode, "scan")) scan_kernels(argv[0]);
    else do_time(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 25);
    return 0;
}
#endif /* NEON29_NO_MAIN */
