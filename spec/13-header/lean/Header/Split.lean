import Header.Isa
/-!
# Running a program in two parts

`stepN p k s` executes `k` instructions from `s` (none if the run ends or traps first). `run_split`: if those `k`
steps go through, running with fuel `n + k` from `s` is running with fuel `n` from the state they reach. The long
straight-line proof of A's branch-free complex row uses it to cut the symbolic run in two, which keeps the kernel's
work small.
-/

namespace Header
namespace Isa

def stepN (p : Prog) : Nat → St → Option St
  | 0, s => some s
  | k + 1, s =>
    if s.pc = p.length then none else
    match p[s.pc]? with
    | none => none
    | some .trap => none
    | some i => stepN p k (exec { s with trace := s.trace ++ [s.pc] } i)

theorem run_split (p : Prog) : ∀ (k n : Nat) (s t : St), stepN p k s = some t → run p (n + k) s = run p n t
  | 0, n, s, t, h => by simp [stepN] at h; subst h; rfl
  | k + 1, n, s, t, h => by
    rw [show n + (k + 1) = (n + k) + 1 by omega]
    simp only [stepN] at h
    simp only [run]
    by_cases hpc : s.pc = p.length
    · simp [hpc] at h
    · simp only [hpc, ite_false] at h ⊢
      cases hp : p[s.pc]? with
      | none => simp [hp] at h
      | some i =>
        simp only [hp] at h ⊢
        cases i <;> first | (simp at h; done) | exact run_split p k n _ t h

end Isa
end Header
