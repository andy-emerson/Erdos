/* tcm.h: TC of the matrix round (spec 11-matrix v0.1 §§3-4): each matrix one array of r*c pairs (m, d) of 64-bit
 * values in lowest terms, d >= 1, zero as (0, 1), row-major; structures (T-A), (T-B), (T-C).  Source src/tcm.c (tc3.c
 * with only the operand and result interface changed); the builds of m1.h. */
#ifndef TCM_H
#define TCM_H

#include "m1.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tcm_class tcm_class;
typedef struct tcm_pair {
    sk_int m, d;
} tcm_pair;

tcm_class *tcm_create(uint32_t rows, uint32_t cols);
void tcm_destroy(tcm_class *c);
sk_status tcm_enter(tcm_class *c, sk_int m, sk_int d); /* the next cell, in the order of entry */
/* the matrix operations: C = A + B, C = A B (C empty; A and B fully entered); the arithmetic is charged to C */
sk_status tcm_madd(tcm_class *a, tcm_class *b, tcm_class *c);
sk_status tcm_mmul(tcm_class *a, tcm_class *b, tcm_class *c);
/* tc3.c's operations, on pairs (spec §4): counted in c */
sk_status tcm_add(tcm_class *c, tcm_pair x, tcm_pair y, tcm_pair *out);
sk_status tcm_sub(tcm_class *c, tcm_pair x, tcm_pair y, tcm_pair *out);
sk_status tcm_mul(tcm_class *c, tcm_pair x, tcm_pair y, tcm_pair *out);
sk_status tcm_div(tcm_class *c, tcm_pair x, tcm_pair y, tcm_pair *out);
sk_status tcm_read(const tcm_class *c, uint32_t cell, sk_int *m, sk_int *d);
uint32_t tcm_size(const tcm_class *c);
void tcm_memory(const tcm_class *c, sk_memory *report);
sk_status tcm_counters(const tcm_class *c, sk_counters *report, r2_xcounters *xreport);
sk_status tcm_overflow(const tcm_class *c, sk_overflow *report);
sk_status tcm_audit(tcm_class *c, const char **why);
sk_status tcm_audit_changed(tcm_class *c, const char **why);
/* test build only (SK_ENOTBUILT otherwise) */
sk_status tcm_t_force_general(tcm_class *c, int on);
sk_status tcm_t_known_off(tcm_class *c, unsigned mask);
sk_status tcm_t_corrupt(tcm_class *c, uint32_t cell, sk_int m, sk_int d);

#ifdef __cplusplus
}
#endif
#endif
