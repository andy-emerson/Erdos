/* r3.h: what round 3 adds to round 2's shared header (spec 10-interning v0.3): the storage counters of §11.1 that
 * replace round 2's lookup counters.  r2.h (the cases, round 2's counters, the switches and the profile) is used
 * unchanged, so that TC (src/tc.c, unchanged) is built exactly as in round 2.
 *
 * Rows (each its own source; round 2's Q32, src/q32.c with src/plk.c, is retired and not built):
 *   q32i_*  Q32-index: the rationals, and a list of whole numbers in arrival order searched top to bottom (src/q32i.c);
 *   q32h_*  Q32-hash:  the rationals, and a fixed hash table of 32,768 slots, linear probing (src/q32h.c);
 *   tc_*    TC, unchanged from round 2 (src/tc.c).
 * Builds as r2.h: timing (nothing), counting (SK_COUNT), test (SK_COUNT, SK_AUDIT), profiling (SK_PROFILE). */
#ifndef R3_H
#define R3_H

/* r2.h gives round 3's builds the integer cases (spec v0.3 §17) only when it is read after R3_H is defined: every
 * round-3 translation unit includes this header (directly, or by q32i.h or q32h.h) before r2.h, tc.h or r2int.h, so
 * that its case counts and arrays are round 3's.  A unit that read r2.h first would disagree on their sizes. */
#if defined(R2_H) && !defined(R2_INT_CASES)
#error "include r3.h before r2.h, tc.h and r2int.h in round 3 (spec v0.3 §17): the integer cases"
#endif
#include "r2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One cell of the storage counters (spec v0.3 §11.1), per phase and part like round 2's.  The intern calls, split
 * into found and new, are round 2's counters intern_found and intern_appended (r2_xcount): searches in Q32-index,
 * stores in Q32-hash.  Only the store part of a cell is ever nonzero: intern is called by the store alone. */
typedef struct r3_scount {
    uint64_t examined; /* Q32-index: list entries compared with the searched value, over all searches */
    uint64_t probes;   /* Q32-hash: slots read by intern, the home slot included (at least 1 per store) */
    uint64_t longest;  /* Q32-hash: the most probes in one store (a maximum, not a sum) */
} r3_scount;

typedef struct r3_scounters {
    r3_scount c[SK_NPHASE][SK_NPART];
} r3_scounters;

/* Q32-hash's table (spec v0.3 §8): its size, the empty mark and the multiplier of the hash function */
#define R3_SLOTS 32768u
#define R3_EMPTY ((sk_int)-1)
#define R3_PHI UINT64_C(0x9E3779B97F4A7C15)

#ifdef __cplusplus
}
#endif
#endif
