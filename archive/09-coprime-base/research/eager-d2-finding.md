# Finding: eager refinement made Design 2 cost about C gcds per new denominator

Oct 29, 2026 · found in the pilot (rule step 3); not a measurement. The fix was approved by the author on Oct 29, 2026. Spec v0.1 §8 is amended accordingly, and [rule.md](../../partial-sharing-baseline/research/rule.md) carries a dated revision.

## What

In the pilot, Design 2 as first specified computed 4–11 million gcds on each mixed job. The control computed 31–47 thousand. On every timed job Design 2 was 55–137 times slower than the control.

| Job | gcds, eager D2 | gcds, control | gcd work, D2 / ctl | time, D2 / ctl (pilot) | base at end, C |
| --- | ---: | ---: | ---: | ---: | ---: |
| 3 | 4,115,545 | 31,476 | 98× | 76× | 2,398 |
| 6 | 4,557,935 | 46,963 | 58× | 55× | 2,473 |
| 9 | 5,190,751 | 44,603 | 68× | 62× | 2,535 |
| 12 | 6,440,199 | 41,381 | 95× | 84× | 2,841 |
| 15 | 9,452,311 | 40,108 | 161× | 137× | 3,223 |
| 18 | 5,067,684 | 43,038 | 71× | 65× | 2,440 |
| 20 | 11,090,167 | 44,882 | 131× | gate only | 3,146 |

Sources:
- Time is the median of the pilot's two timed rounds: Design 2 took 123–304 ms per run, the control 1.6–2.5 ms. They were in `test/results/state.json`, are discarded by rule step 3, and were cleared on Oct 29, 2026 with the other pilot results, which predate the current code.
- Gcds, gcd work (the rule's measure) and C come from counting-build runs of the same workloads, made after the pilot. Their raw counts are 97–247 times the control's.

## Why

Design 2 refined every new denominator against the whole base as soon as it was stored, at entry and for every new result. Whenever part of a denominator was new to the base, the scan tested every piece, so a new denominator cost up to C gcds. The base grew to 2,400–3,200 pieces, and the scan averaged 740–1,550 gcds per refined row. That is about 99% of Design 2's gcds; the arithmetic of the operations themselves needed about as many as the control.

Most of that work was never used:
- The base was pairwise coprime across all denominators, though an operation only ever needs what its two operands share.
- Most pieces were private parts of single denominators.
- Most denominators never met a denominator that shares those parts.

## Fix

Design 2 now refines **on demand**, and only locally ([spec §8](../v0.1.md#8-design-2-the-coprime-base), amended; [the note](design-note.md#design-2-the-coprime-base)):
- **Rows.** Each row holds the known (piece, valuation) entries and an unrefined remainder u.
- **Entry.** Entry computes no gcd against the base.
- **Meetings.** When two denominators meet in addition, subtraction or division, each remainder is refined against the other row's pieces and against the other remainder. A piece that a meeting pulls into one row from its remainder is then tested against the other remainder too, repeating until nothing new is added. This extra refinement is needed: without it, $\tfrac{3}{4} + \tfrac{1}{8}$ is stored as $14/16$, not in lowest terms. The unit test `test_d2_extra` keeps that counterexample.
- **Multiplication** needs no meeting.
- **Base.** The base stays pairwise coprime.

Principle: **no work in advance, no work in the background; each operation computes only what it needs.**

On the same jobs, the on-demand prototype computed 31–164 thousand gcds, 1.0–3.5 times the control's. Its base ended with 3–64 pieces, and its times were 2.0–9.3 ms per run. It matched the exact reference on every workload, and it now replaces `lib/src/d2.c` under the same row name.
