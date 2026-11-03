# Design 3 on matrices, round m3 (factored whole numbers), the sum of products A B + C D: results

Run started 2026-10-30T14:51:40-05:00.

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

- The sop run (job set `sop`: P = A x B; Q = C x D; R = P + Q).
- Started 2026-10-30T14:51:40-05:00; finished 2026-10-30T15:08:29-05:00.
- Step 0 (before the clock): 185.4 s; step 0's files: equal to m2's by sha256.
- Run time 13.7 minutes (hard stop 40).
- Rule sha256 `c33795209798503b17c6a8b1bb625680d2dfe71fb9fdcda3cbe1a1b0d51004fd`; round 3's results after the run: unchanged; m1's and m2's results and the other m3 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[21895], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T14:54:45-05:00: TC `3cbbe6f47fb7`, MHVP `7ed2109b9193`, MHPP `6704a1ab9994`, MHPC `0da57d01e10e`, TC3GATE `685595cafdb3`
- Unit tests: m3_unit: m3_unit: 811293 checks, 0 failures; m3_diff: m3_diff: 3000 seeds, 3000 jobs, 1050092 operations; obtained gcds checked against Euclid 2936692, merges 2186690, reconciliation splits 2796, leftover splits 106; 1523395 checks, 0 failures; m3_alloc-timing: m3_alloc (timing build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-counting: m3_alloc (counting build): 160 jobs, 2893 moments, 38800 checks, 0 failures; m3_alloc-memory: m3_alloc (memory build): 160 jobs, 2893 moments, 43830 checks, 0 failures; m3_unit-asan: m3_unit: 811293 checks, 0 failures; m3_diff-asan: m3_diff: 600 seeds, 600 jobs, 209812 operations; obtained gcds checked against Euclid 579628, merges 431634, reconciliation splits 557, leftover splits 20; 303709 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 10], one per size; block wall times job 1 6.1 s, job 6 10.3 s, job 10 37.9 s.
- Per size class c (its pilot job): O_c the block's checks, K the calibration launches, P the two round launches (0 over cap), t the time per run from the calibration; N_c the run's jobs of that size:
  - size 0 (job 1, N_c = 3): O 2.0 s; TC: K 0.12 s, P 0.36 s, t 0.001 s; Hash-V-pack: K 0.15 s, P 0.43 s, t 0.001 s; Pack-primes: K 0.27 s, P 1.59 s, t 0.264 s; Pack-coprime: K 0.17 s, P 0.55 s, t 0.012 s; Hash-V-pack-null: K 0.00 s, P 0.44 s, t n/a s
  - size 1 (job 6, N_c = 3): O 2.0 s; TC: K 0.12 s, P 0.37 s, t 0.008 s; Hash-V-pack: K 0.13 s, P 0.42 s, t 0.009 s; Pack-primes: K 1.98 s, P 3.97 s, t 1.981 s; Pack-coprime: K 0.13 s, P 0.78 s, t 0.127 s; Hash-V-pack-null: K 0.00 s, P 0.42 s, t n/a s
  - size 2 (job 10, N_c = 3): O 2.0 s; TC: K 0.06 s, P 0.38 s, t 0.062 s; Hash-V-pack: K 0.07 s, P 0.44 s, t 0.070 s; Pack-primes: K 9.68 s, P 19.33 s, t 9.673 s; Pack-coprime: K 1.83 s, P 3.68 s, t 1.830 s; Hash-V-pack-null: K 0.00 s, P 0.44 s, t n/a s
- Projection elapsed + 1.2 [sum_c N_c (O_c + sum_rows (K + 6 P)) + 1/2 sum_c N_c sum_rows 4 t] = 17.1 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.027838868628378942, '79': 0.027838868628378942, '84': 0.027838868628378942, '92': 0.11135501719565598, '21895': 0.027838868628378942}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 4 sum-of-products time; 6 stable bytes of one stored input matrix; peak bytes during the operation (reported). Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-pack against Hash-V-pack-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses; (f) and (q) included). Pack-coprime against Pack-primes is reported with no verdict on any measure. A row over cap runs no rounds: its time verdicts are over cap, with its calibration rep's time; its memory verdicts stand. Gcd work: a check for Hash-V-pack, reported for the factored rows. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-pack, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m3 item 8, carried over).

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.474 [0.471, 0.479], h 0.050 | loss, 0.867 [0.863, 0.871], h 0.050 | reported, 0.845 [0.842, 0.849] | reported, 0.592 [0.570, 0.725] | win, 0.802 (heap 0.819) | reported, 1.177 (heap 1.187) |
| 2 (n 16, rho 4) | loss, 0.551 [0.548, 0.553], h 0.050 | loss, 0.867 [0.862, 0.871], h 0.050 | reported, 0.852 [0.847, 0.856] | reported, 0.592 [0.568, 0.677] | win, 0.294 (heap 0.321) | reported, 0.890 (heap 0.906) |
| 4 (n 16, rho 64) | loss, 0.629 [0.610, 0.642], h 0.050 | loss, 0.857 [0.854, 0.863], h 0.050 | reported, 0.849 [0.846, 0.854] | reported, 0.585 [0.567, 0.642] | win, 0.141 (heap 0.170) | reported, 0.795 (heap 0.812) |
| 5 (n 32, rho 1) | loss, 0.496 [0.495, 0.500], h 0.050 | loss, 0.882 [0.880, 0.886], h 0.050 | reported, 0.872 [0.870, 0.876] | reported, 0.600 [0.578, 0.653] | win, 0.743 (heap 0.748) | reported, 1.118 (heap 1.121) |
| 6 (n 32, rho 4) | loss, 0.561 [0.553, 0.564], h 0.050 | loss, 0.882 [0.871, 0.885], h 0.050 | reported, 0.874 [0.863, 0.877] | reported, 0.575 [0.545, 0.677] | win, 0.388 (heap 0.394) | reported, 0.915 (heap 0.919) |
| 8 (n 32, rho 64) | loss, 0.626 [0.620, 0.629], h 0.050 | loss, 0.879 [0.878, 0.882], h 0.050 | reported, 0.874 [0.873, 0.877] | reported, 0.639 [0.589, 0.775] | win, 0.139 (heap 0.146) | reported, 0.771 (heap 0.776) |
| 9 (n 64, rho 1) | loss, 0.513 [0.510, 0.514], h 0.050 | loss, 0.892 [0.888, 0.896], h 0.050 | reported, 0.886 [0.883, 0.891] | reported, 0.679 [0.638, 0.696] | win, 0.682 (heap 0.683) | reported, 1.073 (heap 1.074) |
| 10 (n 64, rho 4) | loss, 0.585 [0.583, 0.589], h 0.050 | loss, 0.889 [0.886, 0.895], h 0.050 | reported, 0.886 [0.882, 0.892] | reported, 0.762 [0.665, 0.787] | win, 0.374 (heap 0.376) | reported, 0.897 (heap 0.898) |
| 12 (n 64, rho 64) | loss, 0.662 [0.653, 0.677], h 0.050 | loss, 0.893 [0.891, 0.896], h 0.050 | reported, 0.891 [0.889, 0.893] | reported, 0.701 [0.646, 0.841] | win, 0.136 (heap 0.138) | reported, 0.761 (heap 0.762) |

### Pack-primes against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.196 [0.193, 0.198], h 0.050 | loss, 0.003 [0.003, 0.003], h 0.050 | reported, 0.004 [0.003, 0.004] | reported, 0.400 [0.321, 0.515] | loss, 2.338 (heap 2.338) **severe memory loss** | reported, 2.716 (heap 2.709) |
| 2 (n 16, rho 4) | loss, 0.375 [0.373, 0.376], h 0.050 | loss, 0.004 [0.004, 0.004], h 0.050 | reported, 0.004 [0.004, 0.004] | reported, 0.453 [0.321, 0.526] | win, 0.739 (heap 0.772) | reported, 1.814 (heap 1.824) |
| 4 (n 16, rho 64) | loss, 0.566 [0.552, 0.577], h 0.050 | loss, 0.015 [0.015, 0.015], h 0.050 | reported, 0.015 [0.015, 0.015] | reported, 0.478 [0.404, 0.533] | win, 0.185 (heap 0.229) | reported, 1.426 (heap 1.444) |
| 5 (n 32, rho 1) | loss, 0.199 [0.197, 0.202], h 0.050 (c) | loss, 0.004 [0.004, 0.004], h 0.050 (c) | reported, 0.004 [0.004, 0.004] | reported, 0.098 [0.076, 0.144] | loss, 2.099 (heap 2.100) **severe memory loss** | reported, 2.444 (heap 2.443) |
| 6 (n 32, rho 4) | loss, 0.373 [0.370, 0.377], h 0.050 (c) | loss, 0.004 [0.004, 0.004], h 0.050 (c) | reported, 0.004 [0.004, 0.004] | reported, 0.097 [0.078, 0.168] | win, 0.767 (heap 0.774) | reported, 1.691 (heap 1.694) |
| 8 (n 32, rho 64) | loss, 0.609 [0.599, 0.615], h 0.050 (c) | loss, 0.006 [0.006, 0.006], h 0.050 (c) | reported, 0.006 [0.006, 0.006] | reported, 0.188 [0.105, 0.233] | win, 0.174 (heap 0.185) | reported, 1.338 (heap 1.343) |
| 9 (n 64, rho 1) | loss, 0.218 [0.206, 0.220], h 0.050 (c) | loss, 0.006 [0.006, 0.006], h 0.050 (c) | reported, 0.006 [0.006, 0.006] | reported, 0.125 [0.101, 0.132] | loss, 1.889 (heap 1.890) | reported, 2.283 (heap 2.283) |
| 10 (n 64, rho 4) | loss, 0.385 [0.364, 0.404], h 0.050 (c) | loss, 0.006 [0.006, 0.006], h 0.050 (c) | reported, 0.006 [0.006, 0.006] | reported, 0.169 [0.151, 0.182] | win, 0.716 (heap 0.718) | reported, 1.614 (heap 1.615) |
| 12 (n 64, rho 64) | loss, 0.637 [0.625, 0.646], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.151 [0.134, 0.174] | win, 0.165 (heap 0.168) | reported, 1.298 (heap 1.299) |

### Pack-coprime against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.014 [0.014, 0.014], h 0.050 | loss, 0.100 [0.099, 0.100], h 0.050 | reported, 0.084 [0.083, 0.084] | reported, 0.570 [0.521, 0.698] | loss, 2.675 (heap 2.675) **severe memory loss** | reported, 3.073 (heap 3.067) |
| 2 (n 16, rho 4) | loss, 0.097 [0.097, 0.098], h 0.050 | loss, 0.101 [0.101, 0.102], h 0.050 | reported, 0.101 [0.101, 0.102] | reported, 0.559 [0.537, 0.653] | win, 0.859 (heap 0.897) | reported, 2.048 (heap 2.062) |
| 4 (n 16, rho 64) | loss, 0.526 [0.512, 0.538], h 0.050 | loss, 0.105 [0.105, 0.106], h 0.050 | reported, 0.108 [0.107, 0.108] | reported, 0.554 [0.540, 0.614] | win, 0.195 (heap 0.247) | reported, 1.621 (heap 1.645) |
| 5 (n 32, rho 1) | loss, 0.007 [0.007, 0.007], h 0.050 | loss, 0.060 [0.060, 0.060], h 0.050 | reported, 0.053 [0.053, 0.054] | reported, 0.589 [0.550, 0.651] | loss, 2.331 (heap 2.333) **severe memory loss** | reported, 2.756 (heap 2.756) |
| 6 (n 32, rho 4) | loss, 0.050 [0.049, 0.050], h 0.050 | loss, 0.059 [0.059, 0.059], h 0.050 | reported, 0.059 [0.059, 0.059] | reported, 0.535 [0.507, 0.632] | win, 0.854 (heap 0.863) | reported, 1.926 (heap 1.930) |
| 8 (n 32, rho 64) | loss, 0.547 [0.543, 0.549], h 0.050 | loss, 0.060 [0.059, 0.060], h 0.050 | reported, 0.060 [0.060, 0.061] | reported, 0.579 [0.521, 0.729] | win, 0.182 (heap 0.195) | reported, 1.524 (heap 1.530) |
| 9 (n 64, rho 1) | loss, 0.004 [0.004, 0.004], h 0.050 (c) | loss, 0.034 [0.033, 0.034], h 0.050 (c) | reported, 0.032 [0.032, 0.032] | reported, 0.226 [0.143, 0.240] | loss, 2.025 (heap 2.026) **severe memory loss** | reported, 2.555 (heap 2.555) |
| 10 (n 64, rho 4) | loss, 0.026 [0.025, 0.026], h 0.050 (c) | loss, 0.033 [0.033, 0.034], h 0.050 (c) | reported, 0.033 [0.033, 0.034] | reported, 0.200 [0.139, 0.248] | win, 0.774 (heap 0.776) | reported, 1.840 (heap 1.841) |
| 12 (n 64, rho 64) | loss, 0.449 [0.441, 0.454], h 0.050 (c) | loss, 0.034 [0.034, 0.035], h 0.050 (c) | reported, 0.035 [0.034, 0.035] | reported, 0.241 [0.201, 0.306] | win, 0.172 (heap 0.176) | reported, 1.498 (heap 1.499) |

### Pack-primes against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.413 [0.407, 0.417], h 0.050 | loss, 0.004 [0.004, 0.004], h 0.050 | reported, 0.004 [0.004, 0.004] | reported, 0.676 [0.542, 0.755] | loss, 2.915 (heap 2.855) **severe memory loss** | reported, 2.307 (heap 2.281) |
| 2 (n 16, rho 4) | loss, 0.680 [0.678, 0.684], h 0.050 | loss, 0.005 [0.005, 0.005], h 0.050 | reported, 0.005 [0.005, 0.005] | reported, 0.766 [0.533, 0.841] | loss, 2.511 (heap 2.406) **severe memory loss** | reported, 2.037 (heap 2.015) |
| 4 (n 16, rho 64) | loss, 0.901 [0.890, 0.916], h 0.050 | loss, 0.017 [0.017, 0.017], h 0.050 | reported, 0.018 [0.018, 0.018] | reported, 0.816 [0.685, 0.891] | loss, 1.316 (heap 1.343) | reported, 1.795 (heap 1.778) |
| 5 (n 32, rho 1) | loss, 0.402 [0.396, 0.408], h 0.050 (c) | loss, 0.005 [0.005, 0.005], h 0.050 (c) | reported, 0.005 [0.005, 0.005] | reported, 0.164 [0.126, 0.234] | loss, 2.823 (heap 2.809) **severe memory loss** | reported, 2.186 (heap 2.180) |
| 6 (n 32, rho 4) | loss, 0.664 [0.658, 0.675], h 0.050 (c) | loss, 0.004 [0.004, 0.004], h 0.050 (c) | reported, 0.004 [0.004, 0.004] | reported, 0.168 [0.142, 0.277] | loss, 1.975 (heap 1.962) | reported, 1.848 (heap 1.843) |
| 8 (n 32, rho 64) | tie, 0.973 [0.961, 0.980], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.295 [0.160, 0.312] | loss, 1.258 (heap 1.265) | reported, 1.735 (heap 1.731) |
| 9 (n 64, rho 1) | loss, 0.426 [0.402, 0.430], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.184 [0.152, 0.192] | loss, 2.771 (heap 2.767) **severe memory loss** | reported, 2.128 (heap 2.127) |
| 10 (n 64, rho 4) | loss, 0.658 [0.622, 0.690], h 0.050 (c) | loss, 0.007 [0.007, 0.007], h 0.050 (c) | reported, 0.007 [0.007, 0.007] | reported, 0.222 [0.211, 0.246] | loss, 1.914 (heap 1.911) | reported, 1.799 (heap 1.798) |
| 12 (n 64, rho 64) | tie, 0.963 [0.933, 0.979], h 0.050 (c) | loss, 0.008 [0.008, 0.008], h 0.050 (c) | reported, 0.008 [0.008, 0.008] | reported, 0.215 [0.191, 0.243] | loss, 1.215 (heap 1.218) | reported, 1.705 (heap 1.704) |

### Pack-coprime against Hash-V-pack (factored against pack)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.029 [0.029, 0.030], h 0.050 | loss, 0.115 [0.114, 0.116], h 0.050 | reported, 0.099 [0.098, 0.099] | reported, 0.963 [0.901, 0.983] | loss, 3.334 (heap 3.267) **severe memory loss** | reported, 2.610 (heap 2.583) |
| 2 (n 16, rho 4) | loss, 0.177 [0.176, 0.178], h 0.050 | loss, 0.117 [0.116, 0.118], h 0.050 | reported, 0.119 [0.118, 0.119] | reported, 0.945 [0.929, 0.983] | loss, 2.916 (heap 2.794) **severe memory loss** | reported, 2.300 (heap 2.277) |
| 4 (n 16, rho 64) | loss, 0.836 [0.826, 0.849], h 0.050 | loss, 0.123 [0.122, 0.123], h 0.050 | reported, 0.127 [0.126, 0.127] | reported, 0.946 [0.929, 0.980] | loss, 1.389 (heap 1.449) | reported, 2.040 (heap 2.026) |
| 5 (n 32, rho 1) | loss, 0.014 [0.014, 0.014], h 0.050 | loss, 0.068 [0.067, 0.068], h 0.050 | reported, 0.061 [0.061, 0.062] | reported, 0.983 [0.948, 1.007] | loss, 3.136 (heap 3.121) **severe memory loss** | reported, 2.466 (heap 2.460) |
| 6 (n 32, rho 4) | loss, 0.089 [0.088, 0.090], h 0.050 | loss, 0.067 [0.067, 0.068], h 0.050 | reported, 0.067 [0.067, 0.068] | reported, 0.930 [0.894, 0.967] | loss, 2.200 (heap 2.189) **severe memory loss** | reported, 2.104 (heap 2.100) |
| 8 (n 32, rho 64) | loss, 0.874 [0.869, 0.877], h 0.050 | loss, 0.068 [0.068, 0.068], h 0.050 | reported, 0.069 [0.069, 0.069] | reported, 0.906 [0.877, 0.968] | loss, 1.315 (heap 1.334) | reported, 1.975 (heap 1.972) |
| 9 (n 64, rho 1) | loss, 0.008 [0.008, 0.008], h 0.050 (c) | loss, 0.038 [0.037, 0.038], h 0.050 (c) | reported, 0.036 [0.036, 0.036] | reported, 0.333 [0.210, 0.362] | loss, 2.970 (heap 2.966) **severe memory loss** | reported, 2.382 (heap 2.380) |
| 10 (n 64, rho 4) | loss, 0.044 [0.043, 0.045], h 0.050 (c) | loss, 0.037 [0.037, 0.038], h 0.050 (c) | reported, 0.037 [0.037, 0.038] | reported, 0.262 [0.194, 0.319] | loss, 2.068 (heap 2.066) **severe memory loss** | reported, 2.051 (heap 2.050) |
| 12 (n 64, rho 64) | loss, 0.679 [0.661, 0.690], h 0.050 (c) | loss, 0.039 [0.038, 0.039], h 0.050 (c) | reported, 0.039 [0.039, 0.039] | reported, 0.345 [0.294, 0.371] | loss, 1.269 (heap 1.276) | reported, 1.968 (heap 1.967) |

### Pack-coprime against Pack-primes (coprime against primes)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 0.071 [0.070, 0.072] | reported, 29.361 [29.209, 29.558] | reported, 23.907 [23.766, 24.046] | reported, 1.425 [1.267, 1.791] | reported, 1.144 (heap 1.144) | reported, 1.131 (heap 1.132) |
| 2 (n 16, rho 4) | reported, 0.260 [0.259, 0.261] | reported, 23.776 [23.704, 23.908] | reported, 23.016 [22.939, 23.140] | reported, 1.234 [1.128, 1.805] | reported, 1.161 (heap 1.161) | reported, 1.129 (heap 1.130) |
| 4 (n 16, rho 64) | reported, 0.928 [0.918, 0.936] | reported, 7.064 [7.029, 7.126] | reported, 7.029 [6.996, 7.092] | reported, 1.159 [1.069, 1.369] | reported, 1.055 (heap 1.079) | reported, 1.137 (heap 1.139) |
| 5 (n 32, rho 1) | reported, 0.034 [0.034, 0.035] | reported, 14.683 [14.646, 14.726] | reported, 12.907 [12.875, 12.943] | reported, 6.004 [4.212, 7.810] | reported, 1.111 (heap 1.111) | reported, 1.128 (heap 1.128) |
| 6 (n 32, rho 4) | reported, 0.133 [0.131, 0.135] | reported, 15.898 [15.764, 15.960] | reported, 15.608 [15.475, 15.667] | reported, 5.531 [3.310, 6.793] | reported, 1.114 (heap 1.116) | reported, 1.139 (heap 1.139) |
| 8 (n 32, rho 64) | reported, 0.898 [0.891, 0.911] | reported, 9.870 [9.773, 9.927] | reported, 9.855 [9.758, 9.913] | reported, 3.072 [2.851, 5.779] | reported, 1.046 (heap 1.055) | reported, 1.139 (heap 1.139) |
| 9 (n 64, rho 1) | reported, 0.019 [0.019, 0.021] | reported, 5.315 [5.260, 5.337] | reported, 5.004 [4.965, 5.032] | reported, 1.815 [1.149, 2.212] | reported, 1.072 (heap 1.072) | reported, 1.119 (heap 1.119) |
| 10 (n 64, rho 4) | reported, 0.066 [0.063, 0.071] | reported, 5.263 [5.238, 5.343] | reported, 5.212 [5.188, 5.292] | reported, 1.181 [0.867, 1.452] | reported, 1.081 (heap 1.081) | reported, 1.140 (heap 1.140) |
| 12 (n 64, rho 64) | reported, 0.705 [0.692, 0.716] | reported, 4.776 [4.716, 4.794] | reported, 4.773 [4.714, 4.792] | reported, 1.601 [1.320, 1.876] | reported, 1.045 (heap 1.048) | reported, 1.154 (heap 1.154) |

### Gcd work and the call-for-call checks

Hash-V-pack: G(row)/G(TC) MUST be exactly 1, and its per-operation records equal TC's, every step (a check, carried over). The factored rows (rule-m3 item 7, v0.3 §16.9 item 3): per operation the same case and the same exact divisions, multiplications and additions as TC (checked); their gcds are obtained otherwise, so G(row)/G(TC) is reported, not checked, per part (entry; the operations' additions and multiplications), beside their factor work; and each factored row takes Hash-V-pack's arithmetic counters (but the operations' gcds) and store splits (v0.3 §16.9 item 4).

| job | G per operation (TC) | Hash-V-pack: G/G(TC), call for call | Pack-primes: G/G(TC) (entry, add, mul), call for call | Pack-primes vs Hash-V-pack | Pack-coprime: G/G(TC) (entry, add, mul), call for call | Pack-coprime vs Hash-V-pack |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 34.01 | 1.000000 (passed), pass | 0.0253 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2420 (1.0000, 0.3273, 0.0000), pass (case and core arithmetic) | pass |
| 2 | 33.96 | 1.000000 (passed), pass | 0.0252 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2513 (1.0000, 0.3433, 0.0000), pass (case and core arithmetic) | pass |
| 4 | 36.30 | 1.000000 (passed), pass | 0.0238 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.3118 (1.0000, 0.4572, 0.0000), pass (case and core arithmetic) | pass |
| 5 | 35.99 | 1.000000 (passed), pass | 0.0121 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2200 (1.0000, 0.3020, 0.0000), pass (case and core arithmetic) | pass |
| 6 | 35.78 | 1.000000 (passed), pass | 0.0122 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2168 (1.0000, 0.2982, 0.0000), pass (case and core arithmetic) | pass |
| 8 | 35.24 | 1.000000 (passed), pass | 0.0121 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2311 (1.0000, 0.3159, 0.0000), pass (case and core arithmetic) | pass |
| 9 | 37.38 | 1.000000 (passed), pass | 0.0058 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1958 (1.0000, 0.2701, 0.0000), pass (case and core arithmetic) | pass |
| 10 | 37.49 | 1.000000 (passed), pass | 0.0058 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.1938 (1.0000, 0.2671, 0.0000), pass (case and core arithmetic) | pass |
| 12 | 38.34 | 1.000000 (passed), pass | 0.0057 (1.0000, 0.0000, 0.0000), pass (case and core arithmetic) | pass | 0.2030 (1.0000, 0.2831, 0.0000), pass (case and core arithmetic) | pass |

Factor work of the factored rows beside the gcd work (counting runs, the job's factorings and steps summed): merges and entries compared, divisibility tests, per-piece gcds, trial-division candidates, refinement gcds (pass 1 and the reconciliation).

| job | row | G(row)/G(TC) | merges | entries compared | tests | per-piece gcds | trial-division candidates | pass-1 gcds | pass-2 tests | reconciliation gcds | splits (pass 1 / reconciliation / leftover) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | 0.0253 | 21378 | 42691 | 5085 | 0 | 100937366 | 0 | 0 | 0 | 0 / 0 / 0 |
| 1 | Pack-coprime | 0.2420 | 21378 | 42691 | 5118 | 3385 | 0 | 201348 | 217816 | 3915 | 203 / 0 / 0 |
| 2 | Pack-primes | 0.0252 | 21914 | 43432 | 5479 | 0 | 80024481 | 0 | 0 | 0 | 0 / 0 / 0 |
| 2 | Pack-coprime | 0.2513 | 21914 | 43432 | 5585 | 3607 | 0 | 131914 | 143968 | 2586 | 178 / 0 / 0 |
| 4 | Pack-primes | 0.0238 | 23821 | 52256 | 8764 | 0 | 21400845 | 0 | 0 | 0 | 0 / 0 / 0 |
| 4 | Pack-coprime | 0.3118 | 23821 | 52256 | 8753 | 5191 | 0 | 123048 | 136970 | 1976 | 171 / 1 / 0 |
| 5 | Pack-primes | 0.0121 | 172925 | 353761 | 39700 | 0 | 698409868 | 0 | 0 | 0 | 0 / 0 / 0 |
| 5 | Pack-coprime | 0.2200 | 172925 | 353761 | 40920 | 26010 | 0 | 2514160 | 2650597 | 12786 | 536 / 0 / 0 |
| 6 | Pack-primes | 0.0122 | 172264 | 348617 | 38714 | 0 | 756387095 | 0 | 0 | 0 | 0 / 0 / 0 |
| 6 | Pack-coprime | 0.2168 | 172264 | 348617 | 39784 | 25312 | 0 | 1944787 | 2081998 | 9273 | 555 / 0 / 0 |
| 8 | Pack-primes | 0.0121 | 171138 | 361561 | 42660 | 0 | 445399845 | 0 | 0 | 0 | 0 / 0 / 0 |
| 8 | Pack-coprime | 0.2311 | 171138 | 361561 | 43251 | 27178 | 0 | 1833207 | 1954226 | 7260 | 470 / 0 / 0 |
| 9 | Pack-primes | 0.0058 | 1390040 | 2823309 | 287901 | 0 | 3664372140 | 0 | 0 | 0 | 0 / 0 / 0 |
| 9 | Pack-coprime | 0.1958 | 1390040 | 2823309 | 301006 | 188422 | 0 | 32447126 | 33993331 | 40665 | 1563 / 0 / 0 |
| 10 | Pack-primes | 0.0058 | 1393109 | 2836553 | 285652 | 0 | 3668447184 | 0 | 0 | 0 | 0 / 0 / 0 |
| 10 | Pack-coprime | 0.1938 | 1393109 | 2836553 | 298665 | 187354 | 0 | 28972257 | 30467242 | 33030 | 1539 / 0 / 0 |
| 12 | Pack-primes | 0.0057 | 1449680 | 2949603 | 311926 | 0 | 3240326279 | 0 | 0 | 0 | 0 / 0 / 0 |
| 12 | Pack-coprime | 0.2030 | 1449680 | 2949603 | 324303 | 202794 | 0 | 28288513 | 29771350 | 28056 | 1533 / 0 / 0 |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m3 item 11): NONE (rho = 1), LOW (rho = 4), HIGH (rho = 64) (sop has no MID). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.496 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.882 | 0 |
| NONE | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.743 | 0 |
| NONE | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.199 | 0 |
| NONE | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.004 | 0 |
| NONE | Pack-primes vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.099 | 2 |
| NONE | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.007 | 0 |
| NONE | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.060 | 0 |
| NONE | Pack-coprime vs TC | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.331 | 3 |
| NONE | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.413 | 0 |
| NONE | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| NONE | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.823 | 3 |
| NONE | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.014 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.068 | 0 |
| NONE | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 3.136 | 3 |
| NONE | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.034 | 0 |
| NONE | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 14.683 | 0 |
| NONE | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.111 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.561 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.882 | 0 |
| LOW | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.374 | 0 |
| LOW | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.375 | 0 |
| LOW | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.004 | 0 |
| LOW | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.739 | 0 |
| LOW | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.050 | 0 |
| LOW | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.059 | 0 |
| LOW | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.854 | 0 |
| LOW | Pack-primes vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.664 | 0 |
| LOW | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.005 | 0 |
| LOW | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.975 | 1 |
| LOW | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.089 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.067 | 0 |
| LOW | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 2.200 | 3 |
| LOW | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.133 | 0 |
| LOW | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 15.898 | 0 |
| LOW | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.114 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.629 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.879 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.139 | 0 |
| HIGH | Pack-primes vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.609 | 0 |
| HIGH | Pack-primes vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.007 | 0 |
| HIGH | Pack-primes vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.174 | 0 |
| HIGH | Pack-coprime vs TC | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.526 | 0 |
| HIGH | Pack-coprime vs TC | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.060 | 0 |
| HIGH | Pack-coprime vs TC | stable | 3 | 0 | 0 | 0 | 0 | 0 | 0.182 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | build | 0 | 2 | 1 | 0 | 0 | 0 | 0.963 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.008 | 0 |
| HIGH | Pack-primes vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.258 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | build | 0 | 0 | 3 | 0 | 0 | 0 | 0.836 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | operation | 0 | 0 | 3 | 0 | 0 | 0 | 0.068 | 0 |
| HIGH | Pack-coprime vs Hash-V-pack | stable | 0 | 0 | 3 | 0 | 0 | 0 | 1.315 | 0 |
| HIGH | Pack-coprime vs Pack-primes | build | 0 | 0 | 0 | 0 | 0 | 3 | 0.898 | 0 |
| HIGH | Pack-coprime vs Pack-primes | operation | 0 | 0 | 0 | 0 | 0 | 3 | 7.064 | 0 |
| HIGH | Pack-coprime vs Pack-primes | stable | 0 | 0 | 0 | 0 | 0 | 3 | 1.046 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.474 | 0.551 | n/a | 0.629 |
| 32 | 0.496 | 0.561 | n/a | 0.626 |
| 64 | 0.513 | 0.585 | n/a | 0.662 |

Hash-V-pack against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.867 | 0.867 | n/a | 0.857 |
| 32 | 0.882 | 0.882 | n/a | 0.879 |
| 64 | 0.892 | 0.889 | n/a | 0.893 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.802 | 0.294 | n/a | 0.141 |
| 32 | 0.743 | 0.388 | n/a | 0.139 |
| 64 | 0.682 | 0.374 | n/a | 0.136 |

Pack-primes against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.196 | 0.375 | n/a | 0.566 |
| 32 | 0.199 | 0.373 | n/a | 0.609 |
| 64 | 0.218 | 0.385 | n/a | 0.637 |

Pack-primes against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.003 | 0.004 | n/a | 0.015 |
| 32 | 0.004 | 0.004 | n/a | 0.006 |
| 64 | 0.006 | 0.006 | n/a | 0.007 |

Pack-primes against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.338 | 0.739 | n/a | 0.185 |
| 32 | 2.099 | 0.767 | n/a | 0.174 |
| 64 | 1.889 | 0.716 | n/a | 0.165 |

Pack-coprime against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.014 | 0.097 | n/a | 0.526 |
| 32 | 0.007 | 0.050 | n/a | 0.547 |
| 64 | 0.004 | 0.026 | n/a | 0.449 |

Pack-coprime against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.100 | 0.101 | n/a | 0.105 |
| 32 | 0.060 | 0.059 | n/a | 0.060 |
| 64 | 0.034 | 0.033 | n/a | 0.034 |

Pack-coprime against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.675 | 0.859 | n/a | 0.195 |
| 32 | 2.331 | 0.854 | n/a | 0.182 |
| 64 | 2.025 | 0.774 | n/a | 0.172 |

Pack-primes against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.413 | 0.680 | n/a | 0.901 |
| 32 | 0.402 | 0.664 | n/a | 0.973 |
| 64 | 0.426 | 0.658 | n/a | 0.963 |

Pack-primes against Hash-V-pack, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.004 | 0.005 | n/a | 0.017 |
| 32 | 0.005 | 0.004 | n/a | 0.007 |
| 64 | 0.007 | 0.007 | n/a | 0.008 |

Pack-primes against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.915 | 2.511 | n/a | 1.316 |
| 32 | 2.823 | 1.975 | n/a | 1.258 |
| 64 | 2.771 | 1.914 | n/a | 1.215 |

Pack-coprime against Hash-V-pack, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.029 | 0.177 | n/a | 0.836 |
| 32 | 0.014 | 0.089 | n/a | 0.874 |
| 64 | 0.008 | 0.044 | n/a | 0.679 |

Pack-coprime against Hash-V-pack, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.115 | 0.117 | n/a | 0.123 |
| 32 | 0.068 | 0.067 | n/a | 0.068 |
| 64 | 0.038 | 0.037 | n/a | 0.039 |

Pack-coprime against Hash-V-pack, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 3.334 | 2.916 | n/a | 1.389 |
| 32 | 3.136 | 2.200 | n/a | 1.315 |
| 64 | 2.970 | 2.068 | n/a | 1.269 |

Pack-coprime against Pack-primes, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.071 | 0.260 | n/a | 0.928 |
| 32 | 0.034 | 0.133 | n/a | 0.898 |
| 64 | 0.019 | 0.066 | n/a | 0.705 |

Pack-coprime against Pack-primes, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 29.361 | 23.776 | n/a | 7.064 |
| 32 | 14.683 | 15.898 | n/a | 9.870 |
| 64 | 5.315 | 5.263 | n/a | 4.776 |

Pack-coprime against Pack-primes, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.144 | 1.161 | n/a | 1.055 |
| 32 | 1.111 | 1.114 | n/a | 1.046 |
| 64 | 1.072 | 1.081 | n/a | 1.045 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (7 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build (its factoring included), over that input's cells. (f), (f'), (q) and (r) are accounted (v0.3 §16.10).

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | C's stable (build peak) | D's stable (build peak) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 1 | Hash-V-pack | 12.84 (13.36) | 12.92 (13.44) | 18.84 (19.37) | 12.81 (52.81) | 12.84 (52.84) | 12.88 (52.88) | 12.81 (52.81) |
| 1 | Pack-primes | 37.41 (38.14) | 39.07 (39.79) | 43.46 (44.19) | 37.17 (56.89) | 37.88 (57.08) | 37.02 (52.88) | 37.59 (56.89) |
| 1 | Pack-coprime | 42.80 (43.64) | 44.29 (45.13) | 49.17 (50.03) | 42.64 (62.36) | 43.13 (62.36) | 42.36 (52.88) | 43.06 (62.36) |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 2 | Hash-V-pack | 4.71 (5.23) | 6.46 (6.98) | 14.25 (14.77) | 4.62 (46.62) | 4.72 (46.72) | 4.75 (46.75) | 4.75 (46.75) |
| 2 | Pack-primes | 11.83 (12.59) | 18.81 (19.57) | 29.02 (29.76) | 11.65 (46.62) | 12.07 (46.72) | 11.98 (46.75) | 11.62 (46.75) |
| 2 | Pack-coprime | 13.74 (14.62) | 21.08 (21.98) | 32.77 (33.63) | 13.64 (46.62) | 14.00 (46.72) | 13.75 (46.75) | 13.55 (46.75) |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 4 | Hash-V-pack | 2.25 (2.78) | 4.45 (4.98) | 12.71 (13.25) | 2.25 (44.25) | 2.25 (44.25) | 2.25 (44.25) | 2.25 (44.25) |
| 4 | Pack-primes | 2.96 (3.73) | 11.31 (12.07) | 22.82 (23.55) | 2.99 (44.25) | 2.92 (44.25) | 2.95 (44.25) | 2.99 (44.25) |
| 4 | Pack-coprime | 3.13 (4.03) | 12.58 (13.48) | 25.94 (26.83) | 3.14 (44.25) | 3.03 (44.25) | 3.17 (44.25) | 3.17 (44.25) |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 5 | Hash-V-pack | 11.89 (12.02) | 12.02 (12.15) | 17.89 (18.02) | 11.91 (51.91) | 11.90 (51.90) | 11.84 (51.84) | 11.93 (51.93) |
| 5 | Pack-primes | 33.58 (33.76) | 35.38 (35.56) | 39.11 (39.28) | 33.54 (51.91) | 33.70 (51.90) | 33.33 (51.84) | 33.76 (51.93) |
| 5 | Pack-coprime | 37.30 (37.52) | 39.47 (39.69) | 44.10 (44.32) | 37.28 (51.91) | 37.36 (55.05) | 37.01 (51.84) | 37.55 (55.38) |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 6 | Hash-V-pack | 6.21 (6.34) | 7.48 (7.61) | 14.65 (14.78) | 6.22 (46.22) | 6.22 (46.22) | 6.20 (46.20) | 6.21 (46.21) |
| 6 | Pack-primes | 12.26 (12.44) | 18.36 (18.54) | 27.06 (27.24) | 12.20 (46.22) | 12.26 (46.22) | 12.25 (46.20) | 12.34 (46.21) |
| 6 | Pack-coprime | 13.66 (13.88) | 20.55 (20.77) | 30.81 (31.03) | 13.65 (46.22) | 13.67 (46.22) | 13.61 (46.20) | 13.72 (46.21) |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 8 | Hash-V-pack | 2.22 (2.35) | 4.27 (4.40) | 12.34 (12.47) | 2.20 (44.20) | 2.22 (44.22) | 2.22 (44.22) | 2.23 (44.23) |
| 8 | Pack-primes | 2.79 (2.97) | 10.77 (10.95) | 21.41 (21.59) | 2.73 (44.20) | 2.82 (44.22) | 2.78 (44.22) | 2.83 (44.23) |
| 8 | Pack-coprime | 2.92 (3.13) | 11.88 (12.10) | 24.38 (24.59) | 2.86 (44.20) | 2.95 (44.22) | 2.91 (44.22) | 2.95 (44.23) |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 9 | Hash-V-pack | 10.91 (10.94) | 11.16 (11.19) | 17.16 (17.20) | 10.89 (50.89) | 10.91 (50.91) | 10.89 (50.89) | 10.95 (50.95) |
| 9 | Pack-primes | 30.23 (30.27) | 32.42 (32.46) | 36.53 (36.58) | 29.98 (50.89) | 30.09 (50.91) | 30.29 (50.89) | 30.54 (50.95) |
| 9 | Pack-coprime | 32.40 (32.45) | 35.40 (35.46) | 40.88 (40.94) | 32.18 (50.89) | 32.29 (50.91) | 32.37 (50.89) | 32.74 (50.95) |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 10 | Hash-V-pack | 5.98 (6.02) | 7.22 (7.25) | 14.35 (14.39) | 5.98 (45.98) | 5.98 (45.98) | 5.98 (45.98) | 6.00 (46.00) |
| 10 | Pack-primes | 11.45 (11.50) | 17.36 (17.41) | 25.82 (25.87) | 11.39 (45.98) | 11.47 (45.98) | 11.52 (45.98) | 11.43 (46.00) |
| 10 | Pack-coprime | 12.38 (12.43) | 19.32 (19.37) | 29.44 (29.50) | 12.33 (45.98) | 12.40 (45.98) | 12.38 (45.98) | 12.40 (46.00) |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) |
| 12 | Hash-V-pack | 2.17 (2.21) | 4.17 (4.21) | 12.18 (12.21) | 2.17 (44.17) | 2.18 (44.18) | 2.17 (44.17) | 2.18 (44.18) |
| 12 | Pack-primes | 2.64 (2.69) | 10.36 (10.40) | 20.76 (20.81) | 2.66 (44.17) | 2.66 (44.18) | 2.60 (44.17) | 2.64 (44.18) |
| 12 | Pack-coprime | 2.76 (2.82) | 11.70 (11.76) | 23.96 (24.02) | 2.78 (44.17) | 2.77 (44.18) | 2.73 (44.17) | 2.75 (44.18) |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B | C | D |
| --- | --- | --- | --- | --- | --- |
| 1 | TC | 7329 | 6964 | 7087 | 6988 |
| 1 | Hash-V-pack | 15123 | 14744 | 15067 | 14697 |
| 1 | Pack-primes | 40768 | 36426 | 35144 | 35747 |
| 1 | Pack-coprime | 540256 | 494081 | 508258 | 502625 |
| 1 | Hash-V-pack-null | 15107 | 14741 | 15032 | 14771 |
| 2 | TC | 6918 | 6984 | 7017 | 7030 |
| 2 | Hash-V-pack | 12538 | 12777 | 12627 | 12903 |
| 2 | Pack-primes | 21210 | 18107 | 18775 | 18623 |
| 2 | Pack-coprime | 78430 | 70632 | 69934 | 72807 |
| 2 | Hash-V-pack-null | 12545 | 12807 | 12695 | 12948 |
| 4 | TC | 5319 | 5752 | 5447 | 5899 |
| 4 | Hash-V-pack | 8582 | 9182 | 8847 | 9420 |
| 4 | Pack-primes | 10634 | 10034 | 9365 | 10034 |
| 4 | Pack-coprime | 10692 | 10825 | 10586 | 11057 |
| 4 | Hash-V-pack-null | 8568 | 9167 | 8876 | 9410 |
| 5 | TC | 29303 | 29263 | 29342 | 29133 |
| 5 | Hash-V-pack | 59060 | 59142 | 59404 | 58275 |
| 5 | Pack-primes | 152232 | 146940 | 142010 | 144182 |
| 5 | Pack-coprime | 4346133 | 4136702 | 4059012 | 4518403 |
| 5 | Hash-V-pack-null | 58699 | 59084 | 59504 | 58090 |
| 6 | TC | 28640 | 28477 | 28656 | 28503 |
| 6 | Hash-V-pack | 52014 | 50409 | 50821 | 50869 |
| 6 | Pack-primes | 84940 | 73986 | 76713 | 72190 |
| 6 | Pack-coprime | 619410 | 577527 | 561845 | 564113 |
| 6 | Hash-V-pack-null | 51811 | 50220 | 50352 | 50718 |
| 8 | TC | 25934 | 26425 | 24922 | 24531 |
| 8 | Hash-V-pack | 40898 | 41316 | 40247 | 39246 |
| 8 | Pack-primes | 58377 | 41440 | 41943 | 38902 |
| 8 | Pack-coprime | 48158 | 49081 | 45446 | 45283 |
| 8 | Hash-V-pack-null | 41060 | 41327 | 40279 | 39126 |
| 9 | TC | 117360 | 116967 | 117162 | 117404 |
| 9 | Hash-V-pack | 229132 | 228272 | 229348 | 228712 |
| 9 | Pack-primes | 572956 | 531764 | 529242 | 535298 |
| 9 | Pack-coprime | 28349056 | 27731676 | 25440234 | 27793419 |
| 9 | Hash-V-pack-null | 230155 | 229396 | 230004 | 231400 |
| 10 | TC | 117834 | 116969 | 116930 | 117105 |
| 10 | Hash-V-pack | 199152 | 200496 | 199232 | 201316 |
| 10 | Pack-primes | 327273 | 304542 | 274404 | 273716 |
| 10 | Pack-coprime | 4627814 | 4344942 | 3983855 | 4838257 |
| 10 | Hash-V-pack-null | 199428 | 199996 | 199250 | 200894 |
| 12 | TC | 108668 | 115972 | 111232 | 109106 |
| 12 | Hash-V-pack | 160598 | 169787 | 169394 | 166781 |
| 12 | Pack-primes | 213974 | 173026 | 171377 | 165342 |
| 12 | Pack-coprime | 275640 | 232084 | 230516 | 249807 |
| 12 | Hash-V-pack-null | 167312 | 173690 | 168654 | 167644 |

## Stores, per job and matrix (rule-m3 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. T is the build table's. The factored rows' stores, lists and packings are Hash-V-pack's (checked).

| job | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | cells bytes | list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.13 | 6 | 0 (0) | 1024 | 2256 |
| 1 | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.12 | 6 | 0 (0) | 1024 | 2264 |
| 1 | C | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.11 | 5 | 0 (0) | 1024 | 2272 |
| 1 | D | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.09 | 5 | 0 (0) | 1024 | 2256 |
| 1 | P | 314 | 1024 | 30.7% | 64 | 16 | 198 / 314 | 1.16 | 8 | 1 (1024) | 1024 | 2512 |
| 1 | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.10 | 5 | 1 (1024) | 1024 | 2424 |
| 1 | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.11 | 6 | 1 (1024) | 1024 | 2368 |
| 2 | A | 84 | 1024 | 8.2% | 64 | 8 | 428 / 84 | 1.01 | 2 | 0 (0) | 512 | 672 |
| 2 | B | 87 | 1024 | 8.5% | 64 | 8 | 425 / 87 | 1.01 | 2 | 0 (0) | 512 | 696 |
| 2 | C | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.04 | 2 | 0 (0) | 512 | 704 |
| 2 | D | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.08 | 3 | 0 (0) | 512 | 704 |
| 2 | P | 313 | 1024 | 30.6% | 64 | 16 | 199 / 313 | 1.13 | 5 | 1 (1024) | 1024 | 2504 |
| 2 | Q | 309 | 1024 | 30.2% | 64 | 16 | 203 / 309 | 1.13 | 4 | 1 (1024) | 1024 | 2472 |
| 2 | R | 302 | 1024 | 29.5% | 64 | 16 | 210 / 302 | 1.14 | 8 | 1 (1024) | 1024 | 2416 |
| 4 | A | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | C | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | D | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 512 | 64 |
| 4 | P | 305 | 1024 | 29.8% | 64 | 16 | 207 / 305 | 1.09 | 5 | 1 (1024) | 1024 | 2440 |
| 4 | Q | 295 | 1024 | 28.8% | 64 | 16 | 217 / 295 | 1.15 | 5 | 1 (1024) | 1024 | 2360 |
| 4 | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.12 | 8 | 1 (1024) | 1024 | 2368 |
| 5 | A | 1013 | 4096 | 24.7% | 64 | 16 | 1035 / 1013 | 1.06 | 4 | 0 (0) | 4096 | 8104 |
| 5 | B | 1011 | 4096 | 24.7% | 64 | 16 | 1037 / 1011 | 1.07 | 5 | 0 (0) | 4096 | 8088 |
| 5 | C | 1003 | 4096 | 24.5% | 64 | 16 | 1045 / 1003 | 1.09 | 8 | 0 (0) | 4096 | 8024 |
| 5 | D | 1015 | 4096 | 24.8% | 64 | 16 | 1033 / 1015 | 1.05 | 4 | 0 (0) | 4096 | 8120 |
| 5 | P | 1089 | 4096 | 26.6% | 64 | 16 | 959 / 1089 | 1.10 | 5 | 1 (4096) | 4096 | 8712 |
| 5 | Q | 1100 | 4096 | 26.9% | 64 | 16 | 948 / 1100 | 1.10 | 6 | 1 (4096) | 4096 | 8800 |
| 5 | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.10 | 5 | 1 (4096) | 4096 | 8736 |
| 6 | A | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.06 | 2 | 0 (0) | 4096 | 2272 |
| 6 | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.01 | 2 | 0 (0) | 4096 | 2272 |
| 6 | C | 281 | 4096 | 6.9% | 64 | 16 | 1767 / 281 | 1.01 | 2 | 0 (0) | 4096 | 2248 |
| 6 | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 4096 | 2264 |
| 6 | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.10 | 8 | 1 (4096) | 4096 | 8776 |
| 6 | Q | 1096 | 4096 | 26.8% | 64 | 16 | 952 / 1096 | 1.10 | 6 | 1 (4096) | 4096 | 8768 |
| 6 | R | 1095 | 4096 | 26.7% | 64 | 16 | 953 / 1095 | 1.09 | 7 | 1 (4096) | 4096 | 8760 |
| 8 | A | 26 | 4096 | 0.6% | 64 | 8 | 2022 / 26 | 1.00 | 1 | 0 (0) | 2048 | 208 |
| 8 | B | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 2048 | 224 |
| 8 | C | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 2048 | 224 |
| 8 | D | 29 | 4096 | 0.7% | 64 | 8 | 2019 / 29 | 1.00 | 1 | 0 (0) | 2048 | 232 |
| 8 | P | 1093 | 4096 | 26.7% | 64 | 16 | 955 / 1093 | 1.11 | 8 | 1 (4096) | 4096 | 8744 |
| 8 | Q | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.09 | 6 | 1 (4096) | 4096 | 8696 |
| 8 | R | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.14 | 5 | 1 (4096) | 4096 | 8696 |
| 9 | A | 3526 | 16384 | 21.5% | 64 | 16 | 4666 / 3526 | 1.02 | 5 | 0 (0) | 16384 | 28208 |
| 9 | B | 3536 | 16384 | 21.6% | 64 | 16 | 4656 / 3536 | 1.02 | 5 | 0 (0) | 16384 | 28288 |
| 9 | C | 3527 | 16384 | 21.5% | 64 | 16 | 4665 / 3527 | 1.03 | 5 | 0 (0) | 16384 | 28216 |
| 9 | D | 3558 | 16384 | 21.7% | 64 | 16 | 4634 / 3558 | 1.03 | 5 | 0 (0) | 16384 | 28464 |
| 9 | P | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 9 | 1 (16384) | 16384 | 33504 |
| 9 | Q | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.08 | 7 | 1 (16384) | 16384 | 33480 |
| 9 | R | 4182 | 16384 | 25.5% | 64 | 16 | 4010 / 4182 | 1.08 | 8 | 1 (16384) | 16384 | 33456 |
| 10 | A | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 16384 | 8096 |
| 10 | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 16384 | 8120 |
| 10 | C | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 16384 | 8096 |
| 10 | D | 1023 | 16384 | 6.2% | 64 | 16 | 7169 / 1023 | 1.01 | 2 | 0 (0) | 16384 | 8184 |
| 10 | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 8 | 1 (16384) | 16384 | 33520 |
| 10 | Q | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 16384 | 33512 |
| 10 | R | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.09 | 7 | 1 (16384) | 16384 | 33472 |
| 12 | A | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 8192 | 712 |
| 12 | B | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 8192 | 720 |
| 12 | C | 86 | 16384 | 0.5% | 64 | 8 | 8106 / 86 | 1.00 | 1 | 0 (0) | 8192 | 688 |
| 12 | D | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 8192 | 720 |
| 12 | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 9 | 1 (16384) | 16384 | 33520 |
| 12 | Q | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 11 | 1 (16384) | 16384 | 33488 |
| 12 | R | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.09 | 7 | 1 (16384) | 16384 | 33504 |

### Hash-V-pack's packings (from the counting runs)

| job | matrix | W | w_h build | w_h packed | list bytes | cells rewritten | map bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | A | 282 | 16 | 16 | 2256 | 512 | 2048 |
| 1 | B | 283 | 16 | 16 | 2264 | 512 | 2048 |
| 1 | C | 284 | 16 | 16 | 2272 | 512 | 2048 |
| 1 | D | 282 | 16 | 16 | 2256 | 512 | 2048 |
| 1 | P | 314 | 16 | 16 | 2512 | 512 | 2048 |
| 1 | Q | 303 | 16 | 16 | 2424 | 512 | 2048 |
| 1 | R | 296 | 16 | 16 | 2368 | 512 | 2048 |
| 2 | A | 84 | 16 | 8 | 672 | 512 | 2048 |
| 2 | B | 87 | 16 | 8 | 696 | 512 | 2048 |
| 2 | C | 88 | 16 | 8 | 704 | 512 | 2048 |
| 2 | D | 88 | 16 | 8 | 704 | 512 | 2048 |
| 2 | P | 313 | 16 | 16 | 2504 | 512 | 2048 |
| 2 | Q | 309 | 16 | 16 | 2472 | 512 | 2048 |
| 2 | R | 302 | 16 | 16 | 2416 | 512 | 2048 |
| 4 | A | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | B | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | C | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | D | 8 | 16 | 8 | 64 | 512 | 2048 |
| 4 | P | 305 | 16 | 16 | 2440 | 512 | 2048 |
| 4 | Q | 295 | 16 | 16 | 2360 | 512 | 2048 |
| 4 | R | 296 | 16 | 16 | 2368 | 512 | 2048 |
| 5 | A | 1013 | 16 | 16 | 8104 | 2048 | 8192 |
| 5 | B | 1011 | 16 | 16 | 8088 | 2048 | 8192 |
| 5 | C | 1003 | 16 | 16 | 8024 | 2048 | 8192 |
| 5 | D | 1015 | 16 | 16 | 8120 | 2048 | 8192 |
| 5 | P | 1089 | 16 | 16 | 8712 | 2048 | 8192 |
| 5 | Q | 1100 | 16 | 16 | 8800 | 2048 | 8192 |
| 5 | R | 1092 | 16 | 16 | 8736 | 2048 | 8192 |
| 6 | A | 284 | 16 | 16 | 2272 | 2048 | 8192 |
| 6 | B | 284 | 16 | 16 | 2272 | 2048 | 8192 |
| 6 | C | 281 | 16 | 16 | 2248 | 2048 | 8192 |
| 6 | D | 283 | 16 | 16 | 2264 | 2048 | 8192 |
| 6 | P | 1097 | 16 | 16 | 8776 | 2048 | 8192 |
| 6 | Q | 1096 | 16 | 16 | 8768 | 2048 | 8192 |
| 6 | R | 1095 | 16 | 16 | 8760 | 2048 | 8192 |
| 8 | A | 26 | 16 | 8 | 208 | 2048 | 8192 |
| 8 | B | 28 | 16 | 8 | 224 | 2048 | 8192 |
| 8 | C | 28 | 16 | 8 | 224 | 2048 | 8192 |
| 8 | D | 29 | 16 | 8 | 232 | 2048 | 8192 |
| 8 | P | 1093 | 16 | 16 | 8744 | 2048 | 8192 |
| 8 | Q | 1087 | 16 | 16 | 8696 | 2048 | 8192 |
| 8 | R | 1087 | 16 | 16 | 8696 | 2048 | 8192 |
| 9 | A | 3526 | 16 | 16 | 28208 | 8192 | 32768 |
| 9 | B | 3536 | 16 | 16 | 28288 | 8192 | 32768 |
| 9 | C | 3527 | 16 | 16 | 28216 | 8192 | 32768 |
| 9 | D | 3558 | 16 | 16 | 28464 | 8192 | 32768 |
| 9 | P | 4188 | 16 | 16 | 33504 | 8192 | 32768 |
| 9 | Q | 4185 | 16 | 16 | 33480 | 8192 | 32768 |
| 9 | R | 4182 | 16 | 16 | 33456 | 8192 | 32768 |
| 10 | A | 1012 | 16 | 16 | 8096 | 8192 | 32768 |
| 10 | B | 1015 | 16 | 16 | 8120 | 8192 | 32768 |
| 10 | C | 1012 | 16 | 16 | 8096 | 8192 | 32768 |
| 10 | D | 1023 | 16 | 16 | 8184 | 8192 | 32768 |
| 10 | P | 4190 | 16 | 16 | 33520 | 8192 | 32768 |
| 10 | Q | 4189 | 16 | 16 | 33512 | 8192 | 32768 |
| 10 | R | 4184 | 16 | 16 | 33472 | 8192 | 32768 |
| 12 | A | 89 | 16 | 8 | 712 | 8192 | 32768 |
| 12 | B | 90 | 16 | 8 | 720 | 8192 | 32768 |
| 12 | C | 86 | 16 | 8 | 688 | 8192 | 32768 |
| 12 | D | 90 | 16 | 8 | 720 | 8192 | 32768 |
| 12 | P | 4190 | 16 | 16 | 33520 | 8192 | 32768 |
| 12 | Q | 4186 | 16 | 16 | 33488 | 8192 | 32768 |
| 12 | R | 4188 | 16 | 16 | 33504 | 8192 | 32768 |

### The factored rows' factorings (from the counting runs, each at its factoring)

Pack-primes: trial-division candidates, divisions with remainder and exact divisions. Pack-coprime: pass 1's scan gcds, divisions and splits (with the splits' gcds), pass 2's divisibility tests. E: (f)'s entries; |q|: the base's pieces.

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes | candidates | divisions | exact | pass-1 gcds | pass-1 divisions | splits (gcds) | pass-2 tests |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | A | 282 | 567 | 6235 | 0 | 0 | 5873 | 6301 | 428 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | B | 283 | 586 | 6410 | 0 | 0 | 5336 | 5787 | 451 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | C | 284 | 560 | 6180 | 0 | 0 | 5288 | 5714 | 426 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | D | 282 | 579 | 6343 | 0 | 0 | 5375 | 5815 | 440 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | P | 314 | 827 | 8703 | 0 | 0 | 30343825 | 30344778 | 953 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | Q | 303 | 825 | 8641 | 0 | 0 | 25853769 | 25854663 | 894 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-primes | R | 296 | 791 | 8307 | 0 | 0 | 44717900 | 44718713 | 813 | 0 | 0 | 0 (0) | 0 |
| 1 | Pack-coprime | A | 282 | 567 | 6235 | 175 | 1400 | 0 | 0 | 0 | 20253 | 907 | 8 (10) | 20941 |
| 1 | Pack-coprime | B | 283 | 585 | 6401 | 169 | 1352 | 0 | 0 | 0 | 18545 | 963 | 11 (12) | 19398 |
| 1 | Pack-coprime | C | 284 | 560 | 6180 | 171 | 1368 | 0 | 0 | 0 | 19673 | 908 | 11 (9) | 20345 |
| 1 | Pack-coprime | D | 282 | 579 | 6343 | 175 | 1400 | 0 | 0 | 0 | 19048 | 933 | 14 (17) | 19889 |
| 1 | Pack-coprime | P | 314 | 666 | 7254 | 303 | 2424 | 0 | 0 | 0 | 40684 | 1113 | 47 (50) | 44766 |
| 1 | Pack-coprime | Q | 303 | 689 | 7417 | 312 | 2496 | 0 | 0 | 0 | 41660 | 1068 | 57 (59) | 46619 |
| 1 | Pack-coprime | R | 296 | 645 | 6993 | 310 | 2480 | 0 | 0 | 0 | 41485 | 949 | 55 (55) | 45858 |
| 2 | Pack-primes | A | 84 | 162 | 1798 | 0 | 0 | 1547 | 1679 | 132 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | B | 87 | 170 | 1882 | 0 | 0 | 1123 | 1275 | 152 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | C | 88 | 166 | 1850 | 0 | 0 | 1435 | 1588 | 153 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | D | 88 | 156 | 1760 | 0 | 0 | 1437 | 1561 | 124 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | P | 313 | 858 | 8978 | 0 | 0 | 20127177 | 20128143 | 966 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | Q | 309 | 792 | 8368 | 0 | 0 | 24605716 | 24606565 | 849 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-primes | R | 302 | 813 | 8529 | 0 | 0 | 35286046 | 35286913 | 867 | 0 | 0 | 0 (0) | 0 |
| 2 | Pack-coprime | A | 84 | 161 | 1789 | 65 | 520 | 0 | 0 | 0 | 2305 | 236 | 7 (6) | 2524 |
| 2 | Pack-coprime | B | 87 | 169 | 1873 | 63 | 504 | 0 | 0 | 0 | 2112 | 262 | 12 (12) | 2485 |
| 2 | Pack-coprime | C | 88 | 164 | 1832 | 59 | 472 | 0 | 0 | 0 | 2065 | 276 | 6 (6) | 2336 |
| 2 | Pack-coprime | D | 88 | 153 | 1733 | 65 | 520 | 0 | 0 | 0 | 2172 | 219 | 8 (10) | 2484 |
| 2 | Pack-coprime | P | 313 | 715 | 7691 | 305 | 2440 | 0 | 0 | 0 | 41951 | 1188 | 49 (55) | 46017 |
| 2 | Pack-coprime | Q | 309 | 653 | 7117 | 305 | 2440 | 0 | 0 | 0 | 40783 | 1011 | 50 (52) | 44506 |
| 2 | Pack-coprime | R | 302 | 650 | 7062 | 302 | 2416 | 0 | 0 | 0 | 40526 | 1011 | 46 (49) | 43616 |
| 4 | Pack-primes | A | 8 | 17 | 189 | 0 | 0 | 103 | 121 | 18 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | B | 8 | 15 | 171 | 0 | 0 | 144 | 162 | 18 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | C | 8 | 16 | 180 | 0 | 0 | 36 | 51 | 15 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | D | 8 | 17 | 189 | 0 | 0 | 79 | 97 | 18 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | P | 305 | 758 | 8046 | 0 | 0 | 5643717 | 5644519 | 802 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | Q | 295 | 727 | 7727 | 0 | 0 | 7313805 | 7314504 | 699 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-primes | R | 296 | 762 | 8046 | 0 | 0 | 8442961 | 8443724 | 763 | 0 | 0 | 0 (0) | 0 |
| 4 | Pack-coprime | A | 8 | 15 | 171 | 7 | 56 | 0 | 0 | 0 | 37 | 21 | 2 (4) | 60 |
| 4 | Pack-coprime | B | 8 | 12 | 144 | 7 | 56 | 0 | 0 | 0 | 30 | 14 | 3 (3) | 54 |
| 4 | Pack-coprime | C | 8 | 15 | 171 | 8 | 64 | 0 | 0 | 0 | 36 | 20 | 2 (2) | 58 |
| 4 | Pack-coprime | D | 8 | 15 | 171 | 8 | 64 | 0 | 0 | 0 | 32 | 17 | 2 (2) | 54 |
| 4 | Pack-coprime | P | 305 | 640 | 6984 | 304 | 2432 | 0 | 0 | 0 | 40791 | 977 | 50 (51) | 44896 |
| 4 | Pack-coprime | Q | 295 | 626 | 6818 | 307 | 2456 | 0 | 0 | 0 | 40951 | 868 | 53 (55) | 45425 |
| 4 | Pack-coprime | R | 296 | 646 | 7002 | 314 | 2512 | 0 | 0 | 0 | 41171 | 924 | 59 (62) | 46423 |
| 5 | Pack-primes | A | 1013 | 2010 | 22146 | 0 | 0 | 22034 | 23469 | 1435 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | B | 1011 | 2031 | 22327 | 0 | 0 | 21721 | 23158 | 1437 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | C | 1003 | 1999 | 22007 | 0 | 0 | 21870 | 23298 | 1428 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | D | 1015 | 2032 | 22352 | 0 | 0 | 21887 | 23335 | 1448 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | P | 1089 | 2821 | 29749 | 0 | 0 | 196367448 | 196369679 | 2231 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | Q | 1100 | 2933 | 30801 | 0 | 0 | 209214372 | 209216781 | 2409 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-primes | R | 1092 | 2931 | 30751 | 0 | 0 | 292740536 | 292742930 | 2394 | 0 | 0 | 0 (0) | 0 |
| 5 | Pack-coprime | A | 1013 | 2010 | 22146 | 479 | 3832 | 0 | 0 | 0 | 167694 | 3456 | 17 (16) | 169903 |
| 5 | Pack-coprime | B | 1011 | 2031 | 22327 | 468 | 3744 | 0 | 0 | 0 | 159350 | 3508 | 15 (18) | 161364 |
| 5 | Pack-coprime | C | 1003 | 1999 | 22007 | 471 | 3768 | 0 | 0 | 0 | 155970 | 3452 | 11 (12) | 157935 |
| 5 | Pack-coprime | D | 1015 | 2032 | 22352 | 486 | 3888 | 0 | 0 | 0 | 174973 | 3485 | 15 (16) | 177305 |
| 5 | Pack-coprime | P | 1089 | 2382 | 25798 | 1168 | 9344 | 0 | 0 | 0 | 607435 | 2876 | 145 (144) | 646160 |
| 5 | Pack-coprime | Q | 1100 | 2503 | 26931 | 1190 | 9520 | 0 | 0 | 0 | 622657 | 3144 | 166 (169) | 669884 |
| 5 | Pack-coprime | R | 1092 | 2509 | 26953 | 1190 | 9520 | 0 | 0 | 0 | 626081 | 3140 | 167 (168) | 668046 |
| 6 | Pack-primes | A | 284 | 554 | 6126 | 0 | 0 | 5918 | 6334 | 416 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | B | 284 | 561 | 6189 | 0 | 0 | 5779 | 6202 | 423 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | C | 281 | 564 | 6204 | 0 | 0 | 5695 | 6118 | 423 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | D | 283 | 571 | 6275 | 0 | 0 | 5498 | 5945 | 447 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | P | 1097 | 2915 | 30627 | 0 | 0 | 210900822 | 210903206 | 2384 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | Q | 1096 | 2926 | 30722 | 0 | 0 | 226676140 | 226678540 | 2400 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-primes | R | 1095 | 2948 | 30916 | 0 | 0 | 318787243 | 318789654 | 2411 | 0 | 0 | 0 (0) | 0 |
| 6 | Pack-coprime | A | 284 | 554 | 6126 | 186 | 1488 | 0 | 0 | 0 | 22007 | 862 | 14 (13) | 22815 |
| 6 | Pack-coprime | B | 284 | 561 | 6189 | 180 | 1440 | 0 | 0 | 0 | 20682 | 892 | 12 (13) | 21448 |
| 6 | Pack-coprime | C | 281 | 564 | 6204 | 174 | 1392 | 0 | 0 | 0 | 20054 | 902 | 15 (15) | 20895 |
| 6 | Pack-coprime | D | 283 | 571 | 6275 | 177 | 1416 | 0 | 0 | 0 | 20219 | 924 | 13 (18) | 20981 |
| 6 | Pack-coprime | P | 1097 | 2497 | 26865 | 1184 | 9472 | 0 | 0 | 0 | 621853 | 3133 | 163 (171) | 667081 |
| 6 | Pack-coprime | Q | 1096 | 2525 | 27113 | 1190 | 9520 | 0 | 0 | 0 | 622195 | 3179 | 167 (170) | 660596 |
| 6 | Pack-coprime | R | 1095 | 2498 | 26866 | 1193 | 9544 | 0 | 0 | 0 | 617777 | 3112 | 171 (175) | 668182 |
| 8 | Pack-primes | A | 26 | 48 | 540 | 0 | 0 | 370 | 415 | 45 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | B | 28 | 55 | 611 | 0 | 0 | 358 | 412 | 54 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | C | 28 | 51 | 575 | 0 | 0 | 312 | 360 | 48 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | D | 29 | 55 | 615 | 0 | 0 | 332 | 381 | 49 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | P | 1093 | 2946 | 30890 | 0 | 0 | 175142790 | 175145220 | 2430 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | Q | 1087 | 2768 | 29264 | 0 | 0 | 75993074 | 75995272 | 2198 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-primes | R | 1087 | 2952 | 30920 | 0 | 0 | 194262609 | 194265002 | 2393 | 0 | 0 | 0 (0) | 0 |
| 8 | Pack-coprime | A | 26 | 44 | 504 | 21 | 168 | 0 | 0 | 0 | 223 | 61 | 5 (5) | 306 |
| 8 | Pack-coprime | B | 28 | 53 | 593 | 19 | 152 | 0 | 0 | 0 | 267 | 87 | 4 (7) | 346 |
| 8 | Pack-coprime | C | 28 | 48 | 548 | 20 | 160 | 0 | 0 | 0 | 215 | 75 | 5 (4) | 295 |
| 8 | Pack-coprime | D | 29 | 51 | 579 | 20 | 160 | 0 | 0 | 0 | 224 | 79 | 4 (5) | 306 |
| 8 | Pack-coprime | P | 1093 | 2486 | 26750 | 1168 | 9344 | 0 | 0 | 0 | 607508 | 3153 | 147 (151) | 645048 |
| 8 | Pack-coprime | Q | 1087 | 2411 | 26051 | 1172 | 9376 | 0 | 0 | 0 | 608801 | 2945 | 153 (156) | 651952 |
| 8 | Pack-coprime | R | 1087 | 2487 | 26735 | 1172 | 9376 | 0 | 0 | 0 | 615969 | 3108 | 152 (156) | 655973 |
| 9 | Pack-primes | A | 3526 | 7123 | 78215 | 0 | 0 | 79916 | 84910 | 4994 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | B | 3536 | 7158 | 78570 | 0 | 0 | 79548 | 84695 | 5147 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | C | 3527 | 7262 | 79470 | 0 | 0 | 76639 | 81867 | 5228 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | D | 3558 | 7335 | 80251 | 0 | 0 | 79323 | 84600 | 5277 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | P | 4188 | 11162 | 117214 | 0 | 0 | 1135605404 | 1135613210 | 7806 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | Q | 4185 | 11274 | 118210 | 0 | 0 | 1133215274 | 1133223219 | 7945 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-primes | R | 4182 | 11342 | 118810 | 0 | 0 | 1395236036 | 1395244030 | 7994 | 0 | 0 | 0 (0) | 0 |
| 9 | Pack-coprime | A | 3526 | 7123 | 78215 | 1127 | 9016 | 0 | 0 | 0 | 1069619 | 13296 | 11 (11) | 1075963 |
| 9 | Pack-coprime | B | 3536 | 7158 | 78570 | 1128 | 9024 | 0 | 0 | 0 | 1057358 | 13474 | 14 (12) | 1063657 |
| 9 | Pack-coprime | C | 3527 | 7262 | 79470 | 1065 | 8520 | 0 | 0 | 0 | 968899 | 13789 | 14 (13) | 975379 |
| 9 | Pack-coprime | D | 3558 | 7335 | 80251 | 1127 | 9016 | 0 | 0 | 0 | 1057197 | 13788 | 16 (19) | 1063534 |
| 9 | Pack-coprime | P | 4188 | 10023 | 106963 | 4547 | 36376 | 0 | 0 | 0 | 9406848 | 11724 | 489 (489) | 9911366 |
| 9 | Pack-coprime | Q | 4185 | 10154 | 108130 | 4561 | 36488 | 0 | 0 | 0 | 9427168 | 11980 | 513 (517) | 9922833 |
| 9 | Pack-coprime | R | 4182 | 10120 | 107812 | 4575 | 36600 | 0 | 0 | 0 | 9460037 | 11860 | 506 (511) | 9980599 |
| 10 | Pack-primes | A | 1012 | 2015 | 22187 | 0 | 0 | 22625 | 24056 | 1431 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | B | 1015 | 2048 | 22496 | 0 | 0 | 21844 | 23328 | 1484 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | C | 1012 | 2071 | 22691 | 0 | 0 | 20442 | 21987 | 1545 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | D | 1023 | 2016 | 22240 | 0 | 0 | 22788 | 24191 | 1403 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | P | 4190 | 11286 | 118338 | 0 | 0 | 1139827101 | 1139835051 | 7950 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | Q | 4189 | 11178 | 117362 | 0 | 0 | 1072006741 | 1072014587 | 7846 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-primes | R | 4184 | 11265 | 118125 | 0 | 0 | 1456525643 | 1456533559 | 7916 | 0 | 0 | 0 (0) | 0 |
| 10 | Pack-coprime | A | 1012 | 2015 | 22187 | 478 | 3824 | 0 | 0 | 0 | 167316 | 3465 | 15 (13) | 169550 |
| 10 | Pack-coprime | B | 1015 | 2048 | 22496 | 473 | 3784 | 0 | 0 | 0 | 160268 | 3560 | 11 (15) | 162309 |
| 10 | Pack-coprime | C | 1012 | 2071 | 22691 | 441 | 3528 | 0 | 0 | 0 | 147709 | 3709 | 14 (13) | 149727 |
| 10 | Pack-coprime | D | 1023 | 2016 | 22240 | 499 | 3992 | 0 | 0 | 0 | 180542 | 3417 | 12 (13) | 182665 |
| 10 | Pack-coprime | P | 4190 | 10168 | 108276 | 4549 | 36392 | 0 | 0 | 0 | 9446852 | 12027 | 495 (497) | 9929825 |
| 10 | Pack-coprime | Q | 4189 | 10012 | 106868 | 4570 | 36560 | 0 | 0 | 0 | 9463081 | 11673 | 510 (507) | 9986436 |
| 10 | Pack-coprime | R | 4184 | 9995 | 106695 | 4537 | 36296 | 0 | 0 | 0 | 9406489 | 11688 | 482 (482) | 9886730 |
| 12 | Pack-primes | A | 89 | 183 | 2007 | 0 | 0 | 1378 | 1528 | 150 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | B | 90 | 178 | 1966 | 0 | 0 | 1367 | 1529 | 162 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | C | 86 | 159 | 1779 | 0 | 0 | 1668 | 1796 | 128 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | D | 90 | 170 | 1894 | 0 | 0 | 1431 | 1571 | 140 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | P | 4190 | 11285 | 118329 | 0 | 0 | 1001513347 | 1001521283 | 7936 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | Q | 4186 | 11085 | 116513 | 0 | 0 | 980063808 | 980071539 | 7731 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-primes | R | 4188 | 11358 | 118978 | 0 | 0 | 1258743280 | 1258751313 | 8033 | 0 | 0 | 0 (0) | 0 |
| 12 | Pack-coprime | A | 89 | 179 | 1971 | 63 | 504 | 0 | 0 | 0 | 2272 | 279 | 7 (8) | 2538 |
| 12 | Pack-coprime | B | 90 | 174 | 1930 | 65 | 520 | 0 | 0 | 0 | 2332 | 277 | 8 (9) | 2625 |
| 12 | Pack-coprime | C | 86 | 157 | 1761 | 67 | 536 | 0 | 0 | 0 | 2389 | 222 | 10 (14) | 2736 |
| 12 | Pack-coprime | D | 90 | 166 | 1858 | 64 | 512 | 0 | 0 | 0 | 2184 | 255 | 9 (9) | 2487 |
| 12 | Pack-coprime | P | 4190 | 10179 | 108375 | 4558 | 36464 | 0 | 0 | 0 | 9422380 | 12013 | 505 (506) | 9929329 |
| 12 | Pack-coprime | Q | 4186 | 10021 | 106937 | 4535 | 36280 | 0 | 0 | 0 | 9359386 | 11743 | 491 (493) | 9844343 |
| 12 | Pack-coprime | R | 4188 | 10136 | 107980 | 4572 | 36576 | 0 | 0 | 0 | 9497570 | 11931 | 503 (507) | 9987292 |

### The factored rows' (f) and (q) at each matrix's end (after every reconciliation)

| job | row | matrix | W | E | (f) bytes | abs q | (q) bytes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | P | 314 | 827 | 8703 | 0 | 0 |
| 1 | Pack-primes | Q | 303 | 825 | 8641 | 0 | 0 |
| 1 | Pack-primes | A | 282 | 567 | 6235 | 0 | 0 |
| 1 | Pack-primes | B | 283 | 586 | 6410 | 0 | 0 |
| 1 | Pack-primes | C | 284 | 560 | 6180 | 0 | 0 |
| 1 | Pack-primes | D | 282 | 579 | 6343 | 0 | 0 |
| 1 | Pack-primes | R | 296 | 791 | 8307 | 0 | 0 |
| 1 | Pack-coprime | P | 314 | 666 | 7254 | 303 | 2424 |
| 1 | Pack-coprime | Q | 303 | 689 | 7417 | 312 | 2496 |
| 1 | Pack-coprime | A | 282 | 567 | 6235 | 175 | 1400 |
| 1 | Pack-coprime | B | 283 | 585 | 6401 | 169 | 1352 |
| 1 | Pack-coprime | C | 284 | 560 | 6180 | 171 | 1368 |
| 1 | Pack-coprime | D | 282 | 579 | 6343 | 175 | 1400 |
| 1 | Pack-coprime | R | 296 | 645 | 6993 | 310 | 2480 |
| 2 | Pack-primes | P | 313 | 858 | 8978 | 0 | 0 |
| 2 | Pack-primes | Q | 309 | 792 | 8368 | 0 | 0 |
| 2 | Pack-primes | A | 84 | 162 | 1798 | 0 | 0 |
| 2 | Pack-primes | B | 87 | 170 | 1882 | 0 | 0 |
| 2 | Pack-primes | C | 88 | 166 | 1850 | 0 | 0 |
| 2 | Pack-primes | D | 88 | 156 | 1760 | 0 | 0 |
| 2 | Pack-primes | R | 302 | 813 | 8529 | 0 | 0 |
| 2 | Pack-coprime | P | 313 | 715 | 7691 | 305 | 2440 |
| 2 | Pack-coprime | Q | 309 | 653 | 7117 | 305 | 2440 |
| 2 | Pack-coprime | A | 84 | 161 | 1789 | 65 | 520 |
| 2 | Pack-coprime | B | 87 | 169 | 1873 | 63 | 504 |
| 2 | Pack-coprime | C | 88 | 164 | 1832 | 59 | 472 |
| 2 | Pack-coprime | D | 88 | 153 | 1733 | 65 | 520 |
| 2 | Pack-coprime | R | 302 | 650 | 7062 | 302 | 2416 |
| 4 | Pack-primes | P | 305 | 758 | 8046 | 0 | 0 |
| 4 | Pack-primes | Q | 295 | 727 | 7727 | 0 | 0 |
| 4 | Pack-primes | A | 8 | 17 | 189 | 0 | 0 |
| 4 | Pack-primes | B | 8 | 15 | 171 | 0 | 0 |
| 4 | Pack-primes | C | 8 | 16 | 180 | 0 | 0 |
| 4 | Pack-primes | D | 8 | 17 | 189 | 0 | 0 |
| 4 | Pack-primes | R | 296 | 762 | 8046 | 0 | 0 |
| 4 | Pack-coprime | P | 305 | 640 | 6984 | 304 | 2432 |
| 4 | Pack-coprime | Q | 295 | 626 | 6818 | 307 | 2456 |
| 4 | Pack-coprime | A | 8 | 15 | 171 | 7 | 56 |
| 4 | Pack-coprime | B | 8 | 12 | 144 | 7 | 56 |
| 4 | Pack-coprime | C | 8 | 15 | 171 | 8 | 64 |
| 4 | Pack-coprime | D | 8 | 15 | 171 | 8 | 64 |
| 4 | Pack-coprime | R | 296 | 646 | 7002 | 314 | 2512 |
| 5 | Pack-primes | P | 1089 | 2821 | 29749 | 0 | 0 |
| 5 | Pack-primes | Q | 1100 | 2933 | 30801 | 0 | 0 |
| 5 | Pack-primes | A | 1013 | 2010 | 22146 | 0 | 0 |
| 5 | Pack-primes | B | 1011 | 2031 | 22327 | 0 | 0 |
| 5 | Pack-primes | C | 1003 | 1999 | 22007 | 0 | 0 |
| 5 | Pack-primes | D | 1015 | 2032 | 22352 | 0 | 0 |
| 5 | Pack-primes | R | 1092 | 2931 | 30751 | 0 | 0 |
| 5 | Pack-coprime | P | 1089 | 2382 | 25798 | 1168 | 9344 |
| 5 | Pack-coprime | Q | 1100 | 2503 | 26931 | 1190 | 9520 |
| 5 | Pack-coprime | A | 1013 | 2010 | 22146 | 479 | 3832 |
| 5 | Pack-coprime | B | 1011 | 2031 | 22327 | 468 | 3744 |
| 5 | Pack-coprime | C | 1003 | 1999 | 22007 | 471 | 3768 |
| 5 | Pack-coprime | D | 1015 | 2032 | 22352 | 486 | 3888 |
| 5 | Pack-coprime | R | 1092 | 2509 | 26953 | 1190 | 9520 |
| 6 | Pack-primes | P | 1097 | 2915 | 30627 | 0 | 0 |
| 6 | Pack-primes | Q | 1096 | 2926 | 30722 | 0 | 0 |
| 6 | Pack-primes | A | 284 | 554 | 6126 | 0 | 0 |
| 6 | Pack-primes | B | 284 | 561 | 6189 | 0 | 0 |
| 6 | Pack-primes | C | 281 | 564 | 6204 | 0 | 0 |
| 6 | Pack-primes | D | 283 | 571 | 6275 | 0 | 0 |
| 6 | Pack-primes | R | 1095 | 2948 | 30916 | 0 | 0 |
| 6 | Pack-coprime | P | 1097 | 2497 | 26865 | 1184 | 9472 |
| 6 | Pack-coprime | Q | 1096 | 2525 | 27113 | 1190 | 9520 |
| 6 | Pack-coprime | A | 284 | 554 | 6126 | 186 | 1488 |
| 6 | Pack-coprime | B | 284 | 561 | 6189 | 180 | 1440 |
| 6 | Pack-coprime | C | 281 | 564 | 6204 | 174 | 1392 |
| 6 | Pack-coprime | D | 283 | 571 | 6275 | 177 | 1416 |
| 6 | Pack-coprime | R | 1095 | 2498 | 26866 | 1193 | 9544 |
| 8 | Pack-primes | P | 1093 | 2946 | 30890 | 0 | 0 |
| 8 | Pack-primes | Q | 1087 | 2768 | 29264 | 0 | 0 |
| 8 | Pack-primes | A | 26 | 48 | 540 | 0 | 0 |
| 8 | Pack-primes | B | 28 | 55 | 611 | 0 | 0 |
| 8 | Pack-primes | C | 28 | 51 | 575 | 0 | 0 |
| 8 | Pack-primes | D | 29 | 55 | 615 | 0 | 0 |
| 8 | Pack-primes | R | 1087 | 2952 | 30920 | 0 | 0 |
| 8 | Pack-coprime | P | 1093 | 2486 | 26750 | 1168 | 9344 |
| 8 | Pack-coprime | Q | 1087 | 2411 | 26051 | 1172 | 9376 |
| 8 | Pack-coprime | A | 26 | 44 | 504 | 21 | 168 |
| 8 | Pack-coprime | B | 28 | 53 | 593 | 19 | 152 |
| 8 | Pack-coprime | C | 28 | 48 | 548 | 20 | 160 |
| 8 | Pack-coprime | D | 29 | 51 | 579 | 20 | 160 |
| 8 | Pack-coprime | R | 1087 | 2487 | 26735 | 1172 | 9376 |
| 9 | Pack-primes | P | 4188 | 11162 | 117214 | 0 | 0 |
| 9 | Pack-primes | Q | 4185 | 11274 | 118210 | 0 | 0 |
| 9 | Pack-primes | A | 3526 | 7123 | 78215 | 0 | 0 |
| 9 | Pack-primes | B | 3536 | 7158 | 78570 | 0 | 0 |
| 9 | Pack-primes | C | 3527 | 7262 | 79470 | 0 | 0 |
| 9 | Pack-primes | D | 3558 | 7335 | 80251 | 0 | 0 |
| 9 | Pack-primes | R | 4182 | 11342 | 118810 | 0 | 0 |
| 9 | Pack-coprime | P | 4188 | 10023 | 106963 | 4547 | 36376 |
| 9 | Pack-coprime | Q | 4185 | 10154 | 108130 | 4561 | 36488 |
| 9 | Pack-coprime | A | 3526 | 7123 | 78215 | 1127 | 9016 |
| 9 | Pack-coprime | B | 3536 | 7158 | 78570 | 1128 | 9024 |
| 9 | Pack-coprime | C | 3527 | 7262 | 79470 | 1065 | 8520 |
| 9 | Pack-coprime | D | 3558 | 7335 | 80251 | 1127 | 9016 |
| 9 | Pack-coprime | R | 4182 | 10120 | 107812 | 4575 | 36600 |
| 10 | Pack-primes | P | 4190 | 11286 | 118338 | 0 | 0 |
| 10 | Pack-primes | Q | 4189 | 11178 | 117362 | 0 | 0 |
| 10 | Pack-primes | A | 1012 | 2015 | 22187 | 0 | 0 |
| 10 | Pack-primes | B | 1015 | 2048 | 22496 | 0 | 0 |
| 10 | Pack-primes | C | 1012 | 2071 | 22691 | 0 | 0 |
| 10 | Pack-primes | D | 1023 | 2016 | 22240 | 0 | 0 |
| 10 | Pack-primes | R | 4184 | 11265 | 118125 | 0 | 0 |
| 10 | Pack-coprime | P | 4190 | 10168 | 108276 | 4549 | 36392 |
| 10 | Pack-coprime | Q | 4189 | 10012 | 106868 | 4570 | 36560 |
| 10 | Pack-coprime | A | 1012 | 2015 | 22187 | 478 | 3824 |
| 10 | Pack-coprime | B | 1015 | 2048 | 22496 | 473 | 3784 |
| 10 | Pack-coprime | C | 1012 | 2071 | 22691 | 441 | 3528 |
| 10 | Pack-coprime | D | 1023 | 2016 | 22240 | 499 | 3992 |
| 10 | Pack-coprime | R | 4184 | 9995 | 106695 | 4537 | 36296 |
| 12 | Pack-primes | P | 4190 | 11285 | 118329 | 0 | 0 |
| 12 | Pack-primes | Q | 4186 | 11085 | 116513 | 0 | 0 |
| 12 | Pack-primes | A | 89 | 183 | 2007 | 0 | 0 |
| 12 | Pack-primes | B | 90 | 178 | 1966 | 0 | 0 |
| 12 | Pack-primes | C | 86 | 159 | 1779 | 0 | 0 |
| 12 | Pack-primes | D | 90 | 170 | 1894 | 0 | 0 |
| 12 | Pack-primes | R | 4188 | 11358 | 118978 | 0 | 0 |
| 12 | Pack-coprime | P | 4190 | 10179 | 108375 | 4558 | 36464 |
| 12 | Pack-coprime | Q | 4186 | 10021 | 106937 | 4535 | 36280 |
| 12 | Pack-coprime | A | 89 | 179 | 1971 | 63 | 504 |
| 12 | Pack-coprime | B | 90 | 174 | 1930 | 65 | 520 |
| 12 | Pack-coprime | C | 86 | 157 | 1761 | 67 | 536 |
| 12 | Pack-coprime | D | 90 | 166 | 1858 | 64 | 512 |
| 12 | Pack-coprime | R | 4188 | 10136 | 107980 | 4572 | 36576 |

## Also reported (rule-m3 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-pack's stores, from the model; the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | distinct D | distinct P | distinct Q | distinct R | W_A | W_B | W_C | W_D | W_P | W_Q | W_R |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 282 | 283 | 284 | 282 | 314 | 303 | 296 |
| 2 | 16 | 4 | 64 | 64 | 64 | 64 | 64 | 256 | 256 | 256 | 84 | 87 | 88 | 88 | 313 | 309 | 302 |
| 4 | 16 | 64 | 4 | 4 | 4 | 4 | 4 | 256 | 256 | 256 | 8 | 8 | 8 | 8 | 305 | 295 | 296 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1013 | 1011 | 1003 | 1015 | 1089 | 1100 | 1092 |
| 6 | 32 | 4 | 256 | 256 | 256 | 256 | 256 | 1024 | 1024 | 1024 | 284 | 284 | 281 | 283 | 1097 | 1096 | 1095 |
| 8 | 32 | 64 | 16 | 16 | 16 | 16 | 16 | 1024 | 1024 | 1024 | 26 | 28 | 28 | 29 | 1093 | 1087 | 1087 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 3526 | 3536 | 3527 | 3558 | 4188 | 4185 | 4182 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 1024 | 1024 | 4096 | 4096 | 4096 | 1012 | 1015 | 1012 | 1023 | 4190 | 4189 | 4184 |
| 12 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 4096 | 4096 | 4096 | 89 | 90 | 86 | 90 | 4190 | 4186 | 4188 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.3 §16.9)

- job 1, P = A x B: add (4096): zero_x 0.062, int_x 0.003, int_y 0.048, den 0.004, general 0.883; mul (4096): int_both 0.016, den 0.031, cross_cb 0.000, int_x 0.101, int_y 0.109, general 0.743; cross tests 16384 (matched 194, cross reads 16384); P's store W 314, found / new 198 / 314
- job 1, Q = C x D: add (4096): zero_x 0.062, int_both 0.000, int_x 0.002, int_y 0.045, den 0.005, general 0.885; mul (4096): int_both 0.017, den 0.033, num 0.000, int_x 0.084, int_y 0.119, general 0.746; cross tests 16384 (matched 208, cross reads 16384); Q's store W 303, found / new 209 / 303
- job 1, R = P + Q: add (256): den 0.039, general 0.961; cross tests 1024 (matched 10, cross reads 1024); R's store W 296, found / new 216 / 296
- job 2, P = A x B: add (4096): zero_x 0.062, int_x 0.002, int_y 0.040, den 0.002, general 0.893; mul (4096): int_both 0.008, den 0.033, cross_ad 0.001, int_x 0.156, int_y 0.042, general 0.760; cross tests 16384 (matched 174, cross reads 16384); P's store W 313, found / new 199 / 313
- job 2, Q = C x D: add (4096): zero_x 0.062, int_both 0.000, int_x 0.003, int_y 0.024, den 0.004, general 0.907; mul (4096): int_both 0.005, den 0.038, int_x 0.105, int_y 0.038, general 0.814; cross tests 16384 (matched 175, cross reads 16384); Q's store W 309, found / new 203 / 309
- job 2, R = P + Q: add (256): den 0.027, general 0.973; cross tests 1024 (matched 7, cross reads 1024); R's store W 302, found / new 210 / 302
- job 4, P = A x B: add (4096): zero_x 0.062, den 0.012, general 0.925; mul (4096): den 0.052, general 0.948; cross tests 16384 (matched 211, cross reads 16384); P's store W 305, found / new 207 / 305
- job 4, Q = C x D: add (4096): zero_x 0.062, den 0.004, general 0.933; mul (4096): general 1.000; cross tests 16384 (matched 0, cross reads 16384); Q's store W 295, found / new 217 / 295
- job 4, R = P + Q: add (256): den 0.031, general 0.969; cross tests 1024 (matched 8, cross reads 1024); R's store W 296, found / new 216 / 296
- job 5, P = A x B: add (32768): zero_x 0.031, int_x 0.000, int_y 0.040, den 0.003, general 0.926; mul (32768): int_both 0.011, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.091, int_y 0.096, general 0.767; cross tests 131072 (matched 1529, cross reads 131072); P's store W 1089, found / new 959 / 1089
- job 5, Q = C x D: add (32768): zero_x 0.031, int_x 0.000, int_y 0.047, den 0.002, general 0.919; mul (32768): one_y 0.001, int_both 0.015, den 0.034, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.115, int_y 0.103, general 0.732; cross tests 131072 (matched 1621, cross reads 131040); Q's store W 1100, found / new 948 / 1100
- job 5, R = P + Q: add (1024): den 0.057, general 0.943; cross tests 4096 (matched 58, cross reads 4096); R's store W 1092, found / new 956 / 1092
- job 6, P = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.003, int_y 0.044, den 0.002, general 0.920; mul (32768): int_both 0.011, den 0.035, num 0.000, int_x 0.087, int_y 0.094, general 0.772; cross tests 131072 (matched 1522, cross reads 131072); P's store W 1097, found / new 951 / 1097
- job 6, Q = C x D: add (32768): zero_x 0.031, int_x 0.002, int_y 0.052, den 0.003, general 0.912; mul (32768): int_both 0.016, den 0.032, int_x 0.120, int_y 0.112, general 0.719; cross tests 131072 (matched 1584, cross reads 131072); Q's store W 1096, found / new 952 / 1096
- job 6, R = P + Q: add (1024): den 0.077, general 0.923; cross tests 4096 (matched 79, cross reads 4096); R's store W 1095, found / new 953 / 1095
- job 8, P = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.004, int_y 0.070, den 0.003, num 0.000, general 0.891; mul (32768): int_both 0.028, den 0.032, int_x 0.216, int_y 0.090, general 0.634; cross tests 131072 (matched 1959, cross reads 131072); P's store W 1093, found / new 955 / 1093
- job 8, Q = C x D: add (32768): zero_x 0.031, int_x 0.002, int_y 0.026, den 0.002, general 0.939; mul (32768): den 0.036, int_x 0.140, general 0.825; cross tests 131072 (matched 1165, cross reads 131072); Q's store W 1087, found / new 961 / 1087
- job 8, R = P + Q: add (1024): den 0.073, general 0.927; cross tests 4096 (matched 75, cross reads 4096); R's store W 1087, found / new 961 / 1087
- job 9, P = A x B: add (262144): zero_x 0.016, zero_y 0.000, int_both 0.000, int_x 0.001, int_y 0.040, den 0.002, general 0.941; mul (262144): zero 0.000, int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.100, int_y 0.093, general 0.760; cross tests 1048576 (matched 12233, cross reads 1048576); P's store W 4188, found / new 4004 / 4188
- job 9, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.046, den 0.002, general 0.936; mul (262144): int_both 0.014, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.107, int_y 0.098, general 0.746; cross tests 1048576 (matched 12802, cross reads 1048576); Q's store W 4185, found / new 4007 / 4185
- job 9, R = P + Q: add (4096): den 0.085, general 0.915; cross tests 16384 (matched 350, cross reads 16384); R's store W 4182, found / new 4010 / 4182
- job 10, P = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.038, den 0.001, general 0.945; mul (262144): int_both 0.011, den 0.034, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.086, int_y 0.104, general 0.763; cross tests 1048576 (matched 11991, cross reads 1048576); P's store W 4190, found / new 4002 / 4190
- job 10, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.043, den 0.002, general 0.939; mul (262144): int_both 0.013, den 0.036, num 0.000, cross_ad 0.000, int_x 0.120, int_y 0.084, general 0.747; cross tests 1048576 (matched 12798, cross reads 1048576); Q's store W 4189, found / new 4003 / 4189
- job 10, R = P + Q: add (4096): den 0.090, general 0.910; cross tests 16384 (matched 367, cross reads 16384); R's store W 4184, found / new 4008 / 4184
- job 12, P = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.027, den 0.002, general 0.955; mul (262144): int_both 0.005, den 0.032, num 0.000, int_x 0.038, int_y 0.103, general 0.822; cross tests 1048576 (matched 9619, cross reads 1048576); P's store W 4190, found / new 4002 / 4190
- job 12, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.022, den 0.002, general 0.960; mul (262144): int_both 0.001, den 0.036, int_x 0.014, int_y 0.096, general 0.853; cross tests 1048576 (matched 9861, cross reads 1048576); Q's store W 4186, found / new 4006 / 4186
- job 12, R = P + Q: add (4096): den 0.079, general 0.921; cross tests 16384 (matched 323, cross reads 16384); R's store W 4188, found / new 4004 / 4188

### Per step: the factored arithmetic (rule-m3 item 13; counting runs)

Reconciliation (Pack-coprime): passes, gcds, splits, |D_L| and |D_R|, (f) rebuilds (entries written). The arithmetic: merges (entries compared), the temporaries' lists (comparisons forming them), divisibility tests, per-piece gcds, multiplications forming a gcd's value, leftover splits, and the new sub-cases (9a-c num, 10a-c general, 12a-c multiplication's general).

| job | row | step | reconciliation | merges (entries) | list comparisons | tests | per-piece gcds | factor multiplications | leftover splits | 9a / 9b / 9c | 10a / 10b / 10c | 12a / 12b / 12c |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10564 (20796) | 17644 | 2279 | 0 | 17014 | 0 | 0 / 0 / 0 | 77 / 2200 / 1339 | 1774 / 1112 / 159 |
| 1 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10568 (21186) | 18211 | 2303 | 0 | 17343 | 0 | 0 / 0 / 0 | 62 / 2213 / 1351 | 1773 / 1155 / 126 |
| 1 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 246 (709) | 0 | 503 | 0 | 2465 | 0 | 0 / 0 / 0 | 0 / 46 / 200 | 0 / 0 / 0 |
| 1 | Pack-coprime | P | 1 passes, 1032 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10564 (20796) | 17644 | 2548 | 1548 | 20962 | 0 | 0 / 0 / 0 | 77 / 2200 / 1339 | 1774 / 1112 / 159 |
| 1 | Pack-coprime | Q | 1 passes, 1038 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10568 (21186) | 18211 | 2570 | 1541 | 21291 | 0 | 0 / 0 / 0 | 62 / 2213 / 1351 | 1773 / 1155 / 126 |
| 1 | Pack-coprime | R | 1 passes, 1845 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 246 (709) | 0 | 0 | 296 | 3484 | 0 | 0 / 0 / 0 | 0 / 46 / 200 | 0 / 0 / 0 |
| 2 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10694 (20818) | 17445 | 2475 | 0 | 16995 | 0 | 0 / 0 / 0 | 84 / 2134 / 1438 | 1897 / 1084 / 130 |
| 2 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 10971 (21886) | 19045 | 2517 | 0 | 18076 | 0 | 0 / 0 / 0 | 78 / 2200 / 1437 | 2129 / 1108 / 98 |
| 2 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 249 (728) | 0 | 487 | 0 | 2411 | 0 | 0 / 0 / 0 | 0 / 50 / 199 | 0 / 0 / 0 |
| 2 | Pack-coprime | P | 1 passes, 384 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10694 (20818) | 17445 | 2797 | 1640 | 21011 | 0 | 0 / 0 / 0 | 84 / 2134 / 1438 | 1897 / 1084 / 130 |
| 2 | Pack-coprime | Q | 1 passes, 372 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 10971 (21886) | 19045 | 2788 | 1673 | 22429 | 0 | 0 / 0 / 0 | 78 / 2200 / 1437 | 2129 / 1108 / 98 |
| 2 | Pack-coprime | R | 1 passes, 1830 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 249 (728) | 0 | 0 | 294 | 3415 | 0 | 0 / 0 / 0 | 0 / 50 / 199 | 0 / 0 / 0 |
| 4 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 11559 (24430) | 21356 | 4320 | 0 | 30039 | 0 | 0 / 0 / 0 | 1 / 1871 / 1917 | 3385 / 500 / 0 |
| 4 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 12014 (27117) | 26717 | 3971 | 0 | 24099 | 0 | 0 / 0 / 0 | 13 / 1691 / 2118 | 2065 / 2031 / 0 |
| 4 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 248 (709) | 0 | 473 | 0 | 2422 | 0 | 0 / 0 / 0 | 0 / 53 / 195 | 0 / 0 / 0 |
| 4 | Pack-coprime | P | 1 passes, 42 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 11559 (24430) | 21356 | 4513 | 2312 | 38637 | 0 | 0 / 0 / 0 | 1 / 1871 / 1917 | 3385 / 500 / 0 |
| 4 | Pack-coprime | Q | 2 passes, 101 gcds, 1 splits, D 3 / 3, 1 rebuilds (15) | 12014 (27117) | 26717 | 4240 | 2575 | 31709 | 0 | 0 / 0 / 0 | 13 / 1691 / 2118 | 2065 / 2031 / 0 |
| 4 | Pack-coprime | R | 1 passes, 1833 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 248 (709) | 0 | 0 | 304 | 3503 | 0 | 0 / 0 / 0 | 0 / 53 / 195 | 0 / 0 / 0 |
| 5 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 86717 (177056) | 152803 | 18793 | 0 | 156692 | 0 | 0 / 0 / 0 | 329 / 19133 / 10890 | 14848 / 9188 / 1081 |
| 5 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 85242 (173890) | 149285 | 18596 | 0 | 152561 | 0 | 0 / 0 / 0 | 361 / 18960 / 10806 | 14374 / 8576 / 1034 |
| 5 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 966 (2815) | 0 | 2311 | 0 | 10699 | 0 | 0 / 0 / 0 | 0 / 133 / 833 | 0 / 0 / 0 |
| 5 | Pack-coprime | P | 1 passes, 2841 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 86717 (177056) | 152803 | 20545 | 12386 | 190681 | 0 | 0 / 0 / 0 | 329 / 19133 / 10890 | 14848 / 9188 / 1081 |
| 5 | Pack-coprime | Q | 1 passes, 2871 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 85242 (173890) | 149285 | 20375 | 12269 | 185890 | 0 | 0 / 0 / 0 | 361 / 18960 / 10806 | 14374 / 8576 / 1034 |
| 5 | Pack-coprime | R | 1 passes, 7074 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 966 (2815) | 0 | 0 | 1355 | 15626 | 0 | 0 / 0 / 0 | 0 / 133 / 833 | 0 / 0 / 0 |
| 6 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 86683 (173503) | 148722 | 18680 | 0 | 153696 | 0 | 0 / 0 / 0 | 440 / 19144 / 10549 | 15909 / 8557 / 838 |
| 6 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 84636 (172351) | 148031 | 17696 | 0 | 146210 | 0 | 0 / 0 / 0 | 387 / 19032 / 10477 | 13366 / 9130 / 1060 |
| 6 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 945 (2763) | 0 | 2338 | 0 | 10328 | 0 | 0 / 0 / 0 | 0 / 148 / 797 | 0 / 0 / 0 |
| 6 | Pack-coprime | P | 1 passes, 1098 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 86683 (173503) | 148722 | 20380 | 12104 | 186854 | 0 | 0 / 0 / 0 | 440 / 19144 / 10549 | 15909 / 8557 / 838 |
| 6 | Pack-coprime | Q | 1 passes, 1053 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 84636 (172351) | 148031 | 19404 | 11840 | 178006 | 0 | 0 / 0 / 0 | 387 / 19032 / 10477 | 13366 / 9130 / 1060 |
| 6 | Pack-coprime | R | 1 passes, 7122 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 945 (2763) | 0 | 0 | 1368 | 15272 | 0 | 0 / 0 / 0 | 0 / 148 / 797 | 0 / 0 / 0 |
| 8 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 80787 (181893) | 158537 | 21842 | 0 | 175347 | 0 | 0 / 0 / 1 | 157 / 16726 / 12313 | 15177 / 5604 / 0 |
| 8 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 89402 (176899) | 154230 | 18363 | 0 | 173349 | 0 | 0 / 0 / 0 | 272 / 20342 / 10158 | 17334 / 8301 / 1392 |
| 8 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 949 (2769) | 0 | 2455 | 0 | 10933 | 0 | 0 / 0 / 0 | 0 / 136 / 813 | 0 / 0 / 0 |
| 8 | Pack-coprime | P | 1 passes, 120 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 80787 (181893) | 158537 | 23512 | 14412 | 216300 | 0 | 0 / 0 / 1 | 157 / 16726 / 12313 | 15177 / 5604 / 0 |
| 8 | Pack-coprime | Q | 1 passes, 120 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 89402 (176899) | 154230 | 19739 | 11342 | 207144 | 0 | 0 / 0 / 0 | 272 / 20342 / 10158 | 17334 / 8301 / 1392 |
| 8 | Pack-coprime | R | 1 passes, 7020 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 949 (2769) | 0 | 0 | 1424 | 16398 | 0 | 0 / 0 / 0 | 0 / 136 / 813 | 0 / 0 / 0 |
| 9 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 695971 (1413954) | 1216901 | 140478 | 0 | 1287289 | 0 | 0 / 0 / 0 | 2112 / 162436 / 82256 | 116557 / 73457 / 9242 |
| 9 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 690323 (1398418) | 1200800 | 137442 | 0 | 1266924 | 0 | 0 / 0 / 0 | 1976 / 162448 / 80926 | 113220 / 73165 / 9200 |
| 9 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3746 (10937) | 0 | 9981 | 0 | 43474 | 0 | 0 / 0 / 0 | 0 / 496 / 3250 | 0 / 0 / 0 |
| 9 | Pack-coprime | P | 1 passes, 6765 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 695971 (1413954) | 1216901 | 152177 | 92109 | 1548029 | 0 | 0 / 0 / 0 | 2112 / 162436 / 82256 | 116557 / 73457 / 9242 |
| 9 | Pack-coprime | Q | 1 passes, 6576 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 690323 (1398418) | 1200800 | 148829 | 90486 | 1522542 | 0 | 0 / 0 / 0 | 1976 / 162448 / 80926 | 113220 / 73165 / 9200 |
| 9 | Pack-coprime | R | 1 passes, 27324 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3746 (10937) | 0 | 0 | 5827 | 65898 | 0 | 0 / 0 / 0 | 0 / 496 / 3250 | 0 / 0 / 0 |
| 10 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 697997 (1426183) | 1237516 | 140205 | 0 | 1307850 | 0 | 0 / 0 / 0 | 1635 / 163765 / 82277 | 119361 / 71974 / 8803 |
| 10 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 691383 (1399532) | 1198828 | 135503 | 0 | 1241585 | 0 | 0 / 0 / 0 | 2163 / 163537 / 80367 | 110318 / 75813 / 9814 |
| 10 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3729 (10838) | 0 | 9944 | 0 | 43148 | 0 | 0 / 0 / 0 | 0 / 533 / 3196 | 0 / 0 / 0 |
| 10 | Pack-coprime | P | 1 passes, 2853 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 697997 (1426183) | 1237516 | 151335 | 92060 | 1570308 | 0 | 0 / 0 / 0 | 1635 / 163765 / 82277 | 119361 / 71974 / 8803 |
| 10 | Pack-coprime | Q | 1 passes, 2820 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 691383 (1399532) | 1198828 | 147330 | 89516 | 1490747 | 0 | 0 / 0 / 0 | 2163 / 163537 / 80367 | 110318 / 75813 / 9814 |
| 10 | Pack-coprime | R | 1 passes, 27357 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3729 (10838) | 0 | 0 | 5778 | 65222 | 0 | 0 / 0 / 0 | 0 / 533 / 3196 | 0 / 0 / 0 |
| 12 | Pack-primes | P | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 718401 (1444831) | 1284477 | 145812 | 0 | 1397839 | 0 | 0 / 0 / 0 | 1354 / 168620 / 80435 | 109044 / 93040 / 13383 |
| 12 | Pack-primes | Q | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 727506 (1493783) | 1319742 | 155999 | 0 | 1346830 | 0 | 0 / 0 / 0 | 1632 / 156051 / 94049 | 132264 / 83084 / 8143 |
| 12 | Pack-primes | R | 0 passes, 0 gcds, 0 splits, D 0 / 0, 0 rebuilds (0) | 3773 (10989) | 0 | 10115 | 0 | 44108 | 0 | 0 / 0 / 0 | 1 / 501 / 3271 | 0 / 0 / 0 |
| 12 | Pack-coprime | P | 1 passes, 384 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 718401 (1444831) | 1284477 | 155715 | 91957 | 1679413 | 0 | 0 / 0 / 0 | 1354 / 168620 / 80435 | 109044 / 93040 / 13383 |
| 12 | Pack-coprime | Q | 1 passes, 393 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 727506 (1493783) | 1319742 | 168588 | 105028 | 1639402 | 0 | 0 / 0 / 0 | 1632 / 156051 / 94049 | 132264 / 83084 / 8143 |
| 12 | Pack-coprime | R | 1 passes, 27279 gcds, 0 splits, D 3 / 3, 0 rebuilds (0) | 3773 (10989) | 0 | 0 | 5809 | 66646 | 0 | 0 / 0 / 0 | 1 / 501 / 3271 | 0 / 0 / 0 |

The gates checked every counter and store figure against the model (spec v0.3 §16.9 item 2), the TC gate included: MHVP:1 pass, TC:1 pass, MHPC:1 pass, TC:2 pass, MHVP:2 pass, MHPP:1 pass, MHPP:2 pass, TC:4 pass, MHPC:2 pass, MHVP:4 pass, MHPC:4 pass, MHPP:4 pass, TC:5 pass, MHVP:5 pass, MHPC:5 pass, TC:6 pass, MHVP:6 pass, MHPP:5 pass, MHPC:6 pass, TC:8 pass, MHVP:8 pass, MHPP:6 pass, MHPC:8 pass, TC:9 pass, MHVP:9 pass, MHPP:8 pass, MHPC:9 pass, TC:10 pass, MHVP:10 pass, MHPP:9 pass, MHPC:10 pass, TC:12 pass, MHVP:12 pass, MHPP:10 pass, MHPC:12 pass, MHPP:12 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m3 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 4.088 | no | 1 | 14 | 0.071 | 14 | 0 | none | 10 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 8.333 | no | 2 | 14 | 0.143 | 14 | 0 | none | 14 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 10 | pilot | 1 | 35.921 | no | 2 | 14 | 0.143 | 16 | 2 | none | 41 | none | process_api 1 (6->6 threads), sh 0 (1->1 threads), environment-man 1 (8->8 threads), claude 4 (8->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 21.271 | no | 21 | 64 | 0.328 | 66 | 2 | none | 27 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/7 (8->8 threads), claude 3/10 (7->7 threads), bash 0/7 (1->1 threads) |
| 2 | measured | 1 | 18.64 | no | 23 | 64 | 0.359 | 65 | 1 | none | 24 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 2/10 (7->7 threads), bash 0/7 (1->1 threads) |
| 4 | measured | 1 | 12.259 | no | 22 | 64 | 0.344 | 64 | 0 | none | 18 | none | process_api 1/6 (6->6 threads), sh 0/6 (1->1 threads), environment-man 0/6 (8->8 threads), claude 0/8 (8->7 threads), bash 0/6 (1->1 threads) |
| 5 | measured | 1 | 37.146 | no | 22 | 64 | 0.344 | 66 | 2 | none | 43 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 3/14 (7->7 threads), bash 0/8 (1->1 threads) |
| 6 | measured | 1 | 38.604 | no | 23 | 64 | 0.359 | 67 | 3 | none | 44 | none | process_api 2/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 5/14 (7->8 threads), bash 0/8 (1->1 threads) |
| 8 | measured | 1 | 27.54 | no | 23 | 64 | 0.359 | 66 | 2 | none | 33 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/7 (8->8 threads), claude 3/12 (8->8 threads), bash 0/7 (1->1 threads) |
| 9 | measured | 1 | 158.332 | no | 23 | 64 | 0.359 | 73 | 9 | none | 164 | none | process_api 5/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 3/14 (8->8 threads), claude 14/41 (8->6 threads), bash 0/14 (1->1 threads) |
| 10 | measured | 1 | 157.235 | no | 23 | 64 | 0.359 | 73 | 9 | none | 163 | none | process_api 4/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 2/14 (8->8 threads), claude 16/41 (6->7 threads), bash 0/14 (1->1 threads) |
| 12 | measured | 1 | 142.152 | no | 24 | 64 | 0.375 | 71 | 7 | none | 148 | none | process_api 5/13 (6->6 threads), sh 0/13 (1->1 threads), environment-man 2/13 (8->8 threads), claude 17/37 (7->7 threads), bash 0/13 (1->1 threads) |

The runner's own ticks over the 12 blocks: 209 ticks over 618 launches, 0.338 per launch; the highest block 0.375 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9996 | 0.9958 | 921514 | 921385 |
| 1 | Hash-V-pack | 12 | 0.9999 | 0.9939 | 1089562 | 1090064 |
| 1 | Pack-primes | 12 | 0.9986 | 0.9931 | 262836123 | 263218384 |
| 1 | Pack-coprime | 12 | 0.9997 | 0.9970 | 11009801 | 11010007 |
| 1 | Hash-V-pack-null | 12 | 0.9994 | 0.9110 | 1088142 | 1088802 |
| 2 | TC | 12 | 0.9999 | 0.9938 | 920330 | 919832 |
| 2 | Hash-V-pack | 12 | 0.9984 | 0.9757 | 1077091 | 1079298 |
| 2 | Pack-primes | 12 | 0.9992 | 0.9937 | 209268450 | 209610806 |
| 2 | Pack-coprime | 12 | 0.9992 | 0.9426 | 9083997 | 9107212 |
| 2 | Hash-V-pack-null | 12 | 0.9998 | 0.9493 | 1080036 | 1079516 |
| 4 | TC | 12 | 0.9997 | 0.9506 | 866148 | 865908 |
| 4 | Hash-V-pack | 12 | 0.9997 | 0.9950 | 1019365 | 1020463 |
| 4 | Pack-primes | 12 | 0.9996 | 0.9877 | 56562992 | 56591220 |
| 4 | Pack-coprime | 12 | 0.9992 | 0.9956 | 8031446 | 8050916 |
| 4 | Hash-V-pack-null | 12 | 0.9993 | 0.9911 | 1017438 | 1017250 |
| 5 | TC | 12 | 1.0000 | 0.9987 | 7573940 | 7574062 |
| 5 | Hash-V-pack | 12 | 1.0000 | 0.9978 | 8680925 | 8689442 |
| 5 | Pack-primes | 12 | 0.9987 | 0.9873 | 1822255332 | 1828278178 |
| 5 | Pack-coprime | 12 | 0.9999 | 0.9916 | 141580986 | 141647212 |
| 5 | Hash-V-pack-null | 12 | 1.0000 | 0.9892 | 8654883 | 8660360 |
| 6 | TC | 12 | 0.9998 | 0.9979 | 7514516 | 7514216 |
| 6 | Hash-V-pack | 12 | 0.9999 | 0.9896 | 8583397 | 8595632 |
| 6 | Pack-primes | 12 | 0.9981 | 0.9884 | 1974889408 | 1988870143 |
| 6 | Pack-coprime | 12 | 0.9980 | 0.9925 | 127248274 | 127429805 |
| 6 | Hash-V-pack-null | 12 | 1.0000 | 0.9993 | 8559354 | 8558544 |
| 8 | TC | 12 | 0.9998 | 0.9963 | 7191776 | 7194957 |
| 8 | Hash-V-pack | 12 | 0.9998 | 0.9938 | 8227708 | 8231815 |
| 8 | Pack-primes | 12 | 0.9921 | 0.9773 | 1163031848 | 1174626406 |
| 8 | Pack-coprime | 12 | 0.9990 | 0.9753 | 119055792 | 119188666 |
| 8 | Hash-V-pack-null | 12 | 0.9996 | 0.9145 | 8243518 | 8247445 |
| 9 | TC | 12 | 1.0000 | 0.9684 | 61187476 | 61221560 |
| 9 | Hash-V-pack | 12 | 0.9996 | 0.9545 | 69041709 | 69064518 |
| 9 | Pack-primes | 12 | 0.9951 | 0.9880 | 9580355544 | 9632411236 |
| 9 | Pack-coprime | 12 | 0.9963 | 0.9821 | 1916858894 | 1924963858 |
| 9 | Hash-V-pack-null | 12 | 0.9998 | 0.9944 | 68996092 | 69012570 |
| 10 | TC | 12 | 1.0000 | 0.9894 | 61257633 | 61270602 |
| 10 | Hash-V-pack | 12 | 0.9992 | 0.9891 | 69073432 | 69180113 |
| 10 | Pack-primes | 12 | 0.9942 | 0.9917 | 9582732550 | 9636271428 |
| 10 | Pack-coprime | 12 | 0.9960 | 0.9798 | 1826260118 | 1848918966 |
| 10 | Hash-V-pack-null | 12 | 1.0000 | 0.9885 | 69004769 | 69021171 |
| 12 | TC | 12 | 0.9998 | 0.9668 | 62079672 | 62105161 |
| 12 | Hash-V-pack | 12 | 0.9998 | 0.9895 | 69635815 | 69671091 |
| 12 | Pack-primes | 12 | 0.9949 | 0.9903 | 8475016248 | 8535099785 |
| 12 | Pack-coprime | 12 | 0.9980 | 0.9695 | 1783973204 | 1788062044 |
| 12 | Hash-V-pack-null | 12 | 0.9996 | 0.9955 | 69524781 | 69543081 |

Idle checks: 12 (0 busy). Host rates R_h: {'1': 0.027838868628378942, '79': 0.027838868628378942, '84': 0.027838868628378942, '92': 0.11135501719565598, '21895': 0.027838868628378942} (measured: {'1': 0.027838754298913995, '79': 0.0, '84': 0.027838754298913995, '92': 0.11135501719565598, '21895': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

