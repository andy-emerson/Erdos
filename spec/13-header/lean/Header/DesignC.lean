import Header.DesignA
/-!
# Design C: an axis and a signed value, on 64-bit words

Spec 13 v0.2 §4a and the notes' "Design C". A number is one 64-bit word `w = 2v + a`: the axis `a` in bit 0
(0 real, 1 imaginary) and a signed value `v` in bits 1–63, two's complement, so `-2^62 ≤ v < 2^62`; its value is
`v · i^a`.

**The machine.** A word is `BitVec 64`, Lean core's model of a 64-bit register: `+`, `-`, `*` and unary `-` wrap
mod `2^64`; `&&&`, `|||`, `^^^`, `~~~` are AND, OR, XOR, NOT; `<<<` is the shift left; `sshiftRight` is the
arithmetic shift right (`sar`); `sdiv` is the signed divide, truncating toward zero (`idiv`, ARM64 `sdiv`,
RISC-V `div`); `<` on `BitVec` is the **unsigned** compare. `toInt` reads a word signed (two's complement),
`toNat` unsigned, and `BitVec.ofInt 64 z` stores the integer `z` mod `2^64`.

The rules are transcribed from spec §4a. `none` is "an error (an expression)". `div` is the current division rule,
whose divisor is `w2 and not 1`; `divOld`, the first rule (divisor `w2 sar 1`), is kept for finding C-a. Section 7
proves the reference facts for Control (the machine's signed negate and divide).
-/

set_option linter.unusedVariables false

namespace Header
namespace C

/-- A 64-bit word. -/
abbrev Word := BitVec 64

/-- The axis bit as an integer: 0 for the real axis, 1 for the imaginary axis. -/
def abit (a : Bool) : Int := if a then 1 else 0

/-- `i^a` as a direction of `Fin 4`: `a = 0` is 1, `a = 1` is `i`. -/
def adir (a : Bool) : Fin 4 := if a then 1 else 0

/-- A Design C number: a signed value `v` and an axis `a`. -/
structure Num where
  v : Int
  a : Bool
deriving DecidableEq, Repr

/-- The value `v · i^a`. -/
def Num.val (x : Num) : GI := GI.smul x.v (unit (adir x.a))

/-- The range of the signed value: 63 bits, two's complement. -/
def InRange (v : Int) : Prop := -2^62 ≤ v ∧ v < 2^62

instance (v : Int) : Decidable (InRange v) := by unfold InRange; infer_instance

/-! ## Encoding and decoding -/

/-- Encoding: `w = 2v + a`, stored mod `2^64`. -/
def enc (x : Num) : Word := BitVec.ofInt 64 (2 * x.v + abit x.a)

/-- Decoding: `v` is the word shifted right arithmetically by one (`w sar 1`, read signed); `a` is `w and 1`. -/
def dec (w : Word) : Num := ⟨(w.sshiftRight 1).toInt, decide (w &&& 1#64 = 1#64)⟩

/-- The value of a word. -/
def wval (w : Word) : GI := (dec w).val

/-! ### Machine facts -/

theorem toInt_bounds (w : Word) : -2^63 ≤ w.toInt ∧ w.toInt < 2^63 := by
  have h1 := BitVec.le_toInt w
  have h2 := @BitVec.toInt_lt 64 w
  exact ⟨by simpa using h1, by simpa using h2⟩

theorem toInt_toNat (w : Word) :
    w.toInt = if 2 * w.toNat < 2^64 then (w.toNat : Int) else (w.toNat : Int) - 2^64 := by
  rw [BitVec.toInt_eq_toNat_cond]; simp

theorem and_one_toNat (w : Word) : (w &&& 1#64).toNat = w.toNat % 2 := by
  rw [BitVec.toNat_and]; simp [Nat.and_one_is_mod]

/-- `sar 1` is the signed reading halved, rounding down. -/
theorem sar_toInt (w : Word) : (w.sshiftRight 1).toInt = w.toInt / 2 := by
  rw [BitVec.toInt_sshiftRight, Int.shiftRight_eq_div_pow]; rfl

theorem abit_cases (a : Bool) : abit a = 0 ∨ abit a = 1 := by cases a <;> simp [abit]

theorem dec_a_iff (w : Word) : (dec w).a = true ↔ w.toNat % 2 = 1 := by
  simp only [dec, decide_eq_true_eq]
  constructor
  · intro h; have := congrArg BitVec.toNat h; rw [and_one_toNat] at this; simpa using this
  · intro h; apply BitVec.eq_of_toNat_eq; rw [and_one_toNat, h]; rfl

theorem abit_dec (w : Word) : abit (dec w).a = (w.toNat % 2 : Nat) := by
  by_cases h : w.toNat % 2 = 1
  · rw [(dec_a_iff w).2 h, h]; rfl
  · have : (dec w).a = false := by
      cases hh : (dec w).a
      · rfl
      · exact absurd ((dec_a_iff w).1 hh) h
    have h0 : w.toNat % 2 = 0 := by omega
    rw [this, h0]; rfl

/-- **The split.** Every word reads, signed, as `2v + a` with `v` its decoded value and `a` its decoded axis. -/
theorem toInt_split (w : Word) : w.toInt = 2 * (dec w).v + abit (dec w).a := by
  rw [abit_dec]; simp only [dec]; rw [sar_toInt, toInt_toNat]
  have := w.isLt
  split <;> omega

/-- Every word decodes to a value in range. -/
theorem dec_range (w : Word) : InRange (dec w).v := by
  have h1 := toInt_split w; have h2 := toInt_bounds w; have h3 := abit_cases (dec w).a
  unfold InRange; omega

/-- A word whose signed reading is `2v + a`, with `v` in range, decodes to `(v, a)`. -/
theorem dec_of_toInt {w : Word} {x : Num} (hx : InRange x.v) (h : w.toInt = 2 * x.v + abit x.a) :
    dec w = x := by
  have h1 := toInt_split w; have h2 := dec_range w
  have h3 := abit_cases (dec w).a; have h4 := abit_cases x.a
  unfold InRange at hx h2
  have hv : (dec w).v = x.v := by omega
  have ha : abit (dec w).a = abit x.a := by omega
  have ha' : (dec w).a = x.a := by
    cases h5 : (dec w).a <;> cases h6 : x.a <;> rw [h5, h6] at ha <;> simp [abit] at ha ⊢
  have e : dec w = ⟨(dec w).v, (dec w).a⟩ := rfl
  rw [e, hv, ha']

/-- A word stored from an integer `z` congruent to `2v + a` (mod `2^64`), with `v` in range, decodes to `(v, a)`:
only the residue matters, so a result may wrap on the way and still come out right. -/
theorem dec_ofInt {z : Int} {x : Num} (hx : InRange x.v) (h : z % 2^64 = (2 * x.v + abit x.a) % 2^64) :
    dec (BitVec.ofInt 64 z) = x := by
  apply dec_of_toInt hx
  have h1 := toInt_bounds (BitVec.ofInt 64 z)
  have h2 : (BitVec.ofInt 64 z).toInt % 2^64 = z % 2^64 := by
    rw [BitVec.toInt_ofInt]; exact Int.bmod_emod
  have h3 := abit_cases x.a
  unfold InRange at hx
  omega

/-! ## 1. Encoding and decoding -/

/-- **Decoding inverts encoding** for every `v` in range and either axis. -/
theorem dec_enc {x : Num} (hx : InRange x.v) : dec (enc x) = x := dec_ofInt hx rfl

/-- **Encoding inverts decoding**: every word is the encoding of what it decodes to. -/
theorem enc_dec (w : Word) : enc (dec w) = w := by
  unfold enc; rw [← toInt_split]; exact BitVec.ofInt_toInt

/-- So the words and the pairs `(v, a)` with `v` in range are in one-to-one correspondence. -/
theorem enc_inj {x y : Num} (hx : InRange x.v) (hy : InRange y.v) (h : enc x = enc y) : x = y := by
  rw [← dec_enc hx, ← dec_enc hy, h]

theorem dec_inj {w1 w2 : Word} (h : dec w1 = dec w2) : w1 = w2 := by
  rw [← enc_dec w1, ← enc_dec w2, h]

/-- Every word is `ofInt (2v + a)` of its decoding. -/
theorem word_eq (w : Word) : w = BitVec.ofInt 64 (2 * (dec w).v + abit (dec w).a) := (enc_dec w).symm

/-! ### Zero -/

@[simp] theorem val_re (x : Num) : x.val.re = if x.a then 0 else x.v := by
  cases h : x.a <;> simp [Num.val, adir, unit, h]
@[simp] theorem val_im (x : Num) : x.val.im = if x.a then x.v else 0 := by
  cases h : x.a <;> simp [Num.val, adir, unit, h]

/-- A pair is zero exactly when its signed value is 0, on either axis. -/
theorem val_eq_zero_iff (x : Num) : x.val = 0 ↔ x.v = 0 := by
  rw [GI.ext_iff']; cases h : x.a <;> simp [h]

/-- The unsigned compare `w < 2`, the spec's zero check, is exactly `v = 0`. -/
theorem lt_two_iff (w : Word) : w < 2#64 ↔ (dec w).v = 0 := by
  rw [BitVec.lt_def]
  have h1 := toInt_split w; have h2 := abit_cases (dec w).a
  have h3 := toInt_toNat w; have h4 := w.isLt
  simp only [BitVec.toNat_ofNat]
  constructor
  · intro h; split at h3 <;> omega
  · intro h
    split at h3 <;> omega

/-- **Zero is exactly the words 0 and 1.** -/
theorem wval_eq_zero_iff (w : Word) : wval w = 0 ↔ w = 0#64 ∨ w = 1#64 := by
  unfold wval; rw [val_eq_zero_iff, ← lt_two_iff, BitVec.lt_def]
  constructor
  · intro h
    have : w.toNat = 0 ∨ w.toNat = 1 := by simp at h; omega
    rcases this with h | h
    · left; exact BitVec.eq_of_toNat_eq h
    · right; exact BitVec.eq_of_toNat_eq h
  · rintro (rfl | rfl) <;> decide

/-- The word 0 is zero on the real axis, the word 1 zero on the imaginary axis. -/
theorem dec_zero : dec 0#64 = ⟨0, false⟩ := by decide
theorem dec_one : dec 1#64 = ⟨0, true⟩ := by decide

/-- **Every nonzero number has exactly one word**: two pairs have the same value exactly when the values `v` are
equal and (the axes are equal or `v = 0`). -/
theorem val_eq_iff (x y : Num) : x.val = y.val ↔ x.v = y.v ∧ (x.a = y.a ∨ x.v = 0) := by
  rw [GI.ext_iff']
  obtain ⟨v1, a1⟩ := x; obtain ⟨v2, a2⟩ := y
  cases a1 <;> cases a2 <;> simp <;> omega

/-! ## The rules, as spec §4a states them -/

/-- **Add**, with `t = (w1 xor w2) and 1`: `t = 0`, `w1 + (w2 and not 1)`; otherwise `w1` if `w2 < 2` (unsigned),
else `w2` if `w1 < 2`, else an error (an expression, `none`). -/
def add (w1 w2 : Word) : Option Word :=
  if (w1 ^^^ w2) &&& 1#64 = 0#64 then some (w1 + (w2 &&& ~~~1#64))
  else if w2 < 2#64 then some w1
  else if w1 < 2#64 then some w2
  else none

/-- **Negate**: `((w and 1) shl 1) - w`. -/
def neg (w : Word) : Word := ((w &&& 1#64) <<< 1) - w

/-- **Subtract**: `t = 0`, `w1 - (w2 and not 1)`; otherwise `w1` if `w2 < 2`, the negation of `w2` if `w1 < 2`, else
an error. -/
def sub (w1 w2 : Word) : Option Word :=
  if (w1 ^^^ w2) &&& 1#64 = 0#64 then some (w1 - (w2 &&& ~~~1#64))
  else if w2 < 2#64 then some w1
  else if w1 < 2#64 then some (neg w2)
  else none

/-- **Multiply**: `p = (w1 sar 1) * (w2 and not 1)`, negated when `w1 and w2 and 1` is 1, then OR
`(w1 xor w2) and 1`. -/
def mul (w1 w2 : Word) : Word :=
  let p := w1.sshiftRight 1 * (w2 &&& ~~~1#64)
  let p := if w1 &&& w2 &&& 1#64 = 1#64 then -p else p
  p ||| ((w1 ^^^ w2) &&& 1#64)

/-- **Exact division** (the current rule): `q = (w1 and not 1) / (w2 and not 1)`, signed, so `2v1 / 2v2 = v1 / v2`;
then shift left 1 (`2q`); negated when `a2 = 1` and `a1 = 0`; then OR `(w1 xor w2) and 1`. -/
def div (w1 w2 : Word) : Word :=
  let q := (w1 &&& ~~~1#64).sdiv (w2 &&& ~~~1#64)
  let q := q <<< 1
  let q := if w2 &&& 1#64 = 1#64 ∧ w1 &&& 1#64 = 0#64 then -q else q
  q ||| ((w1 ^^^ w2) &&& 1#64)

/-- **Exact division, the old rule** (spec §4a before the fix): `q = (w1 and not 1) / (w2 sar 1)`, signed, negated
when `a2 = 1` and `a1 = 0`, then OR `(w1 xor w2) and 1`. Kept to record finding C-a. -/
def divOld (w1 w2 : Word) : Word :=
  let q := (w1 &&& ~~~1#64).sdiv (w2.sshiftRight 1)
  let q := if w2 &&& 1#64 = 1#64 ∧ w1 &&& 1#64 = 0#64 then -q else q
  q ||| ((w1 ^^^ w2) &&& 1#64)

/-- The most negative word, `-2^63`. -/
def intMin : Word := BitVec.intMin 64

/-- **x86-64's `idiv` overflow trap**: a 64-by-64-bit signed divide raises a divide error when the dividend is
`-2^63` and the divisor is `-1` (the quotient `2^63` does not fit). -/
def idivTraps (x y : Word) : Prop := x = intMin ∧ y = -1#64

/-- Every fault of `idiv`: the overflow trap, or a zero divisor (also a divide error on x86-64). -/
def idivFaults (x y : Word) : Prop := y = 0#64 ∨ idivTraps x y

/-! ### The word pieces, read as integers -/

/-- `w and 1` is the axis bit. -/
theorem lo_eq (w : Word) : w &&& 1#64 = BitVec.ofInt 64 (abit (dec w).a) := by
  apply BitVec.eq_of_toNat_eq
  rw [and_one_toNat, abit_dec, BitVec.toNat_ofInt]
  have := w.isLt
  omega

/-- `w and not 1` is `2v`. -/
theorem clr_eq (w : Word) : w &&& ~~~1#64 = BitVec.ofInt 64 (2 * (dec w).v) := by
  have h1 : (w &&& ~~~1#64) &&& (w &&& 1#64) = 0#64 := by
    ext i hi; simp only [BitVec.getElem_and, BitVec.getElem_not, BitVec.getElem_one, BitVec.getElem_zero]
    by_cases h : i = 0 <;> simp [h]
  have h2 : (w &&& ~~~1#64) ||| (w &&& 1#64) = w := by
    ext i hi; simp only [BitVec.getElem_or, BitVec.getElem_and, BitVec.getElem_not, BitVec.getElem_one]
    by_cases h : i = 0 <;> simp [h]
  have h3 : w &&& ~~~1#64 = w - (w &&& 1#64) := by
    rw [← BitVec.add_eq_or_of_and_eq_zero _ _ h1] at h2
    conv => rhs; arg 1; rw [← h2]
    rw [BitVec.add_sub_cancel]
  rw [h3, lo_eq]
  conv => lhs; arg 1; rw [word_eq w]
  rw [BitVec.sub_eq_add_neg, ← BitVec.ofInt_neg, ← BitVec.ofInt_add]
  congr 1; omega

/-- `w sar 1` is `v`. -/
theorem sar_eq (w : Word) : w.sshiftRight 1 = BitVec.ofInt 64 (dec w).v := by
  rw [← BitVec.ofInt_toInt (x := w.sshiftRight 1)]; rfl

/-- `(w1 xor w2) and 1` is the XOR of the axis bits. -/
theorem xor_lo (w1 w2 : Word) : (w1 ^^^ w2) &&& 1#64 = (w1 &&& 1#64) ^^^ (w2 &&& 1#64) := by
  ext i hi; simp only [BitVec.getElem_and, BitVec.getElem_xor, BitVec.getElem_one]
  by_cases h : i = 0 <;> simp [h]

/-- `w1 and w2 and 1` is the AND of the axis bits. -/
theorem and_lo (w1 w2 : Word) : w1 &&& w2 &&& 1#64 = (w1 &&& 1#64) &&& (w2 &&& 1#64) := by
  ext i hi; simp only [BitVec.getElem_and, BitVec.getElem_one]
  by_cases h : i = 0 <;> simp [h]

theorem xor_axis (w1 w2 : Word) :
    (w1 ^^^ w2) &&& 1#64 = BitVec.ofInt 64 (abit ((dec w1).a ^^ (dec w2).a)) := by
  rw [xor_lo, lo_eq, lo_eq]; cases (dec w1).a <;> cases (dec w2).a <;> decide

/-- **The axis check**: `t = 0` exactly when the axes are equal. -/
theorem axis_check (w1 w2 : Word) : (w1 ^^^ w2) &&& 1#64 = 0#64 ↔ (dec w1).a = (dec w2).a := by
  rw [xor_axis]; cases (dec w1).a <;> cases (dec w2).a <;> decide

/-- OR-ing an axis bit into an even word adds it. -/
theorem or_axis (k : Int) (a : Bool) :
    BitVec.ofInt 64 (2 * k) ||| BitVec.ofInt 64 (abit a) = BitVec.ofInt 64 (2 * k + abit a) := by
  cases a
  · simp [abit]
  · have h : BitVec.ofInt 64 (2 * k) &&& BitVec.ofInt 64 (abit true) = 0#64 := by
      apply BitVec.eq_of_toNat_eq
      rw [show BitVec.ofInt 64 (abit true) = 1#64 from rfl, and_one_toNat, BitVec.toNat_ofInt]
      simp only [BitVec.toNat_ofNat]
      omega
    rw [← BitVec.add_eq_or_of_and_eq_zero _ _ h, ← BitVec.ofInt_add]

/-! ## 2. Addition -/

/-- The value identity of the same-axis branch. -/
theorem val_add_same (v1 v2 : Int) (a : Bool) :
    (⟨v1 + v2, a⟩ : Num).val = (⟨v1, a⟩ : Num).val + (⟨v2, a⟩ : Num).val := by
  apply GI.ext' <;> cases a <;> simp

/-- Adding a zero, on any axis, leaves the other value. -/
theorem val_add_zero_right {x y : Num} (hy : y.v = 0) : x.val + y.val = x.val := by
  rw [(val_eq_zero_iff y).2 hy]; apply GI.ext' <;> simp
theorem val_add_zero_left {x y : Num} (hx : x.v = 0) : x.val + y.val = y.val := by
  rw [(val_eq_zero_iff x).2 hx]; apply GI.ext' <;> simp

/-- Same axis: the rule gives the word `w1 + (w2 and not 1)`, which is `2(v1 + v2) + a1` mod `2^64`. -/
theorem add_of_same {w1 w2 : Word} (ha : (dec w1).a = (dec w2).a) :
    add w1 w2 = some (BitVec.ofInt 64 (2 * ((dec w1).v + (dec w2).v) + abit (dec w1).a)) := by
  simp only [add, (axis_check w1 w2).2 ha, ↓reduceIte, clr_eq]
  conv => lhs; arg 1; arg 1; rw [word_eq w1]
  rw [← BitVec.ofInt_add]; congr 2; omega

/-- **Addition, same axis.** If the true sum `v1 + v2` is in range (no overflow), the rule's word decodes to
`(v1 + v2, a1)`: the true sum, on the common axis. -/
theorem add_same {w1 w2 : Word} (ha : (dec w1).a = (dec w2).a) (hov : InRange ((dec w1).v + (dec w2).v)) :
    ∃ r, add w1 w2 = some r ∧ dec r = ⟨(dec w1).v + (dec w2).v, (dec w1).a⟩ :=
  ⟨_, add_of_same ha, dec_ofInt hov rfl⟩

/-- Different axes, second value zero: the result is `w1`, whole. -/
theorem add_cross_zero_right {w1 w2 : Word} (ha : (dec w1).a ≠ (dec w2).a) (h2 : (dec w2).v = 0) :
    add w1 w2 = some w1 := by
  have ht : ¬ ((w1 ^^^ w2) &&& 1#64 = 0#64) := fun h => ha ((axis_check w1 w2).1 h)
  simp only [add, ht, (lt_two_iff w2).2 h2, ↓reduceIte]

/-- Different axes, first value zero, second not: the result is `w2`, whole. -/
theorem add_cross_zero_left {w1 w2 : Word} (ha : (dec w1).a ≠ (dec w2).a) (h1 : (dec w1).v = 0)
    (h2 : (dec w2).v ≠ 0) : add w1 w2 = some w2 := by
  have ht : ¬ ((w1 ^^^ w2) &&& 1#64 = 0#64) := fun h => ha ((axis_check w1 w2).1 h)
  have h2' : ¬ w2 < 2#64 := fun h => h2 ((lt_two_iff w2).1 h)
  simp only [add, ht, h2', (lt_two_iff w1).2 h1, ↓reduceIte]

/-- **Addition.** Whenever the axes agree or a value is zero, and the true result is in range (needed only on the
same axis: across axes the result is an operand), the rule gives a word whose value is the true sum. -/
theorem add_value {w1 w2 : Word} (hax : (dec w1).a = (dec w2).a ∨ (dec w1).v = 0 ∨ (dec w2).v = 0)
    (hov : (dec w1).a = (dec w2).a → InRange ((dec w1).v + (dec w2).v)) :
    ∃ r, add w1 w2 = some r ∧ wval r = wval w1 + wval w2 := by
  by_cases ha : (dec w1).a = (dec w2).a
  · obtain ⟨r, h1, h2⟩ := add_same ha (hov ha)
    refine ⟨r, h1, ?_⟩
    unfold wval; rw [h2, val_add_same]; congr 1; rw [ha]
  · by_cases h2 : (dec w2).v = 0
    · exact ⟨w1, add_cross_zero_right ha h2, (val_add_zero_right h2).symm⟩
    · have h1 : (dec w1).v = 0 := by
        rcases hax with h | h | h
        · exact absurd h ha
        · exact h
        · exact absurd h h2
      exact ⟨w2, add_cross_zero_left ha h1 h2, (val_add_zero_left h1).symm⟩

/-- **The error.** The rule reports an expression exactly when the axes differ and both values are nonzero. -/
theorem add_eq_none_iff (w1 w2 : Word) :
    add w1 w2 = none ↔ (dec w1).a ≠ (dec w2).a ∧ (dec w1).v ≠ 0 ∧ (dec w2).v ≠ 0 := by
  unfold add
  by_cases ht : (w1 ^^^ w2) &&& 1#64 = 0#64
  · simp [ht, (axis_check w1 w2).1 ht]
  · have ha := mt (axis_check w1 w2).2 ht
    by_cases h2 : w2 < 2#64
    · simp [ht, h2, (lt_two_iff w2).1 h2]
    · by_cases h1 : w1 < 2#64
      · simp [ht, h2, h1, (lt_two_iff w1).1 h1]
      · simp only [ht, h2, h1, ↓reduceIte, true_iff]
        exact ⟨ha, mt (lt_two_iff w1).2 h1, mt (lt_two_iff w2).2 h2⟩

/-- **Across axes, no single number.** In that case no pair `(v, a)`, in range or not, has the sum's value: the
sum has a nonzero real part and a nonzero imaginary part. -/
theorem cross_no_num {x y : Num} (ha : x.a ≠ y.a) (h1 : x.v ≠ 0) (h2 : y.v ≠ 0) :
    ∀ z : Num, z.val ≠ x.val + y.val := by
  intro z heq
  have hre := congrArg GI.re heq; have him := congrArg GI.im heq
  obtain ⟨v1, a1⟩ := x; obtain ⟨v2, a2⟩ := y; obtain ⟨v, a⟩ := z
  cases a1 <;> cases a2 <;> cases a <;> simp at ha hre him h1 h2 <;> omega

theorem add_none_no_num {w1 w2 : Word} (h : add w1 w2 = none) : ∀ z : Num, z.val ≠ wval w1 + wval w2 := by
  obtain ⟨ha, h1, h2⟩ := (add_eq_none_iff w1 w2).1 h
  exact cross_no_num ha h1 h2

/-- **Overflow is real.** On the same axis the sum `(2^62 - 1) + 1` leaves the range, and the word decodes to
`-2^62`: this is why `add_same` needs its hypothesis. -/
theorem add_overflow_example :
    add (enc ⟨2^62 - 1, false⟩) (enc ⟨1, false⟩) = some (enc ⟨-2^62, false⟩) := by decide +kernel

/-! ## 3. Negation and subtraction -/

/-- The negation rule's word is `2(-v) + a` mod `2^64`, always. -/
theorem neg_eq (w : Word) : neg w = BitVec.ofInt 64 (2 * (-(dec w).v) + abit (dec w).a) := by
  have hs : (w &&& 1#64) <<< 1 = BitVec.ofInt 64 (2 * abit (dec w).a) := by
    rw [lo_eq]; cases (dec w).a <;> decide
  unfold neg; rw [hs]
  conv => lhs; arg 2; rw [word_eq w]
  rw [BitVec.sub_eq_add_neg, ← BitVec.ofInt_neg, ← BitVec.ofInt_add]; congr 1; omega

/-- **Negation**: for `v > -2^62`, the rule gives `(-v, a)`, the true negation on the same axis. A zero stays a
zero on its own axis. -/
theorem dec_neg {w : Word} (hv : -2^62 < (dec w).v) : dec (neg w) = ⟨-(dec w).v, (dec w).a⟩ := by
  rw [neg_eq]; refine dec_ofInt (x := ⟨-(dec w).v, (dec w).a⟩) ?_ rfl
  have := dec_range w; unfold InRange at *; simp only; omega

theorem val_neg (x : Num) : (⟨-x.v, x.a⟩ : Num).val = -x.val := by
  apply GI.ext' <;> cases h : x.a <;> simp [h]

theorem neg_value {w : Word} (hv : -2^62 < (dec w).v) : wval (neg w) = -wval w := by
  unfold wval; rw [dec_neg hv, val_neg]

/-- **The one edge case.** `v = -2^62` has no negation in range (`2^62` is not representable); the rule returns the
word unchanged, so `-(-2^62) = -2^62` (on either axis). This is overflow, the same as the machine's own
`-(-2^63) = -2^63`. -/
theorem neg_edge {w : Word} (hv : (dec w).v = -2^62) : neg w = w := by
  apply dec_inj; rw [neg_eq]; apply dec_ofInt (dec_range w); rw [hv]; omega

theorem neg_edge_example : neg (enc ⟨-2^62, false⟩) = enc ⟨-2^62, false⟩ := by decide +kernel

/-- The value identity of the same-axis subtraction. -/
theorem val_sub_same (v1 v2 : Int) (a : Bool) :
    (⟨v1 - v2, a⟩ : Num).val = (⟨v1, a⟩ : Num).val - (⟨v2, a⟩ : Num).val := by
  apply GI.ext' <;> cases a <;> simp

theorem sub_of_same {w1 w2 : Word} (ha : (dec w1).a = (dec w2).a) :
    sub w1 w2 = some (BitVec.ofInt 64 (2 * ((dec w1).v - (dec w2).v) + abit (dec w1).a)) := by
  simp only [sub, (axis_check w1 w2).2 ha, ↓reduceIte, clr_eq]
  conv => lhs; arg 1; arg 1; rw [word_eq w1]
  rw [BitVec.sub_eq_add_neg, ← BitVec.ofInt_neg, ← BitVec.ofInt_add]; congr 2; omega

/-- **Subtraction, same axis**: `(v1 - v2, a1)` when that is in range. -/
theorem sub_same {w1 w2 : Word} (ha : (dec w1).a = (dec w2).a) (hov : InRange ((dec w1).v - (dec w2).v)) :
    ∃ r, sub w1 w2 = some r ∧ dec r = ⟨(dec w1).v - (dec w2).v, (dec w1).a⟩ :=
  ⟨_, sub_of_same ha, dec_ofInt hov rfl⟩

theorem sub_cross_zero_right {w1 w2 : Word} (ha : (dec w1).a ≠ (dec w2).a) (h2 : (dec w2).v = 0) :
    sub w1 w2 = some w1 := by
  have ht : ¬ ((w1 ^^^ w2) &&& 1#64 = 0#64) := fun h => ha ((axis_check w1 w2).1 h)
  simp only [sub, ht, (lt_two_iff w2).2 h2, ↓reduceIte]

theorem sub_cross_zero_left {w1 w2 : Word} (ha : (dec w1).a ≠ (dec w2).a) (h1 : (dec w1).v = 0)
    (h2 : (dec w2).v ≠ 0) : sub w1 w2 = some (neg w2) := by
  have ht : ¬ ((w1 ^^^ w2) &&& 1#64 = 0#64) := fun h => ha ((axis_check w1 w2).1 h)
  have h2' : ¬ w2 < 2#64 := fun h => h2 ((lt_two_iff w2).1 h)
  simp only [sub, ht, h2', (lt_two_iff w1).2 h1, ↓reduceIte]

/-- **Subtraction.** Whenever the axes agree or a value is zero, and the true difference is in range, the rule
gives a word whose value is the true difference. Across axes with `v1 = 0` the result is `-v2`, so the
hypothesis there excludes exactly `v2 = -2^62` (negation's edge case). -/
theorem sub_value {w1 w2 : Word} (hax : (dec w1).a = (dec w2).a ∨ (dec w1).v = 0 ∨ (dec w2).v = 0)
    (hov : InRange ((dec w1).v - (dec w2).v)) :
    ∃ r, sub w1 w2 = some r ∧ wval r = wval w1 - wval w2 := by
  by_cases ha : (dec w1).a = (dec w2).a
  · obtain ⟨r, h1, h2⟩ := sub_same ha hov
    refine ⟨r, h1, ?_⟩
    unfold wval; rw [h2, val_sub_same]; congr 1; rw [ha]
  · by_cases h2 : (dec w2).v = 0
    · refine ⟨w1, sub_cross_zero_right ha h2, ?_⟩
      unfold wval; rw [(val_eq_zero_iff _).2 h2]; apply GI.ext' <;> simp
    · have h1 : (dec w1).v = 0 := by
        rcases hax with h | h | h
        · exact absurd h ha
        · exact h
        · exact absurd h h2
      refine ⟨neg w2, sub_cross_zero_left ha h1 h2, ?_⟩
      have hv : -2^62 < (dec w2).v := by unfold InRange at hov; omega
      rw [neg_value hv]; unfold wval; rw [(val_eq_zero_iff _).2 h1]; apply GI.ext' <;> simp

theorem sub_eq_none_iff (w1 w2 : Word) :
    sub w1 w2 = none ↔ (dec w1).a ≠ (dec w2).a ∧ (dec w1).v ≠ 0 ∧ (dec w2).v ≠ 0 := by
  unfold sub
  by_cases ht : (w1 ^^^ w2) &&& 1#64 = 0#64
  · simp [ht, (axis_check w1 w2).1 ht]
  · have ha := mt (axis_check w1 w2).2 ht
    by_cases h2 : w2 < 2#64
    · simp [ht, h2, (lt_two_iff w2).1 h2]
    · by_cases h1 : w1 < 2#64
      · simp [ht, h2, h1, (lt_two_iff w1).1 h1]
      · simp only [ht, h2, h1, ↓reduceIte, true_iff]
        exact ⟨ha, mt (lt_two_iff w1).2 h1, mt (lt_two_iff w2).2 h2⟩

theorem sub_none_no_num {w1 w2 : Word} (h : sub w1 w2 = none) : ∀ z : Num, z.val ≠ wval w1 - wval w2 := by
  obtain ⟨ha, h1, h2⟩ := (sub_eq_none_iff w1 w2).1 h
  intro z heq
  have h2' : (-(dec w2).v) ≠ 0 := by omega
  apply cross_no_num (y := ⟨-(dec w2).v, (dec w2).a⟩) ha h1 h2' z
  rw [heq, val_neg]; unfold wval; apply GI.ext' <;> simp <;> omega

/-! ## 4. Multiplication -/

/-- The rule's product pair: `((-1)^(a1 a2) v1 v2, a1 xor a2)`. -/
def mulNum (x y : Num) : Num := ⟨if x.a && y.a then -(x.v * y.v) else x.v * y.v, x.a ^^ y.a⟩

/-- **`i · i = -1`**: the product pair has the product value, on every combination of axes. -/
theorem val_mulNum (x y : Num) : (mulNum x y).val = x.val * y.val := by
  obtain ⟨v1, a1⟩ := x; obtain ⟨v2, a2⟩ := y
  apply GI.ext' <;> cases a1 <;> cases a2 <;> simp [mulNum] <;> grind

theorem mul_cond (a1 a2 : Bool) :
    (BitVec.ofInt 64 (abit a1) &&& BitVec.ofInt 64 (abit a2) = 1#64) ↔ (a1 && a2) = true := by
  cases a1 <;> cases a2 <;> decide

/-- The multiplication rule's word is `2·v + a` of the product pair, mod `2^64`, always (no hypothesis). -/
theorem mul_eq (w1 w2 : Word) :
    mul w1 w2 = BitVec.ofInt 64 (2 * (mulNum (dec w1) (dec w2)).v + abit (mulNum (dec w1) (dec w2)).a) := by
  unfold mul
  simp only []
  rw [sar_eq, clr_eq, and_lo, lo_eq w1, lo_eq w2, xor_axis, ← BitVec.ofInt_mul, Int.mul_left_comm]
  simp only [mul_cond]
  simp only [mulNum]
  cases (dec w1).a && (dec w2).a
  · simp only [Bool.false_eq_true, ↓reduceIte]; rw [or_axis]
  · simp only [↓reduceIte]; rw [← BitVec.ofInt_neg, ← Int.mul_neg, or_axis]

/-- **Multiplication.** If the true product's value is in range (no overflow), the rule's word decodes to the
product pair, and its value is the product. The intermediate `v1 · 2v2` may wrap on the way (it does for
`(2^31 i)(2^31 i)`), and the result is still right. -/
theorem dec_mul {w1 w2 : Word} (hov : InRange (mulNum (dec w1) (dec w2)).v) :
    dec (mul w1 w2) = mulNum (dec w1) (dec w2) := by
  rw [mul_eq]; exact dec_ofInt hov rfl

theorem mul_value {w1 w2 : Word} (hov : InRange (mulNum (dec w1) (dec w2)).v) :
    wval (mul w1 w2) = wval w1 * wval w2 := by
  unfold wval; rw [dec_mul hov, val_mulNum]

/-- For integers (both axes real) the rule is the plain product. -/
theorem mulNum_real (v1 v2 : Int) : mulNum ⟨v1, false⟩ ⟨v2, false⟩ = ⟨v1 * v2, false⟩ := rfl

/-- `(2^31 i)(2^31 i) = -2^62`: `v1 · 2v2 = 2^63` wraps to `-2^63`, the negation wraps back to `-2^63`, and the
word is `2·(-2^62)`, the right answer. -/
theorem mul_wrap_example : mul (enc ⟨2^31, true⟩) (enc ⟨2^31, true⟩) = enc ⟨-2^62, false⟩ := by
  decide +kernel

/-! ## 5. Exact division -/

/-- The rule's quotient pair: `((-1)^((1 - a1) a2) v1 / v2, a1 xor a2)`. -/
def divNum (x y : Num) : Num := ⟨if !x.a && y.a then -(x.v / y.v) else x.v / y.v, x.a ^^ y.a⟩

/-- When `v2 ≠ 0` divides `v1`, the quotient pair times the divisor is the dividend: `1/i = -i`. -/
theorem val_divNum_mul {x y : Num} (hy : y.v ≠ 0) (hd : y.v ∣ x.v) : (divNum x y).val * y.val = x.val := by
  obtain ⟨v1, a1⟩ := x; obtain ⟨v2, a2⟩ := y
  obtain ⟨k, rfl⟩ := hd
  simp only at hy
  simp only [divNum, Int.mul_ediv_cancel_left _ hy]
  apply GI.ext' <;> cases a1 <;> cases a2 <;> simp <;> grind

/-- The signed divide of `2(v2 k)` by `2v2` is `k`, exactly, when `v2 ≠ 0` and both values are in range. -/
theorem sdiv_exact {k v2 : Int} (hv2 : v2 ≠ 0) (h2 : InRange v2) (hk : InRange (v2 * k)) :
    (BitVec.ofInt 64 (2 * (v2 * k))).sdiv (BitVec.ofInt 64 (2 * v2)) = BitVec.ofInt 64 k := by
  unfold InRange at h2 hk
  apply BitVec.eq_of_toInt_eq
  rw [BitVec.toInt_sdiv, BitVec.toInt_ofInt_eq_self (by decide) (by show -2^63 ≤ _; omega)
      (by show _ < 2^63; omega),
    BitVec.toInt_ofInt_eq_self (by decide) (by show -2^63 ≤ _; omega) (by show _ < 2^63; omega),
    BitVec.toInt_ofInt, ← Int.mul_assoc, Int.mul_tdiv_cancel_left _ (by omega)]

/-- Shifting left by one doubles, mod `2^64`. -/
theorem shl_one (k : Int) : BitVec.ofInt 64 k <<< 1 = BitVec.ofInt 64 (2 * k) := by
  apply BitVec.eq_of_toNat_eq
  rw [BitVec.toNat_shiftLeft, BitVec.toNat_ofInt, BitVec.toNat_ofInt, Nat.shiftLeft_eq]
  omega

theorem div_cond (a1 a2 : Bool) :
    (BitVec.ofInt 64 (abit a2) = 1#64 ∧ BitVec.ofInt 64 (abit a1) = 0#64) ↔ (!a1 && a2) = true := by
  cases a1 <;> cases a2 <;> decide

/-- When `v2 ≠ 0` divides `v1`, the division rule's word is `2·v + a` of the quotient pair, mod `2^64` (no range
hypothesis on the quotient). -/
theorem div_eq {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v) :
    div w1 w2 = BitVec.ofInt 64 (2 * (divNum (dec w1) (dec w2)).v + abit (divNum (dec w1) (dec w2)).a) := by
  obtain ⟨k, hk⟩ := hd
  have h1 := dec_range w1; rw [hk] at h1
  unfold div
  simp only []
  rw [clr_eq w1, clr_eq w2, lo_eq w1, lo_eq w2, xor_axis, hk, sdiv_exact hv2 (dec_range w2) h1, shl_one]
  simp only [div_cond]
  simp only [divNum, hk, Int.mul_ediv_cancel_left _ hv2]
  cases !(dec w1).a && (dec w2).a
  · simp only [Bool.false_eq_true, ↓reduceIte]; rw [or_axis]
  · simp only [↓reduceIte]; rw [← BitVec.ofInt_neg, ← Int.mul_neg, or_axis]

/-- **Exact division.** When `v2 ≠ 0` divides `v1` and the true quotient is in range (no overflow), the rule's
word decodes to the quotient pair, whose value times the divisor's is the dividend's. -/
theorem dec_div {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v)
    (hov : InRange (divNum (dec w1) (dec w2)).v) : dec (div w1 w2) = divNum (dec w1) (dec w2) := by
  rw [div_eq hv2 hd]; exact dec_ofInt hov rfl

theorem div_value {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v)
    (hov : InRange (divNum (dec w1) (dec w2)).v) : wval (div w1 w2) * wval w2 = wval w1 := by
  unfold wval; rw [dec_div hv2 hd hov]; exact val_divNum_mul hv2 hd

/-- The quotient is the only value with that property (the divisor is nonzero). -/
theorem div_unique {x y z : Num} (hy : y.v ≠ 0) (hd : y.v ∣ x.v) (h : z.val * y.val = x.val) :
    z.val = (divNum x y).val := by
  have h2 := val_divNum_mul hy hd
  rw [← h2] at h
  generalize z.val = Z at h; generalize (divNum x y).val = Q at h
  have hre := congrArg GI.re h; have him := congrArg GI.im h
  obtain ⟨v2, a2⟩ := y
  simp only at hy
  apply GI.ext' <;> cases a2 <;> simp at hre him
  · exact Int.eq_of_mul_eq_mul_right hy hre
  · exact Int.eq_of_mul_eq_mul_right hy him
  · exact Int.eq_of_mul_eq_mul_right hy him
  · exact Int.eq_of_mul_eq_mul_right hy (by omega)

/-! ### The divide never faults -/

theorem intMin_toInt : intMin.toInt = -2^63 := by decide

theorem idivTraps_iff (x y : Word) : idivTraps x y ↔ x.toInt = -2^63 ∧ y.toInt = -1 := by
  unfold idivTraps
  rw [← BitVec.toInt_inj (x := x), ← BitVec.toInt_inj (x := y), intMin_toInt,
    show (-1#64 : Word).toInt = -1 by decide]

/-- `w and not 1`, read signed, is `2v`: an even number. -/
theorem clr_toInt (w : Word) : (w &&& ~~~1#64).toInt = 2 * (dec w).v := by
  have h1 := dec_range w
  rw [clr_eq]; unfold InRange at h1
  exact BitVec.toInt_ofInt_eq_self (by decide) (by show -2^63 ≤ _; omega) (by show _ < 2^63; omega)

/-- **The new divide never traps**, for any two words: its divisor `w2 and not 1` is even, so never `-1`. -/
theorem div_never_traps (w1 w2 : Word) : ¬ idivTraps (w1 &&& ~~~1#64) (w2 &&& ~~~1#64) := by
  rw [idivTraps_iff, clr_toInt, clr_toInt]; omega

/-- Its divisor is zero exactly when `v2 = 0`. -/
theorem clr_eq_zero_iff (w : Word) : w &&& ~~~1#64 = 0#64 ↔ (dec w).v = 0 := by
  rw [← BitVec.toInt_inj, clr_toInt, show (0#64 : Word).toInt = 0 by decide]; omega

/-- **So it never faults** when `v2 ≠ 0`, with no range hypothesis on the quotient. -/
theorem div_never_faults {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) :
    ¬ idivFaults (w1 &&& ~~~1#64) (w2 &&& ~~~1#64) := by
  rintro (h | h)
  · exact hv2 ((clr_eq_zero_iff w2).1 h)
  · exact div_never_traps w1 w2 h

/-- **`(-2^62) / (-i) = -2^62 i`**, the case that trapped under the old rule, step by step: `2v1 / 2v2 = -2^63 / -2`
is `2^62`; `2q = 2^63` wraps to `-2^63`; the negation (`a2 = 1`, `a1 = 0`) leaves `-2^63`; and OR 1 gives
`-2^63 + 1 = 2·(-2^62) + 1`, the word of `-2^62 i`. -/
theorem div_edge_steps :
    let w1 := enc ⟨-2^62, false⟩
    let w2 := enc ⟨-1, true⟩
    let q := (w1 &&& ~~~1#64).sdiv (w2 &&& ~~~1#64)
    (w1 &&& ~~~1#64).toInt = -2^63 ∧ (w2 &&& ~~~1#64).toInt = -2 ∧
    q.toInt = 2^62 ∧ (q <<< 1).toInt = -2^63 ∧ (-(q <<< 1)).toInt = -2^63 ∧
    (-(q <<< 1) ||| ((w1 ^^^ w2) &&& 1#64)) = enc ⟨-2^62, true⟩ := by
  decide +kernel

theorem div_edge_example : div (enc ⟨-2^62, false⟩) (enc ⟨-1, true⟩) = enc ⟨-2^62, true⟩ := by
  decide +kernel

theorem div_edge_in_range : InRange (divNum ⟨-2^62, false⟩ ⟨-1, true⟩).v := by decide

/-! ### The old rule (finding C-a) -/

/-- The old rule's signed divide of `2(v2 k)` by `v2` is `2k` in the model (wrapping on `-2^63 / -1`). -/
theorem sdivOld_exact {k v2 : Int} (hv2 : v2 ≠ 0) (h2 : InRange v2) (hk : InRange (v2 * k)) :
    (BitVec.ofInt 64 (2 * (v2 * k))).sdiv (BitVec.ofInt 64 v2) = BitVec.ofInt 64 (2 * k) := by
  unfold InRange at h2 hk
  apply BitVec.eq_of_toInt_eq
  rw [BitVec.toInt_sdiv, BitVec.toInt_ofInt_eq_self (by decide) (by show -2^63 ≤ _; omega)
      (by show _ < 2^63; omega),
    BitVec.toInt_ofInt_eq_self (by decide) (by show -2^63 ≤ _; omega) (by show _ < 2^63; omega),
    BitVec.toInt_ofInt, Int.mul_left_comm, Int.mul_tdiv_cancel_left _ hv2]

theorem divOld_eq {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v) :
    divOld w1 w2 = BitVec.ofInt 64 (2 * (divNum (dec w1) (dec w2)).v + abit (divNum (dec w1) (dec w2)).a) := by
  obtain ⟨k, hk⟩ := hd
  have h1 := dec_range w1; rw [hk] at h1
  unfold divOld
  simp only []
  rw [clr_eq, sar_eq, lo_eq w1, lo_eq w2, xor_axis, hk, sdivOld_exact hv2 (dec_range w2) h1]
  simp only [div_cond]
  simp only [divNum, hk, Int.mul_ediv_cancel_left _ hv2]
  cases !(dec w1).a && (dec w2).a
  · simp only [Bool.false_eq_true, ↓reduceIte]; rw [or_axis]
  · simp only [↓reduceIte]; rw [← BitVec.ofInt_neg, ← Int.mul_neg, or_axis]

/-- **The new rule agrees with the old** on every exact division (`v2 ≠ 0`, `v2 ∣ v1`), as words, in the model
(where the old rule's divide wraps instead of trapping). -/
theorem div_eq_divOld {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v) :
    div w1 w2 = divOld w1 w2 := by
  rw [div_eq hv2 hd, divOld_eq hv2 hd]

/-- ... in particular wherever the old rule does not trap on x86-64. -/
theorem div_agrees_where_old_runs {w1 w2 : Word} (hv2 : (dec w2).v ≠ 0) (hd : (dec w2).v ∣ (dec w1).v)
    (hnt : ¬ idivTraps (w1 &&& ~~~1#64) (w2.sshiftRight 1)) : div w1 w2 = divOld w1 w2 :=
  div_eq_divOld hv2 hd

/-- The two rules differ off exact division: `1 / 2` (real) is the word 1 under the old rule (it reads as `0·i`) and
the word 0 under the new. Spec §1 excludes such operands. -/
theorem div_inexact_differ :
    divOld (enc ⟨1, false⟩) (enc ⟨2, false⟩) = 1#64 ∧ div (enc ⟨1, false⟩) (enc ⟨2, false⟩) = 0#64 := by
  decide +kernel

/-- **Overflow in the old rule.** `-2^62 / -1 = 2^62` is out of range. The model's signed divide wraps (`-2^63 / -1`
gives `-2^63`), so the word decodes to `-2^62`; on x86-64 the old rule traps here. -/
theorem divOld_overflow_example : divOld (enc ⟨-2^62, false⟩) (enc ⟨-1, false⟩) = enc ⟨-2^62, false⟩ := by
  decide +kernel

/-- **Finding C-a, the old rule's trap.** The old rule's divide `(w1 and not 1) / (w2 sar 1)` traps exactly when
`v1 = -2^62` and `v2 = -1`. -/
theorem divOld_traps_iff (w1 w2 : Word) :
    idivTraps (w1 &&& ~~~1#64) (w2.sshiftRight 1) ↔ ((dec w1).v = -2^62 ∧ (dec w2).v = -1) := by
  rw [idivTraps_iff, clr_toInt, show (w2.sshiftRight 1).toInt = (dec w2).v from rfl]; omega

/-- Among divisions whose quotient is in range, the old rule traps exactly for `v1 = -2^62` real and `v2 = -1`
imaginary: `(-2^62) / (-i) = -2^62 i` (`div_edge_in_range`) trapped on x86-64. -/
theorem div_trap_iff {w1 w2 : Word} (hov : InRange (divNum (dec w1) (dec w2)).v) :
    idivTraps (w1 &&& ~~~1#64) (w2.sshiftRight 1) ↔
      ((dec w1).v = -2^62 ∧ (dec w2).v = -1 ∧ (dec w1).a = false ∧ (dec w2).a = true) := by
  rw [divOld_traps_iff]
  constructor
  · rintro ⟨e1, e2⟩
    unfold divNum InRange at hov
    rw [e1, e2] at hov
    refine ⟨e1, e2, ?_⟩
    cases ha1 : (dec w1).a <;> cases ha2 : (dec w2).a <;> simp [ha1, ha2] at hov ⊢
  · rintro ⟨e1, e2, -, -⟩; exact ⟨e1, e2⟩

theorem divOld_edge_example : divOld (enc ⟨-2^62, false⟩) (enc ⟨-1, true⟩) = enc ⟨-2^62, true⟩ := by
  decide +kernel

/-! ## 6. Agreement with Design A -/

/-- C's pair `(v, a)` as A's pair `(|v|, d)`, with `d = 2s + a`, `s` the sign of `v`. -/
def toA (x : Num) : A.Num :=
  ⟨x.v.natAbs, if x.v < 0 then (if x.a then 3 else 2) else (if x.a then 1 else 0)⟩

/-- The direction is `2s + a`: the sign bit above the axis bit. -/
theorem toA_d (x : Num) : (toA x).d.val = 2 * (if x.v < 0 then 1 else 0) + (if x.a then 1 else 0) := by
  unfold toA; by_cases h : x.v < 0 <;> cases x.a <;> simp [h]

/-- On a word, `s` is the word's top bit (bit 63, the sign of `v`) and `a` its bottom bit: A's direction is C's
word's two outer bits, top above bottom. -/
theorem toA_dec_d (w : Word) : (toA (dec w)).d.val = 2 * (if w.msb then 1 else 0) + w.toNat % 2 := by
  have h1 := toInt_split w; have h2 := abit_cases (dec w).a; have h3 := abit_dec w
  have hs : w.msb = decide ((dec w).v < 0) := by
    rw [BitVec.msb_eq_toInt]; apply decide_eq_decide.2; omega
  rw [toA_d, hs]
  by_cases hv : (dec w).v < 0 <;> cases ha : (dec w).a <;> simp [hv, ha, abit] at h3 ⊢ <;> omega

/-- **C and A give the same value.** -/
theorem val_toA (x : Num) : A.val (toA x) = x.val := by
  obtain ⟨v, a⟩ := x
  apply GI.ext' <;> by_cases h : v < 0 <;> cases a <;> simp [toA, h, unit] <;> omega

/-- A's pair `(m, d)` as C's pair: `v = m` for `d = 0, 1` and `v = -m` for `d = 2, 3`; the axis is `d`'s low bit. -/
def ofA (y : A.Num) : Num := ⟨if y.d.val < 2 then (y.m : Int) else -(y.m : Int), decide (y.d.val % 2 = 1)⟩

theorem val_ofA (y : A.Num) : (ofA y).val = A.val y := by
  obtain ⟨m, d⟩ := y
  apply GI.ext' <;> rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp [ofA, unit]

/-- C's numbers, through `toA`, have magnitudes up to `2^62`. -/
theorem toA_m_le {x : Num} (hx : InRange x.v) : (toA x).m ≤ 2^62 := by
  unfold InRange at hx; simp only [toA]; omega

/-- A's numbers with magnitude below `2^62` are C's numbers (and so is `-2^62` on either axis). -/
theorem ofA_range {y : A.Num} (hy : y.m < 2^62) : InRange (ofA y).v := by
  unfold InRange ofA; split <;> simp only <;> omega

/-- The two maps are inverse: `ofA ∘ toA` is the identity on C's pairs, and `toA ∘ ofA` is the identity on A's
pairs other than zero (A has four zeros, C two: `toA` sends each zero to direction `a`). -/
theorem ofA_toA (x : Num) : ofA (toA x) = x := by
  obtain ⟨v, a⟩ := x
  by_cases h : v < 0 <;> cases a <;> simp [ofA, toA, h] <;> omega

theorem toA_ofA {y : A.Num} (hy : y.m ≠ 0) : toA (ofA y) = y := by
  obtain ⟨m, d⟩ := y
  simp only at hy
  rcases fin4_cases d with rfl|rfl|rfl|rfl <;> simp [ofA, toA] <;> omega

/-- So a value is a C number exactly when it is an A number with `m < 2^62`, or `-2^62` / `-2^62 i`: C holds the same
numbers on the axes as A, over a quarter of A's range. -/
theorem same_numbers_C_to_A (w : Word) : ∃ y : A.Num, A.val y = wval w ∧ y.m ≤ 2^62 :=
  ⟨toA (dec w), val_toA _, toA_m_le (dec_range w)⟩

theorem same_numbers_A_to_C {y : A.Num} (hy : y.m < 2^62) : ∃ w : Word, wval w = A.val y :=
  ⟨enc (ofA y), by unfold wval; rw [dec_enc (ofA_range hy), val_ofA]⟩

/-! ## 7. Control, for reference: the machine's signed negate and divide

The machine's own signed integers (spec §2) on the same 64-bit words: a value is `x.toInt`, in
`-2^63 ≤ z < 2^63` (`InRange64`). -/

/-- The range of a signed 64-bit integer. -/
def InRange64 (z : Int) : Prop := -2^63 ≤ z ∧ z < 2^63

/-- **Signed negate is exact except at `-2^63`.** -/
theorem ctl_neg {x : Word} (h : x ≠ intMin) : (-x).toInt = -x.toInt := by
  have hx : x.toInt ≠ -2^63 := fun e => h (BitVec.toInt_inj.1 (by rw [e, intMin_toInt]))
  have hb := toInt_bounds x
  rw [BitVec.toInt_neg]
  exact Int.bmod_eq_of_le (by show -(2^64/2 : Int) ≤ _; omega) (by rw [Int.natCast_pow]; omega)

/-- `-(-2^63) = -2^63`: the one input whose negation wraps. -/
theorem ctl_neg_intMin : -intMin = intMin := by decide

/-- **Exactly one input has no negation in range**: `-x` is out of range exactly when `x = -2^63`. -/
theorem ctl_neg_out_iff (x : Word) : ¬ InRange64 (-x.toInt) ↔ x = intMin := by
  rw [← BitVec.toInt_inj, intMin_toInt]
  have hb := toInt_bounds x
  unfold InRange64; omega

/-- A nonzero word reads as a nonzero integer. -/
theorem toInt_ne_zero {y : Word} (hy : y ≠ 0#64) : y.toInt ≠ 0 :=
  fun e => hy (BitVec.toInt_inj.1 (by rw [e]; decide))

/-- **Signed divide traps exactly where the true quotient is out of range.** For `y ≠ 0`, the truncated quotient
`x.toInt / y.toInt` is out of range exactly on `idivTraps x y`, i.e. `-2^63 / -1`. -/
theorem ctl_div_out_iff (x y : Word) (hy : y ≠ 0#64) :
    ¬ InRange64 (x.toInt.tdiv y.toInt) ↔ idivTraps x y := by
  rw [idivTraps_iff]
  have hy0 := toInt_ne_zero hy
  have hx := toInt_bounds x; have hyb := toInt_bounds y
  unfold InRange64
  by_cases h1 : y.toInt = -1
  · rw [h1, show (-1 : Int) = -(1 : Int) from rfl, Int.tdiv_neg, Int.tdiv_one]; omega
  · by_cases h2 : y.toInt = 1
    · rw [h2, Int.tdiv_one]; omega
    · have hb : 2 ≤ y.toInt.natAbs := by omega
      have hq : (x.toInt.tdiv y.toInt).natAbs ≤ 2^62 := by
        rw [Int.natAbs_tdiv]
        have := Nat.div_le_div_left (a := x.toInt.natAbs) hb (by decide)
        show x.toInt.natAbs / y.toInt.natAbs ≤ _
        omega
      omega

/-- The trapping case's true quotient is `2^63`, one past the range: in Control the trap is genuine overflow. -/
theorem ctl_trap_quotient : intMin.toInt.tdiv (-1#64 : Word).toInt = 2^63 := by decide

/-- Off the trap (and off a zero divisor), the machine's signed divide is the true truncated quotient. -/
theorem ctl_sdiv {x y : Word} (hy : y ≠ 0#64) (h : ¬ idivTraps x y) :
    (x.sdiv y).toInt = x.toInt.tdiv y.toInt := by
  have hr := (not_congr (ctl_div_out_iff x y hy)).2 h
  simp only [Classical.not_not, InRange64] at hr
  rw [BitVec.toInt_sdiv]
  exact Int.bmod_eq_of_le (by show -(2^64/2 : Int) ≤ _; omega) (by rw [Int.natCast_pow]; omega)

end C
end Header
