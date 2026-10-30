/* q32.c: Q32, layer 1 with 16-bit index fields, the textbook baseline of spec 10-interning v0.2 (§§7, 9-11).
 *
 * Started from l1.c compiled with L1_Q32 (v0.1's L1-Q32), with everything for L1-opt, Q64 and the lookup gate
 * dropped, and the known-answer skips of §11 and the ±1 operand cases of §9 added.  l1.c is not changed (§16).
 *
 * Three structures (§7): the whole numbers W (one 64-bit word each, distinct), the lookup (src/plk.c built with
 * -DL1_Q32, unchanged, its own translation unit: lib D-25, spec R2-2), and the rationals (two 16-bit fields).
 * Every full-width operation goes through the 08 module (sk_internal.h), unchanged.
 *
 * The pseudocode of §10 is followed case by case; each known-answer skip is marked [K-n].  The rows Q32 and TC (tc.c)
 * make the same gcds, divisions, multiplications and additions in the same order, call for call (§14.1 item 3). */
#define L1_Q32 1 /* plk.h: the 16-bit lookup (symbols l1q_plk_*) */
#include "q32.h"

#include "plk.h"
#include "r2int.h"

typedef struct q32_class Q;

typedef uint16_t qfield;
#define TAG 0x8000u
#define IDX 0x7FFFu
#define CAPACITY 32768u    /* whole numbers at indices 0 to 32,767 (§7 item 3) */
#define NOIDX SK_NONE      /* "no index" / no hint, in 32-bit variables: never stored (§7 item 3) */
#define SAME_I (SK_NONE - 1u) /* the hint j "the numerator's" (K-Q2) */

typedef struct qrat {
    qfield num; /* the tag in the top bit (1 for -), the numerator index in the low 15 bits */
    qfield den; /* the denominator index */
} qrat;

struct q32_class {
    sk_int *W; /* the whole numbers, each in [0, 2^63 - 1] */
    uint32_t nW, capW;
    plookup lk;
    qrat *R; /* the rationals */
    uint32_t N, capR;
    sk_opctx ctx;
    sk_overflow ovf;
#ifdef SK_COUNT
    sk_counters cnt;
    r2_xcounters xcnt;
    r2_xcount *xcur;
    uint32_t opreads; /* K-Q3: whole numbers read by the current operation */
#endif
#ifdef SK_AUDIT
    uint32_t mark_N, mark_W; /* the change list: rationals and whole numbers since the last audit */
    int force_general;       /* §14.3: the shortcuts switch */
    unsigned koff;           /* §14.3: the known-answer switch (R2_KOFF bits) */
    char msg[200];
#endif
#ifdef SK_PROFILE
    r2_pf *pf;
#endif
};

#ifdef SK_COUNT
static r2_xcount x_sink;
#endif

/* Test build only: an index written to a 16-bit field reads back equal to the index (§7 item 3). */
#ifdef SK_AUDIT
#define READBACK(stored, k)                                                                                       \
    do {                                                                                                          \
        if ((uint32_t)(stored) != (uint32_t)(k)) sk_die("Q32: an index did not fit its 16-bit field");           \
    } while (0)
#else
#define READBACK(stored, k) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- reads (K-Q3) */

static inline sk_int rd(Q *c, uint32_t k) {
#ifdef SK_COUNT
    c->opreads++;
    XC(c, reads, 1);
#endif
    return c->W[k];
}

/* Read the whole numbers v[k] at idx[k] for each k in `need` (0 a, 1 b, 2 c, 3 d) not in `have` (already read), each
 * distinct index once (K-Q3). */
static inline void rdset(Q *c, const uint32_t idx[4], unsigned need, unsigned have, sk_int v[4]) {
    PF_TO(c, R2_PF_F);
    for (int k = 0; k < 4; k++) {
        if (!((need >> k) & 1) || ((have >> k) & 1)) continue;
        int m = 0;
        while (m < 4 && !(((have >> m) & 1) && idx[m] == idx[k])) m++;
        v[k] = m < 4 ? v[m] : rd(c, idx[k]);
        have |= 1u << k;
    }
    PF_TO(c, R2_PF_AR);
}

/* a gcd, bracketed alone in the profile (A.gcd) */
static inline sk_int gcdp(Q *c, sk_int p, sk_int q) {
    PF_TO(c, R2_PF_AG);
    sk_int g = sk_gcd(p, q);
    PF_TO(c, R2_PF_AR);
    (void)c;
    return g;
}

/* ----------------------------------------------------------------------------------------- intern, store (§10.5) */

static uint32_t intern(Q *c, sk_int v) {
    uint32_t empty = 0, k = l1q_plk_find(&c->lk, c->W, v, &empty);
    if (k != NOIDX) {
        XC(c, intern_found, 1);
        return k;
    }
    if (c->nW >= CAPACITY) { /* one check per new whole number, in every build (§7 item 3) */
        fprintf(stderr, "Q32: capacity of 32,768 whole numbers exceeded (spec v0.2 §7 item 3)\n");
        sk_die("capacity exceeded");
    }
    PF_GROW(c, c->nW >= c->capW, SK_RESERVE(c->W, c->nW, c->capW, 16));
    k = c->nW++;
    c->W[k] = v;
    PF_GROW(c, 2 * ((uint64_t)k + 1) > c->lk.S, l1q_plk_insert(&c->lk, c->W, k, empty));
    XC(c, intern_appended, 1);
    return k;
}

/* Store a result: its tag `neg`, its numerator's magnitude `mag` and its denominator `e` (each -1 when the operation
 * has not read it, which it does only with the matching hint), and the index hints i and j (NOIDX: none; j may be
 * SAME_I).  The store makes no zero test and assigns no e = 1 (K-14). */
static sk_status store_(Q *c, bool neg, sk_int mag, uint32_t i, sk_int e, uint32_t j, int kase, sk_handle *out) {
    (void)kase;
#ifdef SK_COUNT
    if (kase != R2_ENTRY) XC(c, reads_skipped, 4 - c->opreads); /* K-Q3, in the operation's core */
#endif
    SETCELL(c, c->ctx.op, SK_STORE);
    PF_TO(c, R2_PF_S);
    KC(c, 14, ops, 1);
#ifdef SK_AUDIT
    if (!HINTS(c)) {
        if (i != NOIDX && mag < 0) sk_die("Q32: hints off, but a numerator was not read");
        if (j != NOIDX && j != SAME_I && e < 0) sk_die("Q32: hints off, but a denominator was not read");
        i = NOIDX;
        if (j == SAME_I) e = mag; /* the result is +-1/1 */
        j = NOIDX;
    }
    if (!KON(c, 14) && mag == 0) e = 1; /* K-14 off: the v0.1 store's normalization */
    {
        sk_int m = mag >= 0 ? mag : c->W[i];
        sk_int ee = e >= 0 ? e : (j == SAME_I ? m : c->W[j]);
        if (i != NOIDX && (i >= c->nW || c->W[i] != m)) sk_die("K-Q1: a numerator hint does not hold its number");
        if (j != NOIDX && j != SAME_I && (j >= c->nW || c->W[j] != ee))
            sk_die("K-Q1: a denominator hint does not hold its number");
        if (j == SAME_I && m != 1) sk_die("K-Q2: the hint \"the numerator's\" with a numerator other than 1");
        if (m == 0 && (ee != 1 || neg)) sk_die("K-14: a zero result without the denominator 1 and the tag +");
    }
#endif
    XC(c, zero_results, (mag >= 0 ? mag : c->W[i]) == 0);
    if (i == NOIDX) {
        PF_TO(c, R2_PF_I);
        i = intern(c, mag); /* the numerator first (v0.1 L-4) */
        PF_TO(c, R2_PF_S);
    } else {
        XC(c, kq1[kase], 1);
    }
    if (j == SAME_I) {
        j = i;
        XC(c, kq2[kase], 1);
    } else if (j == NOIDX) {
        PF_TO(c, R2_PF_I);
        j = intern(c, e);
        PF_TO(c, R2_PF_S);
    } else {
        XC(c, kq1[kase], 1);
    }
    sk_check_limit(c->N, "stored rationals");
    PF_GROW(c, c->N >= c->capR, SK_RESERVE(c->R, c->N, c->capR, 16));
    c->R[c->N].num = (qfield)((neg ? TAG : 0) | i);
    c->R[c->N].den = (qfield)j;
    READBACK(c->R[c->N].num & IDX, i);
    READBACK(c->R[c->N].den, j);
    *out = c->N++;
    PF_END(c, c->ctx.op, kase);
    return SK_OK;
}

/* a result n / e whose values the operation holds, with hints */
static inline sk_status store(Q *c, sk_int n, uint32_t i, sk_int e, uint32_t j, int kase, sk_handle *out) {
    return store_(c, n < 0, n < 0 ? -n : n, i, e, j, kase, out);
}

/* ----------------------------------------------------------------------------------------- create, entry */

Q *q32_create(void) {
    Q *c = sk_xcalloc(1, sizeof *c);
    l1q_plk_init(&c->lk);
#ifdef SK_COUNT
    c->xcur = &x_sink;
#endif
#ifdef SK_PROFILE
    c->pf = sk_xcalloc(1, sizeof *c->pf);
#endif
    return c;
}

void q32_destroy(Q *c) {
    if (!c) return;
    free(c->W);
    free(c->lk.slot);
    free(c->R);
#ifdef SK_PROFILE
    free(c->pf);
#endif
    free(c);
}

#define OVF(step) return sk_overflow_at(&c->ovf, &c->ctx, (step))

sk_status q32_enter(Q *c, sk_int m, sk_int d, sk_handle *out) {
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
        return store(c, m, NOIDX, d, NOIDX, R2_ENTRY, out);
    }
    sk_int n = sk_divexact(m, g);
    sk_int e = sk_divexact(d, g);
    return store(c, n, NOIDX, e, NOIDX, R2_ENTRY, out);
}

/* ----------------------------------------------------------------------------------------- operations (§10) */

typedef struct opnd {
    uint32_t i, j; /* numerator and denominator indices */
    bool neg;      /* the tag */
} opnd;

static inline opnd operand(const Q *c, sk_handle h) {
    qrat r = c->R[h];
    return (opnd){r.num & IDX, r.den, (r.num & TAG) != 0};
}

static inline sk_status begin(Q *c, sk_phase op, sk_handle x, sk_handle y) {
    if (x >= c->N || y >= c->N) return SK_EINVAL;
    c->ctx.op = op;
    c->ctx.x = x;
    c->ctx.y = y;
    SETCELL(c, op, SK_CORE);
#ifdef SK_COUNT
    c->opreads = 0;
#endif
    return SK_OK;
}

/* The comparisons of §9, all made, in this order, before a case is chosen: same(b, d), same(a, c), same(a, d),
 * same(c, b), and in multiplication and division same(a, b) and same(c, d) (the +-1 tests). */
typedef struct eq {
    bool den, num, ad, cb, ox, oy;
} eq;

static inline eq compare(Q *c, opnd X, opnd Y, int six) {
    eq q;
    q.den = X.j == Y.j;
    q.num = X.i == Y.i;
    q.ad = X.i == Y.j;
    q.cb = Y.i == X.j;
    q.ox = six && X.i == X.j;
    q.oy = six && Y.i == Y.j;
    XC(c, cmp, six ? 6 : 4);
    if (FORCE_GENERAL(c)) q.den = q.num = q.ad = q.cb = q.ox = q.oy = false;
    (void)c;
    return q;
}

/* add, and subtract as addition of the second operand with its tag flipped (§10.1) */
static sk_status addsub(Q *c, sk_phase op, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, op, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    if (op == SK_PH_SUB) Y.neg = !Y.neg;
    PF_TO(c, R2_PF_C);
    eq q = compare(c, X, Y, 0);
    const uint32_t idx[4] = {X.i, X.j, Y.i, Y.j};
    sk_int v[4] = {0, 0, 0, 0};
    sk_int p, r, t, e, n, g, h;
    if (q.den && q.num && X.neg != Y.neg && KON(c, 2)) { /* case zero: x + (-x) = 0/1  [K-2] */
        XCASE(c, R2_ADD_ZERO);
        KC(c, 2, ops, 1);
        if (q.ad || q.cb) { /* +-1/1 -+ 1/1: without K-2, case cross */
            KC(c, 2, mul, 3);
            KC(c, 2, add, 1);
        } else { /* without K-2, case den: t = 0, gcd(0, d), two divisions (counting builds read d for the work) */
            KC(c, 2, add, 1);
            KC(c, 2, gcd, 1);
            KC(c, 2, gcd_work, r2_work(0, c->W[Y.j]));
            KC(c, 2, div, 2);
        }
        PF_TO(c, R2_PF_AR);
        return store(c, 0, NOIDX, 1, NOIDX, R2_ADD_ZERO, out);
    }
    if (q.ad || q.cb) { /* case cross: gcd(b, d) = 1 */
        XCASE(c, R2_ADD_CROSS);
        rdset(c, idx, 0xF, 0, v);
        if (sk_mul(SGN(c, X.neg, v[0]), v[3], &p)) OVF("add cross: s_x*a*d");
        if (sk_mul(SGN(c, Y.neg, v[2]), v[1], &r)) OVF("add cross: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add cross: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add cross: b*d");
        return store(c, t, NOIDX, e, NOIDX, R2_ADD_CROSS, out);
    }
    if (q.den) { /* case den: gcd(b, d) = d */
        XCASE(c, R2_ADD_DEN);
        rdset(c, idx, 0xD, 0, v); /* a, c, d */
        if (sk_add(SGN(c, X.neg, v[0]), SGN(c, Y.neg, v[2]), &t)) OVF("add den: t = s_x*a + s_y*c");
        h = gcdp(c, t, v[3]);
        if (h == 1 && KON(c, 3)) { /* [K-3], [K-Q1] */
            KC(c, 3, ops, 1);
            KC(c, 3, div, 2);
            return store(c, t, NOIDX, v[3], X.j, R2_ADD_DEN, out);
        }
        n = sk_divexact(t, h);
        e = sk_divexact(v[3], h);
        return store(c, n, NOIDX, e, NOIDX, R2_ADD_DEN, out);
    }
    if (q.num) { /* case num: gcd(a, b*d) = 1 */
        XCASE(c, R2_ADD_NUM);
        rdset(c, idx, 0xB, 0, v); /* a, b, d */
        g = gcdp(c, v[1], v[3]);
        if (g == 1 && KON(c, 4)) { /* [K-4] */
            if (sk_add(SGN(c, X.neg, v[3]), SGN(c, Y.neg, v[1]), &t)) OVF("add num: t = s_x*d + s_y*b");
            KC(c, 4, ops, 1);
            KC(c, 4, div, 4);
            KC(c, 4, gcd, 1);
            KC(c, 4, gcd_work, r2_work(t, 1));
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(v[1], v[3], &e)) OVF("add num: b*d");
            return store(c, n, NOIDX, e, NOIDX, R2_ADD_NUM, out);
        }
        sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
        if (sk_add(SGN(c, X.neg, dp), SGN(c, Y.neg, bp), &t)) OVF("add num: t = s_x*d' + s_y*b'");
        h = gcdp(c, t, g);
        if (h == 1 && KON(c, 5)) { /* [K-5] */
            KC(c, 5, ops, 1);
            KC(c, 5, div, 2);
            if (sk_mul(v[0], t, &n)) OVF("add num: a*t");
            if (sk_mul(bp, v[3], &e)) OVF("add num: b'*d");
            return store(c, n, NOIDX, e, NOIDX, R2_ADD_NUM, out);
        }
        sk_int tq = sk_divexact(t, h), dq = sk_divexact(v[3], h);
        if (sk_mul(v[0], tq, &n)) OVF("add num: a*(t/h)");
        if (sk_mul(bp, dq, &e)) OVF("add num: b'*(d/h)");
        return store(c, n, NOIDX, e, NOIDX, R2_ADD_NUM, out);
    }
    XCASE(c, R2_ADD_GEN); /* case general */
    rdset(c, idx, 0xF, 0, v);
    g = gcdp(c, v[1], v[3]);
    if (g == 1 && KON(c, 6)) { /* [K-6] the direct form is in lowest terms */
        if (sk_mul(SGN(c, X.neg, v[0]), v[3], &p)) OVF("add general: s_x*a*d");
        if (sk_mul(SGN(c, Y.neg, v[2]), v[1], &r)) OVF("add general: s_y*c*b");
        if (sk_add(p, r, &t)) OVF("add general: s_x*a*d + s_y*c*b");
        if (sk_mul(v[1], v[3], &e)) OVF("add general: b*d");
        KC(c, 6, ops, 1);
        KC(c, 6, div, 4);
        KC(c, 6, gcd, 1);
        KC(c, 6, gcd_work, r2_work(t, 1));
        return store(c, t, NOIDX, e, NOIDX, R2_ADD_GEN, out);
    }
    sk_int bp = sk_divexact(v[1], g), dp = sk_divexact(v[3], g);
    if (sk_mul(SGN(c, X.neg, v[0]), dp, &p)) OVF("add general: s_x*a*d'");
    if (sk_mul(SGN(c, Y.neg, v[2]), bp, &r)) OVF("add general: s_y*c*b'");
    if (sk_add(p, r, &t)) OVF("add general: t = s_x*a*d' + s_y*c*b'");
    h = gcdp(c, t, g);
    if (h == 1 && KON(c, 7)) { /* [K-7] */
        KC(c, 7, ops, 1);
        KC(c, 7, div, 2);
        if (sk_mul(bp, v[3], &e)) OVF("add general: b'*d");
        return store(c, t, NOIDX, e, NOIDX, R2_ADD_GEN, out);
    }
    n = sk_divexact(t, h);
    sk_int dq = sk_divexact(v[3], h);
    if (sk_mul(bp, dq, &e)) OVF("add general: b'*(d/h)");
    return store(c, n, NOIDX, e, NOIDX, R2_ADD_GEN, out);
}

sk_status q32_add(Q *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_ADD, x, y, out); }
sk_status q32_sub(Q *c, sk_handle x, sk_handle y, sk_handle *out) { return addsub(c, SK_PH_SUB, x, y, out); }

/* Cases one_x and one_y with the other operand O as the result (§10.2 note 1, §10.3 note 3): O with the sign s, its
 * whole numbers O's [K-15, K-Q1].  flip: the +-1 operand is -1.  When it is +1 the result is O itself, tag included,
 * and none of O's whole numbers is read; when it is -1, O's numerator is read, so that a zero keeps the tag +.  `kn`
 * and `kv` give a whole number already read (its index, or NOIDX), so it is not read again. */
static sk_status one(Q *c, int kase, bool flip, opnd O, uint32_t kn, sk_int kv, sk_handle *out) {
    KC(c, 15, ops, 1);
    if (!flip && HINTS(c) && KON(c, 13)) {
        KC(c, 13, mul, 1); /* sgn(s, |n_O|) with s = O's tag: O's own tag */
        PF_TO(c, R2_PF_AR);
        return store_(c, O.neg, -1, O.i, -1, O.j, kase, out);
    }
    PF_TO(c, R2_PF_F);
    sk_int m = O.i == kn ? kv : rd(c, O.i);
    sk_int e = HINTS(c) ? -1 : (O.j == kn ? kv : rd(c, O.j)); /* test build, hints off: looked up, so read */
    PF_TO(c, R2_PF_AR);
    sk_int n = SGN(c, flip != O.neg, m); /* sgn(s, |n_O|) */
    return store(c, n, O.i, e, O.j, kase, out);
}

sk_status q32_mul(Q *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_MUL, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    PF_TO(c, R2_PF_C);
    eq q = compare(c, X, Y, 1);
    const uint32_t idx[4] = {X.i, X.j, Y.i, Y.j};
    sk_int v[4] = {0, 0, 0, 0};
    bool s = X.neg != Y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    if (q.ox) { /* case one_x: x = +-1, x*y = s_x*y  [K-15] */
        XCASE(c, R2_MUL_ONE_X);
        return one(c, R2_MUL_ONE_X, X.neg, Y, NOIDX, 0, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x*y = s_y*x  [K-15] */
        XCASE(c, R2_MUL_ONE_Y);
        return one(c, R2_MUL_ONE_Y, Y.neg, X, NOIDX, 0, out);
    }
    if (q.ad && q.cb) { /* case both: a = d, c = b  [K-12, K-Q2] */
        XCASE(c, R2_MUL_BOTH);
        KC(c, 12, ops, 1);
        PF_TO(c, R2_PF_AR);
        return store(c, SGN(c, s, 1), NOIDX, 1, SAME_I, R2_MUL_BOTH, out);
    }
    if (q.den) { /* case den: no gcd */
        XCASE(c, R2_MUL_DEN);
        rdset(c, idx, 0xD, 0, v); /* a, c, d */
        if (sk_mul(v[0], v[2], &n)) OVF("multiply den: a*c");
        if (sk_mul(v[3], v[3], &e)) OVF("multiply den: d*d");
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_MUL_DEN, out);
    }
    if (q.num) { /* case num: no gcd */
        XCASE(c, R2_MUL_NUM);
        rdset(c, idx, 0xB, 0, v); /* a, b, d */
        if (sk_mul(v[0], v[0], &n)) OVF("multiply num: a*a");
        if (sk_mul(v[1], v[3], &e)) OVF("multiply num: b*d");
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_MUL_NUM, out);
    }
    if (q.ad) { /* case cross_ad: a cancels */
        XCASE(c, R2_MUL_CROSS_AD);
        rdset(c, idx, 0x6, 0, v); /* b, c */
        h = gcdp(c, v[2], v[1]);
        if (h == 1 && KON(c, 8)) { /* [K-8], [K-Q1] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, v[2]), Y.i, v[1], X.j, R2_MUL_CROSS_AD, out);
        }
        n = sk_divexact(v[2], h);
        e = sk_divexact(v[1], h);
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_MUL_CROSS_AD, out);
    }
    if (q.cb) { /* case cross_cb: b cancels */
        XCASE(c, R2_MUL_CROSS_CB);
        rdset(c, idx, 0x9, 0, v); /* a, d */
        h = gcdp(c, v[0], v[3]);
        if (h == 1 && KON(c, 8)) { /* [K-8], [K-Q1] */
            KC(c, 8, ops, 1);
            KC(c, 8, div, 2);
            return store(c, SGN(c, s, v[0]), X.i, v[3], Y.j, R2_MUL_CROSS_CB, out);
        }
        n = sk_divexact(v[0], h);
        e = sk_divexact(v[3], h);
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_MUL_CROSS_CB, out);
    }
    XCASE(c, R2_MUL_GEN); /* case general, on magnitudes with the sign s */
    rdset(c, idx, 0xF, 0, v);
    sk_int g1 = gcdp(c, v[0], v[3]);
    sk_int g2 = gcdp(c, v[2], v[1]);
    sk_int ap, dp, cp, bp;
    if (g1 == 1 && KON(c, 9)) { /* [K-9] */
        ap = v[0];
        dp = v[3];
        KC(c, 9, div, 2);
    } else {
        ap = sk_divexact(v[0], g1);
        dp = sk_divexact(v[3], g1);
    }
    if (g2 == 1 && KON(c, 9)) { /* [K-9] */
        cp = v[2];
        bp = v[1];
        KC(c, 9, div, 2);
    } else {
        cp = sk_divexact(v[2], g2);
        bp = sk_divexact(v[1], g2);
    }
    KC(c, 9, ops, (g1 == 1 || g2 == 1) && KON(c, 9));
    if (sk_mul(ap, cp, &n)) OVF("multiply general: a'*c'");
    if (sk_mul(bp, dp, &e)) OVF("multiply general: b'*d'");
    return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_MUL_GEN, out);
}

sk_status q32_div(Q *c, sk_handle x, sk_handle y, sk_handle *out) {
    sk_status st = begin(c, SK_PH_DIV, x, y);
    if (st) return st;
    PF_BEGIN(c, R2_PF_F);
    opnd X = operand(c, x), Y = operand(c, y);
    const uint32_t idx[4] = {X.i, X.j, Y.i, Y.j};
    sk_int v[4] = {0, 0, 0, 0};
    v[2] = rd(c, Y.i); /* c, by its index, before the comparisons (v0.1 L-12) */
    PF_TO(c, R2_PF_C);
    if (v[2] == 0) return SK_EDIVZERO;
    eq q = compare(c, X, Y, 1);
    bool s = X.neg != Y.neg; /* s = s_x * s_y */
    sk_int n, e, h;
    if (q.ox) { /* case one_x: x = +-1, x/y = s*d/c, c != 0  [K-15, K-Q1] */
        XCASE(c, R2_DIV_ONE_X);
        KC(c, 15, ops, 1);
        if (HINTS(c) && KON(c, 13)) { /* d >= 1 and c != 0: the tag is s, and d is not read */
            KC(c, 13, mul, 1);
            PF_TO(c, R2_PF_AR);
            return store_(c, s, -1, Y.j, v[2], Y.i, R2_DIV_ONE_X, out);
        }
        PF_TO(c, R2_PF_F);
        v[3] = Y.j == Y.i ? v[2] : rd(c, Y.j); /* test build, hints or K-13 off */
        PF_TO(c, R2_PF_AR);
        return store(c, SGN(c, s, v[3]), Y.j, v[2], Y.i, R2_DIV_ONE_X, out);
    }
    if (q.oy) { /* case one_y: y = +-1, x/y = s_y*x  [K-15] */
        XCASE(c, R2_DIV_ONE_Y);
        return one(c, R2_DIV_ONE_Y, Y.neg, X, Y.i, v[2], out);
    }
    if (q.num && q.den) { /* case both: x = +-y, y != 0  [K-12, K-Q2] */
        XCASE(c, R2_DIV_BOTH);
        KC(c, 12, ops, 1);
        PF_TO(c, R2_PF_AR);
        return store(c, SGN(c, s, 1), NOIDX, 1, SAME_I, R2_DIV_BOTH, out);
    }
    if (q.ad) { /* case cross_ad: a^2 / (b*c), no gcd */
        XCASE(c, R2_DIV_CROSS_AD);
        rdset(c, idx, 0x3, 0x4, v); /* a, b (c read) */
        if (sk_mul(v[0], v[0], &n)) OVF("divide cross_ad: a*a");
        if (sk_mul(v[1], v[2], &e)) OVF("divide cross_ad: b*c");
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_DIV_CROSS_AD, out);
    }
    if (q.cb) { /* case cross_cb: a*d / b^2, no gcd; b = c, already read */
        XCASE(c, R2_DIV_CROSS_CB);
        rdset(c, idx, 0x9, 0x4, v); /* a, d */
        if (sk_mul(v[0], v[3], &n)) OVF("divide cross_cb: a*d");
        if (sk_mul(v[2], v[2], &e)) OVF("divide cross_cb: c*c");
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_DIV_CROSS_CB, out);
    }
    if (q.num) { /* case num: a cancels */
        XCASE(c, R2_DIV_NUM);
        rdset(c, idx, 0xA, 0x4, v); /* b, d */
        h = gcdp(c, v[3], v[1]);
        if (h == 1 && KON(c, 10)) { /* [K-10], [K-Q1] */
            KC(c, 10, ops, 1);
            KC(c, 10, div, 2);
            return store(c, SGN(c, s, v[3]), Y.j, v[1], X.j, R2_DIV_NUM, out);
        }
        n = sk_divexact(v[3], h);
        e = sk_divexact(v[1], h);
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_DIV_NUM, out);
    }
    if (q.den) { /* case den: b cancels */
        XCASE(c, R2_DIV_DEN);
        rdset(c, idx, 0x1, 0x4, v); /* a (c read) */
        h = gcdp(c, v[0], v[2]);
        if (h == 1 && KON(c, 10)) { /* [K-10], [K-Q1] */
            KC(c, 10, ops, 1);
            KC(c, 10, div, 2);
            return store(c, SGN(c, s, v[0]), X.i, v[2], Y.i, R2_DIV_DEN, out);
        }
        n = sk_divexact(v[0], h);
        e = sk_divexact(v[2], h);
        return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_DIV_DEN, out);
    }
    XCASE(c, R2_DIV_GEN); /* case general */
    rdset(c, idx, 0xB, 0x4, v); /* a, b, d (c read) */
    sk_int g1 = gcdp(c, v[0], v[2]);
    sk_int g2 = gcdp(c, v[1], v[3]);
    sk_int ap, cp, bp, dp;
    if (g1 == 1 && KON(c, 11)) { /* [K-11] */
        ap = v[0];
        cp = v[2];
        KC(c, 11, div, 2);
    } else {
        ap = sk_divexact(v[0], g1);
        cp = sk_divexact(v[2], g1);
    }
    if (g2 == 1 && KON(c, 11)) { /* [K-11] */
        bp = v[1];
        dp = v[3];
        KC(c, 11, div, 2);
    } else {
        bp = sk_divexact(v[1], g2);
        dp = sk_divexact(v[3], g2);
    }
    KC(c, 11, ops, (g1 == 1 || g2 == 1) && KON(c, 11));
    if (sk_mul(ap, dp, &n)) OVF("divide general: a'*d'");
    if (sk_mul(bp, cp, &e)) OVF("divide general: b'*c'");
    return store(c, SGN(c, s, n), NOIDX, e, NOIDX, R2_DIV_GEN, out);
}

/* ----------------------------------------------------------------------------------------- read, reports */

sk_status q32_read(const Q *c, sk_handle x, sk_int *m, sk_int *d) {
    if (x >= c->N) return SK_EINVAL;
    PF_BEGIN(c, R2_PF_F);
    qrat r = c->R[x];
    sk_int v = c->W[r.num & IDX];
    *m = (r.num & TAG) ? -v : v;
    *d = c->W[r.den];
    PF_END(c, SK_PH_READ, R2_READ);
    return SK_OK;
}

uint32_t q32_size(const Q *c) { return c->N; }
uint32_t q32_wholes(const Q *c) { return c->nW; }

void q32_memory(const Q *c, sk_memory *r) {
    memset(r, 0, sizeof *r);
    sk_memory_add(r, "whole numbers", (uint64_t)c->nW * sizeof(sk_int), (uint64_t)c->capW * sizeof(sk_int));
    uint64_t lb = (uint64_t)c->lk.S * sizeof c->lk.slot[0]; /* every slot, empty or not (v0.1 L-17) */
    sk_memory_add(r, "lookup", lb, lb);
    sk_memory_add(r, "rationals", (uint64_t)c->N * sizeof(qrat), (uint64_t)c->capR * sizeof(qrat));
    sk_memory_finish(r, c->N);
}

sk_status q32_counters(const Q *c, sk_counters *r, r2_xcounters *x) {
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

sk_status q32_overflow(const Q *c, sk_overflow *r) {
    *r = c->ovf;
    return c->ovf.count ? SK_OK : SK_EINVAL;
}

sk_status q32_profile(const Q *c, r2_profile *r) {
#ifdef SK_PROFILE
    *r = c->pf->rep;
    return SK_OK;
#else
    (void)c;
    memset(r, 0, sizeof *r);
    return SK_ENOTBUILT;
#endif
}

void q32_prof_sample(Q *c, int on) {
#ifdef SK_PROFILE
    c->pf->on = on;
#else
    (void)c, (void)on;
#endif
}

void q32_prof_outer(Q *c, uint64_t cycles) {
#ifdef SK_PROFILE
    c->pf->rep.outer[c->pf->jp][c->pf->kase] += cycles;
#else
    (void)c, (void)cycles;
#endif
}

/* ----------------------------------------------------------------------------------------- audits (§14.2) */

#ifdef SK_AUDIT
static const char *check_rats(const Q *c, uint32_t from) {
    for (uint32_t h = from; h < c->N; h++) {
        uint32_t i = c->R[h].num & IDX, j = c->R[h].den;
        if (i >= c->nW || j >= c->nW) return "rational: a field holds an index not below W";
        sk_int a = c->W[i], b = c->W[j];
        if (b < 1) return "rational: denominator below 1";
        if (sk_gcd_raw(a, b) != 1) return "rational: not in lowest terms";
        if (a == 0 && (b != 1 || (c->R[h].num & TAG))) return "rational: zero not stored as +0/1";
    }
    return NULL;
}

/* whole numbers k in [from, nW): in range, found by the lookup at their index, and used by a rational stored since
 * rfrom (a whole number is appended only by a store, whose rational follows at once) */
static const char *check_wholes(const Q *c, uint32_t from, uint32_t rfrom) {
    uint32_t n = c->nW - from;
    if (!n) return NULL;
    unsigned char *used = sk_xcalloc(n, 1);
    for (uint32_t h = rfrom; h < c->N; h++) {
        uint32_t i = c->R[h].num & IDX, j = c->R[h].den;
        if (i >= from && i < c->nW) used[i - from] = 1;
        if (j >= from && j < c->nW) used[j - from] = 1;
    }
    const char *fail = NULL;
    for (uint32_t k = from; k < c->nW && !fail; k++) {
        if (c->W[k] < 0) fail = "whole number: negative";
        else if (l1q_plk_find(&c->lk, c->W, c->W[k], NULL) != k)
            fail = "whole number: not found at its index by the lookup, or a duplicate";
        else if (!used[k - from]) fail = "whole number: used by no rational";
    }
    free(used);
    return fail;
}

static sk_status audit_run(Q *c, int full, const char **why) {
    sk_count *saved = sk_cur;
    r2_xcount *xsaved = c->xcur;
    sk_cur = &sk_sink; /* the audits change no count of the class */
    c->xcur = &x_sink;
    uint32_t rf = full ? 0 : c->mark_N, wf = full ? 0 : c->mark_W;
    const char *fail = check_rats(c, rf);
    if (!fail) fail = check_wholes(c, wf, rf);
    if (!fail) fail = full ? l1q_plk_audit_full(&c->lk, c->W, c->nW) : l1q_plk_audit_count(&c->lk, c->nW);
    if (!fail && c->nW > CAPACITY) fail = "whole numbers: over the capacity of 32,768";
    sk_cur = saved;
    c->xcur = xsaved;
    if (fail) {
        snprintf(c->msg, sizeof c->msg, "q32: %s (%s audit, from rational %u, whole number %u)", fail,
                 full ? "full" : "incremental", rf, wf);
        if (why) *why = c->msg;
        return SK_EAUDIT;
    }
    c->mark_N = c->N;
    c->mark_W = c->nW;
    if (why) *why = NULL;
    return SK_OK;
}
#endif

sk_status q32_audit(Q *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 1, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

sk_status q32_audit_changed(Q *c, const char **why) {
#ifdef SK_AUDIT
    return audit_run(c, 0, why);
#else
    (void)c;
    if (why) *why = "audits not built";
    return SK_ENOTBUILT;
#endif
}

/* ----------------------------------------------------------------------------------------- test build only */

sk_status q32_t_whole(const Q *c, uint32_t k, sk_int *v) {
#ifdef SK_AUDIT
    if (k >= c->nW) return SK_EINVAL;
    *v = c->W[k];
    return SK_OK;
#else
    (void)c, (void)k, (void)v;
    return SK_ENOTBUILT;
#endif
}

sk_status q32_t_rat(const Q *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j) {
#ifdef SK_AUDIT
    if (x >= c->N) return SK_EINVAL;
    *neg = (c->R[x].num & TAG) != 0;
    *i = c->R[x].num & IDX;
    *j = c->R[x].den;
    return SK_OK;
#else
    (void)c, (void)x, (void)neg, (void)i, (void)j;
    return SK_ENOTBUILT;
#endif
}

sk_status q32_t_force_general(Q *c, int on) {
#ifdef SK_AUDIT
    c->force_general = on;
    return SK_OK;
#else
    (void)c, (void)on;
    return SK_ENOTBUILT;
#endif
}

sk_status q32_t_known_off(Q *c, unsigned mask) {
#ifdef SK_AUDIT
    c->koff = mask & R2_KOFF_ALL;
    return SK_OK;
#else
    (void)c, (void)mask;
    return SK_ENOTBUILT;
#endif
}

sk_status q32_t_corrupt_rat(Q *c, sk_handle x, int neg, uint32_t i, uint32_t j) {
#ifdef SK_AUDIT
    if (x >= c->N) return SK_EINVAL;
    c->R[x].num = (qfield)((neg ? TAG : 0) | (i & IDX));
    c->R[x].den = (qfield)j;
    return SK_OK;
#else
    (void)c, (void)x, (void)neg, (void)i, (void)j;
    return SK_ENOTBUILT;
#endif
}

sk_status q32_t_corrupt_whole(Q *c, uint32_t k, sk_int v) {
#ifdef SK_AUDIT
    if (k >= c->nW) return SK_EINVAL;
    c->W[k] = v;
    return SK_OK;
#else
    (void)c, (void)k, (void)v;
    return SK_ENOTBUILT;
#endif
}
