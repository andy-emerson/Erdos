import GaussianCollatz.Variants
import GaussianCollatz.RuleH
/-!
# No symmetric rule inside the window below norm 29 (Section 5 of `theorems.md`)
-/
namespace GInt

/-! ## Helpers -/

theorem unit_of_unit_mul {c x : GInt} (hc : IsUnit c) (h : IsUnit (c * x)) : IsUnit x := by
  have := unit_mul (unit_conj hc) h
  rwa [← mul_assoc, conj_mul_self hc, one_mul] at this

theorem dvd8_of_pi3 {x : GInt} (h : pow pi 3 ∣ x) : 8 ∣ N x := by
  obtain ⟨q, rfl⟩ := h
  rw [N_mul, N_pow_pi]; exact ⟨N q, rfl⟩

/-- Distinct units are incongruent modulo `π^3`. -/
theorem units_mod_pi3 {a b : GInt} (ha : IsUnit a) (hb : IsUnit b) (h : pow pi 3 ∣ a - b) :
    a = b := by
  have h8 := dvd8_of_pi3 h
  rcases ha with rfl | rfl | rfl | rfl <;> rcases hb with rfl | rfl | rfl | rfl <;>
    first | rfl | (exfalso; revert h8; decide)

theorem mem_units {u : GInt} (h : u ∈ [(1 : GInt), I, -1, -I]) : IsUnit u := by
  simp only [List.mem_cons, List.not_mem_nil, or_false] at h
  exact h

/-- Bounds on the coordinates from the norm. -/
theorem box5 (α : GInt) (h : N α < 29) :
    -5 ≤ α.re ∧ α.re ≤ 5 ∧ -5 ≤ α.im ∧ α.im ≤ 5 := by
  unfold N at h
  have h1 : α.re.natAbs ≤ 5 := by
    apply Decidable.byContradiction; intro hc
    have h6 : 6 ≤ _ := Nat.lt_of_not_le hc
    have := Nat.mul_le_mul h6 h6
    omega
  have h2 : α.im.natAbs ≤ 5 := by
    apply Decidable.byContradiction; intro hc
    have h6 : 6 ≤ _ := Nat.lt_of_not_le hc
    have := Nat.mul_le_mul h6 h6
    omega
  omega

/-- **Lemma 5.3**, modulus `π^3`: odd `α` with `8 < N α < 16` is an associate of
`3`, `3 + 2i` or `3 - 2i`. -/
theorem alpha_k3 (α : GInt) (hodd : Odd α) (hlo : 8 < N α) (hhi : N α < 16) :
    ∃ u, u ∈ [(1 : GInt), I, -1, -I] ∧
      (α = u * ⟨3, 0⟩ ∨ α = u * ⟨3, 2⟩ ∨ α = u * ⟨3, -2⟩) := by
  obtain ⟨b1, b2, b3, b4⟩ := box5 α (by omega)
  have hlo' : (8 : Int) < α.re * α.re + α.im * α.im := by rw [← N_cast]; exact_mod_cast hlo
  have hhi' : α.re * α.re + α.im * α.im < 16 := by rw [← N_cast]; exact_mod_cast hhi
  have ho : (α.re - α.im) % 2 ≠ 0 := by intro h; exact hodd ((pi_dvd_iff α).2 h)
  have key : ∀ a b : Int, -5 ≤ a → a ≤ 5 → -5 ≤ b → b ≤ 5 → (a - b) % 2 ≠ 0 →
      8 < a * a + b * b → a * a + b * b < 16 →
      ∃ u, u ∈ [(1 : GInt), I, -1, -I] ∧
        ((⟨a, b⟩ : GInt) = u * ⟨3, 0⟩ ∨ (⟨a, b⟩ : GInt) = u * ⟨3, 2⟩ ∨
          (⟨a, b⟩ : GInt) = u * ⟨3, -2⟩) := by
    intro a b h1 h2 h3 h4
    rcases (by omega : a = -5 ∨ a = -4 ∨ a = -3 ∨ a = -2 ∨ a = -1 ∨ a = 0 ∨ a = 1 ∨ a = 2 ∨ a = 3 ∨ a = 4 ∨ a = 5) with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;>
    rcases (by omega : b = -5 ∨ b = -4 ∨ b = -3 ∨ b = -2 ∨ b = -1 ∨ b = 0 ∨ b = 1 ∨ b = 2 ∨ b = 3 ∨ b = 4 ∨ b = 5) with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;> decide
  have := key α.re α.im b1 b2 b3 b4 ho hlo' hhi'
  simpa using this

/-- **Lemma 5.3**, modulus `π^4`: odd `α` with `16 < N α < 29` is an associate of
`4 + i`, `4 - i`, `5`, `4 + 3i` or `4 - 3i`. -/
theorem alpha_k4 (α : GInt) (hodd : Odd α) (hlo : 16 < N α) (hhi : N α < 29) :
    ∃ u, u ∈ [(1 : GInt), I, -1, -I] ∧
      (α = u * ⟨4, 1⟩ ∨ α = u * ⟨4, -1⟩ ∨ α = u * ⟨5, 0⟩ ∨ α = u * ⟨4, 3⟩ ∨
        α = u * ⟨4, -3⟩) := by
  obtain ⟨b1, b2, b3, b4⟩ := box5 α hhi
  have hlo' : (16 : Int) < α.re * α.re + α.im * α.im := by rw [← N_cast]; exact_mod_cast hlo
  have hhi' : α.re * α.re + α.im * α.im < 29 := by rw [← N_cast]; exact_mod_cast hhi
  have ho : (α.re - α.im) % 2 ≠ 0 := by intro h; exact hodd ((pi_dvd_iff α).2 h)
  have key : ∀ a b : Int, -5 ≤ a → a ≤ 5 → -5 ≤ b → b ≤ 5 → (a - b) % 2 ≠ 0 →
      16 < a * a + b * b → a * a + b * b < 29 →
      ∃ u, u ∈ [(1 : GInt), I, -1, -I] ∧
        ((⟨a, b⟩ : GInt) = u * ⟨4, 1⟩ ∨ (⟨a, b⟩ : GInt) = u * ⟨4, -1⟩ ∨
          (⟨a, b⟩ : GInt) = u * ⟨5, 0⟩ ∨ (⟨a, b⟩ : GInt) = u * ⟨4, 3⟩ ∨
          (⟨a, b⟩ : GInt) = u * ⟨4, -3⟩) := by
    intro a b h1 h2 h3 h4
    rcases (by omega : a = -5 ∨ a = -4 ∨ a = -3 ∨ a = -2 ∨ a = -1 ∨ a = 0 ∨ a = 1 ∨ a = 2 ∨ a = 3 ∨ a = 4 ∨ a = 5) with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;>
    rcases (by omega : b = -5 ∨ b = -4 ∨ b = -3 ∨ b = -2 ∨ b = -1 ∨ b = 0 ∨ b = 1 ∨ b = 2 ∨ b = 3 ∨ b = 4 ∨ b = 5) with rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl | rfl <;> decide
  have := key α.re α.im b1 b2 b3 b4 ho hlo' hhi'
  simpa using this

namespace Expr

/-- **Lemma 5.1.** If the digit `d z₀` equals `(ε π^v - α) z` with `v ≥ k`, `z` odd, then
`d z = d z₀` and `S z = ε z`. -/
theorem fixed_point (E : Expr) (v : Nat) (hv : E.k ≤ v) (ε : GInt) (hε : IsUnit ε)
    (z z0 : GInt) (hz : Odd z) (hz0 : Odd z0)
    (h : E.d z0 = (ε * pow pi v - E.α) * z) : E.d z = E.d z0 ∧ E.S z = ε * z := by
  have e1 : E.α * z + E.d z0 = pow pi v * (ε * z) := by
    rw [h]; ext <;> simp <;> grind
  have hk : pow pi E.k ∣ E.α * z + E.d z0 := by
    rw [e1]; exact dvd_trans (pow_dvd_pow pi hv) ⟨_, rfl⟩
  have h2 := dvd_sub' (E.hdig z0 hz0) hk
  have e2 : E.α * z0 + E.d z0 - (E.α * z + E.d z0) = E.α * (z0 - z) := by
    rw [mul_sub]; ext <;> simp <;> omega
  rw [e2] at h2
  have hd : E.d z0 = E.d z := E.hclass z0 z (pow_dvd_of_odd_mul E.hα E.k h2)
  refine ⟨hd.symm, ?_⟩
  exact E.S_eq v hz (by rw [← hd]; exact e1) (odd_unit_mul hε hz)

/-- **Lemma 5.1**, second part: if moreover `z` is not a unit and the expression is symmetric
or `ε = 1`, the expression is not a rule. -/
theorem fixed_point_not_rule (E : Expr) (v : Nat) (hv : E.k ≤ v) (ε : GInt) (hε : IsUnit ε)
    (hs : E.Symmetric ∨ ε = 1) (z z0 : GInt) (hz : Odd z) (hz0 : Odd z0)
    (h : E.d z0 = (ε * pow pi v - E.α) * z) (hzu : ¬ IsUnit z) : ¬ E.IsRule := by
  obtain ⟨_, hS⟩ := fixed_point E v hv ε hε z z0 hz hz0 h
  rcases hs with hs | rfl
  · apply not_trivial_of_invariant E.S (fun x => ∃ c, IsUnit c ∧ x = c * z) _ _ z hz
      ⟨1, Or.inl rfl, (one_mul z).symm⟩
    · rintro x ⟨c, hc, rfl⟩
      exact ⟨c * ε, unit_mul hc hε, by rw [E.S_commutes hs c z hc, hS, mul_assoc]⟩
    · rintro x ⟨c, hc, rfl⟩ hu
      exact hzu (unit_of_unit_mul hc hu)
  · apply not_trivial_of_invariant E.S (fun x => x = z) _ _ z hz rfl
    · rintro x rfl; rw [hS, one_mul]
    · rintro x rfl; exact hzu

/-- **Corollary 5.2.** If `ε π^v - α` is a unit (`v ≥ k ≥ 1`) and the expression is a rule,
symmetric or with `ε = 1`, then every digit is a unit. -/
theorem digits_units (E : Expr) (hk : 1 ≤ E.k) (v : Nat) (hv : E.k ≤ v) (ε : GInt)
    (hε : IsUnit ε) (hη : IsUnit (ε * pow pi v - E.α)) (hs : E.Symmetric ∨ ε = 1)
    (hr : E.IsRule) : ∀ z, Odd z → IsUnit (E.d z) := by
  intro z0 hz0
  apply Decidable.byContradiction
  intro hnu
  have hδ := E.d_odd hk z0 hz0
  have hz : Odd (conj (ε * pow pi v - E.α) * E.d z0) := odd_unit_mul (unit_conj hη) hδ
  have h : E.d z0 = (ε * pow pi v - E.α) * (conj (ε * pow pi v - E.α) * E.d z0) := by
    rw [← mul_assoc, mul_comm _ (conj _), conj_mul_self hη, one_mul]
  apply fixed_point_not_rule E v hv ε hε hs _ z0 hz hz0 h _ hr
  intro hu
  apply hnu
  rw [h]; exact unit_mul hη hu


/-- Modulus `π^3`: a certificate of a set of non-units, closed under `S` up to units, rules out
a rule once Corollary 5.2 forces the digits to be units. Entries `(z, e, v, c, z')` mean
`α z + e = π^v (c z')`. -/
theorem no_rule_of_cycle3 (E : Expr) (hs : E.Symmetric) (hk : E.k = 3) (α0 : GInt)
    (hα : E.α = α0) (ε : GInt) (v : Nat) (hv : 3 ≤ v) (hε : IsUnit ε)
    (hη : IsUnit (ε * pow pi v - α0)) (L : List (GInt × GInt × Nat × GInt × GInt))
    (hL : ∀ t ∈ L, Odd t.1 ∧ ¬ IsUnit t.1 ∧ IsUnit t.2.1 ∧ IsUnit t.2.2.2.1 ∧ 3 ≤ t.2.2.1 ∧
      α0 * t.1 + t.2.1 = pow pi t.2.2.1 * (t.2.2.2.1 * t.2.2.2.2) ∧ ∃ t' ∈ L, t'.1 = t.2.2.2.2)
    (t0 : GInt × GInt × Nat × GInt × GInt) (ht0 : t0 ∈ L) : ¬ E.IsRule := by
  intro hr
  subst hα
  have hdu := digits_units E (by omega) v (by omega) ε hε hη (Or.inl hs) hr
  have hstep : ∀ t ∈ L, E.S t.1 = t.2.2.2.1 * t.2.2.2.2 := by
    intro t ht
    obtain ⟨hz, _, he, hc, hv', heq, t', ht', ht'e⟩ := hL t ht
    have hz' : Odd t.2.2.2.2 := by rw [← ht'e]; exact (hL t' ht').1
    have hd : E.d t.1 = t.2.1 := by
      apply units_mod_pi3 (hdu _ hz) he
      have h1 := E.hdig t.1 hz
      rw [hk] at h1
      have h2 : pow pi 3 ∣ E.α * t.1 + t.2.1 := by
        rw [heq]; exact dvd_trans (pow_dvd_pow pi hv') ⟨_, rfl⟩
      have := dvd_sub' h1 h2
      have e : E.α * t.1 + E.d t.1 - (E.α * t.1 + t.2.1) = E.d t.1 - t.2.1 := by
        ext <;> simp <;> omega
      rwa [e] at this
    exact E.S_eq t.2.2.1 hz (by rw [hd]; exact heq) (odd_unit_mul hc hz')
  apply not_trivial_of_invariant E.S (fun x => ∃ c, IsUnit c ∧ ∃ t ∈ L, x = c * t.1) _ _
    t0.1 (hL t0 ht0).1 ⟨1, Or.inl rfl, t0, ht0, (one_mul _).symm⟩ hr
  · rintro x ⟨c, hc, t, ht, rfl⟩
    obtain ⟨_, _, _, hc', _, _, t', ht', ht'e⟩ := hL t ht
    refine ⟨c * t.2.2.2.1, unit_mul hc hc', t', ht', ?_⟩
    rw [E.S_commutes hs c _ hc, hstep t ht, ht'e, mul_assoc]
  · rintro x ⟨c, hc, t, ht, rfl⟩ hu
    exact (hL t ht).2.1 (unit_of_unit_mul hc hu)

/-- Modulus `π^4`: Corollary 5.2 forces unit digits, but `-α z₀` lies in a class without units. -/
theorem no_rule_k4 (E : Expr) (hs : E.Symmetric) (hk : E.k = 4) (α0 : GInt) (hα : E.α = α0)
    (ε : GInt) (v : Nat) (hv : 4 ≤ v) (hε : IsUnit ε) (hη : IsUnit (ε * pow pi v - α0))
    (z0 : GInt) (hz0 : Odd z0)
    (hno : ∀ e ∈ [(1 : GInt), I, -1, -I], ¬ (4 : GInt) ∣ α0 * z0 + e) : ¬ E.IsRule := by
  intro hr
  subst hα
  have hdu := digits_units E (by omega) v (by omega) ε hε hη (Or.inl hs) hr
  have h1 := E.hdig z0 hz0
  rw [hk, pi_pow_four] at h1
  obtain ⟨q, hq⟩ := h1
  apply hno (E.d z0) (by rcases hdu z0 hz0 with h | h | h | h <;> simp [h])
  exact ⟨-q, by rw [hq]; ext <;> simp <;> omega⟩

/-- Reduction to `α = α₀` by rotating with `conj u`. -/
theorem rotate_to (E : Expr) (u α0 : GInt) (hu : IsUnit u) (h : E.α = u * α0) :
    (E.rotate (conj u) (unit_conj hu)).α = α0 := by
  show conj u * E.α = α0
  rw [h, ← mul_assoc, conj_mul_self hu, one_mul]

/-- **Theorem 5.4.** No symmetric expression with `2^k < N(α) < 2^(k+1)` and `N(α) < 29`
is a rule, for any digit table. -/
theorem no_symmetric_rule_below_29 (E : Expr) (hs : E.Symmetric)
    (hlo : 2 ^ E.k < N E.α) (hhi : N E.α < 2 ^ (E.k + 1)) (h29 : N E.α < 29) :
    ¬ E.IsRule := by
  have hodd := E.hα
  have hk4 : E.k ≤ 4 := by
    apply Decidable.byContradiction; intro h
    have : 2 ^ 5 ≤ 2 ^ E.k := Nat.pow_le_pow_right (by decide) (by omega)
    omega
  rcases (by omega : E.k = 0 ∨ E.k = 1 ∨ E.k = 2 ∨ E.k = 3 ∨ E.k = 4) with
    hk | hk | hk | hk | hk <;> rw [hk] at hlo hhi
  · simp at hlo hhi; omega
  · exact absurd (by simp at hlo hhi; omega : N E.α = 3) (prop3 E.α)
  · exfalso
    have h1 : E.d (-1) = E.d 1 := E.hclass (-1) 1 (by rw [hk]; exact pi_sq_dvd_neg_two)
    have h2 := hs (-1) 1 (by decide) odd_one
    rw [show (-1 : GInt) * 1 = -1 by decide, h1] at h2
    have h0 : E.d 1 = 0 := by
      have a1 := congrArg GInt.re h2
      have a2 := congrArg GInt.im h2
      simp at a1 a2
      ext <;> simp <;> omega
    have := E.d_odd (by omega) 1 odd_one
    rw [h0] at this
    exact this ⟨0, by decide⟩
  · obtain ⟨u, hu, h⟩ := alpha_k3 E.α hodd (by simpa using hlo) (by simpa using hhi)
    have hu' := mem_units hu
    rw [← rotate_rule E hs (conj u) (unit_conj hu')]
    have hs' := rotate_symmetric E hs (conj u) (unit_conj hu')
    rcases h with h | h | h
    · exact no_rule_of_cycle3 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 4 (by decide) (by decide)
        (by decide) [(⟨-2, -1⟩, I, 3, 1, ⟨1, 2⟩), (⟨1, 2⟩, -1, 3, I, ⟨-2, -1⟩)] (by decide)
        _ (List.mem_cons_self ..)
    · exact no_rule_of_cycle3 _ hs' hk _ (rotate_to E u _ hu' h) (-I) 3 (by decide) (by decide)
        (by decide)
        [(⟨-3, 0⟩, -1, 3, 1, ⟨1, 4⟩), (⟨1, 4⟩, -1, 3, 1, ⟨5, -2⟩), (⟨5, -2⟩, 1, 5, 1, ⟨-3, 2⟩),
         (⟨-3, 2⟩, 1, 4, -1, ⟨-3, 0⟩)] (by decide) _ (List.mem_cons_self ..)
    · exact no_rule_of_cycle3 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 3 (by decide) (by decide)
        (by decide)
        [(⟨-3, 0⟩, -1, 3, 1, ⟨4, 1⟩), (⟨4, 1⟩, -I, 3, 1, ⟨-5, -2⟩), (⟨-5, -2⟩, -1, 5, 1, ⟨2, -3⟩),
         (⟨2, -3⟩, I, 4, -I, ⟨-3, 0⟩)] (by decide) _ (List.mem_cons_self ..)
  · obtain ⟨u, hu, h⟩ := alpha_k4 E.α hodd (by simpa using hlo) h29
    have hu' := mem_units hu
    rw [← rotate_rule E hs (conj u) (unit_conj hu')]
    have hs' := rotate_symmetric E hs (conj u) (unit_conj hu')
    rcases h with h | h | h | h | h
    · exact no_rule_k4 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 4 (by decide) (by decide)
        (by decide) ⟨1, 2⟩ (by decide) (by decide)
    · exact no_rule_k4 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 4 (by decide) (by decide)
        (by decide) ⟨1, 2⟩ (by decide) (by decide)
    · exact no_rule_k4 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 4 (by decide) (by decide)
        (by decide) ⟨1, 2⟩ (by decide) (by decide)
    · exact no_rule_k4 _ hs' hk _ (rotate_to E u _ hu' h) (-1) 5 (by decide) (by decide)
        (by decide) ⟨1, 2⟩ (by decide) (by decide)
    · exact no_rule_k4 _ hs' hk _ (rotate_to E u _ hu' h) I 5 (by decide) (by decide)
        (by decide) ⟨1, 2⟩ (by decide) (by decide)

end Expr
end GInt
