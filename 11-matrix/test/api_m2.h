/* api_m2.h: where round m2's driver (bench_m2.c) meets the rows (spec 11-matrix v0.2 §15.3), from api_m1.h.  Build
 * with IMPL_TC, or with IMPL_HASH and MH_GROW, MH_PACK (mh2.h: one of the three hash rows), and the build's settings:
 * none (timing), SK_COUNT (counting), SK_COUNT and SK_AUDIT (test), M2_MEM (memory). */
#ifndef API_M2_H
#define API_M2_H

#include <stdio.h>
#include <string.h>

#if defined(IMPL_TC) && !defined(IMPL_HASH)
#include "m2.h"
#include "tcm.h"
typedef tcm_class mat;
#define IMPL_NAME "TC"
#define API(f) tcm_##f
static inline mat *m_create(uint32_t n, int id) {
    (void)id;
    return tcm_create(n, n);
}
#elif defined(IMPL_HASH) && !defined(IMPL_TC)
#include "mh2.h"
#ifndef MHF
#error "IMPL_HASH needs MH_GROW and MH_PACK"
#endif
typedef MHF(class) mat;
#define IMPL_NAME MH_NAME
#define API(f) MHF(f)
#define IMPL_Q 1
static inline mat *m_create(uint32_t n, int id) { return MHF(create)(n, n, id); }
#else
#error "define IMPL_TC, or IMPL_HASH with MH_GROW and MH_PACK"
#endif

#if defined(SK_AUDIT)
#define BUILD_NAME "test"
#elif defined(SK_COUNT)
#define BUILD_NAME "counting"
#elif defined(M2_MEM)
#define BUILD_NAME "memory"
#else
#define BUILD_NAME "timing"
#endif

static inline void api_last_error(const mat *c, FILE *f) {
    sk_overflow o;
    if (API(overflow)(c, &o) == SK_OK)
        fprintf(f, "%s(op=%s,x=%u,y=%u,count=%llu)", o.step ? o.step : "?", sk_phase_name(o.op), (unsigned)o.x,
                (unsigned)o.y, (unsigned long long)o.count);
    else
        fprintf(f, "none-recorded");
}

#define M2_BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(mul) X(add) X(ev_overflows)
#define M2_XFIELDS(X) X(cmp) X(reads) X(zero_results)

/* every nonzero counter of one matrix, per phase and part (entry is counted in each input, an operation in its
 * result): "ctr <name> <phase> <part> <field> <value>" (08's) and "xctr <name> ..." (round 3's: cmp, case_<case>,
 * k<n>_<field>, reads (K-Q3), zero_results) */
static inline void api_dump_counters(mat *m, const char *name, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    static const char *const kf[6] = {"ops", "gcd", "gcd_work", "div", "mul", "add"};
    static sk_counters k;
    static r2_xcounters x;
    if (API(counters)(m, &k, &x) != SK_OK) {
        fprintf(f, "counters not built\n");
        return;
    }
    for (int p = 0; p < SK_PH_READ; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const char *ph = sk_phase_name((sk_phase)p);
#define DUMP1(fld)                                                                                                \
    if (k.c[p][q].fld) fprintf(f, "ctr %s %s %s %s %llu\n", name, ph, part[q], #fld, (unsigned long long)k.c[p][q].fld);
            M2_BASE_FIELDS(DUMP1)
#undef DUMP1
#define DUMP2(fld)                                                                                                \
    if (x.c[p][q].fld) fprintf(f, "xctr %s %s %s %s %llu\n", name, ph, part[q], #fld, (unsigned long long)x.c[p][q].fld);
            M2_XFIELDS(DUMP2)
#undef DUMP2
            for (int z = 0; z < R2_NCASE; z++)
                if (x.c[p][q].cases[z])
                    fprintf(f, "xctr %s %s %s case_%s %llu\n", name, ph, part[q], r2_case_name(z),
                            (unsigned long long)x.c[p][q].cases[z]);
            for (int n = 0; n < R2_NK; n++)
                for (int j = 0; j < 6; j++) {
                    uint64_t v = (&x.c[p][q].k[n].ops)[j];
                    if (v) fprintf(f, "xctr %s %s %s k%d_%s %llu\n", name, ph, part[q], n + 1, kf[j], (unsigned long long)v);
                }
        }
}

/* one store's figures and counters: "stor <when> <name> k=v ..." (TC: none), and a result's operation counters
 * "opc <name> k=v ..." */
static inline void api_dump_storage(mat *m, const char *name, const char *when, int result, FILE *f) {
#ifdef IMPL_Q
    m2_fig g;
    m2_scount s;
    API(figures)(m, &g);
    API(storage)(m, &s, NULL);
    fprintf(f,
            "stor %s %s W=%u T=%u w=%u wh=%u whb=%u packed=%d nf=%u found=%llu new=%llu probes=%llu longest=%llu "
            "widenings=%llu copied=%llu growths=%llu reinserted=%llu rprobes=%llu rewritten=%llu bcells=%llu "
            "bvals=%llu\n",
            when, name, g.W, g.T, g.w, g.wh, g.wh_build, g.packed, g.nf, (unsigned long long)s.found,
            (unsigned long long)s.added, (unsigned long long)s.probes, (unsigned long long)s.longest,
            (unsigned long long)s.widenings, (unsigned long long)s.copied, (unsigned long long)s.growths,
            (unsigned long long)s.reinserted, (unsigned long long)s.rprobes, (unsigned long long)s.rewritten,
            (unsigned long long)g.bytes_cells, (unsigned long long)g.bytes_vals);
    if (result) {
        m2_ocount o;
        API(storage)(m, NULL, &o);
        fprintf(f, "opc %s cross=%llu cross_match=%llu cross_reads=%llu\n", name, (unsigned long long)o.cross,
                (unsigned long long)o.cross_match, (unsigned long long)o.cross_reads);
    }
#else
    (void)m, (void)name, (void)when, (void)result, (void)f;
#endif
}

/* the accounted bytes of one matrix now: its memory report (every structure of it; TC its pairs) */
static inline uint64_t api_used(mat *m) {
    sk_memory rep;
    API(memory)(m, &rep);
    return rep.used;
}

#endif
