# 09: Interning (Design 3, layer 1)

Exact sharing of whole numbers, as in [`../paper/v0.4.md`](../paper/v0.4.md) §5. Every distinct whole number is stored once, in one list with a value-to-index lookup. Each rational is a pair of indices, with a sign tag on the numerator, always in lowest terms. Every operation compares indices first and takes a shortcut from the paper's §5.2 table when one applies; otherwise it uses the refined gcd-first method.

Design 3 is built as two rows:
- **L1**, the baseline: the paper's design exactly as written, including a lookup of every whole number of every result. Deferred reduction, an inline small-value tag, partial sharing, clusters and garbage collection are all left out. A complex or axis tag is a possible later extension.
- **L1-opt**, an optimized variant, built separately and run on the same tests. It adds production tricks, each with its own counter. The first, required by the author, skips the lookups whose index the operation already knows when its one gcd is 1. The others (O-2 to O-7) are approved by the author, Oct 29, 2026 (spec §17).

Both are measured against four rows:
- the 08 control (plain reduced rationals), which already takes the equal-denominator shortcut by comparing values;
- FLINT `fmpq`;
- GMP `mpq_t`;
- on fully shared money jobs only, a FLINT shared-denominator ceiling.

There are 11 jobs. Between them they vary reuse, from small sets of whole numbers down to additions that keep making new numerators. Results are reported per measure only (time, bytes per rational, counts, and gcds and multiplications per case), with no overall winner, and the realized reuse is given for every job.

- [v0.1](v0.1.md): the specification. It covers the structures, the index checks and the order of cases, the operations, the library rows, the boundary, instrumentation and correctness checks. §16 lists the choices the paper leaves open, and §17 defines L1-opt and its tricks.
- [research/rule.md](research/rule.md): the pre-registered measurement rule, workloads and run plan. The full run is estimated at 9–17 minutes. The pilot stops the run if it projects more than 45 minutes, and there is a hard stop at 60 minutes.

**Status:** Oct 29, 2026, revised after the first review. Every threshold and choice is approved by the author, Oct 29, 2026: the thresholds, the 60-minute hard stop and 45-minute pilot limit, the 2.0 severe-memory flag, and all seven L1-opt tricks. The rule is fixed.

**Reuses from 08** (unchanged): the shared integer module (`08-skeletons/lib/src/sk_internal.h`, `common.c`), the hash lookup and the control (`ctl.c`). The harness is adapted from 08's harness and from the 07 M3 driver.

**Order of work:** paper → spec and rule → review → build → review → run → results. Built Oct 29, 2026: [`lib/`](lib/) (L1 and L1-opt from one source, the 08 copies in `lib/vendor08/`; [`lib/DECISIONS.md`](lib/DECISIONS.md)) and [`test/`](test/) (generator, driver, runner, analysis; [`test/DECISIONS.md`](test/DECISIONS.md)). Independently reviewed Oct 29, 2026 (differential tests including FLINT and GMP, fidelity, fairness); only minor problems were found, and they are fixed: the analysis reports every item of rule item 12, the spec's wording of O-2, O-4 and O-5 is made exact, and the runner checks that the machine is idle before any timed run (a procedural revision of the rule, re-pinned by hash). The earlier pilot's results were cleared; the pilot and the full run are run later, alone on the machine.
