/* api_m4.h: where round m4's driver (bench_m4.c) meets the rows: api_m3.h (unchanged, m3's) with one row added,
 * IMPL_HASH4: Hash-V-pack-4 (mh4.h, src/mh4.c; test/DECISIONS-m4.md), labelled "Hash-V-pack-4".  From api_m3.h, which
 * is from api_m2.h (unchanged, m2's).  Build with IMPL_TC; or IMPL_HASH with MH_GROW=0 and MH_PACK=1 (Hash-V-pack: m2's mh2.c, unchanged); or IMPL_FAC
 * with MH3_COPRIME (mh3.h: Pack-primes or Pack-coprime); and the build's settings: none (timing), SK_COUNT (counting),
 * SK_COUNT and SK_AUDIT (test), M2_MEM (memory). */
#ifndef API_M4_H
#define API_M4_H

#include <stdio.h>
#include <string.h>

#if defined(IMPL_HASH4) && !defined(IMPL_TC) && !defined(IMPL_HASH) && !defined(IMPL_FAC)
#include "m3.h"
#include "mh4.h"
typedef MH4F(class) mat;
#define IMPL_NAME MH4_NAME
#define API(f) MH4F(f)
#define IMPL_Q 1
static inline mat *m_create(uint32_t n, int id) { return MH4F(create)(n, n, id); }
#elif defined(IMPL_TC) && !defined(IMPL_HASH) && !defined(IMPL_FAC)
#include "m3.h"
#include "tcm.h"
typedef tcm_class mat;
#define IMPL_NAME "TC"
#define API(f) tcm_##f
static inline mat *m_create(uint32_t n, int id) {
    (void)id;
    return tcm_create(n, n);
}
#elif defined(IMPL_HASH) && !defined(IMPL_TC) && !defined(IMPL_FAC)
#include "m3.h"
#include "mh2.h"
#ifndef MHF
#error "IMPL_HASH needs MH_GROW and MH_PACK"
#endif
#if MH_GROW || !MH_PACK
#error "round m3's hash row is Hash-V-pack only (MH_GROW=0, MH_PACK=1)"
#endif
typedef MHF(class) mat;
#define IMPL_NAME MH_NAME
#define API(f) MHF(f)
#define IMPL_Q 1
static inline mat *m_create(uint32_t n, int id) { return MHF(create)(n, n, id); }
#elif defined(IMPL_FAC) && !defined(IMPL_TC) && !defined(IMPL_HASH)
#include "mh3.h"
#ifndef MH3F
#error "IMPL_FAC needs MH3_COPRIME"
#endif
typedef MH3F(class) mat;
#define IMPL_NAME MH3_NAME
#define API(f) MH3F(f)
#define IMPL_Q 1
#define IMPL_F 1
static inline mat *m_create(uint32_t n, int id) { return MH3F(create)(n, n, id); }
#else
#error "define IMPL_HASH4, IMPL_TC, IMPL_HASH with MH_GROW=0 and MH_PACK=1, or IMPL_FAC with MH3_COPRIME"
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

#define M3_BASE_FIELDS(X) X(gcd) X(gcd_work) X(div_exact) X(mul) X(add) X(ev_overflows)
#define M3_XFIELDS(X) X(cmp) X(reads) X(zero_results)

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
            M3_BASE_FIELDS(DUMP1)
#undef DUMP1
#define DUMP2(fld)                                                                                                \
    if (x.c[p][q].fld) fprintf(f, "xctr %s %s %s %s %llu\n", name, ph, part[q], #fld, (unsigned long long)x.c[p][q].fld);
            M3_XFIELDS(DUMP2)
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

#ifdef IMPL_F
/* a factoring's counts (v0.3 §16.10): "fac <name> k=v ..." */
#define M3_FFIELDS(X) X(td_cand) X(td_divs) X(td_exact) X(rgcd) X(rdiv) X(splits) X(sgcd) X(sdiv) X(p2tests)
static inline void api_dump_fcount(const char *name, const m3_ffig *g, const m3_fcount *c, FILE *f) {
    fprintf(f, "fac %s W=%u E=%u nq=%u w=%u bytes_f=%llu bytes_q=%llu", name, g->W, g->E, g->nq, g->w,
            (unsigned long long)g->bytes_f, (unsigned long long)g->bytes_q);
#define F1(x) fprintf(f, " " #x "=%llu", (unsigned long long)c->x);
    M3_FFIELDS(F1)
#undef F1
    fprintf(f, "\n");
}
/* a step's factored arithmetic (v0.3 §16.10): "facc <name> k=v ..." */
#define M3_AFIELDS(X)                                                                                             \
    X(merges) X(mentries) X(lwalk) X(tests) X(pgcd) X(fmul) X(lsplits) X(gchecked) X(zero_reads) X(rc_passes)     \
    X(rc_gcd) X(rc_splits) X(rc_DL) X(rc_DR) X(sp_gcd) X(sp_div) X(rebuilds) X(rentries)
static inline void api_dump_acount(const char *name, const m3_acount *a, FILE *f) {
    static const char *const sub[M3_NSUB] = {"num_cop", "num_uneq", "num_eq", "gen_cop", "gen_uneq",
                                             "gen_eq", "mgen_cop", "mgen_one", "mgen_two"};
    fprintf(f, "facc %s", name);
#define A1(x) fprintf(f, " " #x "=%llu", (unsigned long long)a->x);
    M3_AFIELDS(A1)
#undef A1
    for (int k = 0; k < M3_NSUB; k++) fprintf(f, " %s=%llu", sub[k], (unsigned long long)a->sub[k]);
    fprintf(f, "\n");
}
#endif

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
#ifdef IMPL_F
    m3_ffig ff;
    API(ffigures)(m, &ff);
    fprintf(f, "ffig %s %s W=%u E=%u nq=%u w=%u factored=%d bytes_f=%llu bytes_q=%llu\n", when, name, ff.W, ff.E, ff.nq,
            ff.w, ff.factored, (unsigned long long)ff.bytes_f, (unsigned long long)ff.bytes_q);
    if (result) {
        m3_acount a;
        if (API(fcounters)(m, NULL, &a) == SK_OK) api_dump_acount(name, &a, f);
    }
#endif
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
