/* Decode cost of the header designs on U64 words: the fitted complex layout (2 unit bits, 6 header slash bits),
   the uniform layout (4 unit bits, 4 header slash bits, 2 borrowed), and one decoder taking the layout at run
   time. usage: decode N REPS: prints "fitted uniform runtime" median ns per value for N values (out of cache) and
   for 4096 values decoded N/4096 times (in cache), then the checksums. decode check COUNT: prints COUNT random
   patterns with both decodings, for the test against family.decode. The sign is computed without a branch in
   every variant, so the loops differ only in the layout. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
typedef struct { int8_t sign, axis; uint64_t p, q; } val;
static inline val dec(uint8_t h8, uint64_t v, int U, int h, int b) {
    int D = 64 - b, code = h8 >> h;
    val r; r.sign = (int8_t)(1 - 2 * ((code >> (U - 1)) & 1)); r.axis = (int8_t)(code & ((1 << (U - 1)) - 1));
    int s = ((h8 & ((1 << h) - 1)) << b) | (int)(b ? v >> D : 0);
    uint64_t data = D == 64 ? v : v & ((1ULL << D) - 1);
    if (s > D - 1) s = D - 1;
    int w = D - (s + 1);
    r.p = w == 64 ? 0 : data >> w; r.q = (w ? data & ((1ULL << w) - 1) : 0) + 1;
    return r;
}
__attribute__((noinline)) static uint64_t run_fitted(const uint8_t *H, const uint64_t *V, long n) {
    uint64_t c = 0; for (long i = 0; i < n; i++) { val r = dec(H[i], V[i], 2, 6, 0); c += r.p ^ r.q ^ (uint64_t)r.axis ^ (uint64_t)r.sign; } return c; }
__attribute__((noinline)) static uint64_t run_uniform(const uint8_t *H, const uint64_t *V, long n) {
    uint64_t c = 0; for (long i = 0; i < n; i++) { val r = dec(H[i], V[i], 4, 4, 2); c += r.p ^ r.q ^ (uint64_t)r.axis ^ (uint64_t)r.sign; } return c; }
static volatile int RU = 4, RH = 4, RB = 2;
__attribute__((noinline)) static uint64_t run_runtime(const uint8_t *H, const uint64_t *V, long n) {
    int U = RU, h = RH, b = RB;
    uint64_t c = 0; for (long i = 0; i < n; i++) { val r = dec(H[i], V[i], U, h, b); c += r.p ^ r.q ^ (uint64_t)r.axis ^ (uint64_t)r.sign; } return c; }
static void timeall(const uint8_t *H, const uint64_t *V, long n, long passes, int reps, double *med, uint64_t *ck) {
    uint64_t (*fn[3])(const uint8_t *, const uint64_t *, long) = {run_fitted, run_uniform, run_runtime};
    for (int k = 0; k < 3; k++) {
        double *t = malloc(sizeof(double) * reps);
        for (int r = -1; r < reps; r++) {
            double t0 = now(); uint64_t c = 0;
            for (long j = 0; j < passes; j++) c += fn[k](H, V, n);
            ck[k] = c; if (r >= 0) t[r] = now() - t0;
        }
        qsort(t, reps, sizeof(double), cmpd); med[k] = t[reps / 2] / (n * passes) * 1e9; free(t);
    }
}
int main(int argc, char **argv) {
    uint64_t x = 88172645463325252ULL;
    if (argc == 3 && !strcmp(argv[1], "check")) {
        long c = atol(argv[2]);
        for (long i = 0; i < c; i++) {
            x ^= x << 13; x ^= x >> 7; x ^= x << 17; uint8_t h = (uint8_t)(x >> 3); uint64_t v = x * 0x9E3779B97F4A7C15ULL;
            val a = dec(h, v, 2, 6, 0), b = dec(h, v, 4, 4, 2);
            printf("%u %llu %d %d %llu %llu %d %d %llu %llu\n", h, (unsigned long long)v, a.sign, a.axis, (unsigned long long)a.p,
                   (unsigned long long)a.q, b.sign, b.axis, (unsigned long long)b.p, (unsigned long long)b.q);
        }
        return 0;
    }
    long n = atol(argv[1]); int reps = atoi(argv[2]);
    uint8_t *H = malloc(n); uint64_t *V = malloc(8 * n);
    for (long i = 0; i < n; i++) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; H[i] = (uint8_t)x; x ^= x << 13; x ^= x >> 7; x ^= x << 17; V[i] = x; }
    double big[3], small[3]; uint64_t ck[3], ck2[3];
    timeall(H, V, n, 1, reps, big, ck);
    timeall(H, V, 4096, n / 4096, reps, small, ck2);
    printf("%.3f %.3f %.3f %.3f %.3f %.3f\n%llu %llu %llu\n", big[0], big[1], big[2], small[0], small[1], small[2],
           (unsigned long long)ck[0], (unsigned long long)ck[1], (unsigned long long)ck[2]);
    return 0;
}
