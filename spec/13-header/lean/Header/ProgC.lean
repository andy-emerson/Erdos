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
set_option maxHeartbeats 8000000

namespace Header
namespace PC
open Isa

/-! ## The instruction set on `BitVec 64` -/

abbrev Word := BitVec 64

theorem W_pow : (2 : Nat) ^ 64 = W := rfl

theorem lt_W (w : Word) : w.toNat < W := w.isLt

theorem add_bv (a b : Word) : wadd a.toNat b.toNat = (a + b).toNat := by
  simp [wadd, BitVec.toNat_add, W_pow]

theorem sub_bv (a b : Word) : wsub a.toNat b.toNat = (a - b).toNat := by
  have := lt_W a; have := lt_W b
  simp only [wsub, BitVec.toNat_sub, W_pow]; unfold W at *; omega

theorem mul_bv (a b : Word) : wmul a.toNat b.toNat = (a * b).toNat := by
  simp [wmul, BitVec.toNat_mul, W_pow]

theorem neg_bv (a : Word) : wneg a.toNat = (-a).toNat := by
  have := lt_W a
  simp only [wneg, wsub, BitVec.toNat_neg, W_pow]; unfold W at *; omega

theorem xor_bv (a b : Word) : a.toNat ^^^ b.toNat = (a ^^^ b).toNat := (BitVec.toNat_xor a b).symm
theorem and_bv (a b : Word) : a.toNat &&& b.toNat = (a &&& b).toNat := (BitVec.toNat_and a b).symm
theorem or_bv (a b : Word) : a.toNat ||| b.toNat = (a ||| b).toNat := (BitVec.toNat_or a b).symm

/-- `andi rd, rs, 1` and `andi rd, rs, -2` (`and not 1`). -/
theorem and1_bv (a : Word) : a.toNat &&& 1 = (a &&& 1#64).toNat := by simp [BitVec.toNat_and]
theorem andn1_bv (a : Word) : a.toNat &&& 18446744073709551614 = (a &&& ~~~1#64).toNat := by
  simp [BitVec.toNat_and, BitVec.toNat_not]

theorem shl1_bv (a : Word) : (a.toNat <<< 1) % W = (a <<< 1).toNat := by
  simp [BitVec.toNat_shiftLeft, W_pow]

theorem sltu2_bv (a : Word) : wsltu a.toNat 2 = if a < 2#64 then 1 else 0 := by
  simp [wsltu, BitVec.lt_def]

theorem toInt_bv (a : Word) : toInt a.toNat = a.toInt := by
  have := lt_W a
  rw [BitVec.toInt_eq_toNat_cond]; unfold toInt H; unfold W at *; split <;> split <;> omega

theorem toNat_of_toInt (a : Word) : a.toNat = (a.toInt % (W : Int)).toNat := by
  have := lt_W a
  rw [BitVec.toInt_eq_toNat_cond]; unfold W at *; split <;> omega

theorem div_bv (a b : Word) : wdiv a.toNat b.toNat = (a.sdiv b).toNat := by
  rw [toNat_of_toInt (a.sdiv b), BitVec.toInt_sdiv, wdiv, toInt_bv, toInt_bv, ofInt]
  congr 1
  simp only [Int.bmod_def]
  unfold W; split <;> omega

theorem sra1_bv (a : Word) : wsra a.toNat 1 = (a.sshiftRight 1).toNat := by
  rw [toNat_of_toInt (a.sshiftRight 1), BitVec.toInt_sshiftRight, wsra, toInt_bv, ofInt, Int.shiftRight_eq_div_pow]
  rfl

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
  if (w1 ^^^ w2) &&& 1#64 = 0#64 then .same
  else if w2 < 2#64 then .yZero else if w1 < 2#64 then .xZero else .expr

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

theorem t_bv (w1 w2 : Word) : ((w1.toNat ^^^ w2.toNat) &&& 1 = 0) ↔ ((w1 ^^^ w2) &&& 1#64 = 0#64) := by
  rw [xor_bv, and1_bv]
  constructor
  · intro h; exact BitVec.eq_of_toNat_eq (by rw [h]; rfl)
  · intro h; rw [h]; rfl

theorem lt2_bv (w : Word) : (w.toNat < 2) ↔ w < 2#64 := by simp [BitVec.lt_def]

end PC
end Header
