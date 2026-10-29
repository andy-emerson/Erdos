/* unit.c: unit tests of Design 1 (spec v0.1 §13.3 item 1), run in the test build at widths 64 and 32.  Every worked
 * example of the design note for Design 1, with its intermediate structures; the per-row tests of t_common.h (zero,
 * negatives, division by zero, equal-index fast paths, denominator 1, overflow in each operation); islands, the §7.4
 * example, a divisor inserted mid-list, more than 64 divisors, the pair set, the counting rules, memory accounting,
 * and audits that find broken invariants.
 *
 * Moved out of the joint unit.c (shared/partial-sharing-baseline/lib/tests) on Oct 29, 2026, with no check
 * changed; the control's, the ceiling's and the row table's tests stay there. */
#include "introspect_d1.h"
#include "sk_d1.h"
#include "t_common.h"

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
    /* a later 7/12 + 1/18: the pair is in the set; no gcd of denominators; 36 found at index 3, its row unchanged */
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
    /* 5/18 * 7/30: c = 6, residuals 3 and 5; g_1 = gcd(5, 5), g_2 = gcd(7, 3); no compare, no pair inserted */
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
    /* 5/12 / (-7/18) = -15/14: the pair (0, 1) is in the set, so the gcd is read; 14 is an island */
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

/* The note's incidence example: divisors [1, 4, 6, 15] by value; 30's row holds 1, 6 and 15, residuals [30, 5, 2].
 * 4 is inserted below 6, but ids are stable (spec §7.1, amended Oct 29, 2026): 6 keeps id 1, 4 takes id 2, 15 id 3,
 * and no row shifts. */
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
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_words_shifted - k0.c[SK_PH_ADD][SK_CORE].ref_words_shifted, 0);
    CHECK(sk_d1_t_D(c) > D0);
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, h30, h45, &r) == SK_OK); /* gcd 15: inserted at the end, no shift */
    sk_d1_counters(c, &k1);
    CHECK_EQ(k1.c[SK_PH_ADD][SK_CORE].ref_words_shifted - k0.c[SK_PH_ADD][SK_CORE].ref_words_shifted, 0);
    CHECK_EQ(sk_d1_t_C(c), 4);
    CHECK_EQ(sk_d1_t_divisor(c, 1), 4);
    CHECK_EQ(sk_d1_t_divisor(c, 2), 6);
    CHECK_EQ(sk_d1_t_divisor(c, 3), 15);
    CHECK_EQ(sk_d1_t_divisor_id(c, 0), 0);
    CHECK_EQ(sk_d1_t_divisor_id(c, 1), 2); /* 4 */
    CHECK_EQ(sk_d1_t_divisor_id(c, 2), 1); /* 6, its id unchanged by the insertion of 4 */
    CHECK_EQ(sk_d1_t_divisor_id(c, 3), 3); /* 15 */
    uint32_t k30 = sk_d1_t_den_index(c, h30);
    CHECK(sk_d1_t_bit(c, k30, 0) && !sk_d1_t_bit(c, k30, 1) && sk_d1_t_bit(c, k30, 2) && sk_d1_t_bit(c, k30, 3));
    CHECK_EQ(sk_d1_t_res_len(c, k30), 3);
    CHECK_EQ(sk_d1_t_res(c, k30, 0), 30);
    CHECK_EQ(sk_d1_t_res(c, k30, 1), 5);
    CHECK_EQ(sk_d1_t_res(c, k30, 2), 2); /* the residual at 15 is the row's third entry */
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

/* More than 64 divisors, inserted in decreasing order: each takes the next id and the first place by value, and no
 * row shifts or is renumbered (formerly: the stride grew and every insertion shifted every row). */
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
    sk_counters k;
    sk_d1_counters(c, &k);
    CHECK_EQ(CSUM(k, ref_stride_growths, -1, -1), 0);
    CHECK_EQ(CSUM(k, ref_words_shifted, -1, -1), 0);
    CHECK_EQ(CSUM(k, ref_div_inserted, -1, -1), NPRIMES);
    for (uint32_t col = 1; col < sk_d1_t_C(c); col++) {
        CHECK_EQ(sk_d1_t_divisor(c, col), PRIMES[col - 1]);
        CHECK_EQ(sk_d1_t_divisor_id(c, col), (uint32_t)NPRIMES + 1 - col); /* ids in order of insertion */
    }
    /* every pair compared is still exact after the insertions */
    sk_handle x, y, r;
    sk_d1_enter(c, 1, 2 * PRIMES[1], &x);
    sk_d1_enter(c, 2, 3 * PRIMES[1], &y);
    CHECK(sk_d1_add(c, x, y, &r) == SK_OK); /* the pair is in the set: the search gives the gcd */
    val(&sk_row_d1, c, r, 7, 6 * PRIMES[1]);
    audits_ok(&sk_row_d1, c);
    sk_d1_destroy(c);
}


/* The pair set (spec §7.1, amended Oct 29, 2026): only compared pairs are stored.  It starts with no table, grows
 * at half full, finds every pair at either order, and counts a membership test as one pair read plus a lookup. */
static void test_d1_pair_set(void) {
    sk_d1 *c = sk_d1_create();
    sk_memory m;
    sk_handle h[40], r;
    for (int k = 0; k < 40; k++) sk_d1_enter(c, 1, 6 * (k + 1), &h[k]);
    sk_d1_memory(c, &m);
    CHECK_EQ(find_item(&m, "pair set"), 0); /* 40 islands: no pair met, no bytes */
    CHECK_EQ(sk_d1_t_pairs(c), 0);
    /* 9 compared pairs (0, k): the 9th insertion would pass half of 16 slots, so the set doubles to 32 */
    for (int k = 1; k <= 9; k++) CHECK(sk_d1_add(c, h[0], h[k], &r) == SK_OK);
    CHECK_EQ(sk_d1_t_pairs(c), 9);
    sk_d1_memory(c, &m);
    CHECK_EQ(find_item(&m, "pair set"), 8 * 32);
    for (int k = 1; k <= 9; k++) CHECK(sk_d1_t_pair(c, 0, (uint32_t)k) && sk_d1_t_pair(c, (uint32_t)k, 0)); /* either order */
    CHECK(!sk_d1_t_pair(c, 1, 2));
    /* multiplication searches the rows but never compares, so it inserts nothing */
    CHECK(sk_d1_mul(c, h[1], h[2], &r) == SK_OK);
    CHECK(!sk_d1_t_pair(c, 1, 2));
    CHECK_EQ(sk_d1_t_pairs(c), 9);
    /* a compared pair is read, not compared again: one pair read, one search, at least one probe, no gcd of the
     * denominators (the only gcd is h) */
    sk_counters k0, k1;
    sk_d1_counters(c, &k0);
    CHECK(sk_d1_add(c, h[0], h[1], &r) == SK_OK);
    sk_d1_counters(c, &k1);
    const sk_count *a = &k0.c[SK_PH_ADD][SK_CORE], *b = &k1.c[SK_PH_ADD][SK_CORE];
    CHECK_EQ(b->pair_read - a->pair_read, 1);
    CHECK_EQ(b->ev_pair_hits - a->ev_pair_hits, 1);
    CHECK_EQ(b->lookup_search - a->lookup_search, 1);
    CHECK(b->lookup_probe - a->lookup_probe >= 1);
    CHECK_EQ(b->gcd - a->gcd, 1);
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
    CHECK_EQ(q->scan_and_words, 1);      /* one merge step: the rows' top entries are both divisor 1 */
    CHECK_EQ(q->scan_popcount_words, 0); /* 6 goes above divisor 1, the top entry of each row: no entry read */
    CHECK_EQ(q->bitops, 0);
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

/* ----------------------------------------------------------------------------------------- lookup, memory */

static void test_lookup_growth(void) {
    sk_d1 *c1 = sk_d1_create();
    for (int k = 0; k < 9; k++) {
        sk_handle h;
        sk_d1_enter(c1, 1, 2 + k, &h);
        uint32_t want = k < 8 ? 16 : 32; /* at most half full */
        CHECK_EQ(sk_d1_t_S(c1), want);
    }
    for (int k = 0; k < 9; k++) { /* each found again, at its index */
        sk_handle h;
        sk_d1_enter(c1, 11, 2 + k, &h);
        CHECK_EQ(sk_d1_t_den_index(c1, h), k);
    }
    CHECK_EQ(sk_d1_t_D(c1), 9);
    audits_ok(&sk_row_d1, c1);
    sk_d1_destroy(c1);
}

static void test_memory(void) {
    const uint64_t w = SK_WIDTH / 8;
    sk_memory m;
    sk_handle h;
    /* Design 1, after the note's first addition */
    sk_d1 *c1 = sk_d1_create();
    sk_handle a, b, r;
    sk_d1_enter(c1, 1, 12, &a);
    sk_d1_enter(c1, 5, 18, &b);
    sk_d1_enter(c1, 1, 30, &h);
    sk_d1_add(c1, a, b, &r);
    sk_d1_memory(c1, &m);
    uint64_t N = 4, D = 4, C = 2, S = 16, entries = 2 + 2 + 1 + 1;
    CHECK_EQ(find_item(&m, "numerators"), N * (w + 4));
    CHECK_EQ(find_item(&m, "denominators"), D * w);
    CHECK_EQ(find_item(&m, "lookup"), 4 * S);
    CHECK_EQ(find_item(&m, "divisors"), C * (w + 4)); /* values by id, and ids by value */
    CHECK_EQ(find_item(&m, "pair set"), 8 * 16); /* one compared pair: the initial 16 slots of 8 bytes */
    CHECK_EQ(find_item(&m, "incidence rows"), (w + 4) * entries + 16 * D);
    CHECK_EQ(m.values, N);
    CHECK(m.used_per_rational == (double)m.used / (double)N);
    uint64_t sum = 0;
    for (unsigned t = 0; t < m.n; t++) {
        sum += m.item[t].used;
        CHECK(m.item[t].allocated >= m.item[t].used);
    }
    CHECK_EQ(sum, m.used);
    sk_d1_destroy(c1);
}

/* ----------------------------------------------------------------------------------------- audits find failures */

static void test_audits_detect(void) {
    const char *why;
    sk_handle a, b, r;
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
}

int main(void) {
    test_build();
    test_entry_and_errors(&sk_row_d1);
    test_zero_and_signs(&sk_row_d1);
    test_fast_paths(&sk_row_d1);
    test_note_values(&sk_row_d1);
    test_overflow(&sk_row_d1);
    test_d1_note();
    test_d1_islands();
    test_d1_incidence();
    test_d1_stride();
    test_d1_pair_set();
    test_d1_counting();
    test_lookup_growth();
    test_memory();
    test_audits_detect();
    printf("unit, Design 1 (width %d): %d checks, %d failures\n", SK_WIDTH, checks, failures);
    return failures ? 1 : 0;
}
