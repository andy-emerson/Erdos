import Header.ProgB
open Header Header.Isa Header.PB
set_option maxHeartbeats 400000
example (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (h0 : x0 < W) (h2 : x2 < W) (r : ¬ x0 < x2) (r' : x2 ≤ x0) :
    ∃ s, exe (ccollapse .notes) (regs3 x y z) mem = .done s ∧ s.trace.length = 9 := by
  simp only [ccollapse, ccollAxis]
  run_prog [regs3, ex0, ex1, ex2, ex3, wsub_of_le h0, r, r']
