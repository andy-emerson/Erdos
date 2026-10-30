/* m2.h: what round m2 (spec 11-matrix v0.2 §15) adds beside round m1's headers (m1.h, r3.h, r2.h, all used
 * unchanged).  m1's per-operation record (m1_percall, m1.h; m1int.h; m1common.c) is reused as it is: it is the same
 * record, of the same arithmetic.  New here:
 *   - the matrices of a job by id (§15.5): four inputs, 64-bit and entered, and three results, each built by one
 *     operation from 8 bits (two intermediates and the final result; in add and mul only the result is used);
 *   - the store counters of one matrix (as m1's, without the lookup rewrite's probes, which are gone with Hash-V-grow);
 *   - the operation's own counters (as m1's, without tau, gone with the M rows);
 *   - a growth's record and a packing's record (§15.4), as the growing or packing store reports them to the hooks;
 *   - the memory build's accounted running total (§15.6): every hash row adds and subtracts the bytes of each structure
 *     at its allocation and free, and the total's maximum since the driver last reset it.  The memory build is the
 *     timing build with M2_MEM defined; the timing, counting and test builds carry none of it.
 * Rows (each its own source; nothing of m1 and nothing in ../10-interning is changed):
 *   tcm_*   TC, tcm.c unchanged (m1's);
 *   mhvf_*, mhvgm_*, mhvp_*  Hash-V-fixed, Hash-V-grow-map, Hash-V-pack: one source (src/mh2.c) built three ways
 *   (mh2.h). */
#ifndef M2_H
#define M2_H

#include "m1.h" /* first: r3.h, r2.h; and the per-operation record */

#ifdef __cplusplus
extern "C" {
#endif

/* The matrices of a job (spec v0.2 §15.5): A, B, C, D are inputs (64-bit, entered in that order); X1 and X2 are the
 * two intermediates of an expression (P, Q in sop; S, U in pos) and R its result (C = A + B or C = A B in add and mul,
 * called R in this code).  An id below 4 is an input. */
enum { M2_A = 0, M2_B = 1, M2_C = 2, M2_D = 3, M2_X1 = 4, M2_X2 = 5, M2_R = 6, M2_NMAT = 7 };
#define M2_INPUT(id) ((id) >= M2_A && (id) <= M2_D)

/* One matrix's store counters (counting builds; zero in TC). */
typedef struct m2_scount {
    uint64_t found;      /* whole-number stores that found the value */
    uint64_t added;      /* whole-number stores that wrote a new value */
    uint64_t probes;     /* slots read by the stores' probe sequences (a store after a growth: both sequences) */
    uint64_t longest;    /* the most probes of one store */
    uint64_t widenings;  /* results only: widenings (v0.1 §5.5) */
    uint64_t copied;     /* results only: slots copied by the widenings */
    uint64_t growths;    /* Hash-V-grow-map: growths */
    uint64_t reinserted; /* Hash-V-grow-map: whole numbers reinserted */
    uint64_t rprobes;    /* Hash-V-grow-map: probes while reinserting */
    uint64_t rewritten;  /* Hash-V-grow-map: cell fields rewritten by the growths */
} m2_scount;

/* One operation's own counters (counting builds), held by its result's header; zero in TC. */
typedef struct m2_ocount {
    uint64_t cross;       /* cross tests made (four per operation on two cells) */
    uint64_t cross_match; /* of which matched */
    uint64_t cross_reads; /* whole numbers read before a case is chosen (v0.1 §7.1; DECISIONS M1-D8) */
} m2_ocount;

/* A growth (Hash-V-grow-map), reported inside the growth just before the old table is freed. */
typedef struct m2_growth {
    int id;
    uint32_t fields;     /* handle fields of the matrix written so far */
    uint32_t W;          /* whole numbers in the table before the new one */
    uint32_t T_old;      /* the old size; the new is 2 T_old */
    uint64_t reinserted, rprobes, rewritten;
    uint64_t bytes_new;  /* (n), the grown table */
    uint64_t bytes_map;  /* (r), the slot map */
} m2_growth;

/* A packing (Hash-V-pack, §15.4 item 3), reported after the cells are rewritten and narrowed, just before the build
 * table (h) and the pack map (p) are freed.  x is the matrix (the test build's driver reads (h) through t_whole). */
typedef struct m2_pack {
    int id;
    const void *x;
    uint32_t W;                 /* the list's length */
    uint32_t T;                 /* (h)'s slots, T_fix */
    unsigned w;                 /* the width of (h) and of (l) */
    unsigned wh_build, wh_pack; /* the handle width before and after */
    uint64_t rewritten;         /* cell fields rewritten */
    uint64_t bytes_list;        /* (l) */
    uint64_t bytes_map;         /* (p) */
    uint64_t bytes_table;       /* (h) */
    uint64_t bytes_cells_build, bytes_cells_pack;
} m2_pack;

/* One matrix's figures (every build): dimensions, the table's size (Hash-V-pack: its build table's, T_fix), whole
 * numbers held, handle fields written, widths, and the bytes of its cells and of its table or list. */
typedef struct m2_fig {
    uint32_t R, K, T, W, nf;
    unsigned w, wh, wh_build; /* wh_build: the handle width during the build (Hash-V-pack; else wh) */
    int packed;               /* Hash-V-pack: the matrix is packed (its values are the list (l)) */
    uint64_t bytes_cells, bytes_vals; /* bytes_vals: the table, every slot counted, or the list */
} m2_fig;

/* The hooks: NULL (the default) or the driver's functions.  Each is a read: it changes no state of the store and no
 * control flow.  Defined in src/m2common.c. */
extern void (*m2_growth_hook)(const m2_growth *g);
extern void (*m2_pack_hook)(const m2_pack *p);

/* The accounted running total (spec v0.2 §15.6), memory build only: the bytes of every structure alive, and the
 * largest value since the driver set m2_acct_peak = m2_acct_live.  Defined in src/m2common.c (in every build; only the
 * memory build of the hash rows updates them). */
extern uint64_t m2_acct_live, m2_acct_peak;
#ifdef M2_MEM
static inline void m2_acct(int64_t d) {
    m2_acct_live = (uint64_t)((int64_t)m2_acct_live + d);
    if (m2_acct_live > m2_acct_peak) m2_acct_peak = m2_acct_live;
}
#endif

#ifdef __cplusplus
}
#endif
#endif
