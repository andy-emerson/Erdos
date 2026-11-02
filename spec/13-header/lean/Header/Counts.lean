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
* Folding, on x86-64 and ARM64: an `sltiu` or `andi` whose only use is the zero-test branch right after it becomes the
  branch's `cmp`/`test` (x86-64: `cmp r, k; jb` or `test r, k; jnz`; ARM64: `cmp; b.lo`, or `and` and `cbnz`); a
  shift left whose only use is the add right after it disappears into that add (x86-64 `lea`, scale 2, 4 or 8;
  ARM64 `add rd, rn, rm, lsl #k`).
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
  | sltiu _ a _ | mv _ a | ld _ a _ | lbu _ a _ | lb _ a _ => [a]
  | sd a b _ | sb a b _ | beq a b _ | bne a b _ | bltu a b _ | bgeu a b _ => [a, b]
  | beqz a _ | bnez a _ => [a]
  | _ => []

/-- Register written. -/
def writes : Instr → Option Reg
  | add d .. | sub d .. | and d .. | or d .. | xor d .. | sll d .. | srl d .. | sra d .. | sltu d .. | mul d ..
  | mulhu d .. | divu d .. | div d .. | neg d .. | addi d .. | andi d .. | ori d .. | xori d .. | slli d ..
  | srli d .. | srai d .. | sltiu d .. | mv d .. | ld d .. | lbu d .. | lb d .. | li d .. => some d
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

/-- Is the next instruction a zero-test branch on `d`, with `d` dead after it? -/
def testNext (d : Reg) : List Instr → Bool
  | (.beqz a _) :: rest | (.bnez a _) :: rest => a == d && !liveAfter d rest
  | _ => false

/-- Is the next instruction an add that reads `d`, with `d` dead after it? -/
def addNext (d : Reg) : List Instr → Bool
  | (.add d' a b) :: rest => (a == d || b == d) && (d' == d || !liveAfter d rest)
  | _ => false

/-- Was the previous instruction an `sltiu` or `andi` into `a` (now folded into the branch's `cmp`/`test`)? -/
def cmpPrev (prev : Option Instr) (a : Reg) : Bool :=
  match prev with
  | some (.sltiu d _ _) | some (.andi d _ _) => d == a
  | _ => false

def costAt : Fam → Option Instr → Instr → List Instr → Nat
  | .rv, _, _, _ => 1
  | .arm, prev, i, rest => match i with
    | .sltu _ a b | .beq a b _ | .bne a b _ | .bltu a b _ | .bgeu a b _ => if subFlags prev a b then 1 else 2
    | .sltiu d _ _ => if testNext d rest then 1 else 2
    | .slli d _ _ => if addNext d rest then 0 else 1
    | _ => 1
  | .x86, prev, i, rest => match i with
    | .add .. | .addi .. => 1
    | .andi d a _ => if testNext d rest then 1 else twoOp d a rest
    | .sub d a _ | .neg d a | .ori d a _ | .xori d a _ | .srli d a _ | .srai d a _ => twoOp d a rest
    | .and d a b | .or d a b | .xor d a b | .mul d a b => twoOpC d a b rest
    | .slli d a k => if k ≤ 3 && addNext d rest then 0 else if k ≤ 3 then 1 else twoOp d a rest
    | .sltu _ a b => if subFlags prev a b then 2 else 3
    | .sltiu d _ _ => if testNext d rest then 1 else 3
    | .mulhu .. => 3
    | .divu .. | .div .. => 2
    | .beq a b _ | .bne a b _ | .bltu a b _ | .bgeu a b _ => if subFlags prev a b then 1 else 2
    | .beqz a _ | .bnez a _ => if zfFlags prev a || cmpPrev prev a then 1 else 2
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

/-! ## The table: every path of every program

Each row: design, operation, path, `counts` (instructions, conditional branches, taken, jumps, loads, stores), machine
instructions on x86-64, ARM64 and RISC-V, and the branch log (each conditional branch on the path by index, with
`true` when taken). `table_eq` proves the table is what the programs give. -/

namespace Reg
open Isa

def table : List (String × String × String × Isa.Counts × Nat × Nat × Nat × List (Nat × Bool)) := [
  ("Control", "int add", "one", ⟨4, 0, 0, 0, 2, 1⟩, 4, 4, 4, []),
  ("Control", "int sub", "one", ⟨4, 0, 0, 0, 2, 1⟩, 4, 4, 4, []),
  ("Control", "int neg", "one", ⟨3, 0, 0, 0, 1, 1⟩, 3, 3, 3, []),
  ("Control", "int mul", "one", ⟨4, 0, 0, 0, 2, 1⟩, 4, 4, 4, []),
  ("Control", "int div", "one", ⟨4, 0, 0, 0, 2, 1⟩, 5, 4, 4, []),
  ("Control", "cx add", "one", ⟨8, 0, 0, 0, 4, 2⟩, 8, 8, 8, []),
  ("Control", "cx sub", "one", ⟨8, 0, 0, 0, 4, 2⟩, 8, 8, 8, []),
  ("Control", "cx neg", "one", ⟨6, 0, 0, 0, 2, 2⟩, 6, 6, 6, []),
  ("Control", "cx mul", "one", ⟨12, 0, 0, 0, 4, 2⟩, 14, 12, 12, []),
  ("Control", "cx div", "real divisor", ⟨10, 1, 0, 1, 4, 2⟩, 13, 10, 10, [(4, false)]),
  ("Control", "cx div", "imag divisor", ⟨10, 1, 1, 0, 4, 2⟩, 13, 10, 10, [(4, true)]),
  ("A", "int add (notes)", "same", ⟨11, 1, 1, 0, 4, 2⟩, 11, 11, 11, [(6, true)]),
  ("A", "int add (notes)", "opp, no borrow", ⟨12, 2, 1, 0, 4, 2⟩, 13, 12, 12, [(6, false), (8, true)]),
  ("A", "int add (notes)", "opp, borrow", ⟨15, 2, 0, 1, 4, 2⟩, 16, 15, 15, [(6, false), (8, false)]),
  ("A", "int sub (notes)", "same", ⟨12, 1, 1, 0, 4, 2⟩, 12, 12, 12, [(7, true)]),
  ("A", "int sub (notes)", "opp, no borrow", ⟨13, 2, 1, 0, 4, 2⟩, 14, 13, 13, [(7, false), (9, true)]),
  ("A", "int sub (notes)", "opp, borrow", ⟨16, 2, 0, 1, 4, 2⟩, 17, 16, 16, [(7, false), (9, false)]),
  ("A", "int add (perArm)", "same", ⟨11, 1, 1, 0, 4, 2⟩, 11, 11, 11, [(6, true)]),
  ("A", "int add (perArm)", "opp, no borrow", ⟨13, 2, 0, 1, 4, 2⟩, 14, 14, 13, [(6, false), (7, false)]),
  ("A", "int add (perArm)", "opp, borrow", ⟨14, 2, 1, 1, 4, 2⟩, 15, 15, 14, [(6, false), (7, true)]),
  ("A", "int sub (perArm)", "same", ⟨12, 1, 1, 0, 4, 2⟩, 12, 12, 12, [(7, true)]),
  ("A", "int sub (perArm)", "opp, no borrow", ⟨14, 2, 0, 1, 4, 2⟩, 15, 15, 14, [(7, false), (8, false)]),
  ("A", "int sub (perArm)", "opp, borrow", ⟨15, 2, 1, 1, 4, 2⟩, 16, 16, 15, [(7, false), (8, true)]),
  ("A", "cx add (notes)", "same", ⟨11, 1, 1, 0, 4, 2⟩, 11, 11, 11, [(6, true)]),
  ("A", "cx add (notes)", "opp, no borrow", ⟨14, 3, 1, 0, 4, 2⟩, 15, 14, 14, [(6, false), (8, false), (10, true)]),
  ("A", "cx add (notes)", "opp, borrow", ⟨17, 3, 0, 1, 4, 2⟩, 18, 17, 17, [(6, false), (8, false), (10, false)]),
  ("A", "cx add (notes)", "cross, x = 0", ⟨15, 3, 2, 0, 4, 2⟩, 16, 15, 15, [(6, false), (8, true), (14, true)]),
  ("A", "cx add (notes)", "cross, y = 0", ⟨15, 4, 2, 0, 4, 2⟩, 17, 15, 15, [(6, false), (8, true), (14, false), (15, true)]),
  ("A", "cx add (notes)", "cross, expression", ⟨12, 4, 1, 0, 4, 0⟩, 14, 12, 12, [(6, false), (8, true), (14, false), (15, false)]),
  ("A", "cx sub (notes)", "same", ⟨12, 1, 1, 0, 4, 2⟩, 12, 12, 12, [(7, true)]),
  ("A", "cx sub (notes)", "opp, no borrow", ⟨15, 3, 1, 0, 4, 2⟩, 16, 15, 15, [(7, false), (9, false), (11, true)]),
  ("A", "cx sub (notes)", "opp, borrow", ⟨18, 3, 0, 1, 4, 2⟩, 19, 18, 18, [(7, false), (9, false), (11, false)]),
  ("A", "cx sub (notes)", "cross, x = 0", ⟨16, 3, 2, 0, 4, 2⟩, 17, 16, 16, [(7, false), (9, true), (15, true)]),
  ("A", "cx sub (notes)", "cross, y = 0", ⟨16, 4, 2, 0, 4, 2⟩, 18, 16, 16, [(7, false), (9, true), (15, false), (16, true)]),
  ("A", "cx sub (notes)", "cross, expression", ⟨13, 4, 1, 0, 4, 0⟩, 15, 13, 13, [(7, false), (9, true), (15, false), (16, false)]),
  ("A", "cx add (perArm)", "same", ⟨11, 1, 1, 0, 4, 2⟩, 11, 11, 11, [(6, true)]),
  ("A", "cx add (perArm)", "opp, no borrow", ⟨15, 3, 0, 1, 4, 2⟩, 16, 16, 15, [(6, false), (8, false), (9, false)]),
  ("A", "cx add (perArm)", "opp, borrow", ⟨16, 3, 1, 1, 4, 2⟩, 17, 17, 16, [(6, false), (8, false), (9, true)]),
  ("A", "cx add (perArm)", "cross, x = 0", ⟨15, 3, 2, 0, 4, 2⟩, 16, 15, 15, [(6, false), (8, true), (15, true)]),
  ("A", "cx add (perArm)", "cross, y = 0", ⟨15, 4, 2, 0, 4, 2⟩, 17, 15, 15, [(6, false), (8, true), (15, false), (16, true)]),
  ("A", "cx add (perArm)", "cross, expression", ⟨12, 4, 1, 0, 4, 0⟩, 14, 12, 12, [(6, false), (8, true), (15, false), (16, false)]),
  ("A", "cx sub (perArm)", "same", ⟨12, 1, 1, 0, 4, 2⟩, 12, 12, 12, [(7, true)]),
  ("A", "cx sub (perArm)", "opp, no borrow", ⟨16, 3, 0, 1, 4, 2⟩, 17, 17, 16, [(7, false), (9, false), (10, false)]),
  ("A", "cx sub (perArm)", "opp, borrow", ⟨17, 3, 1, 1, 4, 2⟩, 18, 18, 17, [(7, false), (9, false), (10, true)]),
  ("A", "cx sub (perArm)", "cross, x = 0", ⟨16, 3, 2, 0, 4, 2⟩, 17, 16, 16, [(7, false), (9, true), (16, true)]),
  ("A", "cx sub (perArm)", "cross, y = 0", ⟨16, 4, 2, 0, 4, 2⟩, 18, 16, 16, [(7, false), (9, true), (16, false), (17, true)]),
  ("A", "cx sub (perArm)", "cross, expression", ⟨13, 4, 1, 0, 4, 0⟩, 15, 13, 13, [(7, false), (9, true), (16, false), (17, false)]),
  ("A", "neg", "one", ⟨5, 0, 0, 0, 2, 2⟩, 5, 5, 5, []),
  ("A", "mul", "one", ⟨9, 0, 0, 0, 4, 2⟩, 9, 9, 9, []),
  ("A", "div", "one", ⟨9, 0, 0, 0, 4, 2⟩, 10, 9, 9, []),
  ("A-bf", "int add", "one", ⟨23, 0, 0, 0, 4, 2⟩, 27, 22, 23, []),
  ("A-bf", "int sub", "one", ⟨24, 0, 0, 0, 4, 2⟩, 28, 23, 24, []),
  ("A-bf", "cx add", "one", ⟨35, 0, 0, 0, 4, 2⟩, 47, 37, 35, []),
  ("A-bf", "cx sub", "one", ⟨36, 0, 0, 0, 4, 2⟩, 48, 38, 36, []),
  ("A-mul", "add", "one", ⟨35, 0, 0, 0, 4, 2⟩, 45, 37, 35, []),
  ("A-mul", "sub", "one", ⟨36, 0, 0, 0, 4, 2⟩, 46, 38, 36, []),
  ("B", "int add", "one", ⟨8, 0, 0, 0, 4, 2⟩, 8, 8, 8, []),
  ("B", "int sub", "one", ⟨8, 0, 0, 0, 4, 2⟩, 8, 8, 8, []),
  ("B", "int neg", "one", ⟨4, 0, 0, 0, 2, 2⟩, 4, 4, 4, []),
  ("B", "int mul", "one", ⟨12, 0, 0, 0, 4, 2⟩, 14, 12, 12, []),
  ("B", "int collapse (notes)", "borrow 0", ⟨6, 1, 1, 0, 2, 2⟩, 7, 6, 6, [(3, true)]),
  ("B", "int collapse (notes)", "borrow 1", ⟨8, 1, 0, 1, 2, 2⟩, 9, 8, 8, [(3, false)]),
  ("B", "int collapse (perArm)", "borrow 0", ⟨7, 1, 0, 1, 2, 2⟩, 8, 8, 7, [(2, false)]),
  ("B", "int collapse (perArm)", "borrow 1", ⟨6, 1, 1, 0, 2, 2⟩, 7, 7, 6, [(2, true)]),
  ("B", "int div (notes)", "borrows x 0, y 0", ⟨18, 2, 2, 0, 4, 3⟩, 24, 19, 18, [(6, true), (10, true)]),
  ("B", "int div (notes)", "borrows x 0, y 1", ⟨19, 2, 1, 0, 4, 3⟩, 25, 20, 19, [(6, true), (10, false)]),
  ("B", "int div (notes)", "borrows x 1, y 0", ⟨19, 2, 1, 0, 4, 3⟩, 25, 20, 19, [(6, false), (10, true)]),
  ("B", "int div (notes)", "borrows x 1, y 1", ⟨20, 2, 0, 0, 4, 3⟩, 26, 21, 20, [(6, false), (10, false)]),
  ("B", "int div (perArm)", "borrows x 0, y 0", ⟨20, 2, 0, 2, 4, 3⟩, 26, 23, 20, [(5, false), (10, false)]),
  ("B", "int div (perArm)", "borrows x 0, y 1", ⟨19, 2, 1, 1, 4, 3⟩, 25, 22, 19, [(5, false), (10, true)]),
  ("B", "int div (perArm)", "borrows x 1, y 0", ⟨19, 2, 1, 1, 4, 3⟩, 25, 22, 19, [(5, true), (10, false)]),
  ("B", "int div (perArm)", "borrows x 1, y 1", ⟨18, 2, 2, 0, 4, 3⟩, 24, 21, 18, [(5, true), (10, true)]),
  ("B", "cx add", "one", ⟨16, 0, 0, 0, 8, 4⟩, 16, 16, 16, []),
  ("B", "cx sub", "one", ⟨16, 0, 0, 0, 8, 4⟩, 16, 16, 16, []),
  ("B", "cx neg", "one", ⟨8, 0, 0, 0, 4, 4⟩, 8, 8, 8, []),
  ("B", "cx mul", "one", ⟨40, 0, 0, 0, 8, 4⟩, 52, 40, 40, []),
  ("B", "cx collapse (notes)", "borrows re 0, im 0", ⟨12, 2, 2, 0, 4, 4⟩, 14, 12, 12, [(5, true), (13, true)]),
  ("B", "cx collapse (notes)", "borrows re 0, im 1", ⟨14, 2, 1, 1, 4, 4⟩, 16, 14, 14, [(5, true), (13, false)]),
  ("B", "cx collapse (notes)", "borrows re 1, im 0", ⟨14, 2, 1, 1, 4, 4⟩, 16, 14, 14, [(5, false), (13, true)]),
  ("B", "cx collapse (notes)", "borrows re 1, im 1", ⟨16, 2, 0, 2, 4, 4⟩, 18, 16, 16, [(5, false), (13, false)]),
  ("B", "cx collapse (perArm)", "borrows re 0, im 0", ⟨14, 2, 0, 2, 4, 4⟩, 16, 16, 14, [(4, false), (12, false)]),
  ("B", "cx collapse (perArm)", "borrows re 0, im 1", ⟨13, 2, 1, 1, 4, 4⟩, 15, 15, 13, [(4, false), (12, true)]),
  ("B", "cx collapse (perArm)", "borrows re 1, im 0", ⟨13, 2, 1, 1, 4, 4⟩, 15, 15, 13, [(4, true), (12, false)]),
  ("B", "cx collapse (perArm)", "borrows re 1, im 1", ⟨12, 2, 2, 0, 4, 4⟩, 14, 14, 12, [(4, true), (12, true)]),
  ("B", "cx div (notes)", "borrows x 00, y 00", ⟨40, 4, 4, 0, 8, 5⟩, 56, 45, 40, [(10, true), (14, true), (23, true), (27, true)]),
  ("B", "cx div (notes)", "borrows x 00, y 01", ⟨41, 4, 3, 0, 8, 5⟩, 57, 46, 41, [(10, true), (14, true), (23, true), (27, false)]),
  ("B", "cx div (notes)", "borrows x 00, y 10", ⟨41, 4, 3, 0, 8, 5⟩, 57, 46, 41, [(10, true), (14, true), (23, false), (27, true)]),
  ("B", "cx div (notes)", "borrows x 01, y 00", ⟨41, 4, 3, 0, 8, 5⟩, 57, 46, 41, [(10, true), (14, false), (23, true), (27, true)]),
  ("B", "cx div (notes)", "borrows x 01, y 01", ⟨42, 4, 2, 0, 8, 5⟩, 58, 47, 42, [(10, true), (14, false), (23, true), (27, false)]),
  ("B", "cx div (notes)", "borrows x 01, y 10", ⟨42, 4, 2, 0, 8, 5⟩, 58, 47, 42, [(10, true), (14, false), (23, false), (27, true)]),
  ("B", "cx div (notes)", "borrows x 10, y 00", ⟨41, 4, 3, 0, 8, 5⟩, 57, 46, 41, [(10, false), (14, true), (23, true), (27, true)]),
  ("B", "cx div (notes)", "borrows x 10, y 01", ⟨42, 4, 2, 0, 8, 5⟩, 58, 47, 42, [(10, false), (14, true), (23, true), (27, false)]),
  ("B", "cx div (notes)", "borrows x 10, y 10", ⟨42, 4, 2, 0, 8, 5⟩, 58, 47, 42, [(10, false), (14, true), (23, false), (27, true)]),
  ("B", "cx div (perArm)", "borrows x 00, y 00", ⟨44, 4, 0, 4, 8, 5⟩, 60, 53, 44, [(9, false), (14, false), (24, false), (29, false)]),
  ("B", "cx div (perArm)", "borrows x 00, y 01", ⟨43, 4, 1, 3, 8, 5⟩, 59, 52, 43, [(9, false), (14, false), (24, false), (29, true)]),
  ("B", "cx div (perArm)", "borrows x 00, y 10", ⟨43, 4, 1, 3, 8, 5⟩, 59, 52, 43, [(9, false), (14, false), (24, true), (29, false)]),
  ("B", "cx div (perArm)", "borrows x 01, y 00", ⟨43, 4, 1, 3, 8, 5⟩, 59, 52, 43, [(9, false), (14, true), (24, false), (29, false)]),
  ("B", "cx div (perArm)", "borrows x 01, y 01", ⟨42, 4, 2, 2, 8, 5⟩, 58, 51, 42, [(9, false), (14, true), (24, false), (29, true)]),
  ("B", "cx div (perArm)", "borrows x 01, y 10", ⟨42, 4, 2, 2, 8, 5⟩, 58, 51, 42, [(9, false), (14, true), (24, true), (29, false)]),
  ("B", "cx div (perArm)", "borrows x 10, y 00", ⟨43, 4, 1, 3, 8, 5⟩, 59, 52, 43, [(9, true), (14, false), (24, false), (29, false)]),
  ("B", "cx div (perArm)", "borrows x 10, y 01", ⟨42, 4, 2, 2, 8, 5⟩, 58, 51, 42, [(9, true), (14, false), (24, false), (29, true)]),
  ("B", "cx div (perArm)", "borrows x 10, y 10", ⟨42, 4, 2, 2, 8, 5⟩, 58, 51, 42, [(9, true), (14, false), (24, true), (29, false)]),
  ("C", "add", "same axis", ⟨8, 1, 1, 0, 2, 1⟩, 9, 8, 8, [(4, true)]),
  ("C", "add", "cross, y = 0", ⟨9, 2, 1, 0, 2, 1⟩, 10, 9, 9, [(4, false), (7, true)]),
  ("C", "add", "cross, x = 0", ⟨12, 3, 1, 0, 2, 1⟩, 13, 12, 12, [(4, false), (7, false), (10, true)]),
  ("C", "add", "cross, expression", ⟨12, 3, 0, 0, 2, 0⟩, 13, 12, 12, [(4, false), (7, false), (10, false)]),
  ("C", "sub", "same axis", ⟨8, 1, 1, 0, 2, 1⟩, 9, 8, 8, [(4, true)]),
  ("C", "sub", "cross, y = 0", ⟨9, 2, 1, 0, 2, 1⟩, 10, 9, 9, [(4, false), (7, true)]),
  ("C", "sub", "cross, x = 0", ⟨14, 3, 1, 0, 2, 1⟩, 16, 14, 14, [(4, false), (7, false), (12, true)]),
  ("C", "sub", "cross, expression", ⟨14, 3, 0, 0, 2, 0⟩, 16, 14, 14, [(4, false), (7, false), (12, false)]),
  ("C", "neg", "one", ⟨5, 0, 0, 0, 1, 1⟩, 6, 5, 5, []),
  ("C", "mul", "one", ⟨14, 0, 0, 0, 2, 1⟩, 17, 14, 14, []),
  ("C", "div", "one", ⟨14, 0, 0, 0, 2, 1⟩, 16, 13, 14, [])
]

set_option maxRecDepth 200000 in
/-- **The counts and the branch log of every path**, evaluated from the programs. -/
theorem table_eq : (registry.flatMap fun (d, op, p, paths) => paths.map fun (pn, tr) =>
    (d, op, pn, counts p tr, famCount p tr .x86, famCount p tr .arm, famCount p tr .rv,
      branchLog p (tr ++ [p.length]))) = table := by rfl

/-- Every program is forward: each branch and jump goes forward, to at most the end. -/
theorem all_forward : (registry.all fun (_, _, p, _) => Forward p) = true := by decide

/-- On every path, every load comes before every store. -/
theorem all_loads_first : (registry.all fun (_, _, p, paths) => paths.all fun (_, tr) => loadsFirst p tr) = true := by
  decide

/-- **The branch-free row has no conditional branch** (and no jump): every instruction of its four programs, and of
A's negate, multiply and divide, which it shares. -/
theorem bf_no_branch : ([PA.bfInt false, PA.bfInt true, PA.bfCx false, PA.bfCx true, PA.neg, PA.mul, PA.div].all
    fun p => p.all fun i => !i.isBranch && !i.isJump) = true := by decide

/-- **The multiplication row has no conditional branch** (and no jump). -/
theorem mulRow_no_branch : ([PA.mulRow false, PA.mulRow true].all
    fun p => p.all fun i => !i.isBranch && !i.isJump) = true := by decide

/-- C's negate, multiply and divide, and every program of Control and B but complex division, collapse and B's
division, have no conditional branch either. -/
theorem straight_line : ([Ctl.add, Ctl.sub, Ctl.neg, Ctl.mul, Ctl.div, Ctl.cadd, Ctl.csub, Ctl.cneg, Ctl.cmul,
    PB.add, PB.sub, PB.neg, PB.mul, PB.cadd, PB.csub, PB.cneg, PB.cmul, PC.neg, PC.mul, PC.div].all
    fun p => p.all fun i => !i.isBranch && !i.isJump) = true := by decide

/-- With the class code 0 (headers `64·d`), every `and D` of A is redundant: the direction check's XOR is already
clear in bits 0–5, and the result header, stored as a byte, has bits 0–5 clear without it. -/
theorem andD_redundant : ∀ d1 d2 : Fin 4,
    (PA.hdr d1 ^^^ PA.hdr d2) &&& 0xC0 = PA.hdr d1 ^^^ PA.hdr d2 ∧
    ((PA.hdr d1 + PA.hdr d2) &&& 0xC0) % 256 = (PA.hdr d1 + PA.hdr d2) % 256 ∧
    ((PA.hdr d1 + 128) &&& 0xC0) % 256 = (PA.hdr d1 + 128) % 256 ∧
    (PA.hdr d1 &&& 0xC0) % 256 = PA.hdr d1 % 256 ∧
    (Isa.wsub (PA.hdr d1) (PA.hdr d2) &&& 0xC0) % 256 = Isa.wsub (PA.hdr d1) (PA.hdr d2) % 256 := by
  decide

end Reg

end Header
