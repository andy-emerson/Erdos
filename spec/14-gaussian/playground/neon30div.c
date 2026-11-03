/* Study 30, part B: Gaussian division with remainder reshaped for 32-bit lanes (neon30div.c).
 * Includes neon29div.c (study 29's kernels m1, m4, m5x and its reference and resolve()).
 *
 * Rule: nearest, ties toward zero (decision 28), the same function as study 29's B2 / mask form; q and r mod 2^32;
 * flag bit 0 = some q or r part is -2^31 (resolve() decides), bit 1 = a zero divisor.
 *
 * The reshape: an estimate from the Gaussian reciprocal in 32-bit fixed point, one one-sided exact test.
 *   W1 = round(c 2^(31+T) / N), W2 = round(d 2^(31+T) / N), with 2^(2T+2) <= N (so |W| <= 2^30, T >= 3 for N >= 256).
 *   s1 = sqrdmulh(a, W1) + sqrdmulh(b, W2), s2 = sqrdmulh(b, W1) - sqrdmulh(a, W2), q^ = s >> T (floor).
 *   |s / 2^T - X / N| <= 2 / 2^T <= 1/4, so q^ is q - 1 or q for the nearest q (and at a tie, the toward-zero one).
 *   One test per part: rho = X - q^ N (exact), round up iff rho + s > H, H = N >> 1, s = [q^ < 0] when N is even
 *   (0 when N is odd: no ties). That is ties toward zero: at rho = N/2 the part goes up exactly when q^ < 0.
 *   s1 (r^ form): r^ = x - q^ y in 32-bit lanes (mla/mls), exact while |r^| <= (1/sqrt2 + sqrt2)|y| < 2^31,
 *       i.e. N <= 2^59; then rho1 = r^1 c + r^2 d, rho2 = r^2 c - r^1 d by smull/smlal (Re and Im of r^ conj y).
 *   s2 (X form, any N >= 256): rho1 = X - q^1 N from smull(a,c) + smlal(b,d) - smull(q^1, Nl') - (q^1 Nh' mod 2^32) 2^32,
 *       N = Nh' 2^32 + Nl' in signed-low pieces; true rho is in [-N/4, 5N/4], exact mod 2^64.
 *   Shared divisor: W, T, H once per array (scalar); N < 256 (or N > 2^59 for s1) goes to study 29's m4, once per array.
 *   Per-lane divisor (lane kernels, below).
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -mavx512bitalg -o neon30div neon30div.c
 *        (-DCOUNT, -DFAULT30=k); clang-18 --target=aarch64-linux-gnu -ffreestanding -O2 -DA64 -S neon30div.c
 * Run:   ./neon30div check | quick | time | count | scan
 */
#define main main29_unused
#include "neon29div.c"
#undef main
#ifndef FAULT30
#define FAULT30 0
#endif
/* faults: 1 T one too large (error bound broken); 2 no tie bit; 3 tie bit for odd N too; 4 floor -> round in the
 *         estimate; 5 S1 limit 2^61 instead of 2^59; 6 s2 without the Nh' term; 7 lane: no second stage */

#ifndef A64
/* sqrdmulh .4s: round(2ab / 2^32), saturating only at (-2^31)^2 (never reached here: |W| <= 2^30) */
AI V qrdmulh_(V a, V b) {
    const V rc = _mm_set1_epi64x(1ll << 30);
    V e = _mm_srai_epi64(_mm_add_epi64(_mm_mul_epi32(a, b), rc), 31);
    V o = _mm_slli_epi64(_mm_srai_epi64(_mm_add_epi64(_mm_mul_epi32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32)), rc), 31), 32);
    return _mm_blend_epi32(e, o, 0xA);
}
#define vqrdmulhq_s32(a, b) X_(qrdmulh_(a, b))
#else
#define vqrdmulhq_s32(a, b) FS32_(vqrdmulhq_s32(S32_(a), S32_(b)))
#endif

typedef struct { int ok; int T; i32 W1, W2; u64 N, H; i32 Nl, Nh, e; } recip_t;
static recip_t shared_recip(i32 c, i32 d, u64 lim) {
    recip_t R = {0};
    u64 N = (u64)((i64)c * c) + (u64)((i64)d * d);
    if (N < 256 || N > lim) return R;
    int L = 63 - __builtin_clzll(N), T = (L - 2) >> 1;
#if FAULT30 == 1
    T++;
#endif
    i128 s = (i128)1 << (31 + T);
    i128 w1 = (i128)c * s, w2 = (i128)d * s;          /* round half away; any rounding within 1/2 works */
    w1 = w1 >= 0 ? (w1 + (i128)(N / 2)) / (i128)N : -((-w1 + (i128)(N / 2)) / (i128)N);
    w2 = w2 >= 0 ? (w2 + (i128)(N / 2)) / (i128)N : -((-w2 + (i128)(N / 2)) / (i128)N);
    R.ok = 1; R.T = T; R.W1 = (i32)w1; R.W2 = (i32)w2; R.N = N; R.H = N >> 1;
    R.Nl = (i32)(u32)N; R.Nh = (i32)((N + 0x80000000ull) >> 32);   /* N = Nh 2^32 + Nl, Nl signed */
#if FAULT30 == 3
    R.e = 1;
#elif FAULT30 == 2
    R.e = 0;
#else
    R.e = !(N & 1);
#endif
    return R;
}
/* the estimate: q^ = floor((sqrdmulh(a, W1) + sqrdmulh(b, W2)) / 2^T) and the conjugate part */
AI void est(V a, V b, V W1, V W2, V cT, V *q1, V *q2) {
    V s1 = vaddq_s32(vqrdmulhq_s32(a, W1), vqrdmulhq_s32(b, W2));
    V s2 = vsubq_s32(vqrdmulhq_s32(b, W1), vqrdmulhq_s32(a, W2));
#if FAULT30 == 4
    const V hT = vshrq_s32_r(C32_(INT32_MIN), cT); s1 = vsubq_s32(s1, hT); s2 = vsubq_s32(s2, hT);  /* + 2^(T-1): round */
#endif
    *q1 = vshrq_s32_r(s1, cT); *q2 = vshrq_s32_r(s2, cT);
}
/* the one-sided test for one part: rho (a pair) + tie bit > H -> q + 1 */
AI V up1(V q, P2 rho, V e, V Hv) {
    V s = vandq(vshrq_n_u32(q, 31), e);
    V gl = vcgtq_s64(vaddw_lo_u32(rho.l, s), Hv), gh = vcgtq_s64(vaddw_high_u32(rho.h, s), Hv);
    return vsubq_s32(q, vuzp1q_w(gl, gh));
}
AI void out_qr(V a, V b, V c, V d, V q1, V q2, i32 *qr, i32 *qi, i32 *rr, i32 *ri, V *fq) {
    V r1 = vmlsq_s32(vmlaq_s32(a, q2, d), q1, c), r2 = vmlsq_s32(vmlsq_s32(b, q1, d), q2, c);
    vst1q(qr, q1); vst1q(qi, q2); vst1q(rr, r1); vst1q(ri, r2);
    *fq = vminq_s32(*fq, vminq_s32(vminq_s32(q1, q2), vminq_s32(r1, r2)));
}

/* ======================= s1: shared divisor, r^ form ======================= */
#if FAULT30 == 5
#define S1LIM (1ull << 62)
#else
#define S1LIM (1ull << 59)
#endif
NOINL u32 k_s1(ARGS) {
    recip_t R = shared_recip(pc[0], pd[0], S1LIM);
    if (!R.ok) return k_m4_mb2(PASS);
    const V W1 = C32_(R.W1), W2 = C32_(R.W2), cT = SHCNT(R.T), e = C32_(R.e), Hv = C64_(R.H), c = C32_(pc[0]), d = C32_(pd[0]);
    V fq = C32_(IMAX);
    for (size_t i = 0; i < n; i += 4) {
        V a = vld1q(pa + i), b = vld1q(pb + i), q1, q2;
        est(a, b, W1, W2, cT, &q1, &q2);
        V r1 = vmlsq_s32(vmlaq_s32(a, q2, d), q1, c), r2 = vmlsq_s32(vmlsq_s32(b, q1, d), q2, c);
        P2 p1 = {vmlal_lo_s32(vmull_lo_s32(r1, c), r2, d), vmlal_high_s32(vmull_high_s32(r1, c), r2, d)};
        P2 p2 = {vmlsl_lo_s32(vmull_lo_s32(r2, c), r1, d), vmlsl_high_s32(vmull_high_s32(r2, c), r1, d)};
        q1 = up1(q1, p1, e, Hv); q2 = up1(q2, p2, e, Hv);
        out_qr(a, b, c, d, q1, q2, pqr + i, pqi + i, prr + i, pri + i, &fq);
    }
    return vany(vceqq_s32(fq, C32_(IMIN))) ? 1 : 0;
}
/* ======================= s2: shared divisor, X form ======================= */
NOINL u32 k_s2(ARGS) {
    recip_t R = shared_recip(pc[0], pd[0], ~0ull);
    if (!R.ok) return k_m4_mb2(PASS);
    const V W1 = C32_(R.W1), W2 = C32_(R.W2), cT = SHCNT(R.T), e = C32_(R.e), Hv = C64_(R.H), c = C32_(pc[0]), d = C32_(pd[0]);
    const V Nl = C32_(R.Nl), Nh = C32_(R.Nh);
    V fq = C32_(IMAX);
    for (size_t i = 0; i < n; i += 4) {
        V a = vld1q(pa + i), b = vld1q(pb + i), q1, q2;
        est(a, b, W1, W2, cT, &q1, &q2);
#if FAULT30 == 6
        V t1 = vzero(), t2 = vzero(); (void)Nh;
#else
        V t1 = vmulq_s32(q1, Nh), t2 = vmulq_s32(q2, Nh);
#endif
        P2 p1 = {vsubq_s64(vmlsl_lo_s32(vmlal_lo_s32(vmull_lo_s32(a, c), b, d), q1, Nl), vshll32_lo(t1)),
                 vsubq_s64(vmlsl_high_s32(vmlal_high_s32(vmull_high_s32(a, c), b, d), q1, Nl), vshll32_high(t1))};
        P2 p2 = {vsubq_s64(vmlsl_lo_s32(vmlsl_lo_s32(vmull_lo_s32(b, c), a, d), q2, Nl), vshll32_lo(t2)),
                 vsubq_s64(vmlsl_high_s32(vmlsl_high_s32(vmull_high_s32(b, c), a, d), q2, Nl), vshll32_high(t2))};
        q1 = up1(q1, p1, e, Hv); q2 = up1(q2, p2, e, Hv);
        out_qr(a, b, c, d, q1, q2, pqr + i, pqi + i, prr + i, pri + i, &fq);
    }
    return vany(vceqq_s32(fq, C32_(IMIN))) ? 1 : 0;
}

#ifndef A64
/* ======================= correctness ======================= */
typedef struct { const char *name; kf f; int shared; } kern30;
static kern30 KD[] = { {"m4_mb2", k_m4_mb2, 1}, {"s1", k_s1, 1}, {"s2", k_s2, 1}, {"m1_mb2", s_m1_mb2, 0}, {"m5x_mb2", k_m5x_mb2, 0} };
#define NKD (int)(sizeof KD / sizeof KD[0])
static long nties, dfail, dchk, kdf[16], newpath[16];
static i32 QA[BMAX + 8], QB[BMAX + 8], QC[BMAX + 8], QD[BMAX + 8], QO[4][BMAX + 8];
static void dbatch(size_t n, int ksel) {   /* ksel: bitmask of kernels to run */
    size_t N4 = (n + 3) & ~(size_t)3;
    for (size_t j = n; j < N4; j++) { QA[j] = QB[j] = 0; QC[j] = QC[0]; QD[j] = QD[0]; }
    int same = 1; for (size_t j = 1; j < n; j++) if (QC[j] != QC[0] || QD[j] != QD[0]) same = 0;
    static ref_t RF[BMAX]; int anyzero = 0, anyov = 0;
    for (size_t j = 0; j < n; j++) { if (!QC[j] && !QD[j]) { anyzero = 1; continue; } RF[j] = ref(QA[j], QB[j], QC[j], QD[j], B2); anyov |= RF[j].ov;
        i128 Nn = (i128)QC[j] * QC[j] + (i128)QD[j] * QD[j], Xx = (i128)QA[j] * QC[j] + (i128)QB[j] * QD[j], Yy = (i128)QB[j] * QC[j] - (i128)QA[j] * QD[j];
        if (Nn >= 256 && (2 * (((Xx % Nn) + Nn) % Nn) == Nn || 2 * (((Yy % Nn) + Nn) % Nn) == Nn)) nties++; }
    for (int k = 0; k < NKD; k++) {
        if (!((ksel >> k) & 1) || (KD[k].shared && !same)) continue;
        u32 fl = KD[k].f(QA, QB, QC, QD, QO[0], QO[1], QO[2], QO[3], N4);
        dchk += n;
        for (size_t j = 0; j < n; j++) {
            if (!QC[j] && !QD[j]) continue;
            ref_t R = RF[j];
            if ((u32)QO[0][j] != (u32)(u64)R.q1 || (u32)QO[1][j] != (u32)(u64)R.q2 || (u32)QO[2][j] != (u32)(u64)R.r1 || (u32)QO[3][j] != (u32)(u64)R.r2) {
                kdf[k]++; if (dfail++ < 20) printf("FAIL %s (%d,%d)/(%d,%d): got q (%d,%d) r (%d,%d), want q (%lld,%lld) r (%lld,%lld)\n", KD[k].name,
                    QA[j], QB[j], QC[j], QD[j], QO[0][j], QO[1][j], QO[2][j], QO[3][j], (long long)R.q1, (long long)R.q2, (long long)R.r1, (long long)R.r2); }
        }
        if (KD[k].shared && !anyzero) {
            u64 N = (u64)((i64)QC[0] * QC[0]) + (u64)((i64)QD[0] * QD[0]);
            if (N >= 256 && (k != 1 || N <= S1LIM)) newpath[k] += n;
        }
        if (((fl >> 1) & 1) != (u32)anyzero) { kdf[k]++; if (dfail++ < 20) printf("FAIL %s zero flag\n", KD[k].name); }
        if (anyov && !(fl & 1)) { kdf[k]++; if (dfail++ < 20) printf("FAIL %s missed overflow\n", KD[k].name); }
        if (!anyzero && (fl & 1) && resolve(QA, QB, QC, QD, QO[0], QO[1], QO[2], QO[3], n, B2) != anyov) { kdf[k]++; if (dfail++ < 20) printf("FAIL %s resolve\n", KD[k].name); }
    }
}
static size_t qn; static int qsel;
static void qpush(i64 a, i64 b, i64 c, i64 d) { QA[qn] = (i32)a; QB[qn] = (i32)b; QC[qn] = (i32)c; QD[qn] = (i32)d; if (++qn == BMAX) { dbatch(qn, qsel); qn = 0; } }
static void qflush(void) { if (qn) dbatch(qn, qsel); qn = 0; }
static void qalone(i64 a, i64 b, i64 c, i64 d, int sel) {
    static int rot_; for (int j = 0; j < 4; j++) { QA[j] = 0; QB[j] = 0; QC[j] = (i32)c; QD[j] = (i32)d; }
    int p = rot_++ & 3; QA[p] = (i32)a; QB[p] = (i32)b; dbatch(4, sel);
}
#define SHR_ 0x7                      /* shared kernels: m4, s1, s2 */
static void do_dcheck(int quick, int sel) {
    /* 1. every divisor with |c|, |d| <= D0 and N >= 256 (and some below), x over every pair in a box and edge values */
    int D0 = quick ? 20 : 40; long n1 = 0;
    for (i64 c = -D0; c <= D0; c++) for (i64 d = -D0; d <= D0; d++) {
        if (!(c | d)) continue;
        qsel = sel; int B = quick ? 12 : 40;
        for (i64 a = -B * D0; a <= B * D0; a += (quick ? 7 : 3)) for (i64 b = -B * D0; b <= B * D0; b += (quick ? 11 : 5)) { qpush(a, b, c, d); n1++; }
        for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) { qpush(EV[p], EV[q], c, d); n1++; }
        qflush();
    }
    printf("small divisors (|c|,|d| <= %d) x box and edges: %ld; failures so far %ld\n", D0, n1, dfail); fflush(stdout);
    /* 2. ties: x = q y + t with 2 t / y in {1, i, 1+i} (needs 1+i | y for some) at many sizes */
    long n2 = 0;
    for (int it = 0; it < (quick ? 64 : 640); it++) {
        int e = 4 + (int)(next() % 27); i64 c = rr(-(1ll << e), 1ll << e), d = rr(-(1ll << e), 1ll << e);
        if (!(c | d)) c = 1;
        if ((c ^ d) & 1) d ^= 1;                   /* c = d mod 2: 1+i divides y, so ties exist */
        qsel = sel;
        for (int j = 0; j < BMAX; j++) {
            i64 qb = (i64)1 << (int)(next() % 31); i64 q1 = rr(-qb, qb), q2 = rr(-qb, qb);
            int u = (int)(next() % 3); i64 tr = u == 0 ? c : u == 1 ? -d : c - d, ti = u == 0 ? d : u == 1 ? c : c + d;
            i64 a = q1 * c - q2 * d + tr / 2 * 1, b = q1 * d + q2 * c + ti / 2;
            if ((tr & 1) || (ti & 1)) { a = q1 * c - q2 * d + (c - d) / 2; b = q1 * d + q2 * c + (c + d) / 2; }
            if (!fits(a) || !fits(b)) { a = rr(IMIN, IMAX); b = rr(IMIN, IMAX); }
            qpush(a, b, c, d); n2++;
        }
        qflush();
    }
    printf("tie-aimed batches: %ld; failures so far %ld\n", n2, dfail); fflush(stdout);
    /* 3. edges alone, divisor from edges and from N near the s1 limit 2^59 and near powers of 2 */
    long n3 = 0;
    for (int p = 0; p < NEV; p++) for (int q = 0; q < NEV; q++) for (int r = 0; r < NEV; r++) for (int s = 0; s < NEV; s++) { qalone(EV[p], EV[q], EV[r], EV[s], sel); n3++; }
    for (int it = 0; it < (quick ? 2000 : 20000); it++) {
        int L = 8 + (int)(next() % 56); u64 Nt = (1ull << L) + (u64)rr(-3, 3) * (L > 40 ? (1ull << (L - 34)) : 1);
        i64 c = (i64)(next() % (Nt > 1 ? (u64)1 << (L / 2) : 1)); i64 d2 = (i64)Nt - c * c; if (d2 < 0) d2 = 0;
        i64 d = (i64)__builtin_sqrtl((long double)d2); if (next() & 1) c = -c; if (next() & 1) d = -d;
        if (!fits(c) || !fits(d) || !(c | d)) continue;
        for (int k = 0; k < 4; k++) { qalone(EV[next() % NEV], EV[next() % NEV], c, d, sel); qalone(rr(IMIN, IMAX), rr(IMIN, IMAX), c, d, sel); n3 += 2; }
    }
    printf("edges alone, and divisors near powers of 2: %ld; failures so far %ld\n", n3, dfail); fflush(stdout);
    /* 4. random shared divisors of every size, random x */
    long n4 = 0;
    for (int it = 0; it < (quick ? 40 : 400); it++) {
        int e = (int)(next() % 32); i64 c = rr(-(1ll << e), (1ll << e) - 1), d = rr(-(1ll << e), (1ll << e) - 1);
        if (it % 9 == 8) { c = IMIN; d = it & 16 ? IMIN : rr(IMIN, IMAX); }
        if (!(c | d)) c = 7;
        qsel = sel; for (int j = 0; j < BMAX; j++) { qpush((next() & 15) ? rr(IMIN, IMAX) : EV[next() % NEV], (next() & 15) ? rr(IMIN, IMAX) : EV[next() % NEV], c, d); n4++; }
        qflush();
    }
    printf("random shared divisors: %ld; failures so far %ld\n", n4, dfail);
    /* 5. the s1 limit: N from 2^58 to 2^62, x placed so that x/y - q^ is near a corner of [-1/4, 5/4)^2 (largest r^) */
    long n5 = 0;
    for (int it = 0; it < (quick ? 400 : 4000); it++) {
        double ang = (double)(next() % 1000000) / 1000000.0 * 6.283185307179586, L = 29.0 + (double)(next() % 1000) / 1000.0 * 2.0;
        i64 c = (i64)(__builtin_cos(ang) * __builtin_pow(2.0, L)), d = (i64)(__builtin_sin(ang) * __builtin_pow(2.0, L));
        if (!fits(c) || !fits(d) || !(c | d)) continue;
        qsel = sel;
        for (int j = 0; j < 256; j++) {
            double f1 = (next() & 1) ? 0.999 - (double)(next() % 1000) * 1e-5 : -0.249 + (double)(next() % 1000) * 1e-5;
            double f2 = (next() & 1) ? 0.999 - (double)(next() % 1000) * 1e-5 : -0.249 + (double)(next() % 1000) * 1e-5;
            if (j & 1) { f1 = 0.5 + (f1 > 0.5 ? 1 : -1) * (double)(next() % 1000) * 1e-6; }
            i64 q1 = rr(-2, 2), q2 = rr(-2, 2);
            i64 a = q1 * c - q2 * d + (i64)(c * f1 - d * f2), b = q1 * d + q2 * c + (i64)(d * f1 + c * f2);
            if (!fits(a) || !fits(b)) continue;
            qpush(a, b, c, d); n5++;
        }
        qflush();
    }
    printf("s1 limit (N 2^58 to 2^62, x at the corners): %ld; failures so far %ld\n", n5, dfail);
    printf("elements with a tie (N >= 256): %ld\n", nties);
    printf("TOTAL failures %ld (%ld element results checked)\n", dfail, dchk);
    for (int k = 0; k < NKD; k++) if ((sel >> k) & 1) printf("  %-8s failures %ld, results on the new path %ld\n", KD[k].name, kdf[k], newpath[k]);
}

/* ======================= timing ======================= */
static void dtime(int mem) {
    size_t n = mem ? (1u << 24) : 1024; u64 inner = mem ? 1 : 512;
    alloc_bufs(n);
    static double t[8][REPS];
    printf("ns per element, %s, n = %zu; median [q1, q3] of %d reps, kernels interleaved, order rotated\n", mem ? "memory" : "L1", n, REPS);
    const char *kinds[3] = {"random 32-bit divisor", "small divisor (parts <= 256, N >= 256)", "medium divisor (parts <= 2^20)"};
    for (int kind = 0; kind < 3; kind++) {
        i64 c, d; u64 N;
        do { int e = kind == 0 ? 31 : kind == 1 ? 8 : 20; c = rr(-(1ll << e), (1ll << e) - 1); d = rr(-(1ll << e), (1ll << e) - 1); N = (u64)(c * c) + (u64)(d * d); }
        while (N < 256 || (kind == 0 && N < (1ull << 60)));
        for (size_t i = 0; i < n; i++) { P[0][i] = (i32)rr(IMIN, IMAX); P[1][i] = (i32)rr(IMIN, IMAX); P[2][i] = (i32)c; P[3][i] = (i32)d; }
        kf ks[4] = {s_m1s_fl, k_m4_mb2, k_s1, k_s2}; const char *nm[4] = {"m1s(floor)", "m4_mb2", "s1", "s2"};
        int nk = 4;
        for (int k = 0; k < nk; k++) ks[k](P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
        for (int r = 0; r < REPS; r++) for (int j = 0; j < nk; j++) {
            int k = (j + r) % nk; ks[k](P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            u64 t0 = now_ns(); for (u64 it = 0; it < inner; it++) ks[k](P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            t[k][r] = (double)(now_ns() - t0) / (double)(n * inner);
        }
        printf("shared, %s (N = 2^%.1f)%s\n", kinds[kind], __builtin_log2((double)N), kind == 0 ? " (s1 falls back to m4)" : "");
        for (int k = 0; k < nk; k++) { qsort(t[k], REPS, sizeof(double), cmpd); printf("  %-11s %7.3f [%7.3f, %7.3f]\n", nm[k], t[k][REPS / 2], t[k][REPS / 4], t[k][3 * REPS / 4]); }
        fflush(stdout);
    }
}
static void dcount(void) {
#ifdef COUNT
    size_t n = 4096; alloc_bufs(n);
    printf("NEON instructions per element (all / multiplies / loads+stores), shared divisor\n");
    i64 cs[2][2] = {{123457, -98765}, {1987654321, -1876543210}};
    for (int v = 0; v < 2; v++) {
        for (size_t i = 0; i < n; i++) { P[0][i] = (i32)rr(IMIN, IMAX); P[1][i] = (i32)rr(IMIN, IMAX); P[2][i] = (i32)cs[v][0]; P[3][i] = (i32)cs[v][1]; }
        kf ks[3] = {k_m4_mb2, k_s1, k_s2}; const char *nm[3] = {"m4_mb2", "s1", "s2"};
        for (int k = 0; k < 3; k++) { nops = nmem = nmul = 0; ks[k](P[0], P[1], P[2], P[3], P[4], P[5], P[6], P[7], n);
            printf("  %s divisor: %-7s %6.2f / %5.2f / %4.2f\n", v ? "large" : "medium", nm[k], (double)nops / n, (double)nmul / n, (double)nmem / n); }
    }
#else
    printf("build with -DCOUNT\n");
#endif
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_dcheck(0, SHR_);
    else if (!strcmp(mode, "quick")) do_dcheck(1, SHR_);
    else if (!strcmp(mode, "time")) dtime(0);
    else if (!strcmp(mode, "mem")) dtime(1);
    else if (!strcmp(mode, "count")) dcount();
    else if (!strcmp(mode, "scan")) scan_kernels(argv[0]);
    return 0;
}
#endif
