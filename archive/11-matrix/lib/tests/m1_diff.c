/* m1_diff.c: the six rows against each other on random matrices (spec 11-matrix v0.1 §10 items 1, 3-6; test build),
 * both operations, random n (1 to 9) and sharing:
 *   - every cell of C against the exact reference (128-bit integers);
 *   - call for call: every row's per-operation record (case, gcds, gcd work, exact divisions, multiplications,
 *     additions) equal to TC's (§10 item 3), and every arithmetic counter per phase equal;
 *   - V against M within a sizing: the same store counts (found, new) and the same cells' values (§10 item 4);
 *   - grow against fixed: the same W and found/new per store (§10 item 5); fixed tables at T_fix, grow tables at most
 *     half full;
 *   - map against lookup: the same tables slot by slot, the same handle fields, the same everything except the rewrite
 *     probes (§10 item 6);
 *   - every audit passes, tau's included.
 *
 *   m1_diff [--seeds N] */
#include "m1t.h"

static const sk_int D720[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 48, 60, 72, 80, 90,
                              120, 144, 180, 240, 360, 720};

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

typedef struct res {
    int16_t *pc;
    uint64_t npc;
    sk_counters k;
    r2_xcounters x;
    m1_scount s[3];
    m1_fig f[3];
    sk_int *tab[3];
    uint32_t *fld[3];
} res;

static void collect(const job *j, res *r) {
    memset(&r->k, 0, sizeof r->k);
    memset(&r->x, 0, sizeof r->x);
    for (int m = 0; m < 3; m++) {
        sk_counters k;
        r2_xcounters x;
        j->R->counters(j->M[m], &k, &x);
        uint64_t *a = (uint64_t *)&r->k, *b = (uint64_t *)&k;
        for (size_t i = 0; i < sizeof k / 8; i++) a[i] += b[i];
        a = (uint64_t *)&r->x;
        b = (uint64_t *)&x;
        for (size_t i = 0; i < sizeof x / 8; i++) a[i] += b[i];
        r->tab[m] = NULL;
        r->fld[m] = NULL;
        if (!j->R->hash) continue;
        r->s[m] = job_store(j, m);
        r->f[m] = job_fig(j, m);
        r->tab[m] = malloc(sizeof(sk_int) * r->f[m].T);
        for (uint32_t s = 0; s < r->f[m].T; s++) j->R->t_whole(j->M[m], s, &r->tab[m][s]);
        r->fld[m] = malloc(sizeof(uint32_t) * (r->f[m].nf + 1));
        for (uint32_t i = 0; i < r->f[m].nf; i++) j->R->t_field(j->M[m], i, &r->fld[m][i]);
    }
}

static void release(res *r) {
    free(r->pc);
    for (int m = 0; m < 3; m++) {
        free(r->tab[m]);
        free(r->fld[m]);
    }
}

/* the counters that every row makes alike: 08's, and round 3's but the K-Q3 reads (M rows only) */
static int same_arith(const res *a, const res *b) {
    if (memcmp(&a->k, &b->k, sizeof a->k)) return 0;
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            r2_xcount x = a->x.c[p][q], y = b->x.c[p][q];
            x.reads = y.reads = 0;
            if (memcmp(&x, &y, sizeof x)) return 0;
        }
    return 1;
}

int main(int argc, char **argv) {
    int seeds = 300;
    if (argc > 2 && !strcmp(argv[1], "--seeds")) seeds = atoi(argv[2]);
    long jobs = 0, ops = 0;
    for (int seed = 1; seed <= seeds; seed++) {
        rng_s = 0x9E3779B97F4A7C15ull * (uint64_t)seed + 7;
        int op = seed % 2;
        uint32_t n = 1 + (uint32_t)(rnd() % 9);
        uint32_t s = 1 + (uint32_t)(rnd() % (n * n));
        sk_int mag = op ? 2000 : (seed % 3 ? 50 : 30000);
        sk_int *A = malloc(16 * (size_t)n * n), *B = malloc(16 * (size_t)n * n);
        draw(A, n, s, mag);
        if (seed % 5 == 0) memcpy(B, A, 16 * (size_t)n * n); /* B's values all in A */
        else draw(B, n, s, mag);
        ref_q *want = job_ref(op, n, A, B);
        uint64_t nops = op ? 2ull * n * n * n : (uint64_t)n * n;
        res R[NROWS];
        for (int r = 0; r < NROWS; r++) {
            R[r].pc = malloc(sizeof(int16_t) * M1_PERCALL * nops);
            m1_percall = NULL;
            job j = {0};
            /* the per-operation record is taken from the operation alone */
            const row *W = &ROWS[r];
            j.R = W;
            j.n = n;
            for (int m = 0; m < 3; m++) j.M[m] = W->create(n, n, m);
            for (uint32_t i = 0; i < n * n; i++) CHECK_EQ(W->enter(j.M[0], A[2 * i], A[2 * i + 1]), SK_OK);
            for (uint32_t i = 0; i < n * n; i++) CHECK_EQ(W->enter(j.M[1], B[2 * i], B[2 * i + 1]), SK_OK);
            CHECK_EQ(W->tau_build(j.M[0], j.M[1], j.M[2]), SK_OK);
            if (W->m) {
                const char *why;
                CHECK_EQ(W->audit_tau(j.M[0], j.M[1], j.M[2], &why), SK_OK);
            }
            m1_percall = R[r].pc;
            m1_percall_n = 0;
            m1_percall_cap = nops;
            CHECK_EQ(op ? W->mmul(j.M[0], j.M[1], j.M[2]) : W->madd(j.M[0], j.M[1], j.M[2]), SK_OK);
            m1_percall = NULL;
            R[r].npc = m1_percall_n;
            W->tau_free(j.M[2]);
            CHECK_EQ(job_check(&j, want), 0);
            CHECK(job_audits(&j));
            collect(&j, &R[r]);
            job_free(&j);
        }
        for (int r = 1; r < NROWS; r++) {
            CHECK_EQ(R[r].npc, nops);
            CHECK(!memcmp(R[r].pc, R[0].pc, sizeof(int16_t) * M1_PERCALL * nops)); /* call for call with TC */
            CHECK(same_arith(&R[r], &R[0]));
            for (int m = 0; m < 3; m++) {
                CHECK_EQ(R[r].s[m].found, R[1].s[m].found); /* V/M and grow/fixed: the same found/new, W */
                CHECK_EQ(R[r].s[m].added, R[1].s[m].added);
                CHECK_EQ(R[r].f[m].W, R[1].f[m].W);
                CHECK(2ull * R[r].f[m].W <= R[r].f[m].T || !ROWS[r].grow);
                if (!ROWS[r].grow) { /* fixed: T_fix, never resized */
                    uint64_t kb = 2ull * n * n + 2, tf = 1;
                    while (tf < kb) tf <<= 1;
                    CHECK_EQ(R[r].f[m].T, (uint32_t)tf);
                    CHECK_EQ(R[r].s[m].growths, 0);
                }
            }
        }
        /* V against M within a sizing: the same tables and handles (the cross test does not touch the stores) */
        for (int p = 0; p < 2; p++) {
            int v = p ? 3 : 1, m = p ? 4 : 2;
            for (int x = 0; x < 3; x++) {
                CHECK(R[v].f[x].T == R[m].f[x].T && !memcmp(R[v].tab[x], R[m].tab[x], sizeof(sk_int) * R[v].f[x].T));
                CHECK(!memcmp(R[v].fld[x], R[m].fld[x], sizeof(uint32_t) * R[v].f[x].nf));
                CHECK(!memcmp(&R[v].s[x], &R[m].s[x], sizeof(m1_scount)));
            }
        }
        /* map (5) against lookup (3): the same tables and fields, the same counts except the rewrite probes */
        for (int x = 0; x < 3; x++) {
            CHECK(R[5].f[x].T == R[3].f[x].T && !memcmp(R[5].tab[x], R[3].tab[x], sizeof(sk_int) * R[3].f[x].T));
            CHECK(!memcmp(R[5].fld[x], R[3].fld[x], sizeof(uint32_t) * R[3].f[x].nf));
            m1_scount a = R[5].s[x], b = R[3].s[x];
            CHECK_EQ(a.wprobes, 0);
            a.wprobes = b.wprobes = 0;
            CHECK(!memcmp(&a, &b, sizeof a));
        }
        for (int r = 0; r < NROWS; r++) release(&R[r]);
        free(want);
        free(A);
        free(B);
        jobs++;
        ops += (long)nops;
    }
    printf("m1_diff: %d seeds, %ld jobs, %ld operations; %d checks, %d failures\n", seeds, jobs, ops, checks, failures);
    return failures != 0;
}
