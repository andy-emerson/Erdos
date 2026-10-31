# Design 3 on matrices, round m2, matrix multiplication: results

Run started 2026-10-30T09:15:26-05:00.

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

- The mul run (job set `mul`: C = A x B).
- Started 2026-10-30T09:15:26-05:00; finished 2026-10-30T09:24:16-05:00.
- Step 0 (before the clock): 86.7 s; step 0's files: equal to m1's by sha256.
- Run time 7.4 minutes (hard stop 40).
- Rule sha256 `e340d32b5169154c44a2f66f99d91588620ab986756c7d5fb496149b99e46200`; round 3's results after the run: unchanged; m1's results and the other m2 runs' folders after the run: unchanged.
- Host processes (the runner's ancestors): bash[19300], claude[92], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-30T09:16:52-05:00: TC `179dacc22ad6`, MHVF `eb7f57fd8798`, MHVGM `915116a75090`, MHVP `56a37a633b7c`, TC3GATE `83ff353acc03`
- Unit tests: m2_unit: m2_unit: 642586 checks, 0 failures; m2_diff: m2_diff: 3000 seeds, 3000 jobs, 1050092 operations; 1137376 checks, 0 failures; m2_alloc-timing: m2_alloc (timing build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-counting: m2_alloc (counting build): 160 jobs, 2450 moments, 28443 checks, 0 failures; m2_alloc-memory: m2_alloc (memory build): 160 jobs, 2450 moments, 32596 checks, 0 failures; m2_unit-asan: m2_unit: 642586 checks, 0 failures; m2_diff-asan: m2_diff: 600 seeds, 600 jobs, 209812 operations; 227944 checks, 0 failures

## The pilot (discarded, except what it records for the run)

- Jobs [1, 6, 13]; block wall times job 1 4.5 s, job 6 4.6 s, job 13 11.4 s; P_6 = 4.6 s, P_13 = 11.4 s; N_128 = 4, N_rest = 12.
- Projection elapsed + 1.2 x 5 x (N_128 P_13 + N_rest P_6) = 10.8 min (limit 30).
- Host rates R_h (ticks per second): {'1': 0.10634903754121025, '79': 0.10634903754121025, '84': 0.10634903754121025, '92': 0.10634903754121025, '19300': 0.10634903754121025}.

## Verdicts, per job, pairing and measure (no overall winner)

Headline measures of this run: 1 build time of one input matrix; 3 multiplication time; 6 stable bytes of one stored input matrix; 8 peak bytes during the operation. Time: T(reference)/T(row), above 1 the row is faster; measure 1 pools the run's input matrices per round (median of all samples; the bootstrap resamples rounds with their samples kept together); each time measure has its own band h from Hash-V-fixed against Hash-V-fixed-null. Total and read-out time are reported without a verdict. Bytes per cell: row over reference, below 1 the row is smaller, on the accounted figure (the heap's in parentheses). Hash-V-pack against Hash-V-grow-map is reported with no verdict on any measure. Gcd work is a check, not a verdict. (c): the other rows' own noise is not measured: the null row is a copy of Hash-V-fixed, and on this job a row of the pairing ran 1 rep per invocation (its calibration rep lasted at least 1 s), so its noise may differ (rule-m2 item 8, carried over).

### Hash-V-fixed against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.635 [0.627, 0.646], h 0.050 | loss, 0.873 [0.867, 0.876], h 0.050 | reported, 0.864 [0.858, 0.867] | reported, 0.588 [0.580, 0.626] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.333 (heap 2.318) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.643 [0.633, 0.654], h 0.050 | loss, 0.870 [0.868, 0.875], h 0.050 | reported, 0.862 [0.859, 0.866] | reported, 0.588 [0.581, 0.630] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.333 (heap 2.318) **severe memory loss** |
| 3 (n 16, rho 16) | loss, 0.655 [0.644, 0.670], h 0.050 | loss, 0.843 [0.803, 0.846], h 0.050 | reported, 0.835 [0.796, 0.839] | reported, 0.590 [0.545, 0.710] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.333 (heap 2.318) **severe memory loss** |
| 4 (n 16, rho 64) | loss, 0.679 [0.667, 0.686], h 0.050 | loss, 0.837 [0.834, 0.863], h 0.050 | reported, 0.832 [0.828, 0.857] | reported, 0.622 [0.579, 0.704] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.333 (heap 2.318) **severe memory loss** |
| 5 (n 32, rho 1) | loss, 0.715 [0.709, 0.721], h 0.050 | loss, 0.890 [0.887, 0.893], h 0.050 | reported, 0.886 [0.883, 0.889] | reported, 0.639 [0.614, 0.751] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.333 (heap 2.329) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.690 [0.687, 0.693], h 0.050 | loss, 0.889 [0.886, 0.891], h 0.050 | reported, 0.885 [0.882, 0.887] | reported, 0.622 [0.609, 0.688] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.333 (heap 2.329) **severe memory loss** |
| 7 (n 32, rho 16) | loss, 0.754 [0.742, 0.758], h 0.050 | loss, 0.890 [0.889, 0.892], h 0.050 | reported, 0.888 [0.886, 0.889] | reported, 0.623 [0.612, 0.819] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.333 (heap 2.329) **severe memory loss** |
| 8 (n 32, rho 64) | loss, 0.749 [0.744, 0.754], h 0.050 | loss, 0.878 [0.875, 0.882], h 0.050 | reported, 0.876 [0.873, 0.880] | reported, 0.662 [0.615, 0.809] | loss, 2.250 (heap 2.246) **severe memory loss** | loss, 2.333 (heap 2.329) **severe memory loss** |
| 9 (n 64, rho 1) | loss, 0.713 [0.707, 0.720], h 0.050 | loss, 0.897 [0.892, 0.899], h 0.050 | reported, 0.895 [0.891, 0.897] | reported, 0.610 [0.552, 0.657] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.333 (heap 2.332) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.719 [0.713, 0.724], h 0.050 | loss, 0.895 [0.888, 0.902], h 0.050 | reported, 0.894 [0.886, 0.900] | reported, 0.629 [0.561, 0.722] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.333 (heap 2.332) **severe memory loss** |
| 11 (n 64, rho 16) | loss, 0.758 [0.754, 0.762], h 0.050 | loss, 0.897 [0.892, 0.906], h 0.050 | reported, 0.895 [0.890, 0.905] | reported, 0.611 [0.556, 0.760] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.333 (heap 2.332) **severe memory loss** |
| 12 (n 64, rho 64) | loss, 0.766 [0.757, 0.773], h 0.050 | loss, 0.891 [0.888, 0.895], h 0.050 | reported, 0.890 [0.886, 0.894] | reported, 0.659 [0.569, 0.754] | loss, 2.250 (heap 2.249) **severe memory loss** | loss, 2.333 (heap 2.332) **severe memory loss** |
| 13 (n 128, rho 1) | loss, 0.683 [0.679, 0.689], h 0.050 | loss, 0.900 [0.896, 0.904], h 0.050 | reported, 0.898 [0.895, 0.903] | reported, 0.279 [0.214, 0.315] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.583 (heap 2.583) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.720 [0.709, 0.724], h 0.050 | loss, 0.898 [0.894, 0.904], h 0.050 | reported, 0.896 [0.893, 0.902] | reported, 0.263 [0.200, 0.286] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.583 (heap 2.583) **severe memory loss** |
| 15 (n 128, rho 16) | loss, 0.750 [0.742, 0.755], h 0.050 | loss, 0.906 [0.900, 0.917], h 0.050 | reported, 0.905 [0.899, 0.915] | reported, 0.265 [0.225, 0.351] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.583 (heap 2.583) **severe memory loss** |
| 16 (n 128, rho 64) | loss, 0.755 [0.748, 0.766], h 0.050 | loss, 0.901 [0.889, 0.905], h 0.050 | reported, 0.900 [0.888, 0.904] | reported, 0.335 [0.252, 0.481] | loss, 2.500 (heap 2.500) **severe memory loss** | loss, 2.583 (heap 2.583) **severe memory loss** |

### Hash-V-grow-map against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.312 [0.308, 0.317], h 0.050 | loss, 0.851 [0.846, 0.855], h 0.050 | reported, 0.811 [0.807, 0.815] | reported, 0.549 [0.539, 0.589] | loss, 2.250 (heap 2.236) **severe memory loss** | loss, 2.667 (heap 2.646) **severe memory loss** |
| 2 (n 16, rho 4) | loss, 0.479 [0.472, 0.486], h 0.050 | loss, 0.856 [0.853, 0.860], h 0.050 | reported, 0.837 [0.833, 0.840] | reported, 0.555 [0.545, 0.592] | win, 0.750 (heap 0.764) | loss, 1.667 (heap 1.665) |
| 3 (n 16, rho 16) | loss, 0.578 [0.572, 0.591], h 0.050 | loss, 0.825 [0.822, 0.828], h 0.050 | reported, 0.814 [0.812, 0.818] | reported, 0.550 [0.538, 0.669] | win, 0.375 (heap 0.397) | loss, 1.417 (heap 1.420) |
| 4 (n 16, rho 64) | loss, 0.635 [0.622, 0.643], h 0.050 | loss, 0.824 [0.820, 0.848], h 0.050 | reported, 0.817 [0.812, 0.841] | reported, 0.595 [0.554, 0.678] | win, 0.281 (heap 0.305) | loss, 1.354 (heap 1.358) |
| 5 (n 32, rho 1) | loss, 0.449 [0.445, 0.450], h 0.050 | loss, 0.880 [0.874, 0.885], h 0.050 | reported, 0.867 [0.861, 0.871] | reported, 0.614 [0.584, 0.742] | loss, 1.250 (heap 1.251) | loss, 2.000 (heap 1.998) **severe memory loss** |
| 6 (n 32, rho 4) | loss, 0.521 [0.518, 0.525], h 0.050 | loss, 0.882 [0.877, 0.886], h 0.050 | reported, 0.873 [0.868, 0.876] | reported, 0.595 [0.578, 0.668] | win, 0.750 (heap 0.754) | loss, 1.667 (heap 1.666) |
| 7 (n 32, rho 16) | loss, 0.640 [0.633, 0.650], h 0.050 | loss, 0.885 [0.877, 0.886], h 0.050 | reported, 0.879 [0.872, 0.880] | reported, 0.600 [0.577, 0.786] | win, 0.375 (heap 0.380) | loss, 1.417 (heap 1.417) |
| 8 (n 32, rho 64) | loss, 0.650 [0.641, 0.651], h 0.050 | loss, 0.874 [0.870, 0.878], h 0.050 | reported, 0.870 [0.866, 0.873] | reported, 0.613 [0.569, 0.751] | win, 0.281 (heap 0.287) | loss, 1.354 (heap 1.355) |
| 9 (n 64, rho 1) | loss, 0.459 [0.453, 0.462], h 0.050 | loss, 0.897 [0.893, 0.898], h 0.050 | reported, 0.890 [0.886, 0.891] | reported, 0.655 [0.621, 0.748] | loss, 1.250 (heap 1.250) | loss, 2.000 (heap 1.999) **severe memory loss** |
| 10 (n 64, rho 4) | loss, 0.584 [0.570, 0.594], h 0.050 | loss, 0.894 [0.886, 0.902], h 0.050 | reported, 0.891 [0.882, 0.898] | reported, 0.715 [0.657, 0.819] | win, 0.500 (heap 0.501) | loss, 1.500 (heap 1.500) |
| 11 (n 64, rho 16) | loss, 0.647 [0.639, 0.653], h 0.050 | loss, 0.896 [0.893, 0.905], h 0.050 | reported, 0.893 [0.890, 0.902] | reported, 0.823 [0.732, 1.005] | win, 0.375 (heap 0.376) | loss, 1.417 (heap 1.417) |
| 12 (n 64, rho 64) | loss, 0.661 [0.652, 0.670], h 0.050 | loss, 0.893 [0.890, 0.897], h 0.050 | reported, 0.890 [0.887, 0.894] | reported, 0.729 [0.659, 0.814] | win, 0.281 (heap 0.283) | loss, 1.354 (heap 1.354) |
| 13 (n 128, rho 1) | loss, 0.464 [0.457, 0.467], h 0.050 | loss, 0.899 [0.894, 0.901], h 0.050 | reported, 0.896 [0.891, 0.898] | reported, 0.719 [0.646, 0.845] | loss, 1.500 (heap 1.500) | loss, 2.333 (heap 2.333) **severe memory loss** |
| 14 (n 128, rho 4) | loss, 0.609 [0.606, 0.613], h 0.050 | loss, 0.897 [0.894, 0.908], h 0.050 | reported, 0.896 [0.893, 0.906] | reported, 0.887 [0.800, 0.931] | win, 0.750 (heap 0.750) | loss, 1.833 (heap 1.833) |
| 15 (n 128, rho 16) | loss, 0.657 [0.650, 0.667], h 0.050 | loss, 0.903 [0.892, 0.912], h 0.050 | reported, 0.902 [0.891, 0.911] | reported, 0.905 [0.799, 0.991] | win, 0.562 (heap 0.563) | loss, 1.708 (heap 1.708) |
| 16 (n 128, rho 64) | loss, 0.713 [0.703, 0.728], h 0.050 | loss, 0.902 [0.897, 0.908], h 0.050 | reported, 0.901 [0.896, 0.907] | reported, 0.910 [0.785, 1.014] | win, 0.531 (heap 0.532) | loss, 1.688 (heap 1.687) |

### Hash-V-pack against TC (against TC)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.443 [0.439, 0.450], h 0.050 | loss, 0.865 [0.859, 0.868], h 0.050 | reported, 0.842 [0.836, 0.845] | reported, 0.498 [0.493, 0.529] | win, 0.803 (heap 0.818) | loss, 1.654 (heap 1.655) |
| 2 (n 16, rho 4) | loss, 0.521 [0.516, 0.530], h 0.050 | loss, 0.865 [0.862, 0.868], h 0.050 | reported, 0.849 [0.846, 0.853] | reported, 0.489 [0.482, 0.523] | win, 0.299 (heap 0.326) | loss, 1.319 (heap 1.328) |
| 3 (n 16, rho 16) | loss, 0.563 [0.557, 0.575], h 0.050 | loss, 0.836 [0.832, 0.839], h 0.050 | reported, 0.824 [0.820, 0.828] | reported, 0.493 [0.487, 0.596] | win, 0.174 (heap 0.203) | loss, 1.238 (heap 1.247) |
| 4 (n 16, rho 64) | loss, 0.591 [0.579, 0.596], h 0.050 | loss, 0.831 [0.827, 0.853], h 0.050 | reported, 0.821 [0.817, 0.843] | reported, 0.521 [0.486, 0.592] | win, 0.140 (heap 0.169) | loss, 1.201 (heap 1.211) |
| 5 (n 32, rho 1) | loss, 0.516 [0.512, 0.519], h 0.050 | loss, 0.885 [0.880, 0.887], h 0.050 | reported, 0.874 [0.871, 0.877] | reported, 0.555 [0.532, 0.679] | win, 0.744 (heap 0.748) | loss, 1.590 (heap 1.591) |
| 6 (n 32, rho 4) | loss, 0.583 [0.579, 0.589], h 0.050 | loss, 0.884 [0.883, 0.887], h 0.050 | reported, 0.877 [0.876, 0.880] | reported, 0.563 [0.545, 0.632] | win, 0.387 (heap 0.394) | loss, 1.354 (heap 1.356) |
| 7 (n 32, rho 16) | loss, 0.665 [0.655, 0.670], h 0.050 | loss, 0.890 [0.885, 0.892], h 0.050 | reported, 0.884 [0.880, 0.886] | reported, 0.526 [0.511, 0.691] | win, 0.168 (heap 0.175) | loss, 1.208 (heap 1.210) |
| 8 (n 32, rho 64) | loss, 0.674 [0.661, 0.678], h 0.050 | loss, 0.880 [0.875, 0.884], h 0.050 | reported, 0.876 [0.871, 0.880] | reported, 0.584 [0.531, 0.724] | win, 0.139 (heap 0.146) | loss, 1.186 (heap 1.188) |
| 9 (n 64, rho 1) | loss, 0.535 [0.532, 0.537], h 0.050 | loss, 0.897 [0.894, 0.899], h 0.050 | reported, 0.893 [0.889, 0.894] | reported, 0.576 [0.538, 0.669] | win, 0.685 (heap 0.686) | loss, 1.544 (heap 1.544) |
| 10 (n 64, rho 4) | loss, 0.621 [0.618, 0.625], h 0.050 | loss, 0.896 [0.892, 0.903], h 0.050 | reported, 0.892 [0.888, 0.900] | reported, 0.693 [0.637, 0.786] | win, 0.373 (heap 0.375) | loss, 1.336 (heap 1.336) |
| 11 (n 64, rho 16) | loss, 0.680 [0.676, 0.686], h 0.050 | loss, 0.896 [0.894, 0.907], h 0.050 | reported, 0.894 [0.892, 0.905] | reported, 0.760 [0.670, 0.947] | win, 0.284 (heap 0.286) | loss, 1.277 (heap 1.277) |
| 12 (n 64, rho 64) | loss, 0.700 [0.696, 0.705], h 0.050 | loss, 0.895 [0.885, 0.898], h 0.050 | reported, 0.893 [0.884, 0.896] | reported, 0.691 [0.620, 0.768] | win, 0.136 (heap 0.138) | loss, 1.178 (heap 1.179) |
| 13 (n 128, rho 1) | loss, 0.549 [0.545, 0.554], h 0.050 | loss, 0.901 [0.898, 0.904], h 0.050 | reported, 0.899 [0.895, 0.901] | reported, 0.814 [0.746, 0.905] | win, 0.558 (heap 0.559) | loss, 1.707 (heap 1.707) |
| 14 (n 128, rho 4) | loss, 0.630 [0.624, 0.633], h 0.050 | loss, 0.898 [0.895, 0.903], h 0.050 | reported, 0.897 [0.894, 0.902] | reported, 0.861 [0.780, 0.890] | win, 0.358 (heap 0.358) | loss, 1.573 (heap 1.573) |
| 15 (n 128, rho 16) | loss, 0.681 [0.672, 0.683], h 0.050 | loss, 0.902 [0.898, 0.911], h 0.050 | reported, 0.901 [0.897, 0.910] | reported, 0.884 [0.796, 0.919] | win, 0.281 (heap 0.281) | loss, 1.522 (heap 1.522) |
| 16 (n 128, rho 64) | loss, 0.707 [0.699, 0.717], h 0.050 | loss, 0.901 [0.894, 0.905], h 0.050 | reported, 0.901 [0.894, 0.904] | reported, 0.848 [0.735, 0.953] | win, 0.259 (heap 0.259) | loss, 1.507 (heap 1.507) |

### Hash-V-pack against Hash-V-fixed (pack against fixed)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.698 [0.691, 0.705], h 0.050 | tie, 0.990 [0.984, 0.996], h 0.050 | reported, 0.974 [0.968, 0.980] | reported, 0.846 [0.840, 0.855] | win, 0.357 (heap 0.366) | win, 0.709 (heap 0.714) |
| 2 (n 16, rho 4) | loss, 0.810 [0.805, 0.822], h 0.050 | tie, 0.994 [0.990, 0.998], h 0.050 | reported, 0.985 [0.982, 0.989] | reported, 0.831 [0.824, 0.837] | win, 0.133 (heap 0.146) | win, 0.565 (heap 0.573) |
| 3 (n 16, rho 16) | loss, 0.860 [0.852, 0.872], h 0.050 | tie, 0.993 [0.986, 1.043], h 0.050 | reported, 0.987 [0.980, 1.037] | reported, 0.837 [0.825, 0.903] | win, 0.077 (heap 0.091) | win, 0.530 (heap 0.538) |
| 4 (n 16, rho 64) | loss, 0.870 [0.862, 0.877], h 0.050 | tie, 0.992 [0.986, 0.996], h 0.050 | reported, 0.987 [0.981, 0.991] | reported, 0.837 [0.822, 0.865] | win, 0.062 (heap 0.075) | win, 0.515 (heap 0.523) |
| 5 (n 32, rho 1) | loss, 0.721 [0.713, 0.729], h 0.050 | tie, 0.994 [0.989, 0.998], h 0.050 | reported, 0.987 [0.981, 0.990] | reported, 0.869 [0.845, 0.915] | win, 0.331 (heap 0.333) | win, 0.682 (heap 0.683) |
| 6 (n 32, rho 4) | loss, 0.844 [0.838, 0.853], h 0.050 | tie, 0.995 [0.994, 0.997], h 0.050 | reported, 0.991 [0.990, 0.993] | reported, 0.905 [0.886, 0.932] | win, 0.172 (heap 0.175) | win, 0.580 (heap 0.582) |
| 7 (n 32, rho 16) | loss, 0.882 [0.877, 0.887], h 0.050 | tie, 0.999 [0.994, 1.001], h 0.050 | reported, 0.996 [0.992, 0.999] | reported, 0.844 [0.826, 0.868] | win, 0.075 (heap 0.078) | win, 0.518 (heap 0.520) |
| 8 (n 32, rho 64) | loss, 0.900 [0.885, 0.906], h 0.050 | tie, 1.002 [0.995, 1.006], h 0.050 | reported, 0.999 [0.993, 1.004] | reported, 0.883 [0.853, 0.909] | win, 0.062 (heap 0.065) | win, 0.508 (heap 0.510) |
| 9 (n 64, rho 1) | loss, 0.751 [0.740, 0.757], h 0.050 | tie, 1.000 [0.999, 1.003], h 0.050 | reported, 0.997 [0.996, 1.000] | reported, 0.945 [0.881, 1.055] | win, 0.304 (heap 0.305) | win, 0.662 (heap 0.662) |
| 10 (n 64, rho 4) | loss, 0.864 [0.859, 0.871], h 0.050 | tie, 1.000 [0.997, 1.007], h 0.050 | reported, 0.999 [0.995, 1.005] | reported, 1.103 [1.001, 1.201] | win, 0.166 (heap 0.167) | win, 0.573 (heap 0.573) |
| 11 (n 64, rho 16) | loss, 0.898 [0.891, 0.903], h 0.050 | tie, 1.000 [0.996, 1.008], h 0.050 | reported, 0.999 [0.995, 1.007] | reported, 1.244 [1.018, 1.400] | win, 0.126 (heap 0.127) | win, 0.547 (heap 0.548) |
| 12 (n 64, rho 64) | loss, 0.914 [0.909, 0.926], h 0.050 | tie, 1.005 [0.992, 1.008], h 0.050 | reported, 1.004 [0.992, 1.007] | reported, 1.050 [0.923, 1.209] | win, 0.060 (heap 0.061) | win, 0.505 (heap 0.505) |
| 13 (n 128, rho 1) | loss, 0.804 [0.794, 0.812], h 0.050 | tie, 1.001 [0.997, 1.005], h 0.050 | reported, 1.000 [0.996, 1.004] | reported, 2.923 [2.681, 3.804] | win, 0.223 (heap 0.224) | win, 0.661 (heap 0.661) |
| 14 (n 128, rho 4) | loss, 0.874 [0.867, 0.890], h 0.050 | tie, 1.000 [0.996, 1.006], h 0.050 | reported, 1.000 [0.996, 1.006] | reported, 3.276 [2.979, 4.307] | win, 0.143 (heap 0.143) | win, 0.609 (heap 0.609) |
| 15 (n 128, rho 16) | loss, 0.908 [0.901, 0.914], h 0.050 | tie, 0.996 [0.991, 1.003], h 0.050 | reported, 0.996 [0.991, 1.003] | reported, 3.339 [2.540, 3.908] | win, 0.112 (heap 0.113) | win, 0.589 (heap 0.589) |
| 16 (n 128, rho 64) | loss, 0.936 [0.929, 0.942], h 0.050 | tie, 1.000 [0.992, 1.015], h 0.050 | reported, 1.000 [0.992, 1.015] | reported, 2.530 [1.837, 3.533] | win, 0.103 (heap 0.104) | win, 0.583 (heap 0.583) |

### Hash-V-grow-map against Hash-V-fixed (grow-map against fixed)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | loss, 0.492 [0.489, 0.494], h 0.050 | tie, 0.975 [0.972, 0.981], h 0.050 | reported, 0.939 [0.937, 0.945] | reported, 0.934 [0.919, 0.954] | tie, 1.000 (heap 1.000) | loss, 1.143 (heap 1.141) |
| 2 (n 16, rho 4) | loss, 0.744 [0.736, 0.751], h 0.050 | tie, 0.984 [0.979, 0.986], h 0.050 | reported, 0.971 [0.967, 0.974] | reported, 0.944 [0.930, 0.949] | win, 0.333 (heap 0.342) | win, 0.714 (heap 0.718) |
| 3 (n 16, rho 16) | loss, 0.883 [0.874, 0.895], h 0.050 | tie, 0.979 [0.972, 1.028], h 0.050 | reported, 0.975 [0.969, 1.024] | reported, 0.933 [0.914, 1.013] | win, 0.167 (heap 0.177) | win, 0.607 (heap 0.612) |
| 4 (n 16, rho 64) | loss, 0.935 [0.928, 0.945], h 0.050 | tie, 0.984 [0.977, 0.989], h 0.050 | reported, 0.982 [0.976, 0.988] | reported, 0.956 [0.938, 0.991] | win, 0.125 (heap 0.136) | win, 0.580 (heap 0.586) |
| 5 (n 32, rho 1) | loss, 0.627 [0.622, 0.632], h 0.050 | tie, 0.989 [0.983, 0.995], h 0.050 | reported, 0.978 [0.972, 0.984] | reported, 0.961 [0.933, 0.990] | win, 0.556 (heap 0.557) | win, 0.857 (heap 0.858) |
| 6 (n 32, rho 4) | loss, 0.755 [0.751, 0.761], h 0.050 | tie, 0.993 [0.988, 0.997], h 0.050 | reported, 0.986 [0.982, 0.990] | reported, 0.957 [0.943, 0.981] | win, 0.333 (heap 0.335) | win, 0.714 (heap 0.715) |
| 7 (n 32, rho 16) | loss, 0.849 [0.844, 0.863], h 0.050 | tie, 0.993 [0.985, 0.995], h 0.050 | reported, 0.990 [0.982, 0.992] | reported, 0.963 [0.937, 0.981] | win, 0.167 (heap 0.169) | win, 0.607 (heap 0.608) |
| 8 (n 32, rho 64) | loss, 0.868 [0.855, 0.871], h 0.050 | tie, 0.996 [0.991, 1.000], h 0.050 | reported, 0.993 [0.988, 0.997] | reported, 0.926 [0.911, 0.961] | win, 0.125 (heap 0.128) | win, 0.580 (heap 0.582) |
| 9 (n 64, rho 1) | loss, 0.644 [0.634, 0.651], h 0.050 | tie, 0.999 [0.997, 1.003], h 0.050 | reported, 0.994 [0.992, 0.998] | reported, 1.074 [1.010, 1.206] | win, 0.556 (heap 0.556) | win, 0.857 (heap 0.857) |
| 10 (n 64, rho 4) | loss, 0.813 [0.790, 0.827], h 0.050 | tie, 0.999 [0.993, 1.004], h 0.050 | reported, 0.997 [0.991, 1.001] | reported, 1.137 [1.031, 1.244] | win, 0.222 (heap 0.223) | win, 0.643 (heap 0.643) |
| 11 (n 64, rho 16) | loss, 0.854 [0.843, 0.861], h 0.050 | tie, 0.999 [0.997, 1.003], h 0.050 | reported, 0.997 [0.995, 1.001] | reported, 1.347 [1.134, 1.491] | win, 0.167 (heap 0.167) | win, 0.607 (heap 0.607) |
| 12 (n 64, rho 64) | loss, 0.863 [0.850, 0.883], h 0.050 | tie, 1.002 [0.995, 1.006], h 0.050 | reported, 1.000 [0.994, 1.005] | reported, 1.106 [0.962, 1.272] | win, 0.125 (heap 0.126) | win, 0.580 (heap 0.581) |
| 13 (n 128, rho 1) | loss, 0.679 [0.667, 0.686], h 0.050 | tie, 1.000 [0.993, 1.003], h 0.050 | reported, 0.998 [0.991, 1.001] | reported, 2.583 [2.409, 3.296] | win, 0.600 (heap 0.600) | tie, 0.903 (heap 0.903) |
| 14 (n 128, rho 4) | loss, 0.845 [0.842, 0.860], h 0.050 | tie, 0.999 [0.996, 1.006], h 0.050 | reported, 0.999 [0.995, 1.006] | reported, 3.375 [3.023, 4.288] | win, 0.300 (heap 0.300) | win, 0.710 (heap 0.710) |
| 15 (n 128, rho 16) | loss, 0.876 [0.865, 0.893], h 0.050 | tie, 0.997 [0.988, 1.002], h 0.050 | reported, 0.996 [0.987, 1.002] | reported, 3.422 [2.529, 4.034] | win, 0.225 (heap 0.225) | win, 0.661 (heap 0.661) |
| 16 (n 128, rho 64) | tie, 0.944 [0.930, 0.963], h 0.050 | tie, 1.001 [0.996, 1.016], h 0.050 | reported, 1.001 [0.995, 1.017] | reported, 2.712 [1.961, 3.677] | win, 0.212 (heap 0.213) | win, 0.653 (heap 0.653) |

### Hash-V-pack against Hash-V-grow-map (pack against grow-map)

| job | 1 build time of one input matrix | 3 multiplication time | total (reported) | read-out (reported) | 6 stable bytes of one stored input matrix | 8 peak bytes during the operation |
| --- | --- | --- | --- | --- | --- | --- |
| 1 (n 16, rho 1) | reported, 1.418 [1.413, 1.430] | reported, 1.016 [1.008, 1.019] | reported, 1.037 [1.030, 1.041] | reported, 0.906 [0.888, 0.922] | reported, 0.357 (heap 0.366) | reported, 0.620 (heap 0.625) |
| 2 (n 16, rho 4) | reported, 1.088 [1.085, 1.106] | reported, 1.010 [1.007, 1.016] | reported, 1.014 [1.011, 1.019] | reported, 0.881 [0.876, 0.893] | reported, 0.398 (heap 0.426) | reported, 0.791 (heap 0.797) |
| 3 (n 16, rho 16) | reported, 0.974 [0.969, 0.982] | reported, 1.014 [1.010, 1.017] | reported, 1.012 [1.007, 1.015] | reported, 0.897 [0.880, 0.911] | reported, 0.464 (heap 0.512) | reported, 0.874 (heap 0.879) |
| 4 (n 16, rho 64) | reported, 0.930 [0.922, 0.935] | reported, 1.008 [1.003, 1.011] | reported, 1.005 [1.000, 1.007] | reported, 0.875 [0.860, 0.895] | reported, 0.497 (heap 0.553) | reported, 0.887 (heap 0.892) |
| 5 (n 32, rho 1) | reported, 1.150 [1.140, 1.162] | reported, 1.005 [0.998, 1.012] | reported, 1.009 [1.003, 1.016] | reported, 0.905 [0.875, 0.936] | reported, 0.595 (heap 0.598) | reported, 0.795 (heap 0.797) |
| 6 (n 32, rho 4) | reported, 1.118 [1.108, 1.131] | reported, 1.002 [0.998, 1.007] | reported, 1.005 [1.001, 1.010] | reported, 0.946 [0.916, 0.975] | reported, 0.517 (heap 0.522) | reported, 0.813 (heap 0.814) |
| 7 (n 32, rho 16) | reported, 1.039 [1.022, 1.046] | reported, 1.006 [1.004, 1.012] | reported, 1.006 [1.004, 1.012] | reported, 0.877 [0.850, 0.916] | reported, 0.447 (heap 0.460) | reported, 0.853 (heap 0.854) |
| 8 (n 32, rho 64) | reported, 1.038 [1.028, 1.048] | reported, 1.006 [1.002, 1.009] | reported, 1.007 [1.003, 1.010] | reported, 0.953 [0.902, 0.981] | reported, 0.493 (heap 0.508) | reported, 0.876 (heap 0.877) |
| 9 (n 64, rho 1) | reported, 1.165 [1.155, 1.183] | reported, 1.001 [0.999, 1.004] | reported, 1.003 [1.001, 1.006] | reported, 0.880 [0.840, 0.944] | reported, 0.548 (heap 0.549) | reported, 0.772 (heap 0.772) |
| 10 (n 64, rho 4) | reported, 1.063 [1.047, 1.094] | reported, 1.001 [0.997, 1.010] | reported, 1.002 [0.997, 1.011] | reported, 0.970 [0.933, 1.019] | reported, 0.746 (heap 0.748) | reported, 0.891 (heap 0.891) |
| 11 (n 64, rho 16) | reported, 1.052 [1.040, 1.070] | reported, 1.001 [0.997, 1.006] | reported, 1.001 [0.998, 1.007] | reported, 0.924 [0.872, 0.961] | reported, 0.758 (heap 0.760) | reported, 0.901 (heap 0.901) |
| 12 (n 64, rho 64) | reported, 1.060 [1.045, 1.074] | reported, 1.003 [0.991, 1.006] | reported, 1.004 [0.992, 1.006] | reported, 0.949 [0.916, 0.975] | reported, 0.482 (heap 0.486) | reported, 0.870 (heap 0.870) |
| 13 (n 128, rho 1) | reported, 1.185 [1.172, 1.203] | reported, 1.002 [0.998, 1.007] | reported, 1.003 [0.999, 1.008] | reported, 1.132 [1.001, 1.236] | reported, 0.372 (heap 0.372) | reported, 0.731 (heap 0.731) |
| 14 (n 128, rho 4) | reported, 1.035 [1.024, 1.041] | reported, 1.001 [0.993, 1.005] | reported, 1.001 [0.993, 1.005] | reported, 0.971 [0.908, 1.042] | reported, 0.477 (heap 0.477) | reported, 0.858 (heap 0.858) |
| 15 (n 128, rho 16) | reported, 1.036 [1.018, 1.048] | reported, 0.999 [0.995, 1.008] | reported, 0.999 [0.995, 1.008] | reported, 0.976 [0.887, 1.062] | reported, 0.499 (heap 0.500) | reported, 0.891 (heap 0.891) |
| 16 (n 128, rho 64) | reported, 0.991 [0.970, 1.004] | reported, 0.999 [0.988, 1.005] | reported, 1.000 [0.988, 1.006] | reported, 0.933 [0.890, 1.012] | reported, 0.487 (heap 0.487) | reported, 0.893 (heap 0.893) |

### Gcd work (a check, not a verdict) and the call-for-call checks

G(row)/G(TC) MUST be exactly 1 for every hash row (rule-m2 item 7); the counting runs' per-operation records of every hash row equal TC's, every step; Hash-V-pack and Hash-V-grow-map take Hash-V-fixed's arithmetic counters and store splits (spec v0.2 §15.7 items 3-4).

| job | G per operation | G(Hash-V-fixed)/G(TC), call for call | G(Hash-V-grow-map)/G(TC), call for call | G(Hash-V-pack)/G(TC), call for call | pack vs fixed | grow-map vs fixed |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 34.35 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 2 | 34.05 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 3 | 30.12 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 4 | 31.76 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 5 | 35.64 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 6 | 35.31 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 7 | 35.52 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 8 | 35.46 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 9 | 37.33 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 10 | 36.99 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 11 | 37.11 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 12 | 35.88 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 13 | 38.52 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 14 | 38.28 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 15 | 38.83 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |
| 16 | 38.43 | 1.000000 (passed), pass | 1.000000 (passed), pass | 1.000000 (passed), pass | pass | pass |

### Strata (counts of verdicts; no measure combined, no stratum scored)

The strata are the sharing levels of the inputs (rule-m2 item 11): NONE (rho = 1), LOW (rho = 4), MID (rho = 16), HIGH (rho = 64). The run's headline measures only.

| stratum | pairing | measure | wins | ties | losses | over cap | n/a | reported | median ratio | severe flags |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| NONE | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.698 | 0 |
| NONE | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.894 | 0 |
| NONE | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| NONE | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.333 | 4 |
| NONE | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.454 | 0 |
| NONE | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.889 | 0 |
| NONE | Hash-V-grow-map vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 1.375 | 1 |
| NONE | Hash-V-grow-map vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.167 | 4 |
| NONE | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.526 | 0 |
| NONE | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.891 | 0 |
| NONE | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.714 | 0 |
| NONE | Hash-V-pack vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.622 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.736 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.997 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| NONE | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.672 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.636 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.994 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | stable | 3 | 1 | 0 | 0 | 0 | 0 | 0.578 | 0 |
| NONE | Hash-V-grow-map vs Hash-V-fixed | peak | 2 | 1 | 1 | 0 | 0 | 0 | 0.880 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.175 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.003 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.460 | 0 |
| NONE | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.752 | 0 |
| LOW | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.704 | 0 |
| LOW | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.892 | 0 |
| LOW | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| LOW | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.333 | 4 |
| LOW | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.553 | 0 |
| LOW | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.888 | 0 |
| LOW | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.750 | 0 |
| LOW | Hash-V-grow-map vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.667 | 0 |
| LOW | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.602 | 0 |
| LOW | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| LOW | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.365 | 0 |
| LOW | Hash-V-pack vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.345 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.854 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.997 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.154 | 0 |
| LOW | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.576 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.784 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.996 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.317 | 0 |
| LOW | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.712 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.076 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.002 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.497 | 0 |
| LOW | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.835 | 0 |
| MID | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.752 | 0 |
| MID | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.894 | 0 |
| MID | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| MID | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.333 | 4 |
| MID | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.644 | 0 |
| MID | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| MID | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.375 | 0 |
| MID | Hash-V-grow-map vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.417 | 0 |
| MID | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.673 | 0 |
| MID | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.893 | 0 |
| MID | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.227 | 0 |
| MID | Hash-V-pack vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.257 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.890 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.998 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.095 | 0 |
| MID | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.539 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.865 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.995 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.167 | 0 |
| MID | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.607 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.038 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.003 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.481 | 0 |
| MID | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.882 | 0 |
| HIGH | Hash-V-fixed vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.752 | 0 |
| HIGH | Hash-V-fixed vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.885 | 0 |
| HIGH | Hash-V-fixed vs TC | stable | 0 | 0 | 4 | 0 | 0 | 0 | 2.250 | 4 |
| HIGH | Hash-V-fixed vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 2.333 | 4 |
| HIGH | Hash-V-grow-map vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.655 | 0 |
| HIGH | Hash-V-grow-map vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.884 | 0 |
| HIGH | Hash-V-grow-map vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.281 | 0 |
| HIGH | Hash-V-grow-map vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.354 | 0 |
| HIGH | Hash-V-pack vs TC | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.687 | 0 |
| HIGH | Hash-V-pack vs TC | operation | 0 | 0 | 4 | 0 | 0 | 0 | 0.888 | 0 |
| HIGH | Hash-V-pack vs TC | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.139 | 0 |
| HIGH | Hash-V-pack vs TC | peak | 0 | 0 | 4 | 0 | 0 | 0 | 1.194 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | build | 0 | 0 | 4 | 0 | 0 | 0 | 0.907 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 1.001 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.062 | 0 |
| HIGH | Hash-V-pack vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.512 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | build | 0 | 1 | 3 | 0 | 0 | 0 | 0.901 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | operation | 0 | 4 | 0 | 0 | 0 | 0 | 0.998 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | stable | 4 | 0 | 0 | 0 | 0 | 0 | 0.125 | 0 |
| HIGH | Hash-V-grow-map vs Hash-V-fixed | peak | 4 | 0 | 0 | 0 | 0 | 0 | 0.580 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | build | 0 | 0 | 0 | 0 | 0 | 4 | 1.014 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | operation | 0 | 0 | 0 | 0 | 0 | 4 | 1.005 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | stable | 0 | 0 | 0 | 0 | 0 | 4 | 0.490 | 0 |
| HIGH | Hash-V-pack vs Hash-V-grow-map | peak | 0 | 0 | 0 | 0 | 0 | 4 | 0.881 | 0 |

### Size x sharing grid (ratios; presented, not judged)

Hash-V-fixed against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.635 | 0.643 | 0.655 | 0.679 |
| 32 | 0.715 | 0.690 | 0.754 | 0.749 |
| 64 | 0.713 | 0.719 | 0.758 | 0.766 |
| 128 | 0.683 | 0.720 | 0.750 | 0.755 |

Hash-V-fixed against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.873 | 0.870 | 0.843 | 0.837 |
| 32 | 0.890 | 0.889 | 0.890 | 0.878 |
| 64 | 0.897 | 0.895 | 0.897 | 0.891 |
| 128 | 0.900 | 0.898 | 0.906 | 0.901 |

Hash-V-fixed against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 2.250 | 2.250 | 2.250 |
| 32 | 2.250 | 2.250 | 2.250 | 2.250 |
| 64 | 2.250 | 2.250 | 2.250 | 2.250 |
| 128 | 2.500 | 2.500 | 2.500 | 2.500 |

Hash-V-fixed against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.333 | 2.333 | 2.333 | 2.333 |
| 32 | 2.333 | 2.333 | 2.333 | 2.333 |
| 64 | 2.333 | 2.333 | 2.333 | 2.333 |
| 128 | 2.583 | 2.583 | 2.583 | 2.583 |

Hash-V-grow-map against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.312 | 0.479 | 0.578 | 0.635 |
| 32 | 0.449 | 0.521 | 0.640 | 0.650 |
| 64 | 0.459 | 0.584 | 0.647 | 0.661 |
| 128 | 0.464 | 0.609 | 0.657 | 0.713 |

Hash-V-grow-map against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.851 | 0.856 | 0.825 | 0.824 |
| 32 | 0.880 | 0.882 | 0.885 | 0.874 |
| 64 | 0.897 | 0.894 | 0.896 | 0.893 |
| 128 | 0.899 | 0.897 | 0.903 | 0.902 |

Hash-V-grow-map against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.250 | 0.750 | 0.375 | 0.281 |
| 32 | 1.250 | 0.750 | 0.375 | 0.281 |
| 64 | 1.250 | 0.500 | 0.375 | 0.281 |
| 128 | 1.500 | 0.750 | 0.562 | 0.531 |

Hash-V-grow-map against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 2.667 | 1.667 | 1.417 | 1.354 |
| 32 | 2.000 | 1.667 | 1.417 | 1.354 |
| 64 | 2.000 | 1.500 | 1.417 | 1.354 |
| 128 | 2.333 | 1.833 | 1.708 | 1.688 |

Hash-V-pack against TC, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.443 | 0.521 | 0.563 | 0.591 |
| 32 | 0.516 | 0.583 | 0.665 | 0.674 |
| 64 | 0.535 | 0.621 | 0.680 | 0.700 |
| 128 | 0.549 | 0.630 | 0.681 | 0.707 |

Hash-V-pack against TC, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.865 | 0.865 | 0.836 | 0.831 |
| 32 | 0.885 | 0.884 | 0.890 | 0.880 |
| 64 | 0.897 | 0.896 | 0.896 | 0.895 |
| 128 | 0.901 | 0.898 | 0.902 | 0.901 |

Hash-V-pack against TC, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.803 | 0.299 | 0.174 | 0.140 |
| 32 | 0.744 | 0.387 | 0.168 | 0.139 |
| 64 | 0.685 | 0.373 | 0.284 | 0.136 |
| 128 | 0.558 | 0.358 | 0.281 | 0.259 |

Hash-V-pack against TC, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.654 | 1.319 | 1.238 | 1.201 |
| 32 | 1.590 | 1.354 | 1.208 | 1.186 |
| 64 | 1.544 | 1.336 | 1.277 | 1.178 |
| 128 | 1.707 | 1.573 | 1.522 | 1.507 |

Hash-V-pack against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.698 | 0.810 | 0.860 | 0.870 |
| 32 | 0.721 | 0.844 | 0.882 | 0.900 |
| 64 | 0.751 | 0.864 | 0.898 | 0.914 |
| 128 | 0.804 | 0.874 | 0.908 | 0.936 |

Hash-V-pack against Hash-V-fixed, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.990 | 0.994 | 0.993 | 0.992 |
| 32 | 0.994 | 0.995 | 0.999 | 1.002 |
| 64 | 1.000 | 1.000 | 1.000 | 1.005 |
| 128 | 1.001 | 1.000 | 0.996 | 1.000 |

Hash-V-pack against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.133 | 0.077 | 0.062 |
| 32 | 0.331 | 0.172 | 0.075 | 0.062 |
| 64 | 0.304 | 0.166 | 0.126 | 0.060 |
| 128 | 0.223 | 0.143 | 0.112 | 0.103 |

Hash-V-pack against Hash-V-fixed, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.709 | 0.565 | 0.530 | 0.515 |
| 32 | 0.682 | 0.580 | 0.518 | 0.508 |
| 64 | 0.662 | 0.573 | 0.547 | 0.505 |
| 128 | 0.661 | 0.609 | 0.589 | 0.583 |

Hash-V-grow-map against Hash-V-fixed, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.492 | 0.744 | 0.883 | 0.935 |
| 32 | 0.627 | 0.755 | 0.849 | 0.868 |
| 64 | 0.644 | 0.813 | 0.854 | 0.863 |
| 128 | 0.679 | 0.845 | 0.876 | 0.944 |

Hash-V-grow-map against Hash-V-fixed, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.975 | 0.984 | 0.979 | 0.984 |
| 32 | 0.989 | 0.993 | 0.993 | 0.996 |
| 64 | 0.999 | 0.999 | 0.999 | 1.002 |
| 128 | 1.000 | 0.999 | 0.997 | 1.001 |

Hash-V-grow-map against Hash-V-fixed, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.000 | 0.333 | 0.167 | 0.125 |
| 32 | 0.556 | 0.333 | 0.167 | 0.125 |
| 64 | 0.556 | 0.222 | 0.167 | 0.125 |
| 128 | 0.600 | 0.300 | 0.225 | 0.212 |

Hash-V-grow-map against Hash-V-fixed, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.143 | 0.714 | 0.607 | 0.580 |
| 32 | 0.857 | 0.714 | 0.607 | 0.580 |
| 64 | 0.857 | 0.643 | 0.607 | 0.580 |
| 128 | 0.903 | 0.710 | 0.661 | 0.653 |

Hash-V-pack against Hash-V-grow-map, 1 build time of one input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.418 | 1.088 | 0.974 | 0.930 |
| 32 | 1.150 | 1.118 | 1.039 | 1.038 |
| 64 | 1.165 | 1.063 | 1.052 | 1.060 |
| 128 | 1.185 | 1.035 | 1.036 | 0.991 |

Hash-V-pack against Hash-V-grow-map, 3 multiplication time:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 1.016 | 1.010 | 1.014 | 1.008 |
| 32 | 1.005 | 1.002 | 1.006 | 1.006 |
| 64 | 1.001 | 1.001 | 1.001 | 1.003 |
| 128 | 1.002 | 1.001 | 0.999 | 0.999 |

Hash-V-pack against Hash-V-grow-map, 6 stable bytes of one stored input matrix:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.357 | 0.398 | 0.464 | 0.497 |
| 32 | 0.595 | 0.517 | 0.447 | 0.493 |
| 64 | 0.548 | 0.746 | 0.758 | 0.482 |
| 128 | 0.372 | 0.477 | 0.499 | 0.487 |

Hash-V-pack against Hash-V-grow-map, 8 peak bytes during the operation:

| n | rho = 1 | rho = 4 | rho = 16 | rho = 64 |
| --- | --- | --- | --- | --- |
| 16 | 0.620 | 0.791 | 0.874 | 0.887 |
| 32 | 0.795 | 0.813 | 0.853 | 0.876 |
| 64 | 0.772 | 0.891 | 0.901 | 0.870 |
| 128 | 0.731 | 0.858 | 0.891 | 0.893 |

## Memory, reported (bytes per cell; accounted, heap in parentheses)

From the memory runs (memory build). After creation: the inputs, over their cells; at the end: the inputs and the result (sop, pos: the intermediates freed), over their cells; the operation's peak over the operation's cells (3 n^2); each input's stable bytes, and its build peak: the most the job's accounted total rose above its value before that input's create, during the build, over that input's cells; Hash-V-grow-map's growth peak: the job's total at its largest growth, over the operation's cells.

| job | row | after creation | end | operation peak | A's stable (build peak) | B's stable (build peak) | growth peak |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 1 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 37.33 (37.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 1 | Hash-V-grow-map | 36.00 (36.47) | 36.00 (36.47) | 42.67 (43.16) | 36.00 (56.00) | 36.00 (56.00) | 42.67 (43.16), 18 growths |
| 1 | Hash-V-pack | 12.84 (13.34) | 13.14 (13.64) | 26.47 (26.99) | 12.84 (52.84) | 12.84 (52.84) | - |
| 2 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 2 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 37.33 (37.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 2 | Hash-V-grow-map | 12.00 (12.47) | 20.00 (20.47) | 26.67 (27.16) | 12.00 (17.00) | 12.00 (17.00) | 26.67 (27.16), 14 growths |
| 2 | Hash-V-pack | 4.78 (5.31) | 7.77 (8.30) | 21.10 (21.66) | 4.69 (46.69) | 4.88 (46.88) | - |
| 3 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 3 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 37.33 (37.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 3 | Hash-V-grow-map | 6.00 (6.47) | 16.00 (16.47) | 22.67 (23.16) | 6.00 (7.25) | 6.00 (7.25) | 22.67 (23.16), 10 growths |
| 3 | Hash-V-pack | 2.78 (3.31) | 6.47 (6.99) | 19.80 (20.34) | 2.75 (44.75) | 2.81 (44.81) | - |
| 4 | TC | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.31) | 16.00 (16.00) | 16.00 (16.00) | - |
| 4 | Hash-V-fixed | 36.00 (36.47) | 36.00 (36.47) | 37.33 (37.81) | 36.00 (36.00) | 36.00 (36.00) | - |
| 4 | Hash-V-grow-map | 4.50 (4.97) | 15.00 (15.47) | 21.67 (22.16) | 4.50 (4.50) | 4.50 (4.50) | 21.67 (22.16), 6 growths |
| 4 | Hash-V-pack | 2.23 (2.75) | 5.89 (6.41) | 19.22 (19.76) | 2.22 (44.22) | 2.25 (44.25) | - |
| 5 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 5 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 37.33 (37.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 5 | Hash-V-grow-map | 20.00 (20.12) | 25.33 (25.45) | 32.00 (32.12) | 20.00 (30.00) | 20.00 (30.00) | 32.00 (32.12), 22 growths |
| 5 | Hash-V-pack | 11.90 (12.03) | 12.11 (12.25) | 25.45 (25.59) | 11.86 (51.86) | 11.94 (51.94) | - |
| 6 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 6 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 37.33 (37.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 6 | Hash-V-grow-map | 12.00 (12.12) | 20.00 (20.12) | 26.67 (26.79) | 12.00 (17.00) | 12.00 (17.00) | 26.67 (26.79), 20 growths |
| 6 | Hash-V-pack | 6.20 (6.33) | 8.34 (8.47) | 21.67 (21.80) | 6.21 (46.21) | 6.19 (46.19) | - |
| 7 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 7 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 37.33 (37.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 7 | Hash-V-grow-map | 6.00 (6.12) | 16.00 (16.12) | 22.67 (22.79) | 6.00 (7.25) | 6.00 (7.25) | 22.67 (22.79), 16 growths |
| 7 | Hash-V-pack | 2.68 (2.81) | 5.99 (6.12) | 19.33 (19.46) | 2.70 (44.70) | 2.67 (44.67) | - |
| 8 | TC | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.08) | 16.00 (16.00) | 16.00 (16.00) | - |
| 8 | Hash-V-fixed | 36.00 (36.12) | 36.00 (36.12) | 37.33 (37.45) | 36.00 (36.00) | 36.00 (36.00) | - |
| 8 | Hash-V-grow-map | 4.50 (4.62) | 15.00 (15.12) | 21.67 (21.79) | 4.50 (4.81) | 4.50 (4.81) | 21.67 (21.79), 12 growths |
| 8 | Hash-V-pack | 2.22 (2.34) | 5.64 (5.77) | 18.97 (19.11) | 2.23 (44.23) | 2.21 (44.21) | - |
| 9 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 9 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 37.33 (37.36) | 36.00 (36.00) | 36.00 (36.00) | - |
| 9 | Hash-V-grow-map | 20.00 (20.03) | 25.33 (25.36) | 32.00 (32.03) | 20.00 (30.00) | 20.00 (30.00) | 32.00 (32.03), 28 growths |
| 9 | Hash-V-pack | 10.96 (10.99) | 11.36 (11.40) | 24.70 (24.73) | 10.90 (50.90) | 11.01 (51.01) | - |
| 10 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 10 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 37.33 (37.36) | 36.00 (36.00) | 36.00 (36.00) | - |
| 10 | Hash-V-grow-map | 8.00 (8.03) | 17.33 (17.36) | 24.00 (24.03) | 8.00 (10.50) | 8.00 (10.50) | 24.00 (24.03), 24 growths |
| 10 | Hash-V-pack | 5.97 (6.00) | 8.04 (8.07) | 21.38 (21.41) | 5.98 (45.98) | 5.96 (45.96) | - |
| 11 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 11 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 37.33 (37.36) | 36.00 (36.00) | 36.00 (36.00) | - |
| 11 | Hash-V-grow-map | 6.00 (6.03) | 16.00 (16.03) | 22.67 (22.70) | 6.00 (7.25) | 6.00 (7.25) | 22.67 (22.70), 22 growths |
| 11 | Hash-V-pack | 4.55 (4.58) | 7.09 (7.13) | 20.43 (20.46) | 4.55 (44.55) | 4.55 (44.55) | - |
| 12 | TC | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.02) | 16.00 (16.00) | 16.00 (16.00) | - |
| 12 | Hash-V-fixed | 36.00 (36.03) | 36.00 (36.03) | 37.33 (37.36) | 36.00 (36.00) | 36.00 (36.00) | - |
| 12 | Hash-V-grow-map | 4.50 (4.53) | 15.00 (15.03) | 21.67 (21.70) | 4.50 (4.81) | 4.50 (4.81) | 21.67 (21.70), 18 growths |
| 12 | Hash-V-pack | 2.17 (2.20) | 5.51 (5.55) | 18.85 (18.88) | 2.18 (44.18) | 2.16 (44.16) | - |
| 13 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 13 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 41.33 (41.34) | 40.00 (40.00) | 40.00 (40.00) | - |
| 13 | Hash-V-grow-map | 24.00 (24.01) | 29.33 (29.34) | 37.33 (37.34) | 24.00 (36.00) | 24.00 (36.00) | 37.33 (37.34), 34 growths |
| 13 | Hash-V-pack | 8.93 (8.94) | 9.97 (9.98) | 27.31 (27.31) | 8.94 (60.94) | 8.93 (60.93) | - |
| 14 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 14 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 41.33 (41.34) | 40.00 (40.00) | 40.00 (40.00) | - |
| 14 | Hash-V-grow-map | 12.00 (12.01) | 21.33 (21.34) | 29.33 (29.34) | 12.00 (15.00) | 12.00 (15.00) | 29.33 (29.34), 30 growths |
| 14 | Hash-V-pack | 5.72 (5.73) | 7.83 (7.84) | 25.17 (25.17) | 5.72 (57.72) | 5.73 (57.73) | - |
| 15 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 15 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 41.33 (41.34) | 40.00 (40.00) | 40.00 (40.00) | - |
| 15 | Hash-V-grow-map | 9.00 (9.01) | 19.33 (19.34) | 27.33 (27.34) | 9.00 (9.75) | 9.00 (9.75) | 27.33 (27.34), 26 growths |
| 15 | Hash-V-pack | 4.49 (4.50) | 7.01 (7.02) | 24.35 (24.36) | 4.49 (56.49) | 4.50 (56.50) | - |
| 16 | TC | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | 16.00 (16.00) | - |
| 16 | Hash-V-fixed | 40.00 (40.01) | 40.00 (40.01) | 41.33 (41.34) | 40.00 (40.00) | 40.00 (40.00) | - |
| 16 | Hash-V-grow-map | 8.50 (8.51) | 19.00 (19.01) | 27.00 (27.01) | 8.50 (8.88) | 8.50 (8.88) | 27.00 (27.01), 24 growths |
| 16 | Hash-V-pack | 4.14 (4.15) | 6.78 (6.79) | 24.11 (24.12) | 4.14 (56.14) | 4.14 (56.14) | - |

## Build time per input matrix (reported, no verdict)

The median over rounds of each input matrix's build time per run, ns.

| job | row | A | B |
| --- | --- | --- | --- |
| 1 | TC | 6231 | 6102 |
| 1 | Hash-V-fixed | 9733 | 9699 |
| 1 | Hash-V-grow-map | 19910 | 19690 |
| 1 | Hash-V-pack | 13978 | 13768 |
| 1 | Hash-V-fixed-null | 9785 | 9748 |
| 2 | TC | 6390 | 6227 |
| 2 | Hash-V-fixed | 9868 | 9709 |
| 2 | Hash-V-grow-map | 13248 | 13035 |
| 2 | Hash-V-pack | 12077 | 11991 |
| 2 | Hash-V-fixed-null | 9913 | 9797 |
| 3 | TC | 5616 | 5380 |
| 3 | Hash-V-fixed | 8281 | 8283 |
| 3 | Hash-V-grow-map | 9350 | 9439 |
| 3 | Hash-V-pack | 9641 | 9636 |
| 3 | Hash-V-fixed-null | 8334 | 8324 |
| 4 | TC | 5177 | 5043 |
| 4 | Hash-V-fixed | 7637 | 7489 |
| 4 | Hash-V-grow-map | 8181 | 7907 |
| 4 | Hash-V-pack | 8775 | 8631 |
| 4 | Hash-V-fixed-null | 7587 | 7464 |
| 5 | TC | 29240 | 29258 |
| 5 | Hash-V-fixed | 40928 | 40741 |
| 5 | Hash-V-grow-map | 65723 | 65038 |
| 5 | Hash-V-pack | 57039 | 56307 |
| 5 | Hash-V-fixed-null | 40966 | 40810 |
| 6 | TC | 28707 | 27803 |
| 6 | Hash-V-fixed | 41326 | 40616 |
| 6 | Hash-V-grow-map | 54653 | 54231 |
| 6 | Hash-V-pack | 49106 | 47455 |
| 6 | Hash-V-fixed-null | 41422 | 40495 |
| 7 | TC | 28105 | 28837 |
| 7 | Hash-V-fixed | 37892 | 38136 |
| 7 | Hash-V-grow-map | 43221 | 45492 |
| 7 | Hash-V-pack | 43399 | 42995 |
| 7 | Hash-V-fixed-null | 38010 | 38135 |
| 8 | TC | 24467 | 25855 |
| 8 | Hash-V-fixed | 32784 | 34298 |
| 8 | Hash-V-grow-map | 39661 | 38476 |
| 8 | Hash-V-pack | 37584 | 38113 |
| 8 | Hash-V-fixed-null | 32783 | 34218 |
| 9 | TC | 118290 | 117144 |
| 9 | Hash-V-fixed | 166788 | 163707 |
| 9 | Hash-V-grow-map | 260103 | 255810 |
| 9 | Hash-V-pack | 223890 | 218671 |
| 9 | Hash-V-fixed-null | 166478 | 163882 |
| 10 | TC | 119673 | 118512 |
| 10 | Hash-V-fixed | 165869 | 164922 |
| 10 | Hash-V-grow-map | 205471 | 201855 |
| 10 | Hash-V-pack | 191605 | 190736 |
| 10 | Hash-V-fixed-null | 165126 | 164663 |
| 11 | TC | 117680 | 116592 |
| 11 | Hash-V-fixed | 154784 | 153900 |
| 11 | Hash-V-grow-map | 182796 | 180764 |
| 11 | Hash-V-pack | 173357 | 171644 |
| 11 | Hash-V-fixed-null | 154418 | 153904 |
| 12 | TC | 114586 | 114981 |
| 12 | Hash-V-fixed | 151250 | 149430 |
| 12 | Hash-V-grow-map | 175062 | 172858 |
| 12 | Hash-V-pack | 164853 | 163142 |
| 12 | Hash-V-fixed-null | 150300 | 148960 |
| 13 | TC | 480653 | 476482 |
| 13 | Hash-V-fixed | 701220 | 689701 |
| 13 | Hash-V-grow-map | 1029927 | 1034040 |
| 13 | Hash-V-pack | 875078 | 862897 |
| 13 | Hash-V-fixed-null | 697877 | 698008 |
| 14 | TC | 472916 | 473024 |
| 14 | Hash-V-fixed | 656158 | 662028 |
| 14 | Hash-V-grow-map | 766744 | 778702 |
| 14 | Hash-V-pack | 750451 | 751066 |
| 14 | Hash-V-fixed-null | 652079 | 659811 |
| 15 | TC | 475726 | 476814 |
| 15 | Hash-V-fixed | 639666 | 631786 |
| 15 | Hash-V-grow-map | 726196 | 724983 |
| 15 | Hash-V-pack | 703476 | 691640 |
| 15 | Hash-V-fixed-null | 639088 | 634948 |
| 16 | TC | 468776 | 472795 |
| 16 | Hash-V-fixed | 621175 | 623554 |
| 16 | Hash-V-grow-map | 662511 | 650210 |
| 16 | Hash-V-pack | 666168 | 664057 |
| 16 | Hash-V-fixed-null | 622074 | 622374 |

## Stores, per job and matrix (rule-m2 item 13)

From the counting runs, each matrix at its end (an intermediate just before it was freed). No slot is reserved: W counts the whole numbers stored. Hash-V-pack's T is its build table's.

| job | row | matrix | W | T | fill | w | w_h | stores (found / new) | probes per store | longest | widenings (slots copied) | growths | reinserted (probes) | fields rewritten | cells bytes | table or list bytes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | Hash-V-fixed | A | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.08 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.05 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-fixed | C | 311 | 1024 | 30.4% | 64 | 16 | 201 / 311 | 1.14 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 1 | Hash-V-grow-map | A | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.37 | 7 | 0 (0) | 6 | 504 (576) | 821 | 1024 | 8192 |
| 1 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.37 | 8 | 0 (0) | 6 | 504 (561) | 816 | 1024 | 8192 |
| 1 | Hash-V-grow-map | C | 311 | 1024 | 30.4% | 64 | 16 | 201 / 311 | 1.46 | 9 | 1 (16) | 6 | 504 (595) | 750 | 1024 | 8192 |
| 1 | Hash-V-pack | A | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.08 | 5 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2264 |
| 1 | Hash-V-pack | B | 283 | 1024 | 27.6% | 64 | 16 | 229 / 283 | 1.05 | 3 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 2264 |
| 1 | Hash-V-pack | C | 311 | 1024 | 30.4% | 64 | 16 | 201 / 311 | 1.14 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2488 |
| 2 | Hash-V-fixed | A | 86 | 1024 | 8.4% | 64 | 16 | 426 / 86 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | B | 92 | 1024 | 9.0% | 64 | 16 | 420 / 92 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-fixed | C | 312 | 1024 | 30.5% | 64 | 16 | 200 / 312 | 1.15 | 7 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 2 | Hash-V-grow-map | A | 86 | 256 | 33.6% | 64 | 16 | 426 / 86 | 1.29 | 5 | 0 (0) | 4 | 120 (141) | 200 | 1024 | 2048 |
| 2 | Hash-V-grow-map | B | 92 | 256 | 35.9% | 64 | 16 | 420 / 92 | 1.28 | 7 | 0 (0) | 4 | 120 (138) | 235 | 1024 | 2048 |
| 2 | Hash-V-grow-map | C | 312 | 1024 | 30.5% | 64 | 16 | 200 / 312 | 1.45 | 10 | 1 (16) | 6 | 504 (575) | 760 | 1024 | 8192 |
| 2 | Hash-V-pack | A | 86 | 1024 | 8.4% | 64 | 8 | 426 / 86 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 688 |
| 2 | Hash-V-pack | B | 92 | 1024 | 9.0% | 64 | 8 | 420 / 92 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 512 | 736 |
| 2 | Hash-V-pack | C | 312 | 1024 | 30.5% | 64 | 16 | 200 / 312 | 1.15 | 7 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2496 |
| 3 | Hash-V-fixed | A | 24 | 1024 | 2.3% | 64 | 16 | 488 / 24 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | B | 26 | 1024 | 2.5% | 64 | 16 | 486 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-fixed | C | 315 | 1024 | 30.8% | 64 | 16 | 197 / 315 | 1.13 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 3 | Hash-V-grow-map | A | 24 | 64 | 37.5% | 64 | 16 | 488 / 24 | 1.19 | 4 | 0 (0) | 2 | 24 (26) | 64 | 1024 | 512 |
| 3 | Hash-V-grow-map | B | 26 | 64 | 40.6% | 64 | 16 | 486 / 26 | 1.18 | 7 | 0 (0) | 2 | 24 (26) | 41 | 1024 | 512 |
| 3 | Hash-V-grow-map | C | 315 | 1024 | 30.8% | 64 | 16 | 197 / 315 | 1.43 | 13 | 1 (16) | 6 | 504 (567) | 737 | 1024 | 8192 |
| 3 | Hash-V-pack | A | 24 | 1024 | 2.3% | 64 | 8 | 488 / 24 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 192 |
| 3 | Hash-V-pack | B | 26 | 1024 | 2.5% | 64 | 8 | 486 / 26 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 208 |
| 3 | Hash-V-pack | C | 315 | 1024 | 30.8% | 64 | 16 | 197 / 315 | 1.13 | 6 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2520 |
| 4 | Hash-V-fixed | A | 7 | 1024 | 0.7% | 64 | 16 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | B | 8 | 1024 | 0.8% | 64 | 16 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-fixed | C | 294 | 1024 | 28.7% | 64 | 16 | 218 / 294 | 1.11 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 8192 |
| 4 | Hash-V-grow-map | A | 7 | 16 | 43.8% | 64 | 16 | 505 / 7 | 1.09 | 2 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | B | 8 | 16 | 50.0% | 64 | 16 | 504 / 8 | 1.35 | 4 | 0 (0) | 0 | 0 (0) | 0 | 1024 | 128 |
| 4 | Hash-V-grow-map | C | 294 | 1024 | 28.7% | 64 | 16 | 218 / 294 | 1.42 | 7 | 1 (16) | 6 | 504 (585) | 806 | 1024 | 8192 |
| 4 | Hash-V-pack | A | 7 | 1024 | 0.7% | 64 | 8 | 505 / 7 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 56 |
| 4 | Hash-V-pack | B | 8 | 1024 | 0.8% | 64 | 8 | 504 / 8 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 512 | 64 |
| 4 | Hash-V-pack | C | 294 | 1024 | 28.7% | 64 | 16 | 218 / 294 | 1.11 | 4 | 1 (1024) | 0 | 0 (0) | 0 | 1024 | 2352 |
| 5 | Hash-V-fixed | A | 1006 | 4096 | 24.6% | 64 | 16 | 1042 / 1006 | 1.06 | 8 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | B | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-fixed | C | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 5 | Hash-V-grow-map | A | 1006 | 2048 | 49.1% | 64 | 16 | 1042 / 1006 | 1.35 | 11 | 0 (0) | 7 | 1016 (1153) | 1805 | 4096 | 16384 |
| 5 | Hash-V-grow-map | B | 1016 | 2048 | 49.6% | 64 | 16 | 1032 / 1016 | 1.43 | 24 | 0 (0) | 7 | 1016 (1146) | 1780 | 4096 | 16384 |
| 5 | Hash-V-grow-map | C | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.46 | 12 | 1 (16) | 8 | 2040 (2430) | 3567 | 4096 | 32768 |
| 5 | Hash-V-pack | A | 1006 | 4096 | 24.6% | 64 | 16 | 1042 / 1006 | 1.06 | 8 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8048 |
| 5 | Hash-V-pack | B | 1016 | 4096 | 24.8% | 64 | 16 | 1032 / 1016 | 1.06 | 6 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 8128 |
| 5 | Hash-V-pack | C | 1094 | 4096 | 26.7% | 64 | 16 | 954 / 1094 | 1.11 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8752 |
| 6 | Hash-V-fixed | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | B | 280 | 4096 | 6.8% | 64 | 16 | 1768 / 280 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-fixed | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.09 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 6 | Hash-V-grow-map | A | 283 | 1024 | 27.6% | 64 | 16 | 1765 / 283 | 1.22 | 9 | 0 (0) | 6 | 504 (586) | 1479 | 4096 | 8192 |
| 6 | Hash-V-grow-map | B | 280 | 1024 | 27.3% | 64 | 16 | 1768 / 280 | 1.32 | 36 | 0 (0) | 6 | 504 (582) | 1542 | 4096 | 8192 |
| 6 | Hash-V-grow-map | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.35 | 11 | 1 (16) | 8 | 2040 (2339) | 3547 | 4096 | 32768 |
| 6 | Hash-V-pack | A | 283 | 4096 | 6.9% | 64 | 16 | 1765 / 283 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2264 |
| 6 | Hash-V-pack | B | 280 | 4096 | 6.8% | 64 | 16 | 1768 / 280 | 1.02 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 2240 |
| 6 | Hash-V-pack | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.09 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8816 |
| 7 | Hash-V-fixed | A | 89 | 4096 | 2.2% | 64 | 16 | 1959 / 89 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | B | 86 | 4096 | 2.1% | 64 | 16 | 1962 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-fixed | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 7 | Hash-V-grow-map | A | 89 | 256 | 34.8% | 64 | 16 | 1959 / 89 | 1.12 | 5 | 0 (0) | 4 | 120 (130) | 225 | 4096 | 2048 |
| 7 | Hash-V-grow-map | B | 86 | 256 | 33.6% | 64 | 16 | 1962 / 86 | 1.23 | 8 | 0 (0) | 4 | 120 (140) | 226 | 4096 | 2048 |
| 7 | Hash-V-grow-map | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.50 | 19 | 1 (16) | 8 | 2040 (2383) | 3563 | 4096 | 32768 |
| 7 | Hash-V-pack | A | 89 | 4096 | 2.2% | 64 | 8 | 1959 / 89 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 712 |
| 7 | Hash-V-pack | B | 86 | 4096 | 2.1% | 64 | 8 | 1962 / 86 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 688 |
| 7 | Hash-V-pack | C | 1102 | 4096 | 26.9% | 64 | 16 | 946 / 1102 | 1.10 | 6 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8816 |
| 8 | Hash-V-fixed | A | 29 | 4096 | 0.7% | 64 | 16 | 2019 / 29 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | B | 27 | 4096 | 0.7% | 64 | 16 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-fixed | C | 1086 | 4096 | 26.5% | 64 | 16 | 962 / 1086 | 1.08 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 32768 |
| 8 | Hash-V-grow-map | A | 29 | 64 | 45.3% | 64 | 16 | 2019 / 29 | 1.34 | 7 | 0 (0) | 2 | 24 (26) | 44 | 4096 | 512 |
| 8 | Hash-V-grow-map | B | 27 | 64 | 42.2% | 64 | 16 | 2021 / 27 | 1.23 | 5 | 0 (0) | 2 | 24 (26) | 38 | 4096 | 512 |
| 8 | Hash-V-grow-map | C | 1086 | 4096 | 26.5% | 64 | 16 | 962 / 1086 | 1.38 | 11 | 1 (16) | 8 | 2040 (2333) | 3672 | 4096 | 32768 |
| 8 | Hash-V-pack | A | 29 | 4096 | 0.7% | 64 | 8 | 2019 / 29 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 232 |
| 8 | Hash-V-pack | B | 27 | 4096 | 0.7% | 64 | 8 | 2021 / 27 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 2048 | 216 |
| 8 | Hash-V-pack | C | 1086 | 4096 | 26.5% | 64 | 16 | 962 / 1086 | 1.08 | 5 | 1 (4096) | 0 | 0 (0) | 0 | 4096 | 8688 |
| 9 | Hash-V-fixed | A | 3533 | 16384 | 21.6% | 64 | 16 | 4659 / 3533 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | B | 3589 | 16384 | 21.9% | 64 | 16 | 4603 / 3589 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-fixed | C | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 9 | Hash-V-grow-map | A | 3533 | 8192 | 43.1% | 64 | 16 | 4659 / 3533 | 1.27 | 21 | 0 (0) | 9 | 4088 (4555) | 8338 | 16384 | 65536 |
| 9 | Hash-V-grow-map | B | 3589 | 8192 | 43.8% | 64 | 16 | 4603 / 3589 | 1.24 | 12 | 0 (0) | 9 | 4088 (4566) | 8175 | 16384 | 65536 |
| 9 | Hash-V-grow-map | C | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.43 | 20 | 1 (16) | 10 | 8184 (9653) | 15557 | 16384 | 131072 |
| 9 | Hash-V-pack | A | 3533 | 16384 | 21.6% | 64 | 16 | 4659 / 3533 | 1.03 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28264 |
| 9 | Hash-V-pack | B | 3589 | 16384 | 21.9% | 64 | 16 | 4603 / 3589 | 1.02 | 4 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 28712 |
| 9 | Hash-V-pack | C | 4188 | 16384 | 25.6% | 64 | 16 | 4004 / 4188 | 1.10 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33504 |
| 10 | Hash-V-fixed | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | B | 1006 | 16384 | 6.1% | 64 | 16 | 7186 / 1006 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-fixed | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 10 | Hash-V-grow-map | A | 1013 | 2048 | 49.5% | 64 | 16 | 7179 / 1013 | 1.21 | 11 | 0 (0) | 7 | 1016 (1154) | 2189 | 16384 | 16384 |
| 10 | Hash-V-grow-map | B | 1006 | 2048 | 49.1% | 64 | 16 | 7186 / 1006 | 1.22 | 12 | 0 (0) | 7 | 1016 (1155) | 2203 | 16384 | 16384 |
| 10 | Hash-V-grow-map | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.50 | 20 | 1 (16) | 10 | 8184 (9532) | 15565 | 16384 | 131072 |
| 10 | Hash-V-pack | A | 1013 | 16384 | 6.2% | 64 | 16 | 7179 / 1013 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8104 |
| 10 | Hash-V-pack | B | 1006 | 16384 | 6.1% | 64 | 16 | 7186 / 1006 | 1.01 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 8048 |
| 10 | Hash-V-pack | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 7 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33512 |
| 11 | Hash-V-fixed | A | 280 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | B | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-fixed | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 11 | Hash-V-grow-map | A | 280 | 1024 | 27.3% | 64 | 16 | 7912 / 280 | 1.16 | 13 | 0 (0) | 6 | 504 (620) | 1484 | 16384 | 8192 |
| 11 | Hash-V-grow-map | B | 283 | 1024 | 27.6% | 64 | 16 | 7909 / 283 | 1.15 | 12 | 0 (0) | 6 | 504 (583) | 1836 | 16384 | 8192 |
| 11 | Hash-V-grow-map | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.44 | 19 | 1 (16) | 10 | 8184 (9559) | 15587 | 16384 | 131072 |
| 11 | Hash-V-pack | A | 280 | 16384 | 1.7% | 64 | 16 | 7912 / 280 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2240 |
| 11 | Hash-V-pack | B | 283 | 16384 | 1.7% | 64 | 16 | 7909 / 283 | 1.00 | 2 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 2264 |
| 11 | Hash-V-pack | C | 4189 | 16384 | 25.6% | 64 | 16 | 4003 / 4189 | 1.09 | 9 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33512 |
| 12 | Hash-V-fixed | A | 90 | 16384 | 0.5% | 64 | 16 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | B | 84 | 16384 | 0.5% | 64 | 16 | 8108 / 84 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-fixed | C | 4198 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.09 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 131072 |
| 12 | Hash-V-grow-map | A | 90 | 256 | 35.2% | 64 | 16 | 8102 / 90 | 1.25 | 5 | 0 (0) | 4 | 120 (143) | 245 | 16384 | 2048 |
| 12 | Hash-V-grow-map | B | 84 | 256 | 32.8% | 64 | 16 | 8108 / 84 | 1.23 | 9 | 0 (0) | 4 | 120 (140) | 232 | 16384 | 2048 |
| 12 | Hash-V-grow-map | C | 4198 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.42 | 25 | 1 (16) | 10 | 8184 (9551) | 15530 | 16384 | 131072 |
| 12 | Hash-V-pack | A | 90 | 16384 | 0.5% | 64 | 8 | 8102 / 90 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 720 |
| 12 | Hash-V-pack | B | 84 | 16384 | 0.5% | 64 | 8 | 8108 / 84 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 8192 | 672 |
| 12 | Hash-V-pack | C | 4198 | 16384 | 25.6% | 64 | 16 | 3994 / 4198 | 1.09 | 6 | 1 (16384) | 0 | 0 (0) | 0 | 16384 | 33584 |
| 13 | Hash-V-fixed | A | 10109 | 65536 | 15.4% | 64 | 32 | 22659 / 10109 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | B | 10100 | 65536 | 15.4% | 64 | 32 | 22668 / 10100 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-fixed | C | 16485 | 65536 | 25.2% | 64 | 32 | 16283 / 16485 | 1.08 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 13 | Hash-V-grow-map | A | 10109 | 32768 | 30.9% | 64 | 32 | 22659 / 10109 | 1.14 | 16 | 0 (0) | 11 | 16376 (17358) | 41914 | 131072 | 262144 |
| 13 | Hash-V-grow-map | B | 10100 | 32768 | 30.8% | 64 | 32 | 22668 / 10100 | 1.13 | 17 | 0 (0) | 11 | 16376 (17323) | 41723 | 131072 | 262144 |
| 13 | Hash-V-grow-map | C | 16485 | 65536 | 25.2% | 64 | 32 | 16283 / 16485 | 1.42 | 21 | 1 (16) | 12 | 32760 (38192) | 64335 | 131072 | 524288 |
| 13 | Hash-V-pack | A | 10109 | 65536 | 15.4% | 64 | 16 | 22659 / 10109 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80872 |
| 13 | Hash-V-pack | B | 10100 | 65536 | 15.4% | 64 | 16 | 22668 / 10100 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 80800 |
| 13 | Hash-V-pack | C | 16485 | 65536 | 25.2% | 64 | 16 | 16283 / 16485 | 1.08 | 11 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131880 |
| 14 | Hash-V-fixed | A | 3515 | 65536 | 5.4% | 64 | 32 | 29253 / 3515 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | B | 3533 | 65536 | 5.4% | 64 | 32 | 29235 / 3533 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-fixed | C | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 14 | Hash-V-grow-map | A | 3515 | 8192 | 42.9% | 64 | 32 | 29253 / 3515 | 1.13 | 15 | 0 (0) | 9 | 4088 (4586) | 10418 | 131072 | 65536 |
| 14 | Hash-V-grow-map | B | 3533 | 8192 | 43.1% | 64 | 32 | 29235 / 3533 | 1.14 | 17 | 0 (0) | 9 | 4088 (4541) | 10486 | 131072 | 65536 |
| 14 | Hash-V-grow-map | C | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.41 | 24 | 1 (16) | 12 | 32760 (38356) | 64301 | 131072 | 524288 |
| 14 | Hash-V-pack | A | 3515 | 65536 | 5.4% | 64 | 16 | 29253 / 3515 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28120 |
| 14 | Hash-V-pack | B | 3533 | 65536 | 5.4% | 64 | 16 | 29235 / 3533 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 28264 |
| 14 | Hash-V-pack | C | 16494 | 65536 | 25.2% | 64 | 16 | 16274 / 16494 | 1.09 | 9 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131952 |
| 15 | Hash-V-fixed | A | 1008 | 65536 | 1.5% | 64 | 32 | 31760 / 1008 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | B | 1015 | 65536 | 1.5% | 64 | 32 | 31753 / 1015 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-fixed | C | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.09 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 15 | Hash-V-grow-map | A | 1008 | 2048 | 49.2% | 64 | 32 | 31760 / 1008 | 1.22 | 13 | 0 (0) | 7 | 1016 (1171) | 2223 | 131072 | 16384 |
| 15 | Hash-V-grow-map | B | 1015 | 2048 | 49.6% | 64 | 32 | 31753 / 1015 | 1.24 | 12 | 0 (0) | 7 | 1016 (1196) | 2179 | 131072 | 16384 |
| 15 | Hash-V-grow-map | C | 16495 | 65536 | 25.2% | 64 | 32 | 16273 / 16495 | 1.43 | 30 | 1 (16) | 12 | 32760 (38344) | 64345 | 131072 | 524288 |
| 15 | Hash-V-pack | A | 1008 | 65536 | 1.5% | 64 | 16 | 31760 / 1008 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8064 |
| 15 | Hash-V-pack | B | 1015 | 65536 | 1.5% | 64 | 16 | 31753 / 1015 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 8120 |
| 15 | Hash-V-pack | C | 16495 | 65536 | 25.2% | 64 | 16 | 16273 / 16495 | 1.09 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131960 |
| 16 | Hash-V-fixed | A | 284 | 65536 | 0.4% | 64 | 32 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | B | 284 | 65536 | 0.4% | 64 | 32 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-fixed | C | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 131072 | 524288 |
| 16 | Hash-V-grow-map | A | 284 | 1024 | 27.7% | 64 | 32 | 32484 / 284 | 1.09 | 10 | 0 (0) | 6 | 504 (572) | 1573 | 131072 | 8192 |
| 16 | Hash-V-grow-map | B | 284 | 1024 | 27.7% | 64 | 32 | 32484 / 284 | 1.08 | 16 | 0 (0) | 6 | 504 (561) | 1596 | 131072 | 8192 |
| 16 | Hash-V-grow-map | C | 16494 | 65536 | 25.2% | 64 | 32 | 16274 / 16494 | 1.41 | 19 | 1 (16) | 12 | 32760 (38208) | 64320 | 131072 | 524288 |
| 16 | Hash-V-pack | A | 284 | 65536 | 0.4% | 64 | 16 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2272 |
| 16 | Hash-V-pack | B | 284 | 65536 | 0.4% | 64 | 16 | 32484 / 284 | 1.00 | 1 | 0 (0) | 0 | 0 (0) | 0 | 65536 | 2272 |
| 16 | Hash-V-pack | C | 16494 | 65536 | 25.2% | 64 | 16 | 16274 / 16494 | 1.08 | 10 | 1 (65536) | 0 | 0 (0) | 0 | 65536 | 131952 |

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
| 13 | A | 10109 | 32 | 16 | 80872 | 32768 | 262144 |
| 13 | B | 10100 | 32 | 16 | 80800 | 32768 | 262144 |
| 13 | C | 16485 | 32 | 16 | 131880 | 32768 | 262144 |
| 14 | A | 3515 | 32 | 16 | 28120 | 32768 | 262144 |
| 14 | B | 3533 | 32 | 16 | 28264 | 32768 | 262144 |
| 14 | C | 16494 | 32 | 16 | 131952 | 32768 | 262144 |
| 15 | A | 1008 | 32 | 16 | 8064 | 32768 | 262144 |
| 15 | B | 1015 | 32 | 16 | 8120 | 32768 | 262144 |
| 15 | C | 16495 | 32 | 16 | 131960 | 32768 | 262144 |
| 16 | A | 284 | 32 | 16 | 2272 | 32768 | 262144 |
| 16 | B | 284 | 32 | 16 | 2272 | 32768 | 262144 |
| 16 | C | 16494 | 32 | 16 | 131952 | 32768 | 262144 |

## Also reported (rule-m2 item 13)

### Realized sharing

s: distinct values per input, as drawn; distinct values and W of every matrix (Hash-V-fixed's stores, from the model; the gates checked it).

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
| 13 | 128 | 1 | 16384 | 16384 | 16384 | 16384 | 10109 | 10100 | 16485 |
| 14 | 128 | 4 | 4096 | 4096 | 4096 | 16384 | 3515 | 3533 | 16494 |
| 15 | 128 | 16 | 1024 | 1024 | 1024 | 16384 | 1008 | 1015 | 16495 |
| 16 | 128 | 64 | 256 | 256 | 256 | 16384 | 284 | 284 | 16494 |

### Per step: cases, cross tests and stores (every row takes the same cases; spec v0.2 §15.7)

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
- job 13, C = A x B: add (2097152): zero_x 0.008, zero_y 0.000, int_both 0.000, int_x 0.000, int_y 0.042, den 0.001, general 0.948; mul (2097152): zero 0.000, one_y 0.000, int_both 0.012, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.100, int_y 0.095, general 0.757; cross tests 8388608 (matched 99227, cross reads 8388480); C's store W 16485, found / new 16283 / 16485
- job 14, C = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.044, den 0.001, general 0.946; mul (2097152): one_x 0.000, int_both 0.013, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.098, int_y 0.105, general 0.748; cross tests 8388608 (matched 102423, cross reads 8388224); C's store W 16494, found / new 16274 / 16494
- job 15, C = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.037, den 0.001, general 0.953; mul (2097152): int_both 0.011, den 0.035, num 0.000, cross_ad 0.000, cross_cb 0.000, int_x 0.093, int_y 0.093, general 0.768; cross tests 8388608 (matched 96562, cross reads 8388608); C's store W 16495, found / new 16273 / 16495
- job 16, C = A x B: add (2097152): zero_x 0.008, int_both 0.000, int_x 0.000, int_y 0.048, den 0.001, general 0.943; mul (2097152): int_both 0.015, den 0.031, num 0.000, int_x 0.092, int_y 0.126, general 0.736; cross tests 8388608 (matched 96748, cross reads 8388608); C's store W 16494, found / new 16274 / 16494

The gates checked every counter and store figure against the model (spec v0.2 §15.7 item 2), the TC gate included: MHVF:1 pass, MHVGM:1 pass, TC:1 pass, MHVP:1 pass, MHVF:2 pass, MHVGM:2 pass, TC:2 pass, MHVP:2 pass, MHVF:3 pass, TC:3 pass, MHVGM:3 pass, MHVP:3 pass, TC:4 pass, MHVGM:4 pass, MHVF:4 pass, MHVP:4 pass, MHVF:5 pass, TC:5 pass, MHVGM:5 pass, MHVP:5 pass, TC:6 pass, MHVF:6 pass, MHVGM:6 pass, MHVP:6 pass, MHVF:7 pass, TC:7 pass, MHVGM:7 pass, MHVP:7 pass, MHVF:8 pass, TC:8 pass, MHVGM:8 pass, MHVP:8 pass, MHVF:9 pass, TC:9 pass, MHVGM:9 pass, MHVP:9 pass, MHVF:10 pass, TC:10 pass, MHVGM:10 pass, MHVP:10 pass, MHVF:11 pass, TC:11 pass, MHVGM:11 pass, MHVP:11 pass, MHVF:12 pass, TC:12 pass, MHVGM:12 pass, MHVP:12 pass, MHVF:13 pass, MHVGM:13 pass, TC:13 pass, MHVP:13 pass, MHVF:14 pass, TC:14 pass, MHVGM:14 pass, MHVP:14 pass, MHVF:15 pass, TC:15 pass, MHVGM:15 pass, MHVP:15 pass, MHVF:16 pass, TC:16 pass, MHVGM:16 pass, MHVP:16 pass

### The idle and disturbance checks

As round 3 and m1 (rule-m2 item 5, carried over in full): processes are told apart by process, not by thread; the fork counter is reported only; host ticks are given as ticks / allowance (none in the pilot). The runner's own ticks are reported in total and per launch; the one sanity limit is more than 1 tick per launch plus 2 over a block.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | runner ticks per launch | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot | 1 | 2.541 | no | 2 | 14 | 0.143 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 6 | pilot | 1 | 2.611 | no | 1 | 14 | 0.071 | 14 | 0 | none | 8 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 0 (7->7 threads), bash 0 (1->1 threads) |
| 13 | pilot | 1 | 9.403 | no | 1 | 14 | 0.071 | 16 | 2 | none | 15 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (8->8 threads), claude 1 (7->8 threads), bash 0 (1->1 threads) |
| 1 | measured | 1 | 12.587 | no | 9 | 64 | 0.141 | 66 | 2 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 2 | measured | 1 | 12.317 | no | 7 | 64 | 0.109 | 64 | 0 | none | 18 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 3 | measured | 1 | 13.104 | no | 8 | 64 | 0.125 | 66 | 2 | none | 19 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 4 | measured | 1 | 12.933 | no | 8 | 64 | 0.125 | 64 | 0 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 5 | measured | 1 | 12.866 | no | 9 | 64 | 0.141 | 65 | 1 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 1/8 (7->7 threads), bash 0/8 (1->1 threads) |
| 6 | measured | 1 | 12.686 | no | 9 | 64 | 0.141 | 66 | 2 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/8 (7->7 threads), bash 0/8 (1->1 threads) |
| 7 | measured | 1 | 12.679 | no | 8 | 64 | 0.125 | 65 | 1 | none | 18 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 2/8 (7->7 threads), bash 0/8 (1->1 threads) |
| 8 | measured | 1 | 12.375 | no | 8 | 64 | 0.125 | 64 | 0 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 9 | measured | 1 | 12.951 | no | 8 | 64 | 0.125 | 66 | 2 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/8 (7->9 threads), bash 0/8 (1->1 threads) |
| 10 | measured | 1 | 12.881 | no | 8 | 64 | 0.125 | 64 | 0 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 1/8 (9->7 threads), bash 0/8 (1->1 threads) |
| 11 | measured | 1 | 12.863 | no | 8 | 64 | 0.125 | 65 | 1 | none | 18 | none | process_api 1/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 0/8 (8->8 threads), claude 2/8 (7->8 threads), bash 0/8 (1->1 threads) |
| 12 | measured | 1 | 12.592 | no | 8 | 64 | 0.125 | 64 | 0 | none | 18 | none | process_api 0/8 (6->6 threads), sh 0/8 (1->1 threads), environment-man 1/8 (8->8 threads), claude 1/8 (8->7 threads), bash 0/8 (1->1 threads) |
| 13 | measured | 1 | 50.89 | no | 8 | 64 | 0.125 | 69 | 5 | none | 56 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 6/16 (7->9 threads), bash 0/16 (1->1 threads) |
| 14 | measured | 1 | 50.351 | no | 9 | 64 | 0.141 | 68 | 4 | none | 56 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 15/16 (7->9 threads), bash 0/16 (1->1 threads) |
| 15 | measured | 1 | 50.932 | no | 9 | 64 | 0.141 | 67 | 3 | none | 56 | none | process_api 1/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 5/16 (9->7 threads), bash 0/16 (1->1 threads) |
| 16 | measured | 1 | 50.405 | no | 8 | 64 | 0.125 | 67 | 3 | none | 56 | none | process_api 2/16 (6->6 threads), sh 0/16 (1->1 threads), environment-man 1/16 (8->8 threads), claude 5/16 (7->7 threads), bash 0/16 (1->1 threads) |

The runner's own ticks over the 19 blocks: 136 ticks over 1066 launches, 0.128 per launch; the highest block 0.143 per launch (sanity limit: more than 1 per launch plus 2 per block). Reported only: they fall between reps, not in them.

### The measured process's own CPU time (reported only; enters no verdict)

CPU share: the timed process's CPU time (CLOCK_PROCESS_CPUTIME_ID) over the CLOCK_MONOTONIC time of the same whole rep, for the rep of minimum total in each round.

| job | row | rounds | CPU share, median | CPU share, lowest | CPU ns per run, median | total ns per run, median |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | TC | 12 | 1.0000 | 0.9981 | 440219 | 440157 |
| 1 | Hash-V-fixed | 12 | 0.9998 | 0.9105 | 509674 | 509320 |
| 1 | Hash-V-grow-map | 12 | 0.9992 | 0.9935 | 541571 | 542402 |
| 1 | Hash-V-pack | 12 | 0.9997 | 0.9761 | 522994 | 522837 |
| 1 | Hash-V-fixed-null | 12 | 0.9994 | 0.9792 | 510848 | 510703 |
| 2 | TC | 12 | 1.0000 | 0.9965 | 434763 | 435061 |
| 2 | Hash-V-fixed | 12 | 1.0000 | 0.9993 | 505033 | 504893 |
| 2 | Hash-V-grow-map | 12 | 0.9998 | 0.9989 | 519990 | 519807 |
| 2 | Hash-V-pack | 12 | 0.9999 | 0.9992 | 512923 | 512667 |
| 2 | Hash-V-fixed-null | 12 | 0.9999 | 0.9993 | 505606 | 505303 |
| 3 | TC | 12 | 0.9997 | 0.9974 | 381587 | 381415 |
| 3 | Hash-V-fixed | 12 | 0.9999 | 0.9319 | 456697 | 456515 |
| 3 | Hash-V-grow-map | 12 | 0.9997 | 0.9943 | 468125 | 468300 |
| 3 | Hash-V-pack | 12 | 0.9995 | 0.9946 | 462908 | 462681 |
| 3 | Hash-V-fixed-null | 12 | 0.9992 | 0.9966 | 456828 | 457016 |
| 4 | TC | 12 | 0.9993 | 0.9324 | 376705 | 376978 |
| 4 | Hash-V-fixed | 12 | 0.9994 | 0.9870 | 452642 | 453317 |
| 4 | Hash-V-grow-map | 12 | 0.9995 | 0.9945 | 461387 | 461514 |
| 4 | Hash-V-pack | 12 | 0.9996 | 0.9960 | 459108 | 459171 |
| 4 | Hash-V-fixed-null | 12 | 0.9995 | 0.9980 | 449693 | 449927 |
| 5 | TC | 12 | 0.9999 | 0.9946 | 3711228 | 3711193 |
| 5 | Hash-V-fixed | 12 | 0.9998 | 0.9845 | 4188458 | 4187767 |
| 5 | Hash-V-grow-map | 12 | 0.9994 | 0.9699 | 4272551 | 4281429 |
| 5 | Hash-V-pack | 12 | 0.9999 | 0.9936 | 4242670 | 4244087 |
| 5 | Hash-V-fixed-null | 12 | 0.9997 | 0.9946 | 4194764 | 4195076 |
| 6 | TC | 12 | 0.9999 | 0.9996 | 3688082 | 3687927 |
| 6 | Hash-V-fixed | 12 | 0.9999 | 0.9991 | 4168578 | 4168371 |
| 6 | Hash-V-grow-map | 12 | 0.9997 | 0.9831 | 4225377 | 4225858 |
| 6 | Hash-V-pack | 12 | 0.9997 | 0.9993 | 4203838 | 4205239 |
| 6 | Hash-V-fixed-null | 12 | 0.9999 | 0.9959 | 4166171 | 4169774 |
| 7 | TC | 12 | 0.9998 | 0.9912 | 3692036 | 3692685 |
| 7 | Hash-V-fixed | 12 | 0.9999 | 0.9993 | 4160819 | 4160561 |
| 7 | Hash-V-grow-map | 12 | 0.9999 | 0.9731 | 4201058 | 4200765 |
| 7 | Hash-V-pack | 12 | 1.0000 | 0.9943 | 4174727 | 4175223 |
| 7 | Hash-V-fixed-null | 12 | 1.0000 | 0.9937 | 4161355 | 4161186 |
| 8 | TC | 12 | 0.9999 | 0.9932 | 3544952 | 3548426 |
| 8 | Hash-V-fixed | 12 | 0.9999 | 0.9986 | 4049352 | 4049828 |
| 8 | Hash-V-grow-map | 12 | 1.0000 | 0.9962 | 4078687 | 4078237 |
| 8 | Hash-V-pack | 12 | 1.0000 | 0.9993 | 4052368 | 4051898 |
| 8 | Hash-V-fixed-null | 12 | 1.0000 | 0.9997 | 4049749 | 4049605 |
| 9 | TC | 12 | 0.9999 | 0.9963 | 30436552 | 30479863 |
| 9 | Hash-V-fixed | 12 | 0.9995 | 0.9902 | 34016703 | 34044482 |
| 9 | Hash-V-grow-map | 12 | 0.9998 | 0.9978 | 34246634 | 34249643 |
| 9 | Hash-V-pack | 12 | 0.9998 | 0.9989 | 34127982 | 34145736 |
| 9 | Hash-V-fixed-null | 12 | 0.9998 | 0.9863 | 34054367 | 34078082 |
| 10 | TC | 12 | 0.9993 | 0.8483 | 30311381 | 30337438 |
| 10 | Hash-V-fixed | 12 | 0.9993 | 0.9610 | 33927827 | 33951580 |
| 10 | Hash-V-grow-map | 12 | 1.0000 | 0.9933 | 34024988 | 34066566 |
| 10 | Hash-V-pack | 12 | 0.9998 | 0.8433 | 34001810 | 34000405 |
| 10 | Hash-V-fixed-null | 12 | 0.9994 | 0.9948 | 33882030 | 33948269 |
| 11 | TC | 12 | 0.9994 | 0.9324 | 30371616 | 30372806 |
| 11 | Hash-V-fixed | 12 | 1.0000 | 0.9738 | 33932756 | 33929543 |
| 11 | Hash-V-grow-map | 12 | 0.9988 | 0.9526 | 33976206 | 34017381 |
| 11 | Hash-V-pack | 12 | 1.0000 | 0.9993 | 33973885 | 33971836 |
| 11 | Hash-V-fixed-null | 12 | 0.9997 | 0.9585 | 33876298 | 33904722 |
| 12 | TC | 12 | 0.9997 | 0.9946 | 29516939 | 29521858 |
| 12 | Hash-V-fixed | 12 | 0.9993 | 0.9956 | 33131720 | 33174961 |
| 12 | Hash-V-grow-map | 12 | 0.9995 | 0.9708 | 33159759 | 33171627 |
| 12 | Hash-V-pack | 12 | 0.9999 | 0.9905 | 33048568 | 33047194 |
| 12 | Hash-V-fixed-null | 12 | 0.9997 | 0.9981 | 33075898 | 33089173 |
| 13 | TC | 12 | 0.9996 | 0.9416 | 248294452 | 248475120 |
| 13 | Hash-V-fixed | 12 | 0.9993 | 0.9925 | 276140574 | 276604632 |
| 13 | Hash-V-grow-map | 12 | 0.9993 | 0.9883 | 276952760 | 277269046 |
| 13 | Hash-V-pack | 12 | 0.9993 | 0.9960 | 276267831 | 276495130 |
| 13 | Hash-V-fixed-null | 12 | 0.9998 | 0.9951 | 276390926 | 276843836 |
| 14 | TC | 12 | 0.9997 | 0.9928 | 246378738 | 246388654 |
| 14 | Hash-V-fixed | 12 | 0.9997 | 0.9923 | 274619594 | 274868407 |
| 14 | Hash-V-grow-map | 12 | 0.9997 | 0.9943 | 275013567 | 275076352 |
| 14 | Hash-V-pack | 12 | 0.9996 | 0.9957 | 274529301 | 274786932 |
| 14 | Hash-V-fixed-null | 12 | 0.9998 | 0.9975 | 273872423 | 274235198 |
| 15 | TC | 12 | 0.9976 | 0.9882 | 248542115 | 249232802 |
| 15 | Hash-V-fixed | 12 | 0.9985 | 0.9943 | 275098383 | 275375368 |
| 15 | Hash-V-grow-map | 12 | 0.9997 | 0.9951 | 276159310 | 276411262 |
| 15 | Hash-V-pack | 12 | 0.9997 | 0.9898 | 275835354 | 276597272 |
| 15 | Hash-V-fixed-null | 12 | 0.9971 | 0.9732 | 276312556 | 277532866 |
| 16 | TC | 12 | 0.9994 | 0.9931 | 246909239 | 247253898 |
| 16 | Hash-V-fixed | 12 | 0.9988 | 0.9890 | 274524464 | 274604871 |
| 16 | Hash-V-grow-map | 12 | 0.9995 | 0.9782 | 273791519 | 274434524 |
| 16 | Hash-V-pack | 12 | 0.9985 | 0.9935 | 274122136 | 274567004 |
| 16 | Hash-V-fixed-null | 12 | 0.9997 | 0.9936 | 274325958 | 274776328 |

Idle checks: 19 (0 busy). Host rates R_h: {'1': 0.10634903754121025, '79': 0.10634903754121025, '84': 0.10634903754121025, '92': 0.10634903754121025, '19300': 0.10634903754121025} (measured: {'1': 0.0, '79': 0.0, '84': 0.0, '92': 0.10634597724712484, '19300': 0.0}; R_h is at least 1 tick over the longest pilot block).

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

