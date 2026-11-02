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

## Study 3: stack of words (stack.py)

**What was checked.** A number is a stack of L words, lowest first. Word j holds the real part's digit j in its low half and the imaginary part's digit j in its high half. Lower words' halves are unsigned K-bit digits; the top word's halves are signed, so each part is a two's-complement integer of KL bits. Add and sub carry per half into the same half of the next word, by word operations only (no carry flag). They wrap mod 2^(KL) per part and return a per-half signed-overflow word for the top (study 2's test). Mul is the schoolbook product truncated to L words, re = ac − bd and im = ad + bc, wrapping mod 2^(KL) per part. Each result was checked against Python integers: the wrapped value for add, sub and mul, and that add's and sub's overflow word is nonzero exactly when a true part leaves the KL-bit signed range.

**Cases.** All stack pairs at each small size: K = 1, L = 2 (256 pairs); K = 1, L = 3 (4,096); K = 2, L = 2 (65,536); K = 2, L = 3 (16,777,216); K = 3, L = 2 (16,777,216). Random: K = 32, L = 2 and L = 3, 100,000 pairs each (30% of words built from the digits 0, 1, 2^K − 1, 2^K − 2, 2^(K−1), 2^(K−1) − 1, 20% small positive or negative values, the rest random). **Failures: none** (values and overflow flags).

The stack does not yet use the infinity marker; the top word's halves take every K-bit value, including MIN. Study 2's marker could be applied to the top word's halves.

**Add, how the carry works.** Each word is a packed add with a carry-in c that holds 0 or 1 at bit 0 and bit K: r = ((x AND NH) + (y AND NH) + c) XOR ((x XOR y) AND H). The low K − 1 bits of a half plus the carry stay below 2^K, so nothing crosses into the other half. The carry out of a half is the majority of its top bits: ((x AND y) OR ((x OR y) AND NOT r)) AND H, then shift right by K − 1 to place it at bit 0 and bit K. **Sub** is the same add on NOT y with carry-in 1 in each half (the word with a 1 at bit 0 and bit K), so the two share one routine.

**Mul, how it works.** The digits are extracted from the halves (AND, shift right). Each of the four products is the row-by-row schoolbook product truncated to L digits: t = r + a_i c_j + carry stays below 2^(2K), digit = t AND (2^K − 1), carry = t shr K. The truncated product mod 2^(KL) is the same for signed and unsigned operands, so the top digit is used as unsigned. Then re and im are combined digit by digit with carry (t = p ± q + c, digit = t AND (2^K − 1), c = t sar K), and the halves are packed back.

| operation | instructions | L = 1 | L = 2 | L = 3 | per word, L = 3 |
|---|---|---|---|---|---|
| add (word 0: 6 + 7; middle: 7 + 7; top: 7; overflow 4) | 14L − 3 (13L − 2 with andn) | 11 | 25 | 39 | 13 |
| sub (adds L NOTs and a carry-in in word 0) | 15L − 3 (14L − 2 with andn) | 12 | 27 | 42 | 14 |
| mul (extract 4L, 4 truncated products of L(L+1)/2 digit products at 5 each, combine 2(4L − 1), pack 2L) | 10L² + 24L − 2 | 32 | 86 | 160 | 53 |

The add's cost per word is constant, about 14 (one word with an overflow test was 9 in study 2's terms: 5 for the add, 4 for overflow). The multiply's cost per word grows with L, because the digit products grow as L². Of mul's instructions at L = 3, 4·L(L+1)/2 = 24 are multiplies. The L = 1 multiply (32) is dearer than the one-word multiply (15) because it goes through digits and carries; a one-word number would use the one-word routine. Two savings not tried here are worth noting. The 3-multiply complex product, ac, bd and (a + b)(c + d), saves one of the four truncated products for some extra adds. A packed product could form a digit's real and imaginary partial products in one word. Overflow of the multiply is not detected in this study: the result wraps mod 2^(KL) per part, and the exact product of two L-word numbers needs 2L words.

## Study 4: Barrett against Montgomery (reduce.py, reduce_bench.c)

**What was checked.** Each lane holds a residue in [0, n), n < 2^(K−1). The Gaussian product mod n forms re_t = ac + (n² − bd) and im_t = ad + bc, both non-negative and below 2n² < 2^(W−1), W = 2K. Each is reduced mod n by one of two methods. Both use the low word (mul) and the high word (mulhi: x86-64 `mul`, ARM64 `umulh`, RISC-V `mulhu`) of a W × W product.

- **Barrett:** m = ⌊2^W / n⌋ is precomputed. q = mulhi(t, m), r = t − qn, so r lies in [0, 2n). Then subtract n, and add it back where the result is negative (mask by sar).
- **Montgomery (R = 2^W):** n must be odd, and n′ = n^−1 mod 2^W is precomputed. mm = t·n′ (low word), u = mulhi(mm, n). Since t − mm·n ≡ 0 mod 2^W and t < 2^W, (t − mm·n)/2^W = −u, which lies in (−n, 0]. Add n where negative. The result is t·R^−1 mod n, so values are kept in Montgomery form (x·R mod n).

**Cases.**
- Reduction alone, exhaustive at K = 4, 5, 6, 7, 8: every n from 2 to 2^(K−1) − 1, and every t < 2n² (at K = 8, 1,381,758 Barrett cases and 699,006 Montgomery cases, odd n).
- Gaussian product, exhaustive over all lane values: every n at K = 4 and 5 (178,311 Barrett and 103,495 Montgomery tuples at K = 5). At K = 6, 7 and 8, the three largest n ≤ 63 (44,375,138 Barrett tuples at K = 7 and again at K = 8).
- K = 32, random: nine moduli, each with 20,000 random t and 20,000 random Gaussian pairs plus edge cases. The four largest primes below 2^31 are 2,147,483,647, 2,147,483,629, 2,147,483,587 and 2,147,483,579; also the primes 65,537 and 1,000,003, and the composites 2,147,483,646 (even, Barrett only), 2,147,483,645 and 3·5·7·…·23 = 111,546,435.
- Montgomery was checked twice: in Montgomery form (the result must be the product times R), and on plain lanes (the result must be the product times R^−1).

**Failures: none.** A first K = 8 run ran out of memory building the lane tuples; it now uses a generator, with the n ≤ 63 limit above.

**Proxies (instruction counts, not speed).**

| | instructions | multiplies | longest dependent chain |
|---|---|---|---|
| Barrett reduction (MULHI, MUL, SUB, SUB, SAR, AND, ADD) | 7 | 2 | 7 |
| Montgomery reduction (MUL, MULHI, NEG, SAR, AND, ADD) | 6 | 2 | 6 |
| Gaussian product, Barrett (extract 4, 4 MUL, 3 add/sub, 2 reductions, pack 2) | 27 | 8 | 13 |
| Gaussian product, Montgomery | 25 | 8 | 12 |

The chain for the Gaussian product is extract 1, MUL 1, add/sub 2 (n² − bd and ac in parallel, then the add), the reduction, and pack 2. The two reductions run in parallel.

Montgomery's costs outside the multiply: n must be odd (Barrett takes any n), and values must be in Montgomery form. Converting a lane in costs one Montgomery multiply by the constant R² mod n (a MUL plus a reduction, 7); converting out costs one reduction (6). These are paid once per value entering or leaving the type, not per operation. Add and subtract are the same in either form.

**Toy timing (reduce_bench.c): a toy timing on this machine, not the real build's measurement.** Each run used gcc 13.3 -O2, one Intel Xeon core of a 2-core VM, and 2^20 random pairs per kernel. Each (n, kernel) was timed 41 times, with the three kernels interleaved and their order rotated each repetition, for the four primes near 2^31 above. The table gives ns per element as median [q1, q3]. Outputs agree element for element across the three methods (Montgomery converted back, untimed). The generated loops are branch-free (no jump but the loop's own, no cmov) and not vectorised.

| kernel | Barrett | Montgomery | t % n (hardware divide, reference only) |
|---|---|---|---|
| scalar product, 1 reduction | 0.873–0.889 [0.86, 0.92] | 0.867–0.914 [0.85, 0.96] | 2.62–2.63 |
| Gaussian product, 2 reductions | 2.16–2.19 [2.12, 2.24] | 2.31–2.36 [2.21, 2.39] | 5.25–5.28 |

The ranges are the medians over the four n. For the scalar kernel, the Barrett and Montgomery quartile ranges overlap, so no difference is shown beyond the noise. For the Gaussian kernel, Barrett's median is about 7% lower than Montgomery's for all four n, and again in a second full run (Barrett 2.16–2.23, Montgomery 2.31–2.42). The quartile ranges touch or barely separate, so the difference is small but repeatable on this machine. It runs against the instruction count (Montgomery has two fewer instructions per product); the cause was not investigated. Both are about 2.4 times faster than the hardware divide here. Max values up to 18 ns are single outliers from the VM.
