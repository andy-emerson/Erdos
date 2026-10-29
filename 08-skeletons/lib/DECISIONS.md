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
- **DEC-10 Entry is split into core and store, like the operations.** The core reduces by the gcd; the store does the lookup and, in Design 2, the refinement.
- **DEC-11 The ceiling's `read` is counted under a sixth phase, `SK_PH_READ`.** It is the only read that does arithmetic (a gcd and two divisions), and §12.1's phases have no place for it.
- **DEC-12 Not counted:**
  - negation: subtraction's −m_y, entry's sign move, the ceiling's signed gather;
  - |m_y| in division.
  These follow counting rule 1, which covers the sign. None of them can overflow, because the most negative integer is excluded.
- **DEC-13 Lookup growth.** Placing every index again costs one search per hash and one probe per slot read (rule 4), counted in store.
- **DEC-14 Design 2's refinement counters.**
  - "pieces created" counts the new pieces of each split (|P| − 1) and the piece `refine` appends.
  - "row entries rewritten" counts the entries a split writes, |P| per row that holds the piece.
  - "rows visited in splits" is D + 1 per split, the new row included.
- **DEC-15 Ceiling's "numerators visited by content"** counts visits in both loops: the gcd loop and the division loop.
- **DEC-16 Fast paths and the control.** The control's same-denominator test, and the ceiling's per-element same-denominator branch of the control's divide, count as `ev_fast_paths`.
- **DEC-17 Audits change no count.** They point the counting cell at a scratch cell while they run.

## Arithmetic order (§6–§9)

- **DEC-18 A quantity written twice in one pseudocode function is computed once.** Examples: the control's add, d_x/g; division's m_x/g_1 and t/g_1; Design 2 division's t/g_1, shared as u.
- **DEC-19 Design 2's walk.**
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
  - Per-row headers (residual rows, exponent rows) are 16 bytes: a pointer, a length and a capacity.
  - A ceiling vector is 24 bytes: E plus a 16-byte header. At w = 32 padding keeps it at 24, and it is counted at that real size.
- **DEC-27 What allocated bytes cover, and what is excluded.** Allocated bytes are the capacities of the same structures; lists grow by doubling from 16. Not counted:
  - the class header;
  - per-call temporaries (Design 2's R, L and "equal"; the ceiling's divide arrays);
  - the test build's change lists.
  None of these is a structure of §5–§9.
- **DEC-28 Entry sizes follow the width**, so at w = 32 numerators take 8N, denominators 4D, and so on. §12.2 states the sizes at w = 64 only.

## Audits (§13.2)

- **DEC-29 Design 1's incremental audit.** "The rows created since then (column 0 only)" is read as: rows created since the last audit are checked in full. Such a row may already have gained bits as an operand.
- **DEC-30 The lookup's count in the incremental audit** is checked against an occupancy count kept in the test build only. The full audit scans every slot.
- **DEC-31 The change list** records, by value, the divisors inserted (columns move); dirty flags plus a list for rows and pieces; and the pairs set.

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
