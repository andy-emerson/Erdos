/* api_m1.h: where the matrix round's driver (bench_m1.c) meets the rows (spec 11-matrix v0.1 §3), from round 3's
 * api_r3.h.  Build with IMPL_TC, or with IMPL_HASH and MH_M, MH_GROW, MH_MAP (mh.h: one of the five hash rows), and
 * the build's settings: none (timing), SK_COUNT (counting), SK_COUNT and SK_AUDIT (test). */
#ifndef API_M1_H
#define API_M1_H

#include <stdio.h>
#include <string.h>

#if defined(IMPL_TC) && !defined(IMPL_HASH)
#include "tcm.h"
typedef tcm_class mat;
#define IMPL_NAME "TC"
#define API(f) tcm_##f
static inline mat *m_create(uint32_t n, int role) {
    (void)role;
    return tcm_create(n, n);
}
static inline sk_status m_tau_build(mat *a, mat *b, mat *c) {
    (void)a, (void)b, (void)c;
    return SK_OK;
}
static inline void m_tau_free(mat *c) { (void)c; }
static inline uint64_t m_tau_bytes(const mat *c) {
    (void)c;
    return 0;
}
#elif defined(IMPL_HASH) && !defined(IMPL_TC)
#include "mh.h"
#ifndef MHF
#error "IMPL_HASH needs MH_M, MH_GROW and MH_MAP"
#endif
typedef MHF(class) mat;
#define IMPL_NAME MH_NAME
#define API(f) MHF(f)
#define IMPL_Q 1
static inline mat *m_create(uint32_t n, int role) { return MHF(create)(n, n, role); }
static inline sk_status m_tau_build(mat *a, mat *b, mat *c) { return MHF(tau_build)(a, b, c); }
static inline void m_tau_free(mat *c) { MHF(tau_free)(c); }
static inline uint64_t m_tau_bytes(const mat *c) { return MHF(tau_bytes)(c); }
#else
#error "define IMPL_TC, or IMPL_HASH with MH_M, MH_GROW and MH_MAP"
#endif

#if defined(SK_AUDIT)
#define BUILD_NAME "test"
#elif defined(SK_COUNT)
#define BUILD_NAME "counting"
#else
#define BUILD_NAME "timing"
#endif

static const char *const ROLE[3] = {"A", "B", "C"};

static inline void api_last_error(const mat *c, FILE *f) {
    sk_overflow o;
    if (API(overflow)(c, &o) == SK_OK)
        fprintf(f, "%s(op=%s,x=%u,y=%u,count=%llu)", o.step ? o.step : "?", sk_phase_name(o.op), (unsigned)o.x,
                (unsigned)o.y, (unsigned long long)o.count);
    else
        fprintf(f, "none-recorded");
}

#define M1_BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(mul) X(add) X(ev_overflows)
#define M1_XFIELDS(X) X(cmp) X(reads) X(zero_results)

/* every nonzero counter, per phase and part, summed over the three matrices (entry is counted in A and B, the
 * operation in C): "ctr <phase> <part> <name> <value>" (08's) and "xctr ..." (round 3's: cmp, case_<case>,
 * k<n>_<field>, reads (K-Q3), zero_results) */
static inline void api_dump_counters(mat *const m[3], FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    static const char *const kf[6] = {"ops", "gcd", "gcd_work", "div", "mul", "add"};
    static sk_counters k[3];
    static r2_xcounters x[3];
    for (int r = 0; r < 3; r++)
        if (API(counters)(m[r], &k[r], &x[r]) != SK_OK) {
            fprintf(f, "counters not built\n");
            return;
        }
    for (int p = 0; p < SK_PH_READ; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const char *ph = sk_phase_name((sk_phase)p);
#define SUM1(fld) (k[0].c[p][q].fld + k[1].c[p][q].fld + k[2].c[p][q].fld)
#define SUM2(fld) (x[0].c[p][q].fld + x[1].c[p][q].fld + x[2].c[p][q].fld)
#define DUMP1(fld)                                                                                                \
    if (SUM1(fld)) fprintf(f, "ctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)SUM1(fld));
            M1_BASE_FIELDS(DUMP1)
#undef DUMP1
#define DUMP2(fld)                                                                                                \
    if (SUM2(fld)) fprintf(f, "xctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)SUM2(fld));
            M1_XFIELDS(DUMP2)
#undef DUMP2
            for (int z = 0; z < R2_NCASE; z++)
                if (SUM2(cases[z]))
                    fprintf(f, "xctr %s %s case_%s %llu\n", ph, part[q], r2_case_name(z),
                            (unsigned long long)SUM2(cases[z]));
            for (int n = 0; n < R2_NK; n++) {
                for (int j = 0; j < 6; j++) {
                    uint64_t v = 0;
                    for (int r = 0; r < 3; r++) v += (&x[r].c[p][q].k[n].ops)[j];
                    if (v) fprintf(f, "xctr %s %s k%d_%s %llu\n", ph, part[q], n + 1, kf[j], (unsigned long long)v);
                }
            }
#undef SUM1
#undef SUM2
        }
}

/* each store's figures and counters (spec §9, rule-m1 item 13): "stor <when> <role> k=v ..." (TC: none), and the
 * operation's own counters "opc <when> k=v ..." (C's) */
static inline void api_dump_storage(mat *const m[3], const char *when, FILE *f) {
#ifdef IMPL_Q
    for (int r = 0; r < 3; r++) {
        m1_fig g;
        m1_scount s;
        API(figures)(m[r], &g);
        API(storage)(m[r], &s, NULL);
        fprintf(f,
                "stor %s %s W=%u T=%u w=%u wh=%u nf=%u found=%llu new=%llu probes=%llu longest=%llu widenings=%llu "
                "copied=%llu growths=%llu reinserted=%llu rprobes=%llu rewritten=%llu wprobes=%llu\n",
                when, ROLE[r], g.W, g.T, g.w, g.wh, g.nf, (unsigned long long)s.found, (unsigned long long)s.added,
                (unsigned long long)s.probes, (unsigned long long)s.longest, (unsigned long long)s.widenings,
                (unsigned long long)s.copied, (unsigned long long)s.growths, (unsigned long long)s.reinserted,
                (unsigned long long)s.rprobes, (unsigned long long)s.rewritten, (unsigned long long)s.wprobes);
    }
    m1_ocount o;
    API(storage)(m[2], NULL, &o);
    fprintf(f, "opc %s cross=%llu cross_match=%llu cross_reads=%llu tau_found=%llu tau_none=%llu tau_probes=%llu\n",
            when, (unsigned long long)o.cross, (unsigned long long)o.cross_match, (unsigned long long)o.cross_reads,
            (unsigned long long)o.tau_found, (unsigned long long)o.tau_none, (unsigned long long)o.tau_probes);
#else
    (void)m, (void)when, (void)f;
#endif
}

/* the memory report: "mem <when> <role> <item> <used> <allocated>", "memtau <when> <bytes>", "memN <when> <cells>" */
static inline void api_dump_memory(mat *const m[3], const char *when, FILE *f) {
    uint64_t cells = 0;
    for (int r = 0; r < 3; r++) {
        sk_memory rep;
        API(memory)(m[r], &rep);
        for (unsigned i = 0; i < rep.n; i++)
            fprintf(f, "mem %s %s %s %llu %llu\n", when, ROLE[r], rep.item[i].name, (unsigned long long)rep.item[i].used,
                    (unsigned long long)rep.item[i].allocated);
        cells += rep.values;
    }
    fprintf(f, "memtau %s %llu\n", when, (unsigned long long)m_tau_bytes(m[2]));
    fprintf(f, "memN %s %llu\n", when, (unsigned long long)cells);
}

/* the accounted bytes of the three matrices now (B_used's numerator, spec §9), tau included */
static inline uint64_t api_used(mat *const m[3]) {
    uint64_t u = 0;
    for (int r = 0; r < 3; r++) {
        sk_memory rep;
        API(memory)(m[r], &rep);
        u += rep.used;
    }
    return u + m_tau_bytes(m[2]);
}

#endif
