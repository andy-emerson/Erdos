/* Exact octonion arrays: the vector form against GMP and FLINT.

   usage: oct METHOD OP N DEN REPS INDIR OUTFILE
     METHOD  vec     eight int64 component vectors over the shared denominator DEN; a product is computed in int64
                     when the operands' bit lengths prove it fits, otherwise in __int128, and refused (exit 3) if
                     it would need more than 128 bits (one check per operation)
             mpq     8-tuples of GMP rationals
             fmpz    eight FLINT integers over the shared denominator per octonion (Sage's quaternion layout)
             i64c    int64 components over DEN, every operation overflow-checked (the same written-out product)
     OP      add     elementwise p + q
             mul     elementwise octonion product p q (64 products, from the Cayley-Dickson table)
             norm    the sum of |p|^2 over the array
   Every method computes the product from the same written-out table (oct_table.h, generated from family.py).
   INDIR holds p0.bin .. p7.bin and q0.bin .. q7.bin. Prints "median min"; writes "c0 .. c7" fractions per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include "oct_table.h"

typedef __int128 i128;
static long N, DEN;
static int64_t *P[8], *Q[8];
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

#define PX(a) P[a][i]
#define QX(b) Q[b][i]
#define R64(z) o64[z][i]
#define R128(z) o128[z][i]

/* ---------------------------------------------------------------- the vector form */
static int bits_of(int64_t *x[8]) {
    uint64_t acc = 0;
    for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) { int64_t v = x[c][i]; acc |= v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; }
    return acc ? 64 - __builtin_clzll(acc) : 0;
}
static int64_t *o64[8]; static i128 *o128[8]; static int wide; static i128 vnorm;
static void vec_run(const char *op) {
    if (is(op, "add")) {
        int bp = bits_of(P), bq = bits_of(Q);
        wide = (bp > bq ? bp : bq) + 1 > 63;
        if (!wide) for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) o64[c][i] = P[c][i] + Q[c][i];
        else for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) o128[c][i] = (i128)P[c][i] + Q[c][i];
        return;
    }
    if (is(op, "norm")) {
        int b = bits_of(P);
        int lg = 0; long n = 8 * N; while (n) { n >>= 1; lg++; }
        if (2 * b + lg > 127) { fprintf(stderr, "norm needs more than 128 bits\n"); exit(3); }
        if (2 * b + lg <= 63) { int64_t s = 0; for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) s += P[c][i] * P[c][i]; vnorm = s; }
        else { i128 s = 0; for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) s += (i128)P[c][i] * P[c][i]; vnorm = s; }
        return;
    }
    int bp = bits_of(P), bq = bits_of(Q);                  /* mul: eight products summed, so 3 bits more */
    if (bp + bq + 3 > 127) { fprintf(stderr, "mul needs more than 128 bits\n"); exit(3); }
    wide = bp + bq + 3 > 63;
    if (!wide) for (long i = 0; i < N; i++) OCTMUL(int64_t, PX, QX, R64);
    else for (long i = 0; i < N; i++) OCTMUL(i128, PX, QX, R128);
}

/* ---------------------------------------------------------------- GMP rational 8-tuples */
static mpq_t *gp[8], *gq[8], *go[8], gt, gs;
static void gmp_setup(void) {
    for (int c = 0; c < 8; c++) {
        gp[c] = malloc(sizeof(mpq_t) * N); gq[c] = malloc(sizeof(mpq_t) * N); go[c] = malloc(sizeof(mpq_t) * N);
        for (long i = 0; i < N; i++) {
            mpq_init(gp[c][i]); mpq_init(gq[c][i]); mpq_init(go[c][i]);
            mpq_set_si(gp[c][i], P[c][i], DEN); mpq_canonicalize(gp[c][i]);
            mpq_set_si(gq[c][i], Q[c][i], DEN); mpq_canonicalize(gq[c][i]);
        }
    }
    mpq_init(gt); mpq_init(gs);
}
#define QMUL(r, x, y) mpq_mul(r, x, y)
#define QADDMUL(r, x, y) do { mpq_mul(gt, x, y); mpq_add(r, r, gt); } while (0)
#define QSUBMUL(r, x, y) do { mpq_mul(gt, x, y); mpq_sub(r, r, gt); } while (0)
static void gmp_run(const char *op) {
    if (is(op, "add")) { for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) mpq_add(go[c][i], gp[c][i], gq[c][i]); return; }
    if (is(op, "norm")) { mpq_set_ui(gs, 0, 1); for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) { mpq_mul(gt, gp[c][i], gp[c][i]); mpq_add(gs, gs, gt); } return; }
#define GPX(a) gp[a][i]
#define GQX(b) gq[b][i]
#define GR(z) go[z][i]
    for (long i = 0; i < N; i++) OCTLIB(QMUL, QADDMUL, QSUBMUL, GPX, GQX, GR);
}

/* ---------------------------------------------------------------- FLINT integers over the shared denominator */
static fmpz *zp[8], *zq[8], *zo[8]; static fmpz_t zs;
static void fmpz_setup(void) {
    for (int c = 0; c < 8; c++) {
        zp[c] = _fmpz_vec_init(N); zq[c] = _fmpz_vec_init(N); zo[c] = _fmpz_vec_init(N);
        for (long i = 0; i < N; i++) { fmpz_set_si(zp[c] + i, P[c][i]); fmpz_set_si(zq[c] + i, Q[c][i]); }
    }
    fmpz_init(zs);
}
static void fmpz_run(const char *op) {
    if (is(op, "add")) { for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) fmpz_add(zo[c] + i, zp[c] + i, zq[c] + i); return; }
    if (is(op, "norm")) { fmpz_zero(zs); for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) fmpz_addmul(zs, zp[c] + i, zp[c] + i); return; }
#define ZPX(a) (zp[a] + i)
#define ZQX(b) (zq[b] + i)
#define ZR(z) (zo[z] + i)
    for (long i = 0; i < N; i++) OCTLIB(fmpz_mul, fmpz_addmul, fmpz_submul, ZPX, ZQX, ZR);
}

/* ---------------------------------------------------------------- checked int64 */
static int ovf;
static void i64_run(const char *op) {
    int o = 0;
    if (is(op, "add")) { for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) o |= __builtin_add_overflow(P[c][i], Q[c][i], &o64[c][i]); }
    else if (is(op, "norm")) {
        int64_t s = 0;
        for (int c = 0; c < 8; c++) for (long i = 0; i < N; i++) { int64_t t; o |= __builtin_mul_overflow(P[c][i], P[c][i], &t); o |= __builtin_add_overflow(s, t, &s); }
        vnorm = s;
    } else {
        for (long i = 0; i < N; i++) OCTCHK(PX, QX, R64, o);      /* the same written-out product, checked */
    }
    ovf = o;
}

static void pr128(FILE *f, i128 v) {
    char buf[64]; int k = 63; buf[k] = 0; int neg = v < 0;
    unsigned __int128 u = neg ? -(unsigned __int128)v : (unsigned __int128)v;
    do { buf[--k] = '0' + (int)(u % 10); u /= 10; } while (u);
    if (neg) buf[--k] = '-';
    fputs(buf + k, f);
}

int main(int argc, char **argv) {
    if (argc != 8) { fprintf(stderr, "usage: oct METHOD OP N DEN REPS INDIR OUTFILE\n"); return 2; }
    const char *m = argv[1], *op = argv[2];
    N = atol(argv[3]); DEN = atol(argv[4]); int reps = atoi(argv[5]);
    for (int c = 0; c < 8; c++) {
        char nm[16];
        snprintf(nm, sizeof nm, "p%d.bin", c); P[c] = load(argv[6], nm);
        snprintf(nm, sizeof nm, "q%d.bin", c); Q[c] = load(argv[6], nm);
        o64[c] = malloc(sizeof(int64_t) * N); o128[c] = malloc(sizeof(i128) * N);
    }
    int kind = is(m, "vec") ? 0 : is(m, "mpq") ? 1 : is(m, "fmpz") ? 3 : is(m, "i64c") ? 4 : -1;
    if (kind < 0) { fprintf(stderr, "unknown method %s\n", m); return 2; }
    if (kind == 1) gmp_setup(); else if (kind == 3) fmpz_setup();
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        switch (kind) { case 0: vec_run(op); break; case 1: gmp_run(op); break; case 3: fmpz_run(op); break; default: i64_run(op); }
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g\n", t[reps / 2], t[0]);
    FILE *f = fopen(argv[7], "w");
    mpz_t d; mpz_init(d);
    mpz_ui_pow_ui(d, DEN, is(op, "add") ? 1 : 2);
    if (kind == 4 && ovf) fprintf(f, "overflow\n");
    else if (is(op, "norm")) {
        if (kind == 1) gmp_fprintf(f, "%Qd\n", gs);
        else if (kind == 3) { char *s = fmpz_get_str(NULL, 10, zs); gmp_fprintf(f, "%s/%Zd\n", s, d); flint_free(s); }
        else { pr128(f, vnorm); gmp_fprintf(f, "/%Zd\n", d); }
    } else {
        for (long i = 0; i < N; i++)
            for (int c = 0; c < 8; c++) {
                if (kind == 1) gmp_fprintf(f, "%Qd", go[c][i]);
                else if (kind == 3) { char *s = fmpz_get_str(NULL, 10, zo[c] + i); gmp_fprintf(f, "%s/%Zd", s, d); flint_free(s); }
                else { if (kind == 0 && wide) pr128(f, o128[c][i]); else pr128(f, o64[c][i]); gmp_fprintf(f, "/%Zd", d); }
                fputc(c < 7 ? ' ' : '\n', f);
            }
    }
    fclose(f);
    return 0;
}
