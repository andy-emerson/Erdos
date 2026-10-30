# Builder decisions (11-matrix, spec v0.1, rule-m1)

Choices made where `v0.1.md` and `research/rule-m1.md` are silent, or where a plain reading needed a detail. Each
is the plainest option found and each is **pending the author's review**. None changes a threshold, band, count or
order that the rule fixes. None adds a structure: the rows hold exactly the structures of spec §3 (TC 3, Hash-V-fixed
6, Hash-M-fixed 7, Hash-V-grow 6, Hash-M-grow 7, Hash-V-grow-map 7). The items marked **ambiguous** are also listed
at the end.

Files: `lib/src/tcm.c` (TC), `lib/src/mh.c` (the five hash rows), `lib/include/{m1,mh,tcm}.h`, `lib/src/m1int.h`,
`lib/src/m1common.c`, `lib/tests/{m1t.h,m1_unit.c,m1_diff.c,m1_alloc.c}`; round 3's `tc3.c`, `r2int.h`, `r2.h`,
`r3.h`, `tc.h` and the 08 module (`lib/vendor08/`) copied unchanged. Harness: `test/bench_m1.c`, `test/api_m1.h`,
`test/tc3gate.c`, `test/m1_common.py`, `m1_gen.py`, `m1_model.py`, `m1_libconf.py`, `m1_checks.py`, `m1_runner.py`,
`m1_analyze.py`, `test_m1_harness.py`. Nothing in `../10-interning/` is changed.

## The rows (lib)

- **M1-D1. One source per kind.** TC is `tcm.c` (prefix `tcm_`). The five hash rows are one source, `mh.c`, compiled
  five ways with `-DMH_M`, `-DMH_GROW`, `-DMH_MAP` (0 or 1), giving the prefixes `mhvf_`, `mhmf_`, `mhvg_`, `mhmg_`,
  `mhvgm_`. `mh.h` refuses any other combination: the map rewrite builds only with V and grow (spec §0 item 1). The
  settings change only the cross tests (V or M), the table's sizing (fixed or grow) and a growth's rewrite (lookup or
  map); everything else is one text.
- **M1-D2. tcm.c's interface** (spec §4). The arithmetic function bodies of `tc3.c` (`addsub`, `mul`, `div`,
  `compare`, `gcdp`) are unchanged line for line. What changes: the operations take two pairs; `operand()` forms the
  magnitudes and sign from a pair; `store()` writes to the destination the caller names (a cell of C or a local pair);
  `tcm_enter` writes the next cell of A or B in the order of entry. `begin()` has no handle to check: in the test build
  it asserts (`sk_die`) that each operand pair has d ≥ 1, is in lowest terms and has m ≠ −2^63. The matrix loops
  (`tcm_madd`, `tcm_mmul`) set `ctx.x` and `ctx.y` to the operands' cell indices (`SK_NONE` for local pairs) so an
  overflow report names them. Each matrix is its own class; the operation's arithmetic is counted in C's. The recorded
  diff `lib/src/tcm.diff` is Python's `difflib.unified_diff` of `tc3.c` and `tcm.c` (3 lines of context), checked byte
  for byte before every run and copied into the run's folder.
- **M1-D3. W counts 0 and 1.** **Ambiguous.** W is the whole numbers in a table, 0 and 1 included, since they are
  stored there at creation (spec §0 item 5) and §5.6 item 6 bounds W by K = 2rc + 2, which counts them. So grow's
  trigger, W + 1 > T/2, fires from T0 = 16 at the 7th new whole number (W = 8). The model, the audits ("at most half
  full") and the reports all use this W.
- **M1-D4. A store's probes.** **Ambiguous.** 0 and 1 are found by the store's first test with no probe (counted as
  found, 0 probes). A store that triggers a growth counts both of its probe sequences: the one in the old table that
  found the empty slot, and the one in the grown table. The growth's own reinsertion probes and the lookup's rewrite
  probes are counted apart, in the growth counters (spec §9).
- **M1-D5. Widening.** A new whole number v needs bitlen(v) + 1 bits; if that exceeds C's width w, C widens straight to
  the smallest of 16, 32, 64 that holds it (spec §5.4). A store needing both grows first, at the current width, then
  widens (spec §5.6).
- **M1-D6. The slot map (r)** is an array of T entries of w_h bits, allocated in the growth, filled during the
  reinsertion (0 → 0, 1 → 1; empty slots' entries are never written or read), and freed in the growth after the hook.
- **M1-D7. tau** is held in C's header (the operation's matrix) from `tau_build` to `tau_free`, at the start and end
  of the operation. Its positions for B's empty slots, and for 0 and 1, make no find; every other whole number of B
  is one find in A's table (its probe, without a write), counted as found or none, with its probes. None is
  2^(w_h of A) − 1.
- **M1-D8. Reads.** **Ambiguous** in one point. V's cross tests read a, b, c, d before a case is chosen, each distinct
  handle within one matrix once (a with b, c with d), in every operation on two cells, including cases that then
  need no value (multiplication's `zero` and `both`, addition's `zero`). All are counted as cross reads, none as K-Q3
  reads (spec §7.1). M reads only what its case needs, each distinct handle once, as K-Q3 reads (the masks of
  `q32h.c`). Multiplication's `add(s, p)` has its values at hand and reads nothing. K-2's gcd-work counter looks at d
  without counting a read, as `q32h.c` does.
- **M1-D9. The store of a finished cell.** In multiplication, "store s as C[i][j]" stores C's two whole numbers
  (counted in C's store counters) and is not an arithmetic result: it makes no K-14 count, as TC's write of the pair
  makes none. In addition each result passes through the arithmetic's store (K-14 counted, in every row) into C.
- **M1-D10. Instrumentation globals** (`m1common.c`): the growth hook, a read called inside each growth just before the
  old table is freed; and the per-operation record buffer of the counting and test builds. Neither is a structure of
  any build, and no row reads either back.
- **M1-D11. The TC gate** (spec §4, rule-m1 item 3). `tc3gate.c` includes `tc3.c` unchanged as part of its own
  translation unit (counting build), enters every cell of A then B, enters 0/1 once as s's start in multiplication,
  and runs the job's operations in the same order as `tcm.c`. Its per-operation record (case, gcds, gcd work, exact
  divisions, multiplications, additions; int16 each) must equal `tcm.c`'s byte for byte, as must its read-out.
- **M1-D12. Audits** (test build; spec §10 item 7). Per store: slots 0 and 1 hold 0 and 1; the table's size is a power
  of two the handle field can name; a grow table is at most half full; every cell's fields name used slots, the
  denominator's tag is 0, the cell is in lowest terms with zero as +0/1; every used slot's value fits w, is found at
  its own slot by its probe sequence, is used by some cell, and W counts them. In each growth, every written field's
  value is copied before the rewrite and compared after it (the copy is an audit copy, freed in the growth). tau is
  audited by scanning A's table, independently of the probe.
- **M1-D13. Memory.** B_used's numerator is each matrix's cells and full table, plus tau's bytes in the M rows (spec
  §9). The memory run uses the timing build, as round 3's, with the growth hook reading `mallinfo2` inside each growth.
  The growth peak's accounted figure is every matrix's report at that moment (the growing matrix still holding its old
  table), plus (n), plus (r) in the map row, plus tau.

## The harness (test)

- **M1-H1. Pins.** `m1_common.RULE_SHA256` is the sha256 of `research/rule-m1.md` as approved and committed at
  `5ee4eaa` (`8e951ac6…7aeff7`); a measured run is refused if the file differs or holds the word "PROPOSED". The 08
  module's hashes are round 1's. `tc3.c`, `r2int.h`, `r2.h`, `r3.h`, `tc.h` are pinned at round 3's hashes (`80d06ee`)
  and checked both here and in `../10-interning/lib/`. The seven files of round 3's results are pinned and checked
  before and after each run.
- **M1-H2. Step 0** is run by the runner as its own process before the run's clock starts (the clock is read after
  it), and only on a fresh start. `--resume` and `--rerun` do not regenerate; step 1 checks the sha256 of every step-0
  file against the one recorded when it was written.
- **M1-H3. `--resume`** re-runs step 1 (build and pins) and step 2, the pilot only if it did not run, then steps 4-6.
  It is refused after the hard stop or the pilot's stop, once a timed round has run, and on the other operation's
  state, as round 3's.
- **M1-H4. The multiplication run's order.** **Ambiguous.** The spec says the multiplication run comes "only after the
  author has seen the addition results". The harness cannot see that; a measured `--op mul` refuses unless
  `results-m1-add/` holds a finished, measured addition run. That the author has seen it is the operator's to confirm.
- **M1-H5. The pilot's P_6 and P_13** are the wall times of the pilot blocks of jobs 6 and 13, each from just before
  its idle check to just after its disturbance check ("idle and disturbance checks included"). N_128 counts the jobs
  at n = 128 (13-16). A toy run without jobs 6 and 13 uses its longest pilot block for both.
- **M1-H6. Rows and order.** Calibration runs the six builds in the order TC, Hash-V-fixed, Hash-M-fixed, Hash-V-grow,
  Hash-M-grow, Hash-V-grow-map; Hash-V-fixed-null takes Hash-V-fixed's r (and its calibration record, marked
  `shared_from`). Rounds rotate by one row per round over the seven rows: 6 + 84 = 90 launches per job, and
  6 + 14 = 20 in the pilot.
- **M1-H7. Statistics copied, not imported.** Round 2's `boot_ratio`, verdicts, formatting and the runner-ticks summary
  are copied unchanged into `m1_analyze.py`, since importing `r2_analyze.py` would compile bytecode inside
  `../10-interning/`, where the harness may not write.
- **M1-H8. The analysis.**
  - The ratio is T(reference)/T(row) for time, row over reference for bytes. Creation and operation each have their
    own band from Hash-V-fixed against Hash-V-fixed-null on that part.
  - "(c)" marks a time verdict on a job where either row of the pairing ran 1 rep per invocation (rule-m1 item 8).
  - Map against lookup: a verdict on creation only; its operation and total time and both end memory figures are
    reported with no verdict and no severe flag; the growth peak ratio has no verdict.
  - A row over cap has the verdict "over cap" on both time measures; total and read-out stay "reported".
  - The grid is a table of ratios, n by rho, per pairing and time measure. No plot is made (the rule asks for none).
- **M1-H9. Toy sizes** for harness tests only: n in {4, 6, 8, 12} for the four size rows, s = max(2, n^2 / rho). A toy
  run writes only under `test/build/m1toy/` and is labelled "TOY RUN" in its report.
- **M1-H10. The gate's exact checks** (spec §10 item 2) compare with the model: every counter per phase; per store,
  after creation and at the end, W, T, w, w_h, fields written, found/new, probes, longest, widenings, slots copied,
  growths, reinserted, reinsertion probes, fields rewritten and rewrite probes; every growth in order; the operation's
  cross tests, matches, cross reads and tau's finds; and, from the gate's dump, each table slot by slot, each matrix's
  handle fields, and tau position by position (by sha256). Map against lookup is also checked by equal dumps at the
  gate.
- **M1-H11. The call-for-call checks** (step 5, spec §10 items 3-6) on the counting runs: every hash row's
  per-operation record equal to TC's byte for byte, with equal G; V against M and grow against fixed with the same
  arithmetic counters (all but the K-Q3 reads) and store splits (W, found, new); map against lookup with the same
  store figures but the rewrite probes, the same growths but their rewrite probes and map bytes, and the same
  arithmetic. A failure is a bug of every row, as in round 3.
- **M1-H12. Self-tests** (`test_m1_harness.py`; untimed: nothing measured or reported). They cover the pins, the
  forbidden places, the workloads, the model worked by hand, `tcm.c` against `tc3.c` function by function, the five
  rows from one source, the allocation sites and the allocation check, the C suites, every toy gate and counting run
  of both operations, the exact checks catching a difference, the runner's untimed steps, statistics, the pilot,
  round 3's process and disturbance tests (unchanged), the timed block with a stand-in driver, the analysis, and a toy
  run of each operation end to end (`--pilot-only` then `--resume` for addition). `--full` adds every gate and
  counting run at full size.

## Ambiguous (for the author)

- **M1-D3.** W counts 0 and 1, so grow's first growth comes at the 7th new whole number.
- **M1-D4.** A store that triggers a growth counts the probes of both its sequences, old table and grown table.
- **M1-D8.** V reads all four whole numbers for its cross tests in every operation on two cells, even where the case
  then needs none.
- **M1-H4.** The multiplication run requires a finished addition run on disk; the author's having seen it is not
  checked by the harness.
