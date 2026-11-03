import Header.Packed
/-!
# Packed form, bottom layout

`w = 4m + d`: the direction in bits 0–1, the magnitude in bits 2–63. Formulas from `../prior-art.md`, "Packed form",
and `../test/prior-art/check.py` (`p_add_bot`, `p_mul_bot`, `p_div_bot`), on natural numbers mod `2^64`; each is one
definition below.
-/

namespace Header
namespace Packed
open A

/-- `NOT 3` on 64 bits, `2^64 − 4`. -/
abbrev N3 : Nat := 18446744073709551612

def botEnc (m : Nat) (d : Fin 4) : Nat := 4 * m + d.val
def botDec (w : Nat) : Num := ⟨w / 4, Fin.ofNat 4 (w % 4)⟩

def Bottom : Layout where
  enc := botEnc
  dec := botDec
  dec_enc m d h := by
    simp only [botEnc, botDec]
    have h1 : (4 * m + d.val) / 4 = m := by have := d.isLt; omega
    have h2 : (4 * m + d.val) % 4 = d.val := by have := d.isLt; omega
    rw [h1, h2, fin4_ofNat_val]
  enc_lt m d h := by simp only [botEnc]; have := d.isLt; qomega

/-! ## The formulas -/

def botAdd (w1 w2 : Nat) : Nat × Nat :=
  let t := w1 ^^^ w2
  let s := sar63 ((t <<< 62) % W64)
  let L := (W64 - ltb w1 (w2 &&& N3)) % W64
  let W := w1 ^^^ (t &&& L)
  let V := W ^^^ t
  let mn := V &&& N3
  ((W + (mn ^^^ s) + (W64 - s)) % W64, t &&& 1 &&& nzb mn)

def botSub (w1 w2 : Nat) : Nat × Nat := botAdd w1 (w2 ^^^ 2)
def botNeg (w : Nat) : Nat := w ^^^ 2
def botMul (w1 w2 : Nat) : Nat := (((w1 >>> 2) * (w2 &&& N3)) % W64) ||| ((w1 + w2) &&& 3)
def botDiv (w1 w2 : Nat) : Nat := (((w1 &&& N3) / (w2 >>> 2)) % W64) ||| (((w1 + (W64 - w2)) % W64) &&& 3)

/-! ## Structural facts -/

theorem N3_eq : N3 = 2 ^ 64 - 2 ^ 2 := rfl

theorem benc_lt (m : Nat) (d : Fin 4) (h : m < Q) : botEnc m d < 2 ^ 64 := by
  simp only [botEnc]; have := d.isLt; qomega

theorem benc_and_N3 (m : Nat) (d : Fin 4) (h : m < Q) : botEnc m d &&& N3 = 4 * m := by
  rw [N3_eq, and_high (by decide) (benc_lt m d h)]; simp only [botEnc]; have := d.isLt; omega

theorem benc_comm (m : Nat) (d : Fin 4) : botEnc m d = 2 ^ 2 * m + d.val := rfl

theorem benc_xor (m1 m2 : Nat) (d1 d2 : Fin 4) :
    botEnc m1 d1 ^^^ botEnc m2 d2 = 2 ^ 2 * (m1 ^^^ m2) + (d1.val ^^^ d2.val) := by
  rw [benc_comm, benc_comm, xor_parts d1.isLt d2.isLt]

theorem bt_mod4 (m1 m2 : Nat) (d1 d2 : Fin 4) : (botEnc m1 d1 ^^^ botEnc m2 d2) % 4 = d1.val ^^^ d2.val := by
  rw [benc_xor]; have := Nat.xor_lt_two_pow (n := 2) d1.isLt d2.isLt; omega

theorem bsar_t (m1 m2 : Nat) (d1 d2 : Fin 4) :
    sar63 (((botEnc m1 d1 ^^^ botEnc m2 d2) <<< 62) % W64) =
      if d1.val ^^^ d2.val < 2 then 0 else 2 ^ 64 - 1 := by
  have e := bt_mod4 m1 m2 d1 d2
  rw [Nat.shiftLeft_eq]
  unfold sar63
  generalize botEnc m1 d1 ^^^ botEnc m2 d2 = t at e
  have : (t * 2 ^ 62) % W64 < 2 ^ 63 ↔ d1.val ^^^ d2.val < 2 := by rw [← e]; qomega
  simp only [this]

theorem bL (m1 m2 : Nat) (d1 d2 : Fin 4) (h2 : m2 < Q) :
    (W64 - ltb (botEnc m1 d1) (botEnc m2 d2 &&& N3)) % W64 = if m1 < m2 then 2 ^ 64 - 1 else 0 := by
  rw [benc_and_N3 _ _ h2]; unfold ltb; simp only [botEnc]
  have := d1.isLt
  by_cases h : m1 < m2
  · have : 4 * m1 + d1.val < 4 * m2 := by omega
    simp [h, this]
  · have : ¬ 4 * m1 + d1.val < 4 * m2 := by omega
    simp [h, this]

theorem bt_lt (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) :
    botEnc m1 d1 ^^^ botEnc m2 d2 < 2 ^ 64 := Nat.xor_lt_two_pow (benc_lt _ _ h1) (benc_lt _ _ h2)

theorem bt_and1 (m1 m2 : Nat) (d1 d2 : Fin 4) : (botEnc m1 d1 ^^^ botEnc m2 d2) &&& 1 = (d1.val ^^^ d2.val) % 2 := by
  rw [show (1 : Nat) = 2 ^ 1 - 1 from rfl, Nat.and_two_pow_sub_one_eq_mod]
  have := bt_mod4 m1 m2 d1 d2; omega

theorem botAdd_eval (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) :
    botAdd (botEnc m1 d1) (botEnc m2 d2) =
      (let W := if m1 < m2 then botEnc m2 d2 else botEnc m1 d1
       let mn := if m1 < m2 then 4 * m1 else 4 * m2
       let s := if d1.val ^^^ d2.val < 2 then 0 else 2 ^ 64 - 1
       ((W + (mn ^^^ s) + (W64 - s)) % W64, (d1.val ^^^ d2.val) % 2 &&& nzb mn)) := by
  simp only [botAdd, bL _ _ _ _ h2, bsar_t, bt_and1]
  by_cases h : m1 < m2
  · simp only [h, ite_true, and_ones64 (bt_lt _ _ _ _ h1 h2), xor_xor_cancel, xor_xor_cancel', benc_and_N3 _ _ h1]
  · simp only [h, ite_false, Nat.and_zero, Nat.xor_zero]
    rw [xor_xor_cancel]
    simp only [benc_and_N3 _ _ h2]

/-! ## Add and subtract -/

/-- **Bottom add.** As `topAdd_eq`. -/
theorem botAdd_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (botAdd (botEnc m1 d1) (botEnc m2 d2)).2 = 0 ∧
        (botDec (botAdd (botEnc m1 d1) (botEnc m2 d2)).1).m = r.m ∧
        ((botDec (botAdd (botEnc m1 d1) (botEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (botAdd (botEnc m1 d1) (botEnc m2 d2)).2 = 1 := by
  rw [botAdd_eval _ _ _ _ h1 h2]
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb, botEnc, botDec,
    xor_ones64 (by qomega : 4 * m1 < 2 ^ 64), xor_ones64 (by qomega : 4 * m2 < 2 ^ 64), Fin.ext_iff, Fin.val_ofNat]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

theorem xor2' : ∀ d : Fin 4, d.val ^^^ 2 = (d + 2).val := by decide

theorem benc_flip (m : Nat) (d : Fin 4) : botEnc m d ^^^ 2 = botEnc m (d + 2) := by
  rw [benc_comm, benc_comm, show (2 : Nat) = 2 ^ 2 * 0 + 2 from rfl, xor_parts d.isLt (by decide), Nat.xor_zero, xor2']

theorem botNeg_eq (m : Nat) (d : Fin 4) :
    botNeg (botEnc m d) = botEnc (neg ⟨m, d⟩).m (neg ⟨m, d⟩).d := by
  simp only [botNeg, neg]; exact benc_flip m d

theorem botSub_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match sub Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (botSub (botEnc m1 d1) (botEnc m2 d2)).2 = 0 ∧
        (botDec (botSub (botEnc m1 d1) (botEnc m2 d2)).1).m = r.m ∧
        ((botDec (botSub (botEnc m1 d1) (botEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (botSub (botEnc m1 d1) (botEnc m2 d2)).2 = 1 := by
  simp only [botSub, benc_flip m2 d2, sub, neg]
  exact botAdd_eq m1 m2 d1 (d2 + 2) h1 h2 hs

/-! ## Multiply and divide: exact words -/

/-- **Bottom multiply** gives the word of round 1's `A.mul` exactly, when the product fits. -/
theorem botMul_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hp : m1 * m2 < Q) :
    botMul (botEnc m1 d1) (botEnc m2 d2) = botEnc (mul ⟨m1, d1⟩ ⟨m2, d2⟩).m (mul ⟨m1, d1⟩ ⟨m2, d2⟩).d := by
  simp only [botMul, benc_and_N3 _ _ h2, mul]
  have e0 : botEnc m1 d1 >>> 2 = m1 := by
    rw [Nat.shiftRight_eq_div_pow]; simp only [botEnc]; have := d1.isLt; omega
  have e1 : (m1 * (4 * m2)) % W64 = 2 ^ 2 * (m1 * m2) := by
    rw [show m1 * (4 * m2) = 4 * (m1 * m2) by rw [Nat.mul_left_comm]]; qomega
  have e2 : (botEnc m1 d1 + botEnc m2 d2) &&& 3 = (d1 + d2).val := by
    rw [show (3 : Nat) = 2 ^ 2 - 1 from rfl, Nat.and_two_pow_sub_one_eq_mod, Fin.val_add]
    simp only [botEnc]; omega
  rw [e0, e1, e2, or_low (d1 + d2).isLt]; rfl

/-- **Bottom exact division** gives the word of round 1's `A.div` exactly. -/
theorem botDiv_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hy : 0 < m2) (hd : m2 ∣ m1) :
    botDiv (botEnc m1 d1) (botEnc m2 d2) = botEnc (div ⟨m1, d1⟩ ⟨m2, d2⟩).m (div ⟨m1, d1⟩ ⟨m2, d2⟩).d := by
  simp only [botDiv, benc_and_N3 _ _ h1, div]
  obtain ⟨q, rfl⟩ := hd
  have e0 : botEnc m2 d2 >>> 2 = m2 := by
    rw [Nat.shiftRight_eq_div_pow]; simp only [botEnc]; have := d2.isLt; omega
  have hq : m2 * q / m2 = q := Nat.mul_div_cancel_left q hy
  have e1 : 4 * (m2 * q) / m2 = 2 ^ 2 * q := by
    rw [show 4 * (m2 * q) = m2 * (4 * q) by rw [Nat.mul_left_comm], Nat.mul_div_cancel_left _ hy]
  have hql : q ≤ m2 * q := Nat.le_mul_of_pos_left q hy
  have e2 : (2 ^ 2 * q) % W64 = 2 ^ 2 * q := Nat.mod_eq_of_lt (by qomega)
  have e3 : ((botEnc (m2 * q) d1 + (W64 - botEnc m2 d2)) % W64) &&& 3 = (d1 - d2).val := by
    rw [show (3 : Nat) = 2 ^ 2 - 1 from rfl, Nat.and_two_pow_sub_one_eq_mod, Fin.sub_def]
    simp only [botEnc]; have := d1.isLt; have := d2.isLt; qomega
  rw [e0, e1, e2, e3, or_low (d1 - d2).isLt, hq]; rfl

end Packed
end Header
