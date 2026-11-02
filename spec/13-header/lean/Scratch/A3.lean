import Header.ProgA
open Header Header.Isa Header.PA
set_option maxHeartbeats 400000

example (m1 m2 : Nat)
    (hm1 : m1 < W) (hm2 : m2 < W)
    (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (e3 : mem xm = m1) (e4 : mem ym = m2) (h : m2 ≤ m1) :
    let y : A.Num :=  ⟨m2, 2⟩
    ∃ r, A.add W ⟨m1, 0⟩ y = some r ∧
      ∃ s, exe (intAdd .notes false) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = intTrace .notes false (path 0 y.d m1 m2) ∧ s.stores = [(zh, hdr r.d), (zm, r.m)] := by
  simp only [A.add, A.dirCheck]
  run_prog [intAdd, pre, kk, intTrace, preT, path, regs6, hdr, e1, e2, e3, e4,
      borrow_lemma hm1 hm2, absDiff, borrow, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2, wneg_wsub hm1 hm2, h]
