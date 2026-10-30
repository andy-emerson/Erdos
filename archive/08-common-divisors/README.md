# 08: Common divisors (Design 1)

Exact rationals in one class whose denominators remember the gcds found between them: each denominator has a row of the divisors known to divide it, with their residuals, and a set records the pairs already compared, so that a known common divisor shortens the next gcd (partial sharing). Design 1 of the skeleton designs, measured against a control of plain reduced rationals and against [Design 2](../09-coprime-base/) in the joint [partial-sharing baseline](../partial-sharing-baseline/).

**Status: concluded.** It lost to the control on time on every verdict job and did about the same gcd work (0.99 of the control's) where sharing is partial. See [research/findings.md](research/findings.md). Superseded by the active work on Design 3 in [`10-interning`](../10-interning/).

- [v0.1](v0.1.md): the specification of Design 1, extracted on Oct 29, 2026 from the [joint specification](../partial-sharing-baseline/v0.1.md) with no requirement changed and the same section numbers; amended Oct 29, 2026 (the pair set and the sparse incidence rows).
- [research/design-note.md](research/design-note.md): the design note for Design 1: the concept, the implementation form, the pair set and the sparse incidence rows, with costs and memory.
- [research/findings.md](research/findings.md): what was learned: the changes before the run, the measured results and the lessons.
- [lib/](lib/): Design 1's C source (`src/d1.c`, `src/rows_d1.c`, `include/sk_d1.h`) and unit tests, built against the baseline's shared core ([lib/README.md](lib/README.md)).

The rule, the harness and the measured results are shared with Design 2, in [`shared/partial-sharing-baseline`](../partial-sharing-baseline/).
