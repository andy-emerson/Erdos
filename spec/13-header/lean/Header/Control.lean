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


/-- Each program stores the true result mod `2^64`; read as signed it is the true result whenever that fits
(`toInt_ofInt`). -/
theorem add_run (x y z : Nat) (mem : Nat → Nat) (a b : Int) (ha : mem x = ofInt a) (hb : mem y = ofInt b)
    : ∃ s, exe add (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a + b))] := by
  run_prog [add, regs3, ha, hb, wadd_ofInt]

theorem sub_run (x y z : Nat) (mem : Nat → Nat) (a b : Int) (ha : mem x = ofInt a) (hb : mem y = ofInt b)
    : ∃ s, exe sub (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a - b))] := by
  run_prog [sub, regs3, ha, hb, wsub_ofInt]

theorem mul_run (x y z : Nat) (mem : Nat → Nat) (a b : Int) (ha : mem x = ofInt a) (hb : mem y = ofInt b)
    : ∃ s, exe mul (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧
    s.stores = [(z, ofInt (a * b))] := by
  run_prog [mul, regs3, ha, hb, wmul_ofInt]

/-- Exact division: when `a = q·b` and `a, b` fit, the stored word is `q`. -/
theorem div_run (x y z : Nat) (mem : Nat → Nat) (a b : Int) (ha : mem x = ofInt a) (hb : mem y = ofInt b)
    (hfa : Fits a) (hfb : Fits b) (q : Int) (hq : a = q * b) (hb0 : b ≠ 0) :
    ∃ s, exe div (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3] ∧ s.stores = [(z, ofInt q)] := by
  subst hq; run_prog [div, regs3, ha, hb, wdiv_ofInt hfa hfb, Int.mul_tdiv_cancel _ hb0]


theorem neg_run (x y z : Nat) (mem : Nat → Nat) (a : Int) (ha : mem x = ofInt a) :
    ∃ s, exe neg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2] ∧ s.stores = [(z, ofInt (-a))] := by
  run_prog [neg, regs3, ha, wneg_ofInt]

/-! ## Complex numbers on the axes: a pair (real, imaginary) -/

def cadd : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 2 0, .ld 13 2 8,
  .add 14 10 12, .add 15 11 13, .sd 14 3 0, .sd 15 3 8]
def csub : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 2 0, .ld 13 2 8,
  .sub 14 10 12, .sub 15 11 13, .sd 14 3 0, .sd 15 3 8]
def cneg : Prog := [.ld 10 1 0, .ld 11 1 8, .neg 14 10, .neg 15 11, .sd 14 3 0, .sd 15 3 8]
/-- Four products: `(ar·br − ai·bi, ar·bi + ai·br)`. -/
def cmul : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 2 0, .ld 13 2 8,
  .mul 14 10 12, .mul 15 11 13, .sub 14 14 15, .mul 16 10 13, .mul 17 11 12, .add 16 16 17,
  .sd 14 3 0, .sd 16 3 8]
/-- Exact division by a number on an axis. A real divisor `c` (imaginary part 0) divides each part; an imaginary
one, `c = s·i`, gives `(a_im / s, −(a_re / s))`. One branch, on whether `c_im` is 0. -/
def cdiv : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 2 0, .ld 13 2 8,
  .bnez 13 8,
  .div 14 10 12, .div 15 11 12, .j 11,
  .div 14 11 13, .div 15 10 13, .neg 15 15,
  .sd 14 3 0, .sd 15 3 8]


theorem cadd_run (x y z : Nat) (mem : Nat → Nat) (a b : GI) (ha : mem x = ofInt a.re) (ha' : mem (x + 8) = ofInt a.im) (hb : mem y = ofInt b.re) (hb' : mem (y + 8) = ofInt b.im)
    : ∃ s, exe cadd (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4, 5, 6, 7] ∧
    s.stores = [(z, ofInt (a + b).re), (z + 8, ofInt (a + b).im)] := by
  run_prog [cadd, regs3, ha, ha', hb, hb', wadd_ofInt]

theorem csub_run (x y z : Nat) (mem : Nat → Nat) (a b : GI) (ha : mem x = ofInt a.re) (ha' : mem (x + 8) = ofInt a.im) (hb : mem y = ofInt b.re) (hb' : mem (y + 8) = ofInt b.im)
    : ∃ s, exe csub (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4, 5, 6, 7] ∧
    s.stores = [(z, ofInt (a - b).re), (z + 8, ofInt (a - b).im)] := by
  run_prog [csub, regs3, ha, ha', hb, hb', wsub_ofInt]

theorem cmul_run (x y z : Nat) (mem : Nat → Nat) (a b : GI) (ha : mem x = ofInt a.re) (ha' : mem (x + 8) = ofInt a.im) (hb : mem y = ofInt b.re) (hb' : mem (y + 8) = ofInt b.im)
    : ∃ s, exe cmul (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11] ∧
    s.stores = [(z, ofInt (a * b).re), (z + 8, ofInt (a * b).im)] := by
  run_prog [cmul, regs3, ha, ha', hb, hb', wadd_ofInt, wsub_ofInt, wmul_ofInt]

/-- The two paths of complex division: real divisor (`b_im = 0`) and imaginary divisor. -/
def cdivTrace (bim0 : Bool) : List Nat :=
  if bim0 then [0, 1, 2, 3, 4, 5, 6, 7, 11, 12] else [0, 1, 2, 3, 4, 8, 9, 10, 11, 12]

/-- **Complex exact division.** The divisor `b` lies on an axis and is not 0, `a = q·b`, and all parts fit. The
program stores `q`. -/
theorem cdiv_run (x y z : Nat) (mem : Nat → Nat) (a b : GI) (ha : mem x = ofInt a.re) (ha' : mem (x + 8) = ofInt a.im) (hb : mem y = ofInt b.re) (hb' : mem (y + 8) = ofInt b.im)
    (hax : b.re = 0 ∨ b.im = 0) (hb0 : b ≠ 0) (q : GI) (hq : a = q * b)
    (f1 : Fits a.re) (f2 : Fits a.im) (f3 : Fits b.re) (f4 : Fits b.im) :
    ∃ s, exe cdiv (regs3 x y z) mem = .done s ∧ s.trace = cdivTrace (decide (b.im = 0)) ∧
      s.stores = [(z, ofInt q.re), (z + 8, ofInt q.im)] := by
  have hre := congrArg GI.re hq; have him := congrArg GI.im hq
  simp only [GI.mul_re, GI.mul_im] at hre him
  by_cases h0 : b.im = 0
  · have hr : b.re ≠ 0 := fun h => hb0 (GI.ext' (by simp [h]) (by simp [h0]))
    have e1 : a.re.tdiv b.re = q.re := by rw [hre, h0]; simp [Int.mul_tdiv_cancel _ hr]
    have e2 : a.im.tdiv b.re = q.im := by
      rw [him, h0]; simp only [Int.mul_zero, Int.zero_add]; exact Int.mul_tdiv_cancel _ hr
    have hz : mem (y + 8) = 0 := by rw [hb', h0]; decide
    run_prog [cdiv, cdivTrace, regs3, ha, ha', hb, hz, h0, wdiv_ofInt f1 f3, wdiv_ofInt f2 f3, e1, e2]
  · have hr : b.re = 0 := by omega
    have e1 : a.im.tdiv b.im = q.re := by rw [him, hr]; simp [Int.mul_tdiv_cancel _ h0]
    have e2 : -(a.re.tdiv b.im) = q.im := by
      rw [hre, hr]; simp only [Int.mul_zero, Int.zero_sub, Int.neg_tdiv, Int.mul_tdiv_cancel _ h0, Int.neg_neg]
    have hz : ofInt b.im ≠ 0 := fun h => h0 ((ofInt_eq_zero f4).1 h)
    run_prog [cdiv, cdivTrace, regs3, ha, ha', hb, hb', hz, h0, wdiv_ofInt f2 f4, wdiv_ofInt f1 f4,
      wneg_ofInt, e1, e2]


theorem cneg_run (x y z : Nat) (mem : Nat → Nat) (a : GI)
    (ha : mem x = ofInt a.re) (ha' : mem (x + 8) = ofInt a.im) :
    ∃ s, exe cneg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4, 5] ∧
      s.stores = [(z, ofInt (-a).re), (z + 8, ofInt (-a).im)] := by
  run_prog [cneg, regs3, ha, ha', wneg_ofInt]

end Ctl
end Header
