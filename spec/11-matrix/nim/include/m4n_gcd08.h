/* m4n_gcd08.h: the test build's reference gcd for the Nim models (src/core.nim, gcd08): 08's own sk_gcd_raw, from 08's
 * pinned lib/vendor08/sk_internal.h.  The Nim compile's include path has lib/vendor08 and not lib/vendor08/m4, so this
 * is 08's file; the check below refuses the m4 one. */
#ifndef M4N_GCD08_H
#define M4N_GCD08_H
#include "sk_internal.h"
#ifdef SK_GCD_SPREAD
#error "the reference gcd must be 08's sk_internal.h, not the m4 one"
#endif
#endif
