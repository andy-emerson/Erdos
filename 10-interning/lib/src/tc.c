/* tc.c: TC, the textbook control of spec 10-interning v0.2 (§§8-11): each rational a pair (m, d) of 64-bit values in
 * lowest terms, d >= 1, zero as (0, 1), in one list.  No lookup, no cache, no record of earlier operations.
 *
 * New code (spec R2-10): 08's control (vendor08/ctl.c) is kept unchanged and is no longer a row.  TC follows the
 * pseudocode of §10 as Q32 does, with value comparisons of magnitudes in place of index comparisons (§9), the
 * magnitudes at hand in place of Q32's reads, and a store that appends the pair.  Each known-answer skip is marked
 * [K-n]; the gcds, divisions, multiplications and additions are Q32's, in the same order, call for call (§14.1 item
 * 3).  Every full-width operation goes through the 08 module (sk_internal.h), unchanged. */
#include "tc.h"

#include "r2int.h"

typedef struct tc_class T;

typedef struct tpair {
    sk_int m, d;
} tpair;

struct tc_class {
    tpair *P; /* the pairs */
    uint32_t N, cap;
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    r2_xcounters xcnt;
    r2_xcount *xcur;
#endif
#ifdef SK_AUDIT
    uint32_t mark_N;
    int force_general;
    unsigned koff;
    char msg[200];
#endif
#ifdef SK_PROFILE
    r2_pf *pf;
#endif
};

#ifdef SK_COUNT
static r2_xcount x_sink;
#endif

static inline sk_int gcdp(T *c, sk_int p, sk_int q) {
    PF_TO(c, R2_PF_AG);
    sk_int g = sk_gcd(p, q);
    PF_TO(c, R2_PF_AR);
    (void)c;
    return g;
}

/* ----------------------------------------------------------------------------------------- store (§10.5) */

/* Append n / e (lowest terms, e >= 1).  No zero test and no e = 1 (K-14); the hints of the pseudocode are Q32's. */
static sk_status store(T *c, sk_int n, sk_int e, int kase, sk_handle *out) {
    (void)kase;
    SETCELL(c, c->ctx.op, SK_STORE);
    PF_TO(c, R2_PF_S);
    KC(c, 14, ops, 1);
#ifdef SK_AUDIT
    if (!KON(c, 14) && n == 0) e = 1; /* K-14 off: the v0.1 store's normalization */
    if (n == 0 && e != 1) sk_die("K-14: a zero result without the denominator 1");
#endif
    XC(c, zero_results, n == 0);
    sk_check_limit(c->N, "stored rationals");
    PF_GROW(c, c->N >= c->cap, SK_RESERVE(c->P, c->N, c->cap, 16));
    c->P[c->N].m = n;
    c->P[c->N].d = e;
    *out = c->N++;
    PF_END(c, c->ctx.op, kase);
    return SK_OK;
}

/* ----------------------------------------------------------------------------------------- create, entry */

T *tc_create(void) {
    T *c = sk_xcalloc(1, sizeof *c);
#ifdef SK_COUNT
    c->xcur = &x_sink;
#endif
#ifdef SK_PROFILE
    c->pf = sk_xcalloc(1, sizeof *c->pf);
#endif
    return c;
}

void tc_destroy(T *c) {
    if (!c) return;
    free(c->P);
#ifdef SK_PROFILE
    free(c->pf);
#endif
    free(c);
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_status tc_enter(T *c, sk_int m, sk_int d, sk_handle *out) {
    if (!sk_valid_input(m) || !sk_valid_input(d)) return SK_EINVAL;
    if (d == 0) return SK_EDIVZERO;
    c->ctx.op = SK_PH_ENTRY;
    c->ctx.x = c->ctx.y = SK_NONE;
    SETCELL(c, SK_PH_ENTRY, SK_CORE);
    PF_BEGIN(c, R2_PF_AR);
    if (d < 0) { /* move the sign of d to m; negation is not counted */
        m = -m;
        d = -d;
    }
    sk_int g = gcdp(c, m, d);
    if (g == 1 && KON(c, 1)) { /* [K-1] */
        KC(c, 1, ops, 1);
        KC(c, 1, div, 2);
        return store(c, m, d, R2_ENTRY, out);
    }
    sk_int n = sk_divexact(m, g);
    sk_int e = sk_divexact(d, g);
    return store(c, n, e, R2_ENTRY, out);
}

/* ----------------------------------------------------------------------------------------- operations (§10) */

/* An operand as the pseudocode names it: its magnitudes (a, b for x; c, d for y) and its sign. */
typedef struct opnd {
    sk_int n, e; /* |m| and d */
    bool neg;
} opnd;

static inline opnd operand(const T *c, sk_handle h) {
    tpair p = c->P[h];
    return (opnd){p.m < 0 ? -p.m : p.m, p.d, p.m < 0};
}

static inline sk_status begin(T *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    SETCELL(c, op, SK_CORE);
    return SK_OK;
}

/* The comparisons of §9 on magnitudes, all made, in Q32's order, before a case is chosen. */
typedef struct eq {
    bool den, num, ad, cb, ox, oy;
} eq;

static inline eq compare(T *c, opnd X, opnd Y, int six) {
    eq q;
    q.den = X.e == Y.e;
    q.num = X.n == Y.n;
    q.ad = X.n == Y.e;
    q.cb = Y.n == X.e;
    q.ox = six && X.n == X.e;
    q.oy = six && Y.n == Y.e;
    XC(c, cmp, six ? 6 : 4);
    if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = false;
    (void)c;
    return q;
}

static sk_status addsub(T *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, op, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    if (op == SK_PH_SUB) Y.neg = !Y.neg;
    const sk_int a = X.n, b = X.e, cc = Y.n, d = Y.e;
    PF_TO(c, R2_PF_C);
    eq q = compare(c, X, Y, 0);
    sk_int p, r, t, e, n, g, h;
    if (q.den && q.num && X.neg != Y.neg && KON(c, 2)) { /* case zero: x + (-x) = 0/1  [K-2] */
        XCASE(c, R2_ADD_ZERO);
        KC(c, 2, ops, 1);
        if (q.ad || q.cb) { /* +-1/1 -+ 1/1: without K-2, case cross */
            KC(c, 2, mul, 3);
            KC(c, 2, add, 1);
        } else { /* without K-2, case den: t = 0, gcd(0, d), two divisions */
            KC(c, 2, add, 1);
            KC(c, 2, gcd, 1);
            KC(c, 2, gcd_work, r2_work(0, d));
            KC(c, 2, div, 2);
        }
        PF_TO(c, R2_PF_AR);
        return store(c, 0, 1, R2_ADD_ZERO, out);
    }
    PF_TO(c, R2_PF_AR);
    if (q.ad || q.cb) { /* case cross: gcd(b, d) = 1 */
        XCASE(c, R2_ADD_CROSS);
        if (sk_mul(SGN(c, X.neg, a), d, &p)) OVF("add cross: s_x*a*d");
        if (sk_mul(SGN(c, Y.neg, cc), b, &r)) OVF("add cross: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add cross: s_x*a*d + s_y*c*b");
        if (sk_mul(b, d, &e)) OVF("add cross: b*d");
        return store(c, t, e, R2_ADD_CROSS, out);
    }
    if (q.den) { /* case den: gcd(b, d) = d */
        XCASE(c, R2_ADD_DEN);
        if (sk_add(SGN(c, X.neg, a), SGN(c, Y.neg, cc), &t)) OVF("add den: t = s_x*a + s_y*c");
        h = gcdp(c, t, d);
        if (h == 1 && KON(c, 3)) { /* [K-3] */
            KC(c, 3, ops, 1);
            KC(c, 3, div, 2);
            return store(c, t, d, R2_ADD_DEN, out);
        }
        n = sk_divexact(t, h);
        e = sk_divexact(d, h);
        return store(c, n, e, R2_ADD_DEN, out);
    }
    if (q.num) { /* case num: gcd(a, b*d) = 1 */
        XCASE(c, R2_ADD_NUM);
        g = gcdp(c, b, d);
        if (g == 1 && KON(c, 4)) { /* [K-4] */
            if (sk_add(SGN(c, X.neg, d), SGN(c, Y.neg, b), &t)) OVF("add num: t = s_x*d + s_y*b");
            KC(c, 4, ops, 1);
            KC(c, 4, div, 4);
            KC(c, 4, gcd, 1);
            KC(c, 4, gcd_work, r2_work(t, 1));
            if (sk_mul(a, t, &n)) OVF("add num: a*t");
            if (sk_mul(b, d, &e)) OVF("add num: b*d");
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int bp = sk_divexact(b, g), dp = sk_divexact(d, g);
        if (sk_add(SGN(c, X.neg, dp), SGN(c, Y.neg, bp), &t)) OVF("add num: t = s_x*d' + s_y*b'");
        h = gcdp(c, t, g);
        if (h == 1 && KON(c, 5)) { /* [K-5] */
            KC(c, 5, ops, 1);
            KC(c, 5, div, 2);
            if (sk_mul(a, t, &n)) OVF("add num: a*t");
            if (sk_mul(bp, d, &e)) OVF("add num: b'*d");
            return store(c, n, e, R2_ADD_NUM, out);
        }
        sk_int tq = sk_divexact(t, h), dq = sk_divexact(d, h);
        if (sk_mul(a, tq, &n)) OVF("add num: a*(t/h)");
        if (sk_mul(bp, dq, &e)) OVF("add num: b'*(d/h)");
        return store(c, n, e, R2_ADD_NUM, out);
    }
    XCASE(c, R2_ADD_GEN); /* case general */
    g = gcdp(c, b, d);
    if (g == 1 && KON(c, 6)) { /* [K-6] the direct form is in lowest terms */
        if (sk_mul(SGN(c, X.neg, a), d, &p)) OVF("add general: s_x*a*d");
        if (sk_mul(SGN(c, Y.neg, cc), b, &r)) OVF("add general: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add general: s_x*a*d + s_y*c*b");
        if (sk_mul(b, d, &e)) OVF("add general: b*d");
        KC(c, 6, ops, 1);
        KC(c, 6, div, 4);
        KC(c, 6, gcd, 1);
        KC(c, 6, gcd_work, r2_work(t, 1));
        return store(c, t, e, R2_ADD_GEN, out);
    }
    sk_int bp = sk_divexact(b, g), dp = sk_divexact(d, g);
    if (sk_mul(SGN(c, X.neg, a), dp, &p)) OVF("add general: s_x*a*d'");
    if (sk_mul(SGN(c, Y.neg, cc), bp, &r)) OVF("add general: s_y*c*b'");
    if (sk_add(p, r, &t)) OVF("add general: t = s_x*a*d' + s_y*c*b'");
    h = gcdp(c, t, g);
    if (h == 1 && KON(c, 7)) { /* [K-7] */
        KC(c, 7, ops, 1);
        KC(c, 7, div, 2);
        if (sk_mul(bp, d, &e)) OVF("add general: b'*d");
        return store(c, t, e, R2_ADD_GEN, out);
    }
    n = sk_divexact(t, h);
    sk_int dq = sk_divexact(d, h);
    if (sk_mul(bp, dq, &e)) OVF("add general: b'*(d/h)");
    return store(c, n, e, R2_ADD_GEN, out);
}

sk_status tc_add(T *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status tc_sub(T *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

sk_status tc_mul(T *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    const sk_int a = X.n, b = X.e, cc = Y.n, d = Y.e;
    PF_TO(c, R2_PF_C);
    eq q = compare(c, X, Y, 1);
    bool s = X.neg != Y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    PF_TO(c, R2_PF_AR);
    if (q.ox) { /* case one_x: x = +-1, x*y = s_x*y  [K-15] */
        XCASE(c, R2_MUL_ONE_X);
        KC(c, 15, ops, 1);
        return store(c, SGN(c, s, cc), d, R2_MUL_ONE_X, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x*y = s_y*x  [K-15] */
        XCASE(c, R2_MUL_ONE_Y);
        KC(c, 15, ops, 1);
        return store(c, SGN(c, s, a), b, R2_MUL_ONE_Y, out);
    }
    if (q.ad && q.cb) { /* case both: a = d, c = b  [K-12] */
        XCASE(c, R2_MUL_BOTH);
        KC(c, 12, ops, 1);
        return store(c, SGN(c, s, 1), 1, R2_MUL_BOTH, out);
    }
    if (q.den) { /* case den: no gcd */
        XCASE(c, R2_MUL_DEN);
        if (sk_mul(a, cc, &n)) OVF("multiply den: a*c");
        if (sk_mul(d, d, &e)) OVF("multiply den: d*d");
        return store(c, SGN(c, s, n), e, R2_MUL_DEN, out);
    }
    if (q.num) { /* case num: no gcd */
        XCASE(c, R2_MUL_NUM);
        if (sk_mul(a, a, &n)) OVF("multiply num: a*a");
        if (sk_mul(b, d, &e)) OVF("multiply num: b*d");
        return store(c, SGN(c, s, n), e, R2_MUL_NUM, out);
    }
    if (q.ad) { /* case cross_ad: a cancels */
        XCASE(c, R2_MUL_CROSS_AD);
        h = gcdp(c, cc, b);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, cc), b, R2_MUL_CROSS_AD, out);
        }
        n = sk_divexact(cc, h);
        e = sk_divexact(b, h);
        return store(c, SGN(c, s, n), e, R2_MUL_CROSS_AD, out);
    }
    if (q.cb) { /* case cross_cb: b cancels */
        XCASE(c, R2_MUL_CROSS_CB);
        h = gcdp(c, a, d);
        if (h == 1 && KON(c, 8)) { /* [K-8] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, a), d, R2_MUL_CROSS_CB, out);
        }
        n = sk_divexact(a, h);
        e = sk_divexact(d, h);
        return store(c, SGN(c, s, n), e, R2_MUL_CROSS_CB, out);
    }
    XCASE(c, R2_MUL_GEN); /* case general, on magnitudes with the sign s */
    sk_int g1 = gcdp(c, a, d);
    sk_int g2 = gcdp(c, cc, b);
    sk_int ap, dp, cp, bp;
    if (g1 == 1 && KON(c, 9)) { /* [K-9] */
        ap = a;
        dp = d;
        KC(c, 9, div, 2);
    } else {
        ap = sk_divexact(a, g1);
        dp = sk_divexact(d, g1);
    }
    if (g2 == 1 && KON(c, 9)) { /* [K-9] */
        cp = cc;
        bp = b;
        KC(c, 9, div, 2);
    } else {
        cp = sk_divexact(cc, g2);
        bp = sk_divexact(b, g2);
    }
    KC(c, 9, ops, (g1 == 1 || g2 == 1) && KON(c, 9));
    if (sk_mul(ap, cp, &n)) OVF("multiply general: a'*c'");
    if (sk_mul(bp, dp, &e)) OVF("multiply general: b'*d'");
    return store(c, SGN(c, s, n), e, R2_MUL_GEN, out);
}

sk_status tc_div(T *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_DIV, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    const sk_int a = X.n, b = X.e, cc = Y.n, d = Y.e;
    PF_TO(c, R2_PF_C);
    if (cc == 0) return SK_EDIVZERO; /* m_y = 0, before the comparisons (§10.3 note 1) */
    eq q = compare(c, X, Y, 1);
    bool s = X.neg != Y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    PF_TO(c, R2_PF_AR);
    if (q.ox) { /* case one_x: x = +-1, x/y = s*d/c  [K-15] */
        XCASE(c, R2_DIV_ONE_X);
        KC(c, 15, ops, 1);
        return store(c, SGN(c, s, d), cc, R2_DIV_ONE_X, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x/y = s_y*x  [K-15] */
        XCASE(c, R2_DIV_ONE_Y);
        KC(c, 15, ops, 1);
        return store(c, SGN(c, s, a), b, R2_DIV_ONE_Y, out);
    }
    if (q.num && q.den) { /* case both: x = +-y, y != 0  [K-12] */
        XCASE(c, R2_DIV_BOTH);
        KC(c, 12, ops, 1);
        return store(c, SGN(c, s, 1), 1, R2_DIV_BOTH, out);
    }
    if (q.ad) { /* case cross_ad: a^2 / (b*c), no gcd */
        XCASE(c, R2_DIV_CROSS_AD);
        if (sk_mul(a, a, &n)) OVF("divide cross_ad: a*a");
        if (sk_mul(b, cc, &e)) OVF("divide cross_ad: b*c");
        return store(c, SGN(c, s, n), e, R2_DIV_CROSS_AD, out);
    }
    if (q.cb) { /* case cross_cb: a*d / b^2, no gcd; b = c */
        XCASE(c, R2_DIV_CROSS_CB);
        if (sk_mul(a, d, &n)) OVF("divide cross_cb: a*d");
        if (sk_mul(cc, cc, &e)) OVF("divide cross_cb: c*c");
        return store(c, SGN(c, s, n), e, R2_DIV_CROSS_CB, out);
    }
    if (q.num) { /* case num: a cancels */
        XCASE(c, R2_DIV_NUM);
        h = gcdp(c, d, b);
        if (h == 1 && KON(c, 10)) { /* [K-10] */
            KC(c, 10, ops, 1);
            KC(c, 10, div, 2);
            return store(c, SGN(c, s, d), b, R2_DIV_NUM, out);
        }
        n = sk_divexact(d, h);
        e = sk_divexact(b, h);
        return store(c, SGN(c, s, n), e, R2_DIV_NUM, out);
    }
    if (q.den) { /* case den: b cancels */
        XCASE(c, R2_DIV_DEN);
        h = gcdp(c, a, cc);
        if (h == 1 && KON(c, 10)) { /* [K-10] */
            KC(c, 10, ops, 1);
            KC(c, 10, div, 2);
            return store(c, SGN(c, s, a), cc, R2_DIV_DEN, out);
        }
        n = sk_divexact(a, h);
        e = sk_divexact(cc, h);
        return store(c, SGN(c, s, n), e, R2_DIV_DEN, out);
    }
    XCASE(c, R2_DIV_GEN); /* case general */
    sk_int g1 = gcdp(c, a, cc);
    sk_int g2 = gcdp(c, b, d);
    sk_int ap, cp, bp, dp;
    if (g1 == 1 && KON(c, 11)) { /* [K-11] */
        ap = a;
        cp = cc;
        KC(c, 11, div, 2);
    } else {
        ap = sk_divexact(a, g1);
        cp = sk_divexact(cc, g1);
    }
    if (g2 == 1 && KON(c, 11)) { /* [K-11] */
        bp = b;
        dp = d;
        KC(c, 11, div, 2);
    } else {
        bp = sk_divexact(b, g2);
        dp = sk_divexact(d, g2);
    }
    KC(c, 11, ops, (g1 == 1 || g2 == 1) && KON(c, 11));
    if (sk_mul(ap, dp, &n)) OVF("divide general: a'*d'");
    if (sk_mul(bp, cp, &e)) OVF("divide general: b'*c'");
    return store(c, SGN(c, s, n), e, R2_DIV_GEN, out);
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status tc_read(const T *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    PF_BEGIN(c, R2_PF_F);
    tpair p = c->P[x];
    *m = p.m;
    *d = p.d;
    PF_END(c, SK_PH_READ, R2_READ);
    return SK_OK;
}

uint32_t tc_size(const T *c) { return c->N; }

void tc_memory(const T *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "pairs", (uint64_t)c->N * sizeof(tpair), (uint64_t)c->cap * sizeof(tpair));
    sk_memory_finish(r, c->N);
}

sk_status tc_counters(const T *c, sk_counters *r, r2_xcounters *x) {
#ifdef SK_COUNT
    if (r) *r = c->cnt;
    if (x) *x = c->xcnt;
    return SK_OK;
#else
    (void)c;
    if (r) memset(r, 0, sizeof *r);
    if (x) memset(x, 0, sizeof *x);
    return SK_ENOTBUILT;
#endif
}

sk_status tc_overflow(const T *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

sk_status tc_profile(const T *c, r2_profile *r) {
#ifdef SK_PROFILE
    *r = c->pf->rep;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

void tc_prof_sample(T *c, int on) {
#ifdef SK_PROFILE
    c->pf->on = on;
#else
    (void)c, (void)on;
#endif
}

void tc_prof_outer(T *c, uint64_t cycles) {
#ifdef SK_PROFILE
    c->pf->rep.outer[c->pf->jp][c->pf->kase] += cycles;
#else
    (void)c, (void)cycles;
#endif
}

/* ----------------------------------------------------------------------------------------- audits (§14.2) */

#ifdef SK_AUDIT
static sk_status audit_run(T *c, int full, const char **why) {
    uint32_t from = full ? 0 : c->mark_N;
    const char *fail = NULL;
    uint32_t h;
    for (h = from; h < c->N && !fail; h++) {
        sk_int m = c->P[h].m, d = c->P[h].d;
        if (m == SK_INT_MIN) fail = "pair: numerator -2^63";
        else if (d < 1) fail = "pair: denominator below 1";
        else if (sk_gcd_raw(m, d) != 1) fail = "pair: not in lowest terms";
        else if (m == 0 && d != 1) fail = "pair: zero not stored as (0, 1)";
    }
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "tc: %s (%s audit, pair %u)", fail, full ? "full" : "incremental", h - 1);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status tc_audit(T *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 1, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status tc_audit_changed(T *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 0, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

sk_status tc_t_force_general(T *c, int on) {
#ifdef SK_AUDIT
    c->force_general = on;
    return SK_OK;
#else
    (void)c, (void)on;
    return SK_ENOTBUILT;
#endif
}

sk_status tc_t_known_off(T *c, unsigned mask) {
#ifdef SK_AUDIT
    c->koff = mask & R2_KOFF_ALL;
    return SK_OK;
#else
    (void)c, (void)mask;
    return SK_ENOTBUILT;
#endif
}

sk_status tc_t_corrupt(T *c, sk_handle x, sk_int m, sk_int d) {
#ifdef SK_AUDIT
    if (x >= c->N) return SK_EINVAL;
    c->P[x].m = m;
    c->P[x].d = d;
    return SK_OK;
#else
    (void)c, (void)x, (void)m, (void)d;
    return SK_ENOTBUILT;
#endif
}
