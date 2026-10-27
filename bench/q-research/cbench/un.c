#define _POSIX_C_SOURCE 200809L
/* Fixed-width uN tiers for the vector form, against GMP and FLINT, on workloads that pass 128 bits.

   usage: un METHOD OP N DEN K REPS INDIR OUTFILE
     METHOD  un      int64 numerators over the shared denominator, held in signed _BitInt(W) for W in
                     64, 128, 256, 512, 1024; the whole vector widens to the narrowest W its actual values
                     prove they need (the vector form's rule), checked once per operation from bit lengths
             mpz     GMP integers over the shared denominator
             fmpz    FLINT integers over the shared denominator (small values inline, promotion per element)
             mpq     GMP rationals, one per element, reduced every operation
     OP      sq      (a*b)^2 elementwise
             sumsq   the sum of (a*b)^2
             chain   x <- x*b + c, K steps (numerators over DEN^(K+1))
   INDIR holds a.bin, b.bin, c.bin (int64 numerators over DEN). Inputs are built before timing; outputs are
   preallocated where the method allows. Prints "median min width"; writes results as fractions to OUTFILE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>

static long N, DEN, K;
static int64_t *A, *B, *C;
static int64_t *load(const char *dir, const char *name) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(2); }
    int64_t *x = malloc(sizeof(int64_t) * N);
    if (fread(x, sizeof(int64_t), N, f) != (size_t)N) { fprintf(stderr, "short read\n"); exit(2); }
    fclose(f); return x;
}
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static int bits64(int64_t v) { uint64_t u = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; return u ? 64 - __builtin_clzll(u) : 0; }

/* ---------------------------------------------------------------- uN kernels, one set per width */
#define WIDTHS(X) X(64) X(128) X(256) X(512) X(1024)
#define DECL(W) typedef signed _BitInt(W) i##W;
WIDTHS(DECL)

/* bit length of the largest |x| in an array of W-bit values, from the OR of magnitudes (one pass) */
#define MAXBITS(W) \
static int maxbits_##W(const i##W *x, long n) { \
    unsigned _BitInt(W) acc = 0; \
    for (long i = 0; i < n; i++) { i##W v = x[i]; acc |= (unsigned _BitInt(W))(v < 0 ? -v : v); } \
    int b = 0; while (acc) { acc >>= 1; b++; } return b; }
WIDTHS(MAXBITS)

#define SQ(W) \
static void sq_##W(i##W *out) { for (long i = 0; i < N; i++) { i##W p = (i##W)A[i] * B[i]; out[i] = p * p; } }
WIDTHS(SQ)

#define SUMSQ(W) \
static i##W sumsq_##W(void) { i##W s = 0; for (long i = 0; i < N; i++) { i##W p = (i##W)A[i] * B[i]; s += p * p; } return s; }
WIDTHS(SUMSQ)

#define STEP(W) \
static void step_##W(i##W *x, i##W cs) { for (long i = 0; i < N; i++) x[i] = x[i] * B[i] + (i##W)C[i] * cs; }
WIDTHS(STEP)

/* widening: the whole vector moves to twice the width */
#define UP(W, W2) static void up_##W(const i##W *x, i##W2 *y) { for (long i = 0; i < N; i++) y[i] = x[i]; }
UP(64, 128) UP(128, 256) UP(256, 512) UP(512, 1024)

static void *xa[5];                 /* the chain's vector at each width, allocated once */
static int widx(int W) { return W == 64 ? 0 : W == 128 ? 1 : W == 256 ? 2 : W == 512 ? 3 : 4; }
static int final_width;

static void export_signed(mpz_t z, const void *p, int W) {      /* a W-bit two's complement value to GMP */
    size_t nl = W / 64; uint64_t limbs[16]; memcpy(limbs, p, nl * 8);
    int neg = (int64_t)limbs[nl - 1] < 0;
    if (neg) { unsigned c = 1; for (size_t i = 0; i < nl; i++) { limbs[i] = ~limbs[i] + c; c = c && limbs[i] == 0; } }
    mpz_import(z, nl, -1, 8, 0, 0, limbs);
    if (neg) mpz_neg(z, z);
}

/* the chain on uN: widen before a step whenever the bound from actual bit lengths could pass W - 1 */
static int un_chain(void) {
    int W = 64;
    i64 *x0 = xa[0];
    for (long i = 0; i < N; i++) x0[i] = A[i];
    int bx = 0, bb = 0, bc = 0;
    for (long i = 0; i < N; i++) { int t = bits64(A[i]); if (t > bx) bx = t; t = bits64(B[i]); if (t > bb) bb = t; t = bits64(C[i]); if (t > bc) bc = t; }
    int bd = bits64(DEN);
    for (long k = 0; k < K; k++) {
        int bcs = (int)((k + 1) * bd);                     /* bits of DEN^(k+1), an upper bound */
        int need = (bx + bb > bc + bcs ? bx + bb : bc + bcs) + 2;
        while (need > W) {
            if (W == 1024) return 0;
            switch (W) { case 64: up_64(xa[0], xa[1]); break; case 128: up_128(xa[1], xa[2]); break;
                         case 256: up_256(xa[2], xa[3]); break; default: up_512(xa[3], xa[4]); }
            W *= 2;
        }
        switch (W) {
#define DO(WW) case WW: { i##WW cs = 1; for (long j = 0; j <= k; j++) cs *= DEN; step_##WW(xa[widx(WW)], cs); bx = maxbits_##WW(xa[widx(WW)], N); break; }
            WIDTHS(DO)
#undef DO
        }
    }
    final_width = W;
    return 1;
}

static int un_needed_sq(void) {        /* narrowest width that holds every (a*b)^2 */
    int bmax = 0;
    for (long i = 0; i < N; i++) { int t = bits64(A[i]) + bits64(B[i]); if (t > bmax) bmax = t; }
    int need = 2 * bmax + 2, W = 64;
    while (W < need) W *= 2;
    return W;
}
static int un_needed_sumsq(void) {
    int W = un_needed_sq(), lg = 0; long n = N; while (n) { n >>= 1; lg++; }
    int need = 0; { int bmax = 0; for (long i = 0; i < N; i++) { int t = bits64(A[i]) + bits64(B[i]); if (t > bmax) bmax = t; } need = 2 * bmax + lg + 2; }
    while (W < need) W *= 2;
    return W;
}
static void *outbuf;
static unsigned char sumbuf[128];
static void un_run(const char *op) {
    if (is(op, "chain")) { if (!un_chain()) final_width = -1; return; }
    if (is(op, "sq")) {
        int W = un_needed_sq();
        switch (W) {
#define DO(WW) case WW: sq_##WW(outbuf); break;
            WIDTHS(DO)
#undef DO
            default: final_width = -1; return;
        }
        final_width = W; return;
    }
    int W = un_needed_sumsq();
    switch (W) {
#define DO(WW) case WW: { i##WW s = sumsq_##WW(); memcpy(sumbuf, &s, sizeof s); break; }
        WIDTHS(DO)
#undef DO
        default: final_width = -1; return;
    }
    final_width = W;
}

/* ---------------------------------------------------------------- GMP and FLINT */
static mpz_t *za, *zb, *zc, *zx, zs, zt, zcs;
static fmpz *fa, *fb, *fc, *fx; static fmpz_t fs, ft, fcs;
static mpq_t *qa, *qb, *qc, *qx, qs, qt;
static void lib_setup(const char *m) {
    if (is(m, "mpz")) {
        za = malloc(sizeof(mpz_t) * N); zb = malloc(sizeof(mpz_t) * N); zc = malloc(sizeof(mpz_t) * N); zx = malloc(sizeof(mpz_t) * N);
        for (long i = 0; i < N; i++) { mpz_init_set_si(za[i], A[i]); mpz_init_set_si(zb[i], B[i]); mpz_init_set_si(zc[i], C[i]); mpz_init(zx[i]); }
        mpz_init(zs); mpz_init(zt); mpz_init(zcs);
    } else if (is(m, "fmpz")) {
        fa = _fmpz_vec_init(N); fb = _fmpz_vec_init(N); fc = _fmpz_vec_init(N); fx = _fmpz_vec_init(N);
        for (long i = 0; i < N; i++) { fmpz_set_si(fa + i, A[i]); fmpz_set_si(fb + i, B[i]); fmpz_set_si(fc + i, C[i]); }
        fmpz_init(fs); fmpz_init(ft); fmpz_init(fcs);
    } else {
        qa = malloc(sizeof(mpq_t) * N); qb = malloc(sizeof(mpq_t) * N); qc = malloc(sizeof(mpq_t) * N); qx = malloc(sizeof(mpq_t) * N);
        for (long i = 0; i < N; i++) {
            mpq_init(qa[i]); mpq_init(qb[i]); mpq_init(qc[i]); mpq_init(qx[i]);
            mpq_set_si(qa[i], A[i], DEN); mpq_canonicalize(qa[i]);
            mpq_set_si(qb[i], B[i], DEN); mpq_canonicalize(qb[i]);
            mpq_set_si(qc[i], C[i], DEN); mpq_canonicalize(qc[i]);
        }
        mpq_init(qs); mpq_init(qt);
    }
}
static void lib_run(const char *m, const char *op) {
    if (is(m, "mpz")) {
        if (is(op, "sq")) for (long i = 0; i < N; i++) { mpz_mul(zt, za[i], zb[i]); mpz_mul(zx[i], zt, zt); }
        else if (is(op, "sumsq")) { mpz_set_ui(zs, 0); for (long i = 0; i < N; i++) { mpz_mul(zt, za[i], zb[i]); mpz_addmul(zs, zt, zt); } }
        else for (long i = 0; i < N; i++) mpz_set(zx[i], za[i]);
        if (is(op, "chain")) {
            mpz_set_ui(zcs, 1);
            for (long k = 0; k < K; k++) {
                mpz_mul_si(zcs, zcs, DEN);
                for (long i = 0; i < N; i++) { mpz_mul(zx[i], zx[i], zb[i]); mpz_addmul(zx[i], zc[i], zcs); }
            }
        }
    } else if (is(m, "fmpz")) {
        if (is(op, "sq")) for (long i = 0; i < N; i++) { fmpz_mul(ft, fa + i, fb + i); fmpz_mul(fx + i, ft, ft); }
        else if (is(op, "sumsq")) { fmpz_zero(fs); for (long i = 0; i < N; i++) { fmpz_mul(ft, fa + i, fb + i); fmpz_addmul(fs, ft, ft); } }
        else for (long i = 0; i < N; i++) fmpz_set(fx + i, fa + i);
        if (is(op, "chain")) {
            fmpz_one(fcs);
            for (long k = 0; k < K; k++) {
                fmpz_mul_si(fcs, fcs, DEN);
                for (long i = 0; i < N; i++) { fmpz_mul(fx + i, fx + i, fb + i); fmpz_addmul(fx + i, fc + i, fcs); }
            }
        }
    } else {
        if (is(op, "sq")) for (long i = 0; i < N; i++) { mpq_mul(qt, qa[i], qb[i]); mpq_mul(qx[i], qt, qt); }
        else if (is(op, "sumsq")) { mpq_set_ui(qs, 0, 1); for (long i = 0; i < N; i++) { mpq_mul(qt, qa[i], qb[i]); mpq_mul(qt, qt, qt); mpq_add(qs, qs, qt); } }
        else {
            for (long i = 0; i < N; i++) mpq_set(qx[i], qa[i]);
            for (long k = 0; k < K; k++)
                for (long i = 0; i < N; i++) { mpq_mul(qx[i], qx[i], qb[i]); mpq_add(qx[i], qx[i], qc[i]); }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 9) { fprintf(stderr, "usage: un METHOD OP N DEN K REPS INDIR OUTFILE\n"); return 2; }
    const char *m = argv[1], *op = argv[2];
    N = atol(argv[3]); DEN = atol(argv[4]); K = atol(argv[5]); int reps = atoi(argv[6]);
    A = load(argv[7], "a.bin"); B = load(argv[7], "b.bin"); C = load(argv[7], "c.bin");
    int un = is(m, "un");
    if (un) { outbuf = malloc((size_t)N * 128); for (int w = 0; w < 5; w++) xa[w] = malloc((size_t)N * (8 << w)); }
    else lib_setup(m);
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (un) un_run(op); else lib_run(m, op);
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %d\n", t[reps / 2], t[0], un ? final_width : 0);
    FILE *f = fopen(argv[8], "w");
    mpz_t z, d; mpz_init(z); mpz_init(d);
    /* denominators: sq over DEN^4, sumsq over DEN^4, chain over DEN^(K+1) */
    mpz_ui_pow_ui(d, DEN, is(op, "chain") ? K + 1 : 4);
    if (un && final_width < 0) fprintf(f, "exceeds\n");
    else if (un) {
        if (is(op, "sumsq")) { export_signed(z, sumbuf, final_width); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
        else {
            const unsigned char *base = is(op, "sq") ? outbuf : xa[widx(final_width)];
            for (long i = 0; i < N; i++) { export_signed(z, base + (size_t)i * (final_width / 8), final_width); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
        }
    } else if (is(m, "mpz")) {
        if (is(op, "sumsq")) gmp_fprintf(f, "%Zd/%Zd\n", zs, d);
        else for (long i = 0; i < N; i++) gmp_fprintf(f, "%Zd/%Zd\n", zx[i], d);
    } else if (is(m, "fmpz")) {
        if (is(op, "sumsq")) { fmpz_get_mpz(z, fs); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
        else for (long i = 0; i < N; i++) { fmpz_get_mpz(z, fx + i); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
    } else {
        if (is(op, "sumsq")) gmp_fprintf(f, "%Qd\n", qs);
        else for (long i = 0; i < N; i++) gmp_fprintf(f, "%Qd\n", qx[i]);
    }
    fclose(f);
    return 0;
}
