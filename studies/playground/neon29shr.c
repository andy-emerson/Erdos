/* Study 29, milestone 3: studies 20 and 21 (the shrink test's cost and its whole-run savings) in NEON shape (neon29shr.c)
 *
 * Gaussian integers in layout (b). Base size int32 halves, first grown size int64 halves. Overflow checks with a
 * sticky flag are always on. Rule B (fits in g bits iff v + 2^(g-1) has nothing at bit g or above), g = 31 at int64.
 *   bare  arithmetic only;  v1  + overflow check;  v2  v1 + a separate pass after every operation;
 *   v3    the test fused into add and subtract at int64 only (v3 = v1 elsewhere).
 *
 * What NEON lacks in the AVX2 code, and what replaces it (portable-isa.md):
 *   the end-of-array test (ptest)            umaxv + move + compare, once per array (vany_and)
 *   int32 multiply's even/odd vpmuldq, blend smull/smlsl + smull2/smlsl2, uzp1 (study 28's mul_w)
 *   int32 wrapping multiply (vpmulld)        mul, mls / mla (one NEON op each; pmulld is 2 µops here)
 *   int64 multiply: scalar in AVX2           NEON has no 64-bit lane multiply either. Both forms are kept:
 *                                            s_mul64 (scalar: on ARM mul + smulh per product) and k_mul64 (study 23's
 *                                            signed pieces, NEON-shaped, from neon29mul.c: k_sgn)
 *   promote (vpmovsxdq)                      sxtl, sxtl2
 *   demote (vpermd x2, vperm2i128)           uzp1 of two registers (one op per 4 values)
 *   int32 operand sign-extended on load      sxtl, sxtl2
 * The shrink test itself (add, orr per register; mask once per array) and the 64-bit xor overflow checks are the
 * same instructions on NEON. NEON alternatives considered for the test: cls (32-bit lanes only), smin/smax tracking
 * (no 64-bit min/max on NEON), saturating narrow with QC (QC is wanted for the overflow flag, decision 37, and tests
 * 32 bits, not 31). None is cheaper than add + orr.
 *
 * Build: gcc -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq -o neon29shr neon29shr.c (-DCOUNT, -DFAULT=k)
 * Run:   ./neon29shr check20 | time20 [Nbig] | check21 | time21 [Nmem] | count | scan
 */
#define NEON29_NO_MAIN
#include "neon29mul.c"
/* faults (on top of neon29mul.c's): 11 fused test with rule A's offset; 12 pass64 skips the imaginary part;
 * 13 demote with uzp2 (high halves); 14 int64 add overflow check without the AND NOT */

/* natural-order widen and narrow (through memory, so the stand-in must keep NEON's lane order) */
#ifndef A64
#define vmovl_nat_lo(p)    C_(_mm_cvtepi32_epi64(p))                                   /* sxtl */
#define vmovl_nat_high(p)  C_(_mm_cvtepi32_epi64(_mm_unpackhi_epi64(p, p)))            /* sxtl2 */
#if FAULT == 13
#define vuzp1_nat(a, b)    C_(_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), 0xDD)))
#else
#define vuzp1_nat(a, b)    C_(_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), 0x88)))   /* uzp1 */
#endif
#endif

#define AR32 const i32 *restrict ar, const i32 *restrict ai, const i32 *restrict br, const i32 *restrict bi, i32 *restrict cr, i32 *restrict ci, size_t N
#define AR64 const i64 *restrict ar, const i64 *restrict ai, const i64 *restrict br, const i64 *restrict bi, i64 *restrict cr, i64 *restrict ci, size_t N
#define OVA(x, y, r) vbicq(veorq(x, r), veorq(x, y))
#define OVS(x, y, r) vandq(veorq(x, r), veorq(x, y))
#if FAULT == 14
#define OVA64(x, y, r) veorq(x, r)
#else
#define OVA64 OVA
#endif
#define SIGN32 vdupq_n_s32(INT32_MIN)
#define SIGN64 vdupq_n_s64(INT64_MIN)
#define S32OFF vdupq_n_s32(1 << 14)
#define S32MASK vdupq_n_s32((int)0xFFFF8000u)
#if FAULT == 11
#define S64OFF vdupq_n_s64(1LL << 31)
#else
#define S64OFF vdupq_n_s64(1LL << 30)
#endif
#define S64MASK vdupq_n_s64((i64)0xFFFFFFFF80000000ULL)

/* ======================= study 20: base size, int32 halves (8 per pass) ======================= */
#define BARE32(NAME, OP) NOINL int NAME(AR32) {                                                   \
    for (size_t i = 0; i < N; i += 8) { _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4) { \
        vst1q(cr + i + h, OP(vld1q(ar + i + h), vld1q(br + i + h))); vst1q(ci + i + h, OP(vld1q(ai + i + h), vld1q(bi + i + h))); } } \
    return 0; }
BARE32(k_add32_b, vaddq_s32)
BARE32(k_sub32_b, vsubq_s32)
#define OVF32(NAME, OP, OVF) NOINL int NAME(AR32) {                                               \
    V o = vzero();                                                                               \
    for (size_t i = 0; i < N; i += 8) { _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4) { \
        V x = vld1q(ar + i + h), y = vld1q(br + i + h), u = vld1q(ai + i + h), v = vld1q(bi + i + h); \
        V r = OP(x, y), q = OP(u, v); vst1q(cr + i + h, r); vst1q(ci + i + h, q);                \
        o = vorrq(o, vorrq(OVF(x, y, r), OVF(u, v, q))); } }                                     \
    return vany_and(o, SIGN32); }
OVF32(k_add32_o, vaddq_s32, OVA)
OVF32(k_sub32_o, vsubq_s32, OVS)
NOINL int k_mul32_b(AR32) {
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        vst1q(cr + i, vmlsq_s32(vmulq_s32(a, c), b, d)); vst1q(ci + i, vmlaq_s32(vmulq_s32(a, d), b, c));
    }
    return 0;
}
NOINL int k_mul32_o(AR32) {      /* study 28's mul_w: smull/smlsl pairs, v + 2^31 has nothing in bits 32..63, uzp1 */
    const V off = vdupq_n_s64(1LL << 31); V o = vzero();
    for (size_t i = 0; i < N; i += 4) {
        V a = vld1q(ar + i), b = vld1q(ai + i), c = vld1q(br + i), d = vld1q(bi + i);
        V rl = vmlsl_lo_s32(vmull_lo_s32(a, c), b, d), rh = vmlsl_high_s32(vmull_high_s32(a, c), b, d);
        V il = vmlal_lo_s32(vmull_lo_s32(a, d), b, c), ih = vmlal_high_s32(vmull_high_s32(a, d), b, c);
        o = vorrq(o, vorrq(vorrq(vaddq_s64(rl, off), vaddq_s64(rh, off)), vorrq(vaddq_s64(il, off), vaddq_s64(ih, off))));
        vst1q(cr + i, vuzp1q_w(rl, rh)); vst1q(ci + i, vuzp1q_w(il, ih));
    }
    return vany_and(o, vdupq_n_s64((i64)0xFFFFFFFF00000000ULL));
}
NOINL int k_pass32(const i32 *restrict cr, const i32 *restrict ci, size_t N) {
    V s = vzero();
    for (size_t i = 0; i < N; i += 8) { _Pragma("GCC unroll 2") for (int h = 0; h < 8; h += 4)
        s = vorrq(s, vorrq(vaddq_s32(vld1q(cr + i + h), S32OFF), vaddq_s32(vld1q(ci + i + h), S32OFF))); }
    return vany_and(s, S32MASK);
}

/* ======================= study 20: first grown size, int64 halves (4 per pass) ======================= */
#define BARE64(NAME, OP) NOINL int NAME(AR64) {                                                   \
    for (size_t i = 0; i < N; i += 4) { _Pragma("GCC unroll 2") for (int h = 0; h < 4; h += 2) { \
        vst1q(cr + i + h, OP(vld1q(ar + i + h), vld1q(br + i + h))); vst1q(ci + i + h, OP(vld1q(ai + i + h), vld1q(bi + i + h))); } } \
    return 0; }
BARE64(k_add64_b, vaddq_s64)
BARE64(k_sub64_b, vsubq_s64)
#define OVF64(NAME, OP, OVF, FUSED) NOINL int NAME(AR64) {                                        \
    V o = vzero(), s = vzero();                                                                  \
    for (size_t i = 0; i < N; i += 4) { _Pragma("GCC unroll 2") for (int h = 0; h < 4; h += 2) { \
        V x = vld1q(ar + i + h), y = vld1q(br + i + h), u = vld1q(ai + i + h), v = vld1q(bi + i + h); \
        V r = OP(x, y), q = OP(u, v); vst1q(cr + i + h, r); vst1q(ci + i + h, q);                \
        if (FUSED) s = vorrq(s, vorrq(vaddq_s64(r, S64OFF), vaddq_s64(q, S64OFF)));              \
        o = vorrq(o, vorrq(OVF(x, y, r), OVF(u, v, q))); } }                                     \
    return vany_and(o, SIGN64) | (FUSED ? vany_and(s, S64MASK) << 1 : 0); }
OVF64(k_add64_o, vaddq_s64, OVA64, 0)
OVF64(k_sub64_o, vsubq_s64, OVS, 0)
OVF64(k_add64_f, vaddq_s64, OVA64, 1)
OVF64(k_sub64_f, vsubq_s64, OVS, 1)
NOINL int k_pass64(const i64 *restrict cr, const i64 *restrict ci, size_t N) {
    V s = vzero();
    for (size_t i = 0; i < N; i += 4) { _Pragma("GCC unroll 2") for (int h = 0; h < 4; h += 2)
#if FAULT == 12
        s = vorrq(s, vaddq_s64(vld1q(cr + i + h), S64OFF)); }
#else
        s = vorrq(s, vorrq(vaddq_s64(vld1q(cr + i + h), S64OFF), vaddq_s64(vld1q(ci + i + h), S64OFF))); }
#endif
    return vany_and(s, S64MASK);
}
/* the int64 multiply: scalar (study 20's mul64_b / mul64_o = s_bare / s_chk) and vector (k_bare / k_sgn) */
#define NAIVE(NAME, AR, OPO, PASSF) NOINL int NAME(AR) { int f = OPO(ar, ai, br, bi, cr, ci, N); return f | PASSF(cr, ci, N) << 1; }
NAIVE(k_add32_n, AR32, k_add32_o, k_pass32)
NAIVE(k_sub32_n, AR32, k_sub32_o, k_pass32)
NAIVE(k_mul32_n, AR32, k_mul32_o, k_pass32)
NAIVE(k_add64_n, AR64, k_add64_o, k_pass64)
NAIVE(k_sub64_n, AR64, k_sub64_o, k_pass64)
NAIVE(s_mul64_n, AR64, s_chk, k_pass64)
NAIVE(k_mul64_n, AR64, k_sgn, k_pass64)

#ifndef A64
typedef int (*k32_t)(AR32);
typedef int (*k64_t)(AR64);
enum { BARE, V1, V2, V3 };
typedef struct { const char *name; int w, op, var; void *f; } s20k;   /* op: 0 add, 1 sub, 2 mul (vector), 3 mul (scalar int64) */
static s20k KT[] = {
    {"add32 bare", 32, 0, BARE, k_add32_b}, {"add32 v1", 32, 0, V1, k_add32_o}, {"add32 v2", 32, 0, V2, k_add32_n},
    {"sub32 bare", 32, 1, BARE, k_sub32_b}, {"sub32 v1", 32, 1, V1, k_sub32_o}, {"sub32 v2", 32, 1, V2, k_sub32_n},
    {"mul32 bare", 32, 2, BARE, k_mul32_b}, {"mul32 v1", 32, 2, V1, k_mul32_o}, {"mul32 v2", 32, 2, V2, k_mul32_n},
    {"add64 bare", 64, 0, BARE, k_add64_b}, {"add64 v1", 64, 0, V1, k_add64_o}, {"add64 v2", 64, 0, V2, k_add64_n}, {"add64 v3", 64, 0, V3, k_add64_f},
    {"sub64 bare", 64, 1, BARE, k_sub64_b}, {"sub64 v1", 64, 1, V1, k_sub64_o}, {"sub64 v2", 64, 1, V2, k_sub64_n}, {"sub64 v3", 64, 1, V3, k_sub64_f},
    {"mul64v bare", 64, 2, BARE, k_bare}, {"mul64v v1", 64, 2, V1, k_sgn}, {"mul64v v2", 64, 2, V2, k_mul64_n},
    {"mul64s bare", 64, 3, BARE, s_bare}, {"mul64s v1", 64, 3, V1, s_chk}, {"mul64s v2", 64, 3, V2, s_mul64_n},
};
#define NKT (int)(sizeof KT / sizeof KT[0])
static int callk(int k, void *ar, void *ai, void *br, void *bi, void *cr, void *ci, size_t N) {
    if (KT[k].w == 32) return ((k32_t)KT[k].f)(ar, ai, br, bi, cr, ci, N);
    return ((k64_t)KT[k].f)(ar, ai, br, bi, cr, ci, N);
}

/* ---------- study 20 correctness (shrink20.c's harness) ---------- */
static i64 E32[64], E64[64]; static int NE32, NE64;
static void edges(i64 *E, int *NE, const int *bits, int nb, int w) {
    i128 lo = -((i128)1 << (w - 1)), hi = ((i128)1 << (w - 1)) - 1;
    E[(*NE)++] = 0; E[(*NE)++] = 1; E[(*NE)++] = -1;
    for (int k = 0; k < nb; k++) {
        i128 p = (i128)1 << bits[k];
        i128 c[] = {p - 2, p - 1, p, p + 1, -p - 1, -p, -p + 1, -p + 2};
        for (int j = 0; j < 8; j++) if (c[j] <= hi && c[j] >= lo) E[(*NE)++] = (i64)c[j];
    }
}
static i64 rval20(const i64 *E, int NE, int w, int small, int big) {
    u64 k = next() % 100; i64 v;
    if (k < 20) v = E[next() % NE];
    else if (k < 30) v = (i64)((u64)E[next() % NE] + (next() % 9) - 4);
    else { int b = (k < 30 + (u64)big) ? (int)(next() % w) : (int)(next() % (u64)(small + 1));
           v = b ? (i64)(next() >> (64 - b)) : 0; if (next() & 1) v = -v - (i64)(next() & 1); }
    return w == 32 ? (i64)(i32)v : v;
}
static long fails, cases, kfails[64];
static int fitw(i128 v, int w) { return v >= -((i128)1 << (w - 1)) && v < ((i128)1 << (w - 1)); }
static int fitsumw(i128 a, i128 b, int w) { i128 lo = -((i128)1 << (w - 1)), hi = ((i128)1 << (w - 1)) - 1; return b >= lo - a && b <= hi - a; }
static long seen[64][4];
static void check_one(int k, const i64 *Ar, const i64 *Ai, const i64 *Br, const i64 *Bi, size_t N) {
    static i32 a32[4][256], c32[2][256]; static i64 c64[2][256];
    int w = KT[k].w, op = KT[k].op & 3, var = KT[k].var, g = w == 32 ? 15 : 31, f;
    if (op == 3) op = 2;
    if (w == 32) {
        for (size_t j = 0; j < N; j++) { a32[0][j] = (i32)Ar[j]; a32[1][j] = (i32)Ai[j]; a32[2][j] = (i32)Br[j]; a32[3][j] = (i32)Bi[j]; }
        f = callk(k, a32[0], a32[1], a32[2], a32[3], c32[0], c32[1], N);
    } else f = callk(k, (void *)Ar, (void *)Ai, (void *)Br, (void *)Bi, c64[0], c64[1], N);
    int ovf = 0, nofit = 0;
    for (size_t j = 0; j < N; j++) {
        i128 a = Ar[j], b = Ai[j], c = Br[j], d = Bi[j]; u64 er, ei; int fr, fi;
        if (op == 0) { fr = fitw(a + c, w); fi = fitw(b + d, w); er = (u64)(a + c); ei = (u64)(b + d); }
        else if (op == 1) { fr = fitw(a - c, w); fi = fitw(b - d, w); er = (u64)(a - c); ei = (u64)(b - d); }
        else { fr = fitsumw(a * c, -(b * d), w); fi = fitsumw(a * d, b * c, w); er = (u64)(a * c) - (u64)(b * d); ei = (u64)(a * d) + (u64)(b * c); }
        if (!fr || !fi) ovf = 1; else if (!fitw((i64)er, g) || !fitw((i64)ei, g)) nofit = 1;
        u64 gr, gi;
        if (w == 32) { er = (uint32_t)er; ei = (uint32_t)ei; gr = (uint32_t)c32[0][j]; gi = (uint32_t)c32[1][j]; }
        else { gr = (u64)c64[0][j]; gi = (u64)c64[1][j]; }
        if (gr != er || gi != ei) { kfails[k]++; if (fails++ < 10) printf("FAIL value %s j=%zu\n", KT[k].name, j); }
    }
    cases++;
    if (var == BARE) { seen[k][0]++; return; }
    if (var == V1) { if (f != ovf) { kfails[k]++; if (fails++ < 10) printf("FAIL ovf %s got %d want %d\n", KT[k].name, f, ovf); } seen[k][ovf ? 2 : 0]++; return; }
    if (ovf) { if (!(f & 1)) { kfails[k]++; if (fails++ < 10) printf("FAIL ovf missed %s\n", KT[k].name); } seen[k][f & 2 ? 3 : 2]++; return; }
    if (f != (nofit << 1)) { kfails[k]++; if (fails++ < 10) printf("FAIL flags %s got %d want %d\n", KT[k].name, f, nofit << 1); }
    seen[k][nofit]++;
}
static void do_check20(int quick) {
    const int b32[] = {14, 15, 30, 31}, b64[] = {30, 31, 62, 63};
    edges(E32, &NE32, b32, 4, 32); edges(E64, &NE64, b64, 4, 64);
    printf("edge values: %d at int32, %d at int64\n", NE32, NE64);
    static i64 ar[256], ai[256], br[256], bi[256]; long cnt = 0;
    for (int w = 32; w <= 64; w += 32) {
        const i64 *E = w == 32 ? E32 : E64; int NE = w == 32 ? NE32 : NE64;
        for (int a = 0; a < NE; a++) for (int b = 0; b < NE; b++) for (int c = 0; c < NE; c++) for (int d = 0; d < NE; d++) {
            if (quick && (a + b + c + d) % 5) continue;
            memset(ar, 0, 64); memset(ai, 0, 64); memset(br, 0, 64); memset(bi, 0, 64);
            int l = cnt++ & 7; ar[l] = E[a]; ai[l] = E[b]; br[l] = E[c]; bi[l] = E[d];
            for (int k = 0; k < NKT; k++) if (KT[k].w == w) check_one(k, ar, ai, br, bi, 8);
        }
    }
    printf("edge quadruples: %ld\n", cnt);
    long rnd = 0;
    for (int it = 0; it < (quick ? 40000 : 400000); it++) {
        int w = it & 1 ? 64 : 32;
        const i64 *E = w == 32 ? E32 : E64; int NE = w == 32 ? NE32 : NE64;
        size_t N = 8 * (1 + next() % 16);
        int big = (int)(next() % 3) == 0 ? 0 : (int)(next() % 4);
        static const int sm32[] = {6, 12, 14}, sm64[] = {14, 28, 30};
        int small = w == 32 ? sm32[next() % 3] : sm64[next() % 3];
        if (it % 4 < 2) { for (size_t j = 0; j < N; j++) { ar[j] = rval20(E, NE, w, small, big); ai[j] = rval20(E, NE, w, small, big);
                                                          br[j] = rval20(E, NE, w, small, big); bi[j] = rval20(E, NE, w, small, big); } }
        else { i64 *t[] = {ar, ai, br, bi};
               for (int q = 0; q < 4; q++) for (size_t j = 0; j < N; j++) t[q][j] = (i64)(next() % (2ULL << small)) - (1LL << small);
               if (next() & 1) t[next() % 4][next() % N] = rval20(E, NE, w, small, 50); }
        for (int k = 0; k < NKT; k++) if (KT[k].w == w) check_one(k, ar, ai, br, bi, N);
        rnd++;
    }
    printf("random arrays: %ld (length 8..128)\ncases %ld, failures %ld\n", rnd, cases, fails);
    for (int k = 0; k < NKT; k++) if (kfails[k]) printf("  %-12s %ld failures\n", KT[k].name, kfails[k]);
    printf("outcomes: no ovf & all fit / no ovf & some don't / ovf, shrink bit clear / ovf, shrink bit set\n");
    for (int k = 0; k < NKT; k++) printf("  %-12s %9ld %9ld %9ld %9ld\n", KT[k].name, seen[k][0], seen[k][1], seen[k][2], seen[k][3]);
}
/* ---------- study 20 timing ---------- */
static volatile int sink;
static void do_time20(size_t bigN) {
    char *buf[6];
    for (int j = 0; j < 6; j++) buf[j] = (char *)aligned_alloc(4096, bigN * 8 + 4096) + 576 * j;
    const size_t sizes[3] = {512, 16384, bigN}, inner[3] = {4096, 128, 1};
    static const char *pn[3] = {"L1", "L2", "memory"};
    static double med[3][64];
    printf("ns per element (one Gaussian integer); median [q1, q3] over %d reps; kernels of one width interleaved, order rotated\n", REPS);
    for (int p = 0; p < 3; p++) for (int w = 32; w <= 64; w += 32) {
        size_t N = sizes[p];
        for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) {
            if (w == 32) ((i32 *)buf[j])[i] = (i32)(next() % (1u << 14)) - (1 << 13);
            else ((i64 *)buf[j])[i] = (i64)(next() % (1u << 29)) - (1 << 28);
        }
        memset(buf[4], 0, N * w / 8); memset(buf[5], 0, N * w / 8);
        int ks[32], nk = 0; for (int k = 0; k < NKT; k++) if (KT[k].w == w) ks[nk++] = k;
        static double t[32][REPS];
#define CALL20(k) callk(k, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], N)
        for (int j = 0; j < nk; j++) sink += CALL20(ks[j]);
        for (int r = 0; r < REPS; r++) for (int j = 0; j < nk; j++) {
            int jj = (j + r) % nk, k = ks[jj];
            sink += CALL20(k);
            u64 t0 = now_ns();
            for (size_t it = 0; it < inner[p]; it++) sink += CALL20(k);
            t[jj][r] = (double)(now_ns() - t0) / (double)(N * inner[p]);
        }
        printf("\n%s, int%d halves, N = %zu (%zu KB per array)\n", pn[p], w, N, N * w / 8 / 1024);
        for (int j = 0; j < nk; j++) { qsort(t[j], REPS, sizeof(double), cmpd); med[p][ks[j]] = t[j][REPS / 2];
            printf("  %-12s %.4f [%.4f, %.4f]\n", KT[ks[j]].name, t[j][REPS / 2], t[j][REPS / 4], t[j][3 * REPS / 4]); }
        fflush(stdout);
    }
    printf("\nmedians by operation and variant (v3 = v1 where there is no fused kernel); mix = mean of the 6 cells, int64 multiply vector / scalar\n");
    for (int p = 0; p < 3; p++) {
        printf("%s\n  %-8s %8s %8s %8s %8s\n", pn[p], "", "bare", "v1", "v2", "v3");
        double mix[2][4] = {{0}};
        for (int w = 32; w <= 64; w += 32) for (int op = 0; op < 4; op++) {
            if (w == 32 && op == 3) continue;
            double v[4] = {0};
            for (int k = 0; k < NKT; k++) if (KT[k].w == w && KT[k].op == op) v[KT[k].var] = med[p][k];
            if (v[V3] == 0) v[V3] = v[V1];
            printf("  %s%d%s %8.4f %8.4f %8.4f %8.4f\n", op == 0 ? "add" : op == 1 ? "sub" : "mul", w, op == 3 ? "s" : op == 2 && w == 64 ? "v" : " ", v[0], v[1], v[2], v[3]);
            for (int q = 0; q < 4; q++) { if (op != 3) mix[0][q] += v[q] / 6; if (op != 2 || w == 32) mix[1][q] += v[q] / 6; }
        }
        for (int m = 0; m < 2; m++)
            printf("  mix %s %8.4f %8.4f %8.4f %8.4f   (v2 %+.1f%%, v3 %+.1f%% over v1)\n", m ? "s" : "v", mix[m][0], mix[m][1], mix[m][2], mix[m][3],
                   100 * (mix[m][2] / mix[m][1] - 1), 100 * (mix[m][3] / mix[m][1] - 1));
    }
}

#endif /* A64 */
/* ======================= study 21: whole runs ======================= */
#define ARGS64W const i64 *restrict ar, const i64 *restrict ai, const i32 *restrict br, const i32 *restrict bi, i64 *restrict cr, i64 *restrict ci, size_t N
#define LDW2(p, i) ({ V w_ = vld1q((p) + (i)); (P2){vmovl_nat_lo(w_), vmovl_nat_high(w_)}; })     /* 4 int32 -> two int64 registers */
#define OVF64W(NAME, OP, OVF) NOINL int NAME(ARGS64W) {                                          \
    V o = vzero();                                                                               \
    for (size_t i = 0; i < N; i += 4) {                                                          \
        P2 y = LDW2(br, i), v = LDW2(bi, i);                                                     \
        _Pragma("GCC unroll 2") for (int h = 0; h < 2; h++) {                                    \
            V x = vld1q(ar + i + 2 * h), u = vld1q(ai + i + 2 * h), yy = h ? y.h : y.l, vv = h ? v.h : v.l; \
            V r = OP(x, yy), q = OP(u, vv); vst1q(cr + i + 2 * h, r); vst1q(ci + i + 2 * h, q);   \
            o = vorrq(o, vorrq(OVF(x, yy, r), OVF(u, vv, q))); } }                               \
    return vany_and(o, SIGN64); }
OVF64W(k_add64w, vaddq_s64, OVA64)
OVF64W(k_sub64w, vsubq_s64, OVS)
#define FUSED64W(NAME, OP, OVF) NOINL int NAME(ARGS64W, i64 offv, i64 maskv) {                   \
    const V off = vdupq_n_s64(offv); V o = vzero(), s = vzero();                                 \
    for (size_t i = 0; i < N; i += 4) {                                                          \
        P2 y = LDW2(br, i), v = LDW2(bi, i);                                                     \
        _Pragma("GCC unroll 2") for (int h = 0; h < 2; h++) {                                    \
            V x = vld1q(ar + i + 2 * h), u = vld1q(ai + i + 2 * h), yy = h ? y.h : y.l, vv = h ? v.h : v.l; \
            V r = OP(x, yy), q = OP(u, vv); vst1q(cr + i + 2 * h, r); vst1q(ci + i + 2 * h, q);   \
            s = vorrq(s, vorrq(vaddq_s64(r, off), vaddq_s64(q, off)));                           \
            o = vorrq(o, vorrq(OVF(x, yy, r), OVF(u, vv, q))); } }                               \
    return vany_and(o, SIGN64) | vany_and(s, vdupq_n_s64(maskv)) << 1; }
FUSED64W(k_add64wf, vaddq_s64, OVA64)
FUSED64W(k_sub64wf, vsubq_s64, OVS)
/* the int64 state times an int32 operand (sign-extended on load): vector (signed pieces) and scalar */
NOINL int k_mul64w(ARGS64W) {
    V o = vzero();
    for (size_t i = 0; i < N; i += 4) {
        P2 re, im;
        o = vorrq(o, sgn_pass(LD2(ar, i), LD2(ai, i), LDW2(br, i), LDW2(bi, i), &re, &im));
        ST2(cr, i, re); ST2(ci, i, im);
    }
    return vany(o);
}
__attribute__((optimize("no-tree-vectorize")))
NOINL int s_mul64w(ARGS64W) {
    u64 o = 0;
    for (size_t i = 0; i < N; i++) {
        i64 a = ar[i], b = ai[i], c = br[i], d = bi[i];
        i128 re = (i128)a * c - (i128)b * d, im = (i128)a * d + (i128)b * c;
        i64 rl = (i64)re, il = (i64)im;
        o |= ((u64)(re >> 64) ^ (u64)(rl >> 63)) | ((u64)(im >> 64) ^ (u64)(il >> 63));
        cr[i] = rl; ci[i] = il;
    }
    return o != 0;
}
NOINL int k_pass64r(const i64 *restrict cr, const i64 *restrict ci, size_t N, i64 offv, i64 maskv) {
    const V off = vdupq_n_s64(offv); V s = vzero();
    for (size_t i = 0; i < N; i += 4) { _Pragma("GCC unroll 2") for (int h = 0; h < 4; h += 2)
        s = vorrq(s, vorrq(vaddq_s64(vld1q(cr + i + h), off), vaddq_s64(vld1q(ci + i + h), off))); }
    return vany_and(s, vdupq_n_s64(maskv));
}
NOINL void k_widen(const i32 *restrict r, const i32 *restrict im, i64 *restrict R, i64 *restrict I, size_t N) {
    for (size_t i = 0; i < N; i += 4) { P2 a = LDW2(r, i), b = LDW2(im, i); ST2(R, i, a); ST2(I, i, b); }
}
NOINL void k_narrow(const i64 *restrict R, const i64 *restrict I, i32 *restrict r, i32 *restrict im, size_t N) {
    for (size_t i = 0; i < N; i += 4) { vst1q(r + i, vuzp1_nat(vld1q(R + i), vld1q(R + i + 2))); vst1q(im + i, vuzp1_nat(vld1q(I + i), vld1q(I + i + 2))); }
}

#ifndef A64
enum { ADD, SUB, MUL };
typedef struct { int op, y; } Op;
typedef struct { i32 *r, *i; } Ar32;
typedef struct { i64 *r, *i; } Ar64;
static Ar32 X0, Y[8], B32[2];
static Ar64 B64[2];
typedef struct { int w; const void *r, *i; int prom, dem; } Res;
#define RB_OFF (1LL << 30)
#define RB_MASK ((i64)0xFFFFFFFF80000000ULL)
#define RA_OFF (1LL << 31)
#define RA_MASK ((i64)0xFFFFFFFF00000000ULL)
static int g_smul;     /* 1: the int64 multiply is scalar (study 21's), 0: vector */
static Res run(const Op *ops, int nops, int var, int ruleA, size_t N) {
    i64 off = ruleA ? RA_OFF : RB_OFF, mask = ruleA ? RA_MASK : RB_MASK;
    int w = 32, prom = 0, dem = 0;
    const i32 *xr = X0.r, *xi = X0.i; const i64 *Xr = 0, *Xi = 0;
    for (int k = 0; k < nops; k++) {
        int op = ops[k].op; const i32 *yr = Y[ops[k].y].r, *yi = Y[ops[k].y].i; int f = 0;
        if (w == 32) {
            Ar32 z = (xr == B32[0].r) ? B32[1] : B32[0];
            f = (op == ADD ? k_add32_o : op == SUB ? k_sub32_o : k_mul32_o)(xr, xi, yr, yi, z.r, z.i, N);
            if (!(f & 1)) { xr = z.r; xi = z.i; if (var == 2) sink += k_pass32(xr, xi, N); continue; }
            k_widen(xr, xi, B64[0].r, B64[0].i, N); Xr = B64[0].r; Xi = B64[0].i; w = 64; prom++;
        }
        Ar64 z = (Xr == B64[0].r) ? B64[1] : B64[0];
        if (var == 3 && op != MUL) f = (op == ADD ? k_add64wf : k_sub64wf)(Xr, Xi, yr, yi, z.r, z.i, N, off, mask);
        else f = (op == ADD ? k_add64w : op == SUB ? k_sub64w : g_smul ? s_mul64w : k_mul64w)(Xr, Xi, yr, yi, z.r, z.i, N);
        if (f & 1) { fprintf(stderr, "overflow at int64: out of scope\n"); exit(1); }
        Xr = z.r; Xi = z.i;
        int fits = 0;
        if (var == 2) fits = !k_pass64r(Xr, Xi, N, off, mask);
        else if (var == 3 && op != MUL) fits = !(f & 2);
        if (fits) { k_narrow(Xr, Xi, B32[0].r, B32[0].i, N); xr = B32[0].r; xi = B32[0].i; w = 32; dem++; }
    }
    Res R = {w, w == 32 ? (const void *)xr : (const void *)Xr, w == 32 ? (const void *)xi : (const void *)Xi, prom, dem};
    return R;
}
static i32 urand(i64 lo, i64 hi) { return (i32)(lo + (i64)(next() % (u64)(hi - lo + 1))); }
static void *xalloc(size_t n, int j) { return (char *)aligned_alloc(4096, n + 4096) + 576 * (j % 7); }
enum { YP, YS, YT, YU, YD };
static Ar32 X0b, X0h;
static void make_data(size_t N) {
    int j = 0;
    X0b.r = xalloc(N * 4, j++); X0b.i = xalloc(N * 4, j++); X0h.r = xalloc(N * 4, j++); X0h.i = xalloc(N * 4, j++);
    for (int k = 0; k < 5; k++) { Y[k].r = xalloc(N * 4, j++); Y[k].i = xalloc(N * 4, j++); }
    for (int k = 0; k < 2; k++) { B32[k].r = xalloc(N * 4, j++); B32[k].i = xalloc(N * 4, j++); B64[k].r = xalloc(N * 8, j++); B64[k].i = xalloc(N * 8, j++); }
    for (size_t i = 0; i < N; i++) {
        X0b.r[i] = urand(-(1 << 28), 1 << 28); X0b.i[i] = urand(-(1 << 28), 1 << 28);
        Y[YP].r[i] = urand((1LL << 31) - (1 << 28), (1LL << 31) - 1); Y[YP].i[i] = urand((1LL << 31) - (1 << 28), (1LL << 31) - 1);
        Y[YS].r[i] = urand(-1024, 1024); Y[YS].i[i] = urand(-1024, 1024);
        Y[YT].r[i] = urand(-1024, 1024); Y[YT].i[i] = urand(-1024, 1024);
        static const i32 ur[4] = {1, -1, 0, 0}, ui[4] = {0, 0, 1, -1};
        int u = (int)(next() % 4); Y[YU].r[i] = ur[u]; Y[YU].i[i] = ui[u];
        X0h.r[i] = urand((1LL << 31) - 2048, (1LL << 31) - 1025); X0h.i[i] = urand((1LL << 31) - 2048, (1LL << 31) - 1025);
        Y[YD].r[i] = urand(2048, 3072); Y[YD].i[i] = urand(2048, 3072);
    }
}
enum { BACK, CONTROL, HOVER };
static const char *wname[3] = {"back", "control", "hover"};
static int make_ops(int wl, int L, Op *ops) {
    int n = 0;
    if (wl == HOVER) { for (int k = 0; k < 2 + L; k++) ops[n++] = (Op){k % 2 ? SUB : ADD, YD}; return n; }
    ops[n++] = (Op){ADD, YP};
    ops[n++] = wl == BACK ? (Op){SUB, YP} : (Op){SUB, YS};
    for (int k = 0; k < L; k++) ops[n++] = k % 3 == 0 ? (Op){ADD, YS} : k % 3 == 1 ? (Op){SUB, YT} : (Op){MUL, YU};
    return n;
}
static long fails21;
static void check_cfg(int wl, int L, size_t N) {
    Op ops[200]; int n = make_ops(wl, L, ops);
    X0 = wl == HOVER ? X0h : X0b;
    static i64 er[1 << 16], ei[1 << 16];
    size_t step = N > (1 << 16) ? N / (1 << 16) : 1, M = 0;
    for (size_t i = 0; i < N; i += step, M++) {
        i128 a = X0.r[i], b = X0.i[i];
        for (int k = 0; k < n; k++) {
            i128 c = Y[ops[k].y].r[i], d = Y[ops[k].y].i[i];
            if (ops[k].op == ADD) { a += c; b += d; } else if (ops[k].op == SUB) { a -= c; b -= d; } else { i128 t = a * c - b * d; b = a * d + b * c; a = t; }
        }
        er[M] = (i64)a; ei[M] = (i64)b;
    }
    int nv = wl == HOVER ? 4 : 3;
    printf("  %-7s L=%3d:", wname[wl], L);
    for (int v = 0; v < nv; v++) {
        Res R = run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
        size_t m = 0;
        for (size_t i = 0; i < N; i += step, m++) {
            i64 gr = R.w == 32 ? ((const i32 *)R.r)[i] : ((const i64 *)R.r)[i], gi = R.w == 32 ? ((const i32 *)R.i)[i] : ((const i64 *)R.i)[i];
            if (gr != er[m] || gi != ei[m]) { if (fails21++ < 10) printf("\nFAIL %s L=%d v%d i=%zu\n", wname[wl], L, v + 1, i); break; }
        }
        printf("  v%d%s int%d %d/%d", v < 3 ? v + 1 : 3, v == 3 ? "A" : "", R.w, R.prom, R.dem);
    }
    printf("\n");
}
static const int Ls[] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
#define NL 9
static void do_check21(void) {
    size_t Ns[2] = {1024, (size_t)1 << 20};
    for (int sm = 0; sm < 2; sm++) {
        g_smul = sm;
        for (int s = 0; s < 2; s++) {
            make_data(Ns[s]);
            printf("N = %zu, int64 multiply %s (end size, promotions/demotions)\n", Ns[s], sm ? "scalar" : "vector");
            for (int wl = 0; wl < 3; wl++) for (int l = 0; l < NL; l++) check_cfg(wl, Ls[l], Ns[s]);
        }
    }
    printf("failures: %ld\n", fails21);
}
static void do_time21(size_t Nmem) {
    size_t Ns[2] = {1024, Nmem}; int reps[2] = {31, 7};
    for (int s = 0; s < 2; s++) {
        size_t N = Ns[s]; make_data(N);
        for (int sm = 0; sm < 2; sm++) {
            g_smul = sm;
            printf("\n%s: N = %zu, int64 multiply %s; median [q1, q3] over %d reps, ns per element per run\n", s ? "memory" : "in cache", N, sm ? "scalar" : "vector", reps[s]);
            for (int wl = 0; wl < 3; wl++) {
                if (sm && wl == HOVER) continue;          /* hover has no multiply */
                X0 = wl == HOVER ? X0h : X0b;
                int nv = wl == HOVER ? 4 : 3;
                printf("%s\n  %4s %22s %22s %22s%s   saved v2, v3%s\n", wname[wl], "L", "v1", "v2", "v3", wl == HOVER ? "               v3A (rule A)" : "", wl == HOVER ? ", v3A" : "");
                for (int l = 0; l < NL; l++) {
                    Op ops[200]; int n = make_ops(wl, Ls[l], ops);
                    size_t inner = s ? 1 : (size_t)(300000.0 / ((n + 2) * N * 0.4)) + 1;
                    static double t[4][64];
                    for (int v = 0; v < nv; v++) run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
                    for (int r = 0; r < reps[s]; r++) for (int j = 0; j < nv; j++) {
                        int v = (j + r) % nv;
                        if (!s) run(ops, n, v < 3 ? v + 1 : 3, v == 3, N);
                        u64 t0 = now_ns();
                        for (size_t it = 0; it < inner; it++) sink += run(ops, n, v < 3 ? v + 1 : 3, v == 3, N).w;
                        t[v][r] = (double)(now_ns() - t0) / (double)(N * inner);
                    }
                    double med[4]; printf("  %4d", Ls[l]);
                    for (int v = 0; v < nv; v++) { qsort(t[v], reps[s], sizeof(double), cmpd); med[v] = t[v][reps[s] / 2];
                        char b[64]; snprintf(b, sizeof b, "%.3f [%.3f,%.3f]", med[v], t[v][reps[s] / 4], t[v][3 * reps[s] / 4]); printf(" %22s", b); }
                    printf("   %+.3f %+.3f", med[0] - med[1], med[0] - med[2]);
                    if (nv == 4) printf(" %+.3f", med[0] - med[3]);
                    printf("\n"); fflush(stdout);
                }
            }
        }
    }
}
static void do_count20(void) {
#ifdef COUNT
    enum { N = 1024 }; static i64 b[6][N];
    for (int j = 0; j < 4; j++) for (size_t i = 0; i < N; i++) b[j][i] = (i64)(next() % (1u << 14)) - (1 << 13);
    printf("NEON instructions per element (all / loads+stores); int32 kernels on int32 data, int64 on int64\n");
    for (int k = 0; k < NKT; k++) { nops = nmem = 0; callk(k, b[0], b[1], b[2], b[3], b[4], b[5], N);
        printf("  %-12s %6.3f / %5.3f\n", KT[k].name, (double)nops / N, (double)nmem / N); }
    struct { const char *nm; void (*f)(void); } dummy; (void)dummy;
    nops = nmem = 0; k_widen((i32 *)b[0], (i32 *)b[1], b[4], b[5], N); printf("  %-12s %6.3f / %5.3f\n", "widen", (double)nops / N, (double)nmem / N);
    nops = nmem = 0; k_narrow(b[0], b[1], (i32 *)b[4], (i32 *)b[5], N); printf("  %-12s %6.3f / %5.3f\n", "narrow", (double)nops / N, (double)nmem / N);
    nops = nmem = 0; k_add64wf(b[0], b[1], (i32 *)b[2], (i32 *)b[3], b[4], b[5], N, RB_OFF, RB_MASK); printf("  %-12s %6.3f / %5.3f\n", "add64w fused", (double)nops / N, (double)nmem / N);
    nops = nmem = 0; k_add64w(b[0], b[1], (i32 *)b[2], (i32 *)b[3], b[4], b[5], N); printf("  %-12s %6.3f / %5.3f\n", "add64w", (double)nops / N, (double)nmem / N);
    nops = nmem = 0; k_mul64w(b[0], b[1], (i32 *)b[2], (i32 *)b[3], b[4], b[5], N); printf("  %-12s %6.3f / %5.3f\n", "mul64w", (double)nops / N, (double)nmem / N);
#else
    printf("build with -DCOUNT\n");
#endif
}
int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check20";
    if (!strcmp(mode, "check20")) do_check20(0);
    else if (!strcmp(mode, "quick20")) do_check20(1);
    else if (!strcmp(mode, "time20")) do_time20(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 25);
    else if (!strcmp(mode, "check21")) do_check21();
    else if (!strcmp(mode, "time21")) do_time21(argc > 2 ? (size_t)strtoull(argv[2], 0, 0) : (size_t)1 << 23);
    else if (!strcmp(mode, "count")) do_count20();
    else if (!strcmp(mode, "scan")) scan_kernels(argv[0]);
    return 0;
}
#endif /* A64 */
