import Header.DesignB
/-!
# Design B, complex numbers: four slots

A number is four magnitudes `m 0, m 1, m 2, m 3` along `1, i, -1, -i`; its value is
`m0·1 + m1·i + m2·(-1) + m3·(-i)`.
-/

namespace Header
namespace B4

/-- Four magnitudes, one per direction. -/
abbrev Num := Fin 4 → Nat

/-- The value `m0 + m1 i - m2 - m3 i`, written as the sum of `m_k · i^k`. -/
def val (x : Num) : GI :=
  GI.smul (x 0) (unit 0) + GI.smul (x 1) (unit 1) + GI.smul (x 2) (unit 2) + GI.smul (x 3) (unit 3)

theorem val_re (x : Num) : (val x).re = (x 0 : Int) - x 2 := by simp [val, unit]; omega
theorem val_im (x : Num) : (val x).im = (x 1 : Int) - x 3 := by simp [val, unit]; omega

/-- Addition adds the four parts in place. -/
def add (x y : Num) : Num := fun k => x k + y k
/-- Negation turns every part by 2. -/
def neg (x : Num) : Num := fun k => x (k + 2)
/-- Subtraction: slot `k` of `y` is added to slot `k + 2` (the crossed parts). -/
def sub (x y : Num) : Num := fun k => x k + y (k + 2)
/-- Multiplication: slot `k` collects every product `x j · y l` with `j + l = k (mod 4)`, i.e. `l = k - j`. -/
def mul (x y : Num) : Num := fun k => x 0 * y (k - 0) + x 1 * y (k - 1) + x 2 * y (k - 2) + x 3 * y (k - 3)
/-- Collapse per axis: the parts on `1, -1` collapse as an integer does, and so do those on `i, -i`. -/
def collapse (x : Num) : Num :=
  let r := B.collapse ⟨x 0, x 2⟩
  let s := B.collapse ⟨x 1, x 3⟩
  fun k => match k with
    | 0 => r.P
    | 1 => s.P
    | 2 => r.N
    | 3 => s.N
/-- Collapsed: on each axis one part is zero. -/
def Canonical (x : Num) : Prop := (x 0 = 0 ∨ x 2 = 0) ∧ (x 1 = 0 ∨ x 3 = 0)

/-- The product of slot `j` and slot `l` lands in slot `j + l (mod 4)` and nowhere else: in slot `k`'s sum it is
the term with `l = k - j`. -/
theorem mul_slot : ∀ j l k : Fin 4, (l = k - j ↔ j + l = k) := by decide

theorem val_add (x y : Num) : val (add x y) = val x + val y := by
  apply GI.ext' <;> simp [val_re, val_im, add] <;> omega

theorem val_neg (x : Num) : val (neg x) = -val x := by
  apply GI.ext' <;> simp [val_re, val_im, neg] <;> omega

theorem val_sub (x y : Num) : val (sub x y) = val x - val y := by
  apply GI.ext' <;> simp [val_re, val_im, sub] <;> omega

theorem val_mul (x y : Num) : val (mul x y) = val x * val y := by
  apply GI.ext' <;> simp only [val_re, val_im, mul, GI.mul_re, GI.mul_im] <;> simp <;> grind

theorem val_collapse (x : Num) : val (collapse x) = val x := by
  have h1 := B.val_collapse ⟨x 0, x 2⟩
  have h2 := B.val_collapse ⟨x 1, x 3⟩
  simp only [B.val] at h1 h2
  apply GI.ext' <;> simp only [val_re, val_im, collapse] <;> omega

theorem collapse_canonical (x : Num) : Canonical (collapse x) :=
  ⟨B.collapse_canonical ⟨x 0, x 2⟩, B.collapse_canonical ⟨x 1, x 3⟩⟩

/-- Two collapsed four-slot numbers with the same value are equal: the collapse is the unique canonical form. -/
theorem canonical_unique {x y : Num} (hx : Canonical x) (hy : Canonical y) (h : val x = val y) : x = y := by
  have hre := congrArg GI.re h; have him := congrArg GI.im h
  rw [val_re, val_re] at hre; rw [val_im, val_im] at him
  obtain ⟨hx1, hx2⟩ := hx; obtain ⟨hy1, hy2⟩ := hy
  funext k
  rcases fin4_cases k with rfl|rfl|rfl|rfl <;> omega

/-- The spec's per-axis collapse on a `W`-value word (`|m0 - m2|` to slot 0 or 2, `|m1 - m3|` to slot 1 or 3, by
the borrow) is this collapse, for slots below `W`. -/
def collapseClock (W : Nat) (x : Num) : Num :=
  let r := B.collapseClock W ⟨x 0, x 2⟩
  let s := B.collapseClock W ⟨x 1, x 3⟩
  fun k => match k with
    | 0 => r.P
    | 1 => s.P
    | 2 => r.N
    | 3 => s.N

theorem collapseClock_eq {W : Nat} {x : Num} (h : ∀ k, x k < W) : collapseClock W x = collapse x := by
  have e1 : B.collapseClock W ⟨x 0, x 2⟩ = B.collapse ⟨x 0, x 2⟩ := B.collapseClock_eq (h 0) (h 2)
  have e2 : B.collapseClock W ⟨x 1, x 3⟩ = B.collapse ⟨x 1, x 3⟩ := B.collapseClock_eq (h 1) (h 3)
  simp only [collapseClock, collapse, e1, e2]

end B4
end Header
