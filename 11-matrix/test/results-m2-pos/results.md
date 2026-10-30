# Design 3 on matrices, round m2, the product of sums (A + B)(C + D): results

Run started 2026-10-30T09:38:41-05:00.

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

- The pos run (job set `pos`: S = A + B; U = C + D; R = S x U).
- Started 2026-10-30T09:38:41-05:00; finished 2026-10-30T09:48:31-05:00.
- Step 0 (before the clock): 95.8 s; step 0's files: new job set.
- Run time 8.2 minutes (hard stop 40).
- Rule sha256 `e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200`; round 3's results after the run: unchanged; m1's results and the other m2 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[22484], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T09:40:17-05:00: TC `179dacc22ad6`, MHVF `eb7f57fd8798`, MHVGM `915116a75090`, MHVP `56a37a633b7c`, TC3GATE `83ff353acc03`
- Unit tests: m2_unit: m2_unit: 642586 checks, 0 failures; m2_diff: m2_diff: 3000 seeds, 3000 jobs, 1050092 operations; 1137376 checks, 0 failures; m2_alloc-timing: m2_alloc (timing build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-counting: m2_alloc (counting build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-memory: m2_alloc (memory build): 160 jobs, 2450 moments, 32596 checks, 0 failures; m2_unit-asan: m2_unit: 642586 checks, 0 failures; m2_diff-asan: m2_diff: 600 seeds, 600 jobs, 209812 operations; 227944 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 5.2 s, job 6 4.7 s, job 13 12.7 s; P_6 = 4.7 s, P_13 = 12.7 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 11.4 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.09393199323689648, '79': 0.09393199323689648, '84': 0.09393199323689648, '92': 0.31649252661811345, '22484': 0.09393199323689648}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 5 product-of-sums time; 6 stable bytes of one stored input matrix; peak bytes during the operation (reported). Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-fixed against Hash-V-fixed-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses). Hash-V-pack against Hash-V-grow-map is reported with no verdict on any measure. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m2 item 8, carried over).

### Hash-V-fixed against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.641 [0.630, 0.656], h 0.050 | loss, 0.866 [0.863, 0.871], h 0.050 | reported, 0.852 [0.846, 0.857] | reported, 0.596 [0.584, 0.758] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.000 (heap 1.991) |
| 2 (n 16, rho 4) | loss, 0.653 [0.640, 0.660], h 0.050 | loss, 0.870 [0.865, 0.873], h 0.050 | reported, 0.856 [0.851, 0.859] | reported, 0.704 [0.595, 0.766] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.000 (heap 1.991) |
| 3 (n 16, rho 16) | loss, 0.685 [0.678, 0.690], h 0.050 | loss, 0.865 [0.862, 0.870], h 0.050 | reported, 0.855 [0.852, 0.859] | reported, 0.616 [0.589, 0.672] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.000 (heap 1.991) |
| 4 (n 16, rho 64) | loss, 0.642 [0.638, 0.646], h 0.050 | loss, 0.871 [0.868, 0.873], h 0.050 | reported, 0.860 [0.857, 0.861] | reported, 0.715 [0.627, 0.774] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.000 (heap 1.991) |
| 5 (n 32, rho 1) | loss, 0.723 [0.717, 0.730], h 0.050 | loss, 0.892 [0.890, 0.900], h 0.050 | reported, 0.886 [0.884, 0.895] | reported, 0.724 [0.638, 0.861] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.000 (heap 1.998) |
| 6 (n 32, rho 4) | loss, 0.705 [0.701, 0.713], h 0.050 | loss, 0.895 [0.893, 0.905], h 0.050 | reported, 0.889 [0.887, 0.898] | reported, 0.796 [0.624, 0.843] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.000 (heap 1.998) |
| 7 (n 32, rho 16) | loss, 0.747 [0.734, 0.756], h 0.050 | loss, 0.895 [0.873, 0.899], h 0.050 | reported, 0.890 [0.868, 0.894] | reported, 0.629 [0.599, 0.664] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.000 (heap 1.998) |
| 8 (n 32, rho 64) | loss, 0.750 [0.745, 0.757], h 0.050 | loss, 0.896 [0.891, 0.902], h 0.050 | reported, 0.892 [0.886, 0.897] | reported, 0.715 [0.618, 0.824] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.000 (heap 1.998) |
| 9 (n 64, rho 1) | loss, 0.714 [0.711, 0.717], h 0.050 | loss, 0.905 [0.897, 0.907], h 0.050 | reported, 0.901 [0.893, 0.904] | reported, 0.759 [0.656, 0.838] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.000 (heap 1.999) |
| 10 (n 64, rho 4) | loss, 0.712 [0.705, 0.720], h 0.050 | loss, 0.904 [0.889, 0.910], h 0.050 | reported, 0.901 [0.884, 0.907] | reported, 0.688 [0.598, 0.817] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.000 (heap 1.999) |
| 11 (n 64, rho 16) | loss, 0.754 [0.747, 0.757], h 0.050 | loss, 0.905 [0.901, 0.912], h 0.050 | reported, 0.903 [0.898, 0.910] | reported, 0.751 [0.666, 0.844] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.000 (heap 1.999) |
| 12 (n 64, rho 64) | loss, 0.763 [0.756, 0.774], h 0.050 | loss, 0.903 [0.900, 0.907], h 0.050 | reported, 0.901 [0.898, 0.904] | reported, 0.745 [0.640, 0.833] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.000 (heap 1.999) |
| 13 (n 128, rho 1) | loss, 0.669 [0.667, 0.673], h 0.050 | loss, 0.908 [0.902, 0.918], h 0.050 | reported, 0.905 [0.899, 0.915] | reported, 0.306 [0.252, 0.346] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.250 (heap 2.250) |
| 14 (n 128, rho 4) | loss, 0.712 [0.708, 0.714], h 0.050 | loss, 0.909 [0.904, 0.916], h 0.050 | reported, 0.907 [0.902, 0.913] | reported, 0.329 [0.279, 0.386] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.250 (heap 2.250) |
| 15 (n 128, rho 16) | loss, 0.742 [0.737, 0.745], h 0.050 | loss, 0.909 [0.895, 0.915], h 0.050 | reported, 0.907 [0.891, 0.913] | reported, 0.303 [0.243, 0.396] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.250 (heap 2.250) |
| 16 (n 128, rho 64) | loss, 0.755 [0.751, 0.758], h 0.050 | loss, 0.917 [0.912, 0.921], h 0.050 | reported, 0.915 [0.910, 0.919] | reported, 0.294 [0.230, 0.340] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.250 (heap 2.250) |

### Hash-V-grow-map against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.319 [0.314, 0.323], h 0.050 | loss, 0.830 [0.817, 0.834], h 0.050 | reported, 0.771 [0.758, 0.775] | reported, 0.545 [0.536, 0.693] | loss, 2.250 (heap 2.237) **severe memory loss** | reported, 2.143 (heap 2.132) |
| 2 (n 16, rho 4) | loss, 0.493 [0.485, 0.498], h 0.050 | loss, 0.837 [0.832, 0.839], h 0.050 | reported, 0.810 [0.806, 0.812] | reported, 0.641 [0.541, 0.699] | win, 0.750 (heap 0.765) | reported, 1.286 (heap 1.291) |
| 3 (n 16, rho 16) | loss, 0.590 [0.583, 0.596], h 0.050 | loss, 0.844 [0.841, 0.849], h 0.050 | reported, 0.828 [0.825, 0.833] | reported, 0.575 [0.550, 0.628] | win, 0.375 (heap 0.398) | reported, 0.929 (heap 0.941) |
| 4 (n 16, rho 64) | loss, 0.589 [0.583, 0.593], h 0.050 | loss, 0.847 [0.839, 0.851], h 0.050 | reported, 0.833 [0.826, 0.838] | reported, 0.659 [0.576, 0.717] | win, 0.281 (heap 0.305) | reported, 0.750 (heap 0.765) |
| 5 (n 32, rho 1) | loss, 0.453 [0.450, 0.457], h 0.050 | loss, 0.879 [0.876, 0.888], h 0.050 | reported, 0.857 [0.854, 0.865] | reported, 0.689 [0.598, 0.830] | loss, 1.250 (heap 1.251) | reported, 1.571 (heap 1.571) |
| 6 (n 32, rho 4) | loss, 0.530 [0.526, 0.537], h 0.050 | loss, 0.879 [0.877, 0.889], h 0.050 | reported, 0.863 [0.861, 0.874] | reported, 0.760 [0.593, 0.808] | win, 0.750 (heap 0.754) | reported, 1.286 (heap 1.287) |
| 7 (n 32, rho 16) | loss, 0.620 [0.603, 0.627], h 0.050 | loss, 0.883 [0.843, 0.886], h 0.050 | reported, 0.873 [0.834, 0.876] | reported, 0.593 [0.543, 0.627] | win, 0.375 (heap 0.380) | reported, 0.929 (heap 0.931) |
| 8 (n 32, rho 64) | loss, 0.650 [0.641, 0.659], h 0.050 | loss, 0.887 [0.869, 0.893], h 0.050 | reported, 0.879 [0.862, 0.886] | reported, 0.686 [0.591, 0.794] | win, 0.281 (heap 0.287) | reported, 0.804 (heap 0.807) |
| 9 (n 64, rho 1) | loss, 0.461 [0.458, 0.463], h 0.050 | loss, 0.902 [0.893, 0.904], h 0.050 | reported, 0.891 [0.881, 0.893] | reported, 0.831 [0.732, 0.891] | loss, 1.250 (heap 1.250) | reported, 1.429 (heap 1.429) |
| 10 (n 64, rho 4) | loss, 0.585 [0.579, 0.597], h 0.050 | loss, 0.902 [0.897, 0.908], h 0.050 | reported, 0.896 [0.890, 0.901] | reported, 0.763 [0.697, 0.889] | win, 0.500 (heap 0.501) | reported, 1.000 (heap 1.001) |
| 11 (n 64, rho 16) | loss, 0.648 [0.643, 0.655], h 0.050 | loss, 0.903 [0.900, 0.909], h 0.050 | reported, 0.898 [0.895, 0.904] | reported, 0.793 [0.701, 0.885] | win, 0.375 (heap 0.376) | reported, 0.929 (heap 0.929) |
| 12 (n 64, rho 64) | loss, 0.673 [0.667, 0.688], h 0.050 | loss, 0.903 [0.899, 0.906], h 0.050 | reported, 0.899 [0.895, 0.901] | reported, 0.771 [0.681, 0.865] | win, 0.281 (heap 0.283) | reported, 0.875 (heap 0.876) |
| 13 (n 128, rho 1) | loss, 0.457 [0.454, 0.462], h 0.050 | loss, 0.910 [0.903, 0.920], h 0.050 | reported, 0.902 [0.896, 0.911] | reported, 0.878 [0.791, 0.932] | loss, 1.500 (heap 1.500) | reported, 1.714 (heap 1.714) |
| 14 (n 128, rho 4) | loss, 0.614 [0.607, 0.618], h 0.050 | loss, 0.911 [0.905, 0.917], h 0.050 | reported, 0.908 [0.902, 0.913] | reported, 0.771 [0.633, 0.896] | win, 0.750 (heap 0.750) | reported, 1.286 (heap 1.286) |
| 15 (n 128, rho 16) | loss, 0.660 [0.655, 0.663], h 0.050 | loss, 0.908 [0.905, 0.915], h 0.050 | reported, 0.906 [0.903, 0.912] | reported, 0.824 [0.685, 0.909] | win, 0.562 (heap 0.563) | reported, 1.179 (heap 1.179) |
| 16 (n 128, rho 64) | loss, 0.710 [0.702, 0.715], h 0.050 | loss, 0.917 [0.912, 0.920], h 0.050 | reported, 0.915 [0.910, 0.918] | reported, 0.933 [0.800, 1.033] | win, 0.531 (heap 0.532) | reported, 1.161 (heap 1.161) |

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.448 [0.441, 0.456], h 0.050 | loss, 0.854 [0.851, 0.857], h 0.050 | reported, 0.818 [0.814, 0.822] | reported, 0.503 [0.497, 0.630] | win, 0.804 (heap 0.820) | reported, 1.086 (heap 1.097) |
| 2 (n 16, rho 4) | loss, 0.535 [0.525, 0.541], h 0.050 | loss, 0.855 [0.851, 0.857], h 0.050 | reported, 0.831 [0.827, 0.833] | reported, 0.574 [0.485, 0.630] | win, 0.297 (heap 0.323) | reported, 0.795 (heap 0.813) |
| 3 (n 16, rho 16) | loss, 0.590 [0.582, 0.596], h 0.050 | loss, 0.856 [0.851, 0.862], h 0.050 | reported, 0.838 [0.834, 0.844] | reported, 0.515 [0.489, 0.562] | win, 0.179 (heap 0.209) | reported, 0.705 (heap 0.724) |
| 4 (n 16, rho 64) | loss, 0.549 [0.542, 0.554], h 0.050 | loss, 0.854 [0.846, 0.857], h 0.050 | reported, 0.838 [0.829, 0.840] | reported, 0.592 [0.517, 0.641] | win, 0.139 (heap 0.168) | reported, 0.596 (heap 0.617) |
| 5 (n 32, rho 1) | loss, 0.524 [0.516, 0.530], h 0.050 | loss, 0.883 [0.868, 0.892], h 0.050 | reported, 0.867 [0.853, 0.876] | reported, 0.614 [0.540, 0.735] | win, 0.742 (heap 0.747) | reported, 1.037 (heap 1.040) |
| 6 (n 32, rho 4) | loss, 0.605 [0.598, 0.615], h 0.050 | loss, 0.888 [0.870, 0.899], h 0.050 | reported, 0.877 [0.859, 0.887] | reported, 0.672 [0.522, 0.727] | win, 0.388 (heap 0.394) | reported, 0.835 (heap 0.839) |
| 7 (n 32, rho 16) | loss, 0.667 [0.655, 0.672], h 0.050 | loss, 0.885 [0.881, 0.891], h 0.050 | reported, 0.877 [0.872, 0.882] | reported, 0.540 [0.525, 0.571] | win, 0.168 (heap 0.176) | reported, 0.700 (heap 0.705) |
| 8 (n 32, rho 64) | loss, 0.671 [0.666, 0.676], h 0.050 | loss, 0.893 [0.882, 0.899], h 0.050 | reported, 0.886 [0.875, 0.891] | reported, 0.610 [0.525, 0.714] | win, 0.138 (heap 0.145) | reported, 0.638 (heap 0.644) |
| 9 (n 64, rho 1) | loss, 0.533 [0.531, 0.535], h 0.050 | loss, 0.900 [0.898, 0.902], h 0.050 | reported, 0.891 [0.890, 0.893] | reported, 0.786 [0.688, 0.842] | win, 0.681 (heap 0.682) | reported, 0.996 (heap 0.997) |
| 10 (n 64, rho 4) | loss, 0.617 [0.612, 0.624], h 0.050 | loss, 0.901 [0.872, 0.907], h 0.050 | reported, 0.896 [0.866, 0.901] | reported, 0.673 [0.609, 0.811] | win, 0.374 (heap 0.375) | reported, 0.820 (heap 0.822) |
| 11 (n 64, rho 16) | loss, 0.681 [0.679, 0.685], h 0.050 | loss, 0.902 [0.898, 0.907], h 0.050 | reported, 0.898 [0.894, 0.903] | reported, 0.761 [0.703, 0.850] | win, 0.284 (heap 0.286) | reported, 0.768 (heap 0.769) |
| 12 (n 64, rho 64) | loss, 0.694 [0.691, 0.707], h 0.050 | loss, 0.903 [0.901, 0.906], h 0.050 | reported, 0.900 [0.897, 0.903] | reported, 0.698 [0.611, 0.795] | win, 0.136 (heap 0.138) | reported, 0.660 (heap 0.661) |
| 13 (n 128, rho 1) | loss, 0.544 [0.542, 0.549], h 0.050 | loss, 0.909 [0.903, 0.917], h 0.050 | reported, 0.904 [0.899, 0.913] | reported, 0.921 [0.842, 0.959] | win, 0.560 (heap 0.560) | reported, 1.027 (heap 1.028) |
| 14 (n 128, rho 4) | loss, 0.635 [0.626, 0.639], h 0.050 | loss, 0.909 [0.901, 0.915], h 0.050 | reported, 0.906 [0.898, 0.912] | reported, 0.798 [0.671, 0.920] | win, 0.358 (heap 0.359) | reported, 0.912 (heap 0.912) |
| 15 (n 128, rho 16) | loss, 0.685 [0.678, 0.690], h 0.050 | loss, 0.904 [0.899, 0.909], h 0.050 | reported, 0.902 [0.896, 0.907] | reported, 0.883 [0.817, 0.934] | win, 0.281 (heap 0.281) | reported, 0.867 (heap 0.868) |
| 16 (n 128, rho 64) | loss, 0.691 [0.685, 0.703], h 0.050 | loss, 0.915 [0.909, 0.919], h 0.050 | reported, 0.913 [0.907, 0.917] | reported, 0.857 [0.776, 0.933] | win, 0.259 (heap 0.259) | reported, 0.848 (heap 0.849) |

### Hash-V-pack against Hash-V-fixed (pack against fixed)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.699 [0.691, 0.704], h 0.050 | tie, 0.986 [0.980, 0.992], h 0.050 | reported, 0.960 [0.953, 0.969] | reported, 0.844 [0.831, 0.857] | win, 0.357 (heap 0.367) | reported, 0.543 (heap 0.551) |
| 2 (n 16, rho 4) | loss, 0.819 [0.814, 0.825], h 0.050 | tie, 0.984 [0.978, 0.988], h 0.050 | reported, 0.971 [0.965, 0.976] | reported, 0.815 [0.804, 0.841] | win, 0.132 (heap 0.144) | reported, 0.398 (heap 0.408) |
| 3 (n 16, rho 16) | loss, 0.861 [0.854, 0.871], h 0.050 | tie, 0.989 [0.983, 0.992], h 0.050 | reported, 0.981 [0.974, 0.984] | reported, 0.836 [0.817, 0.852] | win, 0.080 (heap 0.093) | reported, 0.352 (heap 0.364) |
| 4 (n 16, rho 64) | loss, 0.854 [0.846, 0.865], h 0.050 | tie, 0.981 [0.972, 0.984], h 0.050 | reported, 0.975 [0.966, 0.977] | reported, 0.828 [0.817, 0.836] | win, 0.062 (heap 0.075) | reported, 0.298 (heap 0.310) |
| 5 (n 32, rho 1) | loss, 0.724 [0.715, 0.732], h 0.050 | tie, 0.990 [0.970, 0.998], h 0.050 | reported, 0.978 [0.959, 0.985] | reported, 0.848 [0.834, 0.916] | win, 0.330 (heap 0.332) | reported, 0.519 (heap 0.521) |
| 6 (n 32, rho 4) | loss, 0.858 [0.851, 0.865], h 0.050 | tie, 0.992 [0.971, 0.995], h 0.050 | reported, 0.987 [0.966, 0.989] | reported, 0.844 [0.828, 0.895] | win, 0.172 (heap 0.175) | reported, 0.417 (heap 0.420) |
| 7 (n 32, rho 16) | loss, 0.893 [0.876, 0.911], h 0.050 | tie, 0.988 [0.983, 1.018], h 0.050 | reported, 0.985 [0.978, 1.014] | reported, 0.859 [0.847, 0.918] | win, 0.075 (heap 0.078) | reported, 0.350 (heap 0.353) |
| 8 (n 32, rho 64) | loss, 0.894 [0.886, 0.903], h 0.050 | tie, 0.997 [0.984, 1.001], h 0.050 | reported, 0.993 [0.981, 0.998] | reported, 0.853 [0.809, 0.916] | win, 0.061 (heap 0.065) | reported, 0.319 (heap 0.322) |
| 9 (n 64, rho 1) | loss, 0.746 [0.743, 0.750], h 0.050 | tie, 0.995 [0.991, 1.004], h 0.050 | reported, 0.989 [0.986, 0.999] | reported, 1.035 [0.960, 1.099] | win, 0.303 (heap 0.303) | reported, 0.498 (heap 0.499) |
| 10 (n 64, rho 4) | loss, 0.866 [0.860, 0.875], h 0.050 | tie, 0.997 [0.967, 1.014], h 0.050 | reported, 0.994 [0.967, 1.011] | reported, 0.978 [0.896, 1.134] | win, 0.166 (heap 0.167) | reported, 0.410 (heap 0.411) |
| 11 (n 64, rho 16) | loss, 0.904 [0.900, 0.912], h 0.050 | tie, 0.997 [0.990, 1.000], h 0.050 | reported, 0.995 [0.989, 0.999] | reported, 1.014 [0.969, 1.091] | win, 0.126 (heap 0.127) | reported, 0.384 (heap 0.384) |
| 12 (n 64, rho 64) | loss, 0.910 [0.906, 0.925], h 0.050 | tie, 1.000 [0.996, 1.003], h 0.050 | reported, 0.999 [0.995, 1.001] | reported, 0.936 [0.884, 1.034] | win, 0.060 (heap 0.061) | reported, 0.330 (heap 0.331) |
| 13 (n 128, rho 1) | loss, 0.814 [0.809, 0.820], h 0.050 | tie, 1.001 [0.994, 1.007], h 0.050 | reported, 1.000 [0.992, 1.006] | reported, 3.006 [2.685, 3.457] | win, 0.224 (heap 0.224) | reported, 0.457 (heap 0.457) |
| 14 (n 128, rho 4) | loss, 0.891 [0.881, 0.898], h 0.050 | tie, 1.000 [0.993, 1.002], h 0.050 | reported, 0.999 [0.992, 1.001] | reported, 2.430 [2.091, 2.804] | win, 0.143 (heap 0.143) | reported, 0.405 (heap 0.405) |
| 15 (n 128, rho 16) | loss, 0.924 [0.914, 0.930], h 0.050 | tie, 0.994 [0.988, 1.009], h 0.050 | reported, 0.994 [0.987, 1.011] | reported, 2.915 [2.311, 3.532] | win, 0.112 (heap 0.113) | reported, 0.385 (heap 0.386) |
| 16 (n 128, rho 64) | loss, 0.915 [0.908, 0.930], h 0.050 | tie, 0.998 [0.993, 1.004], h 0.050 | reported, 0.998 [0.993, 1.003] | reported, 2.913 [2.500, 3.625] | win, 0.103 (heap 0.104) | reported, 0.377 (heap 0.377) |

### Hash-V-grow-map against Hash-V-fixed (grow-map against fixed)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.497 [0.490, 0.502], h 0.050 | tie, 0.958 [0.945, 0.961], h 0.050 | reported, 0.905 [0.891, 0.908] | reported, 0.915 [0.903, 0.925] | tie, 1.000 (heap 1.000) | reported, 1.071 (heap 1.071) |
| 2 (n 16, rho 4) | loss, 0.755 [0.750, 0.762], h 0.050 | tie, 0.962 [0.958, 0.967], h 0.050 | reported, 0.947 [0.942, 0.951] | reported, 0.911 [0.897, 0.932] | win, 0.333 (heap 0.342) | reported, 0.643 (heap 0.648) |
| 3 (n 16, rho 16) | loss, 0.862 [0.856, 0.868], h 0.050 | tie, 0.975 [0.970, 0.979], h 0.050 | reported, 0.968 [0.963, 0.972] | reported, 0.934 [0.913, 0.959] | win, 0.167 (heap 0.178) | reported, 0.464 (heap 0.473) |
| 4 (n 16, rho 64) | loss, 0.918 [0.910, 0.922], h 0.050 | tie, 0.972 [0.966, 0.977], h 0.050 | reported, 0.969 [0.963, 0.974] | reported, 0.922 [0.913, 0.932] | win, 0.125 (heap 0.136) | reported, 0.375 (heap 0.385) |
| 5 (n 32, rho 1) | loss, 0.626 [0.624, 0.631], h 0.050 | tie, 0.986 [0.980, 0.988], h 0.050 | reported, 0.967 [0.962, 0.969] | reported, 0.952 [0.924, 0.980] | win, 0.556 (heap 0.557) | reported, 0.786 (heap 0.787) |
| 6 (n 32, rho 4) | loss, 0.751 [0.748, 0.754], h 0.050 | tie, 0.982 [0.979, 0.984], h 0.050 | reported, 0.971 [0.969, 0.974] | reported, 0.956 [0.944, 0.982] | win, 0.333 (heap 0.335) | reported, 0.643 (heap 0.644) |
| 7 (n 32, rho 16) | loss, 0.830 [0.805, 0.847], h 0.050 | tie, 0.986 [0.940, 1.011], h 0.050 | reported, 0.980 [0.935, 1.005] | reported, 0.943 [0.880, 1.016] | win, 0.167 (heap 0.169) | reported, 0.464 (heap 0.466) |
| 8 (n 32, rho 64) | loss, 0.867 [0.856, 0.877], h 0.050 | tie, 0.990 [0.973, 0.997], h 0.050 | reported, 0.986 [0.969, 0.992] | reported, 0.960 [0.926, 1.013] | win, 0.125 (heap 0.128) | reported, 0.402 (heap 0.404) |
| 9 (n 64, rho 1) | loss, 0.646 [0.641, 0.650], h 0.050 | tie, 0.998 [0.987, 1.006], h 0.050 | reported, 0.989 [0.977, 0.998] | reported, 1.095 [1.033, 1.164] | win, 0.556 (heap 0.556) | reported, 0.714 (heap 0.715) |
| 10 (n 64, rho 4) | loss, 0.822 [0.817, 0.835], h 0.050 | tie, 0.998 [0.995, 1.013], h 0.050 | reported, 0.994 [0.990, 1.010] | reported, 1.110 [1.050, 1.238] | win, 0.222 (heap 0.223) | reported, 0.500 (heap 0.500) |
| 11 (n 64, rho 16) | loss, 0.860 [0.854, 0.870], h 0.050 | tie, 0.997 [0.994, 1.002], h 0.050 | reported, 0.995 [0.992, 0.999] | reported, 1.056 [0.967, 1.117] | win, 0.167 (heap 0.167) | reported, 0.464 (heap 0.465) |
| 12 (n 64, rho 64) | loss, 0.882 [0.875, 0.898], h 0.050 | tie, 1.000 [0.996, 1.003], h 0.050 | reported, 0.998 [0.994, 1.001] | reported, 1.035 [1.025, 1.121] | win, 0.125 (heap 0.126) | reported, 0.438 (heap 0.438) |
| 13 (n 128, rho 1) | loss, 0.684 [0.679, 0.690], h 0.050 | tie, 1.002 [0.995, 1.008], h 0.050 | reported, 0.997 [0.990, 1.003] | reported, 2.868 [2.600, 3.340] | win, 0.600 (heap 0.600) | reported, 0.762 (heap 0.762) |
| 14 (n 128, rho 4) | loss, 0.862 [0.852, 0.870], h 0.050 | tie, 1.002 [0.997, 1.005], h 0.050 | reported, 1.001 [0.996, 1.003] | reported, 2.348 [1.999, 2.736] | win, 0.300 (heap 0.300) | reported, 0.571 (heap 0.572) |
| 15 (n 128, rho 16) | loss, 0.890 [0.884, 0.894], h 0.050 | tie, 0.999 [0.994, 1.016], h 0.050 | reported, 0.998 [0.993, 1.018] | reported, 2.719 [2.123, 3.278] | win, 0.225 (heap 0.225) | reported, 0.524 (heap 0.524) |
| 16 (n 128, rho 64) | loss, 0.940 [0.931, 0.947], h 0.050 | tie, 1.000 [0.994, 1.005], h 0.050 | reported, 1.000 [0.994, 1.005] | reported, 3.173 [2.698, 3.868] | win, 0.212 (heap 0.213) | reported, 0.516 (heap 0.516) |

### Hash-V-pack against Hash-V-grow-map (pack against grow-map)

| job | 1 build time of one input matrix | 5 product-of-sums time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 1.405 [1.393, 1.421] | reported, 1.029 [1.024, 1.047] | reported, 1.062 [1.054, 1.081] | reported, 0.922 [0.909, 0.939] | reported, 0.357 (heap 0.367) | reported, 0.507 (heap 0.515) |
| 2 (n 16, rho 4) | reported, 1.086 [1.077, 1.093] | reported, 1.022 [1.018, 1.026] | reported, 1.026 [1.022, 1.030] | reported, 0.895 [0.879, 0.919] | reported, 0.396 (heap 0.422) | reported, 0.618 (heap 0.629) |
| 3 (n 16, rho 16) | reported, 0.999 [0.991, 1.009] | reported, 1.014 [1.010, 1.017] | reported, 1.013 [1.009, 1.016] | reported, 0.895 [0.871, 0.913] | reported, 0.478 (heap 0.525) | reported, 0.759 (heap 0.770) |
| 4 (n 16, rho 64) | reported, 0.931 [0.921, 0.946] | reported, 1.009 [0.999, 1.015] | reported, 1.006 [0.995, 1.011] | reported, 0.898 [0.884, 0.909] | reported, 0.495 (heap 0.550) | reported, 0.795 (heap 0.806) |
| 5 (n 32, rho 1) | reported, 1.157 [1.141, 1.163] | reported, 1.004 [0.986, 1.011] | reported, 1.012 [0.994, 1.017] | reported, 0.890 [0.872, 0.970] | reported, 0.594 (heap 0.597) | reported, 0.660 (heap 0.662) |
| 6 (n 32, rho 4) | reported, 1.142 [1.133, 1.150] | reported, 1.011 [0.989, 1.013] | reported, 1.016 [0.994, 1.019] | reported, 0.883 [0.867, 0.931] | reported, 0.517 (heap 0.523) | reported, 0.649 (heap 0.652) |
| 7 (n 32, rho 16) | reported, 1.076 [1.060, 1.098] | reported, 1.003 [0.999, 1.049] | reported, 1.005 [1.001, 1.051] | reported, 0.911 [0.876, 0.977] | reported, 0.449 (heap 0.462) | reported, 0.754 (heap 0.757) |
| 8 (n 32, rho 64) | reported, 1.032 [1.019, 1.047] | reported, 1.007 [0.993, 1.024] | reported, 1.007 [0.993, 1.024] | reported, 0.889 [0.831, 0.966] | reported, 0.490 (heap 0.506) | reported, 0.795 (heap 0.798) |
| 9 (n 64, rho 1) | reported, 1.155 [1.152, 1.162] | reported, 0.997 [0.995, 1.008] | reported, 1.001 [0.998, 1.012] | reported, 0.946 [0.905, 0.964] | reported, 0.545 (heap 0.546) | reported, 0.697 (heap 0.698) |
| 10 (n 64, rho 4) | reported, 1.054 [1.039, 1.061] | reported, 0.999 [0.968, 1.003] | reported, 1.000 [0.969, 1.005] | reported, 0.882 [0.842, 0.951] | reported, 0.747 (heap 0.748) | reported, 0.820 (heap 0.821) |
| 11 (n 64, rho 16) | reported, 1.051 [1.041, 1.062] | reported, 1.000 [0.993, 1.003] | reported, 1.001 [0.995, 1.004] | reported, 0.960 [0.932, 1.036] | reported, 0.758 (heap 0.760) | reported, 0.827 (heap 0.827) |
| 12 (n 64, rho 64) | reported, 1.031 [1.022, 1.041] | reported, 1.001 [0.998, 1.005] | reported, 1.001 [0.999, 1.005] | reported, 0.904 [0.847, 0.937] | reported, 0.483 (heap 0.486) | reported, 0.754 (heap 0.755) |
| 13 (n 128, rho 1) | reported, 1.190 [1.181, 1.201] | reported, 0.999 [0.995, 1.006] | reported, 1.003 [0.997, 1.009] | reported, 1.048 [0.977, 1.094] | reported, 0.373 (heap 0.374) | reported, 0.599 (heap 0.599) |
| 14 (n 128, rho 4) | reported, 1.034 [1.017, 1.050] | reported, 0.998 [0.992, 1.000] | reported, 0.999 [0.993, 1.001] | reported, 1.035 [0.955, 1.158] | reported, 0.477 (heap 0.478) | reported, 0.709 (heap 0.709) |
| 15 (n 128, rho 16) | reported, 1.038 [1.028, 1.049] | reported, 0.995 [0.991, 0.998] | reported, 0.995 [0.991, 0.998] | reported, 1.072 [0.984, 1.257] | reported, 0.500 (heap 0.500) | reported, 0.736 (heap 0.736) |
| 16 (n 128, rho 64) | reported, 0.973 [0.961, 0.996] | reported, 0.998 [0.995, 1.001] | reported, 0.998 [0.995, 1.000] | reported, 0.918 [0.860, 1.057] | reported, 0.487 (heap 0.487) | reported, 0.731 (heap 0.731) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m2 item 7); the counting runs' per-operation records of every hash row equal TC's, every step; Hash-V-pack and Hash-V-grow-map take Hash-V-fixed's arithmetic counters and store splits (spec v0.2 §15.7 items 3-4).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | G(Hash-V-pack)/G(TC), call for call | pack vs fixed | grow-map vs fixed |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 41.05 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 2 | 41.25 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 3 | 41.12 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 4 | 39.66 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 5 | 42.86 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 6 | 42.22 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 7 | 42.61 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 8 | 41.83 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 9 | 44.09 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 10 | 44.15 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 11 | 44.09 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 12 | 44.26 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 13 | 44.88 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 14 | 44.82 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 15 | 44.73 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 16 | 44.74 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m2 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.692 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| NONE | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| NONE | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.455 | 0 |
| NONE | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.891 | 0 |
| NONE | Hash-V-grow-map vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 1.375 | 1 |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.528 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.891 | 0 |
| NONE | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.712 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.735 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.316 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.636 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | stable | 3 | 1 | 0 | 0 | 0 | 0 | 0.578 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.173 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.459 | 0 |
| LOW | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.708 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.900 | 0 |
| LOW | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| LOW | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.557 | 0 |
| LOW | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| LOW | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.750 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.611 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.895 | 0 |
| LOW | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.366 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.862 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.994 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.155 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.788 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.990 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.070 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.005 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.497 | 0 |
| MID | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.744 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.900 | 0 |
| MID | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| MID | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.634 | 0 |
| MID | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.893 | 0 |
| MID | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.375 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.674 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.894 | 0 |
| MID | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.230 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.096 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.861 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.167 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.045 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.001 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.489 | 0 |
| HIGH | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.753 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.899 | 0 |
| HIGH | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| HIGH | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.662 | 0 |
| HIGH | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.895 | 0 |
| HIGH | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.281 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.681 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.898 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.139 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.902 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.997 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.062 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.900 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.995 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.125 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.004 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.489 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-fixed against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.641 | 0.653 | 0.685 | 0.642 |
| 32 | 0.723 | 0.705 | 0.747 | 0.750 |
| 64 | 0.714 | 0.712 | 0.754 | 0.763 |
| 128 | 0.669 | 0.712 | 0.742 | 0.755 |

Hash-V-fixed against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.866 | 0.870 | 0.865 | 0.871 |
| 32 | 0.892 | 0.895 | 0.895 | 0.896 |
| 64 | 0.905 | 0.904 | 0.905 | 0.903 |
| 128 | 0.908 | 0.909 | 0.909 | 0.917 |

Hash-V-fixed against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 2.250 | 2.250 | 2.250 |
| 32 | 2.250 | 2.250 | 2.250 | 2.250 |
| 64 | 2.250 | 2.250 | 2.250 | 2.250 |
| 128 | 2.500 | 2.500 | 2.500 | 2.500 |

Hash-V-grow-map against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.319 | 0.493 | 0.590 | 0.589 |
| 32 | 0.453 | 0.530 | 0.620 | 0.650 |
| 64 | 0.461 | 0.585 | 0.648 | 0.673 |
| 128 | 0.457 | 0.614 | 0.660 | 0.710 |

Hash-V-grow-map against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.830 | 0.837 | 0.844 | 0.847 |
| 32 | 0.879 | 0.879 | 0.883 | 0.887 |
| 64 | 0.902 | 0.902 | 0.903 | 0.903 |
| 128 | 0.910 | 0.911 | 0.908 | 0.917 |

Hash-V-grow-map against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 0.750 | 0.375 | 0.281 |
| 32 | 1.250 | 0.750 | 0.375 | 0.281 |
| 64 | 1.250 | 0.500 | 0.375 | 0.281 |
| 128 | 1.500 | 0.750 | 0.562 | 0.531 |

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.448 | 0.535 | 0.590 | 0.549 |
| 32 | 0.524 | 0.605 | 0.667 | 0.671 |
| 64 | 0.533 | 0.617 | 0.681 | 0.694 |
| 128 | 0.544 | 0.635 | 0.685 | 0.691 |

Hash-V-pack against TC, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.854 | 0.855 | 0.856 | 0.854 |
| 32 | 0.883 | 0.888 | 0.885 | 0.893 |
| 64 | 0.900 | 0.901 | 0.902 | 0.903 |
| 128 | 0.909 | 0.909 | 0.904 | 0.915 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.804 | 0.297 | 0.179 | 0.139 |
| 32 | 0.742 | 0.388 | 0.168 | 0.138 |
| 64 | 0.681 | 0.374 | 0.284 | 0.136 |
| 128 | 0.560 | 0.358 | 0.281 | 0.259 |

Hash-V-pack against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.699 | 0.819 | 0.861 | 0.854 |
| 32 | 0.724 | 0.858 | 0.893 | 0.894 |
| 64 | 0.746 | 0.866 | 0.904 | 0.910 |
| 128 | 0.814 | 0.891 | 0.924 | 0.915 |

Hash-V-pack against Hash-V-fixed, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.986 | 0.984 | 0.989 | 0.981 |
| 32 | 0.990 | 0.992 | 0.988 | 0.997 |
| 64 | 0.995 | 0.997 | 0.997 | 1.000 |
| 128 | 1.001 | 1.000 | 0.994 | 0.998 |

Hash-V-pack against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.132 | 0.080 | 0.062 |
| 32 | 0.330 | 0.172 | 0.075 | 0.061 |
| 64 | 0.303 | 0.166 | 0.126 | 0.060 |
| 128 | 0.224 | 0.143 | 0.112 | 0.103 |

Hash-V-grow-map against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.497 | 0.755 | 0.862 | 0.918 |
| 32 | 0.626 | 0.751 | 0.830 | 0.867 |
| 64 | 0.646 | 0.822 | 0.860 | 0.882 |
| 128 | 0.684 | 0.862 | 0.890 | 0.940 |

Hash-V-grow-map against Hash-V-fixed, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.958 | 0.962 | 0.975 | 0.972 |
| 32 | 0.986 | 0.982 | 0.986 | 0.990 |
| 64 | 0.998 | 0.998 | 0.997 | 1.000 |
| 128 | 1.002 | 1.002 | 0.999 | 1.000 |

Hash-V-grow-map against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.000 | 0.333 | 0.167 | 0.125 |
| 32 | 0.556 | 0.333 | 0.167 | 0.125 |
| 64 | 0.556 | 0.222 | 0.167 | 0.125 |
| 128 | 0.600 | 0.300 | 0.225 | 0.212 |

Hash-V-pack against Hash-V-grow-map, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.405 | 1.086 | 0.999 | 0.931 |
| 32 | 1.157 | 1.142 | 1.076 | 1.032 |
| 64 | 1.155 | 1.054 | 1.051 | 1.031 |
| 128 | 1.190 | 1.034 | 1.038 | 0.973 |

Hash-V-pack against Hash-V-grow-map, 5 product-of-sums time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.029 | 1.022 | 1.014 | 1.009 |
| 32 | 1.004 | 1.011 | 1.003 | 1.007 |
| 64 | 0.997 | 0.999 | 1.000 | 1.001 |
| 128 | 0.999 | 0.998 | 0.995 | 0.998 |

Hash-V-pack against Hash-V-grow-map, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.396 | 0.478 | 0.495 |
| 32 | 0.594 | 0.517 | 0.449 | 0.490 |
| 64 | 0.545 | 0.747 | 0.758 | 0.483 |
| 128 | 0.373 | 0.477 | 0.500 | 0.487 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (7 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build, over that input's cells; Hash-V-grow-map's growth peak: the job's total at its largest growth, over the operation's cells.

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | C's stable (build peak) | D's stable (build peak) | growth peak |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 1 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 32.00 (32.47) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 1 | Hash-V-grow-map | 36.00 (36.48) | 36.00 (36.48) | 34.29 (34.77) | 36.00 (56.00) | 36.00 (56.00) | 36.00 (56.00) | 36.00 (56.00) | 34.29 (34.77), 42 growths |
| 1 | Hash-V-pack | 12.87 (13.38) | 12.94 (13.46) | 17.38 (17.90) | 12.78 (52.78) | 12.84 (52.84) | 12.91 (52.91) | 12.94 (52.94) | - |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 2 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 32.00 (32.47) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 2 | Hash-V-grow-map | 12.00 (12.48) | 16.80 (17.28) | 20.57 (21.06) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 20.57 (21.06), 34 growths |
| 2 | Hash-V-pack | 4.75 (5.27) | 6.47 (6.99) | 12.72 (13.25) | 4.75 (46.75) | 4.84 (46.84) | 4.62 (46.62) | 4.78 (46.78) | - |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 3 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 32.00 (32.47) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 3 | Hash-V-grow-map | 6.00 (6.48) | 12.00 (12.48) | 14.86 (15.34) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 14.86 (15.34), 24 growths |
| 3 | Hash-V-pack | 2.87 (3.41) | 4.99 (5.53) | 11.27 (11.82) | 2.88 (44.88) | 2.91 (44.91) | 2.81 (44.81) | 2.88 (44.88) | - |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 4 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 32.00 (32.47) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 4 | Hash-V-grow-map | 4.50 (4.97) | 10.80 (11.27) | 12.00 (12.49) | 4.50 (4.50) | 4.50 (4.50) | 4.50 (4.50) | 4.50 (4.50) | 12.00 (12.49), 10 growths |
| 4 | Hash-V-pack | 2.23 (2.73) | 4.41 (4.92) | 9.54 (10.07) | 2.25 (44.25) | 2.22 (44.22) | 2.22 (44.22) | 2.22 (44.22) | - |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 5 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 5 | Hash-V-grow-map | 20.00 (20.12) | 23.20 (23.32) | 25.14 (25.26) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 25.14 (25.26), 52 growths |
| 5 | Hash-V-pack | 11.88 (12.01) | 12.00 (12.13) | 16.59 (16.73) | 11.85 (51.85) | 11.88 (51.88) | 11.84 (51.84) | 11.94 (51.94) | - |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 6 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 6 | Hash-V-grow-map | 12.00 (12.12) | 16.80 (16.92) | 20.57 (20.69) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 20.57 (20.69), 48 growths |
| 6 | Hash-V-pack | 6.21 (6.34) | 7.47 (7.60) | 13.35 (13.48) | 6.20 (46.20) | 6.21 (46.21) | 6.21 (46.21) | 6.21 (46.21) | - |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 7 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 7 | Hash-V-grow-map | 6.00 (6.12) | 12.00 (12.12) | 14.86 (14.98) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 14.86 (14.98), 38 growths |
| 7 | Hash-V-pack | 2.69 (2.82) | 4.65 (4.78) | 11.20 (11.33) | 2.69 (44.69) | 2.70 (44.70) | 2.70 (44.70) | 2.69 (44.69) | - |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 8 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 32.00 (32.12) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 8 | Hash-V-grow-map | 4.50 (4.62) | 10.80 (10.92) | 12.86 (12.98) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 12.86 (12.98), 28 growths |
| 8 | Hash-V-pack | 2.21 (2.34) | 4.27 (4.40) | 10.22 (10.35) | 2.21 (44.21) | 2.20 (44.20) | 2.22 (44.22) | 2.20 (44.20) | - |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 9 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 9 | Hash-V-grow-map | 20.00 (20.03) | 23.20 (23.23) | 22.86 (22.89) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 22.86 (22.89), 64 growths |
| 9 | Hash-V-pack | 10.90 (10.93) | 11.15 (11.19) | 15.94 (15.97) | 10.86 (50.86) | 10.88 (50.88) | 10.97 (50.97) | 10.88 (50.88) | - |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 10 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 10 | Hash-V-grow-map | 8.00 (8.03) | 13.60 (13.63) | 16.00 (16.03) | 8.00 (10.50) | 8.00 (10.50) | 8.00 (10.50) | 8.00 (10.50) | 16.00 (16.03), 56 growths |
| 10 | Hash-V-pack | 5.98 (6.01) | 7.22 (7.25) | 13.13 (13.16) | 5.97 (45.97) | 5.98 (45.98) | 5.98 (45.98) | 5.97 (45.97) | - |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 11 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 11 | Hash-V-grow-map | 6.00 (6.03) | 12.00 (12.03) | 14.86 (14.89) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 14.86 (14.89), 52 growths |
| 11 | Hash-V-pack | 4.55 (4.58) | 6.08 (6.11) | 12.28 (12.32) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) | - |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 12 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 32.00 (32.03) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 12 | Hash-V-grow-map | 4.50 (4.53) | 10.80 (10.83) | 14.00 (14.03) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 14.00 (14.03), 44 growths |
| 12 | Hash-V-pack | 2.17 (2.20) | 4.17 (4.20) | 10.56 (10.59) | 2.17 (44.17) | 2.17 (44.17) | 2.17 (44.17) | 2.17 (44.17) | - |
| 13 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 13 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 13 | Hash-V-grow-map | 24.00 (24.01) | 27.20 (27.21) | 27.43 (27.44) | 24.00 (36.00) | 24.00 (36.00) | 24.00 (36.00) | 24.00 (36.00) | 27.43 (27.44), 78 growths |
| 13 | Hash-V-pack | 8.96 (8.97) | 9.58 (9.59) | 16.44 (16.45) | 8.91 (60.91) | 8.96 (60.96) | 9.00 (61.00) | 8.97 (60.97) | - |
| 14 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 14 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 14 | Hash-V-grow-map | 12.00 (12.01) | 17.60 (17.61) | 20.57 (20.58) | 12.00 (15.00) | 12.00 (15.00) | 12.00 (15.00) | 12.00 (15.00) | 20.57 (20.58), 70 growths |
| 14 | Hash-V-pack | 5.73 (5.74) | 6.99 (7.00) | 14.59 (14.59) | 5.74 (57.74) | 5.72 (57.72) | 5.73 (57.73) | 5.73 (57.73) | - |
| 15 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 15 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 15 | Hash-V-grow-map | 9.00 (9.01) | 15.20 (15.21) | 18.86 (18.86) | 9.00 (9.75) | 9.00 (9.75) | 9.00 (9.75) | 9.00 (9.75) | 18.86 (18.86), 62 growths |
| 15 | Hash-V-pack | 4.50 (4.50) | 6.01 (6.02) | 13.88 (13.88) | 4.50 (56.50) | 4.49 (56.49) | 4.50 (56.50) | 4.50 (56.50) | - |
| 16 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 16 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 36.00 (36.01) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 16 | Hash-V-grow-map | 8.50 (8.51) | 14.80 (14.81) | 18.57 (18.58) | 8.50 (8.88) | 8.50 (8.88) | 8.50 (8.88) | 8.50 (8.88) | 18.57 (18.58), 58 growths |
| 16 | Hash-V-pack | 4.14 (4.15) | 5.72 (5.73) | 13.57 (13.58) | 4.14 (56.14) | 4.14 (56.14) | 4.14 (56.14) | 4.14 (56.14) | - |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B | C | D |
| --- | --- | --- | --- | --- | --- |
| 1 | TC | 6708 | 6584 | 6295 | 6407 |
| 1 | Hash-V-fixed | 10300 | 10195 | 10030 | 9960 |
| 1 | Hash-V-grow-map | 20604 | 20510 | 20187 | 19815 |
| 1 | Hash-V-pack | 14605 | 14587 | 14405 | 14228 |
| 1 | Hash-V-fixed-null | 10232 | 10300 | 10103 | 9999 |
| 2 | TC | 6747 | 6383 | 6346 | 6379 |
| 2 | Hash-V-fixed | 10155 | 9674 | 9740 | 10051 |
| 2 | Hash-V-grow-map | 13154 | 12542 | 13073 | 13132 |
| 2 | Hash-V-pack | 12509 | 11855 | 11794 | 12127 |
| 2 | Hash-V-fixed-null | 10120 | 9649 | 9751 | 9975 |
| 3 | TC | 5846 | 6139 | 6087 | 5601 |
| 3 | Hash-V-fixed | 8353 | 8838 | 8979 | 8498 |
| 3 | Hash-V-grow-map | 9906 | 9993 | 10592 | 9736 |
| 3 | Hash-V-pack | 9815 | 10054 | 10403 | 9837 |
| 3 | Hash-V-fixed-null | 8327 | 8814 | 8919 | 8416 |
| 4 | TC | 4772 | 4317 | 4091 | 4884 |
| 4 | Hash-V-fixed | 7102 | 6957 | 6189 | 7514 |
| 4 | Hash-V-grow-map | 7624 | 7779 | 6715 | 8307 |
| 4 | Hash-V-pack | 8306 | 8140 | 7259 | 8632 |
| 4 | Hash-V-fixed-null | 7157 | 6990 | 6181 | 7562 |
| 5 | TC | 29873 | 29186 | 29442 | 29510 |
| 5 | Hash-V-fixed | 41516 | 40384 | 40763 | 40773 |
| 5 | Hash-V-grow-map | 66230 | 64969 | 64506 | 65726 |
| 5 | Hash-V-pack | 57023 | 55790 | 56089 | 56564 |
| 5 | Hash-V-fixed-null | 41685 | 40362 | 40827 | 40992 |
| 6 | TC | 30000 | 29312 | 29059 | 29098 |
| 6 | Hash-V-fixed | 41780 | 41919 | 40936 | 41158 |
| 6 | Hash-V-grow-map | 56535 | 55744 | 54288 | 54846 |
| 6 | Hash-V-pack | 48793 | 48719 | 48451 | 48120 |
| 6 | Hash-V-fixed-null | 41736 | 41852 | 40846 | 41203 |
| 7 | TC | 27470 | 27890 | 29500 | 28849 |
| 7 | Hash-V-fixed | 37597 | 38290 | 39524 | 38069 |
| 7 | Hash-V-grow-map | 45580 | 45047 | 48144 | 46544 |
| 7 | Hash-V-pack | 42689 | 43005 | 44153 | 43043 |
| 7 | Hash-V-fixed-null | 37340 | 38050 | 39274 | 38046 |
| 8 | TC | 23906 | 26559 | 25964 | 26017 |
| 8 | Hash-V-fixed | 32841 | 35344 | 34259 | 34897 |
| 8 | Hash-V-grow-map | 36369 | 40525 | 39682 | 40092 |
| 8 | Hash-V-pack | 37429 | 39071 | 38861 | 38609 |
| 8 | Hash-V-fixed-null | 32834 | 35272 | 34250 | 34779 |
| 9 | TC | 118191 | 117642 | 117593 | 117808 |
| 9 | Hash-V-fixed | 166365 | 165004 | 163446 | 164498 |
| 9 | Hash-V-grow-map | 255764 | 256488 | 252422 | 254686 |
| 9 | Hash-V-pack | 222332 | 222028 | 219678 | 218991 |
| 9 | Hash-V-fixed-null | 166492 | 165779 | 163855 | 163898 |
| 10 | TC | 118726 | 117040 | 117358 | 117301 |
| 10 | Hash-V-fixed | 165427 | 165150 | 164382 | 164380 |
| 10 | Hash-V-grow-map | 203566 | 201416 | 197814 | 198210 |
| 10 | Hash-V-pack | 190955 | 190745 | 190297 | 189961 |
| 10 | Hash-V-fixed-null | 164575 | 164906 | 164065 | 165778 |
| 11 | TC | 116637 | 115673 | 114879 | 117033 |
| 11 | Hash-V-fixed | 153755 | 154288 | 153604 | 154470 |
| 11 | Hash-V-grow-map | 177114 | 177205 | 181020 | 179682 |
| 11 | Hash-V-pack | 170512 | 169981 | 169378 | 170757 |
| 11 | Hash-V-fixed-null | 154188 | 153955 | 152973 | 154343 |
| 12 | TC | 110884 | 113993 | 110548 | 118578 |
| 12 | Hash-V-fixed | 147158 | 147701 | 144626 | 154922 |
| 12 | Hash-V-grow-map | 167090 | 165192 | 166928 | 176198 |
| 12 | Hash-V-pack | 161698 | 162556 | 159240 | 168696 |
| 12 | Hash-V-fixed-null | 147563 | 150429 | 144175 | 153866 |
| 13 | TC | 477726 | 474516 | 480045 | 479272 |
| 13 | Hash-V-fixed | 715000 | 714324 | 712072 | 712954 |
| 13 | Hash-V-grow-map | 1044463 | 1043399 | 1044178 | 1050506 |
| 13 | Hash-V-pack | 895433 | 870387 | 877454 | 860568 |
| 13 | Hash-V-fixed-null | 713918 | 713493 | 712856 | 715408 |
| 14 | TC | 475699 | 475730 | 478455 | 476760 |
| 14 | Hash-V-fixed | 672204 | 667343 | 668498 | 672338 |
| 14 | Hash-V-grow-map | 785010 | 773272 | 777203 | 769327 |
| 14 | Hash-V-pack | 768626 | 748921 | 747486 | 741778 |
| 14 | Hash-V-fixed-null | 668890 | 667455 | 665573 | 670943 |
| 15 | TC | 478122 | 474732 | 479866 | 468964 |
| 15 | Hash-V-fixed | 645574 | 642518 | 638385 | 634930 |
| 15 | Hash-V-grow-map | 720544 | 726231 | 718526 | 717426 |
| 15 | Hash-V-pack | 713026 | 698606 | 687214 | 682898 |
| 15 | Hash-V-fixed-null | 640681 | 638267 | 634558 | 634874 |
| 16 | TC | 471234 | 452794 | 464846 | 464840 |
| 16 | Hash-V-fixed | 634192 | 605018 | 615838 | 615076 |
| 16 | Hash-V-grow-map | 678028 | 644927 | 661724 | 643340 |
| 16 | Hash-V-pack | 684761 | 649534 | 672606 | 660235 |
| 16 | Hash-V-fixed-null | 627514 | 606924 | 618872 | 618940 |

## Stores, per job and matrix (rule-m2 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. Hash-V-pack's T is its build table's.

| job | row | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | cells bytes | table or list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Hash-V-fixed | A | 281 | 1024 | 27.4% | 64 | 16 | 231 / 281 | 1.10 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.14 | 10 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | C | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.12 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | D | 286 | 1024 | 27.9% | 64 | 16 | 226 / 286 | 1.09 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | S | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.13 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 1 | Hash-V-fixed | U | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.10 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 1 | Hash-V-fixed | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.13 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-grow-map | A | 281 | 1024 | 27.4% | 64 | 16 | 231 / 281 | 1.51 | 10 | 0 (0) | 6 | 504 (584) | 814 | 1024 | 8192 |
| 1 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.51 | 12 | 0 (0) | 6 | 504 (602) | 805 | 1024 | 8192 |
| 1 | Hash-V-grow-map | C | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.46 | 10 | 0 (0) | 6 | 504 (581) | 804 | 1024 | 8192 |
| 1 | Hash-V-grow-map | D | 286 | 1024 | 27.9% | 64 | 16 | 226 / 286 | 1.40 | 14 | 0 (0) | 6 | 504 (578) | 805 | 1024 | 8192 |
| 1 | Hash-V-grow-map | S | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.50 | 11 | 1 (16) | 6 | 504 (590) | 794 | 1024 | 4096 |
| 1 | Hash-V-grow-map | U | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.45 | 8 | 1 (16) | 6 | 504 (571) | 790 | 1024 | 4096 |
| 1 | Hash-V-grow-map | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.67 | 15 | 1 (16) | 6 | 504 (605) | 805 | 1024 | 8192 |
| 1 | Hash-V-pack | A | 281 | 1024 | 27.4% | 64 | 16 | 231 / 281 | 1.10 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2248 |
| 1 | Hash-V-pack | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.14 | 10 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2264 |
| 1 | Hash-V-pack | C | 285 | 1024 | 27.8% | 64 | 16 | 227 / 285 | 1.12 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2280 |
| 1 | Hash-V-pack | D | 286 | 1024 | 27.9% | 64 | 16 | 226 / 286 | 1.09 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2288 |
| 1 | Hash-V-pack | S | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.13 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 1144 |
| 1 | Hash-V-pack | U | 286 | 1024 | 27.9% | 32 | 16 | 226 / 286 | 1.10 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 1144 |
| 1 | Hash-V-pack | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.13 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2368 |
| 2 | Hash-V-fixed | A | 88 | 1024 | 8.6% | 64 | 16 | 424 / 88 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | B | 91 | 1024 | 8.9% | 64 | 16 | 421 / 91 | 1.03 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | C | 84 | 1024 | 8.2% | 64 | 16 | 428 / 84 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | D | 89 | 1024 | 8.7% | 64 | 16 | 423 / 89 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | S | 280 | 1024 | 27.3% | 32 | 16 | 232 / 280 | 1.12 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 2 | Hash-V-fixed | U | 276 | 1024 | 27.0% | 32 | 16 | 236 / 276 | 1.12 | 4 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 2 | Hash-V-fixed | R | 300 | 1024 | 29.3% | 64 | 16 | 212 / 300 | 1.14 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-grow-map | A | 88 | 256 | 34.4% | 64 | 16 | 424 / 88 | 1.19 | 5 | 0 (0) | 4 | 120 (127) | 237 | 1024 | 2048 |
| 2 | Hash-V-grow-map | B | 91 | 256 | 35.5% | 64 | 16 | 421 / 91 | 1.17 | 5 | 0 (0) | 4 | 120 (128) | 212 | 1024 | 2048 |
| 2 | Hash-V-grow-map | C | 84 | 256 | 32.8% | 64 | 16 | 428 / 84 | 1.27 | 8 | 0 (0) | 4 | 120 (134) | 230 | 1024 | 2048 |
| 2 | Hash-V-grow-map | D | 89 | 256 | 34.8% | 64 | 16 | 423 / 89 | 1.19 | 5 | 0 (0) | 4 | 120 (132) | 223 | 1024 | 2048 |
| 2 | Hash-V-grow-map | S | 280 | 1024 | 27.3% | 32 | 16 | 232 / 280 | 1.51 | 8 | 1 (16) | 6 | 504 (603) | 819 | 1024 | 4096 |
| 2 | Hash-V-grow-map | U | 276 | 1024 | 27.0% | 32 | 16 | 236 / 276 | 1.48 | 15 | 2 (32) | 6 | 504 (551) | 826 | 1024 | 4096 |
| 2 | Hash-V-grow-map | R | 300 | 1024 | 29.3% | 64 | 16 | 212 / 300 | 1.40 | 13 | 1 (16) | 6 | 504 (575) | 783 | 1024 | 8192 |
| 2 | Hash-V-pack | A | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 704 |
| 2 | Hash-V-pack | B | 91 | 1024 | 8.9% | 64 | 8 | 421 / 91 | 1.03 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 728 |
| 2 | Hash-V-pack | C | 84 | 1024 | 8.2% | 64 | 8 | 428 / 84 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 672 |
| 2 | Hash-V-pack | D | 89 | 1024 | 8.7% | 64 | 8 | 423 / 89 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 712 |
| 2 | Hash-V-pack | S | 280 | 1024 | 27.3% | 32 | 16 | 232 / 280 | 1.12 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 1120 |
| 2 | Hash-V-pack | U | 276 | 1024 | 27.0% | 32 | 16 | 236 / 276 | 1.12 | 4 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 1104 |
| 2 | Hash-V-pack | R | 300 | 1024 | 29.3% | 64 | 16 | 212 / 300 | 1.14 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2400 |
| 3 | Hash-V-fixed | A | 28 | 1024 | 2.7% | 64 | 16 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | B | 29 | 1024 | 2.8% | 64 | 16 | 483 / 29 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | C | 26 | 1024 | 2.5% | 64 | 16 | 486 / 26 | 1.09 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | D | 28 | 1024 | 2.7% | 64 | 16 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | S | 192 | 1024 | 18.8% | 32 | 16 | 320 / 192 | 1.07 | 3 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 3 | Hash-V-fixed | U | 188 | 1024 | 18.4% | 32 | 16 | 324 / 188 | 1.10 | 4 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 3 | Hash-V-fixed | R | 304 | 1024 | 29.7% | 64 | 16 | 208 / 304 | 1.12 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-grow-map | A | 28 | 64 | 43.8% | 64 | 16 | 484 / 28 | 1.26 | 4 | 0 (0) | 2 | 24 (24) | 36 | 1024 | 512 |
| 3 | Hash-V-grow-map | B | 29 | 64 | 45.3% | 64 | 16 | 483 / 29 | 1.23 | 5 | 0 (0) | 2 | 24 (25) | 35 | 1024 | 512 |
| 3 | Hash-V-grow-map | C | 26 | 64 | 40.6% | 64 | 16 | 486 / 26 | 1.47 | 6 | 0 (0) | 2 | 24 (26) | 25 | 1024 | 512 |
| 3 | Hash-V-grow-map | D | 28 | 64 | 43.8% | 64 | 16 | 484 / 28 | 1.16 | 2 | 0 (0) | 2 | 24 (25) | 38 | 1024 | 512 |
| 3 | Hash-V-grow-map | S | 192 | 512 | 37.5% | 32 | 16 | 320 / 192 | 1.37 | 11 | 2 (32) | 5 | 248 (270) | 419 | 1024 | 2048 |
| 3 | Hash-V-grow-map | U | 188 | 512 | 36.7% | 32 | 16 | 324 / 188 | 1.39 | 6 | 2 (32) | 5 | 248 (293) | 405 | 1024 | 2048 |
| 3 | Hash-V-grow-map | R | 304 | 1024 | 29.7% | 64 | 16 | 208 / 304 | 1.46 | 12 | 1 (16) | 6 | 504 (585) | 754 | 1024 | 8192 |
| 3 | Hash-V-pack | A | 28 | 1024 | 2.7% | 64 | 8 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 224 |
| 3 | Hash-V-pack | B | 29 | 1024 | 2.8% | 64 | 8 | 483 / 29 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 232 |
| 3 | Hash-V-pack | C | 26 | 1024 | 2.5% | 64 | 8 | 486 / 26 | 1.09 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 208 |
| 3 | Hash-V-pack | D | 28 | 1024 | 2.7% | 64 | 8 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 224 |
| 3 | Hash-V-pack | S | 192 | 1024 | 18.8% | 32 | 16 | 320 / 192 | 1.07 | 3 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 768 |
| 3 | Hash-V-pack | U | 188 | 1024 | 18.4% | 32 | 16 | 324 / 188 | 1.10 | 4 | 2 (2048) | 0 | 0 (0) | 0 | 1024 | 752 |
| 3 | Hash-V-pack | R | 304 | 1024 | 29.7% | 64 | 16 | 208 / 304 | 1.12 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2432 |
| 4 | Hash-V-fixed | A | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | B | 7 | 1024 | 0.7% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | C | 7 | 1024 | 0.7% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | D | 7 | 1024 | 0.7% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | S | 26 | 1024 | 2.5% | 32 | 16 | 486 / 26 | 1.00 | 1 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 4 | Hash-V-fixed | U | 23 | 1024 | 2.2% | 32 | 16 | 489 / 23 | 1.00 | 1 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 4096 |
| 4 | Hash-V-fixed | R | 292 | 1024 | 28.5% | 64 | 16 | 220 / 292 | 1.13 | 9 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-grow-map | A | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.48 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | B | 7 | 16 | 43.8% | 64 | 16 | 505 / 7 | 1.14 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | C | 7 | 16 | 43.8% | 64 | 16 | 505 / 7 | 1.12 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | D | 7 | 16 | 43.8% | 64 | 16 | 505 / 7 | 1.60 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | S | 26 | 64 | 40.6% | 32 | 16 | 486 / 26 | 1.42 | 5 | 1 (16) | 2 | 24 (28) | 42 | 1024 | 256 |
| 4 | Hash-V-grow-map | U | 23 | 64 | 35.9% | 32 | 16 | 489 / 23 | 1.30 | 5 | 1 (16) | 2 | 24 (30) | 48 | 1024 | 256 |
| 4 | Hash-V-grow-map | R | 292 | 1024 | 28.5% | 64 | 16 | 220 / 292 | 1.36 | 9 | 1 (16) | 6 | 504 (590) | 808 | 1024 | 8192 |
| 4 | Hash-V-pack | A | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | B | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 56 |
| 4 | Hash-V-pack | C | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 56 |
| 4 | Hash-V-pack | D | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 56 |
| 4 | Hash-V-pack | S | 26 | 1024 | 2.5% | 32 | 8 | 486 / 26 | 1.00 | 1 | 1 (1024) | 0 | 0 (0) | 0 | 512 | 104 |
| 4 | Hash-V-pack | U | 23 | 1024 | 2.2% | 32 | 8 | 489 / 23 | 1.00 | 1 | 1 (1024) | 0 | 0 (0) | 0 | 512 | 92 |
| 4 | Hash-V-pack | R | 292 | 1024 | 28.5% | 64 | 16 | 220 / 292 | 1.13 | 9 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2336 |
| 5 | Hash-V-fixed | A | 1005 | 4096 | 24.5% | 64 | 16 | 1043 / 1005 | 1.07 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | B | 1008 | 4096 | 24.6% | 64 | 16 | 1040 / 1008 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | C | 1004 | 4096 | 24.5% | 64 | 16 | 1044 / 1004 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | D | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | S | 1040 | 4096 | 25.4% | 32 | 16 | 1008 / 1040 | 1.09 | 6 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 5 | Hash-V-fixed | U | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.10 | 7 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 5 | Hash-V-fixed | R | 1088 | 4096 | 26.6% | 64 | 16 | 960 / 1088 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-grow-map | A | 1005 | 2048 | 49.1% | 64 | 16 | 1043 / 1005 | 1.44 | 14 | 0 (0) | 7 | 1016 (1186) | 1784 | 4096 | 16384 |
| 5 | Hash-V-grow-map | B | 1008 | 2048 | 49.2% | 64 | 16 | 1040 / 1008 | 1.36 | 13 | 0 (0) | 7 | 1016 (1141) | 1805 | 4096 | 16384 |
| 5 | Hash-V-grow-map | C | 1004 | 2048 | 49.0% | 64 | 16 | 1044 / 1004 | 1.35 | 16 | 0 (0) | 7 | 1016 (1127) | 1800 | 4096 | 16384 |
| 5 | Hash-V-grow-map | D | 1016 | 2048 | 49.6% | 64 | 16 | 1032 / 1016 | 1.35 | 10 | 0 (0) | 7 | 1016 (1158) | 1780 | 4096 | 16384 |
| 5 | Hash-V-grow-map | S | 1040 | 4096 | 25.4% | 32 | 16 | 1008 / 1040 | 1.40 | 16 | 2 (32) | 8 | 2040 (2365) | 3787 | 4096 | 16384 |
| 5 | Hash-V-grow-map | U | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.46 | 16 | 2 (32) | 8 | 2040 (2381) | 3762 | 4096 | 16384 |
| 5 | Hash-V-grow-map | R | 1088 | 4096 | 26.6% | 64 | 16 | 960 / 1088 | 1.43 | 13 | 1 (16) | 8 | 2040 (2371) | 3673 | 4096 | 32768 |
| 5 | Hash-V-pack | A | 1005 | 4096 | 24.5% | 64 | 16 | 1043 / 1005 | 1.07 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8040 |
| 5 | Hash-V-pack | B | 1008 | 4096 | 24.6% | 64 | 16 | 1040 / 1008 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8064 |
| 5 | Hash-V-pack | C | 1004 | 4096 | 24.5% | 64 | 16 | 1044 / 1004 | 1.06 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8032 |
| 5 | Hash-V-pack | D | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8128 |
| 5 | Hash-V-pack | S | 1040 | 4096 | 25.4% | 32 | 16 | 1008 / 1040 | 1.09 | 6 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 4160 |
| 5 | Hash-V-pack | U | 1044 | 4096 | 25.5% | 32 | 16 | 1004 / 1044 | 1.10 | 7 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 4176 |
| 5 | Hash-V-pack | R | 1088 | 4096 | 26.6% | 64 | 16 | 960 / 1088 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8704 |
| 6 | Hash-V-fixed | A | 282 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | B | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.02 | 3 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | C | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | S | 1036 | 4096 | 25.3% | 32 | 16 | 1012 / 1036 | 1.11 | 6 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 6 | Hash-V-fixed | U | 1041 | 4096 | 25.4% | 32 | 16 | 1007 / 1041 | 1.09 | 9 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 6 | Hash-V-fixed | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.09 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-grow-map | A | 282 | 1024 | 27.5% | 64 | 16 | 1766 / 282 | 1.25 | 16 | 0 (0) | 6 | 504 (598) | 1505 | 4096 | 8192 |
| 6 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.26 | 15 | 0 (0) | 6 | 504 (581) | 1406 | 4096 | 8192 |
| 6 | Hash-V-grow-map | C | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.22 | 9 | 0 (0) | 6 | 504 (585) | 1478 | 4096 | 8192 |
| 6 | Hash-V-grow-map | D | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.23 | 14 | 0 (0) | 6 | 504 (587) | 1443 | 4096 | 8192 |
| 6 | Hash-V-grow-map | S | 1036 | 4096 | 25.3% | 32 | 16 | 1012 / 1036 | 1.50 | 17 | 2 (32) | 8 | 2040 (2416) | 3787 | 4096 | 16384 |
| 6 | Hash-V-grow-map | U | 1041 | 4096 | 25.4% | 32 | 16 | 1007 / 1041 | 1.44 | 16 | 1 (16) | 8 | 2040 (2401) | 3785 | 4096 | 16384 |
| 6 | Hash-V-grow-map | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.38 | 17 | 1 (16) | 8 | 2040 (2321) | 3629 | 4096 | 32768 |
| 6 | Hash-V-pack | A | 282 | 4096 | 6.9% | 64 | 16 | 1766 / 282 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2256 |
| 6 | Hash-V-pack | B | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.02 | 3 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | C | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | S | 1036 | 4096 | 25.3% | 32 | 16 | 1012 / 1036 | 1.11 | 6 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 4144 |
| 6 | Hash-V-pack | U | 1041 | 4096 | 25.4% | 32 | 16 | 1007 / 1041 | 1.09 | 9 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 4164 |
| 6 | Hash-V-pack | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.09 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8720 |
| 7 | Hash-V-fixed | A | 88 | 4096 | 2.1% | 64 | 16 | 1960 / 88 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | B | 89 | 4096 | 2.2% | 64 | 16 | 1959 / 89 | 1.03 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | C | 90 | 4096 | 2.2% | 64 | 16 | 1958 / 90 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | D | 88 | 4096 | 2.1% | 64 | 16 | 1960 / 88 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | S | 926 | 4096 | 22.6% | 32 | 16 | 1122 / 926 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 7 | Hash-V-fixed | U | 908 | 4096 | 22.2% | 32 | 16 | 1140 / 908 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 7 | Hash-V-fixed | R | 1084 | 4096 | 26.5% | 64 | 16 | 964 / 1084 | 1.09 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-grow-map | A | 88 | 256 | 34.4% | 64 | 16 | 1960 / 88 | 1.22 | 4 | 0 (0) | 4 | 120 (135) | 216 | 4096 | 2048 |
| 7 | Hash-V-grow-map | B | 89 | 256 | 34.8% | 64 | 16 | 1959 / 89 | 1.22 | 15 | 0 (0) | 4 | 120 (138) | 220 | 4096 | 2048 |
| 7 | Hash-V-grow-map | C | 90 | 256 | 35.2% | 64 | 16 | 1958 / 90 | 1.29 | 7 | 0 (0) | 4 | 120 (136) | 211 | 4096 | 2048 |
| 7 | Hash-V-grow-map | D | 88 | 256 | 34.4% | 64 | 16 | 1960 / 88 | 1.27 | 7 | 0 (0) | 4 | 120 (137) | 232 | 4096 | 2048 |
| 7 | Hash-V-grow-map | S | 926 | 2048 | 45.2% | 32 | 16 | 1122 / 926 | 1.34 | 21 | 1 (16) | 7 | 1016 (1144) | 1832 | 4096 | 8192 |
| 7 | Hash-V-grow-map | U | 908 | 2048 | 44.3% | 32 | 16 | 1140 / 908 | 1.34 | 12 | 1 (16) | 7 | 1016 (1184) | 1883 | 4096 | 8192 |
| 7 | Hash-V-grow-map | R | 1084 | 4096 | 26.5% | 64 | 16 | 964 / 1084 | 1.40 | 12 | 1 (16) | 8 | 2040 (2353) | 3666 | 4096 | 32768 |
| 7 | Hash-V-pack | A | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 704 |
| 7 | Hash-V-pack | B | 89 | 4096 | 2.2% | 64 | 8 | 1959 / 89 | 1.03 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 712 |
| 7 | Hash-V-pack | C | 90 | 4096 | 2.2% | 64 | 8 | 1958 / 90 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 720 |
| 7 | Hash-V-pack | D | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 704 |
| 7 | Hash-V-pack | S | 926 | 4096 | 22.6% | 32 | 16 | 1122 / 926 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 3704 |
| 7 | Hash-V-pack | U | 908 | 4096 | 22.2% | 32 | 16 | 1140 / 908 | 1.07 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 3632 |
| 7 | Hash-V-pack | R | 1084 | 4096 | 26.5% | 64 | 16 | 964 / 1084 | 1.09 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8672 |
| 8 | Hash-V-fixed | A | 27 | 4096 | 0.7% | 64 | 16 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | B | 26 | 4096 | 0.6% | 64 | 16 | 2022 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | C | 28 | 4096 | 0.7% | 64 | 16 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | D | 25 | 4096 | 0.6% | 64 | 16 | 2023 / 25 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | S | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.02 | 2 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 8 | Hash-V-fixed | U | 275 | 4096 | 6.7% | 32 | 16 | 1773 / 275 | 1.01 | 2 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 16384 |
| 8 | Hash-V-fixed | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.10 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-grow-map | A | 27 | 64 | 42.2% | 64 | 16 | 2021 / 27 | 1.15 | 6 | 0 (0) | 2 | 24 (27) | 31 | 4096 | 512 |
| 8 | Hash-V-grow-map | B | 26 | 64 | 40.6% | 64 | 16 | 2022 / 26 | 1.35 | 6 | 0 (0) | 2 | 24 (28) | 40 | 4096 | 512 |
| 8 | Hash-V-grow-map | C | 28 | 64 | 43.8% | 64 | 16 | 2020 / 28 | 1.20 | 6 | 0 (0) | 2 | 24 (27) | 39 | 4096 | 512 |
| 8 | Hash-V-grow-map | D | 25 | 64 | 39.1% | 64 | 16 | 2023 / 25 | 1.22 | 4 | 0 (0) | 2 | 24 (24) | 33 | 4096 | 512 |
| 8 | Hash-V-grow-map | S | 280 | 1024 | 27.3% | 32 | 16 | 1768 / 280 | 1.23 | 13 | 2 (32) | 6 | 504 (576) | 1578 | 4096 | 4096 |
| 8 | Hash-V-grow-map | U | 275 | 1024 | 26.9% | 32 | 16 | 1773 / 275 | 1.19 | 13 | 1 (16) | 6 | 504 (564) | 1593 | 4096 | 4096 |
| 8 | Hash-V-grow-map | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.42 | 13 | 1 (16) | 8 | 2040 (2380) | 3613 | 4096 | 32768 |
| 8 | Hash-V-pack | A | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 216 |
| 8 | Hash-V-pack | B | 26 | 4096 | 0.6% | 64 | 8 | 2022 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 208 |
| 8 | Hash-V-pack | C | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 224 |
| 8 | Hash-V-pack | D | 25 | 4096 | 0.6% | 64 | 8 | 2023 / 25 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 200 |
| 8 | Hash-V-pack | S | 280 | 4096 | 6.8% | 32 | 16 | 1768 / 280 | 1.02 | 2 | 2 (8192) | 0 | 0 (0) | 0 | 4096 | 1120 |
| 8 | Hash-V-pack | U | 275 | 4096 | 6.7% | 32 | 16 | 1773 / 275 | 1.01 | 2 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 1100 |
| 8 | Hash-V-pack | R | 1090 | 4096 | 26.6% | 64 | 16 | 958 / 1090 | 1.10 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8720 |
| 9 | Hash-V-fixed | A | 3510 | 16384 | 21.4% | 64 | 16 | 4682 / 3510 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | B | 3524 | 16384 | 21.5% | 64 | 16 | 4668 / 3524 | 1.03 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | C | 3569 | 16384 | 21.8% | 64 | 16 | 4623 / 3569 | 1.02 | 6 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | D | 3522 | 16384 | 21.5% | 64 | 16 | 4670 / 3522 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | S | 3983 | 16384 | 24.3% | 32 | 16 | 4209 / 3983 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 9 | Hash-V-fixed | U | 4011 | 16384 | 24.5% | 32 | 16 | 4181 / 4011 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 9 | Hash-V-fixed | R | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-grow-map | A | 3510 | 8192 | 42.8% | 64 | 16 | 4682 / 3510 | 1.27 | 20 | 0 (0) | 9 | 4088 (4586) | 8355 | 16384 | 65536 |
| 9 | Hash-V-grow-map | B | 3524 | 8192 | 43.0% | 64 | 16 | 4668 / 3524 | 1.26 | 18 | 0 (0) | 9 | 4088 (4667) | 8276 | 16384 | 65536 |
| 9 | Hash-V-grow-map | C | 3569 | 8192 | 43.6% | 64 | 16 | 4623 / 3569 | 1.24 | 15 | 0 (0) | 9 | 4088 (4552) | 8284 | 16384 | 65536 |
| 9 | Hash-V-grow-map | D | 3522 | 8192 | 43.0% | 64 | 16 | 4670 / 3522 | 1.26 | 13 | 0 (0) | 9 | 4088 (4566) | 8331 | 16384 | 65536 |
| 9 | Hash-V-grow-map | S | 3983 | 8192 | 48.6% | 32 | 16 | 4209 / 3983 | 1.38 | 19 | 2 (32) | 9 | 4088 (4717) | 7913 | 16384 | 32768 |
| 9 | Hash-V-grow-map | U | 4011 | 8192 | 49.0% | 32 | 16 | 4181 / 4011 | 1.41 | 28 | 2 (32) | 9 | 4088 (4770) | 7861 | 16384 | 32768 |
| 9 | Hash-V-grow-map | R | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.42 | 19 | 1 (16) | 10 | 8184 (9483) | 15602 | 16384 | 131072 |
| 9 | Hash-V-pack | A | 3510 | 16384 | 21.4% | 64 | 16 | 4682 / 3510 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28080 |
| 9 | Hash-V-pack | B | 3524 | 16384 | 21.5% | 64 | 16 | 4668 / 3524 | 1.03 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28192 |
| 9 | Hash-V-pack | C | 3569 | 16384 | 21.8% | 64 | 16 | 4623 / 3569 | 1.02 | 6 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28552 |
| 9 | Hash-V-pack | D | 3522 | 16384 | 21.5% | 64 | 16 | 4670 / 3522 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28176 |
| 9 | Hash-V-pack | S | 3983 | 16384 | 24.3% | 32 | 16 | 4209 / 3983 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 15932 |
| 9 | Hash-V-pack | U | 4011 | 16384 | 24.5% | 32 | 16 | 4181 / 4011 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 16044 |
| 9 | Hash-V-pack | R | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33496 |
| 10 | Hash-V-fixed | A | 1011 | 16384 | 6.2% | 64 | 16 | 7181 / 1011 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | C | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | D | 1008 | 16384 | 6.2% | 64 | 16 | 7184 / 1008 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | S | 4012 | 16384 | 24.5% | 32 | 16 | 4180 / 4012 | 1.06 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 10 | Hash-V-fixed | U | 3989 | 16384 | 24.3% | 32 | 16 | 4203 / 3989 | 1.07 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 10 | Hash-V-fixed | R | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-grow-map | A | 1011 | 2048 | 49.4% | 64 | 16 | 7181 / 1011 | 1.22 | 11 | 0 (0) | 7 | 1016 (1169) | 2126 | 16384 | 16384 |
| 10 | Hash-V-grow-map | B | 1015 | 2048 | 49.6% | 64 | 16 | 7177 / 1015 | 1.22 | 23 | 0 (0) | 7 | 1016 (1135) | 2190 | 16384 | 16384 |
| 10 | Hash-V-grow-map | C | 1013 | 2048 | 49.5% | 64 | 16 | 7179 / 1013 | 1.20 | 15 | 0 (0) | 7 | 1016 (1131) | 2207 | 16384 | 16384 |
| 10 | Hash-V-grow-map | D | 1008 | 2048 | 49.2% | 64 | 16 | 7184 / 1008 | 1.26 | 12 | 0 (0) | 7 | 1016 (1154) | 2152 | 16384 | 16384 |
| 10 | Hash-V-grow-map | S | 4012 | 8192 | 49.0% | 32 | 16 | 4180 / 4012 | 1.37 | 18 | 1 (16) | 9 | 4088 (4671) | 7871 | 16384 | 32768 |
| 10 | Hash-V-grow-map | U | 3989 | 8192 | 48.7% | 32 | 16 | 4203 / 3989 | 1.42 | 29 | 1 (16) | 9 | 4088 (4762) | 7885 | 16384 | 32768 |
| 10 | Hash-V-grow-map | R | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.44 | 19 | 1 (16) | 10 | 8184 (9613) | 15575 | 16384 | 131072 |
| 10 | Hash-V-pack | A | 1011 | 16384 | 6.2% | 64 | 16 | 7181 / 1011 | 1.00 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8088 |
| 10 | Hash-V-pack | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8120 |
| 10 | Hash-V-pack | C | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8104 |
| 10 | Hash-V-pack | D | 1008 | 16384 | 6.2% | 64 | 16 | 7184 / 1008 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8064 |
| 10 | Hash-V-pack | S | 4012 | 16384 | 24.5% | 32 | 16 | 4180 / 4012 | 1.06 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 16048 |
| 10 | Hash-V-pack | U | 3989 | 16384 | 24.3% | 32 | 16 | 4203 / 3989 | 1.07 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 15956 |
| 10 | Hash-V-pack | R | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33488 |
| 11 | Hash-V-fixed | A | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | C | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | D | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | S | 3885 | 16384 | 23.7% | 32 | 16 | 4307 / 3885 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 11 | Hash-V-fixed | U | 3889 | 16384 | 23.7% | 32 | 16 | 4303 / 3889 | 1.06 | 9 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 11 | Hash-V-fixed | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-grow-map | A | 282 | 1024 | 27.5% | 64 | 16 | 7910 / 282 | 1.13 | 17 | 0 (0) | 6 | 504 (589) | 1556 | 16384 | 8192 |
| 11 | Hash-V-grow-map | B | 281 | 1024 | 27.4% | 64 | 16 | 7911 / 281 | 1.12 | 8 | 0 (0) | 6 | 504 (565) | 1765 | 16384 | 8192 |
| 11 | Hash-V-grow-map | C | 282 | 1024 | 27.5% | 64 | 16 | 7910 / 282 | 1.20 | 10 | 0 (0) | 6 | 504 (582) | 1628 | 16384 | 8192 |
| 11 | Hash-V-grow-map | D | 283 | 1024 | 27.6% | 64 | 16 | 7909 / 283 | 1.18 | 13 | 0 (0) | 6 | 504 (579) | 1572 | 16384 | 8192 |
| 11 | Hash-V-grow-map | S | 3885 | 8192 | 47.4% | 32 | 16 | 4307 / 3885 | 1.35 | 13 | 2 (32) | 9 | 4088 (4693) | 7951 | 16384 | 32768 |
| 11 | Hash-V-grow-map | U | 3889 | 8192 | 47.5% | 32 | 16 | 4303 / 3889 | 1.35 | 16 | 2 (32) | 9 | 4088 (4719) | 7988 | 16384 | 32768 |
| 11 | Hash-V-grow-map | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.42 | 19 | 1 (16) | 10 | 8184 (9596) | 15560 | 16384 | 131072 |
| 11 | Hash-V-pack | A | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2256 |
| 11 | Hash-V-pack | B | 281 | 16384 | 1.7% | 64 | 16 | 7911 / 281 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2248 |
| 11 | Hash-V-pack | C | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2256 |
| 11 | Hash-V-pack | D | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2264 |
| 11 | Hash-V-pack | S | 3885 | 16384 | 23.7% | 32 | 16 | 4307 / 3885 | 1.07 | 6 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 15540 |
| 11 | Hash-V-pack | U | 3889 | 16384 | 23.7% | 32 | 16 | 4303 / 3889 | 1.06 | 9 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 15556 |
| 11 | Hash-V-pack | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33512 |
| 12 | Hash-V-fixed | A | 88 | 16384 | 0.5% | 64 | 16 | 8104 / 88 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | B | 87 | 16384 | 0.5% | 64 | 16 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | C | 87 | 16384 | 0.5% | 64 | 16 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | D | 89 | 16384 | 0.5% | 64 | 16 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | S | 2590 | 16384 | 15.8% | 32 | 16 | 5602 / 2590 | 1.04 | 5 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 12 | Hash-V-fixed | U | 2585 | 16384 | 15.8% | 32 | 16 | 5607 / 2585 | 1.04 | 5 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 65536 |
| 12 | Hash-V-fixed | R | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.10 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-grow-map | A | 88 | 256 | 34.4% | 64 | 16 | 8104 / 88 | 1.19 | 11 | 0 (0) | 4 | 120 (133) | 206 | 16384 | 2048 |
| 12 | Hash-V-grow-map | B | 87 | 256 | 34.0% | 64 | 16 | 8105 / 87 | 1.14 | 8 | 0 (0) | 4 | 120 (136) | 248 | 16384 | 2048 |
| 12 | Hash-V-grow-map | C | 87 | 256 | 34.0% | 64 | 16 | 8105 / 87 | 1.19 | 7 | 0 (0) | 4 | 120 (147) | 208 | 16384 | 2048 |
| 12 | Hash-V-grow-map | D | 89 | 256 | 34.8% | 64 | 16 | 8103 / 89 | 1.28 | 6 | 0 (0) | 4 | 120 (141) | 183 | 16384 | 2048 |
| 12 | Hash-V-grow-map | S | 2590 | 8192 | 31.6% | 32 | 16 | 5602 / 2590 | 1.27 | 16 | 2 (32) | 9 | 4088 (4706) | 9732 | 16384 | 32768 |
| 12 | Hash-V-grow-map | U | 2585 | 8192 | 31.6% | 32 | 16 | 5607 / 2585 | 1.30 | 15 | 1 (16) | 9 | 4088 (4765) | 9803 | 16384 | 32768 |
| 12 | Hash-V-grow-map | R | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.44 | 17 | 1 (16) | 10 | 8184 (9683) | 15583 | 16384 | 131072 |
| 12 | Hash-V-pack | A | 88 | 16384 | 0.5% | 64 | 8 | 8104 / 88 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 704 |
| 12 | Hash-V-pack | B | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 696 |
| 12 | Hash-V-pack | C | 87 | 16384 | 0.5% | 64 | 8 | 8105 / 87 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 696 |
| 12 | Hash-V-pack | D | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 712 |
| 12 | Hash-V-pack | S | 2590 | 16384 | 15.8% | 32 | 16 | 5602 / 2590 | 1.04 | 5 | 2 (32768) | 0 | 0 (0) | 0 | 16384 | 10360 |
| 12 | Hash-V-pack | U | 2585 | 16384 | 15.8% | 32 | 16 | 5607 / 2585 | 1.04 | 5 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 10340 |
| 12 | Hash-V-pack | R | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.10 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33480 |
| 13 | Hash-V-fixed | A | 10064 | 65536 | 15.4% | 64 | 32 | 22704 / 10064 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | B | 10165 | 65536 | 15.5% | 64 | 32 | 22603 / 10165 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | C | 10233 | 65536 | 15.6% | 64 | 32 | 22535 / 10233 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | D | 10186 | 65536 | 15.5% | 64 | 32 | 22582 / 10186 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | S | 14659 | 65536 | 22.4% | 32 | 32 | 18109 / 14659 | 1.05 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 13 | Hash-V-fixed | U | 14713 | 65536 | 22.5% | 32 | 32 | 18055 / 14713 | 1.04 | 6 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 13 | Hash-V-fixed | R | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.08 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-grow-map | A | 10064 | 32768 | 30.7% | 64 | 32 | 22704 / 10064 | 1.13 | 17 | 0 (0) | 11 | 16376 (17298) | 41856 | 131072 | 262144 |
| 13 | Hash-V-grow-map | B | 10165 | 32768 | 31.0% | 64 | 32 | 22603 / 10165 | 1.14 | 16 | 0 (0) | 11 | 16376 (17400) | 41833 | 131072 | 262144 |
| 13 | Hash-V-grow-map | C | 10233 | 32768 | 31.2% | 64 | 32 | 22535 / 10233 | 1.15 | 16 | 0 (0) | 11 | 16376 (17379) | 41348 | 131072 | 262144 |
| 13 | Hash-V-grow-map | D | 10186 | 32768 | 31.1% | 64 | 32 | 22582 / 10186 | 1.14 | 16 | 0 (0) | 11 | 16376 (17456) | 41623 | 131072 | 262144 |
| 13 | Hash-V-grow-map | S | 14659 | 32768 | 44.7% | 32 | 32 | 18109 / 14659 | 1.29 | 20 | 1 (16) | 11 | 16376 (18569) | 33591 | 131072 | 131072 |
| 13 | Hash-V-grow-map | U | 14713 | 32768 | 44.9% | 32 | 32 | 18055 / 14713 | 1.29 | 20 | 1 (16) | 11 | 16376 (18546) | 33576 | 131072 | 131072 |
| 13 | Hash-V-grow-map | R | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.41 | 24 | 1 (16) | 12 | 32760 (38294) | 64306 | 131072 | 524288 |
| 13 | Hash-V-pack | A | 10064 | 65536 | 15.4% | 64 | 16 | 22704 / 10064 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80512 |
| 13 | Hash-V-pack | B | 10165 | 65536 | 15.5% | 64 | 16 | 22603 / 10165 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81320 |
| 13 | Hash-V-pack | C | 10233 | 65536 | 15.6% | 64 | 16 | 22535 / 10233 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81864 |
| 13 | Hash-V-pack | D | 10186 | 65536 | 15.5% | 64 | 16 | 22582 / 10186 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81488 |
| 13 | Hash-V-pack | S | 14659 | 65536 | 22.4% | 32 | 16 | 18109 / 14659 | 1.05 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58636 |
| 13 | Hash-V-pack | U | 14713 | 65536 | 22.5% | 32 | 16 | 18055 / 14713 | 1.04 | 6 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58852 |
| 13 | Hash-V-pack | R | 16496 | 65536 | 25.2% | 64 | 16 | 16272 / 16496 | 1.08 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131968 |
| 14 | Hash-V-fixed | A | 3569 | 65536 | 5.4% | 64 | 32 | 29199 / 3569 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | B | 3529 | 65536 | 5.4% | 64 | 32 | 29239 / 3529 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | C | 3536 | 65536 | 5.4% | 64 | 32 | 29232 / 3536 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | D | 3536 | 65536 | 5.4% | 64 | 32 | 29232 / 3536 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | S | 14582 | 65536 | 22.3% | 32 | 32 | 18186 / 14582 | 1.05 | 6 | 2 (131072) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 14 | Hash-V-fixed | U | 14610 | 65536 | 22.3% | 32 | 32 | 18158 / 14610 | 1.04 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 14 | Hash-V-fixed | R | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.09 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-grow-map | A | 3569 | 8192 | 43.6% | 64 | 32 | 29199 / 3569 | 1.16 | 38 | 0 (0) | 9 | 4088 (4693) | 10469 | 131072 | 65536 |
| 14 | Hash-V-grow-map | B | 3529 | 8192 | 43.1% | 64 | 32 | 29239 / 3529 | 1.14 | 27 | 0 (0) | 9 | 4088 (4587) | 10289 | 131072 | 65536 |
| 14 | Hash-V-grow-map | C | 3536 | 8192 | 43.2% | 64 | 32 | 29232 / 3536 | 1.14 | 11 | 0 (0) | 9 | 4088 (4610) | 10448 | 131072 | 65536 |
| 14 | Hash-V-grow-map | D | 3536 | 8192 | 43.2% | 64 | 32 | 29232 / 3536 | 1.14 | 16 | 0 (0) | 9 | 4088 (4589) | 10367 | 131072 | 65536 |
| 14 | Hash-V-grow-map | S | 14582 | 32768 | 44.5% | 32 | 32 | 18186 / 14582 | 1.31 | 27 | 2 (32) | 11 | 16376 (18573) | 33884 | 131072 | 131072 |
| 14 | Hash-V-grow-map | U | 14610 | 32768 | 44.6% | 32 | 32 | 18158 / 14610 | 1.30 | 18 | 1 (16) | 11 | 16376 (18654) | 33616 | 131072 | 131072 |
| 14 | Hash-V-grow-map | R | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.42 | 22 | 1 (16) | 12 | 32760 (38342) | 64344 | 131072 | 524288 |
| 14 | Hash-V-pack | A | 3569 | 65536 | 5.4% | 64 | 16 | 29199 / 3569 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28552 |
| 14 | Hash-V-pack | B | 3529 | 65536 | 5.4% | 64 | 16 | 29239 / 3529 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28232 |
| 14 | Hash-V-pack | C | 3536 | 65536 | 5.4% | 64 | 16 | 29232 / 3536 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28288 |
| 14 | Hash-V-pack | D | 3536 | 65536 | 5.4% | 64 | 16 | 29232 / 3536 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28288 |
| 14 | Hash-V-pack | S | 14582 | 65536 | 22.3% | 32 | 16 | 18186 / 14582 | 1.05 | 6 | 2 (131072) | 0 | 0 (0) | 0 | 65536 | 58328 |
| 14 | Hash-V-pack | U | 14610 | 65536 | 22.3% | 32 | 16 | 18158 / 14610 | 1.04 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58440 |
| 14 | Hash-V-pack | R | 16495 | 65536 | 25.2% | 64 | 16 | 16273 / 16495 | 1.09 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131960 |
| 15 | Hash-V-fixed | A | 1015 | 65536 | 1.5% | 64 | 32 | 31753 / 1015 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | B | 1013 | 65536 | 1.5% | 64 | 32 | 31755 / 1013 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | C | 1019 | 65536 | 1.6% | 64 | 32 | 31749 / 1019 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | D | 1024 | 65536 | 1.6% | 64 | 32 | 31744 / 1024 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | S | 14574 | 65536 | 22.2% | 32 | 32 | 18194 / 14574 | 1.04 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 15 | Hash-V-fixed | U | 14471 | 65536 | 22.1% | 32 | 32 | 18297 / 14471 | 1.05 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 15 | Hash-V-fixed | R | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.08 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-grow-map | A | 1015 | 2048 | 49.6% | 64 | 32 | 31753 / 1015 | 1.21 | 11 | 0 (0) | 7 | 1016 (1123) | 2267 | 131072 | 16384 |
| 15 | Hash-V-grow-map | B | 1013 | 2048 | 49.5% | 64 | 32 | 31755 / 1013 | 1.25 | 14 | 0 (0) | 7 | 1016 (1193) | 2198 | 131072 | 16384 |
| 15 | Hash-V-grow-map | C | 1019 | 2048 | 49.8% | 64 | 32 | 31749 / 1019 | 1.21 | 11 | 0 (0) | 7 | 1016 (1135) | 2178 | 131072 | 16384 |
| 15 | Hash-V-grow-map | D | 1024 | 2048 | 50.0% | 64 | 32 | 31744 / 1024 | 1.24 | 15 | 0 (0) | 7 | 1016 (1183) | 2215 | 131072 | 16384 |
| 15 | Hash-V-grow-map | S | 14574 | 32768 | 44.5% | 32 | 32 | 18194 / 14574 | 1.30 | 20 | 1 (16) | 11 | 16376 (18701) | 33862 | 131072 | 131072 |
| 15 | Hash-V-grow-map | U | 14471 | 32768 | 44.2% | 32 | 32 | 18297 / 14471 | 1.31 | 18 | 1 (16) | 11 | 16376 (18508) | 33794 | 131072 | 131072 |
| 15 | Hash-V-grow-map | R | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.40 | 23 | 1 (16) | 12 | 32760 (38022) | 64328 | 131072 | 524288 |
| 15 | Hash-V-pack | A | 1015 | 65536 | 1.5% | 64 | 16 | 31753 / 1015 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8120 |
| 15 | Hash-V-pack | B | 1013 | 65536 | 1.5% | 64 | 16 | 31755 / 1013 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8104 |
| 15 | Hash-V-pack | C | 1019 | 65536 | 1.6% | 64 | 16 | 31749 / 1019 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8152 |
| 15 | Hash-V-pack | D | 1024 | 65536 | 1.6% | 64 | 16 | 31744 / 1024 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8192 |
| 15 | Hash-V-pack | S | 14574 | 65536 | 22.2% | 32 | 16 | 18194 / 14574 | 1.04 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 58296 |
| 15 | Hash-V-pack | U | 14471 | 65536 | 22.1% | 32 | 16 | 18297 / 14471 | 1.05 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 57884 |
| 15 | Hash-V-pack | R | 16494 | 65536 | 25.2% | 64 | 16 | 16274 / 16494 | 1.08 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131952 |
| 16 | Hash-V-fixed | A | 284 | 65536 | 0.4% | 64 | 32 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | B | 285 | 65536 | 0.4% | 64 | 32 | 32483 / 285 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | C | 284 | 65536 | 0.4% | 64 | 32 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | D | 283 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | S | 13246 | 65536 | 20.2% | 32 | 32 | 19522 / 13246 | 1.03 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 16 | Hash-V-fixed | U | 12949 | 65536 | 19.8% | 32 | 32 | 19819 / 12949 | 1.04 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 262144 |
| 16 | Hash-V-fixed | R | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.09 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-grow-map | A | 284 | 1024 | 27.7% | 64 | 32 | 32484 / 284 | 1.13 | 8 | 0 (0) | 6 | 504 (590) | 1492 | 131072 | 8192 |
| 16 | Hash-V-grow-map | B | 285 | 1024 | 27.8% | 64 | 32 | 32483 / 285 | 1.12 | 12 | 0 (0) | 6 | 504 (597) | 1453 | 131072 | 8192 |
| 16 | Hash-V-grow-map | C | 284 | 1024 | 27.7% | 64 | 32 | 32484 / 284 | 1.11 | 10 | 0 (0) | 6 | 504 (565) | 1401 | 131072 | 8192 |
| 16 | Hash-V-grow-map | D | 283 | 1024 | 27.6% | 64 | 32 | 32485 / 283 | 1.09 | 14 | 0 (0) | 6 | 504 (581) | 1523 | 131072 | 8192 |
| 16 | Hash-V-grow-map | S | 13246 | 32768 | 40.4% | 32 | 32 | 19522 / 13246 | 1.27 | 19 | 1 (16) | 11 | 16376 (18533) | 34952 | 131072 | 131072 |
| 16 | Hash-V-grow-map | U | 12949 | 32768 | 39.5% | 32 | 32 | 19819 / 12949 | 1.27 | 21 | 1 (16) | 11 | 16376 (18512) | 35529 | 131072 | 131072 |
| 16 | Hash-V-grow-map | R | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.43 | 27 | 1 (16) | 12 | 32760 (38270) | 64321 | 131072 | 524288 |
| 16 | Hash-V-pack | A | 284 | 65536 | 0.4% | 64 | 16 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2272 |
| 16 | Hash-V-pack | B | 285 | 65536 | 0.4% | 64 | 16 | 32483 / 285 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2280 |
| 16 | Hash-V-pack | C | 284 | 65536 | 0.4% | 64 | 16 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2272 |
| 16 | Hash-V-pack | D | 283 | 65536 | 0.4% | 64 | 16 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2264 |
| 16 | Hash-V-pack | S | 13246 | 65536 | 20.2% | 32 | 16 | 19522 / 13246 | 1.03 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 52984 |
| 16 | Hash-V-pack | U | 12949 | 65536 | 19.8% | 32 | 16 | 19819 / 12949 | 1.04 | 5 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 51796 |
| 16 | Hash-V-pack | R | 16496 | 65536 | 25.2% | 64 | 16 | 16272 / 16496 | 1.09 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131968 |

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
| 13 | A | 10064 | 32 | 16 | 80512 | 32768 | 262144 |
| 13 | B | 10165 | 32 | 16 | 81320 | 32768 | 262144 |
| 13 | C | 10233 | 32 | 16 | 81864 | 32768 | 262144 |
| 13 | D | 10186 | 32 | 16 | 81488 | 32768 | 262144 |
| 13 | S | 14659 | 32 | 16 | 58636 | 32768 | 262144 |
| 13 | U | 14713 | 32 | 16 | 58852 | 32768 | 262144 |
| 13 | R | 16496 | 32 | 16 | 131968 | 32768 | 262144 |
| 14 | A | 3569 | 32 | 16 | 28552 | 32768 | 262144 |
| 14 | B | 3529 | 32 | 16 | 28232 | 32768 | 262144 |
| 14 | C | 3536 | 32 | 16 | 28288 | 32768 | 262144 |
| 14 | D | 3536 | 32 | 16 | 28288 | 32768 | 262144 |
| 14 | S | 14582 | 32 | 16 | 58328 | 32768 | 262144 |
| 14 | U | 14610 | 32 | 16 | 58440 | 32768 | 262144 |
| 14 | R | 16495 | 32 | 16 | 131960 | 32768 | 262144 |
| 15 | A | 1015 | 32 | 16 | 8120 | 32768 | 262144 |
| 15 | B | 1013 | 32 | 16 | 8104 | 32768 | 262144 |
| 15 | C | 1019 | 32 | 16 | 8152 | 32768 | 262144 |
| 15 | D | 1024 | 32 | 16 | 8192 | 32768 | 262144 |
| 15 | S | 14574 | 32 | 16 | 58296 | 32768 | 262144 |
| 15 | U | 14471 | 32 | 16 | 57884 | 32768 | 262144 |
| 15 | R | 16494 | 32 | 16 | 131952 | 32768 | 262144 |
| 16 | A | 284 | 32 | 16 | 2272 | 32768 | 262144 |
| 16 | B | 285 | 32 | 16 | 2280 | 32768 | 262144 |
| 16 | C | 284 | 32 | 16 | 2272 | 32768 | 262144 |
| 16 | D | 283 | 32 | 16 | 2264 | 32768 | 262144 |
| 16 | S | 13246 | 32 | 16 | 52984 | 32768 | 262144 |
| 16 | U | 12949 | 32 | 16 | 51796 | 32768 | 262144 |
| 16 | R | 16496 | 32 | 16 | 131968 | 32768 | 262144 |

## Also reported (rule-m2 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-fixed's stores, from the model; the gates checked it).

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
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16384 | 16384 | 16338 | 16329 | 16384 | 10064 | 10165 | 10233 | 10186 | 14659 | 14713 | 16496 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 4096 | 4096 | 16333 | 16318 | 16384 | 3569 | 3529 | 3536 | 3536 | 14582 | 14610 | 16495 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 1024 | 1024 | 16220 | 16194 | 16384 | 1015 | 1013 | 1019 | 1024 | 14574 | 14471 | 16494 |
| 16 | 128 | 64 | 256 | 256 | 256 | 256 | 256 | 14526 | 14457 | 16384 | 284 | 285 | 284 | 283 | 13246 | 12949 | 16496 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.2 §15.7)

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
- job 13, S = A + B: add (16384): int_both 0.012, int_x 0.096, int_y 0.103, den 0.034, num 0.000, general 0.755; cross tests 65536 (matched 760, cross reads 65536); S's store W 14659, found / new 18109 / 14659
- job 13, U = C + D: add (16384): zero_x 0.000, int_both 0.011, int_x 0.103, int_y 0.103, cross 0.000, den 0.035, general 0.747; cross tests 65536 (matched 764, cross reads 65536); U's store W 14713, found / new 18055 / 14713
- job 13, R = S x U: add (2097152): zero_x 0.008, int_x 0.000, int_y 0.005, den 0.005, general 0.983; mul (2097152): int_both 0.000, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.022, int_y 0.021, general 0.921; cross tests 8388608 (matched 74412, cross reads 8388608); R's store W 16496, found / new 16272 / 16496
- job 14, S = A + B: add (16384): int_both 0.012, int_x 0.099, int_y 0.090, cross 0.000, den 0.037, general 0.761; cross tests 65536 (matched 811, cross reads 65536); S's store W 14582, found / new 18186 / 14582
- job 14, U = C + D: add (16384): int_both 0.012, int_x 0.100, int_y 0.107, cross 0.000, den 0.035, num 0.000, general 0.746; cross tests 65536 (matched 773, cross reads 65536); U's store W 14610, found / new 18158 / 14610
- job 14, R = S x U: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.005, den 0.005, general 0.982; mul (2097152): one_x 0.000, one_y 0.000, int_both 0.001, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.023, int_y 0.022, general 0.919; cross tests 8388608 (matched 75160, cross reads 8388352); R's store W 16495, found / new 16273 / 16495
- job 15, S = A + B: add (16384): int_both 0.012, int_x 0.107, int_y 0.094, cross 0.000, den 0.036, general 0.750; cross tests 65536 (matched 794, cross reads 65536); S's store W 14574, found / new 18194 / 14574
- job 15, U = C + D: add (16384): int_both 0.015, int_x 0.111, int_y 0.101, den 0.033, general 0.740; cross tests 65536 (matched 787, cross reads 65536); U's store W 14471, found / new 18297 / 14471
- job 15, R = S x U: add (2097152): zero_x 0.008, int_x 0.000, int_y 0.005, den 0.005, general 0.982; mul (2097152): int_both 0.001, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.022, int_y 0.025, general 0.918; cross tests 8388608 (matched 74481, cross reads 8388608); R's store W 16494, found / new 16274 / 16494
- job 16, S = A + B: add (16384): int_both 0.014, int_x 0.092, int_y 0.128, den 0.034, num 0.000, general 0.732; cross tests 65536 (matched 792, cross reads 65536); S's store W 13246, found / new 19522 / 13246
- job 16, U = C + D: add (16384): zero_y 0.004, int_both 0.013, int_x 0.099, int_y 0.112, den 0.036, num 0.000, general 0.735; cross tests 65536 (matched 818, cross reads 65536); U's store W 12949, found / new 19819 / 12949
- job 16, R = S x U: add (2097152): zero_x 0.008, int_x 0.000, int_y 0.006, den 0.005, general 0.982; mul (2097152): int_both 0.001, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.025, int_y 0.026, general 0.914; cross tests 8388608 (matched 74721, cross reads 8388608); R's store W 16496, found / new 16272 / 16496

The gates checked every counter and store figure against the model (spec v0.2 §15.7 item 2), the TC gate included: MHVF:1 pass, TC:1 pass, MHVGM:1 pass, MHVP:1 pass, MHVF:2 pass, TC:2 pass, MHVP:2 pass, MHVGM:2 pass, TC:3 pass, MHVF:3 pass, MHVP:3 pass, MHVGM:3 pass, MHVF:4 pass, MHVGM:4 pass, TC:4 pass, MHVP:4 pass, MHVF:5 pass, MHVGM:5 pass, TC:5 pass, MHVP:5 pass, MHVF:6 pass, TC:6 pass, MHVGM:6 pass, MHVP:6 pass, MHVF:7 pass, TC:7 pass, MHVGM:7 pass, MHVP:7 pass, MHVF:8 pass, TC:8 pass, MHVGM:8 pass, MHVP:8 pass, MHVF:9 pass, MHVGM:9 pass, TC:9 pass, MHVP:9 pass, MHVF:10 pass, TC:10 pass, MHVGM:10 pass, MHVP:10 pass, MHVF:11 pass, MHVGM:11 pass, TC:11 pass, MHVP:11 pass, MHVF:12 pass, TC:12 pass, MHVGM:12 pass, MHVP:12 pass, MHVF:13 pass, MHVGM:13 pass, TC:13 pass, MHVP:13 pass, MHVF:14 pass, TC:14 pass, MHVGM:14 pass, MHVP:14 pass, MHVF:15 pass, TC:15 pass, MHVGM:15 pass, MHVP:15 pass, MHVF:16 pass, TC:16 pass, MHVGM:16 pass, MHVP:16 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m2 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 3.16 | no | 1 | 14 | 0.071 | 14 | 0 | none | 9 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 2.655 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 10.646 | no | 1 | 14 | 0.071 | 15 | 1 | none | 16 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (7->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 15.741 | no | 9 | 64 | 0.141 | 66 | 2 | none | 21 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (7->8 threads), bash 0/8 (1->1 threads) |
| 2 | measured | 1 | 15.301 | no | 9 | 64 | 0.141 | 64 | 0 | none | 21 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (8->7 threads), bash 0/8 (1->1 threads) |
| 3 | measured | 1 | 15.194 | no | 9 | 64 | 0.141 | 66 | 2 | none | 21 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (7->8 threads), bash 0/8 (1->1 threads) |
| 4 | measured | 1 | 14.149 | no | 10 | 64 | 0.156 | 64 | 0 | none | 20 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 1/14 (7->6 threads), bash 0/8 (1->1 threads) |
| 5 | measured | 1 | 15.277 | no | 10 | 64 | 0.156 | 67 | 3 | none | 21 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/15 (6->8 threads), bash 0/8 (1->1 threads) |
| 6 | measured | 1 | 15.056 | no | 10 | 64 | 0.156 | 65 | 1 | none | 21 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (8->9 threads), bash 0/8 (1->1 threads) |
| 7 | measured | 1 | 15.068 | no | 10 | 64 | 0.156 | 65 | 1 | none | 21 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/15 (8->7 threads), bash 0/8 (1->1 threads) |
| 8 | measured | 1 | 14.821 | no | 10 | 64 | 0.156 | 66 | 2 | none | 20 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (7->8 threads), bash 0/8 (1->1 threads) |
| 9 | measured | 1 | 10.312 | no | 10 | 64 | 0.156 | 64 | 0 | none | 16 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/7 (8->8 threads), claude 0/12 (8->7 threads), bash 0/7 (1->1 threads) |
| 10 | measured | 1 | 14.819 | no | 10 | 64 | 0.156 | 66 | 2 | none | 20 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 4/15 (7->8 threads), bash 0/8 (1->1 threads) |
| 11 | measured | 1 | 14.705 | no | 11 | 64 | 0.172 | 64 | 0 | none | 20 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/15 (8->7 threads), bash 0/8 (1->1 threads) |
| 12 | measured | 1 | 14.628 | no | 10 | 64 | 0.156 | 66 | 2 | none | 20 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 2/15 (7->8 threads), bash 0/8 (1->1 threads) |
| 13 | measured | 1 | 56.86 | no | 11 | 64 | 0.172 | 68 | 4 | none | 62 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 5/41 (8->8 threads), bash 0/16 (1->1 threads) |
| 14 | measured | 1 | 56.396 | no | 11 | 64 | 0.172 | 69 | 5 | none | 62 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 6/41 (8->7 threads), bash 0/16 (1->1 threads) |
| 15 | measured | 1 | 56.887 | no | 11 | 64 | 0.172 | 67 | 3 | none | 62 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 6/42 (7->7 threads), bash 0/16 (1->1 threads) |
| 16 | measured | 1 | 56.522 | no | 11 | 64 | 0.172 | 66 | 2 | none | 62 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 4/41 (8->7 threads), bash 0/16 (1->1 threads) |

The runner's own ticks over the 19 blocks: 166 ticks over 1066 launches, 0.156 per launch; the highest block 0.172 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9999 | 0.9975 | 537174 | 536807 |
| 1 | Hash-V-fixed | 12 | 0.9988 | 0.9937 | 629350 | 630107 |
| 1 | Hash-V-grow-map | 12 | 0.9982 | 0.9813 | 696206 | 696626 |
| 1 | Hash-V-pack | 12 | 0.9996 | 0.9935 | 655696 | 656220 |
| 1 | Hash-V-fixed-null | 12 | 0.9993 | 0.9953 | 630424 | 629983 |
| 2 | TC | 12 | 0.9999 | 0.9982 | 534318 | 533938 |
| 2 | Hash-V-fixed | 12 | 0.9999 | 0.9949 | 622890 | 623935 |
| 2 | Hash-V-grow-map | 12 | 0.9996 | 0.9612 | 658370 | 659161 |
| 2 | Hash-V-pack | 12 | 0.9996 | 0.9897 | 642384 | 642347 |
| 2 | Hash-V-fixed-null | 12 | 0.9998 | 0.9969 | 622793 | 622693 |
| 3 | TC | 12 | 0.9999 | 0.9940 | 532505 | 532170 |
| 3 | Hash-V-fixed | 12 | 0.9998 | 0.9943 | 622644 | 622544 |
| 3 | Hash-V-grow-map | 12 | 0.9998 | 0.9983 | 643346 | 643099 |
| 3 | Hash-V-pack | 12 | 0.9997 | 0.9898 | 635253 | 634719 |
| 3 | Hash-V-fixed-null | 12 | 0.9997 | 0.9798 | 621396 | 621097 |
| 4 | TC | 12 | 0.9998 | 0.9823 | 489374 | 489143 |
| 4 | Hash-V-fixed | 12 | 1.0000 | 0.9979 | 569256 | 568903 |
| 4 | Hash-V-grow-map | 12 | 0.9995 | 0.9800 | 585802 | 586943 |
| 4 | Hash-V-pack | 12 | 0.9997 | 0.9900 | 583764 | 583635 |
| 4 | Hash-V-fixed-null | 12 | 0.9995 | 0.9055 | 569753 | 569606 |
| 5 | TC | 12 | 0.9996 | 0.9770 | 4366969 | 4367575 |
| 5 | Hash-V-fixed | 12 | 1.0000 | 0.9969 | 4918924 | 4927830 |
| 5 | Hash-V-grow-map | 12 | 0.9997 | 0.9951 | 5089571 | 5095109 |
| 5 | Hash-V-pack | 12 | 0.9994 | 0.9469 | 5035413 | 5036169 |
| 5 | Hash-V-fixed-null | 12 | 0.9992 | 0.9936 | 4913804 | 4918062 |
| 6 | TC | 12 | 0.9996 | 0.9824 | 4331205 | 4331463 |
| 6 | Hash-V-fixed | 12 | 0.9997 | 0.9852 | 4872424 | 4873450 |
| 6 | Hash-V-grow-map | 12 | 0.9999 | 0.9963 | 5014942 | 5018960 |
| 6 | Hash-V-pack | 12 | 0.9996 | 0.9828 | 4933610 | 4938610 |
| 6 | Hash-V-fixed-null | 12 | 0.9996 | 0.8542 | 4878365 | 4882187 |
| 7 | TC | 12 | 0.9994 | 0.9829 | 4339157 | 4340019 |
| 7 | Hash-V-fixed | 12 | 0.9998 | 0.9662 | 4872899 | 4873697 |
| 7 | Hash-V-grow-map | 12 | 0.9977 | 0.9101 | 4970541 | 4973921 |
| 7 | Hash-V-pack | 12 | 0.9993 | 0.9919 | 4942551 | 4950164 |
| 7 | Hash-V-fixed-null | 12 | 0.9997 | 0.9940 | 4877703 | 4880716 |
| 8 | TC | 12 | 0.9991 | 0.9906 | 4270838 | 4279189 |
| 8 | Hash-V-fixed | 12 | 0.9996 | 0.9645 | 4796628 | 4799666 |
| 8 | Hash-V-grow-map | 12 | 0.9983 | 0.9933 | 4861952 | 4867340 |
| 8 | Hash-V-pack | 12 | 0.9995 | 0.9943 | 4828136 | 4831497 |
| 8 | Hash-V-fixed-null | 12 | 0.9994 | 0.9931 | 4794218 | 4798958 |
| 9 | TC | 12 | 0.9995 | 0.9671 | 34766846 | 34785841 |
| 9 | Hash-V-fixed | 12 | 1.0000 | 0.9944 | 38579072 | 38614004 |
| 9 | Hash-V-grow-map | 12 | 0.9998 | 0.9925 | 39055227 | 39058476 |
| 9 | Hash-V-pack | 12 | 0.9996 | 0.9929 | 39017786 | 39029782 |
| 9 | Hash-V-fixed-null | 12 | 1.0000 | 0.9953 | 38670270 | 38671968 |
| 10 | TC | 12 | 0.9992 | 0.9914 | 34857336 | 34879384 |
| 10 | Hash-V-fixed | 12 | 0.9994 | 0.9914 | 38713158 | 38723889 |
| 10 | Hash-V-grow-map | 12 | 0.9993 | 0.9853 | 38935466 | 38948026 |
| 10 | Hash-V-pack | 12 | 0.9994 | 0.9523 | 38934935 | 38947508 |
| 10 | Hash-V-fixed-null | 12 | 0.9994 | 0.9921 | 38677473 | 38699169 |
| 11 | TC | 12 | 0.9995 | 0.9871 | 34745906 | 34778708 |
| 11 | Hash-V-fixed | 12 | 0.9998 | 0.9637 | 38522093 | 38532154 |
| 11 | Hash-V-grow-map | 12 | 0.9997 | 0.9793 | 38720082 | 38739642 |
| 11 | Hash-V-pack | 12 | 0.9999 | 0.9881 | 38708338 | 38719204 |
| 11 | Hash-V-fixed-null | 12 | 0.9989 | 0.9845 | 38644450 | 38723774 |
| 12 | TC | 12 | 0.9999 | 0.9957 | 34882561 | 34884746 |
| 12 | Hash-V-fixed | 12 | 1.0000 | 0.9927 | 38729984 | 38728274 |
| 12 | Hash-V-grow-map | 12 | 1.0000 | 0.9911 | 38811263 | 38810382 |
| 12 | Hash-V-pack | 12 | 0.9995 | 0.9962 | 38754296 | 38770839 |
| 12 | Hash-V-fixed-null | 12 | 1.0000 | 0.9983 | 38643827 | 38666090 |
| 13 | TC | 12 | 0.9990 | 0.9878 | 279424644 | 279793452 |
| 13 | Hash-V-fixed | 12 | 0.9985 | 0.9900 | 308701166 | 309225172 |
| 13 | Hash-V-grow-map | 12 | 0.9983 | 0.9901 | 308846658 | 310305796 |
| 13 | Hash-V-pack | 12 | 0.9993 | 0.9889 | 308819238 | 309359108 |
| 13 | Hash-V-fixed-null | 12 | 0.9996 | 0.9962 | 307772142 | 308005462 |
| 14 | TC | 12 | 0.9997 | 0.9958 | 278742260 | 278975568 |
| 14 | Hash-V-fixed | 12 | 0.9997 | 0.9991 | 307374446 | 307543057 |
| 14 | Hash-V-grow-map | 12 | 0.9997 | 0.9953 | 306924710 | 307379121 |
| 14 | Hash-V-pack | 12 | 0.9997 | 0.9897 | 307692082 | 307778552 |
| 14 | Hash-V-fixed-null | 12 | 0.9998 | 0.9895 | 307203366 | 307277460 |
| 15 | TC | 12 | 0.9992 | 0.9917 | 278547607 | 279141330 |
| 15 | Hash-V-fixed | 12 | 0.9981 | 0.9827 | 306747773 | 307639120 |
| 15 | Hash-V-grow-map | 12 | 0.9987 | 0.9932 | 307389002 | 308115058 |
| 15 | Hash-V-pack | 12 | 0.9982 | 0.9786 | 308959061 | 309603776 |
| 15 | Hash-V-fixed-null | 12 | 0.9994 | 0.9927 | 306567802 | 306593564 |
| 16 | TC | 12 | 0.9963 | 0.9854 | 278822850 | 280397404 |
| 16 | Hash-V-fixed | 12 | 0.9992 | 0.9908 | 305814824 | 306412578 |
| 16 | Hash-V-grow-map | 12 | 0.9995 | 0.9787 | 306255108 | 306459926 |
| 16 | Hash-V-pack | 12 | 0.9997 | 0.9967 | 307098661 | 307137151 |
| 16 | Hash-V-fixed-null | 12 | 0.9991 | 0.9888 | 306717451 | 306775210 |

Idle checks: 19 (0 busy). Host rates R_h: {'1': 0.09393199323689648, '79': 0.09393199323689648, '84': 0.09393199323689648, '92': 0.31649252661811345, '22484': 0.09393199323689648} (measured: {'1': 0.0, '79': 0.0, '84': 0.0, '92': 0.31649252661811345, '22484': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

