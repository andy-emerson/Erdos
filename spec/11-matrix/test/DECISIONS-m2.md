# Builder decisions (11-matrix, round m2: spec v0.2, rule-m2)

Choices made where `v0.2.md` and `research/rule-m2.md` are silent, or where a plain reading needed a detail. Each is the
plainest option found and each is **pending the author's review**. None changes a threshold, band, count or order that
the rule fixes. None adds a structure: the rows hold exactly the structures of spec v0.2 §15.3 (per matrix: TC 1;
Hash-V-fixed 2; Hash-V-grow-map 2 plus (r); Hash-V-pack 2 plus (h) and (p)). m1's decisions (`DECISIONS.md`) stand for
m1 and, where this file says so, for m2. The items marked **ambiguous** are also listed at the end. (The rule's Harness
section names this file; `DECISIONS.md` stays m1's record.)

Files, all new: `lib/src/mh2.c` (the three hash rows), `lib/include/{m2,mh2}.h`, `lib/src/m2common.c`,
`lib/tests/{m2t.h,m2_unit.c,m2_diff.c,m2_alloc.c}`; harness `test/bench_m2.c`, `test/api_m2.h`, `test/tc3gate_m2.c`,
`test/m2_common.py`, `m2_gen.py`, `m2_model.py`, `m2_libconf.py`, `m2_checks.py`, `m2_runner.py`, `m2_analyze.py`,
`test_m2_harness.py`. Nothing of m1 is changed: `tcm.c`, `mh.c`, `m1common.c`, `m1int.h`, `m1.h`, `mh.h`, `tcm.h` and
`tcm.diff` are pinned at m1's last commit (`6e9a85e`) and checked before every run; m1's results folders are never
written and are hashed before and after every m2 run. Nothing in `../10-interning/` is changed.

## The rows (lib)

- **M2-D1. A new source beside mh.c.** **Ambiguous.** v0.2 §15.8 says both that "`lib/src/mh.c` keeps one source for the
  hash rows. It gains the setting `MH_PACK`" and that "The m1 sources stay as m1's record. m2's are new files beside
  them." Changing mh.c would change the code that produced m1's results, so the hash rows are one new source,
  `mh2.c`: mh.c's text with the m2 changes, built three ways with `-DMH_GROW` and `-DMH_PACK` (V and the map rewrite
  are fixed, M and lookup are gone), giving the prefixes the spec names, `mhvf_`, `mhvgm_`, `mhvp_`. `mh2.h` refuses
  `MH_GROW` with `MH_PACK`. m1's `mhvf_` and `mhvgm_` keep their own header; no binary links both.
- **M2-D2. What m2 reuses unchanged.** `tcm.c` (TC; v0.2 §15.8), and m1's per-operation record (`m1.h`'s
  `m1_percall`, `m1int.h`, `m1common.c`), which is the same record of the same arithmetic. New: `m2.h` (matrix ids,
  store and operation counters without tau and lookup, the growth and pack records, the accounted total) and
  `m2common.c` (the hooks and the total).
- **M2-D3. Matrices by id.** An input (A, B, C, D: ids 0-3) is 64-bit and entered; a result (the two intermediates and
  the final result: ids 4-6) starts at 8 bits and is built by `madd` or `mmul`. An operation accepts any two finished
  matrices (in Hash-V-pack, packed) of the result's dimensions; its result must be an empty non-input. Names in the
  driver: the result is C in add and mul and R in sop and pos; the intermediates P, Q (sop) and S, U (pos).
- **M2-D4. Zero, integer and +-1 tests by value.** As v0.2 §15.2 says: V reads a, b, c, d before a case is chosen (m1's
  M1-D8, unchanged), and the tests compare those values: zero `a = 0`, integer `b = 1`, +-1 `a = b`, likewise for y. No
  handle is compared with 0 or 1 anywhere (a harness test checks the source).
- **M2-D5. W and the trigger.** W counts the whole numbers stored (0 and 1 only when a cell uses them). T_fix is m1's
  (v0.2 §15.10 item 1) and grow's trigger is unchanged, W + 1 > T/2, so grow's first growth comes at the 9th new whole
  number (W = 8; m1's came at the 7th, M1-D3). Every store, 0 and 1 included, hashes and probes; a store that triggers
  a growth counts the probes of both its sequences (M1-D4, unchanged). A growth reinserts from slot 0 and maps every
  used slot.
- **M2-D6. The packing's order** (v0.2 §15.4 item 3): allocate (l), then (p); copy (h)'s used slots in slot order;
  one forward pass over the cells, in place, rewriting and narrowing each field; if the width shrank, `realloc` the
  cells; the pack hook (a read: the gate reads (h) through `t_whole` here); free (h), then (p). The packed width is
  the smallest of 8, 16, 32 with 2^(w - 1) >= W. "Cells rewritten" is every field, 2 n^2. Hash-V-pack's reported T
  (and so its fill) is its build table's, T_fix. During the build (h) is exactly Hash-V-fixed's table, widening
  included.
- **M2-D7. A failed operation** (an overflow) returns the error and leaves its result unpacked, with its (h); destroy
  frees whatever the matrix holds. An overflow after the gates is a bug, as in m1.
- **M2-D8. The accounted running total** (v0.2 §15.6), memory build only: a global in `m2common.c` that `mh2.c` adds to
  at each allocation of a structure (cells, table, (h), (w), (n), (r), (l), (p)) and subtracts from at each free, the
  `realloc` that narrows the cells subtracting the difference; headers are not structures and are not counted. Its
  peak is reset by the driver (`m2_acct_peak = m2_acct_live`) before each input's build and at the start of the
  operation. TC's total is kept by the driver from the call order, adding a matrix's pairs at its `create` and
  subtracting them at its `destroy`; tcm.c has no code for it. The memory run checks the total against the sum of the
  live matrices' memory reports after creation and at the end, and that it returns to 0 (an error otherwise).
- **M2-D9. The memory build.** **Ambiguous.** rule-m2 item 2 carries over "three builds per row: timing, counting and
  test", while v0.2 §15.6 and the rule's Harness ask for a memory build with the peak tracking that "the timing build
  carries neither". The memory build is a fourth build: the timing build's flags with `-DM2_MEM`, linked with
  `-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free`. It serves the memory runs only and is never timed. (m1's
  memory run used the timing build.)
- **M2-D10. The heap figures.** The wrapper lives in `bench_m2.c` (the driver), keeping the live sum of
  `malloc_usable_size` and its maximum, a `realloc` counted as a free and an allocation. Heap figures are relative to
  the live sum at the start of the run (before the first `create`), so the driver's own arrays are not counted.
  Measure 6's heap figure is the live sum after the input's build less the sum just before its `create`. The heap
  peak is reset at the start of the operation.
- **M2-D11. Reported memory, no verdict.** The peak during each input's build is reported as that input's own rise:
  the most the accounted total rose above its value just before the input's `create`, over the input's cells.
  Hash-V-grow-map's growth peak is the largest accounted total (and heap) read by the growth hook, over the operation's
  cells. The job's bytes after creation and at the end are the live matrices' memory reports, over their cells.
- **M2-D12. Per-matrix figures.** In the counting and test builds the driver prints each matrix's counters, store
  figures and (results) its operation's own counters just before the matrix is freed: an intermediate inside the
  operation, the others after the read-out. Each input's store figures are also printed after its build.
- **M2-D13. The gate** (test build) audits each input after its build and every matrix before it is freed; dumps every
  table (or list) and handle fields, and in Hash-V-pack every build table (h) at its packing; and writes every
  intermediate's cells, which the runner checks against fractions (v0.2 §15.7 item 1).
- **M2-D14. The TC gate** for every set is a new `tc3gate_m2.c` (m1's `tc3gate.c` unchanged): tc3.c itself enters every
  input in order and, when a step multiplies, 0/1 once (M1-D11); then runs every step in order on tc3.c handles.

## The harness (test)

- **M2-H1. Pins.** `m2_common.RULE_SHA256` is the sha256 of `research/rule-m2.md` as approved and committed at `38f2d5d`
  (`e340d32b…e46200`); a measured run is refused if the file differs or holds the word "PROPOSED". The 08 module,
  tc3.c and its headers and tcm.c's recorded diff are pinned as in m1; m1's sources are pinned at `6e9a85e`.
- **M2-H2. No option runs the four runs in one command.** **Ambiguous.** The rule says the runs go back to back without
  waiting for the author, and that "Each run is still its own background command, with its own steps 0–7, pilot,
  hard stop and folder" and a timeout of at least 45 minutes (item 5). One command for four runs would not be its own
  background command per run (and four 45-minute timeouts exceed a single background command's limit). So the
  operator starts `--op add`, `mul`, `sop`, `pos` one after another; a run that stops does not stop the next. m1's
  check that the multiplication run waits for a finished addition run (M1-H4) is dropped, as the rule says.
- **M2-H3. Step 0 against m1.** **Ambiguous.** "Step 0's files MUST equal m1's by sha256": the workloads and references
  (`job-NN.wl`, `job-NN.ref`) of a measured add or mul run must equal the hashes m1's run recorded in
  `results-m1-<op>/state.json`; step 1 refuses otherwise. m1's third step-0 file, its prediction `job-NN.m1.json`,
  cannot be equal (m2's model differs: no pre-stored 0 and 1, three sizings), so m2 writes `job-NN.m2.json` and does
  not compare it. Toy runs are not compared (m1 recorded no toy files). m2_gen draws add and mul with m1_gen's own
  functions, so they are m1's by construction.
- **M2-H4. Each quantity its own minimum.** rule-m2 item 4: "Each quantity keeps its own minimum of reps 2 and 3, as m1
  does per part". The harness does as the first half says: the total, the operation, the read-out and each input's
  build each take their own minimum over the kept reps. (m1's harness in fact kept the parts of the rep of minimum
  total, so "as m1 does" does not describe m1; see the list at the end.) The process's CPU time, reported only, is the
  rep of minimum total's.
- **M2-H5. Rows and order.** Calibration runs the four builds in the order TC, Hash-V-fixed, Hash-V-grow-map,
  Hash-V-pack; Hash-V-fixed-null takes Hash-V-fixed's r (marked `shared_from`). Rounds rotate by one row per round over
  the five rows: 4 + 60 = 64 launches per job, 4 + 10 = 14 in the pilot.
- **M2-H6. The analysis.** Measure 1: per round, the input matrices' build times are the samples; a row's figure is the
  median of all its samples; the ratio is the reference's figure over the row's; the bootstrap draws rounds with
  replacement, every sample of a drawn round kept with it, the same draw for both rows (seed 20261029, 10,000
  resamples). Measure 1's null band is Hash-V-fixed-null against Hash-V-fixed by the same pooled bootstrap. The peak
  has a verdict in add (measure 7) and mul (measure 8) and is reported in sop and pos. In the reported pairing every
  measure is "reported" and no severe flag is raised. The strata count the run's headline measures (the peak only in
  add and mul); the grid presents the same measures, time and bytes.
- **M2-H7. Where the harness may write.** Never in `../10-interning/` or m1's two results folders; a run of one job set
  never in another m2 run's folder. Before and after each run the files of m1's folders and of every other m2 folder
  are hashed and the outcome recorded (`protected_after`).
- **M2-H8. m1's code imported, not copied.** **Ambiguous.** The rule says the m1 harness is "copied to `m2_*` files: its
  spawner, checks, calibration, bootstrap and stops unchanged". The m2 files import those parts from m1's modules
  (the spawner, `/proc` reading, disturbance and idle checks from `m1_runner`; the statistics and formatting from
  `m1_analyze`; the read-out check, limits and pilot projection from `m1_checks`; the arithmetic model `Arith` from
  `m1_model`; the matrix draw from `m1_gen`; the rule's numbers from `m1_common`), so that they are m1's text exactly
  and cannot drift. m1's files are not edited. The calibration loop, the timed block and the stops are copied into
  `m2_runner.py` (they name m2's rows).
- **M2-H9. Measure 6** is taken from the memory run: each input's memory report just after its build (cells plus table
  or list), which is the accounted figure from the counts; the gates check that the same bytes equal the model's, and
  the toy tests that the memory run's figure equals them too.
- **M2-H10. Toy sizes** for harness tests only: m1's (M1-H9). A toy run writes only under `test/build/m2toy/`.
- **M2-H11. Self-tests** (`test_m2_harness.py`; untimed, nothing measured): the pins; the forbidden places; the
  workloads of the four sets (add and mul equal to m1's); the model by hand; tcm.c unchanged and mh2.c's arithmetic
  bodies equal to mh.c's line for line; the rows from one source; no reserved slot and no handle test for 0 or 1; the
  allocation sites and the allocation checks (three builds); the C suites; every toy gate, counting and memory run of
  every set (pack's (h) equal to fixed's table; call for call; the TC gate); the exact checks catching a difference;
  the runner's untimed steps; statistics and the pooled bootstrap; the pilot; m1's process and disturbance tests
  (unchanged); the timed block with a stand-in driver; the analysis on all four sets; a toy run of each set end to end
  (`add` with `--pilot-only` then `--resume`). `--full` adds every gate and counting run at full size.

## Ambiguous (for the author)

- **M2-D1.** The hash rows are a new source, `mh2.c`, beside an unchanged `mh.c` (§15.8 says both).
- **M2-D9.** A fourth build, the memory build, beside the rule's three.
- **M2-H2.** No option runs the four runs in one command; each is its own command, started back to back.
- **M2-H3.** Only the workloads and references are compared with m1's; m2's predictions are its own.
- **M2-H4.** Each timed quantity takes its own minimum of the kept reps (m1 kept the minimum-total rep's parts).
- **M2-H8.** m1's harness code is imported unchanged rather than copied.

## Found in the spec and rule (for the author)

- v0.2 §15.8 contradicts itself on `mh.c` (M2-D1).
- rule-m2 item 2 ("three builds") against v0.2 §15.6 and the Harness section (a memory build) (M2-D9).
- rule-m2 item 4: "as m1 does per part" — m1's harness kept the parts of the rep of minimum total, not each part's own
  minimum (M2-H4).
- rule-m2, Workloads: "Step 0's files MUST equal m1's" cannot hold for m1's prediction files (M2-H3).
- v0.2 §15.2: "Every row makes the same 10 comparisons (8 in `add(s, p)`)": an addition of two cells also makes 8 (it
  has no +-1 tests); 10 is the multiplication of two cells. The rows do as tc3.c does, which the call-for-call checks
  confirm.
