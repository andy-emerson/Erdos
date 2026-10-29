/* r2int.h: internals shared by the two rows of round 2 (src/q32.c, src/tc.c; spec v0.2): the counting cell, the
 * known-answer and shortcut switches of the test build, the sign application (K-13), and the profiling build's probes
 * (§13.4).  Each row's class has the members these macros name: cnt, xcnt, xcur (counting builds), koff and
 * force_general (test build), pf (profiling build), and ctx.
 *
 * Every macro below compiles to nothing, or to the plain statement it wraps, in the builds that do not define its
 * setting: the timing build has no counter, no switch and no probe (spec §6 item 4). */
#ifndef R2INT_H
#define R2INT_H

#include "r2.h"
#include "sk_internal.h"

#ifdef SK_PROFILE
#include <x86intrin.h>
#endif

/* ----------------------------------------------------------------------------------------- counting (§13.1) */

#ifdef SK_COUNT
#define XC(c, field, n) ((c)->xcur->field += (uint64_t)(n))
#define SETCELL(c, phase, part)                                                                                   \
    do {                                                                                                          \
        CNT_SET((c)->cnt, (phase), (part));                                                                       \
        (c)->xcur = &(c)->xcnt.c[(phase)][(part)];                                                                \
    } while (0)
#else
#define XC(c, field, n) ((void)0)
#define SETCELL(c, phase, part) ((void)0)
#endif
/* K-n's counter `field` (ops, gcd, gcd_work, div, mul, add) */
#define KC(c, n, field, v) XC(c, k[(n) - 1].field, v)
/* a case taken: its counter, and K-13's operations (every case but add's `zero` applies a sign; lib R2-D6) */
#define XCASE(c, kase)                                                                                            \
    do {                                                                                                          \
        XC(c, cases[(kase)], 1);                                                                                  \
        KC(c, 13, ops, (kase) != R2_ADD_ZERO);                                                                    \
    } while (0)

/* the gcd work of gcd(p, q) as 08 counts it: bits of the larger magnitude (counting builds only) */
static inline uint64_t r2_work(sk_int p, sk_int q) {
    sk_uint mp = sk_mag(p), mq = sk_mag(q);
    return (uint64_t)sk_bits_u_raw((uint64_t)(mp > mq ? mp : mq));
}

/* ----------------------------------------------------------------------------------------- test switches (§14.3) */

#ifdef SK_AUDIT
#define KON(c, n) (((c)->koff & R2_KOFF(n)) == 0)
#define HINTS(c) (((c)->koff & R2_KOFF_HINTS) == 0)
#define FORCE_GENERAL(c) ((c)->force_general)
#else
#define KON(c, n) 1
#define HINTS(c) 1
#define FORCE_GENERAL(c) 0
#endif

/* ----------------------------------------------------------------------------------------- signs (K-13) */

/* sgn(s, v): v, or -v when neg; a negation, never a multiplication (K-13).  In the test build with K-13 off, a
 * counted multiplication by +1 or -1 instead (it cannot overflow: v is never the most negative integer). */
static inline sk_int r2_sgn(int k13, bool neg, sk_int v) {
    if (k13) return neg ? -v : v;
    sk_int r;
    (void)sk_mul(v, neg ? -1 : 1, &r);
    return r;
}
#ifdef SK_COUNT
/* counted as K-13's skipped multiplication; a function call, so two in one expression are sequenced */
static inline sk_int r2_sgn_counted(uint64_t *k13mul, int k13, bool neg, sk_int v) {
    (*k13mul)++;
    return r2_sgn(k13, neg, v);
}
#define SGN(c, neg, v) r2_sgn_counted(&(c)->xcur->k[12].mul, KON(c, 13), (neg), (v))
#else
#define SGN(c, neg, v) r2_sgn(KON(c, 13), (neg), (v))
#endif

/* ----------------------------------------------------------------------------------------- the profile (§13.4) */

#ifdef SK_PROFILE
typedef struct r2_pf {
    int on;                  /* the sampling flag, set by the driver around a sampled call */
    int cur;                 /* the phase the last read opened */
    uint64_t last;           /* the last read */
    uint64_t seg[R2_NPF];    /* this sampled call: cycles per phase */
    uint64_t nseg[R2_NPF];   /*   and probe intervals per phase */
    int jp, kase;            /* the job phase and case of the last sampled call */
    r2_profile rep;          /* the accumulators */
} r2_pf;

/* every read is fenced on both sides (spec §13.4 item 2) */
static inline uint64_t r2_pf_read(void) {
    _mm_lfence();
    uint64_t t = __rdtsc();
    _mm_lfence();
    return t;
}
static inline void r2_pf_begin(r2_pf *p, int ph) {
    if (p->on) {
        memset(p->seg, 0, sizeof p->seg);
        memset(p->nseg, 0, sizeof p->nseg);
        p->cur = ph;
        p->last = r2_pf_read();
    }
}
/* chained: one read closes the current phase and opens the next */
static inline void r2_pf_to(r2_pf *p, int ph) {
    if (p->on) {
        uint64_t t = r2_pf_read();
        p->seg[p->cur] += t - p->last;
        p->nseg[p->cur]++;
        p->last = t;
        p->cur = ph;
    }
}
static inline void r2_pf_end(r2_pf *p, int jp, int kase) {
    if (p->on) {
        uint64_t t = r2_pf_read();
        p->seg[p->cur] += t - p->last;
        p->nseg[p->cur]++;
        for (int k = 0; k < R2_NPF; k++) {
            p->rep.cyc[jp][kase][k] += p->seg[k];
            p->rep.segs[jp][kase][k] += p->nseg[k];
        }
        p->rep.calls[jp][kase]++;
        p->jp = jp;
        p->kase = kase;
    }
}
#define PF_BEGIN(c, ph) r2_pf_begin((c)->pf, (ph))
#define PF_TO(c, ph) r2_pf_to((c)->pf, (ph))
#define PF_END(c, jp, kase) r2_pf_end((c)->pf, (jp), (kase))
/* M (spec §13.4 item 4): a growth is bracketed on every call, sampled or not, when its condition holds; in a sampled
 * call it is taken out of the phase it happened in (chained to M and back).  Without the condition, stmt runs as in
 * the timing build. */
#define PF_GROW(c, cond, stmt)                                                                                    \
    do {                                                                                                          \
        if (cond) {                                                                                               \
            r2_pf *p_ = (c)->pf;                                                                                  \
            int prev_ = p_->cur;                                                                                  \
            uint64_t g0_, g1_;                                                                                    \
            if (p_->on) {                                                                                         \
                r2_pf_to(p_, R2_PF_M);                                                                            \
                g0_ = p_->last;                                                                                   \
            } else {                                                                                              \
                g0_ = r2_pf_read();                                                                               \
            }                                                                                                     \
            stmt;                                                                                                 \
            if (p_->on) {                                                                                         \
                r2_pf_to(p_, prev_);                                                                              \
                g1_ = p_->last;                                                                                   \
            } else {                                                                                              \
                g1_ = r2_pf_read();                                                                               \
            }                                                                                                     \
            p_->rep.m_cyc[(c)->ctx.op] += g1_ - g0_;                                                              \
            p_->rep.m_n[(c)->ctx.op]++;                                                                           \
        } else {                                                                                                  \
            stmt;                                                                                                 \
        }                                                                                                         \
    } while (0)
#else
#define PF_BEGIN(c, ph) ((void)0)
#define PF_TO(c, ph) ((void)0)
#define PF_END(c, jp, kase) ((void)0)
#define PF_GROW(c, cond, stmt)                                                                                    \
    do {                                                                                                          \
        stmt;                                                                                                     \
    } while (0)
#endif

#endif
