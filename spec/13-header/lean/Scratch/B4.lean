import Header.ProgB
open Header Header.Isa Header.PB
set_option maxHeartbeats 400000
def P1 : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 1 16, .ld 13 1 24, .sub 14 10 12, .bgeu 10 12 10, .neg 14 14, .sd 0 3 0, .sd 14 3 16, .j 12, .sd 14 3 0, .sd 0 3 16, .sub 14 11 13, .bgeu 11 13 18, .neg 14 14, .sd 0 3 8, .sd 14 3 24, .j 20, .sd 14 3 8, .sd 0 3 24]
example (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (h0 : x0 < W) (h2 : x2 < W) (r : ¬ x0 < x2) (r' : x2 ≤ x0) (i' : x3 ≤ x1) (h1 : x1 < W) :
    ∃ s, exe P1 (regs3 x y z) mem = .done s ∧ s.trace.length = 12 := by
  run_prog [P1, regs3, ex0, ex1, ex2, ex3, wsub_of_le h0, wsub_of_le h1, r, r', i']
