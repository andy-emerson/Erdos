#define _POSIX_C_SOURCE 200809L
/* Fixed-width uN tiers for the vector form, against GMP and FLINT, on workloads that pass 128 bits.

   usage: un METHOD OP N DEN K REPS INDIR OUTFILE
     METHOD  fl      the same vector form held as fixed-width limb arrays: each element is a sign and W/64
                     64-bit limbs in one contiguous array, with arithmetic by GMP's low-level mpn routines
                     (which choose schoolbook, Karatsuba or Toom multiplication by size, and never allocate);
                     a word-sized operand uses a one-limb multiply. Same widening rule as un.
             flc     fl with the element length fixed at compile time to CAP/64 limbs and the one-limb
                     multiply, add and subtract written as constant-length loops (no library calls); chain only,
                     always at CAP bits: a pre-built U2048
             un@W, fl@W   un or fl whose chain starts at width W instead of 64, so it never widens below W
                     (un@2048, fl@2048: a ready-made 2048-bit vector, the same check for the cap)
             un      int64 numerators over the shared denominator, held in signed _BitInt(W) for W in
                     64, 128, ..., 2048; the whole vector widens to the narrowest W its actual values
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
#define WIDTHS(X) X(64) X(128) X(256) X(512) X(1024) X(2048)
#define CAP 2048            /* the widest tier; past it the vector would continue in arbitrary precision */
#define NW 6
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
UP(64, 128) UP(128, 256) UP(256, 512) UP(512, 1024) UP(1024, 2048)

static void *xa[NW];                /* the chain's vector at each width, allocated once */
static int widx(int W) { int i = 0; while ((64 << i) < W) i++; return i; }
static int final_width;
static mpz_t PW;                    /* DEN^(k+1) during a chain, for its exact bit length */
static int pw_next(void) { mpz_mul_ui(PW, PW, (unsigned long)DEN); return (int)mpz_sizeinbase(PW, 2); }
static int START = 64;              /* the chain's starting width; un@W and fl@W set it */

static void export_signed(mpz_t z, const void *p, int W) {      /* a W-bit two's complement value to GMP */
    size_t nl = W / 64; uint64_t limbs[CAP / 64]; memcpy(limbs, p, nl * 8);
    int neg = (int64_t)limbs[nl - 1] < 0;
    if (neg) { unsigned c = 1; for (size_t i = 0; i < nl; i++) { limbs[i] = ~limbs[i] + c; c = c && limbs[i] == 0; } }
    mpz_import(z, nl, -1, 8, 0, 0, limbs);
    if (neg) mpz_neg(z, z);
}

/* the chain on uN: widen before a step whenever the bound from actual bit lengths could pass W - 1 */
static int un_chain(void) {
    int W = START;
    switch (W) {
#define INIT(WW) case WW: { i##WW *x0 = xa[widx(WW)]; for (long i = 0; i < N; i++) x0[i] = A[i]; break; }
        WIDTHS(INIT)
#undef INIT
    }
    int bx = 0, bb = 0, bc = 0;
    for (long i = 0; i < N; i++) { int t = bits64(A[i]); if (t > bx) bx = t; t = bits64(B[i]); if (t > bb) bb = t; t = bits64(C[i]); if (t > bc) bc = t; }
    mpz_set_ui(PW, 1);
    for (long k = 0; k < K; k++) {
        int bcs = pw_next();                               /* bits of DEN^(k+1), exactly */
        int need = (bx + bb > bc + bcs ? bx + bb : bc + bcs) + 2;   /* a signed sum: one carry bit, one sign bit */
        while (need > W) {
            if (W == CAP) return 0;
            switch (W) { case 64: up_64(xa[0], xa[1]); break; case 128: up_128(xa[1], xa[2]); break;
                         case 256: up_256(xa[2], xa[3]); break; case 512: up_512(xa[3], xa[4]); break;
                         default: up_1024(xa[4], xa[5]); }
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
static unsigned char sumbuf[CAP / 8];
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

/* ---------------------------------------------------------------- fixed-width limb arrays (fl) */
static mp_limb_t *flx, *flt1, *flt2, *flcs;   /* magnitudes: element i occupies limbs [i*L, (i+1)*L) */
static unsigned char *fls;                     /* signs: 1 for negative */
static long FL;                                /* limbs per element */
static mp_limb_t flsum[CAP / 64 + 4];
static int fl_maxbits(void) {
    for (long j = FL - 1; j >= 0; j--) {
        mp_limb_t o = 0;
        for (long i = 0; i < N; i++) o |= flx[i * FL + j];
        if (o) return (int)(64 * j + 64 - __builtin_clzll(o));
    }
    return 0;
}
static mp_limb_t *flbuf[2]; static int flcur;   /* two buffers of N * CAP/64 limbs, allocated before timing */
static void fl_widen(long L2) {               /* the whole vector moves to L2 limbs per element, in the other buffer */
    mp_limb_t *y = flbuf[!flcur];
    for (long i = 0; i < N; i++) {
        memcpy(y + i * L2, flx + i * FL, FL * sizeof(mp_limb_t));
        memset(y + i * L2 + FL, 0, (L2 - FL) * sizeof(mp_limb_t));
    }
    flcur = !flcur; flx = y; FL = L2;
}
static uint64_t uabs64(int64_t v) { return v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; }
/* r = (sa, A) + (sb, B), magnitudes of L limbs; the caller proved the result fits L limbs */
static void fl_signed_add(mp_limb_t *r, unsigned char *sr, const mp_limb_t *A, int sa, const mp_limb_t *B, int sb, long L) {
    if (sa == sb) { mpn_add_n(r, A, B, L); *sr = (unsigned char)sa; return; }
    int c = mpn_cmp(A, B, L);
    if (c >= 0) { mpn_sub_n(r, A, B, L); *sr = c == 0 ? 0 : (unsigned char)sa; }
    else { mpn_sub_n(r, B, A, L); *sr = (unsigned char)sb; }
}
static int fl_chain(void) {
    FL = START / 64; flcur = 0; flx = flbuf[0];
    memset(flx, 0, (size_t)N * FL * sizeof(mp_limb_t));
    for (long i = 0; i < N; i++) { flx[i * FL] = uabs64(A[i]); fls[i] = A[i] < 0; }
    int bx = 0, bb = 0, bc = 0;
    for (long i = 0; i < N; i++) { int t = bits64(A[i]); if (t > bx) bx = t; t = bits64(B[i]); if (t > bb) bb = t; t = bits64(C[i]); if (t > bc) bc = t; }
    mpz_set_ui(PW, 1);
    mp_limb_t cs[CAP / 64 + 2]; long lcs = 1; cs[0] = 1;
    for (long k = 0; k < K; k++) {
        int bcs = pw_next();
        int need = (bx + bb > bc + bcs ? bx + bb : bc + bcs) + 1;   /* a magnitude: one carry bit, sign kept apart */
        long L = FL;
        while (need > 64 * L) { if (64 * L >= CAP) return 0; L *= 2; }
        mp_limb_t carry = mpn_mul_1(cs, cs, lcs, (mp_limb_t)DEN);        /* cs = DEN^(k+1) */
        if (carry) cs[lcs++] = carry;
        if (L != FL) fl_widen(L);
        memset(flcs, 0, (FL + 1) * sizeof(mp_limb_t));
        memcpy(flcs, cs, lcs * sizeof(mp_limb_t));
        for (long i = 0; i < N; i++) {
            mp_limb_t *x = flx + i * FL;
            mpn_mul_1(flt1, x, FL, uabs64(B[i]));                          /* |x| |b|: a one-limb multiply */
            mpn_mul_1(flt2, flcs, FL, uabs64(C[i]));                       /* |c| DEN^(k+1) */
            int sx = (fls[i] ^ (B[i] < 0)) & 1, sc = C[i] < 0;
            fl_signed_add(x, fls + i, flt1, sx, flt2, sc, FL);
            if (mpn_zero_p(x, FL)) fls[i] = 0;
        }
        bx = fl_maxbits();
    }
    final_width = (int)(64 * FL);
    return 1;
}
static void fl_run(const char *op) {
    if (is(op, "chain")) { if (!fl_chain()) final_width = -1; return; }
    int W = is(op, "sq") ? un_needed_sq() : un_needed_sumsq();
    if (W > CAP) { final_width = -1; return; }
    long L = W / 64;
    if (is(op, "sq")) {
        flx = flbuf[0]; FL = L;
        for (long i = 0; i < N; i++) {
            unsigned __int128 pr = (unsigned __int128)uabs64(A[i]) * uabs64(B[i]);
            mp_limb_t pl[2] = { (mp_limb_t)pr, (mp_limb_t)(pr >> 64) }, sq[4];
            mpn_sqr(sq, pl, 2);
            mp_limb_t *x = flx + i * L;
            memcpy(x, sq, (L < 4 ? L : 4) * sizeof(mp_limb_t));
            if (L > 4) memset(x + 4, 0, (L - 4) * sizeof(mp_limb_t));
        }
    } else {
        memset(flsum, 0, sizeof flsum);
        for (long i = 0; i < N; i++) {
            unsigned __int128 pr = (unsigned __int128)uabs64(A[i]) * uabs64(B[i]);
            mp_limb_t pl[2] = { (mp_limb_t)pr, (mp_limb_t)(pr >> 64) }, sq[4];
            mpn_sqr(sq, pl, 2);
            mpn_add(flsum, flsum, L, sq, L < 4 ? L : 4);
        }
        FL = L;
    }
    final_width = W;
}
static void fl_export(mpz_t z, const mp_limb_t *x, int neg) {
    mpz_import(z, FL, -1, sizeof(mp_limb_t), 0, 0, x);
    if (neg) mpz_neg(z, z);
}

/* ---------------------------------------------------------------- flc: a pre-built U2048 (compile-time length) */
#define FLCL (CAP / 64)
typedef struct { uint64_t l[FLCL]; } u2048;
static u2048 *fcx; static unsigned char *fcsg;
static inline void flc_mul1(uint64_t *r, const uint64_t *x, uint64_t m) {
    unsigned __int128 c = 0;
    for (int j = 0; j < FLCL; j++) { c += (unsigned __int128)x[j] * m; r[j] = (uint64_t)c; c >>= 64; }
}
static inline int flc_cmp(const uint64_t *a, const uint64_t *b) {
    for (int j = FLCL - 1; j >= 0; j--) if (a[j] != b[j]) return a[j] > b[j] ? 1 : -1;
    return 0;
}
static inline void flc_add(uint64_t *r, const uint64_t *a, const uint64_t *b) {
    unsigned char c = 0;
    for (int j = 0; j < FLCL; j++) { unsigned __int128 t = (unsigned __int128)a[j] + b[j] + c; r[j] = (uint64_t)t; c = (unsigned char)(t >> 64); }
}
static inline void flc_sub(uint64_t *r, const uint64_t *a, const uint64_t *b) {
    uint64_t br = 0;
    for (int j = 0; j < FLCL; j++) { uint64_t t = a[j] - b[j] - br; br = (a[j] < b[j]) || (a[j] - b[j] < br); r[j] = t; }
}
static int flc_maxbits(void) {
    for (int j = FLCL - 1; j >= 0; j--) {
        uint64_t o = 0;
        for (long i = 0; i < N; i++) o |= fcx[i].l[j];
        if (o) return 64 * j + 64 - __builtin_clzll(o);
    }
    return 0;
}
static int flc_chain(void) {
    memset(fcx, 0, (size_t)N * sizeof(u2048));
    for (long i = 0; i < N; i++) { fcx[i].l[0] = uabs64(A[i]); fcsg[i] = A[i] < 0; }
    int bx = 0, bb = 0, bc = 0;
    for (long i = 0; i < N; i++) { int t = bits64(A[i]); if (t > bx) bx = t; t = bits64(B[i]); if (t > bb) bb = t; t = bits64(C[i]); if (t > bc) bc = t; }
    mpz_set_ui(PW, 1);
    u2048 cs = {{1}}, t1, t2;
    for (long k = 0; k < K; k++) {
        int bcs = pw_next();
        int need = (bx + bb > bc + bcs ? bx + bb : bc + bcs) + 1;
        if (need > CAP) return 0;
        flc_mul1(cs.l, cs.l, (uint64_t)DEN);
        for (long i = 0; i < N; i++) {
            uint64_t *x = fcx[i].l;
            flc_mul1(t1.l, x, uabs64(B[i]));
            flc_mul1(t2.l, cs.l, uabs64(C[i]));
            int sa = (fcsg[i] ^ (B[i] < 0)) & 1, sb = C[i] < 0;
            if (sa == sb) { flc_add(x, t1.l, t2.l); fcsg[i] = (unsigned char)sa; }
            else { int c = flc_cmp(t1.l, t2.l);
                   if (c >= 0) { flc_sub(x, t1.l, t2.l); fcsg[i] = c == 0 ? 0 : (unsigned char)sa; }
                   else { flc_sub(x, t2.l, t1.l); fcsg[i] = (unsigned char)sb; } }
        }
        bx = flc_maxbits();
    }
    final_width = CAP;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 9) { fprintf(stderr, "usage: un METHOD OP N DEN K REPS INDIR OUTFILE\n"); return 2; }
    char mbuf[64]; snprintf(mbuf, sizeof mbuf, "%s", argv[1]);
    char *at = strchr(mbuf, '@'); if (at) { *at = 0; START = atoi(at + 1); }
    const char *m = mbuf, *op = argv[2];
    if (START < 64 || START > CAP || (START & (START - 1))) { fprintf(stderr, "bad start width\n"); return 2; }
    N = atol(argv[3]); DEN = atol(argv[4]); K = atol(argv[5]); int reps = atoi(argv[6]);
    A = load(argv[7], "a.bin"); B = load(argv[7], "b.bin"); C = load(argv[7], "c.bin");
    int un = is(m, "un"), flm = is(m, "fl"), flc = is(m, "flc");
    if (flc && !is(op, "chain")) { fprintf(stderr, "flc runs the chain only\n"); return 2; }
    mpz_init(PW);
    if (flm) {
        for (int b = 0; b < 2; b++) flbuf[b] = malloc((size_t)N * (CAP / 64) * sizeof(mp_limb_t));
        fls = calloc(N, 1);
        flt1 = calloc(CAP / 64 + 1, sizeof(mp_limb_t)); flt2 = calloc(CAP / 64 + 1, sizeof(mp_limb_t)); flcs = calloc(CAP / 64 + 1, sizeof(mp_limb_t));
    }
    if (flc) { fcx = malloc((size_t)N * sizeof(u2048)); fcsg = calloc(N, 1); }
    if (un) { outbuf = malloc((size_t)N * (CAP / 8)); for (int w = 0; w < NW; w++) xa[w] = malloc((size_t)N * (8 << w)); }
    else if (!flm && !flc) lib_setup(m);
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (un) un_run(op); else if (flm) fl_run(op); else if (flc) { if (!flc_chain()) final_width = -1; } else lib_run(m, op);
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %d\n", t[reps / 2], t[0], (un || flm || flc) ? final_width : 0);
    FILE *f = fopen(argv[8], "w");
    mpz_t z, d; mpz_init(z); mpz_init(d);
    /* denominators: sq over DEN^4, sumsq over DEN^4, chain over DEN^(K+1) */
    mpz_ui_pow_ui(d, DEN, is(op, "chain") ? K + 1 : 4);
    if ((un || flm || flc) && final_width < 0) fprintf(f, "exceeds\n");
    else if (flc) for (long i = 0; i < N; i++) {
        mpz_import(z, FLCL, -1, 8, 0, 0, fcx[i].l); if (fcsg[i]) mpz_neg(z, z); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
    else if (flm) {
        if (is(op, "sumsq")) { fl_export(z, flsum, 0); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
        else for (long i = 0; i < N; i++) { fl_export(z, flx + i * FL, fls ? fls[i] : 0); gmp_fprintf(f, "%Zd/%Zd\n", z, d); }
    }
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
