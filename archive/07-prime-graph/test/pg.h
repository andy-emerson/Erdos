/* pg: a light prototype of the prime-graph rational (research/idea.md).
 *
 * Numerators are int64_t, denominators are below 2^63, intermediates use __int128.  Every overflow is detected
 * and counted in pg_overflows (or aborts when pg_abort_on_overflow is set); nothing ever wraps.  A value that
 * overflowed carries gid PG_BAD and poisons whatever it touches.
 *
 * Denominator side: a global table of nodes, each a u64 pairwise coprime with every other live node, flagged
 * when known to be prime.  A group is one edge set (sorted (node, weight) pairs); because the nodes are a coprime
 * base, an edge set is determined by its denominator, so groups are interned by denominator.  Arrays store
 * numerators densely plus a group id per element (or one id for a uniform array: the vector form).
 */
#ifndef PG_H
#define PG_H
#include <stddef.h>
#include <stdint.h>

#define PG_MAXE 16  /* a denominator below 2^63 has at most 15 coprime factors, so 16 edges never overflow */
#define PG_BAD 0xFFFFFFFFu

typedef struct {               /* a node on the denominator side */
    uint64_t v;                /* its value, > 1 */
    uint64_t inv, lim;         /* exact-division test for the odd part m = v >> s: u*inv <= lim iff m | u */
    uint8_t s, prime, alive;   /* 2-adic exponent of v; known prime; not yet split */
} pg_node;

typedef struct {               /* a group: one edge set, i.e. one denominator */
    uint64_t den;
    uint32_t node[PG_MAXE];    /* sorted by node id */
    uint8_t w[PG_MAXE];
    uint8_t ne, dirty;
    uint32_t dec[PG_MAXE];     /* cache: the group with weight k lowered by one (PG_BAD = not computed) */
} pg_group;

typedef struct {               /* a single value: numerator, inline edge list, cached denominator, group id */
    int64_t num;
    uint64_t den;
    uint32_t gid;
    uint8_t ne;
    uint8_t w[PG_MAXE];
    uint32_t node[PG_MAXE];
} pg_val;

typedef struct {               /* an array: dense numerators, one group id per element unless uniform */
    size_t n;
    int64_t *num;
    uint32_t *gid;
    int uniform;
    uint32_t ugid;
} pg_arr;

extern uint64_t pg_overflows;
extern int pg_abort_on_overflow;  /* abort() on overflow (tests) instead of counting */
extern int pg_keep_uniform;       /* the 'pgu' variant, documented UNREDUCED: uniform (op) uniform keeps one group
                                     (no per-element cancellation) and same-group adds skip reduction */
extern int pg_use_cache;          /* pair-plan caches on (default); off recomputes every plan (tests) */

extern pg_node *pg_nodes;
extern uint32_t pg_nnodes;
extern pg_group *pg_groups;
extern uint32_t pg_ngroups;

void pg_init(void);   /* idempotent; builds the prime table */
void pg_reset(void);  /* drops the whole graph (nodes, groups, caches, pools) */

/* entry */
uint32_t pg_den_gid(uint64_t den);       /* the group of a new denominator (one unsplit node, refined by gcd) */
pg_val pg_from(int64_t num, uint64_t den);  /* as given, unreduced: call pg_reduce (after decomposition) */

/* decomposition */
int pg_factor(uint64_t n, uint64_t *p, uint32_t *e);  /* full factorisation, primes ascending; returns count */
int pg_is_prime(uint64_t n);                          /* deterministic Miller-Rabin */
uint32_t pg_decompose_node(uint32_t id);              /* returns the number of parts it was split into (0: none) */
uint32_t pg_decompose_all(void);                      /* returns the number of nodes split */
void pg_register_pool(pg_val *v, size_t n);           /* values whose inline edges a split must rewrite */
void pg_sync_pools(void);

/* single values */
pg_val pg_add(pg_val a, pg_val b);
pg_val pg_sub(pg_val a, pg_val b);
pg_val pg_mul(pg_val a, pg_val b);
int pg_cmp(pg_val a, pg_val b);   /* -1, 0, 1; 2 if either is bad */
pg_val pg_reduce(pg_val a);       /* cancel the numerator against its nodes */
pg_val pg_val_of(int64_t num, uint32_t gid);  /* rebuild a value (inline edges) from a group */

/* arrays */
void pg_arr_alloc(pg_arr *a, size_t n);
void pg_arr_free(pg_arr *a);
void pg_arr_enter(pg_arr *a, const int64_t *num, const uint64_t *den);  /* as given, unreduced */
void pg_arr_reduce(pg_arr *a);  /* cancel every element against its nodes (complete once nodes are prime) */
void pg_arr_add(pg_arr *r, const pg_arr *a, const pg_arr *b);
void pg_arr_sub(pg_arr *r, const pg_arr *a, const pg_arr *b);
void pg_arr_mul(pg_arr *r, const pg_arr *a, const pg_arr *b);
void pg_arr_fma(pg_arr *r, const pg_arr *a, const pg_arr *b, const pg_arr *c);  /* r = a*b + c; r may alias */
pg_val pg_arr_sum(const pg_arr *a);
pg_val pg_arr_get(const pg_arr *a, size_t i);
size_t pg_table_bytes(void);  /* memory held by the node table, the group table and the denominator map */

#endif
