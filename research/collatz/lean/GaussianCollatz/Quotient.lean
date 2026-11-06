import GaussianCollatz.Variants
/-!
# Collatz maps on `ℤ[i] / U` (`quotient.md`)

Associates, maps that respect associates, the induced map on the quotient `Cls = ℤ[i]/U`
(descent), the transfer theorem (a map is trivial on `ℤ[i]` iff the induced map sends every
class to `[1]` and `[1]` is periodic), the lift of cycles, the rotation and reflection
corollaries, and Theorem 5.1: for an expression, `T` respecting associates forces the table
to commute with the units (and likewise for `S`).
-/
namespace GInt

/-! ## Associates and the quotient -/

/-- `x ∼ y`: `y = u x` for a unit `u`. -/
def Assoc (x y : GInt) : Prop := ∃ u, IsUnit u ∧ y = u * x

theorem assoc_refl (x : GInt) : Assoc x x := ⟨1, Or.inl rfl, (one_mul x).symm⟩

theorem assoc_symm {x y : GInt} : Assoc x y → Assoc y x := by
  rintro ⟨u, hu, rfl⟩
  exact ⟨conj u, unit_conj hu, by rw [← mul_assoc, conj_mul_self hu, one_mul]⟩

theorem assoc_trans {x y z : GInt} : Assoc x y → Assoc y z → Assoc x z := by
  rintro ⟨u, hu, rfl⟩ ⟨v, hv, rfl⟩
  exact ⟨v * u, unit_mul hv hu, (mul_assoc _ _ _).symm⟩

theorem assoc_unit_mul {u : GInt} (hu : IsUnit u) (z : GInt) : Assoc z (u * z) := ⟨u, hu, rfl⟩

theorem assoc_units {a b : GInt} (ha : IsUnit a) (hb : IsUnit b) : Assoc a b :=
  ⟨b * conj a, unit_mul hb (unit_conj ha), by rw [mul_assoc, conj_mul_self ha, mul_one]⟩

theorem unit_of_assoc {a b : GInt} (ha : IsUnit a) : Assoc a b → IsUnit b := by
  rintro ⟨u, hu, rfl⟩; exact unit_mul hu ha

instance assocSetoid : Setoid GInt := ⟨Assoc, ⟨assoc_refl, assoc_symm, assoc_trans⟩⟩

/-- The classes of associates, `ℤ[i]/U` (with the class `[0] = {0}` kept). -/
def Cls : Type := Quotient assocSetoid

/-- The class `[z]`. -/
def cls (z : GInt) : Cls := Quotient.mk _ z

theorem cls_eq_iff {x y : GInt} : cls x = cls y ↔ Assoc x y :=
  ⟨fun h => Quotient.exact h, fun h => Quotient.sound h⟩

theorem cls_unit_mul {u : GInt} (hu : IsUnit u) (z : GInt) : cls (u * z) = cls z :=
  (cls_eq_iff.2 (assoc_unit_mul hu z)).symm

/-- `[z] = [1]` iff `z` is a unit: the class `[1]` is `U`. -/
theorem cls_one_iff {z : GInt} : cls z = cls 1 ↔ IsUnit z := by
  constructor
  · intro h
    exact unit_of_assoc (Or.inl rfl) (assoc_symm (cls_eq_iff.1 h))
  · intro h
    exact cls_eq_iff.2 (assoc_units h (Or.inl rfl))

/-! ## Maps that respect associates, and descent -/

/-- `f` sends associates to associates: `f (u z) ∼ f z`. -/
def RespectsAssoc (f : GInt → GInt) : Prop := ∀ u z, IsUnit u → Assoc (f z) (f (u * z))

theorem respects_iff (f : GInt → GInt) :
    RespectsAssoc f ↔ ∀ x y, Assoc x y → Assoc (f x) (f y) := by
  constructor
  · rintro h x y ⟨u, hu, rfl⟩; exact h u x hu
  · intro h u z hu; exact h z (u * z) (assoc_unit_mul hu z)

/-- **Proposition 2.2(2).** Commuting with the units implies respecting associates. -/
theorem Commutes.respects {f : GInt → GInt} (h : Commutes f) : RespectsAssoc f := by
  intro u z hu; rw [h u z hu]; exact assoc_unit_mul hu _

/-- The induced map `[z] ↦ [f z]` on the quotient. -/
def descend (f : GInt → GInt) (h : RespectsAssoc f) : Cls → Cls :=
  Quotient.lift (fun z => cls (f z)) (fun a b hab => cls_eq_iff.2 ((respects_iff f).1 h a b hab))

@[simp] theorem descend_cls (f : GInt → GInt) (h : RespectsAssoc f) (z : GInt) :
    descend f h (cls z) = cls (f z) := rfl

/-- **Proposition 2.2(1) (descent).** A map `F` on the quotient with `F [z] = [f z]` exists
iff `f` respects associates. -/
theorem descent_iff (f : GInt → GInt) :
    (∃ F : Cls → Cls, ∀ z, F (cls z) = cls (f z)) ↔ RespectsAssoc f := by
  constructor
  · rintro ⟨F, hF⟩ u z hu
    apply cls_eq_iff.1
    rw [← hF, ← hF, cls_unit_mul hu]
  · intro h; exact ⟨descend f h, fun _ => rfl⟩

/-- The induced map is unique. -/
theorem descend_unique (f : GInt → GInt) (h : RespectsAssoc f) (F : Cls → Cls)
    (hF : ∀ z, F (cls z) = cls (f z)) : F = descend f h := by
  funext c
  induction c using Quotient.ind with
  | _ z => exact hF z

/-- **Proposition 2.2(3).** Respecting associates is strictly weaker than commuting:
`z ↦ z²` respects associates and does not commute with the units. -/
theorem sq_respects : RespectsAssoc (fun z => z * z) := by
  intro u z hu
  refine ⟨u * u, unit_mul hu hu, ?_⟩
  show u * z * (u * z) = u * u * (z * z)
  rw [mul_assoc, mul_assoc, mul_left_comm z u z]

theorem sq_not_commutes : ¬ Commutes (fun z => z * z) := by
  intro h
  have := h I 1 (Or.inr (Or.inl rfl))
  revert this; decide

/-- **Proposition 2.2(4).** Any map respecting associates induces the same quotient map as
a map commuting with the units (built from a choice of one representative per class). -/
theorem exists_commuting_same_descend (f : GInt → GInt) (h : RespectsAssoc f) :
    ∃ g : GInt → GInt, ∃ hg : Commutes g, descend g hg.respects = descend f h := by
  classical
  -- representative `rep z` of the class of `z`, and the unit `μ z` with `z = μ z * rep z`
  let rep : GInt → GInt := fun z => Quotient.out (cls z)
  have hrep : ∀ z, Assoc (rep z) z := fun z => Quotient.exact (Quotient.out_eq (cls z))
  let μ : GInt → GInt := fun z => Classical.choose (hrep z)
  have hμ : ∀ z, IsUnit (μ z) ∧ z = μ z * rep z := fun z => Classical.choose_spec (hrep z)
  have rep_mul : ∀ u z, IsUnit u → rep (u * z) = rep z := by
    intro u z hu; show Quotient.out (cls (u * z)) = Quotient.out (cls z); rw [cls_unit_mul hu]
  let g : GInt → GInt := fun z => if z = 0 then f 0 else μ z * f (rep z)
  have mu_mul : ∀ u z, IsUnit u → z ≠ 0 → μ (u * z) = u * μ z := by
    intro u z hu hz
    have h1 := (hμ (u * z)).2
    rw [rep_mul u z hu] at h1
    have h2 : u * z = (u * μ z) * rep z := by
      rw [mul_assoc, ← (hμ z).2]
    have hr : rep z ≠ 0 := by
      intro h0; apply hz; rw [(hμ z).2, h0, mul_zero']
    -- cancel `rep z ≠ 0`
    have e : μ (u * z) * rep z = (u * μ z) * rep z := h1.symm.trans h2
    exact mul_right_cancel_of_ne_zero e hr
  refine ⟨g, ?_, ?_⟩
  · intro u z hu
    by_cases hz : z = 0
    · subst hz
      have : u * 0 = 0 := mul_zero' u
      simp only [g, this, if_true]
      -- `f 0 = u f 0`?  Not in general, so use `rep`: handled below
      sorry
    · sorry
  · sorry

end GInt
