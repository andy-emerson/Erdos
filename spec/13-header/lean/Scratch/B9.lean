import Header.ProgB
open Header Header.Isa Header.PB
set_option maxHeartbeats 400000
example (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W) (f : Form) :
    ∃ s, exe (ccollapse f) (regs3 x y z) mem = .done s ∧
      s.trace = ccollapseTrace f (decide (x0 < x2)) (decide (x1 < x3)) ∧
      s.stores = [(z, (B.collapse ⟨x0, x2⟩).P), (z + 16, (B.collapse ⟨x0, x2⟩).N),
        (z + 8, (B.collapse ⟨x1, x3⟩).P), (z + 24, (B.collapse ⟨x1, x3⟩).N)] := by
  cases f
  all_goals by_cases r : x0 < x2 <;> by_cases i : x1 < x3
  all_goals first | (have r' : ¬ x2 ≤ x0 := by omega) | (have r' : x2 ≤ x0 := by omega)
  all_goals first | (have i' : ¬ x3 ≤ x1 := by omega) | (have i' : x3 ≤ x1 := by omega)
  all_goals first | (have r'' : x0 ≤ x2 := by omega) | (have r'' : True := trivial)
  all_goals first | (have i'' : x1 ≤ x3 := by omega) | (have i'' : True := trivial)
  all_goals run_prog [ccollapse, ccollapseTrace, ccollAxis, cAxisTrace, B.collapse, regs3, ex0, ex1, ex2, ex3,
    wsub_of_le h0, wsub_of_le h1, wsub_of_le h2, wsub_of_le h3, wneg_wsub h0 h2, wneg_wsub h1 h3,
    r, i, r', i', r'', i'']
