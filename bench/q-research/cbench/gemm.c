/* Exact matrix products with wide entries: multimodular float BLAS against FLINT.

   usage: gemm METHOD N BITS REPS [OUTFILE]
     A and B are N x N integer matrices with random signed entries of up to BITS bits (numerators over one shared
     denominator, the vector form; the denominator of the product is the square and needs no work).
     METHOD  fmpz    FLINT fmpz_mat_mul (chooses classical, Strassen or its own multimodular method by size)
             fmpq    FLINT fmpq_mat_mul on the same matrices with denominator 1000 per entry, reduced
             mm      multimodular float BLAS: residues modulo primes p with n (p/2)^2 < 2^53, held as balanced
                     doubles, so that each product modulo p is one exact OpenBLAS dgemm; then the Chinese
                     remainder theorem per entry (FLINT's fmpz_multi_mod_ui and fmpz_multi_CRT_ui)
             mmg     the same products, with reduction and reconstruction also done as dgemms on 16-bit digits
                     of entries held in the uN fixed-limb layout (see mmg below); output in that layout
             mmga    mmg with everything inside the timed region: choosing primes, tables and buffers, converting
                     the fmpz inputs to limbs and the result back to fmpz, and freeing
             fmpzb   (built as gemm_fb against FLINT 3.0.1 configured with BLAS) fmpz_mat_mul, which may then
                     choose FLINT's own BLAS method
             blas    (gemm_fb) fmpz_mat_mul_blas: FLINT's multimodular method on BLAS dgemm, the same idea as mm
   Each method also prints, last, how far its peak resident memory rose (MB) above the inputs and libraries
   (the classical reference at N <= 128 also counts, a few MB).
   Prints "median min" seconds; for mm also the median split "reduce gemm crt" and the number of primes.
   Every method's result is compared before timing with FLINT's classical product (N <= 128) or FLINT's default
   product (larger N); a mismatch exits 4. GEMM_EDGE=1 or 2 in the environment sets extreme entries instead;
   GEMM_CORRUPT adds 1 to one entry before the check, which must then fail (a test of the check). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <flint/fmpz_vec.h>
#include <flint/fmpq.h>
#include <flint/fmpq_mat.h>
#include <flint/ulong_extras.h>

/* OpenBLAS as bundled with scipy (symbols prefixed scipy_), row-major = 101, no transpose = 111 */
void scipy_cblas_dgemm(int order, int ta, int tb, int m, int n, int k, double alpha, const double *a, int lda,
                       const double *b, int ldb, double beta, double *c, int ldc);
void scipy_openblas_set_num_threads(int);

/* peak resident memory of this process image in MB (VmHWM; unlike ru_maxrss it is not inherited from the parent) */
static double hwm_mb(void) {
    FILE *f = fopen("/proc/self/status", "r"); char line[256]; double kb = 0;
    while (f && fgets(line, sizeof line, f)) if (!strncmp(line, "VmHWM:", 6)) kb = atof(line + 6);
    if (f) fclose(f);
    return kb / 1024.0;
}
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }

static long n; static int bits;
static fmpz_mat_t A, B, Cz, Cref;
static fmpq_mat_t Aq, Bq, Cq;

/* ---------------------------------------------------------------- multimodular float BLAS */
static mp_limb_t *primes; static long np;
static fmpz_comb_t comb; static fmpz_comb_temp_t ctemp;
static double *Ad, *Bd, *Cd;          /* [prime][n*n] */
static mp_limb_t *res;                /* residues of one entry, one per prime */
static double tsplit[3];
static void mm_setup(void) {
    /* largest p with n (p/2)^2 < 2^53: exact sums of n products of balanced residues in a double */
    double pmax = 2.0 * sqrt(ldexp(1.0, 53) / (double)n);
    mp_limb_t p = (mp_limb_t)pmax;
    double need = 2.0 * bits + log2((double)n) + 2;      /* |C| < n 2^(2 bits); one more bit for the sign */
    double have = 0; long cap = 64;
    primes = malloc(sizeof(mp_limb_t) * cap); np = 0;
    while (have < need) {
        do p--; while (!n_is_prime(p));                  /* the next prime below */
        if (np == cap) { cap *= 2; primes = realloc(primes, sizeof(mp_limb_t) * cap); }
        primes[np++] = p; have += log2((double)p);
    }
    fmpz_comb_init(comb, primes, np); fmpz_comb_temp_init(ctemp, comb);
    Ad = malloc(sizeof(double) * np * n * n); Bd = malloc(sizeof(double) * np * n * n); Cd = malloc(sizeof(double) * np * n * n);
    res = malloc(sizeof(mp_limb_t) * np);
}
static void to_balanced(double *dst, const fmpz_mat_t M) {
    for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) {
        fmpz_multi_mod_ui(res, fmpz_mat_entry(M, i, j), comb, ctemp);
        for (long k = 0; k < np; k++) {
            mp_limb_t r = res[k], p = primes[k];
            dst[k * n * n + i * n + j] = r > p / 2 ? (double)r - (double)p : (double)r;
        }
    }
}
static void mm_run(void) {
    double t0 = now();
    to_balanced(Ad, A); to_balanced(Bd, B);
    double t1 = now();
    for (long k = 0; k < np; k++)
        scipy_cblas_dgemm(101, 111, 111, n, n, n, 1.0, Ad + k * n * n, n, Bd + k * n * n, n, 0.0, Cd + k * n * n, n);
    double t2 = now();
    for (long i = 0; i < n * n; i++) {
        for (long k = 0; k < np; k++) {
            double p = (double)primes[k], v = fmod(Cd[k * n * n + i], p);
            if (v < 0) v += p;
            res[k] = (mp_limb_t)v;
        }
        fmpz_multi_CRT_ui(fmpz_mat_entry(Cz, i / n, i % n), res, comb, ctemp, 1);
    }
    double t3 = now();
    tsplit[0] = t1 - t0; tsplit[1] = t2 - t1; tsplit[2] = t3 - t2;
}

/* ---------------------------------------------------------------- mmg: conversions as GEMMs too
   Entries live in the uN fixed-limb layout (a sign and LA limbs each, contiguous), the vector form's own layout.
   Reduction: split every entry into 16-bit digits; residues = digits x T with T[j][k] = 2^(16 j) mod p_k, one
   dgemm for all entries and primes. Reconstruction: y_k = r_k (M/p_k)^-1 mod p_k; x = sum y_k (M/p_k) is one
   dgemm against the 16-bit digits of M/p_k, then carries, x - floor(sum y_k/p_k) M, and the balanced sign.
   Every dgemm is exact: each bound below is asserted against 2^53. */
static long LA, LC, LM, nd, ndc;
static mp_limb_t *Al, *Bl, *Cl, *Ml, *Mhalf, *xt, *mt; static unsigned char *As, *Bs, *Cs;
static double *Dg, *T, *Mkd, *Yd, *Xd, *pinv; static mp_limb_t *inv;
static void to_limbs(mp_limb_t *dst, unsigned char *sg, const fmpz_mat_t M) {
    mpz_t z; mpz_init(z);
    for (long i = 0; i < n * n; i++) {
        fmpz_get_mpz(z, fmpz_mat_entry(M, i / n, i % n));
        sg[i] = mpz_sgn(z) < 0; mpz_abs(z, z);
        memset(dst + i * LA, 0, LA * sizeof(mp_limb_t));
        mpz_export(dst + i * LA, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    }
    mpz_clear(z);
}
static void mmg_setup(void) {
    mm_setup();
    LA = (bits + 63) / 64; nd = 4 * LA;
    fmpz_t M, Mk; fmpz_init(M); fmpz_init(Mk); fmpz_one(M);
    for (long k = 0; k < np; k++) fmpz_mul_ui(M, M, primes[k]);
    long mbits = fmpz_bits(M);
    LM = (mbits + 63) / 64; LC = LM + 1; ndc = (mbits + 15) / 16 + 1;
    double lp = log2((double)primes[0]);
    if (16 + lp + log2((double)nd) >= 53 || 16 + lp + log2((double)np) >= 53) { fprintf(stderr, "bound\n"); exit(5); }
    Al = malloc(sizeof(mp_limb_t) * n * n * LA); Bl = malloc(sizeof(mp_limb_t) * n * n * LA);
    As = malloc(n * n); Bs = malloc(n * n); Cs = malloc(n * n);
    Cl = calloc((size_t)n * n * LC, sizeof(mp_limb_t));
    to_limbs(Al, As, A); to_limbs(Bl, Bs, B);
    Dg = malloc(sizeof(double) * n * n * nd); T = malloc(sizeof(double) * nd * np);
    for (long k = 0; k < np; k++) { mp_limb_t t = 1; for (long j = 0; j < nd; j++) { T[j * np + k] = (double)t; t = n_mulmod2(t, 65536, primes[k]); } }
    Mkd = calloc((size_t)np * ndc, sizeof(double)); inv = malloc(sizeof(mp_limb_t) * np); pinv = malloc(sizeof(double) * np);
    mpz_t z; mpz_init(z);
    for (long k = 0; k < np; k++) {
        fmpz_divexact_ui(Mk, M, primes[k]);
        inv[k] = n_invmod(fmpz_fdiv_ui(Mk, primes[k]), primes[k]);
        pinv[k] = 1.0 / (double)primes[k];
        fmpz_get_mpz(z, Mk);
        for (long j = 0; j < ndc; j++) { Mkd[k * ndc + j] = (double)(mpz_getlimbn(z, j / 4) >> (16 * (j % 4)) & 0xffff); }
    }
    Ml = calloc(LC, sizeof(mp_limb_t)); Mhalf = calloc(LC, sizeof(mp_limb_t));
    fmpz_get_mpz(z, M); mpz_export(Ml, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    mpz_fdiv_q_2exp(z, z, 1); mpz_export(Mhalf, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    mpz_clear(z); fmpz_clear(M); fmpz_clear(Mk);
    Yd = malloc(sizeof(double) * np * n * n); Xd = malloc(sizeof(double) * n * n * ndc);
    xt = malloc(sizeof(mp_limb_t) * (LC + 2)); mt = malloc(sizeof(mp_limb_t) * (LC + 2));
}
/* v mod p for an integer-valued double 0 <= |v| < 2^53: the quotient from 1/p may be off by one, so correct once */
static inline double fmodp(double v, double p, double pi) {
    double r = v - floor(v * pi) * p;
    if (r < 0) r += p; else if (r >= p) r -= p;
    return r;
}
static void reduce_gemm(double *dst, const mp_limb_t *L, const unsigned char *sg) {
    long nn = n * n;
    for (long i = 0; i < nn; i++) for (long j = 0; j < nd; j++) Dg[i * nd + j] = (double)(L[i * LA + j / 4] >> (16 * (j % 4)) & 0xffff);
    /* dst (np x nn) = T^T (np x nd) . Dg^T (nd x nn) */
    scipy_cblas_dgemm(101, 112, 112, np, nn, nd, 1.0, T, np, Dg, nd, 0.0, dst, nn);
    for (long k = 0; k < np; k++) {
        double p = (double)primes[k], h = (double)(primes[k] / 2), pi = pinv[k];
        for (long i = 0; i < nn; i++) {
            double r = fmodp(dst[k * nn + i], p, pi);
            if (sg[i] && r != 0) r = p - r;
            dst[k * nn + i] = r > h ? r - p : r;
        }
    }
}
static void mmg_run(void) {
    long nn = n * n;
    double t0 = now();
    reduce_gemm(Ad, Al, As); reduce_gemm(Bd, Bl, Bs);
    double t1 = now();
    for (long k = 0; k < np; k++)
        scipy_cblas_dgemm(101, 111, 111, n, n, n, 1.0, Ad + k * nn, n, Bd + k * nn, n, 0.0, Cd + k * nn, n);
    double t2 = now();
    for (long k = 0; k < np; k++) {
        double p = (double)primes[k], pi = pinv[k]; mp_limb_t pk = primes[k], ik = inv[k];
        for (long i = 0; i < nn; i++) {
            double r = fmodp(Cd[k * nn + i], p, pi);
            Yd[k * nn + i] = (double)(((mp_limb_t)r * ik) % pk);
        }
    }
    /* Xd (nn x ndc) = Yd^T (nn x np) . Mkd (np x ndc) */
    scipy_cblas_dgemm(101, 112, 111, nn, ndc, np, 1.0, Yd, nn, Mkd, ndc, 0.0, Xd, ndc);
    for (long i = 0; i < nn; i++) {
        memset(xt, 0, sizeof(mp_limb_t) * (LC + 2));
        uint64_t carry = 0;
        for (long j = 0; j < ndc; j++) {
            carry += (uint64_t)Xd[i * ndc + j];
            xt[j / 4] |= (carry & 0xffff) << (16 * (j % 4));
            carry >>= 16;
        }
        for (long j = ndc; carry; j++) { xt[j / 4] |= (carry & 0xffff) << (16 * (j % 4)); carry >>= 16; }
        double s = 0; for (long k = 0; k < np; k++) s += Yd[k * nn + i] * pinv[k];
        mp_limb_t q = (mp_limb_t)floor(s);
        mp_limb_t hi = mpn_mul_1(mt, Ml, LC, q); mt[LC] = hi; xt[LC] = xt[LC];
        if (mpn_sub_n(xt, xt, mt, LC + 1)) mpn_add_n(xt, xt, Ml, LC + 1);       /* q one too big */
        while (mpn_cmp(xt, Ml, LC) >= 0 && !xt[LC]) mpn_sub_n(xt, xt, Ml, LC);  /* q one too small */
        mp_limb_t *c = Cl + i * LC;
        if (mpn_cmp(xt, Mhalf, LC) > 0) { mpn_sub_n(c, Ml, xt, LC); Cs[i] = 1; }
        else { memcpy(c, xt, LC * sizeof(mp_limb_t)); Cs[i] = 0; }
    }
    double t3 = now();
    tsplit[0] = t1 - t0; tsplit[1] = t2 - t1; tsplit[2] = t3 - t2;
}
static void mmg_free(void) {
    free(Al); free(Bl); free(As); free(Bs); free(Cs); free(Cl); free(Dg); free(T); free(Mkd); free(inv); free(pinv);
    free(Ml); free(Mhalf); free(Yd); free(Xd); free(xt); free(mt);
    free(Ad); free(Bd); free(Cd); free(res); free(primes);
    fmpz_comb_temp_clear(ctemp); fmpz_comb_clear(comb);
}
/* Freivalds: C x == A (B x) for a random vector x of 30-bit entries; an independent check of any product */
static int freivalds(const fmpz_mat_t C, flint_rand_t st) {
    fmpz *x = _fmpz_vec_init(n), *y = _fmpz_vec_init(n), *z = _fmpz_vec_init(n), *w = _fmpz_vec_init(n);
    int ok = 1;
    for (int rep = 0; rep < 2 && ok; rep++) {
        for (long i = 0; i < n; i++) fmpz_set_ui(x + i, n_randbits(st, 30));
        for (long i = 0; i < n; i++) { fmpz_zero(y + i); for (long j = 0; j < n; j++) fmpz_addmul(y + i, fmpz_mat_entry(B, i, j), x + j); }
        for (long i = 0; i < n; i++) { fmpz_zero(z + i); for (long j = 0; j < n; j++) fmpz_addmul(z + i, fmpz_mat_entry(A, i, j), y + j); }
        for (long i = 0; i < n; i++) { fmpz_zero(w + i); for (long j = 0; j < n; j++) fmpz_addmul(w + i, fmpz_mat_entry(C, i, j), x + j); }
        ok = _fmpz_vec_equal(z, w, n);
    }
    _fmpz_vec_clear(x, n); _fmpz_vec_clear(y, n); _fmpz_vec_clear(z, n); _fmpz_vec_clear(w, n);
    return ok;
}
static void mmg_export(void) {
    mpz_t z; mpz_init(z);
    for (long i = 0; i < n * n; i++) {
        mpz_import(z, LC, -1, sizeof(mp_limb_t), 0, 0, Cl + i * LC); if (Cs[i]) mpz_neg(z, z);
        fmpz_set_mpz(fmpz_mat_entry(Cz, i / n, i % n), z);
    }
    mpz_clear(z);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: gemm METHOD N BITS REPS [OUTFILE]\n"); return 2; }
    const char *m = argv[1]; n = atol(argv[2]); bits = atoi(argv[3]); int reps = atoi(argv[4]);
    scipy_openblas_set_num_threads(1);
    flint_rand_t st; flint_randinit(st);
    fmpz_mat_init(A, n, n); fmpz_mat_init(B, n, n); fmpz_mat_init(Cz, n, n); fmpz_mat_init(Cref, n, n);
    for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) {
        fmpz_randbits(fmpz_mat_entry(A, i, j), st, bits);       /* random sign, up to BITS bits */
        fmpz_randbits(fmpz_mat_entry(B, i, j), st, bits);
    }
    if (getenv("GEMM_EDGE")) {                         /* extremes: +-(2^BITS - 1) and zeros, the largest products */
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) {
            fmpz *a = fmpz_mat_entry(A, i, j), *b = fmpz_mat_entry(B, i, j);
            fmpz_one(a); fmpz_mul_2exp(a, a, bits); fmpz_sub_ui(a, a, 1); fmpz_set(b, a);
            if ((i + j) % 3 == 1) fmpz_neg(a, a);
            if (getenv("GEMM_EDGE")[0] == '2' && (i * j) % 5 == 4) fmpz_zero(b);
        }
    }
    double mem0 = hwm_mb();                              /* memory before the method's own setup */
    if (n <= 128) fmpz_mat_mul_classical(Cref, A, B);  /* the reference at small sizes; Freivalds' check at every size */
    int fb = is(m, "blas"), fzb = is(m, "fmpzb");
#ifndef FLINTBLAS
    if (fb || fzb) { fprintf(stderr, "%s needs the BLAS-enabled build (gemm_fb)\n", m); return 2; }
#endif
    int q = is(m, "fmpq"), mmga = is(m, "mmga"), mmg = is(m, "mmg"), mm = is(m, "mm") || mmg || mmga;
    if (!q && !mm && !fb && !fzb && !is(m, "fmpz")) { fprintf(stderr, "unknown method\n"); return 2; }
    if (q) {
        fmpq_mat_init(Aq, n, n); fmpq_mat_init(Bq, n, n); fmpq_mat_init(Cq, n, n);
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) {
            fmpq_set_fmpz_frac(fmpq_mat_entry(Aq, i, j), fmpz_mat_entry(A, i, j), (fmpz_t){1000});
            fmpq_set_fmpz_frac(fmpq_mat_entry(Bq, i, j), fmpz_mat_entry(B, i, j), (fmpz_t){1000});
        }
    }
    if (mmg) mmg_setup(); else if (mm && !mmga) mm_setup();
    double *t = malloc(sizeof(double) * reps), *s[3];
    for (int k = 0; k < 3; k++) s[k] = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (q) fmpq_mat_mul(Cq, Aq, Bq); else if (mmga) { mmg_setup(); mmg_run(); mmg_export(); if (r < reps - 1) mmg_free(); } else if (mmg) mmg_run(); else if (mm) mm_run(); else if (fb) { if (!fmpz_mat_mul_blas(Cz, A, B)) { fprintf(stderr, "no BLAS\n"); return 2; } } else fmpz_mat_mul(Cz, A, B);
        double dt = now() - t0;
        if (r < 0 || r == reps - 1) {                  /* check the first and last runs */
            int ok = 1;
            if (q) {                                   /* 10^6 times each entry must be an integer: the product */
                fmpz_t num; fmpz_init(num);
                for (long i = 0; i < n && ok; i++) for (long j = 0; j < n && ok; j++) {
                    fmpq *e = fmpq_mat_entry(Cq, i, j);
                    fmpz_mul_ui(num, fmpq_numref(e), 1000000);
                    ok = fmpz_divisible(num, fmpq_denref(e));
                    if (ok) fmpz_divexact(fmpz_mat_entry(Cz, i, j), num, fmpq_denref(e));
                }
                fmpz_clear(num);
                if (getenv("GEMM_CORRUPT")) fmpz_add_ui(fmpz_mat_entry(Cz, n - 1, n / 2), fmpz_mat_entry(Cz, n - 1, n / 2), 1);
                ok = ok && (n > 128 || fmpz_mat_equal(Cz, Cref)) && freivalds(Cz, st);
            } else {
                if (mmg) mmg_export();
                if (getenv("GEMM_CORRUPT")) fmpz_add_ui(fmpz_mat_entry(Cz, n - 1, n / 2), fmpz_mat_entry(Cz, n - 1, n / 2), 1);
                ok = (n > 128 || fmpz_mat_equal(Cz, Cref)) && freivalds(Cz, st);
            }
            if (!ok) { fprintf(stderr, "mismatch\n"); return 4; }
        }
        if (r >= 0) { t[r] = dt; if (mm) for (int k = 0; k < 3; k++) s[k][r] = tsplit[k]; }
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g", t[reps / 2], t[0]);
    if (mm) { for (int k = 0; k < 3; k++) qsort(s[k], reps, sizeof(double), cmpd);
              printf(" %.9g %.9g %.9g %ld", s[0][reps / 2], s[1][reps / 2], s[2][reps / 2], np); }
    printf(" %.1f\n", hwm_mb() - mem0);
    if (argc > 5) {                                    /* the product, for an independent check in Python */
        FILE *f = fopen(argv[5], "w");
        if (mmg) mmg_export();
        fmpz_mat_t *out = q ? NULL : &Cz;
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) {
            if (q) { char *x = fmpq_get_str(NULL, 10, fmpq_mat_entry(Cq, i, j)); fprintf(f, "%s\n", x); flint_free(x); }
            else { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(*out, i, j)); fprintf(f, "%s\n", x); flint_free(x); }
        }
        fclose(f);
        char path[4096]; snprintf(path, sizeof path, "%s.in", argv[5]);
        FILE *h = fopen(path, "w");
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(A, i, j)); fprintf(h, "%s\n", x); flint_free(x); }
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(B, i, j)); fprintf(h, "%s\n", x); flint_free(x); }
        fclose(h);
    }
    return 0;
}
