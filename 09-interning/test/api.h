/* api.h: where the harness meets the C APIs.  Build with exactly one of IMPL_L1, IMPL_L1O, IMPL_L1Q, IMPL_L1OQ,
 * IMPL_L1G, IMPL_CTL (the scalar rows, lib/: liblayer1 sources and the 08 control copied unchanged; L1Q and L1OQ are
 * the Q32 rows, spec §18; L1G is the lookup gate's build, never timed), IMPL_FMPQ, IMPL_MPQ, IMPL_FZ (the library
 * rows, FLINT and GMP as installed).  SK_COUNT: the counting build; SK_COUNT + SK_AUDIT: the test build (scalar rows). */
#ifndef L1_API_H
#define L1_API_H

#include <stdio.h>

#if defined(IMPL_L1) || defined(IMPL_L1O) || defined(IMPL_L1Q) || defined(IMPL_L1OQ) || defined(IMPL_L1G) ||       \
    defined(IMPL_CTL)
#define SCALAR 1
#include "l1.h"
#include "sk_ctl.h"
#if defined(SK_COUNT)
#define BENCH_COUNTING 1
#endif
#if defined(SK_AUDIT)
#define BENCH_AUDIT 1
#endif

#if defined(IMPL_L1)
#define API(f) l1_##f
typedef l1_class api_class;
#define IMPL_NAME "L1"
#define LAYER1 1
#elif defined(IMPL_L1O)
#define API(f) l1o_##f
typedef l1o_class api_class;
#define IMPL_NAME "L1opt"
#define LAYER1 1
#elif defined(IMPL_L1Q)
#define API(f) l1q_##f
typedef l1q_class api_class;
#define IMPL_NAME "L1q"
#define LAYER1 1
#elif defined(IMPL_L1OQ)
#define API(f) l1oq_##f
typedef l1oq_class api_class;
#define IMPL_NAME "L1oq"
#define LAYER1 1
#elif defined(IMPL_L1G)
#define API(f) l1g_##f
typedef l1g_class api_class;
#define IMPL_NAME "L1g"
#define LAYER1 1
#else
#define API(f) sk_ctl_##f
typedef sk_ctl api_class;
#define IMPL_NAME "ctl"
#endif

static inline void api_reserve(api_class *c, uint32_t n) {
#ifdef IMPL_L1O
    l1o_reserve(c, n); /* O-7, untimed, before entry */
#elif defined(IMPL_L1OQ)
    l1oq_reserve(c, n);
#else
    (void)c, (void)n;
#endif
}

static inline int api_counters(const api_class *c, sk_counters *b, l1_xcounters *x) {
#ifdef LAYER1
    return (int)API(counters)(c, b, x);
#else
    if (x) memset(x, 0, sizeof *x);
    return (int)sk_ctl_counters(c, b);
#endif
}

static inline int api_audit(api_class *c, int full, const char **msg) {
    return full ? (int)API(audit)(c, msg) : (int)API(audit_changed)(c, msg);
}

static inline void api_last_error(const api_class *c, FILE *f) {
    sk_overflow o;
    if (API(overflow)(c, &o) == SK_OK)
        fprintf(f, "%s(op=%s,x=%u,y=%u,count=%llu)", o.step ? o.step : "?", sk_phase_name(o.op), (unsigned)o.x,
                (unsigned)o.y, (unsigned long long)o.count);
    else
        fprintf(f, "none-recorded");
}

#define BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(div_rem) X(mul) X(add) X(bitops) X(lookup_search) \
    X(lookup_probe) X(ev_fast_paths) X(ev_overflows)
#define XFIELDS(X) X(idx_cmp) X(gen_g1) X(intern_found) X(intern_appended) X(zero_results) X(o2_skipped0) \
    X(o2_skipped1) X(o3_gcd_skipped) X(o3_gcd_work) X(o4_div_skipped) X(o4_mul_skipped) X(o5_ops) X(o5_add_skipped) \
    X(o5_gcd_skipped) X(o5_gcd_work) X(o5_div_skipped) X(o6_reads_avoided) X(o7_growths_avoided) X(o7_bytes_avoided)

static inline void api_dump_counters(const api_class *c, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    sk_counters k;
    l1_xcounters x;
    if (api_counters(c, &k, &x) != SK_OK) {
        fprintf(f, "counters not built\n");
        return;
    }
    for (int p = 0; p < SK_PH_READ; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const sk_count *b = &k.c[p][q];
            const l1_xcount *e = &x.c[p][q];
            const char *ph = sk_phase_name((sk_phase)p);
#define DUMP1(fld) if (b->fld) fprintf(f, "ctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)b->fld);
            BASE_FIELDS(DUMP1)
#undef DUMP1
            for (int h = 0; h < SK_HIST_BINS; h++)
                if (b->gcd_hist[h]) fprintf(f, "hist %s %s %d %llu\n", ph, part[q], h, (unsigned long long)b->gcd_hist[h]);
#ifdef LAYER1
            static const char *const o1n[L1_NO1] = {"add_den", "mul_cross_ad", "mul_cross_cb", "div_num", "div_den"};
#define DUMP2(fld) if (e->fld) fprintf(f, "xctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)e->fld);
            XFIELDS(DUMP2)
#undef DUMP2
            for (int z = 0; z < L1_NCASE; z++)
                if (e->cases[z])
                    fprintf(f, "xctr %s %s case_%s %llu\n", ph, part[q], l1_case_name((l1_case)z),
                            (unsigned long long)e->cases[z]);
            for (int z = 0; z < L1_NO1; z++)
                if (e->o1_skipped[z])
                    fprintf(f, "xctr %s %s o1_skipped_%s %llu\n", ph, part[q], o1n[z], (unsigned long long)e->o1_skipped[z]);
#else
            (void)e;
#endif
        }
}

static inline void api_dump_memory(const api_class *c, const char *when, FILE *f) {
    sk_memory r;
    API(memory)(c, &r);
    for (unsigned i = 0; i < r.n; i++)
        fprintf(f, "mem %s %s %llu %llu\n", when, r.item[i].name, (unsigned long long)r.item[i].used,
                (unsigned long long)r.item[i].allocated);
    fprintf(f, "memN %s %llu\n", when, (unsigned long long)r.values);
}

#elif defined(IMPL_FMPQ) || defined(IMPL_MPQ) || defined(IMPL_FZ)
#define LIBROW 1
#include <flint/flint.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>
#include <gmp.h>
#if defined(IMPL_FMPQ)
#define IMPL_NAME "fmpq"
#elif defined(IMPL_MPQ)
#define IMPL_NAME "mpq"
#else
#define IMPL_NAME "fz"
#endif
#else
#error "define one of IMPL_L1, IMPL_L1O, IMPL_L1Q, IMPL_L1OQ, IMPL_L1G, IMPL_CTL, IMPL_FMPQ, IMPL_MPQ, IMPL_FZ"
#endif

#endif
