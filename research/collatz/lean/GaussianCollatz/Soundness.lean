import GaussianCollatz.Basic
/-!
# Soundness of the drop-below-start check (Section 12)

If every odd first-quadrant start `z = a + bi` (`a > 0`, `b ≥ 0`) with `N0 < N z ≤ B` eventually
drops below its own norm, then every `z` with `N z ≤ B` (zero, even, any associate) eventually
reaches norm at most `N0`.
-/
namespace GInt

/-- `N(π w) = 2 N(w)`. -/
theorem N_pi_mul (w : GInt) : N (pi * w) = 2 * N w := by
  apply Int.ofNat.inj
  simp only [Int.ofNat_eq_natCast, Int.natCast_mul, N_cast, mul_re, mul_im, pi_re, pi_im]
  grind

/-- `N(z / π) * 2 = N(z)` when `π ∣ z`. -/
theorem N_divPi (z : GInt) (h : pi ∣ z) : N (divPi z) * 2 = N z := by
  conv => rhs; rw [← divPi_mul z h]
  rw [N_pi_mul, Nat.mul_comm]

/-- Norms are invariant under units. -/
theorem N_unit_mul {u : GInt} (hu : IsUnit u) (z : GInt) : N (u * z) = N z := by
  apply Int.ofNat.inj
  simp only [Int.ofNat_eq_natCast, N_cast]
  rcases hu with rfl | rfl | rfl | rfl <;> simp <;> grind

/-- Unit multiples of odd elements are odd. -/
theorem odd_unit_mul {u : GInt} (hu : IsUnit u) {z : GInt} (hz : Odd z) : Odd (u * z) := by
  unfold Odd at *
  rw [pi_dvd_iff] at *
  rcases hu with rfl | rfl | rfl | rfl <;> simp <;> omega

theorem odd_ne_zero {z : GInt} (hz : Odd z) : z ≠ 0 := by
  rintro rfl; exact hz ⟨0, by decide⟩

/-- Every nonzero `z` has a unit multiple `a + bi` with `a > 0`, `b ≥ 0`. -/
theorem exists_unit_primary (z : GInt) (hz : z ≠ 0) :
    ∃ u, IsUnit u ∧ 0 < (u * z).re ∧ 0 ≤ (u * z).im := by
  have hz' : z.re ≠ 0 ∨ z.im ≠ 0 := by
    apply Decidable.byContradiction; intro h; apply hz; ext <;> simp <;> omega
  by_cases h1 : 0 < z.re ∧ 0 ≤ z.im
  · exact ⟨1, Or.inl rfl, by simp; omega⟩
  by_cases h2 : z.re ≤ 0 ∧ 0 < z.im
  · exact ⟨-I, Or.inr (Or.inr (Or.inr rfl)), by simp; omega⟩
  by_cases h3 : z.re < 0 ∧ z.im ≤ 0
  · exact ⟨-1, Or.inr (Or.inr (Or.inl rfl)), by simp; omega⟩
  · exact ⟨I, Or.inr (Or.inl rfl), by simp; omega⟩

/-- `T^n (u z) = u T^n z` for a unit `u`, if `T` commutes with units. -/
theorem iter_unit_mul (T : GInt → GInt) (hunit : ∀ u z, IsUnit u → T (u * z) = u * T z)
    {u : GInt} (hu : IsUnit u) (n : Nat) (z : GInt) :
    iter T n (u * z) = u * iter T n z := by
  induction n generalizing z with
  | zero => rfl
  | succ n ih => simp only [iter]; rw [hunit u z hu, ih]

/-- **Soundness of the drop-below-start check.** -/
theorem drop_below_start_sound (T : GInt → GInt)
    (hdiv : ∀ z, pi ∣ z → T z = divPi z)
    (hunit : ∀ u z, IsUnit u → T (u * z) = u * T z)
    (N0 B : Nat)
    (hcheck : ∀ z : GInt, Odd z → 0 < z.re → 0 ≤ z.im → N0 < N z → N z ≤ B →
      ∃ n, N (iter T n z) < N z) :
    ∀ z : GInt, N z ≤ B → ∃ n, N (iter T n z) ≤ N0 := by
  -- one step of descent, for every `z` (even or odd, any associate)
  have step : ∀ z : GInt, N0 < N z → N z ≤ B → ∃ n, N (iter T n z) < N z := by
    intro z hlo hhi
    by_cases he : pi ∣ z
    · refine ⟨1, ?_⟩
      simp only [iter]; rw [hdiv z he]
      have := N_divPi z he
      omega
    · obtain ⟨u, hu, hre, him⟩ := exists_unit_primary z (odd_ne_zero he)
      rw [← N_unit_mul hu z] at hlo hhi
      obtain ⟨n, hn⟩ := hcheck (u * z) (odd_unit_mul hu he) hre him hlo hhi
      refine ⟨n, ?_⟩
      rw [iter_unit_mul T hunit hu, N_unit_mul hu, N_unit_mul hu] at hn
      exact hn
  -- strong induction on the norm
  have main : ∀ m, ∀ z : GInt, N z = m → N z ≤ B → ∃ n, N (iter T n z) ≤ N0 := by
    intro m
    induction m using Nat.strongRecOn with
    | ind m ih =>
      intro z hm hB
      by_cases hle : N z ≤ N0
      · exact ⟨0, hle⟩
      · obtain ⟨n, hn⟩ := step z (by omega) hB
        obtain ⟨n', hn'⟩ := ih _ (by omega) (iter T n z) rfl (by omega)
        exact ⟨n + n', by rw [iter_add]; exact hn'⟩
  intro z hB
  exact main _ z rfl hB

end GInt
