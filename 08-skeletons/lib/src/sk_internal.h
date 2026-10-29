/* sk_internal.h: the shared integer module (spec §4.3 item 4), counting (spec §12.1), allocation, growth helpers,
 * and the denominator lookup used by Designs 1 and 2 (spec §5).  Every full-width addition, subtraction,
 * multiplication, division and gcd of every row goes through the routines below, and no other full-width
 * arithmetic is done, except negation and absolute value of values that exclude the most negative integer (which
 * cannot overflow; spec §4.3 item 2), and comparisons. */
#ifndef SK_INTERNAL_H
#define SK_INTERNAL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sk_common.h"

#if defined(SK_AUDIT) && !defined(SK_COUNT)
#error "the test build (SK_AUDIT) also needs SK_COUNT"
#endif

#define SK_INT_MIN (-SK_INT_MAX - 1)
#define SK_NONE UINT32_MAX
/* spec §14: fewer than 2^32 - 1 stored values, denominators, divisors and pieces */
#define SK_LIMIT (UINT32_MAX - 1u)

/* ----------------------------------------------------------------------------------------- failure, memory */

_Noreturn void sk_die(const char *msg);
void *sk_xmalloc(size_t n);
void *sk_xcalloc(size_t n, size_t size);
void *sk_xrealloc(void *p, size_t n);

/* Make room for one more element in a list (grow the capacity by doubling).  `count` is the current length. */
#define SK_RESERVE(ptr, count, cap, initial)                                                                      \
    do {                                                                                                          \
        if ((count) >= (cap)) {                                                                                   \
            uint32_t sk_nc_ = (cap) ? ((cap) > UINT32_MAX / 2 ? UINT32_MAX : 2 * (cap)) : (initial);             \
            (ptr) = sk_xrealloc((ptr), (size_t)sk_nc_ * sizeof *(ptr));                                           \
            (cap) = sk_nc_;                                                                                       \
        }                                                                                                         \
    } while (0)

static inline void sk_check_limit(uint32_t count, const char *what) {
    if (count >= SK_LIMIT) {
        fprintf(stderr, "skeletons: limit of spec §14 exceeded: %s\n", what);
        sk_die("limit exceeded");
    }
}

/* ----------------------------------------------------------------------------------------- counting */

#ifdef SK_COUNT
/* The cell every counted routine adds to.  Each public call points it at its class's cell for (phase, part); the
 * audits point it at a scratch cell, so that they change no count of the class. */
extern sk_count *sk_cur;
extern sk_count sk_sink;
#define CNT(field, n) (sk_cur->field += (uint64_t)(n))
#define CNT_SET(cnt, phase, part) (sk_cur = &(cnt).c[(phase)][(part)])
#else
#define CNT(field, n) ((void)0)
#define CNT_SET(cnt, phase, part) ((void)0)
#endif

/* ----------------------------------------------------------------------------------------- bit routines */

/* Raw forms, uncounted: used inside the gcd (counting rule 3) and by instrumentation. */
static inline int sk_ctz_raw(uint64_t x) { return __builtin_ctzll(x); }
static inline int sk_bits_u_raw(uint64_t x) { return x ? 64 - __builtin_clzll(x) : 0; }
static inline sk_uint sk_mag(sk_int a) { return a < 0 ? (sk_uint)0 - (sk_uint)a : (sk_uint)a; }

/* Counted forms, for the algorithms (spec §4.3 item 4, §4.3 item 5). */
static inline int sk_popcount(uint64_t x) {
    CNT(bitops, 1);
    return __builtin_popcountll(x);
}
static inline int sk_hsb(uint64_t x) { /* highest set bit of a nonzero word */
    CNT(bitops, 1);
    return 63 - __builtin_clzll(x);
}
static inline int sk_ctz(uint64_t x) {
    CNT(bitops, 1);
    return __builtin_ctzll(x);
}
static inline int sk_bits(sk_int x) {
    CNT(bitops, 1);
    return sk_bits_u_raw((uint64_t)sk_mag(x));
}

/* ----------------------------------------------------------------------------------------- gcd */

/* The binary method on magnitudes, gcd(a, 0) = |a| (spec §4.3 item 4, G-4).  The result fits sk_int because the
 * most negative integer is never an operand. */
static inline sk_int sk_gcd_raw(sk_int a, sk_int b) {
    sk_uint u = sk_mag(a), v = sk_mag(b);
    if (u == 0) return (sk_int)v;
    if (v == 0) return (sk_int)u;
    int s = sk_ctz_raw((uint64_t)(u | v));
    u >>= sk_ctz_raw((uint64_t)u);
    do {
        v >>= sk_ctz_raw((uint64_t)v);
        if (u > v) {
            sk_uint t = u;
            u = v;
            v = t;
        }
        v -= u;
    } while (v != 0);
    return (sk_int)(u << s);
}

static inline sk_int sk_gcd(sk_int a, sk_int b) {
#ifdef SK_COUNT
    sk_uint ma = sk_mag(a), mb = sk_mag(b);
    int bits = sk_bits_u_raw((uint64_t)(ma > mb ? ma : mb));
    sk_cur->gcd++;
    sk_cur->gcd_hist[bits]++;
    sk_cur->gcd_work += (uint64_t)bits;
#endif
    return sk_gcd_raw(a, b);
}

/* ----------------------------------------------------------------------------------------- checked arithmetic */

/* Each returns true on overflow: a result outside [-SK_INT_MAX, SK_INT_MAX] (spec §4.4 item 1).  No integer wider
 * than w is used: the builtins compute in sk_int and report the wrap. */
static inline bool sk_add_raw(sk_int a, sk_int b, sk_int *r) {
    return __builtin_add_overflow(a, b, r) || *r == SK_INT_MIN;
}
static inline bool sk_sub_raw(sk_int a, sk_int b, sk_int *r) {
    return __builtin_sub_overflow(a, b, r) || *r == SK_INT_MIN;
}
static inline bool sk_mul_raw(sk_int a, sk_int b, sk_int *r) {
    return __builtin_mul_overflow(a, b, r) || *r == SK_INT_MIN;
}
static inline bool sk_add(sk_int a, sk_int b, sk_int *r) {
    CNT(add, 1);
    return sk_add_raw(a, b, r);
}
static inline bool sk_sub(sk_int a, sk_int b, sk_int *r) {
    CNT(add, 1);
    return sk_sub_raw(a, b, r);
}
static inline bool sk_mul(sk_int a, sk_int b, sk_int *r) {
    CNT(mul, 1);
    return sk_mul_raw(a, b, r);
}

/* ----------------------------------------------------------------------------------------- division */

/* Exact division of a by a known divisor b != 0 (the hardware divide). */
static inline sk_int sk_divexact(sk_int a, sk_int b) {
    CNT(div_exact, 1);
    return a / b;
}
/* Division with remainder (the hardware divide), for divisibility tests. */
static inline sk_int sk_divrem(sk_int a, sk_int b, sk_int *rem) {
    CNT(div_rem, 1);
    *rem = a % b;
    return a / b;
}

/* ----------------------------------------------------------------------------------------- overflow record */

typedef struct sk_opctx {
    sk_phase op;
    sk_handle x, y;
} sk_opctx;

static inline sk_status sk_overflow_at(sk_overflow *rec, const sk_opctx *ctx, const char *step) {
    rec->count++;
    rec->op = ctx->op;
    rec->x = ctx->x;
    rec->y = ctx->y;
    rec->step = step;
    CNT(ev_overflows, 1);
    return SK_EOVERFLOW;
}

/* ----------------------------------------------------------------------------------------- the lookup (spec §5) */

typedef struct sk_lookup {
    uint32_t *slot; /* S slots, each a denominator index or SK_NONE */
    uint32_t S;     /* a power of two, initially 16 */
    unsigned log2S;
#ifdef SK_AUDIT
    uint32_t filled; /* test build only: slots written, for the incremental audit's count check */
#endif
} sk_lookup;

void sk_lookup_init(sk_lookup *t);
void sk_lookup_free(sk_lookup *t);
/* Search for v among the denominators dens[].  Returns its index, or SK_NONE with *empty set to the empty slot
 * where the search stopped. */
uint32_t sk_lookup_find(const sk_lookup *t, const sk_int *dens, sk_int v, uint32_t *empty);
/* Place the new index k (dens[k] already appended, so k + 1 denominators).  `empty` is the slot returned by the
 * search that did not find dens[k]. */
void sk_lookup_insert(sk_lookup *t, const sk_int *dens, uint32_t k, uint32_t empty);
/* Audit helpers (raw, called with the counters pointed at the sink). */
const char *sk_lookup_audit_full(const sk_lookup *t, const sk_int *dens, uint32_t D);
const char *sk_lookup_audit_count(const sk_lookup *t, uint32_t D);

/* ----------------------------------------------------------------------------------------- the control's divide */

/* The control's divide (spec §6) on two values in lowest terms, also used by the ceiling's divide (spec §9 item 4).
 * my != 0.  Returns NULL, or on overflow the name of the step. */
const char *sk_ctl_divide_pair(sk_int mx, sk_int dx, sk_int my, sk_int dy, sk_int *n, sk_int *e, bool *fast);

/* ----------------------------------------------------------------------------------------- shared helpers */

void sk_memory_finish(sk_memory *m, uint64_t values);
void sk_memory_add(sk_memory *m, const char *name, uint64_t used, uint64_t allocated);

/* Validate an input integer: the most negative integer is not allowed (spec §4.3 item 2). */
static inline bool sk_valid_input(sk_int v) { return v != SK_INT_MIN; }

#endif
