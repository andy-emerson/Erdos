# Paper

A concept paper explaining the idea behind the exact-rational work: the problem of adding rationals, lowest terms and the gcd, the common-denominator representation, and partial sharing over a coprime base. It is independent of any one specification.

**Status: active.** The current draft is [v0.4](v0.4.md).

- [v0.1](v0.1.md): sections 1–6 (problem and core idea).
- [v0.2](v0.2.md): sections 1–6, recast as an extension of the gcd method, with costs in three forms throughout. Related work, costs and limits, and evidence are to come.
- [v0.3](v0.3.md): sections 1–6 with the refined gcd method derived as a chain of gcds, an equal-input-size assumption, costs split into arithmetic and data-structure categories, appendices on multiplication, division and the gcd, and a bibliography of classical sources (some fields TBD).
- [v0.4](v0.4.md) (current draft): exact sharing of whole numbers; partial sharing deferred. Shared denominators, shared numerators, and a combined structure of whole numbers stored once, with arithmetic, structure and algorithm kept apart.

**Moved, Oct 29, 2026.** The two working notes that were here are not part of the paper and now live with the work they describe:
- the note on the two skeleton designs, formerly `note-skeletons.md`: the joint note is [`shared/partial-sharing-baseline/research/design-note.md`](../shared/partial-sharing-baseline/research/design-note.md), and each design's part is in [`08-common-divisors/research/design-note.md`](../08-common-divisors/research/design-note.md) and [`09-coprime-base/research/design-note.md`](../09-coprime-base/research/design-note.md);
- the explainer of coprime bases and the exponent-vector notation ($p^{\alpha}$, $\wedge$, $\vee$), formerly `note-coprime-base.md`: [`09-coprime-base/research/coprime-base-notation.md`](../09-coprime-base/research/coprime-base-notation.md).
