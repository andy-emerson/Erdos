/* m2_unit.c: the unit tests of round m2 (spec 11-matrix v0.2 §15.7 item 6; test build), every row:
 *   - every case of addition and of multiplication with operands from two matrices, each of the four cross tests
 *     matching and not matching (V), and multiplication's add(s, p) on temporaries (every addition case);
 *   - the zero, integer and +-1 cases by value, with 0 and 1 at arbitrary slots, and a matrix that holds no 0 or no 1:
 *     no slot is reserved, W counts the whole numbers stored;
 *   - each widening of a result (8 -> 16 -> 32 -> 64), every slot and handle unchanged after it;
 *   - each growth from T0 (Hash-V-grow-map), every cell's value unchanged; a growth triggered by a denominator's store
 *     in A, in B and in a result, the same cell's numerator handle rewritten; a growth of a result mid-operation, in
 *     every job set; a growth and a widening in one store;
 *   - the packing (Hash-V-pack) at every packed width (8, 16, 32), including widths that do not shrink, and the in-place
 *     pass at every pair of widths (build 8, 16, 32 to packed 8, 16, 32, packed <= build): the list is (h)'s used slots
 *     in slot order, (h) is Hash-V-fixed's table slot for slot, every handle < W, every value unchanged;
 *   - both expressions on small matrices against the exact reference, every intermediate included; the intermediates
 *     freed after their last use; packed matrices as operands of addition and multiplication, left and right;
 *     intermediates of every width (8, 16, 32, 64) as operands;
 *   - w_h at the dimension thresholds of both rules (8, 16, 32 bits);
 *   - a 64-bit overflow and a result of -2^63 return the error in every row and job set (the timing and counting
 *     builds: m2_alloc); INT64_MIN entered is refused;
 *   - a value homed at slot 0 lands there (no slot reserved); a probe that wraps;
 *   - the audits catch a broken invariant (a packed handle >= W among them).
 * The rows against each other on random jobs of every set are m2_diff.c. */
#include "m2t.h"

#define P2(m, d) (sk_int)(m), (sk_int)(d)

/* the cross tests of two values (v0.1 §6): b = d, a = c, a = d, c = b on magnitudes */
static int matches(sk_int xm, sk_int xd, sk_int ym, sk_int yd) {
    sk_int a = xm < 0 ? -xm : xm, c = ym < 0 ? -ym : ym;
    return (xd == yd) + (a == c) + (a == yd) + (c == xd);
}

/* ----------------------------------------------------------------------------------------- the cases */

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
        if (R->hash) {
            m2_ocount o = mat_opc(R, j.M[M2_R]);
            sk_int a = t->xm < 0 ? -t->xm : t->xm, c = t->ym < 0 ? -t->ym : t->ym;
            CHECK_EQ(o.cross, 4);
            CHECK_EQ(o.cross_match, matches(t->xm, t->xd, t->ym, t->yd));
            CHECK_EQ(o.cross_reads, (a == t->xd ? 1 : 2) + (c == t->yd ? 1 : 2));
        }
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
        if (ROWS[r].hash) CHECK_EQ(mat_opc(&ROWS[r], j.M[M2_R]).cross, 4 * 8); /* the 8 products only */
        CHECK(job_audits(&j));
        job_free(&j);
    }
    ref_free(want);
}

/* each of the four cross tests matches in some case of the operation and fails in another */
static void cross_coverage(const tcase *t, size_t n) {
    int yes[4] = {0, 0, 0, 0}, no[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < n; i++) {
        sk_int a = t[i].xm < 0 ? -t[i].xm : t[i].xm, c = t[i].ym < 0 ? -t[i].ym : t[i].ym;
        int q[4] = {t[i].xd == t[i].yd, a == c, a == t[i].yd, c == t[i].xd};
        for (int k = 0; k < 4; k++) q[k] ? yes[k]++ : no[k]++;
    }
    for (int k = 0; k < 4; k++) CHECK(yes[k] > 0 && no[k] > 0);
}

static void test_cases(void) {
    cross_coverage(ADD_CASES, sizeof ADD_CASES / sizeof *ADD_CASES);
    cross_coverage(MUL_CASES, sizeof MUL_CASES / sizeof *MUL_CASES);
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) one_case(&ADD_CASES[i], SET_ADD);
    for (size_t i = 0; i < sizeof MUL_CASES / sizeof *MUL_CASES; i++) one_case(&MUL_CASES[i], SET_MUL);
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) temp_case(&ADD_CASES[i]);
    printf("  cases: %zu addition, %zu multiplication, every addition case on temporaries; V\n",
           sizeof ADD_CASES / sizeof *ADD_CASES, sizeof MUL_CASES / sizeof *MUL_CASES);
}

/* ----------------------------------------------------------------------------------------- 0 and 1 by value */

/* the slot a table of 2^t slots gives each of the values vs (distinct), stored in order: the test's own linear
 * probing, independent of mh2.c, with no slot reserved */
static void sim_slots(unsigned t, const sk_int *vs, int n, uint32_t *slot) {
    uint32_t T = 1u << t;
    sk_int *tab = malloc(sizeof(sk_int) * T);
    for (uint32_t s = 0; s < T; s++) tab[s] = -1;
    for (int i = 0; i < n; i++) {
        uint32_t s = t_home(vs[i], t);
        while (tab[s] != -1 && tab[s] != vs[i]) s = (s + 1) & (T - 1);
        tab[s] = vs[i];
        slot[i] = s;
    }
    free(tab);
}

static void test_zero_one(void) {
    /* A (2x2): a value homed at slot 0 first, so that 0 lands elsewhere; then 0/1, 1/1, 3/1.  B: 5/1, 0/1, 2/3, 1/1.
     * A + B: int_y, zero_x, int_x, int_both; A B: every case by value too.  Both against TC, case for case. */
    unsigned t = 4; /* 2x2: T_fix = 16 (fixed, pack's (h)); grow: T0 = 16 */
    sk_int c0 = 0;
    for (sk_int c = 2; c < 1000000 && !c0; c++)
        if (t_home(c, t) == 0 && c % 7) c0 = c;
    CHECK(c0 > 1);
    sk_int A[8] = {P2(c0, 7), P2(0, 1), P2(1, 1), P2(3, 1)}, B[8] = {P2(5, 1), P2(0, 1), P2(2, 3), P2(1, 1)};
    const sk_int *in[2] = {A, B};
    for (int op = 0; op < 2; op++) {
        ref_q *want[M2_NMAT];
        job_ref(op, 2, in, want);
        uint64_t tc[R2_NCASE];
        for (int r = 0; r < NROWS; r++) {
            const row *R = &ROWS[r];
            job j = job_run(R, op, 2, in, 0);
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(job_check(&j, want), 0);
            CHECK(job_audits(&j));
            for (int k = 0; k < R2_NCASE; k++) {
                uint64_t v = job_case(&j, op ? SK_PH_MUL : SK_PH_ADD, k) + job_case(&j, SK_PH_ADD, k) * op;
                if (r == 0) tc[k] = v;
                else CHECK_EQ(v, tc[k]);
            }
            if (!op) {
                CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_INT_Y), 1);
                CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_ZERO_X), 1);
                CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_INT_X), 1);
                CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_INT_BOTH), 1);
            }
            if (R->hash && !R->pack) {
                /* A's whole numbers c0, 7, 0, 1, 3, stored in order, at the test's own slots: 0 not at slot 0 */
                sk_int vs[5] = {c0, 7, 0, 1, 3};
                uint32_t slot[5], f = 0;
                sim_slots(t, vs, 5, slot);
                CHECK_EQ(slot[0], 0);
                CHECK(slot[2] != 0);
                R->t_field(j.M[M2_A], 2, &f); /* cell 1's numerator: 0 */
                CHECK_EQ(f >> 1, slot[2]);
                R->t_field(j.M[M2_A], 5, &f); /* cell 2's denominator: 1 */
                CHECK_EQ(f >> 1, slot[3]);
                CHECK_EQ(mat_fig(R, j.M[M2_A]).W, 5);
            }
            job_free(&j);
        }
        ref_free(want);
    }
    /* a matrix that holds no 0 and no 1: W is its whole numbers alone, and no slot holds 0 or 1 */
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
    printf("  zero, integer and +-1 cases by value, 0 and 1 at arbitrary slots; a matrix with no 0 and no 1\n");
}

/* ----------------------------------------------------------------------------------------- widening */

static void test_widening(void) {
    /* R = A + 0: R's whole numbers in order 100 (8 bits), 1, 1000 (16), 100000 (32), 2^40 (64) */
    const sk_int big = (sk_int)1 << 40;
    sk_int A[8] = {P2(100, 1), P2(-1000, 1), P2(100000, 1), P2(big, 1)}, B[8] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(SET_ADD, 2, in, want);
    void *fixed_r = NULL;
    job jf = job_run(&ROWS[R_FIX], SET_ADD, 2, in, 0);
    fixed_r = jf.M[M2_R];
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
        if (R->pack) {
            CHECK(pack_matches_fixed(fixed_r, j.M[M2_R]));
        } else {
            /* every slot where the test's own probing put it, and every handle naming it: nothing moved */
            sk_int vs[5] = {100, 1, 1000, 100000, big};
            uint32_t slot[5];
            unsigned t = 0;
            while ((1u << t) < f.T) t++;
            sim_slots(t, vs, 5, slot);
            int at[4] = {0, 2, 3, 4};
            for (int i = 0; i < 4; i++) {
                sk_int v = 0;
                uint32_t fl = 0;
                CHECK_EQ(R->t_whole(j.M[M2_R], slot[at[i]], &v), SK_OK);
                CHECK_EQ(v, vs[at[i]]);
                CHECK_EQ(R->t_field(j.M[M2_R], 2 * (uint32_t)i, &fl), SK_OK);
                CHECK_EQ(fl, 2 * slot[at[i]] + (i == 1));
                CHECK_EQ(R->t_field(j.M[M2_R], 2 * (uint32_t)i + 1, &fl), SK_OK);
                CHECK_EQ(fl, 2 * slot[1]);
            }
        }
        CHECK(job_audits(&j));
        job_free(&j);
    }
    job_free(&jf);
    ref_free(want);
    /* each step on its own: 8 -> 16 (one widening), 8 -> 32 straight, 8 -> 64 straight */
    sk_int steps[3] = {1000, 100000, big};
    unsigned wants[3] = {16, 32, 64};
    for (int k = 0; k < 3; k++) {
        sk_int A1[2] = {steps[k], 1}, B1[2] = {0, 1};
        const sk_int *in1[2] = {A1, B1};
        for (int r = 1; r < NROWS; r++) {
            job j = job_run(&ROWS[r], SET_ADD, 1, in1, 0);
            CHECK_EQ(mat_store(&ROWS[r], j.M[M2_R]).widenings, 1);
            CHECK_EQ(mat_fig(&ROWS[r], j.M[M2_R]).w, wants[k]);
            CHECK(job_audits(&j));
            job_free(&j);
        }
    }
    /* inputs never widen: 64-bit from creation */
    for (int r = 1; r < NROWS; r++) {
        job j = job_run(&ROWS[r], SET_ADD, 2, in, 0);
        CHECK_EQ(mat_fig(&ROWS[r], j.M[M2_A]).w, 64);
        CHECK_EQ(mat_fig(&ROWS[r], j.M[M2_B]).w, 64);
        CHECK_EQ(mat_store(&ROWS[r], j.M[M2_A]).widenings + mat_store(&ROWS[r], j.M[M2_B]).widenings, 0);
        job_free(&j);
    }
    printf("  widening: 8 -> 16 -> 32 -> 64 in one job, each step alone; slots and handles unchanged\n");
}

/* ----------------------------------------------------------------------------------------- growth */

typedef struct glog {
    int n;
    m2_growth g[64];
} glog;
static glog GL;
static void hook(const m2_growth *g) {
    if (GL.n < 64) GL.g[GL.n] = *g;
    GL.n++;
}

static void test_growth_denominator(void) {
    /* the cells 2/3, 4/5, 6/7, 8/9 (W = 8), then -2/11: 2 found, 11 the 9th new whole number: a growth while the
     * denominator is stored, with 4 cells and cell 4's numerator written (9 fields); then 0/1 cells */
    const row *R = &ROWS[R_GROW];
    sk_int V[18] = {P2(2, 3), P2(4, 5), P2(6, 7), P2(8, 9), P2(-2, 11), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    sk_int Z[18] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    for (int where = 0; where < 3; where++) {
        /* A: V + Z (A's growth, then R's); B: Z + V (B's, then R's); the result R */
        const sk_int *in[2] = {where == 1 ? Z : V, where == 1 ? V : Z};
        int id = where == 2 ? M2_R : where;
        ref_q *want[M2_NMAT];
        job_ref(SET_ADD, 3, in, want);
        GL.n = 0;
        m2_growth_hook = hook;
        job j = job_run(R, SET_ADD, 3, in, 0);
        m2_growth_hook = NULL;
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        int found = 0;
        for (int i = 0; i < GL.n && i < 64; i++)
            if (GL.g[i].id == id) {
                found++;
                CHECK_EQ(GL.g[i].fields, 9); /* cell 4's numerator written, its denominator not */
                CHECK_EQ(GL.g[i].W, 8);
                CHECK_EQ(GL.g[i].T_old, 16);
                CHECK_EQ(GL.g[i].reinserted, 8);
                CHECK_EQ(GL.g[i].rewritten, 9);
                CHECK_EQ(GL.g[i].bytes_map, 16u * mat_fig(R, j.M[id]).wh / 8);
                CHECK_EQ(GL.g[i].bytes_new, 32u * (M2_INPUT(id) ? 8 : 1));
            }
        CHECK_EQ(found, 1);
        sk_int m = 0, d = 0, v = 0;
        uint32_t f8 = 0;
        CHECK_EQ(R->read(j.M[id], 4, &m, &d), SK_OK);
        CHECK(m == -2 && d == 11);
        R->t_field(j.M[id], 8, &f8);
        R->t_whole(j.M[id], f8 >> 1, &v);
        CHECK(v == 2 && (f8 & 1));
        CHECK(job_audits(&j));
        job_free(&j);
        ref_free(want);
    }
    printf("  growth triggered by a denominator's store: in A, in B, in a result\n");
}

static void test_growth_random(void) {
    /* every growth from T0 on random matrices, every job set: values unchanged (the test build's growth audit), every
     * table at most half full; a result grows mid-operation; the same W and found/new as Hash-V-fixed */
    for (int seed = 1; seed <= 12; seed++) {
        for (int set = 0; set < 4; set++) {
            uint32_t n = 3 + (uint32_t)seed % 5;
            sk_int *X[4];
            rng_s = 1000 + (uint64_t)seed + 77 * (uint64_t)set;
            for (int x = 0; x < 4; x++) {
                X[x] = malloc(16 * n * n);
                for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                    X[x][i] = (sk_int)(rnd() % 2001) - 1000;
                    X[x][i + 1] = D720[rnd() % 30];
                }
            }
            const sk_int *in[4] = {X[0], X[1], X[2], X[3]};
            ref_q *want[M2_NMAT];
            job_ref(set, n, in, want);
            GL.n = 0;
            m2_growth_hook = hook;
            job j = job_run(&ROWS[R_GROW], set, n, in, 1);
            m2_growth_hook = NULL;
            job f = job_run(&ROWS[R_FIX], set, n, in, 1);
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(job_check(&j, want), 0);
            int rg = 0;
            for (int i = 0; i < GL.n && i < 64; i++) rg += !M2_INPUT(GL.g[i].id);
            CHECK(rg >= 1); /* a result grows mid-operation */
            for (int x = 0; x < M2_NMAT; x++) {
                if (!j.M[x]) continue;
                m2_fig g = mat_fig(&ROWS[R_GROW], j.M[x]), h = mat_fig(&ROWS[R_FIX], f.M[x]);
                m2_scount s = mat_store(&ROWS[R_GROW], j.M[x]), t = mat_store(&ROWS[R_FIX], f.M[x]);
                CHECK(2 * (uint64_t)g.W <= g.T);
                CHECK(g.W == h.W && s.found == t.found && s.added == t.added);
                if (M2_INPUT(x)) CHECK(s.growths >= 1);
            }
            CHECK(job_audits(&j));
            job_free(&j);
            job_free(&f);
            ref_free(want);
            for (int x = 0; x < 4; x++) free(X[x]);
        }
    }
    printf("  growth: random matrices, every job set: values kept, at most half full, W and found/new as fixed\n");
}

static void test_growth_and_widening(void) {
    /* R's whole numbers: 2, 1, 3, .., 8 (W = 8 after 7 cells), then 1000, the 9th new one: a growth (at 8 bits) and a
     * widening in one store; the growth comes first: (n) is made at the store's width, 8 bits */
    const row *R = &ROWS[R_GROW];
    sk_int A[18] = {P2(2, 1), P2(3, 1), P2(4, 1), P2(5, 1), P2(6, 1), P2(7, 1), P2(8, 1), P2(1000, 1), P2(2, 1)};
    sk_int B[18] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    const sk_int *in[2] = {A, B};
    ref_q *want[M2_NMAT];
    job_ref(SET_ADD, 3, in, want);
    GL.n = 0;
    m2_growth_hook = hook;
    job j = job_run(R, SET_ADD, 3, in, 0);
    m2_growth_hook = NULL;
    CHECK_EQ(job_check(&j, want), 0);
    int seen = 0;
    for (int i = 0; i < GL.n && i < 64; i++)
        if (GL.g[i].id == M2_R) {
            seen++;
            CHECK_EQ(GL.g[i].fields, 14); /* the 8th cell's numerator */
            CHECK_EQ(GL.g[i].bytes_new, 32); /* 32 slots at 8 bits: grown before widening */
        }
    CHECK_EQ(seen, 1);
    CHECK_EQ(mat_store(R, j.M[M2_R]).widenings, 1);
    CHECK_EQ(mat_fig(R, j.M[M2_R]).w, 16);
    CHECK_EQ(mat_fig(R, j.M[M2_R]).T, 32);
    CHECK(job_audits(&j));
    job_free(&j);
    ref_free(want);
    printf("  a growth and a widening in one store of a result (grow first)\n");
}

/* ----------------------------------------------------------------------------------------- packing */

/* (h), as the pack hook sees it just before it is freed */
static sk_int *HB;
static uint32_t HBT, HBN;
static void pack_hook(const m2_pack *p) {
    HBN++;
    free(HB);
    HBT = p->T;
    HB = malloc(sizeof(sk_int) * p->T);
    for (uint32_t s = 0; s < p->T; s++) mhvp_t_whole((const mhvp_class *)p->x, s, &HB[s]);
}

/* an input of n x n cells (m, d) in Hash-V-fixed and in Hash-V-pack: the packed matrix against the fixed one */
static void pack_case(uint32_t n, const sk_int *X, unsigned wb, unsigned wp) {
    void *f = ROWS[R_FIX].create(n, n, M2_A);
    m2_pack_hook = pack_hook;
    HBN = 0;
    void *p = ROWS[R_PACK].create(n, n, M2_A);
    for (uint32_t i = 0; i < n * n; i++) {
        CHECK_EQ(ROWS[R_FIX].enter(f, X[2 * i], X[2 * i + 1]), SK_OK);
        CHECK_EQ(ROWS[R_PACK].enter(p, X[2 * i], X[2 * i + 1]), SK_OK);
        CHECK_EQ(HBN, i + 1 == n * n); /* packed inside the last cell's enter, once */
    }
    m2_pack_hook = NULL;
    m2_fig ff = mat_fig(&ROWS[R_FIX], f), fp = mat_fig(&ROWS[R_PACK], p);
    CHECK(fp.packed && !ff.packed);
    CHECK_EQ(fp.wh_build, wb);
    CHECK_EQ(ff.wh, wb);
    CHECK_EQ(fp.wh, wp);
    CHECK_EQ(fp.W, ff.W);
    CHECK_EQ(fp.T, ff.T);
    CHECK_EQ(fp.bytes_vals, (uint64_t)fp.W * 8);
    CHECK_EQ(fp.bytes_cells, 2ull * n * n * wp / 8);
    CHECK(pack_matches_fixed(f, p));
    uint32_t T = 0;
    sk_int *tab = mat_vals(&ROWS[R_FIX], f, &T);
    CHECK(HBT == T && !memcmp(HB, tab, sizeof(sk_int) * T)); /* (h) is Hash-V-fixed's table, slot for slot */
    free(tab);
    for (uint32_t c = 0; c < n * n; c++) {
        sk_int m1 = 0, d1 = 0, m2 = 0, d2 = 0;
        ROWS[R_FIX].read(f, c, &m1, &d1);
        ROWS[R_PACK].read(p, c, &m2, &d2);
        CHECK(m1 == m2 && d1 == d2);
    }
    for (uint32_t i = 0; i < fp.nf; i++) { /* every handle < W */
        uint32_t v = 0;
        ROWS[R_PACK].t_field(p, i, &v);
        CHECK((v >> 1) < fp.W);
    }
    const char *why;
    CHECK_EQ(ROWS[R_PACK].audit(p, &why), SK_OK);
    ROWS[R_FIX].destroy(f);
    ROWS[R_PACK].destroy(p);
}

static void test_packing(void) {
    /* every pair of widths with packed <= build: (8, 8), (16, 8), (16, 16), (32, 8), (32, 16), (32, 32) */
    struct {
        uint32_t n;
        int kind; /* 0: one value; 1: a pool of 100 values; 2: every whole number distinct */
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
    printf("  packing: every packed width (8, 16, 32), every pair of widths in place, (h) as Hash-V-fixed's table\n");
}

/* ----------------------------------------------------------------------------------------- expressions */

static void test_expressions(void) {
    /* sop and pos on small random matrices: every matrix exact (the intermediates kept for inspection), every row's
     * cases equal to TC's; with the intermediates freed after their last use, none is left; in Hash-V-pack every
     * matrix packed (inputs and intermediates as operands, left and right, of addition and multiplication) */
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
                CHECK(j.M[M2_X1] && j.M[M2_X2] && j.M[M2_R]);
                CHECK(job_audits(&j));
                for (int k = 0; k < R2_NCASE; k++)
                    for (int ph = 0; ph < 2; ph++) {
                        uint64_t v = job_case(&j, ph ? SK_PH_MUL : SK_PH_ADD, k);
                        if (r == 0) tc[2 * k + ph] = v;
                        else CHECK_EQ(v, tc[2 * k + ph]);
                    }
                if (R->pack)
                    for (int x = 0; x < M2_NMAT; x++) CHECK(mat_fig(R, j.M[x]).packed);
                if (R->hash) { /* each step's own cross tests: 4 per operation on two cells */
                    uint64_t cells = (uint64_t)n * n, prod = cells * n;
                    int add_first = set == SET_POS;
                    CHECK_EQ(mat_opc(R, j.M[M2_X1]).cross, 4 * (add_first ? cells : prod));
                    CHECK_EQ(mat_opc(R, j.M[M2_R]).cross, 4 * (add_first ? prod : cells));
                }
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
    /* an intermediate of each width 8, 16, 32, 64 as an operand: pos with A all v, B zero, C the identity, D zero
     * (S = v, U = I, R = S U = S); sop with B the identity (P = A = v) and Q = C D small (R = P + Q) */
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

/* ----------------------------------------------------------------------------------------- w_h, sizes */

static void test_handle_width(void) {
    /* fixed and pack's build: w_h - 1 >= log2 T_fix; grow: w_h - 1 >= log2 T_fix + 1; T_fix the power of two >=
     * 2 n^2 + 2 (v0.2 §15.10 item 1); every table empty at creation, W = 0 */
    struct {
        uint32_t n;
        unsigned fixed, grow;
    } W[] = {{1, 8, 8}, {5, 8, 8}, {6, 8, 16}, {7, 8, 16}, {8, 16, 16}, {16, 16, 16}, {64, 16, 16},
             {90, 16, 16}, {91, 16, 32}, {127, 16, 32}, {128, 32, 32}};
    for (size_t i = 0; i < sizeof W / sizeof *W; i++)
        for (int r = 1; r < NROWS; r++) {
            void *x = ROWS[r].create(W[i].n, W[i].n, i % 2 ? M2_A : M2_R);
            m2_fig f;
            ROWS[r].figures(x, &f);
            CHECK_EQ(f.wh, ROWS[r].grow ? W[i].grow : W[i].fixed);
            uint64_t kb = 2ull * W[i].n * W[i].n + 2, tf = 1;
            while (tf < kb) tf <<= 1;
            CHECK_EQ(f.T, ROWS[r].grow ? 16u : (uint32_t)tf); /* fixed, pack: T_fix; grow: T0 */
            CHECK_EQ(f.W, 0);
            CHECK_EQ(f.w, i % 2 ? 64u : 8u);
            uint32_t nv = 0;
            sk_int *v = mat_vals(&ROWS[r], x, &nv);
            for (uint32_t s = 0; s < nv; s++) CHECK_EQ(v[s], -1); /* all empty: no slot reserved */
            free(v);
            ROWS[r].destroy(x);
        }
    printf("  w_h at the thresholds of both rules (8, 16, 32 bits); T_fix and T0; every table empty at creation\n");
}

/* ----------------------------------------------------------------------------------------- overflow */

static void test_overflow(void) {
    const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
    struct {
        int set;
        uint32_t n;
        sk_int X[4][8];
    } T[] = {
        {SET_ADD, 1, {{h62, 1}, {h62, 1}}},                       /* addition: 2^63 */
        {SET_ADD, 1, {{-h62, 1}, {-h62, 1}}},                     /* addition: -2^63, the most negative integer */
        {SET_MUL, 1, {{h32, 1}, {h32, 1}}},                       /* product: 2^64 */
        {SET_MUL, 2, {{-h62, 1, -h62, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}}}, /* add(s, p) to -2^63 */
        {SET_MUL, 1, {{h62, 3}, {2, 5}}},                         /* product, general: 2^63 */
        {SET_SOP, 1, {{h62, 1}, {1, 1}, {h62, 1}, {1, 1}}},       /* sop: P + Q = 2^63 */
        {SET_POS, 1, {{h32, 1}, {0, 1}, {h32, 1}, {0, 1}}},       /* pos: S U = 2^64 */
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
    for (int r = 0; r < NROWS; r++) { /* INT64_MIN entered: refused */
        void *x = ROWS[r].create(1, 1, M2_A);
        CHECK_EQ(ROWS[r].enter(x, INT64_MIN, 1), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, INT64_MIN), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, 0), SK_EDIVZERO);
        ROWS[r].destroy(x);
    }
    printf("  overflow: 2^63, -2^63, 2^64 and add(s, p) to -2^63 return the error in every row and job set\n");
}

/* ----------------------------------------------------------------------------------------- probing */

static void test_probing(void) {
    /* a value homed at slot 0 lands there (no slot reserved); a probe that wraps from T - 1 to slot 0 */
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        void *f = R->pack ? ROWS[R_FIX].create(1, 1, M2_A) : NULL;
        void *x = R->create(1, 1, M2_A); /* fixed, pack: T_fix = 4 (t = 2); grow: 16 (t = 4) */
        m2_fig g;
        R->figures(x, &g);
        unsigned t = 0;
        while ((1u << t) < g.T) t++;
        sk_int v[2] = {0, 0};
        int k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if (t_home(c, t) == (k ? (1u << t) - 1 : 0u) && (k == 0 || ref_gcd(c, v[0]) == 1)) v[k++] = c;
        CHECK_EQ(k, 2);
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        if (f) CHECK_EQ(ROWS[R_FIX].enter(f, v[0], v[1]), SK_OK);
        uint32_t f0 = 0, f1 = 0;
        R->t_field(x, 0, &f0);
        R->t_field(x, 1, &f1);
        if (!R->pack) {
            CHECK_EQ(f0 >> 1, 0);
            CHECK_EQ(f1 >> 1, (1u << t) - 1);
        } else {
            CHECK(pack_matches_fixed(f, x));
            CHECK(f0 >> 1 == 0 && f1 >> 1 == 1); /* positions in slot order */
        }
        sk_int m = 0, d = 0;
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        const char *why;
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
        /* wrap: two values homed at T - 1: the second probes T - 1, then 0 (2 probes) */
        x = R->create(1, 1, M2_A);
        k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if (t_home(c, t) == (1u << t) - 1 && (k == 0 || ref_gcd(c, v[0]) == 1)) v[k++] = c;
        CHECK_EQ(k, 2);
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        m2_scount s;
        R->storage(x, &s, NULL);
        CHECK_EQ(s.longest, 2);
        CHECK_EQ(s.probes, 3);
        if (!R->pack) {
            R->t_field(x, 0, &f0);
            R->t_field(x, 1, &f1);
            CHECK_EQ(f0 >> 1, (1u << t) - 1);
            CHECK_EQ(f1 >> 1, 0);
        }
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
        if (f) ROWS[R_FIX].destroy(f);
    }
    printf("  probing: a value homed at slot 0 lands there; a probe that wraps\n");
}

/* ----------------------------------------------------------------------------------------- audits */

static void test_audits(void) {
    sk_int A[8] = {P2(2, 3), P2(-4, 5), P2(0, 1), P2(7, 1)}, B[8] = {P2(1, 3), P2(4, 7), P2(5, 1), P2(-7, 3)};
    const sk_int *in[2] = {A, B};
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        const char *why;
        job j = job_run(R, SET_ADD, 2, in, 0);
        CHECK(job_audits(&j));
        m2_fig fa = mat_fig(R, j.M[M2_A]);
        uint32_t f = 0, bad = 0;
        R->t_field(j.M[M2_A], 0, &f);
        if (R->pack) {
            bad = 2 * fa.W; /* a handle of a packed matrix >= W */
        } else {
            for (uint32_t s = 0; s < fa.T; s++) { /* a numerator handle moved to an empty slot */
                sk_int v = 0;
                R->t_whole(j.M[M2_A], s, &v);
                if (v == -1) {
                    bad = 2 * s;
                    break;
                }
            }
        }
        R->t_corrupt_field(j.M[M2_A], 0, bad);
        CHECK_EQ(R->audit(j.M[M2_A], &why), SK_EAUDIT);
        R->t_corrupt_field(j.M[M2_A], 0, f);
        CHECK_EQ(R->audit(j.M[M2_A], &why), SK_OK);
        if (!R->pack) {
            R->t_field(j.M[M2_B], 1, &f);
            R->t_corrupt_slot(j.M[M2_B], f >> 1, 6); /* a denominator changed: 1/6 in lowest terms, but not found by
                                                        its probe */
            CHECK_EQ(R->audit(j.M[M2_B], &why), SK_EAUDIT);
        } else { /* two list entries the same */
            sk_int v0 = 0;
            R->t_whole(j.M[M2_R], 0, &v0);
            R->t_corrupt_slot(j.M[M2_R], 1, v0);
            CHECK_EQ(R->audit(j.M[M2_R], &why), SK_EAUDIT);
        }
        job_free(&j);
    }
    /* TC: a pair not in lowest terms */
    job j = job_run(&ROWS[R_TC], SET_ADD, 2, in, 0);
    tcm_t_corrupt((tcm_class *)j.M[M2_R], 0, 2, 4);
    const char *why;
    CHECK_EQ(tcm_audit((tcm_class *)j.M[M2_R], &why), SK_EAUDIT);
    job_free(&j);
    printf("  audits: broken fields and slots are caught, a packed handle >= W among them\n");
}

int main(void) {
    printf("m2_unit (test build)\n");
    test_cases();
    test_zero_one();
    test_widening();
    test_growth_denominator();
    test_growth_random();
    test_growth_and_widening();
    test_packing();
    test_expressions();
    test_intermediate_widths();
    test_handle_width();
    test_overflow();
    test_probing();
    test_audits();
    free(HB);
    printf("m2_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
