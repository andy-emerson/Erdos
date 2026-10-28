# Fault injection: pre-registered decision rule

Oct 28, 2026 · written before any measured trial of the fault-injection test (`test/fault/fault_bench.py`), which decides the consistency option of [spec v0.3 §10](../v0.3.md#10-consistency) (§14).  The measured run refuses to start unless this file contains the rule below verbatim, and writes the rule into `test/fault/results/fault.md` before the first trial.

**Decision rule (pre-registered in research/fault-rule.md on 2026-10-28, before the measured run).**

0. **Trials.** Every variant runs the same trials: the same workloads (2,000 random commands on single values and
   arrays, then a readout of every live value and array), and for each trial the same fault class, moment, injector
   seed and K phase.  N runs once (it has no checks); O1, O2 and O3 run at K = 100, 1,000 and 10,000.  There are
   2,000 trials per class per variant and K (100 workloads, 20 trials per class each).  Each trial ends in one
   outcome: detected and repaired, detected and not repaired, silent wrong, crash, or benign.  A wrong result
   returned as success, or a result where the oracle has an error, is silent wrong; an error status where the oracle
   has a result is reported separately as a spurious error and is not counted as silent wrong.  A trial whose fault
   found no target is excluded and counted.
1. **Primary.** S_bug(v, K) is the number of silent-wrong outcomes over the bug classes F1, F2, F5, F6, F7 and F8.
   F5 counts 0 on N, O1 and O3, which store no node-side exponent, so the fault cannot occur there (shown as n/a).
   A variant's primary value is its minimum of S_bug over K (N: its single value).  Two counts are **tied** when the
   95% interval of their paired difference (bootstrap over workloads, 10,000 resamples, seed 20261028) contains 0.
   The primary winners are the variants whose primary value is tied with the lowest one.  A winner's admissible Ks
   are those at which its S_bug is tied with its own primary value.
2. **Secondary**, among the primary winners: S_hw(v, K) is the number of silent-wrong outcomes over F9 (a single
   bit flip).  A variant's secondary value is its minimum of S_hw over its admissible Ks; the winners are those
   tied with the lowest.  The admissible Ks shrink to those whose S_hw is tied with the variant's secondary value.
3. **Tertiary**, among the secondary winners: a variant's overhead at K is the geometric mean, over the two normal
   workloads, of its time divided by N's time (median of 5 runs each).  Its tertiary value is its lowest overhead
   over its admissible Ks.  The variant with the lowest tertiary value is the decision; values within 5% of each
   other are a tie, which is reported as a tie with no verdict forced.  Memory is reported and does not decide.
4. **Residual risk.** F3 and F4 corrupt the single source of truth itself (option 1's holder factorization;
   option 2's shared edge record), so no cross-check against derived data can tell which side is right: no variant
   can correctly repair them, and a rebuild from that truth enshrines the corruption.  They are not counted in 1-3
   and are reported separately.  A per-holder checksum over the factorization and the numerator, written by every
   update and checked on read or audit, would catch them.
