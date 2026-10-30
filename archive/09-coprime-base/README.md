# 09: Coprime base (Design 2)

Exact rationals in one class whose denominators are written over a shared set of pairwise coprime pieces: each row holds a known part, as (piece, valuation) entries, and a remainder not yet refined, and two denominators are refined against each other only when they meet, so that what they share is read from their rows (partial sharing). Design 2 of the skeleton designs, measured against a control of plain reduced rationals and against [Design 1](../08-common-divisors/) in the joint [partial-sharing baseline](../partial-sharing-baseline/).

**Status: concluded.** Its first, eager form cost 58–161 times the control's gcd work and was replaced by on-demand refinement before the measured run. On demand, it still lost to the control on time on every verdict job and did 1.6–2.1 times the control's gcd work where sharing is partial. See [research/findings.md](research/findings.md). Superseded by the active work on Design 3 in [`10-interning`](../10-interning/).

*Numbering:* Design 3, interning, was built as `09-interning` and renumbered [`10-interning`](../10-interning/) on Oct 29, 2026, after its Q32 run, so that the folders stay in design order.

- [v0.1](v0.1.md): the specification of Design 2, on demand, extracted on Oct 29, 2026 from the [joint specification](../partial-sharing-baseline/v0.1.md) with no requirement changed and the same section numbers; amended Oct 29, 2026 (refinement on demand).
- [research/design-note.md](research/design-note.md): the design note for Design 2: the known part and the remainder, meetings, the extra refinement, results that reuse known valuations, with costs and memory.
- [research/coprime-base-notation.md](research/coprime-base-notation.md): an explainer of coprime bases and the valuation notation ($\nu_q$), independent of the design (formerly `paper/note-coprime-base.md`).
- [research/eager-d2-finding.md](research/eager-d2-finding.md): the pilot finding that eager refinement cost about $C$ gcds per new denominator, and the fix.
- [research/findings.md](research/findings.md): what was learned: from eager to on-demand refinement, the measured results and the lessons.
- [lib/](lib/): Design 2's C source (`src/d2.c`, `src/rows_d2.c`, `include/sk_d2.h`) and unit tests, built against the baseline's shared core ([lib/README.md](lib/README.md)).

The rule, the harness and the measured results are shared with Design 1, in [`shared/partial-sharing-baseline`](../partial-sharing-baseline/).
