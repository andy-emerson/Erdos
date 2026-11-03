import Header.PackedTop
/-! # Packed form, top layout: multiply and divide (split from `PackedTop.lean` to keep each build in memory) -/

namespace Header
namespace Packed
open A

/-! ## Multiply and divide -/

theorem enc_xor_low (m : Nat) (d : Fin 4) (h : m < Q) : topEnc m d ^^^ m = Q * d.val := by
  have e : topEnc m d ^^^ m = (2 ^ 62 * d.val + m) ^^^ (2 ^ 62 * 0 + m) := by simp [enc_comm]
  rw [e, xor_parts (by qomega) (by qomega), Nat.xor_zero, Nat.xor_self, Nat.add_zero]

/-- **Top multiply** gives the word of round 1's `A.mul` exactly, when the product fits. -/
theorem topMul_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hp : m1 * m2 < Q) :
    topMul (topEnc m1 d1) (topEnc m2 d2) = topEnc (mul ⟨m1, d1⟩ ⟨m2, d2⟩).m (mul ⟨m1, d1⟩ ⟨m2, d2⟩).d := by
  simp only [topMul, enc_and_M _ _ h1, enc_and_M _ _ h2, enc_xor_low _ _ h1, enc_xor_low _ _ h2, mul]
  have e1 : (m1 * m2) % W64 = m1 * m2 := Nat.mod_eq_of_lt (by qomega)
  have e2 : (Q * d1.val + Q * d2.val) % W64 = 2 ^ 62 * (d1 + d2).val := by
    rw [Fin.val_add]; have := d1.isLt; have := d2.isLt; qomega
  rw [e1, e2, Nat.or_comm, or_low (by qomega), enc_comm]

/-- Two nonzero factors sum to at most their product plus one. -/
theorem add_le_mul_succ {a b : Nat} (ha : 1 ≤ a) (hb : 1 ≤ b) : a + b ≤ a * b + 1 := by
  have h := Nat.mul_le_mul_left (a - 1) hb
  rw [Nat.mul_one, Nat.sub_one_mul] at h
  have : b ≤ a * b := Nat.le_mul_of_pos_left b ha
  omega

/-- **Top multiply, the `lea` form** `(w1 + w2 − 1) and not M`: the product's magnitude always, and its direction
unless the product is zero. -/
theorem topMulLea_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hp : m1 * m2 < Q) :
    (topDec (topMulLea (topEnc m1 d1) (topEnc m2 d2))).m = m1 * m2 ∧
    ((topDec (topMulLea (topEnc m1 d1) (topEnc m2 d2))).d = d1 + d2 ∨ m1 * m2 = 0) := by
  simp only [topMulLea, enc_and_M _ _ h1, enc_and_M _ _ h2]
  have e1 : (m1 * m2) % W64 = m1 * m2 := Nat.mod_eq_of_lt (by qomega)
  have hx : (topEnc m1 d1 + topEnc m2 d2 + (W64 - 1)) % W64 < 2 ^ 64 := Nat.mod_lt _ (by decide)
  rw [e1, NM_eq, and_high (by decide) hx, Nat.or_comm, or_low (by qomega)]
  simp only [topDec, Fin.ext_iff, Fin.val_ofNat, Fin.val_add, topEnc]
  have := d1.isLt; have := d2.isLt
  by_cases z : m1 * m2 = 0
  · refine ⟨by qomega, Or.inr z⟩
  · have ha : 1 ≤ m1 := by
      rcases Nat.eq_zero_or_pos m1 with h | h
      · simp [h] at z
      · omega
    have hb : 1 ≤ m2 := by
      rcases Nat.eq_zero_or_pos m2 with h | h
      · simp [h] at z
      · omega
    have := add_le_mul_succ ha hb
    refine ⟨by qomega, Or.inl (by qomega)⟩

/-- A high part and a low part, read back after reducing mod `4 · P`. -/
theorem parts_mod {P a b : Nat} (hP : 0 < P) (hb : b < P) :
    (P * a + b) % (P * 4) / P = a % 4 := by
  rw [Nat.mod_mul_right_div_self, Nat.mul_add_div hP, Nat.div_eq_of_lt hb, Nat.add_zero]

theorem parts_lo {P a b : Nat} (hb : b < P) : (P * a + b) % P = b := by
  rw [Nat.mul_add_mod, Nat.mod_eq_of_lt hb]

theorem parts_hi {P a b : Nat} (hP : 0 < P) (hb : b < P) : (P * a + b) / P = a := by
  rw [Nat.mul_add_div hP, Nat.div_eq_of_lt hb, Nat.add_zero]

/-- **Top exact division**: the quotient's magnitude always, and its direction unless the dividend is zero. -/
theorem topDiv_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hy : 0 < m2) (hd : m2 ∣ m1) :
    (topDec (topDiv (topEnc m1 d1) (topEnc m2 d2))).m = (div ⟨m1, d1⟩ ⟨m2, d2⟩).m ∧
    ((topDec (topDiv (topEnc m1 d1) (topEnc m2 d2))).d = (div ⟨m1, d1⟩ ⟨m2, d2⟩).d ∨ m1 = 0) := by
  simp only [topDiv, enc_and_M _ _ h1, enc_and_M _ _ h2, div]
  have hq : m1 / m2 ≤ m1 := Nat.div_le_self _ _
  have hx : (topEnc m1 d1 + (W64 - topEnc m2 d2)) % W64 < 2 ^ 64 := Nat.mod_lt _ (by decide)
  rw [NM_eq, and_high (by decide) hx, Nat.or_comm, or_low (by qomega)]
  simp only [topDec, Fin.ext_iff, Fin.val_ofNat, Fin.sub_def, topEnc]
  have := d1.isLt; have := d2.isLt
  have hQ : (0 : Nat) < Q := by decide
  rw [← Q_eq]
  have hqQ : m1 / m2 < Q := Nat.lt_of_le_of_lt hq h1
  by_cases z : m1 = 0
  · refine ⟨parts_lo hqQ, Or.inr z⟩
  · have hge : m2 ≤ m1 := Nat.le_of_dvd (by omega) hd
    have e1 : ↑d1 * Q + m1 + (W64 - (↑d2 * Q + m2)) = Q * (↑d1 + 4 - ↑d2) + (m1 - m2) := by qomega
    have e2 : W64 = Q * 4 := rfl
    rw [e1, e2, parts_mod hQ (by omega)]
    refine ⟨parts_lo hqQ, Or.inl ?_⟩
    rw [parts_hi hQ hqQ, Nat.mod_mod]
    omega

end Packed
end Header
