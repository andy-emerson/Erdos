# Skeleton baselines: results

Run started 2026-10-29T08:04:08-05:00; status: finished. Wall time on the run clock: 13.8 min.

Per measure only: **this report names no overall winner and combines no measures** (rule item 11).

## The measurement rule (research/rule.md, verbatim, written before the first trial)

sha256 `ff2b62379f9e0b7451066ceb413c119fccb73259201d2e5a4165e55d3bcb2c96`

<!-- rule begins -->
# Skeleton baselines: pre-registered measurement rule

Oct 28, 2026 · **Fixed.** Every item of this rule, including every item formerly marked PROPOSED, is approved by the author, Oct 28, 2026; the rule is fixed before any measurement. Approved earlier the same day: the per-measure thresholds (the time band, memory and gcd-work bands, the severe-memory-loss flag) and the 12 interleaved rounds. Revised at the author's direction after the second review, Oct 28, 2026: the rep rule (item 4), the null control's repetition count, counter determinism on a subset (item 2), the use of two cores, the gate schedule (item 3), the trimmed run plan with its pilot and 4-hour stop, and the arrival twin (G4). Revised at the author's direction after the third review, Oct 28, 2026: the LG family is constructed directly, with no redraw (G1); the 4-hour limit is also a hard stop on the whole run; a gate pilot on job 20; Design 2's large-base audits (item 3); timed rounds on one core, one row at a time (item 4); and the calibration of r, stated exactly (item 4). Every item proposed in that revision is now approved, so this file is fixed before any measurement: the harness writes the rule into its results file before the first trial, and refuses a measured run unless this file holds the rule verbatim. A change after that is a revision, dated and explained at the top, and allowed only while nothing has been measured.

**Revision, Oct 29, 2026, approved by the author.** Design 2 was redefined after the pilot because the pilot exposed a design flaw: its eager refinement tested every new denominator against the whole base, about C gcds each ([finding](../../../../09-coprime-base/research/eager-d2-finding.md)). Design 2 is now refined on demand ([spec §8](../../v0.1.md#8-design-2-the-coprime-base), amended Oct 29, 2026). The pilot jobs were also run on the new designs during development, and their gcds, times and memory are reported in the design note; but no measured run under this rule has taken place: nothing from step 4 onwards (gates, counting runs, calibration, rounds) has run, and the pilot is run again on the current code. In the same revision, Design 1's dense pair table is replaced by a hash set of the pairs that met, with the same meaning ([spec §7](../../v0.1.md#7-design-1-common-divisors), amended Oct 29, 2026), because its $D(D-1)/2$ bits made Design 1's memory quadratic in $D$. Also in this revision, Design 1's dense $D \times C$ incidence bit matrix is replaced by sparse rows of (divisor id, residual), with the same facts and results ([spec §7](../../v0.1.md#7-design-1-common-divisors), amended Oct 29, 2026), because it was 7–53% of Design 1's bytes per rational in the pilot. The text that described the replaced forms is updated to match: item 4's reason for keeping n, item 6's note on Design 1's memory, the reason the gate pilot runs job 20, and the run plan's non-normative estimate. One procedure is added: before any timed run, the pilot's included, the harness checks that the machine is idle (its load average and the CPU use of every other process, against thresholds it states), and if it is not, it waits and logs this, and after a stated time refuses to time ([harness decision R-p](../DECISIONS.md)). No other item of this rule changes.

It states how Design 1 (common divisors) and Design 2 (the coprime base) of [spec v0.1](../../v0.1.md) are measured against the control and against each other, at their baselines, and how the results are reported: **per measure only, with no overall winner** (item 11).

**This comparison does not end the debate.** It compares two skeletons, each the plainest correct form of its design, with every improvement left out on purpose. A design that does worse here may have more room to improve: the note lists improvements for each, and they are not equal in size.

## Measurement rule

1. **Rows.** Each job runs these rows:
   - **ctl**: the control;
   - **ctl-null**: a second copy of the control, the same binary run as a separate row;
   - **D1** and **D2**;
   - **ceil**, the shared-denominator ceiling, only on the jobs marked "ceil" in the run plan.

2. **Builds.** All rows are built by one compiler with one set of flags (`-O2`), at width 64 (width 32 for the W32 jobs). The timing build carries no counters or audits. The counting build gives the counts and the memory reports, from one complete run per row and job. Counts are deterministic: on the fixed subset of jobs 3, 9, 14, 18, 19 and 23, every row's counting run is repeated, and the second run MUST give identical counts, or it is a bug, handled by item 5. Every counting run has a limit of **10 minutes**; a run that does not finish within it gives no counts, and its gcd work and memory are reported as **n/a**, with the reason. The test build gives the audits.

3. **Gates before the measured run.** In the test build, every row MUST pass the unit tests and, on every job, the gate of [spec §13.3](../../v0.1.md#133-tests): one complete run, with a full audit after each of the first 200 calls, then after every 64th call, and after the last call; entries are calls and are audited on the same schedule; between full audits, an incremental audit of what changed MAY run after every call. A row that fails a gate is fixed and the gate re-run; no measured run starts until every gate passes. Gates are not timed, so they run two at a time, one per core, but never while a timed row runs (item 4).
   - **Large base (Design 2).** Design 2's full audit checks the base pairwise, $C(C-1)/2$ gcds, which would make the gate quadratic in C at every scheduled audit. So once Design 2's base has more than **1,024 pieces**, each scheduled full audit before the last checks pairwise coprimality incrementally only: `audit_changed` MUST then run after every call, testing each piece created or rewritten by that call against every other piece; everything else of the full audit is still checked in full, and the full audit after the last call is complete, the pairwise check included ([spec §13.3](../../v0.1.md#133-tests)). So no full audit but the last takes more than $1{,}024 \cdot 1{,}023 / 2 = 523{,}776$ coprimality gcds, and every pair of pieces is still checked: when the later of the two was created or last rewritten. Every other audit is linear in the structures and is not capped.

4. **Rounds.** Each job runs **12 interleaved rounds**. In every round, each row runs once, as its own process, in an order rotated by one row per round, so the rows of a round are paired. Each invocation reads the job's workload file, then runs its reps in one process.
   - **A rep** runs the whole job **r** times in sequence, each time on a new, empty class, destroyed after its read-out. The rep's time is its total divided by r.
   - **r** is the smallest power of two for which one rep of that row on that job lasts at least **50 ms**. n stays 20,000 operations per job.
   - **Calibration.** r is found by one **calibration invocation** per job and row (ctl, D1, D2 and, where it runs, ceil), run in the timing build, on the job's workload file, on core 1, immediately before that job's round 1 (for the arrival pair, before their shared round 1). The invocation reads the workload file, then times one rep at r = 1, 2, 4, … in turn, each on new classes as above, and stops at the first r whose rep lasts at least 50 ms; that r is the row's r, and that rep is the **calibration's rep**. Before round 1, the harness writes into the results file, for each row and job, r, the calibration's rep time and the reps per invocation below; they are then **fixed for all 12 rounds** and never changed. Calibration times enter no measure. The pilot's calibration is discarded with the pilot and is not reused.
   - **Reps per invocation.** If the calibration's rep lasts at least **1 s**, the row runs **1 rep** per invocation, and its time is that rep's. Otherwise it runs **3 reps**, the first is discarded, and its time is the minimum of the remaining 2.
   - **ctl-null** uses ctl's r and ctl's number of reps on the same job, and has no calibration of its own.
   - A row's time in a round is that of its **total**: entry + operations + read-out of every stored value as a numerator and denominator in lowest terms. Entry, operations and read-out are also reported separately. Reading the workload file is not timed.
   - **One timed row at a time.** The machine has 2 cores. Every timed invocation (calibration, rounds, and the pilot's timed part) runs on **core 1**, pinned to it, one invocation at a time; the harness runs on **core 0**, pinned to it, and does nothing but start invocations, wait, and record. No other work runs while a timed invocation runs, so nothing that depends on the design or the job runs beside a timed row, and ctl-null's spread measures the remaining noise. Untimed work (workload generation and references, unit tests, gates, counting runs) MAY use both cores, two processes at a time, but only in the steps before the timed rounds (Order of work), never alongside a timed invocation.
   - **Order of jobs.** Jobs run one after another, in job order; each job runs its calibration and all 12 rounds before the next job starts. The arrival pair is the exception: jobs 19 and 20 are calibrated together and share their rounds, each round running the rows of both jobs (8 invocations) in an order rotated by one per round, so that round k of job 19 and round k of job 20 are paired (item 12).
   - **Time cap.** A timed rep of a row that lasts more than **10 s**, in calibration or in any round, ends that row's timing on that job: the row is recorded as **over cap at this size**, which is a reported finding, and is not timed again on that job; its earlier rounds are reported but enter no measure. Its gcd work and memory still come from its counting run, under item 2's limit.

   An earlier experiment made runs long enough to time by raising the number of operations to 200,000. Here n stays 20,000 and r is raised instead, because raising n changes the workload, not only its length: at V = 1 almost every result brings a new denominator, so at n = 200,000 D would reach about 210,000, and every structure that grows with D or with the pairs that meet (Design 1's pair set and sparse incidence rows, Design 2's rows and remainders) would be measured at another size. (The dense pair table that Design 1 had before the revision of Oct 29, 2026 would alone have reached about $D^2/16 \approx 2.8$ GB.)

5. **Checks, and bugs after the gates.** Every stored value of every run is compared with the exact value computed by Python's `fractions` from the workload file. The workloads make overflow impossible (see Size bound), so **a check failure or an overflow after the gates is a bug**, and is handled as follows:
   - the measured run of the affected design stops;
   - the bug is fixed under review ([spec §11](../../v0.1.md#11-instructions-to-builders-and-reviewers)), and the fix is **logged** in the change log, with the job, invocation, operation and step where it appeared;
   - the affected design is **re-run in full**: its gates, its counting runs and every round of every job; its earlier results are discarded. A bug in the control re-runs ctl and ctl-null; a fix to the shared integer routines re-runs every row.

   The report lists every such bug beside the per-stratum tables of item 10.

6. **Measures**, per job and row (X is D1, D2 or ceil):
   - **Time.** The ratio **T(ctl) / T(X)**, T being the median of the row's per-round totals; above 1, X is faster. Its 95% interval [lo, hi] is the 2.5th–97.5th percentile of the same ratio over **10,000 bootstrap resamples** of the rounds (rounds drawn with replacement, the same draw for both rows, so the pairing is kept; seed 20261028).
   - **Gcd work.** **G(X) / G(ctl)**, where G is the counting build's sum, over every gcd call of the job (entry and operations), of the bit length of the larger operand.
   - **Memory.** **B(X) / B(ctl)**, where B is the used bytes per rational ([spec §12.2](../../v0.1.md#122-memory)). B is reported **twice**: **after entry** (when the last entered value is stored) and **at the end of the job**. The memory verdict and the severe-memory-loss flag of item 8 use the end-of-job figure; the after-entry figure is reported beside it.

   **Design 1's memory and the flag.** Design 1's pair set grows with the pairs compared (8 bytes a slot, 16 to 32 bytes a pair), at most one per addition, subtraction or division; its sparse incidence rows grow with the divisor facts recorded, 12 bytes each, and hold one entry for every denominator, result denominators included. On jobs with many denominators per value or many compared pairs, Design 1 may therefore raise the severe-memory-loss flag (≥ 4.0) at the end of the job. That is a finding about the Design 1 skeleton, which this rule is meant to expose, not a flaw of the rule.

   Also reported for every job: every counter of spec §12.1 per phase; entry, operation and read-out times; allocated bytes per rational; the per-structure memory breakdown, after entry and at the end.

7. **Null control.** ctl against ctl-null on the same job, by 6. The job's **null spread** is max(1 − lo, hi − 1, 0) over that interval. The time band's half-width is **h = max(5%, null spread)** (approved by the author, Oct 28, 2026).

8. **Verdict per job and measure, with its magnitude**, for X against the control (approved by the author, Oct 28, 2026):

   | Measure | Win | Loss | Tie | Magnitude reported |
   | --- | --- | --- | --- | --- |
   | Time | lo > 1 + h | hi < 1 − h | otherwise | T(ctl) / T(X) and its interval [lo, hi], with h |
   | Gcd work | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | G(X) / G(ctl) |
   | Memory (end of job) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | B(X) / B(ctl), after entry and at the end |

   Gcd work and memory are deterministic, so they have fixed bands and no interval. A memory ratio ≥ 4.0 at the end of the job raises the **severe-memory-loss flag**, which is reported beside the verdict; it is a flag, not a veto, and changes no other verdict. Every verdict is always reported **together with its magnitude**: a verdict alone is never reported.

   A row **over cap** on a job (item 4) has the time verdict **over cap** there, reported with the time of the rep that passed the cap; it is not a win, loss or tie. A job that is not constructible (see Workloads) is **n/a** for every row and enters no stratum; so is a measure whose counts are n/a (item 2).

9. **Head to head.** D1 against D2 on every job, per measure, by 6 to 8 with D2 in the place of the control: time ratio T(D2) / T(D1) with the paired bootstrap interval and the same band h as the job's null control; gcd work G(D1) / G(D2); memory B(D1) / B(D2), after entry and at the end, with the severe-memory-loss flag at ≥ 4.0. A "win" is a win for D1. If one design is over cap on a job, the time verdict names it ("D1 over cap" or "D2 over cap"); if both are, it is n/a. Each verdict is reported with its ratio and, for time, its interval.

10. **Verdict jobs and strata.** The **verdict jobs** are jobs 1–18 (the matrix) and 21–25 (the chains). The arrival pair (jobs 19 and 20) is reported by item 12, and W32 (jobs 26 and 27) separately, also by item 12. The verdict jobs fall into seven strata:
    - **ALL**, **P75**, **P50**, **P25**, **NONE** and **LG**: three matrix jobs each, one per operation mix;
    - **CHAINS**: jobs 21–25.

    For each stratum, each comparison (D1 against ctl, D2 against ctl, D1 against D2) and each measure (time, memory, gcd work), the report gives the **counts of wins, ties, losses and over-cap results**, the **median ratio** over the stratum's jobs, and the number of severe-memory-loss flags. The per-job verdicts and magnitudes of items 8–9 are listed beneath.

11. **No overall winner.** The rule combines no measures, scores no strata, and names no design as beating the control or as the better baseline. Choosing an overall winner is left to discussion of these per-measure results, since a narrow win on one measure and a large win on another are not comparable by counting.

12. **Also reported:**
    - **Ceiling gap.** On the ceil jobs, T(ceil) / T(X) with its interval, and B(X) / B(ceil), which is deterministic and has no interval: how far each row is from the best possible on fully shared data.
    - **Operation time alone**, entry excluded, with verdicts and magnitudes by 8, to show where entry cost weighs.
    - **Arrival.** For the arriving job 19 and its early twin, job 20 (G4): the verdicts and magnitudes of items 8–9 on each; and, per row, T(19) / T(20), with the paired bootstrap interval over the rounds (the two jobs share their rounds, item 4), G(19) / G(20) and B(19) / B(20). Since the twin runs the same operand pairs in the same order, these ratios measure the effect of arrival alone.
    - **Width 32** (W32, jobs 26 and 27): verdicts and magnitudes by 8–9, and whether the ordering of D1, D2 and ctl on each measure matches that of the width-64 matrix job of the same sharing level and mix.
    - **Entry against operations**, per design: the share of total time and of gcd work spent in entry, and, within operations, in `store`.
    - **Results as operands**, per job: the realized share of result operands, and the number of distinct result denominators used as operands (G3).
    - **Gcd work against time**: a design that removes gcd work but not time at 64 bits is a candidate for wider-number tests, where gcds cost more.

## Deferred: the index-valued pair table

The note's variant of Design 1, a pair table whose cells name the column of the pair's gcd instead of holding 1 bit, is **deferred**. It is a later variant test, run against the Design 1 baseline after this round, and is **not part of this round**: no row, job or verdict here measures it, and the spec keeps it outside the skeleton ([spec §10](../../v0.1.md#10-the-skeletons-boundary)).

## Workloads

All workloads are written to files by one generator (Python, seeded), before any measured run. **Master seed 20261028**; job number n (from the run plan) uses seed 20261028 + n, except the early twin, which is built from job 19's workload. Every workload parameter is approved by the author, Oct 28, 2026: those of the reviewed draft that remain, and those added in the revision: B = 20 in place of 30, and $|m| < 2^B$ (Size bound); the cap $B - 6$ on s and the redraw of s with u (G1); the LG family, its pool of 57 elements and its direct construction (G1); V = 10 for the matrix, and the mul/div mix (run plan); the result share ρ = 1/2 and the result operations (G3, G4); the arrival pair at P50, V = 1, mixed (G4); CH-open at V = 100, CH-muldiv and CH-none (G5); the ceiling's two vector operations on its ALL job; and W32 at B = 9 with its two jobs.

**Size bound.** For a value $x = m/d$ write $H(x) = \max(|m|, d)$. The generator keeps every operation of every job within the **operand rule** $H(x) \cdot H(y) \le (2^{31} - 1)^2$ at width 64. Every full-width intermediate of every row (spec §6–§9, including Design 2's $d_i \cdot r_y$, powers of pieces and leftovers) is at most one operand's height, a product of a quantity at most $H(x)$ with one at most $H(y)$, or a sum or difference of two such products. So every product is at most $(2^{31} - 1)^2 = 2^{62} - 2^{32} + 1$, and every sum at most $2(2^{31} - 1)^2 = 2^{63} - 2^{33} + 2$, about $2^{63} - 2^{33}$. That is below $2^{63} - 1$, the largest magnitude of spec §4.3 item 2, so **no overflow can occur within the generator limits**. At width 32 the rule is $H(x) \cdot H(y) \le (2^{15} - 1)^2$, and the same argument gives products up to $2^{30} - 2^{16} + 1$ and sums up to $2^{31} - 2^{17} + 2 < 2^{31} - 1$.

**The ceiling's common denominator fits.** On ALL data every vector's denominator E divides a power of the one denominator d: E divides d after an addition or subtraction, $d^2$ after a multiplication of two input vectors, and $d^3$ after a multiplication of a result vector by an input vector. So $E \le d^3 < 2^{60}$, since $d < 2^{20}$: **E fits the width.** On the run plan's ceiling jobs it is smaller still: on ALL-add (job 1) $E \le d < 2^{20}$, and on CH-all (job 22), whose steps are all additions, $E \le d < 2^{24}$. The ceiling's numerators $n_k = E \cdot (\text{element})$ are bounded by the operand rule like every other intermediate.

The families keep the rule as follows. An entered value has $H < 2^B$, and a result of two entered values has $H < 2^{2B+1}$. So with B = 20 at width 64, such a result meets an entered value with $H(x) \cdot H(y) < 2^{61}$, and with B = 9 at width 32, below $2^{28}$. Chain values stay below $2^{31}$ by construction or by the redraw rule (G6), and so do their inputs.

**G1. Denominators, `den(D, σ, B)`.** D distinct denominators of about B bits (B = 20 at width 64), with sharing level σ:
- **σ = 0 (NONE):** D distinct primes drawn uniformly from $[2^{B-1}, 2^{B})$. Pairwise coprime.
- **σ in {0.25, 0.50, 0.75} (P25, P50, P75):** each $d = s \cdot u$.
  - The **shared part** s: multiply primes drawn uniformly from the pool {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}, stopping at the first draw that would take s above $\min(\text{round}(\sigma B),\ B - 6)$ bits. The cap $B - 6$ leaves u room to be at least 41.
  - The **private part** u: uniform in $[2^{a-1}, 2^{a})$ with $a = B - \text{bits}(s) \ge 6$, redrawn until it has no prime factor at or below 37.
- **LG (large shared factors):** each $d = s \cdot u$, **constructed directly, with no redraw**:
  - The **pool** of shared parts is fixed, the same for every LG job: the **19 primes** of $K = \{41, 43, \ldots, 127\}$, the primes in $[41, 127]$, and the **38 products** $p \cdot q$ of two distinct primes of K below $2^{12}$ (3 of 11 bits and 35 of 12 bits, from $41 \cdot 43 = 1{,}763$ to $61 \cdot 67 = 4{,}087$): **57** elements.
  - The **private part** u of an element s is a **prime above 127** in $[2^{a-1}, 2^{a})$, with $a = B - \text{bits}(s)$. Each element's admissible u are listed in advance: 872 primes for a 6-bit prime s ($a = 14$), 464 for a 7-bit prime ($a = 13$), 43 for an 11-bit product ($a = 9$) and 23 for a 12-bit product ($a = 8$); 12,198 denominators in all.
  - **Construction.** The D denominators are divided among the 57 elements as evenly as possible: each element gets $\lfloor D / 57 \rfloor$, and $D \bmod 57$ elements, chosen uniformly at random, get one more (17 or 18 at D = 1,000, with 31 elements at 18). Each element draws its u uniformly **without replacement** from its list, and the D denominators $s \cdot u$ are shuffled.
  - **No collision and no exhaustion.** In $d = s \cdot u$, s is the part of d made of primes of K and u is a prime outside K, so d determines s and u: distinct draws give distinct denominators, with no test. The shortest list (23) is longer than the largest allotment (18), so no list runs out. The generator still counts **failures**: one for each denominator whose element has no unused u left when it is drawn, out of the D denominators of the job. By the two facts above the count is 0 on every LG job; a nonzero count is a generator bug, and stops generation.

  *Calculation.* For a prime p of K, let $n_p$ be the number of pool elements it divides (13 for 41, down to 1 for each prime from 101 to 127, which has no product below $2^{12}$). Over ordered pairs of pool elements, those that share a prime number $\sum_p n_p^2 = 763$, less the 38 pairs of an equal product, which share two primes and were counted twice: $725$ of $57^2 = 3{,}249$. The even division makes the shared parts nearly uniform over the pool, so they share a factor with probability about $0.223$. The private parts add a little: two 8- or 9-bit u of different products can be the same prime. The realized fraction is about $0.23$ to $0.24$ (below), between P25 (about $0.10$) and P50 (about $0.38$), with gcds of 6 to 12 bits.

  *Bounds.* Every element has at most 12 bits, so $a \ge 8$, $d < 2^{\text{bits}(s)} \cdot 2^{a} = 2^{20}$, and the result rule above holds at B = 20. *Capacity.* At D = 10,000 the allotment would be 175 or 176, more than a product's list holds, so LG runs only at D = 1,000. A product s shares one prime with other elements, so Design 2 must split composite pieces.
- **ALL:** one denominator, drawn as for P50.
- In NONE and P25–P75, a denominator equal to an earlier one is redrawn (s and u). The generator counts the redraws of each denominator; if any one denominator reaches 1,000 redraws, the job is **not constructible**, and is reported as such. LG needs no redraw (above).

**Realized sharing.** Next to each label (P25, P50, P75, NONE, LG, ALL), the report gives the job's realized sharing, measured over 10,000 random pairs of distinct entered denominators:
- the **fraction of pairs with gcd > 1**;
- the **mean gcd size**, the mean of $\text{bits}(\gcd(d_i, d_j))$ over those pairs;
- the mean of $\text{bits}(\gcd(d_i, d_j)) / \text{bits}(\min(d_i, d_j))$ over all pairs.

*Non-normative:* trial generations at B = 20 and D = 1,000, over five seeds, gave fractions of 0.09–0.10 (P25), 0.37–0.39 (P50), 0.63–0.65 (P75), 0 (NONE) and 0.23–0.24 (LG, by the direct construction), with a mean gcd size of about 4 to 5 bits for P25–P75 and about 6.7 bits for LG; P50 at D = 10,000 (the arrival pair) gave 0.40.

**G2. Values, `vals(dens, V)`.** Each denominator carries exactly V values. Numerators are uniform in $[-(2^{B} - 1),\ 2^{B} - 1]$, redrawn until nonzero and coprime to the denominator, so every input is already in lowest terms and D and V are exact. The values are shuffled; entry order is the shuffled order. $N_{\text{in}} = 10{,}000$, so $D = 10{,}000 / V$.

**G3. Operations, `ops(n, mix, ρ)`.** n = 20,000 operations. Each picks its operation from the mix: **add** (addition only), **muldiv** (multiplication or division, each with probability 1/2) or **mixed** (each of add, sub, mul and div with probability 1/4). Subtraction alone is not a mix: it takes addition's path exactly ($x - y = x + (-y)$, spec §4.2), so an add-only job measures it. Each operation is of one of two kinds:
- a **result operation**, with probability **ρ = 1/2**: its first operand is drawn uniformly from the results of the earlier two-input operations, and its second uniformly from the entered values;
- otherwise a **two-input operation**: both operands drawn uniformly, with replacement, from the entered values.

The first operation of every job is a two-input operation. So **half of all operations, and a quarter of all operands, are earlier results**, and every result operation brings a result's denominator back as an operand. The share is limited by the size bound: a result of a result operation can have about 3B bits (61 at B = 20), too large to meet any further operand, so it is stored and read out but never an operand. The result is always the first operand, so sub and div compute result − entered value and result ÷ entered value.

G3 applies to every job with operations, in every family. The chains (G5) meet it by construction: every step's first operand is a result, so half of all operands are results.

**G4. Arrival: job 19 and its early twin, job 20.** Both are proper jobs with the same values and the **same operations, with the same operand pairs in the same order**; only when the denominators are entered differs.
- **Job 19 (arriving)** is generated as follows, from P50 values with V = 1 (D = 10,000) and the mixed mix. The denominators are split into 20 equal batches in order of first appearance, each with all its values, and the operations into 20 segments of 1,000. Segment t runs after batch t is entered. In segment t, a two-input operation takes one operand from batch t and the other uniformly from batches 1 to t; a result operation takes its result as in G3, from the two-input operations before it, and its entered value from batch t.
- **Job 20 (early twin)** is built from job 19's workload file: it enters all 20 batches first, in the same order, then runs the same 20,000 operations in the same order. Every operand is entered before it is used in both jobs, so the twin is a valid workload, and its results are the same values.

The twin is a separate job, rather than an S-family job of the same parameters, because an S job draws its operand pairs without the batch structure, so a comparison with it would confound arrival with which pairs meet.

**G5. Chains.** 1,000 chains, run round-robin, so every chain's result is an operand of its next step. Chain c starts from an input $x_0$; step k computes $x_{k} = x_{k-1} \circ y_{k}$, with $y_k$ drawn uniformly from the inputs.
- **CH-closed:** K = 32 steps of add or sub (each with probability 1/2). Inputs: 10,000 values over uniformly drawn divisors of $L = 720{,}720 = 2^4 \cdot 3^2 \cdot 5 \cdot 7 \cdot 11 \cdot 13$, with $|m| < d$. Every denominator divides L and every value is below 33 in magnitude, so the size bound holds with no redraw.
- **CH-all:** as CH-closed, but every input is over one denominator drawn as ALL with B = 24; every numerator is below $33 \cdot 2^{24} < 2^{31}$.
- **CH-open:** K = 8 steps, each of the four operations with probability 1/4. Inputs: 10,000 values by G2 with B = 12, σ = 0.5, V = 100 (D = 100; V = 10 is not constructible at 12 bits), $|m| < 2^{12}$. The redraw rule applies.
- **CH-muldiv:** as CH-open, but each step is mul or div, each with probability 1/2. The redraw rule applies.
- **CH-none:** as CH-open, but with σ = 0 (NONE): 100 distinct 12-bit primes, V = 100. The redraw rule applies.

**G6. Redraw rule** (CH-open, CH-muldiv, CH-none). If a step's exact result has $|m|$ or d at or above $2^{31}$, or is a division by zero, $y_k$ is redrawn, up to 16 times; then the chain restarts from a fresh input, and the restart is counted and reported. The generator simulates every chain exactly to apply the rule, so every row runs the same operations.

**Ceiling jobs.** The ceiling runs where every operand shares one denominator and every result of a job shares one: ALL-add (job 1) and CH-all (job 22). On ALL-add, it enters the inputs as one vector; gathers the operands of the two-input operations into two vectors and runs one vector operation, whose result vector holds the results that G3 reuses; then gathers the operands of the result operations, the first from that result vector and the second from the input vector, and runs a second vector operation. Gathering is timed, as operations. On CH-all, it holds the 1,000 chains as one vector; each step gathers the 1,000 $y_k$, negated where the step subtracts, and runs one vector addition. Gathered vectors are working copies and are not counted in its bytes per rational ([spec §12.2](../../v0.1.md#122-memory)). It does not run ALL-muldiv (the quotients' common denominator, an lcm of numerators, does not fit), ALL-mixed (no single vector operation), or any other job.

**Width 32 run.** B = 9 and $|m| < 2^9$, with the width-32 operand rule; G3 applies. B = 9 is the largest B at which a result of two entered values can meet an entered value, and at 9 bits only small D is constructible, so W32 runs two jobs, mixed: P50 at V = 1,000 (D = 10) and ALL; reported by item 12.

## Run plan

Rows ctl, ctl-null, D1, D2 on every job; ceil where marked. Width 64, B = 20 unless stated. Every job has n = 20,000 operations, except the chains. One job per sharing level and operation mix, plus the chains, the arrival pair and two W32 jobs: 27 jobs.

| Jobs | Family | Parameters | Count |
| --- | --- | --- | --- |
| 1–18 | **Matrix** | level in {ALL, P75, P50, P25, NONE, LG} × mix in {add, muldiv, mixed}; V = 10 (D = 1,000), except ALL (V = 10,000, D = 1); ceil on ALL-add (job 1) | 18 |
| 19–20 | **Arrival** | P50, V = 1 (D = 10,000), mixed: 19 arriving, 20 its early twin (G4) | 2 |
| 21 | **CH-closed** | G5 | 1 |
| 22 | **CH-all** | G5; ceil | 1 |
| 23 | **CH-open** | G5, G6 | 1 |
| 24 | **CH-muldiv** | G5, G6 | 1 |
| 25 | **CH-none** | G5, G6 | 1 |
| 26–27 | **W32** (width 32, B = 9; reported only) | P50 at V = 1,000, and ALL; mixed | 2 |

Within the matrix, jobs are numbered in the order level, then mix, as listed: job $1 + 3\ell + k$ for level $\ell$ = 0 (ALL) to 5 (LG) and mix $k$ = 0 (add), 1 (muldiv), 2 (mixed). Coverage of the asked axes: sharing at six levels (σ, LG and ALL); few against many denominators (D = 1 for ALL, 1,000 for the matrix, 10,000 for the arrival pair, Case B of the note, growing with results); early against arriving (job 19 against job 20); each operation family alone and mixed; results reused as operands (G3) and chains (CH).

**Order of work.**
1. Generate the workloads and the exact references.
2. Unit tests, in the test build, at widths 64 and 32.
3. **Pilot**, in two parts:
   - **Timed pilot.** The mixed job of each sharing level (jobs 3, 6, 9, 12, 15 and 18), rows ctl, ctl-null, D1 and D2, in the timing build, with calibration and **2 rounds**, one timed invocation at a time on core 1 as in item 4. Its results and its calibration are discarded.
   - **Gate pilot.** The gate (item 3) of **job 20**, the largest job of the run (10,000 entered denominators, all entered before its first operation), for every row, in the test build, untimed, two rows at a time, after the timed pilot has finished. It is job 20's gate: if every row passes, it is not run again in step 4.

   The harness projects the whole run from the two parts, using the upper figures of the estimate below: the projected total is the time elapsed so far, plus the timed rounds' 35 minutes times the timed pilot's wall time over 2 minutes, plus the remaining gates' 10 minutes times the gate pilot's wall time over 2 minutes, plus the counting runs' 3 minutes times the larger of those two ratios. **If the projected total exceeds 4 hours, the run stops and is reported to the author before anything else runs.**
4. Gates (item 3), two at a time on both cores.
5. Counting runs (item 2), two at a time on both cores.
6. Calibration and the 12 rounds (item 4), job by job, one timed invocation at a time on core 1, with nothing else running.

**Hard stop at 4 hours.** The whole run, from the start of step 1 to the end of the last round, has a hard limit of **4 hours** of wall time: the gates and their audits, the counting runs, the pilot and any re-run under item 5 all count. The harness starts its clock at step 1 and checks it between invocations and, through a timeout on every process, during them. When the limit is reached, it stops whatever is running at once, writes what it has, with the run marked **stopped at 4 hours**, the step and job it was in, and reports to the author. No verdict is reported from a stopped run, and nothing more runs until the author decides.

*Non-normative estimate,* for the designs as revised on Oct 29, 2026, to be confirmed by the pilot. A gcd of pieces or residuals is taken to cost 30–60 ns.
- **Memory.** Design 1's pair set holds only the compared pairs, at most one per addition, subtraction or division, so at most 20,000 on any job: at most $2^{16}$ slots, 0.5 MB. Its sparse rows hold one entry per denominator (up to about 30,000 on the arrival pair, entered and result) plus two per divisor recorded, at 12 bytes an entry. Design 2's base stays small (the development runs of the pilot jobs ended with 3–64 pieces), and its rows and remainders grow with D. On the pilot jobs, run on these designs during development, both designs ended at about 22–71 bytes per rational (the design note).
- **One complete run of a job (r = 1).**
  - ctl, every job: a few ms, so r is 8 to 32, and an invocation (3 reps of 50–100 ms, and process start) takes 0.2–0.35 s.
  - D2: entry computes no gcd against the base, and each addition, subtraction or division meets its two rows, testing each remainder against the other row's few pieces and the two remainders against each other. On the mixed pilot jobs, during development, it took 2–10 ms, with 1.0–3.5 times the control's gcds. Matrix and arrival: a few ms to a few tens of ms.
  - D1: no row shifts on a divisor insertion; each compare merges two short rows and inserts at most two entries. Matrix and arrival: a few ms to a few tens of ms.
  - Chains, W32 and the ceiling: under 50 ms each.

  Every row is expected to run 3 reps per invocation, and none to come near the 10 s cap.
- **One round,** summed over the 27 jobs, one invocation at a time: about 110 invocations (4 rows on each of the 27 jobs, and the ceiling twice), each 0.2–0.35 s, with process start and workload reading: **25–45 s.** Calibration adds about half a round. **Calibration and 12 rounds: 5–10 minutes.**
- **Gates.** About 670 full audits per job and row, each linear in the structures (values, lookup, rows, the pair set; Design 2's pairwise check over a base of tens of pieces), 1–10 ms each on the larger jobs: 1–13 minutes over all jobs and rows. With the test-build runs themselves (under a minute), 2–14 minutes of CPU, **1–7 minutes** on two cores.
- **Pilot:** timed part under a minute (66 invocations: calibration and 2 rounds of 6 jobs); gate pilot 1–2 minutes. **Counting runs,** with the repeated subset: **2–3 minutes.** **Generation, references and unit tests:** about 10 minutes. **Idle checks:** a few seconds each when the machine is idle; a wait, if any, adds to the run clock.
- **The whole run: about 20 to 35 minutes.** If every timed invocation took twice as long, the timed part would take up to about 20 minutes and the whole run under an hour. That is below 3 hours, so no reps or rounds are reduced, and below the 4-hour stop.
- **The projection of step 3** scales fixed figures (35 minutes of rounds per 2 minutes of timed pilot, 10 minutes of gates per 2 minutes of gate pilot, 3 minutes of counting) by the pilot's wall times. They were set for the earlier designs, and they are ratios: the timed rounds and calibration (about 1,400 invocations) are about 21 times the timed pilot (66), against 17.5 in the figures, so the projection of the timed part is about 15% low. That is far inside the 4-hour stop, which is checked throughout the run in any case.
<!-- rule ends -->

## Calibration (fixed for all 12 rounds; enters no measure)

| job | row | r | calibration rep, r runs (ms) | reps per invocation |
| --- | --- | --- | --- | --- |
| 1 | ctl | 64 | 90.75 | 3 |
| 1 | ctl-null | 64 (ctl's) | 90.75 (ctl's) | 3 (ctl's) |
| 1 | D1 | 64 | 96.07 | 3 |
| 1 | D2 | 32 | 55.54 | 3 |
| 1 | ceil | 64 | 86.24 | 3 |
| 2 | ctl | 32 | 56.55 | 3 |
| 2 | ctl-null | 32 (ctl's) | 56.55 (ctl's) | 3 (ctl's) |
| 2 | D1 | 32 | 72.66 | 3 |
| 2 | D2 | 32 | 65.63 | 3 |
| 3 | ctl | 32 | 51.48 | 3 |
| 3 | ctl-null | 32 (ctl's) | 51.48 (ctl's) | 3 (ctl's) |
| 3 | D1 | 32 | 61.15 | 3 |
| 3 | D2 | 32 | 63.26 | 3 |
| 4 | ctl | 32 | 78.12 | 3 |
| 4 | ctl-null | 32 (ctl's) | 78.12 (ctl's) | 3 (ctl's) |
| 4 | D1 | 8 | 52.61 | 3 |
| 4 | D2 | 8 | 72.88 | 3 |
| 5 | ctl | 32 | 79.86 | 3 |
| 5 | ctl-null | 32 (ctl's) | 79.86 (ctl's) | 3 (ctl's) |
| 5 | D1 | 16 | 87.38 | 3 |
| 5 | D2 | 8 | 71.00 | 3 |
| 6 | ctl | 32 | 80.58 | 3 |
| 6 | ctl-null | 32 (ctl's) | 80.58 (ctl's) | 3 (ctl's) |
| 6 | D1 | 16 | 96.86 | 3 |
| 6 | D2 | 8 | 73.94 | 3 |
| 7 | ctl | 32 | 70.65 | 3 |
| 7 | ctl-null | 32 (ctl's) | 70.65 (ctl's) | 3 (ctl's) |
| 7 | D1 | 16 | 86.24 | 3 |
| 7 | D2 | 8 | 61.77 | 3 |
| 8 | ctl | 32 | 93.52 | 3 |
| 8 | ctl-null | 32 (ctl's) | 93.52 (ctl's) | 3 (ctl's) |
| 8 | D1 | 16 | 81.59 | 3 |
| 8 | D2 | 8 | 58.01 | 3 |
| 9 | ctl | 32 | 78.03 | 3 |
| 9 | ctl-null | 32 (ctl's) | 78.03 (ctl's) | 3 (ctl's) |
| 9 | D1 | 16 | 84.09 | 3 |
| 9 | D2 | 8 | 63.74 | 3 |
| 10 | ctl | 32 | 57.98 | 3 |
| 10 | ctl-null | 32 (ctl's) | 57.98 (ctl's) | 3 (ctl's) |
| 10 | D1 | 16 | 67.66 | 3 |
| 10 | D2 | 16 | 81.01 | 3 |
| 11 | ctl | 32 | 82.07 | 3 |
| 11 | ctl-null | 32 (ctl's) | 82.07 (ctl's) | 3 (ctl's) |
| 11 | D1 | 16 | 72.25 | 3 |
| 11 | D2 | 16 | 82.66 | 3 |
| 12 | ctl | 32 | 72.30 | 3 |
| 12 | ctl-null | 32 (ctl's) | 72.30 (ctl's) | 3 (ctl's) |
| 12 | D1 | 16 | 70.12 | 3 |
| 12 | D2 | 16 | 85.29 | 3 |
| 13 | ctl | 32 | 53.18 | 3 |
| 13 | ctl-null | 32 (ctl's) | 53.18 (ctl's) | 3 (ctl's) |
| 13 | D1 | 16 | 56.80 | 3 |
| 13 | D2 | 32 | 78.17 | 3 |
| 14 | ctl | 32 | 83.94 | 3 |
| 14 | ctl-null | 32 (ctl's) | 83.94 (ctl's) | 3 (ctl's) |
| 14 | D1 | 16 | 65.80 | 3 |
| 14 | D2 | 16 | 55.44 | 3 |
| 15 | ctl | 32 | 70.79 | 3 |
| 15 | ctl-null | 32 (ctl's) | 70.79 (ctl's) | 3 (ctl's) |
| 15 | D1 | 16 | 66.25 | 3 |
| 15 | D2 | 16 | 58.63 | 3 |
| 16 | ctl | 32 | 63.32 | 3 |
| 16 | ctl-null | 32 (ctl's) | 63.32 (ctl's) | 3 (ctl's) |
| 16 | D1 | 16 | 77.14 | 3 |
| 16 | D2 | 16 | 97.85 | 3 |
| 17 | ctl | 32 | 81.85 | 3 |
| 17 | ctl-null | 32 (ctl's) | 81.85 (ctl's) | 3 (ctl's) |
| 17 | D1 | 16 | 77.16 | 3 |
| 17 | D2 | 16 | 98.70 | 3 |
| 18 | ctl | 32 | 74.56 | 3 |
| 18 | ctl-null | 32 (ctl's) | 74.56 (ctl's) | 3 (ctl's) |
| 18 | D1 | 16 | 79.02 | 3 |
| 18 | D2 | 8 | 52.04 | 3 |
| 19 | ctl | 32 | 78.07 | 3 |
| 19 | ctl-null | 32 (ctl's) | 78.07 (ctl's) | 3 (ctl's) |
| 19 | D1 | 16 | 91.91 | 3 |
| 19 | D2 | 8 | 60.93 | 3 |
| 20 | ctl | 32 | 78.15 | 3 |
| 20 | ctl-null | 32 (ctl's) | 78.15 (ctl's) | 3 (ctl's) |
| 20 | D1 | 16 | 91.75 | 3 |
| 20 | D2 | 8 | 61.31 | 3 |
| 21 | ctl | 32 | 78.05 | 3 |
| 21 | ctl-null | 32 (ctl's) | 78.05 (ctl's) | 3 (ctl's) |
| 21 | D1 | 16 | 89.70 | 3 |
| 21 | D2 | 16 | 84.81 | 3 |
| 22 | ctl | 32 | 82.13 | 3 |
| 22 | ctl-null | 32 (ctl's) | 82.13 (ctl's) | 3 (ctl's) |
| 22 | D1 | 32 | 90.54 | 3 |
| 22 | D2 | 16 | 67.60 | 3 |
| 22 | ceil | 32 | 86.44 | 3 |
| 23 | ctl | 64 | 55.84 | 3 |
| 23 | ctl-null | 64 (ctl's) | 55.84 (ctl's) | 3 (ctl's) |
| 23 | D1 | 32 | 59.00 | 3 |
| 23 | D2 | 32 | 67.74 | 3 |
| 24 | ctl | 64 | 56.29 | 3 |
| 24 | ctl-null | 64 (ctl's) | 56.29 (ctl's) | 3 (ctl's) |
| 24 | D1 | 32 | 53.25 | 3 |
| 24 | D2 | 32 | 69.35 | 3 |
| 25 | ctl | 128 | 97.70 | 3 |
| 25 | ctl-null | 128 (ctl's) | 97.70 (ctl's) | 3 (ctl's) |
| 25 | D1 | 64 | 90.29 | 3 |
| 25 | D2 | 64 | 81.95 | 3 |
| 26 | ctl | 64 | 84.33 | 3 |
| 26 | ctl-null | 64 (ctl's) | 84.33 (ctl's) | 3 (ctl's) |
| 26 | D1 | 32 | 69.35 | 3 |
| 26 | D2 | 32 | 70.63 | 3 |
| 27 | ctl | 64 | 65.44 | 3 |
| 27 | ctl-null | 64 (ctl's) | 65.44 (ctl's) | 3 (ctl's) |
| 27 | D1 | 64 | 77.40 | 3 |
| 27 | D2 | 64 | 73.20 | 3 |

## Run

- **Pilot** (discarded): timed part 13.4 s, gate pilot (job 20) 1.0 s; projected total 0.08 h (limit 4 h).
- **Unit tests** (test build, widths 64 and 32): unit-0-w32 pass, unit-0-w64 pass
- **Gates:** 83 of 83 passed.
- **Counting runs:** 83; n/a: none. Repeated (jobs 3, 9, 14, 18, 19, 23): D1:14: identical; D1:18: identical; D1:19: identical; D1:23: identical; D1:3: identical; D1:9: identical; D2:14: identical; D2:18: identical; D2:19: identical; D2:23: identical; D2:3: identical; D2:9: identical; ctl:14: identical; ctl:18: identical; ctl:19: identical; ctl:23: identical; ctl:3: identical; ctl:9: identical.

### Bugs after the gates (item 5)

None.

## Workloads and realized sharing

Realized sharing over 10,000 random pairs of distinct entered denominators: fraction with gcd > 1; mean bits(gcd) over those pairs; mean bits(gcd) / bits(min) over all pairs. Result operands (G3): realized share of operands that are earlier results, and distinct result denominators used as operands.

| job | label | D | V | gcd > 1 | mean gcd bits | mean rel. | result operands | distinct result dens | notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | ALL-add | 1 | 10000 | n/a | n/a | n/a | 0.249 | 5 | redraws_total 0, redraws_max 0 |
| 2 | ALL-muldiv | 1 | 10000 | n/a | n/a | n/a | 0.248 | 2355 | redraws_total 0, redraws_max 0 |
| 3 | ALL-mixed | 1 | 10000 | n/a | n/a | n/a | 0.249 | 1222 | redraws_total 0, redraws_max 0 |
| 4 | P75-add | 1000 | 10 | 0.6319 | 5.01 | 0.182 | 0.250 | 5017 | redraws_total 68, redraws_max 2 |
| 5 | P75-muldiv | 1000 | 10 | 0.6422 | 5.07 | 0.187 | 0.252 | 5028 | redraws_total 71, redraws_max 2 |
| 6 | P75-mixed | 1000 | 10 | 0.6331 | 5.10 | 0.186 | 0.250 | 4986 | redraws_total 72, redraws_max 3 |
| 7 | P50-add | 1000 | 10 | 0.3792 | 4.29 | 0.116 | 0.250 | 4981 | redraws_total 32, redraws_max 2 |
| 8 | P50-muldiv | 1000 | 10 | 0.3919 | 4.26 | 0.118 | 0.249 | 4989 | redraws_total 23, redraws_max 1 |
| 9 | P50-mixed | 1000 | 10 | 0.3785 | 4.21 | 0.114 | 0.250 | 4978 | redraws_total 21, redraws_max 2 |
| 10 | P25-add | 1000 | 10 | 0.1044 | 3.76 | 0.067 | 0.247 | 4922 | redraws_total 4, redraws_max 1 |
| 11 | P25-muldiv | 1000 | 10 | 0.1018 | 3.83 | 0.067 | 0.252 | 5026 | redraws_total 9, redraws_max 1 |
| 12 | P25-mixed | 1000 | 10 | 0.1007 | 4.00 | 0.067 | 0.251 | 5017 | redraws_total 10, redraws_max 2 |
| 13 | NONE-add | 1000 | 10 | 0.0000 | n/a | 0.050 | 0.251 | 4997 | redraws_total 11, redraws_max 1 |
| 14 | NONE-muldiv | 1000 | 10 | 0.0000 | n/a | 0.050 | 0.252 | 4966 | redraws_total 15, redraws_max 1 |
| 15 | NONE-mixed | 1000 | 10 | 0.0000 | n/a | 0.050 | 0.251 | 5000 | redraws_total 10, redraws_max 1 |
| 16 | LG-add | 1000 | 10 | 0.2330 | 6.67 | 0.120 | 0.252 | 4973 | redraws_total 0, redraws_max 0, lg_failures 0 |
| 17 | LG-muldiv | 1000 | 10 | 0.2283 | 6.73 | 0.119 | 0.252 | 4961 | redraws_total 0, redraws_max 0, lg_failures 0 |
| 18 | LG-mixed | 1000 | 10 | 0.2373 | 6.69 | 0.121 | 0.251 | 5032 | redraws_total 0, redraws_max 0, lg_failures 0 |
| 19 | ARR-arriving | 10000 | 1 | 0.4132 | 3.95 | 0.115 | 0.253 | 4953 | redraws_total 3196, redraws_max 11 |
| 20 | ARR-early-twin | 10000 | 1 | 0.4132 | 3.95 | 0.115 | 0.253 | 4953 | redraws_total 3196, redraws_max 11 |
| 21 | CH-closed | 240 |  | 0.9358 | 6.06 | 0.695 | 0.484 | 226 | restarts 0, g6_redraws 0 |
| 22 | CH-all | 1 |  | n/a | n/a | n/a | 0.484 | 12 | restarts 0, g6_redraws 0 |
| 23 | CH-open | 100 | 100 | 0.1588 | 4.37 | 0.136 | 0.291 | 4058 | redraws_total 35, redraws_max 4, restarts 2351, g6_redraws 57798 |
| 24 | CH-muldiv | 100 | 100 | 0.1535 | 4.01 | 0.128 | 0.299 | 4554 | redraws_total 24, redraws_max 5, restarts 2209, g6_redraws 54986 |
| 25 | CH-none | 100 | 100 | 0.0000 | n/a | 0.083 | 0.141 | 1854 | redraws_total 40, redraws_max 6, restarts 4741, g6_redraws 93753 |
| 26 | W32-P50-mixed | 10 | 1000 | 0.0229 | 2.00 | 0.127 | 0.249 | 1115 | redraws_total 3, redraws_max 2 |
| 27 | W32-ALL-mixed | 1 | 10000 | n/a | n/a | n/a | 0.251 | 432 | redraws_total 0, redraws_max 0 |

## Per-stratum results (item 10)

Verdict jobs only (1-18, 21-25). For each comparison and measure: wins, ties, losses, over-cap results (and n/a), the median ratio over the stratum's jobs that have one, and severe-memory-loss flags. Time ratio T(ctl)/T(X) (head to head T(D2)/T(D1)); gcd work G(X)/G(ctl) (G(D1)/G(D2)); memory B(X)/B(ctl) (B(D1)/B(D2)), end of job. A win in D1 vs D2 is a win for D1. Bugs are listed under Run.

### ALL (jobs 1, 2, 3)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.838 |  |
| D1 vs ctl | Gcd work | 2 | 1 | 0 | 0 | 0 | 0.839 |  |
| D1 vs ctl | Memory (end of job) | 1 | 0 | 2 | 0 | 0 | 1.386 | 0 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.863 |  |
| D2 vs ctl | Gcd work | 2 | 1 | 0 | 0 | 0 | 0.876 |  |
| D2 vs ctl | Memory (end of job) | 1 | 0 | 2 | 0 | 0 | 1.358 | 0 |
| D1 vs D2 | Time | 0 | 2 | 1 | 0 | 0 | 1.030 |  |
| D1 vs D2 | Gcd work | 0 | 2 | 1 | 0 | 0 | 0.971 |  |
| D1 vs D2 | Memory (end of job) | 0 | 3 | 0 | 0 | 0 | 1.021 | 0 |

### P75 (jobs 4, 5, 6)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.416 |  |
| D1 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.989 |  |
| D1 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.735 | 1 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.271 |  |
| D2 vs ctl | Gcd work | 0 | 0 | 3 | 0 | 0 | 2.045 |  |
| D2 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 4.284 | 2 |
| D1 vs D2 | Time | 3 | 0 | 0 | 0 | 0 | 1.535 |  |
| D1 vs D2 | Gcd work | 3 | 0 | 0 | 0 | 0 | 0.484 |  |
| D1 vs D2 | Memory (end of job) | 1 | 2 | 0 | 0 | 0 | 0.918 | 0 |

### P50 (jobs 7, 8, 9)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.457 |  |
| D1 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.992 |  |
| D1 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.599 | 1 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.305 |  |
| D2 vs ctl | Gcd work | 0 | 0 | 3 | 0 | 0 | 2.075 |  |
| D2 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.763 | 1 |
| D1 vs D2 | Time | 3 | 0 | 0 | 0 | 0 | 1.461 |  |
| D1 vs D2 | Gcd work | 3 | 0 | 0 | 0 | 0 | 0.478 |  |
| D1 vs D2 | Memory (end of job) | 0 | 3 | 0 | 0 | 0 | 1.023 | 0 |

### P25 (jobs 10, 11, 12)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.515 |  |
| D1 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.998 |  |
| D1 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.474 | 1 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.426 |  |
| D2 vs ctl | Gcd work | 0 | 0 | 3 | 0 | 0 | 1.636 |  |
| D2 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.132 | 0 |
| D1 vs D2 | Time | 3 | 0 | 0 | 0 | 0 | 1.209 |  |
| D1 vs D2 | Gcd work | 3 | 0 | 0 | 0 | 0 | 0.610 |  |
| D1 vs D2 | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 1.166 | 0 |

### NONE (jobs 13, 14, 15)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.570 |  |
| D1 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.999 |  |
| D1 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.416 | 0 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.732 |  |
| D2 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 1.001 |  |
| D2 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 2.695 | 0 |
| D1 vs D2 | Time | 0 | 0 | 3 | 0 | 0 | 0.779 |  |
| D1 vs D2 | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.998 |  |
| D1 vs D2 | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 1.267 | 0 |

### LG (jobs 16, 17, 18)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.479 |  |
| D1 vs ctl | Gcd work | 0 | 3 | 0 | 0 | 0 | 0.992 |  |
| D1 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.532 | 1 |
| D2 vs ctl | Time | 0 | 0 | 3 | 0 | 0 | 0.362 |  |
| D2 vs ctl | Gcd work | 0 | 0 | 3 | 0 | 0 | 1.737 |  |
| D2 vs ctl | Memory (end of job) | 0 | 0 | 3 | 0 | 0 | 3.502 | 0 |
| D1 vs D2 | Time | 3 | 0 | 0 | 0 | 0 | 1.276 |  |
| D1 vs D2 | Gcd work | 3 | 0 | 0 | 0 | 0 | 0.571 |  |
| D1 vs D2 | Memory (end of job) | 0 | 3 | 0 | 0 | 0 | 1.089 | 0 |

### CHAINS (jobs 21, 22, 23, 24, 25)

| comparison | measure | win | tie | loss | over cap | n/a | median ratio | flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| D1 vs ctl | Time | 0 | 0 | 5 | 0 | 0 | 0.529 |  |
| D1 vs ctl | Gcd work | 2 | 3 | 0 | 0 | 0 | 0.946 |  |
| D1 vs ctl | Memory (end of job) | 1 | 0 | 4 | 0 | 0 | 2.156 | 0 |
| D2 vs ctl | Time | 0 | 0 | 5 | 0 | 0 | 0.458 |  |
| D2 vs ctl | Gcd work | 0 | 1 | 4 | 0 | 0 | 1.212 |  |
| D2 vs ctl | Memory (end of job) | 2 | 0 | 3 | 0 | 0 | 1.760 | 0 |
| D1 vs D2 | Time | 3 | 1 | 1 | 0 | 0 | 1.143 |  |
| D1 vs D2 | Gcd work | 4 | 1 | 0 | 0 | 0 | 0.709 |  |
| D1 vs D2 | Memory (end of job) | 0 | 3 | 2 | 0 | 0 | 1.022 | 0 |

## Per-job verdicts and magnitudes (items 7-9)

Every verdict with its magnitude. Time: ratio [95% interval], h. Memory: B ratio at the end (the verdict) and after entry.

### Job 1: ALL-add

null control T(ctl)/T(ctl-null) 1.000 [0.997, 1.003], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.941 [0.938, 0.943], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.969 |
| D1 vs ctl | Memory (end of job) | win | end 0.751, after entry 0.751 |
| D2 vs ctl | Time | loss | 0.913 [0.910, 0.917], h = 0.050 |
| D2 vs ctl | Gcd work | tie | 0.999 |
| D2 vs ctl | Memory (end of job) | win | end 0.751, after entry 0.751 |
| D1 vs D2 | Time | tie | 1.031 [1.028, 1.033], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.971 |
| D1 vs D2 | Memory (end of job) | tie | end 1.001, after entry 1.000 |
| ceil vs ctl | Time | tie | 1.051 [1.048, 1.054], h = 0.050 |
| ceil vs ctl | Gcd work | win | 0.000 |
| ceil vs ctl | Memory (end of job) | win | end 0.500, after entry 0.500 |

### Job 2: ALL-muldiv

null control T(ctl)/T(ctl-null) 1.001 [0.995, 1.003], spread 0.005, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.773 [0.771, 0.775], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.823 |
| D1 vs ctl | Memory (end of job) | loss | end 1.914, after entry 0.751 |
| D2 vs ctl | Time | loss | 0.863 [0.860, 0.864], h = 0.050 |
| D2 vs ctl | Gcd work | win | 0.685 |
| D2 vs ctl | Memory (end of job) | loss | end 1.821, after entry 0.751 |
| D1 vs D2 | Time | loss | 0.896 [0.893, 0.900], h = 0.050 |
| D1 vs D2 | Gcd work | loss | 1.200 |
| D1 vs D2 | Memory (end of job) | tie | end 1.051, after entry 1.000 |

### Job 3: ALL-mixed

null control T(ctl)/T(ctl-null) 0.999 [0.996, 1.003], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.838 [0.833, 0.841], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.839 |
| D1 vs ctl | Memory (end of job) | loss | end 1.386, after entry 0.751 |
| D2 vs ctl | Time | loss | 0.813 [0.808, 0.815], h = 0.050 |
| D2 vs ctl | Gcd work | win | 0.876 |
| D2 vs ctl | Memory (end of job) | loss | end 1.358, after entry 0.751 |
| D1 vs D2 | Time | tie | 1.030 [1.027, 1.037], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.958 |
| D1 vs D2 | Memory (end of job) | tie | end 1.021, after entry 1.000 |

### Job 4: P75-add

null control T(ctl)/T(ctl-null) 1.000 [0.997, 1.002], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.372 [0.369, 0.373], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.984 |
| D1 vs ctl | Memory (end of job) | loss | end 4.378, after entry 1.026 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.268 [0.267, 0.269], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.851 |
| D2 vs ctl | Memory (end of job) | loss | end 4.770, after entry 1.001 **severe-memory-loss flag** |
| D1 vs D2 | Time | win | 1.388 [1.375, 1.395], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.531 |
| D1 vs D2 | Memory (end of job) | tie | end 0.918, after entry 1.025 |

### Job 5: P75-muldiv

null control T(ctl)/T(ctl-null) 1.000 [0.998, 1.003], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.461 [0.459, 0.463], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.993 |
| D1 vs ctl | Memory (end of job) | loss | end 3.653, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.284 [0.283, 0.285], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.212 |
| D2 vs ctl | Memory (end of job) | loss | end 3.798, after entry 1.001 |
| D1 vs D2 | Time | win | 1.622 [1.614, 1.631], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.449 |
| D1 vs D2 | Memory (end of job) | tie | end 0.962, after entry 1.025 |

### Job 6: P75-mixed

null control T(ctl)/T(ctl-null) 1.001 [0.999, 1.006], spread 0.006, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.416 [0.413, 0.419], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.989 |
| D1 vs ctl | Memory (end of job) | loss | end 3.735, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.271 [0.270, 0.273], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.045 |
| D2 vs ctl | Memory (end of job) | loss | end 4.284, after entry 1.001 **severe-memory-loss flag** |
| D1 vs D2 | Time | win | 1.535 [1.520, 1.548], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.484 |
| D1 vs D2 | Memory (end of job) | win | end 0.872, after entry 1.025 |

### Job 7: P50-add

null control T(ctl)/T(ctl-null) 0.998 [0.988, 1.001], spread 0.012, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.408 [0.407, 0.411], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.989 |
| D1 vs ctl | Memory (end of job) | loss | end 4.190, after entry 1.026 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.287 [0.283, 0.289], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.101 |
| D2 vs ctl | Memory (end of job) | loss | end 4.094, after entry 1.001 **severe-memory-loss flag** |
| D1 vs D2 | Time | win | 1.422 [1.414, 1.442], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.471 |
| D1 vs D2 | Memory (end of job) | tie | end 1.023, after entry 1.025 |

### Job 8: P50-muldiv

null control T(ctl)/T(ctl-null) 1.000 [0.995, 1.003], spread 0.005, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.513 [0.507, 0.516], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.996 |
| D1 vs ctl | Memory (end of job) | loss | end 3.551, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.351 [0.349, 0.353], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.963 |
| D2 vs ctl | Memory (end of job) | loss | end 3.399, after entry 1.001 |
| D1 vs D2 | Time | win | 1.461 [1.445, 1.468], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.507 |
| D1 vs D2 | Memory (end of job) | tie | end 1.045, after entry 1.025 |

### Job 9: P50-mixed

null control T(ctl)/T(ctl-null) 1.000 [0.997, 1.004], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.457 [0.454, 0.462], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.992 |
| D1 vs ctl | Memory (end of job) | loss | end 3.599, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.305 [0.304, 0.306], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.075 |
| D2 vs ctl | Memory (end of job) | loss | end 3.763, after entry 1.001 |
| D1 vs D2 | Time | win | 1.495 [1.488, 1.511], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.478 |
| D1 vs D2 | Memory (end of job) | tie | end 0.956, after entry 1.025 |

### Job 10: P25-add

null control T(ctl)/T(ctl-null) 1.000 [0.997, 1.004], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.429 [0.427, 0.432], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.995 |
| D1 vs ctl | Memory (end of job) | loss | end 4.031, after entry 1.026 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.355 [0.353, 0.357], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.928 |
| D2 vs ctl | Memory (end of job) | loss | end 3.325, after entry 1.001 |
| D1 vs D2 | Time | win | 1.210 [1.203, 1.218], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.516 |
| D1 vs D2 | Memory (end of job) | loss | end 1.213, after entry 1.025 |

### Job 11: P25-muldiv

null control T(ctl)/T(ctl-null) 1.000 [0.998, 1.004], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.571 [0.569, 0.573], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.999 |
| D1 vs ctl | Memory (end of job) | loss | end 3.462, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.497 [0.497, 0.499], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.439 |
| D2 vs ctl | Memory (end of job) | loss | end 2.970, after entry 1.001 |
| D1 vs D2 | Time | win | 1.148 [1.145, 1.152], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.694 |
| D1 vs D2 | Memory (end of job) | loss | end 1.166, after entry 1.025 |

### Job 12: P25-mixed

null control T(ctl)/T(ctl-null) 1.001 [0.994, 1.019], spread 0.019, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.515 [0.512, 0.526], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.998 |
| D1 vs ctl | Memory (end of job) | loss | end 3.474, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.426 [0.422, 0.433], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.636 |
| D2 vs ctl | Memory (end of job) | loss | end 3.132, after entry 1.001 |
| D1 vs D2 | Time | win | 1.209 [1.200, 1.226], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.610 |
| D1 vs D2 | Memory (end of job) | loss | end 1.109, after entry 1.025 |

### Job 13: NONE-add

null control T(ctl)/T(ctl-null) 1.000 [0.996, 1.007], spread 0.007, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.481 [0.476, 0.485], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.997 |
| D1 vs ctl | Memory (end of job) | loss | end 3.954, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.673 [0.663, 0.677], h = 0.050 |
| D2 vs ctl | Gcd work | tie | 1.009 |
| D2 vs ctl | Memory (end of job) | loss | end 2.695, after entry 1.001 |
| D1 vs D2 | Time | loss | 0.714 [0.707, 0.726], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.988 |
| D1 vs D2 | Memory (end of job) | loss | end 1.467, after entry 1.025 |

### Job 14: NONE-muldiv

null control T(ctl)/T(ctl-null) 1.000 [0.996, 1.004], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.640 [0.637, 0.642], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 1.000 |
| D1 vs ctl | Memory (end of job) | loss | end 3.416, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.774 [0.767, 0.776], h = 0.050 |
| D2 vs ctl | Gcd work | tie | 1.001 |
| D2 vs ctl | Memory (end of job) | loss | end 2.697, after entry 1.001 |
| D1 vs D2 | Time | loss | 0.827 [0.823, 0.835], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.999 |
| D1 vs D2 | Memory (end of job) | loss | end 1.267, after entry 1.025 |

### Job 15: NONE-mixed

null control T(ctl)/T(ctl-null) 1.000 [0.996, 1.007], spread 0.007, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.570 [0.567, 0.576], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.999 |
| D1 vs ctl | Memory (end of job) | loss | end 3.412, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.732 [0.728, 0.739], h = 0.050 |
| D2 vs ctl | Gcd work | tie | 1.001 |
| D2 vs ctl | Memory (end of job) | loss | end 2.694, after entry 1.001 |
| D1 vs D2 | Time | loss | 0.779 [0.773, 0.786], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.998 |
| D1 vs D2 | Memory (end of job) | loss | end 1.267, after entry 1.025 |

### Job 16: LG-add

null control T(ctl)/T(ctl-null) 0.999 [0.997, 1.002], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.405 [0.404, 0.409], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.987 |
| D1 vs ctl | Memory (end of job) | loss | end 4.101, after entry 1.026 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.323 [0.322, 0.325], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.779 |
| D2 vs ctl | Memory (end of job) | loss | end 3.762, after entry 1.001 |
| D1 vs D2 | Time | win | 1.253 [1.246, 1.264], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.555 |
| D1 vs D2 | Memory (end of job) | tie | end 1.090, after entry 1.025 |

### Job 17: LG-muldiv

null control T(ctl)/T(ctl-null) 1.000 [0.995, 1.002], spread 0.005, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.530 [0.529, 0.533], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.996 |
| D1 vs ctl | Memory (end of job) | loss | end 3.498, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.415 [0.414, 0.417], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.616 |
| D2 vs ctl | Memory (end of job) | loss | end 3.213, after entry 1.001 |
| D1 vs D2 | Time | win | 1.276 [1.271, 1.283], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.617 |
| D1 vs D2 | Memory (end of job) | tie | end 1.089, after entry 1.025 |

### Job 18: LG-mixed

null control T(ctl)/T(ctl-null) 0.999 [0.996, 1.001], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.479 [0.476, 0.481], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.992 |
| D1 vs ctl | Memory (end of job) | loss | end 3.532, after entry 1.026 |
| D2 vs ctl | Time | loss | 0.362 [0.360, 0.363], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.737 |
| D2 vs ctl | Memory (end of job) | loss | end 3.502, after entry 1.001 |
| D1 vs D2 | Time | win | 1.322 [1.316, 1.331], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.571 |
| D1 vs D2 | Memory (end of job) | tie | end 1.009, after entry 1.025 |

### Job 19: ARR-arriving

null control T(ctl)/T(ctl-null) 1.002 [0.997, 1.003], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.443 [0.440, 0.444], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.999 |
| D1 vs ctl | Memory (end of job) | loss | end 4.417, after entry 4.450 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.328 [0.325, 0.330], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.032 |
| D2 vs ctl | Memory (end of job) | loss | end 3.958, after entry 3.966 |
| D1 vs D2 | Time | win | 1.352 [1.345, 1.356], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.492 |
| D1 vs D2 | Memory (end of job) | loss | end 1.116, after entry 1.122 |

### Job 20: ARR-early-twin

null control T(ctl)/T(ctl-null) 0.999 [0.997, 1.003], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.441 [0.438, 0.442], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.999 |
| D1 vs ctl | Memory (end of job) | loss | end 4.417, after entry 3.819 **severe-memory-loss flag** |
| D2 vs ctl | Time | loss | 0.328 [0.325, 0.328], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 2.032 |
| D2 vs ctl | Memory (end of job) | loss | end 3.958, after entry 3.569 |
| D1 vs D2 | Time | win | 1.347 [1.342, 1.357], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.492 |
| D1 vs D2 | Memory (end of job) | loss | end 1.116, after entry 1.070 |

### Job 21: CH-closed

null control T(ctl)/T(ctl-null) 1.000 [0.999, 1.002], spread 0.002, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.436 [0.434, 0.437], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.643 |
| D1 vs ctl | Memory (end of job) | loss | end 1.275, after entry 0.817 |
| D2 vs ctl | Time | loss | 0.458 [0.456, 0.460], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.122 |
| D2 vs ctl | Memory (end of job) | win | end 0.774, after entry 0.811 |
| D1 vs D2 | Time | tie | 0.952 [0.947, 0.956], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.573 |
| D1 vs D2 | Memory (end of job) | loss | end 1.646, after entry 1.007 |

### Job 22: CH-all

null control T(ctl)/T(ctl-null) 1.000 [0.998, 1.004], spread 0.004, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.907 [0.905, 0.910], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.658 |
| D1 vs ctl | Memory (end of job) | win | end 0.752, after entry 0.751 |
| D2 vs ctl | Time | loss | 0.605 [0.601, 0.609], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.212 |
| D2 vs ctl | Memory (end of job) | win | end 0.751, after entry 0.751 |
| D1 vs D2 | Time | win | 1.500 [1.492, 1.508], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.543 |
| D1 vs D2 | Memory (end of job) | tie | end 1.001, after entry 1.000 |
| ceil vs ctl | Time | tie | 0.950 [0.948, 0.953], h = 0.050 |
| ceil vs ctl | Gcd work | win | 0.250 |
| ceil vs ctl | Memory (end of job) | win | end 0.501, after entry 0.500 |

### Job 23: CH-open

null control T(ctl)/T(ctl-null) 1.003 [1.000, 1.005], spread 0.005, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.472 [0.469, 0.474], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.980 |
| D1 vs ctl | Memory (end of job) | loss | end 2.467, after entry 0.779 |
| D2 vs ctl | Time | loss | 0.413 [0.411, 0.414], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.245 |
| D2 vs ctl | Memory (end of job) | loss | end 2.414, after entry 0.776 |
| D1 vs D2 | Time | win | 1.143 [1.137, 1.148], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.788 |
| D1 vs D2 | Memory (end of job) | tie | end 1.022, after entry 1.003 |

### Job 24: CH-muldiv

null control T(ctl)/T(ctl-null) 0.998 [0.994, 1.000], spread 0.006, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.529 [0.527, 0.531], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.992 |
| D1 vs ctl | Memory (end of job) | loss | end 2.261, after entry 0.779 |
| D2 vs ctl | Time | loss | 0.406 [0.404, 0.407], h = 0.050 |
| D2 vs ctl | Gcd work | loss | 1.400 |
| D2 vs ctl | Memory (end of job) | loss | end 2.329, after entry 0.776 |
| D1 vs D2 | Time | win | 1.303 [1.299, 1.307], h = 0.050 |
| D1 vs D2 | Gcd work | win | 0.709 |
| D1 vs D2 | Memory (end of job) | tie | end 0.971, after entry 1.003 |

### Job 25: CH-none

null control T(ctl)/T(ctl-null) 1.001 [0.997, 1.003], spread 0.003, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.544 [0.542, 0.545], h = 0.050 |
| D1 vs ctl | Gcd work | tie | 0.946 |
| D1 vs ctl | Memory (end of job) | loss | end 2.156, after entry 0.779 |
| D2 vs ctl | Time | loss | 0.601 [0.598, 0.602], h = 0.050 |
| D2 vs ctl | Gcd work | tie | 1.027 |
| D2 vs ctl | Memory (end of job) | loss | end 1.760, after entry 0.776 |
| D1 vs D2 | Time | loss | 0.905 [0.903, 0.907], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.921 |
| D1 vs D2 | Memory (end of job) | loss | end 1.225, after entry 1.003 |

### Job 26: W32-P50-mixed

null control T(ctl)/T(ctl-null) 1.000 [0.993, 1.005], spread 0.007, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.644 [0.639, 0.647], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.699 |
| D1 vs ctl | Memory (end of job) | loss | end 2.221, after entry 1.005 |
| D2 vs ctl | Time | loss | 0.599 [0.596, 0.602], h = 0.050 |
| D2 vs ctl | Gcd work | win | 0.760 |
| D2 vs ctl | Memory (end of job) | loss | end 2.194, after entry 1.005 |
| D1 vs D2 | Time | win | 1.076 [1.069, 1.082], h = 0.050 |
| D1 vs D2 | Gcd work | tie | 0.920 |
| D1 vs D2 | Memory (end of job) | tie | end 1.012, after entry 1.001 |

### Job 27: W32-ALL-mixed

null control T(ctl)/T(ctl-null) 0.997 [0.995, 1.001], spread 0.005, h = 0.050

| comparison | measure | verdict | magnitude |
| --- | --- | --- | --- |
| D1 vs ctl | Time | loss | 0.853 [0.849, 0.856], h = 0.050 |
| D1 vs ctl | Gcd work | win | 0.849 |
| D1 vs ctl | Memory (end of job) | loss | end 1.276, after entry 1.001 |
| D2 vs ctl | Time | loss | 0.899 [0.898, 0.904], h = 0.050 |
| D2 vs ctl | Gcd work | win | 0.765 |
| D2 vs ctl | Memory (end of job) | loss | end 1.232, after entry 1.001 |
| D1 vs D2 | Time | tie | 0.948 [0.940, 0.951], h = 0.050 |
| D1 vs D2 | Gcd work | loss | 1.110 |
| D1 vs D2 | Memory (end of job) | tie | end 1.036, after entry 1.000 |

## Also reported (item 12)

### Ceiling gap

T(ceil)/T(X) with its interval; B(X)/B(ceil) (deterministic).

| job | X | T(ceil)/T(X) | B(X)/B(ceil), end | after entry |
| --- | --- | --- | --- | --- |
| 1 | ctl | 0.951 [0.948, 0.954] | 1.999 | 1.999 |
| 1 | D1 | 0.895 [0.893, 0.897] | 1.502 | 1.501 |
| 1 | D2 | 0.869 [0.866, 0.872] | 1.501 | 1.501 |
| 22 | ctl | 1.053 [1.050, 1.055] | 1.995 | 1.999 |
| 22 | D1 | 0.955 [0.954, 0.957] | 1.500 | 1.501 |
| 22 | D2 | 0.637 [0.634, 0.640] | 1.499 | 1.501 |

### Operation time alone (entry excluded), by item 8 with the job's h

| job | comparison | verdict | magnitude |
| --- | --- | --- | --- |
| 1 | D1 vs ctl | loss | 0.941 [0.939, 0.943], h = 0.050 |
| 1 | D2 vs ctl | loss | 0.908 [0.905, 0.912], h = 0.050 |
| 1 | D1 vs D2 | tie | 1.036 [1.033, 1.040], h = 0.050 |
| 2 | D1 vs ctl | loss | 0.732 [0.730, 0.735], h = 0.050 |
| 2 | D2 vs ctl | loss | 0.845 [0.843, 0.848], h = 0.050 |
| 2 | D1 vs D2 | loss | 0.866 [0.862, 0.870], h = 0.050 |
| 3 | D1 vs ctl | loss | 0.808 [0.802, 0.813], h = 0.050 |
| 3 | D2 vs ctl | loss | 0.776 [0.770, 0.780], h = 0.050 |
| 3 | D1 vs D2 | tie | 1.041 [1.037, 1.047], h = 0.050 |
| 4 | D1 vs ctl | loss | 0.333 [0.330, 0.334], h = 0.050 |
| 4 | D2 vs ctl | loss | 0.233 [0.232, 0.234], h = 0.050 |
| 4 | D1 vs D2 | win | 1.430 [1.415, 1.439], h = 0.050 |
| 5 | D1 vs ctl | loss | 0.423 [0.421, 0.426], h = 0.050 |
| 5 | D2 vs ctl | loss | 0.249 [0.248, 0.250], h = 0.050 |
| 5 | D1 vs D2 | win | 1.701 [1.693, 1.713], h = 0.050 |
| 6 | D1 vs ctl | loss | 0.379 [0.375, 0.381], h = 0.050 |
| 6 | D2 vs ctl | loss | 0.237 [0.236, 0.238], h = 0.050 |
| 6 | D1 vs D2 | win | 1.597 [1.578, 1.609], h = 0.050 |
| 7 | D1 vs ctl | loss | 0.364 [0.363, 0.367], h = 0.050 |
| 7 | D2 vs ctl | loss | 0.247 [0.243, 0.248], h = 0.050 |
| 7 | D1 vs D2 | win | 1.476 [1.468, 1.501], h = 0.050 |
| 8 | D1 vs ctl | loss | 0.478 [0.472, 0.480], h = 0.050 |
| 8 | D2 vs ctl | loss | 0.313 [0.311, 0.314], h = 0.050 |
| 8 | D1 vs D2 | win | 1.527 [1.510, 1.535], h = 0.050 |
| 9 | D1 vs ctl | loss | 0.418 [0.415, 0.423], h = 0.050 |
| 9 | D2 vs ctl | loss | 0.267 [0.266, 0.269], h = 0.050 |
| 9 | D1 vs D2 | win | 1.562 [1.553, 1.580], h = 0.050 |
| 10 | D1 vs ctl | loss | 0.375 [0.372, 0.377], h = 0.050 |
| 10 | D2 vs ctl | loss | 0.300 [0.299, 0.301], h = 0.050 |
| 10 | D1 vs D2 | win | 1.249 [1.242, 1.257], h = 0.050 |
| 11 | D1 vs ctl | loss | 0.542 [0.541, 0.545], h = 0.050 |
| 11 | D2 vs ctl | loss | 0.462 [0.460, 0.463], h = 0.050 |
| 11 | D1 vs D2 | win | 1.174 [1.171, 1.179], h = 0.050 |
| 12 | D1 vs ctl | loss | 0.476 [0.472, 0.486], h = 0.050 |
| 12 | D2 vs ctl | loss | 0.382 [0.378, 0.388], h = 0.050 |
| 12 | D1 vs D2 | win | 1.246 [1.237, 1.266], h = 0.050 |
| 13 | D1 vs ctl | loss | 0.419 [0.415, 0.424], h = 0.050 |
| 13 | D2 vs ctl | loss | 0.634 [0.622, 0.638], h = 0.050 |
| 13 | D1 vs D2 | loss | 0.661 [0.654, 0.674], h = 0.050 |
| 14 | D1 vs ctl | loss | 0.617 [0.614, 0.619], h = 0.050 |
| 14 | D2 vs ctl | loss | 0.769 [0.761, 0.771], h = 0.050 |
| 14 | D1 vs D2 | loss | 0.802 [0.798, 0.811], h = 0.050 |
| 15 | D1 vs ctl | loss | 0.534 [0.530, 0.540], h = 0.050 |
| 15 | D2 vs ctl | loss | 0.718 [0.713, 0.726], h = 0.050 |
| 15 | D1 vs D2 | loss | 0.743 [0.736, 0.752], h = 0.050 |
| 16 | D1 vs ctl | loss | 0.355 [0.355, 0.359], h = 0.050 |
| 16 | D2 vs ctl | loss | 0.275 [0.274, 0.277], h = 0.050 |
| 16 | D1 vs D2 | win | 1.293 [1.286, 1.305], h = 0.050 |
| 17 | D1 vs ctl | loss | 0.497 [0.495, 0.499], h = 0.050 |
| 17 | D2 vs ctl | loss | 0.377 [0.376, 0.378], h = 0.050 |
| 17 | D1 vs D2 | win | 1.319 [1.314, 1.326], h = 0.050 |
| 18 | D1 vs ctl | loss | 0.439 [0.436, 0.441], h = 0.050 |
| 18 | D2 vs ctl | loss | 0.320 [0.318, 0.320], h = 0.050 |
| 18 | D1 vs D2 | win | 1.373 [1.365, 1.383], h = 0.050 |
| 19 | D1 vs ctl | loss | 0.416 [0.413, 0.418], h = 0.050 |
| 19 | D2 vs ctl | loss | 0.291 [0.288, 0.293], h = 0.050 |
| 19 | D1 vs D2 | win | 1.430 [1.422, 1.437], h = 0.050 |
| 20 | D1 vs ctl | loss | 0.417 [0.414, 0.419], h = 0.050 |
| 20 | D2 vs ctl | loss | 0.293 [0.290, 0.294], h = 0.050 |
| 20 | D1 vs D2 | win | 1.425 [1.417, 1.438], h = 0.050 |
| 21 | D1 vs ctl | loss | 0.403 [0.402, 0.405], h = 0.050 |
| 21 | D2 vs ctl | loss | 0.425 [0.423, 0.426], h = 0.050 |
| 21 | D1 vs D2 | tie | 0.948 [0.945, 0.953], h = 0.050 |
| 22 | D1 vs ctl | loss | 0.898 [0.896, 0.901], h = 0.050 |
| 22 | D2 vs ctl | loss | 0.556 [0.554, 0.560], h = 0.050 |
| 22 | D1 vs D2 | win | 1.614 [1.607, 1.622], h = 0.050 |
| 23 | D1 vs ctl | loss | 0.385 [0.383, 0.388], h = 0.050 |
| 23 | D2 vs ctl | loss | 0.326 [0.324, 0.328], h = 0.050 |
| 23 | D1 vs D2 | win | 1.181 [1.175, 1.191], h = 0.050 |
| 24 | D1 vs ctl | loss | 0.445 [0.443, 0.448], h = 0.050 |
| 24 | D2 vs ctl | loss | 0.319 [0.318, 0.321], h = 0.050 |
| 24 | D1 vs D2 | win | 1.392 [1.387, 1.399], h = 0.050 |
| 25 | D1 vs ctl | loss | 0.442 [0.441, 0.443], h = 0.050 |
| 25 | D2 vs ctl | loss | 0.505 [0.503, 0.507], h = 0.050 |
| 25 | D1 vs D2 | loss | 0.874 [0.871, 0.879], h = 0.050 |
| 26 | D1 vs ctl | loss | 0.608 [0.605, 0.613], h = 0.050 |
| 26 | D2 vs ctl | loss | 0.556 [0.553, 0.560], h = 0.050 |
| 26 | D1 vs D2 | win | 1.095 [1.089, 1.099], h = 0.050 |
| 27 | D1 vs ctl | loss | 0.843 [0.839, 0.848], h = 0.050 |
| 27 | D2 vs ctl | loss | 0.907 [0.905, 0.911], h = 0.050 |
| 27 | D1 vs D2 | loss | 0.929 [0.923, 0.934], h = 0.050 |

### Arrival (job 19 arriving, job 20 its early twin)

Verdicts of items 8-9 on each are in the per-job sections. Per row: T(19)/T(20) with the paired bootstrap interval over the shared rounds, G(19)/G(20), B(19)/B(20) at the end and after entry.

| row | T(19)/T(20) | G(19)/G(20) | B(19)/B(20) end | after entry |
| --- | --- | --- | --- | --- |
| ctl | 1.002 [0.999, 1.003] | 1.000 | 1.000 | 1.000 |
| ctl-null | 0.999 [0.998, 1.004] | 1.000 | 1.000 | 1.000 |
| D1 | 0.998 [0.993, 1.003] | 1.000 | 1.000 | 1.165 |
| D2 | 1.001 [0.992, 1.008] | 1.000 | 1.000 | 1.111 |

### Width 32

Verdicts of items 8-9 are in the per-job sections. Ordering of ctl, D1 and D2 on each measure (smallest first: time T, gcd work G, memory B at the end), against the width-64 matrix job of the same sharing level and mix.

| W32 job | W64 job | measure | W32 order | W64 order | match |
| --- | --- | --- | --- | --- | --- |
| 26 | 9 | time | ctl < D1 < D2 | ctl < D1 < D2 | yes |
| 26 | 9 | gcd | D1 < D2 < ctl | D1 < ctl < D2 | no |
| 26 | 9 | mem | ctl < D2 < D1 | ctl < D1 < D2 | no |
| 27 | 3 | time | ctl < D2 < D1 | ctl < D1 < D2 | no |
| 27 | 3 | gcd | D2 < D1 < ctl | D1 < D2 < ctl | no |
| 27 | 3 | mem | ctl < D2 < D1 | ctl < D2 < D1 | yes |

### Entry against operations

Per design and job: share of total time in entry (median entry / median total), share of gcd work in entry, and within operations the share of gcd work in `store` (the harness cannot time inside a call, so `store`'s share is given for gcd work only).

| job | row | entry share of time | entry share of gcd work | store share of operations' gcd work |
| --- | --- | --- | --- | --- |
| 1 | ctl | 0.292 | 0.320 | 0.000 |
| 1 | D1 | 0.288 | 0.330 | 0.000 |
| 1 | D2 | 0.284 | 0.321 | 0.000 |
| 2 | ctl | 0.231 | 0.229 | 0.000 |
| 2 | D1 | 0.191 | 0.278 | 0.000 |
| 2 | D2 | 0.215 | 0.333 | 0.000 |
| 3 | ctl | 0.252 | 0.276 | 0.000 |
| 3 | D1 | 0.224 | 0.329 | 0.000 |
| 3 | D2 | 0.219 | 0.315 | 0.000 |
| 4 | ctl | 0.168 | 0.143 | 0.000 |
| 4 | D1 | 0.082 | 0.145 | 0.000 |
| 4 | D2 | 0.057 | 0.077 | 0.000 |
| 5 | ctl | 0.164 | 0.151 | 0.000 |
| 5 | D1 | 0.098 | 0.152 | 0.000 |
| 5 | D2 | 0.059 | 0.068 | 0.000 |
| 6 | ctl | 0.164 | 0.149 | 0.000 |
| 6 | D1 | 0.090 | 0.151 | 0.000 |
| 6 | D2 | 0.057 | 0.073 | 0.000 |
| 7 | ctl | 0.185 | 0.164 | 0.000 |
| 7 | D1 | 0.098 | 0.166 | 0.000 |
| 7 | D2 | 0.067 | 0.078 | 0.000 |
| 8 | ctl | 0.162 | 0.150 | 0.000 |
| 8 | D1 | 0.108 | 0.150 | 0.000 |
| 8 | D2 | 0.072 | 0.076 | 0.000 |
| 9 | ctl | 0.169 | 0.158 | 0.000 |
| 9 | D1 | 0.101 | 0.159 | 0.000 |
| 9 | D2 | 0.065 | 0.076 | 0.000 |
| 10 | ctl | 0.226 | 0.217 | 0.000 |
| 10 | D1 | 0.127 | 0.218 | 0.000 |
| 10 | D2 | 0.102 | 0.112 | 0.000 |
| 11 | ctl | 0.159 | 0.148 | 0.000 |
| 11 | D1 | 0.120 | 0.148 | 0.000 |
| 11 | D2 | 0.101 | 0.103 | 0.000 |
| 12 | ctl | 0.183 | 0.175 | 0.000 |
| 12 | D1 | 0.123 | 0.176 | 0.000 |
| 12 | D2 | 0.098 | 0.107 | 0.000 |
| 13 | ctl | 0.249 | 0.251 | 0.000 |
| 13 | D1 | 0.155 | 0.252 | 0.000 |
| 13 | D2 | 0.210 | 0.249 | 0.000 |
| 14 | ctl | 0.159 | 0.146 | 0.000 |
| 14 | D1 | 0.131 | 0.146 | 0.000 |
| 14 | D2 | 0.154 | 0.146 | 0.000 |
| 15 | ctl | 0.189 | 0.184 | 0.000 |
| 15 | D1 | 0.140 | 0.184 | 0.000 |
| 15 | D2 | 0.175 | 0.184 | 0.000 |
| 16 | ctl | 0.207 | 0.190 | 0.000 |
| 16 | D1 | 0.109 | 0.193 | 0.000 |
| 16 | D2 | 0.084 | 0.107 | 0.000 |
| 17 | ctl | 0.161 | 0.149 | 0.000 |
| 17 | D1 | 0.112 | 0.149 | 0.000 |
| 17 | D2 | 0.085 | 0.092 | 0.000 |
| 18 | ctl | 0.176 | 0.168 | 0.000 |
| 18 | D1 | 0.111 | 0.169 | 0.000 |
| 18 | D2 | 0.081 | 0.097 | 0.000 |
| 19 | ctl | 0.170 | 0.156 | 0.000 |
| 19 | D1 | 0.126 | 0.156 | 0.000 |
| 19 | D2 | 0.078 | 0.077 | 0.000 |
| 20 | ctl | 0.170 | 0.156 | 0.000 |
| 20 | D1 | 0.133 | 0.156 | 0.000 |
| 20 | D2 | 0.085 | 0.077 | 0.000 |
| 21 | ctl | 0.118 | 0.085 | 0.000 |
| 21 | D1 | 0.063 | 0.132 | 0.000 |
| 21 | D2 | 0.065 | 0.076 | 0.000 |
| 22 | ctl | 0.179 | 0.153 | 0.000 |
| 22 | D1 | 0.170 | 0.233 | 0.000 |
| 22 | D2 | 0.114 | 0.127 | 0.000 |
| 23 | ctl | 0.321 | 0.290 | 0.000 |
| 23 | D1 | 0.184 | 0.296 | 0.000 |
| 23 | D2 | 0.159 | 0.233 | 0.000 |
| 24 | ctl | 0.318 | 0.283 | 0.000 |
| 24 | D1 | 0.205 | 0.285 | 0.000 |
| 24 | D2 | 0.156 | 0.202 | 0.000 |
| 25 | ctl | 0.374 | 0.372 | 0.000 |
| 25 | D1 | 0.248 | 0.393 | 0.000 |
| 25 | D2 | 0.271 | 0.362 | 0.000 |
| 26 | ctl | 0.173 | 0.191 | 0.000 |
| 26 | D1 | 0.137 | 0.273 | 0.000 |
| 26 | D2 | 0.124 | 0.251 | 0.000 |
| 27 | ctl | 0.221 | 0.290 | 0.000 |
| 27 | D1 | 0.214 | 0.341 | 0.000 |
| 27 | D2 | 0.225 | 0.379 | 0.000 |

### Results as operands

Given per job in the Workloads table above (G3 realized).

### Gcd work against time

Jobs where a design wins on gcd work against the control but not on time: candidates for wider-number tests, where gcds cost more.

- job 2 (ALL-muldiv), D1: gcd work 0.823, time loss (0.773 [0.771, 0.775], h = 0.050)
- job 2 (ALL-muldiv), D2: gcd work 0.685, time loss (0.863 [0.860, 0.864], h = 0.050)
- job 3 (ALL-mixed), D1: gcd work 0.839, time loss (0.838 [0.833, 0.841], h = 0.050)
- job 3 (ALL-mixed), D2: gcd work 0.876, time loss (0.813 [0.808, 0.815], h = 0.050)
- job 21 (CH-closed), D1: gcd work 0.643, time loss (0.436 [0.434, 0.437], h = 0.050)
- job 22 (CH-all), D1: gcd work 0.658, time loss (0.907 [0.905, 0.910], h = 0.050)
- job 26 (W32-P50-mixed), D1: gcd work 0.699, time loss (0.644 [0.639, 0.647], h = 0.050)
- job 26 (W32-P50-mixed), D2: gcd work 0.760, time loss (0.599 [0.596, 0.602], h = 0.050)
- job 27 (W32-ALL-mixed), D1: gcd work 0.849, time loss (0.853 [0.849, 0.856], h = 0.050)
- job 27 (W32-ALL-mixed), D2: gcd work 0.765, time loss (0.899 [0.898, 0.904], h = 0.050)

## Per job: times, memory and counters

Times are medians over the rounds, per run of the job, in ms (entry, operations, read-out, total). Memory: used and allocated bytes per rational after entry and at the end, and the per-structure breakdown (bytes). Every counter of spec §12.1, per phase and part, is in counters.csv.

### Job 1: ALL-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.411 | 0.953 | 0.044 | 1.408 | 16.0 | 16.0 | 17.5 | 624761 | values 480000/524288 |
| ctl-null | 0.409 | 0.956 | 0.044 | 1.408 | 16.0 | 16.0 | 17.5 | 624761 | values 480000/524288 |
| D1 | 0.431 | 1.013 | 0.051 | 1.496 | 12.0 | 12.0 | 13.1 | 605621 | numerators 360000/393216; denominators 48/128; lookup 64/64; divisors 60/192; pair set 128/128; incidence rows 264/460 |
| D2 | 0.438 | 1.050 | 0.053 | 1.542 | 12.0 | 12.0 | 13.1 | 623984 | numerators 360000/393216; denominators 48/128; lookup 64/64; base 24/128; exponent rows 176/416; remainders 48/128 |
| ceil | 0.006 | 0.057 | 1.277 | 1.339 | 8.0 | 8.0 | 8.0 | 100 | stored vectors 240072/240384 |

### Job 2: ALL-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.406 | 1.306 | 0.045 | 1.757 | 16.0 | 16.0 | 17.5 | 853319 | values 480000/524288 |
| ctl-null | 0.406 | 1.306 | 0.045 | 1.755 | 16.0 | 16.0 | 17.5 | 853319 | values 480000/524288 |
| D1 | 0.435 | 1.785 | 0.055 | 2.273 | 12.0 | 30.6 | 36.1 | 702025 | numerators 360000/393216; denominators 87696/131072; lookup 131072/131072; divisors 24/192; pair set 32768/32768; incidence rows 306960/393712 |
| D2 | 0.439 | 1.546 | 0.054 | 2.036 | 12.0 | 29.1 | 36.0 | 584870 | numerators 360000/393216; denominators 87696/131072; lookup 131072/131072; base 8/128; exponent rows 207464/294264; remainders 87696/131072 |

### Job 3: ALL-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.404 | 1.155 | 0.045 | 1.603 | 16.0 | 16.0 | 17.5 | 705825 | values 480000/524288 |
| ctl-null | 0.407 | 1.151 | 0.044 | 1.604 | 16.0 | 16.0 | 17.5 | 705825 | values 480000/524288 |
| D1 | 0.429 | 1.430 | 0.053 | 1.913 | 12.0 | 22.2 | 25.2 | 592033 | numerators 360000/393216; denominators 45944/65536; lookup 65536/65536; divisors 60/192; pair set 32768/32768; incidence rows 160900/200120 |
| D2 | 0.432 | 1.489 | 0.052 | 1.970 | 12.0 | 21.7 | 25.5 | 618287 | numerators 360000/393216; denominators 45944/65536; lookup 65536/65536; base 24/128; exponent rows 134256/173624; remainders 45944/65536 |

### Job 4: P75-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.408 | 1.981 | 0.046 | 2.435 | 16.0 | 16.0 | 17.5 | 1386149 | values 480000/524288 |
| ctl-null | 0.409 | 1.983 | 0.045 | 2.435 | 16.0 | 16.0 | 17.5 | 1386149 | values 480000/524288 |
| D1 | 0.536 | 5.954 | 0.058 | 6.551 | 16.4 | 70.0 | 82.7 | 1363570 | numerators 360000/393216; denominators 166808/262144; lookup 262144/262144; divisors 10176/12288; pair set 524288/524288; incidence rows 777808/1026776 |
| D2 | 0.515 | 8.516 | 0.058 | 9.094 | 16.0 | 76.3 | 91.3 | 2565806 | numerators 360000/393216; denominators 166808/262144; lookup 262144/262144; base 544/1024; exponent rows 1333120/1558576; remainders 166808/262144 |

### Job 5: P75-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.410 | 2.045 | 0.046 | 2.501 | 16.0 | 16.0 | 17.5 | 1310536 | values 480000/524288 |
| ctl-null | 0.410 | 2.044 | 0.046 | 2.500 | 16.0 | 16.0 | 17.5 | 1310536 | values 480000/524288 |
| D1 | 0.534 | 4.837 | 0.060 | 5.429 | 16.4 | 58.4 | 70.1 | 1301949 | numerators 360000/393216; denominators 167880/262144; lookup 262144/262144; divisors 6144/6144; pair set 262144/262144; incidence rows 694932/917336 |
| D2 | 0.516 | 8.225 | 0.061 | 8.805 | 16.0 | 60.8 | 75.4 | 2898471 | numerators 360000/393216; denominators 167880/262144; lookup 262144/262144; base 472/512; exponent rows 864672/1081008; remainders 167880/262144 |

### Job 6: P75-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.411 | 2.054 | 0.046 | 2.509 | 16.0 | 16.0 | 17.5 | 1325233 | values 480000/524288 |
| ctl-null | 0.410 | 2.051 | 0.045 | 2.505 | 16.0 | 16.0 | 17.5 | 1325233 | values 480000/524288 |
| D1 | 0.540 | 5.423 | 0.060 | 6.025 | 16.4 | 59.8 | 71.9 | 1310348 | numerators 360000/393216; denominators 167544/262144; lookup 262144/262144; divisors 8028/12288; pair set 262144/262144; incidence rows 732876/966368 |
| D2 | 0.527 | 8.660 | 0.061 | 9.245 | 16.0 | 68.5 | 83.3 | 2709591 | numerators 360000/393216; denominators 167544/262144; lookup 262144/262144; base 408/512; exponent rows 1098472/1319552; remainders 167544/262144 |

### Job 7: P50-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.409 | 1.759 | 0.044 | 2.212 | 16.0 | 16.0 | 17.5 | 1208471 | values 480000/524288 |
| ctl-null | 0.408 | 1.763 | 0.045 | 2.218 | 16.0 | 16.0 | 17.5 | 1208471 | values 480000/524288 |
| D1 | 0.534 | 4.829 | 0.057 | 5.420 | 16.4 | 67.0 | 78.5 | 1195074 | numerators 360000/393216; denominators 167224/262144; lookup 262144/262144; divisors 3228/6144; pair set 524288/524288; incidence rows 694292/908264 |
| D2 | 0.516 | 7.125 | 0.058 | 7.705 | 16.0 | 65.5 | 80.2 | 2538432 | numerators 360000/393216; denominators 167224/262144; lookup 262144/262144; base 616/1024; exponent rows 1007976/1223992; remainders 167224/262144 |

### Job 8: P50-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.412 | 2.083 | 0.044 | 2.541 | 16.0 | 16.0 | 17.5 | 1322715 | values 480000/524288 |
| ctl-null | 0.411 | 2.083 | 0.046 | 2.541 | 16.0 | 16.0 | 17.5 | 1322715 | values 480000/524288 |
| D1 | 0.536 | 4.362 | 0.057 | 4.955 | 16.4 | 56.8 | 67.9 | 1317301 | numerators 360000/393216; denominators 167824/262144; lookup 262144/262144; divisors 2028/3072; pair set 262144/262144; incidence rows 650276/853736 |
| D2 | 0.520 | 6.660 | 0.056 | 7.240 | 16.0 | 54.4 | 69.0 | 2596412 | numerators 360000/393216; denominators 167824/262144; lookup 262144/262144; base 320/512; exponent rows 673232/891016; remainders 167824/262144 |

### Job 9: P50-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.410 | 1.968 | 0.046 | 2.426 | 16.0 | 16.0 | 17.5 | 1254513 | values 480000/524288 |
| ctl-null | 0.412 | 1.967 | 0.045 | 2.425 | 16.0 | 16.0 | 17.5 | 1254513 | values 480000/524288 |
| D1 | 0.535 | 4.713 | 0.061 | 5.313 | 16.4 | 57.6 | 68.8 | 1244952 | numerators 360000/393216; denominators 167616/262144; lookup 262144/262144; divisors 2640/3072; pair set 262144/262144; incidence rows 672852/881552 |
| D2 | 0.517 | 7.360 | 0.061 | 7.942 | 16.0 | 60.2 | 74.8 | 2603195 | numerators 360000/393216; denominators 167616/262144; lookup 262144/262144; base 448/512; exponent rows 848400/1064776; remainders 167616/262144 |

### Job 10: P25-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.407 | 1.348 | 0.044 | 1.799 | 16.0 | 16.0 | 17.5 | 911184 | values 480000/524288 |
| ctl-null | 0.406 | 1.347 | 0.044 | 1.799 | 16.0 | 16.0 | 17.5 | 911184 | values 480000/524288 |
| D1 | 0.532 | 3.598 | 0.057 | 4.190 | 16.4 | 64.5 | 75.3 | 906862 | numerators 360000/393216; denominators 166904/262144; lookup 262144/262144; divisors 1380/1536; pair set 524288/524288; incidence rows 620320/815576 |
| D2 | 0.515 | 4.494 | 0.057 | 5.069 | 16.0 | 53.2 | 68.0 | 1756455 | numerators 360000/393216; denominators 166904/262144; lookup 262144/262144; base 664/1024; exponent rows 639272/860480; remainders 166904/262144 |

### Job 11: P25-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.409 | 2.116 | 0.044 | 2.567 | 16.0 | 16.0 | 17.5 | 1341474 | values 480000/524288 |
| ctl-null | 0.407 | 2.112 | 0.045 | 2.566 | 16.0 | 16.0 | 17.5 | 1341474 | values 480000/524288 |
| D1 | 0.541 | 3.901 | 0.057 | 4.497 | 16.4 | 55.4 | 66.0 | 1340120 | numerators 360000/393216; denominators 167824/262144; lookup 262144/262144; divisors 780/1536; pair set 262144/262144; incidence rows 609092/799952 |
| D2 | 0.523 | 4.581 | 0.058 | 5.162 | 16.0 | 47.5 | 62.1 | 1930522 | numerators 360000/393216; denominators 167824/262144; lookup 262144/262144; base 408/512; exponent rows 467264/683648; remainders 167824/262144 |

### Job 12: P25-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.414 | 1.804 | 0.046 | 2.261 | 16.0 | 16.0 | 17.5 | 1127800 | values 480000/524288 |
| ctl-null | 0.411 | 1.804 | 0.045 | 2.260 | 16.0 | 16.0 | 17.5 | 1127800 | values 480000/524288 |
| D1 | 0.538 | 3.790 | 0.061 | 4.388 | 16.4 | 55.6 | 66.3 | 1125112 | numerators 360000/393216; denominators 167504/262144; lookup 262144/262144; divisors 1044/1536; pair set 262144/262144; incidence rows 614464/806588 |
| D2 | 0.518 | 4.724 | 0.059 | 5.303 | 16.0 | 50.1 | 64.9 | 1845174 | numerators 360000/393216; denominators 167504/262144; lookup 262144/262144; base 512/512; exponent rows 545656/765944; remainders 167504/262144 |

### Job 13: NONE-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.411 | 1.195 | 0.044 | 1.650 | 16.0 | 16.0 | 17.5 | 797484 | values 480000/524288 |
| ctl-null | 0.413 | 1.193 | 0.044 | 1.651 | 16.0 | 16.0 | 17.5 | 797484 | values 480000/524288 |
| D1 | 0.532 | 2.848 | 0.057 | 3.433 | 16.4 | 63.3 | 73.9 | 794773 | numerators 360000/393216; denominators 166856/262144; lookup 262144/262144; divisors 288/384; pair set 524288/524288; incidence rows 584548/775124 |
| D2 | 0.514 | 1.883 | 0.056 | 2.453 | 16.0 | 43.1 | 57.0 | 804380 | numerators 360000/393216; denominators 166856/262144; lookup 262144/262144; base 184/256; exponent rows 337416/529096; remainders 166856/262144 |

### Job 14: NONE-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.414 | 2.156 | 0.045 | 2.613 | 16.0 | 16.0 | 17.5 | 1365834 | values 480000/524288 |
| ctl-null | 0.414 | 2.154 | 0.045 | 2.613 | 16.0 | 16.0 | 17.5 | 1365834 | values 480000/524288 |
| D1 | 0.535 | 3.494 | 0.058 | 4.083 | 16.4 | 54.7 | 65.2 | 1365239 | numerators 360000/393216; denominators 167848/262144; lookup 262144/262144; divisors 108/192; pair set 262144/262144; incidence rows 587660/776252 |
| D2 | 0.519 | 2.803 | 0.055 | 3.376 | 16.0 | 43.1 | 56.8 | 1367011 | numerators 360000/393216; denominators 167848/262144; lookup 262144/262144; base 64/128; exponent rows 336504/525480; remainders 167848/262144 |

### Job 15: NONE-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.416 | 1.738 | 0.045 | 2.197 | 16.0 | 16.0 | 17.5 | 1086083 | values 480000/524288 |
| ctl-null | 0.416 | 1.736 | 0.045 | 2.197 | 16.0 | 16.0 | 17.5 | 1086083 | values 480000/524288 |
| D1 | 0.541 | 3.256 | 0.058 | 3.854 | 16.4 | 54.6 | 65.2 | 1084788 | numerators 360000/393216; denominators 167424/262144; lookup 262144/262144; divisors 108/192; pair set 262144/262144; incidence rows 586176/775616 |
| D2 | 0.527 | 2.420 | 0.058 | 3.003 | 16.0 | 43.1 | 56.9 | 1087307 | numerators 360000/393216; denominators 167424/262144; lookup 262144/262144; base 64/128; exponent rows 336104/525928; remainders 167424/262144 |

### Job 16: LG-add

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.409 | 1.523 | 0.045 | 1.977 | 16.0 | 16.0 | 17.5 | 1041398 | values 480000/524288 |
| ctl-null | 0.409 | 1.525 | 0.045 | 1.978 | 16.0 | 16.0 | 17.5 | 1041398 | values 480000/524288 |
| D1 | 0.533 | 4.283 | 0.060 | 4.878 | 16.4 | 65.6 | 76.9 | 1028303 | numerators 360000/393216; denominators 166176/262144; lookup 262144/262144; divisors 2760/3072; pair set 524288/524288; incidence rows 653004/861380 |
| D2 | 0.514 | 5.538 | 0.060 | 6.113 | 16.0 | 60.2 | 75.0 | 1853158 | numerators 360000/393216; denominators 166176/262144; lookup 262144/262144; base 432/512; exponent rows 850784/1069080; remainders 166176/262144 |

### Job 17: LG-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.410 | 2.096 | 0.047 | 2.554 | 16.0 | 16.0 | 17.5 | 1333521 | values 480000/524288 |
| ctl-null | 0.413 | 2.098 | 0.046 | 2.555 | 16.0 | 16.0 | 17.5 | 1333521 | values 480000/524288 |
| D1 | 0.539 | 4.221 | 0.061 | 4.821 | 16.4 | 56.0 | 66.8 | 1328746 | numerators 360000/393216; denominators 167696/262144; lookup 262144/262144; divisors 1800/3072; pair set 262144/262144; incidence rows 625288/821672 |
| D2 | 0.522 | 5.566 | 0.062 | 6.150 | 16.0 | 51.4 | 66.0 | 2155222 | numerators 360000/393216; denominators 167696/262144; lookup 262144/262144; base 368/512; exponent rows 584264/798984; remainders 167696/262144 |

### Job 18: LG-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.412 | 1.877 | 0.047 | 2.338 | 16.0 | 16.0 | 17.5 | 1182189 | values 480000/524288 |
| ctl-null | 0.415 | 1.878 | 0.047 | 2.340 | 16.0 | 16.0 | 17.5 | 1182189 | values 480000/524288 |
| D1 | 0.542 | 4.278 | 0.063 | 4.882 | 16.4 | 56.5 | 67.5 | 1173046 | numerators 360000/393216; denominators 167272/262144; lookup 262144/262144; divisors 2352/3072; pair set 262144/262144; incidence rows 641396/842084 |
| D2 | 0.524 | 5.872 | 0.063 | 6.457 | 16.0 | 56.0 | 70.7 | 2053927 | numerators 360000/393216; denominators 167272/262144; lookup 262144/262144; base 392/512; exponent rows 723864/939864; remainders 167272/262144 |

### Job 19: ARR-arriving

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.414 | 1.977 | 0.045 | 2.437 | 16.0 | 16.0 | 17.5 | 1272706 | values 480000/524288 |
| ctl-null | 0.413 | 1.975 | 0.045 | 2.433 | 16.0 | 16.0 | 17.5 | 1272706 | values 480000/524288 |
| D1 | 0.693 | 4.753 | 0.056 | 5.499 | 71.2 | 70.7 | 75.0 | 1272069 | numerators 360000/393216; denominators 239952/262144; lookup 262144/262144; divisors 2748/3072; pair set 262144/262144; incidence rows 993300/1068392 |
| D2 | 0.580 | 6.795 | 0.058 | 7.434 | 63.5 | 63.3 | 72.4 | 2585884 | numerators 360000/393216; denominators 239952/262144; lookup 262144/262144; base 472/512; exponent rows 797416/992416; remainders 239952/262144 |

### Job 20: ARR-early-twin

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.412 | 1.973 | 0.045 | 2.432 | 16.0 | 16.0 | 17.5 | 1272706 | values 480000/524288 |
| ctl-null | 0.412 | 1.977 | 0.045 | 2.435 | 16.0 | 16.0 | 17.5 | 1272706 | values 480000/524288 |
| D1 | 0.732 | 4.726 | 0.055 | 5.512 | 61.1 | 70.7 | 75.0 | 1272069 | numerators 360000/393216; denominators 239952/262144; lookup 262144/262144; divisors 2748/3072; pair set 262144/262144; incidence rows 993300/1068392 |
| D2 | 0.631 | 6.733 | 0.058 | 7.425 | 57.1 | 63.3 | 72.4 | 2585884 | numerators 360000/393216; denominators 239952/262144; lookup 262144/262144; base 472/512; exponent rows 797416/992416; remainders 239952/262144 |

### Job 21: CH-closed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.287 | 2.076 | 0.065 | 2.427 | 16.0 | 16.0 | 25.0 | 1210770 | values 672000/1048576 |
| ctl-null | 0.287 | 2.074 | 0.065 | 2.427 | 16.0 | 16.0 | 25.0 | 1210770 | values 672000/1048576 |
| D1 | 0.349 | 5.148 | 0.072 | 5.573 | 13.1 | 20.4 | 27.8 | 778782 | numerators 504000/786432; denominators 1920/2048; lookup 2048/2048; divisors 2868/3072; pair set 262144/262144; incidence rows 83556/111676 |
| D2 | 0.342 | 4.883 | 0.074 | 5.304 | 13.0 | 12.4 | 19.2 | 1358534 | numerators 504000/786432; denominators 1920/2048; lookup 2048/2048; base 48/128; exponent rows 10496/13216; remainders 1920/2048 |

### Job 22: CH-all

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.459 | 2.041 | 0.067 | 2.567 | 16.0 | 16.0 | 25.0 | 1499376 | values 672000/1048576 |
| ctl-null | 0.458 | 2.041 | 0.067 | 2.566 | 16.0 | 16.0 | 25.0 | 1499376 | values 672000/1048576 |
| D1 | 0.482 | 2.273 | 0.076 | 2.830 | 12.0 | 12.0 | 18.8 | 987189 | numerators 504000/786432; denominators 104/128; lookup 128/128; divisors 144/192; pair set 256/256; incidence rows 628/724 |
| D2 | 0.484 | 3.669 | 0.085 | 4.245 | 12.0 | 12.0 | 18.8 | 1816663 | numerators 504000/786432; denominators 104/128; lookup 128/128; base 32/128; exponent rows 488/664; remainders 104/128 |
| ceil | 0.008 | 0.726 | 1.969 | 2.703 | 8.0 | 8.0 | 8.0 | 374899 | stored vectors 336792/337536 |

### Job 23: CH-open

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.282 | 0.572 | 0.025 | 0.879 | 16.0 | 16.0 | 29.1 | 406547 | values 288000/524288 |
| ctl-null | 0.280 | 0.571 | 0.025 | 0.876 | 16.0 | 16.0 | 29.1 | 406547 | values 288000/524288 |
| D1 | 0.342 | 1.488 | 0.031 | 1.863 | 12.5 | 39.5 | 51.0 | 398523 | numerators 216000/393216; denominators 56544/65536; lookup 65536/65536; divisors 2472/3072; pair set 131072/131072; incidence rows 238836/259976 |
| D2 | 0.338 | 1.757 | 0.032 | 2.128 | 12.4 | 38.6 | 51.1 | 505988 | numerators 216000/393216; denominators 56544/65536; lookup 65536/65536; base 464/512; exponent rows 300048/330152; remainders 56544/65536 |

### Job 24: CH-muldiv

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.280 | 0.574 | 0.025 | 0.880 | 16.0 | 16.0 | 29.1 | 418058 | values 288000/524288 |
| ctl-null | 0.281 | 0.575 | 0.025 | 0.881 | 16.0 | 16.0 | 29.1 | 418058 | values 288000/524288 |
| D1 | 0.342 | 1.291 | 0.031 | 1.663 | 12.5 | 36.2 | 46.7 | 414878 | numerators 216000/393216; denominators 61624/65536; lookup 65536/65536; divisors 2412/3072; pair set 65536/65536; incidence rows 239984/248504 |
| D2 | 0.338 | 1.798 | 0.031 | 2.167 | 12.4 | 37.3 | 48.6 | 585137 | numerators 216000/393216; denominators 61624/65536; lookup 65536/65536; base 520/1024; exponent rows 265392/283512; remainders 61624/65536 |

### Job 25: CH-none

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.286 | 0.453 | 0.025 | 0.764 | 16.0 | 16.0 | 29.1 | 323001 | values 288000/524288 |
| ctl-null | 0.285 | 0.454 | 0.025 | 0.764 | 16.0 | 16.0 | 29.1 | 323001 | values 288000/524288 |
| D1 | 0.349 | 1.025 | 0.030 | 1.406 | 12.5 | 34.5 | 48.2 | 305595 | numerators 216000/393216; denominators 43008/65536; lookup 65536/65536; divisors 1212/1536; pair set 131072/131072; incidence rows 164232/210260 |
| D2 | 0.345 | 0.896 | 0.030 | 1.273 | 12.4 | 28.2 | 43.6 | 331794 | numerators 216000/393216; denominators 43008/65536; lookup 65536/65536; base 800/1024; exponent rows 138656/193440; remainders 43008/65536 |

### Job 26: W32-P50-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.227 | 1.031 | 0.048 | 1.308 | 8.0 | 8.0 | 8.7 | 453759 | values 240000/262144 |
| ctl-null | 0.226 | 1.034 | 0.047 | 1.308 | 8.0 | 8.0 | 8.7 | 453759 | values 240000/262144 |
| D1 | 0.279 | 1.696 | 0.056 | 2.030 | 8.0 | 17.8 | 20.2 | 317268 | numerators 240000/262144; denominators 22576/32768; lookup 65536/65536; divisors 144/256; pair set 65536/65536; incidence rows 139224/180552 |
| D2 | 0.271 | 1.856 | 0.056 | 2.184 | 8.0 | 17.6 | 20.4 | 344917 | numerators 240000/262144; denominators 22576/32768; lookup 65536/65536; base 52/64; exponent rows 175912/218088; remainders 22576/32768 |

### Job 27: W32-ALL-mixed

| row | entry | operations | read-out | total | B used after entry | B used end | B alloc end | gcd work | structures (end: used/alloc) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ctl | 0.224 | 0.743 | 0.047 | 1.015 | 8.0 | 8.0 | 8.7 | 293412 | values 240000/262144 |
| ctl-null | 0.225 | 0.745 | 0.048 | 1.018 | 8.0 | 8.0 | 8.7 | 293412 | values 240000/262144 |
| D1 | 0.255 | 0.882 | 0.055 | 1.190 | 8.0 | 10.2 | 11.3 | 249156 | numerators 240000/262144; denominators 5952/8192; lookup 16384/16384; divisors 16/128; pair set 8192/8192; incidence rows 35728/44688 |
| D2 | 0.253 | 0.820 | 0.054 | 1.129 | 8.0 | 9.9 | 11.0 | 224411 | numerators 240000/262144; denominators 5952/8192; lookup 16384/16384; base 4/64; exponent rows 27296/36304; remainders 5952/8192 |

