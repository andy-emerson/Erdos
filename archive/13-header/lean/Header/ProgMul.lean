import Header.ProgA
/-!
# Round 2: Design A's multiplication row as portable pseudocode

`aAddMul` / `aSubMul` of `../src/designa.nim` (round 1: `MulRow.lean`): A's add with the notes' exponent rule as code,
no branch. Placement and registers as in `ProgA.lean`: `h1` 10, `h2` 11, `m1` 12, `m2` 13; the error flag is left in
register 21 for the caller (computed fresh, not or-ed into an old flag), as in the branch-free row. Headers carry no
`and D`; the byte store takes the header mod 256.

* `t = h1 xor h2`, `ax = (t >> 6) and 1`, `o = (t >> 7) and (ax xor 1)`;
* `z1 = m1 <u 1`, `nz2 = 0 <u m2`, `err = (z1 xor 1) and ax and nz2`;
* `r = m1 + m2·(1 − 2o)`, `b = (m1 <u m2) and o`, magnitude `r·(1 − 2b)` (`1 − 2k` as `slli; neg; addi 1`);
* `k = ax and z1`, header `((h1 + k·(h2 − h1)) and 0xFF) + (b shl 7)`.
-/

namespace Header
namespace PA
open Isa

/-- The multiplication row, add (`sub = false`) or subtract. -/
def mulRow (sub : Bool) : Prog :=
  pre sub ++
  [.xor 14 10 11,                                -- t
   .srli 22 14 6, .andi 22 22 1,                 -- ax
   .srli 16 14 7, .xori 15 22 1, .and 16 16 15,  -- o
   .sltiu 23 12 1, .sltu 24 0 13,                -- z1, nz2
   .xori 21 23 1, .and 21 21 22, .and 21 21 24,  -- err
   .slli 17 16 1, .neg 17 17, .addi 17 17 1,     -- 1 − 2o
   .mul 17 13 17, .add 15 12 17,                 -- r = m1 + m2·(1 − 2o)
   .sltu 19 12 13, .and 19 19 16,                -- b
   .slli 20 19 1, .neg 20 20, .addi 20 20 1,     -- 1 − 2b
   .mul 15 15 20,                                -- magnitude
   .and 23 22 23,                                -- k = ax and z1
   .sub 24 11 10, .mul 24 23 24, .add 24 10 24,  -- h1 + k·(h2 − h1)
   .andi 24 24 0xFF, .slli 19 19 7, .add 24 24 19,
   .sb 24 5 0, .sd 15 6 0]

theorem hdr_0 : hdr 0 = 0 := by decide
theorem hdr_1 : hdr 1 = 64 := by decide
theorem hdr_2 : hdr 2 = 128 := by decide
theorem hdr_3 : hdr 3 = 192 := by decide

end PA
end Header
