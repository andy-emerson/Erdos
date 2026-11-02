import Header.ProgC
open Header Header.Isa Header.PC
set_option maxHeartbeats 100000
example (x y z n1 n2 : Nat) (mem : Nat → Nat) (e1 : mem x = n1) (e2 : mem y = n2)
  (htn : ¬ (n1 ^^^ n2) &&& 1 = 0) (h2n : n2 < 2) :
  ∃ s, run add 7 (start (regs3 x y z) mem) = .done s := by
  simp [run, start, exec, St.set, St.get, St.store, St.goto, St.next, add, regs3, e1, e2, wsltu, M2, htn, h2n]
  sorry
