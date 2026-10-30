# Design 3 on matrices, round m3 (factored whole numbers), matrix addition: results

Run started 2026-10-30T14:35:49-05:00.

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

- The add run (job set `add`: C = A + B).
- Started 2026-10-30T14:35:49-05:00; finished 2026-10-30T14:40:41-05:00.
- Step 0 (before the clock): 3.4 s; step 0's files: equal to m1's by sha256.
- Run time 4.8 minutes (hard stop 40).
- Rule sha256 `c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd`; round 3's results after the run: unchanged; m1's and m2's results and the other m3 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[19297], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T14:35:52-05:00: TC `3cbbe6f47fb7`, MHVP `7ed2109b9193`, MHPP `6704a1ab9994`, MHPC `0da57d01e10e`, TC3GATE `685595cafdb3`
- Unit tests: m3_unit: m3_unit: 811293 checks, 0 failures; m3_diff: m3_diff: 3000 seeds, 3000 jobs, 1050092 operations; obtained gcds checked against Euclid 2936692, merges 2186690, reconciliation splits 2796, leftover splits 106; 1523395 checks, 0 failures; m3_alloc-timing: m3_alloc (timing build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-counting: m3_alloc (counting build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-memory: m3_alloc (memory build): 160 jobs, 2893 moments, 43830 checks, 0 failures; m3_unit-asan: m3_unit: 811293 checks, 0 failures; m3_diff-asan: m3_diff: 600 seeds, 600 jobs, 209812 operations; obtained gcds checked against Euclid 579628, merges 431634, reconciliation splits 557, leftover splits 20; 303709 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 10], one per size; block wall times job 1 4.4 s, job 6 4.9 s, job 10 5.1 s.
- Per size class c (its pilot job): O_c the block's checks, K the calibration launches, P the two round launches (0 over cap), t the time per run from the calibration; N_c the run's jobs of that size:
  - size 0 (job 1, N_c = 4): O 2.0 s; TC: K 0.13 s, P 0.27 s, t 0.000 s; Hash-V-pack: K 0.13 s, P 0.35 s, t 0.000 s; Pack-primes: K 0.13 s, P 0.39 s, t 0.000 s; Pack-coprime: K 0.14 s, P 0.42 s, t 0.002 s; Hash-V-pack-null: K 0.00 s, P 0.38 s, t n/a s
  - size 1 (job 6, N_c = 4): O 2.0 s; TC: K 0.19 s, P 0.50 s, t 0.000 s; Hash-V-pack: K 0.17 s, P 0.49 s, t 0.000 s; Pack-primes: K 0.11 s, P 0.32 s, t 0.000 s; Pack-coprime: K 0.15 s, P 0.47 s, t 0.010 s; Hash-V-pack-null: K 0.00 s, P 0.49 s, t n/a s
  - size 2 (job 10, N_c = 4): O 2.0 s; TC: K 0.19 s, P 0.57 s, t 0.000 s; Hash-V-pack: K 0.19 s, P 0.54 s, t 0.001 s; Pack-primes: K 0.11 s, P 0.32 s, t 0.002 s; Pack-coprime: K 0.09 s, P 0.53 s, t 0.083 s; Hash-V-pack-null: K 0.00 s, P 0.54 s, t n/a s
- Projection elapsed + 1.2 [sum_c N_c (O_c + sum_rows (K + 6 P)) + 1/2 sum_c N_c sum_rows 4 t] = 5.2 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.32499187520311995, '79': 0.32499187520311995, '84': 0.32499187520311995, '92': 0.42738836610428654, '19297': 0.32499187520311995}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 2 addition time; 6 stable bytes of one stored input matrix; 7 peak bytes during the operation. Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-pack against Hash-V-pack-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses; (f) and (q) included). Pack-coprime against Pack-primes is reported with no verdict on any measure. A row over cap runs no rounds: its time verdicts are over cap, with its calibration rep's time; its memory verdicts stand. Gcd work: a check for Hash-V-pack, reported for the factored rows. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-pack, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m3 item 8, carried over).

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.396 [0.391, 0.402], h 0.050 | loss, 0.381 [0.378, 0.383], h 0.050 | reported, 0.394 [0.389, 0.397] | reported, 0.577 [0.563, 0.736] | win, 0.806 (heap 0.822) | loss, 1.213 (heap 1.223) |
| 2 (n 16, rho 4) | loss, 0.426 [0.418, 0.430], h 0.050 | loss, 0.366 [0.364, 0.368], h 0.050 | reported, 0.406 [0.400, 0.408] | reported, 0.576 [0.572, 0.654] | win, 0.294 (heap 0.320) | win, 0.871 (heap 0.887) |
| 3 (n 16, rho 16) | loss, 0.475 [0.469, 0.477], h 0.050 | loss, 0.415 [0.411, 0.417], h 0.050 | reported, 0.452 [0.448, 0.454] | reported, 0.595 [0.571, 0.740] | win, 0.178 (heap 0.205) | win, 0.761 (heap 0.778) |
| 4 (n 16, rho 64) | loss, 0.504 [0.500, 0.516], h 0.050 | loss, 0.482 [0.477, 0.485], h 0.050 | reported, 0.506 [0.501, 0.509] | reported, 0.577 [0.574, 0.584] | win, 0.140 (heap 0.169) | win, 0.685 (heap 0.704) |
| 5 (n 32, rho 1) | loss, 0.488 [0.471, 0.495], h 0.050 | loss, 0.419 [0.409, 0.428], h 0.050 | reported, 0.464 [0.453, 0.471] | reported, 0.753 [0.589, 0.818] | win, 0.747 (heap 0.751) | loss, 1.166 (heap 1.169) |
| 6 (n 32, rho 4) | loss, 0.564 [0.552, 0.567], h 0.050 | loss, 0.451 [0.448, 0.454], h 0.050 | reported, 0.520 [0.512, 0.524] | reported, 0.628 [0.592, 0.733] | win, 0.388 (heap 0.395) | tie, 0.926 (heap 0.930) |
| 7 (n 32, rho 16) | loss, 0.624 [0.611, 0.634], h 0.050 | loss, 0.475 [0.471, 0.481], h 0.050 | reported, 0.559 [0.553, 0.566] | reported, 0.591 [0.583, 0.643] | win, 0.168 (heap 0.175) | win, 0.771 (heap 0.775) |
| 8 (n 32, rho 64) | loss, 0.576 [0.565, 0.584], h 0.050 | loss, 0.415 [0.407, 0.418], h 0.050 | reported, 0.507 [0.504, 0.512] | reported, 0.605 [0.588, 0.802] | win, 0.138 (heap 0.145) | win, 0.698 (heap 0.703) |
| 9 (n 64, rho 1) | loss, 0.502 [0.499, 0.505], h 0.050 | loss, 0.459 [0.457, 0.462], h 0.050 | reported, 0.487 [0.485, 0.488] | reported, 0.641 [0.609, 0.825] | win, 0.683 (heap 0.685) | loss, 1.121 (heap 1.121) |
| 10 (n 64, rho 4) | loss, 0.582 [0.579, 0.584], h 0.050 | loss, 0.464 [0.463, 0.469], h 0.050 | reported, 0.536 [0.532, 0.539] | reported, 0.649 [0.621, 0.762] | win, 0.373 (heap 0.375) | tie, 0.913 (heap 0.914) |
| 11 (n 64, rho 16) | loss, 0.636 [0.633, 0.639], h 0.050 | loss, 0.459 [0.456, 0.462], h 0.050 | reported, 0.561 [0.556, 0.563] | reported, 0.637 [0.614, 0.834] | win, 0.284 (heap 0.286) | win, 0.852 (heap 0.853) |
| 12 (n 64, rho 64) | loss, 0.650 [0.649, 0.658], h 0.050 | loss, 0.473 [0.472, 0.476], h 0.050 | reported, 0.574 [0.571, 0.575] | reported, 0.622 [0.615, 0.647] | win, 0.136 (heap 0.138) | win, 0.726 (heap 0.728) |

### Pack-primes against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.098 [0.097, 0.098], h 0.050 | loss, 0.067 [0.067, 0.068], h 0.050 | reported, 0.084 [0.084, 0.085] | reported, 0.529 [0.523, 0.682] | loss, 2.324 (heap 2.324) **severe memory loss** | loss, 2.225 (heap 2.227) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.247 [0.242, 0.249], h 0.050 | loss, 0.074 [0.073, 0.074], h 0.050 | reported, 0.129 [0.127, 0.129] | reported, 0.518 [0.514, 0.589] | win, 0.787 (heap 0.816) | loss, 1.249 (heap 1.269) |
| 3 (n 16, rho 16) | loss, 0.390 [0.386, 0.395], h 0.050 | loss, 0.109 [0.109, 0.110], h 0.050 | reported, 0.187 [0.185, 0.188] | reported, 0.528 [0.504, 0.659] | win, 0.309 (heap 0.347) | win, 0.849 (heap 0.875) |
| 4 (n 16, rho 64) | loss, 0.479 [0.473, 0.491], h 0.050 | loss, 0.316 [0.314, 0.317], h 0.050 | reported, 0.396 [0.391, 0.398] | reported, 0.533 [0.526, 0.539] | win, 0.178 (heap 0.220) | win, 0.710 (heap 0.741) |
| 5 (n 32, rho 1) | loss, 0.191 [0.185, 0.194], h 0.050 | loss, 0.103 [0.101, 0.105], h 0.050 | reported, 0.149 [0.145, 0.151] | reported, 0.701 [0.548, 0.769] | loss, 2.114 (heap 2.115) **severe memory loss** | loss, 2.087 (heap 2.088) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.379 [0.371, 0.382], h 0.050 | loss, 0.105 [0.104, 0.105], h 0.050 | reported, 0.199 [0.196, 0.200] | reported, 0.600 [0.569, 0.700] | win, 0.767 (heap 0.775) | loss, 1.182 (heap 1.188) |
| 7 (n 32, rho 16) | loss, 0.507 [0.497, 0.518], h 0.050 | loss, 0.119 [0.118, 0.121], h 0.050 | reported, 0.229 [0.226, 0.232] | reported, 0.551 [0.542, 0.598] | win, 0.285 (heap 0.294) | win, 0.849 (heap 0.856) |
| 8 (n 32, rho 64) | loss, 0.531 [0.521, 0.536], h 0.050 | loss, 0.199 [0.195, 0.200], h 0.050 | reported, 0.333 [0.331, 0.337] | reported, 0.560 [0.542, 0.733] | win, 0.175 (heap 0.185) | win, 0.723 (heap 0.730) |
| 9 (n 64, rho 1) | loss, 0.229 [0.228, 0.230], h 0.050 | loss, 0.119 [0.119, 0.120], h 0.050 | reported, 0.173 [0.172, 0.174] | reported, 0.637 [0.609, 0.818] | loss, 1.898 (heap 1.899) | loss, 1.930 (heap 1.931) |
| 10 (n 64, rho 4) | loss, 0.435 [0.434, 0.436], h 0.050 | loss, 0.123 [0.123, 0.124], h 0.050 | reported, 0.229 [0.228, 0.230] | reported, 0.647 [0.618, 0.764] | win, 0.711 (heap 0.713) | loss, 1.138 (heap 1.140) |
| 11 (n 64, rho 16) | loss, 0.585 [0.580, 0.589], h 0.050 | loss, 0.119 [0.118, 0.119], h 0.050 | reported, 0.243 [0.242, 0.244] | reported, 0.634 [0.609, 0.830] | win, 0.377 (heap 0.379) | tie, 0.914 (heap 0.915) |
| 12 (n 64, rho 64) | loss, 0.654 [0.653, 0.658], h 0.050 | loss, 0.162 [0.161, 0.162], h 0.050 | reported, 0.304 [0.303, 0.305] | reported, 0.617 [0.601, 0.641] | win, 0.164 (heap 0.167) | win, 0.745 (heap 0.747) |

### Pack-coprime against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.005 [0.005, 0.005], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.536 [0.528, 0.696] | loss, 2.688 (heap 2.692) **severe memory loss** | loss, 2.468 (heap 2.474) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.041 [0.041, 0.042], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.011 [0.011, 0.011] | reported, 0.525 [0.519, 0.598] | tie, 0.905 (heap 0.944) | loss, 1.397 (heap 1.427) |
| 3 (n 16, rho 16) | loss, 0.183 [0.181, 0.184], h 0.050 | loss, 0.013 [0.013, 0.013], h 0.050 | reported, 0.027 [0.027, 0.027] | reported, 0.550 [0.527, 0.687] | win, 0.345 (heap 0.391) | win, 0.872 (heap 0.907) |
| 4 (n 16, rho 64) | loss, 0.456 [0.452, 0.468], h 0.050 | loss, 0.233 [0.231, 0.234], h 0.050 | reported, 0.324 [0.321, 0.326] | reported, 0.539 [0.535, 0.543] | win, 0.191 (heap 0.239) | win, 0.719 (heap 0.757) |
| 5 (n 32, rho 1) | loss, 0.006 [0.006, 0.006], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.703 [0.548, 0.774] | loss, 2.343 (heap 2.345) **severe memory loss** | loss, 2.292 (heap 2.295) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.042 [0.041, 0.042], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.609 [0.573, 0.713] | win, 0.859 (heap 0.868) | loss, 1.240 (heap 1.247) |
| 7 (n 32, rho 16) | loss, 0.214 [0.211, 0.221], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.012 [0.012, 0.012] | reported, 0.560 [0.529, 0.618] | win, 0.315 (heap 0.327) | win, 0.869 (heap 0.878) |
| 8 (n 32, rho 64) | loss, 0.423 [0.416, 0.426], h 0.050 | loss, 0.022 [0.022, 0.022], h 0.050 | reported, 0.058 [0.057, 0.058] | reported, 0.575 [0.561, 0.756] | win, 0.183 (heap 0.196) | win, 0.728 (heap 0.738) |
| 9 (n 64, rho 1) | loss, 0.004 [0.004, 0.004], h 0.050 | loss, 0.002 [0.002, 0.002], h 0.050 | reported, 0.003 [0.003, 0.003] | reported, 0.589 [0.548, 0.782] | loss, 2.033 (heap 2.034) **severe memory loss** | loss, 2.048 (heap 2.049) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.027 [0.027, 0.027], h 0.050 | loss, 0.002 [0.002, 0.002], h 0.050 | reported, 0.004 [0.004, 0.004] | reported, 0.583 [0.539, 0.682] | win, 0.767 (heap 0.770) | loss, 1.176 (heap 1.178) |
| 11 (n 64, rho 16) | loss, 0.160 [0.159, 0.161], h 0.050 | loss, 0.002 [0.002, 0.002], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.604 [0.579, 0.788] | win, 0.399 (heap 0.401) | tie, 0.941 (heap 0.943) |
| 12 (n 64, rho 64) | loss, 0.478 [0.477, 0.486], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.593 [0.572, 0.621] | win, 0.172 (heap 0.175) | win, 0.751 (heap 0.753) |

### Pack-primes against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.247 [0.242, 0.249], h 0.050 | loss, 0.177 [0.176, 0.179], h 0.050 | reported, 0.213 [0.211, 0.217] | reported, 0.916 [0.906, 0.949] | loss, 2.884 (heap 2.828) **severe memory loss** | loss, 1.834 (heap 1.821) |
| 2 (n 16, rho 4) | loss, 0.578 [0.566, 0.592], h 0.050 | loss, 0.202 [0.199, 0.203], h 0.050 | reported, 0.317 [0.312, 0.321] | reported, 0.899 [0.897, 0.906] | loss, 2.677 (heap 2.551) **severe memory loss** | loss, 1.434 (heap 1.431) |
| 3 (n 16, rho 16) | loss, 0.823 [0.815, 0.832], h 0.050 | loss, 0.263 [0.262, 0.265], h 0.050 | reported, 0.414 [0.412, 0.417] | reported, 0.888 [0.877, 0.896] | loss, 1.741 (heap 1.692) | loss, 1.115 (heap 1.125) |
| 4 (n 16, rho 64) | tie, 0.951 [0.939, 0.956], h 0.050 | loss, 0.656 [0.650, 0.660], h 0.050 | reported, 0.782 [0.778, 0.789] | reported, 0.924 [0.905, 0.932] | loss, 1.272 (heap 1.307) | tie, 1.037 (heap 1.053) |
| 5 (n 32, rho 1) | loss, 0.392 [0.391, 0.394], h 0.050 | loss, 0.246 [0.244, 0.248], h 0.050 | reported, 0.321 [0.319, 0.323] | reported, 0.931 [0.925, 0.964] | loss, 2.831 (heap 2.815) **severe memory loss** | loss, 1.790 (heap 1.787) |
| 6 (n 32, rho 4) | loss, 0.673 [0.670, 0.677], h 0.050 | loss, 0.232 [0.232, 0.233], h 0.050 | reported, 0.383 [0.382, 0.383] | reported, 0.955 [0.941, 0.971] | loss, 1.975 (heap 1.964) | loss, 1.276 (heap 1.277) |
| 7 (n 32, rho 16) | loss, 0.813 [0.806, 0.823], h 0.050 | loss, 0.252 [0.250, 0.253], h 0.050 | reported, 0.410 [0.406, 0.411] | reported, 0.932 [0.915, 0.956] | loss, 1.695 (heap 1.683) | loss, 1.101 (heap 1.104) |
| 8 (n 32, rho 64) | loss, 0.922 [0.913, 0.925], h 0.050 | loss, 0.478 [0.475, 0.481], h 0.050 | reported, 0.658 [0.655, 0.660] | reported, 0.924 [0.905, 0.961] | loss, 1.266 (heap 1.274) | tie, 1.035 (heap 1.039) |
| 9 (n 64, rho 1) | loss, 0.456 [0.453, 0.458], h 0.050 | loss, 0.259 [0.258, 0.261], h 0.050 | reported, 0.355 [0.354, 0.357] | reported, 0.994 [0.985, 1.003] | loss, 2.777 (heap 2.773) **severe memory loss** | loss, 1.723 (heap 1.722) |
| 10 (n 64, rho 4) | loss, 0.748 [0.745, 0.751], h 0.050 | loss, 0.265 [0.264, 0.266], h 0.050 | reported, 0.427 [0.426, 0.429] | reported, 0.998 [0.991, 1.005] | loss, 1.904 (heap 1.902) | loss, 1.246 (heap 1.247) |
| 11 (n 64, rho 16) | loss, 0.920 [0.914, 0.924], h 0.050 | loss, 0.258 [0.257, 0.260], h 0.050 | reported, 0.433 [0.432, 0.437] | reported, 0.994 [0.983, 0.999] | loss, 1.326 (heap 1.326) | tie, 1.072 (heap 1.073) |
| 12 (n 64, rho 64) | tie, 1.005 [0.997, 1.008], h 0.050 | loss, 0.342 [0.340, 0.343], h 0.050 | reported, 0.530 [0.529, 0.531] | reported, 0.992 [0.963, 0.997] | loss, 1.209 (heap 1.213) | tie, 1.026 (heap 1.027) |

### Pack-coprime against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.013 [0.012, 0.013], h 0.050 | loss, 0.012 [0.012, 0.012], h 0.050 | reported, 0.013 [0.013, 0.013] | reported, 0.928 [0.916, 0.954] | loss, 3.336 (heap 3.275) **severe memory loss** | loss, 2.034 (heap 2.023) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.097 [0.096, 0.099], h 0.050 | loss, 0.014 [0.014, 0.014], h 0.050 | reported, 0.027 [0.027, 0.028] | reported, 0.912 [0.904, 0.921] | loss, 3.079 (heap 2.952) **severe memory loss** | loss, 1.605 (heap 1.609) |
| 3 (n 16, rho 16) | loss, 0.385 [0.382, 0.390], h 0.050 | loss, 0.030 [0.030, 0.031], h 0.050 | reported, 0.060 [0.060, 0.061] | reported, 0.924 [0.918, 0.931] | loss, 1.941 (heap 1.907) | loss, 1.147 (heap 1.166) |
| 4 (n 16, rho 64) | loss, 0.905 [0.895, 0.911], h 0.050 | loss, 0.484 [0.479, 0.487], h 0.050 | reported, 0.641 [0.637, 0.647] | reported, 0.934 [0.922, 0.937] | loss, 1.368 (heap 1.420) | tie, 1.050 (heap 1.074) |
| 5 (n 32, rho 1) | loss, 0.012 [0.012, 0.012], h 0.050 | loss, 0.008 [0.008, 0.008], h 0.050 | reported, 0.010 [0.010, 0.010] | reported, 0.934 [0.928, 0.961] | loss, 3.138 (heap 3.121) **severe memory loss** | loss, 1.966 (heap 1.963) |
| 6 (n 32, rho 4) | loss, 0.075 [0.075, 0.075], h 0.050 | loss, 0.007 [0.007, 0.007], h 0.050 | reported, 0.016 [0.016, 0.016] | reported, 0.968 [0.941, 0.981] | loss, 2.211 (heap 2.201) **severe memory loss** | loss, 1.339 (heap 1.341) |
| 7 (n 32, rho 16) | loss, 0.344 [0.340, 0.352], h 0.050 | loss, 0.010 [0.010, 0.010], h 0.050 | reported, 0.021 [0.021, 0.021] | reported, 0.947 [0.893, 0.976] | loss, 1.875 (heap 1.867) | loss, 1.127 (heap 1.132) |
| 8 (n 32, rho 64) | loss, 0.734 [0.729, 0.737], h 0.050 | loss, 0.054 [0.053, 0.054], h 0.050 | reported, 0.114 [0.113, 0.114] | reported, 0.950 [0.938, 0.960] | loss, 1.327 (heap 1.348) | tie, 1.043 (heap 1.050) |
| 9 (n 64, rho 1) | loss, 0.009 [0.009, 0.009], h 0.050 | loss, 0.004 [0.004, 0.004], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.920 [0.874, 0.966] | loss, 2.975 (heap 2.971) **severe memory loss** | loss, 1.828 (heap 1.828) |
| 10 (n 64, rho 4) | loss, 0.046 [0.046, 0.047], h 0.050 | loss, 0.004 [0.004, 0.004], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.899 [0.824, 0.934] | loss, 2.056 (heap 2.054) **severe memory loss** | loss, 1.288 (heap 1.288) |
| 11 (n 64, rho 16) | loss, 0.251 [0.251, 0.253], h 0.050 | loss, 0.004 [0.004, 0.004], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.948 [0.922, 0.982] | loss, 1.402 (heap 1.404) | loss, 1.104 (heap 1.105) |
| 12 (n 64, rho 64) | loss, 0.735 [0.730, 0.744], h 0.050 | loss, 0.007 [0.007, 0.007], h 0.050 | reported, 0.016 [0.015, 0.016] | reported, 0.952 [0.918, 0.966] | loss, 1.266 (heap 1.273) | tie, 1.033 (heap 1.035) |

### Pack-coprime against Pack-primes (coprime against primes)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 0.051 [0.051, 0.051] | reported, 0.068 [0.067, 0.068] | reported, 0.059 [0.059, 0.060] | reported, 1.013 [0.990, 1.026] | reported, 1.157 (heap 1.158) | reported, 1.109 (heap 1.111) |
| 2 (n 16, rho 4) | reported, 0.168 [0.166, 0.172] | reported, 0.071 [0.071, 0.072] | reported, 0.086 [0.086, 0.088] | reported, 1.014 [1.004, 1.023] | reported, 1.150 (heap 1.157) | reported, 1.119 (heap 1.124) |
| 3 (n 16, rho 16) | reported, 0.468 [0.466, 0.471] | reported, 0.116 [0.115, 0.116] | reported, 0.146 [0.146, 0.147] | reported, 1.041 [1.029, 1.058] | reported, 1.115 (heap 1.127) | reported, 1.028 (heap 1.036) |
| 4 (n 16, rho 64) | reported, 0.952 [0.945, 0.961] | reported, 0.738 [0.732, 0.742] | reported, 0.820 [0.816, 0.825] | reported, 1.011 [0.999, 1.025] | reported, 1.076 (heap 1.087) | reported, 1.013 (heap 1.021) |
| 5 (n 32, rho 1) | reported, 0.031 [0.031, 0.032] | reported, 0.033 [0.033, 0.033] | reported, 0.032 [0.032, 0.032] | reported, 1.004 [0.980, 1.017] | reported, 1.108 (heap 1.109) | reported, 1.098 (heap 1.099) |
| 6 (n 32, rho 4) | reported, 0.111 [0.111, 0.112] | reported, 0.032 [0.032, 0.032] | reported, 0.042 [0.042, 0.042] | reported, 1.014 [0.995, 1.028] | reported, 1.119 (heap 1.120) | reported, 1.049 (heap 1.050) |
| 7 (n 32, rho 16) | reported, 0.423 [0.418, 0.433] | reported, 0.038 [0.038, 0.038] | reported, 0.051 [0.050, 0.051] | reported, 1.016 [0.958, 1.047] | reported, 1.106 (heap 1.109) | reported, 1.024 (heap 1.026) |
| 8 (n 32, rho 64) | reported, 0.797 [0.795, 0.800] | reported, 0.112 [0.112, 0.113] | reported, 0.173 [0.172, 0.174] | reported, 1.028 [0.992, 1.041] | reported, 1.048 (heap 1.058) | reported, 1.008 (heap 1.011) |
| 9 (n 64, rho 1) | reported, 0.019 [0.019, 0.019] | reported, 0.015 [0.015, 0.015] | reported, 0.017 [0.017, 0.017] | reported, 0.925 [0.880, 0.971] | reported, 1.071 (heap 1.071) | reported, 1.061 (heap 1.061) |
| 10 (n 64, rho 4) | reported, 0.062 [0.062, 0.062] | reported, 0.014 [0.014, 0.014] | reported, 0.019 [0.019, 0.019] | reported, 0.901 [0.828, 0.935] | reported, 1.080 (heap 1.080) | reported, 1.033 (heap 1.034) |
| 11 (n 64, rho 16) | reported, 0.273 [0.272, 0.276] | reported, 0.015 [0.015, 0.015] | reported, 0.020 [0.020, 0.021] | reported, 0.954 [0.927, 0.991] | reported, 1.058 (heap 1.059) | reported, 1.030 (heap 1.030) |
| 12 (n 64, rho 64) | reported, 0.731 [0.729, 0.740] | reported, 0.021 [0.021, 0.021] | reported, 0.029 [0.029, 0.029] | reported, 0.960 [0.934, 0.984] | reported, 1.048 (heap 1.050) | reported, 1.007 (heap 1.008) |

### Gcd work and the call-for-call checks

Hash-V-pack: G(row)/G(TC) MUST be exactly 1, and its per-operation records equal TC's, every step (a check, carried over). The factored rows (rule-m3 item 7, v0.3 §16.9 item 3): per operation the same case and the same exact divisions, multiplications and additions as TC (checked); their gcds are obtained otherwise, so G(row)/G(TC) is reported, not checked, per part (entry; the operations' additions and multiplications), beside their factor work; and each factored row takes Hash-V-pack's arithmetic counters (but the operations' gcds) and store splits (v0.3 §16.9 item 4).

| job | G per operation (TC) | Hash-V-pack: G/G(TC), call for call | Pack-primes: G/G(TC) (entry, add, mul), call for call | Pack-primes vs Hash-V-pack | Pack-coprime: G/G(TC) (entry, add, mul), call for call | Pack-coprime vs Hash-V-pack |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 42.89 | 1.000000 (passed), pass | 0.6568 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.7920 (1.0000, 0.3938, n/a), pass (case and core arithmetic) | pass |
| 2 | 41.56 | 1.000000 (passed), pass | 0.6779 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8304 (1.0000, 0.4733, n/a), pass (case and core arithmetic) | pass |
| 3 | 43.43 | 1.000000 (passed), pass | 0.6521 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8085 (1.0000, 0.4496, n/a), pass (case and core arithmetic) | pass |
| 4 | 47.53 | 1.000000 (passed), pass | 0.5949 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8262 (1.0000, 0.5709, n/a), pass (case and core arithmetic) | pass |
| 5 | 42.24 | 1.000000 (passed), pass | 0.6624 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8186 (1.0000, 0.4627, n/a), pass (case and core arithmetic) | pass |
| 6 | 42.47 | 1.000000 (passed), pass | 0.6596 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8096 (1.0000, 0.4407, n/a), pass (case and core arithmetic) | pass |
| 7 | 43.54 | 1.000000 (passed), pass | 0.6500 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.7916 (1.0000, 0.4046, n/a), pass (case and core arithmetic) | pass |
| 8 | 38.81 | 1.000000 (passed), pass | 0.7146 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8092 (1.0000, 0.3316, n/a), pass (case and core arithmetic) | pass |
| 9 | 42.22 | 1.000000 (passed), pass | 0.6649 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8170 (1.0000, 0.4541, n/a), pass (case and core arithmetic) | pass |
| 10 | 41.94 | 1.000000 (passed), pass | 0.6662 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8088 (1.0000, 0.4273, n/a), pass (case and core arithmetic) | pass |
| 11 | 41.43 | 1.000000 (passed), pass | 0.6716 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8230 (1.0000, 0.4612, n/a), pass (case and core arithmetic) | pass |
| 12 | 42.82 | 1.000000 (passed), pass | 0.6534 (1.0000, 0.0000, n/a), pass (case and core arithmetic) | pass | 0.8015 (1.0000, 0.4272, n/a), pass (case and core arithmetic) | pass |

Factor work of the factored rows beside the gcd work (counting runs, the job's factorings and steps summed): merges and entries compared, divisibility tests, per-piece gcds, trial-division candidates, refinement gcds (pass 1 and the reconciliation).

| job | row | G(row)/G(TC) | merges | entries compared | tests | per-piece gcds | trial-division candidates | pass-1 gcds | pass-2 tests | reconciliation gcds | splits (pass 1 / reconciliation / leftover) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | 0.6568 | 207 | 349 | 118 | 0 | 29529 | 0 | 0 | 0 | 0 / 0 / 0 |
| 1 | Pack-coprime | 0.7920 | 207 | 349 | 0 | 95 | 0 | 72950 | 75674 | 1122 | 50 / 0 / 0 |
| 2 | Pack-primes | 0.6779 | 185 | 298 | 133 | 0 | 16552 | 0 | 0 | 0 | 0 / 0 / 0 |
| 2 | Pack-coprime | 0.8304 | 185 | 298 | 0 | 106 | 0 | 29910 | 31793 | 366 | 40 / 0 / 0 |
| 3 | Pack-primes | 0.6521 | 202 | 352 | 149 | 0 | 10583 | 0 | 0 | 0 | 0 / 0 / 0 |
| 3 | Pack-coprime | 0.8085 | 202 | 352 | 0 | 107 | 0 | 12125 | 13190 | 126 | 29 / 0 / 0 |
| 4 | Pack-primes | 0.5949 | 256 | 499 | 246 | 0 | 1348 | 0 | 0 | 0 | 0 / 0 / 0 |
| 4 | Pack-coprime | 0.8262 | 256 | 499 | 0 | 168 | 0 | 317 | 446 | 48 | 10 / 0 / 0 |
| 5 | Pack-primes | 0.6624 | 777 | 1343 | 570 | 0 | 107245 | 0 | 0 | 0 | 0 / 0 / 0 |
| 5 | Pack-coprime | 0.8186 | 777 | 1343 | 0 | 438 | 0 | 588396 | 596352 | 2814 | 54 / 0 / 0 |
| 6 | Pack-primes | 0.6596 | 783 | 1351 | 552 | 0 | 78049 | 0 | 0 | 0 | 0 / 0 / 0 |
| 6 | Pack-coprime | 0.8096 | 783 | 1351 | 0 | 422 | 0 | 319782 | 324924 | 1125 | 51 / 0 / 0 |
| 7 | Pack-primes | 0.6500 | 837 | 1411 | 542 | 0 | 61930 | 0 | 0 | 0 | 0 / 0 / 0 |
| 7 | Pack-coprime | 0.7916 | 837 | 1411 | 0 | 409 | 0 | 217502 | 221613 | 381 | 50 / 0 / 0 |
| 8 | Pack-primes | 0.7146 | 658 | 1070 | 313 | 0 | 15955 | 0 | 0 | 0 | 0 / 0 / 0 |
| 8 | Pack-coprime | 0.8092 | 658 | 1070 | 0 | 245 | 0 | 29407 | 30818 | 123 | 33 / 0 / 0 |
| 9 | Pack-primes | 0.6649 | 3062 | 5319 | 2130 | 0 | 420289 | 0 | 0 | 0 | 0 / 0 / 0 |
| 9 | Pack-coprime | 0.8170 | 3062 | 5319 | 0 | 1704 | 0 | 4414935 | 4437097 | 6627 | 57 / 0 / 0 |
| 10 | Pack-primes | 0.6662 | 3060 | 5221 | 2023 | 0 | 296354 | 0 | 0 | 0 | 0 / 0 / 0 |
| 10 | Pack-coprime | 0.8088 | 3060 | 5221 | 0 | 1602 | 0 | 2774167 | 2789765 | 2781 | 65 / 0 / 0 |
| 11 | Pack-primes | 0.6716 | 3001 | 5323 | 2102 | 0 | 272350 | 0 | 0 | 0 | 0 / 0 / 0 |
| 11 | Pack-coprime | 0.8230 | 3001 | 5323 | 0 | 1654 | 0 | 2359680 | 2371375 | 1074 | 63 / 0 / 0 |
| 12 | Pack-primes | 0.6534 | 3174 | 5579 | 2010 | 0 | 185551 | 0 | 0 | 0 | 0 / 0 / 0 |
| 12 | Pack-coprime | 0.8015 | 3174 | 5579 | 0 | 1619 | 0 | 1322281 | 1330687 | 387 | 55 / 0 / 0 |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m3 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.488 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.419 | 0 |
| NONE | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.747 | 0 |
| NONE | Hash-V-pack vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.166 | 0 |
| NONE | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.191 | 0 |
| NONE | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.103 | 0 |
| NONE | Pack-primes vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.114 | 2 |
| NONE | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 2.087 | 2 |
| NONE | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| NONE | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.003 | 0 |
| NONE | Pack-coprime vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.343 | 3 |
| NONE | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 2.292 | 3 |
| NONE | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.392 | 0 |
| NONE | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.246 | 0 |
| NONE | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.831 | 3 |
| NONE | Pack-primes vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.790 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.012 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.008 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 3.138 | 3 |
| NONE | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.966 | 1 |
| NONE | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.031 | 0 |
| NONE | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 0.033 | 0 |
| NONE | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.108 | 0 |
| NONE | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.098 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.564 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.451 | 0 |
| LOW | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.373 | 0 |
| LOW | Hash-V-pack vs TC | peak | 1 | 2 | 0 | 0 | 0 | 0 | 0.913 | 0 |
| LOW | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.379 | 0 |
| LOW | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.105 | 0 |
| LOW | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.767 | 0 |
| LOW | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.182 | 0 |
| LOW | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.041 | 0 |
| LOW | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.003 | 0 |
| LOW | Pack-coprime vs TC | stable | 2 | 1 | 0 | 0 | 0 | 0 | 0.859 | 0 |
| LOW | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.240 | 0 |
| LOW | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.673 | 0 |
| LOW | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.232 | 0 |
| LOW | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.975 | 1 |
| LOW | Pack-primes vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.276 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.075 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.007 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.211 | 3 |
| LOW | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.339 | 0 |
| LOW | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.111 | 0 |
| LOW | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 0.032 | 0 |
| LOW | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.119 | 0 |
| LOW | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.049 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.624 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.459 | 0 |
| MID | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.178 | 0 |
| MID | Hash-V-pack vs TC | peak | 3 | 0 | 0 | 0 | 0 | 0 | 0.771 | 0 |
| MID | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.507 | 0 |
| MID | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.119 | 0 |
| MID | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.309 | 0 |
| MID | Pack-primes vs TC | peak | 2 | 1 | 0 | 0 | 0 | 0 | 0.849 | 0 |
| MID | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.183 | 0 |
| MID | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| MID | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.345 | 0 |
| MID | Pack-coprime vs TC | peak | 2 | 1 | 0 | 0 | 0 | 0 | 0.872 | 0 |
| MID | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.823 | 0 |
| MID | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.258 | 0 |
| MID | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.695 | 0 |
| MID | Pack-primes vs Hash-V-pack | peak | 0 | 1 | 2 | 0 | 0 | 0 | 1.101 | 0 |
| MID | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.344 | 0 |
| MID | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.010 | 0 |
| MID | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.875 | 0 |
| MID | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.127 | 0 |
| MID | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.423 | 0 |
| MID | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 0.038 | 0 |
| MID | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.106 | 0 |
| MID | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.028 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.576 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.473 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.138 | 0 |
| HIGH | Hash-V-pack vs TC | peak | 3 | 0 | 0 | 0 | 0 | 0 | 0.698 | 0 |
| HIGH | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.531 | 0 |
| HIGH | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.199 | 0 |
| HIGH | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.175 | 0 |
| HIGH | Pack-primes vs TC | peak | 3 | 0 | 0 | 0 | 0 | 0 | 0.723 | 0 |
| HIGH | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.456 | 0 |
| HIGH | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.022 | 0 |
| HIGH | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.183 | 0 |
| HIGH | Pack-coprime vs TC | peak | 3 | 0 | 0 | 0 | 0 | 0 | 0.728 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | build | 0 | 2 | 1 | 0 | 0 | 0 | 0.951 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.478 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.266 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | peak | 0 | 3 | 0 | 0 | 0 | 0 | 1.035 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.735 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.054 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.327 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | peak | 0 | 3 | 0 | 0 | 0 | 0 | 1.043 | 0 |
| HIGH | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.797 | 0 |
| HIGH | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 0.112 | 0 |
| HIGH | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.048 | 0 |
| HIGH | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.008 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.396 | 0.426 | 0.475 | 0.504 |
| 32 | 0.488 | 0.564 | 0.624 | 0.576 |
| 64 | 0.502 | 0.582 | 0.636 | 0.650 |

Hash-V-pack against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.381 | 0.366 | 0.415 | 0.482 |
| 32 | 0.419 | 0.451 | 0.475 | 0.415 |
| 64 | 0.459 | 0.464 | 0.459 | 0.473 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.806 | 0.294 | 0.178 | 0.140 |
| 32 | 0.747 | 0.388 | 0.168 | 0.138 |
| 64 | 0.683 | 0.373 | 0.284 | 0.136 |

Hash-V-pack against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.213 | 0.871 | 0.761 | 0.685 |
| 32 | 1.166 | 0.926 | 0.771 | 0.698 |
| 64 | 1.121 | 0.913 | 0.852 | 0.726 |

Pack-primes against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.098 | 0.247 | 0.390 | 0.479 |
| 32 | 0.191 | 0.379 | 0.507 | 0.531 |
| 64 | 0.229 | 0.435 | 0.585 | 0.654 |

Pack-primes against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.067 | 0.074 | 0.109 | 0.316 |
| 32 | 0.103 | 0.105 | 0.119 | 0.199 |
| 64 | 0.119 | 0.123 | 0.119 | 0.162 |

Pack-primes against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.324 | 0.787 | 0.309 | 0.178 |
| 32 | 2.114 | 0.767 | 0.285 | 0.175 |
| 64 | 1.898 | 0.711 | 0.377 | 0.164 |

Pack-primes against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.225 | 1.249 | 0.849 | 0.710 |
| 32 | 2.087 | 1.182 | 0.849 | 0.723 |
| 64 | 1.930 | 1.138 | 0.914 | 0.745 |

Pack-coprime against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.005 | 0.041 | 0.183 | 0.456 |
| 32 | 0.006 | 0.042 | 0.214 | 0.423 |
| 64 | 0.004 | 0.027 | 0.160 | 0.478 |

Pack-coprime against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.005 | 0.005 | 0.013 | 0.233 |
| 32 | 0.003 | 0.003 | 0.005 | 0.022 |
| 64 | 0.002 | 0.002 | 0.002 | 0.003 |

Pack-coprime against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.688 | 0.905 | 0.345 | 0.191 |
| 32 | 2.343 | 0.859 | 0.315 | 0.183 |
| 64 | 2.033 | 0.767 | 0.399 | 0.172 |

Pack-coprime against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.468 | 1.397 | 0.872 | 0.719 |
| 32 | 2.292 | 1.240 | 0.869 | 0.728 |
| 64 | 2.048 | 1.176 | 0.941 | 0.751 |

Pack-primes against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.247 | 0.578 | 0.823 | 0.951 |
| 32 | 0.392 | 0.673 | 0.813 | 0.922 |
| 64 | 0.456 | 0.748 | 0.920 | 1.005 |

Pack-primes against Hash-V-pack, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.177 | 0.202 | 0.263 | 0.656 |
| 32 | 0.246 | 0.232 | 0.252 | 0.478 |
| 64 | 0.259 | 0.265 | 0.258 | 0.342 |

Pack-primes against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.884 | 2.677 | 1.741 | 1.272 |
| 32 | 2.831 | 1.975 | 1.695 | 1.266 |
| 64 | 2.777 | 1.904 | 1.326 | 1.209 |

Pack-primes against Hash-V-pack, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.834 | 1.434 | 1.115 | 1.037 |
| 32 | 1.790 | 1.276 | 1.101 | 1.035 |
| 64 | 1.723 | 1.246 | 1.072 | 1.026 |

Pack-coprime against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.013 | 0.097 | 0.385 | 0.905 |
| 32 | 0.012 | 0.075 | 0.344 | 0.734 |
| 64 | 0.009 | 0.046 | 0.251 | 0.735 |

Pack-coprime against Hash-V-pack, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.012 | 0.014 | 0.030 | 0.484 |
| 32 | 0.008 | 0.007 | 0.010 | 0.054 |
| 64 | 0.004 | 0.004 | 0.004 | 0.007 |

Pack-coprime against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 3.336 | 3.079 | 1.941 | 1.368 |
| 32 | 3.138 | 2.211 | 1.875 | 1.327 |
| 64 | 2.975 | 2.056 | 1.402 | 1.266 |

Pack-coprime against Hash-V-pack, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.034 | 1.605 | 1.147 | 1.050 |
| 32 | 1.966 | 1.339 | 1.127 | 1.043 |
| 64 | 1.828 | 1.288 | 1.104 | 1.033 |

Pack-coprime against Pack-primes, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.051 | 0.168 | 0.468 | 0.952 |
| 32 | 0.031 | 0.111 | 0.423 | 0.797 |
| 64 | 0.019 | 0.062 | 0.273 | 0.731 |

Pack-coprime against Pack-primes, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.068 | 0.071 | 0.116 | 0.738 |
| 32 | 0.033 | 0.032 | 0.038 | 0.112 |
| 64 | 0.015 | 0.014 | 0.015 | 0.021 |

Pack-coprime against Pack-primes, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.157 | 1.150 | 1.115 | 1.076 |
| 32 | 1.108 | 1.119 | 1.106 | 1.048 |
| 64 | 1.071 | 1.080 | 1.058 | 1.048 |

Pack-coprime against Pack-primes, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.109 | 1.119 | 1.028 | 1.013 |
| 32 | 1.098 | 1.049 | 1.024 | 1.008 |
| 64 | 1.061 | 1.033 | 1.030 | 1.007 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (3 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build (its factoring included), over that input's cells. (f), (f'), (q) and (r) are accounted (v0.3 §16.10).

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 1 | Hash-V-pack | 12.89 (13.41) | 11.41 (11.93) | 19.41 (19.95) | 12.91 (52.91) | 12.88 (52.88) |
| 1 | Pack-primes | 37.18 (37.91) | 32.80 (33.52) | 35.60 (36.32) | 37.38 (52.91) | 36.98 (52.88) |
| 1 | Pack-coprime | 43.01 (43.91) | 37.87 (38.78) | 39.49 (40.36) | 43.34 (52.91) | 42.67 (52.88) |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 2 | Hash-V-pack | 4.70 (5.22) | 5.93 (6.45) | 13.93 (14.47) | 4.72 (46.72) | 4.69 (46.69) |
| 2 | Pack-primes | 12.59 (13.31) | 16.46 (17.19) | 19.98 (20.71) | 12.49 (46.72) | 12.69 (46.69) |
| 2 | Pack-coprime | 14.48 (15.41) | 18.82 (19.74) | 22.36 (23.28) | 14.37 (46.72) | 14.59 (46.69) |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 3 | Hash-V-pack | 2.84 (3.34) | 4.17 (4.68) | 12.17 (12.70) | 2.84 (44.84) | 2.84 (44.84) |
| 3 | Pack-primes | 4.95 (5.66) | 8.95 (9.65) | 13.58 (14.28) | 5.00 (44.84) | 4.90 (44.84) |
| 3 | Pack-coprime | 5.52 (6.38) | 10.07 (10.91) | 13.96 (14.80) | 5.55 (44.84) | 5.48 (44.84) |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 4 | Hash-V-pack | 2.23 (2.75) | 2.29 (2.80) | 10.96 (11.49) | 2.22 (44.22) | 2.25 (44.25) |
| 4 | Pack-primes | 2.84 (3.59) | 3.20 (3.94) | 11.36 (12.09) | 2.87 (44.22) | 2.81 (44.25) |
| 4 | Pack-coprime | 3.06 (3.91) | 3.42 (4.28) | 11.51 (12.34) | 3.12 (44.22) | 3.00 (44.25) |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 5 | Hash-V-pack | 11.95 (12.08) | 10.66 (10.79) | 18.66 (18.79) | 11.98 (51.98) | 11.91 (51.91) |
| 5 | Pack-primes | 33.82 (34.00) | 30.07 (30.26) | 33.39 (33.58) | 34.15 (51.98) | 33.49 (51.91) |
| 5 | Pack-coprime | 37.48 (37.70) | 33.36 (33.58) | 36.68 (36.90) | 37.78 (55.54) | 37.18 (51.91) |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 6 | Hash-V-pack | 6.21 (6.34) | 6.82 (6.95) | 14.82 (14.96) | 6.21 (46.21) | 6.22 (46.22) |
| 6 | Pack-primes | 12.27 (12.46) | 15.56 (15.75) | 18.91 (19.10) | 12.28 (46.21) | 12.27 (46.22) |
| 6 | Pack-coprime | 13.74 (13.96) | 17.41 (17.64) | 19.84 (20.05) | 13.76 (46.21) | 13.72 (46.22) |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 7 | Hash-V-pack | 2.69 (2.81) | 4.33 (4.46) | 12.33 (12.47) | 2.71 (44.71) | 2.66 (44.66) |
| 7 | Pack-primes | 4.56 (4.73) | 9.85 (10.03) | 13.58 (13.76) | 4.58 (44.71) | 4.53 (44.66) |
| 7 | Pack-coprime | 5.04 (5.25) | 10.92 (11.14) | 13.90 (14.12) | 5.08 (44.71) | 5.00 (44.66) |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 8 | Hash-V-pack | 2.21 (2.34) | 3.17 (3.30) | 11.17 (11.30) | 2.21 (44.21) | 2.21 (44.21) |
| 8 | Pack-primes | 2.80 (2.98) | 4.85 (5.03) | 11.56 (11.74) | 2.79 (44.21) | 2.81 (44.21) |
| 8 | Pack-coprime | 2.93 (3.15) | 5.23 (5.45) | 11.65 (11.87) | 2.93 (44.21) | 2.93 (44.21) |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 9 | Hash-V-pack | 10.94 (10.97) | 9.93 (9.96) | 17.93 (17.96) | 10.95 (50.95) | 10.92 (50.92) |
| 9 | Pack-primes | 30.37 (30.41) | 27.49 (27.54) | 30.89 (30.93) | 30.57 (50.95) | 30.17 (50.92) |
| 9 | Pack-coprime | 32.53 (32.58) | 29.54 (29.60) | 32.78 (32.83) | 32.69 (50.95) | 32.37 (50.92) |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 10 | Hash-V-pack | 5.97 (6.00) | 6.61 (6.65) | 14.61 (14.65) | 5.98 (45.98) | 5.97 (45.97) |
| 10 | Pack-primes | 11.37 (11.42) | 14.75 (14.80) | 18.21 (18.26) | 11.41 (45.98) | 11.34 (45.97) |
| 10 | Pack-coprime | 12.28 (12.33) | 15.98 (16.03) | 18.82 (18.87) | 12.31 (45.98) | 12.25 (45.97) |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 11 | Hash-V-pack | 4.55 (4.58) | 5.63 (5.66) | 13.63 (13.66) | 4.54 (44.54) | 4.55 (44.55) |
| 11 | Pack-primes | 6.03 (6.07) | 11.11 (11.15) | 14.62 (14.66) | 5.99 (44.54) | 6.06 (44.55) |
| 11 | Pack-coprime | 6.38 (6.43) | 11.94 (11.99) | 15.06 (15.11) | 6.34 (44.54) | 6.42 (44.55) |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 12 | Hash-V-pack | 2.17 (2.20) | 3.62 (3.66) | 11.62 (11.66) | 2.17 (44.17) | 2.17 (44.17) |
| 12 | Pack-primes | 2.63 (2.67) | 6.88 (6.93) | 11.93 (11.97) | 2.62 (44.17) | 2.64 (44.17) |
| 12 | Pack-coprime | 2.75 (2.80) | 7.43 (7.48) | 12.01 (12.06) | 2.74 (44.17) | 2.76 (44.17) |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B |
| --- | --- | --- | --- |
| 1 | TC | 2895 | 2817 |
| 1 | Hash-V-pack | 7338 | 7157 |
| 1 | Pack-primes | 30730 | 27999 |
| 1 | Pack-coprime | 593820 | 552024 |
| 1 | Hash-V-pack-null | 7242 | 7095 |
| 2 | TC | 2796 | 2787 |
| 2 | Hash-V-pack | 6602 | 6425 |
| 2 | Pack-primes | 11430 | 11124 |
| 2 | Pack-coprime | 68648 | 65901 |
| 2 | Hash-V-pack-null | 6587 | 6426 |
| 3 | TC | 2826 | 2714 |
| 3 | Hash-V-pack | 5946 | 5794 |
| 3 | Pack-primes | 7161 | 7160 |
| 3 | Pack-coprime | 15264 | 15330 |
| 3 | Hash-V-pack-null | 5912 | 5783 |
| 4 | TC | 3101 | 3318 |
| 4 | Hash-V-pack | 6238 | 6086 |
| 4 | Pack-primes | 6477 | 6573 |
| 4 | Pack-coprime | 6890 | 6804 |
| 4 | Hash-V-pack-null | 6239 | 6087 |
| 5 | TC | 24970 | 24846 |
| 5 | Hash-V-pack | 50949 | 51501 |
| 5 | Pack-primes | 131007 | 129443 |
| 5 | Pack-coprime | 4095927 | 4182222 |
| 5 | Hash-V-pack-null | 50936 | 51602 |
| 6 | TC | 25160 | 25153 |
| 6 | Hash-V-pack | 45296 | 44220 |
| 6 | Pack-primes | 66558 | 65939 |
| 6 | Pack-coprime | 610377 | 580881 |
| 6 | Hash-V-pack-null | 45415 | 44209 |
| 7 | TC | 23501 | 22978 |
| 7 | Hash-V-pack | 36666 | 37438 |
| 7 | Pack-primes | 45941 | 44442 |
| 7 | Pack-coprime | 110364 | 102152 |
| 7 | Hash-V-pack-null | 36852 | 37424 |
| 8 | TC | 17394 | 19198 |
| 8 | Hash-V-pack | 29794 | 33651 |
| 8 | Pack-primes | 32997 | 36360 |
| 8 | Pack-coprime | 42027 | 44849 |
| 8 | Hash-V-pack-null | 29685 | 33776 |
| 9 | TC | 115519 | 113488 |
| 9 | Hash-V-pack | 229293 | 228949 |
| 9 | Pack-primes | 503386 | 500990 |
| 9 | Pack-coprime | 25541166 | 26988994 |
| 9 | Hash-V-pack-null | 228018 | 227838 |
| 10 | TC | 115513 | 114384 |
| 10 | Hash-V-pack | 197935 | 196539 |
| 10 | Pack-primes | 264262 | 264303 |
| 10 | Pack-coprime | 4315788 | 4173025 |
| 10 | Hash-V-pack-null | 197952 | 196711 |
| 11 | TC | 110709 | 113762 |
| 11 | Hash-V-pack | 174965 | 177191 |
| 11 | Pack-primes | 189512 | 192976 |
| 11 | Pack-coprime | 678762 | 717514 |
| 11 | Hash-V-pack-null | 175380 | 177786 |
| 12 | TC | 112960 | 106809 |
| 12 | Hash-V-pack | 170473 | 162829 |
| 12 | Pack-primes | 170312 | 163290 |
| 12 | Pack-coprime | 230516 | 220393 |
| 12 | Hash-V-pack-null | 170641 | 163254 |

## Stores, per job and matrix (rule-m3 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. T is the build table's. The factored rows' stores, lists and packings are Hash-V-pack's (checked).

| job | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | cells bytes | list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.13 | 6 | 0 (0) | 1024 | 2280 |
| 1 | B | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.08 | 5 | 0 (0) | 1024 | 2272 |
| 1 | C | 285 | 1024 | 27.8% | 32 | 16 | 227 / 285 | 1.11 | 5 | 2 (2048) | 1024 | 1140 |
| 2 | A | 87 | 1024 | 8.5% | 64 | 8 | 425 / 87 | 1.01 | 2 | 0 (0) | 512 | 696 |
| 2 | B | 86 | 1024 | 8.4% | 64 | 8 | 426 / 86 | 1.04 | 2 | 0 (0) | 512 | 688 |
| 2 | C | 281 | 1024 | 27.4% | 32 | 16 | 231 / 281 | 1.10 | 5 | 2 (2048) | 1024 | 1124 |
| 3 | A | 27 | 1024 | 2.6% | 64 | 8 | 485 / 27 | 1.00 | 1 | 0 (0) | 512 | 216 |
| 3 | B | 27 | 1024 | 2.6% | 64 | 8 | 485 / 27 | 1.00 | 1 | 0 (0) | 512 | 216 |
| 3 | C | 181 | 1024 | 17.7% | 32 | 16 | 331 / 181 | 1.10 | 5 | 1 (1024) | 1024 | 724 |
| 4 | A | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 512 | 56 |
| 4 | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | C | 26 | 1024 | 2.5% | 32 | 8 | 486 / 26 | 1.06 | 2 | 1 (1024) | 512 | 104 |
| 5 | A | 1022 | 4096 | 25.0% | 64 | 16 | 1026 / 1022 | 1.07 | 6 | 0 (0) | 4096 | 8176 |
| 5 | B | 1012 | 4096 | 24.7% | 64 | 16 | 1036 / 1012 | 1.06 | 5 | 0 (0) | 4096 | 8096 |
| 5 | C | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.12 | 10 | 2 (8192) | 4096 | 4176 |
| 6 | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 4096 | 2264 |
| 6 | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.02 | 4 | 0 (0) | 4096 | 2272 |
| 6 | C | 1031 | 4096 | 25.2% | 32 | 16 | 1017 / 1031 | 1.10 | 7 | 2 (8192) | 4096 | 4124 |
| 7 | A | 91 | 4096 | 2.2% | 64 | 8 | 1957 / 91 | 1.01 | 2 | 0 (0) | 2048 | 728 |
| 7 | B | 85 | 4096 | 2.1% | 64 | 8 | 1963 / 85 | 1.00 | 1 | 0 (0) | 2048 | 680 |
| 7 | C | 927 | 4096 | 22.6% | 32 | 16 | 1121 / 927 | 1.07 | 5 | 1 (4096) | 4096 | 3708 |
| 8 | A | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 2048 | 216 |
| 8 | B | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 2048 | 216 |
| 8 | C | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.01 | 3 | 1 (4096) | 4096 | 1120 |
| 9 | A | 3559 | 16384 | 21.7% | 64 | 16 | 4633 / 3559 | 1.02 | 4 | 0 (0) | 16384 | 28472 |
| 9 | B | 3543 | 16384 | 21.6% | 64 | 16 | 4649 / 3543 | 1.03 | 5 | 0 (0) | 16384 | 28344 |
| 9 | C | 4013 | 16384 | 24.5% | 32 | 16 | 4179 / 4013 | 1.07 | 6 | 1 (16384) | 16384 | 16052 |
| 10 | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.00 | 2 | 0 (0) | 16384 | 8104 |
| 10 | B | 1007 | 16384 | 6.1% | 64 | 16 | 7185 / 1007 | 1.00 | 3 | 0 (0) | 16384 | 8056 |
| 10 | C | 3988 | 16384 | 24.3% | 32 | 16 | 4204 / 3988 | 1.06 | 6 | 1 (16384) | 16384 | 15952 |
| 11 | A | 279 | 16384 | 1.7% | 64 | 16 | 7913 / 279 | 1.00 | 1 | 0 (0) | 16384 | 2232 |
| 11 | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 16384 | 2248 |
| 11 | C | 3894 | 16384 | 23.8% | 32 | 16 | 4298 / 3894 | 1.07 | 7 | 1 (16384) | 16384 | 15576 |
| 12 | A | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 8192 | 696 |
| 12 | B | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 8192 | 712 |
| 12 | C | 2588 | 16384 | 15.8% | 32 | 16 | 5604 / 2588 | 1.04 | 5 | 2 (32768) | 16384 | 10352 |

### Hash-V-pack's packings (from the counting runs)

| job | matrix | W | w_h build | w_h packed | list bytes | cells rewritten | map bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 285 | 16 | 16 | 2280 | 512 | 2048 |
| 1 | B | 284 | 16 | 16 | 2272 | 512 | 2048 |
| 1 | C | 285 | 16 | 16 | 1140 | 512 | 2048 |
| 2 | A | 87 | 16 | 8 | 696 | 512 | 2048 |
| 2 | B | 86 | 16 | 8 | 688 | 512 | 2048 |
| 2 | C | 281 | 16 | 16 | 1124 | 512 | 2048 |
| 3 | A | 27 | 16 | 8 | 216 | 512 | 2048 |
| 3 | B | 27 | 16 | 8 | 216 | 512 | 2048 |
| 3 | C | 181 | 16 | 16 | 724 | 512 | 2048 |
| 4 | A | 7 | 16 | 8 | 56 | 512 | 2048 |
| 4 | B | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | C | 26 | 16 | 8 | 104 | 512 | 2048 |
| 5 | A | 1022 | 16 | 16 | 8176 | 2048 | 8192 |
| 5 | B | 1012 | 16 | 16 | 8096 | 2048 | 8192 |
| 5 | C | 1044 | 16 | 16 | 4176 | 2048 | 8192 |
| 6 | A | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | B | 284 | 16 | 16 | 2272 | 2048 | 8192 |
| 6 | C | 1031 | 16 | 16 | 4124 | 2048 | 8192 |
| 7 | A | 91 | 16 | 8 | 728 | 2048 | 8192 |
| 7 | B | 85 | 16 | 8 | 680 | 2048 | 8192 |
| 7 | C | 927 | 16 | 16 | 3708 | 2048 | 8192 |
| 8 | A | 27 | 16 | 8 | 216 | 2048 | 8192 |
| 8 | B | 27 | 16 | 8 | 216 | 2048 | 8192 |
| 8 | C | 280 | 16 | 16 | 1120 | 2048 | 8192 |
| 9 | A | 3559 | 16 | 16 | 28472 | 8192 | 32768 |
| 9 | B | 3543 | 16 | 16 | 28344 | 8192 | 32768 |
| 9 | C | 4013 | 16 | 16 | 16052 | 8192 | 32768 |
| 10 | A | 1013 | 16 | 16 | 8104 | 8192 | 32768 |
| 10 | B | 1007 | 16 | 16 | 8056 | 8192 | 32768 |
| 10 | C | 3988 | 16 | 16 | 15952 | 8192 | 32768 |
| 11 | A | 279 | 16 | 16 | 2232 | 8192 | 32768 |
| 11 | B | 281 | 16 | 16 | 2248 | 8192 | 32768 |
| 11 | C | 3894 | 16 | 16 | 15576 | 8192 | 32768 |
| 12 | A | 87 | 16 | 8 | 696 | 8192 | 32768 |
| 12 | B | 89 | 16 | 8 | 712 | 8192 | 32768 |
| 12 | C | 2588 | 16 | 16 | 10352 | 8192 | 32768 |

### The factored rows' factorings (from the counting runs, each at its factoring)

Pack-primes: trial-division candidates, divisions with remainder and exact divisions. Pack-coprime: pass 1's scan gcds, divisions and splits (with the splits' gcds), pass 2's divisibility tests. E: (f)'s entries; |q|: the base's pieces.

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes | candidates | divisions | exact | pass-1 gcds | pass-1 divisions | splits (gcds) | pass-2 tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 285 | 569 | 6265 | 0 | 0 | 6632 | 7037 | 405 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | B | 284 | 559 | 6171 | 0 | 0 | 5685 | 6107 | 422 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | C | 285 | 570 | 3994 | 0 | 0 | 17212 | 17610 | 398 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-coprime | A | 285 | 568 | 6256 | 192 | 1536 | 0 | 0 | 0 | 22039 | 857 | 16 (15) | 22810 |
| 1 | Pack-coprime | B | 284 | 559 | 6171 | 182 | 1456 | 0 | 0 | 0 | 20888 | 884 | 12 (12) | 21629 |
| 1 | Pack-coprime | C | 285 | 564 | 3964 | 234 | 936 | 0 | 0 | 0 | 30023 | 758 | 22 (26) | 31235 |
| 2 | Pack-primes | A | 87 | 182 | 1990 | 0 | 0 | 1151 | 1330 | 179 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | B | 86 | 189 | 2049 | 0 | 0 | 1097 | 1258 | 161 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | C | 281 | 584 | 4048 | 0 | 0 | 14304 | 14745 | 441 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-coprime | A | 87 | 182 | 1990 | 60 | 480 | 0 | 0 | 0 | 2221 | 313 | 9 (9) | 2495 |
| 2 | Pack-coprime | B | 86 | 188 | 2040 | 62 | 496 | 0 | 0 | 0 | 2154 | 299 | 9 (12) | 2396 |
| 2 | Pack-coprime | C | 281 | 580 | 4028 | 215 | 860 | 0 | 0 | 0 | 25535 | 852 | 22 (24) | 26902 |
| 3 | Pack-primes | A | 27 | 49 | 553 | 0 | 0 | 400 | 444 | 44 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | B | 27 | 46 | 526 | 0 | 0 | 386 | 431 | 45 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | C | 181 | 372 | 2588 | 0 | 0 | 9797 | 10088 | 291 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-coprime | A | 27 | 46 | 526 | 21 | 168 | 0 | 0 | 0 | 252 | 65 | 5 (5) | 323 |
| 3 | Pack-coprime | B | 27 | 44 | 508 | 21 | 168 | 0 | 0 | 0 | 227 | 65 | 3 (2) | 294 |
| 3 | Pack-coprime | C | 181 | 370 | 2578 | 146 | 584 | 0 | 0 | 0 | 11646 | 533 | 21 (20) | 12573 |
| 4 | Pack-primes | A | 7 | 15 | 167 | 0 | 0 | 63 | 73 | 10 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | B | 8 | 12 | 144 | 0 | 0 | 152 | 164 | 12 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | C | 26 | 55 | 383 | 0 | 0 | 1133 | 1185 | 52 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-coprime | A | 7 | 14 | 158 | 9 | 72 | 0 | 0 | 0 | 34 | 10 | 3 (5) | 53 |
| 4 | Pack-coprime | B | 8 | 11 | 135 | 7 | 56 | 0 | 0 | 0 | 26 | 10 | 3 (4) | 48 |
| 4 | Pack-coprime | C | 26 | 50 | 358 | 21 | 84 | 0 | 0 | 0 | 257 | 75 | 4 (7) | 345 |
| 5 | Pack-primes | A | 1022 | 2067 | 22695 | 0 | 0 | 21550 | 23056 | 1506 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | B | 1012 | 2005 | 22097 | 0 | 0 | 22131 | 23572 | 1441 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | C | 1044 | 2135 | 14855 | 0 | 0 | 63564 | 64939 | 1375 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-coprime | A | 1022 | 2067 | 22695 | 465 | 3720 | 0 | 0 | 0 | 157136 | 3628 | 13 (13) | 159288 |
| 5 | Pack-coprime | B | 1012 | 2005 | 22097 | 473 | 3784 | 0 | 0 | 0 | 160551 | 3476 | 15 (14) | 162652 |
| 5 | Pack-coprime | C | 1044 | 2135 | 14855 | 647 | 2588 | 0 | 0 | 0 | 270709 | 3233 | 26 (27) | 274412 |
| 6 | Pack-primes | A | 283 | 564 | 6212 | 0 | 0 | 5770 | 6201 | 431 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | B | 284 | 562 | 6198 | 0 | 0 | 5874 | 6294 | 420 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | C | 1031 | 2064 | 14448 | 0 | 0 | 66405 | 67744 | 1339 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-coprime | A | 283 | 564 | 6212 | 190 | 1520 | 0 | 0 | 0 | 21832 | 879 | 14 (16) | 22726 |
| 6 | Pack-coprime | B | 284 | 562 | 6198 | 185 | 1480 | 0 | 0 | 0 | 20795 | 878 | 11 (14) | 21606 |
| 6 | Pack-coprime | C | 1031 | 2061 | 14433 | 676 | 2704 | 0 | 0 | 0 | 277155 | 3044 | 26 (31) | 280592 |
| 7 | Pack-primes | A | 91 | 172 | 1916 | 0 | 0 | 1618 | 1772 | 154 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | B | 85 | 174 | 1910 | 0 | 0 | 1262 | 1407 | 145 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | C | 927 | 1882 | 13122 | 0 | 0 | 59050 | 60279 | 1229 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-coprime | A | 91 | 170 | 1898 | 66 | 528 | 0 | 0 | 0 | 2422 | 265 | 7 (11) | 2668 |
| 7 | Pack-coprime | B | 85 | 173 | 1901 | 61 | 488 | 0 | 0 | 0 | 2258 | 267 | 12 (11) | 2588 |
| 7 | Pack-coprime | C | 927 | 1880 | 13112 | 581 | 2324 | 0 | 0 | 0 | 212822 | 2844 | 31 (29) | 216357 |
| 8 | Pack-primes | A | 27 | 53 | 589 | 0 | 0 | 289 | 331 | 42 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | B | 27 | 56 | 616 | 0 | 0 | 220 | 273 | 53 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | C | 280 | 567 | 3959 | 0 | 0 | 15446 | 15849 | 403 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-coprime | A | 27 | 51 | 571 | 21 | 168 | 0 | 0 | 0 | 232 | 69 | 5 (7) | 327 |
| 8 | Pack-coprime | B | 27 | 52 | 580 | 20 | 160 | 0 | 0 | 0 | 235 | 83 | 4 (3) | 303 |
| 8 | Pack-coprime | C | 280 | 565 | 3949 | 226 | 904 | 0 | 0 | 0 | 28940 | 777 | 24 (23) | 30188 |
| 9 | Pack-primes | A | 3559 | 7347 | 80363 | 0 | 0 | 77911 | 83244 | 5333 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | B | 3543 | 7186 | 78850 | 0 | 0 | 79286 | 84450 | 5164 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | C | 4013 | 8111 | 56611 | 0 | 0 | 263092 | 268266 | 5174 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-coprime | A | 3559 | 7347 | 80363 | 1083 | 8664 | 0 | 0 | 0 | 973632 | 13973 | 13 (14) | 979917 |
| 9 | Pack-coprime | B | 3543 | 7186 | 78850 | 1126 | 9008 | 0 | 0 | 0 | 1025544 | 13522 | 12 (13) | 1031769 |
| 9 | Pack-coprime | C | 4013 | 8111 | 56611 | 1880 | 7520 | 0 | 0 | 0 | 2415759 | 13475 | 32 (33) | 2425411 |
| 10 | Pack-primes | A | 1013 | 2021 | 22245 | 0 | 0 | 21090 | 22539 | 1449 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | B | 1007 | 1997 | 22005 | 0 | 0 | 21403 | 22869 | 1466 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | C | 3988 | 7953 | 55721 | 0 | 0 | 253861 | 258842 | 4981 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-coprime | A | 1013 | 2021 | 22245 | 461 | 3688 | 0 | 0 | 0 | 161282 | 3526 | 9 (9) | 163422 |
| 10 | Pack-coprime | B | 1007 | 1997 | 22005 | 466 | 3728 | 0 | 0 | 0 | 155768 | 3485 | 17 (17) | 157932 |
| 10 | Pack-coprime | C | 3988 | 7953 | 55721 | 1916 | 7664 | 0 | 0 | 0 | 2457117 | 13007 | 39 (38) | 2468411 |
| 11 | Pack-primes | A | 279 | 534 | 5926 | 0 | 0 | 5862 | 6263 | 401 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | B | 281 | 564 | 6204 | 0 | 0 | 5887 | 6350 | 463 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | C | 3894 | 7926 | 55210 | 0 | 0 | 260601 | 265649 | 5048 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-coprime | A | 279 | 533 | 5917 | 177 | 1416 | 0 | 0 | 0 | 19288 | 843 | 15 (14) | 20118 |
| 11 | Pack-coprime | B | 281 | 564 | 6204 | 181 | 1448 | 0 | 0 | 0 | 21107 | 924 | 14 (18) | 22121 |
| 11 | Pack-coprime | C | 3894 | 7926 | 55210 | 1836 | 7344 | 0 | 0 | 0 | 2319285 | 13131 | 34 (32) | 2329136 |
| 12 | Pack-primes | A | 87 | 164 | 1828 | 0 | 0 | 1521 | 1672 | 151 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | B | 89 | 170 | 1890 | 0 | 0 | 1520 | 1669 | 149 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | C | 2588 | 5197 | 36341 | 0 | 0 | 182510 | 185804 | 3294 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-coprime | A | 87 | 163 | 1819 | 64 | 512 | 0 | 0 | 0 | 2304 | 257 | 12 (12) | 2620 |
| 12 | Pack-coprime | B | 89 | 170 | 1890 | 65 | 520 | 0 | 0 | 0 | 2260 | 264 | 12 (11) | 2588 |
| 12 | Pack-coprime | C | 2588 | 5197 | 36341 | 1422 | 5688 | 0 | 0 | 0 | 1317717 | 8189 | 31 (31) | 1325479 |

### The factored rows' (f) and (q) at each matrix's end (after every reconciliation)

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 285 | 569 | 6265 | 0 | 0 |
| 1 | Pack-primes | B | 284 | 559 | 6171 | 0 | 0 |
| 1 | Pack-primes | C | 285 | 570 | 3994 | 0 | 0 |
| 1 | Pack-coprime | A | 285 | 568 | 6256 | 192 | 1536 |
| 1 | Pack-coprime | B | 284 | 559 | 6171 | 182 | 1456 |
| 1 | Pack-coprime | C | 285 | 564 | 3964 | 234 | 936 |
| 2 | Pack-primes | A | 87 | 182 | 1990 | 0 | 0 |
| 2 | Pack-primes | B | 86 | 189 | 2049 | 0 | 0 |
| 2 | Pack-primes | C | 281 | 584 | 4048 | 0 | 0 |
| 2 | Pack-coprime | A | 87 | 182 | 1990 | 60 | 480 |
| 2 | Pack-coprime | B | 86 | 188 | 2040 | 62 | 496 |
| 2 | Pack-coprime | C | 281 | 580 | 4028 | 215 | 860 |
| 3 | Pack-primes | A | 27 | 49 | 553 | 0 | 0 |
| 3 | Pack-primes | B | 27 | 46 | 526 | 0 | 0 |
| 3 | Pack-primes | C | 181 | 372 | 2588 | 0 | 0 |
| 3 | Pack-coprime | A | 27 | 46 | 526 | 21 | 168 |
| 3 | Pack-coprime | B | 27 | 44 | 508 | 21 | 168 |
| 3 | Pack-coprime | C | 181 | 370 | 2578 | 146 | 584 |
| 4 | Pack-primes | A | 7 | 15 | 167 | 0 | 0 |
| 4 | Pack-primes | B | 8 | 12 | 144 | 0 | 0 |
| 4 | Pack-primes | C | 26 | 55 | 383 | 0 | 0 |
| 4 | Pack-coprime | A | 7 | 14 | 158 | 9 | 72 |
| 4 | Pack-coprime | B | 8 | 11 | 135 | 7 | 56 |
| 4 | Pack-coprime | C | 26 | 50 | 358 | 21 | 84 |
| 5 | Pack-primes | A | 1022 | 2067 | 22695 | 0 | 0 |
| 5 | Pack-primes | B | 1012 | 2005 | 22097 | 0 | 0 |
| 5 | Pack-primes | C | 1044 | 2135 | 14855 | 0 | 0 |
| 5 | Pack-coprime | A | 1022 | 2067 | 22695 | 465 | 3720 |
| 5 | Pack-coprime | B | 1012 | 2005 | 22097 | 473 | 3784 |
| 5 | Pack-coprime | C | 1044 | 2135 | 14855 | 647 | 2588 |
| 6 | Pack-primes | A | 283 | 564 | 6212 | 0 | 0 |
| 6 | Pack-primes | B | 284 | 562 | 6198 | 0 | 0 |
| 6 | Pack-primes | C | 1031 | 2064 | 14448 | 0 | 0 |
| 6 | Pack-coprime | A | 283 | 564 | 6212 | 190 | 1520 |
| 6 | Pack-coprime | B | 284 | 562 | 6198 | 185 | 1480 |
| 6 | Pack-coprime | C | 1031 | 2061 | 14433 | 676 | 2704 |
| 7 | Pack-primes | A | 91 | 172 | 1916 | 0 | 0 |
| 7 | Pack-primes | B | 85 | 174 | 1910 | 0 | 0 |
| 7 | Pack-primes | C | 927 | 1882 | 13122 | 0 | 0 |
| 7 | Pack-coprime | A | 91 | 170 | 1898 | 66 | 528 |
| 7 | Pack-coprime | B | 85 | 173 | 1901 | 61 | 488 |
| 7 | Pack-coprime | C | 927 | 1880 | 13112 | 581 | 2324 |
| 8 | Pack-primes | A | 27 | 53 | 589 | 0 | 0 |
| 8 | Pack-primes | B | 27 | 56 | 616 | 0 | 0 |
| 8 | Pack-primes | C | 280 | 567 | 3959 | 0 | 0 |
| 8 | Pack-coprime | A | 27 | 51 | 571 | 21 | 168 |
| 8 | Pack-coprime | B | 27 | 52 | 580 | 20 | 160 |
| 8 | Pack-coprime | C | 280 | 565 | 3949 | 226 | 904 |
| 9 | Pack-primes | A | 3559 | 7347 | 80363 | 0 | 0 |
| 9 | Pack-primes | B | 3543 | 7186 | 78850 | 0 | 0 |
| 9 | Pack-primes | C | 4013 | 8111 | 56611 | 0 | 0 |
| 9 | Pack-coprime | A | 3559 | 7347 | 80363 | 1083 | 8664 |
| 9 | Pack-coprime | B | 3543 | 7186 | 78850 | 1126 | 9008 |
| 9 | Pack-coprime | C | 4013 | 8111 | 56611 | 1880 | 7520 |
| 10 | Pack-primes | A | 1013 | 2021 | 22245 | 0 | 0 |
| 10 | Pack-primes | B | 1007 | 1997 | 22005 | 0 | 0 |
| 10 | Pack-primes | C | 3988 | 7953 | 55721 | 0 | 0 |
| 10 | Pack-coprime | A | 1013 | 2021 | 22245 | 461 | 3688 |
| 10 | Pack-coprime | B | 1007 | 1997 | 22005 | 466 | 3728 |
| 10 | Pack-coprime | C | 3988 | 7953 | 55721 | 1916 | 7664 |
| 11 | Pack-primes | A | 279 | 534 | 5926 | 0 | 0 |
| 11 | Pack-primes | B | 281 | 564 | 6204 | 0 | 0 |
| 11 | Pack-primes | C | 3894 | 7926 | 55210 | 0 | 0 |
| 11 | Pack-coprime | A | 279 | 533 | 5917 | 177 | 1416 |
| 11 | Pack-coprime | B | 281 | 564 | 6204 | 181 | 1448 |
| 11 | Pack-coprime | C | 3894 | 7926 | 55210 | 1836 | 7344 |
| 12 | Pack-primes | A | 87 | 164 | 1828 | 0 | 0 |
| 12 | Pack-primes | B | 89 | 170 | 1890 | 0 | 0 |
| 12 | Pack-primes | C | 2588 | 5197 | 36341 | 0 | 0 |
| 12 | Pack-coprime | A | 87 | 163 | 1819 | 64 | 512 |
| 12 | Pack-coprime | B | 89 | 170 | 1890 | 65 | 520 |
| 12 | Pack-coprime | C | 2588 | 5197 | 36341 | 1422 | 5688 |

## Also reported (rule-m3 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-pack's stores, from the model; the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | W_A | W_B | W_C |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 285 | 284 | 285 |
| 2 | 16 | 4 | 64 | 64 | 64 | 252 | 87 | 86 | 281 |
| 3 | 16 | 16 | 16 | 16 | 16 | 153 | 27 | 27 | 181 |
| 4 | 16 | 64 | 4 | 4 | 4 | 16 | 7 | 8 | 26 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1022 | 1012 | 1044 |
| 6 | 32 | 4 | 256 | 256 | 256 | 1008 | 283 | 284 | 1031 |
| 7 | 32 | 16 | 64 | 64 | 64 | 897 | 91 | 85 | 927 |
| 8 | 32 | 64 | 16 | 16 | 16 | 252 | 27 | 27 | 280 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4095 | 3559 | 3543 | 4013 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 4086 | 1013 | 1007 | 3988 |
| 11 | 64 | 16 | 256 | 256 | 256 | 3978 | 279 | 281 | 3894 |
| 12 | 64 | 64 | 64 | 64 | 64 | 2601 | 87 | 89 | 2588 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.3 §16.9)

- job 1, C = A + B: add (256): int_both 0.008, int_x 0.086, int_y 0.070, den 0.027, general 0.809; cross tests 1024 (matched 9, cross reads 1024); C's store W 285, found / new 227 / 285
- job 2, C = A + B: add (256): int_both 0.023, int_x 0.098, int_y 0.113, den 0.043, general 0.723; cross tests 1024 (matched 17, cross reads 1024); C's store W 281, found / new 231 / 281
- job 3, C = A + B: add (256): int_both 0.012, int_x 0.047, int_y 0.125, den 0.027, general 0.789; cross tests 1024 (matched 10, cross reads 1024); C's store W 181, found / new 331 / 181
- job 4, C = A + B: add (256): general 1.000; cross tests 1024 (matched 0, cross reads 1024); C's store W 26, found / new 486 / 26
- job 5, C = A + B: add (1024): int_both 0.011, int_x 0.090, int_y 0.106, den 0.034, general 0.759; cross tests 4096 (matched 46, cross reads 4096); C's store W 1044, found / new 1004 / 1044
- job 6, C = A + B: add (1024): int_both 0.012, int_x 0.107, int_y 0.086, den 0.030, general 0.765; cross tests 4096 (matched 43, cross reads 4096); C's store W 1031, found / new 1017 / 1031
- job 7, C = A + B: add (1024): int_both 0.003, int_x 0.089, int_y 0.053, den 0.038, general 0.817; cross tests 4096 (matched 42, cross reads 4096); C's store W 927, found / new 1121 / 927
- job 8, C = A + B: add (1024): int_both 0.032, int_x 0.201, int_y 0.112, den 0.012, general 0.643; cross tests 4096 (matched 45, cross reads 4096); C's store W 280, found / new 1768 / 280
- job 9, C = A + B: add (4096): int_both 0.012, int_x 0.101, int_y 0.100, cross 0.000, den 0.039, general 0.748; cross tests 16384 (matched 210, cross reads 16384); C's store W 4013, found / new 4179 / 4013
- job 10, C = A + B: add (4096): int_both 0.012, int_x 0.103, int_y 0.100, den 0.038, general 0.747; cross tests 16384 (matched 207, cross reads 16384); C's store W 3988, found / new 4204 / 3988
- job 11, C = A + B: add (4096): int_both 0.013, int_x 0.116, int_y 0.108, den 0.031, general 0.733; cross tests 16384 (matched 179, cross reads 16384); C's store W 3894, found / new 4298 / 3894
- job 12, C = A + B: add (4096): int_both 0.008, int_x 0.052, int_y 0.136, den 0.029, general 0.775; cross tests 16384 (matched 150, cross reads 16384); C's store W 2588, found / new 5604 / 2588

### Per step: the factored arithmetic (rule-m3 item 13; counting runs)

Reconciliation (Pack-coprime): passes, gcds, splits, |D_L| and |D_R|, (f) rebuilds (entries written). The arithmetic: merges (entries compared), the temporaries' lists (comparisons forming them), divisibility tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, and the new sub-cases (9a-c num, 10a-c general, 12a-c multiplication's general).

| job | row | step | reconciliation | merges (entries) | list comparisons | tests | per-piece gcds | factor multiplications | leftover splits | 9a / 9b / 9c | 10a / 10b / 10c | 12a / 12b / 12c |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 207 (349) | 0 | 118 | 0 | 353 | 0 | 0 / 0 / 0 | 51 / 81 / 75 | 0 / 0 / 0 |
| 1 | Pack-coprime | C | 1 passes, 1122 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 207 (349) | 0 | 0 | 95 | 513 | 0 | 0 / 0 / 0 | 51 / 81 / 75 | 0 / 0 / 0 |
| 2 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 185 (298) | 0 | 133 | 0 | 290 | 0 | 0 / 0 / 0 | 43 / 59 / 83 | 0 / 0 / 0 |
| 2 | Pack-coprime | C | 1 passes, 366 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 185 (298) | 0 | 0 | 106 | 481 | 0 | 0 / 0 / 0 | 43 / 59 / 83 | 0 / 0 / 0 |
| 3 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 202 (352) | 0 | 149 | 0 | 368 | 0 | 0 / 0 / 0 | 46 / 63 / 93 | 0 / 0 / 0 |
| 3 | Pack-coprime | C | 1 passes, 126 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 202 (352) | 0 | 0 | 107 | 564 | 0 | 0 / 0 / 0 | 46 / 63 / 93 | 0 / 0 / 0 |
| 4 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 256 (499) | 0 | 246 | 0 | 557 | 0 | 0 / 0 / 0 | 65 / 49 / 142 | 0 / 0 / 0 |
| 4 | Pack-coprime | C | 1 passes, 48 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 256 (499) | 0 | 0 | 168 | 844 | 0 | 0 / 0 / 0 | 65 / 49 / 142 | 0 / 0 / 0 |
| 5 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 777 (1343) | 0 | 570 | 0 | 1391 | 0 | 0 / 0 / 0 | 166 / 264 / 347 | 0 / 0 / 0 |
| 5 | Pack-coprime | C | 1 passes, 2814 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 777 (1343) | 0 | 0 | 438 | 2137 | 0 | 0 / 0 / 0 | 166 / 264 / 347 | 0 / 0 / 0 |
| 6 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 783 (1351) | 0 | 552 | 0 | 1436 | 0 | 0 / 0 / 0 | 168 / 286 / 329 | 0 / 0 / 0 |
| 6 | Pack-coprime | C | 1 passes, 1125 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 783 (1351) | 0 | 0 | 422 | 2182 | 0 | 0 / 0 / 0 | 168 / 286 / 329 | 0 / 0 / 0 |
| 7 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 837 (1411) | 0 | 542 | 0 | 1419 | 0 | 0 / 0 / 0 | 180 / 326 / 331 | 0 / 0 / 0 |
| 7 | Pack-coprime | C | 1 passes, 381 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 837 (1411) | 0 | 0 | 409 | 2121 | 0 | 0 / 0 / 0 | 180 / 326 / 331 | 0 / 0 / 0 |
| 8 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 658 (1070) | 0 | 313 | 0 | 890 | 0 | 0 / 0 / 0 | 180 / 261 / 217 | 0 / 0 / 0 |
| 8 | Pack-coprime | C | 1 passes, 123 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 658 (1070) | 0 | 0 | 245 | 1310 | 0 | 0 / 0 / 0 | 180 / 261 / 217 | 0 / 0 / 0 |
| 9 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3062 (5319) | 0 | 2130 | 0 | 5274 | 0 | 0 / 0 / 0 | 674 / 1082 / 1306 | 0 / 0 / 0 |
| 9 | Pack-coprime | C | 1 passes, 6627 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3062 (5319) | 0 | 0 | 1704 | 8240 | 0 | 0 / 0 / 0 | 674 / 1082 / 1306 | 0 / 0 / 0 |
| 10 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3060 (5221) | 0 | 2023 | 0 | 5213 | 0 | 0 / 0 / 0 | 666 / 1131 / 1263 | 0 / 0 / 0 |
| 10 | Pack-coprime | C | 1 passes, 2781 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3060 (5221) | 0 | 0 | 1602 | 8007 | 0 | 0 / 0 / 0 | 666 / 1131 / 1263 | 0 / 0 / 0 |
| 11 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3001 (5323) | 0 | 2102 | 0 | 5216 | 0 | 0 / 0 / 0 | 700 / 963 / 1338 | 0 / 0 / 0 |
| 11 | Pack-coprime | C | 1 passes, 1074 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3001 (5323) | 0 | 0 | 1654 | 8121 | 0 | 0 / 0 / 0 | 700 / 963 / 1338 | 0 / 0 / 0 |
| 12 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3174 (5579) | 0 | 2010 | 0 | 5353 | 0 | 0 / 0 / 0 | 703 / 1150 / 1321 | 0 / 0 / 0 |
| 12 | Pack-coprime | C | 1 passes, 387 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3174 (5579) | 0 | 0 | 1619 | 8202 | 0 | 0 / 0 / 0 | 703 / 1150 / 1321 | 0 / 0 / 0 |

The gates checked every counter and store figure against the model (spec v0.3 §16.9 item 2), the TC gate included: MHVP:1 pass, TC:1 pass, MHPP:1 pass, TC:2 pass, MHVP:2 pass, MHPC:1 pass, MHPP:2 pass, TC:3 pass, MHPC:2 pass, MHVP:3 pass, MHPC:3 pass, MHPP:3 pass, MHVP:4 pass, TC:4 pass, MHPP:4 pass, MHPC:4 pass, TC:5 pass, MHVP:5 pass, MHPP:5 pass, TC:6 pass, MHVP:6 pass, MHPP:6 pass, MHPC:5 pass, TC:7 pass, MHPC:6 pass, MHVP:7 pass, MHPP:7 pass, TC:8 pass, MHPC:7 pass, MHVP:8 pass, MHPP:8 pass, MHPC:8 pass, MHVP:9 pass, TC:9 pass, MHPP:9 pass, TC:10 pass, MHVP:10 pass, MHPP:10 pass, MHPC:10 pass, MHPC:9 pass, TC:11 pass, MHVP:11 pass, MHPP:11 pass, TC:12 pass, MHVP:12 pass, MHPP:12 pass, MHPC:12 pass, MHPC:11 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m3 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 2.34 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 2.899 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 10 | pilot | 1 | 3.077 | no | 1 | 14 | 0.071 | 14 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (8->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 12.858 | no | 9 | 64 | 0.141 | 64 | 0 | none | 18 | none | process_api 1/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 0/14 (8->8 threads), claude 1/16 (8->7 threads), bash 0/14 (1->1 threads) |
| 2 | measured | 1 | 12.831 | no | 7 | 64 | 0.109 | 66 | 2 | none | 18 | none | process_api 0/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 1/14 (8->8 threads), claude 1/16 (7->8 threads), bash 0/14 (1->1 threads) |
| 3 | measured | 1 | 16.925 | no | 7 | 64 | 0.109 | 65 | 1 | none | 22 | none | process_api 1/17 (6->6 threads), sh 0/17 (1->1 threads), environment-man 0/17 (8->8 threads), claude 3/20 (8->6 threads), bash 0/17 (1->1 threads) |
| 4 | measured | 1 | 15.537 | no | 8 | 64 | 0.125 | 66 | 2 | none | 21 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (8->8 threads), claude 2/19 (6->7 threads), bash 0/16 (1->1 threads) |
| 5 | measured | 1 | 15.208 | no | 8 | 64 | 0.125 | 64 | 0 | none | 21 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/18 (7->7 threads), bash 0/15 (1->1 threads) |
| 6 | measured | 1 | 14.366 | no | 8 | 64 | 0.125 | 64 | 0 | none | 20 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/18 (8->6 threads), bash 0/15 (1->1 threads) |
| 7 | measured | 1 | 14.061 | no | 8 | 64 | 0.125 | 66 | 2 | none | 20 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/15 (8->8 threads), claude 2/18 (6->8 threads), bash 0/15 (1->1 threads) |
| 8 | measured | 1 | 13.058 | no | 7 | 64 | 0.109 | 64 | 0 | none | 19 | none | process_api 0/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 0/14 (8->8 threads), claude 1/17 (8->7 threads), bash 0/14 (1->1 threads) |
| 9 | measured | 1 | 18.503 | no | 7 | 64 | 0.109 | 66 | 2 | none | 24 | none | process_api 1/18 (6->6 threads), sh 0/18 (1->1 threads), environment-man 0/18 (8->8 threads), claude 2/21 (7->8 threads), bash 0/18 (1->1 threads) |
| 10 | measured | 1 | 15.693 | no | 8 | 64 | 0.125 | 66 | 2 | none | 21 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (8->8 threads), claude 2/19 (8->8 threads), bash 0/16 (1->1 threads) |
| 11 | measured | 1 | 16.375 | no | 7 | 64 | 0.109 | 65 | 1 | none | 22 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (8->8 threads), claude 2/19 (8->6 threads), bash 0/16 (1->1 threads) |
| 12 | measured | 1 | 16.062 | no | 8 | 64 | 0.125 | 66 | 2 | none | 22 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/19 (6->8 threads), bash 0/16 (1->1 threads) |

The runner's own ticks over the 15 blocks: 97 ticks over 810 launches, 0.120 per launch; the highest block 0.143 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9991 | 0.9908 | 10647 | 10496 |
| 1 | Hash-V-pack | 12 | 0.9998 | 0.9851 | 26793 | 26630 |
| 1 | Pack-primes | 12 | 0.9998 | 0.9913 | 124754 | 124775 |
| 1 | Pack-coprime | 12 | 0.9999 | 0.9954 | 2109853 | 2109691 |
| 1 | Hash-V-pack-null | 12 | 0.9993 | 0.9871 | 26696 | 26465 |
| 2 | TC | 12 | 0.9994 | 0.9935 | 10189 | 10044 |
| 2 | Hash-V-pack | 12 | 0.9996 | 0.9756 | 24951 | 24746 |
| 2 | Pack-primes | 12 | 0.9995 | 0.9858 | 78500 | 78161 |
| 2 | Pack-coprime | 12 | 0.9998 | 0.9920 | 906203 | 906841 |
| 2 | Hash-V-pack-null | 12 | 0.9998 | 0.9891 | 24928 | 24733 |
| 3 | TC | 12 | 0.9995 | 0.9935 | 10580 | 10443 |
| 3 | Hash-V-pack | 12 | 0.9992 | 0.9766 | 23295 | 23115 |
| 3 | Pack-primes | 12 | 0.9998 | 0.9975 | 56078 | 55812 |
| 3 | Pack-coprime | 12 | 0.9997 | 0.9968 | 382430 | 382118 |
| 3 | Hash-V-pack-null | 12 | 0.9995 | 0.9979 | 23144 | 22974 |
| 4 | TC | 12 | 0.9998 | 0.9974 | 12516 | 12348 |
| 4 | Hash-V-pack | 12 | 0.9987 | 0.9675 | 24548 | 24407 |
| 4 | Pack-primes | 12 | 0.9996 | 0.9886 | 31435 | 31217 |
| 4 | Pack-coprime | 12 | 0.9999 | 0.9673 | 38373 | 38088 |
| 4 | Hash-V-pack-null | 12 | 0.9995 | 0.9932 | 24591 | 24464 |
| 5 | TC | 12 | 0.9994 | 0.9953 | 79366 | 79257 |
| 5 | Hash-V-pack | 12 | 0.9998 | 0.9949 | 170829 | 170678 |
| 5 | Pack-primes | 12 | 0.9995 | 0.9943 | 530811 | 531157 |
| 5 | Pack-coprime | 12 | 0.9991 | 0.9943 | 16443532 | 16461926 |
| 5 | Hash-V-pack-null | 12 | 0.9994 | 0.9925 | 170937 | 170870 |
| 6 | TC | 12 | 0.9995 | 0.9875 | 79984 | 80385 |
| 6 | Hash-V-pack | 12 | 1.0000 | 0.9970 | 154723 | 154586 |
| 6 | Pack-primes | 12 | 0.9999 | 0.9907 | 404207 | 403862 |
| 6 | Pack-coprime | 12 | 0.9997 | 0.9979 | 9574399 | 9576166 |
| 6 | Hash-V-pack-null | 12 | 0.9995 | 0.8839 | 155066 | 154879 |
| 7 | TC | 12 | 0.9998 | 0.9929 | 77372 | 77256 |
| 7 | Hash-V-pack | 12 | 0.9998 | 0.9975 | 138287 | 138122 |
| 7 | Pack-primes | 12 | 1.0000 | 0.9985 | 337489 | 337086 |
| 7 | Pack-coprime | 12 | 1.0000 | 0.9791 | 6647401 | 6648119 |
| 7 | Hash-V-pack-null | 12 | 0.9995 | 0.9962 | 139101 | 138886 |
| 8 | TC | 12 | 0.9996 | 0.9921 | 59083 | 58871 |
| 8 | Hash-V-pack | 12 | 0.9998 | 0.9975 | 116373 | 116168 |
| 8 | Pack-primes | 12 | 0.9998 | 0.9992 | 176928 | 176608 |
| 8 | Pack-coprime | 12 | 0.9995 | 0.9929 | 1020537 | 1021389 |
| 8 | Hash-V-pack-null | 12 | 0.9997 | 0.9983 | 116082 | 115875 |
| 9 | TC | 12 | 1.0000 | 0.9885 | 368659 | 368540 |
| 9 | Hash-V-pack | 12 | 0.9999 | 0.9959 | 756863 | 756546 |
| 9 | Pack-primes | 12 | 0.9999 | 0.9695 | 2128862 | 2128143 |
| 9 | Pack-coprime | 12 | 0.9998 | 0.9754 | 126480536 | 126511157 |
| 9 | Hash-V-pack-null | 12 | 0.9999 | 0.9990 | 753525 | 753150 |
| 10 | TC | 12 | 0.9998 | 0.9956 | 369273 | 369031 |
| 10 | Hash-V-pack | 12 | 0.9997 | 0.9979 | 688124 | 687932 |
| 10 | Pack-primes | 12 | 1.0000 | 0.9931 | 1610783 | 1610964 |
| 10 | Pack-coprime | 12 | 0.9997 | 0.9788 | 83184257 | 83206580 |
| 10 | Hash-V-pack-null | 12 | 0.9997 | 0.9865 | 688413 | 688409 |
| 11 | TC | 12 | 0.9999 | 0.9965 | 360888 | 360757 |
| 11 | Hash-V-pack | 12 | 0.9996 | 0.9978 | 643457 | 643202 |
| 11 | Pack-primes | 12 | 0.9997 | 0.9861 | 1484263 | 1483991 |
| 11 | Pack-coprime | 12 | 0.9997 | 0.9966 | 72294207 | 72437881 |
| 11 | Hash-V-pack-null | 12 | 0.9997 | 0.8254 | 645906 | 645765 |
| 12 | TC | 12 | 0.9997 | 0.9985 | 364974 | 364809 |
| 12 | Hash-V-pack | 12 | 0.9996 | 0.9707 | 636308 | 635993 |
| 12 | Pack-primes | 12 | 0.9997 | 0.9951 | 1200862 | 1200953 |
| 12 | Pack-coprime | 12 | 0.9997 | 0.9973 | 40958434 | 40972259 |
| 12 | Hash-V-pack-null | 12 | 0.9990 | 0.9846 | 636111 | 636430 |

Idle checks: 15 (0 busy). Host rates R_h: {'1': 0.32499187520311995, '79': 0.32499187520311995, '84': 0.32499187520311995, '92': 0.42738836610428654, '19297': 0.32499187520311995} (measured: {'1': 0.0, '79': 0.0, '84': 0.0, '92': 0.42738836610428654, '19297': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

