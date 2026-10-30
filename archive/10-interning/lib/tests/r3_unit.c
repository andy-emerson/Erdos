/* r3_unit.c: unit tests of round 3's rows, TC, Q32-index and Q32-hash (spec v0.3 §12.3: round 2's tests, v0.2 §14.3,
 * for all three rows, both switches included, each Q32 test run for each Q32 row; plus the capacity of both Q32 rows
 * (in Q32-hash the full table), Q32-hash's probing, Q32-index's search, and the three rows against each other: equal
 * values, equal per-call arithmetic, and equal Q32 found/new splits).  From round 2's r2_unit.c (unchanged).  Amended
 * Oct 29, 2026 (v0.3 §17): 0 and 1 stored by create at handles 0 and 1 (every W counts them; slots 0 and 1 never take
 * another value), and the integer cases of every operation, with their gcds, K-skips, hints and overflow steps.  Test
 * build only. */
#define _GNU_SOURCE
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "r3t.h"

/* ----------------------------------------------------------------------------------------- helpers */

static sk_handle ent(const row *R, void *c, sk_int m, sk_int d) {
    sk_handle h = UINT32_MAX;
    sk_status s = R->enter(c, m, d, &h);
    if (s != SK_OK) fprintf(stderr, "  %s enter %lld/%lld: %s\n", R->name, (long long)m, (long long)d, sk_status_name(s));
    CHECK(s == SK_OK);
    return h;
}

static sk_handle opk(const row *R, void *c, int op, sk_handle x, sk_handle y) {
    sk_handle h = UINT32_MAX;
    sk_status s = R->op[op](c, x, y, &h);
    if (s != SK_OK) fprintf(stderr, "  %s %s: %s\n", R->name, sk_phase_name((sk_phase)op), sk_status_name(s));
    CHECK(s == SK_OK);
    return h;
}

static void val(const row *R, void *c, sk_handle h, sk_int m, sk_int d) {
    sk_int a = 0, b = 0;
    CHECK(R->read(c, h, &a, &b) == SK_OK);
    if (a != m || b != d)
        fprintf(stderr, "  %s: value %lld/%lld, expected %lld/%lld\n", R->name, (long long)a, (long long)b,
                (long long)m, (long long)d);
    CHECK(a == m && b == d);
}

static void audits_ok(const row *R, void *c) {
    const char *why = NULL;
    sk_status s = R->audit_changed(c, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit_changed: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
    s = R->audit(c, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
}

/* the counters of one call: before and after */
typedef struct snap {
    sk_counters b;
    r2_xcounters x;
} snap;

static void qrat(const row *R, void *c, sk_handle h, int *neg, uint32_t *i, uint32_t *j) {
    CHECK(R->t_rat(c, h, neg, i, j) == SK_OK);
}
/* the whole number at handle k (an index or a slot); -1 when there is none (an empty slot) */
static sk_int qwhole(const row *R, void *c, uint32_t k) {
    sk_int v = -1;
    R->t_whole(c, k, &v);
    return v;
}
static r3_scount stor(const row *R, void *c) {
    r3_scounters s;
    CHECK(R->storage(c, &s) == SK_OK);
    return ssum(&s);
}
static uint64_t interns(const snap *s) { return XS(s->x, -1, intern_found) + XS(s->x, -1, intern_appended); }

static snap *SN0, *SN1;
static void take(const row *R, void *c, snap *s) { R->counters(c, &s->b, &s->x); }

/* ----------------------------------------------------------------------------------------- the worked example */

/* Paper §5.5 (v0.1 §14.3): the four entries and their indices; the eight operations, in the paper's order, on the
 * original four values, with the case, the gcds made, the result and, in Q32, the indices appended or found. */
static void test_worked_example(void) {
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        sk_handle x0 = ent(R, c, 1, 6), x1 = ent(R, c, -5, 6), x2 = ent(R, c, 5, 7), x3 = ent(R, c, 6, 7);
        const sk_int W0[5] = {0, 1, 6, 5, 7}; /* 0 and 1 stored by create, at 0 and 1 (v0.3 §17.2) */
        if (R->isq) {
            CHECK_EQ(R->wholes(c), 5);
            if (R->isq == 1) /* Q32-index: the indices are the arrival order, after 0 and 1 */
                for (uint32_t k = 0; k < 5; k++) CHECK_EQ(qwhole(R, c, k), W0[k]);
            const struct { sk_handle h; int neg; uint32_t i, j; } E[4] = {{x0, 0, 1, 2}, {x1, 1, 3, 2}, {x2, 0, 3, 4},
                                                                       {x3, 0, 2, 4}};
            for (int k = 0; k < 4; k++) {
                int neg;
                uint32_t i, j;
                qrat(R, c, E[k].h, &neg, &i, &j);
                CHECK_EQ(neg, E[k].neg);
                CHECK_EQ(qwhole(R, c, i), W0[E[k].i]); /* both Q32 rows: the handles hold the values */
                CHECK_EQ(qwhole(R, c, j), W0[E[k].j]);
                if (R->isq == 1) {
                    CHECK_EQ(i, E[k].i);
                    CHECK_EQ(j, E[k].j);
                }
            }
        }
        struct {
            int op;
            sk_handle x, y;
            int kase, gcds;
            sk_int m, d;
            int neg;
            uint32_t i, j, W; /* Q32: stored as; whole numbers after */
        } T[8] = {
            {SK_PH_ADD, x0, x1, R2_ADD_DEN, 1, -2, 3, 1, 5, 6, 7},
            {SK_PH_MUL, x0, x1, R2_MUL_DEN, 0, -5, 36, 1, 3, 7, 8},
            {SK_PH_MUL, x1, x2, R2_MUL_NUM, 0, -25, 42, 1, 8, 9, 10},
            {SK_PH_MUL, x0, x3, R2_MUL_CROSS_CB, 1, 1, 7, 0, 1, 4, 10},
            {SK_PH_DIV, x2, x1, R2_DIV_NUM, 1, -6, 7, 1, 2, 4, 10},
            {SK_PH_DIV, x3, x2, R2_DIV_DEN, 1, 6, 5, 0, 2, 3, 10},
            {SK_PH_DIV, x3, x0, R2_DIV_CROSS_AD, 0, 36, 7, 0, 7, 4, 10},
            {SK_PH_ADD, x1, x3, R2_ADD_CROSS, 0, 1, 42, 0, 1, 9, 10},
        };
        const sk_int W1[10] = {0, 1, 6, 5, 7, 2, 3, 36, 25, 42};
        for (int k = 0; k < 8; k++) {
            take(R, c, SN0);
            sk_handle h = opk(R, c, T[k].op, T[k].x, T[k].y);
            take(R, c, SN1);
            val(R, c, h, T[k].m, T[k].d);
            CHECK_EQ(XCASES(SN1->x, T[k].op, T[k].kase) - XCASES(SN0->x, T[k].op, T[k].kase), 1);
            CHECK_EQ(BS(SN1->b, T[k].op, gcd) - BS(SN0->b, T[k].op, gcd), T[k].gcds);
            if (R->isq) {
                int neg;
                uint32_t i, j;
                qrat(R, c, h, &neg, &i, &j);
                CHECK_EQ(neg, T[k].neg);
                CHECK_EQ(qwhole(R, c, i), W1[T[k].i]);
                CHECK_EQ(qwhole(R, c, j), W1[T[k].j]);
                if (R->isq == 1) {
                    CHECK_EQ(i, T[k].i);
                    CHECK_EQ(j, T[k].j);
                }
                CHECK_EQ(R->wholes(c), T[k].W);
            }
        }
        if (R->isq == 1) /* the indices appended along the way: 2, 3 at 5, 6; 36 at 7; 25, 42 at 8, 9 */
            for (uint32_t k = 0; k < 10; k++) CHECK_EQ(qwhole(R, c, k), W1[k]);
        if (R->isq == 2) { /* Q32-hash: the same ten whole numbers, each at a used slot, 0 and 1 at slots 0 and 1 */
            uint32_t used = 0;
            for (uint32_t s = 0; s < R3_SLOTS; s++) {
                sk_int v = qwhole(R, c, s);
                if (v < 0) continue;
                used++;
                int in = 0;
                for (int k = 0; k < 10; k++) in |= v == W1[k];
                CHECK(in);
            }
            CHECK_EQ(used, 10);
            CHECK_EQ(qwhole(R, c, 0), 0);
            CHECK_EQ(qwhole(R, c, 1), 1);
        }
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- every case */

typedef struct ex {
    int op;
    sk_int xm, xd, ym, yd;
    int kase;
    sk_int m, d;
} ex;

static const ex EXAMPLES[] = {
    {SK_PH_ADD, 1, 6, 1, 6, R2_ADD_DEN, 1, 3},          /* 1/6 + 1/6 = 1/3 */
    {SK_PH_ADD, 7, 6, 7, 10, R2_ADD_NUM, 28, 15},       /* 7/6 + 7/10 = 28/15 */
    {SK_PH_ADD, 2, 3, 5, 2, R2_ADD_CROSS, 19, 6},       /* 2/3 + 5/2 = 19/6 */
    {SK_PH_ADD, 1, 6, 1, 10, R2_ADD_NUM, 4, 15},        /* 1/6 + 1/10, shared numerator 1 (g = 2, h = 2) */
    {SK_PH_ADD, 5, 6, 7, 10, R2_ADD_GEN, 23, 15},       /* general, g = 2, h = 2 */
    {SK_PH_ADD, 1, 6, 3, 4, R2_ADD_GEN, 11, 12},        /* general, g = 2, h = 1 (K-7) */
    {SK_PH_ADD, 1, 6, 1, 3, R2_ADD_NUM, 1, 2},          /* num, g = 3, h = 3 */
    {SK_PH_ADD, 1, 4, 3, 4, R2_ADD_DEN, 1, 1},          /* den, h = 4 */
    {SK_PH_ADD, 1, 6, 5, 6, R2_ADD_DEN, 1, 1},          /* den, h = 6 */
    {SK_PH_ADD, 1, 12, 1, 4, R2_ADD_NUM, 1, 3},         /* num, g = 4, t = 4, h = 4 */
    {SK_PH_ADD, 1, 6, 3, 10, R2_ADD_GEN, 7, 15},        /* general, g = 2, h = 2 */
    {SK_PH_ADD, 2, 3, 1, 5, R2_ADD_GEN, 13, 15},        /* general, g = 1 (K-6) */
    {SK_PH_ADD, 3, 5, 3, 7, R2_ADD_NUM, 36, 35},        /* num, g = 1 (K-4) */
    {SK_PH_ADD, 3, 4, 3, 10, R2_ADD_NUM, 21, 20},       /* num, g = 2, h = 1 (K-5) */
    {SK_PH_ADD, 1, 3, 1, 3, R2_ADD_DEN, 2, 3},          /* den, h = 1 (K-3) */
    {SK_PH_SUB, 7, 6, 7, 10, R2_ADD_NUM, 7, 15},        /* 7/6 - 7/10: shared numerator, opposite tags */
    {SK_PH_SUB, 5, 7, 5, 7, R2_ADD_ZERO, 0, 1},         /* x - x */
    {SK_PH_ADD, 5, 7, -5, 7, R2_ADD_ZERO, 0, 1},        /* x + (-x) */
    {SK_PH_SUB, 1, 1, 1, 1, R2_ADD_ZERO, 0, 1},         /* 1 - 1: zero, not cross (§9) */
    {SK_PH_SUB, 0, 1, 0, 1, R2_ADD_ZERO, 0, 1},         /* 0 - 0: the effective tags differ: zero (§9) */
    {SK_PH_ADD, 0, 1, 0, 1, R2_ADD_ZERO_X, 0, 1},       /* 0 + 0: zero_x (v0.3 §17.3; den in v0.2) */
    {SK_PH_ADD, 0, 1, -5, 7, R2_ADD_ZERO_X, -5, 7},     /* 0 + y = y: no arithmetic */
    {SK_PH_SUB, 0, 1, -5, 7, R2_ADD_ZERO_X, 5, 7},      /* 0 - y = -y */
    {SK_PH_SUB, 0, 1, 3, 1, R2_ADD_ZERO_X, -3, 1},
    {SK_PH_ADD, -5, 7, 0, 1, R2_ADD_ZERO_Y, -5, 7},     /* x + 0 = x */
    {SK_PH_SUB, 4, 1, 0, 1, R2_ADD_ZERO_Y, 4, 1},       /* x - 0 = x */
    {SK_PH_ADD, 2, 1, 3, 1, R2_ADD_INT_BOTH, 5, 1},     /* integers: (a + c)/1, no gcd, no multiplication */
    {SK_PH_SUB, 2, 1, 7, 1, R2_ADD_INT_BOTH, -5, 1},
    {SK_PH_ADD, 2, 1, 3, 5, R2_ADD_INT_X, 13, 5},       /* 2 + 3/5 = (2*5 + 3)/5, no gcd */
    {SK_PH_SUB, 2, 1, 13, 5, R2_ADD_INT_X, -3, 5},
    {SK_PH_ADD, 3, 5, 2, 1, R2_ADD_INT_Y, 13, 5},       /* 3/5 + 2 = (3 + 2*5)/5, no gcd */
    {SK_PH_SUB, 3, 5, 2, 1, R2_ADD_INT_Y, -7, 5},
    {SK_PH_ADD, 5, 7, 7, 1, R2_ADD_INT_Y, 54, 7},       /* c = b: int_y, not cross */
    {SK_PH_ADD, 1, 1, 1, 3, R2_ADD_INT_X, 4, 3},        /* a = c = 1, a = ... : int_x, not cross or num */
    {SK_PH_MUL, 6, 1, 4, 1, R2_MUL_INT_BOTH, 24, 1},    /* integers: (a*c)/1, no gcd */
    {SK_PH_MUL, -6, 1, 4, 1, R2_MUL_INT_BOTH, -24, 1},
    {SK_PH_MUL, 6, 1, 5, 9, R2_MUL_INT_X, 10, 3},       /* 6 * 5/9: gcd(6, 9) = 3 */
    {SK_PH_MUL, 2, 1, 5, 9, R2_MUL_INT_X, 10, 9},       /* gcd(2, 9) = 1 (K-8) */

    {SK_PH_MUL, 5, 9, 6, 1, R2_MUL_INT_Y, 10, 3},       /* 5/9 * 6: gcd(6, 9) = 3 */
    {SK_PH_MUL, 5, 9, 2, 1, R2_MUL_INT_Y, 10, 9},       /* gcd(2, 9) = 1 (K-8) */
    {SK_PH_MUL, 3, 7, 3, 1, R2_MUL_NUM, 9, 7},          /* a = c: num (no gcd) before int_y (one gcd) */
    {SK_PH_MUL, 3, 7, 7, 1, R2_MUL_CROSS_CB, 3, 1},     /* c = b: cross_cb before int_y */
    {SK_PH_MUL, 1, 7, 5, 1, R2_MUL_CROSS_AD, 5, 7},     /* a = d = 1: cross_ad before int_y */
    {SK_PH_DIV, 6, 1, 4, 1, R2_DIV_DEN, 3, 2},          /* integers: den, a/c after gcd(a, c) */
    {SK_PH_DIV, 6, 1, 4, 5, R2_DIV_GEN, 15, 2},         /* an integer dividend: no integer case in division */
    {SK_PH_DIV, 6, 5, 4, 1, R2_DIV_GEN, 3, 10},         /* an integer divisor: likewise (v0.3 §17.3) */
    {SK_PH_DIV, 3, 7, 3, 1, R2_DIV_NUM, 1, 7},          /* a = c: num (0 multiplications) before int_y */
    {SK_PH_MUL, 3, 7, 2, 7, R2_MUL_DEN, 6, 49},         /* 3/7 * 2/7 = 6/49 */
    {SK_PH_MUL, 3, 4, 3, 5, R2_MUL_NUM, 9, 20},         /* 3/4 * 3/5 = 9/20 */
    {SK_PH_MUL, 4, 9, 3, 4, R2_MUL_CROSS_AD, 1, 3},     /* 4/9 * 3/4 = 1/3 */
    {SK_PH_MUL, 3, 4, 4, 9, R2_MUL_CROSS_CB, 1, 3},     /* 3/4 * 4/9 = 1/3 */
    {SK_PH_MUL, 4, 9, 5, 4, R2_MUL_CROSS_AD, 5, 9},     /* cross_ad, h = 1 (K-8) */
    {SK_PH_MUL, 5, 7, 7, 5, R2_MUL_BOTH, 1, 1},         /* a/b * b/a = 1 */
    {SK_PH_MUL, -5, 7, 7, 5, R2_MUL_BOTH, -1, 1},       /* = -1 */
    {SK_PH_MUL, 4, 9, 3, 8, R2_MUL_GEN, 1, 6},          /* 4/9 * 3/8 = 1/6 */
    {SK_PH_MUL, 2, 9, 5, 7, R2_MUL_GEN, 10, 63},        /* general, g_1 = g_2 = 1 (K-9 twice) */
    {SK_PH_MUL, 2, 9, 3, 7, R2_MUL_GEN, 2, 21},         /* general, g_1 = 1, g_2 = 3 */
    {SK_PH_MUL, 1, 1, 5, 7, R2_MUL_ONE_X, 5, 7},        /* 1 * y */
    {SK_PH_MUL, -1, 1, 5, 7, R2_MUL_ONE_X, -5, 7},      /* -1 * y */
    {SK_PH_MUL, 5, 7, -1, 1, R2_MUL_ONE_Y, -5, 7},      /* x * -1 */
    {SK_PH_MUL, -1, 1, 0, 1, R2_MUL_ZERO, 0, 1},        /* -1 * 0: zero (v0.3 §17.3), the tag + */
    {SK_PH_MUL, 5, 7, 0, 1, R2_MUL_ZERO, 0, 1},         /* x * 0: zero, no arithmetic */
    {SK_PH_MUL, 0, 1, -5, 7, R2_MUL_ZERO, 0, 1},        /* 0 * y */
    {SK_PH_MUL, 0, 1, 0, 1, R2_MUL_ZERO, 0, 1},
    {SK_PH_DIV, 0, 1, -5, 7, R2_DIV_ZERO, 0, 1},        /* 0 / y: zero, no arithmetic */
    {SK_PH_DIV, 0, 1, 3, 1, R2_DIV_ZERO, 0, 1},
    {SK_PH_MUL, -1, 1, -1, 1, R2_MUL_ONE_X, 1, 1},      /* -1 * -1 */
    {SK_PH_DIV, 1, 2, 1, 4, R2_DIV_NUM, 2, 1},          /* 1/2 / 1/4 = 2 */
    {SK_PH_DIV, 3, 10, 9, 10, R2_DIV_DEN, 1, 3},        /* 3/10 / 9/10 = 1/3 */
    {SK_PH_DIV, 2, 3, 5, 2, R2_DIV_CROSS_AD, 4, 15},    /* 2/3 / 5/2 = 4/15 */
    {SK_PH_DIV, 2, 3, 3, 5, R2_DIV_CROSS_CB, 10, 9},    /* 2/3 / 3/5 = 10/9 */
    {SK_PH_DIV, 2, 9, -4, 3, R2_DIV_GEN, -1, 6},        /* 2/9 / (-4/3) = -1/6 */
    {SK_PH_DIV, 2, 9, 5, 7, R2_DIV_GEN, 14, 45},        /* general, g_1 = g_2 = 1 (K-11 twice) */
    {SK_PH_DIV, 3, 10, 7, 10, R2_DIV_DEN, 3, 7},        /* den, h = 1 (K-10) */
    {SK_PH_DIV, 5, 7, 5, 7, R2_DIV_BOTH, 1, 1},         /* x / x = 1 */
    {SK_PH_DIV, 5, 7, -5, 7, R2_DIV_BOTH, -1, 1},
    {SK_PH_DIV, 1, 1, 5, 7, R2_DIV_ONE_X, 7, 5},        /* 1 / y: the reciprocal */
    {SK_PH_DIV, -1, 1, 5, 7, R2_DIV_ONE_X, -7, 5},
    {SK_PH_DIV, 1, 1, -5, 7, R2_DIV_ONE_X, -7, 5},      /* the sign is fixed on the numerator */
    {SK_PH_DIV, 5, 7, -1, 1, R2_DIV_ONE_Y, -5, 7},      /* x / -1 */
    {SK_PH_DIV, 0, 1, -1, 1, R2_DIV_ZERO, 0, 1},        /* 0 / -1: zero (v0.3 §17.3), the tag + */
    {SK_PH_DIV, 1, 5, 1, 1, R2_DIV_ONE_Y, 1, 5},        /* a = c = 1 (index of c, read once) */
    {SK_PH_DIV, -1, 1, -1, 1, R2_DIV_ONE_X, 1, 1},
};
#define NEX (sizeof EXAMPLES / sizeof EXAMPLES[0])

/* one example on one row: the value, the case, the comparisons; with a switch (force: shortcuts off; koff: known
 * answers off), the same value */
static void run_example(const row *R, const ex *e, int force, unsigned koff) {
    void *c = R->create();
    R->force_general(c, force);
    R->known_off(c, koff);
    sk_handle x = ent(R, c, e->xm, e->xd), y = ent(R, c, e->ym, e->yd);
    take(R, c, SN0);
    sk_handle h = opk(R, c, e->op, x, y);
    take(R, c, SN1);
    val(R, c, h, e->m, e->d);
    int k = force ? general_of(e->op) : e->kase;
    if (!force && e->kase == R2_ADD_ZERO && (koff & R2_KOFF(2))) k = -1; /* K-2 off: case zero is not taken */
    if (k >= 0) CHECK_EQ(XCASES(SN1->x, e->op, k) - XCASES(SN0->x, e->op, k), 1);
    CHECK_EQ(XS(SN1->x, e->op, cmp) - XS(SN0->x, e->op, cmp), e->op >= SK_PH_MUL ? 10 : 8); /* + integer, zero tests */
    audits_ok(R, c);
    R->destroy(c);
}

static void test_every_case(void) {
    for (size_t k = 0; k < NEX; k++)
        for (int r = 0; r < NROWS; r++) {
            run_example(ROWS[r], &EXAMPLES[k], 0, 0);
            run_example(ROWS[r], &EXAMPLES[k], 1, 0);              /* shortcuts off */
            run_example(ROWS[r], &EXAMPLES[k], 0, R2_KOFF_ALL);    /* known answers off */
            run_example(ROWS[r], &EXAMPLES[k], 1, R2_KOFF_ALL);    /* both */
        }
}

/* every example with every combination of signs, against the exact reference, with the case of §9 */
static void test_signs(void) {
    for (size_t k = 0; k < NEX; k++) {
        const ex *e = &EXAMPLES[k];
        for (int sx = 0; sx < 2; sx++)
            for (int sy = 0; sy < 2; sy++)
                for (int r = 0; r < NROWS; r++) {
                    const row *R = ROWS[r];
                    sk_int xm = sx ? -e->xm : e->xm, ym = sy ? -e->ym : e->ym;
                    void *c = R->create();
                    sk_handle x = ent(R, c, xm, e->xd), y = ent(R, c, ym, e->yd);
                    sk_handle h = opk(R, c, e->op, x, y);
                    ref_q X = ref_make(xm, e->xd), Y = ref_make(ym, e->yd), want = ref_op(e->op, X, Y);
                    val(R, c, h, (sk_int)want.n, (sk_int)want.d);
                    int ex_ = X.n < 0, ey = (Y.n < 0) != (e->op == SK_PH_SUB);
                    int kase = predict_case(e->op, (uint64_t)ref_abs(X.n), (uint64_t)X.d, (uint64_t)ref_abs(Y.n),
                                            (uint64_t)Y.d, ex_, ey);
                    take(R, c, SN1);
                    CHECK_EQ(XCASES(SN1->x, e->op, kase), 1);
                    if (R->isq && want.n == 0) { /* a zero is stored as +0/1 */
                        int neg;
                        uint32_t i, j;
                        qrat(R, c, h, &neg, &i, &j);
                        CHECK_EQ(neg, 0);
                        CHECK_EQ(qwhole(R, c, i), 0);
                        CHECK_EQ(qwhole(R, c, j), 1);
                    }
                    audits_ok(R, c);
                    R->destroy(c);
                }
    }
}

/* ----------------------------------------------------------------------------------------- zero */

static void test_zero(void) {
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        sk_handle z = ent(R, c, 0, 5), y = ent(R, c, -3, 4), one = ent(R, c, 1, 1), z2 = ent(R, c, 0, -7),
                  mone = ent(R, c, -1, 1);
        val(R, c, z, 0, 1);
        val(R, c, z2, 0, 1);
        for (int op = SK_PH_ADD; op <= SK_PH_DIV; op++) {
            ref_q Z = {0, 1}, Y = {-3, 4}, O = {1, 1}, M = {-1, 1};
            sk_handle h = opk(R, c, op, z, y); /* zero first */
            ref_q w = ref_op(op, Z, Y);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            h = opk(R, c, op, z, one);
            w = ref_op(op, Z, O);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            h = opk(R, c, op, z, mone);
            w = ref_op(op, Z, M);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            if (op != SK_PH_DIV) { /* zero second */
                h = opk(R, c, op, y, z);
                w = ref_op(op, Y, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
                h = opk(R, c, op, one, z);
                w = ref_op(op, O, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
                h = opk(R, c, op, mone, z);
                w = ref_op(op, M, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
                h = opk(R, c, op, z, z2);
                w = ref_op(op, Z, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            }
        }
        /* 0/1 / 0/1, y / 0, +-1 / 0: division by zero, nothing stored */
        uint32_t n = R->size(c);
        sk_handle h = 77;
        CHECK_EQ(R->op[SK_PH_DIV](c, z, z2, &h), SK_EDIVZERO);
        CHECK_EQ(R->op[SK_PH_DIV](c, y, z, &h), SK_EDIVZERO);
        CHECK_EQ(R->op[SK_PH_DIV](c, one, z, &h), SK_EDIVZERO);
        CHECK_EQ(R->op[SK_PH_DIV](c, mone, z, &h), SK_EDIVZERO);
        CHECK_EQ(h, 77);
        CHECK_EQ(R->size(c), n);
        /* zero results: y - y and y + (-y), case zero */
        sk_handle my = ent(R, c, 3, 4);
        val(R, c, opk(R, c, SK_PH_ADD, y, my), 0, 1);
        val(R, c, opk(R, c, SK_PH_SUB, my, my), 0, 1);
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- the +-1 operand shortcut */

/* Spec v0.2 §0 decision 6, §9, §10.2 note 1, §10.3 note 3: (+-1) * y, x * (+-1), x / (+-1), (+-1) / y with every
 * combination of signs, a zero other operand, both operands +-1; the case, no arithmetic; in Q32 the result's indices
 * are the operand's, no lookup is made, and the operand's numerator is read only for -1 (K-Q3). */
static void test_one(void) {
    const sk_int Ys[][2] = {{5, 7}, {0, 1}, {1, 1}, {7, 1}, {1, 9}, {12, 35}};
    for (int r = 0; r < NROWS; r++)
        for (int op = SK_PH_MUL; op <= SK_PH_DIV; op++)
            for (int side = 0; side < 2; side++) /* 0: x = +-1; 1: y = +-1 */
                for (int so = 0; so < 2; so++)    /* the sign of the +-1 */
                    for (size_t k = 0; k < sizeof Ys / sizeof Ys[0]; k++)
                        for (int sv = 0; sv < 2; sv++) {
                            const row *R = ROWS[r];
                            sk_int om = so ? -1 : 1, vm = sv ? -Ys[k][0] : Ys[k][0], vd = Ys[k][1];
                            if (vm == 0) continue; /* a zero other operand: case zero (v0.3 §17.3), test_zero */
                            void *c = R->create();
                            sk_handle o = ent(R, c, om, 1), v = ent(R, c, vm, vd);
                            sk_handle x = side ? v : o, y = side ? o : v;
                            uint32_t w0 = R->isq ? R->wholes(c) : 0;
                            take(R, c, SN0);
                            sk_handle h = opk(R, c, op, x, y);
                            take(R, c, SN1);
                            ref_q X = ref_make(side ? vm : om, side ? vd : 1), Y = ref_make(side ? om : vm, side ? 1 : vd);
                            ref_q want = ref_op(op, X, Y);
                            val(R, c, h, (sk_int)want.n, (sk_int)want.d);
                            bool vone = ref_abs(X.n) == X.d && ref_abs(Y.n) == Y.d; /* both +-1: one_x */
                            int kase = op == SK_PH_MUL ? (side == 0 || vone ? R2_MUL_ONE_X : R2_MUL_ONE_Y)
                                                       : (side == 0 || vone ? R2_DIV_ONE_X : R2_DIV_ONE_Y);
                            CHECK_EQ(XCASES(SN1->x, op, kase) - XCASES(SN0->x, op, kase), 1);
                            CHECK_EQ(XK(SN1->x, op, 15, ops) - XK(SN0->x, op, 15, ops), 1);
                            arith a = arith_diff(arith_of(&SN1->b), arith_of(&SN0->b));
                            CHECK(arith_eq(a, (arith){0, 0, 0, 0, 0}));
                            if (R->isq) {
                                CHECK_EQ(R->wholes(c), w0);
                                CHECK_EQ(interns(SN1) - interns(SN0), 0); /* no whole number stored */
                                CHECK_EQ(XS(SN1->x, op, kq1[kase]) - XS(SN0->x, op, kq1[kase]), 2);
                                int neg, n0;
                                uint32_t i, j, i0, j0;
                                qrat(R, c, h, &neg, &i, &j);
                                /* the operand whose whole numbers the result takes */
                                int recip = op == SK_PH_DIV && kase == R2_DIV_ONE_X;
                                sk_handle src = (kase == R2_MUL_ONE_X || kase == R2_DIV_ONE_X) ? y : x;
                                qrat(R, c, src, &n0, &i0, &j0);
                                CHECK_EQ(i, recip ? j0 : i0);
                                CHECK_EQ(j, recip ? i0 : j0);
                                CHECK_EQ(neg, want.n < 0);
                                /* reads (K-Q3): the other operand's numerator only when the +-1 is -1; a division
                                 * also reads c for its zero test, and one_y does not read a again when a = c */
                                uint64_t reads = XS(SN1->x, op, reads) - XS(SN0->x, op, reads);
                                int xn, yn;
                                uint32_t xi, xj, yi, yj;
                                qrat(R, c, x, &xn, &xi, &xj);
                                qrat(R, c, y, &yn, &yi, &yj);
                                bool flip = (kase == R2_MUL_ONE_X || kase == R2_DIV_ONE_X) ? X.n < 0 : Y.n < 0;
                                if (op == SK_PH_MUL) CHECK_EQ(reads, flip ? 1 : 0);
                                else if (kase == R2_DIV_ONE_X) CHECK_EQ(reads, 1);
                                else CHECK_EQ(reads, 1 + (flip && xi != yi));
                            }
                            audits_ok(R, c);
                            R->destroy(c);
                        }
}

/* ----------------------------------------------------------------------------------------- overlaps */

/* Operands for which several comparisons are equal take the case §9 names; and systematically, random operands
 * over small whole numbers (0 and 1 included), the case counter against §9 on magnitudes and signs. */
static void test_overlaps(void) {
    static const ex O[] = {
        {SK_PH_ADD, 1, 1, 1, 1, R2_ADD_INT_BOTH, 2, 1},    /* cross, den, num and integers, equal tags: int_both */
        {SK_PH_ADD, 1, 1, 1, 3, R2_ADD_INT_X, 4, 3},       /* num, cross and x an integer: int_x */
        {SK_PH_ADD, 1, 3, 1, 1, R2_ADD_INT_Y, 4, 3},       /* num, cross and y an integer: int_y */
        {SK_PH_ADD, 3, 5, 3, 5, R2_ADD_DEN, 6, 5},         /* den and num, equal tags: den */
        {SK_PH_MUL, 3, 5, 3, 5, R2_MUL_DEN, 9, 25},        /* den and num: den */
        {SK_PH_MUL, 1, 1, 1, 1, R2_MUL_ONE_X, 1, 1},       /* everything: one_x */
        {SK_PH_MUL, 2, 1, 1, 2, R2_MUL_BOTH, 1, 1},        /* both */
        {SK_PH_MUL, 1, 3, 1, 1, R2_MUL_ONE_Y, 1, 3},       /* num, cross_ad and one_y: one_y */
        {SK_PH_MUL, 3, 1, 5, 3, R2_MUL_CROSS_AD, 5, 1},    /* cross_ad */
        {SK_PH_MUL, 1, 1, 3, 1, R2_MUL_ONE_X, 3, 1},       /* one_x, den and cross_ad: one_x */
        {SK_PH_MUL, 2, 1, 2, 1, R2_MUL_INT_BOTH, 4, 1},    /* den, num and integers: int_both */
        {SK_PH_MUL, 3, 1, 1, 3, R2_MUL_BOTH, 1, 1},        /* both and x an integer: both */
        {SK_PH_DIV, 1, 1, 1, 1, R2_DIV_ONE_X, 1, 1},       /* everything: one_x */
        {SK_PH_DIV, 2, 1, 1, 2, R2_DIV_CROSS_AD, 4, 1},    /* cross_ad and cross_cb: cross_ad */
        {SK_PH_DIV, 1, 3, 1, 1, R2_DIV_ONE_Y, 1, 3},       /* num, cross_ad and one_y: one_y */
        {SK_PH_DIV, 3, 1, 1, 1, R2_DIV_ONE_Y, 3, 1},       /* den, cross_cb and one_y: one_y */
        {SK_PH_DIV, 3, 7, 3, 5, R2_DIV_NUM, 5, 7},         /* num */
        {SK_PH_DIV, 2, 3, 3, 2, R2_DIV_CROSS_AD, 4, 9},    /* cross_ad and cross_cb: b = c, read once */
    };
    for (size_t k = 0; k < sizeof O / sizeof O[0]; k++)
        for (int r = 0; r < NROWS; r++) run_example(ROWS[r], &O[k], 0, 0);
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        for (int t = 0; t < 6000; t++) {
            void *c = R->create();
            sk_int a = (sk_int)rndn(6), b = 1 + (sk_int)rndn(5), cc = (sk_int)rndn(6), d = 1 + (sk_int)rndn(5);
            int op = 1 + (int)rndn(4);
            if (op == SK_PH_DIV && cc == 0) cc = 1;
            sk_int xm = rndn(2) ? -a : a, ym = rndn(2) ? -cc : cc;
            sk_handle x = ent(R, c, xm, b), y = ent(R, c, ym, d);
            ref_q X = ref_make(xm, b), Y = ref_make(ym, d);
            int want = predict_case(op, (uint64_t)ref_abs(X.n), (uint64_t)X.d, (uint64_t)ref_abs(Y.n), (uint64_t)Y.d,
                                    X.n < 0, (Y.n < 0) != (op == SK_PH_SUB));
            sk_handle h = opk(R, c, op, x, y);
            take(R, c, SN1);
            CHECK_EQ(XCASES(SN1->x, op, want), 1);
            ref_q w = ref_op(op, X, Y);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            R->destroy(c);
        }
    }
}

/* ----------------------------------------------------------------------------------------- entry, errors */

static void test_entry(void) {
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        val(R, c, ent(R, c, 3, -6), -1, 2);
        val(R, c, ent(R, c, -3, -6), 1, 2);
        val(R, c, ent(R, c, 12, 18), 2, 3);
        val(R, c, ent(R, c, 0, -9), 0, 1);
        val(R, c, ent(R, c, INT64_MAX, INT64_MAX), 1, 1);
        val(R, c, ent(R, c, -INT64_MAX, 1), -INT64_MAX, 1);
        val(R, c, ent(R, c, 1, -INT64_MAX), -1, INT64_MAX);
        /* K-1: gcd 1, no division; gcd 6, two divisions */
        take(R, c, SN0);
        ent(R, c, 5, 7);
        take(R, c, SN1);
        CHECK_EQ(BS(SN1->b, SK_PH_ENTRY, div_exact) - BS(SN0->b, SK_PH_ENTRY, div_exact), 0);
        CHECK_EQ(XK(SN1->x, SK_PH_ENTRY, 1, div) - XK(SN0->x, SK_PH_ENTRY, 1, div), 2);
        ent(R, c, 12, 18);
        take(R, c, SN0);
        CHECK_EQ(BS(SN0->b, SK_PH_ENTRY, div_exact) - BS(SN1->b, SK_PH_ENTRY, div_exact), 2);
        sk_handle h = 99;
        uint32_t n = R->size(c);
        CHECK_EQ(R->enter(c, 5, 0, &h), SK_EDIVZERO);
        CHECK_EQ(R->enter(c, INT64_MIN, 3, &h), SK_EINVAL);
        CHECK_EQ(R->enter(c, 3, INT64_MIN, &h), SK_EINVAL);
        CHECK_EQ(R->enter(c, INT64_MIN, 0, &h), SK_EINVAL);
        CHECK_EQ(R->size(c), n);
        /* a handle not in the class: invalid argument, before any test for division by zero */
        sk_handle z = ent(R, c, 0, 1);
        n = R->size(c);
        CHECK_EQ(R->op[SK_PH_DIV](c, n, z, &h), SK_EINVAL);
        CHECK_EQ(R->op[SK_PH_DIV](c, z, n + 5, &h), SK_EINVAL);
        for (int op = 1; op <= 4; op++) CHECK_EQ(R->op[op](c, 0, n, &h), SK_EINVAL);
        sk_int m, d;
        CHECK_EQ(R->read(c, n, &m, &d), SK_EINVAL);
        CHECK_EQ(R->size(c), n);
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- overflow */

typedef struct ovf {
    int op;
    sk_int xm, xd, ym, yd;
    const char *step;
} ovf;

/* every named step that can overflow (found by a search over large operands; lib/DECISIONS.md R2-D10); "add num: t =
 * s_x*d' + s_y*b'" cannot (g >= 2 halves both terms), and "add num: a*t" is reached in K-4 and in K-5 */
static const ovf OVF[] = {
    {SK_PH_ADD, 5LL, 9LL, -9LL, 1317624576693539401LL, "add cross: b*d"},
    {SK_PH_SUB, 3037000499LL, 1227133514LL, -3037000500LL, 3037000499LL, "add cross: s_x*a*d + s_y*c*b"},
    {SK_PH_SUB, -4611686018427387905LL, 12LL, 4294967297LL, 4611686018427387905LL, "add cross: s_x*a*d"},
    {SK_PH_SUB, -3037000493LL, 4611686018427387905LL, 6000000000000000004LL, 3037000493LL, "add cross: s_y*c*b"},
    {SK_PH_ADD, -4611686018427387905LL, 4611686018427387907LL, -6148914691236517205LL, 4611686018427387907LL,
     "add den: t = s_x*a + s_y*c"},
    {SK_PH_SUB, 4611686018427387905LL, 3LL, -4611686018427387905LL, 3LL, "add den: t = s_x*a + s_y*c"},
    {SK_PH_SUB, -8LL, 1537228672809129303LL, -1LL, 1840700271LL, "add general: b'*(d/h)"},
    {SK_PH_ADD, -6LL, 1431655765LL, 4294967297LL, 64424509485LL, "add general: b'*d"},
    {SK_PH_SUB, -4LL, 3037000499LL, -5LL, 12884901891LL, "add general: b*d"},
    {SK_PH_ADD, -1LL, 1LL, -4611686018427387903LL, 6148914691236517205LL, "add int_x: s_x*a*d + s_y*c"},
    {SK_PH_SUB, -2000000000000000002LL, 4294967299LL, 1073741824LL, 4294967297LL, "add general: s_x*a*d"},
    {SK_PH_ADD, -2305843009213693951LL, 25769803794LL, 9LL, 10LL, "add general: s_x*a*d'"},
    {SK_PH_ADD, -1500000000000000001LL, 4611686018427387905LL, 10LL, 1LL, "add int_y: s_y*c*b"},
    {SK_PH_ADD, -1LL, 2LL, -4611686018427387903LL, 6148914691236517205LL, "add general: s_x*a*d + s_y*c*b"},
    {SK_PH_ADD, -1500000000000000001LL, 4611686018427387905LL, 10LL, 3LL, "add general: s_y*c*b"},
    {SK_PH_ADD, 3037000493LL, 4294967299LL, 4611686018427387903LL, 15032385536LL, "add general: s_y*c*b'"},
    {SK_PH_SUB, -9LL, 8LL, 2305843009213693951LL, 10LL, "add general: t = s_x*a*d' + s_y*c*b'"},
    {SK_PH_SUB, 1317624576693539401LL, 12884901891LL, 1317624576693539401LL, 6LL, "add num: a*(t/h)"},
    {SK_PH_SUB, -3037000493LL, 8589934598LL, 3037000493LL, 9111001497LL, "add num: a*t"},
    {SK_PH_ADD, 1317624576693539405LL, 6LL, 1317624576693539405LL, 8LL, "add num: a*t"},
    {SK_PH_ADD, -4LL, 38654705673LL, -4LL, 3074457345618258603LL, "add num: b'*(d/h)"},
    {SK_PH_ADD, 15LL, 1317624576693539401LL, 15LL, 4294967299LL, "add num: b'*d"},
    {SK_PH_SUB, 2LL, 3074457345618258603LL, -2LL, 1317624576693539401LL, "add num: b*d"},
    {SK_PH_SUB, -4294967297LL, 9223372036854775804LL, 4294967297LL, 15LL, "add num: t = s_x*d + s_y*b"},
    {SK_PH_DIV, -1000000000000000001LL, 1LL, 4611686018427387909LL, 1000000000000000001LL, "divide cross_ad: a*a"},
    {SK_PH_DIV, -8LL, 3037000493LL, -4611686018427387909LL, 8LL, "divide cross_ad: b*c"},
    {SK_PH_DIV, 341606371735362067LL, 1LL, -1LL, 1099511627777LL, "divide cross_cb: a*d"},
    {SK_PH_DIV, -253083375LL, 8589934594LL, 8589934594LL, 7LL, "divide cross_cb: c*c"},
    {SK_PH_DIV, -1099511627777LL, 4611686018427387909LL, -3000000000000000002LL, 4611686018427387903LL,
     "divide general: a'*d'"},
    {SK_PH_DIV, -2LL, 4611686018427387907LL, -12LL, 2305843009213693951LL, "divide general: b'*c'"},
    {SK_PH_MUL, 14725602168LL, 5LL, 3074457345618258603LL, 5LL, "multiply den: a*c"},
    {SK_PH_MUL, 2LL, 4294967297LL, 35LL, 4294967297LL, "multiply den: d*d"},
    {SK_PH_MUL, 8589934594LL, 15LL, 2305843009213693951LL, 4LL, "multiply general: a'*c'"},
    {SK_PH_MUL, 9LL, 1000000000000000001LL, -3037000499LL, 4294967291LL, "multiply general: b'*d'"},
    {SK_PH_MUL, -4294967297LL, 3LL, -4294967297LL, 8589934598LL, "multiply num: a*a"},
    {SK_PH_MUL, -8LL, 188232082384791343LL, -8LL, 1317624576693539401LL, "multiply num: b*d"},
    /* the integer cases (v0.3 §17.3) */
    {SK_PH_ADD, 4611686018427387905LL, 1LL, 4611686018427387905LL, 1LL, "add int_both: t = s_x*a + s_y*c"},
    {SK_PH_SUB, -4611686018427387905LL, 1LL, 4611686018427387905LL, 1LL, "add int_both: t = s_x*a + s_y*c"},
    {SK_PH_ADD, 4294967297LL, 1LL, 1LL, 4294967297LL, "add int_x: s_x*a*d"},
    {SK_PH_ADD, 3LL, 1LL, 9223372036854775801LL, 4LL, "add int_x: s_x*a*d + s_y*c"},
    {SK_PH_SUB, 1LL, 4294967297LL, -4294967297LL, 1LL, "add int_y: s_y*c*b"},
    {SK_PH_ADD, 9223372036854775801LL, 4LL, 3LL, 1LL, "add int_y: s_x*a + s_y*c*b"},
    {SK_PH_MUL, 4294967297LL, 1LL, -4294967297LL, 1LL, "multiply int_both: a*c"},
    {SK_PH_MUL, 4294967297LL, 1LL, 4294967299LL, 3LL, "multiply int_x: a*c"},
    {SK_PH_MUL, -8589934594LL, 1LL, 4294967299LL, 2LL, "multiply int_x: a'*c"},
    {SK_PH_MUL, 4294967299LL, 3LL, 4294967297LL, 1LL, "multiply int_y: a*c"},
    {SK_PH_MUL, 4294967299LL, 2LL, -8589934594LL, 1LL, "multiply int_y: a*c'"},
};

static void test_overflow(void) {
    for (size_t k = 0; k < sizeof OVF / sizeof OVF[0]; k++) {
        const ovf *o = &OVF[k];
        for (int r = 0; r < NROWS; r++) {
            const row *R = ROWS[r];
            void *c = R->create();
            sk_handle x = ent(R, c, o->xm, o->xd), y = ent(R, c, o->ym, o->yd), h = 12345;
            audits_ok(R, c);
            uint32_t n = R->size(c), w = R->isq ? R->wholes(c) : 0;
            sk_status s = R->op[o->op](c, x, y, &h);
            CHECK_EQ(s, SK_EOVERFLOW);
            sk_overflow rec;
            CHECK_EQ(R->overflow(c, &rec), SK_OK);
            if (!rec.step || strcmp(rec.step, o->step))
                fprintf(stderr, "  %s: overflow at '%s', expected '%s'\n", R->name, rec.step ? rec.step : "-", o->step);
            CHECK(rec.step && !strcmp(rec.step, o->step));
            CHECK_EQ(rec.x, x);
            CHECK_EQ(rec.y, y);
            CHECK_EQ(rec.op, o->op);
            CHECK_EQ(h, 12345);
            CHECK_EQ(R->size(c), n); /* nothing stored */
            if (R->isq) CHECK_EQ(R->wholes(c), w); /* nothing interned */
            audits_ok(R, c);         /* the class is valid */
            sk_handle sh = ent(R, c, 1, 2);
            val(R, c, opk(R, c, SK_PH_ADD, sh, sh), 1, 1); /* and still usable */
            R->destroy(c);
        }
    }
}

/* ----------------------------------------------------------------------------------------- the store */

static void test_store(void) {
    for (int q = 0; q < 2; q++) {
        const row *R = QROWS[q];
        void *c = R->create();
        CHECK_EQ(R->wholes(c), 2); /* 0 and 1, stored by create (v0.3 §17.2), counted in W */
        CHECK_EQ(qwhole(R, c, 0), 0);
        CHECK_EQ(qwhole(R, c, 1), 1);
        sk_handle a = ent(R, c, 2, 3), b = ent(R, c, 3, 2); /* whole numbers 2, 3 */
        CHECK_EQ(R->wholes(c), 4);
        sk_handle h = opk(R, c, SK_PH_MUL, a, a); /* 4/9: both new */
        CHECK_EQ(R->wholes(c), 6);
        h = opk(R, c, SK_PH_DIV, a, b); /* 4/9 again: both found */
        CHECK_EQ(R->wholes(c), 6);
        int n1, n2;
        uint32_t i1, j1, i2, j2;
        qrat(R, c, h, &n1, &i1, &j1);
        qrat(R, c, h - 1, &n2, &i2, &j2);
        CHECK(n1 == n2 && i1 == i2 && j1 == j2); /* equal results: two rationals, the same handles */
        CHECK_EQ(R->size(c), 4);
        h = opk(R, c, SK_PH_ADD, b, b); /* 3/1: 3 found, 1 found (stored by create) */
        val(R, c, h, 3, 1);
        CHECK_EQ(R->wholes(c), 6);
        for (sk_int v = 10; v < 14; v++) ent(R, c, v, 1); /* 10..13: W = 10 */
        CHECK_EQ(R->wholes(c), 10);
        sk_memory m;
        R->memory(c, &m);
        CHECK_EQ(m.n, 2); /* two structures (v0.3 §§7, 8) */
        uint32_t N = R->size(c);
        double bpr;
        if (R->isq == 1) { /* the whole numbers, in a list of capacity 16, and the rationals: B = 4 + 8W/N (§7) */
            CHECK(!strcmp(m.item[0].name, "whole numbers"));
            CHECK_EQ(m.item[0].used, 8 * 10);
            CHECK_EQ(m.item[0].allocated, 8 * 16);
            bpr = 4.0 + 8.0 * 10 / N;
        } else { /* the table, 256 KB used or not: B = 4 + 262,144/N (§8) */
            CHECK(!strcmp(m.item[0].name, "hash table"));
            CHECK_EQ(m.item[0].used, 262144);
            CHECK_EQ(m.item[0].allocated, 262144);
            bpr = 4.0 + 262144.0 / N;
        }
        CHECK(!strcmp(m.item[1].name, "rationals"));
        CHECK_EQ(m.item[1].used, 4ull * N);
        CHECK(m.used_per_rational > bpr - 1e-9 && m.used_per_rational < bpr + 1e-9);
        for (sk_int v = 20; v < 200; v++) ent(R, c, v, 1);
        if (R->isq == 1) { /* the list grows by doubling from 16, never above what it holds x 2 */
            R->memory(c, &m);
            CHECK_EQ(m.item[0].used, 8ull * R->wholes(c));
            CHECK_EQ(m.item[0].allocated, 8 * 256);
        }
        audits_ok(R, c);
        R->destroy(c);
    }
    /* Q32-hash's create: the table allocated and emptied, then 0 at slot 0 and 1 at slot 1: W = 2 (v0.3 §17.2) */
    void *c = ROW_Q32H.create();
    CHECK_EQ(ROW_Q32H.wholes(c), 2);
    int empty = 0;
    for (uint32_t s = 0; s < R3_SLOTS; s++) empty += qwhole(&ROW_Q32H, c, s) == -1;
    CHECK_EQ(empty, R3_SLOTS - 2);
    CHECK_EQ(qwhole(&ROW_Q32H, c, 0), 0);
    CHECK_EQ(qwhole(&ROW_Q32H, c, 1), 1);
    sk_memory m;
    ROW_Q32H.memory(c, &m);
    CHECK_EQ(m.item[0].allocated, 262144);
    CHECK_EQ(ROW_Q32H.audit(c, NULL), SK_OK);
    ROW_Q32H.destroy(c);
    /* Q32-index's create: the list at its first capacity, 0 at index 0 and 1 at index 1: W = 2 */
    c = ROW_Q32I.create();
    CHECK_EQ(ROW_Q32I.wholes(c), 2);
    CHECK_EQ(qwhole(&ROW_Q32I, c, 0), 0);
    CHECK_EQ(qwhole(&ROW_Q32I, c, 1), 1);
    ROW_Q32I.memory(c, &m);
    CHECK_EQ(m.item[0].used, 16);
    CHECK_EQ(m.item[0].allocated, 8 * 16);
    CHECK_EQ(ROW_Q32I.audit(c, NULL), SK_OK);
    ROW_Q32I.destroy(c);
    /* TC: 16 bytes per rational (v0.2 §8) */
    c = ROW_TC.create();
    for (sk_int v = 1; v < 40; v++) ent(&ROW_TC, c, v, 3);
    ROW_TC.memory(c, &m);
    CHECK_EQ(m.used, 16ull * 39);
    CHECK_EQ(m.allocated, 16ull * 64);
    CHECK(m.used_per_rational == 16.0);
    ROW_TC.destroy(c);
}

/* ----------------------------------------------------------------------------------------- the K-skips (§11) */

/* Each skip with h = 1 and with h != 1 (or g), a zero result among them: the skip's counters, the arithmetic made,
 * and in Q32 the K-Q1 indices (the store's test-build check verifies every hint against the list). */
typedef struct kx {
    int op;
    sk_int xm, xd, ym, yd;
    int k;          /* the skip expected (0: none of K-1..K-11 for the operation) */
    arith made;     /* the arithmetic of the operation */
    uint64_t kq1;   /* Q32: lookups skipped by K-Q1 */
} kx;

static const kx KX[] = {
    {SK_PH_ADD, 1, 3, 1, 3, 3, {1, 2, 0, 0, 1}, 1},          /* den, h = 1: K-3, j: j_x */
    {SK_PH_ADD, 1, 4, 3, 4, 0, {1, 3, 2, 0, 1}, 0},          /* den, h = 4 */
    {SK_PH_ADD, 1, 3, -1, 3, 2, {0, 0, 0, 0, 0}, 0},         /* zero: K-2 */
    {SK_PH_ADD, 3, 5, 3, 7, 4, {1, 3, 0, 2, 1}, 0},          /* num, g = 1: K-4 */
    {SK_PH_ADD, 3, 4, 3, 10, 5, {2, 7, 2, 2, 1}, 0},         /* num, g = 2, h = 1: K-5 */
    {SK_PH_ADD, 1, 6, 1, 10, 0, {2, 8, 4, 2, 1}, 0},         /* num, g = 2, h = 2 */
    {SK_PH_ADD, 2, 3, 1, 5, 6, {1, 3, 0, 3, 1}, 0},          /* general, g = 1: K-6 */
    {SK_PH_ADD, 1, 6, 3, 4, 7, {2, 7, 2, 3, 1}, 0},          /* general, g = 2, h = 1: K-7 */
    {SK_PH_ADD, 1, 6, 3, 10, 0, {2, 8, 4, 3, 1}, 0},         /* general, g = 2, h = 2 */
    {SK_PH_MUL, 4, 9, 5, 4, 8, {1, 4, 0, 0, 0}, 2},          /* cross_ad, h = 1: K-8 */
    {SK_PH_MUL, 4, 9, 3, 4, 0, {1, 4, 2, 0, 0}, 0},          /* cross_ad, h = 3 */
    {SK_PH_MUL, 3, 4, 4, 5, 8, {1, 3, 0, 0, 0}, 2},          /* cross_cb, h = 1: K-8 */
    {SK_PH_MUL, 2, 9, 5, 7, 9, {2, 7, 0, 2, 0}, 0},          /* general, both gcds 1: K-9 */
    {SK_PH_MUL, 2, 9, 3, 7, 9, {2, 7, 2, 2, 0}, 0},          /* general, g_2 = 3: K-9 once */
    {SK_PH_MUL, 4, 9, 3, 8, 0, {2, 8, 4, 2, 0}, 0},          /* general, g_1 = 4, g_2 = 3 */
    {SK_PH_DIV, 3, 7, 3, 5, 10, {1, 3, 0, 0, 0}, 2},         /* num, h = 1: K-10 */
    {SK_PH_DIV, 3, 4, 3, 10, 0, {1, 4, 2, 0, 0}, 0},         /* num, h = 2 */
    {SK_PH_DIV, 3, 10, 7, 10, 10, {1, 3, 0, 0, 0}, 2},       /* den, h = 1: K-10 */
    {SK_PH_DIV, 3, 10, 9, 10, 0, {1, 4, 2, 0, 0}, 0},        /* den, h = 3 */
    {SK_PH_DIV, 2, 9, 5, 7, 11, {2, 7, 0, 2, 0}, 0},         /* general, both gcds 1: K-11 */
    {SK_PH_DIV, 2, 9, -4, 3, 0, {2, 7, 4, 2, 0}, 0},         /* general, g_1 = 2, g_2 = 3 */
    /* the integer cases (v0.3 §17.3): their gcds, multiplications, additions, K-skips and hints */
    {SK_PH_ADD, 2, 1, 3, 1, 0, {0, 0, 0, 0, 1}, 1},          /* int_both: one addition; j: 1's handle */
    {SK_PH_ADD, 0, 1, 3, 5, 0, {0, 0, 0, 0, 0}, 2},          /* zero_x: none; i, j: y's */
    {SK_PH_SUB, 3, 5, 0, 1, 0, {0, 0, 0, 0, 0}, 2},          /* zero_y: none; i, j: x's */
    {SK_PH_MUL, 3, 5, 0, 1, 0, {0, 0, 0, 0, 0}, 0},          /* mul zero: none */
    {SK_PH_DIV, 0, 1, 3, 5, 0, {0, 0, 0, 0, 0}, 0},          /* div zero: none */
    {SK_PH_SUB, 2, 1, 2, 1, 2, {0, 0, 0, 0, 0}, 0},          /* x - x of an integer: zero, K-2 */
    {SK_PH_ADD, 2, 1, 3, 5, 0, {0, 0, 0, 1, 1}, 1},          /* int_x: one multiplication, no gcd; j: j_y */
    {SK_PH_ADD, 3, 5, 2, 1, 0, {0, 0, 0, 1, 1}, 1},          /* int_y: one multiplication, no gcd; j: j_x */
    {SK_PH_MUL, 2, 1, 3, 1, 0, {0, 0, 0, 1, 0}, 1},          /* int_both: one multiplication, no gcd */
    {SK_PH_MUL, 2, 1, 3, 5, 8, {1, 3, 0, 1, 0}, 1},          /* int_x, gcd(2, 5) = 1: K-8, j: j_y */
    {SK_PH_MUL, 2, 1, 3, 4, 0, {1, 3, 2, 1, 0}, 0},          /* int_x, gcd(2, 4) = 2 */
    {SK_PH_MUL, 3, 5, 2, 1, 8, {1, 3, 0, 1, 0}, 1},          /* int_y, gcd(2, 5) = 1: K-8, j: j_x */
    {SK_PH_MUL, 3, 4, 2, 1, 0, {1, 3, 2, 1, 0}, 0},          /* int_y, gcd(2, 4) = 2 */
    {SK_PH_DIV, 6, 1, 4, 1, 0, {1, 3, 2, 0, 0}, 0},          /* integers: den, gcd(6, 4) = 2, no multiplication */
    {SK_PH_DIV, 6, 1, 5, 1, 10, {1, 3, 0, 0, 0}, 2},         /* integers: den, gcd(6, 5) = 1: K-10 */
};

static void test_kskips(void) {
    for (size_t t = 0; t < sizeof KX / sizeof KX[0]; t++) {
        const kx *e = &KX[t];
        for (int r = 0; r < NROWS; r++) {
            const row *R = ROWS[r];
            void *c = R->create();
            sk_handle x = ent(R, c, e->xm, e->xd), y = ent(R, c, e->ym, e->yd);
            take(R, c, SN0);
            sk_handle h = opk(R, c, e->op, x, y);
            take(R, c, SN1);
            ref_q want = ref_op(e->op, ref_make(e->xm, e->xd), ref_make(e->ym, e->yd));
            val(R, c, h, (sk_int)want.n, (sk_int)want.d);
            arith a = arith_diff(arith_of(&SN1->b), arith_of(&SN0->b));
            if (!arith_eq(a, e->made))
                fprintf(stderr, "  %s example %zu: made gcd %llu work %llu div %llu mul %llu add %llu\n", R->name, t,
                        (unsigned long long)a.gcd, (unsigned long long)a.work, (unsigned long long)a.div,
                        (unsigned long long)a.mul, (unsigned long long)a.add);
            CHECK(arith_eq(a, e->made));
            for (int k = 1; k <= 11; k++)
                CHECK_EQ(XK(SN1->x, e->op, k, ops) - XK(SN0->x, e->op, k, ops), k == e->k ? 1 : 0);
            if (R->isq) {
                uint64_t q1 = 0;
                for (int z = 0; z < R2_NCASE; z++) q1 += XS(SN1->x, e->op, kq1[z]) - XS(SN0->x, e->op, kq1[z]);
                CHECK_EQ(q1, e->kq1);
            }
            audits_ok(R, c);
            R->destroy(c);
        }
    }
}

/* zero results: from case zero, 0 + 0 (den, h = 1), a zero factor, a zero dividend, entering zero; each +0/1 */
static void test_zero_results(void) {
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        sk_handle z = ent(R, c, 0, -4), y = ent(R, c, -3, 5), my = ent(R, c, 3, 5);
        sk_handle hs[6] = {z, opk(R, c, SK_PH_ADD, y, my), opk(R, c, SK_PH_ADD, z, z), opk(R, c, SK_PH_MUL, y, z),
                           opk(R, c, SK_PH_DIV, z, y), opk(R, c, SK_PH_SUB, z, z)};
        for (int k = 0; k < 6; k++) {
            val(R, c, hs[k], 0, 1);
            if (R->isq) {
                int neg;
                uint32_t i, j;
                qrat(R, c, hs[k], &neg, &i, &j);
                CHECK_EQ(neg, 0);
                CHECK_EQ(qwhole(R, c, i), 0);
                CHECK_EQ(qwhole(R, c, j), 1);
            }
        }
        take(R, c, SN1);
        CHECK_EQ(XS(SN1->x, -1, zero_results), 6);
        CHECK_EQ(XK(SN1->x, -1, 14, ops), 8); /* K-14: every store (3 entries, 5 operations) */
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- audits catch breaks */

static void test_audits_catch(void) {
    for (int q = 0; q < 2; q++) {
        const row *R = QROWS[q];
        for (int k = 0; k < 12; k++) {
            if ((k == 6 || k == 7 || k == 11) && R->isq != 2) continue; /* cases 6, 7 and 11: Q32-hash's slots */
            void *c = R->create();
            sk_handle h;
            R->enter(c, 2, 3, &h);
            R->enter(c, 5, 7, &h);
            CHECK_EQ(R->audit(c, NULL), SK_OK);
            int n0, n1;
            uint32_t i0, j0, i1, j1; /* 2/3 at (i0, j0), 5/7 at (i1, j1) */
            qrat(R, c, 0, &n0, &i0, &j0);
            qrat(R, c, 1, &n1, &i1, &j1);
            uint32_t unused = 0; /* a handle naming no whole number: index 9, or an empty slot */
            if (R->isq == 1) unused = 9;
            else
                while (qwhole(R, c, unused) != -1) unused++;
            switch (k) {
            case 0: R->corrupt_rat(c, 0, 0, unused, j0); break;          /* a handle naming no whole number */
            case 1: R->corrupt_rat(c, 0, 0, i0, i0); break;              /* 2/2: not lowest terms */
            case 2: R->corrupt_whole(c, j1, 2); break;                   /* 7 replaced by a duplicate of 2 */
            case 3: R->corrupt_whole(c, j1, -7); break;                  /* negative */
            case 4: R->corrupt_rat(c, 1, 0, i0, j0); break;              /* 5 and 7 used by no rational */
            case 5: R->corrupt_rat(c, 1, 1, i0, j0); R->corrupt_whole(c, i0, 0); break; /* -0/3 */
            case 6: R->corrupt_whole(c, j1, R3_EMPTY); break;            /* a used slot emptied */
            case 7: {                                                    /* 7 moved off its probe sequence */
                uint32_t far = (t_home(7) + 1000) % R3_SLOTS;
                CHECK_EQ(qwhole(R, c, far), -1);
                R->corrupt_whole(c, far, 7);
                R->corrupt_whole(c, j1, R3_EMPTY);
                R->corrupt_rat(c, 1, 0, i1, far);
                break;
            }
            /* 0 and 1 at handles 0 and 1 (v0.3 §17.2), used by no rational here: */
            case 8: R->corrupt_whole(c, 0, 4); break;                    /* handle 0 holds another value */
            case 9: R->corrupt_whole(c, 1, 9); break;                    /* handle 1 holds another value */
            case 10: R->corrupt_whole(c, j1, 1); break;                  /* 1 stored again, at 7's handle */
            case 11: R->corrupt_whole(c, 1, R3_EMPTY); break;            /* slot 1 emptied */
            }
            const char *why = NULL;
            CHECK_EQ(R->audit(c, &why), SK_EAUDIT);
            R->destroy(c);
        }
        /* the incremental audit sees a break in what changed */
        void *c = R->create();
        sk_handle h;
        R->enter(c, 2, 3, &h);
        CHECK_EQ(R->audit(c, NULL), SK_OK);
        R->enter(c, 5, 7, &h);
        int n0;
        uint32_t i0, j0;
        qrat(R, c, 0, &n0, &i0, &j0);
        R->corrupt_rat(c, 1, 0, i0, i0);
        CHECK_EQ(R->audit_changed(c, NULL), SK_EAUDIT);
        R->destroy(c);
        /* and a new whole number that duplicates an old one */
        c = R->create();
        R->enter(c, 2, 3, &h);
        CHECK_EQ(R->audit(c, NULL), SK_OK);
        R->enter(c, 5, 7, &h);
        uint32_t i1, j1;
        qrat(R, c, 1, &n0, &i1, &j1);
        R->corrupt_whole(c, j1, 3);
        CHECK_EQ(R->audit_changed(c, NULL), SK_EAUDIT);
        R->destroy(c);
    }
    for (int k = 0; k < 5; k++) {
        tc_class *c = tc_create();
        sk_handle h;
        tc_enter(c, 2, 3, &h);
        tc_enter(c, 5, 7, &h);
        CHECK_EQ(tc_audit(c, NULL), SK_OK);
        switch (k) {
        case 0: tc_t_corrupt(c, 0, 2, 0); break;          /* denominator 0 */
        case 1: tc_t_corrupt(c, 0, 4, 6); break;          /* not lowest terms */
        case 2: tc_t_corrupt(c, 1, 0, 7); break;          /* zero not (0, 1) */
        case 3: tc_t_corrupt(c, 1, INT64_MIN, 1); break;  /* -2^63 */
        case 4: tc_t_corrupt(c, 1, 5, -7); break;         /* negative denominator */
        }
        CHECK_EQ(tc_audit(c, NULL), SK_EAUDIT);
        tc_destroy(c);
    }
    tc_class *t = tc_create();
    sk_handle h;
    tc_enter(t, 2, 3, &h);
    CHECK_EQ(tc_audit(t, NULL), SK_OK);
    tc_enter(t, 5, 7, &h);
    tc_t_corrupt(t, 1, 10, 14);
    CHECK_EQ(tc_audit_changed(t, NULL), SK_EAUDIT);
    tc_destroy(t);
}

/* ----------------------------------------------------------------------------------------- random workloads */

#define WMAX 1500
typedef struct wl {
    int n;
    int op[WMAX];
    sk_int m[WMAX], d[WMAX]; /* entries */
    int x[WMAX], y[WMAX];    /* operations */
} wl;

/* entries over a small set of whole numbers (0 and +-1 included, to meet every case), then operations on earlier
 * values that stay within 64 bits */
static void make_wl(wl *w, int nin, int nops, int maxv) {
    ref_q v[WMAX];
    w->n = 0;
    for (int i = 0; i < nin; i++) {
        sk_int a = (sk_int)rndn((uint64_t)maxv + 1), b = 1 + (sk_int)rndn((uint64_t)maxv);
        if (rndn(6) == 0) a = 0;
        if (rndn(6) == 0) a = b;
        if (rndn(2)) a = -a;
        w->op[w->n] = 0;
        w->m[w->n] = a;
        w->d[w->n] = b;
        v[w->n++] = ref_make(a, b);
    }
    for (int t = 0; t < nops && w->n < WMAX; t++) {
        for (int tries = 0; tries < 50; tries++) {
            int op = 1 + (int)rndn(4), x = (int)rndn((uint64_t)w->n), y = (int)rndn((uint64_t)w->n);
            if (rndn(3) == 0) y = x;
            if (op == SK_PH_DIV && v[y].n == 0) continue;
            if (ref_abs(v[x].n) > 4000000 || ref_abs(v[y].n) > 4000000 || v[x].d > 4000000 || v[y].d > 4000000)
                continue;
            ref_q r = ref_op(op, v[x], v[y]);
            if (!ref_fits(r)) continue;
            w->op[w->n] = op;
            w->x[w->n] = x;
            w->y[w->n] = y;
            v[w->n++] = r;
            break;
        }
    }
}

typedef struct runres {
    sk_int m[WMAX], d[WMAX];
    sk_int W[R3_SLOTS]; /* Q32-index: the list in index order; Q32-hash: the table by slot (-1 empty) */
    uint32_t nW, nWdump;
    arith per[WMAX];                 /* each call's core arithmetic */
    arith kskip[WMAX];               /* each call's K-skipped arithmetic */
    uint64_t lookups[WMAX], kq[WMAX]; /* Q32: intern calls; K-Q1 + K-Q2 skipped */
    uint64_t found[WMAX], appended[WMAX]; /* Q32: intern calls that found / stored a new whole number */
    uint64_t reads[WMAX];            /* Q32: whole-number reads (K-Q3) */
    uint64_t k13[WMAX];
} runres;

static uint64_t kq_total(const r2_xcounters *x) {
    uint64_t s = 0;
    for (int z = 0; z < R2_NCASE; z++) s += XS(*x, -1, kq1[z]) + XS(*x, -1, kq2[z]);
    return s;
}

static void run_wl(const row *R, const wl *w, int force, unsigned koff, runres *o) {
    void *c = R->create();
    R->force_general(c, force);
    R->known_off(c, koff);
    sk_handle H[WMAX];
    take(R, c, SN0);
    for (int i = 0; i < w->n; i++) {
        sk_status s = w->op[i] ? R->op[w->op[i]](c, H[w->x[i]], H[w->y[i]], &H[i]) : R->enter(c, w->m[i], w->d[i], &H[i]);
        CHECK_EQ(s, SK_OK);
        take(R, c, SN1);
        o->per[i] = arith_diff(arith_of(&SN1->b), arith_of(&SN0->b));
        o->kskip[i] = arith_diff(k_skipped(&SN1->x), k_skipped(&SN0->x));
        o->lookups[i] = interns(SN1) - interns(SN0);
        o->found[i] = XS(SN1->x, -1, intern_found) - XS(SN0->x, -1, intern_found);
        o->appended[i] = XS(SN1->x, -1, intern_appended) - XS(SN0->x, -1, intern_appended);
        o->reads[i] = XS(SN1->x, -1, reads) - XS(SN0->x, -1, reads);
        o->kq[i] = kq_total(&SN1->x) - kq_total(&SN0->x);
        o->k13[i] = XK(SN1->x, -1, 13, mul) - XK(SN0->x, -1, 13, mul);
        o->kskip[i].mul -= o->k13[i]; /* K-13 is compared on its own (its switch alone) */
        snap *t = SN0;
        SN0 = SN1;
        SN1 = t;
    }
    for (int i = 0; i < w->n; i++) R->read(c, H[i], &o->m[i], &o->d[i]);
    o->nW = o->nWdump = 0;
    if (R->isq) {
        o->nW = R->wholes(c);
        o->nWdump = R->isq == 1 ? o->nW : R3_SLOTS;
        for (uint32_t k = 0; k < o->nWdump; k++) o->W[k] = qwhole(R, c, k);
    }
    audits_ok(R, c);
    R->destroy(c);
}

static bool same_values(const runres *a, const runres *b, int n) {
    for (int i = 0; i < n; i++)
        if (a->m[i] != b->m[i] || a->d[i] != b->d[i]) return false;
    return true;
}
static bool same_wholes(const runres *a, const runres *b) {
    return a->nW == b->nW && a->nWdump == b->nWdump && !memcmp(a->W, b->W, sizeof(sk_int) * a->nWdump);
}
static int cmp_int(const void *a, const void *b) {
    sk_int x = *(const sk_int *)a, y = *(const sk_int *)b;
    return (x > y) - (x < y);
}
/* the set of whole numbers of a Q32 run, sorted (n set to its size) */
static void whole_set(const runres *a, sk_int *out, uint32_t *n) {
    *n = 0;
    for (uint32_t k = 0; k < a->nWdump; k++)
        if (a->W[k] >= 0) out[(*n)++] = a->W[k];
    qsort(out, *n, sizeof(sk_int), cmp_int);
}

/* §14.3: the two switches, alone and together, give the same stored values and, in Q32, the same whole-number list;
 * with every known-answer skip off but K-13, each call's arithmetic exceeds the baseline's by exactly what its
 * K counters say was skipped, and Q32's lookups by its K-Q1 and K-Q2 counts; with K-13 off alone, its multiplications
 * by K-13's count.  TC against Q32: equal values and equal per-call arithmetic (§14.1 item 3). */
static void test_switches(void) {
    static wl w;
    static runres base[NROWS], alt;
    static sk_int set1[R3_SLOTS], set2[R3_SLOTS];
    for (int rep = 0; rep < 40; rep++) {
        make_wl(&w, 30 + (int)rndn(60), 600, rep % 2 ? 7 : 40);
        for (int r = 0; r < NROWS; r++) {
            const row *R = ROWS[r];
            run_wl(R, &w, 0, 0, &base[r]);
            for (int i = 0; i < w.n; i++) {
                ref_q want = w.op[i] ? ref_make(base[r].m[i], base[r].d[i]) : ref_make(w.m[i], w.d[i]);
                CHECK(want.n == base[r].m[i] && want.d == base[r].d[i]);
            }
            for (int mode = 0; mode < 3; mode++) {
                int force = mode != 1;
                unsigned koff = mode ? R2_KOFF_ALL : 0;
                run_wl(R, &w, force, koff, &alt);
                CHECK(same_values(&base[r], &alt, w.n));
                if (R->isq) CHECK(same_wholes(&base[r], &alt));
            }
            /* every skip off but K-13: the skipped arithmetic, call for call */
            run_wl(R, &w, 0, R2_KOFF_ALL & ~R2_KOFF(13), &alt);
            CHECK(same_values(&base[r], &alt, w.n));
            int bad = 0;
            for (int i = 0; i < w.n; i++) {
                if (!arith_eq(arith_diff(alt.per[i], base[r].per[i]), base[r].kskip[i])) bad++;
                if (R->isq && alt.lookups[i] - base[r].lookups[i] != base[r].kq[i]) bad++;
            }
            if (bad) fprintf(stderr, "  %s: %d calls whose skipped work differs from the K counters\n", R->name, bad);
            CHECK_EQ(bad, 0);
            /* K-13 off alone: one multiplication per sign applied */
            run_wl(R, &w, 0, R2_KOFF(13), &alt);
            CHECK(same_values(&base[r], &alt, w.n));
            bad = 0;
            for (int i = 0; i < w.n; i++)
                if (alt.per[i].mul - base[r].per[i].mul != base[r].k13[i]) bad++;
            CHECK_EQ(bad, 0);
        }
        /* the three rows against each other (v0.3 §12.1 items 3 and 4, §12.3): equal values, equal per-call
         * arithmetic; the two Q32 rows: the same intern calls with the same found/new split and the same reads, call
         * for call, and the same set of whole numbers */
        for (int r = 1; r < NROWS; r++) {
            CHECK(same_values(&base[0], &base[r], w.n));
            int bad = 0;
            for (int i = 0; i < w.n; i++) bad += !arith_eq(base[0].per[i], base[r].per[i]);
            if (bad) fprintf(stderr, "  %s against TC: %d calls with different arithmetic\n", ROWS[r]->name, bad);
            CHECK_EQ(bad, 0);
        }
        int bad = 0;
        for (int i = 0; i < w.n; i++)
            bad += base[1].found[i] != base[2].found[i] || base[1].appended[i] != base[2].appended[i] ||
                   base[1].reads[i] != base[2].reads[i] || base[1].lookups[i] != base[2].lookups[i];
        if (bad) fprintf(stderr, "  Q32-index against Q32-hash: %d calls with a different store\n", bad);
        CHECK_EQ(bad, 0);
        uint32_t n1, n2;
        whole_set(&base[1], set1, &n1);
        whole_set(&base[2], set2, &n2);
        CHECK(n1 == n2 && n1 == base[1].nW && !memcmp(set1, set2, sizeof(sk_int) * n1));
    }
}

/* ----------------------------------------------------------------------------------------- capacity (§14.3) */

/* 32,768 whole numbers stored and audited; the next new whole number aborts (SIGABRT, with the message), by entry and
 * by an operation, in a forked child, while the parent's class is untouched.  In Q32-hash this is the full table:
 * every slot used, the next new value failing, and a known value still found (v0.3 §12.3). */
static void *fill(const row *R) {
    void *c = R->create();
    sk_handle h;
    /* 0/1 first (0 and 1 the first two whole numbers), then k/1 for k = 1 .. 32767: 32,768 whole numbers, and the
     * handle k holds k/1 */
    CHECK_EQ(R->enter(c, 0, 1, &h), SK_OK);
    for (sk_int k = 1; k < 32768; k++) CHECK_EQ(R->enter(c, k, 1, &h), SK_OK);
    return c;
}

static void expect_abort(const row *R, void *c, int how) {
    fflush(stdout);
    fflush(stderr);
    int fd[2];
    CHECK(pipe(fd) == 0);
    pid_t p = fork();
    if (p == 0) {
        close(fd[0]);
        dup2(fd[1], 2);
        sk_handle h;
        if (how == 0) R->enter(c, 40000, 1, &h);
        else R->op[SK_PH_ADD](c, 32767, 32767, &h); /* 32767/1 + 32767/1 = 65534/1: a new whole number */
        _exit(0);
    }
    close(fd[1]);
    char msg[512] = {0};
    size_t got = 0;
    ssize_t k;
    while (got < sizeof msg - 1 && (k = read(fd[0], msg + got, sizeof msg - 1 - got)) > 0) got += (size_t)k;
    close(fd[0]);
    int st = 0;
    waitpid(p, &st, 0);
    CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT);
    const char *want = R->isq == 1 ? "capacity of 32,768 whole numbers exceeded" : "hash table overflow";
    if (!strstr(msg, want)) fprintf(stderr, "  %s: abort message '%s'\n", R->name, msg);
    CHECK(strstr(msg, want) != NULL);
}

static void test_capacity(void) {
    for (int q = 0; q < 2; q++) {
        const row *R = QROWS[q];
        void *c = fill(R);
        CHECK_EQ(R->wholes(c), 32768);
        CHECK_EQ(R->audit(c, NULL), SK_OK);
        if (R->isq == 2) { /* the full table: every slot used */
            int used = 0;
            for (uint32_t s = 0; s < R3_SLOTS; s++) used += qwhole(R, c, s) >= 0;
            CHECK_EQ(used, 32768);
        }
        sk_handle h;
        /* found whole numbers still store: 32767 - 1 = 32766/1, both found */
        CHECK_EQ(R->op[SK_PH_SUB](c, 32767, 1, &h), SK_OK);
        val(R, c, h, 32766, 1);
        CHECK_EQ(R->wholes(c), 32768);
        int neg;
        uint32_t i, j;
        /* the rational whose numerator's handle is the largest, 32,767 */
        sk_handle top = 32767;
        if (R->isq == 2) {
            for (top = 0; top < 32768; top++) {
                CHECK_EQ(R->t_rat(c, top, &neg, &i, &j), SK_OK);
                if (i == 32767) break;
            }
            CHECK(top < 32768);
        }
        CHECK_EQ(R->t_rat(c, top, &neg, &i, &j), SK_OK);
        CHECK_EQ(i, 32767);
        uint32_t n = R->size(c);
        expect_abort(R, c, 0);
        expect_abort(R, c, 1);
        CHECK_EQ(R->size(c), n);
        CHECK_EQ(R->wholes(c), 32768);
        CHECK_EQ(R->audit(c, NULL), SK_OK);
        /* handle 32,767 with the tag -: the field 0xFFFF is a valid rational */
        sk_handle m;
        sk_int tv = qwhole(R, c, 32767);
        CHECK(tv > 0);
        CHECK_EQ(R->enter(c, -tv, 1, &m), SK_OK);
        CHECK_EQ(R->t_rat(c, m, &neg, &i, &j), SK_OK);
        CHECK(neg == 1 && i == 32767);
        val(R, c, m, -tv, 1);
        CHECK_EQ(R->audit(c, NULL), SK_OK);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- Q32-index's search */

/* v0.3 §12.3: a value found at index 0, in the middle and at the end, and a new value after a full search; the
 * entries examined (k + 1 for a value at index k, W for a new one).  0 and 1 are at indices 0 and 1 from create
 * (v0.3 §17.2): the search finds them like any value, after 1 and 2 entries. */
static void test_q32i_search(void) {
    const row *R = &ROW_Q32I;
    void *c = R->create();
    for (sk_int v = 10; v < 20; v++) ent(R, c, v, 1); /* the list: 0, 1, 10, 11, ..., 19 (W = 12) */
    CHECK_EQ(R->wholes(c), 12);
    const sk_int L[12] = {0, 1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    for (uint32_t k = 0; k < 12; k++) CHECK_EQ(qwhole(R, c, k), L[k]);
    struct { sk_int v; uint64_t examined; int appended; uint32_t at; } T[5] = {
        {0, 1 + 2, 0, 0},    /* 0 at index 0; then 1 at index 1 */
        {10, 3 + 2, 0, 2},   /* the first value entered, at index 2 */
        {14, 7 + 2, 0, 6},   /* in the middle */
        {19, 12 + 2, 0, 11}, /* at the end */
        {25, 12 + 2, 1, 12}, /* new: the whole list searched, then appended at 12 */
    };
    for (int t = 0; t < 5; t++) {
        r3_scount s0 = stor(R, c);
        take(R, c, SN0);
        sk_handle h = ent(R, c, T[t].v, 1);
        take(R, c, SN1);
        r3_scount s1 = stor(R, c);
        CHECK_EQ(s1.examined - s0.examined, T[t].examined);
        CHECK_EQ(s1.probes - s0.probes, 0);
        CHECK_EQ(XS(SN1->x, -1, intern_found) - XS(SN0->x, -1, intern_found), 2 - T[t].appended);
        CHECK_EQ(XS(SN1->x, -1, intern_appended) - XS(SN0->x, -1, intern_appended), T[t].appended);
        int neg;
        uint32_t i, j;
        qrat(R, c, h, &neg, &i, &j);
        CHECK_EQ(i, T[t].at);
        CHECK_EQ(j, 1);
    }
    CHECK_EQ(R->wholes(c), 13);
    CHECK_EQ(qwhole(R, c, 12), 25);
    CHECK_EQ(qwhole(R, c, 0), 0);
    CHECK_EQ(qwhole(R, c, 1), 1);
    audits_ok(R, c);
    R->destroy(c);
}

/* ----------------------------------------------------------------------------------------- Q32-hash's probing */

/* v0.3 §12.3: values sharing a home slot (found by search here), a probe sequence that wraps from slot 32,767 to slot
 * 0, and a found value after several probes; the probes per store and the longest store.  Amended Oct 29, 2026 (v0.3
 * §17.2): 0 and 1 are found with no probe; slots 0 and 1 hold them from create, so a probe sequence that reaches them
 * passes on, and values whose home is slot 0 or slot 1 are placed after them: slots 0 and 1 are never reused. */
static void test_q32h_probing(void) {
    const row *R = &ROW_Q32H;
    /* three values with one home slot H (clear of slots 0 to 4 and 32,767); two values with home 32,767; one value
     * (not 0) with home 0 and one (not 1) with home 1 */
    static uint32_t first[R3_SLOTS], second[R3_SLOTS];
    memset(first, 0, sizeof first);
    memset(second, 0, sizeof second);
    sk_int trip[3] = {0, 0, 0}, wrap[2] = {0, 0}, z0 = 0, z1 = 0;
    for (sk_int v = 2; (!trip[0] || !wrap[0] || !z0 || !z1) && v < 200000000; v++) {
        uint32_t h = t_home(v);
        if (h == 0 && !z0) z0 = v;
        if (h == 1 && !z1) z1 = v;
        if (h == 32767 && !wrap[0] && first[h] && !second[h]) {
            wrap[0] = (sk_int)first[h];
            wrap[1] = v;
        }
        if (!first[h]) first[h] = (uint32_t)v;
        else if (!second[h]) second[h] = (uint32_t)v;
        else if (!trip[0] && h >= 5 && h + 2 < R3_SLOTS - 1) {
            trip[0] = first[h];
            trip[1] = second[h];
            trip[2] = v;
        }
    }
    CHECK(trip[0] && wrap[0] && z0 && z1);
    uint32_t H = t_home(trip[0]);
    CHECK(t_home(trip[1]) == H && t_home(trip[2]) == H && t_home(wrap[0]) == 32767 && t_home(wrap[1]) == 32767);
    void *c = R->create();
    /* 0 and 1: found at slots 0 and 1, no probe (entering 1/1 and 0/1: four stores, each found) */
    r3_scount s0 = stor(R, c);
    take(R, c, SN0);
    sk_handle one = ent(R, c, 1, 1), zero = ent(R, c, 0, 1);
    take(R, c, SN1);
    r3_scount s1 = stor(R, c);
    CHECK_EQ(s1.probes - s0.probes, 0);
    CHECK_EQ(XS(SN1->x, -1, intern_found) - XS(SN0->x, -1, intern_found), 4);
    CHECK_EQ(XS(SN1->x, -1, intern_appended) - XS(SN0->x, -1, intern_appended), 0);
    CHECK_EQ(R->wholes(c), 2);
    int neg;
    uint32_t i, j;
    qrat(R, c, one, &neg, &i, &j);
    CHECK(i == 1 && j == 1);
    qrat(R, c, zero, &neg, &i, &j);
    CHECK(i == 0 && j == 1);
    sk_handle hs[3];
    s0 = stor(R, c);
    for (int k = 0; k < 3; k++) hs[k] = ent(R, c, trip[k], 1);
    s1 = stor(R, c);
    CHECK_EQ(s1.probes - s0.probes, 1 + 2 + 3); /* at H, H + 1, H + 2; the denominator 1: no probe */
    for (int k = 0; k < 3; k++) {
        qrat(R, c, hs[k], &neg, &i, &j);
        CHECK_EQ(i, H + (uint32_t)k);
        CHECK_EQ(j, 1);
    }
    s0 = stor(R, c);
    sk_handle again = ent(R, c, trip[2], 1); /* found after three probes; 1 after none */
    s1 = stor(R, c);
    CHECK_EQ(s1.probes - s0.probes, 3);
    CHECK(s1.longest >= 3);
    val(R, c, again, trip[2], 1);
    /* the wrap: the second value with home 32,767 passes slots 0 and 1, which hold 0 and 1, to slot 2 */
    CHECK_EQ(qwhole(R, c, 32767), -1);
    CHECK_EQ(qwhole(R, c, 2), -1);
    sk_handle w0 = ent(R, c, wrap[0], 1);
    s0 = stor(R, c);
    take(R, c, SN0);
    sk_handle w1 = ent(R, c, wrap[1], 1);
    take(R, c, SN1);
    s1 = stor(R, c);
    CHECK_EQ(s1.probes - s0.probes, 4); /* 32,767, 0, 1, then 2 */
    CHECK_EQ(XS(SN1->x, -1, intern_appended) - XS(SN0->x, -1, intern_appended), 1);
    qrat(R, c, w0, &neg, &i, &j);
    CHECK_EQ(i, 32767);
    qrat(R, c, w1, &neg, &i, &j);
    CHECK_EQ(i, 2);
    s0 = stor(R, c); /* found again across the wrap */
    ent(R, c, wrap[1], 1);
    s1 = stor(R, c);
    CHECK_EQ(s1.probes - s0.probes, 4);
    /* values whose home is slot 0 or slot 1: placed after slots 0 and 1 (and slot 2, now used), never in them */
    sk_handle hz0 = ent(R, c, z0, 1), hz1 = ent(R, c, z1, 1);
    qrat(R, c, hz0, &neg, &i, &j);
    CHECK_EQ(i, 3); /* 0, 1, 2 used */
    qrat(R, c, hz1, &neg, &i, &j);
    CHECK_EQ(i, 4); /* 1, 2, 3 used */
    val(R, c, hz0, z0, 1);
    val(R, c, hz1, z1, 1);
    CHECK_EQ(qwhole(R, c, 0), 0);
    CHECK_EQ(qwhole(R, c, 1), 1);
    CHECK_EQ(R->wholes(c), 2 + 3 + 2 + 2);
    audits_ok(R, c);
    R->destroy(c);
}

/* Slots 0 and 1 (Q32-hash) and indices 0 and 1 (Q32-index) hold 0 and 1 through any workload, and no other value is
 * ever stored there (v0.3 §17.2): random workloads, every handle of every stored rational checked. */
static void test_fixed_handles(void) {
    static wl w;
    for (int rep = 0; rep < 20; rep++) {
        make_wl(&w, 40 + (int)rndn(60), 800, rep % 2 ? 9 : 300);
        for (int q = 0; q < 2; q++) {
            const row *R = QROWS[q];
            void *c = R->create();
            sk_handle Hh[WMAX];
            for (int k = 0; k < w.n; k++) {
                sk_status st = w.op[k] ? R->op[w.op[k]](c, Hh[w.x[k]], Hh[w.y[k]], &Hh[k]) : R->enter(c, w.m[k], w.d[k], &Hh[k]);
                CHECK_EQ(st, SK_OK);
                int neg;
                uint32_t i, j;
                qrat(R, c, Hh[k], &neg, &i, &j);
                sk_int m, d;
                R->read(c, Hh[k], &m, &d);
                CHECK((i == 0) == (m == 0)); /* a numerator handle of 0 means zero */
                CHECK((j == 1) == (d == 1)); /* a denominator handle of 1 means an integer */
                CHECK(j != 0);
            }
            CHECK_EQ(qwhole(R, c, 0), 0);
            CHECK_EQ(qwhole(R, c, 1), 1);
            audits_ok(R, c);
            R->destroy(c);
        }
    }
}


/* ----------------------------------------------------------------------------------------- the order of the cases */

/* The cost of a case's result on given operands (v0.3 §17.3: cases are ordered by the cost of their result, cheapest
 * first): its gcds, then its other arithmetic (exact divisions, multiplications, additions), with every known-answer
 * skip on.  Magnitudes a/b and c/d in lowest terms, effective tags sx, sy.  This is the formula of each case, written
 * here independently of the rows; the test checks it against the counters of the case actually taken. */
typedef struct cost {
    int gcd, rest;
} cost;
static int64_t t_gcd(int64_t p, int64_t q) {
    p = p < 0 ? -p : p;
    q = q < 0 ? -q : q;
    while (q) {
        int64_t t = p % q;
        p = q;
        q = t;
    }
    return p;
}
static cost case_cost(int k, int64_t a, int64_t b, int64_t c, int64_t d, int sx, int sy) {
    int64_t A = sx ? -a : a, Cc = sy ? -c : c, g, h;
    switch (k) {
    case R2_ADD_ZERO: case R2_ADD_ZERO_X: case R2_ADD_ZERO_Y: case R2_MUL_ZERO: case R2_DIV_ZERO: case R2_MUL_ONE_X: case R2_MUL_ONE_Y: case R2_MUL_BOTH:
    case R2_DIV_ONE_X: case R2_DIV_ONE_Y: case R2_DIV_BOTH: return (cost){0, 0};
    case R2_ADD_INT_BOTH: return (cost){0, 1};
    case R2_ADD_INT_X: case R2_ADD_INT_Y: return (cost){0, 2};
    case R2_ADD_CROSS: return (cost){0, 4};
    case R2_ADD_DEN: return (cost){1, 1 + (t_gcd(A + Cc, d) != 1) * 2};
    case R2_ADD_NUM:
        g = t_gcd(b, d);
        if (g == 1) return (cost){1, 3};
        h = t_gcd((sx ? -d / g : d / g) + (sy ? -b / g : b / g), g);
        return (cost){2, h == 1 ? 5 : 7};
    case R2_ADD_GEN:
        g = t_gcd(b, d);
        if (g == 1) return (cost){1, 4};
        h = t_gcd(A * (d / g) + Cc * (b / g), g);
        return (cost){2, h == 1 ? 6 : 8};
    case R2_MUL_INT_BOTH: return (cost){0, 1};
    case R2_MUL_DEN: case R2_MUL_NUM: case R2_DIV_CROSS_AD: case R2_DIV_CROSS_CB: return (cost){0, 2};
    case R2_MUL_CROSS_AD: return (cost){1, (t_gcd(c, b) != 1) * 2};
    case R2_MUL_CROSS_CB: return (cost){1, (t_gcd(a, d) != 1) * 2};
    case R2_MUL_INT_X: return (cost){1, 1 + (t_gcd(a, d) != 1) * 2};
    case R2_MUL_INT_Y: return (cost){1, 1 + (t_gcd(c, b) != 1) * 2};
    case R2_MUL_GEN: return (cost){2, 2 + (t_gcd(a, d) != 1) * 2 + (t_gcd(c, b) != 1) * 2};
    case R2_DIV_NUM: return (cost){1, (t_gcd(d, b) != 1) * 2};
    case R2_DIV_DEN: return (cost){1, (t_gcd(a, c) != 1) * 2};
    case R2_DIV_GEN: return (cost){2, 2 + (t_gcd(a, c) != 1) * 2 + (t_gcd(b, d) != 1) * 2};
    }
    return (cost){99, 99};
}
/* whether case k's formula holds for these operands (its condition) */
static bool case_applies(int k, int64_t a, int64_t b, int64_t c, int64_t d, int sx, int sy) {
    switch (k) {
    case R2_ADD_ZERO: return b == d && a == c && sx != sy;
    case R2_ADD_INT_BOTH: case R2_MUL_INT_BOTH: return b == 1 && d == 1;
    case R2_ADD_INT_X: case R2_MUL_INT_X: return b == 1;
    case R2_ADD_INT_Y: case R2_MUL_INT_Y: return d == 1;
    case R2_ADD_ZERO_X: return a == 0;
    case R2_ADD_ZERO_Y: return c == 0;
    case R2_ADD_CROSS: return a == d || c == b;
    case R2_ADD_DEN: case R2_MUL_DEN: case R2_DIV_DEN: return b == d;
    case R2_ADD_NUM: case R2_MUL_NUM: case R2_DIV_NUM: return a == c;
    case R2_MUL_ZERO: return a == 0 || c == 0;
    case R2_DIV_ZERO: return a == 0;
    case R2_MUL_ONE_X: case R2_DIV_ONE_X: return a == b;
    case R2_MUL_ONE_Y: case R2_DIV_ONE_Y: return c == d;
    case R2_MUL_BOTH: return a == d && c == b;
    case R2_DIV_BOTH: return a == c && b == d;
    case R2_MUL_CROSS_AD: case R2_DIV_CROSS_AD: return a == d;
    case R2_MUL_CROSS_CB: case R2_DIV_CROSS_CB: return c == b;
    default: return true; /* general */
    }
}
static bool case_of_op(int k, int op) {
    static const int add[] = {R2_ADD_ZERO, R2_ADD_ZERO_X, R2_ADD_ZERO_Y, R2_ADD_INT_BOTH, R2_ADD_INT_X, R2_ADD_INT_Y,
                              R2_ADD_CROSS, R2_ADD_DEN, R2_ADD_NUM, R2_ADD_GEN};
    static const int mul[] = {R2_MUL_ZERO, R2_MUL_ONE_X, R2_MUL_ONE_Y, R2_MUL_INT_BOTH, R2_MUL_BOTH, R2_MUL_DEN,
                              R2_MUL_NUM, R2_MUL_CROSS_AD, R2_MUL_CROSS_CB, R2_MUL_INT_X, R2_MUL_INT_Y, R2_MUL_GEN};
    static const int dv[] = {R2_DIV_ZERO, R2_DIV_ONE_X, R2_DIV_ONE_Y, R2_DIV_BOTH, R2_DIV_CROSS_AD, R2_DIV_CROSS_CB,
                             R2_DIV_NUM, R2_DIV_DEN, R2_DIV_GEN};
    const int *v = op == SK_PH_MUL ? mul : op == SK_PH_DIV ? dv : add;
    int n = op == SK_PH_MUL ? 12 : op == SK_PH_DIV ? 9 : 10;
    for (int i = 0; i < n; i++)
        if (v[i] == k) return true;
    return false;
}

/* Exhaustively over small operands (magnitudes 0..12 over 1..12, in lowest terms, both signs), every operation, every
 * row: the case taken is §9/§17.3's; its arithmetic (from the counters) is its cost formula's; and no case whose
 * condition holds would give a cheaper result (fewer gcds, then less other arithmetic) than the case taken. */
static void test_case_order(void) {
    enum { MAXV = 12 };
    static sk_int vm[400], vd[400];
    int nv = 0;
    for (sk_int b = 1; b <= MAXV; b++)
        for (sk_int a = 0; a <= MAXV; a++) {
            if (t_gcd(a, b) != 1 || (a == 0 && b != 1)) continue;
            vm[nv] = a, vd[nv++] = b;
            if (a) vm[nv] = -a, vd[nv++] = b;
        }
    uint64_t ops = 0, cheaper = 0, wrong = 0, mism = 0;
    for (int r = 0; r < NROWS; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        sk_handle H[400];
        for (int i = 0; i < nv; i++) H[i] = ent(R, c, vm[i], vd[i]);
        for (int op = SK_PH_ADD; op <= SK_PH_DIV; op++)
            for (int i = 0; i < nv; i++)
                for (int j = 0; j < nv; j++) {
                    if (op == SK_PH_DIV && vm[j] == 0) continue;
                    int64_t a = vm[i] < 0 ? -vm[i] : vm[i], b = vd[i], cc = vm[j] < 0 ? -vm[j] : vm[j], d = vd[j];
                    int sx = vm[i] < 0, sy = (vm[j] < 0) != (op == SK_PH_SUB);
                    take(R, c, SN0);
                    opk(R, c, op, H[i], H[j]);
                    take(R, c, SN1);
                    ops++;
                    int k = predict_case(op, (uint64_t)a, (uint64_t)b, (uint64_t)cc, (uint64_t)d, sx, sy);
                    if (XCASES(SN1->x, op, k) - XCASES(SN0->x, op, k) != 1) wrong++;
                    arith m = arith_diff(arith_of(&SN1->b), arith_of(&SN0->b));
                    cost ck = case_cost(k, a, b, cc, d, sx, sy);
                    if ((int)m.gcd != ck.gcd || (int)(m.div + m.mul + m.add) != ck.rest) {
                        if (mism < 5)
                            fprintf(stderr, "  %s %s %lld/%lld %lld/%lld: case %s made %llu gcd %llu other, formula %d %d\n",
                                    R->name, sk_phase_name((sk_phase)op), (long long)vm[i], (long long)b,
                                    (long long)vm[j], (long long)d, r2_case_name(k), (unsigned long long)m.gcd,
                                    (unsigned long long)(m.div + m.mul + m.add), ck.gcd, ck.rest);
                        mism++;
                    }
                    for (int z = 0; z < R2_NCASE; z++) {
                        if (!case_of_op(z, op) || !case_applies(z, a, b, cc, d, sx, sy)) continue;
                        cost cz = case_cost(z, a, b, cc, d, sx, sy);
                        if (cz.gcd < ck.gcd || (cz.gcd == ck.gcd && cz.rest < ck.rest)) {
                            if (cheaper < 5)
                                fprintf(stderr, "  %s %lld/%lld %lld/%lld: took %s (%d, %d), %s is cheaper (%d, %d)\n",
                                        sk_phase_name((sk_phase)op), (long long)vm[i], (long long)b, (long long)vm[j],
                                        (long long)d, r2_case_name(k), ck.gcd, ck.rest, r2_case_name(z), cz.gcd,
                                        cz.rest);
                            cheaper++;
                        }
                    }
                }
        audits_ok(R, c);
        R->destroy(c);
    }
    CHECK_EQ(wrong, 0);
    CHECK_EQ(mism, 0);
    CHECK_EQ(cheaper, 0);
    printf("  case order: %llu operations over %d values, three rows: no case shadowed by a cheaper one\n",
           (unsigned long long)ops, nv);
}

/* ----------------------------------------------------------------------------------------- builds */

static void test_builds(void) {
    CHECK_EQ(sk_build_width(), 64);
    CHECK_EQ(sk_build_counting(), 1);
    CHECK_EQ(sk_build_audit(), 1);
    r2_profile p;
    q32i_class *c = q32i_create();
    CHECK_EQ(q32i_profile(c, &p), SK_ENOTBUILT); /* the profile is the profiling build's only */
    q32i_destroy(c);
    q32h_class *g = q32h_create();
    CHECK_EQ(q32h_profile(g, &p), SK_ENOTBUILT);
    r3_scounters sc;
    CHECK_EQ(q32h_storage(g, &sc), SK_OK); /* the storage counters: counting builds */
    q32h_destroy(g);
    tc_class *t = tc_create();
    CHECK_EQ(tc_profile(t, &p), SK_ENOTBUILT);
    tc_destroy(t);
    CHECK(r2_probe_cost(3, 3) == 0.0);
}

int main(void) {
    SN0 = malloc(sizeof *SN0);
    SN1 = malloc(sizeof *SN1);
    test_builds();
    test_worked_example();
    test_every_case();
    test_signs();
    test_zero();
    test_one();
    test_overlaps();
    test_entry();
    test_overflow();
    test_store();
    test_kskips();
    test_zero_results();
    test_audits_catch();
    test_switches();
    test_capacity();
    test_q32i_search();
    test_q32h_probing();
    test_fixed_handles();
    test_case_order();
    free(SN0);
    free(SN1);
    printf("r3_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
