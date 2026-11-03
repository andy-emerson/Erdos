import Header.HeaderByte
/-!
# Design A's multiplication row (round 1)

`aAddMul` / `aSubMul` of `../src/designa.nim`: A's add with the notes' exponent rule as code, no branch. Signs are
applied by multiplying by `1 − 2k` (1 or −1 on the 64-bit clock), the cross-axis choice of direction by multiplying by
a 0/1 flag, and the headers carry no `and D` (`HeaderByte.lean`):

* `t = h1 xor h2`, `ax = (t >> 6) and 1`, `o = (t >> 7) and (ax xor 1)`;
* `z1 = (m1 = 0)`, `nz2 = (m2 ≠ 0)`, `err = ax and (z1 xor 1) and nz2`;
* `r = m1 + m2·(1 − 2o)` and the magnitude `r·(1 − 2b)` with `b = (m1 < m2) and o`, all mod `2^64`;
* `k = ax and z1`, the header `(h1 + k·(h2 − h1)) mod 256 + (b shl 7)`, as a byte.

`addMul_eq` proves, for every input (headers `hb d`, magnitudes below `2^64`), that when round 1's `A.add W x y` gives
a number the row gives its header byte and magnitude with `err = 0`, and that `err = 1` exactly when `A.add` gives an
expression. `subMul_eq` proves the same for subtraction (the row with `h2 xor 0x80`) against `A.sub`.
-/

namespace Header
namespace A

/-- A 64-bit word's modulus, as a literal. -/
def Wd : Nat := 18446744073709551616

/-- `1 − 2k` on the 64-bit clock. -/
def sgn (k : Nat) : Nat := (1 + Wd - 2 * k) % Wd

theorem sgn_0 : sgn 0 = 1 := by decide
theorem sgn_1 : sgn 1 = 18446744073709551615 := by decide

/-- Multiplying by `−1` on the clock is negation. -/
theorem mul_ones (x : Nat) :
    (x * 18446744073709551615) % 18446744073709551616 = (18446744073709551616 - x % 18446744073709551616) % 18446744073709551616 := by
  omega

theorem hb_0 : hb 0 = 0 := by decide
theorem hb_1 : hb 1 = 64 := by decide
theorem hb_2 : hb 2 = 128 := by decide
theorem hb_3 : hb 3 = 192 := by decide

theorem one_mod_two : 1 % 2 = 1 := rfl

/-- The row: `(header byte, magnitude, err)`. Every operation is the machine's, on bytes or 64-bit words. -/
def addMul (h1 m1 h2 m2 : Nat) : Nat × Nat × Nat :=
  let t := h1 ^^^ h2
  let ax := (t >>> 6) &&& 1
  let o := (t >>> 7) &&& (ax ^^^ 1)
  let z1 := if m1 = 0 then 1 else 0
  let nz2 := if m2 ≠ 0 then 1 else 0
  let err := ax &&& (z1 ^^^ 1) &&& nz2
  let r := (m1 + m2 * sgn o) % Wd
  let b := (if m1 < m2 then 1 else 0) &&& o
  let mag := (r * sgn b) % Wd
  let k := ax &&& z1
  let hd := (h1 + k * ((h2 + Wd - h1) % Wd)) % Wd
  (((hd &&& 0xFF) + (b <<< 7)) % 256, mag, err)

/-- Subtraction: the row with `h2 xor 0x80`. -/
def subMul (h1 m1 h2 m2 : Nat) : Nat × Nat × Nat := addMul h1 m1 (h2 ^^^ 0x80) m2

theorem Wd_eq : Wd = 2 ^ 64 := rfl

/-- **The multiplication row is A's add.** -/
theorem addMul_eq (x y : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) :
    match add (2 ^ 64) x y with
    | some r => addMul (hb x.d) x.m (hb y.d) y.m = (hb r.d, r.m, 0)
    | none => (addMul (hb x.d) x.m (hb y.d) y.m).2.2 = 1 := by
  obtain ⟨m1, d1⟩ := x
  obtain ⟨m2, d2⟩ := y
  simp only at h1 h2
  rw [← Wd_eq] at h1 h2 ⊢
  rcases fin4_cases d1 with rfl|rfl|rfl|rfl <;> rcases fin4_cases d2 with rfl|rfl|rfl|rfl
  all_goals by_cases hl : m1 < m2 <;> by_cases z1 : m1 = 0 <;> by_cases z2 : m2 = 0
  all_goals try (exfalso; omega)
  all_goals simp [add, addMul, hb_0, hb_1, hb_2, hb_3, dirCheck, clockAdd, absDiffClock, clockSub, clockNeg, borrow, hl, z1, z2, sgn_0,
    sgn_1, mul_ones, one_mod_two, Wd]
  all_goals unfold Wd at *
  all_goals (repeat' split) <;> (try simp only [one_mod_two, Nat.zero_mod, sgn_0, sgn_1, mul_ones, Nat.mul_one, Fin.reduceAdd, Fin.isValue, hb_0, hb_1, hb_2, hb_3] at *) <;> omega

/-- **Subtraction**: the row with `h2 xor 0x80` is A's subtract. -/
theorem subMul_eq (x y : Num) (h1 : x.m < 2 ^ 64) (h2 : y.m < 2 ^ 64) :
    match sub (2 ^ 64) x y with
    | some r => subMul (hb x.d) x.m (hb y.d) y.m = (hb r.d, r.m, 0)
    | none => (subMul (hb x.d) x.m (hb y.d) y.m).2.2 = 1 := by
  unfold subMul sub
  rw [hb_neg]
  exact addMul_eq x (neg y) h1 h2

end A
end Header
