/* sk_common.h: types shared by every row of the skeletons library (spec 08-skeletons v0.1).
 *
 * Build settings (compile-time only, spec §4.3 item 1 and §4.6 item 3):
 *   SK_WIDTH   64 (default) or 32: the width w of every full-width integer.  It is set in this one place; the
 *              library and every program that includes this header MUST be compiled with the same value
 *              (sk_build_width() reports the library's).
 *   SK_COUNT   defined: the counting build (counters compiled in).
 *   SK_AUDIT   defined together with SK_COUNT: the test build (counters and audits compiled in).
 *   Neither:   the timing build.
 *
 * Rows: the control (sk_ctl.h), Design 1 (sk_d1.h), Design 2 (sk_d2.h) and the ceiling (sk_ceil.h).
 * skeletons.h includes all four and a table of function pointers that drives ctl, d1 and d2 identically.
 */
#ifndef SK_COMMON_H
#define SK_COMMON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SK_WIDTH
#define SK_WIDTH 64
#endif

#if SK_WIDTH == 64
typedef int64_t sk_int;  /* a full-width integer */
typedef uint64_t sk_uint; /* its magnitude */
#define SK_INT_MAX INT64_MAX
#elif SK_WIDTH == 32
typedef int32_t sk_int;
typedef uint32_t sk_uint;
#define SK_INT_MAX INT32_MAX
#else
#error "SK_WIDTH must be 64 or 32"
#endif
/* Every stored numerator lies in [-SK_INT_MAX, SK_INT_MAX] and every denominator in [1, SK_INT_MAX] (spec §4.3
 * item 2): the most negative integer is never a valid argument (SK_EINVAL) and never a result (overflow). */

/* A handle is the position of a value in its class (spec §3): 32 bits. */
typedef uint32_t sk_handle;

typedef enum sk_status {
    SK_OK = 0,
    SK_EOVERFLOW = 1, /* spec §4.4: nothing stored; see the row's overflow() for where */
    SK_EDIVZERO = 2,  /* spec §4.5 item 1: enter with d = 0, div with y = 0; nothing stored */
    SK_EINVAL = 3,    /* spec §4.5 item 2: a handle not in the class; also the most negative integer as input,
                         vectors of unequal length, a position out of range (DECISIONS.md) */
    SK_EAUDIT = 4,    /* an audit found a broken invariant; the message names the first one */
    SK_ENOTBUILT = 5  /* counters asked of the timing build, or an audit of a build without audits */
} sk_status;

const char *sk_status_name(sk_status s);

/* Phases (spec §12.1, I-1).  SK_PH_READ exists only for the ceiling, whose read reduces by a gcd (DECISIONS.md). */
typedef enum sk_phase {
    SK_PH_ENTRY = 0,
    SK_PH_ADD = 1,
    SK_PH_SUB = 2,
    SK_PH_MUL = 3,
    SK_PH_DIV = 4,
    SK_PH_READ = 5,
    SK_NPHASE = 6
} sk_phase;

/* Within a phase: the core (everything before store) and the store (lookup, append, the new denominator's
 * structures, and in Design 2 finish, power and refine).  Entry is split the same way. */
typedef enum sk_part { SK_CORE = 0, SK_STORE = 1, SK_NPART = 2 } sk_part;

const char *sk_phase_name(sk_phase p);

#define SK_HIST_BINS 65

/* One cell of counters (spec §12.1).  A field that does not apply to a row stays 0. */
typedef struct sk_count {
    /* gcds */
    uint64_t gcd;                      /* calls of the shared gcd */
    uint64_t gcd_hist[SK_HIST_BINS];   /* by bits of the larger operand's magnitude */
    uint64_t gcd_work;                 /* the sum of those bits */
    /* divisions */
    uint64_t div_exact;                /* exact divisions */
    uint64_t div_rem;                  /* divisions with remainder */
    /* arithmetic */
    uint64_t mul;                      /* full-width multiplications */
    uint64_t add;                      /* full-width additions and subtractions */
    /* bit routines called by the algorithms (spec §4.3 items 4-5; not those inside the gcd) */
    uint64_t bitops;
    /* lookups */
    uint64_t lookup_search;            /* one per hash computed (searches and re-placements on growth) */
    uint64_t lookup_probe;             /* one per slot read */
    uint64_t pair_read;                /* Design 1: pair bits read */
    /* scans */
    uint64_t scan_and_words;           /* Design 1: words ANDed in searches */
    uint64_t scan_popcount_words;      /* Design 1: words popcounted */
    uint64_t scan_merge_entries;       /* Design 2: row entries walked in merges */
    uint64_t scan_refine_pieces;       /* Design 2: pieces tested in refine */
    uint64_t scan_split_rows;          /* Design 2: rows visited in splits (the new row included) */
    uint64_t scan_content;             /* ceiling: numerators visited by content */
    /* refinements */
    uint64_t ref_div_inserted;         /* Design 1: divisors inserted */
    uint64_t ref_words_shifted;        /* Design 1: incidence words shifted */
    uint64_t ref_stride_growths;       /* Design 1: stride growths */
    uint64_t ref_splits;               /* Design 2: splits */
    uint64_t ref_pieces_created;       /* Design 2: pieces created (by a split, or appended by refine) */
    uint64_t ref_entries_rewritten;    /* Design 2: row entries written by splits */
    /* events */
    uint64_t ev_compares;              /* Design 1: compares with the pair bit not set */
    uint64_t ev_pair_hits;             /* Design 1: compares with the pair bit set */
    uint64_t ev_rows_u1;               /* Design 2: new rows built with u = 1 */
    uint64_t ev_rows_refined;          /* Design 2: new rows needing refinement (u > 1) */
    uint64_t ev_fast_paths;            /* all: same-index (control: same-denominator) fast paths */
    uint64_t ev_new_dens;              /* all but the ceiling: new denominators */
    uint64_t ev_overflows;             /* all: operations that returned SK_EOVERFLOW */
} sk_count;

typedef struct sk_counters {
    sk_count c[SK_NPHASE][SK_NPART];
} sk_counters;

/* Memory report (spec §12.2).  Byte counts are of the structures only: temporaries and the test build's change
 * lists are not structures. */
#define SK_MEM_MAX 10
typedef struct sk_mem_item {
    const char *name;
    uint64_t used;      /* entries times entry size, plus every per-row header */
    uint64_t allocated; /* capacity */
} sk_mem_item;

typedef struct sk_memory {
    unsigned n;                 /* items in item[] */
    sk_mem_item item[SK_MEM_MAX];
    uint64_t used, allocated;   /* totals */
    uint64_t values;            /* N: stored values (ceiling: elements of stored vectors) */
    double used_per_rational;   /* bytes per rational: used / N (0 when N = 0) */
    double allocated_per_rational;
} sk_memory;

/* Where the last overflow happened (spec §4.4 item 2). */
typedef struct sk_overflow {
    uint64_t count;    /* overflows so far in this class (all builds) */
    sk_phase op;       /* the operation of the last one */
    sk_handle x, y;    /* its operands (handles; vectors for the ceiling) */
    const char *step;  /* the step, named as in the spec's pseudocode */
} sk_overflow;

/* Build settings of the compiled library. */
int sk_build_width(void);    /* 64 or 32 */
int sk_build_counting(void); /* 1 if counters are compiled in */
int sk_build_audit(void);    /* 1 if audits are compiled in */

#ifdef __cplusplus
}
#endif
#endif
