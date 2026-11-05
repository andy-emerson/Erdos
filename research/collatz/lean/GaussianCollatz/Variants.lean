import GaussianCollatz.Expr
/-!
# Rotated and mirrored expressions (Section 4 of `theorems.md`)
-/
namespace GInt

/-- Complex conjugation. -/
def conj (z : GInt) : GInt := ⟨z.re, -z.im⟩

namespace Expr

/-- The rotated expression: multiplier `u α`, same digit set, table `z ↦ d (u z)`. -/
def rotate (E : Expr) (u : GInt) (hu : IsUnit u) : Expr where
  α := u * E.α
  k := E.k
  d := fun z => E.d (u * z)
  hα := sorry
  hclass := sorry
  hdig := sorry

/-- **Proposition 4.2** (no symmetry): `S_{uα} z = S_α (u z)`. -/
theorem rotate_S (E : Expr) (u : GInt) (hu : IsUnit u) (z : GInt) :
    (E.rotate u hu).S z = E.S (u * z) := by
  sorry

/-- **Theorem 4.4(1).** -/
theorem rotate_symmetric (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).Symmetric := by
  sorry

/-- **Theorem 4.4(1).** `S_{uα} = u S_α`. -/
theorem rotate_S_mul (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (z : GInt) :
    (E.rotate u hu).S z = u * E.S z := by
  sorry

/-- **Theorem 4.4(2).** `S_{uα}^n z = u^n S_α^n z`. -/
theorem rotate_iter (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (n : Nat)
    (z : GInt) : iter (E.rotate u hu).S n z = pow u n * iter E.S n z := by
  sorry

/-- **Theorem 4.4(2).** Equal norm sequences. -/
theorem rotate_norm (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) (n : Nat)
    (z : GInt) : N (iter (E.rotate u hu).S n z) = N (iter E.S n z) := by
  sorry

/-- **Theorem 4.4(3).** The rotated expression is a rule iff the original is. -/
theorem rotate_rule (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).IsRule ↔ E.IsRule := by
  sorry

/-- **Theorem 4.4(4).** `S_{uα}(1) = u w`, `w = S_α(1)`. -/
theorem rotate_S_one (E : Expr) (hs : E.Symmetric) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).S 1 = u * E.S 1 := rotate_S_mul E hs u hu 1

/-- The mirrored expression: multiplier `conj α`, table `z ↦ conj (d (conj z))`. -/
def mirror (E : Expr) : Expr where
  α := conj E.α
  k := E.k
  d := fun z => conj (E.d (conj z))
  hα := sorry
  hclass := sorry
  hdig := sorry

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
  sorry

/-- **Proposition 4.6** (no symmetry): one step preserves norms. -/
theorem mirror_norm (E : Expr) (z : GInt) : N (E.mirror.S z) = N (E.S (conj z)) := by
  sorry

/-- **Theorem 4.7.** The mirror of a symmetric expression is symmetric. -/
theorem mirror_symmetric (E : Expr) (hs : E.Symmetric) : E.mirror.Symmetric := by
  sorry

/-- **Theorem 4.7.** `S_{ᾱ}^n z = (-i)^{V_n(z̄)} conj (S_α^n z̄)`. -/
theorem mirror_iter (E : Expr) (hs : E.Symmetric) (n : Nat) (z : GInt) :
    iter E.mirror.S n z = pow (-I) (E.V n (conj z)) * conj (iter E.S n (conj z)) := by
  sorry

/-- **Theorem 4.7.** The norm sequence of `z` under the mirror is that of `z̄`. -/
theorem mirror_norm_iter (E : Expr) (hs : E.Symmetric) (n : Nat) (z : GInt) :
    N (iter E.mirror.S n z) = N (iter E.S n (conj z)) := by
  sorry

/-- **Theorem 4.7.** The mirror is a rule iff the original is. -/
theorem mirror_rule (E : Expr) (hs : E.Symmetric) : E.mirror.IsRule ↔ E.IsRule := by
  sorry

end Expr
end GInt
