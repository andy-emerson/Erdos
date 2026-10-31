/* m4t.h: shared by round m4's unit, differential and allocation tests (test/DECISIONS-m4.md; test build), from m3t.h
 * (unchanged, m3's): an exact reference with 128-bit integers and its own Euclid gcd (independent of the 08 module), a
 * PRNG, a row table that drives TC, Hash-V-pack-4 (mh4.c) and, as the reference row, Hash-V-pack (m2's mh2.c,
 * unchanged) through one interface, a job runner for the four job sets, and the checks by value that take the place
 * of m3's checks of mh2.c's layout (slots, list order, handles, widths) for Hash-V-pack-4.  The factored rows (mh3.c)
 * are m3's and are not driven here (m3's suites test them, unchanged). */
#ifndef M4T_H
#define M4T_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m3.h" /* first: m2.h, m1.h, r3.h (round 3's cases); the row table keeps m3's fields */
#include "mh4.h" /* and mh2.h */
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

/* ----------------------------------------------------------------------------------------- rows */

typedef struct row {
    const char *name;
    int hash, fac, coprime; /* hash: a hash row (Hash-V-pack or factored); fac: a factored row; coprime: Pack-coprime */
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
    /* factored rows only */
    void (*ffigures)(const void *, m3_ffig *);
    sk_status (*fcounters)(const void *, m3_fcount *, m3_acount *);
    sk_status (*t_nfac)(const void *, uint32_t, uint32_t *);
    sk_status (*t_fac)(const void *, uint32_t, uint32_t, sk_int *, unsigned *);
    sk_status (*t_piece)(const void *, uint32_t, sk_int *);
    sk_status (*t_coprime)(const void *, const char **);
    sk_status (*t_corrupt_fac)(void *, uint32_t, uint32_t, sk_int, unsigned);
    sk_status (*t_force_general)(void *, int);
    sk_status (*t_known_off)(void *, unsigned);
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
    }                                                                                                             \
    static sk_status P##w_t_force_general(void *x, int on) { return P##t_force_general((P##class *)x, on); }      \
    static sk_status P##w_t_known_off(void *x, unsigned m) { return P##t_known_off((P##class *)x, m); }
#define FWRAP(P)                                                                                                  \
    static void P##w_ffigures(const void *x, m3_ffig *f) { P##ffigures((const P##class *)x, f); }                 \
    static sk_status P##w_fcounters(const void *x, m3_fcount *f, m3_acount *a) {                                  \
        return P##fcounters((const P##class *)x, f, a);                                                           \
    }                                                                                                             \
    static sk_status P##w_t_nfac(const void *x, uint32_t k, uint32_t *n) {                                        \
        return P##t_nfac((const P##class *)x, k, n);                                                              \
    }                                                                                                             \
    static sk_status P##w_t_fac(const void *x, uint32_t k, uint32_t j, sk_int *q, unsigned *e) {                  \
        return P##t_fac((const P##class *)x, k, j, q, e);                                                         \
    }                                                                                                             \
    static sk_status P##w_t_piece(const void *x, uint32_t i, sk_int *q) {                                         \
        return P##t_piece((const P##class *)x, i, q);                                                             \
    }                                                                                                             \
    static sk_status P##w_t_coprime(const void *x, const char **w) { return P##t_coprime((const P##class *)x, w); } \
    static sk_status P##w_t_corrupt_fac(void *x, uint32_t k, uint32_t j, sk_int q, unsigned e) {                  \
        return P##t_corrupt_fac((P##class *)x, k, j, q, e);                                                       \
    }
HWRAP(mhv4_)
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
static sk_status tcw_t_force_general(void *x, int on) { return tcm_t_force_general((tcm_class *)x, on); }
static sk_status tcw_t_known_off(void *x, unsigned m) { return tcm_t_known_off((tcm_class *)x, m); }

#define HCOMMON(P)                                                                                                \
    P##w_create, P##w_destroy, P##w_enter, {P##w_madd, P##w_mmul}, P##w_read, P##w_memory, P##w_counters,          \
        P##w_overflow, P##w_figures, P##w_storage, P##w_audit, P##w_t_whole, P##w_t_field, P##w_t_corrupt_field,   \
        P##w_t_corrupt_slot, NULL
#define FCOMMON(P)                                                                                                \
    P##w_ffigures, P##w_fcounters, P##w_t_nfac, P##w_t_fac, P##w_t_piece, P##w_t_coprime, P##w_t_corrupt_fac
/* the rows: TC, Hash-V-pack-4 (the row under test), and Hash-V-pack (mh2.c: the reference row, whose layout checks are
 * m3's) */
enum { NROWS = 3, R_TC = 0, R_PACK4 = 1, R_PACK = 2 };
static const row ROWS[NROWS] = {
    {"TC", 0, 0, 0, tcw_create, tcw_destroy, tcw_enter, {tcw_madd, tcw_mmul}, tcw_read, tcw_memory, tcw_counters,
     tcw_overflow, NULL, NULL, NULL, NULL, NULL, NULL, NULL, tcw_audit, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     tcw_t_force_general, tcw_t_known_off},
    {"Hash-V-pack-4", 1, 0, 0, HCOMMON(mhv4_), NULL, NULL, NULL, NULL, NULL, NULL, NULL, mhv4_w_t_force_general,
     mhv4_w_t_known_off},
    {"Hash-V-pack", 1, 0, 0, HCOMMON(mhvp_), NULL, NULL, NULL, NULL, NULL, NULL, NULL, mhvp_w_t_force_general,
     mhvp_w_t_known_off},
};
/* is R Hash-V-pack-4 (its own layout rules: the bound 2RK, no widening, lists narrowed at the pack) */
static inline int is_v4(const row *R) { return R == &ROWS[R_PACK4]; }

/* the build table's size and the build handle width for n x n cells: Hash-V-pack-4's bound 2n^2, mh2.c's 2n^2 + 2 */
static inline uint32_t row_T(const row *R, uint32_t n) {
    uint64_t kb = 2ull * n * n + (is_v4(R) ? 0 : 2), tf = 1;
    while (tf < kb) tf <<= 1;
    return (uint32_t)tf;
}
static inline unsigned row_whb(const row *R, uint32_t n) {
    unsigned t = 0;
    while ((1ull << t) < row_T(R, n)) t++;
    return t <= 7 ? 8 : t <= 15 ? 16 : 32;
}

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

static inline m3_ffig mat_ffig(const row *R, const void *m) {
    m3_ffig f;
    memset(&f, 0, sizeof f);
    if (R->ffigures) R->ffigures(m, &f);
    return f;
}

static inline m3_acount mat_ac(const row *R, const void *m) {
    m3_acount a;
    memset(&a, 0, sizeof a);
    if (R->fcounters) R->fcounters(m, NULL, &a);
    return a;
}

static inline m3_fcount mat_fc(const row *R, const void *m) {
    m3_fcount f;
    memset(&f, 0, sizeof f);
    if (R->fcounters) R->fcounters(m, &f, NULL);
    return f;
}

/* every audit of a job's live matrices passes; for Pack-coprime also the base pairwise coprime by all pairs */
static inline int job_audits(const job *j) {
    const char *why = NULL;
    for (int x = 0; x < M2_NMAT; x++) {
        if (!j->M[x]) continue;
        sk_status st = j->R->audit ? j->R->audit(j->M[x], &why) : j->R->tc_audit(j->M[x], &why);
        if (st == SK_OK && j->R->coprime) st = j->R->t_coprime(j->M[x], &why);
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

static int cmp_sk64(const void *a, const void *b) {
    sk_int x = *(const sk_int *)a, y = *(const sk_int *)b;
    return (x > y) - (x < y);
}

/* the checks by value (in place of m3's same_as_pack, which compares slots, list order and handles): two hash
 * matrices of rows R1 and R2 hold the same cells, field by field (the value each handle names, and the tag), and the
 * same whole numbers (each table's or list's used entries, as sets) */
static inline int same_by_value(const row *R1, const void *m1, const row *R2, const void *m2) {
    uint32_t n1 = 0, n2 = 0, nf1 = 0, nf2 = 0;
    sk_int *l1 = mat_vals(R1, m1, &n1), *l2 = mat_vals(R2, m2, &n2);
    uint32_t *f1 = mat_fields(R1, m1, &nf1), *f2 = mat_fields(R2, m2, &nf2);
    int ok = nf1 == nf2;
    for (uint32_t i = 0; i < nf1 && ok; i++)
        ok = (f1[i] >> 1) < n1 && (f2[i] >> 1) < n2 && l1[f1[i] >> 1] == l2[f2[i] >> 1] && (f1[i] & 1) == (f2[i] & 1);
    uint32_t k1 = 0, k2 = 0;
    for (uint32_t i = 0; i < n1; i++)
        if (l1[i] != -1) l1[k1++] = l1[i];
    for (uint32_t i = 0; i < n2; i++)
        if (l2[i] != -1) l2[k2++] = l2[i];
    qsort(l1, k1, sizeof(sk_int), cmp_sk64);
    qsort(l2, k2, sizeof(sk_int), cmp_sk64);
    ok = ok && k1 == k2 && !memcmp(l1, l2, sizeof(sk_int) * k1);
    free(l1);
    free(l2);
    free(f1);
    free(f2);
    return ok;
}

/* two tables (h) read slot by slot (-1 empty), as the sets of the values they hold */
static inline int same_values_held(const sk_int *h1, uint32_t T1, const sk_int *h2, uint32_t T2) {
    sk_int *a = malloc(sizeof(sk_int) * (T1 + 1)), *b = malloc(sizeof(sk_int) * (T2 + 1));
    uint32_t k1 = 0, k2 = 0;
    for (uint32_t s = 0; s < T1; s++)
        if (h1[s] != -1) a[k1++] = h1[s];
    for (uint32_t s = 0; s < T2; s++)
        if (h2[s] != -1) b[k2++] = h2[s];
    qsort(a, k1, sizeof(sk_int), cmp_sk64);
    qsort(b, k2, sizeof(sk_int), cmp_sk64);
    int ok = k1 == k2 && !memcmp(a, b, sizeof(sk_int) * k1);
    free(a);
    free(b);
    return ok;
}

/* the product of a whole number's factorization in (f), in 128 bits (0: an empty list) */
static inline i128 fac_product(const row *R, const void *m, uint32_t k, uint32_t *nent) {
    uint32_t n = 0;
    R->t_nfac(m, k, &n);
    i128 p = 1;
    for (uint32_t j = 0; j < n; j++) {
        sk_int q = 0;
        unsigned e = 0;
        R->t_fac(m, k, j, &q, &e);
        for (unsigned z = 0; z < e; z++) p *= q;
    }
    if (nent) *nent = n;
    return p;
}

/* is v prime (trial division in 128 bits, for small v only) */
static inline int is_prime_small(sk_int v) {
    if (v < 2) return 0;
    for (sk_int d = 2; d * d <= v; d++)
        if (v % d == 0) return 0;
    return 1;
}

#endif
