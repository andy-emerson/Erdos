# Spec 13, round 1: results

Nov 1, 2026. Round 1 of [`v0.1.md`](v0.1.md) §4, correctness only, in Python at $W = 4$. Code: [`test/`](test/) (`words.py` shared routines, pools and result classes; `model_a.py`; `model_b.py`; `axes.py` the four-direction check; `round1.py` the harness). Raw output: `test/out/r1.json`, `test/out/run.txt`. Run with `python3 test/round1.py` from `spec/13-header`. The full run takes 104 s on 2 cores. No random count was reduced.

**Verdict: both designs fail.** Three spec rules cause every failure. With the stale-word gap (F1) patched in a diagnostic run, only F2 and F3 remain, in both designs. The four-direction mathematics passes, except that A's header formulas reject a zero added to an off-axis value (F4).

## What was checked

Both models work word by word: 4-bit words, headers as specified (A 8 bits, B 16 bits), classes 0 to 31, one pool per class, slot indices stored in words, the free list linked through freed slots' first words and ended by all ones (15), pools growing from one slot by doubling, result slots allocated before any pointer is taken, the move-down, and release. Pool growth copies memory to a new list and poisons the old one, so a stale pointer would show. The routines are Growing's (spec 12 `growing.nim`), with carries and borrows as comparisons. Python integers are used only as the reference.

Each case runs the operation and then these checks:
- **value:** the model's word-by-word readout equals Python's result (for B, $P - N$);
- **canonical:** every A result, every part of every B result and every collapsed B result is in its smallest class, zero is class 0 word 0 (A header 0), and spare bits are 0;
- **collapse:** "output collapses too" is read as the output step, so every B result is also collapsed, and the collapsed value and its canonical form are checked;
- **bound:** no routine writes past its §1 bound;
- **release:** after the operands and results are released, every slot of every pool is on its free list exactly once;
- **error:** the operation raises: an overflow past a slot or the one-word local, a class above 31, an index too wide for a word, or a Python fault.

After a failing case the pools are reset, so one fault does not cascade.

The sets:
- **signed8:** every pair of signed values $|v| < 2^8$ (261,121 pairs). For negate, every value (511). For division, every pair with $b \ne 0$ and $b \mid a$ (6,338). B takes the collapsed form.
- **unc4 (B only):** every pair of uncollapsed $(P, N)$ with both parts below $2^4$ (65,536 pairs; negate 256; division 19,712 pairs whose value divides).
- **random:** 100,000 cases per operation and design. Values have up to 8 words, with zeros and equal magnitudes each forced at 1/16. B's parts are drawn on their own, and $P = N$ is forced at 1/16. Division builds $a = bq$. Gcd gives half its pairs a common factor (a builder's choice; plain random pairs almost always have gcd 1).

Two modes:
- **literal:** the spec as written. Memory the model never wrote holds all ones.
- **zeroed:** a diagnostic, not the spec. Every allocation zeroes its slot. It shows what fails besides F1.

### Counts and results per design, set and operation

Each cell gives the cases that failed, then the failing checks with their counts. Checks not named passed.

| design | set | op | cases | literal: failing | literal: checks | zeroed: failing | zeroed: checks |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A | signed8 | add | 261121 | 65280 | value 65280 | 0 | pass |
| A | signed8 | sub | 261121 | 65280 | value 65280 | 0 | pass |
| A | signed8 | neg | 511 | 480 | release 480 | 480 | release 480 |
| A | signed8 | mul | 261121 | 0 (see note) | pass | 0 | pass |
| A | signed8 | div | 6338 | 0 | pass | 0 | pass |
| A | signed8 | gcd | 261121 | 1020 | error 960, bound 60 | 1020 | error 960, bound 60 |
| A | signed8 | cmp | 261121 | 0 | pass | 0 | pass |
| A | random | add | 100000 | 37765 | value 37765 | 0 | pass |
| A | random | sub | 100000 | 37701 | value 37701 | 0 | pass |
| A | random | neg | 100000 | 81978 | release 81978 | 81978 | release 81978 |
| A | random | mul | 100000 | 65213 | value 65213 | 0 | pass |
| A | random | div | 100000 | 9186 | value 9186 | 0 | pass |
| A | random | gcd | 100000 | 25251 | error 10513, value 13999, bound 739 | 11252 | error 10513, bound 739 |
| A | random | cmp | 100000 | 0 | pass | 0 | pass |
| B | signed8 | add | 261121 | 65280 | value 65280, collapse value 65280 | 0 | pass |
| B | signed8 | sub | 261121 | 65280 | value 65280, collapse value 65280 | 0 | pass |
| B | signed8 | neg | 511 | 480 | release 480 | 480 | release 480 |
| B | signed8 | mul | 261121 | 54928 | value 54928, collapse value 54928 | 0 | pass |
| B | signed8 | div | 6338 | 0 | pass | 0 | pass |
| B | signed8 | gcd | 261121 | 1020 | error 960, bound 60 | 1020 | error 960, bound 60 |
| B | signed8 | cmp | 261121 | 0 | pass | 0 | pass |
| B | unc4 | add | 65536 | 0 | pass | 0 | pass |
| B | unc4 | sub | 65536 | 0 | pass | 0 | pass |
| B | unc4 | neg | 256 | 0 | pass | 0 | pass |
| B | unc4 | mul | 65536 | 876 | value 876, collapse value 876 | 0 | pass |
| B | unc4 | div | 19712 | 0 | pass | 0 | pass |
| B | unc4 | gcd | 65536 | 7680 | bound 7680 | 7680 | bound 7680 |
| B | unc4 | cmp | 65536 | 0 | pass | 0 | pass |
| B | random | add | 100000 | 60483 | collapse value 60483, value 45074 | 0 | pass |
| B | random | sub | 100000 | 59999 | value 44490, collapse value 59999 | 0 | pass |
| B | random | neg | 100000 | 95856 | collapse value 53956, release 95856 | 95856 | release 95856 |
| B | random | mul | 100000 | 70040 | value 69649, collapse value 70040 | 0 | pass |
| B | random | div | 100000 | 17846 | collapse value 17846, value 5808 | 0 | pass |
| B | random | gcd | 100000 | 27007 | error 11564, value 13268, collapse value 15212, bound 231 | 11795 | error 11564, bound 231 |
| B | random | cmp | 100000 | 0 | pass | 0 | pass |

Note on A signed8 multiply: it passes in the literal run only because of the order of the pairs. Each run of slot reuse happens to begin with a product that writes the slot's top word as 0. With fresh pools for each pair, 23,872 of the 261,121 pairs fail. The smallest is $-20 \times -13$: expected 260, got 61700. B signed8 multiply fails 54,928 pairs either way. Division below $2^8$ passes for a real reason: its quotient never leaves a slot word unwritten at that size.

Checks with no failures in any run: canonical, spare bits, the B collapsed form, compare, and every errorless pool operation. In the zeroed run, add, subtract, multiply and division pass completely for both designs. The pools never held more than 7 slots per class.

### Four directions (value only, against Gaussian integers)

A: every pair with magnitude below $2^5$ in all four directions, plus 100,000 random pairs with up to 128-bit magnitudes. B: every pair of four-slot values with each slot below 4, plus 100,000 random pairs.

| check | cases | failures |
| --- | --- | --- |
| A multiply | 115625 | 0 |
| A divide ($a = bq$) | 108561 | 0 |
| A negate | 115625 | 0 |
| A add, within an axis | 58205 | 0 |
| A subtract, within an axis | 58205 | 0 |
| A add, error raised exactly for sums across axes | 57420 | **6139** (F4) |
| A subtract, same | 57420 | **6139** (F4) |
| B add (across axes included) | 165536 | 0 |
| B subtract | 165536 | 0 |
| B negate | 165536 | 0 |
| B multiply (slot $(d_1 + d_2) \bmod 4$) | 165536 | 0 |
| B collapse (operands and every result) | 662144 | 0 |

## Failures, with minimal counterexamples

### F1. Stale words above a value's length (both designs; add, subtract, multiply, division, gcd, collapse)

Rules: §1 Routines ("reads operands at their real length n (up to the top nonzero word)" and "writes at most its bound"), §1 Result class (the move-down), and §1 Pools (allocation pops a freed slot or takes new memory). A value's length is not stored, so a reader finds it by scanning the slot from the top. That works only if every word above the length is 0. No rule makes them 0. Allocation does not clear the slot. A routine writes only up to its bound, which can be smaller than the slot (bound 3 in a 4-word slot). The move-down copies $n$ words into a larger slot. Whatever the slot held before stays: an old value's words, the free-list link in word 0, or uninitialised memory.

Minimal counterexamples. Unwritten memory holds all ones; each case reproduces from empty pools:

| design | op | inputs | expected | got | how |
| --- | --- | --- | --- | --- | --- |
| A | add | 0xFF + 1 (also −255 + −1) | 256 | 0xF100 = 61696 | bound 3, class 2 (4 words), word 3 never written |
| A | sub | −255 − 1 | −256 | −61696 | same |
| A | mul | −20 × −13 | 260 | 61700 | bound 3 (2 + 1 words), class 2, word 3 never written |
| A | div | −272 / 1 | −272 | −61712 | bound 3, class 2 |
| A | gcd | gcd(267, −267) | 267 | 45323 | bound 3, class 2 |
| B | add | (0, 255) + (0, 1) | −256 | −61696 | N part: bound 3, class 2 |
| B | mul | (0, 16) × (0, 16) | 256 | 61696 | P part: bound 5, class 3, moved down to class 2 with 3 words, word 3 never written |
| B | mul (unc4) | (4, 15) × (8, 15) | 77 | 61517 | P part bound 3 from class-0 parts |
| B | div | (693, 15) / (0, 1) | −678 | −62118 | quotient bound 3 |
| B | collapse of neg | collapse((0, 257)) | −257 | −61697 | collapse bound 3 |

Even when new memory is zero, slot reuse alone triggers F1. In A, release a value 0xFFFF (class 2), then compute 0xFF + 1. The result takes the freed slot, whose word 3 is still 0xF, and reads as 0xF100.

In the zeroed diagnostic, where allocation zeroes the slot, every F1 failure disappears and nothing else fails in add, subtract, multiply or division. Zeroing in the routines (to the slot's end) and in the move-down would work as well.

### F2. Negate shares the operand's slot, so releasing both releases it twice (both designs)

Rules: §2 negate `h xor 0x80` (the word, which is the slot index, is kept); §3 negate "swap P and N" (the words are kept); §1 Pools "the owner releases a value explicitly". The negation is a second value naming the same slot. When the owners of both release them, the slot is pushed twice and the free list loops.

| design | inputs | expected | got |
| --- | --- | --- | --- |
| A | neg(16) (class 1) | −16; after release every slot is free once | pool 1: slot 0 twice on the free list |
| B | neg((0, 16)) | 16; same | pool 1: slot 0 twice on the free list |

Every negation of a value with a slot fails this way: 480 of the 511 signed8 values (the 31 class-0 values pass), and all of unc4, which is class 0 only. Subtract has no such failure, since its negation is internal and never released. B's "collapse value" failures under negate in the literal run are F1.

### F3. The gcd bound min(n1, n2) is wrong when an operand is zero (both designs)

Rules: §1 Routines "gcd min" and §1 Result class "the smallest class holding the bound". Since gcd(0, b) = |b|, which has n2 words, but min(0, n2) = 0, the result is given the one-word local. Growing's routine returns a copy of b, which then overflows when b has 2 or more words and exceeds the bound in any case.

| design | inputs | expected | got |
| --- | --- | --- | --- |
| A | gcd(16, 0) | 16 | overflow: word 1 written into the one-word local (bound 0) |
| A | gcd(0, −1) | 1 | value correct; bound broken (word 0 written, bound 0) |
| B | gcd((0, 16), (0, 0)) | 16 | overflow, as A |
| B | gcd((0, 0), (0, 1)) (unc4) | 1 | bound broken |
| B | gcd((97049091, 96982751), (133839, 133839)) | 66340 | overflow: an uncollapsed zero $(P = N)$ collapses to length 0 |

Possible fix: the bound is min over the nonzero operands, so max(n1, n2) when one operand is zero.

### F4. A's header formulas reject zero plus an off-axis value (four-direction check only)

Rule: §2 "Sums across axes are an error (zero has no axis)", with "zero is header 0". Zero's header gives direction 0, so zero + i·1 is neither an equal-direction add (directions 0 and 1) nor an opposite one (0 xor 1 ≠ 2), and the formulas raise the cross-axis error. The parenthesis says zero has no axis, so this sum should be allowed and equal i. Minimal: $0 + i$, expected $i$, got the cross-axis error. All 6,139 failures have a zero operand. Every true cross-axis sum of nonzero values raised the error, and every same-axis sum was correct. Integers (directions 0 and 2) are not affected.

## Spec issues

1. **F1: the slot's words above a value's length are unspecified,** and the scan for the real length needs them to be 0. The spec must say who zeroes them: allocation, or each routine and the move-down up to the slot's end. This is the blocking issue. It makes most results wrong.
2. **F2: negate aliases a slot.** The spec needs either a copy rule for negate (it then allocates, unlike `h xor 0x80` alone) or an ownership rule, for example that a negation is a borrowed view that is never released.
3. **F3: the gcd bound** for a zero operand (above).
4. **F4: zero plus an off-axis value** in A: "zero has no axis" against the formulas. Not reachable in integers.
5. **Slot index width at W = 4 (not triggered).** A slot index lives in one word and all ones ends the free list, so a pool can use only 15 slots at $W = 4$; slot 15 would read as the end marker. This run never held more than 7 slots per class, so it did not show. At $W = 64$ it does not matter, but the spec could say that the end marker is never a valid index.
6. **Division "exactly n1 − n2 + 1" (interpretation).** Growing's loop count after the shift, nx′ − ny′ + 1, can be one less than n1 − n2 + 1 (for example 16 / 2). The model writes exactly n1 − n2 + 1 words and reads dividend words at or past its current length as 0, since operands are read "at their real length". Growing's `shrw` leaves those buffer words stale, so a literal port would read garbage there. The extra quotient word is always 0. No failure came from this.
7. **A negate of zero (interpretation).** `h xor 0x80` turns zero's header 0 into 0x80. The model then applies "zero is header 0", which costs a zero test in negate. Without it, neg(0) is non-canonical.
8. **"Output collapses too" (interpretation)** is read as the output step. Every B result is collapsed and checked.
9. **B's four-slot collapse** is not defined in the spec. The model collapses per axis: |m0 − m2| goes to slot 0 or 2 by its borrow, and |m1 − m3| to slot 1 or 3.

Builder's choices (not spec issues): operands are built by test setup, which zero-fills their slots. Growing's round-2 one-word gcd special case is omitted (rule 2). B's compare uses one local in two halves. Division and gcd each use one working buffer, which B's collapse writes straight into. Half of the random gcd pairs share a factor.
