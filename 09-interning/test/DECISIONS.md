# Harness decisions

Choices made where research/rule.md and spec v0.1 are silent, each the plainest option found. Each is **pending the
author's review**. None changes a threshold, band, count or order that the rule fixes. 08's decisions G-a, G-c, G-d,
G-h, R-a to R-c, R-g, R-i to R-o, A-a, A-b and A-g carry over as the rule says. Items marked **ambiguous** are also
listed at the end.

## Generator (gen.py, l1model.py)

- **G-1. 08's code is copied, not imported.** `draw_shared`, the P50 denominators, `vals`, the G3 operations and the
  Builder are copied from `08-skeletons/test/gen.py`, which is under separate work. The workload, reference and
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
