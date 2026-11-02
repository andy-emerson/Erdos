import Header.ProgA
/-!
# Round 2: the header form's add and subtract (A with sign-extended headers) as portable pseudocode

The "Header form" add of `../prior-art.md` (round 1: `LargerRow.lean`, `largerAddX`). Placement and registers as in
`ProgA.lean`: registers 1 to 6 hold the addresses of `x`'s header and word, `y`'s, and `z`'s; `h1` 10, `h2` 11, `m1` 12,
`m2` 13. The headers are loaded **sign-extended** (`lb`), so bit 7 fills bits 8–63; subtraction flips `h2` with
`xori h2, h2, −128` (the word `2^64 − 128`, an immediate on all three families), which keeps bits 8–63 equal to bit 7.

* `t = h1 xor h2`, `s = t sar 63`, `L = 0 − (m1 <u m2)`;
* complex only: the expression flag `e = (t shr 6) and (m1 ≠ 0) and (m2 ≠ 0)`, left in register 21 (computed fresh,
  as in the branch-free and multiplication rows); `m ≠ 0` is `0 <u m`;
* `r = m1 + (m2 xor s) − s`, `b = s and L`, magnitude `(r xor b) − b`;
* header `h1 xor (t and L)`, stored as a byte.

No branch and no jump. Integers: 12 compute steps; complex numbers: 17.
-/

namespace Header
namespace PA
open Isa

/-- The loads, sign-extending for the headers, and for subtraction `h2 xor −128`. -/
def preL (sub : Bool) : Prog :=
  [.lb 10 1 0, .lb 11 3 0, .ld 12 2 0, .ld 13 4 0] ++ (if sub then [.xori 11 11 (W - 128)] else [])

/-- The expression flag into register 21 (complex numbers only). -/
def lgFlag : Prog :=
  [.srli 20 14 6, .sltu 22 0 12, .sltu 23 0 13, .and 21 20 22, .and 21 21 23]

/-- The header form's add (`sub = false`) or subtract; `cx` adds the expression flag. -/
def lgRow (cx sub : Bool) : Prog :=
  preL sub ++
  [.xor 14 10 11,                               -- t
   .srai 15 14 63,                              -- s
   .sltu 17 12 13, .neg 17 17] ++               -- L
  (if cx then lgFlag else []) ++
  [.xor 16 13 15, .add 16 12 16, .sub 16 16 15, -- r = m1 + (m2 xor s) − s
   .and 18 15 17,                               -- b
   .xor 16 16 18, .sub 16 16 18,                -- magnitude
   .and 19 14 17, .xor 19 10 19,                -- header
   .sb 19 5 0, .sd 16 6 0]

end PA
end Header
