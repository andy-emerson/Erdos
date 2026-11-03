/* Study 20: what the shrink test costs, naive against refined.
 *
 * Gaussian integers, layout (b) (separate real and imaginary arrays), AVX2.
 * Two sizes: the base size (int32 halves, 32-bit lanes, shrink target 15 bits:
 * there is nothing smaller, but the naive test runs anyway) and the first grown
 * size (int64 halves, 64-bit lanes, shrink target 31 bits). Rule B: a half fits in
 * g bits iff (v + 2^(g-1)) has nothing at bit g or above (add-offset form; AVX2
 * has no 64-bit arithmetic right shift).
 *
 * Every variant keeps the full-range overflow check with its sticky flag.
 *   bare   arithmetic only (reference, no checks)
 *   v1     arithmetic + overflow check, no shrink test
 *   v2     naive: v1, then a separate pass over the two result arrays, after
 *          every operation (add, sub, mul) at both sizes
 *   v3     refined: shrink test fused into the sweep, only for add and sub, only
 *          above the base size (so v3 = v1 at int32 and for mul)
 * Kernels return bit 0 = some result overflowed, bit 1 = some half does not fit
 * the next size down (meaningful only when bit 0 is clear).
 * Multiply at int32: widening 32x32->64 (vpmuldq) on even and odd lanes, overflow
 * test (v + 2^31) per 64-bit part, low halves packed back. Multiply at int64: AVX2
 * has no 64-bit multiply, so it is scalar (64x64->128 imul, two-word sub/add).
 * N must be a multiple of 8.
 *
 * Build: gcc -O2 -mavx2 -o shrink20 shrink20.c
 * Run:   ./shrink20 check | ./shrink20 time [Nbig] | ./shrink20 time-aligned [Nbig]
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int32_t i32;
typedef int64_t i64;
typedef uint64_t u64;
typedef __int128 i128;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define ARGS(T) const T *restrict ar, const T *restrict ai, const T *restrict br, const T *restrict bi, \
                T *restrict cr, T *restrict ci, size_t N
#define TEST(s, m) (_mm256_testz_si256(s, m) ? 0 : 1)

/* overflow contributions (sign bit of each lane) */
#define OVF_ADD(x, y, r) _mm256_andnot_si256(XOR(x, y), XOR(x, r))
#define OVF_SUB(x, y, r) _mm256_and_si256(XOR(x, y), XOR(x, r))

/* ======================= base size: int32 halves ======================= */
#define SIGN32 _mm256_set1_epi32((int)0x80000000u)
#define SHR32_OFF (1 << 14)                 /* fits in 15 bits */
#define SHR32_MASK ((int)0xFFFF8000u)

#define BARE32(NAME, OP)                                              \
    NOINL int NAME(ARGS(i32)) {                                       \
        for (size_t i = 0; i < N; i += 8) {                           \
            ST(cr + i, OP(LD(ar + i), LD(br + i)));                   \
            ST(ci + i, OP(LD(ai + i), LD(bi + i)));                   \
        }                                                             \
        return 0;                                                     \
    }
BARE32(add32_b, _mm256_add_epi32)
BARE32(sub32_b, _mm256_sub_epi32)

#define OVF32(NAME, OP, OVF)                                                    \
    NOINL int NAME(ARGS(i32)) {                                                 \
        V o = _mm256_setzero_si256();                                           \
        for (size_t i = 0; i < N; i += 8) {                                     \
            V x = LD(ar + i), y = LD(br + i), u = LD(ai + i), v = LD(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                       \
            ST(cr + i, r); ST(ci + i, q);                                       \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                          \
        }                                                                       \
        return TEST(o, SIGN32);                                                 \
    }
OVF32(add32_o, _mm256_add_epi32, OVF_ADD)
OVF32(sub32_o, _mm256_sub_epi32, OVF_SUB)

NOINL int mul32_b(ARGS(i32)) {      /* wrapping: low 32 bits of each product */
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(ar + i), b = LD(ai + i), c = LD(br + i), d = LD(bi + i);
        ST(cr + i, _mm256_sub_epi32(_mm256_mullo_epi32(a, c), _mm256_mullo_epi32(b, d)));
        ST(ci + i, _mm256_add_epi32(_mm256_mullo_epi32(a, d), _mm256_mullo_epi32(b, c)));
    }
    return 0;
}
NOINL int mul32_o(ARGS(i32)) {      /* widening, exact in 64 bits except the corner 2^63, which wraps and is still caught */
    V off = _mm256_set1_epi64x(1LL << 31), o = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(ar + i), b = LD(ai + i), c = LD(br + i), d = LD(bi + i);
        V ao = _mm256_srli_epi64(a, 32), bo = _mm256_srli_epi64(b, 32), co = _mm256_srli_epi64(c, 32), do_ = _mm256_srli_epi64(d, 32);
        V re = _mm256_sub_epi64(_mm256_mul_epi32(a, c), _mm256_mul_epi32(b, d));
        V im = _mm256_add_epi64(_mm256_mul_epi32(a, d), _mm256_mul_epi32(b, c));
        V ro = _mm256_sub_epi64(_mm256_mul_epi32(ao, co), _mm256_mul_epi32(bo, do_));
        V io = _mm256_add_epi64(_mm256_mul_epi32(ao, do_), _mm256_mul_epi32(bo, co));
        o = OR(o, OR(OR(_mm256_add_epi64(re, off), _mm256_add_epi64(im, off)),
                     OR(_mm256_add_epi64(ro, off), _mm256_add_epi64(io, off))));
        ST(cr + i, _mm256_blend_epi32(re, _mm256_slli_epi64(ro, 32), 0xAA));
        ST(ci + i, _mm256_blend_epi32(im, _mm256_slli_epi64(io, 32), 0xAA));
    }
    return TEST(o, _mm256_set1_epi64x((i64)0xFFFFFFFF00000000ULL));
}

NOINL int pass32(const i32 *restrict cr, const i32 *restrict ci, size_t N) {
    V off = _mm256_set1_epi32(SHR32_OFF), s = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 8)
        s = OR(s, OR(_mm256_add_epi32(LD(cr + i), off), _mm256_add_epi32(LD(ci + i), off)));
    return TEST(s, _mm256_set1_epi32(SHR32_MASK));
}

/* ======================= first grown size: int64 halves ======================= */
#define SIGN64 _mm256_set1_epi64x((i64)0x8000000000000000ULL)
#define SHR64_OFF (1LL << 30)               /* fits in 31 bits */
#define SHR64_MASK ((i64)0xFFFFFFFF80000000ULL)

#define BARE64(NAME, OP)                                              \
    NOINL int NAME(ARGS(i64)) {                                       \
        for (size_t i = 0; i < N; i += 4) {                           \
            ST(cr + i, OP(LD(ar + i), LD(br + i)));                   \
            ST(ci + i, OP(LD(ai + i), LD(bi + i)));                   \
        }                                                             \
        return 0;                                                     \
    }
BARE64(add64_b, _mm256_add_epi64)
BARE64(sub64_b, _mm256_sub_epi64)

#define OVF64(NAME, OP, OVF)                                                    \
    NOINL int NAME(ARGS(i64)) {                                                 \
        V o = _mm256_setzero_si256();                                           \
        for (size_t i = 0; i < N; i += 4) {                                     \
            V x = LD(ar + i), y = LD(br + i), u = LD(ai + i), v = LD(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                       \
            ST(cr + i, r); ST(ci + i, q);                                       \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                          \
        }                                                                       \
        return TEST(o, SIGN64);                                                 \
    }
OVF64(add64_o, _mm256_add_epi64, OVF_ADD)
OVF64(sub64_o, _mm256_sub_epi64, OVF_SUB)

#define FUSED64(NAME, OP, OVF)                                                  \
    NOINL int NAME(ARGS(i64)) {                                                 \
        V off = _mm256_set1_epi64x(SHR64_OFF), s = _mm256_setzero_si256(),      \
          o = _mm256_setzero_si256();                                           \
        for (size_t i = 0; i < N; i += 4) {                                     \
            V x = LD(ar + i), y = LD(br + i), u = LD(ai + i), v = LD(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                       \
            ST(cr + i, r); ST(ci + i, q);                                       \
            s = OR(s, OR(_mm256_add_epi64(r, off), _mm256_add_epi64(q, off)));  \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                          \
        }                                                                       \
        return TEST(o, SIGN64) | TEST(s, _mm256_set1_epi64x(SHR64_MASK)) << 1;  \
    }
FUSED64(add64_f, _mm256_add_epi64, OVF_ADD)
FUSED64(sub64_f, _mm256_sub_epi64, OVF_SUB)

/* scalar: AVX2 has no 64-bit lane multiply */
__attribute__((optimize("no-tree-vectorize")))
NOINL int mul64_b(ARGS(i64)) {
    for (size_t i = 0; i < N; i++) {
        u64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        cr[i] = (i64)(a * c - b * d);
        ci[i] = (i64)(a * d + b * c);
    }
    return 0;
}
__attribute__((optimize("no-tree-vectorize")))
NOINL int mul64_o(ARGS(i64)) {
    u64 o = 0;
    for (size_t i = 0; i < N; i++) {
        i64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        i128 re = (i128)a * c - (i128)b * d;            /* exact */
        i128 im = (i128)((unsigned __int128)((i128)a * d) + (unsigned __int128)((i128)b * c));   /* exact except 2^127 at the corner, which wraps and is caught */
        i64 rl = (i64)re, il = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl >> 63)) | ((u64)(im >> 64) ^ (u64)(il >> 63));   /* high word = sign of low word */
        cr[i] = rl; ci[i] = il;
    }
    return o != 0;
}

NOINL int pass64(const i64 *restrict cr, const i64 *restrict ci, size_t N) {
    V off = _mm256_set1_epi64x(SHR64_OFF), s = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 4)
        s = OR(s, OR(_mm256_add_epi64(LD(cr + i), off), _mm256_add_epi64(LD(ci + i), off)));
    return TEST(s, _mm256_set1_epi64x(SHR64_MASK));
}

/* naive variant: the operation with its overflow check, then the separate pass */
#define NAIVE(NAME, T, OPO, PASS) \
    NOINL int NAME(ARGS(T)) { int f = OPO(ar, ai, br, bi, cr, ci, N); return f | PASS(cr, ci, N) << 1; }
NAIVE(add32_n, i32, add32_o, pass32)
NAIVE(sub32_n, i32, sub32_o, pass32)
NAIVE(mul32_n, i32, mul32_o, pass32)
NAIVE(add64_n, i64, add64_o, pass64)
NAIVE(sub64_n, i64, sub64_o, pass64)
NAIVE(mul64_n, i64, mul64_o, pass64)

/* ======================= kernel table ======================= */
typedef int (*k32_t)(ARGS(i32));
typedef int (*k64_t)(ARGS(i64));
enum { BARE, V1, V2, V3 };
typedef struct { const char *name; int w, op, var; void *f; } kern;
/* op: 0 add, 1 sub, 2 mul. V3 entries exist only where they differ from V1 */
static kern KT[] = {
    {"add32 bare", 32, 0, BARE, add32_b}, {"add32 v1", 32, 0, V1, add32_o}, {"add32 v2", 32, 0, V2, add32_n},
    {"sub32 bare", 32, 1, BARE, sub32_b}, {"sub32 v1", 32, 1, V1, sub32_o}, {"sub32 v2", 32, 1, V2, sub32_n},
    {"mul32 bare", 32, 2, BARE, mul32_b}, {"mul32 v1", 32, 2, V1, mul32_o}, {"mul32 v2", 32, 2, V2, mul32_n},
    {"add64 bare", 64, 0, BARE, add64_b}, {"add64 v1", 64, 0, V1, add64_o}, {"add64 v2", 64, 0, V2, add64_n},
    {"add64 v3", 64, 0, V3, add64_f},
    {"sub64 bare", 64, 1, BARE, sub64_b}, {"sub64 v1", 64, 1, V1, sub64_o}, {"sub64 v2", 64, 1, V2, sub64_n},
    {"sub64 v3", 64, 1, V3, sub64_f},
    {"mul64 bare", 64, 2, BARE, mul64_b}, {"mul64 v1", 64, 2, V1, mul64_o}, {"mul64 v2", 64, 2, V2, mul64_n},
};
#define NKT (int)(sizeof KT / sizeof KT[0])
static int callk(int k, void *ar, void *ai, void *br, void *bi, void *cr, void *ci, size_t N) {
    if (KT[k].w == 32) return ((k32_t)KT[k].f)(ar, ai, br, bi, cr, ci, N);
    return ((k64_t)KT[k].f)(ar, ai, br, bi, cr, ci, N);
}

/* ======================= random ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

static i64 E32[64], E64[64]; static int NE32, NE64;
static void edges(i64 *E, int *NE, const int *bits, int nb, int w) {
    i128 lo = -((i128)1 << (w - 1)), hi = ((i128)1 << (w - 1)) - 1;
    E[(*NE)++] = 0; E[(*NE)++] = 1; E[(*NE)++] = -1;
    for (int k = 0; k < nb; k++) {
        i128 p = (i128)1 << bits[k];
        i128 c[] = {p - 2, p - 1, p, p + 1, -p - 1, -p, -p + 1, -p + 2};
        for (int j = 0; j < 8; j++) if (c[j] <= hi && c[j] >= lo) E[(*NE)++] = (i64)c[j];
    }
}
/* an edge, an edge +-4 (wrapping), or a random value of random bit length (up to w bits with chance big %) */
static i64 rval(const i64 *E, int NE, int w, int small, int big) {
    u64 k = next() % 100;
    i64 v;
    if (k < 20) v = E[next() % NE];
    else if (k < 30) v = (i64)((u64)E[next() % NE] + (next() % 9) - 4);
    else {
        int b = (k < 30 + (u64)big) ? (int)(next() % w) : (int)(next() % (u64)(small + 1));
        v = b ? (i64)(next() >> (64 - b)) : 0;
        if (next() & 1) v = -v - (i64)(next() & 1);
    }
    return w == 32 ? (i64)(i32)v : v;
}

/* ======================= correctness ======================= */
static long fails, cases;
static int fitw(i128 v, int w) { return v >= -((i128)1 << (w - 1)) && v < ((i128)1 << (w - 1)); }
/* exact a + b fits in w bits, for |a|, |b| <= 2^126 (a + b itself may not fit in i128) */
static int fitsum(i128 a, i128 b, int w) {
    i128 lo = -((i128)1 << (w - 1)), hi = ((i128)1 << (w - 1)) - 1;
    return b >= lo - a && b <= hi - a;
}
static long seen[64][4];

static void check_one(int k, const i64 *Ar, const i64 *Ai, const i64 *Br, const i64 *Bi, size_t N) {
    static i32 a32[4][256], c32[2][256];
    static i64 c64[2][256];
    int w = KT[k].w, op = KT[k].op, var = KT[k].var, g = w == 32 ? 15 : 31;
    int f;
    if (w == 32) {
        for (size_t j = 0; j < N; j++) { a32[0][j] = (i32)Ar[j]; a32[1][j] = (i32)Ai[j]; a32[2][j] = (i32)Br[j]; a32[3][j] = (i32)Bi[j]; }
        f = callk(k, a32[0], a32[1], a32[2], a32[3], c32[0], c32[1], N);
    } else f = callk(k, (void *)Ar, (void *)Ai, (void *)Br, (void *)Bi, c64[0], c64[1], N);
    int ovf = 0, nofit = 0;
    for (size_t j = 0; j < N; j++) {
        i128 a = Ar[j], b = Ai[j], c = Br[j], d = Bi[j];
        u64 er, ei; int fr, fi;
        if (op == 0) { fr = fitw(a + c, w); fi = fitw(b + d, w); er = (u64)(a + c); ei = (u64)(b + d); }
        else if (op == 1) { fr = fitw(a - c, w); fi = fitw(b - d, w); er = (u64)(a - c); ei = (u64)(b - d); }
        else { fr = fitsum(a * c, -(b * d), w); fi = fitsum(a * d, b * c, w);
               er = (u64)(a * c) - (u64)(b * d); ei = (u64)(a * d) + (u64)(b * c); }
        /* er, ei: the exact result mod 2^64; when it fits in w bits, (i64)er is the exact value */
        if (!fr || !fi) ovf = 1;
        else if (!fitw((i64)er, g) || !fitw((i64)ei, g)) nofit = 1;
        u64 gr, gi;
        if (w == 32) { er = (uint32_t)er; ei = (uint32_t)ei; gr = (uint32_t)c32[0][j]; gi = (uint32_t)c32[1][j]; }
        else { gr = (u64)c64[0][j]; gi = (u64)c64[1][j]; }
        if (gr != er || gi != ei) if (fails++ < 10) printf("FAIL value %s j=%zu\n", KT[k].name, j);
    }
    cases++;
    if (var == BARE) { seen[k][0]++; return; }
    if (var == V1) {
        if (f != ovf) if (fails++ < 10) printf("FAIL ovf %s got %d want %d\n", KT[k].name, f, ovf);
        seen[k][ovf ? 2 : 0]++;
        return;
    }
    if (ovf) {   /* the shrink bit describes wrapped results: meaningless, the operation is redone wider */
        if (!(f & 1)) if (fails++ < 10) printf("FAIL ovf missed %s\n", KT[k].name);
        seen[k][f & 2 ? 3 : 2]++;
        return;
    }
    int want = nofit << 1;
    if (f != want) if (fails++ < 10) printf("FAIL flags %s got %d want %d\n", KT[k].name, f, want);
    seen[k][nofit]++;
}

static void do_check(void) {
    const int b32[] = {14, 15, 30, 31}, b64[] = {30, 31, 62, 63};
    edges(E32, &NE32, b32, 4, 32); edges(E64, &NE64, b64, 4, 64);
    printf("edge values: %d at int32, %d at int64\n", NE32, NE64);
    static i64 ar[256], ai[256], br[256], bi[256];
    long cnt = 0;
    for (int w = 32; w <= 64; w += 32) {
        const i64 *E = w == 32 ? E32 : E64; int NE = w == 32 ? NE32 : NE64;
        /* every edge quadruple, one element alone among 8, position rotating */
        for (int a = 0; a < NE; a++) for (int b = 0; b < NE; b++)
            for (int c = 0; c < NE; c++) for (int d = 0; d < NE; d++) {
                memset(ar, 0, 64); memset(ai, 0, 64); memset(br, 0, 64); memset(bi, 0, 64);
                int l = cnt++ & 7;
                ar[l] = E[a]; ai[l] = E[b]; br[l] = E[c]; bi[l] = E[d];
                for (int k = 0; k < NKT; k++) if (KT[k].w == w) check_one(k, ar, ai, br, bi, 8);
            }
    }
    printf("edge quadruples: %ld\n", cnt);
    long rnd = 0;
    for (int it = 0; it < 400000; it++) {
        int w = it & 1 ? 64 : 32;
        const i64 *E = w == 32 ? E32 : E64; int NE = w == 32 ? NE32 : NE64;
        size_t N = 8 * (1 + next() % 16);
        int big = (int)(next() % 3) == 0 ? 0 : (int)(next() % 4);
        static const int sm32[] = {6, 12, 14}, sm64[] = {14, 28, 30};
        int small = w == 32 ? sm32[next() % 3] : sm64[next() % 3];
        if (it % 4 < 2) {
            for (size_t j = 0; j < N; j++) { ar[j] = rval(E, NE, w, small, big); ai[j] = rval(E, NE, w, small, big);
                                             br[j] = rval(E, NE, w, small, big); bi[j] = rval(E, NE, w, small, big); }
        } else {   /* all small values of one width, sometimes one value planted from the general mix */
            i64 *t[] = {ar, ai, br, bi};
            for (int q = 0; q < 4; q++) for (size_t j = 0; j < N; j++)
                t[q][j] = (i64)(next() % (2ULL << small)) - (1LL << small);
            if (next() & 1) t[next() % 4][next() % N] = rval(E, NE, w, small, 50);
        }
        for (int k = 0; k < NKT; k++) if (KT[k].w == w) check_one(k, ar, ai, br, bi, N);
        rnd++;
    }
    printf("random arrays: %ld (length 8..128)\n", rnd);
    printf("cases %ld, failures %ld\n", cases, fails);
    printf("outcomes: no ovf & all fit / no ovf & some don't / ovf, shrink bit clear / ovf, shrink bit set\n");
    for (int k = 0; k < NKT; k++) printf("  %-11s %9ld %9ld %9ld %9ld\n", KT[k].name, seen[k][0], seen[k][1], seen[k][2], seen[k][3]);
}

/* ======================= timing ======================= */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31
static char *buf[6];
static volatile int sink;
static int stagger = 1;

static void do_time(size_t bigN) {
    size_t sz = bigN * sizeof(i64);
    /* each array page-aligned, then shifted by j * 576 bytes so the six streams differ in address bits 0..11 */
    for (int j = 0; j < 6; j++) buf[j] = (char *)aligned_alloc(4096, sz + 4096) + (stagger ? 576 * j : 0);
    const size_t sizes[3] = {512, 16384, bigN}, inner[3] = {4096, 128, 1};
    static const char *pn[3] = {"L1", "L2", "memory"};
    printf("ns per element (one Gaussian integer); median [q1, q3] over %d reps; kernels of one width interleaved, order rotated\n", REPS);
    static double med[3][64];
    for (int p = 0; p < 3; p++)
        for (int w = 32; w <= 64; w += 32) {
            size_t N = sizes[p];
            for (int j = 0; j < 4; j++)      /* operands: |v| < 2^13 at int32, < 2^28 at int64: nothing overflows */
                for (size_t i = 0; i < N; i++) {
                    if (w == 32) ((i32 *)buf[j])[i] = (i32)(next() % (1u << 14)) - (1 << 13);
                    else ((i64 *)buf[j])[i] = (i64)(next() % (1u << 29)) - (1 << 28);
                }
            memset(buf[4], 0, N * w / 8); memset(buf[5], 0, N * w / 8);
            int ks[32], nk = 0;
            for (int k = 0; k < NKT; k++) if (KT[k].w == w) ks[nk++] = k;
            static double t[32][REPS];
#define CALL(k) callk(k, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], N)
            for (int j = 0; j < nk; j++) sink += CALL(ks[j]);
            for (int r = 0; r < REPS; r++)
                for (int j = 0; j < nk; j++) {
                    int jj = (j + r) % nk, k = ks[jj];
                    sink += CALL(k);
                    double t0 = now();
                    for (size_t it = 0; it < inner[p]; it++) sink += CALL(k);
                    t[jj][r] = (now() - t0) / (double)(N * inner[p]);
                }
            printf("\n%s, int%d halves, N = %zu (%zu KB per array)\n", pn[p], w, N, N * w / 8 / 1024);
            for (int j = 0; j < nk; j++) {
                qsort(t[j], REPS, sizeof(double), cmpd);
                med[p][ks[j]] = t[j][REPS / 2];
                printf("  %-11s %.4f [%.4f, %.4f]\n", KT[ks[j]].name, t[j][REPS / 2], t[j][REPS / 4], t[j][3 * REPS / 4]);
            }
            fflush(stdout);
        }
    /* variant totals and the mix: equal numbers of add, sub, mul, half the arrays at each width */
    printf("\nmedians by operation and variant (v3 = v1 where there is no fused kernel); mix = mean of the 6 cells\n");
    for (int p = 0; p < 3; p++) {
        printf("%s\n  %-8s %8s %8s %8s %8s\n", pn[p], "", "bare", "v1", "v2", "v3");
        double mix[4] = {0};
        for (int w = 32; w <= 64; w += 32)
            for (int op = 0; op < 3; op++) {
                double v[4] = {0};
                for (int k = 0; k < NKT; k++) if (KT[k].w == w && KT[k].op == op) v[KT[k].var] = med[p][k];
                if (v[V3] == 0) v[V3] = v[V1];
                printf("  %s%d    %8.4f %8.4f %8.4f %8.4f\n", op == 0 ? "add" : op == 1 ? "sub" : "mul", w, v[0], v[1], v[2], v[3]);
                for (int q = 0; q < 4; q++) mix[q] += v[q] / 6;
            }
        printf("  %-8s %8.4f %8.4f %8.4f %8.4f   (v2 %+.1f%%, v3 %+.1f%% over v1)\n", "mix", mix[0], mix[1], mix[2], mix[3],
               100 * (mix[2] / mix[1] - 1), 100 * (mix[3] / mix[1] - 1));
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else { stagger = strcmp(mode, "time-aligned") != 0;
           do_time(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 25); }
    return 0;
}
