import Header.Isa
import Header.FourSlot
/-!
# Round 2: Design B as portable pseudocode (spec §4)

**Placement**, the same rule as Control and A: one register per list per number, holding the address of that
number's entry. B has one list, so registers 1, 2, 3 hold the addresses of `x`, `y` and the result `z`. An integer
is two words, `P` at `+0` and `N` at `+8`; a complex number is four, slot `k` (along `i^k`) at `+8k`. Every program
loads all its operands before it stores anything.

Scratch: integers `P1 N1 P2 N2` in 10–13; complex slots of `x` in 10–13, of `y` in 20–23; results from 14 or 30.

Collapse and division are written in both forms of the borrow path (`Form`, as for A): the notes' (subtract, branch
on the borrow, negate) and per-arm (compare, branch, one subtraction in each arm).

Division routes the quotient by address, not by a branch: it stores 0 to every part, then the quotient to part
`(d1 − d2) and 1` (integers) or `and 3` (complex), at `z + 8·part`. So it stores one part twice.
-/

set_option linter.unusedVariables false

namespace Header
namespace PB
open Isa

/-- Registers 1, 2, 3 hold the addresses of `x`, `y`, `z`. -/
def regs3 (x y z : Nat) : Reg → Nat := fun i =>
  if i = 1 then x else if i = 2 then y else if i = 3 then z else 0

/-! ## Integers `(P, N)` -/

def ld4 : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 2 0, .ld 13 2 8]

def add : Prog := ld4 ++ [.add 14 10 12, .add 15 11 13, .sd 14 3 0, .sd 15 3 8]
/-- The crossed parts `(P1 + N2, N1 + P2)`. -/
def sub : Prog := ld4 ++ [.add 14 10 13, .add 15 11 12, .sd 14 3 0, .sd 15 3 8]
/-- Swap the parts. -/
def neg : Prog := [.ld 10 1 0, .ld 11 1 8, .sd 11 3 0, .sd 10 3 8]
/-- `(P1P2 + N1N2, P1N2 + N1P2)`. -/
def mul : Prog := ld4 ++ [.mul 14 10 12, .mul 15 11 13, .add 14 14 15,
  .mul 16 10 13, .mul 17 11 12, .add 16 16 17, .sd 14 3 0, .sd 16 3 8]

/-- Collapse: `|P − N|` to `P` when there is no borrow, else to `N`; the other part 0. -/
def collapse : Form → Prog
  | .notes => [.ld 10 1 0, .ld 11 1 8,
      .sub 14 10 11, .bgeu 10 11 8,              -- 2, 3
      .neg 14 14, .sd 0 3 0, .sd 14 3 8, .j 10,  -- 4..7: borrow
      .sd 14 3 0, .sd 0 3 8]                     -- 8, 9
  | .perArm => [.ld 10 1 0, .ld 11 1 8,
      .bltu 10 11 7,                             -- 2
      .sub 14 10 11, .sd 14 3 0, .sd 0 3 8, .j 10, -- 3..6
      .sub 14 11 10, .sd 0 3 0, .sd 14 3 8]      -- 7..9: borrow

/-- The path of a collapse: whether `P < N` (a borrow). -/
def collapseTrace : Form → Bool → List Nat
  | .notes, false => [0, 1, 2, 3, 8, 9]
  | .notes, true => [0, 1, 2, 3, 4, 5, 6, 7]
  | .perArm, false => [0, 1, 2, 3, 4, 5, 6]
  | .perArm, true => [0, 1, 2, 7, 8, 9]

/-- One operand's collapse inside division, from base index `i`: magnitude to `rm`, borrow (as a value) to `rb`. -/
def divColl (f : Form) (i p n rm rb : Nat) : Prog :=
  match f with
  | .notes => [.sub rm p n, .sltu rb p n, .bgeu p n (i + 4), .neg rm rm]
  | .perArm => [.sltu rb p n, .bltu p n (i + 4), .sub rm p n, .j (i + 5), .sub rm n p]

def divCollLen : Form → Nat
  | .notes => 4
  | .perArm => 5

def divCollTrace (f : Form) (i : Nat) (b : Bool) : List Nat :=
  match f, b with
  | .notes, false => [i, i + 1, i + 2]
  | .notes, true => [i, i + 1, i + 2, i + 3]
  | .perArm, false => [i, i + 1, i + 2, i + 3]
  | .perArm, true => [i, i + 1, i + 4]

/-- Exact division: collapse both, divide the magnitudes, A's direction rule; the quotient goes to part
`(b1 − b2) and 1`. -/
def div (f : Form) : Prog :=
  let c := divCollLen f
  ld4 ++ divColl f 4 10 11 14 16 ++ divColl f (4 + c) 12 13 15 17 ++
  [.divu 18 14 15, .sub 19 16 17, .andi 19 19 1, .slli 19 19 3, .add 19 19 3,
   .sd 0 3 0, .sd 0 3 8, .sd 18 19 0]

def divTrace (f : Form) (b1 b2 : Bool) : List Nat :=
  let c := divCollLen f
  [0, 1, 2, 3] ++ divCollTrace f 4 b1 ++ divCollTrace f (4 + c) b2 ++
  (List.range 8).map (· + (4 + 2 * c))

/-! ## Complex numbers: four slots -/

/-- Load `x`'s slots to 10–13 and `y`'s to 20–23. -/
def ld8 : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 1 16, .ld 13 1 24, .ld 20 2 0, .ld 21 2 8, .ld 22 2 16, .ld 23 2 24]

def k4 (k : Nat) : Nat := k % 4

def cadd : Prog := ld8 ++ (List.range 4).map (fun k => Instr.add (30 + k) (10 + k) (20 + k)) ++
  (List.range 4).map (fun k => Instr.sd (30 + k) 3 (8 * k))
/-- Slot `k` of `y` is added into slot `k + 2`. -/
def csub : Prog := ld8 ++ (List.range 4).map (fun k => Instr.add (30 + k) (10 + k) (20 + k4 (k + 2))) ++
  (List.range 4).map (fun k => Instr.sd (30 + k) 3 (8 * k))
/-- Turn every part by 2. -/
def cneg : Prog := [.ld 10 1 0, .ld 11 1 8, .ld 12 1 16, .ld 13 1 24] ++
  (List.range 4).map (fun k => Instr.sd (10 + k4 (k + 2)) 3 (8 * k))
/-- Sixteen products into four slots: slot `k` is `x0·y_k + x1·y_(k−1) + x2·y_(k−2) + x3·y_(k−3)`, stored as soon
as it is done. -/
def cmulSlot (k : Nat) : Prog :=
  [.mul 30 10 (20 + k4 (k + 4)), .mul 31 11 (20 + k4 (k + 3)), .add 30 30 31,
   .mul 31 12 (20 + k4 (k + 2)), .add 30 30 31, .mul 31 13 (20 + k4 (k + 1)), .add 30 30 31, .sd 30 3 (8 * k)]
def cmul : Prog := ld8 ++ cmulSlot 0 ++ cmulSlot 1 ++ cmulSlot 2 ++ cmulSlot 3

/-- One axis of a collapse, from base index `i`: slots `a` and `a + 2`. -/
def ccollAxis (f : Form) (i a : Nat) : Prog :=
  match f with
  | .notes => [.sub 14 (10 + a) (12 + a), .bgeu (10 + a) (12 + a) (i + 6),
      .neg 14 14, .sd 0 3 (8 * a), .sd 14 3 (8 * (a + 2)), .j (i + 8),
      .sd 14 3 (8 * a), .sd 0 3 (8 * (a + 2))]
  | .perArm => [.bltu (10 + a) (12 + a) (i + 5),
      .sub 14 (10 + a) (12 + a), .sd 14 3 (8 * a), .sd 0 3 (8 * (a + 2)), .j (i + 8),
      .sub 14 (12 + a) (10 + a), .sd 0 3 (8 * a), .sd 14 3 (8 * (a + 2))]

def cAxisTrace (f : Form) (i : Nat) (b : Bool) : List Nat :=
  match f, b with
  | .notes, false => [i, i + 1, i + 6, i + 7]
  | .notes, true => [i, i + 1, i + 2, i + 3, i + 4, i + 5]
  | .perArm, false => [i, i + 1, i + 2, i + 3, i + 4]
  | .perArm, true => [i, i + 5, i + 6, i + 7]

/-- Collapse each axis separately. -/
def ccollapse (f : Form) : Prog :=
  [.ld 10 1 0, .ld 11 1 8, .ld 12 1 16, .ld 13 1 24] ++ ccollAxis f 4 0 ++ ccollAxis f 12 1

def ccollapseTrace (f : Form) (br bi : Bool) : List Nat :=
  [0, 1, 2, 3] ++ cAxisTrace f 4 br ++ cAxisTrace f 12 bi

/-- One operand's collapse inside complex division, from base index `i`, for slots `s0..s3` in registers `s`..`s+3`:
the two axis magnitudes to `ra` and `ri`, their borrows to `ba` and `bi`; then the magnitude `ra or ri` to `ra`, and the
direction `2·(ba or bi) + (ri ≠ 0)` to `ba`. On an axis one of `ra`, `ri` is 0. -/
def cdivColl (f : Form) (i s ra ri ba bi : Nat) : Prog :=
  divColl f i s (s + 2) ra ba ++ divColl f (i + divCollLen f) (s + 1) (s + 3) ri bi ++
  [.or ra ra ri, .or ba ba bi, .sltu bi 0 ri, .slli ba ba 1, .or ba ba bi]

def cdivCollLen (f : Form) : Nat := 2 * divCollLen f + 5

def cdivCollTrace (f : Form) (i : Nat) (br bi : Bool) : List Nat :=
  divCollTrace f i br ++ divCollTrace f (i + divCollLen f) bi ++
  (List.range 5).map (· + (i + 2 * divCollLen f))

/-- Exact division of numbers on the axes: collapse both, divide the magnitudes, the quotient to slot
`(d1 − d2) and 3`. -/
def cdiv (f : Form) : Prog :=
  let c := cdivCollLen f
  ld8 ++ cdivColl f 8 10 14 15 16 17 ++ cdivColl f (8 + c) 20 24 25 26 27 ++
  [.divu 18 14 24, .sub 19 16 26, .andi 19 19 3, .slli 19 19 3, .add 19 19 3,
   .sd 0 3 0, .sd 0 3 8, .sd 0 3 16, .sd 0 3 24, .sd 18 19 0]

def cdivTrace (f : Form) (bxr bxi byr byi : Bool) : List Nat :=
  let c := cdivCollLen f
  List.range 8 ++ cdivCollTrace f 8 bxr bxi ++ cdivCollTrace f (8 + c) byr byi ++
  (List.range 10).map (· + (8 + 2 * c))

end PB
end Header
