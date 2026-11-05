import GaussianCollatz.Dynamics
/-!
# Expressions, their maps `T` and `S`, symmetry and rules (Section 1 of `theorems.md`)
-/
namespace GInt

/-- Divide out `π` while possible, with fuel. -/
def oddPartF : Nat → GInt → GInt
  | 0, w => w
  | f + 1, w => if pi ∣ w ∧ w ≠ 0 then oddPartF f (divPi w) else w

/-- Number of divisions by `π` performed by `oddPartF`. -/
def valF : Nat → GInt → Nat
  | 0, _ => 0
  | f + 1, w => if pi ∣ w ∧ w ≠ 0 then valF f (divPi w) + 1 else 0

/-- The odd part `w / π^{v(w)}` of `w ≠ 0` (and `oddPart 0 = 0`). -/
def oddPart (w : GInt) : GInt := oddPartF (N w) w
/-- The exponent `v(w)` of `π` in `w ≠ 0` (and `val 0 = 0`). -/
def val (w : GInt) : Nat := valF (N w) w

/-- An expression: odd multiplier `α`, modulus `π^k`, and a digit table `d` that depends only
on the class modulo `π^k` and makes `π^k ∣ α z + d z` for odd `z`. -/
structure Expr where
  α : GInt
  k : Nat
  d : GInt → GInt
  hα : Odd α
  hclass : ∀ z w, pow pi k ∣ z - w → d z = d w
  hdig : ∀ z, Odd z → pow pi k ∣ α * z + d z

namespace Expr

/-- `T z = z/π` for even `z`, `α z + d z` for odd `z`. -/
def T (E : Expr) (z : GInt) : GInt := if pi ∣ z then divPi z else E.α * z + E.d z

/-- The accelerated map: `S z = (α z + d z) / π^v` for odd `z` (`0` if `α z + d z = 0`),
and `S z = 0` for even `z`. -/
def S (E : Expr) (z : GInt) : GInt := if pi ∣ z then 0 else oddPart (E.α * z + E.d z)

/-- Symmetry: `d (u z) = u d(z)` for every unit `u` and odd `z`. -/
def Symmetric (E : Expr) : Prop := ∀ u z, IsUnit u → Odd z → E.d (u * z) = u * E.d z

/-- A rule: an expression whose accelerated map is trivial. -/
def IsRule (E : Expr) : Prop := Trivial E.S

end Expr

/-- For `w ≠ 0`: `oddPart w` is odd and `w = π^{val w} * oddPart w`. -/
theorem oddPart_spec (w : GInt) (hw : w ≠ 0) :
    Odd (oddPart w) ∧ w = pow pi (val w) * oddPart w := by
  sorry

/-- `oddPart (π^v y) = y` and `val (π^v y) = v` for odd `y`. -/
theorem oddPart_pow_mul (v : Nat) (y : GInt) (hy : Odd y) :
    oddPart (pow pi v * y) = y ∧ val (pow pi v * y) = v := by
  sorry

namespace Expr

/-- For odd `z` with `α z + d z ≠ 0`: `S z` is odd, `α z + d z = π^v S z`, and `v ≥ k`. -/
theorem S_spec (E : Expr) (z : GInt) (hz : Odd z) (hw : E.α * z + E.d z ≠ 0) :
    Odd (E.S z) ∧ E.α * z + E.d z = pow pi (val (E.α * z + E.d z)) * E.S z ∧
    E.k ≤ val (E.α * z + E.d z) := by
  sorry

/-- Certificate for one step of `S`. -/
theorem S_eq (E : Expr) {z y : GInt} (v : Nat) (hz : Odd z)
    (h : E.α * z + E.d z = pow pi v * y) (hy : Odd y) : E.S z = y := by
  sorry

/-- Lemma 1.4(c): under symmetry, `S` commutes with units. -/
theorem S_commutes (E : Expr) (hs : E.Symmetric) : Commutes E.S := by
  sorry

/-- Lemma 1.2(c): for `k ≥ 1` digits are odd. -/
theorem d_odd (E : Expr) (hk : 1 ≤ E.k) (z : GInt) (hz : Odd z) : Odd (E.d z) := by
  sorry

/-- Lemma 1.2(c): one digit per class: congruent digits are equal. -/
theorem d_inj (E : Expr) {z z' : GInt} (hz : Odd z) (hz' : Odd z')
    (h : pow pi E.k ∣ E.d z - E.d z') : E.d z = E.d z' := by
  sorry

/-! Theorems 2.1 and 3.1 for expressions. -/

theorem trivial_cycles_le_four (E : Expr) (L : List GInt)
    (hL : ∀ x ∈ L, OnTrivialCycle E.S x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches E.S x y)) : L.length ≤ 4 :=
  GInt.trivial_cycles_le_four E.S L hL hdist

theorem trivial_cycle_count (E : Expr) (hs : E.Symmetric) (h1 : Periodic E.S 1) :
    (¬ Reaches E.S 1 I ∧ ¬ Reaches E.S 1 (-1) ∧ ¬ Reaches E.S 1 (-I) ∧
      [1, I, -1, -I].Pairwise (fun x y => ¬ Reaches E.S x y)) ∨
    (Reaches E.S 1 (-1) ∧ ¬ Reaches E.S 1 I ∧ ¬ Reaches E.S 1 (-I) ∧ Reaches E.S I (-I)) ∨
    (Reaches E.S 1 I ∧ Reaches E.S 1 (-1) ∧ Reaches E.S 1 (-I)) :=
  GInt.trivial_cycle_count (E.S_commutes hs) h1

theorem unit_cycles (E : Expr) (hs : E.Symmetric) (hw : IsUnit (E.S 1)) :
    (∀ u, IsUnit u → E.S u = u * E.S 1) ∧
    (∀ u n, IsUnit u → iter E.S n u = u * pow (E.S 1) n) ∧
    (∀ u n, IsUnit u → IsUnit (iter E.S n u)) ∧
    (∀ u n, IsUnit u → (iter E.S n u = u ↔ pow (E.S 1) n = 1)) ∧
    (0 < ord (E.S 1) ∧ pow (E.S 1) (ord (E.S 1)) = 1 ∧
      ∀ n, 0 < n → n < ord (E.S 1) → pow (E.S 1) n ≠ 1) :=
  GInt.unit_cycles (E.S_commutes hs) hw

end Expr
end GInt
