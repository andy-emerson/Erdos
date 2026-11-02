import Header.CA_bfCx_run_sub_0
import Header.CA_bfCx_run_sub_1
import Header.CA_bfCx_run_sub_2
import Header.CA_bfCx_run_sub_3
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




theorem bfCx_run_sub (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff true m2 d2) with
    | some r => ∃ s, exe ((bfCx true)) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe ((bfCx true)) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl
  · exact bfCx_run_sub_0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 _ d2 rfl e1 e2
  · exact bfCx_run_sub_1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 _ d2 rfl e1 e2
  · exact bfCx_run_sub_2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 _ d2 rfl e1 e2
  · exact bfCx_run_sub_3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 _ d2 rfl e1 e2

end PA
end Header
