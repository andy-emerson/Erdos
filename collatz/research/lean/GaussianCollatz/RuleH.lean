import GaussianCollatz.Basic
/-!
# Finite facts about the rule `H` (Section 4)

`α = 5 + 2i`, level `k = 4`, `(1+i)^4 = -4`, digits `±1, ±i, ±(2+i), ±(1-2i)`.
-/
namespace GInt

/-- `(1+i)^4 = -4`, so classes modulo `(1+i)^4` are classes modulo `4`. -/
theorem pi_pow_four : pow pi 4 = -4 := by decide

def alphaH : GInt := ⟨5, 2⟩

/-- The digit table of Section 4, indexed by `(a mod 4, b mod 4)` for `z = a + bi`. -/
def digitH (z : GInt) : GInt :=
  match z.re % 4, z.im % 4 with
  | 1, 0 => ⟨-1, 2⟩   -- z ≡ 1       ↦ -1+2i
  | 0, 1 => ⟨-2, -1⟩  -- z ≡ i       ↦ -2-i
  | 3, 0 => ⟨1, -2⟩   -- z ≡ -1      ↦ 1-2i
  | 0, 3 => ⟨2, 1⟩    -- z ≡ -i      ↦ 2+i
  | 1, 2 => ⟨-1, 0⟩   -- z ≡ 1+2i    ↦ -1
  | 2, 1 => ⟨0, -1⟩   -- z ≡ 2+i     ↦ -i
  | 3, 2 => ⟨1, 0⟩    -- z ≡ -1-2i   ↦ 1
  | 2, 3 => ⟨0, 1⟩    -- z ≡ 2-i     ↦ i
  | _, _ => 0         -- even classes (unused)

@[simp] theorem four_re : (4 : GInt).re = 4 := rfl
@[simp] theorem four_im : (4 : GInt).im = 0 := rfl

/-- Divisibility by `4` is componentwise. -/
theorem four_dvd_iff (x : GInt) : (4 : GInt) ∣ x ↔ x.re % 4 = 0 ∧ x.im % 4 = 0 := by
  constructor
  · rintro ⟨q, rfl⟩
    simp only [mul_re, mul_im, four_re, four_im]
    omega
  · intro h
    refine ⟨⟨x.re / 4, x.im / 4⟩, ?_⟩
    ext <;> simp only [mul_re, mul_im, four_re, four_im] <;> omega

instance (x : GInt) : Decidable ((4 : GInt) ∣ x) :=
  decidable_of_iff _ (four_dvd_iff x).symm

/-- Whether `4 ∣ α z + d` depends only on `z mod 4`. -/
theorem four_dvd_class (z r d : GInt) (h : (4 : GInt) ∣ z - r) :
    (4 : GInt) ∣ alphaH * z + d ↔ (4 : GInt) ∣ alphaH * r + d := by
  rw [four_dvd_iff] at h
  rw [four_dvd_iff, four_dvd_iff]
  simp [alphaH] at h ⊢
  omega

/-- The digit table, as (class representative, digit) pairs. -/
def tableH : List (GInt × GInt) :=
  [(⟨1, 0⟩, ⟨-1, 2⟩), (⟨0, 1⟩, ⟨-2, -1⟩), (⟨-1, 0⟩, ⟨1, -2⟩), (⟨0, -1⟩, ⟨2, 1⟩),
   (⟨1, 2⟩, ⟨-1, 0⟩), (⟨2, 1⟩, ⟨0, -1⟩), (⟨-1, -2⟩, ⟨1, 0⟩), (⟨2, -1⟩, ⟨0, 1⟩)]

/-- Kernel check on representatives: each listed digit makes `(5+2i) r + d` divisible by 4,
the representatives are odd, and `digitH` returns the listed digit. -/
theorem tableH_reps :
    ∀ p ∈ tableH, (4 : GInt) ∣ alphaH * p.1 + p.2 ∧ ¬ pi ∣ p.1 ∧ digitH p.1 = p.2 := by
  decide

/-- The representatives are pairwise incongruent modulo 4, so they are the 8 odd classes. -/
theorem tableH_distinct :
    ∀ p ∈ tableH, ∀ q ∈ tableH, (4 : GInt) ∣ p.1 - q.1 → p = q := by
  decide

/-- Every odd `z` is congruent modulo 4 to a representative in the table. -/
theorem tableH_covers (z : GInt) (hz : Odd z) : ∃ p ∈ tableH, (4 : GInt) ∣ z - p.1 := by
  unfold Odd at hz; rw [pi_dvd_iff] at hz
  have ha : z.re % 4 = 0 ∨ z.re % 4 = 1 ∨ z.re % 4 = 2 ∨ z.re % 4 = 3 := by omega
  have hb : z.im % 4 = 0 ∨ z.im % 4 = 1 ∨ z.im % 4 = 2 ∨ z.im % 4 = 3 := by omega
  simp only [tableH, List.mem_cons, List.not_mem_nil, or_false, exists_eq_or_imp,
    exists_eq_left, four_dvd_iff, sub_re, sub_im]
  omega

/-- The digit rule of `H` is correct for every odd `z`: `4 ∣ (5+2i) z + d(z)`. -/
theorem digitH_correct (z : GInt) (hz : Odd z) : (4 : GInt) ∣ alphaH * z + digitH z := by
  unfold Odd at hz; rw [pi_dvd_iff] at hz
  have ha : z.re % 4 = 0 ∨ z.re % 4 = 1 ∨ z.re % 4 = 2 ∨ z.re % 4 = 3 := by omega
  have hb : z.im % 4 = 0 ∨ z.im % 4 = 1 ∨ z.im % 4 = 2 ∨ z.im % 4 = 3 := by omega
  rw [four_dvd_iff]
  rcases ha with ha | ha | ha | ha <;> rcases hb with hb | hb | hb | hb <;>
    simp only [digitH, ha, hb, add_re, add_im, mul_re, mul_im, alphaH] <;> omega

/-- The rule `H`. -/
def H (z : GInt) : GInt := if pi ∣ z then divPi z else alphaH * z + digitH z

/-- `1 → -1 → 1` is a cycle of the accelerated map: from `1` one odd step and then exactly
five divisions by `π` give `-1` (all intermediate values even), and symmetrically from `-1`. -/
theorem H_cycle :
    ¬ pi ∣ (1 : GInt) ∧ ¬ pi ∣ (-1 : GInt) ∧
    H 1 = ⟨4, 4⟩ ∧ (∀ j ∈ [1, 2, 3, 4, 5], pi ∣ iter H j 1) ∧ iter H 6 1 = -1 ∧
    H (-1) = ⟨-4, -4⟩ ∧ (∀ j ∈ [1, 2, 3, 4, 5], pi ∣ iter H j (-1)) ∧ iter H 6 (-1) = 1 := by
  decide

end GInt
