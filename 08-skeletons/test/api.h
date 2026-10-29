/* api.h: the one place where the harness meets the library's C API (spec v0.1 §4.1, §9, §12, §13).
 *
 * bench.c calls API(f) for the calls whose shape is the same in every implementation (create, destroy, enter, add,
 * sub, mul, div, read, memory, enter_vector) and the api_* wrappers below for the rest.  Two back ends:
 *   - lib/ (default): include/skeletons.h, names sk_ctl_*, sk_d1_*, sk_d2_*, sk_ceil_*; build settings SK_WIDTH,
 *     SK_COUNT (counting build), SK_COUNT + SK_AUDIT (test build), reconciled with the headers in lib/include, Oct 28, 2026;
 *   - test/stub/ (-DSK_USE_STUB): a stand-in used only to test the harness; settings SK_COUNTING, SK_AUDIT.
 * BENCH_COUNTING and BENCH_AUDIT tell bench.c which build it is in, for either back end.
 *
 * Build: exactly one of IMPL_CTL, IMPL_D1, IMPL_D2, IMPL_CEIL.
 */
#ifndef SK_API_H
#define SK_API_H

#include <stdio.h>

#ifdef SK_USE_STUB
/* ------------------------------------------------------------------------------------------- the stub */
#include "stub/sk.h"
#if defined(SK_COUNTING)
#define BENCH_COUNTING 1
#endif
#if defined(SK_AUDIT)
#define BENCH_AUDIT 1
#endif
#if defined(IMPL_CTL)
#define API(f) ctl_##f
#elif defined(IMPL_D1)
#define API(f) d1_##f
#elif defined(IMPL_D2)
#define API(f) d2_##f
#elif defined(IMPL_CEIL)
#define API(f) ceil_##f
#endif
typedef API(class) api_class;
static inline int api_audit(api_class *c, int last, const char **msg) { return API(audit)(c, last, msg); }
static inline int api_audit_changed(api_class *c, const char **msg) { return API(audit_changed)(c, msg); }
static inline void api_last_error(const api_class *c, FILE *f) { fprintf(f, "%s", API(last_error)(c)); }
#ifdef IMPL_CEIL
static inline int api_gather(api_class *c, uint32_t v, const uint32_t *pos, const unsigned char *neg, uint32_t len,
                             uint32_t *w) { return ceil_gather(c, v, pos, neg, len, w); }
#endif
static inline void api_dump_counters(const api_class *c, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    sk_counters k;
    API(counters)(c, &k);
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            for (int i = 0; i < SK_NCTR; i++)
                fprintf(f, "ctr %s %s %s %llu\n", sk_phase_names[p], part[q], sk_counter_names[i],
                        (unsigned long long)k.c[p][q][i]);
            for (int b = 0; b <= 64; b++)
                if (k.gcd_hist[p][q][b])
                    fprintf(f, "hist %s %s %d %llu\n", sk_phase_names[p], part[q], b,
                            (unsigned long long)k.gcd_hist[p][q][b]);
        }
}
static inline void api_dump_memory(const api_class *c, const char *when, FILE *f) {
    sk_memory r;
    API(memory)(c, &r);
    for (int i = 0; i < r.nstruct; i++)
        fprintf(f, "mem %s %s %llu %llu\n", when, r.s[i].name, (unsigned long long)r.s[i].used,
                (unsigned long long)r.s[i].alloc);
    fprintf(f, "memN %s %llu\n", when, (unsigned long long)r.nvalues);
}

#else
/* ------------------------------------------------------------------------------------------- lib/ */
#include "skeletons.h"
#if defined(SK_COUNT)
#define BENCH_COUNTING 1
#endif
#if defined(SK_AUDIT)
#define BENCH_AUDIT 1
#endif
#if defined(IMPL_CTL)
#define API(f) sk_ctl_##f
typedef sk_ctl api_class;
#elif defined(IMPL_D1)
#define API(f) sk_d1_##f
typedef sk_d1 api_class;
#elif defined(IMPL_D2)
#define API(f) sk_d2_##f
typedef sk_d2 api_class;
#elif defined(IMPL_CEIL)
#define API(f) sk_ceil_##f
typedef sk_ceil api_class;
#endif
/* the gate (§13.3 item 2): every scheduled full audit but the last is audit_scheduled (Design 2 above 1,024 pieces:
 * pairwise coprimality incrementally only); the audit after the last call is the complete audit. */
static inline int api_audit(api_class *c, int last, const char **msg) {
    return last ? (int)API(audit)(c, msg) : (int)API(audit_scheduled)(c, msg);
}
static inline int api_audit_changed(api_class *c, const char **msg) { return (int)API(audit_changed)(c, msg); }
static inline void api_last_error(const api_class *c, FILE *f) {
    sk_overflow o;
    if (API(overflow)(c, &o) == SK_OK)
        fprintf(f, "%s(op=%s,x=%u,y=%u,count=%llu)", o.step ? o.step : "?", sk_phase_name(o.op), (unsigned)o.x,
                (unsigned)o.y, (unsigned long long)o.count);
    else
        fprintf(f, "none-recorded");
}
#ifdef IMPL_CEIL
static inline int api_gather(api_class *c, uint32_t v, const uint32_t *pos, const unsigned char *neg, uint32_t len,
                             uint32_t *w) {
    return neg ? (int)sk_ceil_gather_signed(c, v, pos, neg, len, w) : (int)sk_ceil_gather(c, v, pos, len, w);
}
#endif
#define SK_COUNT_FIELDS(X)                                                                                         \
    X(gcd) X(gcd_work) X(div_exact) X(div_rem) X(mul) X(add) X(bitops) X(lookup_search) X(lookup_probe)            \
    X(pair_read) X(scan_and_words) X(scan_popcount_words) X(scan_merge_entries) X(scan_refine_pieces)              \
    X(scan_remainder_pieces) X(scan_split_rows) X(scan_content) X(ref_div_inserted) X(ref_words_shifted)           \
    X(ref_stride_growths) X(ref_splits) X(ref_pieces_created) X(ref_entries_rewritten) X(ref_remainder_pulls)      \
    X(ev_compares) X(ev_pair_hits) X(ev_rows_u1) X(ev_rows_unrefined) X(ev_meetings) X(ev_absorbs)                 \
    X(ev_extra_queued) X(ev_fast_paths) X(ev_new_dens) X(ev_overflows)
static inline void api_dump_counters(const api_class *c, FILE *f) {
    static const char *const part[SK_NPART] = {"core", "store"};
    sk_counters k;
    if (API(counters)(c, &k) != SK_OK) { fprintf(f, "counters not built\n"); return; }
    for (int p = 0; p < SK_NPHASE; p++)
        for (int q = 0; q < SK_NPART; q++) {
            const sk_count *x = &k.c[p][q];
            const char *ph = sk_phase_name((sk_phase)p);
#define DUMP1(fld) fprintf(f, "ctr %s %s %s %llu\n", ph, part[q], #fld, (unsigned long long)x->fld);
            SK_COUNT_FIELDS(DUMP1)
#undef DUMP1
            for (int b = 0; b < SK_HIST_BINS; b++)
                if (x->gcd_hist[b]) fprintf(f, "hist %s %s %d %llu\n", ph, part[q], b, (unsigned long long)x->gcd_hist[b]);
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

#if defined(IMPL_CTL)
#define IMPL_NAME "ctl"
#elif defined(IMPL_D1)
#define IMPL_NAME "D1"
#elif defined(IMPL_D2)
#define IMPL_NAME "D2"
#elif defined(IMPL_CEIL)
#define IMPL_NAME "ceil"
#else
#error "define one of IMPL_CTL, IMPL_D1, IMPL_D2, IMPL_CEIL"
#endif

#endif
