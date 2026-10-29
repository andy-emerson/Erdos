/* plk.c: the parameterized plain lookup (plk.h), its own translation unit as 08's lookup is in common.c.
 * 08 common.c's start_slot, sk_lookup_init, sk_lookup_find, place, sk_lookup_insert, sk_lookup_audit_full and
 * sk_lookup_audit_count, line for line, with pslot in place of uint32_t and PSLOT_EMPTY in place of SK_NONE, and the
 * Q32 test build's read-back of each index written (spec §18.1 item 3; lib D-22).  Counting as 08's: one lookup search
 * per hash computed, one probe per slot read (spec §13.1). */
#include "plk.h"

#define PLK_PHI UINT64_C(0x9E3779B97F4A7C15)

/* Q32, test build only (spec §18.1 item 3): an index written to a 16-bit slot reads back equal to the index. */
#if defined(L1_Q32) && defined(SK_AUDIT)
#define Q32_READBACK(stored, k)                                                                                   \
    do {                                                                                                          \
        if ((uint32_t)(stored) != (uint32_t)(k)) sk_die("Q32: an index did not fit its 16-bit field or slot");  \
    } while (0)
#else
#define Q32_READBACK(stored, k) ((void)0)
#endif

/* The starting slot: unsigned 64-bit wrapping arithmetic, counted as one lookup search. */
static inline uint32_t start_slot(const plookup *t, sk_int v) {
    CNT(lookup_search, 1);
    return (uint32_t)(((uint64_t)v * PLK_PHI) >> (64 - t->log2S));
}

void PLKN(init)(plookup *t) {
    t->S = 16;
    t->log2S = 4;
    t->slot = sk_xmalloc(sizeof(pslot) * t->S);
    for (uint32_t s = 0; s < t->S; s++) t->slot[s] = PSLOT_EMPTY;
#ifdef SK_AUDIT
    t->filled = 0;
#endif
}

uint32_t PLKN(find)(const plookup *t, const sk_int *W, sk_int v, uint32_t *empty) {
    uint32_t mask = t->S - 1, s = start_slot(t, v);
    for (;;) {
        CNT(lookup_probe, 1);
        pslot k = t->slot[s];
        if (k == PSLOT_EMPTY) {
            if (empty) *empty = s;
            return SK_NONE;
        }
        if (W[k] == v) return k;
        s = (s + 1) & mask;
    }
}

static void place(plookup *t, const sk_int *W, uint32_t k) {
    uint32_t mask = t->S - 1, s = start_slot(t, W[k]);
    for (;;) {
        CNT(lookup_probe, 1);
        if (t->slot[s] == PSLOT_EMPTY) break;
        s = (s + 1) & mask;
    }
    t->slot[s] = (pslot)k;
    Q32_READBACK(t->slot[s], k);
}

void PLKN(insert)(plookup *t, const sk_int *W, uint32_t k, uint32_t empty) {
    uint64_t count = (uint64_t)k + 1;
    if (2 * count > t->S) { /* double S and place every index again, in index order (08 spec §5 item 2) */
        if (t->log2S >= 32) sk_die("lookup too large");
        t->S *= 2;
        t->log2S++;
        t->slot = sk_xrealloc(t->slot, sizeof(pslot) * t->S);
        for (uint32_t s = 0; s < t->S; s++) t->slot[s] = PSLOT_EMPTY;
        for (uint32_t i = 0; i <= k; i++) place(t, W, i);
#ifdef SK_AUDIT
        t->filled = k + 1;
#endif
        return;
    }
    t->slot[empty] = (pslot)k;
    Q32_READBACK(t->slot[empty], k);
#ifdef SK_AUDIT
    t->filled++;
#endif
}

const char *PLKN(audit_full)(const plookup *t, const sk_int *W, uint32_t D) {
    uint64_t held = 0;
    for (uint32_t s = 0; s < t->S; s++) {
        if (t->slot[s] == PSLOT_EMPTY) continue;
        if (t->slot[s] >= D) return "lookup: a slot holds an index not below D";
        held++;
    }
    if (held != D) return "lookup: does not hold exactly D indices";
    if (2 * (uint64_t)D > t->S) return "lookup: more than half full";
    for (uint32_t k = 0; k < D; k++)
        if (PLKN(find)(t, W, W[k], NULL) != k)
            return "lookup: a denominator index is not found at its value (or the list has a duplicate)";
    return NULL;
}

const char *PLKN(audit_count)(const plookup *t, uint32_t D) {
#ifdef SK_AUDIT
    if (t->filled != D) return "lookup: does not hold exactly D indices";
#endif
    if (2 * (uint64_t)D > t->S) return "lookup: more than half full";
    return NULL;
}
