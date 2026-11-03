/* Study 27: integer-only lane ctz for one set bit, AVX2 (ctz27.c)
 *
 * k = ctz(o) in 32-bit lanes, from x = o & -o = 2^k, with no floating point (except ctz_flt, study 24's float
 * exponent, kept as the reference). Each form is timed inside study 26's vector binary gcd (pi_v3: 3 registers;
 * pi_v: 1 register), with study 26's -2^31 guard and fixup. Op counts include the 2 ops of x = o & -o.
 *   flt    float exponent: cvtdq2ps, srl 23, sub 127                                     5 ops (reference)
 *   bs     study 26 (a): binary search with variable shifts                               22 ops
 *   pop    study 26 (b): popcount of (x & -x) - 1, nibble table, maddubs + maddwd         11 ops
 *   pop2   pop on the thermometer andn(o, o - 1) = 2^k - 1                                10 ops
 *   db     study 26 (c): de Bruijn, vpmulld, top 5 bits, two vpshufb (add 0x70 / sub 0x10) 9 ops
 *   db3    de Bruijn, vpmulld, top 5 bits; k = Tb[i] xor Td[i + 0x70] (one index op)      8 ops
 *   dbm    16-bit de Bruijn 0x09AF through vpmaddwd (5 cycles, not vpmulld's 10): k mod 16
 *          from one vpshufb, plus 16 [k >= 16] from one mask test                          9 ops
 *   dbh    vpmullw by 0x09AF in each word, vpsrlw 12, one vpshufb; the word found is moved
 *          down by vpsrlvd by 16 [k >= 16], which is then ORed in                          9 ops
 *   mask5  the five bit tests: k_j 2^j = min_epu32(x & M_j, 2^j), ORed                     16 ops
 *   mask5c the five bit tests with compare masks: k = 31 + sum (cmpeq(x & M_j, 0) & -2^j)  22 ops
 *   mix    8 [k >= 8 in byte] from two tests, x >> that, then two nibble tables           13 ops
 *   mixn   4 (k >> 2) from three tests, x >> that, then one nibble table                 13 ops
 *
 * Build: gcc -O2 -mavx2 -o ctz27 ctz27.c
 * Run:   ./ctz27 check | ctz | time | icount KIND
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int32_t i32; typedef uint32_t u32; typedef int64_t i64; typedef uint64_t u64;
typedef __int128 i128;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define AI static inline __attribute__((always_inline))
#define HMAX 2147483647
#define IMIN INT32_MIN
#define ARGS const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
             i32 *restrict gr, i32 *restrict gi, size_t N
#define PASSG ar, ai, br, bi, gr, gi, N

/* ======================= exact path for pairs with a -2^31 half ======================= */
static inline int ctz64(u64 v) { return v ? __builtin_ctzll(v) : 64; }
static inline void normalise(i64 *px, i64 *py) {
    i64 x = *px, y = *py;
    if (y < 0 || (y == 0 && x < 0)) { x = -x; y = -y; }
    if (x <= 0 && y > 0) { i64 t = x; x = y; y = -t; }
    *px = x; *py = y;
}
/* (1+i)-ary gcd in 64-bit registers (study 24's model): every value stays within |v| <= 2^31 */
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
/* once per array, only when the sticky flag is set: recompute the pairs with a -2^31 half; 1 if a gcd overflows */
NOINL u32 fixup(ARGS) {
    u32 ov = 0;
    for (size_t i = 0; i < N; i++) {
        if (ar[i] != IMIN && ai[i] != IMIN && br[i] != IMIN && bi[i] != IMIN) continue;
        i64 g1, g2; pi64(ar[i], ai[i], br[i], bi[i], &g1, &g2);
        ov |= g1 > HMAX;                    /* g1 >= 0, |g2| <= 2^31 and g2 >= 0 after normalising */
        gr[i] = (i32)(u32)g1; gi[i] = (i32)(u32)g2;
    }
    return ov;
}
#define HASMIN(i) (ar[i] == IMIN || ai[i] == IMIN || br[i] == IMIN || bi[i] == IMIN)
/* ======================= vector helpers ======================= */
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define ANDN(a, b) _mm256_andnot_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define ADD(a, b) _mm256_add_epi32(a, b)
#define SUB(a, b) _mm256_sub_epi32(a, b)
#define ADD64(a, b) _mm256_add_epi64(a, b)
#define SUB64(a, b) _mm256_sub_epi64(a, b)
#define MULS(a, b) _mm256_mul_epi32(a, b)
#define MULU(a, b) _mm256_mul_epu32(a, b)
#define MULLO(a, b) _mm256_mullo_epi32(a, b)
#define HI32(a) _mm256_srli_epi64(a, 32)
#define GT64(a, b) _mm256_cmpgt_epi64(a, b)
#define SRA1(a) _mm256_srai_epi32(a, 1)
#define EQ(a, b) _mm256_cmpeq_epi32(a, b)
#define C32(x) _mm256_set1_epi32((int)(x))
#define C64(x) _mm256_set1_epi64x((long long)(x))
#define SEL(a, b, m) _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), _mm256_castsi256_ps(m)))
#define PACK(e, o) _mm256_blend_epi32(e, o, 0xAA)
#define PACKQ(e, o) _mm256_blend_epi32(e, _mm256_slli_epi64(o, 32), 0xAA)


/* ======================= lane ctz forms (o != 0; o = 0 gives some value the callers do not use) ======================= */
#define ZERO _mm256_setzero_si256()
#define ISO(o) AND(o, SUB(ZERO, o))
#define TBL(...) _mm256_setr_epi8(__VA_ARGS__, __VA_ARGS__)
AI V ctz_flt(V o) {                                  /* study 24: the converter's priority encoder (k = 31 fails) */
    return SUB(_mm256_srli_epi32(_mm256_castps_si256(_mm256_cvtepi32_ps(ISO(o))), 23), C32(127));
}
AI V ctz_bs(V o) {
    const V z = ZERO;
    V k = AND(EQ(AND(o, C32(0xFFFF)), z), C32(16)); V n = k; o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0xFF)), z), C32(8)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0xF)), z), C32(4)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0x3)), z), C32(2)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    return ADD(n, ANDN(o, C32(1)));
}
AI V popt(V t) {                                     /* popcount per lane: nibble table, maddubs + maddwd */
    const V nib = C32(0x0F0F0F0F), tbl = TBL(0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    V cnt = _mm256_add_epi8(_mm256_shuffle_epi8(tbl, AND(t, nib)), _mm256_shuffle_epi8(tbl, AND(_mm256_srli_epi32(t, 4), nib)));
    return _mm256_madd_epi16(_mm256_maddubs_epi16(cnt, C32(0x01010101)), C32(0x00010001));
}
AI V ctz_pop(V o) { return popt(SUB(ISO(o), C32(1))); }
AI V ctz_pop2(V o) { return popt(ANDN(o, ADD(o, C32(-1)))); }
AI V ctz_db(V o) {
    const V tlo = TBL(0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8);
    const V thi = TBL(31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9);
    V idx = _mm256_srli_epi32(MULLO(ISO(o), C32(0x077CB531)), 27);
    return OR(_mm256_shuffle_epi8(tlo, _mm256_add_epi8(idx, C32(0x80808070))), _mm256_shuffle_epi8(thi, _mm256_sub_epi8(idx, C32(0x80808010))));
}
/* db3: i = top 5 bits (0..31, bytes 1..3 zero). K[i] = k. Tb[n] = K[16 + n], Td[n] = K[n] xor K[16 + n].
 * vpshufb(Tb, i) reads n = i & 15 (bit 7 is clear): K[16 + n]. i + 0x70 has bit 7 set iff i >= 16, so
 * vpshufb(Td, i + 0x70) is Td[n] for i < 16 and 0 otherwise; the xor is K[n] or K[16 + n].
 * Bytes 1..3: Tb[0] xor Td[0] = K[16] xor K[0] xor K[16] = K[0] = 0. */
AI V ctz_db3(V o) {
    const V tb = TBL(31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9);
    const V td = TBL(31, 26, 17, 21, 8, 29, 8, 4, 4, 26, 6, 9, 18, 20, 14, 1);
    V i = _mm256_srli_epi32(MULLO(ISO(o), C32(0x077CB531)), 27);
    return XOR(_mm256_shuffle_epi8(tb, i), _mm256_shuffle_epi8(td, ADD(i, C32(0x70707070))));
}
/* 16-bit de Bruijn 0x09AF: the top nibble of (0x09AF << l) mod 2^16 is distinct for l = 0..15, and 0 for l = 0 */
#define T16 TBL(0, 1, 2, 5, 3, 9, 6, 11, 15, 4, 8, 10, 14, 7, 13, 12)
#define HI16(x) _mm256_min_epu32(AND(x, C32(0xFFFF0000)), C32(16))      /* 16 [k >= 16] */
/* dbm: vpmaddwd(x, 0x09AF in both words) = 2^l 0x09AF exactly (for l = 15 the word is -2^15, and bits 12..15 of
 * -(0x09AF << 15) agree with (0x09AF << 15) mod 2^16). Bits 12..15 give l = k mod 16. */
AI V ctz_dbm(V o) {
    V x = ISO(o);
    V i = AND(_mm256_srli_epi32(_mm256_madd_epi16(x, C32(0x09AF09AF)), 12), C32(15));
    return OR(_mm256_shuffle_epi8(T16, i), HI16(x));
}
/* dbh: vpmullw in each word, vpsrlw 12: the index sits in byte 0 or byte 2 (the other word gives index 0, T16[0] = 0);
 * after the lookup, vpsrlvd by s = 16 [k >= 16] brings byte 2 down, and k = that OR s */
AI V ctz_dbh(V o) {
    V x = ISO(o), s = HI16(x);
    V t = _mm256_shuffle_epi8(T16, _mm256_srli_epi16(_mm256_mullo_epi16(x, C32(0x09AF09AF)), 12));
    return OR(_mm256_srlv_epi32(t, s), s);
}
/* mask5: x has one bit, so x & M_j is 0 or x >= 2^(2^j) >= 2^j; min_epu32 with 2^j gives k_j 2^j */
AI V ctz_mask5(V o) {
    V x = ISO(o);
    V a = OR(_mm256_min_epu32(AND(x, C32(0xAAAAAAAA)), C32(1)), _mm256_min_epu32(AND(x, C32(0xCCCCCCCC)), C32(2)));
    V b = OR(_mm256_min_epu32(AND(x, C32(0xF0F0F0F0)), C32(4)), _mm256_min_epu32(AND(x, C32(0xFF00FF00)), C32(8)));
    return OR(OR(a, b), HI16(x));
}
AI V ctz_mask5c(V o) {                               /* cmpeq gives -1 where bit j of k is 0 */
    V x = ISO(o), z = ZERO;
#define MC(M, W) AND(EQ(AND(x, C32(M)), z), C32(W))
    return ADD(ADD(ADD(MC(0xAAAAAAAA, -1), MC(0xCCCCCCCC, -2)), ADD(MC(0xF0F0F0F0, -4), MC(0xFF00FF00, -8))), ADD(MC(0xFFFF0000, -16), C32(31)));
#undef MC
}
/* mix: s = 8 (k >> 3) from two tests; y = x >> s is 2^p in byte 0; p from the low and high nibble tables */
AI V ctz_mix(V o) {
    V x = ISO(o);
    V s = OR(_mm256_min_epu32(AND(x, C32(0xFF00FF00)), C32(8)), HI16(x));
    V y = _mm256_srlv_epi32(x, s);
    const V tl = TBL(0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0), th = TBL(0, 4, 5, 0, 6, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0);
    return OR(s, OR(_mm256_shuffle_epi8(tl, y), _mm256_shuffle_epi8(th, _mm256_srli_epi32(y, 4))));
}
/* mixn: s = 4 (k >> 2) from three tests; y = x >> s is 2^r, r < 4, in bits 0..3; one table */
AI V ctz_mixn(V o) {
    V x = ISO(o);
    V s = OR(OR(_mm256_min_epu32(AND(x, C32(0xF0F0F0F0)), C32(4)), _mm256_min_epu32(AND(x, C32(0xFF00FF00)), C32(8))), HI16(x));
    const V tn = TBL(0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0);
    return OR(s, _mm256_shuffle_epi8(tn, _mm256_srlv_epi32(x, s)));
}

/* ======================= (1+i)-ary in lanes (study 24), ctz pluggable ======================= */
/* ======================= Study 27, milestone 5: mask tests fitted to the step ======================= */
/* the step's shift: hx, hy >> ctz(o). For the plain forms, one vpsravd per part by k. */
#define SHK(n) AI void sh_ctz_##n(V *hx, V *hy, V o) { V e = ctz_##n(o); *hx = _mm256_srav_epi32(*hx, e); *hy = _mm256_srav_epi32(*hy, e); }
#define FORMS(X) X(flt) X(bs) X(pop) X(pop2) X(db) X(db3) X(dbm) X(dbh) X(mask5) X(mask5c) X(mix) X(mixn)
FORMS(SHK)
#define SRAV2(e) { V e_ = (e); *hx = _mm256_srav_epi32(*hx, e_); *hy = _mm256_srav_epi32(*hy, e_); }
#define TST(M, W) _mm256_min_epu32(AND(x, C32(M)), C32(W))
/* mst5: the five min results used directly as shift counts, one stage each; k is never assembled */
AI V ctz_mst5(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_mst5(V *hx, V *hy, V o) {
    V x = ISO(o);
    SRAV2(TST(0xAAAAAAAA, 1)) SRAV2(TST(0xCCCCCCCC, 2)) SRAV2(TST(0xF0F0F0F0, 4)) SRAV2(TST(0xFF00FF00, 8)) SRAV2(TST(0xFFFF0000, 16))
}
/* mst2: two stages, the rare large part (16 | 8) first, then 4 | 2 | 1 */
AI V ctz_mst2(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_mst2(V *hx, V *hy, V o) {
    V x = ISO(o);
    SRAV2(OR(TST(0xFF00FF00, 8), TST(0xFFFF0000, 16))) SRAV2(OR(OR(TST(0xAAAAAAAA, 1), TST(0xCCCCCCCC, 2)), TST(0xF0F0F0F0, 4)))
}
/* mlo: the low tests (1, 2, 4) on the critical path, the high ones (8, 16) ORed last */
AI V ctz_mlo(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_mlo(V *hx, V *hy, V o) {
    V x = ISO(o);
    V hi = OR(TST(0xFF00FF00, 8), TST(0xFFFF0000, 16)), lo = OR(OR(TST(0xAAAAAAAA, 1), TST(0xCCCCCCCC, 2)), TST(0xF0F0F0F0, 4));
    SRAV2(OR(lo, hi))
}
/* nbr: k < 4 in every lane of the register (95% of register-steps) -> k = T[x] (x's low byte is 1, 2, 4 or 8,
 * bytes 1..3 zero); otherwise mask5. One vptest and a branch per register-step. */
#define T4 TBL(0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0)
AI V ctz_nbr(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_nbr(V *hx, V *hy, V o) {
    V x = ISO(o), e;
    if (__builtin_expect(_mm256_testz_si256(x, C32(0xFFFFFFF0)), 1)) e = _mm256_shuffle_epi8(T4, x);
    else e = OR(OR(OR(TST(0xAAAAAAAA, 1), TST(0xCCCCCCCC, 2)), OR(TST(0xF0F0F0F0, 4), TST(0xFF00FF00, 8))), TST(0xFFFF0000, 16));
    SRAV2(e)
}
/* bbr: k < 8 in every lane (99.99%) -> k from x's low byte by two nibble tables; otherwise mask5 */
AI V ctz_bbr(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_bbr(V *hx, V *hy, V o) {
    V x = ISO(o), e;
    const V tl = T4, th = TBL(0, 4, 5, 0, 6, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0);
    if (__builtin_expect(_mm256_testz_si256(x, C32(0xFFFFFF00)), 1)) e = OR(_mm256_shuffle_epi8(tl, x), _mm256_shuffle_epi8(th, _mm256_srli_epi32(x, 4)));
    else e = OR(OR(OR(TST(0xAAAAAAAA, 1), TST(0xCCCCCCCC, 2)), OR(TST(0xF0F0F0F0, 4), TST(0xFF00FF00, 8))), TST(0xFFFF0000, 16));
    SRAV2(e)
}
/* tbr: as nbr but tested on the thermometer t = andn(o, o - 1) = 2^k - 1 (low byte 0, 1, 3 or 7) */
AI V ctz_tbr(V o) { return ctz_mask5(o); }      /* setup only */
AI void sh_ctz_tbr(V *hx, V *hy, V o) {
    V t = ANDN(o, ADD(o, C32(-1))), e;
    if (__builtin_expect(_mm256_testz_si256(t, C32(0xFFFFFFF8)), 1)) e = _mm256_shuffle_epi8(TBL(0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0), t);
    else { V x = ISO(o); e = OR(OR(OR(TST(0xAAAAAAAA, 1), TST(0xCCCCCCCC, 2)), OR(TST(0xF0F0F0F0, 4), TST(0xFF00FF00, 8))), TST(0xFFFF0000, 16)); }
    SRAV2(e)
}
/* nbrd: nbr with dbh as the slow path */
AI V ctz_nbrd(V o) { return ctz_dbh(o); }      /* setup only */
AI void sh_ctz_nbrd(V *hx, V *hy, V o) {
    V x = ISO(o), e;
    if (__builtin_expect(_mm256_testz_si256(x, C32(0xFFFFFFF0)), 1)) e = _mm256_shuffle_epi8(T4, x); else e = ctz_dbh(o);
    SRAV2(e)
}
#define FITS(X) X(mst5) X(mst2) X(mlo) X(nbr) X(bbr) X(tbr) X(nbrd)

/* the strip: shift by e = ctz, then one division by 1+i where both parts are odd */
AI void stripodd(V *px, V *py) {
    V x = *px, y = *py;
    V m = _mm256_slli_epi32(AND(x, y), 31);
    V hx = SRA1(x), hy = SRA1(y);
    *px = SEL(x, SUB(hx, XOR(hy, C32(-1))), m);
    *py = SEL(y, SUB(hy, hx), m);
}
AI void stripv(V *px, V *py, V e) { *px = _mm256_srav_epi32(*px, e); *py = _mm256_srav_epi32(*py, e); stripodd(px, py); }
AI void primv(V *px, V *py) {
    V x = *px, y = *py;
    V m = _mm256_slli_epi32(x, 31);
    V nx = SEL(y, x, m), ny = SEL(SUB(_mm256_setzero_si256(), x), y, m);
    V t = OR(_mm256_slli_epi32(SUB(nx, ny), 30), C32(1));
    *px = _mm256_sign_epi32(nx, t); *py = _mm256_sign_epi32(ny, t);
}
AI V sizev(V x, V y) { return ADD(_mm256_abs_epi32(x), _mm256_abs_epi32(y)); }
/* the -2^31 guard: lanes with a -2^31 half become (0, 0), (0, 0) and are recorded in the sticky mask */
#define GUARD(P) { V mn_ = C32(IMIN); V g_ = OR(OR(EQ(ax##P, mn_), EQ(ay##P, mn_)), OR(EQ(bx##P, mn_), EQ(by##P, mn_))); \
    ax##P = ANDN(g_, ax##P); ay##P = ANDN(g_, ay##P); bx##P = ANDN(g_, bx##P); by##P = ANDN(g_, by##P); sticky = OR(sticky, g_); }
#define SETUPP(P, OFF, CTZ, G)                                                                              \
    V ax##P = LD(ar + i + OFF), ay##P = LD(ai + i + OFF), bx##P = LD(br + i + OFF), by##P = LD(bi + i + OFF);  \
    if (G) GUARD(P)                                                                                          \
    V oa##P = OR(ax##P, ay##P), ob##P = OR(bx##P, by##P), za##P = EQ(oa##P, zero), zb##P = EQ(ob##P, zero);  \
    V ea##P = CTZ(oa##P), eb##P = CTZ(ob##P);                                                                \
    V va##P = ADD(ADD(ea##P, ea##P), AND(AND(_mm256_srav_epi32(ax##P, ea##P), _mm256_srav_epi32(ay##P, ea##P)), C32(1)));  \
    V vb##P = ADD(ADD(eb##P, eb##P), AND(AND(_mm256_srav_epi32(bx##P, eb##P), _mm256_srav_epi32(by##P, eb##P)), C32(1)));  \
    V k##P = _mm256_min_epi32(SEL(va##P, C32(1000), za##P), SEL(vb##P, C32(1000), zb##P));                 \
    stripv(&ax##P, &ay##P, ea##P); primv(&ax##P, &ay##P); stripv(&bx##P, &by##P, eb##P); primv(&bx##P, &by##P); \
    { V tx = ax##P, ty = ay##P; ax##P = SEL(ax##P, bx##P, za##P); ay##P = SEL(ay##P, by##P, za##P);       \
      bx##P = SEL(bx##P, tx, zb##P); by##P = SEL(by##P, ty, zb##P); }                                        \
    V sa##P = sizev(ax##P, ay##P), sb##P = sizev(bx##P, by##P);                                             \
    { V mn = _mm256_min_epu32(sa##P, sb##P), ge = EQ(mn, sb##P);                                            \
      V tx = ax##P, ty = ay##P; ax##P = SEL(bx##P, ax##P, ge); ay##P = SEL(by##P, ay##P, ge);               \
      bx##P = SEL(tx, bx##P, ge); by##P = SEL(ty, by##P, ge); sb##P = mn; }
#define STEPP(P, CTZ) {                                                                                     \
    V hx = SUB(SRA1(ax##P), SRA1(bx##P)), hy = SUB(SRA1(ay##P), SRA1(by##P));                              \
    V o = OR(hx, hy), z = EQ(o, zero);                                                                       \
    sh_##CTZ(&hx, &hy, o); stripodd(&hx, &hy); primv(&hx, &hy);                                                               \
    hx = SEL(hx, bx##P, z); hy = SEL(hy, by##P, z);                                                          \
    V s = sizev(hx, hy), mn = _mm256_min_epu32(s, sb##P), ge = EQ(mn, sb##P);                               \
    V nbx = SEL(hx, bx##P, ge), nby = SEL(hy, by##P, ge);                                                    \
    ax##P = SEL(bx##P, hx, ge); ay##P = SEL(by##P, hy, ge); bx##P = nbx; by##P = nby; sb##P = mn; }
/* normalise (study 6's rule) and store */
AI void normstore(V gx, V gy, i32 *pr, i32 *pi) {
    const V zero = _mm256_setzero_si256();
    V nm = OR(_mm256_cmpgt_epi32(zero, gy), AND(EQ(gy, zero), _mm256_cmpgt_epi32(zero, gx)));
    gx = SUB(XOR(gx, nm), nm); gy = SUB(XOR(gy, nm), nm);
    V rm = AND(_mm256_cmpgt_epi32(C32(1), gx), _mm256_cmpgt_epi32(gy, zero));
    V rx = SEL(gx, gy, rm); gy = SEL(gy, SUB(zero, gx), rm);
    ST(pr, rx); ST(pi, gy);
}
#define FINISHP(P, OFF) {                                                                                   \
    V s2 = _mm256_srai_epi32(k##P, 1);                                                                       \
    V gx = _mm256_sllv_epi32(ax##P, s2), gy = _mm256_sllv_epi32(ay##P, s2);                                  \
    V od = _mm256_slli_epi32(k##P, 31);                                                                      \
    V tx = SUB(gx, gy), ty = ADD(gx, gy);                                                                    \
    gx = SEL(gx, tx, od); gy = SEL(gy, ty, od);                                                              \
    normstore(gx, gy, gr + i + OFF, gi + i + OFF); }
#define DIFF(P) OR(XOR(ax##P, bx##P), XOR(ay##P, by##P))
#define PI_V(name, CTZ, G)                                                                                  \
NOINL u32 name(ARGS) {                                                                                       \
    const V zero = _mm256_setzero_si256(); V sticky = zero;                                                  \
    for (size_t i = 0; i < N; i += 8) {                                                                      \
        SETUPP(0, 0, CTZ, G)                                                                                 \
        while (!_mm256_testz_si256(DIFF(0), DIFF(0))) STEPP(0, CTZ)                                          \
        FINISHP(0, 0)                                                                                        \
    }                                                                                                        \
    return _mm256_testz_si256(sticky, sticky) ? 0 : fixup(PASSG);                                            \
}
#define PI_V3(name, CTZ, G)                                                                                 \
NOINL u32 name(ARGS) {                                                                                       \
    const V zero = _mm256_setzero_si256(); V sticky = zero;                                                  \
    size_t i = 0;                                                                                            \
    for (; i + 24 <= N; i += 24) {                                                                           \
        SETUPP(0, 0, CTZ, G) SETUPP(1, 8, CTZ, G) SETUPP(2, 16, CTZ, G)                                      \
        while (1) { V d = OR(OR(DIFF(0), DIFF(1)), DIFF(2)); if (_mm256_testz_si256(d, d)) break;            \
                    STEPP(0, CTZ) STEPP(1, CTZ) STEPP(2, CTZ) }                                              \
        FINISHP(0, 0) FINISHP(1, 8) FINISHP(2, 16)                                                           \
    }                                                                                                        \
    for (; i < N; i += 8) { SETUPP(0, 0, CTZ, G) while (!_mm256_testz_si256(DIFF(0), DIFF(0))) STEPP(0, CTZ) FINISHP(0, 0) } \
    return _mm256_testz_si256(sticky, sticky) ? 0 : fixup(PASSG);                                            \
}
/* ======================= reference: Euclid in __int128 (study 6), normalised ======================= */
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

/* ======================= inputs ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i64 rr(i64 lim) { return (i64)(next() % (u64)(2 * lim + 1)) - lim; }
static const i32 EVM[] = {IMIN, IMIN + 1, 0, 1, -1, 2, -2, 3, HMAX, HMAX - 1, 1 << 30, -(1 << 30), (1 << 30) + 1,
                          -(1 << 30) - 1, 1 << 16, -(1 << 16), 46341, -46341, IMIN + 2, -3};
#define NEVM (int)(sizeof EVM / sizeof EVM[0])
/* kinds 0..3 as study 24 (random, small b, common factor 2^4..2^15, large common factor 2^15);
 * 4: -2^31 halves planted (each half -2^31 with probability 1/4, else a random multiple of a power of 2) */
static void gen(int kind, i32 *a1, i32 *a2, i32 *b1, i32 *b2) {
    if (kind == 0) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(HMAX); *b2 = rr(HMAX); return; }
    if (kind == 1) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(1 << 12); *b2 = rr(1 << 12); return; }
    if (kind == 4) {
        i32 *p[4] = {a1, a2, b1, b2};
        for (int j = 0; j < 4; j++) { int s = (int)(next() % 32); *p[j] = (next() & 3) == 0 ? IMIN : (i32)((u32)rr(HMAX) << (s & 31) >> 0) ; if (next() & 1) *p[j] &= ~((1u << (next() % 31)) - 1); }
        return;
    }
    int e = kind == 2 ? 4 + (int)(next() % 12) : 15;
    i64 g1 = rr(1LL << e), g2 = rr(1LL << e), lim = HMAX / (2LL << e);
    i64 u1 = rr(lim), u2 = rr(lim), v1 = rr(lim), v2 = rr(lim);
    *a1 = (i32)(g1 * u1 - g2 * u2); *a2 = (i32)(g1 * u2 + g2 * u1);
    *b1 = (i32)(g1 * v1 - g2 * v2); *b2 = (i32)(g1 * v2 + g2 * v1);
}
#define MK3(n) PI_V3(pi_v3_##n, ctz_##n, 1)
#define MK1(n) PI_V(pi_v_##n, ctz_##n, 1)
FORMS(MK3)
MK1(flt) MK1(pop) MK1(db) MK1(db3) MK1(dbm) MK1(dbh) MK1(mask5) MK1(mixn)
FITS(MK3) FITS(MK1)

/* ======================= kernel table ======================= */
typedef u32 (*kf)(ARGS);
typedef struct { const char *name; kf f; } kern;
#define ENT3(n) {"pi_v3_" #n, pi_v3_##n},
#define ENT1(n) {"pi_v_" #n, pi_v_##n},
static kern KT[] = { FORMS(ENT3)
    {"pi_v_flt", pi_v_flt}, {"pi_v_pop", pi_v_pop}, {"pi_v_db", pi_v_db}, {"pi_v_db3", pi_v_db3}, {"pi_v_dbm", pi_v_dbm},
    {"pi_v_dbh", pi_v_dbh}, {"pi_v_mask5", pi_v_mask5}, {"pi_v_mixn", pi_v_mixn},
    FITS(ENT3) FITS(ENT1) };
#define NKT (int)(sizeof KT / sizeof KT[0])

/* ======================= correctness ======================= */
static long fails, nov, nsticky;
static int divides(i64 g1, i64 g2, i64 a1, i64 a2) {
    i128 n = (i128)g1 * g1 + (i128)g2 * g2;
    if (n == 0) return a1 == 0 && a2 == 0;
    i128 x = (i128)a1 * g1 + (i128)a2 * g2, y = (i128)a2 * g1 - (i128)a1 * g2;
    return x % n == 0 && y % n == 0;
}
#define BM (1 << 16)
static i32 A1[BM + 32], A2[BM + 32], B1[BM + 32], B2[BM + 32], OUT[NKT][2][BM + 32];
/* run every kernel on A..B (n pairs, padded with (0, 0) to a multiple of 24); compare with the reference;
 * the flag must equal "some gcd in the batch does not fit int32" */
static void check_batch(size_t n, int divcheck, int K) {
    size_t Np = (n + 23) / 24 * 24;
    for (size_t j = n; j < Np; j++) A1[j] = A2[j] = B1[j] = B2[j] = 0;
    u32 fl[NKT];
    for (int k = 0; k < NKT; k++) fl[k] = KT[k].f(A1, A2, B1, B2, OUT[k][0], OUT[k][1], Np);
    int anyov = 0, anymin = 0;
    for (size_t j = 0; j < n; j++) {
        i64 g1, g2; ref_gcd(A1[j], A2[j], B1[j], B2[j], &g1, &g2);
        int ov = g1 > HMAX; anyov |= ov;
        int hm = A1[j] == IMIN || A2[j] == IMIN || B1[j] == IMIN || B2[j] == IMIN; anymin |= hm;
        for (int k = 0; k < NKT; k++) {
            
            if (OUT[k][0][j] != (i32)(u32)g1 || OUT[k][1][j] != (i32)(u32)g2)
                if (fails++ < 10) printf("FAIL %s (%d,%d) (%d,%d): got (%d,%d) want (%lld,%lld)\n", KT[k].name, A1[j], A2[j], B1[j], B2[j],
                                         OUT[k][0][j], OUT[k][1][j], (long long)g1, (long long)g2);
        }
        if (divcheck && (!(g1 > 0 ? g2 >= 0 : (g1 == 0 && g2 == 0)) || !divides(g1, g2, A1[j], A2[j]) || !divides(g1, g2, B1[j], B2[j])))
            if (fails++ < 10) printf("FAIL reference (%d,%d) (%d,%d)\n", A1[j], A2[j], B1[j], B2[j]);
        if (divcheck == 2) {   /* brute force at K <= 4: N(g) is the largest norm of a common divisor */
            int H = 1 << (K - 1); i64 best = 0;
            for (int d1 = -2 * H; d1 <= 2 * H; d1++) for (int d2 = -2 * H; d2 <= 2 * H; d2++)
                if ((d1 | d2) && divides(d1, d2, A1[j], A2[j]) && divides(d1, d2, B1[j], B2[j]) && d1 * d1 + d2 * d2 > best) best = d1 * d1 + d2 * d2;
            if (!(A1[j] | A2[j] | B1[j] | B2[j])) best = 0;
            if (best != g1 * g1 + g2 * g2) if (fails++ < 10) printf("FAIL brute (%d,%d) (%d,%d)\n", A1[j], A2[j], B1[j], B2[j]);
        }
    }
    nov += anyov; nsticky += anymin;
    for (int k = 0; k < NKT; k++) if (fl[k] != (u32)anyov)
        if (fails++ < 10) printf("FAIL %s flag %u want %d (%d,%d) (%d,%d)\n", KT[k].name, fl[k], anyov, A1[0], A2[0], B1[0], B2[0]);
}
/* every ctz form: all 32 single-bit inputs, then every nonzero 32-bit value against __builtin_ctz */
typedef V (*cf)(V);
#define CW(n) static V w_##n(V o) { return ctz_##n(o); }
FORMS(CW)
#define CE(n) {#n, w_##n},
static struct { const char *name; cf f; } CT[] = { FORMS(CE) };
#define NCT (int)(sizeof CT / sizeof CT[0])
static void do_ctz_check(void) {
    for (int m = 0; m < NCT; m++) {
        int bad1 = 0; long bad = 0, bad30 = 0;
        for (int k = 0; k < 32; k += 8) {
            u32 t[8]; ST(t, CT[m].f(_mm256_sllv_epi32(C32(1), _mm256_setr_epi32(k, k + 1, k + 2, k + 3, k + 4, k + 5, k + 6, k + 7))));
            for (int j = 0; j < 8; j++) bad1 += (int)t[j] != k + j;
        }
        for (u64 base = 0; base < (1ull << 32); base += 8) {
            V o = _mm256_add_epi32(C32((u32)base), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
            u32 t[8]; ST(t, CT[m].f(o));
            for (int j = 0; j < 8; j++) { u32 v = (u32)(base + j); if (!v) continue; int w = __builtin_ctz(v); if ((int)t[j] != w) { bad++; bad30 += w <= 30; } }
        }
        printf("ctz_%-7s single-bit inputs wrong %d of 32; all nonzero 32-bit values wrong %ld (with ctz <= 30: %ld)\n", CT[m].name, bad1, bad, bad30);
    }
}
/* fitted shifts: hx, hy >> ctz(o) for hx = o, hy = -o: all 32 single-bit o, then every nonzero o
 * (lanes base..base+7, and a second pass with lane 0 forced to a single bit so the branches mix) */
typedef void (*sf)(V *, V *, V);
#define SW(n) static void s_##n(V *a, V *b, V o) { sh_ctz_##n(a, b, o); }
FITS(SW)
#define SE(n) {#n, s_##n},
static struct { const char *name; sf f; } ST_[] = { FITS(SE) };
static void do_fit_check(void) {
    for (int m = 0; m < (int)(sizeof ST_ / sizeof ST_[0]); m++) {
        int bad1 = 0; long bad = 0;
        for (int k = 0; k < 32; k++) for (int j = 0; j < 8; j++) {     /* single bits, one lane at a time among zeros */
            u32 o[8] = {0}; o[j] = 1u << k; V ov = LD(o), a = ov, b = SUB(ZERO, ov); ST_[m].f(&a, &b, ov);
            u32 ra[8], rb[8]; ST(ra, a); ST(rb, b); bad1 += ra[j] != (u32)((i32)o[j] >> k) || rb[j] != (u32)((i32)-o[j] >> k);
        }
        for (int pass = 0; pass < 2; pass++)
        for (u64 base = 0; base < (1ull << 32); base += 8) {
            V ov = _mm256_add_epi32(C32((u32)base), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
            if (pass) ov = _mm256_blend_epi32(ov, C32(1u << (base >> 3 & 31)), 1);
            V a = ov, b = SUB(ZERO, ov); ST_[m].f(&a, &b, ov);
            u32 o[8], ra[8], rb[8]; ST(o, ov); ST(ra, a); ST(rb, b);
            for (int j = 0; j < 8; j++) { if (!o[j]) continue; int k = __builtin_ctz(o[j]);
                bad += ra[j] != (u32)((i32)o[j] >> k) || rb[j] != (u32)((i32)-o[j] >> k); }
        }
        printf("shift %-5s single-bit inputs wrong %d of 256 lane placements; every nonzero o (two passes) wrong %ld\n", ST_[m].name, bad1, bad);
    }
}
static void do_check(void) {
    do_ctz_check(); do_fit_check(); fflush(stdout);
    for (int K = 3; K <= 6; K++) {
        i64 lo = -(1 << (K - 1)), hi = (1 << (K - 1)) - 1; long cnt = 0; size_t n = 0;
        for (i64 p = lo; p <= hi; p++) for (i64 q = lo; q <= hi; q++) for (i64 r = lo; r <= hi; r++) for (i64 s = lo; s <= hi; s++) {
            A1[n] = (i32)p; A2[n] = (i32)q; B1[n] = (i32)r; B2[n] = (i32)s; n++; cnt++;
            if (n == BM) { check_batch(n, K <= 4 ? 2 : 0, K); n = 0; }
        }
        if (n) check_batch(n, K <= 4 ? 2 : 0, K);
        printf("K = %d: exhaustive, %ld pairs; failures so far %ld\n", K, cnt, fails); fflush(stdout);
    }
    long cnt = 0;
    for (int p = 0; p < NEVM; p++) for (int q = 0; q < NEVM; q++) for (int r = 0; r < NEVM; r++) for (int s = 0; s < NEVM; s++) {
        A1[0] = EVM[p]; A2[0] = EVM[q]; B1[0] = EVM[r]; B2[0] = EVM[s]; check_batch(1, 1, 32); cnt++;
    }
    printf("edges incl. -2^31: %ld pairs each alone; failures so far %ld\n", cnt, fails);
    long tot = 0;
    for (int it = 0; it < 40; it++) {
        for (int j = 0; j < BM; j++) gen(j % 5 == 4 ? ((next() & 63) ? 0 : 4) : j % 5, A1 + j, A2 + j, B1 + j, B2 + j);
        check_batch(BM, 1, 32); tot += BM;
    }
    printf("random: %ld pairs (random, small b, common factor 2^4..2^15, factor 2^15, a few -2^31)\nTOTAL failures %ld\n", tot, fails);
}

/* ======================= timing (integer clock) ======================= */
static u64 now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec; }
static int cmpu(const void *a, const void *b) { u64 x = *(const u64 *)a, y = *(const u64 *)b; return (x > y) - (x < y); }
#define REPS 51
static void pps(u64 ps) { printf("%4llu.%02llu", (unsigned long long)(ps / 1000), (unsigned long long)(ps % 1000 / 10)); }
static void do_time(void) {
    enum { N = 1032 };
    static i32 buf[6][N + 256];
    i32 *p[6]; for (int j = 0; j < 6; j++) p[j] = buf[j] + 16 * j;
    static const int kinds[2] = {0, 3};
    static const char *kn[2] = {"random", "large common factor 2^15"};
    printf("ns per gcd pair, N = %d pairs; median [q1, q3] over %d reps; kernels interleaved, order rotated\n", N, REPS);
    for (int c = 0; c < 2; c++) {
        for (int j = 0; j < N; j++) gen(kinds[c], p[0] + j, p[1] + j, p[2] + j, p[3] + j);
        static u64 t[NKT][REPS];
        for (int k = 0; k < NKT; k++) KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NKT; j++) {
                int k = (j + r) % NKT;
                KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                u64 t0 = now_ns();
                for (int it = 0; it < 32; it++) KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                t[k][r] = (now_ns() - t0) * 1000 / ((u64)N * 32);
            }
        printf("\n%s\n", kn[c]);
        for (int k = 0; k < NKT; k++) {
            qsort(t[k], REPS, 8, cmpu);
            printf("  %-14s ", KT[k].name); pps(t[k][REPS / 2]); printf(" ["); pps(t[k][REPS / 4]); printf(", "); pps(t[k][3 * REPS / 4]); printf("]\n");
        }
        fflush(stdout);
    }
}
/* ctz alone (inlined): throughput over an array, latency of a dependent chain v <- (v + ctz(v)) | 2^30 */
static u32 ca[4096];
#define CTIME(n) static void ct_##n(u64 *tt, u64 *tl) {                                                        \
    V acc = ZERO; u64 t0 = now_ns();                                                                         \
    for (int it = 0; it < 64; it++) for (int j = 0; j < 4096; j += 8) acc = ADD(acc, ctz_##n(LD(ca + j)));  \
    *tt = (now_ns() - t0) * 1000 / (64 * 4096 / 8);                                                          \
    V v = OR(LD(ca), C32(1u << 30)); t0 = now_ns();                                                          \
    for (int it = 0; it < 100000; it++) v = OR(ADD(v, ctz_##n(v)), C32(1u << 30));                          \
    *tl = (now_ns() - t0) * 1000 / 100000;                                                                   \
    u32 sink[8]; ST(sink, OR(acc, v)); if (sink[0] == 12345) printf(" "); }
FORMS(CTIME)
#define CTE(n) ct_##n,
static void (*CTF[])(u64 *, u64 *) = { FORMS(CTE) };
static void do_ctz_time(void) {
    for (int j = 0; j < 4096; j++) ca[j] = (u32)next() | 1u << (next() % 31);
    static u64 tt[NCT][REPS], tl[NCT][REPS];
    for (int r = 0; r < REPS; r++) for (int j = 0; j < NCT; j++) { int m = (j + r) % NCT; CTF[m](&tt[m][r], &tl[m][r]); }
    for (int m = 0; m < NCT; m++) {
        qsort(tt[m], REPS, 8, cmpu); qsort(tl[m], REPS, 8, cmpu);
        printf("ctz_%-7s per register of 8: throughput ", CT[m].name); pps(tt[m][REPS / 2]); printf(" [");  pps(tt[m][REPS / 4]); printf(","); pps(tt[m][3 * REPS / 4]);
        printf("] ns, chain latency (with add, or) "); pps(tl[m][REPS / 2]); printf(" ["); pps(tl[m][REPS / 4]); printf(","); pps(tl[m][3 * REPS / 4]); printf("] ns\n");
    }
}
static void do_icount(int kind) {
    enum { N = 1032 }; static i32 b[6][N];
    for (int j = 0; j < N; j++) gen(kind, b[0] + j, b[1] + j, b[2] + j, b[3] + j);
    for (int k = 0; k < NKT; k++) KT[k].f(b[0], b[1], b[2], b[3], b[4], b[5], N);
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else if (!strcmp(mode, "ctz")) do_ctz_time();
    else if (!strcmp(mode, "icount")) do_icount(argc > 2 ? atoi(argv[2]) : 0);
    else do_time();
    return 0;
}
