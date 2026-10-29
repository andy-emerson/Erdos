# Design 3, round 2: results

Run started 2026-10-29T13:12:24-05:00.

## The measurement rule (research/rule-r2.md, verbatim, written before the first trial)

sha256 `44baa5f1db601571d538116d37b1b8859fbf0eb99fe149baaa9ba655879e5f66`

<!-- rule begins -->
# Design 3, round 2: measurement rule

Oct 29, 2026 · **Fixed: approved by the author, Oct 29, 2026.** Revised after the first independent review (spec v0.2 §17): items 5, 7, 9 and 10 and the pilot. This rule measures the two textbook baselines of [spec v0.2](../v0.2.md): **Q32** and **TC**.

Three points are the author's (Oct 29, 2026):
- the two rows;
- the 30-minute hard stop;
- no background process of the runner's own during timed phases.

**Every other threshold, count and limit is approved by the author, Oct 29, 2026**, and each is marked so where it is stated. Round 1's rule, [`rule.md`](rule.md), stays as archived, unchanged. Its text is pinned by hash in the round-1 results.

**Fixing the rule.** The author approved the rule on Oct 29, 2026. Every threshold the draft proposed now carries the mark "approved by the author, Oct 29, 2026", and the rule is fixed. The harness writes it verbatim into its results file before the first trial. It refuses a measured run unless this file has that text (by sha256, pinned in the harness), **and while any mark of a proposal awaiting approval remains in it**. A change after the first trial is a revision, dated and explained at the top.

Results are reported **per measure only, with no overall winner** (item 11). The CPU profile is **reported, not judged** (item 10).

**Scope of the comparison.** Both rows follow one pseudocode and make the same known-answer skips. Their arithmetic is identical, call for call (spec v0.2 §14.1 item 3). They differ only in structure:
- Q32 compares indices, reads whole numbers through them, and looks up every new result's whole numbers;
- TC compares 64-bit values and stores a pair.

So a time difference between the rows is the cost or the saving of the structure, at 64 bits. Neither row is optimized beyond the textbook (spec v0.2 §0, decision 5).

## Measurement rule

1. **Rows.** Each job runs three rows:
   - **Q32**: layer 1 with 16-bit index fields (spec v0.2 §§5, 7);
   - **Q32-null**: the same binary as Q32, run as a separate row, as the null control (item 8);
   - **TC**: the textbook control (spec v0.2 §§5, 8).

   Retired, and not run: L1 and L1-opt at both widths, the 08 control `ctl`, fmpq, mpq and fz (spec v0.2 §16).

2. **Builds.**
   - Q32, TC and the driver are built by one compiler with one set of flags (`gcc -std=c11 -O2`), with the 08 integer module unchanged (its sha256 recorded and checked before every run).
   - Each row has the four builds of spec v0.2 §6 item 4: timing, counting, test and profiling.
   - The timing build carries no counters, audits, probes or allocation hooks.
   - The **counting build** gives the counts and memory reports, from one complete run per row and job. In it, the driver reads both rows' counters after every call, untimed (spec v0.2 §14.1 item 3).
   - The **memory run** (item 7) gives the heap figures.
   - Counts and memory figures are deterministic. On jobs 3 and 10, every counting and memory run is repeated, and the second MUST be identical (**approved by the author, Oct 29, 2026**: the two jobs of round 1's repeat set that fit Q32).
   - Each counting or memory run has a limit of **10 minutes** (**approved by the author, Oct 29, 2026**). A run that does not finish gives n/a, with the reason.
   - Q32-null shares Q32's gates, counting and memory runs.

3. **Gates before the measured run.** In the test build, Q32 and TC MUST pass their unit tests (spec v0.2 §14.3), with the capacity tests and both switches. On every job, each MUST pass the gate (v0.1 §14.3 item 2) and the exact checks of spec v0.2 §14.1:
   - every stored value against `fractions`;
   - Q32's $W$, after entry and at the end;
   - every case count and every K counter against the generator's prediction;
   - Q32's lookups against the prediction.

   The counting runs MUST pass the call-for-call check: Q32's per-call arithmetic equal to TC's (spec v0.2 §14.1 item 3). A row that fails a gate is fixed and the gate is re-run; no measured run starts until every gate passes. Gates are untimed. They MAY run two at a time, one per core, but **only before the timed steps begin**: the worker pool is shut down before step 6 (item 5).

4. **Timing** (as round 1, rule.md item 4; every number **approved by the author, Oct 29, 2026**, as carried over):
   - **Rounds.** Each job runs **12 interleaved rounds**. In each round, each row runs once as its own process, in an order rotated by one row per round. Each invocation reads the job's workload file, then runs its reps.
   - **A rep** runs the whole job **r** times, each on a new, empty class destroyed untimed after its read-out. The rep's time is its total divided by $r$. **r** is the smallest power of two for which one rep lasts at least **50 ms**.
   - **Reps per invocation.** If the calibration rep lasts at least **1 s**, the row runs 1 rep per invocation. Otherwise it runs **3 reps**: the first is discarded, and its time is the minimum of the other two.
   - **Calibration**, per job, immediately before its round 1, in the timing build, on core 1: $r$ for Q32 and TC, by one invocation that times a rep at $r = 1, 2, 4, \ldots$ until one lasts at least 50 ms. Q32-null uses Q32's $r$. There is no form calibration, since there are no library rows. Calibration times enter no measure.
   - **The time** of a row in a round is its **total**: entry + operations + read-out of every stored value. The three parts are also reported. Creating and destroying a class and reading the workload are not timed.
   - **Pinning.** Every timed invocation (calibration, rounds, the pilot's rounds and profiling run, the profile runs) runs pinned to **core 1**, one at a time, by the start of item 5.2. The runner is pinned to **core 0**. Jobs run in job order, each with its calibration and all 12 rounds before the next.
   - **Time cap.** A timed rep that lasts more than **10 s** ends that row's timing on that job. The row is **over cap**, a reported finding.

5. **Nothing of the runner's own runs during a timed phase** (the author's requirement). A **timed phase** is every timed invocation: calibration, rounds, the pilot's rounds and profiling run, and the profile runs. From the start of a job's timed block to its end, the only process that runs is the timed invocation. Everything else of the run is **blocked in the kernel**, not polling.

   **The runner** (the harness process) MUST:
   1. be single-threaded during timed phases: no worker pool, watchdog thread, timer thread or logger thread exists from step 6 on (the pool of steps 4–5 is shut down and joined first);
   2. start each timed invocation by **`fork` and then `exec`**, with **no extra process** (no shell and no `taskset` wrapper). Between the two, the child:
      1. pins itself to core 1 with `sched_setaffinity`, **before `exec`**, so the timed program never runs on another core;
      2. opens its output file and redirects stdout and stderr **to that file** with `dup2`, not to a pipe, so no pipe wakes the runner;
      3. calls `exec`, and if `exec` fails, exits at once with status 127.

      In Python this is `os.fork`, then in the child `os.sched_setaffinity(0, {1})`, `os.open`, `os.dup2`, `os.execv` and `os._exit(127)`. `posix_spawn` (Python's `os.posix_spawn`) MUST NOT be used: it cannot set the affinity before `exec`. Nor may `subprocess`, which may use `posix_spawn` or `vfork` and opens an internal pipe;
   3. wait for it with one **blocking `waitpid`**, with no timeout argument and no polling loop (Python's `Popen.wait(timeout=…)` and `communicate(timeout=…)` poll, and MUST NOT be used);
   4. enforce the hard stop with a **kernel timer**: `setitimer(ITIMER_REAL)` set to the time left before the hard stop, whose `SIGALRM` handler kills the child and ends the run. The timer runs no code until it fires;
   5. write no log line, state file or result between the start and the end of an invocation. It reads the child's output file and writes its records only after `waitpid` returns.

   **The operator** (the person or agent that starts the run, and any tool or session acting for them) MUST:
   1. start the runner **once**, with one command that returns only when the runner exits: a foreground command, or the session's background-command facility that notifies the operator when the command exits (for Claude Code, the Bash tool with `run_in_background`, with a timeout of at least **35 minutes**, **approved by the author, Oct 29, 2026**);
   2. run **nothing else** on the machine until that exit notice: no polling or waiting loop, no `sleep`, `watch`, `tail -f`, file check, Monitor or until-loop, no other tool call, build, test or edit, and no detached (`nohup`, `&`) process. A parent shell that is blocked in `wait` is allowed; anything that wakes periodically is not.

   **Check, after each timed block** (untimed, between blocks). Just before and just after the block, the runner takes two snapshots:
   - **Ticks.** For every process, `utime + stime + cutime + cstime` from `/proc/<pid>/stat` (fields 14–17). `cutime + cstime` holds the ticks of the children a process has waited for, so a short-lived child that started and exited within the block is charged to its parent. For the runner itself only `utime + stime` counts, since its `cutime + cstime` are its timed children's.
   - **Forks.** The fork counter, the `processes` line of `/proc/stat`, which counts every process and thread created since boot. The runner also counts its own spawns in the block (one per timed invocation). The block's **unexplained forks** are the rise of the counter, less the runner's spawns, less the new kernel threads present in the second snapshot. They catch a process that started and exited within the block whatever its parent, even one that was never waited for.

   Excluded from the tick tests are the runner itself (reported separately), the block's timed children, kernel threads (children of `kthreadd`: reported, never flag) and the host processes (below), which have their own allowance. The block is **disturbed** if any of these holds:
   - any other process used more than **5 clock ticks** (50 ms at 100 Hz) over the block (**approved by the author, Oct 29, 2026**);
   - any new process, other than the runner's timed children, started during the block and used at least one tick (**approved by the author, Oct 29, 2026**);
   - the block had more than **0 unexplained forks** (**approved by the author, Oct 29, 2026**);
   - a host process used more than its allowance.

   **Host processes.** An ancestor of the runner that cannot be stopped during the run is a **host process**: for example, the session process that hosts the operator's background command, and process 1. At start, the runner finds its ancestors by following `PPid` to process 1, and lists them in the results. A parent blocked in `wait` uses no ticks. The others are measured, not assumed:
   1. **The pilot records the baseline** (Run plan step 3). For each host process $h$, it records the ticks (all four fields) over each of the pilot's timed blocks, as a rate in ticks per second. $R_h$ is the highest rate over those blocks. During the pilot, host ticks are recorded and do not flag; every other test applies.
   2. **The allowance is then fixed**, before step 6, by this formula (**approved by the author, Oct 29, 2026**): over a block of $L$ seconds, host process $h$ may use at most $5 + \lceil 2\,R_h\,L \rceil$ ticks. The values of $R_h$ and of each block's allowance are written to the results.
   3. **A busy host stops the run.** If any $R_h$ exceeds **1 tick per second** (1% of one core at 100 Hz; **approved by the author, Oct 29, 2026**), the run stops after the pilot and is reported to the author before any further step: the machine is not quiet enough to time on.

   The ticks of the runner's ancestors are thus all counted, and an operator's polling loop would be caught: by its own ticks, by its children's ticks in its `cutime + cstime`, or by the forks of the commands it starts. A disturbed block is discarded, logged with the processes, their ticks and the forks, and **re-run once** (calibration and all rounds; the time counts on the run's clock). A second disturbance of the same job stops the run, which is reported to the author. The runner's own ticks per block are reported. More than **2 ticks** per block (**approved by the author, Oct 29, 2026**) is a harness bug.

6. **Idle machine** (the round-1 check, with a changed criterion: **approved by the author, Oct 29, 2026**). Before each job's timed block (the pilot's, step 6's and step 7's), the runner takes a **2-second** sample (**approved by the author, Oct 29, 2026**). The machine is idle when both of these hold (**approved by the author, Oct 29, 2026**):
   - no process outside the runner used more than **5%** of one core over the sample (the round-1 threshold);
   - core 1 was busy for at most **2%** of the sample, from `/proc/stat`.

   If the machine is busy, the runner re-checks every **15 s**, logging each check. After **10 minutes** it refuses to time and stops the run. The waits count on the run's clock.

   *Why the criterion changes.* Round 1 required the 1-minute load average to be at most 0.5. Right after a timed block, the run's own child had raised that average, which then decays over about a minute. In the Q32 run, about 5 of its 10.9 minutes were spent waiting for the average to fall from the run's own work ([`../test/results-q32/run.log`](../test/results-q32/run.log), lines 6–62). The average is now recorded with every check but does not gate.

7. **Measures**, per job, Q32 against TC:
   - **Time.** **T(TC) / T(Q32)**, with T the median of the row's per-round totals. Above 1, Q32 is faster. Its 95% interval [lo, hi] runs from the 2.5th to the 97.5th percentile of the same ratio over **10,000 bootstrap resamples** of the rounds, using the same draw for both rows to keep the pairing (seed 20261029; **approved by the author, Oct 29, 2026**, as round 1). Entry, operations and read-out are each reported with their ratio and interval, without a verdict.
   - **Bytes per rational, accounted (B_used).** **B_used(Q32) / B_used(TC)**, from the memory reports: Q32 $4 + (8W + 2S)/N$, TC 16 (spec v0.2 §13.2).
   - **Bytes per rational, heap (B_heap).** **B_heap(Q32) / B_heap(TC)**: the growth of the C library's in-use heap (`mallinfo2`: `uordblks + hblkhd`) from just before `create` to the measuring point, divided by $N$. `mallopt` sets `M_MMAP_THRESHOLD` and `M_TRIM_THRESHOLD` to $2^{30}$ in both rows (as round 1).
   - Both memory figures come from one **memory run** per row and job (timing build), taken **after entry** and **at the end** (before read-out). Verdicts use the end figures, and the after-entry figures are reported beside them.
   - **Gcd work: a check, not a verdict** (approved by the author, Oct 29, 2026). **G(Q32) / G(TC)**, with G the counting build's sum over every gcd of entry and operations of the bit length of the larger operand (08 rule item 6). *By the call-for-call check of item 3 it is exactly 1 on every job*: the rows make the same gcds, so it is equal by construction and cannot be won or lost. It is reported as a **check**, with the absolute G per operation beside it, and gets no verdict. A value other than 1 is a bug (item 12), not a finding.

8. **Null control.** Q32 against Q32-null on the same job, by item 7. The job's **null spread** is max(1 − lo, hi − 1, 0), and the time band's half-width is **h = max(5%, null spread)** (**approved by the author, Oct 29, 2026**, as round 1). TC has no null copy of its own. It is timed in the same rounds by the same driver, and its verdict uses the same h.

9. **Verdict per job and measure, with its magnitude** (thresholds **approved by the author, Oct 29, 2026**, as round 1):

   | Measure | Q32 wins | Q32 loses | Tie | Magnitude reported |
   | --- | --- | --- | --- | --- |
   | Time | lo > 1 + h | hi < 1 − h | otherwise | T(TC)/T(Q32), [lo, hi], h; entry, operations, read-out |
   | Bytes, B_used (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | the ratio after entry and at the end; W, N, S, W/N |
   | Bytes, B_heap (end) | ratio ≤ 0.90 | ratio ≥ 1.10 | otherwise | the ratio after entry and at the end |

   Gcd work has no row here: it is a check (item 7), reported per job as **passed** (ratio exactly 1) with G(Q32)/G(TC) and G per operation.

   - A memory ratio of at least **2.0** at the end raises the **severe-memory-loss flag** (**approved by the author, Oct 29, 2026**, as round 1), reported beside the verdict.
   - A row over cap has the time verdict **over cap**.
   - A measure that is n/a has no verdict.
   - A verdict is never reported without its magnitude.

10. **The profile: reported, not judged.** After the rounds (step 7), each row's profiling build (spec v0.2 §13.4) runs on each job. It probes one call in **K = 32** (**approved by the author, Oct 29, 2026**), chosen with random gaps (spec v0.2 §13.4 item 3), and times every part in batches. Per job, the rows alternate Q32, TC, Q32, TC, Q32, TC: **3 invocations per row** (**approved by the author, Oct 29, 2026**), at the timing build's calibrated $r$, 3 reps each with the first discarded. Each runs pinned to core 1, one at a time, under items 5 and 6. Each figure is the median over the 3 invocations. The report gives, per row and job:
    - the **slowdown** of each invocation: its batch total over the timing build's median total for the same row and job. It MUST be at most **1.5** (**approved by the author, Oct 29, 2026**). An invocation over the limit is marked **over the slowdown limit**, with its slowdown, and its phase figures are withheld;
    - the batch time per operation of entry, operations and read-out;
    - for each profile phase (F, C, A.gcd, A.rest, I, S, M and U): its share of the sampled calls' time, raw and corrected, and its estimated nanoseconds per operation (spec v0.2 §13.4 item 7);
    - the probe cost, K, and the number of sampled calls;
    - the same per job phase (entry, add, sub, mul, div, read-out), and the shares per case for cases with at least **100 sampled calls** (**approved by the author, Oct 29, 2026**);
    - **Q32 − TC per phase**, in estimated nanoseconds per operation, labelled **indicative only**. The fences of a sampled call stop one phase's latency from overlapping the next phase's work, as it does in the timing build, so a phase's sampled time is not its time there (spec v0.2 §13.4 item 9).

    No verdict, band or flag is attached to any profile figure.

11. **Strata, and no overall winner.**
    - The strata are **MONEY** (jobs 1–2), **SMALL** (3–4) and **CHAINS** (8–11). MOD (5–6) and NEW (7) have no job within Q32's capacity (Workloads).
    - For each stratum and measure, the report gives the counts of wins, ties, losses, over-cap and n/a, the median ratio, and the number of severe-memory-loss flags. The per-job verdicts and magnitudes are listed beneath.
    - The rule combines no measures, scores no strata, and names no row as the better one.

12. **Checks, and bugs after the gates.** Every stored value of every invocation (rounds, calibration, counting, memory and profile runs) is compared with the exact value from `fractions`. After the gates, any of the following is a **bug**:
    - a check failure;
    - an overflow;
    - a failed exact check of spec v0.2 §14.1;
    - a non-repeating count;
    - Q32 reaching its capacity;
    - gcd work that differs between the rows.

    A bug is handled as in round 1 (rule.md item 5): the row's measured run stops, the bug is fixed under review and logged, and the row is re-run in full. A fix to `q32.c` re-runs Q32 and Q32-null; a fix to `tc.c` re-runs TC. The report lists every bug.

13. **Also reported:**
    - **Realized reuse** per job (from the generator, and equal to Q32's figures by the exact checks): N, W, W/N and 2N/W, after entry and at the end; the share of operations in each case of spec v0.2 §9, per algorithm; the share of operands that are earlier results.
    - **Counts per operation**, per row: gcds, gcd work, multiplications, additions and exact divisions, which are equal between the rows. For Q32 also: lookup searches and probes, interned found and appended, and whole-number reads. Every counter per phase, core and store goes in a CSV.
    - **The known-answer skips**, per job and row: each K counter of spec v0.2 §13.1 with its prediction, and the share of the work it removes (divisions, gcds, lookups, reads).
    - **Memory against reuse:** the jobs sorted by W/N at the end, with both rows' B_used and B_heap.
    - **The idle checks and the disturbance checks** (items 5 and 6): every check, the runner's own ticks per block, the unexplained forks per block, and the host processes with their rates $R_h$ and their ticks and allowance per block.
    - **Round 1, for reference only:** for each job, a link to its round-1 verdicts. Round 1 enters no round-2 measure.

## Workloads

The workloads are **as in round 1** ([`rule.md`](rule.md), Workloads): the same generator, master seed 20261029, job $n$ seeded 20261029 + $n$, the same inputs, operations and chains, and the same size bound. Since the generator is unchanged, every workload file is byte-identical to round 1's, and the harness checks this by sha256 against round 1's `state.json`.

**Only the jobs that fit Q32 are run** (capacity 32,768 whole numbers; $W$ at the end, from the generator, as reported in round 1):

| Job | Name | Stratum | $W$ at the end | Run |
| --- | --- | --- | --- | --- |
| 1 | MONEY-wide | MONEY | 29,866 (91% of capacity) | yes |
| 2 | MONEY-catalog | MONEY | 15,403 | yes |
| 3 | SMALL8 | SMALL | 350 | yes |
| 4 | SMALL64 | SMALL | 21,346 | yes |
| 5 | MOD-mixed | MOD | 40,849 | **no: over capacity** |
| 6 | MOD-muldiv | MOD | 40,798 | **no: over capacity** |
| 7 | NEW-add | NEW | 50,795 | **no: over capacity** |
| 8 | CH-money | CHAINS | 27,881 (85%) | yes |
| 9 | CH-closed | CHAINS | 28,379 (87%) | yes |
| 10 | CH-small | CHAINS | 10,945 | yes |
| 11 | CH-mixed | CHAINS | 12,682 | yes |

The spec's known-answer skips change no stored value, so $W$ is unchanged. Neither row runs on jobs 5–7. They are reported as **over capacity for Q32**, with $W$. TC could run on them, but it would have nothing to be paired with.

## Run plan

1. Build every row in its four builds, check the 08 module's hashes, and generate or verify the workloads, references and predictions.
2. Unit tests of Q32 and TC (test build; spec v0.2 §14.3), including the sanitizer build.
3. **Pilot:** jobs **1, 3 and 10** (**approved by the author, Oct 29, 2026**: round 1's pilot jobs that fit Q32), with every row, $r$ calibration and **2 rounds**, timed under items 4–6. It ends with **one profiling-build run**: one invocation of each row's profiling build on **job 1** (**approved by the author, Oct 29, 2026**: the largest job that fits), as in item 10, in its own timed block. Its results are discarded, except for what it records for the run:
   - the host processes' rates $R_h$ (item 5), from which the host allowance is fixed before step 6;
   - the profiling run's slowdown for each row (item 10);
   - its wall time, in two parts: $P_\text{prof}$, the profiling run's block, and $P$, the rest of the pilot.

   **Stop rule.** The projection is the elapsed run time, plus the remaining work outside the profile, plus the profile:
   $$\text{projection} = \text{elapsed} + 5\ \text{min} \times \max(1,\ P / 0.5\ \text{min}) + 36 \times P_\text{prof}.$$
   The 5 minutes and 0.5 minutes are **approved by the author, Oct 29, 2026** (below). The factor 36 (**approved by the author, Oct 29, 2026**) is 24, since step 7 makes 48 profile invocations against the pilot's 2, with half again for margin.
   - **If the projection exceeds 22 minutes (approved by the author, Oct 29, 2026), the run stops and is reported to the author before anything else runs.** That leaves a quarter of the hard stop for re-runs under item 12, as in round 1.
   - **The run also stops after the pilot**, and is reported to the author, if any host rate $R_h$ exceeds 1 tick per second (item 5), or if either row's profiling slowdown exceeds 1.5 (item 10), so that K can be reconsidered.
4. Gates (item 3), two at a time on both cores.
5. Counting runs and memory runs (items 2 and 7), two at a time on both cores. The worker pool is then shut down and joined (item 5).
6. Per job, in job order: the idle check, then calibration and the 12 rounds, one timed invocation at a time on core 1, then the disturbance check.
7. Per job, in job order: the idle check, then the profile runs (item 10), then the disturbance check.
8. Analysis and report. This step is untimed and runs after the last timed block.

**Hard stop at 30 minutes (the author's).** The run's own wall time, from step 1 to the end of step 7, re-runs included, is limited to 30 minutes, summed over its sessions. Time spent fixing a bug under review, between sessions, is not counted. The limit is enforced by the kernel timer of item 5. When it is reached, the run stops at once, writes what it has, marked **stopped**, and reports to the author. No verdict is reported from a stopped run.

*Non-normative estimate*, from round 1's Q32 run (`test/results-q32/run.log`: about 0.24 s per timed invocation, and builds, generation and unit tests in about 20 s):
- **Rounds:** 3 rows × 8 jobs × 12 rounds = 288 invocations, about 1.2–2 minutes. Calibration is 16 invocations, under 0.2 minutes.
- **Profile:** 2 rows × 3 invocations × 8 jobs = 48 invocations of a build at most 1.5 times slower, about 0.2–0.4 minutes.
- **Gates, counting and memory runs:** 16 gate runs, and 32 counting and memory runs with 4 repeats, under 1 minute on two cores.
- **Idle checks:** 2 s per check, 20 checks (4 pilot blocks, 8 timed blocks, 8 profile blocks), about 0.7 minutes, plus any waits.
- **Pilot:** 18 invocations and 6 calibrations, about 0.2–0.3 minutes, and 2 profiling invocations, about 0.02 minutes.
- **The whole run: about 3–6 minutes** without waits. The 5-minute estimate of step 3 is the upper end of steps 4–7 without the profile, about 3.4 minutes, with half again for margin. The profile is projected separately, from the pilot's profiling run.

## Limitations, reported with the results

- **Textbook only.** Neither row is optimized (spec v0.2 §0, decision 5). The verdicts compare two plain baselines. They do not compare the designs at their best.
- **Every result is kept**, in both rows, as in round 1. Chains do not update in place.
- **64-bit arithmetic only.** At this width a gcd is cheap, so time differences come mostly from structure. The gcd work is equal by construction, and is reported to make that visible.
- **The profile perturbs what it measures** (spec v0.2 §13.4 items 8 and 9). Its fences stop the overlap between phases, so its shares and its per-phase differences between the rows are indicative only. They are meant to guide the next step, not to predict a saving.
- **Jobs 5–7 are not run.** Round 2 says nothing about the low-reuse jobs beyond Q32's capacity.

## Harness

The round-1 harness (`test/gen.py`, `l1model.py`, `runner.py`, `analyze.py`, `bench.c`, `api.h`) is the starting point, with these changes:
- `l1model.py` models spec v0.2: case `zero`, the K counters, Q32's lookups and reads;
- `bench.c` drives Q32 and TC only, and adds a `prof` mode;
- `runner.py` implements items 5 and 6, has no library rows or forms, and enforces the hard stop by kernel timer;
- `analyze.py` reports items 7–13.

**The official data.** Round 2 writes only to **`test/results-r2/`**. The harness refuses to start if its output folder is `test/results/`, `test/results-q64-b3b88b6/` or `test/results-q32/`. It never writes or deletes there, and it checks the sha256 of every file in those three folders before and after the run. Decisions the builder makes go in `test/DECISIONS.md`, under a round-2 heading, pending the author's review.
<!-- rule ends -->

## The run

- Started 2026-10-29T13:12:24-05:00; **stopped**: job 1: the pilot block was disturbed twice (rule-r2 item 5).
- Run time 0.4 minutes (hard stop 30).
- Rule sha256 `44baa5f1db601571d538116d37b1b8859fbf0eb99fe149baaa9ba655879e5f66`; round 1's official data after the run: unchanged.
- Host processes (the runner's ancestors): bash[7691], claude[91], environment-man[84], sh[79], process_api[1]
- Code at 2026-10-29T13:12:24-05:00: Q32 `95cd925fa596`, TC `9471c1318b14`
- Unit tests: r2_unit: r2_unit: 489645 checks, 0 failures; r2_diff: r2_diff: 300 seeds, 233379 operations (8210 divisions by zero, 19460 overflows), every case and K-skip met; 977113 checks, 0 failures; r2_unit-asan: r2_unit: 489645 checks, 0 failures; r2_diff-asan: r2_diff: 60 seeds, 46345 operations (1630 divisions by zero, 3765 overflows), every case and K-skip met; 194197 checks, 0 failures
- **Over capacity for Q32** (not run): job 5 (MOD-mixed, W = 40849), job 6 (MOD-muldiv, W = 40798), job 7 (NEW-add, W = 50795)

## Verdicts

**No verdict is reported: the run did not finish** (stopped in step 3: job 1: the pilot block was disturbed twice (rule-r2 item 5)).

## Also reported (rule-r2 item 13)

### Realized reuse

| job | N after entry | W | W/N | 2N/W | N at the end | W | W/N | 2N/W | result operands |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 10000 | 9991 | 0.999 | 2.00 | 30000 | 29866 | 0.996 | 2.01 | 0.249 |
| 2 | 10000 | 209 | 0.021 | 95.69 | 30000 | 15403 | 0.513 | 3.90 | 0.251 |
| 3 | 10000 | 8 | 0.001 | 2500.00 | 30000 | 350 | 0.012 | 171.43 | 0.249 |
| 4 | 10000 | 64 | 0.006 | 312.50 | 30000 | 21346 | 0.712 | 2.81 | 0.252 |
| 8 | 10000 | 208 | 0.021 | 96.15 | 42000 | 27881 | 0.664 | 3.01 | 0.484 |
| 9 | 10000 | 3268 | 0.327 | 6.12 | 42000 | 28379 | 0.676 | 2.96 | 0.484 |
| 10 | 10000 | 63 | 0.006 | 317.46 | 18000 | 10945 | 0.608 | 3.29 | 0.438 |
| 11 | 10000 | 63 | 0.006 | 317.46 | 18000 | 12682 | 0.705 | 2.84 | 0.438 |

Case shares per algorithm:

- job 1: add: add_den 0.243, add_general 0.757; sub: add_den 0.253, add_general 0.747
- job 2: add: add_zero 0.001, add_den 0.256, add_general 0.743; sub: add_zero 0.001, add_den 0.244, add_general 0.755
- job 3: mul: mul_one_x 0.027, mul_one_y 0.029, mul_both 0.018, mul_den 0.098, mul_num 0.077, mul_cross_ad 0.085, mul_cross_cb 0.077, mul_general 0.589; div: div_one_x 0.028, div_one_y 0.029, div_both 0.019, div_cross_ad 0.100, div_cross_cb 0.079, div_num 0.078, div_den 0.079, div_general 0.588
- job 4: mul: mul_one_x 0.001, mul_one_y 0.000, mul_both 0.000, mul_den 0.010, mul_num 0.010, mul_cross_ad 0.008, mul_cross_cb 0.010, mul_general 0.959; div: div_one_x 0.000, div_one_y 0.000, div_both 0.000, div_cross_ad 0.008, div_cross_cb 0.011, div_num 0.010, div_den 0.009, div_general 0.962
- job 8: add: add_zero 0.000, add_den 0.236, add_general 0.763; sub: add_zero 0.000, add_den 0.243, add_general 0.757
- job 9: add: add_cross 0.000, add_den 0.006, add_num 0.000, add_general 0.994; sub: add_cross 0.000, add_den 0.004, add_num 0.000, add_general 0.996
- job 10: mul: mul_one_x 0.000, mul_one_y 0.000, mul_den 0.003, mul_num 0.002, mul_cross_ad 0.002, mul_cross_cb 0.002, mul_general 0.990; div: div_one_y 0.000, div_cross_ad 0.004, div_cross_cb 0.002, div_num 0.003, div_den 0.004, div_general 0.988
- job 11: add: add_cross 0.004, add_den 0.002, add_num 0.002, add_general 0.992; sub: add_zero 0.001, add_cross 0.002, add_den 0.003, add_num 0.003, add_general 0.992; mul: mul_one_x 0.001, mul_both 0.001, mul_den 0.004, mul_num 0.003, mul_cross_ad 0.005, mul_cross_cb 0.002, mul_general 0.983; div: div_cross_ad 0.002, div_cross_cb 0.002, div_num 0.002, div_den 0.003, div_general 0.991

### Counts per operation (counting build; equal between the rows except Q32's own)

| job | row | gcd | gcd work | mul | add | exact div | lookup searches | probes | found | appended | reads |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

### The known-answer skips (counting build, with the generator's prediction from the gates)

| job | skip | operations | gcds | gcd work | divisions | multiplications | additions |
| --- | --- | --- | --- | --- | --- | --- | --- |

The gates checked every counter above against the generator's prediction (spec v0.2 §14.1 item 2): 

### Memory against reuse (jobs sorted by W/N at the end)

| job | W/N | B_used Q32 | B_used TC | B_heap Q32 | B_heap TC |
| --- | --- | --- | --- | --- | --- |
| 3 | 0.012 | n/a | n/a | n/a | n/a |
| 2 | 0.513 | n/a | n/a | n/a | n/a |
| 10 | 0.608 | n/a | n/a | n/a | n/a |
| 8 | 0.664 | n/a | n/a | n/a | n/a |
| 9 | 0.676 | n/a | n/a | n/a | n/a |
| 11 | 0.705 | n/a | n/a | n/a | n/a |
| 4 | 0.712 | n/a | n/a | n/a | n/a |
| 1 | 0.996 | n/a | n/a | n/a | n/a |

### The idle and disturbance checks

| job | block | attempt | seconds | disturbed | runner ticks | spawns | forks | unexplained | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot rounds | 1 | 1.597 | yes: 2 unexplained forks | 1 | 8 | 10 | 2 | process_api 1, sh 0, environment-man 0, claude 2, bash 0 |
| 1 | pilot rounds | 2 | 1.601 | yes: 1 unexplained forks | 1 | 8 | 9 | 1 | process_api 0, sh 0, environment-man 0, claude 1, bash 0 |

Idle checks: 2 (0 busy). Host rates R_h: {}.

### Round 1, for reference only

Round 1's verdicts per job are in [`../results-q64-b3b88b6/results.md`](../results-q64-b3b88b6/results.md) (Q64, official) and [`../results-q32/results.md`](../results-q32/results.md) (the Q32 run). Round 1 enters no round-2 measure.

## Limitations

- **Textbook only.** Neither row is optimized (spec v0.2 §0, decision 5). The verdicts compare two plain baselines; they do not compare the designs at their best.
- **Every result is kept**, in both rows, as in round 1. Chains do not update in place.
- **64-bit arithmetic only.** At this width a gcd is cheap, so time differences come mostly from structure. The gcd work is equal by construction, and is reported to make that visible.
- **The profile perturbs what it measures** (spec v0.2 §13.4 items 8 and 9). Its fences stop the overlap between phases, so its shares and its per-phase differences between the rows are indicative only.
- **Jobs 5-7 are not run.** Round 2 says nothing about the low-reuse jobs beyond Q32's capacity.

