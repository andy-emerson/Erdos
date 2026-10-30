/* m1.h: what the matrix round (spec 11-matrix v0.1) adds to round 3's shared headers (r3.h, r2.h, used unchanged):
 * the roles of the three matrices, the store counters of the hash rows (§9), the operation's own counters (cross
 * tests, cross reads, tau's finds), the growth hook that the memory run and the gate read inside each growth (§9), and
 * the per-operation record of the counting and test builds (§10 item 3, §4's TC gate).
 *
 * Rows (each its own source; nothing in ../10-interning is changed):
 *   tcm_*   TC: three arrays of 64-bit pairs (src/tcm.c: tc3.c with only the operand and result interface changed);
 *   mhvf_*, mhmf_*, mhvg_*, mhmg_*, mhvgm_*  the five hash rows, one source (src/mh.c) built five ways (mh.h).
 * Builds (rule-m1 item 2): timing (nothing), counting (SK_COUNT), test (SK_COUNT, SK_AUDIT).  No profiling build. */
#ifndef M1_H
#define M1_H

#include "r3.h" /* first: round 3's cases (v0.3 §17), then r2.h */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SK_PROFILE)
#error "this round has no profiling build (spec v0.1 M-9)"
#endif

/* The three matrices (spec §2), in the order of entry (§5.7): A, then B, then C (filled by the operation). */
enum { M1_A = 0, M1_B = 1, M1_C = 2, M1_NROLE = 3 };

/* One matrix's store counters (spec §9), counting builds; zero in TC. */
typedef struct m1_scount {
    uint64_t found;      /* whole-number stores that found the value (0 and 1 by the store's first test included) */
    uint64_t added;      /* whole-number stores that wrote a new value */
    uint64_t probes;     /* slots read by the stores' probe sequences (a store after a growth: both sequences) */
    uint64_t longest;    /* the most probes of one store */
    uint64_t widenings;  /* C only: widenings (§5.5) */
    uint64_t copied;     /* C only: slots copied by the widenings */
    uint64_t growths;    /* grow rows: growths (§5.6) */
    uint64_t reinserted; /* grow rows: whole numbers reinserted */
    uint64_t rprobes;    /* grow rows: probes while reinserting */
    uint64_t rewritten;  /* grow rows: cell fields rewritten */
    uint64_t wprobes;    /* lookup rows: probes while rewriting */
} m1_scount;

/* The operation's own counters (spec §9), counting builds, held by C's header; zero in TC. */
typedef struct m1_ocount {
    uint64_t cross;       /* cross tests made (four per operation on two cells) */
    uint64_t cross_match; /* of which matched */
    uint64_t cross_reads; /* V rows: whole numbers read for the cross tests (§7.1) */
    uint64_t tau_found;   /* M rows: tau's finds that found the whole number in A */
    uint64_t tau_none;    /* M rows: tau's finds that did not (B's empty slots and 0, 1 make no find) */
    uint64_t tau_probes;  /* M rows: slots of A read by tau's finds */
} m1_ocount;

/* A growth (spec §5.6), as the growing store's call reports it to the hook, inside the growth, just before the old
 * table is freed. */
typedef struct m1_growth {
    int role;                  /* M1_A, M1_B or M1_C */
    uint32_t fields;           /* handle fields of the matrix written so far (the moment of the growth, §5.7) */
    uint32_t W;                /* whole numbers in the table before the new one */
    uint32_t T_old;            /* the old size; the new is 2 T_old */
    uint64_t reinserted, rprobes, rewritten, wprobes;
    uint64_t bytes_new;        /* (n), the grown table */
    uint64_t bytes_map;        /* (r), the slot map (map row only; 0 otherwise) */
} m1_growth;

/* One matrix's figures (spec §9), every build: dimensions, handle width, table size and width, whole numbers held (0
 * and 1 included), handle fields written so far, and the bytes of its cells and its table. */
typedef struct m1_fig {
    uint32_t R, K, T, W, nf;
    unsigned w, wh;
    uint64_t bytes_cells, bytes_table;
} m1_fig;

/* The growth hook: NULL (the default) or the driver's function, called once per growth.  It is a read: it changes no
 * state of the store and no control flow (spec §9).  Defined in src/m1common.c. */
extern void (*m1_growth_hook)(const m1_growth *g);

/* The per-operation record (counting and test builds): when m1_percall is not NULL, every arithmetic operation of
 * the matrix operation (each addition, each product) appends M1_PERCALL int16 values: its case (r2_case), and the
 * gcds, gcd work, exact divisions, multiplications and additions of its core.  The driver sizes the buffer (m1_percall_cap
 * records).  Instrumentation only: it is not a structure of any build and is never read by the rows. */
#define M1_PERCALL 6
extern int16_t *m1_percall;
extern uint64_t m1_percall_n, m1_percall_cap;

#ifdef __cplusplus
}
#endif
#endif
