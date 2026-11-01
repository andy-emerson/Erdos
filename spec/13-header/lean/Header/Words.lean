import Header.Clock
/-!
# Word-level routines (spec §1, checked in round 1 §4): add with carry, subtract with borrow, absolute difference

A magnitude is a list of words, low word first, each word `< 2^n` (`Words n`), for **any** word size `n > 0` and
**any** length. Its value is `val n [w0, w1, …] = w0 + 2^n · (w1 + 2^n · …)`.

The routines follow `../test/words.py` (`r_add`, `r_sub_into`, `r_absdiff`, Growing's opAdd, subInto, opAbsdiff2)
line by line:
* `& M` is `wrap n a = a &&& (2^n - 1)` (and `wrap_eq_mod` shows it is `a % 2^n`);
* Python's `(a - b) & M` on its signed integers is `wsub n a b = (a + 2^n - b) &&& (2^n - 1)` (the same word for
  `b ≤ 2^n`, which always holds here);
* a carry or borrow is a value, `int(s < c)` = `lt s c`, never a flag.

**Writes.** Each routine writes word `i` of its destination at step `i`, in order, from `0` (each Python loop
calls `r.set(i, …)` with `i` counting up from where the previous loop stopped). A routine is modelled by the list
of words it writes, so the list's length is the number of words written and the highest index written is the length
minus 1. "No write past the bound" is "the list is no longer than the bound".
-/

set_option linter.unusedVariables false

namespace Header
namespace Words

/-- A word's mask `M = 2^n - 1`. -/
def M (n : Nat) : Nat := 2 ^ n - 1

/-- `a & M`. -/
def wrap (n a : Nat) : Nat := a &&& M n

/-- Python's `(a - b) & M`, for `b ≤ 2^n`. -/
def wsub (n a b : Nat) : Nat := (a + 2 ^ n - b) &&& M n

/-- `int(a < b)`: a carry or borrow computed as a value. -/
def lt (a b : Nat) : Nat := if a < b then 1 else 0

/-- The value of a word list, low word first. -/
def val (n : Nat) : List Nat → Nat
  | [] => 0
  | w :: ws => w + 2 ^ n * val n ws

/-- Every word is below `2^n`. -/
def Words (n : Nat) (l : List Nat) : Prop := ∀ w ∈ l, w < 2 ^ n

/-! ## Basic facts -/

theorem wrap_eq_mod (n a : Nat) : wrap n a = a % 2 ^ n := by
  simp [wrap, M]

theorem wrap_lt (n a : Nat) : wrap n a < 2 ^ n := by
  rw [wrap_eq_mod]; exact Nat.mod_lt _ (Nat.two_pow_pos n)

theorem wsub_lt (n a b : Nat) : wsub n a b < 2 ^ n := by
  simp only [wsub, M, Nat.and_two_pow_sub_one_eq_mod]; exact Nat.mod_lt _ (Nat.two_pow_pos n)

theorem lt_le_one (a b : Nat) : lt a b ≤ 1 := by unfold lt; split <;> omega

theorem pow_succ_mul (n k : Nat) : 2 ^ (n * (k + 1)) = 2 ^ n * 2 ^ (n * k) := by
  rw [Nat.mul_succ, Nat.pow_add, Nat.mul_comm]

theorem words_nil (n : Nat) : Words n [] := by simp [Words]

theorem words_cons {n w : Nat} {ws : List Nat} : Words n (w :: ws) ↔ w < 2 ^ n ∧ Words n ws := by
  simp [Words]

theorem words_append {n : Nat} {a b : List Nat} : Words n (a ++ b) ↔ Words n a ∧ Words n b := by
  simp only [Words, List.mem_append]
  constructor
  · intro h; exact ⟨fun w hw => h w (Or.inl hw), fun w hw => h w (Or.inr hw)⟩
  · rintro ⟨h1, h2⟩ w (hw | hw)
    · exact h1 w hw
    · exact h2 w hw

/-- A `k`-word list has value below `2^(n·k)`: the `k`-word clock. -/
theorem val_lt {n : Nat} : ∀ {l : List Nat}, Words n l → val n l < 2 ^ (n * l.length)
  | [], _ => by simp [val]
  | w :: ws, h => by
    rw [words_cons] at h
    have ih := val_lt h.2
    simp only [val, List.length_cons, pow_succ_mul]
    have hB := Nat.two_pow_pos n
    -- w + B·v < B·P  since  w < B  and  v + 1 ≤ P
    have : 2 ^ n * (val n ws + 1) ≤ 2 ^ n * 2 ^ (n * ws.length) := Nat.mul_le_mul_left _ ih
    rw [Nat.mul_add] at this; omega

theorem val_append (n : Nat) : ∀ (a b : List Nat), val n (a ++ b) = val n a + 2 ^ (n * a.length) * val n b
  | [], b => by simp [val]
  | w :: ws, b => by
    simp only [List.cons_append, val, List.length_cons, val_append n ws b, pow_succ_mul]
    grind

/-- **Carry as a value.** For words `a, b < 2^n`, the wrapped sum `t` and the carry `int(t < b)` satisfy
`a + b = t + 2^n · carry`: the sum is below an operand exactly when it wrapped. -/
theorem carry_lemma {n a b : Nat} (ha : a < 2 ^ n) (hb : b < 2 ^ n) :
    a + b = wrap n (a + b) + 2 ^ n * lt (wrap n (a + b)) b := by
  rw [wrap_eq_mod]; unfold lt
  by_cases h : a + b < 2 ^ n
  · rw [Nat.mod_eq_of_lt h]; split <;> omega
  · rw [Nat.mod_eq_sub_mod (by omega), Nat.mod_eq_of_lt (by omega)]; split <;> omega

/-- The same with the comparison against the first operand, `int(t < a)`. -/
theorem carry_lemma' {n a b : Nat} (ha : a < 2 ^ n) (hb : b < 2 ^ n) :
    a + b = wrap n (a + b) + 2 ^ n * lt (wrap n (a + b)) a := by
  have := carry_lemma hb ha; rw [Nat.add_comm b a] at this; exact this

/-- **Borrow as a value.** For a word `a < 2^n` and `b ≤ 2^n`, the wrapped difference `d = (a - b) & M` and the
borrow `int(a < b)` satisfy `a + 2^n · borrow = d + b`. -/
theorem borrow_word {n a b : Nat} (ha : a < 2 ^ n) (hb : b ≤ 2 ^ n) :
    a + 2 ^ n * lt a b = wsub n a b + b := by
  simp only [wsub, M, Nat.and_two_pow_sub_one_eq_mod]; unfold lt
  by_cases h : b ≤ a
  · rw [Nat.mod_eq_sub_mod (by omega), Nat.mod_eq_of_lt (by omega)]; split <;> omega
  · rw [Nat.mod_eq_of_lt (by omega)]; split <;> omega

/-- XOR with the all-ones word is the one's complement. -/
theorem xor_mask {n r : Nat} (h : r < 2 ^ n) : r ^^^ M n = 2 ^ n - 1 - r := by
  have := @BitVec.toNat_not n (BitVec.ofNat n r)
  rw [BitVec.not_def, BitVec.toNat_xor, BitVec.toNat_allOnes, BitVec.toNat_ofNat, Nat.mod_eq_of_lt h] at this
  rw [M, Nat.xor_comm]; exact this

/-! ## 1. Multi-word add with carry (`r_add`) -/

/-- The tail loop: `t = (x_i + c) & M; c = int(t < c)`. -/
def addTail (n : Nat) : List Nat → Nat → List Nat × Nat
  | [], c => ([], c)
  | x :: xs, c =>
    let t := wrap n (x + c)
    let r := addTail n xs (lt t c)
    (t :: r.1, r.2)

/-- The common loop: `s = (x_i + c) & M; c1 = int(s < c); t = (s + y_i) & M; c = c1 + int(t < s)`; when `y`
runs out, the tail loop on the rest of `x`. (`y` is never the longer: `add` swaps first.) -/
def addCommon (n : Nat) : List Nat → List Nat → Nat → List Nat × Nat
  | x :: xs, y :: ys, c =>
    let s := wrap n (x + c)
    let c1 := lt s c
    let t := wrap n (s + y)
    let r := addCommon n xs ys (c1 + lt t s)
    (t :: r.1, r.2)
  | xs, _, c => addTail n xs c

/-- `r_add`: swap so that `x` is the longer; the two loops from carry 0; then the carry is written as word `nx`. -/
def add (n : Nat) (x y : List Nat) : List Nat :=
  let p := if x.length < y.length then (y, x) else (x, y)
  let r := addCommon n p.1 p.2 0
  r.1 ++ [r.2]

theorem addTail_spec {n : Nat} (hn : 0 < n) : ∀ (xs : List Nat) (c : Nat), Words n xs → c ≤ 1 →
    (addTail n xs c).1.length = xs.length ∧ Words n (addTail n xs c).1 ∧ (addTail n xs c).2 ≤ 1 ∧
    val n (addTail n xs c).1 + 2 ^ (n * xs.length) * (addTail n xs c).2 = val n xs + c
  | [], c, _, hc => by simp [addTail, val, words_nil] <;> omega
  | x :: xs, c, hw, hc => by
    rw [words_cons] at hw
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have key := carry_lemma (n := n) hw.1 (show c < 2 ^ n by omega)
    obtain ⟨h1, h2, h3, h4⟩ := addTail_spec hn xs (lt (wrap n (x + c)) c) hw.2 (lt_le_one _ _)
    simp only [addTail, val, List.length_cons, pow_succ_mul, words_cons]
    refine ⟨by omega, ⟨wrap_lt _ _, h2⟩, h3, ?_⟩
    grind

theorem addCommon_spec {n : Nat} (hn : 0 < n) : ∀ (xs ys : List Nat) (c : Nat), Words n xs → Words n ys →
    ys.length ≤ xs.length → c ≤ 1 →
    (addCommon n xs ys c).1.length = xs.length ∧ Words n (addCommon n xs ys c).1 ∧
    (addCommon n xs ys c).2 ≤ 1 ∧
    val n (addCommon n xs ys c).1 + 2 ^ (n * xs.length) * (addCommon n xs ys c).2 = val n xs + val n ys + c
  | [], [], c, _, _, _, hc => by simp [addCommon, addTail, val, words_nil] <;> omega
  | [], _ :: _, c, _, _, hl, _ => by simp at hl
  | x :: xs, [], c, hx, _, _, hc => by
    obtain ⟨a1, a2, a3, a4⟩ := addTail_spec hn (x :: xs) c hx hc
    simp only [addCommon]; simp only [val] at a4 ⊢; exact ⟨a1, a2, a3, by omega⟩
  | x :: xs, y :: ys, c, hx, hy, hl, hc => by
    rw [words_cons] at hx hy
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have k1 := carry_lemma (n := n) hx.1 (show c < 2 ^ n by omega)
    have k2 := carry_lemma' (n := n) (wrap_lt n (x + c)) hy.1
    -- the new carry is at most 1: if the first add wrapped, its word is 0 and the second cannot wrap
    have hc' : lt (wrap n (x + c)) c + lt (wrap n (wrap n (x + c) + y)) (wrap n (x + c)) ≤ 1 := by
      have := lt_le_one (wrap n (x + c)) c
      have := lt_le_one (wrap n (wrap n (x + c) + y)) (wrap n (x + c))
      unfold lt at *; split <;> split <;> omega
    obtain ⟨h1, h2, h3, h4⟩ := addCommon_spec hn xs ys _ hx.2 hy.2 (by simp at hl; omega) hc'
    simp only [addCommon, val, List.length_cons, pow_succ_mul, words_cons]
    refine ⟨by omega, ⟨wrap_lt _ _, h2⟩, h3, ?_⟩
    grind

/-- **Add with carry.** For any word size `n > 0` and operands `x, y` of any lengths (words `< 2^n`), with
`k = max(len x, len y)`: `r_add` writes exactly `k + 1` words (the bound `max + 1`, never more); the top word, at
index `k`, is the final carry, which is 0 or 1; all words are words; and the first `k` result words plus the final
carry times `2^(n·k)` equal the sum of the two values. -/
theorem add_spec {n : Nat} (hn : 0 < n) {x y : List Nat} (hx : Words n x) (hy : Words n y) :
    ∃ rs c, add n x y = rs ++ [c] ∧ rs.length = max x.length y.length ∧ c ≤ 1 ∧
      Words n (add n x y) ∧ (add n x y).length = max x.length y.length + 1 ∧
      val n rs + c * 2 ^ (n * max x.length y.length) = val n x + val n y := by
  have hB : 2 ≤ 2 ^ n := by
    have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
  unfold add
  by_cases hl : x.length < y.length
  · obtain ⟨h1, h2, h3, h4⟩ := addCommon_spec hn y x 0 hy hx (by omega) (by omega)
    simp only [hl, ite_true]
    refine ⟨_, _, rfl, by rw [h1]; omega, h3, words_append.2 ⟨h2, by simp [Words]; omega⟩,
      by simp [h1]; omega, ?_⟩
    have : max x.length y.length = y.length := by omega
    rw [this]; grind
  · obtain ⟨h1, h2, h3, h4⟩ := addCommon_spec hn x y 0 hx hy (by omega) (by omega)
    simp only [hl, ite_false]
    refine ⟨_, _, rfl, by rw [h1]; omega, h3, words_append.2 ⟨h2, by simp [Words]; omega⟩,
      by simp [h1]; omega, ?_⟩
    have : max x.length y.length = x.length := by omega
    rw [this]; grind

/-- Read as one number, the `k + 1` words are the sum. -/
theorem val_add {n : Nat} (hn : 0 < n) {x y : List Nat} (hx : Words n x) (hy : Words n y) :
    val n (add n x y) = val n x + val n y := by
  obtain ⟨rs, c, he, hl, -, -, -, hv⟩ := add_spec hn hx hy
  rw [he, val_append, hl]; simp [val]; grind

/-! ## 2. Subtract with borrow (`r_sub_into`) -/

/-- Tail when `x` is the longer (`la ≥ lb`): `r_i = (p - br) & M; br = int(p < br)`. -/
def subTailX (n : Nat) : List Nat → Nat → List Nat × Nat
  | [], br => ([], br)
  | p :: xs, br =>
    let r := subTailX n xs (lt p br)
    (wsub n p br :: r.1, r.2)

/-- Tail when `y` is the longer: `x` is read as zero past its end. `d = (0 - q) & M; r_i = (d - br) & M;
br = int(q != 0) + int(d < br)`. -/
def subTailY (n : Nat) : List Nat → Nat → List Nat × Nat
  | [], br => ([], br)
  | q :: ys, br =>
    let d := wsub n 0 q
    let r := subTailY n ys ((if q ≠ 0 then 1 else 0) + lt d br)
    (wsub n d br :: r.1, r.2)

/-- The common loop: `d = (p - q) & M; r_i = (d - br) & M; br = int(p < q) + int(d < br)`; then the tail of the
longer operand. -/
def subCommon (n : Nat) : List Nat → List Nat → Nat → List Nat × Nat
  | p :: xs, q :: ys, br =>
    let d := wsub n p q
    let r := subCommon n xs ys (lt p q + lt d br)
    (wsub n d br :: r.1, r.2)
  | xs, [], br => subTailX n xs br
  | [], ys, br => subTailY n ys br

/-- `r_sub_into`: `x - y` over `max(la, lb)` words, from borrow 0; returns the words and the final borrow. -/
def subInto (n : Nat) (x y : List Nat) : List Nat × Nat := subCommon n x y 0

theorem subTailX_spec {n : Nat} (hn : 0 < n) : ∀ (xs : List Nat) (br : Nat), Words n xs → br ≤ 1 →
    (subTailX n xs br).1.length = xs.length ∧ Words n (subTailX n xs br).1 ∧ (subTailX n xs br).2 ≤ 1 ∧
    val n (subTailX n xs br).1 + br = val n xs + 2 ^ (n * xs.length) * (subTailX n xs br).2
  | [], br, _, hb => by simp [subTailX, val, words_nil] <;> omega
  | p :: xs, br, hw, hb => by
    rw [words_cons] at hw
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have k := borrow_word (n := n) hw.1 (show br ≤ 2 ^ n by omega)
    obtain ⟨h1, h2, h3, h4⟩ := subTailX_spec hn xs (lt p br) hw.2 (lt_le_one _ _)
    simp only [subTailX, val, List.length_cons, pow_succ_mul, words_cons]
    refine ⟨by omega, ⟨wsub_lt _ _ _, h2⟩, h3, ?_⟩
    grind

theorem subTailY_spec {n : Nat} (hn : 0 < n) : ∀ (ys : List Nat) (br : Nat), Words n ys → br ≤ 1 →
    (subTailY n ys br).1.length = ys.length ∧ Words n (subTailY n ys br).1 ∧ (subTailY n ys br).2 ≤ 1 ∧
    val n (subTailY n ys br).1 + val n ys + br = 2 ^ (n * ys.length) * (subTailY n ys br).2
  | [], br, _, hb => by simp [subTailY, val, words_nil] <;> omega
  | q :: ys, br, hw, hb => by
    rw [words_cons] at hw
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have k1 := borrow_word (n := n) (a := 0) (by omega) (show q ≤ 2 ^ n by omega)
    have k2 := borrow_word (n := n) (wsub_lt n 0 q) (show br ≤ 2 ^ n by omega)
    have hq : lt 0 q = if q ≠ 0 then 1 else 0 := by unfold lt; split <;> split <;> omega
    have hb' : (if q ≠ 0 then 1 else 0) + lt (wsub n 0 q) br ≤ 1 := by
      by_cases hq0 : q = 0
      · simp only [hq0, ne_eq, not_true_eq_false, ite_false, Nat.zero_add]; exact lt_le_one _ _
      · have e1 : lt 0 q = 1 := by simp [lt]; omega
        rw [e1] at k1
        have e2 : lt (wsub n 0 q) br = 0 := by simp [lt]; omega
        simp [hq0, e2]
    obtain ⟨h1, h2, h3, h4⟩ := subTailY_spec hn ys _ hw.2 hb'
    simp only [subTailY, val, List.length_cons, pow_succ_mul, words_cons]
    refine ⟨by omega, ⟨wsub_lt _ _ _, h2⟩, h3, ?_⟩
    rw [← hq] at h4
    grind

theorem subCommon_spec {n : Nat} (hn : 0 < n) : ∀ (xs ys : List Nat) (br : Nat), Words n xs → Words n ys →
    br ≤ 1 →
    (subCommon n xs ys br).1.length = max xs.length ys.length ∧ Words n (subCommon n xs ys br).1 ∧
    (subCommon n xs ys br).2 ≤ 1 ∧
    val n (subCommon n xs ys br).1 + val n ys + br =
      val n xs + 2 ^ (n * max xs.length ys.length) * (subCommon n xs ys br).2
  | xs, [], br, hx, _, hb => by
    obtain ⟨a1, a2, a3, a4⟩ := subTailX_spec hn xs br hx hb
    have hm : max xs.length ([] : List Nat).length = xs.length := by simp
    have e : subCommon n xs [] br = subTailX n xs br := by cases xs <;> rfl
    rw [hm, e]; simp only [val] at a4 ⊢; exact ⟨a1, a2, a3, by omega⟩
  | [], q :: ys, br, _, hy, hb => by
    obtain ⟨a1, a2, a3, a4⟩ := subTailY_spec hn (q :: ys) br hy hb
    have hm : max ([] : List Nat).length (q :: ys).length = (q :: ys).length := by simp
    rw [hm]; simp only [subCommon]; simp only [val] at a4 ⊢; exact ⟨a1, a2, a3, by omega⟩
  | p :: xs, q :: ys, br, hx, hy, hb => by
    rw [words_cons] at hx hy
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have k1 := borrow_word (n := n) hx.1 (show q ≤ 2 ^ n by omega)
    have k2 := borrow_word (n := n) (wsub_lt n p q) (show br ≤ 2 ^ n by omega)
    -- the new borrow is at most 1: if p < q, then d = p + 2^n - q ≥ 1 ≥ br
    have hb' : lt p q + lt (wsub n p q) br ≤ 1 := by
      by_cases hpq : p < q
      · have e1 : lt p q = 1 := by simp [lt, hpq]
        rw [e1] at k1
        have e2 : lt (wsub n p q) br = 0 := by simp [lt]; omega
        rw [e1, e2]; omega
      · have e1 : lt p q = 0 := by simp [lt, hpq]
        rw [e1]; have := lt_le_one (wsub n p q) br; omega
    obtain ⟨h1, h2, h3, h4⟩ := subCommon_spec hn xs ys _ hx.2 hy.2 hb'
    simp only [subCommon, val, List.length_cons, words_cons]
    have hm : max (xs.length + 1) (ys.length + 1) = max xs.length ys.length + 1 := by omega
    rw [hm, pow_succ_mul]
    refine ⟨by omega, ⟨wsub_lt _ _ _, h2⟩, h3, ?_⟩
    grind

/-- **Subtract with borrow, on the `k`-word clock.** With `k = max(len x, len y)` and `N = 2^(n·k)`: `r_sub_into`
writes exactly `k` words; their value is the clock difference `clockSub N (val x) (val y)` of `Clock.lean`; and the
final borrow is `1` exactly when `val x < val y`, i.e. it is the value-level `borrow (val x) (val y)`. -/
theorem subInto_spec {n : Nat} (hn : 0 < n) {x y : List Nat} (hx : Words n x) (hy : Words n y) :
    (subInto n x y).1.length = max x.length y.length ∧ Words n (subInto n x y).1 ∧
    val n (subInto n x y).1 = clockSub (2 ^ (n * max x.length y.length)) (val n x) (val n y) ∧
    (subInto n x y).2 = (if borrow (val n x) (val n y) then 1 else 0) := by
  obtain ⟨h1, h2, h3, h4⟩ := subCommon_spec hn x y 0 hx hy (by omega)
  have vx := val_lt hx; have vy := val_lt hy; have vr := val_lt h2
  have px : 2 ^ (n * x.length) ≤ 2 ^ (n * max x.length y.length) :=
    Nat.pow_le_pow_right (by decide) (Nat.mul_le_mul_left _ (by omega))
  have py : 2 ^ (n * y.length) ≤ 2 ^ (n * max x.length y.length) :=
    Nat.pow_le_pow_right (by decide) (Nat.mul_le_mul_left _ (by omega))
  rw [h1] at vr
  simp only [subInto]
  generalize 2 ^ (n * max x.length y.length) = N at *
  generalize (subCommon n x y 0).2 = b at *
  generalize val n (subCommon n x y 0).1 = r at *
  refine ⟨h1, h2, ?_, ?_⟩
  · unfold clockSub
    have : b = 0 ∨ b = 1 := by omega
    rcases this with rfl | rfl <;> split <;> omega
  · unfold borrow
    have : b = 0 ∨ b = 1 := by omega
    rcases this with rfl | rfl <;> simp <;> omega

/-! ## Absolute difference (`r_absdiff`) -/

/-- The negation pass: `t = ((r_i xor m) + c) & M; c = int(t < c)`. -/
def negPass (n m : Nat) : List Nat → Nat → List Nat
  | [], _ => []
  | r :: rs, c =>
    let t := wrap n ((r ^^^ m) + c)
    t :: negPass n m rs (lt t c)

/-- `r_absdiff`: `r_sub_into`, then the pass with `m = (0 - br) & M` and carry `br`, over the same `max` words;
returns the words and the borrow. -/
def absDiff (n : Nat) (x y : List Nat) : List Nat × Nat :=
  let s := subInto n x y
  (negPass n (wsub n 0 s.2) s.1 s.2, s.2)

theorem negPass_length (n m : Nat) : ∀ (rs : List Nat) (c : Nat), (negPass n m rs c).length = rs.length
  | [], _ => rfl
  | r :: rs, c => by simp [negPass, negPass_length n m rs]

theorem negPass_words (n m : Nat) : ∀ (rs : List Nat) (c : Nat), Words n (negPass n m rs c)
  | [], _ => words_nil n
  | r :: rs, c => by simp only [negPass, words_cons]; exact ⟨wrap_lt _ _, negPass_words n m rs _⟩

/-- With `m = 0` and carry 0 the pass rewrites every word unchanged. -/
theorem negPass_zero {n : Nat} : ∀ (rs : List Nat), Words n rs → negPass n 0 rs 0 = rs
  | [], _ => rfl
  | r :: rs, h => by
    rw [words_cons] at h
    simp only [negPass, Nat.xor_zero, Nat.add_zero, wrap_eq_mod, Nat.mod_eq_of_lt h.1]
    have : lt r 0 = 0 := by simp [lt]
    rw [this, negPass_zero rs h.2]

/-- With `m = M` (all ones) the pass adds `c` to the one's complement: `out + rs + 1 + 2^(n·k)·c' = 2^(n·k) + c`. -/
theorem negPass_ones {n : Nat} (hn : 0 < n) : ∀ (rs : List Nat) (c : Nat), Words n rs → c ≤ 1 →
    ∃ c', c' ≤ 1 ∧ val n (negPass n (M n) rs c) + val n rs + 1 + 2 ^ (n * rs.length) * c' =
      2 ^ (n * rs.length) + c
  | [], c, _, hc => ⟨c, hc, by simp [negPass, val] <;> omega⟩
  | r :: rs, c, h, hc => by
    rw [words_cons] at h
    have hB : 2 ≤ 2 ^ n := by
      have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
    have hx := xor_mask h.1
    have k := carry_lemma (n := n) (a := r ^^^ M n) (b := c) (by omega) (by omega)
    obtain ⟨c', hc', ih⟩ := negPass_ones hn rs (lt (wrap n ((r ^^^ M n) + c)) c) h.2 (lt_le_one _ _)
    refine ⟨c', hc', ?_⟩
    have hu : (r ^^^ M n) + r + 1 = 2 ^ n := by rw [hx]; omega
    simp only [negPass, val, List.length_cons, pow_succ_mul]
    generalize wrap n ((r ^^^ M n) + c) = t at *
    generalize r ^^^ M n = u at *
    generalize lt t c = c1 at *
    generalize val n (negPass n (M n) rs c1) = o at *
    generalize 2 ^ (n * rs.length) = P at *
    generalize 2 ^ n = B at *
    grind

/-- On the value, the pass is the clock negation of `Clock.lean` when the borrow is 1, and nothing when it is 0:
`val (pass) = clockNeg N (val rs)` (`N = 2^(n·k)`), for the pass `r_absdiff` runs. -/
theorem negPass_clock {n : Nat} (hn : 0 < n) {rs : List Nat} (h : Words n rs) (b : Nat) (hb : b ≤ 1) :
    val n (negPass n (wsub n 0 b) rs b) =
      if b = 1 then clockNeg (2 ^ (n * rs.length)) (val n rs) else val n rs := by
  have hB : 2 ≤ 2 ^ n := by
    have := Nat.pow_le_pow_right (by decide : 0 < 2) hn; simpa using this
  have vr := val_lt h
  have : b = 0 ∨ b = 1 := by omega
  rcases this with rfl | rfl
  · have : wsub n 0 0 = 0 := by
      simp [wsub, M, Nat.and_two_pow_sub_one_eq_mod]
    simp [this, negPass_zero rs h]
  · have hm : wsub n 0 1 = M n := by
      simp only [wsub, M, Nat.and_two_pow_sub_one_eq_mod, Nat.zero_add]
      exact Nat.mod_eq_of_lt (by omega)
    rw [hm]
    obtain ⟨c', hc', e⟩ := negPass_ones hn rs 1 h (by omega)
    have vo := val_lt (negPass_words n (M n) rs 1)
    rw [negPass_length] at vo
    simp only [ite_true, clockNeg]
    generalize 2 ^ (n * rs.length) = N at *
    generalize val n (negPass n (M n) rs 1) = o at *
    have : c' = 0 ∨ c' = 1 := by omega
    rcases this with rfl | rfl <;> split <;> omega

/-- **Absolute difference with borrow.** For any `n > 0` and operands of any lengths, with
`k = max(len x, len y)`: `r_absdiff` writes exactly `k` words in each of its two passes (the bound `max`, never
more); the result is `|val x − val y|`; the returned borrow is `1` exactly when `val x < val y`. Tied to
`Clock.lean`: on the `k`-word clock `N = 2^(n·k)`, the first pass is `clockSub N`, the second is `clockNeg N` when
the borrow is 1, and the result is `absDiffClock N (val x) (val y)`, which the value-level `borrow_lemma` shows is
`absDiff (val x) (val y)`. -/
theorem absDiff_spec {n : Nat} (hn : 0 < n) {x y : List Nat} (hx : Words n x) (hy : Words n y) :
    (subInto n x y).1.length = max x.length y.length ∧
    (absDiff n x y).1.length = max x.length y.length ∧ Words n (absDiff n x y).1 ∧
    val n (absDiff n x y).1 = absDiffClock (2 ^ (n * max x.length y.length)) (val n x) (val n y) ∧
    val n (absDiff n x y).1 = Header.absDiff (val n x) (val n y) ∧
    ((absDiff n x y).2 = 1 ↔ val n x < val n y) ∧ (absDiff n x y).2 ≤ 1 := by
  obtain ⟨h1, h2, h3, h4⟩ := subInto_spec hn hx hy
  have hb : (subInto n x y).2 ≤ 1 := by rw [h4]; split <;> omega
  have vx := val_lt hx; have vy := val_lt hy
  have px : 2 ^ (n * x.length) ≤ 2 ^ (n * max x.length y.length) :=
    Nat.pow_le_pow_right (by decide) (Nat.mul_le_mul_left _ (by omega))
  have py : 2 ^ (n * y.length) ≤ 2 ^ (n * max x.length y.length) :=
    Nat.pow_le_pow_right (by decide) (Nat.mul_le_mul_left _ (by omega))
  have hneg := negPass_clock hn h2 _ hb
  rw [h1] at hneg
  have hcl : val n (absDiff n x y).1 = absDiffClock (2 ^ (n * max x.length y.length)) (val n x) (val n y) := by
    simp only [absDiff]; rw [hneg, h3, h4]; unfold absDiffClock
    cases borrow (val n x) (val n y) <;> simp
  refine ⟨h1, by simp [absDiff, negPass_length, h1], negPass_words _ _ _ _, hcl, ?_, ?_, hb⟩
  · rw [hcl, borrow_lemma (by omega) (by omega)]
  · simp only [absDiff]; rw [h4]; unfold borrow
    by_cases h : val n x < val n y <;> simp [h]

end Words
end Header
