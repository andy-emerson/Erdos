/* introspect.h: read-only views of the structures, for the unit tests only.  They are defined in the library
 * (src/d1.c, src/d2.c) but are not part of the public API. */
#ifndef SK_INTROSPECT_H
#define SK_INTROSPECT_H

#include "skeletons.h"

uint32_t sk_d1_t_D(const sk_d1 *c);
uint32_t sk_d1_t_C(const sk_d1 *c);
sk_int sk_d1_t_divisor(const sk_d1 *c, uint32_t col);
sk_int sk_d1_t_den(const sk_d1 *c, uint32_t k);
uint32_t sk_d1_t_den_index(const sk_d1 *c, sk_handle x);
int sk_d1_t_bit(const sk_d1 *c, uint32_t k, uint32_t col);
uint32_t sk_d1_t_res_len(const sk_d1 *c, uint32_t k);
sk_int sk_d1_t_res(const sk_d1 *c, uint32_t k, uint32_t pos);
int sk_d1_t_pair(const sk_d1 *c, uint32_t i, uint32_t j);
uint32_t sk_d1_t_W(const sk_d1 *c);
uint32_t sk_d1_t_S(const sk_d1 *c);

uint32_t sk_d2_t_D(const sk_d2 *c);
uint32_t sk_d2_t_C(const sk_d2 *c);
sk_int sk_d2_t_piece(const sk_d2 *c, uint32_t p);
sk_int sk_d2_t_den(const sk_d2 *c, uint32_t k);
uint32_t sk_d2_t_den_index(const sk_d2 *c, sk_handle x);
uint32_t sk_d2_t_row_len(const sk_d2 *c, uint32_t k);
void sk_d2_t_row_entry(const sk_d2 *c, uint32_t k, uint32_t t, uint32_t *q, uint32_t *nu);

/* test build only: break an invariant on purpose */
void sk_d1_t_corrupt_residual(sk_d1 *c, uint32_t k, uint32_t pos, sk_int v);
void sk_d1_t_corrupt_pair(sk_d1 *c, uint32_t i, uint32_t j);
void sk_d2_t_corrupt_nu(sk_d2 *c, uint32_t k, uint32_t t, uint32_t nu);
void sk_d2_t_corrupt_piece(sk_d2 *c, uint32_t p, sk_int v);
void sk_ctl_t_corrupt(sk_ctl *c, sk_handle x, sk_int m, sk_int d);
void sk_ceil_t_corrupt_E(sk_ceil *c, sk_handle v, sk_int E);

#endif
