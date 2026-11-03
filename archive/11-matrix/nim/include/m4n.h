/* m4n.h: the C interface of the Nim build of round m4 (spec v0.4 §17; spec/nim/), for the driver.
 *
 *   nctl_*  Control (src/control.nim): each matrix its cells, r*c pairs (m, d) of 64-bit integers, row-major;
 *   npk_*   Packed  (src/packed.nim):  each matrix its cells (two handle fields each) and (l), the packed list; (h),
 *           the index table, during a build; mul's row buffer inside one call.
 *
 * Both take the mark of §17.10: an overflow leaves 0/0 in the result's cell and the operation goes on, so madd and
 * mmul return SK_OK on overflow; read returns (0, 0) for a marked cell.  The types are m2.h's, so that the driver's
 * printing (api_m4n.h) is the m3/m4 driver's. */
#ifndef M4N_H
#define M4N_H

#include "m2.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "m4n_marks.h"

typedef struct nctl_class nctl_class;
nctl_class *nctl_create(uint32_t rows, uint32_t cols);
void nctl_destroy(nctl_class *x);
sk_status nctl_enter(nctl_class *x, sk_int m, sk_int d);
sk_status nctl_madd(nctl_class *a, nctl_class *b, nctl_class *c);
sk_status nctl_mmul(nctl_class *a, nctl_class *b, nctl_class *c);
sk_status nctl_read(const nctl_class *x, uint32_t cell, sk_int *m, sk_int *d);
void nctl_memory(const nctl_class *x, sk_memory *report);
sk_status nctl_counters(const nctl_class *x, sk_counters *report, r2_xcounters *xreport);
sk_status nctl_marks(const nctl_class *x, m4n_marks *report);
sk_status nctl_audit(nctl_class *x, const char **why);

typedef struct npk_class npk_class;
npk_class *npk_create(uint32_t rows, uint32_t cols, int id);
void npk_destroy(npk_class *x);
sk_status npk_enter(npk_class *x, sk_int m, sk_int d);
sk_status npk_madd(npk_class *a, npk_class *b, npk_class *c);
sk_status npk_mmul(npk_class *a, npk_class *b, npk_class *c);
sk_status npk_read(const npk_class *x, uint32_t cell, sk_int *m, sk_int *d);
void npk_memory(const npk_class *x, sk_memory *report);
void npk_figures(const npk_class *x, m2_fig *f);
sk_status npk_counters(const npk_class *x, sk_counters *report, r2_xcounters *xreport);
sk_status npk_storage(const npk_class *x, m2_scount *s, m2_ocount *o);
sk_status npk_marks(const npk_class *x, m4n_marks *report);
sk_status npk_audit(npk_class *x, const char **why);
/* the bytes of the row buffer of the mul that built x (§17.11); 0 for an input or a sum */
uint64_t npk_rowbuf(const npk_class *x);
/* test build: slot s of (h) as l[h[s]] or -1 during a pack, or position s of (l) once packed; field f's raw value */
sk_status npk_t_whole(const npk_class *x, uint32_t slot, sk_int *v);
sk_status npk_t_field(const npk_class *x, uint32_t f, uint32_t *v);

#ifdef __cplusplus
}
#endif
#endif
