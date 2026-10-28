/* unit.c: unit tests for primegraph M1.  Every test audits the graph (pg_audit) after its steps; the debug build
 * also audits inside the library after every operation.  Exit status 0 when every check passes. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "primegraph.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); } } while (0)
#define AUDIT(g) do { const char *m_ = pg_audit(g); checks++; if (m_) { fails++; fprintf(stderr, "%s:%d: audit: %s\n", __FILE__, __LINE__, m_); } } while (0)

static int is(pg_graph *g, const pg_value *v, int64_t n, int64_t d) {
    int64_t a, b;
    pg_value_get(g, v, &a, &b);
    if (a != n || b != d) fprintf(stderr, "  got %" PRId64 "/%" PRId64 ", want %" PRId64 "/%" PRId64 "\n", a, b, n, d);
    return a == n && b == d;
}
static pg_value *V(pg_graph *g, int64_t n, int64_t d) {
    pg_value *v = NULL;
    if (pg_value_new(g, n, d, &v) != PG_OK) { fprintf(stderr, "V(%" PRId64 ", %" PRId64 ") failed\n", n, d); exit(1); }
    return v;
}
static int nodes_are(pg_graph *g, const uint64_t *want, size_t n) {
    uint64_t got[64];
    size_t k = pg_debug_nodes(g, got, 64);
    int ok = k == n;
    for (size_t i = 0; ok && i < n; i++) ok = got[i] == want[i];
    if (!ok) {
        fprintf(stderr, "  nodes:");
        for (size_t i = 0; i < k && i < 64; i++) fprintf(stderr, " %" PRIu64, got[i]);
        fprintf(stderr, "\n");
    }
    return ok;
}
static pg_stats S(pg_graph *g) { pg_stats s; pg_stats_get(g, &s); return s; }

/* ---------------------------------------------------------------- entry and splits (the spec's examples) */

static void test_entry_examples(void) {
    pg_graph *g = pg_graph_create();
    pg_value *a = V(g, 1, 30), *b = V(g, 1, 45);  /* 30 then 45 gives nodes 2, 3, 5 */
    CHECK(nodes_are(g, (uint64_t[]){2, 3, 5}, 3));
    CHECK(is(g, a, 1, 30) && is(g, b, 1, 45));
    AUDIT(g);
    pg_graph_destroy(g);

    g = pg_graph_create();
    a = V(g, 1, 30);
    b = V(g, 1, 900);  /* 900 = 30^2 splits nothing */
    CHECK(nodes_are(g, (uint64_t[]){30}, 1));
    CHECK(S(g).splits_entry == 0);
    AUDIT(g);
    pg_graph_destroy(g);

    g = pg_graph_create();
    a = V(g, 1, 100);  /* one node 10, weight 2 */
    CHECK(nodes_are(g, (uint64_t[]){10}, 1));
    b = V(g, 1, 35);   /* 100 then 35 gives 2, 5, 7 */
    CHECK(nodes_are(g, (uint64_t[]){2, 5, 7}, 3));
    AUDIT(g);
    pg_value_release(g, a);  /* 2 is freed with its last holder (S5); 5 and 7 stay */
    CHECK(nodes_are(g, (uint64_t[]){5, 7}, 2));
    pg_value_release(g, b);
    CHECK(S(g).nodes_live == 0 && S(g).holders_live == 0);
    AUDIT(g);
    pg_graph_destroy(g);

    g = pg_graph_create();  /* a split rewrites every holder of the node (copy and loop) */
    pg_value *vs[200];
    for (int i = 0; i < 200; i++) vs[i] = V(g, 210 * i + 1, 210);  /* numerators prime to 2*3*5*7 */
    CHECK(S(g).nodes_live > 0);
    pg_value *c = V(g, 1, 4 * 9);  /* 36 = 2^2 3^2: splits 210 into 6 and 35 */
    CHECK(nodes_are(g, (uint64_t[]){6, 35}, 2));
    CHECK(S(g).split_holders >= 200);
    AUDIT(g);
    for (int i = 0; i < 200; i++) {
        int64_t n, d;
        pg_value_get(g, vs[i], &n, &d);
        CHECK(d == 210);
    }
    pg_value_release(g, c);
    for (int i = 0; i < 200; i++) pg_value_release(g, vs[i]);
    CHECK(S(g).nodes_live == 0);
    AUDIT(g);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- reduction: the g = N and g < N cases */

static void test_reduction(void) {
    pg_graph *g = pg_graph_create();
    pg_value *a = V(g, 1, 30), *thirty = V(g, 30, 1), *two = V(g, 2, 1), *r = NULL;
    CHECK(pg_mul(g, a, thirty, &r) == PG_OK && is(g, r, 1, 1));  /* g = N: divides, no split */
    CHECK(S(g).splits_reduce == 0 && nodes_are(g, (uint64_t[]){30}, 1));
    pg_value_release(g, r);
    CHECK(pg_mul(g, a, two, &r) == PG_OK && is(g, r, 1, 15));   /* g = 2 < N: 30 splits into 2 and 15 */
    CHECK(S(g).splits_reduce >= 1 && nodes_are(g, (uint64_t[]){2, 15}, 2));
    AUDIT(g);
    pg_value_release(g, r);
    /* reduction at entry: 6/12 enters 12 (node 2^2*3 = 12, one node), then cancels 6 = 2*3 (g = 6 < 12: split) */
    pg_value *h = V(g, 6, 12);
    CHECK(is(g, h, 1, 2));
    AUDIT(g);
    /* a node with an exponent above 1: 1/(2^3 3) + 1/(2^3 3) = 1/12 */
    pg_value *x = V(g, 1, 24), *y = NULL;
    CHECK(pg_add(g, x, x, &y) == PG_OK && is(g, y, 1, 12));
    AUDIT(g);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- arithmetic and errors */

static void test_arith(void) {
    pg_graph *g = pg_graph_create();
    pg_value *h = V(g, 1, 2), *t = V(g, 1, 3), *r = NULL;
    CHECK(pg_add(g, h, t, &r) == PG_OK && is(g, r, 5, 6)); pg_value_release(g, r);
    CHECK(pg_sub(g, h, t, &r) == PG_OK && is(g, r, 1, 6)); pg_value_release(g, r);
    CHECK(pg_sub(g, t, h, &r) == PG_OK && is(g, r, -1, 6)); pg_value_release(g, r);
    CHECK(pg_mul(g, h, t, &r) == PG_OK && is(g, r, 1, 6)); pg_value_release(g, r);
    CHECK(pg_div(g, h, t, &r) == PG_OK && is(g, r, 3, 2)); pg_value_release(g, r);
    CHECK(pg_sub(g, h, h, &r) == PG_OK && is(g, r, 0, 1) && pg_is_zero(r) && pg_sign(r) == 0); pg_value_release(g, r);
    CHECK(pg_neg(g, t, &r) == PG_OK && is(g, r, -1, 3) && pg_sign(r) == -1); pg_value_release(g, r);
    CHECK(pg_cmp(g, h, t) == 1 && pg_cmp(g, t, h) == -1 && pg_cmp(g, h, h) == 0);
    pg_value *m = V(g, -4, -6);  /* signs on both parts */
    CHECK(is(g, m, 2, 3));
    pg_value *m2 = V(g, 4, -6);
    CHECK(is(g, m2, -2, 3));
    CHECK(pg_value_new(g, 1, 0, &r) == PG_DIVZERO);
    pg_value *zero = V(g, 0, 7);
    CHECK(is(g, zero, 0, 1));
    CHECK(pg_div(g, h, zero, &r) == PG_DIVZERO);
    CHECK(pg_div(g, zero, h, &r) == PG_OK && is(g, r, 0, 1)); pg_value_release(g, r);

    /* overflow: reported exactly when the reduced result does not fit */
    pg_value *mx = V(g, INT64_MAX, 1), *one = V(g, 1, 1), *mn = V(g, INT64_MIN, 1), *two = V(g, 2, 1);
    CHECK(pg_add(g, mx, one, &r) == PG_OVERFLOW);
    CHECK(pg_neg(g, mn, &r) == PG_OVERFLOW);
    CHECK(pg_sub(g, mn, one, &r) == PG_OVERFLOW);
    pg_value *p62 = V(g, INT64_C(1) << 62, 1), *n62 = V(g, -(INT64_C(1) << 62), 1);
    CHECK(pg_mul(g, p62, two, &r) == PG_OVERFLOW);
    CHECK(pg_mul(g, n62, two, &r) == PG_OK && is(g, r, INT64_MIN, 1)); pg_value_release(g, r);
    CHECK(pg_value_new(g, 1, INT64_MIN, &r) == PG_OVERFLOW);             /* -1/2^63 */
    CHECK(pg_value_new(g, 2, INT64_MIN, &r) == PG_OK && is(g, r, -1, INT64_C(1) << 62)); pg_value_release(g, r);
    CHECK(pg_value_new(g, INT64_MIN, -1, &r) == PG_OVERFLOW);            /* 2^63 */
    CHECK(pg_value_new(g, INT64_MIN, INT64_MIN, &r) == PG_OK && is(g, r, 1, 1)); pg_value_release(g, r);
    CHECK(pg_div(g, two, mn, &r) == PG_OK && is(g, r, -1, INT64_C(1) << 62)); pg_value_release(g, r);
    CHECK(pg_div(g, one, mn, &r) == PG_OVERFLOW);
    CHECK(pg_div(g, mn, mn, &r) == PG_OK && is(g, r, 1, 1)); pg_value_release(g, r);
    /* exactness: the common denominator 15 E (E prime, 5 E < 2^63) is above 2^63 but the sum is 1/15 */
    int64_t E = INT64_C(1844674407370955143);
    pg_value *a = V(g, 2, 3 * E), *b = V(g, INT64_C(614891469123651711), 5 * E);
    CHECK(pg_add(g, a, b, &r) == PG_OK && is(g, r, 1, 15)); pg_value_release(g, r);
    CHECK(pg_cmp(g, a, b) == -1);
    CHECK(pg_sub(g, a, a, &r) == PG_OK && is(g, r, 0, 1)); pg_value_release(g, r);
    /* a product of lowest-terms values in lowest terms: cross-cancellation */
    pg_value *c = V(g, 6, 35), *d = V(g, 7, 4);
    CHECK(pg_mul(g, c, d, &r) == PG_OK && is(g, r, 3, 10)); pg_value_release(g, r);
    CHECK(pg_div(g, c, d, &r) == PG_OK && is(g, r, 24, 245)); pg_value_release(g, r);
    /* clone */
    CHECK(pg_value_clone(g, c, &r) == PG_OK && is(g, r, 6, 35)); pg_value_release(g, r);
    AUDIT(g);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- large primes: rho, the dictionary's hash */

static void test_large(void) {
    pg_graph *g = pg_graph_create();
    int64_t p = 2147483693, q = 2147484679;  /* primes above 2^31: p*q needs rho */
    pg_value *a = V(g, 1, p * q);
    CHECK(S(g).rho_calls == 1 && nodes_are(g, (uint64_t[]){(uint64_t)(p * q)}, 1));
    pg_value *b = V(g, 1, p);                /* splits p*q into p and q */
    CHECK(nodes_are(g, (uint64_t[]){(uint64_t)p, (uint64_t)q}, 2));
    pg_value *r = NULL;
    CHECK(pg_add(g, a, b, &r) == PG_OK && is(g, r, 1 + q, p * q));
    pg_value_release(g, r);
    AUDIT(g);
    pg_value_release(g, a);
    pg_value_release(g, b);
    CHECK(S(g).nodes_live == 0 && S(g).dict_entries == 0);
    /* a prime above 2^62, and churn through the dictionary's hash (deletions with backward shift) */
    pg_value *big = V(g, 3, INT64_C(9223372036854775783));
    CHECK(is(g, big, 3, INT64_C(9223372036854775783)));
    for (int round = 0; round < 5; round++) {
        pg_value *vs[300];
        for (int i = 0; i < 300; i++) vs[i] = V(g, 1, 1000003 + 2 * i);
        AUDIT(g);
        for (int i = 0; i < 300; i += 2) pg_value_release(g, vs[i]);
        AUDIT(g);
        for (int i = 1; i < 300; i += 2) pg_value_release(g, vs[i]);
    }
    pg_value_release(g, big);
    CHECK(S(g).nodes_live == 0 && S(g).dict_entries == 0 && S(g).holders_live == 0);
    AUDIT(g);
    /* the factor cache: the same large denominator again */
    size_t hits = S(g).den_cache_hits;
    pg_value *c1 = V(g, 1, p * 3), *c2 = V(g, 5, p * 3);
    CHECK(S(g).den_cache_hits == hits + 1);
    pg_value_release(g, c1);
    pg_value_release(g, c2);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- text */

static void test_text(void) {
    pg_graph *g = pg_graph_create();
    char buf[PG_STRING_MAX];
    pg_value *v = NULL;
    CHECK(pg_value_from_string(g, "-6/8", &v) == PG_OK && is(g, v, -3, 4));
    CHECK(pg_value_to_string(g, v, buf, sizeof buf) == PG_OK && !strcmp(buf, "-3/4"));
    CHECK(pg_value_to_string(g, v, buf, 4) == PG_INVALID);
    pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "42", &v) == PG_OK && is(g, v, 42, 1));
    CHECK(pg_value_to_string(g, v, buf, sizeof buf) == PG_OK && !strcmp(buf, "42"));
    pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "+3/-9", &v) == PG_OK && is(g, v, -1, 3)); pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "-0", &v) == PG_OK && is(g, v, 0, 1)); pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "18446744073709551616/4", &v) == PG_OK && is(g, v, INT64_C(1) << 62, 1));
    pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "-9223372036854775808", &v) == PG_OK && is(g, v, INT64_MIN, 1));
    CHECK(pg_value_to_string(g, v, buf, sizeof buf) == PG_OK && !strcmp(buf, "-9223372036854775808"));
    pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "9223372036854775808", &v) == PG_OVERFLOW);
    CHECK(pg_value_from_string(g, "1/9223372036854775808", &v) == PG_OVERFLOW);
    CHECK(pg_value_from_string(g, "2/9223372036854775808", &v) == PG_OK && is(g, v, 1, INT64_C(1) << 62));
    pg_value_release(g, v);
    CHECK(pg_value_from_string(g, "1/0", &v) == PG_DIVZERO);
    const char *bad[] = {"", "-", "1/", "/2", "1/2/3", " 1", "1 ", "0x10", "1.5", "a", "--1", "1/+-2",
                         "123456789012345678901234567890123456789", NULL};
    for (int i = 0; bad[i]; i++) CHECK(pg_value_from_string(g, bad[i], &v) == PG_INVALID);
    pg_value *w = V(g, -INT64_MAX, INT64_MAX - 1);
    CHECK(pg_value_to_string(g, w, buf, sizeof buf) == PG_OK && strlen(buf) + 1 <= PG_STRING_MAX);
    AUDIT(g);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- arrays */

static void get(pg_graph *g, const pg_array *a, size_t i, int64_t *n, int64_t *d) { pg_array_get_frac(g, a, i, n, d); }

static void test_arrays(void) {
    pg_graph *g = pg_graph_create();
    int64_t n1[] = {123, -5, 7, 250, 0}, d1[] = {100, 100, 10, 100, 1};
    pg_array *a = NULL, *b = NULL, *r = NULL;
    pg_array_info inf;
    CHECK(pg_array_from_fracs(g, n1, d1, 5, &a) == PG_OK);
    pg_array_info_get(a, &inf);
    CHECK(inf.n == 5 && inf.D == 100 && inf.exceptions == 0);
    int64_t n, d;
    get(g, a, 3, &n, &d);
    CHECK(n == 5 && d == 2);
    get(g, a, 2, &n, &d);
    CHECK(n == 7 && d == 10);
    /* D lowered by I4: every numerator even over D = 4 */
    int64_t n2[] = {2, 6, 10}, d2[] = {4, 4, 4};
    CHECK(pg_array_from_fracs(g, n2, d2, 3, &b) == PG_OK);
    pg_array_info_get(b, &inf);
    CHECK(inf.D == 2 && inf.exceptions == 0);
    pg_array_release(g, b);
    AUDIT(g);
    /* the lcm overflows: the most frequent denominator wins and the rest are exceptions */
    int64_t P1 = 1000000007, P2 = 1000000009, P3 = 998244353;
    int64_t n3[] = {1, 2, 3, 4, 5}, d3[] = {P1, P1, P2, P3, P1};
    CHECK(pg_array_from_fracs(g, n3, d3, 5, &b) == PG_OK);
    pg_array_info_get(b, &inf);
    CHECK(inf.D == P1 && inf.exceptions == 2);
    get(g, b, 3, &n, &d);
    CHECK(n == 4 && d == P3);
    pg_value *s = NULL;
    CHECK(pg_array_sum(g, b, &s) == PG_OVERFLOW);  /* (8 P2 P3 + 3 P1 P3 + 4 P1 P2) / (P1 P2 P3) is above 2^63 */
    pg_array_release(g, b);
    AUDIT(g);
    /* elementwise against hand results */
    int64_t n4[] = {1, 1, 3, -1, 0}, d4[] = {2, 3, 5, 100, 1};
    CHECK(pg_array_from_fracs(g, n4, d4, 5, &b) == PG_OK);
    CHECK(pg_array_add(g, a, b, &r) == PG_OK);
    int64_t want_add[][2] = {{173, 100}, {17, 60}, {13, 10}, {249, 100}, {0, 1}};
    for (int i = 0; i < 5; i++) { get(g, r, (size_t)i, &n, &d); CHECK(n == want_add[i][0] && d == want_add[i][1]); }
    pg_array_release(g, r);
    CHECK(pg_array_mul(g, a, b, &r) == PG_OK);
    int64_t want_mul[][2] = {{123, 200}, {-1, 60}, {21, 50}, {-1, 40}, {0, 1}};
    for (int i = 0; i < 5; i++) { get(g, r, (size_t)i, &n, &d); CHECK(n == want_mul[i][0] && d == want_mul[i][1]); }
    pg_array_release(g, r);
    CHECK(pg_array_div(g, a, b, &r) == PG_DIVZERO);  /* b[4] = 0 */
    CHECK(pg_array_div(g, b, a, &r) == PG_DIVZERO);  /* a[4] = 0 */
    CHECK(pg_array_sum(g, a, &s) == PG_OK && is(g, s, 219, 50)); pg_value_release(g, s);
    CHECK(pg_array_dot(g, a, b, &s) == PG_OK);  /* 123/200 - 1/60 + 21/50 - 1/40 + 0 */
    CHECK(is(g, s, 149, 150)); pg_value_release(g, s);
    pg_value *e = NULL;
    CHECK(pg_array_get(g, a, 1, &e) == PG_OK && is(g, e, -1, 20)); pg_value_release(g, e);
    CHECK(pg_array_get(g, a, 9, &e) == PG_INVALID);
    int64_t on[5], od[5];
    CHECK(pg_array_to_fracs(g, a, on, od) == PG_OK && on[0] == 123 && od[0] == 100 && on[4] == 0 && od[4] == 1);
    AUDIT(g);
    /* division with nonzero elements */
    int64_t n5[] = {2, -3, 5, 7, 1}, d5[] = {3, 4, 1, 100, 9};
    pg_array *c = NULL;
    CHECK(pg_array_from_fracs(g, n5, d5, 5, &c) == PG_OK);
    CHECK(pg_array_div(g, a, c, &r) == PG_OK);
    int64_t want_div[][2] = {{369, 200}, {1, 15}, {7, 50}, {250, 7}, {0, 1}};
    for (int i = 0; i < 5; i++) { get(g, r, (size_t)i, &n, &d); CHECK(n == want_div[i][0] && d == want_div[i][1]); }
    pg_array_release(g, r);
    CHECK(pg_array_fma(g, a, b, c, &r) == PG_OK);  /* a*b + c */
    int64_t want_fma[][2] = {{769, 600}, {-23, 30}, {271, 50}, {9, 200}, {1, 9}};
    for (int i = 0; i < 5; i++) { get(g, r, (size_t)i, &n, &d); CHECK(n == want_fma[i][0] && d == want_fma[i][1]); }
    pg_array_release(g, r);
    CHECK(pg_array_sub(g, a, a, &r) == PG_OK);
    pg_array_info_get(r, &inf);
    CHECK(inf.D == 1 && inf.exceptions == 0);
    pg_array_release(g, r);
    /* from values */
    pg_value *vals[3] = {V(g, 1, 6), V(g, 5, 4), V(g, -7, 1)};
    CHECK(pg_array_from_values(g, vals, 3, &r) == PG_OK);
    pg_array_info_get(r, &inf);
    CHECK(inf.D == 12 && inf.exceptions == 0);
    get(g, r, 1, &n, &d);
    CHECK(n == 5 && d == 4);
    for (int i = 0; i < 3; i++) pg_value_release(g, vals[i]);
    pg_array_release(g, r);
    /* lengths must match */
    int64_t n6[] = {1}, d6[] = {1};
    pg_array *one = NULL;
    CHECK(pg_array_from_fracs(g, n6, d6, 1, &one) == PG_OK);
    CHECK(pg_array_add(g, a, one, &r) == PG_INVALID);
    pg_array_release(g, one);
    AUDIT(g);
    pg_array_release(g, a);
    pg_array_release(g, b);
    pg_array_release(g, c);
    CHECK(S(g).nodes_live == 0 && S(g).holders_live == 0 && S(g).arrays_live == 0);
    AUDIT(g);
    pg_graph_destroy(g);
}

/* overflowing dense slots go to the graph path; an element whose exact result does not fit fails the operation */
static void test_array_overflow(void) {
    pg_graph *g = pg_graph_create();
    int64_t big = INT64_C(1) << 62;
    int64_t na[] = {big, 3, big}, da[] = {3, 7, 1}, nb[] = {big, 5, -big}, db[] = {5, 7, 1};
    pg_array *a = NULL, *b = NULL, *r = NULL;
    CHECK(pg_array_from_fracs(g, na, da, 3, &a) == PG_OK);
    CHECK(pg_array_from_fracs(g, nb, db, 3, &b) == PG_OK);
    CHECK(pg_array_sub(g, a, b, &r) == PG_OVERFLOW);  /* 2^62 - (-2^62) = 2^63 */
    CHECK(pg_array_add(g, a, b, &r) == PG_OVERFLOW);   /* element 0: 2^65 / 15 */
    CHECK(pg_array_mul(g, a, b, &r) == PG_OVERFLOW);   /* 2^124 / 15 */
    pg_value *s = NULL;
    pg_array *c = NULL;
    int64_t nc[] = {big, big, -big, -big / 2, 1}, dc[] = {1, 1, 1, 1, 3};
    CHECK(pg_array_from_fracs(g, nc, dc, 5, &c) == PG_OK);
    CHECK(pg_array_sum(g, c, &s) == PG_OK);            /* partial sums pass 2^63; the sum 2^61 + 1/3 does not */
    CHECK(s && pg_sign(s) == 1);
    int64_t n, d;
    if (s) pg_value_get(g, s, &n, &d);
    CHECK(s && n == 3 * (INT64_C(1) << 61) + 1 && d == 3);
    pg_value_release(g, s);
    pg_array_release(g, c);
    AUDIT(g);
    pg_array_release(g, a);
    pg_array_release(g, b);
    CHECK(S(g).holders_live == 0);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- expressions (V4) */

static void test_expr(void) {
    pg_graph *g = pg_graph_create();
    int64_t na[] = {1, 2, 3, 4}, da[] = {10, 10, 10, 10}, nb[] = {5, 5, 5, 5}, db[] = {2, 2, 2, 2};
    pg_array *a = NULL, *b = NULL, *t = NULL, *r = NULL;
    pg_array_from_fracs(g, na, da, 4, &a);
    pg_array_from_fracs(g, nb, db, 4, &b);
    pg_array_info inf;
    CHECK(pg_expr_end(g) == PG_INVALID);
    CHECK(pg_expr_begin(g) == PG_OK);
    CHECK(pg_array_mul(g, a, b, &t) == PG_OK);  /* 5/20 ... unreduced: D = 20 */
    pg_array_info_get(t, &inf);
    CHECK(inf.deferred == 1 && inf.D == 20);
    CHECK(pg_array_add(g, t, t, &r) == PG_OK);
    AUDIT(g);  /* deferred arrays are exempt from I4 */
    int64_t n, d;
    pg_array_get_frac(g, r, 1, &n, &d);  /* reads are reduced even inside */
    CHECK(n == 1 && d == 1);
    pg_array_release(g, t);
    CHECK(pg_expr_begin(g) == PG_OK);  /* nested */
    CHECK(pg_expr_end(g) == PG_OK);
    pg_array_info_get(r, &inf);
    CHECK(inf.deferred == 1);
    CHECK(pg_expr_end(g) == PG_OK);
    pg_array_info_get(r, &inf);
    CHECK(inf.deferred == 0 && inf.D == 2);  /* 10/20, 20/20, 30/20, 40/20 reduced per array: over 2 */
    pg_array_get_frac(g, r, 2, &n, &d);
    CHECK(n == 3 && d == 2);
    AUDIT(g);
    /* an overflow inside an expression: reduce the operands, retry */
    int64_t big = INT64_C(1) << 61;
    int64_t nc[] = {big, big}, dc[] = {3, 3}, nd[] = {3, 6}, dd[] = {4, 4};
    pg_array *c = NULL, *e = NULL, *f = NULL;
    pg_array_from_fracs(g, nc, dc, 2, &c);
    pg_array_from_fracs(g, nd, dd, 2, &e);
    pg_expr_begin(g);
    CHECK(pg_array_mul(g, c, e, &f) == PG_OK);  /* 2^61 * 3 overflows dense, then fits once reduced: 2^61/4... */
    CHECK(pg_expr_end(g) == PG_OK);
    pg_array_get_frac(g, f, 0, &n, &d);
    CHECK(n == INT64_C(1) << 59 && d == 1);
    pg_array_get_frac(g, f, 1, &n, &d);
    CHECK(n == INT64_C(1) << 60 && d == 1);
    pg_array_release(g, f);
    pg_array_release(g, a); pg_array_release(g, b); pg_array_release(g, r);
    pg_array_release(g, c); pg_array_release(g, e);
    AUDIT(g);
    CHECK(S(g).holders_live == 0 && S(g).nodes_live == 0);
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- lifetimes: churn, no leaks, bounded nodes */

static void test_churn(void) {
    pg_graph *g = pg_graph_create();
    size_t base = S(g).mem_bytes;
    uint64_t x = 12345;
    pg_value *live[512] = {0};
    for (int step = 0; step < 20000; step++) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        int k = (int)(x % 512);
        if (live[k]) { pg_value_release(g, live[k]); live[k] = NULL; }
        int64_t den = (int64_t)(x >> 40) % 5000 + 1, num = (int64_t)(x >> 20) % 100000 - 50000;
        if (x & 1) den *= 1000003;
        live[k] = V(g, num, den);
        if (step % 2000 == 0) AUDIT(g);
    }
    for (int k = 0; k < 512; k++) pg_value_release(g, live[k]);
    pg_stats s = S(g);
    CHECK(s.nodes_live == 0 && s.holders_live == 0 && s.edges_live == 0);
    CHECK(s.node_slots < 4096);  /* freed ids are reused: the table is bounded by the live values (S5) */
    AUDIT(g);
    (void)base;
    pg_graph_destroy(g);
}

/* ---------------------------------------------------------------- review fixes */

/* 1/INT64_MIN: |den| = 2^63 is never D and never dense; it fails exactly when it does not reduce to fit */
static void test_den_2_63(void) {
    pg_graph *g = pg_graph_create();
    pg_array *a = NULL;
    int64_t n1[] = {1}, d1[] = {INT64_MIN};
    CHECK(pg_array_from_fracs(g, n1, d1, 1, &a) == PG_OVERFLOW && a == NULL);
    int64_t n2[] = {2, 1, 6}, d2[] = {INT64_MIN, INT64_MIN, INT64_MIN};
    CHECK(pg_array_from_fracs(g, n2, d2, 3, &a) == PG_OVERFLOW && a == NULL);
    int64_t n3[] = {2, 6, -4, 3}, d3[] = {INT64_MIN, INT64_MIN, INT64_MIN, 5};
    CHECK(pg_array_from_fracs(g, n3, d3, 4, &a) == PG_OK);  /* -1/2^62, -3/2^62, 1/2^61, 3/5 */
    pg_array_info inf;
    pg_array_info_get(a, &inf);
    CHECK(inf.D > 0 && (uint64_t)inf.D < (UINT64_C(1) << 63));
    int64_t n, d;
    pg_array_get_frac(g, a, 1, &n, &d);
    CHECK(n == -3 && d == INT64_C(1) << 62);
    AUDIT(g);
    pg_array_release(g, a);
    pg_graph_destroy(g);
}

/* a common denominator above 2^64 that cancels to a fitting result (the wide reduction, fin_w) */
static void test_wide_cancel(void) {
    pg_graph *g = pg_graph_create();
    int64_t E = INT64_C(1844674407370955143);  /* prime; 5 E < 2^63 and 15 E > 2^64 */
    CHECK((unsigned __int128)15 * (uint64_t)E > ((unsigned __int128)1 << 64));
    pg_value *a = V(g, 2, 3 * E), *b = V(g, INT64_C(614891469123651711), 5 * E), *r = NULL;
    CHECK(pg_add(g, a, b, &r) == PG_OK && is(g, r, 1, 15)); pg_value_release(g, r);
    pg_value *c = V(g, -INT64_C(614891469123651711), 5 * E);
    CHECK(pg_sub(g, a, c, &r) == PG_OK && is(g, r, 1, 15)); pg_value_release(g, r);
    pg_value *x = V(g, 1, 3 * E), *y = V(g, 1, 5 * E);
    CHECK(pg_add(g, x, y, &r) == PG_OVERFLOW && r == NULL);  /* 8 / (15 E): genuinely too wide */
    AUDIT(g);
    pg_graph_destroy(g);
}

/* the accumulator's reduce-and-retry (wadd), and sums whose partial sums overflow in one order only (I5) */
static void test_sum_retry_and_order(void) {
    pg_graph *g = pg_graph_create();
    int64_t q1 = INT64_C(4611686018427387847), q2 = INT64_C(4611686018427387817), p62 = INT64_C(1) << 62;
    /* dense 1/2^62 + (2^62 - 1)/2^62 = 1 (unreduced over 2^62), then 1/q1, 1/q2, -1/q1, -1/q2: the second add
     * overflows 128 bits until the accumulator is reduced; the sum is 1 */
    int64_t n1[] = {1, p62 - 1, 1, 1, -1, -1}, d1[] = {p62, p62, q1, q2, q1, q2};
    pg_array *a = NULL;
    CHECK(pg_array_from_fracs(g, n1, d1, 6, &a) == PG_OK);
    size_t r0 = S(g).v4_retries;
    pg_value *s = NULL;
    CHECK(pg_array_sum(g, a, &s) == PG_OK && is(g, s, 1, 1));
    CHECK(S(g).v4_retries > r0);
    pg_value_release(g, s);
    pg_array_release(g, a);
    /* twelve values +-1/p (p near 2^40, six primes): 0 in every order */
    int64_t P[6] = {INT64_C(1099511627791), INT64_C(1099511627803), INT64_C(1099511627831),
                    INT64_C(1099511627873), INT64_C(1099511627891), INT64_C(1099511627917)};
    int64_t na[12], da[12], nb[12], db[12];
    for (int i = 0; i < 6; i++) {
        na[2 * i] = 1; da[2 * i] = P[i]; na[2 * i + 1] = -1; da[2 * i + 1] = P[i];  /* paired */
        nb[i] = 1; db[i] = P[i]; nb[6 + i] = -1; db[6 + i] = P[i];                  /* all + first */
    }
    pg_array *x = NULL, *y = NULL;
    CHECK(pg_array_from_fracs(g, na, da, 12, &x) == PG_OK && pg_array_from_fracs(g, nb, db, 12, &y) == PG_OK);
    CHECK(pg_array_sum(g, x, &s) == PG_OK && is(g, s, 0, 1)); pg_value_release(g, s);
    CHECK(pg_array_sum(g, y, &s) == PG_OK && is(g, s, 0, 1)); pg_value_release(g, s);
    /* dot: x . ones and y . ones; and a sum that really overflows still does */
    int64_t on[12], od[12];
    for (int i = 0; i < 12; i++) { on[i] = 1; od[i] = 1; }
    pg_array *ones = NULL;
    pg_array_from_fracs(g, on, od, 12, &ones);
    CHECK(pg_array_dot(g, y, ones, &s) == PG_OK && is(g, s, 0, 1)); pg_value_release(g, s);
    int64_t nc[6], dc[6];
    for (int i = 0; i < 6; i++) { nc[i] = 1; dc[i] = P[i]; }
    pg_array *z = NULL;
    pg_array_from_fracs(g, nc, dc, 6, &z);
    CHECK(pg_array_sum(g, z, &s) == PG_OVERFLOW && s == NULL);
    pg_array_release(g, x); pg_array_release(g, y); pg_array_release(g, z); pg_array_release(g, ones);
    AUDIT(g);
    CHECK(S(g).holders_live == 0);
    pg_graph_destroy(g);
}

/* released handles: PG_INVALID while the holder is not reused; outputs NULL on every failure */
static void test_released(void) {
    pg_graph *g = pg_graph_create();
    pg_value *a = V(g, 1, 3), *b = V(g, 2, 5), *r = (pg_value *)1;
    pg_value_release(g, a);
    CHECK(pg_add(g, a, b, &r) == PG_INVALID && r == NULL);
    r = (pg_value *)1;
    CHECK(pg_mul(g, b, a, &r) == PG_INVALID && r == NULL);
    CHECK(pg_div(g, b, a, &r) == PG_INVALID && r == NULL);
    CHECK(pg_neg(g, a, &r) == PG_INVALID && r == NULL);
    CHECK(pg_value_clone(g, a, &r) == PG_INVALID && r == NULL);
    CHECK(pg_cmp(g, a, b) == PG_CMP_INVALID);
    int64_t n = 7, d = 7;
    CHECK(pg_value_get(g, a, &n, &d) == PG_INVALID && n == 7);
    char buf[PG_STRING_MAX];
    CHECK(pg_value_to_string(g, a, buf, sizeof buf) == PG_INVALID);
    r = (pg_value *)1;
    CHECK(pg_value_new(g, 1, 0, &r) == PG_DIVZERO && r == NULL);
    r = (pg_value *)1;
    CHECK(pg_value_from_string(g, "x", &r) == PG_INVALID && r == NULL);
    pg_value *z = V(g, 0, 1);
    r = (pg_value *)1;
    CHECK(pg_div(g, b, z, &r) == PG_DIVZERO && r == NULL);
    pg_array *x = (pg_array *)1;
    int64_t nn[] = {1}, dd[] = {0};
    CHECK(pg_array_from_fracs(g, nn, dd, 1, &x) == PG_DIVZERO && x == NULL);
    AUDIT(g);
    pg_graph_destroy(g);
}

int main(void) {
    test_entry_examples();
    test_reduction();
    test_arith();
    test_large();
    test_text();
    test_arrays();
    test_array_overflow();
    test_expr();
    test_churn();
    test_den_2_63();
    test_wide_cancel();
    test_sum_retry_and_order();
    test_released();
    printf("unit: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
