/* r2prof.c: the profiling build's clock (spec v0.2 §13.4 items 2 and 6), shared by both rows and the driver: one
 * fenced read of the time-stamp counter, and the probe cost.  In the other builds both return 0. */
#include "r2int.h"

uint64_t r2_tsc(void) {
#ifdef SK_PROFILE
    return r2_pf_read();
#else
    return 0;
#endif
}

#ifdef SK_PROFILE
static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
#endif

double r2_probe_cost(unsigned blocks, unsigned per) {
#ifdef SK_PROFILE
    double *v = sk_xmalloc(sizeof(double) * blocks);
    for (unsigned b = 0; b < blocks; b++) {
        uint64_t t0 = r2_pf_read(), t = t0;
        for (unsigned k = 1; k < per; k++) t = r2_pf_read();
        v[b] = (double)(t - t0) / (double)(per - 1);
    }
    qsort(v, blocks, sizeof(double), cmp_double);
    double m = blocks % 2 ? v[blocks / 2] : (v[blocks / 2 - 1] + v[blocks / 2]) / 2;
    free(v);
    return m;
#else
    (void)blocks, (void)per;
    return 0;
#endif
}
