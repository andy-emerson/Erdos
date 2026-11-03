import Header.ProgA
import Header.Split
/-! A's branch-free complex row (`bfCx`), one lemma per case: sub, direction of `x` = 2. Each lemma runs the
first 20 instructions (`stepN`), records the registers still needed, and runs the rest from that state
(`run_split`), which keeps the kernel's check of each proof small. -/

set_option linter.unusedVariables false
set_option maxHeartbeats 8000000

namespace Header
namespace PA
open Isa

theorem bfCx_sub_2_0_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 0)  (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_0_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 0)  (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_0_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 0)  (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_0_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 0)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_0_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 0)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 0) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_1_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 1)  (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_1_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 1)  (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_1_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 1)  (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_1_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 1)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_1_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 1)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 1) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_2_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 2)  (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_2_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 2)  (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_2_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 2)  (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_2_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 2)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_2_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 2)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 2) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_3_c0 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 3)  (h : m1 < m2) (z1 : m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_3_c1 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 3)  (h : m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : ¬ m2 ≤ m1 := by omega
  have h'' : m1 ≤ m2 := by omega
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_3_c2 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 3)  (h : ¬ m1 < m2) (z1 : m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' : (m1 = 0) = True := eq_true z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_3_c3 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 3)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' : (m2 = 0) = True := eq_true z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

theorem bfCx_sub_2_3_c4 (m1 m2 : Nat) (hm1 : m1 < W) (hm2 : m2 < W) (xh xm yh ym zh zm : Nat) (mem : Nat → Nat) (e3 : mem xm = m1) (e4 : mem ym = m2)
    (e1 : mem xh = hdr 2) (e2 : mem yh = hdr 3)  (h : ¬ m1 < m2) (z1 : ¬ m1 = 0) (z2 : ¬ m2 = 0) :
    match A.add W ⟨m1, 2⟩ (yEff true m2 3) with
    | some r => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 0 ∧ s.stores = [(zh, hdr r.d), (zm, r.m)]
    | none => ∃ s, exe (bfCx true) (regs6 xh xm yh ym zh zm) mem = .done s ∧
        s.trace = List.range (bfCx true).length ∧ s.r 21 = 1 := by
  have hs := wadd_lt m1 m2
  have hd := wsub_lt' m1 m2
  have hd' := wneg_lt (wsub m1 m2)
  have h' : m2 ≤ m1 := by omega
  have h'' : True := trivial
  have z1' := z1
  have z2' := z2
  simp only [yEff, A.add, A.neg, A.dirCheck]
  cases hst : stepN (bfCx true) 20 (start (regs6 xh xm yh ym zh zm) mem) with
  | none => simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at hst
  | some S =>
    have f := congrArg (Option.map fun s => (s.pc, s.r 5, s.r 6, s.r 10, s.r 12, s.r 13, s.r 16, s.r 17, s.r 21,
      s.trace, s.stores)) hst
    simp [stepN, start, exec, St.set, St.get, St.goto, St.next, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2'] at f
    obtain ⟨fpc, f5, f6, f10, f12, f13, f16, f17, f21, ftr, fsto⟩ := f
    rw [exe, show (bfCx true).length + 1 = 17 + 20 from rfl, run_split _ 20 17 _ _ hst]
    simp [run, exec, St.set, St.get, St.store, St.goto, St.next, ← fpc, ← f5, ← f6, ← f10, ← f12, ← f13, ← f16,
      ← f17, ← f21, ← ftr, fsto, bfCx, pre, kk, regs6, hdr, e1, e2, e3, e4, List.range, List.range.loop,
      borrow_lemma hm1 hm2, absDiff, borrow, wsltu, wneg_zero, wneg_one, wsub_zero hd, condneg_ones hd,
      select_ones hs hd', select_ones' (clockAdd_lt' hm1 hm2) (by omega : m1 - m2 < W), select_ones' (clockAdd_lt' hm1 hm2) (by omega : m2 - m1 < W),
      select_ones' (clockAdd_lt' hm1 hm2) hm1, select_ones' (clockAdd_lt' hm1 hm2) hm2, wsub_zero (by omega : m1 - m2 < W),
      wsub_zero (by omega : m2 - m1 < W), wsub_zero hm1, wsub_zero hm2, wadd_clock hm1 hm2, wsub_of_le hm1, wsub_of_le hm2,
      wneg_wsub hm1 hm2, Nat.lt_one_iff, mod_W_128, mod_W_0, wadd_0_0, wadd_0_128, wadd_64_0, wadd_64_128, wadd_128_0, wadd_128_128, wadd_192_0, wadd_192_128,
      h, h', h'', z1', z2']

end PA
end Header
