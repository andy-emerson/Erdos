/* Exact matrix products with wide entries: three multimodular variants compared with each other (copied from gemm.c).

   usage: p1_gemm METHOD N BITS REPS [OUTFILE]
     A and B are N x N integer matrices with random signed entries of up to BITS bits, held in the uN fixed-limb
     layout (a sign and LA 64-bit limbs per entry); the conversion from fmpz and back is untimed for every method.
     METHOD  mmg   (gemm.c's mmg, its per-residue loops vectorised with AVX-512 doubles like mmi's) residues
                   modulo primes p with N (p/2)^2 < 2^53 as balanced doubles; reduction, one product per prime and
                   the CRT are all OpenBLAS dgemms on 16-bit digits
             mmgc  mmg with bounded memory: primes in chunks of MMGC_CH, entries in blocks of MMGC_EB; the only
                   O(np N^2) buffer is the product residues y_k, kept as uint32 (p < 2^28)
             mmi   the same three phases on AVX-512 IFMA (52-bit multiply-add, low and high halves) with one blocked
                   kernel of our own, primes below 2^50 (about half as many), 52-bit digits
   Prints "median min reduce product crt nprimes peakMB": seconds (the split as medians), the number of primes and
   how far peak resident memory rose (MB) above the inputs as fmpz and the libraries.
   The first and last runs are checked against FLINT's classical product (N <= 128) and by Freivalds' random-vector
   test (every N); a mismatch exits 4. GEMM_EDGE=1 or 2 sets extreme entries; GEMM_CORRUPT adds 1 to one entry
   before the check, which must then fail. With OUTFILE the product and (OUTFILE.in) the inputs are written. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <flint/fmpz_vec.h>
#include <flint/ulong_extras.h>

/* OpenBLAS as bundled with scipy (symbols prefixed scipy_), row-major = 101, no transpose = 111, transpose = 112 */
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
static void bound(int ok, const char *what) { if (!ok) { fprintf(stderr, "bound violated: %s\n", what); exit(5); } }
static long lmin(long a, long b) { return a < b ? a : b; }

static long n; static int bits;
static fmpz_mat_t A, B, Cz, Cref;

/* ---------------------------------------------------------------- primes, shared by all methods
   from just below `start` downwards until sum log2 p >= 2 BITS + log2 N + 2: |C| < N 2^(2 BITS), and the
   balanced recovery needs M > 2 |C| */
static mp_limb_t *primes; static long np;
static double tsplit[3];
static void pick_primes(mp_limb_t p) {
    double need = 2.0 * bits + log2((double)n) + 2;
    double have = 0; long cap = 64;
    primes = malloc(sizeof(mp_limb_t) * cap); np = 0;
    while (have < need) {
        do p--; while (!n_is_prime(p));                  /* the next prime below */
        if (np == cap) { cap *= 2; primes = realloc(primes, sizeof(mp_limb_t) * cap); }
        primes[np++] = p; have += log2((double)p);
    }
}

/* ---------------------------------------------------------------- mmg (as in gemm.c) */
static fmpz_comb_t comb; static fmpz_comb_temp_t ctemp;
static double *Ad, *Bd, *Cd;          /* [prime][n*n] */
static mp_limb_t *res;
static void mm_setup(void) {
    /* largest p with n (p/2)^2 < 2^53: exact sums of n products of balanced residues in a double */
    double pmax = 2.0 * sqrt(ldexp(1.0, 53) / (double)n);
    pick_primes((mp_limb_t)pmax);
    fmpz_comb_init(comb, primes, np); fmpz_comb_temp_init(ctemp, comb);
    Ad = malloc(sizeof(double) * np * n * n); Bd = malloc(sizeof(double) * np * n * n); Cd = malloc(sizeof(double) * np * n * n);
    res = malloc(sizeof(mp_limb_t) * np);
}

/* Entries live in the uN fixed-limb layout (a sign and LA limbs each, contiguous), the vector form's own layout.
   Reduction: split every entry into 16-bit digits; residues = digits x T with T[j][k] = 2^(16 j) mod p_k, one
   dgemm for all entries and primes. Reconstruction: y_k = r_k (M/p_k)^-1 mod p_k; x = sum y_k (M/p_k) is one
   dgemm against the 16-bit digits of M/p_k, then carries, x - floor(sum y_k/p_k) M, and the balanced sign.
   Every dgemm is exact: each bound below is asserted against 2^53. */
static long LA, LC, LM, nd, ndc;
static mp_limb_t *Al, *Bl, *Cl, *Ml, *Mhalf, *xt, *mt; static unsigned char *As, *Bs, *Cs;
static double *Dg, *T, *Mkd, *Yd, *Xd, *pinv, *Sg;   /* Sg (mmg): s_i = sum_k y_k / p_k */ static mp_limb_t *inv;
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
/* shared by all methods: inputs to limbs, the output buffer, M and M/2 (as LC = LM + 1 limbs), inv_k, 1/p_k;
   returns bits(M) and leaves M in the argument */
static long limbs_setup(fmpz_t M) {
    LA = (bits + 63) / 64;
    fmpz_one(M);
    for (long k = 0; k < np; k++) fmpz_mul_ui(M, M, primes[k]);
    long mbits = fmpz_bits(M);
    LM = (mbits + 63) / 64; LC = LM + 1;
    Al = malloc(sizeof(mp_limb_t) * n * n * LA); Bl = malloc(sizeof(mp_limb_t) * n * n * LA);
    As = malloc(n * n); Bs = malloc(n * n); Cs = malloc(n * n);
    Cl = calloc((size_t)n * n * LC, sizeof(mp_limb_t));
    to_limbs(Al, As, A); to_limbs(Bl, Bs, B);
    inv = malloc(sizeof(mp_limb_t) * np); pinv = malloc(sizeof(double) * np);
    fmpz_t Mk; fmpz_init(Mk);
    for (long k = 0; k < np; k++) {
        fmpz_divexact_ui(Mk, M, primes[k]);
        inv[k] = n_invmod(fmpz_fdiv_ui(Mk, primes[k]), primes[k]);
        pinv[k] = 1.0 / (double)primes[k];
    }
    fmpz_clear(Mk);
    mpz_t z; mpz_init(z);
    Ml = calloc(LC, sizeof(mp_limb_t)); Mhalf = calloc(LC, sizeof(mp_limb_t));
    fmpz_get_mpz(z, M); mpz_export(Ml, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    mpz_fdiv_q_2exp(z, z, 1); mpz_export(Mhalf, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    mpz_clear(z);
    return mbits;
}
static void mmg_setup(void) {
    mm_setup();
    fmpz_t M, Mk; fmpz_init(M); fmpz_init(Mk);
    long mbits = limbs_setup(M);
    nd = 4 * LA; ndc = (mbits + 15) / 16 + 1;
    double lp = log2((double)primes[0]);
    if (16 + lp + log2((double)nd) >= 53 || 16 + lp + log2((double)np) >= 53) { fprintf(stderr, "bound\n"); exit(5); }
    Dg = malloc(sizeof(double) * n * n * nd); T = malloc(sizeof(double) * nd * np);
    for (long k = 0; k < np; k++) { mp_limb_t t = 1; for (long j = 0; j < nd; j++) { T[j * np + k] = (double)t; t = n_mulmod2(t, 65536, primes[k]); } }
    Mkd = calloc((size_t)np * ndc, sizeof(double));
    mpz_t z; mpz_init(z);
    for (long k = 0; k < np; k++) {
        fmpz_divexact_ui(Mk, M, primes[k]);
        fmpz_get_mpz(z, Mk);
        for (long j = 0; j < ndc; j++) { Mkd[k * ndc + j] = (double)(mpz_getlimbn(z, j / 4) >> (16 * (j % 4)) & 0xffff); }
    }
    mpz_clear(z); fmpz_clear(M); fmpz_clear(Mk);
    bound(primes[0] < (1UL << 31), "mmg: vfmodp, vmulmodp need p < 2^31");
    Yd = malloc(sizeof(double) * np * n * n); Xd = malloc(sizeof(double) * n * n * ndc); Sg = malloc(sizeof(double) * n * n);
    xt = malloc(sizeof(mp_limb_t) * (LC + 2)); mt = malloc(sizeof(mp_limb_t) * (LC + 2));
}
/* ---- per-residue code of mmg and mmgc, AVX-512 in doubles (8 lanes; no IFMA), the same care as mmi's */
static inline __mmask8 lanes(long c) { return c >= 8 ? 0xff : c <= 0 ? 0 : (__mmask8)((1u << c) - 1); }
/* v mod p for integer-valued v, |v| < 2^53, p < 2^31: q = floor(fl(v/p)) is within one of floor(v/p) (the error
   of fl(v pi) is < 2^-52 v/p + 1 ulp < 1), so v - q p is an integer in [-p, 2p); fnmadd computes it exactly (one
   rounding of an integer < 2^32); one correction each way */
static inline __m512d vfmodp(__m512d v, __m512d p, __m512d pi) {
    __m512d q = _mm512_roundscale_pd(_mm512_mul_pd(v, pi), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    __m512d r = _mm512_fnmadd_pd(q, p, v);
    r = _mm512_mask_add_pd(r, _mm512_cmp_pd_mask(r, _mm512_setzero_pd(), _CMP_LT_OQ), r, p);
    return _mm512_mask_sub_pd(r, _mm512_cmp_pd_mask(r, p, _CMP_GE_OQ), r, p);
}
/* r w mod p for integers 0 <= r, w < p < 2^31: h = fl(r w) and l = r w - h exactly (FMA two-product; r w < 2^62,
   so |l| <= 2^9); q = floor(fl(h pi)) is within one of floor(r w / p) (error < 2^9/p + 2^-51 r w/p < 1); h - q p is
   an integer of magnitude < 2^33, exact from fnmadd, and y = (h - q p) + l = r w - q p in [-p, 2p), corrected once
   each way */
static inline __m512d vmulmodp(__m512d r, __m512d w, __m512d p, __m512d pi) {
    __m512d h = _mm512_mul_pd(r, w), l = _mm512_fmsub_pd(r, w, h);
    __m512d q = _mm512_roundscale_pd(_mm512_mul_pd(h, pi), _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
    __m512d y = _mm512_add_pd(_mm512_fnmadd_pd(q, p, h), l);
    y = _mm512_mask_add_pd(y, _mm512_cmp_pd_mask(y, _mm512_setzero_pd(), _CMP_LT_OQ), y, p);
    return _mm512_mask_sub_pd(y, _mm512_cmp_pd_mask(y, p, _CMP_GE_OQ), y, p);
}
/* the 16-bit digits of an entry are its limbs read as uint16 (little-endian): nd = 4 LA digits to doubles */
static inline void digits16(double *d, const mp_limb_t *L) {
    const uint16_t *u = (const uint16_t *)L;
    for (long j = 0; j < nd; j += 8) {
        __mmask8 mk = lanes(nd - j);
        _mm512_mask_storeu_pd(d + j, mk, _mm512_cvtepi64_pd(_mm512_cvtepu16_epi64(_mm_maskz_loadu_epi16(mk, u + j))));
    }
}
/* 8 residues of v mod p, negated where the sign bytes are set, balanced to [-p/2, p/2] */
static inline __m512d vbalanced(__m512d v, const unsigned char *sg, __mmask8 mk, __m512d p, __m512d h, __m512d pi) {
    __m512d r = vfmodp(v, p, pi);
    __m512i sv = _mm512_cvtepu8_epi64(_mm_maskz_loadu_epi8((__mmask16)mk, sg));
    __mmask8 neg = _mm512_test_epi64_mask(sv, sv) & _mm512_cmp_pd_mask(r, _mm512_setzero_pd(), _CMP_NEQ_OQ);
    r = _mm512_mask_sub_pd(r, neg, p, r);
    return _mm512_mask_sub_pd(r, _mm512_cmp_pd_mask(r, h, _CMP_GT_OQ), r, p);
}
static void reduce_gemm(double *dst, const mp_limb_t *L, const unsigned char *sg) {
    long nn = n * n;
    for (long i = 0; i < nn; i++) digits16(Dg + i * nd, L + i * LA);
    /* dst (np x nn) = T^T (np x nd) . Dg^T (nd x nn) */
    scipy_cblas_dgemm(101, 112, 112, np, nn, nd, 1.0, T, np, Dg, nd, 0.0, dst, nn);
    for (long k = 0; k < np; k++) {
        __m512d p = _mm512_set1_pd((double)primes[k]), h = _mm512_set1_pd((double)(primes[k] / 2)), pi = _mm512_set1_pd(pinv[k]);
        for (long i = 0; i < nn; i += 8) {
            __mmask8 mk = lanes(nn - i);
            double *d = dst + k * nn + i;
            _mm512_mask_storeu_pd(d, mk, vbalanced(_mm512_maskz_loadu_pd(mk, d), sg + i, mk, p, h, pi));
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
    for (long i = 0; i < nn; i++) Sg[i] = 0;
    for (long k = 0; k < np; k++) {
        __m512d p = _mm512_set1_pd((double)primes[k]), pi = _mm512_set1_pd(pinv[k]), ik = _mm512_set1_pd((double)inv[k]);
        for (long i = 0; i < nn; i += 8) {
            __mmask8 mk = lanes(nn - i);
            __m512d y = vmulmodp(vfmodp(_mm512_maskz_loadu_pd(mk, Cd + k * nn + i), p, pi), ik, p, pi);
            _mm512_mask_storeu_pd(Yd + k * nn + i, mk, y);
            _mm512_mask_storeu_pd(Sg + i, mk, _mm512_add_pd(_mm512_maskz_loadu_pd(mk, Sg + i), _mm512_mul_pd(y, pi)));
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
        mp_limb_t q = (mp_limb_t)floor(Sg[i]);
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

/* x (in xt, LC + 1 limbs, x = sum_k y_k M/p_k < np M) and s ~ sum y_k / p_k (so floor(x/M) = floor(s) up to the
   rounding of s, at most one off): x mod M, then the balanced representative into entry i of the output */
static void crt_finish(long i, double s) {
    mp_limb_t q = (mp_limb_t)floor(s);
    mp_limb_t hi = mpn_mul_1(mt, Ml, LC, q); mt[LC] = hi;
    if (mpn_sub_n(xt, xt, mt, LC + 1)) mpn_add_n(xt, xt, Ml, LC + 1);       /* q one too big */
    while (mpn_cmp(xt, Ml, LC) >= 0 && !xt[LC]) mpn_sub_n(xt, xt, Ml, LC);  /* q one too small */
    mp_limb_t *c = Cl + i * LC;
    if (mpn_cmp(xt, Mhalf, LC) > 0) { mpn_sub_n(c, Ml, xt, LC); Cs[i] = 1; }
    else { memcpy(c, xt, LC * sizeof(mp_limb_t)); Cs[i] = 0; }
}

/* ---------------------------------------------------------------- mmgc: mmg in bounded memory
   The same primes, digits, tables and dgemms as mmg, reorganised so that no buffer holds np N^2 doubles:
     for each chunk of MMGC_CH primes: for each block of MMGC_EB entries of A (then B): 16-bit digits (EB x nd)
       -> one dgemm (ch x EB) -> balanced residues, int32 (|r| <= p/2 < 2^27), into Rc (ch x N^2);
     then per prime of the chunk: int32 -> double (N^2), the product dgemm, y_k = r (M/p_k)^-1 mod p_k as uint32
       into Y32 (np x N^2: the one O(np N^2) buffer, 4 bytes per residue);
     CRT per block of MMGC_EB entries: Xb (EB x ndc) += Yb_chunk^T (EB x ch) . Mkd_chunk (ch x ndc), beta = 0 for the
       first chunk and 1 after, then carries, quotient and sign as in mmg.
   Exactness: every partial sum of every dgemm is a nonnegative integer (reduction, CRT) bounded by the full sum, or
   a sum of at most N products of balanced residues (product); the bounds are mmg's: nd 2^16 p < 2^53,
   np 2^16 p < 2^53 (CRT accumulated over all chunks), N (p/2)^2 < 2^53. */
#define MMGC_CH 32
#define MMGC_EB 1024
static uint32_t *Y32; static int32_t *Rc[2]; static double *Dgb, *Rb, *Ad1, *Bd1, *Cd1, *Yb, *Xb, *Sb;
static long ebc;
static void mmgc_setup(void) {
    double pmax = 2.0 * sqrt(ldexp(1.0, 53) / (double)n);
    pick_primes((mp_limb_t)pmax);
    fmpz_t M, Mk; fmpz_init(M); fmpz_init(Mk);
    long mbits = limbs_setup(M), nn = n * n;
    nd = 4 * LA; ndc = (mbits + 15) / 16 + 1;
    double lp = log2((double)primes[0]);
    bound(16 + lp + log2((double)nd) < 53, "mmgc reduction: nd 2^16 p < 2^53");
    bound(16 + lp + log2((double)np) < 53, "mmgc CRT: np 2^16 p < 2^53");
    bound(2 * (lp - 1) + log2((double)n) < 53, "mmgc product: N (p/2)^2 < 2^53");
    bound(primes[0] < (1UL << 31), "mmgc: residues fit int32/uint32; vfmodp, vmulmodp need p < 2^31");
    T = malloc(sizeof(double) * nd * np);
    for (long k = 0; k < np; k++) { mp_limb_t t = 1; for (long j = 0; j < nd; j++) { T[j * np + k] = (double)t; t = n_mulmod2(t, 65536, primes[k]); } }
    Mkd = calloc((size_t)np * ndc, sizeof(double));
    mpz_t z; mpz_init(z);
    for (long k = 0; k < np; k++) {
        fmpz_divexact_ui(Mk, M, primes[k]); fmpz_get_mpz(z, Mk);
        for (long j = 0; j < ndc; j++) Mkd[k * ndc + j] = (double)(mpz_getlimbn(z, j / 4) >> (16 * (j % 4)) & 0xffff);
    }
    mpz_clear(z); fmpz_clear(M); fmpz_clear(Mk);
    long ch = lmin(MMGC_CH, np); ebc = lmin(MMGC_EB, nn);
    Y32 = malloc(sizeof(uint32_t) * np * nn);
    Rc[0] = malloc(sizeof(int32_t) * ch * nn); Rc[1] = malloc(sizeof(int32_t) * ch * nn);
    Dgb = malloc(sizeof(double) * ebc * nd); Rb = malloc(sizeof(double) * ch * ebc);
    Ad1 = malloc(sizeof(double) * nn); Bd1 = malloc(sizeof(double) * nn); Cd1 = malloc(sizeof(double) * nn);
    Yb = malloc(sizeof(double) * ch * ebc); Xb = malloc(sizeof(double) * ebc * ndc); Sb = malloc(sizeof(double) * ebc);
    xt = malloc(sizeof(mp_limb_t) * (LC + 2)); mt = malloc(sizeof(mp_limb_t) * (LC + 2));
}
/* balanced residues of the entries of L modulo primes c0 .. c0+ch-1 into dst (ch x N^2) */
static void mmgc_reduce(int32_t *dst, const mp_limb_t *L, const unsigned char *sg, long c0, long ch) {
    long nn = n * n;
    for (long b0 = 0; b0 < nn; b0 += ebc) {
        long m = lmin(ebc, nn - b0);
        for (long i = 0; i < m; i++) digits16(Dgb + i * nd, L + (b0 + i) * LA);
        /* Rb (ch x m) = T[:, c0:c0+ch]^T (ch x nd) . Dgb^T (nd x m) */
        scipy_cblas_dgemm(101, 112, 112, ch, m, nd, 1.0, T + c0, np, Dgb, nd, 0.0, Rb, m);
        for (long kk = 0; kk < ch; kk++) {
            long k = c0 + kk;
            __m512d p = _mm512_set1_pd((double)primes[k]), h = _mm512_set1_pd((double)(primes[k] / 2)), pi = _mm512_set1_pd(pinv[k]);
            int32_t *d = dst + kk * nn + b0;
            for (long i = 0; i < m; i += 8) {
                __mmask8 mk = lanes(m - i);
                __m512d r = vbalanced(_mm512_maskz_loadu_pd(mk, Rb + kk * m + i), sg + b0 + i, mk, p, h, pi);
                _mm256_mask_storeu_epi32(d + i, mk, _mm512_cvtpd_epi32(r));      /* exact: |r| <= p/2 < 2^30 */
            }
        }
    }
}
static void mmgc_run(void) {
    long nn = n * n;
    double tr = 0, tp = 0, tc = 0, t;
    for (long c0 = 0; c0 < np; c0 += MMGC_CH) {
        long ch = lmin(MMGC_CH, np - c0);
        t = now();
        mmgc_reduce(Rc[0], Al, As, c0, ch); mmgc_reduce(Rc[1], Bl, Bs, c0, ch);
        tr += now() - t;
        for (long kk = 0; kk < ch; kk++) {
            long k = c0 + kk;
            t = now();
            for (long i = 0; i < nn; i += 8) {
                __mmask8 mk = lanes(nn - i);
                _mm512_mask_storeu_pd(Ad1 + i, mk, _mm512_cvtepi32_pd(_mm256_maskz_loadu_epi32(mk, Rc[0] + kk * nn + i)));
                _mm512_mask_storeu_pd(Bd1 + i, mk, _mm512_cvtepi32_pd(_mm256_maskz_loadu_epi32(mk, Rc[1] + kk * nn + i)));
            }
            scipy_cblas_dgemm(101, 111, 111, n, n, n, 1.0, Ad1, n, Bd1, n, 0.0, Cd1, n);
            double t2 = now(); tp += t2 - t;
            __m512d p = _mm512_set1_pd((double)primes[k]), pi = _mm512_set1_pd(pinv[k]), ik = _mm512_set1_pd((double)inv[k]);
            for (long i = 0; i < nn; i += 8) {
                __mmask8 mk = lanes(nn - i);
                __m512d y = vmulmodp(vfmodp(_mm512_maskz_loadu_pd(mk, Cd1 + i), p, pi), ik, p, pi);
                _mm256_mask_storeu_epi32(Y32 + k * nn + i, mk, _mm512_cvtpd_epu32(y));   /* exact: 0 <= y < p < 2^31 */
            }
            tc += now() - t2;
        }
    }
    t = now();
    for (long b0 = 0; b0 < nn; b0 += ebc) {
        long m = lmin(ebc, nn - b0);
        for (long i = 0; i < m; i++) Sb[i] = 0;
        for (long c0 = 0; c0 < np; c0 += MMGC_CH) {
            long ch = lmin(MMGC_CH, np - c0);
            for (long kk = 0; kk < ch; kk++) {
                const uint32_t *y = Y32 + (c0 + kk) * nn + b0; __m512d pk = _mm512_set1_pd(pinv[c0 + kk]);
                for (long i = 0; i < m; i += 8) {                                        /* same order as mmg */
                    __mmask8 mk = lanes(m - i);
                    __m512d yv = _mm512_cvtepu32_pd(_mm256_maskz_loadu_epi32(mk, y + i));
                    _mm512_mask_storeu_pd(Yb + kk * m + i, mk, yv);
                    _mm512_mask_storeu_pd(Sb + i, mk, _mm512_add_pd(_mm512_maskz_loadu_pd(mk, Sb + i), _mm512_mul_pd(yv, pk)));
                }
            }
            /* Xb (m x ndc) (+)= Yb^T (m x ch) . Mkd[c0:c0+ch] (ch x ndc) */
            scipy_cblas_dgemm(101, 112, 111, m, ndc, ch, 1.0, Yb, m, Mkd + c0 * ndc, ndc, c0 ? 1.0 : 0.0, Xb, ndc);
        }
        for (long i = 0; i < m; i++) {
            memset(xt, 0, sizeof(mp_limb_t) * (LC + 2));
            uint64_t carry = 0;
            for (long j = 0; j < ndc; j++) {
                carry += (uint64_t)Xb[i * ndc + j];
                xt[j / 4] |= (carry & 0xffff) << (16 * (j % 4));
                carry >>= 16;
            }
            for (long j = ndc; carry; j++) { xt[j / 4] |= (carry & 0xffff) << (16 * (j % 4)); carry >>= 16; }
            crt_finish(b0 + i, Sb[i]);
        }
    }
    tc += now() - t;
    tsplit[0] = tr; tsplit[1] = tp; tsplit[2] = tc;
}

/* ---------------------------------------------------------------- mmi: AVX-512 IFMA
   One kernel: (Chi 2^52 + Clo) (+)= A B elementwise exactly, A (m x k) and B (k x n) with entries < 2^52 (any
   strides), Clo and Chi m x n uint64 row-major. vpmadd52luq/vpmadd52huq add the low/high 52 bits of the 104-bit
   product of two 52-bit lanes to a 64-bit lane: each term is < 2^52, so a register accumulator takes at most
   2^12 terms before it can overflow. The register tile accumulates IKC <= 4096 terms and is then folded into memory,
   where Clo is kept normalised (< 2^52):  t = Clo + (lo mod 2^52) < 2^53;  Chi += hi + (lo >> 52) + (t >> 52);
   Clo = t mod 2^52. After every fold Chi 2^52 + Clo is the exact partial sum, so Chi < 2^64 as long as the full
   sum is < 2^116 (asserted by every caller). BLIS-style: B packed in IKC x INC panels (INR-wide strips), A in
   IMC x IKC blocks (IMR-tall strips), a 6 x 16 microkernel with 24 accumulators (12 lo, 12 hi). */
#define M52 ((1ULL << 52) - 1)
#define IMR 6
#define INR 16
#define IKC 256
#define IMC 192
#define INC 2048
#define IEB 1024     /* entries per block in mmi's reduction and CRT */
_Static_assert(IKC <= 4096, "register accumulators take at most 2^12 terms of < 2^52");
static uint64_t *Apk, *Bpk;
static void ipack_a(long mc, long kc, const uint64_t *a, long rs, long cs, uint64_t *d) {
    for (long ir = 0; ir < mc; ir += IMR) {
        long mr = lmin(IMR, mc - ir);
        for (long p = 0; p < kc; p++) for (long i = 0; i < IMR; i++) *d++ = i < mr ? a[(ir + i) * rs + p * cs] : 0;
    }
}
static void ipack_b(long kc, long nc, const uint64_t *b, long rs, long cs, uint64_t *d) {
    for (long jr = 0; jr < nc; jr += INR) {
        long nr = lmin(INR, nc - jr);
        if (nr == INR && cs == 1) { for (long p = 0; p < kc; p++, d += INR) memcpy(d, b + p * rs + jr, INR * 8); }
        else for (long p = 0; p < kc; p++) for (long j = 0; j < INR; j++) *d++ = j < nr ? b[p * rs + (jr + j) * cs] : 0;
    }
}
static void iukern(long kc, const uint64_t *restrict ap, const uint64_t *restrict bp, uint64_t *clo, uint64_t *chi,
                   long ldc, long mr, long nr, int acc) {
    __m512i L[IMR][2], H[IMR][2];
    #pragma GCC unroll 6
    for (int i = 0; i < IMR; i++) { L[i][0] = L[i][1] = H[i][0] = H[i][1] = _mm512_setzero_si512(); }
    for (long p = 0; p < kc; p++) {
        __m512i b0 = _mm512_load_si512(bp), b1 = _mm512_load_si512(bp + 8);
        #pragma GCC unroll 6
        for (int i = 0; i < IMR; i++) {
            __m512i a = _mm512_set1_epi64(ap[i]);
            L[i][0] = _mm512_madd52lo_epu64(L[i][0], a, b0); L[i][1] = _mm512_madd52lo_epu64(L[i][1], a, b1);
            H[i][0] = _mm512_madd52hi_epu64(H[i][0], a, b0); H[i][1] = _mm512_madd52hi_epu64(H[i][1], a, b1);
        }
        ap += IMR; bp += INR;
    }
    const __m512i msk = _mm512_set1_epi64(M52);
    __mmask8 mv[2] = { lanes(nr), lanes(nr - 8) };
    #pragma GCC unroll 6
    for (int i = 0; i < IMR; i++) {
        if (i >= mr) break;
        #pragma GCC unroll 2
        for (int v = 0; v < 2; v++) {
            if (!mv[v]) continue;
            uint64_t *pl = clo + i * ldc + 8 * v, *ph = chi + i * ldc + 8 * v;
            __m512i al = L[i][v], ah = H[i][v], cl, chv;
            if (acc) {
                cl = _mm512_maskz_loadu_epi64(mv[v], pl); chv = _mm512_maskz_loadu_epi64(mv[v], ph);
                __m512i t = _mm512_add_epi64(cl, _mm512_and_si512(al, msk));
                chv = _mm512_add_epi64(_mm512_add_epi64(chv, ah), _mm512_add_epi64(_mm512_srli_epi64(al, 52), _mm512_srli_epi64(t, 52)));
                cl = _mm512_and_si512(t, msk);
            } else {
                cl = _mm512_and_si512(al, msk); chv = _mm512_add_epi64(ah, _mm512_srli_epi64(al, 52));
            }
            _mm512_mask_storeu_epi64(pl, mv[v], cl); _mm512_mask_storeu_epi64(ph, mv[v], chv);
        }
    }
}
/* acc = 0: C = A B; acc = 1: C += A B (Clo must then be < 2^52 on entry) */
static void ifma_gemm(long m, long nc_all, long k, const uint64_t *a, long rsa, long csa, const uint64_t *b, long rsb,
                      long csb, uint64_t *clo, uint64_t *chi, long ldc, int acc) {
    for (long jc = 0; jc < nc_all; jc += INC) {
        long nc = lmin(INC, nc_all - jc);
        for (long pc = 0; pc < k; pc += IKC) {
            long kc = lmin(IKC, k - pc);
            ipack_b(kc, nc, b + pc * rsb + jc * csb, rsb, csb, Bpk);
            for (long ic = 0; ic < m; ic += IMC) {
                long mc = lmin(IMC, m - ic);
                ipack_a(mc, kc, a + ic * rsa + pc * csa, rsa, csa, Apk);
                for (long jr = 0; jr < nc; jr += INR)
                    for (long ir = 0; ir < mc; ir += IMR)
                        iukern(kc, Apk + ir * kc, Bpk + jr * kc, clo + (ic + ir) * ldc + jc + jr, chi + (ic + ir) * ldc + jc + jr,
                               ldc, lmin(IMR, mc - ir), lmin(INR, nc - jr), acc || pc > 0);
            }
        }
    }
}
/* Reduction modulo p < 2^50 in 8 lanes. Shoup: for a < 2^52, w < p and ws = floor(w 2^52 / p) < 2^52,
   q = floor(a ws / 2^52) satisfies a w / p - 2 < q <= a w / p, so r = a w - q p is in [0, 2p) and, as 2p < 2^52,
   is exactly (lo52(a w) - lo52(q p)) mod 2^52. red1 maps [0, 2p) to [0, p) (min of t and t - p, unsigned). */
typedef struct { uint64_t p, c1, c1s, c2, c2s, ones, inv, invs; } ipc_t;
static ipc_t *ipc;
static inline __m512i shoup(__m512i a, __m512i w, __m512i ws, __m512i p) {
    const __m512i z = _mm512_setzero_si512();
    __m512i q = _mm512_madd52hi_epu64(z, a, ws);
    __m512i r = _mm512_sub_epi64(_mm512_madd52lo_epu64(z, a, w), _mm512_madd52lo_epu64(z, q, p));
    return _mm512_and_si512(r, _mm512_set1_epi64(M52));
}
static inline __m512i red1(__m512i t, __m512i p) { return _mm512_min_epu64(t, _mm512_sub_epi64(t, p)); }
/* (hi 2^52 + lo) mod p for lo < 2^52, hi < 2^64: hi = hh 2^52 + hl with hh < 2^12, and
   hi 2^52 + lo == hl c1 + hh c2 + lo (mod p) with c1 = 2^52 mod p, c2 = 2^104 mod p; each term by Shoup (< 2p, then
   < p), two sums each < 2p and reduced once */
static inline __m512i vmod(__m512i lo, __m512i hi, const ipc_t *c) {
    __m512i p = _mm512_set1_epi64(c->p);
    __m512i t1 = red1(shoup(_mm512_and_si512(hi, _mm512_set1_epi64(M52)), _mm512_set1_epi64(c->c1), _mm512_set1_epi64(c->c1s), p), p);
    __m512i t2 = red1(shoup(_mm512_srli_epi64(hi, 52), _mm512_set1_epi64(c->c2), _mm512_set1_epi64(c->c2s), p), p);
    __m512i t3 = red1(shoup(lo, _mm512_set1_epi64(1), _mm512_set1_epi64(c->ones), p), p);
    return red1(_mm512_add_epi64(red1(_mm512_add_epi64(t1, t2), p), t3), p);
}
static uint64_t shoup_pre(uint64_t w, uint64_t p) { return (uint64_t)(((unsigned __int128)w << 52) / p); }
static long nd52, ndc52, XTL;
static uint64_t *T52, *Mk52, *Ar, *Br, *Dblk, *Rlo, *Rhi, *Plo, *Phi, *Xlo, *Xhi; static double *Si;
static uint64_t digit52(const mp_limb_t *L, long nl, long j) {
    long off = 52 * j, w = off / 64, s = off % 64;
    if (w >= nl) return 0;
    uint64_t v = L[w] >> s;
    if (s > 12 && w + 1 < nl) v |= L[w + 1] << (64 - s);
    return v & M52;
}
static void mmi_setup(void) {
    pick_primes(1UL << 50);                               /* the largest primes below 2^50 */
    fmpz_t M, Mk; fmpz_init(M); fmpz_init(Mk);
    long mbits = limbs_setup(M), nn = n * n;
    nd52 = (bits + 51) / 52; ndc52 = (mbits + 51) / 52;   /* 52-bit digits of an entry, of M/p_k < M */
    /* bounds for the kernel's full sums (< 2^116 so that Chi < 2^64), terms < 2^52 x 2^50:
       reduction nd52 terms, product N terms (residues < 2^50: < 2^100 each), CRT np terms */
    bound(primes[np - 1] > (1UL << 49) && primes[0] < (1UL << 50), "mmi: 2^49 < p < 2^50");
    bound(nd52 < (1L << 14), "mmi reduction: nd52 2^102 < 2^116");
    bound(n < (1L << 16), "mmi product: N 2^100 < 2^116");
    bound(np < (1L << 14), "mmi CRT: np 2^102 < 2^116");
    ipc = malloc(sizeof(ipc_t) * np);
    T52 = malloc(sizeof(uint64_t) * np * nd52); Mk52 = malloc(sizeof(uint64_t) * np * ndc52);
    mpz_t z; mpz_init(z);
    for (long k = 0; k < np; k++) {
        uint64_t p = primes[k], c1 = (uint64_t)(((unsigned __int128)1 << 52) % p);
        uint64_t c2 = (uint64_t)((unsigned __int128)c1 * c1 % p);
        ipc[k] = (ipc_t){ p, c1, shoup_pre(c1, p), c2, shoup_pre(c2, p), shoup_pre(1, p), inv[k], shoup_pre(inv[k], p) };
        uint64_t t = 1; for (long j = 0; j < nd52; j++) { T52[k * nd52 + j] = t; t = (uint64_t)((unsigned __int128)t * c1 % p); }
        fmpz_divexact_ui(Mk, M, p); fmpz_get_mpz(z, Mk);
        for (long j = 0; j < ndc52; j++) Mk52[k * ndc52 + j] = digit52(mpz_limbs_read(z), mpz_size(z), j);
    }
    mpz_clear(z); fmpz_clear(M); fmpz_clear(Mk);
    long eb = lmin(IEB, nn);
    Apk = aligned_alloc(64, sizeof(uint64_t) * (IMC + IMR) * IKC); Bpk = aligned_alloc(64, sizeof(uint64_t) * (INC + INR) * IKC);
    Ar = malloc(sizeof(uint64_t) * np * nn); Br = malloc(sizeof(uint64_t) * np * nn);
    Dblk = malloc(sizeof(uint64_t) * nd52 * eb); Rlo = malloc(sizeof(uint64_t) * np * eb); Rhi = malloc(sizeof(uint64_t) * np * eb);
    Plo = malloc(sizeof(uint64_t) * nn); Phi = malloc(sizeof(uint64_t) * nn); Si = malloc(sizeof(double) * nn);
    Xlo = malloc(sizeof(uint64_t) * eb * ndc52); Xhi = malloc(sizeof(uint64_t) * eb * ndc52);
    XTL = (52 * (ndc52 + 4)) / 64 + LC + 4;
    xt = malloc(sizeof(mp_limb_t) * XTL); mt = malloc(sizeof(mp_limb_t) * (LC + 2));
}
/* residues in [0, p) of the entries of L, into dst (np x N^2), by blocks of IEB entries:
   (Rhi 2^52 + Rlo) (np x m) = T52 (np x nd52) . D (nd52 x m), D the 52-bit digits, T52[k][j] = 2^(52 j) mod p_k */
static void mmi_reduce(uint64_t *dst, const mp_limb_t *L, const unsigned char *sg) {
    long nn = n * n;
    for (long b0 = 0; b0 < nn; b0 += IEB) {
        long m = lmin(IEB, nn - b0);
        for (long i = 0; i < m; i++) for (long j = 0; j < nd52; j++) Dblk[j * m + i] = digit52(L + (b0 + i) * LA, LA, j);
        ifma_gemm(np, m, nd52, T52, nd52, 1, Dblk, m, 1, Rlo, Rhi, m, 0);
        for (long k = 0; k < np; k++) {
            const ipc_t *c = ipc + k; __m512i p = _mm512_set1_epi64(c->p);
            for (long i = 0; i < m; i += 8) {
                __mmask8 mk = lanes(m - i);
                __m512i r = vmod(_mm512_maskz_loadu_epi64(mk, Rlo + k * m + i), _mm512_maskz_loadu_epi64(mk, Rhi + k * m + i), c);
                __m512i sv = _mm512_cvtepu8_epi64(_mm_maskz_loadu_epi8((__mmask16)mk, sg + b0 + i));
                __mmask8 neg = _mm512_test_epi64_mask(sv, sv) & _mm512_test_epi64_mask(r, r);
                r = _mm512_mask_sub_epi64(r, neg, p, r);                       /* -r mod p */
                _mm512_mask_storeu_epi64(dst + k * nn + b0 + i, mk, r);
            }
        }
    }
}
static void mmi_run(void) {
    long nn = n * n;
    double t0 = now(), tp = 0, tc = 0;
    mmi_reduce(Ar, Al, As); mmi_reduce(Br, Bl, Bs);
    double t1 = now();
    for (long i = 0; i < nn; i++) Si[i] = 0;
    for (long k = 0; k < np; k++) {
        double t = now();
        ifma_gemm(n, n, n, Ar + k * nn, n, 1, Br + k * nn, n, 1, Plo, Phi, n, 0);   /* N terms < 2^100 */
        double t2 = now(); tp += t2 - t;
        /* y_k = (C mod p) inv_k mod p < p < 2^50, over A's residues (no longer needed); s += y_k / p_k */
        const ipc_t *c = ipc + k; __m512i p = _mm512_set1_epi64(c->p), iv = _mm512_set1_epi64(c->inv), ivs = _mm512_set1_epi64(c->invs);
        __m512d pi = _mm512_set1_pd(pinv[k]);
        for (long i = 0; i < nn; i += 8) {
            __mmask8 mk = lanes(nn - i);
            __m512i r = vmod(_mm512_maskz_loadu_epi64(mk, Plo + i), _mm512_maskz_loadu_epi64(mk, Phi + i), c);
            __m512i y = red1(shoup(r, iv, ivs, p), p);
            _mm512_mask_storeu_epi64(Ar + k * nn + i, mk, y);
            __m512d s = _mm512_maskz_loadu_pd(mk, Si + i);
            _mm512_mask_storeu_pd(Si + i, mk, _mm512_add_pd(s, _mm512_mul_pd(_mm512_cvtepu64_pd(y), pi)));
        }
        tc += now() - t2;
    }
    double t3 = now();
    for (long b0 = 0; b0 < nn; b0 += IEB) {
        long m = lmin(IEB, nn - b0);
        /* (Xhi 2^52 + Xlo) (m x ndc52) = Y^T (m x np) . Mk52 (np x ndc52): np terms y < 2^50 times digits < 2^52 */
        ifma_gemm(m, ndc52, np, Ar + b0, 1, nn, Mk52, ndc52, 1, Xlo, Xhi, ndc52, 0);
        for (long i = 0; i < m; i++) {
            memset(xt, 0, sizeof(mp_limb_t) * XTL);
            const uint64_t *lo = Xlo + i * ndc52, *hi = Xhi + i * ndc52;
            unsigned __int128 c = 0;                          /* < 2^66: carry < 2^14, lo < 2^52, hi < 2^64 */
            for (long j = 0; c || j <= ndc52; j++) {
                if (j < ndc52) c += lo[j];
                if (j > 0 && j <= ndc52) c += hi[j - 1];
                uint64_t d = (uint64_t)c & M52; c >>= 52;
                long off = 52 * j, w = off / 64, s = off % 64;
                xt[w] |= d << s;
                if (s > 12) xt[w + 1] |= d >> (64 - s);
            }
            crt_finish(b0 + i, Si[b0 + i]);
        }
    }
    tc += now() - t3;
    tsplit[0] = t1 - t0; tsplit[1] = tp; tsplit[2] = tc;
}

/* ---------------------------------------------------------------- checks and driver */
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
static void limbs_export(void) {
    mpz_t z; mpz_init(z);
    for (long i = 0; i < n * n; i++) {
        mpz_import(z, LC, -1, sizeof(mp_limb_t), 0, 0, Cl + i * LC); if (Cs[i]) mpz_neg(z, z);
        fmpz_set_mpz(fmpz_mat_entry(Cz, i / n, i % n), z);
    }
    mpz_clear(z);
}

int main(int argc, char **argv) {
    if (argc < 5) { fprintf(stderr, "usage: p1_gemm METHOD N BITS REPS [OUTFILE]\n"); return 2; }
    const char *m = argv[1]; n = atol(argv[2]); bits = atoi(argv[3]); int reps = atoi(argv[4]);
    int mg = is(m, "mmg"), mc = is(m, "mmgc"), mi = is(m, "mmi");
    if (!mg && !mc && !mi) { fprintf(stderr, "unknown method\n"); return 2; }
    if (n < 1 || bits < 1 || reps < 1) { fprintf(stderr, "bad arguments\n"); return 2; }
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
    if (mg) mmg_setup(); else if (mc) mmgc_setup(); else mmi_setup();
    double *t = malloc(sizeof(double) * reps), *s[3];
    for (int k = 0; k < 3; k++) s[k] = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (mg) mmg_run(); else if (mc) mmgc_run(); else mmi_run();
        double dt = now() - t0;
        if (r < 0 || r == reps - 1) {                  /* check the first and last runs */
            limbs_export();
            if (getenv("GEMM_CORRUPT")) fmpz_add_ui(fmpz_mat_entry(Cz, n - 1, n / 2), fmpz_mat_entry(Cz, n - 1, n / 2), 1);
            int ok = (n > 128 || fmpz_mat_equal(Cz, Cref)) && freivalds(Cz, st);
            if (!ok) { fprintf(stderr, "mismatch\n"); return 4; }
        }
        if (r >= 0) { t[r] = dt; for (int k = 0; k < 3; k++) s[k][r] = tsplit[k]; }
    }
    qsort(t, reps, sizeof(double), cmpd);
    for (int k = 0; k < 3; k++) qsort(s[k], reps, sizeof(double), cmpd);
    /* memory every method shares: input limbs, the limb output and the fmpz copy made for the check (an estimate) */
    double shared = (8.0 * n * n * (2.0 * LA + 2.0 * LC) + 3.0 * n * n + 16.0 * n * n) / 1048576.0;
    printf("%.9g %.9g %.9g %.9g %.9g %ld %.1f %.1f\n", t[reps / 2], t[0], s[0][reps / 2], s[1][reps / 2], s[2][reps / 2], np,
           hwm_mb() - mem0, shared);
    if (argc > 5) {                                    /* the product, for an independent check in Python */
        FILE *f = fopen(argv[5], "w");
        limbs_export();
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(Cz, i, j)); fprintf(f, "%s\n", x); flint_free(x); }
        fclose(f);
        char path[4096]; snprintf(path, sizeof path, "%s.in", argv[5]);
        FILE *h = fopen(path, "w");
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(A, i, j)); fprintf(h, "%s\n", x); flint_free(x); }
        for (long i = 0; i < n; i++) for (long j = 0; j < n; j++) { char *x = fmpz_get_str(NULL, 10, fmpz_mat_entry(B, i, j)); fprintf(h, "%s\n", x); flint_free(x); }
        fclose(h);
    }
    return 0;
}
