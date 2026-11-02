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

/-- Run the program in three parts with `stepN` (the loads and `t`, `s`, `L`; the flag, empty for integers; the rest
and the stores), read off each part's end state, and join them with `run_split`. Cutting the run keeps the kernel's
check small. -/
syntax "lg_tac" term:max term:max num num num : tactic
set_option hygiene false in
macro_rules
  | `(tactic| lg_tac $cx $sb $k1 $k2 $k3) => `(tactic| (
    intro h1 h2 m1 m2 xh xm yh ym zh zm mem e1 e2 e3 e4
    have g1 : wsxb h1 < W := sx_lt h1
    have g2 : wsxb h2 < W := sx_lt h2
    have g2' : wsxb h2 ^^^ (W - 128) < W := xor_lt g2 (by unfold W; omega)
    first
      | rw [lgOut_eq, ite_cond_eq_false _ _ (by decide), lg_core _ _ _ _ g1 g2]
      | rw [lgOut_eq, ite_cond_eq_true _ _ (by decide), lg_core _ _ _ _ g1 g2']
    cases hs1 : stepN (lgRow $cx $sb) $k1 (start (regs6 xh xm yh ym zh zm) mem) with
    | none => simp [stepN, start, exec, St.set, St.get, lgRow, preL, lgFlag, regs6] at hs1
    | some S1 =>
    have f1 := congrArg (Option.map fun s => (s.pc, s.trace, s.stores, s.r 10, s.r 12, s.r 13, s.r 14, s.r 15,
      s.r 17, s.r 21, s.r 5, s.r 6)) hs1
    simp [stepN, start, exec, St.set, St.get, lgRow, preL, lgFlag, regs6, e1, e2, e3, e4] at f1
    obtain ⟨p1, t1, o1, a10, a12, a13, a14, a15, a17, a21, a5, a6⟩ := f1
    have o1' : S1.stores = [] := by first | exact o1 | exact o1.symm
    cases hs2 : stepN (lgRow $cx $sb) $k2 S1 with
    | none => simp [stepN, exec, St.set, St.get, lgRow, preL, lgFlag, ← p1, ← a12, ← a13, ← a14] at hs2
    | some S2 =>
    have f2 := congrArg (Option.map fun s => (s.pc, s.trace, s.stores, s.r 10, s.r 12, s.r 13, s.r 14, s.r 15,
      s.r 17, s.r 21, s.r 5, s.r 6)) hs2
    simp [stepN, exec, St.set, St.get, lgRow, preL, lgFlag, ← p1, ← t1, o1', ← a10, ← a12, ← a13, ← a14, ← a15,
      ← a17, ← a21, ← a5, ← a6] at f2
    obtain ⟨p2, t2, o2, b10, b12, b13, b14, b15, b17, b21, b5, b6⟩ := f2
    have o2' : S2.stores = [] := by first | exact o2 | exact o2.symm | exact o2.symm.trans o1'
    cases hs3 : stepN (lgRow $cx $sb) $k3 S2 with
    | none => simp [stepN, exec, St.set, St.get, St.store, lgRow, preL, lgFlag, ← p2] at hs3
    | some S3 =>
    have f3 := congrArg (Option.map fun s => (s.pc, s.trace, s.stores, s.r 21)) hs3
    simp [stepN, exec, St.set, St.get, St.store, lgRow, preL, lgFlag, ← p2, ← t2, o2', ← b10, ← b12, ← b13,
      ← b14, ← b15, ← b17, ← b21, ← b5, ← b6] at f3
    obtain ⟨p3, t3, o3, c21⟩ := f3
    rw [exe, show (lgRow $cx $sb).length + 1 = ((1 + $k3) + $k2) + $k1 from rfl, run_split _ $k1 _ _ _ hs1,
      run_split _ $k2 _ _ _ hs2, run_split _ $k3 _ _ _ hs3]
    refine ⟨S3, ?_, ?_, ?_, ?_⟩
    · simp [run, ← p3, lgRow, preL, lgFlag]
    · rw [← t3]; rfl
    · rw [← o3]
    · first | (intro hc; exact absurd hc Bool.false_ne_true) | (intro _; rw [← c21])))

set_option maxHeartbeats 1000000

theorem lgGen_int_add : LgGen false false := by lg_tac false false 8 0 10
theorem lgGen_int_sub : LgGen false true := by lg_tac false true 9 0 10
theorem lgGen_cx_add : LgGen true false := by lg_tac true false 8 5 10
theorem lgGen_cx_sub : LgGen true true := by lg_tac true true 9 5 10

/-- **The header form's program, every input.** It runs every instruction once and ends normally; it stores the
formula's header byte and magnitude; the complex program leaves the formula's expression flag in register 21. -/
theorem lgRow_gen (cx sub : Bool) : LgGen cx sub := by
  cases cx <;> cases sub
  exacts [lgGen_int_add, lgGen_int_sub, lgGen_cx_add, lgGen_cx_sub]

/-! ## In terms of round 1's `A.add` -/

theorem hdr_mod (d : Fin 4) : hdr d % 256 = A.hb d := by revert d; decide

/-- When round 1 gives a number, the formula gives its header (with `LargerRow`'s exception) and magnitude, and flag 0. -/
theorem lgOut_some (sub : Bool) (d1 d2 : Fin 4) (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (r : A.Num)
    (h : A.add W ⟨m1, d1⟩ (yEff sub m2 d2) = some r) :
    lgOut sub (hdr d1) m1 (hdr d2) m2 = (if A.diffAxes d1 d2 ∧ m1 = 0 ∧ m2 = 0 then hdr d1 else hdr r.d, r.m, 0) := by
  have hm1' : m1 < 2 ^ 64 := by rw [← W_eq]; exact hm1
  have hm2' : m2 < 2 ^ 64 := by rw [← W_eq]; exact hm2
  cases sub
  · have L := A.larger_add_eq ⟨m1, d1⟩ ⟨m2, d2⟩ hm1' hm2'
    simp only [yEff, Bool.false_eq_true, ite_false] at h
    rw [← W_eq, h] at L
    simp only [lgOut, Bool.false_eq_true, ite_false, hdr_mod]
    exact L
  · have L := A.larger_sub_eq ⟨m1, d1⟩ ⟨m2, d2⟩ hm1' hm2'
    simp only [yEff, ite_true] at h
    simp only [A.sub] at L
    rw [← W_eq, h] at L
    simp only [lgOut, ite_true, hdr_mod]
    exact L

/-- When round 1 gives an expression, the formula's flag is 1. -/
theorem lgOut_none (sub : Bool) (d1 d2 : Fin 4) (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W)
    (h : A.add W ⟨m1, d1⟩ (yEff sub m2 d2) = none) :
    (lgOut sub (hdr d1) m1 (hdr d2) m2).2.2 = 1 := by
  have hm1' : m1 < 2 ^ 64 := by rw [← W_eq]; exact hm1
  have hm2' : m2 < 2 ^ 64 := by rw [← W_eq]; exact hm2
  cases sub
  · have L := A.larger_add_eq ⟨m1, d1⟩ ⟨m2, d2⟩ hm1' hm2'
    simp only [yEff, Bool.false_eq_true, ite_false] at h
    rw [← W_eq, h] at L
    simp only [lgOut, Bool.false_eq_true, ite_false, hdr_mod]
    exact L
  · have L := A.larger_sub_eq ⟨m1, d1⟩ ⟨m2, d2⟩ hm1' hm2'
    simp only [yEff, ite_true] at h
    simp only [A.sub] at L
    rw [← W_eq, h] at L
    simp only [lgOut, ite_true, hdr_mod]
    exact L

/-- **The header form, complex numbers (add and subtract).** One path, every instruction once. Register 21 ends as
the expression flag: 1 exactly when round 1 gives "an expression"; otherwise 0, and the stores are round 1's `A.add`
(subtract: `A.add W x (A.neg y)`, which is `A.sub`), except that for two zeros on different axes the stored header is
`x`'s (`LargerRow.larger_add_eq`; both are zero). -/
theorem lgCx_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e3 : mem xm = m1) (e4 : mem ym = m2) (sub : Bool) (d1 d2 : Fin 4) (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    match A.add W ⟨m1, d1⟩ (yEff sub m2 d2) with
    | some r => ∃ s, exe (lgRow true sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (lgRow true sub).length ∧ s.r 21 = 0 ∧
        s.stores = [(zh, if A.diffAxes d1 d2 ∧ m1 = 0 ∧ m2 = 0 then hdr d1 else hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (lgRow true sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (lgRow true sub).length ∧ s.r 21 = 1 := by
  obtain ⟨s, hx, ht, hst, h21⟩ := lgRow_gen true sub (hdr d1) (hdr d2) m1 m2 xh xm yh ym zh zm mem e1 e2 e3 e4
  split
  · next r h =>
    rw [lgOut_some sub d1 d2 m1 m2 hm1 hm2 r h] at hst h21
    exact ⟨s, hx, ht, h21 rfl, hst⟩
  · next h =>
    exact ⟨s, hx, ht, (h21 rfl).trans (lgOut_none sub d1 d2 m1 m2 hm1 hm2 h)⟩

/-- **The header form, integers (add and subtract).** For integer directions, one path, every instruction once; the
stores are round 1's `A.add` (subtract: `A.sub`), with no exception (integers have one axis). -/
theorem lgInt_run (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat)
    (e3 : mem xm = m1) (e4 : mem ym = m2) (sub : Bool) (d1 d2 : Fin 4) (hd1 : d1 = 0 ∨ d1 = 2) (hd2 : d2 = 0 ∨ d2 = 2)
    (e1 : mem xh = hdr d1) (e2 : mem yh = hdr d2) :
    ∃ r, A.add W ⟨m1, d1⟩ (yEff sub m2 d2) = some r ∧
      ∃ s, exe (lgRow false sub) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (lgRow false sub).length ∧ s.stores = [(zh, hdr r.d), (zm, r.m)] := by
  obtain ⟨s, hx, ht, hst, -⟩ := lgRow_gen false sub (hdr d1) (hdr d2) m1 m2 xh xm yh ym zh zm mem e1 e2 e3 e4
  have hnd : ¬ A.diffAxes d1 d2 := by
    rcases hd1 with rfl|rfl <;> rcases hd2 with rfl|rfl <;> decide
  cases h : A.add W ⟨m1, d1⟩ (yEff sub m2 d2) with
  | none =>
    exfalso
    rcases hd1 with rfl|rfl <;> rcases hd2 with rfl|rfl <;> cases sub <;>
      simp [A.add, A.dirCheck, yEff, A.neg] at h
  | some r =>
    refine ⟨r, rfl, s, hx, ht, ?_⟩
    rw [hst, lgOut_some sub d1 d2 m1 m2 hm1 hm2 r h]
    simp [hnd]

end PA
end Header
