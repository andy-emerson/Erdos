/* m4_unit.c: the unit tests of round m4 (test/DECISIONS-m4.md; test build): m3_unit.c (unchanged, m3's) on the rows
 * TC, Hash-V-pack-4 (mh4.c) and Hash-V-pack (mh2.c, the reference row), without m3's factored rows' own tests (mh3.c
 * is m3's, tested by m3's suites).  Where m3_unit.c checks mh2.c's layout (the bound 2n^2 + 2, T_fix, the build handle
 * width, the widenings, (h) slot for slot, the list's order and the handles), Hash-V-pack-4 is checked by its own
 * rules (the bound 2n^2; no widening; the list narrowed at the pack to the widest value's width) and against
 * Hash-V-pack by value (every field's value and tag, the values held, as sets).  The tests kept: every case of
 * addition and multiplication, on cells and on temporaries; the cost order, exhaustively (every row TC's case and core
 * arithmetic); zero, integer and +-1 by value; each width of a result; the packing at every pair of widths; both
 * expressions with every intermediate; intermediates of every width as operands; w_h and T_fix; overflow; probing;
 * the audits; the known-answer switches off. */
#include "m4t.h"

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
    printf("  cases: %zu addition, %zu multiplication, every addition case on temporaries; every row\n",
           sizeof ADD_CASES / sizeof *ADD_CASES, sizeof MUL_CASES / sizeof *MUL_CASES);
}

/* ----------------------------------------------------------------------------------------- the cost order */

/* every addition and multiplication of two values +-a/b, 0 <= a <= 12, 1 <= b <= 12: every row's case, value and core
 * arithmetic (gcds, gcd work, exact divisions, multiplications, additions) TC's */
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
            job_free(&j);
        }
        for (int r = 1; r < NROWS; r++) {
            uint64_t bad = 0;
            for (uint64_t i = 0; i < nops; i++) {
                const int16_t *x = pc[r] + M1_PERCALL * i, *t = pc[0] + M1_PERCALL * i;
                if (memcmp(x, t, sizeof(int16_t) * M1_PERCALL))
                    bad++;
            }
            CHECK_EQ(bad, 0);
        }
        printf("    %s: %llu operations on %d values, every row's case and core arithmetic TC's\n",
               op ? "multiplication" : "addition", (unsigned long long)nops, nv);
        for (int r = 0; r < NROWS; r++) free(pc[r]);
        ref_free(want);
    }
    free(A);
    free(B);
    printf("  the cost order, exhaustively: +-a/b, 0 <= a <= 12, 1 <= b <= 12, every pair, every row\n");
}

/* ----------------------------------------------------------------------------------------- m2's tests */

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
        CHECK_EQ(s.widenings, is_v4(R) ? 0 : 3); /* Hash-V-pack-4 never widens: its list is narrowed at the pack */
        CHECK_EQ(s.copied, is_v4(R) ? 0 : 3 * f.T);
        CHECK_EQ(f.w, 64);
        if (is_v4(R)) CHECK(same_by_value(&ROWS[R_PACK], jp.M[M2_R], R, j.M[M2_R]));
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
            CHECK_EQ(mat_store(&ROWS[r], j.M[M2_R]).widenings, is_v4(&ROWS[r]) ? 0 : 1);
            CHECK_EQ(mat_fig(&ROWS[r], j.M[M2_R]).w, wants[k]); /* Hash-V-pack-4: the narrowest that holds the widest */
            CHECK(job_audits(&j));
            job_free(&j);
        }
    }
    printf("  result widths: 8 -> 16 -> 32 -> 64 (Hash-V-pack widened, Hash-V-pack-4 narrowed at the pack); each step alone\n");
}

/* Hash-V-pack-4's matrix of n x n cells against Hash-V-pack's: packed inside the last enter, once; the build width
 * named by its own bound and the packed width wp; the same values held in (h) at the packing (as sets), and the same
 * cells by value */
static sk_int *HB[NROWS];
static uint32_t HBT[NROWS], HBN;
static int HROW;
static void pack_hook(const m2_pack *p) {
    HBN++;
    free(HB[HROW]);
    HBT[HROW] = p->T;
    HB[HROW] = malloc(sizeof(sk_int) * p->T);
    for (uint32_t s = 0; s < p->T; s++) ROWS[HROW].t_whole(p->x, s, &HB[HROW][s]);
}

static void pack_case(uint32_t n, const sk_int *X, unsigned wp) {
    void *M[NROWS] = {NULL};
    m2_pack_hook = pack_hook;
    for (int r = R_PACK4; r <= R_PACK; r++) {
        HROW = r;
        HBN = 0;
        M[r] = ROWS[r].create(n, n, M2_A);
        for (uint32_t i = 0; i < n * n; i++) {
            CHECK_EQ(ROWS[r].enter(M[r], X[2 * i], X[2 * i + 1]), SK_OK);
            CHECK_EQ(HBN, i + 1 == n * n); /* packed inside the last cell's enter, once */
        }
        m2_fig f = mat_fig(&ROWS[r], M[r]);
        CHECK(f.packed);
        CHECK_EQ(f.wh_build, row_whb(&ROWS[r], n));
        CHECK_EQ(f.T, row_T(&ROWS[r], n));
        CHECK_EQ(f.wh, wp);
        const char *why;
        CHECK_EQ(ROWS[r].audit(M[r], &why), SK_OK);
    }
    CHECK(same_by_value(&ROWS[R_PACK], M[R_PACK], &ROWS[R_PACK4], M[R_PACK4]));
    CHECK(same_values_held(HB[R_PACK], HBT[R_PACK], HB[R_PACK4], HBT[R_PACK4])); /* (h) at the packing, as sets */
    m2_pack_hook = NULL;
    for (int r = R_PACK4; r <= R_PACK; r++) ROWS[r].destroy(M[r]);
}

static void test_packing(void) {
    struct {
        uint32_t n;
        int kind;
        unsigned wp;
    } P[] = {{2, 2, 8}, {7, 2, 8}, {8, 0, 8}, {12, 2, 16}, {128, 0, 8}, {128, 1, 16}, {129, 0, 8}, {129, 1, 16},
             {130, 2, 32}};
    for (size_t k = 0; k < sizeof P / sizeof *P; k++) {
        uint32_t n = P[k].n;
        sk_int *X = malloc(16 * (size_t)n * n);
        for (uint32_t i = 0; i < n * n; i++) {
            uint32_t v = P[k].kind == 0 ? 0 : P[k].kind == 1 ? i % 100 : i;
            X[2 * i] = (sk_int)(2 * v + 2) * (i % 3 ? 1 : -1); /* 2v + 2 and 2v + 3: coprime, all distinct */
            X[2 * i + 1] = (sk_int)(2 * v + 3);
        }
        pack_case(n, X, P[k].wp);
        free(X);
    }
    for (int r = 0; r < NROWS; r++) free(HB[r]);
    printf("  packing: every packed width (8, 16, 32) from every build width, in place (Hash-V-pack-4: (8, 8), (16, 8), "
           "(16, 16), (32, 8), (32, 16), (32, 32)); against Hash-V-pack by value\n");
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
            CHECK_EQ(f.wh, is_v4(&ROWS[r]) ? row_whb(&ROWS[r], W[i].n) : W[i].wh); /* the table m3 lists: mh2.c's */
            CHECK_EQ(f.T, row_T(&ROWS[r], W[i].n));
            CHECK_EQ(f.W, 0);
            CHECK_EQ(f.w, is_v4(&ROWS[r]) || i % 2 ? 64u : 8u); /* Hash-V-pack-4: every build list at 64 bits */
            uint32_t nv = 0;
            sk_int *v = mat_vals(&ROWS[r], x, &nv);
            for (uint32_t s = 0; s < nv; s++) CHECK_EQ(v[s], -1);
            free(v);
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
    /* a value homed at slot 0 is at list position 0; a probe that wraps from the last slot (1x1: T_fix = 4 for
     * Hash-V-pack, 2 for Hash-V-pack-4) */
    for (int r = R_PACK4; r < NROWS; r++) {
        const row *R = &ROWS[r];
        unsigned t = row_T(R, 1) == 4 ? 2 : 1, last = (1u << t) - 1;
        sk_int v[2] = {0, 0};
        int k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if ((((uint64_t)c * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t)) == (k ? last : 0u) &&
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
            if ((((uint64_t)c * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t)) == last && (k == 0 || ref_gcd(c, v[0]) == 1))
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
        job_free(&j);
    }
    job j = job_run(&ROWS[R_TC], SET_ADD, 2, in, 0);
    tcm_t_corrupt((tcm_class *)j.M[M2_R], 0, 2, 4);
    const char *why;
    CHECK_EQ(tcm_audit((tcm_class *)j.M[M2_R], &why), SK_EAUDIT);
    job_free(&j);
    printf("  audits: broken fields and list entries are caught\n");
}

/* the known-answer switches off (test build): every row matches TC case for case */
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

int main(void) {
    printf("m4_unit (test build)\n");
    test_cases();
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
    printf("m4_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
