/* q32.h: Q32, layer 1 with 16-bit index fields (spec 10-interning v0.2 §§5, 7): one list of distinct whole numbers,
 * a value-to-index lookup (src/plk.c built with -DL1_Q32, unchanged), and the rationals as a 16-bit numerator field
 * (the sign tag in its top bit, a 15-bit index) and a 16-bit denominator index.  Capacity 32,768 whole numbers;
 * exceeding it aborts with a message.  Source src/q32.c; the builds of r2.h. */
#ifndef Q32_H
#define Q32_H

#include "r2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct q32_class q32_class;

q32_class *q32_create(void);
void q32_destroy(q32_class *c);
sk_status q32_enter(q32_class *c, sk_int m, sk_int d, sk_handle *out);
sk_status q32_add(q32_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32_sub(q32_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32_mul(q32_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32_div(q32_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32_read(const q32_class *c, sk_handle x, sk_int *m, sk_int *d);
uint32_t q32_size(const q32_class *c);
uint32_t q32_wholes(const q32_class *c);
void q32_memory(const q32_class *c, sk_memory *report);
sk_status q32_counters(const q32_class *c, sk_counters *report, r2_xcounters *xreport);
sk_status q32_overflow(const q32_class *c, sk_overflow *report);
sk_status q32_audit(q32_class *c, const char **why);
sk_status q32_audit_changed(q32_class *c, const char **why);
/* the profiling build (spec §13.4; SK_ENOTBUILT, or nothing, in the others): the report; the sampling flag, set by
 * the driver around a sampled call; the driver's outer total of the last sampled call */
sk_status q32_profile(const q32_class *c, r2_profile *report);
void q32_prof_sample(q32_class *c, int on);
void q32_prof_outer(q32_class *c, uint64_t cycles);
/* test build only (SK_ENOTBUILT otherwise): introspection, the two switches of §14.3, corruption for the audits */
sk_status q32_t_whole(const q32_class *c, uint32_t k, sk_int *v);
sk_status q32_t_rat(const q32_class *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j);
sk_status q32_t_force_general(q32_class *c, int on);
sk_status q32_t_known_off(q32_class *c, unsigned mask);
sk_status q32_t_corrupt_rat(q32_class *c, sk_handle x, int neg, uint32_t i, uint32_t j);
sk_status q32_t_corrupt_whole(q32_class *c, uint32_t k, sk_int v);

#ifdef __cplusplus
}
#endif
#endif
