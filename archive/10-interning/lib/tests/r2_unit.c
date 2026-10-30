/* r2_unit.c: unit tests of round 2's rows, Q32 and TC (spec v0.2 §14.3: v0.1 §14.3 item 1 for both rows, at Q32's
 * width, with the capacity tests, case zero, 0 + 0, every K-skip, the +-1 operand shortcut, the two switches, and TC
 * against Q32 call for call).  Test build only. */
#define _GNU_SOURCE
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "r2t.h"

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

static void qrat(void *c, sk_handle h, int *neg, uint32_t *i, uint32_t *j) {
    CHECK(q32_t_rat((const q32_class *)c, h, neg, i, j) == SK_OK);
}
static sk_int qwhole(void *c, uint32_t k) {
    sk_int v = -1;
    q32_t_whole((const q32_class *)c, k, &v);
    return v;
}

/* the counters of one call: before and after */
typedef struct snap {
    sk_counters b;
    r2_xcounters x;
} snap;
static snap *SN0, *SN1;
static void take(const row *R, void *c, snap *s) { R->counters(c, &s->b, &s->x); }

/* ----------------------------------------------------------------------------------------- the worked example */

/* Paper §5.5 (v0.1 §14.3): the four entries and their indices; the eight operations, in the paper's order, on the
 * original four values, with the case, the gcds made, the result and, in Q32, the indices appended or found. */
static void test_worked_example(void) {
    for (int r = 0; r < 2; r++) {
        const row *R = ROWS[r];
        void *c = R->create();
        sk_handle x0 = ent(R, c, 1, 6), x1 = ent(R, c, -5, 6), x2 = ent(R, c, 5, 7), x3 = ent(R, c, 6, 7);
        if (R->isq) {
            const sk_int W0[4] = {1, 6, 5, 7};
            CHECK_EQ(q32_wholes(c), 4);
            for (uint32_t k = 0; k < 4; k++) CHECK_EQ(qwhole(c, k), W0[k]);
            const struct { sk_handle h; int neg; uint32_t i, j; } E[4] = {{x0, 0, 0, 1}, {x1, 1, 2, 1}, {x2, 0, 2, 3},
                                                                       {x3, 0, 1, 3}};
            for (int k = 0; k < 4; k++) {
                int neg;
                uint32_t i, j;
                qrat(c, E[k].h, &neg, &i, &j);
                CHECK_EQ(neg, E[k].neg);
                CHECK_EQ(i, E[k].i);
                CHECK_EQ(j, E[k].j);
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
            {SK_PH_ADD, x0, x1, R2_ADD_DEN, 1, -2, 3, 1, 4, 5, 6},
            {SK_PH_MUL, x0, x1, R2_MUL_DEN, 0, -5, 36, 1, 2, 6, 7},
            {SK_PH_MUL, x1, x2, R2_MUL_NUM, 0, -25, 42, 1, 7, 8, 9},
            {SK_PH_MUL, x0, x3, R2_MUL_CROSS_CB, 1, 1, 7, 0, 0, 3, 9},
            {SK_PH_DIV, x2, x1, R2_DIV_NUM, 1, -6, 7, 1, 1, 3, 9},
            {SK_PH_DIV, x3, x2, R2_DIV_DEN, 1, 6, 5, 0, 1, 2, 9},
            {SK_PH_DIV, x3, x0, R2_DIV_CROSS_AD, 0, 36, 7, 0, 6, 3, 9},
            {SK_PH_ADD, x1, x3, R2_ADD_CROSS, 0, 1, 42, 0, 0, 8, 9},
        };
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
                qrat(c, h, &neg, &i, &j);
                CHECK_EQ(neg, T[k].neg);
                CHECK_EQ(i, T[k].i);
                CHECK_EQ(j, T[k].j);
                CHECK_EQ(q32_wholes(c), T[k].W);
            }
        }
        if (R->isq) { /* the indices appended along the way: 2, 3 at 4, 5; 36 at 6; 25, 42 at 7, 8 */
            const sk_int W1[9] = {1, 6, 5, 7, 2, 3, 36, 25, 42};
            for (uint32_t k = 0; k < 9; k++) CHECK_EQ(qwhole(c, k), W1[k]);
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
    {SK_PH_ADD, 0, 1, 0, 1, R2_ADD_DEN, 0, 1},          /* 0 + 0: den, t = 0, h = 1 (§9) */
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
    {SK_PH_MUL, -1, 1, 0, 1, R2_MUL_ONE_X, 0, 1},       /* -1 * 0: the tag stays + */
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
    {SK_PH_DIV, 0, 1, -1, 1, R2_DIV_ONE_Y, 0, 1},       /* 0 / -1: the tag stays + */
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
    CHECK_EQ(XS(SN1->x, e->op, cmp) - XS(SN0->x, e->op, cmp), e->op >= SK_PH_MUL ? 6 : 4);
    audits_ok(R, c);
    R->destroy(c);
}

static void test_every_case(void) {
    for (size_t k = 0; k < NEX; k++)
        for (int r = 0; r < 2; r++) {
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
                for (int r = 0; r < 2; r++) {
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
                        qrat(c, h, &neg, &i, &j);
                        CHECK_EQ(neg, 0);
                        CHECK_EQ(qwhole(c, i), 0);
                        CHECK_EQ(qwhole(c, j), 1);
                    }
                    audits_ok(R, c);
                    R->destroy(c);
                }
    }
}

/* ----------------------------------------------------------------------------------------- zero */

static void test_zero(void) {
    for (int r = 0; r < 2; r++) {
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
    for (int r = 0; r < 2; r++)
        for (int op = SK_PH_MUL; op <= SK_PH_DIV; op++)
            for (int side = 0; side < 2; side++) /* 0: x = +-1; 1: y = +-1 */
                for (int so = 0; so < 2; so++)    /* the sign of the +-1 */
                    for (size_t k = 0; k < sizeof Ys / sizeof Ys[0]; k++)
                        for (int sv = 0; sv < 2; sv++) {
                            const row *R = ROWS[r];
                            sk_int om = so ? -1 : 1, vm = sv ? -Ys[k][0] : Ys[k][0], vd = Ys[k][1];
                            if (op == SK_PH_DIV && side == 0 && vm == 0) continue; /* below: division by zero */
                            void *c = R->create();
                            sk_handle o = ent(R, c, om, 1), v = ent(R, c, vm, vd);
                            sk_handle x = side ? v : o, y = side ? o : v;
                            uint32_t w0 = R->isq ? q32_wholes(c) : 0;
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
                                CHECK_EQ(q32_wholes(c), w0);
                                CHECK_EQ(BS(SN1->b, op, lookup_search) - BS(SN0->b, op, lookup_search), 0);
                                CHECK_EQ(XS(SN1->x, op, kq1[kase]) - XS(SN0->x, op, kq1[kase]), 2);
                                int neg, n0;
                                uint32_t i, j, i0, j0;
                                qrat(c, h, &neg, &i, &j);
                                /* the operand whose whole numbers the result takes */
                                int recip = op == SK_PH_DIV && kase == R2_DIV_ONE_X;
                                sk_handle src = (kase == R2_MUL_ONE_X || kase == R2_DIV_ONE_X) ? y : x;
                                qrat(c, src, &n0, &i0, &j0);
                                CHECK_EQ(i, recip ? j0 : i0);
                                CHECK_EQ(j, recip ? i0 : j0);
                                CHECK_EQ(neg, want.n < 0);
                                /* reads (K-Q3): the other operand's numerator only when the +-1 is -1; a division
                                 * also reads c for its zero test, and one_y does not read a again when a = c */
                                uint64_t reads = XS(SN1->x, op, reads) - XS(SN0->x, op, reads);
                                int xn, yn;
                                uint32_t xi, xj, yi, yj;
                                qrat(c, x, &xn, &xi, &xj);
                                qrat(c, y, &yn, &yi, &yj);
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
        {SK_PH_ADD, 1, 1, 1, 1, R2_ADD_CROSS, 2, 1},       /* cross, den and num, equal tags: cross */
        {SK_PH_ADD, 1, 1, 1, 3, R2_ADD_CROSS, 4, 3},       /* num and cross: cross */
        {SK_PH_ADD, 3, 5, 3, 5, R2_ADD_DEN, 6, 5},         /* den and num, equal tags: den */
        {SK_PH_MUL, 3, 5, 3, 5, R2_MUL_DEN, 9, 25},        /* den and num: den */
        {SK_PH_MUL, 1, 1, 1, 1, R2_MUL_ONE_X, 1, 1},       /* everything: one_x */
        {SK_PH_MUL, 2, 1, 1, 2, R2_MUL_BOTH, 1, 1},        /* both */
        {SK_PH_MUL, 1, 3, 1, 1, R2_MUL_ONE_Y, 1, 3},       /* num, cross_ad and one_y: one_y */
        {SK_PH_MUL, 3, 1, 5, 3, R2_MUL_CROSS_AD, 5, 1},    /* cross_ad */
        {SK_PH_MUL, 1, 1, 3, 1, R2_MUL_ONE_X, 3, 1},       /* one_x, den and cross_ad: one_x */
        {SK_PH_DIV, 1, 1, 1, 1, R2_DIV_ONE_X, 1, 1},       /* everything: one_x */
        {SK_PH_DIV, 2, 1, 1, 2, R2_DIV_CROSS_AD, 4, 1},    /* cross_ad and cross_cb: cross_ad */
        {SK_PH_DIV, 1, 3, 1, 1, R2_DIV_ONE_Y, 1, 3},       /* num, cross_ad and one_y: one_y */
        {SK_PH_DIV, 3, 1, 1, 1, R2_DIV_ONE_Y, 3, 1},       /* den, cross_cb and one_y: one_y */
        {SK_PH_DIV, 3, 7, 3, 5, R2_DIV_NUM, 5, 7},         /* num */
        {SK_PH_DIV, 2, 3, 3, 2, R2_DIV_CROSS_AD, 4, 9},    /* cross_ad and cross_cb: b = c, read once */
    };
    for (size_t k = 0; k < sizeof O / sizeof O[0]; k++)
        for (int r = 0; r < 2; r++) run_example(ROWS[r], &O[k], 0, 0);
    for (int r = 0; r < 2; r++) {
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
    for (int r = 0; r < 2; r++) {
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
    {SK_PH_ADD, -1LL, 1LL, -4611686018427387903LL, 6148914691236517205LL, "add general: s_x*a*d + s_y*c*b"},
    {SK_PH_SUB, -2000000000000000002LL, 4294967299LL, 1073741824LL, 4294967297LL, "add general: s_x*a*d"},
    {SK_PH_ADD, -2305843009213693951LL, 25769803794LL, 9LL, 10LL, "add general: s_x*a*d'"},
    {SK_PH_ADD, -1500000000000000001LL, 4611686018427387905LL, 10LL, 1LL, "add general: s_y*c*b"},
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
};

static void test_overflow(void) {
    for (size_t k = 0; k < sizeof OVF / sizeof OVF[0]; k++) {
        const ovf *o = &OVF[k];
        for (int r = 0; r < 2; r++) {
            const row *R = ROWS[r];
            void *c = R->create();
            sk_handle x = ent(R, c, o->xm, o->xd), y = ent(R, c, o->ym, o->yd), h = 12345;
            audits_ok(R, c);
            uint32_t n = R->size(c), w = R->isq ? q32_wholes(c) : 0;
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
            if (R->isq) CHECK_EQ(q32_wholes(c), w); /* nothing interned */
            audits_ok(R, c);         /* the class is valid */
            sk_handle sh = ent(R, c, 1, 2);
            val(R, c, opk(R, c, SK_PH_ADD, sh, sh), 1, 1); /* and still usable */
            R->destroy(c);
        }
    }
}

/* ----------------------------------------------------------------------------------------- the store */

static void test_store(void) {
    void *c = ROW_Q32.create();
    const row *R = &ROW_Q32;
    sk_handle a = ent(R, c, 2, 3), b = ent(R, c, 3, 2); /* W: 2, 3 */
    CHECK_EQ(q32_wholes(c), 2);
    sk_handle h = opk(R, c, SK_PH_MUL, a, a); /* 4/9: both new */
    CHECK_EQ(q32_wholes(c), 4);
    h = opk(R, c, SK_PH_DIV, a, b); /* 4/9 again: both found */
    CHECK_EQ(q32_wholes(c), 4);
    int n1, n2;
    uint32_t i1, j1, i2, j2;
    qrat(c, h, &n1, &i1, &j1);
    qrat(c, h - 1, &n2, &i2, &j2);
    CHECK(n1 == n2 && i1 == i2 && j1 == j2); /* equal results: two rationals, the same indices */
    CHECK_EQ(R->size(c), 4);
    h = opk(R, c, SK_PH_ADD, b, b); /* 3/1: 3 found, 1 new */
    val(R, c, h, 3, 1);
    CHECK_EQ(q32_wholes(c), 5);
    /* growth of the lookup: S = 16 holds 8; the 9th whole number doubles it */
    sk_memory m;
    R->memory(c, &m);
    CHECK_EQ(m.item[1].used, 16 * 2);
    for (sk_int v = 10; v < 14; v++) ent(R, c, v, 1); /* 10..13: W = 9 */
    CHECK_EQ(q32_wholes(c), 9);
    R->memory(c, &m);
    CHECK_EQ(m.item[1].used, 32 * 2);
    CHECK_EQ(m.item[0].used, 8 * 9);
    CHECK_EQ(m.item[2].used, 4ull * R->size(c));
    double bpr = 4.0 + (8.0 * 9 + 2.0 * 32) / R->size(c); /* B = 4 + (8W + 2S)/N (§7) */
    CHECK(m.used_per_rational > bpr - 1e-9 && m.used_per_rational < bpr + 1e-9);
    for (sk_int v = 20; v < 200; v++) ent(R, c, v, 1);
    audits_ok(R, c);
    R->destroy(c);
    /* TC: 16 bytes per rational (§8) */
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
};

static void test_kskips(void) {
    for (size_t t = 0; t < sizeof KX / sizeof KX[0]; t++) {
        const kx *e = &KX[t];
        for (int r = 0; r < 2; r++) {
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
    for (int r = 0; r < 2; r++) {
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
                qrat(c, hs[k], &neg, &i, &j);
                CHECK_EQ(neg, 0);
                CHECK_EQ(qwhole(c, i), 0);
                CHECK_EQ(qwhole(c, j), 1);
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
    for (int k = 0; k < 6; k++) {
        q32_class *c = q32_create();
        sk_handle h;
        q32_enter(c, 2, 3, &h);
        q32_enter(c, 5, 7, &h);
        CHECK_EQ(q32_audit(c, NULL), SK_OK);
        switch (k) {
        case 0: q32_t_corrupt_rat(c, 0, 0, 9, 1); break;  /* index not below W */
        case 1: q32_t_corrupt_rat(c, 0, 0, 0, 0); break;  /* 2/2: not lowest terms */
        case 2: q32_t_corrupt_whole(c, 3, 2); break;      /* duplicate of index 0 */
        case 3: q32_t_corrupt_whole(c, 3, -7); break;     /* negative */
        case 4: q32_t_corrupt_rat(c, 1, 0, 0, 1); break;  /* 7 (index 3) used by no rational */
        case 5: q32_t_corrupt_rat(c, 1, 1, 0, 1); q32_t_corrupt_whole(c, 0, 0); break; /* -0/3 */
        }
        const char *why = NULL;
        CHECK_EQ(q32_audit(c, &why), SK_EAUDIT);
        q32_destroy(c);
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
    /* the incremental audit sees a break in what changed */
    q32_class *q = q32_create();
    sk_handle h;
    q32_enter(q, 2, 3, &h);
    CHECK_EQ(q32_audit(q, NULL), SK_OK);
    q32_enter(q, 5, 7, &h);
    q32_t_corrupt_rat(q, 1, 0, 0, 0);
    CHECK_EQ(q32_audit_changed(q, NULL), SK_EAUDIT);
    q32_destroy(q);
    tc_class *t = tc_create();
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
    sk_int W[WMAX * 2];
    uint32_t nW;
    arith per[WMAX];                 /* each call's core arithmetic */
    arith kskip[WMAX];               /* each call's K-skipped arithmetic */
    uint64_t lookups[WMAX], kq[WMAX]; /* Q32: lookup searches; K-Q1 + K-Q2 skipped */
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
        o->lookups[i] = BS(SN1->b, -1, lookup_search) - BS(SN0->b, -1, lookup_search);
        o->kq[i] = kq_total(&SN1->x) - kq_total(&SN0->x);
        o->k13[i] = XK(SN1->x, -1, 13, mul) - XK(SN0->x, -1, 13, mul);
        o->kskip[i].mul -= o->k13[i]; /* K-13 is compared on its own (its switch alone) */
        snap *t = SN0;
        SN0 = SN1;
        SN1 = t;
    }
    for (int i = 0; i < w->n; i++) R->read(c, H[i], &o->m[i], &o->d[i]);
    o->nW = 0;
    if (R->isq) {
        o->nW = q32_wholes(c);
        for (uint32_t k = 0; k < o->nW; k++) o->W[k] = qwhole(c, k);
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
    return a->nW == b->nW && !memcmp(a->W, b->W, sizeof(sk_int) * a->nW);
}

/* §14.3: the two switches, alone and together, give the same stored values and, in Q32, the same whole-number list;
 * with every known-answer skip off but K-13, each call's arithmetic exceeds the baseline's by exactly what its
 * K counters say was skipped, and Q32's lookups by its K-Q1 and K-Q2 counts; with K-13 off alone, its multiplications
 * by K-13's count.  TC against Q32: equal values and equal per-call arithmetic (§14.1 item 3). */
static void test_switches(void) {
    static wl w;
    static runres base[2], alt;
    for (int rep = 0; rep < 40; rep++) {
        make_wl(&w, 30 + (int)rndn(60), 600, rep % 2 ? 7 : 40);
        for (int r = 0; r < 2; r++) {
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
        CHECK(same_values(&base[0], &base[1], w.n));
        int bad = 0;
        for (int i = 0; i < w.n; i++) bad += !arith_eq(base[0].per[i], base[1].per[i]);
        if (bad) fprintf(stderr, "  Q32 against TC: %d calls with different arithmetic\n", bad);
        CHECK_EQ(bad, 0);
    }
}

/* ----------------------------------------------------------------------------------------- capacity (§14.3) */

/* 32,768 whole numbers stored and audited; the next new whole number aborts (SIGABRT, with the message), by entry and
 * by an operation, in a forked child, while the parent's class is untouched. */
static q32_class *fill(void) {
    q32_class *c = q32_create();
    sk_handle h;
    /* 0/1 first (0 and 1 at indices 0 and 1), then k/1 for k = 1 .. 32767 (k at index k): 32,768 whole numbers, and
     * the handle k holds k/1 */
    CHECK_EQ(q32_enter(c, 0, 1, &h), SK_OK);
    for (sk_int k = 1; k < 32768; k++) CHECK_EQ(q32_enter(c, k, 1, &h), SK_OK);
    return c;
}

static void expect_abort(q32_class *c, int how) {
    fflush(stdout);
    fflush(stderr);
    int fd[2];
    CHECK(pipe(fd) == 0);
    pid_t p = fork();
    if (p == 0) {
        close(fd[0]);
        dup2(fd[1], 2);
        sk_handle h;
        if (how == 0) q32_enter(c, 40000, 1, &h);
        else q32_add(c, 32767, 32767, &h); /* 32767/1 + 32767/1 = 65534/1: a new whole number */
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
    CHECK(strstr(msg, "capacity of 32,768 whole numbers exceeded") != NULL);
}

static void test_capacity(void) {
    q32_class *c = fill();
    CHECK_EQ(q32_wholes(c), 32768);
    CHECK_EQ(q32_audit(c, NULL), SK_OK);
    sk_handle h;
    /* found whole numbers still store: 32767 - 1 = 32766/1, both found */
    CHECK_EQ(q32_sub(c, 32767, 1, &h), SK_OK);
    CHECK_EQ(q32_wholes(c), 32768);
    int neg;
    uint32_t i, j;
    CHECK_EQ(q32_t_rat(c, 32767, &neg, &i, &j), SK_OK);
    CHECK_EQ(i, 32767); /* the largest index */
    uint32_t n = q32_size(c);
    expect_abort(c, 0);
    expect_abort(c, 1);
    CHECK_EQ(q32_size(c), n);
    CHECK_EQ(q32_wholes(c), 32768);
    CHECK_EQ(q32_audit(c, NULL), SK_OK);
    /* index 32,767 with the tag -: the field 0xFFFF is a valid rational */
    sk_handle m;
    sk_int top = qwhole(c, 32767);
    CHECK_EQ(top, 32767);
    CHECK_EQ(q32_enter(c, -top, 1, &m), SK_OK);
    CHECK_EQ(q32_t_rat(c, m, &neg, &i, &j), SK_OK);
    CHECK(neg == 1 && i == 32767);
    sk_int a, b;
    q32_read(c, m, &a, &b);
    CHECK(a == -top && b == 1);
    CHECK_EQ(q32_audit(c, NULL), SK_OK);
    q32_destroy(c);
}

/* ----------------------------------------------------------------------------------------- builds */

static void test_builds(void) {
    CHECK_EQ(sk_build_width(), 64);
    CHECK_EQ(sk_build_counting(), 1);
    CHECK_EQ(sk_build_audit(), 1);
    r2_profile p;
    q32_class *c = q32_create();
    CHECK_EQ(q32_profile(c, &p), SK_ENOTBUILT); /* the profile is the profiling build's only */
    q32_destroy(c);
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
    free(SN0);
    free(SN1);
    printf("r2_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
