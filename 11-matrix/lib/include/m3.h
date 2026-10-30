/* m3.h: what round m3 (spec 11-matrix v0.3 §16) adds beside round m2's headers (m2.h, m1.h, r3.h, r2.h, all used
 * unchanged).  m1's per-operation record and m2's matrix ids, store counters, pack record, hooks and accounted running
 * total are reused as they are.  New here, for the two factored rows (Pack-primes, Pack-coprime; src/mh3.c):
 *   - a matrix's factoring counters (§16.10): trial division (Pack-primes), the base's pass 1 and the factorizations'
 *     pass 2 (Pack-coprime), and the factorization list's (f) and the base's (q) sizes;
 *   - a step's factored-arithmetic counters (§16.10), held by the step's result: merges and entries compared,
 *     divisibility tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, the new sub-cases
 *     (§16.5), the obtained gcds checked against Euclid (test build), and the reconciliation's work (§16.7);
 *   - a factoring's record, as the factoring store reports it to the hook at the end of the factoring.
 * Rows (each its own source; nothing of m1, m2 and nothing in ../10-interning is changed):
 *   tcm_*            TC, tcm.c unchanged (m1's);
 *   mhvp_*           Hash-V-pack, m2's mh2.c unchanged (MH_GROW=0, MH_PACK=1);
 *   mhpp_*, mhpc_*   Pack-primes, Pack-coprime: one source (src/mh3.c) built two ways (mh3.h). */
#ifndef M3_H
#define M3_H

#include "m2.h" /* first: m1.h, r3.h, r2.h */

#ifdef __cplusplus
extern "C" {
#endif

/* the new sub-cases of spec v0.3 §16.5, decided by the merges' outcomes */
enum {
    M3_NUM_COP = 0, /* 9a  add num: no common factor */
    M3_NUM_UNEQ,    /* 9b  add num: g > 1, no factor at equal valuation: h = 1 known */
    M3_NUM_EQ,      /* 9c  add num: some factors at equal valuation: h by tests at those only */
    M3_GEN_COP,     /* 10a add general */
    M3_GEN_UNEQ,    /* 10b */
    M3_GEN_EQ,      /* 10c */
    M3_MGEN_COP,    /* 12a mul general: both merges find nothing */
    M3_MGEN_ONE,    /* 12b exactly one of g, h is 1 */
    M3_MGEN_TWO,    /* 12c both above 1 */
    M3_NSUB
};

/* One matrix's factoring (counting builds; spec v0.3 §16.10), made once, after its packing (§16.4). */
typedef struct m3_fcount {
    /* Pack-primes: schoolbook trial division (§16.4 item 3) */
    uint64_t td_cand;  /* candidates d tried (each d whose "while m mod d = 0" ran at least once) */
    uint64_t td_divs;  /* divisions with remainder (each "m mod d") */
    uint64_t td_exact; /* exact divisions (each "m <- m / d") */
    /* Pack-coprime: pass 1, the base (09's absorb scan), and pass 2, the factorizations (§16.4 item 4) */
    uint64_t rgcd;     /* pass 1: the scan's gcds, gcd(rest, q) */
    uint64_t rdiv;     /* pass 1: divisions with remainder dividing rest by a piece "as many times as it goes" */
    uint64_t splits;   /* pass 1: splits of a piece */
    uint64_t sgcd;     /* pass 1: gcds inside the splits (the coprime base of {h, q/h}, notation note §4) */
    uint64_t sdiv;     /* pass 1: divisions inside the splits (residuals, and each piece's exponent in q) */
    uint64_t p2tests;  /* pass 2: divisibility tests */
} m3_fcount;

/* One step's factored arithmetic (counting builds; v0.3 §16.10), held by the step's result. */
typedef struct m3_acount {
    uint64_t merges;      /* merges of two sorted factorizations */
    uint64_t mentries;    /* factor comparisons made by the merges */
    uint64_t lwalk;       /* factor comparisons made while forming a temporary's list (§16.5, last columns) */
    uint64_t tests;       /* divisibility tests in the reductions (and in the leftover check, Pack-coprime) */
    uint64_t pgcd;        /* Pack-coprime: per-piece gcds gcd(t, q^nu) (also counted as the core's gcds) */
    uint64_t fmul;        /* multiplications forming a gcd's value, a piece's power, or h (factor work) */
    uint64_t lsplits;     /* Pack-coprime: leftover splits (§16.8, inside a step) */
    uint64_t sub[M3_NSUB];/* the new sub-cases (§16.5) */
    uint64_t gchecked;    /* test build: obtained gcds checked against the 08 module's Euclid (sk_gcd_raw) */
    uint64_t zero_reads;  /* reads of the (empty) list of a zero: must stay 0 (§16.6 item 1) */
    /* Pack-coprime: the reconciliation at the start of the step (§16.7) */
    uint64_t rc_passes;   /* passes of the repeat loop */
    uint64_t rc_gcd;      /* refine's gcds, gcd(q, r) */
    uint64_t rc_splits;   /* refine's splits */
    uint64_t rc_DL, rc_DR;/* |D_L| and |D_R| as the last pass made them */
    /* Pack-coprime: splits made by the step, at reconciliation or as leftovers */
    uint64_t sp_gcd;      /* gcds inside the splits (the coprime base of {q, r}) */
    uint64_t sp_div;      /* divisions inside the splits (residuals, exponents, and a leftover's exponents) */
    uint64_t rebuilds;    /* (f) rebuilt into (f') (§16.8) */
    uint64_t rentries;    /* entries written by the rebuilds */
} m3_acount;

/* A matrix's factored figures (every build of the factored rows): the list (l)'s W, (f)'s entries E and bytes, (q)'s
 * pieces and bytes (Pack-coprime; 0 in Pack-primes). */
typedef struct m3_ffig {
    uint32_t W, E, nq;
    unsigned w;
    int factored;           /* (f) exists: the matrix has been factored */
    uint64_t bytes_f;       /* 4 (W + 1) + E (w/8 + 1) */
    uint64_t bytes_q;       /* |q| w/8 */
} m3_ffig;

/* A factoring (v0.3 §16.4), reported at its end, after (f) (and (q)) have been shrunk.  x is the matrix. */
typedef struct m3_factor {
    int id;
    const void *x;
    m3_ffig fig;
    m3_fcount fc;           /* this factoring's own counts (counting builds) */
} m3_factor;

/* The hook: NULL (the default) or the driver's function.  A read: it changes no state and no control flow.  Defined in
 * src/m3common.c. */
extern void (*m3_factor_hook)(const m3_factor *f);

#ifdef __cplusplus
}
#endif
#endif
