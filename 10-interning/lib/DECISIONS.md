# Library decisions (layer 1: L1 and L1-opt)

Choices made where spec v0.1 is silent or unclear, each the plainest correct option found (spec §12, L-19). Each is
**pending the author's review**. None changes a structure, a case order or a value the spec fixes. Items marked
**ambiguous** are also listed at the end.

No plain inefficiency was found to fix, so the change log is empty and no `CHANGELOG.md` exists.

## Source and copies

- **D-1. The 08 copies** (spec §4.3 item 4, §9). `vendor08/` holds `sk_internal.h`, `common.c`, `ctl.c`, `sk_common.h`
  and `sk_ctl.h`, copied unchanged from 08-skeletons (now shared/partial-sharing-baseline) at commit `08bbfbe` (its HEAD on Oct 29, 2026). 08's working tree
  had uncommitted changes in progress to `sk_common.h` (new Design 2 counter fields), so the committed file was taken,
  not the working copy (**ambiguous**). The other four files are identical in both. `vendor08/SHA256SUMS` and
  `test/libconf.py` record the hashes; `make hashes` and the runner check them before every run.
- **D-2. One source, two rows** (spec §17 item 2). `src/l1.c` compiled without `L1_OPT` gives `l1_*` (L1); with
  `-DL1_OPT` it gives `l1o_*` (L1-opt). Every trick is inside `#ifdef L1_OPT`. The helpers `gcdk`, `dvx` and `mulx`
  reduce in L1 to `sk_gcd`, `sk_divexact` and `sk_mul`, called in the order of the pseudocode. `l1_case_name` is
  `static inline` in `l1.h`, so either row links alone.

## L1

- **D-3. Counters beyond 08's** (spec §13.1, L-16). 08's `sk_count` cannot grow without changing 08, so layer 1 keeps
  a parallel `l1_xcount` per phase and part, read with the same call (`l1_counters(c, base, x)`). Where each counter
  goes:
  - in the core: cases, index comparisons, `gen_g1`, and O-3, O-4 and O-5;
  - in the store: intern found and appended, zero results, and O-1, O-2, O-6 and O-7.

  An operation that fails before its comparisons counts none. Division by zero is one such failure.
- **D-4. Overflow step names** (spec §4.4 item 2). The operation, the case and the pseudocode's expression, for example
  `add num: a*(t/h)` or `divide general: (b/g_2)*(c/g_1)`. The unit tests reach every named step of every case that
  has a checked operation.
- **D-5. Division's order of tests.** An invalid handle gives `SK_EINVAL` first. Then the divisor's whole number is
  read and tested for zero (spec L-12), and only then are the four comparisons made.
- **D-6. Case num with g = 1.** As the pseudocode writes, h is set to 1 and `t / h` and `d / h` are still computed as
  exact divisions. This also happens in case den with h = 1. Removing a division by a known 1 is trick O-4 (spec
  §17.1), so L1 keeps these two divisions, and no inefficiency was "fixed" here (**ambiguous**).
- **D-7. Audits** (spec §14.2):
  - The incremental audit checks the rationals and whole numbers since the last audit. Each new whole number must be
    used by a rational stored since then, since a whole number is appended only by a store whose rational follows at
    once.
  - The full audit checks every invariant.
  - Both point the counters at a sink, so an audit changes no count.
- **D-8. Test-build calls** (spec §14.3): `t_whole`, `t_rat` (introspection), `t_force_general` (the
  shortcuts-change-nothing switch), `t_corrupt_rat`, `t_corrupt_whole` (audit tests), and `l1o_t_tricks` (spec §17.3
  item 4). In other builds they return `SK_ENOTBUILT`.
- **D-9. Memory report.** The items are "whole numbers" (8W), "lookup" (slot size × S, every slot) and "rationals"
  (8N), with allocated bytes at capacity. The class header and temporaries are not counted (08 DEC-27).

## L1-opt (spec §17; all seven tricks approved by the author, Oct 29, 2026)

- **D-10. O-4's "known 1"** (**ambiguous**). A value is a known 1 when its magnitude is known to be 1 without a
  comparison of values:
  - a whole number at the index of 1;
  - a gcd that came out as 1, including one skipped by O-3, and case num's h = 1 when g = 1;
  - a quotient by a known 1, which takes the dividend's status;
  - a product with a known 1, which takes the other factor's status.

  A signed numerator takes its whole number's status. A sum is never a known 1. A division by a known 1, and a
  multiplication by one, are skipped and only a sign is applied. O-4 also applies in `enter`, where gcd(m, d) = 1
  skips both divisions. The model in `test/l1model.py` predicts exactly this, and the prediction matched on every
  job. *Now stated in spec §17.1, O-4 (Oct 29, 2026, after the independent review).*
- **D-11. O-2 in every store, entry included** (**ambiguous**: the spec says "a result"). `idx0` and `idx1` are set
  when `intern` appends 0 or 1. A lookup skipped by O-1 is counted under O-1 only (spec). With O-2 switched off in the
  test build, the class still remembers `idx1`, and O-3 and O-4 still use it, so the switch removes only O-2's
  skipped lookups. *Spec §17.1, O-2, now says "a stored value, entry included" (Oct 29, 2026).*
- **D-12. Which gcds O-3 skips.** These are the gcds with an argument that is an operand's whole number:
  - gcd(b, d) in add's cases num and general;
  - gcd(t, d) in case den, through d;
  - gcd(c, b) and gcd(a, d) in multiply's cross cases;
  - gcd(m_x, d) and gcd(m_y, b) in multiply's general case;
  - gcd(d, b), gcd(a, c) and gcd(b, d) in divide.

  Entry's gcd and the refined method's second gcd, gcd(t, g), never have such an argument. Each skip counts the gcd
  work the call would have had, in bits of the larger magnitude, as 08 counts it.
- **D-13. O-5's counters** (**ambiguous**). Spec §17.2 lists "additions and gcds skipped, with their gcd work".
  §17.3 item 3, however, needs L1-opt's divisions to equal L1's less the counted skips, and O-5 also removes case
  den's two divisions t/h and d/h. So O-5 also counts `o5_div_skipped` (2 per operation). O-5 is tested first in case
  den, before O-3, so x − x over d = 1 counts under O-5 only. Its result 0/1 is stored through `store`, where O-2
  applies. *Spec §17.2 now lists O-5's divisions (Oct 29, 2026).*
- **D-14. O-6 switched off** (test build). The slots still hold fingerprints, but the probe always reads the whole
  number. The probes are then exactly the 08 lookup's. The audit also checks every slot's fingerprint.
- **D-15. O-7's counters.** `l1o_reserve` acts only before the first store. It sizes the rational list to exactly n.
  The counters record each growth L1's doubling would have made, the first allocation of 16 included, with the bytes
  it would have copied (8 × the rationals stored at that moment). With O-7 switched off, `reserve` does nothing.
- **D-16. Trick switches** are a run-time mask in the test build only. In the timing and counting builds every trick
  is on with no test (spec §17 item 4).

## The Q32 rows (spec §18; added Oct 29, 2026, after the Q64 run of `b3b88b6`, an exception the author approved)

- **D-17. One source, five builds.** `src/l1.c` gives L1 (`l1_`), L1-opt (`-DL1_OPT`, `l1o_`), L1-Q32 (`-DL1_Q32`,
  `l1q_`), L1-opt-Q32 (`-DL1_OPT -DL1_Q32`, `l1oq_`), and the lookup gate's build (`-DL1_LKGATE`, `l1g_`; tests and
  gates only). `L1_Q32` changes only the field type (`l1_field`: `uint16_t`, tag `0x8000`, index mask `0x7FFF`), the
  capacity test (`nW >= 32768` aborts with "capacity of 32,768 whole numbers exceeded"), the lookup's slot type and the
  test-build read-back check. Everything else is the Q64 text.
- **D-18. The Q64 object code is unchanged.** Each Q64 object (L1 and L1-opt; timing, counting and test builds),
  compiled from the new `l1.c`, is identical in every section (`objdump -s`) to the one compiled from `b3b88b6`'s,
  and so are the Q64 bench binaries of the harness (D-24). Checked again after D-25 by building `b3b88b6`'s tree and
  the current tree at the same path and comparing whole files: every timing, counting and test object (`l1.o`,
  `l1o.o`, `common.o`, `ctl.o`) and all 12 Q64 and library bench binaries (L1, L1-opt and ctl in three builds; fmpq,
  mpq, fz) are byte-identical. The sanitizer build's `common.o` and `ctl.o` are identical too; its `l1.o` and `l1o.o`
  differ only in source-location metadata (line numbers in debug information and in UBSan's records, shifted by the
  Q32 lines added to `l1.c`): that build is a test build, never timed. L1-Q64 still calls `sk_lookup` in `vendor08/common.c`;
  L1-opt-Q64 still uses its O-6 lookup in `l1.c` (it never used `sk_lookup`, spec §17.1 O-6), whose only source change
  is a macro (`SLOT_EMPTY`, defined as `NOIDX` at Q64) that compiles to the same code (**ambiguous**: the direction
  said both Q64 rows "keep using the unchanged 08 lookup code"; only L1-Q64 ever did).
- **D-19. The parameterized lookup (PLK).** For L1-Q32 and the gate's build, `src/plk.c` (D-25) repeats 08's `start_slot`,
  `sk_lookup_init`, `sk_lookup_find`, `place`, `sk_lookup_insert` and the two audit helpers line for line, with the
  slot type `pslot` (`uint16_t`, empty `0xFFFF`; or `uint32_t`, empty `0xFFFFFFFF`, in the gate's build) in place of
  `uint32_t` and `SK_NONE`. Counting is the same: one lookup search per hash, one probe per slot read.
- **D-20. L1-opt-Q32's slot** is `{uint16_t idx; uint32_t fp;}` with `__attribute__((packed))`: 6 bytes, the 32-bit
  fingerprint of L1-opt-Q64 kept, so the fingerprint test rejects exactly the same probes (spec §18.1 item 5, Q-4).
  The fingerprint is read unaligned; gcc, the one compiler of rule item 2, emits a plain 4-byte load on x86-64.
- **D-21. "No index" and the empty marks.** The code's "no index" stays `NOIDX` = `SK_NONE` = $2^{32} - 1$ in 32-bit
  variables, never stored. The Q32 empty slot is `0xFFFF`, above every 15-bit index; an L1-opt-Q32 slot is empty by
  its index field alone. So all 32,768 indices are usable, and a numerator field of `0xFFFF` (tag −, index 32,767)
  is a valid rational (unit test `q32`).
- **D-22. The per-write check** (spec §18.1 item 3) is `Q32_READBACK`, compiled only with `L1_Q32` and `SK_AUDIT`:
  after each write of an index into a 16-bit rational field or slot, the field is read back and compared. The timing
  and counting builds of the Q32 rows have no such branch; the capacity test in `intern` (one per new whole number)
  is in every build.
- **D-23. O-7's "bytes avoided"** counts rational-list bytes (`sizeof(rat)` × the rationals at each growth avoided),
  so it is half L1-opt-Q64's at 4 bytes per rational. It is the one counter that differs between the widths; the
  tests and the harness check it as exactly half (spec §18.3 item 1).
- **D-24. Tests at both widths.** `tests/unit.c` and `tests/diff.c` are compiled twice: as written (Q64) and with
  `-DT_Q32`, under which `t_common.h` points `ROW_L1`, `ROW_L1O` and every direct `l1_`/`l1o_` call at the Q32 rows
  (macros defined after the row tables, which hold every row of both widths). The row table now carries each row's
  layer-1 calls as function pointers, and its entry and slot sizes. `tests/q32.c` checks the Q32 rows against the Q64
  rows on 200 random workloads (values, whole-number lists, fields, every counter, memory reports), the gate's build
  against L1 on the same workloads, and capacity: each Q32 row filled to exactly 32,768 whole numbers and audited,
  then the next new whole number, by entry and by an operation, in a forked child, which must abort (`SIGABRT`) with
  the capacity message while the parent's class is untouched.
- **D-25. The Q32 lookup is its own translation unit** (after the review of the Q32 change). The PLK was first written
  as static functions in `l1.c`, which gcc inlined into L1-Q32, while L1-Q64 calls `sk_lookup_find` and
  `sk_lookup_insert` in `vendor08/common.c` across a translation unit: the L1 pair would then have differed in
  inlining as well as in slot width. It is now `src/plk.c` (with `src/plk.h`), compiled once per use (`-DL1_Q32`:
  `plkq.o`, symbols `l1q_plk_*`, 16-bit slots; `-DL1_LKGATE`: `plkg.o`, `l1g_plk_*`, 32-bit slots) and called from
  `l1.c` as external functions at the same three places where L1-Q64 calls `sk_lookup_*` (init, find, insert; the
  audits likewise), with no link-time optimization (rule item 2's flags). `nm` shows `l1q_plk_init`, `_find` and
  `_insert` undefined in `l1q.o`, as `sk_lookup_init`, `_find` and `_insert` are in `l1.o`. The lookup gate's build
  now calls the 32-bit PLK in its own unit too, so the gate compares two separately compiled lookups.
  **The L1-opt pair is unaffected:** both L1-opt rows keep the O-6 lookup inside `l1.c` (static, as at `b3b88b6`);
  `l1oq.o` is byte-identical, in the timing, counting and test builds, to its object before this change, and `l1o.o`
  to `b3b88b6`'s (D-18).

## Ambiguous or not as written (for the author)

1. 08's `sk_common.h` was copied at commit `08bbfbe`, not from the working tree, which has uncommitted Design 2 fields (D-1).
2. Case num with g = 1, and case den with h = 1, divide by 1 in L1 as the pseudocode writes (D-6).
3. The scope of O-4's "known 1", and O-4 in entry (D-10).
4. O-2 in entry, and its switch (D-11).
5. O-5's divisions skipped, needed by §17.3 item 3 but not listed in §17.2 (D-13).
6. L1-opt-Q64 never used 08's `sk_lookup` (O-6 has its own slots); it keeps its own lookup, unchanged in object code (D-18).
7. L1-opt-Q32's slot packed to 6 bytes rather than padded to 8 (D-20; spec Q-4).

# Round 2 (spec v0.2: Q32 and TC), Oct 29, 2026

Builder's choices where spec v0.2 is silent or unclear, each the plainest correct option found, **pending the author's
review**. Instruction followed: textbook baselines, correct programming, not clever engineering. No retired file was
changed; `lib/Makefile` only gained round-2 targets at its end (`r2`, `r2-test`, `r2-sanitize`, `r2-check`). No plain
inefficiency or bug in the reused code was found to fix.

- **R2-D1. Sources.** `include/r2.h` (cases, counters, switches, profile report), `include/q32.h`, `include/tc.h`,
  `src/q32.c`, `src/tc.c`, `src/r2int.h` (counting, switch, sign and probe macros shared by both rows),
  `src/r2prof.c` (the fenced TSC read and the probe cost). Q32 uses `src/plk.c` built with `-DL1_Q32`, unchanged, as
  its own object; its timing-build object is byte-identical to v0.1's `plkq.o`. `q32.c` defines `L1_Q32` itself
  before including `plk.h`.
- **R2-D2. The ±1 test** is `same(a, b)` and `same(c, d)`: an index comparison in Q32, a magnitude comparison in TC
  (in lowest terms a = b only for ±1/1). All six comparisons are made before a case is chosen in multiplication and
  division (v0.1 L-13 carried). Q32 reads the other operand's numerator only when the ±1 is −1, to keep a zero's tag
  + (spec §10.2 note 1); TC applies the sign by negation.
- **R2-D3. Reads (K-Q3).** Q32 reads the whole numbers a case uses through one helper that skips an index already
  read in the operation (including division's c). `reads_skipped` = 4 − reads made, per operation (entry excluded).
- **R2-D4. Store hints.** Q32's store takes the tag, the magnitudes (−1 when not read, only with a hint) and the hints
  i, j (`SAME_I` for K-Q2). The test build checks every hint against the list and K-14's invariant, and aborts on a
  mismatch.
- **R2-D5. Counter cells.** As round 1: core and store per phase. K-14, K-Q1 and K-Q2 count in the store part;
  every other K counter, the cases and the comparisons in the core.
- **R2-D6. What a K counter's skipped arithmetic is** (spec §13.1 says only "the arithmetic it skipped"): the
  difference from the same operation with every known-answer skip off. So K-4 and K-6 count 4 divisions and the gcd
  gcd(t, 1) (work bits(max(|t|, 1))); K-2 counts case cross's 3 multiplications and 1 addition for ±1/1 ∓ 1/1, and
  otherwise case den's addition, gcd(0, d) and 2 divisions (the counting build reads d for the work; not a K-Q3 read);
  K-9 and K-11 count 2 divisions per unit gcd and one operation; K-12 and K-15 count operations only (their cases do
  no arithmetic); K-13 counts every sign the pseudocode applies (as `sgn`), and its operations are those of every
  case but `zero`; K-14 counts stores. The unit test checks this call for call: with every skip off but K-13, each
  call's arithmetic exceeds the baseline's by exactly the K counters, and Q32's lookups by K-Q1 + K-Q2; with K-13
  off alone, its multiplications by K-13's count (**ambiguous**: an interpretation). **Accepted by the build review
  (Oct 29, 2026).**
- **R2-D7. The test-build switches.** `t_force_general` sends every operation to its general case (all six
  comparisons false). `t_known_off(mask)` turns off K-n by bit n−1 and the K-Q1/K-Q2 hints by `R2_KOFF_HINTS`: off,
  K-1 and K-3 to K-11 divide by a gcd of 1 (K-4 and K-6 take the refined path), K-2 lets x − x fall to its next
  case, K-13 multiplies by ±1 with a counted multiplication, K-14 restores v0.1's zero normalization, and the hints
  are dropped (every whole number looked up). K-12 and K-15 are cases, sent to general by the shortcuts switch only.
- **R2-D8. Overflow step names** follow the v0.2 pseudocode ("add cross: s_x*a*d", "multiply general: a'*c'", ...).
  "add num: t = s_x*d' + s_y*b'" is checked but cannot overflow (g ≥ 2 halves both terms); "add num: a*t" names both
  the K-4 and the K-5 branch. **The build review accepted keeping the unreachable `add num` check (Oct 29, 2026).**
- **R2-D9. Division by zero** is tested after reading c, before the comparisons; that read is counted in K-Q3 even
  though the call then fails (nothing is stored).
- **R2-D10. Tests.** `tests/r2_unit.c` (worked example, every case with both switches, signs, zero, ±1, overlaps,
  entry, every reachable overflow step with operands found by a search over large values, the store, each K-skip
  with h = 1 and h ≠ 1, zero results, audits, the switches call for call, TC against Q32, capacity by forked
  children that must abort with the message), `tests/r2_diff.c` (random workloads against a 128-bit reference, both
  rows' every per-call counter equal, divisions by zero and overflows alike; every case and K-skip must be met).
- **R2-D11. The profiling build.** Probes guarded by the class's sampling flag; chained fenced reads; F, C, A.gcd
  (each gcd bracketed alone), A.rest, I, S; M bracketed on every growth (W list, lookup when 2(k+1) > S, rational or
  pair list) and taken out of the sampled call's phase. Entry starts in A.rest (no operand fetch); argument checks
  before the first probe fall in U. Each probe interval is charged to the phase it closes, and a read's cost is
  subtracted once per interval; U's correction subtracts two reads per sampled call (the outer pair).
  `prof_sample` and `prof_outer` are added to the interface for the driver (spec §6 item 1 names only `profile`).

## Round 3: Q32-index and Q32-hash (spec v0.3, approved Oct 29, 2026)

New files beside round 2's, which are unchanged: `include/r3.h`, `include/q32i.h`, `include/q32h.h`, `src/q32i.c`,
`src/q32h.c`, `tests/r3t.h`, `tests/r3_unit.c`, `tests/r3_diff.c`, `tests/r3_alloc.c`, and the `r3` targets of the
Makefile. `tc.c`, `tc.h`, `r2.h`, `r2int.h`, `r2prof.c` and the 08 module are used unchanged; `q32.c`, `q32.h`,
`plk.c` and `plk.h` are retired and not built. Each choice is **pending the author's review**. No point needed a
structure beyond those of the spec, so the builder instruction's stop was not reached.

- **R3-D1. Derivation.** `q32i.c` and `q32h.c` are round 2's `q32.c` with the prefix renamed, and only the
  whole-number storage replaced: `intern`, `create`/`destroy`, the memory report, the audits, the storage counters and
  the test-build introspection. Every other function is the same text (checked by `test_r3_harness`), except two
  lines of `store_`'s test-build hint check in Q32-hash, which test "a slot holding the value" in place of "an index
  below W". Q32-hash keeps round 2's member name `W` for its table, so that every read of a whole number (`c->W[k]`,
  K-Q3) is the same line; `k` is a slot.
- **R3-D2. Q32-index's search** is `for (k = 0; k < W; k++) if (list[k] == v) break;`: top to bottom, stopping at the
  first match, with no sentinel or ordering. A new value is appended after the capacity check, by round 2's
  `SK_RESERVE` growth (doubling from 16, R3-3).
- **R3-D3. Q32-hash's store** is the pseudocode of §8 item 3: the home slot `(uint64_t)v * phi >> 49`, at most 32,768
  probes, `s = (s + 1) % 32768`. The table is allocated with `sk_xmalloc` and emptied by a loop setting each slot to
  −1 in `create`, and never reallocated. A full table aborts with "Q32-hash: hash table overflow: all 32,768 slots are
  used" in every build. `intern` has no `PF_GROW`, since nothing grows there; M covers the rational list only.
- **R3-D4. Storage counters** (§11.1) are a new struct `r3_scounters` in `include/r3.h`, one cell per phase and part,
  read by `q32i_storage` and `q32h_storage` (counting builds). `r2.h` is not changed, so TC is compiled exactly as in
  round 2. The cell is `[ctx.op][SK_STORE]`, since `intern` is called by the store alone. The counts are added after
  the loop (Q32-index: k + 1 when found at k, W when new; Q32-hash: the probe number at exit), so no counter sits in
  the search loop. The found/new split is round 2's `intern_found` / `intern_appended`. The longest store is a
  maximum per cell. Round 2's `lookup_search` and `lookup_probe` stay in 08's struct and are always 0.
- **R3-D5. Audits** (§12.2). Q32-index's incremental audit searches for each new entry, which must be found at its own
  index; its full audit sorts a temporary copy (R3-5). Q32-hash's audits check each slot named by a rational (all of
  them in the full audit, and those since the last audit in the incremental one) by the probe sequence from `h(v)`.
  The full audit also checks that every used slot is used by a rational, that the empty slots hold −1, and that the
  used slots number W. It uses a temporary 32,768-byte flag array, freed before it returns.
- **R3-D6. Test-build introspection.** `q32h_t_whole(k)` takes a slot and returns `SK_EINVAL` for an empty one.
  `q32h_t_corrupt_whole` can write any slot, an empty one included.
- **R3-D7. Tests.**
  - `tests/r3_unit.c` holds round 2's unit tests for all three rows, each Q32 test run for each Q32 row. The worked
    example checks indices in Q32-index and values by slot in Q32-hash. It adds both rows' capacity tests (in Q32-hash
    the full table: every slot used, the next new value aborting by entry and by an operation, a known value still
    found) and Q32-index's search (a value at index 0, in the middle and at the end, and a new value; the entries
    examined).
  - Its probing test covers three values sharing a home slot, found by search with the hash written again in
    `r3t.h`. It also covers a wrap from slot 32,767 to slot 0, the probes, and the longest store.
  - The switches test checks the three rows against each other: equal values, equal per-call arithmetic, and equal
    per-call stores (found, new, reads) in the two Q32 rows, which hold the same set of whole numbers.
  - `tests/r3_diff.c` runs round 2's differential test with three rows. It compares each Q32 row's per-call counters
    with TC's, and the two Q32 rows' per-call stores, reads and K-Q skips with each other. At the end it checks the
    same set of whole numbers.
- **R3-D8. The allocation check** (§6 item 5) is `tests/r3_alloc.c`, linked with
  `-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free`. It runs in the timing, counting and profiling builds.
  - After `create`: the class header, plus Q32-hash's table of exactly 262,144 bytes, plus the profiling build's
    state.
  - After every call: at most TC 2, Q32-index 3 and Q32-hash 3 live blocks (one more in the profiling build).
  - At the end, the live blocks are exactly the memory report's structures at their allocated sizes. The table is
    never reallocated, and `destroy` frees everything.
