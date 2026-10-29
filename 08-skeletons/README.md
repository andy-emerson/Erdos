# 08: Skeletons

Prototypes of the two skeleton designs for exact rational arithmetic of [`../paper/note-skeletons.md`](../paper/note-skeletons.md), Design 1 (common divisors) and Design 2 (the coprime base), measured against a control of plain reduced rationals and, where every value shares one denominator, a shared-denominator ceiling. The aim is to measure each baseline per measure (time, memory, gcd work), with the size of every win or loss. The rule names no overall winner: choosing one is left to discussion of the per-measure results.

- [v0.1](v0.1.md): the specification of the control, both designs and the ceiling, their boundary, instrumentation and correctness checks.
- [research/rule.md](research/rule.md): the pre-registered measurement rule, workloads and run plan (draft; the per-measure thresholds, rounds, reps and reviewed workload parameters are approved by the author, Oct 28, 2026; the items added in that day's revision are still proposed).

**Deferred:** the index-valued pair table, a variant of Design 1 in the note, is not part of this round. It is a later variant test against the Design 1 baseline.

**Order of work:** note → spec → review → build → review → run → results. Nothing is built yet.
