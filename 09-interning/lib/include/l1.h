/* l1.h: Design 3, layer 1 (spec 09-interning v0.1): exact sharing of whole numbers.
 *
 * Two rows from one source (src/l1.c):
 *   l1_*   L1, the baseline (spec §§4-15), compiled without L1_OPT;
 *   l1o_*  L1-opt (spec §17), the same source compiled with -DL1_OPT: the tricks O-1 to O-7, every one on.
 *
 * Build settings, as the 08 library (spec §4.6 item 3): neither SK_COUNT nor SK_AUDIT, the timing build; SK_COUNT,
 * the counting build; SK_COUNT and SK_AUDIT, the test build.  Width 64 only (spec §4.3 item 1).
 *
 * Every call follows the 08 conventions (sk_status results, out-parameters, the types of sk_common.h).
 */
#ifndef L1_H
#define L1_H

#include "sk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SK_WIDTH != 64
#error "layer 1 is built at width 64 only (spec §4.3 item 1)"
#endif

/* The cases of spec §7, numbered across operations.  Add and subtract share their four cases. */
typedef enum l1_case {
    L1_ADD_CROSS = 0,
    L1_ADD_DEN,
    L1_ADD_NUM,
    L1_ADD_GEN,
    L1_MUL_BOTH,
    L1_MUL_DEN,
    L1_MUL_NUM,
    L1_MUL_CROSS_AD,
    L1_MUL_CROSS_CB,
    L1_MUL_GEN,
    L1_DIV_BOTH,
    L1_DIV_CROSS_AD,
    L1_DIV_CROSS_CB,
    L1_DIV_NUM,
    L1_DIV_DEN,
    L1_DIV_GEN,
    L1_NCASE
} l1_case;

static inline const char *l1_case_name(l1_case k) {
    static const char *const names[L1_NCASE] = {
        "add_cross", "add_den", "add_num", "add_general", "mul_both", "mul_den", "mul_num", "mul_cross_ad",
        "mul_cross_cb", "mul_general", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den",
        "div_general"};
    return (unsigned)k < L1_NCASE ? names[k] : "?";
}

/* The O-1 cases (spec §17.1, the table of O-1). */
typedef enum l1_o1_case { L1_O1_ADD_DEN = 0, L1_O1_MUL_CROSS_AD, L1_O1_MUL_CROSS_CB, L1_O1_DIV_NUM, L1_O1_DIV_DEN, L1_NO1 } l1_o1_case;

/* The counters of spec §13.1 beyond those of 08 (held in sk_counters), and of §17.2 (L1-opt only; zero in L1).
 * One cell per phase and part, as the 08 counters. */
typedef struct l1_xcount {
    uint64_t idx_cmp;              /* index comparisons (§7): four per operation that reaches them */
    uint64_t cases[L1_NCASE];      /* operations that took each case */
    uint64_t gen_g1;               /* add/sub in case general with gcd(b, d) = 1, and in case num with g = 1 */
    uint64_t intern_found;         /* intern calls that found the whole number */
    uint64_t intern_appended;      /* intern calls that appended a new whole number */
    uint64_t zero_results;         /* results stored as 0/1 */
    /* L1-opt (§17.2) */
    uint64_t o1_skipped[L1_NO1];   /* O-1: lookups skipped, per case of the O-1 table */
    uint64_t o2_skipped0;          /* O-2: lookups of the whole number 0 skipped */
    uint64_t o2_skipped1;          /* O-2: lookups of the whole number 1 skipped */
    uint64_t o3_gcd_skipped;       /* O-3: gcds skipped */
    uint64_t o3_gcd_work;          /*      with the gcd work they would have had */
    uint64_t o4_div_skipped;       /* O-4: exact divisions skipped */
    uint64_t o4_mul_skipped;       /*      multiplications skipped */
    uint64_t o5_ops;               /* O-5: operations taking it */
    uint64_t o5_add_skipped;       /*      additions skipped */
    uint64_t o5_gcd_skipped;       /*      gcds skipped */
    uint64_t o5_gcd_work;          /*      with their gcd work */
    uint64_t o5_div_skipped;       /*      exact divisions skipped (the two of t/h and d/h; DECISIONS.md) */
    uint64_t o6_reads_avoided;     /* O-6: probes rejected by fingerprint (whole-number reads avoided) */
    uint64_t o7_growths_avoided;   /* O-7: growths of the rational list avoided */
    uint64_t o7_bytes_avoided;     /*      and the bytes they would have copied */
} l1_xcount;

typedef struct l1_xcounters {
    l1_xcount c[SK_NPHASE][SK_NPART];
} l1_xcounters;

/* Trick switches of L1-opt's test build (spec §17 item 2, §17.3 item 4). */
enum {
    L1O_T1 = 1 << 0, L1O_T2 = 1 << 1, L1O_T3 = 1 << 2, L1O_T4 = 1 << 3, L1O_T5 = 1 << 4, L1O_T6 = 1 << 5,
    L1O_T7 = 1 << 6, L1O_ALL = (1 << 7) - 1
};

#define L1_DECLARE(P, T)                                                                                          \
    typedef struct T T;                                                                                           \
    T *P##create(void);                                                                                           \
    void P##destroy(T *c);                                                                                        \
    sk_status P##enter(T *c, sk_int m, sk_int d, sk_handle *out);                                                 \
    sk_status P##add(T *c, sk_handle x, sk_handle y, sk_handle *out);                                             \
    sk_status P##sub(T *c, sk_handle x, sk_handle y, sk_handle *out);                                             \
    sk_status P##mul(T *c, sk_handle x, sk_handle y, sk_handle *out);                                             \
    sk_status P##div(T *c, sk_handle x, sk_handle y, sk_handle *out);                                             \
    sk_status P##read(const T *c, sk_handle x, sk_int *m, sk_int *d);                                             \
    uint32_t P##size(const T *c);                                                                                 \
    uint32_t P##wholes(const T *c);                                                                               \
    void P##memory(const T *c, sk_memory *report);                                                                \
    sk_status P##counters(const T *c, sk_counters *report, l1_xcounters *xreport);                                \
    sk_status P##overflow(const T *c, sk_overflow *report);                                                       \
    sk_status P##audit(T *c, const char **why);                                                                   \
    sk_status P##audit_changed(T *c, const char **why);                                                           \
    /* test build only (SK_ENOTBUILT otherwise): introspection, the shortcut switch of §14.3, corruption */       \
    sk_status P##t_whole(const T *c, uint32_t k, sk_int *v);                                                      \
    sk_status P##t_rat(const T *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j);                              \
    sk_status P##t_force_general(T *c, int on);                                                                   \
    sk_status P##t_corrupt_rat(T *c, sk_handle x, int neg, uint32_t i, uint32_t j);                               \
    sk_status P##t_corrupt_whole(T *c, uint32_t k, sk_int v);

/* L1, the baseline */
L1_DECLARE(l1_, l1_class)
/* L1-opt */
L1_DECLARE(l1o_, l1o_class)

/* O-7: size the rational list for exactly n rationals, before entry (untimed).  A no-op in L1's source compiled
 * without L1_OPT (not declared there) and, in the test build, with O-7 switched off. */
void l1o_reserve(l1o_class *c, uint32_t n);
/* Test build: the tricks that are on (a mask of L1O_T*; every one by default).  Set before the first call. */
sk_status l1o_t_tricks(l1o_class *c, unsigned mask);

#ifdef __cplusplus
}
#endif
#endif
