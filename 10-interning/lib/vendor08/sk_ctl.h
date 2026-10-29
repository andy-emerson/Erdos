/* sk_ctl.h: the control: plain reduced rationals by the gcd-first method (spec §6).  The control, Design 1 and Design 2 have identical signatures (spec §4.1); only the
 * prefix and the class type differ.  See ../README.md for the full contract. */
#ifndef SK_CTL_H
#define SK_CTL_H

#include "sk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_ctl sk_ctl;

/* A new, empty class; counters are reset only here (spec §12.1). */
sk_ctl *sk_ctl_create(void);
/* Frees the class and everything in it. */
void sk_ctl_destroy(sk_ctl *c);

/* Stores m/d in lowest terms (sign on the numerator); *out receives its handle.
 * SK_EDIVZERO if d = 0; SK_EINVAL if m or d is the most negative integer. */
sk_status sk_ctl_enter(sk_ctl *c, sk_int m, sk_int d, sk_handle *out);

/* Stores x op y as a new value; *out receives its handle.  Nothing is stored on an error.
 * SK_EINVAL: a handle not in the class.  SK_EDIVZERO: div with y = 0.  SK_EOVERFLOW: see sk_ctl_overflow. */
sk_status sk_ctl_add(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ctl_sub(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ctl_mul(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out);
sk_status sk_ctl_div(sk_ctl *c, sk_handle x, sk_handle y, sk_handle *out);

/* The numerator and denominator of x, in lowest terms.  SK_EINVAL for a handle not in the class. */
sk_status sk_ctl_read(const sk_ctl *c, sk_handle x, sk_int *m, sk_int *d);
/* The number of stored values N. */
uint32_t sk_ctl_size(const sk_ctl *c);
/* Fills the memory report of spec §12.2 (every build). */
void sk_ctl_memory(const sk_ctl *c, sk_memory *report);
/* Copies the counters of spec §12.1.  SK_ENOTBUILT in the timing build (report zeroed). */
sk_status sk_ctl_counters(const sk_ctl *c, sk_counters *report);
/* Where the last overflow happened (spec §4.4 item 2).  SK_OK if one is recorded, SK_EINVAL if none. */
sk_status sk_ctl_overflow(const sk_ctl *c, sk_overflow *report);

/* Audits (spec §13.2), test build only; SK_ENOTBUILT otherwise.  SK_OK on pass; SK_EAUDIT on the first failure,
 * with *why (if why is not NULL) pointing at a message held by the class until its next audit.
 *   audit            the full audit, complete.
 *   audit_scheduled  the full audit as scheduled by the gate (spec §13.3 item 2): identical to audit, except in
 *                    Design 2 once the base has more than 1,024 pieces, when the pairwise coprimality of the base is
 *                    checked incrementally only (each piece created or rewritten since the last audit against every
 *                    other piece).  Use audit for the audit after the last call.
 *   audit_changed    the incremental audit: only what changed since the last audit of any kind. */
sk_status sk_ctl_audit(sk_ctl *c, const char **why);
sk_status sk_ctl_audit_scheduled(sk_ctl *c, const char **why);
sk_status sk_ctl_audit_changed(sk_ctl *c, const char **why);

#ifdef __cplusplus
}
#endif
#endif
