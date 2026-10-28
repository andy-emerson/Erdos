# Fault injection: the consistency option (spec v0.3 §10)

Oct 28, 2026 · `test/fault/fault_bench.py`

## Rule

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

## Setup

- **Variants** (one C build each from the M1 sources, copied into `test/fault/src/`): N, O1, O2, O3 and J as in the rule.  The check is M1's `pg_audit` without the primality and pairwise-gcd parts of I1, run every K operations (a public call that mutates or reduces: M1's `DEBUG_AUDIT` points), before every refinement and before the readout.
- **Faults**, one per trial, before a random command of the workload (F8: a random refinement of it): F1 drop one list entry (count and length both lowered, as a buggy unlink would); F2 append a stray holder (O2: a stray record); F3 a wrong node id in a factorization; F4 a wrong exponent (+-1 or +2; on O2 the shared record); F5 a wrong list position (another position in the list, or one past a list of one); F6 a reference count +-1 or +-2; F7 an atom's dictionary entry to another live node; F8 a refinement's copy-and-loop stopped halfway, the program continuing; F9 one bit, uniform over the bytes of the graph's live content (nodes, atoms, lists, holders, factorizations or records, dictionary entries, numerators, array metadata).
- **Oracle**: Python `fractions`; after the workload, every live value and array is read and compared. A trial stops at its first wrong output.  Benign trials are audited once more afterwards (not a production check) to find latent inconsistencies.
- **K phase**: each trial's next periodic check comes after a uniformly random 1..K operations.
- **Control**: each job first runs its workload without a fault; it must match the oracle everywhere, with no repair and no halt (so the check has no false positives).

