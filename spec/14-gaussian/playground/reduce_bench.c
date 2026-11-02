/* Study 4 toy timing: Barrett against Montgomery reduction, mod n near 2^31.
 * A TOY TIMING on whatever machine runs it, not a measurement of the real build.
 * Lanes: a word holds residues a (low 32 bits) and b (high 32 bits), each in [0, n).
 * Kernels, each over an array of N random pairs, writing an output array:
 *   scalar:   t = x * y (x, y < n), reduce t mod n
 *   gaussian: re = ac + (n^2 - bd), im = ad + bc, each reduced mod n, packed (as reduce.py)
 * Reductions: Barrett (m = floor(2^64 / n)), Montgomery (R = 2^64, inputs pre-converted to Montgomery form, untimed),
 * and, as a reference only, the compiler's t % n (a hardware divide).
 * Each (n, kernel) is timed REPS times, the kernels interleaved in each repetition; reported in ns per element:
 * median, quartiles and min/max over the repetitions.
 * Build: gcc -O2 -o reduce_bench reduce_bench.c      Run: ./reduce_bench */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef uint64_t u64; typedef unsigned __int128 u128;
#define N (1u << 20)
#define REPS 41

static inline u64 mulhi(u64 x, u64 y) { return (u64)(((u128)x * y) >> 64); }
static inline u64 barrett(u64 t, u64 n, u64 m) {
    u64 q = mulhi(t, m);
    u64 r = t - q * n;                  /* [0, 2n) */
    r -= n;
    return r + (n & (u64)((int64_t)r >> 63));
}
static inline u64 montgomery(u64 t, u64 n, u64 ninv) {
    u64 mm = t * ninv;                  /* low word */
    u64 r = 0 - mulhi(mm, n);           /* (t - mm n) / 2^64, in (-n, 0] */
    return r + (n & (u64)((int64_t)r >> 63));
}
static inline u64 divide(u64 t, u64 n, u64 unused) { (void)unused; return t % n; }

#define LO(w) ((w) & 0xffffffffu)
#define HI(w) ((w) >> 32)
#define KERNELS(NAME, RED) \
static void scalar_##NAME(const u64 *x, const u64 *y, u64 *out, u64 n, u64 c) { \
    for (u64 i = 0; i < N; i++) out[i] = RED(LO(x[i]) * LO(y[i]), n, c); } \
static void gauss_##NAME(const u64 *x, const u64 *y, u64 *out, u64 n, u64 c) { \
    u64 n2 = n * n; \
    for (u64 i = 0; i < N; i++) { \
        u64 a = LO(x[i]), b = HI(x[i]), cc = LO(y[i]), d = HI(y[i]); \
        u64 re = a * cc + (n2 - b * d), im = a * d + b * cc; \
        out[i] = RED(re, n, c) | (RED(im, n, c) << 32); } }
KERNELS(barrett, barrett)
KERNELS(montgomery, montgomery)
KERNELS(divide, divide)

typedef void (*kernel)(const u64 *, const u64 *, u64 *, u64, u64);
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
static u64 rng = 88172645463325252ull;
static u64 next(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }
static u64 modpow(u64 b, u64 e, u64 n) { u64 r = 1; b %= n; while (e) { if (e & 1) r = (u64)((u128)r * b % n); b = (u64)((u128)b * b % n); e >>= 1; } return r; }

int main(void) {
    const u64 ns[] = {2147483647u, 2147483629u, 2147483587u, 2147483579u};
    u64 *x = malloc(N * 8), *y = malloc(N * 8), *xm = malloc(N * 8), *ym = malloc(N * 8);
    u64 *o[3]; for (int k = 0; k < 3; k++) o[k] = malloc(N * 8);
    const char *names[3] = {"barrett", "montgomery", "divide (reference)"};
    printf("TOY TIMING (not the real build's measurement). N = %u pairs, %d repetitions, ns per element.\n", N, REPS);
    for (int shape = 0; shape < 2; shape++) {
        printf("\n%s\n", shape ? "gaussian product mod n (4 products, 2 reductions)" : "scalar product mod n (1 product, 1 reduction)");
        printf("%-12s %-20s %8s %8s %8s %8s %8s\n", "n", "reduction", "median", "q1", "q3", "min", "max");
        for (unsigned in = 0; in < sizeof ns / sizeof *ns; in++) {
            u64 n = ns[in], m = UINT64_MAX / n, ninv = n;
            for (int i = 0; i < 6; i++) ninv *= 2 - n * ninv;          /* n^-1 mod 2^64 (Newton) */
            u64 R = modpow(2, 64, n);                                      /* 2^64 mod n */
            for (u64 i = 0; i < N; i++) {
                u64 a = next() % n, b = next() % n, c = next() % n, d = next() % n;
                x[i] = a | (b << 32); y[i] = c | (d << 32);
                xm[i] = (u64)((u128)a * R % n) | ((u64)((u128)b * R % n) << 32);
                ym[i] = (u64)((u128)c * R % n) | ((u64)((u128)d * R % n) << 32);
            }
            kernel ks[3] = {shape ? gauss_barrett : scalar_barrett, shape ? gauss_montgomery : scalar_montgomery,
                            shape ? gauss_divide : scalar_divide};
            u64 cs[3] = {m, ninv, 0};
            double t[3][REPS];
            for (int r = 0; r < REPS; r++)
                for (int j = 0; j < 3; j++) {
                    int k = (j + r) % 3;                                    /* rotate the order each repetition */
                    const u64 *xx = k == 1 ? xm : x, *yy = k == 1 ? ym : y;
                    double t0 = now(); ks[k](xx, yy, o[k], n, cs[k]); t[k][r] = (now() - t0) / N;
                }
            /* correctness (untimed): Montgomery output is (product) R mod n per lane; bring it back with one reduction */
            u64 bad = 0;
            for (u64 i = 0; i < N; i++) {
                u64 mo = shape ? (montgomery(LO(o[1][i]), n, ninv) | (montgomery(HI(o[1][i]), n, ninv) << 32))
                               : montgomery(o[1][i], n, ninv);
                bad += (o[0][i] != o[2][i]) + (mo != o[2][i]);
            }
            for (int k = 0; k < 3; k++) {
                qsort(t[k], REPS, sizeof(double), cmpd);
                printf("%-12llu %-20s %8.3f %8.3f %8.3f %8.3f %8.3f\n", (unsigned long long)n, names[k],
                       t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4], t[k][0], t[k][REPS - 1]);
            }
            if (bad) printf("  MISMATCHES: %llu\n", (unsigned long long)bad);
        }
    }
    return 0;
}
