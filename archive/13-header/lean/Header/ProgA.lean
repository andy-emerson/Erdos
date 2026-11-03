import Header.Isa
import Header.DesignA
/-!
# Round 2: Design A as portable pseudocode (spec §3, the notes' tree)

A number is an 8-bit header and a word, in two parallel lists. **Placement:** one register per list per number,
holding the address of that number's entry (the same rule as Control and B): registers 1 and 2 hold the addresses of
`x`'s header and word, 3 and 4 those of `y`, 5 and 6 those of the result `z`. Every program loads all its operands
before it stores anything.

A header is `64·d` for a direction `d : Fin 4` (bits 6–7; bits 0–5, the class code, are 0). `D = 0xC0`.

Scratch registers: 10 `h1`, 11 `h2`, 12 `m1`, 13 `m2`, 14 `t`, 15 the result word, 16 and up temporaries.

**Two forms of the opposite-direction path** (coordinator, Nov 1): the notes' form subtracts, branches on the borrow
and negates on a borrow (`.notes`); the form gcc emits compares, branches, and subtracts once in each arm
(`m1 − m2` or `m2 − m1`), with no negation (`.perArm`). Both are written and proved.

The integer programs have only the same/opposite branch (integers give `t ∈ {0, 0x80}`); the complex programs have
the notes' whole tree. Subtraction is the add program with `h2 xor 0x80` after the loads (`sub = true`).
-/

set_option linter.unusedVariables false
set_option maxHeartbeats 4000000

namespace Header
namespace PA
open Isa

/-- Registers 1–6: addresses of `x`'s header and word, `y`'s, and `z`'s. -/
def regs6 (xh xm yh ym zh zm : Nat) : Reg → Nat := fun i =>
  if i = 1 then xh else if i = 2 then xm else if i = 3 then yh else if i = 4 then ym
  else if i = 5 then zh else if i = 6 then zm else 0

/-- The header of direction `d`. -/
def hdr (d : Fin 4) : Nat := 64 * d.val

/-- The loads, and for subtraction `h2 xor 0x80`. -/
def pre (sub : Bool) : Prog :=
  [.lbu 10 1 0, .lbu 11 3 0, .ld 12 2 0, .ld 13 4 0] ++ (if sub then [.xori 11 11 0x80] else [])

/-- The `y` an add sees: `y` itself, or for subtraction its negation. -/
def yEff (sub : Bool) (m2 : Nat) (d2 : Fin 4) : A.Num := if sub then A.neg ⟨m2, d2⟩ else ⟨m2, d2⟩

def kk (sub : Bool) : Nat := if sub then 1 else 0

/-! ## Add and subtract, integers -/

/-- Integer add (`sub = false`) or subtract, in either form. -/
def intAdd (f : Form) (sub : Bool) : Prog :=
  let k := kk sub
  pre sub ++ match f with
  | .notes =>
    [.xor 14 10 11, .andi 14 14 0xC0,
     .beqz 14 (k + 12),                       -- k+6: same direction?
     .sub 15 12 13, .bgeu 12 13 (k + 13),     -- k+7, k+8: subtract; no borrow → store
     .neg 15 15, .mv 10 11, .j (k + 13),      -- k+9..k+11: borrow: negate, second direction
     .add 15 12 13,                           -- k+12: same: add
     .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]  -- k+13..k+15: store
  | .perArm =>
    [.xor 14 10 11, .andi 14 14 0xC0,
     .beqz 14 (k + 13),                       -- k+6
     .bltu 12 13 (k + 10),                    -- k+7: m1 < m2?
     .sub 15 12 13, .j (k + 14),              -- k+8, k+9: m1 − m2
     .sub 15 13 12, .mv 10 11, .j (k + 14),   -- k+10..k+12: m2 − m1, second direction
     .add 15 12 13,                           -- k+13
     .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]

/-- The paths of an add. -/
inductive Path | same | oppNB | oppB | z1 | z2 | expr
deriving DecidableEq, Repr

/-- Which path the inputs take: `d2` is the direction the add sees (for subtraction, `y`'s turned by 2). -/
def path (d1 d2 : Fin 4) (m1 m2 : Nat) : Path :=
  if d1 = d2 then .same
  else if d2 = d1 + 2 then (if m2 ≤ m1 then .oppNB else .oppB)
  else if m1 = 0 then .z1 else if m2 = 0 then .z2 else .expr

def preT (sub : Bool) : List Nat := if sub then [0, 1, 2, 3, 4] else [0, 1, 2, 3]

def intTrace (f : Form) (sub : Bool) (p : Path) : List Nat :=
  preT sub ++ (List.map (· + kk sub) <| match f, p with
  | .notes, .same => [4, 5, 6, 12, 13, 14, 15]
  | .notes, .oppNB => [4, 5, 6, 7, 8, 13, 14, 15]
  | .notes, _ => [4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15]
  | .perArm, .same => [4, 5, 6, 13, 14, 15, 16]
  | .perArm, .oppNB => [4, 5, 6, 7, 8, 9, 14, 15, 16]
  | .perArm, _ => [4, 5, 6, 7, 10, 11, 12, 14, 15, 16])

/-! ## Add and subtract, complex numbers on the axes (the notes' whole tree) -/

def cxAdd (f : Form) (sub : Bool) : Prog :=
  let k := kk sub
  pre sub ++ match f with
  | .notes =>
    [.xor 14 10 11, .andi 14 14 0xC0,
     .beqz 14 (k + 18),                       -- k+6: same → add
     .andi 16 14 0x40, .bnez 16 (k + 14),     -- k+7, k+8: different axes?
     .sub 15 12 13, .bgeu 12 13 (k + 19),     -- k+9, k+10: opposite
     .neg 15 15, .mv 10 11, .j (k + 19),      -- k+11..k+13
     .beqz 12 (k + 17),                       -- k+14: m1 = 0?
     .beqz 13 (k + 18),                       -- k+15: m2 = 0?
     .trap,                                   -- k+16: an expression
     .mv 10 11,                               -- k+17: the zero takes the other's direction
     .add 15 12 13,                           -- k+18: same
     .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]  -- k+19..k+21
  | .perArm =>
    [.xor 14 10 11, .andi 14 14 0xC0,
     .beqz 14 (k + 19),                       -- k+6
     .andi 16 14 0x40, .bnez 16 (k + 15),     -- k+7, k+8
     .bltu 12 13 (k + 12),                    -- k+9
     .sub 15 12 13, .j (k + 20),              -- k+10, k+11
     .sub 15 13 12, .mv 10 11, .j (k + 20),   -- k+12..k+14
     .beqz 12 (k + 18),                       -- k+15
     .beqz 13 (k + 19),                       -- k+16
     .trap,                                   -- k+17
     .mv 10 11,                               -- k+18
     .add 15 12 13,                           -- k+19
     .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]  -- k+20..k+22

def cxTrace (f : Form) (sub : Bool) (p : Path) : List Nat :=
  preT sub ++ (List.map (· + kk sub) <| match f, p with
  | .notes, .same => [4, 5, 6, 18, 19, 20, 21]
  | .notes, .oppNB => [4, 5, 6, 7, 8, 9, 10, 19, 20, 21]
  | .notes, .oppB => [4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 19, 20, 21]
  | .notes, .z1 => [4, 5, 6, 7, 8, 14, 17, 18, 19, 20, 21]
  | .notes, .z2 => [4, 5, 6, 7, 8, 14, 15, 18, 19, 20, 21]
  | .notes, .expr => [4, 5, 6, 7, 8, 14, 15, 16]
  | .perArm, .same => [4, 5, 6, 19, 20, 21, 22]
  | .perArm, .oppNB => [4, 5, 6, 7, 8, 9, 10, 11, 20, 21, 22]
  | .perArm, .oppB => [4, 5, 6, 7, 8, 9, 12, 13, 14, 20, 21, 22]
  | .perArm, .z1 => [4, 5, 6, 7, 8, 15, 18, 19, 20, 21, 22]
  | .perArm, .z2 => [4, 5, 6, 7, 8, 15, 16, 19, 20, 21, 22]
  | .perArm, .expr => [4, 5, 6, 7, 8, 15, 16, 17])

/-! ## Negate, multiply, divide (one path each; the same for integers and complex numbers) -/

/-- Negate: `h xor 0x80`, the word unchanged. -/
def neg : Prog := [.lbu 10 1 0, .ld 12 2 0, .xori 10 10 0x80, .sb 10 5 0, .sd 12 6 0]
/-- Multiply: `m1·m2`, direction `(h1 + h2) and D`. -/
def mul : Prog := [.lbu 10 1 0, .lbu 11 3 0, .ld 12 2 0, .ld 13 4 0,
  .mul 15 12 13, .add 10 10 11, .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]
/-- Exact division: `m1 / m2`, direction `(h1 − h2) and D`. -/
def div : Prog := [.lbu 10 1 0, .lbu 11 3 0, .ld 12 2 0, .ld 13 4 0,
  .divu 15 12 13, .sub 10 10 11, .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0]

/-! ## The branch-free row (spec §7): masks `0 − b`, AND, XOR; no conditional branch, no conditional move -/

/-- Integers. `o = t >> 7` (1 when opposite), `mo = 0 − o`; `s = m1 + m2`, `d = m1 − m2`, `b = (m1 < m2) and o`,
`mb = 0 − b`; `d' = (d xor mb) − mb` (negated on a borrow); result `s xor ((s xor d') and mo)`; direction
`(h1 + (b shl 7)) and D`. -/
def bfInt (sub : Bool) : Prog :=
  pre sub ++
  [.xor 14 10 11, .andi 14 14 0xC0,
   .srli 16 14 7, .neg 17 16,
   .add 15 12 13, .sub 18 12 13,
   .sltu 19 12 13, .and 19 19 16, .neg 20 19,
   .xor 18 18 20, .sub 18 18 20,
   .xor 18 18 15, .and 18 18 17, .xor 15 15 18,
   .slli 19 19 7, .add 10 10 19, .andi 10 10 0xC0,
   .sb 10 5 0, .sd 15 6 0]

/-- Complex numbers. Besides the integer row: `o = (t xor 0x80) <u 1` (opposite), `a = (t >> 6) and 1` (different
axes), `z1 = m1 <u 1`, `z2 = m2 <u 1`; `e = (z1 or z2) xor 1, and a` is the error flag (an expression), left in
register 21 for the caller; when `m1 = 0` across axes, `h1 xor (t and (0 − (z1 and a)))` is `h2`'s direction. -/
def bfCx (sub : Bool) : Prog :=
  pre sub ++
  [.xor 14 10 11, .andi 14 14 0xC0,
   .xori 16 14 0x80, .sltiu 16 16 1,
   .srli 22 14 6, .andi 22 22 1,
   .sltiu 23 12 1, .sltiu 24 13 1,
   .or 21 23 24, .xori 21 21 1, .and 21 21 22,
   .and 23 23 22, .neg 23 23, .and 23 23 14, .xor 10 10 23,
   .neg 17 16,
   .add 15 12 13, .sub 18 12 13,
   .sltu 19 12 13, .and 19 19 16, .neg 20 19,
   .xor 18 18 20, .sub 18 18 20,
   .xor 18 18 15, .and 18 18 17, .xor 15 15 18,
   .slli 19 19 7, .add 10 10 19, .andi 10 10 0xC0,
   .sb 10 5 0, .sd 15 6 0]

end PA
end Header
