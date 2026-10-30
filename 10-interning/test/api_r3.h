/* api_r3.h: where round 3's driver (bench_r3.c) meets the rows (spec v0.3 §§5, 6), from round 2's api_r2.h
 * (unchanged).  Build with exactly one of IMPL_TC, IMPL_Q32I and IMPL_Q32H, and the build's settings: none (timing),
 * SK_COUNT (counting), SK_COUNT and SK_AUDIT (test), SK_PROFILE (profiling). */
#ifndef API_R3_H
#define API_R3_H

#include <stdio.h>
#include <string.h>

#include "q32h.h"
#include "q32i.h"
#include "tc.h"

#if defined(IMPL_Q32I) && !defined(IMPL_Q32H) && !defined(IMPL_TC)
#define API(f) q32i_##f
typedef q32i_class api_class;
#define IMPL_NAME "Q32I"
#define IMPL_Q 1
#elif defined(IMPL_Q32H) && !defined(IMPL_Q32I) && !defined(IMPL_TC)
#define API(f) q32h_##f
typedef q32h_class api_class;
#define IMPL_NAME "Q32H"
#define IMPL_Q 1
#elif defined(IMPL_TC) && !defined(IMPL_Q32I) && !defined(IMPL_Q32H)
#define API(f) tc_##f
typedef tc_class api_class;
#define IMPL_NAME "TC"
#else
#error "define one of IMPL_TC, IMPL_Q32I, IMPL_Q32H"
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
#ifdef IMPL_Q
    return API(wholes)(c);
#else
    (void)c;
    return 0;
#endif
}

/* Q32-index's list in index order; Q32-hash's table slot by slot, all 32,768 (-1 for an empty slot) */
#ifdef SK_AUDIT
static inline void api_write_wholes(const api_class *c, FILE *f) {
#ifdef IMPL_Q
#ifdef IMPL_Q32I
    uint32_t n = q32i_wholes(c);
#else
    uint32_t n = R3_SLOTS;
#endif
    for (uint32_t k = 0; k < n; k++) {
        sk_int v = -1;
        if (API(t_whole)(c, k, &v) != SK_OK) v = -1;
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

/* lookup_search and lookup_probe are round 2's lookup counters: zero in round 3 (no row has a lookup), and not printed */
#define R2_BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(mul) X(add) X(ev_overflows)
#define R2_XFIELDS(X) X(cmp) X(reads) X(reads_skipped) X(intern_found) X(intern_appended) X(zero_results)

/* every nonzero counter, per phase and part: "ctr <phase> <part> <name> <value>" (08's), "xctr ..." (round 2's: cmp,
 * case_<case>, k<n>_<field>, kq1_<case>, kq2_<case>, reads, reads_skipped, intern_found, intern_appended,
 * zero_results) and "sctr ..." (round 3's storage counters, the Q32 rows: examined, probes, longest) */
static inline void api_dump_counters(const api_class *c, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    static const char *const kf[6] = {"ops", "gcd", "gcd_work", "div", "mul", "add"};
    sk_counters k;
    r2_xcounters x;
    if (API(counters)(c, &k, &x) != SK_OK) {
        fprintf(f, "counters not built\n");
        return;
    }
#ifdef IMPL_Q
    r3_scounters s;
    API(storage)(c, &s);
#endif
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
#ifdef IMPL_Q
            const r3_scount *sc = &s.c[p][q];
            if (sc->examined) fprintf(f, "sctr %s %s examined %llu\n", ph, part[q], (unsigned long long)sc->examined);
            if (sc->probes) fprintf(f, "sctr %s %s probes %llu\n", ph, part[q], (unsigned long long)sc->probes);
            if (sc->longest) fprintf(f, "sctr %s %s longest %llu\n", ph, part[q], (unsigned long long)sc->longest);
#endif
        }
}

/* the storage figures so far, over every phase (spec v0.3 §11.1; the reports' "after entry" and "end"):
 * "stor <when> found=.. new=.. examined=.. probes=.. longest=.." (zeros for TC) */
static inline void api_dump_storage(const api_class *c, const char *when, FILE *f) {
    uint64_t found = 0, added = 0, ex = 0, pr = 0, lo = 0;
#ifdef SK_COUNT
    r2_xcounters x;
    API(counters)(c, NULL, &x);
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            found += x.c[p][q].intern_found;
            added += x.c[p][q].intern_appended;
        }
#ifdef IMPL_Q
    r3_scounters s;
    API(storage)(c, &s);
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            ex += s.c[p][q].examined;
            pr += s.c[p][q].probes;
            if (s.c[p][q].longest > lo) lo = s.c[p][q].longest;
        }
#endif
#endif
    (void)c;
    fprintf(f, "stor %s found=%llu new=%llu examined=%llu probes=%llu longest=%llu\n", when, (unsigned long long)found,
            (unsigned long long)added, (unsigned long long)ex, (unsigned long long)pr, (unsigned long long)lo);
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
