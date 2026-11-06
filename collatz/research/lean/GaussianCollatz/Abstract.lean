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
  sorry

theorem descend_unique (f : X → X) (h : A.Respects f) (F : A.Q → A.Q)
    (hF : ∀ x, F (A.cls x) = A.cls (f x)) : F = descend A f h := by
  sorry

theorem descend_iter (f : X → X) (h : A.Respects f) (n : Nat) (x : X) :
    qiter A (descend A f h) n (A.cls x) = A.cls (iter f n x) := by
  sorry

/-! ## Pigeonhole on an orbit -/

/-- Every point of `G x` is in the list `[g x | g ∈ G]`, so an orbit has at most `|G|` points. -/
theorem orb_mem {x y : X} (h : A.Orb x y) : y ∈ A.elems.map (fun g => A.act g x) := by
  sorry

/-- Pigeonhole: `L.length + 1` values in a list `L` contain a repetition. -/
theorem pigeon_list {α : Type} (s : Nat → α) (L : List α) (h : ∀ i, s i ∈ L) :
    ∃ i j, i < j ∧ j ≤ L.length ∧ s i = s j := by
  sorry

/-- If `g` maps the orbit of `x` into itself, two of `x, g x, …, g^{|G|} x` coincide. -/
theorem pigeon (g : X → X) (x : X) (hg : ∀ y, A.Orb x y → A.Orb x (g y)) :
    ∃ i j, i < j ∧ j ≤ A.card ∧ iter g i x = iter g j x := by
  sorry

/-! ## Transfer (Theorem 8.2) -/

/-- **Transfer, orbit form.** For `f` respecting orbits and `P e`: `f` is a Collatz map on `P`
iff every point of `P` reaches `G e` and `f^p e ∈ G e` for some `p ≥ 1`. -/
theorem transfer_orbit (f : X → X) (h : A.Respects f) (e : X) (P : X → Prop) (hPe : P e) :
    CollatzOn A e P f ↔
      (∀ x, P x → ∃ n, A.Orb e (iter f n x)) ∧ ∃ p, 0 < p ∧ A.Orb e (iter f p e) := by
  sorry

/-- **Transfer.** `f` is a Collatz map on `P` iff the induced map is one on `P/G` in
Collatz's sense. -/
theorem transfer (f : X → X) (h : A.Respects f) (e : X) (P : X → Prop) (hPe : P e) :
    CollatzOn A e P f ↔ QCollatzOn A e P (descend A f h) := by
  sorry

/-! ## The trivial-cycle bound and its divisor form (Theorems 7.1 and 7.2) -/

/-- **Trivial-cycle bound.** For any `f`, a list of points on trivial cycles, no two on the
same cycle, has at most `|G|` elements. -/
theorem trivial_cycles_le (e : X) (f : X → X) (L : List X) (hL : ∀ x ∈ L, OnTrivial A e f x)
    (hdist : L.Pairwise (fun x y => ¬ Reaches f x y)) : L.length ≤ A.card := by
  sorry

/-- For equivariant `f` with `e` periodic, every trivial cycle is the cycle of some `l e`. -/
theorem trivial_cycle_eq {e : X} {f : X → X} (hf : A.Equivariant f) {x : X}
    (hx : OnTrivial A e f x) : ∃ g, Reaches f x (A.act g e) ∧ Reaches f (A.act g e) x := by
  sorry

/-- Existence of one representative `l ∈ L` for each cycle through `G e`. -/
theorem exists_cycle_reps {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e) :
    ∃ L : List G, L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)) ∧
      ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e) := by
  sorry

/-- Existence of the stabilizer `H = {g : g e ∈ C_e}` as a duplicate-free list. -/
theorem exists_stab_list (e : X) (f : X → X) :
    ∃ H : List G, H.Nodup ∧ ∀ g, g ∈ H ↔ Reaches f e (A.act g e) := by
  sorry

/-- **Divisor form.** For equivariant `f` with `e` periodic, if `L` lists one group element
for each trivial cycle (through `l e`) and `H` lists the stabilizer `{g : g e ∈ C_e}`, then
`|L| · |H| = |G|`. -/
theorem trivial_cycle_count {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e)
    (L H : List G) (hL : L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)))
    (hcover : ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e))
    (hH : H.Nodup) (hHmem : ∀ g, g ∈ H ↔ Reaches f e (A.act g e)) :
    L.length * H.length = A.card := by
  sorry

/-- So the number of trivial cycles divides `|G|`. -/
theorem trivial_cycles_dvd {e : X} {f : X → X} (hf : A.Equivariant f) (he : Periodic f e)
    (L : List G) (hL : L.Pairwise (fun a b => ¬ Reaches f (A.act a e) (A.act b e)))
    (hcover : ∀ g, ∃ l ∈ L, Reaches f (A.act l e) (A.act g e)) : L.length ∣ A.card := by
  sorry

/-! ## Unit cycles (Theorem 7.3) -/

/-- **Unit cycles.** For equivariant `f` with `f e = w e`: `f^n (g e) = g (w^n e)`. -/
theorem orbit_iter {f : X → X} (hf : A.Equivariant f) {e : X} {w : G} (hw : f e = A.act w e)
    (g : G) (n : Nat) : iter f n (A.act g e) = A.act g (A.act (A.gpow w n) e) := by
  sorry

/-- On a free orbit, `g e` returns after `n` steps iff `w^n = 1`. -/
theorem orbit_iter_period {f : X → X} (hf : A.Equivariant f) {e : X} (hfree : A.Free e) {w : G}
    (hw : f e = A.act w e) (g : G) (n : Nat) : iter f n (A.act g e) = A.act g e ↔ A.gpow w n = A.one := by
  sorry

/-- Every element of a finite group has a power `w^n = 1` with `1 ≤ n ≤ |G|`. -/
theorem gpow_eq_one (w : G) : ∃ n, 0 < n ∧ n ≤ A.card ∧ A.gpow w n = A.one := by
  sorry

/-! ## Lifting cycles (Theorems 8.4 and 8.5) -/

/-- **Lifts.** If `[x]` returns after `p` steps, `f^p x = g x`, `f^p` maps `G x` into itself,
and some point of `G x` is periodic with period `p r`, `1 ≤ r ≤ |G|`. -/
theorem lift_cycle (f : X → X) (h : A.Respects f) {x : X} {p : Nat}
    (hx : qiter A (descend A f h) p (A.cls x) = A.cls x) :
    (∃ g, iter f p x = A.act g x) ∧ (∀ y, A.Orb x y → A.Orb x (iter f p y)) ∧
      ∃ y, A.Orb x y ∧ ∃ r, 0 < r ∧ r ≤ A.card ∧ iter f (p * r) y = y := by
  sorry

/-- A period of a class is a multiple of its least period. -/
theorem qperiod_dvd (F : A.Q → A.Q) {c : A.Q} {p q : Nat} (hp : 0 < p)
    (hpc : qiter A F p c = c) (hmin : ∀ t, 0 < t → t < p → qiter A F t c ≠ c)
    (hq : qiter A F q c = c) : p ∣ q := by
  sorry

/-- Every `f`-period of a point over a cycle of least period `p` is a multiple of `p`. -/
theorem lift_period_dvd (f : X → X) (h : A.Respects f) {x y : X} {p q : Nat}
    (hp : 0 < p) (hpc : qiter A (descend A f h) p (A.cls x) = A.cls x)
    (hmin : ∀ t, 0 < t → t < p → qiter A (descend A f h) t (A.cls x) ≠ A.cls x)
    (hy : A.Orb x y) (hq : iter f q y = y) : p ∣ q := by
  sorry

/-- **Equivariant lifts.** `f^p x = ε x` gives `f^{pm} (h x) = h (ε^m x)`. -/
theorem equivariant_lift {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (he : iter f p x = A.act ε x) (h : G) (m : Nat) :
    iter f (p * m) (A.act h x) = A.act h (A.act (A.gpow ε m) x) := by
  sorry

/-- Every point of the orbit is periodic. -/
theorem equivariant_lift_periodic {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (hp : 0 < p) (he : iter f p x = A.act ε x) (h : G) : Periodic f (A.act h x) := by
  sorry

/-- **Stabilizer.** With `p` the least period of `[x]` and `f^p x = ε x`: `h x` lies on the
cycle of `x` iff `h x = ε^m x` for some `m`. -/
theorem equivariant_stab {f : X → X} (hf : A.Equivariant f) {x : X} {ε : G} {p : Nat}
    (hp : 0 < p) (he : iter f p x = A.act ε x)
    (hmin : ∀ t, 0 < t → t < p →
      qiter A (descend A f (Equivariant.respects A hf)) t (A.cls x) ≠ A.cls x)
    (h : G) : Reaches f x (A.act h x) ↔ ∃ m, A.act h x = A.act (A.gpow ε m) x := by
  sorry

/-- On a free orbit the stabilizer of the cycle of `x` is `⟨ε⟩`. -/
theorem equivariant_stab_free {f : X → X} (hf : A.Equivariant f) {x : X} (hfree : A.Free x)
    {ε : G} {p : Nat} (hp : 0 < p) (he : iter f p x = A.act ε x)
    (hmin : ∀ t, 0 < t → t < p →
      qiter A (descend A f (Equivariant.respects A hf)) t (A.cls x) ≠ A.cls x)
    (h : G) : Reaches f x (A.act h x) ↔ ∃ m, h = A.gpow ε m := by
  sorry

/-! ## Rotation and conjugation (Theorem 8.6) -/

/-- Two maps agreeing up to the group: if `f` respects orbits and `f' x ∈ G f(x)`, then `f'`
respects orbits and induces the same map. -/
theorem agree_respects {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x)) :
    A.Respects f' := by
  sorry

theorem agree_descend {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x)) :
    descend A f' (agree_respects A h hff) = descend A f h := by
  sorry

/-- **Rotation.** Such `f` and `f'` are Collatz maps together. -/
theorem agree_collatz {f f' : X → X} (h : A.Respects f) (hff : ∀ x, A.Orb (f x) (f' x))
    (e : X) (P : X → Prop) (hPe : P e) : CollatzOn A e P f' ↔ CollatzOn A e P f := by
  sorry

/-- **The transfer lemma R4.** If `g` is equivariant and `g x ∈ G f(x)`, then `f` respects
orbits, and `f` and `g` are Collatz maps together. -/
theorem equivariant_agree_collatz {f g : X → X} (hg : A.Equivariant g)
    (hfg : ∀ x, A.Orb (f x) (g x)) (e : X) (P : X → Prop) (hPe : P e) :
    A.Respects f ∧ (CollatzOn A e P g ↔ CollatzOn A e P f) := by
  sorry

/-- **Conjugation (mirror).** Let `τ` be a bijection of `X` mapping orbits to orbits, with
`τ e ∈ G e` and `τ (P) = P`. If `f` respects orbits and `f' x ∈ G τ (f (τ⁻¹ x))`, then `f'`
respects orbits and `f'`, `f` are Collatz maps together. -/
theorem conj_collatz {f f' : X → X} (τ τi : X → X) (h1 : ∀ x, τ (τi x) = x)
    (h2 : ∀ x, τi (τ x) = x) (hτ : ∀ x y, A.Orb x y → A.Orb (τ x) (τ y))
    (hτi : ∀ x y, A.Orb x y → A.Orb (τi x) (τi y)) {e : X} (he : A.Orb e (τ e))
    {P : X → Prop} (hP : ∀ x, P x → P (τ x)) (hPi : ∀ x, P x → P (τi x)) (hPe : P e)
    (h : A.Respects f) (hff : ∀ x, A.Orb (τ (f (τi x))) (f' x)) :
    A.Respects f' ∧ (CollatzOn A e P f' ↔ CollatzOn A e P f) := by
  sorry

end Abstract
