import Header.Clock
/-!
# Design B, integers: one magnitude per direction

An integer is a pair `(P, N)` of magnitudes, value `P - N`.
-/

set_option linter.unusedVariables false

namespace Header
namespace B

/-- A Design B integer `(P, N)`. -/
structure Num where
  P : Nat
  N : Nat
deriving DecidableEq, Repr

/-- The value `P - N`. -/
def val (x : Num) : Int := (x.P : Int) - x.N

/-- Addition, part by part. -/
def add (x y : Num) : Num := ⟨x.P + y.P, x.N + y.N⟩
/-- Subtraction, the crossed parts: `(P1 + N2, N1 + P2)`. -/
def sub (x y : Num) : Num := ⟨x.P + y.N, x.N + y.P⟩
/-- Negation swaps the parts. -/
def neg (x : Num) : Num := ⟨x.N, x.P⟩
/-- Multiplication: `(P1 P2 + N1 N2, P1 N2 + N1 P2)`. -/
def mul (x y : Num) : Num := ⟨x.P * y.P + x.N * y.N, x.P * y.N + x.N * y.P⟩
/-- Comparison: `P1 + N2` against `P2 + N1`, magnitude additions only. -/
def cmp (x y : Num) : Ordering := ord ((x.P + y.N : Nat) : Int) ((y.P + x.N : Nat) : Int)
/-- Collapse, as the notes state it. -/
def collapse (x : Num) : Num := if x.N ≤ x.P then ⟨x.P - x.N, 0⟩ else ⟨0, x.N - x.P⟩
/-- Collapse as spec §3 states it, on a `W`-value word: `|P - N|` by the absolute-difference routine, to `P` when the
borrow is 0, else to `N`; the other part 0. -/
def collapseClock (W : Nat) (x : Num) : Num :=
  if borrow x.P x.N then ⟨0, absDiffClock W x.P x.N⟩ else ⟨absDiffClock W x.P x.N, 0⟩
/-- Canonical (collapsed): one part zero. -/
def Canonical (x : Num) : Prop := x.P = 0 ∨ x.N = 0

theorem val_add (x y : Num) : val (add x y) = val x + val y := by simp [val, add]; omega
theorem val_sub (x y : Num) : val (sub x y) = val x - val y := by simp [val, sub]; omega
theorem val_neg (x : Num) : val (neg x) = -val x := by simp [val, neg]; omega
theorem val_mul (x y : Num) : val (mul x y) = val x * val y := by simp [val, mul]; grind

/-- Two pairs have equal value iff their crossed sums agree. -/
theorem val_eq_iff (x y : Num) : val x = val y ↔ x.P + y.N = y.P + x.N := by simp [val]; omega

/-- Comparison by crossed sums agrees with the order of the values. -/
theorem cmp_correct (x y : Num) : cmp x y = ord (val x) (val y) := by
  have h1 : ((x.P + y.N : Nat) : Int) < ((y.P + x.N : Nat) : Int) ↔ (x.P : Int) - x.N < (y.P : Int) - y.N := by
    omega
  have h2 : ((x.P + y.N : Nat) : Int) = ((y.P + x.N : Nat) : Int) ↔ (x.P : Int) - x.N = (y.P : Int) - y.N := by
    omega
  simp only [cmp, ord, val, h1, h2]
  congr

theorem val_collapse (x : Num) : val (collapse x) = val x := by
  unfold collapse val; split <;> simp <;> omega

theorem collapse_canonical (x : Num) : Canonical (collapse x) := by
  unfold collapse Canonical; split <;> simp

/-- A canonical pair is unique for its value: so the collapse is *the* canonical pair, zero's is `(0, 0)`. -/
theorem canonical_unique {x y : Num} (hx : Canonical x) (hy : Canonical y) (h : val x = val y) : x = y := by
  obtain ⟨p1, n1⟩ := x; obtain ⟨p2, n2⟩ := y
  simp only [Canonical, val] at hx hy h
  simp only [Num.mk.injEq]; omega

/-- Collapse depends only on the value. -/
theorem collapse_eq_of_val {x y : Num} (h : val x = val y) : collapse x = collapse y :=
  canonical_unique (collapse_canonical x) (collapse_canonical y) (by rw [val_collapse, val_collapse, h])

/-- Collapse fixes canonical pairs. -/
theorem collapse_of_canonical {x : Num} (h : Canonical x) : collapse x = x :=
  canonical_unique (collapse_canonical x) h (val_collapse x)

theorem collapse_zero : collapse ⟨0, 0⟩ = ⟨0, 0⟩ := rfl

/-- The spec's collapse (the absolute-difference routine on the clock, routed by its borrow) is the notes'
collapse, for parts below `W`. -/
theorem collapseClock_eq {W : Nat} {x : Num} (hP : x.P < W) (hN : x.N < W) : collapseClock W x = collapse x := by
  unfold collapseClock collapse
  rw [borrow_lemma hP hN]
  unfold absDiff borrow
  by_cases h : x.P < x.N
  · have : ¬ x.N ≤ x.P := by omega
    simp [h, this]
  · have : x.N ≤ x.P := by omega
    simp [h, this]

end B
end Header
