/* mh3.h: the two factored rows of round m3 (spec 11-matrix v0.3 §§16.1, 16.3-16.8), one source (src/mh3.c) built two
 * ways, differing only in the compile-time setting
 *   MH3_COPRIME  0: Pack-primes (prefix mhpp_): factors by schoolbook trial division;
 *                1: Pack-coprime (prefix mhpc_): factors over the matrix's coprime base (q), by gcds alone.
 * Both build every matrix exactly as m2's Hash-V-pack (mh2.c, unchanged): the build table (h) of T_fix slots, the
 * packed list (l), the pack map (p), the cells rewritten in place; then, after the pack, factor each whole number of
 * (l) once, in list order, into the factorization list (f).  Every row is V; no table holds 0 or 1 in advance.
 *
 * Per matrix: (a) the cells, (l) the list, (f) the factorization list; Pack-coprime also (q) the coprime base.
 * Temporaries: (w) a widened (h); (h) and (p) during a build; (f') a rebuilt (f), inside one rebuild (Pack-coprime);
 * (r) the denominator-factor lists D_L, D_R, during each operation step (Pack-coprime).  Scratch (v0.3 §16.3): (t) a
 * temporary's denominator list, the merge's gcd list, refine's working set S.  Nothing else. */
#ifndef MH3_H
#define MH3_H

#include "m3.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MH3_DECLARE(P)                                                                                            \
    typedef struct P##class P##class;                                                                             \
    P##class *P##create(uint32_t rows, uint32_t cols, int id);                                                    \
    void P##destroy(P##class *x);                                                                                 \
    sk_status P##enter(P##class *x, sk_int m, sk_int d);                                                          \
    sk_status P##madd(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##mmul(P##class *a, P##class *b, P##class *c);                                                     \
    sk_status P##read(const P##class *x, uint32_t cell, sk_int *m, sk_int *d);                                    \
    void P##memory(const P##class *x, sk_memory *report);                                                         \
    void P##figures(const P##class *x, m2_fig *f);                                                                \
    void P##ffigures(const P##class *x, m3_ffig *f);                                                              \
    sk_status P##counters(const P##class *x, sk_counters *report, r2_xcounters *xreport);                       \
    sk_status P##storage(const P##class *x, m2_scount *s, m2_ocount *o);                                         \
    sk_status P##fcounters(const P##class *x, m3_fcount *f, m3_acount *a);                                       \
    sk_status P##overflow(const P##class *x, sk_overflow *report);                                                \
    sk_status P##audit(P##class *x, const char **why);                                                            \
    sk_status P##t_whole(const P##class *x, uint32_t slot, sk_int *v);                                            \
    sk_status P##t_field(const P##class *x, uint32_t f, uint32_t *v);                                             \
    sk_status P##t_nfac(const P##class *x, uint32_t k, uint32_t *n);                                              \
    sk_status P##t_fac(const P##class *x, uint32_t k, uint32_t j, sk_int *q, unsigned *e);                        \
    sk_status P##t_piece(const P##class *x, uint32_t i, sk_int *q);                                               \
    sk_status P##t_coprime(const P##class *x, const char **why);                                                  \
    sk_status P##t_force_general(P##class *x, int on);                                                            \
    sk_status P##t_known_off(P##class *x, unsigned mask);                                                         \
    sk_status P##t_corrupt_field(P##class *x, uint32_t f, uint32_t v);                                            \
    sk_status P##t_corrupt_slot(P##class *x, uint32_t slot, sk_int v);                                            \
    sk_status P##t_corrupt_fac(P##class *x, uint32_t k, uint32_t j, sk_int q, unsigned e);

/* As mh2.h's Hash-V-pack: create(rows, cols, id): an input (id < 4: 64-bit, entered) or a result (8 bits, built by
 * madd or mmul); an input is packed and factored inside its last cell's enter; madd and mmul (Pack-coprime: after
 * reconciling a and b, §16.7) build c, pack it and factor it at the end of the call.  ffigures: (f)'s and (q)'s sizes.
 * fcounters: the matrix's factoring counts, and (a result) its step's factored-arithmetic counts.  The t_ functions are
 * the test build's (SK_ENOTBUILT otherwise): t_nfac and t_fac read entry j of whole number k's factorization in (f);
 * t_piece reads piece i of (q) (Pack-coprime); t_coprime checks (q) pairwise coprime by all pairs. */
MH3_DECLARE(mhpp_)
MH3_DECLARE(mhpc_)

#if defined(MH3_COPRIME)
#if !MH3_COPRIME
#define MH3F(f) mhpp_##f
#define MH3_NAME "Pack-primes"
#else
#define MH3F(f) mhpc_##f
#define MH3_NAME "Pack-coprime"
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
