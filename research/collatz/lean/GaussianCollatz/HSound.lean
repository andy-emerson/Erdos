import GaussianCollatz.Soundness
import GaussianCollatz.RuleH
/-!
# The drop-below-start check applied to the rule `H`

`H` divides by `π` on even inputs and commutes with units (its digit set is closed under
units), so `drop_below_start_sound` applies with `T := H`.
-/
namespace GInt

/-- `H z = z / π` when `π ∣ z`. -/
theorem H_div : ∀ z, pi ∣ z → H z = divPi z := by
  intro z h; unfold H; rw [ite_eq_left h]

/-- `digitH` depends only on `(re mod 4, im mod 4)`. -/
theorem digitH_mod (z : GInt) : digitH z = digitH ⟨z.re % 4, z.im % 4⟩ := by
  unfold digitH; simp only [Int.emod_emod]

/-- Multiplication by a unit is compatible with reduction mod 4 componentwise. -/
theorem unit_mul_mod {u : GInt} (hu : IsUnit u) (z : GInt) :
    (u * z).re % 4 = (u * ⟨z.re % 4, z.im % 4⟩).re % 4 ∧
    (u * z).im % 4 = (u * ⟨z.re % 4, z.im % 4⟩).im % 4 := by
  rcases hu with rfl | rfl | rfl | rfl <;> simp <;> omega

/-- Finite kernel check: on the 8 odd residue pairs `(a, b)` with `0 ≤ a, b < 4`, every unit
carries the digit of the class of `a + bi` to the digit of the image class. -/
theorem digitH_unit_table :
    ∀ a ∈ [(0 : Int), 1, 2, 3], ∀ b ∈ [(0 : Int), 1, 2, 3], (a + b) % 2 = 1 →
    ∀ u ∈ [(1 : GInt), I, -1, -I], digitH (u * ⟨a, b⟩) = u * digitH ⟨a, b⟩ := by
  decide

/-- The digit set of `H` is closed under units: `d(u z) = u d(z)` for odd `z`. -/
theorem digitH_unit {u : GInt} (hu : IsUnit u) {z : GInt} (hz : Odd z) :
    digitH (u * z) = u * digitH z := by
  unfold Odd at hz; rw [pi_dvd_iff] at hz
  obtain ⟨h1, h2⟩ := unit_mul_mod hu z
  rw [digitH_mod (u * z), h1, h2, ← digitH_mod (u * _), digitH_mod z]
  have hA : z.re % 4 ∈ [(0 : Int), 1, 2, 3] := by simp; omega
  have hB : z.im % 4 ∈ [(0 : Int), 1, 2, 3] := by simp; omega
  have hu' : u ∈ [(1 : GInt), I, -1, -I] := by
    rcases hu with h | h | h | h <;> simp [h]
  exact digitH_unit_table _ hA _ hB (by omega) u hu'

/-- Division by `π` commutes with units. -/
theorem divPi_unit_mul {u : GInt} (hu : IsUnit u) {z : GInt} (hz : pi ∣ z) :
    divPi (u * z) = u * divPi z := by
  rw [pi_dvd_iff] at hz
  rcases hu with rfl | rfl | rfl | rfl <;> ext <;> simp [divPi] <;> omega

theorem pi_dvd_unit_mul {u z : GInt} (hz : pi ∣ z) : pi ∣ u * z := by
  obtain ⟨q, rfl⟩ := hz
  exact ⟨u * q, by rw [← mul_assoc, mul_comm u pi, mul_assoc]⟩

/-- `H (u z) = u H(z)` for every unit `u`. -/
theorem H_unit : ∀ u z, IsUnit u → H (u * z) = u * H z := by
  intro u z hu
  by_cases hz : pi ∣ z
  · unfold H
    rw [ite_eq_left (pi_dvd_unit_mul hz), ite_eq_left hz, divPi_unit_mul hu hz]
  · have huz : ¬ pi ∣ u * z := odd_unit_mul hu hz
    unfold H
    rw [ite_eq_right huz, ite_eq_right hz, digitH_unit hu hz]
    ext <;> simp <;> grind

/-- **Soundness of the drop-below-start check for `H`.** If every odd `z = a + bi` with
`a > 0`, `b ≥ 0` and `N0 < N z ≤ B` has an iterate of `H` of smaller norm, then every `z`
with `N z ≤ B` has an iterate of `H` of norm at most `N0`. -/
theorem H_sound (N0 B : Nat)
    (hcheck : ∀ z : GInt, Odd z → 0 < z.re → 0 ≤ z.im → N0 < N z → N z ≤ B →
      ∃ n, N (iter H n z) < N z) :
    ∀ z : GInt, N z ≤ B → ∃ n, N (iter H n z) ≤ N0 :=
  drop_below_start_sound H H_div H_unit N0 B hcheck

end GInt
