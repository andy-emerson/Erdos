# Scalability stress test: does the design break as the graph grows?

Oct 28, 2026 · Andrew Emerson

This test checks the [scalability requirements](idea.md#scalability-requirements-agreed-oct-27) all at once, under loads built to break them. Three rows:
- **S** has every requirement:
  - edges stored on both sides;
  - splits by copy and loop;
  - the seen-prime dictionary at entry;
  - batch refinement of leftovers in an idle pass;
  - reference-counted nodes;
  - V3 arrays.
- **S-lazy** is S without Miller–Rabin and rho at entry; leftovers are refined only in the idle pass.
- **C** is round two's control entry (a gcd scan over all nodes), as a reference curve.

The full report is [`test/scale/results/scale.md`](../test/scale/results/scale.md), with plots alongside. Independent reviews came before the run. The final review deliberately broke a scratch copy of S, once at entry and once in operations, and the test caught both. Every sampled result was checked against Python's `fractions`. **Every invariant audit passed:** no two nodes share a factor, edges are symmetric, and reference counts match the lists. **No leaks.** The run took 50 minutes.

## Verdicts

| Test | S | S-lazy | C |
| --- | --- | --- | --- |
| **Growth** to 10⁷ values: operation time (fixed sample and cold), entry and idle per value, each within 2× of the start | **pass:** ops 1.03×, cold 1.02×, entry 1.16× | **pass:** ops 1.06×, entry 1.83×, idle 1.07× | **fail:** entry grew **51×** |
| **Churn** (10⁷ deletes and inserts): nodes, memory and time per event bounded | **pass:** 1.05×, 1.01×, 1.01×; no leak | **pass:** 1.05×, 1.01×, 1.19×; no leak | **fail:** stopped by its time budget before the first step |

**The design does not break.** Operation time stays flat as the graph grows to 10 million values. Entry per value stays flat. Nodes and memory stay bounded under constant creation and deletion, and nothing leaks. The control, which scans every node at entry, is the one that breaks: its entry grew 51× over the same growth.

## The other three tests (graded only on correctness)

- **Heavy split.** Splitting a node shared by 1–10 million numerators costs **16–39 ns per numerator**, linear as expected: 160 ms for the first split at 10 million, 236 ms for the second. This is your copy-and-loop. One cost to note: memory rises in a step at each split (to 3.4 GB at 10 million after two splits), because the edge store reuses freed space only for lists of the same size class. Better reuse would fix it.
- **Deep refinement chain.** One node over the first 15 primes, shared by up to 10⁶ numerators, split 14 times in a row: about 4–130 ns per numerator per split. Every audit passed after every split.
- **Many distinct denominators (10³ to 10⁵).**
  - **Entry per distinct denominator:** C 9.7 µs → 778 µs (80×); S 55 → 65 µs (flat); S-lazy 7.9 → 7.4 µs (flat).
  - **S's flat cost is high,** about 60 µs per new denominator, because rho factors each two-large-prime denominator at entry.
  - **S-lazy's entry is 8× cheaper, but its idle pass grows faster than the graph:** 6 ms, then 155 ms, then 23.5 s. It compares new leftovers against every settled large factor, which the review flagged as a scan that grows with the graph.

## What this means

- **The scalability requirements hold together and do what they promise.** That settles the design question.
- **Two costs remain, both constants or implementation choices, not growth with the graph:**
  1. **Rho at entry** for denominators with two large primes, about 60 µs each, paid once per distinct denominator.
  2. **The edge store's memory reuse** after very large splits.
- **S-lazy as built is not acceptable,** because its idle pass grows faster than the graph. If deferred factoring returns, its idle pass needs refinement in true batches (new leftovers against each other, and against a bounded structure of settled factors), not a comparison with every settled factor.
- **Operation times rise with the size of the working set** (79 → 334 ns at 10⁵ denominators) for S and C alike. That comes from cache and memory effects of the larger data, not from the graph, since the fixed-operand sample stays flat.
