# Spec 13, round 1 mathematics, proved in Lean 4

This folder proves the mathematics of spec 13's two designs for **all values**. Round 1's Python models (`../test/model_a.py`, `model_b.py`, `axes.py`) check the same rules only on samples. The sources are `../../../book/notes-designs-a-b.md` and `../v0.1.md` §2–§4.

Part two adds the word-level routines of spec §1 that round 1 (§4) runs: multi-word add with carry, subtract with borrow, and absolute difference. They are proved for any word size and any operand lengths, write bounds included (`Header/Words.lean`).

The rest of the bookkeeping is **not** covered: headers and their 8-bit layout, size classes, pools and slots, multiply-accumulate, division, gcd, and `norm` (the returned length). The Python tests keep checking those.

## How to check

```sh
cd spec/13-header/lean
lake build        # builds Header and checks every proof
```

- Toolchain: **Lean 4.35.0** (`leanprover/lean4:v4.35.0`, pinned in `lean-toolchain`).
- Dependencies: core Lean only. There is no Mathlib.
- A clean build (`rm -rf .lake && lake build`) takes about **6 s** wall time and finishes with no errors and no warnings.
- No file contains `sorry`, `admit`, `native_decide` or a new `axiom`.
- `#print axioms` was run on all 101 theorems. Each depends only on Lean's standard axioms (`propext`, `Quot.sound`, `Classical.choice`, or a subset of them).
- Tactics used: `decide` (finite checks over `Fin 4`), `omega` (linear arithmetic over Nat and Int), `grind` (ring identities) and `simp`.

**Installing the toolchain.** elan downloads from `release.lean-lang.org`, which this workspace's network refuses. The toolchain was therefore taken from the GitHub release `lean-4.35.0-linux.zip` and unpacked into `~/.elan/toolchains/leanprover--lean4---v4.35.0`. elan then finds it by its usual name. On a normal network, `elan` installs it by itself.

## Modelling choices

| Notion | In Lean |
| --- | --- |
| Gaussian integer | `GI`, a pair of `Int`s `(re, im)` with `+`, `-`, `*` and `smul` (an integer times a GI) |
| direction `d` | `Fin 4`. Its arithmetic is mod 4, so `d1 + d2` and `d1 - d2` are the notes' mod-4 sums. |
| `i^d` | `unit : Fin 4 → GI`, the notes' table: 1, i, −1, −i |
| a word | the bound `N` (= 2^n): magnitudes `< N` |
| clock difference | `clockSub N m1 m2 = if m2 ≤ m1 then m1 - m2 else m1 + N - m2`, with no `%` |
| borrow | `borrow m1 m2 = (m1 < m2)` |
| clock negation | `clockNeg N r = if r = 0 then 0 else N - r` |
| clock addition | `clockAdd N m1 m2 = if m1 + m2 < N then m1 + m2 else m1 + m2 - N` |
| absolute-difference routine | `absDiffClock N m1 m2`: the clock difference, negated on the clock when `b = 1` |
| `\|m1 − m2\|` | `absDiff`, the larger minus the smaller |
| direction check | `dirCheck d1 d2 = d1.val XOR d2.val`, on the two direction bits |
| reference order | `ord a b`: `lt`, `eq` or `gt` on `Int`, Python's `(a > b) - (a < b)` |

The Design A operations (`A.add`, `A.neg`, `A.sub`, `A.mul`, `A.div`, `A.cmp`) are written as the notes' tree and spec §2 state them:

- `add` returns `none` for "an expression", that is, two nonzero numbers on different axes.
- `cmp` returns `none` for "not ordered".

The Design B operations (`B.add`, `B.sub`, `B.neg`, `B.mul`, `B.cmp`, `B.collapse`) and the four-slot operations (`B4.*`) follow notes §Design B and spec §3–§4. Design B's collapse appears twice:

- in the notes' form, `if N ≤ P then (P − N, 0) else (0, N − P)`;
- in the spec's form, `collapseClock`: the absolute-difference routine on the clock, routed by its borrow.

The two forms are proved equal.

## Theorems

Names are as in the files. "For all" means for every value, with no sampling.

### Basics (`Header/Basic.lean`, `Header/Clock.lean`)

| Theorem | Statement |
| --- | --- |
| `unit_add` | `i^(d1+d2) = i^d1 · i^d2`, with `d1 + d2` mod 4 |
| `unit_add_two` | `i^(d+2) = −i^d`: a half turn is negation |
| `unit_sub_mul` | `i^(d1−d2) · i^d2 = i^d1` |
| `GI.smul_mul_smul` | `(a·u)(b·v) = (ab)·(uv)` |
| `clockSub_lt` | For `m1, m2 < N`, the clock difference stays below `N`. |
| `clockSub_int` | The clock difference equals `m1 − m2 + N·b` as integers. So it is `m1 − m2` mod N, and the borrow records exactly the wrap. |
| `clockNeg_spec` | For `r < N`: clock negation stays below `N`, gives 0 for 0, and `r + clockNeg r = N` (≡ 0) for `r > 0`. |
| `clockAdd_of_lt` | With no overflow, clock addition is the true sum. |
| **`borrow_lemma`** (item 1) | For `m1, m2 < N`, the clock difference, negated on the clock when `b = 1`, is exactly `\|m1 − m2\|`. |
| `absDiffClock_lt` | That result stays below `N`. |
| `absDiff_sign` | `m1 − m2 = (−1)^b · \|m1 − m2\|` as integers. |
| `unit_borrow` | `i^(d + 2b) = (−1)^b · i^d` |

### Design A (`Header/DesignA.lean`, namespace `Header.A`)

A number is `Num = (m : Nat, d : Fin 4)` and its value is `val (m, d) = m · i^d`.

**Item 1, the borrow lemma.**

| Theorem | Statement |
| --- | --- |
| `borrow_lemma_A` | For `m1, m2 < N`, the routine gives `\|m1 − m2\|`, and `(that, d1 + 2b)` has value `m1·i^d1 + m2·i^(d1+2)`. So `d1 + 2b` is the right direction. |

**The direction check.**

| Theorem | Statement |
| --- | --- |
| `dirCheck_same` | XOR `00` holds exactly when the directions are the same. |
| `dirCheck_opp` | XOR `10` holds exactly when the directions are opposite (`d2 = d1 + 2`). |
| `dirCheck_diff` | A low bit of 1 holds exactly when the directions are on different axes. |
| `dir_trichotomy` | The three cases are exhaustive and exclusive. |

**Item 2, addition.**

| Theorem | Statement |
| --- | --- |
| `val_zero`, `val_same`, `val_opp` | The value identities behind each branch. |
| `add_of_same`, `add_of_opp`, `add_of_diff` | The branch that the rule takes in each case. |
| **`add_value_opp`** | Opposite directions with `m1, m2 < N`: the rule gives a pair whose value is the true sum and whose magnitude is `< N`. No overflow hypothesis is needed, since none can occur. |
| **`add_value`** | Same direction, opposite directions, or different axes with at least one zero magnitude: if `m1 + m2 < N`, the rule gives a pair whose value is the sum of the values and whose magnitude is `< N`. Overflow is out of scope and appears only as this hypothesis. |
| `add_eq_none_iff` | The rule reports "an expression" exactly when the directions are on different axes and both magnitudes are nonzero. |
| **`add_cross_not_unit`** | In that case the sum is not `k · i^e` for any integer `k` and direction `e`. |
| **`add_cross_no_pair`** | So no pair has that value. |

**Item 3, negation and subtraction.**

| Theorem | Statement |
| --- | --- |
| **`val_neg`** | `value(neg x) = −value(x)` |
| **`sub_value`** | Under the same hypotheses as `add_value`, subtraction (addition of the negation) gives a pair with value `value(x) − value(y)`. |
| `sub_value_same` | Subtracting a number in the same direction (the opposite branch after negation) needs no overflow hypothesis. |
| `sub_eq_none_iff` | Subtraction reports "an expression" exactly when the directions are on different axes and both magnitudes are nonzero. |
| `diffAxes_add_two` | Turning a direction by 2 keeps its axis. |

**Item 4, multiplication.**

| Theorem | Statement |
| --- | --- |
| **`val_mul`** | `value(m1·m2, d1+d2) = value(x) · value(y)`, for Nat magnitudes with no word bound. |

**Item 5, exact division.**

| Theorem | Statement |
| --- | --- |
| **`val_div_mul`** | If `m2 > 0` and `m2 ∣ m1`, then `value(m1/m2, d1−d2) · value(y) = value(x)`. |
| `div_unique` | Any `q` with `value(q) · value(y) = value(x)` has the quotient's value. |

**Item 6, zero.**

| Theorem | Statement |
| --- | --- |
| **`zeros_equal`** | Any two pairs with magnitude 0 have equal value. |
| **`val_eq_iff`** | `value(x) = value(y)` holds exactly when `m1 = m2` and (`d1 = d2` or `m1 = 0`). So every nonzero number has exactly one pair. |

**Item 7, comparison.** `cmp` is spec §2's rule:

1. Two zeros are equal.
2. If one is zero, the other's sign decides; 1 and i are positive.
3. Opposite directions: the positive one is larger.
4. Same direction: compare the magnitudes, reversed when negative.
5. Different axes: not ordered.

`onAxis a x` means that `x` is zero or that its direction lies on axis `a`. `coord` and `off` give a value's coordinate along an axis and off it.

| Theorem | Statement |
| --- | --- |
| `onAxis_iff` | `onAxis a x` holds exactly when `value(x)` has coordinate 0 off axis `a`, so it is a statement about the value. |
| **`cmp_correct`** | If both numbers lie on axis `a`, which always holds when a zero is involved, then `cmp` returns exactly the true order of their coordinates along `a` (the real part, or the imaginary part). |
| `cmp_eq_none_iff` | `cmp` refuses exactly when both magnitudes are nonzero and the directions are on different axes. |
| `cmp_eq_none_iff_no_common_axis` | That is exactly when no axis holds both values. |
| **`cross_ne_zero`** | Two such numbers have `re1·im2 − im1·re2 ≠ 0`. |
| **`no_common_line`** | Hence there is no Gaussian integer `w` and no integers `s, t` with `value(x) = s·w` and `value(y) = t·w`. The two values share no line through zero, so there is no real line on which to order them. |

### Design B, integers (`Header/DesignB.lean`, namespace `Header.B`)

A number is `Num = (P, N)` and its value is `val = P − N` in `Int`.

**Item 8.**

| Theorem | Statement |
| --- | --- |
| **`val_add`** | Part-wise addition gives the sum. |
| **`val_sub`** | The crossed parts `(P1 + N2, N1 + P2)` give the difference. |
| **`val_neg`** | Swapping the parts gives the negation. |
| **`val_mul`** | `(P1P2 + N1N2, P1N2 + N1P2)` gives the product. |
| **`cmp_correct`** | Comparing `P1 + N2` with `P2 + N1` gives the true order of the values. |
| **`val_eq_iff`** | Two pairs have equal value exactly when `P1 + N2 = P2 + N1`. |
| **`val_collapse`** | Collapse keeps the value. |
| **`collapse_canonical`** | Collapse leaves one part zero. |
| **`canonical_unique`** | Two canonical pairs with the same value are equal, so collapse gives *the* canonical pair. |
| `collapse_eq_of_val` | Collapse depends only on the value. |
| `collapse_of_canonical` | Collapse fixes canonical pairs. |
| `collapse_zero` | Zero's canonical pair is `(0, 0)`. |
| **`collapseClock_eq`** | Spec §3's collapse (`\|P − N\|` by the clock routine, to `P` when `b = 0`, else to `N`) equals the notes' collapse when both parts are `< W`. |

### Design B, four slots (`Header/FourSlot.lean`, namespace `Header.B4`)

A number is `Num = Fin 4 → Nat`, slots along 1, i, −1, −i. Its value is `val = Σ m_k · i^k`, so `val_re = m0 − m2` and `val_im = m1 − m3`.

**Item 9.**

| Theorem | Statement |
| --- | --- |
| **`val_add`** | Slot-wise addition gives the sum, across axes included. |
| **`val_neg`** | Rotating the slots by 2 gives the negation. |
| **`val_sub`** | Adding slot `k` of `y` into slot `k+2` gives the difference. |
| **`val_mul`** | Slot `k` collects `x_j · y_l` for every `j + l ≡ k (mod 4)`, sixteen products in four slots, and the result is the Gaussian-integer product. |
| `mul_slot` | The product of slot `j` and slot `l` lands in slot `j + l` mod 4 and nowhere else. |
| **`val_collapse`** | Per-axis collapse (slots 0/2 and slots 1/3, each as a Design B integer) keeps the value. |
| **`collapse_canonical`** | Per-axis collapse leaves one part zero on each axis. |
| `canonical_unique` | The collapsed form is unique. |
| **`collapseClock_eq`** | Spec §4's per-axis collapse by the borrow equals it when all slots are `< W`. |

### Word-level routines (`Header/Words.lean`, namespace `Header.Words`)

**The model.** A magnitude is a list of words, low word first, for **any word size `n > 0` and any length**. `Words n l` says every word is `< 2^n`. The value is `val n [w0, w1, …] = w0 + 2^n·w1 + …`.

The routines are `words.py`'s `r_add`, `r_sub_into` and `r_absdiff`, transcribed loop by loop:
- `& M` is `wrap n a = a &&& (2^n − 1)`, which `wrap_eq_mod` proves is `a % 2^n`.
- Python's `(a − b) & M` is `wsub n a b`.
- Every carry and borrow is a value, `int(s < c)` (`lt s c`). There is no flag.

**Unequal lengths** are covered as the code handles them:
- `r_add` swaps the operands so that `x` is the longer, then carries through `x`'s tail (`addTail`).
- `r_sub_into` has two tails. When `x` is the longer, the borrow propagates through `x`'s tail (`subTailX`). When `y` is the longer, `x` is read as zero past its end (`subTailY`, which computes `d = (0 − q) & M` and `br = int(q != 0) + int(d < br)`).

**Writes.** Each loop writes word `i` at step `i`, in order, starting where the previous loop stopped. So a routine is modelled by the list of words it writes: the list's length is the number of words written, and the highest index written is the length minus 1. "No write past the bound" means "the list is no longer than the bound".

**Cross-check.** The Lean definitions were run with `#eval` on 300 random operand pairs (`n = 4`, lengths 0–5, with zero and all-ones words weighted). Their results matched `words.py`'s `r_add` and `r_absdiff` word for word, borrow included.

**Building blocks.**

| Theorem | Statement |
| --- | --- |
| `carry_lemma`, `carry_lemma'` | For words `a, b`, the wrapped sum `t` and the carry `int(t < b)` (or `int(t < a)`) satisfy `a + b = t + 2^n·carry`. In words: the sum wrapped exactly when it came out below an operand. |
| `borrow_word` | For a word `a` and `b ≤ 2^n`, the wrapped difference `d` and the borrow `int(a < b)` satisfy `a + 2^n·borrow = d + b`. |
| `xor_mask` | XOR with the all-ones word is `2^n − 1 − r`. |
| `val_lt` | A `k`-word list has value below `2^(n·k)`: it lies on the `k`-word clock. |
| `val_append` | `val (a ++ b) = val a + 2^(n·len a)·val b` |
| `addTail_spec`, `addCommon_spec` | Loop invariants for add: the carry stays 0 or 1, and the words written plus the carry times `2^(n·len)` equal the operands plus the incoming carry. |
| `subTailX_spec`, `subTailY_spec`, `subCommon_spec` | The same for subtraction: the borrow stays 0 or 1, the subtraction identity holds, and the common loop writes `max` words. |
| `negPass_length`, `negPass_words`, `negPass_zero`, `negPass_ones` | The second pass of `r_absdiff` writes as many words as it reads, all of them words. With borrow 0 it changes nothing. With borrow 1 it adds 1 to the one's complement. |

**Item 1, multi-word add with carry.**

| Theorem | Statement |
| --- | --- |
| **`add_spec`** | Let `k = max(len x, len y)`. `r_add` writes exactly `k + 1` words, which is the bound `max + 1` and never more. The top word, at index `k`, is the final carry, and it is 0 or 1. Every word written is `< 2^n`. The first `k` words plus the final carry times `2^(n·k)` equal `val x + val y`. |
| `val_add` | Read as one number, the `k + 1` words are the sum. |

**Item 2, absolute difference with borrow.**

| Theorem | Statement |
| --- | --- |
| **`subInto_spec`** | `r_sub_into` writes exactly `k` words. On the `k`-word clock `N = 2^(n·k)`, their value is `Clock.lean`'s `clockSub N (val x) (val y)`. The final borrow is `1` exactly when `val x < val y`, so it is `Clock.lean`'s `borrow (val x) (val y)`. |
| `negPass_clock` | On the value, the second pass is `Clock.lean`'s `clockNeg N` when the borrow is 1, and the identity when it is 0. |
| **`absDiff_spec`** | Both passes of `r_absdiff` write exactly `k` words, the bound `max`, never more, and all of them are words. The result is `absDiffClock N (val x) (val y)`, which by the value-level `borrow_lemma` is `\|val x − val y\|`. The returned borrow is `1` exactly when `val x < val y`, and it is 0 or 1. |

Together these tie the word-level routine to the value-level borrow lemma: the first pass is the clock difference, its borrow is the value-level borrow, and the second pass is the clock negation.

**Item 3, write bounds.**
- Add: `add_spec` shows exactly `max + 1` words are written, with the final carry as the top word.
- Absolute difference: `subInto_spec` and `absDiff_spec` show exactly `max` words in each pass.

Spec §1 says a routine "writes at most its bound". These routines meet their bounds exactly.

## Findings

**No statement in the notes or in spec §2–§4 turned out to be false.** Every rule listed above holds for all values under the stated hypotheses, and no counterexample was found.

Five observations do not contradict the documents:

1. **The borrow lemma needs less than it states.** It uses `m2 < N` only. The hypothesis `m1 < N` is never needed, because the no-borrow branch never touches the clock. The theorem keeps both hypotheses, as the spec does.
2. **Design A's order holds per axis, not across axes.** `cmp` says −1 < 0 and 0 < i, but it refuses to compare −1 with i. This matches spec §2 ("different axes are not ordered") and the notes. It does mean that `cmp` is not a total order on all numbers: on mixed axes, transitivity through zero does not apply. `cmp_correct` states the precise guarantee, which is agreement with the true order along any axis that holds both numbers.
3. **The zero test in comparison is needed, as the notes say.** Without it, `(0, 0)` and `(0, 2)` would fall into the opposite-direction branch and come out unequal. In addition, `add_value` confirms the notes' claim that no zero test is needed on the same-direction and opposite branches: those branches handle a zero in any direction correctly.
4. **The four-slot multiplication is written as a sum.** It is defined as slot `k = Σ_j x_j · y_(k−j)`, which is the notes' rule "the product of `d1` and `d2` lands in `d1 + d2`" (`mul_slot` proves the two readings agree).

5. **Word routines, part two.** No counterexample. Three facts the routines rely on are now proved for every word size and length:
   - the carry and the borrow never exceed 1;
   - `r_add`'s top word is the carry, written even when it is 0, so the result is always `max + 1` words before `norm`;
   - the shorter operand is handled as described above (swap for add; read as zero for subtract).

   Two limits of what is proved:
   - The word size must be positive (`n > 0`); with `n = 0` there are no words.
   - `norm`, which trims the top zero words to return the length, is not modelled.

## Files

- `lean-toolchain`: the pinned Lean version.
- `lakefile.toml`: one library, `Header`, which `lake build` builds and checks.
- `Header.lean`: imports all the modules.
- `Header/Basic.lean`: Gaussian integers, `unit`, `ord`.
- `Header/Clock.lean`: the clock model and the borrow lemma.
- `Header/DesignA.lean`: items 1–7.
- `Header/DesignB.lean`: item 8.
- `Header/FourSlot.lean`: item 9.
- `Header/Words.lean`: part two, the word-level routines.
- `.gitignore`: excludes the build directory `.lake/`.
