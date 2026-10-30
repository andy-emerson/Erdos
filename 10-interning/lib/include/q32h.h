/* q32h.h: Q32-hash (spec 10-interning v0.3 §§5, 8): exactly two structures, the rationals as a 16-bit numerator field
 * (the sign tag in its top bit, a 15-bit slot) and a 16-bit denominator slot, and a hash table of whole numbers with
 * exactly 32,768 slots, allocated in create and never resized (multiplicative hashing, linear probing).  No separate
 * list.  A full table aborts with a message.  Source src/q32h.c (from round 2's src/q32.c); the builds of r3.h. */
#ifndef Q32H_H
#define Q32H_H

#include "r3.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct q32h_class q32h_class;

q32h_class *q32h_create(void);
void q32h_destroy(q32h_class *c);
sk_status q32h_enter(q32h_class *c, sk_int m, sk_int d, sk_handle *out);
sk_status q32h_add(q32h_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32h_sub(q32h_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32h_mul(q32h_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32h_div(q32h_class *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status q32h_read(const q32h_class *c, sk_handle x, sk_int *m, sk_int *d);
uint32_t q32h_size(const q32h_class *c);
uint32_t q32h_wholes(const q32h_class *c);
void q32h_memory(const q32h_class *c, sk_memory *report);
sk_status q32h_counters(const q32h_class *c, sk_counters *report, r2_xcounters *xreport);
sk_status q32h_overflow(const q32h_class *c, sk_overflow *report);
/* the storage counters of spec v0.3 §11.1 (counting builds; SK_ENOTBUILT otherwise) */
sk_status q32h_storage(const q32h_class *c, r3_scounters *report);
sk_status q32h_audit(q32h_class *c, const char **why);
sk_status q32h_audit_changed(q32h_class *c, const char **why);
/* the profiling build (spec §13.4; SK_ENOTBUILT, or nothing, in the others): the report; the sampling flag, set by
 * the driver around a sampled call; the driver's outer total of the last sampled call */
sk_status q32h_profile(const q32h_class *c, r2_profile *report);
void q32h_prof_sample(q32h_class *c, int on);
void q32h_prof_outer(q32h_class *c, uint64_t cycles);
/* test build only (SK_ENOTBUILT otherwise): introspection, the two switches of §14.3, corruption for the audits */
sk_status q32h_t_whole(const q32h_class *c, uint32_t k, sk_int *v); /* k a slot; SK_EINVAL when empty */
sk_status q32h_t_rat(const q32h_class *c, sk_handle x, int *neg, uint32_t *i, uint32_t *j);
sk_status q32h_t_force_general(q32h_class *c, int on);
sk_status q32h_t_known_off(q32h_class *c, unsigned mask);
sk_status q32h_t_corrupt_rat(q32h_class *c, sk_handle x, int neg, uint32_t i, uint32_t j);
sk_status q32h_t_corrupt_whole(q32h_class *c, uint32_t k, sk_int v);

#ifdef __cplusplus
}
#endif
#endif
