/* m3_alloc.c: each row allocates only its named structures (spec 11-matrix v0.3 §16.3, "Nothing else exists"; §16.13,
 * reviewer: the structure counts of §16.3 and nothing else), checked on the allocations themselves, in the builds that
 * are measured (timing, counting) and in the memory build.  From m2_alloc.c.  Linked with
 * -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free, so that every allocation of the rows and of the 08
 * module passes through the wrappers below, which keep the live blocks made while a row is driven.
 *
 * For each row (TC, Hash-V-pack, Pack-primes, Pack-coprime), on random jobs of every set (and on jobs that overflow,
 * which must return the error in this build too):
 *   - create makes, per matrix, the header, the cells (a) and the build table (h) (TC: the header and its pairs);
 *   - inside every packing (the pack hook), the packing matrix's (h), narrowed cells, (l) and (p) are alive, and it
 *     has no (f) yet; inside every factoring (the factor hook, at its end), the blocks beyond the headers are exactly
 *     the live matrices' memory reports: (h) and (p) are freed before the factoring, and no (f'), (r) or scratch block
 *     is alive (rule-m3 item 12: "(h) or (p) alive during a factoring");
 *   - after each input's build and after each step, the blocks beyond the headers are exactly the live matrices'
 *     memory reports: per matrix TC 1 block; Hash-V-pack 2 (cells, list); Pack-primes 5 (cells, list, (f)'s three
 *     arrays); Pack-coprime 6 (and (q)): no (w), (h), (p), (f') or (r) alive outside its call (rule-m3 item 12: "(r)
 *     alive outside its step"), and the intermediates gone once freed after their last use (sop, pos);
 *   - the memory build: the accounted running total equals the bytes of the blocks beyond the headers at every one of
 *     these moments, and its peak is at least each of them;
 *   - destroy frees every block.
 *
 *   m3_alloc        (timing, counting or memory build) */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mh3.h" /* first: m3.h, m2.h, m1.h, r3.h */
#include "mh2.h"
#include "tcm.h"

void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);

#define MAXB 256
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
    int hash, fac, coprime;
    void *(*create)(uint32_t, uint32_t, int);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int);
    sk_status (*op[2])(void *, void *, void *);
    void (*memory)(const void *, sk_memory *);
    void (*figures)(const void *, m2_fig *);
    void (*ffigures)(const void *, m3_ffig *);
} arow;

#define AW(P)                                                                                                     \
    static void *P##a_create(uint32_t r, uint32_t c, int id) { return P##create(r, c, id); }                      \
    static void P##a_destroy(void *x) { P##destroy((P##class *)x); }                                              \
    static sk_status P##a_enter(void *x, sk_int m, sk_int d) { return P##enter((P##class *)x, m, d); }            \
    static sk_status P##a_madd(void *a, void *b, void *c) { return P##madd((P##class *)a, (P##class *)b, (P##class *)c); } \
    static sk_status P##a_mmul(void *a, void *b, void *c) { return P##mmul((P##class *)a, (P##class *)b, (P##class *)c); } \
    static void P##a_memory(const void *x, sk_memory *r) { P##memory((const P##class *)x, r); }                   \
    static void P##a_figures(const void *x, m2_fig *f) { P##figures((const P##class *)x, f); }
#define FW(P)                                                                                                     \
    static void P##a_ffigures(const void *x, m3_ffig *f) { P##ffigures((const P##class *)x, f); }
AW(mhvp_)
AW(mhpp_)
AW(mhpc_)
FW(mhpp_)
FW(mhpc_)
static void *tc_a_create(uint32_t r, uint32_t c, int id) {
    (void)id;
    return tcm_create(r, c);
}
static void tc_a_destroy(void *x) { tcm_destroy((tcm_class *)x); }
static sk_status tc_a_enter(void *x, sk_int m, sk_int d) { return tcm_enter((tcm_class *)x, m, d); }
static sk_status tc_a_madd(void *a, void *b, void *c) { return tcm_madd((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static sk_status tc_a_mmul(void *a, void *b, void *c) { return tcm_mmul((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static void tc_a_memory(const void *x, sk_memory *r) { tcm_memory((const tcm_class *)x, r); }

#define AR(name, f, c, P, FF)                                                                                     \
    {name, 1, f, c, P##a_create, P##a_destroy, P##a_enter, {P##a_madd, P##a_mmul}, P##a_memory, P##a_figures, FF}
static const arow AROWS[4] = {
    {"TC", 0, 0, 0, tc_a_create, tc_a_destroy, tc_a_enter, {tc_a_madd, tc_a_mmul}, tc_a_memory, NULL, NULL},
    AR("Hash-V-pack", 0, 0, mhvp_, NULL),
    AR("Pack-primes", 1, 0, mhpp_, mhpp_a_ffigures),
    AR("Pack-coprime", 1, 1, mhpc_, mhpc_a_ffigures),
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
static uint64_t beyond(void) {
    uint64_t t = 0;
    for (int k = 0; k < nb; k++)
        if (!is_header(BL[k].p)) t += BL[k].n;
    return t;
}
#endif

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

/* the blocks beyond the headers are exactly the live matrices' memory reports (items of 0 bytes have no block) */
static void exactly_reports(void) {
    size_t want[128], got[MAXB];
    int nw = 0, ng = 0;
    for (int x = 0; x < M2_NMAT; x++) {
        if (!MX[x]) continue;
        sk_memory mr;
        CUR->memory(MX[x], &mr);
        for (unsigned k = 0; k < mr.n && nw < 126; k++)
            if (mr.item[k].allocated) want[nw++] = (size_t)mr.item[k].allocated;
    }
    for (int k = 0; k < nb; k++)
        if (!is_header(BL[k].p)) got[ng++] = BL[k].n;
    qsort(want, (size_t)nw, sizeof(size_t), cmp_size);
    qsort(got, (size_t)ng, sizeof(size_t), cmp_size);
    CHECK(nw == ng);
    for (int k = 0; k < nw && k < ng; k++) CHECK(want[k] == got[k]);
    acct_check();
}

/* ----------------------------------------------------------------------------------------- hooks */

static int packs_seen, factors_seen;
static void on_pack(const m2_pack *p) {
    packs_seen++;
    CHECK(CUR->hash);
    int l = 0, m = 0, h = 0, c = 0;
    for (int k = 0; k < nb; k++) {
        if (BL[k].n == p->bytes_list) l = 1;
        if (BL[k].n == p->bytes_map) m = 1;
        if (BL[k].n == p->bytes_table) h = 1;
        if (BL[k].n == p->bytes_cells_pack) c = 1;
    }
    CHECK(l && m && h && c);
    if (CUR->fac) {
        m3_ffig f;
        CUR->ffigures(p->x, &f);
        CHECK(!f.factored); /* factored after the pack, not during the build */
    }
    acct_check();
}
static void on_factor(const m3_factor *f) {
    factors_seen++;
    CHECK(CUR->fac && f->fig.factored);
    exactly_reports(); /* (h), (p), (f'), (r) and the scratch all gone: only the live matrices' structures */
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

/* the blocks per finished matrix: TC 1, Hash-V-pack 2, Pack-primes 5, Pack-coprime 6 (fewer when an array is empty) */
static int per_matrix(void) { return !CUR->hash ? 1 : !CUR->fac ? 2 : CUR->coprime ? 6 : 5; }

static sk_status drive(const arow *R, int set, uint32_t n, const sk_int *const *in) {
    CUR = R;
    NAME = R->name;
    nb = worst = overflowed = nlive = 0;
    packs_seen = factors_seen = 0;
    memset(MX, 0, sizeof MX);
    memset(HDR, 0, sizeof HDR);
#ifdef M2_MEM
    m2_acct_live = m2_acct_peak = 0;
#endif
    armed = 1;
    m2_pack_hook = on_pack;
    m3_factor_hook = on_factor;
    sk_status st = SK_OK;
    int maxlive = 0;
    for (int x = 0; x < NIN[set]; x++) {
        void *m = make(x, n);
        for (uint32_t i = 0; i < n * n; i++) CHECK(R->enter(m, in[x][2 * i], in[x][2 * i + 1]) == SK_OK);
        exactly_reports(); /* the input's build ended: no temporary alive; packed and factored */
        if (R->hash) {
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
        if (st == SK_OK) exactly_reports(); /* no (r) alive outside its step */
    }
    if (st == SK_OK && R->fac)
        for (int x = 0; x < M2_NMAT; x++)
            if (MX[x]) {
                m3_ffig f;
                R->ffigures(MX[x], &f);
                CHECK(f.factored);
            }
    if (st == SK_OK && NSTEP[set] == 3) CHECK(!MX[M2_X1] && !MX[M2_X2] && nlive == NIN[set] + 1);
    /* at most: the live matrices' blocks, the header, cells and (h) of the one being built, and (l), (p) (packing), or
     * (f')'s three arrays and (r)'s two (a rebuild inside a step) */
    CHECK(worst <= (per_matrix() + 1) * maxlive + 6);
    CHECK(!overflowed);
    if (st == SK_OK && R->fac) CHECK(factors_seen == NIN[set] + NSTEP[set]);
    for (int x = 0; x < M2_NMAT; x++)
        if (MX[x]) unmake(x);
    CHECK(nb == 0); /* destroy frees every block */
#ifdef M2_MEM
    if (R->hash) CHECK(m2_acct_live == 0);
#endif
    armed = 0;
    m2_pack_hook = NULL;
    m3_factor_hook = NULL;
    return st;
}

int main(void) {
    long jobs = 0;
    for (int r = 0; r < 4; r++) {
        const arow *R = &AROWS[r];
        long packed = 0, factored = 0;
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
            packed += packs_seen;
            factored += factors_seen;
            if (worst > most) most = worst;
            for (int x = 0; x < 4; x++) free(X[x]);
            jobs++;
        }
        /* a matrix of 0s and 1s only: (f) with no entry, (q) with no piece */
        sk_int Z[8] = {0, 1, 1, 1, 0, 1, -1, 1};
        const sk_int *z[2] = {Z, Z};
        CHECK(drive(R, 1, 2, z) == SK_OK);
        /* overflow, in this build too: the error, and everything freed */
        const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
        sk_int O[2] = {h62, 1}, M[2] = {-h62, 1}, P[2] = {h32, 1}, U[2] = {1, 1}, ZZ[2] = {0, 1};
        const sk_int *o1[2] = {O, O}, *o2[2] = {M, M}, *o3[2] = {P, P}, *o4[4] = {O, U, O, U}, *o5[4] = {P, ZZ, P, ZZ};
        CHECK(drive(R, 0, 1, o1) == SK_EOVERFLOW);
        CHECK(drive(R, 0, 1, o2) == SK_EOVERFLOW); /* -2^63 */
        CHECK(drive(R, 1, 1, o3) == SK_EOVERFLOW);
        CHECK(drive(R, 2, 1, o4) == SK_EOVERFLOW);
        CHECK(drive(R, 3, 1, o5) == SK_EOVERFLOW);
        if (R->hash) CHECK(packed > 0);
        if (R->fac) CHECK(factored > 0);
        printf("  %s: at most %d blocks live; %ld packings, %ld factorings seen\n", R->name, most, packed, factored);
    }
    printf("m3_alloc (%s build): %ld jobs, %d moments, %d checks, %d failures\n",
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
