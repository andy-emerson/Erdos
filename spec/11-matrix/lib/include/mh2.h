/* mh2.h: the three hash rows of round m2 (spec 11-matrix v0.2 §§15.3-15.4), one source (src/mh2.c) built three ways,
 * differing only in compile-time settings:
 *   MH_GROW  0: a table of T_fix slots;  1: from 16 slots, doubling before it passes half full, the map rewrite
 *   MH_PACK  1: Hash-V-pack: the table (h) of T_fix slots during the build only, then the packed list (l)
 * giving the prefixes mhvf_ (Hash-V-fixed: 0, 0), mhvgm_ (Hash-V-grow-map: 1, 0) and mhvp_ (Hash-V-pack: 0, 1).  Every
 * row is V (values compared, v0.1 §7.1); no table holds 0 or 1 in advance (§15.2).  These are not m1's rows (mh.h):
 * a test or driver includes one of mh.h and mh2.h, never both.
 *
 * Per matrix: (a) the cells, two handle fields per cell (2k + tag, the tag at the bottom), and (b) the table (fixed,
 * grow-map) or (l) the list (pack).  Temporaries: (w) a widened table; (n) a grown table and (r) the slot map, inside
 * a growth; (h) the build table and (p) the pack map, during a build (pack).  Nothing else. */
#ifndef MH2_H
#define MH2_H

#include "m2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MH2_DECLARE(P)                                                                                            \
    typedef struct P##class P##class;                                                                             \
    P##class *P##create(uint32_t rows, uint32_t cols, int id);                                                    \
    void P##destroy(P##class *x);                                                                                 \
    sk_status P##enter(P##class *x, sk_int m, sk_int d);                                                          \
    sk_status P##madd(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##mmul(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##read(const P##class *x, uint32_t cell, sk_int *m, sk_int *d);                                    \
    void P##memory(const P##class *x, sk_memory *report);                                                         \
    void P##figures(const P##class *x, m2_fig *f);                                                                \
    sk_status P##counters(const P##class *x, sk_counters *report, r2_xcounters *xreport);                       \
    sk_status P##storage(const P##class *x, m2_scount *s, m2_ocount *o);                                         \
    sk_status P##overflow(const P##class *x, sk_overflow *report);                                                \
    sk_status P##audit(P##class *x, const char **why);                                                            \
    sk_status P##t_whole(const P##class *x, uint32_t slot, sk_int *v);                                            \
    sk_status P##t_field(const P##class *x, uint32_t f, uint32_t *v);                                             \
    sk_status P##t_force_general(P##class *x, int on);                                                            \
    sk_status P##t_known_off(P##class *x, unsigned mask);                                                         \
    sk_status P##t_corrupt_field(P##class *x, uint32_t f, uint32_t v);                                            \
    sk_status P##t_corrupt_slot(P##class *x, uint32_t slot, sk_int v);

/* create(rows, cols, id): an input (id < 4: 64-bit, entered) or a result (8 bits, built by madd or mmul).  enter: the
 * next cell of an input, in the order of entry; Hash-V-pack packs the input inside its last cell's enter.  madd, mmul:
 * c = a + b, c = a b, on finished (and, in Hash-V-pack, packed) matrices of any ids, c an empty result; the arithmetic
 * is charged to c; Hash-V-pack packs c at the end of the call.  t_whole reads slot s of the table (or of (h), during a
 * packing), or position s of the list once packed.  The t_ functions are the test build's (SK_ENOTBUILT otherwise). */
MH2_DECLARE(mhvf_)
MH2_DECLARE(mhvgm_)
MH2_DECLARE(mhvp_)

#if defined(MH_GROW) || defined(MH_PACK)
#if !defined(MH_GROW) || !defined(MH_PACK)
#error "define both MH_GROW and MH_PACK (0 or 1)"
#endif
#if MH_GROW && MH_PACK
#error "packing is built on Hash-V-fixed only (Hash-V-pack, spec v0.2 §15.8)"
#endif
#if !MH_GROW && !MH_PACK
#define MHF(f) mhvf_##f
#define MH_NAME "Hash-V-fixed"
#elif MH_GROW
#define MHF(f) mhvgm_##f
#define MH_NAME "Hash-V-grow-map"
#else
#define MHF(f) mhvp_##f
#define MH_NAME "Hash-V-pack"
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
