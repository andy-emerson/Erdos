/* rows.c: the function-pointer table of skeletons.h (thin wrappers; no behaviour of their own). */
#include "skeletons.h"

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

