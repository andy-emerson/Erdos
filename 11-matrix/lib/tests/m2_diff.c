/* m2_diff.c: the four rows against each other on random jobs of every set (spec 11-matrix v0.2 §15.7 items 1-4; test
 * build): add, mul, sop and pos, random n (1 to 8) and sharing, the intermediates kept for inspection:
 *   - every cell of every matrix (the intermediates and the result) against the exact reference (128-bit integers);
 *   - call for call: every row's per-operation record (case, gcds, gcd work, exact divisions, multiplications,
 *     additions), over every step in order, equal to TC's, and every arithmetic counter per phase equal;
 *   - Hash-V-pack against Hash-V-fixed: the same W and found/new per matrix; its list the fixed table's used slots in
 *     slot order and its handles the fixed handles mapped to them;
 *   - Hash-V-grow-map against Hash-V-fixed: the same W and found/new per matrix; grow tables at most half full; fixed
 *     tables at T_fix, never resized;
 *   - every audit passes.
 *
 *   m2_diff [--seeds N] */
#include "m2t.h"

/* n^2 cells drawn from a pool of s values (numerators up to `mag`), some zeros, ones and integers */
static void draw(sk_int *X, uint32_t n, uint32_t s, sk_int mag) {
    sk_int *pool = malloc(16 * (size_t)s);
    for (uint32_t i = 0; i < s; i++) {
        uint64_t k = rnd() % 16;
        pool[2 * i] = k == 0 ? 0 : k == 1 ? 1 : (sk_int)(rnd() % (uint64_t)(2 * mag + 1)) - mag;
        pool[2 * i + 1] = k == 2 ? 1 : D720[rnd() % 30];
    }
    for (uint32_t c = 0; c < n * n; c++) {
        uint32_t i = c < s ? c : (uint32_t)(rnd() % s);
        X[2 * c] = pool[2 * i];
        X[2 * c + 1] = pool[2 * i + 1];
    }
    free(pool);
}

/* every counter of the live matrices, summed */
static void sum_counters(const job *j, sk_counters *k, r2_xcounters *x) {
    memset(k, 0, sizeof *k);
    memset(x, 0, sizeof *x);
    for (int m = 0; m < M2_NMAT; m++) {
        if (!j->M[m]) continue;
        sk_counters a;
        r2_xcounters b;
        j->R->counters(j->M[m], &a, &b);
        uint64_t *p = (uint64_t *)k, *q = (uint64_t *)&a;
        for (size_t i = 0; i < sizeof a / 8; i++) p[i] += q[i];
        p = (uint64_t *)x;
        q = (uint64_t *)&b;
        for (size_t i = 0; i < sizeof b / 8; i++) p[i] += q[i];
    }
}

int main(int argc, char **argv) {
    int seeds = 300;
    if (argc > 2 && !strcmp(argv[1], "--seeds")) seeds = atoi(argv[2]);
    long jobs = 0, ops = 0;
    for (int seed = 1; seed <= seeds; seed++) {
        rng_s = 0x9E3779B97F4A7C15ull * (uint64_t)seed + 7;
        int set = seed % 4;
        uint32_t n = 1 + (uint32_t)(rnd() % 8);
        uint32_t s = 1 + (uint32_t)(rnd() % (n * n));
        sk_int mag = set == SET_ADD ? (seed % 3 ? 50 : 30000) : set == SET_POS ? 300 : 2000;
        sk_int *X[4];
        for (int x = 0; x < 4; x++) {
            X[x] = malloc(16 * (size_t)n * n);
            draw(X[x], n, s, mag);
        }
        if (seed % 5 == 0) memcpy(X[1], X[0], 16 * (size_t)n * n); /* B's values all in A */
        const sk_int *in[4] = {X[0], X[1], X[2], X[3]};
        ref_q *want[M2_NMAT];
        job_ref(set, n, in, want);
        uint64_t n3 = (uint64_t)n * n * n, n2 = (uint64_t)n * n;
        uint64_t nops = set == SET_ADD ? n2 : set == SET_MUL ? 2 * n3 : set == SET_SOP ? 4 * n3 + n2 : 2 * n2 + 2 * n3;
        int16_t *pc[NROWS];
        uint64_t npc[NROWS];
        job J[NROWS];
        sk_counters K[NROWS];
        r2_xcounters XK[NROWS];
        for (int r = 0; r < NROWS; r++) {
            pc[r] = malloc(sizeof(int16_t) * M1_PERCALL * nops);
            /* the per-operation record of every step, in order (the entries are not operations) */
            const row *W = &ROWS[r];
            job j = {W, n, set, {NULL}, SK_OK, NULL, -1};
            for (int x = 0; x < NIN[set]; x++) {
                j.M[x] = W->create(n, n, x);
                for (uint32_t i = 0; i < n * n; i++) CHECK_EQ(W->enter(j.M[x], in[x][2 * i], in[x][2 * i + 1]), SK_OK);
            }
            m1_percall = pc[r];
            m1_percall_n = 0;
            m1_percall_cap = nops;
            for (int st = 0; st < NSTEP[set]; st++) {
                const step *t = &STEPS[set][st];
                j.M[t->c] = W->create(n, n, t->c);
                CHECK_EQ(W->op[t->op](j.M[t->a], j.M[t->b], j.M[t->c]), SK_OK);
            }
            m1_percall = NULL;
            npc[r] = m1_percall_n;
            CHECK_EQ(job_check(&j, want), 0);
            CHECK(job_audits(&j));
            sum_counters(&j, &K[r], &XK[r]);
            J[r] = j;
        }
        for (int r = 1; r < NROWS; r++) {
            CHECK_EQ(npc[r], nops);
            CHECK(!memcmp(pc[r], pc[0], sizeof(int16_t) * M1_PERCALL * nops)); /* call for call with TC */
            CHECK(!memcmp(&K[r], &K[0], sizeof K[0]));
            CHECK(!memcmp(&XK[r], &XK[0], sizeof XK[0])); /* V reads nothing as K-Q3 reads: every counter equal */
            for (int m = 0; m < M2_NMAT; m++) {
                if (!J[r].M[m]) continue;
                m2_fig f = mat_fig(&ROWS[r], J[r].M[m]), g = mat_fig(&ROWS[R_FIX], J[R_FIX].M[m]);
                m2_scount a = mat_store(&ROWS[r], J[r].M[m]), b = mat_store(&ROWS[R_FIX], J[R_FIX].M[m]);
                CHECK(f.W == g.W && a.found == b.found && a.added == b.added); /* the same W and found/new */
                if (ROWS[r].grow) {
                    CHECK(2ull * f.W <= f.T);
                } else {
                    uint64_t kb = 2ull * n * n + 2, tf = 1;
                    while (tf < kb) tf <<= 1;
                    CHECK_EQ(f.T, (uint32_t)tf);
                    CHECK_EQ(a.growths, 0);
                }
                if (ROWS[r].pack) CHECK(pack_matches_fixed(J[R_FIX].M[m], J[r].M[m]));
            }
        }
        for (int r = 0; r < NROWS; r++) {
            job_free(&J[r]);
            free(pc[r]);
        }
        ref_free(want);
        for (int x = 0; x < 4; x++) free(X[x]);
        jobs++;
        ops += (long)nops;
    }
    printf("m2_diff: %d seeds, %ld jobs, %ld operations; %d checks, %d failures\n", seeds, jobs, ops, checks, failures);
    return failures != 0;
}
