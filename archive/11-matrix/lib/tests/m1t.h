/* m1t.h: shared by the matrix round's unit, differential and allocation tests (spec 11-matrix v0.1 §10), from round 3's
 * r3t.h: an exact reference with 128-bit integers and its own Euclid gcd (independent of the 08 module), a PRNG, a row
 * table that drives TC and the five hash rows through one interface, a job runner (create A, B, C; enter; tau; the
 * operation), and the hash function written again here (independent of mh.c) for the probing tests. */
#ifndef M1T_H
#define M1T_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mh.h" /* first: m1.h, r3.h (round 3's cases) */
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

/* ----------------------------------------------------------------------------------------- the hash, again */

static inline uint32_t t_home(int64_t v, unsigned t) {
    return (uint32_t)(((uint64_t)v * UINT64_C(0x9E3779B97F4A7C15)) >> (64 - t));
}

/* ----------------------------------------------------------------------------------------- rows */

typedef struct row {
    const char *name;
    int hash, m, grow, map;
    void *(*create)(uint32_t, uint32_t, int);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int);
    sk_status (*tau_build)(void *, void *, void *);
    void (*tau_free)(void *);
    uint64_t (*tau_bytes)(const void *);
    sk_status (*madd)(void *, void *, void *);
    sk_status (*mmul)(void *, void *, void *);
    sk_status (*read)(const void *, uint32_t, sk_int *, sk_int *);
    void (*memory)(const void *, sk_memory *);
    sk_status (*counters)(const void *, sk_counters *, r2_xcounters *);
    sk_status (*overflow)(const void *, sk_overflow *);
    /* hash rows only (NULL for TC) */
    void (*figures)(const void *, m1_fig *);
    sk_status (*storage)(const void *, m1_scount *, m1_ocount *);
    sk_status (*audit)(void *, const char **);
    sk_status (*audit_tau)(void *, void *, void *, const char **);
    sk_status (*t_whole)(const void *, uint32_t, sk_int *);
    sk_status (*t_field)(const void *, uint32_t, uint32_t *);
    sk_status (*t_tau)(const void *, uint32_t, uint32_t *);
    sk_status (*t_corrupt_field)(void *, uint32_t, uint32_t);
    sk_status (*t_corrupt_slot)(void *, uint32_t, sk_int);
    sk_status (*tc_audit)(void *, const char **);
} row;

#define HWRAP(P)                                                                                                  \
    static void *P##w_create(uint32_t r, uint32_t c, int role) { return P##create(r, c, role); }                  \
    static void P##w_destroy(void *x) { P##destroy((P##class *)x); }                                              \
    static sk_status P##w_enter(void *x, sk_int m, sk_int d) { return P##enter((P##class *)x, m, d); }            \
    static sk_status P##w_tau_build(void *a, void *b, void *c) {                                                  \
        return P##tau_build((P##class *)a, (P##class *)b, (P##class *)c);                                         \
    }                                                                                                             \
    static void P##w_tau_free(void *c) { P##tau_free((P##class *)c); }                                            \
    static uint64_t P##w_tau_bytes(const void *c) { return P##tau_bytes((const P##class *)c); }                   \
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
    static void P##w_figures(const void *x, m1_fig *f) { P##figures((const P##class *)x, f); }                    \
    static sk_status P##w_storage(const void *x, m1_scount *s, m1_ocount *o) {                                    \
        return P##storage((const P##class *)x, s, o);                                                             \
    }                                                                                                             \
    static sk_status P##w_audit(void *x, const char **w) { return P##audit((P##class *)x, w); }                   \
    static sk_status P##w_audit_tau(void *a, void *b, void *c, const char **w) {                                  \
        return P##audit_tau((P##class *)a, (P##class *)b, (P##class *)c, w);                                      \
    }                                                                                                             \
    static sk_status P##w_t_whole(const void *x, uint32_t s, sk_int *v) {                                         \
        return P##t_whole((const P##class *)x, s, v);                                                             \
    }                                                                                                             \
    static sk_status P##w_t_field(const void *x, uint32_t f, uint32_t *v) {                                       \
        return P##t_field((const P##class *)x, f, v);                                                             \
    }                                                                                                             \
    static sk_status P##w_t_tau(const void *x, uint32_t k, uint32_t *v) {                                         \
        return P##t_tau((const P##class *)x, k, v);                                                               \
    }                                                                                                             \
    static sk_status P##w_t_corrupt_field(void *x, uint32_t f, uint32_t v) {                                      \
        return P##t_corrupt_field((P##class *)x, f, v);                                                           \
    }                                                                                                             \
    static sk_status P##w_t_corrupt_slot(void *x, uint32_t s, sk_int v) {                                         \
        return P##t_corrupt_slot((P##class *)x, s, v);                                                            \
    }
HWRAP(mhvf_)
HWRAP(mhmf_)
HWRAP(mhvg_)
HWRAP(mhmg_)
HWRAP(mhvgm_)

static void *tcw_create(uint32_t r, uint32_t c, int role) {
    (void)role;
    return tcm_create(r, c);
}
static void tcw_destroy(void *x) { tcm_destroy((tcm_class *)x); }
static sk_status tcw_enter(void *x, sk_int m, sk_int d) { return tcm_enter((tcm_class *)x, m, d); }
static sk_status tcw_tau_build(void *a, void *b, void *c) {
    (void)a, (void)b, (void)c;
    return SK_OK;
}
static void tcw_tau_free(void *c) { (void)c; }
static uint64_t tcw_tau_bytes(const void *c) {
    (void)c;
    return 0;
}
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

#define HROW(name, m, g, mp, P)                                                                                   \
    {name, 1, m, g, mp, P##w_create, P##w_destroy, P##w_enter, P##w_tau_build, P##w_tau_free, P##w_tau_bytes,      \
     P##w_madd, P##w_mmul, P##w_read, P##w_memory, P##w_counters, P##w_overflow, P##w_figures, P##w_storage,       \
     P##w_audit, P##w_audit_tau, P##w_t_whole, P##w_t_field, P##w_t_tau, P##w_t_corrupt_field,                     \
     P##w_t_corrupt_slot, NULL}
enum { NROWS = 6 };
static const row ROWS[NROWS] = {
    {"TC", 0, 0, 0, 0, tcw_create, tcw_destroy, tcw_enter, tcw_tau_build, tcw_tau_free, tcw_tau_bytes, tcw_madd,
     tcw_mmul, tcw_read, tcw_memory, tcw_counters, tcw_overflow, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
     tcw_audit},
    HROW("Hash-V-fixed", 0, 0, 0, mhvf_),
    HROW("Hash-M-fixed", 1, 0, 0, mhmf_),
    HROW("Hash-V-grow", 0, 1, 0, mhvg_),
    HROW("Hash-M-grow", 1, 1, 0, mhmg_),
    HROW("Hash-V-grow-map", 0, 1, 1, mhvgm_),
};

/* ----------------------------------------------------------------------------------------- a job */

/* One job on one row: A and B of n x n cells from pairs (m, d) as entered, row-major; op 0 addition, 1 multiplication.
 * The matrices are kept for inspection (job_free).  st: the first failing status, or SK_OK. */
typedef struct job {
    const row *R;
    uint32_t n;
    void *M[3];
    sk_status st;
    const char *where;
} job;

static inline job job_run(const row *R, int op, uint32_t n, const sk_int *A, const sk_int *B, int keep_tau) {
    job j = {R, n, {NULL, NULL, NULL}, SK_OK, NULL};
    for (int r = 0; r < 3; r++) j.M[r] = R->create(n, n, r);
    for (uint32_t i = 0; i < n * n && !j.st; i++) {
        j.st = R->enter(j.M[0], A[2 * i], A[2 * i + 1]);
        j.where = "enter A";
    }
    for (uint32_t i = 0; i < n * n && !j.st; i++) {
        j.st = R->enter(j.M[1], B[2 * i], B[2 * i + 1]);
        j.where = "enter B";
    }
    if (!j.st) {
        j.st = R->tau_build(j.M[0], j.M[1], j.M[2]);
        j.where = "tau";
    }
    if (!j.st) {
        j.st = op ? R->mmul(j.M[0], j.M[1], j.M[2]) : R->madd(j.M[0], j.M[1], j.M[2]);
        j.where = "op";
    }
    if (!keep_tau) R->tau_free(j.M[2]);
    return j;
}

static inline void job_free(job *j) {
    for (int r = 0; r < 3; r++)
        if (j->M[r]) j->R->destroy(j->M[r]);
    j->M[0] = j->M[1] = j->M[2] = NULL;
}

/* the exact C of a job */
static inline ref_q *job_ref(int op, uint32_t n, const sk_int *A, const sk_int *B) {
    ref_q *out = malloc(sizeof(ref_q) * n * n);
    for (uint32_t i = 0; i < n; i++)
        for (uint32_t k = 0; k < n; k++) {
            uint32_t c = i * n + k;
            if (!op) {
                out[c] = ref_add(ref_make(A[2 * c], A[2 * c + 1]), ref_make(B[2 * c], B[2 * c + 1]));
                continue;
            }
            ref_q s = {0, 1};
            for (uint32_t t = 0; t < n; t++) {
                uint32_t x = i * n + t, y = t * n + k;
                s = ref_add(s, ref_mul(ref_make(A[2 * x], A[2 * x + 1]), ref_make(B[2 * y], B[2 * y + 1])));
            }
            out[c] = s;
        }
    return out;
}

/* C's cells against the exact reference: the count of mismatches */
static inline int job_check(const job *j, const ref_q *want) {
    int bad = 0;
    for (uint32_t c = 0; c < j->n * j->n; c++) {
        sk_int m = 0, d = 0;
        if (j->R->read(j->M[2], c, &m, &d) != SK_OK || (i128)m != want[c].n || (i128)d != want[c].d) bad++;
    }
    return bad;
}

/* a counter summed over the three matrices: a case taken, in a phase (entry, add or mul) */
static inline uint64_t job_case(const job *j, int phase, int kase) {
    uint64_t t = 0;
    for (int r = 0; r < 3; r++) {
        r2_xcounters x;
        j->R->counters(j->M[r], NULL, &x);
        t += x.c[phase][SK_CORE].cases[kase];
    }
    return t;
}

static inline m1_ocount job_opc(const job *j) {
    m1_ocount o;
    memset(&o, 0, sizeof o);
    if (j->R->storage) j->R->storage(j->M[2], NULL, &o);
    return o;
}

static inline m1_scount job_store(const job *j, int r) {
    m1_scount s;
    memset(&s, 0, sizeof s);
    if (j->R->storage) j->R->storage(j->M[r], &s, NULL);
    return s;
}

static inline m1_fig job_fig(const job *j, int r) {
    m1_fig f;
    memset(&f, 0, sizeof f);
    if (j->R->figures) j->R->figures(j->M[r], &f);
    return f;
}

/* every audit of a job's matrices passes (hash rows: each store; TC: each matrix's pairs) */
static inline int job_audits(const job *j) {
    const char *why = NULL;
    for (int r = 0; r < 3; r++) {
        sk_status st = j->R->audit ? j->R->audit(j->M[r], &why) : j->R->tc_audit(j->M[r], &why);
        if (st != SK_OK) {
            fprintf(stderr, "  audit (%s, matrix %d): %s\n", j->R->name, r, why ? why : "?");
            return 0;
        }
    }
    return 1;
}

#endif
