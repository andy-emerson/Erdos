/* kernels.h: the arithmetic of spec "Arithmetic", as static inline kernels shared by value.c and array.c.
 *
 * Every kernel reads its operands as views (numerator, denominator, edges) and writes a raw result.  It returns
 * R_OK, R_OVF (the result does not fit the M1 bounds: numerator in int64, denominator below 2^63) or R_RESTART (a
 * reduction split a node: views and raw edge lists are stale and the caller recomputes from the holders).
 *
 * The fast path is round three's R0 code: for prime nodes a reduction is one multiply-and-compare per division
 * (ndiv) and nothing else.  A node with several atoms adds one flag test, and only when the node does not divide
 * the numerator (the spec's g < N case, pgi_partial); that is the whole cost of the dictionary's nodes in an
 * operation.  No epoch checks, no re-expansion of edge lists and no hooked copies of the kernels (round three's D
 * rows had all three), because splits rewrite the holders in place.
 *
 * Exactness.  A reducing kernel computes its result exactly in 128 bits (common denominators up to 2^126) and
 * reduces it fully before checking the bounds, so R_OVF means the exact reduced result does not fit.  The
 * unreduced kernels (V4) only check whether their unreduced result fits; their callers reduce and retry.
 */
#ifndef PG_KERNELS_H
#define PG_KERNELS_H

#include "pg_internal.h"

static inline view vraw(const raw *r) { return (view){r->num, r->den, r->ne, r->e}; }
static inline void setzero(raw *r) { r->num = 0; r->den = 1; r->ne = 0; }

/* r := t / den over the edges e, reduced against their nodes; den < 2^64.  r->e may alias e.  strict: R_OVF unless
 * the result is within the M1 bounds; else only unless it fits a raw (an operand's reduction before a V4 retry, whose
 * denominator may be 2^63). */
static inline int fin_k(pg_graph *g, i128 t, uint64_t den, const uint32_t *e, int ne, raw *r, int strict) {
    if (t == 0) { setzero(r); return R_OK; }
    int neg = t < 0, k2 = 0;
    u128 m = neg ? (u128)0 - (u128)t : (u128)t;
    for (int k = 0; k < ne; k++) {
        uint32_t ek = e[k], id = EN(ek);
        const node_hot *N = &g->nh[id];
        uint32_t w = EW(ek), c = 0;
        while (c < w) {
            if (LIKELY((m >> 64) == 0)) {
                uint64_t q;
                if (!ndiv((uint64_t)m, N, &q)) break;
                m = q;
            } else {
                if (m % N->v) break;
                m /= N->v;
            }
            c++;
        }
        for (uint32_t j = 0; j < c; j++) den = ediv(den, N);
        if (c < w) {
            if (UNLIKELY(N->multi) && pgi_partial(g, id, m, w - c)) return R_RESTART;
            r->e[k2++] = EP(id, w - c);
        }
    }
    if (m > (u128)(NUM_LIM - (neg ? 0 : 1)) || (strict && den >= DEN_LIM)) return R_OVF;
    r->num = neg ? (int64_t)(0 - (uint64_t)m) : (int64_t)(uint64_t)m;
    r->den = den;
    r->ne = k2;
    return R_OK;
}
static inline int fin(pg_graph *g, i128 t, uint64_t den, const uint32_t *e, int ne, raw *r) {
    return fin_k(g, t, den, e, ne, r, 1);
}

/* Wide reduction in place: (*num) / (*den) over e[0..*ne), den up to 2^128 - 1.  R_OK or R_RESTART; on a restart
 * the inputs are unchanged by this function (a pinned e is patched by the split itself). */
static inline int reduce_w(pg_graph *g, i128 *num, u128 *den, uint32_t *e, int *ne) {
    i128 t = *num;
    if (t == 0) { *den = 1; *ne = 0; return R_OK; }
    int neg = t < 0, k2 = 0;
    u128 m = neg ? (u128)0 - (u128)t : (u128)t, d = *den;
    uint32_t out[AW + 32];
    for (int k = 0; k < *ne; k++) {
        uint32_t ek = e[k], id = EN(ek);
        const node_hot *N = &g->nh[id];
        uint32_t w = EW(ek), c = 0;
        while (c < w) {
            if ((m >> 64) == 0) {
                uint64_t q;
                if (!ndiv((uint64_t)m, N, &q)) break;
                m = q;
            } else {
                if (m % N->v) break;
                m /= N->v;
            }
            d /= N->v;
            c++;
        }
        if (c < w) {
            if (N->multi && pgi_partial(g, id, m, w - c)) return R_RESTART;
            out[k2++] = EP(id, w - c);
        }
    }
    *num = neg ? -(i128)m : (i128)m;  /* m <= 2^127 here; -2^127 is representable */
    *den = d;
    memcpy(e, out, (size_t)k2 * 4);
    *ne = k2;
    return R_OK;
}

/* r := t / den reduced, den up to 2^128 - 1 (the rare wide case of an addition) */
static inline int fin_w(pg_graph *g, i128 t, u128 den, const uint32_t *e, int ne, raw *r) {
    uint32_t tmp[AW + 32];
    int k = ne;
    memcpy(tmp, e, (size_t)ne * 4);
    int rc = reduce_w(g, &t, &den, tmp, &k);
    if (rc != R_OK) return rc;
    if (t > INT64_MAX || t < INT64_MIN || den >= DEN_LIM) return R_OVF;
    r->num = (int64_t)t;
    r->den = (uint64_t)den;
    r->ne = k;
    memcpy(r->e, tmp, (size_t)k * 4);
    return R_OK;
}

/* the edge lists of a and b merged by node id: n[], with each one's weight (0 if absent) */
static inline int merge(const uint32_t *ae, int an, const uint32_t *be, int bn, uint32_t *n, uint8_t *wa, uint8_t *wb) {
    int i = 0, j = 0, k = 0;
    while (i < an || j < bn) {
        uint32_t x = i < an ? EN(ae[i]) : UINT32_MAX, y = j < bn ? EN(be[j]) : UINT32_MAX;
        if (x < y) { n[k] = x; wa[k] = (uint8_t)EW(ae[i]); wb[k] = 0; i++; }
        else if (y < x) { n[k] = y; wa[k] = 0; wb[k] = (uint8_t)EW(be[j]); j++; }
        else { n[k] = x; wa[k] = (uint8_t)EW(ae[i]); wb[k] = (uint8_t)EW(be[j]); i++; j++; }
        k++;
    }
    return k;
}

/* The common denominator of a and b (each node at its larger weight: a lookup, not a gcd): its edges into e[], its
 * value into *L (128 bits), and the two scalings.  Returns the edge count, or -1 if a scaling passes 2^64 (only
 * possible for operands above the M1 bounds). */
static inline int lcm_plan(const pg_graph *g, const view *a, const view *b, uint64_t *ca, uint64_t *cb, uint32_t *e,
                           u128 *L) {
    uint32_t n[AW];
    uint8_t wa[AW], wb[AW];
    int k = merge(a->e, a->ne, b->e, b->ne, n, wa, wb);
    uint64_t xa = 1, xb = 1;
    for (int i = 0; i < k; i++) {
        uint64_t v = g->nh[n[i]].v;
        if (wb[i] > wa[i]) { if (!mulpow(&xa, v, wb[i] - wa[i])) return -1; }
        else if (wa[i] > wb[i]) { if (!mulpow(&xb, v, wa[i] - wb[i])) return -1; }
        e[i] = EP(n[i], wa[i] > wb[i] ? wa[i] : wb[i]);
    }
    *L = (u128)a->den * xa;
    *ca = xa;
    *cb = xb;
    return k;
}

/* r := a + b (a - b with neg); operands below the M1 bounds.  red: reduce (exact); else V4's unreduced form. */
static inline int core_add(pg_graph *g, view a, view b, int neg, int red, raw *r) {
    uint64_t ca, cb;
    u128 L;
    int k = lcm_plan(g, &a, &b, &ca, &cb, r->e, &L);
    if (k < 0) return R_OVF;
    i128 x = (i128)a.num * (i128)ca, y = (i128)b.num * (i128)cb;  /* each below 2^126 in magnitude */
    i128 t = neg ? x - y : x + y;
    if (!red) {
        if (t == 0) { setzero(r); return R_OK; }
        if ((L >> 63) || t > INT64_MAX || t < INT64_MIN) return R_OVF;
        r->num = (int64_t)t;
        r->den = (uint64_t)L;
        r->ne = k;
        return R_OK;
    }
    if (UNLIKELY(L >> 64)) return fin_w(g, t, L, r->e, k, r);
    return fin(g, t, (uint64_t)L, r->e, k, r);
}

/* r := a * b.  cancel: each numerator is first cancelled against the other operand's nodes (lowest terms when the
 * operands are); else V4's unreduced form (the weights just add).  Denominators may be 2^63 here (a division's
 * inverse of -2^63). */
static inline int core_mul(pg_graph *g, view a, view b, int cancel, raw *r) {
    if (a.num == 0 || b.num == 0) { setzero(r); return R_OK; }
    int neg = (a.num < 0) != (b.num < 0);
    uint64_t ua = mag64(a.num), ub = mag64(b.num), da = a.den, db = b.den, q;
    uint32_t n[AW];
    uint8_t wa[AW], wb[AW];
    int k = merge(a.e, a.ne, b.e, b.ne, n, wa, wb), ne = 0;
    for (int i = 0; i < k; i++) {
        uint32_t w = (uint32_t)wa[i] + wb[i];
        if (cancel) {
            const node_hot *N = &g->nh[n[i]];
            uint32_t c = 0, d = 0;
            while (c < wb[i] && ndiv(ua, N, &q)) { ua = q; c++; db = ediv(db, N); }
            if (c < wb[i] && UNLIKELY(N->multi) && pgi_partial(g, n[i], ua, wb[i] - c)) return R_RESTART;
            while (d < wa[i] && ndiv(ub, N, &q)) { ub = q; d++; da = ediv(da, N); }
            if (d < wa[i] && UNLIKELY(N->multi) && pgi_partial(g, n[i], ub, wa[i] - d)) return R_RESTART;
            w -= c + d;
        }
        if (w) r->e[ne++] = EP(n[i], w);
    }
    u128 p = (u128)ua * ub, den = (u128)da * db;
    if (p > (u128)(NUM_LIM - (neg ? 0 : 1)) || den >= DEN_LIM) return R_OVF;
    r->num = neg ? (int64_t)(0 - (uint64_t)p) : (int64_t)(uint64_t)p;
    r->den = (uint64_t)den;
    r->ne = ne;
    return R_OK;
}

/* V4: unreduced; on an overflow both operands are reduced and the reducing kernel retried (only an overflow that
 * survives the retry is reported) */
static inline int v4_add(pg_graph *g, view a, view b, int neg, raw *r) {
    int rc = core_add(g, a, b, neg, 0, r);
    if (rc != R_OVF) return rc;
    g->st_v4_retries++;
    raw ra, rb;
    if ((rc = fin(g, a.num, a.den, a.e, a.ne, &ra)) != R_OK) return rc;
    if ((rc = fin(g, b.num, b.den, b.e, b.ne, &rb)) != R_OK) return rc;
    return core_add(g, vraw(&ra), vraw(&rb), neg, 1, r);
}
static inline int v4_mul(pg_graph *g, view a, view b, raw *r) {
    int rc = core_mul(g, a, b, 0, r);
    if (rc != R_OVF) return rc;
    g->st_v4_retries++;
    raw ra, rb;
    if ((rc = fin_k(g, a.num, a.den, a.e, a.ne, &ra, 0)) != R_OK) return rc;
    if ((rc = fin_k(g, b.num, b.den, b.e, b.ne, &rb, 0)) != R_OK) return rc;
    return core_mul(g, vraw(&ra), vraw(&rb), 1, r);
}

#endif
