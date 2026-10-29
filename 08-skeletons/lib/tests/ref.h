/* ref.h: an exact reference for the tests, independent of the library's integer routines (spec §13.1): rationals
 * with 128-bit numerator and denominator, its own gcd (Euclid), and a deterministic PRNG.  Inputs are stored
 * values, so every product of two of them fits in 127 bits. */
#ifndef SK_REF_H
#define SK_REF_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "sk_common.h"

typedef __int128 i128;

typedef struct ref_q {
    i128 n, d; /* lowest terms, d > 0 */
} ref_q;

static inline i128 ref_abs(i128 a) { return a < 0 ? -a : a; }

static inline i128 ref_gcd(i128 a, i128 b) {
    a = ref_abs(a);
    b = ref_abs(b);
    while (b) {
        i128 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static inline ref_q ref_make(i128 n, i128 d) {
    if (d < 0) {
        n = -n;
        d = -d;
    }
    i128 g = ref_gcd(n, d);
    if (g == 0) g = 1;
    ref_q r = {n / g, d / g};
    if (r.n == 0) r.d = 1;
    return r;
}

static inline ref_q ref_add(ref_q a, ref_q b) { return ref_make(a.n * b.d + b.n * a.d, a.d * b.d); }
static inline ref_q ref_sub(ref_q a, ref_q b) { return ref_make(a.n * b.d - b.n * a.d, a.d * b.d); }
static inline ref_q ref_mul(ref_q a, ref_q b) {
    /* cross-reduce first so the products stay within 127 bits */
    i128 g1 = ref_gcd(a.n, b.d), g2 = ref_gcd(b.n, a.d);
    if (g1 == 0) g1 = 1;
    if (g2 == 0) g2 = 1;
    return ref_make((a.n / g1) * (b.n / g2), (a.d / g2) * (b.d / g1));
}
static inline ref_q ref_div(ref_q a, ref_q b) { /* b.n != 0 */
    ref_q inv = ref_make(b.d, b.n);
    return ref_mul(a, inv);
}

/* Does the value fit the width's ranges (spec §4.3 item 2)? */
static inline bool ref_fits(ref_q a) {
    i128 M = (i128)SK_INT_MAX;
    return a.n >= -M && a.n <= M && a.d >= 1 && a.d <= M;
}

static inline bool ref_eq(ref_q a, sk_int n, sk_int d) { return a.n == (i128)n && a.d == (i128)d; }

/* H(x) = max(|m|, d) */
static inline i128 ref_height(ref_q a) {
    i128 m = ref_abs(a.n);
    return m > a.d ? m : a.d;
}

/* xorshift64* */
typedef struct rng {
    uint64_t s;
} rng;
static inline uint64_t rng_next(rng *r) {
    r->s ^= r->s >> 12;
    r->s ^= r->s << 25;
    r->s ^= r->s >> 27;
    return r->s * UINT64_C(2685821657736338717);
}
static inline uint64_t rng_below(rng *r, uint64_t n) { return n ? rng_next(r) % n : 0; }
static inline int64_t rng_range(rng *r, int64_t lo, int64_t hi) { /* inclusive */
    uint64_t span = (uint64_t)hi - (uint64_t)lo; /* unsigned: no overflow for wide ranges */
    return (int64_t)((uint64_t)lo + (span == UINT64_MAX ? rng_next(r) : rng_below(r, span + 1)));
}

static inline void ref_print(FILE *f, i128 v) {
    char buf[64];
    int k = 0;
    bool neg = v < 0;
    unsigned __int128 u = neg ? -(unsigned __int128)v : (unsigned __int128)v;
    do {
        buf[k++] = (char)('0' + (int)(u % 10));
        u /= 10;
    } while (u);
    if (neg) fputc('-', f);
    while (k) fputc(buf[--k], f);
}

#endif
