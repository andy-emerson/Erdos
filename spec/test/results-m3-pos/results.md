# Design 3 on matrices, round m3 (factored whole numbers), the product of sums (A + B)(C + D): results

Run started 2026-10-30T15:08:34-05:00.

## The measurement rule (research/rule-m3.md, verbatim, written before the first trial)

sha256 `c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd`

<!-- rule begins -->
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
<!-- rule ends -->

## The run

- The pos run (job set `pos`: S = A + B; U = C + D; R = S x U).
- Started 2026-10-30T15:08:34-05:00; finished 2026-10-30T15:23:44-05:00.
- Step 0 (before the clock): 65.9 s; step 0's files: equal to m2's by sha256.
- Run time 14.1 minutes (hard stop 40).
- Rule sha256 `c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd`; round 3's results after the run: unchanged; m1's and m2's results and the other m3 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[22980], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T15:09:40-05:00: TC `3cbbe6f47fb7`, MHVP `7ed2109b9193`, MHPP `6704a1ab9994`, MHPC `0da57d01e10e`, TC3GATE `685595cafdb3`
- Unit tests: m3_unit: m3_unit: 811293 checks, 0 failures; m3_diff: m3_diff: 3000 seeds, 3000 jobs, 1050092 operations; obtained gcds checked against Euclid 2936692, merges 2186690, reconciliation splits 2796, leftover splits 106; 1523395 checks, 0 failures; m3_alloc-timing: m3_alloc (timing build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-counting: m3_alloc (counting build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-memory: m3_alloc (memory build): 160 jobs, 2893 moments, 43830 checks, 0 failures; m3_unit-asan: m3_unit: 811293 checks, 0 failures; m3_diff-asan: m3_diff: 600 seeds, 600 jobs, 209812 operations; obtained gcds checked against Euclid 579628, merges 431634, reconciliation splits 557, leftover splits 20; 303709 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 10], one per size; block wall times job 1 5.7 s, job 6 10.1 s, job 10 25.2 s.
- Per size class c (its pilot job): O_c the block's checks, K the calibration launches, P the two round launches (0 over cap), t the time per run from the calibration; N_c the run's jobs of that size:
  - size 0 (job 1, N_c = 4): O 2.0 s; TC: K 0.14 s, P 0.44 s, t 0.001 s; Hash-V-pack: K 0.17 s, P 0.52 s, t 0.001 s; Pack-primes: K 0.21 s, P 1.23 s, t 0.204 s; Pack-coprime: K 0.11 s, P 0.36 s, t 0.007 s; Hash-V-pack-null: K 0.00 s, P 0.52 s, t n/a s
  - size 1 (job 6, N_c = 4): O 2.0 s; TC: K 0.14 s, P 0.42 s, t 0.004 s; Hash-V-pack: K 0.16 s, P 0.48 s, t 0.005 s; Pack-primes: K 0.85 s, P 5.06 s, t 0.844 s; Pack-coprime: K 0.07 s, P 0.38 s, t 0.063 s; Hash-V-pack-null: K 0.00 s, P 0.48 s, t n/a s
  - size 2 (job 10, N_c = 4): O 2.0 s; TC: K 0.11 s, P 0.46 s, t 0.035 s; Hash-V-pack: K 0.12 s, P 0.48 s, t 0.039 s; Pack-primes: K 5.27 s, P 10.55 s, t 5.262 s; Pack-coprime: K 0.82 s, P 4.88 s, t 0.820 s; Hash-V-pack-null: K 0.00 s, P 0.49 s, t n/a s
- Projection elapsed + 1.2 [sum_c N_c (O_c + sum_rows (K + 6 P)) + 1/2 sum_c N_c sum_rows 4 t] = 17.0 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.04314808422506041, '79': 0.04314808422506041, '84': 0.04314808422506041, '92': 0.24850111016934787, '22980': 0.04314808422506041}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 5 product-of-sums time; 6 stable bytes of one stored input matrix; peak bytes during the operation (reported). Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-pack against Hash-V-pack-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses; (f) and (q) included). Pack-coprime against Pack-primes is reported with no verdict on any measure. A row over cap runs no rounds: its time verdicts are over cap, with its calibration rep's time; its memory verdicts stand. Gcd work: a check for Hash-V-pack, reported for the factored rows. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-pack, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m3 item 8, carried over).

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.477 [0.473, 0.486], h 0.050 | loss, 0.850 [0.848, 0.854], h 0.050 | reported, 0.816 [0.815, 0.820] | reported, 0.589 [0.568, 0.724] | win, 0.804 (heap 0.820) | reported, 1.086 (heap 1.097) |
| 2 (n 16, rho 4) | loss, 0.550 [0.546, 0.556], h 0.050 | loss, 0.849 [0.846, 0.854], h 0.050 | reported, 0.827 [0.823, 0.831] | reported, 0.628 [0.575, 0.678] | win, 0.297 (heap 0.323) | reported, 0.795 (heap 0.813) |
| 3 (n 16, rho 16) | loss, 0.586 [0.578, 0.599], h 0.050 | loss, 0.848 [0.843, 0.854], h 0.050 | reported, 0.831 [0.826, 0.836] | reported, 0.571 [0.564, 0.667] | win, 0.179 (heap 0.209) | reported, 0.705 (heap 0.724) |
| 4 (n 16, rho 64) | loss, 0.612 [0.599, 0.629], h 0.050 | loss, 0.849 [0.846, 0.858], h 0.050 | reported, 0.837 [0.833, 0.845] | reported, 0.565 [0.562, 0.688] | win, 0.139 (heap 0.168) | reported, 0.596 (heap 0.617) |
| 5 (n 32, rho 1) | loss, 0.499 [0.495, 0.501], h 0.050 | loss, 0.889 [0.884, 0.893], h 0.050 | reported, 0.871 [0.866, 0.875] | reported, 0.587 [0.574, 0.606] | win, 0.742 (heap 0.747) | reported, 1.037 (heap 1.040) |
| 6 (n 32, rho 4) | loss, 0.568 [0.565, 0.571], h 0.050 | loss, 0.890 [0.886, 0.895], h 0.050 | reported, 0.876 [0.873, 0.882] | reported, 0.650 [0.595, 0.779] | win, 0.388 (heap 0.394) | reported, 0.835 (heap 0.839) |
| 7 (n 32, rho 16) | loss, 0.625 [0.621, 0.627], h 0.050 | loss, 0.888 [0.884, 0.892], h 0.050 | reported, 0.878 [0.874, 0.882] | reported, 0.719 [0.596, 0.797] | win, 0.168 (heap 0.176) | reported, 0.700 (heap 0.705) |
| 8 (n 32, rho 64) | loss, 0.643 [0.638, 0.647], h 0.050 | loss, 0.891 [0.889, 0.893], h 0.050 | reported, 0.883 [0.880, 0.884] | reported, 0.600 [0.591, 0.641] | win, 0.138 (heap 0.145) | reported, 0.638 (heap 0.644) |
| 9 (n 64, rho 1) | loss, 0.510 [0.508, 0.514], h 0.050 | loss, 0.902 [0.896, 0.904], h 0.050 | reported, 0.893 [0.887, 0.895] | reported, 0.658 [0.626, 0.771] | win, 0.681 (heap 0.682) | reported, 0.996 (heap 0.997) |
| 10 (n 64, rho 4) | loss, 0.585 [0.582, 0.588], h 0.050 | loss, 0.900 [0.894, 0.904], h 0.050 | reported, 0.894 [0.887, 0.897] | reported, 0.804 [0.685, 0.862] | win, 0.374 (heap 0.375) | reported, 0.820 (heap 0.822) |
| 11 (n 64, rho 16) | loss, 0.636 [0.632, 0.640], h 0.050 | loss, 0.903 [0.894, 0.915], h 0.050 | reported, 0.898 [0.889, 0.911] | reported, 0.766 [0.693, 0.836] | win, 0.284 (heap 0.286) | reported, 0.768 (heap 0.769) |
| 12 (n 64, rho 64) | loss, 0.649 [0.644, 0.654], h 0.050 | loss, 0.901 [0.898, 0.905], h 0.050 | reported, 0.896 [0.893, 0.900] | reported, 0.663 [0.651, 0.706] | win, 0.136 (heap 0.138) | reported, 0.660 (heap 0.661) |

### Pack-primes against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.189 [0.188, 0.194], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.455 [0.383, 0.619] | loss, 2.323 (heap 2.324) **severe memory loss** | reported, 2.291 (heap 2.293) |
| 2 (n 16, rho 4) | loss, 0.360 [0.359, 0.363], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.374 [0.310, 0.435] | win, 0.761 (heap 0.791) | reported, 1.392 (heap 1.412) |
| 3 (n 16, rho 16) | loss, 0.480 [0.473, 0.492], h 0.050 | loss, 0.002 [0.002, 0.002], h 0.050 | reported, 0.002 [0.002, 0.002] | reported, 0.415 [0.300, 0.557] | win, 0.318 (heap 0.357) | reported, 1.034 (heap 1.061) |
| 4 (n 16, rho 64) | loss, 0.516 [0.500, 0.529], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.380 [0.354, 0.488] | win, 0.177 (heap 0.220) | reported, 0.694 (heap 0.727) |
| 5 (n 32, rho 1) | loss, 0.217 [0.216, 0.218], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.320 [0.289, 0.396] | loss, 2.098 (heap 2.099) **severe memory loss** | reported, 2.087 (heap 2.088) |
| 6 (n 32, rho 4) | loss, 0.419 [0.414, 0.423], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.320 [0.303, 0.458] | win, 0.767 (heap 0.775) | reported, 1.329 (heap 1.334) |
| 7 (n 32, rho 16) | loss, 0.571 [0.567, 0.575], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.454 [0.339, 0.507] | win, 0.284 (heap 0.294) | reported, 1.010 (heap 1.017) |
| 8 (n 32, rho 64) | loss, 0.642 [0.635, 0.648], h 0.050 | loss, 0.004 [0.004, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.292 [0.276, 0.359] | win, 0.171 (heap 0.181) | reported, 0.750 (heap 0.757) |
| 9 (n 64, rho 1) | loss, 0.215 [0.211, 0.219], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.140 [0.122, 0.172] | loss, 1.889 (heap 1.890) | reported, 1.940 (heap 1.940) |
| 10 (n 64, rho 4) | loss, 0.393 [0.362, 0.400], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.184 [0.143, 0.214] | win, 0.718 (heap 0.719) | reported, 1.269 (heap 1.271) |
| 11 (n 64, rho 16) | loss, 0.538 [0.535, 0.555], h 0.050 (c) | loss, 0.008 [0.008, 0.008], h 0.050 (c) | reported, 0.008 [0.008, 0.008] | reported, 0.169 [0.129, 0.202] | win, 0.378 (heap 0.381) | reported, 1.069 (heap 1.070) |
| 12 (n 64, rho 64) | loss, 0.569 [0.561, 0.601], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.131 [0.117, 0.160] | win, 0.165 (heap 0.168) | reported, 0.844 (heap 0.846) |

### Pack-coprime against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.013 [0.013, 0.014], h 0.050 | loss, 0.098 [0.098, 0.099], h 0.050 | reported, 0.074 [0.074, 0.074] | reported, 0.566 [0.538, 0.701] | loss, 2.663 (heap 2.668) **severe memory loss** | reported, 2.630 (heap 2.635) |
| 2 (n 16, rho 4) | loss, 0.092 [0.091, 0.092], h 0.050 | loss, 0.098 [0.098, 0.099], h 0.050 | reported, 0.098 [0.097, 0.098] | reported, 0.607 [0.555, 0.659] | win, 0.879 (heap 0.919) | reported, 1.606 (heap 1.631) |
| 3 (n 16, rho 16) | loss, 0.325 [0.321, 0.334], h 0.050 | loss, 0.120 [0.119, 0.121], h 0.050 | reported, 0.124 [0.123, 0.125] | reported, 0.556 [0.540, 0.652] | win, 0.352 (heap 0.398) | reported, 1.180 (heap 1.212) |
| 4 (n 16, rho 64) | loss, 0.473 [0.463, 0.483], h 0.050 | loss, 0.139 [0.139, 0.141], h 0.050 | reported, 0.143 [0.143, 0.145] | reported, 0.535 [0.524, 0.656] | win, 0.187 (heap 0.238) | reported, 0.791 (heap 0.831) |
| 5 (n 32, rho 1) | loss, 0.007 [0.007, 0.007], h 0.050 | loss, 0.068 [0.068, 0.069], h 0.050 | reported, 0.055 [0.055, 0.056] | reported, 0.551 [0.535, 0.595] | loss, 2.324 (heap 2.326) **severe memory loss** | reported, 2.345 (heap 2.347) |
| 6 (n 32, rho 4) | loss, 0.052 [0.052, 0.053], h 0.050 | loss, 0.070 [0.070, 0.070], h 0.050 | reported, 0.069 [0.069, 0.070] | reported, 0.601 [0.529, 0.737] | win, 0.854 (heap 0.863) | reported, 1.507 (heap 1.513) |
| 7 (n 32, rho 16) | loss, 0.266 [0.264, 0.267], h 0.050 | loss, 0.073 [0.072, 0.073], h 0.050 | reported, 0.074 [0.074, 0.074] | reported, 0.694 [0.561, 0.775] | win, 0.315 (heap 0.328) | reported, 1.152 (heap 1.160) |
| 8 (n 32, rho 64) | loss, 0.560 [0.554, 0.566], h 0.050 | loss, 0.090 [0.088, 0.090], h 0.050 | reported, 0.091 [0.090, 0.092] | reported, 0.522 [0.484, 0.584] | win, 0.180 (heap 0.193) | reported, 0.853 (heap 0.863) |
| 9 (n 64, rho 1) | loss, 0.004 [0.004, 0.004], h 0.050 | loss, 0.043 [0.043, 0.044], h 0.050 | reported, 0.039 [0.039, 0.039] | reported, 0.567 [0.505, 0.644] | loss, 2.026 (heap 2.027) **severe memory loss** | reported, 2.130 (heap 2.131) |
| 10 (n 64, rho 4) | loss, 0.027 [0.027, 0.028], h 0.050 | loss, 0.043 [0.043, 0.044], h 0.050 | reported, 0.043 [0.043, 0.043] | reported, 0.686 [0.579, 0.770] | win, 0.773 (heap 0.776) | reported, 1.414 (heap 1.416) |
| 11 (n 64, rho 16) | loss, 0.166 [0.164, 0.168], h 0.050 | loss, 0.044 [0.044, 0.045], h 0.050 | reported, 0.045 [0.044, 0.045] | reported, 0.621 [0.545, 0.719] | win, 0.400 (heap 0.403) | reported, 1.193 (heap 1.195) |
| 12 (n 64, rho 64) | loss, 0.490 [0.485, 0.493], h 0.050 | loss, 0.048 [0.047, 0.048], h 0.050 | reported, 0.048 [0.048, 0.049] | reported, 0.509 [0.481, 0.567] | win, 0.172 (heap 0.176) | reported, 0.952 (heap 0.954) |

### Pack-primes against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.397 [0.395, 0.401], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.772 [0.616, 0.950] | loss, 2.888 (heap 2.834) **severe memory loss** | reported, 2.109 (heap 2.090) |
| 2 (n 16, rho 4) | loss, 0.655 [0.651, 0.661], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.595 [0.493, 0.662] | loss, 2.562 (heap 2.451) **severe memory loss** | reported, 1.751 (heap 1.737) |
| 3 (n 16, rho 16) | loss, 0.819 [0.810, 0.826], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.727 [0.526, 0.918] | loss, 1.773 (heap 1.711) | reported, 1.468 (heap 1.464) |
| 4 (n 16, rho 64) | loss, 0.843 [0.820, 0.853], h 0.050 | loss, 0.006 [0.006, 0.006], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.672 [0.629, 0.744] | loss, 1.271 (heap 1.314) | reported, 1.164 (heap 1.179) |
| 5 (n 32, rho 1) | loss, 0.435 [0.433, 0.438], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.006] | reported, 0.544 [0.494, 0.666] | loss, 2.826 (heap 2.810) **severe memory loss** | reported, 2.012 (heap 2.007) |
| 6 (n 32, rho 4) | loss, 0.738 [0.728, 0.745], h 0.050 | loss, 0.006 [0.006, 0.006], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.493 [0.452, 0.722] | loss, 1.977 (heap 1.966) | reported, 1.593 (heap 1.591) |
| 7 (n 32, rho 16) | loss, 0.914 [0.908, 0.920], h 0.050 | loss, 0.006 [0.006, 0.006], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.630 [0.537, 0.667] | loss, 1.687 (heap 1.675) | reported, 1.442 (heap 1.442) |
| 8 (n 32, rho 64) | tie, 0.998 [0.989, 1.009], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.486 [0.464, 0.591] | loss, 1.243 (heap 1.249) | reported, 1.174 (heap 1.177) |
| 9 (n 64, rho 1) | loss, 0.422 [0.412, 0.429], h 0.050 (c) | loss, 0.008 [0.008, 0.008], h 0.050 (c) | reported, 0.008 [0.008, 0.008] | reported, 0.212 [0.184, 0.252] | loss, 2.774 (heap 2.770) **severe memory loss** | reported, 1.947 (heap 1.946) |
| 10 (n 64, rho 4) | loss, 0.672 [0.620, 0.684], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.008] | reported, 0.229 [0.207, 0.256] | loss, 1.921 (heap 1.918) | reported, 1.547 (heap 1.547) |
| 11 (n 64, rho 16) | loss, 0.845 [0.841, 0.872], h 0.050 (c) | loss, 0.009 [0.009, 0.009], h 0.050 (c) | reported, 0.009 [0.009, 0.009] | reported, 0.221 [0.186, 0.242] | loss, 1.330 (heap 1.331) | reported, 1.392 (heap 1.392) |
| 12 (n 64, rho 64) | loss, 0.877 [0.866, 0.926], h 0.050 (c) | loss, 0.008 [0.008, 0.008], h 0.050 (c) | reported, 0.008 [0.008, 0.008] | reported, 0.198 [0.175, 0.230] | loss, 1.214 (heap 1.218) | reported, 1.279 (heap 1.279) |

### Pack-coprime against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.028 [0.028, 0.028], h 0.050 | loss, 0.116 [0.115, 0.116], h 0.050 | reported, 0.091 [0.091, 0.091] | reported, 0.961 [0.940, 0.977] | loss, 3.312 (heap 3.254) **severe memory loss** | reported, 2.421 (heap 2.401) |
| 2 (n 16, rho 4) | loss, 0.166 [0.164, 0.168], h 0.050 | loss, 0.115 [0.115, 0.116], h 0.050 | reported, 0.118 [0.118, 0.119] | reported, 0.967 [0.943, 0.973] | loss, 2.962 (heap 2.846) **severe memory loss** | reported, 2.020 (heap 2.007) |
| 3 (n 16, rho 16) | loss, 0.555 [0.551, 0.558], h 0.050 | loss, 0.142 [0.141, 0.143], h 0.050 | reported, 0.149 [0.148, 0.150] | reported, 0.973 [0.949, 0.982] | loss, 1.962 (heap 1.908) | reported, 1.675 (heap 1.672) |
| 4 (n 16, rho 64) | loss, 0.772 [0.754, 0.779], h 0.050 | loss, 0.164 [0.163, 0.164], h 0.050 | reported, 0.171 [0.171, 0.172] | reported, 0.948 [0.930, 0.973] | loss, 1.346 (heap 1.417) | reported, 1.327 (heap 1.346) |
| 5 (n 32, rho 1) | loss, 0.014 [0.014, 0.014], h 0.050 | loss, 0.077 [0.077, 0.077], h 0.050 | reported, 0.064 [0.063, 0.064] | reported, 0.938 [0.902, 1.010] | loss, 3.131 (heap 3.115) **severe memory loss** | reported, 2.261 (heap 2.256) |
| 6 (n 32, rho 4) | loss, 0.092 [0.091, 0.093], h 0.050 | loss, 0.079 [0.078, 0.079], h 0.050 | reported, 0.079 [0.079, 0.079] | reported, 0.925 [0.853, 0.973] | loss, 2.201 (heap 2.191) **severe memory loss** | reported, 1.806 (heap 1.804) |
| 7 (n 32, rho 16) | loss, 0.425 [0.423, 0.427], h 0.050 | loss, 0.082 [0.082, 0.082], h 0.050 | reported, 0.084 [0.084, 0.085] | reported, 0.964 [0.898, 0.987] | loss, 1.869 (heap 1.864) | reported, 1.645 (heap 1.646) |
| 8 (n 32, rho 64) | loss, 0.871 [0.862, 0.882], h 0.050 | loss, 0.100 [0.099, 0.101], h 0.050 | reported, 0.104 [0.102, 0.104] | reported, 0.870 [0.804, 0.949] | loss, 1.305 (heap 1.326) | reported, 1.336 (heap 1.341) |
| 9 (n 64, rho 1) | loss, 0.008 [0.008, 0.008], h 0.050 | loss, 0.048 [0.048, 0.049], h 0.050 | reported, 0.043 [0.043, 0.044] | reported, 0.860 [0.745, 0.924] | loss, 2.975 (heap 2.971) **severe memory loss** | reported, 2.138 (heap 2.137) |
| 10 (n 64, rho 4) | loss, 0.047 [0.047, 0.047], h 0.050 | loss, 0.048 [0.048, 0.049], h 0.050 | reported, 0.048 [0.048, 0.049] | reported, 0.854 [0.799, 0.913] | loss, 2.070 (heap 2.068) **severe memory loss** | reported, 1.724 (heap 1.724) |
| 11 (n 64, rho 16) | loss, 0.261 [0.257, 0.264], h 0.050 | loss, 0.049 [0.049, 0.049], h 0.050 | reported, 0.050 [0.049, 0.050] | reported, 0.811 [0.748, 0.891] | loss, 1.406 (heap 1.408) | reported, 1.554 (heap 1.555) |
| 12 (n 64, rho 64) | loss, 0.755 [0.743, 0.762], h 0.050 | loss, 0.053 [0.052, 0.053], h 0.050 | reported, 0.054 [0.053, 0.054] | reported, 0.768 [0.709, 0.856] | loss, 1.270 (heap 1.277) | reported, 1.443 (heap 1.444) |

### Pack-coprime against Pack-primes (coprime against primes)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 0.071 [0.071, 0.072] | reported, 39.068 [38.962, 39.105] | reported, 27.946 [27.852, 27.976] | reported, 1.246 [1.007, 1.579] | reported, 1.147 (heap 1.148) | reported, 1.148 (heap 1.149) |
| 2 (n 16, rho 4) | reported, 0.254 [0.252, 0.255] | reported, 36.326 [36.094, 36.559] | reported, 34.332 [34.122, 34.556] | reported, 1.624 [1.457, 1.957] | reported, 1.156 (heap 1.161) | reported, 1.154 (heap 1.155) |
| 3 (n 16, rho 16) | reported, 0.678 [0.673, 0.684] | reported, 53.168 [52.793, 53.462] | reported, 52.223 [51.868, 52.518] | reported, 1.339 [1.053, 1.841] | reported, 1.107 (heap 1.115) | reported, 1.141 (heap 1.142) |
| 4 (n 16, rho 64) | reported, 0.915 [0.904, 0.931] | reported, 27.585 [27.527, 27.685] | reported, 27.259 [27.207, 27.358] | reported, 1.411 [1.276, 1.510] | reported, 1.059 (heap 1.078) | reported, 1.140 (heap 1.142) |
| 5 (n 32, rho 1) | reported, 0.032 [0.032, 0.033] | reported, 14.699 [14.644, 14.821] | reported, 11.598 [11.533, 11.700] | reported, 1.724 [1.381, 1.937] | reported, 1.108 (heap 1.108) | reported, 1.124 (heap 1.124) |
| 6 (n 32, rho 4) | reported, 0.125 [0.123, 0.127] | reported, 13.989 [13.948, 14.051] | reported, 13.485 [13.444, 13.545] | reported, 1.876 [1.304, 2.093] | reported, 1.113 (heap 1.114) | reported, 1.134 (heap 1.134) |
| 7 (n 32, rho 16) | reported, 0.465 [0.462, 0.468] | reported, 13.349 [13.314, 13.384] | reported, 13.254 [13.211, 13.289] | reported, 1.530 [1.402, 1.795] | reported, 1.108 (heap 1.113) | reported, 1.141 (heap 1.142) |
| 8 (n 32, rho 64) | reported, 0.872 [0.864, 0.881] | reported, 19.955 [19.551, 20.058] | reported, 19.877 [19.469, 19.978] | reported, 1.789 [1.402, 1.955] | reported, 1.050 (heap 1.062) | reported, 1.138 (heap 1.139) |
| 9 (n 64, rho 1) | reported, 0.020 [0.020, 0.020] | reported, 6.387 [6.362, 6.424] | reported, 5.614 [5.594, 5.646] | reported, 4.055 [3.340, 4.693] | reported, 1.072 (heap 1.073) | reported, 1.098 (heap 1.098) |
| 10 (n 64, rho 4) | reported, 0.070 [0.069, 0.076] | reported, 6.612 [6.572, 6.643] | reported, 6.475 [6.436, 6.506] | reported, 3.733 [3.304, 4.213] | reported, 1.078 (heap 1.078) | reported, 1.114 (heap 1.114) |
| 11 (n 64, rho 16) | reported, 0.309 [0.299, 0.312] | reported, 5.640 [5.613, 5.656] | reported, 5.621 [5.594, 5.636] | reported, 3.668 [3.153, 4.548] | reported, 1.057 (heap 1.058) | reported, 1.116 (heap 1.117) |
| 12 (n 64, rho 64) | reported, 0.861 [0.813, 0.869] | reported, 6.947 [6.891, 6.992] | reported, 6.938 [6.883, 6.984] | reported, 3.872 [3.194, 4.612] | reported, 1.046 (heap 1.048) | reported, 1.129 (heap 1.129) |

### Gcd work and the call-for-call checks

Hash-V-pack: G(row)/G(TC) MUST be exactly 1, and its per-operation records equal TC's, every step (a check, carried over). The factored rows (rule-m3 item 7, v0.3 §16.9 item 3): per operation the same case and the same exact divisions, multiplications and additions as TC (checked); their gcds are obtained otherwise, so G(row)/G(TC) is reported, not checked, per part (entry; the operations' additions and multiplications), beside their factor work; and each factored row takes Hash-V-pack's arithmetic counters (but the operations' gcds) and store splits (v0.3 §16.9 item 4).

| job | G per operation (TC) | Hash-V-pack: G/G(TC), call for call | Pack-primes: G/G(TC) (entry, add, mul), call for call | Pack-primes vs Hash-V-pack | Pack-coprime: G/G(TC) (entry, add, mul), call for call | Pack-coprime vs Hash-V-pack |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 41.05 | 1.000000 (passed), pass | 0.0401 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3163 (1.0000, 0.4436, 0.0000), pass (case and core arithmetic) | pass |
| 2 | 41.25 | 1.000000 (passed), pass | 0.0398 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3439 (1.0000, 0.4884, 0.0000), pass (case and core arithmetic) | pass |
| 3 | 41.12 | 1.000000 (passed), pass | 0.0398 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3074 (1.0000, 0.4346, 0.0000), pass (case and core arithmetic) | pass |
| 4 | 39.66 | 1.000000 (passed), pass | 0.0429 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.4268 (1.0000, 0.6548, 0.0000), pass (case and core arithmetic) | pass |
| 5 | 42.86 | 1.000000 (passed), pass | 0.0198 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3066 (1.0000, 0.4455, 0.0000), pass (case and core arithmetic) | pass |
| 6 | 42.22 | 1.000000 (passed), pass | 0.0200 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2865 (1.0000, 0.4124, 0.0000), pass (case and core arithmetic) | pass |
| 7 | 42.61 | 1.000000 (passed), pass | 0.0200 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3085 (1.0000, 0.4469, 0.0000), pass (case and core arithmetic) | pass |
| 8 | 41.83 | 1.000000 (passed), pass | 0.0202 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2589 (1.0000, 0.3716, 0.0000), pass (case and core arithmetic) | pass |
| 9 | 44.09 | 1.000000 (passed), pass | 0.0098 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2987 (1.0000, 0.4386, 0.0000), pass (case and core arithmetic) | pass |
| 10 | 44.15 | 1.000000 (passed), pass | 0.0098 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3011 (1.0000, 0.4429, 0.0000), pass (case and core arithmetic) | pass |
| 11 | 44.09 | 1.000000 (passed), pass | 0.0098 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2984 (1.0000, 0.4396, 0.0000), pass (case and core arithmetic) | pass |
| 12 | 44.26 | 1.000000 (passed), pass | 0.0098 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3028 (1.0000, 0.4475, 0.0000), pass (case and core arithmetic) | pass |

Factor work of the factored rows beside the gcd work (counting runs, the job's factorings and steps summed): merges and entries compared, divisibility tests, per-piece gcds, trial-division candidates, refinement gcds (pass 1 and the reconciliation).

| job | row | G(row)/G(TC) | merges | entries compared | tests | per-piece gcds | trial-division candidates | pass-1 gcds | pass-2 tests | reconciliation gcds | splits (pass 1 / reconciliation / leftover) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | 0.0401 | 11871 | 27481 | 4018 | 0 | 78138382 | 0 | 0 | 0 | 0 / 0 / 0 |
| 1 | Pack-coprime | 0.3163 | 11871 | 27481 | 3972 | 2555 | 0 | 176953 | 185306 | 3453 | 143 / 0 / 0 |
| 2 | Pack-primes | 0.0398 | 11821 | 28241 | 4373 | 0 | 71903405 | 0 | 0 | 0 | 0 / 0 / 0 |
| 2 | Pack-coprime | 0.3439 | 11821 | 28241 | 4366 | 2817 | 0 | 106061 | 112929 | 2097 | 123 / 0 / 0 |
| 3 | Pack-primes | 0.0398 | 11928 | 27470 | 3989 | 0 | 86412421 | 0 | 0 | 0 | 0 / 0 / 0 |
| 3 | Pack-coprime | 0.3074 | 11928 | 27470 | 4013 | 2473 | 0 | 67034 | 73348 | 1197 | 105 / 0 / 0 |
| 4 | Pack-primes | 0.0429 | 11825 | 27510 | 4917 | 0 | 35765483 | 0 | 0 | 0 | 0 / 0 / 0 |
| 4 | Pack-coprime | 0.4268 | 11825 | 27510 | 5051 | 3379 | 0 | 42692 | 47540 | 216 | 73 / 0 / 0 |
| 5 | Pack-primes | 0.0198 | 94731 | 223030 | 32159 | 0 | 350610862 | 0 | 0 | 0 | 0 / 0 / 0 |
| 5 | Pack-coprime | 0.3066 | 94731 | 223030 | 32735 | 20490 | 0 | 1817482 | 1870700 | 9630 | 270 / 0 / 0 |
| 6 | Pack-primes | 0.0200 | 94355 | 216770 | 29710 | 0 | 323638624 | 0 | 0 | 0 | 0 / 0 / 0 |
| 6 | Pack-coprime | 0.2865 | 94355 | 216770 | 30167 | 18923 | 0 | 1228631 | 1285708 | 6030 | 272 / 0 / 0 |
| 7 | Pack-primes | 0.0200 | 94663 | 222839 | 32333 | 0 | 298672431 | 0 | 0 | 0 | 0 / 0 / 0 |
| 7 | Pack-coprime | 0.3085 | 94663 | 222839 | 32757 | 20641 | 0 | 1082664 | 1134759 | 4413 | 254 / 0 / 0 |
| 8 | Pack-primes | 0.0202 | 94247 | 209885 | 26250 | 0 | 357297643 | 0 | 0 | 0 | 0 / 0 / 0 |
| 8 | Pack-coprime | 0.2589 | 94247 | 209885 | 27198 | 16757 | 0 | 676416 | 732004 | 1527 | 235 / 0 / 0 |
| 9 | Pack-primes | 0.0098 | 756083 | 1776056 | 248586 | 0 | 1921251356 | 0 | 0 | 0 | 0 / 0 / 0 |
| 9 | Pack-coprime | 0.2987 | 756083 | 1776056 | 256504 | 159955 | 0 | 18457116 | 18911388 | 24720 | 591 / 0 / 0 |
| 10 | Pack-primes | 0.0098 | 756425 | 1775846 | 250615 | 0 | 1998212647 | 0 | 0 | 0 | 0 / 0 / 0 |
| 10 | Pack-coprime | 0.3011 | 756425 | 1775846 | 258464 | 161654 | 0 | 15135197 | 15690046 | 16956 | 623 / 0 / 0 |
| 11 | Pack-primes | 0.0098 | 756302 | 1773818 | 251927 | 0 | 1678465368 | 0 | 0 | 0 | 0 / 0 / 0 |
| 11 | Pack-coprime | 0.2984 | 756302 | 1773818 | 259749 | 160407 | 0 | 14268061 | 14799552 | 13272 | 626 / 0 / 0 |
| 12 | Pack-primes | 0.0098 | 756568 | 1780465 | 257262 | 0 | 1907251488 | 0 | 0 | 0 | 0 / 0 / 0 |
| 12 | Pack-coprime | 0.3028 | 756568 | 1780465 | 265064 | 163057 | 0 | 12102594 | 12617026 | 9144 | 617 / 0 / 0 |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m3 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.499 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.889 | 0 |
| NONE | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.742 | 0 |
| NONE | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.215 | 0 |
| NONE | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| NONE | Pack-primes vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.098 | 2 |
| NONE | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.007 | 0 |
| NONE | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.068 | 0 |
| NONE | Pack-coprime vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.324 | 3 |
| NONE | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.422 | 0 |
| NONE | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| NONE | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.826 | 3 |
| NONE | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.014 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.077 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 3.131 | 3 |
| NONE | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.032 | 0 |
| NONE | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 14.699 | 0 |
| NONE | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.108 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.568 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.890 | 0 |
| LOW | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.374 | 0 |
| LOW | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.393 | 0 |
| LOW | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| LOW | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.761 | 0 |
| LOW | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.052 | 0 |
| LOW | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.070 | 0 |
| LOW | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.854 | 0 |
| LOW | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.672 | 0 |
| LOW | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.006 | 0 |
| LOW | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.977 | 1 |
| LOW | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.092 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.079 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.201 | 3 |
| LOW | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.125 | 0 |
| LOW | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 13.989 | 0 |
| LOW | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.113 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.625 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.888 | 0 |
| MID | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.179 | 0 |
| MID | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.538 | 0 |
| MID | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| MID | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.318 | 0 |
| MID | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.266 | 0 |
| MID | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.073 | 0 |
| MID | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.352 | 0 |
| MID | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.845 | 0 |
| MID | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.006 | 0 |
| MID | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.687 | 0 |
| MID | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.425 | 0 |
| MID | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.082 | 0 |
| MID | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.869 | 0 |
| MID | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.465 | 0 |
| MID | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 13.349 | 0 |
| MID | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.107 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.643 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.891 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.138 | 0 |
| HIGH | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.569 | 0 |
| HIGH | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| HIGH | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.171 | 0 |
| HIGH | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.490 | 0 |
| HIGH | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.090 | 0 |
| HIGH | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.180 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | build | 0 | 1 | 2 | 0 | 0 | 0 | 0.877 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.006 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.243 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.772 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.100 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.305 | 0 |
| HIGH | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.872 | 0 |
| HIGH | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 19.955 | 0 |
| HIGH | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.050 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.477 | 0.550 | 0.586 | 0.612 |
| 32 | 0.499 | 0.568 | 0.625 | 0.643 |
| 64 | 0.510 | 0.585 | 0.636 | 0.649 |

Hash-V-pack against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.850 | 0.849 | 0.848 | 0.849 |
| 32 | 0.889 | 0.890 | 0.888 | 0.891 |
| 64 | 0.902 | 0.900 | 0.903 | 0.901 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.804 | 0.297 | 0.179 | 0.139 |
| 32 | 0.742 | 0.388 | 0.168 | 0.138 |
| 64 | 0.681 | 0.374 | 0.284 | 0.136 |

Pack-primes against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.189 | 0.360 | 0.480 | 0.516 |
| 32 | 0.217 | 0.419 | 0.571 | 0.642 |
| 64 | 0.215 | 0.393 | 0.538 | 0.569 |

Pack-primes against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.003 | 0.003 | 0.002 | 0.005 |
| 32 | 0.005 | 0.005 | 0.005 | 0.004 |
| 64 | 0.007 | 0.007 | 0.008 | 0.007 |

Pack-primes against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.323 | 0.761 | 0.318 | 0.177 |
| 32 | 2.098 | 0.767 | 0.284 | 0.171 |
| 64 | 1.889 | 0.718 | 0.378 | 0.165 |

Pack-coprime against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.013 | 0.092 | 0.325 | 0.473 |
| 32 | 0.007 | 0.052 | 0.266 | 0.560 |
| 64 | 0.004 | 0.027 | 0.166 | 0.490 |

Pack-coprime against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.098 | 0.098 | 0.120 | 0.139 |
| 32 | 0.068 | 0.070 | 0.073 | 0.090 |
| 64 | 0.043 | 0.043 | 0.044 | 0.048 |

Pack-coprime against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.663 | 0.879 | 0.352 | 0.187 |
| 32 | 2.324 | 0.854 | 0.315 | 0.180 |
| 64 | 2.026 | 0.773 | 0.400 | 0.172 |

Pack-primes against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.397 | 0.655 | 0.819 | 0.843 |
| 32 | 0.435 | 0.738 | 0.914 | 0.998 |
| 64 | 0.422 | 0.672 | 0.845 | 0.877 |

Pack-primes against Hash-V-pack, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.003 | 0.003 | 0.003 | 0.006 |
| 32 | 0.005 | 0.006 | 0.006 | 0.005 |
| 64 | 0.008 | 0.007 | 0.009 | 0.008 |

Pack-primes against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.888 | 2.562 | 1.773 | 1.271 |
| 32 | 2.826 | 1.977 | 1.687 | 1.243 |
| 64 | 2.774 | 1.921 | 1.330 | 1.214 |

Pack-coprime against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.028 | 0.166 | 0.555 | 0.772 |
| 32 | 0.014 | 0.092 | 0.425 | 0.871 |
| 64 | 0.008 | 0.047 | 0.261 | 0.755 |

Pack-coprime against Hash-V-pack, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.116 | 0.115 | 0.142 | 0.164 |
| 32 | 0.077 | 0.079 | 0.082 | 0.100 |
| 64 | 0.048 | 0.048 | 0.049 | 0.053 |

Pack-coprime against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 3.312 | 2.962 | 1.962 | 1.346 |
| 32 | 3.131 | 2.201 | 1.869 | 1.305 |
| 64 | 2.975 | 2.070 | 1.406 | 1.270 |

Pack-coprime against Pack-primes, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.071 | 0.254 | 0.678 | 0.915 |
| 32 | 0.032 | 0.125 | 0.465 | 0.872 |
| 64 | 0.020 | 0.070 | 0.309 | 0.861 |

Pack-coprime against Pack-primes, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 39.068 | 36.326 | 53.168 | 27.585 |
| 32 | 14.699 | 13.989 | 13.349 | 19.955 |
| 64 | 6.387 | 6.612 | 5.640 | 6.947 |

Pack-coprime against Pack-primes, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.147 | 1.156 | 1.107 | 1.059 |
| 32 | 1.108 | 1.113 | 1.108 | 1.050 |
| 64 | 1.072 | 1.078 | 1.057 | 1.046 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (7 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build (its factoring included), over that input's cells. (f), (f'), (q) and (r) are accounted (v0.3 §16.10).

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | C's stable (build peak) | D's stable (build peak) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 1 | Hash-V-pack | 12.87 (13.38) | 12.94 (13.46) | 17.38 (17.90) | 12.78 (52.78) | 12.84 (52.84) | 12.91 (52.91) | 12.94 (52.94) |
| 1 | Pack-primes | 37.16 (37.91) | 38.67 (39.41) | 36.66 (37.40) | 36.66 (52.78) | 37.32 (57.08) | 37.17 (52.91) | 37.50 (52.94) |
| 1 | Pack-coprime | 42.61 (43.52) | 43.80 (44.69) | 42.08 (42.98) | 42.16 (52.78) | 42.70 (62.45) | 42.64 (52.91) | 42.96 (52.94) |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 2 | Hash-V-pack | 4.75 (5.27) | 6.47 (6.99) | 12.72 (13.25) | 4.75 (46.75) | 4.84 (46.84) | 4.62 (46.62) | 4.78 (46.78) |
| 2 | Pack-primes | 12.17 (12.91) | 19.03 (19.75) | 22.28 (23.03) | 12.12 (46.75) | 12.47 (46.84) | 11.79 (46.62) | 12.30 (46.78) |
| 2 | Pack-coprime | 14.07 (14.98) | 21.31 (22.22) | 25.70 (26.60) | 13.98 (46.75) | 14.41 (46.84) | 13.70 (46.62) | 14.20 (46.78) |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 3 | Hash-V-pack | 2.87 (3.41) | 4.99 (5.53) | 11.27 (11.82) | 2.88 (44.88) | 2.91 (44.91) | 2.81 (44.81) | 2.88 (44.88) |
| 3 | Pack-primes | 5.08 (5.83) | 13.18 (13.93) | 16.55 (17.30) | 5.09 (44.88) | 5.24 (44.91) | 4.78 (44.81) | 5.23 (44.88) |
| 3 | Pack-coprime | 5.63 (6.50) | 14.42 (15.28) | 18.88 (19.76) | 5.61 (44.88) | 5.86 (44.91) | 5.40 (44.81) | 5.64 (44.88) |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 4 | Hash-V-pack | 2.23 (2.73) | 4.41 (4.92) | 9.54 (10.07) | 2.25 (44.25) | 2.22 (44.22) | 2.22 (44.22) | 2.22 (44.22) |
| 4 | Pack-primes | 2.83 (3.59) | 11.46 (12.21) | 11.10 (11.87) | 2.95 (44.25) | 2.80 (44.22) | 2.77 (44.22) | 2.80 (44.22) |
| 4 | Pack-coprime | 3.00 (3.88) | 12.43 (13.29) | 12.66 (13.55) | 3.10 (44.25) | 2.98 (44.22) | 2.95 (44.22) | 2.95 (44.22) |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 5 | Hash-V-pack | 11.88 (12.01) | 12.00 (12.13) | 16.59 (16.73) | 11.85 (51.85) | 11.88 (51.88) | 11.84 (51.84) | 11.94 (51.94) |
| 5 | Pack-primes | 33.56 (33.74) | 35.29 (35.48) | 33.38 (33.57) | 33.32 (51.85) | 33.56 (51.88) | 33.60 (51.84) | 33.77 (51.94) |
| 5 | Pack-coprime | 37.19 (37.40) | 39.18 (39.40) | 37.52 (37.74) | 37.00 (51.85) | 37.16 (54.86) | 37.17 (54.63) | 37.43 (51.94) |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 6 | Hash-V-pack | 6.21 (6.34) | 7.47 (7.60) | 13.35 (13.48) | 6.20 (46.20) | 6.21 (46.21) | 6.21 (46.21) | 6.21 (46.21) |
| 6 | Pack-primes | 12.28 (12.46) | 18.34 (18.52) | 21.27 (21.45) | 12.35 (46.20) | 12.15 (46.21) | 12.29 (46.21) | 12.30 (46.21) |
| 6 | Pack-coprime | 13.66 (13.88) | 20.53 (20.75) | 24.11 (24.32) | 13.74 (46.20) | 13.58 (46.21) | 13.68 (46.21) | 13.66 (46.21) |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 7 | Hash-V-pack | 2.69 (2.82) | 4.65 (4.78) | 11.20 (11.33) | 2.69 (44.69) | 2.70 (44.70) | 2.70 (44.70) | 2.69 (44.69) |
| 7 | Pack-primes | 4.54 (4.73) | 12.07 (12.26) | 16.15 (16.34) | 4.49 (44.69) | 4.60 (44.70) | 4.57 (44.70) | 4.50 (44.69) |
| 7 | Pack-coprime | 5.04 (5.27) | 13.46 (13.69) | 18.43 (18.66) | 4.98 (44.69) | 5.11 (44.70) | 5.08 (44.70) | 4.96 (44.69) |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 8 | Hash-V-pack | 2.21 (2.34) | 4.27 (4.40) | 10.22 (10.35) | 2.21 (44.21) | 2.20 (44.20) | 2.22 (44.22) | 2.20 (44.20) |
| 8 | Pack-primes | 2.74 (2.92) | 10.82 (10.99) | 12.00 (12.18) | 2.72 (44.21) | 2.72 (44.20) | 2.80 (44.22) | 2.74 (44.20) |
| 8 | Pack-coprime | 2.88 (3.10) | 11.97 (12.19) | 13.65 (13.87) | 2.86 (44.21) | 2.88 (44.20) | 2.91 (44.22) | 2.88 (44.20) |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 9 | Hash-V-pack | 10.90 (10.93) | 11.15 (11.19) | 15.94 (15.97) | 10.86 (50.86) | 10.88 (50.88) | 10.97 (50.97) | 10.88 (50.88) |
| 9 | Pack-primes | 30.23 (30.27) | 32.40 (32.45) | 31.04 (31.08) | 30.09 (50.86) | 30.07 (50.88) | 30.57 (50.97) | 30.19 (50.88) |
| 9 | Pack-coprime | 32.41 (32.47) | 35.36 (35.41) | 34.08 (34.13) | 32.26 (50.86) | 32.28 (50.88) | 32.74 (50.97) | 32.38 (50.88) |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 10 | Hash-V-pack | 5.98 (6.01) | 7.22 (7.25) | 13.13 (13.16) | 5.97 (45.97) | 5.98 (45.98) | 5.98 (45.98) | 5.97 (45.97) |
| 10 | Pack-primes | 11.48 (11.53) | 17.39 (17.43) | 20.31 (20.36) | 11.43 (45.97) | 11.51 (45.98) | 11.50 (45.98) | 11.49 (45.97) |
| 10 | Pack-coprime | 12.37 (12.42) | 19.35 (19.40) | 22.63 (22.69) | 12.32 (45.97) | 12.41 (45.98) | 12.38 (45.98) | 12.37 (45.97) |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 11 | Hash-V-pack | 4.55 (4.58) | 6.08 (6.11) | 12.28 (12.32) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) |
| 11 | Pack-primes | 6.05 (6.10) | 13.09 (13.14) | 17.10 (17.14) | 6.08 (44.55) | 6.05 (44.55) | 6.03 (44.55) | 6.05 (44.55) |
| 11 | Pack-coprime | 6.40 (6.45) | 14.58 (14.63) | 19.09 (19.14) | 6.42 (44.55) | 6.40 (44.55) | 6.39 (44.55) | 6.39 (44.55) |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 12 | Hash-V-pack | 2.17 (2.20) | 4.17 (4.20) | 10.56 (10.59) | 2.17 (44.17) | 2.17 (44.17) | 2.17 (44.17) | 2.17 (44.17) |
| 12 | Pack-primes | 2.64 (2.68) | 10.33 (10.38) | 13.50 (13.55) | 2.64 (44.17) | 2.63 (44.17) | 2.64 (44.17) | 2.64 (44.17) |
| 12 | Pack-coprime | 2.76 (2.81) | 11.68 (11.74) | 15.24 (15.29) | 2.76 (44.17) | 2.76 (44.17) | 2.75 (44.17) | 2.76 (44.17) |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B | C | D |
| --- | --- | --- | --- | --- | --- |
| 1 | TC | 7199 | 7100 | 6862 | 6863 |
| 1 | Hash-V-pack | 14634 | 14835 | 14656 | 14456 |
| 1 | Pack-primes | 40530 | 37186 | 36296 | 34637 |
| 1 | Pack-coprime | 525383 | 509778 | 509620 | 523182 |
| 1 | Hash-V-pack-null | 14744 | 14861 | 14671 | 14555 |
| 2 | TC | 7054 | 6766 | 6608 | 6889 |
| 2 | Hash-V-pack | 12521 | 12172 | 12324 | 12474 |
| 2 | Pack-primes | 22599 | 18828 | 18366 | 18922 |
| 2 | Pack-coprime | 74718 | 73055 | 72048 | 77153 |
| 2 | Hash-V-pack-null | 12614 | 12092 | 12255 | 12606 |
| 3 | TC | 6043 | 6252 | 6344 | 5763 |
| 3 | Hash-V-pack | 10186 | 10536 | 10742 | 10225 |
| 3 | Pack-primes | 14386 | 12668 | 12874 | 12110 |
| 3 | Pack-coprime | 18772 | 18926 | 19494 | 18078 |
| 3 | Hash-V-pack-null | 10055 | 10584 | 10736 | 10197 |
| 4 | TC | 5043 | 4679 | 4227 | 5146 |
| 4 | Hash-V-pack | 8072 | 7648 | 6949 | 8399 |
| 4 | Pack-primes | 11203 | 9371 | 8355 | 9580 |
| 4 | Pack-coprime | 10590 | 10382 | 8976 | 10463 |
| 4 | Hash-V-pack-null | 8124 | 7643 | 6921 | 8333 |
| 5 | TC | 29448 | 28833 | 29253 | 29427 |
| 5 | Hash-V-pack | 59220 | 58173 | 58582 | 59108 |
| 5 | Pack-primes | 139579 | 133688 | 133667 | 135391 |
| 5 | Pack-coprime | 4306316 | 4175845 | 3963826 | 4173131 |
| 5 | Hash-V-pack-null | 59340 | 58278 | 58665 | 58896 |
| 6 | TC | 29957 | 29266 | 28878 | 28877 |
| 6 | Hash-V-pack | 51624 | 51640 | 51053 | 51072 |
| 6 | Pack-primes | 72873 | 70942 | 67460 | 68446 |
| 6 | Pack-coprime | 552194 | 588794 | 559476 | 532851 |
| 6 | Hash-V-pack-null | 51818 | 51867 | 50999 | 51392 |
| 7 | TC | 27204 | 27919 | 29446 | 28669 |
| 7 | Hash-V-pack | 44882 | 45530 | 46845 | 45206 |
| 7 | Pack-primes | 52687 | 48952 | 49871 | 48274 |
| 7 | Pack-coprime | 106355 | 106976 | 113590 | 103445 |
| 7 | Hash-V-pack-null | 44917 | 45790 | 46673 | 45307 |
| 8 | TC | 23302 | 26449 | 26702 | 25932 |
| 8 | Hash-V-pack | 39108 | 41187 | 40491 | 41048 |
| 8 | Pack-primes | 42894 | 40780 | 40838 | 40488 |
| 8 | Pack-coprime | 45775 | 47333 | 46589 | 46829 |
| 8 | Hash-V-pack-null | 38997 | 41190 | 40371 | 40853 |
| 9 | TC | 117310 | 116726 | 116342 | 116959 |
| 9 | Hash-V-pack | 228123 | 228968 | 228387 | 229317 |
| 9 | Pack-primes | 578908 | 541311 | 534198 | 529974 |
| 9 | Pack-coprime | 26977718 | 27433497 | 26968372 | 27468810 |
| 9 | Hash-V-pack-null | 228984 | 229098 | 228304 | 229169 |
| 10 | TC | 117864 | 116657 | 116504 | 116140 |
| 10 | Hash-V-pack | 199354 | 199137 | 199146 | 199900 |
| 10 | Pack-primes | 330152 | 294674 | 272688 | 278428 |
| 10 | Pack-coprime | 4373370 | 4265614 | 4201133 | 4074790 |
| 10 | Hash-V-pack-null | 199909 | 200185 | 199164 | 200120 |
| 11 | TC | 116189 | 115421 | 115158 | 116788 |
| 11 | Hash-V-pack | 181519 | 181855 | 182610 | 181934 |
| 11 | Pack-primes | 238268 | 196644 | 195812 | 216003 |
| 11 | Pack-coprime | 684119 | 698662 | 741368 | 683328 |
| 11 | Hash-V-pack-null | 181277 | 180834 | 181084 | 182820 |
| 12 | TC | 110518 | 113778 | 110700 | 119773 |
| 12 | Hash-V-pack | 171933 | 174093 | 168886 | 178708 |
| 12 | Pack-primes | 215207 | 174428 | 167858 | 199909 |
| 12 | Pack-coprime | 227606 | 230396 | 219192 | 239002 |
| 12 | Hash-V-pack-null | 170964 | 173852 | 168676 | 177266 |

## Stores, per job and matrix (rule-m3 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. T is the build table's. The factored rows' stores, lists and packings are Hash-V-pack's (checked).

| job | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | cells bytes | list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 281 | 1024 | 27.4% | 64 | 16 | 231 / 281 | 1.10 | 5 | 0 (0) | 1024 | 2248 |
| 1 | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.14 | 10 | 0 (0) | 1024 | 2264 |
| 1 | C | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.12 | 5 | 0 (0) | 1024 | 2280 |
| 1 | D | 286 | 1024 | 27.9% | 64 | 16 | 226 / 286 | 1.09 | 5 | 0 (0) | 1024 | 2288 |
| 1 | S | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.13 | 5 | 1 (1024) | 1024 | 1144 |
| 1 | U | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.10 | 6 | 1 (1024) | 1024 | 1144 |
| 1 | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.13 | 4 | 1 (1024) | 1024 | 2368 |
| 2 | A | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.02 | 2 | 0 (0) | 512 | 704 |
| 2 | B | 91 | 1024 | 8.9% | 64 | 8 | 421 / 91 | 1.03 | 2 | 0 (0) | 512 | 728 |
| 2 | C | 84 | 1024 | 8.2% | 64 | 8 | 428 / 84 | 1.04 | 2 | 0 (0) | 512 | 672 |
| 2 | D | 89 | 1024 | 8.7% | 64 | 8 | 423 / 89 | 1.02 | 2 | 0 (0) | 512 | 712 |
| 2 | S | 280 | 1024 | 27.3% | 32 | 16 | 232 / 280 | 1.12 | 5 | 1 (1024) | 1024 | 1120 |
| 2 | U | 276 | 1024 | 27.0% | 32 | 16 | 236 / 276 | 1.12 | 4 | 2 (2048) | 1024 | 1104 |
| 2 | R | 300 | 1024 | 29.3% | 64 | 16 | 212 / 300 | 1.14 | 4 | 1 (1024) | 1024 | 2400 |
| 3 | A | 28 | 1024 | 2.7% | 64 | 8 | 484 / 28 | 1.00 | 1 | 0 (0) | 512 | 224 |
| 3 | B | 29 | 1024 | 2.8% | 64 | 8 | 483 / 29 | 1.02 | 2 | 0 (0) | 512 | 232 |
| 3 | C | 26 | 1024 | 2.5% | 64 | 8 | 486 / 26 | 1.09 | 2 | 0 (0) | 512 | 208 |
| 3 | D | 28 | 1024 | 2.7% | 64 | 8 | 484 / 28 | 1.00 | 1 | 0 (0) | 512 | 224 |
| 3 | S | 192 | 1024 | 18.8% | 32 | 16 | 320 / 192 | 1.07 | 3 | 2 (2048) | 1024 | 768 |
| 3 | U | 188 | 1024 | 18.4% | 32 | 16 | 324 / 188 | 1.10 | 4 | 2 (2048) | 1024 | 752 |
| 3 | R | 304 | 1024 | 29.7% | 64 | 16 | 208 / 304 | 1.12 | 5 | 1 (1024) | 1024 | 2432 |
| 4 | A | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | B | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 512 | 56 |
| 4 | C | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 512 | 56 |
| 4 | D | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 512 | 56 |
| 4 | S | 26 | 1024 | 2.5% | 32 | 8 | 486 / 26 | 1.00 | 1 | 1 (1024) | 512 | 104 |
| 4 | U | 23 | 1024 | 2.2% | 32 | 8 | 489 / 23 | 1.00 | 1 | 1 (1024) | 512 | 92 |
| 4 | R | 292 | 1024 | 28.5% | 64 | 16 | 220 / 292 | 1.13 | 9 | 1 (1024) | 1024 | 2336 |
| 5 | A | 1005 | 4096 | 24.5% | 64 | 16 | 1043 / 1005 | 1.07 | 5 | 0 (0) | 4096 | 8040 |
| 5 | B | 1008 | 4096 | 24.6% | 64 | 16 | 1040 / 1008 | 1.06 | 5 | 0 (0) | 4096 | 8064 |
| 5 | C | 1004 | 4096 | 24.5% | 64 | 16 | 1044 / 1004 | 1.06 | 5 | 0 (0) | 4096 | 8032 |
| 5 | D | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 4096 | 8128 |
| 5 | S | 1040 | 4096 | 25.4% | 32 | 16 | 1008 / 1040 | 1.09 | 6 | 2 (8192) | 4096 | 4160 |
| 5 | U | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.10 | 7 | 2 (8192) | 4096 | 4176 |
| 5 | R | 1088 | 4096 | 26.6% | 64 | 16 | 960 / 1088 | 1.11 | 6 | 1 (4096) | 4096 | 8704 |
| 6 | A | 282 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 1.01 | 2 | 0 (0) | 4096 | 2256 |
| 6 | B | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.02 | 3 | 0 (0) | 4096 | 2264 |
| 6 | C | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 4096 | 2264 |
| 6 | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 4096 | 2264 |
| 6 | S | 1036 | 4096 | 25.3% | 32 | 16 | 1012 / 1036 | 1.11 | 6 | 2 (8192) | 4096 | 4144 |
| 6 | U | 1041 | 4096 | 25.4% | 32 | 16 | 1007 / 1041 | 1.09 | 9 | 1 (4096) | 4096 | 4164 |
| 6 | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.09 | 5 | 1 (4096) | 4096 | 8720 |
| 7 | A | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.00 | 2 | 0 (0) | 2048 | 704 |
| 7 | B | 89 | 4096 | 2.2% | 64 | 8 | 1959 / 89 | 1.03 | 2 | 0 (0) | 2048 | 712 |
| 7 | C | 90 | 4096 | 2.2% | 64 | 8 | 1958 / 90 | 1.01 | 2 | 0 (0) | 2048 | 720 |
| 7 | D | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.01 | 2 | 0 (0) | 2048 | 704 |
| 7 | S | 926 | 4096 | 22.6% | 32 | 16 | 1122 / 926 | 1.07 | 5 | 1 (4096) | 4096 | 3704 |
| 7 | U | 908 | 4096 | 22.2% | 32 | 16 | 1140 / 908 | 1.07 | 5 | 1 (4096) | 4096 | 3632 |
| 7 | R | 1084 | 4096 | 26.5% | 64 | 16 | 964 / 1084 | 1.09 | 7 | 1 (4096) | 4096 | 8672 |
| 8 | A | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 2048 | 216 |
| 8 | B | 26 | 4096 | 0.6% | 64 | 8 | 2022 / 26 | 1.00 | 1 | 0 (0) | 2048 | 208 |
| 8 | C | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 2048 | 224 |
| 8 | D | 25 | 4096 | 0.6% | 64 | 8 | 2023 / 25 | 1.00 | 1 | 0 (0) | 2048 | 200 |
| 8 | S | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.02 | 2 | 2 (8192) | 4096 | 1120 |
| 8 | U | 275 | 4096 | 6.7% | 32 | 16 | 1773 / 275 | 1.01 | 2 | 1 (4096) | 4096 | 1100 |
| 8 | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.10 | 7 | 1 (4096) | 4096 | 8720 |
| 9 | A | 3510 | 16384 | 21.4% | 64 | 16 | 4682 / 3510 | 1.03 | 4 | 0 (0) | 16384 | 28080 |
| 9 | B | 3524 | 16384 | 21.5% | 64 | 16 | 4668 / 3524 | 1.03 | 3 | 0 (0) | 16384 | 28192 |
| 9 | C | 3569 | 16384 | 21.8% | 64 | 16 | 4623 / 3569 | 1.02 | 6 | 0 (0) | 16384 | 28552 |
| 9 | D | 3522 | 16384 | 21.5% | 64 | 16 | 4670 / 3522 | 1.03 | 5 | 0 (0) | 16384 | 28176 |
| 9 | S | 3983 | 16384 | 24.3% | 32 | 16 | 4209 / 3983 | 1.07 | 6 | 2 (32768) | 16384 | 15932 |
| 9 | U | 4011 | 16384 | 24.5% | 32 | 16 | 4181 / 4011 | 1.07 | 6 | 2 (32768) | 16384 | 16044 |
| 9 | R | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.09 | 9 | 1 (16384) | 16384 | 33496 |
| 10 | A | 1011 | 16384 | 6.2% | 64 | 16 | 7181 / 1011 | 1.00 | 3 | 0 (0) | 16384 | 8088 |
| 10 | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 16384 | 8120 |
| 10 | C | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 16384 | 8104 |
| 10 | D | 1008 | 16384 | 6.2% | 64 | 16 | 7184 / 1008 | 1.01 | 2 | 0 (0) | 16384 | 8064 |
| 10 | S | 4012 | 16384 | 24.5% | 32 | 16 | 4180 / 4012 | 1.06 | 6 | 1 (16384) | 16384 | 16048 |
| 10 | U | 3989 | 16384 | 24.3% | 32 | 16 | 4203 / 3989 | 1.07 | 7 | 1 (16384) | 16384 | 15956 |
| 10 | R | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 7 | 1 (16384) | 16384 | 33488 |
| 11 | A | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 16384 | 2256 |
| 11 | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 16384 | 2248 |
| 11 | C | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 16384 | 2256 |
| 11 | D | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 1 | 0 (0) | 16384 | 2264 |
| 11 | S | 3885 | 16384 | 23.7% | 32 | 16 | 4307 / 3885 | 1.07 | 6 | 2 (32768) | 16384 | 15540 |
| 11 | U | 3889 | 16384 | 23.7% | 32 | 16 | 4303 / 3889 | 1.06 | 9 | 2 (32768) | 16384 | 15556 |
| 11 | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 16384 | 33512 |
| 12 | A | 88 | 16384 | 0.5% | 64 | 8 | 8104 / 88 | 1.00 | 1 | 0 (0) | 8192 | 704 |
| 12 | B | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 8192 | 696 |
| 12 | C | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 8192 | 696 |
| 12 | D | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 8192 | 712 |
| 12 | S | 2590 | 16384 | 15.8% | 32 | 16 | 5602 / 2590 | 1.04 | 5 | 2 (32768) | 16384 | 10360 |
| 12 | U | 2585 | 16384 | 15.8% | 32 | 16 | 5607 / 2585 | 1.04 | 5 | 1 (16384) | 16384 | 10340 |
| 12 | R | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.10 | 8 | 1 (16384) | 16384 | 33480 |

### Hash-V-pack's packings (from the counting runs)

| job | matrix | W | w_h build | w_h packed | list bytes | cells rewritten | map bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 281 | 16 | 16 | 2248 | 512 | 2048 |
| 1 | B | 283 | 16 | 16 | 2264 | 512 | 2048 |
| 1 | C | 285 | 16 | 16 | 2280 | 512 | 2048 |
| 1 | D | 286 | 16 | 16 | 2288 | 512 | 2048 |
| 1 | S | 286 | 16 | 16 | 1144 | 512 | 2048 |
| 1 | U | 286 | 16 | 16 | 1144 | 512 | 2048 |
| 1 | R | 296 | 16 | 16 | 2368 | 512 | 2048 |
| 2 | A | 88 | 16 | 8 | 704 | 512 | 2048 |
| 2 | B | 91 | 16 | 8 | 728 | 512 | 2048 |
| 2 | C | 84 | 16 | 8 | 672 | 512 | 2048 |
| 2 | D | 89 | 16 | 8 | 712 | 512 | 2048 |
| 2 | S | 280 | 16 | 16 | 1120 | 512 | 2048 |
| 2 | U | 276 | 16 | 16 | 1104 | 512 | 2048 |
| 2 | R | 300 | 16 | 16 | 2400 | 512 | 2048 |
| 3 | A | 28 | 16 | 8 | 224 | 512 | 2048 |
| 3 | B | 29 | 16 | 8 | 232 | 512 | 2048 |
| 3 | C | 26 | 16 | 8 | 208 | 512 | 2048 |
| 3 | D | 28 | 16 | 8 | 224 | 512 | 2048 |
| 3 | S | 192 | 16 | 16 | 768 | 512 | 2048 |
| 3 | U | 188 | 16 | 16 | 752 | 512 | 2048 |
| 3 | R | 304 | 16 | 16 | 2432 | 512 | 2048 |
| 4 | A | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | B | 7 | 16 | 8 | 56 | 512 | 2048 |
| 4 | C | 7 | 16 | 8 | 56 | 512 | 2048 |
| 4 | D | 7 | 16 | 8 | 56 | 512 | 2048 |
| 4 | S | 26 | 16 | 8 | 104 | 512 | 2048 |
| 4 | U | 23 | 16 | 8 | 92 | 512 | 2048 |
| 4 | R | 292 | 16 | 16 | 2336 | 512 | 2048 |
| 5 | A | 1005 | 16 | 16 | 8040 | 2048 | 8192 |
| 5 | B | 1008 | 16 | 16 | 8064 | 2048 | 8192 |
| 5 | C | 1004 | 16 | 16 | 8032 | 2048 | 8192 |
| 5 | D | 1016 | 16 | 16 | 8128 | 2048 | 8192 |
| 5 | S | 1040 | 16 | 16 | 4160 | 2048 | 8192 |
| 5 | U | 1044 | 16 | 16 | 4176 | 2048 | 8192 |
| 5 | R | 1088 | 16 | 16 | 8704 | 2048 | 8192 |
| 6 | A | 282 | 16 | 16 | 2256 | 2048 | 8192 |
| 6 | B | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | C | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | D | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | S | 1036 | 16 | 16 | 4144 | 2048 | 8192 |
| 6 | U | 1041 | 16 | 16 | 4164 | 2048 | 8192 |
| 6 | R | 1090 | 16 | 16 | 8720 | 2048 | 8192 |
| 7 | A | 88 | 16 | 8 | 704 | 2048 | 8192 |
| 7 | B | 89 | 16 | 8 | 712 | 2048 | 8192 |
| 7 | C | 90 | 16 | 8 | 720 | 2048 | 8192 |
| 7 | D | 88 | 16 | 8 | 704 | 2048 | 8192 |
| 7 | S | 926 | 16 | 16 | 3704 | 2048 | 8192 |
| 7 | U | 908 | 16 | 16 | 3632 | 2048 | 8192 |
| 7 | R | 1084 | 16 | 16 | 8672 | 2048 | 8192 |
| 8 | A | 27 | 16 | 8 | 216 | 2048 | 8192 |
| 8 | B | 26 | 16 | 8 | 208 | 2048 | 8192 |
| 8 | C | 28 | 16 | 8 | 224 | 2048 | 8192 |
| 8 | D | 25 | 16 | 8 | 200 | 2048 | 8192 |
| 8 | S | 280 | 16 | 16 | 1120 | 2048 | 8192 |
| 8 | U | 275 | 16 | 16 | 1100 | 2048 | 8192 |
| 8 | R | 1090 | 16 | 16 | 8720 | 2048 | 8192 |
| 9 | A | 3510 | 16 | 16 | 28080 | 8192 | 32768 |
| 9 | B | 3524 | 16 | 16 | 28192 | 8192 | 32768 |
| 9 | C | 3569 | 16 | 16 | 28552 | 8192 | 32768 |
| 9 | D | 3522 | 16 | 16 | 28176 | 8192 | 32768 |
| 9 | S | 3983 | 16 | 16 | 15932 | 8192 | 32768 |
| 9 | U | 4011 | 16 | 16 | 16044 | 8192 | 32768 |
| 9 | R | 4187 | 16 | 16 | 33496 | 8192 | 32768 |
| 10 | A | 1011 | 16 | 16 | 8088 | 8192 | 32768 |
| 10 | B | 1015 | 16 | 16 | 8120 | 8192 | 32768 |
| 10 | C | 1013 | 16 | 16 | 8104 | 8192 | 32768 |
| 10 | D | 1008 | 16 | 16 | 8064 | 8192 | 32768 |
| 10 | S | 4012 | 16 | 16 | 16048 | 8192 | 32768 |
| 10 | U | 3989 | 16 | 16 | 15956 | 8192 | 32768 |
| 10 | R | 4186 | 16 | 16 | 33488 | 8192 | 32768 |
| 11 | A | 282 | 16 | 16 | 2256 | 8192 | 32768 |
| 11 | B | 281 | 16 | 16 | 2248 | 8192 | 32768 |
| 11 | C | 282 | 16 | 16 | 2256 | 8192 | 32768 |
| 11 | D | 283 | 16 | 16 | 2264 | 8192 | 32768 |
| 11 | S | 3885 | 16 | 16 | 15540 | 8192 | 32768 |
| 11 | U | 3889 | 16 | 16 | 15556 | 8192 | 32768 |
| 11 | R | 4189 | 16 | 16 | 33512 | 8192 | 32768 |
| 12 | A | 88 | 16 | 8 | 704 | 8192 | 32768 |
| 12 | B | 87 | 16 | 8 | 696 | 8192 | 32768 |
| 12 | C | 87 | 16 | 8 | 696 | 8192 | 32768 |
| 12 | D | 89 | 16 | 8 | 712 | 8192 | 32768 |
| 12 | S | 2590 | 16 | 16 | 10360 | 8192 | 32768 |
| 12 | U | 2585 | 16 | 16 | 10340 | 8192 | 32768 |
| 12 | R | 4185 | 16 | 16 | 33480 | 8192 | 32768 |

### The factored rows' factorings (from the counting runs, each at its factoring)

Pack-primes: trial-division candidates, divisions with remainder and exact divisions. Pack-coprime: pass 1's scan gcds, divisions and splits (with the splits' gcds), pass 2's divisibility tests. E: (f)'s entries; |q|: the base's pieces.

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes | candidates | divisions | exact | pass-1 gcds | pass-1 divisions | splits (gcds) | pass-2 tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 281 | 554 | 6114 | 0 | 0 | 5636 | 6088 | 452 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | B | 283 | 570 | 6266 | 0 | 0 | 5538 | 5996 | 458 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | C | 285 | 563 | 6211 | 0 | 0 | 5556 | 6026 | 470 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | D | 286 | 571 | 6287 | 0 | 0 | 5099 | 5559 | 460 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | S | 286 | 584 | 4068 | 0 | 0 | 15907 | 16314 | 407 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | U | 286 | 569 | 3993 | 0 | 0 | 16049 | 16426 | 377 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | R | 296 | 763 | 8055 | 0 | 0 | 78084597 | 78085400 | 803 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-coprime | A | 281 | 554 | 6114 | 176 | 1408 | 0 | 0 | 0 | 19502 | 908 | 16 (15) | 20539 |
| 1 | Pack-coprime | B | 283 | 570 | 6266 | 172 | 1376 | 0 | 0 | 0 | 19262 | 944 | 15 (13) | 20061 |
| 1 | Pack-coprime | C | 285 | 563 | 6211 | 175 | 1400 | 0 | 0 | 0 | 19215 | 939 | 14 (12) | 20159 |
| 1 | Pack-coprime | D | 286 | 570 | 6278 | 176 | 1408 | 0 | 0 | 0 | 20054 | 940 | 16 (13) | 20847 |
| 1 | Pack-coprime | S | 286 | 578 | 4038 | 224 | 896 | 0 | 0 | 0 | 28208 | 799 | 20 (20) | 29016 |
| 1 | Pack-coprime | U | 286 | 563 | 3963 | 228 | 912 | 0 | 0 | 0 | 29872 | 751 | 19 (19) | 30737 |
| 1 | Pack-coprime | R | 296 | 606 | 6642 | 299 | 2392 | 0 | 0 | 0 | 40840 | 910 | 43 (45) | 43947 |
| 2 | Pack-primes | A | 88 | 170 | 1886 | 0 | 0 | 1338 | 1484 | 146 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | B | 91 | 176 | 1952 | 0 | 0 | 1383 | 1547 | 164 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | C | 84 | 166 | 1834 | 0 | 0 | 1312 | 1455 | 143 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | D | 89 | 174 | 1926 | 0 | 0 | 1444 | 1593 | 149 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | S | 280 | 556 | 3904 | 0 | 0 | 16154 | 16522 | 368 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | U | 276 | 549 | 3853 | 0 | 0 | 17700 | 18071 | 371 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | R | 300 | 807 | 8467 | 0 | 0 | 71864074 | 71864944 | 870 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-coprime | A | 88 | 168 | 1868 | 62 | 496 | 0 | 0 | 0 | 2229 | 263 | 11 (13) | 2604 |
| 2 | Pack-coprime | B | 91 | 176 | 1952 | 62 | 496 | 0 | 0 | 0 | 2214 | 293 | 9 (8) | 2512 |
| 2 | Pack-coprime | C | 84 | 166 | 1834 | 61 | 488 | 0 | 0 | 0 | 2215 | 254 | 9 (12) | 2464 |
| 2 | Pack-coprime | D | 89 | 171 | 1899 | 64 | 512 | 0 | 0 | 0 | 2379 | 262 | 7 (9) | 2672 |
| 2 | Pack-coprime | S | 280 | 554 | 3894 | 231 | 924 | 0 | 0 | 0 | 28622 | 718 | 20 (19) | 29743 |
| 2 | Pack-coprime | U | 276 | 542 | 3818 | 219 | 876 | 0 | 0 | 0 | 26068 | 729 | 19 (21) | 27249 |
| 2 | Pack-coprime | R | 300 | 646 | 7018 | 304 | 2432 | 0 | 0 | 0 | 42334 | 1002 | 48 (52) | 45685 |
| 3 | Pack-primes | A | 28 | 50 | 566 | 0 | 0 | 468 | 515 | 47 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | B | 29 | 53 | 597 | 0 | 0 | 377 | 421 | 44 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | C | 26 | 44 | 504 | 0 | 0 | 397 | 432 | 35 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | D | 28 | 54 | 602 | 0 | 0 | 366 | 415 | 49 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | S | 192 | 372 | 2632 | 0 | 0 | 13463 | 13734 | 271 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | U | 188 | 376 | 2636 | 0 | 0 | 13438 | 13708 | 270 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | R | 304 | 777 | 8213 | 0 | 0 | 86383912 | 86384757 | 845 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-coprime | A | 28 | 47 | 539 | 20 | 160 | 0 | 0 | 0 | 231 | 74 | 4 (5) | 310 |
| 3 | Pack-coprime | B | 29 | 51 | 579 | 22 | 176 | 0 | 0 | 0 | 231 | 74 | 4 (4) | 314 |
| 3 | Pack-coprime | C | 26 | 43 | 495 | 21 | 168 | 0 | 0 | 0 | 259 | 55 | 3 (4) | 324 |
| 3 | Pack-coprime | D | 28 | 48 | 548 | 20 | 160 | 0 | 0 | 0 | 227 | 74 | 3 (4) | 295 |
| 3 | Pack-coprime | S | 192 | 366 | 2602 | 158 | 632 | 0 | 0 | 0 | 13110 | 494 | 18 (17) | 14020 |
| 3 | Pack-coprime | U | 188 | 371 | 2611 | 158 | 632 | 0 | 0 | 0 | 12979 | 490 | 28 (27) | 14127 |
| 3 | Pack-coprime | R | 304 | 623 | 6827 | 301 | 2408 | 0 | 0 | 0 | 39997 | 975 | 45 (47) | 43958 |
| 4 | Pack-primes | A | 8 | 16 | 180 | 0 | 0 | 71 | 85 | 14 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | B | 7 | 13 | 149 | 0 | 0 | 34 | 53 | 19 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | C | 7 | 12 | 140 | 0 | 0 | 106 | 116 | 10 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | D | 7 | 13 | 149 | 0 | 0 | 132 | 140 | 8 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | S | 26 | 59 | 403 | 0 | 0 | 1162 | 1215 | 53 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | U | 23 | 47 | 331 | 0 | 0 | 929 | 963 | 34 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | R | 292 | 805 | 8417 | 0 | 0 | 35763049 | 35763746 | 697 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-coprime | A | 8 | 14 | 162 | 7 | 56 | 0 | 0 | 0 | 31 | 20 | 1 (1) | 53 |
| 4 | Pack-coprime | B | 7 | 12 | 140 | 7 | 56 | 0 | 0 | 0 | 25 | 17 | 2 (5) | 50 |
| 4 | Pack-coprime | C | 7 | 11 | 131 | 7 | 56 | 0 | 0 | 0 | 28 | 10 | 2 (4) | 44 |
| 4 | Pack-coprime | D | 7 | 11 | 131 | 7 | 56 | 0 | 0 | 0 | 27 | 8 | 3 (2) | 43 |
| 4 | Pack-coprime | S | 26 | 56 | 388 | 23 | 92 | 0 | 0 | 0 | 273 | 83 | 6 (8) | 349 |
| 4 | Pack-coprime | U | 23 | 41 | 301 | 21 | 84 | 0 | 0 | 0 | 220 | 47 | 5 (4) | 283 |
| 4 | Pack-coprime | R | 292 | 647 | 6995 | 311 | 2488 | 0 | 0 | 0 | 42088 | 825 | 54 (54) | 46718 |
| 5 | Pack-primes | A | 1005 | 1995 | 21979 | 0 | 0 | 22139 | 23540 | 1401 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | B | 1008 | 2019 | 22207 | 0 | 0 | 22049 | 23439 | 1390 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | C | 1004 | 2029 | 22281 | 0 | 0 | 21575 | 23035 | 1460 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | D | 1016 | 2032 | 22356 | 0 | 0 | 22261 | 23733 | 1472 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | S | 1040 | 2067 | 14499 | 0 | 0 | 72369 | 73658 | 1289 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | U | 1044 | 2059 | 14475 | 0 | 0 | 69218 | 70537 | 1319 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | R | 1088 | 2898 | 30438 | 0 | 0 | 350381251 | 350383578 | 2327 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-coprime | A | 1005 | 1995 | 21979 | 471 | 3768 | 0 | 0 | 0 | 164599 | 3418 | 14 (13) | 166662 |
| 5 | Pack-coprime | B | 1008 | 2019 | 22207 | 461 | 3688 | 0 | 0 | 0 | 161994 | 3469 | 16 (18) | 164122 |
| 5 | Pack-coprime | C | 1004 | 2029 | 22281 | 456 | 3648 | 0 | 0 | 0 | 151450 | 3538 | 12 (14) | 153459 |
| 5 | Pack-coprime | D | 1016 | 2032 | 22356 | 468 | 3744 | 0 | 0 | 0 | 160222 | 3543 | 13 (12) | 162351 |
| 5 | Pack-coprime | S | 1040 | 2065 | 14489 | 668 | 2672 | 0 | 0 | 0 | 268890 | 3030 | 35 (36) | 273586 |
| 5 | Pack-coprime | U | 1044 | 2058 | 14470 | 686 | 2744 | 0 | 0 | 0 | 292921 | 3018 | 31 (33) | 297099 |
| 5 | Pack-coprime | R | 1088 | 2419 | 26127 | 1171 | 9368 | 0 | 0 | 0 | 617406 | 2964 | 149 (149) | 653421 |
| 6 | Pack-primes | A | 282 | 574 | 6298 | 0 | 0 | 4988 | 5453 | 465 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | B | 283 | 550 | 6086 | 0 | 0 | 6209 | 6610 | 401 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | C | 283 | 566 | 6230 | 0 | 0 | 5310 | 5717 | 407 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | D | 283 | 567 | 6239 | 0 | 0 | 5337 | 5776 | 439 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | S | 1036 | 2103 | 14663 | 0 | 0 | 61920 | 63251 | 1331 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | U | 1041 | 2086 | 14598 | 0 | 0 | 60182 | 61558 | 1376 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | R | 1090 | 2939 | 30815 | 0 | 0 | 323494678 | 323497061 | 2383 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-coprime | A | 282 | 574 | 6298 | 177 | 1416 | 0 | 0 | 0 | 19785 | 955 | 13 (13) | 20556 |
| 6 | Pack-coprime | B | 283 | 550 | 6086 | 182 | 1456 | 0 | 0 | 0 | 20615 | 852 | 12 (13) | 21457 |
| 6 | Pack-coprime | C | 283 | 565 | 6221 | 179 | 1432 | 0 | 0 | 0 | 20455 | 877 | 11 (9) | 21115 |
| 6 | Pack-coprime | D | 283 | 567 | 6239 | 173 | 1384 | 0 | 0 | 0 | 19079 | 920 | 15 (18) | 19962 |
| 6 | Pack-coprime | S | 1036 | 2103 | 14663 | 666 | 2664 | 0 | 0 | 0 | 276624 | 3117 | 29 (30) | 280371 |
| 6 | Pack-coprime | U | 1041 | 2086 | 14598 | 633 | 2532 | 0 | 0 | 0 | 254178 | 3210 | 28 (26) | 257376 |
| 6 | Pack-coprime | R | 1090 | 2500 | 26864 | 1185 | 9480 | 0 | 0 | 0 | 617895 | 3107 | 164 (167) | 664871 |
| 7 | Pack-primes | A | 88 | 166 | 1850 | 0 | 0 | 1651 | 1797 | 146 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | B | 89 | 177 | 1953 | 0 | 0 | 1388 | 1526 | 138 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | C | 90 | 172 | 1912 | 0 | 0 | 1527 | 1670 | 143 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | D | 88 | 167 | 1859 | 0 | 0 | 1390 | 1528 | 138 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | S | 926 | 1846 | 12938 | 0 | 0 | 57409 | 58569 | 1160 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | U | 908 | 1791 | 12591 | 0 | 0 | 54777 | 55894 | 1117 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | R | 1084 | 2900 | 30440 | 0 | 0 | 298554289 | 298556606 | 2317 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-coprime | A | 88 | 164 | 1832 | 65 | 520 | 0 | 0 | 0 | 2264 | 254 | 11 (12) | 2618 |
| 7 | Pack-coprime | B | 89 | 177 | 1953 | 65 | 520 | 0 | 0 | 0 | 2425 | 266 | 10 (9) | 2698 |
| 7 | Pack-coprime | C | 90 | 171 | 1903 | 67 | 536 | 0 | 0 | 0 | 2705 | 256 | 8 (8) | 2933 |
| 7 | Pack-coprime | D | 88 | 164 | 1832 | 62 | 496 | 0 | 0 | 0 | 2204 | 250 | 10 (12) | 2541 |
| 7 | Pack-coprime | S | 926 | 1845 | 12933 | 607 | 2428 | 0 | 0 | 0 | 229114 | 2698 | 31 (33) | 233080 |
| 7 | Pack-coprime | U | 908 | 1789 | 12581 | 605 | 2420 | 0 | 0 | 0 | 227033 | 2567 | 27 (26) | 230078 |
| 7 | Pack-coprime | R | 1084 | 2415 | 26075 | 1180 | 9440 | 0 | 0 | 0 | 616919 | 2918 | 157 (157) | 660811 |
| 8 | Pack-primes | A | 27 | 45 | 517 | 0 | 0 | 258 | 299 | 41 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | B | 26 | 47 | 531 | 0 | 0 | 340 | 378 | 38 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | C | 28 | 53 | 593 | 0 | 0 | 362 | 407 | 45 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | D | 25 | 50 | 554 | 0 | 0 | 404 | 454 | 50 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | S | 280 | 582 | 4034 | 0 | 0 | 11783 | 12202 | 419 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | U | 275 | 559 | 3899 | 0 | 0 | 17781 | 18176 | 395 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | R | 1090 | 2996 | 31328 | 0 | 0 | 357266715 | 357269155 | 2440 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-coprime | A | 27 | 44 | 508 | 19 | 152 | 0 | 0 | 0 | 222 | 66 | 3 (2) | 286 |
| 8 | Pack-coprime | B | 26 | 46 | 522 | 21 | 168 | 0 | 0 | 0 | 233 | 60 | 4 (5) | 304 |
| 8 | Pack-coprime | C | 28 | 49 | 557 | 19 | 152 | 0 | 0 | 0 | 225 | 73 | 3 (4) | 288 |
| 8 | Pack-coprime | D | 25 | 48 | 536 | 20 | 160 | 0 | 0 | 0 | 228 | 74 | 4 (4) | 310 |
| 8 | Pack-coprime | S | 280 | 577 | 4009 | 203 | 812 | 0 | 0 | 0 | 24298 | 847 | 18 (15) | 25162 |
| 8 | Pack-coprime | U | 275 | 555 | 3879 | 227 | 908 | 0 | 0 | 0 | 27372 | 752 | 23 (25) | 28826 |
| 8 | Pack-coprime | R | 1090 | 2523 | 27071 | 1203 | 9624 | 0 | 0 | 0 | 623838 | 3120 | 180 (180) | 676828 |
| 9 | Pack-primes | A | 3510 | 7192 | 78772 | 0 | 0 | 78852 | 83988 | 5136 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | B | 3524 | 7164 | 78576 | 0 | 0 | 80057 | 85170 | 5113 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | C | 3569 | 7332 | 80268 | 0 | 0 | 79603 | 84818 | 5215 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | D | 3522 | 7222 | 79090 | 0 | 0 | 79023 | 84154 | 5131 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | S | 3983 | 8055 | 56211 | 0 | 0 | 264073 | 269198 | 5125 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | U | 4011 | 8049 | 56293 | 0 | 0 | 271785 | 276864 | 5079 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | R | 4187 | 11307 | 118515 | 0 | 0 | 1920397963 | 1920405920 | 7957 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-coprime | A | 3510 | 7192 | 78772 | 1113 | 8904 | 0 | 0 | 0 | 1024496 | 13503 | 15 (15) | 1030781 |
| 9 | Pack-coprime | B | 3524 | 7164 | 78576 | 1132 | 9056 | 0 | 0 | 0 | 1042302 | 13419 | 15 (14) | 1048694 |
| 9 | Pack-coprime | C | 3569 | 7332 | 80268 | 1114 | 8912 | 0 | 0 | 0 | 1030001 | 13785 | 12 (14) | 1036359 |
| 9 | Pack-coprime | D | 3522 | 7222 | 79090 | 1121 | 8968 | 0 | 0 | 0 | 1047833 | 13535 | 13 (13) | 1053985 |
| 9 | Pack-coprime | S | 3983 | 8055 | 56211 | 1847 | 7388 | 0 | 0 | 0 | 2368623 | 13401 | 27 (28) | 2377600 |
| 9 | Pack-coprime | U | 4011 | 8049 | 56293 | 1913 | 7652 | 0 | 0 | 0 | 2465962 | 13246 | 36 (35) | 2476790 |
| 9 | Pack-coprime | R | 4187 | 10004 | 106788 | 4549 | 36392 | 0 | 0 | 0 | 9477899 | 11680 | 473 (477) | 9887179 |
| 10 | Pack-primes | A | 1011 | 2032 | 22336 | 0 | 0 | 22363 | 23812 | 1449 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | B | 1015 | 2062 | 22622 | 0 | 0 | 21669 | 23149 | 1480 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | C | 1013 | 2060 | 22596 | 0 | 0 | 21406 | 22875 | 1469 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | D | 1008 | 2066 | 22630 | 0 | 0 | 21351 | 22845 | 1494 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | S | 4012 | 8022 | 56162 | 0 | 0 | 279669 | 284710 | 5041 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | U | 3989 | 8009 | 56005 | 0 | 0 | 265915 | 271046 | 5131 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | R | 4186 | 11267 | 118151 | 0 | 0 | 1997580274 | 1997588196 | 7922 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-coprime | A | 1011 | 2032 | 22336 | 458 | 3664 | 0 | 0 | 0 | 162363 | 3536 | 11 (10) | 164459 |
| 10 | Pack-coprime | B | 1015 | 2062 | 22622 | 465 | 3720 | 0 | 0 | 0 | 159691 | 3587 | 14 (15) | 161762 |
| 10 | Pack-coprime | C | 1013 | 2060 | 22596 | 453 | 3624 | 0 | 0 | 0 | 157657 | 3596 | 17 (16) | 159886 |
| 10 | Pack-coprime | D | 1008 | 2066 | 22630 | 448 | 3584 | 0 | 0 | 0 | 153169 | 3632 | 11 (14) | 155289 |
| 10 | Pack-coprime | S | 4012 | 8022 | 56162 | 1965 | 7860 | 0 | 0 | 0 | 2630170 | 13072 | 29 (27) | 2640043 |
| 10 | Pack-coprime | U | 3989 | 8009 | 56005 | 1863 | 7452 | 0 | 0 | 0 | 2406407 | 13335 | 29 (29) | 2415649 |
| 10 | Pack-coprime | R | 4186 | 10029 | 107009 | 4580 | 36640 | 0 | 0 | 0 | 9465740 | 11676 | 512 (514) | 9992958 |
| 11 | Pack-primes | A | 282 | 569 | 6253 | 0 | 0 | 5474 | 5940 | 466 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | B | 281 | 556 | 6132 | 0 | 0 | 5842 | 6251 | 409 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | C | 282 | 549 | 6073 | 0 | 0 | 5803 | 6205 | 402 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | D | 283 | 557 | 6149 | 0 | 0 | 5784 | 6220 | 436 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | S | 3885 | 7842 | 54754 | 0 | 0 | 263496 | 268436 | 4940 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | U | 3889 | 7897 | 55045 | 0 | 0 | 263191 | 268247 | 5056 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | R | 4189 | 11371 | 119099 | 0 | 0 | 1677915778 | 1677923805 | 8027 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-coprime | A | 282 | 569 | 6253 | 173 | 1384 | 0 | 0 | 0 | 19648 | 953 | 16 (15) | 20558 |
| 11 | Pack-coprime | B | 281 | 556 | 6132 | 181 | 1448 | 0 | 0 | 0 | 20419 | 870 | 15 (17) | 21306 |
| 11 | Pack-coprime | C | 282 | 549 | 6073 | 182 | 1456 | 0 | 0 | 0 | 22627 | 853 | 11 (10) | 23333 |
| 11 | Pack-coprime | D | 283 | 557 | 6149 | 173 | 1384 | 0 | 0 | 0 | 19949 | 908 | 14 (13) | 20709 |
| 11 | Pack-coprime | S | 3885 | 7842 | 54754 | 1867 | 7468 | 0 | 0 | 0 | 2410856 | 12867 | 36 (33) | 2420334 |
| 11 | Pack-coprime | U | 3889 | 7897 | 55045 | 1848 | 7392 | 0 | 0 | 0 | 2309547 | 13080 | 31 (35) | 2318420 |
| 11 | Pack-coprime | R | 4189 | 10054 | 107246 | 4568 | 36544 | 0 | 0 | 0 | 9465015 | 11752 | 503 (502) | 9974892 |
| 12 | Pack-primes | A | 88 | 174 | 1922 | 0 | 0 | 1258 | 1402 | 144 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | B | 87 | 171 | 1891 | 0 | 0 | 1519 | 1655 | 136 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | C | 87 | 173 | 1909 | 0 | 0 | 1352 | 1495 | 143 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | D | 89 | 171 | 1899 | 0 | 0 | 1803 | 1942 | 139 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | S | 2590 | 5288 | 36804 | 0 | 0 | 170240 | 173625 | 3385 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | U | 2585 | 5206 | 36374 | 0 | 0 | 194215 | 197459 | 3244 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | R | 4185 | 11312 | 118552 | 0 | 0 | 1906881101 | 1906889068 | 7967 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-coprime | A | 88 | 170 | 1886 | 64 | 512 | 0 | 0 | 0 | 2274 | 260 | 11 (10) | 2570 |
| 12 | Pack-coprime | B | 87 | 170 | 1882 | 68 | 544 | 0 | 0 | 0 | 2427 | 245 | 13 (13) | 2824 |
| 12 | Pack-coprime | C | 87 | 169 | 1873 | 62 | 496 | 0 | 0 | 0 | 2150 | 263 | 8 (8) | 2393 |
| 12 | Pack-coprime | D | 89 | 167 | 1863 | 69 | 552 | 0 | 0 | 0 | 2522 | 241 | 11 (11) | 2791 |
| 12 | Pack-coprime | S | 2590 | 5288 | 36804 | 1340 | 5360 | 0 | 0 | 0 | 1202253 | 8544 | 34 (34) | 1208976 |
| 12 | Pack-coprime | U | 2585 | 5206 | 36374 | 1445 | 5780 | 0 | 0 | 0 | 1374144 | 8107 | 26 (26) | 1381079 |
| 12 | Pack-coprime | R | 4185 | 10098 | 107626 | 4579 | 36632 | 0 | 0 | 0 | 9516824 | 11813 | 514 (516) | 10016393 |

### The factored rows' (f) and (q) at each matrix's end (after every reconciliation)

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | S | 286 | 584 | 4068 | 0 | 0 |
| 1 | Pack-primes | U | 286 | 569 | 3993 | 0 | 0 |
| 1 | Pack-primes | A | 281 | 554 | 6114 | 0 | 0 |
| 1 | Pack-primes | B | 283 | 570 | 6266 | 0 | 0 |
| 1 | Pack-primes | C | 285 | 563 | 6211 | 0 | 0 |
| 1 | Pack-primes | D | 286 | 571 | 6287 | 0 | 0 |
| 1 | Pack-primes | R | 296 | 763 | 8055 | 0 | 0 |
| 1 | Pack-coprime | S | 286 | 578 | 4038 | 224 | 896 |
| 1 | Pack-coprime | U | 286 | 563 | 3963 | 228 | 912 |
| 1 | Pack-coprime | A | 281 | 554 | 6114 | 176 | 1408 |
| 1 | Pack-coprime | B | 283 | 570 | 6266 | 172 | 1376 |
| 1 | Pack-coprime | C | 285 | 563 | 6211 | 175 | 1400 |
| 1 | Pack-coprime | D | 286 | 570 | 6278 | 176 | 1408 |
| 1 | Pack-coprime | R | 296 | 606 | 6642 | 299 | 2392 |
| 2 | Pack-primes | S | 280 | 556 | 3904 | 0 | 0 |
| 2 | Pack-primes | U | 276 | 549 | 3853 | 0 | 0 |
| 2 | Pack-primes | A | 88 | 170 | 1886 | 0 | 0 |
| 2 | Pack-primes | B | 91 | 176 | 1952 | 0 | 0 |
| 2 | Pack-primes | C | 84 | 166 | 1834 | 0 | 0 |
| 2 | Pack-primes | D | 89 | 174 | 1926 | 0 | 0 |
| 2 | Pack-primes | R | 300 | 807 | 8467 | 0 | 0 |
| 2 | Pack-coprime | S | 280 | 554 | 3894 | 231 | 924 |
| 2 | Pack-coprime | U | 276 | 542 | 3818 | 219 | 876 |
| 2 | Pack-coprime | A | 88 | 168 | 1868 | 62 | 496 |
| 2 | Pack-coprime | B | 91 | 176 | 1952 | 62 | 496 |
| 2 | Pack-coprime | C | 84 | 166 | 1834 | 61 | 488 |
| 2 | Pack-coprime | D | 89 | 171 | 1899 | 64 | 512 |
| 2 | Pack-coprime | R | 300 | 646 | 7018 | 304 | 2432 |
| 3 | Pack-primes | S | 192 | 372 | 2632 | 0 | 0 |
| 3 | Pack-primes | U | 188 | 376 | 2636 | 0 | 0 |
| 3 | Pack-primes | A | 28 | 50 | 566 | 0 | 0 |
| 3 | Pack-primes | B | 29 | 53 | 597 | 0 | 0 |
| 3 | Pack-primes | C | 26 | 44 | 504 | 0 | 0 |
| 3 | Pack-primes | D | 28 | 54 | 602 | 0 | 0 |
| 3 | Pack-primes | R | 304 | 777 | 8213 | 0 | 0 |
| 3 | Pack-coprime | S | 192 | 366 | 2602 | 158 | 632 |
| 3 | Pack-coprime | U | 188 | 371 | 2611 | 158 | 632 |
| 3 | Pack-coprime | A | 28 | 47 | 539 | 20 | 160 |
| 3 | Pack-coprime | B | 29 | 51 | 579 | 22 | 176 |
| 3 | Pack-coprime | C | 26 | 43 | 495 | 21 | 168 |
| 3 | Pack-coprime | D | 28 | 48 | 548 | 20 | 160 |
| 3 | Pack-coprime | R | 304 | 623 | 6827 | 301 | 2408 |
| 4 | Pack-primes | S | 26 | 59 | 403 | 0 | 0 |
| 4 | Pack-primes | U | 23 | 47 | 331 | 0 | 0 |
| 4 | Pack-primes | A | 8 | 16 | 180 | 0 | 0 |
| 4 | Pack-primes | B | 7 | 13 | 149 | 0 | 0 |
| 4 | Pack-primes | C | 7 | 12 | 140 | 0 | 0 |
| 4 | Pack-primes | D | 7 | 13 | 149 | 0 | 0 |
| 4 | Pack-primes | R | 292 | 805 | 8417 | 0 | 0 |
| 4 | Pack-coprime | S | 26 | 56 | 388 | 23 | 92 |
| 4 | Pack-coprime | U | 23 | 41 | 301 | 21 | 84 |
| 4 | Pack-coprime | A | 8 | 14 | 162 | 7 | 56 |
| 4 | Pack-coprime | B | 7 | 12 | 140 | 7 | 56 |
| 4 | Pack-coprime | C | 7 | 11 | 131 | 7 | 56 |
| 4 | Pack-coprime | D | 7 | 11 | 131 | 7 | 56 |
| 4 | Pack-coprime | R | 292 | 647 | 6995 | 311 | 2488 |
| 5 | Pack-primes | S | 1040 | 2067 | 14499 | 0 | 0 |
| 5 | Pack-primes | U | 1044 | 2059 | 14475 | 0 | 0 |
| 5 | Pack-primes | A | 1005 | 1995 | 21979 | 0 | 0 |
| 5 | Pack-primes | B | 1008 | 2019 | 22207 | 0 | 0 |
| 5 | Pack-primes | C | 1004 | 2029 | 22281 | 0 | 0 |
| 5 | Pack-primes | D | 1016 | 2032 | 22356 | 0 | 0 |
| 5 | Pack-primes | R | 1088 | 2898 | 30438 | 0 | 0 |
| 5 | Pack-coprime | S | 1040 | 2065 | 14489 | 668 | 2672 |
| 5 | Pack-coprime | U | 1044 | 2058 | 14470 | 686 | 2744 |
| 5 | Pack-coprime | A | 1005 | 1995 | 21979 | 471 | 3768 |
| 5 | Pack-coprime | B | 1008 | 2019 | 22207 | 461 | 3688 |
| 5 | Pack-coprime | C | 1004 | 2029 | 22281 | 456 | 3648 |
| 5 | Pack-coprime | D | 1016 | 2032 | 22356 | 468 | 3744 |
| 5 | Pack-coprime | R | 1088 | 2419 | 26127 | 1171 | 9368 |
| 6 | Pack-primes | S | 1036 | 2103 | 14663 | 0 | 0 |
| 6 | Pack-primes | U | 1041 | 2086 | 14598 | 0 | 0 |
| 6 | Pack-primes | A | 282 | 574 | 6298 | 0 | 0 |
| 6 | Pack-primes | B | 283 | 550 | 6086 | 0 | 0 |
| 6 | Pack-primes | C | 283 | 566 | 6230 | 0 | 0 |
| 6 | Pack-primes | D | 283 | 567 | 6239 | 0 | 0 |
| 6 | Pack-primes | R | 1090 | 2939 | 30815 | 0 | 0 |
| 6 | Pack-coprime | S | 1036 | 2103 | 14663 | 666 | 2664 |
| 6 | Pack-coprime | U | 1041 | 2086 | 14598 | 633 | 2532 |
| 6 | Pack-coprime | A | 282 | 574 | 6298 | 177 | 1416 |
| 6 | Pack-coprime | B | 283 | 550 | 6086 | 182 | 1456 |
| 6 | Pack-coprime | C | 283 | 565 | 6221 | 179 | 1432 |
| 6 | Pack-coprime | D | 283 | 567 | 6239 | 173 | 1384 |
| 6 | Pack-coprime | R | 1090 | 2500 | 26864 | 1185 | 9480 |
| 7 | Pack-primes | S | 926 | 1846 | 12938 | 0 | 0 |
| 7 | Pack-primes | U | 908 | 1791 | 12591 | 0 | 0 |
| 7 | Pack-primes | A | 88 | 166 | 1850 | 0 | 0 |
| 7 | Pack-primes | B | 89 | 177 | 1953 | 0 | 0 |
| 7 | Pack-primes | C | 90 | 172 | 1912 | 0 | 0 |
| 7 | Pack-primes | D | 88 | 167 | 1859 | 0 | 0 |
| 7 | Pack-primes | R | 1084 | 2900 | 30440 | 0 | 0 |
| 7 | Pack-coprime | S | 926 | 1845 | 12933 | 607 | 2428 |
| 7 | Pack-coprime | U | 908 | 1789 | 12581 | 605 | 2420 |
| 7 | Pack-coprime | A | 88 | 164 | 1832 | 65 | 520 |
| 7 | Pack-coprime | B | 89 | 177 | 1953 | 65 | 520 |
| 7 | Pack-coprime | C | 90 | 171 | 1903 | 67 | 536 |
| 7 | Pack-coprime | D | 88 | 164 | 1832 | 62 | 496 |
| 7 | Pack-coprime | R | 1084 | 2415 | 26075 | 1180 | 9440 |
| 8 | Pack-primes | S | 280 | 582 | 4034 | 0 | 0 |
| 8 | Pack-primes | U | 275 | 559 | 3899 | 0 | 0 |
| 8 | Pack-primes | A | 27 | 45 | 517 | 0 | 0 |
| 8 | Pack-primes | B | 26 | 47 | 531 | 0 | 0 |
| 8 | Pack-primes | C | 28 | 53 | 593 | 0 | 0 |
| 8 | Pack-primes | D | 25 | 50 | 554 | 0 | 0 |
| 8 | Pack-primes | R | 1090 | 2996 | 31328 | 0 | 0 |
| 8 | Pack-coprime | S | 280 | 577 | 4009 | 203 | 812 |
| 8 | Pack-coprime | U | 275 | 555 | 3879 | 227 | 908 |
| 8 | Pack-coprime | A | 27 | 44 | 508 | 19 | 152 |
| 8 | Pack-coprime | B | 26 | 46 | 522 | 21 | 168 |
| 8 | Pack-coprime | C | 28 | 49 | 557 | 19 | 152 |
| 8 | Pack-coprime | D | 25 | 48 | 536 | 20 | 160 |
| 8 | Pack-coprime | R | 1090 | 2523 | 27071 | 1203 | 9624 |
| 9 | Pack-primes | S | 3983 | 8055 | 56211 | 0 | 0 |
| 9 | Pack-primes | U | 4011 | 8049 | 56293 | 0 | 0 |
| 9 | Pack-primes | A | 3510 | 7192 | 78772 | 0 | 0 |
| 9 | Pack-primes | B | 3524 | 7164 | 78576 | 0 | 0 |
| 9 | Pack-primes | C | 3569 | 7332 | 80268 | 0 | 0 |
| 9 | Pack-primes | D | 3522 | 7222 | 79090 | 0 | 0 |
| 9 | Pack-primes | R | 4187 | 11307 | 118515 | 0 | 0 |
| 9 | Pack-coprime | S | 3983 | 8055 | 56211 | 1847 | 7388 |
| 9 | Pack-coprime | U | 4011 | 8049 | 56293 | 1913 | 7652 |
| 9 | Pack-coprime | A | 3510 | 7192 | 78772 | 1113 | 8904 |
| 9 | Pack-coprime | B | 3524 | 7164 | 78576 | 1132 | 9056 |
| 9 | Pack-coprime | C | 3569 | 7332 | 80268 | 1114 | 8912 |
| 9 | Pack-coprime | D | 3522 | 7222 | 79090 | 1121 | 8968 |
| 9 | Pack-coprime | R | 4187 | 10004 | 106788 | 4549 | 36392 |
| 10 | Pack-primes | S | 4012 | 8022 | 56162 | 0 | 0 |
| 10 | Pack-primes | U | 3989 | 8009 | 56005 | 0 | 0 |
| 10 | Pack-primes | A | 1011 | 2032 | 22336 | 0 | 0 |
| 10 | Pack-primes | B | 1015 | 2062 | 22622 | 0 | 0 |
| 10 | Pack-primes | C | 1013 | 2060 | 22596 | 0 | 0 |
| 10 | Pack-primes | D | 1008 | 2066 | 22630 | 0 | 0 |
| 10 | Pack-primes | R | 4186 | 11267 | 118151 | 0 | 0 |
| 10 | Pack-coprime | S | 4012 | 8022 | 56162 | 1965 | 7860 |
| 10 | Pack-coprime | U | 3989 | 8009 | 56005 | 1863 | 7452 |
| 10 | Pack-coprime | A | 1011 | 2032 | 22336 | 458 | 3664 |
| 10 | Pack-coprime | B | 1015 | 2062 | 22622 | 465 | 3720 |
| 10 | Pack-coprime | C | 1013 | 2060 | 22596 | 453 | 3624 |
| 10 | Pack-coprime | D | 1008 | 2066 | 22630 | 448 | 3584 |
| 10 | Pack-coprime | R | 4186 | 10029 | 107009 | 4580 | 36640 |
| 11 | Pack-primes | S | 3885 | 7842 | 54754 | 0 | 0 |
| 11 | Pack-primes | U | 3889 | 7897 | 55045 | 0 | 0 |
| 11 | Pack-primes | A | 282 | 569 | 6253 | 0 | 0 |
| 11 | Pack-primes | B | 281 | 556 | 6132 | 0 | 0 |
| 11 | Pack-primes | C | 282 | 549 | 6073 | 0 | 0 |
| 11 | Pack-primes | D | 283 | 557 | 6149 | 0 | 0 |
| 11 | Pack-primes | R | 4189 | 11371 | 119099 | 0 | 0 |
| 11 | Pack-coprime | S | 3885 | 7842 | 54754 | 1867 | 7468 |
| 11 | Pack-coprime | U | 3889 | 7897 | 55045 | 1848 | 7392 |
| 11 | Pack-coprime | A | 282 | 569 | 6253 | 173 | 1384 |
| 11 | Pack-coprime | B | 281 | 556 | 6132 | 181 | 1448 |
| 11 | Pack-coprime | C | 282 | 549 | 6073 | 182 | 1456 |
| 11 | Pack-coprime | D | 283 | 557 | 6149 | 173 | 1384 |
| 11 | Pack-coprime | R | 4189 | 10054 | 107246 | 4568 | 36544 |
| 12 | Pack-primes | S | 2590 | 5288 | 36804 | 0 | 0 |
| 12 | Pack-primes | U | 2585 | 5206 | 36374 | 0 | 0 |
| 12 | Pack-primes | A | 88 | 174 | 1922 | 0 | 0 |
| 12 | Pack-primes | B | 87 | 171 | 1891 | 0 | 0 |
| 12 | Pack-primes | C | 87 | 173 | 1909 | 0 | 0 |
| 12 | Pack-primes | D | 89 | 171 | 1899 | 0 | 0 |
| 12 | Pack-primes | R | 4185 | 11312 | 118552 | 0 | 0 |
| 12 | Pack-coprime | S | 2590 | 5288 | 36804 | 1340 | 5360 |
| 12 | Pack-coprime | U | 2585 | 5206 | 36374 | 1445 | 5780 |
| 12 | Pack-coprime | A | 88 | 170 | 1886 | 64 | 512 |
| 12 | Pack-coprime | B | 87 | 170 | 1882 | 68 | 544 |
| 12 | Pack-coprime | C | 87 | 169 | 1873 | 62 | 496 |
| 12 | Pack-coprime | D | 89 | 167 | 1863 | 69 | 552 |
| 12 | Pack-coprime | R | 4185 | 10098 | 107626 | 4579 | 36632 |

## Also reported (rule-m3 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-pack's stores, from the model; the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | distinct D | distinct S | distinct U | distinct R | W_A | W_B | W_C | W_D | W_S | W_U | W_R |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 281 | 283 | 285 | 286 | 286 | 286 | 296 |
| 2 | 16 | 4 | 64 | 64 | 64 | 64 | 64 | 250 | 247 | 256 | 88 | 91 | 84 | 89 | 280 | 276 | 300 |
| 3 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 162 | 160 | 256 | 28 | 29 | 26 | 28 | 192 | 188 | 304 |
| 4 | 16 | 64 | 4 | 4 | 4 | 4 | 4 | 16 | 16 | 256 | 8 | 7 | 7 | 7 | 26 | 23 | 292 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1022 | 1024 | 1005 | 1008 | 1004 | 1016 | 1040 | 1044 | 1088 |
| 6 | 32 | 4 | 256 | 256 | 256 | 256 | 256 | 1017 | 1017 | 1024 | 282 | 283 | 283 | 283 | 1036 | 1041 | 1090 |
| 7 | 32 | 16 | 64 | 64 | 64 | 64 | 64 | 904 | 887 | 1024 | 88 | 89 | 90 | 88 | 926 | 908 | 1084 |
| 8 | 32 | 64 | 16 | 16 | 16 | 16 | 16 | 250 | 250 | 1024 | 27 | 26 | 28 | 25 | 280 | 275 | 1090 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4096 | 4096 | 4093 | 4095 | 4096 | 3510 | 3524 | 3569 | 3522 | 3983 | 4011 | 4187 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 1024 | 1024 | 4085 | 4084 | 4096 | 1011 | 1015 | 1013 | 1008 | 4012 | 3989 | 4186 |
| 11 | 64 | 16 | 256 | 256 | 256 | 256 | 256 | 3970 | 3976 | 4096 | 282 | 281 | 282 | 283 | 3885 | 3889 | 4189 |
| 12 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 2607 | 2591 | 4096 | 88 | 87 | 87 | 89 | 2590 | 2585 | 4185 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.3 §16.9)

- job 1, S = A + B: add (256): int_both 0.016, int_x 0.113, int_y 0.125, den 0.043, general 0.703; cross tests 1024 (matched 15, cross reads 1024); S's store W 286, found / new 226 / 286
- job 1, U = C + D: add (256): int_both 0.004, int_x 0.051, int_y 0.109, den 0.039, general 0.797; cross tests 1024 (matched 11, cross reads 1024); U's store W 286, found / new 226 / 286
- job 1, R = S x U: add (4096): zero_x 0.062, int_x 0.000, int_y 0.001, den 0.006, general 0.930; mul (4096): den 0.039, int_x 0.039, int_y 0.008, general 0.914; cross tests 16384 (matched 161, cross reads 16384); R's store W 296, found / new 216 / 296
- job 2, S = A + B: add (256): int_both 0.012, int_x 0.137, int_y 0.086, den 0.016, general 0.750; cross tests 1024 (matched 7, cross reads 1024); S's store W 280, found / new 232 / 280
- job 2, U = C + D: add (256): int_both 0.020, int_x 0.105, int_y 0.102, den 0.031, general 0.742; cross tests 1024 (matched 13, cross reads 1024); U's store W 276, found / new 236 / 276
- job 2, R = S x U: add (4096): zero_x 0.062, int_x 0.000, int_y 0.002, den 0.006, general 0.929; mul (4096): int_both 0.000, den 0.041, int_x 0.015, int_y 0.039, general 0.905; cross tests 16384 (matched 169, cross reads 16384); R's store W 300, found / new 212 / 300
- job 3, S = A + B: add (256): int_y 0.145, den 0.031, general 0.824; cross tests 1024 (matched 8, cross reads 1024); S's store W 192, found / new 320 / 192
- job 3, U = C + D: add (256): int_both 0.008, int_x 0.188, int_y 0.035, den 0.055, general 0.715; cross tests 1024 (matched 16, cross reads 1024); U's store W 188, found / new 324 / 188
- job 3, R = S x U: add (4096): zero_x 0.062, int_y 0.005, den 0.007, general 0.926; mul (4096): int_both 0.000, den 0.034, int_x 0.011, int_y 0.031, general 0.924; cross tests 16384 (matched 140, cross reads 16384); R's store W 304, found / new 208 / 304
- job 4, S = A + B: add (256): int_y 0.516, general 0.484; cross tests 1024 (matched 0, cross reads 1024); S's store W 26, found / new 486 / 26
- job 4, U = C + D: add (256): int_x 0.516, general 0.484; cross tests 1024 (matched 0, cross reads 1024); U's store W 23, found / new 489 / 23
- job 4, R = S x U: add (4096): zero_x 0.062, int_x 0.001, int_y 0.011, den 0.014, general 0.911; mul (4096): den 0.042, general 0.958; cross tests 16384 (matched 173, cross reads 16384); R's store W 292, found / new 220 / 292
- job 5, S = A + B: add (1024): int_both 0.005, int_x 0.088, int_y 0.095, den 0.033, general 0.779; cross tests 4096 (matched 40, cross reads 4096); S's store W 1040, found / new 1008 / 1040
- job 5, U = C + D: add (1024): int_both 0.013, int_x 0.100, int_y 0.103, den 0.038, general 0.747; cross tests 4096 (matched 52, cross reads 4096); U's store W 1044, found / new 1004 / 1044
- job 5, R = S x U: add (32768): zero_x 0.031, int_x 0.000, int_y 0.004, den 0.006, general 0.959; mul (32768): int_both 0.000, den 0.035, num 0.000, cross_ad 0.000, int_x 0.016, int_y 0.029, general 0.920; cross tests 131072 (matched 1153, cross reads 131072); R's store W 1088, found / new 960 / 1088
- job 6, S = A + B: add (1024): int_both 0.017, int_x 0.099, int_y 0.072, den 0.040, general 0.772; cross tests 4096 (matched 58, cross reads 4096); S's store W 1036, found / new 1012 / 1036
- job 6, U = C + D: add (1024): int_both 0.021, int_x 0.117, int_y 0.097, den 0.024, general 0.741; cross tests 4096 (matched 46, cross reads 4096); U's store W 1041, found / new 1007 / 1041
- job 6, R = S x U: add (32768): zero_x 0.031, int_x 0.000, int_y 0.006, den 0.005, general 0.957; mul (32768): int_both 0.001, den 0.034, int_x 0.028, int_y 0.029, general 0.909; cross tests 131072 (matched 1133, cross reads 131072); R's store W 1090, found / new 958 / 1090
- job 7, S = A + B: add (1024): int_both 0.010, int_x 0.077, int_y 0.078, den 0.033, general 0.802; cross tests 4096 (matched 44, cross reads 4096); S's store W 926, found / new 1122 / 926
- job 7, U = C + D: add (1024): int_both 0.010, int_x 0.084, int_y 0.058, den 0.034, general 0.814; cross tests 4096 (matched 45, cross reads 4096); U's store W 908, found / new 1140 / 908
- job 7, R = S x U: add (32768): zero_x 0.031, int_x 0.000, int_y 0.006, den 0.006, general 0.957; mul (32768): int_both 0.001, den 0.035, num 0.000, cross_cb 0.000, int_x 0.028, int_y 0.020, general 0.917; cross tests 131072 (matched 1162, cross reads 131072); R's store W 1084, found / new 964 / 1084
- job 8, S = A + B: add (1024): int_both 0.009, int_x 0.054, int_y 0.178, den 0.047, general 0.713; cross tests 4096 (matched 57, cross reads 4096); S's store W 280, found / new 1768 / 280
- job 8, U = C + D: add (1024): int_both 0.009, int_x 0.119, int_y 0.054, cross 0.006, den 0.041, general 0.771; cross tests 4096 (matched 57, cross reads 4096); U's store W 275, found / new 1773 / 275
- job 8, R = S x U: add (32768): zero_x 0.031, int_both 0.000, int_x 0.000, int_y 0.011, den 0.004, general 0.953; mul (32768): int_both 0.001, den 0.030, num 0.000, int_x 0.032, int_y 0.028, general 0.908; cross tests 131072 (matched 1025, cross reads 131072); R's store W 1090, found / new 958 / 1090
- job 9, S = A + B: add (4096): int_both 0.010, int_x 0.100, int_y 0.090, cross 0.000, den 0.040, general 0.760; cross tests 16384 (matched 207, cross reads 16383); S's store W 3983, found / new 4209 / 3983
- job 9, U = C + D: add (4096): int_both 0.013, int_x 0.103, int_y 0.095, den 0.034, general 0.754; cross tests 16384 (matched 196, cross reads 16384); U's store W 4011, found / new 4181 / 4011
- job 9, R = S x U: add (262144): zero_x 0.016, int_x 0.000, int_y 0.004, den 0.005, general 0.975; mul (262144): int_both 0.000, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.022, int_y 0.022, general 0.921; cross tests 1048576 (matched 9240, cross reads 1048576); R's store W 4187, found / new 4005 / 4187
- job 10, S = A + B: add (4096): int_both 0.011, int_x 0.095, int_y 0.096, den 0.035, general 0.762; cross tests 16384 (matched 188, cross reads 16384); S's store W 4012, found / new 4180 / 4012
- job 10, U = C + D: add (4096): int_both 0.012, int_x 0.101, int_y 0.110, den 0.032, num 0.000, general 0.745; cross tests 16384 (matched 183, cross reads 16384); U's store W 3989, found / new 4203 / 3989
- job 10, R = S x U: add (262144): zero_x 0.016, int_x 0.000, int_y 0.004, den 0.005, general 0.975; mul (262144): int_both 0.001, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.019, int_y 0.022, general 0.923; cross tests 1048576 (matched 9508, cross reads 1048576); R's store W 4186, found / new 4006 / 4186
- job 11, S = A + B: add (4096): int_both 0.015, int_x 0.116, int_y 0.095, den 0.032, general 0.742; cross tests 16384 (matched 193, cross reads 16384); S's store W 3885, found / new 4307 / 3885
- job 11, U = C + D: add (4096): int_both 0.005, int_x 0.070, int_y 0.091, den 0.041, general 0.793; cross tests 16384 (matched 188, cross reads 16384); U's store W 3889, found / new 4303 / 3889
- job 11, R = S x U: add (262144): zero_x 0.016, int_x 0.000, int_y 0.004, den 0.006, general 0.974; mul (262144): int_both 0.000, den 0.036, num 0.000, cross_cb 0.000, int_x 0.024, int_y 0.017, general 0.923; cross tests 1048576 (matched 9496, cross reads 1048576); R's store W 4189, found / new 4003 / 4189
- job 12, S = A + B: add (4096): int_both 0.012, int_x 0.152, int_y 0.067, den 0.038, num 0.001, general 0.729; cross tests 16384 (matched 210, cross reads 16384); S's store W 2590, found / new 5602 / 2590
- job 12, U = C + D: add (4096): int_both 0.006, int_x 0.079, int_y 0.071, den 0.031, general 0.812; cross tests 16384 (matched 155, cross reads 16384); U's store W 2585, found / new 5607 / 2585
- job 12, R = S x U: add (262144): zero_x 0.016, int_x 0.000, int_y 0.004, den 0.006, general 0.975; mul (262144): int_both 0.000, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.025, int_y 0.015, general 0.924; cross tests 1048576 (matched 9536, cross reads 1048576); R's store W 4185, found / new 4007 / 4185

### Per step: the factored arithmetic (rule-m3 item 13; counting runs)

Reconciliation (Pack-coprime): passes, gcds, splits, |D_L| and |D_R|, (f) rebuilds (entries written). The arithmetic: merges (entries compared), the temporaries' lists (comparisons forming them), divisibility tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, and the new sub-cases (9a-c num, 10a-c general, 12a-c multiplication's general).

| job | row | step | reconciliation | merges (entries) | list comparisons | tests | per-piece gcds | factor multiplications | leftover splits | 9a / 9b / 9c | 10a / 10b / 10c | 12a / 12b / 12c |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 180 (303) | 0 | 134 | 0 | 328 | 0 | 0 / 0 / 0 | 43 / 66 / 71 | 0 / 0 / 0 |
| 1 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 204 (349) | 0 | 125 | 0 | 323 | 0 | 0 / 0 / 0 | 56 / 64 / 84 | 0 / 0 / 0 |
| 1 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 11487 (26829) | 25010 | 3759 | 0 | 27226 | 0 | 0 / 0 / 0 | 11 / 1875 / 1923 | 2498 / 1161 / 84 |
| 1 | Pack-coprime | S | 1 passes, 1044 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 180 (303) | 0 | 0 | 93 | 489 | 0 | 0 / 0 / 0 | 43 / 66 / 71 | 0 / 0 / 0 |
| 1 | Pack-coprime | U | 1 passes, 1053 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 204 (349) | 0 | 0 | 103 | 499 | 0 | 0 / 0 / 0 | 56 / 64 / 84 | 0 / 0 / 0 |
| 1 | Pack-coprime | R | 1 passes, 1356 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 11487 (26829) | 25010 | 3972 | 2359 | 34615 | 0 | 0 / 0 / 0 | 11 / 1875 / 1923 | 2498 / 1161 / 84 |
| 2 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 192 (348) | 0 | 131 | 0 | 353 | 0 | 0 / 0 / 0 | 34 / 68 / 90 | 0 / 0 / 0 |
| 2 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 190 (359) | 0 | 124 | 0 | 331 | 0 | 0 / 0 / 0 | 49 / 57 / 84 | 0 / 0 / 0 |
| 2 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 11439 (27534) | 26109 | 4118 | 0 | 28763 | 0 | 0 / 0 / 0 | 5 / 1670 / 2130 | 2682 / 956 / 69 |
| 2 | Pack-coprime | S | 1 passes, 372 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 192 (348) | 0 | 0 | 105 | 540 | 0 | 0 / 0 / 0 | 34 / 68 / 90 | 0 / 0 / 0 |
| 2 | Pack-coprime | U | 1 passes, 375 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 190 (359) | 0 | 0 | 101 | 514 | 0 | 0 / 0 / 0 | 49 / 57 / 84 | 0 / 0 / 0 |
| 2 | Pack-coprime | R | 1 passes, 1350 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 11439 (27534) | 26109 | 4366 | 2611 | 36895 | 0 | 0 / 0 / 0 | 5 / 1670 / 2130 | 2682 / 956 / 69 |
| 3 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 211 (353) | 0 | 115 | 0 | 336 | 0 | 0 / 0 / 0 | 50 / 91 / 70 | 0 / 0 / 0 |
| 3 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 183 (306) | 0 | 98 | 0 | 265 | 0 | 0 / 0 / 0 | 44 / 84 / 55 | 0 / 0 / 0 |
| 3 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 11534 (26811) | 25299 | 3776 | 0 | 27472 | 0 | 0 / 0 / 0 | 25 / 1900 / 1869 | 2449 / 1250 / 85 |
| 3 | Pack-coprime | S | 1 passes, 126 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 211 (353) | 0 | 0 | 85 | 484 | 0 | 0 / 0 / 0 | 50 / 91 / 70 | 0 / 0 / 0 |
| 3 | Pack-coprime | U | 1 passes, 123 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 183 (306) | 0 | 0 | 81 | 391 | 0 | 0 / 0 / 0 | 44 / 84 / 55 | 0 / 0 / 0 |
| 3 | Pack-coprime | R | 1 passes, 948 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 11534 (26811) | 25299 | 4013 | 2307 | 34665 | 0 | 0 / 0 / 0 | 25 / 1900 / 1869 | 2449 / 1250 / 85 |
| 4 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 124 (219) | 0 | 72 | 0 | 126 | 0 | 0 / 0 / 0 | 40 / 23 / 61 | 0 / 0 / 0 |
| 4 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 124 (286) | 0 | 140 | 0 | 288 | 0 | 0 / 0 / 0 | 0 / 0 / 124 | 0 / 0 / 0 |
| 4 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 11577 (27005) | 25002 | 4705 | 0 | 19710 | 0 | 0 / 0 / 0 | 57 / 1219 / 2455 | 1796 / 1919 / 208 |
| 4 | Pack-coprime | S | 1 passes, 42 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 124 (219) | 0 | 0 | 61 | 248 | 0 | 0 / 0 / 0 | 40 / 23 / 61 | 0 / 0 / 0 |
| 4 | Pack-coprime | U | 1 passes, 42 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 124 (286) | 0 | 0 | 140 | 499 | 0 | 0 / 0 / 0 | 0 / 0 / 124 | 0 / 0 / 0 |
| 4 | Pack-coprime | R | 1 passes, 132 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 11577 (27005) | 25002 | 5051 | 3178 | 28275 | 0 | 0 / 0 / 0 | 57 / 1219 / 2455 | 1796 / 1919 / 208 |
| 5 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 798 (1387) | 0 | 559 | 0 | 1413 | 0 | 0 / 0 / 0 | 180 / 265 / 353 | 0 / 0 / 0 |
| 5 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 765 (1303) | 0 | 498 | 0 | 1283 | 0 | 0 / 0 / 0 | 175 / 288 / 302 | 0 / 0 / 0 |
| 5 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 93168 (220340) | 207170 | 31102 | 0 | 232654 | 0 | 0 / 0 / 0 | 53 / 15163 / 16198 | 20421 / 9058 / 658 |
| 5 | Pack-coprime | S | 1 passes, 2796 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 798 (1387) | 0 | 0 | 441 | 2193 | 0 | 0 / 0 / 0 | 180 / 265 / 353 | 0 / 0 / 0 |
| 5 | Pack-coprime | U | 1 passes, 2772 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 765 (1303) | 0 | 0 | 393 | 1990 | 0 | 0 / 0 / 0 | 175 / 288 / 302 | 0 / 0 / 0 |
| 5 | Pack-coprime | R | 1 passes, 4062 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 93168 (220340) | 207170 | 32735 | 19656 | 294556 | 0 | 0 / 0 / 0 | 53 / 15163 / 16198 | 20421 / 9058 / 658 |
| 6 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 791 (1364) | 0 | 572 | 0 | 1435 | 0 | 0 / 0 / 0 | 140 / 300 / 351 | 0 / 0 / 0 |
| 6 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 759 (1272) | 0 | 470 | 0 | 1230 | 0 | 0 / 0 / 0 | 189 / 267 / 303 | 0 / 0 / 0 |
| 6 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 92805 (214134) | 199343 | 28668 | 0 | 219304 | 0 | 0 / 0 / 0 | 85 / 16211 / 15075 | 19172 / 9786 / 841 |
| 6 | Pack-coprime | S | 1 passes, 1077 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 791 (1364) | 0 | 0 | 441 | 2219 | 0 | 0 / 0 / 0 | 140 / 300 / 351 | 0 / 0 / 0 |
| 6 | Pack-coprime | U | 1 passes, 1056 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 759 (1272) | 0 | 0 | 370 | 1847 | 0 | 0 / 0 / 0 | 189 / 267 / 303 | 0 / 0 / 0 |
| 6 | Pack-coprime | R | 1 passes, 3897 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 92805 (214134) | 199343 | 30167 | 18112 | 276183 | 0 | 0 / 0 / 0 | 85 / 16211 / 15075 | 19172 / 9786 / 841 |
| 7 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 821 (1354) | 0 | 568 | 0 | 1590 | 0 | 0 / 0 / 0 | 132 / 344 / 345 | 0 / 0 / 0 |
| 7 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 834 (1546) | 0 | 599 | 0 | 1411 | 0 | 0 / 0 / 0 | 187 / 249 / 398 | 0 / 0 / 0 |
| 7 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 93008 (219939) | 208276 | 31166 | 0 | 233451 | 0 | 0 / 0 / 0 | 49 / 15189 / 16122 | 20387 / 9054 / 599 |
| 7 | Pack-coprime | S | 1 passes, 390 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 821 (1354) | 0 | 0 | 427 | 2321 | 0 | 0 / 0 / 0 | 132 / 344 / 345 | 0 / 0 / 0 |
| 7 | Pack-coprime | U | 1 passes, 387 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 834 (1546) | 0 | 0 | 505 | 2281 | 0 | 0 / 0 / 0 | 187 / 249 / 398 | 0 / 0 / 0 |
| 7 | Pack-coprime | R | 1 passes, 3636 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 93008 (219939) | 208276 | 32757 | 19709 | 295966 | 0 | 0 / 0 / 0 | 49 / 15189 / 16122 | 20387 / 9054 / 599 |
| 8 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 730 (937) | 0 | 310 | 0 | 844 | 0 | 0 / 0 / 0 | 304 / 256 / 170 | 0 / 0 / 0 |
| 8 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 790 (1329) | 0 | 456 | 0 | 1012 | 0 | 0 / 0 / 0 | 278 / 217 / 295 | 0 / 0 / 0 |
| 8 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 92727 (207619) | 198967 | 25484 | 0 | 200170 | 0 | 0 / 0 / 0 | 127 / 17548 / 13552 | 17729 / 11078 / 950 |
| 8 | Pack-coprime | S | 1 passes, 120 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 730 (937) | 0 | 0 | 224 | 1189 | 0 | 0 / 0 / 0 | 304 / 256 / 170 | 0 / 0 / 0 |
| 8 | Pack-coprime | U | 1 passes, 117 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 790 (1329) | 0 | 0 | 378 | 1704 | 0 | 0 / 0 / 0 | 278 / 217 / 295 | 0 / 0 / 0 |
| 8 | Pack-coprime | R | 1 passes, 1290 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 92727 (207619) | 198967 | 27198 | 16155 | 248256 | 0 | 0 / 0 / 0 | 127 / 17548 / 13552 | 17729 / 11078 / 950 |
| 9 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3112 (5375) | 0 | 2155 | 0 | 5382 | 0 | 0 / 0 / 0 | 681 / 1137 / 1294 | 0 / 0 / 0 |
| 9 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3089 (5223) | 0 | 2091 | 0 | 5366 | 0 | 0 / 0 / 0 | 706 / 1117 / 1266 | 0 / 0 / 0 |
| 9 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 749882 (1765458) | 1657589 | 244340 | 0 | 1879346 | 0 | 0 / 0 / 0 | 436 / 125526 / 129649 | 157479 / 77641 / 6243 |
| 9 | Pack-coprime | S | 1 passes, 6735 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3112 (5375) | 0 | 0 | 1678 | 8275 | 0 | 0 / 0 / 0 | 681 / 1137 / 1294 | 0 / 0 / 0 |
| 9 | Pack-coprime | U | 1 passes, 6705 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3089 (5223) | 0 | 0 | 1602 | 8190 | 0 | 0 / 0 / 0 | 706 / 1117 / 1266 | 0 / 0 / 0 |
| 9 | Pack-coprime | R | 1 passes, 11280 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 749882 (1765458) | 1657589 | 256504 | 156675 | 2377450 | 0 | 0 / 0 / 0 | 436 / 125526 / 129649 | 157479 / 77641 / 6243 |
| 10 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3123 (5305) | 0 | 2021 | 0 | 5276 | 0 | 0 / 0 / 0 | 734 / 1151 / 1238 | 0 / 0 / 0 |
| 10 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3051 (5245) | 0 | 2042 | 0 | 5138 | 0 | 0 / 1 / 0 | 734 / 1039 / 1277 | 0 / 0 / 0 |
| 10 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 750251 (1765296) | 1658335 | 246552 | 0 | 1891849 | 0 | 0 / 0 / 0 | 358 / 123973 / 131184 | 156167 / 79349 / 6570 |
| 10 | Pack-coprime | S | 1 passes, 2769 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3123 (5305) | 0 | 0 | 1561 | 8067 | 0 | 0 / 0 / 0 | 734 / 1151 / 1238 | 0 / 0 / 0 |
| 10 | Pack-coprime | U | 1 passes, 2703 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3051 (5245) | 0 | 0 | 1605 | 7937 | 0 | 0 / 1 / 0 | 734 / 1039 / 1277 | 0 / 0 / 0 |
| 10 | Pack-coprime | R | 1 passes, 11484 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 750251 (1765296) | 1658335 | 258464 | 158488 | 2398363 | 0 | 0 / 0 / 0 | 358 / 123973 / 131184 | 156167 / 79349 / 6570 |
| 11 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3040 (5173) | 0 | 2017 | 0 | 5294 | 0 | 0 / 0 / 0 | 659 / 1147 / 1234 | 0 / 0 / 0 |
| 11 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3248 (5498) | 0 | 2166 | 0 | 5601 | 0 | 0 / 0 / 0 | 783 / 1194 / 1271 | 0 / 0 / 0 |
| 11 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 750014 (1763147) | 1660241 | 247744 | 0 | 1898620 | 0 | 0 / 0 / 0 | 356 / 125881 / 129173 | 158294 / 77483 / 6173 |
| 11 | Pack-coprime | S | 1 passes, 1062 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3040 (5173) | 0 | 0 | 1561 | 7994 | 0 | 0 / 0 / 0 | 659 / 1147 / 1234 | 0 / 0 / 0 |
| 11 | Pack-coprime | U | 1 passes, 1065 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3248 (5498) | 0 | 0 | 1645 | 8554 | 0 | 0 / 0 / 0 | 783 / 1194 / 1271 | 0 / 0 / 0 |
| 11 | Pack-coprime | R | 1 passes, 11145 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 750014 (1763147) | 1660241 | 259749 | 157201 | 2401101 | 0 | 0 / 0 / 0 | 356 / 125881 / 129173 | 158294 / 77483 / 6173 |
| 12 | Pack-primes | S | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 2989 (5297) | 0 | 2074 | 0 | 5004 | 0 | 0 / 0 / 3 | 725 / 968 / 1293 | 0 / 0 / 0 |
| 12 | Pack-primes | U | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3327 (5719) | 0 | 2276 | 0 | 6071 | 0 | 0 / 0 / 0 | 654 / 1316 / 1357 | 0 / 0 / 0 |
| 12 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 750252 (1769449) | 1682058 | 252912 | 0 | 1909156 | 0 | 0 / 0 / 0 | 368 / 123335 / 131796 | 158586 / 77338 / 6214 |
| 12 | Pack-coprime | S | 1 passes, 396 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 2989 (5297) | 0 | 0 | 1622 | 7813 | 0 | 0 / 0 / 3 | 725 / 968 / 1293 | 0 / 0 / 0 |
| 12 | Pack-coprime | U | 1 passes, 393 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3327 (5719) | 0 | 0 | 1740 | 9164 | 0 | 0 / 0 / 0 | 654 / 1316 / 1357 | 0 / 0 / 0 |
| 12 | Pack-coprime | R | 1 passes, 8355 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 750252 (1769449) | 1682058 | 265064 | 159695 | 2421512 | 0 | 0 / 0 / 0 | 368 / 123335 / 131796 | 158586 / 77338 / 6214 |

The gates checked every counter and store figure against the model (spec v0.3 §16.9 item 2), the TC gate included: MHVP:1 pass, TC:1 pass, MHPC:1 pass, TC:2 pass, MHVP:2 pass, MHPP:1 pass, MHPC:2 pass, TC:3 pass, MHVP:3 pass, MHPP:2 pass, MHPC:3 pass, TC:4 pass, MHVP:4 pass, MHPP:4 pass, MHPC:4 pass, TC:5 pass, MHVP:5 pass, MHPP:3 pass, MHPC:5 pass, TC:6 pass, MHVP:6 pass, MHPP:5 pass, MHPC:6 pass, TC:7 pass, MHPP:6 pass, MHVP:7 pass, MHPC:7 pass, TC:8 pass, MHVP:8 pass, MHPP:7 pass, MHPC:8 pass, TC:9 pass, MHPP:8 pass, MHVP:9 pass, MHPC:9 pass, TC:10 pass, MHVP:10 pass, MHPP:9 pass, MHPC:10 pass, TC:11 pass, MHVP:11 pass, MHPP:10 pass, MHPC:11 pass, TC:12 pass, MHVP:12 pass, MHPP:11 pass, MHPC:12 pass, MHPP:12 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m3 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 3.707 | no | 2 | 14 | 0.143 | 14 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 8.048 | no | 2 | 14 | 0.143 | 15 | 1 | none | 14 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 2 (6->7 threads), bash 0 (1->1 threads) |
| 10 | pilot | 1 | 23.176 | no | 1 | 14 | 0.071 | 15 | 1 | none | 29 | none | process_api 1 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 2 (7->7 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 19.267 | no | 19 | 64 | 0.297 | 65 | 1 | none | 25 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 1/15 (8->7 threads), bash 0/7 (1->1 threads) |
| 2 | measured | 1 | 19.861 | no | 19 | 64 | 0.297 | 65 | 1 | none | 25 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 2/15 (7->6 threads), bash 0/7 (1->1 threads) |
| 3 | measured | 1 | 20.355 | no | 19 | 64 | 0.297 | 66 | 2 | none | 26 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/7 (8->8 threads), claude 3/16 (7->8 threads), bash 0/7 (1->1 threads) |
| 4 | measured | 1 | 14.166 | no | 21 | 64 | 0.328 | 65 | 1 | none | 20 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 1/13 (8->7 threads), bash 0/7 (1->1 threads) |
| 5 | measured | 1 | 46.358 | no | 20 | 64 | 0.312 | 66 | 2 | none | 52 | none | process_api 2/10 (6->6 threads), sh 0/10 (1->1 threads), environment-man 0/10 (8->8 threads), claude 4/29 (8->7 threads), bash 0/10 (1->1 threads) |
| 6 | measured | 1 | 43.02 | no | 19 | 64 | 0.297 | 68 | 4 | none | 49 | none | process_api 1/9 (6->6 threads), sh 0/9 (1->1 threads), environment-man 1/9 (8->8 threads), claude 4/27 (7->8 threads), bash 0/9 (1->1 threads) |
| 7 | measured | 1 | 40.275 | no | 19 | 64 | 0.297 | 66 | 2 | none | 46 | none | process_api 0/9 (6->6 threads), sh 0/9 (1->1 threads), environment-man 1/9 (8->8 threads), claude 5/26 (8->7 threads), bash 0/9 (1->1 threads) |
| 8 | measured | 1 | 47.36 | no | 20 | 64 | 0.312 | 68 | 4 | none | 53 | none | process_api 2/10 (6->6 threads), sh 0/10 (1->1 threads), environment-man 1/10 (8->8 threads), claude 4/29 (7->8 threads), bash 0/10 (1->1 threads) |
| 9 | measured | 1 | 108.255 | no | 20 | 64 | 0.312 | 71 | 7 | none | 114 | none | process_api 4/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 2/15 (8->8 threads), claude 15/59 (8->7 threads), bash 0/15 (1->1 threads) |
| 10 | measured | 1 | 107.542 | no | 20 | 64 | 0.312 | 72 | 8 | none | 113 | none | process_api 2/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 2/15 (8->8 threads), claude 11/59 (7->7 threads), bash 0/15 (1->1 threads) |
| 11 | measured | 1 | 95.581 | no | 20 | 64 | 0.312 | 68 | 4 | none | 101 | none | process_api 3/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 1/14 (8->8 threads), claude 10/53 (7->7 threads), bash 0/14 (1->1 threads) |
| 12 | measured | 1 | 101.248 | no | 20 | 64 | 0.312 | 71 | 7 | none | 107 | none | process_api 3/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 2/14 (8->8 threads), claude 10/56 (7->8 threads), bash 0/14 (1->1 threads) |

The runner's own ticks over the 15 blocks: 241 ticks over 810 launches, 0.298 per launch; the highest block 0.328 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 1.0000 | 0.9301 | 539065 | 538777 |
| 1 | Hash-V-pack | 12 | 0.9999 | 0.9932 | 660337 | 660127 |
| 1 | Pack-primes | 12 | 0.9999 | 0.9901 | 202907800 | 202947404 |
| 1 | Pack-coprime | 12 | 0.9999 | 0.9977 | 7262260 | 7262238 |
| 1 | Hash-V-pack-null | 12 | 0.9998 | 0.9888 | 660969 | 660627 |
| 2 | TC | 12 | 0.9992 | 0.9966 | 534626 | 534667 |
| 2 | Hash-V-pack | 12 | 0.9996 | 0.9754 | 647189 | 646747 |
| 2 | Pack-primes | 12 | 0.9985 | 0.9860 | 187247452 | 187902386 |
| 2 | Pack-coprime | 12 | 0.9999 | 0.9961 | 5465724 | 5473148 |
| 2 | Hash-V-pack-null | 12 | 0.9986 | 0.9898 | 648238 | 649847 |
| 3 | TC | 12 | 0.9997 | 0.9585 | 534467 | 534268 |
| 3 | Hash-V-pack | 12 | 0.9994 | 0.9753 | 642401 | 642876 |
| 3 | Pack-primes | 12 | 0.9995 | 0.9929 | 224522496 | 224796508 |
| 3 | Pack-coprime | 12 | 0.9996 | 0.9892 | 4299594 | 4304572 |
| 3 | Hash-V-pack-null | 12 | 0.9995 | 0.9901 | 640955 | 640518 |
| 4 | TC | 12 | 1.0000 | 0.9853 | 489072 | 488739 |
| 4 | Hash-V-pack | 12 | 0.9993 | 0.9806 | 584317 | 584151 |
| 4 | Pack-primes | 12 | 0.9996 | 0.9967 | 93092084 | 93145516 |
| 4 | Pack-coprime | 12 | 0.9999 | 0.9948 | 3414853 | 3417022 |
| 4 | Hash-V-pack-null | 12 | 0.9997 | 0.9987 | 584236 | 583803 |
| 5 | TC | 12 | 0.9998 | 0.9969 | 4373313 | 4375474 |
| 5 | Hash-V-pack | 12 | 0.9995 | 0.9830 | 5025299 | 5025589 |
| 5 | Pack-primes | 12 | 0.9984 | 0.9838 | 915246650 | 917212555 |
| 5 | Pack-coprime | 12 | 0.9996 | 0.9892 | 79076898 | 79085646 |
| 5 | Hash-V-pack-null | 12 | 0.9999 | 0.9985 | 5023533 | 5023200 |
| 6 | TC | 12 | 0.9998 | 0.9922 | 4349628 | 4356114 |
| 6 | Hash-V-pack | 12 | 0.9997 | 0.9976 | 4963811 | 4970815 |
| 6 | Pack-primes | 12 | 0.9978 | 0.9876 | 846294899 | 848055433 |
| 6 | Pack-coprime | 12 | 0.9998 | 0.9496 | 62889612 | 62887782 |
| 6 | Hash-V-pack-null | 12 | 0.9999 | 0.9977 | 4970061 | 4973516 |
| 7 | TC | 12 | 0.9997 | 0.9858 | 4357899 | 4357751 |
| 7 | Hash-V-pack | 12 | 0.9995 | 0.9835 | 4959029 | 4964299 |
| 7 | Pack-primes | 12 | 0.9995 | 0.9939 | 778705828 | 779426632 |
| 7 | Pack-coprime | 12 | 0.9997 | 0.9939 | 58724400 | 58807233 |
| 7 | Hash-V-pack-null | 12 | 0.9991 | 0.9910 | 4972365 | 4988357 |
| 8 | TC | 12 | 1.0000 | 0.9969 | 4298949 | 4298986 |
| 8 | Hash-V-pack | 12 | 0.9996 | 0.9964 | 4867742 | 4870774 |
| 8 | Pack-primes | 12 | 0.9979 | 0.9934 | 932766656 | 934987406 |
| 8 | Pack-coprime | 12 | 0.9988 | 0.9803 | 46919268 | 47039395 |
| 8 | Hash-V-pack-null | 12 | 0.9996 | 0.9949 | 4863530 | 4868296 |
| 9 | TC | 12 | 0.9999 | 0.9971 | 34847938 | 34852610 |
| 9 | Hash-V-pack | 12 | 0.9998 | 0.9929 | 39022398 | 39036267 |
| 9 | Pack-primes | 12 | 0.9955 | 0.9796 | 5027497016 | 5054187017 |
| 9 | Pack-coprime | 12 | 0.9995 | 0.9960 | 898104742 | 900271926 |
| 9 | Hash-V-pack-null | 12 | 0.9999 | 0.9920 | 39005114 | 39027310 |
| 10 | TC | 12 | 1.0000 | 0.9939 | 34897934 | 34959717 |
| 10 | Hash-V-pack | 12 | 0.9991 | 0.9652 | 39046779 | 39121244 |
| 10 | Pack-primes | 12 | 0.9954 | 0.9864 | 5222914388 | 5251989741 |
| 10 | Pack-coprime | 12 | 0.9995 | 0.9911 | 810861463 | 811100162 |
| 10 | Hash-V-pack-null | 12 | 0.9994 | 0.9894 | 38994056 | 39044185 |
| 11 | TC | 12 | 0.9989 | 0.9771 | 34915136 | 35009511 |
| 11 | Hash-V-pack | 12 | 0.9997 | 0.9875 | 38982067 | 38992081 |
| 11 | Pack-primes | 12 | 0.9968 | 0.9905 | 4392265204 | 4410095424 |
| 11 | Pack-coprime | 12 | 0.9989 | 0.9947 | 783650382 | 784601843 |
| 11 | Hash-V-pack-null | 12 | 0.9999 | 0.9953 | 38894750 | 38906558 |
| 12 | TC | 12 | 0.9997 | 0.9882 | 34943574 | 34950643 |
| 12 | Hash-V-pack | 12 | 0.9999 | 0.9854 | 38997496 | 39006788 |
| 12 | Pack-primes | 12 | 0.9932 | 0.9806 | 4988336122 | 5023232554 |
| 12 | Pack-coprime | 12 | 0.9990 | 0.9873 | 720268204 | 723974514 |
| 12 | Hash-V-pack-null | 12 | 1.0000 | 0.9943 | 38979612 | 38985186 |

Idle checks: 15 (0 busy). Host rates R_h: {'1': 0.04314808422506041, '79': 0.04314808422506041, '84': 0.04314808422506041, '92': 0.24850111016934787, '22980': 0.04314808422506041} (measured: {'1': 0.04314780336592122, '79': 0.0, '84': 0.0, '92': 0.24850111016934787, '22980': 0.0}; R_h is at least 1 tick over the longest pilot block).

### Earlier rounds, for reference only

Round m2's runs, [`../results-m2-add/results.md`](../results-m2-add/results.md), [`../results-m2-mul/results.md`](../results-m2-mul/results.md), [`../results-m2-sop/results.md`](../results-m2-sop/results.md) and [`../results-m2-pos/results.md`](../results-m2-pos/results.md); m1's, [`../results-m1-add/results.md`](../results-m1-add/results.md) and [`../results-m1-mul/results.md`](../results-m1-mul/results.md); and round 3's, [`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They enter no measure here.

## Limitations

- **No n = 128 in any row or set, and no rho = 16 in sop.** The step from 64 to 128, where m2 saw the handle width change, is not measured.
- **Pack-primes is deliberately unoptimized.** Its times measure schoolbook trial division, by the author's design, not factoring in general. A fast factorizer is a later, separate change. The aim is a schoolbook baseline: find the weak points first, then optimize systematically.
- **Pack-coprime refines eagerly, per matrix**, the form 09 found costly: 58-161 times the control's gcd work there (09 findings, lesson 1). Its reconciliation, by contrast, is on demand and limited to denominator factors (v0.3 §16.7).
- **The factored rows keep TC's value arithmetic.** They measure only what factored gcds save and what factoring costs, not value shortcuts the factors would allow (v0.3 §16.5).
- **On this workload every denominator is {2, 3, 5}-smooth**, so the denominators' factorizations are short, and reconciliation is small. Workloads with richer denominators would change both.
- **The factoring time is not separated from the operation's time.** The counters separate the work.
- **Results are reported per run**, and not combined across runs or with m1 and m2.
- Carried over from rule-m2, without Hash-V-fixed and Hash-V-grow-map: textbook stores (open addressing with linear probing); Hash-V-pack's build table is a fixed table; the expression sets reuse one value distribution; 64 bits only; square matrices; the disturbance check cannot see every process.

