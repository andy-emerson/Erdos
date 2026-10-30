# Skeleton baselines: pre-registered measurement rule

Oct 28, 2026 · **Fixed.** Every item of this rule, including every item formerly marked PROPOSED, is approved by the author, Oct 28, 2026; the rule is fixed before any measurement. Approved earlier the same day: the per-measure thresholds (the time band, memory and gcd-work bands, the severe-memory-loss flag) and the 12 interleaved rounds. Revised at the author's direction after the second review, Oct 28, 2026: the rep rule (item 4), the null control's repetition count, counter determinism on a subset (item 2), the use of two cores, the gate schedule (item 3), the trimmed run plan with its pilot and 4-hour stop, and the arrival twin (G4). Revised at the author's direction after the third review, Oct 28, 2026: the LG family is constructed directly, with no redraw (G1); the 4-hour limit is also a hard stop on the whole run; a gate pilot on job 20; Design 2's large-base audits (item 3); timed rounds on one core, one row at a time (item 4); and the calibration of r, stated exactly (item 4). Every item proposed in that revision is now approved, so this file is fixed before any measurement: the harness writes the rule into its results file before the first trial, and refuses a measured run unless this file holds the rule verbatim. A change after that is a revision, dated and explained at the top, and allowed only while nothing has been measured.

**Revision, Oct 29, 2026, approved by the author.** Design 2 was redefined after the pilot because the pilot exposed a design flaw: its eager refinement tested every new denominator against the whole base, about C gcds each ([finding](../../../09-coprime-base/research/eager-d2-finding.md)). Design 2 is now refined on demand ([spec §8](../v0.1.md#8-design-2-the-coprime-base), amended Oct 29, 2026). The pilot jobs were also run on the new designs during development, and their gcds, times and memory are reported in the design note; but no measured run under this rule has taken place: nothing from step 4 onwards (gates, counting runs, calibration, rounds) has run, and the pilot is run again on the current code. In the same revision, Design 1's dense pair table is replaced by a hash set of the pairs that met, with the same meaning ([spec §7](../v0.1.md#7-design-1-common-divisors), amended Oct 29, 2026), because its $D(D-1)/2$ bits made Design 1's memory quadratic in $D$. Also in this revision, Design 1's dense $D \times C$ incidence bit matrix is replaced by sparse rows of (divisor id, residual), with the same facts and results ([spec §7](../v0.1.md#7-design-1-common-divisors), amended Oct 29, 2026), because it was 7–53% of Design 1's bytes per rational in the pilot. The text that described the replaced forms is updated to match: item 4's reason for keeping n, item 6's note on Design 1's memory, the reason the gate pilot runs job 20, and the run plan's non-normative estimate. One procedure is added: before any timed run, the pilot's included, the harness checks that the machine is idle (its load average and the CPU use of every other process, against thresholds it states), and if it is not, it waits and logs this, and after a stated time refuses to time ([harness decision R-p](../test/DECISIONS.md)). No other item of this rule changes.

It states how Design 1 (common divisors) and Design 2 (the coprime base) of [spec v0.1](../v0.1.md) are measured against the control and against each other, at their baselines, and how the results are reported: **per measure only, with no overall winner** (item 11).

**This comparison does not end the debate.** It compares two skeletons, each the plainest correct form of its design, with every improvement left out on purpose. A design that does worse here may have more room to improve: the note lists improvements for each, and they are not equal in size.

## Measurement rule

1. **Rows.** Each job runs these rows:
   - **ctl**: the control;
   - **ctl-null**: a second copy of the control, the same binary run as a separate row;
   - **D1** and **D2**;
   - **ceil**, the shared-denominator ceiling, only on the jobs marked "ceil" in the run plan.

2. **Builds.** All rows are built by one compiler with one set of flags (`-O2`), at width 64 (width 32 for the W32 jobs). The timing build carries no counters or audits. The counting build gives the counts and the memory reports, from one complete run per row and job. Counts are deterministic: on the fixed subset of jobs 3, 9, 14, 18, 19 and 23, every row's counting run is repeated, and the second run MUST give identical counts, or it is a bug, handled by item 5. Every counting run has a limit of **10 minutes**; a run that does not finish within it gives no counts, and its gcd work and memory are reported as **n/a**, with the reason. The test build gives the audits.

3. **Gates before the measured run.** In the test build, every row MUST pass the unit tests and, on every job, the gate of [spec §13.3](../v0.1.md#133-tests): one complete run, with a full audit after each of the first 200 calls, then after every 64th call, and after the last call; entries are calls and are audited on the same schedule; between full audits, an incremental audit of what changed MAY run after every call. A row that fails a gate is fixed and the gate re-run; no measured run starts until every gate passes. Gates are not timed, so they run two at a time, one per core, but never while a timed row runs (item 4).
   - **Large base (Design 2).** Design 2's full audit checks the base pairwise, $C(C-1)/2$ gcds, which would make the gate quadratic in C at every scheduled audit. So once Design 2's base has more than **1,024 pieces**, each scheduled full audit before the last checks pairwise coprimality incrementally only: `audit_changed` MUST then run after every call, testing each piece created or rewritten by that call against every other piece; everything else of the full audit is still checked in full, and the full audit after the last call is complete, the pairwise check included ([spec §13.3](../v0.1.md#133-tests)). So no full audit but the last takes more than $1{,}024 \cdot 1{,}023 / 2 = 523{,}776$ coprimality gcds, and every pair of pieces is still checked: when the later of the two was created or last rewritten. Every other audit is linear in the structures and is not capped.

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
   - the bug is fixed under review ([spec §11](../v0.1.md#11-instructions-to-builders-and-reviewers)), and the fix is **logged** in the change log, with the job, invocation, operation and step where it appeared;
   - the affected design is **re-run in full**: its gates, its counting runs and every round of every job; its earlier results are discarded. A bug in the control re-runs ctl and ctl-null; a fix to the shared integer routines re-runs every row.

   The report lists every such bug beside the per-stratum tables of item 10.

6. **Measures**, per job and row (X is D1, D2 or ceil):
   - **Time.** The ratio **T(ctl) / T(X)**, T being the median of the row's per-round totals; above 1, X is faster. Its 95% interval [lo, hi] is the 2.5th–97.5th percentile of the same ratio over **10,000 bootstrap resamples** of the rounds (rounds drawn with replacement, the same draw for both rows, so the pairing is kept; seed 20261028).
   - **Gcd work.** **G(X) / G(ctl)**, where G is the counting build's sum, over every gcd call of the job (entry and operations), of the bit length of the larger operand.
   - **Memory.** **B(X) / B(ctl)**, where B is the used bytes per rational ([spec §12.2](../v0.1.md#122-memory)). B is reported **twice**: **after entry** (when the last entered value is stored) and **at the end of the job**. The memory verdict and the severe-memory-loss flag of item 8 use the end-of-job figure; the after-entry figure is reported beside it.

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

The note's variant of Design 1, a pair table whose cells name the column of the pair's gcd instead of holding 1 bit, is **deferred**. It is a later variant test, run against the Design 1 baseline after this round, and is **not part of this round**: no row, job or verdict here measures it, and the spec keeps it outside the skeleton ([spec §10](../v0.1.md#10-the-skeletons-boundary)).

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

**Ceiling jobs.** The ceiling runs where every operand shares one denominator and every result of a job shares one: ALL-add (job 1) and CH-all (job 22). On ALL-add, it enters the inputs as one vector; gathers the operands of the two-input operations into two vectors and runs one vector operation, whose result vector holds the results that G3 reuses; then gathers the operands of the result operations, the first from that result vector and the second from the input vector, and runs a second vector operation. Gathering is timed, as operations. On CH-all, it holds the 1,000 chains as one vector; each step gathers the 1,000 $y_k$, negated where the step subtracts, and runs one vector addition. Gathered vectors are working copies and are not counted in its bytes per rational ([spec §12.2](../v0.1.md#122-memory)). It does not run ALL-muldiv (the quotients' common denominator, an lcm of numerators, does not fit), ALL-mixed (no single vector operation), or any other job.

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
