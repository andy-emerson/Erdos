/* plk.h: the parameterized plain lookup (PLK), L1-Q32's lookup and the lookup gate's (spec §18.1 item 4; lib D-19).
 *
 * It is 08's lookup (vendor08/common.c: start_slot, sk_lookup_init, sk_lookup_find, place, sk_lookup_insert and the
 * two audit helpers), line for line, with the slot width as the parameter.  Like 08's, it is compiled as its own
 * translation unit (src/plk.c) and called from l1.c through these external functions, so L1-Q32 calls its lookup as
 * L1-Q64 calls sk_lookup_find and sk_lookup_insert in common.c: across a translation unit, never inlined into l1.c
 * (one compiler, one set of flags, no link-time optimization; rule item 2).  So the Q32-against-Q64 comparison of L1
 * differs in the slot width, not in inlining (lib D-25).
 *
 * Two builds, chosen by the same setting as l1.c:
 *   -DL1_Q32 (without L1_OPT):  L1-Q32's, 16-bit slots, empty mark 0xFFFF, symbols l1q_plk_*;
 *   -DL1_LKGATE:                the lookup gate's, 32-bit slots, empty mark 0xFFFFFFFF, symbols l1g_plk_*.
 * L1-Q64 (no setting) and both L1-opt rows (L1_OPT) never include this file. */
#ifndef L1_PLK_H
#define L1_PLK_H

#include "sk_internal.h"

#if defined(L1_LKGATE) && !defined(L1_OPT) && !defined(L1_Q32)
typedef uint32_t pslot;
#define PSLOT_EMPTY 0xFFFFFFFFu
#define PLKN(name) l1g_plk_##name
#elif defined(L1_Q32) && !defined(L1_OPT) && !defined(L1_LKGATE)
typedef uint16_t pslot;
#define PSLOT_EMPTY 0xFFFFu
#define PLKN(name) l1q_plk_##name
#else
#error "plk.h: the parameterized lookup is L1-Q32's (-DL1_Q32) or the lookup gate's (-DL1_LKGATE) only"
#endif

typedef struct plookup {
    pslot *slot; /* S slots, each a whole-number index or PSLOT_EMPTY */
    uint32_t S;  /* a power of two, initially 16 */
    unsigned log2S;
#ifdef SK_AUDIT
    uint32_t filled;
#endif
} plookup;

/* As sk_lookup_init, sk_lookup_find, sk_lookup_insert, sk_lookup_audit_full and sk_lookup_audit_count (08 common.c):
 * find returns the index of v among W[], or SK_NONE (2^32 - 1, never stored) with *empty set to the empty slot that
 * ended the search; insert places index k (W[k] new) at that slot, or doubles S and places every index again. */
void PLKN(init)(plookup *t);
uint32_t PLKN(find)(const plookup *t, const sk_int *W, sk_int v, uint32_t *empty);
void PLKN(insert)(plookup *t, const sk_int *W, uint32_t k, uint32_t empty);
const char *PLKN(audit_full)(const plookup *t, const sk_int *W, uint32_t D);
const char *PLKN(audit_count)(const plookup *t, uint32_t D);

#endif
