/* s_core.c: the code shared by S and C (scale.c #includes it once per binary, with VAR_S or VAR_C defined).
 *
 *   memory accounting (bytes and live blocks: the leak check), the PRNG, factoring (a copy of pg.c's sieve,
 *   Miller-Rabin and Pollard-Brent rho), the node table, the edge pool, holders, the transient distinct-denominator
 *   table of an entry, GMP product / remainder trees (batch gcd), and the value ops (fin, lcm_plan, core_add,
 *   core_mul: ab_core.c's V0' code, 64-bit, with S's hooks).
 *
 * A HOLDER is a numerator on the graph's numerator side: anything that owns an edge list.  It is an exception or
 * single value (kind 0), an array's shared denominator D (kind 1, V3), an op result in the result ring (kind 2), or a
 * transient holder for one distinct denominator of the entry in progress (kind 3).  Edges are packed u32
 * (node << 8 | weight), sorted by node id, in a pooled allocation with size classes 1..16.  In S each edge also
 * stores its position in the node's numerator list (PPL), so removing it from the list is O(1).
 */
#define _GNU_SOURCE
#include <gmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned __int128 u128;
typedef __int128 i128;

#define MAXE 16             /* a denominator below 2^63 has at most 15 coprime factors */
#define AW (2 * MAXE)       /* room for a merged edge list before its denominator is checked */
#define EN(x) ((x) >> 8)    /* packed edge: node id (24 bits), weight (8 bits) */
#define EW(x) ((x) & 0xFFu)
#define EP(n, w) (((uint32_t)(n) << 8) | (uint32_t)(w))
#define NONE 0xFFFFFFFFu
#define NUM_LIM (1ull << 63)

static void die(const char *m) { fprintf(stderr, "scale: %s\n", m); fflush(stderr); abort(); }
static uint64_t now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}

/* ---------------------------------------------------------------- memory accounting */

static size_t mem_cur, mem_peak;
static int64_t blk_cur;       /* live blocks: equal before and after a full delete, or something leaked */
static int corrupt_leak;      /* test hook: the next holder free forgets its block */
static void *xmalloc(size_t n) {
    size_t *p = malloc(n + 16);
    if (!p) die("out of memory");
    p[0] = n;
    mem_cur += n;
    blk_cur++;
    if (mem_cur > mem_peak) mem_peak = mem_cur;
    return p + 2;
}
static void *xcalloc(size_t n) { void *p = xmalloc(n); memset(p, 0, n); return p; }
static void xfree(void *q) {
    if (!q) return;
    size_t *p = (size_t *)q - 2;
    mem_cur -= p[0];
    blk_cur--;
    free(p);
}
static void *xrealloc(void *q, size_t n) {
    if (!q) return xmalloc(n);
    size_t *p = (size_t *)q - 2, old = p[0];
    p = realloc(p, n + 16);
    if (!p) die("out of memory");
    p[0] = n;
    mem_cur = mem_cur - old + n;
    if (mem_cur > mem_peak) mem_peak = mem_cur;
    return p + 2;
}
#define GROW(p, n, cap, need) do { if ((size_t)(n) + (need) > (size_t)(cap)) { \
        while ((size_t)(n) + (need) > (size_t)(cap)) (cap) = (cap) ? 2 * (cap) : 256; \
        (p) = xrealloc((p), (size_t)(cap) * sizeof *(p)); } } while (0)

/* ---------------------------------------------------------------- PRNG (splitmix64) */

static uint64_t rng_s = 1;
static inline uint64_t rnd(void) {
    uint64_t z = (rng_s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
static inline uint64_t rndn(uint64_t n) { return (uint64_t)(((u128)rnd() * n) >> 64); }
static inline double rndf(void) { return (double)(rnd() >> 11) * (1.0 / 9007199254740992.0); }

/* ---------------------------------------------------------------- integers and factoring (pg.c's, MR and rho in Montgomery form) */

static inline uint64_t mag(int64_t a) { return a < 0 ? 0 - (uint64_t)a : (uint64_t)a; }
static inline uint64_t inv64(uint64_t m) {
    uint64_t x = m;
    for (int i = 0; i < 5; i++) x *= 2 - m * x;
    return x;
}
static uint64_t gcd64(uint64_t a, uint64_t b) {
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
static inline size_t hsh(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return (size_t)x;
}

#define NSMALL 6600
static uint64_t sp[NSMALL], spi[NSMALL], spl[NSMALL];  /* odd primes below 2^16, exact-division constants */
static int nsp;
static void sieve(void) {
    static uint8_t comp[65536];
    if (nsp) return;
    for (uint32_t i = 3; i < 65536; i += 2) {
        if (comp[i]) continue;
        sp[nsp] = i; spi[nsp] = inv64(i); spl[nsp] = UINT64_MAX / i; nsp++;
        for (uint32_t j = i * i; j < 65536; j += 2 * i) comp[j] = 1;
    }
}
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
/* Montgomery arithmetic for odd n < 2^63 (Miller-Rabin and rho; pg.c's versions use a u128 % per product) */
typedef struct { uint64_t n, ni, one; } mont;
static inline mont mont_of(uint64_t n) {
    mont M;
    M.n = n;
    M.ni = 0 - inv64(n);
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
static int is_prime(uint64_t n) {  /* deterministic Miller-Rabin for every 64-bit n (the first twelve prime bases) */
    static const uint64_t B[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    if (n < 2) return 0;
    for (int i = 0; i < 12; i++) {
        if (n == B[i]) return 1;
        if (n % B[i] == 0) return 0;
    }
    uint64_t d = n - 1;
    int s = __builtin_ctzll(d);
    d >>= s;
    if (n >> 63) {  /* outside Montgomery's range here: pg.c's code */
        for (int i = 0; i < 12; i++) {
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
    for (int i = 0; i < 12; i++) {
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
static uint64_t st_rho_calls;
static uint64_t rho(uint64_t n) {  /* Pollard-Brent (pg.c's loop) in Montgomery form: a nontrivial factor of odd composite n */
    st_rho_calls++;
    if (n >> 63) die("rho: n above 2^63");
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
                g = gcd64(q, n);
                k += m;
            } while (k < r && g == 1);
            r *= 2;
        } while (g == 1);
        if (g == n) {
            do {
                ys = F(ys);
                g = gcd64(x > ys ? x - ys : ys - x, n);
            } while (g == 1);
        }
#undef F
        if (g != n) return g;
    }
}
static void factor_big(uint64_t n, uint64_t *out, int *k) {
    if (n == 1) return;
    if (is_prime(n)) { out[(*k)++] = n; return; }
    uint64_t d = rho(n);
    factor_big(d, out, k);
    factor_big(n / d, out, k);
}
/* n with no factor below 2^16, composite: its primes with exponents (rho), ascending */
static int factor_rest(uint64_t n, uint64_t *P, uint32_t *E) {
    uint64_t f[64];
    int nf = 0, k = 0;
    factor_big(n, f, &nf);
    for (int a = 1; a < nf; a++)
        for (int b = a; b > 0 && f[b - 1] > f[b]; b--) { uint64_t t = f[b]; f[b] = f[b - 1]; f[b - 1] = t; }
    for (int a = 0; a < nf;) {
        int b = a;
        while (b < nf && f[b] == f[a]) b++;
        P[k] = f[a]; E[k++] = (uint32_t)(b - a);
        a = b;
    }
    return k;
}
/* the coprime base of {a, b} (pg.c's coprime_base2) */
static int coprime_base2(uint64_t a, uint64_t b, uint64_t *L) {
    int n = 0;
    if (a > 1) L[n++] = a;
    if (b > 1) L[n++] = b;
    for (int again = 1; again;) {
        again = 0;
        for (int i = 0; i < n && !again; i++)
            for (int j = i + 1; j < n && !again; j++) {
                uint64_t g = gcd64(L[i], L[j]);
                if (g == 1) continue;
                if (L[i] == L[j]) { L[j] = 1; }
                else {
                    L[i] /= g; L[j] /= g;
                    if (n == 64) die("coprime base too large");
                    L[n++] = g;
                }
                int m = 0;
                for (int t = 0; t < n; t++) if (L[t] > 1) L[m++] = L[t];
                n = m;
                again = 1;
            }
    }
    return n;
}
/* the coprime base of a list (in place; returns the new count; at most cap entries) */
static int coprime_base_list(uint64_t *L, int n, int cap) {
    for (int again = 1; again;) {
        again = 0;
        for (int i = 0; i < n && !again; i++)
            for (int j = i + 1; j < n && !again; j++) {
                uint64_t g = gcd64(L[i], L[j]);
                if (g == 1) continue;
                if (L[i] == L[j]) L[j] = 1;
                else {
                    L[i] /= g; L[j] /= g;
                    if (n == cap) die("coprime base list too large");
                    L[n++] = g;
                }
                int m = 0;
                for (int t = 0; t < n; t++) if (L[t] > 1) L[m++] = L[t];
                n = m;
                again = 1;
            }
    }
    return n;
}
static inline uint32_t vlog(uint64_t x, uint64_t p) { uint32_t e = 0; while (x % p == 0) { x /= p; e++; } return e; }

/* ---------------------------------------------------------------- nodes (the denominator side) */

typedef struct { uint64_t a; uint8_t e, k; } atom_t;  /* k: 0 prime, 1 leftover (factors unknown), 2 pending */
typedef struct {
    uint64_t v, inv, lim;   /* value; exact-division test of its odd part (ab_core) */
    uint8_t s, alive, chk;  /* 2-adic exponent; live; S: needs the hooked checks (several atoms, or not prime) */
#ifdef VAR_S
    uint8_t pend, na, nacap;
    uint32_t rc, len, lcap; /* reference count; the numerator list (holder ids) */
    uint32_t *list;
    atom_t *at;             /* its atoms: v = prod a^e, exponents with gcd 1 */
#else
    uint8_t prime;
#endif
} node_t;
static node_t *nd;
static uint32_t nnodes, ncap, nlive;  /* slots used (high-water), capacity, live nodes */
static uint8_t *wsa, *wsb;            /* scratch weights per node id (kept zero) for V3's sparse lcm */

static void nd_setv(uint32_t id, uint64_t v) {
    node_t *N = &nd[id];
    N->v = v;
    N->s = (uint8_t)__builtin_ctzll(v);
    uint64_t m = v >> N->s;
    N->inv = inv64(m);
    N->lim = UINT64_MAX / m;
}
static uint32_t nd_slot(void) {  /* a fresh slot at the end of the table */
    if (nnodes == ncap) {
        uint32_t oc = ncap;
        ncap = ncap ? 2 * ncap : 256;
        nd = xrealloc(nd, sizeof(node_t) * ncap);
        wsa = xrealloc(wsa, ncap); wsb = xrealloc(wsb, ncap);
        memset(wsa + oc, 0, ncap - oc); memset(wsb + oc, 0, ncap - oc);
    }
    if (nnodes >= (1u << 24)) die("too many nodes");
    return nnodes++;
}
static inline int ndiv(uint64_t u, const node_t *N, uint64_t *q) {  /* exact-division test: multiply, compare */
    if (N->s) {
        if (u & ((1ull << N->s) - 1)) return 0;
        u >>= N->s;
    }
    uint64_t t = u * N->inv;
    if (t > N->lim) return 0;
    *q = t;
    return 1;
}
static inline uint64_t ediv(uint64_t d, const node_t *N) { return (d >> N->s) * N->inv; }  /* d / v, v | d */

/* ---------------------------------------------------------------- the edge pool and holders */

static uint32_t *EPL;   /* packed edges */
#ifdef VAR_S
static uint32_t *PPL;   /* S: each edge's position in its node's numerator list */
#endif
static size_t pool_n, pool_cap, pool_used;  /* used: edge slots handed out and not returned (the leak check) */
static uint32_t pfree[5] = {NONE, NONE, NONE, NONE, NONE};

static uint32_t pool_get(int cls) {
    uint32_t c = 1u << cls;
    pool_used += c;
    if (pfree[cls] != NONE) { uint32_t o = pfree[cls]; pfree[cls] = EPL[o]; return o; }
    if (pool_n + c > pool_cap) {
        while (pool_n + c > pool_cap) pool_cap = pool_cap ? 2 * pool_cap : 4096;
        if (pool_cap >= (1ull << 32)) die("edge pool too large");
        EPL = xrealloc(EPL, pool_cap * 4);
#ifdef VAR_S
        PPL = xrealloc(PPL, pool_cap * 4);
#endif
    }
    uint32_t o = (uint32_t)pool_n;
    pool_n += c;
    return o;
}
static inline void pool_put(uint32_t o, int cls) { EPL[o] = pfree[cls]; pfree[cls] = o; pool_used -= 1u << cls; }
static inline int cls_for(int ne) { int c = 0; while ((1 << c) < ne) c++; return c; }

typedef struct { int64_t num; uint64_t den; uint32_t eo; uint8_t ne, cls, alive, kind; } holder_t;
static holder_t *H;
static uint32_t nh, hcap, hfree = NONE, hlive;

static uint32_t h_alloc(void) {
    uint32_t h;
    if (hfree != NONE) { h = hfree; hfree = H[h].eo; }
    else {
        if (nh == hcap) { hcap = hcap ? 2 * hcap : 1024; H = xrealloc(H, (size_t)hcap * sizeof *H); }
        h = nh++;
    }
    hlive++;
    return h;
}
/* a new holder with the edges e (not linked: the graph's g_link does that) */
static uint32_t h_new(int64_t num, uint64_t den, const uint32_t *e, int ne, int kind) {
    uint32_t h = h_alloc();
    holder_t *X = &H[h];
    X->num = num; X->den = den; X->ne = (uint8_t)ne; X->alive = 1; X->kind = (uint8_t)kind;
    if (ne) {
        X->cls = (uint8_t)cls_for(ne);
        uint32_t o = pool_get(X->cls);
        X = &H[h];
        X->eo = o;
        memcpy(EPL + o, e, 4 * (size_t)ne);
    } else { X->cls = 0xFF; X->eo = NONE; }
    return h;
}
static void h_free(uint32_t h) {
    holder_t *X = &H[h];
    if (corrupt_leak == 1) { corrupt_leak = 0; return; }  /* test hook: the holder is never freed (a leak) */
    if (X->cls != 0xFF) pool_put(X->eo, X->cls);
    X->alive = 0; X->ne = 0; X->cls = 0xFF;
    X->eo = hfree; hfree = h;
    hlive--;
}
static inline int h_find(uint32_t h, uint32_t node) {
    const holder_t *X = &H[h];
    const uint32_t *e = EPL + X->eo;
    for (int k = 0; k < X->ne; k++) if (EN(e[k]) == node) return k;
    return -1;
}
/* replace a holder's edges (C's rewrite after a split) */
static void h_set_edges(uint32_t h, const uint32_t *e, int ne) {
    holder_t *X = &H[h];
    if (ne > (X->cls == 0xFF ? 0 : (1 << X->cls))) {
        if (X->cls != 0xFF) pool_put(X->eo, X->cls);
        int c = cls_for(ne);
        uint32_t o = pool_get(c);
        X = &H[h];
        X->cls = (uint8_t)c; X->eo = o;
    }
    if (ne) memcpy(EPL + X->eo, e, 4 * (size_t)ne);
    X->ne = (uint8_t)ne;
}
#ifdef VAR_S
/* S: insert the edge (node, w) with list position pos, keeping the edges sorted */
static void h_insert_edge(uint32_t h, uint32_t node, uint32_t w, uint32_t pos) {
    holder_t *X = &H[h];
    if (w == 0 || w > 255) die("edge weight out of range");
    int cap = X->cls == 0xFF ? 0 : (1 << X->cls);
    if (X->ne == cap) {
        int c = cls_for(X->ne + 1);
        if (c > 4) die("too many edges on a holder");
        uint32_t o = pool_get(c);
        X = &H[h];
        if (X->ne) {
            memcpy(EPL + o, EPL + X->eo, 4 * (size_t)X->ne);
            memcpy(PPL + o, PPL + X->eo, 4 * (size_t)X->ne);
            pool_put(X->eo, X->cls);
        }
        X->eo = o; X->cls = (uint8_t)c;
    }
    uint32_t *e = EPL + X->eo, *p = PPL + X->eo;
    int k = X->ne;
    while (k > 0 && EN(e[k - 1]) > node) { e[k] = e[k - 1]; p[k] = p[k - 1]; k--; }
    e[k] = EP(node, w); p[k] = pos;
    X->ne++;
}
static void h_remove_edge(uint32_t h, int k) {
    holder_t *X = &H[h];
    uint32_t *e = EPL + X->eo, *p = PPL + X->eo;
    for (int j = k + 1; j < X->ne; j++) { e[j - 1] = e[j]; p[j - 1] = p[j]; }
    X->ne--;
}
#endif

/* ---------------------------------------------------------------- the distinct denominators of one entry */

typedef struct { uint64_t den; uint32_t cnt, hd; uint8_t bad; } dden;  /* hd: its transient holder (kind 3) */
static dden *dd;
static uint32_t ndd, ddcap;
static uint64_t *dhk;
static uint32_t *dhv;
static size_t dhcap, dhcnt;
static uint64_t st_ovf;

static void g_enter_den(uint32_t k, uint64_t den);  /* the graph's pass 1 for a new distinct denominator */
static void dd_begin(void) {
    ndd = 0;
    dhcap = 1024; dhcnt = 0;
    dhk = xcalloc(dhcap * sizeof *dhk);
    dhv = xmalloc(dhcap * sizeof *dhv);
}
static void dd_end(void) {
    xfree(dhk); xfree(dhv); xfree(dd);
    dhk = NULL; dhv = NULL; dd = NULL; ddcap = 0; ndd = 0;
}
static uint32_t dd_get(uint64_t den) {
    if (den == 0) die("zero denominator");
    size_t i = hsh(den) & (dhcap - 1);
    while (dhk[i]) {
        if (dhk[i] == den) return dhv[i];
        i = (i + 1) & (dhcap - 1);
    }
    if (2 * (dhcnt + 1) > dhcap) {
        uint64_t *ok = dhk;
        uint32_t *ov = dhv;
        size_t oc = dhcap;
        dhcap *= 2;
        dhk = xcalloc(dhcap * sizeof *dhk);
        dhv = xmalloc(dhcap * sizeof *dhv);
        for (size_t j = 0; j < oc; j++)
            if (ok[j]) {
                size_t g = hsh(ok[j]) & (dhcap - 1);
                while (dhk[g]) g = (g + 1) & (dhcap - 1);
                dhk[g] = ok[j]; dhv[g] = ov[j];
            }
        xfree(ok); xfree(ov);
        i = hsh(den) & (dhcap - 1);
        while (dhk[i]) i = (i + 1) & (dhcap - 1);
    }
    GROW(dd, ndd, ddcap, 1);
    dden *D = &dd[ndd];
    D->den = den; D->cnt = 0; D->hd = NONE; D->bad = 0;
    if (den >> 63) { st_ovf++; D->bad = 1; }
    else g_enter_den(ndd, den);
    dhk[i] = den; dhv[i] = ndd; dhcnt++;
    return ndd++;
}

/* ---------------------------------------------------------------- GMP product trees (batch gcd, remainders) */

typedef struct { int nlev; size_t *len; mpz_t **lv; } ptree;
static void pt_build(ptree *T, const uint64_t *x, size_t n) {
    int nl = 1;
    for (size_t m = n; m > 1; m = (m + 1) / 2) nl++;
    T->nlev = nl;
    T->len = malloc(sizeof(size_t) * nl);
    T->lv = malloc(sizeof(mpz_t *) * nl);
    T->len[0] = n;
    T->lv[0] = malloc(sizeof(mpz_t) * (n ? n : 1));
    for (size_t i = 0; i < n; i++) mpz_init_set_ui(T->lv[0][i], x[i]);
    for (int l = 1; l < nl; l++) {
        size_t m = (T->len[l - 1] + 1) / 2;
        T->len[l] = m;
        T->lv[l] = malloc(sizeof(mpz_t) * m);
        for (size_t j = 0; j < m; j++) {
            mpz_init(T->lv[l][j]);
            if (2 * j + 1 < T->len[l - 1]) mpz_mul(T->lv[l][j], T->lv[l - 1][2 * j], T->lv[l - 1][2 * j + 1]);
            else mpz_set(T->lv[l][j], T->lv[l - 1][2 * j]);
        }
    }
}
static void pt_free(ptree *T) {
    for (int l = 0; l < T->nlev; l++) {
        for (size_t j = 0; j < T->len[l]; j++) mpz_clear(T->lv[l][j]);
        free(T->lv[l]);
    }
    free(T->lv); free(T->len);
}
/* r[i] = Z mod x[i] for every leaf of T (a remainder tree) */
static void pt_mod_leaves(const ptree *T, const mpz_t Z, uint64_t *r) {
    if (!T->len[0]) return;
    int top = T->nlev - 1;
    mpz_t *cur = malloc(sizeof(mpz_t));
    mpz_init(cur[0]);
    mpz_mod(cur[0], Z, T->lv[top][0]);
    for (int l = top - 1; l >= 0; l--) {
        size_t m = T->len[l];
        mpz_t *nx = malloc(sizeof(mpz_t) * m);
        for (size_t j = 0; j < m; j++) { mpz_init(nx[j]); mpz_mod(nx[j], cur[j / 2], T->lv[l][j]); }
        for (size_t j = 0; j < T->len[l + 1]; j++) mpz_clear(cur[j]);
        free(cur);
        cur = nx;
    }
    for (size_t i = 0; i < T->len[0]; i++) { r[i] = mpz_get_ui(cur[i]); mpz_clear(cur[i]); }
    free(cur);
}
/* hit[i] = gcd(x[i], prod_{j != i} x[j]) > 1: Bernstein / Heninger batch gcd (remainders mod x^2) */
static void batch_gcd(const uint64_t *x, size_t n, uint8_t *hit) {
    memset(hit, 0, n);
    if (n < 2) return;
    ptree T;
    pt_build(&T, x, n);
    int top = T.nlev - 1;
    mpz_t sq;
    mpz_init(sq);
    mpz_t *cur = malloc(sizeof(mpz_t));
    mpz_init_set(cur[0], T.lv[top][0]);
    for (int l = top - 1; l >= 0; l--) {
        size_t m = T.len[l];
        mpz_t *nx = malloc(sizeof(mpz_t) * m);
        for (size_t j = 0; j < m; j++) {
            mpz_init(nx[j]);
            mpz_mul(sq, T.lv[l][j], T.lv[l][j]);
            mpz_mod(nx[j], cur[j / 2], sq);
        }
        for (size_t j = 0; j < T.len[l + 1]; j++) mpz_clear(cur[j]);
        free(cur);
        cur = nx;
    }
    for (size_t i = 0; i < n; i++) {
        mpz_divexact_ui(cur[i], cur[i], x[i]);  /* (Z mod x^2) / x = (Z / x) mod x */
        if (mpz_gcd_ui(NULL, cur[i], x[i]) != 1) hit[i] = 1;
        mpz_clear(cur[i]);
    }
    free(cur);
    mpz_clear(sq);
    pt_free(&T);
}
/* the leaves of T sharing a factor with x (descending only where the gcd is not 1); skip leaf `self` */
static void pt_partners(const ptree *T, int l, size_t j, uint64_t x, size_t self, size_t *out, size_t *nout, size_t cap) {
    if (mpz_gcd_ui(NULL, T->lv[l][j], x) == 1) return;
    if (l == 0) {
        if (j != self && *nout < cap) out[(*nout)++] = j;
        return;
    }
    if (2 * j < T->len[l - 1]) pt_partners(T, l - 1, 2 * j, x, self, out, nout, cap);
    if (2 * j + 1 < T->len[l - 1]) pt_partners(T, l - 1, 2 * j + 1, x, self, out, nout, cap);
}
/* the coprime audit: live node values pairwise coprime (batch gcd; pairwise gcds when small and `pairwise`) */
static const char *coprime_check(const uint64_t *v, size_t n, int pairwise) {
    if (pairwise) {
        for (size_t i = 0; i < n; i++)
            for (size_t j = i + 1; j < n; j++)
                if (gcd64(v[i], v[j]) != 1) return "nodes_not_coprime";
        return NULL;
    }
    uint8_t *hit = malloc(n + 1);
    batch_gcd(v, n, hit);
    const char *m = NULL;
    for (size_t i = 0; i < n && !m; i++) if (hit[i]) m = "nodes_not_coprime";
    free(hit);
    return m;
}

/* ---------------------------------------------------------------- value ops (ab_core.c's V0' code, 64-bit) */

typedef struct { int64_t num; uint64_t den; int ne; const uint32_t *e; } view;  /* den 0: bad (overflowed) */
typedef struct { int64_t num; uint64_t den; int ne; uint32_t e[AW]; } raw;
static inline view vraw(const raw *r) { return (view){r->num, r->den, r->ne, r->e}; }
static inline void setbad(raw *r) { r->num = 0; r->den = 0; r->ne = 0; }
static inline view hview(uint32_t h) {
    const holder_t *X = &H[h];
    return (view){X->num, X->den, X->ne, X->ne ? EPL + X->eo : NULL};
}

#ifdef VAR_S
static int s_partial(uint32_t id, u128 m);  /* S: a node with several atoms (or not prime) shares part of m: split */
#define HOOK(id, m) (nd[id].chk && s_partial((id), (m)))
#else
#define HOOK(id, m) 0
#endif

/* r := t / den over edges e, reduced against its nodes.  1 ok; 0 numerator overflow; -1 (S) a node was split:
 * every view is stale, so the caller restarts.  r->e may alias e. */
static int fin(i128 t, uint64_t den, const uint32_t *e, int ne, raw *r) {
    if (t == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    int neg = t < 0, k2 = 0;
    u128 m = neg ? (u128)0 - (u128)t : (u128)t;
    for (int k = 0; k < ne; k++) {
        uint32_t ek = e[k], id = EN(ek);
        const node_t *N = &nd[id];
        int w = (int)EW(ek), c = 0;
        while (c < w) {
            if ((m >> 64) == 0) {
                uint64_t q;
                if (!ndiv((uint64_t)m, N, &q)) break;
                m = q;
            } else {
                if (m % N->v) break;
                m /= N->v;
            }
            c++;
        }
        for (int j = 0; j < c; j++) den = ediv(den, N);
        if (c < w) {
            if (HOOK(id, m)) return -1;
            r->e[k2++] = EP(id, w - c);
        }
    }
    if (m > ((u128)NUM_LIM - (neg ? 0 : 1))) return 0;
    r->num = neg ? (int64_t)(0 - (uint64_t)m) : (int64_t)(uint64_t)m;
    r->den = den;
    r->ne = k2;
    return 1;
}
static inline int merge(const view *a, const view *b, uint32_t *n, uint8_t *wa, uint8_t *wb) {
    int i = 0, j = 0, k = 0;
    while (i < a->ne || j < b->ne) {
        uint32_t x = i < a->ne ? EN(a->e[i]) : UINT32_MAX, y = j < b->ne ? EN(b->e[j]) : UINT32_MAX;
        if (x < y) { n[k] = x; wa[k] = (uint8_t)EW(a->e[i]); wb[k] = 0; i++; }
        else if (y < x) { n[k] = y; wa[k] = 0; wb[k] = (uint8_t)EW(b->e[j]); j++; }
        else { n[k] = x; wa[k] = (uint8_t)EW(a->e[i]); wb[k] = (uint8_t)EW(b->e[j]); i++; j++; }
        k++;
    }
    return k;
}
static inline int mulpow(uint64_t *c, uint64_t v, int k) {
    while (k--) if (__builtin_mul_overflow(*c, v, c)) return 0;
    return 1;
}
static inline int lcm_plan(const view *a, const view *b, uint64_t *ca, uint64_t *cb, raw *r) {
    uint32_t n[AW];
    uint8_t wa[AW], wb[AW];
    int k = merge(a, b, n, wa, wb);
    uint64_t xa = 1, xb = 1, d;
    for (int i = 0; i < k; i++) {
        uint64_t v = nd[n[i]].v;
        if (wb[i] > wa[i]) { if (!mulpow(&xa, v, wb[i] - wa[i])) return 0; }
        else if (wa[i] > wb[i]) { if (!mulpow(&xb, v, wa[i] - wb[i])) return 0; }
        r->e[i] = EP(n[i], wa[i] > wb[i] ? wa[i] : wb[i]);
    }
    if (__builtin_mul_overflow(a->den, xa, &d) || (d >> 63)) return 0;
    r->den = d; r->ne = k; *ca = xa; *cb = xb;
    return 1;
}
/* r := a + b (a - b with neg).  1 ok, 0 overflow (r bad), -1 restart (S).  A bad operand gives a bad r. */
static int core_add(view a, view b, int neg, raw *r) {
    if (!a.den || !b.den) { setbad(r); return 1; }
    uint64_t ca, cb;
    if (!lcm_plan(&a, &b, &ca, &cb, r)) { setbad(r); return 0; }
    i128 x = (i128)a.num * (i128)ca, y = (i128)b.num * (i128)cb;
    int ok = fin(neg ? x - y : x + y, r->den, r->e, r->ne, r);
    if (ok == 0) setbad(r);
    return ok;
}
/* r := a * b, each numerator first cancelled against the other operand's nodes (lowest terms in, lowest out) */
static int core_mul(view a, view b, raw *r) {
    if (!a.den || !b.den) { setbad(r); return 1; }
    if (a.num == 0 || b.num == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    int neg = (a.num < 0) != (b.num < 0);
    uint64_t ua = mag(a.num), ub = mag(b.num), da = a.den, db = b.den, q;
    uint32_t n[AW];
    uint8_t wa[AW], wb[AW];
    int k = merge(&a, &b, n, wa, wb), ne = 0;
    for (int i = 0; i < k; i++) {
        int w = wa[i] + wb[i];
        const node_t *N = &nd[n[i]];
        int c = 0, d = 0;
        while (c < wb[i] && ndiv(ua, N, &q)) { ua = q; c++; db = ediv(db, N); }
        while (d < wa[i] && ndiv(ub, N, &q)) { ub = q; d++; da = ediv(da, N); }
        if (c < wb[i] && HOOK(n[i], ua)) return -1;
        if (d < wa[i] && HOOK(n[i], ub)) return -1;
        w -= c + d;
        if (w > 255) { setbad(r); return 0; }
        if (w) r->e[ne++] = EP(n[i], w);
    }
    uint64_t p, den;
    if (__builtin_mul_overflow(ua, ub, &p) || p > NUM_LIM - (neg ? 0 : 1)) { setbad(r); return 0; }
    if (__builtin_mul_overflow(da, db, &den) || (den >> 63)) { setbad(r); return 0; }
    r->num = neg ? (int64_t)(0 - p) : (int64_t)p;
    r->den = den;
    r->ne = ne;
    return 1;
}

/* audit helper: edges ascending, live nodes, positive weights, den == the product of the edges */
static const char *audit_edges(uint64_t den, int ne, const uint32_t *e) {
    u128 p = 1;
    for (int k = 0; k < ne; k++) {
        if (k && EN(e[k]) <= EN(e[k - 1])) return "edges_not_ascending";
        if (EN(e[k]) >= nnodes || !nd[EN(e[k])].alive) return "dead_node";
        if (!EW(e[k])) return "zero_weight";
        for (uint32_t j = 0; j < EW(e[k]); j++) {
            p *= nd[EN(e[k])].v;
            if (p >> 64) return "den_mismatch";
        }
    }
    return p == den ? NULL : "den_mismatch";
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

/* the graph's interface (g_s.c or g_c.c) */
static void g_init(void);
static void g_link(uint32_t h);
static void g_unlink(uint32_t h);
static void g_resolve(void);
static void g_entry_done(void);
static uint64_t g_idle(void);
static const char *g_audit(const char *what);
static void g_stats(FILE *f);
