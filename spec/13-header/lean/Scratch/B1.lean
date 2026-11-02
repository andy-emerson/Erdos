import Header.ProgB
open Header Header.Isa Header.PB
set_option maxHeartbeats 400000
example (x y z : Nat) (mem : Nat → Nat) (P1 N1 P2 N2 : Nat)
  (e1 : mem x = P1) (e2 : mem (x + 8) = N1) (e3 : mem y = P2) (e4 : mem (y + 8) = N2) : ∃ s, exe add (regs3 x y z) mem = .done s ∧ s.trace = List.range 8 ∧
    s.stores = [(z, wadd P1 P2), (z + 8, wadd N1 N2)] := by
  run_prog [add, ld4, regs3, e1, e2, e3, e4, List.range, List.range.loop]
