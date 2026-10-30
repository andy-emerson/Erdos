/* factor.c: integer helpers and factoring for entry (spec "Entering a denominator", steps 1 and 2).
 *
 *   - the odd primes below 2^16 with their exact-division constants, tested without branches (round three's D1
 *     tests: multiply and compare, vectorised), 32 at a time, with the adaptive stop p^2 > n checked once per 32;
 *   - deterministic Miller-Rabin for every 64-bit n (the first twelve prime bases), in Montgomery form below 2^63;
 *   - Pollard-Brent rho in Montgomery form.
 * The tables are process-wide constants built once (pthread_once), so separate graphs may live in separate threads.
 */
#include <pthread.h>

#include "pg_internal.h"

#define NODD 6541                 /* odd primes below 2^16 */
#define NGRP ((NODD + 31) / 32)   /* groups of 32 tests */
#define NBLK (4 * NGRP)           /* blocks of 8 */
static uint64_t tb_inv[NBLK * 8], tb_lim[NBLK * 8], tb_sq[NBLK];
static uint32_t tb_p[NBLK * 8];
static pthread_once_t tb_once = PTHREAD_ONCE_INIT;

uint64_t pgi_inv64(uint64_t m) {  /* m odd: m^-1 mod 2^64 (Newton) */
    uint64_t x = m;
    for (int i = 0; i < 5; i++) x *= 2 - m * x;
    return x;
}

static void tb_build(void) {
    static uint8_t comp[65536];
    int k = 0;
    for (uint32_t i = 3; i < 65536; i += 2) {
        if (comp[i]) continue;
        tb_p[k] = i;
        tb_inv[k] = pgi_inv64(i);
        tb_lim[k] = UINT64_MAX / i;
        k++;
        for (uint32_t j = i * i; j < 65536; j += 2 * i) comp[j] = 1;
    }
    if (k != NODD) pgi_die("prime table size");
    for (; k < NBLK * 8; k++) { tb_p[k] = 0; tb_inv[k] = 1; tb_lim[k] = 0; }  /* n * 1 <= 0 never holds for n >= 1 */
    for (int b = 0; b < NBLK; b++) tb_sq[b] = (uint64_t)tb_p[8 * b] * tb_p[8 * b];
}

void pgi_tables_init(void) { pthread_once(&tb_once, tb_build); }

uint64_t pgi_gcd(uint64_t a, uint64_t b) {
    if (!a) return b;
    if (!b) return a;
    int s = __builtin_ctzll(a | b);
    a >>= __builtin_ctzll(a);
    do {
        b >>= __builtin_ctzll(b);
        if (a > b) { uint64_t t = a; a = b; b = t; }
        b -= a;
    } while (b);
    return a << s;
}

static inline int ctz128(u128 x) {
    uint64_t lo = (uint64_t)x;
    return lo ? __builtin_ctzll(lo) : 64 + __builtin_ctzll((uint64_t)(x >> 64));
}

u128 pgi_gcd128(u128 a, u128 b) {
    if (!a) return b;
    if (!b) return a;
    int s = ctz128(a | b);
    a >>= ctz128(a);
    do {
        b >>= ctz128(b);
        if (a > b) { u128 t = a; a = b; b = t; }
        b -= a;
    } while (b);
    return a << s;
}

/* bit j of the result: prime 32 c + j divides n.  32 multiply-and-compare tests without branches (vectorised). */
static inline uint32_t tb_group(uint64_t n, int c) {
    const uint64_t *I = tb_inv + 32 * c, *L = tb_lim + 32 * c;
    uint32_t m = 0;
    for (int j = 0; j < 32; j++) m |= (uint32_t)(n * I[j] <= L[j]) << j;
    return m;
}

/* The spec's step 1 with round three's D1 tests, four blocks of 8 at a time: the stop (p^2 > n, on the group's first
 * prime) once per 32 primes.  Testing a few primes past the stop is harmless: a test is an exact divisibility test. */
uint64_t pgi_trial(uint64_t n, uint64_t *P, uint32_t *E, int *k, int *stopped) {
    *stopped = 0;
    for (int c = 0; c < NGRP; c++) {
        if (n < tb_sq[4 * c]) { *stopped = 1; return n; }
        uint32_t m = tb_group(n, c);
        while (m) {  /* each hit divides the n the group started with, hence also the current n */
            int t = 32 * c + __builtin_ctz(m);
            m &= m - 1;
            uint64_t q = n * tb_inv[t];
            uint32_t e = 0;
            do { n = q; e++; q = n * tb_inv[t]; } while (q <= tb_lim[t]);
            P[*k] = tb_p[t];
            E[(*k)++] = e;
        }
    }
    return n;
}

/* ------------------------------------------------------------------------------------------------- Miller-Rabin, rho */

static inline uint64_t mulmod(uint64_t a, uint64_t b, uint64_t n) { return (uint64_t)((u128)a * b % n); }
static uint64_t powmod(uint64_t a, uint64_t e, uint64_t n) {
    uint64_t r = 1;
    a %= n;
    while (e) {
        if (e & 1) r = mulmod(r, a, n);
        a = mulmod(a, a, n);
        e >>= 1;
    }
    return r;
}
typedef struct { uint64_t n, ni, one; } mont;  /* odd n < 2^63 */
static inline mont mont_of(uint64_t n) {
    mont M;
    M.n = n;
    M.ni = 0 - pgi_inv64(n);
    M.one = (0 - n) % n;  /* R mod n */
    return M;
}
static inline uint64_t mredc(u128 t, const mont *M) {
    uint64_t m = (uint64_t)t * M->ni;
    uint64_t u = (uint64_t)((t + (u128)m * M->n) >> 64);
    return u >= M->n ? u - M->n : u;
}
static inline uint64_t mmul(uint64_t a, uint64_t b, const mont *M) { return mredc((u128)a * b, M); }
static inline uint64_t mto(uint64_t a, const mont *M) { return (uint64_t)(((u128)(a % M->n) << 64) % M->n); }

int pgi_is_prime(uint64_t n) {
    static const uint64_t B[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    if (n < 2) return 0;
    for (int i = 0; i < 12; i++) {
        if (n == B[i]) return 1;
        if (n % B[i] == 0) return 0;
    }
    /* deterministic base sets by size (Jaeschke 1993; Zhang & Tang 2003): the first nb primes suffice below the bound */
    int nb = n < 2152302898747ull ? 5 : n < 3474749660383ull ? 6 : n < 341550071728321ull ? 7
           : n < 3825123056546413051ull ? 9 : 12;
    uint64_t d = n - 1;
    int s = __builtin_ctzll(d);
    d >>= s;
    if (n >> 63) {  /* outside the Montgomery range */
        for (int i = 0; i < nb; i++) {
            uint64_t x = powmod(B[i], d, n);
            if (x == 1 || x == n - 1) continue;
            int r;
            for (r = 1; r < s; r++) {
                x = mulmod(x, x, n);
                if (x == n - 1) break;
            }
            if (r == s) return 0;
        }
        return 1;
    }
    mont M = mont_of(n);
    uint64_t mone = M.one, mmone = n - M.one;
    for (int i = 0; i < nb; i++) {
        uint64_t b = mto(B[i], &M), x = mone;
        for (uint64_t e = d; e; e >>= 1) {
            if (e & 1) x = mmul(x, b, &M);
            b = mmul(b, b, &M);
        }
        if (x == mone || x == mmone) continue;
        int r;
        for (r = 1; r < s; r++) {
            x = mmul(x, x, &M);
            if (x == mmone) break;
        }
        if (r == s) return 0;
    }
    return 1;
}

static uint64_t rho(uint64_t n) {  /* Pollard-Brent: a nontrivial factor of odd composite n < 2^63 */
    if (n >> 63) pgi_die("rho: n above 2^63");
    mont M = mont_of(n);
    for (uint64_t c = 1;; c++) {
        uint64_t y = mto(2, &M), x = y, ys = y, q = M.one, g = 1, r = 1;
        const uint64_t m = 128;
#define F(z) ({ uint64_t _t = mmul((z), (z), &M) + c; _t >= n ? _t - n : _t; })
        do {
            x = y;
            for (uint64_t i = 0; i < r; i++) y = F(y);
            uint64_t k = 0;
            do {
                ys = y;
                uint64_t lim = m < r - k ? m : r - k;
                for (uint64_t i = 0; i < lim; i++) {
                    y = F(y);
                    q = mmul(q, x > y ? x - y : y - x, &M);
                }
                g = pgi_gcd(q, n);
                k += m;
            } while (k < r && g == 1);
            r *= 2;
        } while (g == 1);
        if (g == n) {
            do {
                ys = F(ys);
                g = pgi_gcd(x > ys ? x - ys : ys - x, n);
            } while (g == 1);
        }
#undef F
        if (g != n) return g;
    }
}

static void factor_big(uint64_t n, uint64_t *out, int *k, int known_composite) {
    if (n == 1) return;
    /* n has no factor below 2^16, so below 2^32 it is prime */
    if (!known_composite && (n < (1ull << 32) || pgi_is_prime(n))) { out[(*k)++] = n; return; }
    uint64_t d = rho(n);
    factor_big(d, out, k, 0);
    factor_big(n / d, out, k, 0);
}

int pgi_factor_rest(uint64_t n, uint64_t *P, uint32_t *E) {
    uint64_t f[64];
    int nf = 0, k = 0;
    factor_big(n, f, &nf, 1);
    for (int a = 1; a < nf; a++)
        for (int b = a; b > 0 && f[b - 1] > f[b]; b--) { uint64_t t = f[b]; f[b] = f[b - 1]; f[b - 1] = t; }
    for (int a = 0; a < nf;) {
        int b = a;
        while (b < nf && f[b] == f[a]) b++;
        P[k] = f[a];
        E[k++] = (uint32_t)(b - a);
        a = b;
    }
    return k;
}
