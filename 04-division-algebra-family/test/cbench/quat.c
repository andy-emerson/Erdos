/* Exact quaternion arrays: the vector form against GMP and FLINT.

   usage: quat METHOD OP N DEN REPS INDIR OUTFILE
     METHOD  vec     four int64 component vectors over the shared denominator DEN; products are computed in
                     int64 when the operands' bit lengths prove they fit, otherwise in __int128 (one check per
                     operation, as in the vector form)
             mpq     (a, b, c, d) tuples of GMP rationals
             fmpq    tuples of FLINT rationals
             fmpz    four FLINT integers over the shared denominator per quaternion (Sage's layout)
             i64c    int64 components over DEN, every operation overflow-checked
     OP      mul     elementwise Hamilton product p*q
             norm    the sum of |p|^2 = a^2 + b^2 + c^2 + d^2
             rot     q v q* (two Hamilton products, q* the conjugate): rotating vectors v by quaternions q
             add     elementwise p + q
   INDIR holds pa.bin pb.bin pc.bin pd.bin qa.bin ... (components of p and q) and va.bin ... (v; va is zero).
   Prints "median min"; writes results as "a b c d" fractions per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/fmpz_vec.h>
#include <flint/fmpq_vec.h>

typedef __int128 i128;
static long N, DEN;
static int64_t *P[4], *Q[4], *V[4];
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

/* The Hamilton product as a table: e_x e_y = SIGN[x][y] e_{AXIS[x][y]}, axes 0..3 = 1, i, j, k. */
static const int AXIS[4][4] = {{0, 1, 2, 3}, {1, 0, 3, 2}, {2, 3, 0, 1}, {3, 2, 1, 0}};
static const int SIGN[4][4] = {{1, 1, 1, 1}, {1, -1, 1, -1}, {1, -1, -1, 1}, {1, 1, -1, -1}};

/* ---------------------------------------------------------------- the vector form */
static int bits_of(int64_t *x[4]) {
    uint64_t acc = 0;
    for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) { int64_t v = x[c][i]; acc |= v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; }
    return acc ? 64 - __builtin_clzll(acc) : 0;
}
static int64_t *o64[4]; static i128 *o128[4]; static int wide; static i128 vnorm;
/* p*q, written out from the table (the compiler sees constant signs) */
#define HAMILTON(T, a1, b1, c1, d1, a2, b2, c2, d2, ra, rb, rc, rd) do { \
    ra = (T)a1 * a2 - (T)b1 * b2 - (T)c1 * c2 - (T)d1 * d2; \
    rb = (T)a1 * b2 + (T)b1 * a2 + (T)c1 * d2 - (T)d1 * c2; \
    rc = (T)a1 * c2 - (T)b1 * d2 + (T)c1 * a2 + (T)d1 * b2; \
    rd = (T)a1 * d2 + (T)b1 * c2 - (T)c1 * b2 + (T)d1 * a2; } while (0)
static void vec_run(const char *op) {
    if (is(op, "add")) {
        int bp = bits_of(P), bq = bits_of(Q);
        wide = (bp > bq ? bp : bq) + 1 > 63;                 /* a sum needs one bit more than its larger operand */
        if (!wide) for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) o64[c][i] = P[c][i] + Q[c][i];
        else for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) o128[c][i] = (i128)P[c][i] + Q[c][i];
        return;
    }
    if (is(op, "norm")) {
        int b = bits_of(P);
        int lg = 0; long n = N; while (n) { n >>= 1; lg++; }
        if (2 * b + 2 + lg > 127) { fprintf(stderr, "norm needs more than 128 bits\n"); exit(3); }
        if (2 * b + 2 + lg <= 63) { int64_t s = 0; for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) s += P[c][i] * P[c][i]; vnorm = s; }
        else { i128 s = 0; for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) s += (i128)P[c][i] * P[c][i]; vnorm = s; }
        return;
    }
    if (is(op, "mul")) {
        int bp = bits_of(P), bq = bits_of(Q);
        if (bp + bq + 2 > 127) { fprintf(stderr, "mul needs more than 128 bits\n"); exit(3); }
        wide = bp + bq + 2 > 63;                            /* four products and three sums */
        if (!wide) for (long i = 0; i < N; i++)
            HAMILTON(int64_t, P[0][i], P[1][i], P[2][i], P[3][i], Q[0][i], Q[1][i], Q[2][i], Q[3][i], o64[0][i], o64[1][i], o64[2][i], o64[3][i]);
        else for (long i = 0; i < N; i++)
            HAMILTON(i128, P[0][i], P[1][i], P[2][i], P[3][i], Q[0][i], Q[1][i], Q[2][i], Q[3][i], o128[0][i], o128[1][i], o128[2][i], o128[3][i]);
        return;
    }
    /* rot: q v q*, with q* = (a, -b, -c, -d); the result is over DEN^3; computed in __int128 when it fits */
    int bq = bits_of(P), bv = bits_of(V);
    wide = 1;
    if (2 * bq + bv + 4 > 127) { fprintf(stderr, "rot needs more than 128 bits\n"); exit(3); }
    for (long i = 0; i < N; i++) {
        i128 ta, tb, tc, td;
        HAMILTON(i128, P[0][i], P[1][i], P[2][i], P[3][i], V[0][i], V[1][i], V[2][i], V[3][i], ta, tb, tc, td);
        i128 qa = P[0][i], qb = -(i128)P[1][i], qc = -(i128)P[2][i], qd = -(i128)P[3][i];
        HAMILTON(i128, ta, tb, tc, td, qa, qb, qc, qd, o128[0][i], o128[1][i], o128[2][i], o128[3][i]);
    }
}

/* ---------------------------------------------------------------- GMP rational 4-tuples */
static mpq_t *gp[4], *gq[4], *gv[4], *go[4], gt, gu, gs, gtmp[4];
static void gmp_setup(void) {
    for (int c = 0; c < 4; c++) {
        gp[c] = malloc(sizeof(mpq_t) * N); gq[c] = malloc(sizeof(mpq_t) * N); gv[c] = malloc(sizeof(mpq_t) * N); go[c] = malloc(sizeof(mpq_t) * N);
        for (long i = 0; i < N; i++) {
            mpq_init(gp[c][i]); mpq_init(gq[c][i]); mpq_init(gv[c][i]); mpq_init(go[c][i]);
            mpq_set_si(gp[c][i], P[c][i], DEN); mpq_canonicalize(gp[c][i]);
            mpq_set_si(gq[c][i], Q[c][i], DEN); mpq_canonicalize(gq[c][i]);
            mpq_set_si(gv[c][i], V[c][i], DEN); mpq_canonicalize(gv[c][i]);
        }
        mpq_init(gtmp[c]);
    }
    mpq_init(gt); mpq_init(gu); mpq_init(gs);
}
/* the Hamilton product written out with constant signs, as in the vector form; t is a temporary */
#define QHAM(MUL, ADD, SUB, SET, t, r0, r1, r2, r3, a1, b1, c1, d1, a2, b2, c2, d2) do { \
    MUL(r0, a1, a2); MUL(t, b1, b2); SUB(r0, r0, t); MUL(t, c1, c2); SUB(r0, r0, t); MUL(t, d1, d2); SUB(r0, r0, t); \
    MUL(r1, a1, b2); MUL(t, b1, a2); ADD(r1, r1, t); MUL(t, c1, d2); ADD(r1, r1, t); MUL(t, d1, c2); SUB(r1, r1, t); \
    MUL(r2, a1, c2); MUL(t, b1, d2); SUB(r2, r2, t); MUL(t, c1, a2); ADD(r2, r2, t); MUL(t, d1, b2); ADD(r2, r2, t); \
    MUL(r3, a1, d2); MUL(t, b1, c2); ADD(r3, r3, t); MUL(t, c1, b2); SUB(r3, r3, t); MUL(t, d1, a2); ADD(r3, r3, t); } while (0)
static mpq_t gr[4], gn[4];
static void gmp_run(const char *op) {
    if (is(op, "add")) { for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) mpq_add(go[c][i], gp[c][i], gq[c][i]); return; }
    if (is(op, "norm")) { mpq_set_ui(gs, 0, 1); for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) { mpq_mul(gt, gp[c][i], gp[c][i]); mpq_add(gs, gs, gt); } return; }
    if (is(op, "mul")) {
        for (long i = 0; i < N; i++)
            QHAM(mpq_mul, mpq_add, mpq_sub, mpq_set, gt, go[0][i], go[1][i], go[2][i], go[3][i],
                 gp[0][i], gp[1][i], gp[2][i], gp[3][i], gq[0][i], gq[1][i], gq[2][i], gq[3][i]);
        return;
    }
    for (long i = 0; i < N; i++) {          /* rot: (q v) q*, with q* = (a, -b, -c, -d) */
        QHAM(mpq_mul, mpq_add, mpq_sub, mpq_set, gt, gr[0], gr[1], gr[2], gr[3],
             gp[0][i], gp[1][i], gp[2][i], gp[3][i], gv[0][i], gv[1][i], gv[2][i], gv[3][i]);
        mpq_neg(gn[1], gp[1][i]); mpq_neg(gn[2], gp[2][i]); mpq_neg(gn[3], gp[3][i]);
        QHAM(mpq_mul, mpq_add, mpq_sub, mpq_set, gt, go[0][i], go[1][i], go[2][i], go[3][i],
             gr[0], gr[1], gr[2], gr[3], gp[0][i], gn[1], gn[2], gn[3]);
    }
}

/* ---------------------------------------------------------------- FLINT rational 4-tuples */
static fmpq *fp[4], *fq[4], *fv[4], *fo[4]; static fmpq_t ft, fs, fr[4], ftmp[4];
static void fmpq_setup(void) {
    for (int c = 0; c < 4; c++) {
        fp[c] = _fmpq_vec_init(N); fq[c] = _fmpq_vec_init(N); fv[c] = _fmpq_vec_init(N); fo[c] = _fmpq_vec_init(N);
        for (long i = 0; i < N; i++) { fmpq_set_si(fp[c] + i, P[c][i], DEN); fmpq_set_si(fq[c] + i, Q[c][i], DEN); fmpq_set_si(fv[c] + i, V[c][i], DEN); }
        fmpq_init(fr[c]); fmpq_init(ftmp[c]);
    }
    fmpq_init(ft); fmpq_init(fs);
}
static void fmpq_run(const char *op) {
    if (is(op, "add")) { for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) fmpq_add(fo[c] + i, fp[c] + i, fq[c] + i); return; }
    if (is(op, "norm")) { fmpq_zero(fs); for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) { fmpq_mul(ft, fp[c] + i, fp[c] + i); fmpq_add(fs, fs, ft); } return; }
    if (is(op, "mul")) {
        for (long i = 0; i < N; i++)
            QHAM(fmpq_mul, fmpq_add, fmpq_sub, fmpq_set, ft, fo[0] + i, fo[1] + i, fo[2] + i, fo[3] + i,
                 fp[0] + i, fp[1] + i, fp[2] + i, fp[3] + i, fq[0] + i, fq[1] + i, fq[2] + i, fq[3] + i);
        return;
    }
    for (long i = 0; i < N; i++) {
        QHAM(fmpq_mul, fmpq_add, fmpq_sub, fmpq_set, ft, fr[0], fr[1], fr[2], fr[3],
             fp[0] + i, fp[1] + i, fp[2] + i, fp[3] + i, fv[0] + i, fv[1] + i, fv[2] + i, fv[3] + i);
        fmpq_neg(ftmp[1], fp[1] + i); fmpq_neg(ftmp[2], fp[2] + i); fmpq_neg(ftmp[3], fp[3] + i);
        QHAM(fmpq_mul, fmpq_add, fmpq_sub, fmpq_set, ft, fo[0] + i, fo[1] + i, fo[2] + i, fo[3] + i,
             fr[0], fr[1], fr[2], fr[3], fp[0] + i, ftmp[1], ftmp[2], ftmp[3]);
    }
}

/* ---------------------------------------------------------------- FLINT integers over the shared denominator */
static fmpz *zp[4], *zq[4], *zv[4], *zo[4]; static fmpz_t zs, zt, zr[4], ztmp[4];
static fmpz *zt4[4];
static void fmpz_setup(void) {
    for (int c = 0; c < 4; c++) {
        zp[c] = _fmpz_vec_init(N); zq[c] = _fmpz_vec_init(N); zv[c] = _fmpz_vec_init(N); zo[c] = _fmpz_vec_init(N);
        for (long i = 0; i < N; i++) { fmpz_set_si(zp[c] + i, P[c][i]); fmpz_set_si(zq[c] + i, Q[c][i]); fmpz_set_si(zv[c] + i, V[c][i]); }
        fmpz_init(zr[c]); fmpz_init(ztmp[c]);
        zt4[c] = _fmpz_vec_init(N);
    }
    fmpz_init(zs); fmpz_init(zt);
}
#define FHAM(r0, r1, r2, r3, a1, b1, c1, d1, a2, b2, c2, d2) do { \
    fmpz_mul(r0, a1, a2); fmpz_submul(r0, b1, b2); fmpz_submul(r0, c1, c2); fmpz_submul(r0, d1, d2); \
    fmpz_mul(r1, a1, b2); fmpz_addmul(r1, b1, a2); fmpz_addmul(r1, c1, d2); fmpz_submul(r1, d1, c2); \
    fmpz_mul(r2, a1, c2); fmpz_submul(r2, b1, d2); fmpz_addmul(r2, c1, a2); fmpz_addmul(r2, d1, b2); \
    fmpz_mul(r3, a1, d2); fmpz_addmul(r3, b1, c2); fmpz_submul(r3, c1, b2); fmpz_addmul(r3, d1, a2); } while (0)
static void fmpz_run(const char *op) {
    if (is(op, "add")) { for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) fmpz_add(zo[c] + i, zp[c] + i, zq[c] + i); return; }
    if (is(op, "norm")) { fmpz_zero(zs); for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) fmpz_addmul(zs, zp[c] + i, zp[c] + i); return; }
    if (is(op, "mul")) {
        for (long i = 0; i < N; i++)
            FHAM(zo[0] + i, zo[1] + i, zo[2] + i, zo[3] + i, zp[0] + i, zp[1] + i, zp[2] + i, zp[3] + i,
                 zq[0] + i, zq[1] + i, zq[2] + i, zq[3] + i);
        return;
    }
    for (long i = 0; i < N; i++) {
        FHAM(zt4[0] + i, zt4[1] + i, zt4[2] + i, zt4[3] + i, zp[0] + i, zp[1] + i, zp[2] + i, zp[3] + i,
             zv[0] + i, zv[1] + i, zv[2] + i, zv[3] + i);
        fmpz_neg(ztmp[1], zp[1] + i); fmpz_neg(ztmp[2], zp[2] + i); fmpz_neg(ztmp[3], zp[3] + i);
        FHAM(zo[0] + i, zo[1] + i, zo[2] + i, zo[3] + i, zt4[0] + i, zt4[1] + i, zt4[2] + i, zt4[3] + i,
             zp[0] + i, ztmp[1], ztmp[2], ztmp[3]);
    }
}

/* ---------------------------------------------------------------- checked int64 */
static int ovf;
static void i64_run(const char *op) {
    int o = 0;
    if (is(op, "add")) { for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) o |= __builtin_add_overflow(P[c][i], Q[c][i], &o64[c][i]); }
    else if (is(op, "norm")) {
        int64_t s = 0;
        for (int c = 0; c < 4; c++) for (long i = 0; i < N; i++) { int64_t t; o |= __builtin_mul_overflow(P[c][i], P[c][i], &t); o |= __builtin_add_overflow(s, t, &s); }
        vnorm = s;
    } else if (is(op, "mul")) {
        for (long i = 0; i < N; i++) {
            int64_t acc[4] = {0, 0, 0, 0};
            for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) {
                int64_t t; o |= __builtin_mul_overflow(P[a][i], Q[b][i], &t);
                if (SIGN[a][b] > 0) o |= __builtin_add_overflow(acc[AXIS[a][b]], t, &acc[AXIS[a][b]]);
                else o |= __builtin_sub_overflow(acc[AXIS[a][b]], t, &acc[AXIS[a][b]]);
            }
            for (int c = 0; c < 4; c++) o64[c][i] = acc[c];
        }
    } else {   /* rot: (q v) q*, every product and sum checked */
        for (long i = 0; i < N; i++) {
            int64_t t[4] = {0, 0, 0, 0}, r[4] = {0, 0, 0, 0};
            for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) {
                int64_t u; o |= __builtin_mul_overflow(P[a][i], V[b][i], &u);
                if (SIGN[a][b] > 0) o |= __builtin_add_overflow(t[AXIS[a][b]], u, &t[AXIS[a][b]]);
                else o |= __builtin_sub_overflow(t[AXIS[a][b]], u, &t[AXIS[a][b]]);
            }
            for (int a = 0; a < 4; a++) for (int b = 0; b < 4; b++) {
                int64_t u, qb = b ? -P[b][i] : P[b][i];
                o |= __builtin_mul_overflow(t[a], qb, &u);
                if (SIGN[a][b] > 0) o |= __builtin_add_overflow(r[AXIS[a][b]], u, &r[AXIS[a][b]]);
                else o |= __builtin_sub_overflow(r[AXIS[a][b]], u, &r[AXIS[a][b]]);
            }
            for (int c = 0; c < 4; c++) o64[c][i] = r[c];
        }
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
    if (argc != 8) { fprintf(stderr, "usage: quat METHOD OP N DEN REPS INDIR OUTFILE\n"); return 2; }
    const char *m = argv[1], *op = argv[2];
    N = atol(argv[3]); DEN = atol(argv[4]); int reps = atoi(argv[5]);
    const char *names = "abcd";
    for (int c = 0; c < 4; c++) {
        char nm[8];
        snprintf(nm, sizeof nm, "p%c.bin", names[c]); P[c] = load(argv[6], nm);
        snprintf(nm, sizeof nm, "q%c.bin", names[c]); Q[c] = load(argv[6], nm);
        snprintf(nm, sizeof nm, "v%c.bin", names[c]); V[c] = load(argv[6], nm);
        o64[c] = malloc(sizeof(int64_t) * N); o128[c] = malloc(sizeof(i128) * N);
    }
    int kind = is(m, "vec") ? 0 : is(m, "mpq") ? 1 : is(m, "fmpq") ? 2 : is(m, "fmpz") ? 3 : 4;
    if (kind == 1) gmp_setup(); else if (kind == 2) fmpq_setup(); else if (kind == 3) fmpz_setup();
    if (kind == 1) for (int c = 0; c < 4; c++) { mpq_init(gr[c]); mpq_init(gn[c]); }
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        switch (kind) { case 0: vec_run(op); break; case 1: gmp_run(op); break; case 2: fmpq_run(op); break; case 3: fmpz_run(op); break; default: i64_run(op); }
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g\n", t[reps / 2], t[0]);
    FILE *f = fopen(argv[7], "w");
    mpz_t d; mpz_init(d);
    long dpow = is(op, "add") ? 1 : is(op, "rot") ? 3 : 2;
    mpz_ui_pow_ui(d, DEN, dpow);
    if (kind == 4 && ovf) fprintf(f, "overflow\n");
    else if (is(op, "norm")) {
        if (kind == 1) gmp_fprintf(f, "%Qd\n", gs);
        else if (kind == 2) { char *s = fmpq_get_str(NULL, 10, fs); fprintf(f, "%s\n", s); flint_free(s); }
        else if (kind == 3) { char *s = fmpz_get_str(NULL, 10, zs); gmp_fprintf(f, "%s/%Zd\n", s, d); flint_free(s); }
        else { pr128(f, vnorm); gmp_fprintf(f, "/%Zd\n", d); }
    } else {
        for (long i = 0; i < N; i++) {
            for (int c = 0; c < 4; c++) {
                if (kind == 1) gmp_fprintf(f, "%Qd", go[c][i]);
                else if (kind == 2) { char *s = fmpq_get_str(NULL, 10, fo[c] + i); fputs(s, f); flint_free(s); }
                else if (kind == 3) { char *s = fmpz_get_str(NULL, 10, zo[c] + i); gmp_fprintf(f, "%s/%Zd", s, d); flint_free(s); }
                else { if (kind == 0 && wide) pr128(f, o128[c][i]); else pr128(f, o64[c][i]); gmp_fprintf(f, "/%Zd", d); }
                fputc(c < 3 ? ' ' : '\n', f);
            }
        }
    }
    fclose(f);
    return 0;
}
