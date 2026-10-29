# Harness decisions

Choices made where research/rule.md and spec v0.1 are silent, each the plainest option found. Each is **pending the
author's review**. None changes a threshold, band, count or order that the rule fixes. 08's decisions G-a, G-c, G-d,
G-h, R-a to R-c, R-g, R-i to R-o, A-a, A-b and A-g carry over as the rule says. Items marked **ambiguous** are also
listed at the end.

## Generator (gen.py, l1model.py)

- **G-1. 08's code is copied, not imported.** `draw_shared`, the P50 denominators, `vals`, the G3 operations and the
  Builder are copied from `shared/partial-sharing-baseline/test/gen.py` (08-skeletons/test/gen.py at the time), which is under separate work. The workload, reference and
  read-out formats are 08's. The header's third integer marks the fz jobs.
- **G-2. Random streams.** Job n draws from one `random.Random(20261029 + n)`, in this order: the inputs (catalog, set
  or pool first), then the operations (G3) or the chains (each chain's x_0, then per step the operation and y_k).
- **G-3. Inputs.**
  - **MONEY-catalog and CH-money:** 200 distinct prices are drawn by `sample(range(1, 10^5), 200)`, each amount with a
    uniform sign.
  - **SMALL:** S_k is 1 and `sample(range(2, 2^15), k − 1)`. The pair (a, b) is redrawn together until coprime, so
    ±1/1 can occur.
  - **MOD:** the pool is 08's P50 at B = 20 and D = 1,000, with a repeat redrawn (08 G-d). The pair is redrawn until
    a ≠ b and they are coprime.
  - **NEW-add:** 08's P50 construction at D = 1,000 and V = 10. In toy mode D = n_in / 10.
- **G-4. Chains.** Step k = 0's first operand is an entered x_0, so the realized share of operands that are results is
  0.484 at K = 32 and 0.4375 at K = 8 (reported).
- **G-5. Predicted figures.** `l1model.py` models L1 and L1-opt in exact integers, following spec §6, §8 and §17.1 on
  magnitudes and the order of the whole-number list. It shares no code with the C library. Cases come from comparing
  magnitudes. Every modelled value is checked against `fractions`. W is also checked against the distinct magnitudes
  of the reference. The model's own L1 counts (`l1_base`: gcds, gcd work, divisions, multiplications, additions) are
  used only by the harness tests, not by the gates.

## Driver (bench.c, api.h) and runner (runner.py)

- **R-1. Builds.** These are as libconf.py states:
  - **flags:** one gcc call per row and build, with `-O2 -std=c11 -Wall -Wextra`;
  - **sources:** L1 is `l1.c`; L1-opt is `l1.c -DL1_OPT`; ctl is `ctl.c`. Each also takes `common.c` from
    `lib/vendor08`;
  - **library rows:** these are timing builds only, with `-lflint -lgmp`;
  - **unit tests:** these link `l1.c` twice, as two objects.
- **R-2. Unit tests (step 2).** These are layer 1's unit test and its differential test (200 seeds), both in the test
  build. For ctl, "08's" unit tests are also run, built from 08's lib at commit `08bbfbe`, which is extracted
  read-only with `git archive` and checked file by file against the copies (**ambiguous**: 08's test covers every 08
  row).
- **R-3. Exact checks in the gate.** The test build carries counters, so each gate run of L1 and L1-opt also checks
  W after entry and at the end, every case count per phase, and, for L1-opt, every predicted trick counter. L1-opt
  against L1 (spec §17.3 items 1 and 3) is checked once both gates of a job pass. It compares the read-outs, the
  whole-number lists written by the gate, and the lookups and arithmetic counts.
- **R-4. Cases den and general against ctl** (spec §14.1 item 3). The counting build's `count` mode writes each call's
  core gcds, gcd work, exact divisions, multiplications and additions. The runner attributes each operation to its
  predicted case and compares the sums over den and general per operation. The per-case sums go into the report.
- **R-5. Library rows.**
  - **Holder array:** this is allocated per run with exactly N elements, untimed.
  - **Form A:** each element is initialised in the timed phase that stores it.
  - **Form B:** every element is initialised before the run and cleared after it, both untimed.
  - **Read-out:** a fits test, then `get_si`.
  - **fz:** the distinct input denominators and each entry's position are planned before timing. The timed entry
    computes D (one `fmpz_lcm` per distinct denominator), each cofactor once, and the scaling. fz has one form, with
    elements initialised in their timed phase. The driver refuses fz unless every entry shares one denominator and
    every operation is an addition or subtraction.
  - **fz's timing loop is simpler** than the other rows'. Because fz's workloads have every entry first and only
    additions and subtractions, its driver runs entry and the operations as two plain loops, with no per-call test of
    the phase and one add-or-subtract test in place of a switch on the operation. The other rows run one loop over
    every call that tests for a change of phase and switches on the operation. This slightly favours the ceiling, so a ceiling gap is, if anything, slightly
    overstated. The effect is a few instructions per call, against the work of an `fmpz` addition and the store.
- **R-6. Versions.** Every library row's output line (calib, rep, mem, version) carries the GMP and FLINT versions,
  `flint_get_num_threads()` and the form. A row whose thread count is not 1 is a bug. The versions are also in the
  results file. The scalar rows' lines carry only the row's name.
- **R-7. Memory run** (rule item 6).
  - `mallopt` sets both thresholds in every row and every mode.
  - The heap baseline is taken just before `create` or the holder array. The figures are taken after the last entry
    and at the end, before read-out.
  - For the libraries, B_used is element size × stored values, plus fz's D (8 bytes), plus the hooks' live requested
    bytes. B_heap is heap growth less element size × (N − stored).
  - The untracked-allocation test compares B_heap with element size × stored, plus the live blocks' usable sizes,
    plus 16 bytes per live block, plus 64 KiB.
  - The hooks keep their table in `mmap` memory, with tombstones on free.
- **R-8. Form calibration.** An invocation's total is the minimum of reps 2 and 3 (08 R-b). A rep over the cap counts
  as infinite. When the two medians tie, form A is chosen.
- **R-9. Pilot.**
  - The pilot is jobs 1, 3, 5, 7 and 10, every row, with form and r calibration and 2 rounds.
  - The "pilot's wall time" is the wall time of step 3. The projection is the run clock at its end plus 8.5 minutes ×
    max(1, wall / 60 s).
  - A projection over 45 minutes stops the run (`pilot_stopped`). `--resume` and `--rerun` are then refused until the
    author decides.
  - `--pilot-only` stops after step 3. `--resume` then continues with step 4.
- **R-10. Bugs in library rows** (rule item 5). Reviewing the driver cannot be automated, so a check failure makes the
  row n/a on that job, and it is logged for review. After a driver fix, `--rerun fmpq` (or mpq, fz) re-runs that row
  in full.
- **R-11. Division by zero** (spec §10 item 4). The generator asserts that no divisor is zero, and the driver refuses
  an entered denominator that is not positive.
- **R-12. Repeats** (rule item 2). On jobs 3, 5 and 10, every counting run (counters, memory reports, per-call
  counts) and every memory run is repeated, and the second must be identical.
- **R-13. Idle machine before any timed run** (rule.md, revision of Oct 29, 2026; the same check as 08's R-p).
  - **When:** before the pilot (before its wall time starts, so a wait does not enter the projection factor) and
    before each job's calibration and rounds in step 6.
  - **Test:** the 1-minute load average (`os.getloadavg`) at most 0.5, and no other process using more than 5% of one
    core, measured from `/proc/<pid>/stat` (utime + stime) over a 1-second sample. The runner's own process is
    excluded; it has no child while it checks. The thresholds are in `common.py`.
  - **If busy:** the runner logs the load and the busy processes, re-checks every 15 seconds, and after 10 minutes
    refuses to time: the run stops (`run_stopped`) and is reported to the author. Every check is recorded in
    `state.json` (`idle_checks`) and every wait or refusal in `run.log`. The wait counts on the run's clock.
  - **Toy runs** (harness tests) log the check and never wait or refuse.
  - The load threshold of 0.5 allows the harness itself and ordinary background noise; a timed invocation running
    alone on core 1 is what the check protects.
- **R-14. The hard-stop clock starts at the build**, just before step 1, not at the start of step 1 as the rule's
  Run plan states. The builds (every row binary, the unit tests and 08's unit test) take a few seconds and are counted
  on the run's clock. This errs on the safe side: the run can only stop earlier than the rule requires, never later.
  The pilot's projection includes the same seconds.

## Analysis (analyze.py)

- **A-1. Comparisons.** These are L1 and L1-opt, each against ctl, fmpq, mpq, fz (on the fz jobs) and the best
  library, and L1-opt against L1. Gcd work, multiplications and the per-case ratios are given against ctl, and for
  L1-opt also against L1. Lookups are given for L1-opt against L1 only.
- **A-2. The best library** is the one of fmpq and mpq with the lower median T. Its own verdicts are given beside it.
- **A-3. L1's B_used** is checked against 8 + (8W + 4S)/N from its own report ("formula check").
- **A-4. Unfinished runs.** A run stopped by the pilot, a gate or the hard stop, or stopped after `--pilot-only`,
  reports what it has and no verdict. The pilot section gives each row's median total over the 2 pilot rounds.
- **A-5. Rule item 12, item by item.**
  - **Realized reuse:** N, W, W/N and 2N/W after entry and at the end, the case shares per operation, and the share
    of operands that are earlier results.
  - **Counts per operation:** a table per job, row (L1, L1-opt, ctl) and phase, giving the number of calls and the
    totals of gcds, gcd work, multiplications, additions, exact divisions, lookup searches and probes, whole numbers
    found and appended, index comparisons and cases. Every counter per phase, core and store is in `counters.csv`.
  - **Per case:** per job and case, the number of operations and L1's and ctl's gcds, gcd work and multiplications
    with their ratios, and L1-opt's against L1's.
  - **L1-opt's tricks:** per job and trick, every counter of spec §17.2 summed over entry and the operations, with
    the generator's prediction beside it for O-1 to O-5. The runner keeps the predictions in `state.json`
    (`predicted_tricks`). Also the share of L1's work that each trick removes: lookups for O-1 and O-2; gcds and gcd
    work for O-3; divisions and multiplications for O-4; gcds, gcd work, divisions and additions for O-5. A last row
    gives all tricks together.
  - **Store's share:** `store` does no arithmetic, since it only interns, so its share of gcd work is 0 by
    construction, and its share of lookups is 1. Both are still computed and reported with that note, so that a
    departure would show; the rule asks for them.
  - **Gcd work against time:** for L1 against ctl, and L1-opt against ctl and L1, a table of the gcd-work,
    multiplication and time verdicts. A job with a gcd-work or multiplication win but no time win is listed as a
    candidate for a wider-number test.
  - **Limitations:** the one-row-with-every-trick note heads the verdicts and the tricks table, and both limitations
    are listed at the end. The chain note stays beside every chain verdict against fmpq, mpq and the best library.
  - **Operation time alone:** given against ctl, fmpq, mpq and fz (on the fz jobs), and for L1-opt against L1, each
    verdict with its ratio and interval. It excludes entry and read-out.
  - **Lookups:** the verdict for L1-opt against L1 shows the lookups skipped by O-1 and O-2 beside the ratio. The gcd
    work and multiplication verdicts show their two counts.
  - **Order (rule item 10):** the strata come first, and the per-job verdicts and magnitudes are listed beneath.

## The Q32 rows (rule, revision of Oct 29, 2026; spec §18)

- **R-15. Rows and capacity.** The implementations `L1q` and `L1oq` (rows L1-Q32 and L1-opt-Q32) are built like L1
  and L1-opt with `-DL1_Q32`; `L1q` and `L1g` also compile `lib/src/plk.c`, their lookup, as its own translation unit,
  as every L1 binary compiles `vendor08/common.c` (lib D-25; its code hash covers `plk.c` and `plk.h`). Step 1 records, from the generator's predicted W at the end, the jobs over the capacity
  of 32,768 (`q32_over_capacity` in `state.json`, with W); on those jobs the Q32 rows have no gate, counting, memory
  or timed run, and the analysis reports them as over capacity. With the rule's seeds these are jobs 5, 6 and 7.
- **R-16. The lookup gate** (spec §18.1 item 4) is `L1g`, a test build of L1 with `-DL1_LKGATE`. In step 4 it runs
  `gate` on every job, like L1; once both have passed, every counter line of its gate run (lookup searches and probes
  per phase and part included) must equal L1's, and its read-out and whole-number list must be L1's. It is never
  timed, and is no row.
- **R-17. Q32 against Q64** (spec §18.3 item 1). In step 4, once both gates of a job pass, each Q32 row's read-out and
  whole-number list must equal its Q64 row's, and its counter lines must equal the Q64 row's with O-7's bytes avoided
  halved (lib D-23); L1-opt-Q32 is also checked against L1-Q32 as L1-opt against L1. In step 5 the same counter check,
  and equal per-call core counts, are made on the counting runs; a failure is a bug (rule item 5). The per-case sums
  (spec §14.1 item 3) are of L1, L1-opt and ctl only, since the Q32 rows' per-call counts equal their Q64 rows'.
- **R-18. The Q64 binaries.** The Q64 bench binaries built from the current sources are byte-identical to those built
  from `b3b88b6`'s (timing, counting and test builds of L1, L1-opt and ctl; lib D-18), so the Q64 rows re-run are the
  binaries measured. Checked after lib D-25 by building both trees at one path: all 12 Q64 and library bench binaries
  byte-identical.
- **A-6. Analysis after the revision.**
  - **Verdicts:** the Q32 rows only, each against its Q64 row (time, B_used, B_heap) and against ctl, fmpq, mpq, the
    best library and fz (time, B_used, B_heap); gcd work, multiplications and lookups are checked equal (R-17), not
    compared. On an over-capacity job each such verdict is "over capacity", with W.
  - **The band h** of every time verdict is this run's L1 against L1-null (rule item 7), the Q32 rows' included. This
    is a documented necessity, not a change to the author's safeguard 2, which now reads "the re-run Q64 rows are used
    only for the paired Q32-vs-Q64 comparison and for the band h of the same run": h bands the verdicts of the rounds
    it comes from, so it must come from the same rounds, and `b3b88b6`'s h cannot serve this run. The wording is
    flagged in the README for the author's confirmation.
  - **Changed Q64 verdicts:** the re-run's Q64 verdicts (every comparison of the run at `b3b88b6`) are computed only to
    be compared with `verdicts.csv` of `b3b88b6`, read with `git show`; every (job, X, Y, measure) whose verdict
    differs is listed with both ratios, with no choice between them. They are written to `q64_rerun_verdicts.csv`,
    apart from `verdicts.csv`, and enter no stratum.
  - **The statement** that the Q32 rows were designed after the Q64 results had been seen heads the verdicts and is
    repeated under Limitations.
  - **The model table:** per job, after entry and at the end, for each layer-1 row: W and N from the generator, S from
    the growth rule, S/W, E, the predicted bits per rational, B_used × 8 with an "identity" check to 1e-6 bits, and
    B_heap × 8 with its ratio to the prediction. The table says the B_used column is an accounting identity and only
    B_heap tests anything.
  - **Strata:** the Q32 comparisons, with the number of the stratum's jobs and of its over-capacity jobs; the total of
    over-capacity jobs heads the table, which says the Q32 strata cover fewer jobs.
  - **Gcd work against time** (rule item 12) is reported for the Q32 rows: per job, L1-Q32 and L1-opt-Q32 against ctl,
    their gcd work and multiplications (their own counting runs, equal to their Q64 rows', by the fixed bands; reported,
    not verdicts) beside their time verdict against ctl in this run, with the candidates for a wider-number test;
    over-capacity jobs are marked. L1-opt-Q32 against L1-Q32 has no time verdict (the Q32 rows are not compared with
    each other) and so no line. L1's and L1-opt's table depends on the Q64 time verdicts and is official at `b3b88b6`.
    The counts tables (per case, per operation, tricks) are kept; they are deterministic.
  - **Links:** the report links the archived results (`../results-q64-b3b88b6/results.md`) wherever it refers to the
    official Q64 verdicts, and the changed-verdict list reads `verdicts.csv` from the archive (its sha256 checked), or
    from `git show b3b88b6:...` if the archive is not intact.
- **R-19. The pilot's projection, updated** (rule, revision: Within the hard stop). The remaining-work estimate is
  11.5 minutes (from 8.5: gates 33 -> 60 runs, 1.5 -> 2.7 minutes; counting and memory runs 115 -> 155 with repeats,
  1 -> 1.4; calibration and rounds, 69 -> 85 timed invocations per round, 6 -> 7.4), and the pilot's nominal wall time
  1.2 minutes (from 1: 31 -> 37 invocations per pilot round, the Q32 rows running on pilot jobs 1, 3 and 10), so a
  pilot at the estimated speed gives factor 1 and does not count the added rows twice. The 45-minute limit and the
  60-minute hard stop are unchanged (`common.py`; `test_pilot_stop_rule`).
- **R-20. The official data, and the results folder** (rule, revision: the official data). `test/results/` as the
  run of `b3b88b6` left it (the four committed files and the raw, uncommitted `state.json` and `run.log`) is copied,
  unchanged, to `test/results-q64-b3b88b6/`, to be committed with the code; `common.ARCHIVE_SHA256` records each
  file's sha256. The run under the revision writes to `test/results-q32/` (`common.RESULTS`). The runner refuses to
  start if an archived file is missing or differs, or if its output folder is the archive or the original
  `test/results/` (`common.PROTECTED`), never deletes there, and re-checks the archive at the end; `analyze.py`
  refuses to write a report into either (`test_official_data_protected`). Before this, a fresh run would have deleted
  `state.json` and `run.log` of `b3b88b6` and overwritten its `results.md` and `verdicts.csv`.

## Ambiguous or not possible as written (for the author)

1. **08's unit tests for ctl** (R-2). They cover every 08 row. They are run whole from 08's commit `08bbfbe`, extracted
   read-only.
2. **fz's form** (R-5). The spec offers forms only for fmpq and mpq, so fz has one: elements initialised in their
   timed phase.
3. **"Every output line"** (R-6). Taken as every library row's line. The scalar rows link no library.
4. **The GMP header** is at `/usr/include/x86_64-linux-gnu/gmp.h`, not `/usr/include/gmp.h`. The compiler finds it by
   default, so nothing changes.
5. **The hard-stop clock** starts at the build, a few seconds before the rule's "start of step 1" (R-14). This errs on
   the safe side.
6. **fz's simpler timing loop** slightly favours the ceiling (R-5).
7. **The band h after the revision** comes from the re-run L1 against L1-null, of the same rounds: a documented
   necessity, with safeguard 2 worded to include it; pending the author's confirmation (A-6, README).
8. **The pilot's projection** is updated for the Q32 rows (R-19): estimates only; the 45-minute limit is unchanged.

# Round 2 harness (rule-r2, spec v0.2), Oct 29, 2026

New files beside round 1's, which are kept unchanged (spec §16 item 2): `bench_r2.c`, `api_r2.h`, `r2_common.py`,
`r2_model.py`, `r2_gen.py`, `r2_libconf.py`, `r2_checks.py`, `r2_runner.py`, `r2_analyze.py`, `test_r2_harness.py`.
Each choice is **pending the author's review**.

- **R2-H1. The fixed rule's pin.** `r2_common.RULE_SHA256` holds the sha256 of `research/rule-r2.md` as committed
  at `ba3f647`; a measured run is refused if the file differs or holds the word "PROPOSED".
- **R2-H2. Workloads.** `r2_gen.py` calls round 1's `gen.gen_job` unchanged, checks each workload's sha256 against
  round 1's pinned `results-q32/state.json` (all 11 identical), then runs `r2_model.py` for the predictions (every
  counter per phase, W and the whole-number list's hash) and checks its values against the reference.
- **R2-H3. A lean runner.** The runner imports no numpy and parses read-outs as bytes (the read-out and the reference
  share one format), so the process forked for each timed invocation is small; generation runs as its own process,
  the analysis only after the last timed block. Inside a timed block it only forks, waits and reads a few lines of the
  last output (r after calibration; an over-cap rep); read-out checks, logs and state are written after the block.
- **R2-H4. Spawning and the hard stop**: as rule-r2 item 5 (fork; `sched_setaffinity` to core 1, `dup2` to a file,
  `execv`, `_exit(127)`; one blocking `waitpid`; `setitimer` armed with the time left, its handler kills the child
  and raises). Untimed steps use `subprocess` with `taskset`, two at a time; the pool is joined, and the runner
  asserts it is single-threaded before every timed block.
- **R2-H5. The disturbance check** compares process start times to recognize a reused pid, counts new kernel threads
  as explained forks, and treats a runner over 2 ticks in a block as a harness bug that stops the run (the rule calls
  it a bug but does not say what follows; the limit is replaced by 1 tick per launch plus 2, R2-H16). Pilot blocks record host ticks without flagging; every other test applies,
  and a disturbed pilot block is also re-run once.
- **R2-H6. Calibration and rounds** are one timed block per job; Q32-null takes Q32's r; rounds rotate Q32,
  Q32-null, TC by one per round; the rep kept is the minimum of reps 2-3 (round 1's R-b).
- **R2-H7. The profile.** Every invocation uses the seed 20261029 (so its sampled calls are the same each time) and
  K = 32, at the timing build's calibrated r, 3 reps; the rep kept is the one of minimum batch total among reps 2-3,
  with its phase figures. Batch totals exist per part only (entry, operations, read-out), so estimated ns per
  operation are given per part; per job phase (add, sub, mul, div) and per case only shares and mean corrected times
  (**ambiguous**: spec §13.4 item 7 speaks of each job phase's batch time).
- **R2-H7a. The memory run's stdout buffer** (build review, Oct 29, 2026). The C library allocates stdout's 4 KB
  buffer on the heap at the first `printf`, which in the memory run fell between the after-entry heap figure and the
  end figure, so only B_heap (end) included it. `bench_r2.c` now gives stdout a static buffer before any output, so
  neither figure includes it.
- **R2-H7b. The profile's M per operation** (build review fix, Oct 29, 2026). The driver sums the growth cycles over
  a rep's r runs, like every profile accumulator; the batch figures are divided by r, and M now is too (it was r
  times too large). `test_profile_m_per_run` checks it with a synthetic profile at r = 4 against r = 1.
- **R2-H8. Bugs.** A check failure before the gates (the pilot) stops the run; after them it stops that row (Q32 with
  Q32-null), and `--rerun Q32|TC` re-runs it in full. A call-for-call or gcd-work difference stops both rows.
  `--resume` continues a run stopped before step 6.
- **R2-H8a. P_prof includes the idle check** (build review, Oct 29, 2026). The pilot's P_prof is timed around the
  whole profiling block, including its idle check (2 s at least). It is kept in: the projection's 36 × P_prof is then
  larger, so the pilot's stop is more conservative, never less.
- **R2-H9. Tests without timing.** The harness self-tests time nothing: the timed-block machinery runs with a
  stand-in driver script; the profiling build is run only for its results and accounting; the disturbance detector
  is shown a real short-lived process started inside a snapshot pair (caught by the fork counter and its parent's
  cutime).

## Revision of Oct 29, 2026: the disturbance check (rule-r2 item 5; the author's option 3)

The first round-2 run stopped in its pilot (`results-r2/`, committed at `193ae57`). Job 1's pilot block counted 2
unexplained forks, and its re-run 1, against a limit of 0. The fork counter of `/proc/stat` also counts the
short-lived threads of the host `claude`. No other process used a tick, and no new process was seen. The author chose
option 3 on Oct 29, 2026. `research/rule-r2.md` carries the dated revision. An independent review found it sound, with
minor fixes (R2-H15). The author approved its numbers on Oct 29, 2026: the host floor of 5 ticks per block, the margin
of 2 times the pilot's peak rate times the block length, and a host ceiling of 4 ticks per second (the reviewer's
recommendation, replacing the proposed 10). No timed measurement was run for this change. Each choice below is
**pending the author's review** except those numbers.

- **R2-H10. New processes by process, not by thread** (`r2_runner.disturbance`). This replaces R2-H5's fork test.
  - **Snapshots.** These list the entries of `/proc`, one per thread group, each keyed by its id and start time.
  - **Threads.** A thread of a process present in both snapshots belongs to that process. Its CPU time is in the
    group's `utime + stime`. Each host's thread count is recorded before and after.
  - **The fork counter** is kept, reported only: `forks_rise`, and `forks_unattributed`, which is the rise less the
    runner's spawns and the new kernel threads. It flags nothing.
  - **Flags.** A block is disturbed by:
    - another process present in both snapshots over 5 ticks;
    - a new process (in the second snapshot only, a zombie included) of at least 1 tick;
    - a host over its allowance.

    Each count is of `utime + stime + cutime + cstime`, unchanged. A new process under 1 tick is listed as
    `new_quiet`, reported only. The approved 1-tick threshold is kept.
  - **Short-lived processes** are caught by their CPU time. It lands in the `cutime + cstime` of the process that
    reaped them, against that process's limit: a host's allowance, another process's 5 ticks, or a new process's
    1 tick. Orphans are reaped by process 1, a host.
  - **The limit, stated in the rule.** A process that exits within the block and adds less than its reaper's
    remaining limit is not flagged, for example `/bin/true` started by a host. A child process of a host is not
    attributed to the host while it lives: it counts as a new process. Only its CPU time after it is reaped counts in
    the host's allowance.
- **R2-H11. The host allowance and ceiling** (`r2_common`; approved by the author, Oct 29, 2026).
  - The allowance is `5 + ceil(2 R_h L)` ticks, as before. `R_h` comes from the pilot's blocks (`K.host_rates`; R2-H15
    (c)). The floor of 5 and the factor 2 are approved.
  - Each block's allowance is now written into its record.
  - The stop after the pilot at 1 tick per second is replaced by `HOST_RATE_CEILING` = 4 ticks per second, applied to
    the measured rate.
- **R2-H12. The measured process's CPU time** (`bench_r2.c`; reported only).
  - `rep()` reads `CLOCK_PROCESS_CPUTIME_ID` and `CLOCK_MONOTONIC` once before and once after a rep's r runs. Both
    reads are outside every timed part. `calib` and `rep` lines gain `cpu=` and `span=` after `row=`. The parsers
    read only fields 1-5, so they are unaffected.
  - The runner stores the kept rep's `cpu_ns`, `span_ns` (per run) and `cpu_share` = cpu / span, and every rep's pair.
    These go into `rounds.csv` and into a report table: per job and row, the median and lowest share, and the median
    CPU time beside the median total.
  - The timing build reads `CLOCK_MONOTONIC`, not a cycle counter. Only the profiling build reads the TSC. So the CPU
    time stands beside the CLOCK_MONOTONIC time of the same rep. Its span includes create and destroy, which the
    timed total excludes.
- **R2-H13. The pin** (R2-H1). `RULE_SHA256` is pinned to the revised file with the author's approval in place of the
  PROPOSED marks and the review's fixes (was `698ee612...` while the marks remained). `test_rule_fixed_and_pinned`
  covers both states.
- **R2-H14. Self-tests** (R2-H9 amended), all untimed:
  - `test_disturbance_synthetic` is updated: a fork no snapshot saw, and 500 host forks with a rise in its threads,
    do not flag; a host's reaped child over the allowance flags; a new process under 1 tick is reported; a new
    process that reaped a child flags.
  - `test_host_threads_do_not_disturb`: a real host process creates 60 short-lived threads within the snapshot pair.
    The block is not disturbed, the fork counter rises by at least 60 (reported), and no thread appears as a process.
    The threads' ticks are charged to the host, and a zero allowance would flag them.
  - `test_disturbance_catches_short_lived_process`: within the snapshot pair, an outside process starts a 0.3 s child
    that exits, and another starts a child that stays alive. The first is caught in its reaper's `cutime`, both as
    another process and when its reaper is a host with a tight allowance. The second is caught as a new process.
  - `test_host_allowance_from_pilot`: `R_h` comes from four stand-in pilot blocks, and the allowance follows from it
    (claude at 1.25 ticks per second gives 30 ticks over 10 s). It is applied at 30 and 31 ticks.
  - `test_pilot_stops`: the ceiling is applied, and 1.25 ticks per second no longer stops the run.
  - The stand-in driver prints `cpu=` and `span=`, and `test_timed_block_machinery` checks the kept rep's figures.
    The real driver's new `rep()` runs in the gate, counting and memory modes of the tests, which do not print it;
    no timing mode was run.
- **R2-H15. The independent review's minor fixes** (Oct 29, 2026).
  - (a) **Undetected: a child of a process that ignores `SIGCHLD`.** The kernel reaps it without adding to its
    parent's `cutime + cstime`, so one that starts and exits within a block is charged nowhere. Stated in rule-r2 item
    5 ("Limits") and in its Limitations. Not detected.
  - (b) **A child that existed before the block and is reaped during it** (`r2_runner.disturbance`). Its reaper's
    `cutime + cstime` gains its whole lifetime. Its four tick fields at the first snapshot are now taken off the rise
    of its parent at the first snapshot, when that parent is present in both snapshots, and at most the rise of that
    parent's `cutime + cstime` (so a parent that ignores `SIGCHLD` loses none of its own time). An orphaned child (its
    parent gone at the second snapshot) gets nothing taken off: its reaper is charged its whole lifetime, the
    conservative side. Children of the runner are skipped (its `cutime` is ignored). The ticks taken off are
    recorded per process (`credited`) and reported. Test: `test_existing_child_exit_charged_for_the_block_only`
    (synthetic cases, and a real child of about 30 ticks that exits during the block).
  - (c) **Resolution and scaling.** `R_h` is the highest measured rate, but at least `HOST_RATE_MIN_TICKS` = 1 tick
    over the longest pilot block (the finest rate the pilot resolves; the longest, so a short block does not inflate
    it). The ceiling of 4 ticks per second applies to the measured rate, so the floor alone cannot stop the run. Both
    are recorded (`host_rates`, `host_rates_measured`). The limit of another process is now `5 + ceil(1 x L)` ticks
    over a block of L seconds (`K.other_limit`, `DIST_OTHER_RATE` = 1 tick per second, 1% of one core). **The rate of
    1 tick per second is the builder's choice**, taken from the host stop the author approved before the revision; it
    is not among the numbers the author approved on Oct 29, 2026 for this revision, and is flagged for review.
  - (d) **CPU time for every timed rep.** `finish` now stores `reps_cpu` (cpu and span per rep) for every calibration
    and every timed invocation, over-cap ones included, and the calibration records in `state.json` carry it.
  - (e) The rule's header now says it is fixed with the revision approved.
  - (f) **The stopped run's record.** `results-r2/` as committed at `193ae57` is copied unchanged (each file from git,
    checked equal to the working copy) to `results-r2-stopped-193ae57/`. It is added to `PROTECTED` with each file's
    sha256, so the runner and the analysis refuse to write there, and the files are checked before and after a run.

## Second revision of Oct 29, 2026: the runner's own ticks (rule-r2 item 5)

The second round-2 run (`results-r2/`, committed at `173fad1`) passed its pilot and stopped at the end of its first
measured block (job 1, 38 launches). The runner used 5 ticks against the fixed limit of 2 per block (R2-H5). Nothing
else disturbed the block. The pilot's blocks launched 2-8 invocations and used 0-1 tick (3 over 26 launches). So the
runner's cost scales with its launches, about 0.12-0.13 tick per launch, and the fixed limit did not. The author
approved the change on Oct 29, 2026: the runner's ticks are recorded and reported, not judged, with one sanity limit
of more than 1 tick per launch (plus a fixed 2 per block, a minor fix found in review). `research/rule-r2.md` carries it as a dated second revision. No timed measurement was
run for this change. The choices below are **pending the author's review**, except the approved limit.

- **R2-H16. The runner's ticks, per launch** (`r2_checks.runner_ticks_check`, `r2_runner.disturbance` and
  `timed_block`). This replaces R2-H5's limit of 2 ticks per block (`RUNNER_TICKS_MAX`, removed).
  - **Recorded.** Each block's record gains `runner_ticks_per_launch` (its `utime + stime` over its `spawns`) and
    `runner_over_limit`. The log line gives the ticks, the launches and the ticks per launch.
  - **Limit.** The run stops as a harness bug only if `runner_ticks > 1 x spawns + 2`
    (`RUNNER_TICKS_PER_LAUNCH_MAX` = 1.0, approved; `RUNNER_TICKS_BLOCK_ALLOWANCE` = 2, a minor fix found in
    review: the block's two snapshots cost a fixed amount whatever its launches, so a 2-launch profile block could
    otherwise stop the run at 3 ticks). A block over the limit is not re-run, since it is not a disturbance.
  - **A block with no launch** is held to the allowance of 2 (more than 2 ticks stops the run). Its per-launch
    figure is n/a. Such a block should not occur.
- **R2-H17. Why the runner's ticks do not touch the timings**, checked against the code (`Spawner` docstring; rule
  item 5).
  - Each rep is timed inside `bench_r2` (`rep()`, after `exec` and the workload's read).
  - While a rep runs, the runner is blocked in one `waitpid`. It has no other thread (asserted before every block),
    no pipe and no polling, and `setitimer` runs no code until it fires.
  - Its ticks fall in the `fork`, in the parsing of the output file after `waitpid` (`quick_parse`), in the next
    launch, and in the two snapshots, which are taken before the first launch and after the last `waitpid`.
  - The child's own work before `exec` (`sched_setaffinity`, `open`, `dup2`) is the child's ticks, not the runner's.
  - **The one unordered piece**, reported in the rule rather than removed: two assignments in the parent between
    `fork`'s return and `waitpid`. They run on core 0 and very likely complete before the child's timing starts,
    since the child still has to `exec`, load and read its workload; any overlap is a few nanoseconds on the other
    core, and the first rep is discarded (in a 3-rep invocation; calibration enters no measure).
    No system call can order it without waking the runner.
  - **Not changed:** the runner's cost per launch, which is mostly `fork` of the Python process.
- **R2-H18. Reported** (`r2_analyze`). The disturbance table gains a column "runner ticks per launch" beside the
  ticks and the spawns. A line under it gives the total over every block (ticks, launches, ticks per launch) and the
  highest block's figure against the limit.
- **R2-H19. The pin.** `RULE_SHA256` is re-pinned to the revised file (was `9b47ea0f...`, the rule of the run at
  `173fad1`), and again after the review's minor fixes (`3ad56ffe...`).
- **R2-H20. The second stopped run's record.** The six files of `results-r2/` as committed at `173fad1` are copied
  unchanged to `results-r2-stopped-173fad1/`. Each is taken from git and checked byte-equal to the working copy. The
  folder is added to `PROTECTED` (`RESULTS_STOPPED_2`) with each file's sha256, so the runner and the analysis refuse
  to write there, and the files are checked before and after a run. `results-r2/` is then emptied, its files deleted
  from the working tree (not committed), so the next run starts in a clean folder.
- **R2-H21. Self-tests** (untimed; the stand-in driver runs in place of `bench_r2`).
  - `test_runner_ticks_per_launch` covers the limit: the stopped block (5 over 38) passes, exactly 1 per launch
    plus 2 (40 over 38) passes, and 41 over 38 stops. A 2-launch block passes at 3 and 4 ticks and stops at 5. A
    block with no launch is held to the 2. The runner's ticks never set `disturbed`.
  - **Many launches.** A real block of 38 launches at the runner's normal cost passes, with its per-launch figure
    recorded and logged.
  - **A busy runner.** The runner is made artificially busy, with 30 ms of CPU per launch, about 3 ticks. It stops
    the run as a harness bug, with the figures in `harness_bugs`.
  - `test_analysis` checks the per-block column and the summary line.
  - `test_vendor08_and_official_data` covers the new archive.
  - `test_block_rerun_once_and_harness_bug` is renamed `test_block_rerun_once`, since its harness-bug case moved to
    the new test.
