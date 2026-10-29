/* t_common.h: shared by the unit and differential tests (test build only): an exact reference with 128-bit
 * integers and its own Euclid gcd (independent of the 08 module, spec §14.1), a PRNG, a row table that drives L1,
 * L1-opt and the control through one interface, and the order of cases of spec §7 computed from magnitudes. */
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
    int kind; /* 0 L1, 1 L1-opt, 2 ctl */
    void *(*create)(void);
    void (*destroy)(void *);
    sk_status (*enter)(void *, sk_int, sk_int, sk_handle *);
    sk_status (*op[5])(void *, sk_handle, sk_handle, sk_handle *); /* indexed by sk_phase 1..4 */
    sk_status (*read)(const void *, sk_handle, sk_int *, sk_int *);
    uint32_t (*size)(const void *);
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

WRAP(l1_, l1_class)
WRAP(l1o_, l1o_class)
WRAP(sk_ctl_, sk_ctl)

static const row ROW_L1 = {"L1", 0, l1_w_create, l1_w_destroy, l1_w_enter,
    {NULL, l1_w_add, l1_w_sub, l1_w_mul, l1_w_div}, l1_w_read, l1_w_size};
static const row ROW_L1O = {"L1-opt", 1, l1o_w_create, l1o_w_destroy, l1o_w_enter,
    {NULL, l1o_w_add, l1o_w_sub, l1o_w_mul, l1o_w_div}, l1o_w_read, l1o_w_size};
static const row ROW_CTL = {"ctl", 2, sk_ctl_w_create, sk_ctl_w_destroy, sk_ctl_w_enter,
    {NULL, sk_ctl_w_add, sk_ctl_w_sub, sk_ctl_w_mul, sk_ctl_w_div}, sk_ctl_w_read, sk_ctl_w_size};

/* Layer-1 helpers that work on either row (kind 0 or 1). */
static inline uint32_t lw_wholes(const row *R, const void *c) {
    return R->kind == 0 ? l1_wholes((const l1_class *)c) : l1o_wholes((const l1o_class *)c);
}
static inline sk_int lw_whole(const row *R, const void *c, uint32_t k) {
    sk_int v = -1;
    if (R->kind == 0) l1_t_whole((const l1_class *)c, k, &v);
    else l1o_t_whole((const l1o_class *)c, k, &v);
    return v;
}
static inline void lw_rat(const row *R, const void *c, sk_handle h, int *neg, uint32_t *i, uint32_t *j) {
    if (R->kind == 0) l1_t_rat((const l1_class *)c, h, neg, i, j);
    else l1o_t_rat((const l1o_class *)c, h, neg, i, j);
}
static inline void lw_counters(const row *R, const void *c, sk_counters *b, l1_xcounters *x) {
    if (R->kind == 0) l1_counters((const l1_class *)c, b, x);
    else if (R->kind == 1) l1o_counters((const l1o_class *)c, b, x);
    else {
        sk_counters tmp;
        sk_ctl_counters((const sk_ctl *)c, b ? b : &tmp);
        if (x) memset(x, 0, sizeof *x);
    }
}
static inline sk_status lw_audit(const row *R, void *c, int full, const char **why) {
    if (R->kind == 0) return full ? l1_audit((l1_class *)c, why) : l1_audit_changed((l1_class *)c, why);
    if (R->kind == 1) return full ? l1o_audit((l1o_class *)c, why) : l1o_audit_changed((l1o_class *)c, why);
    return full ? sk_ctl_audit((sk_ctl *)c, why) : sk_ctl_audit_changed((sk_ctl *)c, why);
}
static inline void lw_force_general(const row *R, void *c, int on) {
    if (R->kind == 0) l1_t_force_general((l1_class *)c, on);
    else if (R->kind == 1) l1o_t_force_general((l1o_class *)c, on);
}
static inline void lw_memory(const row *R, const void *c, sk_memory *m) {
    if (R->kind == 0) l1_memory((const l1_class *)c, m);
    else if (R->kind == 1) l1o_memory((const l1o_class *)c, m);
    else sk_ctl_memory((const sk_ctl *)c, m);
}
static inline sk_status lw_overflow(const row *R, const void *c, sk_overflow *o) {
    if (R->kind == 0) return l1_overflow((const l1_class *)c, o);
    if (R->kind == 1) return l1o_overflow((const l1o_class *)c, o);
    return sk_ctl_overflow((const sk_ctl *)c, o);
}

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
