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

/-- Cancellation of a nonzero factor. -/
theorem mul_right_cancel_of_ne_zero {a b z : GInt} (h : a * z = b * z) (hz : z ≠ 0) : a = b := by
  have h1 := congrArg GInt.re h
  have h2 := congrArg GInt.im h
  simp only [mul_re, mul_im] at h1 h2
  have e : (a - b) * z = 0 := by ext <;> simp <;> grind
  have hN := congrArg N e
  rw [N_mul] at hN
  have hz' := N_pos hz
  have h0 : N (a - b) = 0 := by
    rcases Nat.mul_eq_zero.1 hN with h | h
    · exact h
    · exact absurd h (by show N z ≠ 0; omega)
  have := N_eq_zero h0
  have r := congrArg GInt.re this
  have i := congrArg GInt.im this
  simp at r i
  ext <;> omega

/-- **Proposition 2.2(4).** If `f 0 = 0`, the quotient map of an associate-respecting `f` is
also the quotient map of a map commuting with the units (built from a choice of one
representative per class). -/
theorem exists_commuting_same_descend (f : GInt → GInt) (h : RespectsAssoc f) (h0 : f 0 = 0) :
    ∃ g : GInt → GInt, ∃ hg : Commutes g, descend g hg.respects = descend f h := by
  classical
  let rep : GInt → GInt := fun z => Classical.choose (Quot.exists_rep (cls z))
  have hrep : ∀ z, Assoc (rep z) z := fun z =>
    cls_eq_iff.1 (show cls (rep z) = cls z from Classical.choose_spec (Quot.exists_rep (cls z)))
  let μ : GInt → GInt := fun z => Classical.choose (hrep z)
  have hμ : ∀ z, IsUnit (μ z) ∧ z = μ z * rep z := fun z => Classical.choose_spec (hrep z)
  have rep_mul : ∀ u z, IsUnit u → rep (u * z) = rep z := by
    intro u z hu
    show Classical.choose (Quot.exists_rep (cls (u * z))) = Classical.choose (Quot.exists_rep (cls z))
    rw [cls_unit_mul hu]
  have mu_mul : ∀ u z, IsUnit u → z ≠ 0 → μ (u * z) = u * μ z := by
    intro u z hu hz
    have h1 := (hμ (u * z)).2
    rw [rep_mul u z hu] at h1
    have h2 : u * z = (u * μ z) * rep z := by
      rw [mul_assoc, ← (hμ z).2]
    have hr : rep z ≠ 0 := by
      intro e; apply hz; rw [(hμ z).2, e, mul_zero']
    exact mul_right_cancel_of_ne_zero (h1.symm.trans h2) hr
  have unit_ne : ∀ u z, IsUnit u → z ≠ 0 → u * z ≠ 0 := by
    intro u z hu hz e
    apply hz
    have := congrArg (fun x => conj u * x) e
    simp only [← mul_assoc, conj_mul_self hu, one_mul, mul_zero'] at this
    exact this
  let g : GInt → GInt := fun z => if z = 0 then 0 else μ z * f (rep z)
  have hg : Commutes g := by
    intro u z hu
    by_cases hz : z = 0
    · subst hz
      simp only [g, mul_zero', ite_true]
    · simp only [g, hz, unit_ne u z hu hz, ite_false]
      rw [mu_mul u z hu hz, rep_mul u z hu, mul_assoc]
  refine ⟨g, hg, ?_⟩
  refine descend_unique f h _ ?_
  intro z
  show cls (g z) = cls (f z)
  by_cases hz : z = 0
  · subst hz; simp only [g, ite_true, h0]
  · simp only [g, hz, ite_false]
    rw [cls_unit_mul (hμ z).1]
    exact cls_eq_iff.2 ((respects_iff f).1 h _ _ (hrep z))

/-! ## Iteration on the quotient -/

/-- Iteration on the quotient. -/
def qiter (F : Cls → Cls) : Nat → Cls → Cls
  | 0, c => c
  | n + 1, c => qiter F n (F c)

theorem qiter_add (F : Cls → Cls) (m n : Nat) (c : Cls) :
    qiter F (m + n) c = qiter F n (qiter F m c) := by
  induction m generalizing c with
  | zero => simp [qiter]
  | succ m ih => rw [Nat.add_right_comm, qiter, ih, qiter]

/-- `T̄^n [z] = [T^n z]`. -/
theorem descend_iter (f : GInt → GInt) (h : RespectsAssoc f) (n : Nat) (z : GInt) :
    qiter (descend f h) n (cls z) = cls (iter f n z) := by
  induction n generalizing z with
  | zero => rfl
  | succ n ih => show qiter _ n (descend f h (cls z)) = cls (iter f n (f z)); rw [descend_cls, ih]

theorem iter_iter (f : GInt → GInt) (p i : Nat) (z : GInt) :
    iter (iter f p) i z = iter f (p * i) z := by
  induction i generalizing z with
  | zero => rw [Nat.mul_zero]; rfl
  | succ i ih =>
    show iter (iter f p) i (iter f p z) = _
    rw [ih, ← iter_add, Nat.mul_succ, Nat.add_comm]

/-- Pigeonhole on a class: if `g` maps the class of `z` into itself, two of
`z, g z, …, g⁴ z` coincide. -/
theorem pigeon (g : GInt → GInt) (z : GInt) (hg : ∀ x, Assoc z x → Assoc z (g x)) :
    ∃ i j, i < j ∧ j ≤ 4 ∧ iter g i z = iter g j z := by
  have hA : ∀ i, Assoc z (iter g i z) := by
    intro i
    induction i with
    | zero => exact assoc_refl z
    | succ i ih => rw [iter_succ']; exact hg _ ih
  have hmem : ∀ i, iter g i z ∈ [z, I * z, -1 * z, -I * z] := by
    intro i
    obtain ⟨u, hu, e⟩ := hA i
    rw [e]
    rcases hu with rfl | rfl | rfl | rfl <;> simp [one_mul]
  apply Classical.byContradiction
  intro hne
  have H : ∀ i j, i < j → j ≤ 4 → iter g i z ≠ iter g j z :=
    fun i j h1 h2 he => hne ⟨i, j, h1, h2, he⟩
  have hnd : [iter g 0 z, iter g 1 z, iter g 2 z, iter g 3 z, iter g 4 z].Nodup := by
    simp only [List.nodup_cons, List.mem_cons, List.not_mem_nil, or_false, not_or,
      List.nodup_nil, and_true]
    refine ⟨⟨?_, ?_, ?_, ?_⟩, ⟨?_, ?_, ?_⟩, ⟨?_, ?_⟩, ?_, not_false⟩ <;> apply H <;> omega
  have hsub : [iter g 0 z, iter g 1 z, iter g 2 z, iter g 3 z, iter g 4 z] ⊆
      [z, I * z, -1 * z, -I * z] := by
    intro x hx
    simp only [List.mem_cons, List.not_mem_nil, or_false] at hx
    rcases hx with e | e | e | e | e <;> rw [e] <;> exact hmem _
  have := hnd.length_le_of_subset hsub
  simp at this

/-! ## The transfer theorem (Theorem 3.2) -/

/-- `f` is a Collatz map on the points satisfying `P`: every such point has an iterate on a
cycle containing a unit. `CollatzOn Odd f` is `Trivial f`. -/
def CollatzOn (P : GInt → Prop) (f : GInt → GInt) : Prop :=
  ∀ z, P z → ∃ n, OnTrivialCycle f (iter f n z)

theorem trivial_iff_collatzOn (f : GInt → GInt) : Trivial f ↔ CollatzOn Odd f := Iff.rfl

/-- Collatz's sense on the quotient: every class reaches `[1]`, and `[1]` is periodic. -/
def QCollatzOn (P : GInt → Prop) (F : Cls → Cls) : Prop :=
  (∀ z, P z → ∃ n, qiter F n (cls z) = cls 1) ∧ ∃ p, 0 < p ∧ qiter F p (cls 1) = cls 1

/-- **Theorem 3.2 (transfer).** For `f` respecting associates and any `P` with `P 1`:
`f` is a Collatz map on `P` iff the induced map is a Collatz map on `P/U` in Collatz's sense. -/
theorem transfer (f : GInt → GInt) (h : RespectsAssoc f) (P : GInt → Prop) (hP1 : P 1) :
    CollatzOn P f ↔ QCollatzOn P (descend f h) := by
  constructor
  · intro hC
    refine ⟨?_, ?_⟩
    · intro z hz
      obtain ⟨n, _, j, hj⟩ := hC z hz
      exact ⟨n + j, by rw [descend_iter, iter_add]; exact cls_one_iff.2 hj⟩
    · obtain ⟨n, hper, j, hj⟩ := hC 1 hP1
      obtain ⟨q, hq, hqw⟩ := periodic_of_reaches hper ⟨j, rfl⟩
      refine ⟨q, hq, ?_⟩
      rw [← cls_one_iff.2 hj, descend_iter, hqw]
  · rintro ⟨hreach, p, hp, hper⟩ z hz
    have hU : ∀ v, IsUnit v → IsUnit (iter f p v) := by
      intro v hv
      apply cls_one_iff.1
      rw [← descend_iter f h, cls_one_iff.2 hv, hper]
    obtain ⟨n, hn⟩ := hreach z hz
    rw [descend_iter] at hn
    have hv := cls_one_iff.1 hn
    obtain ⟨i, j, hij, _, he⟩ := pigeon (iter f p) (iter f n z)
      (fun x hx => assoc_units hv (hU x (unit_of_assoc hv hx)))
    have hstay : ∀ m x, IsUnit x → IsUnit (iter (iter f p) m x) := by
      intro m
      induction m with
      | zero => intro x hx; exact hx
      | succ m ih => intro x hx; exact ih _ (hU x hx)
    have e1 : iter f (n + p * i) z = iter (iter f p) i (iter f n z) := by
      rw [iter_add, iter_iter]
    refine ⟨n + p * i, ⟨p * (j - i), Nat.mul_pos hp (by omega), ?_⟩, 0, ?_⟩
    · rw [e1, ← iter_iter, ← iter_add, show i + (j - i) = j by omega, ← he]
    · show IsUnit (iter f (n + p * i) z)
      rw [e1]; exact hstay i _ hv

/-- **Theorem 3.2 for expressions.** If `S` respects associates, the expression is a rule iff
the induced map on odd classes sends every odd class to `[1]` and `[1]` is periodic. -/
theorem Expr.rule_iff_quotient (E : Expr) (h : RespectsAssoc E.S) :
    E.IsRule ↔ QCollatzOn Odd (descend E.S h) :=
  transfer E.S h Odd odd_one

/-- The same for `T`, on all nonzero points. -/
theorem transfer_nonzero (f : GInt → GInt) (h : RespectsAssoc f) :
    CollatzOn (fun z => z ≠ 0) f ↔ QCollatzOn (fun z => z ≠ 0) (descend f h) :=
  transfer f h _ (by decide)

/-! ## Lifting cycles (Theorems 4.1 and 4.2) -/

/-- **Theorem 4.1(1), existence part of (3).** If `[z]` has period `p` under the induced map,
then `f^p z = ε z`, `f^p` maps the class of `z` into itself, and some point of the class is
`f`-periodic with period `p r`, `1 ≤ r ≤ 4`. -/
theorem lift_cycle (f : GInt → GInt) (h : RespectsAssoc f) {z : GInt} {p : Nat}
    (hz : qiter (descend f h) p (cls z) = cls z) :
    (∃ ε, IsUnit ε ∧ iter f p z = ε * z) ∧ (∀ y, Assoc z y → Assoc z (iter f p y)) ∧
    ∃ y, Assoc z y ∧ ∃ r, 0 < r ∧ r ≤ 4 ∧ iter f (p * r) y = y := by
  have hmap : ∀ y, Assoc z y → Assoc z (iter f p y) := by
    intro y hy
    apply assoc_symm
    apply cls_eq_iff.1
    rw [← descend_iter f h, ← cls_eq_iff.2 hy, hz]
  refine ⟨hmap z (assoc_refl z), hmap, ?_⟩
  obtain ⟨i, j, hij, hj, he⟩ := pigeon (iter f p) z hmap
  have hA : ∀ m, Assoc z (iter (iter f p) m z) := by
    intro m
    induction m with
    | zero => exact assoc_refl z
    | succ m ih => rw [iter_succ']; exact hmap _ ih
  refine ⟨iter (iter f p) i z, hA i, j - i, by omega, by omega, ?_⟩
  rw [← iter_iter, ← iter_add, show i + (j - i) = j by omega, ← he]

theorem qiter_mul (F : Cls → Cls) {p : Nat} {c : Cls} (h : qiter F p c = c) (m : Nat) :
    qiter F (p * m) c = c := by
  induction m with
  | zero => rfl
  | succ m ih => rw [Nat.mul_succ, qiter_add, ih, h]

/-- A period of a point is a multiple of its least period. -/
theorem qperiod_dvd (F : Cls → Cls) {c : Cls} {p q : Nat} (hp : 0 < p)
    (hpc : qiter F p c = c) (hmin : ∀ t, 0 < t → t < p → qiter F t c ≠ c)
    (hq : qiter F q c = c) : p ∣ q := by
  have e : q = p * (q / p) + q % p := (Nat.div_add_mod q p).symm
  have h1 : qiter F (q % p) c = c := by
    rw [e, qiter_add, qiter_mul F hpc] at hq; exact hq
  apply Nat.dvd_of_mod_eq_zero
  apply Classical.byContradiction
  intro hne
  exact hmin _ (by omega) (Nat.mod_lt q hp) h1

/-- **Theorem 4.1(2).** Every `f`-period of a point over a `T̄`-cycle of least period `p` is a
multiple of `p`. -/
theorem lift_period_dvd (f : GInt → GInt) (h : RespectsAssoc f) {z y : GInt} {p q : Nat}
    (hp : 0 < p) (hpc : qiter (descend f h) p (cls z) = cls z)
    (hmin : ∀ t, 0 < t → t < p → qiter (descend f h) t (cls z) ≠ cls z)
    (hy : Assoc z y) (hq : iter f q y = y) : p ∣ q := by
  apply qperiod_dvd _ hp hpc hmin
  rw [cls_eq_iff.2 hy, descend_iter, hq]

/-- **Theorem 4.2(1).** For a commuting map with `f^p z = ε z`: `f^{pm}(v z) = ε^m v z`. -/
theorem commuting_lift {f : GInt → GInt} (hf : Commutes f) {z ε : GInt} {p : Nat}
    (hε : IsUnit ε) (he : iter f p z = ε * z) (v : GInt) (hv : IsUnit v) (m : Nat) :
    iter f (p * m) (v * z) = pow ε m * (v * z) := by
  induction m with
  | zero => exact (one_mul _).symm
  | succ m ih =>
    rw [Nat.mul_succ, iter_add, ih, iter_unit_mul f hf (pow_unit hε m),
      iter_unit_mul f hf hv, he]
    show pow ε m * (v * (ε * z)) = pow ε m * ε * (v * z)
    rw [mul_assoc, mul_left_comm v ε z]

/-- **Theorem 4.2(1).** Every point of the class is periodic (period dividing `4p`). -/
theorem commuting_lift_periodic {f : GInt → GInt} (hf : Commutes f) {z ε : GInt} {p : Nat}
    (hε : IsUnit ε) (he : iter f p z = ε * z) (v : GInt) (hv : IsUnit v) :
    iter f (p * 4) (v * z) = v * z := by
  rw [commuting_lift hf hε he v hv 4, pow_four_unit hε, one_mul]

/-- **Theorem 4.2(2).** For a commuting map, `z ≠ 0`, `p` the least period of `[z]` and
`f^p z = ε z`: the cycle of `z` contains `v z` iff `v` is a power of `ε`. So the stabilizer
of the cycle in `U` is `⟨ε⟩`, and the cycles over `[z]` are its `4 / ord ε` translates. -/
theorem commuting_stab {f : GInt → GInt} (hf : Commutes f) {z ε : GInt} {p : Nat}
    (hz : z ≠ 0) (hp : 0 < p) (hε : IsUnit ε) (he : iter f p z = ε * z)
    (hmin : ∀ t, 0 < t → t < p → qiter (descend f hf.respects) t (cls z) ≠ cls z)
    {v : GInt} (hv : IsUnit v) : Reaches f z (v * z) ↔ ∃ m, v = pow ε m := by
  have hpc : qiter (descend f hf.respects) p (cls z) = cls z := by
    rw [descend_iter, he, cls_unit_mul hε]
  have h1 := commuting_lift hf hε he 1 (Or.inl rfl)
  simp only [one_mul] at h1
  constructor
  · rintro ⟨i, hi⟩
    have hq : qiter (descend f hf.respects) i (cls z) = cls z := by
      rw [descend_iter, hi, cls_unit_mul hv]
    obtain ⟨m, rfl⟩ := qperiod_dvd _ hp hpc hmin hq
    refine ⟨m, mul_right_cancel_of_ne_zero ?_ hz⟩
    rw [← hi, h1 m]
  · rintro ⟨m, rfl⟩
    exact ⟨p * m, h1 m⟩

/-- **Theorem 4.1(4).** An associate-respecting map whose fixed class `[1]` lifts to a cycle
of length `3`, which divides no unit order, with the unit `i` off the cycle. -/
def ex3 (z : GInt) : GInt :=
  if z = 1 then -I else if z = -I then -1 else if z = -1 then 1 else if z = I then -I else z

theorem ex3_respects : RespectsAssoc ex3 := by
  intro u z hu
  have hunit : ∀ x, IsUnit x → IsUnit (ex3 x) := by
    intro x hx; rcases hx with rfl | rfl | rfl | rfl <;> decide
  have hfix : ∀ x, ¬ IsUnit x → ex3 x = x := by
    intro x hx
    unfold IsUnit at hx
    simp only [not_or] at hx
    obtain ⟨h1, h2, h3, h4⟩ := hx
    simp only [ex3, h1, h2, h3, h4, ite_false]
  by_cases hz : IsUnit z
  · exact assoc_units (hunit z hz) (hunit _ (unit_mul hu hz))
  · have hnu : ¬ IsUnit (u * z) := by
      intro h'
      apply hz
      have := unit_mul (unit_conj hu) h'
      rwa [← mul_assoc, conj_mul_self hu, one_mul] at this
    rw [hfix z hz, hfix _ hnu]
    exact assoc_unit_mul hu z

theorem ex3_cycle :
    descend ex3 ex3_respects (cls 1) = cls 1 ∧ iter ex3 3 1 = 1 ∧ iter ex3 1 1 ≠ 1 ∧
    iter ex3 2 1 ≠ 1 ∧ ¬ Periodic ex3 I := by
  refine ⟨?_, by decide, by decide, by decide, ?_⟩
  · rw [descend_cls]; exact cls_one_iff.2 (by decide)
  · have key : ∀ n, iter ex3 (n + 1) I = 1 ∨ iter ex3 (n + 1) I = -I ∨ iter ex3 (n + 1) I = -1 := by
      intro n
      induction n with
      | zero => decide
      | succ n ih =>
        rw [iter_succ']
        rcases ih with h | h | h <;> rw [h] <;> decide
    rintro ⟨p, hp, h⟩
    obtain ⟨q, rfl⟩ : ∃ q, p = q + 1 := ⟨p - 1, by omega⟩
    rcases key q with e | e | e <;> rw [e] at h <;> revert h <;> decide

/-! ## Rotation and reflection (Corollaries 6.1 and 6.2) -/

theorem assoc_conj {x y : GInt} : Assoc x y → Assoc (conj x) (conj y) := by
  rintro ⟨u, hu, rfl⟩; exact ⟨conj u, unit_conj hu, conj_mul u x⟩

/-- Conjugation on the quotient, `[z] ↦ [z̄]`. -/
def qconj : Cls → Cls :=
  Quotient.lift (fun z => cls (conj z)) (fun _ _ hab => cls_eq_iff.2 (assoc_conj hab))

@[simp] theorem qconj_cls (z : GInt) : qconj (cls z) = cls (conj z) := rfl

theorem qconj_qconj (c : Cls) : qconj (qconj c) = c := by
  induction c using Quotient.ind with
  | _ z => show cls (conj (conj z)) = cls z; rw [conj_conj]

namespace Expr

/-- **Corollary 6.1.** If `S_α` respects associates, so does `S_{uα}`. -/
theorem rotate_respects (E : Expr) (h : RespectsAssoc E.S) (u : GInt) (hu : IsUnit u) :
    RespectsAssoc (E.rotate u hu).S := by
  intro v z hv
  rw [rotate_S, rotate_S, mul_left_comm u v z]
  exact h v (u * z) hv

/-- **Corollary 6.1.** The rotated map induces the same map on the quotient. -/
theorem rotate_descend (E : Expr) (h : RespectsAssoc E.S) (u : GInt) (hu : IsUnit u) :
    descend (E.rotate u hu).S (E.rotate_respects h u hu) = descend E.S h :=
  (descend_unique _ _ (descend E.S h)
    (fun z => by rw [descend_cls, rotate_S]; exact cls_eq_iff.2 (h u z hu))).symm

/-- **Corollary 6.1.** For an associate-respecting `S_α` (no symmetry assumed), the rotated
expression is a rule iff the original is. -/
theorem rotate_rule_of_respects (E : Expr) (h : RespectsAssoc E.S) (u : GInt) (hu : IsUnit u) :
    (E.rotate u hu).IsRule ↔ E.IsRule := by
  have t1 := (E.rotate u hu).rule_iff_quotient (E.rotate_respects h u hu)
  rw [E.rotate_descend h u hu] at t1
  exact t1.trans (E.rule_iff_quotient h).symm

/-- **Corollary 6.2.** If `S_α` respects associates, so does the mirrored `S_ᾱ`. -/
theorem mirror_respects (E : Expr) (h : RespectsAssoc E.S) : RespectsAssoc E.mirror.S := by
  intro u z hu
  rw [mirror_S, mirror_S, conj_mul]
  have h1 := assoc_conj (h (conj u) (conj z) (unit_conj hu))
  exact assoc_trans (assoc_symm (assoc_unit_mul (pow_unit unit_negI _) _))
    (assoc_trans h1 (assoc_unit_mul (pow_unit unit_negI _) _))

/-- **Corollary 6.2.** The mirror induces the conjugate of the induced map by `qconj`. -/
theorem mirror_descend (E : Expr) (h : RespectsAssoc E.S) :
    descend E.mirror.S (E.mirror_respects h) = fun c => qconj (descend E.S h (qconj c)) := by
  funext c
  induction c using Quotient.ind with
  | _ z =>
    show cls (E.mirror.S z) = cls (conj (E.S (conj z)))
    rw [mirror_S, cls_unit_mul (pow_unit unit_negI _)]

end Expr

theorem qiter_qconj (F : Cls → Cls) (n : Nat) (c : Cls) :
    qiter (fun c => qconj (F (qconj c))) n c = qconj (qiter F n (qconj c)) := by
  induction n generalizing c with
  | zero => exact (qconj_qconj c).symm
  | succ n ih =>
    show qiter _ n (qconj (F (qconj c))) = qconj (qiter F n (F (qconj c)))
    rw [ih, qconj_qconj]

theorem qcollatz_qconj {F : Cls → Cls} (hF : QCollatzOn Odd F) :
    QCollatzOn Odd (fun c => qconj (F (qconj c))) := by
  obtain ⟨hr, p, hp, hper⟩ := hF
  refine ⟨fun z hz => ?_, p, hp, ?_⟩
  · obtain ⟨n, hn⟩ := hr (conj z) (odd_conj hz)
    exact ⟨n, by rw [qiter_qconj, qconj_cls, hn]; rfl⟩
  · rw [qiter_qconj]
    show qconj (qiter F p (cls (conj 1))) = cls 1
    rw [show conj 1 = 1 by decide, hper]; rfl

/-- **Corollary 6.2.** For an associate-respecting `S_α` (no symmetry assumed), the mirrored
expression is a rule iff the original is. -/
theorem Expr.mirror_rule_of_respects (E : Expr) (h : RespectsAssoc E.S) :
    E.mirror.IsRule ↔ E.IsRule := by
  rw [E.mirror.rule_iff_quotient (E.mirror_respects h), E.rule_iff_quotient h,
    E.mirror_descend h]
  constructor
  · intro h1
    have := qcollatz_qconj h1
    simp only [qconj_qconj] at this
    exact this
  · exact qcollatz_qconj

/-! ## Theorem 5.1: for expressions, respecting associates is commuting -/

theorem mul_conj_self (b : GInt) : b * conj b = ⟨(N b : Int), 0⟩ := by
  ext <;> simp [N_cast] <;> grind

theorem N_unit {u : GInt} (hu : IsUnit u) : N u = 1 := by
  rcases hu with rfl | rfl | rfl | rfl <;> decide

/-- `N W ≤ N e` when `c W = e` with `c ≠ 0`. -/
theorem N_le_of_mul_eq {c W e : GInt} (hc : c ≠ 0) (h : c * W = e) : N W ≤ N e := by
  rw [← h, N_mul]
  exact Nat.le_mul_of_pos_left _ (N_pos hc)

theorem lt_N_of_re {B : Nat} {W : GInt} (h : B + 1 ≤ W.re.natAbs) : B < N W := by
  unfold N; have := Nat.le_mul_self W.re.natAbs; omega

theorem lt_N_of_im {B : Nat} {W : GInt} (h : B + 1 ≤ W.im.natAbs) : B < N W := by
  unfold N; have := Nat.le_mul_self W.im.natAbs; omega

/-- `ε π^{t+1} ≠ u` for units `ε, u`. -/
theorem unit_mul_pow_pi_ne {ε u : GInt} (hε : IsUnit ε) (hu : IsUnit u) (t : Nat) :
    ε * pow pi (t + 1) ≠ u := by
  intro e
  have := congrArg N e
  rw [N_mul, N_unit hε, N_unit hu, N_pow_pi, Nat.one_mul, Nat.pow_succ] at this
  have := Nat.two_pow_pos t
  omega

namespace Expr

/-- **Lemma 5.0'.** In the class of an odd `z₀` there is a point `z` (same digit, and the same
digit at `u z`) with `N(α z + d z) > B` and `N(α (u z) + d (u z)) > B`. -/
theorem big_point (E : Expr) {z0 : GInt} (hz0 : Odd z0) {u : GInt} (hu : IsUnit u) (B : Nat) :
    ∃ z, Odd z ∧ E.d z = E.d z0 ∧ E.d (u * z) = E.d (u * z0) ∧
      B < N (E.α * z + E.d z) ∧ B < N (E.α * (u * z) + E.d (u * z)) := by
  obtain ⟨P, hPdef⟩ : ∃ P, P = pow pi (E.k + 1) := ⟨_, rfl⟩
  obtain ⟨w0, hw0⟩ : ∃ w, w = E.α * z0 + E.d z0 := ⟨_, rfl⟩
  obtain ⟨w1, hw1⟩ : ∃ w, w = E.α * (u * z0) + E.d (u * z0) := ⟨_, rfl⟩
  obtain ⟨n, hn⟩ : ∃ n : Int,
      n = ((w0.re.natAbs + w0.im.natAbs + w1.re.natAbs + w1.im.natAbs + B + 1 : Nat) : Int) :=
    ⟨_, rfl⟩
  let β := E.α * P
  let c : GInt := ⟨n, 0⟩
  let q := conj β * c
  let z := z0 + P * q
  have hP : P = pow pi E.k * pi := hPdef
  have hP' : P = pi * pow pi E.k := hPdef.trans (pow_succ' pi E.k)
  have hsub : z - z0 = pow pi E.k * (pi * q) := by
    rw [← mul_assoc, ← hP]; ext <;> simp [z] <;> omega
  have hd : E.d z = E.d z0 := E.hclass z z0 ⟨_, hsub⟩
  have hdu : E.d (u * z) = E.d (u * z0) := by
    apply E.hclass
    refine ⟨u * (pi * q), ?_⟩
    rw [← mul_sub, hsub, mul_left_comm]
  have hz : Odd z := by
    intro hpz
    apply hz0
    have h2 : pi ∣ P * q := ⟨pow pi E.k * q, by rw [hP', mul_assoc]⟩
    have := dvd_sub' hpz h2
    rwa [show z - P * q = z0 by ext <;> simp [z] <;> omega] at this
  have hβ : 1 ≤ N β := by
    show 1 ≤ N (E.α * P)
    rw [N_mul, hPdef, N_pow_pi]
    exact Nat.mul_pos (N_pos (odd_ne_zero E.hα)) (Nat.two_pow_pos _)
  have key : E.α * z = E.α * z0 + (β * conj β) * c := by
    show E.α * (z0 + P * (conj (E.α * P) * c)) = E.α * z0 + (E.α * P * conj (E.α * P)) * c
    ext <;> simp <;> grind
  have key2 : E.α * (u * z) = E.α * (u * z0) + u * ((β * conj β) * c) := by
    rw [mul_left_comm, key]
    ext <;> simp <;> grind
  rw [mul_conj_self] at key key2
  obtain ⟨M, hM⟩ : ∃ M : Int, M = (N β : Int) := ⟨_, rfl⟩
  rw [← hM] at key key2
  have hM1 : (1 : Int) ≤ M := by omega
  have hn0 : (0 : Int) ≤ n := by omega
  have hMn : n ≤ M * n := by
    have := Int.mul_le_mul_of_nonneg_right hM1 hn0
    simpa using this
  have e1 : E.α * z + E.d z = w0 + ⟨M * n, 0⟩ := by
    rw [hd, key, hw0]; ext <;> simp [c] <;> omega
  have e2 : E.α * (u * z) + E.d (u * z) = w1 + u * ⟨M * n, 0⟩ := by
    rw [hdu, key2, hw1]; ext <;> simp [c] <;> omega
  refine ⟨z, hz, hd, hdu, ?_, ?_⟩
  · rw [e1]; apply lt_N_of_re; simp only [add_re]; omega
  · rw [e2]
    rcases hu with rfl | rfl | rfl | rfl
    · apply lt_N_of_re; simp only [add_re, mul_re, one_re, one_im]; omega
    · apply lt_N_of_im; simp only [add_im, mul_im, I_re, I_im]; omega
    · apply lt_N_of_re; simp only [add_re, mul_re, neg_re, neg_im, one_re, one_im]; omega
    · apply lt_N_of_im; simp only [add_im, mul_im, neg_re, neg_im, I_re, I_im]; omega

end Expr

end GInt
