/* rows.c: the control's function-pointer table of sk_row.h (thin wrappers; no behaviour of their own).  Design 1's
 * table is in 08-common-divisors/lib/src/rows_d1.c and Design 2's in 09-coprime-base/lib/src/rows_d2.c. */
#include "sk_ctl.h"
#include "sk_row.h"

static void *ctl_create(void) { return sk_ctl_create(); }
static void ctl_destroy(void *c) { sk_ctl_destroy((sk_ctl *)c); }
static sk_status ctl_enter(void *c, sk_int m, sk_int d, sk_handle *o) { return sk_ctl_enter((sk_ctl *)c, m, d, o); }
static sk_status ctl_add(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_ctl_add((sk_ctl *)c, x, y, o); }
static sk_status ctl_sub(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_ctl_sub((sk_ctl *)c, x, y, o); }
static sk_status ctl_mul(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_ctl_mul((sk_ctl *)c, x, y, o); }
static sk_status ctl_div(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_ctl_div((sk_ctl *)c, x, y, o); }
static sk_status ctl_read(const void *c, sk_handle x, sk_int *m, sk_int *d) { return sk_ctl_read((const sk_ctl *)c, x, m, d); }
static uint32_t ctl_size(const void *c) { return sk_ctl_size((const sk_ctl *)c); }
static void ctl_memory(const void *c, sk_memory *r) { sk_ctl_memory((const sk_ctl *)c, r); }
static sk_status ctl_counters(const void *c, sk_counters *r) { return sk_ctl_counters((const sk_ctl *)c, r); }
static sk_status ctl_overflow(const void *c, sk_overflow *r) { return sk_ctl_overflow((const sk_ctl *)c, r); }
static sk_status ctl_audit(void *c, const char **w) { return sk_ctl_audit((sk_ctl *)c, w); }
static sk_status ctl_audit_scheduled(void *c, const char **w) { return sk_ctl_audit_scheduled((sk_ctl *)c, w); }
static sk_status ctl_audit_changed(void *c, const char **w) { return sk_ctl_audit_changed((sk_ctl *)c, w); }
const sk_row sk_row_ctl = {"ctl", ctl_create, ctl_destroy, ctl_enter, ctl_add, ctl_sub, ctl_mul, ctl_div,
    ctl_read, ctl_size, ctl_memory, ctl_counters, ctl_overflow, ctl_audit, ctl_audit_scheduled,
    ctl_audit_changed};
