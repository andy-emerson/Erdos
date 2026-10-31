/* api_m4n.h: where the Nim build's driver (bench_m4n.c) meets the Nim models (include/m4n.h): a copy of
 * test/api_m4.h (unchanged, m4's) with its rows replaced by the two Nim models, IMPL_NPK (Packed, npk_*) and IMPL_NCTL
 * (Control, nctl_*), with api_last_error printing "none-recorded" (an overflow marks a cell and is no error, §17.10),
 * with api_dump_marks, the counts "new" and "passed on" and the mark tests of one operation (§17.10), and with
 * api_dump_reads, Packed's row buffer bytes per mul step and its K-Q3 read counter (§17.11).  The
 * factored rows' printing (IMPL_F) is kept as in api_m4.h and is never compiled here.  Builds as api_m4.h's. */
#ifndef API_M4N_H
#define API_M4N_H

#include <stdio.h>
#include <string.h>

#if defined(IMPL_NPK) && !defined(IMPL_NCTL)
#include "m3.h" /* as api_m4.h: the driver also clears m3's factoring hook */
#include "m4n.h"
typedef npk_class mat;
#define IMPL_NAME "Packed-nim"
#define API(f) npk_##f
#define IMPL_Q 1
static inline mat *m_create(uint32_t n, int id) { return npk_create(n, n, id); }
#elif defined(IMPL_NCTL) && !defined(IMPL_NPK)
#include "m3.h" /* as api_m4.h: the driver also clears m3's factoring hook */
#include "m4n.h"
typedef nctl_class mat;
#define IMPL_NAME "Control-nim"
#define API(f) nctl_##f
static inline mat *m_create(uint32_t n, int id) {
    (void)id;
    return nctl_create(n, n);
}
#else
#error "define IMPL_NPK (Packed) or IMPL_NCTL (Control)"
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
    (void)c;
    fprintf(f, "none-recorded");
}

/* Packed: the row buffer's bytes of the mul step that built the matrix (§17.11), "rowbuf <name> bytes=..", and the
 * K-Q3 read counter summed over the phases (0 in V), "kq3 <name> reads=.." */
static inline void api_dump_reads(mat *m, const char *name, FILE *f) {
#ifdef IMPL_Q
    static r2_xcounters x;
    fprintf(f, "rowbuf %s bytes=%llu\n", name, (unsigned long long)API(rowbuf)(m));
    if (API(counters)(m, NULL, &x) == SK_OK) {
        uint64_t r = 0;
        for (int p = 0; p < SK_NPHASE; p++)
            for (int q = 0; q < SK_NPART; q++) r += x.c[p][q].reads;
        fprintf(f, "kq3 %s reads=%llu\n", name, (unsigned long long)r);
    }
#else
    (void)m, (void)name, (void)f;
#endif
}

/* one operation's overflow record (§17.10), counting and test builds: "mrk <name> new=.. passed=.. tests=.." */
static inline void api_dump_marks(mat *m, const char *name, FILE *f) {
    m4n_marks k;
    if (API(marks)(m, &k) == SK_OK)
        fprintf(f, "mrk %s new=%llu passed=%llu tests=%llu\n", name, (unsigned long long)k.made,
                (unsigned long long)k.passed, (unsigned long long)k.tests);
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
