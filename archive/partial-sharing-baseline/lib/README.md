# skeletons: the library of spec v0.1

C11 library for [spec v0.1](../v0.1.md). It has four rows: the control (§6), Design 1 (§7, amended Oct 29, 2026: a set of compared pairs, and sparse incidence rows), Design 2 (§8, amended Oct 29, 2026: refined on demand) and the shared-denominator ceiling (§9). They share one integer module (§4.3), one lookup (§5), counters (§12.1), a memory report (§12.2) and audits (§13.2).

Choices the spec leaves open are in [DECISIONS.md](DECISIONS.md). Fixes of plain inefficiencies are in [CHANGELOG.md](CHANGELOG.md) (spec §11).

**Layout since Oct 29, 2026.** Designs 1 and 2 are now separate specifications, so their sources live in their own folders and are built against the shared core here, unchanged:

| Part | Where |
| --- | --- |
| shared core: integer module (`src/sk_internal.h`), lookup and common code (`common.c`), the control (`ctl.c`), the ceiling (`ceil.c`), the row type (`include/sk_row.h`) and the control's row (`rows.c`) | this folder |
| Design 1: `src/d1.c`, `include/sk_d1.h`, its row (`src/rows_d1.c`) and unit tests | [`08-common-divisors/lib`](../../08-common-divisors/lib/) |
| Design 2: `src/d2.c`, `include/sk_d2.h`, its row (`src/rows_d2.c`) and unit tests | [`09-coprime-base/lib`](../../09-coprime-base/lib/) |

`make` here still builds the joint library of all four rows, as measured; `make test` runs this folder's tests (the control, the ceiling, the row table, and the differential test of all rows) and then each design's `make test`. Each design folder builds alone, with the core and its own design only. The unit tests of the former single `tests/unit.c` are split into three programs, one per folder, with the shared checks and per-row tests in `tests/t_common.h`; together they make every check of the former program. The decisions and change log below stay here, for all rows.

## Deviations from and additions to the spec's interface (read first)

The spec's interface table (§4.1, §9 item 2) gives call names but no C signatures. Everything below the table follows it. These are the additions and the one place this API goes beyond §9:

| What | Why |
| --- | --- |
| **`*_audit_scheduled`** on every row, next to `*_audit` and `*_audit_changed` | §13.3's large-base rule changes every scheduled full audit *except the last*, and `audit(class)` cannot tell which audit is last. `*_audit_scheduled` applies the rule. In Design 2 with more than 1,024 pieces, it checks pairwise coprimality only for pieces created or rewritten since the last audit. On every other row, and in Design 2 at or below 1,024 pieces, it is the same as `*_audit`. Use `*_audit` after the last call. |
| **`sk_ceil_gather_signed`** (extension, not in §9) | The rule's CH-all job "gathers the 1,000 y_k, negated where the step subtracts", but §9's `gather` has no way to negate. `sk_ceil_gather` is exactly §9. |
| `*_overflow` | Reads back the overflow record that §4.4 item 2 requires: the operation, the operands and the step. |
| `sk_ceil_size`, `sk_ceil_length` | Not listed in §9. Size is the number of stored vectors; length is the number of elements of one vector. |
| Ceiling audits | §9 item 2 does not list them, but §13.2 requires them. |
| `sk_row_ctl`, `sk_row_d1`, `sk_row_d2` | A table of function pointers, so a harness can drive the three rows through one code path (`skeletons.h`). It is a convenience only: it calls the direct functions. |

## Build

```
make            # all six libraries: build/w{64,32}/{timing,counting,test}/libskeletons.a
make test       # unit + differential tests at both widths; the three builds must print the same checksum
make sanitize   # the same tests under ASan + UBSan
make check      # test + sanitize
```

There is one compiler (gcc) and one set of flags, `-std=c11 -O2 -Wall -Wextra -Werror`. The three builds differ only in `-D` settings (§4.6):

| Build | Flags | Contents |
| --- | --- | --- |
| timing | (none) | counters and audits compiled out |
| counting | `-DSK_COUNT` | counters |
| test | `-DSK_COUNT -DSK_AUDIT` | counters, audits, the change list, and Design 2's meeting iteration bound (DEC-49) |

The width is `-DSK_WIDTH=32`; the default is 64 (§4.3 item 1). **A program must be compiled with the same `SK_WIDTH` as the library it links.** `sk_build_width()`, `sk_build_counting()` and `sk_build_audit()` report how the library was built.

Link with `-Iinclude -I../../../08-common-divisors/lib/include -I../../../09-coprime-base/lib/include build/w64/<build>/libskeletons.a`. The library uses no other library (§4.6 item 1).

## API

Include `skeletons.h` for everything, or include one row's header: `sk_ctl.h`, `sk_d1.h`, `sk_d2.h` or `sk_ceil.h`. The common types are in `sk_common.h`.

### Types

- `sk_int` is a full-width integer: `int64_t`, or `int32_t` at width 32. Valid inputs and results lie in `[-SK_INT_MAX, SK_INT_MAX]`. Denominators lie in `[1, SK_INT_MAX]`. The most negative integer is never valid (§4.3 item 2).
- `sk_handle` (`uint32_t`) is the position of a value in its class. Handles are 0, 1, 2, … in the order values are stored.
- `sk_status`:
  - `SK_OK`
  - `SK_EOVERFLOW`: nothing stored (§4.4)
  - `SK_EDIVZERO`: `enter` with d = 0, or `div` with y = 0
  - `SK_EINVAL`: a handle not in the class, the most negative integer as an input, vectors of unequal length, or a position out of range
  - `SK_EAUDIT`: an audit failed
  - `SK_ENOTBUILT`: counters or audits were asked of a build that lacks them
- Out of memory, and the limits of §14, print a message and `abort()` (§4.5).

### The control, Design 1 and Design 2 (identical signatures; `xx` is `ctl`, `d1` or `d2`)

```c
sk_xx    *sk_xx_create(void);                                                 /* counters are reset only here */
void      sk_xx_destroy(sk_xx *c);
sk_status sk_xx_enter(sk_xx *c, sk_int m, sk_int d, sk_handle *out);          /* m/d in lowest terms */
sk_status sk_xx_add(sk_xx *c, sk_handle x, sk_handle y, sk_handle *out);      /* also sub, mul, div */
sk_status sk_xx_read(const sk_xx *c, sk_handle x, sk_int *m, sk_int *d);
uint32_t  sk_xx_size(const sk_xx *c);                                          /* N */
void      sk_xx_memory(const sk_xx *c, sk_memory *report);                     /* every build */
sk_status sk_xx_counters(const sk_xx *c, sk_counters *report);                 /* SK_ENOTBUILT in timing */
sk_status sk_xx_overflow(const sk_xx *c, sk_overflow *report);                 /* SK_EINVAL if none yet */
sk_status sk_xx_audit(sk_xx *c, const char **why);                             /* full, complete */
sk_status sk_xx_audit_scheduled(sk_xx *c, const char **why);                   /* full, large-base rule */
sk_status sk_xx_audit_changed(sk_xx *c, const char **why);                     /* incremental */
```

- On any error nothing is stored, and `*out` is not written.
- Subtraction is computed as `x + (-y)`, but it is counted under its own phase, `SK_PH_SUB`.
- The audits return `SK_OK`, or `SK_EAUDIT` with `*why` set to a message naming the first failure. The message stays valid until the class's next audit. In the timing and counting builds the audits return `SK_ENOTBUILT`.
- Each audit resets "since the last audit" for `audit_changed` (§13.2).

### The ceiling

```c
sk_ceil  *sk_ceil_create(void);
void      sk_ceil_destroy(sk_ceil *c);
sk_status sk_ceil_enter_vector(sk_ceil *c, const sk_int *m, uint32_t len, sk_int d, sk_handle *out);
sk_status sk_ceil_gather(sk_ceil *c, sk_handle v, const uint32_t *positions, uint32_t len, sk_handle *out);
sk_status sk_ceil_gather_signed(sk_ceil *c, sk_handle v, const uint32_t *positions, const unsigned char *negate,
                                uint32_t len, sk_handle *out);                 /* extension, see above */
sk_status sk_ceil_add(sk_ceil *c, sk_handle x, sk_handle y, sk_handle *out);   /* also sub, mul, div */
sk_status sk_ceil_read(sk_ceil *c, sk_handle v, uint32_t k, sk_int *m, sk_int *d);  /* counted, SK_PH_READ */
sk_status sk_ceil_length(const sk_ceil *c, sk_handle v, uint32_t *len);
uint32_t  sk_ceil_size(const sk_ceil *c);                                      /* stored vectors */
void      sk_ceil_memory(const sk_ceil *c, sk_memory *report);
sk_status sk_ceil_counters(const sk_ceil *c, sk_counters *report);
sk_status sk_ceil_overflow(const sk_ceil *c, sk_overflow *report);
sk_status sk_ceil_audit(sk_ceil *c, const char **why);    /* also _audit_scheduled (the same) and _audit_changed */
```

- Stored vectors are entered vectors and results. Their handles are 0, 1, 2, ….
- Working copies from `gather` have handles with bit 31 set (`SK_CEIL_WORKING`). Any vector can be an operand or be read. Working copies are not stored values: they are not counted by `size`, they are not in the memory report and they are not audited.
- Operations work elementwise on vectors of equal length. `div` with any zero element of y returns `SK_EDIVZERO` for the whole operation, and an overflow fails the whole operation.

### Reports

- **`sk_counters`** is `c[phase][part]`, an array of `sk_count`.
  - Phases: `SK_PH_ENTRY`, `ADD`, `SUB`, `MUL`, `DIV`, and `READ` (ceiling only).
  - Parts: `SK_CORE` (everything before `store`, including Design 2's meetings, where all its refinement happens) and `SK_STORE` (lookup, append, the new denominator's structures, and Design 2's `finish` and `power`).
  - Each `sk_count` has these fields, from the table of §12.1:
    - gcds: `gcd`, `gcd_hist[65]` and `gcd_work`
    - divisions: `div_exact` and `div_rem`
    - arithmetic: `mul` and `add`
    - bit routines: `bitops`
    - lookups: `lookup_search`, `lookup_probe` and `pair_read`
    - scans: the `scan_*` fields
    - refinements: the `ref_*` fields
    - events: the `ev_*` fields
  - Fields that do not apply to a row stay 0. The counters can be read at any time.
- **`sk_memory`** has `item[0..n-1]` = {name, used, allocated} for each structure of §12.2, the totals `used` and `allocated`, `values` (N), and `used_per_rational`, which is the bytes per rational. It also has `allocated_per_rational`.
  - Items, in order:
    - ctl: `values`
    - d1: `numerators`, `denominators`, `lookup`, `divisors`, `pair set`, `incidence rows`
    - d2: `numerators`, `denominators`, `lookup`, `base`, `exponent rows`, `remainders`
    - ceil: `stored vectors`
  - Entry sizes follow the width: at w = 32, numerators take 8 bytes and denominators 4. Row headers are 16 bytes; a ceiling vector's E plus its header is 24 bytes. Design 1's pair set is 8 bytes per slot at both widths (its keys are index pairs), and is reported whole, as the lookup is. Design 1's `divisors` are w/8 + 4 bytes each (the value, and its id in the list sorted by value); an entry of its `incidence rows` is a residual and a 32-bit divisor id, w/8 + 4 bytes, with no padding.
  - Design 1's counters on its sparse rows (DEC-43): `scan_and_words` counts merge steps of the search, `scan_popcount_words` counts row entries read to place a new residual, and `ref_words_shifted` and `ref_stride_growths` stay 0, since no row shifts.
- **`sk_overflow`** has `count` (overflows so far), and for the last overflow `op`, `x`, `y` and `step`. The step is named after the pseudocode, for example `"add: t = m_x*r_y + m_y*r_x"`.

### Running the gate (spec §13.3 item 2), as the tests do

Count every call that stores a value: `enter`, `enter_vector` and the operations. After call n:

- call `*_audit_scheduled` if n ≤ 200 or n % 64 == 0;
- call `*_audit` after the last call;
- `*_audit_changed` may be called after every call. **It must be called after every call once Design 2's base has more than 1,024 pieces.**

## Layout

| File | Contents |
| --- | --- |
| `include/sk_common.h` | width, `sk_int`, status, phases, counters, memory report, overflow record |
| `include/sk_{ctl,d1,d2,ceil}.h`, `include/skeletons.h` | the rows; the function-pointer table |
| `src/sk_internal.h` | **the shared integer module**: binary gcd, checked add, subtract and multiply, exact division, division with remainder, bit routines, all counted. Also allocation, the counting cell and the lookup's interface |
| `src/common.c` | the lookup (§5), the control's divide (shared with the ceiling, §9 item 4), names, memory helpers |
| `src/ctl.c`, `src/d1.c`, `src/d2.c`, `src/ceil.c` | the four rows, each with its audits. `d1.c` and `d2.c` end with read-only test hooks; in the test build, every row's file ends with corruption hooks for the tests |
| `src/rows.c` | the function-pointer table |
| `tests/unit.c` | unit tests (below) |
| `tests/diff.c`, `tests/ref.h` | the randomized differential test against an independent `__int128` reference |

## Tests

All of these pass at widths 64 and 32, in the test build and under ASan + UBSan.

- **`tests/unit.c`** (4,664 checks per width):
  - every worked example of the note, with its intermediate structures:
    - Design 1's divisors, incidence rows with their residuals, and compared pairs, islands, and the chain and pair-hit counts
    - the incidence example [1, 4, 6, 15], where 4 is inserted below 6: ids stay as they were and no row shifts
    - Design 2's base, its meetings, splits, exponent rows and remainders: bare rows at entry, and only what each meeting needs refined
  - zero, negatives, division by zero, the most negative integer, invalid handles, same-index fast paths and denominator 1
  - an overflow in each operation, with the record checked and the class still valid
  - Design 2's extra refinement: its counterexample, 3/4 + 1/8, gives 7/8 with it, and 14/16 without it (via a test hook), which the audit then rejects
  - Design 2's meeting iteration bound: an injected fault that would make a meeting loop forever aborts it instead (in a child process)
  - composite pieces in multiplication (the base [6] case, where the leftover stays in the remainder until it meets row 6 and splits 6) and in addition, with exact counts
  - u_12 = 12 meeting the base [6], where the remainder test stays at a piece after dividing by it and after splitting it
  - lookup-found results with no power test, and h = 1 with no division
  - islands for each operation, including multiples of an operand's denominator
  - more than 64 divisors inserted in decreasing order: ids in order of insertion, sorted by value, nothing shifted
  - Design 1's pair set: empty until a pair is compared, growth at half full, either order of a pair, nothing inserted by multiplication, and a membership test counted as one pair read plus a lookup
  - the large-base audit
  - lookup growth at half full
  - exact memory accounting
  - the counting rules
  - audits that catch deliberately broken invariants
  - the ceiling
  - all rows agreeing through the function-pointer table
- **`tests/diff.c`**: 40 seeds × 7 workload families (shared factors, large shared factors, composites, none, one denominator, small, near full width), with half the operands earlier results, plus a ceiling run per seed.
  - Operations within the operand rule must succeed in every row. A result that does not fit must overflow.
  - Every stored value is read out and checked.
  - It checks the overflow counter and records, that Design 1's and Design 2's new-denominator counters equal the distinct stored denominators, and that the memory totals add up. Its coverage line reports Design 2's meetings, absorbs, extra refinements and splits.
  - Audits follow the gate schedule, with incremental audits after every call or every 7th call.
  - The timing, counting and test builds must print the same checksum of every status, handle and value (§4.6 item 4).

No timed measurement is part of the library or its tests.
