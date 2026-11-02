import Header.Words
/-!
# Round 2: a small portable instruction set and its interpreter

Spec 13 v0.2 §6a and algorithm design rule 6 (portable pseudocode). Every step is an instruction that x86-64, ARM64
and RV64IM all provide in their common current form. The table in `../results-r2.md` maps each step to its
instructions on the three families; where a step is more than one instruction on a family (a compare-and-branch on
x86-64, `sltu` on x86-64 and ARM64, a divide on x86-64), the table says so.

Not in the set: a carry or borrow flag (a borrow is a value, from `sltu`, or the condition of a compare-and-branch), a
conditional move or select, and a 128-by-64-bit divide.

## The machine

* A register holds a word, a natural number below `W = 2^64`. Register 0 always reads 0 (RISC-V `x0`, ARM64
  `xzr`); writes to it are discarded. There are as many registers as a program names.
* Memory is a map from addresses to cells. A word load or store moves one 64-bit cell; a byte load or store moves one
  8-bit cell (`lbu` reads the cell mod 256, `sb` writes the value mod 256). The model does not split a word into its
  eight bytes, so it says nothing about a word and a byte at overlapping addresses; no program here has one.
* An address is a register plus a constant offset.
* A branch target is an instruction index. Every branch and jump in every program here goes forward, so a program
  runs at most once through each instruction.
* The program ends normally when the program counter reaches the end (one past the last instruction), and with an
  error when it executes `trap` (an exit to an error handler, counted as one instruction).

The interpreter records the index of every instruction it executes (`trace`) and every store as an
`(address, value)` pair (`stores`). Counts are read off the trace.
-/

set_option linter.unusedVariables false

namespace Header
namespace Isa

/-- The number of values of a word, `2^64`. -/
def W : Nat := 18446744073709551616

theorem W_eq : W = 2 ^ 64 := by decide

/-- Half a word, `2^63`: the sign bit. -/
def H : Nat := 9223372036854775808

abbrev Reg := Nat

/-! ## Word arithmetic -/

def wadd (a b : Nat) : Nat := (a + b) % W
def wsub (a b : Nat) : Nat := (a + (W - b % W)) % W
def wneg (a : Nat) : Nat := wsub 0 a
def wmul (a b : Nat) : Nat := (a * b) % W
def wmulhu (a b : Nat) : Nat := (a * b) / W
def wsltu (a b : Nat) : Nat := if a < b then 1 else 0

/-- A word read as a signed (two's complement) integer. -/
def toInt (a : Nat) : Int := if a < H then (a : Int) else (a : Int) - W

/-- A signed integer stored in a word: its residue mod `2^64`. -/
def ofInt (z : Int) : Nat := (z % (W : Int)).toNat

/-- Signed divide, truncating toward zero (`idiv`, `sdiv`, `div`). The machines differ on a zero divisor (x86-64
traps, ARM64 gives 0, RISC-V gives -1) and on `-2^63 / -1`; no program divides in either case. -/
def wdiv (a b : Nat) : Nat := ofInt (Int.tdiv (toInt a) (toInt b))

/-- Unsigned divide (`div` after zeroing `rdx`, `udiv`, `divu`). On a zero divisor the machines differ; this model
gives 0 (ARM64). No program divides by zero. -/
def wdivu (a b : Nat) : Nat := a / b

/-- Arithmetic shift right by `k` (`sar`, `asr`, `sra`). -/
def wsra (a k : Nat) : Nat := ofInt (toInt a / (2 ^ k : Int))

/-! ## Instructions -/

inductive Instr where
  /-- `rd ← rs1 + rs2` -/
  | add (rd rs1 rs2 : Reg)
  /-- `rd ← rs1 − rs2` (mod 2^64) -/
  | sub (rd rs1 rs2 : Reg)
  /-- `rd ← 0 − rs` -/
  | neg (rd rs : Reg)
  | and (rd rs1 rs2 : Reg)
  | or (rd rs1 rs2 : Reg)
  | xor (rd rs1 rs2 : Reg)
  | addi (rd rs : Reg) (k : Nat)
  | andi (rd rs : Reg) (k : Nat)
  | ori (rd rs : Reg) (k : Nat)
  | xori (rd rs : Reg) (k : Nat)
  /-- shifts by a constant `k < 64` -/
  | slli (rd rs : Reg) (k : Nat)
  | srli (rd rs : Reg) (k : Nat)
  | srai (rd rs : Reg) (k : Nat)
  /-- shifts by a register, amount mod 64 -/
  | sll (rd rs1 rs2 : Reg)
  | srl (rd rs1 rs2 : Reg)
  | sra (rd rs1 rs2 : Reg)
  /-- set-less-than-unsigned: `rd ← (rs1 < rs2) ? 1 : 0` -/
  | sltu (rd rs1 rs2 : Reg)
  /-- `rd ← (rs < k) ? 1 : 0` -/
  | sltiu (rd rs : Reg) (k : Nat)
  /-- `rd ← k` -/
  | li (rd : Reg) (k : Nat)
  /-- `rd ← rs` -/
  | mv (rd rs : Reg)
  /-- low 64 bits of the product -/
  | mul (rd rs1 rs2 : Reg)
  /-- high 64 bits of the unsigned product -/
  | mulhu (rd rs1 rs2 : Reg)
  /-- unsigned 64/64 divide -/
  | divu (rd rs1 rs2 : Reg)
  /-- signed 64/64 divide -/
  | div (rd rs1 rs2 : Reg)
  /-- 64-bit load `rd ← mem[rb + off]` -/
  | ld (rd rb : Reg) (off : Nat)
  /-- 8-bit load, zero-extended -/
  | lbu (rd rb : Reg) (off : Nat)
  /-- 64-bit store `mem[rb + off] ← rs` -/
  | sd (rs rb : Reg) (off : Nat)
  /-- 8-bit store of the low byte -/
  | sb (rs rb : Reg) (off : Nat)
  /-- compare-and-branch to instruction `t` -/
  | beq (rs1 rs2 : Reg) (t : Nat)
  | bne (rs1 rs2 : Reg) (t : Nat)
  | bltu (rs1 rs2 : Reg) (t : Nat)
  | bgeu (rs1 rs2 : Reg) (t : Nat)
  | beqz (rs : Reg) (t : Nat)
  | bnez (rs : Reg) (t : Nat)
  /-- unconditional jump -/
  | j (t : Nat)
  /-- exit to the error handler -/
  | trap
deriving DecidableEq, Repr

namespace Instr

/-- A conditional branch. -/
def isBranch : Instr → Bool
  | beq .. | bne .. | bltu .. | bgeu .. | beqz .. | bnez .. => true
  | _ => false

def isJump : Instr → Bool
  | j .. => true
  | _ => false

def isLoad : Instr → Bool
  | ld .. | lbu .. => true
  | _ => false

def isStore : Instr → Bool
  | sd .. | sb .. => true
  | _ => false

/-- Where control goes if the instruction branches or jumps. -/
def target? : Instr → Option Nat
  | beq _ _ t | bne _ _ t | bltu _ _ t | bgeu _ _ t | beqz _ t | bnez _ t | j t => some t
  | _ => none

end Instr

/-! ## State and one step -/

structure St where
  pc : Nat
  r : Reg → Nat
  mem : Nat → Nat
  /-- indices of the instructions executed, in order -/
  trace : List Nat
  /-- `(address, value)` of every store, in order -/
  stores : List (Nat × Nat)

/-- Read a register; register 0 reads 0. -/
def St.get (s : St) (i : Reg) : Nat := if i = 0 then 0 else s.r i

/-- Write a register and move to the next instruction. -/
def St.set (s : St) (i : Reg) (v : Nat) : St :=
  { s with r := fun k => if k = i then v else s.r k, pc := s.pc + 1 }

def St.next (s : St) : St := { s with pc := s.pc + 1 }

def St.store (s : St) (a v : Nat) : St :=
  { s with mem := fun k => if k = a then v else s.mem k, stores := s.stores ++ [(a, v)], pc := s.pc + 1 }

def St.goto (s : St) (c : Bool) (t : Nat) : St := { s with pc := if c then t else s.pc + 1 }

/-- Execute one (non-`trap`) instruction; the trace is extended by `run`. -/
def exec (s : St) : Instr → St
  | .add d a b => s.set d (wadd (s.get a) (s.get b))
  | .sub d a b => s.set d (wsub (s.get a) (s.get b))
  | .neg d a => s.set d (wneg (s.get a))
  | .and d a b => s.set d (s.get a &&& s.get b)
  | .or d a b => s.set d (s.get a ||| s.get b)
  | .xor d a b => s.set d (s.get a ^^^ s.get b)
  | .addi d a k => s.set d (wadd (s.get a) k)
  | .andi d a k => s.set d (s.get a &&& k)
  | .ori d a k => s.set d (s.get a ||| k)
  | .xori d a k => s.set d (s.get a ^^^ k)
  | .slli d a k => s.set d ((s.get a <<< k) % W)
  | .srli d a k => s.set d (s.get a >>> k)
  | .srai d a k => s.set d (wsra (s.get a) k)
  | .sll d a b => s.set d ((s.get a <<< (s.get b % 64)) % W)
  | .srl d a b => s.set d (s.get a >>> (s.get b % 64))
  | .sra d a b => s.set d (wsra (s.get a) (s.get b % 64))
  | .sltu d a b => s.set d (wsltu (s.get a) (s.get b))
  | .sltiu d a k => s.set d (wsltu (s.get a) k)
  | .li d k => s.set d (k % W)
  | .mv d a => s.set d (s.get a)
  | .mul d a b => s.set d (wmul (s.get a) (s.get b))
  | .mulhu d a b => s.set d (wmulhu (s.get a) (s.get b))
  | .divu d a b => s.set d (wdivu (s.get a) (s.get b))
  | .div d a b => s.set d (wdiv (s.get a) (s.get b))
  | .ld d b o => s.set d (s.mem (s.get b + o))
  | .lbu d b o => s.set d (s.mem (s.get b + o) % 256)
  | .sd a b o => s.store (s.get b + o) (s.get a)
  | .sb a b o => s.store (s.get b + o) (s.get a % 256)
  | .beq a b t => s.goto (s.get a = s.get b) t
  | .bne a b t => s.goto (s.get a ≠ s.get b) t
  | .bltu a b t => s.goto (s.get a < s.get b) t
  | .bgeu a b t => s.goto (s.get b ≤ s.get a) t
  | .beqz a t => s.goto (s.get a = 0) t
  | .bnez a t => s.goto (s.get a ≠ 0) t
  | .j t => s.goto true t
  | .trap => s.next

/-- How a run ends. -/
inductive Outcome where
  | done (s : St)
  | error (s : St)
  | stuck

abbrev Prog := List Instr

/-- Run with `fuel` steps at most. -/
def run (p : Prog) : Nat → St → Outcome
  | 0, _ => .stuck
  | fuel + 1, s =>
    if s.pc = p.length then .done s else
    match p[s.pc]? with
    | none => .stuck
    | some .trap => .error { s with trace := s.trace ++ [s.pc] }
    | some i => run p fuel (exec { s with trace := s.trace ++ [s.pc] } i)

/-- A program whose every branch and jump goes forward, to at most its end. -/
def Forward (p : Prog) : Bool :=
  (List.range p.length).all fun i => match p[i]? with
    | some ins => match ins.target? with
      | some t => decide (i < t ∧ t ≤ p.length)
      | none => true
    | none => true

/-- Start a program: registers given, pc 0, empty trace. -/
def start (r : Reg → Nat) (mem : Nat → Nat) : St := ⟨0, r, mem, [], []⟩

/-- Run a program from the start, with enough fuel for a forward program. -/
def exe (p : Prog) (r : Reg → Nat) (mem : Nat → Nat) : Outcome := run p (p.length + 1) (start r mem)

/-! ## Counts, read off a trace -/

/-- The instructions of a trace. -/
def instrs (p : Prog) (tr : List Nat) : List Instr := tr.filterMap fun i => p[i]?

/-- What a path costs: instructions, conditional branches, of them taken, jumps, loads, stores. -/
structure Counts where
  instrs : Nat
  branches : Nat
  taken : Nat
  jumps : Nat
  loads : Nat
  stores : Nat
deriving DecidableEq, Repr

/-- The conditional branches of a trace, each with whether it was taken (the next index is not the following one).
The trace is given with the end index appended. -/
def branchLog (p : Prog) : List Nat → List (Nat × Bool)
  | i :: k :: rest =>
    (match p[i]? with
     | some ins => if ins.isBranch then [(i, decide (k ≠ i + 1))] else []
     | none => []) ++ branchLog p (k :: rest)
  | _ => []

def counts (p : Prog) (tr : List Nat) : Counts :=
  let is := instrs p tr
  { instrs := is.length
    branches := (is.filter Instr.isBranch).length
    taken := ((branchLog p (tr ++ [p.length])).filter (·.2)).length
    jumps := (is.filter Instr.isJump).length
    loads := (is.filter Instr.isLoad).length
    stores := (is.filter Instr.isStore).length }


/-! ## Word lemmas -/

theorem ofInt_lt (z : Int) : ofInt z < W := by unfold ofInt W; omega

theorem ofInt_cast (z : Int) : ((ofInt z : Nat) : Int) = z % (W : Int) := by
  unfold ofInt W; omega

theorem wadd_ofInt (a b : Int) : wadd (ofInt a) (ofInt b) = ofInt (a + b) := by
  unfold wadd ofInt W; omega

theorem wsub_ofInt (a b : Int) : wsub (ofInt a) (ofInt b) = ofInt (a - b) := by
  unfold wsub ofInt W; omega

theorem wneg_ofInt (a : Int) : wneg (ofInt a) = ofInt (-a) := by
  unfold wneg wsub ofInt W; omega

theorem wmul_ofInt (a b : Int) : wmul (ofInt a) (ofInt b) = ofInt (a * b) := by
  have e : ((wmul (ofInt a) (ofInt b) : Nat) : Int) = ((ofInt a : Nat) : Int) * ((ofInt b : Nat) : Int) % (W : Int) := by
    simp only [wmul, Int.natCast_emod, Int.natCast_mul]
  rw [ofInt_cast, ofInt_cast, ← Int.mul_emod, ← ofInt_cast] at e
  exact Int.ofNat.inj e

/-- A signed integer that fits in a word: `-2^63 ≤ z < 2^63`. -/
def Fits (z : Int) : Prop := -(H : Int) ≤ z ∧ z < H

theorem toInt_ofInt {z : Int} (h : Fits z) : toInt (ofInt z) = z := by
  unfold toInt ofInt Fits H W at *; split <;> omega

theorem wdiv_ofInt {a b : Int} (ha : Fits a) (hb : Fits b) : wdiv (ofInt a) (ofInt b) = ofInt (a.tdiv b) := by
  simp [wdiv, toInt_ofInt ha, toInt_ofInt hb]

/-- The all-ones word, `W - 1`. -/
theorem ones_eq : (18446744073709551615 : Nat) = 2 ^ 64 - 1 := by decide

theorem xor_ones {x : Nat} (h : x < W) : x ^^^ 18446744073709551615 = 18446744073709551615 - x := by
  have := Words.xor_mask (n := 64) (r := x) (by rw [← W_eq]; exact h)
  simpa [Words.M] using this

theorem and_ones {x : Nat} (h : x < W) : x &&& 18446744073709551615 = x := by
  rw [ones_eq, Nat.and_two_pow_sub_one_eq_mod, Nat.mod_eq_of_lt (by rw [← W_eq]; exact h)]

theorem xor_lt {x y : Nat} (hx : x < W) (hy : y < W) : x ^^^ y < W := by
  rw [W_eq] at *; exact Nat.xor_lt_two_pow hx hy

theorem xor_cancel (s d : Nat) : s ^^^ (d ^^^ s) = d := by
  rw [Nat.xor_comm d s, ← Nat.xor_assoc, Nat.xor_self, Nat.zero_xor]

/-! ## Running a program symbolically

`run_prog [facts]` unfolds the interpreter step by step with `simp`, using the given program, register file and
memory facts. Every program here is forward, so this terminates after at most one pass. -/

syntax "run_prog" " [" Lean.Parser.Tactic.simpArg,* "]" : tactic
macro_rules
  | `(tactic| run_prog [$args,*]) =>
    `(tactic| simp [exe, run, start, exec, St.set, St.get, St.store, St.goto, St.next, $args,*])

end Isa
end Header
