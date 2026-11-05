import GaussianCollatz.Expr
/-!
# Rotated and mirrored expressions (Section 4 of `theorems.md`)
-/
namespace GInt

/-! ## Conjugation -/

theorem conj_conj (z : GInt) : conj (conj z) = z := by ext <;> simp
theorem conj_mul (x y : GInt) : conj (x * y) = conj x * conj y := by ext <;> simp <;> grind
theorem conj_add (x y : GInt) : conj (x + y) = conj x + conj y := by ext <;> simp <;> omega
theorem conj_sub (x y : GInt) : conj (x - y) = conj x - conj y := by ext <;> simp <;> omega
theorem conj_zero : conj 0 = 0 := by decide
theorem N_conj (z : GInt) : N (conj z) = N z := by
  unfold N; simp [Int.natAbs_neg]

theorem odd_conj {z : GInt} (hz : Odd z) : Odd (conj z) := by
  unfold Odd at *; rw [pi_dvd_iff] at *; simp; omega

theorem even_conj {z : GInt} (hz : pi ∣ z) : pi ∣ conj z := by
  rw [pi_dvd_iff] at *; simp; omega

theorem mul_mul_mul_comm' (a b c d : GInt) : a * b * (c * d) = a * c * (b * d) := by
  rw [mul_assoc, mul_left_comm b c d, ← mul_assoc]

/-- `conj(π^k) = (-i)^k π^k`. -/
theorem conj_pow_pi (k : Nat) : conj (pow pi k) = pow (-I) k * pow pi k := by
  induction k with
  | zero => decide
  | succ k ih =>
    show conj (pow pi k * pi) = pow (-I) k * (-I) * (pow pi k * pi)
    rw [conj_mul, ih, show conj pi = (-I) * pi by decide, mul_mul_mul_comm']

theorem conj_dvd {k : Nat} {x : GInt} (h : pow pi k ∣ x) : pow pi k ∣ conj x := by
  obtain ⟨q, rfl⟩ := h
  refine ⟨pow (-I) k * conj q, ?_⟩
  rw [conj_mul, conj_pow_pi, mul_assoc, mul_left_comm]

theorem unit_negI : IsUnit (-I) := by decide

theorem N_unit_mul_conj {u : GInt} (hu : IsUnit u) (x : GInt) : N (u * conj x) = N x := by
  rw [N_unit_mul hu, N_conj]

/-- `oddPart (conj w) = (-i)^{v(w)} conj (oddPart w)`. -/
theorem oddPart_conj (w : GInt) : oddPart (conj w) = pow (-I) (val w) * conj (oddPart w) := by
  by_cases hw : w = 0
  · subst hw; rw [conj_zero, oddPart_zero, conj_zero, mul_zero']
  · obtain ⟨h1, h2⟩ := oddPart_spec w hw
    have e : conj w = pow pi (val w) * (pow (-I) (val w) * conj (oddPart w)) := by
      conv => lhs; rw [h2]
      rw [conj_mul, conj_pow_pi, mul_assoc, mul_left_comm]
    rw [e]
    exact (oddPart_pow_mul _ _ (odd_unit_mul (pow_unit unit_negI _) (odd_conj h1))).1

/-- Triviality is preserved by conjugating the map by `conj`. -/
theorem trivial_conj {F : GInt → GInt} (hF : Trivial F) : Trivial (fun z => conj (F (conj z))) := by
  have hit : ∀ n z, iter (fun z => conj (F (conj z))) n z = conj (iter F n (conj z)) := by
    intro n
    induction n with
    | zero => intro z; exact (conj_conj z).symm
    | succ n ih => intro z; show iter _ n _ = _; rw [ih, conj_conj]; rfl
  intro z hz
  obtain ⟨n, ⟨p, hp, hpy⟩, j, hj⟩ := hF (conj z) (odd_conj hz)
  refine ⟨n, ⟨p, hp, ?_⟩, j, ?_⟩
  · rw [hit, hit, conj_conj, hpy]
  · rw [hit, hit, conj_conj]; exact unit_conj hj

namespace Expr

/-- The rotated expression: multiplier `u α`, same digit set, table `z ↦ d (u z)`. -/
def rotate (E : Expr) (u : GInt) (hu : IsUnit u) : Expr where
  α := u * E.α
  k := E.k
  d := fun z => E.d (u * z)
  hα := odd_unit_mul hu E.hα
  hclass := fun z w h => E.hclass (u * z) (u * w) (by rw [← mul_sub]; exact dvd_mul_of_dvd u h)
  hdig := fun z hz => by
    rw [mul_comm u E.α, mul_assoc]; exact E.hdig (u * z) (odd_unit_mul hu hz)

/-- **Proposition 4.2** (no symmetry): `S_{uα} z = S_α (u z)`. -/
theorem rotate_S (E : Expr) (u : GInt) (hu : IsUnit u) (z : GInt) :
    (E.rotate u hu).S z = E.S (u * z) := by
  show (if pi ∣ z then 0 else oddPart (u * E.α * z + E.d (u * z))) =
    if pi ∣ u * z then 0 else oddPart (E.α * (u * z) + E.d (u * z))
  by_cases hz : pi ∣ z
  · rw [ite_eq_left hz, ite_eq_left (pi_dvd_mul_of_dvd u hz)]
  · rw [ite_eq_right hz, ite_eq_right (odd_unit_mul hu hz), mul_comm u E.α, mul_assoc]

/-- **Theorem 4.4(1).** -/
theorem rotate_symmetric (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).Symmetric := by
  intro v z hv hz
  show E.d (u * (v * z)) = v * E.d (u * z)
  rw [mul_left_comm]; exact hs v (u * z) hv (odd_unit_mul hu hz)

/-- **Theorem 4.4(1).** `S_{uα} = u S_α`. -/
theorem rotate_S_mul (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (z : GInt) :
    (E.rotate u hu).S z = u * E.S z := by
  rw [rotate_S]; exact E.S_commutes hs u z hu

/-- **Theorem 4.4(2).** `S_{uα}^n z = u^n S_α^n z`. -/
theorem rotate_iter (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (n : Nat)
    (z : GInt) : iter (E.rotate u hu).S n z = pow u n * iter E.S n z := by
  induction n generalizing z with
  | zero => exact (one_mul _).symm
  | succ n ih =>
    show iter _ n ((E.rotate u hu).S z) = pow u n * u * iter E.S n (E.S z)
    rw [rotate_S_mul E hs, ih, iter_unit_mul E.S (E.S_commutes hs) hu, mul_assoc]

/-- **Theorem 4.4(2).** Equal norm sequences. -/
theorem rotate_norm (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (n : Nat)
    (z : GInt) : N (iter (E.rotate u hu).S n z) = N (iter E.S n z) := by
  rw [rotate_iter E hs, N_unit_mul (pow_unit hu n)]

/-- **Theorem 4.4(3).** The rotated expression is a rule iff the original is. -/
theorem rotate_rule (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).IsRule ↔ E.IsRule := by
  constructor
  · apply trivial_transfer (E.S_commutes hs)
    intro z
    refine ⟨conj u, unit_conj hu, ?_⟩
    rw [rotate_S_mul E hs, ← mul_assoc, conj_mul_self hu, one_mul]
  · apply trivial_transfer ((E.rotate u hu).S_commutes (rotate_symmetric E hs u hu))
    intro z
    exact ⟨u, hu, rotate_S_mul E hs u hu z⟩

/-- **Theorem 4.4(4).** `S_{uα}(1) = u w`, `w = S_α(1)`. -/
theorem rotate_S_one (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).S 1 = u * E.S 1 := rotate_S_mul E hs u hu 1

/-- The mirrored expression: multiplier `conj α`, table `z ↦ conj (d (conj z))`. -/
def mirror (E : Expr) : Expr where
  α := conj E.α
  k := E.k
  d := fun z => conj (E.d (conj z))
  hα := odd_conj E.hα
  hclass := fun z w h => by
    show conj (E.d (conj z)) = conj (E.d (conj w))
    rw [E.hclass (conj z) (conj w) (by rw [← conj_sub]; exact conj_dvd h)]
  hdig := fun z hz => by
    have := conj_dvd (E.hdig (conj z) (odd_conj hz))
    rwa [conj_add, conj_mul, conj_conj] at this

/-- The exponent of `π` in the odd step from `y` (`0` for even `y`). -/
def vstep (E : Expr) (y : GInt) : Nat := if pi ∣ y then 0 else val (E.α * y + E.d y)

/-- `V n y`: total exponent along the first `n` steps of the `S`-orbit of `y`. -/
def V (E : Expr) : Nat → GInt → Nat
  | 0, _ => 0
  | n + 1, y => E.vstep y + E.V n (E.S y)

/-- **Proposition 4.6** (no symmetry): `S_{ᾱ}(z) = (-i)^v conj (S_α (conj z))`, where `v` is
the exponent of `π` in `α z̄ + d(z̄)`. -/
theorem mirror_S (E : Expr) (z : GInt) :
    E.mirror.S z = pow (-I) (E.vstep (conj z)) * conj (E.S (conj z)) := by
  show (if pi ∣ z then 0 else oddPart (conj E.α * z + conj (E.d (conj z)))) =
    pow (-I) (if pi ∣ conj z then 0 else val (E.α * conj z + E.d (conj z))) *
      conj (if pi ∣ conj z then 0 else oddPart (E.α * conj z + E.d (conj z)))
  by_cases hz : pi ∣ z
  · rw [ite_eq_left hz, ite_eq_left (even_conj hz), ite_eq_left (even_conj hz), conj_zero,
      mul_zero']
  · rw [ite_eq_right hz, ite_eq_right (odd_conj hz), ite_eq_right (odd_conj hz),
      ← oddPart_conj, conj_add, conj_mul, conj_conj]

/-- **Proposition 4.6** (no symmetry): one step preserves norms. -/
theorem mirror_norm (E : Expr) (z : GInt) : N (E.mirror.S z) = N (E.S (conj z)) := by
  rw [mirror_S, N_unit_mul_conj (pow_unit unit_negI _)]

/-- **Theorem 4.7.** The mirror of a symmetric expression is symmetric. -/
theorem mirror_symmetric (E : Expr) (hs : E.Symmetric) : E.mirror.Symmetric := by
  intro v z hv hz
  show conj (E.d (conj (v * z))) = v * conj (E.d (conj z))
  rw [conj_mul, hs (conj v) (conj z) (unit_conj hv) (odd_conj hz), conj_mul, conj_conj]

/-- **Theorem 4.7.** `S_{ᾱ}^n z = (-i)^{V_n(z̄)} conj (S_α^n z̄)`. -/
theorem mirror_iter (E : Expr) (hs : E.Symmetric) (n : Nat) (z : GInt) :
    iter E.mirror.S n z = pow (-I) (E.V n (conj z)) * conj (iter E.S n (conj z)) := by
  induction n generalizing z with
  | zero => show z = pow (-I) 0 * conj (conj z); rw [conj_conj]; exact (one_mul z).symm
  | succ n ih =>
    show iter E.mirror.S n (E.mirror.S z) =
      pow (-I) (E.vstep (conj z) + E.V n (E.S (conj z))) * conj (iter E.S n (E.S (conj z)))
    rw [mirror_S, iter_unit_mul _ (E.mirror.S_commutes (mirror_symmetric E hs))
      (pow_unit unit_negI _), ih, conj_conj, pow_add, mul_assoc]

/-- **Theorem 4.7.** The norm sequence of `z` under the mirror is that of `z̄`. -/
theorem mirror_norm_iter (E : Expr) (hs : E.Symmetric) (n : Nat) (z : GInt) :
    N (iter E.mirror.S n z) = N (iter E.S n (conj z)) := by
  rw [mirror_iter E hs, N_unit_mul_conj (pow_unit unit_negI _)]

/-- **Theorem 4.7.** The mirror is a rule iff the original is. -/
theorem mirror_rule (E : Expr) (hs : E.Symmetric) : E.mirror.IsRule ↔ E.IsRule := by
  have hh : Commutes (fun z => conj (E.S (conj z))) := by
    intro u z hu
    show conj (E.S (conj (u * z))) = u * conj (E.S (conj z))
    rw [conj_mul, E.S_commutes hs _ _ (unit_conj hu), conj_mul, conj_conj]
  have hm := E.mirror.S_commutes (mirror_symmetric E hs)
  have back : (fun z => conj ((fun z => conj (E.S (conj z))) (conj z))) = E.S := by
    funext z; simp only [conj_conj]
  constructor
  · intro h
    have h1 : Trivial (fun z => conj (E.S (conj z))) := by
      apply trivial_transfer hh _ h
      intro z
      refine ⟨conj (pow (-I) (E.vstep (conj z))), unit_conj (pow_unit unit_negI _), ?_⟩
      rw [mirror_S, ← mul_assoc, conj_mul_self (pow_unit unit_negI _), one_mul]
    have := trivial_conj h1
    rwa [back] at this
  · intro h
    apply trivial_transfer hm _ (trivial_conj h)
    intro z
    exact ⟨_, pow_unit unit_negI _, mirror_S E z⟩

end Expr
end GInt
