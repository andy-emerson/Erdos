# Builder decisions (11-matrix, round m4: Hash-V-pack-4)

Oct 30, 2026. Build only: no timed run, no wall-clock figure anywhere below. Instruction counts are callgrind Ir, one
run at a time; bytes are the harness's accounted totals (memory build). Nothing is committed. Every choice is
**pending the owner's review**.

## Scope

This file records the final model, as the owner decided it after the earlier comparison:

- **Pack:** the compact index table (variant G) is the model. The pack-map path and variant B are removed. There are no
  build switches.
- **Operand reads:** the per-width read loops are dropped. Only mul's row buffer is kept, freed before the pack.
- **List width:** the list is built at 64 bits, and the widest value is tracked. At the pack, the list is narrowed in
  place to the width of that value and shrunk. The same rule holds for every matrix, inputs included.
- **Kept:**
  - the one-step fill of the table;
  - the cell's two fields written together;
  - the new gcd, for TC and mh4;
  - the bound fix.

  Per-width pack loops were dropped (see "Per-width pack loops").
- **Tests and label:** the existing C suites' mh2-layout checks are rewritten as value checks for mh4, and the row has
  its own label, `Hash-V-pack-4`.

These instructions, and the earlier updates (the pack loops, the bound fix, the G/B comparison), reached the builder
through the coordinator. **The owner should confirm they are theirs.**

## Files (all new; nothing tracked was modified)

| File | What |
| --- | --- |
| `lib/src/mh4.c` | Hash-V-pack-4. Prefix `mhv4_`; one model, no switches. |
| `lib/include/mh4.h` | Its declarations (mh2.h's `MH2_DECLARE`, mh2.h unchanged), `MH4F`, and `MH4_NAME` = "Hash-V-pack-4". |
| `lib/vendor08/m4/sk_internal.h` | 08's `sk_internal.h` with `sk_gcd_raw` changed and nothing else. |
| `lib/vendor08/m4/sk_internal.diff` | Its diff from 08's file. |
| `lib/vendor08/m4/SHA256SUMS` | The m4 pin, with a note (see "gcd"). |
| `test/api_m4.h` | `api_m3.h` plus one row, `IMPL_HASH4` (Hash-V-pack-4, label from `MH4_NAME`). |
| `test/bench_m4.c` | `bench_m3.c` with `api_m4.h` in place of `api_m3.h`; nothing else differs. The binary prints `row=Hash-V-pack-4`. |
| `lib/tests/m4t.h`, `m4_unit.c`, `m4_diff.c`, `m4_alloc.c` | m3's suites, rebased on the rows TC, Hash-V-pack-4 and Hash-V-pack (mh2.c, the reference row), with mh2's layout checks replaced by value checks for Hash-V-pack-4 (see "Tests"). |
| `test/m4_libconf.py` | Builds both sides into `test/build/m4/` and runs every C suite (`--test`). |
| `test/m4_check.py` | The cross-check on all 45 m3 workloads. |
| `test/DECISIONS-m4.md` | This file. |

- `mh2.c`, `tcm.c`, `tc3.c`, 08's files and their pins, and m1's, m2's and m3's sources, harness, C suites and results
  are unchanged. Every existing pin still holds.
- No `m4common.c`: mh4 links `m2common.c` (the hooks and the accounting) unchanged.
- The m3 harness files (`bench_m3.c`, `api_m3.h`, m3's C suites) are copied, not edited, so m3's results stay fresh
  under m3's staleness hash.

## The model (lib/src/mh4.c)

**1. Build: the compact index table (variant G).**
- **(h), the build table:** T slots holding list positions, at the build handle width. That width is named by T: the
  smallest of 8, 16, 32 with w − 1 ≥ log2 T. A position is therefore < T and never equals the empty mark, all ones,
  even at 100% fill.
- **Fill:** (h) is filled in one step with `memset(0xFF)`.
- **(l), the list:** allocated at create for the worst case, 2rc entries of 64 bits. A new value is appended at W, and
  its position is written into the empty slot.
- **Probe:** it compares l[h[s]] with the value.
- **Handles:** the handle is the list position from the start, so (l) is in the order of arrival.
- **Choice:** the probe reads (h) through the width switch per slot read, as mh2's probe did. No per-width probe loop,
  in keeping with dropping the per-width read loops. Across the 45 workloads a store reads 1.00–1.65 slots on
  average, per matrix.

**2. List width.**
- The widest value stored is kept as a header field, `wneed`: its bits, the sign bit included, computed as mh2's
  widening computed them.
- At the pack, (l) is narrowed in place to w, the smallest of 8, 16, 32, 64 that holds `wneed`, in one forward pass.
  Entry k's new place never lies past its old one. (l) is then `realloc`'d to W entries.
- **The same rule for inputs and results:**
  - results end at mh2's width (mh2 started results at 8 bits and widened to the same rule);
  - inputs, which mh2 kept at 64 bits, now narrow too (16 bits for all 132 inputs of the sweep).
- There is no widening and no widened copy (w). `widenings` and `copied` stay 0.
- The audit checks that w is the narrowest width holding the widest entry.

**3. Pack**, once the build ends (inside the last enter; at the end of madd or mmul):
1. the hook (the gate reads (h) here, through `t_whole`, which reports slot s as l[h[s]] or EMPTY);
2. (h) freed;
3. (l) narrowed and shrunk;
4. the fields narrowed in place to the packed handle width, if it is smaller, and the cells shrunk.

There is no map, no scan of (h) and no rewrite of a handle. The `m2_pack` record's `bytes_map` is 0, and `rewritten`
counts the fields narrowed.

**4. Bound (bug fix):** 2rc, not mh2's 2rc + 2, so T_fix = 2^⌈log2 2rc⌉ (= 2rc on the sweep; fill up to 100%).
- **The probe always ends.** A build stores at most 2rc distinct whole numbers in T_fix ≥ 2rc slots:
  - a new value is probed only while W < T_fix, so an empty slot exists and the wrapping probe reaches it within T_fix
    steps;
  - a value already held is found on its own probe sequence.

  `full()` stays as the guard.

**5. Operand reads.**
- mmul decodes A's row i once per i into the scratch row `ra`. B's cells, and madd's operands, are read by handle with
  the width switch per read, as mh2 reads them.
- **The scratch row:** n entries of `opv {sk_int a, b; bool neg, same}`, 24 B each.
  - n = 64: 1,536 B. At most 384 KiB, since create bounds n² ≤ 2²⁸.
  - It is a VLA on the stack in `mmul_cells`, which is `noinline` and returns before `pack(c)`. So the row is gone
    before the pack, and it is in neither the accounted nor the heap peak.

**6. Field writes:** both whole numbers of a cell are stored first, then its two fields are written together through
one width dispatch, and `nf` advances by 2. Nothing reads the cells between the two stores, since there is no growth.

**7. gcd:** the shared module's, from `lib/vendor08/m4/sk_internal.h` (see "gcd").

**Arithmetic:** unchanged. `add_body` and the multiply body are mh2's line for line, and every counter of the
arithmetic is mh2's (the per-operation records are byte-equal, below).

### Per-width pack loops: dropped

Under G the pack has no scan and no rewrite. What is left are the two narrowing passes, over W entries and over 2rc
fields (the latter only when the handle width shrinks). The whole of `pack` (every call) measures:

| Job (n = 64, job 10) | add | mul | sop | pos |
| --- | --- | --- | --- | --- |
| pack's own Ir | 52,385 | 16,481 | 33,265 | 105,162 |
| share of the job's Ir | 1.05% | 0.01% | 0.01% | 0.06% |

Writing those loops once per width could save only part of that, so they were not kept. The passes are plain loops with
the width switch per element.

## Structures

Under the owner's rule (one stored structure per object, one temporary per operation, one local per function):

| Where | Structure | Lifetime |
| --- | --- | --- |
| each stored matrix | (a) the cells: 2rc handle fields | the matrix's |
| each stored matrix | (l) the list | the matrix's: built at 2rc × 64 bits, then W entries of its width |
| each build (an input's enters; madd; mmul) | (h) the index table: T slots of positions | create → the pack |
| `mmul_cells` | `ra`, the scratch row (stack) | one mmul call, gone before the pack |
| audit build only | `before` in `pack` (each field's value); `used` and a sorted copy of the list in `audit_run` | inside the call |

**For the owner:**
1. **(a) and (l).** The table lists two per stored matrix, (a) and (l). The rule's "here the packed list" was read as
   counting the cells array as the matrix's own body, as every hash row since m1 has had it. If (a) counts too, every
   hash row exceeds the rule.
2. **The audit build.** `audit_run` holds two audit copies (`used` and the sorted list), as mh2's audit did. It is
   test-build only, but it is two locals in one function.

There is no map (p), no widened table (w), and no other block. `m4_alloc` checks exactly this, block for block, in the
timing, counting and memory builds.

## gcd

- **Touched:** `sk_gcd_raw` in `lib/vendor08/m4/sk_internal.h`.
- **(a) One remainder first:** when the bit lengths differ by more than `SK_GCD_SPREAD` = 8 (the survey's figure, not
  tuned), the larger is reduced mod the smaller, then Stein's method runs.
- **(b) ctz of the raw difference:** in the loop, the shift is `ctz(u - v)`.
- **Counting:** the remainder is raw, inside the gcd, uncounted (counting rule 3). Every gcd counter depends only on the
  arguments and is unchanged.
- **The m4 pin:** `lib/vendor08/m4/SHA256SUMS` records the m4 `sk_internal.h` =
  `cdfebe87ded6dcd3f53beb7f4a29beb8c23b0cf3437f57fb5a0372e2bbd80326`, with 08's other four files at their unchanged
  shas. A note sits at the top as `#` lines, which `sha256sum -c` skips.
- **08's pin untouched:** 08's `SHA256SUMS` and `sk_internal.h` (`166eba89…`) are unchanged. test_m1_harness requires
  that file to equal the old pin exactly. `m4_libconf.py` checks 08's, m1's, m2's and the m4 pins before every build.
- **How TC gets it:** the m4 build puts `lib/vendor08/m4` first on the include path, so `tcm.c` (unchanged), `mh4.c`
  and the common sources get the m4 gcd. `vendor08/common.c` keeps 08's, via its own directory; its one gcd is on no
  path of these rows.

## Tests (lib/tests/m4_*.c)

The m4 suites are m3's suites on the rows TC, Hash-V-pack-4, and Hash-V-pack (mh2.c, the reference row). m3's factored
rows (mh3.c) are not driven: they are m3's and are tested by m3's suites, unchanged. m2's Hash-V-pack tests are already
inside m3's unit test.

Where m3 checks mh2's layout, Hash-V-pack-4 is checked against its own rules and against Hash-V-pack by value:

- **Its own rules:**
  - T_fix and the build handle width from 2n² (`row_T`, `row_whb`);
  - no widening;
  - the result width the narrowest that holds the widest value;
  - every build list at 64 bits at creation;
  - probing on its 2-slot 1×1 table;
  - the packing at every pair of build and packed widths: (8,8), (16,8), (16,16), (32,8), (32,16), (32,32). n = 129
    was added for (32,8) and (32,16), which mh4's smaller bound reaches only from there.
- **By value against Hash-V-pack** (`same_by_value`, `same_values_held`):
  - every field's value and tag;
  - the values held, as sets, in the list and in (h) at the packing;
  - W, found/new and the packed handle width;
  - a result's list width.
- **Unchanged from m3:** every case, the cost order (each row's per-operation record equal to TC's), the expressions,
  overflow, the audits and the known-answer switches.
- **`m4_alloc`:** Hash-V-pack-4's own block set. At create: header, cells, (h), and (l) at 2n² × 8. At the pack hook:
  (h), the 64-bit (l) and the build-width cells, and no map. After each build and step: exactly the memory reports, 2
  blocks per matrix. In the memory build, the accounted total equals the blocks at every moment.

## Verification

**C suites** (`python3 m4_libconf.py --test`), 21 binaries, all passing with 0 failures:
- reference side: m3_unit, m3_diff (3000 seeds), m2_unit, m2_diff (3000), the six alloc tests, and the four asan runs;
- m4 side:

  | Suite | checks | failures |
  | --- | --- | --- |
  | m4_unit | 565,152 | 0 |
  | m4_diff (3000 seeds) | 799,782 | 0 |
  | m4_alloc, timing / counting / memory | 18,074 / 18,074 / 20,838 | 0 |
  | m4_unit-asan, m4_diff-asan (600) | | 0 |

No sanitizer report anywhere.

**`test/m4_check.py`: 0 differences.**
- Inputs: all 45 m3 workloads (add 12, mul 12, sop 9, pos 12; sha256 checked against m3's gen.json).
- Pairs: Hash-V-pack (mh2) against Hash-V-pack-4, and TC with 08's gcd against TC with the m4 gcd.
- Builds: test (gate, audits on), counting and memory.
- Compared:
  - the results, and each intermediate, cell for cell, byte-equal to each other and to the exact references;
  - the per-operation records, byte-equal;
  - the gate's dumps of Hash-V-pack-4, by value (the raw dumps differ, as they must): each field's value and tag, the
    lists and each (h) at the packing as sets;
  - every printed counter and figure, line for line, except the row's label and the fields set by its sizes and layout:
    stor's T, w, whb, probes, longest, widenings, copied, bvals; pack's T, w, whb, rewritten, bytes_list, bytes_map,
    bytes_table, bytes_cells_build;
  - TC with nothing left out.

**Python harness self-tests**, rerun alone after the final build: `test_m3_harness.py` 35/35;
`test_m2_harness.py` 33/33.

## Bytes (accounted; memory and counting builds; all 45 workloads; medians per (op, n), worst job in brackets)

**Stable bytes per stored matrix:**
- A result's bytes are mh2's exactly: the same W, list width and packed handle width.
- An input's are smaller, as its list narrows from 64 bits.
- TC's are 16n².

| op, n | input: TC / mh2 / mh4 | result: TC / mh2 / mh4 |
| --- | --- | --- |
| add 16 | 4,096 / 966 / 626 | 4,096 / 1,948 / 1,948 |
| add 32 | 16,384 / 4,558 / 3,444 | 16,384 / 8,012 / 8,012 |
| add 64 | 65,536 / 21,544 / 17,674 | 65,536 / 32,148 / 32,148 |
| mul 16 | 4,096 / 968 / 626 | 4,096 / 3,516 / 3,516 |
| mul 32 | 16,384 / 4,548 / 3,441 | 16,384 / 12,880 / 12,880 |
| mul 64 | 65,536 / 21,548 / 17,675 | 65,536 / 49,896 / 49,896 |
| sop 64 | 65,536 / 24,508 / 18,415 | 65,536 / 49,856 / 49,856 |
| pos 64 | 65,536 / 21,559 / 17,678 | 65,536 / 49,876 / 49,876 |

**Peak during the operation, × TC's peak:**

| op | n = 16: mh2 / mh4 | n = 32: mh2 / mh4 | n = 64: mh2 / mh4 |
| --- | --- | --- | --- |
| add | 0.816 / 0.602 (1.213 / 0.759) | 0.848 / 0.640 (1.166 / 0.749) | 0.883 / 0.680 (1.121 / 0.739) |
| mul | 1.278 / 0.602 (1.654 / 0.759) | 1.281 / 0.640 (1.590 / 0.749) | 1.306 / 0.680 (1.544 / 0.739) |
| sop | 0.890 / 0.555 (1.177 / 0.680) | 0.915 / 0.601 (1.118 / 0.652) | 0.897 / 0.592 (1.073 / 0.636) |
| pos | 0.750 / 0.439 (1.086 / 0.588) | 0.767 / 0.474 (1.037 / 0.572) | 0.794 / 0.509 (0.996 / 0.560) |

- **mul job 10 (n = 64):** mh2 262,656 B (1.336 ×); mh4 135,110 B (0.687 ×). That is the inputs 2 × 18,403, C's cells
  16,384, (h) 16,384 and (l) at 2rc × 8 = 65,536.
- **mul job 9 (worst):** mh2 1.544 ×; mh4 0.739 ×.
- **Build peak per input:** mh4 is 1.500 × TC on every job: cells 4n² + (h) 4n² + (l) 16n², against TC's 16n². mh2 is
  2.83–2.92 ×. The worst-case (l) of 2rc × 8 B is mh4's largest build-time block.

## Instruction counts (callgrind Ir; n = 64, job 10; build = per input, from the mul set; op = madd/mmul plus the result's create)

| | build / input | add op | mul op | sop op | pos op |
| --- | --- | --- | --- | --- | --- |
| Hash-V-pack (mh2, 08 gcd) | 1,734,191 | 3,301,386 | 277,684,294 | 563,336,874 | 312,070,099 |
| Hash-V-pack-4 (mh4, m4 gcd) | 1,148,933 | 1,968,628 | 152,658,376 | 311,595,490 | 176,342,872 |
| **mh4 / mh2** | **0.663** | **0.596** | **0.550** | **0.553** | **0.565** |
| TC (08 gcd) | 679,315 | 1,040,800 | 212,656,661 | 431,038,147 | 244,794,010 |
| TC (m4 gcd) | 541,227 | 874,485 | 134,780,182 | 274,926,826 | 156,097,496 |
| TC m4 / TC 08 | 0.797 | 0.840 | 0.634 | 0.638 | 0.638 |
| mh2 / TC, both 08 gcd | 2.553 | 3.172 | 1.306 | 1.307 | 1.275 |
| **mh4 / TC, both m4 gcd** | **2.123** | **2.251** | **1.133** | **1.133** | **1.130** |

**Method:**
- `bench time <wl> <ro> 1 1` (one run; timing build, `test/build/m4/{ref,m4}`) under `valgrind --tool=callgrind`, one
  run at a time.
- Inclusive Ir of the API functions: build = enter + the inputs' share of create; op = madd/mmul + the results' share.
  Create is split evenly per call.
- Script and CSV: `/tmp/claude-0/-home-claude/6c273ef9-4bf4-5ea2-bd9e-566136af1776/scratchpad/ir/run_final.py`,
  `final.csv`. They are not in the repo.

Against the earlier comparison (Ir, n = 64):
- **add's op:** 1,968,628 here, against 1,807,857 for the earlier default and 1,713,641 for G with per-width reads.
  This is the cost of dropping the per-width read loops and reading narrowed lists through the width switch.
- **mul:** 152.66 M here, against 153.47 M (default) and 153.37 M (G). The row buffer alone recovers what the
  per-width loops cost.

## For the owner

1. **Confirm the relayed decisions** (Scope).
2. **The structure rule** (Structures): whether (a) counts as a stored structure, and the audit build's two copies in
   `audit_run`.
3. **The worst-case list.** (l) is held at 2rc × 64 bits during every build, so the build peak is 1.5 × TC, and during
   mul it is 65,536 B of the 135,110 B peak at n = 64. That is inherent to G as specified.
4. **The order of (l)** is now arrival order, not slot order (spec v0.2 §15.4 item 3). Inputs' lists are no longer 64
   bits wide. Both are spec-level changes for the owner's record.
5. **Timed-run tooling.** `m4_libconf.py` builds the binaries, `bench-MHV4-*` (`row=Hash-V-pack-4`) and the m4 side's
   `bench-TC-*`, but no m4 runner exists. The timed round needs one, or m3's runner adapted.
