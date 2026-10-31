# Builder decisions: the Nim build of round m4 (Control and Packed)

Oct 30, 2026; revised the same day after the coordinator's build review (items R-1 to R-8 below). Build and checks only: nothing timed, no wall-clock figure anywhere. Nothing committed. Built from spec
[v0.4](../v0.4.md) §17, which amends [v0.2](../v0.2.md) and [v0.1](../v0.1.md), and through them the archived
arithmetic of [`10-interning`](../../archive/10-interning/) v0.2 §10 and v0.3 §17.3. **Every choice below is pending
the author's review.** [README.md](README.md) has the build, the checks and their results.

## 1. Structures (RULES, Algorithm design 4; §17.8)

The cells are the matrix itself and are not counted (§17.8). A fixed handful of numbers is data, not a structure.

| Model | Kind | Structure | Where | Lifetime |
| --- | --- | --- | --- | --- |
| Control | stored | none beyond its cells (r c pairs of int64) | `Control.cells` | the matrix's |
| Control | temporary | none | | |
| Control | local | none | | |
| Packed | stored | **(l), the packed list**: 2rc entries of 64 bits during the build; W entries of w bits once packed | `Packed.list` | the matrix's |
| Packed | temporary | **(h), the index table**: T_fix slots of w_h,build bits, each a position in (l) or all ones | `Packed.index` | from `npk_create` to the pack (inside the last `npk_enter`, or at the end of `npk_madd`/`npk_mmul`) |
| Packed | local | **mul's row buffer**: n decoded cells of 24 bytes (`Decoded`: a, b as int64; neg, same as bool) | `products` in `packed.nim` | one `npk_mmul` call; malloc'd and accounted at its start, freed and accounted before the pack |

**Data, not structures:** each header's fixed fields (dimensions, T_fix and its log, the handle widths, w, W, the
widest value's bits, the fields written, the packed flag, the id); one operation's counts (`Tally`: case, gcds, gcd
work, divisions, multiplications, additions, comparisons, cross matches); one decoded operand (`Decoded`); the
overflow record (`Marks`: new, passed on, mark tests). In the counting builds each header also holds the driver's
counter records (`sk_counters`, `r2_xcounters`, `m2_scount`, `m2_ocount`, `m4n_marks`), as the C rows do.

**Test build only, outside the model (§17.8, "audit copies are allowed"):**
- `pack`: one copy, `before`, every cell's value, to check that no value changes in the pack;
- `npk_audit`: two copies in one function, `used` (one flag per list entry) and `sorted` (the list, sorted), to check
  that every entry is used and none is held twice;
- the check of every gcd against 08's (`gcd08`): no copy.

**The driver's (not the models'):** (in-A) to (in-D), (out), and beside (out) the count and indices of the result's
marked cells (§17.10 item 5, §17.8: "(out) also holds the count and the indices"); the per-operation record buffer.

`test/alloc.c` checks the models' blocks event by event (README, "Verification").

## 2. Decisions

**N-1. Language and toolchain.** Nim 2.2.14, built from the GitHub tag `v2.2.14` (commit
`c5c2337564e347bb8252a510c79a74817a670231`) with its pinned `csources_v3` (commit
`eeab3ac46e93f10efda8e58c4db02b9438319d71`) by the tag's own `build_all.sh`; `bin/nim` sha256
`c844c0fd004a76b1c1a6bc9b2e16a7878b375aba9dd582a29f66e667a17d0441`. Chosen as the release closest to the pilot's 2.2.13
nightly. The compiler is in the session's scratchpad, not in the repository (README, "Nim").

**N-2. Flags (the pilot's findings).** `--app:staticlib --noMain -d:danger --mm:arc -d:useMalloc --panics:on`, gcc
`-O2` through `--gcc.options.speed` (Nim adds `-fno-strict-aliasing`). No `std/rationals`, no `std/math` gcd. Every
block of a model is `malloc`'d, `realloc`'d and `free`'d directly (`core.nim`), so the driver's heap wrapper sees
exactly the model's blocks. Nothing in the models uses Nim's managed memory, except the test build's audit copies
(`seq`, through `malloc` by `-d:useMalloc`).

**N-3. Natural Nim, not C's machine-code shape.** The gcd and the arithmetic use plain `if` expressions and `min`.
The pilot's `select` (C's `?:`, for gcc's conditional moves) is not used. The Nim objects have 45 (Control) and 68
(Packed) `cmov`s against C's 86 and 90 (README, "Against C" has the Ir).

**N-4. One arithmetic for both models** (`arith.nim`; RULES, Algorithm design 3). Control and Packed call the same
`add`, `multiply` and `reduce` on `Operand` values, so their arithmetic is equal call for call by construction; the
checks confirm it record for record.

**N-5. The step order, and counts up to the stop.** Every checked step runs in the order `tc3.c` makes it, which
defines Control line for line (review R-2). That is the pseudocode's written order, left to right, numerator before
denominator, except in addition's case num with h ≠ 1, where `tc3.c` makes both divisions (t/h, d/h) before the two
multiplications; a step's operands are computed before the step is counted. An operation that overflows
keeps the counts of the steps it made, the overflowing one included (§17.10 item 6, "call for call: included, up to
the stop"), and is recorded in the per-operation record with its case.

**N-6. The mark** (§17.10). Stored as any cell: Control writes (0, 0); Packed stores the whole number 0 twice (found
the second time) and writes both fields with tag 0. Read back as (0, 0). Every operand cell is tested by d = 0 when
read, the left first, both always ("mark tests"). A marked operand stops the cell (addition) or the cell at that term
(multiplication), counted "passed on"; an overflow in the arithmetic, "new". `madd` and `mmul` return `SK_OK`.

**N-7. Reads around the mark** (§17.6, §17.10 item 2; review R-5). In Packed each operand read by handle reads its
fields and its denominator's value first, and is tested for the mark; its numerator is read from the list only once
both operands of the term or cell have passed (a cell whose two fields name one handle reads one value). So a stopped
term or cell makes no numerator read. The left operand of mul comes from the row buffer, which already holds A's row
(§17.10 item 4). The denominator read for the test is part of reading the operand and is not counted. Cross reads (one per distinct
handle of each operand, 1 or 2) are counted only for a term that goes on to the arithmetic. So on jobs with no mark,
every read counter equals C's (checked), and "a term stopped by a mark makes no arithmetic reads".

**N-8. Where the new counters live.** "New", "passed on" and "mark tests" are counted in the counting and test builds
only, in the result of the operation (`m4n_marks`, `npk_marks`/`nctl_marks`), and printed by the driver as
`mrk <X> new=.. passed=.. tests=..` for each matrix an operation built. §17.10 says the first two exist only in the
counting build; for "mark tests" it says only "in every model alike" (see §4, item 5).

**N-9. The row buffer is a heap block** (§17.8, §17.11: "the row buffer's allocation and its release", counted in the
accounted peak). It is malloc'd once per `npk_mmul`, in `products`, and freed there before the pack. So it is in both
the accounted and the heap peak. (C's is a stack array, in neither.)

**N-10. One write per cell** (§17.5). Both whole numbers are stored, then the two fields go in one store of 2 w_h,build
bits (`uint16`, `uint32` or `uint64`, little-endian, the numerator's field low). C writes the two fields one after the
other inside one width dispatch.

**N-11. The pack's hook.** In the counting and test builds, `pack` calls the driver's `m2_pack_hook` (a read) after
computing the packed widths and before freeing (h), so that the gate can dump (h) slot by slot (§17.12). The timing and
memory builds have no hook call.

**N-12. The driver** (`driver/bench_m4n.c`, `driver/api_m4n.h`): copies of `test/bench_m4.c` and `test/api_m4.h` with
the two Nim rows, and only what §17.10 and §17.11 ask of the driver: the mark scan of (out) after the read-out's clock
read, in every build (`marks <R> count=.. cells=..`); the `mrk` lines; Packed's row buffer bytes per mul step and its
K-Q3 read counter (`rowbuf <X> bytes=..`, `kq3 <X> reads=..`, counting and test builds; review R-6); and the per-operation record written as made (mul now
makes n²(2n − 1) operations per step, fewer with marks), not checked against 2n³. `api_last_error` prints
`none-recorded`.

**N-13. The test build's checks** (§17.12; 10-interning v0.2 §10.5):
- every gcd's value against 08's own `sk_gcd_raw`, called as C from 08's pinned `lib/vendor08/sk_internal.h`
  (`include/m4n_gcd08.h`, which refuses the m4 header; review R-3);
- every arithmetic result: a zero has the denominator 1 (K-14), and entry likewise;
- Control: every operand pair that is not the mark is d ≥ 1, in lowest terms, m ≠ −2^63 (v0.1 §4 as §17.10 amends
  it);
- Packed, at each pack: every position of (h) is found by its value's probe sequence, W slots are used and the rest
  hold the empty mark; and no cell's value changes in the pack;
- `npk_audit`, on a packed matrix: (h) freed; (l) at the narrowest width that holds its widest entry, its entries
  non-negative, distinct and each used by a cell; the fields at the packed handle width; every handle < W; every
  denominator's tag 0; every cell in lowest terms with zero as +0/1, or the mark +0/0. `nctl_audit`: every cell valid
  or the mark.

**N-14. Limits.** Packed's `create` refuses 2rc > 2^31 (T_fix must fit a 32-bit handle, v0.1 §5.2) and dies with a
message; Control's refuses zero dimensions only. A full (h) is detected and dies (§17.2; it cannot occur). `enter`
refuses −2^63 (`SK_EINVAL`), then d = 0 (`SK_EDIVZERO`), in `tc3.c`'s order (review R-4). A per-operation record
past the driver's buffer is an error (`sk_die`), not dropped (review R-7).

**N-15. Counters kept.** v0.1 §9's list and §17.11's: per phase, the core's gcds, gcd work, exact divisions,
multiplications and additions; the comparisons (`cmp`: 8 per addition, 10 per multiplication); the cases; Packed's
cross tests, matches and cross reads (`opc`), its stores (found, new, probes, longest), W, T, w, w_h,build,
w_h,pack, bytes and fields narrowed (`stor`, `pack`); the per-operation record; and §17.10's three. K-Q3 reads are 0
in V (every value is read for the cross tests) and are not counted separately.

**N-16. Workloads** (`test/gen.py`, §17.9). As specified, with m3's grid, sizes and seeds (`m2_common`). Pairs are
written as drawn and reduced by the entry, "as before" (m1_gen). Exact references are JSON (`job-NN.ref.json`),
because most marked cells' exact values do not fit the read-out's int64 format. CPython 3.13.17 is checked at start.
Jobs generated: 1, 4, 5, 8, 9, 12 of each op (n = 16, 32, 64 at ρ = 1 and 64).

**N-17. The TC gate** (v0.1 §4; §17.10's TC-gate row; §17.13; review R-1). `test/tc3gate_m4.c`, a copy of
`test/tc3gate_m2.c`: `tc3.c` unchanged (sha checked by `build.sh`), built with `lib/vendor08/m4` first (the new gcd),
fed §17.6's summation (s from the first product, n − 1 additions), run on every job up to its first overflow and no
further. `check.py` runs it on every job: where the simulator gives no overflow, it must complete, and Control's
per-operation record and read-out must equal its own; where it gives one, it must stop at the simulator's first
overflowing operation, and Control's records before it must equal its own. This checks the shared arithmetic module
against `tc3.c` itself.

**N-18. The gcd's pin** (§17.7, §17.14; review R-3). The gcd is in its own file, `src/gcd.nim`, pinned by
`src/GCD.SHA256SUMS`; `build.sh` checks that pin, 08's `lib/vendor08/SHA256SUMS`, the m4 gcd's
`lib/vendor08/m4/SHA256SUMS` and `tc3.c`'s sha before it builds. The test build counts the remainder-first path (6a)
in `gcd.nim` (`remainderFirst`), so that `gcdtest.nim` checks the path, not only the value.

**R. The review of Oct 30, 2026** (the coordinator's), each item done: R-1 the TC gate (N-17); R-2 `tc3.c`'s step order
in add's num with h ≠ 1, in `arith.nim` and `sim.py`, with the crafted case `ovf-num-h` reaching it with an overflow at
a (t/h) (N-5); R-3 the gcd's path counted, its own pin, 08's own C gcd as the check (N-13, N-18); R-4 the entry's
order (N-14); R-5 d tested before the numerator is read (N-7); R-6 the row buffer bytes and the K-Q3 counter printed
(N-12); R-7 the record's capacity an error (N-14); R-8 the test build's pack copies unchanged.

## 3. Not built

- **The K-n counters and their companions** (`k1_*` … `k15_*`, `kq1`, `kq2`, `reads_skipped`, `zero_results`,
  `gcd_hist`): C keeps them from the archived rounds (10-interning v0.2 §11, §13.1); the matrix spec's instrumentation
  list (v0.1 §9, §17.11) does not name them.
- **The test build's switches and corruption hooks** (`t_force_general`, `t_known_off`, `t_corrupt_*`; 10-interning
  v0.2 §14.3): v0.1 §10's test list does not name them.
- **The overflow report** (`overflow()`, `sk_overflow`, the step names, `ev_overflows`): §17.10 makes the record the
  mark plus two counters.
- **Single operations** (`tcm_add`, `tcm_sub`, `tcm_mul`, `tcm_div`) and division: out of the matrix spec's scope.

## 4. For the author

1. **The expected overflow rates are far below what these workloads give** (§17.10, "Expected"). On the pooled jobs
   at ρ = 1, n = 64: mul 91.6% of cells marked; sop's R 99.9%; pos's R 100%. At n = 32, ρ = 1: 3.1%, 14.6%, 44.6%.
   At ρ = 64: at most 0.07%. These are not artefacts of the models: Python's exact sums give values past 64 bits in
   87% (mul), 99% (sop) and 99.9% (pos) of those cells. The spec's figures were unpooled samples; its own note says
   the pools favour rarer, larger values. Measures 3–5 on these jobs would mostly time the mark's path.
2. **The step order of `add` case num with h ≠ 1.** The pseudocode writes `a · (t/h)` before `d/h`; `tc3.c` makes
   both divisions first. By the review (R-2) the build follows `tc3.c`; the pseudocode (10-interning v0.2 §10.1) still
   reads the other way, so the spec text and `tc3.c` disagree there.
3. **§17.9 "reduced to lowest terms, as before":** read as m1's (pairs written as drawn, reduced by the entry). The
   other reading, writing reduced pairs, changes the entry's counts but no value.
4. **The read of d for the mark test** is part of the operand read and uncounted (N-7). Confirm.
5. **The builds of the "mark tests" counter** (N-8): counting and test builds, like every counter. Confirm.
6. **The driver copy** (N-12) and its mark scan, and that the per-operation record is now as long as the operations
   made. The m2/m3 harness's `nops()` (n³ additions per mul step) no longer holds under §17.6.
7. **The row buffer on the heap** (N-9): confirm that §17.11's "allocation and release" means a heap block, so that it
   is in the heap peak too.
8. **What is not built** (§3): confirm that the K counters, the test switches and the overflow report are out.
10. **The per-operation record's int16 range**: C stops on a count over 32,767; the Nim build does not check it (no
    operation of 64-bit operands comes near it). Not changed, as the review did not ask.
9. **Rectangular matrices.** The loops are written for r × c (mul for r × k by k × c); the spec's round is square only
   (v0.1 §2). The driver only makes square ones.
