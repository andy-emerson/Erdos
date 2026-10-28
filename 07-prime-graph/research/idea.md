# The prime graph: exact rationals with lazily decomposed denominators

Oct 27, 2026 · Andrew Emerson

This note records an idea from discussion, before any prior-art review or measurement. It is not a spec. Scope: exact arithmetic on the real rationals only; no complex numbers or other algebras.

## Why

The vector form (one shared denominator, integer numerators) is very fast when values share a scale, such as decimals, but it loses badly when denominators are unrelated, because their common denominator explodes. [Q v0.3](../../03-q/v0.3.md#where-it-loses) measured 1.9–3.5× slower than GMP on mixed small denominators and 5–78× slower on unrelated ones. Single values computed one at a time are at best at parity with FLINT.

The idea is to keep the vector form's speed where values share a denominator, and to gain speed where they share only **part** of one.

## The representation

Rationals are stored in a weighted bipartite graph.

- **Numerator side:** one node per value, holding an integer numerator. The numerator is signed (two's complement) and carries the value's sign; denominators are always positive.
- **Denominator side:** nodes that are primes when known. Until a denominator has been fully decomposed, a node may be a leftover that hasn't been split yet; it is treated as if it were prime.
- **Edges:** a value is connected to each node in its denominator, and the edge's **weight is the power** of that node. $1/12$ connects to 2 with weight 2 and to 3 with weight 1. An integer has no edges.

A value is its numerator over the product of its nodes raised to their weights.

**The one invariant:** no two denominator nodes share a factor (they are pairwise coprime). The arithmetic is correct whether or not every node is prime; primality only makes sharing easier to find. Splitting a node later refines the graph and never changes any value.

**Groups.** Values with identical edge sets have the same denominator. They are stored together as a vector in the vector form: numerators in a dense array, the denominator shared. The graph is the index; the vectors are the storage. Data on one scale, such as a column of decimals, is a single group, so it keeps the vector form's speed.

## Entering a value

A new rational enters as a single value. Its denominator, as given, becomes one node, treated as prime, with an edge of weight 1. No work is done up front.

## Lazy decomposition

As computing resources become available, denominators are decomposed into primes, in stages. Each stage splits a node and moves its values' edges onto the parts.

1. **Powers of 2.** One instruction (count trailing zeros) gives the exponent of 2 directly.
2. **Small primes, from a table.** The primes up to $2^{16}$ (6,542 of them, about 26 KB) are tried in order. Each divisibility test is a multiply and a compare, using the same exact-division trick as the vector form. Each prime that divides is divided out as many times as it goes; the count is the edge weight.
3. **Adaptive stop.** After each prime is divided out, the search bound becomes the square root of what is **left**. When $p^2$ exceeds the leftover, the leftover is prime and decomposition is complete. Fixed upper bounds per type, below which a full search always finishes: primes up to $2^8$ for u16 and $2^{16}$ for u32. Most denominators, such as those of decimals, finish within the first few primes.
4. **64-bit leftovers.** Two checks finish these:
   - a deterministic Miller–Rabin test, which is exact for any 64-bit number with a known fixed set of bases, decides primality;
   - if the leftover is not prime, Pollard's rho splits it.

   Both take microseconds to milliseconds.
5. **Larger leftovers stay as nodes.** They are joined to other values by gcd: if one leftover is $7p$ and another is $11p$, their gcd $p$ becomes a shared node, without $p$ ever being identified as prime. This is factor refinement.

## Operations (sketch)

- **Addition.** The common denominator is each node at its highest weight, which is a lookup, not a gcd. Each numerator is scaled by its missing node powers, and the numerators are added. The more two values share, the fewer scalings are needed; values in the same group need none, and their sum is one integer add.
- **Reduction.** A sum's numerator can only cancel against its own denominator's nodes. Reduction is therefore a divisibility test per node, for example "does 2 divide it? does 5?", instead of a general gcd. A node that is still an unsplit leftover needs one gcd, which may split it, refining the graph.
- **Multiplication.** Edge weights add on shared nodes, and the numerators multiply. Before multiplying, each numerator is cancelled against the other's nodes by divisibility tests, which is Henrici's idea with the factors already known.
- **Division** multiplies by the reciprocal; the numerator's factors must then be found to form the new edges, by the same lazy process. **Comparison** cross-multiplies, using the same scaling as addition.
- **Results** join the group with the same edge set, or start a new one.

## What it should improve, if it works

- **Mixed small denominators**, where the vector form lost to GMP: denominators of 1 to 12 decompose immediately into 2, 3, 5, 7 and 11, so values share nearly everything.
- **Single values:** addition with a shared denominator, and reductions by divisibility tests, could beat FLINT's per-value gcds, not just tie with it.
- **Data on one scale** is unchanged: one group, the vector form as before.

## Open questions

- **Storage.** Each value carries a short list of (node, weight) pairs. That is small for decimals and larger for messy data. How it is stored decides much of the speed.
- **When to decompose and merge:** at idle times, after a set number of operations, or when a value is first used.
- **Indirection.** Values move between groups as their nodes are refined, so they are reached through handles. Lifetimes and holes in groups need a policy.
- **Determinism.** Background decomposition must change only where values live, never what they are.
- **Numerator growth** under repeated multiplication is inherent; no representation avoids it.
- **Wide denominators** above 64 bits rely on gcd refinement alone. How well values sharing large factors get connected is untested.

## Next steps

1. A quick prior-art scan (about half an hour) to learn whether an implementation exists that should be benchmarked against.
2. Build a prototype and a harness, with an independent review after each.
3. Measure on common cases that the earlier specs handled well and badly:
   - decimal columns;
   - mixed small denominators;
   - unrelated denominators;
   - growth chains;
   - single values.

   Contenders: the vector form from 04, GMP `mpq_t`, FLINT `fmpq`, and GMP and FLINT integers over a shared denominator.
4. The full prior-art review, after the results.
