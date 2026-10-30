/* unit.c: unit tests of the baseline's rows (spec §13.3 item 1): the control and the ceiling, and the row table
 * across the control, Design 1 and Design 2; run in the test build at widths 64 and 32.  Zero, negatives, division
 * by zero, equal-index fast paths, denominator 1, overflow in each operation, the counting rules, memory accounting,
 * and audits that find broken invariants.
 *
 * Split Oct 29, 2026, when Designs 1 and 2 became separate specifications: the checks, helpers and per-row tests are
 * in t_common.h; Design 1's tests are in 08-common-divisors/lib/tests/unit.c and Design 2's in
 * 09-coprime-base/lib/tests/unit.c.  The three programs together make every check of the former joint unit.c, which
 * ran them in one program; `make test` here runs all three. */
#include "introspect.h"
#include "skeletons.h"
#include "t_common.h"

static const sk_row *ROWS[3] = {&sk_row_ctl, &sk_row_d1, &sk_row_d2};

/* ----------------------------------------------------------------------------------------- the control */

static void test_counting_rules_ctl(void) {
    sk_ctl *c = sk_ctl_create();
    sk_handle a, b, h;
    sk_ctl_enter(c, 1, 12, &a);
    sk_ctl_enter(c, 5, 18, &b);
    sk_counters k;
    sk_ctl_counters(c, &k);
    CHECK_EQ(k.c[SK_PH_ENTRY][SK_CORE].gcd, 2);
    CHECK_EQ(k.c[SK_PH_ENTRY][SK_CORE].div_exact, 4);
    CHECK_EQ(k.c[SK_PH_ENTRY][SK_CORE].gcd_hist[4], 1); /* gcd(1, 12): bits(12) = 4 */
    CHECK_EQ(k.c[SK_PH_ENTRY][SK_CORE].gcd_hist[5], 1); /* gcd(5, 18): bits(18) = 5 */
    CHECK_EQ(k.c[SK_PH_ENTRY][SK_CORE].gcd_work, 9);
    CHECK(sk_ctl_add(c, a, b, &h) == SK_OK);
    sk_ctl_counters(c, &k);
    const sk_count *q = &k.c[SK_PH_ADD][SK_CORE];
    CHECK_EQ(q->gcd, 2);       /* g, h */
    CHECK_EQ(q->div_exact, 4); /* d_y/g, d_x/g, t/h, d_y/h */
    CHECK_EQ(q->mul, 3);
    CHECK_EQ(q->add, 1);
    CHECK_EQ(q->gcd_hist[5], 1);       /* gcd(12, 18) */
    CHECK_EQ(k.c[SK_PH_ADD][SK_STORE].gcd, 0);
    /* the sign is not counted (counting rule 1): 1/2 / (-1/3) = -3/2 */
    sk_handle x, y;
    sk_ctl_enter(c, 1, 2, &x);
    sk_ctl_enter(c, -1, 3, &y);
    CHECK(sk_ctl_div(c, x, y, &h) == SK_OK);
    sk_ctl_counters(c, &k);
    CHECK_EQ(k.c[SK_PH_DIV][SK_CORE].mul, 2);
    CHECK_EQ(k.c[SK_PH_DIV][SK_CORE].add, 0);
    CHECK_EQ(k.c[SK_PH_DIV][SK_CORE].gcd, 2);
    sk_int m, d;
    sk_ctl_read(c, h, &m, &d);
    CHECK(m == -3 && d == 2);
    sk_ctl_destroy(c);
}

/* ----------------------------------------------------------------------------------------- memory */

static void test_memory(void) {
    const uint64_t w = SK_WIDTH / 8;
    /* control: 2w bytes per value */
    sk_ctl *c0 = sk_ctl_create();
    sk_handle h;
    for (int k = 1; k <= 5; k++) sk_ctl_enter(c0, 1, k, &h);
    sk_memory m;
    sk_ctl_memory(c0, &m);
    CHECK_EQ(m.used, 5 * 2 * w);
    CHECK(m.used_per_rational == 2.0 * (double)w);
    CHECK(m.allocated >= m.used);
    sk_ctl_destroy(c0);
}

/* ----------------------------------------------------------------------------------------- audits find failures */

static void test_audits_detect(void) {
    const char *why;
    sk_ctl *c0 = sk_ctl_create();
    sk_handle h;
    sk_ctl_enter(c0, 1, 2, &h);
    CHECK(sk_ctl_audit(c0, &why) == SK_OK);
    sk_ctl_t_corrupt(c0, h, 2, 4);
    CHECK(sk_ctl_audit(c0, &why) == SK_EAUDIT);
    CHECK(why != NULL);
    sk_ctl_destroy(c0);

    sk_ceil *c3 = sk_ceil_create();
    sk_int v[2] = {1, 3};
    sk_ceil_enter_vector(c3, v, 2, 4, &h);
    CHECK(sk_ceil_audit(c3, &why) == SK_OK);
    sk_int v2[2] = {2, 4};
    sk_ceil_enter_vector(c3, v2, 2, 3, &h);
    sk_ceil_t_corrupt_E(c3, h, 6); /* gcd(6, 2, 4) = 2 */
    CHECK(sk_ceil_audit_changed(c3, &why) == SK_EAUDIT);
    sk_ceil_destroy(c3);
}

/* ----------------------------------------------------------------------------------------- the ceiling */

static void cval(sk_ceil *c, sk_handle v, uint32_t k, sk_int m, sk_int d) {
    sk_int a = 0, b = 0;
    CHECK(sk_ceil_read(c, v, k, &a, &b) == SK_OK);
    if (a != m || b != d)
        fprintf(stderr, "  ceil element %u: %lld/%lld, expected %lld/%lld\n", k, (long long)a, (long long)b,
                (long long)m, (long long)d);
    CHECK(a == m && b == d);
}

static void test_ceil(void) {
    sk_ceil *c = sk_ceil_create();
    const char *why;
    sk_handle x, y, z, r, g;
    sk_int m1[] = {1, 2, 3}, m2[] = {2, 4, 6}, m0[] = {0, 0, 0}, mn[] = {1, -2, 5};
    CHECK(sk_ceil_enter_vector(c, m1, 3, 6, &x) == SK_OK);
    cval(c, x, 0, 1, 6);
    cval(c, x, 1, 1, 3);
    cval(c, x, 2, 1, 2);
    CHECK(sk_ceil_enter_vector(c, m2, 3, 8, &y) == SK_OK); /* canonical: E = 4, [1, 2, 3] */
    cval(c, y, 1, 1, 2);
    CHECK(sk_ceil_enter_vector(c, m0, 3, 5, &z) == SK_OK);
    CHECK(sk_ceil_enter_vector(c, mn, 3, -3, &g) == SK_OK);
    cval(c, g, 1, 2, 3);
    cval(c, g, 0, -1, 3);
    CHECK(sk_ceil_enter_vector(c, m1, 3, 0, &r) == SK_EDIVZERO);
    sk_int bad[] = {1, -SK_INT_MAX - 1};
    CHECK(sk_ceil_enter_vector(c, bad, 2, 3, &r) == SK_EINVAL);
    CHECK_EQ(sk_ceil_size(c), 4);
    /* x + y: 1/6 + 1/4 = 5/12, 1/3 + 1/2 = 5/6, 1/2 + 3/4 = 5/4 */
    CHECK(sk_ceil_add(c, x, y, &r) == SK_OK);
    cval(c, r, 0, 5, 12);
    cval(c, r, 1, 5, 6);
    cval(c, r, 2, 5, 4);
    CHECK(sk_ceil_sub(c, x, x, &r) == SK_OK); /* all zero: E = 1 */
    cval(c, r, 2, 0, 1);
    CHECK(sk_ceil_sub(c, x, y, &r) == SK_OK);
    cval(c, r, 0, -1, 12);
    cval(c, r, 2, -1, 4);
    CHECK(sk_ceil_mul(c, x, y, &r) == SK_OK);
    cval(c, r, 0, 1, 24);
    cval(c, r, 2, 3, 8);
    CHECK(sk_ceil_div(c, x, y, &r) == SK_OK); /* (1/6)/(1/4) = 2/3, (1/3)/(1/2) = 2/3, (1/2)/(3/4) = 2/3 */
    cval(c, r, 0, 2, 3);
    cval(c, r, 1, 2, 3);
    CHECK(sk_ceil_div(c, g, x, &r) == SK_OK); /* (-1/3)/(1/6) = -2, (2/3)/(1/3) = 2, (-5/3)/(1/2) = -10/3 */
    cval(c, r, 0, -2, 1);
    cval(c, r, 1, 2, 1);
    cval(c, r, 2, -10, 3);
    uint32_t n = sk_ceil_size(c);
    CHECK(sk_ceil_div(c, x, z, &r) == SK_EDIVZERO);
    CHECK_EQ(sk_ceil_size(c), n);
    /* gather: working copies are operands, not stored values, and not in the memory report */
    sk_memory m0m, m1m;
    sk_ceil_memory(c, &m0m);
    uint32_t pos[] = {2, 0}, pos1[] = {1, 1};
    sk_handle w1, w2, w3;
    CHECK(sk_ceil_gather(c, x, pos, 2, &w1) == SK_OK);
    CHECK(w1 & SK_CEIL_WORKING);
    CHECK(sk_ceil_gather(c, y, pos1, 2, &w2) == SK_OK);
    unsigned char neg[] = {1, 0};
    CHECK(sk_ceil_gather_signed(c, y, pos1, neg, 2, &w3) == SK_OK);
    sk_ceil_memory(c, &m1m);
    CHECK_EQ(m1m.used, m0m.used);
    CHECK_EQ(m1m.values, m0m.values);
    CHECK_EQ(sk_ceil_size(c), n);
    cval(c, w1, 0, 1, 2);
    cval(c, w3, 0, -1, 2);
    CHECK(sk_ceil_add(c, w1, w2, &r) == SK_OK); /* 1/2 + 1/2, 1/6 + 1/2 */
    cval(c, r, 0, 1, 1);
    cval(c, r, 1, 2, 3);
    CHECK(sk_ceil_add(c, w1, w3, &r) == SK_OK); /* 1/2 - 1/2, 1/6 + 1/2 */
    cval(c, r, 0, 0, 1);
    cval(c, r, 1, 2, 3);
    CHECK(sk_ceil_add(c, x, w1, &r) == SK_EINVAL);  /* lengths differ */
    CHECK(sk_ceil_gather(c, x, (uint32_t[]){3}, 1, &r) == SK_EINVAL);
    CHECK(sk_ceil_gather(c, 999, pos, 2, &r) == SK_EINVAL);
    CHECK(sk_ceil_read(c, x, 3, &m1[0], &m1[1]) == SK_EINVAL);
    /* memory: 8 per numerator, plus E and a 16-byte header per stored vector */
    sk_memory mm;
    sk_ceil_memory(c, &mm);
    uint64_t elems = 0;
    for (sk_handle v = 0; v < sk_ceil_size(c); v++) {
        uint32_t len;
        CHECK(sk_ceil_length(c, v, &len) == SK_OK);
        elems += len;
    }
    CHECK_EQ(mm.values, elems);
    CHECK_EQ(mm.used, elems * (SK_WIDTH / 8) + (uint64_t)sk_ceil_size(c) * 24);
    /* overflow fails the whole vector operation */
    sk_int big = SK_INT_MAX / 2 + 1;
    sk_int mb[] = {1, big};
    sk_handle bv, bv3;
    CHECK(sk_ceil_enter_vector(c, mb, 2, 1, &bv) == SK_OK);
    sk_int mb3[] = {1, 1};
    CHECK(sk_ceil_enter_vector(c, mb3, 2, 3, &bv3) == SK_OK);
    n = sk_ceil_size(c);
    CHECK(sk_ceil_add(c, bv, bv, &r) == SK_EOVERFLOW);
    CHECK(sk_ceil_mul(c, bv, bv, &r) == SK_EOVERFLOW);
    CHECK(sk_ceil_div(c, bv, bv3, &r) == SK_EOVERFLOW);
    CHECK_EQ(sk_ceil_size(c), n);
    sk_overflow o;
    CHECK(sk_ceil_overflow(c, &o) == SK_OK);
    CHECK_EQ(o.count, 3);
    CHECK_EQ(o.op, SK_PH_DIV);
    /* counters: content visits numerators; read counts under SK_PH_READ */
    sk_counters k;
    CHECK(sk_ceil_counters(c, &k) == SK_OK);
    CHECK(CSUM(k, scan_content, -1, -1) > 0);
    CHECK(CSUM(k, gcd, SK_PH_READ, -1) > 0);
    CHECK(sk_ceil_audit_changed(c, &why) == SK_OK);
    CHECK(sk_ceil_audit(c, &why) == SK_OK);
    sk_ceil_destroy(c);
}

/* ----------------------------------------------------------------------------------------- the row table */

static void test_rows_agree(void) {
    /* the same calls through the table give the same handles and values in every row */
    void *c[3];
    for (int t = 0; t < 3; t++) c[t] = ROWS[t]->create();
    rng g = {0x5eed};
    sk_handle hs[64];
    int nh = 0;
    for (int k = 0; k < 24; k++) {
        sk_int m = rng_range(&g, -50, 50), d = rng_range(&g, 1, 60);
        for (int t = 0; t < 3; t++) {
            sk_handle h;
            CHECK(ROWS[t]->enter(c[t], m, d, &h) == SK_OK);
            if (t == 0) hs[nh++] = h;
            else CHECK_EQ(h, hs[nh - 1]);
        }
    }
    for (int k = 0; k < 40 && nh < 64; k++) {
        sk_handle x = hs[rng_below(&g, (uint64_t)nh)], y = hs[rng_below(&g, (uint64_t)nh)];
        int op = (int)rng_below(&g, 4);
        sk_status st[3];
        sk_handle h[3];
        for (int t = 0; t < 3; t++) {
            opfn f = op == 0 ? ROWS[t]->add : op == 1 ? ROWS[t]->sub : op == 2 ? ROWS[t]->mul : ROWS[t]->div;
            st[t] = f(c[t], x, y, &h[t]);
        }
        CHECK(st[0] == st[1] && st[0] == st[2]);
        if (st[0] == SK_OK) {
            CHECK(h[0] == h[1] && h[0] == h[2]);
            sk_int m0, d0, m1, d1;
            ROWS[0]->read(c[0], h[0], &m0, &d0);
            for (int t = 1; t < 3; t++) {
                ROWS[t]->read(c[t], h[t], &m1, &d1);
                CHECK(m0 == m1 && d0 == d1);
            }
            hs[nh++] = h[0];
        }
    }
    for (int t = 0; t < 3; t++) {
        audits_ok(ROWS[t], c[t]);
        ROWS[t]->destroy(c[t]);
    }
}

int main(void) {
    test_build();
    test_entry_and_errors(&sk_row_ctl);
    test_zero_and_signs(&sk_row_ctl);
    test_fast_paths(&sk_row_ctl);
    test_note_values(&sk_row_ctl);
    test_overflow(&sk_row_ctl);
    test_counting_rules_ctl();
    test_memory();
    test_audits_detect();
    test_ceil();
    test_rows_agree();
    printf("unit, baseline (width %d): %d checks, %d failures\n", SK_WIDTH, checks, failures);
    return failures ? 1 : 0;
}
