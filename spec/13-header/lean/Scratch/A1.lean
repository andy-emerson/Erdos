import Header.ProgA
open Header Header.Isa Header.PA
set_option maxHeartbeats 4000000

theorem intAdd_run (f : Form) (sub : Bool) (d1 d2 : Fin 4) (m1 m2 : Nat)
    (hd1 : d1 = 0 ∨ d1 = 2) (hd2 : d2 = 0 ∨ d2 = 2) (hm1 : m1 < W) (hm2 : m2 < W)
    (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) (e3 : mem xm = m1) (e4 : mem ym = m2) :
    let y : A.Num := if sub then A.neg ⟨m2, d2⟩ else ⟨m2, d2⟩
    ∃ r, A.add W ⟨m1, d1⟩ y = some r ∧
      ∃ s, exe (intAdd f sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = intTrace f sub (path d1 y.d m1 m2) ∧ s.stores = [(zh, hdr r.d), (zm, r.m)] := by
  cases f <;> cases sub <;> rcases hd1 with rfl|rfl <;> rcases hd2 with rfl|rfl
  all_goals by_cases h : m1 < m2
  all_goals first | (have h' : ¬ m2 ≤ m1 := by omega) | (have h' : m2 ≤ m1 := by omega)
  all_goals simp only [A.add, A.neg, A.dirCheck]
  all_goals run_prog [intAdd, pre, kk, intTrace, preT, path, regs6, hdr, e1, e2, e3, e4,
      borrow_lemma hm1 hm2, absDiff, borrow, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2, wneg_wsub hm1 hm2, h, h']
