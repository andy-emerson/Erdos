import Header.Isa
import Header.DesignA
open Header Header.Isa

def aAddInt : Prog := [
  .lbu 10 1 0, .lbu 11 3 0, .ld 12 2 0, .ld 13 4 0,
  .xor 14 10 11, .andi 14 14 0xC0,
  .beqz 14 12,
  .sub 15 12 13, .bgeu 12 13 13, .neg 15 15, .mv 10 11, .j 13,
  .add 15 12 13,
  .andi 10 10 0xC0, .sb 10 5 0, .sd 15 6 0 ]

def regsA (xh xm yh ym zh zm : Nat) : Reg → Nat := fun i =>
  if i = 1 then xh else if i = 2 then xm else if i = 3 then yh else if i = 4 then ym
  else if i = 5 then zh else if i = 6 then zm else 0

set_option maxHeartbeats 4000000 in
theorem t1 (xh xm yh ym zh zm m1 m2 : Nat) (mem : Nat → Nat)
   (hx : mem xh = 0) (hy : mem yh = 128) (h1 : mem xm = m1) (h2 : mem ym = m2) (hm1 : m1 < W) (hm2 : m2 < W) :
   ∃ s, exe aAddInt (regsA xh xm yh ym zh zm) mem = .done s ∧ s.stores = [(zh, if m1 < m2 then 128 else 0), (zm, absDiff m1 m2)] := by
  by_cases hb : m2 ≤ m1
  · simp [exe, run, aAddInt, start, exec, St.set, St.get, St.store, St.goto, regsA, hx, hy, h1, h2, wsub, wadd, hb, wneg]
    done
  · sorry
