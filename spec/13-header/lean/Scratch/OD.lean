import Header.Isa
import Header.DesignC
/-!
# Round 2: Design C as portable pseudocode (spec §4a)

A number is one word `w = 2v + a`. Round 1 (`DesignC.lean`) models a word as `BitVec 64`; the interpreter's registers
hold natural numbers below `2^64`, so a word `w` is held as `w.toNat`. The first section proves that each step of the
instruction set computes, on `toNat`, the `BitVec` operation of the same name.

**Placement**, as for Control and B: registers 1, 2, 3 hold the addresses of `x`, `y` and the result `z`; a number,
integer or complex, is one word there. C has one program per operation, for integers and complex numbers alike:
integers have axis bit 0, so they never take the cross-axis paths of add and subtract.

Scratch: `w1` in 10, `w2` in 11, the result in 14 (add, subtract, negate) or 12 (multiply, divide).
-/

set_option linter.unusedVariables false
set_option maxHeartbeats 100000

namespace Header
namespace PC
open Isa

/-! ## The instruction set on `BitVec 64` -/

abbrev Word := BitVec 64

theorem W_pow : (2 : Nat) ^ 64 = W := rfl

theorem lt_W (w : Word) : w.toNat < W := w.isLt

theorem add_bv (a b : Word) : wadd a.toNat b.toNat = (a + b).toNat := by sorry

theorem sub_bv (a b : Word) : wsub a.toNat b.toNat = (a - b).toNat := by sorry

theorem mul_bv (a b : Word) : wmul a.toNat b.toNat = (a * b).toNat := by sorry

theorem neg_bv (a : Word) : wneg a.toNat = (-a).toNat := by sorry

theorem xor_bv (a b : Word) : a.toNat ^^^ b.toNat = (a ^^^ b).toNat := (BitVec.toNat_xor a b).symm
theorem and_bv (a b : Word) : a.toNat &&& b.toNat = (a &&& b).toNat := (BitVec.toNat_and a b).symm
theorem or_bv (a b : Word) : a.toNat ||| b.toNat = (a ||| b).toNat := (BitVec.toNat_or a b).symm

/-- `andi rd, rs, 1` and `andi rd, rs, -2` (`and not 1`). -/
theorem and1_bv (a : Word) : a.toNat &&& 1 = (a &&& 1#64).toNat := by sorry

theorem andn1_bv (a : Word) : a.toNat &&& 18446744073709551614 = (a &&& ~~~1#64).toNat := by sorry

theorem shl1_bv (a : Word) : (a.toNat <<< 1) % W = (a <<< 1).toNat := by sorry

theorem sltu2_bv (a : Word) : wsltu a.toNat 2 = if a < 2#64 then 1 else 0 := by sorry

theorem toInt_bv (a : Word) : toInt a.toNat = a.toInt := by sorry

theorem toNat_of_toInt (a : Word) : a.toNat = (a.toInt % (W : Int)).toNat := by sorry

theorem div_bv (a b : Word) : wdiv a.toNat b.toNat = (a.sdiv b).toNat := by sorry

theorem sra1_bv (a : Word) : wsra a.toNat 1 = (a.sshiftRight 1).toNat := by sorry

/-- Facts that turn the interpreter's word arithmetic into round 1's `BitVec` operations. -/
theorem zero_bv : (0 : Nat) = (0#64 : Word).toNat := rfl

/-! ## The programs -/

/-- The mask for `and not 1`, `2^64 − 2` (RISC-V `andi rd, rs, -2`, ARM64 `and rd, rs, #~1`, x86-64 `and r, -2`). -/
def M2 : Nat := 18446744073709551614

/-- Add: `t = (w1 xor w2) and 1`; `t = 0`: `w1 + (w2 and not 1)`; else `w1` if `w2 <u 2`, else `w2` if `w1 <u 2`,
else an error. -/
def add : Prog := [.ld 10 1 0, .ld 11 2 0, .xor 12 10 11, .andi 12 12 1,
  .beqz 12 12,                           -- 4: same axis → 12
  .mv 14 10, .sltiu 15 11 2, .bnez 15 14, -- 5..7: w2 = 0 → w1
  .mv 14 11, .sltiu 15 10 2, .bnez 15 14, -- 8..10: w1 = 0 → w2
  .trap,                                 -- 11: an expression
  .andi 13 11 M2, .add 14 10 13,         -- 12, 13
  .sd 14 3 0]                            -- 14

/-- Subtract: as add, with `w1 − (w2 and not 1)`, and the negation `((w2 and 1) shl 1) − w2` when `w1 <u 2`. -/
def sub : Prog := [.ld 10 1 0, .ld 11 2 0, .xor 12 10 11, .andi 12 12 1,
  .beqz 12 14,                           -- 4: same axis → 14
  .mv 14 10, .sltiu 15 11 2, .bnez 15 16, -- 5..7
  .andi 16 11 1, .slli 16 16 1, .sub 14 16 11, -- 8..10: neg w2
  .sltiu 15 10 2, .bnez 15 16,           -- 11, 12
  .trap,                                 -- 13
  .andi 13 11 M2, .sub 14 10 13,         -- 14, 15
  .sd 14 3 0]                            -- 16

/-- Negate: `((w and 1) shl 1) − w`. -/
def neg : Prog := [.ld 10 1 0, .andi 12 10 1, .slli 12 12 1, .sub 14 12 10, .sd 14 3 0]

/-- Multiply: `p = (w1 sar 1)·(w2 and not 1)`, negated by the mask `0 − (w1 and w2 and 1)`, then OR
`(w1 xor w2) and 1`. -/
def mul : Prog := [.ld 10 1 0, .ld 11 2 0,
  .srai 12 10 1, .andi 13 11 M2, .mul 12 12 13,
  .and 14 10 11, .andi 14 14 1, .neg 14 14,
  .xor 12 12 14, .sub 12 12 14,
  .xor 15 10 11, .andi 15 15 1, .or 12 12 15,
  .sd 12 3 0]

/-- Exact division: `q = (w1 and not 1) sdiv (w2 and not 1)`; `t = (w1 xor w2) and 1`; `m = 0 − (t and w2)`;
`q' = (q xor m) − m`; result `(q' shl 1) + t`. -/
def div : Prog := [.ld 10 1 0, .ld 11 2 0,
  .andi 12 10 M2, .andi 13 11 M2, .div 12 12 13,
  .xor 14 10 11, .andi 14 14 1,
  .and 15 14 11, .neg 15 15,
  .xor 12 12 15, .sub 12 12 15,
  .slli 12 12 1, .add 12 12 14,
  .sd 12 3 0]

/-- The paths of add and subtract. -/
inductive Path | same | yZero | xZero | expr
deriving DecidableEq, Repr

def path (w1 w2 : Word) : Path :=
  sorry

def addTrace : Path → List Nat
  | .same => [0, 1, 2, 3, 4, 12, 13, 14]
  | .yZero => [0, 1, 2, 3, 4, 5, 6, 7, 14]
  | .xZero => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 14]
  | .expr => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

def subTrace : Path → List Nat
  | .same => [0, 1, 2, 3, 4, 14, 15, 16]
  | .yZero => [0, 1, 2, 3, 4, 5, 6, 7, 16]
  | .xZero => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16]
  | .expr => [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]

/-! ## Correctness -/

/-- Registers 1, 2, 3 hold the addresses of `x`, `y`, `z`. -/
def regs3 (x y z : Nat) : Reg → Nat := fun i =>
  if i = 1 then x else if i = 2 then y else if i = 3 then z else 0

theorem t_bv (w1 w2 : Word) : ((w1.toNat ^^^ w2.toNat) &&& 1 = 0) ↔ ((w1 ^^^ w2) &&& 1#64 = 0#64) := by sorry

theorem lt2_bv (w : Word) : (w.toNat < 2) ↔ w < 2#64 := by sorry

/-- **Add.** The program stores round 1's `C.add` when it gives a word, and ends in the error exit exactly when round 1
gives an expression (`C.add_eq_none_iff`). -/
theorem add_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    match C.add w1 w2 with
    | some w => ∃ s, exe add (regs3 x y z) mem = .done s ∧ s.trace = addTrace (path w1 w2) ∧ s.stores = [(z, w.toNat)]
    | none => ∃ s, exe add (regs3 x y z) mem = .error s ∧ s.trace = addTrace .expr ∧ s.stores = [] := by
  by_cases ht : (w1 ^^^ w2) &&& 1#64 = 0#64 <;> by_cases h2 : w2 < 2#64 <;> by_cases h1 : w1 < 2#64
  all_goals first | have htn := (t_bv w1 w2).mpr ht | have htn : ¬ _ := fun h => ht ((t_bv w1 w2).mp h)
  all_goals first | have h2n := (lt2_bv w2).mpr h2 | have h2n : ¬ _ := fun h => h2 ((lt2_bv w2).mp h)
  all_goals first | have h1n := (lt2_bv w1).mpr h1 | have h1n : ¬ _ := fun h => h1 ((lt2_bv w1).mp h)
  all_goals simp only [C.add, path, ht, h1, h2, ite_true, ite_false]
  all_goals (try rw [← add_bv, ← andn1_bv])
  rotate_left 4
  next => simp (config := {maxSteps := 3000}) [exe, run, start, exec, St.set, St.get, St.store, St.goto, St.next, add, addTrace, regs3, e1, e2, wsltu, M2, htn, h2n, h1n]
  all_goals sorry

theorem sub_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    match C.sub w1 w2 with
    | some w => ∃ s, exe sub (regs3 x y z) mem = .done s ∧ s.trace = subTrace (path w1 w2) ∧ s.stores = [(z, w.toNat)]
    | none => ∃ s, exe sub (regs3 x y z) mem = .error s ∧ s.trace = subTrace .expr ∧ s.stores = [] := by sorry

theorem neg_run (x y z : Nat) (mem : Nat → Nat) (w : Word) (e1 : mem x = w.toNat) :
    ∃ s, exe neg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4] ∧ s.stores = [(z, (C.neg w).toNat)] := by sorry

theorem lo01 (x : Word) : x &&& 1#64 = 0#64 ∨ x &&& 1#64 = 1#64 := by sorry

/-- `C.mul`'s "negated when" is the mask `0 − (w1 and w2 and 1)`. -/
theorem mul_mask (w1 w2 : Word) : C.mul w1 w2 =
    (((w1.sshiftRight 1 * (w2 &&& ~~~1#64)) ^^^ (-(w1 &&& w2 &&& 1#64))) - (-(w1 &&& w2 &&& 1#64))) |||
      ((w1 ^^^ w2) &&& 1#64) := by sorry

/-- **Multiply** stores round 1's `C.mul` (its "negated when" by the mask `0 − (w1 and w2 and 1)`). -/
theorem mul_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    ∃ s, exe mul (regs3 x y z) mem = .done s ∧ s.trace = List.range 14 ∧ s.stores = [(z, (C.mul w1 w2).toNat)] := by sorry

/-- `C.div` written out. -/
theorem div_unfold (w1 w2 : Word) : C.div w1 w2 =
    (((((w1 &&& ~~~1#64).sdiv (w2 &&& ~~~1#64)) ^^^ (-(((w1 ^^^ w2) &&& 1#64) &&& w2))) -
      (-(((w1 ^^^ w2) &&& 1#64) &&& w2))) <<< 1) + ((w1 ^^^ w2) &&& 1#64) := by sorry

/-- **Exact division** stores round 1's `C.div`, the current rule, step for step. -/
theorem div_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    ∃ s, exe div (regs3 x y z) mem = .done s ∧ s.trace = List.range 14 ∧ s.stores = [(z, (C.div w1 w2).toNat)] := by sorry

end PC
end Header
