#define _POSIX_C_SOURCE 200809L
/* Phase one, the widening rule: whole-vector widening against per-element promotion.

   usage: p1_widen RULE OP N REPS INDIR OUTFILE
     RULE  whole  one scan of bit lengths picks the width for the whole vector (128 or 256 bits); every element
                  is computed at that width (the vector form as tested so far)
           elem   each element is checked on its own: those whose result fits 128 bits are computed in the
                  dense 128-bit vector; the rest are recorded in an index list and computed at 256 bits in a
                  side vector (FLINT-style promotion, kept dense for the common case)
     OP    sq     (a b)^2 per element        sumsq  the sum of (a b)^2
   INDIR holds a.bin, b.bin (int64). Prints "median min wide_count"; writes the results as decimal integers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>

typedef signed _BitInt(128) i128; typedef unsigned _BitInt(128) u128b;
typedef signed _BitInt(256) i256; typedef unsigned _BitInt(256) u256;
static long N; static int64_t *A, *B;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static inline int bits64(int64_t v) { uint64_t u = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; return u ? 64 - __builtin_clzll(u) : 0; }
static int64_t *load(const char *dir, const char *name) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(2); }
    int64_t *x = malloc(8 * N); if (fread(x, 8, N, f) != (size_t)N) { fprintf(stderr, "short\n"); exit(2); } fclose(f); return x;
}
static i128 *o128; static i256 *o256, *side, sum; static long *idx, nwide; static int width;
static void whole(int sumsq) {
    int bmax = 0; for (long i = 0; i < N; i++) { int t = bits64(A[i]) + bits64(B[i]); if (t > bmax) bmax = t; }
    width = 2 * bmax + 2 <= 128 ? 128 : 256; nwide = width == 256 ? N : 0;
    if (sumsq) { i256 s = 0;                    /* the sum needs lg N more bits than one square: 256-bit accumulator */
        if (width == 128) { for (long i = 0; i < N; i++) { i128 p = (i128)A[i] * B[i]; s += (i256)(p * p); } }
        else for (long i = 0; i < N; i++) { i256 p = (i256)A[i] * B[i]; s += p * p; }
        sum = s; return; }
    if (width == 128) for (long i = 0; i < N; i++) { i128 p = (i128)A[i] * B[i]; o128[i] = p * p; }
    else for (long i = 0; i < N; i++) { i256 p = (i256)A[i] * B[i]; o256[i] = p * p; }
}
static void elem(int sumsq) {
    long k = 0; i256 s = 0;
    for (long i = 0; i < N; i++) {
        if (2 * (bits64(A[i]) + bits64(B[i])) + 2 <= 128) { i128 p = (i128)A[i] * B[i]; i128 q = p * p; if (sumsq) s += (i256)q; else o128[i] = q; }
        else { idx[k++] = i; if (!sumsq) o128[i] = 0; }
    }
    for (long j = 0; j < k; j++) { i256 p = (i256)A[idx[j]] * B[idx[j]]; if (sumsq) s += p * p; else side[j] = p * p; }
    nwide = k; sum = s; width = k ? 256 : 128;
}
static void out256(FILE *f, i256 v) { mpz_t z; mpz_init(z); u256 u = (u256)(v < 0 ? -v : v);
    mpz_import(z, 4, -1, 8, 0, 0, &u); if (v < 0) mpz_neg(z, z); gmp_fprintf(f, "%Zd\n", z); mpz_clear(z); }
int main(int argc, char **argv) {
    if (argc != 7) { fprintf(stderr, "usage: p1_widen RULE OP N REPS INDIR OUTFILE\n"); return 2; }
    int el = is(argv[1], "elem"), sumsq = is(argv[2], "sumsq"); N = atol(argv[3]); int reps = atoi(argv[4]);
    A = load(argv[5], "a.bin"); B = load(argv[5], "b.bin");
    o128 = malloc(sizeof(i128) * N); o256 = malloc(sizeof(i256) * N); side = malloc(sizeof(i256) * N); idx = malloc(sizeof(long) * N);
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) { double t0 = now(); if (el) elem(sumsq); else whole(sumsq); double dt = now() - t0; if (r >= 0) t[r] = dt; }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %ld\n", t[reps / 2], t[0], nwide);
    FILE *f = fopen(argv[6], "w");
    if (sumsq) out256(f, sum);
    else if (!el && width == 256) for (long i = 0; i < N; i++) out256(f, o256[i]);
    else { long j = 0; for (long i = 0; i < N; i++) { if (el && j < nwide && idx[j] == i) out256(f, side[j++]); else out256(f, (i256)o128[i]); } }
    fclose(f);
    return 0;
}
