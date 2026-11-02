import Header.ProgB
open Header Header.Isa Header.PB
#eval (ccollapse .notes).length
#eval repr (ccollapse .notes)
example : (ccollapse .notes).length = 20 := by simp [ccollapse, ccollAxis]
example : (ccollapse .notes)[5]? = some (.bgeu 10 12 10) := by simp [ccollapse, ccollAxis]
