/* Study 24: the Gaussian gcd in vector lanes, where there is no integer divide.
 *
 * Arrays of independent pairs (a, b), int32 halves in separate arrays (re a, im a,
 * re b, im b), halves in the symmetric range |x| <= 2^31 - 1 as in study 6. Output:
 * gcd(a, b) normalised to the associate with re > 0, im >= 0 (gcd(0, 0) = 0).
 *
 *   euclid   study 6: Euclid by rounded division (two 64-bit divides per step), scalar
 *   pi_sc    the (1+i)-ary gcd of studies 9/11 (|x|+|y| size, stop when equal), scalar, tzcnt
 *   pi_sb    the same step written with masks (only the loop branches)
 *   pi_v     the same in AVX2 lanes, 8 pairs per register, loop until every lane is done
 *            (tested every step)
 *   pi_v4    the same, tested every 4 steps (extra steps are harmless: the state is a
 *            fixed point once a = b)
 *   pi_vT    the same with a fixed step count T = 276, the proven bound (no test)
 *   pi_v2, pi_v3, pi_v4x   2, 3 or 4 registers stepped together (the step is latency-bound)
 *
 * The step in 32-bit lanes. a and b are primary (x odd, y even, x - y = 1 mod 4) with
 * |x|, |y| <= 2^31 - 1, so a - b can need 33 bits. But a - b is divisible by (1+i)^3, so
 * by 2 in each coordinate, and h = (a - b)/2 = ((ax >> 1) - (bx >> 1), (ay >> 1) - (by >> 1))
 * exactly, inside 32 bits. Then strip h (shift by e = ctz(hx | hy), found from the float
 * exponent of the lowest set bit since AVX2 has no lane ctz; then one division by 1+i when
 * both parts are odd, as ((x >> 1) + (y >> 1) + 1, (y >> 1) - (x >> 1))), make it primary
 * (blend for the multiply by -i, vpsignd for the negation), and replace a by it, keeping
 * the larger by |x| + |y| (at most 2^32 - 2: compared unsigned, vpmaxud/vpminud) in a.
 * When a = b, h = 0, and the zero is replaced by b, so (a, b) stays (b, b): a fixed point,
 * and finished lanes need no hold mask.
 *
 * Build: gcc -O2 -mavx2 -o gcdvec gcdvec.c
 * Run:   ./gcdvec check | ./gcdvec stats | ./gcdvec time
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef __int128 i128;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define HMAX 2147483647
#define ARGS const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
             i32 *restrict gr, i32 *restrict gi, size_t N

/* ======================= scalar Euclid (study 6) ======================= */
static inline i64 rdiv(i64 x, i64 n) {          /* nearest, halves up; n > 0 */
    i64 q = x / n, r = x % n;
    i64 m = r >> 63; q += m; r += n & m;         /* floor */
    return q + (r >= n - r);
}
static inline void normalise(i64 *px, i64 *py) {
    i64 x = *px, y = *py;
    if (y < 0 || (y == 0 && x < 0)) { x = -x; y = -y; }
    if (x <= 0 && y > 0) { i64 t = x; x = y; y = -t; }
    *px = x; *py = y;
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
NOINL void euclid(ARGS) { for (size_t i = 0; i < N; i++) euclid1(ar[i], ai[i], br[i], bi[i], gr + i, gi + i, 0); }

/* ======================= scalar (1+i)-ary, and its checked model ======================= */
/* The model (CHK) runs the same algorithm in 64-bit integers and checks that every value the
 * lane version holds fits its lane: coordinates |v| <= H (H = 2^(K-1) - 1 for inputs in
 * K-bit halves), h = (a - b)/2 within H, the 1+i division within H, and |x| + |y| <= 2H. */
static int modelK = 32; static long model_bad, model_steps;
#define FIT(v) do { if ((v) > H || (v) < -H) model_bad++; } while (0)
static inline int ctz64(u64 v) { return v ? __builtin_ctzll(v) : 64; }
#define PI_BODY(T, CHK)                                                                         \
    T H = ((T)1 << (modelK - 1)) - 1; (void)H;                                                  \
    T ax = a1, ay = a2, bx = b1, by = b2;                                                       \
    int za = !(ax | ay), zb = !(bx | by);                                                       \
    int ea = ctz64((u64)(ax | ay)), eb = ctz64((u64)(bx | by));                                 \
    int va = za ? 1000 : 2 * ea + (int)((ax >> ea) & (ay >> ea) & 1);                          \
    int vb = zb ? 1000 : 2 * eb + (int)((bx >> eb) & (by >> eb) & 1);                          \
    int k = va < vb ? va : vb;                                                                  \
    STRIP(ax, ay) PRIM(ax, ay) STRIP(bx, by) PRIM(bx, by)                                       \
    if (za) { ax = bx; ay = by; }                                                               \
    if (zb) { bx = ax; by = ay; }                                                               \
    u64 sa = SZ(ax, ay), sb = SZ(bx, by);                                                       \
    if (sa < sb) { T t = ax; ax = bx; bx = t; t = ay; ay = by; by = t; sb = sa; }               \
    int steps = 0;                                                                              \
    while (ax != bx || ay != by) {                                                              \
        T hx = (ax >> 1) - (bx >> 1), hy = (ay >> 1) - (by >> 1);                               \
        if (CHK) { FIT(hx); FIT(hy); if (((ax - bx) | (ay - by)) & 1) model_bad++; }            \
        STRIP(hx, hy) PRIM(hx, hy)                                                              \
        if (CHK) { FIT(hx); FIT(hy); }                                                          \
        u64 s = SZ(hx, hy);                                                                     \
        if (CHK && s > (u64)(2 * H)) model_bad++;                                               \
        if (s >= sb) { ax = hx; ay = hy; }                                                      \
        else { ax = bx; ay = by; bx = hx; by = hy; sb = s; }                                    \
        steps++;                                                                                \
    }                                                                                           \
    T gx = k > 64 ? 0 : ax << (k >> 1), gy = k > 64 ? 0 : ay << (k >> 1);                       \
    if (k & 1) { T t = gx - gy; gy = gx + gy; gx = t; }                                         \
    i64 nx = gx, ny = gy; normalise(&nx, &ny);
#define STRIP(x, y) { int e_ = ctz64((u64)((x) | (y))) & 63; x >>= e_; y >>= e_;                 \
    if ((x) & (y) & 1) { __typeof__(x) px_ = x >> 1, py_ = y >> 1; x = px_ + py_ + 1; y = py_ - px_; } }
#define PRIM(x, y) { if (!((x) & 1)) { __typeof__(x) t_ = x; x = y; y = -t_; }                  \
    if (((x) - (y)) & 2) { x = -x; y = -y; } }
#define SZ(x, y) ((u64)((x) < 0 ? -(x) : (x)) + (u64)((y) < 0 ? -(y) : (y)))

static inline void pi1(i32 a1, i32 a2, i32 b1, i32 b2, i32 *g1, i32 *g2) {
    PI_BODY(i32, 0)
    *g1 = (i32)nx; *g2 = (i32)ny; (void)steps;
}
NOINL void pi_sc(ARGS) { for (size_t i = 0; i < N; i++) pi1(ar[i], ai[i], br[i], bi[i], gr + i, gi + i); }
static int pi_model(i64 a1, i64 a2, i64 b1, i64 b2, i32 *g1, i32 *g2) {   /* returns the step count */
    PI_BODY(i64, 1)
    *g1 = (i32)nx; *g2 = (i32)ny; model_steps += steps;
    return steps;
}

/* the same step written branch-free in scalar registers (masks, tzcnt); only the loop branches */
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
        m = (x & 1) - 1;                                       /* all ones where x is even */
        i32 nx = x ^ ((x ^ y) & m), ny = y ^ ((y ^ -x) & m);
        m = -((nx - ny) >> 1 & 1);
        x = (nx ^ m) - m; y = (ny ^ m) - m;
        x ^= (x ^ bx) & zm; y ^= (y ^ by) & zm;
        u32 s = (u32)abs(x) + (u32)abs(y);
        i32 gm = -(i32)(s >= sb);                              /* keep the new value in a */
        i32 nax = bx ^ ((bx ^ x) & gm), nay = by ^ ((by ^ y) & gm);
        bx = x ^ ((x ^ bx) & gm); by = y ^ ((y ^ by) & gm);
        ax = nax; ay = nay; sb = s < sb ? s : sb;
    }
    i32 gx = k > 64 ? 0 : ax << (k >> 1), gy = k > 64 ? 0 : ay << (k >> 1);
    if (k & 1) { i32 t = gx - gy; gy = gx + gy; gx = t; }
    i64 nx = gx, ny = gy; normalise(&nx, &ny);
    *g1 = (i32)nx; *g2 = (i32)ny;
}
NOINL void pi_sb(ARGS) { for (size_t i = 0; i < N; i++) pib1(ar[i], ai[i], br[i], bi[i], gr + i, gi + i); }

/* ======================= AVX2 lanes ======================= */
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define ADD(a, b) _mm256_add_epi32(a, b)
#define SUB(a, b) _mm256_sub_epi32(a, b)
#define SRA1(a) _mm256_srai_epi32(a, 1)
#define EQ(a, b) _mm256_cmpeq_epi32(a, b)
#define C32(x) _mm256_set1_epi32((int)(x))
/* choose b where bit 31 of m is set, else a */
#define SEL(a, b, m) _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(a), _mm256_castsi256_ps(b), _mm256_castsi256_ps(m)))

/* e = ctz(o) for o != 0 (o has no bit 31 alone here), from the float exponent; o = 0 gives a
 * huge unsigned count, and vpsravd then fills with the sign (0 for 0) */
static inline V ctzv(V o) {
    V lb = AND(o, SUB(_mm256_setzero_si256(), o));
    return SUB(_mm256_srli_epi32(_mm256_castps_si256(_mm256_cvtepi32_ps(lb)), 23), C32(127));
}
static inline void stripv(V *px, V *py, V e) {
    V x = _mm256_srav_epi32(*px, e), y = _mm256_srav_epi32(*py, e);
    V m = _mm256_slli_epi32(AND(x, y), 31);                      /* both odd */
    V hx = SRA1(x), hy = SRA1(y);
    *px = SEL(x, SUB(hx, XOR(hy, C32(-1))), m);                  /* hx + hy + 1 */
    *py = SEL(y, SUB(hy, hx), m);
}
static inline void primv(V *px, V *py) {
    V x = *px, y = *py;
    V m = _mm256_slli_epi32(x, 31);                              /* x odd */
    V nx = SEL(y, x, m), ny = SEL(SUB(_mm256_setzero_si256(), x), y, m);
    V t = OR(_mm256_slli_epi32(SUB(nx, ny), 30), C32(1));        /* negative iff nx - ny = 3 mod 4 */
    *px = _mm256_sign_epi32(nx, t); *py = _mm256_sign_epi32(ny, t);
}
static inline V sizev(V x, V y) { return ADD(_mm256_abs_epi32(x), _mm256_abs_epi32(y)); }

#define SETUP                                                                                  \
    const V zero = _mm256_setzero_si256();                                                     \
    V ax = LD(ar + i), ay = LD(ai + i), bx = LD(br + i), by = LD(bi + i);                      \
    V oa = OR(ax, ay), ob = OR(bx, by), za = EQ(oa, zero), zb = EQ(ob, zero);                  \
    V ea = ctzv(oa), eb = ctzv(ob);                                                            \
    V va = ADD(ADD(ea, ea), AND(AND(_mm256_srav_epi32(ax, ea), _mm256_srav_epi32(ay, ea)), C32(1)));      \
    V vb = ADD(ADD(eb, eb), AND(AND(_mm256_srav_epi32(bx, eb), _mm256_srav_epi32(by, eb)), C32(1)));      \
    V k = _mm256_min_epi32(SEL(va, C32(1000), za), SEL(vb, C32(1000), zb));                   \
    stripv(&ax, &ay, ea); primv(&ax, &ay); stripv(&bx, &by, eb); primv(&bx, &by);             \
    { V tx = ax, ty = ay; ax = SEL(ax, bx, za); ay = SEL(ay, by, za);                          \
      bx = SEL(bx, tx, zb); by = SEL(by, ty, zb); }                                            \
    V sa = sizev(ax, ay), sb = sizev(bx, by);                                                  \
    { V mn = _mm256_min_epu32(sa, sb), ge = EQ(mn, sb);                                        \
      V tx = ax, ty = ay; ax = SEL(bx, ax, ge); ay = SEL(by, ay, ge);                          \
      bx = SEL(tx, bx, ge); by = SEL(ty, by, ge); sb = mn; }

#define STEP {                                                                                 \
    V hx = SUB(SRA1(ax), SRA1(bx)), hy = SUB(SRA1(ay), SRA1(by));                              \
    V o = OR(hx, hy), z = EQ(o, zero);                                                         \
    stripv(&hx, &hy, ctzv(o)); primv(&hx, &hy);                                                \
    hx = SEL(hx, bx, z); hy = SEL(hy, by, z);                                                  \
    V s = sizev(hx, hy), mn = _mm256_min_epu32(s, sb), ge = EQ(mn, sb);                        \
    V nbx = SEL(hx, bx, ge), nby = SEL(hy, by, ge);                                            \
    ax = SEL(bx, hx, ge); ay = SEL(by, hy, ge); bx = nbx; by = nby; sb = mn; }

#define DONE _mm256_testz_si256(OR(XOR(ax, bx), XOR(ay, by)), OR(XOR(ax, bx), XOR(ay, by)))

#define FINISH {                                                                               \
    V s2 = _mm256_srai_epi32(k, 1);                                                            \
    V gx = _mm256_sllv_epi32(ax, s2), gy = _mm256_sllv_epi32(ay, s2);                          \
    V od = _mm256_slli_epi32(k, 31);                                                           \
    V tx = SUB(gx, gy), ty = ADD(gx, gy);                                                      \
    gx = SEL(gx, tx, od); gy = SEL(gy, ty, od);                                                \
    V nm = OR(_mm256_cmpgt_epi32(zero, gy), AND(EQ(gy, zero), _mm256_cmpgt_epi32(zero, gx)));  \
    gx = SUB(XOR(gx, nm), nm); gy = SUB(XOR(gy, nm), nm);                                      \
    V rm = AND(_mm256_cmpgt_epi32(C32(1), gx), _mm256_cmpgt_epi32(gy, zero));                  \
    V rx = SEL(gx, gy, rm); gy = SEL(gy, SUB(zero, gx), rm);                                   \
    ST(gr + i, rx); ST(gi + i, gy); }

static long vec_regsteps;    /* register-steps, counted by the instrumented copy only */
NOINL void pi_v(ARGS) {
    for (size_t i = 0; i < N; i += 8) {
        SETUP
        while (!DONE) STEP
        FINISH
    }
}
NOINL void pi_v_count(ARGS) {
    for (size_t i = 0; i < N; i += 8) {
        SETUP
        while (!DONE) { STEP vec_regsteps++; }
        FINISH
    }
}
NOINL void pi_v4(ARGS) {
    for (size_t i = 0; i < N; i += 8) {
        SETUP
        while (!DONE) { STEP STEP STEP STEP }
        FINISH
    }
}
#define SETUPP(P, OFF)                                                                                  \
    V ax##P = LD(ar + i + OFF), ay##P = LD(ai + i + OFF), bx##P = LD(br + i + OFF), by##P = LD(bi + i + OFF);                      \
    V oa##P = OR(ax##P, ay##P), ob##P = OR(bx##P, by##P), za##P = EQ(oa##P, zero), zb##P = EQ(ob##P, zero);                  \
    V ea##P = ctzv(oa##P), eb##P = ctzv(ob##P);                                                            \
    V va##P = ADD(ADD(ea##P, ea##P), AND(AND(_mm256_srav_epi32(ax##P, ea##P), _mm256_srav_epi32(ay##P, ea##P)), C32(1)));      \
    V vb##P = ADD(ADD(eb##P, eb##P), AND(AND(_mm256_srav_epi32(bx##P, eb##P), _mm256_srav_epi32(by##P, eb##P)), C32(1)));      \
    V k##P = _mm256_min_epi32(SEL(va##P, C32(1000), za##P), SEL(vb##P, C32(1000), zb##P));                   \
    stripv(&ax##P, &ay##P, ea##P); primv(&ax##P, &ay##P); stripv(&bx##P, &by##P, eb##P); primv(&bx##P, &by##P);             \
    { V tx = ax##P, ty = ay##P; ax##P = SEL(ax##P, bx##P, za##P); ay##P = SEL(ay##P, by##P, za##P);                          \
      bx##P = SEL(bx##P, tx, zb##P); by##P = SEL(by##P, ty, zb##P); }                                            \
    V sa##P = sizev(ax##P, ay##P), sb##P = sizev(bx##P, by##P);                                                  \
    { V mn = _mm256_min_epu32(sa##P, sb##P), ge = EQ(mn, sb##P);                                        \
      V tx = ax##P, ty = ay##P; ax##P = SEL(bx##P, ax##P, ge); ay##P = SEL(by##P, ay##P, ge);                          \
      bx##P = SEL(tx, bx##P, ge); by##P = SEL(ty, by##P, ge); sb##P = mn; }
#define STEPP(P) {                                                                                 \
    V hx = SUB(SRA1(ax##P), SRA1(bx##P)), hy = SUB(SRA1(ay##P), SRA1(by##P));                              \
    V o = OR(hx, hy), z = EQ(o, zero);                                                         \
    stripv(&hx, &hy, ctzv(o)); primv(&hx, &hy);                                                \
    hx = SEL(hx, bx##P, z); hy = SEL(hy, by##P, z);                                                  \
    V s = sizev(hx, hy), mn = _mm256_min_epu32(s, sb##P), ge = EQ(mn, sb##P);                        \
    V nbx = SEL(hx, bx##P, ge), nby = SEL(hy, by##P, ge);                                            \
    ax##P = SEL(bx##P, hx, ge); ay##P = SEL(by##P, hy, ge); bx##P = nbx; by##P = nby; sb##P = mn; }
#define FINISHP(P, OFF) {                                                                               \
    V s2 = _mm256_srai_epi32(k##P, 1);                                                            \
    V gx = _mm256_sllv_epi32(ax##P, s2), gy = _mm256_sllv_epi32(ay##P, s2);                          \
    V od = _mm256_slli_epi32(k##P, 31);                                                           \
    V tx = SUB(gx, gy), ty = ADD(gx, gy);                                                      \
    gx = SEL(gx, tx, od); gy = SEL(gy, ty, od);                                                \
    V nm = OR(_mm256_cmpgt_epi32(zero, gy), AND(EQ(gy, zero), _mm256_cmpgt_epi32(zero, gx)));  \
    gx = SUB(XOR(gx, nm), nm); gy = SUB(XOR(gy, nm), nm);                                      \
    V rm = AND(_mm256_cmpgt_epi32(C32(1), gx), _mm256_cmpgt_epi32(gy, zero));                  \
    V rx = SEL(gx, gy, rm); gy = SEL(gy, SUB(zero, gx), rm);                                   \
    ST(gr + i + OFF, rx); ST(gi + i + OFF, gy); }
#define DIFF(P) OR(XOR(ax##P, bx##P), XOR(ay##P, by##P))
/* interleaved: U independent registers of 8 pairs stepped together (the step is a long
 * dependency chain), until every lane of all U is done */
NOINL void pi_v2(ARGS) {
    const V zero = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 16) {
        SETUPP(0, 0) SETUPP(1, 8)
        while (!_mm256_testz_si256(OR(DIFF(0), DIFF(1)), OR(DIFF(0), DIFF(1)))) { STEPP(0) STEPP(1) }
        FINISHP(0, 0) FINISHP(1, 8)
    }
}
NOINL void pi_v3(ARGS) {
    const V zero = _mm256_setzero_si256();
    size_t i = 0;
    for (; i + 24 <= N; i += 24) {
        SETUPP(0, 0) SETUPP(1, 8) SETUPP(2, 16)
        while (1) { V d = OR(OR(DIFF(0), DIFF(1)), DIFF(2)); if (_mm256_testz_si256(d, d)) break; STEPP(0) STEPP(1) STEPP(2) }
        FINISHP(0, 0) FINISHP(1, 8) FINISHP(2, 16)
    }
    for (; i < N; i += 8) { SETUPP(0, 0) while (!_mm256_testz_si256(DIFF(0), DIFF(0))) STEPP(0) FINISHP(0, 0) }
}
NOINL void pi_v4x(ARGS) {
    const V zero = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 32) {
        SETUPP(0, 0) SETUPP(1, 8) SETUPP(2, 16) SETUPP(3, 24)
        while (1) { V d = OR(OR(DIFF(0), DIFF(1)), OR(DIFF(2), DIFF(3))); if (_mm256_testz_si256(d, d)) break;
                    STEPP(0) STEPP(1) STEPP(2) STEPP(3) }
        FINISHP(0, 0) FINISHP(1, 8) FINISHP(2, 16) FINISHP(3, 24)
    }
}

#define TFIX 276
NOINL void pi_vT(ARGS) {
    for (size_t i = 0; i < N; i += 8) {
        SETUP
        for (int t = 0; t < TFIX; t++) STEP
        FINISH
    }
}

/* ======================= kernel table ======================= */
typedef void (*kf)(ARGS);
typedef struct { const char *name; kf f; } kern;
static kern KT[] = { {"euclid", euclid}, {"pi_sc", pi_sc}, {"pi_sb", pi_sb}, {"pi_v", pi_v}, {"pi_v4", pi_v4}, {"pi_vT", pi_vT}, {"pi_v2", pi_v2}, {"pi_v3", pi_v3}, {"pi_v4x", pi_v4x} };
#define NKT (int)(sizeof KT / sizeof KT[0])

/* ======================= inputs ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i64 rr(i64 lim) { return (i64)(next() % (u64)(2 * lim + 1)) - lim; }   /* uniform in [-lim, lim] */
/* kind 0: random; 1: b small (parts up to 2^12); 2: common factor g with parts up to 2^e,
 * e in 4..15 (study 6's mix); 3: large common factor, parts of g and the cofactors up to 2^15 */
static void gen(int kind, i32 *a1, i32 *a2, i32 *b1, i32 *b2) {
    if (kind == 0) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(HMAX); *b2 = rr(HMAX); return; }
    if (kind == 1) { *a1 = rr(HMAX); *a2 = rr(HMAX); *b1 = rr(1 << 12); *b2 = rr(1 << 12); return; }
    int e = kind == 2 ? 4 + (int)(next() % 12) : 15;
    i64 g1 = rr(1LL << e), g2 = rr(1LL << e), lim = HMAX / (2LL << e);
    i64 u1 = rr(lim), u2 = rr(lim), v1 = rr(lim), v2 = rr(lim);
    *a1 = (i32)(g1 * u1 - g2 * u2); *a2 = (i32)(g1 * u2 + g2 * u1);
    *b1 = (i32)(g1 * v1 - g2 * v2); *b2 = (i32)(g1 * v2 + g2 * v1);
}

/* ======================= correctness ======================= */
static long fails;
static int divides(i64 g1, i64 g2, i64 a1, i64 a2) {     /* g | a; g = 0 divides only 0 */
    i128 n = (i128)g1 * g1 + (i128)g2 * g2;
    if (n == 0) return a1 == 0 && a2 == 0;
    i128 x = (i128)a1 * g1 + (i128)a2 * g2, y = (i128)a2 * g1 - (i128)a1 * g2;
    return x % n == 0 && y % n == 0;
}
static void check_batch(const i32 *A1, const i32 *A2, const i32 *B1, const i32 *B2, size_t n, int divcheck) {
    static i32 out[NKT][2][(1 << 16) + 32];
    size_t N = (n + 31) & ~(size_t)31;
    static i32 a1[(1 << 16) + 32], a2[(1 << 16) + 32], b1[(1 << 16) + 32], b2[(1 << 16) + 32];
    memcpy(a1, A1, n * 4); memcpy(a2, A2, n * 4); memcpy(b1, B1, n * 4); memcpy(b2, B2, n * 4);
    for (size_t j = n; j < N; j++) a1[j] = a2[j] = b1[j] = b2[j] = 0;
    for (int k = 0; k < NKT; k++) KT[k].f(a1, a2, b1, b2, out[k][0], out[k][1], N);
    for (size_t j = 0; j < n; j++) {
        i32 m1, m2; pi_model(a1[j], a2[j], b1[j], b2[j], &m1, &m2);
        i32 e1 = out[0][0][j], e2 = out[0][1][j];
        if (m1 != e1 || m2 != e2) if (fails++ < 10) printf("FAIL model (%d,%d) (%d,%d)\n", a1[j], a2[j], b1[j], b2[j]);
        for (int k = 1; k < NKT; k++)
            if (out[k][0][j] != e1 || out[k][1][j] != e2)
                if (fails++ < 10) printf("FAIL %s (%d,%d) (%d,%d): got (%d,%d) want (%d,%d)\n", KT[k].name,
                                         a1[j], a2[j], b1[j], b2[j], out[k][0][j], out[k][1][j], e1, e2);
        if (divcheck == 2) {   /* brute force: N(g) is the largest norm of a common divisor */
            int H = (1 << (modelK - 1)) - 1; i64 best = 0;
            for (int d1 = -2 * H; d1 <= 2 * H; d1++) for (int d2 = -2 * H; d2 <= 2 * H; d2++)
                if ((d1 | d2) && divides(d1, d2, a1[j], a2[j]) && divides(d1, d2, b1[j], b2[j]) && d1 * d1 + d2 * d2 > best)
                    best = d1 * d1 + d2 * d2;
            if (!(a1[j] | a2[j] | b1[j] | b2[j])) best = 0;
            if (best != (i64)e1 * e1 + (i64)e2 * e2) if (fails++ < 10) printf("FAIL brute (%d,%d) (%d,%d)\n", a1[j], a2[j], b1[j], b2[j]);
        }
        if (divcheck) {
            int okn = e1 > 0 ? e2 >= 0 : (e1 == 0 && e2 == 0);
            if (!okn || !divides(e1, e2, a1[j], a2[j]) || !divides(e1, e2, b1[j], b2[j]))
                if (fails++ < 10) printf("FAIL divides (%d,%d) (%d,%d)\n", a1[j], a2[j], b1[j], b2[j]);
        }
    }
}
static void do_check(void) {
    static i32 A1[1 << 16], A2[1 << 16], B1[1 << 16], B2[1 << 16];
    for (int K = 3; K <= 6; K++) {
        int H = (1 << (K - 1)) - 1; modelK = K; long cnt = 0; size_t n = 0;
        for (int p = -H; p <= H; p++) for (int q = -H; q <= H; q++) for (int r = -H; r <= H; r++) for (int s = -H; s <= H; s++) {
            A1[n] = p; A2[n] = q; B1[n] = r; B2[n] = s; n++; cnt++;
            if (n == (1 << 16)) { check_batch(A1, A2, B1, B2, n, K <= 4 ? 2 : 0); n = 0; }
        }
        if (n) check_batch(A1, A2, B1, B2, n, K <= 4 ? 2 : 0);
        printf("K = %d: exhaustive, %ld pairs%s; failures so far %ld, out-of-range values in the model %ld\n", K, cnt, K <= 4 ? " (with divisibility and brute force)" : "", fails, model_bad);
    }
    modelK = 32;
    /* edges at 31 bits */
    const i32 ev[] = {0, 1, -1, 2, -2, 3, HMAX, -HMAX, HMAX - 1, -HMAX + 1, 1 << 30, -(1 << 30), (1 << 30) + 1, -(1 << 30) - 1,
                      (1 << 30) - 1, -(1 << 30) + 1, 1 << 16, -(1 << 16), 46341, -46341};
    int ne = sizeof ev / sizeof ev[0]; size_t n = 0; long cnt = 0;
    for (int p = 0; p < ne; p++) for (int q = 0; q < ne; q++) for (int r = 0; r < ne; r++) for (int s = 0; s < ne; s++) {
        A1[n] = ev[p]; A2[n] = ev[q]; B1[n] = ev[r]; B2[n] = ev[s]; n++; cnt++;
        if (n == (1 << 16)) { check_batch(A1, A2, B1, B2, n, 1); n = 0; }
    }
    if (n) check_batch(A1, A2, B1, B2, n, 1);
    printf("31 bits: every quadruple of %d edge values, %ld pairs; failures so far %ld\n", ne, cnt, fails);
    long tot = 0;
    for (int it = 0; it < 40; it++) {
        for (int j = 0; j < (1 << 16); j++) gen(j & 3, A1 + j, A2 + j, B1 + j, B2 + j);
        check_batch(A1, A2, B1, B2, 1 << 16, 1); tot += 1 << 16;
    }
    printf("31 bits: %ld random pairs (a quarter each: random, small b, common factor 2^4..2^15, factor 2^15)\n", tot);
    printf("failures %ld, out-of-range values in the model %ld\n", fails, model_bad);
}

/* ======================= step statistics ======================= */
static void do_stats(void) {
    static i32 A1[1 << 16], A2[1 << 16], B1[1 << 16], B2[1 << 16], G1[1 << 16], G2[1 << 16];
    const char *kn[] = {"random", "small b (2^12)", "common factor 2^4..2^15", "large common factor 2^15"};
    printf("per pair: (1+i)-ary steps mean / max; Euclid steps mean / max; per register of 8: max steps mean / max; lane use\n");
    for (int kind = 0; kind < 4; kind++) {
        long hist[300] = {0}; long regsum = 0, gsum[4] = {0}; int regmax = 0, mx = 0, n = 1 << 16, R = 16;
        long esum = 0; int emax = 0; static int st[1 << 16];
        for (int rep = 0; rep < R; rep++) {
            for (int j = 0; j < n; j++) gen(kind, A1 + j, A2 + j, B1 + j, B2 + j);
            for (int j = 0; j < n; j++) {
                i32 g1, g2; int s = pi_model(A1[j], A2[j], B1[j], B2[j], &g1, &g2);
                st[j] = s; hist[s]++; if (s > mx) mx = s;
                long e0 = euclid_steps; euclid1(A1[j], A2[j], B1[j], B2[j], &g1, &g2, 1);
                int es = (int)(euclid_steps - e0); esum += es; if (es > emax) emax = es;
            }
            for (int G = 1; G <= 4; G++)          /* groups of 8G pairs (G registers stepped together) */
                for (int j = 0; j + 8 * G <= n; j += 8 * G) {
                    int rm = 0; for (int l = 0; l < 8 * G; l++) if (st[j + l] > rm) rm = st[j + l];
                    gsum[G - 1] += (long)rm * G;
                    if (G == 1) { regsum += rm; if (rm > regmax) regmax = rm; }
                }
        }
        long tot = (long)n * R, s = 0; for (int i = 0; i < 300; i++) s += i * hist[i];
        double mean = (double)s / tot, rmean = (double)regsum / (tot / 8);
        printf("  %-26s %6.2f / %3d   Euclid %6.2f / %3d   register %6.2f / %3d   lane use %.0f%%\n", kn[kind], mean, mx,
               (double)esum / tot, emax, rmean, regmax, 100 * mean / rmean);
        printf("  %-26s steps per register when G registers go together, G = 1..4: %.2f %.2f %.2f %.2f\n", "",
               (double)gsum[0] / (tot / 8), (double)gsum[1] / (tot / 8), (double)gsum[2] / (tot / 8), (double)gsum[3] / (tot / 8));
    }
    /* the instrumented vector kernel agrees with the model on register-steps */
    for (int j = 0; j < (1 << 16); j++) gen(0, A1 + j, A2 + j, B1 + j, B2 + j);
    long regsum = 0;
    for (int j = 0; j < (1 << 16); j += 8) { int rm = 0; for (int l = 0; l < 8; l++) { i32 g1, g2; int s = pi_model(A1[j + l], A2[j + l], B1[j + l], B2[j + l], &g1, &g2); if (s > rm) rm = s; } regsum += rm; }
    vec_regsteps = 0; pi_v_count(A1, A2, B1, B2, G1, G2, 1 << 16);
    printf("register-steps, model %ld, vector kernel %ld\n", regsum, vec_regsteps);
}

/* ======================= timing ======================= */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31
static void do_time(void) {
    enum { N = 1024 };
    static i32 buf[6][N + 256];
    i32 *p[6]; for (int j = 0; j < 6; j++) p[j] = buf[j] + 16 * j;   /* different offsets */
    static const int kinds[2] = {0, 3};
    static const char *kn[2] = {"random", "large common factor 2^15"};
    static const int inner[NKT] = {16, 8, 16, 64, 64, 4, 64, 64, 64};
    printf("ns per gcd pair, N = %d pairs (24 KB); median [q1, q3] over %d reps; kernels interleaved, order rotated\n", N, REPS);
    for (int c = 0; c < 2; c++) {
        for (int j = 0; j < N; j++) gen(kinds[c], p[0] + j, p[1] + j, p[2] + j, p[3] + j);
        static double t[NKT][REPS];
        for (int k = 0; k < NKT; k++) KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NKT; j++) {
                int k = (j + r) % NKT;
                KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                double t0 = now();
                for (int it = 0; it < inner[k]; it++) KT[k].f(p[0], p[1], p[2], p[3], p[4], p[5], N);
                t[k][r] = (now() - t0) / ((double)N * inner[k]);
            }
        printf("\n%s\n", kn[c]);
        for (int k = 0; k < NKT; k++) {
            qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-7s %8.2f [%8.2f, %8.2f]\n", KT[k].name, t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]);
        }
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else if (!strcmp(mode, "stats")) do_stats();
    else do_time();
    return 0;
}
