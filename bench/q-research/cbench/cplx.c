/* Compiled baselines for exact complex arrays.

   usage: cplx CONTENDER OP N DEN REPS INDIR OUTFILE
     CONTENDER  mpq     (re, im) pairs of GMP rationals
                fmpq    (re, im) pairs of FLINT rationals
                fmpzi   FLINT Gaussian integers over the shared denominator DEN
                i64c    int64 (re, im) numerators over DEN, every operation overflow-checked
     OP         mul  z = x*y       fma  z = x*y + w       dot  sum x*y       abs2  sum |x|^2
     INDIR      xr.bin xi.bin yr.bin yi.bin wr.bin wi.bin (int64 numerators over DEN)
   Prints "median_seconds min_seconds"; writes results as "re im" lines of fractions (over DEN^2 for products). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/fmpq_vec.h>
#include <flint/fmpzi.h>

static long N, DEN;
static int64_t *XR, *XI, *YR, *YI, *WR, *WI;
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

/* ---------------------------------------------------------------- GMP pairs */
static mpq_t *gxr, *gxi, *gyr, *gyi, *gwr, *gwi, *gzr, *gzi, gsr, gsi, t1, t2;
static void gmp_setup(void) {
    mpq_t **arrs[8] = { &gxr, &gxi, &gyr, &gyi, &gwr, &gwi, &gzr, &gzi };
    int64_t *src[6] = { XR, XI, YR, YI, WR, WI };
    for (int k = 0; k < 8; k++) {
        *arrs[k] = malloc(sizeof(mpq_t) * N);
        for (long i = 0; i < N; i++) {
            mpq_init((*arrs[k])[i]);
            if (k < 6) { mpq_set_si((*arrs[k])[i], src[k][i], DEN); mpq_canonicalize((*arrs[k])[i]); }
        }
    }
    mpq_init(gsr); mpq_init(gsi); mpq_init(t1); mpq_init(t2);
}
static void gmp_mul(mpq_t zr, mpq_t zi, mpq_t ar, mpq_t ai, mpq_t br, mpq_t bi) {
    mpq_mul(t1, ar, br); mpq_mul(t2, ai, bi); mpq_sub(zr, t1, t2);
    mpq_mul(t1, ar, bi); mpq_mul(t2, ai, br); mpq_add(zi, t1, t2);
}
static void gmp_op(const char *op) {
    if (is(op, "mul")) for (long i = 0; i < N; i++) gmp_mul(gzr[i], gzi[i], gxr[i], gxi[i], gyr[i], gyi[i]);
    else if (is(op, "fma")) for (long i = 0; i < N; i++) {
        gmp_mul(gzr[i], gzi[i], gxr[i], gxi[i], gyr[i], gyi[i]);
        mpq_add(gzr[i], gzr[i], gwr[i]); mpq_add(gzi[i], gzi[i], gwi[i]);
    } else if (is(op, "dot")) {
        mpq_set_ui(gsr, 0, 1); mpq_set_ui(gsi, 0, 1);
        for (long i = 0; i < N; i++) {
            mpq_mul(t1, gxr[i], gyr[i]); mpq_add(gsr, gsr, t1); mpq_mul(t1, gxi[i], gyi[i]); mpq_sub(gsr, gsr, t1);
            mpq_mul(t1, gxr[i], gyi[i]); mpq_add(gsi, gsi, t1); mpq_mul(t1, gxi[i], gyr[i]); mpq_add(gsi, gsi, t1);
        }
    } else {
        mpq_set_ui(gsr, 0, 1); mpq_set_ui(gsi, 0, 1);
        for (long i = 0; i < N; i++) { mpq_mul(t1, gxr[i], gxr[i]); mpq_add(gsr, gsr, t1); mpq_mul(t1, gxi[i], gxi[i]); mpq_add(gsr, gsr, t1); }
    }
}
static void gmp_write(const char *op, FILE *f) {
    if (is(op, "mul") || is(op, "fma")) for (long i = 0; i < N; i++) gmp_fprintf(f, "%Qd %Qd\n", gzr[i], gzi[i]);
    else gmp_fprintf(f, "%Qd %Qd\n", gsr, gsi);
}

/* ---------------------------------------------------------------- FLINT rational pairs */
static fmpq *fxr, *fxi, *fyr, *fyi, *fwr, *fwi, *fzr, *fzi; static fmpq_t fsr, fsi, f1, f2;
static void fmpq_setup(void) {
    fmpq **arrs[8] = { &fxr, &fxi, &fyr, &fyi, &fwr, &fwi, &fzr, &fzi };
    int64_t *src[6] = { XR, XI, YR, YI, WR, WI };
    for (int k = 0; k < 8; k++) {
        *arrs[k] = _fmpq_vec_init(N);
        if (k < 6) for (long i = 0; i < N; i++) fmpq_set_si(*arrs[k] + i, src[k][i], DEN);
    }
    fmpq_init(fsr); fmpq_init(fsi); fmpq_init(f1); fmpq_init(f2);
}
static void fq_mul(fmpq *zr, fmpq *zi, const fmpq *ar, const fmpq *ai, const fmpq *br, const fmpq *bi) {
    fmpq_mul(f1, ar, br); fmpq_mul(f2, ai, bi); fmpq_sub(zr, f1, f2);
    fmpq_mul(f1, ar, bi); fmpq_mul(f2, ai, br); fmpq_add(zi, f1, f2);
}
static void fmpq_op(const char *op) {
    if (is(op, "mul")) for (long i = 0; i < N; i++) fq_mul(fzr + i, fzi + i, fxr + i, fxi + i, fyr + i, fyi + i);
    else if (is(op, "fma")) for (long i = 0; i < N; i++) {
        fq_mul(fzr + i, fzi + i, fxr + i, fxi + i, fyr + i, fyi + i);
        fmpq_add(fzr + i, fzr + i, fwr + i); fmpq_add(fzi + i, fzi + i, fwi + i);
    } else if (is(op, "dot")) {
        fmpq_zero(fsr); fmpq_zero(fsi);
        for (long i = 0; i < N; i++) {
            fmpq_mul(f1, fxr + i, fyr + i); fmpq_add(fsr, fsr, f1); fmpq_mul(f1, fxi + i, fyi + i); fmpq_sub(fsr, fsr, f1);
            fmpq_mul(f1, fxr + i, fyi + i); fmpq_add(fsi, fsi, f1); fmpq_mul(f1, fxi + i, fyr + i); fmpq_add(fsi, fsi, f1);
        }
    } else {
        fmpq_zero(fsr); fmpq_zero(fsi);
        for (long i = 0; i < N; i++) { fmpq_mul(f1, fxr + i, fxr + i); fmpq_add(fsr, fsr, f1); fmpq_mul(f1, fxi + i, fxi + i); fmpq_add(fsr, fsr, f1); }
    }
}
static void fq_print(FILE *f, const fmpq *a, const fmpq *b) {
    char *s = fmpq_get_str(NULL, 10, a), *t = fmpq_get_str(NULL, 10, b);
    fprintf(f, "%s %s\n", s, t); flint_free(s); flint_free(t);
}
static void fmpq_write(const char *op, FILE *f) {
    if (is(op, "mul") || is(op, "fma")) for (long i = 0; i < N; i++) fq_print(f, fzr + i, fzi + i);
    else fq_print(f, fsr, fsi);
}

/* ---------------------------------------------------------------- FLINT Gaussian integers over DEN */
static fmpzi_struct *zx, *zy, *zw, *zz; static fmpzi_t zs, zt; static fmpz_t dd;
static void fmpzi_setup(void) {
    zx = malloc(sizeof(fmpzi_struct) * N); zy = malloc(sizeof(fmpzi_struct) * N);
    zw = malloc(sizeof(fmpzi_struct) * N); zz = malloc(sizeof(fmpzi_struct) * N);
    for (long i = 0; i < N; i++) {
        fmpzi_init(zx + i); fmpzi_init(zy + i); fmpzi_init(zw + i); fmpzi_init(zz + i);
        fmpzi_set_si_si(zx + i, XR[i], XI[i]); fmpzi_set_si_si(zy + i, YR[i], YI[i]);
        fmpzi_set_si_si(zw + i, WR[i], WI[i]);
    }
    fmpzi_init(zs); fmpzi_init(zt); fmpz_init_set_si(dd, DEN);
}
static void fmpzi_op(const char *op) {
    if (is(op, "mul")) for (long i = 0; i < N; i++) fmpzi_mul(zz + i, zx + i, zy + i);
    else if (is(op, "fma")) for (long i = 0; i < N; i++) {     /* w is scaled to DEN^2 inside the timed loop */
        fmpzi_mul(zz + i, zx + i, zy + i);
        fmpz_addmul_si(fmpzi_realref(zz + i), fmpzi_realref(zw + i), DEN);
        fmpz_addmul_si(fmpzi_imagref(zz + i), fmpzi_imagref(zw + i), DEN);
    }
    else if (is(op, "dot")) { fmpzi_zero(zs); for (long i = 0; i < N; i++) { fmpzi_mul(zt, zx + i, zy + i); fmpzi_add(zs, zs, zt); } }
    else {
        fmpzi_zero(zs);
        for (long i = 0; i < N; i++) {
            fmpz_addmul(fmpzi_realref(zs), fmpzi_realref(zx + i), fmpzi_realref(zx + i));
            fmpz_addmul(fmpzi_realref(zs), fmpzi_imagref(zx + i), fmpzi_imagref(zx + i));
        }
    }
}
static void zi_print(FILE *f, const fmpz *a, const fmpz *b, long dpow) {
    fmpz_t d; fmpz_init(d); fmpz_pow_ui(d, dd, dpow);
    char *s = fmpz_get_str(NULL, 10, a), *t = fmpz_get_str(NULL, 10, b), *u = fmpz_get_str(NULL, 10, d);
    fprintf(f, "%s/%s %s/%s\n", s, u, t, u); flint_free(s); flint_free(t); flint_free(u); fmpz_clear(d);
}
static void fmpzi_write(const char *op, FILE *f) {
    if (is(op, "mul") || is(op, "fma")) for (long i = 0; i < N; i++) zi_print(f, fmpzi_realref(zz + i), fmpzi_imagref(zz + i), 2);
    else zi_print(f, fmpzi_realref(zs), fmpzi_imagref(zs), 2);
}

/* ---------------------------------------------------------------- checked int64 pairs over DEN */
static int64_t *ozr, *ozi, osr, osi; static int ovf;
static void i64_setup(void) { ozr = malloc(sizeof(int64_t) * N); ozi = malloc(sizeof(int64_t) * N); }
static void i64_op(const char *op) {
    int o = 0;
    if (is(op, "mul") || is(op, "fma")) for (long i = 0; i < N; i++) {
        int64_t a, b, c, d;
        o |= __builtin_mul_overflow(XR[i], YR[i], &a); o |= __builtin_mul_overflow(XI[i], YI[i], &b);
        o |= __builtin_mul_overflow(XR[i], YI[i], &c); o |= __builtin_mul_overflow(XI[i], YR[i], &d);
        o |= __builtin_sub_overflow(a, b, &ozr[i]); o |= __builtin_add_overflow(c, d, &ozi[i]);
        if (is(op, "fma")) {
            int64_t e, g;
            o |= __builtin_mul_overflow(WR[i], DEN, &e); o |= __builtin_mul_overflow(WI[i], DEN, &g);
            o |= __builtin_add_overflow(ozr[i], e, &ozr[i]); o |= __builtin_add_overflow(ozi[i], g, &ozi[i]);
        }
    } else if (is(op, "dot")) {
        int64_t sr = 0, si = 0;
        for (long i = 0; i < N; i++) {
            int64_t a, b, c, d;
            o |= __builtin_mul_overflow(XR[i], YR[i], &a); o |= __builtin_mul_overflow(XI[i], YI[i], &b);
            o |= __builtin_mul_overflow(XR[i], YI[i], &c); o |= __builtin_mul_overflow(XI[i], YR[i], &d);
            o |= __builtin_add_overflow(sr, a, &sr); o |= __builtin_sub_overflow(sr, b, &sr);
            o |= __builtin_add_overflow(si, c, &si); o |= __builtin_add_overflow(si, d, &si);
        }
        osr = sr; osi = si;
    } else {
        int64_t sr = 0;
        for (long i = 0; i < N; i++) {
            int64_t a, b;
            o |= __builtin_mul_overflow(XR[i], XR[i], &a); o |= __builtin_mul_overflow(XI[i], XI[i], &b);
            o |= __builtin_add_overflow(sr, a, &sr); o |= __builtin_add_overflow(sr, b, &sr);
        }
        osr = sr; osi = 0;
    }
    ovf = o;
}
static void i64_write(const char *op, FILE *f) {
    if (ovf) { fprintf(f, "overflow\n"); return; }
    long long d2 = (long long)DEN * DEN;
    if (is(op, "mul") || is(op, "fma")) for (long i = 0; i < N; i++) fprintf(f, "%lld/%lld %lld/%lld\n", (long long)ozr[i], d2, (long long)ozi[i], d2);
    else fprintf(f, "%lld/%lld %lld/%lld\n", (long long)osr, d2, (long long)osi, d2);
}

int main(int argc, char **argv) {
    if (argc != 8) { fprintf(stderr, "usage: cplx CONTENDER OP N DEN REPS INDIR OUTFILE\n"); return 2; }
    const char *who = argv[1], *op = argv[2], *dir = argv[6];
    N = atol(argv[3]); DEN = atol(argv[4]); int reps = atoi(argv[5]);
    XR = load(dir, "xr.bin"); XI = load(dir, "xi.bin"); YR = load(dir, "yr.bin"); YI = load(dir, "yi.bin");
    WR = load(dir, "wr.bin"); WI = load(dir, "wi.bin");
    int kind = is(who, "mpq") ? 0 : is(who, "fmpq") ? 1 : is(who, "fmpzi") ? 2 : 3;
    switch (kind) { case 0: gmp_setup(); break; case 1: fmpq_setup(); break; case 2: fmpzi_setup(); break; default: i64_setup(); }
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        switch (kind) { case 0: gmp_op(op); break; case 1: fmpq_op(op); break; case 2: fmpzi_op(op); break; default: i64_op(op); }
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g\n", t[reps / 2], t[0]);
    FILE *f = fopen(argv[7], "w");
    switch (kind) { case 0: gmp_write(op, f); break; case 1: fmpq_write(op, f); break; case 2: fmpzi_write(op, f); break; default: i64_write(op, f); }
    fclose(f);
    return 0;
}
