/* STUB -- NOT THE LIBRARY.  See sk.h.  A plain reduced-rational value list behind every scalar name (ctl, d1, d2)
 * and a plain vector store behind the ceiling's names, with counters, memory reports and audits shaped as the spec
 * says, so the harness's parsing and checks can be exercised.  Build flags: SK_WIDTH (64|32), SK_COUNTING, SK_AUDIT.
 * SK_STUB_BUG=<name>:<call> (environment, tests only; name ctl, d1 or d2) makes that scalar class, in the timing build
 * only, store a wrong numerator at that handle, so the harness's check and its item-5 path (a bug after the gates)
 * can be shown to fire.
 */
#include "sk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const sk_phase_names[SK_NPHASE] = {"entry", "add", "sub", "mul", "div"};
const char *const sk_counter_names[SK_NCTR] = {"gcds", "gcd_work", "divisions", "multiplications", "additions",
    "lookup_searches", "lookup_probes", "pairbit_reads", "scans", "refinements", "events"};

static void *xrealloc(void *p, size_t n) {
    p = realloc(p, n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); abort(); }
    return p;
}
static int bitsof(uint64_t x) { return x ? 64 - __builtin_clzll(x) : 0; }
static uint64_t mag(sk_int a) { return a < 0 ? (uint64_t)0 - (uint64_t)a : (uint64_t)a; }

typedef struct { sk_counters *k; int ph, part; } ctx;
static sk_int gcd_c(ctx *x, sk_int a, sk_int b) {
    uint64_t u = mag(a), v = mag(b);
#ifdef SK_COUNTING
    if (x && x->k) {
        int bb = bitsof(u > v ? u : v);
        x->k->c[x->ph][x->part][SK_C_GCDS]++;
        x->k->c[x->ph][x->part][SK_C_GCD_WORK] += (uint64_t)bb;
        x->k->gcd_hist[x->ph][x->part][bb]++;
    }
#else
    (void)x;
#endif
    while (v) { uint64_t t = u % v; u = v; v = t; }
    return (sk_int)u;
}
static int mulc(sk_int a, sk_int b, sk_int *r) { return __builtin_mul_overflow(a, b, r) || *r == -SK_MAX - 1; }
static int addc(sk_int a, sk_int b, sk_int *r) { return __builtin_add_overflow(a, b, r) || *r == -SK_MAX - 1; }

/* ------------------------------------------------------------------------------------------- scalar stub */
typedef struct { sk_int m, d; } pair;
typedef struct {
    pair *v; uint32_t n, cap, audited;
    sk_counters k;
    const char *err;
    long bug;
} sclass;

static sclass *s_create(const char *name) {
    sclass *c = calloc(1, sizeof *c);
    if (!c) abort();
    c->bug = -1;
#if !defined(SK_COUNTING) && !defined(SK_AUDIT)
    const char *b = getenv("SK_STUB_BUG");
    size_t n = strlen(name);
    if (b && !strncmp(b, name, n) && b[n] == ':') c->bug = atol(b + n + 1);
#else
    (void)name;
#endif
    return c;
}
static void s_destroy(sclass *c) { if (c) { free(c->v); free(c); } }
static int s_store(sclass *c, sk_int m, sk_int d, uint32_t *h) {
    if (m == 0) d = 1;
    if (c->n == c->cap) { c->cap = c->cap ? 2 * c->cap : 64; c->v = xrealloc(c->v, c->cap * sizeof(pair)); }
    if ((long)c->n == c->bug) m += 1;
    c->v[c->n] = (pair){m, d};
    *h = c->n++;
    return SK_OK;
}
static int s_enter(sclass *c, sk_int m, sk_int d, uint32_t *h) {
    if (d == 0) return SK_EDIVZERO;
    if (d < 0) { m = -m; d = -d; }
    ctx x = {&c->k, SK_PH_ENTRY, SK_CORE};
    sk_int g = gcd_c(&x, m, d);
    return s_store(c, m / g, d / g, h);
}
#define OVF(step) do { c->err = step; return SK_EOVERFLOW; } while (0)
static int s_op(sclass *c, int ph, uint32_t ix, uint32_t iy, uint32_t *h) {
    if (ix >= c->n || iy >= c->n) return SK_EINVAL;
    pair x = c->v[ix], y = c->v[iy];
    ctx k = {&c->k, ph, SK_CORE};
    sk_int n, d, t1, t2;
    if (ph == SK_PH_SUB) { y.m = -y.m; ph = SK_PH_ADD; }
    if (ph == SK_PH_ADD) {
        sk_int g = gcd_c(&k, x.d, y.d);
        if (mulc(x.m, y.d / g, &t1) || mulc(y.m, x.d / g, &t2) || addc(t1, t2, &n)) OVF("add: t");
        sk_int hh = gcd_c(&k, n, g);
        if (mulc(x.d / g, y.d / hh, &d)) OVF("add: e");
        n /= hh;
    } else if (ph == SK_PH_MUL) {
        sk_int g1 = gcd_c(&k, x.m, y.d), g2 = gcd_c(&k, y.m, x.d);
        if (mulc(x.m / g1, y.m / g2, &n) || mulc(x.d / g2, y.d / g1, &d)) OVF("mul");
    } else {
        if (y.m == 0) return SK_EDIVZERO;
        sk_int s = y.m < 0 ? -1 : 1, t = y.m < 0 ? -y.m : y.m;
        sk_int g1 = gcd_c(&k, x.m, t), g2 = gcd_c(&k, x.d, y.d);
        if (mulc(s * (x.m / g1), y.d / g2, &n) || mulc(x.d / g2, t / g1, &d)) OVF("div");
    }
    return s_store(c, n, d, h);
}
static int s_read(const sclass *c, uint32_t x, sk_int *m, sk_int *d) {
    if (x >= c->n) return SK_EINVAL;
    *m = c->v[x].m; *d = c->v[x].d;
    return SK_OK;
}
static void s_memory(const sclass *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    r->nstruct = 1;
    r->s[0].name = "values";
    r->s[0].used = 16ull * c->n;
    r->s[0].alloc = 16ull * c->cap;
    r->nvalues = c->n;
}
static int s_check(const sclass *c, uint32_t from, const char **msg) {
    for (uint32_t i = from; i < c->n; i++) {
        pair p = c->v[i];
        if (p.d <= 0 || gcd_c(NULL, p.m, p.d) != 1 || (p.m == 0 && p.d != 1)) { *msg = "value not in lowest terms"; return 1; }
    }
    return 0;
}

#define SCALAR_IMPL(P)                                                                                       \
    struct P##_class { sclass s; };                                                                          \
    P##_class *P##_create(void) { return (P##_class *)s_create(#P); }                                          \
    void P##_destroy(P##_class *c) { s_destroy((sclass *)c); }                                               \
    int P##_enter(P##_class *c, sk_int m, sk_int d, uint32_t *h) { return s_enter(&c->s, m, d, h); }         \
    int P##_add(P##_class *c, uint32_t x, uint32_t y, uint32_t *h) { return s_op(&c->s, SK_PH_ADD, x, y, h); } \
    int P##_sub(P##_class *c, uint32_t x, uint32_t y, uint32_t *h) { return s_op(&c->s, SK_PH_SUB, x, y, h); } \
    int P##_mul(P##_class *c, uint32_t x, uint32_t y, uint32_t *h) { return s_op(&c->s, SK_PH_MUL, x, y, h); } \
    int P##_div(P##_class *c, uint32_t x, uint32_t y, uint32_t *h) { return s_op(&c->s, SK_PH_DIV, x, y, h); } \
    int P##_read(const P##_class *c, uint32_t x, sk_int *m, sk_int *d) { return s_read(&c->s, x, m, d); }    \
    uint32_t P##_size(const P##_class *c) { return c->s.n; }                                                 \
    void P##_memory(const P##_class *c, sk_memory *r) { s_memory(&c->s, r); }                                \
    void P##_counters(const P##_class *c, sk_counters *r) { *r = c->s.k; }                                   \
    const char *P##_last_error(const P##_class *c) { return c->s.err ? c->s.err : ""; }                      \
    int P##_audit(P##_class *c, int last, const char **m) { (void)last; c->s.audited = c->s.n; return s_check(&c->s, 0, m); } \
    int P##_audit_changed(P##_class *c, const char **m) { int r = s_check(&c->s, c->s.audited, m); c->s.audited = c->s.n; return r; }
SCALAR_IMPL(ctl)
SCALAR_IMPL(d1)
SCALAR_IMPL(d2)

/* ------------------------------------------------------------------------------------------- ceiling stub */
typedef struct { sk_int E; sk_int *n; uint32_t len; int stored; } vec;
struct ceil_class { vec *v; uint32_t nv, cap, audited; sk_counters k; const char *err; };

ceil_class *ceil_create(void) { ceil_class *c = calloc(1, sizeof *c); if (!c) abort(); return c; }
void ceil_destroy(ceil_class *c) {
    if (!c) return;
    for (uint32_t i = 0; i < c->nv; i++) free(c->v[i].n);
    free(c->v); free(c);
}
static uint32_t c_new(ceil_class *c, uint32_t len, int stored) {
    if (c->nv == c->cap) { c->cap = c->cap ? 2 * c->cap : 16; c->v = xrealloc(c->v, c->cap * sizeof(vec)); }
    c->v[c->nv] = (vec){1, xrealloc(NULL, (len ? len : 1) * sizeof(sk_int)), len, stored};
    return c->nv++;
}
static void c_content(ceil_class *c, vec *v, int ph) {
    ctx k = {&c->k, ph, SK_CORE};
    sk_int g = v->E;
    for (uint32_t i = 0; i < v->len && g != 1; i++) g = gcd_c(&k, g, v->n[i]);
    if (g > 1) { v->E /= g; for (uint32_t i = 0; i < v->len; i++) v->n[i] /= g; }
}
int ceil_enter_vector(ceil_class *c, const sk_int *m, uint32_t len, sk_int d, uint32_t *out) {
    if (d == 0) return SK_EDIVZERO;
    uint32_t id = c_new(c, len, 1);
    vec *v = &c->v[id];
    memcpy(v->n, m, len * sizeof(sk_int));
    v->E = d;
    c_content(c, v, SK_PH_ENTRY);
    *out = id;
    return SK_OK;
}
int ceil_gather(ceil_class *c, uint32_t src, const uint32_t *pos, const unsigned char *neg, uint32_t len, uint32_t *out) {
    if (src >= c->nv) return SK_EINVAL;
    uint32_t id = c_new(c, len, 0);
    vec *s = &c->v[src], *w = &c->v[id];
    w->E = s->E;
    for (uint32_t i = 0; i < len; i++) w->n[i] = (neg && neg[i]) ? -s->n[pos[i]] : s->n[pos[i]];
    *out = id;
    return SK_OK;
}
static int c_addsub(ceil_class *c, uint32_t x, uint32_t y, int sgn, uint32_t *out) {
    if (x >= c->nv || y >= c->nv || c->v[x].len != c->v[y].len) return SK_EINVAL;
    ctx k = {&c->k, sgn > 0 ? SK_PH_ADD : SK_PH_SUB, SK_CORE};
    sk_int g = gcd_c(&k, c->v[x].E, c->v[y].E), E;
    if (mulc(c->v[x].E, c->v[y].E / g, &E)) { c->err = "add: E"; return SK_EOVERFLOW; }
    uint32_t id = c_new(c, c->v[x].len, 1);
    vec *a = &c->v[x], *b = &c->v[y], *r = &c->v[id];
    r->E = E;
    for (uint32_t i = 0; i < r->len; i++) {
        sk_int t1, t2;
        if (mulc(a->n[i], E / a->E, &t1) || mulc(b->n[i], E / b->E, &t2) || addc(t1, sgn * t2, &r->n[i])) {
            c->err = "add: n"; free(r->n); c->nv--; return SK_EOVERFLOW;
        }
    }
    c_content(c, r, k.ph);
    *out = id;
    return SK_OK;
}
int ceil_add(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v) { return c_addsub(c, x, y, 1, v); }
int ceil_sub(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v) { return c_addsub(c, x, y, -1, v); }
int ceil_mul(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v) { (void)c; (void)x; (void)y; (void)v; return SK_EINVAL; }
int ceil_div(ceil_class *c, uint32_t x, uint32_t y, uint32_t *v) { (void)c; (void)x; (void)y; (void)v; return SK_EINVAL; }
int ceil_read(const ceil_class *c, uint32_t v, uint32_t k, sk_int *m, sk_int *d) {
    if (v >= c->nv || k >= c->v[v].len) return SK_EINVAL;
    sk_int g = gcd_c(NULL, c->v[v].n[k], c->v[v].E);
    *m = c->v[v].n[k] / g; *d = c->v[v].E / g;
    return SK_OK;
}
void ceil_memory(const ceil_class *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    r->nstruct = 1;
    r->s[0].name = "stored vectors";
    for (uint32_t i = 0; i < c->nv; i++)
        if (c->v[i].stored) { r->s[0].used += 8ull * c->v[i].len + 8 + 16; r->nvalues += c->v[i].len; }
    r->s[0].alloc = r->s[0].used;
}
void ceil_counters(const ceil_class *c, sk_counters *r) { *r = c->k; }
const char *ceil_last_error(const ceil_class *c) { return c->err ? c->err : ""; }
static int c_check(ceil_class *c, uint32_t from, const char **msg) {
    for (uint32_t i = from; i < c->nv; i++) {
        if (!c->v[i].stored) continue;
        sk_int g = c->v[i].E;
        for (uint32_t j = 0; j < c->v[i].len; j++) g = gcd_c(NULL, g, c->v[i].n[j]);
        if (g != 1) { *msg = "vector not canonical"; return 1; }
    }
    c->audited = c->nv;
    return 0;
}
int ceil_audit(ceil_class *c, int last, const char **msg) { (void)last; return c_check(c, 0, msg); }
int ceil_audit_changed(ceil_class *c, const char **msg) { return c_check(c, c->audited, msg); }
