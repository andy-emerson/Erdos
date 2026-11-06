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



/-! ## Units and iteration -/

instance (u : GInt) : Decidable (IsUnit u) := by unfold IsUnit; infer_instance

/-- Complex conjugation (on units it is the inverse). -/
def conj (z : GInt) : GInt := ⟨z.re, -z.im⟩

@[simp] theorem conj_re (z : GInt) : (conj z).re = z.re := rfl
@[simp] theorem conj_im (z : GInt) : (conj z).im = -z.im := rfl

theorem unit_mul {u v : GInt} (hu : IsUnit u) (hv : IsUnit v) : IsUnit (u * v) := by
  rcases hu with rfl | rfl | rfl | rfl <;> rcases hv with rfl | rfl | rfl | rfl <;> decide

theorem unit_conj {u : GInt} (hu : IsUnit u) : IsUnit (conj u) := by
  rcases hu with rfl | rfl | rfl | rfl <;> decide

theorem conj_mul_self {u : GInt} (hu : IsUnit u) : conj u * u = 1 := by
  rcases hu with rfl | rfl | rfl | rfl <;> decide

theorem pow_unit {u : GInt} (hu : IsUnit u) (n : Nat) : IsUnit (pow u n) := by
  induction n with
  | zero => exact Or.inl rfl
  | succ n ih => exact unit_mul ih hu

theorem pow_four_unit {u : GInt} (hu : IsUnit u) : pow u 4 = 1 := by
  rcases hu with rfl | rfl | rfl | rfl <;> decide

theorem mul_left_comm (a b c : GInt) : a * (b * c) = b * (a * c) := by
  rw [← mul_assoc, mul_comm a b, mul_assoc]

theorem unit_cancel {u a b : GInt} (hu : IsUnit u) (h : u * a = u * b) : a = b := by
  have h2 := congrArg (fun x => conj u * x) h
  simp only [← mul_assoc, conj_mul_self hu, one_mul] at h2
  exact h2

theorem iter_succ' (f : GInt → GInt) (n : Nat) (z : GInt) :
    iter f (n + 1) z = f (iter f n z) := by
  rw [iter_add]; rfl

theorem reaches_trans {f : GInt → GInt} {x y z : GInt} :
    Reaches f x y → Reaches f y z → Reaches f x z
  | ⟨a, ha⟩, ⟨b, hb⟩ => ⟨a + b, by rw [iter_add, ha, hb]⟩

theorem iter_period {f : GInt → GInt} {p : Nat} {x : GInt} (h : iter f p x = x) (m : Nat) :
    iter f (m * p) x = x := by
  induction m with
  | zero => rw [Nat.zero_mul]; rfl
  | succ m ih => rw [Nat.succ_mul, iter_add, ih, h]

/-- Lemma 1.4(a): on a cycle, every point reaches every other. -/
theorem reaches_back {f : GInt → GInt} {x y : GInt} (hx : Periodic f x) (h : Reaches f x y) :
    Reaches f y x := by
  obtain ⟨p, hp, hpx⟩ := hx
  obtain ⟨j, rfl⟩ := h
  refine ⟨j * p - j, ?_⟩
  have hle : j ≤ j * p := Nat.le_mul_of_pos_right j hp
  rw [← iter_add, show j + (j * p - j) = j * p by omega, iter_period hpx]

theorem periodic_of_reaches {f : GInt → GInt} {x y : GInt} (hx : Periodic f x)
    (h : Reaches f x y) : Periodic f y := by
  obtain ⟨p, hp, hpx⟩ := hx
  obtain ⟨j, rfl⟩ := h
  exact ⟨p, hp, by rw [← iter_add, Nat.add_comm, iter_add, hpx]⟩

theorem iter_u {f : GInt → GInt} (hf : Commutes f) {u : GInt} (hu : IsUnit u) (n : Nat) :
    iter f n u = u * iter f n 1 := by
  have := iter_unit_mul f hf hu n 1
  rwa [mul_one] at this

theorem reaches_rel {f : GInt → GInt} (hf : Commutes f) {a b : GInt} (ha : IsUnit a)
    (h : Reaches f a b) : Reaches f 1 (conj a * b) := by
  obtain ⟨j, hj⟩ := h
  refine ⟨j, ?_⟩
  rw [← hj, iter_u hf ha, ← mul_assoc, conj_mul_self ha, one_mul]

/-! ## Theorem 2.1: the trivial-cycle bound -/

/-- **Theorem 2.1(a).** Any map has at most `|U| = 4` trivial cycles: a list of points on
trivial cycles, no two on the same cycle, has length at most 4. -/
theorem trivial_cycles_le_four (f : GInt → GInt) (L : List GInt)
    (hL : ∀ x ∈ L, OnTrivialCycle f x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches f x y)) : L.length ≤ 4 := by
  classical
  let g : GInt → GInt := fun x =>
    if h : OnTrivialCycle f x then iter f (Classical.choose h.2) x else 1
  have hg : ∀ x ∈ L, IsUnit (g x) ∧ Reaches f x (g x) := by
    intro x hx
    have h := hL x hx
    simp only [g, h, ↓reduceDIte]
    exact ⟨Classical.choose_spec h.2, _, rfl⟩
  have hnd : (L.map g).Nodup := by
    unfold List.Nodup
    rw [List.pairwise_map]
    refine hdist.imp_of_mem ?_
    intro a b ha hb hab heq
    apply hab
    obtain ⟨_, hra⟩ := hg a ha
    obtain ⟨_, hrb⟩ := hg b hb
    refine reaches_trans hra ?_
    rw [heq]; exact reaches_back (hL b hb).1 hrb
  have hsub : L.map g ⊆ [1, I, -1, -I] := by
    intro y hy
    obtain ⟨x, hx, rfl⟩ := List.mem_map.1 hy
    rcases (hg x hx).1 with h | h | h | h <;> simp [h]
  have := hnd.length_le_of_subset hsub
  simpa using this

/-- Every trivial cycle is the cycle of a unit (any map). -/
theorem trivial_cycle_has_unit (f : GInt → GInt) (x : GInt) (h : OnTrivialCycle f x) :
    ∃ u, IsUnit u ∧ Reaches f x u ∧ Reaches f u x := by
  obtain ⟨hper, j, hu⟩ := h
  exact ⟨iter f j x, hu, ⟨j, rfl⟩, reaches_back hper ⟨j, rfl⟩⟩

/-- **Theorem 2.1(b), first part.** Under symmetry a unit is periodic iff `1` is. -/
theorem unit_periodic_iff {f : GInt → GInt} (hf : Commutes f) {u : GInt} (hu : IsUnit u) :
    Periodic f u ↔ Periodic f 1 := by
  constructor
  · rintro ⟨p, hp, h⟩
    refine ⟨p, hp, unit_cancel hu ?_⟩
    rw [← iter_u hf hu, h, mul_one]
  · rintro ⟨p, hp, h⟩
    exact ⟨p, hp, by rw [iter_u hf hu, h, mul_one]⟩

/-- **Theorem 2.1(b), item 1.** The cycle of a unit `u` is `u C₁`. -/
theorem unit_cycle_eq {f : GInt → GInt} (hf : Commutes f) {u : GInt} (hu : IsUnit u) (y : GInt) :
    Reaches f u y ↔ ∃ c, Reaches f 1 c ∧ y = u * c := by
  constructor
  · rintro ⟨j, rfl⟩
    exact ⟨iter f j 1, ⟨j, rfl⟩, iter_u hf hu j⟩
  · rintro ⟨c, ⟨j, rfl⟩, rfl⟩
    exact ⟨j, iter_u hf hu j⟩

/-- **Theorem 2.1(b), item 3.** `u C₁ = C₁` (as sets) iff `u ∈ C₁`. -/
theorem stab_iff {f : GInt → GInt} (hf : Commutes f) (h1 : Periodic f 1) {u : GInt}
    (hu : IsUnit u) :
    (∀ y, (∃ c, Reaches f 1 c ∧ y = u * c) ↔ Reaches f 1 y) ↔ Reaches f 1 u := by
  constructor
  · intro h
    exact (h u).1 ⟨1, ⟨0, rfl⟩, (mul_one u).symm⟩
  · intro h y
    rw [← unit_cycle_eq hf hu y]
    constructor
    · intro hy; exact reaches_trans h hy
    · intro hy; exact reaches_trans (reaches_back h1 h) hy

/-- `U ∩ C₁` is closed under products. -/
theorem stab_mul {f : GInt → GInt} (hf : Commutes f) {u v : GInt} (hu : IsUnit u)
    (h1 : Reaches f 1 u) (h2 : Reaches f 1 v) : Reaches f 1 (u * v) := by
  obtain ⟨a, ha⟩ := h1
  obtain ⟨b, hb⟩ := h2
  exact ⟨a + b, by rw [iter_add, ha, iter_u hf hu, hb]⟩

/-- **Theorem 2.1(b), item 4.** Under symmetry, with `1` periodic, exactly one of:
`U ∩ C₁ = {1}` and the cycles of `1, i, -1, -i` are pairwise distinct (4 trivial cycles);
`U ∩ C₁ = {±1}`, `C₁ = -C₁`, `iC₁ = -iC₁ ≠ C₁` (2 trivial cycles);
`U ∩ C₁ = U` (1 trivial cycle). By `trivial_cycle_has_unit` and `unit_cycle_eq` these are
all the trivial cycles. -/
theorem trivial_cycle_count {f : GInt → GInt} (hf : Commutes f) (_h1 : Periodic f 1) :
    (¬ Reaches f 1 I ∧ ¬ Reaches f 1 (-1) ∧ ¬ Reaches f 1 (-I) ∧
      [1, I, -1, -I].Pairwise (fun x y => ¬ Reaches f x y)) ∨
    (Reaches f 1 (-1) ∧ ¬ Reaches f 1 I ∧ ¬ Reaches f 1 (-I) ∧ Reaches f I (-I)) ∨
    (Reaches f 1 I ∧ Reaches f 1 (-1) ∧ Reaches f 1 (-I)) := by
  have uI : IsUnit I := by decide
  have um1 : IsUnit (-1 : GInt) := by decide
  have umI : IsUnit (-I) := by decide
  by_cases hI : Reaches f 1 I
  · have h2 : Reaches f 1 (-1) := by
      have := stab_mul hf uI hI hI; rwa [show I * I = -1 by decide] at this
    have h3 : Reaches f 1 (-I) := by
      have := stab_mul hf um1 h2 hI; rwa [show (-1) * I = -I by decide] at this
    exact Or.inr (Or.inr ⟨hI, h2, h3⟩)
  by_cases hmI : Reaches f 1 (-I)
  · have h2 : Reaches f 1 (-1) := by
      have := stab_mul hf umI hmI hmI; rwa [show (-I) * (-I) = -1 by decide] at this
    have h3 : Reaches f 1 I := by
      have := stab_mul hf um1 h2 hmI; rwa [show (-1) * (-I) = I by decide] at this
    exact absurd h3 hI
  by_cases hm1 : Reaches f 1 (-1)
  · refine Or.inr (Or.inl ⟨hm1, hI, hmI, ?_⟩)
    exact (unit_cycle_eq hf uI _).2 ⟨-1, hm1, by decide⟩
  · refine Or.inl ⟨hI, hm1, hmI, ?_⟩
    have key : ∀ a b, IsUnit a → (conj a * b = I ∨ conj a * b = -1 ∨ conj a * b = -I) →
        ¬ Reaches f a b := by
      intro a b ha hab h
      have h' := reaches_rel hf ha h
      rcases hab with e | e | e <;> rw [e] at h' <;> contradiction
    refine List.Pairwise.cons ?_ (List.Pairwise.cons ?_ (List.Pairwise.cons ?_
      (List.Pairwise.cons ?_ List.Pairwise.nil))) <;> intro b hb <;>
      simp only [List.mem_cons, List.not_mem_nil, or_false] at hb
    · rcases hb with rfl | rfl | rfl <;> apply key <;> decide
    · rcases hb with rfl | rfl <;> apply key <;> decide
    · subst hb; apply key <;> decide

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
  have h1 : ∀ u, IsUnit u → f u = u * f 1 := by
    intro u hu; have := hf u 1 hu; rwa [mul_one] at this
  have h2 : ∀ u n, IsUnit u → iter f n u = u * pow (f 1) n := by
    intro u n hu
    induction n with
    | zero => exact (mul_one u).symm
    | succ n ih =>
      rw [iter_succ', ih, h1 _ (unit_mul hu (pow_unit hw n)), mul_assoc]; rfl
  refine ⟨h1, h2, ?_, ?_, ?_⟩
  · intro u n hu; rw [h2 u n hu]; exact unit_mul hu (pow_unit hw n)
  · intro u n hu
    rw [h2 u n hu]
    constructor
    · intro h; exact unit_cancel hu (by rw [h, mul_one])
    · intro h; rw [h, mul_one]
  · rcases hw with h | h | h | h <;> rw [h]
    · refine ⟨by decide, by decide, ?_⟩
      intro n h0 hn; rw [show ord 1 = 1 by decide] at hn; omega
    · refine ⟨by decide, by decide, ?_⟩
      intro n h0 hn; rw [show ord I = 4 by decide] at hn
      rcases (by omega : n = 1 ∨ n = 2 ∨ n = 3) with rfl | rfl | rfl <;> decide
    · refine ⟨by decide, by decide, ?_⟩
      intro n h0 hn; rw [show ord (-1) = 2 by decide] at hn
      rcases (by omega : n = 1) with rfl; decide
    · refine ⟨by decide, by decide, ?_⟩
      intro n h0 hn; rw [show ord (-I) = 4 by decide] at hn
      rcases (by omega : n = 1 ∨ n = 2 ∨ n = 3) with rfl | rfl | rfl <;> decide

/-- **Theorem 3.1(2).** If `f 1` is not a unit, no unit maps to a unit. -/
theorem no_unit_cycle {f : GInt → GInt} (hf : Commutes f) (hw : ¬ IsUnit (f 1)) :
    ∀ u, IsUnit u → ¬ IsUnit (f u) := by
  intro u hu h
  apply hw
  have e : f u = u * f 1 := by have := hf u 1 hu; rwa [mul_one] at this
  rw [e] at h
  have := unit_mul (unit_conj hu) h
  rwa [← mul_assoc, conj_mul_self hu, one_mul] at this

/-! ## Lemma 4.3 and an obstruction -/

/-- **Lemma 4.3 (transfer).** If `g` commutes with units and `g z` is a unit times `f z`
for every `z`, then triviality of `f` implies triviality of `g` (no hypothesis on `f`). -/
theorem trivial_transfer {f g : GInt → GInt} (hg : Commutes g)
    (h : ∀ z, ∃ ε, IsUnit ε ∧ g z = ε * f z) : Trivial f → Trivial g := by
  intro hT z hz
  have hit : ∀ n z, ∃ δ, IsUnit δ ∧ iter g n z = δ * iter f n z := by
    intro n
    induction n with
    | zero => intro z; exact ⟨1, Or.inl rfl, (one_mul _).symm⟩
    | succ n ih =>
      intro z
      obtain ⟨ε, hε, he⟩ := h z
      obtain ⟨δ, hδ, hd⟩ := ih (f z)
      refine ⟨ε * δ, unit_mul hε hδ, ?_⟩
      show iter g n (g z) = ε * δ * iter f n (f z)
      rw [he, iter_unit_mul g hg hε, hd, mul_assoc]
  obtain ⟨n, ⟨p, hp, hpy⟩, j, hj⟩ := hT z hz
  obtain ⟨δ, hδ, hx⟩ := hit n z
  refine ⟨n, ⟨4 * p, by omega, ?_⟩, j, ?_⟩
  · obtain ⟨δ', hδ', h'⟩ := hit p (iter f n z)
    have hgp : iter g p (iter g n z) = δ' * iter g n z := by
      rw [hx, iter_unit_mul g hg hδ, h', hpy, mul_left_comm]
    have hm : ∀ m, iter g (m * p) (iter g n z) = pow δ' m * iter g n z := by
      intro m
      induction m with
      | zero => rw [Nat.zero_mul]; exact (one_mul _).symm
      | succ m ih =>
        rw [Nat.succ_mul, iter_add, ih, iter_unit_mul g hg (pow_unit hδ' m), hgp,
          ← mul_assoc]; rfl
    rw [hm 4, pow_four_unit hδ', one_mul]
  · obtain ⟨δ'', hδ'', h''⟩ := hit j (iter f n z)
    rw [hx, iter_unit_mul g hg hδ, h'']
    exact unit_mul hδ (unit_mul hδ'' hj)

/-- A set `P` mapped into itself, with no units and an odd element, rules out triviality. -/
theorem not_trivial_of_invariant (f : GInt → GInt) (P : GInt → Prop)
    (hP : ∀ x, P x → P (f x)) (hU : ∀ x, P x → ¬ IsUnit x) (z : GInt) (hz : Odd z)
    (hPz : P z) : ¬ Trivial f := by
  intro hT
  obtain ⟨n, _, j, hj⟩ := hT z hz
  have hiter : ∀ m x, P x → P (iter f m x) := by
    intro m
    induction m with
    | zero => intro x hx; exact hx
    | succ m ih => intro x hx; exact ih _ (hP x hx)
  exact hU _ (hiter _ _ (hiter _ _ hPz)) hj

end GInt
