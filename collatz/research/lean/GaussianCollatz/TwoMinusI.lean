import GaussianCollatz.Variants
/-!
# Soundness of the check without symmetry, and the expression `α = 2 - i` at modulus `π²`
(Section 6 of `theorems.md`, Remark 3.2)
-/
namespace GInt

theorem pi2_dvd_iff (x : GInt) : pow pi 2 ∣ x ↔ x.re % 2 = 0 ∧ x.im % 2 = 0 := by
  have hp : pow pi 2 = ⟨0, 2⟩ := by decide
  rw [hp]
  constructor
  · rintro ⟨q, rfl⟩; constructor <;> simp <;> omega
  · intro h
    exact ⟨⟨x.im / 2, -(x.re / 2)⟩, by ext <;> simp <;> omega⟩

theorem unit_of_N_one {x : GInt} (h : N x = 1) : IsUnit x := by
  have h' := h
  unfold N at h'
  have ha : x.re.natAbs ≤ 1 := by
    apply Decidable.byContradiction; intro hc
    have h2 : 2 ≤ x.re.natAbs := by omega
    have := Nat.mul_le_mul h2 h2; omega
  have hb : x.im.natAbs ≤ 1 := by
    apply Decidable.byContradiction; intro hc
    have h2 : 2 ≤ x.im.natAbs := by omega
    have := Nat.mul_le_mul h2 h2; omega
  obtain ⟨a, b⟩ := x
  simp only at ha hb
  rcases (by omega : a = -1 ∨ a = 0 ∨ a = 1) with rfl | rfl | rfl <;>
  rcases (by omega : b = -1 ∨ b = 0 ∨ b = 1) with rfl | rfl | rfl <;> revert h <;> decide

/-- **Theorem 6.1.** Soundness of the all-quadrant drop-below-start check (`gstopall.c`):
no relation between `T (u z)` and `T z` is assumed. -/
theorem drop_below_start_sound_all (T : GInt → GInt)
    (hdiv : ∀ z, pi ∣ z → T z = divPi z) (N0 B : Nat)
    (hcheck : ∀ z : GInt, Odd z → N0 < N z → N z ≤ B → ∃ n, N (iter T n z) < N z) :
    ∀ z : GInt, N z ≤ B → ∃ n, N (iter T n z) ≤ N0 := by
  have step : ∀ z : GInt, N0 < N z → N z ≤ B → ∃ n, N (iter T n z) < N z := by
    intro z hlo hhi
    by_cases he : pi ∣ z
    · refine ⟨1, ?_⟩
      simp only [iter]; rw [hdiv z he]
      have := N_divPi z he
      omega
    · exact hcheck z he hlo hhi
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

/-- The digit table of `α = 2 - i` at modulus `π²`: `i` for `a` odd, `b` even;
`1` for `a` even, `b` odd. -/
def digit2 (z : GInt) : GInt :=
  match z.re % 2, z.im % 2 with
  | 1, 0 => I
  | 0, 1 => 1
  | _, _ => 0

/-- **Proposition 6.2(1).** The table depends only on the class modulo `π²`. -/
theorem digit2_class : ∀ z w, pow pi 2 ∣ z - w → digit2 z = digit2 w := by
  intro z w h
  rw [pi2_dvd_iff] at h
  simp only [sub_re, sub_im] at h
  have h1 : z.re % 2 = w.re % 2 := by omega
  have h2 : z.im % 2 = w.im % 2 := by omega
  unfold digit2; rw [h1, h2]

/-- **Proposition 6.2(1).** The table is correct: `π² ∣ (2 - i) z + d(z)` for odd `z`. -/
theorem digit2_correct : ∀ z, Odd z → pow pi 2 ∣ (⟨2, -1⟩ : GInt) * z + digit2 z := by
  intro z hz
  unfold Odd at hz; rw [pi_dvd_iff] at hz
  rw [pi2_dvd_iff]
  rcases (by omega : (z.re % 2 = 1 ∧ z.im % 2 = 0) ∨ (z.re % 2 = 0 ∧ z.im % 2 = 1)) with
    ⟨h1, h2⟩ | ⟨h1, h2⟩
  · have : digit2 z = I := by unfold digit2; rw [h1, h2]; rfl
    rw [this]; simp; omega
  · have : digit2 z = 1 := by unfold digit2; rw [h1, h2]; rfl
    rw [this]; simp; omega

/-- The expression `E₂`. -/
def E2 : Expr where
  α := ⟨2, -1⟩
  k := 2
  d := digit2
  hα := by decide
  hclass := digit2_class
  hdig := digit2_correct

/-- **Proposition 6.2(2).** `T` divides by `π` on even inputs. -/
theorem E2_T_div : ∀ z, pi ∣ z → E2.T z = divPi z := by
  intro z h; unfold Expr.T; rw [ite_eq_left h]

/-- **Proposition 6.2(3).** `1 → -i → -1 → 1` is a cycle of `S`, and `S(i) = -i`. -/
theorem E2_cycle : E2.S 1 = -I ∧ E2.S (-I) = -1 ∧ E2.S (-1) = 1 ∧ E2.S I = -I := by
  refine ⟨E2.S_eq 2 (by decide) (by decide) (by decide),
    E2.S_eq 2 (by decide) (by decide) (by decide),
    E2.S_eq 3 (by decide) (by decide) (by decide),
    E2.S_eq 3 (by decide) (by decide) (by decide)⟩

/-- **Remark 3.2.** `i` is not periodic, and `E₂` is not symmetric. -/
theorem E2_I_not_periodic : ¬ Periodic E2.S I := by
  obtain ⟨h1, h2, h3, h4⟩ := E2_cycle
  have hP : ∀ m x, (x = 1 ∨ x = -I ∨ x = -1) → (iter E2.S m x = 1 ∨ iter E2.S m x = -I ∨
      iter E2.S m x = -1) := by
    intro m
    induction m with
    | zero => intro x hx; exact hx
    | succ m ih =>
      intro x hx
      apply ih
      rcases hx with rfl | rfl | rfl
      · exact Or.inr (Or.inl h1)
      · exact Or.inr (Or.inr h2)
      · exact Or.inl h3
  rintro ⟨p, hp, hpI⟩
  obtain ⟨q, rfl⟩ : ∃ q, p = q + 1 := ⟨p - 1, by omega⟩
  have := hP q (E2.S I) (by rw [h4]; exact Or.inr (Or.inl rfl))
  change iter E2.S q (E2.S I) = I at hpI
  rw [hpI] at this
  revert this; decide

theorem E2_not_symmetric : ¬ E2.Symmetric := by
  intro hs
  have := hs (-1) 1 (by decide) (by decide)
  revert this; decide

/-- **Proposition 6.2(4).** `T z ≠ 0` for `z ≠ 0`. -/
theorem E2_T_ne_zero : ∀ z, z ≠ 0 → E2.T z ≠ 0 := by
  intro z hz h
  unfold Expr.T at h
  by_cases he : pi ∣ z
  · rw [ite_eq_left he] at h
    apply hz; rw [← divPi_mul z he, h]; exact mul_zero' pi
  · rw [ite_eq_right he] at h
    change (⟨2, -1⟩ : GInt) * z + digit2 z = 0 at h
    rw [pi_dvd_iff] at he
    have hd : digit2 z = I ∨ digit2 z = 1 := by
      rcases (by omega : (z.re % 2 = 1 ∧ z.im % 2 = 0) ∨ (z.re % 2 = 0 ∧ z.im % 2 = 1)) with
        ⟨h1, h2⟩ | ⟨h1, h2⟩
      · left; unfold digit2; rw [h1, h2]; rfl
      · right; unfold digit2; rw [h1, h2]; rfl
    have e : (⟨2, -1⟩ : GInt) * z = -digit2 z := by
      have a1 := congrArg GInt.re h
      have a2 := congrArg GInt.im h
      simp only [add_re, add_im, zero_re, zero_im] at a1 a2
      ext <;> simp only [neg_re, neg_im] <;> omega
    have hN := congrArg N e
    rw [N_mul, show N (⟨2, -1⟩ : GInt) = 5 by decide] at hN
    rcases hd with hd | hd <;> rw [hd] at hN
    · rw [show N (-I) = 1 by decide] at hN; omega
    · rw [show N (-1 : GInt) = 1 by decide] at hN; omega

/-- **Corollary 6.3.** If the all-quadrant check holds for `E₂` with `N0 = 1` up to `B`, every
nonzero `z` with `N z ≤ B` reaches the unit cycle `{1, -i, -1}` under `T`. -/
theorem E2_sound (B : Nat)
    (hcheck : ∀ z : GInt, Odd z → 1 < N z → N z ≤ B → ∃ n, N (iter E2.T n z) < N z) :
    ∀ z : GInt, z ≠ 0 → N z ≤ B →
      ∃ n, iter E2.T n z = 1 ∨ iter E2.T n z = -I ∨ iter E2.T n z = -1 := by
  intro z hz hB
  obtain ⟨n, hn⟩ := drop_below_start_sound_all E2.T E2_T_div 1 B hcheck z hB
  have hne : ∀ m x, x ≠ 0 → iter E2.T m x ≠ 0 := by
    intro m
    induction m with
    | zero => intro x hx; exact hx
    | succ m ih => intro x hx; exact ih _ (E2_T_ne_zero x hx)
  have h0 := hne n z hz
  have h1 : N (iter E2.T n z) = 1 := by
    have := N_pos h0; omega
  rcases unit_of_N_one h1 with h | h | h | h
  · exact ⟨n, Or.inl h⟩
  · refine ⟨n + 4, Or.inr (Or.inl ?_)⟩
    rw [iter_add, h]; decide
  · exact ⟨n, Or.inr (Or.inr h)⟩
  · exact ⟨n, Or.inr (Or.inl h)⟩

end GInt
