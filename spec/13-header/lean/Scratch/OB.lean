import Header.ProgB
import Header.DesignA
/-!
# Round 2: Design B's programs are correct, and every input takes one listed path

As for A: each `_run` theorem holds for every input (parts below `2^64`), and states the end, the exact trace and the
exact stores. Sums and products are stored mod `2^64`; under the no-overflow hypotheses they are round 1's
`B.add`, `B.mul`, `B4.mul`, … exactly (`_value` theorems).
-/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PB
open Isa

theorem wadd_of_lt {a b : Nat} (h : a + b < W) : wadd a b = a + b := by sorry

theorem wadd_wmul2 (a b c d : Nat) : wadd (wmul a b) (wmul c d) = (a * b + c * d) % W := by sorry

theorem wadd_wmul4 (a b c d e f g h : Nat) :
    wadd (wadd (wadd (wmul a b) (wmul c d)) (wmul e f)) (wmul g h) = (a * b + c * d + e * f + g * h) % W := by sorry

theorem wsltu_eq (a b : Nat) : wsltu a b = if a < b then 1 else 0 := rfl

/-! ## Integers -/

section
variable (x y z : Nat) (mem : Nat → Nat) (P1 N1 P2 N2 : Nat)
  (e1 : mem x = P1) (e2 : mem (x + 8) = N1) (e3 : mem y = P2) (e4 : mem (y + 8) = N2)
include e1 e2 e3 e4

theorem add_run : ∃ s, exe add (regs3 x y z) mem = .done s ∧ s.trace = List.range 8 ∧
    s.stores = [(z, wadd P1 P2), (z + 8, wadd N1 N2)] := by sorry

theorem sub_run : ∃ s, exe sub (regs3 x y z) mem = .done s ∧ s.trace = List.range 8 ∧
    s.stores = [(z, wadd P1 N2), (z + 8, wadd N1 P2)] := by sorry

theorem mul_run : ∃ s, exe mul (regs3 x y z) mem = .done s ∧ s.trace = List.range 12 ∧
    s.stores = [(z, (P1 * P2 + N1 * N2) % W), (z + 8, (P1 * N2 + N1 * P2) % W)] := by sorry

/-- **Exact division, both forms.** With `b1 = (P1 < N1)`, `b2 = (P2 < N2)` and the quotient
`q = |P1 − N1| / |P2 − N2|`, the program stores 0 to both parts, then `q` to part `b1 xor b2`. -/
theorem div_run (f : Form) (hP1 : P1 < W) (hN1 : N1 < W) (hP2 : P2 < W) (hN2 : N2 < W) (hz : z + 24 < W) :
    ∃ s, exe (div f) (regs3 x y z) mem = .done s ∧ s.trace = divTrace f (decide (P1 < N1)) (decide (P2 < N2)) ∧
      s.stores = [(z, 0), (z + 8, 0),
        (z + 8 * (if decide (P1 < N1) = decide (P2 < N2) then 0 else 1), absDiff P1 N1 / absDiff P2 N2)] := by sorry

end

theorem neg_run (x y z : Nat) (mem : Nat → Nat) (P N : Nat) (e1 : mem x = P) (e2 : mem (x + 8) = N) :
    ∃ s, exe neg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
      s.stores = [(z, (B.neg ⟨P, N⟩).P), (z + 8, (B.neg ⟨P, N⟩).N)] := by sorry

/-- **Collapse, both forms.** Stores round 1's `B.collapse` (the notes' form, which `B.collapseClock_eq` equates
with the spec's), along the path of the borrow `P < N`. -/
theorem collapse_run (f : Form) (x y z : Nat) (mem : Nat → Nat) (P N : Nat) (hP : P < W) (hN : N < W)
    (e1 : mem x = P) (e2 : mem (x + 8) = N) :
    ∃ s, exe (collapse f) (regs3 x y z) mem = .done s ∧ s.trace = collapseTrace f (decide (P < N)) ∧
      s.stores = [(z, (B.collapse ⟨P, N⟩).P), (z + 8, (B.collapse ⟨P, N⟩).N)] := by sorry

/-! ### Values, from round 1 -/

/-- Add, subtract and multiply store round 1's results under no overflow. -/
theorem add_value {P1 N1 P2 N2 : Nat} (h1 : P1 + P2 < W) (h2 : N1 + N2 < W) :
    (wadd P1 P2, wadd N1 N2) = ((B.add ⟨P1, N1⟩ ⟨P2, N2⟩).P, (B.add ⟨P1, N1⟩ ⟨P2, N2⟩).N) := by sorry

theorem sub_value {P1 N1 P2 N2 : Nat} (h1 : P1 + N2 < W) (h2 : N1 + P2 < W) :
    (wadd P1 N2, wadd N1 P2) = ((B.sub ⟨P1, N1⟩ ⟨P2, N2⟩).P, (B.sub ⟨P1, N1⟩ ⟨P2, N2⟩).N) := by sorry

theorem mul_value {P1 N1 P2 N2 : Nat} (h1 : P1 * P2 + N1 * N2 < W) (h2 : P1 * N2 + N1 * P2 < W) :
    ((P1 * P2 + N1 * N2) % W, (P1 * N2 + N1 * P2) % W) =
      ((B.mul ⟨P1, N1⟩ ⟨P2, N2⟩).P, (B.mul ⟨P1, N1⟩ ⟨P2, N2⟩).N) := by sorry

/-- Division's result as a pair: the quotient in part `b1 xor b2`. -/
def divRes (P1 N1 P2 N2 : Nat) : B.Num :=
  if decide (P1 < N1) = decide (P2 < N2) then ⟨absDiff P1 N1 / absDiff P2 N2, 0⟩
  else ⟨0, absDiff P1 N1 / absDiff P2 N2⟩

/-- **Division's value.** When `y ≠ 0` and `|P2 − N2|` divides `|P1 − N1|` (exact division), the stored pair is
collapsed and times `y` gives `x`. -/
theorem div_value {P1 N1 P2 N2 : Nat} (hy : P2 ≠ N2) (hd : absDiff P2 N2 ∣ absDiff P1 N1) :
    B.Canonical (divRes P1 N1 P2 N2) ∧ B.val (divRes P1 N1 P2 N2) * B.val ⟨P2, N2⟩ = B.val ⟨P1, N1⟩ := by sorry

/-! ## Complex numbers -/

/-- Four slots from four words. -/
def v4 (a b c d : Nat) : B4.Num := fun k => match k with
  | 0 => a | 1 => b | 2 => c | 3 => d

section
variable (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
include ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3

theorem cadd_run : ∃ s, exe cadd (regs3 x y z) mem = .done s ∧ s.trace = List.range 16 ∧
    s.stores = [(z, wadd x0 y0), (z + 8, wadd x1 y1), (z + 16, wadd x2 y2), (z + 24, wadd x3 y3)] := by sorry

theorem csub_run : ∃ s, exe csub (regs3 x y z) mem = .done s ∧ s.trace = List.range 16 ∧
    s.stores = [(z, wadd x0 y2), (z + 8, wadd x1 y3), (z + 16, wadd x2 y0), (z + 24, wadd x3 y1)] := by sorry

/-- **Multiply.** Slot `k` is stored as round 1's `B4.mul` slot `k` mod `2^64` (`cmul_slots`). -/
theorem cmul_run : ∃ s, exe cmul (regs3 x y z) mem = .done s ∧ s.trace = List.range 40 ∧
    s.stores = [(z, (x0 * y0 + x1 * y3 + x2 * y2 + x3 * y1) % W), (z + 8, (x0 * y1 + x1 * y0 + x2 * y3 + x3 * y2) % W),
      (z + 16, (x0 * y2 + x1 * y1 + x2 * y0 + x3 * y3) % W), (z + 24, (x0 * y3 + x1 * y2 + x2 * y1 + x3 * y0) % W)] := by sorry

theorem cneg_run : ∃ s, exe cneg (regs3 x y z) mem = .done s ∧ s.trace = List.range 8 ∧
    s.stores = [(z, x2), (z + 8, x3), (z + 16, x0), (z + 24, x1)] := by sorry

/-- **Collapse, both forms.** Each axis stores round 1's `B.collapse` of its pair (slots 0/2, then 1/3), along the
path of the two borrows. -/
theorem ccollapse_run (f : Form) (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W) :
    ∃ s, exe (ccollapse f) (regs3 x y z) mem = .done s ∧
      s.trace = ccollapseTrace f (decide (x0 < x2)) (decide (x1 < x3)) ∧
      s.stores = [(z, (B.collapse ⟨x0, x2⟩).P), (z + 16, (B.collapse ⟨x0, x2⟩).N),
        (z + 8, (B.collapse ⟨x1, x3⟩).P), (z + 24, (B.collapse ⟨x1, x3⟩).N)] := by
  cases f
  all_goals by_cases r : x0 < x2 <;> by_cases i : x1 < x3
  all_goals first | (have r' : ¬ x2 ≤ x0 := by omega) | (have r' : x2 ≤ x0 := by omega)
  all_goals first | (have i' : ¬ x3 ≤ x1 := by omega) | (have i' : x3 ≤ x1 := by omega)
  all_goals first | (have r'' : x0 ≤ x2 := by omega) | (have r'' : True := trivial)
  all_goals first | (have i'' : x1 ≤ x3 := by omega) | (have i'' : True := trivial)
  all_goals run_prog [ccollapse, ccollapseTrace, ccollAxis, cAxisTrace, B.collapse, regs3, ex0, ex1, ex2, ex3,
    wsub_of_le h0, wsub_of_le h1, wsub_of_le h2, wsub_of_le h3, wneg_wsub h0 h2, wneg_wsub h1 h3,
    r, i, r', i', r'', i'']

end

/-! ### Complex division -/

/-- A four-slot number on an axis, read as a Design A number: the real axis when slots 1 and 3 are equal, else the
imaginary axis. -/
def toA (a b c d : Nat) : A.Num :=
  if b = d then ⟨absDiff a c, if a < c then 2 else 0⟩ else ⟨absDiff b d, if b < d then 3 else 1⟩

/-- The four-slot number with magnitude `m` in slot `d` and 0 elsewhere. -/
def ofA (r : A.Num) : B4.Num := fun k => if k = r.d then r.m else 0

/-- On an axis, the reading keeps the value. -/
theorem toA_val (a b c d : Nat) (h : b = d ∨ a = c) : A.val (toA a b c d) = B4.val (v4 a b c d) := by sorry

theorem ofA_val (r : A.Num) : B4.val (ofA r) = A.val r := by sorry

/-- **Complex division's value.** For `x` and `y` on axes, `y ≠ 0` and exact division, the result (round 1's
`A.div` of the two readings, placed in its slot) times `y` is `x`. -/
theorem cdiv_value (x0 x1 x2 x3 y0 y1 y2 y3 : Nat) (hx : x1 = x3 ∨ x0 = x2) (hy : y1 = y3 ∨ y0 = y2)
    (hy0 : 0 < (toA y0 y1 y2 y3).m) (hd : (toA y0 y1 y2 y3).m ∣ (toA x0 x1 x2 x3).m) :
    B4.val (ofA (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3))) * B4.val (v4 y0 y1 y2 y3) =
      B4.val (v4 x0 x1 x2 x3) := by sorry

section
variable (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
include ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3

/-- **Complex exact division, both forms.** For `x` and `y` on axes, the program stores 0 to the four slots, then
round 1's `A.div` of the two readings to its slot (`cdiv_value` gives its value). The path is set by the four borrows. -/
theorem cdiv_run (f : Form) (hx : x1 = x3 ∨ x0 = x2) (hy : y1 = y3 ∨ y0 = y2)
    (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W)
    (g0 : y0 < W) (g1 : y1 < W) (g2 : y2 < W) (g3 : y3 < W) (hz : z + 24 < W) :
    ∃ s, exe (cdiv f) (regs3 x y z) mem = .done s ∧
      s.trace = cdivTrace f (decide (x0 < x2)) (decide (x1 < x3)) (decide (y0 < y2)) (decide (y1 < y3)) ∧
      s.stores = [(z, 0), (z + 8, 0), (z + 16, 0), (z + 24, 0),
        (z + 8 * (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).d.val,
          (A.div (toA x0 x1 x2 x3) (toA y0 y1 y2 y3)).m)] := by sorry

end

/-- `B4.mul`'s slots, written out. -/
theorem cmul_slots (x0 x1 x2 x3 y0 y1 y2 y3 : Nat) :
    B4.mul (v4 x0 x1 x2 x3) (v4 y0 y1 y2 y3) 0 = x0 * y0 + x1 * y3 + x2 * y2 + x3 * y1 ∧
    B4.mul (v4 x0 x1 x2 x3) (v4 y0 y1 y2 y3) 1 = x0 * y1 + x1 * y0 + x2 * y3 + x3 * y2 ∧
    B4.mul (v4 x0 x1 x2 x3) (v4 y0 y1 y2 y3) 2 = x0 * y2 + x1 * y1 + x2 * y0 + x3 * y3 ∧
    B4.mul (v4 x0 x1 x2 x3) (v4 y0 y1 y2 y3) 3 = x0 * y3 + x1 * y2 + x2 * y1 + x3 * y0 :=
  sorry

end PB
end Header
