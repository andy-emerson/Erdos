#define _POSIX_C_SOURCE 200809L
/* Phase one, storage granularity: the vector form (signed integer numerators over one denominator, the whole vector
   at one width) stored three ways between operations, compared with each other only. Compute is always on 64-bit
   limbs with GMP's mpn routines (the lr code of p1_arith.c); only how values sit in memory changes.

   usage: p1_storage VAR OP B N REPS INDIR OUTFILE [K DEN SLACK]
     VAR   pow2     magnitudes in the next power-of-two width >= B bits: 8 * 2^k bytes per value, aligned
           limb     ceil(B/64) limbs per value, aligned
           byte     ceil(B/8) bytes per value, packed contiguously (unaligned), computed in place
           slack    (chain only) byte with geometric slack: when the bound outgrows the stored bytes, the vector is
                    re-stored at ceil(needed bytes * SLACK / 100) (SLACK default 150)
           lslack   (chain only) limb with geometric slack: re-stored at ceil(needed limbs * SLACK / 100) limbs
           bytecopy byte storage through a copy-in/copy-out limb buffer (the first version, kept for comparison)
     Signs are a separate byte array (one byte per element, 1 = negative), the same in every variant.
     Kernels compute on L = ceil(B/64) limbs, the limbs the values need (pow2 never computes on its padding).
     Aligned (pow2, limb): a value is a limb pointer into storage; results are computed in place with mpn. Invariant:
     the bytes of a value beyond its L limbs are zero (set when storage is made or re-stored).
     Packed in place (byte, slack): a value of S = ceil(B/8) bytes is F = floor(S/8) full limbs, used where they lie
     through unaligned limb pointers (x86 loads and stores accept them; the same mpn routines run on them), plus, when
     S is not a multiple of 8, one partial top limb read as an unaligned 8-byte load masked to S mod 8 bytes and
     written by read-modify-write of those 8 bytes (the neighbour's bytes are written back unchanged; every packed
     vector has 8 bytes of tail padding so the last element's access stays inside it). Signed add: compare from the
     top limb down in place, then mpn_add_n / mpn_sub_n on the full limbs straight into the result plus the partial
     limb with the carry. Product: mpn_mul_n on the full limbs plus the partial limb's rows (mpn_addmul_1), into a
     buffer then one copy of S bytes (the aligned variants also multiply into a buffer and copy L limbs).
     Copy-through (bytecopy): a load copies S bytes into a zero-padded limb buffer, a store copies them back.
     Every variant multiplies one-limb operands (floor(B/2) <= 64 bits) inline as one 64x64 -> 128 product; wider
     operands go to mpn_mul_n. Special cases exist in all variants or none.
     Timing: one warm-up run; with P1S_MIN=seconds in the environment one more run calibrates an inner count so that
     each timed sample (inner runs back to back; for the chain each run is timed alone, without freeing the previous
     result) lasts at least that long; times are per run. The chain also times its re-stores (inside the C).
     After the runs every stored vector is checked: the bytes of each value past ceil(bits/8) up to the stride, and the
     8 tail bytes, must be zero (exit 4 otherwise). Test builds: -DP1S_REVERSE=1 processes elements in reverse order
     (so a write that spills into the next element is not hidden by that element's own later write) and -DP1S_BUG=1
     plants such a spill (one byte of the next element) to show that the tests catch it.
     OP    add    a + b; a, b have B - 1 bits, all three vectors stored at B
           mul    ha * hb; ha, hb have floor(B/2) bits stored at floor(B/2), the product stored at B
           sort   order s (stable merge sort of indices); s has B bits stored at B
           scan   largest bit length of |s|
           widen  ha re-stored from floor(B/2) to B
           chain  x <- x b + c DEN^(k+1), K steps from int64 x (B ignored): before each step the proven bound
                  new = max(bits x + bits b, bits c + bits DEN^(k+1)) + 1 (DEN^(k+1) exact in GMP) gives the magnitude
                  bits; when the stored size for that many bits exceeds the current stride the whole vector is
                  re-stored (malloc of the new vector, copy of every old stride, zero fill, free of the old one, all
                  timed). pow2 goes straight to the power of two that fits (doubling, or more if needed).
   INDIR holds X.mag (little-endian limbs, ceil(bits/64) per value) and X.sgn (N bytes) for X in a b s ha hb;
   for chain x0.bin cb.bin cc.bin (int64). Conversion into storage is before timing and out of it after. Timing: one
   warm-up run, then the median of REPS. Prints "median min stride stride_in" (bytes per stored value of the result
   and of the input) and the inner count; for chain "median min bits stride restores alloc_bytes copied_bytes
   peak_bytes restore_seconds inner" (one run; restore_seconds of the median sample).
   OUTFILE: one hexadecimal integer per element, the permutation (decimal) for sort, one number for scan. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <gmp.h>

typedef mp_limb_t limb;
static long N, K, DEN, SLACK = 150; static int B, BH, L, LH, OP, VAR;
enum { ADD, MUL, SORT, SCAN, WIDEN, CHAIN };
enum { POW2, LIMB, BYTE, BSLACK, BCOPY, LSLACK };
static int PK;                                   /* 0 aligned, 1 copy-through, 2 packed in place */
#ifndef P1S_REVERSE
#define P1S_REVERSE 0
#endif
#define ORD(i0) (P1S_REVERSE ? N - 1 - (i0) : (i0))
#ifdef P1S_BUG
#define CLOB(s, i) do { if ((i) + 1 < N) (s)->p[((size_t)(i) + 1) * (s)->stride] ^= 0xA5; } while (0)
#else
#define CLOB(s, i) ((void)0)
#endif
#define AI static inline __attribute__((always_inline))

static void *loadf(const char *dir, const char *name, size_t bytes) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE *f = fopen(path, "rb"); if (!f) { perror(path); exit(2); }
    void *x = malloc(bytes ? bytes : 1); if (fread(x, 1, bytes, f) != bytes) { fprintf(stderr, "short read %s\n", name); exit(2); }
    fclose(f); return x;
}
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + 1e-9 * t.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return x < y ? -1 : x > y; }
static int is(const char *a, const char *b) { return !strcmp(a, b); }
static uint64_t uabs64(int64_t v) { return v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v; }
static int bits64(int64_t v) { uint64_t u = uabs64(v); return u ? 64 - __builtin_clzll(u) : 0; }
static long limbs(int bits) { return bits < 1 ? 1 : (bits + 63) / 64; }

/* ---------------------------------------------------------------- storage */
typedef struct { unsigned char *p; size_t stride, vb; long F; uint64_t m; } st_t;
/* vb: bytes a packed value takes; F full limbs in it, m the mask of the partial top limb (0: none) */
static void geom(st_t *s, int bits) { s->vb = bits < 1 ? 1 : (size_t)(bits + 7) / 8; s->F = (long)(s->vb / 8);
    int r = (int)(s->vb % 8); s->m = r ? ~(uint64_t)0 >> (64 - 8 * r) : 0; }
static size_t stride_for(int var, int bits, size_t cur) {             /* stored bytes per value for that many bits */
    size_t s;
    switch (var) {
    case POW2: { size_t q = 1; while (q < (size_t)limbs(bits)) q *= 2; s = 8 * q; break; }
    case LIMB: s = 8 * (size_t)limbs(bits); break;
    case BYTE: case BCOPY: s = bits < 1 ? 1 : (size_t)(bits + 7) / 8; break;
    case LSLACK: { size_t need = (size_t)limbs(bits);
        if (8 * need <= cur) return cur; s = 8 * ((need * (size_t)SLACK + 99) / 100); if (s < 8 * need) s = 8 * need; break; }
    default: { size_t need = bits < 1 ? 1 : (size_t)(bits + 7) / 8;
        if (need <= cur) return cur; s = (need * (size_t)SLACK + 99) / 100; if (s < need) s = need; }
    }
    return s > cur ? s : cur;
}
/* the three primitives every kernel uses; pk is a compile-time constant in each instantiation */
AI const limb *ld(int pk, const st_t *s, long i, limb *buf, long l) {
    const unsigned char *q = s->p + (size_t)i * s->stride;
    if (!pk) return (const limb *)q;
    buf[l - 1] = 0; memcpy(buf, q, s->vb); return buf;               /* 8(l-1) < vb <= 8l */
}
AI limb *dst(int pk, const st_t *s, long i, limb *buf) { return pk ? buf : (limb *)(s->p + (size_t)i * s->stride); }
AI void st(int pk, const st_t *s, long i, const limb *buf) { if (pk) memcpy(s->p + (size_t)i * s->stride, buf, s->vb); }
AI void stfrom(int pk, const st_t *s, long i, const limb *src, long l) {  /* store from a separate limb buffer */
    memcpy(s->p + (size_t)i * s->stride, src, pk ? s->vb : 8 * (size_t)l);
}
static void mk(st_t *s, int bits) { s->stride = stride_for(VAR, bits, 0); geom(s, bits);
    s->p = calloc((size_t)N * s->stride + 8, 1); }                    /* 8 bytes of tail padding for every variant */
static void fill(st_t *s, const limb *mag, long l) {
    for (long i = 0; i < N; i++) memcpy(s->p + (size_t)i * s->stride, mag + i * l, PK ? s->vb : 8 * (size_t)l);
}

/* signed add of magnitudes (p1_arith's lr_sadd) */
AI void sadd(limb *r, unsigned char *sr, const limb *a, int sa, const limb *b, int sb, long l) {
    if (sa == sb) { mpn_add_n(r, a, b, l); *sr = (unsigned char)sa; return; }
    int c = mpn_cmp(a, b, l);
    if (c >= 0) { mpn_sub_n(r, a, b, l); *sr = c == 0 ? 0 : (unsigned char)sa; } else { mpn_sub_n(r, b, a, l); *sr = (unsigned char)sb; }
}

/* packed in place: unaligned limb access */
#define UL(q) ((limb *)(q))
AI uint64_t ld8(const unsigned char *p) { uint64_t v; memcpy(&v, p, 8); return v; }
AI void st8(unsigned char *p, uint64_t v) { memcpy(p, &v, 8); }
AI uint64_t ldp(const unsigned char *q, long F, uint64_t m) { return m ? ld8(q + 8 * F) & m : 0; }   /* partial top limb */
AI void stp(unsigned char *q, long F, uint64_t m, uint64_t v) { if (m) { unsigned char *p = q + 8 * F; st8(p, (ld8(p) & ~m) | v); } }
AI int pcmp(const limb *a, uint64_t ap, const limb *b, uint64_t bp, long F) {   /* from the top, early exit */
    if (ap != bp) return ap > bp ? 1 : -1; return F ? mpn_cmp(a, b, F) : 0; }
/* o = a + b with signs; a, b: F full limbs (any alignment) and partial top limbs ap, bp; o packed */
AI void psadd(unsigned char *o, unsigned char *so, const limb *a, uint64_t ap, int sa, const limb *b, uint64_t bp, int sb,
              long F, uint64_t m) {
    if (sa == sb) { limb c = F ? mpn_add_n(UL(o), a, b, F) : 0; stp(o, F, m, ap + bp + c); *so = (unsigned char)sa; return; }
    int c = pcmp(a, ap, b, bp, F);
    if (c >= 0) { limb br = F ? mpn_sub_n(UL(o), a, b, F) : 0; stp(o, F, m, ap - bp - br); *so = c == 0 ? 0 : (unsigned char)sa; }
    else { limb br = F ? mpn_sub_n(UL(o), b, a, F) : 0; stp(o, F, m, bp - ap - br); *so = (unsigned char)sb; }
}

/* stable bottom-up merge sort of the permutation (p1_arith's MSORT) */
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

/* ---------------------------------------------------------------- operations */
static st_t SA, SB, SS, SHA, SHB, SO;
static unsigned char *sga, *sgb, *sgs, *sgha, *sghb, *sgo;
static limb *b1, *b2, *b3, *tt; static int scanmax;

AI int less_k(int pk, int x, int y) {
    int sx = sgs[x], sy = sgs[y]; if (sx != sy) return sx > sy;
    int c = mpn_cmp(ld(pk, &SS, x, b1, L), ld(pk, &SS, y, b2, L), L); return sx ? c > 0 : c < 0;
}
#define LESS_AL(x, y) less_k(0, x, y)
#define LESS_PK(x, y) less_k(1, x, y)
AI int less_ip(int x, int y) {
    int sx = sgs[x], sy = sgs[y]; if (sx != sy) return sx > sy;
    const unsigned char *p = SS.p + (size_t)x * SS.stride, *q = SS.p + (size_t)y * SS.stride;
    int c = pcmp(UL(p), ldp(p, SS.F, SS.m), UL(q), ldp(q, SS.F, SS.m), SS.F); return sx ? c > 0 : c < 0;
}
MSORT(sort_al, LESS_AL) MSORT(sort_pk, LESS_PK) MSORT(sort_ip, less_ip)

static void run_ip(void) {      /* packed in place */
    switch (OP) {
    case ADD: { long F = SA.F; uint64_t m = SA.m;
        for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            const unsigned char *a = SA.p + (size_t)i * SA.stride, *b = SB.p + (size_t)i * SB.stride;
            psadd(SO.p + (size_t)i * SO.stride, sgo + i, UL(a), ldp(a, F, m), sga[i], UL(b), ldp(b, F, m), sgb[i], F, m);
            CLOB(&SO, i); }
        break; }
    case MUL: { long F = SHA.F; uint64_t m = SHA.m;  /* tt: max(2 LH, L) limbs; limbs past 2 LH stay zero */
        for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            const unsigned char *a = SHA.p + (size_t)i * SHA.stride, *b = SHB.p + (size_t)i * SHB.stride;
            if (LH == 1) {                         /* one-limb operands: inline, as in every variant */
                unsigned __int128 p = (unsigned __int128)(F ? ld8(a) : ldp(a, 0, m)) * (F ? ld8(b) : ldp(b, 0, m));
                tt[0] = (limb)p; tt[1] = (limb)(p >> 64);
            } else if (!m) mpn_mul_n(tt, UL(a), UL(b), F);
            else {
                uint64_t ap = ldp(a, F, m), bp = ldp(b, F, m); unsigned __int128 p = (unsigned __int128)ap * bp;
                if (F) {
                    mpn_mul_n(tt, UL(a), UL(b), F);
                    limb h0 = mpn_addmul_1(tt + F, UL(a), F, bp), c = mpn_addmul_1(tt + F, UL(b), F, ap);
                    unsigned __int128 t = (unsigned __int128)h0 + c + (uint64_t)p;
                    tt[2 * F] = (limb)t; tt[2 * F + 1] = (limb)(t >> 64) + (limb)(p >> 64);
                } else { tt[0] = (limb)p; tt[1] = (limb)(p >> 64); }
            }
            memcpy(SO.p + (size_t)i * SO.stride, tt, SO.vb); sgo[i] = sgha[i] ^ sghb[i]; CLOB(&SO, i); }
        break; }
    case SORT: sort_ip(); break;
    case SCAN: { long F = SS.F; uint64_t m = SS.m; for (long j = 0; j < L; j++) b3[j] = 0;
        for (long i = 0; i < N; i++) { const unsigned char *x = SS.p + (size_t)i * SS.stride;
            for (long j = 0; j < F; j++) b3[j] |= ld8(x + 8 * j);
            if (m) b3[F] |= ld8(x + 8 * F) & m; }
        long j = L - 1; while (j >= 0 && !b3[j]) j--;
        scanmax = j < 0 ? 0 : (int)(64 * j + 64 - __builtin_clzll(b3[j])); break; }
    case WIDEN: for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            unsigned char *o = SO.p + (size_t)i * SO.stride;
            memcpy(o, SHA.p + (size_t)i * SHA.stride, SHA.vb); memset(o + SHA.vb, 0, SO.vb - SHA.vb); sgo[i] = sgha[i];
            CLOB(&SO, i); }
        break;
    }
}

AI void run_k(int pk) {
    switch (OP) {
    case ADD: for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            const limb *a = ld(pk, &SA, i, b1, L), *b = ld(pk, &SB, i, b2, L); limb *o = dst(pk, &SO, i, b3);
            sadd(o, sgo + i, a, sga[i], b, sgb[i], L); st(pk, &SO, i, o); CLOB(&SO, i); }
        break;
    case MUL:                       /* tt holds max(2 LH, L) limbs; limbs past 2 LH stay zero */
        for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            const limb *a = ld(pk, &SHA, i, b1, LH), *b = ld(pk, &SHB, i, b2, LH);
            if (LH == 1) { unsigned __int128 p = (unsigned __int128)a[0] * b[0]; tt[0] = (limb)p; tt[1] = (limb)(p >> 64); }
            else mpn_mul_n(tt, a, b, LH);
            stfrom(pk, &SO, i, tt, L); sgo[i] = sgha[i] ^ sghb[i]; CLOB(&SO, i); }
        break;
    case SORT: if (pk) sort_pk(); else sort_al(); break;
    case SCAN: { for (long j = 0; j < L; j++) b3[j] = 0;
        for (long i = 0; i < N; i++) { const limb *x = ld(pk, &SS, i, b1, L); for (long j = 0; j < L; j++) b3[j] |= x[j]; }
        long j = L - 1; while (j >= 0 && !b3[j]) j--;
        scanmax = j < 0 ? 0 : (int)(64 * j + 64 - __builtin_clzll(b3[j])); break; }
    case WIDEN: for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            limb *o = dst(pk, &SO, i, b3); const limb *x = ld(pk, &SHA, i, o, LH);   /* packed: loads straight into o */
            if (x != o) memcpy(o, x, 8 * LH);
            if (L > LH) memset(o + LH, 0, 8 * (L - LH));
            st(pk, &SO, i, o); sgo[i] = sgha[i]; CLOB(&SO, i); }
        break;
    }
}
static void run_al(void) { run_k(0); }
static void run_pk(void) { run_k(1); }

/* ---------------------------------------------------------------- chain */
static int64_t *X0, *CB, *CC;
static int BBITS, BCBITS; static mpz_t PW;
static void chain_bounds(void) {
    BBITS = 0; BCBITS = 0;
    for (long i = 0; i < N; i++) { int t = bits64(CB[i]); if (t > BBITS) BBITS = t; t = bits64(CC[i]); if (t > BCBITS) BCBITS = t; }
    mpz_set_ui(PW, 1);
}
static int chain_next(int bx) {           /* magnitude bits after one more step (the sign is stored apart) */
    mpz_mul_ui(PW, PW, (unsigned long)DEN);
    int bcs = (int)mpz_sizeinbase(PW, 2);
    return (bx + BBITS > BCBITS + bcs ? bx + BBITS : BCBITS + bcs) + 1;
}
static int bx0(void) { int b = 0; for (long i = 0; i < N; i++) { int t = bits64(X0[i]); if (t > b) b = t; } return b < 1 ? 1 : b; }

static st_t X; static unsigned char *xs; static limb *csb, *csl, *t1, *t2, *xb; static int final_bits;
static unsigned long long restores, alloc_b, copy_b, peak_b; static double rest_t;
static void restore(size_t ns) {          /* the whole vector moves to stride ns, the new bytes zero */
    double t0 = now();
    unsigned char *q = malloc((size_t)N * ns + 8), *p = X.p; size_t os = X.stride; memset(q + (size_t)N * ns, 0, 8);
    for (long i = 0; i < N; i++) { memcpy(q + (size_t)i * ns, p + (size_t)i * os, os); memset(q + (size_t)i * ns + os, 0, ns - os); }
    free(p); X.p = q; X.stride = ns;
    restores++; alloc_b += (unsigned long long)N * ns; copy_b += (unsigned long long)N * os;
    if ((unsigned long long)N * (os + ns) > peak_b) peak_b = (unsigned long long)N * (os + ns);
    rest_t += now() - t0;
}
AI void chain_k(int pk) {
    chain_bounds(); int nb = bx0();
    restores = 0; rest_t = 0; size_t s0 = stride_for(VAR, nb, 0);
    X.p = malloc((size_t)N * s0 + 8); X.stride = s0; geom(&X, nb); alloc_b = peak_b = (unsigned long long)N * s0; copy_b = 0;
    memset(X.p, 0, (size_t)N * s0 + 8);
    for (long i = 0; i < N; i++) { xb[0] = uabs64(X0[i]); stfrom(pk, &X, i, xb, 1); xs[i] = X0[i] < 0; }
    csb[0] = 1; long lcs = 1;
    for (long k = 0; k < K; k++) {
        nb = chain_next(nb);
        size_t ns = stride_for(VAR, nb, X.stride);
        if (ns != X.stride) restore(ns);
        long l = limbs(nb); geom(&X, nb);
        limb c = mpn_mul_1(csb, csb, lcs, (limb)DEN); if (c) csb[lcs++] = c;
        memcpy(csl, csb, 8 * lcs); if (l > lcs) memset(csl + lcs, 0, 8 * (l - lcs));
        if (pk == 2) {                     /* packed in place */
            long F = X.F; uint64_t m = X.m;
            for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
                unsigned char *q = X.p + (size_t)i * X.stride; uint64_t bm = uabs64(CB[i]);
                limb c1 = F ? mpn_mul_1(t1, UL(q), F, bm) : 0;
                if (m) t1[F] = ldp(q, F, m) * bm + c1;
                mpn_mul_1(t2, csl, l, uabs64(CC[i]));
                psadd(q, xs + i, t1, m ? t1[F] : 0, (xs[i] ^ (CB[i] < 0)) & 1, t2, m ? t2[F] : 0, CC[i] < 0, F, m);
                if (!ldp(q, F, m) && (!F || mpn_zero_p(UL(q), F))) xs[i] = 0;
                CLOB(&X, i);
            }
            continue;
        }
        for (long i0 = 0; i0 < N; i0++) { long i = ORD(i0);
            const limb *x = ld(pk, &X, i, xb, l);
            mpn_mul_1(t1, x, l, uabs64(CB[i])); mpn_mul_1(t2, csl, l, uabs64(CC[i]));
            limb *o = dst(pk, &X, i, xb);
            sadd(o, xs + i, t1, (xs[i] ^ (CB[i] < 0)) & 1, t2, CC[i] < 0, l);
            if (mpn_zero_p(o, l)) xs[i] = 0;
            st(pk, &X, i, o); CLOB(&X, i);
        }
    }
    final_bits = nb;
}
static void chain_al(void) { chain_k(0); }
static void chain_pk(void) { chain_k(1); }
static void chain_ip(void) { chain_k(2); }

/* ---------------------------------------------------------------- main */
static int pad_ok(const st_t *s) {        /* bytes past vb up to the stride, and the tail, are zero */
    if (!s->p) return 1;
    for (long i = 0; i < N; i++) { const unsigned char *q = s->p + (size_t)i * s->stride;
        for (size_t j = s->vb; j < s->stride; j++) if (q[j]) return 0; }
    for (int j = 0; j < 8; j++) if (s->p[(size_t)N * s->stride + j]) return 0;
    return 1;
}
static void put(FILE *f, mpz_t z, const st_t *s, long i, const unsigned char *sg, long l) {
    const limb *x = ld(PK != 0, s, i, b1, l);
    mpz_import(z, l, -1, 8, 0, 0, x); if (sg[i]) mpz_neg(z, z); gmp_fprintf(f, "%Zx\n", z);
}
int main(int argc, char **argv) {
    if (argc < 8) { fprintf(stderr, "usage: p1_storage VAR OP B N REPS INDIR OUTFILE [K DEN SLACK]\n"); return 2; }
    const char *var = argv[1], *op = argv[2]; B = atoi(argv[3]); N = atol(argv[4]); int reps = atoi(argv[5]);
    const char *dir = argv[6];
    const char *ops[] = {"add", "mul", "sort", "scan", "widen", "chain"}, *vars[] = {"pow2", "limb", "byte", "slack", "bytecopy", "lslack"};
    OP = -1; for (int i = 0; i < 6; i++) if (is(op, ops[i])) OP = i;
    VAR = -1; for (int i = 0; i < 6; i++) if (is(var, vars[i])) VAR = i;
    if (OP < 0 || VAR < 0 || reps < 1 || N < 1 || (OP != CHAIN && (B < 2 || VAR == BSLACK || VAR == LSLACK))) { fprintf(stderr, "bad arguments\n"); return 2; }
    int pk = PK = VAR == BCOPY ? 1 : VAR == BYTE || VAR == BSLACK ? 2 : 0;
    mpz_init(PW); mpz_t z; mpz_init(z);
    perm = malloc(sizeof(int) * N); ptmp = malloc(sizeof(int) * N);
    if (OP == CHAIN) {
        if (argc < 10) { fprintf(stderr, "chain needs K DEN\n"); return 2; }
        K = atol(argv[8]); DEN = atol(argv[9]); if (argc > 10) SLACK = atol(argv[10]);
        if (SLACK < 100) { fprintf(stderr, "slack below 100%%\n"); return 2; }
        X0 = loadf(dir, "x0.bin", 8 * N); CB = loadf(dir, "cb.bin", 8 * N); CC = loadf(dir, "cc.bin", 8 * N);
        chain_bounds(); int nb = bx0(); for (long k = 0; k < K; k++) nb = chain_next(nb);   /* sizes the buffers */
        long lm = limbs(nb) + 2;
        xs = malloc(N); csb = calloc(lm, 8); csl = calloc(lm, 8); t1 = calloc(lm, 8); t2 = calloc(lm, 8); xb = calloc(lm, 8);
    } else {
        L = (int)limbs(B); BH = B / 2; LH = (int)limbs(BH);
        long lm = (L > 2 * LH ? L : 2 * LH) + 2;
        b1 = calloc(lm, 8); b2 = calloc(lm, 8); b3 = calloc(lm, 8); tt = calloc(lm, 8);
        const char *nm[5] = {"a", "b", "s", "ha", "hb"}; st_t *ss[5] = {&SA, &SB, &SS, &SHA, &SHB};
        unsigned char **sg[5] = {&sga, &sgb, &sgs, &sgha, &sghb}; int bits[5] = {B, B, B, BH, BH};
        for (int a = 0; a < 5; a++) {
            long l = limbs(bits[a]); char f[16];
            snprintf(f, sizeof f, "%s.mag", nm[a]); limb *mag = loadf(dir, f, 8 * (size_t)N * l);
            snprintf(f, sizeof f, "%s.sgn", nm[a]); *sg[a] = loadf(dir, f, N);
            mk(ss[a], bits[a]); fill(ss[a], mag, l); free(mag);
        }
        mk(&SO, B); sgo = calloc(N, 1);
    }
    double *t = malloc(sizeof(double) * reps), *rt = malloc(sizeof(double) * reps), *t_s = malloc(sizeof(double) * reps);
    const char *me = getenv("P1S_MIN"); double tmin = me ? atof(me) : 0; long inner = 1;
    for (int r = -2; r < reps; r++) {          /* -2 warm-up, -1 calibration of the inner count */
        if (r == -1 && tmin <= 0) continue;
        long cnt = r < 0 ? 1 : inner; double dt = 0, drt = 0;
        if (OP == CHAIN) for (long c = 0; c < cnt; c++) {
            if (X.p) { free(X.p); X.p = NULL; }
            double t0 = now();
            if (pk == 2) chain_ip(); else if (pk) chain_pk(); else chain_al();
            dt += now() - t0; drt += rest_t;
        } else {
            double t0 = now();
            for (long c = 0; c < cnt; c++) { if (pk == 2) run_ip(); else if (pk) run_pk(); else run_al(); }
            dt = now() - t0;
        }
        if (r == -1) { inner = dt > 0 ? (long)(tmin / dt) + 1 : 1000000; if (inner < 1) inner = 1; }
        if (r >= 0) { t[r] = dt / inner; rt[r] = drt / inner; t_s[r] = t[r]; }
    }
    qsort(t_s, reps, sizeof(double), cmpd);
    double med = t_s[reps / 2], rmed = 0;
    for (int r = 0; r < reps; r++) if (t[r] == med) { rmed = rt[r]; break; }
    if (!pad_ok(&SA) || !pad_ok(&SB) || !pad_ok(&SS) || !pad_ok(&SHA) || !pad_ok(&SHB) || !pad_ok(&SO) || !pad_ok(&X)) {
        fprintf(stderr, "padding or neighbour bytes corrupted\n"); return 4; }
    FILE *f = fopen(argv[7], "w"); if (!f) { perror(argv[7]); return 2; }
    if (OP == CHAIN) {
        printf("%.9g %.9g %d %zu %llu %llu %llu %llu %.9g %ld\n", med, t_s[0], final_bits, X.stride, restores, alloc_b, copy_b, peak_b, rmed, inner);
        long l = limbs(final_bits); b1 = calloc(l + 2, 8);
        for (long i = 0; i < N; i++) put(f, z, &X, i, xs, l);
    } else {
        const st_t *in = OP == ADD ? &SA : OP == SORT || OP == SCAN ? &SS : &SHA;
        printf("%.9g %.9g %zu %zu %ld\n", med, t_s[0], OP == SORT || OP == SCAN ? SS.stride : SO.stride, in->stride, inner);
        if (OP == SORT) for (long i = 0; i < N; i++) fprintf(f, "%d\n", perm[i]);
        else if (OP == SCAN) fprintf(f, "%d\n", scanmax);
        else for (long i = 0; i < N; i++) put(f, z, &SO, i, sgo, L);
    }
    fclose(f);
    return 0;
}
