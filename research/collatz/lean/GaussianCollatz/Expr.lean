import GaussianCollatz.Dynamics
import GaussianCollatz.Props
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


/-! ## Arithmetic lemmas -/

theorem N_mul (x y : GInt) : N (x * y) = N x * N y := by
  apply Int.ofNat.inj
  simp only [Int.ofNat_eq_natCast, Int.natCast_mul, N_cast, mul_re, mul_im]
  grind

theorem N_eq_zero {w : GInt} (h : N w = 0) : w = 0 := by
  unfold N at h
  have e1 : w.re.natAbs * w.re.natAbs = 0 := by omega
  have e2 : w.im.natAbs * w.im.natAbs = 0 := by omega
  have f1 : w.re.natAbs = 0 := by rcases Nat.mul_eq_zero.1 e1 with h | h <;> exact h
  have f2 : w.im.natAbs = 0 := by rcases Nat.mul_eq_zero.1 e2 with h | h <;> exact h
  ext
  · exact Int.natAbs_eq_zero.1 f1
  · exact Int.natAbs_eq_zero.1 f2

theorem N_pos {w : GInt} (h : w ≠ 0) : 0 < N w := by
  rcases Nat.eq_zero_or_pos (N w) with h0 | h0
  · exact absurd (N_eq_zero h0) h
  · exact h0

theorem pi_cancel {a b : GInt} (h : pi * a = pi * b) : a = b := by
  have h1 := congrArg GInt.re h
  have h2 := congrArg GInt.im h
  simp only [mul_re, mul_im, pi_re, pi_im] at h1 h2
  ext <;> omega

theorem pow_succ' (z : GInt) (n : Nat) : pow z (n + 1) = z * pow z n := by
  show pow z n * z = z * pow z n
  exact mul_comm _ _

theorem pow_pi_cancel {v : Nat} {a b : GInt} (h : pow pi v * a = pow pi v * b) : a = b := by
  induction v with
  | zero => simpa [pow, one_mul] using h
  | succ v ih =>
    rw [pow_succ', mul_assoc, mul_assoc] at h
    exact ih (pi_cancel h)

theorem dvd_sub' {a x y : GInt} : a ∣ x → a ∣ y → a ∣ x - y
  | ⟨q, hq⟩, ⟨q', hq'⟩ => ⟨q - q', by rw [hq, hq', mul_sub]⟩

theorem dvd_mul_left' (a b : GInt) : a ∣ b * a := ⟨b, mul_comm _ _⟩

theorem dvd_mul_of_dvd {a x : GInt} (c : GInt) : a ∣ x → a ∣ c * x
  | ⟨q, hq⟩ => ⟨c * q, by rw [hq, mul_left_comm]⟩

theorem mul_zero' (x : GInt) : x * 0 = 0 := by ext <;> simp
theorem zero_mul' (x : GInt) : 0 * x = 0 := by ext <;> simp

/-- `π` is prime. -/
theorem pi_dvd_mul {x y : GInt} (h : pi ∣ x * y) : pi ∣ x ∨ pi ∣ y := by
  rw [pi_dvd_iff] at h ⊢; rw [pi_dvd_iff]
  simp only [mul_re, mul_im] at h
  have e : x.re * y.re - x.im * y.im - (x.re * y.im + x.im * y.re) =
      (x.re - x.im) * (y.re - y.im) - 2 * (x.im * y.im) := by grind
  rw [e] at h
  have hm := Int.mul_emod (x.re - x.im) (y.re - y.im) 2
  have hA : (x.re - x.im) % 2 = 0 ∨ (x.re - x.im) % 2 = 1 := by omega
  have hB : (y.re - y.im) % 2 = 0 ∨ (y.re - y.im) % 2 = 1 := by omega
  rcases hA with hA | hA <;> rcases hB with hB | hB
  · exact Or.inl hA
  · exact Or.inl hA
  · exact Or.inr hB
  · rw [hA, hB] at hm; omega

theorem odd_mul {x y : GInt} (hx : Odd x) (hy : Odd y) : Odd (x * y) := by
  intro h; rcases pi_dvd_mul h with h | h
  · exact hx h
  · exact hy h

/-- Lemma 1.2(b): for odd `α`, `π^k ∣ α x` implies `π^k ∣ x`. -/
theorem pow_dvd_of_odd_mul {α : GInt} (hα : Odd α) (k : Nat) {x : GInt}
    (h : pow pi k ∣ α * x) : pow pi k ∣ x := by
  induction k generalizing x with
  | zero => exact ⟨x, (one_mul x).symm⟩
  | succ k ih =>
    have hpi : pi ∣ α * x := by
      obtain ⟨q, hq⟩ := h
      exact ⟨pow pi k * q, by rw [hq, pow_succ', mul_assoc]⟩
    rcases pi_dvd_mul hpi with h1 | h1
    · exact absurd h1 hα
    · obtain ⟨x', rfl⟩ := h1
      obtain ⟨q, hq⟩ := h
      have : α * x' = pow pi k * q := by
        apply pi_cancel
        rw [← mul_assoc, mul_comm pi α, mul_assoc, hq, pow_succ', mul_assoc]
      obtain ⟨q', hq'⟩ := ih ⟨q, this⟩
      exact ⟨q', by rw [hq', pow_succ', mul_assoc]⟩

theorem N_pow_pi (v : Nat) : N (pow pi v) = 2 ^ v := by
  induction v with
  | zero => rfl
  | succ v ih => rw [pow, N_mul, ih]; rfl

theorem pow_pi_mul_ne_zero {v : Nat} {y : GInt} (hy : Odd y) : pow pi v * y ≠ 0 := by
  intro h
  have := congrArg N h
  rw [N_mul, N_pow_pi] at this
  have h0 : N y = 0 := by
    rcases Nat.mul_eq_zero.1 this with h | h
    · exact absurd h (Nat.pos_iff_ne_zero.1 (Nat.two_pow_pos v))
    · exact h
  exact odd_ne_zero hy (N_eq_zero h0)

/-- Uniqueness of `w = π^v * (odd)`. -/
theorem pow_odd_unique {a b : Nat} {o o' : GInt} (ho : Odd o) (ho' : Odd o')
    (h : pow pi a * o = pow pi b * o') : a = b ∧ o = o' := by
  induction a generalizing b with
  | zero =>
    cases b with
    | zero => simp only [pow, one_mul] at h; exact ⟨rfl, h⟩
    | succ b =>
      exfalso; apply ho
      simp only [pow, one_mul] at h
      exact ⟨pow pi b * o', by rw [h, mul_comm (pow pi b) pi, mul_assoc]⟩
  | succ a ih =>
    cases b with
    | zero =>
      exfalso; apply ho'
      simp only [pow, one_mul] at h
      exact ⟨pow pi a * o, by rw [← h, mul_comm (pow pi a) pi, mul_assoc]⟩
    | succ b =>
      rw [pow_succ', pow_succ', mul_assoc, mul_assoc] at h
      obtain ⟨h1, h2⟩ := ih (pi_cancel h)
      exact ⟨by omega, h2⟩

theorem oddPartF_spec : ∀ (f : Nat) (w : GInt), N w ≤ f → w ≠ 0 →
    Odd (oddPartF f w) ∧ w = pow pi (valF f w) * oddPartF f w := by
  intro f
  induction f with
  | zero => intro w hN hw; exact absurd (N_eq_zero (by omega)) hw
  | succ f ih =>
    intro w hN hw
    by_cases h : pi ∣ w ∧ w ≠ 0
    · rw [oddPartF, valF, ite_eq_left h, ite_eq_left h]
      have hd := divPi_mul w h.1
      have hd0 : divPi w ≠ 0 := by
        intro h0; rw [h0, mul_zero'] at hd; exact hw hd.symm
      have hN2 := N_divPi w h.1
      have hpos := N_pos hd0
      obtain ⟨h1, h2⟩ := ih (divPi w) (by omega) hd0
      refine ⟨h1, ?_⟩
      rw [pow_succ', mul_assoc, ← h2, hd]
    · rw [oddPartF, valF, ite_eq_right h, ite_eq_right h]
      refine ⟨fun hp => h ⟨hp, hw⟩, ?_⟩
      exact (one_mul w).symm

/-- For `w ≠ 0`: `oddPart w` is odd and `w = π^{val w} * oddPart w`. -/
theorem oddPart_spec (w : GInt) (hw : w ≠ 0) :
    Odd (oddPart w) ∧ w = pow pi (val w) * oddPart w := by
  exact oddPartF_spec (N w) w (Nat.le_refl _) hw

/-- `oddPart (π^v y) = y` and `val (π^v y) = v` for odd `y`. -/
theorem oddPart_pow_mul (v : Nat) (y : GInt) (hy : Odd y) :
    oddPart (pow pi v * y) = y ∧ val (pow pi v * y) = v := by
  have hw := pow_pi_mul_ne_zero (v := v) hy
  obtain ⟨h1, h2⟩ := oddPart_spec _ hw
  obtain ⟨e1, e2⟩ := pow_odd_unique h1 hy h2.symm
  exact ⟨e2, e1⟩


theorem oddPart_zero : oddPart 0 = 0 := rfl

/-- `oddPart (u w) = u oddPart w` for a unit `u`. -/
theorem oddPart_unit_mul {u : GInt} (hu : IsUnit u) (w : GInt) :
    oddPart (u * w) = u * oddPart w := by
  by_cases hw : w = 0
  · subst hw; rw [mul_zero', oddPart_zero, mul_zero']
  · obtain ⟨h1, h2⟩ := oddPart_spec w hw
    have : u * w = pow pi (val w) * (u * oddPart w) := by
      conv => lhs; rw [h2]
      rw [mul_left_comm]
    rw [this]
    exact (oddPart_pow_mul _ _ (odd_unit_mul hu h1)).1

theorem val_unit_mul {u : GInt} (hu : IsUnit u) (w : GInt) : val (u * w) = val w := by
  by_cases hw : w = 0
  · subst hw; rw [mul_zero']
  · obtain ⟨h1, h2⟩ := oddPart_spec w hw
    have : u * w = pow pi (val w) * (u * oddPart w) := by
      conv => lhs; rw [h2]
      rw [mul_left_comm]
    rw [this]
    exact (oddPart_pow_mul _ _ (odd_unit_mul hu h1)).2

theorem pi_dvd_mul_of_dvd (u : GInt) {z : GInt} (h : pi ∣ z) : pi ∣ u * z :=
  dvd_mul_of_dvd u h

namespace Expr

/-- For odd `z` with `α z + d z ≠ 0`: `S z` is odd, `α z + d z = π^v S z`, and `v ≥ k`. -/
theorem S_spec (E : Expr) (z : GInt) (hz : Odd z) (hw : E.α * z + E.d z ≠ 0) :
    Odd (E.S z) ∧ E.α * z + E.d z = pow pi (val (E.α * z + E.d z)) * E.S z ∧
    E.k ≤ val (E.α * z + E.d z) := by
  have hS : E.S z = oddPart (E.α * z + E.d z) := by unfold S; rw [ite_eq_right hz]
  rw [hS]
  have hdig := E.hdig z hz
  generalize E.α * z + E.d z = w at hw hdig ⊢
  obtain ⟨h1, h2⟩ := oddPart_spec _ hw
  refine ⟨h1, h2, ?_⟩
  apply Decidable.byContradiction
  intro hlt
  obtain ⟨q, hq⟩ := hdig
  apply h1
  have e : pow pi E.k = pow pi (val w) * pow pi (E.k - val w) := by
    rw [← pow_add]; congr 1; omega
  rw [e, mul_assoc] at hq
  have hq' := pow_pi_cancel (h2.symm.trans hq)
  obtain ⟨m, hm⟩ : ∃ m, E.k - val w = m + 1 := ⟨E.k - val w - 1, by omega⟩
  rw [hq', hm, pow_succ', mul_assoc]
  exact ⟨_, rfl⟩

/-- Certificate for one step of `S`. -/
theorem S_eq (E : Expr) {z y : GInt} (v : Nat) (hz : Odd z)
    (h : E.α * z + E.d z = pow pi v * y) (hy : Odd y) : E.S z = y := by
  unfold S; rw [ite_eq_right hz, h]
  exact (oddPart_pow_mul v y hy).1

/-- Lemma 1.4(c): under symmetry, `S` commutes with units. -/
theorem S_commutes (E : Expr) (hs : E.Symmetric) : Commutes E.S := by
  intro u z hu
  by_cases hz : pi ∣ z
  · unfold S; rw [ite_eq_left hz, ite_eq_left (pi_dvd_mul_of_dvd u hz), mul_zero']
  · have huz : ¬ pi ∣ u * z := odd_unit_mul hu hz
    unfold S; rw [ite_eq_right hz, ite_eq_right huz, hs u z hu hz, ← oddPart_unit_mul hu]
    congr 1
    ext <;> simp <;> grind

/-- Lemma 1.2(c): for `k ≥ 1` digits are odd. -/
theorem d_odd (E : Expr) (hk : 1 ≤ E.k) (z : GInt) (hz : Odd z) : Odd (E.d z) := by
  intro hd
  have h1 : pi ∣ E.α * z + E.d z := by
    obtain ⟨q, hq⟩ := E.hdig z hz
    obtain ⟨m, hm⟩ : ∃ m, E.k = m + 1 := ⟨E.k - 1, by omega⟩
    rw [hm, pow_succ', mul_assoc] at hq
    exact ⟨_, hq⟩
  have h2 : pi ∣ E.α * z := by
    have := dvd_sub' h1 hd
    have e : E.α * z + E.d z - E.d z = E.α * z := by ext <;> simp
    rwa [e] at this
  rcases pi_dvd_mul h2 with h | h
  · exact E.hα h
  · exact hz h

/-- Lemma 1.2(c): one digit per class: congruent digits are equal. -/
theorem d_inj (E : Expr) {z z' : GInt} (hz : Odd z) (hz' : Odd z')
    (h : pow pi E.k ∣ E.d z - E.d z') : E.d z = E.d z' := by
  have h1 := dvd_sub' (dvd_sub' (E.hdig z hz) (E.hdig z' hz')) h
  have e : E.α * z + E.d z - (E.α * z' + E.d z') - (E.d z - E.d z') = E.α * (z - z') := by
    rw [mul_sub]; ext <;> simp <;> omega
  rw [e] at h1
  exact E.hclass z z' (pow_dvd_of_odd_mul E.hα E.k h1)

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
