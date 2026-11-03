/* Study 23: the Gaussian multiply at the first grown size (int64 halves, layout (b))
 * in AVX2 lanes, which have no 64-bit multiply.
 *
 * re = ac - bd, im = ad + bc. Each kernel writes the low 64 bits of re and im
 * (exact when they fit) and returns 1 when some true re or im does not fit int64
 * (the full-range sticky flag, tested once per array).
 *
 *   s_bare   scalar, four wrapping imul (study 20)
 *   s_chk    scalar, four 64x64->128 imul, high word = sign of low word (study 20)
 *   v_bare   AVX2, wrapping: per real product lo(a)lo(c) + (hi(a)lo(c) + lo(a)hi(c)) << 32,
 *            12 vpmuludq per 4 elements
 *   v_uns    AVX2, unsigned 32-bit pieces (vpmuludq only), 16 multiplies
 *   v_sgn    AVX2, signed 32-bit pieces (vpmuldq only), 16 multiplies
 *
 * The overflow test in both checked vector kernels: let k = floor((re + 2^63) / 2^64).
 * The result fits iff k = 0. |re| <= 2^127, so |k| <= 2^63, and k may be computed
 * mod 2^64 (wrapping lanes): k = 0 mod 2^64 iff k = 0. So the 2^64 column needs no
 * exact arithmetic, only the 2^32 column does (its carry into k).
 *
 * v_sgn pieces: x = xh*2^32 + xl with xl the low dword read as signed and
 * xh = (x + 2^31) >> 32. That covers [-2^63 - 2^31, 2^63 - 2^31); x >= 2^63 - 2^31
 * ("edge") is represented as x - 2^64, which changes nothing mod 2^64 and adds
 * e_a*c + e_c*a (mod 2^64) to k for a product ac: 2 masked adds per product.
 * Products of signed pieces lie in [-2^62 + 2^31, 2^62], so the differences in re
 * fit int64 and are split at once; the sums in im lie in [-2^63 + 2^32, 2^63] and
 * are split with a shifted offset.
 *
 * v_uns pieces: x = xh*2^32 + xl, both unsigned (x read as unsigned, x_u = x + 2^64
 * when x < 0), so k gets -(s_a*c + s_c*a) for a product ac (s = sign masks). The
 * unsigned products do not pair into int64 differences, so each is split.
 *
 * Build: gcc -O2 -mavx2 -o mul64 mul64.c
 * Run:   ./mul64 check | ./mul64 time [Nmem]
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int64_t i64;
typedef uint64_t u64;
typedef __int128 i128;
typedef unsigned __int128 u128;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define ADD(a, b) _mm256_add_epi64(a, b)
#define SUB(a, b) _mm256_sub_epi64(a, b)
#define SRL(a) _mm256_srli_epi64(a, 32)
#define SLL(a) _mm256_slli_epi64(a, 32)
#define MULS(a, b) _mm256_mul_epi32(a, b)
#define MULU(a, b) _mm256_mul_epu32(a, b)
#define LO32(a) _mm256_blend_epi32(a, _mm256_setzero_si256(), 0xAA)
#define C64(x) _mm256_set1_epi64x((i64)(x))
#define ARGS const i64 *restrict ar, const i64 *restrict ai, const i64 *restrict br, const i64 *restrict bi, \
             i64 *restrict cr, i64 *restrict ci, size_t N

/* ======================= scalar (study 20) ======================= */
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_bare(ARGS) {
    for (size_t i = 0; i < N; i++) {
        u64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        cr[i] = (i64)(a * c - b * d);
        ci[i] = (i64)(a * d + b * c);
    }
    return 0;
}
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_chk(ARGS) {
    u64 o = 0;
    for (size_t i = 0; i < N; i++) {
        i64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        i128 re = (i128)a * c - (i128)b * d;
        i128 im = (i128)((u128)((i128)a * d) + (u128)((i128)b * c));   /* 2^127 at the corner wraps and is caught */
        i64 rl = (i64)re, il = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl >> 63)) | ((u64)(im >> 64) ^ (u64)(il >> 63));
        cr[i] = rl; ci[i] = il;
    }
    return o != 0;
}

/* ======================= AVX2, wrapping ======================= */
NOINL int v_bare(ARGS) {
    for (size_t i = 0; i < N; i += 4) {
        V a = LD(ar + i), b = LD(ai + i), c = LD(br + i), d = LD(bi + i);
        V ah = SRL(a), bh = SRL(b), ch = SRL(c), dh = SRL(d);
        V re = SUB(MULU(a, c), MULU(b, d));
        V rm = SUB(ADD(MULU(ah, c), MULU(a, ch)), ADD(MULU(bh, d), MULU(b, dh)));
        V im = ADD(MULU(a, d), MULU(b, c));
        V imm = ADD(ADD(MULU(ah, d), MULU(a, dh)), ADD(MULU(bh, c), MULU(b, ch)));
        ST(cr + i, ADD(re, SLL(rm)));
        ST(ci + i, ADD(im, SLL(imm)));
    }
    return 0;
}

/* ======================= AVX2, signed pieces ======================= */
NOINL int v_sgn(ARGS) {
    const V off31 = C64(1LL << 31), edge = C64(0x7FFFFFFF7FFFFFFFLL),   /* x > edge  <=>  x >= 2^63 - 2^31 */
            sign = C64(0x8000000000000000ULL), offA = C64(0x7FFFFFFF00000000LL), offB = C64(0x7FFFFFFF00000001LL),
            kre = C64(1LL << 32), kim = C64((1LL << 32) - 2);
    V o = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 4) {
        V a = LD(ar + i), b = LD(ai + i), c = LD(br + i), d = LD(bi + i);
        V ah = SRL(ADD(a, off31)), bh = SRL(ADD(b, off31)), ch = SRL(ADD(c, off31)), dh = SRL(ADD(d, off31));
        V kr, ki;
        /* re: every column is a difference of two products, inside int64 */
        {
            V hh = SUB(MULS(ah, ch), MULS(bh, dh));
            V m1 = SUB(MULS(ah, c), MULS(bh, d));
            V m2 = SUB(MULS(a, ch), MULS(b, dh));
            V ll = SUB(MULS(a, c), MULS(b, d));
            ST(cr + i, ADD(ll, SLL(ADD(m1, m2))));
            V t1 = XOR(m1, sign), t2 = XOR(m2, sign);               /* floor(m/2^32) + 2^31 in the high dword */
            V z = ADD(SRL(XOR(ll, sign)), ADD(LO32(t1), LO32(t2)));  /* = (2^32 column) + 2^31 >= 0 */
            kr = ADD(hh, ADD(ADD(SRL(t1), SRL(t2)), SRL(z)));       /* k + 2^32, before the edge corrections */
        }
        /* im: sums of two products in [-2^63 + 2^32, 2^63], split with offsets */
        {
            V hh = ADD(MULS(ah, dh), MULS(bh, ch));
            V m1 = ADD(MULS(ah, d), MULS(bh, c));
            V m2 = ADD(MULS(a, dh), MULS(b, ch));
            V ll = ADD(MULS(a, d), MULS(b, c));
            ST(ci + i, ADD(ll, SLL(ADD(m1, m2))));
            V t1 = ADD(m1, offA), t2 = ADD(m2, offB);
            V z = ADD(SRL(ADD(ll, offA)), ADD(LO32(t1), LO32(t2)));
            ki = ADD(hh, ADD(ADD(SRL(t1), SRL(t2)), SRL(z)));       /* k + 2^32 - 2 */
        }
        /* edge corrections to k (mod 2^64) */
        V ea = _mm256_cmpgt_epi64(a, edge), eb = _mm256_cmpgt_epi64(b, edge),
          ec = _mm256_cmpgt_epi64(c, edge), ed = _mm256_cmpgt_epi64(d, edge);
        kr = ADD(kr, SUB(SUB(ADD(AND(c, ea), AND(a, ec)), ADD(AND(d, eb), AND(b, ed))), kre));
        ki = ADD(ki, SUB(ADD(ADD(AND(d, ea), AND(a, ed)), ADD(AND(c, eb), AND(b, ec))), kim));
        o = OR(o, OR(kr, ki));                                       /* nonzero: k != 0, does not fit */
    }
    return !_mm256_testz_si256(o, o);
}

/* ======================= AVX2, unsigned pieces ======================= */
NOINL int v_uns(ARGS) {
    const V sign = C64(0x8000000000000000ULL), kre = C64((1LL << 31) + (1LL << 33)), kim = C64(1LL << 31),
            two = C64(2), zero = _mm256_setzero_si256();
    V o = zero;
    for (size_t i = 0; i < N; i += 4) {
        V a = LD(ar + i), b = LD(ai + i), c = LD(br + i), d = LD(bi + i);
        V ah = SRL(a), bh = SRL(b), ch = SRL(c), dh = SRL(d);
        V kr, ki;
        {
            V p = MULU(a, c), q = MULU(b, d), D = SUB(p, q);
            V bw = _mm256_cmpgt_epi64(XOR(q, sign), XOR(p, sign));   /* -1 where p < q unsigned */
            V hh = SUB(MULU(ah, ch), MULU(bh, dh));
            V m1 = MULU(ah, c), m2 = MULU(a, ch), m3 = MULU(bh, d), m4 = MULU(b, dh);
            V L = SUB(ADD(LO32(m1), LO32(m2)), ADD(LO32(m3), LO32(m4)));
            V H = SUB(ADD(SRL(m1), SRL(m2)), ADD(SRL(m3), SRL(m4)));
            ST(cr + i, ADD(D, SLL(L)));
            V z = SRL(ADD(ADD(L, SRL(D)), kre));                      /* carry + 2 */
            kr = ADD(ADD(hh, H), ADD(bw, z));                         /* k + 2, before the sign corrections */
        }
        {
            V p = MULU(a, d), q = MULU(b, c), D = ADD(p, q);
            V cy = _mm256_cmpgt_epi64(XOR(p, sign), XOR(D, sign));   /* -1 where p + q carried */
            V hh = ADD(MULU(ah, dh), MULU(bh, ch));
            V m1 = MULU(ah, d), m2 = MULU(a, dh), m3 = MULU(bh, c), m4 = MULU(b, ch);
            V L = ADD(ADD(LO32(m1), LO32(m2)), ADD(LO32(m3), LO32(m4)));
            V H = ADD(ADD(SRL(m1), SRL(m2)), ADD(SRL(m3), SRL(m4)));
            ST(ci + i, ADD(D, SLL(L)));
            V z = SRL(ADD(ADD(L, SRL(D)), kim));
            ki = SUB(ADD(hh, H), SUB(cy, z));                         /* k, before the sign corrections */
        }
        V sa = _mm256_cmpgt_epi64(zero, a), sb = _mm256_cmpgt_epi64(zero, b),
          sc = _mm256_cmpgt_epi64(zero, c), sd = _mm256_cmpgt_epi64(zero, d);
        kr = SUB(kr, ADD(SUB(ADD(AND(c, sa), AND(a, sc)), ADD(AND(d, sb), AND(b, sd))), two));
        ki = SUB(ki, ADD(ADD(AND(d, sa), AND(a, sd)), ADD(AND(c, sb), AND(b, sc))));
        o = OR(o, OR(kr, ki));
    }
    return !_mm256_testz_si256(o, o);
}

/* ======================= kernel table ======================= */
typedef int (*kf)(ARGS);
typedef struct { const char *name; int chk; kf f; } kern;
static kern KT[] = {
    {"s_bare", 0, s_bare}, {"s_chk", 1, s_chk}, {"v_bare", 0, v_bare}, {"v_uns", 1, v_uns}, {"v_sgn", 1, v_sgn},
};
#define NKT (int)(sizeof KT / sizeof KT[0])

/* ======================= random ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i64 E[128]; static int NE;
static void add_edge(i128 v) {
    if (v < -((i128)1 << 63) || v >= ((i128)1 << 63)) return;
    for (int j = 0; j < NE; j++) if (E[j] == (i64)v) return;
    E[NE++] = (i64)v;
}
static void make_edges(void) {
    add_edge(0); add_edge(1); add_edge(-1);
    const int bits[] = {31, 32, 62, 63};
    for (int k = 0; k < 4; k++) {
        i128 p = (i128)1 << bits[k];
        for (int d = -2; d <= 1; d++) { add_edge(p + d); add_edge(-p + d); }
    }
    /* v_sgn's edge line 2^63 - 2^31, and its mirror */
    i128 L = ((i128)1 << 63) - ((i128)1 << 31);
    for (int d = -1; d <= 1; d++) { add_edge(L + d); add_edge(-L + d); }
}
/* random value: an edge, an edge +- 4, or random bit length */
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

/* ======================= correctness ======================= */
static long fails, cases, nflag[8][2], nnear[2];
/* true value within 4 of the int64 boundary: [-2^63 - 4, -2^63 + 3] or [2^63 - 4, 2^63 + 3] */
static int near(i128 x, i128 y) {
    i128 B = (i128)1 << 63;
    if (x > 0 && y > 0 && (x > B + 8 || y > B + 8)) return 0;
    if (x < 0 && y < 0 && (x < -B - 8 || y < -B - 8)) return 0;
    i128 v = x + y;
    return (v >= B - 4 && v <= B + 3) || (v >= -B - 4 && v <= -B + 3);
}
/* exact x + y fits int64, |x|, |y| <= 2^126 */
static int fitsum(i128 x, i128 y) {
    i128 lo = -((i128)1 << 63), hi = ((i128)1 << 63) - 1;
    return y >= lo - x && y <= hi - x;
}
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
            if ((u64)outr[j] != er || (u64)outi[j] != ei)
                if (fails++ < 10) printf("FAIL value %s j=%zu a=%lld b=%lld c=%lld d=%lld\n", KT[k].name, j,
                                         (long long)A[j], (long long)B[j], (long long)Cc[j], (long long)D[j]);
        }
        if (KT[k].chk) {
            if (f != want) if (fails++ < 10) printf("FAIL flag %s got %d want %d (a=%lld b=%lld c=%lld d=%lld)\n", KT[k].name, f, want,
                                                    (long long)A[0], (long long)B[0], (long long)Cc[0], (long long)D[0]);
            nflag[k][f]++;
        }
        cases++;
    }
}
static i64 sat(i128 v) {
    if (v >= ((i128)1 << 63)) return INT64_MAX;
    if (v < -((i128)1 << 63)) return INT64_MIN;
    return (i64)v;
}
static void do_check(void) {
    make_edges();
    printf("edge values: %d\n", NE);
    static i64 A[256], B[256], Cc[256], D[256];
    long cnt = 0;
    /* every edge quadruple, one element alone among 4 (one register), position rotating */
    for (int p = 0; p < NE; p++) for (int q = 0; q < NE; q++) for (int r = 0; r < NE; r++) for (int s = 0; s < NE; s++) {
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = cnt++ & 3;
        A[l] = E[p]; B[l] = E[q]; Cc[l] = E[r]; D[l] = E[s];
        check_arr(A, B, Cc, D, 4);
    }
    printf("edge quadruples: %ld\n", cnt);
    /* the all -2^63 corner in every lane at once */
    for (int j = 0; j < 4; j++) A[j] = B[j] = Cc[j] = D[j] = INT64_MIN;
    check_arr(A, B, Cc, D, 4);
    /* random elements, alone (per-element flag) */
    long rnd = 0;
    for (int it = 0; it < 4000000; it++) {
        memset(A, 0, 32); memset(B, 0, 32); memset(Cc, 0, 32); memset(D, 0, 32);
        int l = it & 3;
        A[l] = rval(); B[l] = rval(); Cc[l] = rval(); D[l] = rval();
        if (it % 4 == 1) {   /* re aimed at the int64 boundary: b small, d chosen */
            i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = 1;
            B[l] = bb;
            D[l] = sat(((i128)A[l] * Cc[l] - T) / bb);
        } else if (it % 4 == 2) {   /* im aimed at the boundary: b small, c chosen */
            i128 T = (next() & 1 ? ((i128)1 << 63) : -((i128)1 << 63)) + (i128)(next() % 7) - 3;
            i64 bb = (i64)(next() % 17) - 8; if (!bb) bb = -1;
            B[l] = bb;
            Cc[l] = sat((T - (i128)A[l] * D[l]) / bb);
        }
        check_arr(A, B, Cc, D, 4);
        rnd++;
    }
    printf("random single elements: %ld (half aimed at the int64 boundary)\n", rnd);
    /* random full arrays */
    long arr = 0;
    for (int it = 0; it < 200000; it++) {
        size_t N = 4 * (1 + next() % 32);
        int sm = (int)(next() % 3);
        for (size_t j = 0; j < N; j++) {
            if (sm == 0) { A[j] = rval(); B[j] = rval(); Cc[j] = rval(); D[j] = rval(); }
            else {   /* all small enough to fit, sometimes one big value planted */
                int w = 20 + (int)(next() % 11);
                A[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); B[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1));
                Cc[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1)); D[j] = (i64)(next() >> (64 - w)) - (1LL << (w - 1));
            }
        }
        if (sm == 2) A[next() % N] = rval();
        check_arr(A, B, Cc, D, N);
        arr++;
    }
    printf("random arrays: %ld (length 4..128)\n", arr);
    printf("elements whose re or im is within 4 of the boundary: %ld fit, %ld overflow\n", nnear[1], nnear[0]);
    printf("kernel calls %ld, failures %ld\n", cases, fails);
    for (int k = 0; k < NKT; k++) if (KT[k].chk) printf("  %-7s flag clear %ld, set %ld\n", KT[k].name, nflag[k][0], nflag[k][1]);
}

/* ======================= timing ======================= */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31
static volatile int sink;
static void do_time(size_t bigN) {
    char *buf[6];
    for (int j = 0; j < 6; j++) buf[j] = (char *)aligned_alloc(4096, bigN * 8 + 4096) + 576 * j;
    const size_t sizes[3] = {512, 16384, bigN}, inner[3] = {4096, 128, 1};
    static const char *pn[3] = {"L1", "L2", "memory"};
    printf("ns per element (one Gaussian product); median [q1, q3] over %d reps; kernels interleaved, order rotated\n", REPS);
    for (int p = 0; p < 3; p++) {
        size_t N = sizes[p];
        for (int j = 0; j < 4; j++)     /* operands below 2^30: nothing overflows (the loops do not branch anyway) */
            for (size_t i = 0; i < N; i++) ((i64 *)buf[j])[i] = (i64)(next() >> 34) - (1LL << 29);
        memset(buf[4], 0, N * 8); memset(buf[5], 0, N * 8);
        static double t[NKT][REPS];
#define CALL(k) KT[k].f((i64 *)buf[0], (i64 *)buf[1], (i64 *)buf[2], (i64 *)buf[3], (i64 *)buf[4], (i64 *)buf[5], N)
        for (int k = 0; k < NKT; k++) sink += CALL(k);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NKT; j++) {
                int k = (j + r) % NKT;
                sink += CALL(k);
                double t0 = now();
                for (size_t it = 0; it < inner[p]; it++) sink += CALL(k);
                t[k][r] = (now() - t0) / (double)(N * inner[p]);
            }
        printf("\n%s, N = %zu (%zu KB per array)\n", pn[p], N, N * 8 / 1024);
        for (int k = 0; k < NKT; k++) {
            qsort(t[k], REPS, sizeof(double), cmpd);
            printf("  %-7s %.4f [%.4f, %.4f]\n", KT[k].name, t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]);
        }
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else do_time(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 25);
    return 0;
}
