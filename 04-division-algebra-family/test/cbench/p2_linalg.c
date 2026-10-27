/* Phase two, linear algebra: the format's arrays against GMP 6.3 and FLINT 3.0.1 (built with BLAS) on four exact
   operations over the rationals.

   usage: p2_linalg OP CONTENDER N W REPS [OUTFILE]
     OP    mv     y = A x, A N x N, x N                 entries of (W - ceil(log2 N))/2 - 1 bits
           mm     C = A B, A and B N x N                entries as mv
           spmv   y = A x, A N x N in CSR with K nonzeros per row (K = max(1, N/100); P2_K overrides), x dense,
                  entries of (W - ceil(log2 K))/2 - 1 bits
           solve  A x = b over the rationals, A N x N, b N, entries of W/8 bits (A nonsingular)
       Every result entry needs about W bits (mv, mm, spmv). A has the shared denominator dA, x / B / b the
       shared denominator dB (EDGE=2 makes both 1); so the result's denominator is dA dB (products) or comes out
       of the solve. Entries are deterministic (splitmix64), with exactly the stated bit length and random signs.
     CONTENDER
           ours        the vector form: signed numerators over one shared denominator per vector or matrix.
                       mv, spmv: dot products at width W, numerators stored at width W/2 (the narrowest that holds
                       them), compile-time limbs (W <= 1024), run-time limbs with mpn_mul_n / mpn_add_n (W = 2048);
                       a positive and a negative accumulator per row (no comparison per term). P2_RT=1 forces the
                       run-time-limb (mpn) code at every width, to compare the two sides of the threshold.
                       mm: p1_gemm.c's mmi (multimodular, AVX-512 IFMA kernel, 50-bit primes), its arithmetic
                       unchanged, but here its prime choice and CRT constants are timed (ours_mm_prep).
                       solve: multimodular Cramer: residues modulo 50-bit primes (found inside the timed run, 8 at
                       a time, only as many as the bound needs), 8 primes per AVX-512 vector,
                       Gaussian elimination in Montgomery form with IFMA (common pivot rows, lanes that hit a zero
                       pivot are dropped), det A and y = det A . A^-1 b mod p, the CRT with mmi's IFMA kernel over
                       a Hadamard bound, then x = dA y / (dB det) reduced by the common gcd.
           gmp_mpz     GMP mpz numerators over the shared denominators (mpz_addmul loops; solve: fraction-free
                       Bareiss and fraction-free back substitution, then the same reduction as ours)
           gmp_mpq     GMP mpq per entry (canonical): mpq_mul + mpq_add; solve: Gaussian elimination in mpq
           flint_fmpz  FLINT fmpz numerators over the shared denominators; FLINT's routine chosen by P2_METHOD
                       (see the list above fz_setup), solve followed by ours' reduction to one shared denominator
           flint_fmpq  FLINT fmpq per entry; routine chosen by P2_METHOD (list above fz_setup)
   Inputs are generated as fmpz, converted to each contender's representation before timing (untimed); the output is
   converted to fmpz numerator / denominator pairs after timing (untimed). One warm-up run, then REPS timed runs.
   Prints "median min peakMB extra method": seconds, VmHWM rise (MB) from just before the contender's setup to the
   end of the timed runs, the number of primes (ours mm, ours solve; 0 otherwise) and the FLINT routine used ("-"
   for the others).
   The result is verified after the runs: mv and spmv against FLINT / fmpz loops computed before the timing; mm
   against fmpz_mat_mul_classical (N <= 128) and Freivalds' test (every N); solve by A x == b exactly. A mismatch
   exits 4. P2_EDGE=1: entries cycle through 0, 1, -1, +max, -max and random (A[0][0] = 0); P2_EDGE=2: every entry
   +-max (random signs), denominators 1. P2_CORRUPT adds 1 to one result numerator before the check, which must
   then fail. P2_DROP (ours solve) discards one lane per prime batch, to exercise a varying prime set.
   OUTFILE: the inputs and the result as text (see write_out), for the tests' independent check in Python. */
#define main p1_gemm_main
#include "p1_gemm.c"
#undef main
#include <x86intrin.h>
#include <flint/fmpq.h>
#include <flint/fmpq_mat.h>
#include <flint/fmpq_vec.h>

typedef unsigned __int128 u128;
enum { MV, MM, SPMV, SOLVE };
enum { OURS, GZ, GQ, FZ, FQ };
static int OPK, CON, W, EDGE; static long KNZ, NNZ, ENT;
static fmpz_t dA, dB;
static long *SRP; static int32_t *SCI; static fmpz *SV;       /* the spmv source (CSR); A, B of p1_gemm.c otherwise */
static fmpz *RN, *RD; static long RCNT;                         /* the exported result */
static fmpz_mat_t Yref; static fmpz *SYref;                     /* references (mv, spmv) */
static long extra_out;

static int clog2(long x) { int b = 0; while ((1L << b) < x) b++; return b; }
static uint64_t rs_state = 12345;
static uint64_t rnd(void) {
    uint64_t z = (rs_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL; z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL; return z ^ (z >> 31);
}
static mpz_t gz;
static void rand_bits(fmpz_t f, long e) {             /* exactly e bits, random sign */
    long l = (e + 63) / 64; uint64_t w[80];
    for (long i = 0; i < l; i++) w[i] = rnd();
    int top = (int)(e - 64 * (l - 1));
    if (top < 64) w[l - 1] &= (1ULL << top) - 1;
    w[l - 1] |= 1ULL << (top - 1);
    mpz_import(gz, l, -1, 8, 0, 0, w); if (rnd() & 1) mpz_neg(gz, gz);
    fmpz_set_mpz(f, gz);
}
static void set_max(fmpz_t f, long e, int neg) { fmpz_one(f); fmpz_mul_2exp(f, f, e); fmpz_sub_ui(f, f, 1); if (neg) fmpz_neg(f, f); }
static long gpos;
static void gen_entry(fmpz_t f) {
    long pos = gpos++;
    if (EDGE == 2) { set_max(f, ENT, rnd() & 1); return; }
    if (EDGE == 1) switch (pos % 7) {
        case 0: fmpz_zero(f); return; case 1: fmpz_one(f); return; case 2: fmpz_set_si(f, -1); return;
        case 3: set_max(f, ENT, 0); return; case 4: set_max(f, ENT, 1); return; }
    rand_bits(f, ENT);
}
static fmpz *srcA(long t) { return OPK == SPMV ? SV + t : fmpz_mat_entry(A, t / n, t % n); }
static fmpz *srcB(long t) { return OPK == MM ? fmpz_mat_entry(B, t / n, t % n) : fmpz_mat_entry(B, t, 0); }
static long cntA(void) { return OPK == SPMV ? NNZ : n * n; }
static long cntB(void) { return OPK == MM ? n * n : n; }
static long cntY(void) { return OPK == MM ? n * n : n; }
static int cmpi32(const void *a, const void *b) { int32_t x = *(const int32_t *)a, y = *(const int32_t *)b; return (x > y) - (x < y); }

static void generate(void) {
    fmpz_init(dA); fmpz_init(dB);
    if (EDGE == 2) { fmpz_one(dA); fmpz_one(dB); }
    else { fmpz_set_ui(dA, 721440720UL); fmpz_set_ui(dB, 15504000UL); }   /* 2^4 3^2 5 7^2 11^2 13^2; 2^7 3 5^3 17 19 */
    fmpz_mat_init(A, OPK == SPMV ? 1 : n, OPK == SPMV ? 1 : n);
    fmpz_mat_init(B, n, OPK == MM ? n : 1);
    if (OPK == SPMV) {
        NNZ = n * KNZ; SRP = malloc(sizeof(long) * (n + 1)); SCI = malloc(sizeof(int32_t) * NNZ); SV = _fmpz_vec_init(NNZ);
        long *mark = calloc(n, sizeof(long));
        for (long i = 0; i < n; i++) {
            SRP[i] = i * KNZ;
            for (long t = 0; t < KNZ; t++) { long j; do j = (long)(rnd() % (uint64_t)n); while (mark[j] == i + 1); mark[j] = i + 1; SCI[i * KNZ + t] = (int32_t)j; }
            qsort(SCI + i * KNZ, KNZ, sizeof(int32_t), cmpi32);
        }
        SRP[n] = NNZ; free(mark);
    }
    for (long t = 0; t < cntA(); t++) gen_entry(srcA(t));
    for (long t = 0; t < cntB(); t++) gen_entry(srcB(t));
    if (OPK == SOLVE)                                  /* A must be nonsingular: change entries until it is */
        for (long k = 0; fmpz_mat_rank(A) < n; k++) {
            fmpz *e = fmpz_mat_entry(A, k % n, (k * 3 + 1) % n);
            if (EDGE == 2 && k < 10 * n) fmpz_neg(e, e); else rand_bits(e, ENT);
        }
}

/* fmpz to the vector form's layout: l limbs of magnitude and a sign byte */
static void fz_to_limbs(uint64_t *dst, unsigned char *sg, const fmpz_t f, long l) {
    fmpz_get_mpz(gz, f); *sg = mpz_sgn(gz) < 0; mpz_abs(gz, gz);
    bound((long)mpz_size(gz) <= l, "entry fits its width");
    memset(dst, 0, 8 * l); mpz_export(dst, NULL, -1, 8, 0, 0, gz);
}
static void limbs_to_fz(fmpz_t f, const uint64_t *src, int neg, long l) {
    mpz_import(gz, l, -1, 8, 0, 0, src); if (neg) mpz_neg(gz, gz); fmpz_set_mpz(f, gz);
}

/* ================================================================ ours: mv, spmv (dot products at width W) */
static uint64_t *OA, *OX, *OY; static unsigned char *OAS, *OXS, *OYS; static long OLH; static mpz_t OD;
#define ODOT(LH) \
static inline __attribute__((always_inline)) void odot_row##LH(const uint64_t *restrict a, const unsigned char *restrict as, \
        const int32_t *restrict ci, long cnt, uint64_t *restrict y, unsigned char *ys) { \
    uint64_t acc[2][2 * LH]; memset(acc, 0, sizeof acc); \
    for (long t = 0; t < cnt; t++) { \
        long j = ci ? ci[t] : t; \
        const uint64_t *u = a + t * LH, *v = OX + j * LH; \
        uint64_t pr[2 * LH]; \
        for (int q = 0; q < 2 * LH; q++) pr[q] = 0; \
        for (int s = 0; s < LH; s++) { uint64_t c = 0; \
            for (int r = 0; r < LH; r++) { u128 m = (u128)u[s] * v[r] + pr[s + r] + c; pr[s + r] = (uint64_t)m; c = (uint64_t)(m >> 64); } \
            pr[s + LH] = c; } \
        uint64_t *z = acc[as[t] ^ OXS[j]]; unsigned char cy = 0; \
        for (int q = 0; q < 2 * LH; q++) cy = _addcarry_u64(cy, z[q], pr[q], (unsigned long long *)&z[q]); \
    } \
    int c = 0; \
    for (int q = 2 * LH - 1; q >= 0; q--) if (acc[0][q] != acc[1][q]) { c = acc[0][q] > acc[1][q] ? 1 : -1; break; } \
    const uint64_t *hi = c >= 0 ? acc[0] : acc[1], *lo = c >= 0 ? acc[1] : acc[0]; unsigned char bw = 0; \
    for (int q = 0; q < 2 * LH; q++) bw = _subborrow_u64(bw, hi[q], lo[q], (unsigned long long *)&y[q]); \
    *ys = c < 0; } \
static void odot##LH(void) { \
    if (OPK == SPMV) for (long i = 0; i < n; i++) \
        odot_row##LH(OA + SRP[i] * LH, OAS + SRP[i], SCI + SRP[i], SRP[i + 1] - SRP[i], OY + i * 2 * LH, OYS + i); \
    else for (long i = 0; i < n; i++) odot_row##LH(OA + i * n * LH, OAS + i * n, NULL, n, OY + i * 2 * LH, OYS + i); }
ODOT(1) ODOT(2) ODOT(4) ODOT(8)
/* run-time limbs (W = 2048 and anything above the compile-time widths): GMP's mpn routines */
static uint64_t *ORT, *ORACC; static int ORT_FORCE;
static void odot_rt_row(const uint64_t *a, const unsigned char *as, const int32_t *ci, long cnt, uint64_t *y, unsigned char *ys) {
    long lh = OLH, l = 2 * lh; uint64_t *acc0 = ORACC, *acc1 = ORACC + l, *acc[2] = { acc0, acc1 };
    memset(ORACC, 0, 16 * l);
    for (long t = 0; t < cnt; t++) {
        long j = ci ? ci[t] : t;
        mpn_mul_n(ORT, a + t * lh, OX + j * lh, lh);
        uint64_t *z = acc[as[t] ^ OXS[j]];
        mpn_add_n(z, z, ORT, l);
    }
    int c = mpn_cmp(acc0, acc1, l);
    if (c >= 0) mpn_sub_n(y, acc0, acc1, l); else mpn_sub_n(y, acc1, acc0, l);
    *ys = c < 0;
}
static void odot_rt(void) {
    long l = 2 * OLH;
    if (OPK == SPMV) for (long i = 0; i < n; i++) odot_rt_row(OA + SRP[i] * OLH, OAS + SRP[i], SCI + SRP[i], SRP[i + 1] - SRP[i], OY + i * l, OYS + i);
    else for (long i = 0; i < n; i++) odot_rt_row(OA + i * n * OLH, OAS + i * n, NULL, n, OY + i * l, OYS + i);
}
static void ours_vec_setup(void) {
    OLH = W / 128;                                     /* numerators at width W/2, results at width W */
    bound(2 * ENT + clog2(OPK == SPMV ? KNZ : n) <= W - 2, "ours: |y| < 2^(W-2), so each accumulator fits W bits");
    long ca = cntA();
    OA = malloc(8 * ca * OLH); OAS = malloc(ca); OX = malloc(8 * n * OLH); OXS = malloc(n);
    OY = malloc(16 * n * OLH); OYS = malloc(n);
    for (long t = 0; t < ca; t++) fz_to_limbs(OA + t * OLH, OAS + t, srcA(t), OLH);
    for (long t = 0; t < n; t++) fz_to_limbs(OX + t * OLH, OXS + t, srcB(t), OLH);
    ORT = malloc(16 * OLH); ORACC = malloc(32 * OLH); mpz_init(OD);
}

/* ================================================================ ours: mm (p1_gemm.c's mmi, setup timed)
   Untimed (setup): the inputs to mmi's limb layout (to_limbs), as for every contender's input conversion.
   Timed (ours_mm_prep, every run, before mmi_run): everything that depends on n and the entry size, as FLINT's
   multimodular product does inside its call: pick_primes, M = prod p_k, inv_k = (M/p_k)^-1 mod p_k, 1/p_k, M and
   M/2 as limbs, M/p_k as 52-bit digits (Mk52), T52, the Shoup constants (ipc), and the allocation of the output
   and work buffers; all freed and rebuilt each run. The same computations as mmi_setup / limbs_setup (one
   M/p_k per prime instead of two), and mmi_run itself is unchanged; p1_gemm.c's own measurements are unaffected. */
static int MM_PREPPED;
static void ours_mm_setup(void) {
    LA = (bits + 63) / 64;
    Al = malloc(sizeof(mp_limb_t) * n * n * LA); Bl = malloc(sizeof(mp_limb_t) * n * n * LA);
    As = malloc(n * n); Bs = malloc(n * n); Cs = malloc(n * n);
    to_limbs(Al, As, A); to_limbs(Bl, Bs, B);
    mpz_init(OD);
}
static void ours_mm_free(void) {
    free(primes); free(inv); free(pinv); free(Ml); free(Mhalf); free(Cl); free(ipc); free(T52); free(Mk52);
    free(Apk); free(Bpk); free(Ar); free(Br); free(Dblk); free(Rlo); free(Rhi); free(Plo); free(Phi); free(Si);
    free(Xlo); free(Xhi); free(xt); free(mt);
}
static void ours_mm_prep(void) {
    if (MM_PREPPED) ours_mm_free();
    MM_PREPPED = 1;
    pick_primes(1UL << 50);                               /* the largest primes below 2^50, as many as needed */
    fmpz_t M, Mk; fmpz_init(M); fmpz_init(Mk);
    fmpz_one(M);
    for (long k = 0; k < np; k++) fmpz_mul_ui(M, M, primes[k]);
    long mbits = fmpz_bits(M), nn = n * n;
    LM = (mbits + 63) / 64; LC = LM + 1;
    Cl = calloc((size_t)nn * LC, sizeof(mp_limb_t));
    nd52 = (bits + 51) / 52; ndc52 = (mbits + 51) / 52;
    bound(primes[np - 1] > (1UL << 49) && primes[0] < (1UL << 50), "mmi: 2^49 < p < 2^50");
    bound(nd52 < (1L << 14), "mmi reduction: nd52 2^102 < 2^116");
    bound(n < (1L << 16), "mmi product: N 2^100 < 2^116");
    bound(np < (1L << 14), "mmi CRT: np 2^102 < 2^116");
    inv = malloc(sizeof(mp_limb_t) * np); pinv = malloc(sizeof(double) * np); ipc = malloc(sizeof(ipc_t) * np);
    T52 = malloc(sizeof(uint64_t) * np * nd52); Mk52 = malloc(sizeof(uint64_t) * np * ndc52);
    mpz_t z; mpz_init(z);
    for (long k = 0; k < np; k++) {
        uint64_t p = primes[k];
        fmpz_divexact_ui(Mk, M, p);
        inv[k] = n_invmod(fmpz_fdiv_ui(Mk, p), p); pinv[k] = 1.0 / (double)p;
        uint64_t c1 = (uint64_t)(((unsigned __int128)1 << 52) % p), c2 = (uint64_t)((unsigned __int128)c1 * c1 % p);
        ipc[k] = (ipc_t){ p, c1, shoup_pre(c1, p), c2, shoup_pre(c2, p), shoup_pre(1, p), inv[k], shoup_pre(inv[k], p) };
        uint64_t t = 1; for (long j = 0; j < nd52; j++) { T52[k * nd52 + j] = t; t = (uint64_t)((unsigned __int128)t * c1 % p); }
        fmpz_get_mpz(z, Mk);
        for (long j = 0; j < ndc52; j++) Mk52[k * ndc52 + j] = digit52(mpz_limbs_read(z), mpz_size(z), j);
    }
    Ml = calloc(LC, sizeof(mp_limb_t)); Mhalf = calloc(LC, sizeof(mp_limb_t));
    fmpz_get_mpz(z, M); mpz_export(Ml, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
    mpz_fdiv_q_2exp(z, z, 1); mpz_export(Mhalf, NULL, -1, sizeof(mp_limb_t), 0, 0, z);
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

/* ================================================================ ours: solve (multimodular Cramer, IFMA) */
/* Montgomery arithmetic modulo a 50-bit prime per lane, R = 2^52, pp = -p^-1 mod 2^52. For a b < p 2^52:
   lo + hi 2^52 = a b, m = lo pp mod 2^52, (a b + m p) / 2^52 = hi + hi52(m p) + (lo != 0) < 2p; one correction. */
static inline __m512i mmul(__m512i a, __m512i b, __m512i p, __m512i pp) {
    const __m512i z = _mm512_setzero_si512();
    __m512i lo = _mm512_madd52lo_epu64(z, a, b), hi = _mm512_madd52hi_epu64(z, a, b);
    __m512i m = _mm512_madd52lo_epu64(z, lo, pp);
    __m512i t = _mm512_madd52hi_epu64(hi, m, p);
    t = _mm512_mask_add_epi64(t, _mm512_test_epi64_mask(lo, lo), t, _mm512_set1_epi64(1));
    return _mm512_min_epu64(t, _mm512_sub_epi64(t, p));
}
static inline __m512i msub(__m512i a, __m512i b, __m512i p) { __m512i s = _mm512_sub_epi64(a, b); return _mm512_min_epu64(s, _mm512_add_epi64(s, p)); }
static inline __m512i madd(__m512i a, __m512i b, __m512i p) { __m512i s = _mm512_add_epi64(a, b); return _mm512_min_epu64(s, _mm512_sub_epi64(s, p)); }
static inline __m512i vinv(__m512i a, __m512i p, __m512i pp, __m512i one) {     /* a^(p-2), Montgomery form */
    __m512i e = _mm512_sub_epi64(p, _mm512_set1_epi64(2)), r = a;                /* bit 49 of p - 2 is set */
    for (int b = 48; b >= 0; b--) {
        r = mmul(r, r, p, pp);
        r = _mm512_mask_mov_epi64(r, _mm512_test_epi64_mask(e, _mm512_set1_epi64(1LL << b)), mmul(r, a, p, pp));
    }
    (void)one; return r;
}
static long NPT, SND, SLA, SGOOD; static int SDROP;
static uint64_t *TP, *TPP, *TONE, *TC;               /* per prime: p, pp, R mod p; TC[j][k] = 2^(52 (j+2)) mod p */
static double *TL2;
static uint64_t *SL, *SDIG, *SRES, *SGP, *SMK, *SXlo, *SXhi; static unsigned char *SS; static double *SSUM;
static __m512i *SMX, *SXV, *SYB; static long *SRW;
static mpz_t *SY, *ONUM, ODEN;
static long SLCMAX;
static uint64_t binv64(uint64_t d) { uint64_t x = d; for (int i = 0; i < 6; i++) x *= 2 - d * x; return x; }
static void ours_solve_setup(void) {
    SLA = (ENT + 63) / 64; SND = (ENT + 51) / 52;
    double est = n * (ENT + 0.5 * log2((double)n + 1) + 1) + 16;   /* >= the Hadamard bound below, in bits */
    NPT = ((long)(est / 49.0) + 64 + 7) / 8 * 8;
    TP = malloc(8 * NPT); TPP = malloc(8 * NPT); TONE = malloc(8 * NPT); TC = malloc(8 * NPT * SND); TL2 = malloc(8 * NPT);
    long ne = n * n + n;
    SL = malloc(8 * ne * SLA); SS = malloc(ne);
    for (long e = 0; e < ne; e++) fz_to_limbs(SL + e * SLA, SS + e, e < n * n ? srcA(e) : srcB(e - n * n), SLA);
    SDIG = malloc(8 * ne * SND);
    SMX = aligned_alloc(64, 64 * n * (n + 1)); SXV = aligned_alloc(64, 64 * n); SYB = aligned_alloc(64, 64 * (n + 1));
    SRW = malloc(sizeof(long) * n);
    SRES = malloc(8 * (n + 1) * NPT); SGP = malloc(8 * NPT); SSUM = malloc(8 * (n + 1));
    long ndcmax = (NPT * 50) / 52 + 2; SLCMAX = (NPT * 50) / 64 + 3;
    SMK = malloc(8 * NPT * ndcmax); SXlo = malloc(8 * (n + 1) * ndcmax); SXhi = malloc(8 * (n + 1) * ndcmax);
    Ml = malloc(8 * SLCMAX); Mhalf = malloc(8 * SLCMAX); mt = malloc(8 * (SLCMAX + 2));
    XTL = (52 * (ndcmax + 4)) / 64 + SLCMAX + 4; xt = malloc(8 * XTL);
    Cl = malloc(8 * (n + 1) * SLCMAX); Cs = malloc(n + 1);
    Apk = aligned_alloc(64, sizeof(uint64_t) * (IMC + IMR) * IKC); Bpk = aligned_alloc(64, sizeof(uint64_t) * (INC + INR) * IKC);
    SY = malloc(sizeof(mpz_t) * (n + 1)); ONUM = malloc(sizeof(mpz_t) * n);
    for (long i = 0; i <= n; i++) mpz_init2(SY[i], 64 * SLCMAX);
    for (long i = 0; i < n; i++) mpz_init(ONUM[i]);
    mpz_init(ODEN);
}
/* an upper bound for |value| as a double (the top limb rounded up) */
/* Primes and their constants are made inside the timed run (FLINT also chooses its primes inside its calls):
   one batch of 8 at a time, the next primes below the previous one starting from 2^50, only until the bound is
   covered (the last batch may hold up to 7 primes beyond it: its lanes are computed anyway). The arrays above
   are only allocated in setup. */
static uint64_t SPCUR;
static void gen_primes_batch(long b0) {
    uint64_t p = SPCUR;
    for (long k = b0; k < b0 + 8; k++) {
        do p--; while (!n_is_prime(p));
        TP[k] = p; TPP[k] = (0 - binv64(p)) & M52; TL2[k] = log2((double)p);
        uint64_t r = (uint64_t)(((u128)1 << 52) % p); TONE[k] = r;
        uint64_t c = (uint64_t)((u128)r * r % p);                                    /* 2^104 */
        for (long j = 0; j < SND; j++) { TC[j * NPT + k] = c; c = (uint64_t)((u128)c * r % p); }
    }
    SPCUR = p;
    bound(p > (1UL << 49), "ours solve: 2^49 < p < 2^50");
}
static double ub_d(const uint64_t *L, long l) {
    long t = l - 1; while (t >= 0 && !L[t]) t--;
    if (t < 0) return 0;
    int b = 64 - __builtin_clzll(L[t]), s = b > 52 ? b - 52 : 0;
    return ldexp((double)(L[t] >> s) + 1.0, s + 64 * (int)t);
}
/* one batch of 8 primes TP[b0 ..]: residues, elimination, det and y = det x mod p into SYB (standard form);
   returns the lanes that succeeded */
static __mmask8 solve_batch(long b0) {
    const long w = n + 1;
    const __m512i P = _mm512_loadu_si512(TP + b0), PP = _mm512_loadu_si512(TPP + b0), ONE = _mm512_loadu_si512(TONE + b0);
    const __m512i z = _mm512_setzero_si512();
    for (long e = 0; e < n * n + n; e++) {          /* residues in Montgomery form: sum_j mont(d_j, 2^(52(j+2))) */
        const uint64_t *d = SDIG + e * SND; __m512i s = z;
        for (long j = 0; j < SND; j++) s = madd(s, mmul(_mm512_set1_epi64((long long)d[j]), _mm512_loadu_si512(TC + j * NPT + b0), P, PP), P);
        if (SS[e]) s = _mm512_mask_sub_epi64(s, _mm512_test_epi64_mask(s, s), P, s);
        if (e < n * n) SMX[(e / n) * w + e % n] = s; else SMX[(e - n * n) * w + n] = s;
    }
    for (long i = 0; i < n; i++) SRW[i] = i;
    __mmask8 vm = 0xff; if (SDROP) vm &= (__mmask8)~(1u << ((b0 / 8) % 8));
    __m512i det = ONE; long swaps = 0;
    for (long k = 0; k < n; k++) {
        long best = -1; int bestc = -1;               /* a pivot row nonzero in every live lane, else the most lanes */
        for (long r = k; r < n; r++) {
            __m512i v = SMX[SRW[r] * w + k]; __mmask8 nz = _mm512_test_epi64_mask(v, v) & vm;
            int c = __builtin_popcount(nz);
            if (c > bestc) { bestc = c; best = r; }
            if (nz == vm) break;
        }
        { __m512i v = SMX[SRW[best] * w + k]; vm &= _mm512_test_epi64_mask(v, v); }
        if (!vm) return 0;
        if (best != k) { long t = SRW[k]; SRW[k] = SRW[best]; SRW[best] = t; swaps++; }
        __m512i *rk = SMX + SRW[k] * w;
        det = mmul(det, rk[k], P, PP);
        __m512i iv = vinv(rk[k], P, PP, ONE);
        for (long j = k + 1; j <= n; j++) rk[j] = mmul(rk[j], iv, P, PP);
        for (long i = k + 1; i < n; i++) {
            __m512i *ri = SMX + SRW[i] * w, f = ri[k];
            if (!_mm512_test_epi64_mask(f, f)) continue;  /* a zero entry (in every lane): nothing to do */
            for (long j = k + 1; j <= n; j++) ri[j] = msub(ri[j], mmul(f, rk[j], P, PP), P);
        }
    }
    for (long i = n - 1; i >= 0; i--) {               /* unit upper triangular: x_i = c_i - sum_j u_ij x_j */
        __m512i *ri = SMX + SRW[i] * w, s = ri[n];
        for (long j = i + 1; j < n; j++) s = msub(s, mmul(ri[j], SXV[j], P, PP), P);
        SXV[i] = s;
    }
    if (swaps & 1) det = msub(z, det, P);
    const __m512i one = _mm512_set1_epi64(1);
    for (long i = 0; i < n; i++) SYB[i] = mmul(mmul(det, SXV[i], P, PP), one, P, PP);   /* y = det x, out of Montgomery form */
    SYB[n] = mmul(det, one, P, PP);
    return vm;
}
static void reduce_solution_mpz(mpz_t *y, mpz_t det, mpz_t *num, mpz_t den);
static int ours_solve_run(void) {
    const long ne = n * n + n;
    for (long e = 0; e < ne; e++) for (long j = 0; j < SND; j++) SDIG[e * SND + j] = digit52(SL + e * SLA, SLA, j);
    /* Hadamard: |det A| and every |det A_i| (column i replaced by b) <= prod_j max(|col_j|, |b|) */
    double sb = 0, need = 0;
    for (long i = 0; i < n; i++) { double u = ub_d(SL + (n * n + i) * SLA, SLA); sb += u * u; }
    for (long j = 0; j < n; j++) {
        double s = 0;
        for (long i = 0; i < n; i++) { double u = ub_d(SL + (i * n + j) * SLA, SLA); s += u * u; }
        s = s > sb ? s : sb; s *= 1 + 0x1p-40;      /* covers the roundings of n + 1 sums of squares */
        need += 0.5 * log2(s > 1 ? s : 1) + 1e-9;
    }
    need += 3;                                         /* M > 2 H + 1, with room for the roundings of log2 */
    double have = 0; SGOOD = 0; int dead = 0;
    SPCUR = 1UL << 50;
    for (long b0 = 0; have < need; b0 += 8) {
        if (b0 + 8 > NPT) { fprintf(stderr, "ours solve: prime table exhausted\n"); exit(6); }
        gen_primes_batch(b0);
        __mmask8 vm = solve_batch(b0);
        if (!vm) { if (++dead >= 3) return 0; continue; }   /* three dead batches in a row: singular */
        dead = 0;
        for (int l = 0; l < 8; l++) if (vm >> l & 1) {
            const uint64_t *yb = (const uint64_t *)SYB;
            for (long v = 0; v <= n; v++) SRES[v * NPT + SGOOD] = yb[v * 8 + l];
            SGP[SGOOD++] = TP[b0 + l]; have += TL2[b0 + l];
        }
    }
    /* CRT (as mmi): y'_k = r_k (M/p_k)^-1 mod p_k, X = Y' . digits52(M/p_k) on the IFMA kernel, minus floor(s) M */
    mpz_t M, Mk; mpz_init_set_ui(M, 1); mpz_init(Mk);
    for (long k = 0; k < SGOOD; k++) mpz_mul_ui(M, M, SGP[k]);
    long mbits = mpz_sizeinbase(M, 2), ndc = (mbits + 51) / 52;
    LM = (mbits + 63) / 64; LC = LM + 1; bound(LC <= SLCMAX, "ours solve: limbs");
    memset(Ml, 0, 8 * LC); memset(Mhalf, 0, 8 * LC);
    mpz_export(Ml, NULL, -1, 8, 0, 0, M); mpz_fdiv_q_2exp(Mk, M, 1); mpz_export(Mhalf, NULL, -1, 8, 0, 0, Mk);
    bound(SGOOD < (1L << 14), "ours solve CRT: np 2^102 < 2^116");
    for (long v = 0; v <= n; v++) SSUM[v] = 0;
    for (long k = 0; k < SGOOD; k++) {
        uint64_t p = SGP[k];
        mpz_divexact_ui(Mk, M, p);
        uint64_t iv = n_invmod(mpz_fdiv_ui(Mk, p), p), ivs = (uint64_t)(((u128)iv << 64) / p);
        double pi = 1.0 / (double)p;
        for (long v = 0; v <= n; v++) {
            uint64_t r = SRES[v * NPT + k], q = (uint64_t)(((u128)r * ivs) >> 64), y = r * iv - q * p;
            if (y >= p) y -= p;
            SRES[v * NPT + k] = y; SSUM[v] += (double)y * pi;
        }
        const mp_limb_t *ml = mpz_limbs_read(Mk); long ms = mpz_size(Mk);
        for (long j = 0; j < ndc; j++) SMK[k * ndc + j] = digit52(ml, ms, j);
    }
    ifma_gemm(n + 1, ndc, SGOOD, SRES, NPT, 1, SMK, ndc, 1, SXlo, SXhi, ndc, 0);
    for (long v = 0; v <= n; v++) {
        memset(xt, 0, sizeof(mp_limb_t) * XTL);
        const uint64_t *lo = SXlo + v * ndc, *hi = SXhi + v * ndc;
        unsigned __int128 c = 0;
        for (long j = 0; c || j <= ndc; j++) {
            if (j < ndc) c += lo[j];
            if (j > 0 && j <= ndc) c += hi[j - 1];
            uint64_t d = (uint64_t)c & M52; c >>= 52;
            long off = 52 * j, wd = off / 64, s = off % 64;
            xt[wd] |= d << s;
            if (s > 12) xt[wd + 1] |= d >> (64 - s);
        }
        crt_finish(v, SSUM[v]);
        mpz_import(SY[v], LC, -1, 8, 0, 0, Cl + v * LC); if (Cs[v]) mpz_neg(SY[v], SY[v]);
    }
    mpz_clear(M); mpz_clear(Mk);
    if (!mpz_sgn(SY[n])) return 0;
    reduce_solution_mpz(SY, SY[n], ONUM, ODEN);
    extra_out = SGOOD;
    return 1;
}

/* x = dA y / (dB det) over one positive denominator, reduced by the gcd of everything (y and det are clobbered) */
static mpz_t ZdA, ZdB;
static void reduce_solution_mpz(mpz_t *y, mpz_t det, mpz_t *num, mpz_t den) {
    if (mpz_sgn(det) < 0) { for (long i = 0; i < n; i++) mpz_neg(y[i], y[i]); mpz_neg(det, det); }
    mpz_mul(den, det, ZdB);
    mpz_t g; mpz_init_set(g, den);
    for (long i = 0; i < n; i++) { mpz_mul(num[i], y[i], ZdA); if (mpz_cmp_ui(g, 1)) mpz_gcd(g, g, num[i]); }
    if (mpz_cmp_ui(g, 1)) { for (long i = 0; i < n; i++) mpz_divexact(num[i], num[i], g); mpz_divexact(den, den, g); }
    mpz_clear(g);
}
static void reduce_solution_fmpz(fmpz *y, fmpz_t det, fmpz *num, fmpz_t den) {
    if (fmpz_sgn(det) < 0) { _fmpz_vec_neg(y, y, n); fmpz_neg(det, det); }
    fmpz_mul(den, det, dB);
    fmpz_t g; fmpz_init_set(g, den);
    for (long i = 0; i < n; i++) { fmpz_mul(num + i, y + i, dA); if (!fmpz_is_one(g)) fmpz_gcd(g, g, num + i); }
    if (!fmpz_is_one(g)) { for (long i = 0; i < n; i++) fmpz_divexact(num + i, num + i, g); fmpz_divexact(den, den, g); }
    fmpz_clear(g);
}

/* ================================================================ gmp_mpz */
static mpz_t *ZA, *ZB, *ZY, ZD, *ZW, ZT; static mpz_t **ZR;
static void gz_setup(void) {
    long ca = cntA(), cb = cntB(), cy = cntY();
    ZA = malloc(sizeof(mpz_t) * ca); ZB = malloc(sizeof(mpz_t) * cb);
    for (long t = 0; t < ca; t++) { mpz_init(ZA[t]); fmpz_get_mpz(ZA[t], srcA(t)); }
    for (long t = 0; t < cb; t++) { mpz_init(ZB[t]); fmpz_get_mpz(ZB[t], srcB(t)); }
    mpz_init(ZD); mpz_init(ZT);
    if (OPK == SOLVE) {                                /* the work matrix [A | b], row pointers, y */
        ZW = malloc(sizeof(mpz_t) * n * (n + 1)); ZR = malloc(sizeof(mpz_t *) * n);
        for (long t = 0; t < n * (n + 1); t++) mpz_init2(ZW[t], 64 + n * (ENT + 8));
        ZY = malloc(sizeof(mpz_t) * (n + 1)); for (long i = 0; i <= n; i++) mpz_init2(ZY[i], 64 + n * (ENT + 8));
        ONUM = malloc(sizeof(mpz_t) * n); for (long i = 0; i < n; i++) mpz_init(ONUM[i]); mpz_init(ODEN);
    } else { ZY = malloc(sizeof(mpz_t) * cy); for (long i = 0; i < cy; i++) mpz_init2(ZY[i], W + 64); }
}
static int gz_bareiss(void) {
    const long w = n + 1;
    for (long i = 0; i < n; i++) {
        ZR[i] = ZW + i * w;
        for (long j = 0; j < n; j++) mpz_set(ZR[i][j], ZA[i * n + j]);
        mpz_set(ZR[i][n], ZB[i]);
    }
    for (long k = 0; k < n; k++) {
        if (!mpz_sgn(ZR[k][k])) {
            long r = k + 1; while (r < n && !mpz_sgn(ZR[r][k])) r++;
            if (r == n) return 0;
            mpz_t *t = ZR[k]; ZR[k] = ZR[r]; ZR[r] = t;
        }
        mpz_t *rk = ZR[k];
        for (long i = k + 1; i < n; i++) {
            mpz_t *ri = ZR[i];
            for (long j = k + 1; j <= n; j++) {        /* (a_kk a_ij - a_ik a_kj) / a_{k-1,k-1} */
                mpz_mul(ZT, rk[k], ri[j]); mpz_submul(ZT, ri[k], rk[j]);
                if (k) mpz_divexact(ri[j], ZT, ZR[k - 1][k - 1]); else mpz_swap(ri[j], ZT);
            }
        }
    }
    /* U y = det' c with det' = U_nn = +-det A: y_i = (det' c_i - sum_j u_ij y_j) / u_ii, exact */
    mpz_set(ZY[n], ZR[n - 1][n - 1]);
    for (long i = n - 1; i >= 0; i--) {
        mpz_mul(ZT, ZY[n], ZR[i][n]);
        for (long j = i + 1; j < n; j++) mpz_submul(ZT, ZR[i][j], ZY[j]);
        mpz_divexact(ZY[i], ZT, ZR[i][i]);
    }
    reduce_solution_mpz(ZY, ZY[n], ONUM, ODEN);
    return 1;
}
static int gz_run(void) {
    switch (OPK) {
    case MV: for (long i = 0; i < n; i++) { mpz_set_ui(ZY[i], 0); for (long j = 0; j < n; j++) mpz_addmul(ZY[i], ZA[i * n + j], ZB[j]); } break;
    case SPMV: for (long i = 0; i < n; i++) { mpz_set_ui(ZY[i], 0); for (long t = SRP[i]; t < SRP[i + 1]; t++) mpz_addmul(ZY[i], ZA[t], ZB[SCI[t]]); } break;
    case MM:                                           /* i-k-j: a row of C accumulates a_ik times a row of B */
        for (long i = 0; i < n * n; i++) mpz_set_ui(ZY[i], 0);
        for (long i = 0; i < n; i++) for (long k = 0; k < n; k++) {
            mpz_srcptr a = ZA[i * n + k]; mpz_t *c = ZY + i * n, *b = ZB + k * n;
            if (!mpz_sgn(a)) continue;
            for (long j = 0; j < n; j++) mpz_addmul(c[j], a, b[j]);
        }
        break;
    case SOLVE: return gz_bareiss();
    }
    mpz_mul(ZD, ZdA, ZdB);
    return 1;
}

/* ================================================================ gmp_mpq */
static mpq_t *QA, *QB, *QY, *QW, QT, QF; static mpq_t **QR;
static void gq_setup(void) {
    long ca = cntA(), cb = cntB(), cy = cntY();
    QA = malloc(sizeof(mpq_t) * ca); QB = malloc(sizeof(mpq_t) * cb);
    for (long t = 0; t < ca; t++) { mpq_init(QA[t]); fmpz_get_mpz(mpq_numref(QA[t]), srcA(t)); mpz_set(mpq_denref(QA[t]), ZdA); mpq_canonicalize(QA[t]); }
    for (long t = 0; t < cb; t++) { mpq_init(QB[t]); fmpz_get_mpz(mpq_numref(QB[t]), srcB(t)); mpz_set(mpq_denref(QB[t]), ZdB); mpq_canonicalize(QB[t]); }
    mpq_init(QT); mpq_init(QF);
    QY = malloc(sizeof(mpq_t) * cy); for (long i = 0; i < cy; i++) mpq_init(QY[i]);
    if (OPK == SOLVE) {
        QW = malloc(sizeof(mpq_t) * n * (n + 1)); QR = malloc(sizeof(mpq_t *) * n);
        for (long t = 0; t < n * (n + 1); t++) mpq_init(QW[t]);
    }
}
static int gq_solve(void) {
    const long w = n + 1;
    for (long i = 0; i < n; i++) {
        QR[i] = QW + i * w;
        for (long j = 0; j < n; j++) mpq_set(QR[i][j], QA[i * n + j]);
        mpq_set(QR[i][n], QB[i]);
    }
    for (long k = 0; k < n; k++) {
        if (!mpq_sgn(QR[k][k])) {
            long r = k + 1; while (r < n && !mpq_sgn(QR[r][k])) r++;
            if (r == n) return 0;
            mpq_t *t = QR[k]; QR[k] = QR[r]; QR[r] = t;
        }
        for (long i = k + 1; i < n; i++) {
            if (!mpq_sgn(QR[i][k])) continue;
            mpq_div(QF, QR[i][k], QR[k][k]);
            for (long j = k + 1; j <= n; j++) { mpq_mul(QT, QF, QR[k][j]); mpq_sub(QR[i][j], QR[i][j], QT); }
        }
    }
    for (long i = n - 1; i >= 0; i--) {
        mpq_set(QF, QR[i][n]);
        for (long j = i + 1; j < n; j++) { mpq_mul(QT, QR[i][j], QY[j]); mpq_sub(QF, QF, QT); }
        mpq_div(QY[i], QF, QR[i][i]);
    }
    return 1;
}
static int gq_run(void) {
    switch (OPK) {
    case MV: for (long i = 0; i < n; i++) { mpq_set_ui(QY[i], 0, 1); for (long j = 0; j < n; j++) { mpq_mul(QT, QA[i * n + j], QB[j]); mpq_add(QY[i], QY[i], QT); } } break;
    case SPMV: for (long i = 0; i < n; i++) { mpq_set_ui(QY[i], 0, 1); for (long t = SRP[i]; t < SRP[i + 1]; t++) { mpq_mul(QT, QA[t], QB[SCI[t]]); mpq_add(QY[i], QY[i], QT); } } break;
    case MM:
        for (long i = 0; i < n * n; i++) mpq_set_ui(QY[i], 0, 1);
        for (long i = 0; i < n; i++) for (long k = 0; k < n; k++) {
            mpq_srcptr a = QA[i * n + k]; mpq_t *c = QY + i * n, *b = QB + k * n;
            if (!mpq_sgn(a)) continue;
            for (long j = 0; j < n; j++) { mpq_mul(QT, a, b[j]); mpq_add(c[j], c[j], QT); }
        }
        break;
    case SOLVE: return gq_solve();
    }
    return 1;
}

/* ================================================================ flint_fmpz, flint_fmpq
   P2_METHOD picks FLINT's routine (the bench chooses it on a calibration run and reports a separate run, as for
   ours); unset, the first name in the list is used, except flint_fmpz mv, where it is mat up to W = 256 and dot
   above (the faster of those two in a probe).
     flint_fmpz  mv     mat (fmpz_mat_mul by an N x 1 matrix), dot (_fmpz_vec_dot per row), vec (fmpz_mat_mul_fmpz_vec)
                 mm     mul (fmpz_mat_mul), blas (fmpz_mat_mul_blas), multi_mod (fmpz_mat_mul_multi_mod),
                        fft (fmpz_mat_mul_fft)
                 spmv   addmul (CSR loop with fmpz_addmul; FLINT 3.0.1 has no sparse type)
                 solve  solve (fmpz_mat_solve), dixon (fmpz_mat_solve_dixon_den), multi_mod
                        (fmpz_mat_solve_multi_mod_den), fflu (fmpz_mat_solve_fflu), q_solve / q_dixon / q_multi_mod
                        (fmpq_mat_solve_fmpz_mat[_dixon|_multi_mod], whose canonical entries are then put over their
                        lcm); every one ends with the same reduction to one shared denominator as ours
     flint_fmpq  mv     mul (fmpq_mat_mul by an N x 1 matrix), cleared (fmpq_mat_mul_cleared), direct
                        (fmpq_mat_mul_direct), vec (fmpq_mat_mul_fmpq_vec)
                 mm     mul, cleared, direct
                 spmv   addmul (CSR loop with fmpq_addmul)
                 solve  solve (fmpq_mat_solve), dixon (fmpq_mat_solve_dixon), multi_mod (fmpq_mat_solve_multi_mod),
                        fraction_free (fmpq_mat_solve_fraction_free) */
static const char *FZM[4][8] = { { "mat", "dot", "vec" }, { "mul", "blas", "multi_mod", "fft" }, { "addmul" },
    { "solve", "dixon", "multi_mod", "fflu", "q_solve", "q_dixon", "q_multi_mod" } };
static const char *FQM[4][8] = { { "mul", "cleared", "direct", "vec" }, { "mul", "cleared", "direct" }, { "addmul" },
    { "solve", "dixon", "multi_mod", "fraction_free" } };
static const char *FMETH = "";
static int fmeth(const char *m) { return is(FMETH, m); }
static void choose_method(void) {
    const char *env = getenv("P2_METHOD");
    if (CON != FZ && CON != FQ) { if (env && *env) { fprintf(stderr, "P2_METHOD is for the FLINT contenders\n"); exit(2); } return; }
    const char **list = CON == FZ ? FZM[OPK] : FQM[OPK];
    if (!env || !*env) { FMETH = (CON == FZ && OPK == MV) ? (W <= 256 ? "mat" : "dot") : list[0]; return; }
    for (int i = 0; i < 8 && list[i]; i++) if (is(env, list[i])) { FMETH = list[i]; return; }
    fprintf(stderr, "unknown method %s\n", env); exit(2);
}
static fmpz_mat_t FMA, FMB, FMY; static fmpz *FVA, *FVB, *FVY, *FNUM; static fmpz_t FD;
static fmpq_mat_t QMA, QMB, QMY; static fmpq *QVA, *QVB, *QVY;
static void fz_setup(void) {
    fmpz_init(FD);
    if (OPK == SPMV) {
        FVA = _fmpz_vec_init(NNZ); FVB = _fmpz_vec_init(n); FVY = _fmpz_vec_init(n);
        for (long t = 0; t < NNZ; t++) fmpz_set(FVA + t, SV + t);
        for (long t = 0; t < n; t++) fmpz_set(FVB + t, srcB(t));
        return;
    }
    fmpz_mat_init_set(FMA, A); fmpz_mat_init_set(FMB, B);
    fmpz_mat_init(FMY, n, OPK == MM ? n : 1);
    if (OPK == MV) { FVB = _fmpz_vec_init(n); for (long t = 0; t < n; t++) fmpz_set(FVB + t, srcB(t)); FVY = _fmpz_vec_init(n); }
    if (OPK == SOLVE) { FVY = _fmpz_vec_init(n); FNUM = _fmpz_vec_init(n); fmpq_mat_init(QMY, n, 1); }
}
static int fz_run(void) {
    switch (OPK) {
    case MV:
        if (fmeth("mat")) fmpz_mat_mul(FMY, FMA, FMB);
        else if (fmeth("vec")) fmpz_mat_mul_fmpz_vec(FVY, FMA, FVB, n);
        else for (long i = 0; i < n; i++) _fmpz_vec_dot(FVY + i, FMA->rows[i], FVB, n);
        break;
    case SPMV: for (long i = 0; i < n; i++) { fmpz_zero(FVY + i); for (long t = SRP[i]; t < SRP[i + 1]; t++) fmpz_addmul(FVY + i, FVA + t, FVB + SCI[t]); } break;
    case MM:
        if (fmeth("blas")) { if (!fmpz_mat_mul_blas(FMY, FMA, FMB)) { fprintf(stderr, "fmpz_mat_mul_blas: not available\n"); exit(2); } }
        else if (fmeth("multi_mod")) fmpz_mat_mul_multi_mod(FMY, FMA, FMB);
        else if (fmeth("fft")) fmpz_mat_mul_fft(FMY, FMA, FMB);
        else fmpz_mat_mul(FMY, FMA, FMB);
        break;
    case SOLVE: {
        fmpz_t den; fmpz_init(den);
        int ok;
        if (FMETH[0] == 'q') {                           /* canonical rationals X with A X = b, over their lcm */
            ok = fmeth("q_dixon") ? fmpq_mat_solve_fmpz_mat_dixon(QMY, FMA, FMB)
               : fmeth("q_multi_mod") ? fmpq_mat_solve_fmpz_mat_multi_mod(QMY, FMA, FMB) : fmpq_mat_solve_fmpz_mat(QMY, FMA, FMB);
            if (ok) {
                fmpz_one(den);
                for (long i = 0; i < n; i++) fmpz_lcm(den, den, fmpq_denref(fmpq_mat_entry(QMY, i, 0)));
                for (long i = 0; i < n; i++) {
                    fmpq *q = fmpq_mat_entry(QMY, i, 0);
                    fmpz_divexact(FVY + i, den, fmpq_denref(q)); fmpz_mul(FVY + i, FVY + i, fmpq_numref(q));
                }
            }
        } else {
            ok = fmeth("dixon") ? fmpz_mat_solve_dixon_den(FMY, den, FMA, FMB)
               : fmeth("multi_mod") ? fmpz_mat_solve_multi_mod_den(FMY, den, FMA, FMB)
               : fmeth("fflu") ? fmpz_mat_solve_fflu(FMY, den, FMA, FMB) : fmpz_mat_solve(FMY, den, FMA, FMB);
            if (ok) for (long i = 0; i < n; i++) fmpz_set(FVY + i, fmpz_mat_entry(FMY, i, 0));
        }
        if (ok) reduce_solution_fmpz(FVY, den, FNUM, FD);
        fmpz_clear(den);
        return ok; }
    }
    fmpz_mul(FD, dA, dB);
    return 1;
}
static void fq_setup(void) {
    if (OPK == SPMV) {
        QVA = _fmpq_vec_init(NNZ); QVB = _fmpq_vec_init(n); QVY = _fmpq_vec_init(n);
        for (long t = 0; t < NNZ; t++) fmpq_set_fmpz_frac(QVA + t, SV + t, dA);
        for (long t = 0; t < n; t++) fmpq_set_fmpz_frac(QVB + t, srcB(t), dB);
        return;
    }
    fmpq_mat_init(QMA, n, n); fmpq_mat_init(QMB, n, OPK == MM ? n : 1); fmpq_mat_init(QMY, n, OPK == MM ? n : 1);
    for (long t = 0; t < n * n; t++) fmpq_set_fmpz_frac(fmpq_mat_entry(QMA, t / n, t % n), srcA(t), dA);
    for (long t = 0; t < cntB(); t++) fmpq_set_fmpz_frac(OPK == MM ? fmpq_mat_entry(QMB, t / n, t % n) : fmpq_mat_entry(QMB, t, 0), srcB(t), dB);
    if (OPK == MV) {
        QVB = _fmpq_vec_init(n); QVY = _fmpq_vec_init(n);
        for (long t = 0; t < n; t++) fmpq_set(QVB + t, fmpq_mat_entry(QMB, t, 0));
    }
}
static int fq_run(void) {
    switch (OPK) {
    case MV: case MM:
        if (OPK == MV && fmeth("vec")) fmpq_mat_mul_fmpq_vec(QVY, QMA, QVB, n);
        else if (fmeth("cleared")) fmpq_mat_mul_cleared(QMY, QMA, QMB);
        else if (fmeth("direct")) fmpq_mat_mul_direct(QMY, QMA, QMB);
        else fmpq_mat_mul(QMY, QMA, QMB);
        break;
    case SPMV: for (long i = 0; i < n; i++) { fmpq_zero(QVY + i); for (long t = SRP[i]; t < SRP[i + 1]; t++) fmpq_addmul(QVY + i, QVA + t, QVB + SCI[t]); } break;
    case SOLVE:
        return fmeth("dixon") ? fmpq_mat_solve_dixon(QMY, QMA, QMB) : fmeth("multi_mod") ? fmpq_mat_solve_multi_mod(QMY, QMA, QMB)
             : fmeth("fraction_free") ? fmpq_mat_solve_fraction_free(QMY, QMA, QMB) : fmpq_mat_solve(QMY, QMA, QMB);
    }
    return 1;
}

/* ================================================================ dispatch, export, verification */
static void setup(void) {
    switch (CON) {
    case OURS:
        if (OPK == MM) { bits = (int)ENT; ours_mm_setup(); }
        else if (OPK == SOLVE) ours_solve_setup();
        else ours_vec_setup();
        break;
    case GZ: gz_setup(); break;
    case GQ: gq_setup(); break;
    case FZ: fz_setup(); break;
    case FQ: fq_setup(); break;
    }
}
static int run(void) {
    switch (CON) {
    case OURS:
        if (OPK == MM) { ours_mm_prep(); mmi_run(); mpz_mul(OD, ZdA, ZdB); extra_out = np; return 1; }
        if (OPK == SOLVE) return ours_solve_run();
        if (ORT_FORCE) odot_rt();
        else switch (OLH) { case 1: odot1(); break; case 2: odot2(); break; case 4: odot4(); break; case 8: odot8(); break; default: odot_rt(); }
        mpz_mul(OD, ZdA, ZdB);                         /* the shared denominator of y */
        return 1;
    case GZ: return gz_run();
    case GQ: return gq_run();
    case FZ: return fz_run();
    default: return fq_run();
    }
}
static void export(void) {
    RCNT = cntY(); RN = _fmpz_vec_init(RCNT); RD = _fmpz_vec_init(RCNT);
    for (long i = 0; i < RCNT; i++) {
        fmpz *nu = RN + i, *de = RD + i;
        switch (CON) {
        case OURS:
            if (OPK == MM) { if (!i) limbs_export(); fmpz_set(nu, fmpz_mat_entry(Cz, i / n, i % n)); fmpz_set_mpz(de, OD); }
            else if (OPK == SOLVE) { fmpz_set_mpz(nu, ONUM[i]); fmpz_set_mpz(de, ODEN); }
            else { limbs_to_fz(nu, OY + i * 2 * OLH, OYS[i], 2 * OLH); fmpz_set_mpz(de, OD); }
            break;
        case GZ:
            if (OPK == SOLVE) { fmpz_set_mpz(nu, ONUM[i]); fmpz_set_mpz(de, ODEN); }
            else { fmpz_set_mpz(nu, ZY[i]); fmpz_set_mpz(de, ZD); }
            break;
        case GQ: fmpz_set_mpz(nu, mpq_numref(QY[i])); fmpz_set_mpz(de, mpq_denref(QY[i])); break;
        case FZ:
            if (OPK == SOLVE) fmpz_set(nu, FNUM + i);
            else if (OPK == SPMV || (OPK == MV && !fmeth("mat"))) fmpz_set(nu, FVY + i);
            else fmpz_set(nu, fmpz_mat_entry(FMY, i / fmpz_mat_ncols(FMY), i % fmpz_mat_ncols(FMY)));
            fmpz_set(de, FD);
            break;
        case FQ: {
            fmpq *q = (OPK == SPMV || (OPK == MV && fmeth("vec"))) ? QVY + i : fmpq_mat_entry(QMY, i / fmpq_mat_ncols(QMY), i % fmpq_mat_ncols(QMY));
            fmpz_set(nu, fmpq_numref(q)); fmpz_set(de, fmpq_denref(q)); break; }
        }
    }
}
/* the result against the reference: products y = Y / (dA dB) with Y the integer product */
static int verify(flint_rand_t st) {
    fmpz_t d, t1, t2; fmpz_init(d); fmpz_init(t1); fmpz_init(t2);
    fmpz_mul(d, dA, dB);
    int ok = 1;
    for (long i = 0; i < RCNT && ok; i++) ok = fmpz_sgn(RD + i) > 0;
    if (!ok) goto done;
    if (OPK == MV || OPK == SPMV) {
        for (long i = 0; i < n && ok; i++) {
            fmpz_mul(t1, RN + i, d); fmpz_mul(t2, OPK == MV ? fmpz_mat_entry(Yref, i, 0) : SYref + i, RD + i);
            ok = fmpz_equal(t1, t2);
        }
    } else if (OPK == MM) {                              /* C d must be an integer matrix equal to A B */
        fmpz_mat_t VC; fmpz_mat_init(VC, n, n);
        for (long i = 0; i < n * n && ok; i++) {
            fmpz_mul(t1, RN + i, d);
            ok = fmpz_divisible(t1, RD + i);
            if (ok) fmpz_divexact(fmpz_mat_entry(VC, i / n, i % n), t1, RD + i);
        }
        ok = ok && (n > 128 || fmpz_mat_equal(VC, Cref)) && freivalds(VC, st);
        fmpz_mat_clear(VC);
    } else {                                           /* A x == b: dB sum_j A_ij X_j == dA b_i L, X = x L */
        fmpz_t L; fmpz_init_set_ui(L, 1);
        for (long i = 0; i < n; i++) fmpz_lcm(L, L, RD + i);
        fmpz *X = _fmpz_vec_init(n);
        for (long j = 0; j < n; j++) { fmpz_mul(t1, RN + j, L); fmpz_divexact(X + j, t1, RD + j); }
        for (long i = 0; i < n && ok; i++) {
            _fmpz_vec_dot(t1, A->rows[i], X, n); fmpz_mul(t1, t1, dB);
            fmpz_mul(t2, fmpz_mat_entry(B, i, 0), dA); fmpz_mul(t2, t2, L);
            ok = fmpz_equal(t1, t2);
        }
        _fmpz_vec_clear(X, n); fmpz_clear(L);
    }
done:
    fmpz_clear(d); fmpz_clear(t1); fmpz_clear(t2);
    return ok;
}
/* OUTFILE: "op n W k" / "dA dB" / A (n*n entries, or for spmv rowptr (n+1), colidx and values (nnz)) / B (n*n or n)
   / one "num den" line per result entry; integers in decimal, one per line */
static void write_out(const char *path) {
    FILE *f = fopen(path, "w"); if (!f) { perror(path); exit(2); }
    const char *ops[] = { "mv", "mm", "spmv", "solve" };
    fprintf(f, "%s %ld %d %ld\n", ops[OPK], n, W, KNZ);
    fmpz_fprint(f, dA); fputc(' ', f); fmpz_fprint(f, dB); fputc('\n', f);
    if (OPK == SPMV) {
        for (long i = 0; i <= n; i++) fprintf(f, "%ld\n", SRP[i]);
        for (long t = 0; t < NNZ; t++) fprintf(f, "%d\n", SCI[t]);
    }
    for (long t = 0; t < cntA(); t++) { fmpz_fprint(f, srcA(t)); fputc('\n', f); }
    for (long t = 0; t < cntB(); t++) { fmpz_fprint(f, srcB(t)); fputc('\n', f); }
    for (long i = 0; i < RCNT; i++) { fmpz_fprint(f, RN + i); fputc(' ', f); fmpz_fprint(f, RD + i); fputc('\n', f); }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 6) { fprintf(stderr, "usage: p2_linalg OP CONTENDER N W REPS [OUTFILE]\n"); return 2; }
    const char *ops[] = { "mv", "mm", "spmv", "solve" }, *cons[] = { "ours", "gmp_mpz", "gmp_mpq", "flint_fmpz", "flint_fmpq" };
    OPK = CON = -1;
    for (int i = 0; i < 4; i++) if (is(argv[1], ops[i])) OPK = i;
    for (int i = 0; i < 5; i++) if (is(argv[2], cons[i])) CON = i;
    n = atol(argv[3]); W = atoi(argv[4]); int reps = atoi(argv[5]);
    if (OPK < 0 || CON < 0) { fprintf(stderr, "unknown op or contender\n"); return 2; }
    if (n < 1 || reps < 1 || W < 128 || W > 2048 || (W & (W - 1))) { fprintf(stderr, "bad arguments\n"); return 2; }
    EDGE = getenv("P2_EDGE") ? atoi(getenv("P2_EDGE")) : 0;
    SDROP = getenv("P2_DROP") != NULL;
    ORT_FORCE = getenv("P2_RT") != NULL;
    KNZ = OPK == SPMV ? (getenv("P2_K") ? atol(getenv("P2_K")) : (n / 100 > 1 ? n / 100 : 1)) : 0;
    if (OPK == SPMV && (KNZ < 1 || KNZ > n)) { fprintf(stderr, "bad K\n"); return 2; }
    ENT = OPK == SOLVE ? W / 8 : OPK == SPMV ? (W - clog2(KNZ)) / 2 - 1 : (W - clog2(n)) / 2 - 1;
    bound(ENT >= 1 && ENT <= 4096, "entry bits");
    choose_method();
    scipy_openblas_set_num_threads(1);
    flint_rand_t st; flint_randinit(st);
    mpz_init(gz);
    generate();
    mpz_init(ZdA); mpz_init(ZdB); fmpz_get_mpz(ZdA, dA); fmpz_get_mpz(ZdB, dB);
    fmpz_mat_init(Cz, n, n); fmpz_mat_init(Cref, n, n);
    /* references, before the memory baseline */
    if (OPK == MV) { fmpz_mat_init(Yref, n, 1); fmpz_mat_mul_classical(Yref, A, B); }
    if (OPK == SPMV) { SYref = _fmpz_vec_init(n); for (long i = 0; i < n; i++) for (long t = SRP[i]; t < SRP[i + 1]; t++) fmpz_addmul(SYref + i, SV + t, srcB(SCI[t])); }
    if (OPK == MM && n <= 128) fmpz_mat_mul_classical(Cref, A, B);
    double mem0 = hwm_mb();
    setup();
    double *t = malloc(sizeof(double) * reps);
    for (int r = -1; r < reps; r++) {
        double t0 = now();
        int ok = run();
        double dt = now() - t0;
        if (!ok) { fprintf(stderr, "singular\n"); return 4; }
        if (r >= 0) t[r] = dt;
    }
    double mem1 = hwm_mb();
    export();
    if (getenv("P2_CORRUPT")) fmpz_add_ui(RN + RCNT / 2, RN + RCNT / 2, 1);
    if (!verify(st)) { fprintf(stderr, "mismatch\n"); return 4; }
    qsort(t, reps, sizeof(double), cmpd);
    printf("%.9g %.9g %.1f %ld %s\n", t[reps / 2], t[0], mem1 - mem0, extra_out, *FMETH ? FMETH : "-");
    if (argc > 6) write_out(argv[6]);
    return 0;
}
