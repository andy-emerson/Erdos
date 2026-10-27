# Research behind Exact v0.1

The research for this spec was done under [04](../../04-division-algebra-family/), before the family was split into `Exact` (this spec) and the units (06). It is not copied here; this page maps each decision in [v0.1](../v0.1.md) to its source.

| Decision in v0.1 | Source |
| --- | --- |
| Slash encoding, denominator stored minus one, every pattern a value | [Q v0.3](../../03-q/v0.3.md#q72) |
| Tag and word kept apart; tags in a separate column in arrays (no speed cost) | [Phase one, S1](../../04-division-algebra-family/research/phase1-variants.md#s1-value-layouts-a-b-c) |
| Sign and magnitude as separate columns (signs added by 06) | A decision, not a measurement: `Exact` is unsigned, and GMP's kernels work on magnitudes. Contrary data: two's complement was faster at small widths in [Q v0.3](../../03-q/v0.3.md#the-vector-form) and in phase two's informational column ([results](../../04-division-algebra-family/test/results/phase2.md)) |
| Compile-time lengths up to and including the threshold, run-time lengths with GMP's kernels above | [Phase one, S2–S4](../../04-division-algebra-family/research/phase1-variants.md#s2-array-representations-by-width-g-h) |
| The threshold at 1,024 bits, inclusive | [Threshold test](../../04-division-algebra-family/research/threshold-storage-libraries.md#1-the-threshold-test); [results](../../04-division-algebra-family/test/results/threshold.md) |
| 64-bit storage with 1.5× slack | [Storage granularity](../../04-division-algebra-family/research/threshold-storage-libraries.md#2-storage-granularity); [results](../../04-division-algebra-family/test/results/storage.md) |
| Whole-vector widening (and the per-element alternative) | [Phase one, S5](../../04-division-algebra-family/research/phase1-variants.md#s5-the-widening-rule-f) |
| IFMA kernel for matrix products, `mmgc` without IFMA | [Phase one, S6](../../04-division-algebra-family/research/phase1-variants.md#s6-exact-matrix-products-k) |
| Residues only inside products, not as storage | [Phase one, S2 and trade-off H](../../04-division-algebra-family/research/phase1-variants.md#decisions-for-discussion) |
| Performance against GMP and FLINT | [Phase two](../../04-division-algebra-family/research/threshold-storage-libraries.md#3-the-format-against-gmp-and-flint); [results](../../04-division-algebra-family/test/results/phase2.md) |
| Prior art | [Family prior art](../../04-division-algebra-family/research/prior-art-family.md); [Q v0.3](../../03-q/v0.3.md#prior-art) |

The comparison with f64 and the float conversion widths in v0.1 are worked out in the spec itself; they are arithmetic on the encoding, not measurements.
