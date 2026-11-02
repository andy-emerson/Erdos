import Header.Packed
/-!
# Packed form, top layout

`w = d · 2^62 + m`: the direction in bits 62–63, the magnitude in bits 0–61. The formulas are those of
`../prior-art.md`, "Packed form", and of `../test/prior-art/check.py` (`p_add_top`, `p_mul_top`, `p_mul_top_lea`,
`p_div_top`), on natural numbers mod `2^64`; each is one definition below, so a change to a formula means editing it
and rerunning the proof.
-/

namespace Header
namespace Packed
open A

/-- `M = 2^62 − 1`, the magnitude mask. -/
abbrev M : Nat := 4611686018427387903
/-- `NOT M` on 64 bits, `2^64 − 2^62`. -/
abbrev NM : Nat := 13835058055282163712
/-- `2^63`, the sign bit. -/
abbrev SB : Nat := 9223372036854775808

def topEnc (m : Nat) (d : Fin 4) : Nat := d.val * Q + m
def topDec (w : Nat) : Num := ⟨w % Q, Fin.ofNat 4 (w / Q)⟩

theorem fin4_ofNat_val (d : Fin 4) : Fin.ofNat 4 d.val = d := by
  apply Fin.ext; simp [Fin.ofNat]

def Top : Layout where
  enc := topEnc
  dec := topDec
  dec_enc m d h := by
    simp only [topEnc, topDec]
    have h1 : (d.val * Q + m) % Q = m := by qomega
    have h2 : (d.val * Q + m) / Q = d.val := by qomega
    rw [h1, h2, fin4_ofNat_val]
  enc_lt m d h := by simp only [topEnc]; have := d.isLt; qomega

/-! ## The formulas -/

/-- Add: `(result word, expression flag)`. -/
def topAdd (w1 w2 : Nat) : Nat × Nat :=
  let t := w1 ^^^ w2
  let s := sar63 t
  let L := sar63 (((w1 &&& M) + (W64 - (w2 &&& M))) % W64)
  let W := w1 ^^^ (t &&& L)
  let V := W ^^^ t
  let mn := V &&& M
  ((W + (mn ^^^ s) + (W64 - s)) % W64, (t >>> 62) &&& nzb mn &&& 1)

def topSub (w1 w2 : Nat) : Nat × Nat := topAdd w1 (w2 ^^^ SB)
def topNeg (w : Nat) : Nat := w ^^^ SB
def topMul (w1 w2 : Nat) : Nat :=
  let a := w1 &&& M
  let b := w2 &&& M
  ((a * b) % W64) ||| (((w1 ^^^ a) + (w2 ^^^ b)) % W64)
def topMulLea (w1 w2 : Nat) : Nat :=
  let a := w1 &&& M
  let b := w2 &&& M
  ((a * b) % W64) ||| (((w1 + w2 + (W64 - 1)) % W64) &&& NM)
def topDiv (w1 w2 : Nat) : Nat :=
  let a := w1 &&& M
  let b := w2 &&& M
  (a / b) ||| (((w1 + (W64 - w2)) % W64) &&& NM)

/-! ## Structural facts about encoded words -/

theorem M_eq : M = 2 ^ 62 - 1 := rfl
theorem NM_eq : NM = 2 ^ 64 - 2 ^ 62 := rfl

theorem enc_and_M (m : Nat) (d : Fin 4) (h : m < Q) : topEnc m d &&& M = m := by
  rw [M_eq, Nat.and_two_pow_sub_one_eq_mod]; simp only [topEnc]; qomega

theorem enc_shr (m : Nat) (d : Fin 4) (h : m < Q) : topEnc m d >>> 62 = d.val := by
  rw [Nat.shiftRight_eq_div_pow]; simp only [topEnc]; have := d.isLt; qomega

theorem enc_lt' (m : Nat) (d : Fin 4) (h : m < Q) : topEnc m d < 2 ^ 64 := by
  simp only [topEnc]; have := d.isLt; qomega

/-- Decoding a word given by its parts. -/
theorem topDec_parts (c m : Nat) (hc : c < 4) (hm : m < Q) : topDec (c * Q + m) = ⟨m, Fin.ofNat 4 c⟩ := by
  simp only [topDec]
  have h1 : (c * Q + m) % Q = m := by qomega
  have h2 : (c * Q + m) / Q = c := by qomega
  rw [h1, h2]

/-- The sign mask of `t = w1 xor w2`: all ones exactly when the sign bits differ. -/
theorem sar_t (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) :
    sar63 (topEnc m1 d1 ^^^ topEnc m2 d2) = if d1.val ^^^ d2.val < 2 then 0 else 2 ^ 64 - 1 := by
  have e : (topEnc m1 d1 ^^^ topEnc m2 d2) >>> 62 = d1.val ^^^ d2.val := by
    rw [Nat.shiftRight_xor_distrib, enc_shr _ _ h1, enc_shr _ _ h2]
  rw [Nat.shiftRight_eq_div_pow] at e
  unfold sar63
  have : (topEnc m1 d1 ^^^ topEnc m2 d2) < 2 ^ 63 ↔ d1.val ^^^ d2.val < 2 := by
    rw [← e]; qomega
  simp only [this]

theorem sar_L (m1 m2 : Nat) (h1 : m1 < Q) (h2 : m2 < Q) :
    sar63 ((m1 + (W64 - m2)) % W64) = if m1 < m2 then 2 ^ 64 - 1 else 0 := by
  unfold sar63; by_cases h : m1 < m2
  · have : ¬ (m1 + (W64 - m2)) % W64 < 2 ^ 63 := by simp only [W64] at *; qomega
    simp [h, this]
  · have : (m1 + (W64 - m2)) % W64 < 2 ^ 63 := by simp only [W64] at *; qomega
    simp [h, this]

theorem t_lt (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) :
    topEnc m1 d1 ^^^ topEnc m2 d2 < 2 ^ 64 := Nat.xor_lt_two_pow (enc_lt' _ _ h1) (enc_lt' _ _ h2)

/-- The add, evaluated on encoded words: the larger word plus or minus the smaller magnitude. -/
theorem topAdd_eval (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) :
    topAdd (topEnc m1 d1) (topEnc m2 d2) =
      (let W := if m1 < m2 then topEnc m2 d2 else topEnc m1 d1
       let mn := if m1 < m2 then m1 else m2
       let s := if d1.val ^^^ d2.val < 2 then 0 else 2 ^ 64 - 1
       ((W + (mn ^^^ s) + (W64 - s)) % W64, (d1.val ^^^ d2.val) &&& nzb mn &&& 1)) := by
  simp only [topAdd, enc_and_M _ _ h1, enc_and_M _ _ h2, sar_L _ _ h1 h2, sar_t _ _ _ _ h1 h2,
    Nat.shiftRight_xor_distrib, enc_shr _ _ h1, enc_shr _ _ h2]
  by_cases h : m1 < m2
  · simp only [h, ite_true, and_ones64 (t_lt _ _ _ _ h1 h2), xor_xor_cancel, xor_xor_cancel', enc_and_M _ _ h1]
  · simp only [h, ite_false, Nat.and_zero, Nat.xor_zero, enc_and_M _ _ h2]
    rw [xor_xor_cancel]
    simp only [enc_and_M _ _ h2]

/-! ## The add is the reference add -/

theorem topAdd_eq_0 (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) (hk : d1 = 0) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  rw [topAdd_eval _ _ _ _ h1 h2]
  subst hk
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; qomega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb, topEnc, topDec,
    xor_ones64 (by qomega : m1 < 2 ^ 64), xor_ones64 (by qomega : m2 < 2 ^ 64), Fin.ext_iff, Fin.val_ofNat]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

theorem topAdd_eq_1 (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) (hk : d1 = 1) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  rw [topAdd_eval _ _ _ _ h1 h2]
  subst hk
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; qomega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb, topEnc, topDec,
    xor_ones64 (by qomega : m1 < 2 ^ 64), xor_ones64 (by qomega : m2 < 2 ^ 64), Fin.ext_iff, Fin.val_ofNat]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

theorem topAdd_eq_2 (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) (hk : d1 = 2) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  rw [topAdd_eval _ _ _ _ h1 h2]
  subst hk
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; qomega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb, topEnc, topDec,
    xor_ones64 (by qomega : m1 < 2 ^ 64), xor_ones64 (by qomega : m2 < 2 ^ 64), Fin.ext_iff, Fin.val_ofNat]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

theorem topAdd_eq_3 (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) (hk : d1 = 3) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  rw [topAdd_eval _ _ _ _ h1 h2]
  subst hk
  rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; qomega)
  all_goals simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb, topEnc, topDec,
    xor_ones64 (by qomega : m1 < 2 ^ 64), xor_ones64 (by qomega : m2 < 2 ^ 64), Fin.ext_iff, Fin.val_ofNat]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

/-- **Top add.** For magnitudes below `2^62` with no overflow: when round 1's `A.add` gives `r`, the word decodes to
`r`'s magnitude, and to `r`'s direction unless `r` is zero, and the flag is 0; when `A.add` gives an expression, the
flag is 1. -/
theorem topAdd_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topAdd (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topAdd (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl
  · exact topAdd_eq_0 m1 m2 _ d2 h1 h2 hs rfl
  · exact topAdd_eq_1 m1 m2 _ d2 h1 h2 hs rfl
  · exact topAdd_eq_2 m1 m2 _ d2 h1 h2 hs rfl
  · exact topAdd_eq_3 m1 m2 _ d2 h1 h2 hs rfl

/-! ## Negate and subtract -/

theorem enc_comm (m : Nat) (d : Fin 4) : topEnc m d = 2 ^ 62 * d.val + m := by simp only [topEnc]; qomega

theorem xor2 : ∀ d : Fin 4, d.val ^^^ 2 = (d + 2).val := by decide

/-- Flipping the sign bit turns the direction by 2. -/
theorem enc_flip (m : Nat) (d : Fin 4) (h : m < Q) : topEnc m d ^^^ SB = topEnc m (d + 2) := by
  rw [enc_comm, enc_comm, show SB = 2 ^ 62 * 2 + 0 from rfl, xor_parts (by qomega) (by decide), Nat.xor_zero, xor2]

/-- **Top negate** is the reference negate: round 1's `A.neg ⟨m, d⟩ = ⟨m, d + 2⟩` (by definition), encoded. -/
theorem topNeg_eq (m : Nat) (d : Fin 4) (h : m < Q) : topNeg (topEnc m d) = topEnc m (d + 2) := by
  rw [topNeg, enc_flip m d h]

theorem neg_def (m : Nat) (d : Fin 4) : neg ⟨m, d⟩ = ⟨m, d + 2⟩ := rfl

/-- **Top subtract**: the add with the second word's sign flipped, against round 1's `A.sub`. -/
theorem topSub_eq (m1 m2 : Nat) (d1 d2 : Fin 4) (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match sub Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (topSub (topEnc m1 d1) (topEnc m2 d2)).2 = 0 ∧
        (topDec (topSub (topEnc m1 d1) (topEnc m2 d2)).1).m = r.m ∧
        ((topDec (topSub (topEnc m1 d1) (topEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (topSub (topEnc m1 d1) (topEnc m2 d2)).2 = 1 := by
  simp only [topSub, enc_flip m2 d2 h2, sub, neg]
  exact topAdd_eq m1 m2 d1 (d2 + 2) h1 h2 hs

end Packed
end Header
