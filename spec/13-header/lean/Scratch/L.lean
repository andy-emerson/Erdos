import Header.Counts
open Header Header.Isa Header.Reg
def r (i : Nat) : String := if i = 0 then "zero" else s!"r{i}"
def show1 : Instr → String
  | .add d a b => s!"add {r d}, {r a}, {r b}" | .sub d a b => s!"sub {r d}, {r a}, {r b}"
  | .neg d a => s!"neg {r d}, {r a}" | .and d a b => s!"and {r d}, {r a}, {r b}"
  | .or d a b => s!"or {r d}, {r a}, {r b}" | .xor d a b => s!"xor {r d}, {r a}, {r b}"
  | .addi d a k => s!"addi {r d}, {r a}, {k}" | .andi d a k => s!"andi {r d}, {r a}, {k}"
  | .ori d a k => s!"ori {r d}, {r a}, {k}" | .xori d a k => s!"xori {r d}, {r a}, {k}"
  | .slli d a k => s!"slli {r d}, {r a}, {k}" | .srli d a k => s!"srli {r d}, {r a}, {k}"
  | .srai d a k => s!"srai {r d}, {r a}, {k}" | .sll d a b => s!"sll {r d}, {r a}, {r b}"
  | .srl d a b => s!"srl {r d}, {r a}, {r b}" | .sra d a b => s!"sra {r d}, {r a}, {r b}"
  | .sltu d a b => s!"sltu {r d}, {r a}, {r b}" | .sltiu d a k => s!"sltiu {r d}, {r a}, {k}"
  | .li d k => s!"li {r d}, {k}" | .mv d a => s!"mv {r d}, {r a}"
  | .mul d a b => s!"mul {r d}, {r a}, {r b}" | .mulhu d a b => s!"mulhu {r d}, {r a}, {r b}"
  | .divu d a b => s!"divu {r d}, {r a}, {r b}" | .div d a b => s!"div {r d}, {r a}, {r b}"
  | .ld d b o => s!"ld {r d}, {o}({r b})" | .lbu d b o => s!"lbu {r d}, {o}({r b})"
  | .sd a b o => s!"sd {r a}, {o}({r b})" | .sb a b o => s!"sb {r a}, {o}({r b})"
  | .beq a b t => s!"beq {r a}, {r b}, {t}" | .bne a b t => s!"bne {r a}, {r b}, {t}"
  | .bltu a b t => s!"bltu {r a}, {r b}, {t}" | .bgeu a b t => s!"bgeu {r a}, {r b}, {t}"
  | .beqz a t => s!"beqz {r a}, {t}" | .bnez a t => s!"bnez {r a}, {t}"
  | .j t => s!"j {t}" | .trap => "trap"
#eval do
  for (d, op, p, _) in registry do
    IO.println s!"### {d} {op}"
    let mut k := 0
    for i in p do
      IO.println s!"{k}: {show1 i}"
      k := k + 1
