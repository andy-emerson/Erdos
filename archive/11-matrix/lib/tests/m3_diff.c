/* m3_diff.c: the four rows against each other on random jobs of every set (spec 11-matrix v0.3 §16.9 items 1-5; test
 * build): add, mul, sop and pos, random n (1 to 8) and sharing, the intermediates kept for inspection:
 *   - every cell of every matrix (the intermediates and the result) against the exact reference (128-bit integers):
 *     the rows agree on every result value;
 *   - call for call (§16.9 item 3): Hash-V-pack's per-operation record (case, gcds, gcd work, exact divisions,
 *     multiplications, additions) over every step equal to TC's, and every counter equal; the factored rows' case,
 *     exact divisions, multiplications and additions equal to TC's per operation, and every counter equal except the
 *     gcds and gcd work of the operations (reported, not compared); Pack-primes makes no Euclid gcd in an operation;
 *   - each factored row against Hash-V-pack (§16.9 item 4): the same list (l), handle fields, W and found/new per
 *     matrix; Pack-primes against Pack-coprime: their (f) lists multiply back to the same values;
 *   - every (f) list multiplies back to its number; Pack-primes' factors are prime (small factors checked by trial);
 *     Pack-coprime's base pairwise coprime by all pairs; every obtained gcd was checked against Euclid inside the rows
 *     (the test build dies otherwise) and the count of such checks is positive on jobs with gcd-bearing cases;
 *   - every audit passes; no read of 0's list.
 *
 *   m3_diff [--seeds N] */
#include "m3t.h"

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

/* the counters with the operations' gcds, gcd work and gcd histogram cleared (the factored rows obtain their gcds
 * otherwise: reported, not compared, §16.9 item 3) */
static void clear_op_gcds(sk_counters *k) {
    for (int ph = SK_PH_ADD; ph < SK_NPHASE; ph++)
        for (int p = 0; p < SK_NPART; p++) {
            k->c[ph][p].gcd = 0;
            k->c[ph][p].gcd_work = 0;
            memset(k->c[ph][p].gcd_hist, 0, sizeof k->c[ph][p].gcd_hist);
        }
}

static uint64_t op_gcds(const sk_counters *k) {
    uint64_t t = 0;
    for (int ph = SK_PH_ADD; ph < SK_NPHASE; ph++)
        for (int p = 0; p < SK_NPART; p++) t += k->c[ph][p].gcd;
    return t;
}

/* every factorization of a factored matrix multiplies back to its number; Pack-primes' small factors are prime */
static int fac_ok(const row *R, const void *m) {
    uint32_t W = 0;
    sk_int *v = mat_vals(R, m, &W);
    int ok = 1;
    for (uint32_t k = 0; k < W && ok; k++) {
        uint32_t ne = 0;
        i128 p = fac_product(R, m, k, &ne);
        if (v[k] < 2) ok = ne == 0;
        else ok = p == (i128)v[k];
        if (ok && !R->coprime)
            for (uint32_t j = 0; j < ne && ok; j++) {
                sk_int q;
                unsigned e;
                R->t_fac(m, k, j, &q, &e);
                if (q < 1000000) ok = is_prime_small(q);
            }
    }
    free(v);
    return ok;
}

int main(int argc, char **argv) {
    int seeds = 300;
    if (argc > 2 && !strcmp(argv[1], "--seeds")) seeds = atoi(argv[2]);
    long jobs = 0, ops = 0;
    uint64_t gchecked = 0, merges = 0, lsplits = 0, rsplits = 0;
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
            if (!ROWS[r].fac) {
                CHECK(!memcmp(pc[r], pc[0], sizeof(int16_t) * M1_PERCALL * nops)); /* call for call with TC */
                CHECK(!memcmp(&K[r], &K[0], sizeof K[0]));
            } else {
                int same = 1;
                for (uint64_t i = 0; i < nops && same; i++) {
                    const int16_t *a = pc[r] + M1_PERCALL * i, *b = pc[0] + M1_PERCALL * i;
                    same = a[0] == b[0] && a[3] == b[3] && a[4] == b[4] && a[5] == b[5]; /* case, div, mul, add */
                    if (!ROWS[r].coprime) same = same && a[1] == 0 && a[2] == 0;           /* no Euclid gcd */
                }
                CHECK(same);
                sk_counters a = K[r], b = K[0];
                if (!ROWS[r].coprime) CHECK_EQ(op_gcds(&a), 0);
                clear_op_gcds(&a);
                clear_op_gcds(&b);
                CHECK(!memcmp(&a, &b, sizeof a)); /* entry's gcds and every other core counter equal */
            }
            CHECK(!memcmp(&XK[r], &XK[0], sizeof XK[0])); /* cases, comparisons, K-n, reads: every counter equal */
            for (int m = 0; m < M2_NMAT; m++) {
                if (!J[r].M[m]) continue;
                m2_fig f = mat_fig(&ROWS[r], J[r].M[m]), g = mat_fig(&ROWS[R_PACK], J[R_PACK].M[m]);
                m2_scount a = mat_store(&ROWS[r], J[r].M[m]), b = mat_store(&ROWS[R_PACK], J[R_PACK].M[m]);
                CHECK(f.W == g.W && a.found == b.found && a.added == b.added && f.wh == g.wh && f.w == g.w);
                if (ROWS[r].fac) {
                    CHECK(same_as_pack(&ROWS[r], J[R_PACK].M[m], J[r].M[m]));
                    CHECK(fac_ok(&ROWS[r], J[r].M[m]));
                    m3_acount ac = mat_ac(&ROWS[r], J[r].M[m]);
                    CHECK_EQ(ac.zero_reads, 0);
                    gchecked += ac.gchecked;
                    merges += ac.merges;
                    lsplits += ac.lsplits;
                    rsplits += ac.rc_splits;
                }
            }
        }
        /* Pack-primes against Pack-coprime: their lists multiply back to the same values (both are (l)'s) */
        for (int m = 0; m < M2_NMAT; m++) {
            if (!J[R_PRIMES].M[m]) continue;
            uint32_t W = mat_fig(&ROWS[R_PRIMES], J[R_PRIMES].M[m]).W;
            for (uint32_t k = 0; k < W; k++)
                CHECK(fac_product(&ROWS[R_PRIMES], J[R_PRIMES].M[m], k, NULL) ==
                      fac_product(&ROWS[R_COPRIME], J[R_COPRIME].M[m], k, NULL));
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
    CHECK(gchecked > 0);
    printf("m3_diff: %d seeds, %ld jobs, %ld operations; obtained gcds checked against Euclid %llu, merges %llu, "
           "reconciliation splits %llu, leftover splits %llu; %d checks, %d failures\n",
           seeds, jobs, ops, (unsigned long long)gchecked, (unsigned long long)merges, (unsigned long long)rsplits,
           (unsigned long long)lsplits, checks, failures);
    return failures != 0;
}
