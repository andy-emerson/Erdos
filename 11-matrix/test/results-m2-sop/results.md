# Design 3 on matrices, round m2, the sum of products A B + C D: results

Run started 2026-10-30T09:24:23-05:00.

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

- The sop run (job set `sop`: P = A x B; Q = C x D; R = P + Q).
- Started 2026-10-30T09:24:23-05:00; finished 2026-10-30T09:38:33-05:00.
- Step 0 (before the clock): 177.3 s; step 0's files: new job set.
- Run time 11.2 minutes (hard stop 40).
- Rule sha256 `e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200`; round 3's results after the run: unchanged; m1's results and the other m2 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[20878], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T09:27:21-05:00: TC `179dacc22ad6`, MHVF `eb7f57fd8798`, MHVGM `915116a75090`, MHVP `56a37a633b7c`, TC3GATE `83ff353acc03`
- Unit tests: m2_unit: m2_unit: 642586 checks, 0 failures; m2_diff: m2_diff: 3000 seeds, 3000 jobs, 1050092 operations; 1137376 checks, 0 failures; m2_alloc-timing: m2_alloc (timing build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-counting: m2_alloc (counting build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-memory: m2_alloc (memory build): 160 jobs, 2450 moments, 32596 checks, 0 failures; m2_unit-asan: m2_unit: 642586 checks, 0 failures; m2_diff-asan: m2_diff: 600 seeds, 600 jobs, 209812 operations; 227944 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 4.6 s, job 6 4.6 s, job 13 20.8 s; P_6 = 4.6 s, P_13 = 20.8 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 14.7 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.053317545320012996, '79': 0.05331627212625293, '84': 0.3815624792974888, '92': 0.38679374824256924, '20878': 0.05331627212625293}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 4 sum-of-products time; 6 stable bytes of one stored input matrix; peak bytes during the operation (reported). Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-fixed against Hash-V-fixed-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses). Hash-V-pack against Hash-V-grow-map is reported with no verdict on any measure. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m2 item 8, carried over).

### Hash-V-fixed against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.676 [0.667, 0.685], h 0.050 | loss, 0.873 [0.870, 0.875], h 0.050 | reported, 0.865 [0.863, 0.867] | reported, 0.602 [0.593, 0.638] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.286 (heap 2.271) |
| 2 (n 16, rho 4) | loss, 0.664 [0.657, 0.671], h 0.050 | loss, 0.873 [0.867, 0.876], h 0.050 | reported, 0.865 [0.859, 0.868] | reported, 0.619 [0.595, 0.764] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.286 (heap 2.271) |
| 3 (n 16, rho 16) | loss, 0.716 [0.709, 0.730], h 0.050 | loss, 0.870 [0.863, 0.875], h 0.050 | reported, 0.865 [0.858, 0.869] | reported, 0.612 [0.598, 0.758] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.286 (heap 2.271) |
| 4 (n 16, rho 64) | loss, 0.702 [0.695, 0.712], h 0.050 | loss, 0.856 [0.853, 0.861], h 0.050 | reported, 0.851 [0.848, 0.856] | reported, 0.607 [0.596, 0.695] | loss, 2.250 (heap 2.236) **severe memory loss** | reported, 2.286 (heap 2.271) |
| 5 (n 32, rho 1) | loss, 0.718 [0.715, 0.722], h 0.050 | loss, 0.889 [0.885, 0.899], h 0.050 | reported, 0.886 [0.881, 0.895] | reported, 0.783 [0.672, 0.820] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.286 (heap 2.282) |
| 6 (n 32, rho 4) | loss, 0.703 [0.700, 0.707], h 0.050 | loss, 0.889 [0.885, 0.894], h 0.050 | reported, 0.885 [0.881, 0.890] | reported, 0.697 [0.623, 0.799] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.286 (heap 2.282) |
| 7 (n 32, rho 16) | loss, 0.755 [0.744, 0.763], h 0.050 | loss, 0.889 [0.885, 0.892], h 0.050 | reported, 0.886 [0.882, 0.890] | reported, 0.635 [0.612, 0.757] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.286 (heap 2.282) |
| 8 (n 32, rho 64) | loss, 0.747 [0.741, 0.756], h 0.050 | loss, 0.879 [0.872, 0.881], h 0.050 | reported, 0.877 [0.870, 0.878] | reported, 0.730 [0.593, 0.817] | loss, 2.250 (heap 2.246) **severe memory loss** | reported, 2.286 (heap 2.282) |
| 9 (n 64, rho 1) | loss, 0.722 [0.711, 0.728], h 0.050 | loss, 0.895 [0.890, 0.899], h 0.050 | reported, 0.893 [0.888, 0.897] | reported, 0.772 [0.690, 0.919] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.286 (heap 2.285) |
| 10 (n 64, rho 4) | loss, 0.720 [0.717, 0.724], h 0.050 | loss, 0.894 [0.883, 0.902], h 0.050 | reported, 0.892 [0.881, 0.900] | reported, 0.800 [0.755, 0.850] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.286 (heap 2.285) |
| 11 (n 64, rho 16) | loss, 0.748 [0.744, 0.757], h 0.050 | loss, 0.896 [0.890, 0.901], h 0.050 | reported, 0.894 [0.889, 0.899] | reported, 0.769 [0.736, 0.865] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.286 (heap 2.285) |
| 12 (n 64, rho 64) | loss, 0.749 [0.743, 0.757], h 0.050 | loss, 0.899 [0.893, 0.905], h 0.050 | reported, 0.897 [0.892, 0.904] | reported, 0.753 [0.699, 0.866] | loss, 2.250 (heap 2.249) **severe memory loss** | reported, 2.286 (heap 2.285) |
| 13 (n 128, rho 1) | loss, 0.673 [0.667, 0.675], h 0.050 | loss, 0.887 [0.883, 0.894], h 0.050 | reported, 0.886 [0.882, 0.893] | reported, 0.668 [0.628, 0.696] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.536 (heap 2.535) |
| 14 (n 128, rho 4) | loss, 0.714 [0.711, 0.717], h 0.050 | loss, 0.899 [0.895, 0.905], h 0.050 | reported, 0.898 [0.894, 0.904] | reported, 0.661 [0.625, 0.745] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.536 (heap 2.535) |
| 15 (n 128, rho 16) | loss, 0.750 [0.745, 0.754], h 0.050 | loss, 0.897 [0.891, 0.902], h 0.050 | reported, 0.896 [0.891, 0.901] | reported, 0.666 [0.621, 0.729] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.536 (heap 2.535) |
| 16 (n 128, rho 64) | loss, 0.754 [0.748, 0.762], h 0.050 | loss, 0.903 [0.894, 0.912], h 0.050 | reported, 0.902 [0.893, 0.912] | reported, 0.656 [0.587, 0.727] | loss, 2.500 (heap 2.500) **severe memory loss** | reported, 2.536 (heap 2.535) |

### Hash-V-grow-map against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.339 [0.334, 0.343], h 0.050 | loss, 0.846 [0.844, 0.850], h 0.050 | reported, 0.809 [0.807, 0.813] | reported, 0.564 [0.558, 0.597] | loss, 2.250 (heap 2.237) **severe memory loss** | reported, 2.429 (heap 2.412) |
| 2 (n 16, rho 4) | loss, 0.506 [0.501, 0.512], h 0.050 | loss, 0.844 [0.838, 0.848], h 0.050 | reported, 0.828 [0.820, 0.831] | reported, 0.581 [0.557, 0.721] | win, 0.750 (heap 0.765) | reported, 1.571 (heap 1.571) |
| 3 (n 16, rho 16) | loss, 0.578 [0.568, 0.588], h 0.050 | loss, 0.842 [0.834, 0.847], h 0.050 | reported, 0.832 [0.825, 0.836] | reported, 0.572 [0.559, 0.723] | win, 0.375 (heap 0.398) | reported, 1.357 (heap 1.361) |
| 4 (n 16, rho 64) | loss, 0.622 [0.614, 0.632], h 0.050 | loss, 0.830 [0.828, 0.835], h 0.050 | reported, 0.823 [0.821, 0.828] | reported, 0.570 [0.558, 0.649] | win, 0.281 (heap 0.305) | reported, 1.304 (heap 1.308) |
| 5 (n 32, rho 1) | loss, 0.451 [0.450, 0.454], h 0.050 | loss, 0.880 [0.873, 0.887], h 0.050 | reported, 0.867 [0.860, 0.874] | reported, 0.750 [0.647, 0.791] | loss, 1.250 (heap 1.251) | reported, 1.857 (heap 1.856) |
| 6 (n 32, rho 4) | loss, 0.533 [0.531, 0.536], h 0.050 | loss, 0.879 [0.875, 0.883], h 0.050 | reported, 0.870 [0.866, 0.874] | reported, 0.665 [0.601, 0.755] | win, 0.750 (heap 0.754) | reported, 1.571 (heap 1.571) |
| 7 (n 32, rho 16) | loss, 0.642 [0.632, 0.647], h 0.050 | loss, 0.878 [0.873, 0.882], h 0.050 | reported, 0.873 [0.868, 0.877] | reported, 0.608 [0.590, 0.722] | win, 0.375 (heap 0.380) | reported, 1.357 (heap 1.358) |
| 8 (n 32, rho 64) | loss, 0.645 [0.637, 0.654], h 0.050 | loss, 0.864 [0.855, 0.869], h 0.050 | reported, 0.860 [0.851, 0.865] | reported, 0.683 [0.556, 0.779] | win, 0.281 (heap 0.287) | reported, 1.304 (heap 1.305) |
| 9 (n 64, rho 1) | loss, 0.465 [0.460, 0.468], h 0.050 | loss, 0.894 [0.890, 0.899], h 0.050 | reported, 0.887 [0.883, 0.892] | reported, 0.782 [0.701, 0.928] | loss, 1.250 (heap 1.250) | reported, 1.857 (heap 1.857) |
| 10 (n 64, rho 4) | loss, 0.601 [0.596, 0.604], h 0.050 | loss, 0.893 [0.888, 0.899], h 0.050 | reported, 0.889 [0.885, 0.895] | reported, 0.809 [0.751, 0.863] | win, 0.500 (heap 0.501) | reported, 1.429 (heap 1.429) |
| 11 (n 64, rho 16) | loss, 0.671 [0.668, 0.673], h 0.050 | loss, 0.892 [0.888, 0.897], h 0.050 | reported, 0.890 [0.886, 0.895] | reported, 0.763 [0.740, 0.851] | win, 0.375 (heap 0.376) | reported, 1.357 (heap 1.357) |
| 12 (n 64, rho 64) | loss, 0.689 [0.683, 0.696], h 0.050 | loss, 0.892 [0.888, 0.899], h 0.050 | reported, 0.890 [0.885, 0.897] | reported, 0.763 [0.702, 0.880] | win, 0.281 (heap 0.283) | reported, 1.304 (heap 1.304) |
| 13 (n 128, rho 1) | loss, 0.463 [0.457, 0.469], h 0.050 | loss, 0.892 [0.884, 0.897], h 0.050 | reported, 0.889 [0.881, 0.894] | reported, 0.790 [0.750, 0.827] | loss, 1.500 (heap 1.500) | reported, 2.143 (heap 2.143) |
| 14 (n 128, rho 4) | loss, 0.609 [0.595, 0.619], h 0.050 | loss, 0.899 [0.894, 0.904], h 0.050 | reported, 0.897 [0.892, 0.903] | reported, 0.791 [0.769, 0.864] | win, 0.750 (heap 0.750) | reported, 1.714 (heap 1.714) |
| 15 (n 128, rho 16) | loss, 0.666 [0.662, 0.670], h 0.050 | loss, 0.897 [0.893, 0.903], h 0.050 | reported, 0.896 [0.892, 0.901] | reported, 0.822 [0.763, 0.854] | win, 0.562 (heap 0.563) | reported, 1.607 (heap 1.607) |
| 16 (n 128, rho 64) | loss, 0.701 [0.690, 0.708], h 0.050 | loss, 0.903 [0.895, 0.910], h 0.050 | reported, 0.902 [0.894, 0.909] | reported, 0.813 [0.781, 0.913] | win, 0.531 (heap 0.532) | reported, 1.589 (heap 1.589) |

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.476 [0.468, 0.483], h 0.050 | loss, 0.859 [0.855, 0.863], h 0.050 | reported, 0.838 [0.834, 0.842] | reported, 0.513 [0.505, 0.549] | win, 0.802 (heap 0.819) | reported, 1.177 (heap 1.187) |
| 2 (n 16, rho 4) | loss, 0.547 [0.541, 0.554], h 0.050 | loss, 0.861 [0.856, 0.865], h 0.050 | reported, 0.846 [0.841, 0.850] | reported, 0.527 [0.506, 0.647] | win, 0.294 (heap 0.321) | reported, 0.890 (heap 0.906) |
| 3 (n 16, rho 16) | loss, 0.604 [0.593, 0.616], h 0.050 | loss, 0.860 [0.846, 0.865], h 0.050 | reported, 0.850 [0.836, 0.855] | reported, 0.521 [0.489, 0.654] | win, 0.178 (heap 0.206) | reported, 0.820 (heap 0.836) |
| 4 (n 16, rho 64) | loss, 0.601 [0.596, 0.617], h 0.050 | loss, 0.849 [0.848, 0.853], h 0.050 | reported, 0.841 [0.839, 0.844] | reported, 0.516 [0.507, 0.593] | win, 0.141 (heap 0.170) | reported, 0.795 (heap 0.812) |
| 5 (n 32, rho 1) | loss, 0.524 [0.522, 0.525], h 0.050 | loss, 0.889 [0.883, 0.898], h 0.050 | reported, 0.880 [0.873, 0.888] | reported, 0.670 [0.580, 0.712] | win, 0.743 (heap 0.748) | reported, 1.118 (heap 1.121) |
| 6 (n 32, rho 4) | loss, 0.602 [0.598, 0.606], h 0.050 | loss, 0.886 [0.882, 0.890], h 0.050 | reported, 0.880 [0.876, 0.883] | reported, 0.604 [0.538, 0.689] | win, 0.388 (heap 0.394) | reported, 0.915 (heap 0.919) |
| 7 (n 32, rho 16) | loss, 0.673 [0.664, 0.680], h 0.050 | loss, 0.883 [0.879, 0.889], h 0.050 | reported, 0.879 [0.875, 0.885] | reported, 0.547 [0.524, 0.657] | win, 0.167 (heap 0.174) | reported, 0.789 (heap 0.793) |
| 8 (n 32, rho 64) | loss, 0.659 [0.655, 0.669], h 0.050 | loss, 0.872 [0.864, 0.877], h 0.050 | reported, 0.868 [0.860, 0.873] | reported, 0.643 [0.510, 0.723] | win, 0.139 (heap 0.146) | reported, 0.771 (heap 0.776) |
| 9 (n 64, rho 1) | loss, 0.540 [0.535, 0.542], h 0.050 | loss, 0.894 [0.890, 0.899], h 0.050 | reported, 0.890 [0.886, 0.894] | reported, 0.715 [0.643, 0.837] | win, 0.682 (heap 0.683) | reported, 1.073 (heap 1.074) |
| 10 (n 64, rho 4) | loss, 0.628 [0.625, 0.632], h 0.050 | loss, 0.897 [0.892, 0.901], h 0.050 | reported, 0.894 [0.889, 0.898] | reported, 0.738 [0.688, 0.793] | win, 0.374 (heap 0.376) | reported, 0.897 (heap 0.898) |
| 11 (n 64, rho 16) | loss, 0.689 [0.685, 0.691], h 0.050 | loss, 0.892 [0.888, 0.899], h 0.050 | reported, 0.890 [0.886, 0.897] | reported, 0.701 [0.633, 0.798] | win, 0.284 (heap 0.286) | reported, 0.846 (heap 0.847) |
| 12 (n 64, rho 64) | loss, 0.691 [0.686, 0.701], h 0.050 | loss, 0.898 [0.886, 0.902], h 0.050 | reported, 0.896 [0.884, 0.900] | reported, 0.702 [0.646, 0.799] | win, 0.136 (heap 0.138) | reported, 0.761 (heap 0.762) |
| 13 (n 128, rho 1) | loss, 0.550 [0.546, 0.555], h 0.050 | loss, 0.899 [0.892, 0.903], h 0.050 | reported, 0.896 [0.890, 0.901] | reported, 0.790 [0.756, 0.828] | win, 0.559 (heap 0.560) | reported, 1.107 (heap 1.107) |
| 14 (n 128, rho 4) | loss, 0.635 [0.632, 0.640], h 0.050 | loss, 0.897 [0.888, 0.905], h 0.050 | reported, 0.896 [0.887, 0.904] | reported, 0.796 [0.772, 0.862] | win, 0.358 (heap 0.359) | reported, 0.992 (heap 0.992) |
| 15 (n 128, rho 16) | loss, 0.689 [0.681, 0.692], h 0.050 | loss, 0.888 [0.881, 0.900], h 0.050 | reported, 0.887 [0.879, 0.899] | reported, 0.820 [0.792, 0.851] | win, 0.281 (heap 0.281) | reported, 0.948 (heap 0.948) |
| 16 (n 128, rho 64) | loss, 0.699 [0.693, 0.706], h 0.050 | loss, 0.902 [0.892, 0.909], h 0.050 | reported, 0.901 [0.891, 0.908] | reported, 0.780 [0.750, 0.840] | win, 0.259 (heap 0.259) | reported, 0.935 (heap 0.935) |

### Hash-V-pack against Hash-V-fixed (pack against fixed)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.704 [0.699, 0.707], h 0.050 | tie, 0.984 [0.980, 0.989], h 0.050 | reported, 0.968 [0.964, 0.973] | reported, 0.852 [0.841, 0.866] | win, 0.357 (heap 0.366) | reported, 0.515 (heap 0.523) |
| 2 (n 16, rho 4) | loss, 0.824 [0.818, 0.829], h 0.050 | tie, 0.986 [0.984, 0.990], h 0.050 | reported, 0.978 [0.975, 0.982] | reported, 0.851 [0.842, 0.859] | win, 0.131 (heap 0.144) | reported, 0.390 (heap 0.399) |
| 3 (n 16, rho 16) | loss, 0.843 [0.833, 0.849], h 0.050 | tie, 0.989 [0.973, 0.993], h 0.050 | reported, 0.983 [0.968, 0.988] | reported, 0.851 [0.806, 0.876] | win, 0.079 (heap 0.092) | reported, 0.359 (heap 0.368) |
| 4 (n 16, rho 64) | loss, 0.856 [0.855, 0.873], h 0.050 | tie, 0.992 [0.988, 0.995], h 0.050 | reported, 0.988 [0.983, 0.991] | reported, 0.850 [0.845, 0.871] | win, 0.062 (heap 0.076) | reported, 0.348 (heap 0.358) |
| 5 (n 32, rho 1) | loss, 0.730 [0.725, 0.732], h 0.050 | tie, 1.000 [0.993, 1.005], h 0.050 | reported, 0.993 [0.986, 0.998] | reported, 0.855 [0.803, 0.908] | win, 0.330 (heap 0.333) | reported, 0.489 (heap 0.491) |
| 6 (n 32, rho 4) | loss, 0.856 [0.847, 0.861], h 0.050 | tie, 0.997 [0.993, 1.000], h 0.050 | reported, 0.994 [0.989, 0.997] | reported, 0.866 [0.831, 0.898] | win, 0.173 (heap 0.176) | reported, 0.401 (heap 0.403) |
| 7 (n 32, rho 16) | loss, 0.892 [0.884, 0.900], h 0.050 | tie, 0.994 [0.991, 1.001], h 0.050 | reported, 0.992 [0.989, 0.999] | reported, 0.862 [0.837, 0.906] | win, 0.074 (heap 0.078) | reported, 0.345 (heap 0.348) |
| 8 (n 32, rho 64) | loss, 0.883 [0.880, 0.891], h 0.050 | tie, 0.992 [0.982, 1.001], h 0.050 | reported, 0.990 [0.980, 0.999] | reported, 0.880 [0.834, 0.902] | win, 0.062 (heap 0.065) | reported, 0.337 (heap 0.340) |
| 9 (n 64, rho 1) | loss, 0.748 [0.743, 0.756], h 0.050 | tie, 1.000 [0.997, 1.004], h 0.050 | reported, 0.997 [0.994, 1.000] | reported, 0.926 [0.873, 0.959] | win, 0.303 (heap 0.304) | reported, 0.469 (heap 0.470) |
| 10 (n 64, rho 4) | loss, 0.872 [0.867, 0.876], h 0.050 | tie, 1.003 [0.995, 1.016], h 0.050 | reported, 1.002 [0.993, 1.014] | reported, 0.922 [0.904, 0.947] | win, 0.166 (heap 0.167) | reported, 0.392 (heap 0.393) |
| 11 (n 64, rho 16) | loss, 0.921 [0.911, 0.925], h 0.050 | tie, 0.996 [0.990, 1.003], h 0.050 | reported, 0.996 [0.989, 1.003] | reported, 0.912 [0.844, 0.959] | win, 0.126 (heap 0.127) | reported, 0.370 (heap 0.371) |
| 12 (n 64, rho 64) | loss, 0.923 [0.914, 0.929], h 0.050 | tie, 0.999 [0.987, 1.003], h 0.050 | reported, 0.998 [0.986, 1.003] | reported, 0.933 [0.877, 0.964] | win, 0.060 (heap 0.061) | reported, 0.333 (heap 0.334) |
| 13 (n 128, rho 1) | loss, 0.818 [0.812, 0.828], h 0.050 | tie, 1.013 [1.002, 1.020], h 0.050 | reported, 1.012 [1.001, 1.018] | reported, 1.182 [1.130, 1.259] | win, 0.224 (heap 0.224) | reported, 0.436 (heap 0.437) |
| 14 (n 128, rho 4) | loss, 0.890 [0.884, 0.895], h 0.050 | tie, 0.998 [0.991, 1.005], h 0.050 | reported, 0.997 [0.990, 1.004] | reported, 1.204 [1.140, 1.262] | win, 0.143 (heap 0.143) | reported, 0.391 (heap 0.391) |
| 15 (n 128, rho 16) | loss, 0.919 [0.909, 0.924], h 0.050 | tie, 0.990 [0.982, 1.007], h 0.050 | reported, 0.989 [0.981, 1.007] | reported, 1.232 [1.157, 1.307] | win, 0.112 (heap 0.113) | reported, 0.374 (heap 0.374) |
| 16 (n 128, rho 64) | loss, 0.926 [0.916, 0.935], h 0.050 | tie, 0.999 [0.988, 1.004], h 0.050 | reported, 0.998 [0.988, 1.003] | reported, 1.190 [1.132, 1.325] | win, 0.103 (heap 0.104) | reported, 0.369 (heap 0.369) |

### Hash-V-grow-map against Hash-V-fixed (grow-map against fixed)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.502 [0.499, 0.504], h 0.050 | tie, 0.970 [0.968, 0.973], h 0.050 | reported, 0.935 [0.934, 0.939] | reported, 0.937 [0.933, 0.946] | tie, 1.000 (heap 1.000) | reported, 1.062 (heap 1.062) |
| 2 (n 16, rho 4) | loss, 0.762 [0.752, 0.768], h 0.050 | tie, 0.967 [0.965, 0.970], h 0.050 | reported, 0.957 [0.954, 0.959] | reported, 0.939 [0.930, 0.944] | win, 0.333 (heap 0.342) | reported, 0.688 (heap 0.692) |
| 3 (n 16, rho 16) | loss, 0.807 [0.798, 0.809], h 0.050 | tie, 0.967 [0.962, 0.973], h 0.050 | reported, 0.962 [0.957, 0.967] | reported, 0.934 [0.925, 0.967] | win, 0.167 (heap 0.178) | reported, 0.594 (heap 0.599) |
| 4 (n 16, rho 64) | loss, 0.887 [0.882, 0.891], h 0.050 | tie, 0.970 [0.965, 0.974], h 0.050 | reported, 0.967 [0.963, 0.971] | reported, 0.940 [0.922, 0.949] | win, 0.125 (heap 0.136) | reported, 0.570 (heap 0.576) |
| 5 (n 32, rho 1) | loss, 0.629 [0.626, 0.633], h 0.050 | tie, 0.989 [0.980, 0.994], h 0.050 | reported, 0.979 [0.969, 0.983] | reported, 0.957 [0.936, 0.994] | win, 0.556 (heap 0.557) | reported, 0.813 (heap 0.813) |
| 6 (n 32, rho 4) | loss, 0.758 [0.753, 0.761], h 0.050 | tie, 0.989 [0.986, 0.993], h 0.050 | reported, 0.983 [0.980, 0.987] | reported, 0.954 [0.927, 0.979] | win, 0.333 (heap 0.335) | reported, 0.688 (heap 0.689) |
| 7 (n 32, rho 16) | loss, 0.851 [0.843, 0.856], h 0.050 | tie, 0.988 [0.984, 0.991], h 0.050 | reported, 0.985 [0.981, 0.988] | reported, 0.957 [0.931, 0.984] | win, 0.167 (heap 0.169) | reported, 0.594 (heap 0.595) |
| 8 (n 32, rho 64) | loss, 0.864 [0.854, 0.868], h 0.050 | tie, 0.983 [0.974, 0.989], h 0.050 | reported, 0.980 [0.971, 0.987] | reported, 0.936 [0.924, 0.962] | win, 0.125 (heap 0.128) | reported, 0.570 (heap 0.572) |
| 9 (n 64, rho 1) | loss, 0.644 [0.638, 0.653], h 0.050 | tie, 0.999 [0.995, 1.004], h 0.050 | reported, 0.994 [0.990, 0.999] | reported, 1.012 [0.988, 1.030] | win, 0.556 (heap 0.556) | reported, 0.813 (heap 0.813) |
| 10 (n 64, rho 4) | loss, 0.834 [0.826, 0.836], h 0.050 | tie, 0.998 [0.991, 1.013], h 0.050 | reported, 0.996 [0.989, 1.011] | reported, 1.011 [0.978, 1.028] | win, 0.222 (heap 0.223) | reported, 0.625 (heap 0.625) |
| 11 (n 64, rho 16) | loss, 0.897 [0.888, 0.902], h 0.050 | tie, 0.996 [0.992, 1.001], h 0.050 | reported, 0.995 [0.991, 1.000] | reported, 0.993 [0.963, 1.036] | win, 0.167 (heap 0.167) | reported, 0.594 (heap 0.594) |
| 12 (n 64, rho 64) | loss, 0.920 [0.912, 0.924], h 0.050 | tie, 0.993 [0.986, 1.000], h 0.050 | reported, 0.992 [0.985, 1.000] | reported, 1.013 [0.967, 1.031] | win, 0.125 (heap 0.126) | reported, 0.570 (heap 0.571) |
| 13 (n 128, rho 1) | loss, 0.689 [0.679, 0.699], h 0.050 | tie, 1.006 [0.998, 1.012], h 0.050 | reported, 1.004 [0.996, 1.009] | reported, 1.182 [1.131, 1.260] | win, 0.600 (heap 0.600) | reported, 0.845 (heap 0.845) |
| 14 (n 128, rho 4) | loss, 0.853 [0.837, 0.865], h 0.050 | tie, 1.000 [0.996, 1.004], h 0.050 | reported, 0.999 [0.995, 1.003] | reported, 1.198 [1.125, 1.284] | win, 0.300 (heap 0.300) | reported, 0.676 (heap 0.676) |
| 15 (n 128, rho 16) | loss, 0.888 [0.882, 0.894], h 0.050 | tie, 1.000 [0.993, 1.008], h 0.050 | reported, 1.000 [0.993, 1.008] | reported, 1.235 [1.108, 1.323] | win, 0.225 (heap 0.225) | reported, 0.634 (heap 0.634) |
| 16 (n 128, rho 64) | loss, 0.929 [0.915, 0.936], h 0.050 | tie, 1.001 [0.991, 1.007], h 0.050 | reported, 1.000 [0.990, 1.007] | reported, 1.240 [1.166, 1.399] | win, 0.212 (heap 0.213) | reported, 0.627 (heap 0.627) |

### Hash-V-pack against Hash-V-grow-map (pack against grow-map)

| job | 1 build time of one input matrix | 4 sum-of-products time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | peak bytes during the operation (reported) |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 1.402 [1.392, 1.413] | reported, 1.015 [1.009, 1.019] | reported, 1.035 [1.030, 1.040] | reported, 0.910 [0.899, 0.924] | reported, 0.357 (heap 0.366) | reported, 0.485 (heap 0.492) |
| 2 (n 16, rho 4) | reported, 1.082 [1.075, 1.095] | reported, 1.020 [1.016, 1.023] | reported, 1.023 [1.019, 1.026] | reported, 0.906 [0.894, 0.920] | reported, 0.393 (heap 0.419) | reported, 0.567 (heap 0.576) |
| 3 (n 16, rho 16) | reported, 1.045 [1.037, 1.057] | reported, 1.022 [1.003, 1.029] | reported, 1.022 [1.004, 1.029] | reported, 0.911 [0.862, 0.919] | reported, 0.475 (heap 0.518) | reported, 0.604 (heap 0.614) |
| 4 (n 16, rho 64) | reported, 0.966 [0.961, 0.987] | reported, 1.023 [1.019, 1.026] | reported, 1.022 [1.018, 1.024] | reported, 0.904 [0.895, 0.938] | reported, 0.500 (heap 0.560) | reported, 0.610 (heap 0.621) |
| 5 (n 32, rho 1) | reported, 1.160 [1.153, 1.164] | reported, 1.011 [1.005, 1.019] | reported, 1.015 [1.009, 1.023] | reported, 0.893 [0.843, 0.929] | reported, 0.595 (heap 0.597) | reported, 0.602 (heap 0.604) |
| 6 (n 32, rho 4) | reported, 1.129 [1.121, 1.135] | reported, 1.009 [1.003, 1.011] | reported, 1.011 [1.006, 1.013] | reported, 0.907 [0.886, 0.934] | reported, 0.518 (heap 0.523) | reported, 0.583 (heap 0.585) |
| 7 (n 32, rho 16) | reported, 1.049 [1.041, 1.060] | reported, 1.006 [1.003, 1.013] | reported, 1.007 [1.003, 1.014] | reported, 0.901 [0.869, 0.942] | reported, 0.445 (heap 0.458) | reported, 0.581 (heap 0.584) |
| 8 (n 32, rho 64) | reported, 1.021 [1.016, 1.036] | reported, 1.010 [1.001, 1.020] | reported, 1.010 [1.002, 1.020] | reported, 0.941 [0.889, 0.965] | reported, 0.493 (heap 0.508) | reported, 0.592 (heap 0.595) |
| 9 (n 64, rho 1) | reported, 1.161 [1.152, 1.173] | reported, 1.001 [0.998, 1.005] | reported, 1.003 [1.000, 1.007] | reported, 0.915 [0.861, 0.960] | reported, 0.545 (heap 0.546) | reported, 0.578 (heap 0.578) |
| 10 (n 64, rho 4) | reported, 1.046 [1.039, 1.055] | reported, 1.005 [0.998, 1.010] | reported, 1.006 [1.000, 1.011] | reported, 0.912 [0.887, 0.949] | reported, 0.748 (heap 0.749) | reported, 0.628 (heap 0.629) |
| 11 (n 64, rho 16) | reported, 1.027 [1.021, 1.030] | reported, 1.000 [0.995, 1.005] | reported, 1.000 [0.996, 1.005] | reported, 0.919 [0.839, 0.966] | reported, 0.758 (heap 0.760) | reported, 0.623 (heap 0.624) |
| 12 (n 64, rho 64) | reported, 1.003 [1.000, 1.010] | reported, 1.006 [0.993, 1.012] | reported, 1.006 [0.993, 1.012] | reported, 0.921 [0.861, 0.966] | reported, 0.483 (heap 0.487) | reported, 0.584 (heap 0.585) |
| 13 (n 128, rho 1) | reported, 1.187 [1.171, 1.204] | reported, 1.008 [0.999, 1.014] | reported, 1.008 [1.000, 1.014] | reported, 1.000 [0.966, 1.048] | reported, 0.373 (heap 0.373) | reported, 0.516 (heap 0.517) |
| 14 (n 128, rho 4) | reported, 1.043 [1.028, 1.066] | reported, 0.998 [0.991, 1.005] | reported, 0.999 [0.991, 1.005] | reported, 1.005 [0.957, 1.035] | reported, 0.477 (heap 0.478) | reported, 0.579 (heap 0.579) |
| 15 (n 128, rho 16) | reported, 1.035 [1.025, 1.040] | reported, 0.989 [0.981, 1.003] | reported, 0.989 [0.981, 1.003] | reported, 0.997 [0.965, 1.068] | reported, 0.499 (heap 0.500) | reported, 0.590 (heap 0.590) |
| 16 (n 128, rho 64) | reported, 0.997 [0.984, 1.012] | reported, 0.998 [0.992, 1.005] | reported, 0.998 [0.992, 1.005] | reported, 0.959 [0.895, 0.990] | reported, 0.487 (heap 0.487) | reported, 0.588 (heap 0.588) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m2 item 7); the counting runs' per-operation records of every hash row equal TC's, every step; Hash-V-pack and Hash-V-grow-map take Hash-V-fixed's arithmetic counters and store splits (spec v0.2 §15.7 items 3-4).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | G(Hash-V-pack)/G(TC), call for call | pack vs fixed | grow-map vs fixed |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 34.01 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 2 | 33.96 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 3 | 34.57 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 4 | 36.30 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 5 | 35.99 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 6 | 35.78 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 7 | 35.44 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 8 | 35.24 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 9 | 37.38 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 10 | 37.49 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 11 | 37.55 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 12 | 38.34 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 13 | 38.58 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 14 | 38.37 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 15 | 38.66 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 16 | 38.25 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m2 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.697 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.888 | 0 |
| NONE | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| NONE | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.457 | 0 |
| NONE | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.886 | 0 |
| NONE | Hash-V-grow-map vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 1.375 | 1 |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.532 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| NONE | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.713 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.739 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 1.000 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.637 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.994 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | stable | 3 | 1 | 0 | 0 | 0 | 0 | 0.578 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.174 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.009 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.459 | 0 |
| LOW | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.709 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.891 | 0 |
| LOW | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| LOW | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.567 | 0 |
| LOW | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.886 | 0 |
| LOW | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.750 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.615 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| LOW | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.366 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.864 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.998 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.155 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.798 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.994 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.064 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.007 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.498 | 0 |
| MID | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.749 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| MID | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| MID | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.654 | 0 |
| MID | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.885 | 0 |
| MID | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.375 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.681 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.886 | 0 |
| MID | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.230 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.905 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.096 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.869 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.992 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.167 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.040 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.003 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.487 | 0 |
| HIGH | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.748 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.889 | 0 |
| HIGH | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| HIGH | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.667 | 0 |
| HIGH | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.878 | 0 |
| HIGH | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.281 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.675 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.885 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.140 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.903 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.995 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.062 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.904 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.988 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.125 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.000 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.008 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.490 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-fixed against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.676 | 0.664 | 0.716 | 0.702 |
| 32 | 0.718 | 0.703 | 0.755 | 0.747 |
| 64 | 0.722 | 0.720 | 0.748 | 0.749 |
| 128 | 0.673 | 0.714 | 0.750 | 0.754 |

Hash-V-fixed against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.873 | 0.873 | 0.870 | 0.856 |
| 32 | 0.889 | 0.889 | 0.889 | 0.879 |
| 64 | 0.895 | 0.894 | 0.896 | 0.899 |
| 128 | 0.887 | 0.899 | 0.897 | 0.903 |

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
| 16 | 0.339 | 0.506 | 0.578 | 0.622 |
| 32 | 0.451 | 0.533 | 0.642 | 0.645 |
| 64 | 0.465 | 0.601 | 0.671 | 0.689 |
| 128 | 0.463 | 0.609 | 0.666 | 0.701 |

Hash-V-grow-map against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.846 | 0.844 | 0.842 | 0.830 |
| 32 | 0.880 | 0.879 | 0.878 | 0.864 |
| 64 | 0.894 | 0.893 | 0.892 | 0.892 |
| 128 | 0.892 | 0.899 | 0.897 | 0.903 |

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
| 16 | 0.476 | 0.547 | 0.604 | 0.601 |
| 32 | 0.524 | 0.602 | 0.673 | 0.659 |
| 64 | 0.540 | 0.628 | 0.689 | 0.691 |
| 128 | 0.550 | 0.635 | 0.689 | 0.699 |

Hash-V-pack against TC, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.859 | 0.861 | 0.860 | 0.849 |
| 32 | 0.889 | 0.886 | 0.883 | 0.872 |
| 64 | 0.894 | 0.897 | 0.892 | 0.898 |
| 128 | 0.899 | 0.897 | 0.888 | 0.902 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.802 | 0.294 | 0.178 | 0.141 |
| 32 | 0.743 | 0.388 | 0.167 | 0.139 |
| 64 | 0.682 | 0.374 | 0.284 | 0.136 |
| 128 | 0.559 | 0.358 | 0.281 | 0.259 |

Hash-V-pack against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.704 | 0.824 | 0.843 | 0.856 |
| 32 | 0.730 | 0.856 | 0.892 | 0.883 |
| 64 | 0.748 | 0.872 | 0.921 | 0.923 |
| 128 | 0.818 | 0.890 | 0.919 | 0.926 |

Hash-V-pack against Hash-V-fixed, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.984 | 0.986 | 0.989 | 0.992 |
| 32 | 1.000 | 0.997 | 0.994 | 0.992 |
| 64 | 1.000 | 1.003 | 0.996 | 0.999 |
| 128 | 1.013 | 0.998 | 0.990 | 0.999 |

Hash-V-pack against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.131 | 0.079 | 0.062 |
| 32 | 0.330 | 0.173 | 0.074 | 0.062 |
| 64 | 0.303 | 0.166 | 0.126 | 0.060 |
| 128 | 0.224 | 0.143 | 0.112 | 0.103 |

Hash-V-grow-map against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.502 | 0.762 | 0.807 | 0.887 |
| 32 | 0.629 | 0.758 | 0.851 | 0.864 |
| 64 | 0.644 | 0.834 | 0.897 | 0.920 |
| 128 | 0.689 | 0.853 | 0.888 | 0.929 |

Hash-V-grow-map against Hash-V-fixed, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.970 | 0.967 | 0.967 | 0.970 |
| 32 | 0.989 | 0.989 | 0.988 | 0.983 |
| 64 | 0.999 | 0.998 | 0.996 | 0.993 |
| 128 | 1.006 | 1.000 | 1.000 | 1.001 |

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
| 16 | 1.402 | 1.082 | 1.045 | 0.966 |
| 32 | 1.160 | 1.129 | 1.049 | 1.021 |
| 64 | 1.161 | 1.046 | 1.027 | 1.003 |
| 128 | 1.187 | 1.043 | 1.035 | 0.997 |

Hash-V-pack against Hash-V-grow-map, 4 sum-of-products time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.015 | 1.020 | 1.022 | 1.023 |
| 32 | 1.011 | 1.009 | 1.006 | 1.010 |
| 64 | 1.001 | 1.005 | 1.000 | 1.006 |
| 128 | 1.008 | 0.998 | 0.989 | 0.998 |

Hash-V-pack against Hash-V-grow-map, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.393 | 0.475 | 0.500 |
| 32 | 0.595 | 0.518 | 0.445 | 0.493 |
| 64 | 0.545 | 0.748 | 0.758 | 0.483 |
| 128 | 0.373 | 0.477 | 0.499 | 0.487 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (7 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build, over that input's cells; Hash-V-grow-map's growth peak: the job's total at its largest growth, over the operation's cells.

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | C's stable (build peak) | D's stable (build peak) | growth peak |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 1 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 36.57 (37.04) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 1 | Hash-V-grow-map | 36.00 (36.48) | 36.00 (36.48) | 38.86 (39.34) | 36.00 (56.00) | 36.00 (56.00) | 36.00 (56.00) | 36.00 (56.00) | 38.86 (39.34), 42 growths |
| 1 | Hash-V-pack | 12.84 (13.36) | 12.92 (13.44) | 18.84 (19.37) | 12.81 (52.81) | 12.84 (52.84) | 12.88 (52.88) | 12.81 (52.81) | - |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 2 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 36.57 (37.04) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 2 | Hash-V-grow-map | 12.00 (12.48) | 16.80 (17.28) | 25.14 (25.63) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 25.14 (25.63), 34 growths |
| 2 | Hash-V-pack | 4.71 (5.23) | 6.46 (6.98) | 14.25 (14.77) | 4.62 (46.62) | 4.72 (46.72) | 4.75 (46.75) | 4.75 (46.75) | - |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 3 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 36.57 (37.04) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 3 | Hash-V-grow-map | 6.00 (6.48) | 12.00 (12.48) | 21.71 (22.20) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 21.71 (22.20), 26 growths |
| 3 | Hash-V-pack | 2.85 (3.36) | 4.95 (5.46) | 13.12 (13.63) | 2.78 (44.78) | 2.84 (44.84) | 2.88 (44.88) | 2.91 (44.91) | - |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 4 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 36.57 (37.04) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 4 | Hash-V-grow-map | 4.50 (4.97) | 10.80 (11.27) | 20.86 (21.34) | 4.50 (4.50) | 4.50 (4.50) | 4.50 (4.50) | 4.50 (4.50) | 20.86 (21.34), 18 growths |
| 4 | Hash-V-pack | 2.25 (2.78) | 4.45 (4.98) | 12.71 (13.25) | 2.25 (44.25) | 2.25 (44.25) | 2.25 (44.25) | 2.25 (44.25) | - |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 5 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 36.57 (36.69) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 5 | Hash-V-grow-map | 20.00 (20.12) | 23.20 (23.32) | 29.71 (29.83) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 29.71 (29.83), 52 growths |
| 5 | Hash-V-pack | 11.89 (12.02) | 12.02 (12.15) | 17.89 (18.02) | 11.91 (51.91) | 11.90 (51.90) | 11.84 (51.84) | 11.93 (51.93) | - |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 6 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 36.57 (36.69) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 6 | Hash-V-grow-map | 12.00 (12.12) | 16.80 (16.92) | 25.14 (25.26) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 12.00 (17.00) | 25.14 (25.26), 48 growths |
| 6 | Hash-V-pack | 6.21 (6.34) | 7.48 (7.61) | 14.65 (14.78) | 6.22 (46.22) | 6.22 (46.22) | 6.20 (46.20) | 6.21 (46.21) | - |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 7 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 36.57 (36.69) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 7 | Hash-V-grow-map | 6.00 (6.12) | 12.00 (12.12) | 21.71 (21.83) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 21.71 (21.83), 40 growths |
| 7 | Hash-V-pack | 2.67 (2.80) | 4.64 (4.78) | 12.62 (12.75) | 2.64 (44.64) | 2.69 (44.69) | 2.67 (44.67) | 2.69 (44.69) | - |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 8 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 36.57 (36.69) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 8 | Hash-V-grow-map | 4.50 (4.62) | 10.80 (10.92) | 20.86 (20.98) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 20.86 (20.98), 32 growths |
| 8 | Hash-V-pack | 2.22 (2.35) | 4.27 (4.40) | 12.34 (12.47) | 2.20 (44.20) | 2.22 (44.22) | 2.22 (44.22) | 2.23 (44.23) | - |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 9 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 36.57 (36.60) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 9 | Hash-V-grow-map | 20.00 (20.03) | 23.20 (23.23) | 29.71 (29.74) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 20.00 (30.00) | 29.71 (29.74), 66 growths |
| 9 | Hash-V-pack | 10.91 (10.94) | 11.16 (11.19) | 17.16 (17.20) | 10.89 (50.89) | 10.91 (50.91) | 10.89 (50.89) | 10.95 (50.95) | - |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 10 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 36.57 (36.60) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 10 | Hash-V-grow-map | 8.00 (8.03) | 13.60 (13.63) | 22.86 (22.89) | 8.00 (10.50) | 8.00 (10.50) | 8.00 (10.50) | 8.00 (10.50) | 22.86 (22.89), 58 growths |
| 10 | Hash-V-pack | 5.98 (6.02) | 7.22 (7.25) | 14.35 (14.39) | 5.98 (45.98) | 5.98 (45.98) | 5.98 (45.98) | 6.00 (46.00) | - |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 11 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 36.57 (36.60) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 11 | Hash-V-grow-map | 6.00 (6.03) | 12.00 (12.03) | 21.71 (21.74) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 6.00 (7.25) | 21.71 (21.74), 54 growths |
| 11 | Hash-V-pack | 4.55 (4.58) | 6.08 (6.11) | 13.53 (13.57) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) | 4.55 (44.55) | - |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 12 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 36.57 (36.60) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | 36.00 (36.00) | - |
| 12 | Hash-V-grow-map | 4.50 (4.53) | 10.80 (10.83) | 20.86 (20.89) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 4.50 (4.81) | 20.86 (20.89), 46 growths |
| 12 | Hash-V-pack | 2.17 (2.21) | 4.17 (4.21) | 12.18 (12.21) | 2.17 (44.17) | 2.18 (44.18) | 2.17 (44.17) | 2.18 (44.18) | - |
| 13 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 13 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 40.57 (40.58) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 13 | Hash-V-grow-map | 24.00 (24.01) | 27.20 (27.21) | 34.29 (34.29) | 24.00 (36.00) | 24.00 (36.00) | 24.00 (36.00) | 24.00 (36.00) | 34.29 (34.29), 80 growths |
| 13 | Hash-V-pack | 8.95 (8.96) | 9.57 (9.58) | 17.71 (17.72) | 8.99 (60.99) | 8.94 (60.94) | 8.92 (60.92) | 8.95 (60.95) | - |
| 14 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 14 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 40.57 (40.58) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 14 | Hash-V-grow-map | 12.00 (12.01) | 17.60 (17.61) | 27.43 (27.44) | 12.00 (15.00) | 12.00 (15.00) | 12.00 (15.00) | 12.00 (15.00) | 27.43 (27.44), 72 growths |
| 14 | Hash-V-pack | 5.73 (5.74) | 6.99 (7.00) | 15.87 (15.88) | 5.74 (57.74) | 5.73 (57.73) | 5.71 (57.71) | 5.74 (57.74) | - |
| 15 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 15 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 40.57 (40.58) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 15 | Hash-V-grow-map | 9.00 (9.01) | 15.20 (15.21) | 25.71 (25.72) | 9.00 (9.75) | 9.00 (9.75) | 9.00 (9.75) | 9.00 (9.75) | 25.71 (25.72), 64 growths |
| 15 | Hash-V-pack | 4.49 (4.50) | 6.01 (6.01) | 15.16 (15.17) | 4.50 (56.50) | 4.49 (56.49) | 4.49 (56.49) | 4.49 (56.49) | - |
| 16 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 16 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 40.57 (40.58) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | 40.00 (40.00) | - |
| 16 | Hash-V-grow-map | 8.50 (8.51) | 14.80 (14.81) | 25.43 (25.44) | 8.50 (8.88) | 8.50 (8.88) | 8.50 (8.88) | 8.50 (8.88) | 25.43 (25.44), 60 growths |
| 16 | Hash-V-pack | 4.14 (4.15) | 5.72 (5.73) | 14.96 (14.97) | 4.14 (56.14) | 4.14 (56.14) | 4.14 (56.14) | 4.14 (56.14) | - |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B | C | D |
| --- | --- | --- | --- | --- | --- |
| 1 | TC | 7233 | 6854 | 6988 | 6802 |
| 1 | Hash-V-fixed | 10457 | 10279 | 10279 | 10082 |
| 1 | Hash-V-grow-map | 20667 | 20360 | 20551 | 20269 |
| 1 | Hash-V-pack | 14864 | 14417 | 14779 | 14322 |
| 1 | Hash-V-fixed-null | 10494 | 10332 | 10363 | 10216 |
| 2 | TC | 6692 | 6917 | 6700 | 6716 |
| 2 | Hash-V-fixed | 9956 | 10240 | 10069 | 10225 |
| 2 | Hash-V-grow-map | 13131 | 13334 | 13324 | 13902 |
| 2 | Hash-V-pack | 12222 | 12352 | 12267 | 12573 |
| 2 | Hash-V-fixed-null | 9940 | 10218 | 10100 | 10292 |
| 3 | TC | 6144 | 6466 | 5793 | 6095 |
| 3 | Hash-V-fixed | 8248 | 8753 | 8130 | 8691 |
| 3 | Hash-V-grow-map | 9674 | 10729 | 10546 | 10695 |
| 3 | Hash-V-pack | 9805 | 10216 | 9715 | 10328 |
| 3 | Hash-V-fixed-null | 8233 | 8783 | 8116 | 8699 |
| 4 | TC | 5047 | 5411 | 4922 | 5435 |
| 4 | Hash-V-fixed | 7095 | 7842 | 7054 | 7770 |
| 4 | Hash-V-grow-map | 8264 | 8685 | 7949 | 8687 |
| 4 | Hash-V-pack | 8319 | 8835 | 8299 | 8910 |
| 4 | Hash-V-fixed-null | 7024 | 7737 | 7053 | 7749 |
| 5 | TC | 29646 | 29607 | 29794 | 29616 |
| 5 | Hash-V-fixed | 41005 | 41490 | 41909 | 40582 |
| 5 | Hash-V-grow-map | 64306 | 66021 | 65863 | 65598 |
| 5 | Hash-V-pack | 56634 | 56725 | 56940 | 55940 |
| 5 | Hash-V-fixed-null | 41049 | 41403 | 41775 | 40735 |
| 6 | TC | 28930 | 28554 | 29053 | 28835 |
| 6 | Hash-V-fixed | 42003 | 40930 | 40846 | 41152 |
| 6 | Hash-V-grow-map | 56064 | 54173 | 53617 | 54073 |
| 6 | Hash-V-pack | 49130 | 47678 | 47717 | 47907 |
| 6 | Hash-V-fixed-null | 41888 | 40879 | 40755 | 41167 |
| 7 | TC | 27640 | 29116 | 27862 | 29083 |
| 7 | Hash-V-fixed | 37247 | 38532 | 37081 | 38990 |
| 7 | Hash-V-grow-map | 43944 | 45948 | 43579 | 45498 |
| 7 | Hash-V-pack | 41893 | 43064 | 41783 | 43828 |
| 7 | Hash-V-fixed-null | 37185 | 38715 | 37169 | 38951 |
| 8 | TC | 25643 | 26820 | 25333 | 24805 |
| 8 | Hash-V-fixed | 34658 | 35411 | 33884 | 33074 |
| 8 | Hash-V-grow-map | 39252 | 41388 | 39961 | 37343 |
| 8 | Hash-V-pack | 39244 | 40145 | 37775 | 37151 |
| 8 | Hash-V-fixed-null | 34712 | 35579 | 34125 | 33221 |
| 9 | TC | 118806 | 118494 | 119056 | 118936 |
| 9 | Hash-V-fixed | 166065 | 164478 | 164468 | 163794 |
| 9 | Hash-V-grow-map | 259797 | 254403 | 255078 | 255002 |
| 9 | Hash-V-pack | 220733 | 219624 | 219764 | 220516 |
| 9 | Hash-V-fixed-null | 166722 | 165160 | 164946 | 165205 |
| 10 | TC | 120230 | 119221 | 117669 | 118201 |
| 10 | Hash-V-fixed | 164870 | 165744 | 164880 | 164752 |
| 10 | Hash-V-grow-map | 197912 | 197669 | 198154 | 199257 |
| 10 | Hash-V-pack | 189124 | 189914 | 188862 | 189680 |
| 10 | Hash-V-fixed-null | 165486 | 166704 | 163738 | 165760 |
| 11 | TC | 116956 | 117272 | 119766 | 116948 |
| 11 | Hash-V-fixed | 156848 | 154996 | 158312 | 153240 |
| 11 | Hash-V-grow-map | 174994 | 173308 | 177036 | 174967 |
| 11 | Hash-V-pack | 170310 | 170222 | 174872 | 169282 |
| 11 | Hash-V-fixed-null | 156180 | 154846 | 157996 | 153017 |
| 12 | TC | 109428 | 117620 | 112395 | 110726 |
| 12 | Hash-V-fixed | 148874 | 153831 | 149317 | 144716 |
| 12 | Hash-V-grow-map | 162718 | 175778 | 161396 | 156916 |
| 12 | Hash-V-pack | 160786 | 165205 | 161618 | 159028 |
| 12 | Hash-V-fixed-null | 148808 | 153528 | 148894 | 144766 |
| 13 | TC | 477246 | 478916 | 476957 | 476715 |
| 13 | Hash-V-fixed | 716328 | 712337 | 703854 | 696884 |
| 13 | Hash-V-grow-map | 1035892 | 1037937 | 1011015 | 1017720 |
| 13 | Hash-V-pack | 891383 | 870701 | 859958 | 854759 |
| 13 | Hash-V-fixed-null | 714690 | 714246 | 703925 | 694320 |
| 14 | TC | 474178 | 473187 | 474612 | 478266 |
| 14 | Hash-V-fixed | 670887 | 664549 | 659122 | 663995 |
| 14 | Hash-V-grow-map | 784058 | 798260 | 762721 | 768376 |
| 14 | Hash-V-pack | 758860 | 744366 | 740774 | 745574 |
| 14 | Hash-V-fixed-null | 668146 | 666508 | 658746 | 663052 |
| 15 | TC | 476038 | 474832 | 477134 | 479930 |
| 15 | Hash-V-fixed | 642284 | 635510 | 634480 | 633070 |
| 15 | Hash-V-grow-map | 716383 | 714408 | 716535 | 719154 |
| 15 | Hash-V-pack | 707074 | 693786 | 686198 | 689760 |
| 15 | Hash-V-fixed-null | 641098 | 640828 | 639058 | 640699 |
| 16 | TC | 477646 | 455756 | 474934 | 465976 |
| 16 | Hash-V-fixed | 638574 | 609071 | 625586 | 618388 |
| 16 | Hash-V-grow-map | 674038 | 663947 | 681800 | 662828 |
| 16 | Hash-V-pack | 697611 | 659110 | 675354 | 668174 |
| 16 | Hash-V-fixed-null | 639782 | 613284 | 634150 | 620046 |

## Stores, per job and matrix (rule-m2 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. Hash-V-pack's T is its build table's.

| job | row | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | cells bytes | table or list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Hash-V-fixed | A | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.13 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.12 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | C | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.11 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | D | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.09 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | P | 314 | 1024 | 30.7% | 64 | 16 | 198 / 314 | 1.16 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.10 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.11 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-grow-map | A | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.54 | 16 | 0 (0) | 6 | 504 (583) | 801 | 1024 | 8192 |
| 1 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.45 | 12 | 0 (0) | 6 | 504 (568) | 829 | 1024 | 8192 |
| 1 | Hash-V-grow-map | C | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.41 | 7 | 0 (0) | 6 | 504 (589) | 812 | 1024 | 8192 |
| 1 | Hash-V-grow-map | D | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.42 | 9 | 0 (0) | 6 | 504 (579) | 808 | 1024 | 8192 |
| 1 | Hash-V-grow-map | P | 314 | 1024 | 30.7% | 64 | 16 | 198 / 314 | 1.43 | 8 | 1 (16) | 6 | 504 (566) | 736 | 1024 | 8192 |
| 1 | Hash-V-grow-map | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.52 | 14 | 1 (16) | 6 | 504 (572) | 766 | 1024 | 8192 |
| 1 | Hash-V-grow-map | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.50 | 13 | 1 (16) | 6 | 504 (585) | 781 | 1024 | 8192 |
| 1 | Hash-V-pack | A | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.13 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2256 |
| 1 | Hash-V-pack | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.12 | 6 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2264 |
| 1 | Hash-V-pack | C | 284 | 1024 | 27.7% | 64 | 16 | 228 / 284 | 1.11 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2272 |
| 1 | Hash-V-pack | D | 282 | 1024 | 27.5% | 64 | 16 | 230 / 282 | 1.09 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2256 |
| 1 | Hash-V-pack | P | 314 | 1024 | 30.7% | 64 | 16 | 198 / 314 | 1.16 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2512 |
| 1 | Hash-V-pack | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.10 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2424 |
| 1 | Hash-V-pack | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.11 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2368 |
| 2 | Hash-V-fixed | A | 84 | 1024 | 8.2% | 64 | 16 | 428 / 84 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | B | 87 | 1024 | 8.5% | 64 | 16 | 425 / 87 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | C | 88 | 1024 | 8.6% | 64 | 16 | 424 / 88 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | D | 88 | 1024 | 8.6% | 64 | 16 | 424 / 88 | 1.08 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | P | 313 | 1024 | 30.6% | 64 | 16 | 199 / 313 | 1.13 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | Q | 309 | 1024 | 30.2% | 64 | 16 | 203 / 309 | 1.13 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | R | 302 | 1024 | 29.5% | 64 | 16 | 210 / 302 | 1.14 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-grow-map | A | 84 | 256 | 32.8% | 64 | 16 | 428 / 84 | 1.19 | 6 | 0 (0) | 4 | 120 (130) | 217 | 1024 | 2048 |
| 2 | Hash-V-grow-map | B | 87 | 256 | 34.0% | 64 | 16 | 425 / 87 | 1.21 | 6 | 0 (0) | 4 | 120 (134) | 198 | 1024 | 2048 |
| 2 | Hash-V-grow-map | C | 88 | 256 | 34.4% | 64 | 16 | 424 / 88 | 1.26 | 8 | 0 (0) | 4 | 120 (137) | 233 | 1024 | 2048 |
| 2 | Hash-V-grow-map | D | 88 | 256 | 34.4% | 64 | 16 | 424 / 88 | 1.35 | 10 | 0 (0) | 4 | 120 (149) | 212 | 1024 | 2048 |
| 2 | Hash-V-grow-map | P | 313 | 1024 | 30.6% | 64 | 16 | 199 / 313 | 1.74 | 25 | 1 (16) | 6 | 504 (619) | 709 | 1024 | 8192 |
| 2 | Hash-V-grow-map | Q | 309 | 1024 | 30.2% | 64 | 16 | 203 / 309 | 1.53 | 11 | 1 (16) | 6 | 504 (583) | 760 | 1024 | 8192 |
| 2 | Hash-V-grow-map | R | 302 | 1024 | 29.5% | 64 | 16 | 210 / 302 | 1.47 | 12 | 1 (16) | 6 | 504 (594) | 787 | 1024 | 8192 |
| 2 | Hash-V-pack | A | 84 | 1024 | 8.2% | 64 | 8 | 428 / 84 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 672 |
| 2 | Hash-V-pack | B | 87 | 1024 | 8.5% | 64 | 8 | 425 / 87 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 696 |
| 2 | Hash-V-pack | C | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.04 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 704 |
| 2 | Hash-V-pack | D | 88 | 1024 | 8.6% | 64 | 8 | 424 / 88 | 1.08 | 3 | 0 (0) | 0 | 0 (0) | 0 | 512 | 704 |
| 2 | Hash-V-pack | P | 313 | 1024 | 30.6% | 64 | 16 | 199 / 313 | 1.13 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2504 |
| 2 | Hash-V-pack | Q | 309 | 1024 | 30.2% | 64 | 16 | 203 / 309 | 1.13 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2472 |
| 2 | Hash-V-pack | R | 302 | 1024 | 29.5% | 64 | 16 | 210 / 302 | 1.14 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2416 |
| 3 | Hash-V-fixed | A | 25 | 1024 | 2.4% | 64 | 16 | 487 / 25 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | B | 27 | 1024 | 2.6% | 64 | 16 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | C | 28 | 1024 | 2.7% | 64 | 16 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | D | 29 | 1024 | 2.8% | 64 | 16 | 483 / 29 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | P | 307 | 1024 | 30.0% | 64 | 16 | 205 / 307 | 1.16 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.10 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | R | 299 | 1024 | 29.2% | 64 | 16 | 213 / 299 | 1.12 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-grow-map | A | 25 | 64 | 39.1% | 64 | 16 | 487 / 25 | 1.13 | 3 | 0 (0) | 2 | 24 (28) | 40 | 1024 | 512 |
| 3 | Hash-V-grow-map | B | 27 | 64 | 42.2% | 64 | 16 | 485 / 27 | 1.14 | 7 | 0 (0) | 2 | 24 (25) | 35 | 1024 | 512 |
| 3 | Hash-V-grow-map | C | 28 | 64 | 43.8% | 64 | 16 | 484 / 28 | 1.32 | 5 | 0 (0) | 2 | 24 (25) | 45 | 1024 | 512 |
| 3 | Hash-V-grow-map | D | 29 | 64 | 45.3% | 64 | 16 | 483 / 29 | 1.28 | 4 | 0 (0) | 2 | 24 (29) | 34 | 1024 | 512 |
| 3 | Hash-V-grow-map | P | 307 | 1024 | 30.0% | 64 | 16 | 205 / 307 | 1.58 | 15 | 1 (16) | 6 | 504 (570) | 744 | 1024 | 8192 |
| 3 | Hash-V-grow-map | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.39 | 10 | 1 (16) | 6 | 504 (559) | 773 | 1024 | 8192 |
| 3 | Hash-V-grow-map | R | 299 | 1024 | 29.2% | 64 | 16 | 213 / 299 | 1.46 | 12 | 1 (16) | 6 | 504 (567) | 783 | 1024 | 8192 |
| 3 | Hash-V-pack | A | 25 | 1024 | 2.4% | 64 | 8 | 487 / 25 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 200 |
| 3 | Hash-V-pack | B | 27 | 1024 | 2.6% | 64 | 8 | 485 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 216 |
| 3 | Hash-V-pack | C | 28 | 1024 | 2.7% | 64 | 8 | 484 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 224 |
| 3 | Hash-V-pack | D | 29 | 1024 | 2.8% | 64 | 8 | 483 / 29 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 232 |
| 3 | Hash-V-pack | P | 307 | 1024 | 30.0% | 64 | 16 | 205 / 307 | 1.16 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2456 |
| 3 | Hash-V-pack | Q | 303 | 1024 | 29.6% | 64 | 16 | 209 / 303 | 1.10 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2424 |
| 3 | Hash-V-pack | R | 299 | 1024 | 29.2% | 64 | 16 | 213 / 299 | 1.12 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2392 |
| 4 | Hash-V-fixed | A | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | B | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | C | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | D | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | P | 305 | 1024 | 29.8% | 64 | 16 | 207 / 305 | 1.09 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | Q | 295 | 1024 | 28.8% | 64 | 16 | 217 / 295 | 1.15 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.12 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-grow-map | A | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.24 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | B | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.36 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | C | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.20 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | D | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.37 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | P | 305 | 1024 | 29.8% | 64 | 16 | 207 / 305 | 1.37 | 7 | 1 (16) | 6 | 504 (565) | 778 | 1024 | 8192 |
| 4 | Hash-V-grow-map | Q | 295 | 1024 | 28.8% | 64 | 16 | 217 / 295 | 1.48 | 11 | 1 (16) | 6 | 504 (587) | 806 | 1024 | 8192 |
| 4 | Hash-V-grow-map | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.48 | 11 | 1 (16) | 6 | 504 (561) | 806 | 1024 | 8192 |
| 4 | Hash-V-pack | A | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | C | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | D | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | P | 305 | 1024 | 29.8% | 64 | 16 | 207 / 305 | 1.09 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2440 |
| 4 | Hash-V-pack | Q | 295 | 1024 | 28.8% | 64 | 16 | 217 / 295 | 1.15 | 5 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2360 |
| 4 | Hash-V-pack | R | 296 | 1024 | 28.9% | 64 | 16 | 216 / 296 | 1.12 | 8 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2368 |
| 5 | Hash-V-fixed | A | 1013 | 4096 | 24.7% | 64 | 16 | 1035 / 1013 | 1.06 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | B | 1011 | 4096 | 24.7% | 64 | 16 | 1037 / 1011 | 1.07 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | C | 1003 | 4096 | 24.5% | 64 | 16 | 1045 / 1003 | 1.09 | 8 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | D | 1015 | 4096 | 24.8% | 64 | 16 | 1033 / 1015 | 1.05 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | P | 1089 | 4096 | 26.6% | 64 | 16 | 959 / 1089 | 1.10 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | Q | 1100 | 4096 | 26.9% | 64 | 16 | 948 / 1100 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.10 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-grow-map | A | 1013 | 2048 | 49.5% | 64 | 16 | 1035 / 1013 | 1.33 | 12 | 0 (0) | 7 | 1016 (1116) | 1797 | 4096 | 16384 |
| 5 | Hash-V-grow-map | B | 1011 | 2048 | 49.4% | 64 | 16 | 1037 / 1011 | 1.44 | 17 | 0 (0) | 7 | 1016 (1166) | 1787 | 4096 | 16384 |
| 5 | Hash-V-grow-map | C | 1003 | 2048 | 49.0% | 64 | 16 | 1045 / 1003 | 1.44 | 21 | 0 (0) | 7 | 1016 (1197) | 1816 | 4096 | 16384 |
| 5 | Hash-V-grow-map | D | 1015 | 2048 | 49.6% | 64 | 16 | 1033 / 1015 | 1.38 | 12 | 0 (0) | 7 | 1016 (1164) | 1789 | 4096 | 16384 |
| 5 | Hash-V-grow-map | P | 1089 | 4096 | 26.6% | 64 | 16 | 959 / 1089 | 1.44 | 13 | 1 (16) | 8 | 2040 (2369) | 3623 | 4096 | 32768 |
| 5 | Hash-V-grow-map | Q | 1100 | 4096 | 26.9% | 64 | 16 | 948 / 1100 | 1.43 | 19 | 1 (16) | 8 | 2040 (2387) | 3600 | 4096 | 32768 |
| 5 | Hash-V-grow-map | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.42 | 14 | 1 (16) | 8 | 2040 (2386) | 3575 | 4096 | 32768 |
| 5 | Hash-V-pack | A | 1013 | 4096 | 24.7% | 64 | 16 | 1035 / 1013 | 1.06 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8104 |
| 5 | Hash-V-pack | B | 1011 | 4096 | 24.7% | 64 | 16 | 1037 / 1011 | 1.07 | 5 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8088 |
| 5 | Hash-V-pack | C | 1003 | 4096 | 24.5% | 64 | 16 | 1045 / 1003 | 1.09 | 8 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8024 |
| 5 | Hash-V-pack | D | 1015 | 4096 | 24.8% | 64 | 16 | 1033 / 1015 | 1.05 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8120 |
| 5 | Hash-V-pack | P | 1089 | 4096 | 26.6% | 64 | 16 | 959 / 1089 | 1.10 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8712 |
| 5 | Hash-V-pack | Q | 1100 | 4096 | 26.9% | 64 | 16 | 948 / 1100 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8800 |
| 5 | Hash-V-pack | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.10 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8736 |
| 6 | Hash-V-fixed | A | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.06 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | C | 281 | 4096 | 6.9% | 64 | 16 | 1767 / 281 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.10 | 8 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | Q | 1096 | 4096 | 26.8% | 64 | 16 | 952 / 1096 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | R | 1095 | 4096 | 26.7% | 64 | 16 | 953 / 1095 | 1.09 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-grow-map | A | 284 | 1024 | 27.7% | 64 | 16 | 1764 / 284 | 1.32 | 13 | 0 (0) | 6 | 504 (614) | 1440 | 4096 | 8192 |
| 6 | Hash-V-grow-map | B | 284 | 1024 | 27.7% | 64 | 16 | 1764 / 284 | 1.22 | 8 | 0 (0) | 6 | 504 (576) | 1485 | 4096 | 8192 |
| 6 | Hash-V-grow-map | C | 281 | 1024 | 27.4% | 64 | 16 | 1767 / 281 | 1.19 | 8 | 0 (0) | 6 | 504 (574) | 1417 | 4096 | 8192 |
| 6 | Hash-V-grow-map | D | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.23 | 12 | 0 (0) | 6 | 504 (571) | 1679 | 4096 | 8192 |
| 6 | Hash-V-grow-map | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.42 | 14 | 1 (16) | 8 | 2040 (2382) | 3583 | 4096 | 32768 |
| 6 | Hash-V-grow-map | Q | 1096 | 4096 | 26.8% | 64 | 16 | 952 / 1096 | 1.44 | 12 | 1 (16) | 8 | 2040 (2347) | 3598 | 4096 | 32768 |
| 6 | Hash-V-grow-map | R | 1095 | 4096 | 26.7% | 64 | 16 | 953 / 1095 | 1.44 | 16 | 1 (16) | 8 | 2040 (2390) | 3581 | 4096 | 32768 |
| 6 | Hash-V-pack | A | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.06 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2272 |
| 6 | Hash-V-pack | B | 284 | 4096 | 6.9% | 64 | 16 | 1764 / 284 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2272 |
| 6 | Hash-V-pack | C | 281 | 4096 | 6.9% | 64 | 16 | 1767 / 281 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2248 |
| 6 | Hash-V-pack | D | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.10 | 8 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8776 |
| 6 | Hash-V-pack | Q | 1096 | 4096 | 26.8% | 64 | 16 | 952 / 1096 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8768 |
| 6 | Hash-V-pack | R | 1095 | 4096 | 26.7% | 64 | 16 | 953 / 1095 | 1.09 | 7 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8760 |
| 7 | Hash-V-fixed | A | 82 | 4096 | 2.0% | 64 | 16 | 1966 / 82 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | B | 88 | 4096 | 2.1% | 64 | 16 | 1960 / 88 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | C | 86 | 4096 | 2.1% | 64 | 16 | 1962 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | D | 88 | 4096 | 2.1% | 64 | 16 | 1960 / 88 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | Q | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.11 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-grow-map | A | 82 | 256 | 32.0% | 64 | 16 | 1966 / 82 | 1.21 | 5 | 0 (0) | 4 | 120 (143) | 248 | 4096 | 2048 |
| 7 | Hash-V-grow-map | B | 88 | 256 | 34.4% | 64 | 16 | 1960 / 88 | 1.25 | 6 | 0 (0) | 4 | 120 (138) | 188 | 4096 | 2048 |
| 7 | Hash-V-grow-map | C | 86 | 256 | 33.6% | 64 | 16 | 1962 / 86 | 1.14 | 9 | 0 (0) | 4 | 120 (131) | 206 | 4096 | 2048 |
| 7 | Hash-V-grow-map | D | 88 | 256 | 34.4% | 64 | 16 | 1960 / 88 | 1.23 | 8 | 0 (0) | 4 | 120 (140) | 207 | 4096 | 2048 |
| 7 | Hash-V-grow-map | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.42 | 17 | 1 (16) | 8 | 2040 (2355) | 3584 | 4096 | 32768 |
| 7 | Hash-V-grow-map | Q | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.41 | 13 | 1 (16) | 8 | 2040 (2371) | 3605 | 4096 | 32768 |
| 7 | Hash-V-grow-map | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.45 | 13 | 1 (16) | 8 | 2040 (2412) | 3628 | 4096 | 32768 |
| 7 | Hash-V-pack | A | 82 | 4096 | 2.0% | 64 | 8 | 1966 / 82 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 656 |
| 7 | Hash-V-pack | B | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 704 |
| 7 | Hash-V-pack | C | 86 | 4096 | 2.1% | 64 | 8 | 1962 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 688 |
| 7 | Hash-V-pack | D | 88 | 4096 | 2.1% | 64 | 8 | 1960 / 88 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 704 |
| 7 | Hash-V-pack | P | 1097 | 4096 | 26.8% | 64 | 16 | 951 / 1097 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8776 |
| 7 | Hash-V-pack | Q | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8752 |
| 7 | Hash-V-pack | R | 1092 | 4096 | 26.7% | 64 | 16 | 956 / 1092 | 1.11 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8736 |
| 8 | Hash-V-fixed | A | 26 | 4096 | 0.6% | 64 | 16 | 2022 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | B | 28 | 4096 | 0.7% | 64 | 16 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | C | 28 | 4096 | 0.7% | 64 | 16 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | D | 29 | 4096 | 0.7% | 64 | 16 | 2019 / 29 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | P | 1093 | 4096 | 26.7% | 64 | 16 | 955 / 1093 | 1.11 | 8 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | Q | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.09 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | R | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.14 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-grow-map | A | 26 | 64 | 40.6% | 64 | 16 | 2022 / 26 | 1.37 | 4 | 0 (0) | 2 | 24 (28) | 27 | 4096 | 512 |
| 8 | Hash-V-grow-map | B | 28 | 64 | 43.8% | 64 | 16 | 2020 / 28 | 1.34 | 4 | 0 (0) | 2 | 24 (26) | 24 | 4096 | 512 |
| 8 | Hash-V-grow-map | C | 28 | 64 | 43.8% | 64 | 16 | 2020 / 28 | 1.49 | 6 | 0 (0) | 2 | 24 (30) | 27 | 4096 | 512 |
| 8 | Hash-V-grow-map | D | 29 | 64 | 45.3% | 64 | 16 | 2019 / 29 | 1.22 | 4 | 0 (0) | 2 | 24 (24) | 45 | 4096 | 512 |
| 8 | Hash-V-grow-map | P | 1093 | 4096 | 26.7% | 64 | 16 | 955 / 1093 | 1.47 | 21 | 1 (16) | 8 | 2040 (2381) | 3631 | 4096 | 32768 |
| 8 | Hash-V-grow-map | Q | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.39 | 16 | 1 (16) | 8 | 2040 (2325) | 3623 | 4096 | 32768 |
| 8 | Hash-V-grow-map | R | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.43 | 13 | 1 (16) | 8 | 2040 (2389) | 3655 | 4096 | 32768 |
| 8 | Hash-V-pack | A | 26 | 4096 | 0.6% | 64 | 8 | 2022 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 208 |
| 8 | Hash-V-pack | B | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 224 |
| 8 | Hash-V-pack | C | 28 | 4096 | 0.7% | 64 | 8 | 2020 / 28 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 224 |
| 8 | Hash-V-pack | D | 29 | 4096 | 0.7% | 64 | 8 | 2019 / 29 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 232 |
| 8 | Hash-V-pack | P | 1093 | 4096 | 26.7% | 64 | 16 | 955 / 1093 | 1.11 | 8 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8744 |
| 8 | Hash-V-pack | Q | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.09 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8696 |
| 8 | Hash-V-pack | R | 1087 | 4096 | 26.5% | 64 | 16 | 961 / 1087 | 1.14 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8696 |
| 9 | Hash-V-fixed | A | 3526 | 16384 | 21.5% | 64 | 16 | 4666 / 3526 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | B | 3536 | 16384 | 21.6% | 64 | 16 | 4656 / 3536 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | C | 3527 | 16384 | 21.5% | 64 | 16 | 4665 / 3527 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | D | 3558 | 16384 | 21.7% | 64 | 16 | 4634 / 3558 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | P | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | Q | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.08 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | R | 4182 | 16384 | 25.5% | 64 | 16 | 4010 / 4182 | 1.08 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-grow-map | A | 3526 | 8192 | 43.0% | 64 | 16 | 4666 / 3526 | 1.26 | 15 | 0 (0) | 9 | 4088 (4563) | 8339 | 16384 | 65536 |
| 9 | Hash-V-grow-map | B | 3536 | 8192 | 43.2% | 64 | 16 | 4656 / 3536 | 1.25 | 11 | 0 (0) | 9 | 4088 (4523) | 8324 | 16384 | 65536 |
| 9 | Hash-V-grow-map | C | 3527 | 8192 | 43.1% | 64 | 16 | 4665 / 3527 | 1.27 | 13 | 0 (0) | 9 | 4088 (4609) | 8303 | 16384 | 65536 |
| 9 | Hash-V-grow-map | D | 3558 | 8192 | 43.4% | 64 | 16 | 4634 / 3558 | 1.26 | 15 | 0 (0) | 9 | 4088 (4575) | 8319 | 16384 | 65536 |
| 9 | Hash-V-grow-map | P | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.43 | 19 | 1 (16) | 10 | 8184 (9655) | 15570 | 16384 | 131072 |
| 9 | Hash-V-grow-map | Q | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.41 | 18 | 1 (16) | 10 | 8184 (9484) | 15587 | 16384 | 131072 |
| 9 | Hash-V-grow-map | R | 4182 | 16384 | 25.5% | 64 | 16 | 4010 / 4182 | 1.41 | 21 | 1 (16) | 10 | 8184 (9495) | 15614 | 16384 | 131072 |
| 9 | Hash-V-pack | A | 3526 | 16384 | 21.5% | 64 | 16 | 4666 / 3526 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28208 |
| 9 | Hash-V-pack | B | 3536 | 16384 | 21.6% | 64 | 16 | 4656 / 3536 | 1.02 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28288 |
| 9 | Hash-V-pack | C | 3527 | 16384 | 21.5% | 64 | 16 | 4665 / 3527 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28216 |
| 9 | Hash-V-pack | D | 3558 | 16384 | 21.7% | 64 | 16 | 4634 / 3558 | 1.03 | 5 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28464 |
| 9 | Hash-V-pack | P | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33504 |
| 9 | Hash-V-pack | Q | 4185 | 16384 | 25.5% | 64 | 16 | 4007 / 4185 | 1.08 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33480 |
| 9 | Hash-V-pack | R | 4182 | 16384 | 25.5% | 64 | 16 | 4010 / 4182 | 1.08 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33456 |
| 10 | Hash-V-fixed | A | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | C | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | D | 1023 | 16384 | 6.2% | 64 | 16 | 7169 / 1023 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | Q | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | R | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-grow-map | A | 1012 | 2048 | 49.4% | 64 | 16 | 7180 / 1012 | 1.22 | 12 | 0 (0) | 7 | 1016 (1155) | 2139 | 16384 | 16384 |
| 10 | Hash-V-grow-map | B | 1015 | 2048 | 49.6% | 64 | 16 | 7177 / 1015 | 1.20 | 11 | 0 (0) | 7 | 1016 (1189) | 2186 | 16384 | 16384 |
| 10 | Hash-V-grow-map | C | 1012 | 2048 | 49.4% | 64 | 16 | 7180 / 1012 | 1.24 | 28 | 0 (0) | 7 | 1016 (1167) | 2156 | 16384 | 16384 |
| 10 | Hash-V-grow-map | D | 1023 | 2048 | 50.0% | 64 | 16 | 7169 / 1023 | 1.23 | 13 | 0 (0) | 7 | 1016 (1158) | 2101 | 16384 | 16384 |
| 10 | Hash-V-grow-map | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.45 | 22 | 1 (16) | 10 | 8184 (9661) | 15524 | 16384 | 131072 |
| 10 | Hash-V-grow-map | Q | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.43 | 19 | 1 (16) | 10 | 8184 (9572) | 15516 | 16384 | 131072 |
| 10 | Hash-V-grow-map | R | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.44 | 26 | 1 (16) | 10 | 8184 (9592) | 15589 | 16384 | 131072 |
| 10 | Hash-V-pack | A | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8096 |
| 10 | Hash-V-pack | B | 1015 | 16384 | 6.2% | 64 | 16 | 7177 / 1015 | 1.01 | 3 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8120 |
| 10 | Hash-V-pack | C | 1012 | 16384 | 6.2% | 64 | 16 | 7180 / 1012 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8096 |
| 10 | Hash-V-pack | D | 1023 | 16384 | 6.2% | 64 | 16 | 7169 / 1023 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8184 |
| 10 | Hash-V-pack | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 8 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33520 |
| 10 | Hash-V-pack | Q | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33512 |
| 10 | Hash-V-pack | R | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33472 |
| 11 | Hash-V-fixed | A | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | B | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | C | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | D | 280 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | P | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | Q | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.09 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-grow-map | A | 282 | 1024 | 27.5% | 64 | 16 | 7910 / 282 | 1.12 | 13 | 0 (0) | 6 | 504 (563) | 1436 | 16384 | 8192 |
| 11 | Hash-V-grow-map | B | 282 | 1024 | 27.5% | 64 | 16 | 7910 / 282 | 1.11 | 8 | 0 (0) | 6 | 504 (553) | 1568 | 16384 | 8192 |
| 11 | Hash-V-grow-map | C | 282 | 1024 | 27.5% | 64 | 16 | 7910 / 282 | 1.13 | 11 | 0 (0) | 6 | 504 (571) | 1582 | 16384 | 8192 |
| 11 | Hash-V-grow-map | D | 280 | 1024 | 27.3% | 64 | 16 | 7912 / 280 | 1.13 | 6 | 0 (0) | 6 | 504 (562) | 1695 | 16384 | 8192 |
| 11 | Hash-V-grow-map | P | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.42 | 21 | 1 (16) | 10 | 8184 (9630) | 15591 | 16384 | 131072 |
| 11 | Hash-V-grow-map | Q | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.43 | 25 | 1 (16) | 10 | 8184 (9516) | 15624 | 16384 | 131072 |
| 11 | Hash-V-grow-map | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.40 | 15 | 1 (16) | 10 | 8184 (9541) | 15586 | 16384 | 131072 |
| 11 | Hash-V-pack | A | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2256 |
| 11 | Hash-V-pack | B | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2256 |
| 11 | Hash-V-pack | C | 282 | 16384 | 1.7% | 64 | 16 | 7910 / 282 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2256 |
| 11 | Hash-V-pack | D | 280 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2240 |
| 11 | Hash-V-pack | P | 4187 | 16384 | 25.6% | 64 | 16 | 4005 / 4187 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33496 |
| 11 | Hash-V-pack | Q | 4184 | 16384 | 25.5% | 64 | 16 | 4008 / 4184 | 1.09 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33472 |
| 11 | Hash-V-pack | R | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33512 |
| 12 | Hash-V-fixed | A | 89 | 16384 | 0.5% | 64 | 16 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | B | 90 | 16384 | 0.5% | 64 | 16 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | C | 86 | 16384 | 0.5% | 64 | 16 | 8106 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | D | 90 | 16384 | 0.5% | 64 | 16 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | Q | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 11 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | R | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-grow-map | A | 89 | 256 | 34.8% | 64 | 16 | 8103 / 89 | 1.19 | 8 | 0 (0) | 4 | 120 (140) | 242 | 16384 | 2048 |
| 12 | Hash-V-grow-map | B | 90 | 256 | 35.2% | 64 | 16 | 8102 / 90 | 1.36 | 11 | 0 (0) | 4 | 120 (150) | 178 | 16384 | 2048 |
| 12 | Hash-V-grow-map | C | 86 | 256 | 33.6% | 64 | 16 | 8106 / 86 | 1.11 | 8 | 0 (0) | 4 | 120 (127) | 179 | 16384 | 2048 |
| 12 | Hash-V-grow-map | D | 90 | 256 | 35.2% | 64 | 16 | 8102 / 90 | 1.12 | 6 | 0 (0) | 4 | 120 (131) | 240 | 16384 | 2048 |
| 12 | Hash-V-grow-map | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.42 | 20 | 1 (16) | 10 | 8184 (9524) | 15585 | 16384 | 131072 |
| 12 | Hash-V-grow-map | Q | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.47 | 37 | 1 (16) | 10 | 8184 (9556) | 15584 | 16384 | 131072 |
| 12 | Hash-V-grow-map | R | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.45 | 20 | 1 (16) | 10 | 8184 (9630) | 15614 | 16384 | 131072 |
| 12 | Hash-V-pack | A | 89 | 16384 | 0.5% | 64 | 8 | 8103 / 89 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 712 |
| 12 | Hash-V-pack | B | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 720 |
| 12 | Hash-V-pack | C | 86 | 16384 | 0.5% | 64 | 8 | 8106 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 688 |
| 12 | Hash-V-pack | D | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 720 |
| 12 | Hash-V-pack | P | 4190 | 16384 | 25.6% | 64 | 16 | 4002 / 4190 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33520 |
| 12 | Hash-V-pack | Q | 4186 | 16384 | 25.5% | 64 | 16 | 4006 / 4186 | 1.10 | 11 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33488 |
| 12 | Hash-V-pack | R | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33504 |
| 13 | Hash-V-fixed | A | 10220 | 65536 | 15.6% | 64 | 32 | 22548 / 10220 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | B | 10122 | 65536 | 15.4% | 64 | 32 | 22646 / 10122 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | C | 10068 | 65536 | 15.4% | 64 | 32 | 22700 / 10068 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | D | 10131 | 65536 | 15.5% | 64 | 32 | 22637 / 10131 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | P | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.09 | 13 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | Q | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.08 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | R | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.09 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-grow-map | A | 10220 | 32768 | 31.2% | 64 | 32 | 22548 / 10220 | 1.13 | 17 | 0 (0) | 11 | 16376 (17324) | 41467 | 131072 | 262144 |
| 13 | Hash-V-grow-map | B | 10122 | 32768 | 30.9% | 64 | 32 | 22646 / 10122 | 1.14 | 19 | 0 (0) | 11 | 16376 (17318) | 42033 | 131072 | 262144 |
| 13 | Hash-V-grow-map | C | 10068 | 32768 | 30.7% | 64 | 32 | 22700 / 10068 | 1.14 | 15 | 0 (0) | 11 | 16376 (17384) | 42005 | 131072 | 262144 |
| 13 | Hash-V-grow-map | D | 10131 | 32768 | 30.9% | 64 | 32 | 22637 / 10131 | 1.14 | 17 | 0 (0) | 11 | 16376 (17355) | 41825 | 131072 | 262144 |
| 13 | Hash-V-grow-map | P | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.43 | 27 | 1 (16) | 12 | 32760 (38313) | 64338 | 131072 | 524288 |
| 13 | Hash-V-grow-map | Q | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.40 | 21 | 1 (16) | 12 | 32760 (37920) | 64299 | 131072 | 524288 |
| 13 | Hash-V-grow-map | R | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.42 | 20 | 1 (16) | 12 | 32760 (38236) | 64312 | 131072 | 524288 |
| 13 | Hash-V-pack | A | 10220 | 65536 | 15.6% | 64 | 16 | 22548 / 10220 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81760 |
| 13 | Hash-V-pack | B | 10122 | 65536 | 15.4% | 64 | 16 | 22646 / 10122 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80976 |
| 13 | Hash-V-pack | C | 10068 | 65536 | 15.4% | 64 | 16 | 22700 / 10068 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80544 |
| 13 | Hash-V-pack | D | 10131 | 65536 | 15.5% | 64 | 16 | 22637 / 10131 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 81048 |
| 13 | Hash-V-pack | P | 16493 | 65536 | 25.2% | 64 | 16 | 16275 / 16493 | 1.09 | 13 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131944 |
| 13 | Hash-V-pack | Q | 16494 | 65536 | 25.2% | 64 | 16 | 16274 / 16494 | 1.08 | 7 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131952 |
| 13 | Hash-V-pack | R | 16495 | 65536 | 25.2% | 64 | 16 | 16273 / 16495 | 1.09 | 8 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131960 |
| 14 | Hash-V-fixed | A | 3561 | 65536 | 5.4% | 64 | 32 | 29207 / 3561 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | B | 3541 | 65536 | 5.4% | 64 | 32 | 29227 / 3541 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | C | 3512 | 65536 | 5.4% | 64 | 32 | 29256 / 3512 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | D | 3558 | 65536 | 5.4% | 64 | 32 | 29210 / 3558 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | P | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | Q | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.09 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | R | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-grow-map | A | 3561 | 8192 | 43.5% | 64 | 32 | 29207 / 3561 | 1.15 | 17 | 0 (0) | 9 | 4088 (4592) | 10277 | 131072 | 65536 |
| 14 | Hash-V-grow-map | B | 3541 | 8192 | 43.2% | 64 | 32 | 29227 / 3541 | 1.14 | 21 | 0 (0) | 9 | 4088 (4622) | 10531 | 131072 | 65536 |
| 14 | Hash-V-grow-map | C | 3512 | 8192 | 42.9% | 64 | 32 | 29256 / 3512 | 1.14 | 14 | 0 (0) | 9 | 4088 (4558) | 10257 | 131072 | 65536 |
| 14 | Hash-V-grow-map | D | 3558 | 8192 | 43.4% | 64 | 32 | 29210 / 3558 | 1.14 | 13 | 0 (0) | 9 | 4088 (4577) | 10177 | 131072 | 65536 |
| 14 | Hash-V-grow-map | P | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.41 | 27 | 1 (16) | 12 | 32760 (38182) | 64284 | 131072 | 524288 |
| 14 | Hash-V-grow-map | Q | 16496 | 65536 | 25.2% | 64 | 32 | 16272 / 16496 | 1.43 | 31 | 1 (16) | 12 | 32760 (38196) | 64329 | 131072 | 524288 |
| 14 | Hash-V-grow-map | R | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.42 | 22 | 1 (16) | 12 | 32760 (38551) | 64335 | 131072 | 524288 |
| 14 | Hash-V-pack | A | 3561 | 65536 | 5.4% | 64 | 16 | 29207 / 3561 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28488 |
| 14 | Hash-V-pack | B | 3541 | 65536 | 5.4% | 64 | 16 | 29227 / 3541 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28328 |
| 14 | Hash-V-pack | C | 3512 | 65536 | 5.4% | 64 | 16 | 29256 / 3512 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28096 |
| 14 | Hash-V-pack | D | 3558 | 65536 | 5.4% | 64 | 16 | 29210 / 3558 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28464 |
| 14 | Hash-V-pack | P | 16496 | 65536 | 25.2% | 64 | 16 | 16272 / 16496 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131968 |
| 14 | Hash-V-pack | Q | 16496 | 65536 | 25.2% | 64 | 16 | 16272 / 16496 | 1.09 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131968 |
| 14 | Hash-V-pack | R | 16493 | 65536 | 25.2% | 64 | 16 | 16275 / 16493 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131944 |
| 15 | Hash-V-fixed | A | 1021 | 65536 | 1.6% | 64 | 32 | 31747 / 1021 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | B | 1011 | 65536 | 1.5% | 64 | 32 | 31757 / 1011 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | C | 1012 | 65536 | 1.5% | 64 | 32 | 31756 / 1012 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | D | 1000 | 65536 | 1.5% | 64 | 32 | 31768 / 1000 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | P | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | Q | 16498 | 65536 | 25.2% | 64 | 32 | 16270 / 16498 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | R | 16497 | 65536 | 25.2% | 64 | 32 | 16271 / 16497 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-grow-map | A | 1021 | 2048 | 49.9% | 64 | 32 | 31747 / 1021 | 1.22 | 13 | 0 (0) | 7 | 1016 (1154) | 2284 | 131072 | 16384 |
| 15 | Hash-V-grow-map | B | 1011 | 2048 | 49.4% | 64 | 32 | 31757 / 1011 | 1.20 | 14 | 0 (0) | 7 | 1016 (1162) | 2255 | 131072 | 16384 |
| 15 | Hash-V-grow-map | C | 1012 | 2048 | 49.4% | 64 | 32 | 31756 / 1012 | 1.24 | 13 | 0 (0) | 7 | 1016 (1153) | 2225 | 131072 | 16384 |
| 15 | Hash-V-grow-map | D | 1000 | 2048 | 48.8% | 64 | 32 | 31768 / 1000 | 1.22 | 19 | 0 (0) | 7 | 1016 (1149) | 2279 | 131072 | 16384 |
| 15 | Hash-V-grow-map | P | 16493 | 65536 | 25.2% | 64 | 32 | 16275 / 16493 | 1.43 | 22 | 1 (16) | 12 | 32760 (38169) | 64320 | 131072 | 524288 |
| 15 | Hash-V-grow-map | Q | 16498 | 65536 | 25.2% | 64 | 32 | 16270 / 16498 | 1.41 | 24 | 1 (16) | 12 | 32760 (38100) | 64339 | 131072 | 524288 |
| 15 | Hash-V-grow-map | R | 16497 | 65536 | 25.2% | 64 | 32 | 16271 / 16497 | 1.42 | 31 | 1 (16) | 12 | 32760 (38099) | 64355 | 131072 | 524288 |
| 15 | Hash-V-pack | A | 1021 | 65536 | 1.6% | 64 | 16 | 31747 / 1021 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8168 |
| 15 | Hash-V-pack | B | 1011 | 65536 | 1.5% | 64 | 16 | 31757 / 1011 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8088 |
| 15 | Hash-V-pack | C | 1012 | 65536 | 1.5% | 64 | 16 | 31756 / 1012 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8096 |
| 15 | Hash-V-pack | D | 1000 | 65536 | 1.5% | 64 | 16 | 31768 / 1000 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8000 |
| 15 | Hash-V-pack | P | 16493 | 65536 | 25.2% | 64 | 16 | 16275 / 16493 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131944 |
| 15 | Hash-V-pack | Q | 16498 | 65536 | 25.2% | 64 | 16 | 16270 / 16498 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131984 |
| 15 | Hash-V-pack | R | 16497 | 65536 | 25.2% | 64 | 16 | 16271 / 16497 | 1.08 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131976 |
| 16 | Hash-V-fixed | A | 284 | 65536 | 0.4% | 64 | 32 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | B | 283 | 65536 | 0.4% | 64 | 32 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | C | 282 | 65536 | 0.4% | 64 | 32 | 32486 / 282 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | D | 279 | 65536 | 0.4% | 64 | 32 | 32489 / 279 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | P | 16497 | 65536 | 25.2% | 64 | 32 | 16271 / 16497 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | Q | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | R | 16490 | 65536 | 25.2% | 64 | 32 | 16278 / 16490 | 1.08 | 14 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-grow-map | A | 284 | 1024 | 27.7% | 64 | 32 | 32484 / 284 | 1.08 | 11 | 0 (0) | 6 | 504 (557) | 1588 | 131072 | 8192 |
| 16 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 32 | 32485 / 283 | 1.14 | 8 | 0 (0) | 6 | 504 (574) | 1675 | 131072 | 8192 |
| 16 | Hash-V-grow-map | C | 282 | 1024 | 27.5% | 64 | 32 | 32486 / 282 | 1.13 | 11 | 0 (0) | 6 | 504 (594) | 1580 | 131072 | 8192 |
| 16 | Hash-V-grow-map | D | 279 | 1024 | 27.2% | 64 | 32 | 32489 / 279 | 1.12 | 16 | 0 (0) | 6 | 504 (580) | 1564 | 131072 | 8192 |
| 16 | Hash-V-grow-map | P | 16497 | 65536 | 25.2% | 64 | 32 | 16271 / 16497 | 1.42 | 25 | 1 (16) | 12 | 32760 (38282) | 64299 | 131072 | 524288 |
| 16 | Hash-V-grow-map | Q | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.42 | 27 | 1 (16) | 12 | 32760 (38135) | 64366 | 131072 | 524288 |
| 16 | Hash-V-grow-map | R | 16490 | 65536 | 25.2% | 64 | 32 | 16278 / 16490 | 1.41 | 20 | 1 (16) | 12 | 32760 (38051) | 64373 | 131072 | 524288 |
| 16 | Hash-V-pack | A | 284 | 65536 | 0.4% | 64 | 16 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2272 |
| 16 | Hash-V-pack | B | 283 | 65536 | 0.4% | 64 | 16 | 32485 / 283 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2264 |
| 16 | Hash-V-pack | C | 282 | 65536 | 0.4% | 64 | 16 | 32486 / 282 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2256 |
| 16 | Hash-V-pack | D | 279 | 65536 | 0.4% | 64 | 16 | 32489 / 279 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2232 |
| 16 | Hash-V-pack | P | 16497 | 65536 | 25.2% | 64 | 16 | 16271 / 16497 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131976 |
| 16 | Hash-V-pack | Q | 16495 | 65536 | 25.2% | 64 | 16 | 16273 / 16495 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131960 |
| 16 | Hash-V-pack | R | 16490 | 65536 | 25.2% | 64 | 16 | 16278 / 16490 | 1.08 | 14 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131920 |

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
| 3 | A | 25 | 16 | 8 | 200 | 512 | 2048 |
| 3 | B | 27 | 16 | 8 | 216 | 512 | 2048 |
| 3 | C | 28 | 16 | 8 | 224 | 512 | 2048 |
| 3 | D | 29 | 16 | 8 | 232 | 512 | 2048 |
| 3 | P | 307 | 16 | 16 | 2456 | 512 | 2048 |
| 3 | Q | 303 | 16 | 16 | 2424 | 512 | 2048 |
| 3 | R | 299 | 16 | 16 | 2392 | 512 | 2048 |
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
| 7 | A | 82 | 16 | 8 | 656 | 2048 | 8192 |
| 7 | B | 88 | 16 | 8 | 704 | 2048 | 8192 |
| 7 | C | 86 | 16 | 8 | 688 | 2048 | 8192 |
| 7 | D | 88 | 16 | 8 | 704 | 2048 | 8192 |
| 7 | P | 1097 | 16 | 16 | 8776 | 2048 | 8192 |
| 7 | Q | 1094 | 16 | 16 | 8752 | 2048 | 8192 |
| 7 | R | 1092 | 16 | 16 | 8736 | 2048 | 8192 |
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
| 11 | A | 282 | 16 | 16 | 2256 | 8192 | 32768 |
| 11 | B | 282 | 16 | 16 | 2256 | 8192 | 32768 |
| 11 | C | 282 | 16 | 16 | 2256 | 8192 | 32768 |
| 11 | D | 280 | 16 | 16 | 2240 | 8192 | 32768 |
| 11 | P | 4187 | 16 | 16 | 33496 | 8192 | 32768 |
| 11 | Q | 4184 | 16 | 16 | 33472 | 8192 | 32768 |
| 11 | R | 4189 | 16 | 16 | 33512 | 8192 | 32768 |
| 12 | A | 89 | 16 | 8 | 712 | 8192 | 32768 |
| 12 | B | 90 | 16 | 8 | 720 | 8192 | 32768 |
| 12 | C | 86 | 16 | 8 | 688 | 8192 | 32768 |
| 12 | D | 90 | 16 | 8 | 720 | 8192 | 32768 |
| 12 | P | 4190 | 16 | 16 | 33520 | 8192 | 32768 |
| 12 | Q | 4186 | 16 | 16 | 33488 | 8192 | 32768 |
| 12 | R | 4188 | 16 | 16 | 33504 | 8192 | 32768 |
| 13 | A | 10220 | 32 | 16 | 81760 | 32768 | 262144 |
| 13 | B | 10122 | 32 | 16 | 80976 | 32768 | 262144 |
| 13 | C | 10068 | 32 | 16 | 80544 | 32768 | 262144 |
| 13 | D | 10131 | 32 | 16 | 81048 | 32768 | 262144 |
| 13 | P | 16493 | 32 | 16 | 131944 | 32768 | 262144 |
| 13 | Q | 16494 | 32 | 16 | 131952 | 32768 | 262144 |
| 13 | R | 16495 | 32 | 16 | 131960 | 32768 | 262144 |
| 14 | A | 3561 | 32 | 16 | 28488 | 32768 | 262144 |
| 14 | B | 3541 | 32 | 16 | 28328 | 32768 | 262144 |
| 14 | C | 3512 | 32 | 16 | 28096 | 32768 | 262144 |
| 14 | D | 3558 | 32 | 16 | 28464 | 32768 | 262144 |
| 14 | P | 16496 | 32 | 16 | 131968 | 32768 | 262144 |
| 14 | Q | 16496 | 32 | 16 | 131968 | 32768 | 262144 |
| 14 | R | 16493 | 32 | 16 | 131944 | 32768 | 262144 |
| 15 | A | 1021 | 32 | 16 | 8168 | 32768 | 262144 |
| 15 | B | 1011 | 32 | 16 | 8088 | 32768 | 262144 |
| 15 | C | 1012 | 32 | 16 | 8096 | 32768 | 262144 |
| 15 | D | 1000 | 32 | 16 | 8000 | 32768 | 262144 |
| 15 | P | 16493 | 32 | 16 | 131944 | 32768 | 262144 |
| 15 | Q | 16498 | 32 | 16 | 131984 | 32768 | 262144 |
| 15 | R | 16497 | 32 | 16 | 131976 | 32768 | 262144 |
| 16 | A | 284 | 32 | 16 | 2272 | 32768 | 262144 |
| 16 | B | 283 | 32 | 16 | 2264 | 32768 | 262144 |
| 16 | C | 282 | 32 | 16 | 2256 | 32768 | 262144 |
| 16 | D | 279 | 32 | 16 | 2232 | 32768 | 262144 |
| 16 | P | 16497 | 32 | 16 | 131976 | 32768 | 262144 |
| 16 | Q | 16495 | 32 | 16 | 131960 | 32768 | 262144 |
| 16 | R | 16490 | 32 | 16 | 131920 | 32768 | 262144 |

## Also reported (rule-m2 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-fixed's stores, from the model; the gates checked it).

| job | n | rho | s | distinct A | distinct B | distinct C | distinct D | distinct P | distinct Q | distinct R | W_A | W_B | W_C | W_D | W_P | W_Q | W_R |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 16 | 1 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 256 | 282 | 283 | 284 | 282 | 314 | 303 | 296 |
| 2 | 16 | 4 | 64 | 64 | 64 | 64 | 64 | 256 | 256 | 256 | 84 | 87 | 88 | 88 | 313 | 309 | 302 |
| 3 | 16 | 16 | 16 | 16 | 16 | 16 | 16 | 256 | 256 | 256 | 25 | 27 | 28 | 29 | 307 | 303 | 299 |
| 4 | 16 | 64 | 4 | 4 | 4 | 4 | 4 | 256 | 256 | 256 | 8 | 8 | 8 | 8 | 305 | 295 | 296 |
| 5 | 32 | 1 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1024 | 1013 | 1011 | 1003 | 1015 | 1089 | 1100 | 1092 |
| 6 | 32 | 4 | 256 | 256 | 256 | 256 | 256 | 1024 | 1024 | 1024 | 284 | 284 | 281 | 283 | 1097 | 1096 | 1095 |
| 7 | 32 | 16 | 64 | 64 | 64 | 64 | 64 | 1024 | 1024 | 1024 | 82 | 88 | 86 | 88 | 1097 | 1094 | 1092 |
| 8 | 32 | 64 | 16 | 16 | 16 | 16 | 16 | 1024 | 1024 | 1024 | 26 | 28 | 28 | 29 | 1093 | 1087 | 1087 |
| 9 | 64 | 1 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 4096 | 3526 | 3536 | 3527 | 3558 | 4188 | 4185 | 4182 |
| 10 | 64 | 4 | 1024 | 1024 | 1024 | 1024 | 1024 | 4096 | 4096 | 4096 | 1012 | 1015 | 1012 | 1023 | 4190 | 4189 | 4184 |
| 11 | 64 | 16 | 256 | 256 | 256 | 256 | 256 | 4096 | 4096 | 4096 | 282 | 282 | 282 | 280 | 4187 | 4184 | 4189 |
| 12 | 64 | 64 | 64 | 64 | 64 | 64 | 64 | 4096 | 4096 | 4096 | 89 | 90 | 86 | 90 | 4190 | 4186 | 4188 |
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 16384 | 10220 | 10122 | 10068 | 10131 | 16493 | 16494 | 16495 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 4096 | 4096 | 16384 | 16384 | 16384 | 3561 | 3541 | 3512 | 3558 | 16496 | 16496 | 16493 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 1024 | 1024 | 16384 | 16384 | 16384 | 1021 | 1011 | 1012 | 1000 | 16493 | 16498 | 16497 |
| 16 | 128 | 64 | 256 | 256 | 256 | 256 | 256 | 16384 | 16384 | 16384 | 284 | 283 | 282 | 279 | 16497 | 16495 | 16490 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.2 §15.7)

- job 1, P = A x B: add (4096): zero_x 0.062, int_x 0.003, int_y 0.048, den 0.004, general 0.883; mul (4096): int_both 0.016, den 0.031, cross_cb 0.000, int_x 0.101, int_y 0.109, general 0.743; cross tests 16384 (matched 194, cross reads 16384); P's store W 314, found / new 198 / 314
- job 1, Q = C x D: add (4096): zero_x 0.062, int_both 0.000, int_x 0.002, int_y 0.045, den 0.005, general 0.885; mul (4096): int_both 0.017, den 0.033, num 0.000, int_x 0.084, int_y 0.119, general 0.746; cross tests 16384 (matched 208, cross reads 16384); Q's store W 303, found / new 209 / 303
- job 1, R = P + Q: add (256): den 0.039, general 0.961; cross tests 1024 (matched 10, cross reads 1024); R's store W 296, found / new 216 / 296
- job 2, P = A x B: add (4096): zero_x 0.062, int_x 0.002, int_y 0.040, den 0.002, general 0.893; mul (4096): int_both 0.008, den 0.033, cross_ad 0.001, int_x 0.156, int_y 0.042, general 0.760; cross tests 16384 (matched 174, cross reads 16384); P's store W 313, found / new 199 / 313
- job 2, Q = C x D: add (4096): zero_x 0.062, int_both 0.000, int_x 0.003, int_y 0.024, den 0.004, general 0.907; mul (4096): int_both 0.005, den 0.038, int_x 0.105, int_y 0.038, general 0.814; cross tests 16384 (matched 175, cross reads 16384); Q's store W 309, found / new 203 / 309
- job 2, R = P + Q: add (256): den 0.027, general 0.973; cross tests 1024 (matched 7, cross reads 1024); R's store W 302, found / new 210 / 302
- job 3, P = A x B: add (4096): zero_x 0.062, int_x 0.002, int_y 0.012, den 0.007, general 0.916; mul (4096): one_x 0.043, int_both 0.007, den 0.013, int_x 0.075, int_y 0.068, general 0.794; cross tests 16384 (matched 104, cross reads 16208); P's store W 307, found / new 205 / 307
- job 3, Q = C x D: add (4096): zero_x 0.062, int_both 0.001, int_x 0.004, int_y 0.047, den 0.007, general 0.878; mul (4096): int_both 0.005, den 0.016, int_x 0.054, int_y 0.070, general 0.855; cross tests 16384 (matched 86, cross reads 16384); Q's store W 303, found / new 209 / 303
- job 3, R = P + Q: add (256): den 0.066, general 0.934; cross tests 1024 (matched 17, cross reads 1024); R's store W 299, found / new 213 / 299
- job 4, P = A x B: add (4096): zero_x 0.062, den 0.012, general 0.925; mul (4096): den 0.052, general 0.948; cross tests 16384 (matched 211, cross reads 16384); P's store W 305, found / new 207 / 305
- job 4, Q = C x D: add (4096): zero_x 0.062, den 0.004, general 0.933; mul (4096): general 1.000; cross tests 16384 (matched 0, cross reads 16384); Q's store W 295, found / new 217 / 295
- job 4, R = P + Q: add (256): den 0.031, general 0.969; cross tests 1024 (matched 8, cross reads 1024); R's store W 296, found / new 216 / 296
- job 5, P = A x B: add (32768): zero_x 0.031, int_x 0.000, int_y 0.040, den 0.003, general 0.926; mul (32768): int_both 0.011, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.091, int_y 0.096, general 0.767; cross tests 131072 (matched 1529, cross reads 131072); P's store W 1089, found / new 959 / 1089
- job 5, Q = C x D: add (32768): zero_x 0.031, int_x 0.000, int_y 0.047, den 0.002, general 0.919; mul (32768): one_y 0.001, int_both 0.015, den 0.034, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.115, int_y 0.103, general 0.732; cross tests 131072 (matched 1621, cross reads 131040); Q's store W 1100, found / new 948 / 1100
- job 5, R = P + Q: add (1024): den 0.057, general 0.943; cross tests 4096 (matched 58, cross reads 4096); R's store W 1092, found / new 956 / 1092
- job 6, P = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.003, int_y 0.044, den 0.002, general 0.920; mul (32768): int_both 0.011, den 0.035, num 0.000, int_x 0.087, int_y 0.094, general 0.772; cross tests 131072 (matched 1522, cross reads 131072); P's store W 1097, found / new 951 / 1097
- job 6, Q = C x D: add (32768): zero_x 0.031, int_x 0.002, int_y 0.052, den 0.003, general 0.912; mul (32768): int_both 0.016, den 0.032, int_x 0.120, int_y 0.112, general 0.719; cross tests 131072 (matched 1584, cross reads 131072); Q's store W 1096, found / new 952 / 1096
- job 6, R = P + Q: add (1024): den 0.077, general 0.923; cross tests 4096 (matched 79, cross reads 4096); R's store W 1095, found / new 953 / 1095
- job 7, P = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.002, int_y 0.076, den 0.003, general 0.887; mul (32768): int_both 0.023, den 0.026, cross_ad 0.001, int_x 0.114, int_y 0.143, general 0.693; cross tests 131072 (matched 1641, cross reads 131072); P's store W 1097, found / new 951 / 1097
- job 7, Q = C x D: add (32768): zero_x 0.031, int_both 0.000, int_x 0.001, int_y 0.033, den 0.003, general 0.931; mul (32768): int_both 0.007, den 0.030, int_x 0.102, int_y 0.059, general 0.801; cross tests 131072 (matched 1227, cross reads 131072); Q's store W 1094, found / new 954 / 1094
- job 7, R = P + Q: add (1024): den 0.063, general 0.937; cross tests 4096 (matched 65, cross reads 4096); R's store W 1092, found / new 956 / 1092
- job 8, P = A x B: add (32768): zero_x 0.031, int_both 0.000, int_x 0.004, int_y 0.070, den 0.003, num 0.000, general 0.891; mul (32768): int_both 0.028, den 0.032, int_x 0.216, int_y 0.090, general 0.634; cross tests 131072 (matched 1959, cross reads 131072); P's store W 1093, found / new 955 / 1093
- job 8, Q = C x D: add (32768): zero_x 0.031, int_x 0.002, int_y 0.026, den 0.002, general 0.939; mul (32768): den 0.036, int_x 0.140, general 0.825; cross tests 131072 (matched 1165, cross reads 131072); Q's store W 1087, found / new 961 / 1087
- job 8, R = P + Q: add (1024): den 0.073, general 0.927; cross tests 4096 (matched 75, cross reads 4096); R's store W 1087, found / new 961 / 1087
- job 9, P = A x B: add (262144): zero_x 0.016, zero_y 0.000, int_both 0.000, int_x 0.001, int_y 0.040, den 0.002, general 0.941; mul (262144): zero 0.000, int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.100, int_y 0.093, general 0.760; cross tests 1048576 (matched 12233, cross reads 1048576); P's store W 4188, found / new 4004 / 4188
- job 9, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.046, den 0.002, general 0.936; mul (262144): int_both 0.014, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.107, int_y 0.098, general 0.746; cross tests 1048576 (matched 12802, cross reads 1048576); Q's store W 4185, found / new 4007 / 4185
- job 9, R = P + Q: add (4096): den 0.085, general 0.915; cross tests 16384 (matched 350, cross reads 16384); R's store W 4182, found / new 4010 / 4182
- job 10, P = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.038, den 0.001, general 0.945; mul (262144): int_both 0.011, den 0.034, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.086, int_y 0.104, general 0.763; cross tests 1048576 (matched 11991, cross reads 1048576); P's store W 4190, found / new 4002 / 4190
- job 10, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.001, int_y 0.043, den 0.002, general 0.939; mul (262144): int_both 0.013, den 0.036, num 0.000, cross_ad 0.000, int_x 0.120, int_y 0.084, general 0.747; cross tests 1048576 (matched 12798, cross reads 1048576); Q's store W 4189, found / new 4003 / 4189
- job 10, R = P + Q: add (4096): den 0.090, general 0.910; cross tests 16384 (matched 367, cross reads 16384); R's store W 4184, found / new 4008 / 4184
- job 11, P = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.048, den 0.002, general 0.934; mul (262144): int_both 0.016, den 0.033, num 0.000, cross_ad 0.000, cross_cb 0.001, int_x 0.113, int_y 0.104, general 0.733; cross tests 1048576 (matched 13086, cross reads 1048576); P's store W 4187, found / new 4005 / 4187
- job 11, Q = C x D: add (262144): zero_x 0.016, zero_y 0.003, int_both 0.000, int_x 0.001, int_y 0.040, den 0.002, general 0.938; mul (262144): zero 0.003, int_both 0.008, den 0.035, num 0.000, int_x 0.070, int_y 0.096, general 0.788; cross tests 1048576 (matched 11287, cross reads 1048576); Q's store W 4184, found / new 4008 / 4184
- job 11, R = P + Q: add (4096): den 0.087, general 0.913; cross tests 16384 (matched 358, cross reads 16384); R's store W 4189, found / new 4003 / 4189
- job 12, P = A x B: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.027, den 0.002, general 0.955; mul (262144): int_both 0.005, den 0.032, num 0.000, int_x 0.038, int_y 0.103, general 0.822; cross tests 1048576 (matched 9619, cross reads 1048576); P's store W 4190, found / new 4002 / 4190
- job 12, Q = C x D: add (262144): zero_x 0.016, int_both 0.000, int_x 0.000, int_y 0.022, den 0.002, general 0.960; mul (262144): int_both 0.001, den 0.036, int_x 0.014, int_y 0.096, general 0.853; cross tests 1048576 (matched 9861, cross reads 1048576); Q's store W 4186, found / new 4006 / 4186
- job 12, R = P + Q: add (4096): den 0.079, general 0.921; cross tests 16384 (matched 323, cross reads 16384); R's store W 4188, found / new 4004 / 4188
- job 13, P = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.042, den 0.001, general 0.949; mul (2097152): int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.096, int_y 0.098, general 0.760; cross tests 8388608 (matched 98316, cross reads 8388608); P's store W 16493, found / new 16275 / 16493
- job 13, Q = C x D: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.043, den 0.001, general 0.948; mul (2097152): one_y 0.000, int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.098, int_y 0.099, general 0.755; cross tests 8388608 (matched 99914, cross reads 8388480); Q's store W 16494, found / new 16274 / 16494
- job 13, R = P + Q: add (16384): den 0.105, general 0.895; cross tests 65536 (matched 1715, cross reads 65536); R's store W 16495, found / new 16273 / 16495
- job 14, P = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.046, den 0.001, general 0.945; mul (2097152): int_both 0.013, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.104, int_y 0.097, general 0.751; cross tests 8388608 (matched 100470, cross reads 8388608); P's store W 16496, found / new 16272 / 16496
- job 14, Q = C x D: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.044, den 0.001, general 0.947; mul (2097152): int_both 0.012, den 0.036, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.095, int_y 0.102, general 0.754; cross tests 8388608 (matched 101881, cross reads 8388608); Q's store W 16496, found / new 16272 / 16496
- job 14, R = P + Q: add (16384): int_x 0.000, den 0.099, general 0.901; cross tests 65536 (matched 1621, cross reads 65536); R's store W 16493, found / new 16275 / 16493
- job 15, P = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.001, int_y 0.045, den 0.001, general 0.945; mul (2097152): int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.091, int_y 0.102, general 0.760; cross tests 8388608 (matched 97788, cross reads 8388608); P's store W 16493, found / new 16275 / 16493
- job 15, Q = C x D: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.041, den 0.001, general 0.950; mul (2097152): int_both 0.011, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.094, int_y 0.093, general 0.767; cross tests 8388608 (matched 97481, cross reads 8388608); Q's store W 16498, found / new 16270 / 16498
- job 15, R = P + Q: add (16384): den 0.105, general 0.895; cross tests 65536 (matched 1721, cross reads 65536); R's store W 16497, found / new 16271 / 16497
- job 16, P = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.001, int_y 0.054, den 0.001, general 0.937; mul (2097152): int_both 0.016, den 0.036, num 0.000, int_x 0.121, int_y 0.102, general 0.725; cross tests 8388608 (matched 109928, cross reads 8388608); P's store W 16497, found / new 16271 / 16497
- job 16, Q = C x D: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.001, int_y 0.045, den 0.001, general 0.945; mul (2097152): int_both 0.012, den 0.035, num 0.000, cross_cb 0.000, int_x 0.091, int_y 0.103, general 0.758; cross tests 8388608 (matched 99632, cross reads 8388608); Q's store W 16495, found / new 16273 / 16495
- job 16, R = P + Q: add (16384): den 0.098, general 0.902; cross tests 65536 (matched 1599, cross reads 65536); R's store W 16490, found / new 16278 / 16490

The gates checked every counter and store figure against the model (spec v0.2 §15.7 item 2), the TC gate included: MHVF:1 pass, TC:1 pass, MHVGM:1 pass, MHVP:1 pass, TC:2 pass, MHVF:2 pass, MHVGM:2 pass, MHVP:2 pass, TC:3 pass, MHVF:3 pass, MHVGM:3 pass, MHVP:3 pass, MHVF:4 pass, TC:4 pass, MHVP:4 pass, MHVGM:4 pass, MHVF:5 pass, MHVGM:5 pass, TC:5 pass, MHVP:5 pass, TC:6 pass, MHVF:6 pass, MHVGM:6 pass, MHVP:6 pass, MHVF:7 pass, TC:7 pass, MHVGM:7 pass, MHVP:7 pass, MHVF:8 pass, TC:8 pass, MHVGM:8 pass, MHVP:8 pass, MHVF:9 pass, MHVGM:9 pass, TC:9 pass, MHVP:9 pass, MHVF:10 pass, TC:10 pass, MHVGM:10 pass, MHVP:10 pass, MHVF:11 pass, TC:11 pass, MHVGM:11 pass, MHVP:11 pass, MHVF:12 pass, TC:12 pass, MHVGM:12 pass, MHVP:12 pass, MHVF:13 pass, MHVGM:13 pass, TC:13 pass, MHVP:13 pass, MHVF:14 pass, TC:14 pass, MHVGM:14 pass, MHVP:14 pass, MHVF:15 pass, TC:15 pass, MHVGM:15 pass, MHVP:15 pass, MHVF:16 pass, TC:16 pass, MHVGM:16 pass, MHVP:16 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m2 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 2.621 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 1 (8->8 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 2.585 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (8->7 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 18.756 | no | 1 | 14 | 0.071 | 16 | 2 | none | 24 | none | process_api 1 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 2 (7->7 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 13.047 | no | 9 | 64 | 0.141 | 64 | 0 | none | 19 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/15 (8->8 threads), claude 1/16 (7->6 threads), bash 0/7 (1->1 threads) |
| 2 | measured | 1 | 12.809 | no | 9 | 64 | 0.141 | 66 | 2 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 2/15 (6->7 threads), bash 0/7 (1->1 threads) |
| 3 | measured | 1 | 12.738 | no | 10 | 64 | 0.156 | 64 | 0 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (7->6 threads), bash 0/7 (1->1 threads) |
| 4 | measured | 1 | 12.107 | no | 9 | 64 | 0.141 | 65 | 1 | none | 18 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/15 (8->8 threads), claude 2/15 (6->7 threads), bash 0/7 (1->1 threads) |
| 5 | measured | 1 | 13.172 | no | 10 | 64 | 0.156 | 66 | 2 | none | 19 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/16 (8->8 threads), claude 1/16 (6->8 threads), bash 0/7 (1->1 threads) |
| 6 | measured | 1 | 13.069 | no | 10 | 64 | 0.156 | 65 | 1 | none | 19 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 2/16 (8->8 threads), bash 0/7 (1->1 threads) |
| 7 | measured | 1 | 12.968 | no | 10 | 64 | 0.156 | 64 | 0 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/16 (6->6 threads), bash 0/7 (1->1 threads) |
| 8 | measured | 1 | 12.562 | no | 9 | 64 | 0.141 | 66 | 2 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/15 (6->8 threads), bash 0/7 (1->1 threads) |
| 9 | measured | 1 | 12.941 | no | 10 | 64 | 0.156 | 64 | 0 | none | 18 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/15 (8->8 threads), claude 1/16 (7->7 threads), bash 0/7 (1->1 threads) |
| 10 | measured | 1 | 12.947 | no | 10 | 64 | 0.156 | 67 | 3 | none | 18 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 0/15 (8->8 threads), claude 1/16 (7->8 threads), bash 0/7 (1->1 threads) |
| 11 | measured | 1 | 12.847 | no | 11 | 64 | 0.172 | 64 | 0 | none | 18 | none | process_api 0/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/15 (8->8 threads), claude 1/15 (7->6 threads), bash 0/7 (1->1 threads) |
| 12 | measured | 1 | 13.092 | no | 10 | 64 | 0.156 | 66 | 2 | none | 19 | none | process_api 1/7 (6->6 threads), sh 0/7 (1->1 threads), environment-man 1/15 (8->8 threads), claude 1/16 (6->8 threads), bash 0/7 (1->1 threads) |
| 13 | measured | 1 | 101.992 | no | 10 | 64 | 0.156 | 71 | 7 | none | 107 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/83 (8->8 threads), claude 11/84 (7->6 threads), bash 0/16 (1->1 threads) |
| 14 | measured | 1 | 101.176 | no | 10 | 64 | 0.156 | 73 | 9 | none | 107 | none | process_api 3/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 2/83 (8->8 threads), claude 11/84 (7->8 threads), bash 0/16 (1->1 threads) |
| 15 | measured | 1 | 101.947 | no | 10 | 64 | 0.156 | 70 | 6 | none | 107 | none | process_api 3/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/83 (8->8 threads), claude 10/84 (8->7 threads), bash 0/16 (1->1 threads) |
| 16 | measured | 1 | 101.018 | no | 10 | 64 | 0.156 | 74 | 10 | none | 107 | none | process_api 3/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 2/83 (8->8 threads), claude 12/84 (7->8 threads), bash 0/16 (1->1 threads) |

The runner's own ticks over the 19 blocks: 162 ticks over 1066 launches, 0.152 per launch; the highest block 0.172 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 0.9999 | 0.9961 | 915275 | 914880 |
| 1 | Hash-V-fixed | 12 | 1.0000 | 0.9985 | 1057725 | 1057732 |
| 1 | Hash-V-grow-map | 12 | 1.0000 | 0.9989 | 1130512 | 1130845 |
| 1 | Hash-V-pack | 12 | 0.9998 | 0.9942 | 1089408 | 1092154 |
| 1 | Hash-V-fixed-null | 12 | 0.9996 | 0.9787 | 1062746 | 1064036 |
| 2 | TC | 12 | 0.9999 | 0.9991 | 906773 | 906890 |
| 2 | Hash-V-fixed | 12 | 1.0000 | 0.9802 | 1048045 | 1048359 |
| 2 | Hash-V-grow-map | 12 | 1.0000 | 0.9986 | 1095892 | 1095689 |
| 2 | Hash-V-pack | 12 | 0.9999 | 0.9979 | 1071866 | 1071480 |
| 2 | Hash-V-fixed-null | 12 | 0.9999 | 0.9983 | 1052422 | 1052073 |
| 3 | TC | 12 | 0.9989 | 0.9237 | 886851 | 891420 |
| 3 | Hash-V-fixed | 12 | 0.9997 | 0.9875 | 1030509 | 1031071 |
| 3 | Hash-V-grow-map | 12 | 0.9995 | 0.9818 | 1072067 | 1071723 |
| 3 | Hash-V-pack | 12 | 0.9998 | 0.9933 | 1048883 | 1048443 |
| 3 | Hash-V-fixed-null | 12 | 0.9991 | 0.9890 | 1033869 | 1034772 |
| 4 | TC | 12 | 1.0000 | 0.9968 | 845081 | 845641 |
| 4 | Hash-V-fixed | 12 | 0.9995 | 0.9830 | 993414 | 993634 |
| 4 | Hash-V-grow-map | 12 | 0.9999 | 0.9918 | 1026963 | 1027345 |
| 4 | Hash-V-pack | 12 | 0.9998 | 0.9938 | 1006278 | 1005709 |
| 4 | Hash-V-fixed-null | 12 | 0.9999 | 0.9922 | 989446 | 989067 |
| 5 | TC | 12 | 0.9999 | 0.9852 | 7570255 | 7595764 |
| 5 | Hash-V-fixed | 12 | 0.9998 | 0.9931 | 8554193 | 8575388 |
| 5 | Hash-V-grow-map | 12 | 0.9995 | 0.9951 | 8743053 | 8762029 |
| 5 | Hash-V-pack | 12 | 0.9997 | 0.9941 | 8626707 | 8636265 |
| 5 | Hash-V-fixed-null | 12 | 0.9996 | 0.9960 | 8536182 | 8542969 |
| 6 | TC | 12 | 0.9998 | 0.9946 | 7508059 | 7510255 |
| 6 | Hash-V-fixed | 12 | 0.9998 | 0.9986 | 8480742 | 8486498 |
| 6 | Hash-V-grow-map | 12 | 0.9999 | 0.9987 | 8630228 | 8632122 |
| 6 | Hash-V-pack | 12 | 1.0000 | 0.9985 | 8538973 | 8538213 |
| 6 | Hash-V-fixed-null | 12 | 0.9996 | 0.9813 | 8472750 | 8480524 |
| 7 | TC | 12 | 1.0000 | 0.9931 | 7485209 | 7487244 |
| 7 | Hash-V-fixed | 12 | 0.9999 | 0.9948 | 8436442 | 8450272 |
| 7 | Hash-V-grow-map | 12 | 0.9997 | 0.9970 | 8573568 | 8576420 |
| 7 | Hash-V-pack | 12 | 0.9998 | 0.9870 | 8497459 | 8521005 |
| 7 | Hash-V-fixed-null | 12 | 0.9993 | 0.9941 | 8473666 | 8481806 |
| 8 | TC | 12 | 0.9998 | 0.9955 | 7167990 | 7172592 |
| 8 | Hash-V-fixed | 12 | 0.9996 | 0.9827 | 8178223 | 8178885 |
| 8 | Hash-V-grow-map | 12 | 0.9996 | 0.9376 | 8336481 | 8344680 |
| 8 | Hash-V-pack | 12 | 0.9994 | 0.9778 | 8241756 | 8263553 |
| 8 | Hash-V-fixed-null | 12 | 0.9994 | 0.9636 | 8196967 | 8200103 |
| 9 | TC | 12 | 0.9998 | 0.9919 | 61280032 | 61368568 |
| 9 | Hash-V-fixed | 12 | 0.9995 | 0.9798 | 68642544 | 68755957 |
| 9 | Hash-V-grow-map | 12 | 0.9998 | 0.9954 | 69153226 | 69188938 |
| 9 | Hash-V-pack | 12 | 0.9988 | 0.9960 | 68877830 | 68969558 |
| 9 | Hash-V-fixed-null | 12 | 0.9996 | 0.9868 | 68683528 | 68769146 |
| 10 | TC | 12 | 1.0000 | 0.9901 | 61359098 | 61420415 |
| 10 | Hash-V-fixed | 12 | 0.9994 | 0.8888 | 68815062 | 68833853 |
| 10 | Hash-V-grow-map | 12 | 1.0000 | 0.9978 | 69087792 | 69091686 |
| 10 | Hash-V-pack | 12 | 0.9995 | 0.9965 | 68647172 | 68690172 |
| 10 | Hash-V-fixed-null | 12 | 0.9999 | 0.9955 | 68624008 | 68642200 |
| 11 | TC | 12 | 0.9998 | 0.9925 | 61247333 | 61277824 |
| 11 | Hash-V-fixed | 12 | 0.9997 | 0.9914 | 68481255 | 68515983 |
| 11 | Hash-V-grow-map | 12 | 0.9995 | 0.9745 | 68787850 | 68846208 |
| 11 | Hash-V-pack | 12 | 0.9993 | 0.9864 | 68653910 | 68822914 |
| 11 | Hash-V-fixed-null | 12 | 0.9993 | 0.9971 | 68382988 | 68403226 |
| 12 | TC | 12 | 0.9998 | 0.9924 | 62076238 | 62135954 |
| 12 | Hash-V-fixed | 12 | 0.9999 | 0.9827 | 69254856 | 69251116 |
| 12 | Hash-V-grow-map | 12 | 0.9971 | 0.9556 | 69421315 | 69805830 |
| 12 | Hash-V-pack | 12 | 0.9990 | 0.9842 | 69264942 | 69370374 |
| 12 | Hash-V-fixed-null | 12 | 0.9999 | 0.9861 | 69434403 | 69497886 |
| 13 | TC | 12 | 0.9997 | 0.9971 | 496284874 | 497361480 |
| 13 | Hash-V-fixed | 12 | 0.9980 | 0.9905 | 559163539 | 561483494 |
| 13 | Hash-V-grow-map | 12 | 0.9991 | 0.9879 | 558273668 | 559451094 |
| 13 | Hash-V-pack | 12 | 0.9995 | 0.9958 | 554618164 | 554844676 |
| 13 | Hash-V-fixed-null | 12 | 0.9994 | 0.9913 | 556206268 | 557063254 |
| 14 | TC | 12 | 0.9989 | 0.9908 | 494352015 | 496260374 |
| 14 | Hash-V-fixed | 12 | 0.9992 | 0.9934 | 551858843 | 552524839 |
| 14 | Hash-V-grow-map | 12 | 0.9975 | 0.9915 | 551685596 | 553300707 |
| 14 | Hash-V-pack | 12 | 0.9986 | 0.9911 | 552691600 | 554009018 |
| 14 | Hash-V-fixed-null | 12 | 0.9991 | 0.9829 | 550905466 | 551418360 |
| 15 | TC | 12 | 0.9995 | 0.9875 | 495612218 | 496486346 |
| 15 | Hash-V-fixed | 12 | 0.9983 | 0.9759 | 552860154 | 553861576 |
| 15 | Hash-V-grow-map | 12 | 0.9985 | 0.9819 | 553469979 | 553999842 |
| 15 | Hash-V-pack | 12 | 0.9988 | 0.9905 | 556487598 | 559904382 |
| 15 | Hash-V-fixed-null | 12 | 0.9998 | 0.9965 | 550885882 | 550972117 |
| 16 | TC | 12 | 0.9946 | 0.9829 | 491729618 | 495025138 |
| 16 | Hash-V-fixed | 12 | 0.9970 | 0.9877 | 546406566 | 548686658 |
| 16 | Hash-V-grow-map | 12 | 0.9986 | 0.9906 | 547861939 | 548655425 |
| 16 | Hash-V-pack | 12 | 0.9981 | 0.9782 | 548186537 | 549581112 |
| 16 | Hash-V-fixed-null | 12 | 0.9961 | 0.9860 | 547976025 | 550983071 |

Idle checks: 19 (0 busy). Host rates R_h: {'1': 0.053317545320012996, '79': 0.05331627212625293, '84': 0.3815624792974888, '92': 0.38679374824256924, '20878': 0.05331627212625293} (measured: {'1': 0.053317545320012996, '79': 0.0, '84': 0.3815624792974888, '92': 0.38679374824256924, '20878': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

