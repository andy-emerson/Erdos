/* t_common.h: shared by the unit and differential tests (test build only): an exact reference with 128-bit
 * integers and its own Euclid gcd (independent of the 08 module, spec §14.1), a PRNG, a row table that drives every
 * layer-1 row (L1, L1-opt, L1-Q32, L1-opt-Q32, the lookup gate's build) and the control through one interface, and
 * the order of cases of spec §7 computed from magnitudes.  Compiled with -DT_Q32, ROW_L1 and ROW_L1O, and the tests'
 * direct calls, are the Q32 rows (spec §18.3 item 2). */
#ifndef T_COMMON_H
#define T_COMMON_H

#ifndef SK_AUDIT
#error "the tests need the test build (-DSK_COUNT -DSK_AUDIT)"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "l1.h"
#include "sk_ctl.h"

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
static inline ref_q ref_op(int op, ref_q a, ref_q b) {
    switch (op) {
    case SK_PH_ADD: return ref_make(a.n * b.d + b.n * a.d, a.d * b.d);
    case SK_PH_SUB: return ref_make(a.n * b.d - b.n * a.d, a.d * b.d);
    case SK_PH_MUL: {
        i128 g1 = ref_gcd(a.n, b.d), g2 = ref_gcd(b.n, a.d);
        if (!g1) g1 = 1;
        if (!g2) g2 = 1;
        return ref_make((a.n / g1) * (b.n / g2), (a.d / g2) * (b.d / g1));
    }
    default: {
        ref_q inv = ref_make(b.d, b.n);
        i128 g1 = ref_gcd(a.n, inv.d), g2 = ref_gcd(inv.n, a.d);
        if (!g1) g1 = 1;
        if (!g2) g2 = 1;
        return ref_make((a.n / g1) * (inv.n / g2), (a.d / g2) * (inv.d / g1));
    }
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
    int kind;            /* 0 a baseline layer-1 row (L1, L1-Q32, the gate's build), 1 an L1-opt row, 2 ctl */
    unsigned rat_bytes;  /* layer 1: bytes per rational entry (8 at Q64, 4 at Q32) */
    unsigned slot_bytes; /* layer 1: bytes per lookup slot (L1 4, L1-opt 8, L1-Q32 2, L1-opt-Q32 6) */
    void *(*create)(void);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int, sk_handle *);
    sk_status (*op[5])(void *, sk_handle, sk_handle, sk_handle *); /* indexed by sk_phase 1..4 */
    sk_status (*read)(const void *, sk_handle, sk_int *, sk_int *);
    uint32_t (*size)(const void *);
    /* layer 1 only (NULL for ctl) */
    uint32_t (*wholes)(const void *);
    sk_status (*t_whole)(const void *, uint32_t, sk_int *);
    sk_status (*t_rat)(const void *, sk_handle, int *, uint32_t *, uint32_t *);
    sk_status (*t_force_general)(void *, int);
    sk_status (*counters)(const void *, sk_counters *, l1_xcounters *);
    sk_status (*audit)(void *, const char **);
    sk_status (*audit_changed)(void *, const char **);
    void (*memory)(const void *, sk_memory *);
    sk_status (*overflow)(const void *, sk_overflow *);
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
    static uint32_t P##w_size(const void *c) { return P##size((const T *)c); }

/* the layer-1 calls beyond the common ones */
#define WRAPL(P, T)                                                                                               \
    WRAP(P, T)                                                                                                    \
    static uint32_t P##w_wholes(const void *c) { return P##wholes((const T *)c); }                                \
    static sk_status P##w_t_whole(const void *c, uint32_t k, sk_int *v) { return P##t_whole((const T *)c, k, v); } \
    static sk_status P##w_t_rat(const void *c, sk_handle h, int *n, uint32_t *i, uint32_t *j) {                   \
        return P##t_rat((const T *)c, h, n, i, j);                                                                \
    }                                                                                                             \
    static sk_status P##w_t_force_general(void *c, int on) { return P##t_force_general((T *)c, on); }            \
    static sk_status P##w_counters(const void *c, sk_counters *b, l1_xcounters *x) {                              \
        return P##counters((const T *)c, b, x);                                                                   \
    }                                                                                                             \
    static sk_status P##w_audit(void *c, const char **w) { return P##audit((T *)c, w); }                          \
    static sk_status P##w_audit_changed(void *c, const char **w) { return P##audit_changed((T *)c, w); }          \
    static void P##w_memory(const void *c, sk_memory *m) { P##memory((const T *)c, m); }                          \
    static sk_status P##w_overflow(const void *c, sk_overflow *o) { return P##overflow((const T *)c, o); }

WRAPL(l1_, l1_class)
WRAPL(l1o_, l1o_class)
WRAPL(l1q_, l1q_class)
WRAPL(l1oq_, l1oq_class)
WRAPL(l1g_, l1g_class)
WRAP(sk_ctl_, sk_ctl)

#define LROW(name, kind, rb, sb, P)                                                                               \
    {name, kind, rb, sb, P##w_create, P##w_destroy, P##w_enter, {NULL, P##w_add, P##w_sub, P##w_mul, P##w_div},  \
     P##w_read, P##w_size, P##w_wholes, P##w_t_whole, P##w_t_rat, P##w_t_force_general, P##w_counters,           \
     P##w_audit, P##w_audit_changed, P##w_memory, P##w_overflow}

/* every row, at both widths */
static const row ROW_L1_64 = LROW("L1", 0, 8, 4, l1_);
static const row ROW_L1O_64 = LROW("L1-opt", 1, 8, 8, l1o_);
static const row ROW_L1Q = LROW("L1-Q32", 0, 4, 2, l1q_);
static const row ROW_L1OQ = LROW("L1-opt-Q32", 1, 4, 6, l1oq_);
static const row ROW_L1G = LROW("L1-lookup-gate", 0, 8, 4, l1g_);
static const row ROW_CTL = {"ctl", 2, 0, 0, sk_ctl_w_create, sk_ctl_w_destroy, sk_ctl_w_enter,
    {NULL, sk_ctl_w_add, sk_ctl_w_sub, sk_ctl_w_mul, sk_ctl_w_div}, sk_ctl_w_read, sk_ctl_w_size,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* The rows the unit and differential tests drive: the Q64 rows, or with -DT_Q32 the Q32 rows (spec §18.3 item 2). */
#ifdef T_Q32
#define ROW_L1 ROW_L1Q
#define ROW_L1O ROW_L1OQ
#define T_WIDTH "Q32"
#else
#define ROW_L1 ROW_L1_64
#define ROW_L1O ROW_L1O_64
#define T_WIDTH "Q64"
#endif

/* Layer-1 helpers that work on any layer-1 row (kind 0 or 1), and on ctl where it applies. */
static inline uint32_t lw_wholes(const row *R, const void *c) { return R->wholes(c); }
static inline sk_int lw_whole(const row *R, const void *c, uint32_t k) {
    sk_int v = -1;
    R->t_whole(c, k, &v);
    return v;
}
static inline void lw_rat(const row *R, const void *c, sk_handle h, int *neg, uint32_t *i, uint32_t *j) {
    R->t_rat(c, h, neg, i, j);
}
static inline void lw_counters(const row *R, const void *c, sk_counters *b, l1_xcounters *x) {
    if (R->kind != 2) R->counters(c, b, x);
    else {
        sk_counters tmp;
        sk_ctl_counters((const sk_ctl *)c, b ? b : &tmp);
        if (x) memset(x, 0, sizeof *x);
    }
}
static inline sk_status lw_audit(const row *R, void *c, int full, const char **why) {
    if (R->kind != 2) return full ? R->audit(c, why) : R->audit_changed(c, why);
    return full ? sk_ctl_audit((sk_ctl *)c, why) : sk_ctl_audit_changed((sk_ctl *)c, why);
}
static inline void lw_force_general(const row *R, void *c, int on) {
    if (R->kind != 2) R->t_force_general(c, on);
}
static inline void lw_memory(const row *R, const void *c, sk_memory *m) {
    if (R->kind != 2) R->memory(c, m);
    else sk_ctl_memory((const sk_ctl *)c, m);
}
static inline sk_status lw_overflow(const row *R, const void *c, sk_overflow *o) {
    if (R->kind != 2) return R->overflow(c, o);
    return sk_ctl_overflow((const sk_ctl *)c, o);
}

/* With -DT_Q32, the tests' direct calls of L1 and L1-opt go to L1-Q32 and L1-opt-Q32.  Defined after the row tables,
 * so the Q64 rows above stay Q64. */
#ifdef T_Q32
#define l1_class l1q_class
#define l1_create l1q_create
#define l1_destroy l1q_destroy
#define l1_enter l1q_enter
#define l1_add l1q_add
#define l1_sub l1q_sub
#define l1_mul l1q_mul
#define l1_div l1q_div
#define l1_read l1q_read
#define l1_wholes l1q_wholes
#define l1_counters l1q_counters
#define l1_audit l1q_audit
#define l1_audit_changed l1q_audit_changed
#define l1_memory l1q_memory
#define l1_t_whole l1q_t_whole
#define l1_t_rat l1q_t_rat
#define l1_t_corrupt_rat l1q_t_corrupt_rat
#define l1_t_corrupt_whole l1q_t_corrupt_whole
#define l1o_class l1oq_class
#define l1o_create l1oq_create
#define l1o_destroy l1oq_destroy
#define l1o_enter l1oq_enter
#define l1o_add l1oq_add
#define l1o_sub l1oq_sub
#define l1o_mul l1oq_mul
#define l1o_div l1oq_div
#define l1o_read l1oq_read
#define l1o_wholes l1oq_wholes
#define l1o_counters l1oq_counters
#define l1o_audit l1oq_audit
#define l1o_audit_changed l1oq_audit_changed
#define l1o_memory l1oq_memory
#define l1o_t_whole l1oq_t_whole
#define l1o_t_rat l1oq_t_rat
#define l1o_t_corrupt_rat l1oq_t_corrupt_rat
#define l1o_t_corrupt_whole l1oq_t_corrupt_whole
#define l1o_reserve l1oq_reserve
#define l1o_t_tricks l1oq_t_tricks
#endif

/* sums over core and store of one phase */
static inline uint64_t xsum_case(const l1_xcounters *x, int ph, int k) {
    return x->c[ph][SK_CORE].cases[k] + x->c[ph][SK_STORE].cases[k];
}

/* ----------------------------------------------------------------------------------------- order of cases (§7) */

/* The case that §7 names for x op y, from the magnitudes a/b, c/d alone (the generator's rule). */
static inline int predict_case(int op, uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    bool den = b == d, num = a == c, ad = a == d, cb = c == b;
    if (op == SK_PH_ADD || op == SK_PH_SUB) {
        if (ad || cb) return L1_ADD_CROSS;
        if (den) return L1_ADD_DEN;
        if (num) return L1_ADD_NUM;
        return L1_ADD_GEN;
    }
    if (op == SK_PH_MUL) {
        if (ad && cb) return L1_MUL_BOTH;
        if (den) return L1_MUL_DEN;
        if (num) return L1_MUL_NUM;
        if (ad) return L1_MUL_CROSS_AD;
        if (cb) return L1_MUL_CROSS_CB;
        return L1_MUL_GEN;
    }
    if (num && den) return L1_DIV_BOTH;
    if (ad) return L1_DIV_CROSS_AD;
    if (cb) return L1_DIV_CROSS_CB;
    if (num) return L1_DIV_NUM;
    if (den) return L1_DIV_DEN;
    return L1_DIV_GEN;
}

#endif
