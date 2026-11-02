import Header.ProgA
open Header Header.Isa
theorem c00 : clockAdd W 0 0 = 0 := by decide
example : (clockAdd W 0 0 &&& 192) % 256 = 0 ∧ clockAdd W 0 0 ^^^ clockAdd W 0 0 &&& 18446744073709551615 = 0 := by
  simp only [c00]; decide
