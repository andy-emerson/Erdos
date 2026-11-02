import Header.ProgA
open Header Header.Isa
example : clockAdd W 0 0 = 0 := by decide
example : (128 : Nat) % W = 128 := by decide
example : wadd 192 128 = 320 := by decide
