import Header.ProgC
/-!
# Round 2: Design C's programs are correct, and every input takes one listed path

Each `_run` theorem holds for every pair of words and states the end, the exact trace and the exact store: round 1's
`C.add`, `C.sub`, `C.neg`, `C.mul`, `C.div` (`DesignC.lean`), whose value theorems (`C.add_value`, `C.div_value`, …)
then give the values under the no-overflow hypotheses.
-/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PC
open Isa

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
  all_goals generalize BitVec.toNat w1 = n1 at *
  all_goals generalize BitVec.toNat w2 = n2 at *
  all_goals simp only [Nat.and_one_is_mod] at htn h1n h2n
  all_goals run_prog [add, addTrace, regs3, e1, e2, wsltu, M2, htn, h2n, h1n]

theorem sub_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    match C.sub w1 w2 with
    | some w => ∃ s, exe sub (regs3 x y z) mem = .done s ∧ s.trace = subTrace (path w1 w2) ∧ s.stores = [(z, w.toNat)]
    | none => ∃ s, exe sub (regs3 x y z) mem = .error s ∧ s.trace = subTrace .expr ∧ s.stores = [] := by
  by_cases ht : (w1 ^^^ w2) &&& 1#64 = 0#64 <;> by_cases h2 : w2 < 2#64 <;> by_cases h1 : w1 < 2#64
  all_goals first | have htn := (t_bv w1 w2).mpr ht | have htn : ¬ _ := fun h => ht ((t_bv w1 w2).mp h)
  all_goals first | have h2n := (lt2_bv w2).mpr h2 | have h2n : ¬ _ := fun h => h2 ((lt2_bv w2).mp h)
  all_goals first | have h1n := (lt2_bv w1).mpr h1 | have h1n : ¬ _ := fun h => h1 ((lt2_bv w1).mp h)
  all_goals simp only [C.sub, C.neg, path, ht, h1, h2, ite_true, ite_false]
  all_goals (try rw [← sub_bv, ← andn1_bv])
  all_goals (try rw [← sub_bv, ← shl1_bv, ← and1_bv])
  all_goals generalize BitVec.toNat w1 = n1 at *
  all_goals generalize BitVec.toNat w2 = n2 at *
  all_goals simp only [Nat.and_one_is_mod] at htn h1n h2n
  all_goals run_prog [sub, subTrace, regs3, e1, e2, wsltu, M2, htn, h2n, h1n]

theorem neg_run (x y z : Nat) (mem : Nat → Nat) (w : Word) (e1 : mem x = w.toNat) :
    ∃ s, exe neg (regs3 x y z) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4] ∧ s.stores = [(z, (C.neg w).toNat)] := by
  rw [C.neg, ← sub_bv, ← shl1_bv, ← and1_bv]
  run_prog [neg, regs3, e1]

theorem lo01 (x : Word) : x &&& 1#64 = 0#64 ∨ x &&& 1#64 = 1#64 := by
  have h := C.and_one_toNat x
  rcases Nat.mod_two_eq_zero_or_one x.toNat with k | k
  · left; apply BitVec.eq_of_toNat_eq; rw [h, k]; rfl
  · right; apply BitVec.eq_of_toNat_eq; rw [h, k]; rfl

/-- `C.mul`'s "negated when" is the mask `0 − (w1 and w2 and 1)`. -/
theorem mul_mask (w1 w2 : Word) : C.mul w1 w2 =
    (((w1.sshiftRight 1 * (w2 &&& ~~~1#64)) ^^^ (-(w1 &&& w2 &&& 1#64))) - (-(w1 &&& w2 &&& 1#64))) |||
      ((w1 ^^^ w2) &&& 1#64) := by
  unfold C.mul
  rcases lo01 (w1 &&& w2) with h | h
  · simp only [h]; simp
  · simp only [h, ite_true]
    have := C.xor_ones_sub (w1.sshiftRight 1 * (w2 &&& ~~~1#64))
    rw [show -(1#64) = -1#64 from rfl, this]

/-- **Multiply** stores round 1's `C.mul` (its "negated when" by the mask `0 − (w1 and w2 and 1)`). -/
theorem mul_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    ∃ s, exe mul (regs3 x y z) mem = .done s ∧ s.trace = List.range 14 ∧ s.stores = [(z, (C.mul w1 w2).toNat)] := by
  rw [mul_mask, ← or_bv, ← sub_bv, ← xor_bv, ← neg_bv, ← and1_bv, ← and_bv, ← mul_bv, ← sra1_bv, ← andn1_bv,
    ← and1_bv, ← xor_bv]
  run_prog [mul, regs3, e1, e2, List.range, List.range.loop, M2]

/-- `C.div` written out. -/
theorem div_unfold (w1 w2 : Word) : C.div w1 w2 =
    (((((w1 &&& ~~~1#64).sdiv (w2 &&& ~~~1#64)) ^^^ (-(((w1 ^^^ w2) &&& 1#64) &&& w2))) -
      (-(((w1 ^^^ w2) &&& 1#64) &&& w2))) <<< 1) + ((w1 ^^^ w2) &&& 1#64) := by
  simp only [C.div, BitVec.zero_sub]

/-- **Exact division** stores round 1's `C.div`, the current rule, step for step. -/
theorem div_run (x y z : Nat) (mem : Nat → Nat) (w1 w2 : Word) (e1 : mem x = w1.toNat) (e2 : mem y = w2.toNat) :
    ∃ s, exe div (regs3 x y z) mem = .done s ∧ s.trace = List.range 14 ∧ s.stores = [(z, (C.div w1 w2).toNat)] := by
  rw [div_unfold, ← add_bv, ← shl1_bv, ← sub_bv, ← xor_bv, ← neg_bv, ← and_bv, ← div_bv, ← andn1_bv, ← andn1_bv,
    ← and1_bv, ← xor_bv]
  run_prog [div, regs3, e1, e2, List.range, List.range.loop, M2]

end PC
end Header
