import Header.ProgMul
import Header.Split
/-! A's multiplication row, one lemma per case: sub, direction of `x` = 0. Each lemma runs the first 20
instructions (`stepN`), records the registers still needed, and runs the rest from there (`run_split`). -/

set_option linter.unusedVariables false
set_option linter.unusedSimpArgs false
set_option maxHeartbeats 8000000

namespace Header
namespace PA
open Isa

theorem mulRow_sub_0_0_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 0) (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_0_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 0) (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_0_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 0) (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_0_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 0) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_0_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 0) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_1_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 1) (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_1_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 1) (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_1_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 1) (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_1_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 1) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_1_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 1) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_2_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_2_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_2_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_2_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_2_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 2) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_3_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 3) (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_3_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 3) (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : ¬ m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_3_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 3) (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_3_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 3) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  have z2'' : (0 < m2) = False := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

theorem mulRow_sub_0_3_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 0) (e2 : mem yh = hdr 3) (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 0⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (mulRow true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (mulRow true).length ∧ s.r 21 = 1 := by
  have h' : m2 ≤ m1 := by omega
  have z1' := z1
  have z2' := z2
  have z2'' : (0 < m2) = True := by simp; omega
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (mulRow true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 11, s.r 12, s.r 13, s.r 15, s.r 16,
      s.r 17, s.r 21, s.r 22, s.r 23, s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2''] at f
    obtain ⟨fpc, f5, f6, f10, f11, f12, f13, f15, f16, f17, f21, f22, f23, ftr, fsto⟩ := f
    rw [exe, show (mulRow true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f11, ← f12, ← f13,
      ← f15, ← f16, ← f17, ← f21, ← f22, ← f23, ← ftr, fsto, mulRow, pre, kk, regs6, hdr_0, hdr_1, hdr_2, hdr_3, e1, e2, e3, e4, List.range, List.range.loop, wsltu, Nat.lt_one_iff, h, h', z1', z2', z2'', clockAdd, absDiffClock, clockSub, clockNeg,
      borrow, wadd, wsub, wneg, wmul, W]
    all_goals unfold W at hm1 hm2
    all_goals (repeat' split) <;> omega

end PA
end Header
