# Decisions where the spec is silent

**Conflict, first.** Spec §11, which every builder must follow verbatim, says "Where the specification is unclear, ask the author; do not decide." The builder's brief says to choose the plainest correct option and record it here. I could not reach the author while building, so I made each choice below and recorded it. **Every entry is pending the author's review.** Fixes of plain inefficiencies are in [CHANGELOG.md](CHANGELOG.md), not here.

Each entry names the spec section it concerns.

## Interface (§4.1, §9)

- **DEC-1 C signatures.**
  - Calls return `sk_status`; results come back through out-parameters.
  - Each row has its own prefix: `sk_ctl_`, `sk_d1_`, `sk_d2_`, `sk_ceil_`.
  - The control, Design 1 and Design 2 share identical signatures, and skeletons.h has a table of function pointers for them.
  - The README gives the full API.
- **DEC-2 `*_audit_scheduled` (§13.3 item 2).**
  - The large-base rule needs the audit to know whether it is the last one, which `audit(class)` cannot tell.
  - `audit` is always complete. `audit_scheduled` applies the rule: in Design 2 with C > 1,024, it checks the coprimality of the pieces created or rewritten since the last audit against every other piece, and every other check in full. Nothing is missed, even if `audit_changed` did not run after the latest call.
  - On every other row, `audit_scheduled` is the same as `audit`.
- **DEC-3 `sk_ceil_gather_signed` (extension).** The rule's CH-all job negates gathered elements, which §9's `gather` cannot do. `sk_ceil_gather` is kept exactly as §9 defines it.
- **DEC-4 Inputs equal to the most negative integer** (§4.3 item 2) return `SK_EINVAL`. So do vectors of unequal length and gather positions out of range.
- **DEC-5 Error order.** An invalid handle is reported before a division by zero.
- **DEC-6 Ceiling extras.**
  - `size` is the number of stored vectors; `length` is the number of elements of one vector.
  - The ceiling has audits (§13.2 requires them, though §9 does not list them).
  - Working-copy handles have bit 31 set, so stored vectors are limited to 2^31 − 1 per class.
- **DEC-7 `*_overflow`** returns the record of §4.4 item 2, with a running count.

## Counters (§4.3 item 5, §12.1)

- **DEC-8 Bit routines.** §4.3 item 5 requires every routine to count its calls, but §12.1's table has no row for bit routines. They are counted in `bitops`. The trailing-zero counts inside the gcd are excluded (counting rule 3), and so is the `bits()` call the instrumentation makes for the histogram.
- **DEC-9 Divisions** are split into `div_exact` and `div_rem`; their sum is §12.1's "divisions".
- **DEC-10 Entry is split into core and store, like the operations.** The core reduces by the gcd; the store does the lookup and the append. In Design 2 (as amended Oct 29, 2026) the store refines nothing: its only arithmetic is `finish` and `power`. Refinement happens in the core of addition, subtraction and division, in their meetings.
- **DEC-11 The ceiling's `read` is counted under a sixth phase, `SK_PH_READ`.** It is the only read that does arithmetic (a gcd and two divisions), and §12.1's phases have no place for it.
- **DEC-12 Not counted:**
  - negation: subtraction's −m_y, entry's sign move, the ceiling's signed gather;
  - |m_y| in division.
  These follow counting rule 1, which covers the sign. None of them can overflow, because the most negative integer is excluded.
- **DEC-13 Lookup growth.** Placing every index again costs one search per hash and one probe per slot read (rule 4), counted in store. Design 1's pair set is counted the same way, in the core of the operation that inserts: one `lookup_search` per hash and one `lookup_probe` per slot read, for membership tests, insertions and re-placements on growth. Each membership test also counts one `pair_read`, as each pair-bit read did.
- **DEC-14 Design 2's refinement counters** (spec §12.1, as amended Oct 29, 2026).
  - "pieces created" counts the new pieces of each split (|P| − 1) and the piece `absorb` appends.
  - "row entries rewritten" counts the entries a split writes, |P| per row that holds the piece.
  - "rows visited in splits" is D + 1 per split, R included.
  - `scan_remainder_pieces` counts each test of a remainder against a piece in a meeting (one gcd each, including the retest after a division or a split). `scan_refine_pieces` counts each piece tested by `absorb`'s scan of the base.
  - `ref_remainder_pulls` counts each time a piece's powers are pulled out of a remainder into its row.
  - `ev_meetings` counts every meeting, including one whose two remainders are already 1 and which computes nothing. `ev_absorbs` counts each `absorb`, one per round in which the two remainders still shared a factor. `ev_extra_queued` counts the pieces a row gained in a meeting, each queued for the other remainder (the extra refinement).
  - `ev_rows_u1` and `ev_rows_unrefined` count new rows stored with u = 1 and with u > 1. The latter replaces `ev_rows_refined`, since a new row is no longer refined.
  - The gcd of the two remainders is counted as a gcd only. It has no counter of its own, because `ev_absorbs` and `ev_meetings` bound it.
- **DEC-15 Ceiling's "numerators visited by content"** counts visits in both loops: the gcd loop and the division loop.
- **DEC-16 Fast paths and the control.** The control's same-denominator test, and the ceiling's per-element same-denominator branch of the control's divide, count as `ev_fast_paths`.
- **DEC-17 Audits change no count.** They point the counting cell at a scratch cell while they run.

## Arithmetic order (§6–§9)

- **DEC-18 A quantity written twice in one pseudocode function is computed once.** Examples: the control's add, d_x/g; division's m_x/g_1 and t/g_1; Design 2 division's t/g_1, shared as u.
- **DEC-19 Design 2's walk.**
  - r_x and r_y start at u_i and u_j (spec §8.4, §8.6).
  - q^0 is not formed, and r_x is not multiplied when the exponent a − min(a, b) is 0.
  - When the exponent is above 0, q^n takes n − 1 multiplications and r_x is multiplied by it, even when r_x is still 1.
- **DEC-20 Pair refinement (§8.2 item 1)** is a worklist: each pending number is tested against the numbers already known to be pairwise coprime, so no gcd of a pair already known coprime is computed again. P is then sorted in increasing order (D2-2).
- **DEC-21 Design 2's `store` runs `finish` after the lookup misses but before it appends e** (§5 says append, then build structures). The values and counts are the same either way. This way an overflow in `finish`, which the arithmetic rules out since u divides e, would store nothing.
- **DEC-22 Lookup insertion** uses the empty slot where the failed search stopped, except after a growth, which places every index again (0..k) in index order.
- **DEC-23 Ceiling divide.** The control's divide (§6) needs operands in lowest terms, but vector elements are not, so each element is first reduced as `read` reduces it (a gcd and two divisions per operand element).
  - E starts at q_1 and takes the lcm with each later q_k, by one gcd each.
  - A zero element of y is detected before any arithmetic.
- **DEC-24 Ceiling gather** copies E and the chosen numerators, with no content step: a working copy is not a stored value, so canonical form is not required of it.

## Memory (§12.2)

- **DEC-25 Numerators are two parallel arrays**, the values and the 32-bit denominator indices, so that an entry is exactly w/8 + 4 bytes (12 at w = 64), with no padding.
- **DEC-26 Header sizes.**
  - Per-row headers (Design 1's incidence rows, Design 2's exponent rows) are 16 bytes: a pointer, a length and a capacity.
  - A ceiling vector is 24 bytes: E plus a 16-byte header. At w = 32 padding keeps it at 24, and it is counted at that real size.
- **DEC-27 What allocated bytes cover, and what is excluded.** Allocated bytes are the capacities of the same structures; lists grow by doubling from 16. Not counted:
  - the class header;
  - per-call temporaries (Design 2's R, L, "equal" and a meeting's two queues; the ceiling's divide arrays);
  - the test build's change lists.
  None of these is a structure of §5–§9.
- **DEC-28 Entry sizes follow the width**, so at w = 32 numerators take 8N, denominators 4D, and so on. §12.2 states the sizes at w = 64 only.

## Audits (§13.2)

- **DEC-29 Design 1's incremental audit.** "The rows created since then (divisor 1 only)" is read as: rows created since the last audit are checked in full. Such a row may already have gained entries as an operand.
- **DEC-30 The lookup's count in the incremental audit** is checked against an occupancy count kept in the test build only. The full audit scans every slot.
- **DEC-31 The change list** records the ids of the divisors inserted (ids are stable since Oct 29, 2026; before, it recorded values, because columns moved); dirty flags plus a list for rows and pieces; and the pairs inserted into Design 1's pair set. In Design 2 a row is marked when a split rewrites it, or when a meeting changes its remainder or adds to its known part, so `audit_changed` rechecks it (known part times u equals d).

## Design 2 on demand (spec §8, amended Oct 29, 2026)

Ported from the prototype that found the extra refinement. These are the choices the amended spec leaves open.

- **DEC-35 The meeting's worklist.**
  - Queue 0 holds the pieces u_i is tested against, queue 1 those for u_j. Queue 0 is emptied first, then one piece of queue 1, then queue 0 again. Each queue is first-in, first-out.
  - A queue is never given the same piece twice, whether or not that piece has been processed. The check is a linear search, which is index work.
  - When a split rewrites a piece that a queue holds, processed or not, its new pieces join that queue. Testing them again is redundant only when the piece was already processed against that remainder; the skeleton does not track that.
  - A remainder that has reached 1 is not tested.
  - The gcd of the two remainders is taken only when both queues are empty and both remainders are above 1. After an `absorb`, the queues run again.
- **DEC-36 No pair record.** Nothing records that two rows have met. A second meeting of the same pair tests the remainders again: the gcds are repeated, and nothing changes. A record of met pairs would be a new structure, outside the skeleton (spec §10).
- **DEC-37 The remainder of a result is a product of remainders that divides e.** The checks are u_i · u_j in addition, u_i · (t/g_1) in division, (u_i/h)(u_j/h') in multiplication and u_i² for multiplication with the same index. Each is formed with the shared, counted, checked multiplication, after e has been formed and checked. An overflow there cannot happen, because the product divides e; if one did, it would be a bug, so the library aborts rather than reporting `SK_EOVERFLOW`. A factor of 1 is not multiplied.
- **DEC-38 Multiplication's remainders.** After the walk over the pieces, g_2 takes gcd(m_y, u_i) and g_1 takes gcd(m_x, u_j), each with the numerator already divided by the per-piece gcds. A remainder of 1 costs no gcd.
- **DEC-39 Memory.** The remainders are one full-width word per denominator, item `remainders`, sharing the denominators' capacity.
- **DEC-48 A split visits the stored rows only.** Splits happen only in meetings, and a meeting runs before the operation builds its result's row R (`begin` empties R first), so R is empty during every split. The earlier code also visited R; that branch was unreachable, and it added 1 per split to `scan_split_rows`. It is removed, and `scan_split_rows` now counts D per split, the stored rows. The spec's split pseudocode (§8.2) is clarified to match. No value, structure or other count changes.
- **DEC-49 The meeting's iteration bound (test build only).** An injected fault in independent review made a meeting loop forever instead of failing. In the test build (`SK_AUDIT`), every loop step of a meeting counts one: each repeat, each queue take, each step of `reduce` and of `absorb`. With B = bits(u_i) + bits(u_j) at the start, the bound is 8 (B + 2) (C + len_i + len_j + (B + 1)(w + 1)). It is generous: a meeting makes at most B + 1 repeats, B absorbs and B divisions, splits at most B pieces, each into at most w pieces, and every scan covers at most the base and the pieces created. A meeting past the bound aborts with "meeting passed its iteration bound (a fault)". The step count is not a counter and changes no count. The timing and counting builds have neither the count nor the bound. Test: `test_d2_meet_bound`, whose test-only hook `sk_d2_t_fault_stuck` stops `reduce` from dividing out a piece; the meeting aborts in a child process.

## Design 1's pair set (spec §7.1, amended Oct 29, 2026)

- **DEC-40 Layout.**
  - The set is an array of 64-bit keys `(j << 32) | i` with i < j, using the lookup's multiplicative hash on the key and linear probing. 0 marks an empty slot; no key is 0, since j ≥ 1.
  - There is no table until the first insertion, which allocates 16 slots. Before an insertion would pass half full, the table doubles and the old slots are re-placed in order.
  - `pair_set` does not test membership (DEC-47): its only caller, `compare`, inserts only after finding the pair absent.
  - Keys are index pairs, so their size does not follow the width.
- **DEC-41 Memory.** Item `pair set`, used = allocated = 8 × slots, the whole table, as the lookup reports its 4S. It is 0 until a pair is compared.
- **DEC-42 Audits.**
  - The full audit checks the set's shape: a power-of-two size, at most half full, the count equal to the keys held, and each key a pair i < j < D found at its slot. It then checks every pair as before.
  - The incremental audit checks the load, and that each pair inserted since the last audit is in the set, then checks it as before.
  - `sk_d1_t_pair` and `sk_d1_t_pairs` are uncounted test views.
- **DEC-47 No membership search on insertion** (a plain inefficiency, fixed inside the skeleton: CHANGELOG CL-3). `pair_set` used to search the set for the pair before inserting it, though `compare` had just found it absent. That search is removed. Each insertion now counts one fewer `lookup_search` and its probes. A duplicate insertion, which no operation makes, would break the count of keys, which the full audit checks. No value, structure, pair or other count changes.

## Design 1's sparse incidence rows (spec §7.1, amended Oct 29, 2026)

- **DEC-43 Ids, order and counters.**
  - **Ids.** Divisor ids are append-only: a new divisor takes id C, and `dv[id]` never changes. A second array, `ord`, lists the ids sorted by value. The binary search for g runs over `ord`, comparing values only (uncounted, as before), and inserting g moves the later ids of `ord` up by one. Nothing else moves, and no row is touched or renumbered. This is the plainest form with stable ids: a list kept sorted by value would renumber every row entry above the new divisor.
  - **Divisor 1 is explicit.** Entry 0 of every row is (id 0, d_k), as column 0 was always set. The search needs no special case, and residual(k, 1) = d_k is stored as before.
  - **Search.** It merges the two rows from their last entry down. When the ids differ, it steps down the row whose divisor is larger; it stops at the first equal id, at the latest at entry 0. Each step compares one pair of entries and counts one `scan_and_words`, the match included, as each word ANDed counted one, the hit included. The positions found give the residuals directly, so the search reads no residual by popcount.
  - **Placing a residual.** g = c·g' is above c, so its place in a row is above the entry where the merge stopped. It is found by a binary search of those entries, and each entry read counts one `scan_popcount_words`, the cost of locating a residual, as each word popcounted counted one. If the row already holds g, nothing is inserted. Otherwise the residual and id are inserted at that place, and the later entries of that row only move up.
  - **Counters kept, meanings stated.** The counter names are unchanged, so the harness's counter list is unchanged. `scan_and_words` is now "row-merge steps in searches", and `scan_popcount_words` is "row entries read to place a residual". `ref_words_shifted` and `ref_stride_growths` remain and are always 0, since no row shifts and there is no stride. Design 1 now calls no bit routine, so its `bitops` is 0.
- **DEC-44 Layout.** A row is one block of `cap` residuals followed by `cap` 32-bit ids, behind a 16-byte header (pointer, length, capacity). An entry is therefore w/8 + 4 bytes with no padding: 12 at w = 64, 8 at w = 32. A new row has capacity 1 and doubles when full, as the residual rows did.
- **DEC-45 Memory.** Item `incidence rows` replaces `incidence bits` and `residual rows`. Its used bytes are (w/8 + 4) × entries + 16 × D; its allocated bytes are the capacities plus 16 × the header slots. Item `divisors` is (w/8 + 4) × C, the values and `ord`.
- **DEC-46 Audits.**
  - **Full audit.** It checks `dv[0] = 1` and `ord[0] = 0`. It checks that `ord` holds C ids below C with strictly increasing values, which makes it a permutation and the divisors distinct. Every row is checked in full: length in 1..capacity, entry 0 is id 0, ids below C, entries strictly increasing by divisor value, and divisor × residual = d_k. Every pair in the set is checked by the search, as before.
  - **Incremental audit.** Each inserted id is checked against its two neighbours in `ord`. New rows and rows that gained an entry are checked in full. Pairs are checked as before.
  - **Test views.** `sk_d1_t_divisor(col)` and `sk_d1_t_bit(k, col)` take a **rank** by value, so the note's column examples read unchanged. `sk_d1_t_divisor_id(col)` gives the id at that rank. `sk_d1_t_W` is removed.

## Build

- **DEC-32 Settings.** `SK_WIDTH` (64 or 32), `SK_COUNT` and `SK_AUDIT` are the only build settings. `SK_AUDIT` requires `SK_COUNT`. The compiler is gcc, with `-std=c11 -O2 -Wall -Wextra -Werror` for every row.
- **DEC-33 The shared integer module** is a header of `static inline` functions (`src/sk_internal.h`), one source used by every row. Checked arithmetic uses `__builtin_*_overflow` on `sk_int`, so nothing is computed wider than w.
- **DEC-34 Test hooks.** Read-only views, and in the test build corruption hooks, are exported from the library for the unit tests only (`tests/introspect.h`). They are not part of the public API.

## Ambiguities in the spec, and points for the author

1. **§11 against the brief:** see the top of this file.
2. **§13.3 item 2:** the large-base rule cannot be met by `audit(class)` alone (DEC-2).
3. **§9 against the rule's CH-all:** the negated gather (DEC-3).
4. **§9 item 4:** the control's divide applied to elements that are not in lowest terms (DEC-23).
5. **Divisions by 1.**
   - §7.5's Design 1 add divides by h after setting h to 1. This is fixed as CL-1, but §15.1 does not list it.
   - Divisions by a gcd that turns out to be 1 are left literal in every row (see CL-1).
6. **§12.1** has no row for bit-routine calls (DEC-8) and no phase for the ceiling's read (DEC-11).
7. **§12.2** gives entry sizes at w = 64 only (DEC-28).
8. **The note's lookup example** uses slot = value mod 8. The spec's hash (G-5) differs, so the example's slots cannot be reproduced; the unit tests check the behaviour instead (growth at half full, and found at its index).

## Change log

- **Oct 29, 2026: review fixes, after two independent reviews (adversarial differential tests, fault injection), which found no correctness bug.** Design 1's pair insertion no longer searches for the pair first (DEC-47, CL-3): fewer `lookup_search` and `lookup_probe` counts in the operations that compare. Design 2's split no longer visits the empty result row (DEC-48): `scan_split_rows` is 1 lower per split. In the test build, a meeting has an iteration bound, so a fault fails loudly instead of hanging (DEC-49). Every stored value is the same: the differential test's checksums are unchanged at both widths (64: 389aef6a973dd3a3, 32: 545098a69c2331ce).

- **Oct 29, 2026: Design 1's incidence matrix is sparse (spec §7, amended), approved by the author.** The dense D × C bit matrix and the residual rows are merged into one sparse row per denominator: entries (divisor id, residual), sorted by divisor value. Divisor ids are stable, and no row shifts on an insertion (DEC-43 to DEC-46). The search merges two rows from the top down, and the merge finds the same largest common divisor that the AND found. So every stored value, gcd, division, multiplication, compare, pair hit and divisor insertion is the same. The differential test's checksums are unchanged at both widths (64: 389aef6a973dd3a3, 32: 545098a69c2331ce). Only the scan counters change: they now count merge steps and entries read. The words-shifted and stride-growth counters are now 0, and so is `bitops`. The memory items `incidence bits` and `residual rows` become `incidence rows`.

- **Oct 29, 2026: Design 1 stores only the compared pairs (spec §7, amended), approved by the author.** The dense pair table of D(D−1)/2 bits is replaced by a hash set of compared pairs (DEC-40 to DEC-42). A pair is inserted exactly where a bit was set, so every stored value, gcd and pair hit is the same: the differential test's checksums and pair-hit counts are unchanged at both widths. The counters keep their meaning; a membership test is one `pair_read` plus a lookup (DEC-13). The memory item is renamed from `pair table` to `pair set`. The incidence bits and residual rows are unchanged.

- **Oct 29, 2026: Design 2 is refined on demand (spec §8, amended), approved by the author.** `src/d2.c` is replaced by the on-demand form, ported from the prototype, under the same row name. Entry computes no gcd against the base. Addition, subtraction and division meet their two operand rows. A new result is stored with the known row and an unrefined remainder. The test hooks gain `sk_d2_t_u`, `sk_d2_t_corrupt_u` and `sk_d2_t_skip_extra`. Counters added: `scan_remainder_pieces`, `ref_remainder_pulls`, `ev_meetings`, `ev_absorbs` and `ev_extra_queued`. `ev_rows_refined` is renamed `ev_rows_unrefined` (DEC-14). The harness's counter list (`test/api.h`) carries the new names. The differential test's checksums are unchanged at both widths: every stored value is the same. The other rows are unchanged. The reason is in [../research/eager-d2-finding.md](../../../09-coprime-base/research/eager-d2-finding.md).

- **Oct 29, 2026: shared gcd made branch-free, same method, same counts; speeds up every row; approved by the author.** `sk_gcd_raw` in `src/sk_internal.h` (the binary method of DEC-33, shared by every row) now takes the minimum and the absolute difference by conditional moves instead of a compare-and-swap branch. It is the same binary method on the same magnitudes, so every result is the same; the gcd counters count calls, not loop steps, so every count is the same.
