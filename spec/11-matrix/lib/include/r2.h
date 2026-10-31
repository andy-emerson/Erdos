/* r2.h: what the two rows of round 2 share (spec 10-interning v0.2): the cases of §9, the counters of §13.1 beyond
 * 08's, the known-answer switches of the test build (§14.3) and the profile report of the profiling build (§13.4).
 *
 * Rows (each its own source; the retired v0.1 rows of l1.h are not touched):
 *   q32_*  Q32, layer 1 with 16-bit index fields (src/q32.c, with src/plk.c built -DL1_Q32 as its lookup), q32.h;
 *   tc_*   TC, the textbook control: pairs of 64-bit values in lowest terms (src/tc.c), tc.h.
 *
 * Builds (spec §6 item 4), one source each, differing only in compile-time settings:
 *   timing     nothing defined;
 *   counting   SK_COUNT;
 *   test       SK_COUNT and SK_AUDIT;
 *   profiling  SK_PROFILE alone (x86-64: fenced time-stamp-counter probes on sampled calls).
 */
#ifndef R2_H
#define R2_H

#include "sk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#if SK_WIDTH != 64
#error "round 2 is built at width 64 only (spec v0.2 §1)"
#endif
#if defined(SK_PROFILE) && (defined(SK_COUNT) || defined(SK_AUDIT))
#error "the profiling build is SK_PROFILE alone (spec v0.2 §13.4 item 1)"
#endif

/* The cases of spec §9, in its order within each algorithm.  Add and subtract share theirs. */
typedef enum r2_case {
    R2_ADD_ZERO = 0,
    R2_ADD_CROSS,
    R2_ADD_DEN,
    R2_ADD_NUM,
    R2_ADD_GEN,
    R2_MUL_ONE_X,
    R2_MUL_ONE_Y,
    R2_MUL_BOTH,
    R2_MUL_DEN,
    R2_MUL_NUM,
    R2_MUL_CROSS_AD,
    R2_MUL_CROSS_CB,
    R2_MUL_GEN,
    R2_DIV_ONE_X,
    R2_DIV_ONE_Y,
    R2_DIV_BOTH,
    R2_DIV_CROSS_AD,
    R2_DIV_CROSS_CB,
    R2_DIV_NUM,
    R2_DIV_DEN,
    R2_DIV_GEN,
#ifdef R3_H
    /* Round 3 only (r3.h, included first, defines R3_H), amendment of Oct 29, 2026 (spec v0.3 §17): the integer cases,
     * appended so that every case above keeps its number.  Round 2's builds (which never include r3.h) are unchanged.
     * Their place in each algorithm's order of checks is v0.3 §17.3's. */
    R2_ADD_INT_BOTH,
    R2_ADD_INT_X,
    R2_ADD_INT_Y,
    R2_MUL_INT_BOTH,
    R2_MUL_INT_X,
    R2_MUL_INT_Y,
    R2_ADD_ZERO_X,
    R2_ADD_ZERO_Y,
    R2_MUL_ZERO,
    R2_DIV_ZERO,
#endif
    R2_NCASE,
    /* the profile's two further "cases": entry and read-out (spec §13.4 item 5) */
    R2_ENTRY = R2_NCASE,
    R2_READ,
    R2_NCASEX
} r2_case;
#ifdef R3_H
#define R2_INT_CASES 1 /* this translation unit has round 3's integer cases (r3.h checks it) */
#endif

static inline const char *r2_case_name(int k) {
    static const char *const names[R2_NCASEX] = {
        "add_zero", "add_cross", "add_den", "add_num", "add_general",
        "mul_one_x", "mul_one_y", "mul_both", "mul_den", "mul_num", "mul_cross_ad", "mul_cross_cb", "mul_general",
        "div_one_x", "div_one_y", "div_both", "div_cross_ad", "div_cross_cb", "div_num", "div_den", "div_general",
#ifdef R3_H
        "add_int_both", "add_int_x", "add_int_y", "mul_int_both", "mul_int_x", "mul_int_y", "add_zero_x", "add_zero_y",
        "mul_zero", "div_zero",
#endif
        "entry", "read"};
    return (unsigned)k < R2_NCASEX ? names[k] : "?";
}

/* The known-answer skips K-1 to K-15 of spec §11.1, at positions 0 to 14. */
enum { R2_NK = 15 };

/* One skip's counters (spec §13.1): the operations that took it, and the arithmetic it skipped, measured against the
 * same operation with every known-answer skip off (lib/DECISIONS.md R2-D6). */
typedef struct r2_kcount {
    uint64_t ops, gcd, gcd_work, div, mul, add;
} r2_kcount;

/* The counters of spec §13.1 beyond 08's (held in sk_counters), one cell per phase and part, as 08's. */
typedef struct r2_xcount {
    uint64_t cmp;                /* the comparisons of §9: 4 per add/sub, 6 per mul/div that reaches them */
    uint64_t cases[R2_NCASE];    /* operations that took each case */
    r2_kcount k[R2_NK];          /* K-1 .. K-15 */
    uint64_t kq1[R2_NCASE];      /* Q32: K-Q1 lookups skipped, per case */
    uint64_t kq2[R2_NCASE];      /* Q32: K-Q2 lookups skipped, per case */
    uint64_t reads;              /* Q32: K-Q3, whole-number reads made by the operations */
    uint64_t reads_skipped;      /* Q32: K-Q3, reads skipped against "read all four" */
    uint64_t intern_found;       /* Q32: intern calls that found the whole number */
    uint64_t intern_appended;    /* Q32: intern calls that appended it */
    uint64_t zero_results;       /* results stored as 0/1 */
} r2_xcount;

typedef struct r2_xcounters {
    r2_xcount c[SK_NPHASE][SK_NPART];
} r2_xcounters;

/* Test build: the known-answer switch (spec §14.3).  Bit n-1 turns K-n off; R2_KOFF_HINTS turns off Q32's K-Q1 and
 * K-Q2 hints (every whole number of every result is then looked up).  K-12 and K-15 are cases, not switches: they are
 * sent to the general case by the shortcuts switch only (lib/DECISIONS.md R2-D7). */
#define R2_KOFF(n) (1u << ((n) - 1))
#define R2_KOFF_HINTS (1u << 15)
#define R2_KOFF_ALL (R2_KOFF_HINTS | 0x7FFFu)

/* ----------------------------------------------------------------------------------------- the profile (§13.4) */

/* Profile phases: F operand fetch, C shortcut checks, AG a gcd, AR the rest of the arithmetic, I interning, S the
 * record write, M memory management (growth).  U, the unattributed rest of a sampled call, is computed by the driver
 * from the outer total. */
enum { R2_PF_F = 0, R2_PF_C, R2_PF_AG, R2_PF_AR, R2_PF_I, R2_PF_S, R2_PF_M, R2_NPF };
/* Job phases: entry, add, sub, mul, div (sk_phase), and read-out at SK_PH_READ (5). */
enum { R2_NJP = SK_NPHASE };

typedef struct r2_profile {
    uint64_t cyc[R2_NJP][R2_NCASEX][R2_NPF];   /* sampled calls: cycles per phase (M: growths inside them) */
    uint64_t segs[R2_NJP][R2_NCASEX][R2_NPF];  /* sampled calls: probe intervals charged to each phase */
    uint64_t calls[R2_NJP][R2_NCASEX];         /* sampled calls */
    uint64_t outer[R2_NJP][R2_NCASEX];         /* the driver's outer totals of the sampled calls (r2_prof_outer) */
    uint64_t m_cyc[R2_NJP];                    /* every growth, sampled or not: cycles */
    uint64_t m_n[R2_NJP];                      /*   and the number of growths */
} r2_profile;

/* The probe cost: cycles of one fenced read, the median over `blocks` blocks of `per` back-to-back reads (spec
 * §13.4 item 6).  Profiling build only; 0 in the others. */
double r2_probe_cost(unsigned blocks, unsigned per);
/* One fenced read of the time-stamp counter (profiling build; 0 in the others). */
uint64_t r2_tsc(void);

#ifdef __cplusplus
}
#endif
#endif
