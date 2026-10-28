/* ab_core.c: the core shared by the ablation strategies V0' (the control) and V1..V5.  See ABLATION in
 * ablation_bench.py for the strategy list.  This file is #included into abl_bench.c, once per strategy binary, with
 * VARIANT set; everything is static.
 *
 * Model (V0', no grouping): a global node table (the denominator side of the graph: pairwise coprime u64 values,
 * prime after decomposition) and values that each carry their numerator (int64), their denominator (cached, < 2^63)
 * and an inline edge list (node id, weight) sorted by node id.  There is no group table and no pair-plan cache:
 * every operation merges its operands' edge lists and computes the shared nodes and the scalings itself.
 *
 * Entry is shared by every strategy: a transient hash from each distinct input denominator to its edge list, built
 * during entry (new denominators are gcd-refined against the live nodes, exactly as pg_den_gid does), used to copy
 * edge lists into the values, and freed when entry ends.  Nothing from it survives into the operations.
 *
 * Overflow: every intermediate is checked; nothing wraps.  A value whose result overflowed is "bad" (den 0) and
 * poisons whatever uses it; each overflow is counted once in ab_overflows.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../pg.h" /* pg_factor (decomposition: the same code as the prototype) and pg_init (its prime table) */

#define AB_MAXE 16           /* a denominator below 2^63 has at most 15 distinct prime factors */
#define AB_W (2 * AB_MAXE)   /* room for a merged edge list before its denominator is checked */
#define EN(x) ((x) >> 8)     /* packed edge: node id (24 bits), weight (8 bits) */
#define EW(x) ((x) & 0xFFu)
#define EP(n, w) (((uint32_t)(n) << 8) | (uint32_t)(w))

typedef unsigned __int128 u128;
typedef __int128 i128;

static uint64_t ab_overflows;
static int ab_abort_on_overflow;

static void die(const char *m) { fprintf(stderr, "ab: %s\n", m); abort(); }
static inline void ovf(void) {
    ab_overflows++;
    if (ab_abort_on_overflow) die("overflow");
}
static inline uint64_t mag(int64_t a) { return a < 0 ? 0 - (uint64_t)a : (uint64_t)a; }

/* ---------------------------------------------------------------- memory accounting (representation only) */

static size_t mem_cur, mem_peak;
static void *xmalloc(size_t n) {
    size_t *p = malloc(n + 16);
    if (!p) die("out of memory");
    p[0] = n;
    mem_cur += n;
    if (mem_cur > mem_peak) mem_peak = mem_cur;
    return p + 2;
}
static void *xcalloc(size_t n) { void *p = xmalloc(n); memset(p, 0, n); return p; }
static void xfree(void *q) {
    if (!q) return;
    size_t *p = (size_t *)q - 2;
    mem_cur -= p[0];
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

/* ---------------------------------------------------------------- nodes */

typedef struct { uint64_t v, inv, lim; uint8_t s, prime, alive; } ab_node;
static ab_node *nodes;
static uint32_t nnodes, ncap;

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
static uint32_t node_new(uint64_t v, int prime) {
    if (nnodes == ncap) {
        ncap = ncap ? 2 * ncap : 256;
        nodes = xrealloc(nodes, sizeof(ab_node) * ncap);
    }
    if (nnodes >= (1u << 24)) die("too many nodes");
    ab_node *N = &nodes[nnodes];
    N->v = v;
    N->s = (uint8_t)__builtin_ctzll(v);
    uint64_t m = v >> N->s;
    N->inv = inv64(m);
    N->lim = UINT64_MAX / m;
    N->prime = (uint8_t)prime;
    N->alive = 1;
    return nnodes++;
}
static inline int ndiv(uint64_t u, const ab_node *N, uint64_t *q) { /* exact-division test: multiply, compare */
    if (N->s) {
        if (u & ((1ull << N->s) - 1)) return 0;
        u >>= N->s;
    }
    uint64_t t = u * N->inv;
    if (t > N->lim) return 0;
    *q = t;
    return 1;
}
static inline uint64_t ediv(uint64_t d, const ab_node *N) { return (d >> N->s) * N->inv; } /* d / v, v | d */

static int coprime_base2(uint64_t a, uint64_t b, uint64_t *L) { /* as in pg.c */
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

/* Make den (1 < den < 2^63) a power product of live nodes: gcd refinement against every live node, as pg_den_gid
 * does, but a split only touches the node table (there are no groups to rewrite). */
static void refine(uint64_t den) {
    uint64_t left = den;
    uint32_t nn = nnodes;
    for (uint32_t id = 0; id < nn && left > 1; id++) {
        if (!nodes[id].alive) continue;
        uint64_t v = nodes[id].v;
        if (gcd64(left, v) == 1) continue;
        uint64_t base[64], pv[64];
        uint32_t pe[64];
        int nb = coprime_base2(v, left, base), np = 0;
        for (int b = 0; b < nb; b++) {
            uint32_t e = 0;
            for (uint64_t t = v; t % base[b] == 0; t /= base[b]) e++;
            if (e) { pv[np] = base[b]; pe[np++] = e; }
        }
        if (!(np == 1 && pe[0] == 1)) {
            nodes[id].alive = 0;
            for (int j = 0; j < np; j++) node_new(pv[j], 0);
        }
        for (int j = 0; j < np; j++)
            while (left % pv[j] == 0) left /= pv[j];
    }
    if (left > 1) node_new(left, 0);
}

static uint32_t ab_decompose(void) { /* every live node into primes (pg_factor: the prototype's stages 1-4) */
    uint32_t nn = nnodes, splits = 0;
    for (uint32_t id = 0; id < nn; id++) {
        if (!nodes[id].alive || nodes[id].prime) continue;
        uint64_t p[64];
        uint32_t e[64];
        int k = pg_factor(nodes[id].v, p, e);
        if (k == 1 && e[0] == 1) { nodes[id].prime = 1; continue; }
        nodes[id].alive = 0;
        for (int j = 0; j < k; j++) node_new(p[j], 1);
        splits++;
    }
    return splits;
}

/* ---------------------------------------------------------------- values: views and raw results */

typedef struct { int64_t num; uint64_t den; int ne; const uint32_t *e; } view;  /* den 0: bad */
typedef struct { int64_t num; uint64_t den; int ne; uint32_t e[AB_W]; } raw;
static inline view vraw(const raw *r) { return (view){r->num, r->den, r->ne, r->e}; }
static inline void setbad(raw *r) { r->num = 0; r->den = 0; r->ne = 0; }

/* r := t / den over edges e (reduced against its nodes when red).  Returns 0 if the numerator does not fit int64.
 * r->e may alias e.  With prime nodes and red, the result is in lowest terms. */
static inline int fin(i128 t, uint64_t den, const uint32_t *e, int ne, int red, raw *r) {
    if (t == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    int neg = t < 0, k2 = 0;
    u128 m = neg ? (u128)0 - (u128)t : (u128)t;
    if (red) {
        for (int k = 0; k < ne; k++) {
            uint32_t ek = e[k];
            const ab_node *N = &nodes[EN(ek)];
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
            if (c < w) r->e[k2++] = EP(EN(ek), w - c);
        }
    } else {
        if (e != r->e) memmove(r->e, e, sizeof(uint32_t) * ne);
        k2 = ne;
    }
    if (m > (((u128)1 << 63) - (neg ? 0 : 1))) return 0;
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

/* The common denominator of a and b (each node at its larger weight) and the two scalings: r->den, r->e, *ca, *cb.
 * Returns 0 if the common denominator reaches 2^63. */
static inline int lcm_plan(const view *a, const view *b, uint64_t *ca, uint64_t *cb, raw *r) {
    uint32_t n[AB_W];
    uint8_t wa[AB_W], wb[AB_W];
    int k = merge(a, b, n, wa, wb);
    uint64_t xa = 1, xb = 1, d;
    for (int i = 0; i < k; i++) {
        uint64_t v = nodes[n[i]].v;
        if (wb[i] > wa[i]) { if (!mulpow(&xa, v, wb[i] - wa[i])) return 0; }
        else if (wa[i] > wb[i]) { if (!mulpow(&xb, v, wa[i] - wb[i])) return 0; }
        r->e[i] = EP(n[i], wa[i] > wb[i] ? wa[i] : wb[i]);
    }
    if (__builtin_mul_overflow(a->den, xa, &d) || (d >> 63)) return 0;
    r->den = d; r->ne = k; *ca = xa; *cb = xb;
    return 1;
}

/* r := a + b, or a - b (neg); reduced when red.  Returns 0 on overflow (r bad); a bad operand gives a bad r and
 * returns 1 (the overflow was counted where it happened).  r may alias a or b. */
static inline int core_add(view a, view b, int neg, int red, raw *r) {
    if (!a.den || !b.den) { setbad(r); return 1; }
#if VARIANT == VAR_V1
    /* V1: the cached denominators identify the edge sets (coprime base), so check them before any edge work */
    if (a.den == b.den) {
        i128 t = neg ? (i128)a.num - b.num : (i128)a.num + b.num;
        if (!fin(t, a.den, a.e, a.ne, red, r)) { setbad(r); return 0; }
        return 1;
    }
    if (a.den > b.den) {
        uint64_t c = a.den / b.den;
        if (c * b.den == a.den) {
            i128 y = (i128)b.num * c, t = neg ? (i128)a.num - y : (i128)a.num + y;
            if (!fin(t, a.den, a.e, a.ne, red, r)) { setbad(r); return 0; }
            return 1;
        }
    } else {
        uint64_t c = b.den / a.den;
        if (c * a.den == b.den) {
            i128 x = (i128)a.num * c, t = neg ? x - b.num : x + b.num;
            if (!fin(t, b.den, b.e, b.ne, red, r)) { setbad(r); return 0; }
            return 1;
        }
    }
#endif
    uint64_t ca, cb;
    if (!lcm_plan(&a, &b, &ca, &cb, r)) { setbad(r); return 0; }
    i128 x = (i128)a.num * ca, y = (i128)b.num * cb;
    if (!fin(neg ? x - y : x + y, r->den, r->e, r->ne, red, r)) { setbad(r); return 0; }
    return 1;
}

/* r := a * b; with cancel, each numerator is first cancelled against the other operand's nodes (lowest terms when
 * the operands are).  Without cancel (V4's deferred form), the weights just add.  Returns as core_add. */
static inline int core_mul(view a, view b, int cancel, raw *r) {
    if (!a.den || !b.den) { setbad(r); return 1; }
    if (a.num == 0 || b.num == 0) { r->num = 0; r->den = 1; r->ne = 0; return 1; }
    int neg = (a.num < 0) != (b.num < 0);
    uint64_t ua = mag(a.num), ub = mag(b.num), da = a.den, db = b.den, q;
    uint32_t n[AB_W];
    uint8_t wa[AB_W], wb[AB_W];
    int k, ne = 0;
#if VARIANT == VAR_V1
    if (a.den == b.den) { /* V1: equal denominators, equal edge sets: no merge */
        k = a.ne;
        for (int i = 0; i < k; i++) { n[i] = EN(a.e[i]); wa[i] = wb[i] = (uint8_t)EW(a.e[i]); }
    } else
#endif
        k = merge(&a, &b, n, wa, wb);
    for (int i = 0; i < k; i++) {
        int w = wa[i] + wb[i];
        if (cancel) {
            const ab_node *N = &nodes[n[i]];
            int c = 0, d = 0;
            while (c < wb[i] && ndiv(ua, N, &q)) { ua = q; c++; db = ediv(db, N); }
            while (d < wa[i] && ndiv(ub, N, &q)) { ub = q; d++; da = ediv(da, N); }
            w -= c + d;
        }
        if (w > 255) { setbad(r); return 0; }
        if (w) r->e[ne++] = EP(n[i], w);
    }
    uint64_t p, den;
    if (__builtin_mul_overflow(ua, ub, &p) || p > (1ull << 63) - (neg ? 0 : 1)) { setbad(r); return 0; }
    if (__builtin_mul_overflow(da, db, &den) || (den >> 63)) { setbad(r); return 0; }
    r->num = neg ? (int64_t)(0 - p) : (int64_t)p;
    r->den = den;
    r->ne = ne;
    return 1;
}

/* An unreduced i128 accumulator (V2's bucket combination, V3's sum): acc += bn / b, merged and scaled, no reduction.
 * Returns 0 on overflow. */
typedef struct { i128 num; uint64_t den; int ne; uint32_t e[AB_W]; } raw128;
static inline int add128(raw128 *acc, i128 bn, view b) {
    view a = {0, acc->den, acc->ne, acc->e};
    uint64_t ca, cb;
    raw r;
    if (!lcm_plan(&a, &b, &ca, &cb, &r)) return 0;
    i128 x, y, z;
    if (__builtin_mul_overflow(acc->num, (i128)ca, &x) || __builtin_mul_overflow(bn, (i128)cb, &y) ||
        __builtin_add_overflow(x, y, &z)) return 0;
    acc->num = z; acc->den = r.den; acc->ne = r.ne;
    memcpy(acc->e, r.e, sizeof(uint32_t) * r.ne);
    return 1;
}

/* ---------------------------------------------------------------- entry (shared by every strategy) */

typedef struct { uint64_t den; uint8_t ne, bad; uint32_t e[AB_MAXE]; } dden;
static dden *dd;
static uint32_t ndd, ddcap;
static uint64_t *dhk;
static uint32_t *dhv;
static size_t dhcap, dhcnt;

static inline size_t hsh(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return (size_t)x;
}
static void dh_grow(void) {
    uint64_t *ok = dhk;
    uint32_t *ov = dhv;
    size_t oc = dhcap;
    dhcap *= 2;
    dhk = xcalloc(dhcap * sizeof *dhk);
    dhv = xmalloc(dhcap * sizeof *dhv);
    for (size_t i = 0; i < oc; i++)
        if (ok[i]) {
            size_t j = hsh(ok[i]) & (dhcap - 1);
            while (dhk[j]) j = (j + 1) & (dhcap - 1);
            dhk[j] = ok[i]; dhv[j] = ov[i];
        }
    xfree(ok); xfree(ov);
}
static uint32_t dd_get(uint64_t den) { /* the index of den among this entry's distinct denominators */
    if (den == 0) die("zero denominator");
    size_t i = hsh(den) & (dhcap - 1);
    while (dhk[i]) {
        if (dhk[i] == den) return dhv[i];
        i = (i + 1) & (dhcap - 1);
    }
    if (2 * (dhcnt + 1) > dhcap) {
        dh_grow();
        i = hsh(den) & (dhcap - 1);
        while (dhk[i]) i = (i + 1) & (dhcap - 1);
    }
    if (ndd == ddcap) { ddcap = ddcap ? 2 * ddcap : 256; dd = xrealloc(dd, ddcap * sizeof *dd); }
    dden *D = &dd[ndd];
    D->den = den; D->ne = 0; D->bad = 0;
    if (den >> 63) { ovf(); D->bad = 1; }
    else if (den > 1) refine(den);
    dhk[i] = den; dhv[i] = ndd; dhcnt++;
    return ndd++;
}

struct ab_arr;
typedef struct { struct ab_arr *a; const int64_t *num; uint32_t *di; int single; } pend;
static pend pending[8];
static int npend;

static void ab_enter_begin(void) {
    ndd = 0; ddcap = 0; dd = NULL;
    dhcap = 1024; dhcnt = 0;
    dhk = xcalloc(dhcap * sizeof *dhk);
    dhv = xmalloc(dhcap * sizeof *dhv);
    npend = 0;
}
static size_t arr_n(const struct ab_arr *a);
/* Pass 1: each element's distinct-denominator index; new denominators are refined against the nodes. */
static void ab_enter(struct ab_arr *a, const int64_t *num, const uint64_t *den, int single) {
    size_t n = arr_n(a);
    uint32_t *di = xmalloc(n * sizeof *di + 4);
    uint64_t last = 0;
    uint32_t li = 0;
    for (size_t i = 0; i < n; i++) {
        if (den[i] != last) { li = dd_get(den[i]); last = den[i]; }
        di[i] = li;
    }
    if (npend == 8) die("too many arrays entered");
    pending[npend++] = (pend){a, num, di, single};
}
static void fill_begin(void);
static void fill(struct ab_arr *a, const int64_t *num, const uint32_t *di, int single);
static void fill_end(void);
/* After decomposition: each distinct denominator's edges over the (prime) nodes, then pass 2 per array. */
static void ab_enter_end(void) {
    for (uint32_t k = 0; k < ndd; k++) {
        dden *D = &dd[k];
        if (D->bad || D->den == 1) continue;
        uint64_t left = D->den, q;
        int ne = 0;
        for (uint32_t id = 0; id < nnodes && left > 1; id++) {
            if (!nodes[id].alive) continue;
            int c = 0;
            while (ndiv(left, &nodes[id], &q)) { left = q; c++; }
            if (c) {
                if (ne == AB_MAXE) die("too many edges");
                D->e[ne++] = EP(id, c);
            }
        }
        if (left != 1) die("unresolved denominator");
        D->ne = (uint8_t)ne;
    }
    fill_begin();
    for (int p = 0; p < npend; p++) {
        fill(pending[p].a, pending[p].num, pending[p].di, pending[p].single);
        xfree(pending[p].di);
    }
    fill_end();
    xfree(dhk); xfree(dhv); xfree(dd);
    dhk = NULL; dhv = NULL; dd = NULL;
    npend = 0;
}

static void layout_reset(void);
static void ab_reset(void) {
    nnodes = 0;
    layout_reset();
}
static uint32_t ab_live_nodes(void) {
    uint32_t a = 0;
    for (uint32_t i = 0; i < nnodes; i++) a += nodes[i].alive;
    return a;
}

/* Structural audit helper: edges ascending, live nodes, positive weights, den == product of the edges. */
static const char *audit_edges(uint64_t den, int ne, const uint32_t *e) {
    u128 p = 1;
    for (int k = 0; k < ne; k++) {
        if (k && EN(e[k]) <= EN(e[k - 1])) return "edges_not_ascending";
        if (EN(e[k]) >= nnodes || !nodes[EN(e[k])].alive) return "dead_node";
        if (!EW(e[k])) return "zero_weight";
        for (uint32_t j = 0; j < EW(e[k]); j++) {
            p *= nodes[EN(e[k])].v;
            if (p >> 64) return "den_mismatch";
        }
    }
    return p == den ? NULL : "den_mismatch";
}
