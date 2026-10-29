/* rows_d2.c: Design 2's function-pointer table of sk_row.h (thin wrappers; no behaviour of their own).  Moved
 * unchanged from the joint library's rows.c, Oct 29, 2026. */
#include "sk_d2.h"
#include "sk_row.h"

static void *d2_create(void) { return sk_d2_create(); }
static void d2_destroy(void *c) { sk_d2_destroy((sk_d2 *)c); }
static sk_status d2_enter(void *c, sk_int m, sk_int d, sk_handle *o) { return sk_d2_enter((sk_d2 *)c, m, d, o); }
static sk_status d2_add(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d2_add((sk_d2 *)c, x, y, o); }
static sk_status d2_sub(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d2_sub((sk_d2 *)c, x, y, o); }
static sk_status d2_mul(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d2_mul((sk_d2 *)c, x, y, o); }
static sk_status d2_div(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d2_div((sk_d2 *)c, x, y, o); }
static sk_status d2_read(const void *c, sk_handle x, sk_int *m, sk_int *d) { return sk_d2_read((const sk_d2 *)c, x, m, d); }
static uint32_t d2_size(const void *c) { return sk_d2_size((const sk_d2 *)c); }
static void d2_memory(const void *c, sk_memory *r) { sk_d2_memory((const sk_d2 *)c, r); }
static sk_status d2_counters(const void *c, sk_counters *r) { return sk_d2_counters((const sk_d2 *)c, r); }
static sk_status d2_overflow(const void *c, sk_overflow *r) { return sk_d2_overflow((const sk_d2 *)c, r); }
static sk_status d2_audit(void *c, const char **w) { return sk_d2_audit((sk_d2 *)c, w); }
static sk_status d2_audit_scheduled(void *c, const char **w) { return sk_d2_audit_scheduled((sk_d2 *)c, w); }
static sk_status d2_audit_changed(void *c, const char **w) { return sk_d2_audit_changed((sk_d2 *)c, w); }
const sk_row sk_row_d2 = {"d2", d2_create, d2_destroy, d2_enter, d2_add, d2_sub, d2_mul, d2_div,
    d2_read, d2_size, d2_memory, d2_counters, d2_overflow, d2_audit, d2_audit_scheduled,
    d2_audit_changed};
