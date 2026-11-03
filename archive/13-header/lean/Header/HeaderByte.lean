import Header.DesignA
/-!
# Design A's header byte without `and D`

Round 1 rerun for A after the change of Nov 1, 2026: with bits 0–5 of the header always 0, the
header of direction `d` is the byte `64·d`, and the header rules of spec v0.2 §3 drop every
`and D` (`D = 0xC0`). This file proves that each rule, without the mask, gives the header byte of
round 1's result (`A.add`, `A.neg`, `A.sub`, `A.mul`, `A.div`), with the byte read mod 256 (a
stored byte) after any 64-bit register arithmetic. Round 1's value theorems then apply unchanged.
-/

namespace Header
namespace A

/-- The header byte of direction `d`: bits 6–7 hold `d`, bits 0–5 are 0. -/
def hb (d : Fin 4) : Nat := 64 * d.val

/-- The register word size; a stored header byte is a register value mod 256. -/
def R : Nat := 2 ^ 64

theorem hb_lt (d : Fin 4) : hb d < 256 := by revert d; decide
theorem hb_low (d : Fin 4) : hb d &&& 0x3F = 0 := by revert d; decide
theorem hb_inj (d1 d2 : Fin 4) : hb d1 = hb d2 ↔ d1 = d2 := by revert d1 d2; decide

/-- The direction check without the mask: `h1 xor h2` is already `64·(d1 xor d2)`. -/
theorem hb_xor (d1 d2 : Fin 4) : hb d1 ^^^ hb d2 = 64 * dirCheck d1 d2 := by revert d1 d2; decide

/-- So the mask on `t` changes nothing. -/
theorem t_mask (d1 d2 : Fin 4) : (hb d1 ^^^ hb d2) &&& 0xC0 = hb d1 ^^^ hb d2 := by
  revert d1 d2; decide

/-- The borrow path's header `h1 + (b shl 7)`, stored as a byte. -/
theorem hb_borrow (d : Fin 4) (b : Bool) :
    (hb d + (if b then 1 else 0) * 128) % R % 256 = hb (d + (if b then 2 else 0)) := by
  revert d b; decide

/-- Negation: `h xor 0x80`. -/
theorem hb_neg (d : Fin 4) : hb d ^^^ 0x80 = hb (d + 2) := by revert d; decide

/-- Multiplication: `h1 + h2`, stored as a byte. -/
theorem hb_mul (d1 d2 : Fin 4) : (hb d1 + hb d2) % R % 256 = hb (d1 + d2) := by
  revert d1 d2; decide

/-- Division: `h1 - h2` in a 64-bit register, stored as a byte. -/
theorem hb_div (d1 d2 : Fin 4) : (hb d1 + (R - hb d2)) % R % 256 = hb (d1 - d2) := by
  revert d1 d2; decide

/-- A's addition on header bytes, spec v0.2 §3 with no `and D`. -/
def addByte (N : Nat) (h1 m1 h2 m2 : Nat) : Option (Nat × Nat) :=
  let t := h1 ^^^ h2
  if t = 0 then some (h1, clockAdd N m1 m2)
  else if t = 0x80 then
    some ((h1 + (if borrow m1 m2 then 1 else 0) * 128) % R % 256, absDiffClock N m1 m2)
  else if m1 = 0 then some (h2, clockAdd N m1 m2)
  else if m2 = 0 then some (h1, clockAdd N m1 m2)
  else none

/-- The encoding of a Design A number as (header byte, magnitude). -/
def enc (x : Num) : Nat × Nat := (hb x.d, x.m)

theorem addByte_eq (N : Nat) (x y : Num) :
    addByte N (hb x.d) x.m (hb y.d) y.m = (add N x y).map enc := by
  obtain ⟨m1, d1⟩ := x
  obtain ⟨m2, d2⟩ := y
  simp only [addByte, add, hb_xor]
  have h0 : (64 * dirCheck d1 d2 = 0) ↔ dirCheck d1 d2 = 0 := by omega
  have h2 : (64 * dirCheck d1 d2 = 0x80) ↔ dirCheck d1 d2 = 2 := by omega
  by_cases a : dirCheck d1 d2 = 0
  · simp [a, enc]
  · by_cases b : dirCheck d1 d2 = 2
    · have : 64 * dirCheck d1 d2 ≠ 0 := fun h => a (h0.mp h)
      simp [b, enc, hb_borrow]
    · have e1 : ¬ (64 * dirCheck d1 d2 = 0) := fun h => a (h0.mp h)
      have e2 : ¬ (64 * dirCheck d1 d2 = 0x80) := fun h => b (h2.mp h)
      simp only [e1, e2, a, b, ite_false]
      by_cases z1 : m1 = 0
      · simp [z1, enc]
      · by_cases z2 : m2 = 0 <;> simp [z1, z2, enc]

/-- Subtraction is addition with `h2 xor 0x80`. -/
theorem subByte_eq (N : Nat) (x y : Num) :
    addByte N (hb x.d) x.m (hb y.d ^^^ 0x80) y.m = (sub N x y).map enc := by
  rw [hb_neg]; exact addByte_eq N x (neg y)

theorem negByte_eq (x : Num) : (hb x.d ^^^ 0x80, x.m) = enc (neg x) := by
  simp [enc, neg, hb_neg]

theorem mulByte_eq (x y : Num) (W : Nat) :
    ((hb x.d + hb y.d) % R % 256, x.m * y.m % W) = ((enc (mul x y)).1, (mul x y).m % W) := by
  simp [enc, mul, hb_mul]

theorem divByte_eq (x y : Num) : ((hb x.d + (R - hb y.d)) % R % 256, x.m / y.m) = enc (div x y) := by
  simp [enc, div, hb_div]

/-- Every result header is again a canonical byte: below 256 with bits 0–5 clear. -/
theorem result_canonical (d : Fin 4) : hb d < 256 ∧ hb d &&& 0x3F = 0 := ⟨hb_lt d, hb_low d⟩

end A
end Header
