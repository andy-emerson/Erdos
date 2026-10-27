#define _POSIX_C_SOURCE 200809L
/* The threshold test at 1024 bits: is 1024 the last size for the small routines (length fixed at compile time) or
   the first for the large ones (length set at run time)?

   usage: p1_thresh SET KERNEL OP N REPS INDIR OUTFILE [K]
     SET     small  the length is the compile-time constant LC (16 limbs = 1024 bits)
             large  the length is read at run time (16, from the command line, opaque to the compiler)
     KERNEL  c      hand-written C loops (the compiler can unroll and specialise them when the length is constant)
             cn     the same C loops with unrolling disabled (unrolling made some loops slower: each set may choose)
             asm    GMP's mpn routines (mpn_add_n, mpn_sub_n, mpn_mul_n, mpn_divexact_1, mpn_mul_1, mpn_addmul_1,
                    mpn_submul_1 are assembly and take the length as an argument either way; mpn_cmp, mpn_zero_p and
                    mpn_neg are inline C in gmp.h, so a constant length does help them)
             Each set may use either kernel for each operation: the harness takes each set's faster one, so a set
             is "everything that runs with its kind of length", and the only difference between the sets is the
             one being tested.
     OP      add   a + b                    mul   ha hb (inputs at 512 bits)    dot  sum of ha hb
             div1  e1 / 999 (exact)         div2  e2 / 1000 (exact)              sort  order a
             scan  largest bit length of a  widen ha from 512 to 1024 bits
             step  K chain steps x <- x b + c 1000^(k+1) on values that start above 512 bits and stay at 1024
             pipe  mul, add, div1, add, div2, add, step (one), sort, scan, each result feeding the next, using the
                   kernels named in KERNEL as a comma list (mul=asm,add=c,...); the harness passes each set's best
   Values are sign-magnitude: a separate sign byte and LC limbs. Inputs are built before timing; outputs written
   after. Prints "median min"; writes results as decimal integers (vectors, then scalars, then the order). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>

#define LC 16
typedef unsigned __int128 u128;
static long N, K; static long LR;                 /* LR: the run-time length (16) */
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static uint64_t binv64(uint64_t d) { uint64_t x = d; for (int i = 0; i < 6; i++) x *= 2 - d * x; return x; }
static uint64_t umulh(uint64_t a, uint64_t b) { return (uint64_t)(((u128)a * b) >> 64); }
static void *loadf(const char *dir, const char *name, size_t bytes) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(2); }
    void *x = malloc(bytes); if (fread(x, 1, bytes, f) != bytes) { fprintf(stderr, "short read %s\n", name); exit(2); }
    fclose(f); return x;
}

/* ---------------------------------------------------------------- the C kernels, written once for both sets.
   L is either the macro LC (small) or the variable LR (large): the same source, compiled twice. */
#define KERNELS(SFX, L, UNR) \
static inline int cmp_##SFX(const uint64_t *a, const uint64_t *b) { UNR for (long j = (L) - 1; j >= 0; j--) if (a[j] != b[j]) return a[j] > b[j] ? 1 : -1; return 0; } \
static inline void addm_##SFX(uint64_t *r, const uint64_t *a, const uint64_t *b) { unsigned char c = 0; \
    UNR for (long j = 0; j < (L); j++) { u128 t = (u128)a[j] + b[j] + c; r[j] = (uint64_t)t; c = (unsigned char)(t >> 64); } } \
static inline void subm_##SFX(uint64_t *r, const uint64_t *a, const uint64_t *b) { uint64_t br = 0; \
    UNR for (long j = 0; j < (L); j++) { uint64_t aj = a[j], bj = b[j], t = aj - bj - br; br = (aj < bj) | ((aj - bj) < br); r[j] = t; } } \
static inline void sadd_##SFX(uint64_t *r, unsigned char *sr, const uint64_t *a, int sa, const uint64_t *b, int sb, int useasm) { \
    if (sa == sb) { if (useasm) mpn_add_n(r, a, b, (L)); else addm_##SFX(r, a, b); *sr = (unsigned char)sa; return; } \
    int c = useasm ? mpn_cmp(a, b, (L)) : cmp_##SFX(a, b); \
    if (c >= 0) { if (useasm) mpn_sub_n(r, a, b, (L)); else subm_##SFX(r, a, b); *sr = c == 0 ? 0 : (unsigned char)sa; } \
    else { if (useasm) mpn_sub_n(r, b, a, (L)); else subm_##SFX(r, b, a); *sr = (unsigned char)sb; } } \
static inline void mulm_##SFX(uint64_t *r, const uint64_t *a, const uint64_t *b, int useasm) {  /* L/2-limb inputs */ \
    long h = (L) / 2; \
    if (useasm) { mpn_mul_n(r, a, b, h); return; } \
    UNR for (long j = 0; j < (L); j++) r[j] = 0; \
    UNR for (long i = 0; i < h; i++) { uint64_t c = 0; \
        UNR for (long j = 0; j < h; j++) { u128 p = (u128)a[i] * b[j] + r[i + j] + c; r[i + j] = (uint64_t)p; c = (uint64_t)(p >> 64); } \
        r[i + h] = c; } } \
static inline void divx_##SFX(uint64_t *q, const uint64_t *x, uint64_t d, uint64_t inv, int useasm) {  /* exact */ \
    if (useasm) { mpn_divexact_1(q, x, (L), d); return; } \
    int sh = __builtin_ctzll(d); uint64_t od = d >> sh, oinv = sh ? binv64(od) : inv, c = 0; \
    UNR for (long j = 0; j < (L); j++) { uint64_t s = sh ? ((x[j] >> sh) | (j + 1 < (L) ? x[j + 1] << (64 - sh) : 0)) : x[j]; \
        uint64_t b1 = s < c; s -= c; uint64_t qj = s * oinv; q[j] = qj; c = umulh(qj, od) + b1; } } \
static inline int scanbits_##SFX(const uint64_t *v) { int m = 0; \
    UNR for (long j = (L) - 1; j >= 0 && !m; j--) { uint64_t acc = 0; UNR for (long i = 0; i < N; i++) acc |= v[i * (L) + j]; \
        if (acc) m = (int)(64 * j + 64 - __builtin_clzll(acc)); } return m; } \
/* one chain step on one value: x <- x |b| (sign sx ^ sb) + cs |c| (sign sc); magnitudes fit L limbs by the bound */ \
static inline void step1_##SFX(uint64_t *x, unsigned char *sx, uint64_t bm, int sb, const uint64_t *cs, uint64_t cm, int sc, uint64_t *t, int useasm) { \
    int s1 = (*sx ^ sb) & 1; \
    if (useasm) { mpn_mul_1(x, x, (L), bm); \
        if (s1 == sc) mpn_addmul_1(x, cs, (L), cm); \
        else if (mpn_submul_1(x, cs, (L), cm)) { mpn_neg(x, x, (L)); s1 = sc; } \
        *sx = (unsigned char)(mpn_zero_p(x, (L)) ? 0 : s1); return; } \
    uint64_t c1 = 0, c2 = 0; \
    UNR for (long j = 0; j < (L); j++) { u128 p = (u128)x[j] * bm + c1; x[j] = (uint64_t)p; c1 = (uint64_t)(p >> 64); \
                                     u128 q = (u128)cs[j] * cm + c2; t[j] = (uint64_t)q; c2 = (uint64_t)(q >> 64); } \
    sadd_##SFX(x, sx, x, s1, t, sc, 0); \
    int z = 1; UNR for (long j = 0; j < (L); j++) if (x[j]) { z = 0; break; } if (z) *sx = 0; }
#define NOUNROLL _Pragma("clang loop unroll(disable)")
#define UNROLLOK
KERNELS(s, LC, UNROLLOK)
KERNELS(l, LR, UNROLLOK)
KERNELS(sn, LC, NOUNROLL)
KERNELS(ln, LR, NOUNROLL)

/* ---------------------------------------------------------------- data */
static uint64_t *A, *B, *E1, *E2, *HA, *HB, *O, *T2, *T3, *V, *X, *SUM, *TMP; static unsigned char *SA, *SB, *S1, *S2, *SHA, *SHB, *SO, *ST2, *ST3, *SV, *SX, SSUM;
static int64_t *CB, *CC; static uint64_t *CS; static int SCAN;
static int *perm, *ptmp; static uint64_t *KEYV; static unsigned char *KEYS; static int KASM, SMALL, CMPN;
static int lessv(int x, int y) { int sx = KEYS[x], sy = KEYS[y]; if (sx != sy) return sx > sy;
    const uint64_t *p = KEYV + (long)x * LC, *q = KEYV + (long)y * LC;
    int c = KASM ? mpn_cmp(p, q, SMALL ? LC : LR) : SMALL ? (CMPN ? cmp_sn(p, q) : cmp_s(p, q)) : (CMPN ? cmp_ln(p, q) : cmp_l(p, q));
    return sx ? c > 0 : c < 0; }
static void sortv(uint64_t *v, unsigned char *s, int useasm) {      /* stable bottom-up merge sort of indices */
    KEYV = v; KEYS = s; KASM = useasm;
    for (long i = 0; i < N; i++) perm[i] = (int)i;
    int *src = perm, *dst = ptmp;
    for (long w = 1; w < N; w *= 2) {
        for (long lo = 0; lo < N; lo += 2 * w) {
            long mid = lo + w < N ? lo + w : N, hi = lo + 2 * w < N ? lo + 2 * w : N, i = lo, j = mid, k = lo;
            while (i < mid && j < hi) { int x = src[i], y = src[j]; if (lessv(y, x)) dst[k++] = src[j++]; else dst[k++] = src[i++]; }
            while (i < mid) dst[k++] = src[i++]; while (j < hi) dst[k++] = src[j++]; }
        int *t = src; src = dst; dst = t; }
    if (src != perm) memcpy(perm, src, N * sizeof(int));
}

/* every operation, for one set (SFX) and one kernel choice per operation */
#define OPS(SFX, L) \
static void mul_##SFX(int k) { uint64_t r[2 * LC]; for (long i = 0; i < N; i++) { mulm_##SFX(r, HA + i * LC, HB + i * LC, k); memcpy(O + i * LC, r, 8 * LC); SO[i] = SHA[i] ^ SHB[i]; } } \
static void add_##SFX(uint64_t *r, unsigned char *sr, const uint64_t *a, const unsigned char *sa, const uint64_t *b, const unsigned char *sb, int k) { \
    for (long i = 0; i < N; i++) sadd_##SFX(r + i * LC, sr + i, a + i * LC, sa[i], b + i * LC, sb[i], k); } \
static void dot_##SFX(int k) { uint64_t r[2 * LC]; memset(SUM, 0, 8 * LC); SSUM = 0; \
    for (long i = 0; i < N; i++) { mulm_##SFX(r, HA + i * LC, HB + i * LC, k); sadd_##SFX(SUM, &SSUM, SUM, SSUM, r, SHA[i] ^ SHB[i], k); } } \
static void div_##SFX(uint64_t *q, unsigned char *sq, const uint64_t *e, const unsigned char *se, uint64_t d, int k) { uint64_t inv = binv64(d >> __builtin_ctzll(d)); \
    for (long i = 0; i < N; i++) { divx_##SFX(q + i * LC, e + i * LC, d, inv, k); sq[i] = se[i]; } } \
static void widen_##SFX(void) { long h = (L) / 2; for (long i = 0; i < N; i++) { memcpy(O + i * LC, HA + i * LC, 8 * h); memset(O + i * LC + h, 0, 8 * ((L) - h)); SO[i] = SHA[i]; } } \
static void steps_##SFX(long k0, long kn, int k) {        /* chain steps k0 .. kn-1 on X; CS holds 1000^(k+1) per step */ \
    uint64_t t[LC]; for (long s = k0; s < kn; s++) { const uint64_t *cs = CS + s * LC; \
        for (long i = 0; i < N; i++) step1_##SFX(X + i * LC, SX + i, CB[i] < 0 ? (uint64_t)0 - (uint64_t)CB[i] : (uint64_t)CB[i], CB[i] < 0, \
                                                    cs, CC[i] < 0 ? (uint64_t)0 - (uint64_t)CC[i] : (uint64_t)CC[i], CC[i] < 0, t, k); } }
OPS(s, LC)
OPS(l, LR)
OPS(sn, LC)
OPS(ln, LR)

int main(int argc, char **argv) {
    if (argc < 8) { fprintf(stderr, "usage: p1_thresh SET KERNEL OP N REPS INDIR OUTFILE [K]\n"); return 2; }
    SMALL = is(argv[1], "small"); const char *kern = argv[2], *op = argv[3]; N = atol(argv[4]); int reps = atoi(argv[5]);
    const char *dir = argv[6]; K = argc > 8 ? atol(argv[8]) : 0;
    LR = atol(getenv("P1_THRESH_L") ? getenv("P1_THRESH_L") : "16");         /* opaque to the compiler */
    if (LR != LC) { fprintf(stderr, "length must be %d\n", LC); return 2; }
    /* kernel per operation: one name for all, or a comma list op=kernel (for pipe) */
    int km[16], kn[16]; const char *opn[] = {"mul", "add", "dot", "div1", "div2", "sort", "scan", "widen", "step"};
    for (int i = 0; i < 9; i++) { km[i] = is(kern, "asm"); kn[i] = is(kern, "cn"); }
    if (strchr(kern, '=')) { char buf[256]; snprintf(buf, sizeof buf, "%s", kern);
        for (char *tok = strtok(buf, ","); tok; tok = strtok(NULL, ",")) { char *eq = strchr(tok, '='); *eq = 0;
            for (int i = 0; i < 9; i++) if (is(tok, opn[i])) { km[i] = is(eq + 1, "asm"); kn[i] = is(eq + 1, "cn"); } } }
    size_t vb = 8 * (size_t)N * LC;
    A = loadf(dir, "a.mag", vb); B = loadf(dir, "b.mag", vb); E1 = loadf(dir, "e1.mag", vb); E2 = loadf(dir, "e2.mag", vb);
    HA = calloc(N, 8 * LC); HB = calloc(N, 8 * LC);          /* 512-bit inputs, stored at stride LC with the top half zero */
    { uint64_t *ha = loadf(dir, "ha.mag", vb / 2), *hb = loadf(dir, "hb.mag", vb / 2);
      for (long i = 0; i < N; i++) { memcpy(HA + i * LC, ha + i * LC / 2, 4 * LC); memcpy(HB + i * LC, hb + i * LC / 2, 4 * LC); } free(ha); free(hb); }
    SA = loadf(dir, "a.sgn", N); SB = loadf(dir, "b.sgn", N); S1 = loadf(dir, "e1.sgn", N); S2 = loadf(dir, "e2.sgn", N);
    SHA = loadf(dir, "ha.sgn", N); SHB = loadf(dir, "hb.sgn", N);
    O = calloc(N, 8 * LC); T2 = calloc(N, 8 * LC); T3 = calloc(N, 8 * LC); V = calloc(N, 8 * LC); SUM = calloc(1, 8 * LC + 8);
    SO = calloc(N, 1); ST2 = calloc(N, 1); ST3 = calloc(N, 1); SV = calloc(N, 1);
    perm = malloc(sizeof(int) * N); ptmp = malloc(sizeof(int) * N);
    int step = is(op, "step"), pipe = is(op, "pipe");
    uint64_t *X0 = NULL; unsigned char *SX0 = NULL;
    if (step || pipe) {
        CB = loadf(dir, "cb.bin", 8 * N); CC = loadf(dir, "cc.bin", 8 * N);
        X0 = loadf(dir, "x0.mag", vb); SX0 = loadf(dir, "x0.sgn", N); X = malloc(vb); SX = malloc(N);
        long ks = step ? K : 1; CS = calloc(ks + 1, 8 * LC);
        mpz_t p; mpz_init(p); mpz_ui_pow_ui(p, 1000, (unsigned long)atol(getenv("P1_THRESH_K0") ? getenv("P1_THRESH_K0") : "0"));
        for (long s = 0; s < ks; s++) { mpz_mul_ui(p, p, 1000); size_t c; mpz_export(CS + s * LC, &c, -1, 8, 0, 0, p); }
        mpz_clear(p);
    }
    /* INNER: repeat the operation within each timed sample, so short operations on small n are timed reliably
       (not for step and pipe, which change their own input) */
    long inner = (step || pipe) ? 1 : atol(getenv("P1_THRESH_INNER") ? getenv("P1_THRESH_INNER") : "1");
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        if (step || pipe) { memcpy(X, X0, vb); memcpy(SX, SX0, N); }        /* reset the chain state (untimed) */
        double t0 = now();
        for (long rep = 0; rep < inner; rep++) {
#define PICK(F, S, i, ...) do { if (SMALL) { if (kn[i]) F##_s##n(__VA_ARGS__); else F##_s(__VA_ARGS__); } \
                              else { if (kn[i]) F##_l##n(__VA_ARGS__); else F##_l(__VA_ARGS__); } } while (0)
#define SC(i, v) (SMALL ? (kn[i] ? scanbits_sn(v) : scanbits_s(v)) : (kn[i] ? scanbits_ln(v) : scanbits_l(v)))
        if (is(op, "mul")) PICK(mul, 0, 0, km[0]);
        else if (is(op, "add")) PICK(add, 0, 1, O, SO, A, SA, B, SB, km[1]);
        else if (is(op, "dot")) PICK(dot, 0, 2, km[2]);
        else if (is(op, "div1")) PICK(div, 0, 3, O, SO, E1, S1, 999, km[3]);
        else if (is(op, "div2")) PICK(div, 0, 4, O, SO, E2, S2, 1000, km[4]);
        else if (is(op, "sort")) { CMPN = kn[5]; sortv(A, SA, km[5]); }
        else if (is(op, "scan")) SCAN = SC(6, A);
        else if (is(op, "widen")) PICK(widen, 0, 7);
        else if (step) PICK(steps, 0, 8, 0, K, km[8]);
        else { PICK(mul, 0, 0, km[0]); PICK(add, 0, 1, T2, ST2, O, SO, A, SA, km[1]); PICK(div, 0, 3, T3, ST3, E1, S1, 999, km[3]);
               PICK(add, 0, 1, V, SV, T2, ST2, T3, ST3, km[1]); PICK(div, 0, 4, T3, ST3, E2, S2, 1000, km[4]); PICK(add, 0, 1, T2, ST2, V, SV, T3, ST3, km[1]);
               PICK(add, 0, 1, V, SV, T2, ST2, X, SX, km[1]); PICK(steps, 0, 8, 0, 1, km[8]); PICK(add, 0, 1, V, SV, V, SV, X, SX, km[1]);
               CMPN = kn[5]; sortv(V, SV, km[5]); SCAN = SC(6, V); }
        }
        double dt = (now() - t0) / inner; if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g\n", t[reps / 2], t[0]);
    FILE *f = fopen(argv[7], "w"); mpz_t z; mpz_init(z);
#define VEC(v, s) for (long i = 0; i < N; i++) { mpz_import(z, LC, -1, 8, 0, 0, (v) + i * LC); if ((s)[i]) mpz_neg(z, z); gmp_fprintf(f, "%Zd\n", z); }
    if (is(op, "dot")) { mpz_import(z, LC, -1, 8, 0, 0, SUM); if (SSUM) mpz_neg(z, z); gmp_fprintf(f, "%Zd\n", z); }
    else if (is(op, "sort")) for (long i = 0; i < N; i++) fprintf(f, "%d\n", perm[i]);
    else if (is(op, "scan")) fprintf(f, "%d\n", SCAN);
    else if (step) { VEC(X, SX) }
    else if (pipe) { VEC(V, SV) fprintf(f, "%d\n", SCAN); for (long i = 0; i < N; i++) fprintf(f, "%d\n", perm[i]); }
    else { VEC(O, SO) }
    fclose(f);
    return 0;
}
