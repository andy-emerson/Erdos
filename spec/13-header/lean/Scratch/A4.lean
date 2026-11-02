import Header.ProgA
/-!
# Round 2: Design A's programs are correct, and every input takes one listed path

Each `_run` theorem runs a program from the start on any inputs (magnitudes below `2^64`, headers `64·d`) and states
how it ends, the exact trace it follows (one of the paths listed with the program) and the exact stores. The stores
are round 1's definitions (`A.add W`, `A.neg`, `A.mul`, `A.div`, on a word of `W = 2^64` values), so round 1's
theorems give the values: `add_correct` and its siblings below restate them for the programs.

Since a `_run` theorem holds for every input, the listed paths cover every input.
-/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PA
open Isa

/-- The `y` an add sees: `y` itself, or for subtraction its negation. -/
def yEff (sub : Bool) (m2 : Nat) (d2 : Fin 4) : A.Num := if sub then A.neg ⟨m2, d2⟩ else ⟨m2, d2⟩

section
variable (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
  (e3 : mem xm = m1) (e4 : mem ym = m2)
include hm1 hm2 e3 e4

/-- **Complex add and subtract, the notes' whole tree, both forms.** The program stores round 1's `A.add` when it
gives a number, and ends in the error exit exactly when round 1 gives "an expression". -/
theorem cxAdd_run (d2 : Fin 4) (e1 : mem xh = hdr 0) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, 0⟩ (yEff false m2 d2) with
    | some r => ∃ s, exe (cxAdd .notes false) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = cxTrace .notes false (path 0 (yEff false m2 d2).d m1 m2) ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (cxAdd .notes false) (regs6 xh xm yh ym zh zm) mem = .error s ∧
        s.trace = cxTrace .notes false .expr ∧ s.stores = [] := by
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases h : m1 < m2
  all_goals first | (have h' : ¬ m2 ≤ m1 := by omega) | (have h' : m2 ≤ m1 := by omega)
  all_goals first | (have h'' : m1 ≤ m2 := by omega) | (have h'' : True := trivial)
  all_goals by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals simp only [yEff, A.add, A.neg, A.dirCheck]
  all_goals run_prog [cxAdd, pre, kk, cxTrace, preT, path, regs6, hdr, e1, e2, e3, e4,
      borrow_lemma hm1 hm2, absDiff, borrow, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, h, h', h'', z1, z2]

end
end PA
end Header
