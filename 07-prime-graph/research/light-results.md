# The prime graph, first pass: light prototype results

Oct 27, 2026 · Andrew Emerson

A light prototype of the [idea](idea.md), measured against GMP, FLINT and the vector form. Full tables are in [`test/results/pg-light.md`](../test/results/pg-light.md); the code is in [`test/`](../test/). An independent review came before the run, and its findings were fixed and re-verified. Every result was checked against Python's `fractions`.

**Limits of this pass:**
- The prototype handles 64-bit numerators and denominators only, while GMP and FLINT are arbitrary precision.
- One machine, 5 interleaved rounds.

## Results

Total time (entry + decomposition + operation) of each contender over pg's. Above 1, pg is faster.

| Workload | GMP `mpq_t` | FLINT `fmpq` | FLINT shared denominator (incl. its reduction) | Vector form (unreduced) |
| --- | --- | --- | --- | --- |
| W1 decimal column sum | 6.2 | 4.4 | 0.46 | 0.09 |
| W2 decimals, a·b + c | 3.8 | 2.9 | 0.85 | 0.07 |
| W3 mixed denominators 1–12, sum | 5.0 | 3.4 | 0.47 | 0.09 |
| W3 mixed denominators 1–12, a·b + c | 2.4 | 1.35 | 0.78 | unable |
| W4 unrelated denominators 1–1000, sum | pg unable (64 bits) | — | — | unable |
| W4 unrelated denominators, a·b + c | 0.81 | 0.56 | 2.9 | unable |
| W5 single values, add | 1.29 | 0.73 | 0.45 | 0.04 |
| W5 single values, mul | 1.44 | 0.84 | 0.59 | unable |
| W6 growth chain | 5.1 | 4.5 | 0.95 | 0.07 |
| W7 32–62-bit denominators sharing primes, a·b + c | 1.46 | 1.51 | 20 | unable |

## Findings

1. **Against ordinary rationals, pg is faster on most workloads.**
   - It beats GMP `mpq_t` everywhere except unrelated denominators.
   - It beats FLINT `fmpq` by 1.35–4.5× on columns, mixed denominators, chains and W7.
   - It loses to FLINT on single values (0.73–0.84) and on unrelated denominators (0.56).
2. **Partial sharing, the case the idea targets, wins.** On W7, larger denominators built from shared primes, pg is about 1.5× faster than GMP and FLINT and 20× faster than a shared denominator, which explodes there. Caveat: W7 was constructed with shared primes, so it shows what pg does when sharing exists, not how common sharing is.
3. **On data with one scale, pg loses to the plain shared denominator, and the cause is clear.** Per-value reduction to lowest terms fragments the data. A column of three-place decimals has one denominator, 1000, but reducing each value gives denominators 1, 2, 4, 5, 8, 10 and so on: 16 groups instead of one (49 for W2). The fast integer loops then no longer apply.
   - The unreduced variant (pgu), which keeps one group, runs at vector-form speed: 0.05–0.06 of pg's time.
   - **The canonical form should be per group, as the vector form's is,** not per value.
4. **Entry is the bottleneck.** Entering values takes 60–95% of pg's time (merging denominators, refinement, reduction), against about 1 ms for the operation on 10⁶ decimals.
5. **Unrelated denominators make groups explode.** W4's a·b + c created 274,488 groups (92 MiB). Grouping needs a policy for values that share nothing, for example leaving them as plain single values.
6. **Decomposition is cheap.** On W7 it split 24 nodes (Pollard's rho included) in 6.5 ms for 200,000 values; elsewhere gcd refinement at entry did the splitting.

## For the second pass

- **A per-group canonical form,** so data with one scale stays one group. This is the vector form's rule.
- **Cheaper entry:** avoid reducing each value, and avoid the linear scan for gcd refinement.
- **A grouping policy:** values sharing nothing stay single values, with no new group per denominator.
- **Arbitrary precision beyond 64 bits,** so unrelated-denominator sums run at all.
- **More realistic partial-sharing data** than W7's construction.

For the third prior-art pass, aimed at the weak spots:
- Hall's 1974 ALTRAN benchmarks;
- Bernstein's batch gcd and product trees, for refinement;
- fast hashing and grouping at entry.
