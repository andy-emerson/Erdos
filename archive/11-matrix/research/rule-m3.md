# Design 3 on matrices, round m3: measurement rule

Oct 30, 2026 · **Fixed: approved by the author, Oct 30, 2026,** in full: the rows, the new structures, the pairings, the adapted checks, the pilot and its projection, the grid, and spec v0.3's design. **Revised the same day after the m3 review, before any trial, by the author's decisions:** the grid drops $n = 128$ for every row and set, and sop also drops $\rho = 16$, because the pilot's projection would have stopped the runs (the first draft's carve-out for Pack-primes at $n = 128$ is withdrawn); the estimate now covers steps 1–3 and the unit tests' large trial divisions; the projection's job counts are written out. The harness refuses a measured run unless this file has the approved text (by sha256, pinned in the harness), and while any mark of a proposal awaiting approval remains in it. Nothing is built, run or measured.

This is a new rule for spec [v0.3](../v0.3.md). [`rule-m1.md`](rule-m1.md) and [`rule-m2.md`](rule-m2.md) stay fixed and unedited. Their runs' results stand as measured and enter no measure here. Each threshold is marked in one of two ways:
- **carried over** from rule-m2, and through it from rule-m1 (approved by the author, Oct 29–30, 2026);
- **approved by the author, Oct 30, 2026**.

A change after the first trial is a revision, dated and explained at the top.

## The rows

The four builds of spec v0.3 §16.1 (approved by the author, Oct 30, 2026). No row pre-stores 0 or 1.

| Build | Structures, by name (v0.3 §16.3) | add, mul | sop, pos |
| --- | --- | --- | --- |
| **TC** | each matrix's pairs | 3 | 7 |
| **Hash-V-pack** | per matrix, (a) cells and (l) the packed list; (h) build table and (p) pack map, each only during a build | 8 | 16 |
| **Pack-primes** | as Hash-V-pack, plus per matrix **(f) the factorization list**, new: primes found by trial division | 11 | 23 |
| **Pack-coprime** | as Pack-primes, with factors over a coprime base, plus per matrix **(q) the coprime base**, new, and **(r) the denominator-factor lists**, new, only during each operation step | 15 | 31 |

- All four use the same 64-bit core and the same inputs, and give the same value in every cell.
- TC and Hash-V-pack are identical call for call, as in m2.
- The two factored rows take TC's cases and TC's core arithmetic (exact divisions, multiplications, additions) call for call, but obtain their gcd values from factorizations (v0.3 §16.5, §16.9 item 3).
- Results are reported **per measure, with no overall winner**.

**Four job sets, four runs** (carried over): **add**, **mul**, **sop** ($A \cdot B + C \cdot D$), **pos** ($(A + B) \cdot (C + D)$). Each run has its own steps 0–7, pilot, hard stop, results folder and report. Nothing measured in one run enters a measure of another.

**Run order** (approved by the author, Oct 30, 2026): add, mul, sop, pos, **back to back**, as rule-m2. A run that stops does not stop the next one, and its report says so.

## Measurement rule

1. **Rows.** Each job runs **five rows**. **Order for the rounds (approved by the author, Oct 30, 2026):** TC, Hash-V-pack, Pack-primes, Pack-coprime, Hash-V-pack-null. The null row is Hash-V-pack's binary (approved by the author, Oct 30, 2026) and stays last, as in m1 and m2.

   **The grid, after the m3 review** (approved by the author, Oct 30, 2026; Workloads below): $n \le 64$ for every row and set, and sop also without $\rho = 16$. Every row runs every job of its set. There is no "not run" cell.

   **The 10 s cap applies as carried over**, with over-cap reporting. A row whose calibration rep passes 10 s runs no rounds on that job. Its time verdicts are **over cap**, with the calibration rep's time as the magnitude. Its memory verdicts stand, from the memory run. Expected over cap: Pack-primes in sop at $n = 64$ (about 14–29 s per run). Pack-primes in pos at $n = 64$ (about 6–12 s) may land on either side of the cap.

2. **Builds.** As rule-m2 item 2 (carried over):
   - `gcc -std=c11 -O2`;
   - the 08 module, `tc3.c` and m2's `mh.c` unchanged, by sha256;
   - `tcm.c`'s diff checked;
   - three builds per row;
   - one counting and one memory run per row and job, with a limit of **10 minutes** each;
   - repeats on jobs **1 and 6** of each set identical;
   - the same binaries for all four runs.

3. **Gates before the measured run.** As rule-m2 item 3 (carried over), for every row and job of the set:
   - the unit tests;
   - spec v0.3 §16.9's exact checks, every step included;
   - the call-for-call check, **in v0.3 §16.9 item 3's adapted form for the factored rows** (approved by the author, Oct 30, 2026);
   - the TC gate.

   Gates MAY run two at a time, only before the timed steps.

4. **Timing.** As rule-m2 item 4, every number carried over:
   - 12 interleaved rounds, rotated by one row per round over the five rows;
   - r, the smallest power of two with a rep of at least 50 ms;
   - 3 reps per invocation, or 1 if the calibration rep lasts at least 1 s;
   - calibration per job for the four builds, the null row taking Hash-V-pack's r;
   - core 1, the runner on core 0;
   - a cap of **10 s** per rep;
   - the process's CPU time reported only.

   The parts are rule-m2's:
   - **one build per input matrix**, which now includes its factoring;
   - **the operation**: every step with its result's `create`, its reconciliation (Pack-coprime) and its result's build, pack and factoring;
   - **the read-out.**

   So **a result's factoring is operation time** (v0.3 §16.6 item 2).

5. **Nothing of the runner's own runs during a timed phase.** Carried over in full from rule-m2 item 5, every number included:
   - other processes at most $5 + \lceil L \rceil$ ticks;
   - hosts at most $5 + \lceil 2 R_h L \rceil$;
   - a stop above 4 ticks per second;
   - one re-run of a disturbed block;
   - a timeout of at least 45 minutes per background command.

6. **Idle machine.** Carried over: a 2-second sample; idle at 5% and 2%; re-check every 15 s; stop after 10 minutes.

7. **Measures, per job, for each pairing, in each run.**
   - **The eight headline measures, carried over** (rule-m2 item 7, the author's), with their run assignment, ratios, bootstrap (10,000 paired resamples of the rounds, seed 20261029), pooling of measure 1, and accounted bytes per cell with the heap beside them.
   - **Pairings, approved by the author, Oct 30, 2026** (v0.3 §16.1):
     - **against TC**, with verdicts: Hash-V-pack, Pack-primes, Pack-coprime;
     - **Pack-primes against Hash-V-pack**, with verdicts;
     - **Pack-coprime against Hash-V-pack**, with verdicts;
     - **Pack-coprime against Pack-primes:** reported, no verdict on any measure.
   - **Gcd work.**
     - For Hash-V-pack and its null, **a check**: G(row)/G(TC) MUST be exactly 1, call for call (carried over).
     - For the factored rows, **reported, not checked** (approved by the author, Oct 30, 2026). Report G(row)/G(TC) per part, beside their factor work: merges and entries compared, divisibility tests, trial-division candidates, refinement gcds.
     - What is checked for them instead is v0.3 §16.9 item 3: the same case and core arithmetic as TC per operation, and every obtained gcd equal to Euclid's in the test build.
   - **Reported, no verdict:** as rule-m2, without grow-map's growth peak, plus the counters of item 13.

8. **Null control.** Hash-V-pack against Hash-V-pack-null gives the null spread per time measure, and **h = max(5%, that spread)** for every pairing of the run. There are two bands per run, as rule-m2. The caveat and the "(c)" mark are carried over. The (c) mark applies in particular to the factored rows, whose launches at $n \ge 32$ often run 1 rep.

9. **Verdicts and magnitudes.** Carried over:
   - time wins when lo > 1 + h, loses when hi < 1 − h;
   - memory wins at ≤ 0.90, loses at ≥ 1.10;
   - the severe flag at 2.0;
   - "over cap" on both time measures;
   - no verdict without its magnitude.


10. **No profile** (carried over).

11. **Strata, and no overall winner.** Carried over, per run, for the headline measures: NONE, LOW, MID, HIGH ($\rho$ = 1, 4, 16, 64), with counts of verdicts (over cap included), median ratios, severe flags, and the size × sharing grid. sop has no MID stratum (Workloads).

12. **Checks, and bugs after the gates.** As rule-m2 item 12 (carried over), every step included. **New bugs, approved by the author, Oct 30, 2026:**
    - an (f) list that does not multiply back to its number;
    - an (f) list out of order;
    - a factor of Pack-primes that is not the model's prime;
    - a Pack-coprime base that is not the model's, or not pairwise coprime;
    - an obtained gcd that differs from Euclid's;
    - a factored row's case or core arithmetic that differs from TC's;
    - a read of 0's factorization;
    - a compared pair, after reconciliation, that is neither equal nor coprime;
    - (r) alive outside its step;
    - (h) or (p) alive during a factoring.

13. **Also reported.** As rule-m2 item 13, for Hash-V-pack. For the factored rows (approved by the author, Oct 30, 2026), the counters of v0.3 §16.10:
    - per matrix: $W$, $E$, (f)'s bytes, $\lvert q \rvert$, trial-division candidates, refinement gcds, splits, pass 2's tests;
    - per step: reconciliation work, merges, tests, per-piece gcds, leftover splits and the new sub-cases' counts.

    The m2 runs are linked for reference only.

## Workloads

- **add, mul:** `m1_gen.py`, seeds 20261029 + $j$ and + 100 + $j$ (carried over). Step 0's files MUST equal m1's by sha256, job for job.
- **sop, pos:** m2's generator, seeds 20261029 + 200 + $j$ and + 300 + $j$ (carried over). Step 0's files MUST equal m2's by sha256, job for job.
- **The grid** (approved by the author, Oct 30, 2026, after the m3 review):
  - **add, mul, pos:** $n \in \{16, 32, 64\}$ by $\rho \in \{1, 4, 16, 64\}$, **jobs 1–12**, numbered as rule-m1;
  - **sop:** the same without $\rho = 16$, **jobs 1, 2, 4, 5, 6, 8, 9, 10, 12** (9 jobs).
  - Every job keeps its rule-m1 number and seed, so each job is the same job as in m1 or m2.
  - *Why $n = 128$ goes:* at $n = 128$, Pack-primes' trial division costs about 33 s per mul run, 47 s per pos run and 100 s per sop run. That cost is paid in full by every gate, counting and memory run, whatever the cap. Pack-coprime's refinement costs about $1.5 \times 10^8$ gcds per product matrix there. With $n = 128$, the pilot projection of mul, sop and pos would pass the 30-minute limit.
  - *Why sop also drops $\rho = 16$:* with all 12 jobs, sop's projection at the worst cost basis is 27.7 minutes, a margin of only 1.08 on the 30-minute limit. sop is the costliest set, since it factors three product-like matrices per run. Its intermediates and result are nearly unshared at every $\rho$ (v0.2 §15.5 item 9), so the dropped level changes only the inputs' sharing. It is the MID stratum, so sop's strata are NONE, LOW and HIGH. No other set needs a cut: see the estimate below.

## Run plan (once per run)

Steps 0–7 as rule-m2's, per run. Results go to **`11-matrix/test/results-m3-add/`**, **`-mul/`**, **`-sop/`** and **`-pos/`** (approved by the author, Oct 30, 2026).

- **Step 0, outside the clock** (carried over in form). The model adds:
  - each distinct whole number's factorization (`sympy.factorint`), with its trial-division count in closed form (v0.3 §16.4 item 3);
  - a run of Pack-coprime's refinement and reconciliation, in Python, for its exact base and counters: about $10^7$ gcds per product matrix at $n = 64$.

  About 1 minute for add, and 1–3 minutes for each of the others.
- **Step 2** now includes the unit tests' large trial divisions (v0.3 §16.9 item 6): the largest prime below $2^{63}$ and a semiprime of two primes near $2^{31}$. They are schoolbook, like the row, with no shortcut: about $2.6 \times 10^9$ candidates, 9–20 s in the test build and about twice that under the sanitizers.
- **Step 3, pilot** (approved by the author, Oct 30, 2026): jobs **1, 6 and 10**, one per size ($n$ = 16, 32, 64), every row, r calibration and 2 rounds. Rule-m2 used jobs 1, 6 and 13. Job 13 is gone, and job 10 stands for $n = 64$, which is no longer at the launch floor for the factored rows. All three are in every set's grid. **Stop rule, in place of rule-m2's** (approved by the author, Oct 30, 2026):
  $$\text{projection} = \text{elapsed} + 1.2 \Big[ \sum_{c} N_c \Big( O_c + \sum_{\text{rows}} (K_{\text{row},c} + 6\, P_{\text{row},c}) \Big) + \tfrac{1}{2} \sum_{c} N_c \sum_{\text{rows}} 4\, t_{\text{row},c} \Big],$$

  where:
  - $c$ runs over the three sizes, each with its pilot job: 1, 6 and 10;
  - $N_c$ is the number of the run's jobs of size $c$: 4 for each size in add, mul and pos, and 3 for each size in sop;
  - $K_{\text{row},c}$ is the row's calibration launch in that pilot block;
  - $P_{\text{row},c}$ is the sum of its two pilot round launches, 0 if it was over cap;
  - $O_c$ is the block's checks, its wall time less its launches;
  - $t_{\text{row},c}$ is the row's time per run from its calibration.

  - **elapsed** is steps 1–3 as they ran: the builds, the unit tests with their large trial divisions, and the pilot.
  - The bracket's first term projects step 6.
  - Its last term projects steps 4–5: each row and job makes a gate, a counting and a memory run, taken as 4 runs (the test build is the slowest), two at a time.

  *Why:* rule-m2's factor 5 assumed that every row runs every round, and that steps 4–5 cost little. Neither holds for the factored rows. An over-cap row runs only its calibration, which rule-m2's form would overcount five times. The gate, counting and memory runs of a 6–29 s row cost more than its rounds.

  **If the projection exceeds 30 minutes, the run stops** (carried over). It also stops on a host above 4 ticks per second.
- **Step 6:** per job, in job order, as rule-m2: the idle check, calibration (4 launches), 12 rounds (60 launches, fewer when a row is over cap), and the disturbance check.
- **Hard stop at 40 minutes, per run** (carried over), from step 1 to the end of step 6. A stopped run reports no verdict.

### Runtime estimate (non-normative)

**Basis, with the range used for the worst case.**
- **Trial division:** 5 ns per candidate (3.5–7.5): one 64-bit division with remainder, on this machine's Xeon at 2.7 GHz (AVX-512 IFMA and VBMI, so Ice Lake or later).
- **Pack-coprime's refinement:** 60 ns per gcd on product numerators (35–85).
- **Pack-coprime's pass 1 on one input** at $n = 64$, $\rho = 1$: 0.25 s (0.15–0.35, the review's estimate), scaled down by the gcd count at smaller $n$. All four jobs of a size are taken at $\rho = 1$, which overstates.
- **Pass 2's divisibility tests:** at the trial-division rate.
- **Counts:**
  - from m2's counters ($W$ of every matrix);
  - from a planning simulation of the generators: 400 cells per size, and Pack-coprime's refinement run on full matrices up to $n = 64$.
  - The mean trial-division count is set by the few numerators that are prime or nearly so. It is uncertain to about ±15%, within the range above.
- **TC, Hash-V-pack and the null, from m2's logs:** about 0.2 s per launch at $n \le 64$, 38 launches and a 2 s idle check per block.
- **Steps 1–5 measured in m2:** steps 1–2 took 21 s; steps 4–5 took 1–28 s.
- **Steps 1–3 as added here:** the unit tests' trial divisions (13 s central, both builds about 40 s), and the pilot.

**Factoring and arithmetic time per run**, in seconds, central (range):

| Run | Row | $n = 16$ | 32 | 64 |
| --- | --- | --- | --- | --- |
| add | Pack-primes | < 0.01 | < 0.01 | < 0.01 |
| add | Pack-coprime | 0.01 | 0.1 | 0.6 (0.4–0.9) |
| mul | Pack-primes | 0.22 | 0.85 | 6.0 (4.2–9.0) |
| mul | Pack-coprime | 0.02 | 0.13 | 1.2 (0.7–1.6) |
| sop | Pack-primes | 0.7 | 3.0 | 19.5 (14–29), **over cap** |
| sop | Pack-coprime | 0.03 | 0.3 | 3.0 (1.8–4.2) |
| pos | Pack-primes | 0.3 | 1.7 | 8.1 (5.6–12), near the cap |
| pos | Pack-coprime | 0.03 | 0.25 | 1.9 (1.1–2.7) |

**Steps 1–6 on the clock, and the pilot's projection, minutes:**

| Run | Jobs | Run, central | Run, worst | Projection, central | Projection, worst | Projection margin on 30, worst |
| --- | --- | --- | --- | --- | --- | --- |
| add | 12 | 6.1 | 7.3 | 7.1 | 8.4 | 3.6 |
| mul | 12 | 15.0 | 19.2 | 17.6 | 22.6 | 1.33 |
| sop | 9 | 13.6 | 18.3 | 15.9 | 21.4 | 1.40 |
| pos | 12 | 17.8 | 21.2 | 21.0 | 25.0 | 1.20 |

- **Worst** is the largest over every combination of the ranges above. It is not at the slowest corner: a row just under the 10 s cap runs 13 launches per job, while a slower one goes over cap and runs one.
- **The projection** is the run's own estimate with steps 4–6 multiplied by the rule's 1.2, so it sits above the run.
- **For comparison:** sop with all 12 jobs would project 20.6 central and 27.7 worst, a margin of 1.08. That is why it drops $\rho = 16$.

**Margins.**
- **Hard stop (40 min):** about 1.9 at the worst basis (40 over 21.2).
- **Projection limit (30 min):** at least 1.2 at the worst basis, and at least 1.4 at the central basis.
- **A re-run** of a disturbed $n = 64$ block adds about 0.5–1.5 minutes.

**No threshold of rule-m2 changes.** The grid is smaller, and the pilot has a new job and a new projection form.

## Limitations, reported with the results

Carried over from rule-m2, without Hash-V-fixed and Hash-V-grow-map. Also (approved by the author, Oct 30, 2026):
- **No $n = 128$ in any row or set, and no $\rho = 16$ in sop.** The step from 64 to 128, where m2 saw the handle width change, is not measured.
- **Pack-primes is deliberately unoptimized.** Its times measure schoolbook trial division, by the author's design, not factoring in general. A fast factorizer is a later, separate change. The aim is a schoolbook baseline: find the weak points first, then optimize systematically.
- **Pack-coprime refines eagerly, per matrix**, the form 09 found costly: 58–161 times the control's gcd work there (09 findings, lesson 1). On this grid that is about $10^7$ gcds per product matrix at $n = 64$. Its reconciliation, by contrast, is on demand and limited to denominator factors (v0.3 §16.7).
- **The factored rows keep TC's value arithmetic.** They measure only what factored gcds save and what factoring costs, not value shortcuts the factors would allow (v0.3 §16.5).
- **On this workload every denominator is $\{2, 3, 5\}$-smooth**, so the denominators' factorizations are short, and reconciliation is small. Workloads with richer denominators would change both.
- **The factoring time is not separated from the operation's time.** The counters separate the work.
- Results are reported per run, and not combined across runs or with m1 and m2.

## Harness

The m2 harness is copied to `m3_*` files. Its spawner, checks, calibration, bootstrap and stops are unchanged (carried over).

New (approved by the author, Oct 30, 2026):
- the rows of v0.3;
- the grid of 12 jobs, and sop's 9;
- the model with factorizations, trial-division counts and Pack-coprime's simulation;
- the adapted call-for-call check;
- the pairings and the reported gcd work of the factored rows;
- the pilot's jobs 1, 6 and 10, and its projection;
- the new counters and accounted structures;
- this rule's pin.

The harness refuses to write in `../10-interning/`, in the m1 and m2 results, or in an earlier m3 run's folder, and checks their sha256 before and after each run (carried over in form). Builder decisions go in a new `11-matrix/test/DECISIONS-m3.md`, pending the author's review.
