/* Study 21: what the shrink test saves over a whole run.
 *
 * Arrays of Gaussian integers, layout (b), AVX2. The state array starts at int32
 * halves; operands are int32 arrays. Overflow checks with a sticky flag are always
 * on. Real promotion: on overflow at int32 the state is copied up to int64 halves
 * and the operation is redone there from the untouched input. At int64 the
 * operand is sign-extended on load (vpmovsxdq), so it is never copied up.
 * Real demotion: when the shrink test allows, the int64 state is copied down.
 *
 *   v1  no shrink test: once grown, stays at int64.
 *   v2  naive: a separate pass over the result after every operation (at both
 *       sizes; at int32 there is nothing to shrink to), demote when it allows.
 *   v3  refined: test fused into add and sub, only at int64, demote at the end
 *       of the sweep when it allows.
 *   v3A as v3 with rule A (fits in 32 bits, no margin): hover workload only.
 * Rule B: demote when every half fits in 31 bits.
 *
 * Workloads, each 2 + L operations:
 *   back     add P (promotes), sub P (values back under the rule B line), then L ops
 *            cycling add S, sub T, mul U (S, T small, U a unit: no further overflow)
 *   control  add P (promotes), sub S (values stay above the line), then the same L ops
 *   hover    values just under 2^31: add D, sub D, ... alternating (2 + L ops)
 *
 * Build: gcc -O2 -mavx2 -o shrink21 shrink21.c
 * Run:   ./shrink21 check | ./shrink21 time [Nmem]
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
#define LDW(p) _mm256_cvtepi32_epi64(_mm_loadu_si128((const __m128i *)(p)))   /* 4 int32 -> 4 int64 */
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define TEST(s, m) (_mm256_testz_si256(s, m) ? 0 : 1)
#define OVF_ADD(x, y, r) _mm256_andnot_si256(XOR(x, y), XOR(x, r))
#define OVF_SUB(x, y, r) _mm256_and_si256(XOR(x, y), XOR(x, r))

#define ARGS32 const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
               i32 *restrict cr, i32 *restrict ci, size_t N
#define ARGS64 const i64 *restrict ar, const i64 *restrict ai, const i32 *restrict br, const i32 *restrict bi, \
               i64 *restrict cr, i64 *restrict ci, size_t N

/* ---------- int32 state, int32 operand ---------- */
#define OVF32(NAME, OP, OVF)                                                    \
    NOINL int NAME(ARGS32) {                                                    \
        V o = _mm256_setzero_si256();                                           \
        for (size_t i = 0; i < N; i += 8) {                                     \
            V x = LD(ar + i), y = LD(br + i), u = LD(ai + i), v = LD(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                       \
            ST(cr + i, r); ST(ci + i, q);                                       \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                          \
        }                                                                       \
        return TEST(o, _mm256_set1_epi32((int)0x80000000u));                    \
    }
OVF32(add32, _mm256_add_epi32, OVF_ADD)
OVF32(sub32, _mm256_sub_epi32, OVF_SUB)

NOINL int mul32(ARGS32) {
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

NOINL int pass32(const i32 *restrict cr, const i32 *restrict ci, size_t N) {   /* 15-bit test; nothing smaller exists */
    V off = _mm256_set1_epi32(1 << 14), s = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 8)
        s = OR(s, OR(_mm256_add_epi32(LD(cr + i), off), _mm256_add_epi32(LD(ci + i), off)));
    return TEST(s, _mm256_set1_epi32((int)0xFFFF8000u));
}

/* ---------- int64 state, int32 operand (sign-extended on load) ---------- */
#define SIGN64 _mm256_set1_epi64x((i64)0x8000000000000000ULL)
#define OVF64(NAME, OP, OVF)                                                      \
    NOINL int NAME(ARGS64) {                                                      \
        V o = _mm256_setzero_si256();                                             \
        for (size_t i = 0; i < N; i += 4) {                                       \
            V x = LD(ar + i), y = LDW(br + i), u = LD(ai + i), v = LDW(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                         \
            ST(cr + i, r); ST(ci + i, q);                                         \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                            \
        }                                                                         \
        return TEST(o, SIGN64);                                                   \
    }
OVF64(add64, _mm256_add_epi64, OVF_ADD)
OVF64(sub64, _mm256_sub_epi64, OVF_SUB)

/* fused shrink test; off/mask: rule B 2^30 / bits 31..63, rule A 2^31 / bits 32..63 */
#define FUSED64(NAME, OP, OVF)                                                    \
    NOINL int NAME(ARGS64, i64 offv, i64 maskv) {                                 \
        V off = _mm256_set1_epi64x(offv), s = _mm256_setzero_si256(),             \
          o = _mm256_setzero_si256();                                             \
        for (size_t i = 0; i < N; i += 4) {                                       \
            V x = LD(ar + i), y = LDW(br + i), u = LD(ai + i), v = LDW(bi + i);   \
            V r = OP(x, y), q = OP(u, v);                                         \
            ST(cr + i, r); ST(ci + i, q);                                         \
            s = OR(s, OR(_mm256_add_epi64(r, off), _mm256_add_epi64(q, off)));    \
            o = OR(o, OR(OVF(x, y, r), OVF(u, v, q)));                            \
        }                                                                         \
        return TEST(o, SIGN64) | TEST(s, _mm256_set1_epi64x(maskv)) << 1;         \
    }
FUSED64(add64f, _mm256_add_epi64, OVF_ADD)
FUSED64(sub64f, _mm256_sub_epi64, OVF_SUB)

__attribute__((optimize("no-tree-vectorize")))
NOINL int mul64(ARGS64) {   /* scalar: AVX2 has no 64-bit lane multiply */
    u64 o = 0;
    for (size_t i = 0; i < N; i++) {
        i64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        i128 re = (i128)a * c - (i128)b * d, im = (i128)a * d + (i128)b * c;   /* |.| < 2^96: exact */
        i64 rl = (i64)re, il = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl >> 63)) | ((u64)(im >> 64) ^ (u64)(il >> 63));
        cr[i] = rl; ci[i] = il;
    }
    return o != 0;
}

NOINL int pass64(const i64 *restrict cr, const i64 *restrict ci, size_t N, i64 offv, i64 maskv) {
    V off = _mm256_set1_epi64x(offv), s = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 4)
        s = OR(s, OR(_mm256_add_epi64(LD(cr + i), off), _mm256_add_epi64(LD(ci + i), off)));
    return TEST(s, _mm256_set1_epi64x(maskv));
}

/* ---------- copies between sizes ---------- */
NOINL void widen(const i32 *restrict r, const i32 *restrict im, i64 *restrict R, i64 *restrict I, size_t N) {
    for (size_t i = 0; i < N; i += 4) { ST(R + i, LDW(r + i)); ST(I + i, LDW(im + i)); }
}
NOINL void narrow(const i64 *restrict R, const i64 *restrict I, i32 *restrict r, i32 *restrict im, size_t N) {
    const V idx = _mm256_setr_epi32(0, 2, 4, 6, 1, 3, 5, 7);
    for (size_t i = 0; i < N; i += 8) {
        V a = _mm256_permutevar8x32_epi32(LD(R + i), idx), b = _mm256_permutevar8x32_epi32(LD(R + i + 4), idx);
        ST(r + i, _mm256_permute2x128_si256(a, b, 0x20));
        V c = _mm256_permutevar8x32_epi32(LD(I + i), idx), d = _mm256_permutevar8x32_epi32(LD(I + i + 4), idx);
        ST(im + i, _mm256_permute2x128_si256(c, d, 0x20));
    }
}

/* ---------- the run ---------- */
enum { ADD, SUB, MUL };
typedef struct { int op, y; } Op;
typedef struct { i32 *r, *i; } A32;
typedef struct { i64 *r, *i; } A64;
static A32 X0, Y[8], B32[2];
static A64 B64[2];
static volatile int sink;
typedef struct { int w; const void *r, *i; int prom, dem; } Res;

#define RB_OFF (1LL << 30)
#define RB_MASK ((i64)0xFFFFFFFF80000000ULL)
#define RA_OFF (1LL << 31)
#define RA_MASK ((i64)0xFFFFFFFF00000000ULL)

/* var: 1, 2, 3; ruleA: use rule A in v3 */
static Res run(const Op *ops, int nops, int var, int ruleA, size_t N) {
    i64 off = ruleA ? RA_OFF : RB_OFF, mask = ruleA ? RA_MASK : RB_MASK;
    int w = 32, prom = 0, dem = 0;
    const i32 *xr = X0.r, *xi = X0.i;        /* the input is never written */
    const i64 *Xr = 0, *Xi = 0;
    for (int k = 0; k < nops; k++) {
        int op = ops[k].op; const i32 *yr = Y[ops[k].y].r, *yi = Y[ops[k].y].i;
        int f = 0, at64 = 0;
        if (w == 32) {
            A32 z = (xr == B32[0].r) ? B32[1] : B32[0];
            f = (op == ADD ? add32 : op == SUB ? sub32 : mul32)(xr, xi, yr, yi, z.r, z.i, N);
            if (!(f & 1)) {
                xr = z.r; xi = z.i;
                if (var == 2) sink += pass32(xr, xi, N);
                continue;
            }
            widen(xr, xi, B64[0].r, B64[0].i, N);          /* promote: copy up, redo from the untouched input */
            Xr = B64[0].r; Xi = B64[0].i; w = 64; prom++;
        }
        A64 z = (Xr == B64[0].r) ? B64[1] : B64[0];
        if (var == 3 && op != MUL) f = (op == ADD ? add64f : sub64f)(Xr, Xi, yr, yi, z.r, z.i, N, off, mask);
        else f = (op == ADD ? add64 : op == SUB ? sub64 : mul64)(Xr, Xi, yr, yi, z.r, z.i, N);
        if (f & 1) { fprintf(stderr, "overflow at int64: out of scope\n"); exit(1); }
        Xr = z.r; Xi = z.i; at64 = 1;
        int fits = 0;
        if (var == 2) fits = !pass64(Xr, Xi, N, off, mask);
        else if (var == 3 && op != MUL) fits = !(f & 2);
        if (at64 && fits) {
            narrow(Xr, Xi, B32[0].r, B32[0].i, N);         /* demote: copy down */
            xr = B32[0].r; xi = B32[0].i; w = 32; dem++;
        }
    }
    Res R = {w, w == 32 ? (const void *)xr : (const void *)Xr, w == 32 ? (const void *)xi : (const void *)Xi, prom, dem};
    return R;
}

/* ---------- data and workloads ---------- */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i32 urand(i64 lo, i64 hi) { return (i32)(lo + (i64)(next() % (u64)(hi - lo + 1))); }
static void *xalloc(size_t n, int j) { return (char *)aligned_alloc(4096, n + 4096) + 576 * (j % 7); }

enum { YP, YS, YT, YU, YD };   /* operand arrays */
static A32 X0b, X0h;           /* start values for back/control and for hover */
static void make_data(size_t N) {
    int j = 0;
    X0b.r = xalloc(N * 4, j++); X0b.i = xalloc(N * 4, j++); X0h.r = xalloc(N * 4, j++); X0h.i = xalloc(N * 4, j++);
    for (int k = 0; k < 5; k++) { Y[k].r = xalloc(N * 4, j++); Y[k].i = xalloc(N * 4, j++); }
    for (int k = 0; k < 2; k++) { B32[k].r = xalloc(N * 4, j++); B32[k].i = xalloc(N * 4, j++);
                                  B64[k].r = xalloc(N * 8, j++); B64[k].i = xalloc(N * 8, j++); }
    for (size_t i = 0; i < N; i++) {
        X0b.r[i] = urand(-(1 << 28), 1 << 28); X0b.i[i] = urand(-(1 << 28), 1 << 28);
        Y[YP].r[i] = urand((1LL << 31) - (1 << 28), (1LL << 31) - 1); Y[YP].i[i] = urand((1LL << 31) - (1 << 28), (1LL << 31) - 1);
        Y[YS].r[i] = urand(-1024, 1024); Y[YS].i[i] = urand(-1024, 1024);
        Y[YT].r[i] = urand(-1024, 1024); Y[YT].i[i] = urand(-1024, 1024);
        static const i32 ur[4] = {1, -1, 0, 0}, ui[4] = {0, 0, 1, -1};
        int u = (int)(next() % 4); Y[YU].r[i] = ur[u]; Y[YU].i[i] = ui[u];
        X0h.r[i] = urand((1LL << 31) - 2048, (1LL << 31) - 1025); X0h.i[i] = urand((1LL << 31) - 2048, (1LL << 31) - 1025);
        Y[YD].r[i] = urand(2048, 3072); Y[YD].i[i] = urand(2048, 3072);   /* X0h + D >= 2^31 always: every add overflows int32 */
    }
}
enum { BACK, CONTROL, HOVER };
static const char *wname[3] = {"back", "control", "hover"};
static int make_ops(int wl, int L, Op *ops) {
    int n = 0;
    if (wl == HOVER) { for (int k = 0; k < 2 + L; k++) ops[n++] = (Op){k % 2 ? SUB : ADD, YD}; return n; }
    ops[n++] = (Op){ADD, YP};
    ops[n++] = wl == BACK ? (Op){SUB, YP} : (Op){SUB, YS};
    for (int k = 0; k < L; k++) ops[n++] = k % 3 == 0 ? (Op){ADD, YS} : k % 3 == 1 ? (Op){SUB, YT} : (Op){MUL, YU};
    return n;
}

/* ---------- correctness ---------- */
static long fails;
static void check_cfg(int wl, int L, size_t N) {
    Op ops[200]; int n = make_ops(wl, L, ops);
    X0 = wl == HOVER ? X0h : X0b;
    static i64 er[1 << 16], ei[1 << 16];
    size_t step = N > (1 << 16) ? N / (1 << 16) : 1, M = 0;   /* at large N, the exact reference on a sample of elements */
    for (size_t i = 0; i < N; i += step, M++) {
        i128 a = X0.r[i], b = X0.i[i];
        for (int k = 0; k < n; k++) {
            i128 c = Y[ops[k].y].r[i], d = Y[ops[k].y].i[i];
            if (ops[k].op == ADD) { a += c; b += d; }
            else if (ops[k].op == SUB) { a -= c; b -= d; }
            else { i128 t = a * c - b * d; b = a * d + b * c; a = t; }
        }
        er[M] = (i64)a; ei[M] = (i64)b;
    }
    int nv = wl == HOVER ? 4 : 3;
    printf("  %-7s L=%3d:", wname[wl], L);
    for (int v = 0; v < nv; v++) {
        Res R = run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
        size_t m = 0;
        for (size_t i = 0; i < N; i += step, m++) {
            i64 gr = R.w == 32 ? ((const i32 *)R.r)[i] : ((const i64 *)R.r)[i];
            i64 gi = R.w == 32 ? ((const i32 *)R.i)[i] : ((const i64 *)R.i)[i];
            if (gr != er[m] || gi != ei[m]) { if (fails++ < 10) printf("\nFAIL %s L=%d v%d i=%zu\n", wname[wl], L, v + 1, i); break; }
        }
        printf("  v%d%s end int%d, %d up / %d down", v < 3 ? v + 1 : 3, v == 3 ? "A" : "", R.w, R.prom, R.dem);
    }
    printf("\n");
}
static const int Ls[] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
#define NL 9
static void do_check(void) {
    size_t Ns[2] = {1024, (size_t)1 << 20};
    for (int s = 0; s < 2; s++) {
        make_data(Ns[s]);
        printf("N = %zu\n", Ns[s]);
        for (int wl = 0; wl < 3; wl++) for (int l = 0; l < NL; l++) check_cfg(wl, Ls[l], Ns[s]);
    }
    printf("failures: %ld\n", fails);
}

/* ---------- timing ---------- */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

static void do_time(size_t Nmem) {
    size_t Ns[2] = {1024, Nmem};
    int reps[2] = {31, 7};
    for (int s = 0; s < 2; s++) {
        size_t N = Ns[s];
        make_data(N);
        printf("\n%s: N = %zu (int64 state %zu KB per array); median [q1, q3] over %d reps, ns per element per run\n",
               s ? "memory" : "in cache", N, N * 8 / 1024, reps[s]);
        for (int wl = 0; wl < 3; wl++) {
            X0 = wl == HOVER ? X0h : X0b;
            int nv = wl == HOVER ? 4 : 3;
            printf("%s\n  %4s %22s %22s %22s%s   saved v2, v3%s\n", wname[wl], "L", "v1", "v2", "v3",
                   wl == HOVER ? "               v3A (rule A)" : "", wl == HOVER ? ", v3A" : "");
            for (int l = 0; l < NL; l++) {
                Op ops[200]; int n = make_ops(wl, Ls[l], ops);
                /* in cache: repeat runs to fill about 0.3 ms per sample */
                size_t inner = s ? 1 : (size_t)(300000.0 / ((n + 2) * N * 0.4)) + 1;
                static double t[4][64];
                for (int v = 0; v < nv; v++) run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
                for (int r = 0; r < reps[s]; r++)
                    for (int j = 0; j < nv; j++) {
                        int v = (j + r) % nv;
                        if (!s) run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
                        double t0 = now();
                        for (size_t it = 0; it < inner; it++) sink += run(ops, n, v < 3 ? v + 1 : 3, v == 3, N).w;
                        t[v][r] = (now() - t0) / (double)(N * inner);
                    }
                double med[4];
                printf("  %4d", Ls[l]);
                for (int v = 0; v < nv; v++) {
                    qsort(t[v], reps[s], sizeof(double), cmpd);
                    med[v] = t[v][reps[s] / 2];
                    char b[64]; snprintf(b, sizeof b, "%.3f [%.3f,%.3f]", med[v], t[v][reps[s] / 4], t[v][3 * reps[s] / 4]);
                    printf(" %22s", b);
                }
                printf("   %+.3f %+.3f", med[0] - med[1], med[0] - med[2]);
                if (nv == 4) printf(" %+.3f", med[0] - med[3]);
                printf("\n");
                fflush(stdout);
            }
        }
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else do_time(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 23);
    return 0;
}
