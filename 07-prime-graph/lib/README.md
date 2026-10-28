# primegraph: milestone M1

A C library of exact rational arithmetic at 64 bits, built to [spec v0.2](../v0.2.md) (the same design as v0.1, rewritten): denominators are stored once,
in factored form, as pairwise coprime nodes of one weighted bipartite graph, shared by every value that uses them.

## Build and test

Requirements: gcc or clang with `__int128` (64-bit Linux), Python 3 for the differential test.

| Command | What it does |
| --- | --- |
| `make` | the static library `build/libprimegraph.a` (`-O3 -march=native`) |
| `make test` | unit tests, then 40 rounds of the differential test against Python's `fractions` |
| `make check` | `test`, plus the debug build (the library audits every invariant after every operation) and the ASan + UBSan build (gcc), each running the unit tests and the differential test with an audit after every command |
| `make debug`, `make asan` | those builds alone, in `build/debug/` and `build/asan/` |
| `make fuzz` | the differential test with fresh seeds for `FUZZ_SECONDS` (default 600), on `FUZZ_BUILD` = `debug` (default), `asan` or `release`; a failing script is kept in `build/diff-fail.txt` |
| `make bench` | `build/bench/pg_bench` (clang, as round three) and a short comparison with round three's R0 and D1 binaries |

Link with `-Iinclude build/libprimegraph.a -lpthread`.

### Tests

- `tests/unit.c`: the spec's examples (30 then 45 gives 2, 3, 5; 30 then 900 splits nothing; 100 then 35 gives 2, 5,
  7), splits rewriting every holder, the reduction's g = N and g < N cases, overflow at every boundary (including
  sums whose common denominator passes 2^64 but whose result fits), rho, the dictionary's deletions, text, arrays,
  the D rule, I4 per array, expressions, and churn (nodes freed, ids reused, nothing left).
- `tests/diff.c` + `tests/diff.py`: random scripts of every operation (values, text, arrays from fractions and from
  values, all elementwise operations, sum, dot, element reads, releases, nested expressions, audits) over denominators
  built to share primes, decimals, large primes, two-large-prime products and extremes.  Every output is checked
  exactly against `fractions.Fraction`: values, elements, each error code (overflow must be reported exactly when the
  exact result does not fit), the D chosen at entry and the exception count, the deferred flag, and an empty graph
  after everything is released.  It prints coverage counters (splits, restarts, V4 retries, rho calls, errors).

## API overview

The full documentation is `include/primegraph.h`.  Opaque types: `pg_graph` (the context, single-threaded),
`pg_value` (a handle to a holder), `pg_array`.  Every fallible function returns a `pg_status`: `PG_OK`,
`PG_OVERFLOW`, `PG_DIVZERO`, `PG_INVALID`.

```c
pg_graph *g = pg_graph_create();
pg_value *a, *b, *c;
pg_value_new(g, 1, 30, &a);                 /* entry: steps 1-4, then reduction */
pg_value_from_string(g, "-7/45", &b);
if (pg_add(g, a, b, &c) == PG_OK) { int64_t n, d; pg_value_get(g, c, &n, &d); }   /* -11/90 */
pg_array *x, *y, *r;
pg_array_from_fracs(g, nums, dens, n, &x);  /* V3: shared D, dense numerators, exceptions */
pg_array_from_fracs(g, nums2, dens2, n, &y);
pg_expr_begin(g);                           /* V4 inside: unreduced, reduce-and-retry on overflow */
pg_array_fma(g, x, y, x, &r);
pg_expr_end(g);                             /* one reduction of r (I4) */
pg_array_to_fracs(g, r, out_num, out_den);
pg_graph_destroy(g);                        /* frees everything still alive */
```

- Context: `pg_graph_create`, `pg_graph_destroy`.
- Values: `pg_value_new(num, den)`, `pg_value_from_int`, `pg_value_get`, `pg_value_release`, `pg_value_clone`.
- Arithmetic: `pg_add`, `pg_sub`, `pg_mul`, `pg_div`, `pg_neg`, `pg_cmp`, `pg_sign`, `pg_is_zero`.
- Text: `pg_value_to_string` ("a/b", or "a" for integers), `pg_value_from_string` ("[+-]a" or "[+-]a/[+-]b").
- Arrays: `pg_array_from_fracs`, `pg_array_from_values`, `pg_array_get`, `pg_array_get_frac`, `pg_array_to_fracs`,
  `pg_array_len`, `pg_array_release`, `pg_array_add/sub/mul/div/fma`, `pg_array_sum`, `pg_array_dot`,
  `pg_array_info_get` (n, D, exceptions, deferred).
- Expressions (V4): `pg_expr_begin`, `pg_expr_end` (nestable; the outermost end reduces every surviving array made
  inside, once).
- Audit and statistics: `pg_audit` (I1-I4 and the structure behind them; NULL when all hold), `pg_stats_get` (nodes,
  splits, holders, edges, restarts, V4 retries, rho calls, factor-cache hits, memory), `pg_debug_nodes`.

## Where the spec lives in the code

| Spec | Code |
| --- | --- |
| Holders, nodes, atoms, edges with list positions, the dictionary (direct below 2^16, hash above) | `src/pg_internal.h`, `src/graph.c` |
| Entry steps 1-4 (ctz; D1's branch-free tests with the stop; leftover: dictionary, prime below 2^32 or after the stop, the 16 recent large primes, Miller-Rabin, rho; sharing by ratio classes; edges by lookups) | `graph.c` (`den_atoms`, `pgi_den_enter`), `factor.c` |
| Splitting by copy and loop (one pass over the holder list, weights `R^(w gR) P^(w gQ)`, the causing holder linked after) | `graph.c` (`split_out`, `split_classes`) |
| Reduction (prime atoms; a several-atom node: g = N divides, g < N splits and the operation continues on the parts) | `kernels.h` (`fin`), `graph.c` (`pgi_partial`) |
| Add, sub, mul, div, cmp, neg | `kernels.h`, `value.c` |
| Lifetimes, reference counts, freeing (S5) | `graph.c` (`unlink_edge`, `node_release`) |
| Arrays (V3), the D rule, the result-D rule, V4 in expressions, I4 per array | `array.c` |
| I1-I4 audit | `audit.c` |

S1-S5: an operation reads only its operands' holders, edges and nodes; edges are stored on both sides; splits are
copy and loop with no history; entry finds sharing by dictionary lookups and a bounded list (no scan of nodes or known
primes, and the factor cache is a fixed 8,192 slots); nodes are freed by reference count and their ids reused.

## Choices (where the spec is open or silent)

1. **Errors.** Overflow is reported exactly when the exact, reduced result does not fit (numerator in
   [-2^63, 2^63 - 1], denominator below 2^63): reducing kernels compute in 128 bits (common denominators up to 2^126)
   and reduce before checking.  `pg_array_fma` checks its two steps: the product a*b, then the sum.
   `pg_array_sum` and `pg_array_dot` first accumulate in 128 bits (dense part in one integer sum; exception products
   exact in 128 bits; reduce-and-retry on overflow).  If that overflows, an order-free fallback (I5) takes every term
   in lowest terms, sums each denominator's group exactly, and combines the nonzero groups exactly in multiprecision
   over their common denominator; overflow is then reported only if the result does not fit, or if that common
   denominator passes 16,384 bits.  So x and -x cancel in any order, and the outcome never depends on the
   dense/exception split.
2. **Handles** are checked: a released value (until its holder is reused) gives `PG_INVALID` in every value function
   (`pg_cmp` returns `PG_CMP_INVALID`); releasing twice aborts.  Object outputs are NULL after any failure.
3. **Array division by zero** fails the whole operation (`PG_DIVZERO`, no result).  An element overflow fails the whole
   operation too.
4. **Out of memory** prints a message and aborts (M1 does not unwind half-made structures).
5. **Atoms are always proved primes in M1.** Entry factors every leftover (rho at entry), so "leftover" atoms never
   exist; I1 is audited as "each atom prime, each in exactly one node" (plus pairwise gcds up to 2,000 nodes).
6. **Splitting in a reduction** uses the same ratio-class rule as entry: atom p's ratio is (the power of p cancelled)
   / (p's exponent in the node).  Each class becomes a node that cancels a whole number of times, then the operation
   restarts from its holders (splits rewrite every linked holder; local edge lists are recomputed, or patched when
   pinned, as the 128-bit accumulators are).
7. **The g < N test runs after the reduction loop**, on the final numerator: dividing by the other, coprime nodes leaves
   gcd(m, N) unchanged, and the loop then has no call in it.
8. **The D rule** is the provisional default (lcm below 2^63, else the most frequent denominator, the first seen on a
   tie).  At entry it is applied to the denominators as given, then I4 lowers D.  An operation whose result D would
   reach 2^63 (spec silent) computes every element on the graph path and applies the default rule to the results, as
   division does.
   A denominator of 2^63 (from INT64_MIN) is never chosen as D and never dense; `fin` decides whether it reduces
   to fit.
9. **I4 per array** is enforced at entry and after each operation outside an expression: D is divided by the common
   factor of D and every dense numerator (a scan that stops when the factor reaches 1, usually within a few elements).
10. **V4 operands.** Inside an expression a dense element is taken as n/D, unreduced (round three's R1), and exceptions
   are stored unreduced; `pg_expr_end` reduces them in place (only edges that vanish leave their node lists).
   Element reads, sums and dot products always return lowest terms.
11. **Strings** accept at most 38 digits per part; a part beyond 64 bits is reduced by a 128-bit gcd before entry.
12. **Limits:** arrays below 2^32 elements; at most 2^24 nodes and about 2^32 holders per graph (a fatal error past them).
13. **Additions to the API:** `pg_array_fma` (a fused a*b + c, round three's job), `pg_array_to_fracs`,
    `pg_array_info_get`, `pg_debug_nodes`.

## Departures from the spec

1. **Trial division tests 32 primes per stop check** (four of D1's branch-free blocks of 8 at a time; the stop
   p^2 > n is checked once per 32 instead of once per 8).  Testing a few primes past the stop is harmless, and it is
   1.55x faster on 33-40-bit leftovers.
2. **A factor cache at entry** (the requested parity fix): 8,192 slots in 2-way sets (most recent kept) from a
   denominator to its primes (denominators with an odd part above 2^16 and at most 7 primes; 576 KiB per graph).
   The list of the 16 most recently seen large primes is kept most-recent-first, every large prime of an entered
   denominator counting as seen, and tested with exact-division constants.  It skips steps 1-2 for a denominator seen
   before; steps 3-4 always run, so the graph is the same.  Bounded, so S1/S4 hold.
3. **Node 2 by count of trailing zeros** in reductions and products (one instruction for any power), an exact
   shortcut of the division loop.

## Parity with round three (`make bench`)

`bench/compare.py`, 3 interleaved rounds, medians in ms, the a*b + c job on round three's data; every row's output
equals R0's line for line.  `lib-v3`: each operation reduced; `lib-v4`: inside an expression.  Ratios above 1: the
library is faster.

| Workload | R0 total | D1 total | lib-v3 total (entry / op / red) | R0 / lib-v3 | D1 / lib-v3 | R0 / lib-v4 |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| W2 (decimals) | 39.6 | 43.4 | 27.3 (12.8 / 1.9 / 12.6) | **1.45** | 1.59 | 1.43 |
| W8 (money) | 36.6 | 42.8 | 28.1 (13.0 / 1.9 / 13.2) | **1.30** | 1.53 | 1.29 |
| N100 | 42.3 | 62.4 | 61.4 (25.9 / 32.6 / 2.4) | 0.69 | **1.02** | 0.43 |
| N10k | 2476 | 109.4 | 134.1 (83.5 / 46.5 / 3.1) | **18.5** | 0.82 | 12.7 |
| P64 | 32.5 | 35.4 | 67.4 (47.7 / 18.5 / 1.1) | 0.48 | 0.52 | 0.33 |

**Fix 1, decimal elementwise (D1 was ~10% slower than R0 with equal entry).**  Cause: round three's D rows sent every
reduction and operation through hooked copies of the kernels whenever any node had several primes (decimal
denominators make the node 10), and re-checked a split epoch on every operation.  Here the fast path is R0's kernel
with one flag test after the loop, splits rewrite holders in place (no epochs, no re-expansion), and the hot kernels are
forced inline.  Entry and operation now match R0 (12.8 vs 14.6 ms; 1.9 vs 2.2 ms); the final reduction is 1.8x faster
(node 2 by trailing zeros).  **Done.**

**Fix 2, entry at about 100 distinct denominators.**  Each distinct denominator is factored once per call, and once per
graph through the factor cache; trial division is 1.55x faster.  The dictionary pass of N100's entry (hashing 600,000
elements and entering about 110 distinct denominators) now takes 2.4 ms, against D1's 2.8 ms.  Round three's own counters show
its N100 gap was in the fill (D1: dictionary 2.8 ms, fill 21.6 ms), that is in the hooked reductions of fix 1, not in
trial division.  **What remains is not entry work:** N100 is 99% exceptions, and every exception here is a holder linked
into its nodes' lists (two-sided edges, S2, and reference counts, S5), about 25 ns each; R0 and D1 keep exceptions as
plain copies with no node-side lists.  That is about 11 ms of N100's entry and 5 ms of its operation.  The same cost
puts N10k at 0.82 of D1 (18x faster than R0).

**Not fixed, by design:** P64 (two-large-prime denominators from a pool of 64 primes) spends ~30 ms in Pollard rho
(about 1,230 calls per run, after the 2-way cache keeps 1,848 of the second array's 2,000 denominators): the spec
bounds the known-prime shortcut to the 16 most recently seen large primes, so most new p*q go to rho (~25 us each; an
open decision of the spec).  D1 divided every leftover by every known large prime, a scan the spec forbids (S4).
V4 is slower than V3 on N100-like data, where a dense operand over D = lcm(1..12) is taken unreduced and each exception
carries the extra edges until the expression ends (as round three's R1 was on W7 with the lcm rule).
