/* api_r2.h: where round 2's driver (bench_r2.c) meets the rows (spec v0.2 §§5, 6).  Build with exactly one of
 * IMPL_Q32 and IMPL_TC, and the build's settings: none (timing), SK_COUNT (counting), SK_COUNT and SK_AUDIT (test),
 * SK_PROFILE (profiling).  Round 1's api.h is kept unchanged (spec §16). */
#ifndef API_R2_H
#define API_R2_H

#include <stdio.h>
#include <string.h>

#include "q32.h"
#include "tc.h"

#if defined(IMPL_Q32) && !defined(IMPL_TC)
#define API(f) q32_##f
typedef q32_class api_class;
#define IMPL_NAME "Q32"
#elif defined(IMPL_TC) && !defined(IMPL_Q32)
#define API(f) tc_##f
typedef tc_class api_class;
#define IMPL_NAME "TC"
#else
#error "define one of IMPL_Q32, IMPL_TC"
#endif

#if defined(SK_PROFILE)
#define BUILD_NAME "profiling"
#elif defined(SK_AUDIT)
#define BUILD_NAME "test"
#elif defined(SK_COUNT)
#define BUILD_NAME "counting"
#else
#define BUILD_NAME "timing"
#endif

static inline uint32_t api_wholes(const api_class *c) {
#ifdef IMPL_Q32
    return q32_wholes(c);
#else
    (void)c;
    return 0;
#endif
}

#ifdef SK_AUDIT
static inline void api_write_wholes(const api_class *c, FILE *f) {
#ifdef IMPL_Q32
    for (uint32_t k = 0; k < q32_wholes(c); k++) {
        sk_int v = 0;
        q32_t_whole(c, k, &v);
        int64_t w = v;
        fwrite(&w, 8, 1, f);
    }
#else
    (void)c, (void)f;
#endif
}
#endif

static inline void api_last_error(const api_class *c, FILE *f) {
    sk_overflow o;
    if (API(overflow)(c, &o) == SK_OK)
        fprintf(f, "%s(op=%s,x=%u,y=%u,count=%llu)", o.step ? o.step : "?", sk_phase_name(o.op), (unsigned)o.x,
                (unsigned)o.y, (unsigned long long)o.count);
    else
        fprintf(f, "none-recorded");
}

#define R2_BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(mul) X(add) X(lookup_search) X(lookup_probe) X(ev_overflows)
#define R2_XFIELDS(X) X(cmp) X(reads) X(reads_skipped) X(intern_found) X(intern_appended) X(zero_results)

/* every nonzero counter, per phase and part: "ctr <phase> <part> <name> <value>" (08's) and "xctr ..." (round 2's:
 * cmp, case_<case>, k<n>_<field>, kq1_<case>, kq2_<case>, reads, reads_skipped, intern_found, intern_appended,
 * zero_results) */
static inline void api_dump_counters(const api_class *c, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    static const char *const kf[6] = {"ops", "gcd", "gcd_work", "div", "mul", "add"};
    sk_counters k;
    r2_xcounters x;
    if (API(counters)(c, &k, &x) != SK_OK) {
        fprintf(f, "counters not built\n");
        return;
    }
    for (int p = 0; p < SK_PH_READ; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const sk_count *b = &k.c[p][q];
            const r2_xcount *e = &x.c[p][q];
            const char *ph = sk_phase_name((sk_phase)p);
#define DUMP1(fld) if (b->fld) fprintf(f, "ctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)b->fld);
            R2_BASE_FIELDS(DUMP1)
#undef DUMP1
#define DUMP2(fld) if (e->fld) fprintf(f, "xctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)e->fld);
            R2_XFIELDS(DUMP2)
#undef DUMP2
            for (int z = 0; z < R2_NCASE; z++) {
                if (e->cases[z])
                    fprintf(f, "xctr %s %s case_%s %llu\n", ph, part[q], r2_case_name(z), (unsigned long long)e->cases[z]);
                if (e->kq1[z])
                    fprintf(f, "xctr %s %s kq1_%s %llu\n", ph, part[q], r2_case_name(z), (unsigned long long)e->kq1[z]);
                if (e->kq2[z])
                    fprintf(f, "xctr %s %s kq2_%s %llu\n", ph, part[q], r2_case_name(z), (unsigned long long)e->kq2[z]);
            }
            for (int n = 0; n < R2_NK; n++) {
                const uint64_t *v = &e->k[n].ops;
                for (int j = 0; j < 6; j++)
                    if (v[j]) fprintf(f, "xctr %s %s k%d_%s %llu\n", ph, part[q], n + 1, kf[j], (unsigned long long)v[j]);
            }
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

#endif
