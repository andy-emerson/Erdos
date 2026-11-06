/-!
# Gaussian integers from scratch (core Lean only)

`GInt` is `a + b i` with `a b : Int`.  We define addition, negation, subtraction,
multiplication, the norm `N(a+bi) = a² + b²` (as a natural number, via `natAbs`),
the four units, the prime `π = 1 + i`, divisibility, and exact division by `π`.
-/

@[ext] structure GInt where
  re : Int
  im : Int
deriving DecidableEq, Repr

namespace GInt

instance : Add GInt := ⟨fun z w => ⟨z.re + w.re, z.im + w.im⟩⟩
instance : Neg GInt := ⟨fun z => ⟨-z.re, -z.im⟩⟩
instance : Sub GInt := ⟨fun z w => ⟨z.re - w.re, z.im - w.im⟩⟩
instance : Mul GInt := ⟨fun z w => ⟨z.re * w.re - z.im * w.im, z.re * w.im + z.im * w.re⟩⟩
instance : OfNat GInt n := ⟨⟨(n : Int), 0⟩⟩

/-- The imaginary unit. -/
def I : GInt := ⟨0, 1⟩
/-- The prime `π = 1 + i`. -/
def pi : GInt := ⟨1, 1⟩

@[simp] theorem add_re (z w : GInt) : (z + w).re = z.re + w.re := rfl
@[simp] theorem add_im (z w : GInt) : (z + w).im = z.im + w.im := rfl
@[simp] theorem neg_re (z : GInt) : (-z).re = -z.re := rfl
@[simp] theorem neg_im (z : GInt) : (-z).im = -z.im := rfl
@[simp] theorem sub_re (z w : GInt) : (z - w).re = z.re - w.re := rfl
@[simp] theorem sub_im (z w : GInt) : (z - w).im = z.im - w.im := rfl
@[simp] theorem mul_re (z w : GInt) : (z * w).re = z.re * w.re - z.im * w.im := rfl
@[simp] theorem mul_im (z w : GInt) : (z * w).im = z.re * w.im + z.im * w.re := rfl
@[simp] theorem ofNat_re (n : Nat) : (OfNat.ofNat n : GInt).re = (n : Int) := rfl
@[simp] theorem ofNat_im (n : Nat) : (OfNat.ofNat n : GInt).im = 0 := rfl
@[simp] theorem one_re : (1 : GInt).re = 1 := rfl
@[simp] theorem one_im : (1 : GInt).im = 0 := rfl
@[simp] theorem zero_re : (0 : GInt).re = 0 := rfl
@[simp] theorem zero_im : (0 : GInt).im = 0 := rfl
@[simp] theorem I_re : I.re = 0 := rfl
@[simp] theorem I_im : I.im = 1 := rfl
@[simp] theorem pi_re : pi.re = 1 := rfl
@[simp] theorem pi_im : pi.im = 1 := rfl

theorem mul_assoc (x y z : GInt) : x * y * z = x * (y * z) := by
  ext <;> simp <;> grind
theorem mul_comm (x y : GInt) : x * y = y * x := by
  ext <;> simp <;> grind
theorem one_mul (x : GInt) : 1 * x = x := by
  ext <;> simp
theorem mul_one (x : GInt) : x * 1 = x := by
  ext <;> simp
theorem mul_sub (x y z : GInt) : x * (y - z) = x * y - x * z := by
  ext <;> simp <;> grind

/-- The norm `N(a+bi) = a² + b²`, as a natural number. -/
def N (z : GInt) : Nat := z.re.natAbs * z.re.natAbs + z.im.natAbs * z.im.natAbs

theorem N_cast (z : GInt) : (N z : Int) = z.re * z.re + z.im * z.im := by
  simp only [N, Int.natCast_add, Int.natAbs_mul_self]

/-- The units `1, i, -1, -i`. -/
def IsUnit (u : GInt) : Prop := u = 1 ∨ u = I ∨ u = -1 ∨ u = -I

/-- Divisibility in `ℤ[i]`. -/
def dvd (x y : GInt) : Prop := ∃ q, y = x * q
instance : _root_.Dvd GInt := ⟨GInt.dvd⟩

theorem dvd_def (x y : GInt) : (x ∣ y) = ∃ q, y = x * q := rfl

theorem dvd_trans {x y z : GInt} (h1 : x ∣ y) (h2 : y ∣ z) : x ∣ z := by
  obtain ⟨p, rfl⟩ := h1; obtain ⟨q, rfl⟩ := h2
  exact ⟨p * q, mul_assoc _ _ _⟩

/-- Powers: `pow z 0 = 1`, `pow z (n+1) = pow z n * z`. -/
def pow (z : GInt) : Nat → GInt
  | 0 => 1
  | n + 1 => pow z n * z

theorem pow_add (z : GInt) (m n : Nat) : pow z (m + n) = pow z m * pow z n := by
  induction n with
  | zero => simp [pow, mul_one]
  | succ n ih => rw [← Nat.add_assoc, pow, pow, ih, mul_assoc]

/-- `z` is even iff `π ∣ z`; odd otherwise. -/
def Even (z : GInt) : Prop := pi ∣ z
def Odd (z : GInt) : Prop := ¬ pi ∣ z

/-- `π ∣ a + bi` iff `a ≡ b (mod 2)`. -/
theorem pi_dvd_iff (z : GInt) : pi ∣ z ↔ (z.re - z.im) % 2 = 0 := by
  constructor
  · rintro ⟨q, rfl⟩
    simp only [mul_re, mul_im, pi_re, pi_im]; omega
  · intro h
    refine ⟨⟨(z.re + z.im) / 2, (z.im - z.re) / 2⟩, ?_⟩
    ext <;> simp <;> omega

instance (z : GInt) : Decidable (pi ∣ z) :=
  decidable_of_iff _ (pi_dvd_iff z).symm

/-- Division by `π`: `(a+bi)/(1+i) = ((a+b) + (b-a)i)/2`. Exact when `π ∣ z`. -/
def divPi (z : GInt) : GInt := ⟨(z.re + z.im) / 2, (z.im - z.re) / 2⟩

theorem divPi_mul (z : GInt) (h : pi ∣ z) : pi * divPi z = z := by
  rw [pi_dvd_iff] at h
  ext <;> simp [divPi] <;> omega

/-- Iteration: `iter f 0 z = z`, `iter f (n+1) z = iter f n (f z)`. -/
def iter (f : GInt → GInt) : Nat → GInt → GInt
  | 0, z => z
  | n + 1, z => iter f n (f z)

theorem iter_add (f : GInt → GInt) (m n : Nat) (z : GInt) :
    iter f (m + n) z = iter f n (iter f m z) := by
  induction m generalizing z with
  | zero => simp [iter]
  | succ m ih => rw [Nat.add_right_comm, iter, ih, iter]

end GInt
