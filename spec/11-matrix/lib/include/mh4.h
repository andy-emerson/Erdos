/* mh4.h: Hash-V-pack-4, the row of round m4 (src/mh4.c; test/DECISIONS-m4.md): Hash-V-pack with the owner's m4
 * changes, one model.  The same API as mh2.h's rows (MH2_DECLARE), under the prefix mhv4_, so that a test or driver
 * can link it beside mh2.c's Hash-V-pack (mhvp_).  mh2.h is included unchanged, for the declaration macro and m2.h.
 *
 * Per matrix: (a) the cells, two handle fields per cell (2k + tag, the tag at the bottom), and (l) the list, the
 * handle k naming its k-th entry.  During a build: (h), the index table of list positions.  In mmul: the scratch row
 * of A's decoded cells, local to one call.  Nothing else. */
#ifndef MH4_H
#define MH4_H

#include "mh2.h"

#ifdef __cplusplus
extern "C" {
#endif

MH2_DECLARE(mhv4_)

#define MH4F(f) mhv4_##f
#define MH4_NAME "Hash-V-pack-4"

#ifdef __cplusplus
}
#endif
#endif
