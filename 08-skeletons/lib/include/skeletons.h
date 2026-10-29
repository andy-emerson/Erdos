/* skeletons.h: every row of the skeletons library, and a table of function pointers that drives the control,
 * Design 1 and Design 2 through one interface (spec §4.1).  The table is a convenience: the direct calls of
 * sk_ctl.h, sk_d1.h and sk_d2.h do exactly the same thing. */
#ifndef SKELETONS_H
#define SKELETONS_H

#include "sk_ceil.h"
#include "sk_common.h"
#include "sk_ctl.h"
#include "sk_d1.h"
#include "sk_d2.h"

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
