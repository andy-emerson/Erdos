/* m1_alloc.c: each row allocates only its named structures (spec 11-matrix v0.1 §3, "Nothing else exists"; §14,
 * reviewer: the structures and counts 3, 6, 7, 6, 7, 7), checked on the allocations themselves, in the builds that are
 * measured (timing and counting).  From round 3's r3_alloc.c.  Linked with
 * -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free, so that every allocation of the rows and of the 08
 * module passes through the wrappers below, which keep the live blocks made while a row is driven.
 *
 * For each row, on random jobs of both operations (and on jobs that overflow, which must return the error in this
 * build too):
 *   - create makes, per matrix, the header and its structures: TC the pairs (T-x); a hash row the cells (a) and the
 *     table (b), at T_fix (fixed) or 16 (grow) slots;
 *   - tau_build makes one block in the M rows, tau (t) of T_B x w_h(A) bits, and none in the V rows;
 *   - at most these blocks are live at once: TC 6; Hash-V-fixed 9 + 1 (the widened table (w) beside the old, inside
 *     its store call); Hash-M-fixed 9 + 1 (t) + 1 (w); Hash-V-grow 9 + 1 ((n) or (w)); Hash-M-grow 9 + 1 (t) + 1;
 *     Hash-V-grow-map 9 + 1 (n) + 1 (r);
 *   - inside every growth (the hook): the grown table (n) of 2T slots at the store's width, and in the map row the slot
 *     map (r) of T entries of w_h bits, are the only blocks beyond the matrices' own and tau;
 *   - a fixed row frees no block while A and B are entered, and frees only C's replaced tables (one per widening)
 *     during the operation;
 *   - at the end of the operation the blocks beyond the headers are exactly the memory reports' structures and tau;
 *     after tau_free, exactly the memory reports'; destroy frees every block.
 *
 *   m1_alloc        (timing or counting build) */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mh.h" /* first: m1.h, r3.h */
#include "tcm.h"

void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);

#define MAXB 64
static struct blk {
    void *p;
    size_t n;
} BL[MAXB];
static int armed, nb, overflowed, worst;
static uint64_t frees;

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
    frees++;
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
    int hash, m, grow, map;
    void *(*create)(uint32_t, uint32_t, int);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int);
    sk_status (*tau_build)(void *, void *, void *);
    void (*tau_free)(void *);
    uint64_t (*tau_bytes)(const void *);
    sk_status (*op[2])(void *, void *, void *);
    void (*memory)(const void *, sk_memory *);
    void (*figures)(const void *, m1_fig *);
} arow;

#define AW(P)                                                                                                     \
    static void *P##a_create(uint32_t r, uint32_t c, int role) { return P##create(r, c, role); }                  \
    static void P##a_destroy(void *x) { P##destroy((P##class *)x); }                                              \
    static sk_status P##a_enter(void *x, sk_int m, sk_int d) { return P##enter((P##class *)x, m, d); }            \
    static sk_status P##a_tau_build(void *a, void *b, void *c) {                                                  \
        return P##tau_build((P##class *)a, (P##class *)b, (P##class *)c);                                         \
    }                                                                                                             \
    static void P##a_tau_free(void *c) { P##tau_free((P##class *)c); }                                            \
    static uint64_t P##a_tau_bytes(const void *c) { return P##tau_bytes((const P##class *)c); }                   \
    static sk_status P##a_madd(void *a, void *b, void *c) { return P##madd((P##class *)a, (P##class *)b, (P##class *)c); } \
    static sk_status P##a_mmul(void *a, void *b, void *c) { return P##mmul((P##class *)a, (P##class *)b, (P##class *)c); } \
    static void P##a_memory(const void *x, sk_memory *r) { P##memory((const P##class *)x, r); }                   \
    static void P##a_figures(const void *x, m1_fig *f) { P##figures((const P##class *)x, f); }
AW(mhvf_)
AW(mhmf_)
AW(mhvg_)
AW(mhmg_)
AW(mhvgm_)
static void *tc_a_create(uint32_t r, uint32_t c, int role) {
    (void)role;
    return tcm_create(r, c);
}
static void tc_a_destroy(void *x) { tcm_destroy((tcm_class *)x); }
static sk_status tc_a_enter(void *x, sk_int m, sk_int d) { return tcm_enter((tcm_class *)x, m, d); }
static sk_status tc_a_tau_build(void *a, void *b, void *c) {
    (void)a, (void)b, (void)c;
    return SK_OK;
}
static void tc_a_tau_free(void *c) { (void)c; }
static uint64_t tc_a_tau_bytes(const void *c) {
    (void)c;
    return 0;
}
static sk_status tc_a_madd(void *a, void *b, void *c) { return tcm_madd((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static sk_status tc_a_mmul(void *a, void *b, void *c) { return tcm_mmul((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static void tc_a_memory(const void *x, sk_memory *r) { tcm_memory((const tcm_class *)x, r); }

#define AR(name, m, g, mp, P)                                                                                     \
    {name, 1, m, g, mp, P##a_create, P##a_destroy, P##a_enter, P##a_tau_build, P##a_tau_free, P##a_tau_bytes,      \
     {P##a_madd, P##a_mmul}, P##a_memory, P##a_figures}
static const arow AROWS[6] = {
    {"TC", 0, 0, 0, 0, tc_a_create, tc_a_destroy, tc_a_enter, tc_a_tau_build, tc_a_tau_free, tc_a_tau_bytes,
     {tc_a_madd, tc_a_mmul}, tc_a_memory, NULL},
    AR("Hash-V-fixed", 0, 0, 0, mhvf_),
    AR("Hash-M-fixed", 1, 0, 0, mhmf_),
    AR("Hash-V-grow", 0, 1, 0, mhvg_),
    AR("Hash-M-grow", 1, 1, 0, mhmg_),
    AR("Hash-V-grow-map", 0, 1, 1, mhvgm_),
};

/* ----------------------------------------------------------------------------------------- the growth hook */

static const arow *CUR;
static void *MX[3];
static int in_op, growths_seen;
static void hook(const m1_growth *g) {
    growths_seen++;
    /* the blocks beyond the headers and the matrices' structures (each its own memory report, the old table still
     * its table) and tau: exactly (n), and (r) in the map row */
    size_t extra[2];
    int ne = 0;
    extra[ne++] = (size_t)g->bytes_new;
    if (CUR->map) extra[ne++] = (size_t)g->bytes_map;
    CHECK(CUR->map ? g->bytes_map > 0 : g->bytes_map == 0);
    int expect = 3 * 3 + ne + (in_op && CUR->m ? 1 : 0);
    CHECK(nb == expect);
    int seen_new = 0, seen_map = 0;
    for (int k = 0; k < nb; k++) {
        if (BL[k].n == extra[0]) seen_new = 1;
        if (ne > 1 && BL[k].n == extra[1]) seen_map = 1;
    }
    CHECK(seen_new);
    CHECK(ne < 2 || seen_map);
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

static int cmp_size(const void *a, const void *b) {
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

/* one job; returns the operation's status */
static sk_status drive(const arow *R, int op, uint32_t n, const sk_int *A, const sk_int *B) {
    CUR = R;
    NAME = R->name;
    nb = 0;
    worst = 0;
    overflowed = 0;
    frees = 0;
    growths_seen = 0;
    in_op = 0;
    armed = 1;
    m1_growth_hook = R->grow ? hook : NULL;
    void *hdr[3];
    for (int r = 0; r < 3; r++) {
        int before = nb;
        MX[r] = R->create(n, n, r);
        hdr[r] = BL[before].p; /* the first block create makes: the header */
        CHECK(nb - before == (R->hash ? 3 : 2));
        if (R->hash) {
            m1_fig f;
            R->figures(MX[r], &f);
            CHECK(f.T == (R->grow ? 16u : f.T) && f.W == 2);
            /* the cells and the table, with their sizes */
            size_t want[2] = {(size_t)f.bytes_cells, (size_t)f.bytes_table}, got[2] = {BL[before + 1].n, BL[before + 2].n};
            qsort(want, 2, sizeof(size_t), cmp_size);
            qsort(got, 2, sizeof(size_t), cmp_size);
            CHECK(want[0] == got[0] && want[1] == got[1]);
        }
    }
    for (uint32_t i = 0; i < n * n; i++) CHECK(R->enter(MX[0], A[2 * i], A[2 * i + 1]) == SK_OK);
    for (uint32_t i = 0; i < n * n; i++) CHECK(R->enter(MX[1], B[2 * i], B[2 * i + 1]) == SK_OK);
    if (!R->grow) CHECK(frees == 0); /* fixed: no table is replaced while A and B are entered */
    uint64_t f0 = frees;
    int before = nb;
    CHECK(R->tau_build(MX[0], MX[1], MX[2]) == SK_OK);
    CHECK(nb - before == (R->m ? 1 : 0));
    if (R->m) CHECK(BL[nb - 1].n == (size_t)R->tau_bytes(MX[2]));
    in_op = 1;
    sk_status st = R->op[op](MX[0], MX[1], MX[2]);
    in_op = 0;
    if (st == SK_OK) {
        /* the blocks beyond the headers: exactly the memory reports' structures, and tau */
        size_t want[16], got[MAXB];
        int nw = 0, ng = 0;
        for (int r = 0; r < 3; r++) {
            sk_memory mr;
            R->memory(MX[r], &mr);
            CHECK((int)mr.n == (R->hash ? 2 : 1));
            for (unsigned k = 0; k < mr.n; k++) want[nw++] = (size_t)mr.item[k].allocated;
        }
        if (R->m) want[nw++] = (size_t)R->tau_bytes(MX[2]);
        for (int k = 0; k < nb; k++)
            if (BL[k].p != hdr[0] && BL[k].p != hdr[1] && BL[k].p != hdr[2]) got[ng++] = BL[k].n;
        qsort(want, (size_t)nw, sizeof(size_t), cmp_size);
        qsort(got, (size_t)ng, sizeof(size_t), cmp_size);
        CHECK(nw == ng);
        for (int k = 0; k < nw && k < ng; k++) CHECK(want[k] == got[k]);
        if (!R->grow && R->hash) {
            m1_fig fc;
            R->figures(MX[2], &fc);
            /* fixed: only C's replaced tables are freed, one per widening (8 -> w: at most 3) */
            CHECK(frees - f0 <= 3);
            CHECK(fc.w > 8 ? frees - f0 >= 1 : frees == f0);
        }
    }
    R->tau_free(MX[2]);
    CHECK(nb == (R->hash ? 9 : 6));
    int limit = R->hash ? 9 + 1 + (R->m || R->map ? 1 : 0) : 6;
    CHECK(worst <= limit);
    CHECK(!overflowed);
    for (int r = 0; r < 3; r++) R->destroy(MX[r]);
    CHECK(nb == 0); /* destroy frees every block */
    armed = 0;
    m1_growth_hook = NULL;
    return st;
}

int main(void) {
    long jobs = 0;
    for (int r = 0; r < 6; r++) {
        const arow *R = &AROWS[r];
        long grown = 0;
        int most = 0;
        for (uint64_t s = 1; s <= 30; s++) {
            rs = s * 0x9E3779B97F4A7C15ull + 1;
            int op = (int)(s % 2);
            uint32_t n = 1 + (uint32_t)(s % 9);
            sk_int *A = malloc(16 * (size_t)n * n), *B = malloc(16 * (size_t)n * n);
            for (uint32_t i = 0; i < 2 * n * n; i += 2) {
                A[i] = (sk_int)(rnd() % 4001) - 2000;
                A[i + 1] = D720[rnd() % 30];
                B[i] = (sk_int)(rnd() % 4001) - 2000;
                B[i + 1] = D720[rnd() % 30];
            }
            CHECK(drive(R, op, n, A, B) == SK_OK);
            grown += growths_seen;
            if (worst > most) most = worst;
            free(A);
            free(B);
            jobs++;
        }
        /* overflow, in this build too: the error, nothing more allocated, everything freed */
        const sk_int h62 = (sk_int)1 << 62, h32 = (sk_int)1 << 32;
        sk_int OA[2] = {h62, 1}, OB[2] = {h62, 1}, MA[2] = {-h62, 1}, PA[2] = {h32, 1};
        CHECK(drive(R, 0, 1, OA, OB) == SK_EOVERFLOW);
        CHECK(drive(R, 0, 1, MA, MA) == SK_EOVERFLOW); /* -2^63 */
        CHECK(drive(R, 1, 1, PA, PA) == SK_EOVERFLOW);
        if (R->grow) CHECK(grown > 0);
        printf("  %s: at most %d blocks live (limit %s); %ld growths seen\n", R->name, most,
               R->hash ? (R->m || R->map ? "11" : "10") : "6", grown);
    }
    printf("m1_alloc (%s build): %ld jobs, %d checks, %d failures\n",
#if defined(SK_COUNT)
           "counting",
#else
           "timing",
#endif
           jobs, checks, failures);
    return failures != 0;
}
