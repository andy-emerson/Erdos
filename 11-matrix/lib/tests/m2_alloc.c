/* m2_alloc.c: each row allocates only its named structures (spec 11-matrix v0.2 §15.3, "No other structure exists";
 * §15.9, reviewer: the counts of §15.3 and nothing else), checked on the allocations themselves, in the builds that
 * are measured (timing, counting) and in the memory build.  From m1_alloc.c.  Linked with
 * -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free, so that every allocation of the rows and of the 08
 * module passes through the wrappers below, which keep the live blocks made while a row is driven.
 *
 * For each row, on random jobs of every set (and on jobs that overflow, which must return the error in this build
 * too):
 *   - create makes, per matrix, the header and its structures: TC its pairs; a hash row the cells (a) and the table (b)
 *     ((h) in Hash-V-pack), at T_fix (fixed, pack) or 16 (grow) slots;
 *   - inside every growth (the hook), the blocks beyond the live matrices' own are exactly (n) and (r); inside every
 *     packing, exactly (l) and (p) beside the packing matrix's cells and (h);
 *   - after each input's build and after each step, the blocks beyond the headers are exactly the live matrices'
 *     memory reports: no (w), (n), (r), (h) or (p) alive outside its call, every matrix of Hash-V-pack packed (a list
 *     of W entries), and the intermediates gone once freed after their last use (sop, pos);
 *   - at most 3 blocks per live matrix plus 2 are alive at once (TC: 2 per live matrix);
 *   - the memory build: the accounted running total equals the bytes of the blocks beyond the headers at every one of
 *     these moments, and its peak is at least each of them;
 *   - destroy frees every block.
 *
 *   m2_alloc        (timing, counting or memory build) */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mh2.h" /* first: m2.h, m1.h, r3.h */
#include "tcm.h"

void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);

#define MAXB 128
static struct blk {
    void *p;
    size_t n;
} BL[MAXB];
static int armed, nb, overflowed, worst;

static int find(void *p) {
    for (int k = 0; k < nb; k++)
        if (BL[k].p == p) return k;
    return -1;
}
static void add(void *p, size_t n) {
    if (!armed || !p) return;
    if (nb == MAXB) {
        overflowed = 1;
        return;
    }
    BL[nb].p = p;
    BL[nb].n = n;
    nb++;
    if (nb > worst) worst = nb;
}
static void drop(void *p) {
    int k = find(p);
    if (k < 0) return;
    BL[k] = BL[--nb];
}
void *__wrap_malloc(size_t n) {
    void *p = __real_malloc(n);
    add(p, n);
    return p;
}
void *__wrap_calloc(size_t n, size_t s) {
    void *p = __real_calloc(n, s);
    add(p, n * s);
    return p;
}
void *__wrap_realloc(void *q, size_t n) {
    void *p = __real_realloc(q, n);
    if (p && armed) {
        if (q) drop(q);
        add(p, n);
    }
    return p;
}
void __wrap_free(void *p) {
    if (armed) drop(p);
    __real_free(p);
}

static int failures, checks;
static const char *NAME;
#define CHECK(cond)                                                                                               \
    do {                                                                                                          \
        checks++;                                                                                                 \
        if (!(cond)) {                                                                                            \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, NAME);                           \
        }                                                                                                         \
    } while (0)

/* ----------------------------------------------------------------------------------------- rows */

typedef struct arow {
    const char *name;
    int hash, grow, pack;
    void *(*create)(uint32_t, uint32_t, int);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int);
    sk_status (*op[2])(void *, void *, void *);
    void (*memory)(const void *, sk_memory *);
    void (*figures)(const void *, m2_fig *);
} arow;

#define AW(P)                                                                                                     \
    static void *P##a_create(uint32_t r, uint32_t c, int id) { return P##create(r, c, id); }                      \
    static void P##a_destroy(void *x) { P##destroy((P##class *)x); }                                              \
    static sk_status P##a_enter(void *x, sk_int m, sk_int d) { return P##enter((P##class *)x, m, d); }            \
    static sk_status P##a_madd(void *a, void *b, void *c) { return P##madd((P##class *)a, (P##class *)b, (P##class *)c); } \
    static sk_status P##a_mmul(void *a, void *b, void *c) { return P##mmul((P##class *)a, (P##class *)b, (P##class *)c); } \
    static void P##a_memory(const void *x, sk_memory *r) { P##memory((const P##class *)x, r); }                   \
    static void P##a_figures(const void *x, m2_fig *f) { P##figures((const P##class *)x, f); }
AW(mhvf_)
AW(mhvgm_)
AW(mhvp_)
static void *tc_a_create(uint32_t r, uint32_t c, int id) {
    (void)id;
    return tcm_create(r, c);
}
static void tc_a_destroy(void *x) { tcm_destroy((tcm_class *)x); }
static sk_status tc_a_enter(void *x, sk_int m, sk_int d) { return tcm_enter((tcm_class *)x, m, d); }
static sk_status tc_a_madd(void *a, void *b, void *c) { return tcm_madd((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static sk_status tc_a_mmul(void *a, void *b, void *c) { return tcm_mmul((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static void tc_a_memory(const void *x, sk_memory *r) { tcm_memory((const tcm_class *)x, r); }

#define AR(name, g, p, P)                                                                                         \
    {name, 1, g, p, P##a_create, P##a_destroy, P##a_enter, {P##a_madd, P##a_mmul}, P##a_memory, P##a_figures}
static const arow AROWS[4] = {
    {"TC", 0, 0, 0, tc_a_create, tc_a_destroy, tc_a_enter, {tc_a_madd, tc_a_mmul}, tc_a_memory, NULL},
    AR("Hash-V-fixed", 0, 0, mhvf_),
    AR("Hash-V-grow-map", 1, 0, mhvgm_),
    AR("Hash-V-pack", 0, 1, mhvp_),
};

/* the job sets (spec v0.2 §15.5) */
typedef struct step {
    int op, a, b, c;
} step;
static const int NIN[4] = {2, 2, 4, 4}, NSTEP[4] = {1, 1, 3, 3};
static const step STEPS[4][3] = {
    {{0, M2_A, M2_B, M2_R}},
    {{1, M2_A, M2_B, M2_R}},
    {{1, M2_A, M2_B, M2_X1}, {1, M2_C, M2_D, M2_X2}, {0, M2_X1, M2_X2, M2_R}},
    {{0, M2_A, M2_B, M2_X1}, {0, M2_C, M2_D, M2_X2}, {1, M2_X1, M2_X2, M2_R}},
};

/* ----------------------------------------------------------------------------------------- the live state */

static const arow *CUR;
static void *MX[M2_NMAT], *HDR[M2_NMAT];
static int nlive, moments;

static int is_header(void *p) {
    for (int x = 0; x < M2_NMAT; x++)
        if (MX[x] && HDR[x] == p) return 1;
    return 0;
}

#ifdef M2_MEM
/* the bytes of the blocks beyond the headers */
static uint64_t beyond(void) {
    uint64_t t = 0;
    for (int k = 0; k < nb; k++)
        if (!is_header(BL[k].p)) t += BL[k].n;
    return t;
}
#endif

/* the memory build: the accounted running total is the bytes of every structure alive (headers are not structures) */
static void acct_check(void) {
    moments++;
#ifdef M2_MEM
    if (CUR->hash) {
        CHECK(m2_acct_live == beyond());
        CHECK(m2_acct_peak >= m2_acct_live);
    }
#endif
}

static int cmp_size(const void *a, const void *b) {
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

/* the blocks beyond the headers are exactly the live matrices' memory reports (plus `extra`); Hash-V-pack's finished
 * matrices packed */
static void exactly_reports(const size_t *extra, int nextra) {
    size_t want[64], got[MAXB];
    int nw = 0, ng = 0;
    for (int x = 0; x < M2_NMAT; x++) {
        if (!MX[x]) continue;
        sk_memory mr;
        CUR->memory(MX[x], &mr);
        CHECK((int)mr.n == (CUR->hash ? 2 : 1));
        for (unsigned k = 0; k < mr.n && nw < 62; k++) want[nw++] = (size_t)mr.item[k].allocated;
    }
    for (int k = 0; k < nextra; k++) want[nw++] = extra[k];
    for (int k = 0; k < nb; k++)
        if (!is_header(BL[k].p)) got[ng++] = BL[k].n;
    qsort(want, (size_t)nw, sizeof(size_t), cmp_size);
    qsort(got, (size_t)ng, sizeof(size_t), cmp_size);
    CHECK(nw == ng);
    for (int k = 0; k < nw && k < ng; k++) CHECK(want[k] == got[k]);
    acct_check();
}

/* ----------------------------------------------------------------------------------------- hooks */

static int growths_seen, packs_seen;
static void on_growth(const m2_growth *g) {
    growths_seen++;
    CHECK(CUR->grow);
    /* beyond the live matrices' structures (the growing one still holding its old table): exactly (n) and (r) */
    CHECK(nb == 3 * nlive + 2);
    int seen_new = 0, seen_map = 0;
    for (int k = 0; k < nb; k++) {
        if (BL[k].n == g->bytes_new) seen_new = 1;
        if (BL[k].n == g->bytes_map) seen_map = 1;
    }
    CHECK(seen_new && seen_map && g->bytes_map > 0);
    acct_check();
}
static void on_pack(const m2_pack *p) {
    packs_seen++;
    CHECK(CUR->pack);
    /* the packing matrix's cells (narrowed) and (h), and (l) and (p), beside the other live matrices' structures */
    CHECK(nb == 3 * nlive + 2);
    int l = 0, m = 0, h = 0, c = 0;
    for (int k = 0; k < nb; k++) {
        if (BL[k].n == p->bytes_list) l = 1;
        if (BL[k].n == p->bytes_map) m = 1;
        if (BL[k].n == p->bytes_table) h = 1;
        if (BL[k].n == p->bytes_cells_pack) c = 1;
    }
    CHECK(l && m && h && c);
    CHECK(p->wh_pack <= p->wh_build && (p->W <= (1u << (p->wh_pack - 1))));
    acct_check();
}

/* ----------------------------------------------------------------------------------------- driving */

static uint64_t rs = 88172645463325252ull;
static uint64_t rnd(void) {
    rs ^= rs << 13;
    rs ^= rs >> 7;
    rs ^= rs << 17;
    return rs;
}
static const sk_int D720[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 48, 60, 72, 80, 90,
                              120, 144, 180, 240, 360, 720};

static void *make(int id, uint32_t n) {
    int before = nb;
    void *x = CUR->create(n, n, id);
    MX[id] = x;
    HDR[id] = BL[before].p; /* the first block create makes: the header */
    nlive++;
    CHECK(nb - before == (CUR->hash ? 3 : 2));
    if (CUR->hash) {
        m2_fig f;
        CUR->figures(x, &f);
        CHECK(f.W == 0 && f.T == (CUR->grow ? 16u : f.T));
        size_t want[2] = {(size_t)f.bytes_cells, (size_t)f.bytes_vals}, got[2] = {BL[before + 1].n, BL[before + 2].n};
        qsort(want, 2, sizeof(size_t), cmp_size);
        qsort(got, 2, sizeof(size_t), cmp_size);
        CHECK(want[0] == got[0] && want[1] == got[1]);
    }
    acct_check();
    return x;
}

static void unmake(int id) {
    CUR->destroy(MX[id]);
    MX[id] = HDR[id] = NULL;
    nlive--;
}

/* one job; returns the first failing status */
static sk_status drive(const arow *R, int set, uint32_t n, const sk_int *const *in) {
    CUR = R;
    NAME = R->name;
    nb = worst = overflowed = nlive = 0;
    growths_seen = packs_seen = 0;
    memset(MX, 0, sizeof MX);
    memset(HDR, 0, sizeof HDR);
#ifdef M2_MEM
    m2_acct_live = m2_acct_peak = 0;
#endif
    armed = 1;
    m2_growth_hook = on_growth;
    m2_pack_hook = on_pack;
    sk_status st = SK_OK;
    int maxlive = 0;
    for (int x = 0; x < NIN[set]; x++) {
        void *m = make(x, n);
        for (uint32_t i = 0; i < n * n; i++) CHECK(R->enter(m, in[x][2 * i], in[x][2 * i + 1]) == SK_OK);
        exactly_reports(NULL, 0); /* the input's build ended: no temporary alive; Hash-V-pack: packed */
        if (R->pack) {
            m2_fig f;
            R->figures(m, &f);
            CHECK(f.packed && f.bytes_vals == (uint64_t)f.W * 8);
        }
    }
    for (int s = 0; s < NSTEP[set] && st == SK_OK; s++) {
        const step *t = &STEPS[set][s];
        make(t->c, n);
        if (nlive > maxlive) maxlive = nlive;
        st = R->op[t->op](MX[t->a], MX[t->b], MX[t->c]);
        if (st == SK_OK && s == NSTEP[set] - 1)
            for (int x = M2_X1; x <= M2_X2; x++) /* the intermediates, freed after their last use */
                if (MX[x]) unmake(x);
        if (st == SK_OK) exactly_reports(NULL, 0);
    }
    if (st == SK_OK && R->pack)
        for (int x = 0; x < M2_NMAT; x++)
            if (MX[x]) {
                m2_fig f;
                R->figures(MX[x], &f);
                CHECK(f.packed);
            }
    if (st == SK_OK && NSTEP[set] == 3) CHECK(!MX[M2_X1] && !MX[M2_X2] && nlive == NIN[set] + 1);
    CHECK(worst <= (R->hash ? 3 * maxlive + 2 : 2 * maxlive));
    CHECK(!overflowed);
    for (int x = 0; x < M2_NMAT; x++)
        if (MX[x]) unmake(x);
    CHECK(nb == 0); /* destroy frees every block */
#ifdef M2_MEM
    if (R->hash) CHECK(m2_acct_live == 0);
#endif
    armed = 0;
    m2_growth_hook = NULL;
    m2_pack_hook = NULL;
    return st;
}

int main(void) {
    long jobs = 0;
    for (int r = 0; r < 4; r++) {
        const arow *R = &AROWS[r];
        long grown = 0, packed = 0;
        int most = 0;
        for (uint64_t s = 1; s <= 40; s++) {
            rs = s * 0x9E3779B97F4A7C15ull + 1;
            int set = (int)(s % 4);
            uint32_t n = 1 + (uint32_t)(s % 9);
            sk_int *X[4];
            for (int x = 0; x < 4; x++) {
                X[x] = malloc(16 * (size_t)n * n);
                for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                    X[x][i] = (sk_int)(rnd() % 4001) - 2000;
                    X[x][i + 1] = D720[rnd() % 30];
                }
            }
            const sk_int *in[4] = {X[0], X[1], X[2], X[3]};
            CHECK(drive(R, set, n, in) == SK_OK);
            grown += growths_seen;
            packed += packs_seen;
            if (worst > most) most = worst;
            for (int x = 0; x < 4; x++) free(X[x]);
            jobs++;
        }
        /* overflow, in this build too: the error, and everything freed */
        const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
        sk_int O[2] = {h62, 1}, M[2] = {-h62, 1}, P[2] = {h32, 1}, U[2] = {1, 1}, Z[2] = {0, 1};
        const sk_int *o1[2] = {O, O}, *o2[2] = {M, M}, *o3[2] = {P, P}, *o4[4] = {O, U, O, U}, *o5[4] = {P, Z, P, Z};
        CHECK(drive(R, 0, 1, o1) == SK_EOVERFLOW);
        CHECK(drive(R, 0, 1, o2) == SK_EOVERFLOW); /* -2^63 */
        CHECK(drive(R, 1, 1, o3) == SK_EOVERFLOW);
        CHECK(drive(R, 2, 1, o4) == SK_EOVERFLOW);
        CHECK(drive(R, 3, 1, o5) == SK_EOVERFLOW);
        if (R->grow) CHECK(grown > 0);
        if (R->pack) CHECK(packed > 0);
        printf("  %s: at most %d blocks live (limit %s per live matrix); %ld growths, %ld packings seen\n", R->name, most,
               R->hash ? "3, plus 2" : "2", grown, packed);
    }
    printf("m2_alloc (%s build): %ld jobs, %d moments, %d checks, %d failures\n",
#if defined(M2_MEM)
           "memory",
#elif defined(SK_COUNT)
           "counting",
#else
           "timing",
#endif
           jobs, moments, checks, failures);
    return failures != 0;
}
