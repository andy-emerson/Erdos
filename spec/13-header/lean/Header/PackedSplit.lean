import Header.Packed
/-!
# Packed form, split layout

`w = s · 2^63 + 2m + a`: the sign `s` in bit 63, the axis `a` in bit 0, the magnitude in bits 1–62; the direction is
`d = 2s + a`. The formulas are those of `../prior-art.md`, "Packed, split layout", and of
`../test/prior-art/split.py` (class `Word`, `R = 64`), on natural numbers mod `2^64`. Each formula is one definition
below; where `split.py` has two variants (the add's compare `L`, the multiply's and the divide's direction part `D`,
the divide's quotient) each variant is one definition, and the proofs go through a shared body (`spAddWith`,
`spMulWith`, `spDivWith`) that takes the variant as a parameter and asks only for its one fact.
-/

namespace Header
namespace Packed
open A

/-- `F = 2^63 − 2`, the magnitude field (bits 1–62). -/
abbrev SF : Nat := 9223372036854775806
/-- `K = 2^63 + 1`, the sign and axis bits. -/
abbrev SK : Nat := 9223372036854775809

/-- A split word from its three fields: sign `s` (bit 63), magnitude `m` (bits 1–62), axis `a` (bit 0). -/
def T3 (s m a : Nat) : Nat := 2 ^ 63 * s + (2 ^ 1 * m + a)

def spEnc (m : Nat) (d : Fin 4) : Nat := T3 (d.val / 2) m (d.val % 2)
/-- `split.py`'s `dec_split`: `((w >> 1) & MAX, 2 (w >> 63) + (w & 1))`. -/
def spDec (w : Nat) : Num := ⟨w / 2 % Q, Fin.ofNat 4 (2 * (w / 2 ^ 63) + w % 2)⟩

theorem T3_dec {s m a : Nat} (_hs : s < 2) (hm : m < Q) (ha : a < 2) :
    spDec (T3 s m a) = ⟨m, Fin.ofNat 4 (2 * s + a)⟩ := by
  simp only [spDec, T3]
  have e1 : (2 ^ 63 * s + (2 ^ 1 * m + a)) / 2 % Q = m := by qomega
  have e2 : (2 ^ 63 * s + (2 ^ 1 * m + a)) / 2 ^ 63 = s := by qomega
  have e3 : (2 ^ 63 * s + (2 ^ 1 * m + a)) % 2 = a := by qomega
  rw [e1, e2, e3]

theorem spEnc_dec (m : Nat) (d : Fin 4) (h : m < Q) : spDec (spEnc m d) = ⟨m, d⟩ := by
  have := d.isLt
  rw [spEnc, T3_dec (by omega) h (by omega), Nat.div_add_mod, fin4_ofNat_val]

theorem T3_lt {s m a : Nat} (hs : s < 2) (hm : m < Q) (ha : a < 2) : T3 s m a < 2 ^ 64 := by
  simp only [T3]; qomega

def Split : Layout where
  enc := spEnc
  dec := spDec
  dec_enc m d h := spEnc_dec m d h
  enc_lt m d h := by have := d.isLt; exact T3_lt (by omega) h (by omega)

/-! ## The formulas -/

/-- The add's body, with the compare `L` (all ones when `w2` is the larger) as a parameter `Lf w1 w2 t`. -/
def spAddWith (Lf : Nat → Nat → Nat → Nat) (w1 w2 : Nat) : Nat × Nat :=
  let t := w1 ^^^ w2
  let s := sar63 t
  let L := Lf w1 w2 t
  let W := w1 ^^^ (t &&& L)
  let v := (W ^^^ t) &&& SF
  ((W + (v ^^^ s) + (W64 - s)) % W64, t &&& 1 &&& nzb v)

/-- `L = 0 − ((w1 shl 1) <u (w2 shl 1))` (`lvariant = 0`). -/
def spL0 (w1 w2 _t : Nat) : Nat := (W64 - ltb ((w1 <<< 1) % W64) ((w2 <<< 1) % W64)) % W64
/-- `L = 0 − ((w1 xor (t and S)) <u w2)` (`lvariant = 1`). -/
def spL1 (w1 w2 t : Nat) : Nat := (W64 - ltb (w1 ^^^ (t &&& SB)) w2) % W64

/-- Add: `(result word, expression flag)`. -/
def spAdd : Nat → Nat → Nat × Nat := spAddWith spL0
def spAdd1 : Nat → Nat → Nat × Nat := spAddWith spL1
def spSub (w1 w2 : Nat) : Nat × Nat := spAdd w1 (w2 ^^^ SB)
def spSub1 (w1 w2 : Nat) : Nat × Nat := spAdd1 w1 (w2 ^^^ SB)
def spNeg (w : Nat) : Nat := w ^^^ SB

/-- The multiply's body, with the direction part as a parameter `Df w1 w2 x2`. -/
def spMulWith (Df : Nat → Nat → Nat → Nat) (w1 w2 : Nat) : Nat :=
  let x1 := w1 &&& SF
  let x2 := w2 &&& SF
  (((x1 * x2) % W64) >>> 1) ||| Df w1 w2 x2

/-- `D = ((w1 or F) + (w2 xor x2)) and K` (`variant = 0`). -/
def spMulD0 (w1 w2 x2 : Nat) : Nat := ((w1 ||| SF) + (w2 ^^^ x2)) % W64 &&& SK
/-- `D = ((w1 xor w2) xor ((w1 and w2) shl 63)) and K` (`variant = 1`). -/
def spMulD1 (w1 w2 _x2 : Nat) : Nat := ((w1 ^^^ w2) ^^^ (((w1 &&& w2) <<< 63) % W64)) &&& SK

def spMul : Nat → Nat → Nat := spMulWith spMulD0
def spMul1 : Nat → Nat → Nat := spMulWith spMulD1

/-- The exact divide's body, with the quotient part `qf x1 x2` and the direction part `Df w1 w2 x1 x2` as
parameters. -/
def spDivWith (qf : Nat → Nat → Nat) (Df : Nat → Nat → Nat → Nat → Nat) (w1 w2 : Nat) : Nat :=
  let x1 := w1 &&& SF
  let x2 := w2 &&& SF
  qf x1 x2 ||| Df w1 w2 x1 x2

/-- `q = (x1 /u x2) shl 1` (`variant = 0`). -/
def spDivQ0 (x1 x2 : Nat) : Nat := ((x1 / x2) <<< 1) % W64
/-- `D = ((w1 xor x1) − (w2 xor x2)) and K` (`variant = 0`). -/
def spDivD0 (w1 w2 x1 x2 : Nat) : Nat := ((w1 ^^^ x1) + (W64 - (w2 ^^^ x2))) % W64 &&& SK
/-- `q = x1 /u (x2 shr 1)` (`variant = 1`). -/
def spDivQ1 (x1 x2 : Nat) : Nat := x1 / (x2 >>> 1)
/-- `D = ((w1 xor w2) xor ((w2 and not w1) shl 63)) and K` (`variant = 1`). -/
def spDivD1 (w1 w2 _x1 _x2 : Nat) : Nat :=
  ((w1 ^^^ w2) ^^^ (((w2 &&& (w1 ^^^ (2 ^ 64 - 1))) <<< 63) % W64)) &&& SK

def spDiv : Nat → Nat → Nat := spDivWith spDivQ0 spDivD0
def spDiv1 : Nat → Nat → Nat := spDivWith spDivQ1 spDivD1

/-! ## Three-field arithmetic -/

section
variable {s1 s2 m1 m2 a1 a2 : Nat}

theorem low_lt {m a : Nat} (hm : m < Q) (ha : a < 2) : 2 ^ 1 * m + a < 2 ^ 63 := by qomega
theorem a_lt {a : Nat} (ha : a < 2) : a < 2 ^ 1 := ha

theorem T3_xor (hm1 : m1 < Q) (hm2 : m2 < Q) (ha1 : a1 < 2) (ha2 : a2 < 2) :
    T3 s1 m1 a1 ^^^ T3 s2 m2 a2 = T3 (s1 ^^^ s2) (m1 ^^^ m2) (a1 ^^^ a2) := by
  unfold T3; rw [xor_parts (low_lt hm1 ha1) (low_lt hm2 ha2), xor_parts (a_lt ha1) (a_lt ha2)]

theorem T3_and (hm1 : m1 < Q) (hm2 : m2 < Q) (ha1 : a1 < 2) (ha2 : a2 < 2) :
    T3 s1 m1 a1 &&& T3 s2 m2 a2 = T3 (s1 &&& s2) (m1 &&& m2) (a1 &&& a2) := by
  unfold T3; rw [and_parts (low_lt hm1 ha1) (low_lt hm2 ha2), and_parts (a_lt ha1) (a_lt ha2)]

theorem T3_or (hm1 : m1 < Q) (hm2 : m2 < Q) (ha1 : a1 < 2) (ha2 : a2 < 2) :
    T3 s1 m1 a1 ||| T3 s2 m2 a2 = T3 (s1 ||| s2) (m1 ||| m2) (a1 ||| a2) := by
  unfold T3; rw [or_parts (low_lt hm1 ha1) (low_lt hm2 ha2), or_parts (a_lt ha1) (a_lt ha2)]

end

theorem T3_mod2 (s m a : Nat) : T3 s m a % 2 = a % 2 := by simp only [T3]; omega

theorem SB_T3 : SB = T3 1 0 0 := by decide
theorem SF_T3 : SF = T3 0 (2 ^ 62 - 1) 0 := by decide
theorem SK_T3 : SK = T3 1 0 1 := by decide
theorem ones_T3 : 2 ^ 64 - 1 = T3 1 (2 ^ 62 - 1) 1 := by decide

theorem M62_lt : 2 ^ 62 - 1 < Q := by decide

/-- `shl 63` on a word keeps bit 0, moved to bit 63. -/
theorem shl63 (x : Nat) : (x <<< 63) % W64 = T3 (x % 2) 0 0 := by
  rw [Nat.shiftLeft_eq, T3, show W64 = 2 ^ 63 * 2 from rfl, Nat.mul_comm x, Nat.mul_mod_mul_left]; rfl

section
variable {m m1 m2 : Nat} {d d1 d2 : Fin 4}

theorem dlt (d : Fin 4) : d.val / 2 < 2 := by have := d.isLt; omega
theorem alt (d : Fin 4) : d.val % 2 < 2 := Nat.mod_lt _ (by decide)

theorem sp_lt (h : m < Q) : spEnc m d < 2 ^ 64 := T3_lt (dlt d) h (alt d)

theorem sp_xor (h1 : m1 < Q) (h2 : m2 < Q) :
    spEnc m1 d1 ^^^ spEnc m2 d2 = T3 (d1.val / 2 ^^^ d2.val / 2) (m1 ^^^ m2) (d1.val % 2 ^^^ d2.val % 2) :=
  T3_xor h1 h2 (alt d1) (alt d2)

/-- `w and F = 2m`. -/
theorem sp_and_F (h : m < Q) : spEnc m d &&& SF = 2 * m := by
  rw [spEnc, SF_T3, T3_and h M62_lt (alt d) (by decide), Nat.and_zero, Nat.and_zero,
    Nat.and_two_pow_sub_one_eq_mod, Nat.mod_eq_of_lt (by qomega)]
  simp only [T3]; omega

end

theorem xm_lt {m1 m2 : Nat} (h1 : m1 < Q) (h2 : m2 < Q) : m1 ^^^ m2 < Q :=
  Nat.xor_lt_two_pow (n := 62) h1 h2

theorem axor_lt : ∀ d1 d2 : Fin 4, d1.val % 2 ^^^ d2.val % 2 < 2 := by decide
theorem sxor : ∀ d1 d2 : Fin 4, d1.val / 2 ^^^ d2.val / 2 = if d1.val / 2 = d2.val / 2 then 0 else 1 := by decide

theorem fv0 : ((0 : Fin 4) : Nat) = 0 := rfl
theorem fv1 : ((1 : Fin 4) : Nat) = 1 := rfl
theorem fv2 : ((2 : Fin 4) : Nat) = 2 := rfl
theorem fv3 : ((3 : Fin 4) : Nat) = 3 := rfl

theorem two_T3 (m : Nat) : 2 * m = T3 0 m 0 := by simp only [T3]; omega

/-! ## Add and subtract -/

section
variable {m1 m2 : Nat} {d1 d2 : Fin 4}

/-- `t sar 63` is all ones exactly when the signs differ. -/
theorem sp_sar (h1 : m1 < Q) (h2 : m2 < Q) :
    sar63 (spEnc m1 d1 ^^^ spEnc m2 d2) = if d1.val / 2 = d2.val / 2 then 0 else 2 ^ 64 - 1 := by
  rw [sp_xor h1 h2, sxor]
  have := xm_lt h1 h2; have := axor_lt d1 d2
  unfold sar63 T3
  by_cases h : d1.val / 2 = d2.val / 2 <;> simp only [h, ↓reduceIte] <;> split <;> qomega

theorem sp_t_lt (h1 : m1 < Q) (h2 : m2 < Q) : spEnc m1 d1 ^^^ spEnc m2 d2 < 2 ^ 64 :=
  Nat.xor_lt_two_pow (sp_lt h1) (sp_lt h2)

/-- `t and 1` is the axis check. -/
theorem sp_t1 (h1 : m1 < Q) (h2 : m2 < Q) : (spEnc m1 d1 ^^^ spEnc m2 d2) &&& 1 = d1.val % 2 ^^^ d2.val % 2 := by
  rw [Nat.and_one_is_mod, sp_xor h1 h2, T3_mod2, Nat.mod_eq_of_lt (axor_lt d1 d2)]

theorem sp_shl1 {m : Nat} {d : Fin 4} (h : m < Q) : (spEnc m d <<< 1) % W64 = 4 * m + 2 * (d.val % 2) := by
  rw [Nat.shiftLeft_eq, spEnc, T3]; have := dlt d; have := alt d; qomega

/-- The first compare: all ones when `w2`'s magnitude-and-axis is the larger. -/
theorem spL0_eq (h1 : m1 < Q) (h2 : m2 < Q) (t : Nat) :
    spL0 (spEnc m1 d1) (spEnc m2 d2) t =
      if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then 2 ^ 64 - 1 else 0 := by
  rw [spL0, sp_shl1 h1, sp_shl1 h2, ltb]
  have := alt d1; have := alt d2
  by_cases h : 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2
  · have : 4 * m1 + 2 * (d1.val % 2) < 4 * m2 + 2 * (d2.val % 2) := by omega
    simp only [h, this, ↓reduceIte]; decide
  · have : ¬ 4 * m1 + 2 * (d1.val % 2) < 4 * m2 + 2 * (d2.val % 2) := by omega
    simp only [h, this, ↓reduceIte]; decide

theorem sflip : ∀ d1 d2 : Fin 4, d1.val / 2 ^^^ ((d1.val / 2 ^^^ d2.val / 2) &&& 1) = d2.val / 2 := by decide

/-- The second compare: `w1` with `w2`'s sign, then an unsigned compare. -/
theorem spL1_eq (h1 : m1 < Q) (h2 : m2 < Q) :
    spL1 (spEnc m1 d1) (spEnc m2 d2) (spEnc m1 d1 ^^^ spEnc m2 d2) =
      if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then 2 ^ 64 - 1 else 0 := by
  have hx := xm_lt h1 h2; have ha := axor_lt d1 d2
  rw [spL1, sp_xor h1 h2, SB_T3, T3_and hx (by decide) ha (by decide), Nat.and_zero, Nat.and_zero, spEnc,
    T3_xor h1 (by decide) (alt d1) (by decide), Nat.xor_zero, Nat.xor_zero, sflip, ltb]
  have : T3 (d2.val / 2) m1 (d1.val % 2) < spEnc m2 d2 ↔ 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 := by
    simp only [spEnc, T3]; omega
  by_cases h : 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2
  · simp only [h, this.mpr h, ↓reduceIte]; decide
  · simp only [h, mt this.mp h, ↓reduceIte]; decide

/-- `W ± v`: the sum on the same sign, the difference otherwise. -/
theorem sp_addsub {mW mv : Nat} {dW : Fin 4} (p : Prop) [Decidable p] (hle : mv ≤ mW) (hs : mW + mv < Q) :
    (spEnc mW dW + ((2 * mv) ^^^ (if p then 0 else 2 ^ 64 - 1)) + (W64 - (if p then 0 else 2 ^ 64 - 1))) % W64 =
      if p then spEnc (mW + mv) dW else spEnc (mW - mv) dW := by
  have := dlt dW; have := alt dW
  by_cases h : p
  · simp only [h, ↓reduceIte, Nat.xor_zero, spEnc, T3]; qomega
  · simp only [h, ↓reduceIte, xor_ones64 (by qomega : 2 * mv < 2 ^ 64), spEnc, T3]; qomega

variable (Lf : Nat → Nat → Nat → Nat)

/-- The add body on two encoded words, for any compare `L` with the compare's fact. -/
theorem spAddWith_eval (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q)
    (hL : Lf (spEnc m1 d1) (spEnc m2 d2) (spEnc m1 d1 ^^^ spEnc m2 d2) =
      if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then 2 ^ 64 - 1 else 0) :
    spAddWith Lf (spEnc m1 d1) (spEnc m2 d2) =
      (if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then
          (if d1.val / 2 = d2.val / 2 then spEnc (m2 + m1) d2 else spEnc (m2 - m1) d2)
        else (if d1.val / 2 = d2.val / 2 then spEnc (m1 + m2) d1 else spEnc (m1 - m2) d1),
       (d1.val % 2 ^^^ d2.val % 2) &&&
         nzb (2 * if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then m1 else m2)) := by
  simp only [spAddWith, hL, sp_sar h1 h2]
  have := alt d1; have := alt d2
  by_cases hc : 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2
  · simp only [hc, ↓reduceIte, and_ones64 (sp_t_lt h1 h2), xor_xor_cancel, xor_xor_cancel', sp_and_F h1,
      sp_t1 h1 h2]
    rw [sp_addsub _ (by omega) (by omega)]
  · simp only [hc, ↓reduceIte, Nat.and_zero, Nat.xor_zero]
    rw [xor_xor_cancel, sp_and_F h2, sp_t1 h1 h2, sp_addsub _ (by omega) (by omega)]

/-- The add body, decoded. -/
theorem spAddWith_dec (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q)
    (hL : Lf (spEnc m1 d1) (spEnc m2 d2) (spEnc m1 d1 ^^^ spEnc m2 d2) =
      if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then 2 ^ 64 - 1 else 0) :
    spDec (spAddWith Lf (spEnc m1 d1) (spEnc m2 d2)).1 =
      (if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then
          (if d1.val / 2 = d2.val / 2 then ⟨m2 + m1, d2⟩ else ⟨m2 - m1, d2⟩)
        else (if d1.val / 2 = d2.val / 2 then ⟨m1 + m2, d1⟩ else ⟨m1 - m2, d1⟩)) := by
  rw [spAddWith_eval Lf h1 h2 hs hL]
  by_cases ha : 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 <;> by_cases hb : d1.val / 2 = d2.val / 2 <;>
    simp only [ha, hb, ↓reduceIte] <;> exact spEnc_dec _ _ (by omega)

/-- **Split add**, for either compare: with no overflow (`m1 + m2 < 2^62`), the flag is 0 and the word decodes to
round 1's sum (its magnitude, and its direction unless the sum is zero) whenever round 1 gives a number, and the flag
is 1 when round 1 reports an expression. -/
theorem spAddWith_eq (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q)
    (hL : Lf (spEnc m1 d1) (spEnc m2 d2) (spEnc m1 d1 ^^^ spEnc m2 d2) =
      if 2 * m1 + d1.val % 2 < 2 * m2 + d2.val % 2 then 2 ^ 64 - 1 else 0) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (spAddWith Lf (spEnc m1 d1) (spEnc m2 d2)).2 = 0 ∧
        (spDec (spAddWith Lf (spEnc m1 d1) (spEnc m2 d2)).1).m = r.m ∧
        ((spDec (spAddWith Lf (spEnc m1 d1) (spEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (spAddWith Lf (spEnc m1 d1) (spEnc m2 d2)).2 = 1 := by
  have hd := spAddWith_dec Lf h1 h2 hs hL
  have hf := congrArg Prod.snd (spAddWith_eval Lf h1 h2 hs hL)
  simp only at hf
  rw [hd, hf]
  clear hd hf hL
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases he : m1 = m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals set_option maxRecDepth 20000 in simp [add, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, nzb,
    Fin.ext_iff]
  all_goals (repeat' split) <;> (try simp only [Nat.zero_xor, Nat.xor_zero] at *) <;> qomega

end

/-- **Split add** (`lvariant = 0`). -/
theorem spAdd_eq {m1 m2 : Nat} {d1 d2 : Fin 4} (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (spAdd (spEnc m1 d1) (spEnc m2 d2)).2 = 0 ∧
        (spDec (spAdd (spEnc m1 d1) (spEnc m2 d2)).1).m = r.m ∧
        ((spDec (spAdd (spEnc m1 d1) (spEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (spAdd (spEnc m1 d1) (spEnc m2 d2)).2 = 1 :=
  spAddWith_eq spL0 h1 h2 hs (spL0_eq h1 h2 _)

/-- **Split add** (`lvariant = 1`). -/
theorem spAdd1_eq {m1 m2 : Nat} {d1 d2 : Fin 4} (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match add Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (spAdd1 (spEnc m1 d1) (spEnc m2 d2)).2 = 0 ∧
        (spDec (spAdd1 (spEnc m1 d1) (spEnc m2 d2)).1).m = r.m ∧
        ((spDec (spAdd1 (spEnc m1 d1) (spEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (spAdd1 (spEnc m1 d1) (spEnc m2 d2)).2 = 1 :=
  spAddWith_eq spL1 h1 h2 hs (spL1_eq h1 h2)

theorem sp_flip_bits : ∀ d : Fin 4, d.val / 2 ^^^ 1 = (d + 2).val / 2 ∧ d.val % 2 = (d + 2).val % 2 := by decide

/-- Flipping bit 63 turns the direction by 2. -/
theorem sp_flip {m : Nat} {d : Fin 4} (h : m < Q) : spEnc m d ^^^ SB = spEnc m (d + 2) := by
  rw [spEnc, spEnc, SB_T3, T3_xor h (by decide) (alt d) (by decide), Nat.xor_zero, Nat.xor_zero,
    (sp_flip_bits d).1, (sp_flip_bits d).2]

/-- **Split negate** gives the word of round 1's `A.neg` exactly. -/
theorem spNeg_eq {m : Nat} {d : Fin 4} (h : m < Q) :
    spNeg (spEnc m d) = spEnc (neg ⟨m, d⟩).m (neg ⟨m, d⟩).d := by
  rw [spNeg, sp_flip h]; rfl

/-- **Split subtract** (`lvariant = 0`), as the add. -/
theorem spSub_eq {m1 m2 : Nat} {d1 d2 : Fin 4} (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match sub Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (spSub (spEnc m1 d1) (spEnc m2 d2)).2 = 0 ∧
        (spDec (spSub (spEnc m1 d1) (spEnc m2 d2)).1).m = r.m ∧
        ((spDec (spSub (spEnc m1 d1) (spEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (spSub (spEnc m1 d1) (spEnc m2 d2)).2 = 1 := by
  simp only [spSub, sp_flip h2, sub, neg]
  exact spAdd_eq h1 h2 hs

/-- **Split subtract** (`lvariant = 1`). -/
theorem spSub1_eq {m1 m2 : Nat} {d1 d2 : Fin 4} (h1 : m1 < Q) (h2 : m2 < Q) (hs : m1 + m2 < Q) :
    match sub Q ⟨m1, d1⟩ ⟨m2, d2⟩ with
    | some r => (spSub1 (spEnc m1 d1) (spEnc m2 d2)).2 = 0 ∧
        (spDec (spSub1 (spEnc m1 d1) (spEnc m2 d2)).1).m = r.m ∧
        ((spDec (spSub1 (spEnc m1 d1) (spEnc m2 d2)).1).d = r.d ∨ r.m = 0)
    | none => (spSub1 (spEnc m1 d1) (spEnc m2 d2)).2 = 1 := by
  simp only [spSub1, sp_flip h2, sub, neg]
  exact spAdd1_eq h1 h2 hs

end Packed
end Header
