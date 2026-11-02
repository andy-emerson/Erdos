import Header.Isa
/-!
# Round 2: Control as portable pseudocode (spec §2)

The machine's signed 64-bit integers. A word read as signed is `toInt`; a signed integer is stored as `ofInt`, its
residue mod `2^64`.

**Placement** (the same rule for every design): the caller passes one register per list per number, holding the
address of that number's entry. Control has one list, so registers 1, 2, 3 hold the addresses of `x`, `y` and the
result `z`. An integer is one word at that address; a complex number is two, the real part at `+0` and the imaginary
part at `+8`. Every program loads all its operands before it stores anything, so a result may share storage with an
operand.

Registers 10 and up are scratch.
-/

set_option linter.unusedVariables false

namespace Header
namespace Ctl
open Isa

/-- Registers 1, 2, 3 hold the addresses of `x`, `y`, `z`. -/
def regs3 (x y z : Nat) : Reg → Nat := fun i =>
  if i = 1 then x else if i = 2 then y else if i = 3 then z else 0

theorem ofInt_eq_zero {z : Int} (h : Fits z) : ofInt z = 0 ↔ z = 0 := by
  unfold ofInt Fits H W at *; omega

/-! ## Integers -/

def add : Prog := [.ld 10 1 0, .ld 11 2 0, .add 12 10 11, .sd 12 3 0]
def sub : Prog := [.ld 10 1 0, .ld 11 2 0, .sub 12 10 11, .sd 12 3 0]
def neg : Prog := [.ld 10 1 0, .neg 12 10, .sd 12 3 0]
def mul : Prog := [.ld 10 1 0, .ld 11 2 0, .mul 12 10 11, .sd 12 3 0]
def div : Prog := [.ld 10 1 0, .ld 11 2 0, .div 12 10 11, .sd 12 3 0]

section
variable (x y z : Nat) (mem : Nat → Nat) (a b : Int) (ha : mem x = ofInt a) (hb : mem y = ofInt b)
include ha hb

/-- Each program stores the true result mod `2^64`; read as signed it is the true result whenever that fits
(`toInt_ofInt`). -/
theorem add_run : ∃ s, exe add (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a + b))] := by
  run_prog [add, regs3, ha, hb, wadd_ofInt]

theorem sub_run : ∃ s, exe sub (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a - b))] := by
  run_prog [sub, regs3, ha, hb, wsub_ofInt]

theorem mul_run : ∃ s, exe mul (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a * b))] := by
  run_prog [mul, regs3, ha, hb, wmul_ofInt]

/-- Exact division: when `a = q·b` and `a, b` fit, the stored word is `q`. -/
theorem div_run (hfa : Fits a) (hfb : Fits b) (q : Int) (hq : a = q * b) (hb0 : b ≠ 0) :
    ∃ s, exe div (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧ s.stores = [(z, ofInt q)] := by
  run_prog [div, regs3, ha, hb, wdiv_ofInt hfa hfb, hq, Int.mul_tdiv_cancel _ hb0]

end

theorem neg_run (x y z : Nat) (mem : Nat → Nat) (a : Int) (ha : mem x = ofInt a) :
    ∃ s, exe neg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2] ∧ s.stores = [(z, ofInt (-a))] := by
  run_prog [neg, regs3, ha, wneg_ofInt]

end Ctl
end Header
