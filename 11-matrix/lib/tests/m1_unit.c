/* m1_unit.c: the unit tests of the matrix round (spec 11-matrix v0.1 §10, "Tests"; test build), for both operations:
 *   - every case of addition with operands from two matrices, each of its four cross tests matching and not matching,
 *     by V and by M (a whole number of B that A lacks included); every case of multiplication likewise, and its
 *     add(s, p) on temporaries (every addition case on two local pairs);
 *   - each widening of C (8 -> 16 -> 32 -> 64), every slot and handle unchanged after it; a growth and a widening in
 *     one store of C;
 *   - each growth from T0 by both methods, every cell's value unchanged, both methods giving the same handles, 0 and 1
 *     still at 0 and 1; a growth triggered by a denominator's store in A, in B and in C, by both methods, the same
 *     cell's numerator handle rewritten and its value unchanged; a growth of C mid-operation, in both operations; tau
 *     built from grown A and B;
 *   - w_h at the dimension thresholds of both rules (8, 16, 32 bits);
 *   - a 64-bit overflow and a result of -2^63 return the error in every row (the timing and counting builds:
 *     m1_alloc); INT64_MIN entered is refused;
 *   - values whose home slot is 0 or 1, and a probe that wraps;
 *   - tau with no match, all matching, and 0 and 1;
 *   - the audits catch a broken invariant.
 * The six rows against each other on random matrices are m1_diff.c. */
#include "m1t.h"

#define P2(m, d) (sk_int)(m), (sk_int)(d)

/* the cross tests of two values (spec §6): b = d, a = c, a = d, c = b on magnitudes */
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

/* one operation on 1x1 matrices (multiplication: the product, then add(0/1, p), case zero_x or zero_y... zero_x) */
static void one_case(const tcase *t, int op) {
    sk_int A[2] = {t->xm, t->xd}, B[2] = {t->ym, t->yd};
    ref_q *want = job_ref(op, 1, A, B);
    for (int r = 0; r < NROWS; r++) {
        const row *R = &ROWS[r];
        job j = job_run(R, op, 1, A, B, 1);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        CHECK_EQ(job_case(&j, op ? SK_PH_MUL : SK_PH_ADD, t->kase), 1);
        if (op) CHECK_EQ(job_case(&j, SK_PH_ADD, R2_ADD_ZERO_X), 1); /* the first addition of a cell (§6.2) */
        CHECK(job_audits(&j));
        if (R->hash) {
            m1_ocount o = job_opc(&j);
            CHECK_EQ(o.cross, 4);
            CHECK_EQ(o.cross_match, matches(t->xm, t->xd, t->ym, t->yd));
            if (R->m) {
                CHECK_EQ(o.cross_reads, 0);
                const char *why;
                CHECK_EQ(R->audit_tau(j.M[0], j.M[1], j.M[2], &why), SK_OK);
                /* B's whole numbers other than 0 and 1: found in A, or none */
                uint64_t lack = 0;
                sk_int bw[2] = {t->ym < 0 ? -t->ym : t->ym, t->yd};
                for (int k = 0; k < 2; k++) {
                    if (bw[k] <= 1 || (k == 1 && bw[1] == bw[0])) continue;
                    lack += !(bw[k] == (t->xm < 0 ? -t->xm : t->xm) || bw[k] == t->xd);
                }
                CHECK_EQ(o.tau_none, lack);
            } else {
                sk_int a = t->xm < 0 ? -t->xm : t->xm, c = t->ym < 0 ? -t->ym : t->ym;
                CHECK_EQ(o.cross_reads, (a == t->xd ? 1 : 2) + (c == t->yd ? 1 : 2));
                CHECK_EQ(o.tau_found + o.tau_none, 0);
            }
        }
        R->tau_free(j.M[2]);
        job_free(&j);
    }
    free(want);
}

/* every addition case on two local pairs: C = A B with B all 1, so that each cell's sum is u + v (§6.2 item 3) */
static void temp_case(const tcase *t) {
    sk_int A[8] = {t->xm, t->xd, t->ym, t->yd, t->xm, t->xd, t->ym, t->yd}, B[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    ref_q *want = job_ref(1, 2, A, B);
    for (int r = 0; r < NROWS; r++) {
        job j = job_run(&ROWS[r], 1, 2, A, B, 0);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        /* each of the 4 cells: add(0/1, x) then add(x, y) */
        uint64_t extra = t->kase == R2_ADD_ZERO_X ? 4 : 0;
        CHECK_EQ(job_case(&j, SK_PH_ADD, t->kase), 4 + extra);
        if (ROWS[r].hash) CHECK_EQ(job_opc(&j).cross, 4 * 8); /* the 8 products only: add(s, p) makes no cross test */
        CHECK(job_audits(&j));
        job_free(&j);
    }
    free(want);
}

static void test_cases(void) {
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) one_case(&ADD_CASES[i], 0);
    for (size_t i = 0; i < sizeof MUL_CASES / sizeof *MUL_CASES; i++) one_case(&MUL_CASES[i], 1);
    for (size_t i = 0; i < sizeof ADD_CASES / sizeof *ADD_CASES; i++) temp_case(&ADD_CASES[i]);
    printf("  cases: %zu addition, %zu multiplication, every addition case on temporaries; V and M\n",
           sizeof ADD_CASES / sizeof *ADD_CASES, sizeof MUL_CASES / sizeof *MUL_CASES);
}

/* ----------------------------------------------------------------------------------------- widening */

/* the slots a table of 2^t slots gives the values vs (each distinct, none 0 or 1), stored in order: the test's own
 * linear probing, independent of mh.c */
static void sim_slots(unsigned t, const sk_int *vs, int n, uint32_t *slot) {
    uint32_t T = 1u << t;
    sk_int *tab = malloc(sizeof(sk_int) * T);
    for (uint32_t s = 0; s < T; s++) tab[s] = -1;
    tab[0] = 0;
    tab[1] = 1;
    for (int i = 0; i < n; i++) {
        uint32_t s = t_home(vs[i], t);
        while (tab[s] != -1 && tab[s] != vs[i]) s = (s + 1) & (T - 1);
        tab[s] = vs[i];
        slot[i] = s;
    }
    free(tab);
}

static void test_widening(void) {
    /* C = A + 0: C's whole numbers in order 100 (8 bits), 1000 (16), 100000 (32), 2^40 (64); denominators 1 */
    const sk_int big = (sk_int)1 << 40;
    sk_int A[8] = {P2(100, 1), P2(-1000, 1), P2(100000, 1), P2(big, 1)}, B[8] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    ref_q *want = job_ref(0, 2, A, B);
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        job j = job_run(R, 0, 2, A, B, 0);
        CHECK_EQ(j.st, SK_OK);
        CHECK_EQ(job_check(&j, want), 0);
        m1_scount s = job_store(&j, M1_C);
        m1_fig f = job_fig(&j, M1_C);
        CHECK_EQ(s.widenings, 3);
        CHECK_EQ(s.copied, 3 * f.T);
        CHECK_EQ(f.w, 64);
        /* every slot where the test's own probing put it, and every handle naming it: nothing moved */
        sk_int vs[4] = {100, 1000, 100000, big};
        uint32_t slot[4];
        unsigned t = 0;
        while ((1u << t) < f.T) t++;
        sim_slots(t, vs, 4, slot);
        for (int i = 0; i < 4; i++) {
            sk_int v = 0;
            uint32_t fl = 0;
            CHECK_EQ(R->t_whole(j.M[2], slot[i], &v), SK_OK);
            CHECK_EQ(v, vs[i]);
            CHECK_EQ(R->t_field(j.M[2], 2 * (uint32_t)i, &fl), SK_OK);
            CHECK_EQ(fl, 2 * slot[i] + (i == 1));
        }
        CHECK(job_audits(&j));
        job_free(&j);
    }
    /* each step on its own: 8 -> 16 (one widening), 8 -> 32 straight, 8 -> 64 straight */
    sk_int steps[3] = {1000, 100000, big};
    unsigned wants[3] = {16, 32, 64};
    for (int k = 0; k < 3; k++) {
        sk_int A1[2] = {steps[k], 1}, B1[2] = {0, 1};
        for (int r = 1; r < NROWS; r++) {
            job j = job_run(&ROWS[r], 0, 1, A1, B1, 0);
            CHECK_EQ(job_store(&j, M1_C).widenings, 1);
            CHECK_EQ(job_fig(&j, M1_C).w, wants[k]);
            CHECK(job_audits(&j));
            job_free(&j);
        }
    }
    /* A and B never widen: 64-bit from creation */
    for (int r = 1; r < NROWS; r++) {
        job j = job_run(&ROWS[r], 0, 2, A, B, 0);
        CHECK_EQ(job_fig(&j, M1_A).w, 64);
        CHECK_EQ(job_fig(&j, M1_B).w, 64);
        CHECK_EQ(job_store(&j, M1_A).widenings + job_store(&j, M1_B).widenings, 0);
        job_free(&j);
    }
    free(want);
    printf("  widening: 8 -> 16 -> 32 -> 64 in one job, each step alone; slots and handles unchanged\n");
}

/* ----------------------------------------------------------------------------------------- growth */

typedef struct glog {
    int n;
    m1_growth g[64];
} glog;
static glog GL;
static void hook(const m1_growth *g) {
    if (GL.n < 64) GL.g[GL.n] = *g;
    GL.n++;
}

/* the handle fields of one matrix */
static uint32_t *fields_of(const job *j, int r, uint32_t *nf) {
    m1_fig f = job_fig(j, r);
    uint32_t *v = malloc(sizeof(uint32_t) * (f.nf + 1));
    for (uint32_t i = 0; i < f.nf; i++) j->R->t_field(j->M[r], i, &v[i]);
    *nf = f.nf;
    return v;
}

static void test_growth_denominator(void) {
    /* in A: the cells 2/3, 4/5, 6/5, 7/9: W = 8 when 9, a denominator, is stored: a growth with the fields of 3 cells
     * and cell 3's numerator written (7 fields) */
    sk_int V[8] = {P2(2, 3), P2(4, 5), P2(6, 5), P2(-7, 9)}, Z[8] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    for (int where = 0; where < 3; where++) {
        /* A: V + Z; B: Z + V; C: (V + Z) stored in C (A's own growth comes first, then C's) */
        const sk_int *A = where == 1 ? Z : V, *B = where == 1 ? V : Z;
        int role = where;
        ref_q *want = job_ref(0, 2, A, B);
        uint32_t *fl[2] = {NULL, NULL}, nfl[2] = {0, 0};
        int k = 0;
        for (int r = 1; r < NROWS; r++) {
            const row *R = &ROWS[r];
            if (!R->grow) continue;
            GL.n = 0;
            m1_growth_hook = hook;
            job j = job_run(R, 0, 2, A, B, 0);
            m1_growth_hook = NULL;
            CHECK_EQ(j.st, SK_OK);
            CHECK_EQ(job_check(&j, want), 0);
            int found = 0;
            for (int i = 0; i < GL.n && i < 64; i++)
                if (GL.g[i].role == role) {
                    found++;
                    CHECK_EQ(GL.g[i].fields, 7); /* cell 3's numerator written, its denominator not */
                    CHECK_EQ(GL.g[i].W, 8);
                    CHECK_EQ(GL.g[i].T_old, 16);
                    CHECK_EQ(GL.g[i].rewritten, 7);
                    CHECK_EQ(GL.g[i].bytes_map, R->map ? 16u * job_fig(&j, role).wh / 8 : 0u);
                }
            CHECK_EQ(found, 1);
            /* cell 3 of the matrix: its value, and its numerator's handle names 7 in the grown table */
            sk_int m = 0, d = 0, v = 0;
            uint32_t f6 = 0;
            CHECK_EQ(R->read(j.M[role], 3, &m, &d), SK_OK);
            CHECK(m == -7 && d == 9);
            R->t_field(j.M[role], 6, &f6);
            R->t_whole(j.M[role], f6 >> 1, &v);
            CHECK(v == 7 && (f6 & 1));
            CHECK(job_audits(&j));
            if (!R->m && k < 2) fl[k] = fields_of(&j, role, &nfl[k]), k++; /* lookup and map */
            job_free(&j);
        }
        CHECK(fl[0] && fl[1] && nfl[0] == nfl[1] && !memcmp(fl[0], fl[1], sizeof(uint32_t) * nfl[0]));
        free(fl[0]);
        free(fl[1]);
        free(want);
    }
    printf("  growth triggered by a denominator's store: in A, in B, in C; lookup and map give the same handles\n");
}

static void test_growth_random(void) {
    /* every growth from T0 by both methods, on random matrices, both operations: values unchanged (the test build's
     * growth audit), the same handles by both methods, 0 and 1 at 0 and 1; C grows mid-operation; tau from grown A, B */
    for (int seed = 1; seed <= 12; seed++) {
        for (int op = 0; op < 2; op++) {
            uint32_t n = 3 + (uint32_t)seed % 5;
            sk_int *A = malloc(16 * n * n), *B = malloc(16 * n * n);
            rng_s = 1000 + (uint64_t)seed;
            static const sk_int D720[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 48, 60,
                                          72, 80, 90, 120, 144, 180, 240, 360, 720};
            for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                A[i] = (sk_int)(rnd() % 2001) - 1000;
                A[i + 1] = D720[rnd() % 30];
                B[i] = (sk_int)(rnd() % 2001) - 1000;
                B[i + 1] = D720[rnd() % 30];
            }
            ref_q *want = job_ref(op, n, A, B);
            uint32_t *f[3][3] = {{0}}, nf[3][3] = {{0}};
            int k = 0;
            for (int r = 1; r < NROWS; r++) {
                const row *R = &ROWS[r];
                if (!R->grow) continue;
                GL.n = 0;
                m1_growth_hook = hook;
                job j = job_run(R, op, n, A, B, 1);
                m1_growth_hook = NULL;
                CHECK_EQ(j.st, SK_OK);
                CHECK_EQ(job_check(&j, want), 0);
                int cg = 0;
                for (int i = 0; i < GL.n && i < 64; i++) cg += GL.g[i].role == M1_C;
                CHECK(cg >= 1); /* C grows mid-operation */
                CHECK(job_store(&j, M1_A).growths >= 1 && job_store(&j, M1_B).growths >= 1);
                for (int x = 0; x < 3; x++) {
                    sk_int v0 = -1, v1 = -1;
                    R->t_whole(j.M[x], 0, &v0);
                    R->t_whole(j.M[x], 1, &v1);
                    CHECK(v0 == 0 && v1 == 1);
                    m1_fig g = job_fig(&j, x);
                    CHECK(2 * (uint64_t)g.W <= g.T);
                    if (k < 3) f[k][x] = fields_of(&j, x, &nf[k][x]);
                }
                if (R->m) { /* tau from grown A and B */
                    const char *why;
                    CHECK_EQ(R->audit_tau(j.M[0], j.M[1], j.M[2], &why), SK_OK);
                    CHECK_EQ(R->tau_bytes(j.M[2]), (uint64_t)job_fig(&j, M1_B).T * job_fig(&j, M1_A).wh / 8);
                }
                R->tau_free(j.M[2]);
                if (R->map) CHECK_EQ(job_store(&j, M1_A).wprobes + job_store(&j, M1_C).wprobes, 0);
                else CHECK(job_store(&j, M1_A).wprobes >= job_store(&j, M1_A).rewritten / 2);
                CHECK(job_audits(&j));
                k++;
                job_free(&j);
            }
            for (int x = 0; x < 3; x++) {
                for (int a = 1; a < 3; a++)
                    CHECK(nf[a][x] == nf[0][x] && !memcmp(f[a][x], f[0][x], sizeof(uint32_t) * nf[0][x]));
                for (int a = 0; a < 3; a++) free(f[a][x]);
            }
            free(want);
            free(A);
            free(B);
        }
    }
    printf("  growth: random matrices, both operations, both methods and M-grow: same handles, values kept, tau\n");
}

static void test_growth_and_widening(void) {
    /* C's whole numbers: 2..7 (6 new: W = 8), then 1000, the 7th new one: a growth (at 8 bits) and a widening in one
     * store; the growth comes first: (n) is made at the store's width, 8 bits */
    sk_int A[18] = {P2(2, 1), P2(3, 1), P2(4, 1), P2(5, 1), P2(6, 1), P2(7, 1), P2(1000, 1), P2(2, 1), P2(3, 1)};
    sk_int B[18] = {P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1), P2(0, 1)};
    ref_q *want = job_ref(0, 3, A, B);
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        if (!R->grow) continue;
        GL.n = 0;
        m1_growth_hook = hook;
        job j = job_run(R, 0, 3, A, B, 0);
        m1_growth_hook = NULL;
        CHECK_EQ(job_check(&j, want), 0);
        int seen = 0;
        for (int i = 0; i < GL.n && i < 64; i++)
            if (GL.g[i].role == M1_C) {
                seen++;
                CHECK_EQ(GL.g[i].fields, 12); /* the 7th cell's numerator */
                CHECK_EQ(GL.g[i].bytes_new, 32); /* 32 slots at 8 bits: grown before widening */
            }
        CHECK_EQ(seen, 1);
        CHECK_EQ(job_store(&j, M1_C).widenings, 1);
        CHECK_EQ(job_fig(&j, M1_C).w, 16);
        CHECK_EQ(job_fig(&j, M1_C).T, 32);
        CHECK(job_audits(&j));
        job_free(&j);
    }
    free(want);
    printf("  a growth and a widening in one store of C (grow first)\n");
}

/* ----------------------------------------------------------------------------------------- w_h, sizes */

static void test_handle_width(void) {
    /* fixed: w_h - 1 >= log2 T_fix; grow: w_h - 1 >= log2 T_fix + 1; T_fix the power of two >= 2 n^2 + 2 */
    struct {
        uint32_t n;
        unsigned fixed, grow;
    } W[] = {{1, 8, 8}, {5, 8, 8}, {6, 8, 16}, {7, 8, 16}, {8, 16, 16}, {16, 16, 16}, {64, 16, 16},
             {90, 16, 16}, {91, 16, 32}, {127, 16, 32}, {128, 32, 32}};
    for (size_t i = 0; i < sizeof W / sizeof *W; i++)
        for (int r = 1; r < NROWS; r++) {
            void *x = ROWS[r].create(W[i].n, W[i].n, M1_A);
            m1_fig f;
            ROWS[r].figures(x, &f);
            CHECK_EQ(f.wh, ROWS[r].grow ? W[i].grow : W[i].fixed);
            uint64_t kb = 2ull * W[i].n * W[i].n + 2, tf = 1;
            while (tf < kb) tf <<= 1;
            CHECK_EQ(f.T, ROWS[r].grow ? 16u : (uint32_t)tf); /* fixed: T_fix; grow: T0 */
            CHECK_EQ(f.W, 2);
            ROWS[r].destroy(x);
        }
    printf("  w_h at the thresholds of both rules (8, 16, 32 bits); T_fix and T0\n");
}

/* ----------------------------------------------------------------------------------------- overflow */

static void test_overflow(void) {
    const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
    struct {
        int op;
        uint32_t n;
        sk_int A[8], B[8];
    } T[] = {
        {0, 1, {h62, 1}, {h62, 1}},                        /* addition: 2^63 */
        {0, 1, {-h62, 1}, {-h62, 1}},                      /* addition: -2^63, the most negative integer */
        {1, 1, {h32, 1}, {h32, 1}},                        /* product: 2^64 */
        {1, 2, {-h62, 1, -h62, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}}, /* add(s, p): -2^62 + -2^62 = -2^63 */
        {1, 1, {h62, 3}, {2, 5}},                          /* product, general: 2^63 */
    };
    for (size_t i = 0; i < sizeof T / sizeof *T; i++)
        for (int r = 0; r < NROWS; r++) {
            job j = job_run(&ROWS[r], T[i].op, T[i].n, T[i].A, T[i].B, 0);
            CHECK_EQ(j.st, SK_EOVERFLOW);
            sk_overflow o;
            CHECK_EQ(ROWS[r].overflow(j.M[2], &o), SK_OK);
            CHECK(o.count == 1 && o.step);
            job_free(&j);
        }
    for (int r = 0; r < NROWS; r++) { /* INT64_MIN entered: refused */
        void *x = ROWS[r].create(1, 1, M1_A);
        CHECK_EQ(ROWS[r].enter(x, INT64_MIN, 1), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, INT64_MIN), SK_EINVAL);
        CHECK_EQ(ROWS[r].enter(x, 1, 0), SK_EDIVZERO);
        ROWS[r].destroy(x);
    }
    printf("  overflow: 2^63, -2^63, 2^64 and add(s, p) to -2^63 return the error in every row\n");
}

/* ----------------------------------------------------------------------------------------- probing */

static void test_probing(void) {
    /* values whose home slot is 0 or 1 (at 2^t slots) are placed past slots 0 and 1; a probe that wraps from T - 1 */
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        void *x = R->create(1, 1, M1_A); /* fixed: T_fix = 4 (t = 2); grow: 16 (t = 4) */
        m1_fig f;
        R->figures(x, &f);
        unsigned t = 0;
        while ((1u << t) < f.T) t++;
        /* two coprime values, both homed at slot 0 (or 1), as one cell */
        sk_int v[2] = {0, 0};
        int k = 0;
        for (sk_int c = 2; k < 2 && c < 1000000; c++)
            if (t_home(c, t) <= 1 && (k == 0 || ref_gcd(c, v[0]) == 1)) v[k++] = c;
        CHECK_EQ(k, 2);
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        uint32_t f0 = 0, f1 = 0;
        R->t_field(x, 0, &f0);
        R->t_field(x, 1, &f1);
        CHECK((f0 >> 1) >= 2 && (f1 >> 1) >= 2 && (f0 >> 1) != (f1 >> 1));
        sk_int m = 0, d = 0;
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        const char *why;
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
        /* wrap: two values homed at T - 1: the second probes T - 1, 0, 1, 2 (4 probes) */
        x = R->create(1, 1, M1_A);
        k = 0;
        for (sk_int c = 2; k < 2 && c < 10000000; c++)
            if (t_home(c, t) == (1u << t) - 1 && (k == 0 || ref_gcd(c, v[0]) == 1)) v[k++] = c;
        CHECK_EQ(k, 2);
        CHECK_EQ(R->enter(x, v[0], v[1]), SK_OK);
        R->t_field(x, 0, &f0);
        R->t_field(x, 1, &f1);
        CHECK_EQ(f0 >> 1, (1u << t) - 1);
        CHECK_EQ(f1 >> 1, 2);
        m1_scount s;
        R->storage(x, &s, NULL);
        CHECK_EQ(s.longest, 4);
        CHECK_EQ(s.probes, 5);
        R->read(x, 0, &m, &d);
        CHECK(m == v[0] && d == v[1]);
        CHECK_EQ(R->audit(x, &why), SK_OK);
        R->destroy(x);
    }
    printf("  probing: home slots 0 and 1 passed over; a probe that wraps\n");
}

/* ----------------------------------------------------------------------------------------- tau */

static void test_tau(void) {
    sk_int A[8] = {P2(2, 3), P2(-4, 5), P2(0, 1), P2(7, 1)};
    sk_int same[8] = {P2(7, 1), P2(2, 3), P2(-4, 5), P2(0, 1)};   /* every whole number of B in A */
    sk_int other[8] = {P2(11, 13), P2(17, 19), P2(-23, 29), P2(31, 37)}; /* none (but 0 and 1) */
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        if (!R->m) continue;
        for (int k = 0; k < 2; k++) {
            const sk_int *B = k ? other : same;
            job j = job_run(R, 0, 2, A, B, 1);
            CHECK_EQ(j.st, SK_OK);
            m1_ocount o = job_opc(&j);
            if (k) CHECK(o.tau_found == 0 && o.tau_none == 8);
            else CHECK(o.tau_none == 0 && o.tau_found == 5);
            uint32_t t0 = 9, t1 = 9;
            CHECK_EQ(R->t_tau(j.M[2], 0, &t0), SK_OK);
            CHECK_EQ(R->t_tau(j.M[2], 1, &t1), SK_OK);
            CHECK(t0 == 0 && t1 == 1);
            const char *why;
            CHECK_EQ(R->audit_tau(j.M[0], j.M[1], j.M[2], &why), SK_OK);
            /* an empty slot of B holds none: 2^wh - 1 */
            m1_fig fb = job_fig(&j, M1_B), fa = job_fig(&j, M1_A);
            for (uint32_t s = 0; s < fb.T; s++) {
                sk_int v = 0;
                uint32_t e = 0;
                R->t_whole(j.M[1], s, &v);
                R->t_tau(j.M[2], s, &e);
                if (v == -1) CHECK_EQ(e, (uint32_t)((1ull << fa.wh) - 1));
            }
            /* a broken entry is caught */
            R->tau_free(j.M[2]);
            job_free(&j);
        }
    }
    printf("  tau: all matching, no match, 0 and 1, empty slots\n");
}

/* ----------------------------------------------------------------------------------------- audits */

static void test_audits(void) {
    sk_int A[8] = {P2(2, 3), P2(-4, 5), P2(0, 1), P2(7, 1)}, B[8] = {P2(1, 3), P2(4, 7), P2(5, 1), P2(-7, 3)};
    for (int r = 1; r < NROWS; r++) {
        const row *R = &ROWS[r];
        const char *why;
        job j = job_run(R, 0, 2, A, B, 0);
        CHECK(job_audits(&j));
        uint32_t f = 0;
        R->t_field(j.M[0], 0, &f);
        R->t_corrupt_field(j.M[0], 0, f ^ 2); /* a numerator handle moved to another slot */
        CHECK_EQ(R->audit(j.M[0], &why), SK_EAUDIT);
        R->t_corrupt_field(j.M[0], 0, f);
        CHECK_EQ(R->audit(j.M[0], &why), SK_OK);
        R->t_field(j.M[1], 1, &f);
        R->t_corrupt_slot(j.M[1], f >> 1, 6); /* a denominator changed: 1/6 is in lowest terms, but not found by
                                                 its probe */
        CHECK_EQ(R->audit(j.M[1], &why), SK_EAUDIT);
        R->t_corrupt_slot(j.M[2], 1, 5); /* slot 1 no longer holds 1 */
        CHECK_EQ(R->audit(j.M[2], &why), SK_EAUDIT);
        job_free(&j);
    }
    /* TC: a pair not in lowest terms */
    job j = job_run(&ROWS[0], 0, 2, A, B, 0);
    tcm_t_corrupt((tcm_class *)j.M[2], 0, 2, 4);
    const char *why;
    CHECK_EQ(tcm_audit((tcm_class *)j.M[2], &why), SK_EAUDIT);
    job_free(&j);
    printf("  audits: broken fields and slots are caught\n");
}

int main(void) {
    printf("m1_unit (test build)\n");
    test_cases();
    test_widening();
    test_growth_denominator();
    test_growth_random();
    test_growth_and_widening();
    test_handle_width();
    test_overflow();
    test_probing();
    test_tau();
    test_audits();
    printf("m1_unit: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}
