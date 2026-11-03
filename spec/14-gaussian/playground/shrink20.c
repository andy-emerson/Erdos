/* Study 20: what the fused shrink test costs.
 *
 * Gaussian integers at the first grown size: int64 halves, layout (b) (separate
 * real and imaginary arrays), AVX2 64-bit lanes. Add and subtract of two arrays,
 * three variants each:
 *   1 bare      r = x +/- y
 *   2 shrink    also: sticky_s |= r + 2^30         (rule B: r fits in 31 bits iff
 *                                                    (r + 2^30) mod 2^64 < 2^31)
 *   3 checks    also: sticky_o |= (x^r) & ~(x^y)   (add, sign bit = signed overflow)
 *                     sticky_o |= (x^r) &  (x^y)   (sub)
 * Each sticky is a register; its end mask is applied once per array (vptest),
 * which is valid since OR(t_i) AND m = OR(t_i AND m).
 * No 64-bit arithmetic right shift is needed (AVX2 has none): the add-offset form
 * replaces "top 34 bits all equal".
 * N must be a multiple of 4 (one register per half per pass).
 *
 * Build: gcc -O2 -mavx2 -o shrink20 shrink20.c
 * Run:   ./shrink20 check | ./shrink20 time [N] | ./shrink20 time-aligned [N]   (N large-array elements, default 2^24)
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int64_t i64;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define KARGS const i64 *restrict ar, const i64 *restrict ai, const i64 *restrict br, const i64 *restrict bi, \
              i64 *restrict cr, i64 *restrict ci, size_t N

/* return value: bit 0 = some result overflowed int64, bit 1 = some half does not fit in 31 bits */
#define SHRINK_MASK 0xFFFFFFFF80000000LL     /* bits 31..63 of r + 2^30 */
#define SIGN_MASK   ((i64)0x8000000000000000ULL)

NOINL int add1(KARGS) {
    for (size_t i = 0; i < N; i += 4) {
        ST(cr + i, _mm256_add_epi64(LD(ar + i), LD(br + i)));
        ST(ci + i, _mm256_add_epi64(LD(ai + i), LD(bi + i)));
    }
    return 0;
}
NOINL int sub1(KARGS) {
    for (size_t i = 0; i < N; i += 4) {
        ST(cr + i, _mm256_sub_epi64(LD(ar + i), LD(br + i)));
        ST(ci + i, _mm256_sub_epi64(LD(ai + i), LD(bi + i)));
    }
    return 0;
}

#define SHRINK_BODY(OP)                                               \
    V off = _mm256_set1_epi64x(1LL << 30), s = _mm256_setzero_si256(); \
    for (size_t i = 0; i < N; i += 4) {                               \
        V r = OP(LD(ar + i), LD(br + i));                             \
        V q = OP(LD(ai + i), LD(bi + i));                             \
        ST(cr + i, r); ST(ci + i, q);                                 \
        s = _mm256_or_si256(s, _mm256_add_epi64(r, off));             \
        s = _mm256_or_si256(s, _mm256_add_epi64(q, off));             \
    }                                                                 \
    return _mm256_testz_si256(s, _mm256_set1_epi64x(SHRINK_MASK)) ? 0 : 2;

NOINL int add2(KARGS) { SHRINK_BODY(_mm256_add_epi64) }
NOINL int sub2(KARGS) { SHRINK_BODY(_mm256_sub_epi64) }

/* add overflow: (x^r) & ~(x^y)  -> andnot(x^y, x^r);  sub overflow: (x^r) & (x^y) */
#define OVF_ADD(x, y, r) _mm256_andnot_si256(_mm256_xor_si256(x, y), _mm256_xor_si256(x, r))
#define OVF_SUB(x, y, r) _mm256_and_si256(_mm256_xor_si256(x, y), _mm256_xor_si256(x, r))

#define CHECK_BODY(OP, OVF)                                                  \
    V off = _mm256_set1_epi64x(1LL << 30), s = _mm256_setzero_si256(),       \
      o = _mm256_setzero_si256();                                            \
    for (size_t i = 0; i < N; i += 4) {                                      \
        V x = LD(ar + i), y = LD(br + i), u = LD(ai + i), v = LD(bi + i);    \
        V r = OP(x, y), q = OP(u, v);                                        \
        ST(cr + i, r); ST(ci + i, q);                                        \
        s = _mm256_or_si256(s, _mm256_add_epi64(r, off));                    \
        s = _mm256_or_si256(s, _mm256_add_epi64(q, off));                    \
        o = _mm256_or_si256(o, OVF(x, y, r));                                \
        o = _mm256_or_si256(o, OVF(u, v, q));                                \
    }                                                                        \
    V sm = _mm256_set1_epi64x(SIGN_MASK);                                    \
    return (_mm256_testz_si256(o, sm) ? 0 : 1) |                             \
           (_mm256_testz_si256(s, _mm256_set1_epi64x(SHRINK_MASK)) ? 0 : 2);

NOINL int add3(KARGS) { CHECK_BODY(_mm256_add_epi64, OVF_ADD) }
NOINL int sub3(KARGS) { CHECK_BODY(_mm256_sub_epi64, OVF_SUB) }

typedef int (*kern_t)(KARGS);
#define NK 6
static kern_t K[NK] = {add1, add2, add3, sub1, sub2, sub3};
static const char *kname[NK] = {"add bare", "add shrink", "add checks", "sub bare", "sub shrink", "sub checks"};

/* ---------- random ---------- */
static uint64_t rs = 0x9E3779B97F4A7C15ULL;
static uint64_t next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }

static i64 E[64]; static int NE;
static void make_edges(void) {
    const int bits[] = {30, 31, 62, 63};
    E[NE++] = 0; E[NE++] = 1; E[NE++] = -1;
    for (int k = 0; k < 4; k++) {
        __int128 p = (__int128)1 << bits[k];
        __int128 c[] = {p - 2, p - 1, p, p + 1, -p - 1, -p, -p + 1, -p + 2};
        for (int j = 0; j < 8; j++) if (c[j] <= INT64_MAX && c[j] >= INT64_MIN) E[NE++] = (i64)c[j];
    }
}
/* a value: an edge, an edge plus a small offset, or a random value of random bit length */
static i64 rval(int bigprob) {
    uint64_t k = next() % 100;
    if (k < 25) return E[next() % NE];
    if (k < 40) return (i64)((uint64_t)E[next() % NE] + (next() % 9) - 4);   /* wraps near +-2^63 */
    int b = (k < 40 + (uint64_t)bigprob) ? (int)(next() % 64) : (int)(next() % 31);
    i64 v = b ? (i64)(next() >> (64 - b)) : 0;
    return (next() & 1) ? -v - (i64)(next() & 1) : v;
}

/* ---------- correctness ---------- */
static long fails, cases;
static int fit31(__int128 v) { return v >= -((__int128)1 << 30) && v < ((__int128)1 << 30); }
static int fit64(__int128 v) { return v >= INT64_MIN && v <= INT64_MAX; }

/* compare kernel k on arrays of length N against __int128; return expected bad/fit info */
static void check_one(int k, const i64 *ar, const i64 *ai, const i64 *br, const i64 *bi, size_t N, long *seen) {
    static i64 cr[1024], ci[1024];
    int sub = k >= 3, var = k % 3 + 1;
    int f = K[k](ar, ai, br, bi, cr, ci, N);
    int ovf = 0, nofit = 0;
    for (size_t j = 0; j < N; j++) {
        __int128 tr = sub ? (__int128)ar[j] - br[j] : (__int128)ar[j] + br[j];
        __int128 ti = sub ? (__int128)ai[j] - bi[j] : (__int128)ai[j] + bi[j];
        if (!fit64(tr) || !fit64(ti)) ovf = 1;
        if (!fit31(tr) || !fit31(ti)) nofit = 1;
        if (cr[j] != (i64)(uint64_t)(unsigned __int128)tr || ci[j] != (i64)(uint64_t)(unsigned __int128)ti) {
            if (fails++ < 10) printf("FAIL value k=%d j=%zu\n", k, j);
        }
    }
    cases++;
    int want = 0;
    if (var == 1) { seen[0]++; return; }  /* bare: values only */
    if (ovf) {
        /* the shrink flag describes the wrapped results, so it is meaningless here: the
           operation is redone at the next size. Variant 2 has no overflow test at all. */
        if (var == 3 && !(f & 1)) { if (fails++ < 10) printf("FAIL ovf missed k=%d\n", k); }
        seen[var == 3 ? (f & 2 ? 3 : 1) : 4]++;
        return;
    }
    want = nofit << 1;
    if (f != want) { if (fails++ < 10) printf("FAIL flags k=%d got %d want %d\n", k, f, want); }
    seen[want]++;
}

static void do_check(void) {
    make_edges();
    static i64 ar[1024], ai[1024], br[1024], bi[1024];
    long seen[NK][5]; memset(seen, 0, sizeof seen);
    printf("%d edge values\n", NE);
    /* every edge pair in the real half x every edge pair in the imaginary half,
       one element alone in a register (other lanes 0), lane position rotating */
    long cnt = 0;
    for (int a = 0; a < NE; a++) for (int b = 0; b < NE; b++)
        for (int c = 0; c < NE; c++) for (int d = 0; d < NE; d++) {
            memset(ar, 0, 32); memset(ai, 0, 32); memset(br, 0, 32); memset(bi, 0, 32);
            int l = cnt++ & 3;
            ar[l] = E[a]; br[l] = E[b]; ai[l] = E[c]; bi[l] = E[d];
            for (int k = 0; k < NK; k++) check_one(k, ar, ai, br, bi, 4, seen[k]);
        }
    printf("edge quadruples: %ld\n", cnt);
    /* random arrays: mostly small values with a tunable chance of a big one */
    long rnd = 0;
    for (int it = 0; it < 400000; it++) {
        size_t N = 4 * (1 + next() % 32);
        int bigprob = (int)(next() % 3) == 0 ? 0 : (int)(next() % 4);   /* % chance per value of a long random value */
        for (size_t j = 0; j < N; j++) { ar[j] = rval(bigprob); ai[j] = rval(bigprob); br[j] = rval(bigprob); bi[j] = rval(bigprob); }
        if (it % 2) for (size_t j = 0; j < N; j++) {          /* half the arrays: all small, one edge planted */
            ar[j] = (i64)(next() % (1u << 29)) - (1 << 28); ai[j] = (i64)(next() % (1u << 29)) - (1 << 28);
            br[j] = (i64)(next() % (1u << 29)) - (1 << 28); bi[j] = (i64)(next() % (1u << 29)) - (1 << 28);
            if (it % 4 == 1) { size_t p = next() % N; i64 *t[] = {ar, ai, br, bi}; t[next() % 4][p] = rval(0); }
        }
        for (int k = 0; k < NK; k++) check_one(k, ar, ai, br, bi, N, seen[k]);
        rnd++;
    }
    printf("random arrays: %ld (length 4..128)\n", rnd);
    printf("cases %ld, failures %ld\n", cases, fails);
    printf("flag outcomes (no ovf, all fit / no ovf, some don't fit / ovf, wrapped results all fit / ovf, some don't / ovf, untested):\n");
    for (int k = 0; k < NK; k++) printf("  %-11s %9ld %9ld %9ld %9ld %9ld\n", kname[k], seen[k][0], seen[k][2], seen[k][1], seen[k][3], seen[k][4]);
}

/* ---------- timing ---------- */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31
static i64 *Ar, *Ai, *Br, *Bi, *Cr, *Ci;
static volatile int sink;
static int stagger = 1;

static void do_time(size_t bigN) {
    size_t sz = bigN * sizeof(i64);
    /* each array page-aligned, then shifted by j * 576 bytes, so that the six streams do not share
       their address bits 0..11 (4K aliasing between stores and later loads) */
    i64 **arr[6] = {&Ar, &Ai, &Br, &Bi, &Cr, &Ci};
    for (int j = 0; j < 6; j++) {
        char *m = aligned_alloc(4096, sz + 4096);
        *arr[j] = (i64 *)(m + (stagger ? 576 * j : 0));
    }
    for (size_t i = 0; i < bigN; i++) {               /* values that fit: no overflow, shrink flag "all fit" */
        Ar[i] = (i64)(next() % (1u << 29)) - (1 << 28); Ai[i] = (i64)(next() % (1u << 29)) - (1 << 28);
        Br[i] = (i64)(next() % (1u << 29)) - (1 << 28); Bi[i] = (i64)(next() % (1u << 29)) - (1 << 28);
        Cr[i] = Ci[i] = 0;
    }
    const size_t sizes[3] = {512, 16384, bigN};
    const size_t inner[3] = {4096, 128, 1};
    const char *label[3] = {"L1 (512 elements, 24 KB in all)", "L2 (16384 elements, 768 KB in all)", "memory"};
    printf("ns per element (one Gaussian integer = 2 int64 halves); median [q1, q3] over %d reps; kernels interleaved, order rotated\n", REPS);
    for (int p = 0; p < 3; p++) {
        size_t N = sizes[p];
        if (p == 2) printf("\nmemory (%zu elements, %zu MB per array, %zu MB in all)\n", N, sz >> 20, 6 * (sz >> 20));
        else printf("\n%s, %zu calls per sample\n", label[p], inner[p]);
        static double t[NK][REPS];
        for (int k = 0; k < NK; k++) sink += K[k](Ar, Ai, Br, Bi, Cr, Ci, N);
        for (int r = 0; r < REPS; r++)
            for (int j = 0; j < NK; j++) {
                int k = (j + r) % NK;
                sink += K[k](Ar, Ai, Br, Bi, Cr, Ci, N);
                double t0 = now();
                for (size_t it = 0; it < inner[p]; it++) sink += K[k](Ar, Ai, Br, Bi, Cr, Ci, N);
                t[k][r] = (now() - t0) / (double)(N * inner[p]);
            }
        double med[NK];
        for (int k = 0; k < NK; k++) { qsort(t[k], REPS, sizeof(double), cmpd); med[k] = t[k][REPS / 2]; }
        for (int k = 0; k < NK; k++) {
            double base = med[k < 3 ? 0 : 3];
            printf("  %-11s %.4f [%.4f, %.4f]  extra %+.4f (%+.1f%%)\n", kname[k], med[k], t[k][REPS / 4], t[k][3 * REPS / 4],
                   med[k] - base, 100 * (med[k] - base) / base);
        }
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else { stagger = strcmp(mode, "time-aligned") != 0;   /* time-aligned: every array at the same page offset */
           do_time(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 24); }
    return 0;
}
