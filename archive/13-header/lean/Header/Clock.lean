import Header.Basic
/-!
# The clock: a word of `N` values, modelled without `%`

For magnitudes `m1, m2 < N` (one word, `N = 2^n`):
* the clock difference is `m1 - m2` when `m1 ≥ m2`, else `m1 + N - m2` (it wrapped);
* the borrow is `b = (m1 < m2)`;
* clock negation of `r < N` is `N - r` when `r > 0`, else `0`;
* the absolute-difference routine subtracts, then negates on the clock when `b = 1`.
-/

set_option linter.unusedVariables false

namespace Header

/-- The machine's subtraction on an `N`-hour clock. -/
def clockSub (N m1 m2 : Nat) : Nat := if m2 ≤ m1 then m1 - m2 else m1 + N - m2

/-- The borrow the subtraction reports: `1` exactly when `m1 < m2`. -/
def borrow (m1 m2 : Nat) : Bool := decide (m1 < m2)

/-- Negation on the clock. -/
def clockNeg (N r : Nat) : Nat := if r = 0 then 0 else N - r

/-- Addition on the clock (wraps on a carry). -/
def clockAdd (N m1 m2 : Nat) : Nat := if m1 + m2 < N then m1 + m2 else m1 + m2 - N

/-- The absolute-difference routine: `(-1)^b · (m1 - m2)` taken on the clock, i.e. negate when `b = 1`. -/
def absDiffClock (N m1 m2 : Nat) : Nat :=
  if borrow m1 m2 then clockNeg N (clockSub N m1 m2) else clockSub N m1 m2

/-- `|m1 - m2|` as naturals: the larger minus the smaller. -/
def absDiff (m1 m2 : Nat) : Nat := if m2 ≤ m1 then m1 - m2 else m2 - m1

theorem borrow_true_iff {m1 m2 : Nat} : borrow m1 m2 = true ↔ m1 < m2 := by simp [borrow]
theorem borrow_false_iff {m1 m2 : Nat} : borrow m1 m2 = false ↔ m2 ≤ m1 := by
  simp [borrow]

/-- The clock difference stays on the clock. -/
theorem clockSub_lt {N m1 m2 : Nat} (h1 : m1 < N) (h2 : m2 < N) : clockSub N m1 m2 < N := by
  unfold clockSub; split <;> omega

/-- The clock difference is `m1 - m2` plus `N` exactly when there was a borrow
(so it is `m1 - m2 mod N`, and the borrow records the wrap). -/
theorem clockSub_int {N m1 m2 : Nat} (h1 : m1 < N) (h2 : m2 < N) :
    (clockSub N m1 m2 : Int) = (m1 : Int) - m2 + (if borrow m1 m2 then (N : Int) else 0) := by
  unfold clockSub borrow; by_cases h : m2 ≤ m1 <;> simp [h] <;> omega

/-- Clock negation: `r + clockNeg N r` is `N` (≡ 0) for `0 < r < N`, and `clockNeg N 0 = 0`. -/
theorem clockNeg_spec {N r : Nat} (h : r < N) :
    clockNeg N r < N ∧ (r = 0 → clockNeg N r = 0) ∧ (0 < r → r + clockNeg N r = N) := by
  unfold clockNeg; split <;> omega

/-- Clock addition is the true sum when it does not overflow. -/
theorem clockAdd_of_lt {N m1 m2 : Nat} (h : m1 + m2 < N) : clockAdd N m1 m2 = m1 + m2 := by
  simp [clockAdd, h]

/-- **Borrow lemma (magnitude).** For `m1, m2 < N`, negating the clock difference when `b = 1`
gives exactly `|m1 - m2|`. -/
theorem borrow_lemma {N m1 m2 : Nat} (h1 : m1 < N) (h2 : m2 < N) :
    absDiffClock N m1 m2 = absDiff m1 m2 := by
  unfold absDiffClock clockNeg clockSub absDiff borrow
  by_cases h : m1 < m2
  · have : ¬ m2 ≤ m1 := by omega
    simp only [h, this, decide_true, ite_true, ite_false]; split <;> omega
  · have : m2 ≤ m1 := by omega
    simp [h, this]

/-- The result of the routine stays on the clock. -/
theorem absDiffClock_lt {N m1 m2 : Nat} (h1 : m1 < N) (h2 : m2 < N) : absDiffClock N m1 m2 < N := by
  rw [borrow_lemma h1 h2]; unfold absDiff; split <;> omega

/-- **Borrow lemma (sign).** `m1 - m2 = (-1)^b · |m1 - m2|` as integers. -/
theorem absDiff_sign (m1 m2 : Nat) :
    (m1 : Int) - m2 = (if borrow m1 m2 then -1 else 1) * (absDiff m1 m2 : Int) := by
  unfold borrow absDiff; by_cases h : m1 < m2
  · have : ¬ m2 ≤ m1 := by omega
    simp [h, this]; omega
  · have : m2 ≤ m1 := by omega
    simp [h, this]; omega

/-- **Borrow lemma (direction).** Turning `d1` by `2b` multiplies its unit by `(-1)^b`. -/
theorem unit_borrow (d : Fin 4) (b : Bool) :
    unit (d + (if b then 2 else 0)) = GI.smul (if b then -1 else 1) (unit d) := by
  revert d b; decide

end Header
