# Spec 14 toy studies

Lane width K: one word holds a + bi as two signed K-bit halves, real part in the low half. Instruction counts follow book/logical-arithmetic.md: each word operation (AND, OR, XOR, NOT, add, subtract, shift, rotate, multiply, compare-to-value) is 1 instruction, a constant in the program is free, a mask from a flag is 1 (0 − k). Counts are for plain scalar word operations; "andn" means x86-64 `andn` / ARM64 `bic`.

## Study 1: infinity marker (inf.py)

**What was checked.** MIN = −2^(K−1) is never a value; a word is the point at infinity when either half is MIN. Operations add, sub, neg, mul, conj, multiply by i. For each pair of words: if an input is infinity the result must be infinity; if both inputs are finite and the true result's two halves lie in the symmetric range, the result must be exact. Results outside the range are counted, not corrected. Negation, conjugation and multiply by i were checked on every finite word to give a result in range.

**Cases.** K = 4: all 65,536 word pairs. K = 5: all 1,048,576 pairs. K = 32: 200,000 random pairs (5% with a real half MIN, 5% with an imaginary half MIN, 10% built from ±(2^31 − 1), 0, ±1, the rest small or fully random). **Failures: none** for every operation at every K.

**Undefined points of the Riemann sphere.** ∞ + ∞, ∞ − ∞ and 0 · ∞ are undefined (with one unsigned infinity, ∞ + ∞ is undefined too). The type has no "undefined" value, so the toy returns ∞ for all three (961 such add pairs and 62 mul pairs at K = 4). Defined cases: z ± ∞ = ∞ for finite z, z · ∞ = ∞ for z ≠ 0, ∞ · ∞ = ∞, −∞ = conj(∞) = i · ∞ = ∞.

**Overflow (not handled).** A finite result outside the symmetric range wraps each half mod 2^K. If a wrapped half lands exactly on MIN, the result reads as infinity (spurious); otherwise it is a wrong finite value. K = 4 add: 22,064 overflowing pairs, 6,104 read as infinity, 15,960 wrong finite. K = 4 mul: 46,768 overflowing, 7,168 read as infinity. K = 32 (random): add 587 of 34,313 read as infinity, mul 432 of 141,921. Overflow is silent in this study.

**Negation is always valid for finite values**: −a is in range for every a in the symmetric range (225 finite words at K = 4, 961 at K = 5, 180,020 at K = 32, none out of range).

**Unary operations need no check.** −MIN = MIN mod 2^K, so negating a half, swapping halves or keeping a half leaves MIN as MIN: neg, conj and multiply by i turn infinity into infinity at zero extra cost. The result is not always the canonical word pack(MIN, MIN) (neg(pack(MIN, 3)) = pack(MIN, −3)), so comparing two words for equality needs the infinity test first, or a canonicalising step.

**The check, branch-free, per half.** Both halves are tested at once in the packed word:

- y = (x AND NH) + NH, where NH is every bit except each half's top bit. Each half of y has its top bit set exactly when that half's low K − 1 bits are not all zero; the per-half sum is below 2^K, so nothing carries into the next half.
- marks = x AND NOT y AND H. This keeps the top bit of each half that equals MIN.

Cost: 5 instructions for both halves (4 with andn), so 2.5 per half. For a binary operation, OR the two inputs' marks (1). Then force each marked half of the result to MIN: m = t − (t shr (K − 1)) (2) is the low K − 1 bits of each marked half, and (r AND NOT m) OR t (3, or 2 with andn) sets it to MIN. The whole operation stays per half, with no compare and no move across halves.

| operation | plain | infinity overhead | total |
|---|---|---|---|
| add, sub (packed, HD 2-18) | 5 | 16 (13 with andn) | 21 (18) |
| neg (packed 0 − x) | 5 (4 with andn) | 0 | 5 (4) |
| conj | 7 | 0 | 7 |
| multiply by i (rotate, negate low half) | 7 | 0 | 7 |
| mul (extract 6, 4 multiplies, 2 add/sub, pack 3) | 15 | 11 (4 compares to MIN on the extracted halves, 3 OR, mask 1, conditional choice 3) | 26 |

The multiply works on extracted halves, so its check uses four compares to the value MIN instead of the packed test. In vector registers a per-lane compare-equal against MIN does both halves in one instruction on all three chip families; that is not counted here (plain word operations only).

## Study 2: overflow marker (ovf.py)

**What was checked.** Wrapping add, sub and mul, where a result half outside the symmetric range turns the result into infinity by a mask (no branch), and an infinite input gives infinity. For each pair of words the expected result is: infinity if an input is infinity; the true value if both true halves are in range; otherwise infinity. Two ways to mark, both checked. "Force" sets each offending half to MIN and stays inside the halves; the result reads as infinity but is not always the canonical word pack(MIN, MIN). "Canon" replaces the whole word by pack(MIN, MIN), and the check demands exactly that word.

**Cases.** K = 4: all 65,536 word pairs (28,561 in range, 22,064 out of range, 14,911 with an infinite input, for add). K = 5: all 1,048,576 pairs. K = 32: 200,000 random pairs (10% with a MIN half; 20% built from ±(2^31 − 1), ±(2^31 − 2), 0, ±1, ±2^16, 2^16 − 1; 30% small; the rest fully random). **Failures: none** for add, sub (both ways) and mul (canon).

**Add and sub, how the test works.** The packed wrapping add (HD 2-18, 5 instructions) gives r. A half has signed overflow when x and y have the same sign and r has the other: (x XOR r) AND (y XOR r) AND H, 4 instructions; for subtract, (x XOR y) AND (x XOR r) AND H. A half that wraps without signed overflow is out of the symmetric range only when it equals MIN, and then it is already the marker. So "force" needs only overflow OR inf(x) OR inf(y). "Canon" must also catch that case, by testing r's halves too. A first run without it failed 2,366 canon pairs at K = 4, all of this kind.

**Mul, how the test works.** The four products are formed in the 2K-bit word from the extracted signed halves. |ac − bd| ≤ 2(2^(K−1) − 1)^2 < 2^(2K−1), so each half's true value is exact there for finite inputs. A value v is in range when (v + HI) read unsigned is ≤ 2·HI, where HI = 2^(K−1) − 1 (an add and a compare, 2 per half). This also catches v = MIN. Infinite inputs are found by four compares to MIN on the extracted halves.

| operation | plain | overflow + infinity overhead | total |
|---|---|---|---|
| add or sub, force | 5 | overflow 4, inf(x) 5, inf(y) 5, OR 2, force 5 = 21 (18 with andn) | 26 (23) |
| add or sub, canon | 5 | as force plus inf(r) 5 and OR 1, with canon's compare 1, mask 1, choice 3 in place of force = 27 (24 with andn) | 32 (29) |
| mul, canon | 15 | range 2 + 2 + OR 1, four compares to MIN 4, OR 4, mask 1, choice 3 = 17 | 32 |

Compared with study 1, the add pays 5 more instructions for exact overflow marking (21 to 26); the multiply pays 6 (26 to 32). The canonical form costs add and sub 6 more than force; it buys word equality between infinities. In the code, `if mark is canon` picks a variant when the function is called, not a branch on data.
