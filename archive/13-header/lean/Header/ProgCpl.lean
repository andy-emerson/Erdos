import Header.Isa
/-!
# Round 3 (counts only): the complement layout's five operations as portable pseudocode

The build `cpl` (`../src/designcpl.nim`, `../results-cpl.md`; round 1: `Complement.lean`, `Cpl.cadd`, `Cpl.cneg`,
`Cpl.csub`, `Cpl.cmul`, `Cpl.cdiv`). A number is one 64-bit word: bit 63 the sign start, bit 62 the axis
(`G = 2^62`), bits 0–61 the count. One program per operation serves integers and complex numbers on the axes.

**Placement.** One list, as Control, B and C (`../results-r2.md` §1): registers 1, 2 and 3 hold the addresses of `x`,
`y` and the result `z`, each entry one word. **The two constants are in registers**: register 4 holds `G = 2^62` and
register 5 holds `NG = not G = 2^64 − 1 − 2^62`. Neither is an immediate on x86-64 (`imm32`, sign-extended) or RV64
(`imm12`); ARM64 encodes both as logical immediates. They are loop invariants, set once before the loop, as gcc does
(`movabs` before the loop in `../build/asm/cpl.s`), so the caller passes them like the addresses and the counts below
do not include them. `−1` (`not`) is an immediate on all three (`xori r, r, −1`: x86-64 `not`, ARM64 `mvn`, RV64
`not`).

Values: `w1` 10, `w2` 11. The add leaves its expression flag `e` in register 21, computed fresh (as the
branch-free, multiplication and header-form rows of A); the build ORs it into a sticky `err`, one more step.

No branch and no jump in any of the five.
-/

namespace Header
namespace PCpl
open Isa

/-- `G = 2^62`, the axis bit (register 4). -/
abbrev rG : Reg := 4
/-- `NG = not G` (register 5). -/
abbrev rNG : Reg := 5

/-- The all-ones immediate, `−1`. -/
abbrev ONES : Nat := 18446744073709551615

/-- The full add's body on `w1` (10) and `w2` (11): 11 steps for the result, 6 for the flag (the research's 5 with
`andn`; here `not Z1` is its own step). Result in 17, flag in 21. -/
def addBody : Prog :=
  [.xor 12 10 11,                              -- t = w1 xor w2
   .and 13 10 rNG,                             -- z1 = w1 and NG
   .or 14 11 rG, .add 14 13 14,                -- b = z1 + (w2 or G)   (the bridge)
   .sltiu 15 13 1, .neg 15 15,                 -- Z1 = 0 − (z1 = 0)
   .and 16 12 15, .xor 16 10 16,               -- src = w1 xor (t and Z1)
   .xor 17 14 16, .and 17 17 rG, .xor 17 14 17, -- r = b xor ((b xor src) and G)
   .and 18 11 rNG,                             -- z2 = w2 and NG
   .xori 19 15 ONES, .and 19 12 19,            -- t and not Z1
   .srli 19 19 62,                             -- shr 62
   .sltu 20 0 18,                              -- z2 ≠ 0
   .and 21 19 20]                              -- e

/-- The 4-step negate of register `s` into register `d` (`z` in `t`): `z = w and G; d = z − w; (d and NG) or z`. -/
def negBody (d s t : Reg) : Prog :=
  [.and t s rG, .sub d t s, .and d d rNG, .or d d t]

/-- Full add: 2 loads, 17 steps, 1 store (20). -/
def add : Prog := [.ld 10 1 0, .ld 11 2 0] ++ addBody ++ [.sd 17 3 0]

/-- Subtract: negate `w2` in place (4), then the full add (24). -/
def sub : Prog := [.ld 10 1 0, .ld 11 2 0] ++ negBody 11 11 22 ++ addBody ++ [.sd 17 3 0]

/-- Negate (6). -/
def neg : Prog := [.ld 10 1 0] ++ negBody 13 10 12 ++ [.sd 13 3 0]

/-- Sign extension of `s` into `d` (4): `s xor ((s xor (s shr 1)) and G)`. -/
def sext (d s : Reg) : Prog := [.srli d s 1, .xor d d s, .and d d rG, .xor d d s]

/-- The fix (4): `v xor ((v xor w1 xor w2) and G)`, from `v` (15) into 16. -/
def fix : Prog := [.xor 16 15 10, .xor 16 16 11, .and 16 16 rG, .xor 16 15 16]

/-- Multiply: sign-extend both (8), `m = ((w1 and w2) shl 1) sar 63` (3), `p = ((x1 xor m) − m)·x2` (3), the fix
(4): 18 steps, 21 with the loads and the store. -/
def mul : Prog :=
  [.ld 10 1 0, .ld 11 2 0] ++ sext 12 10 ++ sext 13 11 ++
  [.and 14 10 11, .slli 14 14 1, .srai 14 14 63,
   .xor 15 12 14, .sub 15 15 14, .mul 15 15 13] ++ fix ++ [.sd 16 3 0]

/-- Exact division: sign-extend both (8), `m = ((w2 and not w1) shl 1) sar 63` (4: no `andn`), `q = x1 ÷ₛ x2` (1),
`(q xor m) − m` (2), the fix (4): 19 steps, 22 with the loads and the store. -/
def div : Prog :=
  [.ld 10 1 0, .ld 11 2 0] ++ sext 12 10 ++ sext 13 11 ++
  [.xori 14 10 ONES, .and 14 11 14, .slli 14 14 1, .srai 14 14 63,
   .div 15 12 13, .xor 15 15 14, .sub 15 15 14] ++ fix ++ [.sd 16 3 0]

end PCpl
end Header
