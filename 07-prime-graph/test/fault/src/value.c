/* value.c: single values (spec "Entering a value", "Arithmetic"), text, and the debug audit hook.
 * Single values are always reduced after every operation (I4); deferred reduction for single values is an open
 * decision that M1 leaves off. */
#include "kernels.h"

const char *pg_status_str(pg_status s) {
    switch (s) {
    case PG_OK: return "ok";
    case PG_OVERFLOW: return "overflow";
    case PG_DIVZERO: return "division by zero";
    case PG_INVALID: return "invalid argument";
    }
    return "unknown status";
}

void pgi_debug_audit(pg_graph *g) {
    const char *m = pg_audit(g);
    if (m) {
        fprintf(stderr, "primegraph: audit failed: %s\n", m);
        abort();
    }
}

static inline int live(const pg_value *v) { return v && v->kind == HK_VALUE; }  /* a released handle is not */

static inline pg_value *value_from_raw(pg_graph *g, const raw *r) { return pgi_holder_from_raw(g, r, HK_VALUE); }

/* Enter t / d (d >= 1, d <= 2^63): enter d (steps 1-4) as a linked transient holder, then reduce t against its
 * nodes.  When nothing cancels, the transient holder becomes the value. */
static pg_status enter_value(pg_graph *g, i128 t, uint64_t d, pg_value **out) {
    if (t == 0) { *out = pgi_holder_new(g, 0, 1, NULL, 0, HK_VALUE); return PG_OK; }
    if (d == 1) {
        if (t > INT64_MAX || t < INT64_MIN) return PG_OVERFLOW;
        *out = pgi_holder_new(g, (int64_t)t, 1, NULL, 0, HK_VALUE);
        return PG_OK;
    }
    pg_value *h = pgi_den_enter(g, d);
    raw r;
    int rc;
    for (;;) {
        view v = hview(g, h);
        rc = fin(g, t, d, v.e, v.ne, &r);
        if (rc != R_RESTART) break;
        g->st_restarts++;
    }
    if (rc == R_OVF) { pgi_holder_release(g, h); return PG_OVERFLOW; }
    if (r.den == d) {  /* nothing cancelled: the edges are d's own */
        h->num = r.num;
        g->hkind[HK_TMP]--;
        g->hkind[HK_VALUE]++;
        h->kind = HK_VALUE;
        *out = h;
    } else {
        *out = value_from_raw(g, &r);
        pgi_holder_release(g, h);
    }
    return PG_OK;
}

pg_status pg_value_new(pg_graph *g, int64_t num, int64_t den, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !out) return PG_INVALID;
    if (den == 0) return PG_DIVZERO;
    i128 t = num;
    uint64_t d = (uint64_t)den;
    if (den < 0) { t = -t; d = 0 - (uint64_t)den; }
    pg_status s = enter_value(g, t, d, out);
    DEBUG_AUDIT(g);
    return s;
}

pg_status pg_value_from_int(pg_graph *g, int64_t n, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !out) return PG_INVALID;
    *out = pgi_holder_new(g, n, 1, NULL, 0, HK_VALUE);
    return PG_OK;
}

pg_status pg_value_get(const pg_graph *g, const pg_value *v, int64_t *num, int64_t *den) {
    if (!g || !live(v)) return PG_INVALID;
    if (num) *num = v->num;
    if (den) *den = (int64_t)v->den;
    return PG_OK;
}

void pg_value_release(pg_graph *g, pg_value *v) {
    if (!v) return;
    if (v->kind != HK_VALUE) pgi_die("pg_value_release: not a live value (released twice?)");
    pgi_holder_release(g, v);
    DEBUG_AUDIT(g);
}

pg_status pg_value_clone(pg_graph *g, const pg_value *v, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !live(v) || !out) return PG_INVALID;
    *out = pgi_holder_new(g, v->num, v->den, HE(g, v), v->ne, HK_VALUE);
    return PG_OK;
}

/* ------------------------------------------------------------------------------------------------- arithmetic */

enum { B_ADD, B_SUB, B_MUL };

static pg_status binop(pg_graph *g, const pg_value *a, const pg_value *b, int op, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !live(a) || !live(b) || !out) return PG_INVALID;
    raw r;
    int rc;
    for (;;) {
        view va = hview(g, a), vb = hview(g, b);
        rc = op == B_MUL ? core_mul(g, va, vb, 1, &r) : core_add(g, va, vb, op == B_SUB, 1, &r);
        if (rc != R_RESTART) break;
        g->st_restarts++;
    }
    if (rc == R_OVF) return PG_OVERFLOW;
    *out = value_from_raw(g, &r);
    DEBUG_AUDIT(g);
    return PG_OK;
}

pg_status pg_add(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out) { return binop(g, a, b, B_ADD, out); }
pg_status pg_sub(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out) { return binop(g, a, b, B_SUB, out); }
pg_status pg_mul(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out) { return binop(g, a, b, B_MUL, out); }

/* a / b = a * (d_b / m_b): the sign of m_b moves to the numerator, |m_b| is entered as a new denominator, d_b's
 * node powers multiply into the numerator (overflow checked), and the result is reduced (cross-cancellation of
 * lowest-terms operands gives lowest terms). */
pg_status pg_div(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !live(a) || !live(b) || !out) return PG_INVALID;
    if (b->num == 0) return PG_DIVZERO;
    uint64_t mb = mag64(b->num);
    pg_value *t = pgi_den_enter(g, mb);
    raw r;
    int rc;
    for (;;) {
        view va = hview(g, a), vt = hview(g, t);
        view vi = {b->num < 0 ? -(int64_t)b->den : (int64_t)b->den, mb, vt.ne, vt.e};
        rc = core_mul(g, va, vi, 1, &r);
        if (rc != R_RESTART) break;
        g->st_restarts++;
    }
    pg_status s = PG_OK;
    if (rc == R_OVF) s = PG_OVERFLOW;
    else *out = value_from_raw(g, &r);
    pgi_holder_release(g, t);
    DEBUG_AUDIT(g);
    return s;
}

pg_status pg_neg(pg_graph *g, const pg_value *a, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !live(a) || !out) return PG_INVALID;
    if (a->num == INT64_MIN) return PG_OVERFLOW;
    *out = pgi_holder_new(g, -a->num, a->den, HE(g, a), a->ne, HK_VALUE);
    DEBUG_AUDIT(g);
    return PG_OK;
}

int pg_cmp(const pg_graph *g, const pg_value *a, const pg_value *b) {
    if (!g || !live(a) || !live(b)) return PG_CMP_INVALID;
    view va = hview(g, a), vb = hview(g, b);
    uint64_t ca, cb;
    uint32_t e[AW];
    u128 L;
    if (lcm_plan(g, &va, &vb, &ca, &cb, e, &L) < 0) pgi_die("pg_cmp: operand above the bounds");
    i128 x = (i128)va.num * (i128)ca, y = (i128)vb.num * (i128)cb;  /* each below 2^126 in magnitude */
    return (x > y) - (x < y);
}

int pg_sign(const pg_value *v) { return live(v) ? (v->num > 0) - (v->num < 0) : 0; }
int pg_is_zero(const pg_value *v) { return live(v) && v->num == 0; }

/* ------------------------------------------------------------------------------------------------- text */

pg_status pg_value_to_string(const pg_graph *g, const pg_value *v, char *buf, size_t cap) {
    (void)g;
    if (!live(v) || !buf) return PG_INVALID;
    char tmp[64];
    int n = v->den == 1 ? snprintf(tmp, sizeof tmp, "%lld", (long long)v->num)
                        : snprintf(tmp, sizeof tmp, "%lld/%llu", (long long)v->num, (unsigned long long)v->den);
    if (n < 0 || (size_t)n + 1 > cap) return PG_INVALID;
    memcpy(buf, tmp, (size_t)n + 1);
    return PG_OK;
}

static const char *parse_part(const char *s, int *neg, u128 *mag) {  /* [+-]digits, at most 38 digits */
    *neg = 0;
    if (*s == '+' || *s == '-') { *neg = *s == '-'; s++; }
    if (*s < '0' || *s > '9') return NULL;
    u128 m = 0;
    int nd = 0;
    while (*s >= '0' && *s <= '9') {
        if (++nd > 38) return NULL;
        m = m * 10 + (u128)(*s - '0');
        s++;
    }
    *mag = m;
    return s;
}

pg_status pg_value_from_string(pg_graph *g, const char *s, pg_value **out) {
    if (out) *out = NULL;
    if (!g || !s || !out) return PG_INVALID;
    int n1, n2 = 0;
    u128 a, b = 1;
    const char *p = parse_part(s, &n1, &a);
    if (!p) return PG_INVALID;
    if (*p == '/') {
        p = parse_part(p + 1, &n2, &b);
        if (!p) return PG_INVALID;
    }
    if (*p) return PG_INVALID;
    if (b == 0) return PG_DIVZERO;
    int neg = n1 != n2;
    if (a == 0) neg = 0;
    if (a > NUM_LIM || b > DEN_LIM) {  /* beyond 64 bits: reduce by gcd first, then the reduced form must fit */
        u128 d = pgi_gcd128(a, b);
        a /= d;
        b /= d;
        if (a > (u128)(NUM_LIM - (neg ? 0 : 1)) || b >= DEN_LIM) return PG_OVERFLOW;
    }
    i128 t = neg ? -(i128)a : (i128)a;
    pg_status st = enter_value(g, t, (uint64_t)b, out);
    DEBUG_AUDIT(g);
    return st;
}
