# Fault injection: pre-registered decision rule (revised after pre-run review)

Oct 28, 2026 · **revised after pre-run review**, before any measured trial of the fault-injection test (`test/fault/fault_bench.py`), which decides the consistency option of [spec v0.3 §10](../v0.3.md#10-consistency) (§14).  The measured run refuses to start unless this file contains the rule below verbatim, and writes the rule into `test/fault/results/fault.md` before the first trial.

Changes from the first version (same date, never measured), following the independent review's no-go: F5 is now an index-side list position for every variant (the old F5 wrote O2's shared exponent, a truth corruption, now under F4); the primary counts every undetected outcome (silent wrong, crash, benign with a latent inconsistency) over the index faults F1, F2, F5, F6, F7, with ties broken by the detected count and overhead deciding only after both; a gate penalises repairs that turn a detected fault into a silent wrong result; O3 also checks before every refinement, and every checking variant checks before the readout; K = 10, 100, 1,000; overhead per workload, interleaved rounds, a 10% band with bootstrap intervals, each variant at a K fixed by the correctness rule; F9 per memory region, not pooled; F8 in its own section with a journaled variant J that does not compete; the audit's own crashes and N's internal diagnostics are disclosed.

**Decision rule (revised after pre-run review; pre-registered in research/fault-rule.md on 2026-10-28, before the measured run).**

0. **Trials.** Variants N, O1, O2 and O3 compete; J (O1 plus a journaled refinement) runs F8 only and does not
   compete.  Every variant runs the same trials: 100 workloads of 2,000 random commands on single values and arrays,
   then a readout of every live value and array, and for each trial the same class, moment, injector seed and K
   phase.  N runs once (it has no checks); the others run at K = 10, 100 and 1,000.  Every checking variant runs the
   same check (the production audit) every K operations, before every refinement and once before the readout, so
   every trial meets at least one check; they differ only in what follows a mismatch.  There are 2,000 trials per
   class per variant and K (20 per class per workload).  Classes: the **index faults** F1 (a list entry dropped),
   F2 (a stray list entry), F5 (a wrong list position: M1's position array, O2's record position), F6 (a reference
   count) and F7 (a dictionary entry); the **truth corruptions** F3 (a wrong node in a factorization), F4 (a wrong
   exponent in a factorization; on O2, the shared record's exponent) and F8 (a refinement stopped halfway); and the
   **hardware fault** F9 (one bit flipped in the graph's live memory).
1. **Outcomes.** Detected and repaired (all results right); detected and not repaired (a production check halted,
   or the library's own diagnostic aborted); silent wrong (a wrong result returned as success, or a result where the
   oracle has an error); crash (a signal, an abort without a diagnostic, or a hang); benign (all results right),
   split by whether the graph is left inconsistent.  **Undetected** = silent wrong + crash + benign with a latent
   inconsistency.  **Repair-induced** = silent wrong after a repair.  An error status where the oracle has a result
   is a spurious error, reported separately.  A fault that found no target is excluded and counted.
2. **Gate.** R(v, K) is the repair-induced count over all classes.  (v, K) is penalised if the 95% interval of
   R(v, K) (bootstrap over workloads, 10,000 resamples, seed 20261028) lies above 0.  A variant's eligible Ks are its
   unpenalised ones; a variant with none ranks after every variant with one, and if no variant has one, every K is
   eligible.
3. **Primary.** U(v, K) is the undetected count over the index faults F1, F2, F5, F6 and F7.  A variant's primary
   value is its minimum over its eligible Ks (N: its single value).  Two counts are **tied** when the 95% interval of
   their paired difference (bootstrap over workloads, as in 2) contains 0.  The winners are the eligible variants
   whose primary value is tied with the lowest; a winner's admissible Ks are its eligible Ks tied with its own value.
4. **First tie-break.** D(v, K) is the detected count (repaired + not repaired) over the same index faults.  Among
   the winners, a variant's value is its maximum over its admissible Ks; the winners are those tied with the highest,
   and the admissible Ks shrink to those tied with the variant's value.
5. **Second tie-break: overhead**, only between variants tied on both 3 and 4, each at its largest admissible K
   (fixed before any timing is read; N has no K).  Overhead is measured on two workloads separately (a decimal array
   expression; many distinct denominators), in 7 rounds that run every configuration once in a rotated order.  X is
   compared with Y per workload by the median over rounds of T(X) / T(Y), with a 95% bootstrap interval over rounds.
   X is cheaper than Y if the interval lies below 1/1.1 on at least one workload and above 1.1 on none.  A variant
   cheaper than every other remaining one is the decision; otherwise it is a tie, reported with no verdict forced.
   Memory is reported and does not decide.
6. **Reported, not deciding:** the truth-corruption class (undetected, detected and repair-induced counts; its
   repair-induced count enters the gate); F9 per memory region, not pooled, because the byte surfaces differ between
   variants; F8 in its own section, with J against O1; spurious errors; crashes inside a check.
7. **Residual risk.** F3 and F4 corrupt the single source of truth itself, so no cross-check against derived data
   can tell which side is right: detection is incidental (a cached p^a or a list position happens to disagree), and
   a rebuild from that truth enshrines the corruption.  A per-holder checksum over the factorization and the
   numerator, written by every update and checked on read or audit, would catch them.  F8 shows that a refinement
   must be atomic (a journal, or an equivalent).
