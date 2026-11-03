/* m1int.h: the per-operation record of the counting and test builds (m1.h), shared by tcm.c and mh.c.  An operation's
 * figures are the change in its phase's core counters across the call: M1_OP_BEGIN takes a snapshot into a variable
 * declared by M1_SNAP, M1_OP_END appends the record.  Both compile to nothing in the timing build. */
#ifndef M1INT_H
#define M1INT_H

#include "m1.h"
#include "r2int.h"

#ifdef SK_COUNT
typedef struct m1_snap {
    uint64_t gcd, gcd_work, div_exact, mul, add;
    uint64_t cases[R2_NCASE];
} m1_snap;

static inline void m1_snap_take(m1_snap *s, const sk_count *k, const r2_xcount *x) {
    s->gcd = k->gcd;
    s->gcd_work = k->gcd_work;
    s->div_exact = k->div_exact;
    s->mul = k->mul;
    s->add = k->add;
    memcpy(s->cases, x->cases, sizeof s->cases);
}

static inline int16_t m1_i16(uint64_t v) {
    if (v > 32767) sk_die("per-operation record: a count over 32,767");
    return (int16_t)v;
}

static inline void m1_percall_put(const m1_snap *s, const sk_count *k, const r2_xcount *x) {
    if (!m1_percall) return;
    if (m1_percall_n >= m1_percall_cap) sk_die("per-operation record: the driver's buffer is full");
    int kase = -1;
    for (int z = 0; z < R2_NCASE; z++)
        if (x->cases[z] != s->cases[z]) {
            if (kase >= 0 || x->cases[z] != s->cases[z] + 1) sk_die("per-operation record: not one case");
            kase = z;
        }
    if (kase < 0) sk_die("per-operation record: no case");
    int16_t *r = m1_percall + M1_PERCALL * m1_percall_n++;
    r[0] = (int16_t)kase;
    r[1] = m1_i16(k->gcd - s->gcd);
    r[2] = m1_i16(k->gcd_work - s->gcd_work);
    r[3] = m1_i16(k->div_exact - s->div_exact);
    r[4] = m1_i16(k->mul - s->mul);
    r[5] = m1_i16(k->add - s->add);
}

#define M1_SNAP(s) m1_snap s
#define M1_OP_BEGIN(c, ph, s)                                                                                     \
    do {                                                                                                          \
        if (m1_percall) m1_snap_take(&(s), &(c)->cnt.c[(ph)][SK_CORE], &(c)->xcnt.c[(ph)][SK_CORE]);            \
    } while (0)
#define M1_OP_END(c, ph, s)                                                                                       \
    do {                                                                                                          \
        if (m1_percall) m1_percall_put(&(s), &(c)->cnt.c[(ph)][SK_CORE], &(c)->xcnt.c[(ph)][SK_CORE]);          \
    } while (0)
#else
#define M1_SNAP(s) int s##_unused_ __attribute__((unused))
#define M1_OP_BEGIN(c, ph, s) ((void)0)
#define M1_OP_END(c, ph, s) ((void)0)
#endif

#endif
