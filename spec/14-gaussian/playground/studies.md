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

## Study 5: the mod p split (split.py)

**What was checked.** For a prime p ≡ 1 (mod 4), s = c^((p−1)/4) for a non-residue c has s² ≡ −1. The map φ(a + bi) = (a + bs, a − bs) mod p takes Z[i]/p to F_p × F_p, with inverse a = (u + v)/2 and b = (u − v)/(2s). Both forms keep one residue pair per word. Reductions use study 4's Barrett.

The map was checked to be a ring isomorphism: it is bijective (p² distinct images), it inverts (ψ(φ(z)) = z), φ(x + y) = φ(x) + φ(y), φ(xy) = φ(x)φ(y) with the split product componentwise, and φ(1) = (1, 1). Also checked: the direct Gaussian product mod p, that inverting the split product gives the direct product, that conjugation becomes a swap of the components, and that the norm a² + b² is u·v.

**Cases.** Exhaustive for p = 5, 13, 17, 29, 37: all p² elements and all p⁴ pairs (1,874,161 pairs at p = 37). Random for the four largest primes ≡ 1 (mod 4) below 2^31 (2,147,483,629, 2,147,483,549, 2,147,483,497, 2,147,483,489): 20,000 elements plus 16 edge pairs from {0, 1, p − 1, ⌊p/2⌋}, and 20,000 pairs, each. In all, 2,774,149 pairs and 82,757 elements. **Failures: none.**

**Costs** (word operations; Barrett 7 with 2 multiplies; a + b mod p 5; a − b mod p 4; halving mod p, adding p when odd and shifting, 5):

| operation | instructions | multiplies | longest chain |
|---|---|---|---|
| to split form (extract 2, b·s: MUL + reduce 8, a + bs 5, a − bs 4, pack 2) | 21 | 3 | 15 |
| from split form (extract 2, (u + v)/2: 5 + 5, (u − v)·(2s)^−1: 4 + 8, pack 2) | 26 | 3 | 15 |
| product, split (extract 4, 2 MUL, 2 reductions, pack 2) | 22 | 6 | 11 |
| product, direct (study 4) | 27 | 8 | 13 |
| add, either form (lane for lane the same) | 16 | 0 | 8 |
| conjugate, split (swap: one rotate) | 1 | 0 | 1 |
| norm, split (u·v: extract 2, MUL, reduce 7) | 10 | 3 | 9 |

The chain for the split product is extract 1, MUL 1, reduction 7, pack 2.

The split product saves 5 instructions, 2 multiplies and 2 chain steps per product. Converting a value in and out costs 47 instructions and 6 multiplies, so the split pays only when each converted value takes part in about ten or more products. For a value kept in split form for a whole computation, conversion happens only at the boundary. Multiply by i is dearer in split form: i ↦ (s, −s), two products by constants, where the direct form needs only a swap and one negation.

**p ≡ 3 (mod 4).** −1 is not a square mod p, so x² + 1 is irreducible, Z[i]/p is the field with p² elements, and there is no split. The product stays at 4 products (or 3 by the ac, bd, (a + b)(c + d) arrangement).

**p = 2.** 2 = −i(1 + i)², so 2 ramifies, and Z[i]/2 = F_2[i] with (1 + i)² = 0. It is not a product of fields (1 + i is nilpotent), so there is no split. Its four elements are two bits, and arithmetic on them is trivial bit logic.

**Composite n.** For a composite n whose prime factors are all ≡ 1 (mod 4), the split holds factor by factor through the Chinese remainder theorem (s² ≡ −1 mod n exists then); this was not checked.

## Study 6: GCD and division (gcd.py)

**What was checked.** Halves are in the symmetric range. The arithmetic runs on extracted halves in 2K-bit signed words, as at K = 32 with 64-bit registers. a/b = a·conj(b)/N(b) with x = a₁b₁ + a₂b₂, y = a₂b₁ − a₁b₂ and N = b₁² + b₂², all below 2^(2K−1) in size. Division uses the machine's signed truncating divide of 2K-bit words (x86-64 `idiv`, ARM64 `sdiv`, RISC-V `div`), and the rounding is fixed by masks.

- **Exact division** (b divides a): q = (x/N, y/N).
- **Division with remainder:** each part of q is rounded to the nearest integer, halves up, and r = a − qb. The rounding is the truncated quotient, then a floor correction by the remainder's sign mask, then +1 where r ≥ N − r (2r ≥ N without forming 2r, which could overflow at K = 32). Then N(r) ≤ N(b)/2 < N(b).
- **Gaussian gcd:** Euclid, (a, b) ← (b, a mod b) until b = 0, normalised to the associate with real part > 0 and imaginary part ≥ 0 (gcd(0, 0) = 0).
- **Division by 0:** x86-64 traps on a zero divisor, so N is replaced by 1 when it is 0 (compare, OR), and the quotient becomes the infinity marker by conditional choice. a/0 = ∞ for a ≠ 0, and 0/0 (undefined) also gives ∞.

The reference is separate code using Python rationals and big integers: rounded division, and an extended Euclid that gives g = ua + vb. Each gcd was checked to equal the reference after normalisation and to carry a certificate: g divides a, g divides b, and g = ua + vb, so every common divisor divides g. At K ≤ 4 it was also checked against brute force: N(g) equals the largest norm of any common divisor, found by searching all candidates with |parts| ≤ 2·HI. Division was checked for: q and r matching the reference, a = qb + r, N(r) ≤ N(b)/2, and q and r in range. Exact division was checked as (qb)/b = q for every in-range product.

**Cases.**
- Exhaustive over all pairs of in-range values: K = 3 (2,401 pairs), K = 4 (50,625 pairs, with brute force), K = 5 (923,521 pairs).
- K = 32: 100,006 pairs for division and gcd. A quarter are fully random, a quarter have a divisor up to 2^12, and half are built with a common factor of size 2^4 to 2^15, plus edge pairs such as (HI + HI·i, HI + HI·i) and (0, 0). Also 100,000 exact divisions with the divisor's parts up to 2^30 and quotients sized to keep products in range, and 1,007 divisions by zero.
- **Failures: none.**

**Euclid steps** (one division with remainder each):

| K | max | mean | bound ⌊log₂ N(b)⌋ + 1 |
|---|---|---|---|
| 3 | 3 | 1.88 | 5 |
| 4 | 4 | 2.58 | 7 |
| 5 | 6 | 3.24 | 9 |
| 32 (random) | 27 | 14.07 | 63 |

N(r) ≤ N(b)/2 halves the norm at every step, which gives the bound.

**Instruction counts** (scalar, on extracted halves; a divide is 1 instruction, and is far dearer than a multiply on all three families):

| operation | instructions | multiplies | divides |
|---|---|---|---|
| exact division (extract 6, N 3, zero guard 2, x and y 6, 2 DIV, pack 3, infinity choice 4) | 26 | 6 | 2 |
| division with remainder (extract 6, N 3, x and y 6, rounding 2 × 10, r = a − qb 8, pack q and r 6) | 49 | 12 | 2 |
| rounding of one part (DIV, MUL, SUB, SAR, ADD, AND, ADD, SUB, compare, ADD) | 10 | 1 | 1 |
| normalise (extract 3, sign of (y, x) lexicographically: SHL, ADD, SAR 3, conditional negation 4, compare 1, mask 1, multiply by −i by choice 7, pack 3) | 22 | 0 | 0 |
| gcd | steps × (49 + 1 loop test) + 22 | 12 per step | 2 per step |

The divide's remainder costs a MUL and a SUB (one `msub` on ARM64; x86-64 `idiv` returns it for free). The division-with-remainder count does not include the zero guard, because Euclid never divides by 0.

**The gcd loop is data-dependent.** Its step count varies, and the loop test is a branch. A branch-free version would run the bound's 63 steps at K = 32, with masks holding (a, b) once b = 0 (zero guard 2, conditional choice of two words 6, about 57 per step). That is about 3,600 instructions, against about 14 × 50 + 22 ≈ 720 for the observed mean. This was not implemented. Infinity inputs to the gcd are outside this study.

**Binary-gcd analogue (literature; details unverified).** A. Weilert, "(1+i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Computation 30 (2000). Its title was found by search; the journal details are from memory and were not checked. It plays (1 + i), the prime above 2, the part that 2 plays in the binary gcd. a + bi is divisible by 1 + i exactly when a ≡ b (mod 2), and (a + bi)/(1 + i) = ((a + b) + (b − a)i)/2, an add, a subtract and two shifts, with no divide. I. Damgård and G. S. Frandsen, "Efficient algorithms for gcd and cubic residuosity in the ring of Eisenstein integers", BRICS RS-03-8 (2003). Its abstract was read; it generalises the binary gcd to the Eisenstein integers in O(n²) time and states that the technique gives equally fast algorithms for gcd and quartic residuosity in Z[i]. S. Agarwal and G. S. Frandsen, "Binary GCD like algorithms for some complex quadratic rings", ANTS 2004, is from memory and was not checked. None of these was implemented or tested here; a (1+i)-ary gcd would replace the two divides per step by shifts and masks, at the cost of more steps.

## Study 7: deferred check, one shared path for overflow and infinity (deferred.py)

**What was checked.** Each operation (add, sub, mul) does its plain arithmetic and ORs one per-half test of its result into a running sticky word. Inputs are not tested per operation. A run's external inputs are tested once (study 1's detector). Every later operand is the result of an earlier operation, which was tested. Once a sticky word is nonzero its element is infinity, so its value no longer matters (it may be garbage). At the end of the run, one check turns every flagged element into the canonical infinity pack(MIN, MIN).

The per-operation test must catch exactly "a result half outside the symmetric range", given operands in range:

- **add and sub:** signed overflow of the half (4) OR the result half equals MIN (study 1's detector, 5). The second test is needed because a true value of exactly −2^(K−1) wraps to MIN with no signed overflow. It cannot be left to the end: pack(−HI, 0) + pack(−1, 0) gives pack(MIN, 0) with no signed overflow, and adding pack(1, 0) next gives pack(−HI, 0), an ordinary finite value that no later test would catch.
- **mul:** study 2's range test on the exact 2K-bit parts, which also catches MIN.

Two variants were checked against study 2's semantics (an operation gives infinity when an input is infinity or a true result half leaves the symmetric range):

- **V1, per-element sticky:** a sticky word per element, held in a register through the element's run. Each external input is tested once, with one canonical choice per element at the end.
- **V2, one sticky word for the whole array:** every element's tests are ORed into one word. Once per array, a zero word means the array is exact. A nonzero word sends the whole array to recomputation with study 2's per-operation marking: one branch per array, and it needs the inputs kept.

A run is acc = x₀ followed by acc = op(acc, xⱼ) or op(xⱼ, acc), with the side chosen per step. Study 2's chain (`run_study2`, per-operation marking) was also checked against the true-value reference on every run.

**Cases.**
- K = 3, exhaustive: every word and every choice of operation and side, for runs of 1 operation (24,576) and 2 operations (9,437,184), plus 20,000 random runs of 3 to 8 operations. 9,481,760 runs in all.
- K = 4: exhaustive for runs of 1 operation (393,216), plus 20,000 random runs of 3 to 8.
- K = 32: 100,000 random runs of 1 to 16 operations. 4% of inputs have a MIN half, 16% are built from edge values, 50% are small, the rest random.
- V2: 2,000 arrays of 64 elements at K = 3 and 4, and 400 arrays of 256 at K = 32. Half are filled with small values and half with random words. At K = 32, 200 arrays stayed exact on the fast path and 200 were recomputed. At small K almost every array overflows (7 fast at K = 4), so the fast path is exercised mainly at K = 32. Each fast-path array was checked to contain no infinity.
- **Failures: none** (V1, V2, and study 2's chain).

**Instruction counts.**

| | deferred, per operation | study 2, per operation |
|---|---|---|
| add or sub (plain 5, overflow 4, MIN result 5, OR 1, sticky OR 1) | 16 (15 with andn) | 26 force, 32 canon |
| mul (plain 15, range test 5, sticky OR 1) | 21 | 32 |

| once | V1 | V2 |
|---|---|---|
| per external input element | 6 (detector 5 + OR 1; 5 with andn) | 6 |
| per element at the end of the run | 5 (compare, mask, choice) | 0 |
| per array | 0 | 2 (compare and branch), plus a full recomputation at study 2's cost when any element is flagged |

The deferred test saves 10 instructions per add or sub and 11 per mul against study 2's canonical form. It moves the input test from every operation to once per external input. The once-per-element costs (6 per input, 5 at the end) are paid once, however long the run. A run of R operations with R + 1 fresh inputs costs 16R + 6(R + 1) + 5 for adds. That is 33 against 32 at R = 1 (no gain) and 99 against 128 at R = 4. The saving grows when inputs are reused across many operations, as in a product of matrices, where an input element is tested once and used many times.

Structure, per RULES' "one structure each": in V1 the sticky word is local to an element's run when the run is fused into one loop. Across separate passes over an array, V1 would need a stored sticky array, a second structure. V2 keeps one word, but its slow path needs the inputs kept for the recomputation.

## Study 8: Gauss's three-multiply product (gauss3.py)

**What was checked.** k₁ = c(a + b), k₂ = a(d − c), k₃ = b(c + d); re = k₁ − k₃ = ac − bd, im = k₁ + k₂ = ad + bc. It was compared with the four-multiply product, in two forms. The arithmetic runs on extracted halves in 2K-bit words (64-bit registers at K = 32), wrapping like the machine.

- **Wrapping** (study 1's multiply): halves are int-K, MIN included, and result halves wrap mod 2^K. Checked: the three-multiply result equals the four-multiply result and the true value mod 2^K. For halves in the symmetric range, the 2K-bit re and im are also the exact true values, so study 2's range test applies unchanged.
- **Mod n**, with Barrett as in study 4: every intermediate is kept non-negative. a + b < 2n, (d + n) − c lies in (0, 2n), and c + d < 2n, so k₁, k₂, k₃ < 2n². Then re_t = (k₁ + 2n²) − k₃ lies in (0, 4n²), and im_t = k₁ + k₂ = ad + bc + an ≡ ad + bc (mod n), < 4n². Since n < 2^(K−1), 4n² < 2^(2K), so one Barrett reduction still leaves r < 2n.

**Cases.**
- Wrapping, exhaustive over all word pairs: K = 3 (4,096), K = 4 (65,536), K = 5 (1,048,576).
- Mod n, exhaustive over all residue quadruples for every n from 2 to 2^(K−1) − 1: up to n = 15 at K = 5 (178,311 quadruples).
- K = 32: 200,000 random wrapping pairs (a quarter built from 0, ±1, ±HI, MIN, HI − 1, ±2^16). Mod n: 25,000 random quadruples (a fifth from the edges 0, 1, n − 1, n − 2, ⌊n/2⌋) for each of eight moduli: 2,147,483,647, 2,147,483,629, 2,147,483,587, 2,147,483,579, 2,147,483,646 (even), 2,147,483,645, 65,537 and 3.
- **Failures: none.** One input at each small K and 9 at K = 32 gave a 2K-bit re or im that was not the true value. All of them have a MIN half, which is infinity. For example (MIN, MIN)·(MIN, MIN) has im = 2·MIN² = 2^(2K−1), one past the 2K-bit signed range; the four-multiply product overflows the same way. Their halves mod 2^K are still right.

**Proxies (not speed).**

| product | instructions | multiplies | longest chain |
|---|---|---|---|
| wrapping, four multiplies (extract 6, 4 MUL, 2 add/sub, pack 3) | 15 | 4 | 6 |
| wrapping, three multiplies (extract 6, 3 add/sub, 3 MUL, 2 add/sub, pack 3) | 17 | 3 | 7 |
| mod n, four multiplies (study 4) | 27 | 8 | 13 |
| mod n, three multiplies (extract 4, a + b 1, (d + n) − c 2, c + d 1, 3 MUL, re 2, im 1, 2 Barrett 14, pack 2) | 30 | 7 | 14 |
| wrapping, y fixed (y's sums precomputed), four / three | 12 / 12 | 4 / 3 | |
| mod n, y fixed, four / three | 25 / 25 | 8 / 7 | |

On a product of two fresh values, Gauss trades one multiply for three more adds and subtracts when wrapping (two more instructions in all) and four more mod n (three more in all), and the chain gets one step longer (the add before the multiply). When one operand is fixed across many products (a constant, or a matrix entry reused), its sums c, d − c, c + d (or c, (d + n) − c, c + d) are computed once. The trade is then one multiply for one add, at equal instruction counts. Both chains contain exactly one multiply, so the chain comparison does not depend on how dear a multiply is. The three-multiply form gains only where a multiply costs more than an add in throughput, and these proxies cannot show that.

**Intermediate widening.** a + b, c + d and d − c need one more bit than a half: K + 1 bits signed, so 33 bits at K = 32. With extracted halves in 64-bit registers this costs nothing, and the products c(a + b) etc. stay below 2^(2K−1) for halves in the symmetric range. In vector lanes it matters. A 32 × 32 → 64 widening multiply (x86-64 `pmuldq`, ARM64 `smull`, RISC-V `vwmul`) takes 32-bit inputs, so a 33-bit a + b does not fit. The choices are a 64-bit lane multiply, which ARM64's base vector unit lacks as far as I know (unverified), or giving up the exact value. Wrapping alone does not need the extra bit, since a + b mod 2^K gives the right low K bits. But then the exact 2K-bit value that study 2's range test needs is lost. Mod n has no such problem: n < 2^(K−1) leaves a spare bit, so a + b < 2n < 2^K fits an unsigned K-bit lane, and the unsigned widening multiply (`pmuludq`, `umull`, `vwmulu`) applies.
