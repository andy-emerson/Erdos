# Spec 13, round 1: results (second run)

Nov 1, 2026. Round 1 of [`v0.1.md`](v0.1.md) §4, as revised the same day, correctness only, in Python at $W = 4$. Code: [`test/`](test/) (`words.py` shared routines, pools and result classes; `model_a.py`; `model_b.py`; `axes.py` the four-direction check; `round1.py` the harness). Raw output: `test/out/r1.json`, `test/out/run.txt`. Run with `python3 test/round1.py` from `spec/13-header`. The full run takes 52 s on 2 cores. No random count was reduced.

**Verdict: both designs pass every check.** Two new spec points (N1, N2) concern wording and rule 4, not correctness.

## First run (superseded)

The first run, on v0.1 before the revision, failed in both designs from four causes, and the spec has fixed each:
- **F1:** words above a value's length in its slot were never zeroed, so the length scan read stale words (for example, 0xFF + 1 gave 0xF100). Now results and the move-down zero-fill their slot.
- **F2:** negate shared the operand's slot, so releasing both released it twice. Now negate copies into a new slot.
- **F3:** gcd's bound min(n1, n2) was 0 when one operand was zero (gcd(16, 0) overflowed the one-word local). Now the bound is the nonzero operand's length, and gcd(0, 0) is an error.
- **F4:** in A, zero plus an off-axis value hit the cross-axis error. Now an add with a zero operand returns the other.

The first run's spec issues are resolved by the same revision:
- division writes exactly n1 − n2 + 1 words and reads past its length as 0;
- negating zero gives zero;
- B's four-slot collapse is per axis;
- output collapses only when a single value is needed;
- the 15-slot pool limit is stated.

## What the model follows

Both models work word by word:
- 4-bit words and headers as specified (A 8 bits, B 16 bits);
- classes 0 to 31, one pool per class, slot indices stored in words;
- the free list linked through freed slots' first words and ended by all ones (15);
- pools growing from one slot by doubling, up to $2^W - 1 = 15$ slots;
- result slots allocated before any pointer is taken;
- the move-down;
- release.

Pool growth copies memory to a new list and poisons the old one, so a stale pointer would show. Memory the model never wrote holds all ones, so reading an unwritten word would show. The routines are Growing's (spec 12 `growing.nim`), with carries and borrows as comparisons. Python integers are only the reference.

The revision is followed literally:
- every routine zeroes the rest of its slot after writing, and the move-down zero-fills;
- negate copies a pool number into a new slot, and an inline number copies its word;
- A's add and subtract return the other operand when one is zero (for a subtract with a zero first operand, the negated second), before any direction rule;
- negating zero gives header 0;
- gcd's bound is the nonzero operand's length when one is zero, and gcd(0, 0) raises an error before any allocation;
- division writes exactly n1 − n2 + 1 words, reading dividend words past its current length as 0;
- a pool's 16th allocation is an error.

## Checks

Each case runs the operation and then these checks. A failing case resets the pools, though none failed.
- **value:** the model's word-by-word readout equals Python's result (for B, $P - N$). For gcd(0, 0), the operation must raise its error.
- **canonical:** every A result, every part of every B result and every collapsed B result (division, gcd) is in its smallest class, zero is class 0 word 0 (A header 0), and spare bits are 0.
- **output collapse (B):** each B result is also output once as a single value, by collapse. Its value and its canonical form (one part zero) are checked. This exercises collapse into a slot. The spec now collapses only when a single value is needed, and this output step is that case.
- **bound:** no routine writes past its §1 bound. The zero-fill of the rest of the slot is the spec's own write and is exempt.
- **release:** after the operands and every result are released, every slot of every pool is on its free list exactly once. Subtract's internal negation is released by the subtract (N1).
- **error:** no operation raises an error the spec does not call for: an overflow past a slot or the one-word local, a class above 31, a full pool, or a Python fault.

The sets:
- **signed8:** every pair of signed values $|v| < 2^8$ (261,121 pairs). For negate, every value (511). For division, every pair with $b \ne 0$ and $b \mid a$ (6,338). B takes the collapsed form.
- **unc4 (B only):** every pair of uncollapsed $(P, N)$ with both parts below $2^4$ (65,536 pairs; negate 256; division 19,712 pairs whose value divides).
- **random:** 100,000 cases per operation and design. Values have up to 8 words, with zeros and equal magnitudes each forced at 1/16. B's parts are drawn on their own, and $P = N$ is forced at 1/16. Division builds $a = bq$. Gcd gives half its pairs a common factor.

The gcd(0, 0) error cases are 1 in each signed8 set, 256 in unc4, 535 in A random and 613 in B random. All raised the error and left every slot free.

### Results per design, set and operation

| design | set | add | sub | neg | mul | div | gcd | cmp |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| A | signed8 | 261121 pass | 261121 pass | 511 pass | 261121 pass | 6338 pass | 261121 pass | 261121 pass |
| A | random | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass |
| B | signed8 | 261121 pass | 261121 pass | 511 pass | 261121 pass | 6338 pass | 261121 pass | 261121 pass |
| B | unc4 | 65536 pass | 65536 pass | 256 pass | 65536 pass | 19712 pass | 65536 pass | 65536 pass |
| B | random | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass | 100000 pass |

"Pass" covers every check above with zero failures: value, canonical, spare bits, output collapse (B), bound, release and error. The most slots a pool held at once was 4 in A and 8 in B.

**Pool limit.** For each design, 15 class-1 values were made. The 16th allocation raised the error, and after release all 15 slots were on the free list. Pass for both.

**The checks still catch faults.** I reran 3,000 random cases per operation with one revision undone. Without the zero-fill, 11 of the 14 operation and design pairs failed. Only both compares and A's negate passed, since they write no new slot words. B's negate failed through its output collapse. Without negate's copy, A's add, subtract and negate and B's subtract and negate failed the release check. So the passes above test the new rules, not an idle harness.

### Four directions (value only, against Gaussian integers)

A: every pair with magnitude below $2^5$ in all four directions, plus 100,000 random pairs with up to 128-bit magnitudes. B: every pair of four-slot values with each slot below 4, plus 100,000 random pairs.

| check | cases | failures |
| --- | --- | --- |
| A multiply | 115625 | 0 |
| A divide ($a = bq$) | 108561 | 0 |
| A negate | 115625 | 0 |
| A add (within an axis, or a zero operand) | 64344 | 0 |
| A subtract (same) | 64344 | 0 |
| A add, error raised exactly for nonzero sums across axes | 51281 | 0 |
| A subtract, same | 51281 | 0 |
| B add (across axes included) | 165536 | 0 |
| B subtract | 165536 | 0 |
| B negate | 165536 | 0 |
| B multiply (slot $(d_1 + d_2) \bmod 4$) | 165536 | 0 |
| B collapse, per axis (operands and every result) | 662144 | 0 |

## Spec issues

1. **N1. Subtract's negation is now a copy, which is a temporary that rule 4 does not list.** Subtract "adds the negation" and negate copies (§1 Operations), so a subtract allocates the copy (one slot in A, up to two in B), runs the add, and must release the copy itself. The model does this, and the release check passes. But §3 Structures lists the only temporary as "a slot on a move-down". With the copy, an A subtract can hold two temporaries (the copy and a move-down), and a B subtract up to four. It also costs a copy of the operand's slot on every subtract. A simpler rule needs no copy: A applies the add rules with `h2 xor 0x80` in place of `h2`, and B adds crossed parts, $(P_1 + N_2, N_1 + P_2)$. This is for the author. It is not a correctness failure.
2. **N2. "Returns the other" must mean a copy.** In A, an add or subtract with a zero operand "returns the other". Returned as the same value, the result would share the operand's slot, and releasing both would release the slot twice: the first run's F2 again. The mutation test confirms this. The model copies, following §1's "no two numbers share a slot". The spec could say "a copy of the other".
3. **Reading, not an issue:** the output-collapse check runs collapse once on each B result, as the "single value needed" case. The spec now limits collapse to that need, and the check counts as exactly that.

Builder's choices (not spec issues):
- operands are built by test setup, which zero-fills their slots;
- Growing's round-2 one-word gcd special case is omitted (rule 2);
- B's compare uses one local in two halves;
- division and gcd each use one working buffer, which B's collapse writes straight into;
- half of the random gcd pairs share a factor.
