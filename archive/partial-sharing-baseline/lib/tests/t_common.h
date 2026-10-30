/* t_common.h: the unit tests' checks, helpers and per-row tests, shared by the baseline's tests (this directory) and
 * by each design's (08-common-divisors/lib/tests, 09-coprime-base/lib/tests).  Split out of the joint unit.c on
 * Oct 29, 2026, when Designs 1 and 2 became separate specifications; no check was changed.  Each test program
 * includes this header once. */
#ifndef SK_T_COMMON_H
#define SK_T_COMMON_H
#ifndef SK_AUDIT
#error "the unit tests need the test build (-DSK_COUNT -DSK_AUDIT)"
#endif

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "ref.h"
#include "sk_row.h"

#pragma GCC diagnostic ignored "-Wunused-function" /* not every program uses every helper */

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

static uint64_t find_item(const sk_memory *m, const char *name) {
    for (unsigned t = 0; t < m->n; t++)
        if (!strcmp(m->item[t].name, name)) return m->item[t].used;
    CHECK(!"memory item missing");
    return 0;
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

#endif
