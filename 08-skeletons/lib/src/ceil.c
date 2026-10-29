/* ceil.c: the shared-denominator ceiling (spec §9).  Plain loops over the numerators, with no other structure. */
#include "sk_ceil.h"

#include "sk_internal.h"

typedef struct ceil_vec { /* E plus the per-vector header (pointer, length, unused) */
    sk_int *n;
    uint32_t len;
    uint32_t unused;
    sk_int E;
} ceil_vec;

typedef struct ceil_list {
    ceil_vec *v;
    uint32_t count, cap;
} ceil_list;

struct sk_ceil {
    ceil_list stored;  /* entered and result vectors */
    ceil_list working; /* gathered working copies: not stored values, not in the memory report */
    uint64_t elements; /* elements of stored vectors */
    sk_int *tp, *tq;   /* temporaries of divide */
    uint32_t tcap;
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
#endif
#ifdef SK_AUDIT
    uint32_t mark;
    char msg[160];
#endif
};

#define OVF(step)                                                                                                 \
    do {                                                                                                          \
        free(n);                                                                                                  \
        return sk_overflow_at(&c->ovf, &c->ctx, (step));                                                          \
    } while (0)

sk_ceil *sk_ceil_create(void) { return sk_xcalloc(1, sizeof(sk_ceil)); }

void sk_ceil_destroy(sk_ceil *c) {
    if (!c) return;
    for (uint32_t k = 0; k < c->stored.count; k++) free(c->stored.v[k].n);
    for (uint32_t k = 0; k < c->working.count; k++) free(c->working.v[k].n);
    free(c->stored.v);
    free(c->working.v);
    free(c->tp);
    free(c->tq);
#ifdef SK_COUNT
    sk_cur = &sk_sink;
#endif
    free(c);
}

static const ceil_vec *get(const sk_ceil *c, sk_handle h) {
    if (h & SK_CEIL_WORKING) {
        h &= ~SK_CEIL_WORKING;
        return h < c->working.count ? &c->working.v[h] : NULL;
    }
    return h < c->stored.count ? &c->stored.v[h] : NULL;
}

static sk_handle append(ceil_list *l, sk_int *n, uint32_t len, sk_int E, uint32_t limit, const char *what) {
    sk_check_limit(l->count >= limit ? SK_LIMIT : l->count, what);
    SK_RESERVE(l->v, l->count, l->cap, 16);
    l->v[l->count].n = n;
    l->v[l->count].len = len;
    l->v[l->count].unused = 0;
    l->v[l->count].E = E;
    return l->count++;
}

/* content (spec §9 item 3): gcd(E, n_1, n_2, ...), stopping when it reaches 1; divide E and every n_k by it. */
static void content(sk_int *E, sk_int *n, uint32_t len) {
    sk_int g = *E;
    for (uint32_t k = 0; k < len && g != 1; k++) {
        CNT(scan_content, 1);
        g = sk_gcd(g, n[k]);
    }
    if (g == 1) return;
    *E = sk_divexact(*E, g);
    for (uint32_t k = 0; k < len; k++) {
        CNT(scan_content, 1);
        n[k] = sk_divexact(n[k], g);
    }
}

static sk_status store(sk_ceil *c, sk_int *n, uint32_t len, sk_int E, sk_handle *out) {
    CNT_SET(c->cnt, c->ctx.op, SK_STORE);
    *out = append(&c->stored, n, len, E, SK_CEIL_WORKING, "stored vectors");
    c->elements += len;
    return SK_OK;
}

sk_status sk_ceil_enter_vector(sk_ceil *c, const sk_int *m, uint32_t len, sk_int d, sk_handle *out) {
    if (len > 0 && !m) return SK_EINVAL;
    if (!sk_valid_input(d)) return SK_EINVAL;
    for (uint32_t k = 0; k < len; k++)
        if (!sk_valid_input(m[k])) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    CNT_SET(c->cnt, SK_PH_ENTRY, SK_CORE);
    sk_int *n = sk_xmalloc(sizeof(sk_int) * (len ? len : 1));
    bool neg = d < 0; /* move the sign of d to the numerators; negation is not counted */
    for (uint32_t k = 0; k < len; k++) n[k] = neg ? -m[k] : m[k];
    sk_int E = neg ? -d : d;
    content(&E, n, len);
    return store(c, n, len, E, out);
}

static sk_status gather(sk_ceil *c, sk_handle v, const uint32_t *pos, const unsigned char *negate, uint32_t len,
                        sk_handle *out) {
    const ceil_vec *s = get(c, v);
    if (!s || (len > 0 && !pos)) return SK_EINVAL;
    for (uint32_t k = 0; k < len; k++)
        if (pos[k] >= s->len) return SK_EINVAL;
    sk_int *n = sk_xmalloc(sizeof(sk_int) * (len ? len : 1));
    for (uint32_t k = 0; k < len; k++) n[k] = (negate && negate[k]) ? -s->n[pos[k]] : s->n[pos[k]];
    sk_int E = s->E; /* `s` may move when the working list grows */
    *out = append(&c->working, n, len, E, SK_CEIL_WORKING, "working copies") | SK_CEIL_WORKING;
    return SK_OK;
}

sk_status sk_ceil_gather(sk_ceil *c, sk_handle v, const uint32_t *positions, uint32_t len, sk_handle *out) {
    return gather(c, v, positions, NULL, len, out);
}

sk_status sk_ceil_gather_signed(sk_ceil *c, sk_handle v, const uint32_t *positions, const unsigned char *negate,
                                uint32_t len, sk_handle *out) {
    return gather(c, v, positions, negate, len, out);
}

static sk_status begin(sk_ceil *c, sk_phase op, sk_handle x, sk_handle y, const ceil_vec **vx,
                       const ceil_vec **vy) {
    *vx = get(c, x);
    *vy = get(c, y);
    if (!*vx || !*vy || (*vx)->len != (*vy)->len) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    CNT_SET(c->cnt, op, SK_CORE);
    return SK_OK;
}

static sk_status addsub(sk_ceil *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    const ceil_vec *vx, *vy;
    sk_status st = begin(c, op, x, y, &vx, &vy);
    if (st) return st;
    uint32_t len = vx->len;
    sk_int *n = sk_xmalloc(sizeof(sk_int) * (len ? len : 1));
    sk_int g = sk_gcd(vx->E, vy->E);
    sk_int fx = sk_divexact(vy->E, g); /* E/E_x, which is E_y/g (change log CL-2) */
    sk_int E, fy, a, b;
    if (sk_mul(vx->E, fx, &E)) OVF("add: E = E_x*(E_y/g)");
    fy = sk_divexact(E, vy->E);
    for (uint32_t k = 0; k < len; k++) {
        if (sk_mul(vx->n[k], fx, &a)) OVF("add: x_k*(E/E_x)");
        if (sk_mul(vy->n[k], fy, &b)) OVF("add: y_k*(E/E_y)");
        if (op == SK_PH_SUB ? sk_sub(a, b, &n[k]) : sk_add(a, b, &n[k])) OVF("add: n_k = x_k*(E/E_x) +- y_k*(E/E_y)");
    }
    content(&E, n, len);
    return store(c, n, len, E, out);
}

sk_status sk_ceil_add(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status sk_ceil_sub(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status sk_ceil_mul(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out) {
    const ceil_vec *vx, *vy;
    sk_status st = begin(c, SK_PH_MUL, x, y, &vx, &vy);
    if (st) return st;
    uint32_t len = vx->len;
    sk_int *n = sk_xmalloc(sizeof(sk_int) * (len ? len : 1));
    for (uint32_t k = 0; k < len; k++)
        if (sk_mul(vx->n[k], vy->n[k], &n[k])) OVF("multiply: x_k*y_k");
    sk_int E;
    if (sk_mul(vx->E, vy->E, &E)) OVF("multiply: E_x*E_y");
    content(&E, n, len);
    return store(c, n, len, E, out);
}

sk_status sk_ceil_div(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out) {
    const ceil_vec *vx, *vy;
    sk_status st = begin(c, SK_PH_DIV, x, y, &vx, &vy);
    if (st) return st;
    uint32_t len = vx->len;
    for (uint32_t k = 0; k < len; k++)
        if (vy->n[k] == 0) return SK_EDIVZERO; /* for the whole operation */
    if (len > c->tcap) {
        c->tcap = len;
        c->tp = sk_xrealloc(c->tp, sizeof(sk_int) * len);
        c->tq = sk_xrealloc(c->tq, sizeof(sk_int) * len);
    }
    sk_int *n = NULL;
    for (uint32_t k = 0; k < len; k++) {
        /* each element in lowest terms, as read gives it, then the control's divide (DECISIONS.md) */
        sk_int gx = sk_gcd(vx->n[k], vx->E), gy = sk_gcd(vy->n[k], vy->E);
        sk_int a = sk_divexact(vx->n[k], gx), b = sk_divexact(vx->E, gx);
        sk_int cc = sk_divexact(vy->n[k], gy), d = sk_divexact(vy->E, gy);
        bool fast;
        const char *step = sk_ctl_divide_pair(a, b, cc, d, &c->tp[k], &c->tq[k], &fast);
        if (step) OVF(step);
        if (fast) CNT(ev_fast_paths, 1);
    }
    /* E: the lcm of the q_k, one element at a time with a gcd */
    sk_int E = len ? c->tq[0] : 1;
    for (uint32_t k = 1; k < len; k++) {
        sk_int g = sk_gcd(E, c->tq[k]);
        if (sk_mul(E, sk_divexact(c->tq[k], g), &E)) OVF("divide: E = lcm of the q_k");
    }
    n = sk_xmalloc(sizeof(sk_int) * (len ? len : 1));
    for (uint32_t k = 0; k < len; k++)
        if (sk_mul(c->tp[k], sk_divexact(E, c->tq[k]), &n[k])) OVF("divide: p_k*(E/q_k)");
    content(&E, n, len);
    return store(c, n, len, E, out);
}

sk_status sk_ceil_read(sk_ceil *c, sk_handle v, uint32_t k, sk_int *m, sk_int *d) {
    const ceil_vec *s = get(c, v);
    if (!s || k >= s->len) return SK_EINVAL;
    CNT_SET(c->cnt, SK_PH_READ, SK_CORE);
    sk_int g = sk_gcd(s->n[k], s->E);
    *m = sk_divexact(s->n[k], g);
    *d = sk_divexact(s->E, g);
    return SK_OK;
}

sk_status sk_ceil_length(const sk_ceil *c, sk_handle v, uint32_t *len) {
    const ceil_vec *s = get(c, v);
    if (!s) return SK_EINVAL;
    *len = s->len;
    return SK_OK;
}

uint32_t sk_ceil_size(const sk_ceil *c) { return c->stored.count; }

void sk_ceil_memory(const sk_ceil *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    /* stored vectors only: numerators, plus E and the header per vector (sizeof(ceil_vec): 8 + 16 at w = 64) */
    uint64_t w = sizeof(sk_int);
    sk_memory_add(r, "stored vectors", c->elements * w + (uint64_t)c->stored.count * sizeof(ceil_vec),
                  c->elements * w + (uint64_t)c->stored.cap * sizeof(ceil_vec));
    sk_memory_finish(r, c->elements);
}

sk_status sk_ceil_counters(const sk_ceil *c, sk_counters *r) {
#ifdef SK_COUNT
    *r = c->cnt;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

sk_status sk_ceil_overflow(const sk_ceil *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits */

#ifdef SK_AUDIT
static const char *check(const sk_ceil *c, uint32_t from) {
    for (uint32_t v = from; v < c->stored.count; v++) {
        const ceil_vec *s = &c->stored.v[v];
        if (s->E < 1) return "vector: E below 1";
        sk_int g = s->E;
        for (uint32_t k = 0; k < s->len; k++) {
            if (s->n[k] == SK_INT_MIN) return "vector: a numerator is the most negative integer";
            g = sk_gcd(g, s->n[k]);
        }
        if (g != 1) return "vector: not in canonical form";
    }
    return NULL;
}

static sk_status result(sk_ceil *c, const char *f, const char **why) {
    if (f) {
        snprintf(c->msg, sizeof c->msg, "ceil: %s", f);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark = c->stored.count;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status sk_ceil_audit(sk_ceil *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *f = check(c, 0);
    sk_cur = saved;
    return result(c, f, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status sk_ceil_audit_scheduled(sk_ceil *c, const char **why) { return sk_ceil_audit(c, why); }

sk_status sk_ceil_audit_changed(sk_ceil *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *f = check(c, c->mark);
    sk_cur = saved;
    return result(c, f, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

#ifdef SK_AUDIT
/* test build only: break an invariant, so the tests can check that the audits find it */
void sk_ceil_t_corrupt_E(sk_ceil *c, sk_handle v, sk_int E) { c->stored.v[v].E = E; }
#endif
