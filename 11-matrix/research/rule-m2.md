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
