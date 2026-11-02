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




theorem bfCx_run_add_2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (d1 d2 : Fin 4) (hk : d1 = 2) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff false m2 d2) with
    | some r => ∃ s, exe ((bfCx false)) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx false).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe ((bfCx false)) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx false).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have hsub1 : m1 - m2 < W := by omega
  have hsub2 : m2 - m1 < W := by omega
  have hs2 : clockAdd W m1 m2 < W := by rw [← wadd_clock hm1 hm2]; exact wadd_lt _ _
  have k0 : (128 : Nat) % W = 128 := by decide
  have k1 : (0 : Nat) % W = 0 := by decide
  have w0_0 : wadd 0 0 = 0 := by decide
  have w0_128 : wadd 0 128 = 128 := by decide
  have w64_0 : wadd 64 0 = 64 := by decide
  have w64_128 : wadd 64 128 = 192 := by decide
  have w128_0 : wadd 128 0 = 128 := by decide
  have w128_128 : wadd 128 128 = 256 := by decide
  have w192_0 : wadd 192 0 = 192 := by decide
  have w192_128 : wadd 192 128 = 320 := by decide
  obtain rfl := hk
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases h : m1 < m2
  all_goals first | (have h' : ¬ m2 ≤ m1 := by omega) | (have h' : m2 ≤ m1 := by omega)
  all_goals first | (have h'' : m1 ≤ m2 := by omega) | (have h'' : True := trivial)
  all_goals by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals try subst z1
  all_goals try subst z2
  all_goals simp only [yEff, A.add, A.neg, A.dirCheck]
  all_goals run_prog [bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' hs2 hsub1, select_ones' hs2 hsub2, select_ones' hs2 hm1, select_ones' hs2 hm2, wsub_zero hsub1, wsub_zero hsub2, wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, *]
  all_goals (simp only [clockAdd_zero]; decide)

end PA
end Header
