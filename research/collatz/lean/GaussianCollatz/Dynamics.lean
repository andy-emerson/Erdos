import GaussianCollatz.Soundness
/-!
# Cycles of a map on `ℤ[i]` (Sections 1 to 4 of `theorems.md`)

Abstract dynamics for any `f : GInt → GInt`: periodic points, cycles, trivial cycles, the
triviality criterion, the trivial-cycle bound (Theorem 2.1), the unit-cycle theorem
(Theorem 3.1) and the transfer lemma (Lemma 4.3).
-/
namespace GInt

/-- `x` is periodic: `f^p x = x` for some `p ≥ 1`. -/
def Periodic (f : GInt → GInt) (x : GInt) : Prop := ∃ p, 0 < p ∧ iter f p x = x

/-- `y` lies on the forward orbit of `x`. For periodic `x` this says that `y` lies on the
cycle of `x`, and then the cycles of `x` and `y` are equal (`reaches_symm`). -/
def Reaches (f : GInt → GInt) (x y : GInt) : Prop := ∃ j, iter f j x = y

/-- `x` is periodic and its cycle contains a unit (a trivial cycle). -/
def OnTrivialCycle (f : GInt → GInt) (x : GInt) : Prop :=
  Periodic f x ∧ ∃ j, IsUnit (iter f j x)

/-- Triviality: every odd `z` has an iterate lying on a trivial cycle. -/
def Trivial (f : GInt → GInt) : Prop := ∀ z, Odd z → ∃ n, OnTrivialCycle f (iter f n z)

/-- `f` commutes with the units: `f (u z) = u f(z)`. -/
def Commutes (f : GInt → GInt) : Prop := ∀ u z, IsUnit u → f (u * z) = u * f z

/-- The order of a unit `w` (`1`, `2` or `4`). -/
def ord (w : GInt) : Nat := if w = 1 then 1 else if w = -1 then 2 else 4


/-! ## Theorem 2.1: the trivial-cycle bound -/

/-- **Theorem 2.1(a).** Any map has at most `|U| = 4` trivial cycles: a list of points on
trivial cycles, no two on the same cycle, has length at most 4. -/
theorem trivial_cycles_le_four (f : GInt → GInt) (L : List GInt)
    (hL : ∀ x ∈ L, OnTrivialCycle f x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches f x y)) : L.length ≤ 4 := by
  sorry

/-- Every trivial cycle is the cycle of a unit (any map). -/
theorem trivial_cycle_has_unit (f : GInt → GInt) (x : GInt) (h : OnTrivialCycle f x) :
    ∃ u, IsUnit u ∧ Reaches f x u ∧ Reaches f u x := by
  sorry

/-- **Theorem 2.1(b), first part.** Under symmetry a unit is periodic iff `1` is. -/
theorem unit_periodic_iff {f : GInt → GInt} (hf : Commutes f) {u : GInt} (hu : IsUnit u) :
    Periodic f u ↔ Periodic f 1 := by
  sorry

/-- **Theorem 2.1(b), item 1.** The cycle of a unit `u` is `u C₁`. -/
theorem unit_cycle_eq {f : GInt → GInt} (hf : Commutes f) {u : GInt} (hu : IsUnit u) (y : GInt) :
    Reaches f u y ↔ ∃ c, Reaches f 1 c ∧ y = u * c := by
  sorry

/-- **Theorem 2.1(b), item 3.** `u C₁ = C₁` (as sets) iff `u ∈ C₁`. -/
theorem stab_iff {f : GInt → GInt} (hf : Commutes f) (h1 : Periodic f 1) {u : GInt}
    (hu : IsUnit u) : (∀ y, Reaches f u y ↔ Reaches f 1 y) ↔ Reaches f 1 u := by
  sorry

/-- `U ∩ C₁` is closed under products. -/
theorem stab_mul {f : GInt → GInt} (hf : Commutes f) {u v : GInt} (hu : IsUnit u)
    (h1 : Reaches f 1 u) (h2 : Reaches f 1 v) : Reaches f 1 (u * v) := by
  sorry

/-- **Theorem 2.1(b), item 4.** Under symmetry, with `1` periodic, exactly one of:
`U ∩ C₁ = {1}` and the cycles of `1, i, -1, -i` are pairwise distinct (4 trivial cycles);
`U ∩ C₁ = {±1}`, `C₁ = -C₁`, `iC₁ = -iC₁ ≠ C₁` (2 trivial cycles);
`U ∩ C₁ = U` (1 trivial cycle). By `trivial_cycle_has_unit` and `unit_cycle_eq` these are
all the trivial cycles. -/
theorem trivial_cycle_count {f : GInt → GInt} (hf : Commutes f) (h1 : Periodic f 1) :
    (¬ Reaches f 1 I ∧ ¬ Reaches f 1 (-1) ∧ ¬ Reaches f 1 (-I) ∧
      [1, I, -1, -I].Pairwise (fun x y => ¬ Reaches f x y)) ∨
    (Reaches f 1 (-1) ∧ ¬ Reaches f 1 I ∧ ¬ Reaches f 1 (-I) ∧ Reaches f I (-I)) ∨
    (Reaches f 1 I ∧ Reaches f 1 (-1) ∧ Reaches f 1 (-I)) := by
  sorry

/-! ## Theorem 3.1: unit cycles -/

/-- **Theorem 3.1(1).** If `f` commutes with units and `w = f 1` is a unit, then `f u = u w`,
`f^n u = u w^n` stays a unit, `f^n u = u ↔ w^n = 1`, and `ord w` is the least period. -/
theorem unit_cycles {f : GInt → GInt} (hf : Commutes f) (hw : IsUnit (f 1)) :
    (∀ u, IsUnit u → f u = u * f 1) ∧
    (∀ u n, IsUnit u → iter f n u = u * pow (f 1) n) ∧
    (∀ u n, IsUnit u → IsUnit (iter f n u)) ∧
    (∀ u n, IsUnit u → (iter f n u = u ↔ pow (f 1) n = 1)) ∧
    (0 < ord (f 1) ∧ pow (f 1) (ord (f 1)) = 1 ∧
      ∀ n, 0 < n → n < ord (f 1) → pow (f 1) n ≠ 1) := by
  sorry

/-- **Theorem 3.1(2).** If `f 1` is not a unit, no unit maps to a unit. -/
theorem no_unit_cycle {f : GInt → GInt} (hf : Commutes f) (hw : ¬ IsUnit (f 1)) :
    ∀ u, IsUnit u → ¬ IsUnit (f u) := by
  sorry

/-! ## Lemma 4.3 and an obstruction -/

/-- **Lemma 4.3 (transfer).** If `f, g` commute with units and `g z` is a unit times `f z`
for every `z`, then triviality of `f` implies triviality of `g`. -/
theorem trivial_transfer {f g : GInt → GInt} (hf : Commutes f) (hg : Commutes g)
    (h : ∀ z, ∃ ε, IsUnit ε ∧ g z = ε * f z) : Trivial f → Trivial g := by
  sorry

/-- A set `P` mapped into itself, with no units and an odd element, rules out triviality. -/
theorem not_trivial_of_invariant (f : GInt → GInt) (P : GInt → Prop)
    (hP : ∀ x, P x → P (f x)) (hU : ∀ x, P x → ¬ IsUnit x) (z : GInt) (hz : Odd z)
    (hPz : P z) : ¬ Trivial f := by
  sorry

end GInt
