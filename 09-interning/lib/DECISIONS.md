# Library decisions (layer 1: L1 and L1-opt)

Choices made where spec v0.1 is silent or unclear, each the plainest correct option found (spec §12, L-19). Each is
**pending the author's review**. None changes a structure, a case order or a value the spec fixes. Items marked
**ambiguous** are also listed at the end.

No plain inefficiency was found to fix, so the change log is empty and no `CHANGELOG.md` exists.

## Source and copies

- **D-1. The 08 copies** (spec §4.3 item 4, §9). `vendor08/` holds `sk_internal.h`, `common.c`, `ctl.c`, `sk_common.h`
  and `sk_ctl.h`, copied unchanged from 08-skeletons at commit `08bbfbe` (its HEAD on Oct 29, 2026). 08's working tree
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

## Ambiguous or not as written (for the author)

1. 08's `sk_common.h` was copied at commit `08bbfbe`, not from the working tree, which has uncommitted Design 2 fields (D-1).
2. Case num with g = 1, and case den with h = 1, divide by 1 in L1 as the pseudocode writes (D-6).
3. The scope of O-4's "known 1", and O-4 in entry (D-10).
4. O-2 in entry, and its switch (D-11).
5. O-5's divisions skipped, needed by §17.3 item 3 but not listed in §17.2 (D-13).
