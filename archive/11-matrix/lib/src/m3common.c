/* m3common.c: round m3's driver-set global (m3.h): the factoring hook.  Linked into every m3 binary once, beside
 * m1common.c and m2common.c (unchanged). */
#include "m3.h"

void (*m3_factor_hook)(const m3_factor *f) = 0;
