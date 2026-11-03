/-!
# Basic objects: Gaussian integers, the four units, directions on `Fin 4`

A Gaussian integer is a pair of `Int`s `(re, im)`. The direction `d : Fin 4` stands for `i^d`; arithmetic on
`Fin 4` is arithmetic mod 4, so `d1 + d2` is the notes' "`d1 + d2` taken mod 4".
-/

namespace Header

/-- A Gaussian integer `re + im·i`. -/
structure GI where
  re : Int
  im : Int
deriving DecidableEq, Repr

namespace GI

instance : Zero GI := ⟨⟨0, 0⟩⟩
instance : Add GI := ⟨fun a b => ⟨a.re + b.re, a.im + b.im⟩⟩
instance : Neg GI := ⟨fun a => ⟨-a.re, -a.im⟩⟩
instance : Sub GI := ⟨fun a b => ⟨a.re - b.re, a.im - b.im⟩⟩
instance : Mul GI := ⟨fun a b => ⟨a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re⟩⟩

/-- An integer times a Gaussian integer. -/
def smul (m : Int) (a : GI) : GI := ⟨m * a.re, m * a.im⟩

@[simp] theorem zero_re : (0 : GI).re = 0 := rfl
@[simp] theorem zero_im : (0 : GI).im = 0 := rfl
@[simp] theorem add_re (a b : GI) : (a + b).re = a.re + b.re := rfl
@[simp] theorem add_im (a b : GI) : (a + b).im = a.im + b.im := rfl
@[simp] theorem neg_re (a : GI) : (-a).re = -a.re := rfl
@[simp] theorem neg_im (a : GI) : (-a).im = -a.im := rfl
@[simp] theorem sub_re (a b : GI) : (a - b).re = a.re - b.re := rfl
@[simp] theorem sub_im (a b : GI) : (a - b).im = a.im - b.im := rfl
@[simp] theorem mul_re (a b : GI) : (a * b).re = a.re * b.re - a.im * b.im := rfl
@[simp] theorem mul_im (a b : GI) : (a * b).im = a.re * b.im + a.im * b.re := rfl
@[simp] theorem smul_re (m : Int) (a : GI) : (smul m a).re = m * a.re := rfl
@[simp] theorem smul_im (m : Int) (a : GI) : (smul m a).im = m * a.im := rfl

theorem ext_iff' {a b : GI} : a = b ↔ a.re = b.re ∧ a.im = b.im := by
  cases a; cases b; simp

theorem ext' {a b : GI} (h1 : a.re = b.re) (h2 : a.im = b.im) : a = b := ext_iff'.2 ⟨h1, h2⟩

/-- `(a·u)(b·v) = (ab)·(uv)`. -/
theorem smul_mul_smul (a b : Int) (u v : GI) : smul a u * smul b v = smul (a * b) (u * v) := by
  apply ext' <;> simp <;> grind

end GI

/-- `i^d` for `d : Fin 4`, the notes' table: `00 ↦ 1, 01 ↦ i, 10 ↦ -1, 11 ↦ -i`. -/
def unit : Fin 4 → GI
  | 0 => ⟨1, 0⟩
  | 1 => ⟨0, 1⟩
  | 2 => ⟨-1, 0⟩
  | 3 => ⟨0, -1⟩

/-- `i^(d1 + d2) = i^d1 · i^d2`, with `d1 + d2` mod 4. -/
theorem unit_add : ∀ a b : Fin 4, unit (a + b) = unit a * unit b := by decide

/-- `i^(d + 2) = -i^d`: a half turn is negation. -/
theorem unit_add_two : ∀ a : Fin 4, unit (a + 2) = -unit a := by decide

/-- `i^(d1 - d2) · i^d2 = i^d1`. -/
theorem unit_sub_mul : ∀ a b : Fin 4, unit (a - b) * unit b = unit a := by decide

@[simp] theorem fin4_val_0 : (0 : Fin 4).val = 0 := rfl
@[simp] theorem fin4_val_1 : (1 : Fin 4).val = 1 := rfl
@[simp] theorem fin4_val_2 : (2 : Fin 4).val = 2 := rfl
@[simp] theorem fin4_val_3 : (3 : Fin 4).val = 3 := rfl

/-- Every direction is one of the four. -/
theorem fin4_cases : ∀ d : Fin 4, d = 0 ∨ d = 1 ∨ d = 2 ∨ d = 3 := by decide

/-- A three-way comparison of integers, the reference order (Python's `(a > b) - (a < b)`). -/
def ord (a b : Int) : Ordering := if a < b then .lt else if a = b then .eq else .gt

end Header
