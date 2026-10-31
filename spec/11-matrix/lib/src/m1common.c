/* m1common.c: the round's two driver-set globals (m1.h): the growth hook and the per-operation record.  Linked into
 * every row's binary once; the rows only read them. */
#include "m1.h"

void (*m1_growth_hook)(const m1_growth *g) = 0;
int16_t *m1_percall = 0;
uint64_t m1_percall_n = 0, m1_percall_cap = 0;
