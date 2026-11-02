import Header.Control
import Header.ProgA
import Header.ProgB
import Header.ProgC
import Header.ProgMul
import Header.ProgLg
import Header.ProgCpl
/-!
# Round 2: every program and every path, in one list

`registry` names each program with its paths. A path is the trace that the `_run` theorems
(`Control.lean`, `CorrectA.lean`, `CorrectB.lean`) prove the inputs take; its name says which inputs take it.
`Counts.lean` proves the counts of each.
-/

namespace Header
namespace Reg
open Isa

def fs : List (String × Form) := [("notes", .notes), ("perArm", .perArm)]
def subs : List (String × Bool) := [("add", false), ("sub", true)]
/-- The borrow pairs `(real, imaginary)` an operand on an axis can give: at most one. -/
def feas : List (String × Bool × String × Bool) := [("0", false, "0", false), ("0", false, "1", true), ("1", true, "0", false)]

def cPaths : List (String × PC.Path) :=
  [("same axis", .same), ("cross, y = 0", .yZero), ("cross, x = 0", .xZero), ("cross, expression", .expr)]

def bools : List (String × Bool) := [("0", false), ("1", true)]

def aPaths : List (String × PA.Path) :=
  [("same", .same), ("opp, no borrow", .oppNB), ("opp, borrow", .oppB), ("cross, x = 0", .z1),
   ("cross, y = 0", .z2), ("cross, expression", .expr)]

/-- `(design, operation, program, [(path, trace)])`. -/
def registry : List (String × String × Prog × List (String × List Nat)) :=
  [("Control", "int add", Ctl.add, [("one", [0, 1, 2, 3])]),
   ("Control", "int sub", Ctl.sub, [("one", [0, 1, 2, 3])]),
   ("Control", "int neg", Ctl.neg, [("one", [0, 1, 2])]),
   ("Control", "int mul", Ctl.mul, [("one", [0, 1, 2, 3])]),
   ("Control", "int div", Ctl.div, [("one", [0, 1, 2, 3])]),
   ("Control", "cx add", Ctl.cadd, [("one", List.range 8)]),
   ("Control", "cx sub", Ctl.csub, [("one", List.range 8)]),
   ("Control", "cx neg", Ctl.cneg, [("one", List.range 6)]),
   ("Control", "cx mul", Ctl.cmul, [("one", List.range 12)]),
   ("Control", "cx div", Ctl.cdiv, [("real divisor", Ctl.cdivTrace true), ("imag divisor", Ctl.cdivTrace false)])] ++
  (fs.flatMap fun (fn, f) => subs.map fun (sn, s) =>
    ("A", "int " ++ sn ++ " (" ++ fn ++ ")", PA.intAdd f s,
      (aPaths.take 3).map fun (pn, p) => (pn, PA.intTrace f s p))) ++
  (fs.flatMap fun (fn, f) => subs.map fun (sn, s) =>
    ("A", "cx " ++ sn ++ " (" ++ fn ++ ")", PA.cxAdd f s,
      aPaths.map fun (pn, p) => (pn, PA.cxTrace f s p))) ++
  [("A", "neg", PA.neg, [("one", List.range 5)]),
   ("A", "mul", PA.mul, [("one", List.range 9)]),
   ("A", "div", PA.div, [("one", List.range 9)])] ++
  (subs.map fun (sn, s) => ("A-bf", "int " ++ sn, PA.bfInt s, [("one", List.range (PA.bfInt s).length)])) ++
  (subs.map fun (sn, s) => ("A-bf", "cx " ++ sn, PA.bfCx s, [("one", List.range (PA.bfCx s).length)])) ++
  (subs.map fun (sn, s) => ("A-mul", sn, PA.mulRow s, [("one", List.range (PA.mulRow s).length)])) ++
  (subs.map fun (sn, s) => ("A-lg", "int " ++ sn, PA.lgRow false s, [("one", List.range (PA.lgRow false s).length)])) ++
  (subs.map fun (sn, s) => ("A-lg", "cx " ++ sn, PA.lgRow true s, [("one", List.range (PA.lgRow true s).length)])) ++
  [("B", "int add", PB.add, [("one", List.range 8)]),
   ("B", "int sub", PB.sub, [("one", List.range 8)]),
   ("B", "int neg", PB.neg, [("one", List.range 4)]),
   ("B", "int mul", PB.mul, [("one", List.range 12)])] ++
  (fs.map fun (fn, f) => ("B", "int collapse (" ++ fn ++ ")", PB.collapse f,
    bools.map fun (bn, b) => ("borrow " ++ bn, PB.collapseTrace f b))) ++
  (fs.map fun (fn, f) => ("B", "int div (" ++ fn ++ ")", PB.div f,
    bools.flatMap fun (b1n, b1) => bools.map fun (b2n, b2) =>
      ("borrows x " ++ b1n ++ ", y " ++ b2n, PB.divTrace f b1 b2))) ++
  [("B", "cx add", PB.cadd, [("one", List.range 16)]),
   ("B", "cx sub", PB.csub, [("one", List.range 16)]),
   ("B", "cx neg", PB.cneg, [("one", List.range 8)]),
   ("B", "cx mul", PB.cmul, [("one", List.range 40)])] ++
  (fs.map fun (fn, f) => ("B", "cx collapse (" ++ fn ++ ")", PB.ccollapse f,
    bools.flatMap fun (b1n, b1) => bools.map fun (b2n, b2) =>
      ("borrows re " ++ b1n ++ ", im " ++ b2n, PB.ccollapseTrace f b1 b2))) ++
  (fs.map fun (fn, f) => ("B", "cx div (" ++ fn ++ ")", PB.cdiv f,
    feas.flatMap fun (a, ba, b, bb) => feas.map fun (c, bc, d, bd) =>
      ("borrows x " ++ a ++ b ++ ", y " ++ c ++ d, PB.cdivTrace f ba bb bc bd))) ++
  [("C", "add", PC.add, cPaths.map fun (n, p) => (n, PC.addTrace p)),
   ("C", "sub", PC.sub, cPaths.map fun (n, p) => (n, PC.subTrace p)),
   ("C", "neg", PC.neg, [("one", List.range 5)]),
   ("C", "mul", PC.mul, [("one", List.range 14)]),
   ("C", "div", PC.div, [("one", List.range 14)])] ++
  ([("add", PCpl.add), ("sub", PCpl.sub), ("neg", PCpl.neg), ("mul", PCpl.mul), ("div", PCpl.div)].map
    fun (n, p) => ("cpl", n, p, [("one", List.range p.length)]))

end Reg
end Header
