import Header.ProgLg
import Header.LargerRow
import Header.Split
/-!
# The header form's add and subtract are correct (round 2)

`lgRow_gen`: for every input (any header bytes, magnitudes below `2^64`), the program runs every instruction once, stores
the header byte and magnitude of round 1's formula (`A.largerAdd`, or `A.largerSub` for subtraction, on the header bytes
as loaded), and, in the complex program, leaves the formula's expression flag in register 21. The program is
straight-line, so one symbolic run covers every input; no case split is needed.

`lgCx_run` and `lgInt_run` then put this in terms of round 1's `A.add` through `larger_add_eq` / `larger_sub_eq`:
the stores are `A.add`'s (subtraction: `A.add W x (A.neg y)`, which is `A.sub`) with `LargerRow`'s one exception, two
zeros on different axes, where the header is `x`'s (both are zero); the flag is 1 exactly for an expression.
-/

set_option linter.unusedSimpArgs false

namespace Header
namespace PA
open Isa

/-! ## The machine's steps are the formula's -/

theorem wsxb_sx (v : Nat) : wsxb v = A.sx (v % 256) := rfl

theorem sx_lt (v : Nat) : A.sx (v % 256) < W := by
  unfold A.sx A.Wd W; split <;> omega

theorem wsra63 {x : Nat} (hx : x < W) : wsra x 63 = A.sar63 x := by
  unfold wsra ofInt toInt H A.sar63 A.Wd W at *
  simp only [Int.reducePow]
  split <;> omega

theorem sar63_lt (x : Nat) : A.sar63 x < W := by unfold A.sar63 W; split <;> decide

theorem ltMask_eq (a b : Nat) : wneg (wsltu a b) = A.ltMask a b := by
  unfold wsltu A.ltMask; split
  · rfl
  · rfl

theorem nz_eq (m : Nat) : wsltu 0 m = if m ≠ 0 then 1 else 0 := by
  unfold wsltu; by_cases h : m = 0 <;> simp [h] <;> omega

/-- The formula, written with the machine's steps in the program's order. -/
theorem lg_core (g1 g2 m1 m2 : Nat) (hg1 : g1 < W) (hg2 : g2 < W) :
    A.largerAddX g1 m1 g2 m2 =
      ((g1 ^^^ ((g1 ^^^ g2) &&& wneg (wsltu m1 m2))) % 256,
       wsub (wsub (wadd m1 (m2 ^^^ wsra (g1 ^^^ g2) 63)) (wsra (g1 ^^^ g2) 63) ^^^
           (wsra (g1 ^^^ g2) 63 &&& wneg (wsltu m1 m2)))
         (wsra (g1 ^^^ g2) 63 &&& wneg (wsltu m1 m2)),
       (g1 ^^^ g2) >>> 6 &&& wsltu 0 m1 &&& wsltu 0 m2) := by
  have ht : g1 ^^^ g2 < W := xor_lt hg1 hg2
  rw [wsra63 ht, ltMask_eq, nz_eq, nz_eq]
  have hs := sar63_lt (g1 ^^^ g2)
  have hb : A.sar63 (g1 ^^^ g2) &&& A.ltMask m1 m2 < W := Nat.lt_of_le_of_lt Nat.and_le_left hs
  have hr : wsub (wadd m1 (m2 ^^^ A.sar63 (g1 ^^^ g2))) (A.sar63 (g1 ^^^ g2)) =
      (m1 + (m2 ^^^ A.sar63 (g1 ^^^ g2)) + (A.Wd - A.sar63 (g1 ^^^ g2))) % A.Wd := by
    unfold wsub wadd A.Wd W at *; omega
  rw [hr]
  unfold A.largerAddX
  simp only [Prod.mk.injEq, true_and, and_true]
  unfold wsub A.Wd W at *; omega

/-! ## One run covers every input -/

/-- The formula's output for the header bytes as stored: add, or subtract. -/
def lgOut (sub : Bool) (h1 m1 h2 m2 : Nat) : Nat × Nat × Nat :=
  if sub then A.largerSub (h1 % 256) m1 (h2 % 256) m2 else A.largerAdd (h1 % 256) m1 (h2 % 256) m2

theorem lgOut_eq (sub : Bool) (h1 m1 h2 m2 : Nat) :
    lgOut sub h1 m1 h2 m2 = A.largerAddX (wsxb h1) m1 (if sub then wsxb h2 ^^^ (W - 128) else wsxb h2) m2 := by
  cases sub <;> rfl

/-- The statement of `lgRow_gen` for one program. -/
def LgGen (cx sub : Bool) : Prop :=
  ∀ (h1 h2 m1 m2 : Nat) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat),
    mem xh = h1 → mem yh = h2 → mem xm = m1 → mem ym = m2 →
    ∃ s, exe (lgRow cx sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
      s.trace = List.range (lgRow cx sub).length ∧
      s.stores = [(zh, (lgOut sub h1 m1 h2 m2).1), (zm, (lgOut sub h1 m1 h2 m2).2.1)] ∧
      (cx = true → s.r 21 = (lgOut sub h1 m1 h2 m2).2.2)

/-- Run the first `k` instructions (all of them) with `stepN`, read off the end state, and finish with `run_split`. -/
syntax "lg_tac" num : tactic
set_option hygiene false in
macro_rules
  | `(tactic| lg_tac $k) => `(tactic| (
    intro h1 h2 m1 m2 xh xm yh ym zh zm mem e1 e2 e3 e4
    have g1 : wsxb h1 < W := sx_lt h1
    have g2 : wsxb h2 < W := sx_lt h2
    have g2' : wsxb h2 ^^^ (W - 128) < W := xor_lt g2 (by unfold W; omega)
    first
      | rw [lgOut_eq, if_neg Bool.false_ne_true, lg_core _ _ _ _ g1 g2]
      | rw [lgOut_eq, if_pos rfl, lg_core _ _ _ _ g1 g2']
    cases hst : stepN (lgRow _ _) $k (start (regs6 xh xm yh ym zh zm) mem) with
    | none => simp [stepN, start, exec, St.set, St.get, St.store, lgRow, preL, lgFlag, regs6] at hst
    | some S =>
      have f := congrArg (Option.map fun s => (s.pc, s.trace, s.stores, s.r 21)) hst
      simp [stepN, start, exec, St.set, St.get, St.store, lgRow, preL, lgFlag, regs6, e1, e2, e3, e4] at f
      obtain ⟨fpc, ftr, fsto, f21⟩ := f
      rw [exe, show (lgRow _ _).length + 1 = 1 + $k from rfl, run_split _ $k 1 _ _ hst]
      refine ⟨S, ?_, ?_, ?_, ?_⟩
      · simp [run, ← fpc, lgRow, preL, lgFlag]
      · rw [← ftr]; rfl
      · rw [← fsto]
      · first | (intro hc; exact absurd hc Bool.false_ne_true) | (intro _; rw [← f21])))

theorem lgGen_int_add : LgGen false false := by lg_tac 18
theorem lgGen_int_sub : LgGen false true := by lg_tac 19
theorem lgGen_cx_add : LgGen true false := by lg_tac 23
theorem lgGen_cx_sub : LgGen true true := by lg_tac 24

/-- **The header form's program, every input.** It runs every instruction once and ends normally; it stores the
formula's header byte and magnitude; the complex program leaves the formula's expression flag in register 21. -/
theorem lgRow_gen (cx sub : Bool) : LgGen cx sub := by
  cases cx <;> cases sub
  exacts [lgGen_int_add, lgGen_int_sub, lgGen_cx_add, lgGen_cx_sub]

end PA
end Header
