/* Deferred reduction for elementwise chains on fractions with unrelated denominators: x <- x*b + c, K times.

   usage: hensel METHOD N K REPS INDIR OUTFILE
     METHOD  mpq     GMP rationals, reduced after every operation (the baseline)
             pairs   a 128-bit numerator and denominator per element, no gcd until the end; an element whose
                     arithmetic would overflow is redone with GMP (counted)
             hensel  quote notation's representation: x = 2^e * r, r a residue modulo 2^128 standing for P/Q with
                     Q odd. Adding and multiplying are 128-bit integer operations; P/Q is recovered at the end by
                     rational reconstruction. Per-element bounds on |P| and Q are carried along; if they do not
                     guarantee a unique reconstruction (2|P|Q < 2^128) the element is redone with GMP (counted).
   INDIR holds a.bin, b.bin, c.bin (int64 numerators) and a_q.bin, b_q.bin, c_q.bin (int64 denominators >= 1).
   Prints "median_seconds min_seconds fallbacks"; writes each result as p/q to OUTFILE. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <gmp.h>

typedef unsigned __int128 u128;
typedef __int128 i128;

static long N, K;
static int64_t *A, *B, *C, *AQ, *BQ, *CQ;

static int64_t *load(const char *dir, const char *name) {
    char path[4096];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    int64_t *x = malloc(sizeof(int64_t) * N);
    if (fread(x, sizeof(int64_t), N, f) != (size_t)N) { fprintf(stderr, "short read\n"); exit(2); }
    fclose(f);
    return x;
}
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }

/* ---------------------------------------------------------------- GMP (also the fallback) */
static mpq_t *qa, *qb, *qc, *qx, qt;
static void gmp_setup(void) {
    qa = malloc(sizeof(mpq_t) * N); qb = malloc(sizeof(mpq_t) * N); qc = malloc(sizeof(mpq_t) * N); qx = malloc(sizeof(mpq_t) * N);
    for (long i = 0; i < N; i++) {
        mpq_init(qa[i]); mpq_init(qb[i]); mpq_init(qc[i]); mpq_init(qx[i]);
        mpq_set_si(qa[i], A[i], AQ[i]); mpq_canonicalize(qa[i]);
        mpq_set_si(qb[i], B[i], BQ[i]); mpq_canonicalize(qb[i]);
        mpq_set_si(qc[i], C[i], CQ[i]); mpq_canonicalize(qc[i]);
    }
    mpq_init(qt);
}
static void gmp_load(long i) {
    mpq_set_si(qa[i], A[i], AQ[i]); mpq_canonicalize(qa[i]);
    mpq_set_si(qb[i], B[i], BQ[i]); mpq_canonicalize(qb[i]);
    mpq_set_si(qc[i], C[i], CQ[i]); mpq_canonicalize(qc[i]);
}
static void gmp_one(long i) {
    gmp_load(i);
    mpq_set(qx[i], qa[i]);
    for (long k = 0; k < K; k++) { mpq_mul(qx[i], qx[i], qb[i]); mpq_add(qx[i], qx[i], qc[i]); }
}

/* ---------------------------------------------------------------- pairs: unreduced 128-bit fractions */
static i128 *pn; static u128 *pd; static char *pfall;
static int pair_one(long i) {
    i128 n = A[i]; u128 d = (u128)AQ[i];
    for (long k = 0; k < K; k++) {
        i128 t; u128 dd; i128 u, v;
        if (__builtin_mul_overflow(n, (i128)B[i], &t)) return 0;          /* n*b / (d*bq) */
        if (__builtin_mul_overflow(d, (u128)BQ[i], &dd)) return 0;
        if (__builtin_mul_overflow(t, (i128)CQ[i], &u)) return 0;          /* + c / cq     */
        if (dd > (u128)(((u128)1 << 127) - 1)) return 0;
        if (__builtin_mul_overflow((i128)C[i], (i128)dd, &v)) return 0;
        if (__builtin_add_overflow(u, v, &n)) return 0;
        if (__builtin_mul_overflow(dd, (u128)CQ[i], &d)) return 0;
    }
    pn[i] = n; pd[i] = d;
    return 1;
}
static u128 gcd128(u128 a, u128 b) { while (b) { u128 t = a % b; a = b; b = t; } return a; }

/* ---------------------------------------------------------------- hensel: residues modulo 2^128 */
static u128 inv128(u128 q) {      /* inverse of odd q modulo 2^128, by Newton's iteration */
    uint64_t q0 = (uint64_t)q, y = q0;                       /* correct to 3 bits */
    for (int i = 0; i < 5; i++) y *= 2 - q0 * y;             /* 96 bits > 64: correct modulo 2^64 */
    u128 x = y;
    x *= 2 - q * x;                                          /* doubles the correct bits to 128 */
    return x;
}
typedef struct { u128 r; int e; int bp, bq; } hval;   /* value 2^e * P/Q with |P| < 2^bp and Q < 2^bq (exact bounds) */
static hval hmake(int64_t p, int64_t q) {
    hval h; int t = __builtin_ctzll((uint64_t)q); uint64_t qo = (uint64_t)q >> t;
    h.e = -t;
    h.r = (u128)(i128)p * inv128((u128)qo);
    uint64_t ap = p < 0 ? (uint64_t)0 - (uint64_t)p : (uint64_t)p;      /* |p|, also for INT64_MIN */
    h.bp = ap ? 64 - __builtin_clzll(ap) : 0;                            /* |P| < 2^bits(|p|) */
    h.bq = 64 - __builtin_clzll(qo);                                    /* Q < 2^bits(Q) */
    return h;
}
static hval hmul(hval x, hval y) {
    hval h = { x.r * y.r, x.e + y.e, x.bp + y.bp, x.bq + y.bq };
    return h;
}
static hval hadd(hval x, hval y) {   /* align to the smaller exponent: multiply the other residue by 2^d */
    if (x.e > y.e) { hval t = x; x = y; y = t; }
    int d = y.e - x.e;
    hval h;
    h.e = x.e;
    h.r = x.r + (d < 128 ? (y.r << d) : 0);
    /* P/Q = Px/Qx + 2^d Py/Qy = (Px Qy + 2^d Py Qx) / (Qx Qy) */
    int t1 = x.bp + y.bq, t2 = d + y.bp + x.bq;                      /* |a + b| < 2 max(|a|, |b|) */
    h.bp = (t1 > t2 ? t1 : t2) + 1;
    h.bq = x.bq + y.bq;
    if (d >= 128) h.bp = 1 << 20;    /* the residue lost the term: force the fallback */
    return h;
}
/* Rational reconstruction (Wang): the unique P/Q with |P| < 2^ap, 0 < Q < 2^aq, ap + aq + 1 <= 128,
   P = r Q modulo 2^128, if one exists. Half-extended Euclid on (2^128, r). */
static int reconstruct(u128 r, int ap, int aq, i128 *P, u128 *Q) {
    u128 A0 = (u128)1 << ap, B0 = (u128)1 << aq;
    /* first step by hand, since 2^128 does not fit: 2^128 = q0 r + rem */
    if (r == 0) { *P = 0; *Q = 1; return 1; }
    if (r < A0) { *P = (i128)r; *Q = 1; return 1; }
    u128 q0 = (~(u128)0) / r, rem = (~(u128)0) - q0 * r + 1;
    if (rem == r) { q0 += 1; rem = 0; }
    /* rows: (remainder, cofactor of r) ; start (2^128, 0), (r, 1) -> (rem, -q0) */
    u128 r0 = r, r1 = rem;
    i128 t0 = 1, t1 = -(i128)q0;     /* r_i = t_i * r  (mod 2^128) */
    if (q0 >> 127) return 0;
    while (r1 >= A0) {
        if (r1 == 0) return 0;
        u128 q = r0 / r1, nr = r0 - q * r1;
        i128 nt;
        if (__builtin_mul_overflow((i128)q, t1, &nt)) return 0;
        if (__builtin_sub_overflow(t0, nt, &nt)) return 0;
        r0 = r1; r1 = nr; t0 = t1; t1 = nt;
    }
    if (t1 == 0) return 0;
    u128 qq = t1 < 0 ? (u128)(-t1) : (u128)t1;
    if (qq >= B0) return 0;
    *P = t1 < 0 ? -(i128)r1 : (i128)r1;
    *Q = qq;
    return 1;
}
static i128 *hn; static u128 *hd; static int *he;
static int hensel_one(long i) {
    hval x = hmake(A[i], AQ[i]), b = hmake(B[i], BQ[i]), c = hmake(C[i], CQ[i]);
    for (long k = 0; k < K; k++) x = hadd(hmul(x, b), c);
    /* split the 128 bits between numerator and denominator according to the bounds */
    int aq = x.bq, ap = x.bp;
    if (aq < 1) aq = 1;
    if (ap < 1) ap = 1;
    if (ap + aq + 1 > 128) return 0;
    i128 P; u128 Q;
    if (!reconstruct(x.r, ap, aq, &P, &Q)) return 0;
    hn[i] = P; hd[i] = Q; he[i] = x.e;
    return 1;
}

static void printu128(FILE *f, u128 u) {
    char buf[64]; int k = 63; buf[k] = 0;
    do { buf[--k] = '0' + (int)(u % 10); u /= 10; } while (u);
    fputs(buf + k, f);
}
static void print128(FILE *f, i128 v) {
    char buf[64]; int k = 63; buf[k] = 0; int neg = v < 0;
    u128 u = neg ? -(u128)v : (u128)v;
    do { buf[--k] = '0' + (int)(u % 10); u /= 10; } while (u);
    if (neg) buf[--k] = '-';
    fputs(buf + k, f);
}

int main(int argc, char **argv) {
    if (argc != 7) { fprintf(stderr, "usage: hensel METHOD N K REPS INDIR OUTFILE\n"); return 2; }
    const char *m = argv[1];
    N = atol(argv[2]); K = atol(argv[3]); int reps = atoi(argv[4]);
    A = load(argv[5], "a.bin"); B = load(argv[5], "b.bin"); C = load(argv[5], "c.bin");
    AQ = load(argv[5], "a_q.bin"); BQ = load(argv[5], "b_q.bin"); CQ = load(argv[5], "c_q.bin");
    gmp_setup();
    int kind = !strcmp(m, "mpq") ? 0 : !strcmp(m, "pairs") ? 1 : 2;
    pn = malloc(sizeof(i128) * N); pd = malloc(sizeof(u128) * N); pfall = calloc(N, 1);
    hn = malloc(sizeof(i128) * N); hd = malloc(sizeof(u128) * N); he = malloc(sizeof(int) * N);
    double *t = malloc(sizeof(double) * reps);
    long falls = 0;
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        falls = 0;
        for (long i = 0; i < N; i++) {
            if (kind == 0) gmp_one(i);
            else {
                int ok = kind == 1 ? pair_one(i) : hensel_one(i);
                pfall[i] = !ok;
                if (!ok) { gmp_one(i); falls++; }
            }
        }
        if (kind == 1)             /* the deferred gcd */
            for (long i = 0; i < N; i++) if (!pfall[i]) {
                u128 g = gcd128(pn[i] < 0 ? -(u128)pn[i] : (u128)pn[i], pd[i]);
                if (g > 1) { pn[i] /= (i128)g; pd[i] /= g; }
            }
        if (kind == 2)             /* put the powers of two back and reduce */
            for (long i = 0; i < N; i++) if (!pfall[i]) {
                i128 P = hn[i]; u128 Q = hd[i]; int e = he[i];
                if (P == 0) { Q = 1; e = 0; }
                while (e < 0 && P % 2 == 0 && P) { P /= 2; e++; }
                if (e > 0) { if (e > 120 || __builtin_mul_overflow(P, (i128)1 << e, &P)) { pfall[i] = 1; gmp_one(i); falls++; continue; } e = 0; }
                u128 g = gcd128(P < 0 ? -(u128)P : (u128)P, Q);
                if (g > 1) { P /= (i128)g; Q /= g; }
                if (e < 0) { if (-e > 120 || __builtin_mul_overflow(Q, (u128)1 << (-e), &Q)) { pfall[i] = 1; gmp_one(i); falls++; continue; } }
                hn[i] = P; hd[i] = Q;
            }
        double dt = now() - t0;
        if (r >= 0) t[r] = dt;
    }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %ld\n", t[reps / 2], t[0], falls);
    FILE *f = fopen(argv[6], "w");
    for (long i = 0; i < N; i++) {
        if (kind == 0 || pfall[i]) gmp_fprintf(f, "%Qd\n", qx[i]);
        else {
            i128 P = kind == 1 ? pn[i] : hn[i]; u128 Q = kind == 1 ? pd[i] : hd[i];
            print128(f, P);
            if (Q != 1) { fputc('/', f); printu128(f, Q); }
            fputc('\n', f);
        }
    }
    fclose(f);
    return 0;
}
