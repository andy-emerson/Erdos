#define _POSIX_C_SOURCE 200809L
/* Phase one, arrays: five representations of the vector form compared with each other at fixed widths.

   usage: p1_arith REP OP W N REPS INDIR OUTFILE [K DEN]
     REP   bi    signed _BitInt(W), the compiler's fixed-width integers (widths 64 .. 2048 compiled in)
           lc    sign + W/64 limbs with the length fixed at compile time, hand-written loops
           lr    sign + W/64 limbs with the length set at run time, GMP's mpn routines
           rns   residues: modulus 2^64 (plain machine arithmetic) plus primes below 2^62 (Montgomery form);
                 as many primes as the width needs, so a 64-bit vector is exactly an int64 vector
           mpz   GMP mpz_t per element (arbitrary precision)
   Each representation is written the natural way for it, which the results reflect:
     bi multiplies at the full width W even when one factor is a single word (the chain, and exact division by
     the inverse modulo 2^W): C has no narrower multiply for _BitInt; lc and lr multiply word by word.
     lc and lr scan bit lengths limb by limb from the top and stop at the first nonzero limb (vectors that fill
     their width end after one pass); bi reads every value in full; residues must reconstruct every value.
     Residues pay base extension (mixed radix, quadratic in the number of moduli) to grow; the widen operation
     measures it, and the harness also reports products and dot products with that cost included.
     OP    add   a + b               mul   ha * hb            dot   sum of ha * hb
           div1  e1 / 999 (exact)    div2  e2 / 1000 (exact)  sort  order a (stable merge sort of indices)
           scan  largest bit length of |a| (what narrowing needs)
           widen ha moved from width W/2 to W (zero or sign extension; base extension for residues)
           chain x <- x * b + c * DEN^(k+1), K steps from int64 values, widening as the proven bound requires;
                 every representation uses the same bound rule (no scans), so they widen at the same steps
   Every vector fills its width, as the vector form's narrowest-width rule makes it: a, b have W - 12 bits and
   e1 = 999 a, e2 = 1000 a up to W - 2 bits, all stored at width W; ha, hb have (W - 20)/2 - 1 bits, stored at
   width WH = max(64, W/2), and their products and dot product are produced at width W. Residues are the
   exception: ha and hb are stored with the moduli of width W already (residues cannot widen without base
   extension, whose cost the widen operation measures separately). INDIR holds, per array X in a, b, e1, e2
   (W/64 limbs) and ha, hb (WH/64 limbs): X.mag (magnitudes) and X.sgn (N bytes); for chain, x0.bin cb.bin cc.bin (int64). Conversion into each representation is done before
   timing and conversion out after; everything an operation needs at run time (inverses, keys, base extension)
   is inside the timed region. Prints "median min width"; writes results as decimal integers to OUTFILE:
   one per element, one for dot and scan, the permutation for sort. Exit 3: the chain passed the 2048-bit cap. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>

typedef unsigned __int128 u128;
static long N, K, DEN; static int W, L, LH, OP;
enum { ADD, MUL, DOT, DIV1, DIV2, SORT, SCAN, WIDEN, CHAIN };
static uint64_t *MAG[6]; static unsigned char *SG[6];        /* a, b, e1, e2 (width W); ha, hb (width W/2) */
static int64_t *X0, *CB, *CC;
static int final_width;
#define CAP 2048

static void *loadf(const char *dir, const char *name, size_t bytes) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(2); }
    void *x = malloc(bytes); if (fread(x, 1, bytes, f) != bytes) { fprintf(stderr, "short read %s\n", name); exit(2); }
    fclose(f); return x;
}
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static int bits64(int64_t v) { uint64_t u = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; return u ? 64 - __builtin_clzll(u) : 0; }
static uint64_t uabs64(int64_t v) { return v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; }
static uint64_t binv64(uint64_t d) { uint64_t x = d; for (int i = 0; i < 6; i++) x *= 2 - d * x; return x; }  /* d odd */
static uint64_t umulh(uint64_t a, uint64_t b) { return (uint64_t)(((u128)a * b) >> 64); }

/* the chain's proven bound: bits of x after a step, from bits of x, b, c and DEN^(k+1) (exact, tracked in GMP) */
static int BB, BC; static mpz_t PW;
static void chain_bounds(void) {
    BB = 0; BC = 0;
    for (long i = 0; i < N; i++) { int t = bits64(CB[i]); if (t > BB) BB = t; t = bits64(CC[i]); if (t > BC) BC = t; }
    mpz_set_ui(PW, 1);
}
static int chain_next(int bx) {           /* magnitude bits after one more step; the caller adds a sign bit */
    mpz_mul_ui(PW, PW, (unsigned long)DEN);
    int bcs = (int)mpz_sizeinbase(PW, 2);
    return (bx + BB > BC + bcs ? bx + BB : BC + bcs) + 1;
}
static int bx0(void) { int b = 0; for (long i = 0; i < N; i++) { int t = bits64(X0[i]); if (t > b) b = t; } return b; }

/* stable bottom-up merge sort of the permutation, with the comparison inlined per representation */
static int *perm, *ptmp;
#define MSORT(NAME, LESS) static void NAME(void) { \
    for (long i = 0; i < N; i++) perm[i] = (int)i; \
    int *src = perm, *dst = ptmp; \
    for (long w = 1; w < N; w *= 2) { \
        for (long lo = 0; lo < N; lo += 2 * w) { \
            long mid = lo + w < N ? lo + w : N, hi = lo + 2 * w < N ? lo + 2 * w : N, i = lo, j = mid, k = lo; \
            while (i < mid && j < hi) { int x = src[i], y = src[j]; if (LESS(y, x)) dst[k++] = src[j++]; else dst[k++] = src[i++]; } \
            while (i < mid) dst[k++] = src[i++]; \
            while (j < hi) dst[k++] = src[j++]; } \
        int *t = src; src = dst; dst = t; } \
    if (src != perm) memcpy(perm, src, N * sizeof(int)); }

/* outputs common to all representations: a vector of signed integers (exported as mpz), or one integer */
static mpz_t *ZOUT; static mpz_t ZONE; static int out_kind;   /* 0 vector, 1 one value, 2 permutation */

/* ================================================================ bi: _BitInt(W) */
#define WIDTHS(X) X(64) X(128) X(256) X(512) X(1024) X(2048)
#define BIT(WW) typedef signed _BitInt(WW) bi##WW; typedef unsigned _BitInt(WW) bu##WW;
WIDTHS(BIT)
#define BI(WW, WH) \
static bi##WW *bia##WW, *bib##WW, *bie1##WW, *bie2##WW, *bio##WW; static bi##WH *biha##WW, *bihb##WW; \
static bi##WW bisum##WW; static int bimax##WW; \
static bi##WW *bi_conv##WW(int which) { \
    bi##WW *x = malloc(sizeof(bi##WW) * N); \
    for (long i = 0; i < N; i++) { bu##WW v = 0; memcpy(&v, MAG[which] + i * (WW / 64), WW / 8); \
        x[i] = SG[which][i] ? -(bi##WW)v : (bi##WW)v; } \
    return x; } \
static bu##WW bi_inv##WW(bu##WW d) { bu##WW x = d; for (int i = 0; i < 12; i++) x *= 2 - d * x; return x; } \
static int bi_bits##WW(bu##WW acc) { int b = 0; while (acc) { acc >>= 1; b++; } return b; } \
static void bi_setup##WW(void) { bia##WW = bi_conv##WW(0); bib##WW = bi_conv##WW(1); bie1##WW = bi_conv##WW(2); \
    bie2##WW = bi_conv##WW(3); biha##WW = bi_conv##WH(4); bihb##WW = bi_conv##WH(5); bio##WW = malloc(sizeof(bi##WW) * N); } \
static void bi_sortk##WW(void); \
static void bi_run##WW(void) { \
    bi##WW *a = bia##WW, *b = bib##WW, *o = bio##WW; bi##WH *ha = biha##WW, *hb = bihb##WW; \
    switch (OP) { \
    case ADD: for (long i = 0; i < N; i++) o[i] = a[i] + b[i]; break; \
    case MUL: for (long i = 0; i < N; i++) o[i] = (bi##WW)ha[i] * (bi##WW)hb[i]; break; \
    case DOT: { bi##WW s = 0; for (long i = 0; i < N; i++) s += (bi##WW)ha[i] * (bi##WW)hb[i]; bisum##WW = s; break; } \
    case WIDEN: for (long i = 0; i < N; i++) o[i] = (bi##WW)ha[i]; break; \
    case DIV1: { bu##WW inv = bi_inv##WW(999); bi##WW *e = bie1##WW; \
        for (long i = 0; i < N; i++) o[i] = (bi##WW)((bu##WW)e[i] * inv); break; } \
    case DIV2: { bu##WW inv = bi_inv##WW(125); bi##WW *e = bie2##WW; \
        for (long i = 0; i < N; i++) o[i] = (bi##WW)((bu##WW)(e[i] >> 3) * inv); break; } \
    case SORT: bi_sortk##WW(); break; \
    case SCAN: { bu##WW acc = 0; for (long i = 0; i < N; i++) { bi##WW v = a[i]; acc |= (bu##WW)(v < 0 ? -v : v); } \
        bimax##WW = bi_bits##WW(acc); break; } \
    } } \
static void bi_out##WW(void) { \
    if (OP == DOT) { bu##WW v = (bu##WW)bisum##WW; int neg = bisum##WW < 0; if (neg) v = -v; \
        mpz_import(ZONE, WW / 64, -1, 8, 0, 0, &v); if (neg) mpz_neg(ZONE, ZONE); out_kind = 1; return; } \
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)bimax##WW); out_kind = 1; return; } \
    if (OP == SORT) { out_kind = 2; return; } \
    for (long i = 0; i < N; i++) { bi##WW s = bio##WW[i]; bu##WW v = (bu##WW)(s < 0 ? -s : s); \
        mpz_import(ZOUT[i], WW / 64, -1, 8, 0, 0, &v); if (s < 0) mpz_neg(ZOUT[i], ZOUT[i]); } \
    out_kind = 0; }
/* the conversion of a WH-wide input needs bi_conv at width WH, defined by the narrower instance first */
BI(64, 64) BI(128, 64) BI(256, 128) BI(512, 256) BI(1024, 512) BI(2048, 1024)
#define BILESS64(x, y) (bia64[x] < bia64[y])
#define BILESS128(x, y) (bia128[x] < bia128[y])
#define BILESS256(x, y) (bia256[x] < bia256[y])
#define BILESS512(x, y) (bia512[x] < bia512[y])
#define BILESS1024(x, y) (bia1024[x] < bia1024[y])
#define BILESS2048(x, y) (bia2048[x] < bia2048[y])
MSORT(bi_sortk64, BILESS64) MSORT(bi_sortk128, BILESS128) MSORT(bi_sortk256, BILESS256)
MSORT(bi_sortk512, BILESS512) MSORT(bi_sortk1024, BILESS1024) MSORT(bi_sortk2048, BILESS2048)

/* bi chain: the whole vector moves to twice the width when the bound requires */
static void *bix[6];
static int widx(int w) { int i = 0; while ((64 << i) < w) i++; return i; }
#define BIUP(WW, W2) static void biup##WW(void) { bi##WW *x = bix[widx(WW)]; bi##W2 *y = bix[widx(W2)]; \
    for (long i = 0; i < N; i++) y[i] = x[i]; }
BIUP(64, 128) BIUP(128, 256) BIUP(256, 512) BIUP(512, 1024) BIUP(1024, 2048)
static int bi_chain(void) {
    chain_bounds(); int w = 64, bx = bx0();
    { bi64 *x = bix[0]; for (long i = 0; i < N; i++) x[i] = X0[i]; }
    mpz_t cs; mpz_init_set_ui(cs, 1);
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx);
        while (bx + 1 > w) {
            if (w == CAP) { mpz_clear(cs); return 0; }
            switch (w) { case 64: biup64(); break; case 128: biup128(); break; case 256: biup256(); break;
                         case 512: biup512(); break; default: biup1024(); }
            w *= 2;
        }
        mpz_mul_ui(cs, cs, (unsigned long)DEN);
        switch (w) {
#define BISTEP(WW) case WW: { bu##WW c = 0; mpz_export(&c, NULL, -1, 8, 0, 0, cs); bi##WW csv = (bi##WW)c, *x = bix[widx(WW)]; \
            for (long i = 0; i < N; i++) x[i] = x[i] * CB[i] + (bi##WW)CC[i] * csv; break; }
            WIDTHS(BISTEP)
        }
    }
    mpz_clear(cs);
    final_width = w; return 1;
}
static void bi_chain_out(void) {
    switch (final_width) {
#define BICOUT(WW) case WW: { bi##WW *x = bix[widx(WW)]; for (long i = 0; i < N; i++) { bi##WW s = x[i]; \
        bu##WW v = (bu##WW)(s < 0 ? -s : s); mpz_import(ZOUT[i], WW / 64, -1, 8, 0, 0, &v); if (s < 0) mpz_neg(ZOUT[i], ZOUT[i]); } break; }
        WIDTHS(BICOUT)
    }
    out_kind = 0;
}

/* ================================================================ lc: compile-time limbs */
#define LCS(X) X(1) X(2) X(4) X(8) X(16) X(32)
#define LCT(LL) typedef struct { uint64_t l[LL]; } lc##LL;
LCS(LCT)
#define LC(LL, LHH) \
static lc##LHH *lcha##LL, *lchb##LL; \
static lc##LL *lca##LL, *lcb##LL, *lce1##LL, *lce2##LL, *lco##LL; static unsigned char *lcsa##LL, *lcsb##LL, *lcs1##LL, *lcs2##LL, *lcso##LL; \
static lc##LL lcsum##LL; static unsigned char lcsums##LL; static int lcmax##LL; \
static inline int lc_cmp##LL(const uint64_t *a, const uint64_t *b) { \
    for (int j = LL - 1; j >= 0; j--) if (a[j] != b[j]) return a[j] > b[j] ? 1 : -1; return 0; } \
static inline void lc_addm##LL(uint64_t *r, const uint64_t *a, const uint64_t *b) { \
    unsigned char c = 0; for (int j = 0; j < LL; j++) { u128 t = (u128)a[j] + b[j] + c; r[j] = (uint64_t)t; c = (unsigned char)(t >> 64); } } \
static inline void lc_subm##LL(uint64_t *r, const uint64_t *a, const uint64_t *b) { \
    uint64_t br = 0; for (int j = 0; j < LL; j++) { uint64_t aj = a[j], bj = b[j], t = aj - bj - br; br = (aj < bj) | ((aj - bj) < br); r[j] = t; } } \
static inline void lc_sadd##LL(uint64_t *r, unsigned char *sr, const uint64_t *a, int sa, const uint64_t *b, int sb) { \
    if (sa == sb) { lc_addm##LL(r, a, b); *sr = (unsigned char)sa; return; } \
    int c = lc_cmp##LL(a, b); \
    if (c >= 0) { lc_subm##LL(r, a, b); *sr = c == 0 ? 0 : (unsigned char)sa; } else { lc_subm##LL(r, b, a); *sr = (unsigned char)sb; } } \
enum { LH##LL = LHH }; \
static inline void lc_mulm##LL(uint64_t *r, const uint64_t *a, const uint64_t *b) {  /* operands use LH limbs */ \
    uint64_t t[2 * LH##LL]; for (int j = 0; j < 2 * LH##LL; j++) t[j] = 0; \
    for (int i = 0; i < LH##LL; i++) { uint64_t c = 0; \
        for (int j = 0; j < LH##LL; j++) { u128 p = (u128)a[i] * b[j] + t[i + j] + c; t[i + j] = (uint64_t)p; c = (uint64_t)(p >> 64); } \
        t[i + LH##LL] = c; } \
    for (int j = 0; j < LL; j++) r[j] = j < 2 * LH##LL ? t[j] : 0; } \
static inline void lc_divx##LL(uint64_t *q, const uint64_t *x, uint64_t d, uint64_t inv) {  /* exact, d odd */ \
    uint64_t c = 0; for (int j = 0; j < LL; j++) { uint64_t s = x[j], b1 = s < c; s -= c; uint64_t qj = s * inv; q[j] = qj; c = umulh(qj, d) + b1; } } \
static inline void lc_shr##LL(uint64_t *r, const uint64_t *x, int a) { \
    for (int j = 0; j < LL; j++) r[j] = (x[j] >> a) | (j + 1 < LL ? x[j + 1] << (64 - a) : 0); } \
static void *lc_conv##LL(int which, unsigned char **sg, size_t sz) { \
    void *x = malloc(sz * N); memcpy(x, MAG[which], sz * N); \
    *sg = malloc(N); memcpy(*sg, SG[which], N); return x; } \
static unsigned char *lcsha##LL, *lcshb##LL; \
static void lc_setup##LL(void) { lca##LL = lc_conv##LL(0, &lcsa##LL, sizeof(lc##LL)); lcb##LL = lc_conv##LL(1, &lcsb##LL, sizeof(lc##LL)); \
    lce1##LL = lc_conv##LL(2, &lcs1##LL, sizeof(lc##LL)); lce2##LL = lc_conv##LL(3, &lcs2##LL, sizeof(lc##LL)); \
    lcha##LL = lc_conv##LL(4, &lcsha##LL, sizeof(lc##LHH)); lchb##LL = lc_conv##LL(5, &lcshb##LL, sizeof(lc##LHH)); \
    lco##LL = malloc(sizeof(lc##LL) * N); lcso##LL = malloc(N); } \
static inline int lc_less##LL(int x, int y) { int sx = lcsa##LL[x], sy = lcsa##LL[y]; \
    if (sx != sy) return sx > sy; int c = lc_cmp##LL(lca##LL[x].l, lca##LL[y].l); return sx ? c > 0 : c < 0; } \
MSORT(lc_sortk##LL, lc_less##LL) \
static void lc_run##LL(void) { \
    lc##LL *a = lca##LL, *b = lcb##LL, *o = lco##LL; unsigned char *sa = lcsa##LL, *sb = lcsb##LL, *so = lcso##LL; \
    switch (OP) { \
    case ADD: for (long i = 0; i < N; i++) lc_sadd##LL(o[i].l, so + i, a[i].l, sa[i], b[i].l, sb[i]); break; \
    case MUL: for (long i = 0; i < N; i++) { lc_mulm##LL(o[i].l, lcha##LL[i].l, lchb##LL[i].l); so[i] = lcsha##LL[i] ^ lcshb##LL[i]; } break; \
    case DOT: { lc##LL s, t; unsigned char ss = 0; memset(&s, 0, sizeof s); \
        for (long i = 0; i < N; i++) { lc_mulm##LL(t.l, lcha##LL[i].l, lchb##LL[i].l); lc_sadd##LL(s.l, &ss, s.l, ss, t.l, lcsha##LL[i] ^ lcshb##LL[i]); } \
        lcsum##LL = s; lcsums##LL = ss; break; } \
    case WIDEN: for (long i = 0; i < N; i++) { for (int j = 0; j < LL; j++) o[i].l[j] = j < LHH ? lcha##LL[i].l[j] : 0; so[i] = lcsha##LL[i]; } break; \
    case DIV1: { uint64_t inv = binv64(999); for (long i = 0; i < N; i++) { lc_divx##LL(o[i].l, lce1##LL[i].l, 999, inv); so[i] = lcs1##LL[i]; } break; } \
    case DIV2: { uint64_t inv = binv64(125); lc##LL t; \
        for (long i = 0; i < N; i++) { lc_shr##LL(t.l, lce2##LL[i].l, 3); lc_divx##LL(o[i].l, t.l, 125, inv); so[i] = lcs2##LL[i]; } break; } \
    case SORT: lc_sortk##LL(); break; \
    case SCAN: { int m = 0; for (int j = LL - 1; j >= 0 && !m; j--) { uint64_t acc = 0; \
            for (long i = 0; i < N; i++) acc |= a[i].l[j]; if (acc) m = 64 * j + 64 - __builtin_clzll(acc); } \
        lcmax##LL = m; break; } \
    } } \
static void lc_out##LL(void) { \
    if (OP == DOT) { mpz_import(ZONE, LL, -1, 8, 0, 0, lcsum##LL.l); if (lcsums##LL) mpz_neg(ZONE, ZONE); out_kind = 1; return; } \
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)lcmax##LL); out_kind = 1; return; } \
    if (OP == SORT) { out_kind = 2; return; } \
    for (long i = 0; i < N; i++) { mpz_import(ZOUT[i], LL, -1, 8, 0, 0, lco##LL[i].l); if (lcso##LL[i]) mpz_neg(ZOUT[i], ZOUT[i]); } \
    out_kind = 0; } \
/* chain step at this length: x <- x b + c cs, magnitudes with signs */ \
static void lc_step##LL(void *xv, unsigned char *sx, const uint64_t *cs) { \
    lc##LL *x = xv, t1, t2; \
    for (long i = 0; i < N; i++) { \
        uint64_t bm = uabs64(CB[i]), cm = uabs64(CC[i]), c1 = 0, c2 = 0; \
        for (int j = 0; j < LL; j++) { u128 p = (u128)x[i].l[j] * bm + c1; t1.l[j] = (uint64_t)p; c1 = (uint64_t)(p >> 64); \
                                       u128 q = (u128)cs[j] * cm + c2; t2.l[j] = (uint64_t)q; c2 = (uint64_t)(q >> 64); } \
        lc_sadd##LL(x[i].l, sx + i, t1.l, (sx[i] ^ (CB[i] < 0)) & 1, t2.l, CC[i] < 0); \
        if (!lc_cmp##LL(x[i].l, (uint64_t[LL]){0})) sx[i] = 0; } }
LC(1, 1) LC(2, 1) LC(4, 2) LC(8, 4) LC(16, 8) LC(32, 16)

static void *lcx[6]; static unsigned char *lcxs;
static int lc_chain(void) {
    chain_bounds(); int w = 64, bx = bx0();
    { lc1 *x = lcx[0]; for (long i = 0; i < N; i++) { x[i].l[0] = uabs64(X0[i]); lcxs[i] = X0[i] < 0; } }
    uint64_t cs[CAP / 64 + 1]; memset(cs, 0, sizeof cs); cs[0] = 1; long lcs = 1;
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx);
        while (bx + 1 > w) {                               /* the same requirement as every representation */
            if (w == CAP) return 0;
            int l1 = w / 64, l2 = 2 * l1; uint64_t *x = lcx[widx(w)], *y = lcx[widx(2 * w)];
            for (long i = 0; i < N; i++) { memcpy(y + i * l2, x + i * l1, 8 * l1); memset(y + i * l2 + l1, 0, 8 * l1); }
            w *= 2;
        }
        uint64_t c = mpn_mul_1(cs, cs, lcs, (uint64_t)DEN); if (c) cs[lcs++] = c;
        switch (w / 64) {
#define LCSTEP(LL) case LL: lc_step##LL(lcx[widx(64 * LL)], lcxs, cs); break;
            LCS(LCSTEP)
        }
    }
    final_width = w; return 1;
}
static void lc_chain_out(void) {
    int l = final_width / 64; uint64_t *x = lcx[widx(final_width)];
    for (long i = 0; i < N; i++) { mpz_import(ZOUT[i], l, -1, 8, 0, 0, x + i * l); if (lcxs[i]) mpz_neg(ZOUT[i], ZOUT[i]); }
    out_kind = 0;
}

/* ================================================================ lr: run-time limbs, GMP mpn */
static uint64_t *lra, *lrb, *lre1, *lre2, *lrha, *lrhb, *lro, *lrsum, *lrt; static unsigned char *lrsa, *lrsb, *lrs1, *lrs2, *lrsha, *lrshb, *lrso, lrsums; static int lrmax;
static void lr_sadd(uint64_t *r, unsigned char *sr, const uint64_t *a, int sa, const uint64_t *b, int sb, long l) {
    if (sa == sb) { mpn_add_n(r, a, b, l); *sr = (unsigned char)sa; return; }
    int c = mpn_cmp(a, b, l);
    if (c >= 0) { mpn_sub_n(r, a, b, l); *sr = c == 0 ? 0 : (unsigned char)sa; } else { mpn_sub_n(r, b, a, l); *sr = (unsigned char)sb; }
}
static void lr_divx(uint64_t *q, const uint64_t *x, long l, uint64_t d, uint64_t inv) {   /* exact, d odd */
    uint64_t c = 0; for (long j = 0; j < l; j++) { uint64_t s = x[j], b1 = s < c; s -= c; uint64_t qj = s * inv; q[j] = qj; c = umulh(qj, d) + b1; }
}
static int lr_less(int x, int y) { int sx = lrsa[x], sy = lrsa[y]; if (sx != sy) return sx > sy;
    int c = mpn_cmp(lra + (long)x * L, lra + (long)y * L, L); return sx ? c > 0 : c < 0; }
MSORT(lr_sortk, lr_less)
static void lr_setup(void) {
    size_t bytes = sizeof(uint64_t) * N * L;
    lra = malloc(bytes); memcpy(lra, MAG[0], bytes); lrb = malloc(bytes); memcpy(lrb, MAG[1], bytes);
    lre1 = malloc(bytes); memcpy(lre1, MAG[2], bytes); lre2 = malloc(bytes); memcpy(lre2, MAG[3], bytes);
    lrsa = SG[0]; lrsb = SG[1]; lrs1 = SG[2]; lrs2 = SG[3]; lrsha = SG[4]; lrshb = SG[5];
    size_t hbytes = sizeof(uint64_t) * N * LH; lrha = malloc(hbytes); memcpy(lrha, MAG[4], hbytes); lrhb = malloc(hbytes); memcpy(lrhb, MAG[5], hbytes);
    lro = malloc(bytes); lrso = malloc(N); lrsum = malloc(8 * (L + 1)); lrt = malloc(8 * (2 * L + 2));
}
static void lr_run(void) {
    long h = LH;
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) lr_sadd(lro + i * L, lrso + i, lra + i * L, lrsa[i], lrb + i * L, lrsb[i], L); break;
    case MUL: for (long i = 0; i < N; i++) { mpn_mul_n(lrt, lrha + i * h, lrhb + i * h, h); memcpy(lro + i * L, lrt, 8 * L); lrso[i] = lrsha[i] ^ lrshb[i]; } break;
    case DOT: { memset(lrsum, 0, 8 * L); lrsums = 0;
        for (long i = 0; i < N; i++) { mpn_mul_n(lrt, lrha + i * h, lrhb + i * h, h); lr_sadd(lrsum, &lrsums, lrsum, lrsums, lrt, lrsha[i] ^ lrshb[i], L); }
        break; }
    case WIDEN: for (long i = 0; i < N; i++) { memcpy(lro + i * L, lrha + i * h, 8 * h); if (L > h) memset(lro + i * L + h, 0, 8 * (L - h)); lrso[i] = lrsha[i]; } break;
    case DIV1: { uint64_t inv = binv64(999); for (long i = 0; i < N; i++) { lr_divx(lro + i * L, lre1 + i * L, L, 999, inv); lrso[i] = lrs1[i]; } break; }
    case DIV2: { uint64_t inv = binv64(125);
        for (long i = 0; i < N; i++) { mpn_rshift(lrt, lre2 + i * L, L, 3); lr_divx(lro + i * L, lrt, L, 125, inv); lrso[i] = lrs2[i]; } break; }
    case SORT: lr_sortk(); break;
    case SCAN: { int m = 0; for (long j = L - 1; j >= 0 && !m; j--) { uint64_t acc = 0;
            for (long i = 0; i < N; i++) acc |= lra[i * L + j]; if (acc) m = (int)(64 * j + 64 - __builtin_clzll(acc)); }
        lrmax = m; break; }
    }
}
static void lr_out(void) {
    if (OP == DOT) { mpz_import(ZONE, L, -1, 8, 0, 0, lrsum); if (lrsums) mpz_neg(ZONE, ZONE); out_kind = 1; return; }
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)lrmax); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    for (long i = 0; i < N; i++) { mpz_import(ZOUT[i], L, -1, 8, 0, 0, lro + i * L); if (lrso[i]) mpz_neg(ZOUT[i], ZOUT[i]); }
    out_kind = 0;
}
static uint64_t *lrbuf[2], *lrt1, *lrt2, *lrcs; static unsigned char *lrxs; static long LX; static uint64_t *lrx;
static int lr_chain(void) {
    chain_bounds(); int w = 64, bx = bx0(), cur = 0; LX = 1; lrx = lrbuf[0];
    for (long i = 0; i < N; i++) { lrx[i] = uabs64(X0[i]); lrxs[i] = X0[i] < 0; }
    uint64_t cs[CAP / 64 + 1]; memset(cs, 0, sizeof cs); cs[0] = 1; long lcs = 1;
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx);
        while (bx + 1 > w) {
            if (w == CAP) return 0;
            long l2 = 2 * LX; uint64_t *y = lrbuf[!cur];
            for (long i = 0; i < N; i++) { memcpy(y + i * l2, lrx + i * LX, 8 * LX); memset(y + i * l2 + LX, 0, 8 * LX); }
            cur = !cur; lrx = y; LX = l2; w *= 2;
        }
        uint64_t c = mpn_mul_1(cs, cs, lcs, (uint64_t)DEN); if (c) cs[lcs++] = c;
        memset(lrcs, 0, 8 * (LX + 1)); memcpy(lrcs, cs, 8 * lcs);
        for (long i = 0; i < N; i++) {
            uint64_t *x = lrx + i * LX;
            mpn_mul_1(lrt1, x, LX, uabs64(CB[i])); mpn_mul_1(lrt2, lrcs, LX, uabs64(CC[i]));
            lr_sadd(x, lrxs + i, lrt1, (lrxs[i] ^ (CB[i] < 0)) & 1, lrt2, CC[i] < 0, LX);
            if (mpn_zero_p(x, LX)) lrxs[i] = 0;
        }
    }
    final_width = w; return 1;
}
static void lr_chain_out(void) {
    for (long i = 0; i < N; i++) { mpz_import(ZOUT[i], LX, -1, 8, 0, 0, lrx + i * LX); if (lrxs[i]) mpz_neg(ZOUT[i], ZOUT[i]); }
    out_kind = 0;
}

/* ================================================================ rns: 2^64 plus primes below 2^62 */
#define MAXM 40
static int KM;                                  /* moduli in use: 1 (2^64) + primes */
static uint64_t P[MAXM], PNI[MAXM], R2[MAXM], ONE[MAXM];  /* P[0] unused (2^64) */
static uint64_t CJI[MAXM][MAXM];                /* (p_j^-1 mod p_i) R mod p_i, for mixed-radix conversion */
static uint64_t **RA, **RB, **RE1, **RE2, **RHA, **RHB, **RHW, **RO, RSUM[MAXM]; static int KH;
static inline uint64_t mont(uint64_t a, uint64_t b, int m) {      /* a b R^-1 mod p_m, a, b < p_m < 2^62 */
    u128 t = (u128)a * b; uint64_t q = (uint64_t)t * PNI[m];
    uint64_t u = (uint64_t)((t + (u128)q * P[m]) >> 64);
    return u >= P[m] ? u - P[m] : u;
}
static uint64_t powmod(uint64_t a, uint64_t e, uint64_t p) { u128 r = 1, b = a % p; while (e) { if (e & 1) r = r * b % p; b = b * b % p; e >>= 1; } return (uint64_t)r; }
static int isprime(uint64_t n) {        /* deterministic Miller-Rabin for 64-bit n */
    if (n < 2 || !(n & 1)) return n == 2;
    static const uint64_t bs[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    uint64_t d = n - 1; int s = 0; while (!(d & 1)) { d >>= 1; s++; }
    for (int i = 0; i < 12; i++) { uint64_t a = bs[i] % n; if (!a) continue; uint64_t x = powmod(a, d, n);
        if (x == 1 || x == n - 1) continue; int ok = 0;
        for (int r = 1; r < s; r++) { x = (uint64_t)((u128)x * x % n); if (x == n - 1) { ok = 1; break; } }
        if (!ok) return 0; }
    return 1;
}
static uint64_t Pinv64[MAXM + 1]; static u128 Pmod128[MAXM + 1];   /* for k moduli: (prod primes)^-1 mod 2^64, prod mod 2^128 */
static mpz_t Pz[MAXM + 1];                                         /* prod of primes p_1 .. p_{k-1} */
static mpz_t Mz[MAXM + 1];                                         /* M = 2^64 Pz */
static void rns_primes(void) {
    uint64_t p = (1ULL << 62) - 1; int m = 1;
    while (m < MAXM) { while (!isprime(p)) p -= 2; P[m++] = p; p -= 2; }
    for (m = 1; m < MAXM; m++) {
        PNI[m] = (uint64_t)0 - binv64(P[m]);
        u128 r = ((u128)1 << 64) % P[m]; ONE[m] = (uint64_t)r; R2[m] = (uint64_t)((r * r) % P[m]);
    }
    for (int i = 1; i < MAXM; i++) for (int j = 1; j < i; j++)
        CJI[j][i] = (uint64_t)(((u128)powmod(P[j] % P[i], P[i] - 2, P[i]) << 64) % P[i]);
    mpz_init_set_ui(Pz[1], 1); Pinv64[1] = 1; Pmod128[1] = 1;
    for (int k = 2; k <= MAXM; k++) {
        mpz_init(Pz[k]); mpz_mul_ui(Pz[k], Pz[k - 1], P[k - 1]);
        Pmod128[k] = Pmod128[k - 1] * P[k - 1]; Pinv64[k] = binv64((uint64_t)Pmod128[k]);
    }
}
static int rns_moduli_for(int bits) {           /* fewest moduli with M >= 2^bits, M = 2^64 prod primes (exact) */
    int k = 1; while (k < MAXM && mpz_sizeinbase(Mz[k], 2) - 1 < (size_t)bits) k++; return k;
}
static uint64_t to_mont(uint64_t x, int m) { return mont(x, R2[m], m); }
static uint64_t *rns_conv1(int which, int m) {    /* residues of one input array modulo one modulus (setup) */
    uint64_t *r = malloc(8 * N); mpz_t z; mpz_init(z);
    for (long i = 0; i < N; i++) {
        long l = which >= 4 ? LH : L; mpz_import(z, l, -1, 8, 0, 0, MAG[which] + i * l); if (SG[which][i]) mpz_neg(z, z);
        if (m == 0) { uint64_t lo = mpz_size(z) ? mpz_getlimbn(z, 0) : 0; r[i] = mpz_sgn(z) < 0 ? (uint64_t)0 - lo : lo; }
        else r[i] = to_mont((uint64_t)mpz_fdiv_ui(z, P[m]), m);
    }
    mpz_clear(z); return r;
}
static uint64_t **rns_conv(int which) { uint64_t **r = malloc(sizeof(uint64_t *) * KM); for (int m = 0; m < KM; m++) r[m] = rns_conv1(which, m); return r; }
static uint64_t **rns_alloc(int km) { uint64_t **r = malloc(sizeof(uint64_t *) * km); for (int m = 0; m < km; m++) r[m] = malloc(8 * N); return r; }
/* mixed-radix digits of element i over the primes, standard form: v[1..k-1]; returns t (x = y + P t, y < P) */
static inline uint64_t rns_mrc(uint64_t **R, long i, int k, uint64_t *v) {
    for (int a = 1; a < k; a++) {
        uint64_t x = mont(R[a][i], 1, a);                       /* out of Montgomery form */
        for (int j = 1; j < a; j++) {
            uint64_t vj = v[j] >= P[a] ? v[j] - P[a] : v[j];
            uint64_t d = x >= vj ? x - vj : x + P[a] - vj;
            x = mont(d, CJI[j][a], a);
        }
        v[a] = x;
    }
    u128 y = 0; for (int a = k - 1; a >= 1; a--) y = y * P[a] + v[a];   /* y mod 2^128 */
    return (R[0][i] - (uint64_t)y) * Pinv64[k];
}
static uint64_t **RK; static int rns_maxbits;
static int rns_less(int x, int y) {                 /* keys: (t ^ 2^63, v_{k-1} .. v_1), lexicographic */
    for (int a = 0; a < KM; a++) { uint64_t p = RK[a][x], q = RK[a][y]; if (p != q) return p < q; }
    return 0;
}
MSORT(rns_sortk, rns_less)
static void rns_setup(void) {
    KM = rns_moduli_for(W - 1);                 /* values of width W need W - 1 magnitude bits plus the sign */
    RA = rns_conv(0); RB = rns_conv(1); RE1 = rns_conv(2); RE2 = rns_conv(3); RO = rns_alloc(KM); RK = rns_alloc(KM);
    RHA = rns_conv(4); RHB = rns_conv(5);                    /* with the moduli of width W (pre-extended) */
    KH = rns_moduli_for(64 * LH - 1); RHW = rns_alloc(KM);    /* ha at width W/2, for the widen operation */
    for (int m = 0; m < KH; m++) { uint64_t *r = rns_conv1(4, m); memcpy(RHW[m], r, 8 * N); free(r); }
}
/* the magnitude of element i as limbs (for scan and output): y by Horner, x = y + P t, v = x - M if negative */
static int rns_value(uint64_t **R, long i, int k, mpz_t z, uint64_t *v) {
    uint64_t t = rns_mrc(R, i, k, v);
    mpz_set_ui(z, 0);
    for (int a = k - 1; a >= 1; a--) { mpz_mul_ui(z, z, P[a]); mpz_add_ui(z, z, v[a]); }
    mpz_addmul_ui(z, Pz[k], t);
    int neg = (int)(t >> 63);
    if (neg) mpz_sub(z, z, Mz[k]);
    return neg;
}
static mp_limb_t rns_buf[MAXM + 4], rns_P[MAXM + 1][MAXM + 2], rns_M[MAXM + 1][MAXM + 3]; static long rns_Pn[MAXM + 1], rns_Mn[MAXM + 1];
static int rns_bits_of(uint64_t **R, long i, int k, uint64_t *v) {   /* bit length of |x|, with mpn (timed path) */
    uint64_t t = rns_mrc(R, i, k, v);
    long n = 1; rns_buf[0] = k > 1 ? v[k - 1] : 0;
    for (int a = k - 2; a >= 1; a--) { mp_limb_t c = mpn_mul_1(rns_buf, rns_buf, n, P[a]); if (c) rns_buf[n++] = c;
        c = mpn_add_1(rns_buf, rns_buf, n, v[a]); if (c) rns_buf[n++] = c; }
    for (long j = n; j < rns_Mn[k] + 1; j++) rns_buf[j] = 0;
    long m = rns_Pn[k] + 1; if (m < n) m = n;
    mp_limb_t c = mpn_addmul_1(rns_buf, rns_P[k], rns_Pn[k], t); if (c) mpn_add_1(rns_buf + rns_Pn[k], rns_buf + rns_Pn[k], m + 1 - rns_Pn[k], c);
    long len = rns_Mn[k] + 1;
    if (t >> 63) mpn_sub_n(rns_buf, rns_M[k], rns_buf, len);       /* |x - M| = M - x */
    while (len && !rns_buf[len - 1]) len--;
    return len ? (int)(64 * (len - 1) + 64 - __builtin_clzll(rns_buf[len - 1])) : 0;
}
/* base extension: every element of R gets residues for moduli kfrom .. kto-1, through its mixed-radix digits */
static void rns_extend(uint64_t **R, int kfrom, int kto) {
    if (kto <= kfrom) return;
    uint64_t pm[MAXM], mm[MAXM], pa[MAXM][MAXM], v[MAXM];
    for (int m = kfrom; m < kto; m++) { pm[m] = (uint64_t)mpz_fdiv_ui(Pz[kfrom], P[m]); mm[m] = (uint64_t)mpz_fdiv_ui(Mz[kfrom], P[m]);
        for (int a = 1; a < kfrom; a++) pa[m][a] = P[a] % P[m]; }
    for (long i = 0; i < N; i++) {
        uint64_t t = rns_mrc(R, i, kfrom, v);
        for (int m = kfrom; m < kto; m++) {
            uint64_t p = P[m]; u128 y = 0;
            for (int a = kfrom - 1; a >= 1; a--) y = (y * pa[m][a] + v[a] % p) % p;
            u128 x = (y + (u128)pm[m] * (t % p)) % p;
            if (t >> 63) x = (x + p - mm[m]) % p;
            R[m][i] = to_mont((uint64_t)x, m);
        }
    }
}
static void rns_run(void) {
    int k = KM; uint64_t v[MAXM];
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) RO[0][i] = RA[0][i] + RB[0][i];
        for (int m = 1; m < k; m++) { uint64_t p = P[m], *a = RA[m], *b = RB[m], *o = RO[m];
            for (long i = 0; i < N; i++) { uint64_t s = a[i] + b[i]; o[i] = s >= p ? s - p : s; } }
        break;
    case MUL: for (long i = 0; i < N; i++) RO[0][i] = RHA[0][i] * RHB[0][i];
        for (int m = 1; m < k; m++) { uint64_t *a = RHA[m], *b = RHB[m], *o = RO[m]; for (long i = 0; i < N; i++) o[i] = mont(a[i], b[i], m); }
        break;
    case WIDEN: rns_extend(RHW, KH, k); break;
    case DOT: { uint64_t s0 = 0; for (long i = 0; i < N; i++) s0 += RHA[0][i] * RHB[0][i]; RSUM[0] = s0;
        for (int m = 1; m < k; m++) { uint64_t p = P[m], s = 0, *a = RHA[m], *b = RHB[m];
            for (long i = 0; i < N; i++) { s += mont(a[i], b[i], m); if (s >= p) s -= p; } RSUM[m] = s; }
        break; }
    case DIV1: { uint64_t i64 = binv64(999); for (long i = 0; i < N; i++) RO[0][i] = RE1[0][i] * i64;
        for (int m = 1; m < k; m++) { uint64_t c = to_mont(powmod(999, P[m] - 2, P[m]), m), *e = RE1[m], *o = RO[m];
            for (long i = 0; i < N; i++) o[i] = mont(e[i], c, m); }
        break; }
    case DIV2: { uint64_t i125 = binv64(125); u128 Mlo = Pmod128[k] << 64;   /* M mod 2^128 */
        for (long i = 0; i < N; i++) {                   /* the 2^64 residue needs x mod 2^67: mixed radix */
            uint64_t t = rns_mrc(RE2, i, k, v);
            u128 y = 0; for (int a = k - 1; a >= 1; a--) y = y * P[a] + v[a];
            u128 x = y + Pmod128[k] * t; if (t >> 63) x -= Mlo;
            RO[0][i] = (uint64_t)(x >> 3) * i125;
        }
        for (int m = 1; m < k; m++) { uint64_t c = to_mont(powmod(1000, P[m] - 2, P[m]), m), *e = RE2[m], *o = RO[m];
            for (long i = 0; i < N; i++) o[i] = mont(e[i], c, m); }
        break; }
    case SORT:
        for (long i = 0; i < N; i++) { uint64_t t = rns_mrc(RA, i, k, v); RK[0][i] = t ^ (1ULL << 63);
            for (int a = 1; a < k; a++) RK[a][i] = v[k - a]; }
        rns_sortk(); break;
    case SCAN: { int m = 0; for (long i = 0; i < N; i++) { int b = rns_bits_of(RA, i, k, v); if (b > m) m = b; } rns_maxbits = m; break; }
    }
}
static void rns_out_vec(uint64_t **R, int k) { uint64_t v[MAXM]; for (long i = 0; i < N; i++) rns_value(R, i, k, ZOUT[i], v); out_kind = 0; }
static void rns_out(void) {
    if (OP == DOT) { uint64_t *R1[MAXM], v[MAXM]; uint64_t cell[MAXM]; for (int m = 0; m < KM; m++) { cell[m] = RSUM[m]; R1[m] = &cell[m]; }
        rns_value(R1, 0, KM, ZONE, v); out_kind = 1; return; }
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)rns_maxbits); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    rns_out_vec(OP == WIDEN ? RHW : RO, KM);
}
/* rns chain: moduli are added (base extension through mixed radix) when the proven bound needs them */
static uint64_t **RX, **RXB, **RXC;               /* x, and b, c in Montgomery form per modulus (made once per modulus) */
static int rns_chain(void) {
    chain_bounds(); int bx = bx0(), k = 1;
    for (long i = 0; i < N; i++) RX[0][i] = (uint64_t)X0[i];
    uint64_t csr[MAXM]; csr[0] = 1; for (int m = 1; m < MAXM; m++) csr[m] = ONE[m];    /* DEN^(k+1) per modulus */
    uint64_t denm[MAXM]; for (int m = 1; m < MAXM; m++) denm[m] = to_mont((uint64_t)DEN % P[m], m);
    for (long s = 0; s < K; s++) {
        bx = chain_next(bx);
        if (bx + 1 > CAP) return 0;                       /* the same cap as the others */
        int need = rns_moduli_for(bx + 1);               /* values lie in [-M/2, M/2): |x| < 2^bx needs M >= 2^(bx+1) */
        if (need > k) {                                   /* base extension of every element to the new primes */
            rns_extend(RX, k, need);
            for (int m = k; m < need; m++) {
                uint64_t p = P[m];
                for (long i = 0; i < N; i++) {
                    RXB[m][i] = to_mont(CB[i] < 0 ? p - uabs64(CB[i]) % p : (uint64_t)CB[i] % p, m);
                    RXC[m][i] = to_mont(CC[i] < 0 ? p - uabs64(CC[i]) % p : (uint64_t)CC[i] % p, m);
                }
            }
            k = need;
        }
        csr[0] *= (uint64_t)DEN; for (int m = 1; m < MAXM; m++) csr[m] = mont(csr[m], denm[m], m);
        for (long i = 0; i < N; i++) RX[0][i] = RX[0][i] * (uint64_t)CB[i] + (uint64_t)CC[i] * csr[0];
        for (int m = 1; m < k; m++) {
            uint64_t p = P[m], *x = RX[m], *bm = RXB[m], *cv = RXC[m], cm = csr[m];
            for (long i = 0; i < N; i++) { uint64_t s1 = mont(x[i], bm[i], m) + mont(cv[i], cm, m); x[i] = s1 >= p ? s1 - p : s1; }
        }
    }
    KM = k; final_width = (int)mpz_sizeinbase(Mz[k], 2) - 1; return 1;
}

/* ================================================================ mpz: GMP integers */
static mpz_t *za, *zb, *ze1, *ze2, *zha, *zhb, *zo, zs; static int zmax;
static mpz_t *zconv(int which) { mpz_t *x = malloc(sizeof(mpz_t) * N); long l = which >= 4 ? LH : L;
    for (long i = 0; i < N; i++) { mpz_init2(x[i], W); mpz_import(x[i], l, -1, 8, 0, 0, MAG[which] + i * l); if (SG[which][i]) mpz_neg(x[i], x[i]); }
    return x; }
static int z_less(int x, int y) { return mpz_cmp(za[x], za[y]) < 0; }
MSORT(z_sortk, z_less)
static void z_setup(void) { za = zconv(0); zb = zconv(1); ze1 = zconv(2); ze2 = zconv(3); zha = zconv(4); zhb = zconv(5);
    zo = malloc(sizeof(mpz_t) * N); for (long i = 0; i < N; i++) mpz_init2(zo[i], W); mpz_init2(zs, W + 64); }
static void z_run(void) {
    switch (OP) {
    case ADD: for (long i = 0; i < N; i++) mpz_add(zo[i], za[i], zb[i]); break;
    case MUL: for (long i = 0; i < N; i++) mpz_mul(zo[i], zha[i], zhb[i]); break;
    case DOT: mpz_set_ui(zs, 0); for (long i = 0; i < N; i++) mpz_addmul(zs, zha[i], zhb[i]); break;
    case WIDEN: break;                         /* nothing to do: an mpz_t has no width */
    case DIV1: for (long i = 0; i < N; i++) mpz_divexact_ui(zo[i], ze1[i], 999); break;
    case DIV2: for (long i = 0; i < N; i++) mpz_divexact_ui(zo[i], ze2[i], 1000); break;
    case SORT: z_sortk(); break;
    case SCAN: { size_t m = 0; for (long i = 0; i < N; i++) { size_t b = mpz_sgn(za[i]) ? mpz_sizeinbase(za[i], 2) : 0; if (b > m) m = b; } zmax = (int)m; break; }
    }
}
static void z_out(void) {
    if (OP == DOT) { mpz_set(ZONE, zs); out_kind = 1; return; }
    if (OP == SCAN) { mpz_set_ui(ZONE, (unsigned long)zmax); out_kind = 1; return; }
    if (OP == SORT) { out_kind = 2; return; }
    for (long i = 0; i < N; i++) mpz_set(ZOUT[i], OP == WIDEN ? zha[i] : zo[i]); out_kind = 0;
}
static mpz_t *zx;
static int z_chain(void) {
    chain_bounds(); int bx = bx0();
    for (long i = 0; i < N; i++) mpz_set_si(zx[i], X0[i]);
    mpz_t cs, t; mpz_init_set_ui(cs, 1); mpz_init(t);
    for (long k = 0; k < K; k++) {
        bx = chain_next(bx); if (bx + 1 > CAP) { mpz_clear(cs); mpz_clear(t); return 0; }   /* the same cap as the others */
        mpz_mul_ui(cs, cs, (unsigned long)DEN);
        for (long i = 0; i < N; i++) { mpz_mul_si(zx[i], zx[i], CB[i]); mpz_mul_si(t, cs, CC[i]); mpz_add(zx[i], zx[i], t); }
    }
    mpz_clear(cs); mpz_clear(t); final_width = 0; return 1;
}

/* ================================================================ main */
int main(int argc, char **argv) {
    if (argc < 8) { fprintf(stderr, "usage: p1_arith REP OP W N REPS INDIR OUTFILE [K DEN]\n"); return 2; }
    const char *rep = argv[1], *op = argv[2]; W = atoi(argv[3]); N = atol(argv[4]); int reps = atoi(argv[5]);
    const char *dir = argv[6]; L = W / 64; LH = L > 1 ? L / 2 : 1;
    const char *ops[] = {"add", "mul", "dot", "div1", "div2", "sort", "scan", "widen", "chain"};
    OP = -1; for (int i = 0; i < 9; i++) if (is(op, ops[i])) OP = i;
    if (OP < 0 || (OP != CHAIN && (W < 64 || W > CAP || (W & (W - 1))))) { fprintf(stderr, "bad op or width\n"); return 2; }
    int bi = is(rep, "bi"), lc = is(rep, "lc"), lr = is(rep, "lr"), rn = is(rep, "rns"), mz = is(rep, "mpz");
    if (!(bi || lc || lr || rn || mz)) { fprintf(stderr, "bad rep\n"); return 2; }
    mpz_init(PW); mpz_init(ZONE);
    rns_primes();
    for (int k = 1; k <= MAXM; k++) {
        mpz_init(Mz[k]); mpz_mul_2exp(Mz[k], Pz[k], 64);
        size_t c; memset(rns_P[k], 0, sizeof rns_P[k]); mpz_export(rns_P[k], &c, -1, 8, 0, 0, Pz[k]); rns_Pn[k] = (long)c;
        memset(rns_M[k], 0, sizeof rns_M[k]); mpz_export(rns_M[k], &c, -1, 8, 0, 0, Mz[k]); rns_Mn[k] = (long)c;
    }
    perm = malloc(sizeof(int) * N); ptmp = malloc(sizeof(int) * N);
    ZOUT = malloc(sizeof(mpz_t) * N); for (long i = 0; i < N; i++) mpz_init(ZOUT[i]);
    if (OP == CHAIN) {
        if (argc < 10) { fprintf(stderr, "chain needs K DEN\n"); return 2; }
        K = atol(argv[8]); DEN = atol(argv[9]);
        X0 = loadf(dir, "x0.bin", 8 * N); CB = loadf(dir, "cb.bin", 8 * N); CC = loadf(dir, "cc.bin", 8 * N);
        if (bi) for (int w = 0; w < 6; w++) bix[w] = malloc((size_t)N * (8 << w));
        if (lc) { for (int w = 0; w < 6; w++) lcx[w] = malloc((size_t)N * (8 << w)); lcxs = malloc(N); }
        if (lr) { for (int b = 0; b < 2; b++) lrbuf[b] = malloc((size_t)N * (CAP / 8)); lrxs = malloc(N);
                  lrt1 = malloc(8 * (CAP / 64 + 1)); lrt2 = malloc(8 * (CAP / 64 + 1)); lrcs = malloc(8 * (CAP / 64 + 1)); }
        if (rn) { RX = rns_alloc(MAXM); RXB = rns_alloc(MAXM); RXC = rns_alloc(MAXM); }
        if (mz) { zx = malloc(sizeof(mpz_t) * N); for (long i = 0; i < N; i++) mpz_init2(zx[i], CAP); }
    } else {
        const char *nm[6] = {"a", "b", "e1", "e2", "ha", "hb"};
        for (int a = 0; a < 6; a++) { char f[16]; snprintf(f, sizeof f, "%s.mag", nm[a]); MAG[a] = loadf(dir, f, 8 * (size_t)N * (a >= 4 ? LH : L));
            snprintf(f, sizeof f, "%s.sgn", nm[a]); SG[a] = loadf(dir, f, N); }
        if (bi) switch (W) {
#define BISET(WW) case WW: bi_setup##WW(); break;
            WIDTHS(BISET) }
        if (lc) switch (L) {
#define LCSET(LL) case LL: lc_setup##LL(); break;
            LCS(LCSET) }
        if (lr) lr_setup(); if (rn) rns_setup(); if (mz) z_setup();
    }
    double *t = malloc(sizeof(double) * reps); int ok = 1;
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (OP == CHAIN) ok = bi ? bi_chain() : lc ? lc_chain() : lr ? lr_chain() : rn ? rns_chain() : z_chain();
        else if (bi) switch (W) {
#define BIRUN(WW) case WW: bi_run##WW(); break;
            WIDTHS(BIRUN) }
        else if (lc) switch (L) {
#define LCRUN(LL) case LL: lc_run##LL(); break;
            LCS(LCRUN) }
        else if (lr) lr_run(); else if (rn) rns_run(); else z_run();
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
        if (!ok) break;
    }
    if (!ok) { printf("exceeds\n"); return 3; }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %d\n", t[reps / 2], t[0], OP == CHAIN ? final_width : W);
    if (OP == CHAIN) {
        if (bi) bi_chain_out(); else if (lc) lc_chain_out(); else if (lr) lr_chain_out();
        else if (rn) rns_out_vec(RX, KM); else { for (long i = 0; i < N; i++) mpz_set(ZOUT[i], zx[i]); out_kind = 0; }
    } else if (bi) switch (W) {
#define BIOUT(WW) case WW: bi_out##WW(); break;
        WIDTHS(BIOUT) }
    else if (lc) switch (L) {
#define LCOUT(LL) case LL: lc_out##LL(); break;
        LCS(LCOUT) }
    else if (lr) lr_out(); else if (rn) rns_out(); else z_out();
    FILE *f = fopen(argv[7], "w");
    if (out_kind == 1) gmp_fprintf(f, "%Zd\n", ZONE);
    else if (out_kind == 2) for (long i = 0; i < N; i++) fprintf(f, "%d\n", perm[i]);
    else for (long i = 0; i < N; i++) gmp_fprintf(f, "%Zd\n", ZOUT[i]);
    fclose(f);
    return 0;
}
