/* sk_ceil.h: the shared-denominator ceiling (spec §9).  A vector is a common denominator E >= 1 and numerators
 * n_1..n_L; element k is n_k / E.  Stored vectors are in canonical form, gcd(E, n_1, ..., n_L) = 1.
 *
 * Handles name vectors.  Stored vectors (entered or results) have handles 0, 1, 2, ...; working copies made by
 * gather have handles with bit 31 set (SK_CEIL_WORKING).  Both kinds can be operands and can be read; only stored
 * vectors count as stored values and in the memory report (spec §12.2). */
#ifndef SK_CEIL_H
#define SK_CEIL_H

#include "sk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_ceil sk_ceil;

#define SK_CEIL_WORKING UINT32_C(0x80000000)

sk_ceil *sk_ceil_create(void);
void sk_ceil_destroy(sk_ceil *c);

/* Stores the vector m[0..len-1] / d in canonical form.  SK_EDIVZERO if d = 0; SK_EINVAL if d or an element is the
 * most negative integer, or m is NULL with len > 0. */
sk_status sk_ceil_enter_vector(sk_ceil *c, const sk_int *m, uint32_t len, sk_int d, sk_handle *out);

/* A working copy (not a stored value) of the elements of v at positions[0..len-1], over v's E, copied as they are
 * (no content step).  SK_EINVAL for a bad handle or a position out of range. */
sk_status sk_ceil_gather(sk_ceil *c, sk_handle v, const uint32_t *positions, uint32_t len, sk_handle *out);

/* EXTENSION, not in spec §9: as gather, but element i is negated when negate[i] is nonzero.  The measurement rule's
 * CH-all job "gathers the 1,000 y_k, negated where the step subtracts"; the spec's gather has no way to negate.
 * Negation changes no E and is not counted (DECISIONS.md). */
sk_status sk_ceil_gather_signed(sk_ceil *c, sk_handle v, const uint32_t *positions, const unsigned char *negate,
                                uint32_t len, sk_handle *out);

/* Elementwise on vectors of equal length (SK_EINVAL otherwise); the result is a new stored vector. */
sk_status sk_ceil_add(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ceil_sub(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ceil_mul(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ceil_div(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out); /* SK_EDIVZERO if any y_k = 0 */

/* Element k of v, n_k / E reduced by gcd(n_k, E) (counted under SK_PH_READ). */
sk_status sk_ceil_read(sk_ceil *c, sk_handle v, uint32_t k, sk_int *m, sk_int *d);
/* The length of v. */
sk_status sk_ceil_length(const sk_ceil *c, sk_handle v, uint32_t *len);
/* The number of stored vectors (not elements; the memory report's `values` is the number of elements). */
uint32_t sk_ceil_size(const sk_ceil *c);

void sk_ceil_memory(const sk_ceil *c, sk_memory *report);
sk_status sk_ceil_counters(const sk_ceil *c, sk_counters *report);
sk_status sk_ceil_overflow(const sk_ceil *c, sk_overflow *report);

/* Audits (spec §13.2, "Ceiling: every vector is in canonical form"); as for the other rows.  audit_scheduled is
 * identical to audit. */
sk_status sk_ceil_audit(sk_ceil *c, const char **why);
sk_status sk_ceil_audit_scheduled(sk_ceil *c, const char **why);
sk_status sk_ceil_audit_changed(sk_ceil *c, const char **why);

#ifdef __cplusplus
}
#endif
#endif
