example (n1 n2 : Nat) (h : ¬ (n1 ^^^ n2) &&& 1 = 0) : False := by simp at h; trace_state; sorry
example (n1 n2 : Nat) (h : (n1 ^^^ n2) &&& 1 = 0) : False := by simp at h; trace_state; sorry
