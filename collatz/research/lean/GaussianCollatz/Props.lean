import GaussianCollatz.Basic
/-!
# Propositions 1, 3 and 5(2) of `papers/gaussian-collatz.md`
-/
namespace GInt

/-- A square of an integer is `0`, `1` or at least `4`. -/
theorem sq_cases (a : Int) : a * a = 0 ∨ a * a = 1 ∨ 4 ≤ a * a := by
  rw [← Int.natAbs_mul_self]
  generalize a.natAbs = n
  match n with
  | 0 => simp
  | 1 => simp
  | n + 2 =>
    right; right
    have : 2 * 2 ≤ (n + 2) * (n + 2) := Nat.mul_le_mul (by omega) (by omega)
    omega

/-- **Proposition 3.** `a² + b² ≠ 3`. -/
theorem prop3_int (a b : Int) : a * a + b * b ≠ 3 := by
  rcases sq_cases a with h | h | h <;> rcases sq_cases b with h' | h' | h' <;> omega

/-- **Proposition 3.** No Gaussian integer has norm 3. -/
theorem prop3 (z : GInt) : N z ≠ 3 := by
  intro h
  apply prop3_int z.re z.im
  rw [← N_cast, h]; rfl

theorem pow_dvd_pow (z : GInt) {j k : Nat} (h : j ≤ k) : pow z j ∣ pow z k := by
  refine ⟨pow z (k - j), ?_⟩
  rw [← pow_add, Nat.add_sub_cancel' h]

theorem odd_one : Odd (1 : GInt) := by
  unfold Odd; rw [pi_dvd_iff]; decide

theorem not_odd_zero : ¬ Odd (0 : GInt) := by
  unfold Odd; rw [pi_dvd_iff]; decide

/-- `π² ∣ 2z`, hence `-z ≡ z (mod π²)`; the case `z = 1` is all we need. -/
theorem pi_sq_dvd_neg_two : pow pi 2 ∣ (-1 - 1 : GInt) := ⟨I, by decide⟩

theorem pi_sq_dvd_two_mul (z : GInt) : pow pi 2 ∣ z - (-z) :=
  ⟨⟨z.im, -z.re⟩, by ext <;> simp [pow] <;> omega⟩

/-- **Proposition 1** (levels `k ≤ 2`, in particular `k = 1, 2`): if the digit depends only
on the class modulo `π^k`, digits of odd numbers are odd, and `π^k ∣ α z + d z` for odd `z`,
then the digit function does not commute with `-1` on odd numbers. -/
theorem prop1 (k : Nat) (hk : k ≤ 2) (α : GInt) (d : GInt → GInt)
    (hclass : ∀ z w, pow pi k ∣ z - w → d z = d w)
    (hodd : ∀ z, Odd z → Odd (d z))
    (_hdiv : ∀ z, Odd z → pow pi k ∣ α * z + d z) :
    ∃ z, Odd z ∧ d (-z) ≠ -(d z) := by
  refine ⟨1, odd_one, ?_⟩
  intro hneg
  have hsame : d (-1) = d 1 :=
    hclass (-1) 1 (dvd_trans (pow_dvd_pow pi hk) pi_sq_dvd_neg_two)
  have h0 : d 1 = 0 := by
    rw [hsame] at hneg
    ext
    · have := congrArg GInt.re hneg; simp at this; simp; omega
    · have := congrArg GInt.im hneg; simp at this; simp; omega
  exact not_odd_zero (h0 ▸ hodd 1 odd_one)

/-- `π³ = -2 + 2i` does not divide `10`. -/
theorem pi_cube_not_dvd_ten : ¬ pow pi 3 ∣ (10 : GInt) := by
  rintro ⟨q, hq⟩
  have h1 := congrArg GInt.re hq
  have h2 := congrArg GInt.im hq
  have hp : pow pi 3 = ⟨-2, 2⟩ := by decide
  rw [hp] at h1 h2
  simp at h1 h2
  change (10 : Int) = _ at h1
  change (0 : Int) = _ at h2
  omega

/-- **Proposition 5(2)**, core fact: if `π^k ∣ 10` then `k ≤ 2`. -/
theorem prop5_2 (k : Nat) (h : pow pi k ∣ 10) : k ≤ 2 := by
  apply Decidable.byContradiction
  intro hk
  exact pi_cube_not_dvd_ten (dvd_trans (pow_dvd_pow pi (by omega : 3 ≤ k)) h)

/-- **Proposition 5(2)**: if the odd step at every positive odd integer `n` is `n ↦ 3n+1` at
level `k`, i.e. `π^k ∣ 3n+1`, then `k ≤ 2` (take `n = 3`). -/
theorem prop5_2' (k : Nat)
    (h : ∀ n : Nat, n % 2 = 1 → pow pi k ∣ (⟨3 * (n : Int) + 1, 0⟩ : GInt)) : k ≤ 2 :=
  prop5_2 k (h 3 rfl)

end GInt
