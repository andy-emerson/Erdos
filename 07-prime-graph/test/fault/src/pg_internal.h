/* pg_internal.h: the representation (spec "The representation") and the internal interface between the library's
 * translation units.  Nothing here is part of the public API.
 *
 * Holders.  A holder is anything with edges: a value (kind HK_VALUE, the object behind a pg_value handle), an array's
 * shared denominator D (HK_DEN), an array exception (HK_EXC) or a transient denominator held during entry (HK_TMP).
 * It keeps its numerator, its denominator as one integer (the product of its edges; cached for overflow checks and
 * the dense test d | D) and its edges in the edge pool: packed u32 (node id << 8 | weight), ascending by node id,
 * each with its position in the node's holder list (PPL) so unlinking is O(1) per edge.  Holders live in fixed-size
 * chunks, so a pg_value pointer stays valid until release; node lists name holders by id.
 *
 * Nodes.  The node table is split in two: the hot part (value and exact-division constants, read by every reduction)
 * and the cold part (atoms, holder list, reference count).  A node is stored as the product of its atoms' powers with
 * exponents of gcd 1; every atom is a proved prime in M1 (entry factors every leftover, see entry.c), so a node
 * with one atom is that prime, and a node with several atoms (multi) is the only kind a reduction can split.
 *
 * Restarts.  A split rewrites every linked holder of the split node (copy and loop), but not edge lists held in
 * local variables.  Every kernel that splits a node returns R_RESTART, and its caller recomputes from the holders.
 * Long-lived local edge lists (the accumulators of sum and dot) are registered as pins and patched by every split.
 */
#ifndef PG_INTERNAL_H
#define PG_INTERNAL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/primegraph.h"

/* ---- fault-injection copy (test/fault): the consistency variants of spec v0.3 section 10.
 * PGF_POLICY 0: N  (M1, no production checks)
 *            1: O1 (single source of truth; verify every K ops and before every refinement; rebuild the index)
 *            2: O2 (shared edge records; same verification; repair of membership by rebuild)
 *            3: O3 (M1 + the production audit every K ops; halt on mismatch, no repair)
 * PGF_O2 is set for policy 2: holders and node lists reference shared edge records (holder, node, exponent). */
#ifndef PGF_POLICY
#define PGF_POLICY 0
#endif
#if PGF_POLICY == 2
#define PGF_O2 1
#else
#define PGF_O2 0
#endif

typedef unsigned __int128 u128;
typedef __int128 i128;

#define MAXE 16          /* edges of one holder: a denominator below 2^64 has at most 15 distinct primes */
#define AW (2 * MAXE)    /* room for a merged edge list before its denominator is checked */
#define EN(x) ((x) >> 8) /* packed edge: node id (24 bits), weight (8 bits) */
#define EW(x) ((x) & 0xFFu)
#define EP(n, w) (((uint32_t)(n) << 8) | (uint32_t)(w))
#define NONE 0xFFFFFFFFu
#define MAX_NODES (1u << 24)
#define NUM_LIM (1ull << 63)   /* |numerator| <= 2^63 (2^63 only when negative) */
#define DEN_LIM (1ull << 63)   /* stored denominators are below 2^63 */
#define HCHUNK_BITS 10
#define HCHUNK (1u << HCHUNK_BITS)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define HOT static inline __attribute__((always_inline))  /* the kernels of the fast path, inlined into each loop */

/* kernel results */
enum { R_OVF = 0, R_OK = 1, R_RESTART = -1 };

/* holder kinds */
enum { HK_FREE = 0, HK_VALUE, HK_DEN, HK_EXC, HK_TMP };

struct pg_value {      /* a holder */
    int64_t num;
    uint64_t den;      /* the product of the edges (1 with none); 0 never */
    uint32_t eo;       /* offset of the edges in the pool (NONE: no block); next free holder while free */
    uint32_t id;       /* own id: chunk << HCHUNK_BITS | slot */
    uint8_t ne, cls, kind, pad;  /* edges; size class of the block (0xFF none); HK_* */
};

typedef struct {       /* node, hot part: what a reduction reads */
    uint64_t v, inv, lim;  /* the node's value; the exact-division test of its odd part */
    uint8_t s, multi, alive, pad;  /* 2-adic exponent; several atoms; live */
    uint32_t pad2;
} node_hot;

typedef struct {       /* an atom: a proved prime with its exponent in the node */
    uint64_t p, inv, lim;  /* inv, lim: exact-division test (odd p) */
    uint32_t e;
} atom_t;

typedef struct {       /* node, cold part */
    atom_t *at;
    uint32_t *list;    /* holder ids */
    uint32_t len, lcap, rc;
    uint8_t na;
} node_cold;

typedef struct { int64_t num; uint64_t den; int ne; const uint32_t *e; } view;  /* an operand (read only) */
typedef struct { int64_t num; uint64_t den; int ne; uint32_t e[AW]; } raw;      /* a result (den < 2^64) */

typedef struct { uint32_t *e; int *ne; } pin_t;

/* O2: one shared record per edge.  A holder's block in the edge pool holds record ids (ascending by node); a node's
 * list holds record ids; the exponent lives only in the record.  pos is the record's position in its node's list. A
 * free record has w == 0 and h = the next free record. */
typedef struct { uint32_t h, n, pos, w; } er_t;
#define GRING 64  /* O2: gathered (node, weight) views of holders, a ring of short-lived buffers */

typedef struct {       /* the factor cache: one denominator's primes (at most 7) */
    uint64_t d;
    uint8_t na;
    uint8_t e[7];
    uint64_t p[7];
} dcache_t;
#define DCACHE_SETS 4096  /* 2-way sets: 8,192 slots, 576 KiB, allocated with the first odd part above 2^16 */

struct pg_array {
    size_t n;
    int64_t *num;      /* dense numerators over D; 0 in exception slots */
    uint64_t D;
    pg_value *dh;      /* D's holder (NULL when D = 1) */
    size_t nx, xcap;   /* exceptions: ascending indices and their holders */
    uint32_t *xi;
    pg_value **xh;
    uint8_t deferred;  /* unreduced: made inside an open expression */
    size_t gpos;       /* position in the graph's array list */
    size_t ppos;       /* position in the open expression's list (deferred only) */
};

struct pg_graph {
    /* nodes */
    node_hot *nh;
    node_cold *nc;
    uint32_t nnodes, ncap, nlive;
    uint32_t *nfree;
    uint32_t nfree_n, nfree_cap;
    uint32_t nmulti;
    uint8_t *wsa, *wsb;            /* scratch weights per node id, kept zero */
    /* the edge pool */
    uint32_t *EPL, *PPL;
    size_t pool_n, pool_cap, pool_used;
    uint32_t pfree[5];
    /* holders */
    pg_value **hch;
    uint32_t nhch, hn, hfree, hlive;
    size_t hkind[5];
    /* the dictionary: atoms below 2^16 direct, above by open addressing (key 0 empty) */
    uint32_t *dsm;
    uint64_t *dbk;
    uint32_t *dbv;
    size_t dbcap, dbcnt;
    uint64_t hot[16], hinv[16], hlim[16];  /* the 16 most recently seen large primes, most recent first (a shortcut
                                             * only), with their exact-division constants */
    int nhot, hotpos;
    dcache_t *dc;                  /* factor cache (allocated on first use) */
    /* pins: local edge lists patched by every split */
    pin_t pins[8];
    int npins;
    /* arrays and expressions */
    pg_array **arrs;
    size_t narr, arrcap;
    int expr_depth;
    pg_array **pend;
    size_t npend, pendcap;
    /* statistics */
    size_t st_nodes_new, st_nodes_freed, st_split_entry, st_split_reduce, st_split_holders, st_restarts,
        st_v4_retries, st_rho, st_dc_hit, st_dc_miss;
    size_t mem_cur, mem_peak;
    /* O2 */
    er_t *ER;
    uint32_t ern, ercap, erfree;
    size_t erlive;
    unsigned gpos;
    uint32_t gring[GRING][MAXE];
};

/* ------------------------------------------------------------------------------------------------- mem.c */

__attribute__((noreturn)) void pgi_die(const char *msg);
void *pgi_alloc(pg_graph *g, size_t n);
void *pgi_calloc(pg_graph *g, size_t n);
void *pgi_realloc(pg_graph *g, void *p, size_t n);
void pgi_free(pg_graph *g, void *p);

/* ------------------------------------------------------------------------------------------------- factor.c */

void pgi_tables_init(void);
uint64_t pgi_gcd(uint64_t a, uint64_t b);
u128 pgi_gcd128(u128 a, u128 b);
uint64_t pgi_inv64(uint64_t m);
int pgi_is_prime(uint64_t n);
/* d's odd small primes: trial division (step 1, branch-free blocks of 8 with the stop once per block).  Appends
 * (p, e) to P/E, returns the leftover; *stopped is 1 if the stop fired (the leftover is 1 or prime). */
uint64_t pgi_trial(uint64_t n, uint64_t *P, uint32_t *E, int *k, int *stopped);
/* n (odd, no factor below 2^16, composite): its prime factors with exponents, via Pollard-Brent rho. */
int pgi_factor_rest(uint64_t n, uint64_t *P, uint32_t *E);

/* ------------------------------------------------------------------------------------------------- graph.c */

static inline pg_value *HP(const pg_graph *g, uint32_t id) { return g->hch[id >> HCHUNK_BITS] + (id & (HCHUNK - 1)); }
/* a holder's edges as packed (node << 8 | weight): M1 reads them in place; O2 gathers them from the shared records
 * into a ring buffer (valid until GRING further gathers; every use is short-lived, and stale after a split exactly
 * as M1's views are) */
static inline const uint32_t *HE(const pg_graph *g, const pg_value *h) {
    if (!h->ne) return NULL;
#if PGF_O2
    pg_graph *gg = (pg_graph *)g;
    uint32_t *b = gg->gring[gg->gpos++ % GRING];
    const uint32_t *r = g->EPL + h->eo;
    for (int k = 0; k < h->ne; k++) b[k] = EP(g->ER[r[k]].n, g->ER[r[k]].w);
    return b;
#else
    return g->EPL + h->eo;
#endif
}
static inline view hview(const pg_graph *g, const pg_value *h) {
    return (view){h->num, h->den, h->ne, HE(g, h)};
}

pg_value *pgi_holder_new(pg_graph *g, int64_t num, uint64_t den, const uint32_t *e, int ne, int kind);  /* linked */
void pgi_holder_release(pg_graph *g, pg_value *h);  /* unlink and free */
pg_value *pgi_holder_from_raw(pg_graph *g, const raw *r, int kind);
void pgi_holder_reduce(pg_graph *g, pg_value *h, const raw *r);  /* h := r, a reduction of h (edges only shrink) */
uint32_t pgi_dict_get(const pg_graph *g, uint64_t a);
/* Entry: enter d (1 <= d < 2^64) and return a linked transient holder (kind HK_TMP, num 0) whose edges are d's. */
pg_value *pgi_den_enter(pg_graph *g, uint64_t d);
/* Reduction hook: node id (several atoms) shares part of m (not all of N^wr).  Splits id by the ratio classes of the
 * cancellation and returns 1 (the caller restarts); 0 if no atom of id divides m. */
int pgi_partial(pg_graph *g, uint32_t id, u128 m, uint32_t wr);
/* pgi_partial for every node with several atoms among the edges e (weight: the power left), against m */
/* split node id by the ratio classes of the amounts (ap[j], amt[j]) cancelled from its atoms (a reduction's split) */
void pgi_split_amounts(pg_graph *g, uint32_t id, const uint64_t *ap, const uint32_t *amt, int na);
int pgi_partial_edges(pg_graph *g, const uint32_t *e, int ne, u128 m);
/* a product's form: nodes n[0..k) with the powers left of a (wa, against ub) and of b (wb, against ua) */
int pgi_partial_mul(pg_graph *g, const uint32_t *n, int k, const uint8_t *wa, const uint8_t *wb, uint64_t ua,
                    uint64_t ub);
void pgi_pin(pg_graph *g, uint32_t *e, int *ne);
void pgi_unpin(pg_graph *g, uint32_t *e);

/* ------------------------------------------------------------------------------------------------- array.c, value.c */

void pgi_array_free_all(pg_graph *g);
void pgi_debug_audit(pg_graph *g);  /* with PG_AUDIT_EVERY_OP: abort if pg_audit fails */

/* ------------------------------------------------------------------------------------------------- fault.c */

/* the production checks: every K operations (an operation is a public call that ends with DEBUG_AUDIT) */
extern uint64_t pgf_K, pgf_ops;
void pgf_periodic(pg_graph *g);
void pgf_before_split(pg_graph *g);          /* O1, O2: verification before a refinement loops over a list */
int pgf_split_interrupt(uint32_t *len);       /* F8: 1 if this split is to stop halfway (*len set to the stop) */
void pgf_on_die(const char *msg);             /* record an internal diagnostic before abort */
/* the audit, with options: light skips the primality and pairwise-gcd parts of I1 (production); midop skips what is
 * legitimately incomplete in the middle of an operation (transient holders, pins, arrays under construction) */
const char *pgi_audit_opt(pg_graph *g, int light, int midop);

#if PGF_POLICY != 0
#define DEBUG_AUDIT(g) do { if (pgf_K && ++pgf_ops >= pgf_K) { pgf_ops = 0; pgf_periodic(g); } } while (0)
#else
#define DEBUG_AUDIT(g) ((void)0)
#endif

/* ------------------------------------------------------------------------------------------------- small helpers */

static inline uint64_t mag64(int64_t a) { return a < 0 ? 0 - (uint64_t)a : (uint64_t)a; }

/* exact-division test of u by node N: 1 and *q = u / N->v if N->v divides u (multiply, compare) */
static inline int ndiv(uint64_t u, const node_hot *N, uint64_t *q) {
    if (N->s) {
        if (u & ((1ull << N->s) - 1)) return 0;
        u >>= N->s;
    }
    uint64_t t = u * N->inv;
    if (t > N->lim) return 0;
    *q = t;
    return 1;
}
static inline uint64_t ediv(uint64_t d, const node_hot *N) { return (d >> N->s) * N->inv; }  /* d / v, v | d */

static inline int mulpow(uint64_t *c, uint64_t v, int k) {
    while (k--)
        if (__builtin_mul_overflow(*c, v, c)) return 0;
    return 1;
}

#endif
