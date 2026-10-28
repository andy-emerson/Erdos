/* primegraph.h: exact rational arithmetic with shared, lazily refined denominators (spec 07, v0.1, milestone M1).
 *
 * Every value lives in one weighted bipartite graph owned by a context (pg_graph).  A value's numerator is a signed
 * 64-bit integer; its denominator is a product of shared, pairwise coprime NODES, each stored once and used by every
 * value whose denominator contains it.  Arrays keep one shared denominator D for their dense elements and hold the
 * other elements as exceptions.  See ../README.md for an overview and the choices made where the spec is open.
 *
 * M1 bounds.  A value's numerator is in [-2^63, 2^63 - 1] and its denominator in [1, 2^63 - 1]; intermediates use
 * 128 bits.  Results are exact and in lowest terms.  An operation that cannot represent its exact result returns
 * PG_OVERFLOW (never a wrapped value); see "Errors" below for the precise rule.
 *
 * Threads.  A pg_graph and everything made from it are single-threaded: use each graph from one thread at a time.
 * Different graphs may be used from different threads.
 *
 * Ownership.  Every pg_value and pg_array belongs to the graph that made it and must only be passed to functions
 * together with that graph.  A value or array stays valid until it is released (pg_value_release,
 * pg_array_release) or its graph is destroyed; pg_graph_destroy frees everything still alive.  Results are always
 * new objects: no function modifies its operands' values.
 *
 * Memory.  Allocation failure is fatal: the library prints a message to stderr and calls abort() (a choice for M1;
 * see the README).
 */
#ifndef PRIMEGRAPH_H
#define PRIMEGRAPH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------------------------------- types */

typedef struct pg_graph pg_graph;  /* the context: the graph, its dictionary and every value and array made from it */
typedef struct pg_value pg_value;  /* a handle to one exact rational (opaque) */
typedef struct pg_array pg_array;  /* an array of exact rationals in the array form (opaque) */

/* Errors.  Every function that can fail returns a pg_status; on failure its output is left unset (NULL for
 * objects) and the graph is unchanged apart from internal refinement that never changes a value.
 *
 * PG_OVERFLOW: the exact result cannot be represented under the M1 bounds.  For one operation (add, sub, mul, div,
 *   neg, entry) and for each element of an elementwise array operation, this is reported exactly when the exact,
 *   reduced result does not fit.  Two operations are composed of steps, and each step must fit: pg_array_fma checks
 *   the product a*b and then the sum; pg_array_sum and pg_array_dot accumulate with 128-bit numerators and
 *   denominators, reducing the accumulator and retrying when it overflows, so they fail only if a reduced partial sum
 *   needs more than 127 bits or the final reduced result does not fit.
 * PG_DIVZERO: division by zero, or a zero denominator at entry.
 * PG_INVALID: a malformed argument (a NULL pointer where an object is required, a malformed string, an index out of
 *   range, arrays of different lengths, an output buffer too small, pg_expr_end without pg_expr_begin). */
typedef enum pg_status {
    PG_OK = 0,
    PG_OVERFLOW = 1,
    PG_DIVZERO = 2,
    PG_INVALID = 3
} pg_status;

/* A short constant description of a status ("ok", "overflow", ...). */
const char *pg_status_str(pg_status s);

/* ------------------------------------------------------------------------------------------------- context */

/* A new, empty graph.  Never returns NULL (allocation failure aborts). */
pg_graph *pg_graph_create(void);

/* Frees the graph and every value and array still alive in it.  NULL is ignored. */
void pg_graph_destroy(pg_graph *g);

/* ------------------------------------------------------------------------------------------------- values */

/* num/den, entered and reduced to lowest terms (the sign may be on either part).  The denominator is entered as the
 * spec describes (entry steps 1-4), then the numerator is reduced against its nodes.  PG_DIVZERO if den == 0;
 * PG_OVERFLOW if the reduced value does not fit (for example 1 / INT64_MIN). */
pg_status pg_value_new(pg_graph *g, int64_t num, int64_t den, pg_value **out);

/* The integer n (no denominator). */
pg_status pg_value_from_int(pg_graph *g, int64_t n, pg_value **out);

/* The value in lowest terms: *num carries the sign, *den >= 1.  Either pointer may be NULL. */
void pg_value_get(const pg_graph *g, const pg_value *v, int64_t *num, int64_t *den);

/* Releases v: its holder leaves its nodes' lists, and nodes no longer used are freed.  NULL is ignored. */
void pg_value_release(pg_graph *g, pg_value *v);

/* A new value equal to v (a new holder sharing v's nodes). */
pg_status pg_value_clone(pg_graph *g, const pg_value *v, pg_value **out);

/* ------------------------------------------------------------------------------------------------- arithmetic */

pg_status pg_add(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out);  /* a + b */
pg_status pg_sub(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out);  /* a - b */
pg_status pg_mul(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out);  /* a * b */
pg_status pg_div(pg_graph *g, const pg_value *a, const pg_value *b, pg_value **out);  /* a / b; PG_DIVZERO if b = 0 */
pg_status pg_neg(pg_graph *g, const pg_value *a, pg_value **out);  /* -a; PG_OVERFLOW for a numerator of -2^63 */

/* -1, 0 or 1 as a < b, a = b, a > b.  Never overflows (128-bit cross-scaling). */
int pg_cmp(const pg_graph *g, const pg_value *a, const pg_value *b);
int pg_sign(const pg_value *v);     /* -1, 0 or 1 */
int pg_is_zero(const pg_value *v);  /* 1 if v = 0 */

/* ------------------------------------------------------------------------------------------------- text */

/* The longest string pg_value_to_string writes, with its terminating NUL ("-9223372036854775808/9223372036854775807"). */
#define PG_STRING_MAX 41

/* Writes "num/den", or "num" when den = 1, NUL-terminated.  PG_INVALID if cap is too small. */
pg_status pg_value_to_string(const pg_graph *g, const pg_value *v, char *buf, size_t cap);

/* Parses "[+-]digits" or "[+-]digits/[+-]digits" (no spaces, decimal digits only; at most 38 digits per part) and
 * enters it as pg_value_new does.  PG_INVALID if malformed, PG_DIVZERO for a zero denominator, PG_OVERFLOW if the
 * reduced value does not fit. */
pg_status pg_value_from_string(pg_graph *g, const char *s, pg_value **out);

/* ------------------------------------------------------------------------------------------------- arrays */

/* An array is stored in the array form (V3): one shared denominator D with dense integer numerators scaled to it,
 * and the elements whose denominator does not divide D (or whose scaled numerator overflows) as exceptions.  D is the
 * lcm of the elements' denominators if it is below 2^63, otherwise the most frequent denominator (the provisional M1
 * default).  Every array is in lowest terms per array (I4) whenever it can be observed outside an expression. */

/* From n fractions num[i]/den[i] (signs on either part; each element reduced exactly as pg_value_new would). */
pg_status pg_array_from_fracs(pg_graph *g, const int64_t *num, const int64_t *den, size_t n, pg_array **out);

/* From n values (copied; the values stay owned by the caller). */
pg_status pg_array_from_values(pg_graph *g, pg_value *const *vals, size_t n, pg_array **out);

size_t pg_array_len(const pg_array *a);

/* Element i as a new value. */
pg_status pg_array_get(pg_graph *g, const pg_array *a, size_t i, pg_value **out);

/* Element i in lowest terms.  Either pointer may be NULL. */
pg_status pg_array_get_frac(pg_graph *g, const pg_array *a, size_t i, int64_t *num, int64_t *den);

/* Every element in lowest terms, into num[0..n) and den[0..n) (one pass; the fastest way to read an array). */
pg_status pg_array_to_fracs(pg_graph *g, const pg_array *a, int64_t *num, int64_t *den);

/* Releases the array: its D and its exceptions leave their nodes.  NULL is ignored. */
void pg_array_release(pg_graph *g, pg_array *a);

/* Elementwise operations on arrays of equal length; the result is a new array.  The result's D is lcm(Da, Db) for
 * add and sub, Da*Db for mul, Da*Db then lcm with Dc for fma, and the default rule applied to the results for div
 * (and for any operation whose D would reach 2^63).  pg_array_div fails with PG_DIVZERO if any element of b is 0
 * (the whole operation fails; no result is made). */
pg_status pg_array_add(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out);
pg_status pg_array_sub(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out);
pg_status pg_array_mul(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out);
pg_status pg_array_div(pg_graph *g, const pg_array *a, const pg_array *b, pg_array **out);
pg_status pg_array_fma(pg_graph *g, const pg_array *a, const pg_array *b, const pg_array *c, pg_array **out);  /* a*b + c */

/* Reductions to a single value: sum of a, and sum of a[i]*b[i]. */
pg_status pg_array_sum(pg_graph *g, const pg_array *a, pg_value **out);
pg_status pg_array_dot(pg_graph *g, const pg_array *a, const pg_array *b, pg_value **out);

/* The array's layout, for tests and tuning: its length, its D, how many elements are exceptions, and whether it is
 * still unreduced inside an open expression. */
typedef struct pg_array_info {
    size_t n;
    int64_t D;
    size_t exceptions;
    int deferred;
} pg_array_info;
void pg_array_info_get(const pg_array *a, pg_array_info *info);

/* ------------------------------------------------------------------------------------------------- expressions (V4) */

/* Between pg_expr_begin and pg_expr_end, array operations defer reduction: their results (dense numerators and
 * exceptions alike) are kept unreduced, an overflowing step reduces its operands and retries, and only an overflow
 * that survives the retry is reported.  pg_expr_end reduces every array made inside the expression that is still
 * alive, once (I4).  Expressions nest; only the outermost pg_expr_end reduces.  Single values (pg_add ...),
 * element reads (pg_array_get, pg_array_to_fracs) and reductions (pg_array_sum, pg_array_dot) always give lowest
 * terms, inside an expression or not.  Outside an expression each array operation is its own expression.
 *
 *     pg_expr_begin(g);
 *     pg_array_mul(g, a, b, &t);      // unreduced
 *     pg_array_add(g, t, c, &r);      // unreduced
 *     pg_array_release(g, t);         // released before the end: never reduced
 *     pg_expr_end(g);                 // r is reduced here
 *
 * pg_expr_end returns PG_INVALID if no expression is open. */
pg_status pg_expr_begin(pg_graph *g);
pg_status pg_expr_end(pg_graph *g);

/* ------------------------------------------------------------------------------------------------- audit and statistics */

/* Checks every invariant of the spec over the whole graph: I1 (atoms are proved primes, each in exactly one node,
 * so nodes are pairwise coprime; also checked by pairwise gcds when there are at most 2,000 nodes), I2 (edge
 * symmetry, list positions), I3 (reference counts equal list lengths; no unused node), I4 (every value and every
 * exception of a non-deferred array in lowest terms, every non-deferred array in lowest terms per array), and the
 * structure behind them (each holder's denominator equals its edges' product, edges ascending with positive weights,
 * the dictionary, array layouts, accounting).  Returns NULL if all hold, else a short constant description of the
 * first failure.  Cost: linear in the graph (quadratic in the node count up to 2,000 nodes); for tests. */
const char *pg_audit(pg_graph *g);

typedef struct pg_stats {
    size_t nodes_live;         /* live denominator nodes */
    size_t nodes_multi;        /* live nodes with several atoms */
    size_t node_slots;         /* node table slots (high-water mark) */
    size_t nodes_created, nodes_freed;
    size_t holders_live;       /* values, array denominators and exceptions */
    size_t values_live, arrays_live;
    size_t edges_live;         /* edges (each counted once) */
    size_t dict_entries;       /* atoms above 2^16 in the dictionary's hash */
    size_t splits_entry;       /* node splits caused by entering a denominator */
    size_t splits_reduce;      /* node splits caused by a reduction (the g < N case) */
    size_t split_holders;      /* holders updated by all splits (the copy-and-loop work) */
    size_t restarts;           /* operations recomputed after a split */
    size_t v4_retries;         /* deferred steps that overflowed and were reduced and retried */
    size_t rho_calls;          /* Pollard rho invocations at entry */
    size_t den_cache_hits, den_cache_misses;  /* the per-denominator factor cache at entry */
    size_t mem_bytes, mem_peak; /* bytes allocated by the graph now, and at most */
} pg_stats;
void pg_stats_get(const pg_graph *g, pg_stats *s);

/* The values of the live nodes, ascending, into vals[0..cap); returns the number of live nodes.  For tests. */
size_t pg_debug_nodes(const pg_graph *g, uint64_t *vals, size_t cap);

#ifdef __cplusplus
}
#endif
#endif
