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

## Results

186,000 trials in 13.0 min; 100 workloads of 2,000 commands; 20 trials per class per workload.

### Summary

| variant | K | index: undetected | index: detected | truth: undetected | truth: repair-induced | all: repair-induced | F9: undetected |
| --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | 5233 | 4128 | 2573 | 0 | 0 | 744 |
| O1 | 10 | 55 | 9885 | 966 | 933 | 945 | 287 |
| O1 | 100 | 242 | 9553 | 967 | 829 | 850 | 371 |
| O1 | 1,000 | 378 | 9361 | 993 | 778 | 810 | 404 |
| O2 | 10 | 55 | 9885 | 973 | 941 | 949 | 308 |
| O2 | 100 | 242 | 9553 | 981 | 838 | 855 | 395 |
| O2 | 1,000 | 378 | 9361 | 1008 | 787 | 814 | 430 |
| O3 | 10 | 55 | 9885 | 31 | 0 | 0 | 266 |
| O3 | 100 | 232 | 9564 | 124 | 0 | 0 | 351 |
| O3 | 1,000 | 356 | 9385 | 203 | 0 | 0 | 385 |

### Index faults (decide)

| variant | K | class | undetected | silent wrong (after a repair) | silent wrong (other) | crash (inside a check) | benign, latent | benign, clean | detected, repaired | detected, not repaired (internal diagnostic) | spurious error | not injected |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | F1 | 1958 | 0 | 109 | 1312 (0) | 537 | 0 | 0 | 25 (25) | 17 | 0 |
| N | - | F2 | 124 | 0 | 0 | 0 (0) | 124 | 0 | 0 | 1876 (1876) | 0 | 0 |
| N | - | F5 | 579 | 0 | 0 | 13 (0) | 566 | 566 | 0 | 855 (855) | 0 | 0 |
| N | - | F6 | 1982 | 0 | 53 | 455 (0) | 1474 | 0 | 0 | 9 (9) | 9 | 0 |
| N | - | F7 | 590 | 0 | 0 | 0 (0) | 590 | 47 | 0 | 1363 (1363) | 0 | 0 |
| O1 | 10 | F1 | 52 | 0 | 0 | 52 (0) | 0 | 0 | 1931 | 17 (0) | 0 | 0 |
| O1 | 10 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1769 | 231 (231) | 0 | 0 |
| O1 | 10 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 60 | 1922 | 18 (18) | 0 | 0 |
| O1 | 10 | F6 | 3 | 0 | 0 | 3 (0) | 0 | 0 | 1987 | 10 (0) | 0 | 0 |
| O1 | 10 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1770 | 230 (230) | 0 | 0 |
| O1 | 100 | F1 | 217 | 8 | 4 | 205 (0) | 0 | 0 | 1721 | 62 (2) | 0 | 0 |
| O1 | 100 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1355 | 645 (645) | 0 | 0 |
| O1 | 100 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 204 | 1716 | 80 (80) | 0 | 0 |
| O1 | 100 | F6 | 25 | 2 | 4 | 19 (0) | 0 | 0 | 1939 | 35 (1) | 1 | 0 |
| O1 | 100 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1393 | 607 (607) | 0 | 0 |
| O1 | 1,000 | F1 | 324 | 14 | 8 | 302 (0) | 0 | 0 | 1620 | 54 (2) | 2 | 0 |
| O1 | 1,000 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1213 | 787 (787) | 0 | 0 |
| O1 | 1,000 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 258 | 1611 | 131 (131) | 0 | 0 |
| O1 | 1,000 | F6 | 53 | 7 | 8 | 38 (0) | 0 | 0 | 1915 | 31 (1) | 1 | 0 |
| O1 | 1,000 | F7 | 1 | 0 | 0 | 1 (0) | 0 | 0 | 1269 | 730 (730) | 0 | 0 |
| O2 | 10 | F1 | 52 | 0 | 0 | 52 (0) | 0 | 0 | 1931 | 17 (0) | 0 | 0 |
| O2 | 10 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 2000 | 0 (0) | 0 | 0 |
| O2 | 10 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 60 | 1922 | 18 (18) | 0 | 0 |
| O2 | 10 | F6 | 3 | 0 | 0 | 3 (0) | 0 | 0 | 1987 | 10 (0) | 0 | 0 |
| O2 | 10 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1770 | 230 (230) | 0 | 0 |
| O2 | 100 | F1 | 217 | 8 | 4 | 205 (0) | 0 | 0 | 1721 | 62 (2) | 0 | 0 |
| O2 | 100 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 2000 | 0 (0) | 0 | 0 |
| O2 | 100 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 204 | 1716 | 80 (80) | 0 | 0 |
| O2 | 100 | F6 | 25 | 2 | 4 | 19 (0) | 0 | 0 | 1939 | 35 (1) | 1 | 0 |
| O2 | 100 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 1393 | 607 (607) | 0 | 0 |
| O2 | 1,000 | F1 | 324 | 14 | 8 | 302 (0) | 0 | 0 | 1620 | 54 (2) | 2 | 0 |
| O2 | 1,000 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 2000 | 0 (0) | 0 | 0 |
| O2 | 1,000 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 258 | 1612 | 130 (130) | 0 | 0 |
| O2 | 1,000 | F6 | 53 | 7 | 8 | 38 (0) | 0 | 0 | 1915 | 31 (1) | 1 | 0 |
| O2 | 1,000 | F7 | 1 | 0 | 0 | 1 (0) | 0 | 0 | 1269 | 730 (730) | 0 | 0 |
| O3 | 10 | F1 | 52 | 0 | 0 | 52 (0) | 0 | 0 | 0 | 1948 (0) | 0 | 0 |
| O3 | 10 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (231) | 0 | 0 |
| O3 | 10 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 60 | 0 | 1940 (18) | 0 | 0 |
| O3 | 10 | F6 | 3 | 0 | 0 | 3 (0) | 0 | 0 | 0 | 1997 (0) | 0 | 0 |
| O3 | 10 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (230) | 0 | 0 |
| O3 | 100 | F1 | 209 | 0 | 4 | 205 (0) | 0 | 0 | 0 | 1791 (2) | 0 | 0 |
| O3 | 100 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (645) | 0 | 0 |
| O3 | 100 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 204 | 0 | 1796 (80) | 0 | 0 |
| O3 | 100 | F6 | 23 | 0 | 4 | 19 (0) | 0 | 0 | 0 | 1977 (1) | 0 | 0 |
| O3 | 100 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (607) | 0 | 0 |
| O3 | 1,000 | F1 | 310 | 0 | 8 | 302 (0) | 0 | 0 | 0 | 1689 (2) | 1 | 0 |
| O3 | 1,000 | F2 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (787) | 0 | 0 |
| O3 | 1,000 | F5 | 0 | 0 | 0 | 0 (0) | 0 | 258 | 0 | 1742 (131) | 0 | 0 |
| O3 | 1,000 | F6 | 46 | 0 | 8 | 38 (0) | 0 | 0 | 0 | 1954 (1) | 0 | 0 |
| O3 | 1,000 | F7 | 0 | 0 | 0 | 0 (0) | 0 | 0 | 0 | 2000 (730) | 0 | 0 |

### Truth corruptions F3, F4 (scored; repair-induced enters the gate)

F3 and F4 corrupt the single source of truth itself, so no cross-check can tell which side is right. Detection is incidental (a cached p^a, a list position or the edge order happens to disagree), and a rebuild from that truth enshrines the corruption (the repair-induced column).  A per-holder checksum over the factorization and the numerator would catch them.

| variant | K | class | undetected | silent wrong (after a repair) | silent wrong (other) | crash (inside a check) | benign, latent | benign, clean | detected, repaired | detected, not repaired (internal diagnostic) | spurious error | not injected |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | F3 | 963 | 0 | 121 | 649 (0) | 193 | 0 | 0 | 971 (971) | 66 | 0 |
| N | - | F4 | 212 | 0 | 105 | 1 (0) | 106 | 1727 | 0 | 12 (12) | 49 | 0 |
| O1 | 10 | F3 | 234 | 215 | 9 | 10 (0) | 0 | 0 | 823 | 906 (56) | 37 | 0 |
| O1 | 10 | F4 | 364 | 359 | 5 | 0 (0) | 0 | 88 | 1140 | 374 (2) | 34 | 0 |
| O1 | 100 | F3 | 270 | 169 | 27 | 74 (1) | 0 | 0 | 756 | 932 (247) | 42 | 0 |
| O1 | 100 | F4 | 338 | 321 | 17 | 0 (0) | 0 | 401 | 915 | 314 (3) | 32 | 0 |
| O1 | 1,000 | F3 | 319 | 156 | 43 | 120 (2) | 0 | 0 | 712 | 928 (332) | 41 | 0 |
| O1 | 1,000 | F4 | 320 | 293 | 27 | 0 (0) | 0 | 573 | 807 | 267 (5) | 33 | 0 |
| O2 | 10 | F3 | 233 | 215 | 9 | 9 (0) | 0 | 0 | 825 | 905 (56) | 37 | 0 |
| O2 | 10 | F4 | 364 | 359 | 5 | 0 (0) | 0 | 88 | 1140 | 374 (2) | 34 | 0 |
| O2 | 100 | F3 | 273 | 168 | 27 | 78 (3) | 0 | 0 | 753 | 932 (249) | 42 | 0 |
| O2 | 100 | F4 | 338 | 321 | 17 | 0 (0) | 0 | 401 | 915 | 314 (3) | 32 | 0 |
| O2 | 1,000 | F3 | 323 | 156 | 42 | 125 (3) | 0 | 0 | 709 | 927 (334) | 41 | 0 |
| O2 | 1,000 | F4 | 320 | 293 | 27 | 0 (0) | 0 | 573 | 807 | 267 (5) | 33 | 0 |
| O3 | 10 | F3 | 17 | 0 | 9 | 8 (0) | 0 | 0 | 0 | 1978 (56) | 5 | 0 |
| O3 | 10 | F4 | 5 | 0 | 5 | 0 (0) | 0 | 88 | 0 | 1904 (2) | 3 | 0 |
| O3 | 100 | F3 | 87 | 0 | 27 | 60 (0) | 0 | 0 | 0 | 1898 (247) | 15 | 0 |
| O3 | 100 | F4 | 17 | 0 | 17 | 0 (0) | 0 | 401 | 0 | 1574 (3) | 8 | 0 |
| O3 | 1,000 | F3 | 151 | 0 | 43 | 108 (0) | 0 | 0 | 0 | 1832 (332) | 17 | 0 |
| O3 | 1,000 | F4 | 27 | 0 | 27 | 0 (0) | 0 | 573 | 0 | 1386 (5) | 14 | 0 |

### F8: an interrupted refinement (its own section; not in the index totals)

The half-update is in the truth: the split node's atoms have changed and only part of its holders were rewritten (graph.c, split_out).  A rebuild from the factorizations cannot recover it, so refinement must be atomic.  J is O1 plus a journal written before the atoms change (N, P, Q, R, gQ, gR); its check first rolls an interrupted refinement forward (a holder that already has its P edge is done).  J does not compete in the decision.

| variant | K | class | undetected | silent wrong (after a repair) | silent wrong (other) | crash (inside a check) | benign, latent | benign, clean | detected, repaired | detected, not repaired (internal diagnostic) | spurious error | not injected |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | F8 | 1398 | 0 | 139 | 0 (0) | 1259 | 0 | 0 | 584 (584) | 18 | 0 |
| O1 | 10 | F8 | 368 | 359 | 9 | 0 (0) | 0 | 0 | 1553 | 48 (48) | 31 | 0 |
| O1 | 100 | F8 | 359 | 339 | 20 | 0 (0) | 0 | 0 | 1547 | 64 (64) | 30 | 0 |
| O1 | 1,000 | F8 | 354 | 329 | 25 | 0 (0) | 0 | 0 | 1548 | 68 (68) | 30 | 0 |
| O3 | 10 | F8 | 9 | 0 | 9 | 0 (0) | 0 | 0 | 0 | 1989 (48) | 2 | 0 |
| O3 | 100 | F8 | 20 | 0 | 20 | 0 (0) | 0 | 0 | 0 | 1977 (64) | 3 | 0 |
| O3 | 1,000 | F8 | 25 | 0 | 25 | 0 (0) | 0 | 0 | 0 | 1971 (68) | 4 | 0 |
| J | 10 | F8 | 112 | 103 | 9 | 0 (0) | 0 | 0 | 1566 | 300 (48) | 22 | 0 |
| J | 100 | F8 | 128 | 105 | 20 | 3 (0) | 0 | 0 | 1510 | 338 (64) | 24 | 0 |
| J | 1,000 | F8 | 142 | 109 | 25 | 8 (0) | 0 | 0 | 1491 | 343 (68) | 24 | 0 |
| O2 | 10 | F8 | 376 | 367 | 9 | 0 (0) | 0 | 109 | 1484 | 0 (0) | 31 | 0 |
| O2 | 100 | F8 | 370 | 349 | 21 | 0 (0) | 0 | 160 | 1440 | 0 (0) | 30 | 0 |
| O2 | 1,000 | F8 | 365 | 338 | 27 | 0 (0) | 0 | 198 | 1407 | 0 (0) | 30 | 0 |

### F9 by memory region (undetected / trials; not pooled, the byte surfaces differ)

| variant | K | array_meta | atoms | cached_den | dictionary | factorization | holder_meta | list | node | numerator | positions | record |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | 146/253 | 26/120 | 39/168 | 22/24 | 82/155 | 149/309 | 17/118 | 73/170 | 112/536 | 78/147 | - |
| O1 | 10 | 116/253 | 5/120 | 0/168 | 1/24 | 11/155 | 16/309 | 1/118 | 36/170 | 96/536 | 5/147 | - |
| O1 | 100 | 124/253 | 3/120 | 7/168 | 3/24 | 31/155 | 42/309 | 3/118 | 41/170 | 100/536 | 17/147 | - |
| O1 | 1,000 | 128/253 | 3/120 | 11/168 | 3/24 | 40/155 | 50/309 | 3/118 | 42/170 | 101/536 | 23/147 | - |
| O2 | 10 | 80/201 | 3/94 | 1/126 | 1/24 | 56/92 | 36/247 | 1/129 | 37/155 | 76/452 | - | 17/480 |
| O2 | 100 | 83/201 | 1/94 | 8/126 | 1/24 | 61/92 | 56/247 | 8/129 | 39/155 | 76/452 | - | 62/480 |
| O2 | 1,000 | 86/201 | 2/94 | 11/126 | 1/24 | 66/92 | 61/247 | 10/129 | 40/155 | 76/452 | - | 77/480 |
| O3 | 10 | 115/253 | 0/120 | 0/168 | 1/24 | 6/155 | 11/309 | 1/118 | 31/170 | 96/536 | 5/147 | - |
| O3 | 100 | 122/253 | 0/120 | 7/168 | 3/24 | 26/155 | 37/309 | 3/118 | 36/170 | 100/536 | 17/147 | - |
| O3 | 1,000 | 128/253 | 0/120 | 11/168 | 3/24 | 35/155 | 45/309 | 3/118 | 36/170 | 101/536 | 23/147 | - |

Full F9 outcomes:

| variant | K | class | undetected | silent wrong (after a repair) | silent wrong (other) | crash (inside a check) | benign, latent | benign, clean | detected, repaired | detected, not repaired (internal diagnostic) | spurious error | not injected |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| N | - | F9 | 744 | 0 | 177 | 439 (0) | 128 | 1010 | 0 | 196 (196) | 50 | 0 |
| O1 | 10 | F9 | 287 | 12 | 100 | 175 (115) | 0 | 471 | 626 | 584 (27) | 32 | 0 |
| O1 | 100 | F9 | 371 | 11 | 116 | 244 (62) | 0 | 591 | 529 | 473 (62) | 36 | 0 |
| O1 | 1,000 | F9 | 404 | 11 | 124 | 269 (54) | 0 | 700 | 486 | 374 (75) | 36 | 0 |
| O2 | 10 | F9 | 308 | 8 | 80 | 220 (160) | 0 | 401 | 649 | 618 (17) | 24 | 0 |
| O2 | 100 | F9 | 395 | 7 | 89 | 299 (106) | 0 | 552 | 538 | 485 (50) | 30 | 0 |
| O2 | 1,000 | F9 | 430 | 6 | 96 | 328 (89) | 0 | 659 | 477 | 403 (65) | 31 | 0 |
| O3 | 10 | F9 | 266 | 0 | 100 | 166 (113) | 0 | 471 | 0 | 1232 (25) | 31 | 0 |
| O3 | 100 | F9 | 351 | 0 | 116 | 235 (64) | 0 | 591 | 0 | 1024 (63) | 34 | 0 |
| O3 | 1,000 | F9 | 385 | 0 | 124 | 261 (54) | 0 | 700 | 0 | 881 (72) | 34 | 0 |

### Overhead (no fault; median of the rounds, ratio to N with its 95% interval)

| workload | variant | K | median s | vs N | peak MB | checks per run |
| --- | --- | --- | --- | --- | --- | --- |
| dec | N | - | 0.7346 | 1.000 [1.000, 1.000] | 2.30 | 0 |
| dec | O1 | 10 | 0.7625 | 1.034 [1.021, 1.051] | 2.30 | 601 |
| dec | O1 | 100 | 0.7501 | 1.023 [1.006, 1.024] | 2.30 | 61 |
| dec | O1 | 1,000 | 0.7414 | 1.007 [0.999, 1.060] | 2.30 | 7 |
| dec | O2 | 10 | 0.7947 | 1.076 [1.066, 1.096] | 2.31 | 601 |
| dec | O2 | 100 | 0.7731 | 1.049 [1.045, 1.061] | 2.31 | 61 |
| dec | O2 | 1,000 | 0.7757 | 1.057 [1.036, 1.059] | 2.31 | 7 |
| dec | O3 | 10 | 0.7322 | 0.993 [0.983, 1.009] | 2.30 | 601 |
| dec | O3 | 100 | 0.7138 | 0.974 [0.954, 1.001] | 2.30 | 61 |
| dec | O3 | 1,000 | 0.7083 | 0.960 [0.950, 0.967] | 2.30 | 7 |
| dens | N | - | 0.1213 | 1.000 [1.000, 1.000] | 1.51 | 0 |
| dens | O1 | 10 | 59.1663 | 487.891 [484.693, 490.075] | 1.51 | 82139 |
| dens | O1 | 100 | 6.0950 | 50.236 [49.568, 50.958] | 1.51 | 8257 |
| dens | O1 | 1,000 | 0.7496 | 6.174 [6.158, 6.397] | 1.51 | 868 |
| dens | O2 | 10 | 67.8148 | 557.193 [555.232, 563.113] | 1.91 | 82139 |
| dens | O2 | 100 | 6.9673 | 57.426 [57.029, 58.370] | 1.91 | 8257 |
| dens | O2 | 1,000 | 0.8450 | 6.943 [6.820, 7.177] | 1.91 | 868 |
| dens | O3 | 10 | 59.5389 | 490.600 [488.633, 492.711] | 1.51 | 82139 |
| dens | O3 | 100 | 6.1377 | 50.598 [49.953, 51.113] | 1.51 | 8257 |
| dens | O3 | 1,000 | 0.7603 | 6.273 [6.237, 6.438] | 1.51 | 868 |

Result checksums equal across variants: dec yes, dens yes.

## Decision (by the pre-registered rule)

**2. Gate** (repair-induced silent wrong, all classes; penalised if its 95% interval lies above 0):

- N K = -: 0 [0, 0] -> passes
- O1 K = 10: 945 [870, 1019] -> penalised
- O1 K = 100: 850 [779, 923] -> penalised
- O1 K = 1,000: 810 [741, 880] -> penalised
- O2 K = 10: 949 [875, 1023] -> penalised
- O2 K = 100: 855 [783, 928] -> penalised
- O2 K = 1,000: 814 [745, 885] -> penalised
- O3 K = 10: 0 [0, 0] -> passes
- O3 K = 100: 0 [0, 0] -> passes
- O3 K = 1,000: 0 [0, 0] -> passes
- ranked after the others (no eligible K): O1, O2

**3. Primary** (undetected over the index faults F1, F2, F5, F6, F7; each variant at its best eligible K):

- N: 5233 at K = -; difference with O3 [5106, 5249] -> worse
- O3: 55 at K = 10; difference with O3 [0, 0] -> winner
  - O3 admissible K: 10

**4. First tie-break** (detected over the index faults; higher is better):

- O3: 9885 at K = 10; difference with O3 [0, 0] -> winner
  - O3 admissible K: 10

**Decision: O3** (admissible K: 10); overhead does not enter.

## Caveats and disclosures

- **Crashes inside the check.** The audit follows pointers and indices that the fault may have corrupted, and can
  crash itself; such crashes count as crashes (undetected) for every checking variant, and are shown in the
  "(inside a check)" column.  They inflate the crash counts of O1, O2 and O3 relative to N.
- **N's detections** come only from M1's own internal consistency checks (`pgi_die` diagnostics such as "unlink: a
  list names a holder without the edge"); they are counted as detected, not repaired, and shown in the "(internal
  diagnostic)" column.  The other variants get the same credit when those checks fire first.
- Moments are uniform over the workload's commands, so a fault lands in whatever state the workload has then
  (64 value slots, 16 array slots); faults in values overwritten before any read end benign (clean or latent).
- F9 is uniform over the live content bytes, not over all allocated memory (free capacity, empty dictionary slots
  and the entry caches are excluded); O2's records add bytes, so its regions differ in size and mix.
- Single-threaded, one graph; faults are injected between operations (F8 inside one).
