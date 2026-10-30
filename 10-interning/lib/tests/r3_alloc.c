/* r3_alloc.c: each row allocates only its named structures (spec v0.3 §6 item 5, §16 reviewer item 1), checked on the
 * allocations themselves.  Linked with -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free, so that every
 * allocation of the rows and of the 08 module (sk_xmalloc, sk_xcalloc, sk_xrealloc) passes through the wrappers below,
 * which keep the set of live blocks made while a row is driven.  Built in the timing, counting and profiling builds
 * (the builds that are measured); the test build's audits are checked by r3_unit and r3_diff.
 *
 * For each row, on random workloads (entries, then operations on earlier values; overflows and divisions by zero are
 * attempted, and store nothing):
 *   - create makes the class header, and in Q32-hash the table of exactly 32,768 x 8 = 262,144 bytes (plus, in the
 *     profiling build, the profile's state, part of the class header: v0.3 §6 item 5);
 *   - after every call, the live blocks are at most: TC 2 (header, pairs), Q32-index 3 (header, list of whole numbers,
 *     rationals), Q32-hash 3 (header, table, rationals), plus the profile's state in the profiling build;
 *   - at the end, the blocks other than the header (and the profile's state) are exactly the structures of the row's
 *     memory report, with their allocated sizes; the table is never reallocated or freed before destroy;
 *   - destroy frees every block.
 *
 *   r3_alloc        (timing, counting or profiling build) */
#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q32h.h"
#include "q32i.h"
#include "tc.h"

/* ----------------------------------------------------------------------------------------- the wrappers */

void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);

#define MAXB 64
static struct blk {
    void *p;
    size_t n;
} B[MAXB];
static int armed, nb, overflowed, table_moved;
static uint64_t allocs;

static int find(void *p) {
    for (int k = 0; k < nb; k++)
        if (B[k].p == p) return k;
    return -1;
}
static void add(void *p, size_t n) {
    if (!armed || !p) return;
    allocs++;
    if (nb == MAXB) {
        overflowed = 1;
        return;
    }
    B[nb].p = p;
    B[nb].n = n;
    nb++;
}
static void drop(void *p) {
    int k = find(p);
    if (k < 0) return;
    if (B[k].n == (size_t)R3_SLOTS * sizeof(sk_int)) table_moved++; /* counted; reset by the caller at destroy */
    B[k] = B[--nb];
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

/* ----------------------------------------------------------------------------------------- rows */

typedef struct arow {
    const char *name;
    int structures; /* named structures beyond the class header (v0.3 §§5, 7-9) */
    int table;      /* 1: one structure is Q32-hash's fixed table */
    void *(*create)(void);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int, sk_handle *);
    sk_status (*op[5])(void *, sk_handle, sk_handle, sk_handle *);
    void (*memory)(const void *, sk_memory *);
} arow;

#define AWRAP(P, T)                                                                                               \
    static void *P##a_create(void) { return P##create(); }                                                        \
    static void P##a_destroy(void *c) { P##destroy((T *)c); }                                                    \
    static sk_status P##a_enter(void *c, sk_int m, sk_int d, sk_handle *o) { return P##enter((T *)c, m, d, o); } \
    static sk_status P##a_add(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##add((T *)c, x, y, o); } \
    static sk_status P##a_sub(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##sub((T *)c, x, y, o); } \
    static sk_status P##a_mul(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##mul((T *)c, x, y, o); } \
    static sk_status P##a_div(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##div((T *)c, x, y, o); } \
    static void P##a_memory(const void *c, sk_memory *m) { P##memory((const T *)c, m); }
AWRAP(tc_, tc_class)
AWRAP(q32i_, q32i_class)
AWRAP(q32h_, q32h_class)
#define AROW(name, k, t, P)                                                                                       \
    {name, k, t, P##a_create, P##a_destroy, P##a_enter, {NULL, P##a_add, P##a_sub, P##a_mul, P##a_div}, P##a_memory}
static const arow AROWS[3] = {AROW("TC", 1, 0, tc_), AROW("Q32-index", 2, 0, q32i_), AROW("Q32-hash", 2, 1, q32h_)};

#ifdef SK_PROFILE
#define PF_BLOCKS 1 /* the profile's state, allocated in create with the class header */
#else
#define PF_BLOCKS 0
#endif

static int failures, checks;
#define CHECK(cond)                                                                                               \
    do {                                                                                                          \
        checks++;                                                                                                 \
        if (!(cond)) {                                                                                            \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s (%s)\n", __FILE__, __LINE__, #cond, R->name);                        \
        }                                                                                                         \
    } while (0)

static uint64_t rs = 88172645463325252ull;
static uint64_t rnd(void) {
    rs ^= rs << 13;
    rs ^= rs >> 7;
    rs ^= rs << 17;
    return rs;
}

static int cmp_size(const void *a, const void *b) {
    size_t x = *(const size_t *)a, y = *(const size_t *)b;
    return (x > y) - (x < y);
}

static void drive(const arow *R, uint64_t seed, int nin, int nops, int maxv) {
    rs = seed * 0x9E3779B97F4A7C15ull + 1;
    sk_handle *H = malloc(sizeof(sk_handle) * (size_t)(nin + nops));
    nb = 0;
    overflowed = table_moved = 0;
    armed = 1;
    void *c = R->create();
    void *header = nb ? B[0].p : NULL; /* the first block create makes */
    void *pfstate = NULL;               /* profiling build: create's block that is neither the header nor the table */
    for (int k = 1; k < nb; k++)
        if (PF_BLOCKS && B[k].n != (size_t)R3_SLOTS * sizeof(sk_int)) pfstate = B[k].p;
    int after_create = nb;
    CHECK(after_create == 1 + PF_BLOCKS + R->table);
    CHECK(!PF_BLOCKS || pfstate);
    if (R->table) {
        int t = 0;
        for (int k = 0; k < nb; k++) t += B[k].n == (size_t)R3_SLOTS * sizeof(sk_int);
        CHECK(t == 1);
    }
    int limit = 1 + PF_BLOCKS + R->structures, worst = nb, n = 0;
    for (int i = 0; i < nin; i++) {
        sk_int m = (sk_int)(rnd() % (uint64_t)maxv), d = 1 + (sk_int)(rnd() % (uint64_t)maxv);
        if (rnd() % 2) m = -m;
        if (R->enter(c, m, d, &H[n]) == SK_OK) n++;
        if (nb > worst) worst = nb;
    }
    for (int t = 0; t < nops; t++) {
        int op = 1 + (int)(rnd() % 4);
        sk_handle x = H[rnd() % (uint64_t)n], y = H[rnd() % (uint64_t)n];
        if (R->op[op](c, x, y, &H[n]) == SK_OK) n++;
        if (nb > worst) worst = nb;
    }
    CHECK(!overflowed);
    CHECK(worst <= limit);
    CHECK(table_moved == 0); /* the table is never reallocated or freed while the class lives */
    /* the blocks beyond the header (and the profile's state) are exactly the memory report's structures */
    sk_memory mr;
    R->memory(c, &mr);
    CHECK((int)mr.n == R->structures);
    size_t want[8], got[MAXB];
    int nw = 0, ng = 0;
    for (unsigned k = 0; k < mr.n && k < 8; k++)
        if (mr.item[k].allocated) want[nw++] = (size_t)mr.item[k].allocated;
    for (int k = 0; k < nb; k++)
        if (B[k].p != header && B[k].p != pfstate) got[ng++] = B[k].n;
    qsort(want, (size_t)nw, sizeof(size_t), cmp_size);
    qsort(got, (size_t)ng, sizeof(size_t), cmp_size);
    CHECK(nw == ng);
    for (int k = 0; k < nw && k < ng; k++) CHECK(want[k] == got[k]);
    R->destroy(c);
    CHECK(nb == 0); /* destroy frees every block */
    armed = 0;
    free(H);
    if (seed == 1)
        printf("  %s: %d blocks after create, at most %d live (limit %d), %llu allocations in all\n", R->name,
               after_create, worst, limit, (unsigned long long)allocs);
    allocs = 0;
}

int main(void) {
    for (int r = 0; r < 3; r++) {
        const arow *R = &AROWS[r];
        for (uint64_t s = 1; s <= 40; s++) drive(R, s, 50 + (int)(s % 7) * 30, 3000, s % 2 ? 50 : 100000);
        /* an empty class: create and destroy only */
        nb = 0;
        armed = 1;
        void *c = R->create();
        CHECK(nb == 1 + PF_BLOCKS + R->table);
        R->destroy(c);
        CHECK(nb == 0);
        armed = 0;
    }
    printf("r3_alloc (%s build): %d checks, %d failures\n",
#if defined(SK_PROFILE)
           "profiling",
#elif defined(SK_COUNT)
           "counting",
#else
           "timing",
#endif
           checks, failures);
    return failures != 0;
}
