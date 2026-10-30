# Partial-sharing baseline

The joint measurement of [Design 1](../08-common-divisors/) (common divisors) and [Design 2](../09-coprime-base/) (the coprime base) against one control of plain reduced rationals and, where every value shares one denominator, a shared-denominator ceiling, in one run. It belongs to neither design alone, so it lives here: the shared integer module, the control, the ceiling, the harness, the fixed rule and the measured results. Formerly `08-skeletons`; laid out this way on Oct 29, 2026, when the designs became separate specifications.

**Status: concluded.** Run on Oct 29, 2026 (commit `f39d34e`): every gate passed and no bug appeared. Both designs lost to the control on time on every verdict job. What was learned is in [research/findings.md](research/findings.md), and per design in [08's](../08-common-divisors/research/findings.md) and [09's](../09-coprime-base/research/findings.md) findings.

- [v0.1](v0.1.md): the **joint specification**, as measured: the control (§6), both designs (§7, §8), the ceiling (§9), their boundary, instrumentation and correctness checks. Each design's part is also its own specification ([08](../08-common-divisors/v0.1.md), [09](../09-coprime-base/v0.1.md)), with the same section numbers.
- [research/rule.md](research/rule.md): the pre-registered measurement rule, workloads and run plan, approved by the author on Oct 28, 2026, with a dated revision of Oct 29, 2026, and fixed before any measurement. Its hash was re-pinned on Oct 29, 2026, after the run, for one link path (test/common.py).
- [research/design-note.md](research/design-note.md): the **joint design note** of both designs, with the side-by-side comparison (formerly `paper/note-skeletons.md`).
- [research/findings.md](research/findings.md): what the joint run showed, per measure, stratum and design, and head to head.
- [lib/](lib/): the C library. The shared core is here (integer module, lookup, control, ceiling); Design 1 and Design 2 are built from their folders ([lib/README.md](lib/README.md)). Decisions: [lib/DECISIONS.md](lib/DECISIONS.md); change log: [lib/CHANGELOG.md](lib/CHANGELOG.md).
- [test/](test/): the harness (`gen.py`, `runner.py`, `analyze.py`, `bench.c`; decisions in [test/DECISIONS.md](test/DECISIONS.md)) and the [measured results](test/results/results.md), with their CSVs. The runner's working state (`results/state.json`, `results/run.log`) is not committed.

**Deferred, not run:** the index-valued pair table, a variant of Design 1 in the note.
