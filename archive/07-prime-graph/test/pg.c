/* pg: a light prototype of the prime-graph rational.  See pg.h for the model and research/idea.md for the design. */
#include "pg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned __int128 u128;
typedef __int128 i128;

uint64_t pg_overflows;
int pg_abort_on_overflow;
int pg_keep_uniform;
int pg_use_cache = 1;

pg_node *pg_nodes;
uint32_t pg_nnodes;
static uint32_t nodes_cap;
pg_group *pg_groups;
uint32_t pg_ngroups;
static uint32_t groups_cap;

static void die(const char *m) { fprintf(stderr, "pg: %s\n", m); abort(); }
static void ovf(void) {
    pg_overflows++;
    if (pg_abort_on_overflow) die("overflow");
}
static inline uint64_t mag(int64_t a) { return a < 0 ? 0 - (uint64_t)a : (uint64_t)a; }
static inline uint64_t inv64(uint64_t m) {  /* m odd: m^-1 mod 2^64 by Newton */
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

/* ---------------------------------------------------------------- small primes, Miller-Rabin, Pollard rho */

#define NSMALL_MAX 6600
static uint64_t sp[NSMALL_MAX], spinv[NSMALL_MAX], splim[NSMALL_MAX];  /* odd primes below 2^16 */
static int nsmall;

static void sieve(void) {
    static uint8_t comp[65536];
    nsmall = 0;
    for (uint32_t i = 3; i < 65536; i += 2) {
        if (comp[i]) continue;
        sp[nsmall] = i; spinv[nsmall] = inv64(i); splim[nsmall] = UINT64_MAX / i; nsmall++;
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

int pg_is_prime(uint64_t n) {  /* deterministic for all 64-bit n with the first twelve prime bases */
    static const uint64_t B[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    if (n < 2) return 0;
    for (int i = 0; i < 12; i++) {
        if (n == B[i]) return 1;
        if (n % B[i] == 0) return 0;
    }
    uint64_t d = n - 1;
    int s = __builtin_ctzll(d);
    d >>= s;
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

static uint64_t rho(uint64_t n) {  /* Pollard-Brent: a nontrivial factor of an odd composite n */
    for (uint64_t c = 1;; c++) {
        uint64_t y = 2, x = 2, ys = 2, q = 1, g = 1, r = 1;
        const uint64_t m = 128;
#define F(z) ((uint64_t)(((u128)mulmod((z), (z), n) + c) % n))
        do {
            x = y;
            for (uint64_t i = 0; i < r; i++) y = F(y);
            uint64_t k = 0;
            do {
                ys = y;
                uint64_t lim = m < r - k ? m : r - k;
                for (uint64_t i = 0; i < lim; i++) {
                    y = F(y);
                    q = mulmod(q, x > y ? x - y : y - x, n);
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
    if (pg_is_prime(n)) { out[(*k)++] = n; return; }
    uint64_t d = rho(n);
    factor_big(d, out, k);
    factor_big(n / d, out, k);
}

int pg_factor(uint64_t n, uint64_t *P, uint32_t *E) {
    int k = 0;
    if (n <= 1) return 0;
    int s = __builtin_ctzll(n);                       /* stage 1: powers of 2 */
    if (s) { P[k] = 2; E[k++] = (uint32_t)s; n >>= s; }
    int i;
    for (i = 0; i < nsmall; i++) {                    /* stage 2: the table, with the adaptive stop */
        uint64_t p = sp[i];
        if (p * p > n) break;
        uint64_t q = n * spinv[i];
        if (q <= splim[i]) {
            uint32_t e = 0;
            do { n = q; e++; q = n * spinv[i]; } while (q <= splim[i]);
            P[k] = p; E[k++] = e;
        }
    }
    if (n == 1) return k;
    if (i < nsmall || n < (1ull << 32)) { P[k] = n; E[k++] = 1; return k; }  /* p^2 > leftover: prime */
    uint64_t f[64];                                   /* stages 3 and 4: Miller-Rabin, then Pollard rho */
    int nf = 0;
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

/* ---------------------------------------------------------------- nodes, groups, the denominator map */

static uint32_t node_new(uint64_t v, int prime) {
    if (pg_nnodes == nodes_cap) {
        nodes_cap = nodes_cap ? 2 * nodes_cap : 256;
        pg_nodes = realloc(pg_nodes, sizeof(pg_node) * nodes_cap);
        if (!pg_nodes) die("out of memory");
    }
    pg_node *N = &pg_nodes[pg_nnodes];
    N->v = v;
    N->s = (uint8_t)__builtin_ctzll(v);
    uint64_t m = v >> N->s;
    N->inv = inv64(m);
    N->lim = UINT64_MAX / m;
    N->prime = (uint8_t)prime;
    N->alive = 1;
    return pg_nnodes++;
}

static inline int ndiv(uint64_t u, const pg_node *N, uint64_t *q) {  /* exact-division test: multiply, compare */
    if (N->s) {
        if (u & ((1ull << N->s) - 1)) return 0;
        u >>= N->s;
    }
    uint64_t t = u * N->inv;
    if (t > N->lim) return 0;
    *q = t;
    return 1;
}

static uint64_t *dmk;  /* open addressing: denominator -> group id; key 0 is empty */
static uint32_t *dmv;
static size_t dmcap, dmcnt;
static inline size_t hsh(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return (size_t)x;
}
static inline uint32_t dm_get(uint64_t d) {
    size_t i = hsh(d) & (dmcap - 1);
    while (dmk[i]) {
        if (dmk[i] == d) return dmv[i];
        i = (i + 1) & (dmcap - 1);
    }
    return PG_BAD;
}
static void dm_ins(uint64_t d, uint32_t g) {
    size_t i = hsh(d) & (dmcap - 1);
    while (dmk[i]) i = (i + 1) & (dmcap - 1);
    dmk[i] = d; dmv[i] = g;
}
static void dm_put(uint64_t d, uint32_t g) {
    if (2 * (dmcnt + 1) > dmcap) {
        uint64_t *ok = dmk; uint32_t *ov = dmv; size_t oc = dmcap;
        dmcap = dmcap ? 2 * dmcap : 1024;
        dmk = calloc(dmcap, sizeof *dmk); dmv = malloc(dmcap * sizeof *dmv);
        if (!dmk || !dmv) die("out of memory");
        for (size_t i = 0; i < oc; i++) if (ok[i]) dm_ins(ok[i], ov[i]);
        free(ok); free(ov);
    }
    dm_ins(d, g);
    dmcnt++;
}

static uint32_t group_new(uint64_t den, const uint32_t *node, const uint8_t *w, int ne) {
    if (pg_ngroups == groups_cap) {
        groups_cap = groups_cap ? 2 * groups_cap : 256;
        pg_groups = realloc(pg_groups, sizeof(pg_group) * groups_cap);
        if (!pg_groups) die("out of memory");
    }
    pg_group *G = &pg_groups[pg_ngroups];
    memset(G, 0, sizeof *G);
    G->den = den;
    G->ne = (uint8_t)ne;
    for (int k = 0; k < ne; k++) { G->node[k] = node[k]; G->w[k] = w[k]; }
    for (int k = 0; k < PG_MAXE; k++) G->dec[k] = PG_BAD;
    dm_put(den, pg_ngroups);
    return pg_ngroups++;
}

/* The group of a sorted edge list, or PG_BAD if it has more than 8 edges or its denominator reaches 2^63.  On a
 * coprime base the denominator determines the edge set, so the lookup is by denominator. */
static uint32_t intern(const uint32_t *node, const uint8_t *w, int ne) {
    if (ne > PG_MAXE) return PG_BAD;
    uint64_t d = 1;
    for (int k = 0; k < ne; k++) {
        uint64_t v = pg_nodes[node[k]].v;
        for (int j = 0; j < w[k]; j++)
            if (__builtin_mul_overflow(d, v, &d) || (d >> 63)) return PG_BAD;
    }
    uint32_t g = dm_get(d);
    return g != PG_BAD ? g : group_new(d, node, w, ne);
}

static uint32_t group_dec(uint32_t g, int k) {  /* group g with edge k's weight lowered by one */
    uint32_t d = pg_groups[g].dec[k];
    if (d != PG_BAD) return d;
    pg_group G = pg_groups[g];
    uint64_t den = G.den / pg_nodes[G.node[k]].v;
    d = dm_get(den);
    if (d == PG_BAD) {
        if (--G.w[k] == 0) {
            for (int j = k; j + 1 < G.ne; j++) { G.node[j] = G.node[j + 1]; G.w[j] = G.w[j + 1]; }
            G.ne--;
        }
        d = group_new(den, G.node, G.w, G.ne);
    }
    pg_groups[g].dec[k] = d;
    return d;
}

static void sort_edges(uint32_t *node, uint8_t *w, int ne) {
    for (int a = 1; a < ne; a++)
        for (int b = a; b > 0 && node[b - 1] > node[b]; b--) {
            uint32_t t = node[b]; node[b] = node[b - 1]; node[b - 1] = t;
            uint8_t u = w[b]; w[b] = w[b - 1]; w[b - 1] = u;
        }
}

/* ---------------------------------------------------------------- pair plans (cached per pair of groups) */

#define PCBITS 16
typedef struct { uint32_t ga, gb, epoch, gr; uint64_t ca, cb; } add_plan;
typedef struct { uint32_t ga, gb, epoch, gs; uint8_t ne, wa[2 * PG_MAXE], wb[2 * PG_MAXE]; uint32_t node[2 * PG_MAXE]; } mul_plan;
static add_plan *acache;
static mul_plan *mcache;
static uint32_t epoch = 1;  /* bumped on every split: invalidates all plans */
static inline size_t pidx(uint32_t ga, uint32_t gb) { return (uint32_t)(ga * 0x9E3779B1u + gb * 0x85EBCA77u) >> (32 - PCBITS); }

static int merge(uint32_t ga, uint32_t gb, uint32_t *node, uint8_t *wa, uint8_t *wb) {
    const pg_group *A = &pg_groups[ga], *B = &pg_groups[gb];
    int i = 0, j = 0, k = 0;
    while (i < A->ne || j < B->ne) {
        if (j >= B->ne || (i < A->ne && A->node[i] < B->node[j])) { node[k] = A->node[i]; wa[k] = A->w[i]; wb[k] = 0; i++; }
        else if (i >= A->ne || B->node[j] < A->node[i]) { node[k] = B->node[j]; wa[k] = 0; wb[k] = B->w[j]; j++; }
        else { node[k] = A->node[i]; wa[k] = A->w[i]; wb[k] = B->w[j]; i++; j++; }
        k++;
    }
    return k;
}

static void make_add_plan(uint32_t ga, uint32_t gb, add_plan *P) {
    uint32_t node[2 * PG_MAXE]; uint8_t wa[2 * PG_MAXE], wb[2 * PG_MAXE], w[2 * PG_MAXE];
    int k = merge(ga, gb, node, wa, wb);
    for (int i = 0; i < k; i++) w[i] = wa[i] > wb[i] ? wa[i] : wb[i];  /* each node at its maximum weight */
    P->ga = ga; P->gb = gb; P->epoch = epoch;
    P->gr = intern(node, w, k);
    if (P->gr != PG_BAD) {
        P->ca = pg_groups[P->gr].den / pg_groups[ga].den;
        P->cb = pg_groups[P->gr].den / pg_groups[gb].den;
    }
}
static void make_mul_plan(uint32_t ga, uint32_t gb, mul_plan *P) {
    uint8_t w[2 * PG_MAXE];
    P->ga = ga; P->gb = gb; P->epoch = epoch;
    P->ne = (uint8_t)merge(ga, gb, P->node, P->wa, P->wb);
    for (int i = 0; i < P->ne; i++) w[i] = P->wa[i] + P->wb[i];
    P->gs = intern(P->node, w, P->ne);  /* PG_BAD if the uncancelled product does not fit; handled per value */
}
static inline const add_plan *get_add_plan(uint32_t ga, uint32_t gb, add_plan *tmp) {
    if (!pg_use_cache) { make_add_plan(ga, gb, tmp); return tmp; }
    add_plan *P = &acache[pidx(ga, gb)];
    if (P->ga != ga || P->gb != gb || P->epoch != epoch) make_add_plan(ga, gb, P);
    return P;
}
static inline const mul_plan *get_mul_plan(uint32_t ga, uint32_t gb, mul_plan *tmp) {
    if (!pg_use_cache) { make_mul_plan(ga, gb, tmp); return tmp; }
    mul_plan *P = &mcache[pidx(ga, gb)];
    if (P->ga != ga || P->gb != gb || P->epoch != epoch) make_mul_plan(ga, gb, P);
    return P;
}

/* ---------------------------------------------------------------- the per-element cores */

/* Cancel the magnitude m against the nodes of group g, as many times as each divides (up to its weight). */
static uint32_t reduce(u128 *pm, uint32_t g) {
    const pg_group *G = &pg_groups[g];
    int ne = G->ne;
    uint32_t node[PG_MAXE]; uint8_t w[PG_MAXE];
    memcpy(node, G->node, sizeof node); memcpy(w, G->w, sizeof w);  /* group_dec may move pg_groups */
    u128 m = *pm;
    for (int k = ne - 1; k >= 0; k--) {  /* descending, so lower edge indices stay put as edges drop out */
        const pg_node *N = &pg_nodes[node[k]];
        int c = 0;
        while (c < w[k]) {
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
        while (c--) g = group_dec(g, k);
    }
    *pm = m;
    return g;
}

static inline int fin(i128 t, uint32_t g, int64_t *rn, uint32_t *rg) {
    if (t == 0) { *rn = 0; *rg = 0; return 1; }
    int neg = t < 0;
    u128 m = neg ? (u128)0 - (u128)t : (u128)t;
    if (g) g = reduce(&m, g);
    if (m > (((u128)1 << 63) - (neg ? 0 : 1))) { ovf(); *rn = 0; *rg = PG_BAD; return 0; }
    *rn = neg ? (int64_t)(0 - (uint64_t)m) : (int64_t)(uint64_t)m;
    *rg = g;
    return 1;
}

static inline int core_addsub(int64_t a, uint32_t ga, int64_t b, uint32_t gb, int neg, int64_t *rn, uint32_t *rg) {
    if (ga == PG_BAD || gb == PG_BAD) { *rn = 0; *rg = PG_BAD; return 0; }
    if (ga == gb) {  /* same group: one integer add, then (unless pgu) the divisibility tests */
        int64_t r;
        if (neg ? __builtin_sub_overflow(a, b, &r) : __builtin_add_overflow(a, b, &r)) { ovf(); *rn = 0; *rg = PG_BAD; return 0; }
        if (pg_keep_uniform) { *rn = r; *rg = ga; return 1; }
        return fin((i128)r, ga, rn, rg);
    }
    add_plan tmp;
    const add_plan *P = get_add_plan(ga, gb, &tmp);
    if (P->gr == PG_BAD) { ovf(); *rn = 0; *rg = PG_BAD; return 0; }
    i128 x = (i128)a * (i128)P->ca, y = (i128)b * (i128)P->cb;
    return fin(neg ? x - y : x + y, P->gr, rn, rg);
}

static inline int core_mul(int64_t a, uint32_t ga, int64_t b, uint32_t gb, int64_t *rn, uint32_t *rg) {
    if (ga == PG_BAD || gb == PG_BAD) { *rn = 0; *rg = PG_BAD; return 0; }
    if (a == 0 || b == 0) { *rn = 0; *rg = 0; return 1; }
    int neg = (a < 0) != (b < 0);
    uint64_t ua = mag(a), ub = mag(b), q;
    mul_plan tmp;
    const mul_plan *P = get_mul_plan(ga, gb, &tmp);
    uint8_t cnt[2 * PG_MAXE];
    int any = 0;
    for (int k = 0; k < P->ne; k++) {  /* cancel each numerator against the other operand's nodes */
        const pg_node *N = &pg_nodes[P->node[k]];
        int c = 0, d = 0;
        while (c < P->wb[k] && ndiv(ua, N, &q)) { ua = q; c++; }
        while (d < P->wa[k] && ndiv(ub, N, &q)) { ub = q; d++; }
        cnt[k] = (uint8_t)(c + d);
        any |= c + d;
    }
    uint64_t p;
    if (__builtin_mul_overflow(ua, ub, &p) || p > (1ull << 63) - (neg ? 0 : 1)) { ovf(); *rn = 0; *rg = PG_BAD; return 0; }
    uint32_t g;
    if (P->gs != PG_BAD) {
        g = P->gs;
        if (any)
            for (int k = P->ne - 1; k >= 0; k--)
                for (int c = cnt[k]; c > 0; c--) g = group_dec(g, k);
    } else {  /* the uncancelled product overflowed: build the cancelled edge set directly */
        uint32_t node[2 * PG_MAXE]; uint8_t w[2 * PG_MAXE];
        int ne = 0;
        for (int k = 0; k < P->ne; k++) {
            int wk = P->wa[k] + P->wb[k] - cnt[k];
            if (wk) { node[ne] = P->node[k]; w[ne++] = (uint8_t)wk; }
        }
        g = intern(node, w, ne);
        if (g == PG_BAD) { ovf(); *rn = 0; *rg = PG_BAD; return 0; }
    }
    *rn = neg ? (int64_t)(0 - p) : (int64_t)p;
    *rg = g;
    return 1;
}

static int core_cmp(int64_t a, uint32_t ga, int64_t b, uint32_t gb) {
    if (ga == PG_BAD || gb == PG_BAD) return 2;
    if (ga == gb) return (a > b) - (a < b);
    add_plan tmp;
    const add_plan *P = get_add_plan(ga, gb, &tmp);
    i128 x, y;
    if (P->gr != PG_BAD) { x = (i128)a * (i128)P->ca; y = (i128)b * (i128)P->cb; }  /* same scaling as add */
    else { x = (i128)a * (i128)pg_groups[gb].den; y = (i128)b * (i128)pg_groups[ga].den; }
    return (x > y) - (x < y);
}

/* ---------------------------------------------------------------- splitting, entry, decomposition */

static struct { pg_val *v; size_t n; } pools[16];
static int npools, pools_dirty;

void pg_register_pool(pg_val *v, size_t n) {
    if (npools == 16) die("too many pools");
    pools[npools].v = v; pools[npools].n = n; npools++;
}

void pg_sync_pools(void) {  /* rewrite the inline edges of every registered value whose group was split */
    if (!pools_dirty) return;
    for (int p = 0; p < npools; p++)
        for (size_t i = 0; i < pools[p].n; i++) {
            pg_val *v = &pools[p].v[i];
            if (v->gid == PG_BAD || !pg_groups[v->gid].dirty) continue;
            const pg_group *G = &pg_groups[v->gid];
            v->ne = G->ne;
            memcpy(v->node, G->node, sizeof v->node); memcpy(v->w, G->w, sizeof v->w);
        }
    for (uint32_t g = 0; g < pg_ngroups; g++) pg_groups[g].dirty = 0;
    pools_dirty = 0;
}

/* Replace node id by parts pv[j]^pe[j] (their product is its value) and rewrite every group that uses it.
 * Denominators do not change, so group ids, the denominator map and every value stay valid. */
static void split_node(uint32_t id, const uint64_t *pv, const uint32_t *pe, int np, int prime, uint32_t *ids) {
    for (int j = 0; j < np; j++) ids[j] = node_new(pv[j], prime);
    pg_nodes[id].alive = 0;
    for (uint32_t g = 0; g < pg_ngroups; g++) {
        pg_group *G = &pg_groups[g];
        int k;
        for (k = 0; k < G->ne && G->node[k] != id; k++) {}
        if (k == G->ne) continue;
        uint32_t node[PG_MAXE + 64]; uint8_t w[PG_MAXE + 64];
        int ne = 0;
        for (int i = 0; i < G->ne; i++) if (i != k) { node[ne] = G->node[i]; w[ne++] = G->w[i]; }
        for (int j = 0; j < np; j++) { node[ne] = ids[j]; w[ne++] = (uint8_t)(G->w[k] * pe[j]); }
        if (ne > PG_MAXE) { pg_overflows++; die("a split needs more than 8 edges"); }
        sort_edges(node, w, ne);
        memset(G->node, 0, sizeof G->node); memset(G->w, 0, sizeof G->w);
        for (int i = 0; i < ne; i++) { G->node[i] = node[i]; G->w[i] = w[i]; }
        G->ne = (uint8_t)ne;
        for (int i = 0; i < PG_MAXE; i++) G->dec[i] = PG_BAD;
        G->dirty = 1;
    }
    epoch++;
    pools_dirty = 1;
}

/* A coprime base of {a, b}: pairwise coprime numbers > 1 of which a and b are both power products. */
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
                L[i] /= g; L[j] /= g;
                if (n == 64) die("coprime base too large");
                L[n++] = g;
                int m = 0;
                for (int t = 0; t < n; t++) if (L[t] > 1) L[m++] = L[t];
                n = m;
                again = 1;
            }
    }
    return n;
}

uint32_t pg_den_gid(uint64_t den) {
    if (den == 0) die("zero denominator");
    if (den == 1) return 0;
    if (den >> 63) { ovf(); return PG_BAD; }
    uint32_t g = dm_get(den);
    if (g != PG_BAD) return g;
    uint32_t node[64]; uint8_t w[64];
    int ne = 0;
    uint64_t left = den;
    uint32_t nn = pg_nnodes;  /* nodes made by splits below are divisors of a scanned node: coprime to left */
    for (uint32_t id = 0; id < nn && left > 1; id++) {
        if (!pg_nodes[id].alive) continue;
        uint64_t v = pg_nodes[id].v;
        if (gcd64(left, v) == 1) continue;
        uint64_t base[64], pv[64]; uint32_t pe[64], ids[64];
        int nb = coprime_base2(v, left, base), np = 0;  /* gcd refinement of the old node and the new one */
        for (int b = 0; b < nb; b++) {
            uint32_t e = 0;
            for (uint64_t t = v; t % base[b] == 0; t /= base[b]) e++;
            if (e) { pv[np] = base[b]; pe[np++] = e; }
        }
        if (np == 1 && pe[0] == 1) ids[0] = id;  /* equal, or v divides den: merge with the existing node */
        else split_node(id, pv, pe, np, 0, ids);
        for (int j = 0; j < np; j++) {
            uint32_t f = 0;
            while (left % pv[j] == 0) { left /= pv[j]; f++; }
            if (f) { node[ne] = ids[j]; w[ne++] = (uint8_t)f; }
        }
    }
    if (left > 1) { node[ne] = node_new(left, 0); w[ne++] = 1; }  /* one unsplit node, treated as prime */
    sort_edges(node, w, ne);
    g = intern(node, w, ne);
    if (g == PG_BAD) ovf();
    return g;
}

pg_val pg_val_of(int64_t num, uint32_t g) {
    pg_val v;
    v.num = num; v.gid = g;
    if (g == PG_BAD) { v.den = 0; v.ne = 0; memset(v.node, 0, sizeof v.node); memset(v.w, 0, sizeof v.w); return v; }
    const pg_group *G = &pg_groups[g];
    v.den = G->den; v.ne = G->ne;
    memcpy(v.node, G->node, sizeof v.node); memcpy(v.w, G->w, sizeof v.w);
    return v;
}

pg_val pg_from(int64_t num, uint64_t den) { return pg_val_of(num, pg_den_gid(den)); }

uint32_t pg_decompose_node(uint32_t id) {
    pg_node *N = &pg_nodes[id];
    if (!N->alive || N->prime) return 0;
    uint64_t p[64]; uint32_t e[64], ids[64];
    int k = pg_factor(N->v, p, e);
    if (k == 1 && e[0] == 1) { N->prime = 1; return 0; }
    split_node(id, p, e, k, 1, ids);
    return (uint32_t)k;
}

uint32_t pg_decompose_all(void) {
    uint32_t nn = pg_nnodes, splits = 0;
    for (uint32_t id = 0; id < nn; id++) splits += pg_decompose_node(id) != 0;
    pg_sync_pools();
    return splits;
}

/* ---------------------------------------------------------------- setup */

void pg_reset(void) {
    pg_nnodes = 0;
    pg_ngroups = 0;
    if (dmk) memset(dmk, 0, dmcap * sizeof *dmk);
    dmcnt = 0;
    npools = 0; pools_dirty = 0;
    epoch++;
    group_new(1, NULL, NULL, 0);  /* group 0: the integers */
}

void pg_init(void) {
    if (acache) return;
    sieve();
    acache = calloc((size_t)1 << PCBITS, sizeof *acache);
    mcache = calloc((size_t)1 << PCBITS, sizeof *mcache);
    if (!acache || !mcache) die("out of memory");
    pg_reset();
}

/* ---------------------------------------------------------------- single values */

pg_val pg_add(pg_val a, pg_val b) { int64_t n; uint32_t g; core_addsub(a.num, a.gid, b.num, b.gid, 0, &n, &g); return pg_val_of(n, g); }
pg_val pg_sub(pg_val a, pg_val b) { int64_t n; uint32_t g; core_addsub(a.num, a.gid, b.num, b.gid, 1, &n, &g); return pg_val_of(n, g); }
pg_val pg_mul(pg_val a, pg_val b) { int64_t n; uint32_t g; core_mul(a.num, a.gid, b.num, b.gid, &n, &g); return pg_val_of(n, g); }
int pg_cmp(pg_val a, pg_val b) { return core_cmp(a.num, a.gid, b.num, b.gid); }
pg_val pg_reduce(pg_val a) {
    if (a.gid == PG_BAD) return a;
    int64_t n; uint32_t g;
    fin((i128)a.num, a.gid, &n, &g);
    return pg_val_of(n, g);
}

/* ---------------------------------------------------------------- arrays */

#define GID(A, i) ((A)->uniform ? (A)->ugid : (A)->gid[i])

void pg_arr_alloc(pg_arr *a, size_t n) {
    a->n = n;
    a->num = malloc(n * sizeof *a->num + 1);
    a->gid = malloc(n * sizeof *a->gid + 1);
    a->uniform = 0; a->ugid = 0;
    if (!a->num || !a->gid) die("out of memory");
}
void pg_arr_free(pg_arr *a) { free(a->num); free(a->gid); a->num = NULL; a->gid = NULL; a->n = 0; }

void pg_arr_enter(pg_arr *a, const int64_t *num, const uint64_t *den) {
    uint64_t last = 0;
    uint32_t g = 0, g0 = 0;
    int same = 1;
    for (size_t i = 0; i < a->n; i++) {
        if (den[i] != last) { g = pg_den_gid(den[i]); last = den[i]; }
        a->num[i] = num[i];
        a->gid[i] = g;
        if (i == 0) g0 = g;
        same &= g == g0;
    }
    a->uniform = same && a->n > 0;
    a->ugid = g0;
}

void pg_arr_reduce(pg_arr *a) {
    uint32_t g0 = 0;
    int same = 1;
    for (size_t i = 0; i < a->n; i++) {
        int64_t x; uint32_t g = GID(a, i);
        if (g == PG_BAD) { x = 0; }
        else fin((i128)a->num[i], g, &x, &g);
        a->num[i] = x; a->gid[i] = g;
        if (i == 0) g0 = g;
        same &= g == g0;
    }
    a->uniform = same && a->n > 0; a->ugid = g0;
}

size_t pg_table_bytes(void) {
    return (size_t)nodes_cap * sizeof(pg_node) + (size_t)groups_cap * sizeof(pg_group) + dmcap * (sizeof *dmk + sizeof *dmv);
}

static void poison(pg_arr *r, size_t bad) {  /* an overflow inside a uniform loop poisons the whole result */
    if (!bad) return;
    r->uniform = 1; r->ugid = PG_BAD;
}

static void arr_addsub(pg_arr *r, const pg_arr *a, const pg_arr *b, int neg) {
    size_t n = a->n, bad = 0;
    if (a->uniform && b->uniform && a->ugid != PG_BAD && b->ugid != PG_BAD) {
        uint32_t ga = a->ugid, gb = b->ugid;
        if (ga == gb && pg_keep_uniform) {  /* one group, unreduced (pgu): the vector form */
            for (size_t i = 0; i < n; i++) {
                int64_t x;
                if (neg ? __builtin_sub_overflow(a->num[i], b->num[i], &x) : __builtin_add_overflow(a->num[i], b->num[i], &x)) { ovf(); bad++; }
                r->num[i] = x;
            }
            r->uniform = 1; r->ugid = ga; poison(r, bad);
            return;
        }
        if (pg_keep_uniform) {
            add_plan tmp;
            const add_plan *P = get_add_plan(ga, gb, &tmp);
            if (P->gr == PG_BAD) { ovf(); r->uniform = 1; r->ugid = PG_BAD; return; }
            int64_t ca = (int64_t)P->ca, cb = (int64_t)P->cb;
            for (size_t i = 0; i < n; i++) {
                int64_t x, y, z;
                int o = (int)__builtin_mul_overflow(a->num[i], ca, &x) | (int)__builtin_mul_overflow(b->num[i], cb, &y);
                o |= neg ? __builtin_sub_overflow(x, y, &z) : __builtin_add_overflow(x, y, &z);
                if (o) { ovf(); bad++; }
                r->num[i] = z;
            }
            r->uniform = 1; r->ugid = P->gr; poison(r, bad);
            return;
        }
    }
    uint32_t g0 = 0;
    int same = 1;
    for (size_t i = 0; i < n; i++) {
        int64_t x; uint32_t g;
        core_addsub(a->num[i], GID(a, i), b->num[i], GID(b, i), neg, &x, &g);
        r->num[i] = x; r->gid[i] = g;
        if (i == 0) g0 = g;
        same &= g == g0;
    }
    r->uniform = same && n > 0; r->ugid = g0;
}
void pg_arr_add(pg_arr *r, const pg_arr *a, const pg_arr *b) { arr_addsub(r, a, b, 0); }
void pg_arr_sub(pg_arr *r, const pg_arr *a, const pg_arr *b) { arr_addsub(r, a, b, 1); }

void pg_arr_mul(pg_arr *r, const pg_arr *a, const pg_arr *b) {
    size_t n = a->n, bad = 0;
    if (pg_keep_uniform && a->uniform && b->uniform && a->ugid != PG_BAD && b->ugid != PG_BAD) {
        mul_plan tmp;
        const mul_plan *P = get_mul_plan(a->ugid, b->ugid, &tmp);
        if (P->gs == PG_BAD) { ovf(); r->uniform = 1; r->ugid = PG_BAD; return; }
        for (size_t i = 0; i < n; i++) {
            int64_t x;
            if (__builtin_mul_overflow(a->num[i], b->num[i], &x)) { ovf(); bad++; }
            r->num[i] = x;
        }
        r->uniform = 1; r->ugid = P->gs; poison(r, bad);
        return;
    }
    uint32_t g0 = 0;
    int same = 1;
    for (size_t i = 0; i < n; i++) {
        int64_t x; uint32_t g;
        core_mul(a->num[i], GID(a, i), b->num[i], GID(b, i), &x, &g);
        r->num[i] = x; r->gid[i] = g;
        if (i == 0) g0 = g;
        same &= g == g0;
    }
    r->uniform = same && n > 0; r->ugid = g0;
}

void pg_arr_fma(pg_arr *r, const pg_arr *a, const pg_arr *b, const pg_arr *c) {
    size_t n = a->n, bad = 0;
    if (pg_keep_uniform && a->uniform && b->uniform && c->uniform && a->ugid != PG_BAD && b->ugid != PG_BAD && c->ugid != PG_BAD) {
        mul_plan tm;
        const mul_plan *M = get_mul_plan(a->ugid, b->ugid, &tm);
        if (M->gs == PG_BAD) { ovf(); r->uniform = 1; r->ugid = PG_BAD; return; }
        add_plan ta;
        const add_plan *A = get_add_plan(M->gs, c->ugid, &ta);
        if (A->gr == PG_BAD) { ovf(); r->uniform = 1; r->ugid = PG_BAD; return; }
        int64_t ca = (int64_t)A->ca, cb = (int64_t)A->cb;
        for (size_t i = 0; i < n; i++) {
            int64_t t, x, y, z;
            int o = __builtin_mul_overflow(a->num[i], b->num[i], &t);
            o |= (int)__builtin_mul_overflow(t, ca, &x) | (int)__builtin_mul_overflow(c->num[i], cb, &y);
            o |= __builtin_add_overflow(x, y, &z);
            if (o) { ovf(); bad++; }
            r->num[i] = z;
        }
        r->uniform = 1; r->ugid = A->gr; poison(r, bad);
        return;
    }
    uint32_t g0 = 0;
    int same = 1;
    for (size_t i = 0; i < n; i++) {
        int64_t t, x; uint32_t gt, g;
        core_mul(a->num[i], GID(a, i), b->num[i], GID(b, i), &t, &gt);
        core_addsub(t, gt, c->num[i], GID(c, i), 0, &x, &g);
        r->num[i] = x; r->gid[i] = g;
        if (i == 0) g0 = g;
        same &= g == g0;
    }
    r->uniform = same && n > 0; r->ugid = g0;
}

pg_val pg_arr_sum(const pg_arr *a) {
    int64_t rn = 0; uint32_t rg = 0;
    if (a->uniform) {  /* one group: one integer sum */
        if (a->ugid == PG_BAD) return pg_val_of(0, PG_BAD);
        i128 s = 0;
        for (size_t i = 0; i < a->n; i++) s += a->num[i];
        fin(s, a->ugid, &rn, &rg);
        return pg_val_of(rn, rg);
    }
    uint32_t ng = pg_ngroups;
    i128 *acc = calloc(ng, sizeof *acc);
    if (!acc) die("out of memory");
    for (size_t i = 0; i < a->n; i++) {  /* integer sums within each group */
        uint32_t g = a->gid[i];
        if (g >= ng) { free(acc); return pg_val_of(0, PG_BAD); }
        acc[g] += a->num[i];
    }
    for (uint32_t g = 0; g < ng && rg != PG_BAD; g++) {  /* then graph addition of the group sums */
        if (!acc[g]) continue;
        if (acc[g] > INT64_MAX || acc[g] < INT64_MIN) { ovf(); rg = PG_BAD; break; }
        core_addsub(rn, rg, (int64_t)acc[g], g, 0, &rn, &rg);
    }
    free(acc);
    if (rg != PG_BAD) fin((i128)rn, rg, &rn, &rg);
    return pg_val_of(rn, rg);
}

pg_val pg_arr_get(const pg_arr *a, size_t i) { return pg_val_of(a->num[i], GID(a, i)); }
