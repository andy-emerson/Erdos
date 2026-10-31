/* tc.h: TC, the textbook control (spec 10-interning v0.2 §§5, 8): each rational a pair (m, d) of 64-bit values in
 * lowest terms, d >= 1, zero as (0, 1), in one list that grows by doubling.  No other structure.  Source src/tc.c;
 * the builds of r2.h. */
#ifndef TC_H
#define TC_H

#include "r2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tc_class tc_class;

tc_class *tc_create(void);
void tc_destroy(tc_class *c);
sk_status tc_enter(tc_class *c, sk_int m, sk_int d, sk_handle *out);
sk_status tc_add(tc_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status tc_sub(tc_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status tc_mul(tc_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status tc_div(tc_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status tc_read(const tc_class *c, sk_handle x, sk_int *m, sk_int *d);
uint32_t tc_size(const tc_class *c);
void tc_memory(const tc_class *c, sk_memory *report);
sk_status tc_counters(const tc_class *c, sk_counters *report, r2_xcounters *xreport);
sk_status tc_overflow(const tc_class *c, sk_overflow *report);
sk_status tc_audit(tc_class *c, const char **why);
sk_status tc_audit_changed(tc_class *c, const char **why);
sk_status tc_profile(const tc_class *c, r2_profile *report);
void tc_prof_sample(tc_class *c, int on);
void tc_prof_outer(tc_class *c, uint64_t cycles);
/* test build only (SK_ENOTBUILT otherwise) */
sk_status tc_t_force_general(tc_class *c, int on);
sk_status tc_t_known_off(tc_class *c, unsigned mask);
sk_status tc_t_corrupt(tc_class *c, sk_handle x, sk_int m, sk_int d);

#ifdef __cplusplus
}
#endif
#endif
