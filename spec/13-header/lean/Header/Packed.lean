import Header.MulRow
import Header.Isa
/-!
# Packed forms: the shared part

A packed number is one 64-bit word holding a magnitude `m < 2^62` and a direction `d = 2s + a` (`s` the sign, `a` the
axis); its value is round 1's `A.val ⟨m, d⟩ = m · i^d`. A layout says where `m` and `d` sit (`PackedTop.lean`,
`PackedBottom.lean`, `PackedSplit.lean`). This file holds what every layout shares:

* `Layout`: an encoding and a decoding, with the proofs that they are inverse on magnitudes below `2^62` and that every
  encoded word is a word;
* the reference operations: decode, round 1's operation (`A.add (2^62)`, `A.sub`, `A.neg`, `A.mul`, `A.div`), encode;
  and their correctness against round 1's values;
* bit lemmas the word formulas need (masks, disjoint OR, XOR of equal parts).

A word formula is then proved by showing it decodes to the reference result (`PackedTop.lean` and the others). Zero may
have any direction (spec v0.2 §3), so formulas are compared on the magnitude and, unless the result is zero, the
direction.
-/

namespace Header
namespace Packed
open A

/-- The magnitude bound, `2^62`. -/
abbrev Q : Nat := 4611686018427387904
/-- A word's modulus, `2^64`. -/
abbrev W64 : Nat := 18446744073709551616

theorem Q_eq : Q = 2 ^ 62 := rfl
theorem W64_eq : W64 = 2 ^ 64 := rfl

/-- `2^63`, the sign bit (top and split layouts). -/
abbrev SB : Nat := 9223372036854775808

theorem fin4_ofNat_val (d : Fin 4) : Fin.ofNat 4 d.val = d := by
  apply Fin.ext; simp [Fin.ofNat]

/-- `omega` after unfolding the word constants. -/
macro "qomega" : tactic => `(tactic| ((try simp only [Q, W64, Nat.reducePow] at *) <;> omega))

/-- A layout: where the magnitude and the direction sit in a word. -/
structure Layout where
  enc : Nat → Fin 4 → Nat
  dec : Nat → Num
  dec_enc : ∀ m d, m < Q → dec (enc m d) = ⟨m, d⟩
  enc_lt : ∀ m d, m < Q → enc m d < W64

/-- A word's value. -/
def Layout.val (L : Layout) (w : Nat) : GI := A.val (L.dec w)

/-! ## Reference operations: decode, round 1's operation, encode -/

def refAdd (L : Layout) (w1 w2 : Nat) : Option Nat := (add Q (L.dec w1) (L.dec w2)).map fun r => L.enc r.m r.d
def refSub (L : Layout) (w1 w2 : Nat) : Option Nat := (sub Q (L.dec w1) (L.dec w2)).map fun r => L.enc r.m r.d
def refNeg (L : Layout) (w : Nat) : Nat := let r := neg (L.dec w); L.enc r.m r.d
def refMul (L : Layout) (w1 w2 : Nat) : Nat := let r := mul (L.dec w1) (L.dec w2); L.enc r.m r.d
def refDiv (L : Layout) (w1 w2 : Nat) : Nat := let r := div (L.dec w1) (L.dec w2); L.enc r.m r.d

section
variable (L : Layout) {m1 m2 : Nat} {d1 d2 : Fin 4} (h1 : m1 < Q) (h2 : m2 < Q)
include h1 h2

/-- **Reference add.** With no overflow (`m1 + m2 < 2^62`) and not across axes with two nonzero magnitudes, the
reference add gives a word whose value is the sum. -/
theorem refAdd_correct (hs : m1 + m2 < Q) (hax : ¬ diffAxes d1 d2 ∨ m1 = 0 ∨ m2 = 0) :
    ∃ w, refAdd L (L.enc m1 d1) (L.enc m2 d2) = some w ∧ L.val w = A.val ⟨m1, d1⟩ + A.val ⟨m2, d2⟩ := by
  obtain ⟨r, hr, hv, hrm⟩ := add_value (N := Q) (x := ⟨m1, d1⟩) (y := ⟨m2, d2⟩) hs hax
  refine ⟨L.enc r.m r.d, ?_, ?_⟩
  · simp [refAdd, L.dec_enc _ _ h1, L.dec_enc _ _ h2, hr]
  · simp [Layout.val, L.dec_enc _ _ hrm, hv]

/-- The reference add reports an expression (`none`) exactly when round 1 does. -/
theorem refAdd_none_iff :
    refAdd L (L.enc m1 d1) (L.enc m2 d2) = none ↔ diffAxes d1 d2 ∧ m1 ≠ 0 ∧ m2 ≠ 0 := by
  simp [refAdd, L.dec_enc _ _ h1, L.dec_enc _ _ h2, add_eq_none_iff]

theorem refSub_correct (hs : m1 + m2 < Q) (hax : ¬ diffAxes d1 d2 ∨ m1 = 0 ∨ m2 = 0) :
    ∃ w, refSub L (L.enc m1 d1) (L.enc m2 d2) = some w ∧ L.val w = A.val ⟨m1, d1⟩ - A.val ⟨m2, d2⟩ := by
  obtain ⟨r, hr, hv, hrm⟩ := sub_value (N := Q) (x := ⟨m1, d1⟩) (y := ⟨m2, d2⟩) hs hax
  refine ⟨L.enc r.m r.d, ?_, ?_⟩
  · simp [refSub, L.dec_enc _ _ h1, L.dec_enc _ _ h2, hr]
  · simp [Layout.val, L.dec_enc _ _ hrm, hv]

theorem refMul_correct (hp : m1 * m2 < Q) :
    L.val (refMul L (L.enc m1 d1) (L.enc m2 d2)) = A.val ⟨m1, d1⟩ * A.val ⟨m2, d2⟩ := by
  simp [refMul, Layout.val, L.dec_enc _ _ h1, L.dec_enc _ _ h2, mul, L.dec_enc _ _ hp]
  exact val_mul ⟨m1, d1⟩ ⟨m2, d2⟩

theorem refDiv_correct (hy : 0 < m2) (hd : m2 ∣ m1) :
    L.val (refDiv L (L.enc m1 d1) (L.enc m2 d2)) * A.val ⟨m2, d2⟩ = A.val ⟨m1, d1⟩ := by
  have hq : m1 / m2 < Q := Nat.lt_of_le_of_lt (Nat.div_le_self _ _) h1
  simp [refDiv, Layout.val, L.dec_enc _ _ h1, L.dec_enc _ _ h2, div, L.dec_enc _ _ hq]
  exact val_div_mul (x := ⟨m1, d1⟩) (y := ⟨m2, d2⟩) hy hd

end

theorem refNeg_correct (L : Layout) {m : Nat} {d : Fin 4} (h : m < Q) :
    L.val (refNeg L (L.enc m d)) = -A.val ⟨m, d⟩ := by
  simp [refNeg, Layout.val, L.dec_enc _ _ h, neg]
  exact val_neg ⟨m, d⟩

/-! ## Bit lemmas -/

/-- AND with the high mask `2^64 − 2^k` keeps the bits from `k` up. -/
theorem and_high {x k : Nat} (hk : k ≤ 64) (hx : x < 2 ^ 64) :
    x &&& (2 ^ 64 - 2 ^ k) = 2 ^ k * (x / 2 ^ k) := by
  have hm : 2 ^ 64 - 2 ^ k = (2 ^ (64 - k) - 1) * 2 ^ k := by
    rw [Nat.sub_mul, Nat.one_mul, ← Nat.pow_add, Nat.sub_add_cancel hk]
  rw [hm, Nat.mul_comm (2 ^ k)]
  apply Nat.eq_of_testBit_eq
  intro i
  rw [Nat.testBit_and, Nat.testBit_mul_two_pow, Nat.testBit_mul_two_pow, Nat.testBit_two_pow_sub_one,
    Nat.testBit_div_two_pow]
  by_cases hi : k ≤ i
  · simp only [hi, decide_true, Bool.true_and, Nat.sub_add_cancel hi]
    by_cases h64 : i < 64
    · have : i - k < 64 - k := by omega
      simp [this]
    · have : ¬ i - k < 64 - k := by omega
      simp only [this, decide_false, Bool.and_false]
      exact (Nat.testBit_lt_two_pow (Nat.lt_of_lt_of_le hx (Nat.pow_le_pow_right (by decide : 2 > 0) (by omega : 64 ≤ i)))).symm
  · simp [hi]

/-- `x xor (x mod 2^k)` clears the low `k` bits. -/
theorem xor_low (x k : Nat) : x ^^^ (x % 2 ^ k) = 2 ^ k * (x / 2 ^ k) := by
  apply Nat.eq_of_testBit_eq
  intro i
  rw [Nat.testBit_xor, Nat.testBit_mod_two_pow, Nat.mul_comm, Nat.testBit_mul_two_pow, Nat.testBit_div_two_pow]
  by_cases hi : i < k
  · have : ¬ k ≤ i := by omega
    simp [hi, this]
  · have : k ≤ i := by omega
    simp [hi, this, Nat.sub_add_cancel this]

/-- OR of a multiple of `2^k` and something below `2^k` is their sum. -/
theorem or_low {a b k : Nat} (hb : b < 2 ^ k) : (2 ^ k * a) ||| b = 2 ^ k * a + b :=
  (Nat.two_pow_add_eq_or_of_lt hb a).symm

/-- XOR acts on a word's high and low parts separately. -/
theorem xor_parts {a b c e k : Nat} (hb : b < 2 ^ k) (he : e < 2 ^ k) :
    (2 ^ k * a + b) ^^^ (2 ^ k * c + e) = 2 ^ k * (a ^^^ c) + (b ^^^ e) := by
  apply Nat.eq_of_testBit_eq
  intro j
  rw [Nat.testBit_xor, Nat.testBit_two_pow_mul_add _ hb, Nat.testBit_two_pow_mul_add _ he,
    Nat.testBit_two_pow_mul_add _ (Nat.xor_lt_two_pow hb he), Nat.testBit_xor, Nat.testBit_xor]
  split <;> rfl

/-- AND acts on a word's high and low parts separately. -/
theorem and_parts {a b c e k : Nat} (hb : b < 2 ^ k) (he : e < 2 ^ k) :
    (2 ^ k * a + b) &&& (2 ^ k * c + e) = 2 ^ k * (a &&& c) + (b &&& e) := by
  apply Nat.eq_of_testBit_eq
  intro j
  rw [Nat.testBit_and, Nat.testBit_two_pow_mul_add _ hb, Nat.testBit_two_pow_mul_add _ he,
    Nat.testBit_two_pow_mul_add _ (Nat.and_lt_two_pow b he), Nat.testBit_and, Nat.testBit_and]
  split <;> rfl

/-- OR acts on a word's high and low parts separately. -/
theorem or_parts {a b c e k : Nat} (hb : b < 2 ^ k) (he : e < 2 ^ k) :
    (2 ^ k * a + b) ||| (2 ^ k * c + e) = 2 ^ k * (a ||| c) + (b ||| e) := by
  apply Nat.eq_of_testBit_eq
  intro j
  rw [Nat.testBit_or, Nat.testBit_two_pow_mul_add _ hb, Nat.testBit_two_pow_mul_add _ he,
    Nat.testBit_two_pow_mul_add _ (Nat.or_lt_two_pow hb he), Nat.testBit_or, Nat.testBit_or]
  split <;> rfl

/-- OR with the low mask `2^k − 1` fills a number below `2^k`. -/
theorem or_ones_low {x k : Nat} (hx : x < 2 ^ k) : x ||| (2 ^ k - 1) = 2 ^ k - 1 := by
  apply Nat.eq_of_testBit_eq
  intro i
  rw [Nat.testBit_or, Nat.testBit_two_pow_sub_one]
  by_cases hi : i < k
  · simp [hi]
  · have : k ≤ i := by omega
    simp only [hi, decide_false, Bool.or_false]
    exact Nat.testBit_lt_two_pow (Nat.lt_of_lt_of_le hx (Nat.pow_le_pow_right (by decide) this))

/-- `w1 xor (w1 xor w2) = w2`. -/
theorem xor_xor_cancel (a b : Nat) : a ^^^ (a ^^^ b) = b := by
  rw [← Nat.xor_assoc, Nat.xor_self, Nat.zero_xor]

/-- `w2 xor (w1 xor w2) = w1`. -/
theorem xor_xor_cancel' (a b : Nat) : b ^^^ (a ^^^ b) = a := by
  rw [Nat.xor_comm a b, xor_xor_cancel]

theorem and_ones64 {x : Nat} (hx : x < 2 ^ 64) : x &&& (2 ^ 64 - 1) = x := by
  rw [Nat.and_two_pow_sub_one_eq_mod, Nat.mod_eq_of_lt hx]

theorem xor_ones64 {x : Nat} (hx : x < 2 ^ 64) : x ^^^ (2 ^ 64 - 1) = 2 ^ 64 - 1 - x := by
  have := Isa.xor_ones (x := x) (by rw [Isa.W_eq]; exact hx)
  simpa using this

/-- `x sar 63` on a 64-bit word. -/
def sar63 (x : Nat) : Nat := if x < 2 ^ 63 then 0 else 2 ^ 64 - 1

/-- The unsigned compare as a 0/1 value. -/
def ltb (a b : Nat) : Nat := if a < b then 1 else 0

/-- `a ≠ 0` as a 0/1 value. -/
def nzb (a : Nat) : Nat := if a ≠ 0 then 1 else 0

end Packed
end Header
