/* Study 29, milestone 1: study 25's Gaussian division in NEON shape on the x86 stand-in (neon29div.c)
 *
 * Same problem as divvec.c: x = a + bi over y = c + di, int32 halves over the full range, separate arrays, no float.
 * X = ac + bd, Y = bc - ad, N = c^2 + d^2 in 64-bit lanes (X wraps only at the all -2^31 corner, X = 2^63).
 * 128-bit registers: 4 elements per register, each part as a pair of 64-bit registers (lo, hi).
 *
 * What changed from the AVX2 code (portable-isa.md; the AVX2 form in brackets):
 *   front end   smull + smlal / smlsl per pair (2 per pair)                         [vpmuldq even + shifted odd, add]
 *   zero divisor  fz = umin(fz, c | d), c = c - (c | d == 0)                       [cmpeq64, and, or per pair]
 *   mask form   |u| = abs64(u) (one op, correct at the wrapped 2^63); the 32-bit sign mask from the high halves:
 *               mX = [hi32(X - 1) < 0], mY = [hi32(Y) < 0] (uzp2, cmlt); 64-bit masks (sxtl) only for ties up/down
 *                                                                                 [64-bit compare, xor, sub, blend]
 *   direct form floor remainder rho = bit(R, N - 1 - R, s) (sub, bsl)              [(R xor s) + (N and s)]
 *   packing     uzp1 / uzp2                                                        [vpblendd with an immediate]
 *   flags       fq = smin(fq, part): some part is -2^31 iff the min is; fz likewise; tested once per array
 *                                                                                 [cmpeq, or, movemask per register]
 *   remainder   r = x - q y by mla / mls (2 per part)                              [4 vpmulld + adds]
 *   m2          ctz by clz (k = 31 - clz(o & -o)), the 1+i test by cmtst, Newton by mls + mul [popcount by
 *               vpshufb + vpmaddubsw + vpmaddwd; slli/srai pair; mullo, sub, mullo]
 *   m4 estimate S = U' + hi(U') m' + (lo(U') m' >> 32): uzp1, uzp2, umlal, umull, usra    [shifts, vpmuludq, adds]
 *   correction  rho = U - q N: umlsl by lo(N), then the q hi(N) term by mul and shll; rho >= N by cmhs
 *                                                                                 [vpmuludq x2, shift, bias xor, cmpgt]
 *   m5 normalise  64-bit clz from two 32-bit clz (clz, uzp, cmeq, and, add), then ushl   [6-step binary search, 30 ops]
 *   m5 reciprocal the same Newton steps on dh packed in 32-bit lanes (umlsl, usubw, shrn, umull, uzp2)
 *   m5 scaling  th = ushl(U, sh - 32), one op with a signed count                  [sllv, srlv, or]
 * m3 (shift and subtract) is not ported: decision 32 dropped it.
 *
 * Methods: m1 scalar divide (control), m2 exact by the inverse mod 2^32 (m2p: ctz by popcount, for comparison),
 * m2s shared, m4 shared reciprocal, m5 per-lane reciprocal (m5x: 2 registers interleaved, m5x3: 3).
 * Modes: EX exact, FL floor core, NA ties up, ND ties down, B1 away from zero, B2 toward zero; MAG = mask form.
 * Flags: bit 0 = some q part (nearest: or r part) is -2^31 (resolve() decides); bit 1 = some divisor is zero.
 * n is a multiple of 4.
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -mavx512bitalg -o neon29div neon29div.c
 *        (-DCOUNT for NEON counts; -DFAULT=k for the injected faults)
 * Run:   ./neon29div check | quick | recip | time | mem | ties | tiesmem | count | icount | scan
 */
#include "neon29isa.c"

#define IMIN INT32_MIN
#define IMAX INT32_MAX
enum { EX, FL, NA, B1, B2, ND };
#define MAG 16
#define RULE(m) ((m) & 15)
#define NEAR(m) (RULE(m) >= NA)
#ifndef FAULT
#define FAULT 0
#endif
/* faults: 1 drop the m4/m5 correction; 2 drop the 1+i step on x in m2; 3 plain sign of X (no wrapped 2^63) in the
 * direct form; 4 one reciprocal fix-up; 5 mask form's sign of X from hi32(X) (no -1); 6 m5 scaling by sh - 31;
 * 7 mask form toward zero uses (N - 1) >> 1 (that is away from zero) */
#define RECIP_FIX (FAULT == 4 ? 1 : 2)

#define ARGS const i32 *restrict pa, const i32 *restrict pb, const i32 *restrict pc, const i32 *restrict pd, \
             i32 *restrict pqr, i32 *restrict pqi, i32 *restrict prr, i32 *restrict pri, size_t n
#define PASS pa, pb, pc, pd, pqr, pqi, prr, pri, n
typedef u32 (*kf)(ARGS);

static inline u64 div128(u64 hi, u64 lo, u64 d) {      /* floor((hi 2^64 + lo) / d), needs hi < d */
#ifdef A64
    return (u64)((((u128)hi << 64) | lo) / d);
#else
    u64 q, r; __asm__("divq %4" : "=a"(q), "=d"(r) : "a"(lo), "d"(hi), "rm"(d)); return q;
#endif
}

/* ======================= m1: scalar hardware divide (divvec.c, rules B3..B5 removed) ======================= */
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
AI u32 s_div(i32 a, i32 b, i32 c, i32 d, u64 N, i32 *qr, i32 *qi, i32 *rr, i32 *ri, int mode) {
    u64 X = (u64)((i64)a * c) + (u64)((i64)b * d), Y = (u64)((i64)b * c) - (u64)((i64)a * d);
    u64 sx = -(u64)((i64)(X - 1) < -1), sy = -(u64)((i64)Y < 0);
    __asm__("" : "+r"(sx), "+r"(sy));
    if (mode & MAG) {
        u64 ux = (X ^ sx) - sx, uy = (Y ^ sy) - sy;
        u64 qx = ux / N, qy = uy / N;
        u32 q1 = (u32)qx, q2 = (u32)qy;
        if (NEAR(mode)) {
            u64 rx = ux % N, ry = uy % N, hx, hy; int R_ = RULE(mode);
            if (R_ == B1) hx = hy = (N - 1) >> 1;
            else if (R_ == NA) { hx = (N - 1 - sx) >> 1; hy = (N - 1 - sy) >> 1; }
            else if (R_ == ND) { hx = (N + sx) >> 1; hy = (N + sy) >> 1; }
            else hx = hy = N >> 1;
            q1 += (u32)((hx - rx) >> 63); q2 += (u32)((hy - ry) >> 63);
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
        u64 hx, hy;
        if (RULE(mode) == NA) hx = hy = (N - 1) >> 1;
        else if (RULE(mode) == B1) { hx = (N - 1 - sx) >> 1; hy = (N - 1 - sy) >> 1; }
        else if (RULE(mode) == B2) { hx = (N + sx) >> 1; hy = (N + sy) >> 1; }
        else hx = hy = N >> 1;
        q1 += (u32)((hx - rx) >> 63); q2 += (u32)((hy - ry) >> 63);
    }
    return s_out(a, b, c, d, q1, q2, qr, qi, rr, ri, mode);
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

/* ======================= vector front and back ======================= */
#define C64_(x) vdupq_n_s64(x)
#define C32_(x) vdupq_n_s32(x)
/* X, Y (and N when the divisor varies) for one register of 4 elements */
AI void front(V a, V b, V c, V d, int shared, P2 *X, P2 *Y, P2 *N) {
    X->l = vmlal_lo_s32(vmull_lo_s32(a, c), b, d); X->h = vmlal_high_s32(vmull_high_s32(a, c), b, d);
    Y->l = vmlsl_lo_s32(vmull_lo_s32(b, c), a, d); Y->h = vmlsl_high_s32(vmull_high_s32(b, c), a, d);
    if (!shared) { N->l = vmlal_lo_s32(vmull_lo_s32(c, c), d, d); N->h = vmlal_high_s32(vmull_high_s32(c, c), d, d); }
}
/* zero divisor (varying): fz tracks the unsigned min of c | d; a zero divisor becomes 1 (its outputs are unspecified) */
AI void zero_fix(V *c, V d, V *fz) { V o = vorrq(*c, d); *fz = vminq_u32(*fz, o); *c = vsubq_s32(*c, vceqzq_s32(o)); }
/* direct form: s = [X < 0] with the wrapped 2^63 positive (X - 1 < -1), s = [Y < 0]; U = u xor s */
AI void signs_direct(P2 X, P2 Y, P2 *UX, P2 *UY, P2 *SX, P2 *SY) {
    const V one = C64_(1), mone = C64_(-1);
#if FAULT == 3
    SX->l = vcltzq_s64(X.l); SX->h = vcltzq_s64(X.h); (void)one; (void)mone;
#else
    SX->l = vcltq_s64(vsubq_s64(X.l, one), mone); SX->h = vcltq_s64(vsubq_s64(X.h, one), mone);
#endif
    SY->l = vcltzq_s64(Y.l); SY->h = vcltzq_s64(Y.h);
    UX->l = veorq(X.l, SX->l); UX->h = veorq(X.h, SX->h); UY->l = veorq(Y.l, SY->l); UY->h = veorq(Y.h, SY->h);
}
/* mask form: |u| = abs64(u) (the wrapped 2^63 reads as 2^63 unsigned); 32-bit masks from the high halves.
 * mX = [X - 1 < 0] = [X <= 0] with the corner positive; X = 0 counted negative is harmless (q = 0, r = x). */
AI void signs_mask(P2 X, P2 Y, P2 *UX, P2 *UY, V *mx, V *my) {
    UX->l = vabsq_s64(X.l); UX->h = vabsq_s64(X.h); UY->l = vabsq_s64(Y.l); UY->h = vabsq_s64(Y.h);
#if FAULT == 5
    *mx = vcltzq_s32(vuzp2q_w(X.l, X.h));
#else
    const V one = C64_(1);
    *mx = vcltzq_s32(vuzp2q_w(vsubq_s64(X.l, one), vsubq_s64(X.h, one)));
#endif
    *my = vcltzq_s32(vuzp2q_w(Y.l, Y.h));
}
/* round, form q and r, store, update the sticky flag fq (running min) */
AI void back(int mode, V a, V b, V c, V d, V qx, V qy, P2 RX, P2 RY, P2 SX, P2 SY, V mx, V my, P2 N,
             i32 *qr, i32 *qi, i32 *rr, i32 *ri, V *fq) {
    const V one = C64_(1);
    int r = RULE(mode);
    V qre, qim;
    if (mode & MAG) {
        if (NEAR(mode)) {
            P2 HX, HY;
            if (r == B1 || r == B2) {           /* on the magnitude: away = ties up, toward = ties down; no sign */
#if FAULT == 7
                HX.l = vshrq_n_u64(vsubq_s64(N.l, one), 1); HX.h = vshrq_n_u64(vsubq_s64(N.h, one), 1);
#else
                if (r == B1) { HX.l = vshrq_n_u64(vsubq_s64(N.l, one), 1); HX.h = vshrq_n_u64(vsubq_s64(N.h, one), 1); }
                else { HX.l = vshrq_n_u64(N.l, 1); HX.h = vshrq_n_u64(N.h, 1); }
#endif
                HY = HX;
            } else {                            /* signed ties up / down need the sign in the threshold: sxtl */
                P2 mX = {vmovl_lo_s32(mx), vmovl_high_s32(mx)}, mY = {vmovl_lo_s32(my), vmovl_high_s32(my)};
                if (r == NA) {
                    P2 Nm1 = {vsubq_s64(N.l, one), vsubq_s64(N.h, one)};
                    HX.l = vshrq_n_u64(vsubq_s64(Nm1.l, mX.l), 1); HX.h = vshrq_n_u64(vsubq_s64(Nm1.h, mX.h), 1);
                    HY.l = vshrq_n_u64(vsubq_s64(Nm1.l, mY.l), 1); HY.h = vshrq_n_u64(vsubq_s64(Nm1.h, mY.h), 1);
                } else {
                    HX.l = vshrq_n_u64(vaddq_s64(N.l, mX.l), 1); HX.h = vshrq_n_u64(vaddq_s64(N.h, mX.h), 1);
                    HY.l = vshrq_n_u64(vaddq_s64(N.l, mY.l), 1); HY.h = vshrq_n_u64(vaddq_s64(N.h, mY.h), 1);
                }
            }
            qx = vsubq_s32(qx, vuzp1q_w(vcgtq_s64(RX.l, HX.l), vcgtq_s64(RX.h, HX.h)));
            qy = vsubq_s32(qy, vuzp1q_w(vcgtq_s64(RY.l, HY.l), vcgtq_s64(RY.h, HY.h)));
        }
        qre = vsubq_s32(veorq(qx, mx), mx); qim = vsubq_s32(veorq(qy, my), my);
    } else {
        qre = veorq(qx, vuzp1q_w(SX.l, SX.h)); qim = veorq(qy, vuzp1q_w(SY.l, SY.h));
        if (NEAR(mode)) {
            P2 Nm1 = {vsubq_s64(N.l, one), vsubq_s64(N.h, one)};
            P2 rX = {vbslq(SX.l, vsubq_s64(Nm1.l, RX.l), RX.l), vbslq(SX.h, vsubq_s64(Nm1.h, RX.h), RX.h)};   /* NEON: bit */
            P2 rY = {vbslq(SY.l, vsubq_s64(Nm1.l, RY.l), RY.l), vbslq(SY.h, vsubq_s64(Nm1.h, RY.h), RY.h)};
            P2 HX, HY;
            if (r == NA) { HX.l = vshrq_n_u64(Nm1.l, 1); HX.h = vshrq_n_u64(Nm1.h, 1); HY = HX; }
            else if (r == ND) { HX.l = vshrq_n_u64(N.l, 1); HX.h = vshrq_n_u64(N.h, 1); HY = HX; }
            else if (r == B2) {
                HX.l = vshrq_n_u64(vaddq_s64(N.l, SX.l), 1); HX.h = vshrq_n_u64(vaddq_s64(N.h, SX.h), 1);
                HY.l = vshrq_n_u64(vaddq_s64(N.l, SY.l), 1); HY.h = vshrq_n_u64(vaddq_s64(N.h, SY.h), 1);
            } else {
                HX.l = vshrq_n_u64(vsubq_s64(Nm1.l, SX.l), 1); HX.h = vshrq_n_u64(vsubq_s64(Nm1.h, SX.h), 1);
                HY.l = vshrq_n_u64(vsubq_s64(Nm1.l, SY.l), 1); HY.h = vshrq_n_u64(vsubq_s64(Nm1.h, SY.h), 1);
            }
            qre = vsubq_s32(qre, vuzp1q_w(vcgtq_s64(rX.l, HX.l), vcgtq_s64(rX.h, HX.h)));
            qim = vsubq_s32(qim, vuzp1q_w(vcgtq_s64(rY.l, HY.l), vcgtq_s64(rY.h, HY.h)));
        }
    }
    vst1q(qr, qre); vst1q(qi, qim);
    *fq = vminq_s32(vminq_s32(*fq, qre), qim);
    if (r != EX) {
        V rre = vmlsq_s32(vmlaq_s32(a, qim, d), qre, c);     /* a - qre c + qim d */
        V rim = vmlsq_s32(vmlsq_s32(b, qre, d), qim, c);     /* b - qre d - qim c */
        vst1q(rr, rre); vst1q(ri, rim);
        if (NEAR(mode)) *fq = vminq_s32(vminq_s32(*fq, rre), rim);
    }
}
AI u32 flags_out(V fq, V fz) {
    return (vany(vceqq_s32(fq, C32_(IMIN))) ? 1 : 0) | (vany(vceqzq_s32(fz)) ? 2 : 0);
}

/* one exact correction from q^ in {q - 1, q}: rho = U - q^ N, and if rho >= N: rho -= N, q += 1.
 * Nl, Nh: the low and high 32 bits of N packed (even dwords with lo, odd with hi); Np: N as a pair. */
AI V corr(P2 U, V q, V Nl, V Nh, P2 Np, P2 *R) {
    V rl = vmlsl_lo_u32(U.l, q, Nl), rh = vmlsl_high_u32(U.h, q, Nl);
    V t = vmulq_s32(q, Nh);
    rl = vsubq_s64(rl, vshll32_lo(t)); rh = vsubq_s64(rh, vshll32_high(t));
#if FAULT != 1
    V gl = vcgeq_u64(rl, Np.l), gh = vcgeq_u64(rh, Np.h);
    rl = vsubq_s64(rl, vandq(gl, Np.l)); rh = vsubq_s64(rh, vandq(gh, Np.h));
    q = vsubq_s32(q, vuzp1q_w(gl, gh));
#endif
    R->l = rl; R->h = rh;
    return q;
}

/* ======================= m4: shared divisor, precomputed reciprocal ======================= */
/* L = bitlen(N - 1), m = floor(2^(L+32) / N) in [2^32, 2^33), m' = m - 2^32, s = max(0, L - 32), U' = U >> s,
 * S = U' + hi(U') m' + (lo(U') m' >> 32), q^ = S >> (L - s), q - 1 <= q^ <= q. */
AI V m4_core(P2 U, V cs, V cL, V mp, V Nl, V Nh, P2 Np, P2 *R) {
    V ul = vshrq_u64_r(U.l, cs), uh = vshrq_u64_r(U.h, cs);
    V lo = vuzp1q_w(ul, uh), hi = vuzp2q_w(ul, uh);
    V Sl = vmlal_lo_u32(ul, hi, mp), Sh = vmlal_high_u32(uh, hi, mp);
    Sl = vsraq_n_u64(Sl, vmull_lo_u32(lo, mp), 32); Sh = vsraq_n_u64(Sh, vmull_high_u32(lo, mp), 32);
    V q = vuzp1q_w(vshrq_u64_r(Sl, cL), vshrq_u64_r(Sh, cL));
    return corr(U, q, Nl, Nh, Np, R);
}
AI u32 m4_body(ARGS, int mode) {
    u32 f = 0;
    u64 Ns = (u64)((i64)pc[0] * pc[0]) + (u64)((i64)pd[0] * pd[0]);
    if (Ns == 0) { f |= 2; Ns = 1; }
    int L = Ns == 1 ? 0 : 64 - __builtin_clzll(Ns - 1);
    int K = L + 32, s = L > 32 ? L - 32 : 0;
    u64 m = K >= 64 ? div128(1ull << (K - 64), 0, Ns) : (1ull << K) / Ns;
    const V mp = C32_((u32)(m - (1ull << 32))), Nl = C32_((u32)Ns), Nh = C32_((u32)(Ns >> 32)), N64 = C64_(Ns);
    const V cs = SHCNT(s), cL = SHCNT(L - s), cb = C32_(pc[0]), db = C32_(pd[0]);
    const P2 Np = {N64, N64};
    V fq = C32_(IMAX);
    for (size_t i = 0; i < n; i += 4) {
        V a = vld1q(pa + i), b = vld1q(pb + i);
        P2 X, Y, Nd, UX, UY, SX = {0}, SY = {0}, RX, RY; V mx = vzero(), my = vzero();
        front(a, b, cb, db, 1, &X, &Y, &Nd);
        if (mode & MAG) signs_mask(X, Y, &UX, &UY, &mx, &my); else signs_direct(X, Y, &UX, &UY, &SX, &SY);
        V qx = m4_core(UX, cs, cL, mp, Nl, Nh, Np, &RX), qy = m4_core(UY, cs, cL, mp, Nl, Nh, Np, &RY);
        back(mode, a, b, cb, db, qx, qy, RX, RY, SX, SY, mx, my, Np, pqr + i, pqi + i, prr + i, pri + i, &fq);
    }
    return f | (vany(vceqq_s32(fq, C32_(IMIN))) ? 1 : 0);
}

/* ======================= m5: per-lane reciprocal ======================= */
/* dh in [2^31, 2^32), packed in 32-bit lanes. v' = floor(2^64 / (dh + 1)) - 2^32 by divvec's Newton steps:
 * linear seed Y = A - hi32(dh B + B) - 1, three steps Y += hi32(Y ((2^63 - dh Y - Y) >> 31)), v = max(2Y - 2^32, 0),
 * one 33-bit step v += e_h + hi32(v e_h) with e_h = (2^64 - D (2^32 + v)) >> 32; then RECIP_FIX exact fix-ups.
 * Every value that divvec held in a 64-bit lane but which fits 32 bits (Y, f, v, e_h) is packed here, so the
 * products are umull / umull2 on 4 lanes and the high halves come back by one uzp2. */
AI V recip_newton_p(V dh) {
    const V Bq = C64_(3817748708ull), B32 = C32_(3817748708u), Am1 = C32_((u32)(5726623061ull - 1));
    const V T63 = C64_(1ull << 63), Cm = C64_(-(1ll << 32));
    V Y = vsubq_s32(Am1, vuzp2q_w(vmlal_lo_u32(Bq, dh, B32), vmlal_high_u32(Bq, dh, B32)));
    _Pragma("GCC unroll 4") for (int it = 0; it < 3; it++) {
        V el = vsubw_lo_u32(vmlsl_lo_u32(T63, dh, Y), Y), eh = vsubw_high_u32(vmlsl_high_u32(T63, dh, Y), Y);
        V f = vshrn_pair(el, eh, 31);
        Y = vaddq_s32(Y, vuzp2q_w(vmull_lo_u32(Y, f), vmull_high_u32(Y, f)));
    }
    V v = vandq(vaddq_s32(Y, Y), vcltzq_s32(Y));                        /* Y >= 2^31 ? 2Y - 2^32 : 0 */
    V el = vsubw_lo_u32(vmlsl_lo_u32(Cm, dh, v), v), eh2 = vsubw_high_u32(vmlsl_high_u32(Cm, dh, v), v);
    V e_h = vsubq_s32(vuzp2q_w(el, eh2), dh);                            /* subtract dh 2^32: high dwords only */
    return vaddq_s32(v, vaddq_s32(e_h, vuzp2q_w(vmull_lo_u32(v, e_h), vmull_high_u32(v, e_h))));
}
AI V recip_fix_p(V dh, V v, int nfix) {
    const V Cm = C64_(-(1ll << 32)), one = C64_(1);
    P2 d64 = {vmovl_lo_u32(dh), vmovl_high_u32(dh)}, D = {vaddq_s64(d64.l, one), vaddq_s64(d64.h, one)};
    V Rl = vsubq_s64(vsubw_lo_u32(vmlsl_lo_u32(Cm, dh, v), v), vshll32_lo(dh));     /* 2^64 - D (2^32 + v) */
    V Rh = vsubq_s64(vsubw_high_u32(vmlsl_high_u32(Cm, dh, v), v), vshll32_high(dh));
    _Pragma("GCC unroll 4") for (int k = 0; k < nfix; k++) {
        V gl = vcgtq_s64(Rl, d64.l), gh = vcgtq_s64(Rh, d64.h);
        Rl = vsubq_s64(Rl, vandq(gl, D.l)); Rh = vsubq_s64(Rh, vandq(gh, D.h));
        v = vsubq_s32(v, vuzp1q_w(gl, gh));
    }
    return v;
}
/* normalise N (64-bit clz from two 32-bit clz), take dh, reciprocal; kk = sh - 32 as signed 64-bit counts */
AI void m5_setup(P2 N, P2 *kk, V *vp, V *Nl, V *Nh) {
    const V c32 = C32_(32);
    V zl = vclzq_u32(N.l), zh = vclzq_u32(N.h);
    V ch = vuzp2q_w(zl, zh), cl = vuzp1q_w(zl, zh);
    V k = vaddq_s32(ch, vandq(cl, vceqq_s32(ch, c32)));                 /* clz64 = ch, or 32 + cl when ch = 32 */
    V dh = vuzp2q_w(vshlq_u64_v(N.l, vmovl_lo_u32(k)), vshlq_u64_v(N.h, vmovl_high_u32(k)));
    *vp = recip_fix_p(dh, recip_newton_p(dh), RECIP_FIX);
#if FAULT == 6
    V km = vsubq_s32(k, C32_(31));
#else
    V km = vsubq_s32(k, c32);
#endif
    kk->l = vmovl_lo_s32(km); kk->h = vmovl_high_s32(km);
    *Nl = vuzp1q_w(N.l, N.h); *Nh = vuzp2q_w(N.l, N.h);
}
AI V m5_core(P2 U, P2 kk, V vp, V Nl, V Nh, P2 Np, P2 *R) {
    V tl = vshlq_u64_sgn(U.l, kk.l), th = vshlq_u64_sgn(U.h, kk.h);       /* floor(U 2^sh / 2^32) */
    V lo = vuzp1q_w(tl, th), hi = vuzp2q_w(tl, th);
    V Sl = vmlal_lo_u32(tl, hi, vp), Sh = vmlal_high_u32(th, hi, vp);
    Sl = vsraq_n_u64(Sl, vmull_lo_u32(lo, vp), 32); Sh = vsraq_n_u64(Sh, vmull_high_u32(lo, vp), 32);
    return corr(U, vuzp2q_w(Sl, Sh), Nl, Nh, Np, R);
}
/* G registers (4G elements) per pass, every stage done for all G before the next */
AI void m5_pass(int G, int mode, const i32 *pa, const i32 *pb, const i32 *pc, const i32 *pd, i32 *pqr, i32 *pqi, i32 *prr, i32 *pri, V *fq, V *fz) {
    V a[3], b[3], c[3], d[3], mx[3], my[3], vp[3], Nl[3], Nh[3], qx[3], qy[3];
    P2 X[3], Y[3], N[3], UX[3], UY[3], SX[3], SY[3], RX[3], RY[3], kk[3];
    _Pragma("GCC unroll 4") for (int g = 0; g < G; g++) {
        a[g] = vld1q(pa + 4 * g); b[g] = vld1q(pb + 4 * g); c[g] = vld1q(pc + 4 * g); d[g] = vld1q(pd + 4 * g);
        zero_fix(&c[g], d[g], fz);
        front(a[g], b[g], c[g], d[g], 0, &X[g], &Y[g], &N[g]);
        mx[g] = my[g] = vzero(); SX[g].l = SX[g].h = SY[g].l = SY[g].h = vzero();
        if (mode & MAG) signs_mask(X[g], Y[g], &UX[g], &UY[g], &mx[g], &my[g]);
        else signs_direct(X[g], Y[g], &UX[g], &UY[g], &SX[g], &SY[g]);
    }
    _Pragma("GCC unroll 4") for (int g = 0; g < G; g++) m5_setup(N[g], &kk[g], &vp[g], &Nl[g], &Nh[g]);
    _Pragma("GCC unroll 4") for (int g = 0; g < G; g++) {
        qx[g] = m5_core(UX[g], kk[g], vp[g], Nl[g], Nh[g], N[g], &RX[g]);
        qy[g] = m5_core(UY[g], kk[g], vp[g], Nl[g], Nh[g], N[g], &RY[g]);
    }
    _Pragma("GCC unroll 4") for (int g = 0; g < G; g++)
        back(mode, a[g], b[g], c[g], d[g], qx[g], qy[g], RX[g], RY[g], SX[g], SY[g], mx[g], my[g], N[g],
             pqr + 4 * g, pqi + 4 * g, prr + 4 * g, pri + 4 * g, fq);
}
AI u32 m5_body(ARGS, int mode, int G) {
    V fq = C32_(IMAX), fz = C32_(-1); size_t i = 0;
    for (; i + 4 * G <= n; i += 4 * G) m5_pass(G, mode, pa + i, pb + i, pc + i, pd + i, pqr + i, pqi + i, prr + i, pri + i, &fq, &fz);
    for (; i < n; i += 4) m5_pass(1, mode, pa + i, pb + i, pc + i, pd + i, pqr + i, pqi + i, prr + i, pri + i, &fq, &fz);
    return flags_out(fq, fz);
}

/* ======================= m2: exact division by the inverse mod 2^32 ======================= */
/* ctz of 32-bit lanes. clz form: k = 31 - clz(o AND -o) (NEON computes -k = clz - 31 at the same cost and shifts
 * by it). popcount form (m2p): popcount((o AND -o) - 1) by cnt and two pairwise widening adds. */
AI V ctz_clz(V o) { return vsubq_s32(C32_(31), vclzq_u32(vandq(o, vnegq_s32(o)))); }
AI V ctz_pop(V o) { return vpaddlq_u16(vpaddlq_u8(vcntq_u8(vsubq_s32(vandq(o, vnegq_s32(o)), C32_(1))))); }
/* x / (1+i) where m (full-lane mask; t = 1 there) says so: ((a >> 1) + (b >> 1) + (a & 1), (b >> 1) - (a >> 1)).
 * NEON shifts by sshl(x, m) (m = -t); here srav by t. */
AI void div1i(V *a, V *b, V t, V m) {
    V x = *a, y = *b;
    *a = vaddq_s32(vaddq_s32(vshrq_s32_v(x, t), vshrq_s32_v(vandq(y, m), t)), vandq(x, t));
    *b = vsubq_s32(vshrq_s32_v(y, t), vshrq_s32_v(vandq(x, m), t));
}
AI V inv32(V N) {                                   /* N odd: N^-1 mod 2^32, seed (3N) xor 2, three steps by mls, mul */
    const V two = C32_(2);
    V i = veorq(vaddq_s32(vaddq_s32(N, N), N), two);
    _Pragma("GCC unroll 4") for (int k = 0; k < 3; k++) i = vmulq_s32(i, vmlsq_s32(two, N, i));
    return i;
}
AI u32 m2_body(ARGS, int pop) {
    V fq = C32_(IMAX), fz = C32_(-1); const V one = C32_(1);
    for (size_t i = 0; i < n; i += 4) {
        V a = vld1q(pa + i), b = vld1q(pb + i), c = vld1q(pc + i), d = vld1q(pd + i);
        V o = vorrq(c, d); fz = vminq_u32(fz, o);
        V k = pop ? ctz_pop(o) : ctz_clz(o);
        a = vshrq_s32_v(a, k); b = vshrq_s32_v(b, k); c = vshrq_s32_v(c, k); d = vshrq_s32_v(d, k);
        V m = vtstq_s32(vandq(c, d), one), t = vshrq_n_u32(m, 31);
        div1i(&c, &d, t, m);
#if FAULT != 2
        div1i(&a, &b, t, m);
#endif
        V iv = inv32(vmlaq_s32(vmulq_s32(c, c), d, d));
        V wr = vmulq_s32(c, iv), wi = vmulq_s32(d, iv);                /* conj(y') / N(y') = wr - wi i */
        V qre = vmlaq_s32(vmulq_s32(a, wr), b, wi), qim = vmlsq_s32(vmulq_s32(b, wr), a, wi);
        vst1q(pqr + i, qre); vst1q(pqi + i, qim);
        fq = vminq_s32(vminq_s32(fq, qre), qim);
    }
    return flags_out(fq, fz);
}
/* shared divisor: k, the 1+i step and w = conj(y') N(y')^-1 once; per element a shift, the 1+i step, 4 multiplies */
AI u32 m2s_body(ARGS) {
    i32 c = pc[0], d = pd[0]; u32 fl = 0;
    if (!(c | d)) { fl = 2; c = 1; }
    int k = __builtin_ctz((u32)(c | d));
    c >>= k; d >>= k;
    int t = c & d & 1;
    if (t) { i32 hc = c >> 1, hd = d >> 1; c = hc + hd + 1; d = hd - hc; }
    u32 N = (u32)c * (u32)c + (u32)d * (u32)d, iv = 3 * N ^ 2;
    for (int j = 0; j < 3; j++) iv *= 2 - N * iv;
    const V wr = C32_((u32)c * iv), wi = C32_((u32)d * iv), one = C32_(1);
    const V ck = SHCNT(k);
    V fq = C32_(IMAX);
#define M2S_LOOP(T)                                                                                         \
    for (size_t i = 0; i < n; i += 4) {                                                                      \
        V a = vshrq_s32_r(vld1q(pa + i), ck), b = vshrq_s32_r(vld1q(pb + i), ck);                             \
        if (T) { V ha = vshrq_n_s32(a, 1), hb = vshrq_n_s32(b, 1);                                           \
                 a = vaddq_s32(vaddq_s32(ha, hb), vandq(a, one)); b = vsubq_s32(hb, ha); }                   \
        V qre = vmlaq_s32(vmulq_s32(a, wr), b, wi), qim = vmlsq_s32(vmulq_s32(b, wr), a, wi);                \
        vst1q(pqr + i, qre); vst1q(pqi + i, qim);                                                            \
        fq = vminq_s32(vminq_s32(fq, qre), qim);                                                             \
    }
    if (t) { M2S_LOOP(1) } else { M2S_LOOP(0) }
    return fl | (vany(vceqq_s32(fq, C32_(IMIN))) ? 1 : 0);
}

/* ======================= kernels ======================= */
#define K1(name, body) NOINL u32 name(ARGS) { return body; }
#define KR(p, body) K1(p##_fl, body(FL)) K1(p##_na, body(NA)) K1(p##_ma, body(NA | MAG)) K1(p##_nd, body(ND)) K1(p##_md, body(ND | MAG)) \
                    K1(p##_b1, body(B1)) K1(p##_mb1, body(B1 | MAG)) K1(p##_b2, body(B2)) K1(p##_mb2, body(B2 | MAG)) K1(p##_ex, body(EX))
#define BM1(md) m1_body(PASS, md, 0)
#define BM4(md) m4_body(PASS, md)
#define BM5(md) m5_body(PASS, md, 1)
#define BM5X(md) m5_body(PASS, md, 2)
KR(s_m1, BM1) KR(k_m4, BM4) KR(k_m5, BM5) KR(k_m5x, BM5X)
K1(s_m1s_ex, m1_body(PASS, EX, 1)) K1(s_m1s_fl, m1_body(PASS, FL, 1))
K1(k_m2_ex, m2_body(PASS, 0)) K1(k_m2p_ex, m2_body(PASS, 1)) K1(k_m2s_ex, m2s_body(PASS))
K1(k_m5x3_fl, m5_body(PASS, FL, 3)) K1(k_m5x3_mb2, m5_body(PASS, B2 | MAG, 3))

#ifndef A64
enum { VAR = 0, SHR = 1 };
typedef struct { const char *name; kf f; int mode, shared; } kern;
#define KE(p, nm, sh) {nm "_fl", p##_fl, FL, sh}, {nm "_na", p##_na, NA, sh}, {nm "_ma", p##_ma, NA | MAG, sh}, {nm "_nd", p##_nd, ND, sh}, \
    {nm "_md", p##_md, ND | MAG, sh}, {nm "_b1", p##_b1, B1, sh}, {nm "_mb1", p##_mb1, B1 | MAG, sh}, {nm "_b2", p##_b2, B2, sh}, \
    {nm "_mb2", p##_mb2, B2 | MAG, sh}, {nm "_ex", p##_ex, EX, sh}
static kern KT[] = {
    KE(s_m1, "m1", VAR), KE(k_m5, "m5", VAR), KE(k_m5x, "m5x", VAR), KE(k_m4, "m4", SHR),
    {"m1s_ex", s_m1s_ex, EX, SHR}, {"m1s_fl", s_m1s_fl, FL, SHR},
    {"m2_ex", k_m2_ex, EX, VAR}, {"m2p_ex", k_m2p_ex, EX, VAR}, {"m2s_ex", k_m2s_ex, EX, SHR},
    {"m5x3_fl", k_m5x3_fl, FL, VAR}, {"m5x3_mb2", k_m5x3_mb2, B2 | MAG, VAR},
};
#define NKT (int)(sizeof KT / sizeof KT[0])
#define NMODE 6

NOINL int resolve(ARGS, int mode) {
    int ov = 0;
    for (size_t i = 0; i < n; i++) {
        i64 a = pa[i], b = pb[i], c = pc[i], d = pd[i];
        i64 X = (i64)((u64)(a * c) + (u64)(b * d)), Y = b * c - a * d;
        i64 q1 = pqr[i], q2 = pqi[i];
        if (q1 == IMIN && X > 0) { q1 = -q1; ov = 1; }
        if (q2 == IMIN && Y > 0) { q2 = -q2; ov = 1; }
        if (NEAR(mode)) { i64 r1 = a - (q1 * c - q2 * d), r2 = b - (q1 * d + q2 * c); if (r1 > IMAX || r2 > IMAX) ov = 1; }
    }
    return ov;
}

/* ======================= reference (__int128), from the exact rationals ======================= */
static i128 fdiv(i128 x, i128 y) { i128 q = x / y; if ((x % y != 0) && ((x < 0) != (y < 0))) q--; return q; }
typedef struct { i128 q1, q2, r1, r2; int ov; } ref_t;
static ref_t ref(i64 a, i64 b, i64 c, i64 d, int mode) {
    ref_t R; i128 N = (i128)c * c + (i128)d * d, X = (i128)a * c + (i128)b * d, Y = (i128)b * c - (i128)a * d;
    i128 Z[2] = {X, Y}, q[2];
    for (int p = 0; p < 2; p++) {
        i128 fl = fdiv(Z[p], N), rem = Z[p] - fl * N;
        q[p] = mode >= NA ? fl + (2 * rem > N) : fl;
        if (mode >= NA && 2 * rem == N) {
            i128 away = Z[p] > 0 ? fl + 1 : fl, toward = Z[p] > 0 ? fl : fl + 1;
            q[p] = mode == NA ? fl + 1 : mode == ND ? fl : mode == B1 ? away : toward;
        }
    }
    R.q1 = q[0]; R.q2 = q[1];
    R.r1 = a - (R.q1 * c - R.q2 * d); R.r2 = b - (R.q1 * d + R.q2 * c);
    R.ov = R.q1 > IMAX || R.q1 < IMIN || R.q2 > IMAX || R.q2 < IMIN;
    if (mode >= NA) R.ov |= R.r1 > IMAX || R.r1 < IMIN || R.r2 > IMAX || R.r2 < IMIN;
    return R;
}

/* ======================= correctness (divvec.c's harness) ======================= */
static i64 rr(i64 lo, i64 hi) { return lo + (i64)(next() % (u64)(hi - lo + 1)); }
static long fails, nchecked, novf, nflagged, kfails[64];
#define BMAX (1 << 16)
static i32 A[BMAX + 8], B[BMAX + 8], C[BMAX + 8], D[BMAX + 8];
static i32 O[4][BMAX + 8];
static int exact_ok(i64 a, i64 b, i64 c, i64 d) {
    i128 N = (i128)c * c + (i128)d * d; if (N == 0) return 0;
    return ((i128)a * c + (i128)b * d) % N == 0 && ((i128)b * c - (i128)a * d) % N == 0;
}
#define FAIL_(k, ...) do { kfails[k]++; if (fails++ < 20) printf(__VA_ARGS__); } while (0)
static void check_batch(size_t n, int exact, int flagcheck) {
    size_t N4 = (n + 3) & ~(size_t)3;
    for (size_t j = n; j < N4; j++) { A[j] = B[j] = 0; C[j] = C[0]; D[j] = D[0]; }
    int same = 1; for (size_t j = 1; j < n; j++) if (C[j] != C[0] || D[j] != D[0]) same = 0;
    int anyzero = 0, anyov[NMODE] = {0};
    static ref_t RF[NMODE][BMAX];
    for (size_t j = 0; j < n; j++) {
        if (!C[j] && !D[j]) { anyzero = 1; continue; }
        for (int m = 0; m < NMODE; m++) { if (exact && m) break; if (!exact && !m) continue;
            RF[m][j] = ref(A[j], B[j], C[j], D[j], m); anyov[m] |= RF[m][j].ov; }
    }
    for (int k = 0; k < NKT; k++) {
        int m = RULE(KT[k].mode);
        if ((m == EX) != exact) continue;
        if (KT[k].shared && !same) continue;
        u32 fl = KT[k].f(A, B, C, D, O[0], O[1], O[2], O[3], N4);
        nchecked += n;
        for (size_t j = 0; j < n; j++) {
            if (!C[j] && !D[j]) continue;
            ref_t R = RF[m][j]; int bad = 0;
            if ((u32)O[0][j] != (u32)(u64)R.q1 || (u32)O[1][j] != (u32)(u64)R.q2) bad = 1;
            if (m != EX && ((u32)O[2][j] != (u32)(u64)R.r1 || (u32)O[3][j] != (u32)(u64)R.r2)) bad = 1;
            if (bad) FAIL_(k, "FAIL %s (%d,%d)/(%d,%d): got q (%d,%d) r (%d,%d), want q (%lld,%lld) r (%lld,%lld)\n",
                KT[k].name, A[j], B[j], C[j], D[j], O[0][j], O[1][j], O[2][j], O[3][j],
                (long long)R.q1, (long long)R.q2, (long long)R.r1, (long long)R.r2);
        }
        if (((fl >> 1) & 1) != (u32)anyzero) FAIL_(k, "FAIL %s zero flag\n", KT[k].name);
        if (anyov[m] && !(fl & 1)) FAIL_(k, "FAIL %s missed overflow (%d,%d)/(%d,%d)\n", KT[k].name, A[0], B[0], C[0], D[0]);
        if (flagcheck && !anyzero) {
            int ex = (fl & 1) ? resolve(A, B, C, D, O[0], O[1], O[2], O[3], n, m) : 0;
            if (ex != anyov[m]) FAIL_(k, "FAIL %s resolve (%d,%d)/(%d,%d)\n", KT[k].name, A[0], B[0], C[0], D[0]);
            if (KT[k].f == s_m1_fl || KT[k].f == s_m1_na) { novf += anyov[m]; nflagged += fl & 1; }
        }
    }
}
static size_t nb; static int bexact, bflag;
static void push(i64 a, i64 b, i64 c, i64 d) {
    A[nb] = (i32)a; B[nb] = (i32)b; C[nb] = (i32)c; D[nb] = (i32)d; nb++;
    if (nb == BMAX) { check_batch(nb, bexact, bflag); nb = 0; }
}
static void flush(void) { if (nb) check_batch(nb, bexact, bflag); nb = 0; }
static int rot;
/* one element alone in a register of 4 (position rotating), so the flags are per element; shared kernels run too */
static void alone(i64 a, i64 b, i64 c, i64 d, int exact) {
    for (int j = 0; j < 4; j++) { A[j] = 0; B[j] = 0; C[j] = (i32)c; D[j] = (i32)d; }
    int p = rot++ & 3; A[p] = (i32)a; B[p] = (i32)b;
    check_batch(4, exact, 1);
}
static const i64 EV[] = {IMIN, IMIN + 1, -(1 << 30) - 1, -(1 << 30), -46341, -65536, -3, -2, -1, 0,
                         1, 2, 3, 65536, 46341, 1 << 30, (1 << 30) + 1, IMAX - 1, IMAX, IMIN + 2};
#define NEV (int)(sizeof EV / sizeof EV[0])
static int fits(i64 v) { return v >= IMIN && v <= IMAX; }

static void do_check(int quick) {
    for (int K = 3; K <= (quick ? 5 : 6); K++) {
        i64 lo = -(1 << (K - 1)), hi = (1 << (K - 1)) - 1; long cnt = 0, ce = 0;
        bexact = 0; bflag = 1;
        for (i64 a = lo; a <= hi; a++) for (i64 b = lo; b <= hi; b++) for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++) { push(a, b, c, d); cnt++; }
        flush();
        bexact = 1;
        for (i64 q1 = lo; q1 <= hi; q1++) for (i64 q2 = lo; q2 <= hi; q2++) for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++)
            if (c | d) { push(q1 * c - q2 * d, q1 * d + q2 * c, c, d); ce++; }
        flush();
        for (i64 c = lo; c <= hi; c++) for (i64 d = lo; d <= hi; d++) {
            if (!(c | d)) continue;
            bexact = 0; for (i64 a = lo; a <= hi; a++) for (i64 b = lo; b <= hi; b++) push(a, b, c, d); flush();
            bexact = 1; for (i64 q1 = lo; q1 <= hi; q1++) for (i64 q2 = lo; q2 <= hi; q2++) push(q1 * c - q2 * d, q1 * d + q2 * c, c, d); flush();
        }
        printf("K = %d: exhaustive %ld divisions with remainder, %ld exact (also per shared divisor); failures so far %ld\n", K, cnt, ce, fails);
        fflush(stdout);
    }
    long ne = 0, nx = 0;
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) for (int r = 0; r < NEV; r++) for (int s = 0; s < NEV; s++) {
        alone(EV[p], EV[q], EV[r], EV[s], 0); ne++;
        if (exact_ok(EV[p], EV[q], EV[r], EV[s])) { alone(EV[p], EV[q], EV[r], EV[s], 1); nx++; }
    }
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
    printf("edges: %ld divisions with remainder, %ld exact, each alone with flags; overflows %ld, flagged %ld (m1 fl, na); failures so far %ld\n", ne, nx, novf, nflagged, fails);
    fflush(stdout);
    bflag = 1; long nr = 0, nrx = 0;
    for (int it = 0; it < (quick ? 16 : 160); it++) {
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
            else { c = 2 * rr(-30000, 30000) + 1; d = 2 * rr(-30000, 30000) + 1; i64 q1 = rr(-30000, 30000), q2 = rr(-30000, 30000);
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
            if (kind == 2) { int s = (int)(next() % 8); c <<= s; d <<= s; q1 >>= s; q2 >>= s; }
            if (!(c | d)) c = 1;
            i64 x1 = q1 * c - q2 * d, x2 = q1 * d + q2 * c;
            if (fits(x1) && fits(x2) && fits(c) && fits(d)) { push(x1, x2, c, d); nrx++; }
        }
        flush();
    }
    printf("random: %ld divisions with remainder, %ld exact (batches of 65,536); failures so far %ld\n", nr, nrx, fails);
    for (int it = 0; it < 40; it++) {
        i64 c = it & 1 ? rr(IMIN, IMAX) : rr(-300, 300), d = it & 1 ? rr(IMIN, IMAX) : rr(-300, 300);
        if (it % 5 == 4) { c = IMIN; d = it & 2 ? IMIN : 0; }
        if (!(c | d)) c = 7;
        bexact = 0; for (int j = 0; j < BMAX; j++) push(EDGEOR(rr(IMIN, IMAX)), EDGEOR(rr(IMIN, IMAX)), c, d); flush();
    }
    printf("shared random: 40 divisors x 65,536; failures so far %ld\n", fails);
    printf("TOTAL failures %ld (%ld element results checked)\n", fails, nchecked);
    for (int k = 0; k < NKT; k++) if (kfails[k]) printf("  %-9s %ld failures\n", KT[k].name, kfails[k]);
}

/* exhaustive check of the packed reciprocal over every dh in [2^31, 2^32) */
static void do_recip(void) {
    long hist[4] = {0}, bad = 0;
    for (u64 base = 1ull << 31; base < (1ull << 32); base += 4) {
        V dh = _mm_setr_epi32((int)base, (int)(base + 1), (int)(base + 2), (int)(base + 3));
        V v0 = recip_newton_p(dh), v1 = recip_fix_p(dh, v0, RECIP_FIX);
        u32 a0[4], a1[4]; _mm_storeu_si128((V *)a0, v0); _mm_storeu_si128((V *)a1, v1);
        for (int j = 0; j < 4; j++) {
            u64 want = div128(1, 0, base + j + 1) - (1ull << 32);
            i64 def = (i64)want - (i64)a0[j];
            if (def < 0 || def > 2) hist[3]++; else hist[def]++;
            if (a1[j] != want) bad++;
        }
    }
    printf("packed reciprocal, all 2^31 values of dh: deficit after Newton 0: %ld, 1: %ld, 2: %ld, other: %ld; wrong after %d fix-up(s): %ld\n",
           hist[0], hist[1], hist[2], hist[3], RECIP_FIX, bad);
}

/* ======================= timing ======================= */
static i32 *P[8];
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
static void time_set(const int *ks, int nk, size_t n, u64 inner, double out[][3]) {
    static double t[64][REPS];
    for (int k = 0; k < nk; k++) KT[ks[k]].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
    for (int r = 0; r < REPS; r++)
        for (int j = 0; j < nk; j++) {
            int k = ks[(j + r) % nk];
            KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            u64 t0 = now_ns();
            for (u64 it = 0; it < inner; it++) KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            t[(j + r) % nk][r] = (double)(now_ns() - t0) / (double)(n * inner);
        }
    for (int k = 0; k < nk; k++) { qsort(t[k], REPS, sizeof(double), cmpd); out[k][0] = t[k][REPS / 2]; out[k][1] = t[k][REPS / 4]; out[k][2] = t[k][3 * REPS / 4]; }
}
static int kidx(const char *s) { for (int k = 0; k < NKT; k++) if (!strcmp(KT[k].name, s)) return k; printf("no kernel %s\n", s); exit(1); }
static void run_group(const char *title, const char **names, int nk, int kind, int shared, size_t n, u64 inner) {
    int ks[64]; for (int k = 0; k < nk; k++) ks[k] = kidx(names[k]);
    fill(n, kind, shared);
    double out[64][3]; time_set(ks, nk, n, inner, out);
    printf("%s\n", title);
    for (int k = 0; k < nk; k++) printf("  %-9s %7.3f  [%7.3f, %7.3f]\n", names[k], out[k][0], out[k][1], out[k][2]);
    fflush(stdout);
}
static void do_time(int mem) {
    size_t n = mem ? (1u << 24) : 1024; u64 inner = mem ? 1 : 512;
    alloc_bufs(n);
    printf("ns per element, %s, n = %zu; median [q1, q3] of %d reps, kernels interleaved, order rotated\n", mem ? "from memory" : "in L1", n, REPS);
    const char *vr[] = {"m1_fl", "m5_fl", "m5x_fl", "m5x3_fl", "m1_mb2", "m5_mb2", "m5x_mb2", "m5x3_mb2"};
    const char *vx[] = {"m1_ex", "m2_ex", "m2p_ex", "m5x_ex"};
    const char *sr[] = {"m1s_fl", "m4_fl", "m4_mb2"};
    const char *sx[] = {"m1s_ex", "m2s_ex", "m4_ex"};
    run_group("varying divisor, remainder, random 32-bit", vr, 8, 0, 0, n, inner);
    run_group("varying divisor, remainder, small divisor (|parts| <= 256)", vr, 8, 1, 0, n, inner);
    run_group("varying divisor, exact, q and y parts below 2^15", vx, 4, 2, 0, n, inner);
    run_group("shared divisor, remainder, random 32-bit divisor", sr, 3, 0, 1, n, inner);
    run_group("shared divisor, remainder, small divisor", sr, 3, 1, 1, n, inner);
    run_group("shared divisor, exact, q and y parts below 2^15", sx, 3, 2, 1, n, inner);
}
static void do_ties(int mem) {
    size_t n = mem ? (1u << 24) : 1024; u64 inner = mem ? 1 : 512;
    alloc_bufs(n);
    printf("tie rules, ns per element, %s, n = %zu; median [q1, q3] of %d reps, kernels interleaved, order rotated\n", mem ? "from memory" : "in L1", n, REPS);
    const char *sfx[] = {"fl", "na", "ma", "nd", "md", "b2", "mb2", "b1", "mb1"};
    const char *meth[] = {"m1", "m5x", "m4"};
    for (int m = 0; m < 3; m++) {
        static char nbuf[9][16]; const char *nm[9];
        for (int k = 0; k < 9; k++) { snprintf(nbuf[k], 16, "%s_%s", meth[m], sfx[k]); nm[k] = nbuf[k]; }
        char title[64]; snprintf(title, 64, "%s, random 32-bit%s", meth[m], m == 2 ? " (shared divisor)" : "");
        run_group(title, nm, 9, 0, m == 2, n, inner);
    }
}
static void do_count(void) {
#ifdef COUNT
    size_t n = 4096; alloc_bufs(n);
    printf("NEON instructions per element (all / of which multiplies / loads+stores), random 32-bit inputs (exact: q, y below 2^15)\n");
    for (int k = 0; k < NKT; k++) {
        if (KT[k].name[0] == 'm' && KT[k].name[1] == '1') continue;
        int ex = RULE(KT[k].mode) == EX;
        fill(n, ex ? 2 : 0, KT[k].shared);
        nops = nmem = nmul = 0;
        KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
        printf("  %-9s %6.2f / %5.2f / %4.2f\n", KT[k].name, (double)nops / n, (double)nmul / n, (double)nmem / n);
    }
#else
    printf("build with -DCOUNT\n");
#endif
}

/* every kernel once on 4,096 elements (random; exact kinds for EX), for a dynamic x86 count under callgrind */
static void do_icount(void) {
    size_t n = 4096; alloc_bufs(n);
    for (int k = 0; k < NKT; k++) { fill(n, RULE(KT[k].mode) == EX ? 2 : 0, KT[k].shared); KT[k].f(P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n); }
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check(0);
    else if (!strcmp(mode, "quick")) do_check(1);
    else if (!strcmp(mode, "recip")) do_recip();
    else if (!strcmp(mode, "time")) do_time(0);
    else if (!strcmp(mode, "mem")) do_time(1);
    else if (!strcmp(mode, "ties")) do_ties(0);
    else if (!strcmp(mode, "tiesmem")) do_ties(1);
    else if (!strcmp(mode, "count")) do_count();
    else if (!strcmp(mode, "icount")) do_icount();
    else if (!strcmp(mode, "scan")) scan_kernels(argv[0]);
    return 0;
}
#endif /* A64 */
