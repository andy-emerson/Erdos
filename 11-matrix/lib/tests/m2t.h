/* m2t.h: shared by round m2's unit and differential tests (spec 11-matrix v0.2 §15.7; test build), from m1t.h: an exact
 * reference with 128-bit integers and its own Euclid gcd (independent of the 08 module), a PRNG, a row table that
 * drives TC and the three hash rows through one interface, a job runner for the four job sets (the inputs entered,
 * then the steps, each result created just before its own step, the intermediates freed after their last use unless
 * kept for inspection), and the hash function written again here (independent of mh2.c) for the probing tests. */
#ifndef M2T_H
#define M2T_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mh2.h" /* first: m2.h, m1.h, r3.h (round 3's cases) */
#include "tcm.h"

static int failures, checks;

#define CHECK(cond)                                                                                               \
    do {                                                                                                          \
        checks++;                                                                                                 \
        if (!(cond)) {                                                                                            \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                       \
        }                                                                                                         \
    } while (0)

#define CHECK_EQ(a, b)                                                                                            \
    do {                                                                                                          \
        checks++;                                                                                                 \
        long long a_ = (long long)(a), b_ = (long long)(b);                                                       \
        if (a_ != b_) {                                                                                           \
            failures++;                                                                                           \
            fprintf(stderr, "FAIL %s:%d: %s == %lld, expected %s == %lld\n", __FILE__, __LINE__, #a, a_, #b, b_); \
        }                                                                                                         \
    } while (0)

/* ----------------------------------------------------------------------------------------- exact reference */

typedef __int128 i128;
typedef struct ref_q {
    i128 n, d;
} ref_q;

static inline i128 ref_abs(i128 a) { return a < 0 ? -a : a; }
static inline i128 ref_gcd(i128 a, i128 b) {
    a = ref_abs(a);
    b = ref_abs(b);
    while (b) {
        i128 t = a % b;
        a = b;
        b = t;
    }
    return a;
}
static inline ref_q ref_make(i128 n, i128 d) {
    if (d < 0) n = -n, d = -d;
    i128 g = ref_gcd(n, d);
    if (g == 0) g = 1;
    ref_q r = {n / g, d / g};
    if (r.n == 0) r.d = 1;
    return r;
}
static inline ref_q ref_add(ref_q a, ref_q b) { return ref_make(a.n * b.d + b.n * a.d, a.d * b.d); }
static inline ref_q ref_mul(ref_q a, ref_q b) { return ref_make(a.n * b.n, a.d * b.d); }

/* ----------------------------------------------------------------------------------------- PRNG */

static uint64_t rng_s = 88172645463325252ull;
static inline uint64_t rnd(void) {
    rng_s ^= rng_s << 13;
    rng_s ^= rng_s >> 7;
    rng_s ^= rng_s << 17;
    return rng_s;
}

static const sk_int D720[] = {1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 15, 16, 18, 20, 24, 30, 36, 40, 45, 48, 60, 72, 80, 90,
                              120, 144, 180, 240, 360, 720};

/* ----------------------------------------------------------------------------------------- the hash, again */

static inline uint32_t t_home(int64_t v, unsigned t) {
    return (uint32_t)(((uint64_t)v * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t));
}

/* ----------------------------------------------------------------------------------------- rows */

typedef struct row {
    const char *name;
    int hash, grow, pack;
    void *(*create)(uint32_t, uint32_t, int);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int);
    sk_status (*op[2])(void *, void *, void *); /* madd, mmul */
    sk_status (*read)(const void *, uint32_t, sk_int *, sk_int *);
    void (*memory)(const void *, sk_memory *);
    sk_status (*counters)(const void *, sk_counters *, r2_xcounters *);
    sk_status (*overflow)(const void *, sk_overflow *);
    /* hash rows only (NULL for TC) */
    void (*figures)(const void *, m2_fig *);
    sk_status (*storage)(const void *, m2_scount *, m2_ocount *);
    sk_status (*audit)(void *, const char **);
    sk_status (*t_whole)(const void *, uint32_t, sk_int *);
    sk_status (*t_field)(const void *, uint32_t, uint32_t *);
    sk_status (*t_corrupt_field)(void *, uint32_t, uint32_t);
    sk_status (*t_corrupt_slot)(void *, uint32_t, sk_int);
    sk_status (*tc_audit)(void *, const char **);
} row;

#define HWRAP(P)                                                                                                  \
    static void *P##w_create(uint32_t r, uint32_t c, int id) { return P##create(r, c, id); }                      \
    static void P##w_destroy(void *x) { P##destroy((P##class *)x); }                                              \
    static sk_status P##w_enter(void *x, sk_int m, sk_int d) { return P##enter((P##class *)x, m, d); }            \
    static sk_status P##w_madd(void *a, void *b, void *c) {                                                       \
        return P##madd((P##class *)a, (P##class *)b, (P##class *)c);                                              \
    }                                                                                                             \
    static sk_status P##w_mmul(void *a, void *b, void *c) {                                                       \
        return P##mmul((P##class *)a, (P##class *)b, (P##class *)c);                                              \
    }                                                                                                             \
    static sk_status P##w_read(const void *x, uint32_t i, sk_int *m, sk_int *d) {                                 \
        return P##read((const P##class *)x, i, m, d);                                                             \
    }                                                                                                             \
    static void P##w_memory(const void *x, sk_memory *r) { P##memory((const P##class *)x, r); }                   \
    static sk_status P##w_counters(const void *x, sk_counters *k, r2_xcounters *e) {                              \
        return P##counters((const P##class *)x, k, e);                                                            \
    }                                                                                                             \
    static sk_status P##w_overflow(const void *x, sk_overflow *o) { return P##overflow((const P##class *)x, o); } \
    static void P##w_figures(const void *x, m2_fig *f) { P##figures((const P##class *)x, f); }                    \
    static sk_status P##w_storage(const void *x, m2_scount *s, m2_ocount *o) {                                    \
        return P##storage((const P##class *)x, s, o);                                                             \
    }                                                                                                             \
    static sk_status P##w_audit(void *x, const char **w) { return P##audit((P##class *)x, w); }                   \
    static sk_status P##w_t_whole(const void *x, uint32_t s, sk_int *v) {                                         \
        return P##t_whole((const P##class *)x, s, v);                                                             \
    }                                                                                                             \
    static sk_status P##w_t_field(const void *x, uint32_t f, uint32_t *v) {                                       \
        return P##t_field((const P##class *)x, f, v);                                                             \
    }                                                                                                             \
    static sk_status P##w_t_corrupt_field(void *x, uint32_t f, uint32_t v) {                                      \
        return P##t_corrupt_field((P##class *)x, f, v);                                                           \
    }                                                                                                             \
    static sk_status P##w_t_corrupt_slot(void *x, uint32_t s, sk_int v) {                                         \
        return P##t_corrupt_slot((P##class *)x, s, v);                                                            \
    }
HWRAP(mhvf_)
HWRAP(mhvgm_)
HWRAP(mhvp_)

static void *tcw_create(uint32_t r, uint32_t c, int id) {
    (void)id;
    return tcm_create(r, c);
}
static void tcw_destroy(void *x) { tcm_destroy((tcm_class *)x); }
static sk_status tcw_enter(void *x, sk_int m, sk_int d) { return tcm_enter((tcm_class *)x, m, d); }
static sk_status tcw_madd(void *a, void *b, void *c) { return tcm_madd((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static sk_status tcw_mmul(void *a, void *b, void *c) { return tcm_mmul((tcm_class *)a, (tcm_class *)b, (tcm_class *)c); }
static sk_status tcw_read(const void *x, uint32_t i, sk_int *m, sk_int *d) {
    return tcm_read((const tcm_class *)x, i, m, d);
}
static void tcw_memory(const void *x, sk_memory *r) { tcm_memory((const tcm_class *)x, r); }
static sk_status tcw_counters(const void *x, sk_counters *k, r2_xcounters *e) {
    return tcm_counters((const tcm_class *)x, k, e);
}
static sk_status tcw_overflow(const void *x, sk_overflow *o) { return tcm_overflow((const tcm_class *)x, o); }
static sk_status tcw_audit(void *x, const char **w) { return tcm_audit((tcm_class *)x, w); }

#define HROW(name, g, p, P)                                                                                       \
    {name, 1, g, p, P##w_create, P##w_destroy, P##w_enter, {P##w_madd, P##w_mmul}, P##w_read, P##w_memory,         \
     P##w_counters, P##w_overflow, P##w_figures, P##w_storage, P##w_audit, P##w_t_whole, P##w_t_field,             \
     P##w_t_corrupt_field, P##w_t_corrupt_slot, NULL}
enum { NROWS = 4, R_TC = 0, R_FIX = 1, R_GROW = 2, R_PACK = 3 };
static const row ROWS[NROWS] = {
    {"TC", 0, 0, 0, tcw_create, tcw_destroy, tcw_enter, {tcw_madd, tcw_mmul}, tcw_read, tcw_memory, tcw_counters,
     tcw_overflow, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tcw_audit},
    HROW("Hash-V-fixed", 0, 0, mhvf_),
    HROW("Hash-V-grow-map", 1, 0, mhvgm_),
    HROW("Hash-V-pack", 0, 1, mhvp_),
};

/* ----------------------------------------------------------------------------------------- the job sets */

/* The four job sets (spec v0.2 §15.5): their inputs and steps.  A step: c = a op b (op 0 addition, 1 multiplication). */
enum { SET_ADD = 0, SET_MUL = 1, SET_SOP = 2, SET_POS = 3 };
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

/* One job on one row: inputs of n x n cells from pairs (m, d) as entered, row-major; each result created just before
 * its own step; the intermediates freed after the last step unless `keep`.  st: the first failing status. */
typedef struct job {
    const row *R;
    uint32_t n;
    int set;
    void *M[M2_NMAT];
    sk_status st;
    const char *where;
    int last; /* the id of the last result created */
} job;

static inline job job_run(const row *R, int set, uint32_t n, const sk_int *const *in, int keep) {
    job j = {R, n, set, {NULL}, SK_OK, NULL, -1};
    for (int x = 0; x < NIN[set] && !j.st; x++) {
        j.M[x] = R->create(n, n, x);
        for (uint32_t i = 0; i < n * n && !j.st; i++) {
            j.st = R->enter(j.M[x], in[x][2 * i], in[x][2 * i + 1]);
            j.where = "enter";
        }
    }
    for (int s = 0; s < NSTEP[set] && !j.st; s++) {
        const step *t = &STEPS[set][s];
        j.M[t->c] = R->create(n, n, t->c);
        j.last = t->c;
        j.st = R->op[t->op](j.M[t->a], j.M[t->b], j.M[t->c]);
        j.where = "op";
    }
    if (!keep)
        for (int x = M2_X1; x <= M2_X2; x++)
            if (j.M[x]) {
                R->destroy(j.M[x]);
                j.M[x] = NULL;
            }
    return j;
}

static inline void job_free(job *j) {
    for (int x = 0; x < M2_NMAT; x++)
        if (j->M[x]) j->R->destroy(j->M[x]);
    memset(j->M, 0, sizeof j->M);
}

/* the exact value of every matrix of a job: out[id] (n^2 each; NULL for a matrix the set lacks) */
static inline void job_ref(int set, uint32_t n, const sk_int *const *in, ref_q *out[M2_NMAT]) {
    for (int x = 0; x < M2_NMAT; x++) out[x] = NULL;
    for (int x = 0; x < NIN[set]; x++) {
        out[x] = malloc(sizeof(ref_q) * n * n);
        for (uint32_t c = 0; c < n * n; c++) out[x][c] = ref_make(in[x][2 * c], in[x][2 * c + 1]);
    }
    for (int s = 0; s < NSTEP[set]; s++) {
        const step *t = &STEPS[set][s];
        ref_q *a = out[t->a], *b = out[t->b], *c = malloc(sizeof(ref_q) * n * n);
        for (uint32_t i = 0; i < n; i++)
            for (uint32_t k = 0; k < n; k++) {
                uint32_t q = i * n + k;
                if (!t->op) {
                    c[q] = ref_add(a[q], b[q]);
                    continue;
                }
                ref_q acc = {0, 1};
                for (uint32_t u = 0; u < n; u++) acc = ref_add(acc, ref_mul(a[i * n + u], b[u * n + k]));
                c[q] = acc;
            }
        out[t->c] = c;
    }
}

static inline void ref_free(ref_q *out[M2_NMAT]) {
    for (int x = 0; x < M2_NMAT; x++) free(out[x]);
}

/* one matrix's cells against the exact reference: the count of mismatches */
static inline int mat_check(const row *R, const void *m, uint32_t n, const ref_q *want) {
    int bad = 0;
    for (uint32_t c = 0; c < n * n; c++) {
        sk_int a = 0, d = 0;
        if (R->read(m, c, &a, &d) != SK_OK || (i128)a != want[c].n || (i128)d != want[c].d) bad++;
    }
    return bad;
}

/* every live matrix of a job against the reference */
static inline int job_check(const job *j, ref_q *const want[M2_NMAT]) {
    int bad = 0;
    for (int x = 0; x < M2_NMAT; x++)
        if (j->M[x] && want[x]) bad += mat_check(j->R, j->M[x], j->n, want[x]);
    return bad;
}

/* a case taken, in a phase (entry, add or mul), summed over the live matrices */
static inline uint64_t job_case(const job *j, int phase, int kase) {
    uint64_t t = 0;
    for (int x = 0; x < M2_NMAT; x++) {
        if (!j->M[x]) continue;
        r2_xcounters c;
        j->R->counters(j->M[x], NULL, &c);
        t += c.c[phase][SK_CORE].cases[kase];
    }
    return t;
}

static inline m2_ocount mat_opc(const row *R, const void *m) {
    m2_ocount o;
    memset(&o, 0, sizeof o);
    if (R->storage) R->storage(m, NULL, &o);
    return o;
}

static inline m2_scount mat_store(const row *R, const void *m) {
    m2_scount s;
    memset(&s, 0, sizeof s);
    if (R->storage) R->storage(m, &s, NULL);
    return s;
}

static inline m2_fig mat_fig(const row *R, const void *m) {
    m2_fig f;
    memset(&f, 0, sizeof f);
    if (R->figures) R->figures(m, &f);
    return f;
}

/* every audit of a job's live matrices passes */
static inline int job_audits(const job *j) {
    const char *why = NULL;
    for (int x = 0; x < M2_NMAT; x++) {
        if (!j->M[x]) continue;
        sk_status st = j->R->audit ? j->R->audit(j->M[x], &why) : j->R->tc_audit(j->M[x], &why);
        if (st != SK_OK) {
            fprintf(stderr, "  audit (%s, matrix %d): %s\n", j->R->name, x, why ? why : "?");
            return 0;
        }
    }
    return 1;
}

/* the values a hash matrix holds, by slot (table) or position (list): v[0 .. *nv - 1] */
static inline sk_int *mat_vals(const row *R, const void *m, uint32_t *nv) {
    m2_fig f = mat_fig(R, m);
    uint32_t k = f.packed ? f.W : f.T;
    sk_int *v = malloc(sizeof(sk_int) * (k + 1));
    for (uint32_t s = 0; s < k; s++) R->t_whole(m, s, &v[s]);
    *nv = k;
    return v;
}

static inline uint32_t *mat_fields(const row *R, const void *m, uint32_t *nf) {
    m2_fig f = mat_fig(R, m);
    uint32_t *v = malloc(sizeof(uint32_t) * (f.nf + 1));
    for (uint32_t i = 0; i < f.nf; i++) R->t_field(m, i, &v[i]);
    *nf = f.nf;
    return v;
}

/* Hash-V-pack's matrix against Hash-V-fixed's of the same values (v0.2 §15.7 item 2): its list is the fixed table's
 * used slots in slot order, and each of its fields is the fixed field's handle mapped to that position, tag kept */
static inline int pack_matches_fixed(const void *fixed, const void *packed) {
    uint32_t T = 0, W = 0, nf1 = 0, nf2 = 0;
    sk_int *tab = mat_vals(&ROWS[R_FIX], fixed, &T), *list = mat_vals(&ROWS[R_PACK], packed, &W);
    uint32_t *f1 = mat_fields(&ROWS[R_FIX], fixed, &nf1), *f2 = mat_fields(&ROWS[R_PACK], packed, &nf2);
    uint32_t *pos = malloc(sizeof(uint32_t) * (T + 1));
    int ok = nf1 == nf2 && mat_fig(&ROWS[R_PACK], packed).packed;
    uint32_t k = 0;
    for (uint32_t s = 0; s < T && ok; s++) {
        if (tab[s] == -1) continue;
        if (k >= W || list[k] != tab[s]) ok = 0;
        pos[s] = k++;
    }
    ok = ok && k == W;
    for (uint32_t i = 0; i < nf1 && ok; i++) ok = f2[i] == 2 * pos[f1[i] >> 1] + (f1[i] & 1);
    free(tab);
    free(list);
    free(f1);
    free(f2);
    free(pos);
    return ok;
}

#endif
