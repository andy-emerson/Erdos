/* unit.c: unit tests of Design 2 (spec v0.1 §13.3 item 1), run in the test build at widths 64 and 32.  Every worked
 * example of the design note for Design 2, with its intermediate structures; the per-row tests of t_common.h (zero,
 * negatives, division by zero, equal-index fast paths, denominator 1, overflow in each operation); composite pieces
 * (multiplication and addition), the remainder test that stays at a piece, lookup-found results with no power test,
 * h = 1 with no division, the extra refinement (its counterexample, with and without), the meeting's bound, the
 * large-base audit, memory accounting, and audits that find broken invariants.
 *
 * Moved out of the joint unit.c (shared/partial-sharing-baseline/lib/tests) on Oct 29, 2026, with no check
 * changed; the control's, the ceiling's and the row table's tests stay there. */
#define _POSIX_C_SOURCE 200809L /* fork, pipe, waitpid: the meeting-bound test runs its fault in a child */

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "introspect_d2.h"
#include "sk_d2.h"
#include "t_common.h"

/* ----------------------------------------------------------------------------------------- Design 2 */

static void d2_row_is(const sk_d2 *c, uint32_t k, const sk_int *pieces, const uint32_t *nus, uint32_t n, sk_int u) {
    /* row k is exactly {pieces[t]: nus[t]} (pieces by value, in row order), with remainder u */
    CHECK_EQ(sk_d2_t_row_len(c, k), n);
    CHECK_EQ(sk_d2_t_u(c, k), u);
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

#define D2_DELTA(field, ph, part) ((long long)(k1.c[(ph)][(part)].field - k0.c[(ph)][(part)].field))

/* The note's worked example: entry computes no gcd against the base; meetings refine only what they need. */
static void test_d2_note(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle a, b, e, r;
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    sk_d2_enter(c, 1, 12, &a);
    sk_d2_enter(c, 5, 18, &b);
    sk_d2_enter(c, 1, 30, &e);
    sk_d2_counters(c, &k1);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_ENTRY, -1) - CSUM(k0, gcd, SK_PH_ENTRY, -1), 3); /* lowest terms only */
    CHECK_EQ(sk_d2_t_C(c), 0);
    for (uint32_t k = 0; k < 3; k++) d2_row_is(c, k, NULL, NULL, 0, sk_d2_t_den(c, k)); /* bare: u = d */
    /* 1/12 + 5/18: the remainders share 6, absorbed as a piece; 6 splits on u_12 = 2; base [2, 3] */
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, a, b, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 13, 36);
    CHECK_EQ(D2_DELTA(ev_meetings, SK_PH_ADD, SK_CORE), 1);
    CHECK_EQ(D2_DELTA(ev_absorbs, SK_PH_ADD, SK_CORE), 1);
    CHECK_EQ(D2_DELTA(ref_splits, SK_PH_ADD, SK_CORE), 1);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_ADD, SK_STORE) - CSUM(k0, gcd, SK_PH_ADD, SK_STORE), 0); /* store refines nothing */
    CHECK_EQ(D2_DELTA(ev_rows_u1, SK_PH_ADD, SK_STORE), 1);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 1}; d2_row_is(c, 0, P, V, 2, 1); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 2}; d2_row_is(c, 1, P, V, 2, 1); }
    d2_row_is(c, 2, NULL, NULL, 0, 30); /* 30 has met nothing: untouched */
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 2}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2, 1); }
    /* 1/18 + 1/30: u_30 is refined against row 18's pieces only, leaving u = 5, which never becomes a piece.
     * gcds: u_30 against 2 (twice: divide, then coprime) and 3 (twice); then gcd(8, 2) at the equal piece 2 */
    sk_handle b1;
    sk_d2_enter(c, 1, 18, &b1);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, b1, e, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 4, 45);
    CHECK_EQ(D2_DELTA(gcd, SK_PH_ADD, SK_CORE), 5);
    CHECK_EQ(D2_DELTA(ref_remainder_pulls, SK_PH_ADD, SK_CORE), 2);
    CHECK_EQ(D2_DELTA(ev_extra_queued, SK_PH_ADD, SK_CORE), 2); /* row 30 gains 2 and 3 */
    CHECK_EQ(D2_DELTA(ev_absorbs, SK_PH_ADD, SK_CORE), 0);      /* u_18 = 1: no remainder gcd */
    CHECK_EQ(D2_DELTA(gcd, SK_PH_ADD, SK_STORE), 0);
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_ADD, SK_STORE), 1); /* power(2, 2) */
    CHECK_EQ(D2_DELTA(ev_rows_unrefined, SK_PH_ADD, SK_STORE), 1);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 2, P, V, 2, 5); }
    { sk_int P[] = {3}; uint32_t V[] = {2}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 1, 5); }
    /* found by the lookup: 7/18 + 1/30 = 19/45, no power test.  The pair meets again: u_30 = 5 is tested against
     * 2 and 3 once more (nothing records that the pair met), then gcd(7*5 + 3, 2) */
    sk_handle b7;
    sk_d2_enter(c, 7, 18, &b7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, b7, e, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 19, 45);
    CHECK_EQ(D2_DELTA(gcd, SK_PH_ADD, SK_CORE), 3);
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_ADD, SK_STORE), 0);
    CHECK_EQ(D2_DELTA(ev_new_dens, SK_PH_ADD, SK_STORE), 0);
    /* 1/12 - 5/18 */
    CHECK(sk_d2_sub(c, a, b, &r) == SK_OK);
    val(&sk_row_d2, c, r, -7, 36);
    /* 5/18 * 7/30 = 7/108: no meeting; pieces 2 and 3 in both rows are skipped; then gcd(5, u_30 = 5) = 5 */
    sk_handle s7;
    sk_d2_enter(c, 7, 30, &s7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, b, s7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 7, 108);
    CHECK_EQ(D2_DELTA(gcd, SK_PH_MUL, SK_CORE), 1);
    CHECK_EQ(D2_DELTA(ev_meetings, SK_PH_MUL, SK_CORE), 0);
    CHECK_EQ(D2_DELTA(ev_rows_u1, SK_PH_MUL, SK_STORE), 1);
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 3}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2, 1); }
    /* again: found by the lookup, no power test */
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, b, s7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_MUL, SK_STORE), 0);
    CHECK_EQ(D2_DELTA(ev_new_dens, SK_PH_MUL, SK_STORE), 0);
    /* 5/12 * 7/12 = 35/144: no gcd; row 12 doubled, u = 1 */
    sk_handle f5, f7;
    sk_d2_enter(c, 5, 12, &f5);
    sk_d2_enter(c, 7, 12, &f7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, f5, f7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 35, 144);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_MUL, -1) - CSUM(k0, gcd, SK_PH_MUL, -1), 0);
    { sk_int P[] = {2, 3}; uint32_t V[] = {4, 2}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 2, 1); }
    /* 5/12 / (-7/18) = -15/14: both remainders are 1, so the meeting computes nothing; g_1 = gcd(5, 7) is the only
     * gcd; R = {2:1} and u = 7, which stays in the remainder: the base is still [2, 3] */
    sk_handle m7;
    sk_d2_enter(c, -7, 18, &m7);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_div(c, f5, m7, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, -15, 14);
    CHECK_EQ(CSUM(k1, gcd, SK_PH_DIV, -1) - CSUM(k0, gcd, SK_PH_DIV, -1), 1);
    CHECK_EQ(D2_DELTA(ev_meetings, SK_PH_DIV, SK_CORE), 1);
    CHECK_EQ(D2_DELTA(ev_rows_unrefined, SK_PH_DIV, SK_STORE), 1);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 1, 7); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* The extra refinement (spec §8.2 item 3).  1/2 + 1/6 = 2/3, then 1/2 / 2/3 = 3/4 leaves row 4 = {2:1} with u = 2:
 * the remainder shares its own row's piece.  Then 3/4 + 1/8: u_8 is refined against row 4's piece 2, which row 8
 * gains; only the extra refinement then refines u_4 against 2.  Without it, u_4 = 2 survives, the walk misses that
 * 2 divides both denominators, and the sum is 14/16, not in lowest terms. */
static void d2_case_extra(int skip, sk_int *m, sk_int *d, int *audit_ok) {
    sk_d2 *c = sk_d2_create();
    const sk_row *R = &sk_row_d2;
    sk_handle h0 = ent(R, c, 1, 2), h1 = ent(R, c, 1, 6);
    sk_handle s = op_ok(R->add, c, h0, h1);
    val(R, c, s, 2, 3);
    sk_handle q = op_ok(R->div, c, h0, s);
    val(R, c, q, 3, 4);
    { sk_int P[] = {2}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, q), P, V, 1, 2); }
    sk_handle e8 = ent(R, c, 1, 8);
    sk_counters k0 = cnt_of(R, c);
    sk_d2_t_skip_extra = skip;
    sk_handle r = op_ok(R->add, c, q, e8);
    sk_d2_t_skip_extra = 0;
    sk_counters k1 = cnt_of(R, c);
    if (!skip) {
        CHECK_EQ(CSUM(k1, gcd, SK_PH_ADD, SK_CORE) - CSUM(k0, gcd, SK_PH_ADD, SK_CORE), 2); /* u_8 vs 2, u_4 vs 2 */
        CHECK_EQ(D2_DELTA(ev_extra_queued, SK_PH_ADD, SK_CORE), 1);
        { sk_int P[] = {2}; uint32_t V[] = {2}; d2_row_is(c, sk_d2_t_den_index(c, q), P, V, 1, 1); }
        { sk_int P[] = {2}; uint32_t V[] = {3}; d2_row_is(c, sk_d2_t_den_index(c, e8), P, V, 1, 1); }
    }
    R->read(c, r, m, d);
    const char *why;
    *audit_ok = sk_d2_audit(c, &why) == SK_OK;
    sk_d2_destroy(c);
}

/* The meeting's iteration bound (DEC-49).  With the injected fault, reduce finds that piece 2 divides u = 2 but
 * does not divide it out, so the meeting would loop forever; the bound makes it abort with its message instead.
 * The fault runs in a child process; the parent checks that the child aborted and said why.  Without the fault the
 * same meeting passes well inside the bound. */
static void test_d2_meet_bound(void) {
    int fd[2];
    CHECK(pipe(fd) == 0);
    fflush(NULL);
    pid_t pid = fork();
    CHECK(pid >= 0);
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], 2);
        sk_d2 *c = sk_d2_create();
        sk_handle a, b, r;
        sk_d2_enter(c, 1, 2, &a);
        sk_d2_enter(c, 1, 6, &b);
        sk_d2_t_fault_stuck = 1;
        sk_d2_add(c, a, b, &r); /* absorbs piece 2, then reduce sticks at it */
        _exit(0);               /* reached only if the bound did not trigger */
    }
    close(fd[1]);
    char buf[512];
    size_t n = 0;
    ssize_t got;
    while (n + 1 < sizeof buf && (got = read(fd[0], buf + n, sizeof buf - 1 - n)) > 0) n += (size_t)got;
    buf[n] = 0;
    close(fd[0]);
    int status = 0;
    CHECK(waitpid(pid, &status, 0) == pid);
    CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    CHECK(strstr(buf, "meeting passed its iteration bound") != NULL);
    /* the same meeting without the fault */
    sk_d2 *c = sk_d2_create();
    sk_handle a = ent(&sk_row_d2, c, 1, 2), b = ent(&sk_row_d2, c, 1, 6);
    sk_handle r = op_ok(sk_row_d2.add, c, a, b);
    val(&sk_row_d2, c, r, 2, 3);
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

static void test_d2_extra(void) {
    sk_int m, d;
    int ok;
    d2_case_extra(0, &m, &d, &ok);
    CHECK(m == 7 && d == 8);
    CHECK(ok);
    d2_case_extra(1, &m, &d, &ok);
    CHECK(m == 14 && d == 16); /* the counterexample: without the extra refinement, a result not in lowest terms */
    CHECK(!ok);                /* which the audit finds */
}

/* Base [6], from the meeting of 6 and 36.  2/1 * 1/6 = 1/3: h = gcd(2, 6) = 2 is not a power of 6; R is empty and
 * the leftover 3 stays in the remainder: no split.  Then 1/3 + 1/6 = 1/2: the leftover meets row 6 and splits 6. */
static void test_d2_composite_mul(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle s6, s36, two, r;
    sk_d2_enter(c, 1, 6, &s6);
    sk_d2_enter(c, 1, 36, &s36);
    sk_d2_add(c, s6, s36, &r);
    { sk_int B[] = {6}; d2_base_is(c, B, 1); }
    { sk_int P[] = {6}; uint32_t V[] = {1}; d2_row_is(c, 0, P, V, 1, 1); }
    { sk_int P[] = {6}; uint32_t V[] = {2}; d2_row_is(c, 1, P, V, 1, 1); }
    sk_d2_enter(c, 2, 1, &two);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_mul(c, two, s6, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 1, 3);
    const sk_count *a = &k0.c[SK_PH_MUL][SK_STORE], *b = &k1.c[SK_PH_MUL][SK_STORE];
    CHECK_EQ(b->ev_rows_unrefined - a->ev_rows_unrefined, 1);
    CHECK_EQ(b->ref_splits - a->ref_splits, 0);
    CHECK_EQ(b->div_rem - a->div_rem, 1);     /* power(2, 6): one failing attempt */
    CHECK_EQ(b->div_exact - a->div_exact, 1); /* p/h in finish */
    CHECK_EQ(b->gcd - a->gcd, 0);
    CHECK_EQ(b->mul - a->mul, 1); /* u = 1 * (p/h) */
    { sk_int B[] = {6}; d2_base_is(c, B, 1); }
    d2_row_is(c, sk_d2_t_den_index(c, r), NULL, NULL, 0, 3);
    d2_row_is(c, sk_d2_t_den_index(c, two), NULL, NULL, 0, 1); /* the row of d = 1 is empty, u = 1 */
    sk_handle third = r;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, third, s6, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 1, 2);
    a = &k0.c[SK_PH_ADD][SK_CORE], b = &k1.c[SK_PH_ADD][SK_CORE];
    CHECK_EQ(b->ref_splits - a->ref_splits, 1);
    CHECK_EQ(b->ref_pieces_created - a->ref_pieces_created, 1);
    CHECK_EQ(b->ref_entries_rewritten - a->ref_entries_rewritten, 4); /* rows 6 and 36, two entries each */
    CHECK_EQ(b->scan_split_rows - a->scan_split_rows, 4);             /* rows 6, 36, 1 and 3; R is empty then */
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 0, P, V, 2, 1); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 2}; d2_row_is(c, 1, P, V, 2, 1); }
    { sk_int P[] = {3}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, third), P, V, 1, 1); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* A leftover at a piece of equal valuation in addition: base [6]; 1/6 + 11/30 = 8/15, h = 2.  The meeting puts 30
 * over the base as {6:1}, u = 5; the leftover 3 joins the remainder, and 6 is not split. */
static void test_d2_composite_add(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y, z, r;
    sk_d2_enter(c, 1, 6, &x);
    sk_d2_enter(c, 1, 36, &z);
    sk_d2_add(c, x, z, &r);
    sk_d2_enter(c, 11, 30, &y);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, x, y, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 8, 15);
    CHECK_EQ(CSUM(k1, ref_splits, -1, -1) - CSUM(k0, ref_splits, -1, -1), 0);
    CHECK_EQ(D2_DELTA(ev_rows_unrefined, SK_PH_ADD, SK_STORE), 1);
    { sk_int B[] = {6}; d2_base_is(c, B, 1); }
    { sk_int P[] = {6}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, y), P, V, 1, 5); }
    d2_row_is(c, sk_d2_t_den_index(c, r), NULL, NULL, 0, 15);
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* h = 1 costs no division: 1/6 + 1/15 = 7/30.  The meeting absorbs gcd(6, 15) = 3 as a piece; the equal piece 3
 * has h = gcd(7, 3) = 1.  gcds: gcd(u_6, u_15), u_6 against 3 (twice), u_15 against 3 (twice), gcd(2, 5), h. */
static void test_d2_h1(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y, r;
    sk_d2_enter(c, 1, 6, &x);
    sk_d2_enter(c, 1, 15, &y);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, x, y, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 7, 30);
    CHECK_EQ(D2_DELTA(gcd, SK_PH_ADD, SK_CORE), 7);
    CHECK_EQ(D2_DELTA(div_exact, SK_PH_ADD, SK_CORE), 0); /* h = 1: no division */
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_ADD, SK_STORE), 0);  /* power(1, 3) */
    CHECK_EQ(D2_DELTA(ev_absorbs, SK_PH_ADD, SK_CORE), 1);
    CHECK_EQ(D2_DELTA(ev_rows_unrefined, SK_PH_ADD, SK_STORE), 1);
    { sk_int B[] = {3}; d2_base_is(c, B, 1); }
    { sk_int P[] = {3}; uint32_t V[] = {1}; d2_row_is(c, 0, P, V, 1, 2); }
    { sk_int P[] = {3}; uint32_t V[] = {1}; d2_row_is(c, 1, P, V, 1, 5); }
    { sk_int P[] = {3}; uint32_t V[] = {1}; d2_row_is(c, sk_d2_t_den_index(c, r), P, V, 1, 10); }
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* A remainder test that stays at a piece: base [6]; in 1/12 + 1/6, u_12 = 12 is divided by 6, stays at 6, splits
 * it, then is divided by 2. */
static void test_d2_scan_stays(void) {
    sk_d2 *c = sk_d2_create();
    sk_handle x, y, z, r;
    sk_d2_enter(c, 1, 6, &x);
    sk_d2_enter(c, 1, 36, &z);
    sk_d2_add(c, x, z, &r);
    sk_d2_enter(c, 1, 12, &y);
    sk_counters k0, k1;
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, y, x, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 1, 4);
    CHECK_EQ(D2_DELTA(scan_remainder_pieces, SK_PH_ADD, SK_CORE), 3); /* 6, 6 again (split), then 2 */
    CHECK_EQ(D2_DELTA(ref_splits, SK_PH_ADD, SK_CORE), 1);
    /* divisions with remainder: 12 by 6 (1 + failing 1), nu_2(6) (2), nu_3(6) (2), 2 by 2 (1 + failing 1) */
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_ADD, SK_CORE), 8);
    { sk_int B[] = {2, 3}; d2_base_is(c, B, 2); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {2, 1}; d2_row_is(c, sk_d2_t_den_index(c, y), P, V, 2, 1); }
    { sk_int P[] = {2, 3}; uint32_t V[] = {1, 1}; d2_row_is(c, 0, P, V, 2, 1); }
    /* repeated division counts every attempt, the failing one included: u_8 = 8 against 2 in 1/8 + 1/12 */
    sk_handle w;
    sk_d2_enter(c, 1, 8, &w);
    sk_d2_counters(c, &k0);
    CHECK(sk_d2_add(c, w, y, &r) == SK_OK);
    sk_d2_counters(c, &k1);
    val(&sk_row_d2, c, r, 5, 24);
    CHECK_EQ(D2_DELTA(div_rem, SK_PH_ADD, SK_CORE), 4);
    CHECK_EQ(D2_DELTA(scan_remainder_pieces, SK_PH_ADD, SK_CORE), 1);
    audits_ok(&sk_row_d2, c);
    sk_d2_destroy(c);
}

/* More than 1,024 pieces: the scheduled full audit checks coprimality incrementally only (spec §13.3 item 2).
 * Each 1/p + 1/(2p), p an odd prime, absorbs p as a new piece. */
static void test_d2_large_base(void) {
    sk_d2 *c = sk_d2_create();
    int n = 0;
    const char *why = NULL;
    for (sk_int p = 3; n < 1100; p++) {
        bool prime = true;
        for (sk_int q = 2; q * q <= p; q++)
            if (p % q == 0) {
                prime = false;
                break;
            }
        if (!prime) continue;
        sk_handle h1, h2, r;
        sk_d2_enter(c, 1, p, &h1);
        sk_d2_enter(c, 1, 2 * p, &h2);
        CHECK(sk_d2_add(c, h1, h2, &r) == SK_OK);
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
    sk_d2 *c2 = sk_d2_create();
    for (int k = 0; k < 9; k++) {
        sk_handle h;
        sk_d2_enter(c2, 1, 2 + k, &h);
    }
    sk_memory m;
    sk_d2_memory(c2, &m);
    CHECK_EQ(m.item[2].used, 4 * 32);
    audits_ok(&sk_row_d2, c2);
    sk_d2_destroy(c2);
}

static void test_memory(void) {
    const uint64_t w = SK_WIDTH / 8;
    sk_memory m;
    sk_handle a, b, h, r;
    uint64_t N = 4, D = 4, S = 16, sum;
    /* Design 2, after the note's first addition: rows 12 {2,3}, 18 {2,3}, 30 {} (it met nothing), 36 {2,3}:
     * 6 entries; base [2, 3]; one remainder per denominator */
    sk_d2 *c2 = sk_d2_create();
    sk_d2_enter(c2, 1, 12, &a);
    sk_d2_enter(c2, 5, 18, &b);
    sk_d2_enter(c2, 1, 30, &h);
    sk_d2_add(c2, a, b, &r);
    sk_d2_memory(c2, &m);
    CHECK_EQ(find_item(&m, "numerators"), N * (w + 4));
    CHECK_EQ(find_item(&m, "denominators"), D * w);
    CHECK_EQ(find_item(&m, "lookup"), 4 * S);
    CHECK_EQ(find_item(&m, "base"), 2 * w);
    CHECK_EQ(find_item(&m, "exponent rows"), 8 * 6 + 16 * D);
    CHECK_EQ(find_item(&m, "remainders"), D * w);
    sum = 0;
    for (unsigned t = 0; t < m.n; t++) sum += m.item[t].used;
    CHECK_EQ(sum, m.used);
    sk_d2_destroy(c2);
}

/* ----------------------------------------------------------------------------------------- audits find failures */

static void test_audits_detect(void) {
    const char *why;
    sk_handle a, b, r;
    sk_d2 *c2 = sk_d2_create();
    sk_d2_enter(c2, 1, 12, &a);
    sk_d2_enter(c2, 1, 18, &b);
    sk_d2_add(c2, a, b, &r); /* the meeting: rows 12 {2:2, 3:1} and 18 {2:1, 3:2}, remainders 1 */
    CHECK(sk_d2_audit(c2, &why) == SK_OK);
    sk_d2_t_corrupt_nu(c2, 0, 0, 3); /* 2^3 * 3 != 12 */
    CHECK(sk_d2_audit_changed(c2, &why) == SK_EAUDIT);
    CHECK(sk_d2_audit(c2, &why) == SK_EAUDIT);
    sk_d2_t_corrupt_nu(c2, 0, 0, 2);
    CHECK(sk_d2_audit(c2, &why) == SK_OK);
    sk_d2_t_corrupt_u(c2, 0, 3); /* known part 12 times u = 3 != 12 */
    CHECK(sk_d2_audit_changed(c2, &why) == SK_EAUDIT);
    CHECK(sk_d2_audit(c2, &why) == SK_EAUDIT);
    sk_d2_t_corrupt_u(c2, 0, 1);
    CHECK(sk_d2_audit(c2, &why) == SK_OK);
    sk_d2_t_corrupt_piece(c2, 1, 4); /* pieces 2 and 4 */
    CHECK(sk_d2_audit_changed(c2, &why) == SK_EAUDIT);
    sk_d2_destroy(c2);
}

int main(void) {
    test_build();
    test_entry_and_errors(&sk_row_d2);
    test_zero_and_signs(&sk_row_d2);
    test_fast_paths(&sk_row_d2);
    test_note_values(&sk_row_d2);
    test_overflow(&sk_row_d2);
    test_d2_note();
    test_d2_extra();
    test_d2_meet_bound();
    test_d2_composite_mul();
    test_d2_composite_add();
    test_d2_h1();
    test_d2_scan_stays();
    test_d2_large_base();
    test_lookup_growth();
    test_memory();
    test_audits_detect();
    printf("unit, Design 2 (width %d): %d checks, %d failures\n", SK_WIDTH, checks, failures);
    return failures ? 1 : 0;
}
