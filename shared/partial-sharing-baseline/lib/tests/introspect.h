/* introspect.h: test hooks of the control and the ceiling, for the unit tests only.  They are defined in the library
 * (src/ctl.c, src/ceil.c) but are not part of the public API.  Design 1's hooks are in
 * 08-common-divisors/lib/tests/introspect_d1.h and Design 2's in 09-coprime-base/lib/tests/introspect_d2.h (split
 * Oct 29, 2026; no declaration changed). */
#ifndef SK_INTROSPECT_H
#define SK_INTROSPECT_H

#include "sk_ceil.h"
#include "sk_ctl.h"

/* test build only: break an invariant on purpose */
void sk_ctl_t_corrupt(sk_ctl *c, sk_handle x, sk_int m, sk_int d);
void sk_ceil_t_corrupt_E(sk_ceil *c, sk_handle v, sk_int E);

#endif
