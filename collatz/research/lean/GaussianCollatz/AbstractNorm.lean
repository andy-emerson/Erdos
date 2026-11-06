/-!
# Respecting associates is commuting, over any commutative ring (`general-theory.md`, Section 9)

`R` is any commutative ring (core's `Lean.Grind.CommRing`, so that `grind` proves the ring
identities), with a norm `N : R → ℕ` that is multiplicative, nonzero off `0`, `1` on the
group `U` (a predicate `isU`), and at least `2` on the prime `π`. No inverse of `π`, no
finiteness of `U`, and not that `N = 1` only on units. The input that is specific to a ring
is the big-point property (B): in every class there are points where the two values
`α z + d z` and `α (u z) + d (u z)` have large norm (Lemma 9.2 proves it for every order of a
number field; for `ℤ[i]` it is `Expr.big_point`).
-/
namespace Abstract

/-- A norm on a commutative ring, with a group of units and a prime. -/
structure NormData (R : Type) [Lean.Grind.CommRing R] where
  N : R → Nat
  N_mul : ∀ a b, N (a * b) = N a * N b
  N_one : N 1 = 1
  N_zero : N 0 = 0
  N_pos : ∀ a, a ≠ 0 → 0 < N a
  isU : R → Prop
  N_unit : ∀ u, isU u → N u = 1
  pi : R
  N_pi : 2 ≤ N pi

variable {R : Type} [Lean.Grind.CommRing R] (D : NormData R)

theorem NormData.N_pow (a : R) (t : Nat) : D.N (a ^ t) = D.N a ^ t := by
  induction t with
  | zero => rw [Lean.Grind.Semiring.pow_zero, D.N_one, Nat.pow_zero]
  | succ t ih => rw [Lean.Grind.Semiring.pow_succ, D.N_mul, ih, Nat.pow_succ]

/-- `N(c W) ≥ N W` for `c ≠ 0`. -/
theorem NormData.N_le_mul {c W : R} (hc : c ≠ 0) : D.N W ≤ D.N (c * W) := by
  rw [D.N_mul]; exact Nat.le_mul_of_pos_left _ (D.N_pos c hc)

/-- `ε π^{t+1} ≠ u` for `ε, u ∈ U`. -/
theorem NormData.unit_pow_ne {ε u : R} (hε : D.isU ε) (hu : D.isU u) (t : Nat) :
    ε * D.pi ^ (t + 1) - u ≠ 0 := by
  intro h0
  have e : ε * D.pi ^ (t + 1) = u := by
    have : ε * D.pi ^ (t + 1) = (ε * D.pi ^ (t + 1) - u) + u := by grind
    rw [this, h0]; grind
  have := congrArg D.N e
  rw [D.N_mul, D.N_unit ε hε, D.N_unit u hu, D.N_pow, Nat.one_mul] at this
  have h2 := D.N_pi
  have : 2 ^ (t + 1) ≤ D.N D.pi ^ (t + 1) := Nat.pow_le_pow_left h2 _
  have : 2 ≤ 2 ^ (t + 1) := by
    rw [Nat.pow_succ]; have := Nat.two_pow_pos t; omega
  omega

/-- Core of the `T` case (Lemma 5.0(1)): `W' = u W + e`, `W' = ε W`, `N e < N W` force `e = 0`. -/
theorem NormData.core_T {u ε W e : R} (_hu : D.isU u) (_hε : D.isU ε) (hlin : ε * W = u * W + e)
    (hN : D.N e < D.N W) : e = 0 := by
  have hc : (ε - u) * W = e := by grind
  apply Classical.byContradiction
  intro he
  have hne : ε - u ≠ 0 := by
    intro h0; apply he; rw [← hc, h0]; grind
  have := D.N_le_mul (W := W) hne
  rw [hc] at this
  omega

/-- Core of the `S` case (Lemma 5.0(2)): `W = π^a o`, `W' = π^b (ε o)`, `W' = u W + e`, and
`N e` below `N W` and `N W'`, force `e = 0`. -/
theorem NormData.core_S {u ε W W' e o : R} {a b : Nat} (hu : D.isU u) (hε : D.isU ε)
    (hW : W = D.pi ^ a * o) (hW' : W' = D.pi ^ b * (ε * o)) (hlin : W' = u * W + e)
    (hN : D.N e < D.N W) (hN' : D.N e < D.N W') : e = 0 := by
  rcases Nat.lt_or_ge b a with hab | hab
  · obtain ⟨t, rfl⟩ : ∃ t, a = b + (t + 1) := ⟨a - b - 1, by omega⟩
    have hrel : D.pi ^ (t + 1) * W' = ε * W := by
      rw [hW, hW', Lean.Grind.Semiring.pow_add D.pi b (t + 1)]
      generalize D.pi ^ b = P1; generalize D.pi ^ (t + 1) = P2; grind
    have hc : (ε - u * D.pi ^ (t + 1)) * W' = ε * e := by
      have : (ε - u * D.pi ^ (t + 1)) * W' = ε * W' - u * (D.pi ^ (t + 1) * W') := by grind
      rw [this, hrel, hlin]; grind
    have hne : ε - u * D.pi ^ (t + 1) ≠ 0 := by
      intro h0
      apply D.unit_pow_ne hu hε t
      have : u * D.pi ^ (t + 1) - ε = -(ε - u * D.pi ^ (t + 1)) := by grind
      rw [this, h0]; grind
    have := D.N_le_mul (W := W') hne
    rw [hc, D.N_mul, D.N_unit ε hε, Nat.one_mul] at this
    omega
  · obtain ⟨t, rfl⟩ : ∃ t, b = a + t := ⟨b - a, by omega⟩
    have hrel : W' = (ε * D.pi ^ t) * W := by
      rw [hW, hW', Lean.Grind.Semiring.pow_add D.pi a t]
      generalize D.pi ^ a = P1; generalize D.pi ^ t = P2; grind
    rcases t with _ | t
    · rw [Lean.Grind.Semiring.pow_zero] at hrel
      exact D.core_T hu hε (by rw [← hlin, hrel]; grind) hN
    · have hc : (ε * D.pi ^ (t + 1) - u) * W = e := by
        have : (ε * D.pi ^ (t + 1) - u) * W = (ε * D.pi ^ (t + 1)) * W - u * W := by grind
        rw [this, ← hrel, hlin]; grind
      exfalso
      have := D.N_le_mul (W := W) (D.unit_pow_ne hε hu t)
      rw [hc] at this
      omega

/-- **Theorem 9.1 for `T`.** If at a point of every class the values `α z + d z` and
`α (u z) + d (u z)` are associates and large, the table commutes with `U`. -/
theorem NormData.commutes_of_T_respects (α : R) (d : R → R) (Odd : R → Prop)
    (big : ∀ z0 u (B : Nat), Odd z0 → D.isU u → ∃ z, Odd z ∧ d z = d z0 ∧
      d (u * z) = d (u * z0) ∧ B < D.N (α * z + d z))
    (resp : ∀ u z, D.isU u → Odd z → ∃ ε, D.isU ε ∧
      α * (u * z) + d (u * z) = ε * (α * z + d z)) :
    ∀ u z0, D.isU u → Odd z0 → d (u * z0) = u * d z0 := by
  intro u z0 hu hz0
  obtain ⟨z, hz, hd, hdu, hW⟩ := big z0 u (D.N (d (u * z0) - u * d z0)) hz0 hu
  obtain ⟨ε, hε, heq⟩ := resp u z hu hz
  have hlin : ε * (α * z + d z) = u * (α * z + d z) + (d (u * z0) - u * d z0) := by
    rw [← heq, hd, hdu]; grind
  have := D.core_T hu hε hlin hW
  grind

/-- **Theorem 9.1 for `S`.** The same with odd parts: `op w` is the odd part and `val w` the
exponent, `w = π^{val w} op w` for `w ≠ 0`. -/
theorem NormData.commutes_of_S_respects (α : R) (d : R → R) (Odd : R → Prop)
    (op : R → R) (val : R → Nat) (hop : ∀ w, w ≠ 0 → w = D.pi ^ val w * op w)
    (big : ∀ z0 u (B : Nat), Odd z0 → D.isU u → ∃ z, Odd z ∧ d z = d z0 ∧
      d (u * z) = d (u * z0) ∧ B < D.N (α * z + d z) ∧ B < D.N (α * (u * z) + d (u * z)))
    (resp : ∀ u z, D.isU u → Odd z → ∃ ε, D.isU ε ∧
      op (α * (u * z) + d (u * z)) = ε * op (α * z + d z)) :
    ∀ u z0, D.isU u → Odd z0 → d (u * z0) = u * d z0 := by
  intro u z0 hu hz0
  obtain ⟨z, hz, hd, hdu, hW, hW'⟩ := big z0 u (D.N (d (u * z0) - u * d z0)) hz0 hu
  obtain ⟨ε, hε, heq⟩ := resp u z hu hz
  have ne0 : ∀ w : R, D.N (d (u * z0) - u * d z0) < D.N w → w ≠ 0 := by
    intro w hw h0
    rw [h0, D.N_zero] at hw; omega
  have hlin : α * (u * z) + d (u * z) = u * (α * z + d z) + (d (u * z0) - u * d z0) := by
    rw [hd, hdu]; grind
  have e1 := hop _ (ne0 _ hW)
  have e2 := hop _ (ne0 _ hW')
  rw [heq] at e2
  have := D.core_S hu hε e1 e2 hlin hW hW'
  grind

end Abstract
