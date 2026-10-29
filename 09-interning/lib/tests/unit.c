/* unit.c: unit tests of layer 1 (spec §14.3 item 1, §17.3 items 4-5), L1 and L1-opt, with the control where a test
 * applies to it.  Test build only. */
#include "t_common.h"

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
    sk_status s = lw_audit(R, c, 0, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit_changed: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
    s = lw_audit(R, c, 1, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
}

/* totals over core and store of a phase */
static uint64_t tot(const sk_counters *b, int ph, size_t off) {
    return *(const uint64_t *)((const char *)&b->c[ph][SK_CORE] + off) +
           *(const uint64_t *)((const char *)&b->c[ph][SK_STORE] + off);
}
#define BT(b, ph, f) tot(&(b), (ph), offsetof(sk_count, f))
static uint64_t xtot(const l1_xcounters *x, int ph, size_t off) {
    return *(const uint64_t *)((const char *)&x->c[ph][SK_CORE] + off) +
           *(const uint64_t *)((const char *)&x->c[ph][SK_STORE] + off);
}
#define XT(x, ph, f) xtot(&(x), (ph), offsetof(l1_xcount, f))

static const row *LROWS[2] = {&ROW_L1, &ROW_L1O};

/* ----------------------------------------------------------------------------------------- the worked example */

/* Paper §5.5, exactly: the four entries and their indices; the eight operations, in the paper's order, on the
 * original four values, with the case, the gcds made, the result and the indices appended or found. */
static void test_worked_example(void) {
    for (int r = 0; r < 2; r++) {
        const row *R = LROWS[r];
        void *c = R->create();
        sk_handle x0 = ent(R, c, 1, 6), x1 = ent(R, c, -5, 6), x2 = ent(R, c, 5, 7), x3 = ent(R, c, 6, 7);
        const sk_int W0[4] = {1, 6, 5, 7};
        CHECK_EQ(lw_wholes(R, c), 4);
        for (uint32_t k = 0; k < 4; k++) CHECK_EQ(lw_whole(R, c, k), W0[k]);
        const struct { sk_handle h; int neg; uint32_t i, j; } E[4] = {{x0, 0, 0, 1}, {x1, 1, 2, 1}, {x2, 0, 2, 3}, {x3, 0, 1, 3}};
        for (int k = 0; k < 4; k++) {
            int neg;
            uint32_t i, j;
            lw_rat(R, c, E[k].h, &neg, &i, &j);
            CHECK_EQ(neg, E[k].neg);
            CHECK_EQ(i, E[k].i);
            CHECK_EQ(j, E[k].j);
        }
        struct {
            int op;
            sk_handle x, y;
            int kase, gcds;
            sk_int m, d;
            int neg;
            uint32_t i, j, W; /* stored as; whole numbers after */
        } T[8] = {
            {SK_PH_ADD, x0, x1, L1_ADD_DEN, 1, -2, 3, 1, 4, 5, 6},
            {SK_PH_MUL, x0, x1, L1_MUL_DEN, 0, -5, 36, 1, 2, 6, 7},
            {SK_PH_MUL, x1, x2, L1_MUL_NUM, 0, -25, 42, 1, 7, 8, 9},
            {SK_PH_MUL, x0, x3, L1_MUL_CROSS_CB, 1, 1, 7, 0, 0, 3, 9},
            {SK_PH_DIV, x2, x1, L1_DIV_NUM, 1, -6, 7, 1, 1, 3, 9},
            {SK_PH_DIV, x3, x2, L1_DIV_DEN, 1, 6, 5, 0, 1, 2, 9},
            {SK_PH_DIV, x3, x0, L1_DIV_CROSS_AD, 0, 36, 7, 0, 6, 3, 9},
            {SK_PH_ADD, x1, x3, L1_ADD_CROSS, 0, 1, 42, 0, 0, 8, 9},
        };
        for (int k = 0; k < 8; k++) {
            sk_counters b0, b1;
            l1_xcounters q0, q1;
            lw_counters(R, c, &b0, &q0);
            sk_handle h = opk(R, c, T[k].op, T[k].x, T[k].y);
            lw_counters(R, c, &b1, &q1);
            val(R, c, h, T[k].m, T[k].d);
            CHECK_EQ(xsum_case(&q1, T[k].op, T[k].kase) - xsum_case(&q0, T[k].op, T[k].kase), 1);
            /* the gcds of the paper's table: made, or (L1-opt) skipped by O-3 because an argument is 1 */
            uint64_t g = BT(b1, T[k].op, gcd) - BT(b0, T[k].op, gcd) + XT(q1, T[k].op, o3_gcd_skipped) -
                         XT(q0, T[k].op, o3_gcd_skipped);
            CHECK_EQ(g, T[k].gcds);
            int neg;
            uint32_t i, j;
            lw_rat(R, c, h, &neg, &i, &j);
            CHECK_EQ(neg, T[k].neg);
            CHECK_EQ(i, T[k].i);
            CHECK_EQ(j, T[k].j);
            CHECK_EQ(lw_wholes(R, c), T[k].W);
        }
        /* the indices appended along the way: 2, 3 at 4, 5; 36 at 6; 25, 42 at 7, 8 */
        const sk_int W1[9] = {1, 6, 5, 7, 2, 3, 36, 25, 42};
        for (uint32_t k = 0; k < 9; k++) CHECK_EQ(lw_whole(R, c, k), W1[k]);
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
    {SK_PH_ADD, 1, 6, 1, 6, L1_ADD_DEN, 1, 3},       /* 1/6 + 1/6 = 1/3 */
    {SK_PH_ADD, 7, 6, 7, 10, L1_ADD_NUM, 28, 15},    /* 7/6 + 7/10 = 28/15 */
    {SK_PH_ADD, 2, 3, 5, 2, L1_ADD_CROSS, 19, 6},    /* 2/3 + 5/2 = 19/6 */
    {SK_PH_ADD, 1, 6, 1, 10, L1_ADD_NUM, 4, 15},     /* 1/6 + 1/10 (paper §2.1), shared numerator 1 */
    {SK_PH_ADD, 5, 6, 7, 10, L1_ADD_GEN, 23, 15},    /* general, g = 2 */
    {SK_PH_ADD, 2, 3, 1, 5, L1_ADD_GEN, 13, 15},     /* general, g = 1 */
    {SK_PH_ADD, 3, 5, 3, 7, L1_ADD_NUM, 36, 35},     /* num, g = 1 */
    {SK_PH_SUB, 7, 6, 7, 10, L1_ADD_NUM, 7, 15},     /* 7/6 - 7/10: shared numerator, opposite tags */
    {SK_PH_MUL, 3, 7, 2, 7, L1_MUL_DEN, 6, 49},      /* 3/7 * 2/7 = 6/49 */
    {SK_PH_MUL, 3, 4, 3, 5, L1_MUL_NUM, 9, 20},      /* 3/4 * 3/5 = 9/20 */
    {SK_PH_MUL, 4, 9, 3, 4, L1_MUL_CROSS_AD, 1, 3},  /* 4/9 * 3/4 = 1/3 */
    {SK_PH_MUL, 3, 4, 4, 9, L1_MUL_CROSS_CB, 1, 3},  /* 3/4 * 4/9 = 1/3 */
    {SK_PH_MUL, 5, 7, 7, 5, L1_MUL_BOTH, 1, 1},      /* a/b * b/a = 1 */
    {SK_PH_MUL, -5, 7, 7, 5, L1_MUL_BOTH, -1, 1},    /* = -1 */
    {SK_PH_MUL, 4, 9, 3, 8, L1_MUL_GEN, 1, 6},       /* 4/9 * 3/8 = 1/6 */
    {SK_PH_DIV, 1, 2, 1, 4, L1_DIV_NUM, 2, 1},       /* 1/2 / 1/4 = 2 */
    {SK_PH_DIV, 3, 10, 9, 10, L1_DIV_DEN, 1, 3},     /* 3/10 / 9/10 = 1/3 */
    {SK_PH_DIV, 2, 3, 5, 2, L1_DIV_CROSS_AD, 4, 15}, /* 2/3 / 5/2 = 4/15 */
    {SK_PH_DIV, 2, 3, 3, 5, L1_DIV_CROSS_CB, 10, 9}, /* 2/3 / 3/5 = 10/9 */
    {SK_PH_DIV, 2, 9, -4, 3, L1_DIV_GEN, -1, 6},     /* 2/9 / (-4/3) = -1/6 */
    {SK_PH_DIV, 5, 7, 5, 7, L1_DIV_BOTH, 1, 1},      /* x / x = 1 */
    {SK_PH_DIV, 5, 7, -5, 7, L1_DIV_BOTH, -1, 1},
};

static void run_example(const row *R, const ex *e, int force) {
    void *c = R->create();
    lw_force_general(R, c, force);
    sk_handle x = ent(R, c, e->xm, e->xd), y = ent(R, c, e->ym, e->yd);
    l1_xcounters q;
    sk_handle h = opk(R, c, e->op, x, y);
    val(R, c, h, e->m, e->d);
    lw_counters(R, c, NULL, &q);
    if (R->kind != 2) {
        int k = e->kase;
        if (force)
            k = e->op == SK_PH_MUL ? L1_MUL_GEN : e->op == SK_PH_DIV ? L1_DIV_GEN : L1_ADD_GEN;
        CHECK_EQ(xsum_case(&q, e->op, k), 1);
        CHECK_EQ(XT(q, e->op, idx_cmp), 4);
    }
    audits_ok(R, c);
    R->destroy(c);
}

static void test_every_case(void) {
    const row *rows[3] = {&ROW_L1, &ROW_L1O, &ROW_CTL};
    for (size_t k = 0; k < sizeof EXAMPLES / sizeof EXAMPLES[0]; k++)
        for (int r = 0; r < 3; r++) {
            run_example(rows[r], &EXAMPLES[k], 0);
            if (r < 2) run_example(rows[r], &EXAMPLES[k], 1); /* shortcuts change nothing (§14.3) */
        }
}

/* every example with every combination of tags, against the exact reference, with the case unchanged */
static void test_signs(void) {
    for (size_t k = 0; k < sizeof EXAMPLES / sizeof EXAMPLES[0]; k++) {
        const ex *e = &EXAMPLES[k];
        for (int sx = 0; sx < 2; sx++)
            for (int sy = 0; sy < 2; sy++)
                for (int r = 0; r < 3; r++) {
                    const row *R = r == 0 ? &ROW_L1 : r == 1 ? &ROW_L1O : &ROW_CTL;
                    sk_int xm = sx ? -e->xm : e->xm, ym = sy ? -e->ym : e->ym;
                    void *c = R->create();
                    sk_handle x = ent(R, c, xm, e->xd), y = ent(R, c, ym, e->yd);
                    sk_handle h = opk(R, c, e->op, x, y);
                    ref_q want = ref_op(e->op, ref_make(xm, e->xd), ref_make(ym, e->yd));
                    val(R, c, h, (sk_int)want.n, (sk_int)want.d);
                    if (r < 2) {
                        l1_xcounters q;
                        lw_counters(R, c, NULL, &q);
                        CHECK_EQ(xsum_case(&q, e->op, e->kase), 1);
                    }
                    audits_ok(R, c);
                    R->destroy(c);
                }
    }
    /* x - x = 0, stored as +0/1, for each sign of x */
    for (int r = 0; r < 2; r++)
        for (int s = 0; s < 2; s++) {
            const row *R = LROWS[r];
            void *c = R->create();
            sk_handle x = ent(R, c, s ? -5 : 5, 6);
            sk_handle h = opk(R, c, SK_PH_SUB, x, x);
            val(R, c, h, 0, 1);
            int neg;
            uint32_t i, j;
            lw_rat(R, c, h, &neg, &i, &j);
            CHECK_EQ(neg, 0);
            CHECK_EQ(lw_whole(R, c, i), 0);
            CHECK_EQ(lw_whole(R, c, j), 1);
            sk_handle mx = ent(R, c, s ? 5 : -5, 6);
            h = opk(R, c, SK_PH_ADD, x, mx); /* x + (-x) */
            val(R, c, h, 0, 1);
            lw_rat(R, c, h, &neg, &i, &j);
            CHECK_EQ(neg, 0);
            audits_ok(R, c);
            R->destroy(c);
        }
}

/* ----------------------------------------------------------------------------------------- zero */

static void test_zero(void) {
    const row *rows[3] = {&ROW_L1, &ROW_L1O, &ROW_CTL};
    for (int r = 0; r < 3; r++) {
        const row *R = rows[r];
        void *c = R->create();
        sk_handle z = ent(R, c, 0, 5), y = ent(R, c, -3, 4), one = ent(R, c, 1, 1), z2 = ent(R, c, 0, -7);
        val(R, c, z, 0, 1);
        val(R, c, z2, 0, 1);
        for (int op = SK_PH_ADD; op <= SK_PH_DIV; op++) {
            ref_q Z = {0, 1}, Y = {-3, 4}, O = {1, 1};
            sk_handle h = opk(R, c, op, z, y); /* zero first */
            ref_q w = ref_op(op, Z, Y);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            h = opk(R, c, op, z, one);
            w = ref_op(op, Z, O);
            val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            if (op != SK_PH_DIV) { /* zero second */
                h = opk(R, c, op, y, z);
                w = ref_op(op, Y, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
                h = opk(R, c, op, one, z);
                w = ref_op(op, O, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
                h = opk(R, c, op, z, z2);
                w = ref_op(op, Z, Z);
                val(R, c, h, (sk_int)w.n, (sk_int)w.d);
            }
        }
        /* 0/1 / 0/1 and y / 0: division by zero, nothing stored */
        uint32_t n = R->size(c);
        sk_handle h = 77;
        CHECK_EQ(R->op[SK_PH_DIV](c, z, z2, &h), SK_EDIVZERO);
        CHECK_EQ(R->op[SK_PH_DIV](c, y, z, &h), SK_EDIVZERO);
        CHECK_EQ(R->size(c), n);
        /* zero results: y - y and y + (-y) */
        sk_handle my = ent(R, c, 3, 4);
        val(R, c, opk(R, c, SK_PH_ADD, y, my), 0, 1);
        val(R, c, opk(R, c, SK_PH_SUB, my, my), 0, 1);
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- overlaps */

/* Operands for which several comparisons are equal take the case §7 names. */
static void test_overlaps(void) {
    static const ex O[] = {
        {SK_PH_ADD, 1, 1, 1, 1, L1_ADD_CROSS, 2, 1},       /* cross, den and num: cross */
        {SK_PH_ADD, 1, 1, 1, 3, L1_ADD_CROSS, 4, 3},       /* num and cross: cross */
        {SK_PH_ADD, 3, 5, 3, 5, L1_ADD_DEN, 6, 5},         /* den and num: den */
        {SK_PH_MUL, 3, 5, 3, 5, L1_MUL_DEN, 9, 25},        /* den and num: den */
        {SK_PH_MUL, 1, 1, 1, 1, L1_MUL_BOTH, 1, 1},        /* everything: both */
        {SK_PH_MUL, 2, 1, 1, 2, L1_MUL_BOTH, 1, 1},        /* both */
        {SK_PH_MUL, 1, 3, 1, 1, L1_MUL_NUM, 1, 3},         /* num and cross_ad: num */
        {SK_PH_MUL, 3, 1, 5, 3, L1_MUL_CROSS_AD, 5, 1},    /* cross_ad */
        {SK_PH_MUL, 1, 1, 3, 1, L1_MUL_DEN, 3, 1},         /* den and cross_ad: den */
        {SK_PH_DIV, 1, 1, 1, 1, L1_DIV_BOTH, 1, 1},        /* everything: both */
        {SK_PH_DIV, 2, 1, 1, 2, L1_DIV_CROSS_AD, 4, 1},    /* cross_ad and cross_cb: cross_ad */
        {SK_PH_DIV, 1, 3, 1, 1, L1_DIV_CROSS_AD, 1, 3},    /* num and cross_ad: cross_ad */
        {SK_PH_DIV, 3, 1, 1, 1, L1_DIV_CROSS_CB, 3, 1},    /* den and cross_cb: cross_cb */
        {SK_PH_DIV, 3, 7, 3, 5, L1_DIV_NUM, 5, 7},         /* num */
    };
    for (size_t k = 0; k < sizeof O / sizeof O[0]; k++)
        for (int r = 0; r < 2; r++) run_example(LROWS[r], &O[k], 0);
    /* and systematically: random operands over small whole numbers, the case counter against §7 on magnitudes */
    for (int r = 0; r < 2; r++) {
        const row *R = LROWS[r];
        for (int t = 0; t < 4000; t++) {
            void *c = R->create();
            sk_int a = 1 + (sk_int)rndn(6), b = 1 + (sk_int)rndn(6), cc = (sk_int)rndn(6), d = 1 + (sk_int)rndn(6);
            int op = 1 + (int)rndn(4);
            if (op == SK_PH_DIV && cc == 0) cc = 1;
            sk_handle x = ent(R, c, rndn(2) ? -a : a, b), y = ent(R, c, rndn(2) ? -cc : cc, d);
            ref_q X = ref_make(a, b), Y = ref_make(cc, d);
            int want = predict_case(op, (uint64_t)ref_abs(X.n), (uint64_t)X.d, (uint64_t)ref_abs(Y.n), (uint64_t)Y.d);
            l1_xcounters q;
            opk(R, c, op, x, y);
            lw_counters(R, c, NULL, &q);
            CHECK_EQ(xsum_case(&q, op, want), 1);
            R->destroy(c);
        }
    }
}

/* ----------------------------------------------------------------------------------------- entry, errors */

static void test_entry(void) {
    const row *rows[3] = {&ROW_L1, &ROW_L1O, &ROW_CTL};
    for (int r = 0; r < 3; r++) {
        const row *R = rows[r];
        void *c = R->create();
        val(R, c, ent(R, c, 3, -6), -1, 2);
        val(R, c, ent(R, c, -3, -6), 1, 2);
        val(R, c, ent(R, c, 12, 18), 2, 3);
        val(R, c, ent(R, c, INT64_MAX, INT64_MAX), 1, 1);
        val(R, c, ent(R, c, -INT64_MAX, 1), -INT64_MAX, 1);
        sk_handle h = 99;
        uint32_t n = R->size(c);
        CHECK_EQ(R->enter(c, 5, 0, &h), SK_EDIVZERO);
        CHECK_EQ(R->enter(c, INT64_MIN, 3, &h), SK_EINVAL);
        CHECK_EQ(R->enter(c, 3, INT64_MIN, &h), SK_EINVAL);
        CHECK_EQ(R->enter(c, INT64_MIN, 0, &h), SK_EINVAL);
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

#define P32a 4294967297LL  /* 2^32 + 1 */
#define P32b 4294967299LL  /* 2^32 + 3 */
#define P62 4611686018427387907LL /* 2^62 + 3 */
#define BIGa 1500000000000000001LL
#define BIGc 1000000000000000001LL

static const ovf OVF[] = {
    {SK_PH_ADD, P32a, 3, 5, P32a, "add cross: m_x*d"},
    {SK_PH_ADD, 2, 1099511627777LL, 1099511627777LL, 2, "add cross: m_y*b"},
    {SK_PH_ADD, 3037000499LL, 3, 3037000500LL, 3037000499LL, "add cross: m_x*d + m_y*b"},
    {SK_PH_ADD, 2147483648LL, P32a, 1, 2147483648LL, "add cross: b*d"},
    {SK_PH_ADD, 4611686018427387905LL, 3, 4611686018427387905LL, 3, "add den: t = m_x + m_y"},
    {SK_PH_ADD, 1, 4611686018427387905LL, 1, 4611686018427387907LL, "add num: t = s_x*d' + s_y*b'"},
    {SK_PH_ADD, 4611686018427387903LL, 2, 4611686018427387903LL, 4, "add num: a*(t/h)"},
    {SK_PH_ADD, 1, P32a, 1, P32b, "add num: b'*(d/h)"},
    {SK_PH_ADD, P62, 3, 1, 5, "add general: m_x*d"},
    {SK_PH_ADD, 1, 3, P62, 5, "add general: m_y*b"},
    {SK_PH_ADD, BIGa, 3, BIGc, 5, "add general: m_x*d + m_y*b"},
    {SK_PH_ADD, 1, P32a, 2, P32b, "add general: b*d"},
    {SK_PH_ADD, P62, 6, 1, 10, "add general: m_x*d'"},
    {SK_PH_ADD, 1, 6, P62, 10, "add general: m_y*b'"},
    {SK_PH_ADD, BIGa, 6, BIGc, 10, "add general: t = m_x*d' + m_y*b'"},
    {SK_PH_ADD, 1, 2 * P32a, 3, 2 * P32b, "add general: b'*(d/h)"},
    {SK_PH_MUL, P32a, 3, P32b, 3, "multiply den: m_x*m_y"},
    {SK_PH_MUL, 1, P32a, 2, P32a, "multiply den: d*d"},
    {SK_PH_MUL, P32a, 3, P32a, 5, "multiply num: a*a"},
    {SK_PH_MUL, 2, P32a, 2, P32b, "multiply num: b*d"},
    {SK_PH_MUL, P32a, 3, P32b, 5, "multiply general: (m_x/g_1)*(m_y/g_2)"},
    {SK_PH_MUL, 2, P32a, 3, P32b, "multiply general: (b/g_2)*(d/g_1)"},
    {SK_PH_DIV, P32a, 3, 5, P32a, "divide cross_ad: a*a"},
    {SK_PH_DIV, 2, P32a, P32b, 2, "divide cross_ad: b*c"},
    {SK_PH_DIV, P32a, 2, 2, P32b, "divide cross_cb: a*d"},
    {SK_PH_DIV, 1, P32a, P32a, 5, "divide cross_cb: b*b"},
    {SK_PH_DIV, P32a, 3, 5, P32b, "divide general: (a/g_1)*(d/g_2)"},
    {SK_PH_DIV, 3, P32a, P32b, 5, "divide general: (b/g_2)*(c/g_1)"},
    {SK_PH_SUB, 4611686018427387905LL, 3, -4611686018427387905LL, 3, "add den: t = m_x + m_y"},
};

static void test_overflow(void) {
    for (size_t k = 0; k < sizeof OVF / sizeof OVF[0]; k++) {
        const ovf *o = &OVF[k];
        for (int r = 0; r < 2; r++) {
            const row *R = LROWS[r];
            void *c = R->create();
            sk_handle x = ent(R, c, o->xm, o->xd), y = ent(R, c, o->ym, o->yd), h = 12345;
            audits_ok(R, c);
            uint32_t n = R->size(c), w = lw_wholes(R, c);
            sk_status s = R->op[o->op](c, x, y, &h);
            CHECK_EQ(s, SK_EOVERFLOW);
            sk_overflow rec;
            CHECK_EQ(lw_overflow(R, c, &rec), SK_OK);
            if (!rec.step || strcmp(rec.step, o->step))
                fprintf(stderr, "  %s: overflow at '%s', expected '%s'\n", R->name, rec.step ? rec.step : "-", o->step);
            CHECK(rec.step && !strcmp(rec.step, o->step));
            CHECK_EQ(rec.x, x);
            CHECK_EQ(rec.y, y);
            CHECK_EQ(rec.op, o->op);
            CHECK_EQ(R->size(c), n);     /* nothing stored */
            CHECK_EQ(lw_wholes(R, c), w); /* nothing interned */
            audits_ok(R, c);             /* the class is valid */
            /* and still usable */
            sk_handle sh = ent(R, c, 1, 2);
            val(R, c, opk(R, c, SK_PH_ADD, sh, sh), 1, 1);
            R->destroy(c);
        }
    }
}

/* ----------------------------------------------------------------------------------------- the store */

static void test_store(void) {
    for (int r = 0; r < 2; r++) {
        const row *R = LROWS[r];
        void *c = R->create();
        sk_handle a = ent(R, c, 2, 3), b = ent(R, c, 3, 2); /* W: 2, 3 */
        CHECK_EQ(lw_wholes(R, c), 2);
        sk_handle h = opk(R, c, SK_PH_MUL, a, a); /* 4/9: both new */
        CHECK_EQ(lw_wholes(R, c), 4);
        h = opk(R, c, SK_PH_DIV, a, b); /* 4/9 again: both found */
        CHECK_EQ(lw_wholes(R, c), 4);
        int n1, n2;
        uint32_t i1, j1, i2, j2;
        lw_rat(R, c, h, &n1, &i1, &j1);
        lw_rat(R, c, h - 1, &n2, &i2, &j2);
        CHECK(h != h - 1 && n1 == n2 && i1 == i2 && j1 == j2); /* equal results: two rationals, same indices */
        CHECK_EQ(R->size(c), 4);
        h = opk(R, c, SK_PH_ADD, a, a); /* 4/3: one found (3), one new (4)? 4 found, 3 found */
        val(R, c, h, 4, 3);
        CHECK_EQ(lw_wholes(R, c), 4);
        h = opk(R, c, SK_PH_ADD, b, b); /* 3/1: 3 found, 1 new */
        val(R, c, h, 3, 1);
        CHECK_EQ(lw_wholes(R, c), 5);
        /* growth of the lookup: S = 16 holds 8; the 9th whole number doubles it */
        sk_memory m;
        lw_memory(R, c, &m);
        uint64_t slot = R->kind == 0 ? 4 : 8;
        CHECK_EQ(m.item[1].used, 16 * slot);
        for (sk_int v = 10; v < 14; v++) ent(R, c, v, 1); /* 10..13: W = 9 */
        CHECK_EQ(lw_wholes(R, c), 9);
        lw_memory(R, c, &m);
        CHECK_EQ(m.item[1].used, 32 * slot);
        CHECK_EQ(m.item[0].used, 8 * 9);
        CHECK_EQ(m.item[2].used, 8 * R->size(c));
        double bpr = 8.0 + (8.0 * 9 + (double)slot * 32) / R->size(c);
        CHECK(m.used_per_rational > bpr - 1e-9 && m.used_per_rational < bpr + 1e-9);
        for (sk_int v = 20; v < 200; v++) ent(R, c, v, 1);
        audits_ok(R, c);
        R->destroy(c);
    }
}

/* ----------------------------------------------------------------------------------------- audits catch breaks */

static void test_audits_catch(void) {
    for (int k = 0; k < 6; k++) {
        l1_class *c = l1_create();
        sk_handle h;
        l1_enter(c, 2, 3, &h);
        l1_enter(c, 5, 7, &h);
        CHECK_EQ(l1_audit(c, NULL), SK_OK);
        switch (k) {
        case 0: l1_t_corrupt_rat(c, 0, 0, 9, 1); break;   /* index not below W */
        case 1: l1_t_corrupt_rat(c, 0, 0, 0, 0); break;   /* 2/2: not lowest terms */
        case 2: l1_t_corrupt_whole(c, 3, 2); break;       /* duplicate of index 0 */
        case 3: l1_t_corrupt_whole(c, 3, -7); break;      /* negative */
        case 4: l1_t_corrupt_rat(c, 1, 0, 0, 1); break;   /* 7 (index 3) used by no rational */
        case 5: l1_t_corrupt_rat(c, 1, 1, 0, 1); l1_t_corrupt_whole(c, 0, 0); break; /* -0/3 */
        }
        const char *why = NULL;
        CHECK_EQ(l1_audit(c, &why), SK_EAUDIT);
        l1_destroy(c);
    }
    /* the incremental audit sees a break in what changed */
    l1o_class *o = l1o_create();
    sk_handle h;
    l1o_enter(o, 2, 3, &h);
    CHECK_EQ(l1o_audit(o, NULL), SK_OK);
    l1o_enter(o, 5, 7, &h);
    l1o_t_corrupt_rat(o, 1, 0, 0, 0);
    CHECK_EQ(l1o_audit_changed(o, NULL), SK_EAUDIT);
    l1o_destroy(o);
}

/* ----------------------------------------------------------------------------------------- random workloads */

typedef struct wl {
    int n;
    int op[600];
    sk_int m[600], d[600]; /* entries */
    int x[600], y[600];    /* operations */
} wl;

/* entries over a small set of whole numbers (to meet every case), then operations on earlier values that stay
 * within the size bound */
static void make_wl(wl *w, int nin, int nops, int maxv, int ops_mask) {
    ref_q v[600];
    w->n = 0;
    for (int i = 0; i < nin; i++) {
        sk_int a = (sk_int)rndn((uint64_t)maxv + 1), b = 1 + (sk_int)rndn((uint64_t)maxv);
        if (rndn(5) == 0) a = 0;
        if (rndn(2)) a = -a;
        w->op[w->n] = 0;
        w->m[w->n] = a;
        w->d[w->n] = b;
        v[w->n++] = ref_make(a, b);
    }
    for (int t = 0; t < nops; t++) {
        for (int tries = 0; tries < 200; tries++) {
            int op = 1 + (int)rndn(4);
            if (!(ops_mask & (1 << op))) continue;
            int x = (int)rndn((uint64_t)w->n), y = (int)rndn((uint64_t)w->n);
            if (rndn(3) == 0) y = x;
            if (op == SK_PH_DIV && v[y].n == 0) continue;
            i128 hx = ref_abs(v[x].n) > v[x].d ? ref_abs(v[x].n) : v[x].d;
            i128 hy = ref_abs(v[y].n) > v[y].d ? ref_abs(v[y].n) : v[y].d;
            if (hx * hy > (i128)2147483647 * 2147483647) continue;
            w->op[w->n] = op;
            w->x[w->n] = x;
            w->y[w->n] = y;
            v[w->n++] = ref_op(op, v[x], v[y]);
            break;
        }
    }
}

static void run_wl(const row *R, void *c, const wl *w, sk_handle *hd) {
    for (int i = 0; i < w->n; i++) {
        if (w->op[i] == 0) hd[i] = ent(R, c, w->m[i], w->d[i]);
        else hd[i] = opk(R, c, w->op[i], hd[w->x[i]], hd[w->y[i]]);
    }
}

static bool same_store(const row *A, void *ca, const sk_handle *ha, const row *B, void *cb, const sk_handle *hb, int n) {
    bool ok = true;
    for (int i = 0; i < n; i++) {
        sk_int m1, d1, m2, d2;
        A->read(ca, ha[i], &m1, &d1);
        B->read(cb, hb[i], &m2, &d2);
        ok &= m1 == m2 && d1 == d2;
    }
    if (A->kind < 2 && B->kind < 2) {
        uint32_t w = lw_wholes(A, ca);
        ok &= w == lw_wholes(B, cb);
        for (uint32_t k = 0; ok && k < w; k++) ok &= lw_whole(A, ca, k) == lw_whole(B, cb, k);
    }
    return ok;
}

/* §14.3 "shortcuts change nothing": every operation sent to its general case gives the same stored values and the
 * same whole-number list.  Also the values against the exact reference and the control. */
static void test_shortcuts_change_nothing(void) {
    static wl w;
    static sk_handle h1[600], h2[600], h3[600];
    for (int t = 0; t < 300; t++) {
        make_wl(&w, 40, 200, t % 3 == 0 ? 6 : t % 3 == 1 ? 30 : 1000, 0x1e);
        for (int r = 0; r < 2; r++) {
            const row *R = LROWS[r];
            void *a = R->create(), *b = R->create(), *k = ROW_CTL.create();
            lw_force_general(R, b, 1);
            run_wl(R, a, &w, h1);
            run_wl(R, b, &w, h2);
            run_wl(&ROW_CTL, k, &w, h3);
            CHECK(same_store(R, a, h1, R, b, h2, w.n));
            CHECK(same_store(R, a, h1, &ROW_CTL, k, h3, w.n));
            audits_ok(R, a);
            audits_ok(R, b);
            R->destroy(a);
            R->destroy(b);
            ROW_CTL.destroy(k);
        }
    }
}

/* ----------------------------------------------------------------------------------------- L1-opt */

#define OXT(q, f) (XT(q, SK_PH_ENTRY, f) + XT(q, SK_PH_ADD, f) + XT(q, SK_PH_SUB, f) + XT(q, SK_PH_MUL, f) + XT(q, SK_PH_DIV, f))
#define OBT(b, f) (BT(b, SK_PH_ENTRY, f) + BT(b, SK_PH_ADD, f) + BT(b, SK_PH_SUB, f) + BT(b, SK_PH_MUL, f) + BT(b, SK_PH_DIV, f))

/* spec §17.3 item 3: L1-opt against L1, on one workload */
static void check_against_l1(const sk_counters *b1, const l1_xcounters *x1, const sk_counters *bo, const l1_xcounters *xo) {
    uint64_t sk = OXT(*xo, o2_skipped0) + OXT(*xo, o2_skipped1);
    for (int k = 0; k < L1_NO1; k++)
        for (int ph = 0; ph < SK_NPHASE; ph++)
            sk += xo->c[ph][SK_CORE].o1_skipped[k] + xo->c[ph][SK_STORE].o1_skipped[k];
    CHECK_EQ(OXT(*xo, intern_found) + sk, OXT(*x1, intern_found));
    CHECK_EQ(OXT(*xo, intern_appended), OXT(*x1, intern_appended));
    CHECK_EQ(OBT(*bo, gcd), OBT(*b1, gcd) - OXT(*xo, o3_gcd_skipped) - OXT(*xo, o5_gcd_skipped));
    CHECK_EQ(OBT(*bo, gcd_work), OBT(*b1, gcd_work) - OXT(*xo, o3_gcd_work) - OXT(*xo, o5_gcd_work));
    CHECK_EQ(OBT(*bo, div_exact), OBT(*b1, div_exact) - OXT(*xo, o4_div_skipped) - OXT(*xo, o5_div_skipped));
    CHECK_EQ(OBT(*bo, mul), OBT(*b1, mul) - OXT(*xo, o4_mul_skipped));
    CHECK_EQ(OBT(*bo, add), OBT(*b1, add) - OXT(*xo, o5_add_skipped));
    CHECK_EQ(OBT(*bo, lookup_search), OBT(*b1, lookup_search) - sk);
    for (int k = 0; k < L1_NCASE; k++)
        for (int ph = 0; ph < SK_NPHASE; ph++) CHECK_EQ(xsum_case(xo, ph, k), xsum_case(x1, ph, k));
}

static void test_switches(void) {
    static wl w;
    static sk_handle hl[600], ho[600];
    const unsigned masks[] = {L1O_ALL, L1O_ALL & ~L1O_T1, L1O_ALL & ~L1O_T2, L1O_ALL & ~L1O_T3, L1O_ALL & ~L1O_T4,
                              L1O_ALL & ~L1O_T5, L1O_ALL & ~L1O_T6, L1O_ALL & ~L1O_T7, 0};
    uint64_t seen_o[8] = {0};
    for (int t = 0; t < 200; t++) {
        make_wl(&w, 40, 200, t % 2 ? 5 : 40, 0x1e);
        l1_class *l = l1_create();
        run_wl(&ROW_L1, l, &w, hl);
        sk_counters b1, bo;
        l1_xcounters x1, xo;
        l1_counters(l, &b1, &x1);
        for (size_t m = 0; m < sizeof masks / sizeof masks[0]; m++) {
            l1o_class *o = l1o_create();
            l1o_t_tricks(o, masks[m]);
            l1o_reserve(o, (uint32_t)w.n);
            run_wl(&ROW_L1O, o, &w, ho);
            CHECK(same_store(&ROW_L1, l, hl, &ROW_L1O, o, ho, w.n));
            audits_ok(&ROW_L1O, o);
            l1o_counters(o, &bo, &xo);
            check_against_l1(&b1, &x1, &bo, &xo);
            if (masks[m] == 0) { /* all off: the counters equal L1's */
                CHECK(memcmp(&bo, &b1, sizeof bo) == 0);
                CHECK(memcmp(&xo, &x1, sizeof xo) == 0);
            }
            if (masks[m] == L1O_ALL) {
                seen_o[1] += OXT(xo, o1_skipped[0]) + OXT(xo, o1_skipped[1]) + OXT(xo, o1_skipped[2]) +
                             OXT(xo, o1_skipped[3]) + OXT(xo, o1_skipped[4]);
                seen_o[2] += OXT(xo, o2_skipped0) + OXT(xo, o2_skipped1);
                seen_o[3] += OXT(xo, o3_gcd_skipped);
                seen_o[4] += OXT(xo, o4_div_skipped) + OXT(xo, o4_mul_skipped);
                seen_o[5] += OXT(xo, o5_ops);
                seen_o[6] += OXT(xo, o6_reads_avoided);
                seen_o[7] += OXT(xo, o7_growths_avoided);
            }
            l1o_destroy(o);
        }
        l1_destroy(l);
    }
    for (int k = 1; k < 8; k++) {
        if (!seen_o[k]) fprintf(stderr, "  trick O-%d never fired in the random workloads\n", k);
        CHECK(seen_o[k] > 0);
    }
}

/* one operation in a fresh L1-opt class; returns the counters' change over the operation */
static sk_handle o_run(l1o_class *c, int op, sk_handle x, sk_handle y, l1_xcounters *dx) {
    l1_xcounters a, b;
    l1o_counters(c, NULL, &a);
    sk_handle h = opk(&ROW_L1O, c, op, x, y);
    l1o_counters(c, NULL, &b);
    for (int ph = 0; ph < SK_NPHASE; ph++)
        for (int p = 0; p < SK_NPART; p++) {
            uint64_t *pa = (uint64_t *)&a.c[ph][p], *pb = (uint64_t *)&b.c[ph][p], *pd = (uint64_t *)&dx->c[ph][p];
            for (size_t i = 0; i < sizeof(l1_xcount) / 8; i++) pd[i] = pb[i] - pa[i];
        }
    return h;
}

static void test_o1(void) {
    /* each O-1 case with h = 1 and with h != 1 */
    static const struct { int op; sk_int xm, xd, ym, yd; int o1, h1; sk_int m, d; } T[] = {
        {SK_PH_ADD, 1, 6, 5, 6, L1_O1_ADD_DEN, 0, 1, 1},   /* h = gcd(6, 6) = 6 */
        {SK_PH_ADD, 1, 7, 3, 7, L1_O1_ADD_DEN, 1, 4, 7},   /* h = 1: denominator j_x */
        {SK_PH_ADD, 3, 7, -3, 7, L1_O1_ADD_DEN, 0, 0, 1},  /* zero by O-5, not O-1 */
        {SK_PH_MUL, 4, 9, 3, 4, L1_O1_MUL_CROSS_AD, 0, 1, 3},
        {SK_PH_MUL, 4, 9, 5, 4, L1_O1_MUL_CROSS_AD, 1, 5, 9},
        {SK_PH_MUL, 3, 4, 4, 9, L1_O1_MUL_CROSS_CB, 0, 1, 3},
        {SK_PH_MUL, 5, 4, 4, 9, L1_O1_MUL_CROSS_CB, 1, 5, 9},
        {SK_PH_DIV, 1, 2, 1, 4, L1_O1_DIV_NUM, 0, 2, 1},
        {SK_PH_DIV, 2, 9, -2, 5, L1_O1_DIV_NUM, 1, -5, 9},
        {SK_PH_DIV, 3, 10, 9, 10, L1_O1_DIV_DEN, 0, 1, 3},
        {SK_PH_DIV, 3, 10, 7, 10, L1_O1_DIV_DEN, 1, 3, 7},
        {SK_PH_ADD, 0, 1, 0, 1, L1_O1_ADD_DEN, 1, 0, 1},   /* 0 + 0: a zero result over the known 1 */
        {SK_PH_MUL, 0, 1, 1, 5, L1_O1_MUL_CROSS_CB, 0, 0, 1}, /* 0 * 1/5: h = 5 */
    };
    for (size_t k = 0; k < sizeof T / sizeof T[0]; k++) {
        l1o_class *c = l1o_create();
        sk_handle x = ent(&ROW_L1O, c, T[k].xm, T[k].xd), y = ent(&ROW_L1O, c, T[k].ym, T[k].yd);
        l1_xcounters d;
        uint32_t w = l1o_wholes(c);
        sk_handle h = o_run(c, T[k].op, x, y, &d);
        val(&ROW_L1O, c, h, T[k].m, T[k].d);
        uint64_t sk = d.c[T[k].op][SK_STORE].o1_skipped[T[k].o1];
        CHECK_EQ(sk, T[k].h1 ? 2 - (T[k].o1 == L1_O1_ADD_DEN) : 0);
        if (T[k].h1 && T[k].o1 != L1_O1_ADD_DEN) CHECK_EQ(l1o_wholes(c), w); /* nothing looked up, nothing new */
        audits_ok(&ROW_L1O, c);
        l1o_destroy(c);
    }
}

static void test_o2(void) {
    l1o_class *c = l1o_create();
    l1_xcounters d;
    sk_handle a = ent(&ROW_L1O, c, 2, 3), b = ent(&ROW_L1O, c, 3, 2);
    /* the +1 of a both case before 1 is interned: both whole numbers looked up; the denominator 1 is then known */
    sk_handle h = o_run(c, SK_PH_MUL, a, b, &d);
    val(&ROW_L1O, c, h, 1, 1);
    CHECK_EQ(d.c[SK_PH_MUL][SK_STORE].o2_skipped1, 1);
    CHECK_EQ(d.c[SK_PH_MUL][SK_STORE].intern_appended, 1);
    /* after: both skipped */
    h = o_run(c, SK_PH_MUL, b, a, &d);
    CHECK_EQ(d.c[SK_PH_MUL][SK_STORE].o2_skipped1, 2);
    CHECK_EQ(d.c[SK_PH_MUL][SK_STORE].intern_found + d.c[SK_PH_MUL][SK_STORE].intern_appended, 0);
    /* an integer result: the denominator 1 skipped; a zero: 0 interned once, then skipped */
    sk_handle e = ent(&ROW_L1O, c, 4, 2);
    h = o_run(c, SK_PH_ADD, e, e, &d);
    val(&ROW_L1O, c, h, 4, 1);
    CHECK_EQ(d.c[SK_PH_ADD][SK_STORE].o2_skipped1 + d.c[SK_PH_ADD][SK_STORE].o1_skipped[L1_O1_ADD_DEN], 1);
    h = o_run(c, SK_PH_SUB, a, a, &d);
    CHECK_EQ(d.c[SK_PH_SUB][SK_STORE].o2_skipped0, 0);
    CHECK_EQ(d.c[SK_PH_SUB][SK_STORE].intern_appended, 1);
    h = o_run(c, SK_PH_SUB, b, b, &d);
    CHECK_EQ(d.c[SK_PH_SUB][SK_STORE].o2_skipped0, 1);
    CHECK_EQ(d.c[SK_PH_SUB][SK_STORE].o2_skipped1, 1);
    audits_ok(&ROW_L1O, c);
    l1o_destroy(c);
}

static void test_o3_o4_o5(void) {
    /* integer operands in every operation: gcds with a known 1 skipped, multiplications and divisions by 1 skipped,
     * and the values the same as L1's */
    for (int op = SK_PH_ADD; op <= SK_PH_DIV; op++) {
        l1o_class *c = l1o_create();
        l1_class *l = l1_create();
        sk_handle o1 = ent(&ROW_L1O, c, 1, 1), l1h = ent(&ROW_L1, l, 1, 1); /* intern 1 */
        (void)o1, (void)l1h;
        sk_handle x = ent(&ROW_L1O, c, 6, 1), y = ent(&ROW_L1O, c, 4, 9);
        sk_handle lx = ent(&ROW_L1, l, 6, 1), ly = ent(&ROW_L1, l, 4, 9);
        l1_xcounters d;
        sk_counters b0, b1, lb0, lb1;
        l1o_counters(c, &b0, NULL);
        l1_counters(l, &lb0, NULL);
        sk_handle h = o_run(c, op, x, y, &d), hl = opk(&ROW_L1, l, op, lx, ly);
        sk_handle h2 = o_run(c, op, y, x, &d), hl2 = opk(&ROW_L1, l, op, ly, lx);
        l1o_counters(c, &b1, NULL);
        l1_counters(l, &lb1, NULL);
        sk_int m1, d1, m2, d2;
        l1o_read(c, h, &m1, &d1);
        l1_read(l, hl, &m2, &d2);
        CHECK(m1 == m2 && d1 == d2);
        l1o_read(c, h2, &m1, &d1);
        l1_read(l, hl2, &m2, &d2);
        CHECK(m1 == m2 && d1 == d2);
        l1_xcounters xo, xl;
        l1o_counters(c, NULL, &xo);
        l1_counters(l, NULL, &xl);
        CHECK(XT(xo, op, o3_gcd_skipped) >= 1);
        CHECK(XT(xo, op, o4_div_skipped) + XT(xo, op, o4_mul_skipped) >= 1);
        CHECK_EQ(BT(b1, op, gcd) - BT(b0, op, gcd), BT(lb1, op, gcd) - BT(lb0, op, gcd) - XT(xo, op, o3_gcd_skipped));
        audits_ok(&ROW_L1O, c);
        l1o_destroy(c);
        l1_destroy(l);
    }
    /* O-5: x - x and x + (-x) */
    l1o_class *c = l1o_create();
    sk_handle x = ent(&ROW_L1O, c, 5, 6), mx = ent(&ROW_L1O, c, -5, 6);
    l1_xcounters d;
    val(&ROW_L1O, c, o_run(c, SK_PH_SUB, x, x, &d), 0, 1);
    CHECK_EQ(d.c[SK_PH_SUB][SK_CORE].o5_ops, 1);
    CHECK_EQ(d.c[SK_PH_SUB][SK_CORE].o5_gcd_work, 3);
    val(&ROW_L1O, c, o_run(c, SK_PH_ADD, x, mx, &d), 0, 1);
    CHECK_EQ(d.c[SK_PH_ADD][SK_CORE].o5_ops, 1);
    CHECK_EQ(d.c[SK_PH_ADD][SK_STORE].o2_skipped0 + d.c[SK_PH_ADD][SK_STORE].o2_skipped1, 2);
    val(&ROW_L1O, c, o_run(c, SK_PH_ADD, x, x, &d), 5, 3); /* same tags: no O-5 */
    CHECK_EQ(d.c[SK_PH_ADD][SK_CORE].o5_ops, 0);
    audits_ok(&ROW_L1O, c);
    l1o_destroy(c);
}

static void test_o6_o7(void) {
    l1o_class *c = l1o_create();
    l1o_reserve(c, 3000);
    sk_handle h;
    for (sk_int v = 2; v < 3002; v++) l1o_enter(c, v, 1, &h);
    sk_counters b;
    l1_xcounters x;
    l1o_counters(c, &b, &x);
    CHECK(XT(x, SK_PH_ENTRY, o6_reads_avoided) > 0);   /* probes rejected by fingerprint */
    CHECK_EQ(XT(x, SK_PH_ENTRY, o7_growths_avoided), 9); /* 16, 32, ..., 2048 and the first */
    sk_memory m;
    l1o_memory(c, &m);
    CHECK_EQ(m.item[2].allocated, 8 * 3000);
    CHECK_EQ(m.item[1].used, 8 * 8192);
    audits_ok(&ROW_L1O, c);
    l1o_destroy(c);
}

/* the counting rules: four index comparisons per operation that reaches them; a failed operation counts none */
static void test_counting_rules(void) {
    l1_class *c = l1_create();
    sk_handle a, z, h;
    l1_enter(c, 2, 3, &a);
    l1_enter(c, 0, 1, &z);
    l1_xcounters x;
    sk_counters b;
    CHECK_EQ(l1_div(c, a, z, &h), SK_EDIVZERO);
    l1_counters(c, &b, &x);
    CHECK_EQ(XT(x, SK_PH_DIV, idx_cmp), 0);
    l1_mul(c, a, a, &h);
    l1_add(c, a, a, &h);
    l1_counters(c, &b, &x);
    CHECK_EQ(XT(x, SK_PH_MUL, idx_cmp), 4);
    CHECK_EQ(XT(x, SK_PH_ADD, idx_cmp), 4);
    /* every intern counts one lookup search (no growth yet): entry 2/3 and 0/1, mul 4/9, add 4/3 */
    CHECK_EQ(OBT(b, lookup_search), 8);
    CHECK_EQ(OXT(x, intern_found) + OXT(x, intern_appended), 8);
    CHECK_EQ(x.c[SK_PH_ADD][SK_STORE].intern_found, 2);
    CHECK_EQ(x.c[SK_PH_MUL][SK_STORE].intern_appended, 2);
    l1_destroy(c);
}

int main(void) {
    test_worked_example();
    test_every_case();
    test_signs();
    test_zero();
    test_overlaps();
    test_entry();
    test_overflow();
    test_store();
    test_audits_catch();
    test_shortcuts_change_nothing();
    test_switches();
    test_o1();
    test_o2();
    test_o3_o4_o5();
    test_o6_o7();
    test_counting_rules();
    printf("unit: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
