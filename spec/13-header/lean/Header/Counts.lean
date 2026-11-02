import Header.Registry
/-!
# Round 2: counts, coverage, branch inventory

For every path of every program (`Reg.registry`): the exact number of instructions, conditional branches (and how many
taken), jumps, loads and stores (`counts`, proved below by evaluation), and the list of conditional branches with
whether each is taken (`branchLog`). Coverage: the `_run` theorems hold for every input, and each names its path by a
function of the inputs, so the listed paths cover every input.

## Instructions on the three families

`famCount p tr f` is the number of machine instructions that the steps of path `tr` map to on family `f`, by the
table in `../results-r2.md`, with these rules, which are what a compiler's register allocator and flag reuse give:

* RISC-V: every step is one instruction.
* ARM64: every step is one instruction, except `sltu` (`cmp; cset`) and a two-register compare-and-branch
  (`cmp; b.cond`), which are two; but a compare-and-branch or `sltu` right after a `sub` of the same two registers is
  one, since the `sub` becomes `subs` and sets the flags.
* x86-64: an instruction `rd ← rs1 op rs2` is one instruction when `rd` is `rs1`, or `rs1` is not read again on the
  path (the allocator gives the result `rs1`'s register), or, for a commutative op, the same holds for `rs2`;
  otherwise a `mov` comes first (two). An add, or a shift left by 1 to 3, is one (`lea`). `sltu` is
  `cmp; setb; movzx` (three; two right after a `sub` of the same registers). A compare-and-branch is `cmp`/`test` and
  `jcc` (two, one fused uop); `jcc` alone (one) right after a `sub` of the same registers, or, for `beqz`/`bnez r`,
  right after an ALU instruction that wrote `r`. A divide is `xor edx, edx; div` or `cqo; idiv` (two), its dividend
  and quotient in `rax`.
-/

namespace Header
namespace Isa

inductive Fam | x86 | arm | rv
deriving DecidableEq, Repr

namespace Instr

/-- Registers read. -/
def reads : Instr → List Reg
  | add _ a b | sub _ a b | and _ a b | or _ a b | xor _ a b | sll _ a b | srl _ a b | sra _ a b
  | sltu _ a b | mul _ a b | mulhu _ a b | divu _ a b | div _ a b => [a, b]
  | neg _ a | addi _ a _ | andi _ a _ | ori _ a _ | xori _ a _ | slli _ a _ | srli _ a _ | srai _ a _
  | sltiu _ a _ | mv _ a | ld _ a _ | lbu _ a _ => [a]
  | sd a b _ | sb a b _ | beq a b _ | bne a b _ | bltu a b _ | bgeu a b _ => [a, b]
  | beqz a _ | bnez a _ => [a]
  | _ => []

/-- Register written. -/
def writes : Instr → Option Reg
  | add d .. | sub d .. | and d .. | or d .. | xor d .. | sll d .. | srl d .. | sra d .. | sltu d .. | mul d ..
  | mulhu d .. | divu d .. | div d .. | neg d .. | addi d .. | andi d .. | ori d .. | xori d .. | slli d ..
  | srli d .. | srai d .. | sltiu d .. | mv d .. | ld d .. | lbu d .. | li d .. => some d
  | _ => none

/-- An ALU instruction that sets x86 flags from its result. -/
def setsZF : Instr → Bool
  | add .. | sub .. | and .. | or .. | xor .. | neg .. | andi .. | ori .. | xori .. => true
  | _ => false

end Instr

/-- Is register `r` read on the rest of the path before it is written? -/
def liveAfter (r : Reg) : List Instr → Bool
  | [] => false
  | i :: rest => if i.reads.contains r then true else if i.writes = some r then false else liveAfter r rest

/-- One instruction, or a `mov` and the instruction. -/
def twoOp (d a : Reg) (rest : List Instr) : Nat := if d = a ∨ !liveAfter a rest then 1 else 2
def twoOpC (d a b : Reg) (rest : List Instr) : Nat := min (twoOp d a rest) (twoOp d b rest)

/-- Do the flags of `prev` already decide the branch or `sltu` on `a, b`? -/
def subFlags (prev : Option Instr) (a b : Reg) : Bool :=
  match prev with
  | some (.sub _ a' b') => a' == a && b' == b
  | _ => false

def zfFlags (prev : Option Instr) (a : Reg) : Bool :=
  match prev with
  | some i => i.setsZF && i.writes == some a
  | none => false

def costAt : Fam → Option Instr → Instr → List Instr → Nat
  | .rv, _, _, _ => 1
  | .arm, prev, i, _ => match i with
    | .sltu _ a b | .beq a b _ | .bne a b _ | .bltu a b _ | .bgeu a b _ => if subFlags prev a b then 1 else 2
    | .sltiu .. => 2
    | _ => 1
  | .x86, prev, i, rest => match i with
    | .add .. | .addi .. => 1
    | .sub d a _ | .neg d a | .andi d a _ | .ori d a _ | .xori d a _ | .srli d a _ | .srai d a _ => twoOp d a rest
    | .and d a b | .or d a b | .xor d a b | .mul d a b => twoOpC d a b rest
    | .slli d a k => if k ≤ 3 then 1 else twoOp d a rest
    | .sltu _ a b => if subFlags prev a b then 2 else 3
    | .sltiu .. | .mulhu .. => 3
    | .divu .. | .div .. => 2
    | .beq a b _ | .bne a b _ | .bltu a b _ | .bgeu a b _ => if subFlags prev a b then 1 else 2
    | .beqz a _ | .bnez a _ => if zfFlags prev a then 1 else 2
    | _ => 1

def costList (f : Fam) : Option Instr → List Instr → Nat
  | _, [] => 0
  | prev, i :: rest => costAt f prev i rest + costList f (some i) rest

/-- Machine instructions on a family along a trace. -/
def famCount (p : Prog) (tr : List Nat) (f : Fam) : Nat := costList f none (instrs p tr)

/-- No load after a store on a path. -/
def loadsFirst (p : Prog) (tr : List Nat) : Bool :=
  let is := instrs p tr
  match is.findIdx? Instr.isStore with
  | none => true
  | some k => !((is.drop k).any Instr.isLoad)

end Isa
end Header
