/* q32i.h: Q32-index (spec 10-interning v0.3 §§5, 7): exactly two structures, the rationals as a 16-bit numerator field
 * (the sign tag in its top bit, a 15-bit index) and a 16-bit denominator index, and a list of distinct whole numbers in
 * arrival order, searched top to bottom.  No hash and no lookup.  Capacity 32,768 whole numbers; exceeding it aborts
 * with a message.  Source src/q32i.c (from round 2's src/q32.c); the builds of r3.h. */
#ifndef Q32I_H
#define Q32I_H

#include "r3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct q32i_class q32i_class;

q32i_class *q32i_create(void);
void q32i_destroy(q32i_class *c);
sk_status q32i_enter(q32i_class *c, sk_int m, sk_int d, sk_handle *out);
sk_status q32i_add(q32i_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32i_sub(q32i_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32i_mul(q32i_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32i_div(q32i_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32i_read(const q32i_class *c, sk_handle x, sk_int *m, sk_int *d);
uint32_t q32i_size(const q32i_class *c);
uint32_t q32i_wholes(const q32i_class *c);
void q32i_memory(const q32i_class *c, sk_memory *report);
sk_status q32i_counters(const q32i_class *c, sk_counters *report, r2_xcounters *xreport);
sk_status q32i_overflow(const q32i_class *c, sk_overflow *report);
/* the storage counters of spec v0.3 §11.1 (counting builds; SK_ENOTBUILT otherwise) */
sk_status q32i_storage(const q32i_class *c, r3_scounters *report);
sk_status q32i_audit(q32i_class *c, const char **why);
sk_status q32i_audit_changed(q32i_class *c, const char **why);
/* the profiling build (spec §13.4; SK_ENOTBUILT, or nothing, in the others): the report; the sampling flag, set by
 * the driver around a sampled call; the driver's outer total of the last sampled call */
sk_status q32i_profile(const q32i_class *c, r2_profile *report);
void q32i_prof_sample(q32i_class *c, int on);
void q32i_prof_outer(q32i_class *c, uint64_t cycles);
/* test build only (SK_ENOTBUILT otherwise): introspection, the two switches of §14.3, corruption for the audits */
sk_status q32i_t_whole(const q32i_class *c, uint32_t k, sk_int *v);
sk_status q32i_t_rat(const q32i_class *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j);
sk_status q32i_t_force_general(q32i_class *c, int on);
sk_status q32i_t_known_off(q32i_class *c, unsigned mask);
sk_status q32i_t_corrupt_rat(q32i_class *c, sk_handle x, int neg, uint32_t i, uint32_t j);
sk_status q32i_t_corrupt_whole(q32i_class *c, uint32_t k, sk_int v);

#ifdef __cplusplus
}
#endif
#endif
