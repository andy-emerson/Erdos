/* skeletons.h: every row of the skeletons library, and a table of function pointers that drives the control,
 * Design 1 and Design 2 through one interface (spec §4.1).  The table is a convenience: the direct calls of
 * sk_ctl.h, sk_d1.h and sk_d2.h do exactly the same thing.  Since Oct 29, 2026, Design 1's header (sk_d1.h) is in
 * 08-common-divisors/lib/include and Design 2's (sk_d2.h) in 09-coprime-base/lib/include. */
#ifndef SKELETONS_H
#define SKELETONS_H

#include "sk_ceil.h"
#include "sk_common.h"
#include "sk_ctl.h"
#include "sk_d1.h"
#include "sk_d2.h"
#include "sk_row.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the row type and the three row tables are declared in sk_row.h */

#ifdef __cplusplus
}
#endif
#endif
