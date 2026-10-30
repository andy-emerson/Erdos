> **Status** (Oct 30, 2026, 18:40 CDT)
>
> - **Paper:** v0.6 done (chapters 1–4). Waiting for your direction.
> - **Test suite:** stopped: the new workload (uniform denominators) overflows every product in mul, sop and pos. Waiting for your decision.
>
> **Results:** latest completed round: [m3](11-matrix/test/results-m3-summary.md)

# Erdos

Exact rational arithmetic, made fast: specifications, tests and a companion paper.

- [`11-matrix`](11-matrix/): the current specifications (exact rational matrices: Control, Packed, Packed-235, Packed-coprime), with their research notes (`research/`) and their harness, tests and results (`test/`).
- [`paper`](paper/): the current version of the paper, [v0.6](paper/v0.6.md), and the scripts that check its pseudocode (`verify/`).
- [`RULES.md`](RULES.md): the rules for algorithm design, optimization, reviews and prose.
- [`tools`](tools/): generic harness tools (a link checker, a test runner, a literate-source tangler).
- [`archive`](archive/): earlier specifications (01 to 10), the partial-sharing baseline, and earlier versions of the paper.
