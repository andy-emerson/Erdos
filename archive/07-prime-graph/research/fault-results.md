# Fault injection: deciding the consistency option

Oct 28, 2026 · Andrew Emerson

This test decides spec v0.3 §10, which addresses the risk that the two-sided edges (holder factorizations and node lists) silently diverge. The rule was fixed before measurement ([fault-rule.md](fault-rule.md), revised after the pre-run review):
1. **A gate:** a variant whose repairs turn faults into silent wrong answers is ranked last.
2. **The primary measure:** the fewest *undetected* index faults (silent wrong, latent inconsistency, or crash with no diagnostic).
3. **Tie-breaks:** detection first, then overhead.

The first run was invalid: its control runs exposed a false positive in the check, which was fixed. The valid run made 186,000 trials over 100 workloads, and every control passed. The full tables are in [`test/fault/results/fault.md`](../test/fault/results/fault.md).

The variants:

| Variant | Mechanism |
| --- | --- |
| N | M1 with no production checks |
| O1 | single source of truth plus a derived index, with verify and **rebuild** every K operations |
| O2 | shared edge records, with verify and repair |
| O3 | periodic audit, **detect and halt**, no repair |
| J | O1 plus a journaled refinement (F8 only) |

## Results

Undetected faults (out of about 10,000 trials per group):

| Variant | Index faults | Truth corruptions | Repairs that caused a silent wrong answer | Bit flips |
| --- | --- | --- | --- | --- |
| N | 5,233 | 1,175 | 0 | 744 |
| O1, K = 10 | 55 | 598 | **945** | 287 |
| O2, K = 10 | 55 | 597 | **949** | 308 |
| **O3, K = 10** | **55** | **22** | **0** | **266** |
| O3, K = 1,000 | 356 | 178 | 0 | 385 |

**Decision by the rule: O3, detection only, at K = 10.**

## Findings

1. **Repairing from the single truth is dangerous.** O1 and O2 detect index faults as well as O3 does: 55 undetected each at K = 10, against 5,233 with no checking. But when the *truth* itself is corrupt, a check sometimes notices an inconsistency, and the rebuild then copies the corrupt truth into the index. That turned about 900 detectable faults into silent wrong answers. Halting is safer than healing, unless there is a second, independent copy to vote with, such as a per-holder checksum.
2. **Frequent detection is very effective.** Checking every 10 operations caught all but 55 of about 10,000 index faults, 22 truth corruptions, and all but 9 of 2,000 interrupted refinements. Checking every 1,000 operations lets several times more through.
3. **But a full audit is too costly as built.** On the decimal workload, checks were nearly free (≤ 1.08×). On the many-denominators workload, O3 cost **490× at K = 10**, 51× at K = 100 and 6× at K = 1,000, because each check audits the *whole* graph. That breaks the spirit of S1, under which no operation may cost more as the graph grows. The remedy is to make checking **local**: after each operation, verify only the operands' edges and the nodes they touched. That costs about as much as the operation itself, and it is how K = 10's detection can be kept without the 490×.
4. **An interrupted refinement needs atomicity, and the journal as built isn't enough.**
   - O3 detects and halts almost every interrupted refinement (9 of 2,000 undetected at K = 10). O1's rebuild silently corrupted 359.
   - J, O1 plus a journal, cut silent corruption from 359 to 103 but did not eliminate it. It confirms the review's diagnosis: the roll-forward runs too late (at the next check), and the journal does not record which holders the split affects.
   - A correct atomic refinement must record its holder list and complete or undo the split before the next public call.
5. **Bit flips:** checking cuts undetected flips from 744 to 266–430. The remainder are mostly in numerators and array metadata, which carry no redundancy to check against.

## What this means for the spec (proposed, not yet applied)

- **§10:** choose detection-only consistency (O3). Drop rebuilding as a repair, or allow it only with an independent redundant check.
- **§10 and §11:** require checks to be **local to each operation** (the operands' edges and the nodes they touched), so that frequent checking costs O(edges) rather than O(graph).
- **§7:** require refinement to be atomic: journal the split, including its holder list, and complete or roll it back before the next public operation.
- **Residual risk:** corruption of numerators and array metadata, and truth corruptions the checks cannot see. A per-holder checksum is the candidate defence.
