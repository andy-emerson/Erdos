/* array.c: arrays in the array form V3 with V4 inside expressions (spec "Arrays").
 *
 * Layout: the shared denominator D (one holder, HK_DEN, linked like any other), a dense int64 numerator per element
 * scaled to D (0 in exception slots), and the exceptions: ascending indices with their own holders (HK_EXC).
 *
 * Elementwise operations compute the result's D once (lcm for add/sub, product for mul, both for fma), run a
 * checked int64 loop over every slot (round three's R0 loops), then take the graph path for the exceptions of every
 * operand and for the slots whose dense arithmetic overflowed.  A graph-path result is stored dense if its
 * denominator divides the result's D and its scaled numerator fits, else as an exception.  Division, and any
 * operation whose D would reach 2^63, takes the graph path for every element and then chooses D by the default rule.
 *
 * Outside an expression every operation reduces its operands on the graph path (V3) and its result per array at the
 * end (I4).  Inside one (V4) nothing is reduced: graph-path steps run unreduced and reduce-and-retry on overflow,
 * and pg_expr_end reduces each surviving result once.
 */
#include "kernels.h"

#define R_DIVZ (-2)

typedef struct { uint32_t *p; size_t n, cap; } vec32;
static inline void v32_push(pg_graph *g, vec32 *v, uint32_t x) {
    if (v->n == v->cap) {
        v->cap = v->cap ? 2 * v->cap : 256;
        v->p = pgi_realloc(g, v->p, v->cap * 4);
    }
    v->p[v->n++] = x;
}

/* ------------------------------------------------------------------------------------------------- lifetime */

static pg_array *arr_new(pg_graph *g, size_t n) {
    pg_array *A = pgi_calloc(g, sizeof *A);
    A->n = n;
    A->num = pgi_alloc(g, n * 8 + 8);
    A->D = 1;
    if (g->narr == g->arrcap) {
        g->arrcap = g->arrcap ? 2 * g->arrcap : 16;
        g->arrs = pgi_realloc(g, g->arrs, g->arrcap * sizeof *g->arrs);
    }
    A->gpos = g->narr;
    g->arrs[g->narr++] = A;
    return A;
}

static inline void xpush(pg_graph *g, pg_array *A, size_t i, pg_value *h) {
    if (A->nx == A->xcap) {
        A->xcap = A->xcap ? 2 * A->xcap : 16;
        A->xi = pgi_realloc(g, A->xi, A->xcap * 4);
        A->xh = pgi_realloc(g, A->xh, A->xcap * sizeof *A->xh);
    }
    A->xi[A->nx] = (uint32_t)i;
    A->xh[A->nx++] = h;
}

static void pend_remove(pg_graph *g, pg_array *A) {
    pg_array *last = g->pend[--g->npend];
    g->pend[A->ppos] = last;
    last->ppos = A->ppos;
}

static void arr_destroy(pg_graph *g, pg_array *A) {
    for (size_t j = A->nx; j-- > 0;) pgi_holder_release(g, A->xh[j]);  /* newest first: removals at list ends */
    if (A->dh) pgi_holder_release(g, A->dh);
    if (A->deferred) pend_remove(g, A);
    pg_array *last = g->arrs[--g->narr];
    g->arrs[A->gpos] = last;
    last->gpos = A->gpos;
    pgi_free(g, A->num);
    pgi_free(g, A->xi);
    pgi_free(g, A->xh);
    pgi_free(g, A);
}

void pgi_array_free_all(pg_graph *g) {  /* graph destruction: memory only, the graph goes too */
    for (size_t k = 0; k < g->narr; k++) {
        pg_array *A = g->arrs[k];
        pgi_free(g, A->num);
        pgi_free(g, A->xi);
        pgi_free(g, A->xh);
        pgi_free(g, A);
    }
    g->narr = 0;
}

void pg_array_release(pg_graph *g, pg_array *a) {
    if (!a) return;
    arr_destroy(g, a);
    DEBUG_AUDIT(g);
}

size_t pg_array_len(const pg_array *a) { return a->n; }

void pg_array_info_get(const pg_array *a, pg_array_info *info) {
    info->n = a->n;
    info->D = (int64_t)a->D;
    info->exceptions = a->nx;
    info->deferred = a->deferred;
}

/* ------------------------------------------------------------------------------------------------- choosing D */

typedef struct { uint64_t D; int ne; uint32_t e[MAXE]; } dplan;

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static inline size_t hsh64(uint64_t x) {
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
    return (size_t)x;
}

/* The provisional M1 default over the holders hs[0..m) (cnt[k]: elements with that holder's denominator, or one
 * each when cnt is NULL), plus nint further elements with denominator 1: the lcm of the denominators if it is below
 * 2^63, otherwise the most frequent denominator (the first one seen on a tie). */
static void choose_D(pg_graph *g, pg_value *const *hs, const uint32_t *cnt, size_t m, size_t nint, dplan *P) {
    vec32 t = {0};
    int fits = 1;
    uint64_t L = 1;
    for (size_t k = 0; k < m; k++) {
        const pg_value *h = hs[k];
        const uint32_t *e = g->EPL + h->eo;
        for (int j = 0; j < h->ne; j++) {
            uint32_t id = EN(e[j]), w = EW(e[j]);
            if (!g->wsa[id]) v32_push(g, &t, id);
            if (w > g->wsa[id]) g->wsa[id] = (uint8_t)w;
        }
    }
    qsort(t.p, t.n, 4, cmp_u32);
    for (size_t j = 0; j < t.n && fits; j++)
        if (!mulpow(&L, g->nh[t.p[j]].v, g->wsa[t.p[j]]) || L >= DEN_LIM) fits = 0;
    if (fits) {
        P->D = L;
        P->ne = (int)t.n;
        for (size_t j = 0; j < t.n; j++) P->e[j] = EP(t.p[j], g->wsa[t.p[j]]);
    }
    for (size_t j = 0; j < t.n; j++) g->wsa[t.p[j]] = 0;
    pgi_free(g, t.p);
    if (fits) return;
    /* the most frequent denominator */
    size_t best = SIZE_MAX;
    uint64_t bestc = nint;  /* denominator 1 (D = 1) wins unless another is strictly more frequent */
    if (cnt) {
        for (size_t k = 0; k < m; k++)
            if (cnt[k] > bestc) { bestc = cnt[k]; best = k; }
    } else {
        size_t cap = 16;
        while (cap < 2 * m) cap *= 2;
        uint64_t *hk = pgi_calloc(g, cap * 8);
        uint32_t *hv = pgi_alloc(g, cap * 4), *hc = pgi_alloc(g, cap * 4);
        for (size_t k = 0; k < m; k++) {
            uint64_t d = hs[k]->den;
            size_t i = hsh64(d) & (cap - 1);
            while (hk[i] && hk[i] != d) i = (i + 1) & (cap - 1);
            if (!hk[i]) { hk[i] = d; hv[i] = (uint32_t)k; hc[i] = 0; }
            if (++hc[i] > bestc) {
                bestc = hc[i];
                best = hv[i];
            }
        }
        pgi_free(g, hk); pgi_free(g, hv); pgi_free(g, hc);
    }
    P->D = 1;
    P->ne = 0;
    if (best != SIZE_MAX) {
        const pg_value *h = hs[best];
        P->D = h->den;
        P->ne = h->ne;
        memcpy(P->e, g->EPL + h->eo, (size_t)h->ne * 4);
    }
}

/* ------------------------------------------------------------------------------------------------- reduction (I4) */

/* Lowest terms per array: each exception reduced (only if deferred), then D lowered by the common factor of D and
 * every dense numerator.  The scan stops as soon as that factor is 1, which on most data is within a few elements. */
static void arr_reduce(pg_graph *g, pg_array *A) {
    raw r;
    int rc;
    if (A->deferred) {
        for (size_t j = 0; j < A->nx; j++) {
            pg_value *h = A->xh[j];
            for (;;) {
                view v = hview(g, h);
                rc = fin(g, v.num, v.den, v.e, v.ne, &r);
                if (rc != R_RESTART) break;
                g->st_restarts++;
            }
            if (rc != R_OK) pgi_die("reduction grew a value");
            if (r.den != h->den) {
                A->xh[j] = pgi_holder_from_raw(g, &r, HK_EXC);
                pgi_holder_release(g, h);
            }
        }
        A->deferred = 0;
        pend_remove(g, A);
    }
    if (A->D == 1) return;
    uint64_t t = A->D;
    int s = __builtin_ctzll(t);
    uint64_t mask = (1ull << s) - 1, oi = pgi_inv64(t >> s), ol = UINT64_MAX / (t >> s);
    for (size_t i = 0; i < A->n; i++) {
        uint64_t x = mag64(A->num[i]);
        if (!(x & mask) && (x >> s) * oi <= ol) continue;  /* t | x (zero included) */
        t = pgi_gcd(t, x);
        if (t == 1) return;
        s = __builtin_ctzll(t);
        mask = (1ull << s) - 1;
        oi = pgi_inv64(t >> s);
        ol = UINT64_MAX / (t >> s);
    }
    for (;;) {  /* D / t, with its edges: t cancelled against D's nodes (a split here rewrites D's holder) */
        view d = hview(g, A->dh);
        rc = fin(g, (i128)t, A->D, d.e, d.ne, &r);
        if (rc != R_RESTART) break;
        g->st_restarts++;
    }
    if (rc != R_OK || r.num != 1) pgi_die("array reduction");
    for (size_t i = 0; i < A->n; i++)  /* exact division by t: shift, then multiply by the odd part's inverse */
        A->num[i] = (int64_t)((uint64_t)(A->num[i] >> s) * oi);
    pg_value *old = A->dh;
    A->dh = r.den > 1 ? pgi_holder_from_raw(g, &r, HK_DEN) : NULL;
    A->D = r.den;
    pgi_holder_release(g, old);
}

static void arr_finish(pg_graph *g, pg_array *R, int defer) {
    if (defer) {
        R->deferred = 1;
        if (g->npend == g->pendcap) {
            g->pendcap = g->pendcap ? 2 * g->pendcap : 16;
            g->pend = pgi_realloc(g, g->pend, g->pendcap * sizeof *g->pend);
        }
        R->ppos = g->npend;
        g->pend[g->npend++] = R;
    } else arr_reduce(g, R);
}

/* ------------------------------------------------------------------------------------------------- entry */

pg_status pg_array_from_fracs(pg_graph *g, const int64_t *num, const int64_t *den, size_t n, pg_array **out) {
    if (!g || !out || (n && (!num || !den)) || n >= UINT32_MAX) return PG_INVALID;
    for (size_t i = 0; i < n; i++)
        if (den[i] == 0) return PG_DIVZERO;
    /* pass 1: each element's distinct denominator (by magnitude); each new one entered once (steps 1-4) */
    size_t cap = 64, nd = 0, ndcap = 16;
    uint64_t *hk = pgi_calloc(g, cap * 8), *dm = pgi_alloc(g, ndcap * 8);
    uint32_t *hv = pgi_alloc(g, cap * 4), *cnt = pgi_alloc(g, ndcap * 4), *di = pgi_alloc(g, n * 4 + 4);
    pg_value **hold = pgi_alloc(g, ndcap * sizeof *hold);
    int64_t last = 0;
    uint32_t li = 0;
    for (size_t i = 0; i < n; i++) {
        if (den[i] != last) {
            uint64_t d = mag64(den[i]);
            size_t j = hsh64(d) & (cap - 1);
            while (hk[j] && hk[j] != d) j = (j + 1) & (cap - 1);
            if (!hk[j]) {
                if (nd == ndcap) {
                    ndcap *= 2;
                    dm = pgi_realloc(g, dm, ndcap * 8);
                    cnt = pgi_realloc(g, cnt, ndcap * 4);
                    hold = pgi_realloc(g, hold, ndcap * sizeof *hold);
                }
                dm[nd] = d;
                cnt[nd] = 0;
                hold[nd] = pgi_den_enter(g, d);
                hk[j] = d;
                hv[j] = (uint32_t)nd++;
                if (2 * nd > cap) {  /* grow and re-hash */
                    size_t oc = cap;
                    uint64_t *ok = hk;
                    uint32_t *ov = hv;
                    cap *= 2;
                    hk = pgi_calloc(g, cap * 8);
                    hv = pgi_alloc(g, cap * 4);
                    for (size_t q = 0; q < oc; q++)
                        if (ok[q]) {
                            size_t t = hsh64(ok[q]) & (cap - 1);
                            while (hk[t]) t = (t + 1) & (cap - 1);
                            hk[t] = ok[q];
                            hv[t] = ov[q];
                        }
                    pgi_free(g, ok);
                    pgi_free(g, ov);
                    j = hsh64(d) & (cap - 1);
                    while (hk[j] != d) j = (j + 1) & (cap - 1);
                }
            }
            li = hv[j];
            last = den[i];
        }
        di[i] = li;
        cnt[li]++;
    }
    pgi_free(g, hk);
    pgi_free(g, hv);
    /* D, and each distinct denominator's scale D / d (0 if d does not divide D) */
    dplan P;
    choose_D(g, hold, cnt, nd, 0, &P);
    pg_array *A = arr_new(g, n);
    A->D = P.D;
    A->dh = P.D > 1 ? pgi_holder_new(g, 0, P.D, P.e, P.ne, HK_DEN) : NULL;
    int64_t *sc = pgi_alloc(g, nd * 8 + 8);
    for (size_t k = 0; k < nd; k++) sc[k] = P.D % dm[k] == 0 ? (int64_t)(P.D / dm[k]) : 0;
    /* pass 2: dense where the denominator divides D and the scaled numerator fits; else an exception, reduced */
    pg_status st = PG_OK;
    for (size_t i = 0; i < n; i++) {
        uint32_t k = di[i];
        int64_t s = sc[k], x;
        if (LIKELY(den[i] > 0)) {
            if (s && !__builtin_mul_overflow(num[i], s, &x)) { A->num[i] = x; continue; }
        }
        i128 t = den[i] < 0 ? -(i128)num[i] : (i128)num[i];
        if (s && t >= INT64_MIN && t <= INT64_MAX && !__builtin_mul_overflow((int64_t)t, s, &x)) {
            A->num[i] = x;
            continue;
        }
        A->num[i] = 0;
        raw r;
        int rc;
        for (;;) {
            view v = hview(g, hold[k]);
            rc = fin(g, t, dm[k], v.e, v.ne, &r);
            if (rc != R_RESTART) break;
            g->st_restarts++;
        }
        if (rc == R_OVF) { st = PG_OVERFLOW; break; }
        xpush(g, A, i, pgi_holder_from_raw(g, &r, HK_EXC));
    }
    for (size_t k = nd; k-- > 0;) pgi_holder_release(g, hold[k]);
    pgi_free(g, sc); pgi_free(g, dm); pgi_free(g, cnt); pgi_free(g, di); pgi_free(g, hold);
    if (st != PG_OK) { arr_destroy(g, A); return st; }
    arr_reduce(g, A);
    *out = A;
    DEBUG_AUDIT(g);
    return PG_OK;
}

pg_status pg_array_from_values(pg_graph *g, pg_value *const *vals, size_t n, pg_array **out) {
    if (!g || !out || (n && !vals) || n >= UINT32_MAX) return PG_INVALID;
    for (size_t i = 0; i < n; i++)
        if (!vals[i] || vals[i]->kind != HK_VALUE) return PG_INVALID;
    dplan P;
    choose_D(g, vals, NULL, n, 0, &P);
    pg_array *A = arr_new(g, n);
    A->D = P.D;
    A->dh = P.D > 1 ? pgi_holder_new(g, 0, P.D, P.e, P.ne, HK_DEN) : NULL;
    for (size_t i = 0; i < n; i++) {
        const pg_value *v = vals[i];
        int64_t x;
        if (P.D % v->den == 0 && !__builtin_mul_overflow(v->num, (int64_t)(P.D / v->den), &x)) {
            A->num[i] = x;
            continue;
        }
        A->num[i] = 0;
        xpush(g, A, i, pgi_holder_new(g, v->num, v->den, v->ne ? g->EPL + v->eo : NULL, v->ne, HK_EXC));
    }
    arr_reduce(g, A);
    *out = A;
    DEBUG_AUDIT(g);
    return PG_OK;
}

/* ------------------------------------------------------------------------------------------------- elements */

static inline view dview(const pg_graph *g, const pg_array *A) {  /* D as an operand (numerator 0) */
    if (!A->dh) return (view){0, 1, 0, NULL};
    return (view){0, A->D, A->dh->ne, g->EPL + A->dh->eo};
}

/* dense element i: as its own reduced value (red), or as n / D unreduced */
static inline int dense_view(pg_graph *g, const pg_array *A, size_t i, int red, raw *t, view *v) {
    if (!A->dh) {
        *v = (view){A->num[i], 1, 0, NULL};
        if (red) setzero(t), t->num = A->num[i];
        return R_OK;
    }
    view d = hview(g, A->dh);
    if (!red) { *v = (view){A->num[i], A->D, d.ne, d.e}; return R_OK; }
    int rc = fin(g, A->num[i], A->D, d.e, d.ne, t);
    if (rc == R_OK) *v = vraw(t);
    return rc;
}

static ptrdiff_t xfind(const pg_array *A, size_t i) {  /* the exception at index i, or -1 */
    size_t lo = 0, hi = A->nx;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (A->xi[mid] < i) lo = mid + 1;
        else hi = mid;
    }
    return lo < A->nx && A->xi[lo] == i ? (ptrdiff_t)lo : -1;
}

/* element i in lowest terms (x: its exception holder, or NULL when dense) */
static void elem_reduced(pg_graph *g, const pg_array *A, size_t i, pg_value *x, raw *r) {
    int rc;
    for (;;) {
        if (x) {
            if (!A->deferred) {
                r->num = x->num;
                r->den = x->den;
                r->ne = x->ne;
                memcpy(r->e, g->EPL + x->eo, (size_t)x->ne * 4);
                return;
            }
            view v = hview(g, x);
            rc = fin(g, v.num, v.den, v.e, v.ne, r);
        } else {
            view v;
            rc = dense_view(g, A, i, 1, r, &v);
        }
        if (rc != R_RESTART) break;
        g->st_restarts++;
    }
    if (rc != R_OK) pgi_die("reduction grew a value");
}

pg_status pg_array_get(pg_graph *g, const pg_array *a, size_t i, pg_value **out) {
    if (!g || !a || !out || i >= a->n) return PG_INVALID;
    ptrdiff_t j = xfind(a, i);
    raw r;
    elem_reduced(g, a, i, j >= 0 ? a->xh[j] : NULL, &r);
    *out = pgi_holder_from_raw(g, &r, HK_VALUE);
    DEBUG_AUDIT(g);
    return PG_OK;
}

pg_status pg_array_get_frac(pg_graph *g, const pg_array *a, size_t i, int64_t *num, int64_t *den) {
    if (!g || !a || i >= a->n) return PG_INVALID;
    ptrdiff_t j = xfind(a, i);
    raw r;
    elem_reduced(g, a, i, j >= 0 ? a->xh[j] : NULL, &r);
    if (num) *num = r.num;
    if (den) *den = (int64_t)r.den;
    return PG_OK;
}

pg_status pg_array_to_fracs(pg_graph *g, const pg_array *a, int64_t *num, int64_t *den) {
    if (!g || !a || (a->n && (!num || !den))) return PG_INVALID;
    size_t j = 0;
    raw r;
    for (size_t i = 0; i < a->n; i++) {
        pg_value *x = NULL;
        if (j < a->nx && a->xi[j] == i) x = a->xh[j++];
        elem_reduced(g, a, i, x, &r);
        num[i] = r.num;
        den[i] = (int64_t)r.den;
    }
    return PG_OK;
}

/* ------------------------------------------------------------------------------------------------- elementwise */

enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_FMA };

/* division's inverses: |numerator| -> its entered denominator (a transient holder), for one operation */
typedef struct { uint64_t *k; pg_value **v; size_t cap, n; } invtab;

static pg_value *inv_get(const invtab *T, uint64_t m) {
    if (!T->cap) return NULL;
    size_t i = hsh64(m) & (T->cap - 1);
    while (T->k[i]) {
        if (T->k[i] == m) return T->v[i];
        i = (i + 1) & (T->cap - 1);
    }
    return NULL;
}
static void inv_put(pg_graph *g, invtab *T, uint64_t m, pg_value *h) {
    if (2 * (T->n + 1) > T->cap) {
        invtab O = *T;
        T->cap = T->cap ? 2 * T->cap : 64;
        T->k = pgi_calloc(g, T->cap * 8);
        T->v = pgi_alloc(g, T->cap * sizeof *T->v);
        for (size_t i = 0; i < O.cap; i++)
            if (O.k[i]) {
                size_t j = hsh64(O.k[i]) & (T->cap - 1);
                while (T->k[j]) j = (j + 1) & (T->cap - 1);
                T->k[j] = O.k[i];
                T->v[j] = O.v[i];
            }
        pgi_free(g, O.k);
        pgi_free(g, O.v);
    }
    size_t i = hsh64(m) & (T->cap - 1);
    while (T->k[i]) i = (i + 1) & (T->cap - 1);
    T->k[i] = m;
    T->v[i] = h;
    T->n++;
}
static void inv_free(pg_graph *g, invtab *T) {
    for (size_t i = 0; i < T->cap; i++)
        if (T->k[i]) pgi_holder_release(g, T->v[i]);
    pgi_free(g, T->k);
    pgi_free(g, T->v);
}

/* The graph path for element i: x* are the operands' exception holders at i (NULL: dense).  R_OK, R_OVF or R_DIVZ;
 * restarts after splits are handled here. */
static int elem_compute(pg_graph *g, int op, int defer, const pg_array *A, const pg_array *B, const pg_array *C,
                        size_t i, pg_value *xa, pg_value *xb, pg_value *xc, invtab *it, raw *r) {
    for (;; g->st_restarts++) {
        raw ta, tb, tc, tt;
        view va, vb, vc = {0, 1, 0, NULL};
        int rc = R_OK;
        if (xa) va = hview(g, xa);
        else rc = dense_view(g, A, i, !defer, &ta, &va);
        if (rc == R_OK) {
            if (xb) vb = hview(g, xb);
            else rc = dense_view(g, B, i, !defer, &tb, &vb);
        }
        if (rc == R_OK && C) {
            if (xc) vc = hview(g, xc);
            else rc = dense_view(g, C, i, !defer, &tc, &vc);
        }
        if (rc == R_RESTART) continue;
        if (rc != R_OK) return rc;
        switch (op) {
        case OP_ADD:
        case OP_SUB:
            rc = defer ? v4_add(g, va, vb, op == OP_SUB, r) : core_add(g, va, vb, op == OP_SUB, 1, r);
            break;
        case OP_MUL:
            rc = defer ? v4_mul(g, va, vb, r) : core_mul(g, va, vb, 1, r);
            break;
        case OP_FMA:
            rc = defer ? v4_mul(g, va, vb, &tt) : core_mul(g, va, vb, 1, &tt);
            if (rc == R_OK) rc = defer ? v4_add(g, vraw(&tt), vc, 0, r) : core_add(g, vraw(&tt), vc, 0, 1, r);
            break;
        default: {  /* OP_DIV: a * (d_b / m_b) */
            if (vb.num == 0) return R_DIVZ;
            uint64_t mb = mag64(vb.num);
            pg_value *th = inv_get(it, mb);
            if (!th) {  /* entering may split nodes: put it in the table, then recompute the views */
                inv_put(g, it, mb, pgi_den_enter(g, mb));
                g->st_restarts--;  /* not a split */
                continue;
            }
            view vt = hview(g, th);
            view vi = {vb.num < 0 ? -(int64_t)vb.den : (int64_t)vb.den, mb, vt.ne, vt.e};
            rc = defer ? v4_mul(g, va, vi, r) : core_mul(g, va, vi, 1, r);
            break;
        }
        }
        if (rc != R_RESTART) return rc;
    }
}

static inline void store(pg_graph *g, pg_array *R, size_t i, const raw *r) {
    if (r->den == R->D) { R->num[i] = r->num; return; }
    if (R->D % r->den == 0) {
        int64_t x;
        if (!__builtin_mul_overflow(r->num, (int64_t)(R->D / r->den), &x)) { R->num[i] = x; return; }
    }
    R->num[i] = 0;
    xpush(g, R, i, pgi_holder_from_raw(g, r, HK_EXC));
}

/* R has D = 1 and every element computed: choose D by the default rule over the results and re-scale */
static void repack(pg_graph *g, pg_array *R) {
    if (!R->nx) return;
    dplan P;
    choose_D(g, R->xh, NULL, R->nx, R->n - R->nx, &P);
    if (P.D == 1) return;
    pg_value *dh = pgi_holder_new(g, 0, P.D, P.e, P.ne, HK_DEN);
    uint32_t *oxi = R->xi;
    pg_value **oxh = R->xh;
    size_t onx = R->nx, j = 0;
    R->xi = NULL;
    R->xh = NULL;
    R->nx = 0;
    R->xcap = 0;
    for (size_t i = 0; i < R->n; i++) {
        int64_t x;
        if (j < onx && oxi[j] == i) {
            pg_value *h = oxh[j++];
            if (P.D % h->den == 0 && !__builtin_mul_overflow(h->num, (int64_t)(P.D / h->den), &x)) {
                R->num[i] = x;
                pgi_holder_release(g, h);
            } else xpush(g, R, i, h);
        } else if (__builtin_mul_overflow(R->num[i], (int64_t)P.D, &x)) {
            xpush(g, R, i, pgi_holder_new(g, R->num[i], 1, NULL, 0, HK_EXC));
            R->num[i] = 0;
        } else R->num[i] = x;
    }
    pgi_free(g, oxi);
    pgi_free(g, oxh);
    R->D = P.D;
    R->dh = dh;
}

static int prod_plan(const view *a, const view *b, dplan *P) {  /* Da * Db with its edges, if below 2^63 */
    u128 d = (u128)a->den * b->den;
    if (d >= DEN_LIM) return 0;
    uint32_t n[AW];
    uint8_t wa[AW], wb[AW];
    int k = merge(a->e, a->ne, b->e, b->ne, n, wa, wb);
    for (int i = 0; i < k; i++) P->e[i] = EP(n[i], wa[i] + wb[i]);
    P->D = (uint64_t)d;
    P->ne = k;
    return 1;
}

static pg_status elementwise(pg_graph *g, int op, const pg_array *A, const pg_array *B, const pg_array *C,
                             pg_array **out) {
    if (!g || !A || !B || !out || (op == OP_FMA && !C)) return PG_INVALID;
    if (A->n != B->n || (C && C->n != A->n)) return PG_INVALID;
    int defer = g->expr_depth > 0;
    size_t n = A->n;
    pg_array *R = arr_new(g, n);
    dplan P;
    int plan = 0, ok = R_OK;
    int64_t ka = 1, kb = 1;
    view a = dview(g, A), b = dview(g, B);
    if (op == OP_ADD || op == OP_SUB) {
        uint64_t ca, cb;
        u128 L;
        uint32_t e[AW];
        int k = lcm_plan(g, &a, &b, &ca, &cb, e, &L);
        if (k >= 0 && L < DEN_LIM) {
            plan = 1;
            P.D = (uint64_t)L;
            P.ne = k;
            memcpy(P.e, e, (size_t)k * 4);
            ka = (int64_t)ca;
            kb = (int64_t)cb;
        }
    } else if (op == OP_MUL) {
        plan = prod_plan(&a, &b, &P);
    } else if (op == OP_FMA) {
        dplan T;
        if (prod_plan(&a, &b, &T)) {
            view t = {0, T.D, T.ne, T.e}, c = dview(g, C);
            uint64_t ct, cc;
            u128 L;
            uint32_t e[AW];
            int k = lcm_plan(g, &t, &c, &ct, &cc, e, &L);
            if (k >= 0 && L < DEN_LIM) {
                plan = 1;
                P.D = (uint64_t)L;
                P.ne = k;
                memcpy(P.e, e, (size_t)k * 4);
                ka = (int64_t)ct;
                kb = (int64_t)cc;
            }
        }
    }
    invtab it = {0};
    raw r;
    if (plan) {
        R->D = P.D;
        R->dh = P.D > 1 ? pgi_holder_new(g, 0, P.D, P.e, P.ne, HK_DEN) : NULL;
        vec32 ov = {0};
        const int64_t *an = A->num, *bn = B->num;
        int64_t *rn = R->num;
        if (op == OP_ADD) {
            for (size_t i = 0; i < n; i++) {
                int64_t x, y, z;
                int o = __builtin_mul_overflow(an[i], ka, &x) | __builtin_mul_overflow(bn[i], kb, &y);
                o |= __builtin_add_overflow(x, y, &z);
                rn[i] = z;
                if (UNLIKELY(o)) v32_push(g, &ov, (uint32_t)i);
            }
        } else if (op == OP_SUB) {
            for (size_t i = 0; i < n; i++) {
                int64_t x, y, z;
                int o = __builtin_mul_overflow(an[i], ka, &x) | __builtin_mul_overflow(bn[i], kb, &y);
                o |= __builtin_sub_overflow(x, y, &z);
                rn[i] = z;
                if (UNLIKELY(o)) v32_push(g, &ov, (uint32_t)i);
            }
        } else if (op == OP_MUL) {
            for (size_t i = 0; i < n; i++) {
                int64_t z;
                int o = __builtin_mul_overflow(an[i], bn[i], &z);
                rn[i] = z;
                if (UNLIKELY(o)) v32_push(g, &ov, (uint32_t)i);
            }
        } else {
            const int64_t *cn = C->num;
            for (size_t i = 0; i < n; i++) {
                int64_t t, x, y, z;
                int o = __builtin_mul_overflow(an[i], bn[i], &t);
                o |= __builtin_mul_overflow(t, ka, &x) | __builtin_mul_overflow(cn[i], kb, &y);
                o |= __builtin_add_overflow(x, y, &z);
                rn[i] = z;
                if (UNLIKELY(o)) v32_push(g, &ov, (uint32_t)i);
            }
        }
        /* the graph path: every operand's exceptions and every overflowed slot, in index order */
        size_t ia = 0, ib = 0, ic = 0, io = 0;
        for (;;) {
            size_t m = SIZE_MAX;
            if (ia < A->nx) m = A->xi[ia];
            if (ib < B->nx && B->xi[ib] < m) m = B->xi[ib];
            if (C && ic < C->nx && C->xi[ic] < m) m = C->xi[ic];
            if (io < ov.n && ov.p[io] < m) m = ov.p[io];
            if (m == SIZE_MAX) break;
            pg_value *xa = ia < A->nx && A->xi[ia] == m ? A->xh[ia++] : NULL;
            pg_value *xb = ib < B->nx && B->xi[ib] == m ? B->xh[ib++] : NULL;
            pg_value *xc = C && ic < C->nx && C->xi[ic] == m ? C->xh[ic++] : NULL;
            if (io < ov.n && ov.p[io] == m) io++;
            ok = elem_compute(g, op, defer, A, B, C, m, xa, xb, xc, &it, &r);
            if (ok != R_OK) break;
            store(g, R, m, &r);
        }
        pgi_free(g, ov.p);
    } else {  /* every element on the graph path, then D by the default rule */
        size_t ia = 0, ib = 0, ic = 0;
        for (size_t i = 0; i < n; i++) {
            pg_value *xa = ia < A->nx && A->xi[ia] == i ? A->xh[ia++] : NULL;
            pg_value *xb = ib < B->nx && B->xi[ib] == i ? B->xh[ib++] : NULL;
            pg_value *xc = C && ic < C->nx && C->xi[ic] == i ? C->xh[ic++] : NULL;
            ok = elem_compute(g, op, defer, A, B, C, i, xa, xb, xc, &it, &r);
            if (ok != R_OK) break;
            store(g, R, i, &r);
        }
        if (ok == R_OK) repack(g, R);
    }
    inv_free(g, &it);
    if (ok != R_OK) {
        arr_destroy(g, R);
        return ok == R_DIVZ ? PG_DIVZERO : PG_OVERFLOW;
    }
    arr_finish(g, R, defer);
    *out = R;
    DEBUG_AUDIT(g);
    return PG_OK;
}

pg_status pg_array_add(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out) {
    return elementwise(g, OP_ADD, a, b, NULL, out);
}
pg_status pg_array_sub(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out) {
    return elementwise(g, OP_SUB, a, b, NULL, out);
}
pg_status pg_array_mul(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out) {
    return elementwise(g, OP_MUL, a, b, NULL, out);
}
pg_status pg_array_div(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out) {
    return elementwise(g, OP_DIV, a, b, NULL, out);
}
pg_status pg_array_fma(pg_graph *g, const pg_array *a, const pg_array *b, const pg_array *c, pg_array **out) {
    return elementwise(g, OP_FMA, a, b, c, out);
}

/* ------------------------------------------------------------------------------------------------- reductions */

/* A wide accumulator: numerator below 2^127, denominator below 2^128, edges pinned (patched by splits). */
typedef struct { i128 num; u128 den; int ne; uint32_t e[AW]; } wacc;

static int wadd_try(const pg_graph *g, wacc *W, i128 xn, u128 xd, const uint32_t *xe, int xne) {
    uint32_t n[2 * AW];
    uint8_t wa[2 * AW], wb[2 * AW];
    int k = merge(W->e, W->ne, xe, xne, n, wa, wb);
    if (k > AW) return 0;  /* more than 32 coprime nodes: beyond 128 bits */
    u128 sa = 1, sb = 1;   /* the scalings of W and of x */
    for (int i = 0; i < k; i++) {
        uint64_t v = g->nh[n[i]].v;
        for (int j = wa[i]; j < wb[i]; j++)
            if (__builtin_mul_overflow(sa, (u128)v, &sa)) return 0;
        for (int j = wb[i]; j < wa[i]; j++)
            if (__builtin_mul_overflow(sb, (u128)v, &sb)) return 0;
    }
    u128 L;
    i128 p, q, s;
    (void)xd;
    if (__builtin_mul_overflow(W->den, sa, &L) || (sa >> 127) || (sb >> 127)) return 0;
    if (__builtin_mul_overflow(W->num, (i128)sa, &p) || __builtin_mul_overflow(xn, (i128)sb, &q) ||
        __builtin_add_overflow(p, q, &s))
        return 0;
    W->num = s;
    W->den = L;
    W->ne = k;
    for (int i = 0; i < k; i++) W->e[i] = EP(n[i], wa[i] > wb[i] ? wa[i] : wb[i]);
    return 1;
}

/* W += xn / xd: unreduced; on an overflow W and x are reduced and the add retried.  R_OK, R_OVF, or R_RESTART (a
 * split: x's edges are stale, the caller recomputes x; W stays consistent) */
static int wadd(pg_graph *g, wacc *W, i128 xn, u128 xd, const uint32_t *xe, int xne) {
    if (xn == 0) return R_OK;
    if (wadd_try(g, W, xn, xd, xe, xne)) return R_OK;
    g->st_v4_retries++;
    int rc = reduce_w(g, &W->num, &W->den, W->e, &W->ne);
    if (rc != R_OK) return rc;
    uint32_t e[AW];
    int ne = xne;
    memcpy(e, xe, (size_t)xne * 4);
    if ((rc = reduce_w(g, &xn, &xd, e, &ne)) != R_OK) return rc;
    return wadd_try(g, W, xn, xd, e, ne) ? R_OK : R_OVF;
}

static pg_status wfinish(pg_graph *g, wacc *W, pg_value **out) {
    int rc;
    while ((rc = reduce_w(g, &W->num, &W->den, W->e, &W->ne)) == R_RESTART) g->st_restarts++;
    if (W->num > INT64_MAX || W->num < INT64_MIN || W->den >= DEN_LIM) return PG_OVERFLOW;
    *out = pgi_holder_new(g, (int64_t)W->num, (uint64_t)W->den, W->e, W->ne, HK_VALUE);
    return PG_OK;
}

pg_status pg_array_sum(pg_graph *g, const pg_array *a, pg_value **out) {
    if (!g || !a || !out) return PG_INVALID;
    wacc W;
    i128 s = 0;
    for (size_t i = 0; i < a->n; i++) s += a->num[i];  /* the dense part: one integer sum over D */
    view d = dview(g, a);
    W.num = s;
    W.den = a->D;
    W.ne = d.ne;
    memcpy(W.e, d.e, (size_t)d.ne * 4);
    if (s == 0) { W.den = 1; W.ne = 0; }
    pgi_pin(g, W.e, &W.ne);
    pg_status st = PG_OK;
    for (size_t j = 0; j < a->nx && st == PG_OK; j++) {  /* the exceptions */
        int rc;
        do {
            view x = hview(g, a->xh[j]);
            rc = wadd(g, &W, x.num, x.den, x.e, x.ne);
        } while (rc == R_RESTART && ++g->st_restarts);
        if (rc == R_OVF) st = PG_OVERFLOW;
    }
    if (st == PG_OK) st = wfinish(g, &W, out);
    pgi_unpin(g, W.e);
    DEBUG_AUDIT(g);
    return st;
}

pg_status pg_array_dot(pg_graph *g, const pg_array *a, const pg_array *b, pg_value **out) {
    if (!g || !a || !b || !out || a->n != b->n) return PG_INVALID;
    int defer = g->expr_depth > 0;
    wacc W = {0, 1, 0, {0}};
    dplan P;
    uint32_t pe[AW];
    int pne = 0;
    view da = dview(g, a), db = dview(g, b);
    u128 pden = (u128)da.den * db.den;
    {
        uint32_t n[AW];
        uint8_t wa[AW], wb[AW];
        int k = merge(da.e, da.ne, db.e, db.ne, n, wa, wb);
        for (int i = 0; i < k; i++) pe[i] = EP(n[i], wa[i] + wb[i]);
        pne = k;
    }
    (void)P;
    pgi_pin(g, W.e, &W.ne);
    pgi_pin(g, pe, &pne);
    pg_status st = PG_OK;
    int rc;
    /* the dense pairs: products summed over Da * Db in 128 bits, flushed into W when the sum would overflow */
    i128 S = 0;
    size_t ia = 0, ib = 0;
    for (size_t i = 0; i < a->n && st == PG_OK; i++) {
        int xa = ia < a->nx && a->xi[ia] == i, xb = ib < b->nx && b->xi[ib] == i;
        ia += xa;
        ib += xb;
        if (xa || xb) continue;
        i128 p = (i128)a->num[i] * b->num[i], t;
        if (UNLIKELY(__builtin_add_overflow(S, p, &t))) {
            while ((rc = wadd(g, &W, S, pden, pe, pne)) == R_RESTART) g->st_restarts++;
            if (rc == R_OVF) st = PG_OVERFLOW;
            t = p;
        }
        S = t;
    }
    if (st == PG_OK) {
        while ((rc = wadd(g, &W, S, pden, pe, pne)) == R_RESTART) g->st_restarts++;
        if (rc == R_OVF) st = PG_OVERFLOW;
    }
    /* the pairs with an exception: one graph multiplication each */
    invtab it = {0};
    ia = ib = 0;
    for (;;) {
        if (st != PG_OK) break;
        size_t m = SIZE_MAX;
        if (ia < a->nx) m = a->xi[ia];
        if (ib < b->nx && b->xi[ib] < m) m = b->xi[ib];
        if (m == SIZE_MAX) break;
        pg_value *xa = ia < a->nx && a->xi[ia] == m ? a->xh[ia++] : NULL;
        pg_value *xb = ib < b->nx && b->xi[ib] == m ? b->xh[ib++] : NULL;
        raw r;
        for (;;) {
            rc = elem_compute(g, OP_MUL, defer, a, b, NULL, m, xa, xb, NULL, &it, &r);
            if (rc != R_OK) break;
            rc = wadd(g, &W, r.num, r.den, r.e, r.ne);
            if (rc != R_RESTART) break;
            g->st_restarts++;
        }
        if (rc != R_OK) st = PG_OVERFLOW;
    }
    inv_free(g, &it);
    if (st == PG_OK) st = wfinish(g, &W, out);
    pgi_unpin(g, pe);
    pgi_unpin(g, W.e);
    DEBUG_AUDIT(g);
    return st;
}

/* ------------------------------------------------------------------------------------------------- expressions */

pg_status pg_expr_begin(pg_graph *g) {
    if (!g) return PG_INVALID;
    g->expr_depth++;
    return PG_OK;
}

pg_status pg_expr_end(pg_graph *g) {
    if (!g || g->expr_depth == 0) return PG_INVALID;
    if (--g->expr_depth == 0)
        while (g->npend) arr_reduce(g, g->pend[g->npend - 1]);  /* the single reduction (removes it from the list) */
    DEBUG_AUDIT(g);
    return PG_OK;
}
