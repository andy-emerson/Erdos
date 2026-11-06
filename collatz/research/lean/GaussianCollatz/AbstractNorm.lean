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
  N_pos : ∀ a, a ≠ 0 → 0 < N a
  isU : R → Prop
  N_unit : ∀ u, isU u → N u = 1
  pi : R
  N_pi : 2 ≤ N pi

variable {R : Type} [Lean.Grind.CommRing R] (D : NormData R)

/-- Core of the `T` case (Lemma 5.0(1)): `W' = u W + e`, `W' = ε W`, `N e < N W` force `e = 0`. -/
theorem NormData.core_T {u ε W e : R} (hu : D.isU u) (hε : D.isU ε) (hlin : ε * W = u * W + e)
    (hN : D.N e < D.N W) : e = 0 := by
  sorry

/-- Core of the `S` case (Lemma 5.0(2)): `W = π^a o`, `W' = π^b (ε o)`, `W' = u W + e`, and
`N e` below `N W` and `N W'`, force `e = 0`. -/
theorem NormData.core_S {u ε W W' e o : R} {a b : Nat} (hu : D.isU u) (hε : D.isU ε)
    (hW : W = D.pi ^ a * o) (hW' : W' = D.pi ^ b * (ε * o)) (hlin : W' = u * W + e)
    (hN : D.N e < D.N W) (hN' : D.N e < D.N W') : e = 0 := by
  sorry

/-- **Theorem 9.1 for `T`.** If at a point of every class the values `α z + d z` and
`α (u z) + d (u z)` are associates and large, the table commutes with `U`. -/
theorem NormData.commutes_of_T_respects (α : R) (d : R → R) (Odd : R → Prop)
    (big : ∀ z0 u (B : Nat), Odd z0 → D.isU u → ∃ z, Odd z ∧ d z = d z0 ∧
      d (u * z) = d (u * z0) ∧ B < D.N (α * z + d z))
    (resp : ∀ u z, D.isU u → Odd z → ∃ ε, D.isU ε ∧
      α * (u * z) + d (u * z) = ε * (α * z + d z)) :
    ∀ u z0, D.isU u → Odd z0 → d (u * z0) = u * d z0 := by
  sorry

/-- **Theorem 9.1 for `S`.** The same with odd parts: `op w` is the odd part and `val w` the
exponent, `w = π^{val w} op w` for `w ≠ 0`. -/
theorem NormData.commutes_of_S_respects (α : R) (d : R → R) (Odd : R → Prop)
    (op : R → R) (val : R → Nat) (hop : ∀ w, w ≠ 0 → w = D.pi ^ val w * op w)
    (big : ∀ z0 u (B : Nat), Odd z0 → D.isU u → ∃ z, Odd z ∧ d z = d z0 ∧
      d (u * z) = d (u * z0) ∧ B < D.N (α * z + d z) ∧ B < D.N (α * (u * z) + d (u * z)))
    (resp : ∀ u z, D.isU u → Odd z → ∃ ε, D.isU ε ∧
      op (α * (u * z) + d (u * z)) = ε * op (α * z + d z)) :
    ∀ u z0, D.isU u → Odd z0 → d (u * z0) = u * d z0 := by
  sorry

end Abstract
