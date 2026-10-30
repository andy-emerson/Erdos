/* introspect_d2.h: read-only views of Design 2's structures, for the unit tests only.  They are defined in the library
 * (src/d2.c) but are not part of the public API.  Split out of the joint introspect.h, Oct 29, 2026, unchanged. */
#ifndef SK_INTROSPECT_D2_H
#define SK_INTROSPECT_D2_H

#include "sk_d2.h"

uint32_t sk_d2_t_D(const sk_d2 *c);
uint32_t sk_d2_t_C(const sk_d2 *c);
sk_int sk_d2_t_piece(const sk_d2 *c, uint32_t p);
sk_int sk_d2_t_den(const sk_d2 *c, uint32_t k);
sk_int sk_d2_t_u(const sk_d2 *c, uint32_t k);
uint32_t sk_d2_t_den_index(const sk_d2 *c, sk_handle x);
uint32_t sk_d2_t_row_len(const sk_d2 *c, uint32_t k);
void sk_d2_t_row_entry(const sk_d2 *c, uint32_t k, uint32_t t, uint32_t *q, uint32_t *nu);

/* test build only: break an invariant on purpose */
void sk_d2_t_corrupt_nu(sk_d2 *c, uint32_t k, uint32_t t, uint32_t nu);
void sk_d2_t_corrupt_u(sk_d2 *c, uint32_t k, sk_int u);
void sk_d2_t_corrupt_piece(sk_d2 *c, uint32_t p, sk_int v);
/* test build only: 1 drops Design 2's extra refinement in meetings, to show it is needed */
extern int sk_d2_t_skip_extra;
/* test build only: 1 injects a fault into Design 2's reduce that would loop forever without the meeting's bound */
extern int sk_d2_t_fault_stuck;

#endif
