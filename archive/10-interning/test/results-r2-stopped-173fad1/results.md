# Design 3, round 2: results

Run started 2026-10-29T16:37:06-05:00.

## The measurement rule (research/rule-r2.md, verbatim, written before the first trial)

sha256 `9b47ea0ffdbbe719f70e5ee3356db829393864f8158cf293142e68d9975e5b1d`

<!-- rule begins -->
# Design 3, round 2: measurement rule

Oct 29, 2026 · **Fixed: approved by the author, Oct 29, 2026, with the revision of Oct 29, 2026 below, whose numbers the author approved the same day.** Revised after the first independent review (spec v0.2 §17): items 5, 7, 9 and 10 and the pilot. This rule measures the two textbook baselines of [spec v0.2](../v0.2.md): **Q32** and **TC**.

**Revision of Oct 29, 2026 (the disturbance check; the author's option 3, approved Oct 29, 2026), made after the stopped pilot and before any measured round (step 6). Its numbers were approved by the author, Oct 29, 2026, after an independent review that found it sound; the review's minor fixes are included.** The first round-2 run stopped in its pilot. Its record ([`../test/results-r2/`](../test/results-r2/), as committed at `193ae57`) is kept unchanged in [`../test/results-r2-stopped-193ae57/`](../test/results-r2-stopped-193ae57/). Its first timed block counted 2 unexplained forks, and the re-run counted 1, against a limit of 0. The fork counter of `/proc/stat` counts threads as well as processes, and the host process `claude` (which cannot be stopped in this environment) creates short-lived threads. No other process used a tick, and no new process was seen. The host also ran at about 1.25 ticks per second, against the fixed stop of 1. What changes:
- **Item 5, the check.** New processes are identified by process (thread-group id), from the two snapshots, not by thread. A thread of an existing process belongs to that process. The fork counter is reported only. The `cutime + cstime` of every process still charge a short-lived child's CPU time to the process that reaped it, against that process's limit. A child that existed before the block and is reaped during it is charged only its ticks during the block. The limit of another process is a floor plus a rate, $5 + \lceil L \rceil$ ticks over a block of $L$ seconds. The check, and what it cannot see (a child of a process that ignores `SIGCHLD`), are stated precisely below.
- **Item 5, the host allowance.** The allowance comes from the pilot, $5 + \lceil 2\,R_h\,L \rceil$ (the floor of 5 ticks and the margin 2, **approved by the author, Oct 29, 2026**). $R_h$ is at least 1 tick over the longest pilot block. The stop at 1 tick per second is replaced by a stop above a ceiling of **4 ticks per second** (**approved by the author, Oct 29, 2026**, as the review recommended). The null control's spread (item 8) remains the safeguard on the timings.
- **Item 4, the process's own CPU time.** It is recorded for every timed rep, calibration included, reported only.
- **Item 13 and Run plan step 3** follow these changes.

- **Harness, the official data.** The stopped run's record is protected like round 1's.

Nothing else changes. The harness pins this file's hash as approved.

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
   - **The process's own CPU time** (revision of Oct 29, 2026: **reported only**). For every timed rep (calibration and rounds), the timing build also reads the process's own CPU time (`CLOCK_PROCESS_CPUTIME_ID`) and `CLOCK_MONOTONIC` once before the rep's $r$ runs and once after. Both reads are outside every timed part, so the timed parts are read as before. Its **CPU share** is the CPU time over the `CLOCK_MONOTONIC` time of the same whole rep, create and destroy included. A share well below 1 means the process lost its core during the rep. Both figures of every timed rep, calibration reps included, are stored in the results (`state.json`). For the rep kept in each round, the share and the CPU time per run are reported beside the time. They enter no measure, band or verdict.

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

   **Check, after each timed block** (untimed, between blocks; revision of Oct 29, 2026). Just before and just after the block, the runner takes two snapshots:
   - **Processes, by process.** The set of processes is the numbered entries of `/proc`, one per thread group: a thread is never an entry. Each process is identified by its id and its start time (field 22), so a reused id is a new process. A process in the second snapshot only is **new**. A thread created during the block by a process present in both snapshots (a host, the runner, or any other) **belongs to that process**: the kernel sums the CPU time of every thread of a group, exited threads included, into the group's `utime + stime`. Its time is charged to that process, and nothing else counts it. Each host's thread count (field 20) is reported before and after.
   - **Ticks.** For every process, `utime + stime + cutime + cstime` from `/proc/<pid>/stat` (fields 14–17). A process's **ticks over the block** are the rise of that sum, or the whole sum for a new process. For the runner itself only `utime + stime` counts, since its `cutime + cstime` are its timed children's.
   - **Short-lived processes.** A process that starts and exits within the block is in neither snapshot. When a process reaps a child, the kernel adds the child's CPU time, and that of the child's own reaped children, to the reaper's `cutime + cstime`. So the short-lived process's CPU time is charged to the process that reaped it, against **that process's limit** below: a host's allowance, the limit of any other process, or the 1 tick of a new process. A child never reaped is still listed at the second snapshot, as a zombie with its own times, and counts as new. An orphan is reaped by its subreaper or by process 1, a host, and is charged there. The runner reaps only its timed children. **Limits:** a short-lived process is caught by its CPU time only. One that exits within the block and adds less than its reaper's remaining limit is not flagged. For a child of a host, its CPU time counts in the host's allowance. **A child of a process that ignores `SIGCHLD`** (or sets `SA_NOCLDWAIT`) is reaped by the kernel without adding to its parent's `cutime + cstime`. If it starts and exits within the block, it is in neither snapshot and its CPU time is charged nowhere, so it **goes undetected**.
   - **A child that existed before the block and is reaped during it.** It is in the first snapshot only. The kernel adds its whole lifetime's ticks to its reaper's `cutime + cstime`, but only its ticks during the block are charged. Its ticks at the first snapshot (all four fields) are taken off the rise of its parent at the first snapshot, if that parent is present in both snapshots, and at most the rise of that parent's `cutime + cstime` (a parent that ignores `SIGCHLD` gains nothing there, so nothing is taken off its own time). A zombie at the first snapshot is thus charged nothing. If that parent is gone at the second snapshot, the child was orphaned and reaped elsewhere, nothing is taken off, and its whole lifetime is charged to its reaper (the conservative side). The ticks taken off are reported per process.
   - **The fork counter** (reported only). The `processes` line of `/proc/stat` counts every process **and thread** created since boot. So it cannot tell a host's threads from new processes, and it no longer flags. Its rise over the block, and that rise less the runner's spawns and the new kernel threads, are reported.

   Excluded from the tick tests are the runner itself (reported separately), the block's timed children, kernel threads (children of `kthreadd`: reported, never flag) and the host processes (below), which have their own allowance. The block is **disturbed** if any of these holds:
   - any other process present in both snapshots used more than $5 + \lceil 1 \cdot L \rceil$ **clock ticks** over a block of $L$ seconds, its reaped children's included: a floor of **5 ticks** (50 ms at 100 Hz; **approved by the author, Oct 29, 2026**) plus a rate of **1 tick per second** (1% of one core, the rate the author approved, Oct 29, 2026, for the earlier host stop; set under the review's fix of Oct 29, 2026, so that the limit scales with the block's length);
   - any new process, other than the runner's timed children, used at least one tick, its reaped children's included (**approved by the author, Oct 29, 2026**). A new process under one tick is reported;
   - a host process used more than its allowance, its threads' and its reaped children's ticks included.

   **Host processes.** An ancestor of the runner that cannot be stopped during the run is a **host process**: for example, the session process that hosts the operator's background command, and process 1. At start, the runner finds its ancestors by following `PPid` to process 1, and lists them in the results. A parent blocked in `wait` uses no ticks. The others are measured, not assumed:
   1. **The pilot records the baseline** (Run plan step 3). For each host process $h$, it records the ticks (all four fields) over each of the pilot's timed blocks, as a rate in ticks per second. Its **measured rate** is the highest rate over those blocks. $R_h$ is the measured rate, but at least **1 tick** over the longest pilot block (**approved by the author, Oct 29, 2026**): a tick is the pilot's resolution, so a host with no tick in the pilot is not taken to use none. During the pilot, host ticks are recorded and do not flag; every other test applies.
   2. **The allowance is then fixed** before step 6, from the pilot's measurement (revision of Oct 29, 2026). Over a block of $L$ seconds, host process $h$ may use at most $5 + \lceil 2\,R_h\,L \rceil$ ticks: a floor of **5 ticks** (**approved by the author, Oct 29, 2026**) plus **2 times** (**approved by the author, Oct 29, 2026**) its rate $R_h$ over the block. The values of $R_h$, of the measured rates and of each block's allowance are written to the results. *Example:* the stopped run's highest host rate was 1.25 ticks per second (2 ticks in a pilot block of 1.6 s). Over a block of 10 s, that host would be allowed $5 + \lceil 25 \rceil = 30$ ticks.
   3. **Only a very busy host stops the run.** If any host's measured rate exceeds a ceiling of **4 ticks per second** (4% of one core at 100 Hz; **approved by the author, Oct 29, 2026**), the run stops after the pilot and is reported to the author before any further step: the machine is not quiet enough to time on. Below the ceiling, a host is held to its allowance. The timings are guarded by the null control (item 8): a host that disturbs the timings shows in the null spread, which widens the band $h$.

   The ticks of the runner's ancestors are thus all counted, threads included. An operator's polling loop is caught by its own ticks, by its children's ticks in its `cutime + cstime`, or by a command it starts that is alive at the second snapshot. Since the fork counter no longer flags, a light loop whose commands exit within the block, under a host and within that host's allowance, is not caught by the check. The operator's duty (above) is unchanged. A disturbed block is discarded, logged with the processes, their ticks and the forks, and **re-run once** (calibration and all rounds; the time counts on the run's clock). A second disturbance of the same job stops the run, which is reported to the author. The runner's own ticks per block are reported. More than **2 ticks** per block (**approved by the author, Oct 29, 2026**) is a harness bug.

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
    - **The idle checks and the disturbance checks** (items 5 and 6): every check, the runner's own ticks per block, the fork counter's rise per block (reported only), the new processes under one tick, the limit of other processes and the ticks taken off for children reaped in the block, and the host processes with their rates $R_h$, and their ticks, allowance and thread counts per block.
    - **The process's own CPU time** (item 4, reported only): per job and row, the median and lowest CPU share of the kept reps, and the median CPU time per run beside the median total.
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
   - **The run also stops after the pilot**, and is reported to the author, if any host's measured rate exceeds the ceiling of 4 ticks per second (item 5; **approved by the author, Oct 29, 2026**), or if either row's profiling slowdown exceeds 1.5 (item 10), so that K can be reconsidered.
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
- **The disturbance check cannot see every process** (item 5). A short-lived process is caught only by its CPU time in its reaper's `cutime + cstime`. A child of a process that ignores `SIGCHLD` is reaped without adding to `cutime`, so one that starts and exits within a block goes undetected. One whose reaper stays within its limit is not flagged either. The null control (item 8) remains the safeguard on the timings.

## Harness

The round-1 harness (`test/gen.py`, `l1model.py`, `runner.py`, `analyze.py`, `bench.c`, `api.h`) is the starting point, with these changes:
- `l1model.py` models spec v0.2: case `zero`, the K counters, Q32's lookups and reads;
- `bench.c` drives Q32 and TC only, and adds a `prof` mode;
- `runner.py` implements items 5 and 6, has no library rows or forms, and enforces the hard stop by kernel timer;
- `analyze.py` reports items 7–13.

**The official data.** Round 2 writes only to **`test/results-r2/`**. The harness refuses to start if its output folder is `test/results/`, `test/results-q64-b3b88b6/`, `test/results-q32/` or `test/results-r2-stopped-193ae57/` (the stopped first round-2 run's record, copied unchanged from `test/results-r2/` as committed at `193ae57`, before any new run can overwrite it). It never writes or deletes there, and it checks the sha256 of every file in those four folders before and after the run. Decisions the builder makes go in `test/DECISIONS.md`, under a round-2 heading, pending the author's review.
<!-- rule ends -->

## The run

- Started 2026-10-29T16:37:06-05:00; **stopped**: harness bug: the runner used 5 ticks in the measured block of job 1 (rule-r2 item 5: more than 2 is a harness bug).
- Run time 0.8 minutes (hard stop 30).
- Rule sha256 `9b47ea0ffdbbe719f70e5ee3356db829393864f8158cf293142e68d9975e5b1d`; the official data after the run: unchanged.
- Host processes (the runner's ancestors): bash[1096], claude[92], environment-man[85], sh[79], process_api[1]
- Code at 2026-10-29T16:37:06-05:00: Q32 `a00bd4454820`, TC `d35c102d1098`
- Unit tests: r2_unit: r2_unit: 489645 checks, 0 failures; r2_diff: r2_diff: 300 seeds, 233379 operations (8210 divisions by zero, 19460 overflows), every case and K-skip met; 977113 checks, 0 failures; r2_unit-asan: r2_unit: 489645 checks, 0 failures; r2_diff-asan: r2_diff: 60 seeds, 46345 operations (1630 divisions by zero, 3765 overflows), every case and K-skip met; 194197 checks, 0 failures
- **Over capacity for Q32** (not run): job 5 (MOD-mixed, W = 40849), job 6 (MOD-muldiv, W = 40798), job 7 (NEW-add, W = 50795)
- **Bug**: {'what': 'the runner used 5 ticks in a timed block (more than 2)', 'job': 1, 'label': 'measured', 'time': '2026-10-29T16:37:54-05:00'}

## The pilot (discarded, except what it records for the run)

- Jobs [1, 3, 10], profiling run on job 1; P = 11.2 s, P_prof = 2.6 s; projection 7.1 min (limit 22).
- Host rates R_h (ticks per second): {'1': 0.5500550055005501, '79': 0.5500550055005501, '85': 0.6207443031462708, '92': 0.5500550055005501, '1096': 0.5500550055005501}.
- Profiling slowdown: Q32 1.226, TC 1.255.

## Verdicts

**No verdict is reported: the run did not finish** (stopped in steps 4-7: harness bug: the runner used 5 ticks in the measured block of job 1 (rule-r2 item 5: more than 2 is a harness bug)).

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
| 1 | Q32 | 1.479 | 25.067 | 1.504 | 0.667 | 1.929 | 3.073 | 4.077 | 0.985 | 0.996 | 2.501 |
| 1 | TC | 1.479 | 25.067 | 1.504 | 0.667 | 1.929 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 2 | Q32 | 1.473 | 18.485 | 1.498 | 0.666 | 1.907 | 2.526 | 3.001 | 1.466 | 0.513 | 2.496 |
| 2 | TC | 1.473 | 18.485 | 1.498 | 0.666 | 1.907 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 3 | Q32 | 1.224 | 20.070 | 1.021 | 0.000 | 0.567 | 1.807 | 2.348 | 1.778 | 0.012 | 2.189 |
| 3 | TC | 1.224 | 20.070 | 1.021 | 0.000 | 0.567 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 4 | Q32 | 1.627 | 31.875 | 1.307 | 0.000 | 0.634 | 3.074 | 4.264 | 1.270 | 0.712 | 2.629 |
| 4 | TC | 1.627 | 31.875 | 1.307 | 0.000 | 0.634 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 8 | Q32 | 1.545 | 19.895 | 1.737 | 0.762 | 2.048 | 2.759 | 3.308 | 1.315 | 0.664 | 2.864 |
| 8 | TC | 1.545 | 19.895 | 1.737 | 0.762 | 2.048 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 9 | Q32 | 1.748 | 28.783 | 2.275 | 0.762 | 2.199 | 2.780 | 3.311 | 1.324 | 0.676 | 3.044 |
| 9 | TC | 1.748 | 28.783 | 2.275 | 0.762 | 2.199 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 10 | Q32 | 1.437 | 16.686 | 0.884 | 0.000 | 0.629 | 2.906 | 3.463 | 1.388 | 0.608 | 1.770 |
| 10 | TC | 1.437 | 16.686 | 0.884 | 0.000 | 0.629 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |
| 11 | Q32 | 1.313 | 16.660 | 1.105 | 0.222 | 0.509 | 2.908 | 3.485 | 1.293 | 0.705 | 1.771 |
| 11 | TC | 1.313 | 16.660 | 1.105 | 0.222 | 0.509 | 0.000 | 0.000 | 0.000 | 0.000 | 0.000 |

### The known-answer skips (counting build, with the generator's prediction from the gates)

| job | skip | operations | gcds | gcd work | divisions | multiplications | additions |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | K-1 | 3993 | 0 | 0 | 7986 (12.1%) | 0 | 0 |
| 1 | K-3 | 585 | 0 | 0 | 1170 (2.0%) | 0 | 0 |
| 1 | K-6 | 676 | 676 (1.5%) | 14592 (1.9%) | 2704 (4.5%) | 0 | 0 |
| 1 | K-7 | 10173 | 0 | 0 | 20346 (26.0%) | 0 | 0 |
| 1 | K-13 | 20000 | 0 | 0 | 0 | 40000 (47.0%) | 0 |
| 1 | K-14 | 30000 | 0 | 0 | 0 | 0 | 0 |
| 1 | K-Q1, K-Q2 | lookups skipped 585 (0.010 of lookups) | reads made 75042, skipped 4958 (K-Q3) | | | | |
| 2 | K-1 | 3970 | 0 | 0 | 7940 (12.2%) | 0 | 0 |
| 2 | K-2 | 21 | 21 (0.0%) | 119 (0.0%) | 42 (0.1%) | 0 | 21 (0.1%) |
| 2 | K-3 | 610 | 0 | 0 | 1220 (2.1%) | 0 | 0 |
| 2 | K-6 | 754 | 754 (1.7%) | 11325 (2.0%) | 3016 (5.0%) | 0 | 0 |
| 2 | K-7 | 10261 | 0 | 0 | 20522 (26.4%) | 0 | 0 |
| 2 | K-13 | 19979 | 0 | 0 | 0 | 39958 (47.1%) | 0 |
| 2 | K-14 | 30000 | 0 | 0 | 0 | 0 | 0 |
| 2 | K-Q1, K-Q2 | lookups skipped 610 (0.010 of lookups) | reads made 74893, skipped 5107 (K-Q3) | | | | |
| 3 | K-1 | 10000 | 0 | 0 | 20000 (54.0%) | 0 | 0 |
| 3 | K-8 | 907 | 0 | 0 | 1814 (9.6%) | 0 | 0 |
| 3 | K-9 | 5477 | 0 | 0 | 16200 (48.8%) | 0 | 0 |
| 3 | K-10 | 928 | 0 | 0 | 1856 (9.8%) | 0 | 0 |
| 3 | K-11 | 5565 | 0 | 0 | 16594 (49.4%) | 0 | 0 |
| 3 | K-12 | 367 | 0 | 0 | 0 | 0 | 0 |
| 3 | K-13 | 20000 | 0 | 0 | 0 | 20000 (39.5%) | 0 |
| 3 | K-14 | 30000 | 0 | 0 | 0 | 0 | 0 |
| 3 | K-15 | 1132 | 0 | 0 | 0 | 0 | 0 |
| 3 | K-Q1, K-Q2 | lookups skipped 6301 (0.105 of lookups) | reads made 65674, skipped 14326 (K-Q3) | | | | |
| 4 | K-1 | 10000 | 0 | 0 | 20000 (51.3%) | 0 | 0 |
| 4 | K-8 | 132 | 0 | 0 | 264 (1.4%) | 0 | 0 |
| 4 | K-9 | 9177 | 0 | 0 | 28974 (60.4%) | 0 | 0 |
| 4 | K-10 | 128 | 0 | 0 | 256 (1.3%) | 0 | 0 |
| 4 | K-11 | 9213 | 0 | 0 | 29096 (60.5%) | 0 | 0 |
| 4 | K-12 | 5 | 0 | 0 | 0 | 0 | 0 |
| 4 | K-13 | 20000 | 0 | 0 | 0 | 20000 (33.8%) | 0 |
| 4 | K-14 | 30000 | 0 | 0 | 0 | 0 | 0 |
| 4 | K-15 | 20 | 0 | 0 | 0 | 0 | 0 |
| 4 | K-Q1, K-Q2 | lookups skipped 565 (0.009 of lookups) | reads made 78877, skipped 1123 (K-Q3) | | | | |
| 8 | K-1 | 3876 | 0 | 0 | 7752 (8.3%) | 0 | 0 |
| 8 | K-2 | 8 | 8 (0.0%) | 53 (0.0%) | 16 (0.0%) | 0 | 8 (0.0%) |
| 8 | K-3 | 907 | 0 | 0 | 1814 (2.1%) | 0 | 0 |
| 8 | K-6 | 1418 | 1418 (2.1%) | 23340 (2.7%) | 5672 (6.2%) | 0 | 0 |
| 8 | K-7 | 15677 | 0 | 0 | 31354 (26.7%) | 0 | 0 |
| 8 | K-13 | 31992 | 0 | 0 | 0 | 63984 (46.7%) | 0 |
| 8 | K-14 | 42000 | 0 | 0 | 0 | 0 | 0 |
| 8 | K-Q1, K-Q2 | lookups skipped 907 (0.011 of lookups) | reads made 120293, skipped 7707 (K-Q3) | | | | |
| 9 | K-1 | 10000 | 0 | 0 | 20000 (17.8%) | 0 | 0 |
| 9 | K-3 | 7 | 0 | 0 | 14 (0.0%) | 0 | 0 |
| 9 | K-4 | 2 | 2 (0.0%) | 15 (0.0%) | 8 (0.0%) | 0 | 0 |
| 9 | K-6 | 407 | 407 (0.6%) | 6478 (0.5%) | 1628 (1.7%) | 0 | 0 |
| 9 | K-7 | 16826 | 0 | 0 | 33652 (26.7%) | 0 | 0 |
| 9 | K-13 | 32000 | 0 | 0 | 0 | 64000 (40.1%) | 0 |
| 9 | K-14 | 42000 | 0 | 0 | 0 | 0 | 0 |
| 9 | K-Q1, K-Q2 | lookups skipped 7 (0.000 of lookups) | reads made 127838, skipped 162 (K-Q3) | | | | |
| 10 | K-1 | 10000 | 0 | 0 | 20000 (63.9%) | 0 | 0 |
| 10 | K-8 | 15 | 0 | 0 | 30 (0.3%) | 0 | 0 |
| 10 | K-9 | 3510 | 0 | 0 | 10220 (47.4%) | 0 | 0 |
| 10 | K-10 | 19 | 0 | 0 | 38 (0.3%) | 0 | 0 |
| 10 | K-11 | 3465 | 0 | 0 | 10116 (47.2%) | 0 | 0 |
| 10 | K-13 | 8000 | 0 | 0 | 0 | 8000 (33.5%) | 0 |
| 10 | K-14 | 18000 | 0 | 0 | 0 | 0 | 0 |
| 10 | K-15 | 3 | 0 | 0 | 0 | 0 | 0 |
| 10 | K-Q1, K-Q2 | lookups skipped 74 (0.002 of lookups) | reads made 31868, skipped 132 (K-Q3) | | | | |
| 11 | K-1 | 10000 | 0 | 0 | 20000 (68.6%) | 0 | 0 |
| 11 | K-2 | 1 | 1 (0.0%) | 3 (0.0%) | 2 (0.0%) | 0 | 1 (0.0%) |
| 11 | K-3 | 9 | 0 | 0 | 18 (0.2%) | 0 | 0 |
| 11 | K-4 | 7 | 7 (0.0%) | 41 (0.0%) | 28 (0.3%) | 0 | 0 |
| 11 | K-5 | 3 | 0 | 0 | 6 (0.1%) | 0 | 0 |
| 11 | K-6 | 2238 | 2238 (8.6%) | 48726 (14.0%) | 8952 (49.4%) | 0 | 0 |
| 11 | K-7 | 1381 | 0 | 0 | 2762 (23.2%) | 0 | 0 |
| 11 | K-8 | 8 | 0 | 0 | 16 (0.2%) | 0 | 0 |
| 11 | K-9 | 1937 | 0 | 0 | 5624 (38.0%) | 0 | 0 |
| 11 | K-10 | 8 | 0 | 0 | 16 (0.2%) | 0 | 0 |
| 11 | K-11 | 1737 | 0 | 0 | 5178 (36.1%) | 0 | 0 |
| 11 | K-12 | 2 | 0 | 0 | 0 | 0 | 0 |
| 11 | K-13 | 7999 | 0 | 0 | 0 | 11998 (37.6%) | 0 |
| 11 | K-14 | 18000 | 0 | 0 | 0 | 0 | 0 |
| 11 | K-15 | 3 | 0 | 0 | 0 | 0 | 0 |
| 11 | K-Q1, K-Q2 | lookups skipped 49 (0.001 of lookups) | reads made 31877, skipped 123 (K-Q3) | | | | |

The gates checked every counter above against the generator's prediction (spec v0.2 §14.1 item 2): TC:1 pass, Q32:1 pass, Q32:2 pass, TC:2 pass, Q32:3 pass, TC:3 pass, Q32:4 pass, TC:4 pass, TC:8 pass, Q32:8 pass, TC:9 pass, Q32:9 pass, Q32:10 pass, TC:10 pass, Q32:11 pass, TC:11 pass

### Memory against reuse (jobs sorted by W/N at the end)

| job | W/N | B_used Q32 | B_used TC | B_heap Q32 | B_heap TC |
| --- | --- | --- | --- | --- | --- |
| 3 | 0.012 | 4.16 | 16.00 | 4.71 | 17.48 |
| 2 | 0.513 | 10.29 | 16.00 | 11.12 | 17.48 |
| 10 | 0.608 | 12.51 | 16.00 | 18.47 | 29.13 |
| 8 | 0.664 | 12.43 | 16.00 | 15.75 | 24.97 |
| 9 | 0.676 | 12.53 | 16.00 | 15.75 | 24.97 |
| 11 | 0.705 | 13.28 | 16.00 | 18.47 | 29.13 |
| 4 | 0.712 | 14.06 | 16.00 | 17.63 | 17.48 |
| 1 | 0.996 | 16.33 | 16.00 | 17.69 | 17.48 |

### The idle and disturbance checks

Processes are told apart by process (thread-group id), not by thread; the fork counter counts threads too and is reported only (rule-r2 item 5, revision of Oct 29, 2026). Host ticks are given as ticks / allowance (none in the pilot) and threads before -> after. Other processes may use 5 + ceil(1 x L) ticks over a block of L seconds (the limit column). Credited: the ticks, before the block, of children that existed at its start and were reaped during it, taken off their parent's rise.

| job | block | attempt | seconds | disturbed | runner ticks | spawns | forks (reported) | forks less spawns (reported) | new processes under 1 tick (reported) | other limit | credited | host ticks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | pilot rounds | 1 | 1.611 | no | 1 | 8 | 8 | 0 | none | 7 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 1 (7->7 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 3 | pilot rounds | 1 | 1.729 | no | 0 | 8 | 8 | 0 | none | 7 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (7->7 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 10 | pilot rounds | 1 | 1.818 | no | 1 | 8 | 9 | 0 | none | 7 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (7->7 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 1 | pilot prof | 1 | 0.563 | no | 1 | 2 | 2 | 0 | none | 6 | none | process_api 0 (6->6 threads), sh 0 (1->1 threads), environment-man 0 (7->7 threads), claude 0 (8->8 threads), bash 0 (1->1 threads) |
| 1 | measured rounds | 1 | 8.379 | no | 5 | 38 | 38 | 0 | none | 14 | none | process_api 0/15 (6->6 threads), sh 0/15 (1->1 threads), environment-man 0/16 (7->7 threads), claude 1/15 (8->7 threads), bash 0/15 (1->1 threads) |

Idle checks: 5 (0 busy). Host rates R_h: {'1': 0.5500550055005501, '79': 0.5500550055005501, '85': 0.6207443031462708, '92': 0.5500550055005501, '1096': 0.5500550055005501} (measured: {'1': 0.0, '79': 0.0, '85': 0.6207443031462708, '92': 0.0, '1096': 0.0}; R_h is at least 1 tick over the longest pilot block).

### Round 1, for reference only

Round 1's verdicts per job are in [`../results-q64-b3b88b6/results.md`](../results-q64-b3b88b6/results.md) (Q64, official) and [`../results-q32/results.md`](../results-q32/results.md) (the Q32 run). Round 1 enters no round-2 measure.

## Limitations

- **Textbook only.** Neither row is optimized (spec v0.2 §0, decision 5). The verdicts compare two plain baselines; they do not compare the designs at their best.
- **Every result is kept**, in both rows, as in round 1. Chains do not update in place.
- **64-bit arithmetic only.** At this width a gcd is cheap, so time differences come mostly from structure. The gcd work is equal by construction, and is reported to make that visible.
- **The profile perturbs what it measures** (spec v0.2 §13.4 items 8 and 9). Its fences stop the overlap between phases, so its shares and its per-phase differences between the rows are indicative only.
- **Jobs 5-7 are not run.** Round 2 says nothing about the low-reuse jobs beyond Q32's capacity.

