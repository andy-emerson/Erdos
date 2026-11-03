/* Study 26: a fair gcd comparison, integer only (gcdfair.c)
 *
 * Arrays of independent pairs (a, b), int32 halves in four separate arrays, the full two's complement range
 * (decision 12 allows -2^31). Output: gcd(a, b) normalised to re > 0, im >= 0 (gcd(0, 0) = 0), and a flag.
 *   euclid     study 6: Euclid by rounded division in 64-bit scalar registers (two idiv per step), ties up
 *   euclid_v   Euclid in AVX2 lanes, 8 pairs per register; division by study 25's m3 (non-restoring, width per
 *              register) with rule A (nearest, ties up: the same steps as euclid)
 *   euclid_v5  the same with study 25's m5x division (per-lane reciprocal, two registers interleaved)
 *   pi_sb      study 24's (1+i)-ary gcd, scalar, branch-free step (only the loop branches)
 *   pi_v3_*    study 24's best vector version (3 registers stepped together), with an integer-only lane ctz:
 *              _bs  binary search with compares and variable shifts
 *              _pop popcount of (x & -x) - 1 with the vpshufb nibble table
 *              _db  de Bruijn: (x & -x) * 0x077CB531, top 5 bits index a 32-entry table (two vpshufb)
 *   pi_v_*     the same with one register (latency-bound, so the ctz latency shows more)
 *   pi_v3_pop_ng   pi_v3_pop without the -2^31 guard (to time the guard; wrong for -2^31 inputs)
 * The -2^31 gap. Making a number primary (or normalising a gcd) negates it, and -2^31 has no negation in int32.
 * Every kernel detects pairs with a -2^31 half (4 compares per register in lanes), replaces them by (0, 0) so the
 * lanes stay inside the proven range, and ORs the mask into a sticky flag. Once per array, if the flag is set,
 * fixup() recomputes exactly those pairs by the (1+i)-ary gcd in 64-bit scalar registers. The result can be
 * 2^31 (gcd(-2^31, 0) = 2^31), which does not fit: then bit 0 of the return value is set (overflow: promote).
 * The vector kernels need N to be a multiple of 8.
 *
 * Build: gcc -O2 -mavx2 -o gcdfair gcdfair.c
 * Run:   ./gcdfair check | ./gcdfair ctz | ./gcdfair time | ./gcdfair stats
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

/* ======================= scalar Euclid (study 6) ======================= */
static inline i64 rdiv(i64 x, i64 n) {          /* nearest, halves up; n > 0 */
    i64 q = x / n, r = x % n;
    i64 m = r >> 63; q += m; r += n & m;         /* floor */
    return q + (r >= n - r);
}
static long euclid_steps;
static inline void euclid1(i64 a1, i64 a2, i64 b1, i64 b2, i32 *g1, i32 *g2, int count) {
    while (b1 | b2) {
        i64 n = b1 * b1 + b2 * b2, x = a1 * b1 + a2 * b2, y = a2 * b1 - a1 * b2;
        i64 q1 = rdiv(x, n), q2 = rdiv(y, n);
        i64 r1 = a1 - (q1 * b1 - q2 * b2), r2 = a2 - (q1 * b2 + q2 * b1);
        a1 = b1; a2 = b2; b1 = r1; b2 = r2;
        if (count) euclid_steps++;
    }
    normalise(&a1, &a2);
    *g1 = (i32)a1; *g2 = (i32)a2;
}
NOINL u32 euclid(ARGS) {
    int st = 0;
    for (size_t i = 0; i < N; i++) { if (HASMIN(i)) { st = 1; continue; } euclid1(ar[i], ai[i], br[i], bi[i], gr + i, gi + i, 0); }
    return st ? fixup(PASSG) : 0;
}

/* ======================= scalar (1+i)-ary, branch-free step (study 24's pi_sb) ======================= */
static inline void pib1(i32 a1, i32 a2, i32 b1, i32 b2, i32 *g1, i32 *g2) {
    i32 ax = a1, ay = a2, bx = b1, by = b2;
    int za = !(ax | ay), zb = !(bx | by);
    int ea = ctz64((u64)(u32)(ax | ay)) & 31, eb = ctz64((u64)(u32)(bx | by)) & 31;
    int va = za ? 1000 : 2 * ea + (int)((ax >> ea) & (ay >> ea) & 1);
    int vb = zb ? 1000 : 2 * eb + (int)((bx >> eb) & (by >> eb) & 1);
    int k = va < vb ? va : vb;
    STRIP(ax, ay) PRIM(ax, ay) STRIP(bx, by) PRIM(bx, by)
    if (za) { ax = bx; ay = by; }
    if (zb) { bx = ax; by = ay; }
    u32 sa = (u32)abs(ax) + (u32)abs(ay), sb = (u32)abs(bx) + (u32)abs(by);
    if (sa < sb) { i32 t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; sb = sa; }
    while ((ax ^ bx) | (ay ^ by)) {
        i32 x = (ax >> 1) - (bx >> 1), y = (ay >> 1) - (by >> 1);
        i32 zm = -(i32)((x | y) == 0);
        int e = __builtin_ctz((u32)(x | y) | 0x80000000u);
        x >>= e; y >>= e;
        i32 m = -(x & y & 1), px = x >> 1, py = y >> 1;
        x ^= (x ^ (px + py + 1)) & m; y ^= (y ^ (py - px)) & m;
        m = (x & 1) - 1;
        i32 nx = x ^ ((x ^ y) & m), ny = y ^ ((y ^ -x) & m);
        m = -((nx - ny) >> 1 & 1);
        x = (nx ^ m) - m; y = (ny ^ m) - m;
        x ^= (x ^ bx) & zm; y ^= (y ^ by) & zm;
        u32 s = (u32)abs(x) + (u32)abs(y);
        i32 gm = -(i32)(s >= sb);
        i32 nax = bx ^ ((bx ^ x) & gm), nay = by ^ ((by ^ y) & gm);
        bx = x ^ ((x ^ bx) & gm); by = y ^ ((y ^ by) & gm);
        ax = nax; ay = nay; sb = s < sb ? s : sb;
    }
    i32 gx = k > 64 ? 0 : ax << (k >> 1), gy = k > 64 ? 0 : ay << (k >> 1);
    if (k & 1) { i32 t = gx - gy; gy = gx + gy; gx = t; }
    i64 nx = gx, ny = gy; normalise(&nx, &ny);
    *g1 = (i32)nx; *g2 = (i32)ny;
}
NOINL u32 pi_sb(ARGS) {
    int st = 0;
    for (size_t i = 0; i < N; i++) { if (HASMIN(i)) { st = 1; continue; } pib1(ar[i], ai[i], br[i], bi[i], gr + i, gi + i); }
    return st ? fixup(PASSG) : 0;
}

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

/* ---------- lane ctz, integer only (x = 0 gives some value; the callers do not use it) ---------- */
AI V ctz_bs(V o) {                                   /* (a) binary search: low 16, 8, 4, 2 bits zero? shift them out */
    const V z = _mm256_setzero_si256();
    V k = AND(EQ(AND(o, C32(0xFFFF)), z), C32(16)); V n = k; o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0xFF)), z), C32(8)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0xF)), z), C32(4)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    k = AND(EQ(AND(o, C32(0x3)), z), C32(2)); n = ADD(n, k); o = _mm256_srlv_epi32(o, k);
    return ADD(n, ANDN(o, C32(1)));
}
AI V ctz_pop(V o) {                                  /* (b) popcount of (x & -x) - 1, nibble table */
    const V nib = C32(0x0F0F0F0F);
    const V tbl = _mm256_setr_epi8(0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4, 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    V t = SUB(AND(o, SUB(_mm256_setzero_si256(), o)), C32(1));
    V cnt = _mm256_add_epi8(_mm256_shuffle_epi8(tbl, AND(t, nib)), _mm256_shuffle_epi8(tbl, AND(_mm256_srli_epi32(t, 4), nib)));
    return _mm256_madd_epi16(_mm256_maddubs_epi16(cnt, C32(0x01010101)), C32(0x00010001));
}
/* (c) de Bruijn: index = ((x & -x) * 0x077CB531) >> 27 in byte 0 of each dword; table halves by vpshufb, where
 * an index byte with bit 7 set gives 0: byte 0 + 0x70 selects the low half (idx < 16), byte 0 - 0x10 the high half,
 * and the other three bytes get 0x80 so they read 0 */
static const i32 DB_T[32] = {0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
                             31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9};
AI V ctz_db(V o) {
    const V tlo = _mm256_setr_epi8(0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8);
    const V thi = _mm256_setr_epi8(31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9, 31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9);
    V idx = _mm256_srli_epi32(MULLO(AND(o, SUB(_mm256_setzero_si256(), o)), C32(0x077CB531)), 27);
    V lo = _mm256_shuffle_epi8(tlo, _mm256_add_epi8(idx, C32(0x80808070)));
    V hi = _mm256_shuffle_epi8(thi, _mm256_sub_epi8(idx, C32(0x80808010)));
    return OR(lo, hi);
}

/* ======================= (1+i)-ary in lanes (study 24), ctz pluggable ======================= */
AI void stripv(V *px, V *py, V e) {
    V x = _mm256_srav_epi32(*px, e), y = _mm256_srav_epi32(*py, e);
    V m = _mm256_slli_epi32(AND(x, y), 31);
    V hx = SRA1(x), hy = SRA1(y);
    *px = SEL(x, SUB(hx, XOR(hy, C32(-1))), m);
    *py = SEL(y, SUB(hy, hx), m);
}
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
    stripv(&hx, &hy, CTZ(o)); primv(&hx, &hy);                                                               \
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
PI_V(pi_v_bs, ctz_bs, 1) PI_V(pi_v_pop, ctz_pop, 1) PI_V(pi_v_db, ctz_db, 1)
PI_V3(pi_v3_bs, ctz_bs, 1) PI_V3(pi_v3_pop, ctz_pop, 1) PI_V3(pi_v3_db, ctz_db, 1) PI_V3(pi_v3_pop_ng, ctz_pop, 0)

/* ======================= Euclid in lanes ======================= */
/* study 25's front end: x = (ar, ai), y = (br, bi) -> U (magnitudes in one's complement), sign masks S, N (0 -> 1) */
AI void front(V a, V b, V c, V d, V N[2], V U[4], V S[4]) {
    const V zero = _mm256_setzero_si256(), one = C64(1), m1 = C64(-1);
    V ao = HI32(a), bo = HI32(b), co = HI32(c), dO = HI32(d);
    V X0 = ADD64(MULS(a, c), MULS(b, d)), X1 = ADD64(MULS(ao, co), MULS(bo, dO));
    V Y0 = SUB64(MULS(b, c), MULS(a, d)), Y1 = SUB64(MULS(bo, co), MULS(ao, dO));
    V N0 = ADD64(MULS(c, c), MULS(d, d)), N1 = ADD64(MULS(co, co), MULS(dO, dO));
    N[0] = OR(N0, AND(_mm256_cmpeq_epi64(N0, zero), one)); N[1] = OR(N1, AND(_mm256_cmpeq_epi64(N1, zero), one));
    S[0] = GT64(m1, SUB64(X0, one)); S[1] = GT64(m1, SUB64(X1, one));
    S[2] = GT64(zero, Y0); S[3] = GT64(zero, Y1);
    U[0] = XOR(X0, S[0]); U[1] = XOR(X1, S[1]); U[2] = XOR(Y0, S[2]); U[3] = XOR(Y1, S[3]);
}
/* rule A from the floor parts: q = floor q + [rho > (N - 1) >> 1], packed */
AI void near_a(V N[2], V Q[4], V R[4], V S[4], V *qre, V *qim) {
    V M[4];
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
        V Nj = N[j & 1];
        M[j] = GT64(ADD64(XOR(R[j], S[j]), AND(Nj, S[j])), _mm256_srli_epi64(SUB64(Nj, C64(1)), 1));
    }
    *qre = SUB(XOR(PACKQ(Q[0], Q[1]), PACK(S[0], S[1])), PACK(M[0], M[1]));
    *qim = SUB(XOR(PACKQ(Q[2], Q[3]), PACK(S[2], S[3])), PACK(M[2], M[3]));
}
/* study 25's m3 core: non-restoring division, width per register */
static const int wseq[9] = {1, 2, 3, 4, 6, 8, 12, 16, 24};
#define NR_STEP(R, L, nN) { V m_ = GT64(zero, R); V b_ = _mm256_srli_epi64(L, 63);                        \
    L = SUB64(ADD64(L, L), m_); R = ADD64(ADD64(R, R), ADD64(SUB64(b_, m_), XOR(nN, m_))); }
AI void m3_core(V U[4], V N[2], V Q[4], V R[4]) {
    const V zero = _mm256_setzero_si256();
    V Nm0 = SUB64(N[0], C64(1)), Nm1 = SUB64(N[1], C64(1));
    int W = 32;
    for (int k = 0; k < 9; k++) {
        __m128i cw = _mm_cvtsi32_si128(wseq[k]);
        V f = OR(OR(GT64(_mm256_srl_epi64(U[0], cw), Nm0), GT64(_mm256_srl_epi64(U[1], cw), Nm1)),
                 OR(GT64(_mm256_srl_epi64(U[2], cw), Nm0), GT64(_mm256_srl_epi64(U[3], cw), Nm1)));
        if (_mm256_testz_si256(f, f)) { W = wseq[k]; break; }
    }
    __m128i cw = _mm_cvtsi32_si128(W), cl = _mm_cvtsi32_si128(64 - W);
    V nN0 = SUB64(zero, N[0]), nN1 = SUB64(zero, N[1]);
    V R0 = _mm256_srl_epi64(U[0], cw), R1 = _mm256_srl_epi64(U[1], cw), R2 = _mm256_srl_epi64(U[2], cw), R3 = _mm256_srl_epi64(U[3], cw);
    V L0 = _mm256_sll_epi64(U[0], cl), L1 = _mm256_sll_epi64(U[1], cl), L2 = _mm256_sll_epi64(U[2], cl), L3 = _mm256_sll_epi64(U[3], cl);
    for (int j = 0; j < W; j++) { NR_STEP(R0, L0, nN0) NR_STEP(R1, L1, nN1) NR_STEP(R2, L2, nN0) NR_STEP(R3, L3, nN1) }
    V mask = C64((1ull << W) - 1);
    V Rr[4] = {R0, R1, R2, R3}, Lr[4] = {L0, L1, L2, L3};
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
        V m = GT64(zero, Rr[j]);
        Q[j] = ANDN(SUB64(ADD64(Lr[j], Lr[j]), m), mask);
        R[j] = ADD64(Rr[j], AND(N[j & 1], m));
    }
}
/* study 25's m5 pieces: per-lane reciprocal (exhaustively checked there), estimate, one correction */
AI V recip(V dh) {
    const V zero = _mm256_setzero_si256();
    V Y = SUB64(SUB64(C64(5726623061ull), HI32(ADD64(MULU(dh, C64(3817748708ull)), C64(3817748708ull)))), C64(1));
    _Pragma("GCC unroll 9") for (int it = 0; it < 3; it++) {
        V e = SUB64(C64(1ull << 63), ADD64(MULU(dh, Y), Y));
        Y = ADD64(Y, HI32(MULU(Y, _mm256_srli_epi64(e, 31))));
    }
    V v = SUB64(ADD64(Y, Y), C64(1ull << 32));
    v = ANDN(GT64(zero, v), v);
    V dhs = _mm256_slli_epi64(dh, 32);
    V e = SUB64(SUB64(SUB64(C64(-(1ll << 32)), dhs), MULU(dh, v)), v);
    V eh = HI32(e);
    v = ADD64(v, ADD64(eh, HI32(MULU(v, eh))));
    V D = ADD64(dh, C64(1));
    V R = SUB64(SUB64(SUB64(C64(-(1ll << 32)), dhs), MULU(dh, v)), v);
    _Pragma("GCC unroll 9") for (int k = 0; k < 2; k++) { V ge = GT64(R, dh); R = SUB64(R, AND(ge, D)); v = SUB64(v, ge); }
    return v;
}
#define EST_CORR(U, q, rho, Nv, Nhi, Nm1b) {                                                                 \
    V qn_ = ADD64(MULU(q, Nv), _mm256_slli_epi64(MULU(q, Nhi), 32));                                      \
    rho = SUB64(U, qn_);                                                                                  \
    V ge_ = GT64(XOR(rho, bias), Nm1b);                                                                   \
    rho = SUB64(rho, AND(ge_, Nv)); q = SUB64(q, ge_); }
/* G registers (G = 1 or 2) interleaved */
AI void m5_core(int G, V U[][4], V N[][2], V Q[][4], V R[][4]) {
    const V zero = _mm256_setzero_si256(), bias = C64(1ull << 63);
    V vp[4], cl[4], cr[4], Nhi[4], Nm1b[4], Nn[4], sh[4];
    _Pragma("GCC unroll 9") for (int h = 0; h < 2 * G; h++) { Nn[h] = N[h >> 1][h & 1]; sh[h] = zero; }
    _Pragma("GCC unroll 9") for (int k = 32; k >= 1; k >>= 1)
        _Pragma("GCC unroll 9") for (int h = 0; h < 2 * G; h++) {
            V ks = AND(_mm256_cmpeq_epi64(_mm256_srli_epi64(Nn[h], 64 - k), zero), C64(k));
            Nn[h] = _mm256_sllv_epi64(Nn[h], ks); sh[h] = ADD64(sh[h], ks);
        }
    _Pragma("GCC unroll 9") for (int h = 0; h < 2 * G; h++) {
        vp[h] = recip(HI32(Nn[h]));
        cl[h] = SUB64(sh[h], C64(32)); cr[h] = SUB64(C64(32), sh[h]);
        V Nh = N[h >> 1][h & 1]; Nhi[h] = HI32(Nh); Nm1b[h] = XOR(SUB64(Nh, C64(1)), bias);
    }
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++)
        _Pragma("GCC unroll 9") for (int g = 0; g < G; g++) {
            int h = 2 * g + (j & 1); V Nh = N[g][j & 1];
            V th = OR(_mm256_sllv_epi64(U[g][j], cl[h]), _mm256_srlv_epi64(U[g][j], cr[h]));
            V Sm = ADD64(ADD64(th, MULU(HI32(th), vp[h])), HI32(MULU(th, vp[h])));
            Q[g][j] = HI32(Sm);
            EST_CORR(U[g][j], Q[g][j], R[g][j], Nh, Nhi[h], Nm1b[h])
        }
}
/* one Euclid step on lanes where b != 0: (a, b) <- (b, a - q b), q = a / b rounded by rule A; finished lanes hold */
AI void estep_finish(V *ax, V *ay, V *bx, V *by, V qr, V qi) {
    V done = EQ(OR(*bx, *by), _mm256_setzero_si256());
    V rx = ADD(SUB(*ax, MULLO(qr, *bx)), MULLO(qi, *by)), ry = SUB(SUB(*ay, MULLO(qr, *by)), MULLO(qi, *bx));
    *ax = SEL(*bx, *ax, done); *ay = SEL(*by, *ay, done);
    *bx = ANDN(done, rx); *by = ANDN(done, ry);
}
static long ev_regsteps;
NOINL u32 euclid_v(ARGS) {
    const V zero = _mm256_setzero_si256(); V sticky = zero;
    for (size_t i = 0; i < N; i += 8) {
        V ax0 = LD(ar + i), ay0 = LD(ai + i), bx0 = LD(br + i), by0 = LD(bi + i);
        GUARD(0)
        while (!_mm256_testz_si256(OR(bx0, by0), OR(bx0, by0))) {
            V Nv[2], U[4], S[4], Q[4], R[4], qr, qi;
            front(ax0, ay0, bx0, by0, Nv, U, S);
            m3_core(U, Nv, Q, R);
            near_a(Nv, Q, R, S, &qr, &qi);
            estep_finish(&ax0, &ay0, &bx0, &by0, qr, qi);
        }
        normstore(ax0, ay0, gr + i, gi + i);
    }
    return _mm256_testz_si256(sticky, sticky) ? 0 : fixup(PASSG);
}
NOINL u32 euclid_v_count(ARGS) {     /* register-steps, for the statistics */
    const V zero = _mm256_setzero_si256(); V sticky = zero;
    for (size_t i = 0; i < N; i += 8) {
        V ax0 = LD(ar + i), ay0 = LD(ai + i), bx0 = LD(br + i), by0 = LD(bi + i);
        GUARD(0)
        while (!_mm256_testz_si256(OR(bx0, by0), OR(bx0, by0))) {
            V Nv[2], U[4], S[4], Q[4], R[4], qr, qi;
            front(ax0, ay0, bx0, by0, Nv, U, S);
            m3_core(U, Nv, Q, R);
            near_a(Nv, Q, R, S, &qr, &qi);
            estep_finish(&ax0, &ay0, &bx0, &by0, qr, qi);
            ev_regsteps++;
        }
        normstore(ax0, ay0, gr + i, gi + i);
    }
    return _mm256_testz_si256(sticky, sticky) ? 0 : fixup(PASSG);
}
NOINL u32 euclid_v5(ARGS) {          /* m5 division, two registers stepped together until both are done */
    const V zero = _mm256_setzero_si256(); V sticky = zero;
    size_t i = 0;
    for (; i + 16 <= N; i += 16) {
        V ax0 = LD(ar + i), ay0 = LD(ai + i), bx0 = LD(br + i), by0 = LD(bi + i);
        V ax1 = LD(ar + i + 8), ay1 = LD(ai + i + 8), bx1 = LD(br + i + 8), by1 = LD(bi + i + 8);
        GUARD(0) GUARD(1)
        while (1) {
            V o = OR(OR(bx0, by0), OR(bx1, by1)); if (_mm256_testz_si256(o, o)) break;
            V Nv[2][2], U[2][4], S[2][4], Q[2][4], R[2][4], qr0, qi0, qr1, qi1;
            front(ax0, ay0, bx0, by0, Nv[0], U[0], S[0]); front(ax1, ay1, bx1, by1, Nv[1], U[1], S[1]);
            m5_core(2, U, Nv, Q, R);
            near_a(Nv[0], Q[0], R[0], S[0], &qr0, &qi0); near_a(Nv[1], Q[1], R[1], S[1], &qr1, &qi1);
            estep_finish(&ax0, &ay0, &bx0, &by0, qr0, qi0); estep_finish(&ax1, &ay1, &bx1, &by1, qr1, qi1);
        }
        normstore(ax0, ay0, gr + i, gi + i); normstore(ax1, ay1, gr + i + 8, gi + i + 8);
    }
    for (; i < N; i += 8) {
        V ax0 = LD(ar + i), ay0 = LD(ai + i), bx0 = LD(br + i), by0 = LD(bi + i);
        GUARD(0)
        while (!_mm256_testz_si256(OR(bx0, by0), OR(bx0, by0))) {
            V Nv[1][2], U[1][4], S[1][4], Q[1][4], R[1][4], qr, qi;
            front(ax0, ay0, bx0, by0, Nv[0], U[0], S[0]);
            m5_core(1, U, Nv, Q, R);
            near_a(Nv[0], Q[0], R[0], S[0], &qr, &qi);
            estep_finish(&ax0, &ay0, &bx0, &by0, qr, qi);
        }
        normstore(ax0, ay0, gr + i, gi + i);
    }
    return _mm256_testz_si256(sticky, sticky) ? 0 : fixup(PASSG);
}

/* ======================= kernel table ======================= */
typedef u32 (*kf)(ARGS);
typedef struct { const char *name; kf f; int guarded; } kern;
static kern KT[] = { {"euclid", euclid, 1}, {"euclid_v", euclid_v, 1}, {"euclid_v5", euclid_v5, 1}, {"pi_sb", pi_sb, 1},
                     {"pi_v_bs", pi_v_bs, 1}, {"pi_v_pop", pi_v_pop, 1}, {"pi_v_db", pi_v_db, 1},
                     {"pi_v3_bs", pi_v3_bs, 1}, {"pi_v3_pop", pi_v3_pop, 1}, {"pi_v3_db", pi_v3_db, 1}, {"pi_v3_pop_ng", pi_v3_pop_ng, 0} };
#define NKT (int)(sizeof KT / sizeof KT[0])

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
            if (!KT[k].guarded && hm) continue;
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
    for (int k = 0; k < NKT; k++) if (KT[k].guarded && fl[k] != (u32)anyov)
        if (fails++ < 10) printf("FAIL %s flag %u want %d (%d,%d) (%d,%d)\n", KT[k].name, fl[k], anyov, A1[0], A2[0], B1[0], B2[0]);
}
static void do_ctz_check(void) {
    long bad[3] = {0};
    for (u64 base = 1; base < (1ull << 32); base += 8) {
        V o = _mm256_add_epi32(C32((u32)base), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
        V r[3] = {ctz_bs(o), ctz_pop(o), ctz_db(o)};
        u32 t[3][8]; for (int m = 0; m < 3; m++) ST(t[m], r[m]);
        for (int j = 0; j < 8; j++) { u32 v = (u32)(base + j); if (!v) continue; int want = __builtin_ctz(v);
            for (int m = 0; m < 3; m++) bad[m] += (int)t[m][j] != want; }
    }
    printf("lane ctz against __builtin_ctz, every nonzero 32-bit value: wrong (bs, pop, db) = %ld, %ld, %ld\n", bad[0], bad[1], bad[2]);
}
static void do_check(void) {
    do_ctz_check();
    /* exhaustive, two's complement K-bit halves (at 32 bits these contain no -2^31) */
    for (int K = 3; K <= 6; K++) {
        i64 lo = -(1 << (K - 1)), hi = (1 << (K - 1)) - 1; long cnt = 0; size_t n = 0;
        for (i64 p = lo; p <= hi; p++) for (i64 q = lo; q <= hi; q++) for (i64 r = lo; r <= hi; r++) for (i64 s = lo; s <= hi; s++) {
            A1[n] = (i32)p; A2[n] = (i32)q; B1[n] = (i32)r; B2[n] = (i32)s; n++; cnt++;
            if (n == BM) { check_batch(n, K <= 4 ? 2 : 0, K); n = 0; }
        }
        if (n) check_batch(n, K <= 4 ? 2 : 0, K);
        printf("K = %d: exhaustive, %ld pairs%s; failures so far %ld\n", K, cnt, K <= 4 ? " (with divisibility and brute force)" : "", fails);
    }
    /* edges with -2^31: every quadruple of 20 values, each pair alone (so the flag is per pair) */
    long cnt = 0;
    for (int p = 0; p < NEVM; p++) for (int q = 0; q < NEVM; q++) for (int r = 0; r < NEVM; r++) for (int s = 0; s < NEVM; s++) {
        A1[0] = EVM[p]; A2[0] = EVM[q]; B1[0] = EVM[r]; B2[0] = EVM[s]; check_batch(1, 1, 32); cnt++;
    }
    printf("edges incl. -2^31: %ld pairs each alone; batches with a -2^31 half %ld, with an overflowing gcd %ld; failures so far %ld\n", cnt, nsticky, nov, fails);
    nov = nsticky = 0;
    /* -2^31 pairs alone, random */
    for (int it = 0; it < 200000; it++) { gen(4, A1, A2, B1, B2); check_batch(1, 1, 32); }
    printf("random -2^31 pairs alone: 200,000; with -2^31 %ld, overflowing %ld; failures so far %ld\n", nsticky, nov, fails);
    /* random batches, five kinds mixed */
    long tot = 0;
    for (int it = 0; it < 40; it++) {
        for (int j = 0; j < BM; j++) gen(j % 5 == 4 ? ((next() & 63) ? 0 : 4) : j % 5, A1 + j, A2 + j, B1 + j, B2 + j);
        check_batch(BM, 1, 32); tot += BM;
    }
    printf("random: %ld pairs (random, small b, common factor 2^4..2^15, factor 2^15, a few -2^31)\nTOTAL failures %ld\n", tot, fails);
}

/* ======================= statistics ======================= */
static void do_stats(void) {
    static i32 G1[BM], G2[BM];
    const char *kn[] = {"random", "large common factor 2^15"};
    int kinds[2] = {0, 3};
    for (int c = 0; c < 2; c++) {
        long esum = 0, regsum = 0;
        for (int j = 0; j < BM; j++) gen(kinds[c], A1 + j, A2 + j, B1 + j, B2 + j);
        for (int j = 0; j < BM; j += 8) {
            int rm = 0;
            for (int l = 0; l < 8; l++) { long e0 = euclid_steps; euclid1(A1[j + l], A2[j + l], B1[j + l], B2[j + l], G1, G2, 1);
                int s = (int)(euclid_steps - e0); esum += s; if (s > rm) rm = s; }
            regsum += rm;
        }
        ev_regsteps = 0; euclid_v_count(A1, A2, B1, B2, G1, G2, BM);
        printf("%-26s Euclid steps per pair %ld.%02ld, per register (max of 8) %ld.%02ld (vector kernel counted %ld.%02ld), lane use %ld%%\n", kn[c],
               esum / BM, esum * 100 / BM % 100, regsum * 8 / BM, regsum * 800 / BM % 100, ev_regsteps * 8 / BM, ev_regsteps * 800 / BM % 100, esum * 100 / (regsum * 8));
    }
}

/* ======================= timing (integer clock) ======================= */
static u64 now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec; }
static int cmpu(const void *a, const void *b) { u64 x = *(const u64 *)a, y = *(const u64 *)b; return (x > y) - (x < y); }
#define REPS 31
static void pps(u64 ps) { printf("%4llu.%02llu", (unsigned long long)(ps / 1000), (unsigned long long)(ps % 1000 / 10)); }
static void do_time(void) {
    enum { N = 1032 };          /* a multiple of 24 and of 8, about 24 KB */
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
                int inner = k == 0 || k == 3 ? 8 : 32;
                KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                u64 t0 = now_ns();
                for (int it = 0; it < inner; it++) KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                t[k][r] = (now_ns() - t0) * 1000 / ((u64)N * inner);
            }
        printf("\n%s\n", kn[c]);
        for (int k = 0; k < NKT; k++) {
            qsort(t[k], REPS, 8, cmpu);
            printf("  %-13s ", KT[k].name); pps(t[k][REPS / 2]); printf(" ["); pps(t[k][REPS / 4]); printf(", "); pps(t[k][3 * REPS / 4]); printf("]\n");
        }
        fflush(stdout);
    }
}
/* ctz alone: throughput over an array, and latency of a dependent chain v <- v + ctz(v) + 1 */
static void do_ctz_time(void) {
    enum { M = 4096 };
    static u32 a[M]; for (int j = 0; j < M; j++) a[j] = (u32)next() | 1u << (next() % 32);
    const char *nm[3] = {"bs", "pop", "db"};
    for (int m = 0; m < 3; m++) {
        u64 tt[REPS], tl[REPS];
        for (int r = 0; r < REPS; r++) {
            V acc = _mm256_setzero_si256();
            u64 t0 = now_ns();
            for (int it = 0; it < 64; it++)
                for (int j = 0; j < M; j += 8) { V o = LD(a + j); acc = ADD(acc, m == 0 ? ctz_bs(o) : m == 1 ? ctz_pop(o) : ctz_db(o)); }
            tt[r] = (now_ns() - t0) * 1000 / (64 * M / 8);
            V v = LD(a);
            t0 = now_ns();
            for (int it = 0; it < 100000; it++) v = ADD(v, ADD(m == 0 ? ctz_bs(v) : m == 1 ? ctz_pop(v) : ctz_db(v), C32(1)));
            tl[r] = (now_ns() - t0) * 1000 / 100000;
            u32 sink[8]; ST(sink, OR(acc, v)); if (sink[0] == 12345) printf(" ");
        }
        qsort(tt, REPS, 8, cmpu); qsort(tl, REPS, 8, cmpu);
        printf("ctz_%-4s per register of 8: throughput ", nm[m]); pps(tt[REPS / 2]); printf(" ns, chain latency (with one add) "); pps(tl[REPS / 2]); printf(" ns\n");
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else if (!strcmp(mode, "stats")) do_stats();
    else if (!strcmp(mode, "ctz")) do_ctz_time();
    else do_time();
    return 0;
}
