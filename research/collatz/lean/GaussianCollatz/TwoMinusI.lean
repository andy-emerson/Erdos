import GaussianCollatz.Variants
/-!
# Soundness of the check without symmetry, and the expression `α = 2 - i` at modulus `π²`
(Section 6 of `theorems.md`, Remark 3.2)
-/
namespace GInt

/-- **Theorem 6.1.** Soundness of the all-quadrant drop-below-start check (`gstopall.c`):
no relation between `T (u z)` and `T z` is assumed. -/
theorem drop_below_start_sound_all (T : GInt → GInt)
    (hdiv : ∀ z, pi ∣ z → T z = divPi z) (N0 B : Nat)
    (hcheck : ∀ z : GInt, Odd z → N0 < N z → N z ≤ B → ∃ n, N (iter T n z) < N z) :
    ∀ z : GInt, N z ≤ B → ∃ n, N (iter T n z) ≤ N0 := by
  sorry

/-- The digit table of `α = 2 - i` at modulus `π²`: `i` for `a` odd, `b` even;
`1` for `a` even, `b` odd. -/
def digit2 (z : GInt) : GInt :=
  match z.re % 2, z.im % 2 with
  | 1, 0 => I
  | 0, 1 => 1
  | _, _ => 0

/-- **Proposition 6.2(1).** The table depends only on the class modulo `π²`. -/
theorem digit2_class : ∀ z w, pow pi 2 ∣ z - w → digit2 z = digit2 w := by
  sorry

/-- **Proposition 6.2(1).** The table is correct: `π² ∣ (2 - i) z + d(z)` for odd `z`. -/
theorem digit2_correct : ∀ z, Odd z → pow pi 2 ∣ (⟨2, -1⟩ : GInt) * z + digit2 z := by
  sorry

/-- The expression `E₂`. -/
def E2 : Expr where
  α := ⟨2, -1⟩
  k := 2
  d := digit2
  hα := sorry
  hclass := digit2_class
  hdig := digit2_correct

/-- **Proposition 6.2(2).** `T` divides by `π` on even inputs. -/
theorem E2_T_div : ∀ z, pi ∣ z → E2.T z = divPi z := by
  sorry

/-- **Proposition 6.2(3).** `1 → -i → -1 → 1` is a cycle of `S`, and `S(i) = -i`. -/
theorem E2_cycle : E2.S 1 = -I ∧ E2.S (-I) = -1 ∧ E2.S (-1) = 1 ∧ E2.S I = -I := by
  sorry

/-- **Remark 3.2.** `i` is not periodic, and `E₂` is not symmetric. -/
theorem E2_I_not_periodic : ¬ Periodic E2.S I := by
  sorry

theorem E2_not_symmetric : ¬ E2.Symmetric := by
  sorry

/-- **Proposition 6.2(4).** `T z ≠ 0` for `z ≠ 0`. -/
theorem E2_T_ne_zero : ∀ z, z ≠ 0 → E2.T z ≠ 0 := by
  sorry

/-- **Corollary 6.3.** If the all-quadrant check holds for `E₂` with `N0 = 1` up to `B`, every
nonzero `z` with `N z ≤ B` reaches the unit cycle `{1, -i, -1}` under `T`. -/
theorem E2_sound (B : Nat)
    (hcheck : ∀ z : GInt, Odd z → 1 < N z → N z ≤ B → ∃ n, N (iter E2.T n z) < N z) :
    ∀ z : GInt, z ≠ 0 → N z ≤ B →
      ∃ n, iter E2.T n z = 1 ∨ iter E2.T n z = -I ∨ iter E2.T n z = -1 := by
  sorry

end GInt
