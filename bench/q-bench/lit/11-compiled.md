# Compiled baselines

The baselines that matter most for the vector form are compiled ones: GMP and FLINT called from C pay no interpreter cost per element. `cbench.c` builds its inputs before timing, preallocates and reuses its outputs, times each operation as the median of several repetitions after one warm-up, and writes every result as text. `cbase.py` writes the inputs, runs the program, and the benchmark checks every result line against an exact reference before it is reported. The overflow-checked int64 contender uses the compiler's checked arithmetic, and reports an overflow rather than a result when one occurs.

```c {file=cbench/cbench.c}
/* Compiled baselines for exact column arithmetic, so that the vector form is compared with GMP and FLINT
   called from C, not through Python object arrays.

   usage: cbench CONTENDER OP N DEN G REPS INDIR OUTFILE
     CONTENDER  mpq | fmpq            per-element rationals (DEN 0: per-element denominators in a_q.bin, ...)
                i64w                  int64 numerators, overflow checked; on overflow the whole operation is
                                      redone in __int128 (the vector form's strategy, compiled)
                mpz | fmpz            integer numerators over the shared denominator DEN
                i64 | i64c            int64 numerators over DEN, unchecked / with every overflow checked
     OP         sum | fma | group | var | chain (x <- x*b + c, G times; mpq and fmpq only)
                | sq ((a*b)^2; mpq, mpz and fmpz only)
     INDIR      holds a.bin, b.bin, c.bin (int64 numerators), keys.bin (int64), and for DEN 0 a_q.bin, ...
   Inputs are built before timing; outputs are preallocated and reused, as a C program would.
   Prints "median_seconds min_seconds"; writes the result (as text) to OUTFILE for an independent check. */
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
#include <flint/fmpz_vec.h>

static int64_t *load(const char *dir, const char *name, long n) {
    char path[4096];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    int64_t *x = malloc(sizeof(int64_t) * (n ? n : 1));
    if (fread(x, sizeof(int64_t), n, f) != (size_t)n) { fprintf(stderr, "short read %s\n", path); exit(2); }
    fclose(f);
    return x;
}

static double now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

static int cmpd(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return x < y ? -1 : x > y;
}

static long N, G;
static long DEN;
static int64_t *A, *B, *C, *AQ, *BQ, *CQ, *KEYS;

/* ---------------------------------------------------------------- GMP rationals */
static mpq_t *qa, *qb, *qc, *qo, *qg, qs, qs2, qt;
static void mpq_setup(void) {
    qa = malloc(sizeof(mpq_t) * N); qb = malloc(sizeof(mpq_t) * N); qc = malloc(sizeof(mpq_t) * N);
    qo = malloc(sizeof(mpq_t) * N); qg = malloc(sizeof(mpq_t) * G);
    for (long i = 0; i < N; i++) {
        mpq_init(qa[i]); mpq_init(qb[i]); mpq_init(qc[i]); mpq_init(qo[i]);
        mpq_set_si(qa[i], A[i], DEN ? DEN : AQ[i]); mpq_canonicalize(qa[i]);
        mpq_set_si(qb[i], B[i], DEN ? DEN : BQ[i]); mpq_canonicalize(qb[i]);
        mpq_set_si(qc[i], C[i], DEN ? DEN : CQ[i]); mpq_canonicalize(qc[i]);
    }
    for (long g = 0; g < G; g++) mpq_init(qg[g]);
    mpq_init(qs); mpq_init(qs2); mpq_init(qt);
}
static void mpq_op(const char *op) {
    if (!strcmp(op, "sum")) {
        mpq_set_ui(qs, 0, 1);
        for (long i = 0; i < N; i++) mpq_add(qs, qs, qa[i]);
    } else if (!strcmp(op, "fma")) {
        for (long i = 0; i < N; i++) { mpq_mul(qt, qa[i], qb[i]); mpq_add(qo[i], qt, qc[i]); }
    } else if (!strcmp(op, "sq")) {
        for (long i = 0; i < N; i++) { mpq_mul(qo[i], qa[i], qb[i]); mpq_mul(qo[i], qo[i], qo[i]); }
    } else if (!strcmp(op, "chain")) {
        for (long i = 0; i < N; i++) mpq_set(qo[i], qa[i]);
        for (long k = 0; k < G; k++)
            for (long i = 0; i < N; i++) { mpq_mul(qo[i], qo[i], qb[i]); mpq_add(qo[i], qo[i], qc[i]); }
    } else if (!strcmp(op, "group")) {
        for (long g = 0; g < G; g++) mpq_set_ui(qg[g], 0, 1);
        for (long i = 0; i < N; i++) mpq_add(qg[KEYS[i]], qg[KEYS[i]], qa[i]);
    } else {  /* var: (n*S2 - S1^2) / (n(n-1)) */
        mpq_set_ui(qs, 0, 1); mpq_set_ui(qs2, 0, 1);
        for (long i = 0; i < N; i++) { mpq_add(qs, qs, qa[i]); mpq_mul(qt, qa[i], qa[i]); mpq_add(qs2, qs2, qt); }
        mpq_set_si(qt, N, 1); mpq_mul(qs2, qs2, qt); mpq_mul(qt, qs, qs); mpq_sub(qs2, qs2, qt);
        mpq_set_si(qt, N * (N - 1), 1); mpq_div(qs, qs2, qt);
    }
}
static void mpq_write(const char *op, FILE *f) {
    if (!strcmp(op, "sum") || !strcmp(op, "var")) gmp_fprintf(f, "%Qd\n", qs);
    else if (!strcmp(op, "fma") || !strcmp(op, "chain") || !strcmp(op, "sq")) for (long i = 0; i < N; i++) gmp_fprintf(f, "%Qd\n", qo[i]);
    else for (long g = 0; g < G; g++) gmp_fprintf(f, "%Qd\n", qg[g]);
}

/* ---------------------------------------------------------------- GMP integers over the shared denominator */
static mpz_t *za, *zb, *zc, *zo, *zg, zs, zs2, zt;
static void mpz_setup(void) {
    za = malloc(sizeof(mpz_t) * N); zb = malloc(sizeof(mpz_t) * N); zc = malloc(sizeof(mpz_t) * N);
    zo = malloc(sizeof(mpz_t) * N); zg = malloc(sizeof(mpz_t) * G);
    for (long i = 0; i < N; i++) {
        mpz_init_set_si(za[i], A[i]); mpz_init_set_si(zb[i], B[i]); mpz_init_set_si(zc[i], C[i]); mpz_init(zo[i]);
    }
    for (long g = 0; g < G; g++) mpz_init(zg[g]);
    mpz_init(zs); mpz_init(zs2); mpz_init(zt);
}
static void mpz_op(const char *op) {
    if (!strcmp(op, "sum")) {
        mpz_set_ui(zs, 0);
        for (long i = 0; i < N; i++) mpz_add(zs, zs, za[i]);
    } else if (!strcmp(op, "fma")) {  /* numerators over DEN^2: a*b + c*DEN */
        for (long i = 0; i < N; i++) { mpz_mul_si(zo[i], zc[i], DEN); mpz_addmul(zo[i], za[i], zb[i]); }
    } else if (!strcmp(op, "sq")) {   /* numerators over DEN^4 */
        for (long i = 0; i < N; i++) { mpz_mul(zo[i], za[i], zb[i]); mpz_mul(zo[i], zo[i], zo[i]); }
    } else if (!strcmp(op, "group")) {
        for (long g = 0; g < G; g++) mpz_set_ui(zg[g], 0);
        for (long i = 0; i < N; i++) mpz_add(zg[KEYS[i]], zg[KEYS[i]], za[i]);
    } else {  /* numerator of var over n(n-1) DEN^2 */
        mpz_set_ui(zs, 0); mpz_set_ui(zs2, 0);
        for (long i = 0; i < N; i++) { mpz_add(zs, zs, za[i]); mpz_addmul(zs2, za[i], za[i]); }
        mpz_mul_si(zs2, zs2, N); mpz_submul(zs2, zs, zs);
    }
}
static void mpz_write(const char *op, FILE *f) {
    if (!strcmp(op, "sum")) gmp_fprintf(f, "%Zd/%ld\n", zs, DEN);
    else if (!strcmp(op, "var")) {
        mpz_set_si(zt, N * (N - 1)); mpz_mul_si(zt, zt, DEN); mpz_mul_si(zt, zt, DEN);
        gmp_fprintf(f, "%Zd/%Zd\n", zs2, zt);
    }
    else if (!strcmp(op, "fma") || !strcmp(op, "sq")) {
        mpz_set_si(zt, DEN); mpz_pow_ui(zt, zt, !strcmp(op, "sq") ? 4 : 2);
        for (long i = 0; i < N; i++) gmp_fprintf(f, "%Zd/%Zd\n", zo[i], zt);
    }
    else for (long g = 0; g < G; g++) gmp_fprintf(f, "%Zd/%ld\n", zg[g], DEN);
}

/* ---------------------------------------------------------------- FLINT rationals */
static fmpq *fa, *fb, *fc, *fo, *fg;
static fmpq_t fs, fs2, ft;
static fmpz_t fnn;
static void fmpq_setup(void) {
    fa = _fmpq_vec_init(N); fb = _fmpq_vec_init(N); fc = _fmpq_vec_init(N); fo = _fmpq_vec_init(N);
    fg = _fmpq_vec_init(G);
    for (long i = 0; i < N; i++) {
        fmpq_set_si(fa + i, A[i], DEN ? DEN : AQ[i]);
        fmpq_set_si(fb + i, B[i], DEN ? DEN : BQ[i]);
        fmpq_set_si(fc + i, C[i], DEN ? DEN : CQ[i]);
    }
    fmpq_init(fs); fmpq_init(fs2); fmpq_init(ft); fmpz_init_set_si(fnn, N * (N - 1));
}
static void fmpq_op(const char *op) {
    if (!strcmp(op, "sum")) {
        fmpq_zero(fs);
        for (long i = 0; i < N; i++) fmpq_add(fs, fs, fa + i);
    } else if (!strcmp(op, "fma")) {
        for (long i = 0; i < N; i++) { fmpq_mul(ft, fa + i, fb + i); fmpq_add(fo + i, ft, fc + i); }
    } else if (!strcmp(op, "chain")) {
        for (long i = 0; i < N; i++) fmpq_set(fo + i, fa + i);
        for (long k = 0; k < G; k++)
            for (long i = 0; i < N; i++) { fmpq_mul(fo + i, fo + i, fb + i); fmpq_add(fo + i, fo + i, fc + i); }
    } else if (!strcmp(op, "group")) {
        for (long g = 0; g < G; g++) fmpq_zero(fg + g);
        for (long i = 0; i < N; i++) fmpq_add(fg + KEYS[i], fg + KEYS[i], fa + i);
    } else {
        fmpq_zero(fs); fmpq_zero(fs2);
        for (long i = 0; i < N; i++) { fmpq_add(fs, fs, fa + i); fmpq_mul(ft, fa + i, fa + i); fmpq_add(fs2, fs2, ft); }
        fmpq_mul_si(fs2, fs2, N); fmpq_mul(ft, fs, fs); fmpq_sub(fs2, fs2, ft);
        fmpq_div_fmpz(fs, fs2, fnn);
    }
}
static void fmpq_write(const char *op, FILE *f) {
    char *s;
    if (!strcmp(op, "sum") || !strcmp(op, "var")) { s = fmpq_get_str(NULL, 10, fs); fprintf(f, "%s\n", s); flint_free(s); }
    else if (!strcmp(op, "fma") || !strcmp(op, "chain")) for (long i = 0; i < N; i++) { s = fmpq_get_str(NULL, 10, fo + i); fprintf(f, "%s\n", s); flint_free(s); }
    else for (long g = 0; g < G; g++) { s = fmpq_get_str(NULL, 10, fg + g); fprintf(f, "%s\n", s); flint_free(s); }
}

/* ---------------------------------------------------------------- FLINT integers (small values stored inline) */
static fmpz *ia, *ib, *ic, *io, *ig;
static fmpz_t is, is2, it;
static void fmpz_setup(void) {
    ia = _fmpz_vec_init(N); ib = _fmpz_vec_init(N); ic = _fmpz_vec_init(N); io = _fmpz_vec_init(N);
    ig = _fmpz_vec_init(G);
    for (long i = 0; i < N; i++) { fmpz_set_si(ia + i, A[i]); fmpz_set_si(ib + i, B[i]); fmpz_set_si(ic + i, C[i]); }
    fmpz_init(is); fmpz_init(is2); fmpz_init(it);
}
static void fmpz_op(const char *op) {
    if (!strcmp(op, "sum")) {
        fmpz_zero(is);
        for (long i = 0; i < N; i++) fmpz_add(is, is, ia + i);
    } else if (!strcmp(op, "fma")) {
        for (long i = 0; i < N; i++) { fmpz_mul_si(io + i, ic + i, DEN); fmpz_addmul(io + i, ia + i, ib + i); }
    } else if (!strcmp(op, "sq")) {
        for (long i = 0; i < N; i++) { fmpz_mul(io + i, ia + i, ib + i); fmpz_mul(io + i, io + i, io + i); }
    } else if (!strcmp(op, "group")) {
        for (long g = 0; g < G; g++) fmpz_zero(ig + g);
        for (long i = 0; i < N; i++) fmpz_add(ig + KEYS[i], ig + KEYS[i], ia + i);
    } else {
        fmpz_zero(is); fmpz_zero(is2);
        for (long i = 0; i < N; i++) { fmpz_add(is, is, ia + i); fmpz_addmul(is2, ia + i, ia + i); }
        fmpz_mul_si(is2, is2, N); fmpz_submul(is2, is, is);
    }
}
static void fmpz_write(const char *op, FILE *f) {
    char *s;
    if (!strcmp(op, "sum")) { s = fmpz_get_str(NULL, 10, is); fprintf(f, "%s/%ld\n", s, DEN); flint_free(s); }
    else if (!strcmp(op, "var")) {
        s = fmpz_get_str(NULL, 10, is2);
        fmpz_set_si(it, N * (N - 1)); fmpz_mul_si(it, it, DEN); fmpz_mul_si(it, it, DEN);
        char *t = fmpz_get_str(NULL, 10, it);
        fprintf(f, "%s/%s\n", s, t); flint_free(s); flint_free(t);
    }
    else if (!strcmp(op, "fma") || !strcmp(op, "sq")) {
        fmpz_set_si(it, DEN); fmpz_pow_ui(it, it, !strcmp(op, "sq") ? 4 : 2);
        char *t = fmpz_get_str(NULL, 10, it);
        for (long i = 0; i < N; i++) { s = fmpz_get_str(NULL, 10, io + i); fprintf(f, "%s/%s\n", s, t); flint_free(s); }
        flint_free(t);
    }
    else for (long g = 0; g < G; g++) { s = fmpz_get_str(NULL, 10, ig + g); fprintf(f, "%s/%ld\n", s, DEN); flint_free(s); }
}

/* ---------------------------------------------------------------- int64 numerators */
static int64_t *xo, *xg, xs;
static __int128 *wo, *wg, ws, xs2;
static int overflowed, widened;
static void i64_setup(void) {
    xo = malloc(sizeof(int64_t) * N); xg = malloc(sizeof(int64_t) * G);
    wo = malloc(sizeof(__int128) * N); wg = malloc(sizeof(__int128) * G);
}
/* mode 0 unchecked, 1 checked (report overflow), 2 checked then redone in __int128 on overflow */
static void i64_op(const char *op, int mode) {
    int o = 0;
    widened = 0;
    if (!strcmp(op, "sum")) {
        int64_t s = 0;
        if (mode == 0) for (long i = 0; i < N; i++) s += A[i];
        else for (long i = 0; i < N; i++) o |= __builtin_add_overflow(s, A[i], &s);
        xs = s;
        if (o && mode == 2) { __int128 w = 0; for (long i = 0; i < N; i++) w += A[i]; ws = w; widened = 1; o = 0; }
    } else if (!strcmp(op, "fma")) {
        if (mode == 0) for (long i = 0; i < N; i++) xo[i] = A[i] * B[i] + C[i] * DEN;
        else for (long i = 0; i < N; i++) {
            int64_t p, q;
            o |= __builtin_mul_overflow(A[i], B[i], &p);
            o |= __builtin_mul_overflow(C[i], (int64_t)DEN, &q);
            o |= __builtin_add_overflow(p, q, &xo[i]);
        }
        if (o && mode == 2) {
            for (long i = 0; i < N; i++) wo[i] = (__int128)A[i] * B[i] + (__int128)C[i] * DEN;
            widened = 1; o = 0;
        }
    } else if (!strcmp(op, "group")) {
        memset(xg, 0, sizeof(int64_t) * G);
        if (mode == 0) for (long i = 0; i < N; i++) xg[KEYS[i]] += A[i];
        else for (long i = 0; i < N; i++) o |= __builtin_add_overflow(xg[KEYS[i]], A[i], &xg[KEYS[i]]);
        if (o && mode == 2) {
            memset(wg, 0, sizeof(__int128) * G);
            for (long i = 0; i < N; i++) wg[KEYS[i]] += A[i];
            widened = 1; o = 0;
        }
    } else if (!strcmp(op, "var")) {   /* int64 sum, __int128 sum of squares, as a C programmer would */
        int64_t s = 0;
        __int128 s2 = 0;
        if (mode == 0) for (long i = 0; i < N; i++) { s += A[i]; s2 += (__int128)A[i] * A[i]; }
        else for (long i = 0; i < N; i++) {
            o |= __builtin_add_overflow(s, A[i], &s);
            o |= __builtin_add_overflow(s2, (__int128)A[i] * A[i], &s2);
        }
        if (o && mode == 2) {
            __int128 w = 0; s2 = 0;
            for (long i = 0; i < N; i++) { w += A[i]; s2 += (__int128)A[i] * A[i]; }
            ws = w; widened = 1; o = 0;
            xs2 = (__int128)N * s2 - w * w;
        } else {
            xs = s;
            xs2 = (__int128)N * s2 - (__int128)s * s;
        }
    } else { fprintf(stderr, "int64 contenders do not implement %s\n", op); exit(2); }
    overflowed = (mode != 0) && o;
}
static void print128(FILE *f, __int128 v) {
    char buf[64]; int k = 63; buf[k] = 0;
    int neg = v < 0; unsigned __int128 u = neg ? -(unsigned __int128)v : (unsigned __int128)v;
    do { buf[--k] = '0' + (int)(u % 10); u /= 10; } while (u);
    if (neg) buf[--k] = '-';
    fputs(buf + k, f);
}
static void i64_write(const char *op, FILE *f) {
    if (overflowed) { fprintf(f, "overflow\n"); return; }
    __int128 d2 = (__int128)DEN * DEN;
    if (!strcmp(op, "sum")) { print128(f, widened ? ws : xs); fprintf(f, "/%ld\n", DEN); }
    else if (!strcmp(op, "var")) { print128(f, xs2); fprintf(f, "/"); print128(f, (__int128)N * (N - 1) * d2); fprintf(f, "\n"); }
    else if (!strcmp(op, "fma")) for (long i = 0; i < N; i++) { print128(f, widened ? wo[i] : xo[i]); fputc('/', f); print128(f, d2); fputc('\n', f); }
    else for (long g = 0; g < G; g++) { print128(f, widened ? wg[g] : xg[g]); fprintf(f, "/%ld\n", DEN); }
}

int main(int argc, char **argv) {
    if (argc != 9) { fprintf(stderr, "usage: cbench CONTENDER OP N DEN G REPS INDIR OUTFILE\n"); return 2; }
    const char *who = argv[1], *op = argv[2], *dir = argv[7];
    N = atol(argv[3]); DEN = atol(argv[4]); G = atol(argv[5]);
    int reps = atoi(argv[6]);
    A = load(dir, "a.bin", N); B = load(dir, "b.bin", N); C = load(dir, "c.bin", N); KEYS = load(dir, "keys.bin", N);
    if (DEN == 0) { AQ = load(dir, "a_q.bin", N); BQ = load(dir, "b_q.bin", N); CQ = load(dir, "c_q.bin", N); }
    int kind = !strcmp(who, "mpq") ? 0 : !strcmp(who, "mpz") ? 1 : !strcmp(who, "fmpq") ? 2 : !strcmp(who, "fmpz") ? 3
             : !strcmp(who, "i64") ? 4 : !strcmp(who, "i64c") ? 5 : 6;
    if (DEN == 0 && kind != 0 && kind != 2) { fprintf(stderr, "%s needs a shared denominator\n", who); return 2; }
    if (!strcmp(op, "chain") && kind != 0 && kind != 2) { fprintf(stderr, "chain is for mpq and fmpq\n"); return 2; }
    if (!strcmp(op, "sq") && kind != 0 && kind != 1 && kind != 3) { fprintf(stderr, "sq is for mpq, mpz, fmpz\n"); return 2; }
    switch (kind) { case 0: mpq_setup(); break; case 1: mpz_setup(); break; case 2: fmpq_setup(); break;
                    case 3: fmpz_setup(); break; default: i64_setup(); }
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {   /* one untimed warm-up */
        double t0 = now();
        switch (kind) { case 0: mpq_op(op); break; case 1: mpz_op(op); break; case 2: fmpq_op(op); break;
                        case 3: fmpz_op(op); break; case 4: i64_op(op, 0); break; case 5: i64_op(op, 1); break;
                        default: i64_op(op, 2); }
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g\n", t[reps / 2], t[0]);
    FILE *f = fopen(argv[8], "w");
    switch (kind) { case 0: mpq_write(op, f); break; case 1: mpz_write(op, f); break; case 2: fmpq_write(op, f); break;
                    case 3: fmpz_write(op, f); break; default: i64_write(op, f); }
    fclose(f);
    return 0;
}
```

```python {file=build/cbase.py}
"""Runs the compiled baselines (cbench/cbench.c) and checks every result they write against an exact reference."""
import subprocess
import tempfile
from fractions import Fraction
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
BIN = HERE.parent / "cbench" / "cbench"
SRC = HERE.parent / "cbench" / "cbench.c"

NAMES = {"mpq": "GMP mpq_t (C)", "fmpq": "FLINT fmpq (C)", "mpz": "GMP mpz_t numerators (C)",
         "fmpz": "FLINT fmpz numerators (C)", "i64": "C int64, unchecked", "i64c": "C int64, overflow-checked",
         "i64w": "C int64, checked, widening to int128"}


def build():
    if not BIN.exists() or BIN.stat().st_mtime < SRC.stat().st_mtime:
        subprocess.run(["gcc", "-O3", "-march=native", "-Wall", "-o", str(BIN), str(SRC), "-lflint", "-lgmp"],
                       check=True)


class Stats:
    """The harness's Stats shape: median, min, spread, noisy, times, loops. The compiled program reports only its
    median and minimum, so spread here is (median - min) / median, and times holds the median alone."""

    def __init__(self, median, minimum):
        self.median, self.min, self.times, self.loops = median, minimum, [median], 1
        self.spread = (median - minimum) / median if median else 0.0
        self.noisy = self.spread > 0.1


class Inputs:
    """Numerator columns (and optional per-element denominators) written once for all contenders."""

    def __init__(self, a, b, c, keys, den, a_q=None, b_q=None, c_q=None):
        self.dir = tempfile.TemporaryDirectory()
        d = Path(self.dir.name)
        for name, x in (("a", a), ("b", b), ("c", c), ("keys", keys), ("a_q", a_q), ("b_q", b_q), ("c_q", c_q)):
            if x is not None:
                np.ascontiguousarray(x, dtype=np.int64).tofile(d / f"{name}.bin")
        self.n, self.den = len(a), int(den)

    def run(self, who, op, groups=1, reps=7):
        out = Path(self.dir.name) / f"out-{who}-{op}.txt"
        r = subprocess.run([str(BIN), who, op, str(self.n), str(self.den), str(groups), str(reps),
                            self.dir.name, str(out)], capture_output=True, text=True, check=True)
        med, mn = map(float, r.stdout.split())
        lines = out.read_text().split()
        return Stats(med, mn), lines


def parse(lines):
    """Results as Fractions; None for an overflow report."""
    if lines == ["overflow"]:
        return None
    return [Fraction(s) for s in lines]
```
