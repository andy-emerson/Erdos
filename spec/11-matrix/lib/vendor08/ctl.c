/* ctl.c: the control (spec §6).  One value list of (m, d) pairs, and nothing else; the gcd-first method. */
#include "sk_ctl.h"

#include "sk_internal.h"

typedef struct ctl_pair {
    sk_int m, d;
} ctl_pair;

struct sk_ctl {
    ctl_pair *v; /* the value list */
    uint32_t N, cap;
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
#endif
#ifdef SK_AUDIT
    uint32_t mark_N; /* change list: values from mark_N on are new since the last audit */
    char msg[160];
#endif
};

sk_ctl *sk_ctl_create(void) {
    sk_ctl *c = sk_xcalloc(1, sizeof *c);
    return c;
}

void sk_ctl_destroy(sk_ctl *c) {
    if (!c) return;
    free(c->v);
    free(c);
}

/* store (control): appends the pair, turning a zero numerator into 0/1 (spec §6, G-1). */
static sk_status store(sk_ctl *c, sk_int n, sk_int e, sk_handle *out) {
    CNT_SET(c->cnt, c->ctx.op, SK_STORE);
    if (n == 0) e = 1;
    sk_check_limit(c->N, "stored values");
    SK_RESERVE(c->v, c->N, c->cap, 16);
    c->v[c->N].m = n;
    c->v[c->N].d = e;
    *out = c->N++;
    return SK_OK;
}

static sk_status begin(sk_ctl *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    CNT_SET(c->cnt, op, SK_CORE);
    return SK_OK;
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_status sk_ctl_enter(sk_ctl *c, sk_int m, sk_int d, sk_handle *out) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    CNT_SET(c->cnt, SK_PH_ENTRY, SK_CORE);
    if (d < 0) { /* move the sign of d to m (spec §4.2 item 4); negation is not counted */
        m = -m;
        d = -d;
    }
    sk_int g = sk_gcd(m, d);
    return store(c, sk_divexact(m, g), sk_divexact(d, g), out);
}

/* add, and subtract as addition of the negated second operand (spec §4.2 item 3) */
static sk_status addsub(sk_ctl *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, op, x, y);
    if (st) return st;
    sk_int mx = c->v[x].m, dx = c->v[x].d, my = c->v[y].m, dy = c->v[y].d;
    if (op == SK_PH_SUB) my = -my;
    sk_int t, a, b, e;
    if (dx == dy) {
        CNT(ev_fast_paths, 1);
        if (sk_add(mx, my, &t)) OVF("add: t = m_x + m_y");
        sk_int h = sk_gcd(t, dx);
        return store(c, sk_divexact(t, h), sk_divexact(dx, h), out);
    }
    sk_int g = sk_gcd(dx, dy);
    if (g == 1) {
        if (sk_mul(mx, dy, &a)) OVF("add: m_x*d_y");
        if (sk_mul(my, dx, &b)) OVF("add: m_y*d_x");
        if (sk_add(a, b, &t)) OVF("add: m_x*d_y + m_y*d_x");
        if (sk_mul(dx, dy, &e)) OVF("add: d_x*d_y");
        return store(c, t, e, out);
    }
    sk_int ry = sk_divexact(dy, g), rx = sk_divexact(dx, g);
    if (sk_mul(mx, ry, &a)) OVF("add: m_x*(d_y/g)");
    if (sk_mul(my, rx, &b)) OVF("add: m_y*(d_x/g)");
    if (sk_add(a, b, &t)) OVF("add: t = m_x*(d_y/g) + m_y*(d_x/g)");
    sk_int h = sk_gcd(t, g);
    sk_int n = sk_divexact(t, h);
    if (sk_mul(rx, sk_divexact(dy, h), &e)) OVF("add: (d_x/g)*(d_y/h)");
    return store(c, n, e, out);
}

sk_status sk_ctl_add(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status sk_ctl_sub(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status sk_ctl_mul(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    sk_int mx = c->v[x].m, dx = c->v[x].d, my = c->v[y].m, dy = c->v[y].d;
    sk_int n, e;
    if (dx == dy) {
        CNT(ev_fast_paths, 1);
        if (sk_mul(mx, my, &n)) OVF("multiply: m_x*m_y");
        if (sk_mul(dx, dx, &e)) OVF("multiply: d_x*d_x");
        return store(c, n, e, out);
    }
    sk_int g1 = sk_gcd(mx, dy);
    sk_int g2 = sk_gcd(my, dx);
    if (sk_mul(sk_divexact(mx, g1), sk_divexact(my, g2), &n)) OVF("multiply: (m_x/g_1)*(m_y/g_2)");
    if (sk_mul(sk_divexact(dx, g2), sk_divexact(dy, g1), &e)) OVF("multiply: (d_x/g_2)*(d_y/g_1)");
    return store(c, n, e, out);
}

sk_status sk_ctl_div(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    if (c->v[y].m == 0) return SK_EDIVZERO;
    sk_status st = begin(c, SK_PH_DIV, x, y);
    if (st) return st;
    sk_int n, e;
    bool fast;
    const char *step = sk_ctl_divide_pair(c->v[x].m, c->v[x].d, c->v[y].m, c->v[y].d, &n, &e, &fast);
    if (step) OVF(step);
    if (fast) CNT(ev_fast_paths, 1);
    return store(c, n, e, out);
}

sk_status sk_ctl_read(const sk_ctl *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    *m = c->v[x].m;
    *d = c->v[x].d;
    return SK_OK;
}

uint32_t sk_ctl_size(const sk_ctl *c) { return c->N; }

void sk_ctl_memory(const sk_ctl *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "values", (uint64_t)c->N * sizeof(ctl_pair), (uint64_t)c->cap * sizeof(ctl_pair));
    sk_memory_finish(r, c->N);
}

sk_status sk_ctl_counters(const sk_ctl *c, sk_counters *r) {
#ifdef SK_COUNT
    *r = c->cnt;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

sk_status sk_ctl_overflow(const sk_ctl *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

/* ----------------------------------------------------------------------------------------- audits */

#ifdef SK_AUDIT
static const char *check_values(const sk_ctl *c, uint32_t from) {
    for (uint32_t i = from; i < c->N; i++) {
        sk_int m = c->v[i].m, d = c->v[i].d;
        if (d < 1) return "value: denominator not positive";
        if (m == SK_INT_MIN) return "value: numerator is the most negative integer";
        if (m == 0 && d != 1) return "value: zero not stored as 0/1";
        if (sk_gcd(m, d) != 1) return "value: not in lowest terms";
    }
    return NULL;
}

static sk_status audit_result(sk_ctl *c, const char *fail, uint32_t from, const char **why) {
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "ctl: %s (checking from value %u)", fail, from);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status sk_ctl_audit(sk_ctl *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    const char *fail = check_values(c, 0);
    sk_cur = saved;
    return audit_result(c, fail, 0, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status sk_ctl_audit_scheduled(sk_ctl *c, const char **why) { return sk_ctl_audit(c, why); }

sk_status sk_ctl_audit_changed(sk_ctl *c, const char **why) {
#ifdef SK_AUDIT
    sk_count *saved = sk_cur;
    sk_cur = &sk_sink;
    uint32_t from = c->mark_N;
    const char *fail = check_values(c, from);
    sk_cur = saved;
    return audit_result(c, fail, from, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

#ifdef SK_AUDIT
/* test build only: break an invariant, so the tests can check that the audits find it */
void sk_ctl_t_corrupt(sk_ctl *c, sk_handle x, sk_int m, sk_int d) {
    c->v[x].m = m;
    c->v[x].d = d;
}
#endif
