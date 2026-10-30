/* mh.h: the five hash rows of the matrix round (spec 11-matrix v0.1 §§3, 5, 7), one source (src/mh.c) built five
 * ways, differing only in compile-time settings:
 *   MH_M     0: V, cross tests compare values (§7.1);  1: M, the translation tau from B's handles to A's (§7.2)
 *   MH_GROW  0: fixed, T_fix slots, never resized (§5.3);  1: grow, from 16 slots, doubling (§5.6)
 *   MH_MAP   1: the slot-map rewrite (Hash-V-grow-map only); 0: the lookup rewrite (grow rows) or none (fixed)
 * giving the prefixes mhvf_ (Hash-V-fixed), mhmf_ (Hash-M-fixed), mhvg_ (Hash-V-grow), mhmg_ (Hash-M-grow) and mhvgm_
 * (Hash-V-grow-map).  Every prefix is declared here, so that a test can link all five; a translation unit compiled
 * with MH_M, MH_GROW and MH_MAP defined also gets MHF(f), its own prefix, and MH_NAME.
 *
 * Per matrix (A, B, C): (a) the cells, two handle fields of w_h bits per cell (2k + tag, the tag at the bottom); (b)
 * one open-addressing table of whole numbers, linear probing, round 3's multiplicative hash, -1 empty, slot = handle,
 * 0 and 1 at slots 0 and 1.  A and B are 64-bit; C starts at 8 bits and widens by copy.  The M rows add (t), tau,
 * during the operation; Hash-V-grow-map adds (r), the slot map, inside each growth.  Nothing else. */
#ifndef MH_H
#define MH_H

#include "m1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MH_DECLARE(P)                                                                                             \
    typedef struct P##class P##class;                                                                             \
    P##class *P##create(uint32_t rows, uint32_t cols, int role);                                                  \
    void P##destroy(P##class *x);                                                                                 \
    sk_status P##enter(P##class *x, sk_int m, sk_int d);                                                          \
    sk_status P##tau_build(P##class *a, P##class *b, P##class *c);                                               \
    void P##tau_free(P##class *c);                                                                                \
    uint64_t P##tau_bytes(const P##class *c);                                                                     \
    sk_status P##madd(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##mmul(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##read(const P##class *x, uint32_t cell, sk_int *m, sk_int *d);                                    \
    void P##memory(const P##class *x, sk_memory *report);                                                         \
    void P##figures(const P##class *x, m1_fig *f);                                                                \
    sk_status P##counters(const P##class *x, sk_counters *report, r2_xcounters *xreport);                       \
    sk_status P##storage(const P##class *x, m1_scount *s, m1_ocount *o);                                         \
    sk_status P##overflow(const P##class *x, sk_overflow *report);                                                \
    sk_status P##audit(P##class *x, const char **why);                                                            \
    sk_status P##audit_tau(P##class *a, P##class *b, P##class *c, const char **why);                              \
    sk_status P##t_whole(const P##class *x, uint32_t slot, sk_int *v);                                            \
    sk_status P##t_field(const P##class *x, uint32_t f, uint32_t *v);                                             \
    sk_status P##t_tau(const P##class *c, uint32_t k, uint32_t *v);                                               \
    sk_status P##t_force_general(P##class *x, int on);                                                            \
    sk_status P##t_known_off(P##class *x, unsigned mask);                                                         \
    sk_status P##t_corrupt_field(P##class *x, uint32_t f, uint32_t v);                                            \
    sk_status P##t_corrupt_slot(P##class *x, uint32_t slot, sk_int v);

/* create(rows, cols, role): role M1_A or M1_B (64-bit table) or M1_C (from 8 bits).  enter: the next cell of A or B,
 * in the order of entry (§5.7).  tau_build: M rows, after A and B are complete, at the start of the operation (V rows:
 * nothing, SK_OK); tau_free at its end.  madd, mmul: C = A + B, C = A B, charged to C's counters.  The t_ functions
 * are the test build's (SK_ENOTBUILT otherwise). */
MH_DECLARE(mhvf_)
MH_DECLARE(mhmf_)
MH_DECLARE(mhvg_)
MH_DECLARE(mhmg_)
MH_DECLARE(mhvgm_)

#if defined(MH_M) || defined(MH_GROW) || defined(MH_MAP)
#if !defined(MH_M) || !defined(MH_GROW) || !defined(MH_MAP)
#error "define all of MH_M, MH_GROW and MH_MAP (0 or 1)"
#endif
#if MH_MAP && (MH_M || !MH_GROW)
#error "the map rewrite is built on Hash-V-grow only (Hash-V-grow-map, spec §0 item 1)"
#endif
#if !MH_M && !MH_GROW
#define MHF(f) mhvf_##f
#define MH_NAME "Hash-V-fixed"
#elif MH_M && !MH_GROW
#define MHF(f) mhmf_##f
#define MH_NAME "Hash-M-fixed"
#elif !MH_M && MH_GROW && !MH_MAP
#define MHF(f) mhvg_##f
#define MH_NAME "Hash-V-grow"
#elif MH_M && MH_GROW
#define MHF(f) mhmg_##f
#define MH_NAME "Hash-M-grow"
#else
#define MHF(f) mhvgm_##f
#define MH_NAME "Hash-V-grow-map"
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
