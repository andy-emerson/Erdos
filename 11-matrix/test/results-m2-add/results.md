# Design 3 on matrices, round m2, matrix addition: results

Run started 2026-10-30T09:10:08-05:00.

## The measurement rule (research/rule-m2.md, verbatim, written before the first trial)

sha256 `e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200`

<!-- rule begins -->
# Design 3 on matrices, round m2: measurement rule

Oct 30, 2026 · **Fixed: approved by the author, Oct 30, 2026,** in full: the rows, the pairings, the measures, the four job sets, the grid, the run order and spec v0.2's design. The harness refuses a measured run unless this file has the approved text (by sha256, pinned in the harness), and while any mark of a proposal awaiting approval remains in it. Nothing is built, run or measured.

This is a new rule for spec [v0.2](../v0.2.md). [`rule-m1.md`](rule-m1.md) stays fixed and unedited, because it was used for the m1 runs, whose results stand as measured and enter no measure here. Each threshold is marked either **carried over** from rule-m1 (approved by the author, Oct 29, 2026) or **approved by the author, Oct 30, 2026**. A change after the first trial is a revision, dated and explained at the top.

## The rows

This rule measures the four builds of spec v0.2 §15.3. No row pre-stores 0 or 1; every zero and one test is a value comparison (v0.2 §15.2).

| Build | Structures, by name | add, mul | sop, pos |
| --- | --- | --- | --- |
| **TC** | each matrix's pairs | 3 | 7 |
| **Hash-V-fixed** | per matrix, (a) cells and (b) a table of $T_\text{fix}$ slots | 6 | 14 |
| **Hash-V-grow-map** | per matrix, (a) and a growing (b); (r) the slot map during a growth | 7 | 15 |
| **Hash-V-pack** | per matrix, (a) cells and **(l) the packed list**, new; (h) the build table and (p) the pack map, both only during a build | 8 | 16 |

All four use the same 64-bit core and the same inputs. Their arithmetic is identical, call for call. Results are reported **per measure, with no overall winner**.

**Four job sets, four runs** (approved by the author, Oct 30, 2026): **add**, **mul**, **sop** ($A \cdot B + C \cdot D$) and **pos** ($(A + B) \cdot (C + D)$). Each run has its own steps 0–7, pilot, hard stop, results folder and report. Nothing measured in one run enters a measure of another.

**Run order** (approved by the author, Oct 30, 2026): add, mul, sop, pos, **back to back**, without waiting for the author between runs. Each run is still its own background command, with its own steps 0–7, pilot, hard stop and folder. A run that stops (pilot or hard stop) does not stop the next one; its report says so. The m1 harness's check that a run waits for a finished earlier run (DECISIONS M1-H4) is dropped.

## Measurement rule

1. **Rows.** Each job runs **five rows**, in this order for the rounds: **TC, Hash-V-fixed, Hash-V-grow-map, Hash-V-pack, Hash-V-fixed-null**. The null row is Hash-V-fixed's binary, as in m1, and stays last (approved by the author, Oct 30, 2026).

2. **Builds.** As rule-m1 item 2 (carried over):
   - `gcc -std=c11 -O2`;
   - the 08 module and `tc3.c` unchanged, their sha256 checked before every run;
   - `tcm.c`'s recorded diff checked;
   - three builds per row: timing, counting and test;
   - one counting run and one memory run per row and job, with a limit of **10 minutes** each;
   - repeats on jobs **1 and 6** of each set MUST be identical.

   The same binaries serve all four runs; the job decides the operation.

3. **Gates before the measured run.** As rule-m1 item 3 (carried over), for every row and every job of the run's set:
   - the unit tests;
   - spec v0.2 §15.7's exact checks, every step of an expression included;
   - the call-for-call check against TC;
   - the TC gate, with `tc3.c` run on the job's operations, every step.

   Gates MAY run two at a time, only before the timed steps.

4. **Timing.** As rule-m1 item 4, every number carried over:
   - **12 interleaved rounds**, the order rotated by one row per round over the **five** rows;
   - **r**, the smallest power of two with a rep of at least **50 ms**;
   - **3 reps** per invocation, the first discarded and the minimum of the other two kept, or **1 rep** if the calibration rep lasts at least **1 s**;
   - calibration per job just before round 1 for the four builds, the null row taking Hash-V-fixed's r;
   - pinning to **core 1**, with the runner on core 0;
   - a cap of **10 s** per rep;
   - the process's own CPU time, reported only.

   **The parts** (v0.2 §15.6; approved by the author, Oct 30, 2026):
   - **one build per input matrix**, timed separately: its `create`, its entry, its growths (Hash-V-grow-map) and its packing (Hash-V-pack);
   - **the operation:** every step, each result's `create` coming just before its own step, so only one build table (h) is alive at a time (v0.2 §15.5 item 5); with each result's stores, widenings, growths and packing;
   - **the read-out:** of the final result only.

   Each timed quantity (each input's build, the operation, the read-out) is taken per run, as its rep's time over r. Each quantity keeps its own minimum of reps 2 and 3, as m1 does per part (carried over in form).

5. **Nothing of the runner's own runs during a timed phase.** Carried over in full from rule-m1 item 5:
   - the single-threaded runner;
   - `fork`/`exec` with `sched_setaffinity` to core 1 and `dup2` to a file;
   - one blocking `waitpid`;
   - the kernel timer for the hard stop;
   - no writes during an invocation;
   - one background command per run, with a timeout of at least **45 minutes**.

   **The disturbance check by process**, with every number:
   - other processes at most $5 + \lceil 1 \cdot L \rceil$ ticks;
   - a new process under 1 tick;
   - hosts held to $5 + \lceil 2\,R_h\,L \rceil$, with $R_h$ from the run's own pilot;
   - a stop above **4 ticks per second** for any host;
   - a disturbed block re-run once, with a second disturbance stopping the run.

   **The runner's ticks** are reported, with the sanity limit of more than **1 tick per launch plus 2** per block.

6. **Idle machine.** Carried over:
   - a **2-second** sample before each timed block;
   - idle when no process outside the runner used more than **5%** of one core and core 1 was at most **2%** busy;
   - if busy, re-check every **15 s** and stop after **10 minutes**.

7. **Measures, per job, for each pairing, in each run** (the author's measures, approved by the author, Oct 30, 2026; spec v0.2 §15.6).

   **Six pairings** (approved by the author, Oct 30, 2026):
   - **against TC** (3), with verdicts: Hash-V-fixed, Hash-V-grow-map, Hash-V-pack;
   - **Hash-V-pack against Hash-V-fixed**, with verdicts. It isolates one change: packing after the build;
   - **Hash-V-grow-map against Hash-V-fixed**, with verdicts. It isolates the sizing;
   - **Hash-V-pack against Hash-V-grow-map: reported, no verdict on any measure.**

   **Headline measures, each with its own verdict per pairing:**

   | Run | Speed | Memory |
   | --- | --- | --- |
   | add | **1** build time of one input matrix; **2** addition time | **6** stable bytes of one stored input matrix; **7** peak bytes during addition |
   | mul | **1**; **3** multiplication time | **6**; **8** peak bytes during multiplication |
   | sop | **1**; **4** sum-of-products time | **6** (sop's peak: reported, no verdict) |
   | pos | **1**; **5** product-of-sums time | **6** (pos's peak: reported, no verdict) |

   - **Time measures (1–5):** $T(\text{reference})/T(\text{row})$, above 1 the row is faster. $T$ is the median over rounds of the operation per run (2–5), or the median of the pooled per-matrix samples (1). The 95% interval comes from **10,000 paired bootstrap resamples of the rounds**, seed 20261029 (carried over). For measure 1, a resampled round brings all its input matrices' samples with it. This pooling is v0.2 §15.6 (approved by the author, Oct 30, 2026).
   - **Memory measures (6–8):** row over reference, below 1 the row is smaller. The verdict is on the accounted bytes per cell:
     - measure 6: the inputs' stable bytes summed, over their cells summed;
     - measures 7 and 8: the accounted peak during the operation, over the operation's cells.

     The heap figures are reported beside them. The peak tracking is v0.2 §15.6's (approved by the author, Oct 30, 2026).
   - **Gcd work: a check, not a verdict.** G(row)/G(TC) MUST be exactly 1 for every hash row (carried over).

   **Reported, no verdict:**
   - total and read-out time;
   - per-matrix build times;
   - the whole job's bytes after creation and at the end (sop and pos: the four inputs and $R$, the intermediates having been freed);
   - the peak during each input's build;
   - Hash-V-grow-map's growth peak;
   - sop's and pos's peaks;
   - the counts of item 13.

8. **Null control.** Carried over: Hash-V-fixed against Hash-V-fixed-null gives the null spread per time measure, and **h = max(5%, that spread)** for every pairing of the run. Each run has two time bands: one for measure 1 and one for its operation measure. The caveat and the "(c)" mark are carried over.

9. **Verdicts and magnitudes.** Carried over, as rule-m1 item 9:
   - time (measures 1–5) wins when lo > 1 + h, loses when hi < 1 − h, and ties otherwise;
   - memory (measures 6–8) wins at a ratio ≤ **0.90**, loses at ≥ **1.10**, and ties otherwise;
   - severe-memory-loss flag at a ratio of **2.0** or more;
   - "over cap" on both time measures;
   - no verdict without its magnitude.

10. **No profile** (carried over).

11. **Strata, and no overall winner.** Carried over, per run: **NONE**, **LOW**, **MID** and **HIGH** ($\rho$ = 1, 4, 16, 64), which set the sharing of the inputs. For each stratum, pairing and measure:
    - counts of verdicts, the median ratio and severe flags;
    - the size × sharing grid of ratios, presented, not judged.

    These are given for the run's headline measures only.

    No measures and no runs are combined.

12. **Checks, and bugs after the gates.** As rule-m1 item 12 (carried over), with every step of an expression checked. New bugs (approved by the author, Oct 30, 2026):
    - a reserved or pre-stored slot;
    - a handle test for 0 or 1;
    - a packed list whose length is not $W$;
    - a handle of a packed matrix $\ge W$;
    - a build table or pack map alive outside its build.

13. **Also reported.** As rule-m1 item 13 (carried over), without $\tau$, per matrix and, in sop and pos, per step. For Hash-V-pack (approved by the author, Oct 30, 2026): $W$, $w_{h,\text{build}}$, $w_{h,\text{pack}}$, list bytes, cells rewritten and map bytes. The m1 runs are linked for reference only.

## Workloads

- **add and mul:** carried over unchanged from rule-m1:
  - the generator `m1_gen.py`;
  - seeds 20261029 + $j$ and 20261029 + 100 + $j$;
  - 16 jobs each.

  Step 0's files MUST equal m1's by sha256 (approved by the author, Oct 30, 2026).
- **sop and pos** (approved by the author, Oct 30, 2026; v0.2 §15.5 item 7): $A$, $B$, $C$, $D$ are each drawn as rule-m1 draws $A$ and $B$:
  - numerators uniform in $[-32767, 32767]$, denominators among the 30 divisors of 720, reduced;
  - $s = n^2/\rho$ distinct values per matrix, each matrix with its own pool;
  - one stream per job, drawing $A$, $B$, $C$, $D$ in order;
  - job $j$ seeded 20261029 + 200 + $j$ (sop) or + 300 + $j$ (pos).

  Every 64-bit intermediate fits, by v0.2 §15.5 item 8.
- **The expression grid** (approved by the author, Oct 30, 2026) is the same as add and mul: $n \in \{16, 32, 64, 128\}$ by $\rho \in \{1, 4, 16, 64\}$, **16 jobs per set**, numbered as in rule-m1. *Why:* the runtime estimate below fits the 40-minute hard stop with a margin of about 2 or more on both bases, so no size needs to go. Fewer sizes would lose the $n$ steps that separate floor-dominated jobs from arithmetic-dominated ones.

## Run plan (once per run)

Steps 0–7 are as rule-m1's Run plan (carried over), per run. Results go to **`11-matrix/test/results-m2-add/`**, **`-mul/`**, **`-sop/`** and **`-pos/`** (approved by the author, Oct 30, 2026).

- **Step 3, pilot:** jobs **1, 6 and 13**, every row, r calibration and **2 rounds**. That is 4 + 10 = **14 launches** per block (m1: 20). **Stop rule, carried over:**
  $$\text{projection} = \text{elapsed} + 1.2 \times 5 \times (N_{128}\,P_{13} + N_\text{rest}\,P_6),$$
  with $N_{128} = 4$ and $N_\text{rest} = 12$. The factor 5 still covers the scale-up: $64 / 14 = 4.57$. **If the projection exceeds 30 minutes, the run stops** (carried over). It also stops on a host above 4 ticks per second.
- **Step 6:** per job, in job order: the idle check, calibration (**4 launches**) and 12 rounds (**60 launches**), **64 launches** per block, then the disturbance check.
- **Hard stop at 40 minutes, per run** (carried over), from step 1 to the end of step 6, by the kernel timer. A stopped run reports no verdict.

### Runtime estimate (non-normative)

There are two bases:
- **measured:** m1's own runs, scaled to 64 launches per block and 14 per pilot block;
- **model:** rule-m1's own estimate, which was about 1.6–1.8 times slower than m1 measured, scaled the same way.

**Per-run cost:**
- Packing costs one pass over $T_\text{fix}$ slots and the cells per matrix: microseconds to a millisecond.
- **sop** does two multiplications and an addition, about twice mul's time per run: at $n = 128$, about 0.5 s (TC) to 0.56 s (hash rows), against a model of 0.7–0.9 s. That is under 1 s, so 3 reps, about 1.8 s per launch (model 2.6 s), within the 10 s cap.
- **pos** does two additions and a multiplication of larger values, about 1.15 times mul's.
- Launches at $n \le 32$ are at their floor of about 0.2–0.3 s in every set.

| Run | On the clock, measured basis | On the clock, model basis | Pilot projection, measured | Pilot projection, model |
| --- | --- | --- | --- | --- |
| add | about 5 min | about 10 min | about 8 min | about 11 min |
| mul | about 8 min (9 with one re-run) | about 14.5 min | about 11 min | about 17 min |
| sop | about 13 min (15 with one re-run) | about 20 min | about 17.5 min ($P_{13} \approx 26.5$ s) | about 25.5 min ($P_{13} \approx 38$ s) |
| pos | about 8.5 min | about 15.5 min | about 12.5 min | about 18.5 min |

**Margins:**
- **Hard stop (40 min):** about 3 or more on the measured basis (sop about 2.7 with one re-run), and at least 2 on the model basis.
- **Pilot limit (30 min):** at least 1.7 on the measured basis. On the model basis it is 1.2 for sop, the tightest. sop's projection would pass 30 only if its $n = 128$ launches ran about 1.2 times slower than the model, about 2 times slower than m1 measured. The pilot's stop would then report it to the author, as intended.

The per-matrix clock reads add one or two reads per input per run, and the peak tracking is in the memory build only, so the estimates above stand. **No threshold needs to change.**

**Step 0, outside the clock:** mul about 1.5 min (m1 measured 88 s), sop about 3 min, pos about 2 min.

## Limitations, reported with the results

Carried over from rule-m1, without the index store, the M rows and $\tau$. Also (approved by the author, Oct 30, 2026):
- **Only V is tested.** The cross-test variant M was dropped with its rows.
- **Hash-V-pack's build table is Hash-V-fixed's**, so its build costs at least as much as Hash-V-fixed's. Its gains, if any, are in memory and in reading operands from dense lists.
- **The expression sets reuse one value distribution**, and their sharing comes only from the inputs.
- **Results are reported per run** and not combined across runs or with m1.

## Harness

The m1 harness is copied to `m2_*` files: its spawner, checks, calibration, bootstrap and stops unchanged (carried over). New (approved by the author, Oct 30, 2026):
- the rows of v0.2;
- the model without pre-stored 0 and 1, and with packing;
- the expression generator and its model, step by step;
- a run argument of `add`, `mul`, `sop` or `pos`;
- **the driver times each input matrix's build separately**: a clock read before its `create` and after its last `enter`. The result's and the intermediates' `create` fall in the operation;
- **the memory build tracks the peak:** each row's running total of accounted live bytes, updated at every allocation and free of a structure, and an allocation wrapper keeping the live sum of `malloc_usable_size` and its maximum, reset at the start of the operation. It records each input's stable bytes after its build, the peak during each build and during the operation, and the end figures. The timing build carries neither;
- the analysis of the six pairings on the eight headline measures, with the pooled bootstrap of measure 1;
- this rule's pin.

The harness refuses to write in `../10-interning/`, `results-m1-add/` or `results-m1-mul/`, or in an earlier m2 run's folder, and checks their sha256 before and after each run (carried over in form). Builder decisions go in a new `11-matrix/test/DECISIONS-m2.md`, pending the author's review.
<!-- rule ends -->

## The run

- The add run (job set `add`: C = A + B).
- Started 2026-10-30T09:10:08-05:00; finished 2026-10-30T09:15:18-05:00.
- Step 0 (before the clock): 2.2 s; step 0's files: equal to m1's by sha256.
- Run time 5.1 minutes (hard stop 40).
- Rule sha256 `e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200`; round 3's results after the run: unchanged; m1's results and the other m2 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[17736], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T09:10:10-05:00: TC `179dacc22ad6`, MHVF `eb7f57fd8798`, MHVGM `915116a75090`, MHVP `56a37a633b7c`, TC3GATE `83ff353acc03`
- Unit tests: m2_unit: m2_unit: 642586 checks, 0 failures; m2_diff: m2_diff: 3000 seeds, 3000 jobs, 1050092 operations; 1137376 checks, 0 failures; m2_alloc-timing: m2_alloc (timing build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-counting: m2_alloc (counting build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-memory: m2_alloc (memory build): 160 jobs, 2450 moments, 32596 checks, 0 failures; m2_unit-asan: m2_unit: 642586 checks, 0 failures; m2_diff-asan: m2_diff: 600 seeds, 600 jobs, 209812 operations; 227944 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 5.0 s, job 6 5.0 s, job 13 5.2 s; P_6 = 5.0 s, P_13 = 5.2 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 8.7 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.31094527363184077, '79': 0.31094527363184077, '84': 0.337026276628065, '92': 0.33843642932135554, '17736': 0.31094527363184077}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 2 addition time; 6 stable bytes of one stored input matrix; 7 peak bytes during the operation. Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-fixed against Hash-V-fixed-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses). Hash-V-pack against Hash-V-grow-map is reported with no verdict on any measure. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m2 item 8, carried over).

### Hash-V-fixed against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.561 [0.548, 0.567], h 0.050 | loss, 0.434 [0.432, 0.438], h 0.050 | reported, 0.504 [0.500, 0.508] | reported, 0.783 [0.601, 0.787] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.083 (heap 2.073) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.604 [0.596, 0.611], h 0.050 | loss, 0.422 [0.419, 0.425], h 0.050 | reported, 0.516 [0.505, 0.521] | reported, 0.601 [0.576, 0.606] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.083 (heap 2.073) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.623 [0.611, 0.631], h 0.050 | loss, 0.464 [0.460, 0.475], h 0.050 | reported, 0.544 [0.538, 0.550] | reported, 0.604 [0.598, 0.672] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.000 (heap 1.991) **severe memory loss** |
| 4 (n 16, rho 64) | loss, 0.645 [0.632, 0.672], h 0.050 | loss, 0.533 [0.528, 0.546], h 0.050 | reported, 0.599 [0.589, 0.605] | reported, 0.734 [0.599, 0.847] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.000 (heap 1.991) **severe memory loss** |
| 5 (n 32, rho 1) | loss, 0.623 [0.616, 0.638], h 0.050 | loss, 0.455 [0.449, 0.462], h 0.050 | reported, 0.550 [0.541, 0.559] | reported, 0.632 [0.625, 0.760] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.083 (heap 2.081) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.602 [0.594, 0.616], h 0.050 | loss, 0.467 [0.461, 0.473], h 0.050 | reported, 0.550 [0.541, 0.555] | reported, 0.637 [0.622, 0.850] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.083 (heap 2.081) **severe memory loss** |
| 7 (n 32, rho 16) | loss, 0.668 [0.654, 0.679], h 0.050 | loss, 0.497 [0.492, 0.502], h 0.050 | reported, 0.589 [0.584, 0.600] | reported, 0.698 [0.625, 0.850] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.000 (heap 1.998) **severe memory loss** |
| 8 (n 32, rho 64) | loss, 0.602 [0.596, 0.606], h 0.058 | loss, 0.400 [0.396, 0.404], h 0.050 | reported, 0.517 [0.509, 0.519] | reported, 0.744 [0.654, 0.864] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.000 (heap 1.998) **severe memory loss** |
| 9 (n 64, rho 1) | loss, 0.694 [0.690, 0.698], h 0.050 | loss, 0.564 [0.561, 0.568], h 0.050 | reported, 0.640 [0.638, 0.642] | reported, 0.671 [0.659, 0.888] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.000 (heap 1.999) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.694 [0.691, 0.700], h 0.050 | loss, 0.562 [0.559, 0.566], h 0.050 | reported, 0.641 [0.639, 0.644] | reported, 0.679 [0.664, 0.703] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.000 (heap 1.999) **severe memory loss** |
| 11 (n 64, rho 16) | loss, 0.734 [0.732, 0.736], h 0.050 | loss, 0.561 [0.557, 0.562], h 0.050 | reported, 0.659 [0.653, 0.662] | reported, 0.678 [0.659, 0.882] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.000 (heap 1.999) **severe memory loss** |
| 12 (n 64, rho 64) | loss, 0.744 [0.736, 0.747], h 0.050 | loss, 0.537 [0.535, 0.543], h 0.050 | reported, 0.646 [0.643, 0.649] | reported, 0.701 [0.671, 0.821] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.083 (heap 2.083) **severe memory loss** |
| 13 (n 128, rho 1) | loss, 0.687 [0.682, 0.689], h 0.050 | loss, 0.536 [0.524, 0.542], h 0.050 | reported, 0.621 [0.612, 0.624] | reported, 0.610 [0.588, 0.630] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.250 (heap 2.250) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.721 [0.718, 0.723], h 0.050 | loss, 0.566 [0.561, 0.570], h 0.050 | reported, 0.652 [0.649, 0.658] | reported, 0.629 [0.608, 0.736] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.250 (heap 2.250) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.755 [0.753, 0.759], h 0.050 | loss, 0.555 [0.553, 0.557], h 0.050 | reported, 0.665 [0.663, 0.668] | reported, 0.664 [0.638, 0.824] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.333 (heap 2.333) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.754 [0.753, 0.756], h 0.050 | loss, 0.553 [0.551, 0.555], h 0.050 | reported, 0.665 [0.664, 0.667] | reported, 0.624 [0.611, 0.663] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.250 (heap 2.250) **severe memory loss** |

### Hash-V-grow-map against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.195 [0.194, 0.198], h 0.050 | loss, 0.266 [0.265, 0.268], h 0.050 | reported, 0.228 [0.226, 0.230] | reported, 0.726 [0.561, 0.733] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.167 (heap 2.155) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.388 [0.383, 0.394], h 0.050 | loss, 0.281 [0.277, 0.282], h 0.050 | reported, 0.339 [0.335, 0.342] | reported, 0.541 [0.534, 0.547] | win, 0.750 (heap 0.764) | loss, 1.167 (heap 1.174) |
| 3 (n 16, rho 16) | loss, 0.513 [0.504, 0.520], h 0.050 | loss, 0.378 [0.373, 0.381], h 0.050 | reported, 0.445 [0.439, 0.448] | reported, 0.588 [0.581, 0.651] | win, 0.375 (heap 0.397) | win, 0.625 (heap 0.643) |
| 4 (n 16, rho 64) | loss, 0.542 [0.533, 0.566], h 0.050 | loss, 0.497 [0.491, 0.511], h 0.050 | reported, 0.530 [0.522, 0.539] | reported, 0.732 [0.608, 0.841] | win, 0.281 (heap 0.305) | win, 0.307 (heap 0.331) |
| 5 (n 32, rho 1) | loss, 0.352 [0.346, 0.360], h 0.050 | loss, 0.288 [0.285, 0.292], h 0.050 | reported, 0.329 [0.323, 0.336] | reported, 0.619 [0.604, 0.738] | loss, 1.250 (heap 1.251) | loss, 1.500 (heap 1.500) |
| 6 (n 32, rho 4) | loss, 0.416 [0.410, 0.425], h 0.050 | loss, 0.295 [0.291, 0.298], h 0.050 | reported, 0.366 [0.362, 0.369] | reported, 0.594 [0.570, 0.790] | win, 0.750 (heap 0.754) | loss, 1.167 (heap 1.169) |
| 7 (n 32, rho 16) | loss, 0.516 [0.505, 0.523], h 0.050 | loss, 0.371 [0.366, 0.375], h 0.050 | reported, 0.452 [0.446, 0.456] | reported, 0.675 [0.605, 0.822] | win, 0.375 (heap 0.380) | win, 0.625 (heap 0.630) |
| 8 (n 32, rho 64) | loss, 0.459 [0.455, 0.462], h 0.058 | loss, 0.330 [0.328, 0.334], h 0.050 | reported, 0.409 [0.404, 0.411] | reported, 0.703 [0.620, 0.814] | win, 0.281 (heap 0.287) | win, 0.417 (heap 0.422) |
| 9 (n 64, rho 1) | loss, 0.435 [0.432, 0.438], h 0.050 | loss, 0.421 [0.419, 0.423], h 0.050 | reported, 0.433 [0.431, 0.434] | reported, 0.641 [0.630, 0.865] | loss, 1.250 (heap 1.250) | loss, 1.208 (heap 1.209) |
| 10 (n 64, rho 4) | loss, 0.558 [0.555, 0.563], h 0.050 | loss, 0.422 [0.420, 0.426], h 0.050 | reported, 0.500 [0.498, 0.505] | reported, 0.663 [0.633, 0.686] | win, 0.500 (heap 0.501) | win, 0.708 (heap 0.709) |
| 11 (n 64, rho 16) | loss, 0.628 [0.624, 0.630], h 0.050 | loss, 0.422 [0.420, 0.424], h 0.050 | reported, 0.533 [0.528, 0.537] | reported, 0.664 [0.641, 0.862] | win, 0.375 (heap 0.376) | win, 0.625 (heap 0.626) |
| 12 (n 64, rho 64) | loss, 0.660 [0.654, 0.666], h 0.050 | loss, 0.425 [0.423, 0.430], h 0.050 | reported, 0.545 [0.542, 0.548] | reported, 0.673 [0.638, 0.787] | win, 0.281 (heap 0.283) | win, 0.562 (heap 0.564) |
| 13 (n 128, rho 1) | loss, 0.460 [0.456, 0.461], h 0.050 | loss, 0.433 [0.432, 0.435], h 0.050 | reported, 0.451 [0.448, 0.453] | reported, 0.659 [0.646, 0.669] | loss, 1.500 (heap 1.500) | loss, 1.500 (heap 1.500) |
| 14 (n 128, rho 4) | loss, 0.595 [0.590, 0.597], h 0.050 | loss, 0.442 [0.439, 0.444], h 0.050 | reported, 0.527 [0.522, 0.531] | reported, 0.650 [0.633, 0.754] | win, 0.750 (heap 0.750) | tie, 1.000 (heap 1.000) |
| 15 (n 128, rho 16) | loss, 0.651 [0.648, 0.653], h 0.050 | loss, 0.439 [0.438, 0.441], h 0.050 | reported, 0.552 [0.551, 0.554] | reported, 0.686 [0.660, 0.851] | win, 0.562 (heap 0.563) | win, 0.875 (heap 0.875) |
| 16 (n 128, rho 64) | loss, 0.694 [0.692, 0.696], h 0.050 | loss, 0.424 [0.423, 0.425], h 0.050 | reported, 0.563 [0.561, 0.566] | reported, 0.663 [0.647, 0.705] | win, 0.531 (heap 0.532) | win, 0.854 (heap 0.854) |

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.381 [0.373, 0.385], h 0.050 | loss, 0.374 [0.371, 0.375], h 0.050 | reported, 0.382 [0.378, 0.386] | reported, 0.618 [0.473, 0.624] | win, 0.806 (heap 0.822) | loss, 1.213 (heap 1.223) |
| 2 (n 16, rho 4) | loss, 0.445 [0.439, 0.450], h 0.050 | loss, 0.364 [0.359, 0.366], h 0.050 | reported, 0.409 [0.406, 0.413] | reported, 0.474 [0.464, 0.483] | win, 0.294 (heap 0.320) | win, 0.871 (heap 0.887) |
| 3 (n 16, rho 16) | loss, 0.486 [0.479, 0.491], h 0.050 | loss, 0.413 [0.407, 0.417], h 0.050 | reported, 0.451 [0.448, 0.456] | reported, 0.483 [0.473, 0.532] | win, 0.178 (heap 0.205) | win, 0.761 (heap 0.778) |
| 4 (n 16, rho 64) | loss, 0.510 [0.501, 0.532], h 0.050 | loss, 0.485 [0.482, 0.498], h 0.050 | reported, 0.509 [0.501, 0.514] | reported, 0.648 [0.537, 0.744] | win, 0.140 (heap 0.169) | win, 0.685 (heap 0.704) |
| 5 (n 32, rho 1) | loss, 0.403 [0.398, 0.413], h 0.050 | loss, 0.403 [0.395, 0.407], h 0.050 | reported, 0.406 [0.398, 0.412] | reported, 0.498 [0.492, 0.594] | win, 0.747 (heap 0.751) | loss, 1.166 (heap 1.169) |
| 6 (n 32, rho 4) | loss, 0.471 [0.464, 0.483], h 0.050 | loss, 0.403 [0.397, 0.408], h 0.050 | reported, 0.447 [0.441, 0.453] | reported, 0.499 [0.489, 0.670] | win, 0.388 (heap 0.395) | tie, 0.926 (heap 0.930) |
| 7 (n 32, rho 16) | loss, 0.548 [0.538, 0.557], h 0.050 | loss, 0.440 [0.435, 0.446], h 0.050 | reported, 0.501 [0.496, 0.507] | reported, 0.541 [0.487, 0.657] | win, 0.168 (heap 0.175) | win, 0.771 (heap 0.775) |
| 8 (n 32, rho 64) | loss, 0.477 [0.472, 0.477], h 0.058 | loss, 0.367 [0.363, 0.370], h 0.050 | reported, 0.434 [0.428, 0.436] | reported, 0.575 [0.502, 0.672] | win, 0.138 (heap 0.145) | win, 0.698 (heap 0.703) |
| 9 (n 64, rho 1) | loss, 0.506 [0.500, 0.509], h 0.050 | loss, 0.465 [0.461, 0.467], h 0.050 | reported, 0.492 [0.480, 0.494] | reported, 0.578 [0.539, 0.783] | win, 0.683 (heap 0.685) | loss, 1.121 (heap 1.121) |
| 10 (n 64, rho 4) | loss, 0.593 [0.589, 0.598], h 0.050 | loss, 0.468 [0.465, 0.469], h 0.050 | reported, 0.540 [0.538, 0.544] | reported, 0.592 [0.560, 0.628] | win, 0.373 (heap 0.375) | tie, 0.913 (heap 0.914) |
| 11 (n 64, rho 16) | loss, 0.651 [0.647, 0.652], h 0.050 | loss, 0.462 [0.459, 0.465], h 0.050 | reported, 0.566 [0.561, 0.568] | reported, 0.569 [0.544, 0.736] | win, 0.284 (heap 0.286) | win, 0.852 (heap 0.853) |
| 12 (n 64, rho 64) | loss, 0.667 [0.661, 0.668], h 0.050 | loss, 0.467 [0.465, 0.472], h 0.050 | reported, 0.571 [0.567, 0.574] | reported, 0.593 [0.568, 0.692] | win, 0.136 (heap 0.138) | win, 0.726 (heap 0.728) |
| 13 (n 128, rho 1) | loss, 0.545 [0.543, 0.547], h 0.050 | loss, 0.483 [0.481, 0.484], h 0.050 | reported, 0.521 [0.519, 0.523] | reported, 0.623 [0.604, 0.637] | win, 0.560 (heap 0.561) | loss, 1.282 (heap 1.282) |
| 14 (n 128, rho 4) | loss, 0.625 [0.621, 0.628], h 0.050 | loss, 0.488 [0.485, 0.492], h 0.050 | reported, 0.565 [0.562, 0.571] | reported, 0.613 [0.600, 0.712] | win, 0.358 (heap 0.359) | loss, 1.146 (heap 1.147) |
| 15 (n 128, rho 16) | loss, 0.679 [0.677, 0.681], h 0.050 | loss, 0.475 [0.474, 0.476], h 0.050 | reported, 0.585 [0.583, 0.587] | reported, 0.625 [0.595, 0.775] | win, 0.281 (heap 0.281) | tie, 1.095 (heap 1.095) |
| 16 (n 128, rho 64) | loss, 0.689 [0.687, 0.691], h 0.050 | loss, 0.474 [0.473, 0.476], h 0.050 | reported, 0.590 [0.588, 0.593] | reported, 0.623 [0.596, 0.666] | win, 0.259 (heap 0.259) | tie, 1.072 (heap 1.073) |

### Hash-V-pack against Hash-V-fixed (pack against fixed)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.679 [0.667, 0.693], h 0.050 | loss, 0.860 [0.853, 0.863], h 0.050 | reported, 0.758 [0.750, 0.769] | reported, 0.790 [0.776, 0.797] | win, 0.358 (heap 0.368) | win, 0.582 (heap 0.590) |
| 2 (n 16, rho 4) | loss, 0.736 [0.725, 0.752], h 0.050 | loss, 0.862 [0.848, 0.870], h 0.050 | reported, 0.793 [0.786, 0.812] | reported, 0.788 [0.770, 0.831] | win, 0.131 (heap 0.143) | win, 0.418 (heap 0.428) |
| 3 (n 16, rho 16) | loss, 0.779 [0.773, 0.790], h 0.050 | loss, 0.890 [0.872, 0.897], h 0.050 | reported, 0.830 [0.824, 0.840] | reported, 0.800 [0.784, 0.811] | win, 0.079 (heap 0.092) | win, 0.380 (heap 0.391) |
| 4 (n 16, rho 64) | loss, 0.792 [0.788, 0.798], h 0.050 | loss, 0.909 [0.904, 0.914], h 0.050 | reported, 0.850 [0.845, 0.855] | reported, 0.883 [0.870, 0.905] | win, 0.062 (heap 0.075) | win, 0.342 (heap 0.354) |
| 5 (n 32, rho 1) | loss, 0.647 [0.642, 0.650], h 0.050 | loss, 0.884 [0.871, 0.896], h 0.050 | reported, 0.737 [0.732, 0.742] | reported, 0.788 [0.780, 0.797] | win, 0.332 (heap 0.334) | win, 0.560 (heap 0.562) |
| 6 (n 32, rho 4) | loss, 0.782 [0.779, 0.786], h 0.050 | loss, 0.862 [0.855, 0.868], h 0.050 | reported, 0.813 [0.809, 0.819] | reported, 0.783 [0.776, 0.809] | win, 0.173 (heap 0.176) | win, 0.445 (heap 0.447) |
| 7 (n 32, rho 16) | loss, 0.821 [0.817, 0.826], h 0.050 | loss, 0.885 [0.878, 0.892], h 0.050 | reported, 0.851 [0.844, 0.854] | reported, 0.775 [0.765, 0.787] | win, 0.075 (heap 0.078) | win, 0.385 (heap 0.388) |
| 8 (n 32, rho 64) | loss, 0.792 [0.785, 0.793], h 0.058 | loss, 0.917 [0.911, 0.923], h 0.050 | reported, 0.839 [0.834, 0.845] | reported, 0.773 [0.759, 0.782] | win, 0.061 (heap 0.065) | win, 0.349 (heap 0.352) |
| 9 (n 64, rho 1) | loss, 0.729 [0.720, 0.732], h 0.050 | loss, 0.824 [0.819, 0.828], h 0.050 | reported, 0.768 [0.752, 0.772] | reported, 0.862 [0.803, 0.905] | win, 0.304 (heap 0.304) | win, 0.560 (heap 0.561) |
| 10 (n 64, rho 4) | loss, 0.854 [0.849, 0.857], h 0.050 | loss, 0.831 [0.824, 0.835], h 0.050 | reported, 0.842 [0.838, 0.848] | reported, 0.871 [0.828, 0.923] | win, 0.166 (heap 0.167) | win, 0.457 (heap 0.457) |
| 11 (n 64, rho 16) | loss, 0.886 [0.880, 0.889], h 0.050 | loss, 0.824 [0.822, 0.828], h 0.050 | reported, 0.859 [0.855, 0.862] | reported, 0.839 [0.803, 0.879] | win, 0.126 (heap 0.127) | win, 0.426 (heap 0.427) |
| 12 (n 64, rho 64) | loss, 0.896 [0.892, 0.899], h 0.050 | loss, 0.870 [0.865, 0.874], h 0.050 | reported, 0.885 [0.878, 0.888] | reported, 0.846 [0.825, 0.881] | win, 0.060 (heap 0.061) | win, 0.349 (heap 0.349) |
| 13 (n 128, rho 1) | loss, 0.793 [0.790, 0.798], h 0.050 | loss, 0.901 [0.890, 0.920], h 0.050 | reported, 0.839 [0.834, 0.852] | reported, 1.022 [0.991, 1.061] | win, 0.224 (heap 0.224) | win, 0.570 (heap 0.570) |
| 14 (n 128, rho 4) | loss, 0.867 [0.861, 0.872], h 0.050 | loss, 0.862 [0.855, 0.875], h 0.050 | reported, 0.867 [0.862, 0.874] | reported, 0.974 [0.950, 1.006] | win, 0.143 (heap 0.144) | win, 0.509 (heap 0.510) |
| 15 (n 128, rho 16) | loss, 0.899 [0.896, 0.903], h 0.050 | loss, 0.857 [0.852, 0.859], h 0.050 | reported, 0.879 [0.876, 0.883] | reported, 0.941 [0.909, 0.999] | win, 0.112 (heap 0.113) | win, 0.469 (heap 0.469) |
| 16 (n 128, rho 64) | loss, 0.914 [0.912, 0.915], h 0.050 | loss, 0.857 [0.854, 0.861], h 0.050 | reported, 0.888 [0.885, 0.890] | reported, 0.999 [0.954, 1.038] | win, 0.103 (heap 0.104) | win, 0.477 (heap 0.477) |

### Hash-V-grow-map against Hash-V-fixed (grow-map against fixed)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.348 [0.346, 0.356], h 0.050 | loss, 0.613 [0.610, 0.618], h 0.050 | reported, 0.453 [0.450, 0.457] | reported, 0.928 [0.917, 0.937] | tie, 1.000 (heap 1.000) | tie, 1.040 (heap 1.040) |
| 2 (n 16, rho 4) | loss, 0.642 [0.632, 0.658], h 0.050 | loss, 0.666 [0.657, 0.670], h 0.050 | reported, 0.657 [0.651, 0.672] | reported, 0.901 [0.891, 0.936] | win, 0.333 (heap 0.342) | win, 0.560 (heap 0.567) |
| 3 (n 16, rho 16) | loss, 0.823 [0.811, 0.838], h 0.050 | loss, 0.814 [0.796, 0.822], h 0.050 | reported, 0.818 [0.804, 0.828] | reported, 0.974 [0.963, 0.983] | win, 0.167 (heap 0.177) | win, 0.312 (heap 0.323) |
| 4 (n 16, rho 64) | loss, 0.841 [0.838, 0.847], h 0.050 | loss, 0.932 [0.920, 0.943], h 0.050 | reported, 0.885 [0.881, 0.895] | reported, 0.997 [0.987, 1.022] | win, 0.125 (heap 0.136) | win, 0.154 (heap 0.166) |
| 5 (n 32, rho 1) | loss, 0.564 [0.558, 0.567], h 0.050 | loss, 0.633 [0.628, 0.641], h 0.050 | reported, 0.597 [0.595, 0.602] | reported, 0.979 [0.962, 0.983] | win, 0.556 (heap 0.557) | win, 0.720 (heap 0.721) |
| 6 (n 32, rho 4) | loss, 0.691 [0.687, 0.695], h 0.050 | loss, 0.632 [0.627, 0.634], h 0.050 | reported, 0.666 [0.663, 0.669] | reported, 0.932 [0.912, 0.958] | win, 0.333 (heap 0.335) | win, 0.560 (heap 0.562) |
| 7 (n 32, rho 16) | loss, 0.772 [0.768, 0.777], h 0.050 | loss, 0.746 [0.740, 0.752], h 0.050 | reported, 0.767 [0.759, 0.769] | reported, 0.967 [0.960, 0.981] | win, 0.167 (heap 0.169) | win, 0.312 (heap 0.315) |
| 8 (n 32, rho 64) | loss, 0.762 [0.754, 0.770], h 0.058 | loss, 0.826 [0.819, 0.832], h 0.050 | reported, 0.791 [0.784, 0.799] | reported, 0.945 [0.928, 0.969] | win, 0.125 (heap 0.128) | win, 0.208 (heap 0.211) |
| 9 (n 64, rho 1) | loss, 0.627 [0.625, 0.631], h 0.050 | loss, 0.747 [0.742, 0.751], h 0.050 | reported, 0.676 [0.674, 0.679] | reported, 0.956 [0.948, 0.982] | win, 0.556 (heap 0.556) | win, 0.604 (heap 0.605) |
| 10 (n 64, rho 4) | loss, 0.804 [0.799, 0.807], h 0.050 | loss, 0.750 [0.746, 0.757], h 0.050 | reported, 0.780 [0.777, 0.786] | reported, 0.976 [0.946, 0.985] | win, 0.222 (heap 0.223) | win, 0.354 (heap 0.355) |
| 11 (n 64, rho 16) | loss, 0.855 [0.850, 0.858], h 0.050 | loss, 0.753 [0.751, 0.756], h 0.050 | reported, 0.808 [0.805, 0.811] | reported, 0.980 [0.965, 0.990] | win, 0.167 (heap 0.167) | win, 0.312 (heap 0.313) |
| 12 (n 64, rho 64) | loss, 0.887 [0.881, 0.894], h 0.050 | loss, 0.792 [0.788, 0.794], h 0.050 | reported, 0.844 [0.841, 0.847] | reported, 0.959 [0.946, 0.983] | win, 0.125 (heap 0.126) | win, 0.270 (heap 0.271) |
| 13 (n 128, rho 1) | loss, 0.669 [0.665, 0.673], h 0.050 | loss, 0.808 [0.802, 0.825], h 0.050 | reported, 0.726 [0.723, 0.736] | reported, 1.082 [1.061, 1.115] | win, 0.600 (heap 0.600) | win, 0.667 (heap 0.667) |
| 14 (n 128, rho 4) | loss, 0.826 [0.819, 0.830], h 0.050 | loss, 0.781 [0.774, 0.788], h 0.050 | reported, 0.809 [0.801, 0.813] | reported, 1.033 [1.009, 1.066] | win, 0.300 (heap 0.300) | win, 0.444 (heap 0.445) |
| 15 (n 128, rho 16) | loss, 0.862 [0.855, 0.867], h 0.050 | loss, 0.791 [0.788, 0.793], h 0.050 | reported, 0.830 [0.827, 0.834] | reported, 1.033 [1.021, 1.062] | win, 0.225 (heap 0.225) | win, 0.375 (heap 0.375) |
| 16 (n 128, rho 64) | loss, 0.921 [0.917, 0.923], h 0.050 | loss, 0.766 [0.765, 0.769], h 0.050 | reported, 0.847 [0.844, 0.848] | reported, 1.063 [1.042, 1.088] | win, 0.212 (heap 0.213) | win, 0.380 (heap 0.380) |

### Hash-V-pack against Hash-V-grow-map (pack against grow-map)

| job | 1 build time of one input matrix | 2 addition time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 7 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 1.950 [1.908, 1.968] | reported, 1.402 [1.389, 1.405] | reported, 1.675 [1.657, 1.692] | reported, 0.851 [0.833, 0.864] | reported, 0.358 (heap 0.368) | reported, 0.560 (heap 0.567) |
| 2 (n 16, rho 4) | reported, 1.146 [1.128, 1.163] | reported, 1.295 [1.287, 1.311] | reported, 1.207 [1.197, 1.217] | reported, 0.875 [0.857, 0.898] | reported, 0.392 (heap 0.419) | reported, 0.746 (heap 0.755) |
| 3 (n 16, rho 16) | reported, 0.947 [0.935, 0.962] | reported, 1.093 [1.082, 1.104] | reported, 1.016 [1.008, 1.030] | reported, 0.821 [0.805, 0.834] | reported, 0.474 (heap 0.517) | reported, 1.217 (heap 1.211) |
| 4 (n 16, rho 64) | reported, 0.941 [0.935, 0.946] | reported, 0.975 [0.965, 0.985] | reported, 0.960 [0.949, 0.963] | reported, 0.885 [0.871, 0.895] | reported, 0.497 (heap 0.553) | reported, 2.229 (heap 2.125) |
| 5 (n 32, rho 1) | reported, 1.147 [1.142, 1.153] | reported, 1.396 [1.379, 1.411] | reported, 1.235 [1.224, 1.243] | reported, 0.805 [0.801, 0.821] | reported, 0.597 (heap 0.600) | reported, 0.777 (heap 0.779) |
| 6 (n 32, rho 4) | reported, 1.132 [1.126, 1.140] | reported, 1.365 [1.358, 1.375] | reported, 1.222 [1.216, 1.229] | reported, 0.841 [0.820, 0.868] | reported, 0.518 (heap 0.524) | reported, 0.794 (heap 0.796) |
| 7 (n 32, rho 16) | reported, 1.063 [1.057, 1.068] | reported, 1.186 [1.176, 1.196] | reported, 1.110 [1.107, 1.117] | reported, 0.802 [0.790, 0.811] | reported, 0.448 (heap 0.460) | reported, 1.233 (heap 1.232) |
| 8 (n 32, rho 64) | reported, 1.040 [1.028, 1.045] | reported, 1.110 [1.102, 1.120] | reported, 1.060 [1.054, 1.066] | reported, 0.818 [0.799, 0.833] | reported, 0.491 (heap 0.506) | reported, 1.676 (heap 1.665) |
| 9 (n 64, rho 1) | reported, 1.162 [1.149, 1.167] | reported, 1.104 [1.096, 1.109] | reported, 1.136 [1.113, 1.142] | reported, 0.902 [0.842, 0.935] | reported, 0.547 (heap 0.548) | reported, 0.927 (heap 0.928) |
| 10 (n 64, rho 4) | reported, 1.063 [1.058, 1.068] | reported, 1.108 [1.100, 1.112] | reported, 1.079 [1.073, 1.085] | reported, 0.893 [0.847, 0.958] | reported, 0.747 (heap 0.748) | reported, 1.289 (heap 1.289) |
| 11 (n 64, rho 16) | reported, 1.037 [1.031, 1.040] | reported, 1.094 [1.090, 1.102] | reported, 1.063 [1.055, 1.065] | reported, 0.857 [0.814, 0.907] | reported, 0.758 (heap 0.759) | reported, 1.363 (heap 1.362) |
| 12 (n 64, rho 64) | reported, 1.010 [1.002, 1.015] | reported, 1.099 [1.093, 1.104] | reported, 1.048 [1.041, 1.052] | reported, 0.882 [0.855, 0.917] | reported, 0.483 (heap 0.486) | reported, 1.292 (heap 1.291) |
| 13 (n 128, rho 1) | reported, 1.185 [1.181, 1.192] | reported, 1.115 [1.108, 1.119] | reported, 1.156 [1.149, 1.161] | reported, 0.945 [0.922, 0.970] | reported, 0.374 (heap 0.374) | reported, 0.855 (heap 0.855) |
| 14 (n 128, rho 4) | reported, 1.049 [1.044, 1.059] | reported, 1.103 [1.099, 1.117] | reported, 1.072 [1.066, 1.085] | reported, 0.943 [0.926, 0.964] | reported, 0.478 (heap 0.478) | reported, 1.146 (heap 1.146) |
| 15 (n 128, rho 16) | reported, 1.043 [1.038, 1.051] | reported, 1.083 [1.078, 1.085] | reported, 1.059 [1.055, 1.063] | reported, 0.911 [0.880, 0.954] | reported, 0.499 (heap 0.500) | reported, 1.251 (heap 1.251) |
| 16 (n 128, rho 64) | reported, 0.993 [0.990, 0.996] | reported, 1.119 [1.114, 1.121] | reported, 1.048 [1.045, 1.054] | reported, 0.940 [0.898, 0.967] | reported, 0.487 (heap 0.487) | reported, 1.256 (heap 1.255) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m2 item 7); the counting runs' per-operation records of every hash row equal TC's, every step; Hash-V-pack and Hash-V-grow-map take Hash-V-fixed's arithmetic counters and store splits (spec v0.2 §15.7 items 3-4).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | G(Hash-V-pack)/G(TC), call for call | pack vs fixed | grow-map vs fixed |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 42.89 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 2 | 41.56 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 3 | 43.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 4 | 47.53 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 5 | 42.24 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 6 | 42.47 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 7 | 43.54 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 8 | 38.81 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 9 | 42.22 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 10 | 41.94 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 11 | 41.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 12 | 42.82 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 13 | 42.13 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 14 | 42.16 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 15 | 41.94 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 16 | 41.14 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m2 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.655 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.496 | 0 |
| NONE | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| NONE | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| NONE | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.393 | 0 |
| NONE | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.355 | 0 |
| NONE | Hash-V-grow-map vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 1.375 | 1 |
| NONE | Hash-V-grow-map vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.500 | 1 |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.454 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.434 | 0 |
| NONE | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.715 | 0 |
| NONE | Hash-V-pack vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.190 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.704 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.872 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.318 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.565 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.595 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.690 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | stable | 3 | 1 | 0 | 0 | 0 | 0 | 0.578 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | peak | 3 | 1 | 0 | 0 | 0 | 0 | 0.693 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.174 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.256 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.460 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.816 | 0 |
| LOW | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.649 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.515 | 0 |
| LOW | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| LOW | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.083 | 4 |
| LOW | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.487 | 0 |
| LOW | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.358 | 0 |
| LOW | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.750 | 0 |
| LOW | Hash-V-grow-map vs TC | peak | 1 | 1 | 2 | 0 | 0 | 0 | 1.083 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.532 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.435 | 0 |
| LOW | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.366 | 0 |
| LOW | Hash-V-pack vs TC | peak | 1 | 2 | 1 | 0 | 0 | 0 | 0.920 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.818 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.862 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.155 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.451 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.747 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.708 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.502 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.098 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.202 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.498 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.970 | 0 |
| MID | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.701 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.526 | 0 |
| MID | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| MID | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.000 | 4 |
| MID | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.572 | 0 |
| MID | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.400 | 0 |
| MID | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.375 | 0 |
| MID | Hash-V-grow-map vs TC | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.625 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.600 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.451 | 0 |
| MID | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.229 | 0 |
| MID | Hash-V-pack vs TC | peak | 3 | 1 | 0 | 0 | 0 | 0 | 0.811 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.854 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.871 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.096 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.406 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.839 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.772 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.167 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.312 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.040 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.094 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.487 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 1.242 | 0 |
| HIGH | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.694 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.535 | 0 |
| HIGH | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| HIGH | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.042 | 4 |
| HIGH | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.601 | 0 |
| HIGH | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.424 | 0 |
| HIGH | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.281 | 0 |
| HIGH | Hash-V-grow-map vs TC | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.490 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.588 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.471 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.139 | 0 |
| HIGH | Hash-V-pack vs TC | peak | 3 | 1 | 0 | 0 | 0 | 0 | 0.712 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.844 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.062 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.349 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.864 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.809 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.125 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.239 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.001 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.105 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.489 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 1.484 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-fixed against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.561 | 0.604 | 0.623 | 0.645 |
| 32 | 0.623 | 0.602 | 0.668 | 0.602 |
| 64 | 0.694 | 0.694 | 0.734 | 0.744 |
| 128 | 0.687 | 0.721 | 0.755 | 0.754 |

Hash-V-fixed against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.434 | 0.422 | 0.464 | 0.533 |
| 32 | 0.455 | 0.467 | 0.497 | 0.400 |
| 64 | 0.564 | 0.562 | 0.561 | 0.537 |
| 128 | 0.536 | 0.566 | 0.555 | 0.553 |

Hash-V-fixed against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 2.250 | 2.250 | 2.250 |
| 32 | 2.250 | 2.250 | 2.250 | 2.250 |
| 64 | 2.250 | 2.250 | 2.250 | 2.250 |
| 128 | 2.500 | 2.500 | 2.500 | 2.500 |

Hash-V-fixed against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.083 | 2.083 | 2.000 | 2.000 |
| 32 | 2.083 | 2.083 | 2.000 | 2.000 |
| 64 | 2.000 | 2.000 | 2.000 | 2.083 |
| 128 | 2.250 | 2.250 | 2.333 | 2.250 |

Hash-V-grow-map against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.195 | 0.388 | 0.513 | 0.542 |
| 32 | 0.352 | 0.416 | 0.516 | 0.459 |
| 64 | 0.435 | 0.558 | 0.628 | 0.660 |
| 128 | 0.460 | 0.595 | 0.651 | 0.694 |

Hash-V-grow-map against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.266 | 0.281 | 0.378 | 0.497 |
| 32 | 0.288 | 0.295 | 0.371 | 0.330 |
| 64 | 0.421 | 0.422 | 0.422 | 0.425 |
| 128 | 0.433 | 0.442 | 0.439 | 0.424 |

Hash-V-grow-map against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 0.750 | 0.375 | 0.281 |
| 32 | 1.250 | 0.750 | 0.375 | 0.281 |
| 64 | 1.250 | 0.500 | 0.375 | 0.281 |
| 128 | 1.500 | 0.750 | 0.562 | 0.531 |

Hash-V-grow-map against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.167 | 1.167 | 0.625 | 0.307 |
| 32 | 1.500 | 1.167 | 0.625 | 0.417 |
| 64 | 1.208 | 0.708 | 0.625 | 0.562 |
| 128 | 1.500 | 1.000 | 0.875 | 0.854 |

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.381 | 0.445 | 0.486 | 0.510 |
| 32 | 0.403 | 0.471 | 0.548 | 0.477 |
| 64 | 0.506 | 0.593 | 0.651 | 0.667 |
| 128 | 0.545 | 0.625 | 0.679 | 0.689 |

Hash-V-pack against TC, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.374 | 0.364 | 0.413 | 0.485 |
| 32 | 0.403 | 0.403 | 0.440 | 0.367 |
| 64 | 0.465 | 0.468 | 0.462 | 0.467 |
| 128 | 0.483 | 0.488 | 0.475 | 0.474 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.806 | 0.294 | 0.178 | 0.140 |
| 32 | 0.747 | 0.388 | 0.168 | 0.138 |
| 64 | 0.683 | 0.373 | 0.284 | 0.136 |
| 128 | 0.560 | 0.358 | 0.281 | 0.259 |

Hash-V-pack against TC, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.213 | 0.871 | 0.761 | 0.685 |
| 32 | 1.166 | 0.926 | 0.771 | 0.698 |
| 64 | 1.121 | 0.913 | 0.852 | 0.726 |
| 128 | 1.282 | 1.146 | 1.095 | 1.072 |

Hash-V-pack against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.679 | 0.736 | 0.779 | 0.792 |
| 32 | 0.647 | 0.782 | 0.821 | 0.792 |
| 64 | 0.729 | 0.854 | 0.886 | 0.896 |
| 128 | 0.793 | 0.867 | 0.899 | 0.914 |

Hash-V-pack against Hash-V-fixed, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.860 | 0.862 | 0.890 | 0.909 |
| 32 | 0.884 | 0.862 | 0.885 | 0.917 |
| 64 | 0.824 | 0.831 | 0.824 | 0.870 |
| 128 | 0.901 | 0.862 | 0.857 | 0.857 |

Hash-V-pack against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.358 | 0.131 | 0.079 | 0.062 |
| 32 | 0.332 | 0.173 | 0.075 | 0.061 |
| 64 | 0.304 | 0.166 | 0.126 | 0.060 |
| 128 | 0.224 | 0.143 | 0.112 | 0.103 |

Hash-V-pack against Hash-V-fixed, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.582 | 0.418 | 0.380 | 0.342 |
| 32 | 0.560 | 0.445 | 0.385 | 0.349 |
| 64 | 0.560 | 0.457 | 0.426 | 0.349 |
| 128 | 0.570 | 0.509 | 0.469 | 0.477 |

Hash-V-grow-map against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.348 | 0.642 | 0.823 | 0.841 |
| 32 | 0.564 | 0.691 | 0.772 | 0.762 |
| 64 | 0.627 | 0.804 | 0.855 | 0.887 |
| 128 | 0.669 | 0.826 | 0.862 | 0.921 |

Hash-V-grow-map against Hash-V-fixed, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.613 | 0.666 | 0.814 | 0.932 |
| 32 | 0.633 | 0.632 | 0.746 | 0.826 |
| 64 | 0.747 | 0.750 | 0.753 | 0.792 |
| 128 | 0.808 | 0.781 | 0.791 | 0.766 |

Hash-V-grow-map against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.000 | 0.333 | 0.167 | 0.125 |
| 32 | 0.556 | 0.333 | 0.167 | 0.125 |
| 64 | 0.556 | 0.222 | 0.167 | 0.125 |
| 128 | 0.600 | 0.300 | 0.225 | 0.212 |

Hash-V-grow-map against Hash-V-fixed, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.040 | 0.560 | 0.312 | 0.154 |
| 32 | 0.720 | 0.560 | 0.312 | 0.208 |
| 64 | 0.604 | 0.354 | 0.312 | 0.270 |
| 128 | 0.667 | 0.444 | 0.375 | 0.380 |

Hash-V-pack against Hash-V-grow-map, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.950 | 1.146 | 0.947 | 0.941 |
| 32 | 1.147 | 1.132 | 1.063 | 1.040 |
| 64 | 1.162 | 1.063 | 1.037 | 1.010 |
| 128 | 1.185 | 1.049 | 1.043 | 0.993 |

Hash-V-pack against Hash-V-grow-map, 2 addition time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.402 | 1.295 | 1.093 | 0.975 |
| 32 | 1.396 | 1.365 | 1.186 | 1.110 |
| 64 | 1.104 | 1.108 | 1.094 | 1.099 |
| 128 | 1.115 | 1.103 | 1.083 | 1.119 |

Hash-V-pack against Hash-V-grow-map, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.358 | 0.392 | 0.474 | 0.497 |
| 32 | 0.597 | 0.518 | 0.448 | 0.491 |
| 64 | 0.547 | 0.747 | 0.758 | 0.483 |
| 128 | 0.374 | 0.478 | 0.499 | 0.487 |

Hash-V-pack against Hash-V-grow-map, 7 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.560 | 0.746 | 1.217 | 2.229 |
| 32 | 0.777 | 0.794 | 1.233 | 1.676 |
| 64 | 0.927 | 1.289 | 1.363 | 1.292 |
| 128 | 0.855 | 1.146 | 1.251 | 1.256 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (3 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build, over that input's cells; Hash-V-grow-map's growth peak: the job's total at its largest growth, over the operation's cells.

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | growth peak |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 1 | Hash-V-fixed | 36.00 (36.47) | 30.67 (31.14) | 33.33 (33.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 1 | Hash-V-grow-map | 36.00 (36.47) | 30.67 (31.14) | 34.67 (35.16) | 36.00 (56.00) | 36.00 (56.00) | 34.67 (35.16), 18 growths |
| 1 | Hash-V-pack | 12.89 (13.41) | 11.41 (11.93) | 19.41 (19.95) | 12.91 (52.91) | 12.88 (52.88) | - |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 2 | Hash-V-fixed | 36.00 (36.47) | 30.67 (31.14) | 33.33 (33.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 2 | Hash-V-grow-map | 12.00 (12.47) | 14.67 (15.14) | 18.67 (19.16) | 12.00 (17.00) | 12.00 (17.00) | 18.67 (19.16), 14 growths |
| 2 | Hash-V-pack | 4.70 (5.22) | 5.93 (6.45) | 13.93 (14.47) | 4.72 (46.72) | 4.69 (46.69) | - |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 3 | Hash-V-fixed | 36.00 (36.47) | 30.67 (31.14) | 32.00 (32.48) | 36.00 (36.00) | 36.00 (36.00) | - |
| 3 | Hash-V-grow-map | 6.00 (6.47) | 8.00 (8.47) | 10.00 (10.49) | 6.00 (7.25) | 6.00 (7.25) | 10.00 (10.49), 9 growths |
| 3 | Hash-V-pack | 2.84 (3.34) | 4.17 (4.68) | 12.17 (12.70) | 2.84 (44.84) | 2.84 (44.84) | - |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 4 | Hash-V-fixed | 36.00 (36.47) | 30.67 (31.14) | 32.00 (32.48) | 36.00 (36.00) | 36.00 (36.00) | - |
| 4 | Hash-V-grow-map | 4.50 (4.97) | 4.67 (5.14) | 4.92 (5.41) | 4.50 (4.50) | 4.50 (4.50) | 4.92 (5.41), 2 growths |
| 4 | Hash-V-pack | 2.23 (2.75) | 2.29 (2.80) | 10.96 (11.49) | 2.22 (44.22) | 2.25 (44.25) | - |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 5 | Hash-V-fixed | 36.00 (36.12) | 30.67 (30.78) | 33.33 (33.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 5 | Hash-V-grow-map | 20.00 (20.12) | 20.00 (20.12) | 24.00 (24.12) | 20.00 (30.00) | 20.00 (30.00) | 24.00 (24.12), 22 growths |
| 5 | Hash-V-pack | 11.95 (12.08) | 10.66 (10.79) | 18.66 (18.79) | 11.98 (51.98) | 11.91 (51.91) | - |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 6 | Hash-V-fixed | 36.00 (36.12) | 30.67 (30.78) | 33.33 (33.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 6 | Hash-V-grow-map | 12.00 (12.12) | 14.67 (14.78) | 18.67 (18.79) | 12.00 (17.00) | 12.00 (17.00) | 18.67 (18.79), 20 growths |
| 6 | Hash-V-pack | 6.21 (6.34) | 6.82 (6.95) | 14.82 (14.96) | 6.21 (46.21) | 6.22 (46.22) | - |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 7 | Hash-V-fixed | 36.00 (36.12) | 30.67 (30.78) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | - |
| 7 | Hash-V-grow-map | 6.00 (6.12) | 8.00 (8.12) | 10.00 (10.12) | 6.00 (7.25) | 6.00 (7.25) | 10.00 (10.12), 15 growths |
| 7 | Hash-V-pack | 2.69 (2.81) | 4.33 (4.46) | 12.33 (12.47) | 2.71 (44.71) | 2.66 (44.66) | - |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 8 | Hash-V-fixed | 36.00 (36.12) | 30.67 (30.78) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | - |
| 8 | Hash-V-grow-map | 4.50 (4.62) | 5.67 (5.78) | 6.67 (6.79) | 4.50 (4.81) | 4.50 (4.81) | 6.67 (6.79), 10 growths |
| 8 | Hash-V-pack | 2.21 (2.34) | 3.17 (3.30) | 11.17 (11.30) | 2.21 (44.21) | 2.21 (44.21) | - |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 9 | Hash-V-fixed | 36.00 (36.03) | 30.67 (30.70) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | - |
| 9 | Hash-V-grow-map | 20.00 (20.03) | 17.33 (17.36) | 19.33 (19.36) | 20.00 (30.00) | 20.00 (30.00) | 19.33 (19.36), 27 growths |
| 9 | Hash-V-pack | 10.94 (10.97) | 9.93 (9.96) | 17.93 (17.96) | 10.95 (50.95) | 10.92 (50.92) | - |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 10 | Hash-V-fixed | 36.00 (36.03) | 30.67 (30.70) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | - |
| 10 | Hash-V-grow-map | 8.00 (8.03) | 9.33 (9.36) | 11.33 (11.36) | 8.00 (10.50) | 8.00 (10.50) | 11.33 (11.36), 23 growths |
| 10 | Hash-V-pack | 5.97 (6.00) | 6.61 (6.65) | 14.61 (14.65) | 5.98 (45.98) | 5.97 (45.97) | - |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 11 | Hash-V-fixed | 36.00 (36.03) | 30.67 (30.70) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | - |
| 11 | Hash-V-grow-map | 6.00 (6.03) | 8.00 (8.03) | 10.00 (10.03) | 6.00 (7.25) | 6.00 (7.25) | 10.00 (10.03), 21 growths |
| 11 | Hash-V-pack | 4.55 (4.58) | 5.63 (5.66) | 13.63 (13.66) | 4.54 (44.54) | 4.55 (44.55) | - |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 12 | Hash-V-fixed | 36.00 (36.03) | 30.67 (30.70) | 33.33 (33.36) | 36.00 (36.00) | 36.00 (36.00) | - |
| 12 | Hash-V-grow-map | 4.50 (4.53) | 7.00 (7.03) | 9.00 (9.03) | 4.50 (4.81) | 4.50 (4.81) | 9.00 (9.03), 17 growths |
| 12 | Hash-V-pack | 2.17 (2.20) | 3.62 (3.66) | 11.62 (11.66) | 2.17 (44.17) | 2.17 (44.17) | - |
| 13 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 13 | Hash-V-fixed | 40.00 (40.01) | 34.67 (34.67) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | - |
| 13 | Hash-V-grow-map | 24.00 (24.01) | 21.33 (21.34) | 24.00 (24.01) | 24.00 (36.00) | 24.00 (36.00) | 24.00 (24.01), 33 growths |
| 13 | Hash-V-pack | 8.97 (8.97) | 8.51 (8.52) | 20.51 (20.52) | 8.99 (60.99) | 8.94 (60.94) | - |
| 14 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 14 | Hash-V-fixed | 40.00 (40.01) | 34.67 (34.67) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | - |
| 14 | Hash-V-grow-map | 12.00 (12.01) | 13.33 (13.34) | 16.00 (16.01) | 12.00 (15.00) | 12.00 (15.00) | 16.00 (16.01), 29 growths |
| 14 | Hash-V-pack | 5.73 (5.74) | 6.34 (6.35) | 18.34 (18.35) | 5.73 (57.73) | 5.74 (57.74) | - |
| 15 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 15 | Hash-V-fixed | 40.00 (40.01) | 34.67 (34.67) | 37.33 (37.34) | 40.00 (40.00) | 40.00 (40.00) | - |
| 15 | Hash-V-grow-map | 9.00 (9.01) | 11.33 (11.34) | 14.00 (14.01) | 9.00 (9.75) | 9.00 (9.75) | 14.00 (14.01), 25 growths |
| 15 | Hash-V-pack | 4.49 (4.50) | 5.51 (5.52) | 17.51 (17.52) | 4.49 (56.49) | 4.50 (56.50) | - |
| 16 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 16 | Hash-V-fixed | 40.00 (40.01) | 34.67 (34.67) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | - |
| 16 | Hash-V-grow-map | 8.50 (8.51) | 11.00 (11.01) | 13.67 (13.67) | 8.50 (8.88) | 8.50 (8.88) | 13.67 (13.67), 23 growths |
| 16 | Hash-V-pack | 4.14 (4.15) | 5.16 (5.17) | 17.16 (17.17) | 4.14 (56.14) | 4.14 (56.14) | - |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B |
| --- | --- | --- | --- |
| 1 | TC | 2902 | 2828 |
| 1 | Hash-V-fixed | 5218 | 5077 |
| 1 | Hash-V-grow-map | 14781 | 14397 |
| 1 | Hash-V-pack | 7667 | 7466 |
| 1 | Hash-V-fixed-null | 5115 | 5017 |
| 2 | TC | 2810 | 2809 |
| 2 | Hash-V-fixed | 4661 | 4619 |
| 2 | Hash-V-grow-map | 7287 | 7186 |
| 2 | Hash-V-pack | 6366 | 6256 |
| 2 | Hash-V-fixed-null | 4702 | 4627 |
| 3 | TC | 2836 | 2740 |
| 3 | Hash-V-fixed | 4545 | 4476 |
| 3 | Hash-V-grow-map | 5652 | 5390 |
| 3 | Hash-V-pack | 5822 | 5693 |
| 3 | Hash-V-fixed-null | 4501 | 4348 |
| 4 | TC | 3108 | 3319 |
| 4 | Hash-V-fixed | 4945 | 4889 |
| 4 | Hash-V-grow-map | 5875 | 5831 |
| 4 | Hash-V-pack | 6228 | 6187 |
| 4 | Hash-V-fixed-null | 4950 | 4908 |
| 5 | TC | 20761 | 20761 |
| 5 | Hash-V-fixed | 33578 | 32983 |
| 5 | Hash-V-grow-map | 59400 | 58523 |
| 5 | Hash-V-pack | 51626 | 51358 |
| 5 | Hash-V-fixed-null | 33263 | 32928 |
| 6 | TC | 20183 | 20564 |
| 6 | Hash-V-fixed | 33582 | 33703 |
| 6 | Hash-V-grow-map | 48906 | 48537 |
| 6 | Hash-V-pack | 43428 | 42578 |
| 6 | Hash-V-fixed-null | 33695 | 33778 |
| 7 | TC | 20323 | 20170 |
| 7 | Hash-V-fixed | 30391 | 30279 |
| 7 | Hash-V-grow-map | 39184 | 39324 |
| 7 | Hash-V-pack | 36533 | 37118 |
| 7 | Hash-V-fixed-null | 30684 | 30342 |
| 8 | TC | 14596 | 16517 |
| 8 | Hash-V-fixed | 24560 | 27197 |
| 8 | Hash-V-grow-map | 32696 | 35095 |
| 8 | Hash-V-pack | 31250 | 34551 |
| 8 | Hash-V-fixed-null | 24696 | 27285 |
| 9 | TC | 112730 | 110613 |
| 9 | Hash-V-fixed | 161088 | 160636 |
| 9 | Hash-V-grow-map | 257274 | 256009 |
| 9 | Hash-V-pack | 221512 | 220104 |
| 9 | Hash-V-fixed-null | 161442 | 161571 |
| 10 | TC | 112722 | 111201 |
| 10 | Hash-V-fixed | 161117 | 160714 |
| 10 | Hash-V-grow-map | 201431 | 199624 |
| 10 | Hash-V-pack | 189659 | 186972 |
| 10 | Hash-V-fixed-null | 160502 | 159654 |
| 11 | TC | 108031 | 110509 |
| 11 | Hash-V-fixed | 148087 | 150670 |
| 11 | Hash-V-grow-map | 174532 | 174633 |
| 11 | Hash-V-pack | 167581 | 169181 |
| 11 | Hash-V-fixed-null | 148284 | 151402 |
| 12 | TC | 109358 | 101408 |
| 12 | Hash-V-fixed | 146420 | 139199 |
| 12 | Hash-V-grow-map | 162005 | 157540 |
| 12 | Hash-V-pack | 162810 | 155227 |
| 12 | Hash-V-fixed-null | 146208 | 139271 |
| 13 | TC | 474746 | 474577 |
| 13 | Hash-V-fixed | 690322 | 694202 |
| 13 | Hash-V-grow-map | 1030455 | 1034675 |
| 13 | Hash-V-pack | 873838 | 868931 |
| 13 | Hash-V-fixed-null | 690800 | 690977 |
| 14 | TC | 469632 | 472397 |
| 14 | Hash-V-fixed | 651532 | 656175 |
| 14 | Hash-V-grow-map | 786919 | 795716 |
| 14 | Hash-V-pack | 752352 | 755599 |
| 14 | Hash-V-fixed-null | 653601 | 656197 |
| 15 | TC | 472137 | 476632 |
| 15 | Hash-V-fixed | 623716 | 632283 |
| 15 | Hash-V-grow-map | 727583 | 730895 |
| 15 | Hash-V-pack | 693399 | 701724 |
| 15 | Hash-V-fixed-null | 622339 | 630554 |
| 16 | TC | 462203 | 463749 |
| 16 | Hash-V-fixed | 614662 | 611667 |
| 16 | Hash-V-grow-map | 665776 | 668505 |
| 16 | Hash-V-pack | 672902 | 668926 |
| 16 | Hash-V-fixed-null | 611691 | 609989 |

## Stores, per job and matrix (rule-m2 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. Hash-V-pack's T is its build table's.

| job | row | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | cells bytes | table or list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Hash-V-fixed | A | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.13 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | B | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.08 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | C | 285 | 1024 | 27.8% | 32 | 16 | 227 / 285 | 1.11 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 1 | Hash-V-grow-map | A | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.53 | 10 | 0 (0) | 6 | 504 (605) | 808 | 1024 | 8192 |
| 1 | Hash-V-grow-map | B | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.42 | 13 | 0 (0) | 6 | 504 (571) | 806 | 1024 | 8192 |
| 1 | Hash-V-grow-map | C | 285 | 1024 | 27.8% | 32 | 16 | 227 / 285 | 1.42 | 9 | 2 (32) | 6 | 504 (592) | 796 | 1024 | 4096 |
| 1 | Hash-V-pack | A | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.13 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2280 |
| 1 | Hash-V-pack | B | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.08 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2272 |
| 1 | Hash-V-pack | C | 285 | 1024 | 27.8% | 32 | 16 | 227 / 285 | 1.11 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 1140 |
| 2 | Hash-V-fixed | A | 87 | 1024 | 8.5% | 64 | 16 | 425 / 87 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | B | 86 | 1024 | 8.4% | 64 | 16 | 426 / 86 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | C | 281 | 1024 | 27.4% | 32 | 16 | 231 / 281 | 1.10 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 2 | Hash-V-grow-map | A | 87 | 256 | 34.0% | 64 | 16 | 425 / 87 | 1.18 | 12 | 0 (0) | 4 | 120 (135) | 201 | 1024 | 2048 |
| 2 | Hash-V-grow-map | B | 86 | 256 | 33.6% | 64 | 16 | 426 / 86 | 1.16 | 4 | 0 (0) | 4 | 120 (130) | 203 | 1024 | 2048 |
| 2 | Hash-V-grow-map | C | 281 | 1024 | 27.4% | 32 | 16 | 231 / 281 | 1.42 | 10 | 2 (32) | 6 | 504 (577) | 798 | 1024 | 4096 |
| 2 | Hash-V-pack | A | 87 | 1024 | 8.5% | 64 | 8 | 425 / 87 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 696 |
| 2 | Hash-V-pack | B | 86 | 1024 | 8.4% | 64 | 8 | 426 / 86 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 688 |
| 2 | Hash-V-pack | C | 281 | 1024 | 27.4% | 32 | 16 | 231 / 281 | 1.10 | 5 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 1124 |
| 3 | Hash-V-fixed | A | 27 | 1024 | 2.6% | 64 | 16 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | B | 27 | 1024 | 2.6% | 64 | 16 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | C | 181 | 1024 | 17.7% | 32 | 16 | 331 / 181 | 1.10 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 3 | Hash-V-grow-map | A | 27 | 64 | 42.2% | 64 | 16 | 485 / 27 | 1.40 | 5 | 0 (0) | 2 | 24 (26) | 42 | 1024 | 512 |
| 3 | Hash-V-grow-map | B | 27 | 64 | 42.2% | 64 | 16 | 485 / 27 | 1.12 | 7 | 0 (0) | 2 | 24 (27) | 30 | 1024 | 512 |
| 3 | Hash-V-grow-map | C | 181 | 512 | 35.4% | 32 | 16 | 331 / 181 | 1.31 | 6 | 1 (16) | 5 | 248 (282) | 425 | 1024 | 2048 |
| 3 | Hash-V-pack | A | 27 | 1024 | 2.6% | 64 | 8 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 216 |
| 3 | Hash-V-pack | B | 27 | 1024 | 2.6% | 64 | 8 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 216 |
| 3 | Hash-V-pack | C | 181 | 1024 | 17.7% | 32 | 16 | 331 / 181 | 1.10 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 724 |
| 4 | Hash-V-fixed | A | 7 | 1024 | 0.7% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | B | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | C | 26 | 1024 | 2.5% | 32 | 16 | 486 / 26 | 1.06 | 2 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 4 | Hash-V-grow-map | A | 7 | 16 | 43.8% | 64 | 16 | 505 / 7 | 1.63 | 4 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | B | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.11 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | C | 26 | 64 | 40.6% | 32 | 16 | 486 / 26 | 1.46 | 6 | 1 (16) | 2 | 24 (31) | 41 | 1024 | 256 |
| 4 | Hash-V-pack | A | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 56 |
| 4 | Hash-V-pack | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | C | 26 | 1024 | 2.5% | 32 | 8 | 486 / 26 | 1.06 | 2 | 1 (1024) | 0 | 0 (0) | 0 | 512 | 104 |
| 5 | Hash-V-fixed | A | 1022 | 4096 | 25.0% | 64 | 16 | 1026 / 1022 | 1.07 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | B | 1012 | 4096 | 24.7% | 64 | 16 | 1036 / 1012 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | C | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.12 | 10 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 5 | Hash-V-grow-map | A | 1022 | 2048 | 49.9% | 64 | 16 | 1026 / 1022 | 1.41 | 13 | 0 (0) | 7 | 1016 (1217) | 1786 | 4096 | 16384 |
| 5 | Hash-V-grow-map | B | 1012 | 2048 | 49.4% | 64 | 16 | 1036 / 1012 | 1.40 | 10 | 0 (0) | 7 | 1016 (1157) | 1812 | 4096 | 16384 |
| 5 | Hash-V-grow-map | C | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.45 | 16 | 2 (32) | 8 | 2040 (2409) | 3769 | 4096 | 16384 |
| 5 | Hash-V-pack | A | 1022 | 4096 | 25.0% | 64 | 16 | 1026 / 1022 | 1.07 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8176 |
| 5 | Hash-V-pack | B | 1012 | 4096 | 24.7% | 64 | 16 | 1036 / 1012 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8096 |
| 5 | Hash-V-pack | C | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.12 | 10 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 4176 |
| 6 | Hash-V-fixed | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | C | 1031 | 4096 | 25.2% | 32 | 16 | 1017 / 1031 | 1.10 | 7 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 6 | Hash-V-grow-map | A | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.22 | 9 | 0 (0) | 6 | 504 (565) | 1445 | 4096 | 8192 |
| 6 | Hash-V-grow-map | B | 284 | 1024 | 27.7% | 64 | 16 | 1764 / 284 | 1.24 | 14 | 0 (0) | 6 | 504 (585) | 1492 | 4096 | 8192 |
| 6 | Hash-V-grow-map | C | 1031 | 4096 | 25.2% | 32 | 16 | 1017 / 1031 | 1.45 | 17 | 2 (32) | 8 | 2040 (2380) | 3807 | 4096 | 16384 |
| 6 | Hash-V-pack | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2272 |
| 6 | Hash-V-pack | C | 1031 | 4096 | 25.2% | 32 | 16 | 1017 / 1031 | 1.10 | 7 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 4124 |
| 7 | Hash-V-fixed | A | 91 | 4096 | 2.2% | 64 | 16 | 1957 / 91 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | B | 85 | 4096 | 2.1% | 64 | 16 | 1963 / 85 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | C | 927 | 4096 | 22.6% | 32 | 16 | 1121 / 927 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 7 | Hash-V-grow-map | A | 91 | 256 | 35.5% | 64 | 16 | 1957 / 91 | 1.16 | 7 | 0 (0) | 4 | 120 (133) | 213 | 4096 | 2048 |
| 7 | Hash-V-grow-map | B | 85 | 256 | 33.2% | 64 | 16 | 1963 / 85 | 1.17 | 6 | 0 (0) | 4 | 120 (135) | 192 | 4096 | 2048 |
| 7 | Hash-V-grow-map | C | 927 | 2048 | 45.3% | 32 | 16 | 1121 / 927 | 1.37 | 13 | 1 (16) | 7 | 1016 (1161) | 1850 | 4096 | 8192 |
| 7 | Hash-V-pack | A | 91 | 4096 | 2.2% | 64 | 8 | 1957 / 91 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 728 |
| 7 | Hash-V-pack | B | 85 | 4096 | 2.1% | 64 | 8 | 1963 / 85 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 680 |
| 7 | Hash-V-pack | C | 927 | 4096 | 22.6% | 32 | 16 | 1121 / 927 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 3708 |
| 8 | Hash-V-fixed | A | 27 | 4096 | 0.7% | 64 | 16 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | B | 27 | 4096 | 0.7% | 64 | 16 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | C | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.01 | 3 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 8 | Hash-V-grow-map | A | 27 | 64 | 42.2% | 64 | 16 | 2021 / 27 | 1.35 | 6 | 0 (0) | 2 | 24 (28) | 48 | 4096 | 512 |
| 8 | Hash-V-grow-map | B | 27 | 64 | 42.2% | 64 | 16 | 2021 / 27 | 1.28 | 5 | 0 (0) | 2 | 24 (29) | 45 | 4096 | 512 |
| 8 | Hash-V-grow-map | C | 280 | 1024 | 27.3% | 32 | 16 | 1768 / 280 | 1.19 | 9 | 1 (16) | 6 | 504 (573) | 1614 | 4096 | 4096 |
| 8 | Hash-V-pack | A | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 216 |
| 8 | Hash-V-pack | B | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 216 |
| 8 | Hash-V-pack | C | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.01 | 3 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 1120 |
| 9 | Hash-V-fixed | A | 3559 | 16384 | 21.7% | 64 | 16 | 4633 / 3559 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | B | 3543 | 16384 | 21.6% | 64 | 16 | 4649 / 3543 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | C | 4013 | 16384 | 24.5% | 32 | 16 | 4179 / 4013 | 1.07 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 9 | Hash-V-grow-map | A | 3559 | 8192 | 43.4% | 64 | 16 | 4633 / 3559 | 1.26 | 18 | 0 (0) | 9 | 4088 (4545) | 8288 | 16384 | 65536 |
| 9 | Hash-V-grow-map | B | 3543 | 8192 | 43.2% | 64 | 16 | 4649 / 3543 | 1.26 | 14 | 0 (0) | 9 | 4088 (4529) | 8307 | 16384 | 65536 |
| 9 | Hash-V-grow-map | C | 4013 | 8192 | 49.0% | 32 | 16 | 4179 / 4013 | 1.41 | 23 | 1 (16) | 9 | 4088 (4744) | 7850 | 16384 | 32768 |
| 9 | Hash-V-pack | A | 3559 | 16384 | 21.7% | 64 | 16 | 4633 / 3559 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28472 |
| 9 | Hash-V-pack | B | 3543 | 16384 | 21.6% | 64 | 16 | 4649 / 3543 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28344 |
| 9 | Hash-V-pack | C | 4013 | 16384 | 24.5% | 32 | 16 | 4179 / 4013 | 1.07 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 16052 |
| 10 | Hash-V-fixed | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | B | 1007 | 16384 | 6.1% | 64 | 16 | 7185 / 1007 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | C | 3988 | 16384 | 24.3% | 32 | 16 | 4204 / 3988 | 1.06 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 10 | Hash-V-grow-map | A | 1013 | 2048 | 49.5% | 64 | 16 | 7179 / 1013 | 1.21 | 14 | 0 (0) | 7 | 1016 (1187) | 2293 | 16384 | 16384 |
| 10 | Hash-V-grow-map | B | 1007 | 2048 | 49.2% | 64 | 16 | 7185 / 1007 | 1.23 | 13 | 0 (0) | 7 | 1016 (1154) | 2166 | 16384 | 16384 |
| 10 | Hash-V-grow-map | C | 3988 | 8192 | 48.7% | 32 | 16 | 4204 / 3988 | 1.37 | 29 | 1 (16) | 9 | 4088 (4684) | 7872 | 16384 | 32768 |
| 10 | Hash-V-pack | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8104 |
| 10 | Hash-V-pack | B | 1007 | 16384 | 6.1% | 64 | 16 | 7185 / 1007 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8056 |
| 10 | Hash-V-pack | C | 3988 | 16384 | 24.3% | 32 | 16 | 4204 / 3988 | 1.06 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 15952 |
| 11 | Hash-V-fixed | A | 279 | 16384 | 1.7% | 64 | 16 | 7913 / 279 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | C | 3894 | 16384 | 23.8% | 32 | 16 | 4298 / 3894 | 1.07 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 11 | Hash-V-grow-map | A | 279 | 1024 | 27.2% | 64 | 16 | 7913 / 279 | 1.13 | 7 | 0 (0) | 6 | 504 (565) | 1666 | 16384 | 8192 |
| 11 | Hash-V-grow-map | B | 281 | 1024 | 27.4% | 64 | 16 | 7911 / 281 | 1.11 | 7 | 0 (0) | 6 | 504 (566) | 1520 | 16384 | 8192 |
| 11 | Hash-V-grow-map | C | 3894 | 8192 | 47.5% | 32 | 16 | 4298 / 3894 | 1.38 | 31 | 1 (16) | 9 | 4088 (4661) | 7919 | 16384 | 32768 |
| 11 | Hash-V-pack | A | 279 | 16384 | 1.7% | 64 | 16 | 7913 / 279 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2232 |
| 11 | Hash-V-pack | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2248 |
| 11 | Hash-V-pack | C | 3894 | 16384 | 23.8% | 32 | 16 | 4298 / 3894 | 1.07 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 15576 |
| 12 | Hash-V-fixed | A | 87 | 16384 | 0.5% | 64 | 16 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | B | 89 | 16384 | 0.5% | 64 | 16 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | C | 2588 | 16384 | 15.8% | 32 | 16 | 5604 / 2588 | 1.04 | 5 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 12 | Hash-V-grow-map | A | 87 | 256 | 34.0% | 64 | 16 | 8105 / 87 | 1.11 | 5 | 0 (0) | 4 | 120 (134) | 208 | 16384 | 2048 |
| 12 | Hash-V-grow-map | B | 89 | 256 | 34.8% | 64 | 16 | 8103 / 89 | 1.18 | 9 | 0 (0) | 4 | 120 (136) | 201 | 16384 | 2048 |
| 12 | Hash-V-grow-map | C | 2588 | 8192 | 31.6% | 32 | 16 | 5604 / 2588 | 1.29 | 20 | 2 (32) | 9 | 4088 (4700) | 9738 | 16384 | 32768 |
| 12 | Hash-V-pack | A | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 696 |
| 12 | Hash-V-pack | B | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 712 |
| 12 | Hash-V-pack | C | 2588 | 16384 | 15.8% | 32 | 16 | 5604 / 2588 | 1.04 | 5 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 10352 |
| 13 | Hash-V-fixed | A | 10219 | 65536 | 15.6% | 64 | 32 | 22549 / 10219 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | B | 10123 | 65536 | 15.4% | 64 | 32 | 22645 / 10123 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | C | 14718 | 65536 | 22.5% | 32 | 32 | 18050 / 14718 | 1.04 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 13 | Hash-V-grow-map | A | 10219 | 32768 | 31.2% | 64 | 32 | 22549 / 10219 | 1.13 | 13 | 0 (0) | 11 | 16376 (17349) | 41696 | 131072 | 262144 |
| 13 | Hash-V-grow-map | B | 10123 | 32768 | 30.9% | 64 | 32 | 22645 / 10123 | 1.14 | 15 | 0 (0) | 11 | 16376 (17398) | 41940 | 131072 | 262144 |
| 13 | Hash-V-grow-map | C | 14718 | 32768 | 44.9% | 32 | 32 | 18050 / 14718 | 1.29 | 19 | 1 (16) | 11 | 16376 (18544) | 33724 | 131072 | 131072 |
| 13 | Hash-V-pack | A | 10219 | 65536 | 15.6% | 64 | 16 | 22549 / 10219 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81752 |
| 13 | Hash-V-pack | B | 10123 | 65536 | 15.4% | 64 | 16 | 22645 / 10123 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80984 |
| 13 | Hash-V-pack | C | 14718 | 65536 | 22.5% | 32 | 16 | 18050 / 14718 | 1.04 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58872 |
| 14 | Hash-V-fixed | A | 3541 | 65536 | 5.4% | 64 | 32 | 29227 / 3541 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | B | 3561 | 65536 | 5.4% | 64 | 32 | 29207 / 3561 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | C | 14573 | 65536 | 22.2% | 32 | 32 | 18195 / 14573 | 1.04 | 6 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 14 | Hash-V-grow-map | A | 3541 | 8192 | 43.2% | 64 | 32 | 29227 / 3541 | 1.16 | 16 | 0 (0) | 9 | 4088 (4514) | 10422 | 131072 | 65536 |
| 14 | Hash-V-grow-map | B | 3561 | 8192 | 43.5% | 64 | 32 | 29207 / 3561 | 1.15 | 16 | 0 (0) | 9 | 4088 (4643) | 10327 | 131072 | 65536 |
| 14 | Hash-V-grow-map | C | 14573 | 32768 | 44.5% | 32 | 32 | 18195 / 14573 | 1.29 | 21 | 1 (16) | 11 | 16376 (18646) | 33794 | 131072 | 131072 |
| 14 | Hash-V-pack | A | 3541 | 65536 | 5.4% | 64 | 16 | 29227 / 3541 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28328 |
| 14 | Hash-V-pack | B | 3561 | 65536 | 5.4% | 64 | 16 | 29207 / 3561 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28488 |
| 14 | Hash-V-pack | C | 14573 | 65536 | 22.2% | 32 | 16 | 18195 / 14573 | 1.04 | 6 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58292 |
| 15 | Hash-V-fixed | A | 1011 | 65536 | 1.5% | 64 | 32 | 31757 / 1011 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | B | 1016 | 65536 | 1.6% | 64 | 32 | 31752 / 1016 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | C | 14561 | 65536 | 22.2% | 32 | 32 | 18207 / 14561 | 1.04 | 6 | 2 (131072) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 15 | Hash-V-grow-map | A | 1011 | 2048 | 49.4% | 64 | 32 | 31757 / 1011 | 1.22 | 14 | 0 (0) | 7 | 1016 (1130) | 2214 | 131072 | 16384 |
| 15 | Hash-V-grow-map | B | 1016 | 2048 | 49.6% | 64 | 32 | 31752 / 1016 | 1.20 | 13 | 0 (0) | 7 | 1016 (1145) | 2272 | 131072 | 16384 |
| 15 | Hash-V-grow-map | C | 14561 | 32768 | 44.4% | 32 | 32 | 18207 / 14561 | 1.31 | 18 | 2 (32) | 11 | 16376 (18727) | 33702 | 131072 | 131072 |
| 15 | Hash-V-pack | A | 1011 | 65536 | 1.5% | 64 | 16 | 31757 / 1011 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8088 |
| 15 | Hash-V-pack | B | 1016 | 65536 | 1.6% | 64 | 16 | 31752 / 1016 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8128 |
| 15 | Hash-V-pack | C | 14561 | 65536 | 22.2% | 32 | 16 | 18207 / 14561 | 1.04 | 6 | 2 (131072) | 0 | 0 (0) | 0 | 65536 | 58244 |
| 16 | Hash-V-fixed | A | 281 | 65536 | 0.4% | 64 | 32 | 32487 / 281 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | B | 283 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | C | 13122 | 65536 | 20.0% | 32 | 32 | 19646 / 13122 | 1.04 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 16 | Hash-V-grow-map | A | 281 | 1024 | 27.4% | 64 | 32 | 32487 / 281 | 1.09 | 7 | 0 (0) | 6 | 504 (561) | 1608 | 131072 | 8192 |
| 16 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 32 | 32485 / 283 | 1.11 | 12 | 0 (0) | 6 | 504 (582) | 1368 | 131072 | 8192 |
| 16 | Hash-V-grow-map | C | 13122 | 32768 | 40.0% | 32 | 32 | 19646 / 13122 | 1.28 | 21 | 1 (16) | 11 | 16376 (18666) | 35469 | 131072 | 131072 |
| 16 | Hash-V-pack | A | 281 | 65536 | 0.4% | 64 | 16 | 32487 / 281 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2248 |
| 16 | Hash-V-pack | B | 283 | 65536 | 0.4% | 64 | 16 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2264 |
| 16 | Hash-V-pack | C | 13122 | 65536 | 20.0% | 32 | 16 | 19646 / 13122 | 1.04 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 52488 |

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
| 13 | A | 10219 | 32 | 16 | 81752 | 32768 | 262144 |
| 13 | B | 10123 | 32 | 16 | 80984 | 32768 | 262144 |
| 13 | C | 14718 | 32 | 16 | 58872 | 32768 | 262144 |
| 14 | A | 3541 | 32 | 16 | 28328 | 32768 | 262144 |
| 14 | B | 3561 | 32 | 16 | 28488 | 32768 | 262144 |
| 14 | C | 14573 | 32 | 16 | 58292 | 32768 | 262144 |
| 15 | A | 1011 | 32 | 16 | 8088 | 32768 | 262144 |
| 15 | B | 1016 | 32 | 16 | 8128 | 32768 | 262144 |
| 15 | C | 14561 | 32 | 16 | 58244 | 32768 | 262144 |
| 16 | A | 281 | 32 | 16 | 2248 | 32768 | 262144 |
| 16 | B | 283 | 32 | 16 | 2264 | 32768 | 262144 |
| 16 | C | 13122 | 32 | 16 | 52488 | 32768 | 262144 |

## Also reported (rule-m2 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-fixed's stores, from the model; the gates checked it).

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
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16329 | 10219 | 10123 | 14718 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 16306 | 3541 | 3561 | 14573 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 16218 | 1011 | 1016 | 14561 |
| 16 | 128 | 64 | 256 | 256 | 256 | 14447 | 281 | 283 | 13122 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.2 §15.7)

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
- job 13, C = A + B: add (16384): int_both 0.012, int_x 0.096, int_y 0.099, cross 0.000, den 0.033, num 0.000, general 0.760; cross tests 65536 (matched 744, cross reads 65536); C's store W 14718, found / new 18050 / 14718
- job 14, C = A + B: add (16384): zero_x 0.000, int_both 0.012, int_x 0.095, int_y 0.098, den 0.037, general 0.758; cross tests 65536 (matched 790, cross reads 65536); C's store W 14573, found / new 18195 / 14573
- job 15, C = A + B: add (16384): int_both 0.013, int_x 0.096, int_y 0.107, den 0.034, general 0.749; cross tests 65536 (matched 783, cross reads 65536); C's store W 14561, found / new 18207 / 14561
- job 16, C = A + B: add (16384): int_both 0.019, int_x 0.135, int_y 0.119, cross 0.001, den 0.033, general 0.693; cross tests 65536 (matched 868, cross reads 65536); C's store W 13122, found / new 19646 / 13122

The gates checked every counter and store figure against the model (spec v0.2 §15.7 item 2), the TC gate included: MHVF:1 pass, TC:1 pass, MHVGM:1 pass, MHVP:1 pass, TC:2 pass, MHVF:2 pass, MHVP:2 pass, MHVGM:2 pass, MHVF:3 pass, MHVGM:3 pass, TC:3 pass, MHVP:3 pass, MHVF:4 pass, TC:4 pass, MHVGM:4 pass, MHVP:4 pass, TC:5 pass, MHVF:5 pass, MHVGM:5 pass, MHVP:5 pass, MHVF:6 pass, TC:6 pass, MHVGM:6 pass, MHVP:6 pass, TC:7 pass, MHVF:7 pass, MHVP:7 pass, MHVGM:7 pass, TC:8 pass, MHVF:8 pass, MHVGM:8 pass, MHVP:8 pass, MHVF:9 pass, TC:9 pass, MHVP:9 pass, MHVGM:9 pass, TC:10 pass, MHVF:10 pass, MHVGM:10 pass, MHVP:10 pass, TC:11 pass, MHVGM:11 pass, MHVF:11 pass, MHVP:11 pass, TC:12 pass, MHVGM:12 pass, MHVF:12 pass, MHVP:12 pass, TC:13 pass, MHVGM:13 pass, MHVF:13 pass, TC:14 pass, MHVP:13 pass, MHVF:14 pass, MHVGM:14 pass, TC:15 pass, MHVP:14 pass, MHVGM:15 pass, MHVF:15 pass, TC:16 pass, MHVP:15 pass, MHVF:16 pass, MHVGM:16 pass, MHVP:16 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m2 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 2.967 | no | 1 | 14 | 0.071 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 1 (8->8 threads), claude 1 (8->8 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 2.955 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (8->7 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 3.216 | no | 1 | 14 | 0.071 | 14 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 16.229 | no | 8 | 64 | 0.125 | 66 | 2 | none | 22 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 2/16 (7->8 threads), bash 0/16 (1->1 threads) |
| 2 | measured | 1 | 14.178 | no | 7 | 64 | 0.109 | 64 | 0 | none | 20 | none | process_api 0/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (8->6 threads), bash 0/14 (1->1 threads) |
| 3 | measured | 1 | 17.152 | no | 9 | 64 | 0.141 | 65 | 1 | none | 23 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/17 (8->8 threads), claude 2/17 (7->7 threads), bash 0/16 (1->1 threads) |
| 4 | measured | 1 | 14.547 | no | 8 | 64 | 0.125 | 65 | 1 | none | 20 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (7->7 threads), bash 0/15 (1->1 threads) |
| 5 | measured | 1 | 13.186 | no | 7 | 64 | 0.109 | 64 | 0 | none | 19 | none | process_api 0/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 1/14 (8->8 threads), claude 1/14 (8->7 threads), bash 0/14 (1->1 threads) |
| 6 | measured | 1 | 14.429 | no | 8 | 64 | 0.125 | 66 | 2 | none | 20 | none | process_api 1/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (7->8 threads), bash 0/14 (1->1 threads) |
| 7 | measured | 1 | 13.396 | no | 8 | 64 | 0.125 | 64 | 0 | none | 19 | none | process_api 0/14 (6->6 threads), sh 0/14 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (8->6 threads), bash 0/14 (1->1 threads) |
| 8 | measured | 1 | 10.998 | no | 8 | 64 | 0.125 | 65 | 1 | none | 16 | none | process_api 0/12 (6->6 threads), sh 0/12 (1->1 threads), environment-man 0/13 (8->8 threads), claude 1/13 (6->7 threads), bash 0/12 (1->1 threads) |
| 9 | measured | 1 | 15.158 | no | 8 | 64 | 0.125 | 65 | 1 | none | 21 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/16 (8->7 threads), bash 0/15 (1->1 threads) |
| 10 | measured | 1 | 16.344 | no | 9 | 64 | 0.141 | 66 | 2 | none | 22 | none | process_api 0/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 0/17 (8->8 threads), claude 2/17 (7->8 threads), bash 0/16 (1->1 threads) |
| 11 | measured | 1 | 15.396 | no | 7 | 64 | 0.109 | 65 | 1 | none | 21 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/16 (8->7 threads), bash 0/15 (1->1 threads) |
| 12 | measured | 1 | 15.475 | no | 8 | 64 | 0.125 | 65 | 1 | none | 21 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/16 (8->8 threads), claude 2/16 (7->8 threads), bash 0/15 (1->1 threads) |
| 13 | measured | 1 | 15.858 | no | 8 | 64 | 0.125 | 65 | 1 | none | 21 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/16 (7->7 threads), bash 0/15 (1->1 threads) |
| 14 | measured | 1 | 15.214 | no | 9 | 64 | 0.141 | 66 | 2 | none | 21 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/16 (7->7 threads), bash 0/15 (1->1 threads) |
| 15 | measured | 1 | 14.723 | no | 9 | 64 | 0.141 | 65 | 1 | none | 20 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 1/15 (8->8 threads), claude 2/15 (7->8 threads), bash 0/15 (1->1 threads) |
| 16 | measured | 1 | 15.827 | no | 9 | 64 | 0.141 | 65 | 1 | none | 21 | none | process_api 1/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (8->8 threads), claude 2/16 (8->6 threads), bash 0/15 (1->1 threads) |

The runner's own ticks over the 19 blocks: 134 ticks over 1066 launches, 0.126 per launch; the highest block 0.143 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9998 | 0.9757 | 10726 | 10548 |
| 1 | Hash-V-fixed | 12 | 0.9997 | 0.9956 | 21100 | 20938 |
| 1 | Hash-V-grow-map | 12 | 0.9999 | 0.9916 | 46415 | 46233 |
| 1 | Hash-V-pack | 12 | 0.9998 | 0.9993 | 27843 | 27607 |
| 1 | Hash-V-fixed-null | 12 | 0.9999 | 0.9977 | 20946 | 20757 |
| 2 | TC | 12 | 0.9999 | 0.9929 | 10222 | 10053 |
| 2 | Hash-V-fixed | 12 | 0.9997 | 0.9693 | 19627 | 19499 |
| 2 | Hash-V-grow-map | 12 | 0.9994 | 0.9911 | 29809 | 29660 |
| 2 | Hash-V-pack | 12 | 0.9994 | 0.9922 | 24787 | 24578 |
| 2 | Hash-V-fixed-null | 12 | 0.9989 | 0.9779 | 19673 | 19546 |
| 3 | TC | 12 | 0.9968 | 0.9848 | 10541 | 10399 |
| 3 | Hash-V-fixed | 12 | 0.9981 | 0.9803 | 19209 | 19130 |
| 3 | Hash-V-grow-map | 12 | 0.9995 | 0.9946 | 23570 | 23394 |
| 3 | Hash-V-pack | 12 | 0.9963 | 0.9709 | 23158 | 23036 |
| 3 | Hash-V-fixed-null | 12 | 0.9989 | 0.9908 | 18987 | 18919 |
| 4 | TC | 12 | 0.9999 | 0.9913 | 12545 | 12453 |
| 4 | Hash-V-fixed | 12 | 0.9991 | 0.9917 | 20954 | 20793 |
| 4 | Hash-V-grow-map | 12 | 0.9995 | 0.9050 | 23512 | 23501 |
| 4 | Hash-V-pack | 12 | 0.9999 | 0.9965 | 24639 | 24472 |
| 4 | Hash-V-fixed-null | 12 | 0.9993 | 0.9941 | 20954 | 20783 |
| 5 | TC | 12 | 0.9998 | 0.9814 | 68234 | 68057 |
| 5 | Hash-V-fixed | 12 | 0.9998 | 0.9965 | 123951 | 123645 |
| 5 | Hash-V-grow-map | 12 | 0.9997 | 0.9928 | 206817 | 207145 |
| 5 | Hash-V-pack | 12 | 0.9993 | 0.9810 | 167794 | 167795 |
| 5 | Hash-V-fixed-null | 12 | 0.9998 | 0.9955 | 123479 | 123296 |
| 6 | TC | 12 | 0.9998 | 0.9831 | 67400 | 67346 |
| 6 | Hash-V-fixed | 12 | 0.9998 | 0.9968 | 122776 | 122524 |
| 6 | Hash-V-grow-map | 12 | 0.9998 | 0.9811 | 184262 | 184034 |
| 6 | Hash-V-pack | 12 | 1.0000 | 0.9981 | 150926 | 150646 |
| 6 | Hash-V-fixed-null | 12 | 0.9999 | 0.9957 | 123086 | 122871 |
| 7 | TC | 12 | 0.9994 | 0.8446 | 68734 | 68641 |
| 7 | Hash-V-fixed | 12 | 0.9996 | 0.9958 | 116761 | 116515 |
| 7 | Hash-V-grow-map | 12 | 0.9995 | 0.9727 | 152085 | 151881 |
| 7 | Hash-V-pack | 12 | 0.9996 | 0.9988 | 137100 | 136878 |
| 7 | Hash-V-fixed-null | 12 | 0.9999 | 0.9945 | 116691 | 116385 |
| 8 | TC | 12 | 0.9996 | 0.9935 | 51054 | 50935 |
| 8 | Hash-V-fixed | 12 | 0.9989 | 0.9941 | 98673 | 98506 |
| 8 | Hash-V-grow-map | 12 | 0.9993 | 0.9847 | 124628 | 124466 |
| 8 | Hash-V-pack | 12 | 1.0000 | 0.9967 | 117669 | 117472 |
| 8 | Hash-V-fixed-null | 12 | 0.9984 | 0.7941 | 98956 | 99215 |
| 9 | TC | 12 | 0.9999 | 0.9960 | 366225 | 365979 |
| 9 | Hash-V-fixed | 12 | 0.9998 | 0.9979 | 571989 | 572055 |
| 9 | Hash-V-grow-map | 12 | 0.9998 | 0.9956 | 845249 | 846013 |
| 9 | Hash-V-pack | 12 | 0.9996 | 0.9797 | 744945 | 744592 |
| 9 | Hash-V-fixed-null | 12 | 0.9997 | 0.9644 | 573104 | 572965 |
| 10 | TC | 12 | 0.9995 | 0.9982 | 365742 | 365446 |
| 10 | Hash-V-fixed | 12 | 0.9997 | 0.9791 | 570067 | 569791 |
| 10 | Hash-V-grow-map | 12 | 0.9997 | 0.9962 | 730170 | 730296 |
| 10 | Hash-V-pack | 12 | 0.9997 | 0.9946 | 676951 | 676951 |
| 10 | Hash-V-fixed-null | 12 | 0.9996 | 0.9833 | 568232 | 567968 |
| 11 | TC | 12 | 0.9999 | 0.9972 | 359273 | 358972 |
| 11 | Hash-V-fixed | 12 | 0.9996 | 0.9955 | 544240 | 544395 |
| 11 | Hash-V-grow-map | 12 | 0.9998 | 0.9985 | 673449 | 673564 |
| 11 | Hash-V-pack | 12 | 0.9998 | 0.9976 | 633954 | 633942 |
| 11 | Hash-V-fixed-null | 12 | 0.9998 | 0.9936 | 546780 | 546506 |
| 12 | TC | 12 | 0.9995 | 0.9927 | 356048 | 355859 |
| 12 | Hash-V-fixed | 12 | 1.0000 | 0.9958 | 550945 | 550975 |
| 12 | Hash-V-grow-map | 12 | 0.9999 | 0.9913 | 652730 | 652484 |
| 12 | Hash-V-pack | 12 | 0.9997 | 0.9992 | 623037 | 622779 |
| 12 | Hash-V-fixed-null | 12 | 0.9998 | 0.9985 | 551273 | 551154 |
| 13 | TC | 12 | 0.9984 | 0.9911 | 1545672 | 1547503 |
| 13 | Hash-V-fixed | 12 | 0.9997 | 0.9936 | 2490554 | 2491923 |
| 13 | Hash-V-grow-map | 12 | 0.9998 | 0.9966 | 3431493 | 3432206 |
| 13 | Hash-V-pack | 12 | 0.9998 | 0.9973 | 2968492 | 2969380 |
| 13 | Hash-V-fixed-null | 12 | 0.9994 | 0.9975 | 2483740 | 2483264 |
| 14 | TC | 12 | 0.9996 | 0.9772 | 1547288 | 1546628 |
| 14 | Hash-V-fixed | 12 | 0.9999 | 0.9756 | 2374386 | 2373587 |
| 14 | Hash-V-grow-map | 12 | 0.9998 | 0.9945 | 2934507 | 2934311 |
| 14 | Hash-V-pack | 12 | 1.0000 | 0.9947 | 2737736 | 2736927 |
| 14 | Hash-V-fixed-null | 12 | 1.0000 | 0.9954 | 2378763 | 2379820 |
| 15 | TC | 12 | 1.0000 | 0.9969 | 1547825 | 1547790 |
| 15 | Hash-V-fixed | 12 | 0.9999 | 0.9643 | 2326593 | 2326064 |
| 15 | Hash-V-grow-map | 12 | 0.9997 | 0.9955 | 2800929 | 2802867 |
| 15 | Hash-V-pack | 12 | 0.9999 | 0.9945 | 2643020 | 2646721 |
| 15 | Hash-V-fixed-null | 12 | 1.0000 | 0.9956 | 2319299 | 2319585 |
| 16 | TC | 12 | 0.9995 | 0.9900 | 1499627 | 1499903 |
| 16 | Hash-V-fixed | 12 | 1.0000 | 0.9966 | 2256576 | 2256615 |
| 16 | Hash-V-grow-map | 12 | 0.9998 | 0.9927 | 2663817 | 2663438 |
| 16 | Hash-V-pack | 12 | 0.9998 | 0.9870 | 2538591 | 2542571 |
| 16 | Hash-V-fixed-null | 12 | 0.9997 | 0.9787 | 2252907 | 2253974 |

Idle checks: 19 (0 busy). Host rates R_h: {'1': 0.31094527363184077, '79': 0.31094527363184077, '84': 0.337026276628065, '92': 0.33843642932135554, '17736': 0.31094527363184077} (measured: {'1': 0.0, '79': 0.0, '84': 0.337026276628065, '92': 0.33843642932135554, '17736': 0.0}; R_h is at least 1 tick over the longest pilot block).

### Earlier rounds, for reference only

Round m1's runs, [`../results-m1-add/results.md`](../results-m1-add/results.md) and [`../results-m1-mul/results.md`](../results-m1-mul/results.md), and round 3's, [`../../../10-interning/test/results-r3/results.md`](../../../10-interning/test/results-r3/results.md). They enter no measure here.

## Limitations

- **Textbook only.** No row is optimized: plain open addressing with linear probing; growth by doubling.
- **Only V is tested.** The cross-test variant M was dropped with its rows.
- **Hash-V-pack's build table is Hash-V-fixed's**, so its build costs at least as much as Hash-V-fixed's. Its gains, if any, are in memory and in reading operands from dense lists.
- **Fixed's fill is low and not controlled:** its tables are sized from the dimensions (T >= 2n^2 + 2), and it loses on memory by design. **Grow has a fill limit** of one half.
- **The expression sets reuse one value distribution**, and their sharing comes only from the inputs.
- **Results are reported per run** and not combined across runs or with m1.
- **64 bits only.** A value past 64 bits is an error in every build; the sweep stays far below.
- **Square matrices.** Every input of a job has the same sharing level.
- Every other limitation of round 3 and m1 that applies (the disturbance check cannot see every process; a child of a process that ignores SIGCHLD is not charged).

