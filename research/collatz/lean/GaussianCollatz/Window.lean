import GaussianCollatz.Variants
/-!
# No symmetric rule inside the window below norm 29 (Section 5 of `theorems.md`)
-/
namespace GInt
namespace Expr

/-- **Lemma 5.1.** If the digit `d z₀` equals `(ε π^v - α) z` with `v ≥ k`, `z` odd, then
`d z = d z₀` and `S z = ε z`. -/
theorem fixed_point (E : Expr) (v : Nat) (hv : E.k ≤ v) (ε : GInt) (hε : IsUnit ε)
    (z z0 : GInt) (hz : Odd z) (hz0 : Odd z0)
    (h : E.d z0 = (ε * pow pi v - E.α) * z) : E.d z = E.d z0 ∧ E.S z = ε * z := by
  sorry

/-- **Lemma 5.1**, second part: if moreover `z` is not a unit and the expression is symmetric
or `ε = 1`, the expression is not a rule. -/
theorem fixed_point_not_rule (E : Expr) (v : Nat) (hv : E.k ≤ v) (ε : GInt) (hε : IsUnit ε)
    (hs : E.Symmetric ∨ ε = 1) (z z0 : GInt) (hz : Odd z) (hz0 : Odd z0)
    (h : E.d z0 = (ε * pow pi v - E.α) * z) (hzu : ¬ IsUnit z) : ¬ E.IsRule := by
  sorry

/-- **Corollary 5.2.** If `ε π^v - α` is a unit (`v ≥ k ≥ 1`) and the expression is a rule,
symmetric or with `ε = 1`, then every digit is a unit. -/
theorem digits_units (E : Expr) (hk : 1 ≤ E.k) (v : Nat) (hv : E.k ≤ v) (ε : GInt)
    (hε : IsUnit ε) (hη : IsUnit (ε * pow pi v - E.α)) (hs : E.Symmetric ∨ ε = 1)
    (hr : E.IsRule) : ∀ z, Odd z → IsUnit (E.d z) := by
  sorry

/-- **Theorem 5.4.** No symmetric expression with `2^k < N(α) < 2^(k+1)` and `N(α) < 29`
is a rule, for any digit table. -/
theorem no_symmetric_rule_below_29 (E : Expr) (hs : E.Symmetric)
    (hlo : 2 ^ E.k < N E.α) (hhi : N E.α < 2 ^ (E.k + 1)) (h29 : N E.α < 29) :
    ¬ E.IsRule := by
  sorry

end Expr
end GInt
