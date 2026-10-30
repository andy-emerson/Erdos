/* sk_row.h: the function-pointer row type of skeletons.h, alone, so that each design's library can define its own
 * row (08-common-divisors/lib/src/rows_d1.c, 09-coprime-base/lib/src/rows_d2.c) without the other design's header.
 * Split out of skeletons.h on Oct 29, 2026, when Designs 1 and 2 became separate specifications; the type is unchanged.
 * Each row is defined only where its design is built; a declaration here needs no definition unless it is used. */
#ifndef SK_ROW_H
#define SK_ROW_H

#include "sk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_row {
    const char *name; /* "ctl", "d1", "d2" */
    void *(*create)(void);
    void (*destroy)(void *c);
    sk_status (*enter)(void *c, sk_int m, sk_int d, sk_handle *out);
    sk_status (*add)(void *c, sk_handle x, sk_handle y, sk_handle *out);
    sk_status (*sub)(void *c, sk_handle x, sk_handle y, sk_handle *out);
    sk_status (*mul)(void *c, sk_handle x, sk_handle y, sk_handle *out);
    sk_status (*div)(void *c, sk_handle x, sk_handle y, sk_handle *out);
    sk_status (*read)(const void *c, sk_handle x, sk_int *m, sk_int *d);
    uint32_t (*size)(const void *c);
    void (*memory)(const void *c, sk_memory *report);
    sk_status (*counters)(const void *c, sk_counters *report);
    sk_status (*overflow)(const void *c, sk_overflow *report);
    sk_status (*audit)(void *c, const char **why);
    sk_status (*audit_scheduled)(void *c, const char **why);
    sk_status (*audit_changed)(void *c, const char **why);
} sk_row;

extern const sk_row sk_row_ctl;
extern const sk_row sk_row_d1;
extern const sk_row sk_row_d2;

#ifdef __cplusplus
}
#endif
#endif
