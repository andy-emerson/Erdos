/* Phase two, single values: the packed format as a machine type against GMP and FLINT rationals.

   usage: p2_scalar REP OP W N REPS INFILE OUTFILE
     REP  fmt   the packed format, W = 64 or 128 bits, laid out as agreed: the unit field (sign and axis, 4 bits)
                in the lowest bits, the slash field (k bits) at the top, the data (numerator, then denominator - 1)
                between. Each value is decoded, the arithmetic done exactly, the result reduced and encoded.
          mpq   GMP rationals          fmpq  FLINT rationals
     OP   add   a[i] + b[i]   (real values: axis 0)
          mul   a[i] * b[i]   (any axes: the unit product comes from the octonion table)
          sort  order a (stable merge sort of indices by value)
   Every value is independent (its own denominator): this is scalar arithmetic, not the vector form.
   INFILE holds 2N records (sign int8, axis int8, pad 6, p lo, p hi, q lo, q hi as uint64): a then b.
   Prints "median min failures"; failures are results that do not fit W bits (none by construction); writes the
   results as "sign axis p q" lines, or the permutation for sort. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/fmpq.h>
#include <flint/fmpq_vec.h>

typedef unsigned __int128 u128; typedef __int128 i128;
typedef unsigned _BitInt(256) u256;
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static inline int bitlen128(u128 x) { uint64_t hi = (uint64_t)(x >> 64), lo = (uint64_t)x; return hi ? 128 - __builtin_clzll(hi) : lo ? 64 - __builtin_clzll(lo) : 0; }
static inline int ctz128(u128 x) { uint64_t lo = (uint64_t)x; return lo ? __builtin_ctzll(lo) : 64 + __builtin_ctzll((uint64_t)(x >> 64)); }
static inline u128 gcd128(u128 a, u128 b) { if (!a) return b; if (!b) return a; int sh = ctz128(a | b); a >>= ctz128(a);
    do { b >>= ctz128(b); if (a > b) { u128 t = a; a = b; b = t; } b -= a; } while (b); return a << sh; }
static const int OAXIS[8][8] = {{0,1,2,3,4,5,6,7},{1,0,3,2,5,4,7,6},{2,3,0,1,6,7,4,5},{3,2,1,0,7,6,5,4},{4,5,6,7,0,1,2,3},{5,4,7,6,1,0,3,2},{6,7,4,5,2,3,0,1},{7,6,5,4,3,2,1,0}};
static const int OSIGN[8][8] = {{1,1,1,1,1,1,1,1},{1,-1,1,-1,1,-1,-1,1},{1,-1,-1,1,1,1,-1,-1},{1,1,-1,-1,1,-1,1,-1},{1,-1,-1,-1,-1,1,1,1},{1,1,-1,1,-1,-1,-1,1},{1,1,1,-1,-1,1,-1,-1},{1,-1,1,1,-1,-1,1,-1}};
typedef struct { int sign, axis; u128 p, q; } val;

/* the agreed layout: unit (4 bits) low, slash (K bits) top, data (D bits) between */
#define FMT(T, WB, K, D) \
static inline val dec##WB(T v) { val r; int code = (int)(v & 15); r.sign = (code >> 3) ? -1 : 1; r.axis = code & 7; \
    int s = (int)(v >> ((WB) - (K))); if (s > (D) - 1) s = (D) - 1; T data = (v >> 4) & (((T)1 << (D)) - 1); int w = (D) - (s + 1); \
    r.p = (u128)(data >> w); r.q = (u128)(data & (((T)1 << w) - 1)) + 1; if (!r.p) { r.sign = 1; r.axis = 0; r.q = 1; } return r; } \
static inline int enc##WB(val x, T *out) { if (!x.p) { *out = 0; return 1; } \
    int lp = bitlen128(x.p), lq = bitlen128(x.q - 1); if (lp + lq > (D)) return 0; \
    T data = (T)((x.p << ((D) - lp)) | (x.q - 1)); \
    *out = ((T)(lp - 1) << ((WB) - (K))) | (data << 4) | (T)(((x.sign < 0) << 3) | x.axis); return 1; }
FMT(uint64_t, 64, 6, 54)
FMT(u128, 128, 7, 117)

static long N; static int W;
static val *IN; static void *FA, *FB, *FO;
static int *perm, *ptmp;
#define MSORT(NAME, LESS) static void NAME(void) { for (long i = 0; i < N; i++) perm[i] = (int)i; int *src = perm, *dst = ptmp; \
    for (long w = 1; w < N; w *= 2) { for (long lo = 0; lo < N; lo += 2 * w) { long mid = lo + w < N ? lo + w : N, hi = lo + 2 * w < N ? lo + 2 * w : N, i = lo, j = mid, k = lo; \
        while (i < mid && j < hi) { int x = src[i], y = src[j]; if (LESS(y, x)) dst[k++] = src[j++]; else dst[k++] = src[i++]; } \
        while (i < mid) dst[k++] = src[i++]; while (j < hi) dst[k++] = src[j++]; } int *t = src; src = dst; dst = t; } \
    if (src != perm) memcpy(perm, src, N * sizeof(int)); }

/* exact add and multiply of decoded values, by Henrici's algorithms (the ones GMP and FLINT use): the gcds are taken
   of the operands, not of the products, so they stay small and the result comes out reduced; comparisons by
   cross-multiplication. Word-size gcd and division whenever the operands fit one word. */
static inline uint64_t gcd64(uint64_t a, uint64_t b) { if (!a) return b; if (!b) return a; int sh = __builtin_ctzll(a | b); a >>= __builtin_ctzll(a);
    do { b >>= __builtin_ctzll(b); if (a > b) { uint64_t t = a; a = b; b = t; } b -= a; } while (b); return a << sh; }
static inline u128 gcdu(u128 a, u128 b) { return (a >> 64) | (b >> 64) ? gcd128(a, b) : gcd64((uint64_t)a, (uint64_t)b); }
static inline u128 divu(u128 a, u128 g) { return g == 1 ? a : ((a >> 64) | (g >> 64)) ? a / g : (u128)((uint64_t)a / (uint64_t)g); }
static inline val vmul(val x, val y) {
    val z; u128 g1 = gcdu(x.p, y.q), g2 = gcdu(y.p, x.q);
    z.p = divu(x.p, g1) * divu(y.p, g2); z.q = divu(x.q, g2) * divu(y.q, g1);
    z.sign = x.sign * y.sign * OSIGN[x.axis][y.axis]; z.axis = OAXIS[x.axis][y.axis];
    if (!z.p) { z.sign = 1; z.axis = 0; z.q = 1; } return z; }
#define VADD(NAME, WT) static inline val NAME(val x, val y) { \
    val z; u128 g = gcdu(x.q, y.q), qa = divu(x.q, g), qb = divu(y.q, g); \
    WT a = (WT)x.p * qb, b = (WT)y.p * qa, t; int sg; \
    if (x.sign == y.sign) { t = a + b; sg = x.sign; } else if (a >= b) { t = a - b; sg = x.sign; } else { t = b - a; sg = y.sign; } \
    if (!t) { z.p = 0; z.q = 1; z.sign = 1; z.axis = 0; return z; } \
    if ((t >> 127) >> 1) { z.p = 0; z.q = 0; z.sign = sg; z.axis = 0; return z; }    /* too wide: a failure */ \
    u128 tt = (u128)t, g2 = g == 1 ? 1 : gcdu(tt % g, g);                 /* gcd(t, g) */ \
    z.p = divu(tt, g2); z.q = qa * divu(y.q, g2); z.sign = sg; z.axis = 0; return z; }
VADD(vadd64, u128)
VADD(vadd128, u256)
#define VLESS(NAME, WT) static inline int NAME(val x, val y) { \
    int sx = x.p ? x.sign : 0, sy = y.p ? y.sign : 0; if (sx != sy) return sx < sy; \
    WT a = (WT)x.p * y.q, b = (WT)y.p * x.q; return sx >= 0 ? a < b : a > b; }
VLESS(vless64, u128)
VLESS(vless128, u256)
#define LESS64(i, j) vless64(dec64(((uint64_t *)FA)[i]), dec64(((uint64_t *)FA)[j]))
#define LESS128(i, j) vless128(dec128(((u128 *)FA)[i]), dec128(((u128 *)FA)[j]))
MSORT(sort64, LESS64) MSORT(sort128, LESS128)
static mpq_t *qa, *qb, *qo; static fmpq *ga, *gb, *go;
static int q_less(int i, int j) { return mpq_cmp(qa[i], qa[j]) < 0; }
static int g_less(int i, int j) { return fmpq_cmp(ga + i, ga + j) < 0; }
MSORT(qsortk, q_less) MSORT(gsortk, g_less)
static int fails;

#define RUNFMT(T, WB) static void run##WB(const char *op) { T *a = FA, *b = FB, *o = FO; int f = 0; \
    if (is(op, "add")) for (long i = 0; i < N; i++) { val z = vadd##WB(dec##WB(a[i]), dec##WB(b[i])); f += !z.q || !enc##WB(z, &o[i]); } \
    else if (is(op, "mul")) for (long i = 0; i < N; i++) f += !enc##WB(vmul(dec##WB(a[i]), dec##WB(b[i])), &o[i]); \
    else sort##WB(); fails = f; }
RUNFMT(uint64_t, 64) RUNFMT(u128, 128)

int main(int argc, char **argv) {
    if (argc != 8) { fprintf(stderr, "usage: p2_scalar REP OP W N REPS INFILE OUTFILE\n"); return 2; }
    const char *rep = argv[1], *op = argv[2]; W = atoi(argv[3]); N = atol(argv[4]); int reps = atoi(argv[5]);
    if (W != 64 && W != 128) { fprintf(stderr, "W is 64 or 128\n"); return 2; }
    FILE *f = fopen(argv[6], "rb"); IN = malloc(sizeof(val) * 2 * N);
    for (long i = 0; i < 2 * N; i++) { int8_t sa[8]; uint64_t q4[4]; if (fread(sa, 1, 8, f) != 8 || fread(q4, 8, 4, f) != 4) { fprintf(stderr, "short\n"); return 2; }
        IN[i].sign = sa[0]; IN[i].axis = sa[1]; IN[i].p = ((u128)q4[1] << 64) | q4[0]; IN[i].q = ((u128)q4[3] << 64) | q4[2]; }
    fclose(f);
    perm = malloc(sizeof(int) * N); ptmp = malloc(sizeof(int) * N);
    int fmt = is(rep, "fmt"), mq = is(rep, "mpq");
    if (fmt) { size_t sz = W / 8; FA = malloc(sz * N); FB = malloc(sz * N); FO = calloc(N, sz);
        for (long i = 0; i < N; i++) { int ok = W == 64 ? enc64(IN[i], (uint64_t *)FA + i) && enc64(IN[N + i], (uint64_t *)FB + i)
                                                       : enc128(IN[i], (u128 *)FA + i) && enc128(IN[N + i], (u128 *)FB + i);
            if (!ok) { fprintf(stderr, "input does not fit\n"); return 2; } } }
    else {
        mpz_t z; mpz_init(z);
        if (mq) { qa = malloc(sizeof(mpq_t) * N); qb = malloc(sizeof(mpq_t) * N); qo = malloc(sizeof(mpq_t) * N); }
        else { ga = _fmpq_vec_init(N); gb = _fmpq_vec_init(N); go = _fmpq_vec_init(N); }
        for (long i = 0; i < 2 * N; i++) {
            mpq_t v; mpq_init(v);
            mpz_import(mpq_numref(v), 2, -1, 8, 0, 0, (uint64_t[2]){(uint64_t)IN[i].p, (uint64_t)(IN[i].p >> 64)});
            mpz_import(mpq_denref(v), 2, -1, 8, 0, 0, (uint64_t[2]){(uint64_t)IN[i].q, (uint64_t)(IN[i].q >> 64)});
            if (IN[i].sign < 0) mpq_neg(v, v);
            if (IN[i].axis) { fprintf(stderr, "libraries: real values only (axis 0)\n"); return 2; }
            if (mq) { mpq_t *dst = i < N ? &qa[i] : &qb[i - N]; mpq_init(*dst); mpq_set(*dst, v); }
            else fmpq_set_mpq(i < N ? ga + i : gb + i - N, v);
            mpq_clear(v);
        }
        if (mq) for (long i = 0; i < N; i++) mpq_init(qo[i]);
        mpz_clear(z);
    }
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        if (fmt) { if (W == 64) run64(op); else run128(op); }
        else if (mq) { if (is(op, "add")) for (long i = 0; i < N; i++) mpq_add(qo[i], qa[i], qb[i]);
                       else if (is(op, "mul")) for (long i = 0; i < N; i++) mpq_mul(qo[i], qa[i], qb[i]); else qsortk(); }
        else { if (is(op, "add")) for (long i = 0; i < N; i++) fmpq_add(go + i, ga + i, gb + i);
               else if (is(op, "mul")) for (long i = 0; i < N; i++) fmpq_mul(go + i, ga + i, gb + i); else gsortk(); }
        double dt = now() - t0; if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %d\n", t[reps / 2], t[0], fails);
    FILE *o = fopen(argv[7], "w");
    if (is(op, "sort")) for (long i = 0; i < N; i++) fprintf(o, "%d\n", perm[i]);
    else for (long i = 0; i < N; i++) {
        if (fmt) { val z = W == 64 ? dec64(((uint64_t *)FO)[i]) : dec128(((u128 *)FO)[i]);
            mpz_t p, q; mpz_init(p); mpz_init(q);
            mpz_import(p, 2, -1, 8, 0, 0, (uint64_t[2]){(uint64_t)z.p, (uint64_t)(z.p >> 64)}); mpz_import(q, 2, -1, 8, 0, 0, (uint64_t[2]){(uint64_t)z.q, (uint64_t)(z.q >> 64)});
            gmp_fprintf(o, "%d %d %Zd %Zd\n", z.sign, z.axis, p, q); mpz_clear(p); mpz_clear(q); }
        else { mpq_t v; mpq_init(v); if (mq) mpq_set(v, qo[i]); else fmpq_get_mpq(v, go + i);
            int sg = mpq_sgn(v) < 0 ? -1 : 1; mpq_abs(v, v);
            gmp_fprintf(o, "%d 0 %Zd %Zd\n", sg, mpq_numref(v), mpq_denref(v)); mpq_clear(v); }
    }
    fclose(o);
    return 0;
}
