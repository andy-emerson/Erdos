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

### Design C, on 64-bit words (`Header/DesignC.lean`, namespace `Header.C`)

Spec v0.2 §4a and §5. A number is `Num = (v : Int, a : Bool)`, with value `val = v · i^a` (`GI.smul v (unit a)`, the same Gaussian integers and units as A). `InRange v` is `-2^62 ≤ v < 2^62`.

**The machine.** A word is `BitVec 64`, Lean core's 64-bit register: `+`, `-`, `*` and negation wrap mod `2^64`; `&&&`, `|||`, `^^^`, `~~~` are AND, OR, XOR, NOT; `<<<` is shift left; `sshiftRight` is `sar`; `sdiv` is the signed divide, truncating (it wraps on `-2^63 / -1`); `<` is the unsigned compare. `enc (v, a) = ofInt (2v + a)`; `dec w = ((w sar 1) read signed, w and 1 = 1)`; `wval w = val (dec w)`. `add`, `sub`, `neg`, `mul` and `div` are §4a's rules, transcribed instruction by instruction; `none` is the error (an expression).

**Method.** Every word is `ofInt (2v + a)` of its decoding (`word_eq`), and `w and 1`, `w and not 1` and `w sar 1` are `ofInt a`, `ofInt 2v` and `ofInt v` (`lo_eq`, `clr_eq`, `sar_eq`). Each rule's word is then `ofInt (2v' + a')` of the true result, mod `2^64` (`add_of_same`, `neg_eq`, `mul_eq`, `div_eq`, with no hypothesis), and `dec_ofInt` reads it back exactly when `v'` is in range. So only the result needs to be in range; an intermediate may wrap.

**Item C1, encoding and zero.**

| Theorem | Statement |
| --- | --- |
| **`dec_enc`** | For `v` in range and either axis, `dec (enc (v, a)) = (v, a)`. |
| **`enc_dec`** | Every word is the encoding of its decoding. |
| `dec_range`, `enc_inj`, `dec_inj` | Every word decodes into range; so words and in-range pairs correspond one to one. |
| `toInt_split` | Every word, read signed, is `2v + a` of its decoding. |
| **`wval_eq_zero_iff`** | A word's value is zero exactly when the word is 0 or 1. |
| `lt_two_iff` | The unsigned `w < 2` holds exactly when `v = 0`. |
| `val_eq_iff` | Two pairs have equal value exactly when `v1 = v2` and (`a1 = a2` or `v1 = 0`): every nonzero number has one word, and zero two. |

**Item C2, add and subtract.**

| Theorem | Statement |
| --- | --- |
| `axis_check` | `t = (w1 xor w2) and 1` is 0 exactly when the axes are equal. |
| **`add_same`**, **`sub_same`** | Same axis, `v1 ± v2` in range: the word decodes to `(v1 ± v2, a1)`. |
| `add_cross_zero_right`, `add_cross_zero_left`, `sub_cross_zero_right`, `sub_cross_zero_left` | Across axes with a zero, the result is `w1`, `w2`, `w1` or `neg w2`. |
| **`add_value`**, **`sub_value`** | When the axes agree or a value is zero, and the true result is in range, the word's value is the true sum (difference). |
| **`add_eq_none_iff`**, **`sub_eq_none_iff`** | The error holds exactly when the axes differ and both values are nonzero. |
| **`cross_no_num`**, `add_none_no_num`, `sub_none_no_num` | Then no pair `(v, a)`, in range or not, has the sum's (difference's) value. |
| `add_overflow_example` | `(2^62 − 1) + 1` on the real axis gives `-2^62`: the hypothesis is needed. |

**Item C3, negation.**

| Theorem | Statement |
| --- | --- |
| `neg_eq` | The rule's word is `2(−v) + a` mod `2^64`, always. |
| **`dec_neg`**, **`neg_value`** | For `v > −2^62`: `(−v, a)`, the true negation on the same axis (a zero stays on its axis). |
| **`neg_edge`**, `neg_edge_example` | For `v = −2^62` (whose negation `2^62` is out of range) the rule returns the word unchanged. |

**Item C4, multiplication.** `mulNum (v1, a1) (v2, a2) = ((−1)^(a1·a2) v1·v2, a1 xor a2)`.

| Theorem | Statement |
| --- | --- |
| **`val_mulNum`** | Its value is the product, on all four axis pairs: `i · i = −1`. |
| `mul_eq` | The rule's word is `2v + a` of `mulNum`, mod `2^64`, always. |
| **`dec_mul`**, **`mul_value`** | With the true product in range, the word decodes to `mulNum` and its value is the product. |
| `mul_wrap_example` | `(2^31 i)(2^31 i) = −2^62`: the intermediate `v1 · 2v2 = 2^63` wraps, and the result is still right. |

**Item C5, exact division.** `divNum (v1, a1) (v2, a2) = ((−1)^((1 − a1)·a2) v1/v2, a1 xor a2)`. `div` is the current rule of §4a: `q = (w1 and not 1) / (w2 and not 1)` (`2v1 / 2v2`); `q' = (q xor m) − m` with the mask `m = 0 − (((w1 xor w2) and 1) and w2)`, which negates when `a2 = 1` and `a1 = 0`; then `(q' shl 1) + ((w1 xor w2) and 1)`. `divShl` is the second rule (double, then negate by a branch, then OR the axis), and `divOld` the first, with divisor `w2 sar 1`. `idivTraps x y` is x86-64's `idiv` overflow trap, `x = −2^63 ∧ y = −1`; `idivFaults` adds a zero divisor.

| Theorem | Statement |
| --- | --- |
| **`val_divNum_mul`**, `div_unique` | When `v2 ≠ 0` divides `v1`, `divNum` times the divisor is the dividend, and is the only such value. |
| **`div_eq_divShl`**, `div_mask`, `xor_ones_sub`, `shl_or_axis` | The current rule gives the second rule's word on **every** input: the mask is all ones exactly when `a2 = 1` and `a1 = 0`, `(q xor −1) − (−1) = −q`, negating before or after the doubling is the same, and adding the axis bit to an even word is OR-ing it. So every theorem below holds for both. |
| `sdiv_exact`, `shl_one`, `div_eq` | `2v1 / 2v2` is `v1/v2` exactly, `shl 1` doubles mod `2^64`, and the rule's word is `2v + a` of `divNum`, mod `2^64`. |
| **`dec_div`**, **`div_value`** | With `v2 ≠ 0`, `v2 ∣ v1` and the true quotient in range, the word decodes to `divNum`. |
| **`div_never_traps`** | For **any** two words, the new rule's divide is not `−2^63 / −1`: its divisor `2v2` is even. No range hypothesis. |
| **`div_never_faults`**, `clr_eq_zero_iff` | With `v2 ≠ 0` it does not fault at all (its divisor is 0 exactly when `v2 = 0`). |
| **`div_edge_steps`**, `divShl_edge_steps`, `div_edge_example`, `div_edge_in_range` | `(−2^62)/(−i) = −2^62 i`, step by step: `−2^63 / −2 = 2^62`; the mask is all ones, so `q' = −2^62`; `q' shl 1 = −2^63` with no wrap; `+ 1` gives the word of `−2^62 i`. Under the second rule the doubling wrapped (`2^62 → −2^63`) and the negation left it there, with the same word. |
| **`div_eq_divOld`**, `div_agrees_where_old_runs` | On every exact division the new rule's word equals the old rule's (in the model, where the old divide wraps), so in particular wherever the old one does not trap. |
| `div_inexact_differ` | Off exact division they differ (`1 / 2`: old gives the word 1, new the word 0); spec §1 excludes such operands. |
| **`divOld_traps_iff`**, **`div_trap_iff`**, `divOld_edge_example`, `divOld_overflow_example` | The old rule traps exactly when `v1 = −2^62` and `v2 = −1`; among in-range quotients, exactly for `v1` real and `v2` imaginary, `(−2^62)/(−i)`. Finding C-a, kept as a record. |

**Item C7, Control for reference** (the machine's signed integers on the same words; `InRange64 z` is `−2^63 ≤ z < 2^63`).

| Theorem | Statement |
| --- | --- |
| `ctl_neg`, `ctl_neg_intMin` | Signed negate is exact for every `x ≠ −2^63`; `−(−2^63) = −2^63`. |
| **`ctl_neg_out_iff`** | Exactly one input has no negation in range: `−2^63`. |
| **`ctl_div_out_iff`** | For `y ≠ 0`, the true truncated quotient is out of range exactly on `idivTraps x y`. |
| `ctl_trap_quotient` | That quotient is `2^63`: in Control the trap is genuine overflow (unlike C's old rule). |
| `ctl_sdiv` | Off the trap and a zero divisor, the machine's divide is the true truncated quotient. |

**Item C6, agreement with A.** `toA (v, a) = (|v|, d)` with `d = 2s + a`, `s = 1` for `v < 0`; `ofA (m, d) = (±m, d mod 2)`, minus for `d = 2, 3`.

| Theorem | Statement |
| --- | --- |
| `toA_d`, `toA_dec_d` | `d = 2s + a`; on a word, `s` is bit 63 and `a` bit 0. |
| **`val_toA`**, **`val_ofA`** | Both maps keep the value. |
| `ofA_toA`, `toA_ofA` | They are inverse (`toA ∘ ofA` on A's nonzero pairs: A has four zeros, C two). |
| **`same_numbers_C_to_A`**, **`same_numbers_A_to_C`** | Every C word is an A number with `m ≤ 2^62`; every A number with `m < 2^62` is a C word. C holds A's numbers on the axes over a quarter of A's range, plus `−2^62` and `−2^62 i`. |

**Design C findings.** No rule in §4a or the notes is false under the no-overflow hypothesis.

- **C-a. One in-range division trapped on x86-64 (fixed).** Under the first rule, `(−2^62) / (−i) = −2^62 i` is in range and comes out right in the model, but its intermediate `(w1 and not 1) / (w2 sar 1)` is `−2^63 / −1`, on which x86-64's `idiv` raises a divide error (ARM64 `sdiv` and RISC-V `div` return `−2^63`). `div_trap_iff` proves it was the only in-range case. Spec §4a now divides by `w2 and not 1` and shifts left after. `div_never_traps` proves the new divide never traps for any words, and `div_eq_divOld` proves it gives the same words on every exact division. The current order (negate, then double and add the axis) gives the same word as doubling first on every input (`div_eq_divShl`); in that case it does not even wrap (`div_edge_steps`). In Control the same trap is genuine overflow (`ctl_div_out_iff`).
- **C-b. Negation's edge case.** `−(−2^62)` is out of range, and the rule returns the operand unchanged (`neg_edge`), like the machine's own `−(−2^63)`. It also reaches subtraction across axes with `v1 = 0`, `v2 = −2^62`. This is overflow, which the spec puts out of scope, but the notes' "negation negates the value" holds only for `v > −2^62`.
- **C-c. Zero has two words**, 0 and 1. The notes' table says C has "exactly one" form of a number; that holds for nonzero numbers, the same caveat as A's four zeros.
- **C-d. Only the result must be in range.** Multiply and divide may wrap in their intermediates (`mul_wrap_example`, `divShl_edge_steps`) and still give the right word, since the result is fixed mod `2^64`.

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
- `Header/DesignC.lean`: Design C on 64-bit words, items C1–C7.
- `Header/FourSlot.lean`: item 9.
- `Header/Words.lean`: part two, the word-level routines.
- `.gitignore`: excludes the build directory `.lake/`.


## Round 2 (spec v0.2 §6): programs in a portable instruction set

Results and tables: [`../results-r2.md`](../results-r2.md). Files:

- `Header/Isa.lean`: the instruction set (36 instructions: add, sub, neg, and, or, xor and their constant forms, shifts, `sltu`, `sltiu`, `li`, `mv`, `mul`, `mulhu`, `divu`, `div`, `ld`, `lbu`, `sd`, `sb`, six compare-and-branches, `j`, `trap`), with no flag, no conditional move and no 128/64 divide; an interpreter over registers and memory that records the trace and the stores; the word lemmas (`wadd_clock`, `wsub_clock`, `wneg_clock`: the machine's add, subtract and negate are round 1's clock operations; `wmul_ofInt`, `wdiv_ofInt`: the signed reading); the `run_prog` tactic, which runs a program symbolically with `simp`.
- `Header/Control.lean`: Control's programs and their proofs.
- `Header/ProgA.lean`, `Header/CorrectA.lean` (with `Header/CA_cxAdd_*.lean`, the complex add and subtract split into four files, one per form and operation, so that each fits in memory; and with `Header/Split.lean`, `Header/BF_*.lean` and `Header/BfCx.lean`, the branch-free complex row in 160 case lemmas): Design A (integer and complex add and subtract, both forms of the opposite-direction path; negate, multiply, divide; the branch-free row) and their proofs against `A.add`, `A.neg`, `A.mul`, `A.div`.
- `Header/ProgB.lean`, `Header/CorrectB.lean`: Design B (integer and four-slot programs; collapse and division in both forms) and their proofs against `B.*`, `B4.*`, with `div_value` and `cdiv_value` for division.
- `Header/ProgC.lean`, `Header/CorrectC.lean`: Design C, with a bridge from the interpreter's words to round 1's `BitVec 64`, and proofs against `C.add`, `C.sub`, `C.neg`, `C.mul`, `C.div`.
- `Header/Registry.lean`: every program with its paths.
- `Header/Counts.lean`: the per-family cost rules; `table_eq` (counts, per-family instruction counts and branch log of every path, by evaluation); `all_forward`; `all_loads_first`; `bf_no_branch` (the branch-free row has no conditional branch); `straight_line`; `andD_redundant`.

| Theorem | Statement |
| --- | --- |
| `Ctl.add_run` … `Ctl.cdiv_run` | Control's programs store `ofInt` of the true result; complex division by a nonzero number on an axis stores the exact quotient. |
| `PA.intAdd_run`, `PA.cxAdd_run` | For every input, A's add and subtract (both forms) follow the trace `intTrace`/`cxTrace` of `path d1 d2 m1 m2` and store round 1's `A.add W x y` (or `A.add W x (A.neg y)`); the complex program ends at `trap` exactly when that is an expression. |
| `PA.neg_run`, `PA.mul_run`, `PA.div_run` | One path; stores `A.neg`, `(d1 + d2, m1·m2 mod 2^64)`, `A.div`. |
| `PA.bfInt_run`, `PA.bfCx_run` | The branch-free row runs every instruction once and stores what the tree stores; register 21 is 1 exactly when the tree's add gives an expression. `bfCx_run` (`BfCx.lean`) assembles 160 case lemmas `bfCx_{add,sub}_d1_d2_cK` (`BF_*.lean`). |
| `Isa.run_split` (`Split.lean`) | If `stepN p k s = some t` (the first `k` instructions run without ending), running with fuel `n + k` from `s` is running with fuel `n` from `t`. The case lemmas cut each run there, which keeps the kernel's check small. |
| `PB.add_run` … `PB.cmul_run`, `PB.cneg_run` | B's straight-line programs store round 1's parts mod `2^64` (`add_value`, `sub_value`, `mul_value`, `cmul_slots` give round 1's results under no overflow). |
| `PB.collapse_run`, `PB.ccollapse_run` | Both forms store `B.collapse` (per axis), along the path of the borrows. |
| `PB.div_run`, `PB.div_value` | Integer division stores 0, 0, then the quotient in part `b1 xor b2`; the result is collapsed and times `y` gives `x`. |
| `PB.cdiv_run`, `PB.cdiv_value`, `PB.toA_val` | For operands on axes, complex division stores 0 to four slots and `A.div` of the two operands (read as A numbers by `toA`) in its slot; its value times `y` is `x`. |
| `PC.add_run` … `PC.div_run` | C's programs store `C.add`, `C.sub`, `C.neg`, `C.mul`, `C.div`; add and subtract end at `trap` exactly when round 1 gives an expression. `PC.mul_mask`: `C.mul` equals its mask form. |
| `Reg.table_eq` | The table of counts and branch logs in `results-r2.md` §4 is what the programs give. |
| `Reg.all_forward`, `Reg.all_loads_first`, `Reg.bf_no_branch`, `Reg.straight_line`, `Reg.andD_redundant` | As named. |

Coverage: every `_run` theorem holds for every input, and names the trace as a function of the inputs, so the listed paths cover every input.

**Build time.** Design A's proofs run the programs symbolically for all 16 direction pairs and every magnitude case and take most of the build. As one file (`CorrectA.lean`) they took about 16 minutes and, built beside `CorrectB`, ran out of memory on this 8 GB machine (Lean exited with code 137), so the complex part is split into the four `CA_cxAdd_*.lean` files, and B's complex division into eight `CB_cdiv_run_*.lean` files (form × axis of `x` × axis of `y`), combined in `CB_cdiv_notes.lean`, `CB_cdiv_perArm.lean` and `CorrectB2.lean` (`PB.cdiv_run`). Even split, a few files take 2–3 GB; if `lake build` is killed (code 137), build the `CB_cdiv_run_*` modules one at a time first. `#print axioms` on all 313 round 2 theorems: 278 use `propext`, `Quot.sound`, `Classical.choice` or a subset, 35 none.  Theorems with many memory hypotheses are stated with plain binders, not `variable … include`: with an `include` section the same symbolic runs stalled.

## A's header rules without `and D` (`Header/HeaderByte.lean`, from the coordinator)

The author's change after round 2's finding 2: A's header rules with no mask. `hb d = 64·d` is the header byte.

| Theorem | Statement |
| --- | --- |
| `hb_lt`, `hb_low`, `hb_inj` | A header byte is below 256, has bits 0–5 clear, and determines its direction. |
| `hb_xor`, `t_mask` | `h1 xor h2` is `64·(d1 xor d2)`, so the mask on `t` changes nothing. |
| `hb_borrow` | The borrow path's header `h1 + (b shl 7)`, stored as a byte, is the right direction. |
| `hb_neg`, `hb_mul`, `hb_div` | Negate (`h xor 0x80`), multiply (`h1 + h2`) and divide (`h1 − h2` in a 64-bit register), stored as a byte, give `hb` of round 1's direction. |
| `addByte_eq`, `subByte_eq`, `negByte_eq`, `mulByte_eq`, `divByte_eq` | A's operations on (header byte, magnitude) with no `and D` agree with round 1's `A.add`, `A.sub`, `A.neg`, `A.mul`, `A.div` under the encoding `enc`. |
| `result_canonical` | Every result header is again a canonical byte. |

## A's multiplication row, round 1 (`Header/MulRow.lean`)

`aAddMul` / `aSubMul` of `../src/designa.nim`: A's add with the notes' exponent rule as code, no branch, and no
`and D` on headers. `addMul h1 m1 h2 m2` is the row on bytes and 64-bit words, returning (header byte, magnitude,
err); `subMul` is the row with `h2 xor 0x80`.

| Theorem | Statement |
| --- | --- |
| `addMul_eq` | For every `x`, `y` with magnitudes below `2^64` (headers `hb d`): when `A.add (2^64) x y` gives `r`, the row gives `(hb r.d, r.m)` with `err = 0`; when it gives an expression, `err = 1`. |
| `subMul_eq` | The same for `subMul` against `A.sub`. |
| `sgn_0`, `sgn_1`, `mul_ones` | `1 − 2k` on the clock is 1 or `2^64 − 1`, and multiplying by `2^64 − 1` is negation mod `2^64`. |

`#print axioms`: `addMul_eq` and `subMul_eq` use `propext`, `Classical.choice`, `Quot.sound`; the helpers fewer or none.

## A's multiplication row, round 2 (`Header/ProgMul.lean`, `Header/MR_*.lean`, `Header/MulRun.lean`)

| Theorem | Statement |
| --- | --- |
| `PA.mulRow_run` | For every input, `mulRow sub` runs every instruction once; register 21 is 1 exactly when round 1's `A.add W x (yEff sub y)` gives an expression, else 0 and the stores are its header byte and magnitude. Assembled from 160 case lemmas `mulRow_{add,sub}_d1_d2_cK`, each split at instruction 20 by `Isa.run_split`. |
| `Reg.mulRow_no_branch` | The row has no conditional branch and no jump. |
| `Reg.table_eq` | Now includes the row's two lines (35/36 steps; x86-64 45/46, ARM64 37/38, RV64IM 35/36). |
| `PA.hdr_0` … `PA.hdr_3` | The header bytes as literals (used to keep the kernel's checks small). |

## The header form's add from the prior-art research, round 1 (`Header/LargerRow.lean`)

`../prior-art.md`, "Header form": headers loaded sign-extended (`sx`), `t = h1 xor h2`, `s = t sar 63`,
`r = m1 + (m2 xor s) − s`, `L = 0 − (m1 <u m2)`, `b = s and L`, magnitude `(r xor b) − b`, direction
`h1 xor (t and L)` stored as a byte, flag `(t shr 6) and (m1 ≠ 0) and (m2 ≠ 0)`; subtract flips the second header
register with `xor −128`. All arithmetic mod `2^64` (`largerAdd`, `largerSub`).

| Theorem | Statement |
| --- | --- |
| `larger_add_eq` | For magnitudes below `2^64` and all 16 direction pairs: when `A.add (2^64) x y` gives `r`, the formula gives `r.m`, flag 0, and header `hb r.d`, except for two zeros on different axes, where it gives `hb x.d` (round 1 gives `hb y.d`; both are zero); when `A.add` gives an expression, the flag is 1. |
| `larger_sub_eq` | The same for `largerSub` against `A.sub`. |
| `obs_larger` | Observation 1, on round 1's `A.add`: with no overflow, the result's direction is that of the operand with the larger magnitude (the first on a tie), or the result is zero. |
| `obs_signs` | Observation 2: with no overflow, the result's magnitude is `m1 + m2` when the sign bits (`d / 2`) agree and `\|m1 − m2\|` when they differ, whatever the axes. |
| `add_eq_absDiff_of_zero` | With a zero operand, `m1 + m2 = \|m1 − m2\|`. |
| `sx_flip` | `xor −128` on a sign-extended header is the sign-extended `h xor 0x80`. |

`#print axioms`: the main theorems use `propext`, `Classical.choice`, `Quot.sound` (or fewer).
