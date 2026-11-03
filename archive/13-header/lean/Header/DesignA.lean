import Header.Clock
/-!
# Design A: one magnitude with one direction

A number is a pair `(m, d)`, `m` a magnitude (a natural number; one word, so `m < N` where the word matters)
and `d : Fin 4` a direction; its value is the Gaussian integer `m · i^d`.
-/

set_option linter.unusedVariables false

namespace Header
namespace A

/-- A Design A number `(m, d)`. -/
structure Num where
  m : Nat
  d : Fin 4
deriving DecidableEq, Repr

/-- The value `m · i^d`. -/
def val (x : Num) : GI := GI.smul x.m (unit x.d)

/-- Two directions lie on different axes: they differ by 1 (mod 4), i.e. their low bits differ. -/
def diffAxes (d1 d2 : Fin 4) : Prop := d1.val % 2 ≠ d2.val % 2

instance (d1 d2 : Fin 4) : Decidable (diffAxes d1 d2) := by unfold diffAxes; infer_instance

/-! ## The direction check (one XOR) -/

/-- The direction check `t = d1 XOR d2` on the two direction bits. -/
def dirCheck (d1 d2 : Fin 4) : Nat := d1.val ^^^ d2.val

/-- `00` means the same direction. -/
theorem dirCheck_same : ∀ d1 d2 : Fin 4, dirCheck d1 d2 = 0 ↔ d1 = d2 := by decide
/-- `10` means opposite: `d2 = d1 + 2`. -/
theorem dirCheck_opp : ∀ d1 d2 : Fin 4, dirCheck d1 d2 = 2 ↔ d2 = d1 + 2 := by decide
/-- A low bit of `1` (`01` or `11`) means different axes. -/
theorem dirCheck_diff : ∀ d1 d2 : Fin 4, dirCheck d1 d2 % 2 = 1 ↔ diffAxes d1 d2 := by decide
/-- The three cases are exhaustive and exclusive. -/
theorem dir_trichotomy : ∀ d1 d2 : Fin 4,
    (d1 = d2 ∧ ¬ diffAxes d1 d2) ∨ (d2 = d1 + 2 ∧ d1 ≠ d2 ∧ ¬ diffAxes d1 d2) ∨
    (diffAxes d1 d2 ∧ d1 ≠ d2 ∧ d2 ≠ d1 + 2) := by decide

/-! ## The operations, as the notes and spec §2 state them -/

/-- Addition on an `N`-value word (`none` is "an expression": different axes, both nonzero).
* same (`t = 0`): `m1 + m2` (on the clock), first direction;
* opposite (`t = 2`): the absolute-difference routine, direction `d1 + 2b`;
* different axes: a zero takes the other's direction and the add runs as same-direction. -/
def add (N : Nat) (x y : Num) : Option Num :=
  let t := dirCheck x.d y.d
  if t = 0 then some ⟨clockAdd N x.m y.m, x.d⟩
  else if t = 2 then some ⟨absDiffClock N x.m y.m, x.d + (if borrow x.m y.m then 2 else 0)⟩
  else if x.m = 0 then some ⟨clockAdd N x.m y.m, y.d⟩
  else if y.m = 0 then some ⟨clockAdd N x.m y.m, x.d⟩
  else none

/-- Negation adds 2 to the direction. -/
def neg (x : Num) : Num := ⟨x.m, x.d + 2⟩

/-- Subtraction is addition of the negation. -/
def sub (N : Nat) (x y : Num) : Option Num := add N x (neg y)

/-- Multiplication multiplies magnitudes and adds directions. -/
def mul (x y : Num) : Num := ⟨x.m * y.m, x.d + y.d⟩

/-- Exact division divides magnitudes and subtracts directions. -/
def div (x y : Num) : Num := ⟨x.m / y.m, x.d - y.d⟩

/-! ## Value lemmas -/

@[simp] theorem val_re (x : Num) : (val x).re = x.m * (unit x.d).re := rfl
@[simp] theorem val_im (x : Num) : (val x).im = x.m * (unit x.d).im := rfl

/-- Any pair with magnitude 0 is zero. -/
theorem val_zero (d : Fin 4) : val ⟨0, d⟩ = 0 := by
  apply GI.ext' <;> simp

/-- Same direction: `m1 i^d + m2 i^d = (m1 + m2) i^d`. -/
theorem val_same (m1 m2 : Nat) (d : Fin 4) : val ⟨m1 + m2, d⟩ = val ⟨m1, d⟩ + val ⟨m2, d⟩ := by
  apply GI.ext' <;> simp <;> grind

/-- Opposite directions: `|m1 - m2| · i^(d + 2b) = m1 i^d + m2 i^(d+2)`. -/
theorem val_opp (m1 m2 : Nat) (d : Fin 4) :
    val ⟨absDiff m1 m2, d + (if borrow m1 m2 then 2 else 0)⟩ = val ⟨m1, d⟩ + val ⟨m2, d + 2⟩ := by
  unfold absDiff borrow
  by_cases h : m1 < m2
  · have h' : ¬ m2 ≤ m1 := by omega
    apply GI.ext' <;> rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp [h, h', unit] <;> omega
  · have h' : m2 ≤ m1 := by omega
    apply GI.ext' <;> rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp [h, h', unit] <;> omega

/-! ## 1. The borrow lemma, for the direction -/

/-- **Borrow lemma.** For `m1, m2 < N`: the routine's magnitude is `|m1 - m2|` (`borrow_lemma` in `Clock`), and the
direction `d1 + 2b` is right: `(m1, d1) + (m2, d1 + 2)` has value `|m1 - m2| · i^(d1 + 2b)`. -/
theorem borrow_lemma_A {N m1 m2 : Nat} (h1 : m1 < N) (h2 : m2 < N) (d : Fin 4) :
    absDiffClock N m1 m2 = absDiff m1 m2 ∧
    val ⟨absDiffClock N m1 m2, d + (if borrow m1 m2 then 2 else 0)⟩ = val ⟨m1, d⟩ + val ⟨m2, d + 2⟩ := by
  rw [borrow_lemma h1 h2]; exact ⟨rfl, val_opp m1 m2 d⟩

/-! ## 2. Addition -/

theorem add_of_same {N : Nat} {x y : Num} (h : x.d = y.d) :
    add N x y = some ⟨clockAdd N x.m y.m, x.d⟩ := by
  simp [add, (dirCheck_same _ _).2 h]

theorem add_of_opp {N : Nat} {x y : Num} (h : y.d = x.d + 2) :
    add N x y = some ⟨absDiffClock N x.m y.m, x.d + (if borrow x.m y.m then 2 else 0)⟩ := by
  have h2 := (dirCheck_opp _ _).2 h
  simp [add, h2]

theorem add_of_diff {N : Nat} {x y : Num} (h : diffAxes x.d y.d) :
    add N x y = if x.m = 0 then some ⟨clockAdd N x.m y.m, y.d⟩
                else if y.m = 0 then some ⟨clockAdd N x.m y.m, x.d⟩ else none := by
  have h1 := (dirCheck_diff _ _).2 h
  have h0 : dirCheck x.d y.d ≠ 0 := by omega
  have h2 : dirCheck x.d y.d ≠ 2 := by omega
  simp [add, h0, h2]

/-- **Addition, opposite directions.** For `m1, m2 < N` (no further hypothesis: no overflow is possible), the rule
gives the true sum, and its magnitude stays below `N`. -/
theorem add_value_opp {N : Nat} {x y : Num} (h1 : x.m < N) (h2 : y.m < N) (hopp : y.d = x.d + 2) :
    ∃ r, add N x y = some r ∧ val r = val x + val y ∧ r.m < N := by
  refine ⟨_, add_of_opp hopp, ?_, absDiffClock_lt h1 h2⟩
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  simp only at hopp ⊢; subst hopp
  exact (borrow_lemma_A h1 h2 d1).2

/-- **Addition.** If the directions are the same, opposite, or on different axes with a zero magnitude, then
(assuming no overflow, `m1 + m2 < N`) the rule gives a pair whose value is the sum of the values. -/
theorem add_value {N : Nat} {x y : Num} (hsum : x.m + y.m < N)
    (hax : ¬ diffAxes x.d y.d ∨ x.m = 0 ∨ y.m = 0) :
    ∃ r, add N x y = some r ∧ val r = val x + val y ∧ r.m < N := by
  have hc := clockAdd_of_lt hsum
  rcases dir_trichotomy x.d y.d with ⟨hs, -⟩ | ⟨ho, -, -⟩ | ⟨hd, -, -⟩
  · refine ⟨_, add_of_same hs, ?_, by rw [hc]; exact hsum⟩
    obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
    simp only at hs hc ⊢; subst hs; rw [hc]; exact val_same m1 m2 d1
  · exact add_value_opp (by omega) (by omega) ho
  · rw [add_of_diff hd]
    obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
    simp only at hc hax hd hsum ⊢
    rw [hc]
    by_cases hm1 : m1 = 0
    · subst hm1; refine ⟨_, rfl, ?_, by omega⟩
      rw [val_zero]; apply GI.ext' <;> simp
    · have hm2 : m2 = 0 := by simp [hd] at hax; omega
      subst hm2; refine ⟨⟨m1 + 0, d1⟩, by simp [hm1], ?_, by simp; omega⟩
      apply GI.ext' <;> simp

/-- The rule returns "an expression" exactly when the directions are on different axes and both magnitudes are
nonzero. -/
theorem add_eq_none_iff {N : Nat} {x y : Num} :
    add N x y = none ↔ diffAxes x.d y.d ∧ x.m ≠ 0 ∧ y.m ≠ 0 := by
  rcases dir_trichotomy x.d y.d with ⟨hs, hn⟩ | ⟨ho, -, hn⟩ | ⟨hd, -, -⟩
  · simp [add_of_same hs, hn]
  · simp [add_of_opp ho, hn]
  · rw [add_of_diff hd]
    by_cases h1 : x.m = 0 <;> by_cases h2 : y.m = 0 <;> simp [h1, h2, hd]

/-- **Across axes, no single direction.** If the directions are on different axes and both magnitudes are nonzero,
the sum is not an integer times a single unit; in particular no pair has that value. -/
theorem add_cross_not_unit {x y : Num} (hd : diffAxes x.d y.d) (h1 : x.m ≠ 0) (h2 : y.m ≠ 0) :
    ∀ (k : Int) (e : Fin 4), GI.smul k (unit e) ≠ val x + val y := by
  intro k e heq
  have hre := congrArg GI.re heq
  have him := congrArg GI.im heq
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  simp only [diffAxes] at hd h1 h2
  rcases fin4_cases e with rfl|rfl|rfl|rfl <;> rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;>
    rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;> simp [unit] at hd hre him <;> omega

theorem add_cross_no_pair {x y : Num} (hd : diffAxes x.d y.d) (h1 : x.m ≠ 0) (h2 : y.m ≠ 0) :
    ∀ r : Num, val r ≠ val x + val y :=
  fun r => add_cross_not_unit hd h1 h2 r.m r.d

/-! ## 3. Negation and subtraction -/

/-- Turning the second direction by 2 keeps the axis relation. -/
theorem diffAxes_add_two : ∀ a b : Fin 4, diffAxes a (b + 2) ↔ diffAxes a b := by decide

/-- `value(-x) = -value(x)`. -/
theorem val_neg (x : Num) : val (neg x) = -val x := by
  simp only [val, neg, unit_add_two]; apply GI.ext' <;> simp [Int.mul_neg]

/-- **Subtraction** (no overflow, and not across axes unless a magnitude is zero): value `x - y`. -/
theorem sub_value {N : Nat} {x y : Num} (hsum : x.m + y.m < N)
    (hax : ¬ diffAxes x.d y.d ∨ x.m = 0 ∨ y.m = 0) :
    ∃ r, sub N x y = some r ∧ val r = val x - val y ∧ r.m < N := by
  have hax' : ¬ diffAxes x.d (neg y).d ∨ x.m = 0 ∨ (neg y).m = 0 := by
    simpa [neg, diffAxes_add_two] using hax
  obtain ⟨r, h1, h2, h3⟩ := add_value (y := neg y) hsum hax'
  refine ⟨r, h1, ?_, h3⟩
  rw [h2, val_neg]; apply GI.ext' <;> simp <;> omega

/-- **Subtraction, same directions** (the opposite branch after negation): no overflow hypothesis. -/
theorem sub_value_same {N : Nat} {x y : Num} (h1 : x.m < N) (h2 : y.m < N) (hs : x.d = y.d) :
    ∃ r, sub N x y = some r ∧ val r = val x - val y ∧ r.m < N := by
  obtain ⟨r, e1, e2, e3⟩ := add_value_opp (y := neg y) h1 h2 (by simp [neg, hs])
  refine ⟨r, e1, ?_, e3⟩
  rw [e2, val_neg]; apply GI.ext' <;> simp <;> omega

theorem sub_eq_none_iff {N : Nat} {x y : Num} :
    sub N x y = none ↔ diffAxes x.d y.d ∧ x.m ≠ 0 ∧ y.m ≠ 0 := by
  simp [sub, add_eq_none_iff, neg, diffAxes_add_two]

/-! ## 4. Multiplication -/

/-- `value(m1 m2, d1 + d2) = value(x) · value(y)`. -/
theorem val_mul (x y : Num) : val (mul x y) = val x * val y := by
  simp only [val, mul, unit_add, GI.smul_mul_smul, Int.natCast_mul]

/-! ## 5. Exact division -/

/-- If `m2 > 0` and `m2 ∣ m1`, then `value(m1 / m2, d1 - d2) · value(y) = value(x)`. -/
theorem val_div_mul {x y : Num} (hy : 0 < y.m) (hdvd : y.m ∣ x.m) : val (div x y) * val y = val x := by
  simp only [val, div, GI.smul_mul_smul, unit_sub_mul]
  rw [← Int.natCast_mul, Nat.div_mul_cancel hdvd]

/-- The quotient is the only pair-value with that property (division by a nonzero value is exact). -/
theorem div_unique {x y q : Num} (hy : 0 < y.m) (h : val q * val y = val x) (hdvd : y.m ∣ x.m) :
    val q = val (div x y) := by
  have h2 := val_div_mul hy hdvd
  rw [← h2] at h
  -- cancel the nonzero factor val y: multiply by its conjugate
  obtain ⟨m2, d2⟩ := y
  simp only at hy
  have hre := congrArg GI.re h; have him := congrArg GI.im h
  apply GI.ext'
  all_goals
    rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;> simp [unit] at hre him <;>
    first
    | exact Int.eq_of_mul_eq_mul_right (by omega) hre
    | exact Int.eq_of_mul_eq_mul_right (by omega) him
    | (have := Int.eq_of_mul_eq_mul_right (a := (m2 : Int)) (by omega) (by grind : _ = _); omega)
    | grind

/-! ## 6. Zero and equality -/

/-- Any two pairs with magnitude 0 have equal value. -/
theorem zeros_equal (d1 d2 : Fin 4) : val ⟨0, d1⟩ = val ⟨0, d2⟩ := by rw [val_zero, val_zero]

/-- Equality of values holds exactly when the magnitudes are equal and (the directions are equal or the magnitude
is 0): every number other than zero has exactly one pair. -/
theorem val_eq_iff (x y : Num) : val x = val y ↔ x.m = y.m ∧ (x.d = y.d ∨ x.m = 0) := by
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  rw [GI.ext_iff']
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    simp [unit] <;> omega

/-! ## 7. Comparison -/

/-- 1 and `i` are the positive directions. -/
def isPos (d : Fin 4) : Bool := decide (d.val < 2)

/-- Spec §2's ordering: zero first (both zero, equal; one zero, the other's sign decides); then opposite
directions, the positive one larger; same direction, magnitudes, reversed when negative; different axes are not
ordered (`none`). -/
def cmp (x y : Num) : Option Ordering :=
  if x.m = 0 ∧ y.m = 0 then some .eq
  else if x.m = 0 then some (if isPos y.d then .lt else .gt)
  else if y.m = 0 then some (if isPos x.d then .gt else .lt)
  else if dirCheck x.d y.d = 2 then some (if isPos x.d then .gt else .lt)
  else if dirCheck x.d y.d = 0 then some (if isPos x.d then ord x.m y.m else ord y.m x.m)
  else none

/-- The two axes. -/
inductive Axis | real | imag
deriving DecidableEq, Repr

/-- The coordinate along an axis (the real part on the real axis, the imaginary part on the imaginary axis). -/
def coord : Axis → GI → Int
  | .real, v => v.re
  | .imag, v => v.im

/-- The coordinate off an axis: a value lies on the axis exactly when this is 0. -/
def off : Axis → GI → Int
  | .real, v => v.im
  | .imag, v => v.re

/-- The axis bit of a direction on that axis. -/
def Axis.bit : Axis → Nat
  | .real => 0
  | .imag => 1

/-- A pair lies on an axis: it is zero (on both axes) or its direction is on that axis. -/
def onAxis (a : Axis) (x : Num) : Prop := x.m = 0 ∨ x.d.val % 2 = a.bit

/-- `onAxis` is a statement about the value: the value's coordinate off the axis is 0. -/
theorem onAxis_iff (a : Axis) (x : Num) : onAxis a x ↔ off a (val x) = 0 := by
  obtain ⟨m, d⟩ := x
  cases a <;> rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp [onAxis, off, Axis.bit, unit] <;> omega

/-- **Comparison on a common axis.** When both numbers lie on the axis `a` (in particular when a zero is
involved, since zero lies on both), the rule returns the true order of their coordinates along `a`. -/
theorem cmp_correct (a : Axis) (x y : Num) (hx : onAxis a x) (hy : onAxis a y) :
    cmp x y = some (ord (coord a (val x)) (coord a (val y))) := by
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  simp only [onAxis] at hx hy
  cases a <;> rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    simp [Axis.bit] at hx hy <;>
    simp [cmp, ord, coord, unit, isPos, dirCheck] <;>
    (repeat' split) <;> intros <;> first | rfl | omega | (exfalso; omega)

/-- The rule refuses (`none`) exactly when both are nonzero on different axes. -/
theorem cmp_eq_none_iff (x y : Num) : cmp x y = none ↔ x.m ≠ 0 ∧ y.m ≠ 0 ∧ diffAxes x.d y.d := by
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    simp [cmp, dirCheck, diffAxes] <;> (repeat' split) <;> intros <;> first | rfl | omega | (exfalso; omega) | simp_all

/-- ... which is exactly when no axis holds both values. -/
theorem cmp_eq_none_iff_no_common_axis (x y : Num) :
    cmp x y = none ↔ ¬ ∃ a, off a (val x) = 0 ∧ off a (val y) = 0 := by
  rw [cmp_eq_none_iff]
  simp only [← onAxis_iff, onAxis, diffAxes]
  constructor
  · rintro ⟨h1, h2, hd⟩ ⟨a, ha1, ha2⟩; omega
  · intro h
    have hx0 : x.m ≠ 0 := fun h0 => h ⟨if y.d.val % 2 = 0 then .real else .imag, Or.inl h0, by
      split <;> simp [Axis.bit] <;> omega⟩
    have hy0 : y.m ≠ 0 := fun h0 => h ⟨if x.d.val % 2 = 0 then .real else .imag, by
      split <;> simp [Axis.bit] <;> omega, Or.inl h0⟩
    refine ⟨hx0, hy0, fun he => h ⟨if x.d.val % 2 = 0 then .real else .imag, ?_, ?_⟩⟩ <;>
      split <;> simp [Axis.bit] <;> omega

/-- **No common real line.** Two nonzero numbers on different axes are not on any common line through zero:
their cross product `re1·im2 - im1·re2` is nonzero, so there is no Gaussian integer `w` and integers `s, t` with
`value(x) = s·w` and `value(y) = t·w`; there is no line on which to order them. -/
theorem cross_ne_zero {x y : Num} (hd : diffAxes x.d y.d) (h1 : x.m ≠ 0) (h2 : y.m ≠ 0) :
    (val x).re * (val y).im - (val x).im * (val y).re ≠ 0 := by
  obtain ⟨m1, d1⟩ := x; obtain ⟨m2, d2⟩ := y
  simp only [diffAxes] at hd h1 h2
  have hp : (m1 : Int) * m2 ≠ 0 := Int.mul_ne_zero (by omega) (by omega)
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    simp [unit] at hd ⊢ <;> first | exact hp | exact ⟨h1, h2⟩ | (intro h; apply hp; grind) | omega

theorem no_common_line {x y : Num} (hd : diffAxes x.d y.d) (h1 : x.m ≠ 0) (h2 : y.m ≠ 0) :
    ¬ ∃ (w : GI) (s t : Int), val x = GI.smul s w ∧ val y = GI.smul t w := by
  rintro ⟨w, s, t, hx, hy⟩
  apply cross_ne_zero hd h1 h2
  rw [hx, hy]; simp; grind

end A
end Header
