/* r3t.h: shared by round 3's unit, differential and allocation tests (spec v0.3 §12.3; test build only), from round
 * 2's r2t.h (unchanged): an exact reference with 128-bit integers and its own Euclid gcd (independent of the 08
 * module), a PRNG, a row table that drives TC, Q32-index and Q32-hash through one interface, the order of cases of
 * v0.2 §9 and of the amendment's §17.3 (v0.3) computed from magnitudes and effective signs, counter sums, and Q32-hash's hash function written again here
 * (independent of q32h.c) for the probing tests. */
#ifndef R3T_H
#define R3T_H

#ifndef SK_AUDIT
#error "the tests need the test build (-DSK_COUNT -DSK_AUDIT)"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "q32h.h" /* first: r3.h, round 3's cases (spec v0.3 §17) */
#include "q32i.h"
#include "tc.h"

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
/* x op y for values whose numerators and denominators are below 2^63: every product fits 128 bits */
static inline ref_q ref_op(int op, ref_q a, ref_q b) {
    switch (op) {
    case SK_PH_ADD: return ref_make(a.n * b.d + b.n * a.d, a.d * b.d);
    case SK_PH_SUB: return ref_make(a.n * b.d - b.n * a.d, a.d * b.d);
    case SK_PH_MUL: return ref_make(a.n * b.n, a.d * b.d);
    default: return ref_make(a.n * b.d, a.d * b.n);
    }
}
static inline bool ref_fits(ref_q a) { return ref_abs(a.n) <= INT64_MAX && a.d <= INT64_MAX; }

/* ----------------------------------------------------------------------------------------- PRNG */

static uint64_t rng_s = 88172645463325252ull;
static inline uint64_t rnd(void) {
    rng_s ^= rng_s << 13;
    rng_s ^= rng_s >> 7;
    rng_s ^= rng_s << 17;
    return rng_s;
}
static inline uint64_t rndn(uint64_t n) { return rnd() % n; }

/* ----------------------------------------------------------------------------------------- rows */

typedef struct row {
    const char *name;
    int isq; /* 0 TC, 1 Q32-index, 2 Q32-hash */
    void *(*create)(void);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int, sk_handle *);
    sk_status (*op[5])(void *, sk_handle, sk_handle, sk_handle *); /* indexed by sk_phase 1..4 */
    sk_status (*read)(const void *, sk_handle, sk_int *, sk_int *);
    uint32_t (*size)(const void *);
    sk_status (*counters)(const void *, sk_counters *, r2_xcounters *);
    sk_status (*audit)(void *, const char **);
    sk_status (*audit_changed)(void *, const char **);
    sk_status (*force_general)(void *, int);
    sk_status (*known_off)(void *, unsigned);
    void (*memory)(const void *, sk_memory *);
    sk_status (*overflow)(const void *, sk_overflow *);
    /* the Q32 rows only (NULL for TC) */
    uint32_t (*wholes)(const void *);
    sk_status (*t_rat)(const void *, sk_handle, int *, uint32_t *, uint32_t *);
    sk_status (*t_whole)(const void *, uint32_t, sk_int *);
    sk_status (*storage)(const void *, r3_scounters *);
    sk_status (*corrupt_rat)(void *, sk_handle, int, uint32_t, uint32_t);
    sk_status (*corrupt_whole)(void *, uint32_t, sk_int);
} row;

#define WRAP(P, T)                                                                                                \
    static void *P##w_create(void) { return P##create(); }                                                        \
    static void P##w_destroy(void *c) { P##destroy((T *)c); }                                                    \
    static sk_status P##w_enter(void *c, sk_int m, sk_int d, sk_handle *o) { return P##enter((T *)c, m, d, o); } \
    static sk_status P##w_add(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##add((T *)c, x, y, o); } \
    static sk_status P##w_sub(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##sub((T *)c, x, y, o); } \
    static sk_status P##w_mul(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##mul((T *)c, x, y, o); } \
    static sk_status P##w_div(void *c, sk_handle x, sk_handle y, sk_handle *o) { return P##div((T *)c, x, y, o); } \
    static sk_status P##w_read(const void *c, sk_handle x, sk_int *m, sk_int *d) {                                \
        return P##read((const T *)c, x, m, d);                                                                    \
    }                                                                                                             \
    static uint32_t P##w_size(const void *c) { return P##size((const T *)c); }                                    \
    static sk_status P##w_counters(const void *c, sk_counters *b, r2_xcounters *x) {                              \
        return P##counters((const T *)c, b, x);                                                                   \
    }                                                                                                             \
    static sk_status P##w_audit(void *c, const char **w) { return P##audit((T *)c, w); }                          \
    static sk_status P##w_audit_changed(void *c, const char **w) { return P##audit_changed((T *)c, w); }          \
    static sk_status P##w_force_general(void *c, int on) { return P##t_force_general((T *)c, on); }              \
    static sk_status P##w_known_off(void *c, unsigned m) { return P##t_known_off((T *)c, m); }                   \
    static void P##w_memory(const void *c, sk_memory *m) { P##memory((const T *)c, m); }                          \
    static sk_status P##w_overflow(const void *c, sk_overflow *o) { return P##overflow((const T *)c, o); }

#define QWRAP(P, T)                                                                                               \
    static uint32_t P##w_wholes(const void *c) { return P##wholes((const T *)c); }                                 \
    static sk_status P##w_t_rat(const void *c, sk_handle x, int *n, uint32_t *i, uint32_t *j) {                   \
        return P##t_rat((const T *)c, x, n, i, j);                                                                \
    }                                                                                                             \
    static sk_status P##w_t_whole(const void *c, uint32_t k, sk_int *v) { return P##t_whole((const T *)c, k, v); } \
    static sk_status P##w_storage(const void *c, r3_scounters *s) { return P##storage((const T *)c, s); }          \
    static sk_status P##w_corrupt_rat(void *c, sk_handle x, int n, uint32_t i, uint32_t j) {                      \
        return P##t_corrupt_rat((T *)c, x, n, i, j);                                                              \
    }                                                                                                             \
    static sk_status P##w_corrupt_whole(void *c, uint32_t k, sk_int v) { return P##t_corrupt_whole((T *)c, k, v); }

WRAP(q32i_, q32i_class)
WRAP(q32h_, q32h_class)
WRAP(tc_, tc_class)
QWRAP(q32i_, q32i_class)
QWRAP(q32h_, q32h_class)

#define ROWDEF(name, isq, P, Q)                                                                                   \
    {name, isq, P##w_create, P##w_destroy, P##w_enter, {NULL, P##w_add, P##w_sub, P##w_mul, P##w_div}, P##w_read, \
     P##w_size, P##w_counters, P##w_audit, P##w_audit_changed, P##w_force_general, P##w_known_off, P##w_memory,  \
     P##w_overflow, Q}
#define QFNS(P) P##w_wholes, P##w_t_rat, P##w_t_whole, P##w_storage, P##w_corrupt_rat, P##w_corrupt_whole
#define NOQ NULL, NULL, NULL, NULL, NULL, NULL

static const row ROW_TC = ROWDEF("TC", 0, tc_, NOQ);
static const row ROW_Q32I = ROWDEF("Q32-index", 1, q32i_, QFNS(q32i_));
static const row ROW_Q32H = ROWDEF("Q32-hash", 2, q32h_, QFNS(q32h_));
enum { NROWS = 3 };
static const row *const ROWS[NROWS] = {&ROW_TC, &ROW_Q32I, &ROW_Q32H};
static const row *const QROWS[2] = {&ROW_Q32I, &ROW_Q32H};

/* ----------------------------------------------------------------------------------------- Q32-hash's hash */

/* h(v) of spec v0.3 §8 item 3, written again: the top 15 bits of v * 0x9E3779B97F4A7C15 mod 2^64 */
static inline uint32_t t_home(sk_int v) {
    unsigned __int128 p = (unsigned __int128)(uint64_t)v * 0x9E3779B97F4A7C15ull;
    uint64_t low = (uint64_t)(p & UINT64_MAX); /* mod 2^64 */
    return (uint32_t)(low / (UINT64_C(1) << 49));
}

/* storage counters summed over the phases and parts (longest: the maximum) */
static inline r3_scount ssum(const r3_scounters *s) {
    r3_scount t = {0, 0, 0};
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            t.examined += s->c[p][q].examined;
            t.probes += s->c[p][q].probes;
            if (s->c[p][q].longest > t.longest) t.longest = s->c[p][q].longest;
        }
    return t;
}

/* ----------------------------------------------------------------------------------------- order of cases (§9) */

/* The case that §9 and the amendment's §17.3 (spec v0.3) name for x op y, from the magnitudes a/b, c/d and the
 * effective tags (subtraction's second tag already flipped); ix, iy: x, y an integer (b = 1, d = 1). */
static inline int predict_case(int op, uint64_t a, uint64_t b, uint64_t c, uint64_t d, int sx, int sy) {
    bool den = b == d, num = a == c, ad = a == d, cb = c == b, ox = a == b, oy = c == d, ix = b == 1, iy = d == 1;
    if (op == SK_PH_ADD || op == SK_PH_SUB) {
        if (den && num && sx != sy) return R2_ADD_ZERO;
        if (a == 0) return R2_ADD_ZERO_X;
        if (c == 0) return R2_ADD_ZERO_Y;
        if (ix && iy) return R2_ADD_INT_BOTH;
        if (ix) return R2_ADD_INT_X;
        if (iy) return R2_ADD_INT_Y;
        if (ad || cb) return R2_ADD_CROSS;
        if (den) return R2_ADD_DEN;
        if (num) return R2_ADD_NUM;
        return R2_ADD_GEN;
    }
    if (op == SK_PH_MUL) {
        if (a == 0 || c == 0) return R2_MUL_ZERO;
        if (ox) return R2_MUL_ONE_X;
        if (oy) return R2_MUL_ONE_Y;
        if (ix && iy) return R2_MUL_INT_BOTH;
        if (ad && cb) return R2_MUL_BOTH;
        if (den) return R2_MUL_DEN;
        if (num) return R2_MUL_NUM;
        if (ad) return R2_MUL_CROSS_AD;
        if (cb) return R2_MUL_CROSS_CB;
        if (ix) return R2_MUL_INT_X;
        if (iy) return R2_MUL_INT_Y;
        return R2_MUL_GEN;
    }
    if (a == 0) return R2_DIV_ZERO;
    if (ox) return R2_DIV_ONE_X;
    if (oy) return R2_DIV_ONE_Y;
    if (num && den) return R2_DIV_BOTH;
    if (ad) return R2_DIV_CROSS_AD;
    if (cb) return R2_DIV_CROSS_CB;
    if (num) return R2_DIV_NUM;
    if (den) return R2_DIV_DEN;
    return R2_DIV_GEN;
}

static inline int general_of(int op) {
    return op == SK_PH_MUL ? R2_MUL_GEN : op == SK_PH_DIV ? R2_DIV_GEN : R2_ADD_GEN;
}

/* ----------------------------------------------------------------------------------------- counter sums */

/* a field of sk_count or r2_xcount, summed over core and store of one phase (or of every phase with ph < 0) */
static inline uint64_t bsum(const sk_counters *b, int ph, size_t off) {
    uint64_t s = 0;
    for (int p = 0; p < SK_NPHASE; p++)
        if (ph < 0 || p == ph)
            for (int q = 0; q < SK_NPART; q++) s += *(const uint64_t *)((const char *)&b->c[p][q] + off);
    return s;
}
static inline uint64_t xsum(const r2_xcounters *x, int ph, size_t off) {
    uint64_t s = 0;
    for (int p = 0; p < SK_NPHASE; p++)
        if (ph < 0 || p == ph)
            for (int q = 0; q < SK_NPART; q++) s += *(const uint64_t *)((const char *)&x->c[p][q] + off);
    return s;
}
#define BS(b, ph, f) bsum(&(b), (ph), offsetof(sk_count, f))
#define XS(x, ph, f) xsum(&(x), (ph), offsetof(r2_xcount, f))
#define XK(x, ph, n, f) xsum(&(x), (ph), offsetof(r2_xcount, k) + ((n) - 1) * sizeof(r2_kcount) + offsetof(r2_kcount, f))
#define XCASES(x, ph, k) xsum(&(x), (ph), offsetof(r2_xcount, cases) + (size_t)(k) * sizeof(uint64_t))

/* the arithmetic of a call: core gcds, gcd work, exact divisions, multiplications, additions (§14.1 item 3) */
typedef struct arith {
    uint64_t gcd, work, div, mul, add;
} arith;

static inline arith arith_of(const sk_counters *b) {
    arith a = {0, 0, 0, 0, 0};
    for (int p = 0; p < SK_NPHASE; p++) {
        const sk_count *k = &b->c[p][SK_CORE];
        a.gcd += k->gcd;
        a.work += k->gcd_work;
        a.div += k->div_exact;
        a.mul += k->mul;
        a.add += k->add;
    }
    return a;
}
static inline arith arith_diff(arith x, arith y) {
    return (arith){x.gcd - y.gcd, x.work - y.work, x.div - y.div, x.mul - y.mul, x.add - y.add};
}
static inline bool arith_eq(arith x, arith y) {
    return x.gcd == y.gcd && x.work == y.work && x.div == y.div && x.mul == y.mul && x.add == y.add;
}
/* the sum of every K-skip's skipped arithmetic */
static inline arith k_skipped(const r2_xcounters *x) {
    arith a = {0, 0, 0, 0, 0};
    for (int k = 1; k <= R2_NK; k++) {
        a.gcd += XK(*x, -1, k, gcd);
        a.work += XK(*x, -1, k, gcd_work);
        a.div += XK(*x, -1, k, div);
        a.mul += XK(*x, -1, k, mul);
        a.add += XK(*x, -1, k, add);
    }
    return a;
}

#endif
