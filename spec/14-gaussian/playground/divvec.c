/* Study 25: Gaussian division in AVX2 lanes, int32 halves, separate re/im arrays (divvec.c)
 *
 * x = a + bi, y = c + di, halves in the full int32 range (decision 12 allows -2^31).
 * q = x conj(y) / N(y): X = ac + bd, Y = bc - ad, N = c^2 + d^2, all in 64 bits.
 *   X lies in [-2^63 + 2^32, 2^63]; 2^63 only when a = b = c = d = -2^31, where the 64-bit sum wraps to -2^63.
 *   Y lies in [-2^63 + 2^31, 2^63 - 2^31], N in [0, 2^63].  |X|/N, |Y|/N <= |x|/|y| <= 2^31.5 < 2^32.
 * Floor by the one's complement: s = [X < 0] as a mask (the wrapped 2^63 counts as positive: s = (X - 1 < -1)),
 *   U = X xor s (X, or |X| - 1), then q = floor(U / N) xor s, and the floor remainder
 *   rho = U mod N, or N - 1 - (U mod N) where s.  U <= 2^63 and q < 2^32 in magnitude.
 * Modes:  EX  exact division (y divides x), q only
 *         FL  the floor core: q = floor parts, r = x - q y (mod 2^32)
 *         nearest rules: q0 = floor q, rho the floor remainder; round up where 2 rho > N; at a tie (2 rho = N):
 *         NA  rule A, ties up (= floor((2X + N) / (2N)) per part), the control
 *         B1  ties away from zero        B2  ties toward zero
 *         B3  ties to even               B4  ties to odd
 *         B5  coupled: a tied part rounds away from zero if the other part, rounded away from zero, is even,
 *             toward zero if it is odd (the B1..B5 rules all commute with -1, i and conjugation)
 * Methods (varying divisor per lane unless "shared"):
 *   m1    scalar hardware divide (two 64-bit div per element), the control
 *   m2    exact only: strip 2^k and one (1+i) so N is odd, Newton inverse of N mod 2^32, q = x' conj(y') N^-1 mod 2^32
 *   m3    non-restoring shift-and-subtract in 64-bit lanes, width W chosen per register (all 16 quotient parts < 2^W)
 *   m4    shared divisor: precomputed m = floor(2^(L+32) / N) (L = bitlen(N - 1)), estimate q - 1 <= q^ <= q, one correction
 *   m5    per-lane reciprocal: normalise N, integer Newton reciprocal of its top 32 bits, estimate, one correction
 *   m5x   m5 with two registers per pass, every stage interleaved (the chain per register is ~160 cycles)
 * Flags returned (sticky over the array): bit 0 = some q part (or, for the nearest rules, some r part) equals -2^31, a superset of
 *   true overflow (true value +2^31); resolve() decides exactly, once per array, only when bit 0 is set.
 *   bit 1 = some divisor is zero (its outputs are unspecified).
 * Arrays are padded to a multiple of 8 elements.
 *
 * Build: gcc -O2 -mavx2 -o divvec divvec.c
 * Run:   ./divvec check | ./divvec recip | ./divvec time | ./divvec mem | ./divvec thresh | ./divvec width
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef int32_t i32; typedef uint32_t u32; typedef int64_t i64; typedef uint64_t u64;
typedef __int128 i128; typedef unsigned __int128 u128;
typedef __m256i V;
#define NOINL __attribute__((noinline))
#define AI static inline __attribute__((always_inline))
#define IMIN INT32_MIN
#define IMAX INT32_MAX
enum { EX, FL, NA, B1, B2, B3, B4, B5 };
/* form flags on the mode: MAG = magnitude form (m = sign mask, |u| = (u xor m) - m, divide and round the magnitude,
 * q = (q' xor m) - m); NOS = no sign handling at all (only correct for X, Y >= 0; timing only) */
#define MAG 16
#define NOS 32
#define RULE(m) ((m) & 15)
#define NEAR(m) (RULE(m) >= NA)

#define ARGS const i32 *restrict pa, const i32 *restrict pb, const i32 *restrict pc, const i32 *restrict pd, \
             i32 *restrict pqr, i32 *restrict pqi, i32 *restrict prr, i32 *restrict pri, size_t n
#define PASS pa, pb, pc, pd, pqr, pqi, prr, pri, n
typedef u32 (*kf)(ARGS);

static inline u64 div128(u64 hi, u64 lo, u64 d) {      /* floor((hi 2^64 + lo) / d), needs hi < d */
    u64 q, r; __asm__("divq %4" : "=a"(q), "=d"(r) : "a"(lo), "d"(hi), "rm"(d)); return q;
}

/* ======================= m1: scalar hardware divide ======================= */
AI u32 s_out(i32 a, i32 b, i32 c, i32 d, u32 q1, u32 q2, i32 *qr, i32 *qi, i32 *rr, i32 *ri, int mode);
AI u32 s_div(i32 a, i32 b, i32 c, i32 d, u64 N, i32 *qr, i32 *qi, i32 *rr, i32 *ri, int mode) {
    u64 X = (u64)((i64)a * c) + (u64)((i64)b * d), Y = (u64)((i64)b * c) - (u64)((i64)a * d);
    u64 sx = -(u64)((i64)(X - 1) < -1), sy = -(u64)((i64)Y < 0);
    if (mode & NOS) sx = sy = 0;
    __asm__("" : "+r"(sx), "+r"(sy));        /* keep the masks as masks (gcc otherwise may keep a byte flag in memory) */
    if (mode & (MAG | NOS)) {                /* magnitude form */
        u64 ux = (X ^ sx) - sx, uy = (Y ^ sy) - sy;
        u64 qx = ux / N, qy = uy / N;
        u32 q1 = (u32)qx, q2 = (u32)qy;
        if (NEAR(mode)) {
            u64 rx = ux % N, ry = uy % N, hx, hy;
            int R_ = RULE(mode);
            if (R_ == NA || R_ == B1 || R_ == B5) hx = hy = (N - 1) >> 1;      /* magnitude ties up = away from zero */
            else if (R_ == B2) hx = hy = N >> 1;
            else if (R_ == B3) { hx = (N - (q1 & 1)) >> 1; hy = (N - (q2 & 1)) >> 1; }
            else { hx = (N - 1 + (q1 & 1)) >> 1; hy = (N - 1 + (q2 & 1)) >> 1; }
            u32 ux1 = (u32)((hx - rx) >> 63), uy1 = (u32)((hy - ry) >> 63);
            if (R_ == B5) {
                u32 tx = (u32)(((N >> 1) - rx) >> 63) ^ ux1, ty = (u32)(((N >> 1) - ry) >> 63) ^ uy1;
                u32 ox = (q1 + ux1) & 1, oy = (q2 + uy1) & 1;
                ux1 ^= tx & oy; uy1 ^= ty & ox;
            }
            q1 += ux1; q2 += uy1;
        }
        q1 = (q1 ^ (u32)sx) - (u32)sx; q2 = (q2 ^ (u32)sy) - (u32)sy;
        return s_out(a, b, c, d, q1, q2, qr, qi, rr, ri, mode);
    }
    u64 ux = X ^ sx, uy = Y ^ sy;
    u64 qx = ux / N, qy = uy / N;
    u32 q1 = (u32)(qx ^ sx), q2 = (u32)(qy ^ sy);
    if (NEAR(mode)) {
        u64 rx = ux % N, ry = uy % N;
        rx = (rx ^ sx) + (N & sx); ry = (ry ^ sy) + (N & sy);
        u64 hx, hy;                                                   /* round up where rho > h */
        if (RULE(mode) == NA) hx = hy = (N - 1) >> 1;
        else if (RULE(mode) == B1 || RULE(mode) == B5) { hx = (N - 1 - sx) >> 1; hy = (N - 1 - sy) >> 1; }
        else if (RULE(mode) == B2) { hx = (N + sx) >> 1; hy = (N + sy) >> 1; }
        else if (RULE(mode) == B3) { hx = (N - (q1 & 1)) >> 1; hy = (N - (q2 & 1)) >> 1; }
        else { hx = (N - 1 + (q1 & 1)) >> 1; hy = (N - 1 + (q2 & 1)) >> 1; }
        u32 ux1 = (u32)((hx - rx) >> 63), uy1 = (u32)((hy - ry) >> 63);   /* rho > h; both below 2^63 */
        if (RULE(mode) == B5) {        /* toward zero instead where tied and the other part (away) is odd */
            u32 tx = (u32)((((N + sx) >> 1) - rx) >> 63) ^ ux1, ty = (u32)((((N + sy) >> 1) - ry) >> 63) ^ uy1;
            u32 ox = (q1 + ux1) & 1, oy = (q2 + uy1) & 1;
            ux1 ^= tx & oy; uy1 ^= ty & ox;
        }
        q1 += ux1; q2 += uy1;
    }
    return s_out(a, b, c, d, q1, q2, qr, qi, rr, ri, mode);
}
AI u32 s_out(i32 a, i32 b, i32 c, i32 d, u32 q1, u32 q2, i32 *qr, i32 *qi, i32 *rr, i32 *ri, int mode) {
    *qr = (i32)q1; *qi = (i32)q2;
    u32 f = (q1 == 0x80000000u) | (q2 == 0x80000000u);
    if (RULE(mode) != EX) {
        u32 r1 = (u32)a - (q1 * (u32)c - q2 * (u32)d), r2 = (u32)b - (q1 * (u32)d + q2 * (u32)c);
        *rr = (i32)r1; *ri = (i32)r2;
        if (NEAR(mode)) f |= (r1 == 0x80000000u) | (r2 == 0x80000000u);
    }
    return f;
}
AI u32 m1_body(ARGS, int mode, int shared) {
    u32 f = 0;
    u64 Ns = (u64)((i64)pc[0] * pc[0]) + (u64)((i64)pd[0] * pd[0]);
    if (shared) { f |= (Ns == 0) << 1; Ns |= Ns == 0; }
    for (size_t i = 0; i < n; i++) {
        i32 c = shared ? pc[0] : pc[i], d = shared ? pd[0] : pd[i];
        u64 N = Ns;
        if (!shared) { N = (u64)((i64)c * c) + (u64)((i64)d * d); u64 z = N == 0; f |= (u32)z << 1; N |= z; }
        f |= s_div(pa[i], pb[i], c, d, N, pqr + i, pqi + i, prr + i, pri + i, mode);
    }
    return f;
}

/* ======================= vector helpers ======================= */
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)
#define OR(a, b) _mm256_or_si256(a, b)
#define AND(a, b) _mm256_and_si256(a, b)
#define ANDN(a, b) _mm256_andnot_si256(a, b)
#define XOR(a, b) _mm256_xor_si256(a, b)
#define ADD32(a, b) _mm256_add_epi32(a, b)
#define SUB32(a, b) _mm256_sub_epi32(a, b)
#define ADD64(a, b) _mm256_add_epi64(a, b)
#define SUB64(a, b) _mm256_sub_epi64(a, b)
#define MULS(a, b) _mm256_mul_epi32(a, b)
#define MULU(a, b) _mm256_mul_epu32(a, b)
#define MULLO(a, b) _mm256_mullo_epi32(a, b)
#define HI32(a) _mm256_srli_epi64(a, 32)
#define GT64(a, b) _mm256_cmpgt_epi64(a, b)
#define EQ32(a, b) _mm256_cmpeq_epi32(a, b)
#define C32(x) _mm256_set1_epi32((int)(x))
#define C64(x) _mm256_set1_epi64x((long long)(x))
#define PACK(e, o) _mm256_blend_epi32(e, o, 0xAA)                 /* even dwords from e, odd dwords from o */
#define PACKQ(e, o) _mm256_blend_epi32(e, _mm256_slli_epi64(o, 32), 0xAA)

/* front end: 8 elements -> U[0..3] (re even, re odd, im even, im odd), sign masks S[0..3], N[0..1] (even, odd) */
AI void front(V a, V b, V c, V d, int shared, V Nsh, V N[2], V U[4], V S[4], V *zf, int mode) {
    const V zero = _mm256_setzero_si256(), one = C64(1), m1 = C64(-1);
    V ao = HI32(a), bo = HI32(b), co = shared ? c : HI32(c), dO = shared ? d : HI32(d);
    V X0 = ADD64(MULS(a, c), MULS(b, d)), X1 = ADD64(MULS(ao, co), MULS(bo, dO));
    V Y0 = SUB64(MULS(b, c), MULS(a, d)), Y1 = SUB64(MULS(bo, co), MULS(ao, dO));
    if (shared) { N[0] = N[1] = Nsh; }
    else {
        V N0 = ADD64(MULS(c, c), MULS(d, d)), N1 = ADD64(MULS(co, co), MULS(dO, dO));
        V z0 = _mm256_cmpeq_epi64(N0, zero), z1 = _mm256_cmpeq_epi64(N1, zero);
        N[0] = OR(N0, AND(z0, one)); N[1] = OR(N1, AND(z1, one)); *zf = OR(*zf, OR(z0, z1));
    }
    if (mode & NOS) { S[0] = S[1] = S[2] = S[3] = zero; U[0] = X0; U[1] = X1; U[2] = Y0; U[3] = Y1; return; }
    S[0] = GT64(m1, SUB64(X0, one)); S[1] = GT64(m1, SUB64(X1, one));
    S[2] = GT64(zero, Y0); S[3] = GT64(zero, Y1);
    if (mode & MAG) {   /* |u| = (u xor m) - m */
        U[0] = SUB64(XOR(X0, S[0]), S[0]); U[1] = SUB64(XOR(X1, S[1]), S[1]); U[2] = SUB64(XOR(Y0, S[2]), S[2]); U[3] = SUB64(XOR(Y1, S[3]), S[3]);
        return;
    }
    U[0] = XOR(X0, S[0]); U[1] = XOR(X1, S[1]); U[2] = XOR(Y0, S[2]); U[3] = XOR(Y1, S[3]);
}
/* nearest rules from the core's Q (q_u) and R (U mod N), applied to the packed floor quotients */
AI void nearest_mag(int mode, V N[2], V Q[4], V R[4], V *qre, V *qim) {   /* on magnitudes: R is the remainder */
    V M[4], T[4];
    const V one = C64(1);
    int r = RULE(mode);
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
        V Nj = N[j & 1], H;
        if (r == NA || r == B1 || r == B5) H = _mm256_srli_epi64(SUB64(Nj, one), 1);
        else if (r == B2) H = _mm256_srli_epi64(Nj, 1);
        else if (r == B3) H = _mm256_srli_epi64(SUB64(Nj, AND(Q[j], one)), 1);
        else H = _mm256_srli_epi64(ADD64(SUB64(Nj, one), AND(Q[j], one)), 1);
        M[j] = GT64(R[j], H);
        if (r == B5) T[j] = XOR(M[j], GT64(R[j], _mm256_srli_epi64(Nj, 1)));
    }
    V mr = PACK(M[0], M[1]), mi = PACK(M[2], M[3]);
    if (r == B5) {
        V tr = PACK(T[0], T[1]), ti = PACK(T[2], T[3]);
        V odr = _mm256_srai_epi32(_mm256_slli_epi32(SUB32(*qre, mr), 31), 31);
        V odi = _mm256_srai_epi32(_mm256_slli_epi32(SUB32(*qim, mi), 31), 31);
        mr = XOR(mr, AND(tr, odi)); mi = XOR(mi, AND(ti, odr));
    }
    *qre = SUB32(*qre, mr); *qim = SUB32(*qim, mi);
}
AI void nearest(int mode, V N[2], V Q[4], V R[4], V S[4], V *qre, V *qim) {
    V M[4], T[4];
    const V one = C64(1);
    mode = RULE(mode);
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
        V Nj = N[j & 1];
        V rho = ADD64(XOR(R[j], S[j]), AND(Nj, S[j]));                   /* floor remainder */
        V H;                                                                /* round up where rho > H */
        if (mode == NA) H = _mm256_srli_epi64(SUB64(Nj, one), 1);          /* 2 rho >= N */
        else if (mode == B1 || mode == B5) H = _mm256_srli_epi64(SUB64(SUB64(Nj, one), S[j]), 1);   /* X < 0: N >> 1 */
        else if (mode == B2) H = _mm256_srli_epi64(ADD64(Nj, S[j]), 1);
        else if (mode == B3) H = _mm256_srli_epi64(SUB64(Nj, AND(XOR(Q[j], S[j]), one)), 1);
        else H = _mm256_srli_epi64(ADD64(SUB64(Nj, one), AND(XOR(Q[j], S[j]), one)), 1);
        M[j] = GT64(rho, H);
        if (mode == B5) T[j] = XOR(M[j], GT64(rho, _mm256_srli_epi64(ADD64(Nj, S[j]), 1)));   /* tie mask */
    }
    V mr = PACK(M[0], M[1]), mi = PACK(M[2], M[3]);
    if (mode == B5) {
        V tr = PACK(T[0], T[1]), ti = PACK(T[2], T[3]);
        V odr = _mm256_srai_epi32(_mm256_slli_epi32(SUB32(*qre, mr), 31), 31);   /* away-rounded part odd */
        V odi = _mm256_srai_epi32(_mm256_slli_epi32(SUB32(*qim, mi), 31), 31);
        mr = XOR(mr, AND(tr, odi)); mi = XOR(mi, AND(ti, odr));
    }
    *qre = SUB32(*qre, mr); *qim = SUB32(*qim, mi);
}
/* back end: q = q_u xor s, packed; r = x - q y mod 2^32; flags */
AI u32 back(int mode, V a, V b, V c, V d, V Q[4], V S[4], V N[2], V R[4], i32 *qr, i32 *qi, i32 *rr, i32 *ri) {
    V qre, qim;
    if (mode & (MAG | NOS)) {
        qre = PACKQ(Q[0], Q[1]); qim = PACKQ(Q[2], Q[3]);
        if (NEAR(mode)) nearest_mag(mode, N, Q, R, &qre, &qim);
        if (mode & MAG) { V sr = PACK(S[0], S[1]), si = PACK(S[2], S[3]); qre = SUB32(XOR(qre, sr), sr); qim = SUB32(XOR(qim, si), si); }
    } else {
        qre = XOR(PACKQ(Q[0], Q[1]), PACK(S[0], S[1])); qim = XOR(PACKQ(Q[2], Q[3]), PACK(S[2], S[3]));
        if (NEAR(mode)) nearest(mode, N, Q, R, S, &qre, &qim);
    }
    ST(qr, qre); ST(qi, qim);
    const V mn = C32(IMIN);
    V f = OR(EQ32(qre, mn), EQ32(qim, mn));
    if (RULE(mode) != EX) {
        V rre = ADD32(SUB32(a, MULLO(qre, c)), MULLO(qim, d));
        V rim = SUB32(SUB32(b, MULLO(qre, d)), MULLO(qim, c));
        ST(rr, rre); ST(ri, rim);
        if (NEAR(mode)) f = OR(f, OR(EQ32(rre, mn), EQ32(rim, mn)));
    }
    return (u32)_mm256_movemask_epi8(f);
}

/* ======================= m3: non-restoring shift-and-subtract ======================= */
static const int wseq[9] = {1, 2, 3, 4, 6, 8, 12, 16, 24};
static int g_W = 32;          /* forced width for the fixed-width timing kernel */
static long w_hist[33];       /* chosen widths, counted by the instrumented copy */
/* one step: m = [R < 0]; bring down the next dividend bit from L; append m to L; R = 2R + bit -+ N */
#define NR_STEP(R, L, nN) { V m_ = GT64(zero, R); V b_ = _mm256_srli_epi64(L, 63);                        \
    L = SUB64(ADD64(L, L), m_); R = ADD64(ADD64(R, R), ADD64(SUB64(b_, m_), XOR(nN, m_))); }
AI void m3_core(V U[4], V N[2], V Q[4], V R[4], int wmode, int count) {
    const V zero = _mm256_setzero_si256();
    V Nm0 = SUB64(N[0], C64(1)), Nm1 = SUB64(N[1], C64(1));
    int W = 32;
    if (wmode == 0) {                                  /* smallest W in wseq with every U >> W < N */
        _Pragma("GCC unroll 9") for (int k = 0; k < 9; k++) {
            __m128i cw = _mm_cvtsi32_si128(wseq[k]);
            V f = OR(OR(GT64(_mm256_srl_epi64(U[0], cw), Nm0), GT64(_mm256_srl_epi64(U[1], cw), Nm1)),
                     OR(GT64(_mm256_srl_epi64(U[2], cw), Nm0), GT64(_mm256_srl_epi64(U[3], cw), Nm1)));
            if (_mm256_testz_si256(f, f)) { W = wseq[k]; break; }
        }
    } else W = wmode;
    if (count) w_hist[W]++;
    __m128i cw = _mm_cvtsi32_si128(W), cl = _mm_cvtsi32_si128(64 - W);
    V nN0 = SUB64(zero, N[0]), nN1 = SUB64(zero, N[1]);
    V R0 = _mm256_srl_epi64(U[0], cw), R1 = _mm256_srl_epi64(U[1], cw), R2 = _mm256_srl_epi64(U[2], cw), R3 = _mm256_srl_epi64(U[3], cw);
    V L0 = _mm256_sll_epi64(U[0], cl), L1 = _mm256_sll_epi64(U[1], cl), L2 = _mm256_sll_epi64(U[2], cl), L3 = _mm256_sll_epi64(U[3], cl);
    for (int j = 0; j < W; j++) { NR_STEP(R0, L0, nN0) NR_STEP(R1, L1, nN1) NR_STEP(R2, L2, nN0) NR_STEP(R3, L3, nN1) }
    V mask = C64((1ull << W) - 1);
    V Rr[4] = {R0, R1, R2, R3}, Lr[4] = {L0, L1, L2, L3};
    _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
        V m = GT64(zero, Rr[j]);
        Q[j] = ANDN(SUB64(ADD64(Lr[j], Lr[j]), m), mask);          /* quotient bits are the complemented signs */
        R[j] = ADD64(Rr[j], AND(N[j & 1], m));
    }
}
AI u32 m3_body(ARGS, int mode, int shared, int wmode, int count) {
    V zf = _mm256_setzero_si256(); u32 f = 0;
    V cs = C32(pc[0]), ds = C32(pd[0]);
    V Nsh = C64((u64)((i64)pc[0] * pc[0]) + (u64)((i64)pd[0] * pd[0]));
    if (shared) { V z = _mm256_cmpeq_epi64(Nsh, _mm256_setzero_si256()); Nsh = OR(Nsh, AND(z, C64(1))); zf = z; }
    for (size_t i = 0; i < n; i += 8) {
        V a = LD(pa + i), b = LD(pb + i), c = shared ? cs : LD(pc + i), d = shared ? ds : LD(pd + i);
        V N[2], U[4], S[4], Q[4], R[4];
        front(a, b, c, d, shared, Nsh, N, U, S, &zf, mode);
        m3_core(U, N, Q, R, wmode, count);
        f |= back(mode, a, b, c, d, Q, S, N, R, pqr + i, pqi + i, prr + i, pri + i);
    }
    return (f ? 1 : 0) | (_mm256_testz_si256(zf, zf) ? 0 : 2);
}

/* ======================= m4: shared divisor, precomputed reciprocal ======================= */
/* L = bitlen(N - 1) so N in (2^(L-1), 2^L]; K = L + 32; m = floor(2^K / N) in [2^32, 2^33), m' = m - 2^32;
 * s = max(0, L - 32); U' = U >> s; S = floor(U' m / 2^32) = U' + (U' >> 32) m' + ((U' mod 2^32) m' >> 32);
 * q^ = S >> (L - s).  q - 1 <= q^ <= q (study text), so one correction: rho = U - q^ N, if rho >= N subtract. */
#define EST_CORR(U, q, rho, N, Nhi, Nm1b) {                                                                  \
    V qn_ = ADD64(MULU(q, N), _mm256_slli_epi64(MULU(q, Nhi), 32));                                       \
    rho = SUB64(U, qn_);                                                                                  \
    V ge_ = GT64(XOR(rho, bias), Nm1b);                                                                   \
    rho = SUB64(rho, AND(ge_, N)); q = SUB64(q, ge_); }
AI u32 m4_body(ARGS, int mode) {
    u32 f = 0;
    u64 Ns = (u64)((i64)pc[0] * pc[0]) + (u64)((i64)pd[0] * pd[0]);
    if (Ns == 0) { f |= 2; Ns = 1; }
    int L = Ns == 1 ? 0 : 64 - __builtin_clzll(Ns - 1);
    int K = L + 32, s = L > 32 ? L - 32 : 0;
    u64 m = K >= 64 ? div128(1ull << (K - 64), 0, Ns) : (1ull << K) / Ns;
    const V bias = C64(1ull << 63), mp = C64(m - (1ull << 32)), Nv = C64(Ns), Nhi = C64(Ns >> 32), Nm1b = C64((Ns - 1) ^ (1ull << 63));
    const __m128i cs = _mm_cvtsi32_si128(s), cL = _mm_cvtsi32_si128(L - s);
    V cb = C32(pc[0]), db = C32(pd[0]), zf = _mm256_setzero_si256();
    for (size_t i = 0; i < n; i += 8) {
        V a = LD(pa + i), b = LD(pb + i);
        V N[2], U[4], S[4], Q[4], R[4];
        front(a, b, cb, db, 1, Nv, N, U, S, &zf, mode);
        _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
            V u = _mm256_srl_epi64(U[j], cs);
            V Sm = ADD64(ADD64(u, MULU(HI32(u), mp)), HI32(MULU(u, mp)));
            Q[j] = _mm256_srl_epi64(Sm, cL);
            EST_CORR(U[j], Q[j], R[j], Nv, Nhi, Nm1b)
        }
        f |= back(mode, a, b, cb, db, Q, S, N, R, pqr + i, pqi + i, prr + i, pri + i) ? 1 : 0;
    }
    return f;
}

/* ======================= m5: per-lane reciprocal ======================= */
/* dh in [2^31, 2^32) in the low dword of each 64-bit lane; returns v' = floor(2^64 / (dh + 1)) - 2^32 before fix-ups.
 * Y ~ 2^63 / D (D = dh + 1) from a linear seed (8/3 - 8/9 D/2^32) 2^31, always below, relative error <= 1/9;
 * three Newton steps Y += Y (2^63 - D Y) / 2^63, which stay below; then v' = max(2Y - 2^32, 0) and one Newton
 * step in the 33-bit form v = 2^32 + v': v' += e_h + (v' e_h >> 32), e = 2^64 - D v (mod 2^64), e_h = e >> 32. */
#define RECIP_FIX 2      /* fix-ups after the Newton steps; the exhaustive check ("recip") shows the deficit is at most 2 */
AI V recip_newton(V dh) {
    const V zero = _mm256_setzero_si256();
    const V A = C64(5726623061ull), B = C64(3817748708ull), T63 = C64(1ull << 63);
    V Y = SUB64(SUB64(A, HI32(ADD64(MULU(dh, B), B))), C64(1));
    _Pragma("GCC unroll 9") for (int it = 0; it < 3; it++) {
        V e = SUB64(T63, ADD64(MULU(dh, Y), Y));
        Y = ADD64(Y, HI32(MULU(Y, _mm256_srli_epi64(e, 31))));
    }
    V v = SUB64(ADD64(Y, Y), C64(1ull << 32));
    v = ANDN(GT64(zero, v), v);
    V dhs = _mm256_slli_epi64(dh, 32);
    V e = SUB64(SUB64(SUB64(C64(-(1ll << 32)), dhs), MULU(dh, v)), v);
    V eh = HI32(e);
    v = ADD64(v, ADD64(eh, HI32(MULU(v, eh))));
    return v;
}
AI V recip_fix(V dh, V v, int nfix) {
    V dhs = _mm256_slli_epi64(dh, 32), D = ADD64(dh, C64(1));
    V R = SUB64(SUB64(SUB64(C64(-(1ll << 32)), dhs), MULU(dh, v)), v);   /* 2^64 - D (2^32 + v') */
    _Pragma("GCC unroll 9") for (int k = 0; k < nfix; k++) { V ge = GT64(R, dh); R = SUB64(R, AND(ge, D)); v = SUB64(v, ge); }
    return v;
}
AI u32 m5_body(ARGS, int mode) {
    const V zero = _mm256_setzero_si256(), bias = C64(1ull << 63);
    V zf = zero; u32 f = 0;
    for (size_t i = 0; i < n; i += 8) {
        V a = LD(pa + i), b = LD(pb + i), c = LD(pc + i), d = LD(pd + i);
        V N[2], U[4], S[4], Q[4], R[4], vp[2], cl[2], cr[2], Nhi[2], Nm1b[2];
        front(a, b, c, d, 0, zero, N, U, S, &zf, mode);
        _Pragma("GCC unroll 9") for (int h = 0; h < 2; h++) {
            V Nn = N[h], sh = zero;                             /* normalise: Nn = N << sh, top bit set */
            _Pragma("GCC unroll 9") for (int k = 32; k >= 1; k >>= 1) {
                V ks = AND(_mm256_cmpeq_epi64(_mm256_srli_epi64(Nn, 64 - k), zero), C64(k));
                Nn = _mm256_sllv_epi64(Nn, ks); sh = ADD64(sh, ks);
            }
            V dh = HI32(Nn);
            vp[h] = recip_fix(dh, recip_newton(dh), RECIP_FIX);
            cl[h] = SUB64(sh, C64(32)); cr[h] = SUB64(C64(32), sh);   /* out-of-range counts shift to 0 */
            Nhi[h] = HI32(N[h]); Nm1b[h] = XOR(SUB64(N[h], C64(1)), bias);
        }
        _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++) {
            int h = j & 1;
            V th = OR(_mm256_sllv_epi64(U[j], cl[h]), _mm256_srlv_epi64(U[j], cr[h]));   /* floor(U 2^sh / 2^32) */
            V Sm = ADD64(ADD64(th, MULU(HI32(th), vp[h])), HI32(MULU(th, vp[h])));
            Q[j] = HI32(Sm);
            EST_CORR(U[j], Q[j], R[j], N[h], Nhi[h], Nm1b[h])
        }
        f |= back(mode, a, b, c, d, Q, S, N, R, pqr + i, pqi + i, prr + i, pri + i) ? 1 : 0;
    }
    return f | (_mm256_testz_si256(zf, zf) ? 0 : 2);
}
/* the same with two registers (16 elements) per pass, every stage interleaved across them: the per-register
 * dependency chain is about 160 cycles long, so one register at a time leaves the out-of-order window short */
AI u32 m5x2_body(ARGS, int mode) {
    const V zero = _mm256_setzero_si256(), bias = C64(1ull << 63);
    V zf = zero; u32 f = 0; size_t i = 0;
    for (; i + 16 <= n; i += 16) {
        V a[2], b[2], c[2], d[2], N[2][2], U[2][4], S[2][4], Q[2][4], R[2][4], vp[4], cl[4], cr[4], Nhi[4], Nm1b[4], Nn[4], sh[4];
        _Pragma("GCC unroll 9") for (int g = 0; g < 2; g++) {
            a[g] = LD(pa + i + 8 * g); b[g] = LD(pb + i + 8 * g); c[g] = LD(pc + i + 8 * g); d[g] = LD(pd + i + 8 * g);
            front(a[g], b[g], c[g], d[g], 0, zero, N[g], U[g], S[g], &zf, mode);
        }
        _Pragma("GCC unroll 9") for (int h = 0; h < 4; h++) { Nn[h] = N[h >> 1][h & 1]; sh[h] = zero; }
        _Pragma("GCC unroll 9") for (int k = 32; k >= 1; k >>= 1)
            _Pragma("GCC unroll 9") for (int h = 0; h < 4; h++) {
                V ks = AND(_mm256_cmpeq_epi64(_mm256_srli_epi64(Nn[h], 64 - k), zero), C64(k));
                Nn[h] = _mm256_sllv_epi64(Nn[h], ks); sh[h] = ADD64(sh[h], ks);
            }
        _Pragma("GCC unroll 9") for (int h = 0; h < 4; h++) {
            V dh = HI32(Nn[h]);
            vp[h] = recip_fix(dh, recip_newton(dh), RECIP_FIX);
            cl[h] = SUB64(sh[h], C64(32)); cr[h] = SUB64(C64(32), sh[h]);
            V Nh = N[h >> 1][h & 1]; Nhi[h] = HI32(Nh); Nm1b[h] = XOR(SUB64(Nh, C64(1)), bias);
        }
        _Pragma("GCC unroll 9") for (int j = 0; j < 4; j++)
            _Pragma("GCC unroll 9") for (int g = 0; g < 2; g++) {
                int h = 2 * g + (j & 1); V Nh = N[g][j & 1];
                V th = OR(_mm256_sllv_epi64(U[g][j], cl[h]), _mm256_srlv_epi64(U[g][j], cr[h]));
                V Sm = ADD64(ADD64(th, MULU(HI32(th), vp[h])), HI32(MULU(th, vp[h])));
                Q[g][j] = HI32(Sm);
                EST_CORR(U[g][j], Q[g][j], R[g][j], Nh, Nhi[h], Nm1b[h])
            }
        _Pragma("GCC unroll 9") for (int g = 0; g < 2; g++)
            f |= back(mode, a[g], b[g], c[g], d[g], Q[g], S[g], N[g], R[g], pqr + i + 8 * g, pqi + i + 8 * g, prr + i + 8 * g, pri + i + 8 * g) ? 1 : 0;
    }
    if (i < n) f |= m5_body(pa + i, pb + i, pc + i, pd + i, pqr + i, pqi + i, prr + i, pri + i, n - i, mode);   /* last 8 */
    return f | (_mm256_testz_si256(zf, zf) ? 0 : 2);
}

/* ======================= m2: exact division by the inverse mod 2^32 ======================= */
/* ctz of 32-bit lanes, integer only: popcount of (o & -o) - 1 by the nibble table (o = 0 gives 32) */
AI V ctz_pop(V o) {
    const V nib = C32(0x0F0F0F0F);
    const V tbl = _mm256_setr_epi8(0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4, 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    V t = SUB32(AND(o, SUB32(_mm256_setzero_si256(), o)), C32(1));
    V cnt = _mm256_add_epi8(_mm256_shuffle_epi8(tbl, AND(t, nib)), _mm256_shuffle_epi8(tbl, AND(_mm256_srli_epi32(t, 4), nib)));
    return _mm256_madd_epi16(_mm256_maddubs_epi16(cnt, C32(0x01010101)), C32(0x00010001));
}
/* x / (1+i) where t (0 or 1 per lane, mask m) says so: ((a >> 1) + (b >> 1) + (a & 1), (b >> 1) - (a >> 1)) */
AI void div1i(V *a, V *b, V t, V m) {
    V x = *a, y = *b;
    *a = ADD32(ADD32(_mm256_srav_epi32(x, t), _mm256_srav_epi32(AND(y, m), t)), AND(x, t));
    *b = SUB32(_mm256_srav_epi32(y, t), _mm256_srav_epi32(AND(x, m), t));
}
AI V inv32(V N) {                                   /* N odd: N^-1 mod 2^32, seed (3N) xor 2 (5 bits), three steps */
    V i = XOR(ADD32(ADD32(N, N), N), C32(2));
    _Pragma("GCC unroll 9") for (int k = 0; k < 3; k++) i = MULLO(i, SUB32(C32(2), MULLO(N, i)));
    return i;
}
AI u32 m2_body(ARGS) {
    const V zero = _mm256_setzero_si256(), mn = C32(IMIN);
    V fz = zero, fq = zero;
    for (size_t i = 0; i < n; i += 8) {
        V a = LD(pa + i), b = LD(pb + i), c = LD(pc + i), d = LD(pd + i);
        V o = OR(c, d); fz = OR(fz, EQ32(o, zero));
        V k = ctz_pop(o);
        a = _mm256_srav_epi32(a, k); b = _mm256_srav_epi32(b, k); c = _mm256_srav_epi32(c, k); d = _mm256_srav_epi32(d, k);
        V m = _mm256_srai_epi32(_mm256_slli_epi32(AND(c, d), 31), 31), t = _mm256_srli_epi32(m, 31);
        div1i(&c, &d, t, m); div1i(&a, &b, t, m);
        V iv = inv32(ADD32(MULLO(c, c), MULLO(d, d)));
        V wr = MULLO(c, iv), wi = MULLO(d, iv);                    /* conj(y') / N(y') = wr - wi i */
        V qre = ADD32(MULLO(a, wr), MULLO(b, wi)), qim = SUB32(MULLO(b, wr), MULLO(a, wi));
        ST(pqr + i, qre); ST(pqi + i, qim);
        fq = OR(fq, OR(EQ32(qre, mn), EQ32(qim, mn)));
    }
    return (_mm256_testz_si256(fq, fq) ? 0 : 1) | (_mm256_testz_si256(fz, fz) ? 0 : 2);
}
/* shared divisor: k, the (1+i) step and w = conj(y') N(y')^-1 once; per element a shift, the (1+i) step, 4 multiplies */
AI u32 m2s_body(ARGS) {
    i32 c = pc[0], d = pd[0]; u32 fl = 0;
    if (!(c | d)) { fl = 2; c = 1; }
    int k = __builtin_ctz((u32)(c | d));
    c >>= k; d >>= k;
    int t = c & d & 1;
    if (t) { i32 hc = c >> 1, hd = d >> 1; c = hc + hd + 1; d = hd - hc; }
    u32 N = (u32)c * (u32)c + (u32)d * (u32)d, iv = 3 * N ^ 2;
    for (int j = 0; j < 3; j++) iv *= 2 - N * iv;
    const V wr = C32((u32)c * iv), wi = C32((u32)d * iv), mn = C32(IMIN), one = C32(1);
    const __m128i ck = _mm_cvtsi32_si128(k);
    V fq = _mm256_setzero_si256();
#define M2S_LOOP(T)                                                                                         \
    for (size_t i = 0; i < n; i += 8) {                                                                      \
        V a = _mm256_sra_epi32(LD(pa + i), ck), b = _mm256_sra_epi32(LD(pb + i), ck);                        \
        if (T) { V ha = _mm256_srai_epi32(a, 1), hb = _mm256_srai_epi32(b, 1);                               \
                 a = ADD32(ADD32(ha, hb), AND(a, one)); b = SUB32(hb, ha); }                                 \
        V qre = ADD32(MULLO(a, wr), MULLO(b, wi)), qim = SUB32(MULLO(b, wr), MULLO(a, wi));                  \
        ST(pqr + i, qre); ST(pqi + i, qim);                                                                  \
        fq = OR(fq, OR(EQ32(qre, mn), EQ32(qim, mn)));                                                       \
    }
    if (t) { M2S_LOOP(1) } else { M2S_LOOP(0) }
    return fl | (_mm256_testz_si256(fq, fq) ? 0 : 1);
}

/* ======================= kernels ======================= */
#define K1(name, body) NOINL u32 name(ARGS) { return body; }
#define KR(m, body3) K1(m##_fl, body3(FL)) K1(m##_na, body3(NA)) K1(m##_b1, body3(B1)) K1(m##_b2, body3(B2)) \
                     K1(m##_b3, body3(B3)) K1(m##_b4, body3(B4)) K1(m##_b5, body3(B5))
#define BM1(md) m1_body(PASS, md, 0)
#define BM1S(md) m1_body(PASS, md, 1)
#define BM3(md) m3_body(PASS, md, 0, 0, 0)
#define BM3S(md) m3_body(PASS, md, 1, 0, 0)
#define BM4(md) m4_body(PASS, md)
#define BM5(md) m5_body(PASS, md)
#define BM5X(md) m5x2_body(PASS, md)
KR(m5x, BM5X) KR(m1, BM1)
#define KM(m, body3) K1(m##_mb1, body3(B1 | MAG)) K1(m##_mb2, body3(B2 | MAG)) K1(m##_mb3, body3(B3 | MAG)) K1(m##_mb4, body3(B4 | MAG)) \
                     K1(m##_mb5, body3(B5 | MAG)) K1(m##_nsf, body3(FL | NOS)) K1(m##_nsa, body3(NA | NOS))
KM(m1, BM1) KM(m3, BM3) KM(m4, BM4) KM(m5x, BM5X) KR(m1s, BM1S) KR(m3, BM3) KR(m3s, BM3S) KR(m4, BM4) KR(m5, BM5)
K1(m1_ex, m1_body(PASS, EX, 0)) K1(m1s_ex, m1_body(PASS, EX, 1)) K1(m2_ex, m2_body(PASS)) K1(m2s_ex, m2s_body(PASS))
K1(m5x_ex, m5x2_body(PASS, EX)) K1(m3_ex, m3_body(PASS, EX, 0, 0, 0)) K1(m3s_ex, m3_body(PASS, EX, 1, 0, 0)) K1(m4_ex, m4_body(PASS, EX)) K1(m5_ex, m5_body(PASS, EX))
K1(m3w_fl, m3_body(PASS, FL, 0, g_W, 0)) K1(m3c_fl, m3_body(PASS, FL, 0, 0, 1))

enum { VAR = 0, SHR = 1 };
typedef struct { const char *name; kf f; int mode, shared; } kern;   /* NOS kernels are timing only (mode < 0: not checked) */
#define KE(m, sh) {#m "_fl", m##_fl, FL, sh}, {#m "_na", m##_na, NA, sh}, {#m "_b1", m##_b1, B1, sh}, {#m "_b2", m##_b2, B2, sh}, \
                  {#m "_b3", m##_b3, B3, sh}, {#m "_b4", m##_b4, B4, sh}, {#m "_b5", m##_b5, B5, sh}
static kern KT[] = {
    {"m1_ex", m1_ex, EX, VAR}, {"m2_ex", m2_ex, EX, VAR}, {"m3_ex", m3_ex, EX, VAR}, {"m5_ex", m5_ex, EX, VAR}, {"m5x_ex", m5x_ex, EX, VAR},
    {"m1s_ex", m1s_ex, EX, SHR}, {"m2s_ex", m2s_ex, EX, SHR}, {"m3s_ex", m3s_ex, EX, SHR}, {"m4_ex", m4_ex, EX, SHR},
    KE(m1, VAR), KE(m3, VAR), KE(m5, VAR), KE(m5x, VAR),
    {"m3w_fl", m3w_fl, FL, VAR}, {"m3c_fl", m3c_fl, FL, VAR},
#define KEM(m, sh) {#m "_mb1", m##_mb1, B1, sh}, {#m "_mb2", m##_mb2, B2, sh}, {#m "_mb3", m##_mb3, B3, sh}, {#m "_mb4", m##_mb4, B4, sh}, \
                   {#m "_mb5", m##_mb5, B5, sh}, {#m "_nsf", m##_nsf, -1, sh}, {#m "_nsa", m##_nsa, -2, sh}
    KEM(m1, VAR), KEM(m3, VAR), KEM(m5x, VAR), KEM(m4, SHR), KE(m1s, SHR), KE(m3s, SHR), KE(m4, SHR),
};
#define NKT (int)(sizeof KT / sizeof KT[0])
#define NMODE 8

/* exact overflow decision, once per array when bit 0 is set (rare): a part computed as -2^31 is truly +2^31 iff
 * its numerator is positive; an r part computed as -2^31 is truly +2^31 iff r computed in 64 bits is. */
NOINL int resolve(ARGS, int mode) {
    int ov = 0;
    for (size_t i = 0; i < n; i++) {
        i64 a = pa[i], b = pb[i], c = pc[i], d = pd[i];
        i64 X = (i64)((u64)(a * c) + (u64)(b * d)), Y = b * c - a * d;
        i64 q1 = pqr[i], q2 = pqi[i];
        if (q1 == IMIN && X > 0) { q1 = -q1; ov = 1; }
        if (q2 == IMIN && Y > 0) { q2 = -q2; ov = 1; }
        if (NEAR(mode)) {
            i64 r1 = a - (q1 * c - q2 * d), r2 = b - (q1 * d + q2 * c);
            if (r1 > IMAX || r2 > IMAX) ov = 1;
        }
    }
    return ov;
}

/* ======================= reference (__int128) ======================= */
static i128 fdiv(i128 x, i128 y) { i128 q = x / y; if ((x % y != 0) && ((x < 0) != (y < 0))) q--; return q; }
typedef struct { i128 q1, q2, r1, r2; int ov; } ref_t;
/* nearest by rule, from the exact rational (independent of the kernels' threshold formulas) */
static ref_t ref(i64 a, i64 b, i64 c, i64 d, int mode) {
    ref_t R; i128 N = (i128)c * c + (i128)d * d, X = (i128)a * c + (i128)b * d, Y = (i128)b * c - (i128)a * d;
    i128 Z[2] = {X, Y}, q[2], fl[2]; int tie[2];
    for (int p = 0; p < 2; p++) {
        fl[p] = fdiv(Z[p], N); i128 rem = Z[p] - fl[p] * N;
        tie[p] = 2 * rem == N; q[p] = mode >= NA ? fl[p] + (2 * rem > N) : fl[p];
    }
    if (mode >= NA) for (int p = 0; p < 2; p++) if (tie[p]) {
        i128 away = Z[p] > 0 ? fl[p] + 1 : fl[p], toward = Z[p] > 0 ? fl[p] : fl[p] + 1;
        if (mode == NA) q[p] = fl[p] + 1;
        else if (mode == B1) q[p] = away;
        else if (mode == B2) q[p] = toward;
        else if (mode == B3) q[p] = (fl[p] & 1) ? fl[p] + 1 : fl[p];
        else if (mode == B4) q[p] = (fl[p] & 1) ? fl[p] : fl[p] + 1;
        else {   /* B5: the other part rounded away from zero */
            int o = 1 - p; i128 oa = tie[o] ? (Z[o] > 0 ? fl[o] + 1 : fl[o]) : q[o];
            q[p] = (oa & 1) ? toward : away;
        }
    }
    R.q1 = q[0]; R.q2 = q[1];
    R.r1 = a - (R.q1 * c - R.q2 * d); R.r2 = b - (R.q1 * d + R.q2 * c);
    R.ov = R.q1 > IMAX || R.q1 < IMIN || R.q2 > IMAX || R.q2 < IMIN;
    if (mode >= NA) R.ov |= R.r1 > IMAX || R.r1 < IMIN || R.r2 > IMAX || R.r2 < IMIN;
    return R;
}

/* ======================= correctness ======================= */
static u64 rs = 0x9E3779B97F4A7C15ULL;
static u64 next(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static i64 rr(i64 lo, i64 hi) { return lo + (i64)(next() % (u64)(hi - lo + 1)); }
static long fails, nchecked, novf, nflagged;
#define BMAX (1 << 16)
static i32 A[BMAX + 8], B[BMAX + 8], C[BMAX + 8], D[BMAX + 8];
static i32 O[4][BMAX + 8];
static int exact_ok(i64 a, i64 b, i64 c, i64 d) {   /* y divides x (y != 0) */
    i128 N = (i128)c * c + (i128)d * d; if (N == 0) return 0;
    return ((i128)a * c + (i128)b * d) % N == 0 && ((i128)b * c - (i128)a * d) % N == 0;
}
/* run every kernel of the right kind on the batch (A..D, n elements); shared kernels only when all divisors equal */
static void check_batch(size_t n, int exact, int flagcheck) {
    size_t N8 = (n + 7) & ~(size_t)7;
    for (size_t j = n; j < N8; j++) { A[j] = B[j] = 0; C[j] = C[0]; D[j] = D[0]; }
    int same = 1; for (size_t j = 1; j < n; j++) if (C[j] != C[0] || D[j] != D[0]) same = 0;
    int anyzero = 0, anyov[NMODE] = {0};
    static ref_t RF[NMODE][BMAX];
    for (size_t j = 0; j < n; j++) {
        if (!C[j] && !D[j]) { anyzero = 1; continue; }
        for (int m = 0; m < NMODE; m++) { if (exact && m) break; if (!exact && !m) continue;
            RF[m][j] = ref(A[j], B[j], C[j], D[j], m); anyov[m] |= RF[m][j].ov; }
    }
    for (int k = 0; k < NKT; k++) {
        if (KT[k].mode < 0) continue;
        if ((KT[k].mode == EX) != exact) continue;
        if (KT[k].shared && !same) continue;
        int m = KT[k].mode;
        u32 fl = KT[k].f(A, B, C, D, O[0], O[1], O[2], O[3], N8);
        nchecked += n;
        for (size_t j = 0; j < n; j++) {
            if (!C[j] && !D[j]) continue;
            ref_t R = RF[m][j]; int bad = 0;
            if ((u32)O[0][j] != (u32)(u64)R.q1 || (u32)O[1][j] != (u32)(u64)R.q2) bad = 1;
            if (m != EX && ((u32)O[2][j] != (u32)(u64)R.r1 || (u32)O[3][j] != (u32)(u64)R.r2)) bad = 1;
            if (bad && fails++ < 20) printf("FAIL %s (%d,%d)/(%d,%d): got q (%d,%d) r (%d,%d), want q (%lld,%lld) r (%lld,%lld)\n",
                KT[k].name, A[j], B[j], C[j], D[j], O[0][j], O[1][j], O[2][j], O[3][j],
                (long long)R.q1, (long long)R.q2, (long long)R.r1, (long long)R.r2);
        }
        if (((fl >> 1) & 1) != (u32)anyzero && fails++ < 20) printf("FAIL %s zero flag\n", KT[k].name);
        if (anyov[m] && !(fl & 1) && fails++ < 20) printf("FAIL %s missed overflow (%d,%d)/(%d,%d)\n", KT[k].name, A[0], B[0], C[0], D[0]);
        if (flagcheck && !anyzero) {
            int ex = (fl & 1) ? resolve(A, B, C, D, O[0], O[1], O[2], O[3], n, m) : 0;
            if (ex != anyov[m] && fails++ < 20) printf("FAIL %s resolve (%d,%d)/(%d,%d)\n", KT[k].name, A[0], B[0], C[0], D[0]);
            if (k == 0 || KT[k].f == m1_fl || KT[k].f == m1_na) { novf += anyov[m]; nflagged += fl & 1; }
        }
    }
}
static size_t nb; static int bexact, bflag;
static void push(i64 a, i64 b, i64 c, i64 d) {
    A[nb] = (i32)a; B[nb] = (i32)b; C[nb] = (i32)c; D[nb] = (i32)d; nb++;
    if (nb == BMAX) { check_batch(nb, bexact, bflag); nb = 0; }
}
static void flush(void) { if (nb) check_batch(nb, bexact, bflag); nb = 0; }
/* one element alone in a batch of 8 (position rotating), so the flags are per element; shared kernels run too */
static int rot;
static void alone(i64 a, i64 b, i64 c, i64 d, int exact) {
    for (int j = 0; j < 8; j++) { A[j] = 0; B[j] = 0; C[j] = (i32)c; D[j] = (i32)d; }
    int p = rot++ & 7; A[p] = (i32)a; B[p] = (i32)b;
    check_batch(8, exact, 1);
}
static const i64 EV[] = {IMIN, IMIN + 1, -(1 << 30) - 1, -(1 << 30), -46341, -65536, -3, -2, -1, 0,
                         1, 2, 3, 65536, 46341, 1 << 30, (1 << 30) + 1, IMAX - 1, IMAX, IMIN + 2};
#define NEV (int)(sizeof EV / sizeof EV[0])
static int fits(i64 v) { return v >= IMIN && v <= IMAX; }

/* N(r) <= N(y)/2 for every nearest rule, and for rule A: r depends only on x mod y (r(x + k y) = r(x)) */
static long prop_fail, prop_n;
static const int RULES[6] = {NA, B1, B2, B3, B4, B5};
static const char *RNAME[6] = {"A ties up", "B1 away from 0", "B2 toward 0", "B3 to even", "B4 to odd", "B5 coupled"};
static kf rule_kernels[6][3] = {{m1_na, m3_na, m5_na}, {m1_b1, m3_b1, m5_b1}, {m1_b2, m3_b2, m5_b2},
                                {m1_b3, m3_b3, m5_b3}, {m1_b4, m3_b4, m5_b4}, {m1_b5, m3_b5, m5_b5}};
static void prop_one(i64 a, i64 b, i64 c, i64 d) {
    if (!(c | d)) return;
    i128 Ny = (i128)c * c + (i128)d * d;
#define CALL(k, x1, x2, y1, y2, o) { for (int j = 0; j < 8; j++) { A[j] = (i32)(x1); B[j] = (i32)(x2); C[j] = (i32)(y1); D[j] = (i32)(y2); } \
        k(A, B, C, D, O[0], O[1], O[2], O[3], 8); o[0] = O[0][3]; o[1] = O[1][3]; o[2] = O[2][3]; o[3] = O[3][3]; }
    for (int u = 0; u < 6; u++) for (int t = 0; t < 3; t++) {
        i32 r0[4], r1[4]; ref_t R = ref(a, b, c, d, RULES[u]);
        if (R.ov) continue;
        CALL(rule_kernels[u][t], a, b, c, d, r0)
        prop_n++;
        i128 nr = (i128)r0[2] * r0[2] + (i128)r0[3] * r0[3];
        if (2 * nr > Ny) prop_fail++;
        if (u) continue;
        i64 ks[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {1, 1}};
        for (int v = 0; v < 4; v++) {
            i64 x1 = a + ks[v][0] * c - ks[v][1] * d, x2 = b + ks[v][0] * d + ks[v][1] * c;
            if (!fits(x1) || !fits(x2)) continue;
            CALL(rule_kernels[0][t], x1, x2, c, d, r1)
            prop_n++; if (r1[2] != r0[2] || r1[3] != r0[3]) prop_fail++;
        }
    }
}

static void do_check(void) {
    /* exhaustive, K-bit two's complement halves */
    for (int K = 3; K <= 6; K++) {
        i64 lo = -(1 << (K - 1)), hi = (1 << (K - 1)) - 1; long cnt = 0;
        bexact = 0; bflag = 1;
        for (i64 a = lo; a <= hi; a++) for (i64 b = lo; b <= hi; b++) for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++) { push(a, b, c, d); cnt++; }
        flush();
        long ce = 0; bexact = 1;
        for (i64 q1 = lo; q1 <= hi; q1++) for (i64 q2 = lo; q2 <= hi; q2++) for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++)
            if (c | d) { push(q1 * c - q2 * d, q1 * d + q2 * c, c, d); ce++; }
        flush();
        /* shared divisor, exhaustive at this size: one batch per divisor */
        for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++) {
            if (!(c | d)) continue;
            bexact = 0; for (i64 a = lo; a <= hi; a++) for (i64 b = lo; b <= hi; b++) push(a, b, c, d); flush();
            bexact = 1; for (i64 q1 = lo; q1 <= hi; q1++) for (i64 q2 = lo; q2 <= hi; q2++) push(q1 * c - q2 * d, q1 * d + q2 * c, c, d); flush();
        }
        printf("K = %d: exhaustive %ld divisions with remainder, %ld exact divisions (also per shared divisor); failures so far %ld\n", K, cnt, ce, fails);
    }
    /* edges: every quadruple of the 20 edge values, each alone (flags per element) */
    long ne = 0, nx = 0;
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) for (int r = 0; r < NEV; r++) for (int s = 0; s < NEV; s++) {
        alone(EV[p], EV[q], EV[r], EV[s], 0); ne++;
        if (exact_ok(EV[p], EV[q], EV[r], EV[s])) { alone(EV[p], EV[q], EV[r], EV[s], 1); nx++; }
    }
    /* exact: edge quotients times edge divisors where the product fits; and small divisors (units, 1+i, 2, ...) */
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) for (int r = 0; r < NEV; r++) for (int s = 0; s < NEV; s++) {
        i64 x1 = EV[p] * EV[r] - EV[q] * EV[s], x2 = EV[p] * EV[s] + EV[q] * EV[r];
        if ((EV[r] | EV[s]) && fits(x1) && fits(x2)) { alone(x1, x2, EV[r], EV[s], 1); nx++; }
    }
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++)
        for (i64 c = -4; c <= 4; c++) for (i64 d = -4; d <= 4; d++) {
            if (!(c | d)) continue;
            alone(EV[p], EV[q], c, d, 0); ne++;
            if (exact_ok(EV[p], EV[q], c, d)) { alone(EV[p], EV[q], c, d, 1); nx++; }
        }
    printf("edges: %ld divisions with remainder, %ld exact, each alone with flags; overflows %ld, flagged %ld (two kernels); failures so far %ld\n", ne, nx, novf, nflagged, fails);
    /* random */
    bflag = 1; long nr = 0, nrx = 0;
    for (int it = 0; it < 160; it++) {
        bexact = 0;
        for (int j = 0; j < BMAX; j++) {
            int kind = (int)(next() % 6); i64 a, b, c, d;
            #define EDGEOR(v) ((next() & 15) ? (v) : EV[next() % NEV])
            a = EDGEOR(rr(IMIN, IMAX)); b = EDGEOR(rr(IMIN, IMAX));
            if (kind == 0) { c = EDGEOR(rr(IMIN, IMAX)); d = EDGEOR(rr(IMIN, IMAX)); }
            else if (kind == 1) { int e = (int)(next() % 17); c = rr(-(1 << e), 1 << e); d = rr(-(1 << e), 1 << e); }
            else if (kind == 2) { int e = (int)(next() % 31); c = (next() & 1 ? 1 : -1) * ((1ll << e) + rr(-2, 2)); d = rr(-3, 3); if (next() & 1) { i64 t = c; c = d; d = t; } }
            else if (kind == 3) { c = rr(-2, 2); d = rr(-2, 2); }
            else if (kind == 4) { int e = (int)(next() % 32); c = rr(-(1ll << e), (1ll << e) - 1); d = rr(-(1ll << e), (1ll << e) - 1); }
            else { /* near ties: y with even norm, x = q y + y (1+i)/2-ish */ c = 2 * rr(-30000, 30000) + 1; d = 2 * rr(-30000, 30000) + 1; i64 q1 = rr(-30000, 30000), q2 = rr(-30000, 30000);
                   a = q1 * c - q2 * d + ((c + d) >> 1) * rr(-1, 1); b = q1 * d + q2 * c + ((d - c) >> 1) * rr(-1, 1); if (!fits(a) || !fits(b)) { a = 0; b = 0; } }
            if (!fits(c) || !fits(d)) { c = 1; d = 0; }
            push(a, b, c, d); nr++;
        }
        flush();
        bexact = 1;
        for (int j = 0; j < BMAX; j++) {
            int kind = (int)(next() % 4); i64 q1, q2, c, d;
            if (kind == 3) { i64 u[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}}; int k = (int)(next() % 8); c = u[k][0]; d = u[k][1];
                             i64 a = EDGEOR(rr(IMIN, IMAX)), b = EDGEOR(rr(IMIN, IMAX)); if (k >= 4 && ((a ^ b) & 1)) b ^= 1;
                             if (exact_ok(a, b, c, d)) { push(a, b, c, d); nrx++; } continue; }
            int e1 = (int)(next() % 30), e2 = 29 - e1;
            if (kind == 1) { e2 = (int)(next() % 30); e1 = 29 - e2; }
            q1 = rr(-(1ll << e1), 1ll << e1); q2 = rr(-(1ll << e1), 1ll << e1); c = rr(-(1ll << e2), 1ll << e2); d = rr(-(1ll << e2), 1ll << e2);
            if (kind == 2) { int s = (int)(next() % 8); c <<= s; d <<= s; q1 >>= s; q2 >>= s; }   /* powers of 2 in y */
            if (!(c | d)) c = 1;
            i64 x1 = q1 * c - q2 * d, x2 = q1 * d + q2 * c;
            if (fits(x1) && fits(x2) && fits(c) && fits(d)) { push(x1, x2, c, d); nrx++; }
        }
        flush();
    }
    printf("random: %ld divisions with remainder, %ld exact (batches of 65,536, batch flags); failures so far %ld\n", nr, nrx, fails);
    /* shared-divisor random: one random divisor per batch */
    for (int it = 0; it < 40; it++) {
        i64 c = it & 1 ? rr(IMIN, IMAX) : rr(-300, 300), d = it & 1 ? rr(IMIN, IMAX) : rr(-300, 300);
        if (it % 5 == 4) { c = IMIN; d = it & 2 ? IMIN : 0; }
        bexact = 0; for (int j = 0; j < BMAX; j++) push(EDGEOR(rr(IMIN, IMAX)), EDGEOR(rr(IMIN, IMAX)), c, d); flush();
    }
    printf("shared random: 40 divisors x 65,536; failures so far %ld\n", fails);
    /* properties of rules A and B */
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) for (int r = -3; r <= 3; r++) for (int s = -3; s <= 3; s++) prop_one(EV[p], EV[q], r, s);
    for (i64 a = -8; a <= 8; a++) for (i64 b = -8; b <= 8; b++) for (i64 c = -4; c <= 4; c++) for (i64 d = -4; d <= 4; d++) prop_one(a, b, c, d);
    for (int j = 0; j < 400000; j++) {
        int e = (int)(next() % 31);
        i64 c = rr(-(1ll << e), 1ll << e), d = rr(-(1ll << e), 1ll << e);
        if (j & 1) { c |= 1; d |= 1; i64 q1 = rr(-20000, 20000), q2 = rr(-20000, 20000);   /* ties: (1+i) | y */
            c &= 0xFFFF; d &= 0xFFFF; i64 a = q1 * c - q2 * d + ((c + d) >> 1), b = q1 * d + q2 * c + ((d - c) >> 1);
            if (fits(a) && fits(b)) prop_one(a, b, c, d); }
        else prop_one(rr(IMIN, IMAX), rr(IMIN, IMAX), c, d);
    }
    printf("rule properties (N(r) <= N(y)/2 for all six rules; A: r(x + ky) = r(x)), m1/m3/m5: %ld checks, %ld failures\n", prop_n, prop_fail);
    printf("TOTAL failures %ld (%ld element results checked)\n", fails + prop_fail, nchecked);
}

/* sign and unit changes. Transforms 0..6 are symmetries of z = x/y: x by -1, i, -i (q and r times the unit), y by -1,
 * i, -i (q times the inverse unit, r unchanged), conjugating both (q and r conjugated). For these the predicted
 * q', r' are checked exactly. Transforms 7..12 (conjugate x alone or y alone, negate one part of x or of y) are not
 * symmetries of z: z' is not a unit or conjugate image of z, so nothing is predicted; we count how often N(r) changes. */
static const char *TRN[13] = {"x*-1", "x*i", "x*-i", "y*-1", "y*i", "y*-i", "conj both", "conj x", "conj y",
                              "-re x", "-im x", "-re y", "-im y"};
static void xf(int t, i64 *a, i64 *b, i64 *c, i64 *d) {
    i64 x = *a, y = *b, u = *c, v = *d;
    switch (t) {
    case 0: x = -x; y = -y; break;              case 1: { i64 s = x; x = -y; y = s; } break;
    case 2: { i64 s = x; x = y; y = -s; } break; case 3: u = -u; v = -v; break;
    case 4: { i64 s = u; u = -v; v = s; } break; case 5: { i64 s = u; u = v; v = -s; } break;
    case 6: y = -y; v = -v; break;               case 7: y = -y; break;  case 8: v = -v; break;
    case 9: x = -x; break; case 10: y = -y; break; case 11: u = -u; break; case 12: v = -v; break;
    }
    *a = x; *b = y; *c = u; *d = v;
}
static void predict(int t, i64 q1, i64 q2, i64 r1, i64 r2, i64 *p) {   /* p = q1', q2', r1', r2' */
    i64 P[7][4] = {{-q1, -q2, -r1, -r2}, {-q2, q1, -r2, r1}, {q2, -q1, r2, -r1}, {-q1, -q2, r1, r2},
                   {q2, -q1, r1, r2}, {-q2, q1, r1, r2}, {q1, -q2, r1, -r2}};
    memcpy(p, P[t], sizeof P[t]);
}
static void do_signs(void) {
    enum { M = 4096 };
    static i32 a0[M], b0[M], c0[M], d0[M], a1[M], b1[M], c1[M], d1[M], o0[4][M], o1[4][M];
    long bad[6][13] = {{0}}, badtie[6][13] = {{0}}, normchg[6][13] = {{0}}, tot[6][13] = {{0}}, kfail = 0, nties = 0, bnd = 0;
    for (int it = 0; it < 600; it++) {
        int nn = 0;
        while (nn < M) {      /* inputs: no -2^31 half (so every transform stays in range); half of them at ties */
            i64 a, b, c, d, kind = (i64)(it % 3);
            int e = (int)(next() % 31);
            c = rr(-(1ll << e), (1ll << e)); d = rr(-(1ll << e), (1ll << e));
            if (kind == 0) { a = rr(-IMAX, IMAX); b = rr(-IMAX, IMAX); }
            else if (kind == 1) { c = rr(-8, 8); d = rr(-8, 8); a = rr(-40, 40); b = rr(-40, 40); }
            else {   /* exact half and corner points: x = q y + (s y + t i y)/2 with (1+i) | y */
                c = rr(-30000, 30000); d = rr(-30000, 30000); if ((c ^ d) & 1) d++;
                i64 q1 = rr(-30000, 30000), q2 = rr(-30000, 30000), s = rr(-1, 1), t = rr(-1, 1);
                a = q1 * c - q2 * d + (s * c - t * d) / 2; b = q1 * d + q2 * c + (s * d + t * c) / 2;
            }
            if (!(c | d) || !fits(a) || !fits(b) || a == IMIN || b == IMIN || c == IMIN || d == IMIN || !fits(c) || !fits(d)) continue;
            a0[nn] = (i32)a; b0[nn] = (i32)b; c0[nn] = (i32)c; d0[nn] = (i32)d; nn++;
        }
        for (int rule = 0; rule < 6; rule++) for (int k = 0; k < 3; k++) {
            rule_kernels[rule][k](a0, b0, c0, d0, o0[0], o0[1], o0[2], o0[3], M);
            for (int t = 0; t < 13; t++) {
                for (int j = 0; j < M; j++) { i64 a = a0[j], b = b0[j], c = c0[j], d = d0[j]; xf(t, &a, &b, &c, &d);
                    a1[j] = (i32)a; b1[j] = (i32)b; c1[j] = (i32)c; d1[j] = (i32)d; }
                rule_kernels[rule][k](a1, b1, c1, d1, o1[0], o1[1], o1[2], o1[3], M);
                for (int j = 0; j < M; j++) {
                    ref_t R0 = ref(a0[j], b0[j], c0[j], d0[j], RULES[rule]), R1 = ref(a1[j], b1[j], c1[j], d1[j], RULES[rule]);
                    if (R0.ov || R1.ov) continue;
                    if (o0[0][j] != R0.q1 || o0[1][j] != R0.q2 || o1[0][j] != R1.q1 || o1[1][j] != R1.q2) kfail++;
                    i128 N = (i128)c0[j] * c0[j] + (i128)d0[j] * d0[j];
                    i128 X = (i128)a0[j] * c0[j] + (i128)b0[j] * d0[j], Y = (i128)b0[j] * c0[j] - (i128)a0[j] * d0[j];
                    int tie = ((2 * X) % (2 * N) + 2 * N) % (2 * N) == N || ((2 * Y) % (2 * N) + 2 * N) % (2 * N) == N;
                    if (k == 0 && t == 0 && rule == 0) nties += tie;
                    i64 n0 = (i64)o0[2][j] * o0[2][j] + (i64)o0[3][j] * o0[3][j], n1 = (i64)o1[2][j] * o1[2][j] + (i64)o1[3][j] * o1[3][j];
                    i128 N1 = (i128)c1[j] * c1[j] + (i128)d1[j] * d1[j];
                    if (2 * (i128)n0 > N || 2 * (i128)n1 > N1) bnd++;
                    tot[rule][t]++;
                    if (n0 != n1) normchg[rule][t]++;
                    if (t < 7) {
                        i64 p[4]; predict(t, o0[0][j], o0[1][j], o0[2][j], o0[3][j], p);
                        if (p[0] != o1[0][j] || p[1] != o1[1][j] || p[2] != o1[2][j] || p[3] != o1[3][j]) { bad[rule][t]++; badtie[rule][t] += tie; }
                    }
                }
            }
        }
    }
    printf("sign and unit changes, m1/m3/m5, %ld base pairs at a tie (of %d); kernel/reference mismatches %ld; N(r) > N(y)/2: %ld\n", nties, 600 * M, kfail, bnd);
    for (int rule = 0; rule < 6; rule++) {
        printf("rule %s\n  %-10s %10s %12s %10s %12s\n", RNAME[rule], "transform", "cases", "not as pred.", "at a tie", "N(r) changed");
        for (int t = 0; t < 13; t++) {
            if (t < 7) printf("  %-10s %10ld %12ld %10ld %12ld\n", TRN[t], tot[rule][t], bad[rule][t], badtie[rule][t], normchg[rule][t]);
            else printf("  %-10s %10ld %12s %10s %12ld\n", TRN[t], tot[rule][t], "-", "-", normchg[rule][t]);
        }
    }
}

/* exhaustive check of the per-lane reciprocal over every dh in [2^31, 2^32) */
static void do_recip(void) {
    long hist[8] = {0}; long bad = 0;
    for (u64 base = 1ull << 31; base < (1ull << 32); base += 4) {
        V dh = _mm256_setr_epi64x(base, base + 1, base + 2, base + 3);
        V v0 = recip_newton(dh), v1 = recip_fix(dh, v0, RECIP_FIX);
        u64 a0[4], a1[4]; ST(a0, v0); ST(a1, v1);
        for (int j = 0; j < 4; j++) {
            u64 want = div128(1, 0, base + j + 1) - (1ull << 32);
            i64 def = (i64)(want - a0[j]);
            if (def < 0 || def > 7) bad++; else hist[def]++;
            if (a1[j] != want) bad++;
        }
    }
    printf("reciprocal, all 2^31 values of dh: deficit after Newton 0: %ld, 1: %ld, 2: %ld, more or negative: %ld; wrong after %d fix-up(s): %ld\n",
           hist[0], hist[1], hist[2], hist[3] + hist[4] + hist[5] + hist[6] + hist[7], RECIP_FIX, bad);
}

/* ======================= timing (integer clock) ======================= */
static u64 now_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec; }
static int cmpu(const void *a, const void *b) { u64 x = *(const u64 *)a, y = *(const u64 *)b; return (x > y) - (x < y); }
#define REPS 31
static void pps(u64 ps) { printf("%3llu.%03llu", (unsigned long long)(ps / 1000), (unsigned long long)(ps % 1000)); }
static i32 *P[8];
/* input kinds: 0 random 32-bit; 1 random x, small divisor (|parts| <= 256); 2 exact, q and y parts below 2^15;
 * shared: the divisor of element 0 is used (kinds 0 and 1: random or small; 2: y with parts below 2^15) */
static void fill(size_t n, int kind, int shared) {
    i64 cs = 0, ds = 0;
    while (!(cs | ds)) { cs = kind == 0 ? rr(IMIN, IMAX) : kind == 1 ? rr(-256, 256) : rr(-32767, 32767); ds = kind == 0 ? rr(IMIN, IMAX) : kind == 1 ? rr(-256, 256) : rr(-32767, 32767); }
    for (size_t i = 0; i < n; i++) {
        i64 c = cs, d = ds;
        if (!shared) { c = d = 0; while (!(c | d)) { c = kind == 0 ? rr(IMIN, IMAX) : kind == 1 ? rr(-256, 256) : rr(-32767, 32767); d = kind == 0 ? rr(IMIN, IMAX) : kind == 1 ? rr(-256, 256) : rr(-32767, 32767); } }
        i64 a, b;
        if (kind == 2) { i64 q1 = rr(-32767, 32767), q2 = rr(-32767, 32767); a = q1 * c - q2 * d; b = q1 * d + q2 * c; }
        else { a = rr(IMIN, IMAX); b = rr(IMIN, IMAX); }
        P[0][i] = (i32)a; P[1][i] = (i32)b; P[2][i] = (i32)c; P[3][i] = (i32)d;
    }
}
static void alloc_bufs(size_t n) {
    for (int j = 0; j < 8; j++) { char *p = aligned_alloc(4096, n * 4 + 4096 * 9); P[j] = (i32 *)(p + 64 * 9 * j % 4096 + 64 * j); }
}
/* time a set of kernels on the current buffers; results in ps per element */
static void time_set(const int *ks, int nk, size_t n, u64 inner, u64 out[][3]) {
    static u64 t[64][REPS];
    for (int k = 0; k < nk; k++) KT[ks[k]].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
    for (int r = 0; r < REPS; r++)
        for (int j = 0; j < nk; j++) {
            int k = ks[(j + r) % nk];
            KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            u64 t0 = now_ns();
            for (u64 it = 0; it < inner; it++) KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            t[(j + r) % nk][r] = (now_ns() - t0) * 1000 / (n * inner);
        }
    for (int k = 0; k < nk; k++) { qsort(t[k], REPS, 8, cmpu); out[k][0] = t[k][REPS / 2]; out[k][1] = t[k][REPS / 4]; out[k][2] = t[k][3 * REPS / 4]; }
}
static int kidx(const char *s) { for (int k = 0; k < NKT; k++) if (!strcmp(KT[k].name, s)) return k; printf("no kernel %s\n", s); exit(1); }
static void run_group(const char *title, const char **names, int nk, int kind, int shared, size_t n, u64 inner) {
    int ks[64]; for (int k = 0; k < nk; k++) ks[k] = kidx(names[k]);
    fill(n, kind, shared);
    u64 out[64][3]; time_set(ks, nk, n, inner, out);
    printf("%s\n", title);
    for (int k = 0; k < nk; k++) { printf("  %-8s ", names[k]); pps(out[k][0]); printf("  ["); pps(out[k][1]); printf(","); pps(out[k][2]); printf("]\n"); }
    fflush(stdout);
}
static void do_time(int mem) {
    size_t n = mem ? (1u << 24) : 1024; u64 inner = mem ? 1 : 512;
    alloc_bufs(n);
    printf("ns per element, %s, n = %zu; median [q1, q3] of %d reps, kernels interleaved, order rotated\n", mem ? "from memory" : "in L1", n, REPS);
    const char *vr[] = {"m1_fl", "m1_na", "m1_b1", "m1_b2", "m1_b3", "m1_b4", "m1_b5", "m3_fl", "m3_na", "m3_b1", "m3_b2", "m3_b3", "m3_b4", "m3_b5",
                        "m5_fl", "m5_na", "m5x_fl", "m5x_na", "m5x_b1", "m5x_b2", "m5x_b3", "m5x_b4", "m5x_b5"};
    const char *vx[] = {"m1_ex", "m2_ex", "m3_ex", "m5_ex", "m5x_ex"};
    const char *sr[] = {"m1s_fl", "m1s_na", "m1s_b1", "m1s_b5", "m3s_fl", "m3s_na", "m3s_b1", "m3s_b5", "m4_fl", "m4_na", "m4_b1", "m4_b2", "m4_b3", "m4_b4", "m4_b5"};
    const char *sx[] = {"m1s_ex", "m2s_ex", "m3s_ex", "m4_ex"};
    run_group("varying divisor, remainder, random 32-bit", vr, 23, 0, 0, n, inner);
    run_group("varying divisor, remainder, small divisor (|parts| <= 256)", vr, 23, 1, 0, n, inner);
    run_group("varying divisor, exact, q and y parts below 2^15", vx, 5, 2, 0, n, inner);
    run_group("shared divisor, remainder, random 32-bit divisor", sr, 15, 0, 1, n, inner);
    run_group("shared divisor, remainder, small divisor", sr, 15, 1, 1, n, inner);
    run_group("shared divisor, exact, q and y parts below 2^15", sx, 4, 2, 1, n, inner);
}
/* cost against quotient width: m3 with a forced width W, and the adaptive kernel's chosen widths */
static void do_width(void) {
    size_t n = 1024; alloc_bufs(n);
    int k = kidx("m3w_fl"), k1 = kidx("m1_fl");
    printf("m3 floor core, forced width W (in L1, random data; the loop does not depend on the data)\n");
    int Ws[] = {1, 2, 4, 8, 16, 24, 32};
    fill(n, 0, 0);
    for (int w = 0; w < 7; w++) {
        g_W = Ws[w]; int ks[2] = {k, k1}; u64 out[2][3]; time_set(ks, 2, n, 256, out);
        printf("  W = %2d: ", g_W); pps(out[0][0]); printf("  ["); pps(out[0][1]); printf(","); pps(out[0][2]); printf("]   (m1_fl "); pps(out[1][0]); printf(")\n");
    }
    printf("adaptive width chosen per register (m3c_fl) by input kind:\n");
    const char *kn[] = {"random", "small divisor", "exact 2^15 x 2^15"};
    for (int kind = 0; kind < 3; kind++) {
        fill(n, kind, 0); memset(w_hist, 0, sizeof w_hist);
        m3c_fl(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
        long tot = 0, sum = 0; for (int w = 0; w <= 32; w++) { tot += w_hist[w]; sum += w * w_hist[w]; }
        printf("  %-18s mean W = %ld.%02ld  (", kn[kind], sum / tot, sum * 100 / tot % 100);
        for (int w = 0; w <= 32; w++) if (w_hist[w]) printf(" %d:%ld", w, w_hist[w]);
        printf(" )\n");
    }
}
/* threshold: whole call (precomputation included) for short shared-divisor arrays */
static void do_thresh(void) {
    alloc_bufs(4096);
    const char *nm[] = {"m1s_fl", "m3s_fl", "m4_fl", "m1s_na", "m3s_na", "m4_na"};
    int ks[6]; for (int k = 0; k < 6; k++) ks[k] = kidx(nm[k]);
    size_t ns[] = {8, 16, 24, 32, 48, 64, 128, 256};
    for (int kind = 0; kind < 2; kind++) {
        printf("%s divisor: ns per call (median of %d), whole call including precomputation\n   n ", kind ? "small" : "random 32-bit", REPS);
        for (int k = 0; k < 6; k++) printf(" %9s", nm[k]); printf("\n");
        for (int s = 0; s < 8; s++) {
            fill(ns[s], kind, 1);
            u64 out[6][3]; time_set(ks, 6, ns[s], 20000, out);
            printf("  %3zu", ns[s]); for (int k = 0; k < 6; k++) { printf("  "); pps(out[k][0] * ns[s]); } printf("\n");
        }
    }
}

/* one call of every kernel on 4,096 elements of one input kind, for a dynamic instruction count under valgrind */
static void do_icount(int kind) {
    size_t n = 4096; alloc_bufs(n);
    for (int shared = 0; shared < 2; shared++) {
        fill(n, kind, shared);
        for (int k = 0; k < NKT; k++) if (KT[k].shared == shared && ((KT[k].mode == EX) == (kind == 2)) && KT[k].f != m3w_fl && KT[k].f != m3c_fl)
            KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
    }
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else if (!strcmp(mode, "recip")) do_recip();
    else if (!strcmp(mode, "signs")) do_signs();
    else if (!strcmp(mode, "signsmag")) {    /* the same checks on the magnitude-form B kernels */
        kf mk[5][3] = {{m1_mb1, m3_mb1, m5x_mb1}, {m1_mb2, m3_mb2, m5x_mb2}, {m1_mb3, m3_mb3, m5x_mb3}, {m1_mb4, m3_mb4, m5x_mb4}, {m1_mb5, m3_mb5, m5x_mb5}};
        for (int u = 0; u < 5; u++) for (int t = 0; t < 3; t++) rule_kernels[u + 1][t] = mk[u][t];
        do_signs(); }
    else if (!strcmp(mode, "time")) do_time(0);
    else if (!strcmp(mode, "mem")) do_time(1);
    else if (!strcmp(mode, "width")) do_width();
    else if (!strcmp(mode, "thresh")) do_thresh();
    else if (!strcmp(mode, "one") && argc > 3) {     /* one kernel alone: ./divvec one NAME KIND */
        alloc_bufs(1024); const char *nm[1] = {argv[2]}; int kind = atoi(argv[3]);
        run_group("alone", nm, 1, kind, KT[kidx(argv[2])].shared, 1024, 512); }
    else if (!strcmp(mode, "icount")) do_icount(argc > 2 ? atoi(argv[2]) : 0);
    return 0;
}
