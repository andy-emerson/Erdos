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

theorem T3_dec {s m a : Nat} (hs : s < 2) (hm : m < Q) (ha : a < 2) :
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

end Packed
end Header
