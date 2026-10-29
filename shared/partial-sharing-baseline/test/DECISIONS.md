# Harness decisions

Choices made where research/rule.md and spec v0.1 are silent, each the plainest option found. None changes a threshold,
band, count or order the rule fixes. Items marked **ambiguous** are also listed at the end for the author.

## Generator (gen.py)

- **G-a. Random streams.** Job n draws from one `random.Random(20261028 + n)`, in this order: denominators, values,
  operations (or chains), then the 10,000 pairs for realized sharing. Job 20 draws nothing; it is rebuilt from job 19.
- **G-b. round(σB).** Python's `round`. Every σB used is an integer, except W32-ALL (σB = 4.5), where the cap B − 6 = 3 binds anyway.
- **G-c. Uniform primes.** NONE's primes, and the 12-bit primes of CH-none, are drawn as uniform integers in the
  range, rejected until prime. That is uniform over the primes in the range.
- **G-d. Redraws.** Reported per job as a total and a maximum. A job is not constructible once one denominator
  reaches 1,000 redraws. LG asserts the pool (19 + 38 = 57 elements; list sizes 872/464/43/23; 12,198 in all) and stops on
  any failure.
- **G-e. Arrival.**
  - Batches are the denominators in order of first appearance in the shuffled values, cut into 20 equal parts, each
    entered in shuffled order.
  - A two-input operation puts its batch-t operand first and the operand from batches 1..t second.
- **G-f. Chain inputs.** CH-closed and CH-all say only $|m| < d$. Numerators are drawn uniformly in [−(d−1), d−1] and
  redrawn until coprime to d, as in G2, so every input is in lowest terms. For CH-all this keeps every input over the
  one denominator. For d = 1 this gives 0.
- **G-g. G6 restarts.** A step makes the first draw plus up to 16 redraws of $y_k$. If the 17th draw fails, the chain's
  current value becomes a fresh input, drawn uniformly. The step keeps its operation and starts again with a new redraw
  budget, so every chain still takes K steps. Redraws and restarts are reported.
- **G-h. Size bound.** Every operation is checked: $H(x)H(y) \le (2^{31}-1)^2$, or $(2^{15}-1)^2$ at width 32. Every
  stored value is checked to fit the width. A violation stops generation.
- **G-i. Realized sharing.**
  - Jobs with fewer than two distinct denominators (ALL, CH-all, W32-ALL) report n/a.
  - In the third figure (relative gcd size), a gcd of 1 counts as bits(1) = 1, taken literally.
- **G-j. Results as operands.** The distinct result denominators used as operands are counted as distinct values. The
  report also gives how many of them are not entered denominators.
- **G-k. Toy mode** (`--toy`, harness tests only): 200 inputs, 400 operations, 20 chains, and D scaled by 1/50 (at least
  min(D, 10)).

## Driver (bench.c) and runner (runner.py)

- **R-a. What is timed.**
  - total = entry + operations + read-out. `create` and `destroy` are not timed, and neither is reading the workload.
  - A rep "lasts" the sum of its r runs' totals. The 50 ms and 10 s tests use that sum, and the rep's time is the sum
    divided by r.
  - In job 19, the clock is read at every switch between entering and operating calls; it is never read inside a call.
- **R-b. Three reps.** The kept rep is the one of minimum total among reps 2 and 3. Its entry, operation and read-out
  times are reported with it.
- **R-c. Cap.** A rep runs to its end. If it lasted more than 10 s, the invocation stops there: no further reps, and the
  row is not timed again on that job. Its time per run is reported. A process is otherwise limited only by the hard stop.
- **R-d. ctl-null.** It is the ctl binary. Gates, counting runs and the pilot's gate run once per implementation (ctl, D1,
  D2, ceil); ctl-null shares ctl's.
- **R-e. Gate.**
  - `audit_changed` runs after every call for every row (a MAY in general, a MUST for Design 2 above 1,024 pieces).
  - Scheduled full audits call lib's `audit_scheduled`. The one after the last call calls `audit`, the complete audit.
  - The gate also checks the read-out against the reference.
  - For the ceiling, the calls are its storing calls (`enter_vector` and vector operations), 3 or 33; every one is
    fully audited.
- **R-f. The ceiling's plan.**
  - The position arrays are built from the workload before timing.
  - The gathers are timed as operations.
  - CH-all's first step adds to a gathered working copy of the chains' $x_0$.
  - Negation uses lib's extension `sk_ceil_gather_signed` (**ambiguous**, see below).
- **R-g. Builds.** The harness compiles bench.c together with lib's six sources, in one gcc call per row, build and width:
  `-O2 -std=c11 -Wall -Wextra`. lib's Makefile adds only `-Werror`. lib/build is neither used nor written.
- **R-h. Unit tests.** lib/tests/unit.c, compiled by the harness in the test build at widths 64 and 32. lib's
  `make test` is not used: it writes lib/build and adds a differential test.
- **R-i. Cores and rotation.**
  - The harness pins itself to core 0. Untimed work is pinned with `taskset` to core 0 or core 1, at most two processes
    at a time, and only in steps 1–5 (and in a re-run's gates and counting runs).
  - The rotation list is ctl, ctl-null, D1, D2[, ceil]; for the arrival pair, job 19's rows and then job 20's. Round k
    starts at position k mod length.
  - A row that is over cap, or stopped by a bug, is skipped and the others keep their positions.
- **R-j. Pilot.**
  - The timed pilot is jobs 3, 6, 9, 12, 15 and 18 (rows ctl, ctl-null, D1, D2), with calibration and 2 rounds. The
    gate pilot is job 20's gate for ctl, D1 and D2.
  - The projection uses the run clock at the end of the pilot.
  - A check failure in the pilot comes before the gates. It is not an item-5 bug: the run stops and reports it.
- **R-k. Bugs after the gates (item 5).**
  - From step 5 on, a check failure, an overflow or another error stops every row of that design for the rest of the
    run. The bug is logged with the job, the invocation, the call, the operation and the step: the overflow report's
    step name, or for a mismatch the first wrong handle.
  - `--rerun D` re-runs D's unit tests, gates, counting runs, calibration and all 12 rounds, and discards D's earlier
    results. Its rounds are paired by round number with the rounds already kept for the other rows.
  - `--rerun all` covers a fix to the shared integer routines.
- **R-l. Resume after a failed gate.** `--resume` re-runs the unit tests and the gates not passed. It re-runs the pilot
  if the pilot did not finish. It is refused once a timed round has run.
- **R-m. Hard-stop clock (ambiguous).** The clock is the run's own wall time: it adds up across the sessions of one run
  (`--resume`, `--rerun`) and stops between them, so time spent fixing a bug under review is not counted. Within a
  session it is checked between invocations, and every process gets a timeout of the time left. A stopped run cannot
  be resumed.
- **R-o. Code versions** (added Oct 28, 2026, after the independent review).
  - Every gate and counting result stores two sha256 hashes: `code`, over the implementation's own files and every
    shared file (the shared integer module, `common.c`, `rows.c`, the common headers, the driver `bench.c`/`api.h`,
    the compiler and flags), and `int`, over the shared integer module (`lib/src/sk_internal.h`) alone (libconf.py).
  - On `--resume` and `--rerun`, any result whose hashes differ from the current code (or that has none) is dropped
    and logged in `dropped_stale`. `--resume` redoes those gates and counting runs; `--rerun D` also re-runs in full
    every design that had one. A shared file is in every row's hash, so a change to it invalidates every row.
  - Each session records the code it ran in `code_sessions`. Tested in test_harness.py (`test_code_hashes_invalidate`).
- **R-n. Rule check.**
  - The sha256 of rule.md as fixed (`ff2b6237…`) is in common.py. Any other content is refused before step 1.
  - The text checked is the text written verbatim into results.md, before the build and the first trial. The final
    report reprints that stored text.
  - `--stub` and `--hard-stop-s` are refused outside `--toy`.
- **R-p. Idle machine before timing** (added Oct 29, 2026, after two independent reviews; procedure only, noted in
  the rule's revision).
  - Before the timed pilot, and before each job's calibration and rounds in step 6 (the arrival pair counts as one
    job), the runner checks that the machine is idle: the 1-minute load average is at most **0.5**, and no other
    process used more than **5% of one core** over a **1 s** sample (from `/proc/*/stat`; the runner itself is
    excluded and has no child then). The thresholds are in common.py.
  - If the machine is busy, the runner logs the load and the busy processes, waits **15 s** and checks again. After
    **10 minutes** busy, it refuses to time: the run stops (`RunStop`) and is reported, with nothing timed on a busy
    machine. The wait counts on the run clock.
  - Every check, idle or not, is stored in `state.json` under `idle_checks`. Toy runs (harness tests) log the check
    and never wait or refuse. Tested in test_harness.py (`test_idle_check`).

## Analysis (analyze.py)

- **A-a. Bootstrap.** For each ratio, a fresh `numpy.random.default_rng(20261028)` draws 10,000 × 12 round indices, the
  same for both rows. The interval is numpy's default (linear) 2.5th and 97.5th percentiles.
- **A-b. Null control.** The ratio is T(ctl)/T(ctl-null). If either row has fewer than 12 rounds, h is n/a and so are the
  job's time verdicts.
- **A-c. Operation time alone** (item 12) uses the job's h from the total-time null control.
- **A-d. G.** G is the sum of `gcd_work` over the phases entry, add, sub, mul and div, core and store. lib also counts the
  ceiling's read-out gcds under a READ phase; they are excluded, since the rule's G covers "entry and operations".
- **A-e. Ceiling against the control.** Item 6 lets X be ceil, so the per-job tables of jobs 1 and 22 give ceil vs ctl
  verdicts. They enter no stratum; item 10 names only D1, D2 and D1 vs D2.
- **A-f. Shares and orderings.**
  - The entry share of time is median entry divided by median total.
  - `store`'s share is given for gcd work only, since nothing can be timed inside a call.
  - The W32 orderings sort ctl, D1 and D2 by median T, by G and by B at the end.
- **A-g. n/a.**
  - A ratio over a zero G is n/a.
  - A design stopped by a bug is n/a until it is re-run.
  - Strata count over cap and n/a beside win, tie and loss.
- **A-h. Output.**
  - results.md holds the rule, the calibration, the run summary and bugs, the workloads with their realized sharing,
    the strata, the per-job verdicts, the item-12 sections, and per-job times and memory.
  - The CSVs are rounds, verdicts, strata, reported, counters, memory and jobs.
  - A run that stopped (at 4 hours, or by the projection or a gate) reports what it has and no verdict.

## Ambiguous or not possible as written (for the author)

1. **CH-all negation.** The ceiling's job "gathers the 1,000 $y_k$, negated where the step subtracts", but the gather of
   spec §9 cannot negate. lib added `sk_ceil_gather_signed` as a marked extension, and the harness uses it.
2. **Hard stop across a re-run.** Item 5's re-run "counts" toward the 4 hours, but a fix under review takes outside wall
   time. Read literally, a re-run after a long review always hits the stop. The harness counts only the run's own time
   (R-m).
3. **G3 in the chains.** "Half of all operands are results" holds for CH-closed and CH-all (realized 0.48). Under G6,
   restarts break it at full size: the realized share is 0.29 for CH-open (2,351 restarts in 8,000 steps), 0.30 for
   CH-muldiv and 0.14 for CH-none. It is reported, and nothing is changed.
4. **"Every row" for gates, the gate pilot and counting runs.** ctl-null is the same binary as ctl and is run once (R-d).
5. **Chain numerators.** The rule gives only $|m| < d$ for CH-closed and CH-all. Coprime numerators were chosen (G-f).
