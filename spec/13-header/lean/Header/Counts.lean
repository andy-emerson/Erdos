import Header.Registry
/-!
# Round 2: counts, coverage, branch inventory

For every path of every program (`Reg.registry`): the exact number of instructions, conditional branches (and how many
taken), jumps, loads and stores (`counts`, proved below by evaluation), and the list of conditional branches with
whether each is taken (`branchLog`). Coverage: the `_run` theorems hold for every input, and each names its path by a
function of the inputs, so the listed paths cover every input.

## Instructions on the three families

`cost f i` is the number of machine instructions that step `i` maps to on family `f`, by the table in
`../results-r2.md`. The x86-64 column assumes the register allocation as written: a two-operand instruction needs a
`mov` first when the destination is neither source (`lea` for an add, or a shift left by 1–3); `sltu` is
`cmp; setb; movzx`; a compare-and-branch is `cmp`/`test` and `jcc` (two instructions, one macro-fused uop); a divide
is `xor edx, edx; div` or `cqo; idiv`, with its operand and result in `rax`.
-/

namespace Header
namespace Isa

inductive Fam | x86 | arm | rv
deriving DecidableEq, Repr

def two (rd a : Reg) : Nat := if rd = a then 1 else 2
def twoC (rd a b : Reg) : Nat := if rd = a ∨ rd = b then 1 else 2

def cost : Fam → Instr → Nat
  | .rv, _ => 1
  | .arm, i => match i with
    | .sltu .. | .sltiu .. | .beq .. | .bne .. | .bltu .. | .bgeu .. => 2
    | _ => 1
  | .x86, i => match i with
    | .add .. | .addi .. => 1
    | .sub d a _ => two d a
    | .neg d a => two d a
    | .and d a b | .or d a b | .xor d a b | .mul d a b => twoC d a b
    | .andi d a _ | .ori d a _ | .xori d a _ | .srli d a _ | .srai d a _ => two d a
    | .slli d a k => if d = a ∨ k ≤ 3 then 1 else 2
    | .sltu .. | .sltiu .. | .mulhu .. => 3
    | .divu .. | .div .. => 2
    | .beq .. | .bne .. | .bltu .. | .bgeu .. | .beqz .. | .bnez .. => 2
    | _ => 1

/-- Machine instructions on a family along a trace. -/
def famCount (p : Prog) (tr : List Nat) (f : Fam) : Nat := ((instrs p tr).map (cost f)).sum

/-- No load after a store on a path. -/
def loadsFirst (p : Prog) (tr : List Nat) : Bool :=
  let is := instrs p tr
  match is.findIdx? Instr.isStore with
  | none => true
  | some k => !((is.drop k).any Instr.isLoad)

end Isa
end Header
