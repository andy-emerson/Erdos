import GaussianCollatz.Quotient
import GaussianCollatz.Abstract
import GaussianCollatz.AbstractNorm
/-!
# The ring-specific theorems as corollaries of the abstract layer

`ℤ[i]` with its four units `i^a` (`U4 : FinAction (Fin 4) GInt`) and `ℤ` with `±1`
(`Z2 : FinAction (Fin 2) Int`) are instances of `Abstract.FinAction`. The bridge lemmas
identify orbits with associates, `Respects` with `RespectsAssoc`, `Equivariant` with
`Commutes`, and the abstract iteration and Collatz property with those of `Dynamics.lean`
and `Quotient.lean`. The theorems below have the statements of the earlier `ℤ[i]` theorems
(`transfer`, `trivial_cycles_le_four`, `trivial_transfer`, `lift_cycle`, `commuting_stab`,
the unit-cycle iterate, the rotation and mirror corollaries, Theorem 5.1) and are proved
from the abstract theorems only. New: the number of trivial cycles of a commuting map
divides `4`, and on `ℤ` Barina's conjecture is equivalent to `7x ± 1` being a Collatz map on
`ℤ \ {0}`.
-/
namespace GInt

/-! ## The four units as a finite group acting on `ℤ[i]` -/

/-- `u4 a = i^a`. -/
def u4 (g : Fin 4) : GInt := if g = 0 then 1 else if g = 1 then I else if g = 2 then -1 else -I

theorem u4_add (a b : Fin 4) : u4 (a + b) = u4 a * u4 b := by revert a b; decide

theorem u4_unit (g : Fin 4) : IsUnit (u4 g) := by revert g; decide

theorem u4_inj {a b : Fin 4} (h : u4 a = u4 b) : a = b := by revert a b h; decide

/-- The index of a unit. -/
def idx (u : GInt) : Fin 4 := if u = 1 then 0 else if u = I then 1 else if u = -1 then 2 else 3

theorem u4_idx {u : GInt} (hu : IsUnit u) : u4 (idx u) = u := by
  rcases hu with rfl | rfl | rfl | rfl <;> decide

/-- `ℤ[i]` with the action of its unit group. -/
def U4 : Abstract.FinAction (Fin 4) GInt where
  mul := (· + ·)
  one := 0
  inv := fun a => -a
  mul_assoc := by decide
  one_mul := by decide
  mul_one := by decide
  inv_mul := by decide
  mul_inv := by decide
  elems := [0, 1, 2, 3]
  nodup := by decide
  complete := by decide
  act := fun g z => u4 g * z
  act_one := fun z => one_mul z
  act_mul := fun g h z => by show u4 (g + h) * z = u4 g * (u4 h * z); rw [u4_add, mul_assoc]

theorem U4_card : U4.card = 4 := rfl

theorem U4_orb_iff {x y : GInt} : U4.Orb x y ↔ Assoc x y := by
  constructor
  · rintro ⟨g, rfl⟩; exact ⟨u4 g, u4_unit g, rfl⟩
  · rintro ⟨u, hu, rfl⟩; exact ⟨idx u, by show u * x = u4 (idx u) * x; rw [u4_idx hu]⟩

theorem U4_orb_one {y : GInt} : U4.Orb 1 y ↔ IsUnit y := by
  rw [U4_orb_iff]
  exact ⟨fun h => unit_of_assoc (Or.inl rfl) h, fun h => assoc_units (Or.inl rfl) h⟩

theorem U4_respects_iff (f : GInt → GInt) : U4.Respects f ↔ RespectsAssoc f := by
  constructor
  · intro h u z hu
    have := h (idx u) z
    rw [U4_orb_iff] at this
    simpa [U4, u4_idx hu] using this
  · intro h g z
    rw [U4_orb_iff]; exact h (u4 g) z (u4_unit g)

theorem U4_equivariant_iff (f : GInt → GInt) : U4.Equivariant f ↔ Commutes f := by
  constructor
  · intro h u z hu
    have := h (idx u) z
    simpa [U4, u4_idx hu] using this
  · intro h g z; exact h (u4 g) z (u4_unit g)

theorem U4_free {z : GInt} (hz : z ≠ 0) : U4.Free z := fun _ _ h =>
  u4_inj (mul_right_cancel_of_ne_zero h hz)

theorem u4_gpow (g : Fin 4) (m : Nat) : u4 (U4.gpow g m) = pow (u4 g) m := by
  induction m with
  | zero => rfl
  | succ m ih => show u4 (U4.gpow g m + g) = pow (u4 g) m * u4 g; rw [u4_add, ih]

theorem iter_eq (f : GInt → GInt) (n : Nat) (z : GInt) : Abstract.iter f n z = iter f n z := by
  induction n generalizing z with
  | zero => rfl
  | succ n ih => exact ih (f z)

theorem periodic_iff (f : GInt → GInt) (x : GInt) : Abstract.Periodic f x ↔ Periodic f x := by
  simp only [Abstract.Periodic, Periodic, iter_eq]

theorem reaches_iff (f : GInt → GInt) (x y : GInt) : Abstract.Reaches f x y ↔ Reaches f x y := by
  simp only [Abstract.Reaches, Reaches, iter_eq]

theorem onTrivial_iff (f : GInt → GInt) (x : GInt) :
    Abstract.OnTrivial U4 1 f x ↔ OnTrivialCycle f x := by
  simp only [Abstract.OnTrivial, OnTrivialCycle, periodic_iff, U4_orb_one, iter_eq]

theorem collatzOn_iff (P : GInt → Prop) (f : GInt → GInt) :
    Abstract.CollatzOn U4 1 P f ↔ CollatzOn P f := by
  simp only [Abstract.CollatzOn, CollatzOn, onTrivial_iff, iter_eq]

theorem qiter_cls_iff (f : GInt → GInt) (h : RespectsAssoc f) (n : Nat) (x y : GInt) :
    qiter (descend f h) n (cls x) = cls y ↔ Assoc (iter f n x) y := by
  rw [descend_iter, cls_eq_iff]

theorem aqiter_cls_iff (f : GInt → GInt) (h : U4.Respects f) (n : Nat) (x y : GInt) :
    Abstract.qiter U4 (Abstract.descend U4 f h) n (U4.cls x) = U4.cls y ↔
      Assoc (iter f n x) y := by
  rw [Abstract.descend_iter, U4.cls_eq_iff, U4_orb_iff, iter_eq]

/-! ## Corollaries: the earlier `ℤ[i]` theorems from the abstract layer -/

/-- `transfer` (Theorem 3.2 of `quotient.md`), from `Abstract.transfer_orbit`. -/
theorem transfer_of_abstract (f : GInt → GInt) (h : RespectsAssoc f) (P : GInt → Prop)
    (hP1 : P 1) : CollatzOn P f ↔ QCollatzOn P (descend f h) := by
  rw [← collatzOn_iff, Abstract.transfer_orbit U4 f ((U4_respects_iff f).2 h) 1 P hP1]
  unfold QCollatzOn
  simp only [qiter_cls_iff, U4_orb_iff, iter_eq]
  constructor
  · rintro ⟨h1, p, hp, h2⟩
    exact ⟨fun z hz => (h1 z hz).imp fun _ => assoc_symm, p, hp, assoc_symm h2⟩
  · rintro ⟨h1, p, hp, h2⟩
    exact ⟨fun z hz => (h1 z hz).imp fun _ => assoc_symm, p, hp, assoc_symm h2⟩

/-- `trivial_cycles_le_four` (Theorem 2.1(a)), from `Abstract.trivial_cycles_le`. -/
theorem trivial_cycles_le_four_of_abstract (f : GInt → GInt) (L : List GInt)
    (hL : ∀ x ∈ L, OnTrivialCycle f x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches f x y)) : L.length ≤ 4 :=
  Abstract.trivial_cycles_le U4 1 f L (fun x hx => (onTrivial_iff f x).2 (hL x hx))
    (hdist.imp fun h h' => h ((reaches_iff f _ _).1 h'))

/-- **New.** For a map commuting with the units, with `1` periodic, the number of trivial
cycles divides `4`: if `L` lists one unit on each trivial cycle, `|L| ∣ 4`. -/
theorem trivial_cycles_dvd_four {f : GInt → GInt} (hf : Commutes f) (h1 : Periodic f 1)
    (L : List GInt) (hLu : ∀ l ∈ L, IsUnit l) (hdist : L.Pairwise (fun x y => ¬ Reaches f x y))
    (hcover : ∀ u, IsUnit u → ∃ l ∈ L, Reaches f l u) : L.length ∣ 4 := by
  have hf' := (U4_equivariant_iff f).2 hf
  have hact : ∀ g : Fin 4, U4.act g 1 = u4 g := fun g => mul_one _
  have hmapL : ∀ l ∈ L, U4.act (idx l) 1 = l := fun l hl => by rw [hact, u4_idx (hLu l hl)]
  have := Abstract.trivial_cycles_dvd U4 hf' ((periodic_iff f 1).2 h1) (L.map idx) ?_ ?_
  · rw [List.length_map, U4_card] at this; exact this
  · rw [List.pairwise_map]
    refine hdist.imp_of_mem ?_
    intro a b ha hb hab h'
    rw [hmapL a ha, hmapL b hb, reaches_iff] at h'
    exact hab h'
  · intro g
    obtain ⟨l, hl, hr⟩ := hcover (u4 g) (u4_unit g)
    refine ⟨idx l, List.mem_map.2 ⟨l, hl, rfl⟩, ?_⟩
    rw [hmapL l hl, hact, reaches_iff]; exact hr

/-- `trivial_transfer` (Lemma 4.3), from `Abstract.equivariant_agree_collatz`. -/
theorem trivial_transfer_of_abstract {f g : GInt → GInt} (hg : Commutes g)
    (h : ∀ z, ∃ ε, IsUnit ε ∧ g z = ε * f z) : Trivial f → Trivial g := by
  have key := Abstract.equivariant_agree_collatz U4 ((U4_equivariant_iff g).2 hg)
    (fun x => U4_orb_iff.2 (h x)) 1 Odd odd_one
  intro hT
  rw [trivial_iff_collatzOn, ← collatzOn_iff] at hT ⊢
  exact key.2.2 hT

/-- `lift_cycle` (Theorem 4.1), from `Abstract.lift_cycle`. -/
theorem lift_cycle_of_abstract (f : GInt → GInt) (h : RespectsAssoc f) {z : GInt} {p : Nat}
    (hz : qiter (descend f h) p (cls z) = cls z) :
    (∃ ε, IsUnit ε ∧ iter f p z = ε * z) ∧ (∀ y, Assoc z y → Assoc z (iter f p y)) ∧
    ∃ y, Assoc z y ∧ ∃ r, 0 < r ∧ r ≤ 4 ∧ iter f (p * r) y = y := by
  have h' := (U4_respects_iff f).2 h
  have hz' : Abstract.qiter U4 (Abstract.descend U4 f h') p (U4.cls z) = U4.cls z :=
    (aqiter_cls_iff f h' p z z).2 ((qiter_cls_iff f h p z z).1 hz)
  obtain ⟨⟨g, hg⟩, hmap, y, hy, r, hr, hr4, hry⟩ := Abstract.lift_cycle U4 f h' hz'
  refine ⟨⟨u4 g, u4_unit g, by rw [← iter_eq]; exact hg⟩, fun y hy => ?_,
    y, U4_orb_iff.1 hy, r, hr, hr4, by rw [← iter_eq]; exact hry⟩
  have := hmap y (U4_orb_iff.2 hy)
  rw [U4_orb_iff, iter_eq] at this; exact this

/-- `commuting_stab` (Theorem 4.2(2)), from `Abstract.equivariant_stab_free`. -/
theorem commuting_stab_of_abstract {f : GInt → GInt} (hf : Commutes f) {z ε : GInt} {p : Nat}
    (hz : z ≠ 0) (hp : 0 < p) (hε : IsUnit ε) (he : iter f p z = ε * z)
    (hmin : ∀ t, 0 < t → t < p → qiter (descend f hf.respects) t (cls z) ≠ cls z)
    {v : GInt} (hv : IsUnit v) : Reaches f z (v * z) ↔ ∃ m, v = pow ε m := by
  have hf' := (U4_equivariant_iff f).2 hf
  have he' : Abstract.iter f p z = U4.act (idx ε) z := by
    rw [iter_eq, he]; show ε * z = u4 (idx ε) * z; rw [u4_idx hε]
  have hmin' : ∀ t, 0 < t → t < p →
      Abstract.qiter U4 (Abstract.descend U4 f (Abstract.Equivariant.respects U4 hf'))
        t (U4.cls z) ≠ U4.cls z := by
    intro t h0 ht hq
    apply hmin t h0 ht
    exact (qiter_cls_iff f _ t z z).2
      ((aqiter_cls_iff f (Abstract.Equivariant.respects U4 hf') t z z).1 hq)
  have key := Abstract.equivariant_stab_free U4 hf' (U4_free hz) hp he' hmin' (idx v)
  have hvz : U4.act (idx v) z = v * z := by show u4 (idx v) * z = v * z; rw [u4_idx hv]
  rw [hvz, reaches_iff] at key
  rw [key]
  constructor
  · rintro ⟨m, hm⟩
    refine ⟨m, ?_⟩
    have := congrArg u4 hm
    rwa [u4_idx hv, u4_gpow, u4_idx hε] at this
  · rintro ⟨m, hm⟩
    refine ⟨m, u4_inj ?_⟩
    rw [u4_idx hv, u4_gpow, u4_idx hε, hm]

/-- The unit-cycle iterate of Theorem 3.1, `f^n u = u w^n`, from `Abstract.orbit_iter`. -/
theorem unit_iter_of_abstract {f : GInt → GInt} (hf : Commutes f) (hw : IsUnit (f 1))
    {u : GInt} (hu : IsUnit u) (n : Nat) : iter f n u = u * pow (f 1) n := by
  have hf' := (U4_equivariant_iff f).2 hf
  have hw' : f 1 = U4.act (idx (f 1)) 1 := by
    show f 1 = u4 (idx (f 1)) * 1; rw [u4_idx hw, mul_one]
  have := Abstract.orbit_iter U4 hf' hw' (idx u) n
  rw [iter_eq] at this
  have e1 : U4.act (idx u) 1 = u := by show u4 (idx u) * 1 = u; rw [mul_one, u4_idx hu]
  rw [e1] at this
  rw [this]
  show u4 (idx u) * (u4 (U4.gpow (idx (f 1)) n) * 1) = u * pow (f 1) n
  rw [mul_one, u4_gpow, u4_idx hu, u4_idx hw]

namespace Expr

/-- `rotate_rule_of_respects` (Corollary 6.1), from `Abstract.agree_collatz`. -/
theorem rotate_rule_of_respects' (E : Expr) (h : RespectsAssoc E.S) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).IsRule ↔ E.IsRule := by
  unfold IsRule
  rw [trivial_iff_collatzOn, trivial_iff_collatzOn, ← collatzOn_iff, ← collatzOn_iff]
  refine Abstract.agree_collatz U4 ((U4_respects_iff _).2 h) (fun z => ?_) 1 Odd odd_one
  rw [U4_orb_iff, rotate_S]; exact h u z hu

/-- `mirror_rule_of_respects` (Corollary 6.2), from `Abstract.conj_collatz` with `τ = conj`. -/
theorem mirror_rule_of_respects' (E : Expr) (h : RespectsAssoc E.S) :
    E.mirror.IsRule ↔ E.IsRule := by
  unfold IsRule
  rw [trivial_iff_collatzOn, trivial_iff_collatzOn, ← collatzOn_iff, ← collatzOn_iff]
  refine (Abstract.conj_collatz U4 conj conj conj_conj conj_conj
    (fun x y hxy => U4_orb_iff.2 (assoc_conj (U4_orb_iff.1 hxy)))
    (fun x y hxy => U4_orb_iff.2 (assoc_conj (U4_orb_iff.1 hxy)))
    (by rw [show conj 1 = 1 by decide]; exact U4.orb_refl 1)
    (fun _ hx => odd_conj hx) (fun _ hx => odd_conj hx) odd_one
    ((U4_respects_iff _).2 h) (fun z => ?_)).2
  rw [U4_orb_iff, mirror_S]; exact assoc_unit_mul (pow_unit unit_negI _) _

end Expr

/-! ## Theorem 5.1 from the abstract norm version -/

/-- `ℤ[i]` as a commutative ring in the sense of core's `Lean.Grind.CommRing`. -/
instance instCommRingGInt : Lean.Grind.CommRing GInt where
  add := (· + ·)
  mul := (· * ·)
  neg := Neg.neg
  sub := Sub.sub
  natCast := ⟨fun n => ⟨n, 0⟩⟩
  ofNat := fun n => ⟨⟨n, 0⟩⟩
  nsmul := ⟨fun n z => ⟨n * z.re, n * z.im⟩⟩
  npow := ⟨fun z n => pow z n⟩
  intCast := ⟨fun i => ⟨i, 0⟩⟩
  zsmul := ⟨fun i z => ⟨i * z.re, i * z.im⟩⟩
  add_zero := by intro a; ext <;> simp
  add_comm := by intro a b; ext <;> simp <;> omega
  add_assoc := by intro a b c; ext <;> simp <;> omega
  mul_assoc := mul_assoc
  mul_one := mul_one
  one_mul := one_mul
  left_distrib := by intro a b c; ext <;> simp <;> grind
  right_distrib := by intro a b c; ext <;> simp <;> grind
  zero_mul := by intro a; ext <;> simp
  mul_zero := by intro a; ext <;> simp
  pow_zero := fun _ => rfl
  pow_succ := fun _ _ => rfl
  ofNat_succ := by intro n; ext <;> simp
  ofNat_eq_natCast := fun _ => rfl
  nsmul_eq_natCast_mul := by
    intro n a
    show (⟨n * a.re, n * a.im⟩ : GInt) = (⟨(n : Int), 0⟩ : GInt) * a
    ext <;> simp
  neg_add_cancel := by intro a; ext <;> simp <;> omega
  sub_eq_add_neg := by intro a b; ext <;> simp <;> omega
  neg_zsmul := by
    intro i a
    show (⟨-i * a.re, -i * a.im⟩ : GInt) = -(⟨i * a.re, i * a.im⟩ : GInt)
    ext <;> simp <;> grind
  zsmul_natCast_eq_nsmul := fun _ _ => rfl
  intCast_ofNat := fun _ => rfl
  intCast_neg := by
    intro i
    show (⟨-i, 0⟩ : GInt) = -(⟨i, 0⟩ : GInt)
    ext <;> simp
  mul_comm := mul_comm

/-- The norm data of `ℤ[i]`: `N`, the four units, `π = 1 + i`. -/
def gaussNorm : Abstract.NormData GInt where
  N := N
  N_mul := N_mul
  N_one := by decide
  N_zero := by decide
  N_pos := fun _ h => N_pos h
  isU := IsUnit
  N_unit := fun _ hu => N_unit hu
  pi := pi
  N_pi := by decide

namespace Expr

/-- `symmetric_of_T_respects` (Theorem 5.1 for `T`), from `Abstract.NormData.commutes_of_T_respects`
with the big-point lemma `Expr.big_point`. -/
theorem symmetric_of_T_respects' (E : Expr)
    (h : ∀ u z, IsUnit u → Odd z → Assoc (E.T z) (E.T (u * z))) : E.Symmetric := by
  refine gaussNorm.commutes_of_T_respects E.α E.d Odd ?_ ?_
  · intro z0 u B hz0 hu
    obtain ⟨z, hz, hd, hdu, hB, -⟩ := E.big_point hz0 hu B
    exact ⟨z, hz, hd, hdu, hB⟩
  · intro u z hu hz
    have hT1 : E.T z = E.α * z + E.d z := by simp [T, show ¬ pi ∣ z from hz]
    have hT2 : E.T (u * z) = E.α * (u * z) + E.d (u * z) := by
      simp [T, show ¬ pi ∣ u * z from odd_unit_mul hu hz]
    obtain ⟨ε, hε, heq⟩ := h u z hu hz
    rw [hT1, hT2] at heq
    exact ⟨ε, hε, heq⟩

/-- `symmetric_of_S_respects` (Theorem 5.1 for `S`), from
`Abstract.NormData.commutes_of_S_respects` with `oddPart`, `val` and `Expr.big_point`. -/
theorem symmetric_of_S_respects' (E : Expr)
    (h : ∀ u z, IsUnit u → Odd z → Assoc (E.S z) (E.S (u * z))) : E.Symmetric := by
  refine gaussNorm.commutes_of_S_respects E.α E.d Odd oddPart val
    (fun w hw => (oddPart_spec w hw).2) (fun z0 u B hz0 hu => E.big_point hz0 hu B) ?_
  intro u z hu hz
  have hS1 : E.S z = oddPart (E.α * z + E.d z) := by simp [S, show ¬ pi ∣ z from hz]
  have hS2 : E.S (u * z) = oddPart (E.α * (u * z) + E.d (u * z)) := by
    simp [S, show ¬ pi ∣ u * z from odd_unit_mul hu hz]
  obtain ⟨ε, hε, heq⟩ := h u z hu hz
  rw [hS1, hS2] at heq
  exact ⟨ε, hε, heq⟩

end Expr

/-! ## Same statements

Each corollary has exactly the type of the earlier theorem: the equalities below typecheck
only if the two statements are identical (and then hold by proof irrelevance). -/

example : @transfer = @transfer_of_abstract := rfl
example : @trivial_cycles_le_four = @trivial_cycles_le_four_of_abstract := rfl
example : @trivial_transfer = @trivial_transfer_of_abstract := rfl
example : @lift_cycle = @lift_cycle_of_abstract := rfl
example : @commuting_stab = @commuting_stab_of_abstract := rfl
example : @Expr.rotate_rule_of_respects = @Expr.rotate_rule_of_respects' := rfl
example : @Expr.mirror_rule_of_respects = @Expr.mirror_rule_of_respects' := rfl
example : @Expr.symmetric_of_T_respects = @Expr.symmetric_of_T_respects' := rfl
example : @Expr.symmetric_of_S_respects = @Expr.symmetric_of_S_respects' := rfl

end GInt

/-! ## `ℤ` with `±1` -/
namespace IntCollatz

/-- `z2 0 = 1`, `z2 1 = -1`. -/
def z2 (g : Fin 2) : Int := if g = 0 then 1 else -1

theorem z2_add (a b : Fin 2) : z2 (a + b) = z2 a * z2 b := by revert a b; decide

/-- `ℤ` with the action of `{±1}`. -/
def Z2 : Abstract.FinAction (Fin 2) Int where
  mul := (· + ·)
  one := 0
  inv := fun a => -a
  mul_assoc := by decide
  one_mul := by decide
  mul_one := by decide
  inv_mul := by decide
  mul_inv := by decide
  elems := [0, 1]
  nodup := by decide
  complete := by decide
  act := fun g z => z2 g * z
  act_one := fun z => Int.one_mul z
  act_mul := fun g h z => by show z2 (g + h) * z = z2 g * (z2 h * z); rw [z2_add, Int.mul_assoc]

theorem Z2_orb_one {y : Int} : Z2.Orb 1 y ↔ y = 1 ∨ y = -1 := by
  constructor
  · rintro ⟨g, rfl⟩
    revert g; decide
  · rintro (rfl | rfl)
    · exact ⟨0, rfl⟩
    · exact ⟨1, rfl⟩

theorem B7_equivariant : Z2.Equivariant B7 := by
  intro g z
  show B7 (z2 g * z) = z2 g * B7 z
  by_cases hg : g = 0
  · subst hg; show B7 (1 * z) = 1 * B7 z; rw [Int.one_mul, Int.one_mul]
  · have : z2 g = -1 := by simp [z2, hg]
    rw [this, Int.neg_one_mul, Int.neg_one_mul, B7_commutes]

theorem B7_iter_pos (m : Nat) (n : Int) (hn : 0 < n) : 0 < Abstract.iter B7 m n := by
  induction m generalizing n with
  | zero => exact hn
  | succ m ih => exact ih _ (B7_pos n hn)

/-- **Barina's conjecture as a Collatz property on `ℤ`** (catalog A14, now in Lean): every
positive orbit of `7x ± 1` reaches `1` iff `7x ± 1` is a Collatz map on `ℤ \ {0}`, with
trivial cycles the cycles through `±1`. From `Abstract.transfer_orbit` with `G = {±1}`. -/
theorem barina_iff :
    (∀ n : Int, 0 < n → ∃ m, Abstract.iter B7 m n = 1) ↔
      Abstract.CollatzOn Z2 1 (fun z => z ≠ 0) B7 := by
  rw [Abstract.transfer_orbit Z2 B7 (Abstract.Equivariant.respects Z2 B7_equivariant) 1 _
    (by decide)]
  constructor
  · intro hB
    refine ⟨fun z hz => ?_, 4, by decide, Z2_orb_one.2 (Or.inl (by decide))⟩
    rcases Int.lt_or_gt_of_ne hz with hneg | hpos
    · obtain ⟨m, hm⟩ := hB (-z) (by omega)
      refine ⟨m, Z2_orb_one.2 (Or.inr ?_)⟩
      have := Abstract.equivariant_iter Z2 B7_equivariant 1 m (-z)
      have e1 : Z2.act 1 (-z) = z := by show z2 1 * -z = z; simp [z2]
      rw [e1, hm] at this
      rw [this]; rfl
    · obtain ⟨m, hm⟩ := hB z hpos
      exact ⟨m, Z2_orb_one.2 (Or.inl hm)⟩
  · rintro ⟨h1, -⟩ n hn
    obtain ⟨m, hm⟩ := h1 n (by omega)
    refine ⟨m, ?_⟩
    rcases Z2_orb_one.1 hm with h | h
    · exact h
    · have := B7_iter_pos m n hn; omega

end IntCollatz
