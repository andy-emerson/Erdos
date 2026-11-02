import Header.ProgA
import Header.CA_cxAdd_run_notes_add
import Header.CA_cxAdd_run_notes_sub
import Header.CA_cxAdd_run_perArm_add
import Header.CA_cxAdd_run_perArm_sub
import Header.CA_bfCx_run_add
import Header.CA_bfCx_run_sub
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



/-- **Integer add and subtract, both forms.** For integer directions (`d ∈ {0, 2}`) the program stores round 1's
`A.add W x y` (subtraction: `A.add W x (A.neg y)`, which is `A.sub`), along the path `path`. -/
theorem intAdd_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (f : Form) (sub : Bool) (d1 d2 : Fin 4) (hd1 : d1 = 0 ∨ d1 = 2) (hd2 : d2 = 0 ∨ d2 = 2)
    (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    ∃ r, A.add W ⟨m1, d1⟩ (yEff sub m2 d2) = some r ∧
      ∃ s, exe (intAdd f sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = intTrace f sub (path d1 (yEff sub m2 d2).d m1 m2) ∧ s.stores = [(zh, hdr r.d), (zm, r.m)] := by
  cases f <;> cases sub <;> rcases hd1 with rfl|rfl <;> rcases hd2 with rfl|rfl
  all_goals by_cases h : m1 < m2
  all_goals first | (have h' : ¬ m2 ≤ m1 := by omega) | (have h' : m2 ≤ m1 := by omega)
  all_goals first | (have h'' : m1 ≤ m2 := by omega) | (have h'' : True := trivial)
  all_goals simp only [yEff, A.add, A.neg, A.dirCheck]
  all_goals run_prog [intAdd, pre, kk, intTrace, preT, path, regs6, hdr, e1, e2, e3, e4,
      borrow_lemma hm1 hm2, absDiff, borrow, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, h, h', h'']

/-- **Complex add and subtract, the notes' whole tree, both forms.** The program stores round 1's `A.add` when it
gives a number, and ends in the error exit exactly when round 1 gives "an expression". -/
theorem cxAdd_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (f : Form) (sub : Bool) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff sub m2 d2) with
    | some r => ∃ s, exe (cxAdd f sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = cxTrace f sub (path d1 (yEff sub m2 d2).d m1 m2) ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (cxAdd f sub) (regs6 xh xm yh ym zh zm) mem = .error s ∧
        s.trace = cxTrace f sub .expr ∧ s.stores = [] := by
  cases f <;> cases sub
  all_goals first
    | exact cxAdd_run_notes_add m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2
    | exact cxAdd_run_notes_sub m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2
    | exact cxAdd_run_perArm_add m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2
    | exact cxAdd_run_perArm_sub m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2



/-- **Multiply.** Stores `(d1 + d2, m1·m2 mod 2^64)`: round 1's `A.mul` when `m1·m2 < 2^64`. -/
theorem mul_run (m1 m2 : Nat) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) (e3 : mem xm = m1) (e4 : mem ym = m2)
    : ∃ s, exe mul (regs6 xh xm yh ym zh zm) mem = .done s ∧
    s.trace = [0, 1, 2, 3, 4, 5, 6, 7, 8] ∧ s.stores = [(zh, hdr (d1 + d2)), (zm, wmul m1 m2)] := by
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    run_prog [mul, regs6, hdr, e1, e2, e3, e4, wadd, W]

/-- **Exact division.** Stores `(d1 − d2, m1 / m2)`: round 1's `A.div`. -/
theorem div_run (m1 m2 : Nat) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) (e3 : mem xm = m1) (e4 : mem ym = m2)
    : ∃ s, exe div (regs6 xh xm yh ym zh zm) mem = .done s ∧
    s.trace = [0, 1, 2, 3, 4, 5, 6, 7, 8] ∧ s.stores = [(zh, hdr (A.div ⟨m1, d1⟩ ⟨m2, d2⟩).d),
      (zm, (A.div ⟨m1, d1⟩ ⟨m2, d2⟩).m)] := by
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl <;>
    run_prog [div, A.div, regs6, hdr, e1, e2, e3, e4, wsub, wdivu, W]


/-- **Negate.** Stores round 1's `A.neg`. -/
theorem neg_run (m1 : Nat) (d1 : Fin 4) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e1 : mem xh = hdr d1) (e3 : mem xm = m1) :
    ∃ s, exe neg (regs6 xh xm yh ym zh zm) mem = .done s ∧ s.trace = [0, 1, 2, 3, 4] ∧
      s.stores = [(zh, hdr (A.neg ⟨m1, d1⟩).d), (zm, (A.neg ⟨m1, d1⟩).m)] := by
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> run_prog [neg, A.neg, regs6, hdr, e1, e3]

/-! ## The branch-free row -/


/-- **Branch-free integer add and subtract.** One path, every instruction once; the same stores as `intAdd`. -/
theorem bfInt_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (sub : Bool) (d1 d2 : Fin 4) (hd1 : d1 = 0 ∨ d1 = 2) (hd2 : d2 = 0 ∨ d2 = 2)
    (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    ∃ r, A.add W ⟨m1, d1⟩ (yEff sub m2 d2) = some r ∧
      ∃ s, exe (bfInt sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfInt sub).length ∧ s.stores = [(zh, hdr r.d), (zm, r.m)] := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have hsub1 : m1 - m2 < W := by omega
  have hsub2 : m2 - m1 < W := by omega
  have hs2 : clockAdd W m1 m2 < W := by rw [← wadd_clock hm1 hm2]; exact wadd_lt _ _
  cases sub <;> rcases hd1 with rfl|rfl <;> rcases hd2 with rfl|rfl
  all_goals by_cases h : m1 < m2
  all_goals first | (have h' : ¬ m2 ≤ m1 := by omega) | (have h' : m2 ≤ m1 := by omega)
  all_goals first | (have h'' : m1 ≤ m2 := by omega) | (have h'' : True := trivial)
  all_goals simp only [yEff, A.add, A.neg, A.dirCheck]
  all_goals run_prog [bfInt, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones', wadd_lit, mod_W_small, wsub_zero, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, h, h', h'']

/-- **Branch-free complex add and subtract.** One path. Register 21 ends as the error flag: 1 exactly when round 1
gives "an expression"; otherwise it is 0 and the stores are round 1's `A.add`. -/
theorem bfCx_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (sub : Bool) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff sub m2 d2) with
    | some r => ∃ s, exe (bfCx sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx sub).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx sub).length ∧ s.r 21 = 1 := by
  cases sub
  all_goals first
    | exact bfCx_run_add m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2
    | exact bfCx_run_sub m1 m2 hm1 hm2 xh xm yh ym zh zm mem e3 e4 d1 d2 e1 e2


end PA
end Header
