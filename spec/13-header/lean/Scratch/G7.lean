import Header.ProgB
open Header Header.Isa Header.PB
set_option maxHeartbeats 400000
theorem lt_cases (a b : Nat) : (a < b ∧ ¬ b ≤ a ∧ a ≤ b) ∨ (¬ a < b ∧ b ≤ a ∧ True) := by
  by_cases h : a < b
  · exact Or.inl ⟨h, by omega, by omega⟩
  · exact Or.inr ⟨h, by omega, trivial⟩

section
variable (x y z : Nat) (mem : Nat → Nat) (x0 x1 x2 x3 y0 y1 y2 y3 : Nat)
  (ex0 : mem x = x0) (ex1 : mem (x + 8) = x1) (ex2 : mem (x + 16) = x2) (ex3 : mem (x + 24) = x3)
  (ey0 : mem y = y0) (ey1 : mem (y + 8) = y1) (ey2 : mem (y + 16) = y2) (ey3 : mem (y + 24) = y3)
include ex0 ex1 ex2 ex3 ey0 ey1 ey2 ey3
theorem foo   (h0 : x0 < W) (h1 : x1 < W) (h2 : x2 < W) (h3 : x3 < W) (f : Form) :
    ∃ s, exe (ccollapse f) (regs3 x y z) mem = .done s ∧
      s.trace = ccollapseTrace f (decide (x0 < x2)) (decide (x1 < x3)) ∧
      s.stores = [(z, (B.collapse ⟨x0, x2⟩).P), (z + 16, (B.collapse ⟨x0, x2⟩).N),
        (z + 8, (B.collapse ⟨x1, x3⟩).P), (z + 24, (B.collapse ⟨x1, x3⟩).N)] := by
  cases f
  all_goals rcases lt_cases x0 x2 with ⟨r, r', r''⟩ | ⟨r, r', r''⟩ <;> rcases lt_cases x1 x3 with ⟨i, i', i''⟩ | ⟨i, i', i''⟩
  rotate_left 7
  next =>
    run_prog [ccollapse, ccollapseTrace, ccollAxis, cAxisTrace, B.collapse, regs3, ex0, ex1, ex2, ex3,
      wsub_of_le h0, wsub_of_le h1, wsub_of_le h2, wsub_of_le h3, wneg_wsub h0 h2, wneg_wsub h1 h3,
      r, i, r', i', r'', i'']

  all_goals sorry
end
