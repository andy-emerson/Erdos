/* m2common.c: round m2's driver-set globals (m2.h): the growth and pack hooks, and the memory build's accounted running
 * total.  Linked into every m2 binary once, beside m1common.c (m1's per-operation record, unchanged). */
#include "m2.h"

void (*m2_growth_hook)(const m2_growth *g) = 0;
void (*m2_pack_hook)(const m2_pack *p) = 0;
uint64_t m2_acct_live = 0, m2_acct_peak = 0;
