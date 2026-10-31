/* alloc.c: the structures of spec v0.4 §17.8, block by block (README.md, "Verification").  Linked with one build of
 * both Nim models and with -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free, it records every allocation
 * event of the models and checks, on small matrices of every handle width:
 *   Packed:  create allocates exactly its header, the cells (2rc fields at the build width), (h) (T_fix slots) and
 *            (l) (2rc entries of 64 bits); entry allocates nothing until the last enter, whose pack frees (h), then
 *            reallocs (l) to W w/8 bytes, then reallocs the cells to the packed width if it is smaller; madd allocates
 *            nothing before its pack; mmul allocates one row buffer of n x 24 bytes and frees it before the pack;
 *            destroy frees the cells, (l) and the header; nothing else is ever allocated;
 *   Control: create allocates its header and its cells (16 rc bytes); enter, madd and mmul allocate nothing;
 *            destroy frees both.
 * Built once per model (-DALLOC_CONTROL for Control; the two Nim libraries cannot be linked into one program, each
 * carrying Nim's own entry points).  In the memory build it also checks that the accounted total equals the bytes of the model's live structures after
 * every call.  Prints "alloc: <checks> checks, <failures> failures"; exit 1 on a failure. */
#define _GNU_SOURCE
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m4n.h"

enum { E_MALLOC, E_CALLOC, E_REALLOC, E_FREE };
typedef struct ev {
    int kind;
    size_t size; /* requested (realloc: the new size) */
    void *p, *old;
} ev;
static ev EV[256];
static int NEV, ON;

void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t s);
void *__real_realloc(void *p, size_t n);
void __real_free(void *p);
static void rec(int k, size_t n, void *p, void *old) {
    if (ON && NEV < 256) EV[NEV++] = (ev){k, n, p, old};
}
void *__wrap_malloc(size_t n) {
    void *p = __real_malloc(n);
    rec(E_MALLOC, n, p, 0);
    return p;
}
void *__wrap_calloc(size_t n, size_t s) {
    void *p = __real_calloc(n, s);
    rec(E_CALLOC, n * s, p, 0);
    return p;
}
void *__wrap_realloc(void *q, size_t n) {
    void *p = __real_realloc(q, n);
    rec(E_REALLOC, n, p, q);
    return p;
}
void __wrap_free(void *p) {
    rec(E_FREE, 0, 0, p);
    __real_free(p);
}

static int CHECKS, FAILS;
#define CHECK(c, ...)                                                                                             \
    do {                                                                                                          \
        CHECKS++;                                                                                                 \
        if (!(c)) {                                                                                               \
            FAILS++;                                                                                              \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                           \
            printf(__VA_ARGS__);                                                                                  \
            printf("\n");                                                                                         \
        }                                                                                                         \
    } while (0)

static void start(void) {
    NEV = 0;
    ON = 1;
}
static void stop(void) { ON = 0; }

#ifndef ALLOC_CONTROL
static unsigned build_width(unsigned tbits) { return tbits <= 7 ? 8 : tbits <= 15 ? 16 : 32; }
static unsigned pack_width(unsigned W) { return W <= 128 ? 8 : W <= 32768 ? 16 : 32; }
static unsigned tbits_of(size_t bound) {
    unsigned t = 0;
    while (((size_t)1 << t) < bound) t++;
    return t;
}

static void acct_check(npk_class *const *xs, int k, const char *when) {
#ifdef M2_MEM
    uint64_t u = 0;
    for (int i = 0; i < k; i++) {
        sk_memory r;
        npk_memory(xs[i], &r);
        u += r.used;
    }
    CHECK(u == m2_acct_live, "%s: the accounted total %llu is not the live structures' %llu", when,
          (unsigned long long)m2_acct_live, (unsigned long long)u);
#else
    (void)xs, (void)k, (void)when;
#endif
}

/* an input of n x n cells with `distinct` distinct values (k+1)/k, built and checked block by block */
static npk_class *packed_input(uint32_t n, uint32_t distinct, int id) {
    size_t rc = (size_t)n * n, bound = 2 * rc;
    unsigned t = tbits_of(bound), whb = build_width(t);
    size_t T = (size_t)1 << t;
    start();
    npk_class *x = npk_create(n, n, id);
    stop();
    CHECK(NEV == 4, "create n=%u: %d events, not 4", n, NEV);
    if (NEV == 4) {
        CHECK(EV[0].kind == E_CALLOC, "create: the header first");
        CHECK(EV[1].kind == E_MALLOC && EV[1].size == bound * whb / 8, "create: the cells, %zu bytes", EV[1].size);
        CHECK(EV[2].kind == E_MALLOC && EV[2].size == T * whb / 8, "create: (h), %zu bytes", EV[2].size);
        CHECK(EV[3].kind == E_MALLOC && EV[3].size == bound * 8, "create: (l), %zu bytes", EV[3].size);
    }
    void *table = NEV == 4 ? EV[2].p : 0, *cells = NEV == 4 ? EV[1].p : 0, *list = NEV == 4 ? EV[3].p : 0;
    for (size_t i = 0; i + 1 < rc; i++) {
        start();
        sk_int k = 2 + 2 * (sk_int)(i % distinct);
        CHECK(npk_enter(x, k + 1, k) == SK_OK, "enter");
        stop();
        CHECK(NEV == 0, "enter %zu: %d events, not 0", i, NEV);
    }
    start();
    sk_int k = 2 + 2 * (sk_int)((rc - 1) % distinct);
    CHECK(npk_enter(x, k + 1, k) == SK_OK, "enter");
    stop();
    m2_fig f;
    npk_figures(x, &f);
    unsigned whp = pack_width(f.W);
    CHECK(f.W == 2 * (distinct < rc ? distinct : rc), "W = %u", f.W);
    int narrow = whp < whb;
    CHECK(NEV == 2 + narrow, "the pack: %d events, not %d", NEV, 2 + narrow);
    if (NEV == 2 + narrow) {
        CHECK(EV[0].kind == E_FREE && EV[0].old == table, "the pack: (h) freed first");
        CHECK(EV[1].kind == E_REALLOC && EV[1].old == list && EV[1].size == (size_t)f.W * f.w / 8,
              "the pack: (l) shrunk to W w/8 = %u bytes (%zu)", f.W * f.w / 8, EV[1].size);
        if (narrow)
            CHECK(EV[2].kind == E_REALLOC && EV[2].old == cells && EV[2].size == bound * whp / 8,
                  "the pack: the cells shrunk to %zu bytes", EV[2].size);
    }
    return x;
}

static npk_class *packed_result(npk_class *a, npk_class *b, uint32_t n, int mul) {
    size_t rc = (size_t)n * n, bound = 2 * rc;
    unsigned t = tbits_of(bound), whb = build_width(t);
    start();
    npk_class *c = npk_create(n, n, 6);
    int e0 = NEV;
    void *table = NEV >= 3 ? EV[2].p : 0;
    CHECK((mul ? npk_mmul(a, b, c) : npk_madd(a, b, c)) == SK_OK, "op");
    stop();
    m2_fig f;
    npk_figures(c, &f);
    int narrow = pack_width(f.W) < whb;
    int want = 4 + (mul ? 2 : 0) + 2 + narrow;
    CHECK(e0 == 4 && NEV == want, "%s n=%u: %d events, not %d", mul ? "mmul" : "madd", n, NEV, want);
    if (NEV == want && mul) {
        CHECK(EV[4].kind == E_MALLOC && EV[4].size == (size_t)n * 24, "mmul: the row buffer, n x 24 = %zu bytes",
              EV[4].size);
        CHECK(EV[5].kind == E_FREE && EV[5].old == EV[4].p, "mmul: the row buffer freed");
        CHECK(EV[6].kind == E_FREE && EV[6].old == table, "mmul: freed before the pack, which frees (h)");
    }
    if (NEV == want && !mul) CHECK(EV[4].kind == E_FREE && EV[4].old == table, "madd: the pack frees (h) first");
    (void)bound;
    return c;
}

static void packed_destroy(npk_class *x) {
    start();
    npk_destroy(x);
    stop();
    int frees = 0;
    for (int i = 0; i < NEV; i++) frees += EV[i].kind == E_FREE && EV[i].old;
    CHECK(NEV == 4 && frees == 3, "destroy: %d events, %d frees (the cells, (l), the header)", NEV, frees);
}

#else
static void control(uint32_t n) {
    size_t rc = (size_t)n * n;
    nctl_class *x[3];
    for (int m = 0; m < 3; m++) {
        start();
        x[m] = nctl_create(n, n);
        stop();
        CHECK(NEV == 2 && EV[0].kind == E_CALLOC && EV[1].kind == E_MALLOC && EV[1].size == 16 * rc,
              "Control create: the header and 16 rc bytes of cells");
    }
    start();
    for (size_t i = 0; i < rc; i++) {
        nctl_enter(x[0], (sk_int)i + 1, 3);
        nctl_enter(x[1], 2, (sk_int)i + 1);
    }
    nctl_mmul(x[0], x[1], x[2]);
    stop();
    CHECK(NEV == 0, "Control enter and mmul: %d events", NEV);
    for (int m = 0; m < 3; m++) {
        start();
        nctl_destroy(x[m]);
        stop();
        CHECK(NEV == 2, "Control destroy: 2 frees");
    }
}
#endif

int main(void) {
#ifdef ALLOC_CONTROL
    static const uint32_t cn[] = {1, 2, 8, 9, 16};
    for (unsigned i = 0; i < sizeof cn / sizeof *cn; i++) control(cn[i]);
#else
    /* sizes at every build width (8: n <= 8; 16: up to 128; 32: 129 and up) and value counts at every packed one */
    static const uint32_t ns[] = {1, 2, 8, 9, 16, 129};
    for (unsigned i = 0; i < sizeof ns / sizeof *ns; i++) {
        uint32_t n = ns[i];
        size_t rc = (size_t)n * n;
        uint32_t counts[] = {1, 40, 1000, (uint32_t)rc};
        for (unsigned j = 0; j < 4; j++) {
            uint32_t k = counts[j] < rc ? counts[j] : (uint32_t)rc;
            npk_class *x[3];
            x[0] = packed_input(n, k, 0);
            acct_check(x, 1, "after A's pack");
            x[1] = packed_input(n, k, 1);
            acct_check(x, 2, "after B's pack");
            x[2] = packed_result(x[0], x[1], n, 0);
            acct_check(x, 3, "after madd");
            packed_destroy(x[2]);
            if (n <= 16) {
                x[2] = packed_result(x[0], x[1], n, 1);
                acct_check(x, 3, "after mmul");
                packed_destroy(x[2]);
            }
            packed_destroy(x[0]);
            packed_destroy(x[1]);
#ifdef M2_MEM
            CHECK(m2_acct_live == 0, "everything freed: the accounted total is %llu", (unsigned long long)m2_acct_live);
#endif
        }
    }
#endif
    printf("alloc: %d checks, %d failures\n", CHECKS, FAILS);
    return FAILS != 0;
}
