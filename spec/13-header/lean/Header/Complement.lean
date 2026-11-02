import Header.Packed
/-!
# The complement layout (round 1)

`../complement-research.md` and the build `cpl` (`../src/designcpl.nim`). One 64-bit word:

* bit 63, the sign start `s`; bit 62, the axis `a` (`G = 2^62`); bits 0–61, the count `c`;
* value `(c − 2^62·s) · i^a`. On each axis `(s, c)` is a 63-bit two's-complement integer, `−2^62 … 2^62 − 1`.

The formulas are the ones built, on natural numbers mod `2^64` (as in `MulRow.lean`, `LargerRow.lean`):

* full add (`cadd`): the research's "Branch-free, no selects" add, the bridge `b = z1 + (w2 OR G)` and the
  expression flag `e`;
* negate (`cneg`): `z = w AND G; d = z − w; r = (d AND NOT G) OR z`;
* subtract (`csub`): `cneg w2`, then `cadd`.

**Round 1.** `cenc` writes a round 1 number `⟨m, d⟩` (`A.Num`, value `m · i^d`) as a word: the signed value
`±m` on axis `d mod 2`. It needs `±m` in `−2^62 … 2^62 − 1` (`fits`). The layout has one zero per axis, so
`⟨0, d⟩` and `⟨0, d + 2⟩` (`0` and `−0`) are the same word: this is the only place where "either zero" enters, and
it is stated by `cenc_zero`. With that, the theorems are exact word equalities with round 1's `A.add`, `A.sub`,
`A.neg` (`cadd_ref`, `csub_ref`, `cneg_ref`): whenever round 1's result fits the layout, the formula gives its word
and flag 0; when round 1 gives an expression, the flag is 1.

**Proof method.** A word is written by its parts `P h c = 2^62·h + c` (`h < 4` the two top bits, `c < 2^62`);
each formula is evaluated on parts (`cadd_parts`, `cneg_parts`), then compared with `cenc` of round 1's result by
cases on the directions and `omega`.
-/

namespace Header
namespace Cpl
open A Packed

/-! ## Constants and parts -/

/-- `G = 2^62`, the axis bit. -/
abbrev G : Nat := 4611686018427387904
/-- `2^64`. -/
abbrev W : Nat := 18446744073709551616
/-- `2^62 − 1`, the count mask. -/
abbrev CM : Nat := 4611686018427387903
/-- `NOT G` on 64 bits, `2^64 − 1 − 2^62`. -/
abbrev NG : Nat := 13835058055282163711
/-- All ones, `2^64 − 1`. -/
abbrev ONES : Nat := 18446744073709551615

/-- A word by its parts: the top two bits `h` (sign start, axis) and the count `c`. -/
def P (h c : Nat) : Nat := G * h + c

/-- `omega` after unfolding the parts and the word constants. -/
macro "comega" : tactic => `(tactic| ((try simp only [P, G, W, CM, NG, ONES, Nat.reducePow] at *) <;> omega))

theorem G_eq : G = 2 ^ 62 := rfl

/-- `h < 4` as four cases. -/
theorem four_cases {h : Nat} (hh : h < 4) : h = 0 ∨ h = 1 ∨ h = 2 ∨ h = 3 := by omega

theorem P_lt {h c : Nat} (hh : h < 4) (hc : c < G) : P h c < W := by comega
theorem P_lt' {h c : Nat} (hh : h < 4) (hc : c < G) : P h c < 2 ^ 64 := P_lt hh hc

theorem P_eq_iff {h c k e : Nat} (hc : c < G) (he : e < G) : P h c = P k e ↔ h = k ∧ c = e := by
  comega

theorem P_and {h c k e : Nat} (hc : c < G) (he : e < G) : P h c &&& P k e = P (h &&& k) (c &&& e) :=
  and_parts (k := 62) hc he
theorem P_xor {h c k e : Nat} (hc : c < G) (he : e < G) : P h c ^^^ P k e = P (h ^^^ k) (c ^^^ e) :=
  xor_parts (k := 62) hc he
theorem P_or {h c k e : Nat} (hc : c < G) (he : e < G) : P h c ||| P k e = P (h ||| k) (c ||| e) :=
  or_parts (k := 62) hc he

theorem NG_P : NG = P 2 CM := rfl
theorem G_P : G = P 1 0 := rfl
theorem ONES_P : ONES = P 3 CM := rfl
theorem zero_P : (0 : Nat) = P 0 0 := rfl

theorem and_CM {c : Nat} (hc : c < G) : c &&& CM = c := by
  have := Nat.and_two_pow_sub_one_eq_mod c 62
  simp only [Nat.reducePow, Nat.reduceSub] at this
  rw [this]; comega

theorem P_and_NG {h c : Nat} (hc : c < G) : P h c &&& NG = P (h &&& 2) c := by
  rw [NG_P, P_and hc (by decide), and_CM hc]
theorem P_and_G {h c : Nat} (hc : c < G) : P h c &&& G = P (h &&& 1) 0 := by
  rw [G_P, P_and hc (by decide), Nat.and_zero]
theorem P_or_G {h c : Nat} (hc : c < G) : P h c ||| G = P (h ||| 1) c := by
  rw [G_P, P_or hc (by decide), Nat.or_zero]
theorem P_xor_P0 {h c k : Nat} (hc : c < G) : P h c ^^^ P k 0 = P (h ^^^ k) c := by
  rw [P_xor hc (by decide), Nat.xor_zero]

/-- Add on parts: the carry out of the counts goes into the top bits. -/
theorem P_add {h c k e : Nat} :
    (P h c + P k e) % W = P ((h + k + (c + e) / G) % 4) ((c + e) % G) := by
  comega

/-- Subtract on parts (`x − y` as `x + (2^64 − y)`): the borrow of the counts comes out of the top bits. -/
theorem P_sub {h c k e : Nat} (hk : k < 4) (hc : c < G) (he : e < G) :
    (P h c + (W - P k e)) % W = P ((h + 7 - k + (c + G - e) / G) % 4) ((c + G - e) % G) := by
  comega

theorem P_shr {h c : Nat} (hc : c < G) : P h c >>> 62 = h := by
  rw [Nat.shiftRight_eq_div_pow]; comega

theorem P_eq_zero {h c : Nat} : P h c = 0 ↔ h = 0 ∧ c = 0 := by comega

theorem and_ones {x : Nat} (hx : x < W) : x &&& ONES = x := and_ones64 hx

/-! ## Small facts on the top two bits -/

theorem h_and2 {h : Nat} (hh : h < 4) : h &&& 2 = 2 * (h / 2) := by
  rcases four_cases hh with rfl|rfl|rfl|rfl <;> rfl
theorem h_or1 {h : Nat} (hh : h < 4) : h ||| 1 = 2 * (h / 2) + 1 := by
  rcases four_cases hh with rfl|rfl|rfl|rfl <;> rfl
theorem h_and1 (h : Nat) : h &&& 1 = h % 2 := Nat.and_one_is_mod h
theorem h_xor_lt {h k : Nat} (hh : h < 4) (hk : k < 4) : h ^^^ k < 4 := Nat.xor_lt_two_pow (n := 2) hh hk
theorem h_xor_mod2 {h k : Nat} (hh : h < 4) (hk : k < 4) : (h ^^^ k) % 2 = (h + k) % 2 := by
  rcases four_cases hh with rfl|rfl|rfl|rfl <;> rcases four_cases hk with rfl|rfl|rfl|rfl <;> rfl

/-- The fix `x xor ((x xor y) and G)`: `x` with `y`'s axis bit. -/
theorem fix_P {hx cx hy cy : Nat} (hhx : hx < 4) (hhy : hy < 4) (hcx : cx < G) (hcy : cy < G) :
    P hx cx ^^^ ((P hx cx ^^^ P hy cy) &&& G) = P (2 * (hx / 2) + hy % 2) cx := by
  rw [P_xor hcx hcy, P_and_G (Nat.xor_lt_two_pow (n := 62) hcx hcy), P_xor_P0 hcx]
  congr 1
  rcases four_cases hhx with rfl|rfl|rfl|rfl <;> rcases four_cases hhy with rfl|rfl|rfl|rfl <;> rfl

/-! ## The formulas -/

/-- `x = 0` as a 0/1 value (`seqz`). -/
def seqz (x : Nat) : Nat := if x = 0 then 1 else 0
/-- `x ≠ 0` as a 0/1 value (`snez`). -/
def snez (x : Nat) : Nat := if x ≠ 0 then 1 else 0

/-- The full add on the axes: `(result word, expression flag e)`. 11 steps for the result, 5 for `e`. -/
def cadd (w1 w2 : Nat) : Nat × Nat :=
  let t := w1 ^^^ w2
  let z1 := w1 &&& NG
  let b := (z1 + (w2 ||| G)) % W
  let Z1 := (W - seqz z1) % W
  let src := w1 ^^^ (t &&& Z1)
  let r := b ^^^ ((b ^^^ src) &&& G)
  let z2 := w2 &&& NG
  let e := ((t &&& (Z1 ^^^ ONES)) >>> 62) &&& snez z2
  (r, e)

/-- Negate: `z = w AND G; d = z − w; r = (d AND NOT G) OR z`. -/
def cneg (w : Nat) : Nat :=
  let z := w &&& G
  let d := (z + (W - w)) % W
  (d &&& NG) ||| z

/-- Subtract: negate `w2`, then the full add. -/
def csub (w1 w2 : Nat) : Nat × Nat := cadd w1 (cneg w2)

/-! ## The formulas on parts -/

/-- **The add on parts.** `w1` is a zero exactly when `h1 < 2` (sign start 0) and `c1 = 0`. The result's sign start
is bit 63 of the bridge `b`, its axis is `w2`'s when `w1` is a zero and `w1`'s otherwise, and its count is the
counts' sum mod `2^62`; the flag is 1 exactly when neither is a zero and the axes differ. -/
theorem cadd_parts {h1 c1 h2 c2 : Nat} (hh1 : h1 < 4) (hh2 : h2 < 4) (hc1 : c1 < G) (hc2 : c2 < G) :
    cadd (P h1 c1) (P h2 c2) =
      (P (2 * (((2 * (h1 / 2) + (2 * (h2 / 2) + 1) + (c1 + c2) / G) % 4) / 2)
          + (if h1 < 2 ∧ c1 = 0 then h2 % 2 else h1 % 2)) ((c1 + c2) % G),
       if (h1 < 2 ∧ c1 = 0) ∨ (h2 < 2 ∧ c2 = 0) then 0 else (h1 + h2) % 2) := by
  have hcs : (c1 + c2) % G < G := Nat.mod_lt _ (by decide)
  have hb4 : (2 * (h1 / 2) + (2 * (h2 / 2) + 1) + (c1 + c2) / G) % 4 < 4 := Nat.mod_lt _ (by decide)
  have ht : P h1 c1 ^^^ P h2 c2 < W := Nat.xor_lt_two_pow (P_lt' hh1 hc1) (P_lt' hh2 hc2)
  have hz2 : P h2 c2 &&& NG = 0 ↔ h2 < 2 ∧ c2 = 0 := by
    rw [P_and_NG hc2, h_and2 hh2, P_eq_zero]; comega
  unfold cadd
  simp only []
  rw [P_and_NG hc1, P_or_G hc2, h_and2 hh1, h_or1 hh2, P_add]
  by_cases z : h1 < 2 ∧ c1 = 0
  · have e0 : P (2 * (h1 / 2)) c1 = 0 := by rw [P_eq_zero]; comega
    have hZ : (W - seqz (P (2 * (h1 / 2)) c1)) % W = ONES := by simp [seqz, e0]
    rw [hZ, and_ones ht, xor_xor_cancel, fix_P hb4 hh2 hcs hc2, Nat.xor_self]
    simp [z]
  · have e0 : P (2 * (h1 / 2)) c1 ≠ 0 := by rw [Ne, P_eq_zero]; comega
    simp only [seqz, e0, ite_false, z, false_or]
    simp only [Nat.sub_zero, Nat.mod_self, Nat.and_zero, Nat.xor_zero, Nat.zero_xor]
    rw [fix_P hb4 hh1 hcs hc1]
    have e1 : (P h1 c1 ^^^ P h2 c2) &&& ONES = P h1 c1 ^^^ P h2 c2 := and_ones ht
    rw [e1, P_xor hc1 hc2, P_shr (Nat.xor_lt_two_pow (n := 62) hc1 hc2)]
    simp only [snez, ne_eq, hz2]
    by_cases z2 : h2 < 2 ∧ c2 = 0
    · simp [z2]
    · simp only [z2, not_false_eq_true, ite_true, ite_false, h_and1, h_xor_mod2 hh1 hh2]

theorem h_or_low {x y : Nat} (hx : x < 4) (hy : y < 2) : (2 * (x / 2)) ||| y = 2 * (x / 2) + y := by
  rcases four_cases hx with rfl|rfl|rfl|rfl <;> (have : y = 0 ∨ y = 1 := by omega) <;> rcases this with rfl|rfl <;> rfl

/-- **Negate on parts.** The count is `2^62 − c` (mod `2^62`), the sign start comes from the borrow through the axis
column, and the axis is kept. -/
theorem cneg_parts {h c : Nat} (hh : h < 4) (hc : c < G) :
    cneg (P h c) = P (2 * (((h % 2 + 7 - h + (G - c) / G) % 4) / 2) + h % 2) ((G - c) % G) := by
  have hd4 : (h % 2 + 7 - h + (0 + G - c) / G) % 4 < 4 := Nat.mod_lt _ (by decide)
  have hdc : (0 + G - c) % G < G := Nat.mod_lt _ (by decide)
  unfold cneg
  simp only []
  rw [P_and_G hc, h_and1, P_sub hh (by decide) hc, P_and_NG hdc, h_and2 hd4,
    P_or hdc (by decide), Nat.or_zero, h_or_low hd4 (Nat.mod_lt _ (by decide))]
  simp only [Nat.zero_add]

/-! ## Round 1's numbers as words -/

/-- A round 1 number fits the layout: its signed value `±m` is in `−2^62 … 2^62 − 1`. -/
def fits (x : Num) : Prop := if x.d.val < 2 then x.m < G else x.m ≤ G

/-- The 63-bit two's complement of the signed value `±m`. -/
def cu (x : Num) : Nat := if x.d.val < 2 ∨ x.m = 0 then x.m else 2 * G - x.m

/-- **The encoding.** Sign start and count from `cu`, the axis `d mod 2`. -/
def cenc (x : Num) : Nat := P (2 * (cu x / G) + x.d.val % 2) (cu x % G)

theorem cu_lt {x : Num} (hx : fits x) : cu x < 2 * G := by
  unfold fits at hx; unfold cu; split <;> split at hx <;> comega

theorem cenc_h {x : Num} (hx : fits x) : 2 * (cu x / G) + x.d.val % 2 < 4 := by
  have := cu_lt hx; have : x.d.val % 2 < 2 := Nat.mod_lt _ (by decide); comega

theorem cenc_c (x : Num) : cu x % G < G := Nat.mod_lt _ (by decide)

/-- The top two bits of `cenc x`. -/
def cenc_hy (x : Num) : Nat := 2 * (cu x / G) + x.d.val % 2

theorem cenc_lt {x : Num} (hx : fits x) : cenc x < W := P_lt (cenc_h hx) (cenc_c x)

/-- **Either zero.** The layout has one zero per axis: `0` and `−0` (directions `d` and `d + 2`) are one word, `0`
on the real axis and `G` on the imaginary one. This is the only identification the encoding makes. -/
theorem cenc_zero : ∀ d : Fin 4, cenc ⟨0, d⟩ = cenc ⟨0, d + 2⟩ ∧ cenc ⟨0, d⟩ = G * (d.val % 2) := by decide

theorem cenc_inj {x y : Num} (hx : fits x) (hy : fits y) :
    cenc x = cenc y ↔ x.m = y.m ∧ (x.d = y.d ∨ (x.m = 0 ∧ x.d.val % 2 = y.d.val % 2)) := by
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  unfold fits at hx hy
  simp only [cenc, cu, P] at hx hy ⊢
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals simp [z1, z2] at hx hy ⊢
  all_goals comega

/-! ## The add, subtract and negate are round 1's -/

/-- **The full add is round 1's add.** For numbers that fit the layout: when round 1's `A.add` gives `r` and `r`
fits (no overflow), the formula gives `r`'s word and flag 0; when `A.add` gives an expression (two nonzero numbers on
different axes), the flag is 1. -/
theorem cadd_ref (x y : Num) (hx : fits x) (hy : fits y) :
    match add W x y with
    | some r => fits r → cadd (cenc x) (cenc y) = (cenc r, 0)
    | none => (cadd (cenc x) (cenc y)).2 = 1 := by
  have e := cadd_parts (cenc_h hx) (cenc_h hy) (cenc_c x) (cenc_c y)
  unfold cenc
  rw [e]
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  unfold fits at hx hy
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, cu, fits, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, P] at hx hy ⊢
  all_goals (repeat' split) <;> comega

/-- **Negate is round 1's negate**, for a number whose negation fits (every number but `−2^62`). -/
theorem cneg_ref (x : Num) (hx : fits x) (hn : fits (neg x)) : cneg (cenc x) = cenc (neg x) := by
  unfold cenc
  rw [cneg_parts (cenc_h hx) (cenc_c x)]
  obtain ⟨m, d⟩ := x
  unfold fits at hx hn
  rcases fin4_cases d with rfl|rfl|rfl|rfl
  all_goals by_cases z : m = 0
  all_goals simp [neg, cu, z, P] at hx hn ⊢
  all_goals (repeat' split) <;> comega

/-- **The exception of negate**: `−2^62` has no negation in the layout; the formula gives `−2^62` back (the 63-bit
wrap, as int64's `neg` does with its minimum). -/
theorem cneg_min (d : Fin 4) (hd : 2 ≤ d.val) : cneg (cenc ⟨G, d⟩) = cenc ⟨G, d⟩ := by
  rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp at hd <;> decide

/-- **Subtract is round 1's subtract** (negate `w2`, then the full add), with the same statement as `cadd_ref`. No
exception for `w2 = −2^62`: the negation wraps, and the add's wrap undoes it whenever round 1's difference fits. -/
theorem csub_ref (x y : Num) (hx : fits x) (hy : fits y) :
    match sub W x y with
    | some r => fits r → csub (cenc x) (cenc y) = (cenc r, 0)
    | none => (csub (cenc x) (cenc y)).2 = 1 := by
  have hn4 : 2 * (((cenc_hy y % 2 + 7 - cenc_hy y + (G - cu y % G) / G) % 4) / 2) + cenc_hy y % 2 < 4 := by
    have : (cenc_hy y % 2 + 7 - cenc_hy y + (G - cu y % G) / G) % 4 < 4 := Nat.mod_lt _ (by decide)
    have : cenc_hy y % 2 < 2 := Nat.mod_lt _ (by decide)
    omega
  have e := cadd_parts (cenc_h hx) hn4 (cenc_c x) (Nat.mod_lt (G - cu y % G) (by decide))
  unfold csub cenc
  rw [cneg_parts (cenc_h hy) (cenc_c y)]
  unfold cenc_hy at e
  rw [e]
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  unfold fits at hx hy
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [sub, neg, add, cu, fits, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, P]
    at hx hy ⊢
  all_goals (repeat' split) <;> comega

end Cpl
end Header
