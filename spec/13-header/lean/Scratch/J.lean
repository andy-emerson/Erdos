import Header.Counts
open Header Header.Isa Header.Reg
def kind : Instr → String
  | .mul .. => "mul" | .divu .. => "divu" | .div .. => "div"
  | .ld .. | .lbu .. => "load" | .sd .. | .sb .. => "store"
  | .sltu .. | .sltiu .. => "sltu"
  | .beq .. | .bne .. | .bltu .. | .bgeu .. | .beqz .. | .bnez .. => "br" | .j .. => "jmp" | .trap => "trap"
  | _ => "alu"
def costs (f : Fam) : Option Instr → List Instr → List Nat
  | _, [] => []
  | prev, i :: rest => costAt f prev i rest :: costs f (some i) rest
#eval do
  IO.println "["
  let mut first := true
  for (d, op, p, paths) in registry do
    for (pn, tr) in paths do
      let is := instrs p tr
      let cs := costs .x86 none is
      let taken := (counts p tr).taken
      let items := (is.zip cs).map fun (ic : Instr × Nat) => let (i, c) := ic;
        s!"[\"{kind i}\", {c}, {(i.writes.map toString).getD "null"}, {i.reads}]"
      let sep := if first then "" else ","
      first := false
      IO.println s!"{sep}\{\"design\": \"{d}\", \"op\": \"{op}\", \"path\": \"{pn}\", \"taken\": {taken}, \"ins\": [{", ".intercalate items}]}"
  IO.println "]"
