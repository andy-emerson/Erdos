/* Study 19: Barrett against Montgomery reduction mod odd n < 2^31, scalar and AVX2 32-bit lanes.
 * Residues are uint32 in [0, n). Layout: separate arrays (x, y for the product; a, b = re, im of the first factor,
 * c, d = re, im of the second for the Gaussian product; outputs re, im).
 * Kernels (each over arrays of N elements):
 *   prod:  x*y mod n                      gauss: (ac - bd) mod n, (ad + bc) mod n
 * Versions:
 *   div    scalar t % n (hardware divide), reference only
 *   bar_s  scalar Barrett, m = floor(2^64/n), q = mulhi(t, m), r in [0, 2n), one conditional subtract
 *   mon_s  scalar Montgomery, R = 2^32, n' = n^-1 mod 2^32, r = (t - m n) / 2^32 in (-n, n), add n if negative
 *   bar_v  AVX2 Barrett, 32x32->64 multiplies only. L = ceil(log2 n), mu = floor(2^(L+31)/n) < 2^32.
 *          prod:  q = ((t >> (L-1)) * mu) >> 32, error <= 2, r in [0, 3n)
 *          gauss: q = ((t >> L) * mu) >> 31,      error <= 3, r in [0, 4n)   (t = ac + n^2 - bd or ad + bc, t < 2n^2)
 *          then: subtract 2n where r >= 2n (64-bit lanes), pack to 32-bit lanes, subtract n where r >= n (min trick)
 *   mon_v  AVX2 Montgomery, R = 2^32, as mon_s, in even/odd 64-bit halves (vpmuludq / vpmuldq)
 * Montgomery inputs and outputs are in Montgomery form (x R mod n); conversion in (REDC(x * (R^2 mod n))) and
 * out (REDC(x)) are separate kernels.
 * Build: gcc -O2 -mavx2 -o reduce19 reduce19.c
 * Run:   ./reduce19 check | ./reduce19 time | ./reduce19 dump (sample cases for an independent Python check) */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef uint32_t u32; typedef int32_t i32; typedef uint64_t u64; typedef int64_t i64;
typedef unsigned __int128 u128; typedef __int128 i128;
#define NOINL __attribute__((noinline))

typedef struct { u32 n; u64 n2, bm; u32 L, mu, ninv, r2; } mod_t;

static mod_t mod_make(u32 n) {
    mod_t M; M.n = n; M.n2 = (u64)n * n; M.bm = UINT64_MAX / n;   /* floor(2^64/n) for n not a power of 2 */
    u32 L = 0; while ((1ull << L) < n) L++;                        /* 2^(L-1) < n <= 2^L */
    M.L = L; M.mu = (u32)((1ull << (L + 31)) / n);                 /* < 2^32 since n > 2^(L-1) */
    u32 inv = n; for (int i = 0; i < 5; i++) inv *= 2 - n * inv;   /* n^-1 mod 2^32 */
    M.ninv = inv; M.r2 = (u32)(((u128)1 << 64) % n);
    return M;
}

/* ---------- scalar reductions ---------- */
static inline u32 bar_s(u64 t, u64 n, u64 m) {                     /* t < 2^64 */
    u64 q = (u64)(((u128)t * m) >> 64);
    u64 r = t - q * n;                                             /* [0, 2n) */
    return (u32)(r >= n ? r - n : r);
}
static inline u32 fixneg(i32 r, u32 n) { return (u32)r + (n & (u32)(r >> 31)); }
static inline u32 mon_s(u64 t, u32 n, u32 ninv) {                  /* unsigned m; needs t < n 2^32 */
    u32 m = (u32)t * ninv;
    return fixneg((i32)((t - (u64)m * n) >> 32), n);
}
static inline u32 mon_s_signed(i64 t, u32 n, u32 ninv) {           /* signed m; needs |t| < n^2 */
    i32 m = (i32)((u32)t * ninv);
    return fixneg((i32)((u64)(t - (i64)m * n) >> 32), n);
}

/* ---------- vector helpers ---------- */
#define V __m256i
static inline V odd(V x) { return _mm256_srli_epi64(x, 32); }
static inline V fixneg_v(V r, V n) { return _mm256_min_epu32(r, _mm256_add_epi32(r, n)); }   /* (-n, n) -> [0, n) */
static inline V fixge_v(V r, V n) { return _mm256_min_epu32(r, _mm256_sub_epi32(r, n)); }    /* [0, 2n) -> [0, n) */
/* Montgomery: te, to hold 64-bit t for even and odd lanes; returns 8 residues */
static inline V mon_v(V te, V to, V n, V ninv, int sgn) {
    V me = _mm256_mul_epu32(te, ninv), mo = _mm256_mul_epu32(to, ninv);
    V mne = sgn ? _mm256_mul_epi32(me, n) : _mm256_mul_epu32(me, n);
    V mno = sgn ? _mm256_mul_epi32(mo, n) : _mm256_mul_epu32(mo, n);
    V re = _mm256_sub_epi64(te, mne), ro = _mm256_sub_epi64(to, mno);  /* high dwords = result, low dwords 0 */
    return fixneg_v(_mm256_blend_epi32(_mm256_srli_epi64(re, 32), ro, 0xAA), n);
}
/* Barrett half: 4 x 64-bit t -> 4 x 64-bit r in [0, 2n) (lo dword) */
static inline V bar_v_half(V t, V sh1, V mu, int sh2, V n, V tn, V tnm1) {
    V q = _mm256_srli_epi64(_mm256_mul_epu32(_mm256_srlv_epi64(t, sh1), mu), sh2);
    V r = _mm256_sub_epi64(t, _mm256_mul_epu32(q, n));
    return _mm256_sub_epi64(r, _mm256_and_si256(_mm256_cmpgt_epi64(r, tnm1), tn));
}
static inline V bar_v(V te, V to, V sh1, V mu, int sh2, V n, V tn, V tnm1) {
    V re = bar_v_half(te, sh1, mu, sh2, n, tn, tnm1), ro = bar_v_half(to, sh1, mu, sh2, n, tn, tnm1);
    return fixge_v(_mm256_blend_epi32(re, _mm256_slli_epi64(ro, 32), 0xAA), n);
}
#define LD(p) _mm256_loadu_si256((const V *)(p))
#define ST(p, v) _mm256_storeu_si256((V *)(p), v)

/* ---------- product kernels ---------- */
typedef void (*kprod)(const u32 *, const u32 *, u32 *, size_t, const mod_t *);
NOINL static void prod_div(const u32 *x, const u32 *y, u32 *o, size_t N, const mod_t *M) {
    u64 n = M->n; for (size_t i = 0; i < N; i++) o[i] = (u32)((u64)x[i] * y[i] % n); }
NOINL static void prod_bar_s(const u32 *x, const u32 *y, u32 *o, size_t N, const mod_t *M) {
    u64 n = M->n, m = M->bm; for (size_t i = 0; i < N; i++) o[i] = bar_s((u64)x[i] * y[i], n, m); }
NOINL static void prod_mon_s(const u32 *x, const u32 *y, u32 *o, size_t N, const mod_t *M) {
    u32 n = M->n, ni = M->ninv; for (size_t i = 0; i < N; i++) o[i] = mon_s((u64)x[i] * y[i], n, ni); }
NOINL static void prod_bar_v(const u32 *x, const u32 *y, u32 *o, size_t N, const mod_t *M) {
    V n = _mm256_set1_epi32(M->n), mu = _mm256_set1_epi32(M->mu), sh1 = _mm256_set1_epi64x(M->L - 1);
    V tn = _mm256_set1_epi64x(2 * (u64)M->n), tnm1 = _mm256_set1_epi64x(2 * (u64)M->n - 1);
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(x + i), b = LD(y + i);
        ST(o + i, bar_v(_mm256_mul_epu32(a, b), _mm256_mul_epu32(odd(a), odd(b)), sh1, mu, 32, n, tn, tnm1));
    }
}
NOINL static void prod_mon_v(const u32 *x, const u32 *y, u32 *o, size_t N, const mod_t *M) {
    V n = _mm256_set1_epi32(M->n), ni = _mm256_set1_epi32(M->ninv);
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(x + i), b = LD(y + i);
        ST(o + i, mon_v(_mm256_mul_epu32(a, b), _mm256_mul_epu32(odd(a), odd(b)), n, ni, 0));
    }
}

/* ---------- Gaussian kernels ---------- */
typedef void (*kgauss)(const u32 *, const u32 *, const u32 *, const u32 *, u32 *, u32 *, size_t, const mod_t *);
#define GARGS const u32 *a, const u32 *b, const u32 *c, const u32 *d, u32 *re, u32 *im, size_t N, const mod_t *M
NOINL static void gauss_div(GARGS) {
    u64 n = M->n, n2 = M->n2;
    for (size_t i = 0; i < N; i++) {
        u64 A = a[i], B = b[i], C = c[i], D = d[i];
        re[i] = (u32)((A * C + (n2 - B * D)) % n); im[i] = (u32)((A * D + B * C) % n);
    }
}
NOINL static void gauss_bar_s(GARGS) {
    u64 n = M->n, n2 = M->n2, m = M->bm;
    for (size_t i = 0; i < N; i++) {
        u64 A = a[i], B = b[i], C = c[i], D = d[i];
        re[i] = bar_s(A * C + (n2 - B * D), n, m); im[i] = bar_s(A * D + B * C, n, m);
    }
}
NOINL static void gauss_mon_s(GARGS) {
    u32 n = M->n, ni = M->ninv;
    for (size_t i = 0; i < N; i++) {
        u64 A = a[i], B = b[i], C = c[i], D = d[i];
        re[i] = mon_s_signed((i64)(A * C) - (i64)(B * D), n, ni); im[i] = mon_s(A * D + B * C, n, ni);
    }
}
NOINL static void gauss_bar_v(GARGS) {
    V n = _mm256_set1_epi32(M->n), mu = _mm256_set1_epi32(M->mu), sh1 = _mm256_set1_epi64x(M->L);
    V n2 = _mm256_set1_epi64x(M->n2);
    V tn = _mm256_set1_epi64x(2 * (u64)M->n), tnm1 = _mm256_set1_epi64x(2 * (u64)M->n - 1);
    for (size_t i = 0; i < N; i += 8) {
        V A = LD(a + i), B = LD(b + i), C = LD(c + i), D = LD(d + i);
        V Ao = odd(A), Bo = odd(B), Co = odd(C), Do = odd(D);
        V ree = _mm256_add_epi64(_mm256_mul_epu32(A, C), _mm256_sub_epi64(n2, _mm256_mul_epu32(B, D)));
        V reo = _mm256_add_epi64(_mm256_mul_epu32(Ao, Co), _mm256_sub_epi64(n2, _mm256_mul_epu32(Bo, Do)));
        V ime = _mm256_add_epi64(_mm256_mul_epu32(A, D), _mm256_mul_epu32(B, C));
        V imo = _mm256_add_epi64(_mm256_mul_epu32(Ao, Do), _mm256_mul_epu32(Bo, Co));
        ST(re + i, bar_v(ree, reo, sh1, mu, 31, n, tn, tnm1));
        ST(im + i, bar_v(ime, imo, sh1, mu, 31, n, tn, tnm1));
    }
}
NOINL static void gauss_mon_v(GARGS) {
    V n = _mm256_set1_epi32(M->n), ni = _mm256_set1_epi32(M->ninv);
    for (size_t i = 0; i < N; i += 8) {
        V A = LD(a + i), B = LD(b + i), C = LD(c + i), D = LD(d + i);
        V Ao = odd(A), Bo = odd(B), Co = odd(C), Do = odd(D);
        V ree = _mm256_sub_epi64(_mm256_mul_epu32(A, C), _mm256_mul_epu32(B, D));     /* signed, |t| < n^2 */
        V reo = _mm256_sub_epi64(_mm256_mul_epu32(Ao, Co), _mm256_mul_epu32(Bo, Do));
        V ime = _mm256_add_epi64(_mm256_mul_epu32(A, D), _mm256_mul_epu32(B, C));     /* [0, 2n^2) */
        V imo = _mm256_add_epi64(_mm256_mul_epu32(Ao, Do), _mm256_mul_epu32(Bo, Co));
        ST(re + i, mon_v(ree, reo, n, ni, 1));
        ST(im + i, mon_v(ime, imo, n, ni, 0));
    }
}

/* ---------- Montgomery conversions ---------- */
typedef void (*kconv)(const u32 *, u32 *, size_t, const mod_t *);
NOINL static void to_mon_s(const u32 *x, u32 *o, size_t N, const mod_t *M) {
    u32 n = M->n, ni = M->ninv; u64 r2 = M->r2;
    for (size_t i = 0; i < N; i++) o[i] = mon_s(x[i] * r2, n, ni); }
NOINL static void from_mon_s(const u32 *x, u32 *o, size_t N, const mod_t *M) {
    u32 n = M->n, ni = M->ninv;
    for (size_t i = 0; i < N; i++) o[i] = mon_s(x[i], n, ni); }
NOINL static void to_mon_v(const u32 *x, u32 *o, size_t N, const mod_t *M) {
    V n = _mm256_set1_epi32(M->n), ni = _mm256_set1_epi32(M->ninv), r2 = _mm256_set1_epi32(M->r2);
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(x + i);
        ST(o + i, mon_v(_mm256_mul_epu32(a, r2), _mm256_mul_epu32(odd(a), r2), n, ni, 0));
    }
}
NOINL static void from_mon_v(const u32 *x, u32 *o, size_t N, const mod_t *M) {
    /* t = x < 2^32: result = 0 - hi(m n), in (-n, 0] */
    V n = _mm256_set1_epi32(M->n), ni = _mm256_set1_epi32(M->ninv), z = _mm256_setzero_si256();
    for (size_t i = 0; i < N; i += 8) {
        V a = LD(x + i);
        V mne = _mm256_mul_epu32(_mm256_mul_epu32(a, ni), n), mno = _mm256_mul_epu32(_mm256_mul_epu32(odd(a), ni), n);
        V h = _mm256_blend_epi32(_mm256_srli_epi64(mne, 32), mno, 0xAA);
        ST(o + i, fixneg_v(_mm256_sub_epi32(z, h), n));
    }
}

/* ---------- reference and checks ---------- */
static u64 rng = 0x9E3779B97F4A7C15ull;
static u64 next(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }
static int isprime(u32 n) {
    if (n < 2) return 0;
    for (u32 p = 2; (u64)p * p <= n; p++) if (n % p == 0) return 0;
    return 1;
}
static u32 rpow(u32 x, u32 n) { return (u32)(((u64)x << 32) % n); }      /* x R mod n */

static const char *vname[5] = {"div", "bar_s", "mon_s", "bar_v", "mon_v"};
static kprod KP[5] = {prod_div, prod_bar_s, prod_mon_s, prod_bar_v, prod_mon_v};
static kgauss KG[5] = {gauss_div, gauss_bar_s, gauss_mon_s, gauss_bar_v, gauss_mon_v};
static u64 cases[2][5], fails[2][5], convfail;

/* run all versions on chunk of N (multiple of 8) and compare with the exact reference */
static u32 *A, *B, *C, *D, *Am, *Bm, *Cm, *Dm, *O1, *O2, *T1, *T2;
static void check_chunk(size_t N, const mod_t *M, int gauss) {
    u32 n = M->n;
    /* Montgomery path: convert in (scalar and vector both checked), multiply, convert out */
    to_mon_v(A, Am, N, M); to_mon_s(B, Bm, N, M); to_mon_v(C, Cm, N, M); to_mon_s(D, Dm, N, M);
    for (size_t i = 0; i < N; i++)
        convfail += (Am[i] != rpow(A[i], n)) + (Bm[i] != rpow(B[i], n)) + (Cm[i] != rpow(C[i], n)) + (Dm[i] != rpow(D[i], n));
    for (int v = 0; v < 5; v++) {
        int mont = v == 2 || v == 4;
        const u32 *a = mont ? Am : A, *b = mont ? Bm : B, *c = mont ? Cm : C, *d = mont ? Dm : D;
        if (gauss) KG[v](a, b, c, d, O1, O2, N, M); else KP[v](a, c, O1, N, M);
        if (mont) {                                       /* Montgomery form check, then convert out */
            for (size_t i = 0; i < N; i++) {
                u32 wr, wi;
                if (gauss) {
                    i128 r = (i128)A[i] * C[i] - (i128)B[i] * D[i]; r %= n; if (r < 0) r += n;
                    wr = (u32)r; wi = (u32)(((u128)A[i] * D[i] + (u128)B[i] * C[i]) % n);
                } else wr = (u32)((u64)A[i] * C[i] % n), wi = 0;
                fails[gauss][v] += (O1[i] != rpow(wr, n)) + (gauss && O2[i] != rpow(wi, n));
            }
            if (v == 2) { from_mon_s(O1, T1, N, M); from_mon_v(O2, T2, N, M); }
            else { from_mon_v(O1, T1, N, M); from_mon_s(O2, T2, N, M); }
            memcpy(O1, T1, N * 4); memcpy(O2, T2, N * 4);
        }
        for (size_t i = 0; i < N; i++) {
            u32 wr, wi = 0;
            if (gauss) {
                i128 r = (i128)A[i] * C[i] - (i128)B[i] * D[i]; r %= n; if (r < 0) r += n;
                wr = (u32)r; wi = (u32)(((u128)A[i] * D[i] + (u128)B[i] * C[i]) % n);
            } else wr = (u32)((u64)A[i] * C[i] % n);
            fails[gauss][v] += (O1[i] != wr) + (gauss && O2[i] != wi);
        }
        cases[gauss][v] += N;
    }
}
#define CH (1u << 16)
static size_t fill;
static void flush(const mod_t *M, int gauss) {
    if (!fill) return;
    while (fill % 8) { A[fill] = B[fill] = C[fill] = D[fill] = 0; fill++; }   /* pad with zeros (valid residues) */
    check_chunk(fill, M, gauss); fill = 0;
}
static void push(u32 a, u32 b, u32 c, u32 d, const mod_t *M, int gauss) {
    A[fill] = a; B[fill] = b; C[fill] = c; D[fill] = d;
    if (++fill == CH) flush(M, gauss);
}
static u32 pick(u32 n) {                                  /* residue: 1/4 edge values, else uniform */
    u64 r = next();
    if ((r & 3) == 0) { u32 e[6] = {0, 1, 2, n - 1, n - 2, n / 2}; return e[(r >> 2) % 6]; }
    return (u32)((r >> 2) % n);
}
/* direct test of the vector Barrett reduction on arbitrary t in [0, T): max quotient error observed */
static void check_bar_reduce(const mod_t *M, int gauss, u64 count, u64 *bad, u64 *maxerr) {
    u64 n = M->n, T = gauss ? 2 * M->n2 : M->n2;
    V vn = _mm256_set1_epi32(M->n), mu = _mm256_set1_epi32(M->mu), sh1 = _mm256_set1_epi64x(gauss ? M->L : M->L - 1);
    V tn = _mm256_set1_epi64x(2 * n), tnm1 = _mm256_set1_epi64x(2 * n - 1);
    u64 t[8] __attribute__((aligned(32)));
    for (u64 k = 0; k < count; k += 8) {
        for (int j = 0; j < 8; j++) {
            u64 r = next(), s = next() % 4;
            if (s == 0) t[j] = T - 1 - (r % 64);                       /* top of range */
            else if (s == 1) { u64 q = (r >> 2) % (T / n); t[j] = q * n + ((r & 2) ? n - 1 : 0); if (t[j] >= T) t[j] = T - 1; }
            else t[j] = r % T;
            /* direct error measure on the estimate */
            u64 x = t[j] >> (gauss ? M->L : M->L - 1);
            u64 q = (x * M->mu) >> (gauss ? 31 : 32);
            u64 e = t[j] / n - q; if (e > *maxerr) *maxerr = e;
            if (q > t[j] / n) (*bad)++;
        }
        V te = _mm256_set_epi64x(t[6], t[4], t[2], t[0]), to = _mm256_set_epi64x(t[7], t[5], t[3], t[1]);
        u32 out[8]; ST(out, bar_v(te, to, sh1, mu, gauss ? 31 : 32, vn, tn, tnm1));
        for (int j = 0; j < 8; j++) *bad += out[j] != t[j] % n;
    }
}

static void do_check(void) {
    size_t sz = (CH + 8) * 4;
    A = malloc(sz); B = malloc(sz); C = malloc(sz); D = malloc(sz); Am = malloc(sz); Bm = malloc(sz); Cm = malloc(sz);
    Dm = malloc(sz); O1 = malloc(sz); O2 = malloc(sz); T1 = malloc(sz); T2 = malloc(sz);
    /* exhaustive: product over all (x, y) for odd n <= 1023; Gaussian over all (a, b, c, d) for odd n <= 63 */
    u64 np = 0, ng = 0;
    for (u32 n = 3; n <= 1023; n += 2) {
        mod_t M = mod_make(n); np++;
        for (u32 x = 0; x < n; x++) for (u32 y = 0; y < n; y++) push(x, 0, y, 0, &M, 0);
        flush(&M, 0);
    }
    for (u32 n = 3; n <= 63; n += 2) {
        mod_t M = mod_make(n); ng++;
        for (u32 a = 0; a < n; a++) for (u32 b = 0; b < n; b++) for (u32 c = 0; c < n; c++) for (u32 d = 0; d < n; d++)
            push(a, b, c, d, &M, 1);
        flush(&M, 1);
    }
    printf("exhaustive: product, %llu odd n in [3, 1023]; Gaussian, %llu odd n in [3, 63]\n", (unsigned long long)np, (unsigned long long)ng);
    for (int g = 0; g < 2; g++) for (int v = 0; v < 5; v++)
        printf("  %-8s %-6s cases %12llu  failures %llu\n", g ? "gauss" : "product", vname[v],
               (unsigned long long)cases[g][v], (unsigned long long)fails[g][v]);
    printf("  conversion-in mismatches: %llu\n", (unsigned long long)convfail);
    memset(cases, 0, sizeof cases); memset(fails, 0, sizeof fails); convfail = 0;
    /* random at 31-bit n (and some smaller), 10^7 per version per kernel per n */
    const u32 ns[] = {2147483647u, 2147483629u, 2147483645u, 2147483643u, 1073741825u, 1073741827u,
                      1431655767u, 111546435u, 1000003u, 65537u};
    printf("random, 10^7 elements per version per kernel per n (1/4 of residues edge values 0, 1, 2, n-1, n-2, n/2):\n");
    for (unsigned k = 0; k < sizeof ns / sizeof *ns; k++) {
        u32 n = ns[k]; mod_t M = mod_make(n);
        memset(cases, 0, sizeof cases); memset(fails, 0, sizeof fails); convfail = 0;
        for (int g = 0; g < 2; g++) {
            for (u64 i = 0; i < 10000000; i++) push(pick(n), g ? pick(n) : 0, pick(n), g ? pick(n) : 0, &M, g);
            flush(&M, g);
        }
        u64 badp = 0, badg = 0, ep = 0, eg = 0;
        if (n == 1073741827u) {   /* worst cases found by an exhaustive scan over x = t >> s: errors 2 and 3 */
            V vn = _mm256_set1_epi32(n), mu = _mm256_set1_epi32(M.mu), tn = _mm256_set1_epi64x(2 * (u64)n), tnm1 = _mm256_set1_epi64x(2 * (u64)n - 1);
            u64 tp = 768614337836220415ull, tg = 1537228675672440831ull; u32 out[8];
            ST(out, bar_v(_mm256_set1_epi64x(tp), _mm256_set1_epi64x(tp), _mm256_set1_epi64x(M.L - 1), mu, 32, vn, tn, tnm1));
            badp += out[0] != tp % n;
            ST(out, bar_v(_mm256_set1_epi64x(tg), _mm256_set1_epi64x(tg), _mm256_set1_epi64x(M.L), mu, 31, vn, tn, tnm1));
            badg += out[0] != tg % n;
            printf("  worst-case t for n = 1073741827 (q errors 2 and 3): failures %llu\n", (unsigned long long)(badp + badg));
        }
        check_bar_reduce(&M, 0, 4000000, &badp, &ep); check_bar_reduce(&M, 1, 4000000, &badg, &eg);
        u64 f = 0; for (int g = 0; g < 2; g++) for (int v = 0; v < 5; v++) f += fails[g][v];
        printf("  n = %10u (%s, L = %2u): %llu cases per version and kernel, failures %llu, conversion mismatches %llu;"
               " lane Barrett on 4e6 t each: product max q error %llu (bad %llu), gauss max q error %llu (bad %llu)\n",
               n, isprime(n) ? "prime    " : "composite", M.L, (unsigned long long)cases[0][0], (unsigned long long)f,
               (unsigned long long)convfail, (unsigned long long)ep, (unsigned long long)badp,
               (unsigned long long)eg, (unsigned long long)badg);
    }
}

static void do_dump(void) {          /* sample cases for an independent Python check of this program's outputs */
    const u32 ns[] = {2147483647u, 2147483645u, 1073741825u, 1000003u};
    for (unsigned k = 0; k < 4; k++) {
        u32 n = ns[k]; mod_t M = mod_make(n);
        u32 a[8], b[8], c[8], d[8], am[8], bm[8], cm[8], dm[8], o[5][2][8], t1[8], t2[8];
        for (int rep = 0; rep < 50; rep++) {
            for (int j = 0; j < 8; j++) a[j] = pick(n), b[j] = pick(n), c[j] = pick(n), d[j] = pick(n);
            to_mon_v(a, am, 8, &M); to_mon_v(b, bm, 8, &M); to_mon_v(c, cm, 8, &M); to_mon_v(d, dm, 8, &M);
            for (int v = 0; v < 5; v++) {
                int mont = v == 2 || v == 4;
                KG[v](mont ? am : a, mont ? bm : b, mont ? cm : c, mont ? dm : d, o[v][0], o[v][1], 8, &M);
                if (mont) { from_mon_v(o[v][0], t1, 8, &M); from_mon_v(o[v][1], t2, 8, &M); memcpy(o[v][0], t1, 32); memcpy(o[v][1], t2, 32); }
            }
            for (int j = 0; j < 8; j++) {
                printf("%u %u %u %u %u", n, a[j], b[j], c[j], d[j]);
                for (int v = 0; v < 5; v++) printf(" %u %u", o[v][0][j], o[v][1][j]);
                printf("\n");
            }
        }
    }
}

/* ---------- timing ---------- */
static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec * 1e9 + ts.tv_nsec; }
static int cmpd(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }
#define REPS 31
#define NK 14
static const char *kname[NK] = {"prod div", "prod bar_s", "prod mon_s", "prod bar_v", "prod mon_v",
                                "gauss div", "gauss bar_s", "gauss mon_s", "gauss bar_v", "gauss mon_v",
                                "to_mon_s", "from_mon_s", "to_mon_v", "from_mon_v"};
static int swap_arrays;
static void run_kernel(int k, size_t N, const mod_t *M) {
    int mont = (k == 2 || k == 4 || k == 7 || k == 9) ^ swap_arrays;   /* swap: control for array placement */
    const u32 *a = mont ? Am : A, *b = mont ? Bm : B, *c = mont ? Cm : C, *d = mont ? Dm : D;
    if (k < 5) KP[k](a, c, O1, N, M);
    else if (k < 10) KG[k - 5](a, b, c, d, O1, O2, N, M);
    else if (k == 10) to_mon_s(A, O1, N, M);
    else if (k == 11) from_mon_s(Am, O1, N, M);
    else if (k == 12) to_mon_v(A, O1, N, M);
    else from_mon_v(Am, O1, N, M);
}
static void do_time(void) {
    const size_t BIG = 1u << 22, SMALL = 1024, INNER = 2048;      /* in-cache: 1024 elements x 2048 calls */
    size_t sz = BIG * 4;
    A = malloc(sz); B = malloc(sz); C = malloc(sz); D = malloc(sz); Am = malloc(sz); Bm = malloc(sz);
    Cm = malloc(sz); Dm = malloc(sz); O1 = malloc(sz); O2 = malloc(sz);
    const u32 ns[] = {2147483647u, 2147483645u, 1073741825u, 1000003u};
    printf("ns per element (a Gaussian element = 2 residues out); median [q1, q3] over %d repetitions, kernels interleaved, order rotated\n", REPS);
    for (int pass = 0; pass < 2; pass++) {
        size_t N = pass ? BIG : SMALL, inner = pass ? 1 : INNER;
        printf("\n%s: N = %zu elements per array%s\n", pass ? "large (memory)" : "in cache (L1)", N,
               pass ? "" : ", 2048 calls per sample");
        printf("%-12s", "kernel"); for (unsigned k = 0; k < 4; k++) printf("  %26u", ns[k]); printf("\n");
        static double med[4][NK];
        static double res[NK][4][3];
        for (unsigned in = 0; in < 4; in++) {
            mod_t M = mod_make(ns[in]); u32 n = ns[in];
            for (size_t i = 0; i < N; i++) { A[i] = next() % n; B[i] = next() % n; C[i] = next() % n; D[i] = next() % n; }
            to_mon_v(A, Am, N, &M); to_mon_v(B, Bm, N, &M); to_mon_v(C, Cm, N, &M); to_mon_v(D, Dm, N, &M);
            static double t[NK][REPS];
            for (int k = 0; k < NK; k++) run_kernel(k, N, &M);           /* warm up */
            for (int r = 0; r < REPS; r++)
                for (int j = 0; j < NK; j++) {
                    int k = (j + r) % NK;
                    run_kernel(k, N, &M);       /* untimed call first: the timed call does not inherit the previous kernel's cache state */
                    double t0 = now();
                    for (size_t it = 0; it < inner; it++) run_kernel(k, N, &M);
                    t[k][r] = (now() - t0) / (double)(N * inner);
                }
            for (int k = 0; k < NK; k++) {
                qsort(t[k], REPS, sizeof(double), cmpd);
                res[k][in][0] = t[k][REPS / 2]; res[k][in][1] = t[k][REPS / 4]; res[k][in][2] = t[k][3 * REPS / 4];
                med[in][k] = t[k][REPS / 2];
            }
        }
        for (int k = 0; k < NK; k++) {
            printf("%-12s", kname[k]);
            for (int in = 0; in < 4; in++) {
                double base = med[in][k < 5 ? 0 : 5];
                char buf[64]; snprintf(buf, sizeof buf, "%.3f [%.3f,%.3f] x%.1f", res[k][in][0], res[k][in][1], res[k][in][2],
                                       k < 10 ? base / res[k][in][0] : 0.0);
                printf("  %26s", buf);
            }
            printf("\n");
        }
    }
    printf("(xF = divide baseline median / this median, i.e. speed-up over the hardware divide; conversions: x0.0)\n");
}

int main(int argc, char **argv) {
    const char *mode = argc > 1 ? argv[1] : "check";
    if (!strcmp(mode, "check")) do_check();
    else if (!strcmp(mode, "dump")) do_dump();
    else { swap_arrays = !strcmp(mode, "time-swap"); do_time(); }
    return 0;
}
