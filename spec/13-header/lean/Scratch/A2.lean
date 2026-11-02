import Header.ProgA
open Header Header.Isa Header.PA
set_option maxHeartbeats 4000000

example (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W)
    (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (e3 : mem xm = m1) (e4 : mem ym = m2) (h : m2 ≤ m1) :
      ∃ s, exe (intAdd .notes false) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = intTrace .notes false .oppNB := by
  run_prog [intAdd, pre, kk, intTrace, preT, regs6, hdr, e1, e2, e3, e4, h]
