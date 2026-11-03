import Header.MulRow
import Header.Isa
/-!
# The header form's add from the prior-art research (round 1)

`../prior-art.md`, "Header form", add and subtract (checker: `../test/prior-art/check.py`, `h_add` with `lb`).
Headers are loaded **sign-extended** into a 64-bit register (bit 7 fills bits 8–63) and the result's header is stored
back as a byte:

* `t = h1 xor h2`, `s = t sar 63` (all ones when the sign bits differ, else 0);
* `r = m1 + (m2 xor s) − s` (that is `m1 + m2` or `m1 − m2`), `L = 0 − (m1 <u m2)`, `b = s and L`;
* magnitude `(r xor b) − b`; direction `h1 xor (t and L)`, stored as a byte;
* expression flag `(t shr 6) and (m1 ≠ 0) and (m2 ≠ 0)`;
* subtract: the same with `h2` flipped by `xor −128` on the sign-extended register.

All arithmetic is mod `2^64`, written out on natural numbers as in `MulRow.lean`.

**Finding.** The formula agrees with round 1's `A.add` on every input except one header: two zeros on different axes.
There round 1 gives the second operand's direction (`m1 = 0` is tested first) and the formula the first's (`L = 0`).
Both are zero, so the values agree, and spec v0.2 §3 lets a zero keep any direction. `larger_add_eq` states this
exactly; observation 1 below allows it in the same way.
-/

set_option linter.unusedSimpArgs false

namespace Header
namespace A

/-- A header byte read sign-extended into a 64-bit register. -/
def sx (h : Nat) : Nat := if h < 128 then h else h + (Wd - 256)

/-- `x sar 63` on a 64-bit word: every bit becomes the sign bit. -/
def sar63 (x : Nat) : Nat := if x < 9223372036854775808 then 0 else Wd - 1

/-- `0 − (a <u b)`. -/
def ltMask (a b : Nat) : Nat := if a < b then Wd - 1 else 0

/-- The header form's add: `(header byte, magnitude, flag)` from sign-extended header registers `g1`, `g2`. -/
def largerAddX (g1 m1 g2 m2 : Nat) : Nat × Nat × Nat :=
  let t := g1 ^^^ g2
  let s := sar63 t
  let r := (m1 + (m2 ^^^ s) + (Wd - s)) % Wd
  let L := ltMask m1 m2
  let b := s &&& L
  let mag := ((r ^^^ b) + (Wd - b)) % Wd
  let hd := (g1 ^^^ (t &&& L)) % 256
  let e := (t >>> 6) &&& (if m1 ≠ 0 then 1 else 0) &&& (if m2 ≠ 0 then 1 else 0)
  (hd, mag, e)

/-- Add: both headers loaded sign-extended. -/
def largerAdd (h1 m1 h2 m2 : Nat) : Nat × Nat × Nat := largerAddX (sx h1) m1 (sx h2) m2

/-- Subtract: the second header register flipped by `xor −128` (the word `2^64 − 128`). -/
def largerSub (h1 m1 h2 m2 : Nat) : Nat × Nat × Nat := largerAddX (sx h1) m1 (sx h2 ^^^ (Wd - 128)) m2

theorem xor_ones_W {x : Nat} (h : x < 18446744073709551616) :
    x ^^^ 18446744073709551615 = 18446744073709551615 - x :=
  Isa.xor_ones (x := x) h

theorem xor_ones_mod (x : Nat) :
    x % 18446744073709551616 ^^^ 18446744073709551615 = 18446744073709551615 - x % 18446744073709551616 :=
  Isa.xor_ones (Nat.mod_lt _ (by decide))

theorem clockAdd_lit {a b : Nat} (ha : a < 18446744073709551616) (hb' : b < 18446744073709551616) :
    clockAdd 18446744073709551616 a b = (a + b) % 18446744073709551616 := by
  unfold clockAdd; split <;> omega

theorem mod_lt_W (x : Nat) : x % 18446744073709551616 < 18446744073709551616 := Nat.mod_lt _ (by decide)

/-- `xor −128` on a sign-extended header is the sign-extended `h xor 0x80`. -/
theorem sx_flip : ∀ d : Fin 4, sx (hb d) ^^^ (Wd - 128) = sx (hb (d + 2)) := by decide

theorem sx_0 : sx 0 = 0 := by decide
theorem sx_1 : sx 64 = 64 := by decide
theorem sx_2 : sx 128 = 18446744073709551488 := by decide
theorem sx_3 : sx 192 = 18446744073709551552 := by decide

/-- **The header form's add is A's add.** For magnitudes below `2^64` and every pair of directions: when round 1's
`A.add` gives `r`, the formula gives `r`'s magnitude and flag 0, and `r`'s header byte, except for two zeros on
different axes, where it gives the first operand's header (both are zero); when `A.add` gives an expression, the flag
is 1. -/
theorem larger_add_eq (x y : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) :
    match add (2 ^ 64) x y with
    | some r => largerAdd (hb x.d) x.m (hb y.d) y.m =
        (if diffAxes x.d y.d ∧ x.m = 0 ∧ y.m = 0 then hb x.d else hb r.d, r.m, 0)
    | none => (largerAdd (hb x.d) x.m (hb y.d) y.m).2.2 = 1 := by
  obtain ⟨m1, d1⟩ := x
  obtain ⟨m2, d2⟩ := y
  simp only at h1 h2
  have h1' : m1 < 18446744073709551616 := h1
  have h2' : m2 < 18446744073709551616 := h2
  rw [← Wd_eq]
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, largerAdd, largerAddX, sx, hb_0, hb_1, hb_2, hb_3, dirCheck, diffAxes, Wd,
    clockAdd_lit, h1', h2', absDiffClock, clockSub, clockNeg, borrow, sar63, ltMask, hl, z1, z2, xor_ones_W h2',
    xor_ones_mod]
  all_goals (repeat' split) <;>
    (try simp only [Fin.reduceAdd, Fin.isValue, hb_0, hb_1, hb_2, hb_3, Nat.reduceAnd, Nat.reduceXor, Nat.reduceMod, Nat.reduceSub, xor_ones_mod, true_and, and_true] at *) <;> omega

/-- **Subtraction**: the formula with `h2` flipped is A's subtract, with the same exception. -/
theorem larger_sub_eq (x y : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) :
    match sub (2 ^ 64) x y with
    | some r => largerSub (hb x.d) x.m (hb y.d) y.m =
        (if diffAxes x.d y.d ∧ x.m = 0 ∧ y.m = 0 then hb x.d else hb r.d, r.m, 0)
    | none => (largerSub (hb x.d) x.m (hb y.d) y.m).2.2 = 1 := by
  have e := larger_add_eq x (neg y) h1 h2
  unfold largerSub; rw [sx_flip]
  unfold sub
  have hd : diffAxes x.d (neg y).d ↔ diffAxes x.d y.d := diffAxes_add_two x.d y.d
  simp only [neg] at e hd ⊢
  unfold largerAdd at e
  simpa [hd] using e

/-! ## The two observations, on round 1's definitions -/

/-- **Observation 1.** The result takes the direction of the operand with the larger magnitude (the first when they
are equal), or the result is zero, when any direction will do. -/
theorem obs_larger (x y r : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) (hs : x.m + y.m < 2 ^ 64)
    (h : add (2 ^ 64) x y = some r) :
    r.m = 0 ∨ r.d = (if x.m < y.m then y.d else x.d) := by
  obtain ⟨m1, d1⟩ := x
  obtain ⟨m2, d2⟩ := y
  simp only at h1 h2 hs ⊢
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2] at h
  all_goals subst h
  all_goals simp [hl, z1, z2]
  all_goals (repeat' split) <;> omega

/-- The sign bit of a direction: 0 for `1` and `i`, 1 for `−1` and `−i`. -/
def signBit (d : Fin 4) : Nat := d.val / 2

/-- **Observation 2.** Whether the magnitudes add or subtract depends only on the sign bits: when `A.add` gives a
number (and the sum does not overflow), its magnitude is `m1 + m2` when the sign bits agree and `|m1 − m2|` when they
differ. On different axes one magnitude is 0, where the two agree. -/
theorem obs_signs (x y r : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) (hs : x.m + y.m < 2 ^ 64)
    (h : add (2 ^ 64) x y = some r) :
    r.m = if signBit x.d = signBit y.d then x.m + y.m else absDiff x.m y.m := by
  obtain ⟨m1, d1⟩ := x
  obtain ⟨m2, d2⟩ := y
  simp only at h1 h2 hs ⊢
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2] at h
  all_goals subst h
  all_goals simp [signBit, absDiff, hl, z1, z2]
  all_goals (repeat' split) <;> omega

/-- On different axes with a zero operand, `m1 + m2 = |m1 − m2|`. -/
theorem add_eq_absDiff_of_zero (m1 m2 : Nat) (h : m1 = 0 ∨ m2 = 0) : m1 + m2 = absDiff m1 m2 := by
  unfold absDiff; split <;> omega

end A
end Header
