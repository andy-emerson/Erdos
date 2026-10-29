/* unit.c: unit tests of every row (spec §13.3 item 1), run in the test build at widths 64 and 32.
 * Every worked example of the design note, with its intermediate structures; zero, negatives, division by zero,
 * equal-index fast paths, denominator 1, overflow in each operation, composite pieces (multiplication and
 * addition), the scan that stays at a piece, lookup-found results with no power test, h = 1 with no division,
 * islands, the §7.4 example, a divisor inserted mid-list, stride growth, the large-base audit, memory accounting,
 * the counting rules, and audits that find broken invariants. */
#ifndef SK_AUDIT
#error "the unit tests need the test build (-DSK_COUNT -DSK_AUDIT)"
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "introspect.h"
#include "ref.h"
#include "skeletons.h"

static int failures, checks;

#define CHECK(cond)                                                                                               \
    do {                                                                                                          \
        checks++;                                                                                                 \
        if (!(cond)) {                                                                                            \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                       \
        }                                                                                                         \
    } while (0)

#define CHECK_EQ(a, b)                                                                                            \
    do {                                                                                                          \
        checks++;                                                                                                 \
        long long a_ = (long long)(a), b_ = (long long)(b);                                                       \
        if (a_ != b_) {                                                                                           \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s == %lld, expected %s == %lld\n", __FILE__, __LINE__, #a, a_, #b, b_); \
        }                                                                                                         \
    } while (0)

static const sk_row *ROWS[3] = {&sk_row_ctl, &sk_row_d1, &sk_row_d2};

/* ----------------------------------------------------------------------------------------- generic helpers */

static sk_handle ent(const sk_row *R, void *c, sk_int m, sk_int d) {
    sk_handle h = UINT32_MAX;
    sk_status s = R->enter(c, m, d, &h);
    CHECK(s == SK_OK);
    return h;
}

typedef sk_status (*opfn)(void *, sk_handle, sk_handle, sk_handle *);

static sk_handle op_ok(opfn f, void *c, sk_handle x, sk_handle y) {
    sk_handle h = 0;
    sk_status s = f(c, x, y, &h);
    if (s != SK_OK) fprintf(stderr, "  op returned %s\n", sk_status_name(s));
    CHECK(s == SK_OK);
    return h;
}

static void val(const sk_row *R, void *c, sk_handle h, sk_int m, sk_int d) {
    sk_int a = 0, b = 0;
    CHECK(R->read(c, h, &a, &b) == SK_OK);
    if (a != m || b != d)
        fprintf(stderr, "  %s: value %lld/%lld, expected %lld/%lld\n", R->name, (long long)a, (long long)b,
                (long long)m, (long long)d);
    CHECK(a == m && b == d);
}

static void audits_ok(const sk_row *R, void *c) {
    const char *why = NULL;
    sk_status s = R->audit_changed(c, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit_changed: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
    s = R->audit(c, &why);
    if (s != SK_OK) fprintf(stderr, "  %s audit: %s\n", R->name, why ? why : sk_status_name(s));
    CHECK(s == SK_OK);
    s = R->audit_scheduled(c, &why);
    CHECK(s == SK_OK);
}

/* Sum one counter field over phases and parts (phase < 0: all; part < 0: both). */
static uint64_t csum(const sk_counters *k, size_t off, int phase, int part) {
    uint64_t s = 0;
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++)
            if ((phase < 0 || phase == p) && (part < 0 || part == q))
                s += *(const uint64_t *)((const char *)&k->c[p][q] + off);
    return s;
}
#define CSUM(cnt, field, phase, part) csum(&(cnt), offsetof(sk_count, field), (phase), (part))

static sk_counters cnt_of(const sk_row *R, const void *c) {
    sk_counters k;
    CHECK(R->counters(c, &k) == SK_OK);
    return k;
}

/* ----------------------------------------------------------------------------------------- every row */

static void test_build(void) {
    CHECK_EQ(sk_build_width(), SK_WIDTH);
    CHECK_EQ(sk_build_counting(), 1);
    CHECK_EQ(sk_build_audit(), 1);
    CHECK_EQ(sizeof(sk_int) * 8, SK_WIDTH);
}

static void test_entry_and_errors(const sk_row *R) {
    void *c = R->create();
    val(R, c, ent(R, c, 3, -6), -1, 2);
    val(R, c, ent(R, c, -4, -6), 2, 3);
    val(R, c, ent(R, c, 0, -3), 0, 1);
    val(R, c, ent(R, c, 0, 5), 0, 1);
    val(R, c, ent(R, c, 7, 1), 7, 1);
    val(R, c, ent(R, c, SK_INT_MAX, SK_INT_MAX), 1, 1);
    val(R, c, ent(R, c, -SK_INT_MAX, 1), -SK_INT_MAX, 1);
    uint32_t n = R->size(c);
    CHECK_EQ(n, 7);
    sk_handle h = 12345;
    CHECK(R->enter(c, 1, 0, &h) == SK_EDIVZERO);
    CHECK(R->enter(c, 0, 0, &h) == SK_EDIVZERO);
    CHECK(R->enter(c, -SK_INT_MAX - 1, 3, &h) == SK_EINVAL);
    CHECK(R->enter(c, 3, -SK_INT_MAX - 1, &h) == SK_EINVAL);
    CHECK(R->add(c, 0, 99, &h) == SK_EINVAL);
    CHECK(R->sub(c, 99, 0, &h) == SK_EINVAL);
    CHECK(R->mul(c, 7, 0, &h) == SK_EINVAL);
    CHECK(R->div(c, 0, 7, &h) == SK_EINVAL);
    sk_int m, d;
    CHECK(R->read(c, 7, &m, &d) == SK_EINVAL);
    CHECK_EQ(R->size(c), n);
    sk_overflow o;
    CHECK(R->overflow(c, &o) == SK_EINVAL);
    audits_ok(R, c);
    R->destroy(c);
}

static void test_zero_and_signs(const sk_row *R) {
    void *c = R->create();
    sk_handle z = ent(R, c, 0, 7), x = ent(R, c, 3, 4), y = ent(R, c, 5, 6), mx = ent(R, c, -3, 4);
    val(R, c, op_ok(R->add, c, z, x), 3, 4);
    val(R, c, op_ok(R->add, c, x, z), 3, 4);
    val(R, c, op_ok(R->sub, c, x, x), 0, 1);       /* zero result, same index */
    val(R, c, op_ok(R->add, c, x, mx), 0, 1);      /* zero result, same index */
    val(R, c, op_ok(R->sub, c, z, x), -3, 4);
    val(R, c, op_ok(R->sub, c, z, z), 0, 1);
    val(R, c, op_ok(R->mul, c, z, y), 0, 1);
    val(R, c, op_ok(R->mul, c, y, z), 0, 1);
    val(R, c, op_ok(R->mul, c, z, z), 0, 1);
    val(R, c, op_ok(R->div, c, z, y), 0, 1);
    sk_handle h;
    uint32_t n = R->size(c);
    CHECK(R->div(c, x, z, &h) == SK_EDIVZERO);
    CHECK(R->div(c, z, z, &h) == SK_EDIVZERO);
    CHECK_EQ(R->size(c), n);
    /* negatives */
    sk_handle a = ent(R, c, -1, 3), b = ent(R, c, 1, 6), e = ent(R, c, 3, -4), f = ent(R, c, -2, 3);
    val(R, c, op_ok(R->add, c, a, b), -1, 6);
    val(R, c, op_ok(R->sub, c, a, b), -1, 2);
    val(R, c, op_ok(R->mul, c, f, e), 1, 2);
    val(R, c, op_ok(R->div, c, b, e), -2, 9);
    val(R, c, op_ok(R->div, c, a, f), 1, 2);
    val(R, c, op_ok(R->div, c, e, mx), 1, 1);       /* same index, negative over negative */
    /* denominator 1 */
    sk_handle i3 = ent(R, c, 3, 1), i4 = ent(R, c, 4, 1), t3 = ent(R, c, 1, 3);
    val(R, c, op_ok(R->add, c, i3, i4), 7, 1);
    val(R, c, op_ok(R->sub, c, i3, i4), -1, 1);
    val(R, c, op_ok(R->mul, c, i3, t3), 1, 1);
    val(R, c, op_ok(R->div, c, ent(R, c, 6, 1), i4), 3, 2);
    val(R, c, op_ok(R->div, c, t3, i3), 1, 9);
    audits_ok(R, c);
    R->destroy(c);
}

static void test_fast_paths(const sk_row *R) {
    void *c = R->create();
    sk_handle x = ent(R, c, 1, 10), y = ent(R, c, 3, 10);
    sk_counters k0 = cnt_of(R, c);
    val(R, c, op_ok(R->add, c, x, y), 2, 5);
    val(R, c, op_ok(R->sub, c, x, y), -1, 5);
    val(R, c, op_ok(R->mul, c, x, y), 3, 100);
    val(R, c, op_ok(R->div, c, x, y), 1, 3);
    sk_counters k1 = cnt_of(R, c);
    for (int ph = SK_PH_ADD; ph <= SK_PH_DIV; ph++)
        CHECK_EQ(CSUM(k1, ev_fast_paths, ph, -1) - CSUM(k0, ev_fast_paths, ph, -1), 1);
    /* the same-denominator multiply computes no gcd (note: "with no gcd") */
    CHECK_EQ(CSUM(k1, gcd, SK_PH_MUL, SK_CORE) - CSUM(k0, gcd, SK_PH_MUL, SK_CORE), 0);
    audits_ok(R, c);
    R->destroy(c);
}

/* The note's worked values, in the order of the note (all rows). */
static void test_note_values(const sk_row *R) {
    void *c = R->create();
    sk_handle a = ent(R, c, 1, 12), b = ent(R, c, 5, 18), e = ent(R, c, 1, 30);
    val(R, c, op_ok(R->add, c, a, b), 13, 36);
    val(R, c, op_ok(R->add, c, a, e), 7, 60);
    val(R, c, op_ok(R->add, c, b, ent(R, c, 1, 30)), 14, 45); /* 5/18 + 1/30 */
    val(R, c, op_ok(R->add, c, ent(R, c, 1, 18), e), 4, 45);
    val(R, c, op_ok(R->add, c, ent(R, c, 7, 12), ent(R, c, 1, 18)), 23, 36);
    val(R, c, op_ok(R->sub, c, a, b), -7, 36);
    val(R, c, op_ok(R->mul, c, b, ent(R, c, 7, 30)), 7, 108);
    sk_handle f = ent(R, c, 5, 12);
    val(R, c, op_ok(R->mul, c, f, ent(R, c, 7, 12)), 35, 144);
    val(R, c, op_ok(R->div, c, f, ent(R, c, -7, 18)), -15, 14);
    val(R, c, op_ok(R->add, c, ent(R, c, 1, 12), ent(R, c, 1, 20)), 2, 15); /* coprime-base note §6 */
    audits_ok(R, c);
    R->destroy(c);
}

static void expect_overflow(const sk_row *R, void *c, opfn f, sk_phase ph, sk_handle x, sk_handle y) {
    uint32_t n = R->size(c);
    sk_overflow o0, o1;
    R->overflow(c, &o0);
    sk_counters k0 = cnt_of(R, c);
    sk_handle h = 777;
    sk_status s = f(c, x, y, &h);
    CHECK(s == SK_EOVERFLOW);
    CHECK_EQ(h, 777);
    CHECK_EQ(R->size(c), n);
    CHECK(R->overflow(c, &o1) == SK_OK);
    CHECK_EQ(o1.count, o0.count + 1);
    CHECK_EQ(o1.op, ph);
    CHECK_EQ(o1.x, x);
    CHECK_EQ(o1.y, y);
    CHECK(o1.step != NULL);
    sk_counters k1 = cnt_of(R, c);
    CHECK_EQ(CSUM(k1, ev_overflows, ph, -1) - CSUM(k0, ev_overflows, ph, -1), 1);
    audits_ok(R, c); /* the class remains valid (spec §4.4 item 3) */
}

static void test_overflow(const sk_row *R) {
    void *c = R->create();
    sk_int big = SK_INT_MAX / 2 + 1; /* 2^(w-2) */
    sk_handle b1 = ent(R, c, big, 1), b4 = ent(R, c, big - 1, 4), b8 = ent(R, c, big - 1, 8), b8n = ent(R, c, 1 - big, 8);
    sk_handle i3 = ent(R, c, 3, 1), t3 = ent(R, c, 1, 3), f35 = ent(R, c, 3, 5), b7 = ent(R, c, big, 7);
    expect_overflow(R, c, R->add, SK_PH_ADD, b1, b1);     /* same index: t = m_x + m_y */
    expect_overflow(R, c, R->add, SK_PH_ADD, b4, b8);     /* different indices: t = 3(big-1) */
    expect_overflow(R, c, R->sub, SK_PH_SUB, ent(R, c, -big, 1), b1); /* same index */
    expect_overflow(R, c, R->sub, SK_PH_SUB, b4, b8n);
    expect_overflow(R, c, R->mul, SK_PH_MUL, b1, i3);     /* same index */
    expect_overflow(R, c, R->mul, SK_PH_MUL, b7, f35);    /* different indices */
    expect_overflow(R, c, R->div, SK_PH_DIV, b1, t3);     /* different indices: big*3 */
    expect_overflow(R, c, R->div, SK_PH_DIV, t3, b7);     /* denominator 3*big */
    /* the largest values that fit still work */
    val(R, c, op_ok(R->sub, c, b1, ent(R, c, 1, 1)), big - 1, 1);
    val(R, c, op_ok(R->add, c, ent(R, c, SK_INT_MAX - 1, 1), ent(R, c, 1, 1)), SK_INT_MAX, 1);
    R->destroy(c);
}

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

/* ----------------------------------------------------------------------------------------- Design 1 */

static void d1_row_is(const sk_d1 *c, uint32_t k, const sk_int *divs, const sk_int *res, uint32_t n) {
    /* row k holds exactly the divisors divs[0..n-1] (by value), with residuals res[] */
    uint32_t set = 0;
    for (uint32_t col = 0; col < sk_d1_t_C(c); col++) set += (uint32_t)sk_d1_t_bit(c, k, col);
    CHECK_EQ(set, n);
    CHECK_EQ(sk_d1_t_res_len(c, k), n);
    for (uint32_t t = 0; t < n; t++) {
        uint32_t col = 0;
        while (col < sk_d1_t_C(c) && sk_d1_t_divisor(c, col) != divs[t]) col++;
        CHECK(col < sk_d1_t_C(c));
        CHECK(sk_d1_t_bit(c, k, col));
        CHECK_EQ(sk_d1_t_res(c, k, t), res[t]);
    }
}

static void d1_island(const sk_d1 *c, uint32_t k) {
    sk_int one[1] = {1}, e[1] = {sk_d1_t_den(c, k)};
    d1_row_is(c, k, one, e, 1);
    for (uint32_t i = 0; i < sk_d1_t_D(c); i++) CHECK(!sk_d1_t_pair(c, i, k));
}

static void test_d1_note(void) {
    sk_d1 *c = sk_d1_create();
    sk_handle a, b, e, r, h;
    sk_d1_enter(c, 1, 12, &a);
    sk_d1_enter(c, 5, 18, &b);
    sk_d1_enter(c, 1, 30, &e);
    CHECK_EQ(sk_d1_t_C(c), 1);
    CHECK_EQ(sk_d1_t_divisor(c, 0), 1);
    { sk_int dv[] = {1}, rs[] = {12}; d1_row_is(c, 0, dv, rs, 1); }
    /* 1/12 + 5/18 */
    CHECK(sk_d1_add(c, a, b, &r) == SK_OK);
    val(&sk_row_d1, c, r, 13, 36);
    CHECK_EQ(sk_d1_t_C(c), 2);
    CHECK_EQ(sk_d1_t_divisor(c, 1), 6);
    { sk_int dv[] = {1, 6}, rs[] = {12, 2}; d1_row_is(c, 0, dv, rs, 2); }
    { sk_int dv[] = {1, 6}, rs[] = {18, 3}; d1_row_is(c, 1, dv, rs, 2); }
    CHECK(sk_d1_t_pair(c, 0, 1));
    CHECK_EQ(sk_d1_t_den_index(c, r), 3);
    d1_island(c, 3); /* spec §7.4 example: 36 is {1:36}, though 6 divides it */
    /* 1/12 + 1/30 */
    CHECK(sk_d1_add(c, a, e, &r) == SK_OK);
    val(&sk_row_d1, c, r, 7, 60);
    CHECK_EQ(sk_d1_t_C(c), 2);
    { sk_int dv[] = {1, 6}, rs[] = {30, 5}; d1_row_is(c, 2, dv, rs, 2); }
    CHECK(sk_d1_t_pair(c, 0, 2));
    CHECK_EQ(sk_d1_t_den_index(c, r), 4);
    d1_island(c, 4);
    /* 1/18 + 1/30: the chain at work */
    sk_handle b2, e2;
    sk_d1_enter(c, 1, 18, &b2);
    sk_d1_enter(c, 1, 30, &e2);
    sk_counters k0, k1;
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, b2, e2, &r) == SK_OK);
    sk_d1_counters(c, &k1);
    val(&sk_row_d1, c, r, 4, 45);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].gcd - k0.c[SK_PH_ADD][SK_CORE].gcd, 2); /* gcd(3, 5), gcd(8, 6) */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].gcd_hist[3] - k0.c[SK_PH_ADD][SK_CORE].gcd_hist[3], 1); /* on residuals */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ev_compares - k0.c[SK_PH_ADD][SK_CORE].ev_compares, 1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_div_inserted - k0.c[SK_PH_ADD][SK_CORE].ref_div_inserted, 0);
    CHECK_EQ(sk_d1_t_C(c), 2);
    CHECK(sk_d1_t_pair(c, 1, 2));
    CHECK_EQ(sk_d1_t_den_index(c, r), 5);
    d1_island(c, 5);
    /* a later 7/12 + 1/18: the pair bit is set; no gcd of denominators; 36 found at index 3, its row unchanged */
    sk_handle a7, b3;
    sk_d1_enter(c, 7, 12, &a7);
    sk_d1_enter(c, 1, 18, &b3);
    uint32_t D = sk_d1_t_D(c);
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, a7, b3, &r) == SK_OK);
    sk_d1_counters(c, &k1);
    val(&sk_row_d1, c, r, 23, 36);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ev_pair_hits - k0.c[SK_PH_ADD][SK_CORE].ev_pair_hits, 1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].gcd - k0.c[SK_PH_ADD][SK_CORE].gcd, 1); /* gcd(23, 6) only */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_new_dens - k0.c[SK_PH_ADD][SK_STORE].ev_new_dens, 0);
    CHECK_EQ(sk_d1_t_den_index(c, r), 3);
    CHECK_EQ(sk_d1_t_D(c), D);
    d1_island(c, 3);
    /* subtraction: 1/12 - 5/18 takes addition's steps */
    CHECK(sk_d1_sub(c, a, b, &r) == SK_OK);
    val(&sk_row_d1, c, r, -7, 36);
    /* 5/18 * 7/30: c = 6, residuals 3 and 5; g_1 = gcd(5, 5), g_2 = gcd(7, 3); no compare, no pair bit */
    sk_handle s7;
    sk_d1_enter(c, 7, 30, &s7);
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_mul(c, b, s7, &r) == SK_OK);
    sk_d1_counters(c, &k1);
    val(&sk_row_d1, c, r, 7, 108);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_CORE].gcd - k0.c[SK_PH_MUL][SK_CORE].gcd, 2);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_CORE].gcd_hist[3] - k0.c[SK_PH_MUL][SK_CORE].gcd_hist[3], 2); /* 5,5 and 7,3 */
    CHECK_EQ(k1.c[SK_PH_MUL][SK_CORE].pair_read - k0.c[SK_PH_MUL][SK_CORE].pair_read, 0);
    d1_island(c, sk_d1_t_den_index(c, r));
    /* 5/12 * 7/12: no gcd */
    sk_handle f5, f7;
    sk_d1_enter(c, 5, 12, &f5);
    sk_d1_enter(c, 7, 12, &f7);
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_mul(c, f5, f7, &r) == SK_OK);
    sk_d1_counters(c, &k1);
    val(&sk_row_d1, c, r, 35, 144);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_CORE].gcd - k0.c[SK_PH_MUL][SK_CORE].gcd, 0);
    d1_island(c, sk_d1_t_den_index(c, r)); /* 144 = 12^2, a multiple of 12: column 0 only, residual 144 */
    /* 5/12 / (-7/18) = -15/14: the pair bit (0, 1) is set, so the gcd is read; 14 is an island */
    sk_handle m7;
    sk_d1_enter(c, -7, 18, &m7);
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_div(c, f5, m7, &h) == SK_OK);
    sk_d1_counters(c, &k1);
    val(&sk_row_d1, c, h, -15, 14);
    CHECK_EQ(k1.c[SK_PH_DIV][SK_CORE].ev_pair_hits - k0.c[SK_PH_DIV][SK_CORE].ev_pair_hits, 1);
    CHECK_EQ(k1.c[SK_PH_DIV][SK_CORE].gcd - k0.c[SK_PH_DIV][SK_CORE].gcd, 1); /* g_1 = gcd(5, 7) only */
    d1_island(c, sk_d1_t_den_index(c, h));
    audits_ok(&sk_row_d1, c);
    sk_d1_destroy(c);
}

/* New result denominators of each operation are islands; one multiple of an operand's denominator; a result
 * found by the lookup keeps its row. */
static void test_d1_islands(void) {
    sk_d1 *c = sk_d1_create();
    sk_handle a, b, r;
    sk_d1_enter(c, 1, 12, &a);
    sk_d1_enter(c, 1, 18, &b);
    CHECK(sk_d1_add(c, a, b, &r) == SK_OK); /* 5/36 */
    d1_island(c, sk_d1_t_den_index(c, r));
    CHECK(sk_d1_sub(c, a, b, &r) == SK_OK); /* 1/36: found, row unchanged */
    d1_island(c, sk_d1_t_den_index(c, r));
    sk_handle f;
    sk_d1_enter(c, 1, 5, &f);
    CHECK(sk_d1_mul(c, a, f, &r) == SK_OK); /* 1/60 = 12 * 5: a multiple of d_i, known */
    val(&sk_row_d1, c, r, 1, 60);
    d1_island(c, sk_d1_t_den_index(c, r));
    sk_handle g;
    sk_d1_enter(c, 7, 1, &g);
    CHECK(sk_d1_div(c, a, g, &r) == SK_OK); /* 1/84 = 12 * 7 */
    val(&sk_row_d1, c, r, 1, 84);
    d1_island(c, sk_d1_t_den_index(c, r));
    CHECK(sk_d1_mul(c, a, a, &r) == SK_OK); /* 1/144: same index */
    d1_island(c, sk_d1_t_den_index(c, r));
    /* a found result: row 0 (12) has columns 1 and 6; 1/12 result from 1/24 + 1/24 is found and unchanged */
    sk_handle q;
    sk_d1_enter(c, 1, 24, &q);
    CHECK(sk_d1_add(c, q, q, &r) == SK_OK);
    CHECK_EQ(sk_d1_t_den_index(c, r), 0);
    { sk_int dv[] = {1, 6}, rs[] = {12, 2}; d1_row_is(c, 0, dv, rs, 2); }
    audits_ok(&sk_row_d1, c);
    sk_d1_destroy(c);
}

/* The note's incidence example: divisors [1, 4, 6, 15]; 30's bits 1 0 1 1, residuals [30, 5, 2]; 4 is inserted
 * mid-list, so every row shifts. */
static void test_d1_incidence(void) {
    sk_d1 *c = sk_d1_create();
    sk_handle h30, h12, h8, h45, r;
    sk_d1_enter(c, 1, 30, &h30);
    sk_d1_enter(c, 1, 12, &h12);
    sk_d1_enter(c, 1, 8, &h8);
    sk_d1_enter(c, 1, 45, &h45);
    CHECK(sk_d1_add(c, h30, h12, &r) == SK_OK); /* gcd 6 */
    sk_counters k0, k1;
    sk_d1_counters(c, &k0);
    uint32_t D0 = sk_d1_t_D(c);
    CHECK(sk_d1_add(c, h12, h8, &r) == SK_OK); /* gcd 4: inserted below 6 */
    sk_d1_counters(c, &k1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_div_inserted - k0.c[SK_PH_ADD][SK_CORE].ref_div_inserted, 1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_words_shifted - k0.c[SK_PH_ADD][SK_CORE].ref_words_shifted,
             D0); /* one word per row */
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, h30, h45, &r) == SK_OK); /* gcd 15: inserted at the end, no shift */
    sk_d1_counters(c, &k1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_words_shifted - k0.c[SK_PH_ADD][SK_CORE].ref_words_shifted, 0);
    CHECK_EQ(sk_d1_t_C(c), 4);
    CHECK_EQ(sk_d1_t_divisor(c, 1), 4);
    CHECK_EQ(sk_d1_t_divisor(c, 2), 6);
    CHECK_EQ(sk_d1_t_divisor(c, 3), 15);
    uint32_t k30 = sk_d1_t_den_index(c, h30);
    CHECK(sk_d1_t_bit(c, k30, 0) && !sk_d1_t_bit(c, k30, 1) && sk_d1_t_bit(c, k30, 2) && sk_d1_t_bit(c, k30, 3));
    CHECK_EQ(sk_d1_t_res_len(c, k30), 3);
    CHECK_EQ(sk_d1_t_res(c, k30, 0), 30);
    CHECK_EQ(sk_d1_t_res(c, k30, 1), 5);
    CHECK_EQ(sk_d1_t_res(c, k30, 2), 2); /* the residual at 15 (column 3) is at position 2 */
    { sk_int dv[] = {1, 4, 6}, rs[] = {12, 3, 2}; d1_row_is(c, sk_d1_t_den_index(c, h12), dv, rs, 3); }
    { sk_int dv[] = {1, 4}, rs[] = {8, 2}; d1_row_is(c, sk_d1_t_den_index(c, h8), dv, rs, 2); }
    { sk_int dv[] = {1, 15}, rs[] = {45, 3}; d1_row_is(c, sk_d1_t_den_index(c, h45), dv, rs, 2); }
    audits_ok(&sk_row_d1, c);
    sk_d1_destroy(c);
}

static const int PRIMES[] = {7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79,
                             83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
                             179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269,
                             271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359};
#define NPRIMES ((int)(sizeof PRIMES / sizeof PRIMES[0]))

/* More than 64 divisors: the stride grows; inserted in decreasing order, so every insertion shifts. */
static void test_d1_stride(void) {
    sk_d1 *c = sk_d1_create();
    CHECK(NPRIMES >= 66);
    for (int t = NPRIMES - 1; t >= 0; t--) {
        sk_handle x, y, r;
        sk_d1_enter(c, 1, 2 * PRIMES[t], &x);
        sk_d1_enter(c, 1, 3 * PRIMES[t], &y);
        CHECK(sk_d1_add(c, x, y, &r) == SK_OK);
        val(&sk_row_d1, c, r, 5, 6 * PRIMES[t]);
        if (t % 9 == 0) audits_ok(&sk_row_d1, c);
    }
    CHECK_EQ(sk_d1_t_C(c), NPRIMES + 1);
    CHECK_EQ(sk_d1_t_W(c), 2);
    sk_counters k;
    sk_d1_counters(c, &k);
    CHECK_EQ(CSUM(k, ref_stride_growths, -1, -1), 1);
    CHECK_EQ(CSUM(k, ref_div_inserted, -1, -1), NPRIMES);
    for (uint32_t col = 1; col < sk_d1_t_C(c); col++) CHECK_EQ(sk_d1_t_divisor(c, col), PRIMES[col - 1]);
    /* every pair compared is still exact after the shifts and the stride growth */
    sk_handle x, y, r;
    sk_d1_enter(c, 1, 2 * PRIMES[1], &x);
    sk_d1_enter(c, 2, 3 * PRIMES[1], &y);
    CHECK(sk_d1_add(c, x, y, &r) == SK_OK); /* the pair bit is set: the search gives the gcd */
    val(&sk_row_d1, c, r, 7, 6 * PRIMES[1]);
    audits_ok(&sk_row_d1, c);
    sk_d1_destroy(c);
}

static void test_d1_counting(void) {
    sk_d1 *c = sk_d1_create();
    sk_handle a, b, r;
    sk_d1_enter(c, 1, 12, &a);
    sk_d1_enter(c, 5, 18, &b);
    CHECK(sk_d1_add(c, a, b, &r) == SK_OK);
    sk_counters k;
    sk_d1_counters(c, &k);
    const sk_count *q = &k.c[SK_PH_ADD][SK_CORE];
    CHECK_EQ(q->gcd, 2);                 /* g' = gcd(12, 18), h = gcd(13, 6) */
    CHECK_EQ(q->mul, 4);                 /* c*g', m_x*r_y, m_y*r_x, r_x*(d_j/h) */
    CHECK_EQ(q->add, 1);
    CHECK_EQ(q->div_exact, 4);           /* r_i/g', r_j/g', t/h, d_j/h */
    CHECK_EQ(q->pair_read, 1);
    CHECK_EQ(q->scan_and_words, 1);
    CHECK_EQ(q->scan_popcount_words, 2); /* positions of column 1 in rows 0 and 1; column 0 needs none */
    CHECK_EQ(q->ev_compares, 1);
    CHECK_EQ(q->ref_div_inserted, 1);
    CHECK_EQ(k.c[SK_PH_ADD][SK_STORE].ev_new_dens, 1);
    CHECK_EQ(k.c[SK_PH_ADD][SK_STORE].lookup_search, 1);
    CHECK_EQ(k.c[SK_PH_ADD][SK_STORE].gcd + k.c[SK_PH_ADD][SK_STORE].mul + k.c[SK_PH_ADD][SK_STORE].div_exact, 0);
    /* g = 1: h is 1 and no division by it (change log CL-1) */
    sk_handle x, y;
    sk_d1_enter(c, 1, 5, &x);
    sk_d1_enter(c, 1, 7, &y);
    sk_counters k0;
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, x, y, &r) == SK_OK);
    sk_d1_counters(c, &k);
    val(&sk_row_d1, c, r, 12, 35);
    CHECK_EQ(k.c[SK_PH_ADD][SK_CORE].gcd - k0.c[SK_PH_ADD][SK_CORE].gcd, 1); /* g' = gcd(5, 7) only */
    CHECK_EQ(k.c[SK_PH_ADD][SK_CORE].div_exact - k0.c[SK_PH_ADD][SK_CORE].div_exact, 0);
    CHECK_EQ(k.c[SK_PH_ADD][SK_CORE].mul - k0.c[SK_PH_ADD][SK_CORE].mul, 3);
    sk_d1_destroy(c);
}

/* ----------------------------------------------------------------------------------------- Design 2 */

static void d2_row_is(const sk_d2 *c, uint32_t k, const sk_int *pieces, const uint32_t *nus, uint32_t n) {
    /* row k is exactly {pieces[t]: nus[t]} (pieces by value, in row order) */
    CHECK_EQ(sk_d2_t_row_len(c, k), n);
    if (sk_d2_t_row_len(c, k) != n) return;
    for (uint32_t t = 0; t < n; t++) {
        uint32_t q, nu;
        sk_d2_t_row_entry(c, k, t, &q, &nu);
        CHECK_EQ(sk_d2_t_piece(c, q), pieces[t]);
        CHECK_EQ(nu, nus[t]);
    }
}

static void d2_base_is(const sk_d2 *c, const sk_int *pieces, uint32_t n) {
    CHECK_EQ(sk_d2_t_C(c), n);
    for (uint32_t p = 0; p < n && p < sk_d2_t_C(c); p++) CHECK_EQ(sk_d2_t_piece(c, p), pieces[p]);
}

static void test_d2_note(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle a, b, e, r;
    sk_d2_enter(c, 1, 12, &a);
    { sk_int B[] = {12}; d2_base_is(c, B, 1); }
    { sk_int P[] = {12}; uint32_t V[] = {1}; d2_row_is(c, 0, P, V, 1); }
    sk_d2_enter(c, 5, 18, &b);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 1}; d2_row_is(c, 0, P, V, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 2}; d2_row_is(c, 1, P, V, 2); }
    sk_d2_enter(c, 1, 30, &e);
    { sk_int B[] = {2, 3, 5}; d2_base_is(c, B, 3); }
    { sk_int P[] = {2, 3, 5}; uint32_t V[] = {1, 1, 1}; d2_row_is(c, 2, P, V, 3); }
    /* 1/12 + 5/18: no piece of equal valuation; no gcd; R = {2:2, 3:2}, u = 1 */
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, a, b, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 13, 36);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_ADD, -1) - CSUM(k0, gcd, SK_PH_ADD, -1), 0);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_rows_u1 - k0.c[SK_PH_ADD][SK_STORE].ev_rows_u1, 1);
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 2}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    /* 1/18 + 1/30: piece 2 has equal valuations and cancels by 2 = 2^1; R = {3:2, 5:1}, u = 1 */
    sk_handle b1, e1;
    sk_d2_enter(c, 1, 18, &b1);
    sk_d2_enter(c, 1, 30, &e1);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, b1, e1, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 4, 45);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].gcd - k0.c[SK_PH_ADD][SK_CORE].gcd, 1); /* gcd(8, 2) */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].gcd - k0.c[SK_PH_ADD][SK_STORE].gcd, 0);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].div_rem - k0.c[SK_PH_ADD][SK_STORE].div_rem, 1); /* power(2, 2) */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_rows_u1 - k0.c[SK_PH_ADD][SK_STORE].ev_rows_u1, 1);
    { sk_int P[] = {3, 5}; uint32_t V[] = {2, 1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    /* found by the lookup: 7/18 + 1/30 = 19/45, no power test */
    sk_handle b7;
    sk_d2_enter(c, 7, 18, &b7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, b7, e1, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 19, 45);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].div_rem - k0.c[SK_PH_ADD][SK_STORE].div_rem, 0);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_new_dens - k0.c[SK_PH_ADD][SK_STORE].ev_new_dens, 0);
    /* 1/12 - 5/18 */
    CHECK(sk_d2_sub(c, a, b, &r) == SK_OK);
    val(&sk_row_d2, c, r, -7, 36);
    /* 5/18 * 7/30 = 7/108: pieces 2 and 3 in both rows are skipped; gcd(5, 5^1) = 5 */
    sk_handle s7;
    sk_d2_enter(c, 7, 30, &s7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, b, s7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 7, 108);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_CORE].gcd - k0.c[SK_PH_MUL][SK_CORE].gcd, 1);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_STORE].ev_rows_u1 - k0.c[SK_PH_MUL][SK_STORE].ev_rows_u1, 1);
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 3}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    /* again: found by the lookup, no power test */
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, b, s7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_STORE].div_rem - k0.c[SK_PH_MUL][SK_STORE].div_rem, 0);
    CHECK_EQ(k1.c[SK_PH_MUL][SK_STORE].ev_new_dens - k0.c[SK_PH_MUL][SK_STORE].ev_new_dens, 0);
    /* 5/12 * 7/12 = 35/144: no gcd; row 12 doubled, u = 1 */
    sk_handle f5, f7;
    sk_d2_enter(c, 5, 12, &f5);
    sk_d2_enter(c, 7, 12, &f7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, f5, f7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 35, 144);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_MUL, -1) - CSUM(k0, gcd, SK_PH_MUL, -1), 0);
    { sk_int P[] = {2, 3}; uint32_t V[] = {4, 2}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    /* 5/12 / (-7/18) = -15/14: R = {2:1}, u = 7 becomes a new piece */
    sk_handle m7;
    sk_d2_enter(c, -7, 18, &m7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_div(c, f5, m7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, -15, 14);
    CHECK_EQ(k1.c[SK_PH_DIV][SK_STORE].ev_rows_refined - k0.c[SK_PH_DIV][SK_STORE].ev_rows_refined, 1);
    CHECK_EQ(k1.c[SK_PH_DIV][SK_STORE].scan_refine_pieces - k0.c[SK_PH_DIV][SK_STORE].scan_refine_pieces, 3);
    { sk_int B[] = {2, 3, 5, 7}; d2_base_is(c, B, 4); }
    { sk_int P[] = {2, 7}; uint32_t V[] = {1, 1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* Base [6]: 2/1 * 1/6 = 1/3; h = gcd(2, 6) = 2 is not a power of 6; R is empty, the leftover 3 is refined and
 * splits 6 into 2 and 3.  With the exact counts of the store. */
static void test_d2_composite_mul(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle s6, s36, two, r;
    sk_d2_enter(c, 1, 6, &s6);
    sk_d2_enter(c, 1, 36, &s36);
    { sk_int B[] = {6}; d2_base_is(c, B, 1); }
    { sk_int P[] = {6}; uint32_t V[] = {2}; d2_row_is(c, 1, P, V, 1); }
    sk_d2_enter(c, 2, 1, &two);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, two, s6, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 1, 3);
    const sk_count *a = &k0.c[SK_PH_MUL][SK_STORE], *b = &k1.c[SK_PH_MUL][SK_STORE];
    CHECK_EQ(b->ev_rows_refined - a->ev_rows_refined, 1);
    CHECK_EQ(b->ref_splits - a->ref_splits, 1);
    CHECK_EQ(b->ref_pieces_created - a->ref_pieces_created, 1);
    CHECK_EQ(b->ref_entries_rewritten - a->ref_entries_rewritten, 4);  /* rows 6 and 36, two entries each */
    CHECK_EQ(b->scan_split_rows - a->scan_split_rows, 4);              /* rows 1, 6, 36 and the new row */
    CHECK_EQ(b->div_rem - a->div_rem, 7);   /* power(2, 6): 1; nu_2(6): 2; nu_3(6): 2; 3 by piece 3: 2 */
    CHECK_EQ(b->div_exact - a->div_exact, 2); /* q/g in the split; p/h in finish */
    CHECK_EQ(b->gcd - a->gcd, 4);           /* pair refinement 1; scan gcd(3,6), gcd(3,2), gcd(3,3) */
    CHECK_EQ(b->mul - a->mul, 1);           /* u = 1 * (p/h) */
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 0, P, V, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 2}; d2_row_is(c, 1, P, V, 2); }
    { sk_int P[] = {3}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 1); }
    CHECK_EQ(sk_d2_t_row_len(c, sk_d2_t_den_index(c, two)), 0); /* the row of d = 1 is empty */
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* A leftover at a piece of equal valuation in addition: base [6, 5]; 1/6 + 11/30 = 8/15, h = 2. */
static void test_d2_composite_add(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y, r;
    sk_d2_enter(c, 1, 6, &x);
    sk_d2_enter(c, 11, 30, &y);
    { sk_int B[] = {6, 5}; d2_base_is(c, B, 2); }
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, x, y, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 8, 15);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ref_splits - k0.c[SK_PH_ADD][SK_STORE].ref_splits, 1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_rows_refined - k0.c[SK_PH_ADD][SK_STORE].ev_rows_refined, 1);
    { sk_int B[] = {2, 5, 3}; d2_base_is(c, B, 3); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 0, P, V, 2); }
    { sk_int P[] = {2, 5, 3}; uint32_t V[] = {1, 1, 1}; d2_row_is(c, 1, P, V, 3); }
    { sk_int P[] = {5, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* h = 1 costs no division: base [2, 3, 5]; 1/6 + 1/15 = 7/30, equal piece 3 with h = gcd(7, 3) = 1. */
static void test_d2_h1(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y, r;
    sk_d2_enter(c, 1, 6, &x);
    sk_d2_enter(c, 1, 15, &y);
    { sk_int B[] = {2, 3, 5}; d2_base_is(c, B, 3); }
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, x, y, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 7, 30);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].gcd - k0.c[SK_PH_ADD][SK_CORE].gcd, 1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].div_exact - k0.c[SK_PH_ADD][SK_CORE].div_exact, 0); /* h = 1: no division */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].div_rem - k0.c[SK_PH_ADD][SK_STORE].div_rem, 0);   /* power(1, 3) */
    CHECK_EQ(k1.c[SK_PH_ADD][SK_STORE].ev_rows_u1 - k0.c[SK_PH_ADD][SK_STORE].ev_rows_u1, 1);
    { sk_int P[] = {2, 3, 5}; uint32_t V[] = {1, 1, 1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 3); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* Entering 12 over the base [6]: the scan divides by 6, stays at 6, then splits it, then divides by 2. */
static void test_d2_scan_stays(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y;
    sk_d2_enter(c, 1, 6, &x);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    sk_d2_enter(c, 1, 12, &y);
    sk_d2_counters(c, &k1);
    const sk_count *a = &k0.c[SK_PH_ENTRY][SK_STORE], *b = &k1.c[SK_PH_ENTRY][SK_STORE];
    CHECK_EQ(b->scan_refine_pieces - a->scan_refine_pieces, 3); /* 6, 6 again (split), then 2 */
    CHECK_EQ(b->ref_splits - a->ref_splits, 1);
    /* divisions with remainder: 12 by 6 (1 + failing 1), nu_2(6) (2), nu_3(6) (2), 2 by 2 (1 + failing 1) */
    CHECK_EQ(b->div_rem - a->div_rem, 8);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 1}; d2_row_is(c, 1, P, V, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 0, P, V, 2); }
    /* repeated division counts every attempt, the failing one included: 1/8 over the base [2, 3] */
    sk_d2_counters(c, &k0);
    sk_d2_enter(c, 1, 8, &y);
    sk_d2_counters(c, &k1);
    CHECK_EQ(k1.c[SK_PH_ENTRY][SK_STORE].div_rem - k0.c[SK_PH_ENTRY][SK_STORE].div_rem, 4);
    CHECK_EQ(k1.c[SK_PH_ENTRY][SK_STORE].scan_refine_pieces - k0.c[SK_PH_ENTRY][SK_STORE].scan_refine_pieces, 1);
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* More than 1,024 pieces: the scheduled full audit checks coprimality incrementally only (spec §13.3 item 2). */
static void test_d2_large_base(void) {
    sk_d2 *c = sk_d2_create();
    int n = 0;
    const char *why = NULL;
    for (sk_int p = 2; n < 1100; p++) {
        bool prime = true;
        for (sk_int q = 2; q * q <= p; q++)
            if (p % q == 0) {
                prime = false;
                break;
            }
        if (!prime) continue;
        sk_handle h;
        sk_d2_enter(c, 1, p, &h);
        n++;
        CHECK(sk_d2_audit_changed(c, &why) == SK_OK);
    }
    CHECK_EQ(sk_d2_t_C(c), 1100);
    CHECK(sk_d2_audit_scheduled(c, &why) == SK_OK);
    /* a piece broken after the last audit is caught by the scheduled audit's incremental check */
    sk_int keep = sk_d2_t_piece(c, 1050);
    sk_d2_t_corrupt_piece(c, 1050, 2 * 3);
    CHECK(sk_d2_audit_scheduled(c, &why) == SK_EAUDIT);
    sk_d2_t_corrupt_piece(c, 1050, keep);
    CHECK(sk_d2_audit(c, &why) == SK_OK);
    sk_d2_destroy(c);
}

/* ----------------------------------------------------------------------------------------- lookup, memory */

static void test_lookup_growth(void) {
    sk_d1 *c1 = sk_d1_create();
    sk_d2 *c2 = sk_d2_create();
    for (int k = 0; k < 9; k++) {
        sk_handle h;
        sk_d1_enter(c1, 1, 2 + k, &h);
        sk_d2_enter(c2, 1, 2 + k, &h);
        uint32_t want = k < 8 ? 16 : 32; /* at most half full */
        CHECK_EQ(sk_d1_t_S(c1), want);
    }
    sk_memory m;
    sk_d2_memory(c2, &m);
    CHECK_EQ(m.item[2].used, 4 * 32);
    for (int k = 0; k < 9; k++) { /* each found again, at its index */
        sk_handle h;
        sk_d1_enter(c1, 11, 2 + k, &h);
        CHECK_EQ(sk_d1_t_den_index(c1, h), k);
    }
    CHECK_EQ(sk_d1_t_D(c1), 9);
    audits_ok(&sk_row_d1, c1);
    audits_ok(&sk_row_d2, c2);
    sk_d1_destroy(c1);
    sk_d2_destroy(c2);
}

static uint64_t find_item(const sk_memory *m, const char *name) {
    for (unsigned t = 0; t < m->n; t++)
        if (!strcmp(m->item[t].name, name)) return m->item[t].used;
    CHECK(!"memory item missing");
    return 0;
}

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
    /* Design 1, after the note's first addition */
    sk_d1 *c1 = sk_d1_create();
    sk_handle a, b, r;
    sk_d1_enter(c1, 1, 12, &a);
    sk_d1_enter(c1, 5, 18, &b);
    sk_d1_enter(c1, 1, 30, &h);
    sk_d1_add(c1, a, b, &r);
    sk_d1_memory(c1, &m);
    uint64_t N = 4, D = 4, C = 2, S = 16, W = 1, bits = 2 + 2 + 1 + 1;
    CHECK_EQ(find_item(&m, "numerators"), N * (w + 4));
    CHECK_EQ(find_item(&m, "denominators"), D * w);
    CHECK_EQ(find_item(&m, "lookup"), 4 * S);
    CHECK_EQ(find_item(&m, "divisors"), C * w);
    CHECK_EQ(find_item(&m, "pair table"), 8 * ((D * (D - 1) + 127) / 128));
    CHECK_EQ(find_item(&m, "incidence bits"), 8 * D * W);
    CHECK_EQ(find_item(&m, "residual rows"), w * bits + 16 * D);
    CHECK_EQ(m.values, N);
    CHECK(m.used_per_rational == (double)m.used / (double)N);
    uint64_t sum = 0;
    for (unsigned t = 0; t < m.n; t++) {
        sum += m.item[t].used;
        CHECK(m.item[t].allocated >= m.item[t].used);
    }
    CHECK_EQ(sum, m.used);
    sk_d1_destroy(c1);
    /* Design 2: the same values; rows 12 {2,3}, 18 {2,3}, 30 {2,3,5}, 36 {2,3}: 9 entries */
    sk_d2 *c2 = sk_d2_create();
    sk_d2_enter(c2, 1, 12, &a);
    sk_d2_enter(c2, 5, 18, &b);
    sk_d2_enter(c2, 1, 30, &h);
    sk_d2_add(c2, a, b, &r);
    sk_d2_memory(c2, &m);
    CHECK_EQ(find_item(&m, "numerators"), N * (w + 4));
    CHECK_EQ(find_item(&m, "denominators"), D * w);
    CHECK_EQ(find_item(&m, "lookup"), 4 * S);
    CHECK_EQ(find_item(&m, "base"), 3 * w);
    CHECK_EQ(find_item(&m, "exponent rows"), 8 * 9 + 16 * D);
    sk_d2_destroy(c2);
}

/* ----------------------------------------------------------------------------------------- audits find failures */

static void test_audits_detect(void) {
    const char *why;
    sk_ctl *c0 = sk_ctl_create();
    sk_handle h, a, b, r;
    sk_ctl_enter(c0, 1, 2, &h);
    CHECK(sk_ctl_audit(c0, &why) == SK_OK);
    sk_ctl_t_corrupt(c0, h, 2, 4);
    CHECK(sk_ctl_audit(c0, &why) == SK_EAUDIT);
    CHECK(why != NULL);
    sk_ctl_destroy(c0);

    sk_d1 *c1 = sk_d1_create();
    sk_d1_enter(c1, 1, 12, &a);
    sk_d1_enter(c1, 1, 18, &b);
    sk_d1_add(c1, a, b, &r);
    CHECK(sk_d1_audit(c1, &why) == SK_OK);
    sk_d1_t_corrupt_residual(c1, 0, 1, 3); /* 6 * 3 != 12 */
    CHECK(sk_d1_audit(c1, &why) == SK_EAUDIT);
    sk_d1_t_corrupt_residual(c1, 0, 1, 2);
    CHECK(sk_d1_audit(c1, &why) == SK_OK);
    sk_handle e;
    sk_d1_enter(c1, 1, 30, &e);
    sk_d1_t_corrupt_pair(c1, 1, 3); /* pair (18, 30) marked compared, but its gcd 6 is not in row 30 */
    CHECK(sk_d1_audit_changed(c1, &why) == SK_EAUDIT);
    CHECK(sk_d1_audit(c1, &why) == SK_EAUDIT);
    sk_d1_destroy(c1);

    sk_d2 *c2 = sk_d2_create();
    sk_d2_enter(c2, 1, 12, &a);
    sk_d2_enter(c2, 1, 18, &b);
    CHECK(sk_d2_audit(c2, &why) == SK_OK);
    sk_d2_t_corrupt_nu(c2, 0, 0, 3); /* 2^3 * 3 != 12 */
    CHECK(sk_d2_audit_changed(c2, &why) == SK_EAUDIT);
    CHECK(sk_d2_audit(c2, &why) == SK_EAUDIT);
    sk_d2_t_corrupt_nu(c2, 0, 0, 2);
    CHECK(sk_d2_audit(c2, &why) == SK_OK);
    sk_d2_t_corrupt_piece(c2, 1, 4); /* pieces 2 and 4 */
    CHECK(sk_d2_audit_changed(c2, &why) == SK_EAUDIT);
    sk_d2_destroy(c2);

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
    for (int t = 0; t < 3; t++) {
        test_entry_and_errors(ROWS[t]);
        test_zero_and_signs(ROWS[t]);
        test_fast_paths(ROWS[t]);
        test_note_values(ROWS[t]);
        test_overflow(ROWS[t]);
    }
    test_counting_rules_ctl();
    test_d1_note();
    test_d1_islands();
    test_d1_incidence();
    test_d1_stride();
    test_d1_counting();
    test_d2_note();
    test_d2_composite_mul();
    test_d2_composite_add();
    test_d2_h1();
    test_d2_scan_stays();
    test_d2_large_base();
    test_lookup_growth();
    test_memory();
    test_audits_detect();
    test_ceil();
    test_rows_agree();
    printf("unit (width %d): %d checks, %d failures\n", SK_WIDTH, checks, failures);
    return failures ? 1 : 0;
}
