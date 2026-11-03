import Header.MR_add_0
import Header.MR_add_1
import Header.MR_add_2
import Header.MR_add_3
import Header.MR_sub_0
import Header.MR_sub_1
import Header.MR_sub_2
import Header.MR_sub_3
/-!
# A's multiplication row is correct

`mulRow_run` assembles the 160 case lemmas of `MR_*.lean` (operation × direction of `x` × direction of `y` × five
magnitude cases).
-/

set_option linter.unusedVariables false

namespace Header
namespace PA
open Isa

/-- **The multiplication row, add and subtract.** One path, every instruction once. Register 21 ends as the error
flag: 1 exactly when round 1 gives "an expression"; otherwise 0, and the stores are round 1's `A.add` (subtract:
`A.add W x (A.neg y)`, which is `A.sub`). -/
theorem mulRow_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e3 : mem xm = m1) (e4 : mem ym = m2) (sub : Bool) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff sub m2 d2) with
    | some r => ∃ s, exe (mulRow sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow sub).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow sub).length ∧ s.r 21 = 1 := by
  by_cases h : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  · exfalso; omega
  · cases sub
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_0_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_1_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_2_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_3_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_0_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_1_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_2_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_3_0_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_1_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_2_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_3_c0 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
  · exfalso; omega
  · cases sub
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_0_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_1_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_2_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_3_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_0_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_1_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_2_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_3_0_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_1_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_2_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_3_c1 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
  · cases sub
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_0_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_1_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_2_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_3_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_0_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_1_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_2_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_3_0_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_1_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_2_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_3_c2 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
  · exfalso; omega
  · cases sub
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_0_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_1_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_2_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_3_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_0_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_1_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_2_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_3_0_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_1_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_2_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_3_c3 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
  · cases sub
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_0_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_0_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_1_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_1_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_2_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_2_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_add_3_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_add_3_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
    · rcases fin4_cases d1 with rfl|rfl|rfl|rfl
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_0_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_0_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_1_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_1_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_2_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_2_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
      · rcases fin4_cases d2 with rfl|rfl|rfl|rfl
        · exact mulRow_sub_3_0_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_1_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_2_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2
        · exact mulRow_sub_3_3_c4 m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 e1 e2 h z1 z2

end PA
end Header
