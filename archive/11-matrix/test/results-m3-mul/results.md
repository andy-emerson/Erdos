# Design 3 on matrices, round m3 (factored whole numbers), matrix multiplication: results

Run started 2026-10-30T14:40:47-05:00.

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

- The mul run (job set `mul`: C = A x B).
- Started 2026-10-30T14:40:47-05:00; finished 2026-10-30T14:51:29-05:00.
- Step 0 (before the clock): 53.8 s; step 0's files: equal to m1's by sha256.
- Run time 9.8 minutes (hard stop 40).
- Rule sha256 `c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd`; round 3's results after the run: unchanged; m1's and m2's results and the other m3 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[20580], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T14:41:41-05:00: TC `3cbbe6f47fb7`, MHVP `7ed2109b9193`, MHPP `6704a1ab9994`, MHPC `0da57d01e10e`, TC3GATE `685595cafdb3`
- Unit tests: m3_unit: m3_unit: 811293 checks, 0 failures; m3_diff: m3_diff: 3000 seeds, 3000 jobs, 1050092 operations; obtained gcds checked against Euclid 2936692, merges 2186690, reconciliation splits 2796, leftover splits 106; 1523395 checks, 0 failures; m3_alloc-timing: m3_alloc (timing build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-counting: m3_alloc (counting build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-memory: m3_alloc (memory build): 160 jobs, 2893 moments, 43830 checks, 0 failures; m3_unit-asan: m3_unit: 811293 checks, 0 failures; m3_diff-asan: m3_diff: 600 seeds, 600 jobs, 209812 operations; obtained gcds checked against Euclid 579628, merges 431634, reconciliation splits 557, leftover splits 20; 303709 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 10], one per size; block wall times job 1 4.6 s, job 6 7.5 s, job 10 17.5 s.
- Per size class c (its pilot job): O_c the block's checks, K the calibration launches, P the two round launches (0 over cap), t the time per run from the calibration; N_c the run's jobs of that size:
  - size 0 (job 1, N_c = 4): O 2.0 s; TC: K 0.12 s, P 0.37 s, t 0.000 s; Hash-V-pack: K 0.14 s, P 0.41 s, t 0.001 s; Pack-primes: K 0.08 s, P 0.44 s, t 0.072 s; Pack-coprime: K 0.14 s, P 0.44 s, t 0.004 s; Hash-V-pack-null: K 0.00 s, P 0.42 s, t n/a s
  - size 1 (job 6, N_c = 4): O 2.0 s; TC: K 0.13 s, P 0.39 s, t 0.004 s; Hash-V-pack: K 0.13 s, P 0.41 s, t 0.004 s; Pack-primes: K 0.47 s, P 2.83 s, t 0.465 s; Pack-coprime: K 0.14 s, P 0.53 s, t 0.044 s; Hash-V-pack-null: K 0.00 s, P 0.43 s, t n/a s
  - size 2 (job 10, N_c = 4): O 2.0 s; TC: K 0.09 s, P 0.38 s, t 0.030 s; Hash-V-pack: K 0.11 s, P 0.46 s, t 0.034 s; Pack-primes: K 3.21 s, P 6.38 s, t 3.203 s; Pack-coprime: K 0.63 s, P 3.78 s, t 0.623 s; Hash-V-pack-null: K 0.00 s, P 0.42 s, t n/a s
- Projection elapsed + 1.2 [sum_c N_c (O_c + sum_rows (K + 6 P)) + 1/2 sum_c N_c sum_rows 4 t] = 11.9 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.06469142191745375, '79': 0.06469142191745375, '84': 0.06469142191745375, '92': 0.06469142191745375, '20580': 0.06469142191745375}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 3 multiplication time; 6 stable bytes of one stored input matrix; 8 peak bytes during the operation. Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-pack against Hash-V-pack-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses; (f) and (q) included). Pack-coprime against Pack-primes is reported with no verdict on any measure. A row over cap runs no rounds: its time verdicts are over cap, with its calibration rep's time; its memory verdicts stand. Gcd work: a check for Hash-V-pack, reported for the factored rows. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-pack, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m3 item 8, carried over).

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.502 [0.495, 0.506], h 0.050 | loss, 0.862 [0.857, 0.865], h 0.050 | reported, 0.843 [0.838, 0.846] | reported, 0.573 [0.552, 0.667] | win, 0.803 (heap 0.818) | loss, 1.654 (heap 1.655) |
| 2 (n 16, rho 4) | loss, 0.584 [0.573, 0.591], h 0.050 | loss, 0.862 [0.857, 0.865], h 0.050 | reported, 0.848 [0.845, 0.851] | reported, 0.556 [0.552, 0.566] | win, 0.299 (heap 0.326) | loss, 1.319 (heap 1.328) |
| 3 (n 16, rho 16) | loss, 0.658 [0.653, 0.666], h 0.050 | loss, 0.843 [0.840, 0.850], h 0.050 | reported, 0.836 [0.832, 0.842] | reported, 0.590 [0.558, 0.663] | win, 0.174 (heap 0.203) | loss, 1.238 (heap 1.247) |
| 4 (n 16, rho 64) | loss, 0.616 [0.601, 0.626], h 0.050 | loss, 0.840 [0.834, 0.851], h 0.050 | reported, 0.831 [0.826, 0.841] | reported, 0.558 [0.547, 0.580] | win, 0.140 (heap 0.169) | loss, 1.201 (heap 1.211) |
| 5 (n 32, rho 1) | loss, 0.499 [0.493, 0.502], h 0.050 | loss, 0.888 [0.882, 0.893], h 0.050 | reported, 0.878 [0.867, 0.882] | reported, 0.595 [0.578, 0.714] | win, 0.744 (heap 0.748) | loss, 1.590 (heap 1.591) |
| 6 (n 32, rho 4) | loss, 0.562 [0.556, 0.565], h 0.050 | loss, 0.887 [0.882, 0.890], h 0.050 | reported, 0.880 [0.875, 0.882] | reported, 0.585 [0.579, 0.708] | win, 0.387 (heap 0.394) | loss, 1.354 (heap 1.356) |
| 7 (n 32, rho 16) | loss, 0.627 [0.621, 0.631], h 0.050 | loss, 0.887 [0.885, 0.890], h 0.050 | reported, 0.882 [0.879, 0.884] | reported, 0.593 [0.577, 0.771] | win, 0.168 (heap 0.175) | loss, 1.208 (heap 1.210) |
| 8 (n 32, rho 64) | loss, 0.640 [0.632, 0.645], h 0.050 | loss, 0.877 [0.876, 0.880], h 0.050 | reported, 0.872 [0.871, 0.875] | reported, 0.615 [0.580, 0.776] | win, 0.139 (heap 0.146) | loss, 1.186 (heap 1.188) |
| 9 (n 64, rho 1) | loss, 0.508 [0.504, 0.511], h 0.050 | loss, 0.895 [0.892, 0.899], h 0.050 | reported, 0.889 [0.886, 0.894] | reported, 0.780 [0.687, 0.915] | win, 0.685 (heap 0.686) | loss, 1.544 (heap 1.544) |
| 10 (n 64, rho 4) | loss, 0.587 [0.582, 0.597], h 0.050 | loss, 0.890 [0.886, 0.900], h 0.050 | reported, 0.887 [0.883, 0.896] | reported, 0.699 [0.610, 0.853] | win, 0.373 (heap 0.375) | loss, 1.336 (heap 1.336) |
| 11 (n 64, rho 16) | loss, 0.645 [0.637, 0.656], h 0.050 | loss, 0.897 [0.892, 0.903], h 0.050 | reported, 0.894 [0.890, 0.901] | reported, 0.699 [0.631, 0.727] | win, 0.284 (heap 0.286) | loss, 1.277 (heap 1.277) |
| 12 (n 64, rho 64) | loss, 0.662 [0.655, 0.685], h 0.050 | loss, 0.891 [0.887, 0.894], h 0.050 | reported, 0.889 [0.885, 0.892] | reported, 0.667 [0.635, 0.760] | win, 0.136 (heap 0.138) | loss, 1.178 (heap 1.179) |

### Pack-primes against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.175 [0.172, 0.181], h 0.050 | loss, 0.006 [0.006, 0.006], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.336 [0.255, 0.476] | loss, 2.330 (heap 2.330) **severe memory loss** | loss, 2.852 (heap 2.842) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.346 [0.339, 0.349], h 0.050 | loss, 0.006 [0.006, 0.007], h 0.050 | reported, 0.007 [0.007, 0.007] | reported, 0.386 [0.289, 0.470] | win, 0.755 (heap 0.787) | loss, 1.806 (heap 1.817) |
| 3 (n 16, rho 16) | loss, 0.481 [0.460, 0.487], h 0.050 | loss, 0.008 [0.007, 0.008], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.408 [0.339, 0.488] | win, 0.304 (heap 0.343) | loss, 1.517 (heap 1.533) |
| 4 (n 16, rho 64) | loss, 0.514 [0.491, 0.523], h 0.050 | loss, 0.009 [0.009, 0.009], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.351 [0.318, 0.504] | win, 0.180 (heap 0.224) | loss, 1.352 (heap 1.373) |
| 5 (n 32, rho 1) | loss, 0.213 [0.212, 0.215], h 0.050 | loss, 0.008 [0.008, 0.008], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.277 [0.261, 0.343] | loss, 2.117 (heap 2.119) **severe memory loss** | loss, 2.563 (heap 2.562) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.405 [0.399, 0.406], h 0.050 | loss, 0.008 [0.008, 0.008], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.339 [0.320, 0.451] | win, 0.760 (heap 0.768) | loss, 1.666 (heap 1.670) |
| 7 (n 32, rho 16) | loss, 0.580 [0.562, 0.583], h 0.050 | loss, 0.012 [0.012, 0.012], h 0.050 | reported, 0.012 [0.012, 0.012] | reported, 0.339 [0.306, 0.425] | win, 0.283 (heap 0.293) | loss, 1.348 (heap 1.353) |
| 8 (n 32, rho 64) | loss, 0.625 [0.611, 0.631], h 0.050 | loss, 0.006 [0.006, 0.006], h 0.050 | reported, 0.006 [0.006, 0.006] | reported, 0.281 [0.250, 0.393] | win, 0.173 (heap 0.184) | loss, 1.260 (heap 1.265) |
| 9 (n 64, rho 1) | loss, 0.206 [0.199, 0.211], h 0.050 (c) | loss, 0.010 [0.010, 0.010], h 0.050 (c) | reported, 0.010 [0.010, 0.010] | reported, 0.184 [0.161, 0.216] | loss, 1.901 (heap 1.901) | loss, 2.373 (heap 2.373) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.366 [0.360, 0.380], h 0.050 (c) | loss, 0.009 [0.009, 0.009], h 0.050 (c) | reported, 0.009 [0.009, 0.010] | reported, 0.173 [0.142, 0.215] | win, 0.721 (heap 0.723) | loss, 1.587 (heap 1.587) |
| 11 (n 64, rho 16) | loss, 0.496 [0.488, 0.529], h 0.050 (c) | loss, 0.012 [0.012, 0.012], h 0.050 (c) | reported, 0.012 [0.012, 0.012] | reported, 0.158 [0.143, 0.165] | win, 0.379 (heap 0.381) | loss, 1.359 (heap 1.360) |
| 12 (n 64, rho 64) | loss, 0.590 [0.573, 0.595], h 0.050 (c) | loss, 0.011 [0.011, 0.011], h 0.050 (c) | reported, 0.011 [0.011, 0.011] | reported, 0.151 [0.129, 0.194] | win, 0.165 (heap 0.168) | loss, 1.218 (heap 1.220) |

### Pack-coprime against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.012 [0.012, 0.012], h 0.050 | loss, 0.134 [0.131, 0.134], h 0.050 | reported, 0.102 [0.100, 0.103] | reported, 0.516 [0.430, 0.605] | loss, 2.688 (heap 2.688) **severe memory loss** | loss, 3.293 (heap 3.282) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.090 [0.089, 0.091], h 0.050 | loss, 0.133 [0.132, 0.133], h 0.050 | reported, 0.131 [0.130, 0.132] | reported, 0.521 [0.502, 0.536] | win, 0.875 (heap 0.918) | loss, 2.088 (heap 2.105) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.328 [0.326, 0.336], h 0.050 | loss, 0.126 [0.125, 0.127], h 0.050 | reported, 0.128 [0.128, 0.129] | reported, 0.545 [0.506, 0.614] | win, 0.335 (heap 0.383) | loss, 1.740 (heap 1.761) |
| 4 (n 16, rho 64) | loss, 0.495 [0.488, 0.501], h 0.050 | loss, 0.128 [0.127, 0.130], h 0.050 | reported, 0.131 [0.130, 0.132] | reported, 0.516 [0.504, 0.544] | win, 0.189 (heap 0.239) | loss, 1.555 (heap 1.579) |
| 5 (n 32, rho 1) | loss, 0.007 [0.007, 0.007], h 0.050 | loss, 0.086 [0.086, 0.087], h 0.050 | reported, 0.074 [0.073, 0.074] | reported, 0.531 [0.487, 0.614] | loss, 2.342 (heap 2.345) **severe memory loss** | loss, 2.904 (heap 2.904) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.050 [0.049, 0.050], h 0.050 | loss, 0.086 [0.086, 0.086], h 0.050 | reported, 0.085 [0.085, 0.085] | reported, 0.541 [0.481, 0.663] | win, 0.847 (heap 0.857) | loss, 1.918 (heap 1.922) |
| 7 (n 32, rho 16) | loss, 0.278 [0.276, 0.280], h 0.050 | loss, 0.089 [0.089, 0.089], h 0.050 | reported, 0.090 [0.090, 0.090] | reported, 0.569 [0.535, 0.746] | win, 0.311 (heap 0.323) | loss, 1.557 (heap 1.563) |
| 8 (n 32, rho 64) | loss, 0.545 [0.538, 0.548], h 0.050 | loss, 0.083 [0.083, 0.083], h 0.050 | reported, 0.084 [0.084, 0.084] | reported, 0.561 [0.514, 0.725] | win, 0.183 (heap 0.195) | loss, 1.457 (heap 1.463) |
| 9 (n 64, rho 1) | loss, 0.004 [0.004, 0.004], h 0.050 | loss, 0.050 [0.049, 0.050], h 0.050 | reported, 0.046 [0.046, 0.046] | reported, 0.628 [0.491, 0.754] | loss, 2.037 (heap 2.038) **severe memory loss** | loss, 2.649 (heap 2.649) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.029 [0.029, 0.029], h 0.050 | loss, 0.049 [0.049, 0.050], h 0.050 | reported, 0.049 [0.049, 0.049] | reported, 0.593 [0.461, 0.719] | win, 0.777 (heap 0.779) | loss, 1.809 (heap 1.810) |
| 11 (n 64, rho 16) | loss, 0.167 [0.166, 0.169], h 0.050 | loss, 0.050 [0.050, 0.050], h 0.050 | reported, 0.050 [0.050, 0.051] | reported, 0.459 [0.389, 0.536] | win, 0.400 (heap 0.403) | loss, 1.558 (heap 1.559) |
| 12 (n 64, rho 64) | loss, 0.510 [0.505, 0.514], h 0.050 | loss, 0.049 [0.049, 0.049], h 0.050 | reported, 0.049 [0.049, 0.049] | reported, 0.492 [0.445, 0.566] | win, 0.172 (heap 0.175) | loss, 1.409 (heap 1.410) |

### Pack-primes against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.350 [0.344, 0.360], h 0.050 | loss, 0.007 [0.007, 0.007], h 0.050 | reported, 0.007 [0.007, 0.007] | reported, 0.586 [0.449, 0.754] | loss, 2.903 (heap 2.848) **severe memory loss** | loss, 1.724 (heap 1.717) |
| 2 (n 16, rho 4) | loss, 0.592 [0.581, 0.602], h 0.050 | loss, 0.008 [0.007, 0.008], h 0.050 | reported, 0.008 [0.008, 0.008] | reported, 0.695 [0.521, 0.845] | loss, 2.526 (heap 2.418) **severe memory loss** | loss, 1.369 (heap 1.369) |
| 3 (n 16, rho 16) | loss, 0.730 [0.698, 0.739], h 0.050 | loss, 0.009 [0.009, 0.009], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.691 [0.574, 0.828] | loss, 1.746 (heap 1.689) | loss, 1.225 (heap 1.229) |
| 4 (n 16, rho 64) | loss, 0.835 [0.798, 0.855], h 0.050 | loss, 0.011 [0.011, 0.011], h 0.050 | reported, 0.011 [0.011, 0.011] | reported, 0.628 [0.575, 0.918] | loss, 1.288 (heap 1.330) | loss, 1.126 (heap 1.133) |
| 5 (n 32, rho 1) | loss, 0.427 [0.424, 0.433], h 0.050 | loss, 0.009 [0.009, 0.009], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.465 [0.431, 0.563] | loss, 2.847 (heap 2.831) **severe memory loss** | loss, 1.612 (heap 1.610) |
| 6 (n 32, rho 4) | loss, 0.720 [0.712, 0.725], h 0.050 | loss, 0.009 [0.009, 0.009], h 0.050 | reported, 0.009 [0.009, 0.009] | reported, 0.581 [0.540, 0.717] | loss, 1.961 (heap 1.951) | loss, 1.230 (heap 1.231) |
| 7 (n 32, rho 16) | loss, 0.925 [0.898, 0.931], h 0.050 | loss, 0.014 [0.014, 0.014], h 0.050 | reported, 0.014 [0.014, 0.014] | reported, 0.572 [0.518, 0.670] | loss, 1.687 (heap 1.675) | loss, 1.116 (heap 1.118) |
| 8 (n 32, rho 64) | tie, 0.978 [0.964, 0.985], h 0.050 | loss, 0.007 [0.007, 0.007], h 0.050 | reported, 0.007 [0.007, 0.007] | reported, 0.457 [0.425, 0.580] | loss, 1.251 (heap 1.260) | tie, 1.062 (heap 1.064) |
| 9 (n 64, rho 1) | loss, 0.406 [0.394, 0.416], h 0.050 (c) | loss, 0.011 [0.011, 0.011], h 0.050 (c) | reported, 0.012 [0.011, 0.012] | reported, 0.235 [0.214, 0.242] | loss, 2.777 (heap 2.773) **severe memory loss** | loss, 1.538 (heap 1.537) |
| 10 (n 64, rho 4) | loss, 0.623 [0.605, 0.646], h 0.050 (c) | loss, 0.011 [0.010, 0.011], h 0.050 (c) | reported, 0.011 [0.011, 0.011] | reported, 0.248 [0.222, 0.259] | loss, 1.931 (heap 1.928) | loss, 1.188 (heap 1.188) |
| 11 (n 64, rho 16) | loss, 0.769 [0.749, 0.822], h 0.050 (c) | loss, 0.013 [0.013, 0.013], h 0.050 (c) | reported, 0.013 [0.013, 0.013] | reported, 0.226 [0.219, 0.231] | loss, 1.332 (heap 1.332) | tie, 1.064 (heap 1.064) |
| 12 (n 64, rho 64) | loss, 0.891 [0.855, 0.902], h 0.050 (c) | loss, 0.013 [0.012, 0.013], h 0.050 (c) | reported, 0.013 [0.013, 0.013] | reported, 0.226 [0.196, 0.266] | loss, 1.216 (heap 1.219) | tie, 1.034 (heap 1.035) |

### Pack-coprime against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.024 [0.024, 0.025], h 0.050 | loss, 0.155 [0.153, 0.156], h 0.050 | reported, 0.121 [0.119, 0.122] | reported, 0.900 [0.754, 0.941] | loss, 3.348 (heap 3.286) **severe memory loss** | loss, 1.990 (heap 1.983) |
| 2 (n 16, rho 4) | loss, 0.155 [0.153, 0.157], h 0.050 | loss, 0.154 [0.153, 0.155], h 0.050 | reported, 0.154 [0.153, 0.155] | reported, 0.937 [0.907, 0.955] | loss, 2.929 (heap 2.818) **severe memory loss** | loss, 1.583 (heap 1.586) |
| 3 (n 16, rho 16) | loss, 0.499 [0.496, 0.508], h 0.050 | loss, 0.149 [0.149, 0.150], h 0.050 | reported, 0.154 [0.153, 0.154] | reported, 0.923 [0.890, 0.944] | loss, 1.926 (heap 1.887) | loss, 1.406 (heap 1.412) |
| 4 (n 16, rho 64) | loss, 0.805 [0.795, 0.819], h 0.050 | loss, 0.152 [0.151, 0.153], h 0.050 | reported, 0.157 [0.156, 0.158] | reported, 0.925 [0.911, 0.964] | loss, 1.353 (heap 1.420) | loss, 1.294 (heap 1.304) |
| 5 (n 32, rho 1) | loss, 0.014 [0.014, 0.015], h 0.050 | loss, 0.097 [0.097, 0.098], h 0.050 | reported, 0.084 [0.084, 0.085] | reported, 0.892 [0.786, 0.931] | loss, 3.149 (heap 3.133) **severe memory loss** | loss, 1.826 (heap 1.825) |
| 6 (n 32, rho 4) | loss, 0.089 [0.088, 0.090], h 0.050 | loss, 0.097 [0.097, 0.097], h 0.050 | reported, 0.097 [0.096, 0.097] | reported, 0.925 [0.821, 0.976] | loss, 2.186 (heap 2.177) **severe memory loss** | loss, 1.416 (heap 1.418) |
| 7 (n 32, rho 16) | loss, 0.444 [0.442, 0.446], h 0.050 | loss, 0.100 [0.100, 0.101], h 0.050 | reported, 0.102 [0.102, 0.102] | reported, 0.960 [0.899, 1.011] | loss, 1.854 (heap 1.847) | loss, 1.289 (heap 1.291) |
| 8 (n 32, rho 64) | loss, 0.852 [0.844, 0.856], h 0.050 | loss, 0.095 [0.094, 0.095], h 0.050 | reported, 0.097 [0.096, 0.097] | reported, 0.912 [0.872, 0.966] | loss, 1.318 (heap 1.337) | loss, 1.229 (heap 1.231) |
| 9 (n 64, rho 1) | loss, 0.008 [0.008, 0.009], h 0.050 | loss, 0.056 [0.055, 0.056], h 0.050 | reported, 0.052 [0.051, 0.052] | reported, 0.804 [0.671, 0.858] | loss, 2.975 (heap 2.971) **severe memory loss** | loss, 1.716 (heap 1.716) |
| 10 (n 64, rho 4) | loss, 0.049 [0.048, 0.049], h 0.050 | loss, 0.056 [0.055, 0.056], h 0.050 | reported, 0.055 [0.055, 0.056] | reported, 0.848 [0.721, 0.897] | loss, 2.081 (heap 2.079) **severe memory loss** | loss, 1.354 (heap 1.355) |
| 11 (n 64, rho 16) | loss, 0.259 [0.254, 0.263], h 0.050 | loss, 0.056 [0.055, 0.056], h 0.050 | reported, 0.056 [0.056, 0.056] | reported, 0.657 [0.562, 0.779] | loss, 1.408 (heap 1.410) | loss, 1.220 (heap 1.221) |
| 12 (n 64, rho 64) | loss, 0.771 [0.743, 0.778], h 0.050 | loss, 0.055 [0.054, 0.055], h 0.050 | reported, 0.055 [0.055, 0.056] | reported, 0.737 [0.678, 0.778] | loss, 1.268 (heap 1.274) | loss, 1.196 (heap 1.197) |

### Pack-coprime against Pack-primes (coprime against primes)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 0.069 [0.068, 0.070] | reported, 22.313 [21.983, 22.450] | reported, 16.523 [16.272, 16.613] | reported, 1.535 [1.176, 2.007] | reported, 1.153 (heap 1.154) | reported, 1.155 (heap 1.155) |
| 2 (n 16, rho 4) | reported, 0.261 [0.259, 0.267] | reported, 20.478 [20.366, 20.595] | reported, 19.560 [19.445, 19.668] | reported, 1.349 [1.107, 1.798] | reported, 1.160 (heap 1.165) | reported, 1.156 (heap 1.158) |
| 3 (n 16, rho 16) | reported, 0.683 [0.676, 0.717] | reported, 16.716 [16.666, 16.784] | reported, 16.523 [16.474, 16.590] | reported, 1.336 [1.096, 1.645] | reported, 1.103 (heap 1.117) | reported, 1.147 (heap 1.149) |
| 4 (n 16, rho 64) | reported, 0.963 [0.947, 1.011] | reported, 14.081 [14.014, 14.205] | reported, 13.981 [13.917, 14.104] | reported, 1.473 [1.026, 1.608] | reported, 1.051 (heap 1.068) | reported, 1.150 (heap 1.150) |
| 5 (n 32, rho 1) | reported, 0.034 [0.034, 0.034] | reported, 11.384 [11.313, 11.422] | reported, 9.548 [9.494, 9.583] | reported, 1.919 [1.568, 2.045] | reported, 1.106 (heap 1.107) | reported, 1.133 (heap 1.133) |
| 6 (n 32, rho 4) | reported, 0.124 [0.122, 0.125] | reported, 10.995 [10.915, 11.077] | reported, 10.709 [10.628, 10.788] | reported, 1.592 [1.293, 1.720] | reported, 1.115 (heap 1.116) | reported, 1.151 (heap 1.151) |
| 7 (n 32, rho 16) | reported, 0.479 [0.478, 0.494] | reported, 7.374 [7.364, 7.386] | reported, 7.340 [7.330, 7.351] | reported, 1.677 [1.410, 1.875] | reported, 1.099 (heap 1.103) | reported, 1.155 (heap 1.155) |
| 8 (n 32, rho 64) | reported, 0.871 [0.864, 0.883] | reported, 14.074 [14.042, 14.236] | reported, 14.045 [14.012, 14.205] | reported, 1.995 [1.542, 2.179] | reported, 1.053 (heap 1.061) | reported, 1.157 (heap 1.157) |
| 9 (n 64, rho 1) | reported, 0.021 [0.020, 0.022] | reported, 4.897 [4.868, 4.930] | reported, 4.497 [4.473, 4.527] | reported, 3.419 [2.885, 3.743] | reported, 1.071 (heap 1.072) | reported, 1.116 (heap 1.116) |
| 10 (n 64, rho 4) | reported, 0.078 [0.075, 0.080] | reported, 5.259 [5.242, 5.291] | reported, 5.190 [5.173, 5.221] | reported, 3.423 [2.916, 3.602] | reported, 1.077 (heap 1.078) | reported, 1.140 (heap 1.141) |
| 11 (n 64, rho 16) | reported, 0.337 [0.316, 0.343] | reported, 4.284 [4.260, 4.344] | reported, 4.275 [4.251, 4.335] | reported, 2.912 [2.469, 3.468] | reported, 1.057 (heap 1.058) | reported, 1.147 (heap 1.147) |
| 12 (n 64, rho 64) | reported, 0.865 [0.857, 0.887] | reported, 4.369 [4.347, 4.415] | reported, 4.367 [4.344, 4.412] | reported, 3.266 [2.683, 3.874] | reported, 1.043 (heap 1.045) | reported, 1.156 (heap 1.156) |

### Gcd work and the call-for-call checks

Hash-V-pack: G(row)/G(TC) MUST be exactly 1, and its per-operation records equal TC's, every step (a check, carried over). The factored rows (rule-m3 item 7, v0.3 §16.9 item 3): per operation the same case and the same exact divisions, multiplications and additions as TC (checked); their gcds are obtained otherwise, so G(row)/G(TC) is reported, not checked, per part (entry; the operations' additions and multiplications), beside their factor work; and each factored row takes Hash-V-pack's arithmetic counters (but the operations' gcds) and store splits (v0.3 §16.9 item 4).

| job | G per operation (TC) | Hash-V-pack: G/G(TC), call for call | Pack-primes: G/G(TC) (entry, add, mul), call for call | Pack-primes vs Hash-V-pack | Pack-coprime: G/G(TC) (entry, add, mul), call for call | Pack-coprime vs Hash-V-pack |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 34.35 | 1.000000 (passed), pass | 0.0256 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2243 (1.0000, 0.3038, 0.0000), pass (case and core arithmetic) | pass |
| 2 | 34.05 | 1.000000 (passed), pass | 0.0255 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2374 (1.0000, 0.3258, 0.0000), pass (case and core arithmetic) | pass |
| 3 | 30.12 | 1.000000 (passed), pass | 0.0298 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2280 (1.0000, 0.3150, 0.0000), pass (case and core arithmetic) | pass |
| 4 | 31.76 | 1.000000 (passed), pass | 0.0281 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3388 (1.0000, 0.4669, 0.0000), pass (case and core arithmetic) | pass |
| 5 | 35.64 | 1.000000 (passed), pass | 0.0123 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1980 (1.0000, 0.2722, 0.0000), pass (case and core arithmetic) | pass |
| 6 | 35.31 | 1.000000 (passed), pass | 0.0124 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1918 (1.0000, 0.2626, 0.0000), pass (case and core arithmetic) | pass |
| 7 | 35.52 | 1.000000 (passed), pass | 0.0122 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2242 (1.0000, 0.3146, 0.0000), pass (case and core arithmetic) | pass |
| 8 | 35.46 | 1.000000 (passed), pass | 0.0118 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2330 (1.0000, 0.3159, 0.0000), pass (case and core arithmetic) | pass |
| 9 | 37.33 | 1.000000 (passed), pass | 0.0059 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1894 (1.0000, 0.2617, 0.0000), pass (case and core arithmetic) | pass |
| 10 | 36.99 | 1.000000 (passed), pass | 0.0059 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1890 (1.0000, 0.2613, 0.0000), pass (case and core arithmetic) | pass |
| 11 | 37.11 | 1.000000 (passed), pass | 0.0058 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1864 (1.0000, 0.2571, 0.0000), pass (case and core arithmetic) | pass |
| 12 | 35.88 | 1.000000 (passed), pass | 0.0061 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1829 (1.0000, 0.2495, 0.0000), pass (case and core arithmetic) | pass |

Factor work of the factored rows beside the gcd work (counting runs, the job's factorings and steps summed): merges and entries compared, divisibility tests, per-piece gcds, trial-division candidates, refinement gcds (pass 1 and the reconciliation).

| job | row | G(row)/G(TC) | merges | entries compared | tests | per-piece gcds | trial-division candidates | pass-1 gcds | pass-2 tests | reconciliation gcds | splits (pass 1 / reconciliation / leftover) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | 0.0256 | 10626 | 21396 | 2317 | 0 | 27362929 | 0 | 0 | 0 | 0 / 0 / 0 |
| 1 | Pack-coprime | 0.2243 | 10626 | 21396 | 2588 | 1551 | 0 | 83923 | 90547 | 1098 | 81 / 0 / 0 |
| 2 | Pack-primes | 0.0255 | 10843 | 21428 | 2540 | 0 | 24924978 | 0 | 0 | 0 | 0 / 0 / 0 |
| 2 | Pack-coprime | 0.2374 | 10843 | 21428 | 2828 | 1668 | 0 | 46580 | 51582 | 387 | 74 / 0 / 0 |
| 3 | Pack-primes | 0.0298 | 10003 | 17978 | 1992 | 0 | 19084132 | 0 | 0 | 0 | 0 / 0 / 0 |
| 3 | Pack-coprime | 0.2280 | 10003 | 17978 | 2274 | 1407 | 0 | 42822 | 46935 | 123 | 63 / 0 / 0 |
| 4 | Pack-primes | 0.0281 | 9810 | 21612 | 3403 | 0 | 15469510 | 0 | 0 | 0 | 0 / 0 / 0 |
| 4 | Pack-coprime | 0.3388 | 9810 | 21612 | 3924 | 2381 | 0 | 41241 | 44931 | 45 | 50 / 0 / 0 |
| 5 | Pack-primes | 0.0123 | 85882 | 171841 | 17511 | 0 | 183615252 | 0 | 0 | 0 | 0 / 0 / 0 |
| 5 | Pack-coprime | 0.1980 | 85882 | 171841 | 19129 | 11533 | 0 | 931121 | 973249 | 2760 | 179 / 0 / 0 |
| 6 | Pack-primes | 0.0124 | 85949 | 170307 | 17088 | 0 | 177313092 | 0 | 0 | 0 | 0 / 0 / 0 |
| 6 | Pack-coprime | 0.1918 | 85949 | 170307 | 18671 | 11186 | 0 | 665816 | 710422 | 1077 | 192 / 0 / 0 |
| 7 | Pack-primes | 0.0122 | 88181 | 183353 | 20267 | 0 | 115229133 | 0 | 0 | 0 | 0 / 0 / 0 |
| 7 | Pack-coprime | 0.2242 | 88181 | 183353 | 22266 | 13529 | 0 | 616805 | 655992 | 363 | 167 / 0 / 0 |
| 8 | Pack-primes | 0.0118 | 85437 | 178639 | 21962 | 0 | 226606969 | 0 | 0 | 0 | 0 / 0 / 0 |
| 8 | Pack-coprime | 0.2330 | 85437 | 178639 | 23594 | 13480 | 0 | 618324 | 657550 | 120 | 161 / 0 / 0 |
| 9 | Pack-primes | 0.0059 | 694204 | 1404018 | 138100 | 0 | 1129105525 | 0 | 0 | 0 | 0 / 0 / 0 |
| 9 | Pack-coprime | 0.1894 | 694204 | 1404018 | 149826 | 91120 | 0 | 11472093 | 11981629 | 6681 | 532 / 0 / 0 |
| 10 | Pack-primes | 0.0059 | 687584 | 1394507 | 135494 | 0 | 1214626243 | 0 | 0 | 0 | 0 / 0 / 0 |
| 10 | Pack-coprime | 0.1890 | 687584 | 1394507 | 147274 | 89888 | 0 | 9706122 | 10228166 | 2742 | 535 / 0 / 0 |
| 11 | Pack-primes | 0.0058 | 695039 | 1406978 | 134560 | 0 | 982287308 | 0 | 0 | 0 | 0 / 0 / 0 |
| 11 | Pack-coprime | 0.1864 | 695039 | 1406978 | 145840 | 89740 | 0 | 9471253 | 9972749 | 1065 | 514 / 0 / 0 |
| 12 | Pack-primes | 0.0061 | 671131 | 1333633 | 129893 | 0 | 999766746 | 0 | 0 | 0 | 0 / 0 / 0 |
| 12 | Pack-coprime | 0.1829 | 671131 | 1333633 | 140287 | 85318 | 0 | 9438433 | 9946513 | 363 | 517 / 0 / 0 |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m3 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.502 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.888 | 0 |
| NONE | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.744 | 0 |
| NONE | Hash-V-pack vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.590 | 0 |
| NONE | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.206 | 0 |
| NONE | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.008 | 0 |
| NONE | Pack-primes vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.117 | 2 |
| NONE | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 2.563 | 3 |
| NONE | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.007 | 0 |
| NONE | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.086 | 0 |
| NONE | Pack-coprime vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.342 | 3 |
| NONE | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 2.904 | 3 |
| NONE | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.406 | 0 |
| NONE | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.009 | 0 |
| NONE | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.847 | 3 |
| NONE | Pack-primes vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.612 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.014 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.097 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 3.149 | 3 |
| NONE | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.826 | 0 |
| NONE | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.034 | 0 |
| NONE | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 11.384 | 0 |
| NONE | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.106 | 0 |
| NONE | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.133 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.584 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.887 | 0 |
| LOW | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.373 | 0 |
| LOW | Hash-V-pack vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.336 | 0 |
| LOW | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.366 | 0 |
| LOW | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.008 | 0 |
| LOW | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.755 | 0 |
| LOW | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.666 | 0 |
| LOW | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.050 | 0 |
| LOW | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.086 | 0 |
| LOW | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.847 | 0 |
| LOW | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.918 | 1 |
| LOW | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.623 | 0 |
| LOW | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.009 | 0 |
| LOW | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.961 | 1 |
| LOW | Pack-primes vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.230 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.089 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.097 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.186 | 3 |
| LOW | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.416 | 0 |
| LOW | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.124 | 0 |
| LOW | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 10.995 | 0 |
| LOW | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.115 | 0 |
| LOW | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.151 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.645 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.887 | 0 |
| MID | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.174 | 0 |
| MID | Hash-V-pack vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.238 | 0 |
| MID | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.496 | 0 |
| MID | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.012 | 0 |
| MID | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.304 | 0 |
| MID | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.359 | 0 |
| MID | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.278 | 0 |
| MID | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.089 | 0 |
| MID | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.335 | 0 |
| MID | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.558 | 0 |
| MID | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.769 | 0 |
| MID | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.013 | 0 |
| MID | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.687 | 0 |
| MID | Pack-primes vs Hash-V-pack | peak | 0 | 1 | 2 | 0 | 0 | 0 | 1.116 | 0 |
| MID | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.444 | 0 |
| MID | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.100 | 0 |
| MID | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.854 | 0 |
| MID | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.289 | 0 |
| MID | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.479 | 0 |
| MID | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 7.374 | 0 |
| MID | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.099 | 0 |
| MID | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.147 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.640 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.877 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.139 | 0 |
| HIGH | Hash-V-pack vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.186 | 0 |
| HIGH | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.590 | 0 |
| HIGH | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.009 | 0 |
| HIGH | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.173 | 0 |
| HIGH | Pack-primes vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.260 | 0 |
| HIGH | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.510 | 0 |
| HIGH | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.083 | 0 |
| HIGH | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.183 | 0 |
| HIGH | Pack-coprime vs TC | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.457 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | build | 0 | 1 | 2 | 0 | 0 | 0 | 0.891 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.011 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.251 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | peak | 0 | 2 | 1 | 0 | 0 | 0 | 1.062 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.805 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.095 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.318 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | peak | 0 | 0 | 3 | 0 | 0 | 0 | 1.229 | 0 |
| HIGH | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.871 | 0 |
| HIGH | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 14.074 | 0 |
| HIGH | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.051 | 0 |
| HIGH | Pack-coprime vs Pack-primes | peak | 0 | 0 | 0 | 0 | 0 | 3 | 1.156 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.502 | 0.584 | 0.658 | 0.616 |
| 32 | 0.499 | 0.562 | 0.627 | 0.640 |
| 64 | 0.508 | 0.587 | 0.645 | 0.662 |

Hash-V-pack against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.862 | 0.862 | 0.843 | 0.840 |
| 32 | 0.888 | 0.887 | 0.887 | 0.877 |
| 64 | 0.895 | 0.890 | 0.897 | 0.891 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.803 | 0.299 | 0.174 | 0.140 |
| 32 | 0.744 | 0.387 | 0.168 | 0.139 |
| 64 | 0.685 | 0.373 | 0.284 | 0.136 |

Hash-V-pack against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.654 | 1.319 | 1.238 | 1.201 |
| 32 | 1.590 | 1.354 | 1.208 | 1.186 |
| 64 | 1.544 | 1.336 | 1.277 | 1.178 |

Pack-primes against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.175 | 0.346 | 0.481 | 0.514 |
| 32 | 0.213 | 0.405 | 0.580 | 0.625 |
| 64 | 0.206 | 0.366 | 0.496 | 0.590 |

Pack-primes against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.006 | 0.006 | 0.008 | 0.009 |
| 32 | 0.008 | 0.008 | 0.012 | 0.006 |
| 64 | 0.010 | 0.009 | 0.012 | 0.011 |

Pack-primes against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.330 | 0.755 | 0.304 | 0.180 |
| 32 | 2.117 | 0.760 | 0.283 | 0.173 |
| 64 | 1.901 | 0.721 | 0.379 | 0.165 |

Pack-primes against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.852 | 1.806 | 1.517 | 1.352 |
| 32 | 2.563 | 1.666 | 1.348 | 1.260 |
| 64 | 2.373 | 1.587 | 1.359 | 1.218 |

Pack-coprime against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.012 | 0.090 | 0.328 | 0.495 |
| 32 | 0.007 | 0.050 | 0.278 | 0.545 |
| 64 | 0.004 | 0.029 | 0.167 | 0.510 |

Pack-coprime against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.134 | 0.133 | 0.126 | 0.128 |
| 32 | 0.086 | 0.086 | 0.089 | 0.083 |
| 64 | 0.050 | 0.049 | 0.050 | 0.049 |

Pack-coprime against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.688 | 0.875 | 0.335 | 0.189 |
| 32 | 2.342 | 0.847 | 0.311 | 0.183 |
| 64 | 2.037 | 0.777 | 0.400 | 0.172 |

Pack-coprime against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 3.293 | 2.088 | 1.740 | 1.555 |
| 32 | 2.904 | 1.918 | 1.557 | 1.457 |
| 64 | 2.649 | 1.809 | 1.558 | 1.409 |

Pack-primes against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.350 | 0.592 | 0.730 | 0.835 |
| 32 | 0.427 | 0.720 | 0.925 | 0.978 |
| 64 | 0.406 | 0.623 | 0.769 | 0.891 |

Pack-primes against Hash-V-pack, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.007 | 0.008 | 0.009 | 0.011 |
| 32 | 0.009 | 0.009 | 0.014 | 0.007 |
| 64 | 0.011 | 0.011 | 0.013 | 0.013 |

Pack-primes against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.903 | 2.526 | 1.746 | 1.288 |
| 32 | 2.847 | 1.961 | 1.687 | 1.251 |
| 64 | 2.777 | 1.931 | 1.332 | 1.216 |

Pack-primes against Hash-V-pack, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.724 | 1.369 | 1.225 | 1.126 |
| 32 | 1.612 | 1.230 | 1.116 | 1.062 |
| 64 | 1.538 | 1.188 | 1.064 | 1.034 |

Pack-coprime against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.024 | 0.155 | 0.499 | 0.805 |
| 32 | 0.014 | 0.089 | 0.444 | 0.852 |
| 64 | 0.008 | 0.049 | 0.259 | 0.771 |

Pack-coprime against Hash-V-pack, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.155 | 0.154 | 0.149 | 0.152 |
| 32 | 0.097 | 0.097 | 0.100 | 0.095 |
| 64 | 0.056 | 0.056 | 0.056 | 0.055 |

Pack-coprime against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 3.348 | 2.929 | 1.926 | 1.353 |
| 32 | 3.149 | 2.186 | 1.854 | 1.318 |
| 64 | 2.975 | 2.081 | 1.408 | 1.268 |

Pack-coprime against Hash-V-pack, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.990 | 1.583 | 1.406 | 1.294 |
| 32 | 1.826 | 1.416 | 1.289 | 1.229 |
| 64 | 1.716 | 1.354 | 1.220 | 1.196 |

Pack-coprime against Pack-primes, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.069 | 0.261 | 0.683 | 0.963 |
| 32 | 0.034 | 0.124 | 0.479 | 0.871 |
| 64 | 0.021 | 0.078 | 0.337 | 0.865 |

Pack-coprime against Pack-primes, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 22.313 | 20.478 | 16.716 | 14.081 |
| 32 | 11.384 | 10.995 | 7.374 | 14.074 |
| 64 | 4.897 | 5.259 | 4.284 | 4.369 |

Pack-coprime against Pack-primes, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.153 | 1.160 | 1.103 | 1.051 |
| 32 | 1.106 | 1.115 | 1.099 | 1.053 |
| 64 | 1.071 | 1.077 | 1.057 | 1.043 |

Pack-coprime against Pack-primes, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.155 | 1.156 | 1.147 | 1.150 |
| 32 | 1.133 | 1.151 | 1.155 | 1.157 |
| 64 | 1.116 | 1.140 | 1.147 | 1.156 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (3 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build (its factoring included), over that input's cells. (f), (f'), (q) and (r) are accounted (v0.3 §16.10).

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 1 | Hash-V-pack | 12.84 (13.34) | 13.14 (13.64) | 26.47 (26.99) | 12.84 (52.84) | 12.84 (52.84) |
| 1 | Pack-primes | 37.29 (38.00) | 40.71 (41.42) | 45.63 (46.35) | 36.65 (52.84) | 37.92 (57.08) |
| 1 | Pack-coprime | 43.00 (43.84) | 46.08 (46.91) | 52.68 (53.53) | 42.56 (52.84) | 43.45 (62.61) |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 2 | Hash-V-pack | 4.78 (5.31) | 7.77 (8.30) | 21.10 (21.66) | 4.69 (46.69) | 4.88 (46.88) |
| 2 | Pack-primes | 12.08 (12.84) | 23.63 (24.38) | 28.89 (29.65) | 11.64 (46.69) | 12.52 (46.88) |
| 2 | Pack-coprime | 14.00 (14.97) | 26.47 (27.41) | 33.40 (34.34) | 13.59 (46.69) | 14.41 (46.88) |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 3 | Hash-V-pack | 2.78 (3.31) | 6.47 (6.99) | 19.80 (20.34) | 2.75 (44.75) | 2.81 (44.81) |
| 3 | Pack-primes | 4.86 (5.59) | 19.35 (20.08) | 24.26 (25.00) | 4.86 (44.75) | 4.85 (44.81) |
| 3 | Pack-coprime | 5.36 (6.25) | 21.40 (22.26) | 27.84 (28.72) | 5.28 (44.75) | 5.44 (44.81) |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) |
| 4 | Hash-V-pack | 2.23 (2.75) | 5.89 (6.41) | 19.22 (19.76) | 2.22 (44.22) | 2.25 (44.25) |
| 4 | Pack-primes | 2.88 (3.66) | 16.52 (17.29) | 21.63 (22.40) | 2.77 (44.22) | 2.99 (44.25) |
| 4 | Pack-coprime | 3.02 (3.91) | 18.28 (19.16) | 24.88 (25.76) | 2.88 (44.22) | 3.17 (44.25) |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 5 | Hash-V-pack | 11.90 (12.03) | 12.11 (12.25) | 25.45 (25.59) | 11.86 (51.86) | 11.94 (51.94) |
| 5 | Pack-primes | 33.88 (34.06) | 36.88 (37.06) | 41.01 (41.20) | 33.75 (51.86) | 34.01 (51.94) |
| 5 | Pack-coprime | 37.47 (37.70) | 41.07 (41.29) | 46.47 (46.69) | 37.31 (54.72) | 37.63 (55.25) |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 6 | Hash-V-pack | 6.20 (6.33) | 8.34 (8.47) | 21.67 (21.80) | 6.21 (46.21) | 6.19 (46.19) |
| 6 | Pack-primes | 12.15 (12.34) | 22.37 (22.56) | 26.66 (26.84) | 12.22 (46.21) | 12.08 (46.19) |
| 6 | Pack-coprime | 13.55 (13.77) | 25.14 (25.36) | 30.69 (30.91) | 13.65 (46.21) | 13.45 (46.19) |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 7 | Hash-V-pack | 2.68 (2.81) | 5.99 (6.12) | 19.33 (19.46) | 2.70 (44.70) | 2.67 (44.67) |
| 7 | Pack-primes | 4.53 (4.71) | 17.16 (17.34) | 21.57 (21.76) | 4.58 (44.70) | 4.47 (44.67) |
| 7 | Pack-coprime | 4.97 (5.20) | 19.29 (19.51) | 24.91 (25.13) | 5.02 (44.70) | 4.93 (44.67) |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) |
| 8 | Hash-V-pack | 2.22 (2.34) | 5.64 (5.77) | 18.97 (19.11) | 2.23 (44.23) | 2.21 (44.21) |
| 8 | Pack-primes | 2.78 (2.95) | 15.77 (15.95) | 20.15 (20.33) | 2.82 (44.23) | 2.73 (44.21) |
| 8 | Pack-coprime | 2.92 (3.13) | 17.72 (17.93) | 23.31 (23.52) | 2.97 (44.23) | 2.88 (44.21) |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 9 | Hash-V-pack | 10.96 (10.99) | 11.36 (11.40) | 24.70 (24.73) | 10.90 (50.90) | 11.01 (51.01) |
| 9 | Pack-primes | 30.42 (30.46) | 33.85 (33.89) | 37.97 (38.02) | 30.26 (50.90) | 30.58 (51.01) |
| 9 | Pack-coprime | 32.59 (32.64) | 37.43 (37.48) | 42.39 (42.44) | 32.42 (50.90) | 32.77 (51.01) |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 10 | Hash-V-pack | 5.97 (6.00) | 8.04 (8.07) | 21.38 (21.41) | 5.98 (45.98) | 5.96 (45.96) |
| 10 | Pack-primes | 11.53 (11.58) | 21.35 (21.39) | 25.39 (25.43) | 11.56 (45.98) | 11.51 (45.96) |
| 10 | Pack-coprime | 12.42 (12.48) | 24.04 (24.10) | 28.95 (29.00) | 12.47 (45.98) | 12.38 (45.96) |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 11 | Hash-V-pack | 4.55 (4.58) | 7.09 (7.13) | 20.43 (20.46) | 4.55 (44.55) | 4.55 (44.55) |
| 11 | Pack-primes | 6.06 (6.10) | 17.68 (17.73) | 21.74 (21.78) | 6.06 (44.55) | 6.06 (44.55) |
| 11 | Pack-coprime | 6.41 (6.46) | 20.07 (20.12) | 24.93 (24.98) | 6.41 (44.55) | 6.40 (44.55) |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) |
| 12 | Hash-V-pack | 2.17 (2.20) | 5.51 (5.55) | 18.85 (18.88) | 2.18 (44.18) | 2.16 (44.16) |
| 12 | Pack-primes | 2.64 (2.69) | 15.34 (15.38) | 19.49 (19.54) | 2.67 (44.18) | 2.60 (44.16) |
| 12 | Pack-coprime | 2.75 (2.81) | 17.58 (17.63) | 22.54 (22.59) | 2.79 (44.18) | 2.72 (44.16) |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B |
| --- | --- | --- | --- |
| 1 | TC | 6936 | 6917 |
| 1 | Hash-V-pack | 13937 | 13679 |
| 1 | Pack-primes | 40209 | 37386 |
| 1 | Pack-coprime | 588697 | 539746 |
| 1 | Hash-V-pack-null | 13878 | 13668 |
| 2 | TC | 7054 | 6566 |
| 2 | Hash-V-pack | 11756 | 11723 |
| 2 | Pack-primes | 20905 | 19054 |
| 2 | Pack-coprime | 74895 | 76569 |
| 2 | Hash-V-pack-null | 11885 | 11824 |
| 3 | TC | 6057 | 5929 |
| 3 | Hash-V-pack | 9112 | 9128 |
| 3 | Pack-primes | 13816 | 12087 |
| 3 | Pack-coprime | 17254 | 18589 |
| 3 | Hash-V-pack-null | 9047 | 9135 |
| 4 | TC | 5298 | 5131 |
| 4 | Hash-V-pack | 8386 | 8709 |
| 4 | Pack-primes | 11351 | 10086 |
| 4 | Pack-coprime | 10598 | 10562 |
| 4 | Hash-V-pack-null | 8413 | 8690 |
| 5 | TC | 29342 | 29276 |
| 5 | Hash-V-pack | 59038 | 58568 |
| 5 | Pack-primes | 139922 | 134372 |
| 5 | Pack-coprime | 3987593 | 4141234 |
| 5 | Hash-V-pack-null | 58822 | 58569 |
| 6 | TC | 28704 | 27881 |
| 6 | Hash-V-pack | 51160 | 50008 |
| 6 | Pack-primes | 74122 | 68120 |
| 6 | Pack-coprime | 577612 | 563499 |
| 6 | Hash-V-pack-null | 51101 | 49896 |
| 7 | TC | 28017 | 28533 |
| 7 | Hash-V-pack | 45458 | 45052 |
| 7 | Pack-primes | 52146 | 48752 |
| 7 | Pack-coprime | 102173 | 102233 |
| 7 | Hash-V-pack-null | 45160 | 45315 |
| 8 | TC | 24076 | 26346 |
| 8 | Hash-V-pack | 39074 | 39745 |
| 8 | Pack-primes | 42128 | 40194 |
| 8 | Pack-coprime | 46687 | 46364 |
| 8 | Hash-V-pack-null | 39179 | 39782 |
| 9 | TC | 118008 | 116518 |
| 9 | Hash-V-pack | 233240 | 229251 |
| 9 | Pack-primes | 585422 | 536018 |
| 9 | Pack-coprime | 26315546 | 27857142 |
| 9 | Hash-V-pack-null | 232783 | 229585 |
| 10 | TC | 118892 | 117080 |
| 10 | Hash-V-pack | 201580 | 200130 |
| 10 | Pack-primes | 340700 | 296026 |
| 10 | Pack-coprime | 4295230 | 3913580 |
| 10 | Hash-V-pack-null | 200595 | 202072 |
| 11 | TC | 117065 | 115636 |
| 11 | Hash-V-pack | 181954 | 180059 |
| 11 | Pack-primes | 250514 | 199592 |
| 11 | Pack-coprime | 704287 | 677623 |
| 11 | Hash-V-pack-null | 182154 | 181458 |
| 12 | TC | 113934 | 115130 |
| 12 | Hash-V-pack | 167926 | 173930 |
| 12 | Pack-primes | 229058 | 171867 |
| 12 | Pack-coprime | 226770 | 222086 |
| 12 | Hash-V-pack-null | 173650 | 173224 |

## Stores, per job and matrix (rule-m3 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. T is the build table's. The factored rows' stores, lists and packings are Hash-V-pack's (checked).

| job | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | cells bytes | list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.08 | 5 | 0 (0) | 1024 | 2264 |
| 1 | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.05 | 3 | 0 (0) | 1024 | 2264 |
| 1 | C | 311 | 1024 | 30.4% | 64 | 16 | 201 / 311 | 1.14 | 6 | 1 (1024) | 1024 | 2488 |
| 2 | A | 86 | 1024 | 8.4% | 64 | 8 | 426 / 86 | 1.01 | 2 | 0 (0) | 512 | 688 |
| 2 | B | 92 | 1024 | 9.0% | 64 | 8 | 420 / 92 | 1.01 | 2 | 0 (0) | 512 | 736 |
| 2 | C | 312 | 1024 | 30.5% | 64 | 16 | 200 / 312 | 1.15 | 7 | 1 (1024) | 1024 | 2496 |
| 3 | A | 24 | 1024 | 2.3% | 64 | 8 | 488 / 24 | 1.00 | 1 | 0 (0) | 512 | 192 |
| 3 | B | 26 | 1024 | 2.5% | 64 | 8 | 486 / 26 | 1.00 | 1 | 0 (0) | 512 | 208 |
| 3 | C | 315 | 1024 | 30.8% | 64 | 16 | 197 / 315 | 1.13 | 6 | 1 (1024) | 1024 | 2520 |
| 4 | A | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 512 | 56 |
| 4 | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | C | 294 | 1024 | 28.7% | 64 | 16 | 218 / 294 | 1.11 | 4 | 1 (1024) | 1024 | 2352 |
| 5 | A | 1006 | 4096 | 24.6% | 64 | 16 | 1042 / 1006 | 1.06 | 8 | 0 (0) | 4096 | 8048 |
| 5 | B | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 4096 | 8128 |
| 5 | C | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.11 | 6 | 1 (4096) | 4096 | 8752 |
| 6 | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.03 | 4 | 0 (0) | 4096 | 2264 |
| 6 | B | 280 | 4096 | 6.8% | 64 | 16 | 1768 / 280 | 1.02 | 2 | 0 (0) | 4096 | 2240 |
| 6 | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.09 | 5 | 1 (4096) | 4096 | 8816 |
| 7 | A | 89 | 4096 | 2.2% | 64 | 8 | 1959 / 89 | 1.01 | 2 | 0 (0) | 2048 | 712 |
| 7 | B | 86 | 4096 | 2.1% | 64 | 8 | 1962 / 86 | 1.00 | 1 | 0 (0) | 2048 | 688 |
| 7 | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.10 | 6 | 1 (4096) | 4096 | 8816 |
| 8 | A | 29 | 4096 | 0.7% | 64 | 8 | 2019 / 29 | 1.00 | 1 | 0 (0) | 2048 | 232 |
| 8 | B | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 2048 | 216 |
| 8 | C | 1086 | 4096 | 26.5% | 64 | 16 | 962 / 1086 | 1.08 | 5 | 1 (4096) | 4096 | 8688 |
| 9 | A | 3533 | 16384 | 21.6% | 64 | 16 | 4659 / 3533 | 1.03 | 4 | 0 (0) | 16384 | 28264 |
| 9 | B | 3589 | 16384 | 21.9% | 64 | 16 | 4603 / 3589 | 1.02 | 4 | 0 (0) | 16384 | 28712 |
| 9 | C | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 6 | 1 (16384) | 16384 | 33504 |
| 10 | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 16384 | 8104 |
| 10 | B | 1006 | 16384 | 6.1% | 64 | 16 | 7186 / 1006 | 1.01 | 2 | 0 (0) | 16384 | 8048 |
| 10 | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 16384 | 33512 |
| 11 | A | 280 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 1.00 | 2 | 0 (0) | 16384 | 2240 |
| 11 | B | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 2 | 0 (0) | 16384 | 2264 |
| 11 | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 16384 | 33512 |
| 12 | A | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 8192 | 720 |
| 12 | B | 84 | 16384 | 0.5% | 64 | 8 | 8108 / 84 | 1.00 | 1 | 0 (0) | 8192 | 672 |
| 12 | C | 4198 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.09 | 6 | 1 (16384) | 16384 | 33584 |

### Hash-V-pack's packings (from the counting runs)

| job | matrix | W | w_h build | w_h packed | list bytes | cells rewritten | map bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 283 | 16 | 16 | 2264 | 512 | 2048 |
| 1 | B | 283 | 16 | 16 | 2264 | 512 | 2048 |
| 1 | C | 311 | 16 | 16 | 2488 | 512 | 2048 |
| 2 | A | 86 | 16 | 8 | 688 | 512 | 2048 |
| 2 | B | 92 | 16 | 8 | 736 | 512 | 2048 |
| 2 | C | 312 | 16 | 16 | 2496 | 512 | 2048 |
| 3 | A | 24 | 16 | 8 | 192 | 512 | 2048 |
| 3 | B | 26 | 16 | 8 | 208 | 512 | 2048 |
| 3 | C | 315 | 16 | 16 | 2520 | 512 | 2048 |
| 4 | A | 7 | 16 | 8 | 56 | 512 | 2048 |
| 4 | B | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | C | 294 | 16 | 16 | 2352 | 512 | 2048 |
| 5 | A | 1006 | 16 | 16 | 8048 | 2048 | 8192 |
| 5 | B | 1016 | 16 | 16 | 8128 | 2048 | 8192 |
| 5 | C | 1094 | 16 | 16 | 8752 | 2048 | 8192 |
| 6 | A | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | B | 280 | 16 | 16 | 2240 | 2048 | 8192 |
| 6 | C | 1102 | 16 | 16 | 8816 | 2048 | 8192 |
| 7 | A | 89 | 16 | 8 | 712 | 2048 | 8192 |
| 7 | B | 86 | 16 | 8 | 688 | 2048 | 8192 |
| 7 | C | 1102 | 16 | 16 | 8816 | 2048 | 8192 |
| 8 | A | 29 | 16 | 8 | 232 | 2048 | 8192 |
| 8 | B | 27 | 16 | 8 | 216 | 2048 | 8192 |
| 8 | C | 1086 | 16 | 16 | 8688 | 2048 | 8192 |
| 9 | A | 3533 | 16 | 16 | 28264 | 8192 | 32768 |
| 9 | B | 3589 | 16 | 16 | 28712 | 8192 | 32768 |
| 9 | C | 4188 | 16 | 16 | 33504 | 8192 | 32768 |
| 10 | A | 1013 | 16 | 16 | 8104 | 8192 | 32768 |
| 10 | B | 1006 | 16 | 16 | 8048 | 8192 | 32768 |
| 10 | C | 4189 | 16 | 16 | 33512 | 8192 | 32768 |
| 11 | A | 280 | 16 | 16 | 2240 | 8192 | 32768 |
| 11 | B | 283 | 16 | 16 | 2264 | 8192 | 32768 |
| 11 | C | 4189 | 16 | 16 | 33512 | 8192 | 32768 |
| 12 | A | 90 | 16 | 8 | 720 | 8192 | 32768 |
| 12 | B | 84 | 16 | 8 | 672 | 8192 | 32768 |
| 12 | C | 4198 | 16 | 16 | 33584 | 8192 | 32768 |

### The factored rows' factorings (from the counting runs, each at its factoring)

Pack-primes: trial-division candidates, divisions with remainder and exact divisions. Pack-coprime: pass 1's scan gcds, divisions and splits (with the splits' gcds), pass 2's divisibility tests. E: (f)'s entries; |q|: the base's pieces.

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes | candidates | divisions | exact | pass-1 gcds | pass-1 divisions | splits (gcds) | pass-2 tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 283 | 551 | 6095 | 0 | 0 | 6032 | 6438 | 406 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | B | 283 | 587 | 6419 | 0 | 0 | 5760 | 6219 | 459 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | C | 311 | 824 | 8664 | 0 | 0 | 27351137 | 27352080 | 943 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-coprime | A | 283 | 551 | 6095 | 189 | 1512 | 0 | 0 | 0 | 21808 | 845 | 14 (14) | 22632 |
| 1 | Pack-coprime | B | 283 | 587 | 6419 | 177 | 1416 | 0 | 0 | 0 | 20070 | 953 | 13 (12) | 20806 |
| 1 | Pack-coprime | C | 311 | 680 | 7368 | 311 | 2488 | 0 | 0 | 0 | 42045 | 1124 | 54 (59) | 47109 |
| 2 | Pack-primes | A | 86 | 159 | 1779 | 0 | 0 | 1603 | 1738 | 135 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | B | 92 | 176 | 1956 | 0 | 0 | 1457 | 1617 | 160 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | C | 312 | 799 | 8443 | 0 | 0 | 24921918 | 24922828 | 910 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-coprime | A | 86 | 156 | 1752 | 66 | 528 | 0 | 0 | 0 | 2199 | 231 | 9 (10) | 2518 |
| 2 | Pack-coprime | B | 92 | 174 | 1938 | 63 | 504 | 0 | 0 | 0 | 2322 | 286 | 11 (10) | 2614 |
| 2 | Pack-coprime | C | 312 | 656 | 7156 | 310 | 2480 | 0 | 0 | 0 | 42059 | 1072 | 54 (56) | 46450 |
| 3 | Pack-primes | A | 24 | 49 | 541 | 0 | 0 | 224 | 265 | 41 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | B | 26 | 46 | 522 | 0 | 0 | 425 | 469 | 44 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-primes | C | 315 | 841 | 8833 | 0 | 0 | 19083483 | 19084384 | 901 | 0 | 0 | 0 (0) | 0 |
| 3 | Pack-coprime | A | 24 | 43 | 487 | 20 | 160 | 0 | 0 | 0 | 194 | 56 | 4 (5) | 265 |
| 3 | Pack-coprime | B | 26 | 44 | 504 | 21 | 168 | 0 | 0 | 0 | 261 | 61 | 4 (4) | 332 |
| 3 | Pack-coprime | C | 315 | 711 | 7663 | 311 | 2488 | 0 | 0 | 0 | 42367 | 1124 | 55 (56) | 46338 |
| 4 | Pack-primes | A | 7 | 12 | 140 | 0 | 0 | 122 | 130 | 8 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | B | 8 | 17 | 189 | 0 | 0 | 50 | 61 | 11 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | C | 294 | 740 | 7840 | 0 | 0 | 15469338 | 15469970 | 632 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-coprime | A | 7 | 9 | 113 | 7 | 56 | 0 | 0 | 0 | 18 | 5 | 2 (2) | 37 |
| 4 | Pack-coprime | B | 8 | 15 | 171 | 8 | 64 | 0 | 0 | 0 | 33 | 15 | 3 (2) | 51 |
| 4 | Pack-coprime | C | 294 | 613 | 6697 | 302 | 2416 | 0 | 0 | 0 | 41190 | 768 | 45 (47) | 44843 |
| 5 | Pack-primes | A | 1006 | 2043 | 22415 | 0 | 0 | 21282 | 22782 | 1500 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | B | 1016 | 2059 | 22599 | 0 | 0 | 21276 | 22806 | 1530 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | C | 1094 | 2965 | 31065 | 0 | 0 | 183572694 | 183575127 | 2433 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-coprime | A | 1006 | 2043 | 22415 | 456 | 3648 | 0 | 0 | 0 | 153205 | 3592 | 14 (14) | 155258 |
| 5 | Pack-coprime | B | 1016 | 2059 | 22599 | 464 | 3712 | 0 | 0 | 0 | 160267 | 3645 | 14 (12) | 162488 |
| 5 | Pack-coprime | C | 1094 | 2532 | 27168 | 1175 | 9400 | 0 | 0 | 0 | 617649 | 3228 | 151 (151) | 655503 |
| 6 | Pack-primes | A | 283 | 558 | 6158 | 0 | 0 | 5642 | 6091 | 449 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | B | 280 | 546 | 6038 | 0 | 0 | 5450 | 5887 | 437 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | C | 1102 | 2945 | 30917 | 0 | 0 | 177302000 | 177304415 | 2415 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-coprime | A | 283 | 558 | 6158 | 183 | 1464 | 0 | 0 | 0 | 20513 | 898 | 13 (14) | 21309 |
| 6 | Pack-coprime | B | 280 | 545 | 6029 | 176 | 1408 | 0 | 0 | 0 | 20050 | 879 | 11 (12) | 20770 |
| 6 | Pack-coprime | C | 1102 | 2515 | 27047 | 1190 | 9520 | 0 | 0 | 0 | 625253 | 3164 | 168 (170) | 668343 |
| 7 | Pack-primes | A | 89 | 175 | 1935 | 0 | 0 | 1337 | 1489 | 152 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | B | 86 | 166 | 1842 | 0 | 0 | 1443 | 1571 | 128 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-primes | C | 1102 | 2901 | 30521 | 0 | 0 | 115226353 | 115228741 | 2388 | 0 | 0 | 0 (0) | 0 |
| 7 | Pack-coprime | A | 89 | 171 | 1899 | 60 | 480 | 0 | 0 | 0 | 2073 | 271 | 12 (10) | 2365 |
| 7 | Pack-coprime | B | 86 | 164 | 1824 | 61 | 488 | 0 | 0 | 0 | 2138 | 246 | 11 (10) | 2415 |
| 7 | Pack-coprime | C | 1102 | 2490 | 26822 | 1167 | 9336 | 0 | 0 | 0 | 612594 | 3181 | 144 (146) | 651212 |
| 8 | Pack-primes | A | 29 | 54 | 606 | 0 | 0 | 249 | 303 | 54 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | B | 27 | 47 | 535 | 0 | 0 | 455 | 497 | 42 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | C | 1086 | 2849 | 29989 | 0 | 0 | 226606265 | 226608489 | 2224 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-coprime | A | 29 | 54 | 606 | 19 | 152 | 0 | 0 | 0 | 232 | 91 | 4 (4) | 310 |
| 8 | Pack-coprime | B | 27 | 45 | 517 | 21 | 168 | 0 | 0 | 0 | 233 | 62 | 4 (4) | 317 |
| 8 | Pack-coprime | C | 1086 | 2436 | 26272 | 1175 | 9400 | 0 | 0 | 0 | 617859 | 2930 | 153 (155) | 656923 |
| 9 | Pack-primes | A | 3533 | 7241 | 79305 | 0 | 0 | 78280 | 83518 | 5238 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | B | 3589 | 7309 | 80141 | 0 | 0 | 80279 | 85502 | 5223 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | C | 4188 | 11127 | 116899 | 0 | 0 | 1128946966 | 1128954783 | 7817 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-coprime | A | 3533 | 7241 | 79305 | 1104 | 8832 | 0 | 0 | 0 | 1002914 | 13702 | 15 (16) | 1009237 |
| 9 | Pack-coprime | B | 3589 | 7309 | 80141 | 1123 | 8984 | 0 | 0 | 0 | 1058193 | 13775 | 17 (18) | 1064684 |
| 9 | Pack-coprime | C | 4188 | 9985 | 106621 | 4556 | 36448 | 0 | 0 | 0 | 9410986 | 11675 | 500 (501) | 9907708 |
| 10 | Pack-primes | A | 1013 | 2089 | 22857 | 0 | 0 | 22401 | 23943 | 1542 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | B | 1006 | 2074 | 22694 | 0 | 0 | 20397 | 21942 | 1545 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | C | 4189 | 11241 | 117929 | 0 | 0 | 1214583445 | 1214591335 | 7890 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-coprime | A | 1013 | 2089 | 22857 | 468 | 3744 | 0 | 0 | 0 | 159058 | 3666 | 13 (15) | 161185 |
| 10 | Pack-coprime | B | 1006 | 2074 | 22694 | 446 | 3568 | 0 | 0 | 0 | 147273 | 3687 | 14 (14) | 149358 |
| 10 | Pack-coprime | C | 4189 | 10060 | 107300 | 4560 | 36480 | 0 | 0 | 0 | 9399791 | 11771 | 508 (512) | 9917623 |
| 11 | Pack-primes | A | 280 | 565 | 6209 | 0 | 0 | 5631 | 6052 | 421 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | B | 283 | 559 | 6167 | 0 | 0 | 5283 | 5729 | 446 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-primes | C | 4189 | 11222 | 117758 | 0 | 0 | 982276394 | 982284316 | 7922 | 0 | 0 | 0 (0) | 0 |
| 11 | Pack-coprime | A | 280 | 565 | 6209 | 180 | 1440 | 0 | 0 | 0 | 20384 | 890 | 13 (13) | 21198 |
| 11 | Pack-coprime | B | 283 | 559 | 6167 | 175 | 1400 | 0 | 0 | 0 | 20098 | 916 | 12 (11) | 20821 |
| 11 | Pack-coprime | C | 4189 | 10120 | 107840 | 4547 | 36376 | 0 | 0 | 0 | 9430771 | 11969 | 489 (492) | 9930730 |
| 12 | Pack-primes | A | 90 | 186 | 2038 | 0 | 0 | 1197 | 1365 | 168 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | B | 84 | 162 | 1798 | 0 | 0 | 1438 | 1585 | 147 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | C | 4198 | 11120 | 116876 | 0 | 0 | 999764111 | 999771946 | 7835 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-coprime | A | 90 | 184 | 2020 | 60 | 480 | 0 | 0 | 0 | 2118 | 310 | 6 (8) | 2369 |
| 12 | Pack-coprime | B | 84 | 160 | 1780 | 61 | 488 | 0 | 0 | 0 | 2014 | 255 | 10 (11) | 2277 |
| 12 | Pack-coprime | C | 4198 | 10021 | 106985 | 4564 | 36512 | 0 | 0 | 0 | 9434301 | 11760 | 501 (502) | 9941867 |

### The factored rows' (f) and (q) at each matrix's end (after every reconciliation)

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 283 | 551 | 6095 | 0 | 0 |
| 1 | Pack-primes | B | 283 | 587 | 6419 | 0 | 0 |
| 1 | Pack-primes | C | 311 | 824 | 8664 | 0 | 0 |
| 1 | Pack-coprime | A | 283 | 551 | 6095 | 189 | 1512 |
| 1 | Pack-coprime | B | 283 | 587 | 6419 | 177 | 1416 |
| 1 | Pack-coprime | C | 311 | 680 | 7368 | 311 | 2488 |
| 2 | Pack-primes | A | 86 | 159 | 1779 | 0 | 0 |
| 2 | Pack-primes | B | 92 | 176 | 1956 | 0 | 0 |
| 2 | Pack-primes | C | 312 | 799 | 8443 | 0 | 0 |
| 2 | Pack-coprime | A | 86 | 156 | 1752 | 66 | 528 |
| 2 | Pack-coprime | B | 92 | 174 | 1938 | 63 | 504 |
| 2 | Pack-coprime | C | 312 | 656 | 7156 | 310 | 2480 |
| 3 | Pack-primes | A | 24 | 49 | 541 | 0 | 0 |
| 3 | Pack-primes | B | 26 | 46 | 522 | 0 | 0 |
| 3 | Pack-primes | C | 315 | 841 | 8833 | 0 | 0 |
| 3 | Pack-coprime | A | 24 | 43 | 487 | 20 | 160 |
| 3 | Pack-coprime | B | 26 | 44 | 504 | 21 | 168 |
| 3 | Pack-coprime | C | 315 | 711 | 7663 | 311 | 2488 |
| 4 | Pack-primes | A | 7 | 12 | 140 | 0 | 0 |
| 4 | Pack-primes | B | 8 | 17 | 189 | 0 | 0 |
| 4 | Pack-primes | C | 294 | 740 | 7840 | 0 | 0 |
| 4 | Pack-coprime | A | 7 | 9 | 113 | 7 | 56 |
| 4 | Pack-coprime | B | 8 | 15 | 171 | 8 | 64 |
| 4 | Pack-coprime | C | 294 | 613 | 6697 | 302 | 2416 |
| 5 | Pack-primes | A | 1006 | 2043 | 22415 | 0 | 0 |
| 5 | Pack-primes | B | 1016 | 2059 | 22599 | 0 | 0 |
| 5 | Pack-primes | C | 1094 | 2965 | 31065 | 0 | 0 |
| 5 | Pack-coprime | A | 1006 | 2043 | 22415 | 456 | 3648 |
| 5 | Pack-coprime | B | 1016 | 2059 | 22599 | 464 | 3712 |
| 5 | Pack-coprime | C | 1094 | 2532 | 27168 | 1175 | 9400 |
| 6 | Pack-primes | A | 283 | 558 | 6158 | 0 | 0 |
| 6 | Pack-primes | B | 280 | 546 | 6038 | 0 | 0 |
| 6 | Pack-primes | C | 1102 | 2945 | 30917 | 0 | 0 |
| 6 | Pack-coprime | A | 283 | 558 | 6158 | 183 | 1464 |
| 6 | Pack-coprime | B | 280 | 545 | 6029 | 176 | 1408 |
| 6 | Pack-coprime | C | 1102 | 2515 | 27047 | 1190 | 9520 |
| 7 | Pack-primes | A | 89 | 175 | 1935 | 0 | 0 |
| 7 | Pack-primes | B | 86 | 166 | 1842 | 0 | 0 |
| 7 | Pack-primes | C | 1102 | 2901 | 30521 | 0 | 0 |
| 7 | Pack-coprime | A | 89 | 171 | 1899 | 60 | 480 |
| 7 | Pack-coprime | B | 86 | 164 | 1824 | 61 | 488 |
| 7 | Pack-coprime | C | 1102 | 2490 | 26822 | 1167 | 9336 |
| 8 | Pack-primes | A | 29 | 54 | 606 | 0 | 0 |
| 8 | Pack-primes | B | 27 | 47 | 535 | 0 | 0 |
| 8 | Pack-primes | C | 1086 | 2849 | 29989 | 0 | 0 |
| 8 | Pack-coprime | A | 29 | 54 | 606 | 19 | 152 |
| 8 | Pack-coprime | B | 27 | 45 | 517 | 21 | 168 |
| 8 | Pack-coprime | C | 1086 | 2436 | 26272 | 1175 | 9400 |
| 9 | Pack-primes | A | 3533 | 7241 | 79305 | 0 | 0 |
| 9 | Pack-primes | B | 3589 | 7309 | 80141 | 0 | 0 |
| 9 | Pack-primes | C | 4188 | 11127 | 116899 | 0 | 0 |
| 9 | Pack-coprime | A | 3533 | 7241 | 79305 | 1104 | 8832 |
| 9 | Pack-coprime | B | 3589 | 7309 | 80141 | 1123 | 8984 |
| 9 | Pack-coprime | C | 4188 | 9985 | 106621 | 4556 | 36448 |
| 10 | Pack-primes | A | 1013 | 2089 | 22857 | 0 | 0 |
| 10 | Pack-primes | B | 1006 | 2074 | 22694 | 0 | 0 |
| 10 | Pack-primes | C | 4189 | 11241 | 117929 | 0 | 0 |
| 10 | Pack-coprime | A | 1013 | 2089 | 22857 | 468 | 3744 |
| 10 | Pack-coprime | B | 1006 | 2074 | 22694 | 446 | 3568 |
| 10 | Pack-coprime | C | 4189 | 10060 | 107300 | 4560 | 36480 |
| 11 | Pack-primes | A | 280 | 565 | 6209 | 0 | 0 |
| 11 | Pack-primes | B | 283 | 559 | 6167 | 0 | 0 |
| 11 | Pack-primes | C | 4189 | 11222 | 117758 | 0 | 0 |
| 11 | Pack-coprime | A | 280 | 565 | 6209 | 180 | 1440 |
| 11 | Pack-coprime | B | 283 | 559 | 6167 | 175 | 1400 |
| 11 | Pack-coprime | C | 4189 | 10120 | 107840 | 4547 | 36376 |
| 12 | Pack-primes | A | 90 | 186 | 2038 | 0 | 0 |
| 12 | Pack-primes | B | 84 | 162 | 1798 | 0 | 0 |
| 12 | Pack-primes | C | 4198 | 11120 | 116876 | 0 | 0 |
| 12 | Pack-coprime | A | 90 | 184 | 2020 | 60 | 480 |
| 12 | Pack-coprime | B | 84 | 160 | 1780 | 61 | 488 |
| 12 | Pack-coprime | C | 4198 | 10021 | 106985 | 4564 | 36512 |

## Also reported (rule-m3 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-pack's stores, from the model; the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | W_A | W_B | W_C |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 283 | 283 | 311 |
| 2 | 16 | 4 | 64 | 64 | 64 | 256 | 86 | 92 | 312 |
| 3 | 16 | 16 | 16 | 16 | 16 | 256 | 24 | 26 | 315 |
| 4 | 16 | 64 | 4 | 4 | 4 | 256 | 7 | 8 | 294 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1006 | 1016 | 1094 |
| 6 | 32 | 4 | 256 | 256 | 256 | 1024 | 283 | 280 | 1102 |
| 7 | 32 | 16 | 64 | 64 | 64 | 1024 | 89 | 86 | 1102 |
| 8 | 32 | 64 | 16 | 16 | 16 | 1024 | 29 | 27 | 1086 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4096 | 3533 | 3589 | 4188 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 4096 | 1013 | 1006 | 4189 |
| 11 | 64 | 16 | 256 | 256 | 256 | 4096 | 280 | 283 | 4189 |
| 12 | 64 | 64 | 64 | 64 | 64 | 4096 | 90 | 84 | 4198 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.3 §16.9)

- job 1, C = A x B: add (4096): zero_x 0.062, int_x 0.001, int_y 0.037, den 0.003, general 0.896; mul (4096): int_both 0.011, den 0.040, int_x 0.106, int_y 0.095, general 0.749; cross tests 16384 (matched 206, cross reads 16384); C's store W 311, found / new 201 / 311
- job 2, C = A x B: add (4096): zero_x 0.062, int_both 0.000, int_x 0.005, int_y 0.033, den 0.003, general 0.896; mul (4096): int_both 0.007, den 0.038, cross_ad 0.001, int_x 0.056, int_y 0.103, general 0.796; cross tests 16384 (matched 188, cross reads 16384); C's store W 312, found / new 200 / 312
- job 3, C = A x B: add (4096): zero_x 0.062, int_x 0.005, int_y 0.112, den 0.005, general 0.816; mul (4096): int_both 0.014, den 0.074, int_x 0.103, int_y 0.095, general 0.714; cross tests 16384 (matched 361, cross reads 16384); C's store W 315, found / new 197 / 315
- job 4, C = A x B: add (4096): zero_x 0.062, int_x 0.001, den 0.026, general 0.911; mul (4096): int_x 0.516, general 0.484; cross tests 16384 (matched 0, cross reads 16384); C's store W 294, found / new 218 / 294
- job 5, C = A x B: add (32768): zero_x 0.031, int_x 0.001, int_y 0.047, den 0.002, general 0.919; mul (32768): int_both 0.012, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.098, int_y 0.105, general 0.750; cross tests 131072 (matched 1564, cross reads 131072); C's store W 1094, found / new 954 / 1094
- job 6, C = A x B: add (32768): zero_x 0.031, int_x 0.003, int_y 0.043, den 0.003, general 0.920; mul (32768): int_both 0.013, den 0.035, num 0.000, cross_ad 0.000, int_x 0.088, int_y 0.112, general 0.751; cross tests 131072 (matched 1595, cross reads 131072); C's store W 1102, found / new 946 / 1102
- job 7, C = A x B: add (32768): zero_x 0.031, int_x 0.001, int_y 0.031, den 0.003, general 0.934; mul (32768): int_both 0.005, den 0.036, int_x 0.039, int_y 0.121, general 0.799; cross tests 131072 (matched 1362, cross reads 131072); C's store W 1102, found / new 946 / 1102
- job 8, C = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.002, int_y 0.036, den 0.004, general 0.926; mul (32768): int_both 0.013, den 0.040, int_x 0.104, int_y 0.108, general 0.734; cross tests 131072 (matched 1756, cross reads 131072); C's store W 1086, found / new 962 / 1086
- job 9, C = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.042, den 0.002, general 0.939; mul (262144): int_both 0.012, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.105, int_y 0.090, general 0.757; cross tests 1048576 (matched 12558, cross reads 1048576); C's store W 4188, found / new 4004 / 4188
- job 10, C = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.049, den 0.002, general 0.932; mul (262144): int_both 0.014, den 0.034, num 0.000, cross_cb 0.000, int_x 0.101, int_y 0.111, general 0.739; cross tests 1048576 (matched 12783, cross reads 1048576); C's store W 4189, found / new 4003 / 4189
- job 11, C = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.042, den 0.001, general 0.940; mul (262144): int_both 0.012, den 0.036, num 0.000, cross_cb 0.000, int_x 0.114, int_y 0.080, general 0.758; cross tests 1048576 (matched 12442, cross reads 1048576); C's store W 4189, found / new 4003 / 4189
- job 12, C = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.058, den 0.002, general 0.924; mul (262144): int_both 0.022, den 0.030, int_x 0.097, int_y 0.162, general 0.689; cross tests 1048576 (matched 13668, cross reads 1048576); C's store W 4198, found / new 3994 / 4198

### Per step: the factored arithmetic (rule-m3 item 13; counting runs)

Reconciliation (Pack-coprime): passes, gcds, splits, |D_L| and |D_R|, (f) rebuilds (entries written). The arithmetic: merges (entries compared), the temporaries' lists (comparisons forming them), divisibility tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, and the new sub-cases (9a-c num, 10a-c general, 12a-c multiplication's general).

| job | row | step | reconciliation | merges (entries) | list comparisons | tests | per-piece gcds | factor multiplications | leftover splits | 9a / 9b / 9c | 10a / 10b / 10c | 12a / 12b / 12c |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10626 (21396) | 18448 | 2317 | 0 | 17591 | 0 | 0 / 0 / 0 | 59 / 2265 / 1346 | 1730 / 1204 / 132 |
| 1 | Pack-coprime | C | 1 passes, 1098 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10626 (21396) | 18448 | 2588 | 1551 | 21541 | 0 | 0 / 0 / 0 | 59 / 2265 / 1346 | 1730 / 1204 / 132 |
| 2 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10843 (21428) | 17983 | 2540 | 0 | 19178 | 0 | 0 / 0 / 0 | 93 / 2145 / 1433 | 2019 / 1080 / 161 |
| 2 | Pack-coprime | C | 1 passes, 387 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10843 (21428) | 17983 | 2828 | 1668 | 23687 | 0 | 0 / 0 / 0 | 93 / 2145 / 1433 | 2019 / 1080 / 161 |
| 3 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10003 (17978) | 16061 | 1992 | 0 | 13243 | 0 | 0 / 0 / 0 | 140 / 1974 / 1229 | 1306 / 1419 / 200 |
| 3 | Pack-coprime | C | 1 passes, 123 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10003 (17978) | 16061 | 2274 | 1407 | 16565 | 0 | 0 / 0 / 0 | 140 / 1974 / 1229 | 1306 / 1419 / 200 |
| 4 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 9810 (21612) | 17230 | 3403 | 0 | 13660 | 0 | 0 / 0 / 0 | 23 / 1926 / 1781 | 1192 / 792 / 0 |
| 4 | Pack-coprime | C | 1 passes, 45 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 9810 (21612) | 17230 | 3924 | 2381 | 19043 | 0 | 0 / 0 / 0 | 23 / 1926 / 1781 | 1192 / 792 / 0 |
| 5 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 85882 (171841) | 146687 | 17511 | 0 | 149713 | 0 | 0 / 0 / 0 | 445 / 19368 / 10295 | 14226 / 9140 / 1196 |
| 5 | Pack-coprime | C | 1 passes, 2760 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 85882 (171841) | 146687 | 19129 | 11533 | 181035 | 0 | 0 / 0 / 0 | 445 / 19368 / 10295 | 14226 / 9140 / 1196 |
| 6 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 85949 (170307) | 146439 | 17088 | 0 | 149417 | 0 | 0 / 0 / 0 | 455 / 19756 / 9937 | 13845 / 9489 / 1287 |
| 6 | Pack-coprime | C | 1 passes, 1077 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 85949 (170307) | 146439 | 18671 | 11186 | 179917 | 0 | 0 / 0 / 0 | 455 / 19756 / 9937 | 13845 / 9489 / 1287 |
| 7 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 88181 (183353) | 161341 | 20267 | 0 | 147517 | 0 | 0 / 0 / 0 | 325 / 18453 / 11821 | 15973 / 9129 / 1074 |
| 7 | Pack-coprime | C | 1 passes, 363 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 88181 (183353) | 161341 | 22266 | 13529 | 182944 | 0 | 0 / 0 / 0 | 325 / 18453 / 11821 | 15973 / 9129 / 1074 |
| 8 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 85437 (178639) | 154459 | 21962 | 0 | 176432 | 0 | 0 / 0 / 0 | 253 / 18391 / 11715 | 12433 / 10863 / 770 |
| 8 | Pack-coprime | C | 1 passes, 120 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 85437 (178639) | 154459 | 23594 | 13480 | 216945 | 0 | 0 / 0 / 0 | 253 / 18391 / 11715 | 12433 / 10863 / 770 |
| 9 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 694204 (1404018) | 1208970 | 138100 | 0 | 1277193 | 0 | 0 / 0 / 0 | 2089 / 162820 / 81310 | 114100 / 74871 / 9389 |
| 9 | Pack-coprime | C | 1 passes, 6681 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 694204 (1404018) | 1208970 | 149826 | 91120 | 1533941 | 0 | 0 / 0 / 0 | 2089 / 162820 / 81310 | 114100 / 74871 / 9389 |
| 10 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 687584 (1394507) | 1196231 | 135494 | 0 | 1244324 | 0 | 0 / 0 / 0 | 2173 / 161789 / 80445 | 108483 / 75296 / 10026 |
| 10 | Pack-coprime | C | 1 passes, 2742 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 687584 (1394507) | 1196231 | 147274 | 89888 | 1494052 | 0 | 0 / 0 / 0 | 2173 / 161789 / 80445 | 108483 / 75296 / 10026 |
| 11 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 695039 (1406978) | 1213296 | 134560 | 0 | 1259485 | 0 | 0 / 0 / 0 | 2187 / 163543 / 80733 | 109648 / 78795 / 10381 |
| 11 | Pack-coprime | C | 1 passes, 1065 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 695039 (1406978) | 1213296 | 145840 | 89740 | 1511134 | 0 | 0 / 0 / 0 | 2187 / 163543 / 80733 | 109648 / 78795 / 10381 |
| 12 | Pack-primes | C | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 671131 (1333633) | 1125819 | 129893 | 0 | 1200312 | 0 | 0 / 0 / 0 | 2220 / 164356 / 75583 | 96284 / 72180 / 12032 |
| 12 | Pack-coprime | C | 1 passes, 363 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 671131 (1333633) | 1125819 | 140287 | 85318 | 1445060 | 0 | 0 / 0 / 0 | 2220 / 164356 / 75583 | 96284 / 72180 / 12032 |

The gates checked every counter and store figure against the model (spec v0.3 §16.9 item 2), the TC gate included: MHVP:1 pass, TC:1 pass, MHPC:1 pass, TC:2 pass, MHVP:2 pass, MHPP:1 pass, MHPC:2 pass, TC:3 pass, MHVP:3 pass, MHPP:2 pass, MHPC:3 pass, TC:4 pass, MHVP:4 pass, MHPP:3 pass, MHPC:4 pass, TC:5 pass, MHPP:4 pass, MHVP:5 pass, MHPC:5 pass, TC:6 pass, MHVP:6 pass, MHPP:5 pass, MHPC:6 pass, MHPP:6 pass, MHVP:7 pass, TC:7 pass, MHPC:7 pass, TC:8 pass, MHVP:8 pass, MHPP:7 pass, MHPC:8 pass, TC:9 pass, MHVP:9 pass, MHPP:8 pass, MHPC:9 pass, TC:10 pass, MHVP:10 pass, MHPP:9 pass, MHPC:10 pass, TC:11 pass, MHVP:11 pass, MHPP:10 pass, MHPC:11 pass, TC:12 pass, MHVP:12 pass, MHPP:11 pass, MHPC:12 pass, MHPP:12 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m3 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 2.565 | no | 1 | 14 | 0.071 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (6->6 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 5.459 | no | 2 | 14 | 0.143 | 14 | 0 | none | 11 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (6->6 threads), bash 0 (1->1 threads) |
| 10 | pilot | 1 | 15.458 | no | 2 | 14 | 0.143 | 15 | 1 | none | 21 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (7->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 13.216 | no | 18 | 64 | 0.281 | 65 | 1 | none | 19 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 1/7 (8->7 threads), bash 0/7 (1->1 threads) |
| 2 | measured | 1 | 12.116 | no | 18 | 64 | 0.281 | 65 | 1 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/7 (8->8 threads), claude 1/7 (7->8 threads), bash 0/7 (1->1 threads) |
| 3 | measured | 1 | 14.56 | no | 18 | 64 | 0.281 | 65 | 1 | none | 20 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/7 (8->8 threads), claude 1/7 (8->7 threads), bash 0/7 (1->1 threads) |
| 4 | measured | 1 | 15.386 | no | 18 | 64 | 0.281 | 66 | 2 | none | 21 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 2/7 (7->8 threads), bash 0/7 (1->1 threads) |
| 5 | measured | 1 | 27.787 | no | 19 | 64 | 0.297 | 66 | 2 | none | 33 | none | process_api 0/9 (6->6 threads), sh 0/9 (1->1 threads), environment-man 1/9 (8->8 threads), claude 2/9 (7->8 threads), bash 0/9 (1->1 threads) |
| 6 | measured | 1 | 28.496 | no | 19 | 64 | 0.297 | 66 | 2 | none | 34 | none | process_api 0/9 (6->6 threads), sh 0/9 (1->1 threads), environment-man 0/9 (8->8 threads), claude 2/9 (8->7 threads), bash 0/9 (1->1 threads) |
| 7 | measured | 1 | 22.207 | no | 18 | 64 | 0.281 | 66 | 2 | none | 28 | none | process_api 2/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 8 | measured | 1 | 32.896 | no | 17 | 64 | 0.266 | 66 | 2 | none | 38 | none | process_api 0/10 (6->6 threads), sh 0/10 (1->1 threads), environment-man 0/10 (8->8 threads), claude 3/10 (8->7 threads), bash 0/10 (1->1 threads) |
| 9 | measured | 1 | 71.672 | no | 19 | 64 | 0.297 | 68 | 4 | none | 77 | none | process_api 2/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/15 (8->8 threads), claude 9/15 (7->6 threads), bash 0/15 (1->1 threads) |
| 10 | measured | 1 | 72.632 | no | 19 | 64 | 0.297 | 69 | 5 | none | 78 | none | process_api 2/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 2/15 (8->8 threads), claude 8/15 (6->8 threads), bash 0/15 (1->1 threads) |
| 11 | measured | 1 | 64.646 | no | 18 | 64 | 0.281 | 68 | 4 | none | 70 | none | process_api 2/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 1/14 (8->8 threads), claude 6/14 (7->7 threads), bash 0/14 (1->1 threads) |
| 12 | measured | 1 | 62.082 | no | 18 | 64 | 0.281 | 67 | 3 | none | 68 | none | process_api 2/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 1/14 (8->8 threads), claude 6/14 (7->7 threads), bash 0/14 (1->1 threads) |

The runner's own ticks over the 15 blocks: 224 ticks over 810 launches, 0.277 per launch; the highest block 0.297 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 1.0000 | 0.9751 | 442409 | 443073 |
| 1 | Hash-V-pack | 12 | 0.9995 | 0.9795 | 525494 | 525598 |
| 1 | Pack-primes | 12 | 0.9997 | 0.9950 | 71702858 | 71711046 |
| 1 | Pack-coprime | 12 | 0.9994 | 0.9911 | 4321935 | 4340120 |
| 1 | Hash-V-pack-null | 12 | 0.9999 | 0.9828 | 526425 | 526402 |
| 2 | TC | 12 | 0.9992 | 0.9944 | 436634 | 436854 |
| 2 | Hash-V-pack | 12 | 0.9994 | 0.9870 | 514925 | 515055 |
| 2 | Pack-primes | 12 | 0.9996 | 0.9941 | 65176828 | 65283096 |
| 2 | Pack-coprime | 12 | 0.9997 | 0.9769 | 3331731 | 3337538 |
| 2 | Hash-V-pack-null | 12 | 1.0000 | 0.9132 | 515715 | 515440 |
| 3 | TC | 12 | 0.9994 | 0.8435 | 388076 | 388423 |
| 3 | Hash-V-pack | 12 | 0.9998 | 0.9931 | 464581 | 464803 |
| 3 | Pack-primes | 12 | 0.9991 | 0.8823 | 49947836 | 50027234 |
| 3 | Pack-coprime | 12 | 0.9995 | 0.8496 | 3025722 | 3027824 |
| 3 | Hash-V-pack-null | 12 | 0.9994 | 0.9911 | 464873 | 464686 |
| 4 | TC | 12 | 0.9998 | 0.9863 | 379362 | 379526 |
| 4 | Hash-V-pack | 12 | 0.9989 | 0.9915 | 456399 | 456744 |
| 4 | Pack-primes | 12 | 0.9992 | 0.9600 | 40562205 | 40600774 |
| 4 | Pack-coprime | 12 | 0.9998 | 0.9628 | 2901203 | 2903906 |
| 4 | Hash-V-pack-null | 12 | 0.9997 | 0.9780 | 455857 | 455857 |
| 5 | TC | 12 | 0.9997 | 0.9838 | 3722157 | 3731511 |
| 5 | Hash-V-pack | 12 | 0.9992 | 0.9792 | 4245631 | 4250190 |
| 5 | Pack-primes | 12 | 0.9978 | 0.9926 | 480450084 | 483248920 |
| 5 | Pack-coprime | 12 | 0.9995 | 0.9925 | 50553032 | 50614070 |
| 5 | Hash-V-pack-null | 12 | 0.9977 | 0.9904 | 4245652 | 4254281 |
| 6 | TC | 12 | 0.9995 | 0.9759 | 3701543 | 3704472 |
| 6 | Hash-V-pack | 12 | 0.9998 | 0.9962 | 4205056 | 4211905 |
| 6 | Pack-primes | 12 | 0.9985 | 0.9314 | 464627870 | 465724438 |
| 6 | Pack-coprime | 12 | 0.9989 | 0.9688 | 43408638 | 43488785 |
| 6 | Hash-V-pack-null | 12 | 0.9999 | 0.9973 | 4213583 | 4215376 |
| 7 | TC | 12 | 0.9999 | 0.9986 | 3706498 | 3706442 |
| 7 | Hash-V-pack | 12 | 0.9997 | 0.9944 | 4203453 | 4203831 |
| 7 | Pack-primes | 12 | 0.9992 | 0.9927 | 302424991 | 302635432 |
| 7 | Pack-coprime | 12 | 0.9999 | 0.9968 | 41226448 | 41233614 |
| 7 | Hash-V-pack-null | 12 | 0.9993 | 0.9876 | 4209141 | 4219794 |
| 8 | TC | 12 | 1.0000 | 0.9925 | 3559307 | 3559426 |
| 8 | Hash-V-pack | 12 | 0.9999 | 0.9854 | 4074225 | 4079673 |
| 8 | Pack-primes | 12 | 0.9975 | 0.9783 | 591344807 | 592720642 |
| 8 | Pack-coprime | 12 | 0.9998 | 0.9960 | 42193064 | 42202816 |
| 8 | Hash-V-pack-null | 12 | 1.0000 | 0.9915 | 4069611 | 4070862 |
| 9 | TC | 12 | 0.9995 | 0.9942 | 30494572 | 30534541 |
| 9 | Hash-V-pack | 12 | 0.9992 | 0.9971 | 34300823 | 34338219 |
| 9 | Pack-primes | 12 | 0.9928 | 0.9862 | 2965036634 | 2985753508 |
| 9 | Pack-coprime | 12 | 0.9987 | 0.9905 | 662857913 | 663920448 |
| 9 | Hash-V-pack-null | 12 | 0.9997 | 0.9875 | 34328119 | 34335081 |
| 10 | TC | 12 | 0.9999 | 0.9272 | 30356670 | 30381210 |
| 10 | Hash-V-pack | 12 | 0.9977 | 0.9923 | 34162208 | 34259540 |
| 10 | Pack-primes | 12 | 0.9935 | 0.9874 | 3184226532 | 3204992406 |
| 10 | Pack-coprime | 12 | 0.9991 | 0.9772 | 616857336 | 617585826 |
| 10 | Hash-V-pack-null | 12 | 0.9992 | 0.9653 | 34042831 | 34088423 |
| 11 | TC | 12 | 1.0000 | 0.9951 | 30585755 | 30585975 |
| 11 | Hash-V-pack | 12 | 0.9996 | 0.9914 | 34172643 | 34199994 |
| 11 | Pack-primes | 12 | 0.9922 | 0.9719 | 2593099990 | 2607097298 |
| 11 | Pack-coprime | 12 | 0.9983 | 0.9824 | 609129404 | 609861724 |
| 11 | Hash-V-pack-null | 12 | 0.9994 | 0.9948 | 34149605 | 34249409 |
| 12 | TC | 12 | 0.9998 | 0.9962 | 29635010 | 29646629 |
| 12 | Hash-V-pack | 12 | 0.9999 | 0.9959 | 33330692 | 33351578 |
| 12 | Pack-primes | 12 | 0.9975 | 0.9777 | 2630985601 | 2637687820 |
| 12 | Pack-coprime | 12 | 0.9994 | 0.9939 | 603295842 | 604052260 |
| 12 | Hash-V-pack-null | 12 | 1.0000 | 0.9912 | 33343812 | 33347988 |

Idle checks: 15 (0 busy). Host rates R_h: {'1': 0.06469142191745375, '79': 0.06469142191745375, '84': 0.06469142191745375, '92': 0.06469142191745375, '20580': 0.06469142191745375} (measured: {'1': 0.0, '79': 0.0, '84': 0.0, '92': 0.06469064844193971, '20580': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

