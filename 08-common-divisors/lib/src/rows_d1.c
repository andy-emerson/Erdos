/* rows_d1.c: Design 1's function-pointer table of sk_row.h (thin wrappers; no behaviour of their own).  Moved
 * unchanged from the joint library's rows.c, Oct 29, 2026. */
#include "sk_d1.h"
#include "sk_row.h"

static void *d1_create(void) { return sk_d1_create(); }
static void d1_destroy(void *c) { sk_d1_destroy((sk_d1 *)c); }
static sk_status d1_enter(void *c, sk_int m, sk_int d, sk_handle *o) { return sk_d1_enter((sk_d1 *)c, m, d, o); }
static sk_status d1_add(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d1_add((sk_d1 *)c, x, y, o); }
static sk_status d1_sub(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d1_sub((sk_d1 *)c, x, y, o); }
static sk_status d1_mul(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d1_mul((sk_d1 *)c, x, y, o); }
static sk_status d1_div(void *c, sk_handle x, sk_handle y, sk_handle *o) { return sk_d1_div((sk_d1 *)c, x, y, o); }
static sk_status d1_read(const void *c, sk_handle x, sk_int *m, sk_int *d) { return sk_d1_read((const sk_d1 *)c, x, m, d); }
static uint32_t d1_size(const void *c) { return sk_d1_size((const sk_d1 *)c); }
static void d1_memory(const void *c, sk_memory *r) { sk_d1_memory((const sk_d1 *)c, r); }
static sk_status d1_counters(const void *c, sk_counters *r) { return sk_d1_counters((const sk_d1 *)c, r); }
static sk_status d1_overflow(const void *c, sk_overflow *r) { return sk_d1_overflow((const sk_d1 *)c, r); }
static sk_status d1_audit(void *c, const char **w) { return sk_d1_audit((sk_d1 *)c, w); }
static sk_status d1_audit_scheduled(void *c, const char **w) { return sk_d1_audit_scheduled((sk_d1 *)c, w); }
static sk_status d1_audit_changed(void *c, const char **w) { return sk_d1_audit_changed((sk_d1 *)c, w); }
const sk_row sk_row_d1 = {"d1", d1_create, d1_destroy, d1_enter, d1_add, d1_sub, d1_mul, d1_div,
    d1_read, d1_size, d1_memory, d1_counters, d1_overflow, d1_audit, d1_audit_scheduled,
    d1_audit_changed};
