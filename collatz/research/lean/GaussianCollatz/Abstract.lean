/-!
# The abstract layer: a finite group acting on a set (`general-theory.md`, Sections 7 and 8)

A type `X`, a finite group `G` acting on `X` (given by its operations and a duplicate-free
list of all its elements, so that core Lean can count), a distinguished point `e` (for rings,
`X = R`, `G = U`, `e = 1`, so that the orbit `G e` is the set of units), and a map
`f : X → X`. Proved here once for all rings: descent, the transfer theorem, the trivial-cycle
bound and its divisor form, unit cycles, cycle lifts with the stabilizer, and the rotation and
conjugation statements. `AbstractGInt.lean` instantiates it with `ℤ[i]` and its four units,
and with `ℤ` and `±1`.
-/
namespace Abstract

/-- A finite group `G` acting on `X`. -/
structure FinAction (G X : Type) where
  mul : G → G → G
  one : G
  inv : G → G
  mul_assoc : ∀ a b c, mul (mul a b) c = mul a (mul b c)
  one_mul : ∀ a, mul one a = a
  mul_one : ∀ a, mul a one = a
  inv_mul : ∀ a, mul (inv a) a = one
  mul_inv : ∀ a, mul a (inv a) = one
  elems : List G
  nodup : elems.Nodup
  complete : ∀ g, g ∈ elems
  act : G → X → X
  act_one : ∀ x, act one x = x
  act_mul : ∀ g h x, act (mul g h) x = act g (act h x)

variable {G X : Type}

/-- Iteration: `iter f 0 x = x`, `iter f (n+1) x = iter f n (f x)`. -/
def iter (f : X → X) : Nat → X → X
  | 0, x => x
  | n + 1, x => iter f n (f x)

/-- `x` is periodic. -/
def Periodic (f : X → X) (x : X) : Prop := ∃ p, 0 < p ∧ iter f p x = x

/-- `y` lies on the forward orbit of `x`. -/
def Reaches (f : X → X) (x y : X) : Prop := ∃ j, iter f j x = y

namespace FinAction

variable (A : FinAction G X)

/-- `|G|`. -/
def card : Nat := A.elems.length

/-- `y ∈ G x`. -/
def Orb (x y : X) : Prop := ∃ g, y = A.act g x

/-- `f` respects orbits: `f (g x) ∈ G f(x)`. -/
def Respects (f : X → X) : Prop := ∀ g x, A.Orb (f x) (f (A.act g x))

/-- `f` is equivariant: `f (g x) = g f(x)`. -/
def Equivariant (f : X → X) : Prop := ∀ g x, f (A.act g x) = A.act g (f x)

/-- The orbit of `x` is free. -/
def Free (x : X) : Prop := ∀ g h, A.act g x = A.act h x → g = h

/-- Powers in `G`. -/
def gpow (g : G) : Nat → G
  | 0 => A.one
  | n + 1 => A.mul (gpow g n) g

theorem orb_refl (x : X) : A.Orb x x := ⟨A.one, (A.act_one x).symm⟩

theorem orb_symm {x y : X} : A.Orb x y → A.Orb y x := by
  rintro ⟨g, rfl⟩
  exact ⟨A.inv g, by rw [← A.act_mul, A.inv_mul, A.act_one]⟩

theorem orb_trans {x y z : X} : A.Orb x y → A.Orb y z → A.Orb x z := by
  rintro ⟨g, rfl⟩ ⟨h, rfl⟩
  exact ⟨A.mul h g, by rw [A.act_mul]⟩

theorem orb_act (g : G) (x : X) : A.Orb x (A.act g x) := ⟨g, rfl⟩

/-- The orbit relation as a setoid. -/
def setoid : Setoid X := ⟨A.Orb, ⟨A.orb_refl, A.orb_symm, A.orb_trans⟩⟩

/-- The orbit space `X/G`. -/
def Q : Type := Quotient A.setoid

/-- The orbit `[x]`. -/
def cls (x : X) : A.Q := Quotient.mk A.setoid x

theorem cls_eq_iff {x y : X} : A.cls x = A.cls y ↔ A.Orb x y :=
  ⟨fun h => Quotient.exact h, fun h => Quotient.sound h⟩

end FinAction

open FinAction

variable (A : FinAction G X)

/-- The cycle of `x` is trivial: `x` is periodic and its cycle meets the orbit `G e`. -/
def OnTrivial (e : X) (f : X → X) (x : X) : Prop :=
  Periodic f x ∧ ∃ j, A.Orb e (iter f j x)

/-- `f` is a Collatz map on `P`: every point of `P` has an iterate on a trivial cycle. -/
def CollatzOn (e : X) (P : X → Prop) (f : X → X) : Prop :=
  ∀ x, P x → ∃ n, OnTrivial A e f (iter f n x)

/-- Iteration on the orbit space. -/
def qiter (F : A.Q → A.Q) : Nat → A.Q → A.Q
  | 0, c => c
  | n + 1, c => qiter F n (F c)

/-- Collatz's sense on `X/G`: every class of `P` reaches `[e]`, and `[e]` is periodic. -/
def QCollatzOn (e : X) (P : X → Prop) (F : A.Q → A.Q) : Prop :=
  (∀ x, P x → ∃ n, qiter A F n (A.cls x) = A.cls e) ∧ ∃ p, 0 < p ∧ qiter A F p (A.cls e) = A.cls e

/-! ## Iteration -/

theorem iter_add (f : X → X) (m n : Nat) (x : X) :
    iter f (m + n) x = iter f n (iter f m x) := by
  induction m generalizing x with
  | zero => simp [iter]
  | succ m ih => rw [Nat.add_right_comm, iter, ih, iter]

theorem iter_succ' (f : X → X) (n : Nat) (x : X) : iter f (n + 1) x = f (iter f n x) := by
  rw [iter_add]; rfl

theorem iter_iter (f : X → X) (p i : Nat) (x : X) :
    iter (iter f p) i x = iter f (p * i) x := by
  induction i generalizing x with
  | zero => rw [Nat.mul_zero]; rfl
  | succ i ih =>
    show iter (iter f p) i (iter f p x) = _
    rw [ih, ← iter_add, Nat.mul_succ, Nat.add_comm]

theorem iter_period {f : X → X} {p : Nat} {x : X} (h : iter f p x = x) (m : Nat) :
    iter f (m * p) x = x := by
  induction m with
  | zero => rw [Nat.zero_mul]; rfl
  | succ m ih => rw [Nat.succ_mul, iter_add, ih, h]

theorem reaches_refl (f : X → X) (x : X) : Reaches f x x := ⟨0, rfl⟩

theorem reaches_trans {f : X → X} {x y z : X} :
    Reaches f x y → Reaches f y z → Reaches f x z
  | ⟨a, ha⟩, ⟨b, hb⟩ => ⟨a + b, by rw [iter_add, ha, hb]⟩

theorem reaches_back {f : X → X} {x y : X} (hx : Periodic f x) (h : Reaches f x y) :
    Reaches f y x := by
  obtain ⟨p, hp, hpx⟩ := hx
  obtain ⟨j, rfl⟩ := h
  refine ⟨j * p - j, ?_⟩
  have hle : j ≤ j * p := Nat.le_mul_of_pos_right j hp
  rw [← iter_add, show j + (j * p - j) = j * p by omega, iter_period hpx]

theorem periodic_of_reaches {f : X → X} {x y : X} (hx : Periodic f x)
    (h : Reaches f x y) : Periodic f y := by
  obtain ⟨p, hp, hpx⟩ := hx
  obtain ⟨j, rfl⟩ := h
  exact ⟨p, hp, by rw [← iter_add, Nat.add_comm, iter_add, hpx]⟩

theorem respects_rel {f : X → X} (h : A.Respects f) {a b : X} (hab : A.Orb a b) :
    A.Orb (f a) (f b) := by
  obtain ⟨g, rfl⟩ := hab; exact h g a

theorem respects_iter {f : X → X} (h : A.Respects f) (n : Nat) {a b : X} (hab : A.Orb a b) :
    A.Orb (iter f n a) (iter f n b) := by
  induction n generalizing a b with
  | zero => exact hab
  | succ n ih => exact ih (respects_rel A h hab)

theorem equivariant_iter {f : X → X} (hf : A.Equivariant f) (g : G) (n : Nat) (x : X) :
    iter f n (A.act g x) = A.act g (iter f n x) := by
  induction n generalizing x with
  | zero => rfl
  | succ n ih => show iter f n (f (A.act g x)) = _; rw [hf, ih]; rfl

theorem act_inv_act (g : G) (x : X) : A.act (A.inv g) (A.act g x) = x := by
  rw [← A.act_mul, A.inv_mul, A.act_one]

theorem mul_cancel {g a b : G} (h : A.mul g a = A.mul g b) : a = b := by
  have := congrArg (A.mul (A.inv g)) h
  rwa [← A.mul_assoc, ← A.mul_assoc, A.inv_mul, A.one_mul, A.one_mul] at this

theorem gpow_add (g : G) (m n : Nat) : A.gpow g (m + n) = A.mul (A.gpow g m) (A.gpow g n) := by
  induction n with
  | zero => show A.gpow g m = A.mul (A.gpow g m) A.one; rw [A.mul_one]
  | succ n ih =>
    show A.mul (A.gpow g (m + n)) g = A.mul (A.gpow g m) (A.mul (A.gpow g n) g)
    rw [ih, A.mul_assoc]

/-! ## Descent (Theorem 8.1) -/

/-- The induced map `[x] ↦ [f x]`. -/
def descend (f : X → X) (h : A.Respects f) : A.Q → A.Q :=
  Quotient.lift (fun x => A.cls (f x)) (fun a b hab => by
    obtain ⟨g, rfl⟩ := hab
    exact (A.cls_eq_iff).2 (h g a))

theorem descend_cls (f : X → X) (h : A.Respects f) (x : X) :
    descend A f h (A.cls x) = A.cls (f x) := rfl

theorem Equivariant.respects {f : X → X} (hf : A.Equivariant f) : A.Respects f := by
  intro g x; rw [hf g x]; exact A.orb_act g _

/-- **Descent.** A map `F` on `X/G` with `F [x] = [f x]` exists iff `f` respects orbits. -/
theorem descent_iff (f : X → X) :
    (∃ F : A.Q → A.Q, ∀ x, F (A.cls x) = A.cls (f x)) ↔ A.Respects f := by
  constructor
  · rintro ⟨F, hF⟩ g x
    apply (A.cls_eq_iff).1
    rw [← hF, ← hF, (A.cls_eq_iff).2 (A.orb_act g x)]
  · intro h; exact ⟨descend A f h, fun _ => rfl⟩

theorem descend_unique (f : X → X) (h : A.Respects f) (F : A.Q → A.Q)
    (hF : ∀ x, F (A.cls x) = A.cls (f x)) : F = descend A f h := by
  funext c
  induction c using Quotient.ind with
  | _ x => exact hF x

theorem descend_iter (f : X → X) (h : A.Respects f) (n : Nat) (x : X) :
    qiter A (descend A f h) n (A.cls x) = A.cls (iter f n x) := by
  induction n generalizing x with
  | zero => rfl
  | succ n ih => show qiter A _ n (descend A f h (A.cls x)) = _; rw [descend_cls, ih]; rfl

theorem qiter_add (F : A.Q → A.Q) (m n : Nat) (c : A.Q) :
    qiter A F (m + n) c = qiter A F n (qiter A F m c) := by
  induction m generalizing c with
  | zero => simp [qiter]
  | succ m ih => rw [Nat.add_right_comm, qiter, ih, qiter]

theorem qiter_mul (F : A.Q → A.Q) {p : Nat} {c : A.Q} (h : qiter A F p c = c) (m : Nat) :
    qiter A F (p * m) c = c := by
  induction m with
  | zero => rfl
  | succ m ih => rw [Nat.mul_succ, qiter_add, ih, h]

/-! ## Pigeonhole on an orbit -/

/-- Every point of `G x` is in the list `[g x | g ∈ G]`, so an orbit has at most `|G|` points. -/
theorem orb_mem {x y : X} (h : A.Orb x y) : y ∈ A.elems.map (fun g => A.act g x) := by
  obtain ⟨g, rfl⟩ := h
  exact List.mem_map.2 ⟨g, A.complete g, rfl⟩

/-- Pigeonhole: `L.length + 1` values in a list `L` contain a repetition. -/
theorem pigeon_list {α : Type} (s : Nat → α) (L : List α) (h : ∀ i, s i ∈ L) :
    ∃ i j, i < j ∧ j ≤ L.length ∧ s i = s j := by
  apply Classical.byContradiction
  intro hne
  have hnd : ((List.range (L.length + 1)).map s).Nodup := by
    unfold List.Nodup
    rw [List.pairwise_map]
    refine List.pairwise_lt_range.imp_of_mem ?_
    intro a b _ hb hab heq
    have := List.mem_range.1 hb
    exact hne ⟨a, b, hab, by omega, heq⟩
  have hsub : (List.range (L.length + 1)).map s ⊆ L := by
    intro y hy
    obtain ⟨i, _, rfl⟩ := List.mem_map.1 hy
    exact h i
  have := hnd.length_le_of_subset hsub
  simp only [List.length_map, List.length_range] at this
  omega

/-- If `g` maps the orbit of `x` into itself, two of `x, g x, …, g^{|G|} x` coincide. -/
theorem pigeon (g : X → X) (x : X) (hg : ∀ y, A.Orb x y → A.Orb x (g y)) :
    ∃ i j, i < j ∧ j ≤ A.card ∧ iter g i x = iter g j x := by
  have hA : ∀ i, A.Orb x (iter g i x) := by
    intro i
    induction i with
    | zero => exact A.orb_refl x
    | succ i ih => rw [iter_succ']; exact hg _ ih
  have := pigeon_list (fun i => iter g i x) (A.elems.map (fun g => A.act g x))
    (fun i => orb_mem A (hA i))
  simpa [card] using this

/-! ## Transfer (Theorem 8.2) -/

/-- **Transfer, orbit form.** For `f` respecting orbits and `P e`: `f` is a Collatz map on `P`
iff every point of `P` reaches `G e` and `f^p e ∈ G e` for some `p ≥ 1`. -/
theorem transfer_orbit (f : X → X) (h : A.Respects f) (e : X) (P : X → Prop) (hPe : P e) :
    CollatzOn A e P f ↔
      (∀ x, P x → ∃ n, A.Orb e (iter f n x)) ∧ ∃ p, 0 < p ∧ A.Orb e (iter f p e) := by
  constructor
  · intro hC
    refine ⟨fun x hx => ?_, ?_⟩
    · obtain ⟨n, _, j, hj⟩ := hC x hx
      exact ⟨n + j, by rw [iter_add]; exact hj⟩
    · obtain ⟨n, hper, j, hj⟩ := hC e hPe
      obtain ⟨q, hq, hqy⟩ := periodic_of_reaches hper ⟨j, rfl⟩
      refine ⟨q, hq, ?_⟩
      have h1 : A.Orb (iter f q e) (iter f q (iter f j (iter f n e))) := respects_iter A h q hj
      rw [hqy] at h1
      exact A.orb_trans hj (A.orb_symm h1)
  · rintro ⟨hreach, p, hp, hper⟩ x hx
    have hU : ∀ v, A.Orb e v → A.Orb e (iter f p v) := fun v hv =>
      A.orb_trans hper (respects_iter A h p hv)
    obtain ⟨n, hn⟩ := hreach x hx
    obtain ⟨i, j, hij, _, he⟩ := pigeon A (iter f p) (iter f n x)
      (fun y hy => A.orb_trans (A.orb_symm hn) (hU y (A.orb_trans hn hy)))
    have hstay : ∀ m v, A.Orb e v → A.Orb e (iter (iter f p) m v) := by
      intro m
      induction m with
      | zero => intro v hv; exact hv
      | succ m ih => intro v hv; exact ih _ (hU v hv)
    have e1 : iter f (n + p * i) x = iter (iter f p) i (iter f n x) := by
      rw [iter_add, iter_iter]
    refine ⟨n + p * i, ⟨p * (j - i), Nat.mul_pos hp (by omega), ?_⟩, 0, ?_⟩
    · rw [e1, ← iter_iter, ← iter_add, show i + (j - i) = j by omega, ← he]
    · show A.Orb e (iter f (n + p * i) x)
      rw [e1]; exact hstay i _ hn

/-- **Transfer.** `f` is a Collatz map on `P` iff the induced map is one on `P/G` in
Collatz's sense. -/
theorem transfer (f : X → X) (h : A.Respects f) (e : X) (P : X → Prop) (hPe : P e) :
    CollatzOn A e P f ↔ QCollatzOn A e P (descend A f h) := by
  rw [transfer_orbit A f h e P hPe]
  unfold QCollatzOn
  simp only [descend_iter, A.cls_eq_iff]
  constructor
  · rintro ⟨h1, p, hp, h2⟩
    exact ⟨fun x hx => (h1 x hx).imp fun _ => A.orb_symm, p, hp, A.orb_symm h2⟩
  · rintro ⟨h1, p, hp, h2⟩
    exact ⟨fun x hx => (h1 x hx).imp fun _ => A.orb_symm, p, hp, A.orb_symm h2⟩

/-! ## The trivial-cycle bound and its divisor form (Theorems 7.1 and 7.2) -/

/-- **Trivial-cycle bound.** For any `f`, a list of points on trivial cycles, no two on the
same cycle, has at most `|G|` elements. -/
theorem trivial_cycles_le (e : X) (f : X → X) (L : List X) (hL : ∀ x ∈ L, OnTrivial A e f x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches f x y)) : L.length ≤ A.card := by
  classical
  have hsel : ∀ x ∈ L, ∃ g, Reaches f x (A.act g e) := by
    intro x hx
    obtain ⟨_, j, g, hg⟩ := hL x hx
    exact ⟨g, j, hg⟩
  let sel : X → G := fun x => if h : ∃ g, Reaches f x (A.act g e) then Classical.choose h else A.one
  have hs : ∀ x ∈ L, Reaches f x (A.act (sel x) e) := by
    intro x hx
    have h := hsel x hx
    simp only [sel, h, ↓reduceDIte]
    exact Classical.choose_spec h
  have hnd : (L.map sel).Nodup := by
    unfold List.Nodup
    rw [List.pairwise_map]
    refine hdist.imp_of_mem ?_
    intro a b ha hb hab heq
    apply hab
    refine reaches_trans (hs a ha) ?_
    rw [heq]; exact reaches_back (hL b hb).1 (hs b hb)
  have hsub : L.map sel ⊆ A.elems := fun g _ => A.complete g
  have := hnd.length_le_of_subset hsub
  simpa [card] using this

/-- Every trivial cycle is the cycle of some `g e`. -/
theorem trivial_cycle_eq {e : X} {f : X → X} {x : X}
    (hx : OnTrivial A e f x) : ∃ g, Reaches f x (A.act g e) ∧ Reaches f (A.act g e) x := by
  obtain ⟨hper, j, g, hg⟩ := hx
  exact ⟨g, ⟨j, hg⟩, reaches_back hper ⟨j, hg⟩⟩

theorem periodic_act {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e) (g : G) :
    Periodic f (A.act g e) := by
  obtain ⟨p, hp, h⟩ := he
  exact ⟨p, hp, by rw [equivariant_iter A hf, h]⟩

/-- Existence of one representative `l ∈ L` for each cycle through `G e`. -/
theorem exists_cycle_reps {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e) :
    ∃ L : List G, L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)) ∧
      ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e) := by
  classical
  have key : ∀ M : List G, ∃ L : List G,
      L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)) ∧
      ∀ g ∈ M, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e) := by
    intro M
    induction M with
    | nil => exact ⟨[], List.Pairwise.nil, fun g hg => absurd hg (List.not_mem_nil)⟩
    | cons g M ih =>
      obtain ⟨L, hL, hcov⟩ := ih
      by_cases hg : ∃ l ∈ L, Reaches f (A.act l e) (A.act g e)
      · refine ⟨L, hL, fun g' hg' => ?_⟩
        rcases List.mem_cons.1 hg' with rfl | hm
        · exact hg
        · exact hcov g' hm
      · refine ⟨g :: L, List.Pairwise.cons ?_ hL, fun g' hg' => ?_⟩
        · intro l hl hr
          exact hg ⟨l, hl, reaches_back (periodic_act A hf he g) hr⟩
        · rcases List.mem_cons.1 hg' with rfl | hm
          · exact ⟨g', List.mem_cons_self, reaches_refl f _⟩
          · obtain ⟨l, hl, hr⟩ := hcov g' hm
            exact ⟨l, List.mem_cons_of_mem g hl, hr⟩
  obtain ⟨L, hL, hcov⟩ := key A.elems
  exact ⟨L, hL, fun g => hcov g (A.complete g)⟩

/-- Existence of the stabilizer `H = {g : g e ∈ C_e}` as a duplicate-free list. -/
theorem exists_stab_list (e : X) (f : X → X) :
    ∃ H : List G, H.Nodup ∧ ∀ g, g ∈ H ↔ Reaches f e (A.act g e) := by
  classical
  refine ⟨A.elems.filter (fun g => decide (Reaches f e (A.act g e))), ?_, ?_⟩
  · exact A.nodup.sublist List.filter_sublist
  · intro g
    rw [List.mem_filter, decide_eq_true_iff]
    exact ⟨fun h => h.2, fun h => ⟨A.complete g, h⟩⟩

theorem flatMap_length_const {α β : Type} (L : List α) (F : α → List β) (c : Nat)
    (h : ∀ a ∈ L, (F a).length = c) : (L.flatMap F).length = L.length * c := by
  induction L with
  | nil => simp
  | cons a L ih =>
    rw [List.flatMap_cons, List.length_append, h a List.mem_cons_self,
      ih (fun b hb => h b (List.mem_cons_of_mem a hb)), List.length_cons, Nat.succ_mul,
      Nat.add_comm]

/-- **Divisor form.** For equivariant `f` with `e` periodic, if `L` lists one group element
for each trivial cycle (through `l e`) and `H` lists the stabilizer `{g : g e ∈ C_e}`, then
`|L| · |H| = |G|`. -/
theorem trivial_cycle_count {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e)
    (L H : List G) (hL : L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)))
    (hcover : ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e))
    (hH : H.Nodup) (hHmem : ∀ g, g ∈ H ↔ Reaches f e (A.act g e)) :
    L.length * H.length = A.card := by
  let F : G → List G := fun l => H.map (A.mul l)
  have key1 : ∀ l g, Reaches f (A.act l e) (A.act g e) → A.mul (A.inv l) g ∈ H := by
    rintro l g ⟨j, hj⟩
    refine (hHmem _).2 ⟨j, ?_⟩
    rw [equivariant_iter A hf] at hj
    rw [A.act_mul, ← hj, act_inv_act]
  have key2 : ∀ l h, h ∈ H → Reaches f (A.act l e) (A.act (A.mul l h) e) := by
    intro l h hh
    obtain ⟨j, hj⟩ := (hHmem h).1 hh
    exact ⟨j, by rw [equivariant_iter A hf, hj, A.act_mul]⟩
  have hmem : ∀ g, g ∈ L.flatMap F := by
    intro g
    obtain ⟨l, hl, hr⟩ := hcover g
    refine List.mem_flatMap.2 ⟨l, hl, List.mem_map.2 ⟨_, key1 l g hr, ?_⟩⟩
    rw [← A.mul_assoc, A.mul_inv, A.one_mul]
  have hnodupF : ∀ l, (F l).Nodup := by
    intro l
    unfold List.Nodup
    rw [List.pairwise_map]
    exact hH.imp fun hab heq => hab (mul_cancel A heq)
  have hnd : ∀ L' : List G, L'.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)) →
      (L'.flatMap F).Nodup := by
    intro L' hL'
    induction L' with
    | nil => exact List.Pairwise.nil
    | cons a L' ih =>
      rw [List.flatMap_cons, List.nodup_append]
      obtain ⟨ha, hrest⟩ := List.pairwise_cons.1 hL'
      refine ⟨hnodupF a, ih hrest, ?_⟩
      intro b1 hb1 b2 hb2 heq
      obtain ⟨h1, hh1, rfl⟩ := List.mem_map.1 hb1
      obtain ⟨a', ha', hb2'⟩ := List.mem_flatMap.1 hb2
      obtain ⟨h2, hh2, rfl⟩ := List.mem_map.1 hb2'
      apply ha a' ha'
      have r1 := key2 a h1 hh1
      have r2 := key2 a' h2 hh2
      rw [heq] at r1
      exact reaches_trans r1 (reaches_back (periodic_act A hf he a') r2)
  have hperm : (L.flatMap F).Perm A.elems :=
    (List.perm_ext_iff_of_nodup (hnd L hL) A.nodup).2
      (fun g => ⟨fun _ => A.complete g, fun _ => hmem g⟩)
  have := hperm.length_eq
  rw [flatMap_length_const L F H.length (fun _ _ => List.length_map ..)] at this
  exact this

/-- So the number of trivial cycles divides `|G|`. -/
theorem trivial_cycles_dvd {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e)
    (L : List G) (hL : L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)))
    (hcover : ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e)) : L.length ∣ A.card := by
  obtain ⟨H, hH, hHmem⟩ := exists_stab_list A e f
  exact ⟨H.length, (trivial_cycle_count A hf he L H hL hcover hH hHmem).symm⟩

/-! ## Unit cycles (Theorem 7.3) -/

/-- **Unit cycles.** For equivariant `f` with `f e = w e`: `f^n (g e) = g (w^n e)`. -/
theorem orbit_iter {f : X → X} (hf : A.Equivariant f) {e : X} {w : G} (hw : f e = A.act w e)
    (g : G) (n : Nat) : iter f n (A.act g e) = A.act g (A.act (A.gpow w n) e) := by
  induction n with
  | zero => show A.act g e = A.act g (A.act A.one e); rw [A.act_one]
  | succ n ih =>
    rw [iter_succ', ih, hf, hf, hw]
    show A.act g (A.act (A.gpow w n) (A.act w e)) = A.act g (A.act (A.mul (A.gpow w n) w) e)
    rw [A.act_mul]

/-- On a free orbit, `g e` returns after `n` steps iff `w^n = 1`. -/
theorem orbit_iter_period {f : X → X} (hf : A.Equivariant f) {e : X} (hfree : A.Free e) {w : G}
    (hw : f e = A.act w e) (g : G) (n : Nat) :
    iter f n (A.act g e) = A.act g e ↔ A.gpow w n = A.one := by
  rw [orbit_iter A hf hw, ← A.act_mul]
  constructor
  · intro h
    have := hfree _ _ h
    exact mul_cancel A (this.trans (A.mul_one g).symm)
  · intro h; rw [h, A.mul_one]

/-- Every element of a finite group has a power `w^n = 1` with `1 ≤ n ≤ |G|`. -/
theorem gpow_eq_one (w : G) : ∃ n, 0 < n ∧ n ≤ A.card ∧ A.gpow w n = A.one := by
  obtain ⟨i, j, hij, hj, he⟩ := pigeon_list (fun i => A.gpow w i) A.elems (fun i => A.complete _)
  refine ⟨j - i, by omega, by unfold card; omega, ?_⟩
  have : A.gpow w j = A.mul (A.gpow w i) (A.gpow w (j - i)) := by
    rw [← gpow_add, show i + (j - i) = j by omega]
  rw [← he] at this
  exact (mul_cancel A ((A.mul_one _).trans this)).symm

/-! ## Lifting cycles (Theorems 8.4 and 8.5) -/

/-- **Lifts.** If `[x]` returns after `p` steps, `f^p x = g x`, `f^p` maps `G x` into itself,
and some point of `G x` is periodic with period `p r`, `1 ≤ r ≤ |G|`. -/
theorem lift_cycle (f : X → X) (h : A.Respects f) {x : X} {p : Nat}
    (hx : qiter A (descend A f h) p (A.cls x) = A.cls x) :
    (∃ g, iter f p x = A.act g x) ∧ (∀ y, A.Orb x y → A.Orb x (iter f p y)) ∧
      ∃ y, A.Orb x y ∧ ∃ r, 0 < r ∧ r ≤ A.card ∧ iter f (p * r) y = y := by
  have hxx : A.Orb x (iter f p x) := by
    rw [descend_iter] at hx; exact A.orb_symm ((A.cls_eq_iff).1 hx)
  have hmap : ∀ y, A.Orb x y → A.Orb x (iter f p y) := fun y hy =>
    A.orb_trans hxx (respects_iter A h p hy)
  refine ⟨hxx, hmap, ?_⟩
  obtain ⟨i, j, hij, hj, he⟩ := pigeon A (iter f p) x hmap
  have hA : ∀ m, A.Orb x (iter (iter f p) m x) := by
    intro m
    induction m with
    | zero => exact A.orb_refl x
    | succ m ih => rw [iter_succ']; exact hmap _ ih
  refine ⟨iter (iter f p) i x, hA i, j - i, by omega, by omega, ?_⟩
  rw [← iter_iter, ← iter_add, show i + (j - i) = j by omega, ← he]

/-- A period of a class is a multiple of its least period. -/
theorem qperiod_dvd (F : A.Q → A.Q) {c : A.Q} {p q : Nat} (hp : 0 < p)
    (hpc : qiter A F p c = c) (hmin : ∀ t, 0 < t → t < p → qiter A F t c ≠ c)
    (hq : qiter A F q c = c) : p ∣ q := by
  have e : q = p * (q / p) + q % p := (Nat.div_add_mod q p).symm
  have h1 : qiter A F (q % p) c = c := by
    rw [e, qiter_add, qiter_mul A F hpc] at hq; exact hq
  apply Nat.dvd_of_mod_eq_zero
  apply Classical.byContradiction
  intro hne
  exact hmin _ (by omega) (Nat.mod_lt q hp) h1

/-- Every `f`-period of a point over a cycle of least period `p` is a multiple of `p`. -/
theorem lift_period_dvd (f : X → X) (h : A.Respects f) {x y : X} {p q : Nat}
    (hp : 0 < p) (hpc : qiter A (descend A f h) p (A.cls x) = A.cls x)
    (hmin : ∀ t, 0 < t → t < p → qiter A (descend A f h) t (A.cls x) ≠ A.cls x)
    (hy : A.Orb x y) (hq : iter f q y = y) : p ∣ q := by
  apply qperiod_dvd A _ hp hpc hmin
  rw [(A.cls_eq_iff).2 hy, descend_iter, hq]

/-- **Equivariant lifts.** `f^p x = ε x` gives `f^{pm} (h x) = h (ε^m x)`. -/
theorem equivariant_lift {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (he : iter f p x = A.act ε x) (h : G) (m : Nat) :
    iter f (p * m) (A.act h x) = A.act h (A.act (A.gpow ε m) x) := by
  induction m with
  | zero => show A.act h x = A.act h (A.act A.one x); rw [A.act_one]
  | succ m ih =>
    rw [Nat.mul_succ, iter_add, ih, equivariant_iter A hf, equivariant_iter A hf, he]
    show A.act h (A.act (A.gpow ε m) (A.act ε x)) = A.act h (A.act (A.mul (A.gpow ε m) ε) x)
    rw [A.act_mul]

/-- Every point of the orbit is periodic. -/
theorem equivariant_lift_periodic {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (hp : 0 < p) (he : iter f p x = A.act ε x) (h : G) : Periodic f (A.act h x) := by
  obtain ⟨n, hn, _, h1⟩ := gpow_eq_one A ε
  refine ⟨p * n, Nat.mul_pos hp hn, ?_⟩
  rw [equivariant_lift A hf he h n, h1, A.act_one]

/-- **Stabilizer.** With `p` the least period of `[x]` and `f^p x = ε x`: `h x` lies on the
cycle of `x` iff `h x = ε^m x` for some `m`. -/
theorem equivariant_stab {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (hp : 0 < p) (he : iter f p x = A.act ε x)
    (hmin : ∀ t, 0 < t → t < p →
      qiter A (descend A f (Equivariant.respects A hf)) t (A.cls x) ≠ A.cls x)
    (h : G) : Reaches f x (A.act h x) ↔ ∃ m, A.act h x = A.act (A.gpow ε m) x := by
  have h1 : ∀ m, iter f (p * m) x = A.act (A.gpow ε m) x := by
    intro m
    have := equivariant_lift A hf he A.one m
    rwa [A.act_one, A.act_one] at this
  have hpc : qiter A (descend A f (Equivariant.respects A hf)) p (A.cls x) = A.cls x := by
    rw [descend_iter, he]; exact (A.cls_eq_iff).2 (A.orb_symm (A.orb_act ε x))
  constructor
  · rintro ⟨i, hi⟩
    have hq : qiter A (descend A f (Equivariant.respects A hf)) i (A.cls x) = A.cls x := by
      rw [descend_iter, hi]; exact (A.cls_eq_iff).2 (A.orb_symm (A.orb_act h x))
    obtain ⟨m, rfl⟩ := qperiod_dvd A _ hp hpc hmin hq
    exact ⟨m, by rw [← hi, h1 m]⟩
  · rintro ⟨m, hm⟩
    exact ⟨p * m, by rw [h1 m, hm]⟩

/-- On a free orbit the stabilizer of the cycle of `x` is `⟨ε⟩`. -/
theorem equivariant_stab_free {f : X → X} (hf : A.Equivariant f) {x : X} (hfree : A.Free x)
    {ε : G} {p : Nat} (hp : 0 < p) (he : iter f p x = A.act ε x)
    (hmin : ∀ t, 0 < t → t < p →
      qiter A (descend A f (Equivariant.respects A hf)) t (A.cls x) ≠ A.cls x)
    (h : G) : Reaches f x (A.act h x) ↔ ∃ m, h = A.gpow ε m := by
  rw [equivariant_stab A hf hp he hmin h]
  exact ⟨fun ⟨m, hm⟩ => ⟨m, hfree _ _ hm⟩, fun ⟨m, hm⟩ => ⟨m, by rw [hm]⟩⟩

/-! ## Rotation and conjugation (Theorem 8.6) -/

/-- Two maps agreeing up to the group: if `f` respects orbits and `f' x ∈ G f(x)`, then `f'`
respects orbits and induces the same map. -/
theorem agree_respects {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x)) :
    A.Respects f' := fun g x =>
  A.orb_trans (A.orb_symm (hff x)) (A.orb_trans (h g x) (hff (A.act g x)))

theorem agree_descend {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x)) :
    descend A f' (agree_respects A h hff) = descend A f h := by
  funext c
  induction c using Quotient.ind with
  | _ x => exact (A.cls_eq_iff).2 (A.orb_symm (hff x))

/-- **Rotation.** Such `f` and `f'` are Collatz maps together. -/
theorem agree_collatz {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x))
    (e : X) (P : X → Prop) (hPe : P e) : CollatzOn A e P f' ↔ CollatzOn A e P f := by
  rw [transfer A f' (agree_respects A h hff) e P hPe, agree_descend A h hff,
    ← transfer A f h e P hPe]

/-- **The transfer lemma R4.** If `g` is equivariant and `g x ∈ G f(x)`, then `f` respects
orbits, and `f` and `g` are Collatz maps together. -/
theorem equivariant_agree_collatz {f g : X → X} (hg : A.Equivariant g)
    (hfg : ∀ x, A.Orb (f x) (g x)) (e : X) (P : X → Prop) (hPe : P e) :
    A.Respects f ∧ (CollatzOn A e P g ↔ CollatzOn A e P f) := by
  have hf : A.Respects f := fun a x =>
    A.orb_trans (hfg x) (A.orb_trans (Equivariant.respects A hg a x) (A.orb_symm (hfg _)))
  exact ⟨hf, agree_collatz A hf hfg e P hPe⟩

/-- **Conjugation (mirror).** Let `τ` be a bijection of `X` mapping orbits to orbits, with
`τ e ∈ G e` and `τ (P) = P`. If `f` respects orbits and `f' x ∈ G τ (f (τ⁻¹ x))`, then `f'`
respects orbits and `f'`, `f` are Collatz maps together. -/
theorem conj_collatz {f f' : X → X} (τ τi : X → X) (h1 : ∀ x, τ (τi x) = x)
    (h2 : ∀ x, τi (τ x) = x) (hτ : ∀ x y, A.Orb x y → A.Orb (τ x) (τ y))
    (hτi : ∀ x y, A.Orb x y → A.Orb (τi x) (τi y)) {e : X} (he : A.Orb e (τ e))
    {P : X → Prop} (hP : ∀ x, P x → P (τ x)) (hPi : ∀ x, P x → P (τi x)) (hPe : P e)
    (h : A.Respects f) (hff : ∀ x, A.Orb (τ (f (τi x))) (f' x)) :
    A.Respects f' ∧ (CollatzOn A e P f' ↔ CollatzOn A e P f) := by
  have hf' : A.Respects f' := by
    intro g x
    have a1 : A.Orb (τi x) (τi (A.act g x)) := hτi _ _ (A.orb_act g x)
    have a2 := hτ _ _ (respects_rel A h a1)
    exact A.orb_trans (A.orb_symm (hff x)) (A.orb_trans a2 (hff _))
  refine ⟨hf', ?_⟩
  have hit : ∀ n x, A.Orb (τ (iter f n (τi x))) (iter f' n x) := by
    intro n
    induction n with
    | zero => intro x; show A.Orb (τ (τi x)) x; rw [h1]; exact A.orb_refl x
    | succ n ih =>
      intro x
      have b1 : A.Orb (τi (f' x)) (f (τi x)) := by
        have := hτi _ _ (A.orb_symm (hff x)); rwa [h2] at this
      have b2 := hτ _ _ (respects_iter A h n b1)
      exact A.orb_trans (A.orb_symm b2) (ih (f' x))
  have hEτ : ∀ y, A.Orb e (τ y) ↔ A.Orb e y := by
    intro y
    constructor
    · intro hy
      have := hτi _ _ (A.orb_trans (A.orb_symm he) hy)
      rwa [h2, h2] at this
    · intro hy; exact A.orb_trans he (hτ _ _ hy)
  have hstep : ∀ n x, A.Orb e (iter f' n x) ↔ A.Orb e (iter f n (τi x)) := by
    intro n x
    exact ⟨fun hy => (hEτ _).1 (A.orb_trans hy (A.orb_symm (hit n x))),
      fun hy => A.orb_trans ((hEτ _).2 hy) (hit n x)⟩
  have hie : A.Orb (τi e) e := by have := hτi _ _ he; rwa [h2] at this
  rw [transfer_orbit A f' hf' e P hPe, transfer_orbit A f h e P hPe]
  constructor
  · rintro ⟨r1, p, hp, r2⟩
    refine ⟨fun x hx => ?_, p, hp, ?_⟩
    · obtain ⟨n, hn⟩ := r1 (τ x) (hP x hx)
      rw [hstep, h2] at hn; exact ⟨n, hn⟩
    · rw [hstep] at r2; exact A.orb_trans r2 (respects_iter A h p hie)
  · rintro ⟨r1, p, hp, r2⟩
    refine ⟨fun x hx => ?_, p, hp, ?_⟩
    · obtain ⟨n, hn⟩ := r1 (τi x) (hPi x hx)
      exact ⟨n, (hstep n x).2 hn⟩
    · rw [hstep]; exact A.orb_trans r2 (A.orb_symm (respects_iter A h p hie))

end Abstract
