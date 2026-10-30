/* m3_unit.c: the unit tests of round m3 (spec 11-matrix v0.3 §16.9 item 6; test build), every row (TC, Hash-V-pack,
 * Pack-primes, Pack-coprime) unless a test is a factored row's own:
 *   - trial division (Pack-primes): small primes and prime squares, 2^62, the largest prime below 2^63, a semiprime of
 *     two primes near 2^31 (the d d bound), 1, and the refusal of 0 (its list empty); the factors, and the candidates,
 *     divisions with remainder and exact divisions against the closed form of §16.4 item 3 (computed here from the
 *     known factorization, independently of mh3.c).  The two large cases run the same schoolbook trial division, with
 *     no shortcut: about 1.5e9 and 1.1e9 candidates;
 *   - refinement with splits (Pack-coprime): {12, 18}, {15, 45}, {15, 225} (a composite piece kept), {36, 6, 4};
 *   - reconciliation that splits an operand's numerator piece against the other's denominator piece, in each direction,
 *     and a reconciliation that needs a second pass;
 *   - a leftover split inside a multiplication's sum (a piece 6, t even and not divisible by 3), and a leftover in a
 *     stored sum that splits nothing;
 *   - every new sub-case (9a-c, 10a-c, 12a-c), matched and not;
 *   - the cost order, exhaustively (as 10-interning/v0.3 §17.5): every addition and multiplication of two values
 *     +-a/b, 0 <= a <= 12, 1 <= b <= 12, in all four rows: the case is TC's, the value equal, the core arithmetic
 *     (exact divisions, multiplications, additions) equal, and every obtained gcd equal to Euclid's (checked inside
 *     the factored rows in this build);
 *   - every v0.2 §15.7 test that does not name Hash-V-fixed or Hash-V-grow-map (m2_unit.c's): every case of addition
 *     and multiplication, on cells and on temporaries; zero, integer and +-1 by value; each widening of a result; the
 *     packing at every width (the factored rows' (h), (l) and handles equal to Hash-V-pack's); both expressions with
 *     every intermediate; intermediates of every width as operands; w_h; overflow; probing; the audits, (f)'s broken
 *     invariants among them. */
#include "m3t.h"

#define P2(m, d) (sk_int)(m), (sk_int)(d)

/* ----------------------------------------------------------------------------------------- the cases (m2's) */

typedef struct tcase {
    const char *name;
    int kase;
    sk_int xm, xd, ym, yd;
} tcase;

static const tcase ADD_CASES[] = {
    {"zero", R2_ADD_ZERO, 3, 5, -3, 5},       {"zero (int)", R2_ADD_ZERO, -4, 1, 4, 1},
    {"zero_x", R2_ADD_ZERO_X, 0, 1, -3, 5},   {"zero_y", R2_ADD_ZERO_Y, 3, 5, 0, 1},
    {"int_both", R2_ADD_INT_BOTH, 3, 1, -4, 1}, {"int_x", R2_ADD_INT_X, 3, 1, 2, 5},
    {"int_y", R2_ADD_INT_Y, -2, 5, 3, 1},     {"cross a=d", R2_ADD_CROSS, 5, 7, 2, 5},
    {"cross c=b", R2_ADD_CROSS, 2, 7, -7, 9}, {"den", R2_ADD_DEN, 2, 7, 3, 7},
    {"den (h>1)", R2_ADD_DEN, 1, 6, 1, 6},    {"num", R2_ADD_NUM, 3, 4, 3, 5},
    {"num (g>1)", R2_ADD_NUM, 3, 4, -3, 10},  {"num (g>1, h>1)", R2_ADD_NUM, 1, 4, 1, 12},
    {"general", R2_ADD_GEN, 2, 9, 4, 15},     {"general (g=1)", R2_ADD_GEN, 2, 9, 4, 25},
    {"general (h>1)", R2_ADD_GEN, 1, 6, 7, 10},
};
static const tcase MUL_CASES[] = {
    {"zero", R2_MUL_ZERO, 0, 1, 3, 5},          {"zero (y)", R2_MUL_ZERO, 3, 5, 0, 1},
    {"one_x", R2_MUL_ONE_X, -1, 1, 3, 5},       {"one_y", R2_MUL_ONE_Y, 3, 5, 1, 1},
    {"int_both", R2_MUL_INT_BOTH, 3, 1, -4, 1}, {"both", R2_MUL_BOTH, 3, 5, -5, 3},
    {"den", R2_MUL_DEN, 2, 7, 3, 7},            {"num", R2_MUL_NUM, 3, 4, 3, 5},
    {"cross_ad", R2_MUL_CROSS_AD, 3, 4, 5, 3},  {"cross_ad (h>1)", R2_MUL_CROSS_AD, 3, 4, 2, 3},
    {"cross_cb", R2_MUL_CROSS_CB, 4, 5, 5, 7},  {"cross_cb (h>1)", R2_MUL_CROSS_CB, 4, 5, 5, 8},
    {"int_x", R2_MUL_INT_X, 3, 1, 2, 5},        {"int_x (h>1)", R2_MUL_INT_X, 10, 1, 3, 5},
    {"int_y", R2_MUL_INT_Y, 2, 5, 3, 1},        {"int_y (h>1)", R2_MUL_INT_Y, 3, 5, 10, 1},
    {"general", R2_MUL_GEN, 2, 9, 4, 15},       {"general (g=1)", R2_MUL_GEN, 2, 9, 4, 25},
};

/* one operation on 1x1 matrices (multiplication: the product, then add(0/1, p), case zero_x) */
static void one_case(const tcase *t, int op) {
    sk_int A[2] = {t->xm, t->xd}, B[2] = {t->ym, t->yd};
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(op, 1, in, want);
    for (int r = 0; r < NROWS; r++) {
        const row *R = &ROWS[r];
        job j = job_run(R, op, 1, in, 0);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        CHECK_EQ(job_case(&j, op ? SK_PH_MUL : SK_PH_ADD, t->kase), 1);
        if (op) CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_ZERO_X), 1); /* the first addition of a cell (v0.1 §6.2) */
        CHECK(job_audits(&j));
        if (R->hash) CHECK_EQ(mat_opc(R, j.M[M2_R]).cross, 4);
        job_free(&j);
    }
    ref_free(want);
}

/* every addition case on two local pairs: R = A B with B all 1, so that each cell's sum is u + v (v0.1 §6.2 item 3) */
static void temp_case(const tcase *t) {
    sk_int A[8] = {t->xm, t->xd, t->ym, t->yd, t->xm, t->xd, t->ym, t->yd}, B[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(SET_MUL, 2, in, want);
    for (int r = 0; r < NROWS; r++) {
        job j = job_run(&ROWS[r], SET_MUL, 2, in, 0);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        uint64_t extra = t->kase == R2_ADD_ZERO_X ? 4 : 0; /* each of the 4 cells: add(0/1, x) then add(x, y) */
        CHECK_EQ(job_case(&j, SK_PH_ADD, t->kase), 4 + extra);
        CHECK(job_audits(&j));
        job_free(&j);
    }
    ref_free(want);
}

static void test_cases(void) {
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) one_case(&ADD_CASES[i], SET_ADD);
    for (size_t i = 0; i < sizeof MUL_CASES / sizeof *MUL_CASES; i++) one_case(&MUL_CASES[i], SET_MUL);
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) temp_case(&ADD_CASES[i]);
    printf("  cases: %zu addition, %zu multiplication, every addition case on temporaries; all four rows\n",
           sizeof ADD_CASES / sizeof *ADD_CASES, sizeof MUL_CASES / sizeof *MUL_CASES);
}

/* ----------------------------------------------------------------------------------------- the new sub-cases */

static void test_subcases(void) {
    struct {
        int op, sub;
        sk_int xm, xd, ym, yd;
    } S[] = {
        {SET_ADD, M3_NUM_COP, 3, 4, 3, 5},   /* 9a: gcd(4, 5) = 1 */
        {SET_ADD, M3_NUM_UNEQ, 1, 4, 1, 2},  /* 9b: 2^2 against 2^1 */
        {SET_ADD, M3_NUM_EQ, 1, 4, 1, 12},   /* 9c: 2^2 against 2^2 3 */
        {SET_ADD, M3_NUM_EQ, 1, 6, 1, 10},   /* 9c, h = 1 after the test: (5 + 3) = 8, h = gcd(8, 2) = 2 */
        {SET_ADD, M3_GEN_COP, 2, 9, 4, 25},  /* 10a */
        {SET_ADD, M3_GEN_UNEQ, 2, 9, 4, 15}, /* 10b: 3^2 against 3 */
        {SET_ADD, M3_GEN_EQ, 1, 6, 7, 10},   /* 10c: 2 against 2 */
        {SET_ADD, M3_GEN_EQ, 1, 6, 4, 15},   /* 10c: 3 against 3, t = 5 + 8 = 13: h = 1 after the test */
        {SET_MUL, M3_MGEN_COP, 2, 9, 4, 25}, /* 12a */
        {SET_MUL, M3_MGEN_ONE, 2, 9, 3, 5},  /* 12b: gcd(2, 5) = 1, gcd(3, 9) = 3 */
        {SET_MUL, M3_MGEN_ONE, 4, 9, 7, 2},  /* 12b: gcd(4, 2) = 2, gcd(7, 9) = 1 */
        {SET_MUL, M3_MGEN_TWO, 2, 9, 3, 4},  /* 12c */
    };
    for (size_t i = 0; i < sizeof S / sizeof *S; i++) {
        sk_int A[2] = {S[i].xm, S[i].xd}, B[2] = {S[i].ym, S[i].yd};
        const sk_int *in[2] = {A, B};
        ref_q *want[M2_NMAT];
        job_ref(S[i].op, 1, in, want);
        for (int r = R_PRIMES; r <= R_COPRIME; r++) {
            job j = job_run(&ROWS[r], S[i].op, 1, in, 0);
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(job_check(&j, want), 0);
            m3_acount a = mat_ac(&ROWS[r], j.M[M2_R]);
            for (int k = 0; k < M3_NSUB; k++) CHECK_EQ(a.sub[k], k == S[i].sub); /* matched, and every other not */
            CHECK(a.gchecked > 0);
            if (S[i].sub == M3_NUM_UNEQ || S[i].sub == M3_GEN_UNEQ) CHECK_EQ(a.tests + a.pgcd, 0); /* h = 1 known */
            if (S[i].sub == M3_NUM_EQ || S[i].sub == M3_GEN_EQ) CHECK(a.tests + a.pgcd > 0);
            CHECK(job_audits(&j));
            job_free(&j);
        }
        ref_free(want);
    }
    /* the same sub-cases in add(s, p), on temporaries: R = A B with B all 1, cells x and y in A's rows */
    for (size_t i = 0; i < sizeof S / sizeof *S; i++) {
        if (S[i].op != SET_ADD) continue;
        sk_int A[8] = {S[i].xm, S[i].xd, S[i].ym, S[i].yd, S[i].xm, S[i].xd, S[i].ym, S[i].yd};
        sk_int B[8] = {1, 1, 1, 1, 1, 1, 1, 1};
        const sk_int *in[2] = {A, B};
        ref_q *want[M2_NMAT];
        job_ref(SET_MUL, 2, in, want);
        for (int r = R_PRIMES; r <= R_COPRIME; r++) {
            job j = job_run(&ROWS[r], SET_MUL, 2, in, 0);
            CHECK_EQ(job_check(&j, want), 0);
            m3_acount a = mat_ac(&ROWS[r], j.M[M2_R]);
            for (int k = 0; k < M3_NSUB; k++) CHECK_EQ(a.sub[k], k == S[i].sub ? 4u : 0u);
            job_free(&j);
        }
        ref_free(want);
    }
    printf("  sub-cases: 9a-c, 10a-c, 12a-c each matched once and the others not, on cells and on temporaries\n");
}

/* ----------------------------------------------------------------------------------------- trial division */

/* The closed form of §16.4 item 3, from the factorization (primes p[0] < ... < p[k-1], exponents e[]): the loop stops at
 * the first candidate d (2, then the odd numbers) with d^2 > m(d), m(d) the product of the prime factors >= d.  Within
 * (p[j-1], p[j]] m(d) is constant, M_j = prod_{i >= j} p[i]^e[i], so d* is found interval by interval. */
static void closed_form(const uint64_t *p, const unsigned *e, int k, uint64_t *cand, uint64_t *divs, uint64_t *exact) {
    unsigned __int128 M[17];
    M[k] = 1;
    for (int j = k - 1; j >= 0; j--) {
        M[j] = M[j + 1];
        for (unsigned z = 0; z < e[j]; z++) M[j] *= p[j];
    }
    uint64_t dstar = 0;
    int jstar = k;
    for (int j = 0; j <= k && !dstar; j++) {
        uint64_t lo = j ? p[j - 1] + 1 : 2, hi = j < k ? p[j] : UINT64_MAX;
        uint64_t r = 0; /* the smallest r with r^2 > M[j] */
        unsigned __int128 lo2 = 0, hi2 = (unsigned __int128)1 << 33;
        while (lo2 < hi2) {
            unsigned __int128 mid = (lo2 + hi2) / 2;
            if (mid * mid > M[j]) hi2 = mid;
            else lo2 = mid + 1;
        }
        r = (uint64_t)lo2;
        uint64_t d = r > lo ? r : lo;
        if (d > 2 && d % 2 == 0) d++; /* the next candidate */
        if (d <= hi) {
            dstar = d;
            jstar = j;
        }
    }
    *cand = dstar == 2 ? 0 : (dstar + 1) / 2 - 1; /* candidates before d*: 2, 3, 5, ..., d* - 2 */
    uint64_t removed = 0;                         /* prime factors removed by the loop: those below d* */
    for (int j = 0; j < jstar; j++) removed += e[j];
    /* the primes in [p[jstar], ...) are left: one prime cofactor (appended as is), or nothing */
    *divs = *cand + removed;
    *exact = removed;
}

static void td_case(const char *name, sk_int v, const uint64_t *p, const unsigned *e, int k) {
    const row *R = &ROWS[R_PRIMES];
    void *x = R->create(1, 1, M2_A);
    CHECK_EQ(R->enter(x, v, 1), SK_OK);
    uint32_t W = mat_fig(R, x).W, at = W;
    for (uint32_t s = 0; s < W; s++) {
        sk_int u = 0;
        R->t_whole(x, s, &u);
        if (u == v) at = s;
    }
    CHECK(at < W);
    uint32_t ne = 0;
    R->t_nfac(x, at, &ne);
    CHECK_EQ(ne, (uint32_t)k);
    for (int j = 0; j < k && j < (int)ne; j++) {
        sk_int q = 0;
        unsigned ex = 0;
        R->t_fac(x, at, (uint32_t)j, &q, &ex);
        CHECK_EQ(q, (sk_int)p[j]);
        CHECK_EQ(ex, e[j]);
    }
    uint64_t cand, divs, exact;
    closed_form(p, e, k, &cand, &divs, &exact);
    m3_fcount f = mat_fc(R, x);
    CHECK_EQ(f.td_cand, cand);
    CHECK_EQ(f.td_divs, divs);
    CHECK_EQ(f.td_exact, exact);
    printf("    %s: %d factors, %llu candidates\n", name, k, (unsigned long long)f.td_cand);
    const char *why;
    CHECK_EQ(R->audit(x, &why), SK_OK);
    R->destroy(x);
}

static void test_trial_division(void) {
    uint64_t P1[] = {2}, P2_[] = {3}, P3[] = {97}, P4[] = {2}, P5[] = {3}, P6[] = {97}, P7[] = {2}, P8[] = {3, 5, 7};
    unsigned E1[] = {1}, E2[] = {1}, E3[] = {1}, E4[] = {2}, E5[] = {2}, E6[] = {2}, E7[] = {62}, E8[] = {2, 1, 3};
    td_case("2", 2, P1, E1, 1);
    td_case("3", 3, P2_, E2, 1);
    td_case("97", 97, P3, E3, 1);
    td_case("4", 4, P4, E4, 1);
    td_case("9", 9, P5, E5, 1);
    td_case("97^2", 9409, P6, E6, 1);
    td_case("2^62", (sk_int)1 << 62, P7, E7, 1);
    td_case("3^2 5 7^3", 9 * 5 * 343, P8, E8, 3);
    uint64_t Pp[] = {9223372036854775783ull}, Ps[] = {2147483629ull, 2147483647ull};
    unsigned Ep[] = {1}, Es[] = {1, 1};
    td_case("the largest prime below 2^63", (sk_int)9223372036854775783ull, Pp, Ep, 1);
    td_case("2147483629 * 2147483647", (sk_int)(2147483629ull * 2147483647ull), Ps, Es, 2);
    /* 1 and 0: empty lists, no candidate (0 is refused by the factoring: its list stays empty) */
    sk_int Z[8] = {P2(0, 1), P2(1, 1), P2(0, 1), P2(1, 1)};
    for (int r = R_PRIMES; r <= R_COPRIME; r++) {
        void *x = ROWS[r].create(2, 2, M2_A);
        for (int i = 0; i < 4; i++) CHECK_EQ(ROWS[r].enter(x, Z[2 * i], Z[2 * i + 1]), SK_OK);
        m3_ffig g = mat_ffig(&ROWS[r], x);
        CHECK(g.factored && g.W == 2 && g.E == 0 && g.nq == 0);
        for (uint32_t k = 0; k < 2; k++) {
            uint32_t n = 9;
            ROWS[r].t_nfac(x, k, &n);
            CHECK_EQ(n, 0);
        }
        m3_fcount f = mat_fc(&ROWS[r], x);
        CHECK_EQ(f.td_cand + f.td_divs + f.rgcd + f.p2tests, 0);
        const char *why;
        CHECK_EQ(ROWS[r].audit(x, &why), SK_OK);
        ROWS[r].destroy(x);
    }
    printf("  trial division: small primes and squares, 2^62, the largest prime below 2^63, a semiprime near 2^62, "
           "1 and 0; counts against the closed form\n");
}

/* ----------------------------------------------------------------------------------------- refinement, reconciliation */

static int cmp_sk(const void *a, const void *b) {
    sk_int x = *(const sk_int *)a, y = *(const sk_int *)b;
    return (x > y) - (x < y);
}

/* the base of a Pack-coprime matrix, sorted, into q[] (at most cap); returns |q| */
static uint32_t base_of(const void *x, sk_int *q, uint32_t cap) {
    uint32_t n = mat_ffig(&ROWS[R_COPRIME], x).nq;
    for (uint32_t i = 0; i < n && i < cap; i++) ROWS[R_COPRIME].t_piece(x, i, &q[i]);
    qsort(q, n < cap ? n : cap, sizeof(sk_int), cmp_sk);
    return n;
}

static int base_is(const void *x, const sk_int *want, uint32_t n) {
    sk_int q[64];
    uint32_t k = base_of(x, q, 64);
    return k == n && !memcmp(q, want, sizeof(sk_int) * n);
}

/* every factorization of a factored matrix multiplies back to its number */
static int fac_back(const row *R, const void *x) {
    uint32_t W = 0;
    sk_int *v = mat_vals(R, x, &W);
    int ok = 1;
    for (uint32_t k = 0; k < W && ok; k++) {
        uint32_t ne = 0;
        i128 p = fac_product(R, x, k, &ne);
        ok = v[k] < 2 ? ne == 0 : p == (i128)v[k];
    }
    free(v);
    return ok;
}

static void test_refinement(void) {
    struct {
        sk_int v[4];
        sk_int base[4];
        uint32_t nb;
        int split;
    } T[] = {
        {{12, 18, 12, 18}, {2, 3}, 2, 1},
        {{15, 45, 15, 45}, {3, 5}, 2, 1},
        {{15, 225, 15, 225}, {15}, 1, 0}, /* a composite piece kept: 3 and 5 always occur together */
        {{36, 6, 4, 36}, {2, 3}, 2, 1},
    };
    const row *R = &ROWS[R_COPRIME];
    for (size_t i = 0; i < sizeof T / sizeof *T; i++) {
        void *x = R->create(2, 2, M2_A);
        for (int c = 0; c < 4; c++) CHECK_EQ(R->enter(x, T[i].v[c], 1), SK_OK);
        CHECK(base_is(x, T[i].base, T[i].nb));
        CHECK(fac_back(R, x));
        m3_fcount f = mat_fc(R, x);
        CHECK(f.splits >= (uint64_t)T[i].split && f.rgcd > 0 && f.p2tests > 0 && (!T[i].split || f.sgcd > 0));
        const char *why;
        CHECK_EQ(R->audit(x, &why), SK_OK);
        CHECK_EQ(R->t_coprime(x, &why), SK_OK);
        R->destroy(x);
    }
    /* 225 = 15^2: nu_15 = 2 */
    void *x = R->create(1, 1, M2_A);
    CHECK_EQ(R->enter(x, 225, 1), SK_OK);
    CHECK_EQ(R->enter(x, 0, 1), SK_EINVAL); /* full */
    R->destroy(x);
    printf("  refinement: {12, 18}, {15, 45}, {15, 225} (15 kept), {36, 6, 4}; pairwise coprime by all pairs\n");
}

/* one 2x2 step of Pack-coprime on A, B (op: 0 add, 1 mul), with the reference; the result's arithmetic counters */
static m3_acount pc_step(int op, const sk_int *A, const sk_int *B, job *keep) {
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(op, 2, in, want);
    job j = job_run(&ROWS[R_COPRIME], op, 2, in, 0);
    CHECK_EQ(j.st, SK_OK);
    CHECK_EQ(job_check(&j, want), 0);
    CHECK(job_audits(&j));
    m3_acount a = mat_ac(&ROWS[R_COPRIME], j.M[M2_R]);
    ref_free(want);
    *keep = j;
    return a;
}

static void test_reconcile(void) {
    job j;
    /* L's numerator piece 6 against R's denominator piece 2 (x = 6/1, y = 1/2: cross_cb, k = gcd(6, 2) by merge) */
    sk_int A1[8] = {P2(6, 1), P2(6, 1), P2(6, 1), P2(6, 1)}, B1[8] = {P2(1, 2), P2(1, 2), P2(1, 2), P2(1, 2)};
    sk_int b23[2] = {2, 3}, b2[1] = {2}, b6[1] = {6};
    m3_acount a = pc_step(SET_MUL, A1, B1, &j);
    CHECK(a.rc_splits >= 1 && a.rc_passes >= 2 && a.rebuilds >= 1);
    CHECK(base_is(j.M[M2_A], b23, 2));
    CHECK(base_is(j.M[M2_B], b2, 1));
    CHECK_EQ(a.rc_DR, 1);
    CHECK(fac_back(&ROWS[R_COPRIME], j.M[M2_A]));
    job_free(&j);
    /* the other direction: R's numerator piece 6 against L's denominator piece 2 (x = 1/2, y = 6/1: cross_ad) */
    a = pc_step(SET_MUL, B1, A1, &j);
    CHECK(a.rc_splits >= 1 && a.rc_passes >= 2);
    CHECK(base_is(j.M[M2_B], b23, 2));
    CHECK(base_is(j.M[M2_A], b2, 1));
    CHECK_EQ(a.rc_DL, 1);
    job_free(&j);
    /* no split needed: the pieces already equal or coprime (6 against 5) */
    sk_int B2[8] = {P2(1, 5), P2(1, 5), P2(1, 5), P2(1, 5)};
    a = pc_step(SET_MUL, A1, B2, &j);
    CHECK(a.rc_splits == 0 && a.rc_passes == 1 && a.rebuilds == 0);
    CHECK(base_is(j.M[M2_A], b6, 1));
    job_free(&j);
    /* a reconciliation that needs a second pass: refine(L, D_R) splits L's denominator piece 6 (against R's 10),
     * leaving D_L stale; refine(R, D_L) splits R's pieces against the stale 6; the second pass makes D_L again from
     * L's (f) and checks every pair */
    sk_int A3[8] = {P2(1, 6), P2(1, 6), P2(1, 6), P2(1, 6)}, B3[8] = {P2(1, 10), P2(1, 10), P2(3, 10), P2(1, 10)};
    a = pc_step(SET_ADD, A3, B3, &j);
    CHECK(a.rc_passes >= 2 && a.rc_splits >= 1);
    sk_int b235[3] = {2, 3, 5};
    sk_int q[8];
    uint32_t n = base_of(j.M[M2_A], q, 8);
    CHECK(n == 2 && q[0] == 2 && q[1] == 3);
    n = base_of(j.M[M2_B], q, 8);
    CHECK(n == 3 && !memcmp(q, b235, sizeof b235)); /* 10 split into 2 and 5; 3 a numerator */
    job_free(&j);
    printf("  reconciliation: a numerator piece split against a denominator piece, both directions; no split when "
           "equal or coprime; a second pass\n");
}

static void test_leftover(void) {
    job j;
    /* inside a multiplication's sum: A all 1/6 (base {6}), B all 1: s = 1/6, p = 1/6, case den, t = 2:
     * gcd(2, 6) = 2 is not a power of 6: 6 split into 2 and 3, in A and in the live lists */
    sk_int A[8] = {P2(1, 6), P2(1, 6), P2(1, 6), P2(1, 6)}, B[8] = {P2(1, 1), P2(1, 1), P2(1, 1), P2(1, 1)};
    sk_int b23[2] = {2, 3}, b6[1] = {6};
    m3_acount a = pc_step(SET_MUL, A, B, &j);
    CHECK(a.lsplits >= 1 && a.rc_splits == 0 && a.pgcd > 0);
    CHECK(base_is(j.M[M2_A], b23, 2));
    CHECK(fac_back(&ROWS[R_COPRIME], j.M[M2_A]));
    job_free(&j);
    /* Pack-primes on the same: no split exists; the tests divide t by 2 once, then fail at 3 */
    {
        const sk_int *in[2] = {A, B};
        job k = job_run(&ROWS[R_PRIMES], SET_MUL, 2, in, 0);
        CHECK_EQ(k.st, SK_OK);
        m3_acount b = mat_ac(&ROWS[R_PRIMES], k.M[M2_R]);
        CHECK(b.lsplits == 0 && b.pgcd == 0 && b.tests > 0);
        job_free(&k);
    }
    /* in a stored sum: 1/6 + 1/6 = 1/3, the same leftover, but the result is stored: nothing split */
    a = pc_step(SET_ADD, A, A, &j);
    CHECK(a.lsplits == 0 && a.rc_splits == 0 && a.pgcd == 4);
    CHECK(base_is(j.M[M2_A], b6, 1));
    CHECK(base_is(j.M[M2_B], b6, 1));
    job_free(&j);
    printf("  leftovers: a split inside a multiplication's sum (piece 6, t = 2); none in a stored sum\n");
}

/* ----------------------------------------------------------------------------------------- the cost order */

static void test_cost_order(void) {
    /* every value +-a/b, 0 <= a <= 12, 1 <= b <= 12, in lowest terms, once */
    sk_int V[400][2];
    int nv = 0;
    V[nv][0] = 0;
    V[nv++][1] = 1;
    for (sk_int b = 1; b <= 12; b++)
        for (sk_int a = 1; a <= 12; a++)
            if (ref_gcd(a, b) == 1) {
                V[nv][0] = a;
                V[nv++][1] = b;
                V[nv][0] = -a;
                V[nv++][1] = b;
            }
    uint32_t n = (uint32_t)nv;
    sk_int *A = malloc(16 * (size_t)n * n), *B = malloc(16 * (size_t)n * n);
    for (int op = 0; op < 2; op++) {
        /* add: A[i][j] = V[i], B[i][j] = V[j]: every pair once; mul: A[i][k] = V[(i + k) % n], B[k][j] = V[j]: every
         * product V[u] V[j], and sums of every kind on temporaries */
        for (uint32_t i = 0; i < n; i++)
            for (uint32_t j = 0; j < n; j++) {
                uint32_t a = op ? (i + j) % n : i;
                A[2 * (i * n + j)] = V[a][0];
                A[2 * (i * n + j) + 1] = V[a][1];
                B[2 * (i * n + j)] = V[j][0];
                B[2 * (i * n + j) + 1] = V[j][1];
            }
        const sk_int *in[2] = {A, B};
        ref_q *want[M2_NMAT];
        job_ref(op, n, in, want);
        uint64_t nops = op ? 2ull * n * n * n : (uint64_t)n * n;
        int16_t *pc[NROWS];
        uint64_t gchk[NROWS];
        for (int r = 0; r < NROWS; r++) {
            pc[r] = malloc(sizeof(int16_t) * M1_PERCALL * nops);
            m1_percall = pc[r];
            m1_percall_n = 0;
            m1_percall_cap = nops;
            job j = job_run(&ROWS[r], op, n, in, 0);
            m1_percall = NULL;
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(m1_percall_n, nops);
            CHECK_EQ(mat_check(&ROWS[r], j.M[M2_R], n, want[M2_R]), 0);
            gchk[r] = mat_ac(&ROWS[r], j.M[M2_R]).gchecked;
            job_free(&j);
        }
        for (int r = 1; r < NROWS; r++) {
            uint64_t bad = 0;
            for (uint64_t i = 0; i < nops; i++) {
                const int16_t *x = pc[r] + M1_PERCALL * i, *t = pc[0] + M1_PERCALL * i;
                if (ROWS[r].fac ? (x[0] != t[0] || x[3] != t[3] || x[4] != t[4] || x[5] != t[5])
                                : memcmp(x, t, sizeof(int16_t) * M1_PERCALL))
                    bad++;
            }
            CHECK_EQ(bad, 0);
            if (ROWS[r].fac) CHECK(gchk[r] > 0);
        }
        printf("    %s: %llu operations on %d values, every row's case and core arithmetic TC's; obtained gcds checked: "
               "%llu (Pack-primes), %llu (Pack-coprime)\n",
               op ? "multiplication" : "addition", (unsigned long long)nops, nv, (unsigned long long)gchk[R_PRIMES],
               (unsigned long long)gchk[R_COPRIME]);
        for (int r = 0; r < NROWS; r++) free(pc[r]);
        ref_free(want);
    }
    free(A);
    free(B);
    printf("  the cost order, exhaustively: +-a/b, 0 <= a <= 12, 1 <= b <= 12, every pair, all four rows\n");
}

/* ----------------------------------------------------------------------------------------- m2's tests, four rows */

static void test_zero_one(void) {
    sk_int A[8] = {P2(7, 3), P2(0, 1), P2(1, 1), P2(3, 1)}, B[8] = {P2(5, 1), P2(0, 1), P2(2, 3), P2(1, 1)};
    const sk_int *in[2] = {A, B};
    for (int op = 0; op < 2; op++) {
        ref_q *want[M2_NMAT];
        job_ref(op, 2, in, want);
        uint64_t tc[R2_NCASE];
        for (int r = 0; r < NROWS; r++) {
            job j = job_run(&ROWS[r], op, 2, in, 0);
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(job_check(&j, want), 0);
            CHECK(job_audits(&j));
            for (int k = 0; k < R2_NCASE; k++) {
                uint64_t v = job_case(&j, op ? SK_PH_MUL : SK_PH_ADD, k) + job_case(&j, SK_PH_ADD, k) * op;
                if (r == 0) tc[k] = v;
                else CHECK_EQ(v, tc[k]);
            }
            job_free(&j);
        }
        ref_free(want);
    }
    sk_int N[8] = {P2(2, 3), P2(4, 5), P2(-2, 7), P2(3, 5)};
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        void *x = R->create(2, 2, M2_A);
        for (int i = 0; i < 4; i++) CHECK_EQ(R->enter(x, N[2 * i], N[2 * i + 1]), SK_OK);
        CHECK_EQ(mat_fig(R, x).W, 5); /* 2, 3, 4, 5, 7 */
        uint32_t nv = 0;
        sk_int *v = mat_vals(R, x, &nv);
        for (uint32_t s = 0; s < nv; s++) CHECK(v[s] != 0 && v[s] != 1);
        free(v);
        if (R->fac) CHECK_EQ(mat_ffig(R, x).E, R->coprime ? 5u : 5u); /* 2, 3, 2^2, 5, 7: one entry each */
        const char *why;
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
    }
    printf("  zero, integer and +-1 cases by value; a matrix with no 0 and no 1\n");
}

static void test_widening(void) {
    const sk_int big = (sk_int)1 << 40;
    sk_int A[8] = {P2(100, 1), P2(-1000, 1), P2(100000, 1), P2(big, 1)}, B[8] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(SET_ADD, 2, in, want);
    job jp = job_run(&ROWS[R_PACK], SET_ADD, 2, in, 0);
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        job j = job_run(R, SET_ADD, 2, in, 0);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        m2_scount s = mat_store(R, j.M[M2_R]);
        m2_fig f = mat_fig(R, j.M[M2_R]);
        CHECK_EQ(s.widenings, 3);
        CHECK_EQ(s.copied, 3 * f.T);
        CHECK_EQ(f.w, 64);
        if (R->fac) {
            CHECK(same_as_pack(R, jp.M[M2_R], j.M[M2_R]));
            m3_ffig g = mat_ffig(R, j.M[M2_R]);
            CHECK_EQ(g.bytes_f, 4ull * (g.W + 1) + (uint64_t)g.E * 9); /* factors at the list's width, 64 */
        }
        CHECK(job_audits(&j));
        job_free(&j);
    }
    job_free(&jp);
    ref_free(want);
    sk_int steps[3] = {1000, 100000, big};
    unsigned wants[3] = {16, 32, 64};
    for (int k = 0; k < 3; k++) {
        sk_int A1[2] = {steps[k], 1}, B1[2] = {0, 1};
        const sk_int *in1[2] = {A1, B1};
        for (int r = 1; r < NROWS; r++) {
            job j = job_run(&ROWS[r], SET_ADD, 1, in1, 0);
            CHECK_EQ(mat_store(&ROWS[r], j.M[M2_R]).widenings, 1);
            CHECK_EQ(mat_fig(&ROWS[r], j.M[M2_R]).w, wants[k]);
            if (ROWS[r].fac) {
                m3_ffig g = mat_ffig(&ROWS[r], j.M[M2_R]);
                CHECK_EQ(g.bytes_f, 4ull * (g.W + 1) + (uint64_t)g.E * (wants[k] / 8 + 1));
                if (ROWS[r].coprime) CHECK_EQ(g.bytes_q, (uint64_t)g.nq * (wants[k] / 8));
            }
            CHECK(job_audits(&j));
            job_free(&j);
        }
    }
    printf("  widening: 8 -> 16 -> 32 -> 64; each step alone; (f) and (q) at the list's width\n");
}

/* the factored rows' matrix of n x n cells against Hash-V-pack's: the same (h) at the packing, list and handles */
static sk_int *HB[NROWS];
static uint32_t HBT[NROWS], HBN;
static int HROW;
static void pack_hook(const m2_pack *p) {
    HBN++;
    free(HB[HROW]);
    HBT[HROW] = p->T;
    HB[HROW] = malloc(sizeof(sk_int) * p->T);
    for (uint32_t s = 0; s < p->T; s++) {
        if (HROW == R_PACK) mhvp_t_whole((const mhvp_class *)p->x, s, &HB[HROW][s]);
        else if (HROW == R_PRIMES) mhpp_t_whole((const mhpp_class *)p->x, s, &HB[HROW][s]);
        else mhpc_t_whole((const mhpc_class *)p->x, s, &HB[HROW][s]);
    }
}
static int FHN;
static void factor_hook(const m3_factor *f) {
    FHN++;
    CHECK(f->fig.factored);
}

static void pack_case(uint32_t n, const sk_int *X, unsigned wb, unsigned wp) {
    void *M[NROWS] = {NULL};
    m2_pack_hook = pack_hook;
    m3_factor_hook = factor_hook;
    for (int r = R_PACK; r < NROWS; r++) {
        HROW = r;
        HBN = 0;
        FHN = 0;
        M[r] = ROWS[r].create(n, n, M2_A);
        for (uint32_t i = 0; i < n * n; i++) {
            CHECK_EQ(ROWS[r].enter(M[r], X[2 * i], X[2 * i + 1]), SK_OK);
            CHECK_EQ(HBN, i + 1 == n * n); /* packed inside the last cell's enter, once */
            if (ROWS[r].fac) CHECK_EQ(FHN, i + 1 == n * n); /* and factored there, once */
        }
        m2_fig f = mat_fig(&ROWS[r], M[r]);
        CHECK(f.packed);
        CHECK_EQ(f.wh_build, wb);
        CHECK_EQ(f.wh, wp);
        if (r > R_PACK) {
            CHECK(same_as_pack(&ROWS[r], M[R_PACK], M[r]));
            CHECK(HBT[r] == HBT[R_PACK] && !memcmp(HB[r], HB[R_PACK], sizeof(sk_int) * HBT[r])); /* (h), slot for slot */
            CHECK(fac_back(&ROWS[r], M[r]));
        }
        const char *why;
        CHECK_EQ(ROWS[r].audit(M[r], &why), SK_OK);
    }
    m2_pack_hook = NULL;
    m3_factor_hook = NULL;
    for (int r = R_PACK; r < NROWS; r++) ROWS[r].destroy(M[r]);
}

static void test_packing(void) {
    struct {
        uint32_t n;
        int kind;
        unsigned wb, wp;
    } P[] = {{2, 2, 8, 8}, {7, 2, 8, 8}, {8, 0, 16, 8}, {12, 2, 16, 16}, {128, 0, 32, 8}, {128, 1, 32, 16},
             {130, 2, 32, 32}};
    for (size_t k = 0; k < sizeof P / sizeof *P; k++) {
        uint32_t n = P[k].n;
        sk_int *X = malloc(16 * (size_t)n * n);
        for (uint32_t i = 0; i < n * n; i++) {
            uint32_t v = P[k].kind == 0 ? 0 : P[k].kind == 1 ? i % 100 : i;
            X[2 * i] = (sk_int)(2 * v + 2) * (i % 3 ? 1 : -1); /* 2v + 2 and 2v + 3: coprime, all distinct */
            X[2 * i + 1] = (sk_int)(2 * v + 3);
        }
        pack_case(n, X, P[k].wb, P[k].wp);
        free(X);
    }
    for (int r = 0; r < NROWS; r++) free(HB[r]);
    printf("  packing: every packed width (8, 16, 32), every pair of widths in place; the factored rows' (h), list and "
           "handles Hash-V-pack's; factored once, inside the last enter\n");
}

static void test_expressions(void) {
    for (int seed = 1; seed <= 20; seed++)
        for (int set = SET_SOP; set <= SET_POS; set++) {
            rng_s = 5000 + (uint64_t)seed;
            uint32_t n = 1 + (uint32_t)seed % 5;
            sk_int *X[4];
            for (int x = 0; x < 4; x++) {
                X[x] = malloc(16 * n * n);
                for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                    uint64_t k = rnd() % 8;
                    X[x][i] = k == 0 ? 0 : k == 1 ? 1 : (sk_int)(rnd() % 201) - 100;
                    X[x][i + 1] = k == 2 ? 1 : D720[rnd() % 30];
                }
            }
            const sk_int *in[4] = {X[0], X[1], X[2], X[3]};
            ref_q *want[M2_NMAT];
            job_ref(set, n, in, want);
            uint64_t tc[R2_NCASE * 2];
            for (int r = 0; r < NROWS; r++) {
                const row *R = &ROWS[r];
                job j = job_run(R, set, n, in, 1);
                CHECK_EQ(j.st, SK_OK);
                CHECK_EQ(job_check(&j, want), 0); /* the intermediates and R */
                CHECK(job_audits(&j));
                for (int k = 0; k < R2_NCASE; k++)
                    for (int ph = 0; ph < 2; ph++) {
                        uint64_t v = job_case(&j, ph ? SK_PH_MUL : SK_PH_ADD, k);
                        if (r == 0) tc[2 * k + ph] = v;
                        else CHECK_EQ(v, tc[2 * k + ph]);
                    }
                if (R->hash)
                    for (int x = 0; x < M2_NMAT; x++) CHECK(mat_fig(R, j.M[x]).packed);
                if (R->fac)
                    for (int x = 0; x < M2_NMAT; x++) CHECK(mat_ffig(R, j.M[x]).factored && fac_back(R, j.M[x]));
                job_free(&j);
                j = job_run(R, set, n, in, 0);
                CHECK(!j.M[M2_X1] && !j.M[M2_X2]);
                CHECK_EQ(mat_check(R, j.M[M2_R], n, want[M2_R]), 0);
                job_free(&j);
            }
            ref_free(want);
            for (int x = 0; x < 4; x++) free(X[x]);
        }
    printf("  expressions: sop and pos against the exact reference, every intermediate; freed after last use\n");
}

static void test_intermediate_widths(void) {
    const sk_int vals[4] = {100, 1000, 100000, (sk_int)1 << 40};
    const unsigned w[4] = {8, 16, 32, 64};
    for (int k = 0; k < 4; k++) {
        sk_int V[8] = {P2(vals[k], 1), P2(-vals[k], 1), P2(vals[k], 3), P2(1, 1)};
        sk_int Z[8] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)}, I[8] = {P2(1, 1), P2(0, 1), P2(0, 1), P2(1, 1)};
        sk_int S[8] = {P2(1, 2), P2(2, 3), P2(-3, 4), P2(5, 6)};
        const sk_int *pos[4] = {V, Z, I, Z}, *sop[4] = {V, I, S, S};
        for (int set = SET_SOP; set <= SET_POS; set++) {
            const sk_int *const *in = set == SET_POS ? pos : sop;
            ref_q *want[M2_NMAT];
            job_ref(set, 2, in, want);
            for (int r = 0; r < NROWS; r++) {
                const row *R = &ROWS[r];
                job j = job_run(R, set, 2, in, 1);
                CHECK_EQ(j.st, SK_OK);
                CHECK_EQ(job_check(&j, want), 0);
                if (R->hash) CHECK_EQ(mat_fig(R, j.M[M2_X1]).w, w[k]);
                CHECK(job_audits(&j));
                job_free(&j);
            }
            ref_free(want);
        }
    }
    printf("  intermediates of every width (8, 16, 32, 64) as operands, in sop and pos\n");
}

static void test_handle_width(void) {
    struct {
        uint32_t n;
        unsigned wh;
    } W[] = {{1, 8}, {5, 8}, {7, 8}, {8, 16}, {16, 16}, {64, 16}, {90, 16}, {127, 16}, {128, 32}};
    for (size_t i = 0; i < sizeof W / sizeof *W; i++)
        for (int r = 1; r < NROWS; r++) {
            void *x = ROWS[r].create(W[i].n, W[i].n, i % 2 ? M2_A : M2_R);
            m2_fig f;
            ROWS[r].figures(x, &f);
            CHECK_EQ(f.wh, W[i].wh);
            uint64_t kb = 2ull * W[i].n * W[i].n + 2, tf = 1;
            while (tf < kb) tf <<= 1;
            CHECK_EQ(f.T, (uint32_t)tf);
            CHECK_EQ(f.W, 0);
            CHECK_EQ(f.w, i % 2 ? 64u : 8u);
            uint32_t nv = 0;
            sk_int *v = mat_vals(&ROWS[r], x, &nv);
            for (uint32_t s = 0; s < nv; s++) CHECK_EQ(v[s], -1);
            free(v);
            if (ROWS[r].fac) CHECK(!mat_ffig(&ROWS[r], x).factored);
            ROWS[r].destroy(x);
        }
    printf("  w_h at the thresholds (8, 16, 32 bits); T_fix; every build table empty at creation\n");
}

static void test_overflow(void) {
    const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
    struct {
        int set;
        uint32_t n;
        sk_int X[4][8];
    } T[] = {
        {SET_ADD, 1, {{h62, 1}, {h62, 1}}},
        {SET_ADD, 1, {{-h62, 1}, {-h62, 1}}},
        {SET_MUL, 1, {{h32, 1}, {h32, 1}}},
        {SET_MUL, 2, {{-h62, 1, -h62, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}}},
        {SET_MUL, 1, {{h62, 3}, {2, 5}}},
        {SET_SOP, 1, {{h62, 1}, {1, 1}, {h62, 1}, {1, 1}}},
        {SET_POS, 1, {{h32, 1}, {0, 1}, {h32, 1}, {0, 1}}},
    };
    for (size_t i = 0; i < sizeof T / sizeof *T; i++) {
        const sk_int *in[4] = {T[i].X[0], T[i].X[1], T[i].X[2], T[i].X[3]};
        for (int r = 0; r < NROWS; r++) {
            job j = job_run(&ROWS[r], T[i].set, T[i].n, in, 1);
            CHECK_EQ(j.st, SK_EOVERFLOW);
            sk_overflow o;
            CHECK_EQ(ROWS[r].overflow(j.M[j.last], &o), SK_OK);
            CHECK(o.count == 1 && o.step);
            job_free(&j);
        }
    }
    for (int r = 0; r < NROWS; r++) {
        void *x = ROWS[r].create(1, 1, M2_A);
        CHECK_EQ(ROWS[r].enter(x, INT64_MIN, 1), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, INT64_MIN), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, 0), SK_EDIVZERO);
        ROWS[r].destroy(x);
    }
    printf("  overflow: 2^63, -2^63, 2^64 and add(s, p) to -2^63 return the error in every row and job set\n");
}

static void test_probing(void) {
    /* a value homed at slot 0 is at list position 0; a probe that wraps: the factored rows as Hash-V-pack */
    for (int r = R_PACK; r < NROWS; r++) {
        const row *R = &ROWS[r];
        unsigned t = 2; /* 1x1: T_fix = 4 */
        sk_int v[2] = {0, 0};
        int k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if ((((uint64_t)c * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t)) == (k ? 3u : 0u) &&
                (k == 0 || ref_gcd(c, v[0]) == 1))
                v[k++] = c;
        CHECK_EQ(k, 2);
        void *x = R->create(1, 1, M2_A);
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        uint32_t f0 = 0, f1 = 0;
        R->t_field(x, 0, &f0);
        R->t_field(x, 1, &f1);
        CHECK(f0 >> 1 == 0 && f1 >> 1 == 1);
        sk_int m = 0, d = 0;
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        const char *why;
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
        x = R->create(1, 1, M2_A);
        k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if ((((uint64_t)c * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t)) == 3u && (k == 0 || ref_gcd(c, v[0]) == 1))
                v[k++] = c;
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        m2_scount s = mat_store(R, x);
        CHECK_EQ(s.longest, 2);
        CHECK_EQ(s.probes, 3);
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
    }
    printf("  probing: a value homed at slot 0; a probe that wraps\n");
}

static void test_audits(void) {
    sk_int A[8] = {P2(12, 35), P2(-4, 5), P2(0, 1), P2(7, 1)}, B[8] = {P2(1, 3), P2(4, 7), P2(5, 1), P2(-7, 3)};
    const sk_int *in[2] = {A, B};
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        const char *why;
        job j = job_run(R, SET_ADD, 2, in, 0);
        CHECK(job_audits(&j));
        m2_fig fa = mat_fig(R, j.M[M2_A]);
        uint32_t f = 0;
        R->t_field(j.M[M2_A], 0, &f);
        R->t_corrupt_field(j.M[M2_A], 0, 2 * fa.W); /* a handle of a packed matrix >= W */
        CHECK_EQ(R->audit(j.M[M2_A], &why), SK_EAUDIT);
        R->t_corrupt_field(j.M[M2_A], 0, f);
        CHECK_EQ(R->audit(j.M[M2_A], &why), SK_OK);
        sk_int v0 = 0, v1 = 0;
        R->t_whole(j.M[M2_R], 0, &v0);
        R->t_whole(j.M[M2_R], 1, &v1);
        R->t_corrupt_slot(j.M[M2_R], 1, v0); /* two list entries the same */
        CHECK_EQ(R->audit(j.M[M2_R], &why), SK_EAUDIT);
        R->t_corrupt_slot(j.M[M2_R], 1, v1);
        if (R->fac) { /* (f): a list that does not multiply back; out of order; an exponent of 0 */
            uint32_t W = fa.W, k = 0, n = 0;
            for (k = 0; k < W; k++) {
                R->t_nfac(j.M[M2_A], k, &n);
                if (n >= 2) break;
            }
            CHECK(k < W); /* 12 = 2^2 3, or 35 = 5 7 */
            sk_int q0, q1;
            unsigned e0, e1;
            R->t_fac(j.M[M2_A], k, 0, &q0, &e0);
            R->t_fac(j.M[M2_A], k, 1, &q1, &e1);
            R->t_corrupt_fac(j.M[M2_A], k, 0, q0, e0 + 1);
            CHECK_EQ(R->audit(j.M[M2_A], &why), SK_EAUDIT);
            R->t_corrupt_fac(j.M[M2_A], k, 0, q1, e1); /* out of order (and a duplicate) */
            R->t_corrupt_fac(j.M[M2_A], k, 1, q0, e0);
            CHECK_EQ(R->audit(j.M[M2_A], &why), SK_EAUDIT);
            R->t_corrupt_fac(j.M[M2_A], k, 0, q0, 0);
            R->t_corrupt_fac(j.M[M2_A], k, 1, q1, e1);
            CHECK_EQ(R->audit(j.M[M2_A], &why), SK_EAUDIT);
            R->t_corrupt_fac(j.M[M2_A], k, 0, q0, e0);
            CHECK_EQ(R->audit(j.M[M2_A], &why), SK_OK);
        }
        job_free(&j);
    }
    job j = job_run(&ROWS[R_TC], SET_ADD, 2, in, 0);
    tcm_t_corrupt((tcm_class *)j.M[M2_R], 0, 2, 4);
    const char *why;
    CHECK_EQ(tcm_audit((tcm_class *)j.M[M2_R], &why), SK_EAUDIT);
    job_free(&j);
    printf("  audits: broken fields, list entries and factorizations are caught\n");
}

/* the known-answer switches off (test build): the factored rows still obtain every gcd and match TC case for case */
static void test_known_off(void) {
    for (int seed = 1; seed <= 10; seed++)
        for (int set = SET_ADD; set <= SET_MUL; set++) {
            rng_s = 900 + (uint64_t)seed;
            uint32_t n = 3;
            sk_int X[2][18];
            for (int x = 0; x < 2; x++)
                for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                    X[x][i] = (sk_int)(rnd() % 41) - 20;
                    X[x][i + 1] = D720[rnd() % 30];
                }
            const sk_int *in[2] = {X[0], X[1]};
            ref_q *want[M2_NMAT];
            job_ref(set, n, in, want);
            uint64_t tc[R2_NCASE];
            for (int r = 0; r < NROWS; r++) {
                const row *R = &ROWS[r];
                job j = {R, n, set, {NULL}, SK_OK, NULL, -1};
                for (int x = 0; x < 2; x++) {
                    j.M[x] = R->create(n, n, x);
                    for (uint32_t i = 0; i < n * n; i++) R->enter(j.M[x], in[x][2 * i], in[x][2 * i + 1]);
                }
                j.M[M2_R] = R->create(n, n, M2_R);
                R->t_known_off(j.M[M2_R], R2_KOFF_ALL);
                CHECK_EQ(R->op[set](j.M[M2_A], j.M[M2_B], j.M[M2_R]), SK_OK);
                CHECK_EQ(job_check(&j, want), 0);
                for (int k = 0; k < R2_NCASE; k++) {
                    uint64_t v = job_case(&j, set ? SK_PH_MUL : SK_PH_ADD, k) + (set ? job_case(&j, SK_PH_ADD, k) : 0);
                    if (r == 0) tc[k] = v;
                    else CHECK_EQ(v, tc[k]);
                }
                job_free(&j);
            }
            ref_free(want);
        }
    printf("  the known-answer switches off: every row TC's case for case\n");
}

int main(int argc, char **argv) {
    int quick = argc > 1 && !strcmp(argv[1], "--quick"); /* the harness's own toy checks: no large trial divisions */
    printf("m3_unit (test build)%s\n", quick ? ", --quick" : "");
    test_cases();
    test_subcases();
    if (!quick) test_trial_division();
    test_refinement();
    test_reconcile();
    test_leftover();
    test_cost_order();
    test_zero_one();
    test_widening();
    test_packing();
    test_expressions();
    test_intermediate_widths();
    test_handle_width();
    test_overflow();
    test_probing();
    test_audits();
    test_known_off();
    printf("m3_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
