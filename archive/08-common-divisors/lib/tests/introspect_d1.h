/* introspect_d1.h: read-only views of Design 1's structures, for the unit tests only.  They are defined in the library
 * (src/d1.c) but are not part of the public API.  Split out of the joint introspect.h, Oct 29, 2026, unchanged. */
#ifndef SK_INTROSPECT_D1_H
#define SK_INTROSPECT_D1_H

#include "sk_d1.h"

uint32_t sk_d1_t_D(const sk_d1 *c);
uint32_t sk_d1_t_C(const sk_d1 *c);
sk_int sk_d1_t_divisor(const sk_d1 *c, uint32_t col);      /* the divisor of rank col by value */
uint32_t sk_d1_t_divisor_id(const sk_d1 *c, uint32_t col); /* its stable id */
sk_int sk_d1_t_den(const sk_d1 *c, uint32_t k);
uint32_t sk_d1_t_den_index(const sk_d1 *c, sk_handle x);
int sk_d1_t_bit(const sk_d1 *c, uint32_t k, uint32_t col);
uint32_t sk_d1_t_res_len(const sk_d1 *c, uint32_t k);
sk_int sk_d1_t_res(const sk_d1 *c, uint32_t k, uint32_t pos);
int sk_d1_t_pair(const sk_d1 *c, uint32_t i, uint32_t j);
uint32_t sk_d1_t_pairs(const sk_d1 *c); /* the number of compared pairs in the set */
uint32_t sk_d1_t_S(const sk_d1 *c);

/* test build only: break an invariant on purpose */
void sk_d1_t_corrupt_residual(sk_d1 *c, uint32_t k, uint32_t pos, sk_int v);
void sk_d1_t_corrupt_pair(sk_d1 *c, uint32_t i, uint32_t j);

#endif
