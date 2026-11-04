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

## Study 9: (1+i)-ary binary gcd (gcd1i.py)

**The algorithm** is a variant in the spirit of Weilert (2000). It was written from the facts below and not checked against his paper, which was not read.

π = 1 + i has norm 2, and π² = 2i.
- π divides x + yi exactly when x ≡ y (mod 2), and (x + yi)/π = ((x + y) + (y − x)i)/2: an add, a subtract and two shifts.
- The π-valuation of z ≠ 0 is v = 2e, plus 1 when x shr e and y shr e are both odd, where e = ctz(x OR y).
- Up to a unit, z/π^v is (x sar e, y sar e), followed by one division by π when both parts are odd. That is "strip".
- Every element prime to π has exactly one primary associate, ≡ 1 mod π³: x odd, y even, x − y ≡ 1 (mod 4). It is reached by a conditional multiply by −i, then a conditional negation. That is "primary".
- For primary a and b, a − b ≡ 0 (mod π³). So with N(a) ≥ N(b), N(strip(a − b)) ≤ (|a| + |b|)²/8 ≤ N(a)/2.

Steps: k = min(v(a), v(b)); strip and make primary both a and b; order them so N(a) ≥ N(b). Each step sets a ← primary(strip(a − b)) and swaps if N(a) < N(b), stopping when b = 0. The gcd is a·π^k up to a unit, where π^k = 2^⌊k/2⌋ × a unit, × (1 + i) when k is odd. It is then normalised as in study 6. Once b = 0 the step leaves (a, b) unchanged (a − 0 = a, which is already stripped and primary, and no swap). So the branch-free version just runs the step a fixed number of times, with no extra masks.

Values are two extracted halves in 2K-bit registers, since a − b can leave a half's range. The run asserts that every value stays below 2^K in size.

**Cases.** Exhaustive over all pairs of in-range values: K = 3 (2,401), K = 4 (50,625), K = 5 (923,521). K = 32: 100,008 random pairs, with the same mix as study 6 (random, a small divisor, or a built-in common factor of 2^4 to 2^15), plus edge pairs including (0, 0), (HI + HI·i, 0) and (2^30 + 2^30·i, 2^29). Each result was compared with study 6's Euclid (its rational-arithmetic reference, which study 6's word code matched exactly). **Failures: none.**

**Steps.**

| K | (1+i)-ary max | (1+i)-ary mean | Euclid max | Euclid mean |
|---|---|---|---|---|
| 3 | 4 | 2.11 | 3 | 1.88 |
| 4 | 6 | 3.17 | 4 | 2.58 |
| 5 | 8 | 4.20 | 6 | 3.24 |
| 32 (random) | 44 | 23.58 | 28 | 14.05 |

The proven bound: each step at least halves N(a)·N(b), which starts below 2^(4K−2) after stripping. So there are at most 4K − 2 steps, 126 at K = 32, against Euclid's 63. The observed worst is far below either bound.

**Instruction counts** (scalar, on extracted halves):

| part | (1+i)-ary | Euclid (study 6) |
|---|---|---|
| step (a − b 2, strip 16, primary 17, norm 3 with 2 MUL, compare and mask 2, conditional swap of a, b and N(a), N(b) 12) | 52, + 2 loop test = 54 | 49, + 1 loop test = 50 |
| multiplies / divides per step | 2 / 0 | 12 / 2 |
| once per gcd: valuations 2 × 7, min 3, strip and primary 2 × 33, norms 6, order 14 | about 103 | 0 |
| once per gcd at the end: π^k (shift 3, conditional (1 + i) 10), normalise 22 | about 35 | 22 |
| zero inputs | prologue case (once per gcd; masks not counted) | none (b = 0 ends the loop) |

| per gcd, K = 32 | (1+i)-ary | Euclid |
|---|---|---|
| with a loop branch, at the observed mean | 138 + 54 × 23.58 ≈ 1,410 instructions, ≈ 53 multiplies, 0 divides | 22 + 50 × 14.05 ≈ 725 instructions, ≈ 169 multiplies, ≈ 28 divides |
| branch-free, at the proven bound | 138 + 52 × 126 ≈ 6,690 | about 3,600 (63 × 57) |

By instruction count the (1+i)-ary gcd costs about twice Euclid's: 1.7 times the steps, at about the same instructions per step. It removes every divide and most multiplies (2 per step for the norm comparison, against 12 multiplies and 2 divides). The proxies weigh a divide like an add, and on all three families a 64-bit divide takes far longer than an add, so these proxies cannot settle which is faster. Its branch-free form is simpler (the step is its own fixed point once b = 0), but its proven bound is twice Euclid's.

**Portability.** ctz is x86-64 `tzcnt` (BMI1), ARM64 `rbit` + `clz` (2 instructions, counted here as 1), and RISC-V `ctz` (the Zbb extension, part of the RVA22 profile; base RV64I lacks it). Without ctz, strip would take one masked division by π per factor.

## Study 10: Weilert's published (1+i)-ary gcd (not done: paper not found)

**Outcome.** Weilert's text could not be retrieved, so no algorithm was implemented (no gcd_weilert.py) and nothing was guessed.

- **The paper.** A. Weilert, "(1 + i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Computation 30(5), 605–617 (2000), DOI 10.1006/jsco.2000.0422, ScienceDirect PII S074771710090422X. This citation is now confirmed from the reference lists of Agarwal and Frandsen (ANTS 2004) and Wikström (ICALP 2005, KTH report). Every route to the text was refused or blocked: ScienceDirect, the DOI resolver, Springer PDFs, and direct download through this session's proxy.
- **What the paper claims** (from its abstract, as indexed): it extracts powers of (1 + i) like the binary gcd. It runs in O(n²) bit operations for n-bit inputs, which it says is faster than the Euclidean algorithm in Z[i] and the Caviness–Collins algorithm, and a constant factor faster than a Lehmer-type gcd with the same O(n²). The advantage claimed is bit complexity for large numbers, not cost within one machine word.
- **The other paper.** Weilert's ANTS 2000 paper (LNCS 1838, 595–613) is a different one, "Asymptotically fast GCD computation in Z[i]" (a fast, Schönhage-type method), not the (1+i)-ary algorithm.

**Damgård and Frandsen, BRICS RS-03-8 (2003): read.** It gives an explicit algorithm, but for the Eisenstein integers Z[ζ], where 1 − ζ plays the part of 1 + i:

1. Write α = unit · (1 − ζ)^j₁ · γ and β = unit · (1 − ζ)^j₂ · δ with γ, δ primary.
2. g ← (1 − ζ)^min(j₁, j₂).
3. Replace α, β by γ, δ.
4. While α ≠ β, let γ be the primary part of α − β, and replace the approximately larger of α, β by γ.
5. g ← g · α.

The paper does not mention Weilert. For Z[i] it notes only the ingredients: primary = 1 + (2 + 2i)β, N((1 + i)³) = 8, and N(α − β) ≤ 4 max(N(α), N(β)). It claims O(n²) bit operations against O(n·M(n)) for the Euclid-based methods.

**Two differences from study 9** appear in its Eisenstein algorithm, as reported, not tested in Z[i]:

- **Loop end.** The loop runs until α = β, not until one element is 0. When both are primary and equal, the difference is 0, so this saves the last step.
- **Approximate norm.** It replaces the element with the larger approximate norm, which is cheaper than the exact norm. In the Eisenstein case the approximation can be off by a factor up to 9/8. Study 9 compares exact norms, 2 multiplies per step.

**Agarwal and Frandsen, "Binary GCD like algorithms for some complex quadratic rings"** (ANTS 2004, Lecture Notes in Computer Science, 57–71): its metadata and reference list were read, but the text was not reachable.

## Study 11: Damgård and Frandsen's ideas applied to study 9's gcd (gcd1i_v2.py)

**What was checked.** Study 9's (1+i)-ary gcd, with each idea from Damgård and Frandsen's Eisenstein algorithm (BRICS RS-03-8) as a separate variant. Strip, primary, the gcd's power of (1 + i) and normalise are study 9's, unchanged.

- **(a) Stop when equal.** Stop when the two numbers are equal, not when one becomes 0. Both are primary, so equal means a − b = 0. Study 9's last step is always the one that turns a = b into 0, so (a) saves exactly one step per gcd.
- **(b) Their approximate norm.** Each coordinate's absolute value keeps only its 6 most significant bits, and the results are squared and summed (their Lemma 2). For Z[ζ] they show (8/9)N ≤ Ñ ≤ N; for Z[i] the same truncation gives (31/32)² N ≤ Ñ ≤ N. They use it because on long numbers it takes linear time where the exact norm needs a multiplication.
- **(c)** (a) and (b) together.
- **(d) |x| + |y| (not from the paper).** A word-friendly size, with no multiply. N ≤ (|x| + |y|)² ≤ 2N, so replacing a when it is the larger by this measure gives N(b) ≤ 2N(a). Then N(new a) ≤ (|a| + |b|)²/8 ≤ (1 + √2)²/8 · N(a) < 0.73 N(a), and the product of the norms still falls by a factor above 1.37 per step.
- **(e)** (d) and (a) together.

The rest of their algorithm is already in study 9: the primary associate (≡ 1 mod 2 + 2i = −i(1 + i)³), the difference of two primaries being divisible by (1 + i)³, replacing the larger of the two, and the gcd = (1 + i)^min · (the final value). Their unit-exponent bookkeeping, (−ζ)^i, is needed for residue symbols, not for the gcd. Their linear-time division by the prime matters for long numbers; within one word, study 9's ctz and shifts already do it.

**Cases.** Exhaustive over all pairs of in-range values: K = 3 (2,401), K = 4 (50,625), K = 5 (923,521). K = 32: the same 100,008 pairs as study 9. Each variant was compared with study 6's Euclid. **Failures: none**, in every variant.

**Steps (mean / max) and cost per step.** The instructions per step are 49 for the core (a − b 2, strip 16, primary 17, compare and mask 2, conditional swap 12), plus the size, plus the loop test (2 to test for zero, 4 to test for equality).

| variant | K = 4 | K = 5 | K = 32 | per step | MUL / DIV per step | ≈ instructions in steps, K = 32 |
|---|---|---|---|---|---|---|
| Euclid (study 6) | 2.58 / 4 | 3.24 / 6 | 14.05 / 28 | 50 | 12 / 2 | 703 |
| study 9 (exact norm, stop at 0) | 3.17 / 6 | 4.20 / 8 | 23.58 / 44 | 54 | 2 / 0 | 1,274 |
| (a) stop when equal | 2.18 / 5 | 3.20 / 7 | 22.58 / 43 | 56 | 2 / 0 | 1,265 |
| (b) their approximate norm (size 23: per coordinate abs 3, clz 1, sub 1, max 3, shift 1, and 1; then 2 MUL, ADD) | 3.17 / 6 | 4.20 / 8 | 23.59 / 42 | 74 | 2 / 0 | 1,745 |
| (c) (a) + (b) | 2.18 / 5 | 3.20 / 7 | 22.59 / 41 | 76 | 2 / 0 | 1,717 |
| (d) \|x\| + \|y\| (size 7) | 3.17 / 6 | 4.21 / 9 | 23.65 / 43 | 58 | 0 / 0 | 1,372 |
| (e) (d) + (a) | 2.18 / 5 | 3.21 / 8 | 22.65 / 42 | 60 | 0 / 0 | 1,359 |

The once-per-gcd costs (about 103 at the start and 35 at the end for the (1+i)-ary gcd, 22 for Euclid) come on top of the in-loop totals.

**What each idea does in one word.**
- **(a)** saves one step (54 instructions) but costs 2 more per loop test, so at K = 32 it comes out about even (1,265 against 1,274). Folding the test into the step removes that cost. Compute d = a − b first and leave the loop when d = 0 (OR, compare: 2, the same as study 9's test). Then (a) costs 54 per step plus one final subtraction and test (4), about 1,223 at K = 32. Not coded separately; the steps are (a)'s.
- **(b)** does not pay inside one word. The exact norm is 2 multiplies and an add (3), while the approximation needs an absolute value, a leading-zero count and a mask per coordinate before the same 2 multiplies (23). It adds 20 instructions per step and changes the step count by almost nothing (23.59 against 23.58). Its advantage is for long numbers, where the exact norm needs a multiplication of long numbers and the approximation does not.
- **(d)** is the version suited to one word: no multiply at all in the step, at 4 more instructions than the exact norm (7 against 3). The step count rises by 0.07 on average at K = 32.

None of the variants changes the picture from study 9. In instructions the (1+i)-ary gcd stays at roughly 1.75 to 2 times Euclid at K = 32 (about 1,220 to 1,275 in steps for the best variants, against about 703). It needs at most 2 multiplies and no divide per step, against Euclid's 12 multiplies and 2 divides. The proxies count a divide like an add, so they cannot settle which is faster.

## Study 12: a family of word sizes, growing and shrinking (family.py)

**What was checked.** A word of width 2h holds two h-bit halves, h on the ladder 4, 8, 16, 32, 64, 128, and an array has one width for all its elements. A half fits in h bits when it lies in the symmetric range |v| ≤ 2^(h−1) − 1, so −2^(h−1) does not fit.

- **Promotion.** Each operation runs at the array's width (the wider of the two operands; the narrower is first copied up). It ORs each element's "doesn't fit" test into one accumulator (study 7's deferred check). If the accumulator is set at the end, both arrays are copied to width 2h and the operation is redone.
- **Shrinking.** After each operation, the array halves its width for as long as every element passes the shrink rule.
  - **Rule A:** fits in h/2 bits.
  - **Rule B:** one bit of margin, fits in h/2 − 1 bits.
- **Fresh operand arrays** get the smallest width that passes the same rule.
- **Detection:**
  - add and sub either by study 7's sign test on the packed word (signed overflow, or a half equal to −2^(h−1)), or at double width followed by the fit test;
  - mul at double width followed by the fit test (ac − bd is checked to fit in 2h bits);
  - neg, conj, exact division, division with remainder (study 6) and mod n (Barrett, study 4) are checked never to leave the range.

**The fit tests.**
- **On an extracted half** v (a 2h-bit value): v fits when (v + 2^(h−1) − 1), read unsigned, is ≤ 2·(2^(h−1) − 1). That is ADD and one unsigned compare, the same test as study 2.
- **The packed shrink test** on a word of width 2h, asking whether both halves fit in g bits (g < h):
  - u = v + (2^(g−1) − 1) and t = v + 2^(g−1) are formed in each half by one packed add each, (w AND NH) + constant, then XOR with (w AND H). The constants are below 2^(h−1), so nothing carries between halves.
  - The half fits exactly when neither u nor t has a bit at or above g: (u OR t) AND U = 0, where U holds bits g to h − 1 of each half.
  - u catches the values below the range (including −2^(g−1)); t catches those above. The cost is 8 instructions per word: AND, AND, ADD, XOR, ADD, XOR, OR, AND.

**Checks.**
1. **Every result equals the exact Python big-integer result.**
2. **Promotion happens if and only if some exact result does not fit,** checked at every width tried.
3. **Shrinking happens if and only if the rule holds,** with the packed test's decision compared with the rule on exact values at every step.

Also checked: a single promotion level always suffices, and the two fit tests match their definitions.

**Cases.**
- **Exhaustive at the smallest width:**
  - the packed shrink test on every word of width 8 and 16 (h = 4 and 8) for every target g < h (393,728 cases);
  - the extracted-half test on every 2h-bit value for h = 4 and 8;
  - every pair of elements at h = 4 as one-element arrays (225² pairs), under rules A and B, for add and sub by both detections, mul, neg, conj, division with remainder, and exact division (all in-range products);
  - mod n: all residue quadruples for n = 2 to 7 at h = 4.
- **Random:** arrays of 8 elements at h = 8, 16, 32 and 64, each against operand arrays of any width up to its own, 1,500 rounds per width and rule. Values mix uniform and near the boundary (±HI, HI − 1, ±(HI/2 + 1)). Mod n used n = 2^(h−1) − 1, 2^(h−1) − 3 and 3 at each h.
- **Failures: none.** Promotions needing two levels at once: 0. An add needs at most h + 1 bits, and a product of symmetric halves at most 2h − 1.

**Bouncing.** Two kinds of walk, 4,000 operations each, arrays of 16 elements.

- **Mixed walks.** A target bit size changes every 250 operations among 5, 12, 20, 28 and 45 bits. Multiply by small values or divide by small values (each about 1.5 bits) pulls toward the target, and adds and subtracts of quarter-size values, negations and conjugations come in between. The count is the steps the array's width moves on the ladder for any reason, including being copied up to a wider operand's width.
- **Hovering walks.** One element starts just under a threshold (HI − d/2), and the walk alternately adds and subtracts d at that element. It was run at the h = 8, 16 and 32 thresholds, with d = 10, 100 and 1,000.

| walk | rule A: up / down per 1,000 ops | rule B: up / down per 1,000 ops |
|---|---|---|
| mixed, seed 1 | 8.8 / 8.0 | 3.0 / 2.2 |
| mixed, seed 2 | 2.0 / 1.2 | 1.8 / 1.0 |
| mixed, seed 3 | 11.2 / 10.5 | 3.2 / 2.5 |
| hover at h = 8, 16, 32 | 500 / 500 (every operation) | 0 / 0 |

Under rule A a value hovering at a threshold promotes on every add and shrinks on every subtract: one copy of the whole array per operation. Under rule B it never moves, because rule B places the array one width up as soon as a value comes within one bit of the threshold, and only shrinks once every value has a bit of margin. In the mixed walks rule B cuts width moves by about 3 times in two seeds out of three. The third seed spends most of its time far from thresholds, and the two rules are nearly equal there. The time spent at each width barely differs between the rules (rule B spends slightly more at the wider sizes).

**Instruction counts per element** (each "doesn't fit" bit ORed into the accumulator costs 1, included).

| | instructions |
|---|---|
| fit test on two extracted halves (2 + 2, OR 1, accumulate 1) | 6 |
| add or sub with the sign test (plain 5, overflow 4, half = −2^(h−1) 5, OR 1, accumulate 1) | 16 |
| add or sub at double width (extract 6, 2 add/sub, fit 4, OR 1, pack 3, accumulate 1) | 17 |
| mul at double width (plain 15, fit 4, OR 1, accumulate 1) | 21 |
| shrink test, packed (8, accumulate 1); rule B differs only in its constants | 9 |
| copy to the next width up or down (extract 3, pack 3) | 6 |
| once per array: test the accumulator (compare, branch) | 2 |

A promotion costs a copy of both arrays (6 per element each) and a full redo of the operation at the wider width. A shrink costs 6 per element per level. Testing for a shrink after every operation costs 9 per element, half as much again as an add. Shrinking is not needed for correctness, so the test could run less often: after operations that can shrink values, or every k operations. Neither was measured here.

## Study 13: inverses mod n by the norm route (inv_norm.py)

**What was checked.**
- **Rational modulus n.** x = a + bi is a unit mod n exactly when N(x) = a² + b² is, and then x^−1 = conj(x)·N(x)^−1 mod n. The integer inverse of N(x) was taken two ways:
  - **(i) extended Euclid:** a data-dependent loop with one divide per step;
  - **(ii) Bernstein–Yang divsteps** with a fixed iteration count, for odd n: masks only, with no multiply and no divide.
  The divstep invariants are f ≡ d·g₀ and g ≡ e·g₀ (mod n), starting from (δ, f, g, d, e) = (1, n, g₀, 0, 1). Each step:
  - if δ > 0 and g is odd, then (δ, f, g, d, e) ← (−δ, g, −f, e, −d), by a conditional swap and negation;
  - then δ += 1, g ← (g + (g mod 2)·f)/2, and e ← (e + (g mod 2)·d)/2 mod n (halving mod n: add n when odd, then shift).
  At the end g = 0, f = ±gcd, and the inverse is ±d.
- **Iteration count.** ⌊(49d + 80)/17⌋ for d < 46 and ⌊(49d + 57)/17⌋ otherwise. This is Bernstein and Yang's bound as I remember it; it was not looked up, so it is checked here instead.
- **Gaussian modulus ν = c + di with gcd(c, d) = 1.** Z[i]/(ν) ≅ Z/N(ν), with i ↦ t = −c·d^−1 mod N (one integer inverse per modulus). x = p + qi maps to p + q·t mod N, which is inverted as an integer. The result z is itself a Gaussian representative of x^−1 mod ν, reduced to a small one by study 6's rounded division by ν.
- **The norm trick fails for a Gaussian modulus.** For ν = 2 + i and x = 2 − i, N(x) = 5 is not a unit mod N(ν) = 5, yet x ≡ −1 is a unit mod ν. This was checked.
- **Batch inversion (Montgomery's trick)** over an array sharing n: prefix products, one inverse, then two products per element going back.

**Cases.**
- **The divstep bound, exhaustive for d = 1 to 11:** every odd f < 2^d and every g < 2^d, counting divsteps until g = 0. The worst case is always below the bound, by 5 to 6 steps (d = 11: worst 30, bound 36). At d = 31, 20,000 random odd n and g < n needed at most 74 against the bound's 94.
- **Rational n = 2 to 39:** every residue a + bi against a brute-force search over all residues.
  - "Unit iff N(x) is a unit" was checked in 20,539 cases.
  - Euclid matched the brute-force inverse in 13,426 cases and reported 7,113 non-units.
  - Divsteps (odd n) matched in 9,032 cases and reported 1,627 non-units.
- **Gaussian ν with |c|, |d| ≤ 6, coprime:** x over a 9 × 9 box against brute force over the N(ν) classes (7,452 cases). Invertible exactly when a brute-force inverse exists, x·x^−1 ≡ 1 mod ν, and the representative is reduced (2N(r) ≤ N(ν)).
- **Batches** of 1, 2 and 5 elements for n = 5, 7, 9, 13 and 15: the batch equals the individual inverses, or reports failure when any element is not a unit.
- **Random, 31-bit halves:**
  - the three largest primes below 2^31, plus 2^31 − 2, 3·5·7·…·23 and 3·5·65,537, with 4,000 x each, by both methods (divsteps for odd n only);
  - a batch of 64 per modulus;
  - 300 random ν with 31-bit coprime parts, 30 x each.
- **Failures: none.**

**Steps.** Extended Euclid on 31-bit n: mean 16.67, max 32. On N(ν) (up to 63 bits): mean 35.82, max 53. Divsteps always run the fixed count: 94 at 31 bits, 184 at 63 bits.

**Instruction counts** (extended Euclid per step: DIV, MUL and SUB for the remainder, MUL and SUB for the coefficient, loop test = 6, with 2 MUL and 1 DIV. Divsteps per step = 37: condition 4, swap and negate f, g 6, δ 3, swap d, e 4, e ← n − e 4, g update 5, e update 6, halve mod n 5).

| rational n, 31-bit | instructions | multiplies | divides |
|---|---|---|---|
| norm route around the integer inverse (extract 4, N mod n 10, two products by N^−1 mod n 17, pack 2) | 33 | 10 | 0 |
| integer inverse, extended Euclid (6 per step, + 3) | ≈ 103 mean, 195 max | ≈ 33 mean | ≈ 17 mean |
| integer inverse, divsteps (94 × 37, + 6) | 3,484 always | 0 | 0 |
| **x^−1, Euclid** | ≈ 136 mean, 228 max | ≈ 43 | ≈ 17 |
| **x^−1, divsteps** | ≈ 3,517 | 10 | 0 |

The fixed-count divsteps cost about 25 times Euclid's mean in instructions, in exchange for no divides, no branches and a fixed length (the shape vector lanes need). Two measures from the survey were not tried here: batching divsteps into 2 × 2 transition matrices on low bits, and delaying the coefficient halving into one final multiply by 2^−K mod n. Both cut the per-step coefficient work, which is 19 of the 37 instructions.

**Gaussian modulus**, per element:
- the map in: one 64-bit mulmod (a 64 × 64 → 128-bit product) and an add mod N;
- an integer inverse mod N(ν) < 2^63: Euclid at about 36 steps, ≈ 220 instructions; divsteps at 184 × 37 ≈ 6,800;
- the reduction back mod ν: two roundings of 128-bit products.
Per modulus there is one more inverse (for t). Two things were not counted to the instruction. The 128-bit steps need 64 × 64 → 128 products, which are missing from some vector units (survey §4), and a Barrett-style rounding, because the 128-by-64 divide is not portable (RULES). With 63-bit N the divstep values f + g need 65 bits, so a 64-bit divstep loop does not fit as written. Staying in the Z/N image for a whole computation avoids the map back.

**Batch inversion** (rational n, Gaussian product mod n 27 instructions with 8 multiplies, study 4). The cost is 3(k − 1) products plus one inverse, per element:

| k | with Euclid | with divsteps | multiplies per element |
|---|---|---|---|
| 1 (no batch) | 136 | 3,517 | 43 / 10 |
| 16 | ≈ 84 | ≈ 296 | ≈ 25 |
| 64 | ≈ 82 | ≈ 135 | ≈ 24 |
| 1,024 | ≈ 81 | ≈ 85 | ≈ 24 |

With batching the inverse's cost becomes negligible, and the method of inversion stops mattering, from a few hundred elements. One non-unit makes the whole batch fail (detected by the single inverse failing); the toy then returns failure and does not bisect.

## Study 14: the extended (1+i)-ary gcd (xgcd1i.py)

**What was checked.** Write p(z) = (re + im) mod 2, which is z mod (1 + i). Dividing by 1 + i is z/(1 + i) = ((x + y) + (y − x)i)/2 when p(z) = 0.

- **Stripping.** The common power is removed exactly first: A = π^k A′, B = π^k B′ (π = 1 + i), so one of A′, B′ is prime to π.
- **Invariants.** The algorithm keeps a = u_a A′ + v_a B′ and b = u_b A′ + v_b B′.
- **Dividing a by π** (the survey's division-free trick). First add m·(B′, −A′) to (u_a, v_a), which leaves the sum unchanged. m ∈ {0, 1} is chosen so both coefficients become divisible: m = p(u_a) when B′ is prime to π, else m = p(v_a). This works because p(a) = p(u_a)p(A′) + p(v_a)p(B′) = 0.
- **Making a primary** multiplies a by a unit, and the coefficients by the same unit.
- **At the end,** g = π^k·g′ with the same coefficients (since A = π^k A′), and one normalising unit is applied to g and both coefficients.

The versions:
- **Loop version:** study 11's best variant (L1 size |x| + |y|, stop when equal). Strip a, make a and b primary, order them. While a ≠ b: a −= b (and the coefficients), divide out π while p(a) = 0, make a primary, and swap if smaller.
- **Branch-free version:** a fixed number T of micro-steps. Each selects by the mask p(a) between "even" (a ← a/π with the coefficient fix) and "odd" (make a primary, swap if smaller, a ← a − b). Once a = 0, every micro-step is "even" on 0: a stays 0, and b, u_b, v_b never change, so the fixed count needs no freeze mask. The coefficients u_a, v_a keep changing but stay bounded; they were tracked.
- **T from a proof.** An odd micro-step grows N(a)·N(b) by at most (1 + √2)², because the L1 ordering gives N(b) ≤ 2N(a). The next ≥ 3 even micro-steps divide it by 8. That is a factor ≤ 0.729 per 4 micro-steps, starting from N(A′)·N(B′) < 2^(4K−2). So T = 4⌈(4K − 2)/log₂(8/(1 + √2)²)⌉ + 4 + 2K: 98 at K = 3, 136 at K = 4, 174 at K = 5, 1,172 at K = 32. The run asserts a = 0 after T steps.
- **For comparison, extended Euclid:** study 6's division with nearest rounding, with both coefficients (s, t ← t, s − q·t).

Every result was checked against study 6's gcd (its rational reference) and for the Bezout identity g = uA + vB exactly.

**Cases.** Exhaustive over all pairs of in-range values: K = 3 (2,401) and K = 4 (50,625), all three algorithms. K = 5 (923,521 pairs): the loop and extended Euclid exhaustive, branch-free on a 2% random sample (18,224 pairs). K = 32: 20,005 random pairs (study 6's mix plus edge pairs) for the loop and extended Euclid, and branch-free on the first 2,005. **Failures: none.**

**Counts.**

| K | loop: outer steps mean / max | loop: divisions by 1 + i mean / max | branch-free: micro-steps needed mean / max (T run) | extended Euclid steps mean / max |
|---|---|---|---|---|
| 3 | 1.19 / 3 | 4.97 / 11 | 7.17 / 15 (98) | 1.88 / 3 |
| 4 | 2.20 / 5 | 9.14 / 18 | 12.34 / 24 (136) | 2.58 / 4 |
| 5 | 3.21 / 8 | 13.24 / 26 | 17.46 / 35 (174) | 3.24 / 6 |
| 32 | 22.68 / 41 | 91.09 / 149 | 114.72 / 183 (1,172) | 14.07 / 27 |

**Coefficient size.** The (1+i)-ary coefficients reach about 3.4 times the input bound: the largest part seen at K = 32 is about 2^32.8, against inputs below 2^31 (up to 2 extra bits). So they do not fit the K-bit halves, but fit easily in 2K-bit registers. The same holds at small K (58 against inputs below 16 at K = 5). Euclid's coefficients were not tracked.

**Instruction counts** (scalar, extracted halves; unit multiplies by masks as in study 9).

| part | instructions | multiplies | divides |
|---|---|---|---|
| (1+i)-ary loop: one division by 1 + i with the coefficient fix (a 4, m: parity 2 + mask 1, u: masked add of B′ 4 + divide 4, v: the same 8, inner loop test 3) | 26 | 0 | 0 |
| (1+i)-ary loop: one outer step (subtract a, u, v 6; primary on a, u, v: masks 6 + 3 × 11; L1 size 7; compare 2; swap 7 words 28; loop test 4) | 86 | 0 | 0 |
| branch-free micro-step (even 23, odd 82, mask 3, select 13 words 39) | 147 | 0 | 0 |
| extended Euclid step (study 6's division 49; two coefficient updates s − q·t, 4 MUL + 4 add/sub each, 16; loop test 1) | 66 | 20 | 2 |

| per extended gcd, K = 32 | instructions | multiplies | divides |
|---|---|---|---|
| (1+i)-ary loop, at the mean (22.68 × 86 + 91.09 × 26, plus about 150 for setup and finish) | ≈ 4,470 | 0 in the loop | 0 |
| (1+i)-ary branch-free, at the proven T (1,172 × 147) | ≈ 172,000 | 0 | 0 |
| (1+i)-ary branch-free, at the largest count seen (183 × 147; not a bound) | ≈ 26,900 | 0 | 0 |
| extended Euclid, at the mean (14.07 × 66, plus normalising g and two coefficients about 44) | ≈ 973 | ≈ 281 | ≈ 28 |
| for reference, gcd only: study 11's best (1+i)-ary, Euclid (study 6) | ≈ 1,500, ≈ 725 | | |
| for reference, study 13: x^−1 mod a 31-bit n by the norm route, with Euclid or divsteps | ≈ 136, ≈ 3,517 | | |

Keeping both coefficients roughly triples the (1+i)-ary gcd. The divisions by 1 + i, about four per outer step, each carry two coefficient fixes. The extended (1+i)-ary loop is about 4.6 times extended Euclid in instructions, but uses no multiply and no divide. Its branch-free form at the proven step count is far dearer still. The bound (1,172) is 6.4 times the largest count seen (183), because the per-step factor 0.729 in the proof is far from typical. For an inverse modulo a rational n, the norm route of study 13 remains much cheaper than either extended gcd: about 136 instructions with Euclid's integer inverse.

## Study 15: the overflow and shrink checks recounted (checks.py)

**What was checked.** The cheapest correct mask forms of each check, in two models and two variants.
- **Models:**
  - **Scalar:** the packed word in a 64-bit register, as in earlier studies.
  - **Vector:** 32-bit lanes for add, sub, neg, conj and shrink. Mul uses the widening 32 × 32 → 64 lane multiply, counted in its SSE4.1/AVX2 form; NEON (`smull`, `uzp`) and RVV (`vwmul`, `vnsrl`) have the same steps but were not counted instruction by instruction.
- **Variants:**
  - **A:** keeps −2^31 reserved, so the range is symmetric.
  - **B:** drops the reservation and uses the full two's-complement range.

Every check ORs a contribution into a sticky word, and the mask that picks the meaningful bits is applied once per array. That is valid because (OR of tᵢ) AND m = OR of (tᵢ AND m). Checks whose meaningful bits differ keep separate sticky words: the sign bits for add, sub and neg; the high half's sign bit for scalar conj; the high h bits of the double-width parts for mul; and bits g and up for shrink.

**A correction to earlier counts.** Under the book's rules, the packed add used since toy.py, ((x AND NH) + (y AND NH)) XOR ((x XOR y) AND H), is 6 instructions, not 5. The packed subtract, ((x OR H) − (y AND NH)) XOR ((x XOR NOT y) AND H), is 7 (6 with ARM64 `eon` or RISC-V `xnor`), not 5. Both have a 5-instruction form: a full add (or subtract), then a fix for the carry (or borrow) that crossed into the high half.

- **add:** s = x + y, t = x XOR y, r = s − ((t XOR s) AND 2^32).
- **sub:** s = x − y, t = x XOR y, r = s + ((t XOR s) AND 2^32).

Bit 32 of x XOR y XOR s is the carry (or borrow) into the high half. These forms also leave t = x XOR y in a register for the overflow test. The arithmetic counts in studies 1, 2, 7 and 12 for add and sub should read 5 with these forms (they used 5 for forms that cost 6 and 7).

**The cheapest forms, all verified.**
- **add overflow:** (x XOR r) AND NOT t, which needs t from the add. That is XOR, ANDN, then OR into the sticky: 2 + 1 with `andn` (x86-64 BMI1 `andn`, ARM64 `bic`, RISC-V Zbb `andn`), or 3 + 1 without. So 3 + 1 can be beaten: 2 + 1, by sharing x XOR y with the arithmetic and deferring the sign-bit mask.
- **sub overflow:** t AND (x XOR r). That is XOR, AND, OR: 2 + 1 with no `andn` needed.
- **neg:** s = 0 − x, then add back the borrow, (x XOR s) AND 2^32. Under B, a half overflows exactly when x and −x are both negative, which happens only at −2^31: x AND r, then OR (2).
- **conj:** r = (x AND ML) − (x AND NOT ML), which is lo − hi·2^32 with no borrow (3). Under B the check is x AND r at the high half's sign bit, kept in its own sticky (2).
- **The half = −2^31 test (variant A only):** r AND NOT ((r AND NH) + NH), then OR: 3 + 1.
- **mul, "add form":** a part v fits exactly when (v + 2^31) has nothing in bits 32 and up (B). Under A, both v + (2^31 − 1) and v + 2^31 must have nothing there. The ORs are taken first and the bit mask applied once per array: under B, ADD, ADD, OR, then OR into the sticky (4).
  - The specified "shift form" ((v shl 32) sar 32 XOR v per part) gives the same answers, but costs 3 per part against 2.
  - The compare form, (v + HI) > 2HI unsigned (study 2), costs 2 per part. Its compare counts 1 by the book's convention, but takes two instructions on x86-64 and ARM64 (`cmp` and `setcc`/`cset`).
- **Vector lanes:** add and sub are 1 instruction (lanes never carry into each other), and the overflow test is XOR, XOR, AND (3), then OR. NEON (`sqadd`) and RVV (`vsadd`) have 32-bit saturating adds, which would give a 2-instruction test (saturated ≠ wrapped), but SSE/AVX2 have no 32-bit saturating add, so it is not portable. Variant A's −2^31 test is one compare-equal against −2^31 (`pcmpeqd`, `cmeq`, `vmseq`), then OR. RVV compares give mask registers, so that sticky is kept with `vmor`.
- **Shrink (packed scalar):** t = ((w AND NH) + C) XOR (w AND H) with C = 2^(g−1) in each half, and the bits g and up are tested once per array (B: 4 + 1). A also needs u, the same with C − 1, ORed in (7 + 1). In vector lanes it is x + 2^(g−1) (B: 1 + 1) and also x + 2^(g−1) − 1 (A: 3 + 1). A wrapped lane sum cannot land below 2^g, because x < 2^31 (verified).

**Verification.** Each check: the flag (contribution AND end mask ≠ 0) is set exactly when the true result does not fit; each result equals the true value mod 2^h; and the sticky over an array of 16 is set exactly when any element does not fit.
- **h = 4:** every word pair (scalar), every lane pair (vector), every 8-bit part (mul checks).
- **h = 8:** neg, conj and shrink on every word. Add, sub and mul exhaustive in each half, with the other half from 20 edge and random pairs, about 2.8 million word pairs per variant. Vector add and sub on every lane pair, and the mul checks on every 16-bit part.
- **h = 32:** 100,000 random pairs per variant.
- **Arrays:** 20,000 deferred arrays (add and mul) per variant.
- **Failures: none.** Under B one corner exists, (−2^31)(−2^31) · 2 = 2^63, which overflows the 64-bit part itself. It wraps to −2^63 and is still flagged.

**Table.** Instructions per operation, as arithmetic / check / total. Check counts include the OR into the sticky. Scalar is one packed word, one Gaussian integer. Vector is one vector register, which holds 2 Gaussian integers at 128 bits and 4 at 256 bits.

| operation | scalar, A | scalar, B | vector, A | vector, B |
|---|---|---|---|---|
| add | 5 / 7 / 12 (14 without andn) | 5 / 3 / 8 (9 without andn) | 1 / 6 / 7 | 1 / 4 / 5 |
| sub | 5 / 7 / 12 (13 without andn) | 5 / 3 / 8 | 1 / 6 / 7 | 1 / 4 / 5 |
| neg | 4 / 0 / 4 | 4 / 2 / 6 | 1 / 0 / 1 | 1 / 2 / 3 |
| conj | 3 / 0 / 3 | 3 / 2 / 5 | 2 / 0 / 2 | 2 / 2 / 4 |
| mul, add form | 15 / 8 / 23 | 15 / 4 / 19 | 11 / 8 / 19 | 11 / 4 / 15 |
| mul, compare form (compare = 1) | 15 / 6 / 21 | 15 / 6 / 21 | (no unsigned 64-bit compare in SSE/AVX2) | |
| mul, shift form | 15 / 12 / 27 | 15 / 8 / 23 | | |
| shrink test (32-bit halves into 16) | 8 | 5 | 4 | 2 |
| once per array, per sticky word (mask, test, branch) | 2 to 3 | 2 to 3 | 2 to 3 | 2 to 3 |

**What the variants need.**
- **A:** neg and conj need no check. The symmetric range is closed under negation, and −(−2^31) = −2^31 keeps the reserved value in place. Add and sub pay for the "half = −2^31" test: 4 scalar (3 + OR), 2 vector (compare-equal + OR). Mul pays 4 more in the add form, for its second offset.
- **B:** add, sub and mul are cheaper (no reserved value to catch). neg and conj each need a 2-instruction check, because −(−2^31) overflows. Under B, −2^31 is an ordinary value, so the type has no reserved marker.

Against the earlier counts: study 7's deferred add was 16 in total (a 5 that was really 6, and a check of 4 + 5 + 1 + 1). The cheapest correct forms give 12 under A (8 under B) in scalar, and 7 under A (5 under B) per vector register.

## Study 16: exact Gaussian integers in a quadratic residue number system (qrns.py)

**What was checked.** Primes p ≡ 1 (mod 4) below 2^31, each with t² ≡ −1. Per prime, a + bi ↦ (u, v) = (a + bt, a − bt) mod p (study 5's split), so add and multiply are lane by lane: 2 residues per prime per Gaussian integer, m primes, M = p₁⋯p_m. The full two's-complement range is used (decision 12), so inputs are any 32-bit halves.

- **Conversion in:** per prime, a mod p = Barrett(a + 2p) (a + 2p > 0 because p > 2^30), the same for b, then b·t mod p, u = a + bt, v = a − bt.
- **Conversion out:** per prime, a = (u + v)/2 and b = (u − v)·(2t)^−1. Then Garner's mixed-radix digits over the primes, Horner to binary, and the centred value in (−M/2, M/2). Exact while |re|, |im| < M/2.
- **Scalar residues:** Barrett (study 4).
- **Vector lanes:** Montgomery with R = 2^32 in 32-bit lanes. REDC uses only the widening 32 × 32 → 64 multiply and a low 32-bit multiply, so it needs no 64-bit mulhi, which SSE/AVX2 and NEON lack. Then a conditional subtract by the unsigned-min trick, min(u, u − p). A chain of k products picks up R^−k; that is undone at conversion out by folding R^k into the per-prime constants (R^k/2 and R^k·(2t)^−1). The halving for a then becomes a constant multiply, which is included in the vector conversion-out count.

**Cases.**
- **Toy:**
  - Montgomery lanes (h = 8), lane add and sub, and the scalar Barrett product, for every residue pair and every prime p ≡ 1 (mod 4) below 128 (68,414 pairs each).
  - RNS over (5, 13, 17) with redundant prime 29 (M = 1,105): the round trip for every a, b with |a|, |b| ≤ 552 (1,221,025).
  - RNS over (5, 13) with redundant prime 17 (M = 65): add and multiply for every pair with parts in −15 to 15 (923,521 each). Exact when the result fits, and flagged by the redundant prime otherwise (854,320 products flagged).
  - Random chains of 1 to 4 adds and multiplies: exact when they fit (15,236), flagged otherwise (4,661).
- **31-bit primes** (the seven largest p ≡ 1 (mod 4) below 2^31, from 2,147,483,629 down):
  - Montgomery lanes at h = 32 for three primes, 30,000 pairs each.
  - Products of k + 1 inputs with full-range 32-bit halves (edges −2^31 and 2^31 − 1 included), k = 0 to 4, 3,000 each with m sized by the bound: all exact.
  - The same with one prime fewer: every result that does not fit is flagged by the redundant prime (5,448), and every one that fits is exact (9,552).
- **Bound tracking** was never "safe" for a result that did not fit (20,691 checks).
- **Failures: none.** A first run reported 2,079 bound-tracking failures. That was a harness bug: a chain with no operations never had its input bound checked. Fixed and rerun.

**Primes needed** for products of k + 1 inputs with 32-bit halves (|z| < √2·2^31, so |re|, |im| < 2^(31.5(k+1)), plus a sign):

| multiplies k | 0 | 1 | 2 | 3 | 4 |
|---|---|---|---|---|---|
| primes m (bits of M) | 2 (62) | 3 (93) | 4 (124) | 5 (155) | 6 (186) |

Each further add needs about 1 bit, so roughly one more prime per 31 adds.

**Instruction counts per Gaussian element.**
- **Scalar** (64-bit registers, Barrett): add 5, sub 4 and multiply 8 per residue.
- **Vector** (128-bit registers of four 32-bit lanes; an element uses 2m lanes, and 256-bit registers halve every vector figure):
  - add: ADD, SUB, unsigned MIN = 3 per register;
  - sub: SUB, ADD, MIN = 3;
  - Montgomery multiply ≈ 12 per register (widening multiply 2, low multiply for m with its narrowing 2, widening m·p 2, 64-bit add 2, take the high halves 2, conditional subtract 2). The arithmetic was verified; the instruction count is a chip-agnostic estimate, ±2 by chip.
  - The vector conversion out runs the per-prime unsplit and Garner in lanes; the Horner step to binary and the centring are scalar multiword (no portable 64 × 64 → 128 in vectors).

| per element | scalar | vector (128-bit) |
|---|---|---|
| conversion in (extract 3; per prime: a mod p 8, b mod p 8, b·t 8, u 5, v 4 = 33; vector per prime 7: sign fix 5 ×2, Montgomery 12, u 3, v 3 per 4 elements) | 33m + 3 | ≈ 7m + 0.5 |
| add | 10m | 1.5m |
| sub | 8m | 1.5m |
| multiply | 16m | 6m |
| conj (swap u and v) | 0 (register choice) | 0 |
| conversion out (unsplit 22 per prime; Garner 12 per digit step, m(m − 1)/2 steps, two parts; Horner and centring multiword) | 22m + 12m(m − 1) + 2(Horner + centring): 82, 172, 286, 434 for m = 2 to 5 | ≈ 7.5m + 4m(m − 1) + the scalar Horner and centring |

**Break-even.**
- **At 32-bit halves, never.** The direct multiply with full-range checks (study 15) is 19 scalar, or 15 per 128-bit register of two elements (7.5 per element). The RNS multiply is 16m scalar and 6m vector per element. With m ≥ 2, which 32-bit halves need, that is 32 against 19 scalar and 12 against 7.5 vector: the RNS multiply costs more, so no number of multiplies recovers the conversions.
- **Where it can win: results the direct type would hold in wider halves.** Study 3's stack multiply (10L² + 24L − 2 for halves of 32L bits, wrapping, before any overflow check) is the proxy for the direct multiply at those widths:

| result halves | direct multiply (stack of L) | RNS primes m | RNS multiply (scalar) | saved per multiply | conversion in + out | break-even, multiplies per converted value |
|---|---|---|---|---|---|---|
| 64-bit (L = 2) | 86 | 3 | 48 | 38 | 102 + 172 | ≈ 7 |
| 96-bit (L = 3) | 160 | 4 | 64 | 96 | 135 + 286 | ≈ 4.4 |
| 128-bit (L = 4) | 254 | 5 | 80 | 174 | 168 + 434 | ≈ 3.5 |

In vector lanes the RNS keeps 32-bit lanes and grows linearly (6m per element), while a direct multiply at 64-bit halves needs 64 × 64 → 128 products that SSE/AVX2 and NEON do not have. RNS's real case is long multiply-heavy computations whose results outgrow 32-bit halves, converted once at each end.

**Overflow.** RNS cannot see overflow: a result outside (−M/2, M/2) silently wraps mod M. The options:

1. **Bound tracking.** Keep a bound on |z| per array (add: B₁ + B₂; multiply: B₁·B₂, starting from √2·max|half|) and compare it with M/2. That is a few scalar instructions per array operation, nothing per element. It is conservative (it assumes worst-case growth), so it may ask for more primes than the data needs, and it was verified sound.
2. **A redundant extra prime.** Carry one more prime and check at conversion out that the reconstructed value agrees with it. The cost is one more prime of work in every operation (+10 add and +16 multiply scalar, +1.5 and +6 vector, per element). At conversion out, the redundant residue is unsplit (22) and the reconstructed value reduced mod p_r from its mixed-radix digits (about 13m per part). This catches every result with |x| < M·p_r/2: x′ − x = kM with 0 < |k| < p_r, and p_r does not divide kM. Verified.
3. **Sizing in advance.** Choose m from the computation's known depth (the table above). Zero runtime cost, but it needs the bound before the computation starts.

**Comparison and division.**
- **Equality** is cheap: all 2m residues equal (2m compares, or 2m/4 per vector register, then OR).
- **Order and sign** need magnitude, which the residues do not show: a mixed-radix conversion of the difference (Garner, about 12m(m − 1)/2 per part) and a centred test on the top digits. That is O(m²), much like a conversion out.
- **Exact division** by z is a multiply by z's inverse residues when z is a unit mod every prime (both u and v nonzero for each p). The inverses cost 2m modular inverses (study 13: about 100 instructions each by Euclid), once per divisor, then 16m per element. It fails when z shares a factor with some p, which needs a different prime set.
- **Division with remainder** (nearest Gaussian quotient) needs the magnitude: convert out, divide (study 6), and convert back in.

## Study 17: three memory layouts at the vector level (layout.py)

**What was checked.** The same type in three layouts, with study 15's full-range checks (decision 12) ORed into sticky words (decision 13):

- **(a) Interleaved:** 32-bit lanes re, im, re, im, ... This is the packed word seen as lanes, as now.
- **(b) Structure of arrays:** one array of real parts and one of imaginary parts. A register holds four real parts or four imaginary parts.
- **(c) Whole-word:** the packed word in 64-bit lanes, using 64-bit instructions only: study 15's scalar forms applied per lane, with the carry-fix add and subtract.

The operations are built from a small simulated vector machine. A register is 4h bits (128 at h = 32), viewed as 4 h-bit lanes or 2 2h-bit lanes. Each primitive counts one instruction, so the counts below are exact for the instruction sequences written:
- lane add, sub, xor, and, andn, or, and shifts;
- `mul_even` (x86 `pmuldq`: signed low halves of 64-bit lanes);
- `wmul_lo`/`wmul_hi` (NEON `smull`/`smull2`; RVV `vwmul` does both in one register group);
- `narrow2` (NEON `uzp1`, RVV `vnsrl`);
- `swap_pairs` (x86 `pshufd`, NEON `rev64`) and an even/odd blend.

**Verification.** For each layout and operation: the results equal the true values mod 2^h; the sticky flag (contribution AND the end mask ≠ 0) is set exactly when some element in the register does not fit; and the shrink flag exactly when some half does not fit in h/2 bits.
- Each element was tested alone in a register (other slots zero), which gives exact per-element flags, and in full registers (flag iff any element is bad).
- **h = 4:** every element pair.
- **h = 8:** real parts exhaustive, with imaginary parts from 10 edge and random pairs, and the reverse (sampled every 7th pair alone and every 5th group in full registers).
- **h = 32:** 60,000 random pairs with edges.
- In all, 431,193 cases per operation for (a) and (c), and 346,273 for (b), with both multiply forms for (b). **Failures: none.**

**Instruction count per register,** including one OR into the sticky word where there is a check. A 128-bit register holds 2 Gaussian integers in (a) and (c); in (b) a pair of registers (one real, one imaginary) holds 4. The table is per element, and a 256-bit register halves every figure.

| operation | (a) interleaved | (b) structure of arrays | (c) whole-word, 64-bit |
|---|---|---|---|
| add (arithmetic + check) | 2.5 (1 + 4 per register) | 2.5 (2 + 8 per 4) | 4 (5 + 3 per register; RVV base without `vandn`: 4.5) |
| sub | 2.5 | 2.5 | 4 |
| neg | 1.5 | 1.5 | 3 |
| conj | 2 | 0.75 (only the imaginary array is touched) | 2.5 |
| mul (arithmetic + check) | 7.5 (11 + 4 per register; x86 form) | 5.5 generic (14 + 8 per 4), 7 with x86 `pmuldq` (20 + 8 per 4) | 7.5 (the same instructions as (a)) |
| of which lane rearrangement in mul | 2.5 (2 input shifts, a pair swap, a shift and a blend for the pack) | 0.5 generic (2 narrows), 2 on x86 (4 odd-lane shifts, 2 shifts and 2 blends to narrow) | 2.5 |
| shrink test (32-bit halves into 16) | 1 (1 + 1 per register) | 1 (3 + 1 per 4) | 2.5 (4 + 1 per register) |
| loads / stores per element: add, sub, mul | 1 / 0.5 | 1 / 0.5 | 1 / 0.5 |
| loads / stores: neg | 0.5 / 0.5 | 0.5 / 0.5 | 0.5 / 0.5 |
| loads / stores: conj | 0.5 / 0.5 | 0.25 / 0.25 in place (+ 0.25 / 0.25 to copy the real array) | 0.5 / 0.5 |
| loads / stores: shrink test | 0.5 / 0 | 0.5 / 0 | 0.5 / 0 |

The sticky words' end test (mask, test, branch) is once per array, about 2 to 3 per sticky word.

**What each layout does.**
- **(a) and (b)** cost the same for add, sub and neg. Lanes are independent and the checks are lane-wise, so the layout does not matter.
- **(b) removes most of the multiply's rearrangement.**
  - In (a) the real and imaginary parts share a register, so each product needs its operands lined up first: a shift to bring the imaginary parts down, a pair swap so a meets d and b meets c, and a shift and blend to repack.
  - In (b), a, b, c and d are already in separate registers, so the four products are straight widening multiplies of whole registers. Only the narrowing of the 64-bit results back to 32-bit lanes is left.
  - With NEON or RVV's low/high widening multiply that is 5.5 per element against 7.5. With x86's `pmuldq`, which only takes even lanes, (b) needs shifts for the odd lanes and blends to narrow, and comes to 7 against 7.5.
- **(b) makes conj (and multiply by i) cheap.**
  - Conj touches only the imaginary array: 0.75 per element and a quarter of the memory traffic, against 2.
  - Multiply by i is a swap of the two array pointers plus a negation of one array.
- **(c) is dearer for add, sub, neg and the shrink test.** In 64-bit lanes the two halves can carry into each other, so it pays study 15's carry fix (5 instructions for add against 1) and the per-half isolation in the shrink test. Its multiply is the same instruction stream as (a), since `pmuldq` reads each 64-bit lane's low half. One more gap: SSE/AVX2 have no 64-bit arithmetic right shift (AVX-512 has `vpsraq`), so (c) cannot sign-extend a low half with shifts. The forms here avoid it.
- **What (b) costs when one number is needed whole.**
  - **Reading one number:** two loads from two arrays (usually two cache lines), then a combine (shift and OR, or one insert or zip), against one load in (a) and (c).
  - **Writing one number:** a split and two stores.
  - **Streams:** every loop runs two address streams per array, against one.
  - **Converting a whole array** between (a) and (b) is one zip or unzip per register (NEON `zip`/`uzp`; x86 needs 2 to 3 shuffles). NEON `ld2`/`st2` and RVV segment loads and stores (`vlseg2`/`vsseg2`) do it during the load or store itself.
- **Memory traffic** per element is the same in all three layouts for add, sub and mul: the same bytes are moved. (b) moves fewer only where it can skip an array (conj, multiply by i, operations on one part).

The counts for (a) and (c) match study 15's vector and scalar-per-lane figures (add 5 and 8 per register, mul 15). Study 15's "mul 11 + 4" for the vector model is (a) here.

## Study 18: Gaussian matrix products, direct against residue (matmul.py)

**What was compared.** C = A·B, A n × k, B k × m, entries with b-bit halves in the full range. The result needs R = 2b + ⌈log₂ k⌉ + 1 bits, since |re C|, |im C| ≤ k·2^(2b−1). The widths and prime counts are chosen in advance from b and k, so there is no runtime overflow check.

- **Direct** (vector, study 17's layout (b) with separate re/im arrays; (a) as a variant). Per term (one complex multiply-add C_ij += A_it·B_tj), the cheapest chip-agnostic forms (NEON `smlal`, RVV `vwmacc`). x86 has no general widening multiply-add: its 16-bit `pmaddwd` multiplies and adds pairs (with interleaved inputs, one `pmaddwd` gives ac − bd), and at 32 bits `pmuldq` plus an add takes about twice the counts below:
  - **b = 8:** 16-bit inputs, 16 → 32 widening multiply-accumulate into 32-bit sums. Four per register of four sums, so 1 per term. B keeps a negated copy of its imaginary part for the −bd term.
  - **b = 16:** 32-bit inputs, 32 → 64 widening multiply-accumulate: 2 per term.
  - **b = 32:** four 32 → 64 products. ac − bd is exact in 64 bits, but ad + bc reaches 2^63 at the full-range corner (all halves −2^31), so ad and bc are taken separately. Each 64-bit part is split into its low 32 bits (AND) and high 32 bits (arithmetic shift), and added into two 64-bit sums. That is lazy accumulation with no carries, combined once per result entry (8). 8.5 per term.
  - **b = 64:** scalar, since there is no portable vector 64 × 64 → 128: four MUL and MULHI pairs, two-word sub and add, and two three-word accumulates. 32 per term.
- **Residue** (study 16). The primes p ≡ 1 (mod 4) of 31 bits needed, m_p = ⌈R/31⌉, come from a table. Every input entry is converted in, there are two lane multiply-adds per prime per term (split form), and every result entry is converted out.
  - **Lazy accumulation:** raw 62-bit products are summed in 64-bit lanes and reduced by Montgomery REDC (R = 2^32) every 2 terms. REDC needs t < p·2^32, and 2(p − 1)² < p·2^32.
  - **Cost:** per register of four residue lanes, a widening multiply 2 and a 64-bit add 2 per term, plus REDC 10 and an add mod p 3 every 2 terms. That is 2.625 per lane-term, and 5.25·m_p per Gaussian term. Reducing after every term costs 7.5·m_p.
  - **The R^−1 factors** are folded into the conversion-out constants.

**Cost formulas** (instructions; 128-bit registers; loads and stores not counted):

- Direct: n·k·m·T_d(b) + (n·k + k·m)·P_d(b) + n·m·Q_d(b).
  - T_d = 1, 2, 8.5, 32 for b = 8, 16, 32, 64.
  - P_d (input widening) = 0.25, 0.5, 0, 0.
  - Q_d (combining split sums) = 0, 0, 8, 0.
  - Layout (a) adds lane rearrangement per term: 0.75, 1.5, 1.5, 0. Preparing shifted and pair-swapped copies of each entry once removes it, for about 2 per entry.
- Residue: n·k·m·5.25·m_p + (n·k + k·m)·I(b, m_p) + n·m·O(m_p).
  - m_p = 1, 2, 3, 5 for b = 8, 16, 32, 64. These hold for k up to 2^13, 2^28, 2^27 and 2^25.
  - Conversion in, per entry: I = 7m_p + 0.5 for b ≤ 32, and 14m_p + 1 for b = 64 (64-bit halves are reduced in two 32-bit pieces).
  - Conversion out, per entry (study 16): O = 17.5, 37, 80.5, 201.5 for m_p = 1, 2, 3, 5.

**Per term,** the residue form costs 5.25, 10.5, 15.75, 26.25 against direct 1, 2, 8.5, 32. Only at b = 64 is a residue multiply-add cheaper than a direct one, because only there does the direct product leave the vector unit (64 × 64 → 128). Below that, a widening multiply-add does a term in 1 to 8.5 instructions, while every residue term pays two lanes per prime plus the reductions.

**Square matrices** (k = m = n), totals for direct (b) / direct (a) / residue lazy / residue reducing every term:

| n | b = 8 (m_p = 1) | b = 16 (2) | b = 32 (3) | b = 64 (5) |
|---|---|---|---|---|
| 2 | 10 / 16 / 172 / 190 | 20 / 32 / 348 / 384 | 100 / 112 / 620 / 674 | 256 / 256 / 1,584 / 1,674 |
| 4 | 72 / 120 / 856 / 1,000 | 144 / 240 / 1,728 / 2,016 | 672 / 768 / 2,984 / 3,416 | 2,048 / 2,048 / 7,176 / 7,896 |
| 8 | 544 / 928 / 4,768 / 5,920 | 1,088 / 1,856 / 9,600 / 11,904 | 4,864 / 5,632 / 15,968 / 19,424 | 16,384 / 16,384 / 35,424 / 41,184 |
| 16 | 4,224 / 7,296 / 29,824 / 39,040 | 8,448 / 14,592 / 59,904 / 78,336 | 36,864 / 43,008 / 96,128 / 123,776 | 131,072 / 131,072 / 195,456 / 241,536 |
| 32 | 33,280 / 57,856 / 205,312 / 279,040 | 66,560 / 115,712 / 411,648 / 559,104 | 286,720 / 335,872 / 642,560 / 863,744 | 1,048,576 / 1,048,576 / 1,211,904 / 1,580,544 |

**Crossovers** (the smallest square n from which the residue form is cheaper; searched up to n = 4,096):

| b | 31-bit primes, reduce every 2 terms | reduce every term | 27-bit primes, reduce every 32 terms |
|---|---|---|---|
| 8 | never | never | never |
| 16 | never | never | never |
| 32 | never (15.75 per term against 8.5) | never | n = 62 (3 primes, 6.6 per term) |
| 64 | n = 60 | never (37.5 against 32) | n = 17 (5 primes, 11 per term) |

The crossovers against layout (a) are the same: at b = 64 the direct method is scalar either way, and elsewhere the residue form does not win.

**The 27-bit-prime variant.** Smaller primes let 2^(32−27) = 32 products accumulate before a REDC (REDC needs t < p·2^32). That brings the per-lane cost to (4 + 13/32)/4 ≈ 1.1 and the per-term cost to 2.2 per prime, at the price of more primes for the same R: ⌈R/27⌉, which is 3 for b = 32 when k ≤ 2^15, and 5 for b = 64 when k ≤ 32. This is the one change that makes the residue form pay at b = 32. Checked functionally with k = 40 and 70, so that several reduction blocks occur.

**Verification.** Both methods were compared with exact big-integer matrix products. The direct method was run with its chosen widths, wrapping like the machine, which shows the widths suffice.
- **Exhaustive, tiny,** with toy primes 5 and 13 in 8-bit Montgomery lanes:
  - b = 1, every pair of 2 × 2 matrices (65,536 products);
  - b = 2, every 1 × 2 times 2 × 1 pair (65,536).
- **Random,** at b = 8, 16, 32 and 64 with the real choices (31-bit primes, R = 2^32): n = 2, 3, 4 and 8 (3 pairs each) and n = 16 (1). The first pair of each set has the corner entry (−2^(b−1), −2^(b−1)) in both matrices.
- **Also:** an 8 × 8 matrix times a vector at each b, and the 27-bit variant at b = 32 and 64 with k = 40 and 70.
- **Failures: none** in the final run. A first run failed 9 of 13 direct b = 32 cases: ad + bc overflowed int64 at the corner. The b = 32 direct form now splits ad and bc separately, and its cost went from 7 to 8.5 per term.

**Matrix times vector** (k × 1, so m = 1). Each entry of A is used once, so its conversion in is not shared. The residue form then pays I + 5.25·m_p per term against T_d, and loses at every b, by a margin growing with n² (at b = 64 and n = 1,024, about 69 million instructions). It pays only when the same A multiplies many vectors and is converted once. Then each vector costs n²·5.25·m_p + n·(I + O) against n²·T_d, and at b = 64 the residue form wins from about n = 48.

**Repeated products (powers A^e).** A is converted in once, the e − 1 products stay in residue form, and the result is converted out once. The conversions are spread over (e − 1)·n³ terms, so the crossover falls roughly as 1/(e − 1). The primes are sized for the final power, about e·b + (e − 1)·⌈log₂ n⌉ + 1 bits. In the direct method the intermediate products outgrow 64-bit halves after the first product, and from then on every term is a multiword multiply whose cost grows with the square of the width (study 3: 10L² + 24L − 2). The residue term cost grows only linearly with m_p. So powers move the crossover far down, to small n even at b = 8 or 16, once the entries outgrow one word. This was not counted in the sweep.

**Memory** (not counted above): a residue entry holds 2·m_p 32-bit lanes against 2 parts of the direct input width. At b = 32 that is 6 lanes against 2 per entry, so the residue form moves about 3 times the data per term. Register blocking reduces loads per term in both methods, but the ratio stays.

## Study 19: Barrett against Montgomery, scalar and AVX2 lanes, timed (reduce19.c)

**What was compared.** Reduction mod an odd n < 2^31, residues stored as uint32 in separate arrays (x, y for the product; re and im of each factor for the Gaussian product, study 17's layout (b)). Two kernels: one product xy mod n, and the Gaussian product ((ac − bd) mod n, (ad + bc) mod n). Five versions:

- **div:** scalar t % n, the hardware divide, for reference.
- **Barrett scalar:** as study 4, m = ⌊2^64/n⌋, q = mulhi(t, m), r in [0, 2n), one conditional subtract (gcc emits cmov). The Gaussian forms t = ac + (n² − bd) and ad + bc, both below 2n².
- **Montgomery scalar:** R = 2^32 (study 4 used 2^64), n′ = n^−1 mod 2^32, m = t·n′ mod 2^32, r = (t − mn)/2^32 in (−n, n), add n where negative. The Gaussian takes ac − bd as a signed t with a signed m (|t| < n² keeps r in (−n, n)), and ad + bc with an unsigned m. So no n² offset is needed.
- **Barrett in AVX2 lanes:** only the 32 × 32 → 64 multiply (vpmuludq). Let L = ⌈log₂ n⌉ and mu = ⌊2^(L+31)/n⌋ < 2^32. The product uses q = ((t >> (L − 1))·mu) >> 32, and the Gaussian (t < 2n²) uses q = ((t >> L)·mu) >> 31, each in even and odd 64-bit halves. The proven quotient error is at most 2 for the product and 3 for the Gaussian. An exhaustive scan over every x = t >> s (the worst t for each x is x·2^s + 2^s − 1) shows both bounds are reached: at n = 1,073,741,827, errors of 2 and 3 occur, while at n = 2^31 − 1 the worst is 1 and 2. So r can reach 3n or 4n, which overflows 32 bits once n > 2^32/3. The fix is to subtract 2n where r ≥ 2n in 64-bit lanes (vpcmpgtq, and, sub), pack to 32-bit lanes, then subtract n where r ≥ n (min(r, r − n), unsigned). With a 32-bit x and a 32-bit mu there is no way to get the error down to 1 at 31-bit n: dropping bits of t and rounding mu down each cost about one unit near the top of the range.
- **Montgomery in AVX2 lanes:** as study 16. vpmuludq gives m, vpmuludq (vpmuldq for the signed Gaussian real part) gives mn, a 64-bit sub, take the high dwords (shift and blend), then min(r, r + n).

Conversion into Montgomery form is REDC(x·(R² mod n)); out is REDC(x). Both were timed as separate kernels.

**Correctness.** Every version was compared with an exact reference (__int128 in C) through the whole Montgomery path (convert in, multiply, convert out). The Montgomery-form output (product·R mod n) and the conversion in were also checked on their own.
- Exhaustive: the product over all (x, y) for every odd n from 3 to 1,023 (178,960,376 cases per version); the Gaussian over all (a, b, c, d) for every odd n from 3 to 63 (107,287,032 per version).
- Random, 10^7 elements per version per kernel per n, with a quarter of residues drawn from 0, 1, 2, n − 1, n − 2 and n/2. The ten n: primes 2,147,483,647, 2,147,483,629, 1,073,741,827, 1,000,003 and 65,537; composites 2,147,483,645, 2,147,483,643, 1,431,655,767, 1,073,741,825 (2^30 + 1) and 3·5·…·23.
- The lane Barrett reduction was also tested directly: 4·10^6 t per shape per n over the whole range [0, n²) or [0, 2n²), weighted to the top of the range and to just below multiples of n, plus the two worst-case t from the scan.
- 1,600 Gaussian outputs of each version were checked independently in Python.
- **Failures: none.** The same checks pass for the -march=native build.

**Instruction counts** (whole inner loop from objdump, per element, including loads, stores and loop overhead; gcc 13.3 -O2 -mavx2; vector loops do 8 elements per pass; a vector multiply covers 4 lanes):

| per element | div | Barrett scalar | Montgomery scalar | Barrett AVX2 | Montgomery AVX2 |
|---|---|---|---|---|---|
| product | 9 (1 div) | 16 (3 mul) | 16 (3 mul) | 3.88 (0.75 mul) | 2.62 (0.75 mul) |
| Gaussian product | 22 (2 div) | 42 (8 mul) | 41 (8 mul) | 8.38 (2 mul) | 5.62 (2 mul) |
| conversion in / out | | | 15 / 14 | | 2.25 / 1.88 |

In lanes the two methods use the same multiplies. Barrett's extra cost is the rest: per half of four lanes it has a variable shift, a shift, the 64-bit compare, and and subtract (8 against Montgomery's 3), plus the n² offset in the Gaussian. In scalar code the counts are equal. In the Gaussian Barrett loop, register pressure sends m and n² to the stack, so it has 10 memory operands against 7. gcc reloads one or two inputs it already holds in the -O2 -mavx2 vector product loops. -march=native drops those reloads (30 and 20 instructions for the vector product loops) and otherwise gives the same code; it does not vectorize the scalar loops.

**Timing.** The machine was an Intel Xeon (family 6, model 173) in a 2-vCPU KVM guest, with L1d 48 KB and L2 2 MB per core; the guest reports a 480 MB L3 shared with the host. Before each run the load was below 0.6 and no other process was busy. One run at a time, pinned to one CPU. Four n: 2^31 − 1 (prime), 2,147,483,645, 2^30 + 1 (both composite) and 1,000,003 (prime). For each n there were 31 repetitions. All 14 kernels were interleaved in each repetition, the order rotated each time, and each timed call was preceded by an untimed call of the same kernel. In cache: 1,024 elements per array (24 KB for the Gaussian), 2,048 calls per sample. The ranges below are medians over the four n and three runs, in ns per element (a Gaussian element writes two residues). Quartile spreads were about ±2%, except scalar Barrett (up to +15%). No difference by n, prime or composite, was seen.

| in L1 | div | Barrett scalar | Montgomery scalar | Barrett AVX2 | Montgomery AVX2 |
|---|---|---|---|---|---|
| product | 2.60–2.62 | 0.83–1.04 (×2.5–3.2) | 0.79–0.80 (×3.3) | 0.305–0.312 (×8.5) | 0.184–0.190 (×14) |
| Gaussian product | 5.20–5.24 | 2.08–2.14 (×2.5) | 2.09–2.25 (×2.4) | 0.74–0.75 (×7.0) | 0.457–0.463 (×11.4) |
| conversion in / out | | | 0.79–0.81 / 0.62–0.65 | | 0.162–0.166 / 0.126–0.129 |

(×k is the speed-up over the divide.) -march=native gave the same figures within noise (Montgomery AVX2 product 0.176–0.178).

Large arrays: 2^22 elements, 16 MB per array, beyond L2 but within the reported L3.

| 16 MB arrays | div | Barrett AVX2 | Montgomery AVX2 |
|---|---|---|---|
| product | 2.63–2.65 | 0.49–0.60 | 0.43–0.46 |
| Gaussian product | 5.30–5.32 | 1.09–1.15 | 1.00–1.06 |

Here the vector kernels are limited by memory traffic (12 and 24 bytes per element), and Montgomery's lead shrinks to about 10%. The scalar large-array figures depended on the harness rather than the reduction, so they are not tabulated:
- Scalar Barrett took 0.82 ns per product run alone but 1.6 ns interleaved with one other kernel.
- Scalar Montgomery took 0.82 alone and 1.3–2.0 interleaved, slower than Barrett in every interleaved setup, including with the input arrays swapped.
- At 2^25 elements, run alone, both took 0.98.
- The likely cause is how much of the data the shared host L3 holds, which the guest cannot control; it was not pinned down.
- The scalar Gaussian product was 2.2–2.3 for both methods in every setup.

**Conversion costs and break-even.** In lanes, Montgomery saves about 0.12 ns per product and 0.29 per Gaussian product over Barrett. Converting a residue in costs 0.16 and out 0.13. A Gaussian product whose two inputs are converted in and whose output is converted out pays 4 × 0.16 + 2 × 0.13 ≈ 0.91 ns, about three products' saving. So in lanes Montgomery pays from about three multiplies per converted value, and in any chain of products that stays in Montgomery form. In scalar code the two methods are within a few percent either way: Montgomery is about 0.05 ns ahead on the product, and on the Gaussian they tied in two runs while Barrett was 4–7% ahead in the third. So the conversions (0.8 and 0.64 ns per residue) are never repaid, and Barrett, which also takes even n, is the better scalar choice here. Study 4's toy (R = 2^64 Montgomery) had Barrett 7% ahead on the scalar Gaussian. With R = 2^32 that gap is at most this size and not consistent.

**Summary.** The scalar methods tie at about 3 times the speed of the divide for the product and 2.5 times for the Gaussian. In AVX2 lanes, Montgomery is about 1.6 to 1.7 times faster than Barrett (14 and 11 times the divide, against 8.5 and 7). The reason is that a 31-bit Barrett quotient built from 32 × 32 → 64 multiplies is off by up to 2 or 3, which forces a 64-bit correction. Against memory traffic the vector gap mostly closes. Not tried: AVX-512 (vpmullq, 64-bit compares into masks, IFMA's 52-bit multiplies) and a floating-point quotient estimate.

## Study 20: what the shrink test costs, naive against refined (shrink20.c)

**What was measured.** Gaussian integers in layout (b) (separate real and imaginary arrays), AVX2, gcc 13.3 -O2 -mavx2, at two sizes:
- **Base size:** int32 halves in 32-bit lanes. Its shrink target would be 15 bits, but there is no smaller size, so a test here is pure cost.
- **First grown size:** int64 halves in 64-bit lanes, with a shrink target of 31 bits.

The operations are add, subtract and multiply of two arrays. Division and mod also shrink values, but are out of scope here.

Every variant keeps the full-range overflow check, ORed into a sticky register and tested once per array. A bare-arithmetic line is kept as a reference.
- **Bare:** arithmetic only, with no checks.
- **v1, no shrink test:** arithmetic plus the overflow check.
- **v2, naive:** v1, then a separate pass over the two result arrays. It runs after every operation (add, subtract and multiply) at both sizes.
- **v3, refined (decision 18):** the shrink test is fused into the operation's own sweep, only for add and subtract, and only above the base size. At the base size and for multiply, v3 is v1.

The forms used:
- **Shrink test (rule B):** a half v fits in g bits exactly when v + 2^(g−1) has nothing at bit g or above. That is an add and an OR into a sticky register per vector, with the mask applied once per array. At int64 (g = 31) this is the "top 34 bits equal" test without the 64-bit arithmetic right shift that AVX2 lacks.
- **Overflow checks:**
  - add: (x XOR r) AND NOT (x XOR y), using `vpandn`;
  - subtract: (x XOR r) AND (x XOR y);
  - both read at the sign bit.
- **Multiply at int32:** the widening 32 × 32 → 64 `vpmuldq` on the even lanes, and on the odd lanes after a shift, with the two-product sums in 64 bits. The check ORs in v + 2^31 for each 64-bit part, tested at bits 32 to 63. The low halves are packed back with a shift and a blend. The bare multiply is the wrapping `vpmulld` form.
- **Multiply at int64:** scalar, because AVX2 has no 64-bit lane multiply. It uses four 64 × 64 → 128 `imul`s and a two-word subtract and add. The check is that the high word equals the sign of the low word. The bare multiply is four wrapping `imul`s.

The copy to the smaller size is not done; the kernels return the flags. N is a multiple of 8.

**Correctness.** Every kernel was compared with __int128.
- **Edge quadruples:** every quadruple of 32 edge values at each width, 2,097,152 in all, each element alone among 8 with its position rotating. The edges were 0, ±1, and 2^k − 2 … 2^k + 1 and −2^k − 1 … −2^k + 2 within range, for k = 14, 15, 30, 31 at int32 and k = 30, 31, 62, 63 at int64. So the multiply corner (every half the most negative value) is included.
- **Random:** 400,000 arrays of 8 to 128 elements, mixing edges, random bit lengths, and all-small arrays of several widths with an occasional planted value.

Every result equals the true value mod 2^w. The overflow flag is set exactly when some true result does not fit. When it is clear, the shrink flag says "all fit" exactly when every half fits in g bits. Each outcome occurs at least 25,000 times per kernel. **Failures: none** (25 million kernel calls).

As before, when a result overflows, the shrink flag describes the wrapped value. In 4,000 to 12,000 cases per kernel, overflowed results wrapped into g bits. This is harmless only because the overflow flag is always checked first and the operation is then redone at the next size.

**Instruction counts** (inner loops from objdump, per element = one Gaussian integer, loads, stores and loop overhead included; v2 adds the pass loop, 0.875 per element at int32 and 1.75 at int64):

| per element | bare | v1 | v2 naive | v3 refined |
|---|---|---|---|---|
| add32, sub32 | 1.125 | 2.125 | 3.0 | 2.125 |
| mul32 | 1.75 | 4.625 | 5.5 | 4.625 |
| add64, sub64 | 2.25 | 4.25 | 6.0 | 5.25 |
| mul64 (scalar) | 17 | 38 | 39.75 | 38 |
| mix (mean of the six) | 4.25 | 9.23 | 10.54 (+14%) | 9.56 (+3.6%) |

- **The pass:** 2 adds and 2 ORs per two vectors, plus loop overhead, and it reads both result arrays again: 8 bytes per element at int32, 16 at int64.
- **The fused test:** the same 2 adds and 2 ORs, with no loop overhead and no loads.
- **The scalar int64 multiply:** with its check it needs more registers than x86-64 has free next to the `rdx:rax` pair of one-operand `imul`, and gcc keeps some values on the stack. That is part of its 38.

**Timing.** Same machine and harness as above: an Intel Xeon (family 6, model 173) in a 2-vCPU KVM guest, with L1d 48 KB and L2 2 MB per core and a reported 480 MB L3 shared with the host.
- The load was below 1, nothing else was busy, and the runs were pinned to one CPU.
- Each run made 31 repetitions, with the kernels of one width interleaved and their order rotated, and an untimed call before each timed one.
- Each of the six arrays starts at a different page offset (j × 576 bytes). With all at one offset, the in-cache int64 loops ran 10–12% slower and were noisier.
- Operands were small (no overflow), which does not matter, since the loops do not branch.

Three sizes:
- **L1:** 512 elements, 4,096 calls per sample.
- **L2:** 16,384 elements, 128 calls.
- **Memory:** 2^25 elements, 128 MB per array at int32 and 256 MB at int64, 0.75 and 1.5 GB in all. That is beyond the reported L3.

The figures are medians of three runs, in ns per element. The three runs agreed within 1–3%, and quartile spreads were ±1–3%.

| ns per element | bare | v1 | v2 naive (over v1) | v3 refined (over v1) |
|---|---|---|---|---|
| L1, add32 / sub32 | 0.088 / 0.089 | 0.155 / 0.155 | 0.218 / 0.216 (+40%) | = v1 |
| L1, mul32 | 0.154 | 0.344 | 0.411 (+19%) | = v1 |
| L1, add64 / sub64 | 0.156 / 0.152 | 0.307 / 0.310 | 0.416 / 0.416 (+35%) | 0.350 / 0.350 (+0.04, +13 to 14%) |
| L1, mul64 | 1.058 | 2.084 | 2.160 (+4%) | = v1 |
| L1, mix | 0.282 | 0.559 | 0.639 (+14%) | 0.573 (+2.5%) |
| L2, add32 / sub32 | 0.256 / 0.259 | 0.264 / 0.263 | 0.332 / 0.334 (+26%) | = v1 |
| L2, mul32 | 0.269 | 0.324 | 0.399 (+23%) | = v1 |
| L2, add64 / sub64 | 0.515 / 0.516 | 0.528 / 0.530 | 0.670 / 0.670 (+27%) | 0.532 / 0.524 (−1 to +1%) |
| L2, mul64 | 1.044 | 2.043 | 2.176 (+7%) | = v1 |
| L2, mix | 0.477 | 0.659 | 0.764 (+16%) | 0.659 (0%) |
| memory, add32 / sub32 | 1.253 / 1.261 | 1.325 / 1.323 | 1.701 / 1.712 (+29%) | = v1 |
| memory, mul32 | 1.323 | 1.370 | 1.755 (+28%) | = v1 |
| memory, add64 / sub64 | 2.486 / 2.482 | 2.636 / 2.621 | 3.421 / 3.426 (+30%) | 2.669 / 2.661 (+0.03 to +0.04, +1.3 to 1.5%) |
| memory, mul64 | 2.633 | 3.325 | 4.114 (+24%) | = v1 |
| memory, mix | 1.906 | 2.096 | 2.688 (+28%) | 2.104 (+0.4%) |

The mix is the mean over the six cells: equal numbers of add, subtract and multiply, half the arrays at each size. The scalar int64 multiply weighs heavily in it in cache, since it is 7 to 13 times slower than the vector operations there.

**What the figures show.**
- **In L1** the loops are bound by instructions, and the costs follow the counts. The pass adds about 0.06 ns per element at int32 and 0.1 at int64. The fused test adds 0.04 at int64, 13–14% of the add or subtract.
- **From L2 or memory** the vector operations are bound by bandwidth:
  - The fused test is free within noise (−1 to +1.5%; per run, −0.01 to +0.04 ns).
  - The naive pass costs about what re-reading the results costs. In memory that is 0.38 to 0.39 ns per element at int32 (8 bytes) and 0.79 to 0.81 at int64 (16 bytes), a quarter to a third of the operation. In L2 it is 0.07 and 0.14 (+23 to 27%).
- **For multiply,** the pass is the only shrink cost. It is small next to the scalar int64 multiply in cache (+4 to 7%) and 24–28% from memory.
- **The overflow checks (v1 over bare)** cost 75–125% in L1, where they double the instructions, but only 2–6% from L2 or memory for add and subtract. For the int32 multiply they cost 20% in L2 and 4% from memory. The scalar int64 multiply costs about 2 times in cache and 26% from memory.

**Over the mix:**
- **The naive test** costs 14% in L1, 16% in L2 and 28% from memory.
- **The refined test** costs 2.5% in L1 and nothing measurable in L2 or memory (0.0 and +0.4%). Three things make it cheap: it skips the base size and multiply, which here are four of the six cells, and when it does run it reads no memory of its own.

The earlier finding that the fused test costs about 5–7% from memory over a bare loop was a comparison with a bare loop of 9 instructions. Adding anything to that loop cost the same, even four `nop`s. With the overflow checks always present, as here, that excess is in v1, and v3 adds only 1.3–1.5% to it.

**Summary.** When the loop waits on memory, the refined shrink test is close to free: 0 to 1.5% on an add or subtract at int64, and nothing elsewhere, by construction. A separate pass is not free, because it re-reads the result arrays. It costs 24–31% of every operation from memory, and 28% over the mix. In L1 the fused test costs its instructions: +0.04 ns, or 13–14% of an int64 add or subtract, and 2.5% over the mix. The shrink flag is meaningful only when the overflow flag is clear, so the two checks belong together.

## Study 21: what the shrink test saves over a whole run (shrink21.c)

**What was measured.** Whole runs on an array of Gaussian integers in layout (b), with AVX2. Overflow checks and the sticky flag are always on, and the size changes for real:
- **The state:** starts at int32 halves. The operands are int32 arrays.
- **Promotion:** on overflow at int32 the state is copied up to int64 halves and the operation is redone there from the untouched input. At int64 the operand is sign-extended as it is loaded (`vpmovsxdq`), so operands are never copied up.
- **Demotion:** when the shrink flag allows, the int64 state is copied down to int32 (two `vpermd` and a lane permute per 8 halves).
- **Kernels:** those of study 20. The int64 multiply is scalar, since AVX2 has no 64-bit lane multiply.

Three variants of the shrink policy:
- **v1, no shrink test:** once grown, the state stays at int64.
- **v2, naive:** a separate pass over the result after every operation, at both sizes, and demotion when it allows.
- **v3, refined:** the test is fused into add and subtract at int64 only, and demotion happens at the end of the sweep when it allows.
- **v3A:** v3 with rule A (fits in 32 bits, no margin). It is used in the hover workload only.

Workloads, each of 2 + L operations, with L = 0, 1, 2, 4, …, 128:
- **back:**
  1. add P, where P is just under 2^31, so the sums overflow int32 and the state is promoted;
  2. subtract P, which brings every value back under the rule B line (|v| < 2^28);
  3. L operations cycling add S, subtract T, multiply U. S and T are below 2^10 and U is a unit (±1, ±i), so nothing overflows again. The first multiply is the third of the L.
- **control:** add P, then subtract S, so the values stay above the line and no variant ever demotes, then the same L operations. This shows the pure cost.
- **hover:** values just under 2^31, then add D and subtract D alternately (D ≈ 2^11). Every add overflows int32, and every subtract returns the values to between 2^31 − 2^11 and 2^31 − 2^10. Those fit in 32 bits but not in 31.

**Correctness.** For every workload, L and variant, the final array was compared with an exact __int128 replay of the operations: every element at N = 1,024, and every 16th at N = 2^20. **Failures: none.** The size changes were as intended:
- **back:** v2 and v3 promote once and demote once; v1 promotes once and ends at int64.
- **control:** one promotion, no demotion, in every variant.
- **hover:** v1, v2 and v3 promote once and stay at int64 (rule B). v3A promotes and demotes once per add–subtract pair, 65 times each at L = 128.

**Timing.** Same machine (Intel Xeon family 6 model 173, 2-vCPU KVM guest) under the same conditions: load below 1, pinned to one CPU, variants interleaved with the order rotated.
- **In cache:** N = 1,024 (8 KB per int64 array, about 100 KB in all), 31 repetitions of about 0.3 ms each.
- **Memory:** N = 2^23 (64 MB per int64 array, about 0.7 GB in all), 7 repetitions.

The figures are the medians of two full runs, in ns per element for the whole run.
- **Spreads:** quartiles were ±1–3% in cache and ±1–4% in memory.
- **Between runs:** the in-cache figures agreed within 2%. Memory figures were 5–10% lower in the second run for every variant alike, so the savings agree within about 10%.

**back** (ns per element per run; saved = v1 − v):

| L | in cache v1 / v2 / v3 | saved v2 / v3 | memory v1 / v2 / v3 | saved v2 / v3 |
|---|---|---|---|---|
| 0 | 0.96 / 1.27 / 1.22 | −0.31 / −0.25 | 6.4 / 9.0 / 7.5 | −2.5 / −1.1 |
| 1 | 1.36 / 1.73 / 1.64 | −0.37 / −0.29 | 8.5 / 10.8 / 8.9 | −2.3 / −0.4 |
| 2 | 1.64 / 1.94 / 1.78 | −0.30 / −0.14 | 11.0 / 12.7 / 10.7 | −1.7 / +0.3 |
| 4 | 4.03 / 2.55 / 2.26 | +1.5 / +1.8 | 15.8 / 16.0 / 13.0 | −0.2 / +2.9 |
| 8 | 6.84 / 3.61 / 3.07 | +3.2 / +3.8 | 25.1 / 21.7 / 17.3 | +3.4 / +7.7 |
| 16 | 14.7 / 5.8 / 4.9 | +8.9 / +9.8 | 43.8 / 34.2 / 27.4 | +9.5 / +16.4 |
| 32 | 28.3 / 10.0 / 8.1 | +18.3 / +20.2 | 79.9 / 56.2 / 44.0 | +23.8 / +36.0 |
| 64 | 56.5 / 18.7 / 15.0 | +37.9 / +41.6 | 152 / 105 / 81 | +47 / +71 |
| 128 | 112 / 35.9 / 28.4 | +76 / +84 | 283 / 178 / 130 | +105 / +153 |

**Break-even.**
- **In cache:** both v2 and v3 pay off between L = 2 and L = 4. Below that, they lose 0.15–0.37 ns per element: the copy down, plus v2's passes.
- **Memory:**
  - v3 breaks even at L = 2. At L = 1 it is 0.3–0.4 ns behind, and at L = 2 it is 0.1–0.4 ahead.
  - v2 breaks even at L = 4, where it is level within noise, and is clearly ahead from L = 8.

Past break-even, each further operation saves about 0.65 ns in cache and 1.2 ns from memory with v3 (0.6 and 0.8 with v2). At L = 128, v3 takes a quarter of v1's time in cache and under half of it from memory.

The saving has two sources. At int32, add and subtract move half the bytes. And the multiply stays a vector operation (study 20: 0.34 ns in L1, against 2.1 for the scalar int64 multiply), which is why the in-cache saving jumps once the first multiply arrives (L = 3). With add and subtract only, the per-operation saving would be the int64 minus int32 cost: about 0.15 ns in L1 and 1.3 ns from memory (study 20).

**control** (no demotion ever, so pure cost):
- **In cache:** v3 adds 0.15–0.3 ns per element at small L, and 6–7% of the run at L = 64 to 128. v2 adds 8–13%.
- **Memory:** v3 is level with v1 at every L (−1.1 to +3.4 ns on runs of 6 to 300 ns, all within the spread). v2 adds 22–36% (1.4 to 102 ns).

**hover** (L = 128; ns per element):

| | v1 | v2 (B) | v3 (B) | v3A (rule A) |
|---|---|---|---|---|
| in cache | 38.7 | 52.4 (+35%) | 49.7 (+28%) | 84.4 (2.2 times) |
| memory | 248 | 336 (+35%) | 239 (−4%) | 469 (1.9 times) |

- **Rule B:** never demotes here, so v3's cost is only its fused test. That is nothing measurable from memory, and +28% in cache, where this workload is pure int64 add and subtract (about 0.085 ns per operation, twice study 20's L1 figure; not pinned down).
- **Rule A:** demotes after every subtract and promotes again on every add. That is a copy down, a copy up and a redo per pair, about 0.27 ns per operation in cache and 1.8 ns from memory, which makes v3A about twice as slow as never shrinking. Rule B's margin removes all of it.

**Summary.** Once values come back under the line, the refined test pays for itself after about 2 further operations from memory and 3 in cache (the first multiply). From then on it saves about 1.2 ns per operation from memory and 0.65 ns in cache, so a long run takes a quarter (in cache) to a half (memory) of the time it takes when it never shrinks. When values never come back down, it costs nothing measurable from memory and 6–7% in cache (up to 28% in an add-and-subtract-only loop). The naive pass breaks even later (L = 4 to 8 from memory), saves less, and costs 22–36% from memory when shrinking never happens. Rule A loses about half the speed when values hover near the line, and rule B none. Division and mod were not measured.

## Study 22: powers of a Gaussian matrix, residue against direct (matpow.py)

**What was compared.** A^e for a square n × n matrix with b-bit halves (full range), counts only, with study 18's cost model.

- **Entry sizes, chosen in advance.** A's entries have |z| ≤ 2^(b − ½), and entries of A^(i+j) are at most n·|A^i|·|A^j|. So log₂|A^j| ≤ j(b − ½) + (j − 1)log₂ n, and a half of A^j needs x_j = ⌊that⌋ + 2 bits.
- **Direct** (separate re/im arrays), by repeated multiplication (A^j = A^(j−1)·A) and by repeated squaring. Per term with operand halves of x and y bits:
  - both ≤ 32 bits: study 18's vector forms (1, 2, 8.5);
  - otherwise scalar multi-word, calibrated to study 18's 64-bit term (32). With word counts wx, wy for the operands, wp for the exact product and wR for the accumulator: four real products of 2·wx·wy + 4(wx·wy − 1), two combines of 2·wp, and two accumulates of 2·wR + 2. This grows with the square of the width.
- **Residue.** m primes sized for A^e (31-bit primes, or the 27-bit variant). Convert in once, do all products in residue form by repeated squaring, convert out once. Products of Montgomery-form values, summed lazily and reduced by REDC, stay in Montgomery form, so the chain needs no corrections between products. The 31-bit primes reduce every 2 terms; the 27-bit primes reduce every min(n, 32) terms (once per entry here).
- **Verified against exact big-integer powers:** n = 2, 3 and 4 with e = 2, 3, 4 (and 8, 16 for n ≤ 3), and n = 8 and 16 with e = 2 and 4, at b = 8, 16 and 32. Half of the matrices were the extreme one, every entry (−2^(b−1), −2^(b−1)). Every entry of A^e fits the advance bound (the direct widths), and both residue variants give A^e exactly. 51 powers, **no failures**. (e = 3 exercises the residue form's repeated-multiplication path.)

**Totals** (instructions): best direct (the cheaper of repeated multiplication and squaring) / best residue (the cheaper of 31-bit and 27-bit primes). Squaring is always the cheaper direct method from e = 4 on; repeated multiplication costs up to about 9 times as much at e = 64.

| b = 8 | e = 2 | 4 | 8 | 16 | 32 | 64 |
|---|---|---|---|---|---|---|
| n = 2 | 16 / 142 | 84 / 374 | 308 / 786 | 1,172 / 1,788 | 3,124 / 4,542 | 8,404 / 14,054 |
| 3 | 54 / 338 | 284 / 914 | 1,040 / 1,930 | 3,956 / 4,968 | 10,544 / 14,283 | 28,580 / 39,902 |
| 4 | 128 / 632 | 672 / 1,752 | 2,464 / 3,720 | 9,376 / 10,512 | 24,992 / 29,592 | 67,744 / 86,792 |
| 8 | 1,024 / 3,040 | 5,376 / 9,056 | 19,712 / 27,552 | 75,008 / **67,456** | 204,032 / **176,608** | 685,312 / **500,608** |
| 16 | 8,192 / 16,256 | 43,008 / 52,608 | 157,696 / 159,360 | 600,064 / **384,512** | 1,632,256 / **1,070,208** | 5,515,264 / **3,070,080** |

| b = 16 | e = 2 | 4 | 8 | 16 | 32 | 64 |
|---|---|---|---|---|---|---|
| n = 2 | 68 / 290 | 292 / 660 | 1,156 / 1,578 | 3,108 / 4,164 | 8,388 / 13,298 | 24,932 / 42,748 |
| 3 | 230 / 688 | 986 / 1,593 | 3,902 / 4,259 | 10,490 / 11,070 | 28,310 / 34,173 | 84,362 / 111,226 |
| 4 | 544 / 1,288 | 2,336 / 3,024 | 9,248 / **8,832** | 24,864 / 25,472 | 67,104 / 74,128 | 199,968 / 225,616 |
| 8 | 4,352 / 6,176 | 18,688 / **15,168** | 73,984 / **46,496** | 198,912 / **124,416** | 540,928 / **381,216** | 1,841,408 / **1,224,992** |
| 16 | 34,816 / **32,896** | 149,504 / **85,248** | 591,872 / **259,712** | 1,591,296 / **758,912** | 4,327,424 / **2,235,008** | 14,764,032 / **6,620,416** |

| b = 32 | e = 2 | 4 | 8 | 16 | 32 | 64 |
|---|---|---|---|---|---|---|
| n = 2 | 224 / 534 | 1,088 / 1,368 | 3,040 / 3,786 | 8,320 / 11,412 | 24,864 / 39,242 | 82,368 / 140,714 |
| 3 | 756 / 1,256 | 3,672 / **3,258** | 10,260 / **9,794** | 28,080 / 31,622 | 83,916 / 101,144 | 278,208 / 344,822 |
| 4 | 1,792 / 2,328 | 8,704 / **6,112** | 24,320 / **19,680** | 66,560 / **62,264** | 198,912 / **194,512** | 659,456 / 664,312 |
| 8 | 14,336 / **10,848** | 69,632 / **37,856** | 194,560 / **108,576** | 532,480 / **325,056** | 1,595,392 / **1,031,776** | 5,713,920 / **3,372,832** |
| 16 | 114,688 / **55,680** | 557,056 / **200,576** | 1,556,480 / **569,472** | 4,259,840 / **1,644,288** | 12,763,136 / **5,350,272** | 45,744,128 / **17,288,192** |

Bold marks the residue form winning. Primes needed at e = 64: about 18 to 28 for b = 8, 35 to 47 for b = 16, and 68 to 85 for b = 32 (31-bit and 27-bit respectively).

**Where the residue form wins** (e in {2, 4, …, 64}):

| | n = 2 | n = 3 | n = 4 | n = 8 | n = 16 |
|---|---|---|---|---|---|
| b = 8 | never | never | never | from e = 16 (27-bit primes; 31-bit only at e = 64) | from e = 16 |
| b = 16 | never | never | only e = 8, and narrowly | from e = 4 (27-bit; 31-bit from e = 8) | from e = 2 (27-bit; 31-bit from e = 8) |
| b = 32 | never | e = 4 and 8 only | e = 4 to 32, narrowly | from e = 2 (27-bit; 31-bit from e = 4) | from e = 2 |

**Why the small cases do not keep their win.** At large e both methods are dominated by costs quadratic in the final width W:
- **direct by squaring:** about n³ term costs per squaring at the widest size, about 6·(W/64)² each;
- **residue:** the conversion out, O(m²) per entry with m ≈ W/31, about n²·27·(W/64)² in all.

The residue's products grow only linearly in m. So for large e the ratio direct/residue tends to about n/3. The residue form wins at large powers only from n ≈ 4 on: never for n = 2 and 3, roughly breaking even at n = 4, and clearly winning at n = 8 and 16. At small e and wider inputs (b = 32), the residue form wins briefly even at n = 3 and 4, before the direct method has had to go multi-word.

**Caveats.**
- Both sides use quadratic methods. At the widths reached here (up to about 2,300 bits at b = 32, e = 64), a real direct multi-word multiply would use Karatsuba or better, and the residue conversion out could use subquadratic CRT (product trees). Neither was modelled.
- Only powers of 2 were counted. Other e need one extra product per set bit, in both methods.
- The direct model charges 33- to 64-bit operands as scalar words even where one operand is narrow (A in repeated multiplication), which may overcharge the direct method a little at mid widths. Repeated multiplication never beat squaring, so this does not move the winners.

## Study 23: the int64-half multiply in AVX2 lanes (mul64.c)

**What was compared.** The Gaussian multiply at the first grown size: int64 halves in separate re and im arrays (layout (b)), re = ac − bd, im = ad + bc. AVX2 has no 64-bit lane multiply, only the widening 32 × 32 → 64 `vpmuludq` (unsigned) and `vpmuldq` (signed), which read the low dword of each 64-bit lane. Each kernel writes the low 64 bits of re and im and, where checked, returns the full-range sticky flag: set iff some true re or im does not fit int64.

- **s_bare, s_chk:** study 20's scalar kernels (four wrapping `imul`; four 64 × 64 → 128 `imul` with "high word = sign of low word").
- **v_bare:** wrapping only. Per real product lo(a)lo(c) + ((hi(a)lo(c) + lo(a)hi(c)) << 32), so 12 multiplies per register of 4 elements.
- **v_uns:** unsigned 32-bit pieces, 16 `vpmuludq` per register, with sign corrections.
- **v_sgn:** signed 32-bit pieces, 16 `vpmuldq` per register, with corrections only at one edge.
- **v_bnd:** v_bare plus a per-array magnitude bound, and v_sgn as an exact fallback (below).

**The overflow test in lanes.** Let k = ⌊(re + 2^63)/2^64⌋. The result fits iff k = 0. Since |re| ≤ 2^127, |k| ≤ 2^63, so k may be computed mod 2^64 in wrapping lanes: k ≡ 0 iff k = 0. So only the 2^32 column needs exact arithmetic (its carry into k); the 2^64 column (the high-by-high products and every correction) just wraps. Splitting the 2^32 column needs no 64-bit arithmetic shift (AVX2 has none): a value m in int64 is split as hi = (m XOR 2^63) >> 32, which is ⌊m/2^32⌋ + 2^31, and lo = its low dword (one `vpblendd` with zero).

- **Signed pieces (v_sgn).** x = xh·2^32 + xl, with xl the low dword read as signed and xh = (x + 2^31) >> 32. This covers [−2^63 − 2^31, 2^63 − 2^31). A value x ≥ 2^63 − 2^31 (the edge) is represented as x − 2^64, which changes nothing mod 2^64 and adds e_a·c + e_c·a to k for a product ac. That is one `vpcmpgtq` per input and two masked adds (AND, ADD) per product. Products of signed pieces lie in [−2^62 + 2^31, 2^62], so every column of re is a difference of two products inside int64, split once. The columns of im are sums in [−2^63 + 2^32, 2^63], split with the offset 2^63 − 2^32 instead of 2^63 (and 2^63 − 2^32 + 1 on one column, which absorbs a +1 the carry would otherwise need).
- **Unsigned pieces (v_uns).** Both pieces unsigned, so x is read as x + 2^64 when negative, and k gets −(s_a·c + s_c·a) per product (s = sign masks), for every negative input rather than only at an edge. The unsigned products lie in [0, 2^64) and do not pair into int64 differences, so each of the four middle products is split on its own, and the low products need an unsigned 64-bit compare (XOR both with 2^63, `vpcmpgtq`) for their borrow or carry.
- **Bound with fallback (v_bnd).** v_bare, plus x XOR (x < 0 mask) = |x| − [x < 0], ORed over each operand array. If every entry of the first operand is below 2^p in that form and every entry of the second below 2^q, then |re|, |im| ≤ 2^(p+q+1), which fits when p + q ≤ 61. Once per array, after the sweep: if the bound holds, the flag is 0 (exactly); if not, the array is redone by v_sgn, which writes the same results and gives the exact flag. One branch per array, like the sticky flag's own.
- **Karatsuba and Gauss's three multiplies do not fit.** Both need sums of pieces (ah + al, or a + b), which take 33 bits, and the widening multiply reads 32. Smaller pieces (three of about 22 bits) would need 6 multiplies per real product by Karatsuba against 4 by schoolbook with two pieces. So 16 multiplies per register of 4 elements (4 per element) is the floor with 32-bit multipliers, against 4 one-operand `imul` in scalar code.

**Correctness.** Every kernel was compared with __int128 (results mod 2^64, and the flag).
- **Edge quadruples:** every quadruple of 37 edge values, 1,874,161 in all, each element alone in a register of 4 (so the flag is per element), with its position rotating. The edges: 0, ±1, ±2^k − 2 … ±2^k + 1 for k = 31, 32, 62, within range for k = 63 (so −2^63, −2^63 ± 1, 2^63 − 1, 2^63 − 2), and v_sgn's edge line ±(2^63 − 2^31) ± 1.
- **The corner:** all four halves −2^63 in every lane (im = 2^127, the one value that also overflows int128 and wraps; s_chk still flags it).
- **Random, alone:** 4,000,000 elements mixing edges, edges ± 4 and random bit lengths. Half were aimed at the boundary: b small (|b| ≤ 8) and d (or c) chosen so that re (or im) lands within 3 of ±2^63. 67,471 elements had re or im within 4 of the boundary and fit, 644,601 within 4 and did not.
- **Random arrays:** 200,000 of 4 to 128 elements, a third general, a third all small (20 to 30 bits, all fit), a third small with one general value planted.
- **Failures: none** (36 million kernel calls). Each checked kernel saw the flag clear 474,196 times and set 5,599,966 times. v_bnd was settled by its bound in 304,985 calls and fell back otherwise.
- **The tests catch faults.** Dropping either edge correction in v_sgn, one sign correction or the borrow in v_uns, using the wrong offset on one im column, or the wrong constant in the k test each gave between 8,000 and 470,000 failures.

**Instruction counts** (inner loop from objdump, gcc 13.3 -O2 -mavx2, loads, stores and loop overhead included; a vector loop does 4 elements per pass):

| per element | s_bare | s_chk | v_bare | v_uns | v_sgn | v_bnd (bound holds) |
|---|---|---|---|---|---|---|
| instructions | 17 | 38 | 9.25 | 32.5 | 27.5 | 13 |
| multiplies | 4 (`imul`) | 4 (`imul`, 128-bit) | 3 | 4 | 4 | 3 |
| spill moves | 0 | 1.5 | 0 | 5 | 2.75 | 0.75 |

v_sgn as written is 99 per register: loads and stores 6, pieces 8, 16 multiplies, 8 to form the four columns of each part, 3 per part for the stored low word, 15 per part for the splits and the carry into k, 20 for the edge corrections, 2 ORs and 3 of loop. AVX2's 16 registers do not hold it, and gcc adds 11 spill moves. The check is the larger share: v_bare's 37 per register becomes 110.

**Timing.** Same machine and harness as studies 19 to 21 (Intel Xeon family 6 model 173, 2-vCPU KVM guest). The load was below 1 with nothing else running, pinned to one CPU, 31 repetitions with the six kernels interleaved and their order rotated, an untimed call before each timed one, and each array at its own page offset. Operands were below 2^30 (the loops do not branch, so values do not matter, except for v_bnd's choice of path). The clock ran at about 3.8 GHz (measured with an `imul` latency chain). Sizes: L1 512 elements (4 KB per array), L2 16,384 (128 KB), memory 2^25 (256 MB per array, 1.5 GB in all). The figures are the range of the medians over three runs, in ns per element. Quartile spreads were ±1–2%. An earlier set of three runs (without v_bnd) agreed within 2%, except s_chk from memory, which was 4% higher there (3.43–3.48).

| ns per element | s_bare | s_chk | v_bare | v_uns | v_sgn | v_bnd (bound holds) |
|---|---|---|---|---|---|---|
| L1 | 1.04–1.05 | 2.09–2.12 | 0.65–0.66 | 2.27–2.30 | 2.01–2.05 | 0.88–0.91 |
| L2 | 1.04–1.05 | 2.07–2.08 | 0.65–0.66 | 2.22–2.24 | 2.00–2.03 | 0.87–0.89 |
| memory | 2.64–2.65 | 3.28–3.34 | 2.71–2.72 | 3.35–3.39 | 3.20–3.24 | 2.73–2.76 |

When v_bnd's bound fails, it costs its own sweep plus v_sgn: about 2.9 ns in cache and 6 ns from memory.

**What the figures show.**
- **Bare, the vector multiply wins in cache:** 0.65 against 1.04 ns (1.6 times), with 3 multiplies per element against 4. From memory both are at the bandwidth limit (48 bytes per element) and tie (v_bare 3% slower).
- **With the exact check, the vector multiply only ties.** v_sgn takes 2.01–2.05 ns against the scalar 2.09–2.12 in cache (2–4% faster) and 3.20–3.24 against 3.28–3.34 from memory (2–3% faster). It runs 1.4 times fewer instructions per element (27.5 against 38), but the vector loop is bound by AVX2's three vector ALU ports (about 7.7 cycles per element, 3.6 instructions per cycle), while the scalar loop retires about 4.75 per cycle over more ports.
- **Unsigned pieces lose to signed ones** by 10–13% in cache and 4–5% from memory: they correct for every negative input rather than one edge, split twice as many middle products, and spill more.
- **The bound with fallback is the fast path when it holds:** 0.88–0.91 ns in cache, 2.3 to 2.4 times faster than v_sgn and the scalar check, and level with the bare loops from memory (the bound is 4 instructions per element and reads no extra memory). Its bound is per array, not per element: it holds whenever every first operand has at most p bits and every second at most q with p + q ≤ 61, for example everything below 2^30. It fails when the arrays hold values near 2^31 and above on both sides, which is common just after a promotion from int32 (a product of two int32 halves already has 63 bits), and then it costs 44% more than v_sgn alone in cache and 85% more from memory.
- **The check, not the multiply, is the cost:** in v_sgn the multiplies are 16 of 110 instructions. The 2^32 column needs an exact carry, and the edge (or sign) corrections are 20 instructions per register.

**What AVX-512 would change** (not built; this CPU reports AVX-512F, DQ, VL and IFMA).
- **`vpmullq`** (DQ) gives the low 64 bits of a 64 × 64 product in one instruction (3 µops on Intel). The bare multiply becomes 4 `vpmullq` and 2 add/sub per register of 8 elements. It gives no high half, so the exact check still needs the 32-bit columns or IFMA.
- **The check gets cheaper in several small ways:** 64-bit arithmetic shift (`vpsraq`) splits a column in 1 instruction instead of 2; unsigned 64-bit compares (`vpcmpuq`) into mask registers give a borrow in 1 instead of 3; masked adds apply an edge correction in 1 instead of 2; mask registers hold the sticky; and 32 vector registers remove the spills. A rough recount of v_sgn under these rules is about 85 per 8-lane register, about 11 per element against 27.5.
- **IFMA** (`vpmadd52luq`/`vpmadd52huq`) multiplies 52-bit pieces and adds the low or high 52 bits of the 104-bit product into a 64-bit accumulator. With 52-bit limbs each lane has 12 bits of headroom, so columns can be summed before any carry is propagated, which removes most of the split-and-carry work. A 64-bit half needs two limbs (52 + 12), so the multiply count is like the 32-bit schoolbook's; signedness needs the same kind of correction as v_uns or an offset. Not counted further.
- **Wider registers:** 8 lanes per instruction. On Intel server cores 512-bit operations issue on two ports rather than three, so the gain per instruction is nearer 1.3 times than 2.

**Summary.** In AVX2, the int64-half multiply with an exact overflow check is worth about as much as the scalar one: 2.0 against 2.1 ns per element in cache, 3.2 against 3.3 from memory, at 27.5 against 38 instructions. Signed 32-bit pieces are the better split: one edge correction instead of a correction per negative input, and every re column fits int64 as a difference of two products. Karatsuba does not fit 32-bit multipliers. The bare vector multiply is 1.6 times faster than scalar, and a per-array bound (p + q ≤ 61 bits) keeps the checked multiply near that speed (0.9 ns) whenever it holds, falling back to the exact check otherwise. AVX-512 would remove most of the check's cost (shifts, unsigned compares, masked adds, more registers) and add `vpmullq` for the low word; IFMA's headroom would remove most of the carry work.

## Study 24: the Gaussian gcd in vector lanes (gcdvec.c)

**What was compared.** Arrays of independent pairs (a, b), int32 halves in four separate arrays, halves in study 6's symmetric range |x| ≤ 2^31 − 1. Output: gcd(a, b) normalised to the associate with re > 0 and im ≥ 0 (gcd(0, 0) = 0). Gcd only, no coefficients. gcc 13.3 -O2 -mavx2.

- **euclid:** study 6, Euclid by rounded division, scalar in 64-bit registers (two `idiv` per step).
- **pi_sc, pi_sb:** the (1+i)-ary gcd of studies 9 and 11 with the best variant (size |x| + |y|, stop when equal), scalar with `tzcnt`. pi_sc is written plainly, and gcc compiles its choices as branches. pi_sb is the same step written with masks, so that only the loop branches.
- **pi_v:** the same in AVX2 lanes, 8 pairs per register, looping until every lane of the register is done, tested every step.
- **pi_v4:** pi_v tested every 4 steps.
- **pi_v2, pi_v3, pi_v4x:** 2, 3 or 4 registers (16, 24, 32 pairs) stepped together, until every lane of all of them is done.
- **pi_vT:** a fixed step count with no test, T = 276, the proven bound (below).

**The step in 32-bit lanes.** a and b are primary (x odd, y even, x − y ≡ 1 mod 4), so a − b can need 33 bits. But a − b is divisible by (1 + i)^3, so by 2 in each part, and h = (a − b)/2 = ((ax >> 1) − (bx >> 1), (ay >> 1) − (by >> 1)) exactly, inside 32 bits. Then:
- **Strip:** shift both parts right by e = ctz(hx OR hy). AVX2 has no lane ctz, so e comes from the float exponent of the lowest set bit (AND with the negation, `vcvtdq2ps`, shift by 23, subtract 127), and the shift is `vpsravd`. Then one division by 1 + i where both parts are odd, as ((x >> 1) + (y >> 1) + 1, (y >> 1) − (x >> 1)), which cannot overflow.
- **Primary:** a blend for the multiply by −i, then `vpsignd` for the conditional negation.
- **Order:** |x| + |y| is at most 2^32 − 2, so it is compared unsigned (`vpminud`, then compare-equal). Only b's size is kept: the new b size is the minimum.
- **Finished lanes need no hold mask.** When a = b, h = 0, and the zero is replaced by b (a compare and two blends), so (a, b) stays (b, b): a fixed point. That is also why testing every 4 steps, or never (pi_vT), is safe.
- **Once per register:** the valuations and the minimum k, stripping and making a and b primary, the zero cases (a = 0 becomes b, and the reverse), the first ordering, and at the end the factor (1 + i)^k (`vpsllvd` by ⌊k/2⌋ and a blend for one more 1 + i), then the normalisation (study 6's).
- **Range.** Every value stays within ±(2^31 − 1), and |x| + |y| within 2^32 − 2. This was checked in a scalar model that runs the same algorithm in 64-bit integers and flags any lane value outside the K-bit range (below). The range argument: each coordinate of h, of the 1 + i quotient and of the primary associate is at most the largest input coordinate. Inputs with a −2^31 half, which decision 12 allows, are not covered. Making such a value primary can negate −2^31, which overflows. They need a pre-step or 64-bit lanes.
- **Proven step bound for pi_vT.** After stripping, N(a)·N(b) < 2^126, and each step that does not yet end in a = b multiplies it by at most (1 + √2)²/8 ≈ 0.7286 (study 11 (d)). The product of two nonzero norms is at least 1, so there are at most ⌊126/log₂(1/0.7286)⌋ = 275 steps before a = b. T = 276.

**Correctness.** Every kernel was compared with euclid, which also checks euclid against the (1+i)-ary model.
- **Exhaustive at small sizes:** every pair with halves in K-bit symmetric ranges, for K = 3 (2,401 pairs), 4 (50,625), 5 (923,521) and 6 (15,752,961). At K ≤ 4, each gcd was also checked to divide both inputs and to be normalised, and by brute force: N(g) equals the largest norm of any common divisor. In every exhaustive run the model stayed inside the K-bit range: no coordinate beyond 2^(K−1) − 1, no |x| + |y| beyond 2^K − 2. So the 32-bit lanes cannot overflow at K = 32, given the range argument above.
- **31 bits:** every quadruple of 20 edge values (0, ±1, ±2, 3, ±(2^31 − 1), ±(2^31 − 2), ±2^30, ±(2^30 ± 1), ±2^16, ±46,341), 160,000 pairs. Then 2,621,440 random pairs, a quarter each of four kinds: random; b small (parts up to 2^12); a common factor with parts up to 2^e, e from 4 to 15 (study 6's mix); and a large common factor (g and the cofactors with parts up to 2^15). Each gcd was checked to divide both inputs and to be normalised.
- **Failures: none.** The model's lane-range checks also found nothing at 31 bits.
- **Other checks.** The instrumented vector kernel's register-step count equals the model's exactly. Removing the zero-to-b replacement gives 57.8 million failures, and T = 40 instead of 276 gives 264 failures. Breaking the 1 + i division or the ctz bias makes some loops never end.

**Steps** (1,048,576 pairs per kind at 31 bits). "Register" is the mean over registers of the largest step count among its 8 lanes, which is what a register pays. Lane use is mean / register.

| inputs | (1+i)-ary mean / max | Euclid mean / max | register mean / max | lane use | per register with 2 / 3 / 4 registers together |
|---|---|---|---|---|---|
| random | 30.32 / 46 | 20.39 / 29 | 34.36 / 46 | 88% | 35.36 / 35.89 / 36.26 |
| b small (2^12) | 20.76 / 31 | 8.36 / 13 | 23.78 / 31 | 87% | 24.52 / 24.91 / 25.19 |
| common factor 2^4 to 2^15 | 19.78 / 38 | 13.74 / 24 | 25.77 / 38 | 77% | 27.07 / 27.74 / 28.17 |
| large common factor (2^15) | 14.27 / 25 | 10.25 / 15 | 17.15 / 25 | 83% | 17.88 / 18.27 / 18.53 |

Studies 9 and 11 gave 22.65 and 14.05 at K = 32 for a mix of these kinds; this table separates them. Uniformly random pairs take about 30 steps.

**Instruction counts** (objdump, inner loops):

| | per step | once per register (8 pairs) | per gcd, random inputs |
|---|---|---|---|
| euclid | 62 (10 `imul`, 2 `idiv`) | | ≈ 1,300 |
| pi_sb | 57 | | ≈ 1,800 |
| pi_v | 59 per register (8 of them register moves), 7.4 per lane | setup 99, finish 31 | ≈ 270 |
| pi_v4 | 48.5 per register (194 per 4 steps) | the same | ≈ 235 |
| pi_v2 / pi_v3 / pi_v4x | 60.5 / 63.3 / 63.8 per register (12 / 33 / 56 stack operands per pass: AVX2's 16 registers do not hold 3 or 4 states) | the same | ≈ 285 to 300 |
| pi_vT | 54 per register | the same | 276 × 54 / 8 + 16 ≈ 1,880 |

**Timing.** Same machine and conditions as study 23 (load below 1, nothing else running, pinned to one CPU, 31 repetitions, kernels interleaved and their order rotated, an untimed call first; clock about 3.8 GHz). 1,024 pairs (24 KB, in L1). Ranges of the medians over three runs, ns per gcd pair. Quartile spreads were ±1–2%.

| ns per gcd | random | large common factor (2^15) |
|---|---|---|
| euclid | 274.8–277.0 | 118.9–119.6 |
| pi_sc (branches) | 494.8–502.9 | 250.2–250.9 |
| pi_sb (branch-free step) | 333.9–335.6 | 171.6–172.1 |
| pi_v | 57.3–57.7 | 29.0–29.0 |
| pi_v4 (test every 4 steps) | 57.4–57.6 | 30.6–31.0 |
| pi_vT (fixed 276 steps) | 420.4–423.3 | 420.5–424.0 |
| pi_v2 | 41.2–41.3 | 21.7–21.9 |
| pi_v3 | 38.1–38.6 | 20.0–20.1 |
| pi_v4x | 38.4–38.7 | 20.3–20.4 |

**What the figures show.**
- **Lanes against Euclid:** pi_v3 takes 38 ns per gcd against Euclid's 276 on random pairs (7.2 times faster), and 20 against 119 with a large common factor (6.0 times). Even one register (pi_v) is 4.8 and 4.1 times faster. Euclid pays about 51 cycles per step, mostly two dependent 64-bit divides.
- **One register is latency-bound.** The step is one long dependency chain (shifts, the float conversion, variable shifts, 10 blends). pi_v runs at about 51 cycles per register-step and 1.2 instructions per cycle. Stepping two registers together gives 1.4 times, three gives 1.5 times (about 32 cycles per register-step). Four gains nothing more: its 4 states spill, and the larger group waits on its slowest lane (36.26 against 34.36 steps per register).
- **Finished lanes:** a register pays for its slowest lane, so 12–23% of lane-steps are idle (lane use 77–88%), more when the inputs are mixed. The fixed point makes idle lanes free of masks, but not free of time.
- **Testing less often does not help.** The test is 5 of 59 instructions, and testing every 4 steps runs up to 3 extra steps. They tie on random inputs, and pi_v4 is 6% slower on the shorter common-factor runs.
- **A fixed step count is not better (c).** The proven bound is 276 steps against about 34 per register in practice, so pi_vT takes 420 ns whatever the input: 7.3 times pi_v and 1.5 times slower than Euclid. A fixed count only pays where the data cannot be allowed to steer timing.
- **In scalar code Euclid stays ahead,** as studies 9 and 11 predicted from the counts. The (1+i)-ary gcd is 1.2 to 1.5 times slower than Euclid with the branch-free step (pi_sb), and 1.8 to 2.1 times slower with branches (mispredictions). Its advantage is only that its step has no divide, so it can go into lanes, and there it wins clearly.

**Not tried.** Refilling a finished lane with the next pair (per-lane indices, then a gather on load and a masked or per-lane store on finish) would recover the idle 12–23% and the cost of larger groups. AVX-512 would help in several ways: lane ctz via `vplzcntd` (CD) or `vpopcntd` instead of the float trick; mask registers for the blends (1 µop instead of the 3 that `vblendvps` costs on recent Intel cores); `vpternlogd`; and 32 registers to step 3 or 4 states without spills.

**Summary.** The (1+i)-ary gcd goes into AVX2 lanes without overflow and without a divide. The trick is to halve a − b before forming it. With finished lanes held by a fixed point rather than masks, and three registers stepped together to hide the step's latency, it takes 38 ns per gcd on random 31-bit pairs, 7.2 times faster than study 6's scalar Euclid (276 ns), and 20 against 119 ns with a large common factor. A fixed step count at the proven bound (276) is 11 times slower than pi_v3's loop and slower than Euclid. In scalar code Euclid remains the better gcd. Inputs with −2^31 halves are not covered.

## Study 25: Gaussian division in AVX2 lanes (divvec.c)

**What was compared.** x = a + bi divided by y = c + di, int32 halves over the full two's complement range (decision 12 allows −2^31), separate re and im arrays, no floating point anywhere. The quotient is x·conj(y)/N with X = ac + bd, Y = bc − ad and N = c² + d², all formed exactly in 64-bit lanes by `vpmuldq`. X can reach 2^63 (all four halves −2^31), where the 64-bit sum wraps; every other value fits. Each quotient part is at most |x|/|y| ≤ 2^31.5 in size, so its magnitude is below 2^32.
- **Floor without a sign branch.** s = [X < 0] as a mask (tested as X − 1 < −1, so the wrapped 2^63 counts as positive), U = X xor s (X, or |X| − 1), then q = floor(U/N) xor s. The floor remainder is U mod N, or N − 1 − (U mod N) where s is set. All methods share this front end and divide U by N.
- **Modes.** Exact (y divides x, q only). The floor core: floor parts of q plus r = x − qy, the latter mod 2^32 since floor can leave |r| up to √2·|y|. Nearest with a tie rule; a tie is a part of x/y exactly halfway between two integers, which happens only when 1 + i divides y. The four rules compared: ties up (the library standard, floor((2X + N)/(2N)) per part), ties down, ties toward zero and ties away from zero. Kept from an earlier round and reported separately: ties to even, ties to odd, and a coupled rule (a tied part rounds away from zero if the other part, rounded away from zero, is even, and toward zero if it is odd).
- **Two forms of each nearest rule.**
  - **Direct,** on the signed numerator: from the floor part q0 and the floor remainder ρ (3 instructions per part to form it from U mod N), round up where ρ > H. H is:
    - (N − 1) >> 1 for ties up, N >> 1 for ties down;
    - (N + s) >> 1 toward zero, (N − 1 − s) >> 1 away;
    - (N − odd(q0)) >> 1 to even, (N − 1 + odd(q0)) >> 1 to odd.
  - **Mask:** one sign mask m per part, |u| = (u xor m) − m, divide the magnitude (floor and truncation agree there), round the magnitude, then q = (q′ xor m) − m with the same m.
    - On the magnitude, away from zero is ties up, H = (N − 1) >> 1, and toward zero is ties down, H = N >> 1, with no sign in the threshold.
    - Ties up and ties down need the sign there: (N − 1 − m) >> 1 and (N + m) >> 1, reusing m.
    - AVX2 has no 64-bit arithmetic shift, so m is a compare against zero: two instructions for X, whose one wrapped value 2^63 must count as positive, one for Y.
- **Overflow.** A quotient part can be +2^31 only when y is a unit or (1 + i) times a unit and x has a −2^31 half. A nearest remainder part can be +2^31 only when |y| is near 2^31.5. Each kernel ORs "some output part equals −2^31" into a sticky flag, a superset of true overflow. Once per array, and only when the flag is set, a scalar pass decides exactly from the sign of the numerator (`resolve`). A second flag records zero divisors.

**Methods.**
- **m1, control:** scalar, two 64-bit `div` per element (`U/N` and `U % N` share one `div`).
- **m2, exact by the inverse mod 2^32 (survey section 4):** k = ctz(c | d) by the popcount method of study 26 (integer only), shift all four halves right by k, then one exact division by 1 + i where c and d are both odd (masked shifts, no blends), so N is odd. Newton inverse from the seed (3N) xor 2 in three steps (5, 10, 20, 40 bits), then q = x·conj(y)·N⁻¹ mod 2^32: 14 `vpmulld` per 8 elements. With a shared divisor all of that is done once, and per element only the shift, the 1 + i step and 4 `vpmulld` remain.
- **m3, non-restoring shift and subtract, branch-free in the lanes.** The partial remainder stays in [−N, N), which fits int64 even at N = 2^63, because the lanes wrap mod 2^64 and each true remainder is in range. One step per quotient bit is 9 instructions per 4 lanes: sign mask, bring down the next dividend bit, R = 2R + bit ∓ N. The quotient bits go into the dividend register's freed low bits (the sign bits, complemented at the end), so no separate quotient register is needed. Width W per register of 8 elements: the smallest of 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 with every U >> W < N (one shift and compare per 4 lanes per try).
- **m4, shared divisor, precomputed reciprocal (survey section 2):**
  - L = bitlen(N − 1) and m = floor(2^(L+32)/N), which lies in [2^32, 2^33). One scalar `div` per array.
  - Per part: S = U' + (U' >> 32)·m' + ((U' mod 2^32)·m' >> 32), with m' = m − 2^32 and U' = U >> max(0, L − 32), then q̂ = S >> (L − max(0, L − 32)). Two `vpmuludq` per 4 lanes.
  - Error: q − 1 ≤ q̂ ≤ q. For N ≥ 3, |q| ≤ 2^31.5/√3 bounds the reciprocal's share of the error by 0.41 and the dropped low product by 0.25. For N ≤ 2, m is exact.
  - So one exact correction: ρ = U − q̂N, then subtract N once if ρ ≥ N (unsigned 64-bit compare by bias).
- **m5, per-lane reciprocal (survey section 3.3):**
  - Normalise N by a 6-step binary search in 64-bit lanes (shift by 32, 16, …, 1 where the top bits are zero).
  - Take dh, the top 32 bits, and form v = floor(2^64/(dh + 1)). Linear seed (8/3 − 8/9·δ)·2^31, which is always below the true value. Three Newton steps in 32-bit form, one in 33-bit form, then two exact fix-ups.
  - An exhaustive run over all 2^31 values of dh shows the Newton result is short by at most 2. So two fix-ups make v exact for every dh.
  - Estimate as in m4 with per-lane shifts (`vpsllvq`, `vpsrlvq`). The error is at most q/(dh + 1) < 0.82 for N ≥ 3 and below 1 for N ≤ 2, so one correction.
- **m5x:** m5 with two registers per pass, each stage interleaved across them. The per-register dependency chain is about 160 cycles, so one register at a time leaves the out-of-order window short.

**Correctness.** Every kernel against an __int128 reference (exact rationals for each rule, written independently of the kernels' thresholds), quotient and remainder mod 2^32, both flags, and `resolve`.
- **Exhaustive at small sizes:** every (x, y) with halves in K-bit two's complement ranges, K = 3 to 6 (16,777,216 divisions with remainder at K = 6). Every exact division q·y with q and y in those ranges. The same again per shared divisor.
- **Edges:** every quadruple of 20 edge values (−2^31, −2^31 + 1, −2^31 + 2, ±2^30, ±(2^30 + 1), ±46,341, ±2^16, ±1, ±2, ±3, 0, 2^31 − 1, 2^31 − 2), plus every edge x over every divisor with parts in [−4, 4], and every exact product of edge values that fits. Each element ran alone in a register of 8, so the flags are checked per element. Every true overflow was flagged, and `resolve` was exact.
- **Random:** 10.5 million divisions with remainder, in kinds aimed at large quotients, N near powers of 2, divisors of every size and ties. 10.3 million exact divisions, including units with −2^31 halves. 40 shared divisors × 65,536.
- **Totals:** 3,760,966,169 element results, failures none.
- **Rule properties:** N(r) ≤ N(y)/2 for every rule in both forms. For ties up and ties down, r(x + ky) = r(x) for k = ±1, i and 1 + i (the remainder depends only on x mod y). 39.3 million checks, failures none.
- **Sign and unit changes:** 2,457,600 base pairs, a quarter of them at a tie, each under 13 changes, for m1, m3 and m5x and every rule in both forms. Seven changes are symmetries of x/y:
  - x times −1, i or −i: q and r times the same unit.
  - y times −1, i or −i: q times the inverse unit, r unchanged.
  - Conjugating both: q and r conjugated.
  
  For these the exact predicted q and r were checked. The other six (conjugating x alone or y alone, negating one part of x or of y) are not symmetries of x/y: x̄/y = conj(x/ȳ) is no unit or conjugate image of x/y. So for those only N(r) was compared.

| changes (7,372,800 cases each; both forms gave identical counts) | up | down | toward 0 | away from 0 | to even | to odd | coupled |
|---|---|---|---|---|---|---|---|
| a symmetry, q or r not as predicted | 1.5 to 1.9 million, all at ties | the same | 0 | 0 | 0 | 0 | 0 |
| a symmetry, N(r) changed | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| a one-sided change, N(r) changed | 6,079,671 | the same | the same | the same | the same | the same | the same |

Toward zero and away from zero (and to even, to odd, coupled) commute exactly with −1, i, −i and conjugation, in both forms. Ties up and ties down break those symmetries only at ties, and even there N(r) does not change, because every candidate at a tie has the same norm. Up and down instead have the fixed-remainder property, which the symmetric rules lack at ties (rounding survey, section 1).

The one-sided changes alter N(r) in 82% of cases under every rule, by the same count. The remainder norm is N(y) times the squared distance from the changed x/y to the nearest lattice point, which no rule can control. N(r) ≤ N(y)/2 held in every case.

**Other rules with the same symmetry.** A rule commutes with units and conjugation exactly when the choice at a tie depends only on quantities they leave unchanged, and acts oddly on the tied part. Toward zero, away from zero, to even and to odd use the tied part alone (its sign or its parity). The coupled rule uses the other part's parity. Others in the same family would decide by the parity of N(x) or N(y), or treat corner ties (both parts halves) differently from edge ties. At a corner the natural joint choices reduce to per-part ones:
- the candidate of largest norm is away from zero, and the smallest is toward zero;
- the candidate with both parts even is to even, and both odd is to odd.

The coupled rule was built and tested as the representative non-per-part rule.

**Tests catch faults.** Removing the m4/m5 correction, the final restore in m3, or the 1 + i step on x in m2 each failed millions of results. So did a W one too small, using bitlen(N) for m4's L, or ties down in place of ties up. The plain sign of X (without the wrapped 2^63 case) gave 41 failures. Dropping the second reciprocal fix-up was not caught end to end; the exhaustive reciprocal run is what proves it is needed.

**Instruction counts** (per element, dynamic, counted with valgrind over 4,096 elements, loads, stores, loop and per-call setup included):

| per element | random 32-bit inputs | small divisor (parts ≤ 256) | exact (q and y below 2^15) |
|---|---|---|---|
| m1 varying / shared | 71 / 56 | 71 / 56 | 53 / 38 |
| m2 varying / shared | | | 9.9 / 3.1 |
| m3 varying / shared | 27.7 / 23.1 (mean W 1.85) | 172 / 154 (W 24 to 32) | 104 / 102 (W 16) |
| m4 (shared) | 18.5 | 18.5 | 16.9 |
| m5 / m5x | 45.0 / 48.1 | 45.0 / 48.1 | 43.3 / 46.2 |

An m3 bit step is 39 instructions per 8 elements (4.9 per element per bit). gcc spills in m5x: 249 of its 773 loop instructions per 16 elements touch the stack.

**Timing.** Same machine as studies 19 to 24 (Intel Xeon family 6 model 173, 2-vCPU KVM guest, about 3.8 GHz). Load below 0.6 before each run, nothing else running, pinned to one CPU, 31 repetitions, kernels interleaved with order rotated, an untimed call before each timed one. The clock is read in integer nanoseconds. In L1: 1,024 elements, the ranges of the medians of three runs. From memory: 2^24 elements (64 MB per array, 512 MB in all), one run. Quartile spreads were about ±1%.

| ns per element, floor core | L1, random | L1, small divisor | memory, random | memory, small divisor |
|---|---|---|---|---|
| m1 | 5.21 | 5.18–5.20 | 5.36 | 5.41 |
| m3 | 2.59 | 15.83–15.90 | 3.63 | 15.98 |
| m5 | 5.93–5.97 | 5.93–5.95 | 6.13 | 6.09 |
| m5x | 4.44–4.47 | 4.46–4.48 | 4.90 | 4.91 |
| shared: m1 | 5.17–5.18 | 5.18–5.20 | 5.47 | 5.48 |
| shared: m3 | 1.70–1.71 | 13.45–13.47 | 1.95 | 16.80 |
| shared: m4 | 1.73–1.74 | 1.74 | 1.99 | 1.99 |

| ns per element, exact | L1 | memory |
|---|---|---|
| m1, varying / shared | 5.16–5.19 / 5.15–5.19 | 5.37 / 5.28 |
| m2, varying / shared | 1.61–1.62 / 0.21 | 1.99 / 0.89 |
| m3, varying / shared | 9.52–9.54 / 9.27–9.32 | 9.74 / 9.42 |
| m4 (shared) | 1.50 | 1.78 |
| m5x | 4.21–4.24 | 5.05 |

**Cost against quotient width** (m3 floor core with a forced width, L1, ns per element): W = 1: 1.87; 2: 2.23; 4: 2.96; 8: 4.68; 16: 7.94; 24: 11.38; 32: 14.64. That is about 1.5 ns plus 0.41 ns per quotient bit; m3 beats the hardware divide up to about 8 bits. On random 32-bit inputs the chosen width averaged 1.85 bits per register (59 registers at 1, 43 at 2, 18 at 3, 8 above). On exact divisions with 15-bit quotients it was 16, and with divisors up to 256 it was 24 or 32.

**Threshold for the shared-divisor precomputation** (whole call including m4's setup, ns per call, L1):

| n | m1 shared | m3 shared, random divisor | m3 shared, small divisor | m4 |
|---|---|---|---|---|
| 8 | 41.2 | 16.3 | 109.4 | 17.3 |
| 32 | 165.5 | 56.3 | 534.6 | 58.8 |
| 256 | 1,340 | 443 | 3,454 | 452 |

m4's setup costs about 3.3 ns (one `div` and the broadcasts). Against the hardware divide it pays from 4 elements (m1 shared takes about 5.15 ns per element and m4's smallest call, one register of 8, 17.3 ns). Against m3 it depends on the quotients, not on the length. With random 31-bit divisors the quotients are 1 or 2 bits, and m3 ties with m4 at every length (within 5%; one register with a wide quotient costs m3 a step, as at n = 48). With small divisors m4 is 6 to 8 times faster at every length, 8 included.

**The four tie rules, each in its two forms.** Instructions per element (valgrind, random inputs) and ns per element. L1 is the range of the medians of three runs; memory is one run of 2^24 elements. The floor core is shown for reference.

| m3 (varying divisor) | instructions | L1 | memory |
|---|---|---|---|
| floor core | 27.7 | 2.58–2.61 | 3.65 |
| up: direct / mask | 33.4 / 33.2 | 2.87–2.89 / 2.89–2.92 | 3.91 / 3.93 |
| down: direct / mask | 33.4 / 32.6 | 2.88–2.89 / 2.89–2.90 | 3.90 / 3.91 |
| toward zero: direct / mask | 33.8 / 31.4 | 2.95–2.98 / 2.81–2.82 | 4.00 / 3.87 |
| away from zero: direct / mask | 34.4 / 32.1 | 2.93–2.95 / 2.80–2.82 | 3.95 / 3.82 |

| m5x (varying divisor) | instructions | L1 | memory |
|---|---|---|---|
| floor core | 48.1 | 4.45–4.47 | 4.91 |
| up: direct / mask | 53.3 / 53.0 | 4.87–4.89 / 4.85–4.88 | 5.27 / 5.26 |
| down: direct / mask | 52.6 / 52.7 | 4.87–4.89 / 4.88–4.90 | 5.29 / 5.29 |
| toward zero: direct / mask | 54.5 / 51.8 | 4.92–4.95 / 4.83–4.84 | 5.35 / 5.25 |
| away from zero: direct / mask | 54.6 / 52.0 | 4.93–4.97 / 4.82–4.86 | 5.31 / 5.23 |

| m4 (shared divisor) | instructions | L1 | memory |
|---|---|---|---|
| floor core | 18.5 | 1.74–1.75 | 1.99 |
| up: direct / mask | 22.5 / 23.0 | 2.16–2.18 / 2.21 | 2.38 / 2.43 |
| down: direct / mask | 22.5 / 22.8 | 2.16–2.19 / 2.15–2.17 | 2.39 / 2.38 |
| toward zero: direct / mask | 24.4 / 21.4 | 2.22–2.25 / 2.10–2.13 | 2.45 / 2.34 |
| away from zero: direct / mask | 24.5 / 21.4 | 2.22–2.23 / 2.10–2.14 | 2.45 / 2.35 |

| m1 (scalar control) | instructions | L1 | memory |
|---|---|---|---|
| floor core | 71 | 5.18–5.20 | 5.43 |
| up: direct / mask | 100 / 96 | 6.35–6.59 / 9.18–9.23 | 6.85 / 9.38 |
| down: direct / mask | 100 / 93 | 6.33–6.55 / 9.14–9.17 | 6.76 / 9.34 |
| toward zero: direct / mask | 99 / 92 | 6.47–6.52 / 6.53–6.56 | 6.76 / 6.78 |
| away from zero: direct / mask | 101 / 93 | 6.53–6.55 / 6.53–6.60 | 6.87 / 6.84 |

**Rule by rule, direct against mask.**
- **Ties up: direct.** In lanes the two forms tie: within 0.03 ns for m3 and m5x, and the direct form is 0.05 ns ahead for m4. The mask form saves the 3-instruction floor-remainder fix per part. But it spends one subtract to form |u| and one more to restore the sign, and its threshold needs the sign anyway. In scalar code the mask form runs 9.2 ns against 6.4 (the gcc sensitivity below).
- **Ties down: either; direct by a hair.** The same balance: within 0.02 ns in every lane method. Scalar again favours the direct form (6.4 against 9.2).
- **Toward zero: mask.** On the magnitude it is plain ties down, with one threshold per element instead of one per part, and no remainder fix. 2.4 to 3.0 fewer instructions per element, and 0.09 to 0.15 ns faster in L1 (m3 2.82 against 2.97, m5x 4.83 against 4.94, m4 2.12 against 2.23). From memory 0.1 ns faster. Scalar: equal.
- **Away from zero: mask,** for the same reason (on the magnitude it is ties up): 0.08 to 0.13 ns faster in L1 and 0.08 to 0.13 ns from memory. Scalar: equal.

**The four rules, each in its better form** (ns per element, L1 middle run, and the increase over the floor core):

| | up (direct) | down (direct) | toward zero (mask) | away from zero (mask) |
|---|---|---|---|---|
| m3 | 2.88 (+0.29) | 2.88 (+0.29) | 2.82 (+0.23) | 2.80 (+0.21) |
| m5x | 4.88 (+0.42) | 4.89 (+0.43) | 4.83 (+0.37) | 4.86 (+0.40) |
| m4 | 2.16 (+0.41) | 2.16 (+0.41) | 2.10 (+0.35) | 2.10 (+0.35) |
| m1 (scalar forms tie for toward and away; direct shown) | 6.45 (+1.28) | 6.38 (+1.20) | 6.48 (+1.30) | 6.53 (+1.35) |
| instructions over floor, m3 / m5x / m4 | 5.8 / 5.3 / 4.0 | 5.8 / 4.6 / 4.0 | 3.8 / 3.7 / 2.9 | 4.4 / 3.9 / 2.9 |

From memory the order is the same: m3 3.91, 3.90, 3.87, 3.82; m4 2.38, 2.39, 2.34, 2.35. In lanes the symmetric rules in mask form are the cheapest of the four, by 0.02 to 0.08 ns per element (1 to 2 instructions); the library standard (ties up) is not cheaper. All four add under half a nanosecond per element over the floor core.

**How much of ties up's cost is negative handling.** Timed against copies with all sign handling removed (wrong for negative numerators, timing only):
- m5x: floor 4.28 without signs against 4.46 with; ties up 4.54 against 4.88.
- m4: 1.42 against 1.75; ties up 1.67 against 2.17.

So in ties up's direct form the negative handling costs 5.0 instructions per element in m5x (0.34 ns of 4.88, 7%) and 4.75 in m4 (0.48 ns of 2.17, 22%). That covers the sign compares, the one's complement in and out, and the floor-remainder fix. The rounding itself costs 0.26 ns in both. For m3 the copy without signs is not a fair measure, since negative numerators read as huge magnitudes and widen the quotient. Its negative handling is the same 26 instructions per 8 elements (3.25 per element) by count.

**Ties to even, to odd and coupled** (kept from the earlier round; ns L1, direct / mask):
- m3: 3.07 / 2.99, 3.07 / 2.98, 3.23 / 3.06.
- m5x: 5.05 / 5.00, 5.03 / 5.00, 5.18 / 5.16.
- m4: 2.30 / 2.25, 2.32 / 2.21, 2.43 / 2.51.

The mask form wins for even and odd (they also need no sign in the threshold). The coupled rule is 0.1 to 0.3 ns dearer than toward or away from zero. All three are 0.1 to 0.2 ns dearer than toward and away in their better form.

**The scalar control is fragile under gcc.** The same rule written in equivalent ways ran at 6.4 or 12.3 ns (ties up direct and toward zero direct, in earlier builds), 7.1 or 8.4 ns (to even), and now 9.2 ns for both mask forms that put the sign in the threshold. The divides and branches are the same; alignment and scheduling flags did not change it. The m1 rows use one build, with the sign masks kept in registers (an empty asm barrier). The vector timings were stable to 0.03 ns across builds.

**What the figures show.**
- **Exact division: m2 is the method.** 1.6 ns against the divide's 5.2 in L1 (3.2 times) and 2.0 against 5.4 from memory. With a shared divisor it is 0.21 ns per element (25 times the divide), limited only by memory (0.89 ns) for large arrays. It needs no 64-bit arithmetic at all. A divisibility check, if the caller cannot guarantee it, would add a back-multiply (not timed).
- **Remainder, varying divisor:**
  - m3 wins when quotients are short: 2.59 ns against 5.21 in L1 (2.0 times), 3.6 against 5.4 from memory. Random 31-bit pairs and Euclid's later steps are of this kind. Its cost grows by 0.41 ns per quotient bit, so with divisors up to 256 (24-bit quotients) it takes 15.9 ns, three times the divide.
  - m5x does not depend on the data: 4.45 ns, 1.17 times faster than the divide in L1 and 1.09 times from memory. One register at a time (m5) is slower than the divide (5.95).
  - The per-lane reciprocal costs about 30 instructions per element before any division work. The 64-bit normalisation and the Newton steps run on 4 lanes per instruction and are latency-bound.
- **Remainder, shared divisor:** m4 at 1.74 ns is 3.0 times faster than the divide in L1 (2.75 times from memory) whatever the quotient size. Its precomputation pays from 4 elements. m3 ties with it only when the quotients are 1 or 2 bits.
- **Tie rules** add 0.2 to 0.45 ns per element in lanes. With each rule in its better form, toward zero and away from zero (mask form) are the cheapest of the four; ties up and ties down (direct form) cost 0.02 to 0.08 ns more. The symmetric rules are therefore free relative to the library standard.
- **Overflow** is confined to a few inputs with −2^31 halves or a divisor near 2^31.5 in size. The candidate flag costs 2 to 4 compares per 8 elements, and the exact decision runs only when it fires.

**What AVX-512 would change** (not built). `vplzcntq` (CD) replaces m5's 6-step normalisation (30 instructions per 4 lanes) with one instruction. `vpcmpuq` into mask registers removes every bias XOR, and masked subtracts make the corrections 1 instruction. `vpmullq` (DQ) gives m2 64-bit products if wider halves are wanted. IFMA's 52-bit multiplies would let m5 use a longer reciprocal and drop the fix-ups. `vpopcntd` or `vplzcntd` gives m2's ctz in 2 to 4 instructions.

**Summary.**
- Exact division: multiply by the inverse mod 2^32 after stripping 2 and 1 + i, 3.2 times faster than the divide (25 times with a shared divisor).
- Remainder with a shared divisor: the precomputed 33-bit reciprocal with one correction, 3 times faster, paying from 4 elements.
- Remainder with varying divisors: no method dominates. Shift and subtract is twice as fast as the divide for quotients of a few bits and loses beyond about 8 bits. The per-lane Newton reciprocal is steady at 1.17 times the divide.
- Tie rules: ties up and ties down are best written directly on the signed numerator; toward zero and away from zero are best written on the magnitude with one sign mask, and are then the cheapest. All keep N(r) ≤ N(y)/2. Toward and away from zero commute exactly with −1, i and conjugation; up and down break that only at ties (without changing any remainder norm) but keep the remainder a function of x mod y.

## Study 26: a fair gcd comparison, integer only (gcdfair.c)

**What was compared.** Study 24's arrays of independent pairs, int32 halves, now over the full two's complement range (decision 12), gcd normalised to re > 0 and im ≥ 0, with no floating point anywhere. Four rows:
- **Euclid scalar** (study 6, two `idiv` per step).
- **Euclid in vector lanes** (euclid_v). Each step divides by study 25's m3 (non-restoring, width per register), with ties up, study 6's rule, so its steps match the scalar Euclid's exactly. Finished lanes (b = 0) are held by masks. The test is "every b of the register is 0".
- **Binary (1+i)-ary scalar** (pi_sb, study 24's branch-free step).
- **Binary vector** (pi_v3, study 24's best: 3 registers stepped together).

Also timed: euclid_v5 (study 25's m5x division, two registers), and pi_v with one register.

**Integer-only lane ctz** (replacing study 24's float exponent):
- (a) **bs:** binary search. Where the low 16, 8, 4, 2 bits are zero, add that count and shift them out (`vpsrlvd`), then add 1 if bit 0 is clear. 22 instructions.
- (b) **pop:** popcount of (x AND −x) − 1. Two `vpshufb` lookups of a 16-entry nibble table, then `vpmaddubsw` and `vpmaddwd` to sum the bytes of each lane. 11 instructions.
- (c) **db:** de Bruijn. (x AND −x)·0x077CB531 by `vpmulld`, the top 5 bits index a 32-entry table. That is two `vpshufb` on table halves; adding 0x70 or subtracting 0x10 from the index byte sets bit 7 (which `vpshufb` reads as zero) for the wrong half, and the other three bytes get 0x80. 9 instructions.

**The −2^31 gap, closed.** Making a number primary negates it, and so does normalising a gcd; −2^31 has no negation in int32. The true gcd can itself be 2^31 (gcd(−2^31, 0) = 2^31, gcd(−2^31 − 5i, 0) = 2^31 + 5i), which does not fit.
- Every kernel detects pairs with a −2^31 half: 4 compares and 3 ORs per register in lanes, one test per pair in scalar code.
- In lanes it replaces such pairs by (0, 0), (0, 0), so every lane stays inside study 24's proven range, and ORs the mask into a sticky flag.
- Once per array, only if the flag is set, `fixup` recomputes exactly those pairs by the (1+i)-ary gcd in 64-bit scalar registers. There every value stays within ±2^31 and |x| + |y| within 2^32.
- It returns the overflow flag (some gcd has a part 2^31, so the array must be promoted) and stores the result mod 2^32.
- The scalar kernels skip such pairs in their loop and call the same `fixup`.

**Correctness.** Every kernel against an independent reference: study 6's Euclid in __int128 with exact rounding, normalised. The overflow flag must equal "some gcd in the batch does not fit int32".
- **The three ctz methods:** every nonzero 32-bit value against `__builtin_ctz`: no errors.
- **Exhaustive:** every pair with halves in K-bit two's complement ranges, K = 3 to 6 (16,777,216 pairs at K = 6). At K ≤ 4 the reference was also checked by brute force and divisibility.
- **Edges with −2^31:** every quadruple of 20 values (−2^31, −2^31 + 1, −2^31 + 2, 0, ±1, ±2, 3, −3, 2^31 − 1, 2^31 − 2, ±2^30, ±(2^30 + 1), ±2^16, ±46,341). Each pair ran alone, so the flag is per pair: 160,000 pairs, 29,679 with a −2^31 half, 87 with an overflowing gcd.
- **Random −2^31 pairs alone:** 200,000 pairs with planted −2^31 halves and power-of-2 factors; 1,802 overflowing gcds.
- **Random batches:** 2,621,440 pairs, of study 24's four kinds plus a few −2^31 pairs.
- **Failures: none.**
- **Tests catch faults:** no fixup (1.8 million failures); no overflow flag (19,220); a wrong de Bruijn constant (18,482); no hold mask in vector Euclid (28.9 million). A broken last step in the bs ctz made a loop never end, as in study 24. With the guard removed from pi_v3, the −2^31 pairs gave 11,793 wrong results (no hang in these runs).

**Steps** (65,536 pairs per kind): Euclid 20.39 steps per pair on random pairs, 23.16 per register of 8 (lane use 88%); with a large common factor 10.24 and 12.17 (84%). The vector kernel's own count of register steps matched exactly.

**Instruction counts** (dynamic, valgrind, per gcd, setup and fixup checks included):

| per gcd | euclid | euclid_v | euclid_v5 | pi_sb | pi_v3_bs | pi_v3_pop | pi_v3_db |
|---|---|---|---|---|---|---|---|
| random | 1,301 | 966 | 1,240 | 2,413 | 378 | 337 | 331 |
| large common factor | 678 | 496 | 666 | 1,215 | 205 | 183 | 180 |

The pi_v3 step loop (3 registers) is 238, 212 and 208 instructions with bs, pop and db, so 79, 71 and 69 per register. Study 24's float version had 63 per register. euclid_v's step is about 330 instructions per register of 8 at the usual widths of 1 or 2 quotient bits.

**The ctz alone** (ns per register of 8, median of 31; latency of a chain v ← v + ctz(v) + 1):

| | instructions | throughput | latency |
|---|---|---|---|
| bs | 22 | 3.4–3.7 | 7.0 |
| pop | 11 | 1.7–2.0 | 5.1–5.2 |
| db | 9 (one `vpmulld`) | 1.25 | 4.9 |

**Timing.** Same machine and harness as study 24: load below 0.6, nothing else running, pinned to one CPU, 31 repetitions, kernels interleaved with order rotated, an untimed call first. 1,032 pairs (24 KB, in L1), integer clock. Ranges of the medians of three runs, ns per gcd; quartile spreads about ±1%.

| ns per gcd | random | large common factor (2^15) |
|---|---|---|
| Euclid scalar | 269.8–271.2 | 121.8–123.0 |
| Euclid in lanes (m3 division, ties up) | 108.5–109.1 | 50.3–50.9 |
| Euclid in lanes (m5x division, 2 registers) | 115.5–115.6 | 61.9–62.0 |
| binary scalar (pi_sb) | 329.4–331.6 | 167.1–168.0 |
| binary vector, pi_v3_bs | 48.6–49.0 | 26.0–26.2 |
| binary vector, pi_v3_pop | 42.6–42.9 | 23.0–23.1 |
| binary vector, pi_v3_db | 42.8–43.2 | 23.0–23.4 |
| binary vector, pi_v3_pop without the guard | 42.3–42.8 | 22.8 |
| one register: pi_v_bs / pop / db | 69.9–70.4 / 67.6–67.7 / 67.5–67.6 | 36.7 / 35.4–35.5 / 34.9–35.1 |

**What the figures show.**
- **The fair four rows** (random pairs, then large common factor):

  | | random | ratio to scalar Euclid | large factor | ratio to scalar Euclid |
  |---|---|---|---|---|
  | Euclid scalar | 270 | 1 | 122 | 1 |
  | Euclid in lanes | 109 | 2.5 times faster | 50.5 | 2.4 times faster |
  | binary scalar | 330 | 1.2 times slower | 167 | 1.4 times slower |
  | binary vector | 42.7 | 6.3 times faster | 23 | 5.3 times faster |

  In lanes the binary gcd is still 2.5 times faster than Euclid (2.2 with a large factor). Euclid gains 2.5 times from lanes; the binary gcd gains 7.7 times over its own scalar form.
- **Euclid's division in lanes:** m3 beats m5x (109 against 115 ns) even though m5x is the faster divider for random division. Euclid's quotients are mostly 1 or 2 bits, where m3 costs about 2 ns per element against m5x's 4.4. This matches study 25's width curve.
- **Integer ctz against the float trick:**
  - pop and db tie inside pi_v3 (42.6 to 43.2 ns). bs is 14% slower (49 ns): its 4 dependent compare, shift and add rounds are the longest chain.
  - db has the shortest latency and best throughput alone, but its `vpmulld` (10 cycles on this core) leaves it no faster than pop inside the step.
  - Against study 24's float exponent (38.1 to 38.6 and 20.0 to 20.1 ns), the integer ctz costs 11% on random pairs and 15% with a large common factor. That is the price of the no-floating-point rule here, about 4.5 ns per gcd.
- **The −2^31 guard is nearly free:** pi_v3_pop with and without it differ by at most 0.3 ns per gcd (under 1%), about 1 instruction per gcd. The fixup runs only for arrays containing such a pair.

**What AVX-512 would offer** (not built; this CPU has AVX-512 CD, BITALG and VPOPCNTDQ):
- `vplzcntd` (CD) gives ctz as 31 − lzcnt(x AND −x), 4 instructions.
- `vpopcntd` (VPOPCNTDQ) gives popcount((x AND −x) − 1), also 4 instructions, about 6 cycles of latency against about 18 for pop and db here.
- Neither has a direct lane tzcnt; both work on 64-bit lanes too (`vplzcntq`, `vpopcntq`).
- With the ctz at 4 instructions the step loses about 7 instructions per register and a third of the ctz latency, likely enough to recover study 24's float-trick figure (not measured).
- Mask registers would also cut the 10 blends per step (3 µops each with `vblendvps` here) to 1 µop each.

**Summary.**
- With integer-only ctz and the −2^31 gap closed, the binary vector gcd (3 registers, popcount or de Bruijn ctz) takes 42.7 ns per random 31-bit pair. That is 6.3 times faster than scalar Euclid (270 ns) and 2.5 times faster than Euclid in vector lanes (109 ns).
- With a large common factor: 23, 122 and 50.5 ns.
- Euclid in lanes uses study 25's shift-and-subtract division, which suits its short quotients better than the reciprocal.
- The integer ctz costs about 11% over study 24's float exponent; AVX-512's `vpopcntd` or `vplzcntd` would remove that.
- Pairs with a −2^31 half are detected for under 1% of the time, computed exactly in a 64-bit scalar pass once per array, and flagged when the gcd itself (as 2^31) does not fit.

## Study 27: integer-only lane ctz for one set bit (ctz27.c)

**What was compared.** The position k of the single set bit in x = o AND −o (x = 2^k), in 32-bit AVX2 lanes, with no floating point. Study 24's float exponent (flt) is kept only as the reference. Each form was timed alone and inside study 26's binary vector gcd: pi_v3 (3 registers) and pi_v (1 register, latency-bound), both with the −2^31 guard. Op counts below include the 2 ops for x. gcc 13.3 -O2 -mavx2. The machine is a Granite Rapids Xeon (family 6, model 173), the same core as studies 24 and 26: `vpmulld` 10 cycles and 2 µops, `vpmaddwd` and `vpmullw` 5 cycles, `vcvtdq2ps` 4.

**The forms.**

| form | how | ops | dependency depth (cycles, estimated) |
|---|---|---|---|
| flt | `vcvtdq2ps`, shift 23, subtract 127 (reference) | 5 | 8 |
| bs | study 26 (a): binary search with variable shifts | 22 | 14 |
| pop | study 26 (b): popcount of x − 1 (nibble table, `vpmaddubsw` + `vpmaddwd`) | 11 | 17 |
| pop2 | pop on the thermometer andn(o, o − 1) = 2^k − 1, which saves one op | 10 | 16 |
| db | study 26 (c): de Bruijn, `vpmulld`, top 5 bits, two `vpshufb` (add 0x70, subtract 0x10) | 9 | 16 |
| db3 | db with one index op: k = Tb[i] XOR Td[i + 0x70] (below) | 8 | 16 |
| dbm | 16-bit de Bruijn 0x09AF through `vpmaddwd`: k mod 16 from one `vpshufb`, OR 16·[k ≥ 16] | 9 | 11 |
| dbh | `vpmullw` by 0x09AF per word, `vpsrlw` 12, one `vpshufb`; `vpsrlvd` by s = 16·[k ≥ 16] moves the hit word down, then OR s | 9 | 11 |
| mask5 | the five bit tests: k_j·2^j = min_epu32(x AND M_j, 2^j), ORed | 16 | 7 |
| mask5c | the five tests with compare masks: k = 31 + Σ (cmpeq(x AND M_j, 0) AND −2^j) | 22 | 8 |
| mix | 8·(k >> 3) from two tests, x shifted right by it, then two nibble tables | 13 | 10 |
| mixn | 4·(k >> 2) from three tests, x shifted right by it, then one nibble table | 13 | 9 |

**The tricks that made forms cheaper.**
- **A bit test in 2 ops.** x AND M_j is 0 or x. When bit j of k is set, k ≥ 2^j, so x ≥ 2^(2^j) ≥ 2^j. Then min_epu32(x AND M_j, 2^j) is exactly k_j·2^j: an AND and a min, with no compare and no weight. The other ways of turning a test into a bit cost 3 or 4 ops per bit: compare to zero then AND with a weight (mask5c), summing masks Horner-style and negating, or the sign of −(x AND M_j) shifted by 31. Five tests and 4 ORs make 14 ops, so testing is cheap in latency but heavy in instructions.
- **Fewer tests through a shift and a table.** After the coarse part s (two or three tests), x >> s is 2^r in the low byte or nibble, and `vpshufb` reads it. One `vpshufb` can tell apart at most 4 single-bit positions: bits 0 to 3 index the table, bit 7 zeroes the result, and bits 4 to 6 all fall on entry 0. So a table finishes a nibble (mixn: 3 tests, 1 table) or, with a second table on x >> 4, a byte (mix: 2 tests, 2 tables). Both take 13 ops.
- **A 32-entry table with one index op (db3).** For the top-5-bit index i of the de Bruijn product (0 to 31, bytes 1 to 3 zero), set Tb[n] = K[16 + n] and Td[n] = K[n] XOR K[16 + n]. `vpshufb`(Tb, i) reads n = i AND 15. i + 0x70 has bit 7 set exactly when i ≥ 16, so `vpshufb`(Td, i + 0x70) is Td[n] for i < 16 and 0 otherwise, and the XOR is K[i]. In bytes 1 to 3 the two lookups are Tb[0] and Td[0], whose XOR is K[0] = 0. That is one op fewer than study 26's db.
- **A shorter multiply (dbm, dbh).** `vpmaddwd` of x with 0x09AF in both words gives 2^l·0x09AF (l = k mod 16) exactly, since the other word is 0. For l = 15 the word is −2^15, and the bits read (12 to 15) still agree. 0x09AF is a 16-bit de Bruijn sequence, so its top nibble after the shift indexes a 16-entry table, and 16·[k ≥ 16] comes from one 2-op test. That is 5 cycles of multiply in place of 10, at 9 ops.

**Searched and ruled out.**
- **One `vpmaddwd` giving a 32-entry index.** An exhaustive search over both word constants (2^32 pairs), every shift and both field shapes (5 contiguous bits; bits 0 to 3 plus 7 of a byte) found nothing. The products 2^l·c for l = 14 and 15 in both words read only c's lowest bits, which forces collisions. So `vpmaddwd` alone cannot replace `vpmulld`, and dbm needs its separate k ≥ 16 test.
- **The index read straight from the bits `vpshufb` uses** (bits 24 to 27 and 31 of the product, which would save the shift). A full search proves no `vpmulld` constant works: for k ≥ 28 the nibble is all zero-fill, so at most 2 distinct indices are left. The same holds for every byte of the `vpmaddwd` product.
- **k = T[i AND 15] + 16·(a fifth bit) with one table.** Impossible for any multiply-made index (proof by the shift structure), so a 32-entry lookup costs 4 ops (db3) or a 16-entry one plus a test (dbm).
- **`vpermd` as the table.** It holds 8 dwords, so it needs a byte select afterwards: 6 ops after the multiply, against db3's 4.
- **The whole bit in one byte-level `vpshufb`.** The table cannot see which byte it is in, so the byte's position needs a horizontal sum (`vpmaddubsw` + `vpmaddwd`, 10 cycles) or a fold plus a mask (5 ops). Every such form came to 14 ops or more.
- **Leaving k out of the step.** The step uses k only for the two `vpsravd`, which need the count. hx >> k is exact (2^k divides hx and hy), but multiplying by 2^(31−k) needs a bit reverse of x (about 7 ops) and 4 `vpmuldq` with packing, and shifting hx and hy alongside the binary search triples the shifts. Neither was built.
- **gather from a de Bruijn table:** about 20 cycles on Intel; not built.

**Correctness.**
- Every form, against `__builtin_ctz`: all 32 single-bit inputs, then every nonzero 32-bit value. No errors, except flt on 2^31 (one value), which the gcd never produces since |h| < 2^31.
- Inside the gcd, every pi_v3 and pi_v kernel against study 26's __int128 reference: exhaustive K = 3 to 6 (16,777,216 pairs at K = 6), the 160,000 edge pairs with −2^31, and 2,621,440 random pairs of the five kinds. 0 failures.
- Fault injection: one wrong Td entry in db3, constant 0x09AE in dbm, and the k ≥ 16 test dropped from mixn. The single-bit check caught all three (1, 8 and 16 of 32 wrong). In the gcd they pass the exhaustive runs up to K = 5, because the faults touch only large k, and they make the 31-bit runs never end (as in studies 24 and 26). So the all-inputs ctz check is the test that matters.

**Instruction counts** (objdump). The pi_v3 step loop over 3 registers is 188 instructions with flt, 205 with db3, 206 with db and dbm, 207 with dbh, 209 with pop and pop2, 218 with mixn, 219 with mix, 231 with mask5, 238 with bs and 240 with mask5c. Per register that is 62.7, 68.3, 68.7, 69, 69.7, 72.7, 73, 77, 79.3 and 80. The loop has no stack operands. Dynamic counts per gcd on random pairs (valgrind): flt 299, db3 326, dbm and db 328, dbh 329, pop 332, mixn 347, mask5 367.

**The ctz alone** (ns per register of 8, median of 51 runs, inlined; latency from a chain v ← (v + ctz(v)) OR 2^30, which adds 2 cycles):

| | flt | bs | pop | pop2 | db | db3 | dbm | dbh | mask5 | mask5c | mix | mixn |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| throughput | 0.57 | 2.18 | 1.11 | 1.00 | 1.06 | 0.94 | 0.89 | 0.88 | 1.52 | 1.98 | 1.25 | 1.25 |
| latency | 3.09 | 5.46 | 5.15 | 4.94 | 5.14 | 4.99 | 3.69 | 3.69 | 3.25 | 4.06 | 3.32 | 3.09 |

**Timing in the gcd.** Same harness as study 26: load below 0.6, nothing else running, pinned to one CPU, 51 repetitions, kernels interleaved with order rotated, an untimed call first. 1,032 pairs. Ranges of the medians of three runs, ns per gcd; quartile spreads mostly ±1%, with some upper quartiles up to +10% in one run.

| ns per gcd | pi_v3 random | pi_v3 large factor 2^15 | pi_v random | pi_v large factor |
|---|---|---|---|---|
| flt (float) | 37.7–38.3 | 20.4–20.7 | 57.9–58.8 | 29.9–30.1 |
| dbh | 40.0–40.4 | 21.5–21.6 | 61.2–61.6 | 32.0–32.3 |
| dbm | 40.1–40.7 | 21.6–21.7 | 62.0–62.9 | 32.2–32.5 |
| mixn | 41.3–41.9 | 22.2–22.5 | 58.2–58.4 | 30.2–30.4 |
| pop2 | 41.4–42.0 | 22.5–22.9 | | |
| mix | 41.8–42.3 | 22.5–22.7 | | |
| mask5 | 41.8–42.3 | 22.6–22.8 | 59.9–60.5 | 31.7–31.9 |
| db3 | 42.7–43.0 | 22.9–23.1 | 67.0–67.8 | 33.9–34.0 |
| pop (study 26) | 43.0–43.8 | 23.2–23.5 | 67.7–68.5 | 35.6–35.7 |
| db (study 26) | 43.2–43.7 | 23.1–23.4 | 67.7–68.3 | 35.3–35.5 |
| mask5c | 44.2–44.9 | 23.7–24.0 | | |
| bs | 48.8–49.3 | 26.2–26.4 | | |

**What the figures show.**
- **Three registers stepped together are bound by instruction count, not by latency.** The ranking follows the step loop's size, with the 2-µop `vpmulld` counted as two. dbh and dbm (9 ops, 9 µops, 11 cycles deep) are best at 40.0 to 40.7 ns, against the float trick's 37.7 to 38.3. The gap is about 2.1 ns (5.5%) on random pairs and 1.1 ns (5.5%) with a large common factor. Study 26's best integer forms (pop, db) stood at 4.5 to 5 ns (13%), so the gap is more than halved.
- **db3 saves an op but not time:** its `vpmulld` (10 cycles, 2 µops) leaves it at 42.7 to 43.0, between pop2 and db.
- **The mask-test idea is fast in latency but expensive in instructions.** mask5 has the shortest chain of any integer form (7 ops deep, 3.25 ns chain) but 16 ops, so it reaches only 42 ns in pi_v3. Its best use is mixed with a table: mixn (3 tests, a shift, one 16-entry `vpshufb`) has the float trick's chain latency (3.09 ns).
- **With one register (latency-bound), mixn ties the float trick:** 58.2 to 58.4 ns against 57.9 to 58.8, and 30.2 to 30.4 against 29.9 to 30.1. The multiply-based forms lose there: dbh 61 ns, db 68.
- **So the best form depends on the kernel.** dbh (or dbm) for the 3-register gcd, mixn where one chain is latency-bound. No integer form here reaches flt's 5 ops: the converter's priority encoder is worth about 4 instructions per step.

**Fitting the mask tests into the step (milestone 5).** Everything above assembles k and then runs one `vpsravd` per part by k. The step only needs hx, hy >> ctz(o), so these forms were fitted to the step instead and timed in the same harness, in a later session. Its three runs (load below 1) repeated the earlier figures within 0.3 ns, except pi_v flt at 57.0 to 57.3.
- **(c) Distribution of k per step** (65,536 pairs per kind, 1,987,953 live lane-steps on random pairs):

  | k | 0 | 1 | 2 | 3 | 4 | 5 | 6 or more |
  |---|---|---|---|---|---|---|---|
  | lanes | 50.6% | 37.5% | 9.1% | 2.2% | 0.53% | 0.13% | 0.04% |
  | largest k in a register | 9.1% | 36.6% | 36.8% | 12.9% | 3.5% | 0.87% | 0.26% |
  | largest k over 3 registers | 3.5% | 13.5% | 40.9% | 29.0% | 9.6% | 2.6% | 0.8% |

  So every lane of a register has k < 4 in 95.4% of register-steps (96.3% with a large common factor), and k < 8 in 99.996%. When k < 4 the low byte of x is 1, 2, 4 or 8 and the other bytes are 0, so k = `vpshufb`(T4, x) in one op.
- **nbr and nbrd:** x = o AND −o, one `vptest` of x against 0xFFFFFFF0 per register-step, and a branch. On the fast path k = `vpshufb`(T4, x): 3 ops, 3 cycles deep, against the float trick's 5 ops and 8 cycles. The rare path is the full mask5 (nbr) or dbh (nbrd). The branch takes the rare path in 4.6% of register-steps.
- **bbr:** the same with k < 8 (99.996%), using two nibble tables (6 ops on the fast path).
- **tbr:** nbr tested on the thermometer andn(o, o − 1), whose low byte is 0, 1, 3 or 7 when k < 4. (e) The thermometer gives only comparisons k > j, not the bits of k, so it cannot replace x in the five tests.
- **(a) Stages:** mst5 uses each min result directly as a shift count (5 `vpsravd` stages per part, k never assembled). mst2 shifts by (8 | 16), then by (1 | 2 | 4).
- **(b) Depth:** mlo ORs the low-weight tests and the 8 and 16 tests separately, then together. All five tests come from x at the same depth (4 cycles), so no ordering shortens the chain below mask5's 7 plus the shift.
- **(d) Sharing:** there is one ctz per Gaussian number (o = hx OR hy already covers both parts), the three registers hold independent data, and the masks and weights are shared constants. Nothing else is left to share.
- **Correctness:** every fitted shift against o >> ctz(o) and −o >> ctz(o): every single-bit o in each of the 8 lane positions (other lanes 0), then every nonzero o in two passes (the second with lane 0 forced to a single bit, so both branches meet in one register). Then the gcd checks above: exhaustive K = 3 to 6, the −2^31 edges, and 2,621,440 random pairs. 0 failures.
- **A pitfall:** in a first build the fitted forms were macro aliases of ctz_mask5, which the kernel macros expanded before pasting, so all of them silently ran mask5 (or dbh) and timed the same. The fitted forms are now distinct functions, and objdump confirms the `vpshufb` and the branch in each loop.

| ns per gcd | pi_v3 random | pi_v3 large factor | pi_v random | pi_v large factor | dynamic instructions per gcd (pi_v3, random) |
|---|---|---|---|---|---|
| flt (float) | 38.0–38.1 | 20.3–20.4 | 57.0–57.3 | 29.0–29.2 | 299 |
| **nbrd** (k < 4 branch, dbh otherwise) | **36.3–36.4** | **19.4–19.5** | **52.1–52.5** | **26.8–26.9** | 305 |
| nbr (k < 4 branch, mask5 otherwise) | 36.9–37.1 | 19.8–20.1 | 53.1–53.4 | 27.0–27.2 | 316 |
| bbr (k < 8 branch) | 37.9–38.1 | 20.5–20.6 | 53.5–53.7 | 28.1–28.2 | 325 |
| dbh | 40.0–40.2 | 21.4–21.7 | 61.1–61.3 | 31.6–32.0 | 329 |
| tbr (thermometer, k < 4 branch) | 41.5 | 23.1–23.5 | 56.1 | 28.9–29.1 | 365 |
| mixn | 41.4–41.5 | 22.2–22.3 | 58.1–58.2 | 29.7–29.9 | 347 |
| mask5 (k, then `vpsravd`) | 42.0–42.2 | 22.5–22.6 | 59.7–59.9 | 30.9–31.1 | 367 |
| mlo | 42.3 | 22.6–22.7 | 59.3–59.5 | 30.6–30.7 | 360 |
| mst2 (2 stages) | 42.5–42.9 | 22.8–22.9 | 59.1–59.3 | 30.7–30.8 | 363 |
| pop (study 26) | 43.0–43.1 | 23.1–23.2 | 67.8–67.9 | 35.2–35.7 | 332 |
| mst5 (5 stages) | 44.9–45.0 | 24.0 | 61.7–62.1 | 32.5–32.6 | 375 |

- **Shifting in stages loses.** Each stage costs two `vpsravd` where assembling k costs one OR, and all five counts are ready at once, so nothing overlaps. mst5 is 7% slower than mask5 and mst2 1%.
- **Reordering for depth gains nothing** (mlo against mask5), as expected when every test starts from x at the same time.
- **The distribution is what pays.** On the fast path nbrd costs 3 ops, 3 cycles deep, plus a `vptest` and a branch off the dependency chain. It is the only integer form that beats the float trick: 4% faster in the 3-register gcd (36.3 against 38.0 ns), 9% faster in the latency-bound one-register gcd (52.3 against 57.2), and 4 to 8% faster with a large common factor. That is despite a few more instructions (305 against 299 per gcd) and the mispredictions on the 4.6% of register-steps that take the rare path.
- **The rare path matters little:** dbh beats mask5 there by 0.6 to 1 ns. A k < 8 threshold (bbr) is mispredicted almost never but doubles the fast path, and loses to nbr.
- **tbr was compiled badly** by gcc (365 instructions per gcd against nbr's 316), so it says little about the thermometer itself.
- The same branch would also speed up the float version. Its fast path does not use the converter, so the float trick has no remaining advantage.

**Summary.**
- **Best overall, with a branch:** nbrd. x = o AND −o; if every lane of the register has k < 4 (one `vptest`; 95% of register-steps, since half the lanes have k = 0 and 88% have k ≤ 1), then k = `vpshufb`(T4, x). Otherwise k comes from dbh. It takes 36.3 ns per random 31-bit pair in study 26's 3-register binary gcd, 4% faster than the float trick (38.0), and 52.3 against 57.2 ns with one register.
- **Best without a branch:** dbh. It multiplies each 16-bit word by the de Bruijn constant 0x09AF (`vpmullw`, 5 cycles), shifts each word right by 12, does one `vpshufb`, then shifts right by s = min_epu32(x AND 0xFFFF0000, 16) and ORs s back in. That is 9 ops, 40.0 to 40.4 ns, 5.5% (about 2 ns) above the float trick. Study 26's popcount and de Bruijn forms were 13% above it. dbm (`vpmaddwd` instead of `vpmullw`) ties.
- **The five single-bit tests:** 2 ops each with the min trick (AND, then min_epu32 with 2^j), the shortest chain of all but 16 ops, so 42 ns. Three of them plus one nibble table (mixn, 13 ops) tie the float trick in the latency-bound one-register gcd.
- **Fitting the tests into the step** (shift stages, depth order, the thermometer, sharing) gains nothing over assembling k. What pays is the distribution of k: a branch on the common case k < 4 replaces the whole ctz by one table lookup.
- **Ruled out by searches and counts:** a 32-entry index from `vpmaddwd`; an index read straight from the bits `vpshufb` uses; `vpermd` tables; byte-level lookups needing a horizontal sum; and leaving k out of the step (`vpsravd` needs the count, and multiplying by 2^−k needs a bit reverse and 4 `vpmuldq`).

**Addendum: nbrf, the float trick as the rare path.** This is nbr's `vptest` branch and `vpshufb`(T4, x) fast path, with flt in place of dbh on the 4.6% of register-steps where some lane has k ≥ 4. It is not integer-only.
- **Correctness:** checked as nbrd. Every single-bit o in all 8 lane positions and every nonzero o in two passes. For o = 2^31 alone, flt's count is 287, which `vpsravd` treats as 31, so the shift is still right. Then the gcd checks (exhaustive K = 3 to 6, the −2^31 edges, 2,621,440 random pairs): 0 failures.
- **Timing:** same harness, three runs, load below 1. Ranges of the medians, ns per gcd. Half the interquartile range was within 3% for the one-register runs and up to 7% for some three-register runs in this session.

| ns per gcd | pi_v3 random | pi_v3 large factor | pi_v random | pi_v large factor |
|---|---|---|---|---|
| flt | 38.1 | 20.4–20.8 | 57.2–57.7 | 29.1–29.4 |
| nbr (mask5 rare path) | 37.0–37.3 | 20.0–20.3 | 52.0–52.5 | 26.8–27.4 |
| nbrd (dbh rare path) | 36.3–36.9 | 19.5–19.6 | 52.0–52.6 | 26.8–27.1 |
| nbrf (flt rare path) | 36.3–36.5 | 19.4–19.7 | 50.7–51.1 | 26.1–26.5 |

- **With three registers, nbrf and nbrd tie** (36.3 to 36.9 ns). The rare path runs too seldom for its cost to show.
- **With one register (latency-bound), nbrf is about 1.3 ns (2.5%) faster than nbrd** on random pairs, and 0.6 ns with a large common factor. That is the float trick's shorter chain (8 cycles against dbh's 11) on the rare path.
- **So the float converter is worth at most 2.5% once the branch is in place,** against 5.5 to 13% when every step needs a full ctz. nbrd remains the best integer-only form.

## Study 28: NEON-shaped kernels on the x86 stand-in (neon28.c, portable-isa.md)

**What was done.** Decision 34 moves the build to ARM NEON and keeps this x86 machine as a stand-in: 128-bit registers (AVX-512VL), only operations NEON also has, no mask registers, no 64-bit lane multiply, no three-input logic, no floating point. portable-isa.md lists the shared operations (NEON intrinsic, x86 stand-in, op count), the NEON operations with no direct x86 equivalent and the substitute used for each, and the x86 operations ruled out because NEON lacks them. In neon28.c every vector operation is a macro named after its NEON intrinsic. Built with `-DCOUNT`, each macro also counts one NEON instruction, so NEON counts come from the code as written, whatever the stand-in costs on x86. gcc 13.3 -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq.

**Build traps found.**
- With AVX-512VL, gcc fuses two logic ops into `vpternlog` on its own. Every logic op now passes through an empty asm. `vpternlogq $0x55` (a one-input NOT, NEON's `mvn`) is allowed.
- When values spill into xmm16 to 31, gcc rewrites compares and byte blends into compares into mask registers and masked moves, because the VEX forms reach only xmm0 to 15. Compares and `bsl` are pinned to their VEX forms with inline asm. So on x86 they work on 16 registers, against NEON's 32.
- A scan of every kernel in the final binary finds no mask registers, ymm or zmm, other `vpternlog`, 64-bit multiplies or min/max, `vpopcntd`, `psignd`, `blendvps`, permutes or floating point.
- The gcd's selects now use full-lane masks (`vtst`, `bsl`). Studies 24 to 27 relied on `blendvps` reading only the sign bit, and on `psignd`; neither is in NEON.

**Timing harness** (all parts): same machine as studies 19 to 27 (Granite Rapids Xeon, family 6 model 173, 2-vCPU KVM guest). Load below 1 with nothing else running, pinned to one CPU, 31 repetitions, kernels interleaved with order rotated, an untimed call first, three runs. Ranges below are of the medians; quartile spreads were about ±1 to 3%.

### Part 1: add, subtract, multiply with the sticky flag (milestone 2)

Layout (b), int32 halves, study 15's full-range checks ORed into a sticky register and tested once per array.
- **add_xor, sub_xor:** study 15's test, (x XOR r) AND NOT (x XOR y) (`bic`) and (x XOR r) AND (x XOR y).
- **mul_w:** `smull`, `smlsl`, `smull2`, `smlsl2` per part pair, study 15's add form (v + 2^31 has nothing in bits 32 to 63), and `uzp1` to narrow.
- **mul_h:** the same, with the check as the high halves (`uzp2`) XOR the sign of the low halves.
- **Saturating forms** (NEON only; emulated here for correctness, not timed):
  - add_sat compares `sqadd` with the wrapped sum.
  - add_qc and mul_qc use NEON's sticky saturation bit QC in FPSR as the flag. add_qc is `sqadd` alone. mul_qc narrows with `sqxtn`, `sqxtn2` in place of `uzp1` and the check.
  - On overflow the operation is redone at the next size from the inputs (decision 13), so the saturated values are never used.

**Correctness.** Every kernel against exact arithmetic: the value mod 2^32 (for the saturating forms, only when nothing overflowed), and the flag exactly when some result does not fit.
- **Edges:** every quadruple of 32 edge values (study 20's set, k = 14, 15, 30, 31), each element alone among 8 with its position rotating: 1,048,576 quadruples.
- **Exhaustive in the top bits:** values (t << S) | f, with t every 8-bit two's complement pattern and f one of 5 fills (1,280 values). Every pair for the real parts, with the imaginary parts from cycling edge pairs, then the roles swapped; every 4th group of 8 is also run element by element. S = 24 for add and subtract (the carry into bit 31); S = 12 and 16 for multiply (products from 2^24 to 2^46). 3,276,800 pairs per S.
- **Random:** 200,000 arrays of 8 to 128 elements, four mixes, some with a planted large value.
- **Failures: none.** Three injected faults (offset 2^30, a check without the AND NOT, a check dropping the imaginary part) were all caught.

**NEON instructions per element** (from the counters; loads and stores 1.5 more in every row):

| | bare | xor check (study 15) | saturating compare | QC flag |
|---|---|---|---|---|
| add, subtract | 0.5 | 2.5 | 2.0 | 0.5 |
| multiply | 1.0 | 4.5 (add form or high-half form) | | 3.0 |

**x86 timing,** ns per element (one Gaussian integer). L1: 512 elements. L2: 16,384. Memory: 2^24 per array (384 MB in all, beyond L2, within the host's reported L3). x86 loop instructions per element: add bare 1.9, checked 3.9; multiply bare 3.5, mul_w 9.5, mul_h 10.

| | L1 | L2 | memory |
|---|---|---|---|
| add bare / checked | 0.142–0.144 / 0.235–0.242 | 0.253–0.262 / 0.270–0.273 | 1.336–1.352 / 1.402–1.433 |
| sub bare / checked | 0.142–0.145 / 0.240–0.246 | 0.250–0.253 / 0.253–0.259 | 1.332–1.350 / 1.397–1.441 |
| mul bare | 0.268–0.271 | 0.260–0.263 | 1.394–1.419 |
| mul_w / mul_h | 0.648–0.653 / 0.706–0.711 | 0.644–0.662 / 0.700–0.710 | 1.476–1.486 / 1.483–1.492 |

**What this shows.**
- **The checks' cost** follows study 20's pattern at half the width:
  - In L1 the check costs 66 to 70% on add and subtract, and 2.4 times on multiply.
  - From L2, add and subtract pay 1 to 7%. The multiply still pays 2.5 times, because at 128 bits it is still bound by instructions there.
  - From memory, every check costs about 5%.
- **The add-offset multiply check beats the high-half check** by 8% on x86 at the same NEON count; on NEON they tie by count.
- **NEON's QC bit is a hardware sticky flag.** With it, the checked add costs the same as the bare one (0.5 per element), and the checked multiply 3.0 against 4.5. The flag is read once per array (`mrs` FPSR, clear it at the start). Caveats, not testable here:
  - QC is shared by every saturating instruction in the sweep.
  - The FPSR access must not be reordered across the loop (an asm barrier).
  - Its cost on real cores is unmeasured.
  - It is the strongest NEON-specific result of this study and needs an ARM machine to confirm.

### Part 2: Montgomery against Barrett mod n (milestone 3)

Residues u32 in [0, n), separate arrays. Four kernels: P (x·y), S (an array times one shared residue), G (the Gaussian product of arrays), K (a Gaussian array times one shared Gaussian).
- **mon_w:** Montgomery with R = 2^32, widening. t in 64-bit lanes (`smull`, `smlsl`), m = lo(t)·n′ (`uzp1`, `mul`), t − mn (`smlsl`, `smlsl2`), high halves (`uzp2`), then add n where negative (unsigned min). Odd n.
- **mon_h:** Montgomery with the 32-bit high-half multiply: h = `sqdmulh`(a, b), m = (ab)·n′ (two `mul`, or one when b is shared and b·n′ is precomputed), t = `sqdmulh`(m, n), r = `shsub`(h, t) = (ab − mn)/2^32 in (−n, n). The halving is exact: ab ≡ mn mod 2^32, so 2ab and 2mn agree in their low 33 bits and h − t is even. The Gaussian forms reduce each of the four products separately, because a sum of products needs a carry correction that costs as much as it saves (worked out, not built).
- **bar_w:** study 19's lane Barrett in NEON shape, any n.
- **bar_h:** Barrett with the high-half multiply, no Montgomery form, any n parity.
  - P, for n ≤ 2^30: h = `sqdmulh`(a << (31 − L), b) = ⌊ab/2^L⌋, then q = `sqdmulh`(2h, μ₂) = ⌊h·μ₂/2^30⌋ with μ₂ = ⌊2^(L+30)/n⌋. That gives q_true − 3 ≤ q ≤ q_true, so r = ab − qn (`mul`, `mls`, mod 2^32) lies in [0, 4n), which fits in 32 bits for n ≤ 2^30. Then two unsigned-min steps.
  - S and K: Barrett multiplication by a shared k (Becker, Hwang, Kannwischer, Yang and Yang, 2021). Precompute k′ = round(k·2^31/n); then q = `sqrdmulh`(a, k′) and r = ak − qn (`mul`, `mls`) lies in (−n, n) for every n < 2^31.
  - bar_h30, K for n ≤ 2^30: the two quotients of each part are summed before one `mls`, r in (−2n, 2n).

**Correctness.** Every version against exact arithmetic. Montgomery outputs are checked in Montgomery form, and the conversions on their own.
- **Exhaustive:**
  - P and S for every n from 2 to 1,023 (odd n only for Montgomery; S over every shared constant). 357 million cases per Barrett kernel, 179 million per Montgomery kernel.
  - G and K for every n from 2 to 63 (K over every shared Gaussian). 206 and 107 million.
- **Random:** 4,194,304 cases per kernel at each of 17 n, from 65,536 to 2^31 − 1. Odd and even, prime and composite, on both sides of 2^30, with constants redrawn every 65,536 cases.
- **Failures: none.** Five injected faults were all caught: one range step dropped in bar_h P, bar_h30 allowed above 2^30, a plain halving in place of `shsub`, a floor in place of round for k′, and μ₂ one too large.

**Counts and timing.** Per 4 elements: NEON instructions (from the counters), of which multiplies (`mul`, `mls`, `smull`, `smlsl`, `sqdmulh`). Then x86 L1 timing, ns per element: 1,024 elements, 2,048 calls per sample, ranges over n = 2^31 − 1, 2,147,483,645, 1,073,741,823 and 1,000,003 and three runs. bar_h P and bar_h30 ran only at the two n ≤ 2^30.

| per 4 elements; ns per element | P | S | G | K |
|---|---|---|---|---|
| divide (scalar) | 2.58–2.61 ns | | 5.17–5.20 ns | |
| mon_w | 9 (5 mul); 0.432–0.442 | 9 (5); 0.408–0.416 | 22 (14); 1.019–1.034 | 22 (14); 0.987–0.998 |
| mon_h | 7 (4); 0.654–0.666 | 6 (3); 0.563–0.574 | 34 (16); 2.72–2.75 | 30 (12); 2.36–2.38 |
| bar_w | 19 (6); 0.744–0.755 | 19 (6); 0.733–0.750 | 44 (16); 1.75–1.77 | 44 (16); 1.66–1.68 |
| bar_h | 10 (4), n ≤ 2^30; 0.644–0.651 | 5 (3); 0.373–0.380 | | 26 (12); 1.57–1.59 |
| bar_h30 (n ≤ 2^30) | | | | 20 (10); 1.365–1.381 |

Conversions, per 4 residues: into Montgomery form 9 (mon_w) or 6 (mon_h) NEON instructions, 0.406–0.413 and 0.563–0.573 ns; out 7 or 5, 0.260–0.266 and 0.387–0.401 ns. The memory-sized pass (2^22 elements per array) was erratic: the same kernel moved by up to 25% between runs, and S was slower than P with less data. Its figures are not used, as with study 19's scalar large-array figures.

**Uncertain rankings.** On x86 the high-half rows (mon_h, bar_h) pay for the emulation:
- `sqdmulh` and `sqrdmulh` take about 8 x86 instructions each (two `pmuldq`, shifts, a blend).
- `shsub` takes 5.
- `mul` and `mls` are `pmulld`: 2 µops and 10 cycles here, against one NEON instruction.
So x86 ranks every high-half form lower than its NEON count does. Where the two disagree (P: mon_h 7 against mon_w 9 by count, 0.66 against 0.43 ns here), the NEON count is the better guide, and the ranking is marked uncertain. Where they agree (G and K's mon_w over mon_h, S's bar_h first) the ranking is firm.

**What this shows.**
- **Array by array (P, G): Montgomery stays.** For G, widening Montgomery is half of every other form by count (22 against 34 and 44) and fastest here (1.02 ns, 5 times the divide). Study 19's ranking holds in NEON shape. For P, high-half Montgomery may be cheapest on NEON (7 against 9, uncertain).
- **Barrett becomes competitive where one factor is shared.**
  - Barrett multiplication by `sqrdmulh` with a precomputed k′ is the cheapest S on every measure: 5 NEON instructions against 6 and 9, and 0.375 ns against 0.41 here. It also takes even n and needs no conversion.
  - For a shared Gaussian (K), bar_h30 is cheapest by count at n ≤ 2^30 (20 against 22). At any n < 2^31, bar_h is 26 against mon_w's 22.
  - Counting conversions, a single K pass costs mon_w 22 + 18 in + 14 out = 54 against bar_h's 26. Montgomery repays its conversions only after about 8 chained K operations by NEON count (about 2 to 3 by x86 time).
- **High-half Barrett for array by array (bar_h P)** needs n ≤ 2^30 and costs 10 against 9. Not worth a special case.
- **Widening Barrett (bar_w) is never competitive:** its error of up to 3 forces a 64-bit correction (19 against 9 for P).
- **Multiplies:** if a core issues fewer vector multiplies per cycle than other vector instructions, the multiply column matters. There mon_h and bar_h use 3 to 4 per 4 products against mon_w's 5, and K's bar_h30 10 against 14. Not measurable here.

### Part 3: the binary gcd with clz, interleaved (milestone 4)

Study 26 and 27's (1+i)-ary gcd at 128 bits, 4 pairs per register, NEON-shaped:
- `vtst` and `bsl` with full-lane masks;
- conditional negation as `bsl`(m, −x, x);
- right shifts by a lane count (NEON's `sshl` by the negated count, produced at the same cost);
- the −2^31 guard and the scalar fixup unchanged.

Three trailing-zero forms:
- **clz:** k = 31 − clz(x), x = o AND −o, by `vclzq_u32` (`vplzcntd` here). 4 instructions, no branch. NEON's count −k = clz − 31 is the same subtraction.
- **nbrd:** study 27's best. If every lane has k < 4 (one any-lane test), k = `tbl`(T4, x); otherwise dbh, the 16-bit de Bruijn multiply.
- **nbrc:** nbrd's branch with clz as the rare path.

R = 1, 2, 3, 4, 6 or 8 registers stepped together until all are done; the tail, one register at a time.

**Correctness.**
- **The fitted shifts:** every nonzero o, in two passes (the second forces lane 0 to a single bit, so that one register mixes small and large k and both branches meet). The setup's ctz values over every nonzero o.
- **The 18 kernels against study 26's __int128 reference:**
  - exhaustive K = 3 to 6 (16,777,216 pairs at K = 6);
  - the 160,000 edge pairs with −2^31, each alone: 29,679 guarded and 87 overflowing gcds, as in study 26;
  - 200,000 random −2^31 pairs alone;
  - 2,621,300 random pairs of the five kinds, in batches whose lengths are not multiples of 4R.
- **Failures: none.**
- **Injected faults:**
  - a wrong clz, a broken strip mask and a dropped sticky flag were all caught;
  - a wrong de Bruijn constant was caught by the shift check, after which the gcd never ends, as in study 27;
  - removing only the guard's zeroing is not caught, because the fixup overwrites every guarded pair.

**NEON instructions per gcd** (counters; the any-lane test counted as 2), and register-steps per register of 4:

| R | 1 | 2 | 3 | 4 | 6 | 8 |
|---|---|---|---|---|---|---|
| clz, random | 438 | 448 | 453 | 457 | 460 | 465 |
| nbrd, random | 451 | 461 | 466 | 470 | 473 | 477 |
| nbrc, random | 447 | 457 | 462 | 466 | 469 | 474 |
| register-steps, random | 33.2 | 34.4 | 34.9 | 35.3 | 35.6 | 36.0 |
| clz / nbrd / nbrc, large common factor | 231 / 238 / 236 | 238 / 246 / 243 | 242 / 250 / 247 | 245 / 253 / 250 | 250 / 258 / 255 | 252 / 260 / 257 |

**x86 timing,** ns per gcd, 1,056 pairs (25 KB, in L1). Medians of three runs, which agreed within 0.6%:

| ns per gcd | R = 1 | 2 | 3 | 4 | 6 | 8 |
|---|---|---|---|---|---|---|
| clz, random | 117.2–117.5 | 87.5–87.9 | 83.1–83.5 | 83.0–83.4 | 83.8–84.5 | 84.6–84.9 |
| nbrd, random | 109.2–110.1 | 82.1–82.3 | 79.3–79.5 | 80.6–81.1 | 80.8–81.2 | 82.3–82.8 |
| nbrc, random | 107.9–108.5 | 81.8–82.2 | 78.6–79.3 | 80.0–80.5 | 81.0–81.3 | 81.4–82.0 |
| clz, large factor | 59.8–59.9 | 46.5–46.7 | 44.7–44.8 | 44.3–44.5 | 45.4–45.6 | 45.9–46.1 |
| nbrd, large factor | 55.8–56.1 | 43.2–43.7 | 42.7–42.9 | 43.0–43.2 | 43.8–44.1 | 44.6–44.9 |
| nbrc, large factor | 54.9–55.3 | 43.1–43.4 | 42.2–42.4 | 42.5–42.8 | 44.0–44.1 | 43.9–44.3 |

Stack operands per kernel (spills) grow with R on x86: 2 at R = 1, 33 at R = 3, 51 at R = 4, 97 at R = 6, 157 at R = 8 (clz).

**What this shows.**
- **clz against the branch: the stand-in and the count disagree.**
  - Here, clz without a branch is 4 to 5% slower than nbrd and nbrc at their best R: 83.1 against 79.3 and 78.6 to 79.3 ns random; 44.3 against 42.7 and 42.2 with a large factor.
  - By NEON count, clz is 2.7% leaner (453 against 466 per gcd at R = 3). Its 4 instructions replace a test, a branch and a lookup on most steps, plus dbh on the rest (the share of fast-path steps at 4 lanes was not measured; with 4 lanes per register it should exceed study 27's 95% at 8). NEON's any-lane test (`umaxv`, then a move to a general register, then a compare) is dearer than `ptest`, which this count understates.
  - So decision 35's choice (clz, no branch) is supported by count and by NEON's costlier branch test, and contradicted by x86 time. Ranking uncertain until timed on ARM.
  - nbrc (clz on the rare path) is never worse than nbrd and is simpler; if a branch is kept, nbrc is the form.
- **More registers stop paying at 3 or 4.** Every form is best at R = 3 (clz ties R = 3 and 4); R = 6 and 8 are 1 to 4% slower. Two reasons:
  - With all R registers stepping until the last one finishes, lanes wait: register-steps per register rise from 33.2 at R = 1 to 34.9 at R = 3 and 36.0 at R = 8.
  - Past R = 2 the x86 build spills. NEON's 32 registers, all reachable by `cmeq` and `bsl`, would spill less: about 5 live values per register plus 6 or 7 constants and temporaries fits R = 3 or 4 in 32 registers, not 6 or 8.
  - So 6 and 8 cannot win: the lane waiting alone costs 2 to 3% and the step is already throughput-bound at R = 3. On NEON, 3 or 4.
- **R = 1 to R = 3** saves 29% (latency-bound to throughput-bound), as at 256 bits.
- **Against study 27 at 256 bits** (nbrd, R = 3, 36.3 ns), the 128-bit gcd takes 2.2 times as long: twice for half the lanes, plus about 10% not attributed. The NEON-shaped selects are a likely part of it: the stand-in pays 3 instructions for `vtst`, against 1 on NEON.

**Summary.**
- **Overflow checks:** NEON-shaped, they cost what they did at 256 bits relative to the bare loop: free from memory, about 1.7 times (add) and 2.4 times (multiply) in L1. NEON's QC saturation bit can serve as the sticky flag at no instruction cost (add 0.5, multiply 3.0 per element against 2.5 and 4.5); this needs an ARM machine to confirm.
- **Mod n:** widening Montgomery stays the choice for array by array (G: 22 NEON instructions per 4, against 44 for Barrett). For a shared multiplier, Barrett multiplication by `sqrdmulh` is competitive and needs no conversions:
  - S: 5 against 6 and 9.
  - K: 20 at n ≤ 2^30, 26 otherwise, against 22 plus conversions.
  - A candidate amendment to decisions 19 and 24: Barrett for array-by-constant products.
- **gcd:** best at 3 registers (4 ties). clz without a branch is leaner by NEON count but 5% slower on the stand-in; uncertain until timed on ARM. If a branch is kept, nbrc.
- **Build:** the stand-in needs guarding. gcc reintroduces three-input logic and mask registers on its own; asm barriers and a scan of the binary keep the code inside the portable list.

## Study 29: studies 20, 21, 23 and 25 in NEON shape (neon29isa.c, neon29a64.c, neon29div.c, neon29mul.c, neon29shr.c)

**What was done.** Studies 20, 21, 23 and 25 were built for AVX2 and use instructions NEON lacks: immediate blends, the sign-bit `blendv` select, `vpmaddwd` and `vpmaddubsw`, `movemask`, x86's even-lane widening multiply, `ptest`, `vpermd`, and scalar fallbacks where AVX2 has no 64-bit multiply. Each kernel was ported to 128-bit NEON shape on study 28's stand-in (decision 34). This is not a redesign: each port starts from the study's own code and findings, and only what NEON lacks is replaced, with the best NEON-shaped substitute from portable-isa.md. Where a substitute had several reasonable forms, they were compared.

**Build** (study 28's approach, kept):
- **Macros:** every vector operation is a macro named after its NEON intrinsic, and its body is the x86 stand-in. The layer is neon29isa.c: study 28's layer plus 64-bit `abs`, `cmhs`, `usra`/`ssra`, `usubw`, `shll`, `shrn` pairs, `addhn` pairs and the 64-bit shift by a signed lane count.
- **Barriers:** empty asm barriers against `vpternlog` fusion, and compares and `bsl` pinned to their VEX forms.
- **Counts:** a `-DCOUNT` build counts NEON instructions (and multiplies) from the code as written.
- **Scan:** each binary disassembles itself (`./prog scan`) and checks every `k_*` kernel against the forbidden list.
- **Compiler:** gcc 13.3 -O2 -mavx512f -mavx512vl -mavx512cd -mavx512bw -mavx512dq (-mavx512bitalg for `cnt`).

**New: the same kernels compiled for AArch64.** neon29a64.c maps the macro layer onto `arm_neon.h`, and every kernel file compiles with clang-18 --target=aarch64-linux-gnu -ffreestanding -O2 -DA64 -S. There is no AArch64 machine or emulator here, so the code is counted, not run. A script counts each kernel's main loop: instructions, vector instructions, multiplies, register copies and stack spills, with NEON's 32 registers. This checks the `-DCOUNT` figures against real target code, and shows what 32 registers do.

**Lane order.** NEON's `smull` takes lanes 0 and 1 and `smull2` lanes 2 and 3; the stand-in takes even and odd lanes. Every macro keeps one convention, so the same kernel source is correct on both. Data moved through memory between sizes (study 21's widen and narrow) uses natural-order stand-ins.

**Machine and timing** (all parts): the same machine as studies 19 to 28 (Xeon family 6 model 173, 2-vCPU KVM guest). Load was below 1.2 with nothing else running. Runs were pinned to one CPU, with 31 repetitions, kernels interleaved and their order rotated, and an untimed call first. Ranges are of the medians over 3 runs, except division from memory (1 run) and study 21's runs (2); quartile spreads were about ±1 to 3%. Timings are read as rankings. This guest has no hardware counters and valgrind does not run AVX-512, so x86 instruction counts per element are static counts of each kernel's main loop.

### Part 1: division (study 25; neon29div.c, milestone 1)

Study 25's problem and methods at 128 bits: 4 elements per register, each part's 64-bit values as a pair of registers. m3 (shift and subtract) is not ported, since decision 32 dropped it. The tie rules are up, down, toward zero and away from zero, each in the direct and the mask form; even, odd and coupled are not repeated.

**What NEON lacks, and the substitutes** (AVX2 form in brackets):
- **Front end:** `smull` + `smlal`/`smlsl` per pair [`vpmuldq` on even lanes, shifts for the odd lanes, add].
- **Zero divisor:** fz = `umin`(fz, c | d), then c −= (c | d = 0) [64-bit compare, and, or per pair].
- **Mask form:**
  - |u| = `abs` in 64-bit lanes. It is correct at the wrapped 2^63, which reads as 2^63 unsigned.
  - The 32-bit sign masks come from the high halves: mX = [hi32(X − 1) < 0], mY = [hi32(Y) < 0] (`uzp2`, `cmlt`). X = 0 counts as negative, which is harmless (q = 0, r = x).
  - So toward and away from zero need no 64-bit mask at all. Ties up and down widen the mask (`sxtl`) for their thresholds.
  - [64-bit compare, xor, subtract, blend]
- **Direct form's floor remainder:** ρ = `bit`(R, N − 1 − R, s) [(R xor s) + (N and s)].
- **Packing:** `uzp1`/`uzp2` [`vpblendd` with an immediate].
- **Flags:** a running `smin` of the output parts (some part is −2^31 iff the minimum is), tested once per array [compare, or, `movemask` per register]. `cmeq` + `orr` would cost 2 per part against 1.
- **Remainder:** r = x − qy by `mla`/`mls`, 2 per part [4 `vpmulld` and adds].
- **m2:**
  - ctz = 31 − `clz`(o AND −o), 4 instructions; NEON shifts by the negative count at the same cost. The popcount form (`cnt`, two `uaddlp`) is 6.
  - The 1+i test by `cmtst`, 2 instructions against 3 for the shift pair.
  - Newton steps by `mls` + `mul` [`mullo`, sub, `mullo`].
  - [popcount by `vpshufb`, `vpmaddubsw`, `vpmaddwd`]
- **m4 estimate:** S = U′ + hi(U′)·m′ + (lo(U′)·m′ >> 32) by `uzp1`, `uzp2`, `umlal`, `umull`, `usra`: 8 per 4 values. Narrowing each register with `xtn` and `shrn` instead is 10.
- **Correction:**
  - ρ = U − q̂N by `umlsl` on the low 32 bits of N, then q̂·hi(N) by `mul`, `shll` and a subtract.
  - ρ ≥ N by `cmhs`.
  - [two `vpmuludq`, a shift, a bias xor, `pcmpgtq`]
- **m5 normalisation:** the 64-bit clz from the two 32-bit ones (`clz`, `uzp`, `cmeq`, `and`, `add`), then `ushl`: 12 instructions per 4 elements [a 6-step binary search, 30 per register of 2].
- **m5 reciprocal:**
  - Study 25's Newton steps, on dh packed in 32-bit lanes (`umlsl`, `usubw`, `shrn`, `umull`, `uzp2`).
  - Every value that fits 32 bits (Y, f, v, e_h) stays packed. The 2^32·dh term of e_h becomes a 32-bit subtract on the high halves.
- **m5 scaling:** th = `ushl`(U, sh − 32), one instruction with a signed count [`vpsllvq`, `vpsrlvq`, or].

**Correctness** (study 25's harness):
- Exhaustive K = 3 to 6, both kinds, with every shared divisor at each size.
- Every quadruple of the 20 edges (−2^31 included), each element alone in a register of 4, plus exact edge products and edges over divisors in [−4, 4].
- 10,485,760 random divisions with remainder and 10,331,021 exact ones, and 40 shared divisors × 65,536.
- **Total:** 1,937,980,193 element results, failures none. Overflow flags and `resolve` were exact.
- **The packed reciprocal,** over all 2^31 values of dh: short by 0 (1,162,476,704 values), 1 (896,286,121) or 2 (88,720,823) after Newton, as in study 25, and exact after the two fix-ups.
- **Injected faults,** all caught except one: dropping the m4/m5 correction; dropping the 1+i step on x; the plain sign of X in the direct form (only the all −2^31 corner, 2 failures per kernel); the mask form's sign of X without the −1 (the same corner); m5's scaling by sh − 31; toward zero with away's threshold. The exception, as in study 25, is dropping one reciprocal fix-up: only the exhaustive reciprocal run catches it (88,720,823 values wrong).

**Scan and compiled code:**
- **x86:** no forbidden instruction in 35 kernels.
  - gcc builds the all-ones constant in xmm16 to 31 with `vpternlogd $0xff` (NEON `movi`), allowed and counted.
  - Spills in the main loop: m2 none, m4 up to 25 stack references, m5 7 to 21, m5x 42 to 102, m5x3 93 to 113.
- **AArch64 (clang):**
  - m4 and m5 at one register spill 0 to 4 times per loop. m5x spills 16 to 43 times per 8 elements and m5x3 67 to 75 per 12: two interleaved registers already exceed NEON's 32.
  - Destructive multiply-accumulates add register copies (`mov v.16b`, 9 to 18 per m5x loop; not in the counts).
  - clang fuses shift and add into `ssra`/`usra`, and in m2s it writes `mls` as `neg` + `mla` (+1 per register).
  - Compiled vector instructions per element against the counted ones: m2 14.25 against 13.0, m4 22.5 against 21.0, m5 47.0 against 45.25.

**NEON instructions per element** (counters, random inputs; loads and stores 1.5 to 2 more), and x86 main-loop instructions per element:

| | m2 | m2p | m2s | m4 floor | m5 / m5x floor | m4 toward 0 (mask) | m5x toward 0 (mask) |
|---|---|---|---|---|---|---|---|
| NEON (multiplies) | 13.0 (3.5) | 13.5 | 2.0 (1.0) | 21.0 (6.5) | 45.25 (12.5) | 24.0 | 48.25 |
| AArch64 loop, all | 16.5 | 17.0 | 5.5 | 28.75 | 50.25 / 53.75 | 33.75 | 59.6 |
| x86 loop | 17.5 | 19.0 | 4.25 | 36.25 | 77.75 / 79.6 | 39.0 | 84.1 |

The scalar control on AArch64 is 37 instructions per element for the floor core, with two `udiv` (31 exact, 29 shared), against 71 on x86.

**Timing,** ns per element. L1: 1,024 elements, 3 runs. Memory: 2^24 elements, 1 run.

| | L1 | memory |
|---|---|---|
| exact: m1 / m2 / m2p | 5.17–5.19 / 2.68–2.71 / 2.89–2.91 | 5.46 / 3.07 / 3.38 |
| exact, shared: m1 / m2s / m4 | 5.17–5.21 / 0.39 / 3.22–3.24 | 5.27 / 1.02 / 3.65 |
| floor, shared: m1 / m4 (random divisor) | 5.17–5.25 / 3.75–3.80 | 5.34 / 3.94 |
| floor, shared, small divisor: m1 / m4 | 5.18–5.24 / 3.73–3.77 | 5.38 / 3.97 |
| floor, varying: m1 / m5 / m5x / m5x3 | 5.19–5.20 / 11.24–11.27 / 8.58–8.62 / 8.61–8.62 | 5.39 / 11.50 / 8.82 / 8.77 |
| floor, varying, small divisor: m1 / m5x | 5.20–5.30 / 8.60–8.62 | 5.43 / 8.82 |

**Rankings against study 25:**
- **Exact division, m2 against the divide: holds.** 1.9 times faster in L1 (was 3.2) and 1.8 from memory. With a shared divisor m2s is 0.39 ns, 13 times the divide (was 25), and 1.02 from memory. By count m2 is 13 NEON instructions per element against the scalar's 31 with two divides. ctz by clz beats the popcount form, by 0.5 instructions and 0.2 ns.
- **Shared divisor, m4 against the divide: holds,** at 1.38 times in L1 (was 3.0) and 1.36 from memory: 21 NEON instructions per element against 29 scalar with two divides. Two things shrink the margin here: half the lanes, and every 64-bit correction step covering 2 values per instruction.
- **Varying divisor, m5x against the divide: flips on x86, uncertain.** 8.6 ns against 5.2 (AVX2 had m5x at 4.45, 1.17 times faster).
  - The stand-in spends 80 x86 instructions per element on 47 NEON ones (1.7 per NEON instruction, 12.5 of them multiplies).
  - The scalar divide is 71 x86 instructions, or 37 AArch64 instructions with two 64-bit `udiv`.
  - On ARM the comparison is 48 vector instructions against two divides plus 35 scalar; which wins depends on the core's divide throughput and vector width. Reopen on ARM.
- **Interleaving:** two registers cut m5 from 11.24 to 8.6 ns (−24%); three tie with two and spill twice as much here, and on AArch64 too. Two stays the form.

**The four tie rules, each in its two forms** (ns per element, L1; NEON instructions per element in brackets):

| | floor | up: direct / mask | down: direct / mask | toward 0: direct / mask | away from 0: direct / mask |
|---|---|---|---|---|---|
| m4 | 3.75–3.77 [21.0] | 4.99–5.02 / 4.35–4.38 [26.5 / 27.0] | 4.98–5.01 / 4.38–4.40 [26.5 / 26.5] | 5.08–5.10 / 4.17–4.18 [28.0 / 24.0] | 5.09–5.11 / 4.12–4.14 [28.0 / 24.5] |
| m5x | 8.60–8.62 [45.25] | 9.74–9.77 / 9.43–9.52 [50.75 / 51.25] | 9.76–9.79 / 9.33–9.40 [50.75 / 50.75] | 9.87–9.91 / 9.10–9.16 [52.25 / 48.25] | 9.90–9.96 / 9.12–9.17 [52.25 / 48.75] |
| m1 (scalar) | 5.19–5.21 | 6.85–6.89 / 6.69–6.72 | 6.88–6.91 / 6.60–6.63 | 6.69–6.74 / 6.91–6.94 | 7.49–7.51 / 6.53–6.55 |

From memory the order is the same (m4: 5.15 / 4.48, 5.16 / 4.54, 5.21 / 4.30, 5.24 / 4.26; m5x: 9.93 / 9.57, 9.92 / 9.59, 10.09 / 9.29, 10.11 / 9.35).

- **Toward and away from zero: mask, as before,** now by 4 NEON instructions per element (study 25: 2.4 to 3.0) and 0.7 to 1.0 ns. The NEON mask form needs neither the 64-bit sign mask nor the floor-remainder fix.
- **Ties up and ties down: a tie, as before, by NEON count** (direct 0.5 lower for up, equal for down). x86 time favours the mask form by 0.2 to 0.67 ns, because the direct form's floor remainder is `bsl`, which is `vpblendvb` here (1 to 3 µops against NEON's 1). That ranking is uncertain; by count it is a tie.
- **The best forms against each other:** toward zero (mask) 24.0 and away from zero (mask) 24.5 are cheapest for m4, against 26.5 for up and down; for m5x, 48.25 and 48.75 against 50.75. In time, toward and away (4.12 to 4.18, 9.10 to 9.17) are ahead of up and down in either form (4.35 to 5.02, 9.33 to 9.79).
- **Decision 28 (toward zero, mask form) holds,** with a wider margin: 2.5 NEON instructions per element cheaper than ties up (study 25: 1.1 for m4), because the mask form loses its 64-bit sign work on NEON.
- **The scalar control** is as fragile under gcc as before (study 25); its rule timings are not used for rankings.

### Part 2: the int64-half multiply (study 23; neon29mul.c, milestone 2)

Study 23's kernels at 128 bits: 4 elements per pass, as two registers per array. NEON, like AVX2, has no 64-bit lane multiply, only the widening 32 × 32 → 64 and the 32-bit low multiply.

**What NEON lacks, and the substitutes:**
- **Pieces:**
  - lo(x) and hi(x) of two registers by `uzp1`/`uzp2`, packed in 4 lanes: 2 per 4 values, the same as x86's shift per register. Narrowing each register instead (`xtn`, `shrn`) is 4.
  - Signed high pieces by `addhn`/`addhn2` (x + 2^31, high half, narrowed): 2 per 4 values.
- **Products:** `smull`/`smull2` with `smlal`/`smlsl` fuse each column's two products and their sum or difference [two multiplies and an add].
- **Splits:**
  - re's columns are differences inside int64, so floor(m / 2^32) is one `ssra` (NEON has the 64-bit arithmetic shift) and lo(m) one `and`.
  - im's columns can reach 2^63 and keep study 23's offsets, then `usra`.
  - [xor with 2^63, logical shift, blend]
- **Unsigned borrows and carries:** `cmhi`, one instruction [bias xor twice, `pcmpgtq`].
- **The bare multiply's 2^32 column** needs only 32 bits: `mul`, `mla`, `mls` on packed lanes (4 per 4 elements), then `shll` and add. k_barew keeps study 23's widening form for comparison.
- **Flag:** the k test ORed and tested once per array, as before.

**Correctness** (study 23's harness):
- Every quadruple of the 37 edges, each element alone in a pass of 4 with its position rotating; the all −2^63 corner.
- 4,000,000 random single elements, half aimed at the int64 boundary (67,471 within 4 of it fit, 644,601 do not), and 200,000 random arrays.
- **Total:** 42,519,134 kernel calls, failures none. The flag counts (clear 474,196, set 5,599,966) and the calls settled by the bound (304,985) are study 23's exactly. The 16-register build (below) passes too.
- **Injected faults,** all caught: dropping k_sgn's e_a correction terms, the wrong im offset, dropping k_uns's borrow, a logical split of re's column, `addhn` without the 2^31 offset, and a missing term in the bare middle column.

**Counts and compiled code** (per element; NEON from the counters, plus 3 loads and stores; AArch64 and x86 main loops):

| | s_bare | s_chk | k_bare | k_barew | k_sgn | k_uns | k_bnd (bound holds) |
|---|---|---|---|---|---|---|---|
| NEON (multiplies) | | | 8 (4) | 10 (6) | 34.5 (8) | 43.5 (8) | 14 (4) |
| AArch64 loop, all | 13 | 24 | 11.75 | 13.75 | 46.0 | 47.25 | 17.75 |
| AArch64 spills per 4 elements | 0 | 0 | 0 | 0 | 21 | 11 | 0 |
| x86 loop | 17 | 38 | 16.25 | 21.5 | 61.0 | 65.25 | 24.75 |

- **Scalar on AArch64:**
  - `__int128` products are `mul` + `smulh`: s_chk is 4 `mul`, 4 `smulh`, `subs`/`sbc`, `adds`/`adc`, and `eor` with an `asr #63` operand for each check, with no spills (x86 needs the `rdx:rax` pair and spills, study 20).
  - It uses 8 multiply instructions per element, the same as k_sgn, whose `smull` gives 2 products per instruction.
- **The compiled AArch64 code** follows the counts. Two exceptions: clang turns `addhn`(x, 2^31) into `raddhn`(x, 0), saving a constant (an improvement); and k_sgn still spills 21 times per 4 elements with 32 registers.

**Timing,** ns per element. L1 512 elements, L2 16,384, memory 2^25 (256 MB per array). 3 runs; L2 within 0.01 to 0.04 of L1 for every kernel.

| | s_bare | s_chk | k_bare | k_barew | k_sgn | k_uns | k_bnd (bound holds) |
|---|---|---|---|---|---|---|---|
| L1 | 1.04–1.06 | 2.09–2.15 | 1.47–1.50 | 1.68–1.72 | 4.32–4.39 | 4.75–4.82 | 2.04–2.07 |
| memory | 2.68–2.76 | 3.59–3.67 | 3.01–3.08 | 3.07–3.15 | 5.06–5.18 | 5.45–5.56 | 3.23–3.29 |

**Rankings against study 23:**
- **Signed against unsigned pieces: holds.** k_sgn is 9 to 10% faster in L1 and 7 to 8% from memory, 34.5 against 43.5 NEON instructions. The margin narrows on AArch64 (46 against 47.25), where k_uns spills less.
- **Checked, vector against scalar: flips, and both measures agree.**
  - Study 23 had v_sgn level with the scalar check (2.01 against 2.09 ns).
  - Here k_sgn takes 4.32 to 4.39 against 2.09 to 2.15, 2.1 times slower, and 1.4 times slower from memory.
  - On AArch64 it is 46 instructions against 24, with the same 8 multiplies.
  - At 128 bits each 64-bit lane op covers 2 values, so the vector check costs 37.5 NEON instructions per element against AVX2's 27.5 per element at 4 lanes. The fused forms (`smlal`, `ssra`, `cmhi`) do not make that up.
- **Bare, vector against scalar: from 1.6 times faster to a tie, uncertain.**
  - x86 has scalar ahead (1.04 against 1.47), but the vector's 2^32 column is `pmulld` (2 µops, 10 cycles here).
  - AArch64 has 11.75 against 13.
- **The bound with fallback: from 2.3 times faster to a small win, uncertain.** It ties the scalar check on x86 (2.04 against 2.10) and is 17.75 against 24 on AArch64 when the bound holds. When the bound fails it costs its sweep plus the fallback, so the fallback should be the scalar check.
- **The bare middle column in 32 bits** beats the widening form: 8 against 10 by count, and 1.47 against 1.69 ns even with `pmulld`.

**Registers** (decision 35 assumed 32 registers make the vector multiply a tie or a win):
- On x86 the build was repeated with -ffixed-xmm16 … -ffixed-xmm31, so every instruction has 16 registers.
- k_sgn's main loop went from 39 stack references to 73, and k_uns's from 47 to 113.
- Time rose 2 to 5% for k_sgn (L1 4.45 to 4.59) and 1 to 6% for k_uns (4.82 to 5.13), and 2 to 3% from memory. The bare kernels did not change.
- On AArch64 even 32 registers leave k_sgn with 21 spills per pass of 4. The register count is a small effect and moves no ranking.

**What this shows.** On NEON the checked int64-half multiply is best scalar: 24 AArch64 instructions per element with `smulh`, no spills, against 46 for the vector check. The vector forms do not repay their 32-bit pieces at 2 lanes per 64-bit instruction. Signed pieces stay better than unsigned ones. The bare product and the bound are roughly level with scalar, which is uncertain until timed on ARM. Decision 35's "a tie or a win, not retested" for the grown-size multiply does not hold for the checked multiply.

### Part 3: the shrink test and its whole-run savings (studies 20 and 21; neon29shr.c, milestone 3)

Studies 20 and 21's kernels, variants (bare, v1 overflow check only, v2 naive pass, v3 refined fused test, v3A rule A) and workloads (back, control, hover), unchanged in design. 8 int32 or 4 int64 elements per pass.

**What NEON lacks, and the substitutes:**
- **End-of-array test:** `umaxv` + move + compare, once per array [`ptest`].
- **int32 multiply:** study 28's `smull`/`smlsl` pairs and `uzp1` [even/odd `vpmuldq`, blend]. The wrapping multiply is `mul`, `mls`/`mla`.
- **int64 multiply:** AVX2 used scalar code, and NEON has no 64-bit lane multiply either. Following part 2, the scalar form (on ARM `mul` + `smulh`) is the main case. The vector form (part 2's k_sgn) is timed beside it.
- **Promotion:** `sxtl`, `sxtl2` [`vpmovsxdq`].
- **Demotion:** `uzp1` of two registers, one instruction per 4 values [two `vpermd` and a `vperm2i128` per 8].
- **The int32 operand at int64:** `sxtl`, `sxtl2` on load.
- **The shrink test itself is unchanged:** an add and an `orr` per register, with the mask applied once per array.
  - The NEON alternatives considered do not fit. `cls` has 32-bit lanes only. Min/max tracking needs 64-bit min and max, which NEON lacks. A saturating narrow sets QC, which decision 37 wants for the overflow flag, and it tests 32 bits, not 31.
  - So the test costs the same instructions as in AVX2, at half the lanes.

**Correctness:**
- **Study 20's harness:** every quadruple of 32 edges at each width, each element alone among 8, and 400,000 random arrays. 28,717,248 kernel calls, failures none. Every outcome (fits, does not fit, overflow with the shrink bit clear or set) occurs at least 4,400 times per checked kernel.
- **Study 21's exact `__int128` replays:** every workload, L and variant, at N = 1,024 (every element) and 2^20 (every 16th), with both int64 multiplies. Failures none.
- **Size changes as designed:** back promotes and demotes once (v2, v3); control promotes once and never demotes; hover's v3A promotes and demotes 65 times at L = 128.
- **Injected faults,** all caught: the fused test with rule A's offset; the pass skipping the imaginary part; demotion taking the high halves (caught by the replay only); the int64 add check without its AND NOT.

**Counts** (NEON per element from the counters, loads and stores apart; AArch64 loops beside):
- **int32:** add v1 2.5 (bare 0.5), and the naive pass +1.0 plus 0.5 loads. Multiply v1 4.5 (bare 1.0).
- **int64:** add v1 5.0, and the fused test +2.0 with no memory (AArch64 loop 35 → 43 per 4 elements). The naive pass is +2.0 plus 1 load per element.
- **Copies, AArch64:** demotion 2.75 instructions per element, promotion 3.25.
- **int64 × int32 multiply, AArch64:** scalar 24, vector 39.75 with 16 spills.

**Study 20 timing,** ns per element, 3 runs. Sizes as study 20: L1 512, L2 16,384, memory 2^25. The mix is the mean of the six cells, with the scalar int64 multiply as in study 20; mix (vector) uses k_sgn.

| | bare | v1 | v2 naive (over v1) | v3 refined (over v1) |
|---|---|---|---|---|
| L1, add32 / sub32 | 0.143–0.147 | 0.227–0.234 | 0.340–0.351 (+49 to 50%) | = v1 |
| L1, mul32 | 0.266–0.268 | 0.642–0.648 | 0.753–0.764 (+17%) | = v1 |
| L1, add64 / sub64 | 0.275–0.277 | 0.549–0.554 / 0.465 | 0.745–0.762 / 0.670–0.677 (+36 to 46%) | 0.610–0.614 / 0.607–0.610 (+0.06 / +0.14, +11 / +31%) |
| L1, mul64 scalar / vector | 1.035–1.039 / 1.465–1.478 | 2.092–2.100 / 4.170–4.194 | 2.300–2.306 / 4.372–4.404 (+10 / +4 to 6%) | = v1 |
| L1, mix / mix (vector) | 0.357–0.359 / 0.429–0.432 | 0.702–0.705 / 1.049–1.054 | 0.861–0.865 (+22.5%) / 1.207–1.211 (+15%) | 0.736–0.739 (+4.8%) / 1.082–1.088 (+3.2%) |
| L2, add32 / sub32 | 0.247–0.251 | 0.245–0.271 | 0.340–0.367 (+35 to 39%) | = v1 |
| L2, add64 / sub64 | 0.499–0.505 | 0.535–0.539 / 0.503–0.508 | 0.734–0.738 / 0.696–0.702 (+37 to 38%) | 0.614–0.616 / 0.608 (+14 / +20%) |
| L2, mix / mix (vector) | 0.466–0.469 / 0.538–0.541 | 0.712–0.717 / 1.060–1.069 | 0.860–0.866 (+20.8%) / 1.208–1.218 (+14%) | 0.743–0.747 (+4.3%) / 1.091–1.099 (+2.9%) |
| memory, add32 / sub32 | 1.297–1.354 | 1.367–1.442 | 1.744–1.858 (+27 to 30%) | = v1 |
| memory, mul32 | 1.377–1.444 | 1.452–1.518 | 1.815–1.934 (+25 to 27%) | = v1 |
| memory, add64 / sub64 | 2.712–2.764 | 2.849–2.930 | 3.671–3.781 (+28 to 29%) | 2.863–2.928 (−1 to +1.4%) |
| memory, mul64 scalar / vector | 2.731–2.818 / 3.080–3.101 | 3.626–3.690 / 4.936–4.962 | 4.478–4.541 (+23%) / 5.756–5.885 (+17 to 19%) | = v1 |
| memory, mix / mix (vector) | 2.028–2.077 / 2.086–2.124 | 2.273–2.317 / 2.485–2.529 | 2.881–2.956 (+27%) / 3.099–3.180 (+25%) | 2.271–2.322 (0%) / 2.484–2.534 (0%) |

**Rankings against study 20:**
- **The naive pass against the refined fused test: holds.** Over the mix the pass costs 22.5% in L1, 21% in L2 and 27% from memory (study 20: 14, 16, 28). The refined test costs 4.8%, 4.3% and 0% (study 20: 2.5, 0, 0.4).
- **The pass from memory** costs what re-reading the results costs, as before: 0.37 to 0.42 ns at int32 and 0.81 to 0.89 at int64.
- **What changed: the fused test is no longer free in L2.** It costs 14 to 20% of an int64 add or subtract there (study 20: −1 to +1%), and 11 to 31% in L1 (study 20: 13 to 14%). From memory it is still free.
  - At 128 bits an int64 add moves 2 values per instruction. The add-with-test loop (10 NEON instructions per element with loads and stores, against 5.25 x86 instructions in study 20) is then bound by instructions in L2 as well as L1.
  - Whether NEON cores, with their own balance of issue width and L2 bandwidth, show the same is uncertain.
  - The add64 v1 against sub64 v1 gap in L1 (0.55 against 0.465, identical counts) is an x86 scheduling effect, not pinned down.
- **The checks (v1 over bare)** cost 59% (add32) to 2.4 times (mul32) in L1 and 5 to 6% from memory (study 20: 75 to 125%, 2 to 6%).

**Study 21 timing,** ns per element per run. In cache N = 1,024, 31 repetitions; memory N = 2^23, 7 repetitions. 2 runs; means of the two medians, with each run's saving.

**back,** scalar int64 multiply (saved = v1 − v):

| L | in cache v1 / v2 / v3 | saved v2 / v3 | memory v1 / v2 / v3 | saved v2 / v3 |
|---|---|---|---|---|
| 0 | 1.62 / 2.21 / 2.17 | −0.57 to −0.61 / −0.53 to −0.56 | 7.20 / 10.16 / 8.64 | −2.9 to −3.0 / −1.2 to −1.6 |
| 1 | 2.31 / 2.66 / 2.58 | −0.35 / −0.22 to −0.31 | 9.72 / 12.22 / 10.15 | −2.3 to −2.7 / −0.4 to −0.5 |
| 2 | 2.83 / 2.95 / 2.71 | −0.11 / +0.12 to +0.13 | 12.01 / 13.79 / 11.72 | −1.7 to −1.9 / −0.25 to +0.83 |
| 4 | 5.52 / 4.02 / 3.58 | +1.5 / +1.9 to +2.0 | 17.09 / 17.93 / 14.42 | −0.6 to −1.0 / +2.6 to +2.7 |
| 8 | 9.47 / 5.79 / 4.91 | +3.6 to +3.8 / +4.5 to +4.7 | 26.70 / 24.41 / 19.82 | +2.0 to +2.6 / +6.6 to +7.1 |
| 16 | 18.55 / 9.66 / 8.02 | +8.8 to +8.9 / +10.5 to +10.6 | 46.10 / 38.10 / 30.69 | +7.6 to +8.4 / +15.0 to +15.8 |
| 32 | 35.29 / 17.04 / 13.79 | +18.2 / +21.5 | 86.95 / 66.03 / 53.68 | +19.4 to +22.5 / +31.8 to +34.8 |
| 64 | 70.80 / 32.24 / 25.79 | +38.1 to +39.0 / +44.6 to +45.4 | 162.88 / 119.19 / 91.98 | +40.1 to +47.2 / +70.7 to +71.1 |
| 128 | 139.05 / 62.36 / 49.23 | +76.6 to +76.8 / +89.8 | 322.40 / 223.57 / 174.72 | +96.3 to +101.4 / +146.8 to +148.6 |

**Break-even:**
- **The refined test: L = 2, in cache and from memory.** In cache it is +0.12 at L = 2 (study 21: between 2 and 4, −0.14 at L = 2). From memory it is level at L = 2 (−0.25 and +0.83 in the two runs) and clearly ahead from L = 4 (study 21: L = 2, +0.1 to 0.4). It holds.
- **The naive pass:** between L = 2 and 4 in cache, and between 4 and 8 from memory (study 21: L = 4 level, ahead from 8). It holds.
- **Each further operation past break-even** saves 0.70 ns in cache and 1.20 from memory with v3, and 0.60 and 0.86 with v2. Study 21: 0.65, 1.2, 0.6, 0.8. Practically unchanged.
- **At L = 128,** v3 takes 35% of v1's time in cache and 54% from memory (study 21: 25% and 46%).
- **With the vector int64 multiply** the savings are larger, because that multiply is dearer (4.2 against 0.65 ns at int32): 1.44 ns per operation in cache and 1.90 from memory. The break-even is the same, L = 2.
- **The jump at the first multiply** (L = 3) stays, now from the scalar int64 multiply against the vector int32 one. On AArch64 that is 24 instructions per element against 6.75, so it holds there too.

**control** (no demotion; pure cost):
- **In cache:** v3 adds 0.37 to 0.88 ns at L ≤ 4 and 11.5% at L = 64 to 128 (study 21: 6 to 7%). v2 adds 19% (study 21: 8 to 13%). The fused test's larger L1 cost (study 20 above) carries over.
- **Memory:** v3 is level with v1 at every L, as before: within 2%, or −1.2 to +3.0 ns on runs of 7 to 320 ns. v2 adds 19 to 36% (study 21: 22 to 36%).

**hover** (L = 128, vector multiply build; hover has no multiply):

| | v1 | v2 (B) | v3 (B) | v3A (rule A) |
|---|---|---|---|---|
| in cache | 76.3 | 101.6 (+33%) | 99.5 (+30%) | 141.6 (1.86 times) |
| memory | 292.0 | 396.7 (+36%) | 295.9 (+1.3%) | 563.3 (1.93 times) |

Study 21: in cache +35%, +28%, 2.2 times; memory +35%, −4%, 1.9 times. Rule B's margin still removes all of rule A's cost.

**Summary of part 3.** The rankings of studies 20 and 21 hold. The refined fused test is far cheaper than the naive pass (+4.8% against +22.5% of the mix in L1, 0% against +27% from memory). It breaks even after 2 further operations in cache and from memory, and saves 0.70 ns per operation in cache and 1.2 from memory after that, as in study 21. The one change is in L1 and L2: at 128 bits the fused test costs 11 to 31% of an int64 add or subtract (study 20: 13 to 14% in L1, free in L2). That raises its pure cost when values never come back (11.5% in cache, from 6 to 7%), and it is still free from memory.

### Summary

- **Division (decisions 28 to 30, 32):**
  - Exact division by the inverse (m2) and the shared reciprocal (m4) stay ahead of the divide: 1.9 and 1.4 times on the stand-in, 13 and 21 NEON instructions per element against 31 and 29 scalar with two divides.
  - The per-lane reciprocal (m5x) falls behind the divide on the stand-in: 8.6 against 5.2 ns, where AVX2 had it 1.17 times ahead. By NEON count it is 48 vector instructions against two 64-bit divides and 35 scalar. Uncertain; reopen on ARM.
  - Two registers interleaved stay the form, and 32 registers do not fit three.
  - Toward zero in mask form is still the cheapest tie rule, now by 2.5 NEON instructions per element over ties up (was about 1). Decision 28 holds.
  - Ties up and down tie their two forms by count; on x86 the direct form's `bsl` (`vpblendvb`) makes it look slower (uncertain).
- **The int64-half multiply (decision 35):**
  - Checked, the vector form loses to scalar on both measures: 4.36 against 2.10 ns, and 46 against 24 AArch64 instructions with the same 8 multiplies. On NEON the grown-size checked multiply should be scalar `mul` + `smulh`. Decision 35's untested "a tie or a win" does not hold for it.
  - Signed pieces still beat unsigned ones.
  - The bare product and the per-array bound are roughly level with scalar (uncertain; the stand-in's `pmulld`).
  - 32 registers against 16 changes the vector check by 2 to 5% and moves no ranking; even with 32 the check spills.
- **Shrinking (decisions 18 and 26):** the refined fused test holds. It breaks even after 2 further operations, saves 0.7 ns per operation in cache and 1.2 from memory, costs nothing from memory when it never fires, and rule B's margin still removes rule A's hover cost. In L1 and L2 at 128 bits it costs 11 to 31% of an int64 add or subtract, against 13% and free in study 20.
- **Build:**
  - The AArch64 compile of the same kernels (neon29a64.c, clang) matches the counters within 1 to 2 instructions per element, except for register copies before destructive multiply-accumulates.
  - Deviations found: `raddhn` for `addhn` (better), `ssra`/`usra` fusions (better), `neg` + `mla` for `mls` (worse by 1), and spills in k_sgn (21 per 4 elements) and m5x (16 to 43 per 8) even with 32 registers.
  - On x86, gcc's all-ones constant is `vpternlogd $0xff` (allowed).
- **ARM reopen list, additions:**
  - m5x against the scalar divide.
  - The bare and bounded int64 multiply against scalar.
  - The fused shrink test's cost in L2.
  - Ties up and down, direct against mask (`bsl`).

## Study 30: the int64 multiply and division reshaped for vector lanes (neon30mul.c, neon30div.c)

**What was done.** Study 29 found two operations where the NEON-shaped code lost: the checked grown-size multiply (46 AArch64 instructions per element against 24 for `mul` + `smulh`) and division with remainder (shared divisor 3.8 ns and per-lane divisor 8.6 ns, against the divide's 5.2). Both were ports of AVX2 designs. This study redesigns those two operations for 128-bit lanes rather than translating them: other layouts, other representations, and NEON instructions not used for them before (`srsra`, `sli`, `uaddl`, `smin` as a detector, `sqrdmulh`, `sqdmulh`, `sqshl`). The rules, layer, barriers, scan, `-DCOUNT` counters and AArch64 build are study 29's (neon29isa.c, neon29a64.c). The new files include study 29's (neon29mul.c, neon29div.c), so study 29's kernels run beside the new ones, in the same binary and harness. Same machine (Xeon family 6 model 173, 2-vCPU KVM guest). Timing: load below 0.5 before each run, nothing else running, pinned to one CPU, 31 repetitions, kernels interleaved with their order rotated, an untimed call first. Ranges are of the medians over 3 runs in L1 and L2, and 1 run from memory, except where stated. Quartile spreads were about ±1%. AArch64 counts are of clang-18's main loop (study 29's script), loads, stores and loop overhead included.

The ranking measure is the AArch64 count (the author's main measure for ARM), with x86 time beside it. Where they disagree, the ranking is marked uncertain.

### Part A: the checked int64-half multiply

**The floor.** With 32 × 32 → 64 multipliers, two 32-bit pieces per int64 are the minimum. A Gaussian product is then 16 piece products, which is 8 `smull`/`smlal` per element (2 products each). That is the same 8 multiplies as scalar `mul` + `smulh`. A vector form can therefore win only on the other instructions. Scalar spends about 16 of those per element, 6 of them loads and stores.

**Options surveyed:**
- **(1) Separate arrays of 32-bit pieces.** Built (k_pc).
- **(2) Smaller limbs.** 22 or 26 bits with delayed carries: 3 limbs make 36 piece products per element (18 multiply instructions).
  - Karatsuba and Gauss's three products need limb sums, which fit with 22-bit limbs. They bring it to 18 products (9 instructions), plus about 10 instructions per element of 64-bit recombination.
  - Counted, not built: neither reaches the 8-multiply floor, so neither can beat scalar.
- **(3) Practice:**
  - On AArch64, OpenSSL and BoringSSL bignum code, and most X25519 code, use scalar `mul`/`umulh`.
  - Bernstein and Schwabe's radix-2^25.5 NEON arithmetic paid off on ARMv7, which has no 64-bit scalar multiply.
  - Hybrid loops that run scalar and NEON work side by side (Lenngren's X25519; Becker and Kannwischer 2022) raise throughput, not instruction count.
  - SVE2 has 64-bit lane `MUL`, `SMULH` and `ADCLB`/`ADCLT`, which would put the scalar algorithm itself into lanes.
  - NEON's QC flag (decision 37) was worked through for the overflow test. With the right order of saturating steps it is exact, but it costs more than the K test below (about 10 against 7 per pair).

**The new exact test** (k_pc, k_v2):
- **Signed-low pieces.** l = the low word read as signed, h' = hi + (lo >> 31, logical): one `usra` per operand register. Then x = h'·2^32 + l.
- **Columns.** HH = ah'ch' − bh'dh', M1 = ah'cl − bh'dl, M2 = al ch' − bl dh', LL = al cl − bl dl. Each is two `smull`/`smlsl` per pair.
- **Low part.** Z0 = lo(M1) + lo(M2) + (LL >> 32): `uzp1` twice, `uaddl`, `uaddl2`, then `ssra`.
- **Output pieces.** lo = `uzp1`(LL) and hi = `uzp1`(Z0). These are the standard pieces of re mod 2^64.
- **K test.** K = HH + (M1 >> 32) + (M2 >> 32) + round(Z0 / 2^32), by `ssra`, `ssra`, `srsra`. Its value is floor((re + 2^63) / 2^64), computed mod 2^64. Since |K| ≤ 2^63, re fits int64 iff K = 0. No 64-bit result is ever formed.
- **im.** The same with sums. LL' = al dl + bl cl reaches 2^63 when all four lows are −2^31, so it is read as LL' + 2^63 − 2^32 unsigned: one add, a `usra`, and a +1.
- **Edges.** Study 23's edge corrections (20 instructions per register) are replaced by a running `smin` of the four h' registers. If the minimum reaches −2^31 anywhere in the array, the array is redone by the exact scalar kernel, once per array like the sticky flag's own branch. That happens only for an input within 2^31 of ±2^63. With every h' above −2^31, HH, M1 and M2 cannot wrap.
- **Flag.** The four K registers are ORed into one, tested once per array.

**Layouts:**
- **The pieces layout (k_pc).** Each int64 array is split into its two 32-bit words, hi and lo, as two int32 arrays. The meaning is unchanged and the size stays int64. Loads give the pieces directly, and the outputs are packed pieces.
- **The int64 layout (k_v2).** The same test, with pieces by `uzp1`, `uzp2`, `usra` on load and outputs rebuilt by `sli` (shift left and insert, one per pair).
- **s_pc** is the scalar kernel in the pieces layout, showing what the layout costs scalar code. **k_pcb** is the bare (wrapping) product in the pieces layout.

**Correctness** (study 23's harness, plus pieces):
- **Edges:** every quadruple of 44 edges, each element alone in a pass of 4 with its position rotating. These are study 23's 37 edges plus values around 2^63 − 2^31 (where h' wraps), ±2^63 ± 2^31, and lows of −2^31.
- **A piece grid:** high pieces from 9 values and low pieces from {−2^31, −2^31 + 1, −1, 0, 1, 2^31 − 1}, 944,784 quadruples. This reaches LL' = 2^63.
- **Random:** 4 million single elements, half aimed at the int64 boundary, and 200,000 arrays.
- **Totals:** 44,464,405 kernel calls, failures none. Each checked kernel saw the flag clear 366,399 times and set 8,526,482 times. 6,887,700 calls went through the edge fallback.
- **Side kernels:** add, shrink test and promotion in both layouts, 200,000 arrays, failures none.
- **Injected faults, all caught:**
  - K without the `srsra` term: 18,626 failures (quick run);
  - im without the +1;
  - no edge fallback: 875;
  - `ssra` in place of the offset `usra` for LL': 3, at the all −2^31 lows only, which the piece grid exists for;
  - h' without the `usra`;
  - Z0 dropping lo(M2).

**Counts and timing,** per element:

| | s_chk (scalar, int64 layout) | k_sgn (study 29) | k_v2 (new, int64 layout) | k_pc (new, pieces) | s_pc (scalar, pieces) | k_pcb (bare, pieces) |
|---|---|---|---|---|---|---|
| AArch64 loop | 24 | 46.0 | 25.75 | 23.5 | 38 | 9.25 |
| AArch64 spills per 4 elements | 0 | 21 | 0 | 0 | 0 | 0 |
| NEON counter + loads/stores | | 37.5 | 24.5 | 22.25 | | 8.5 |
| x86 loop | 39 | 61.0 | 52.5 | 55.75 | 61 | 12.75 |
| x86 L1 ns | 2.08–2.09 | 4.16–4.17 | 3.54–3.55 | 3.06 | 3.12 | 1.12–1.13 |
| x86 L2 ns | 2.08–2.10 | 4.17–4.19 | 3.54–3.55 | 3.06–3.07 | 2.88–2.91 | 1.12 |
| x86 memory ns (3 runs) | 3.45–3.47 | 4.86–4.88 | 4.30–4.33 | 3.45–3.47 | 3.51–3.54 | 2.48–2.51 |

For comparison, on AArch64 the bare forms are s_bare 13 and k_bare 11.75, and the bound form k_bnd is 17.75.

**Side costs of the pieces layout** (AArch64 loop per element; x86 L1 ns per element; the add is per Gaussian, promotion and the shrink test per int64 value):

| | pieces | int64 layout |
|---|---|---|
| checked add (pieces: carry by `cmhi`, V rule on the high word) | 7.75; 0.525–0.529 | 9.5; 0.562–0.565 |
| promotion from int32 halves (pieces: lo = x, hi = `sshr` 31; int64: `sxtl`, `sxtl2`) | 1.75; 0.129–0.130 | 2.0; 0.102 |
| rule B shrink test (pieces: `usra` + `orr` on hi, add + `orr` on lo; int64: add + `orr`) | 2.25; 0.142–0.146 | 3.0; 0.141 |
| demotion | take lo, no work | `uzp1` |
| the checked multiply in scalar code | 38 | 24 |

**What this shows:**
- **By the AArch64 count, the reshaped vector multiply ties plain registers:** 23.5 against 24, 2% fewer.
  - Unrolling by 2 would save about 0.4 more (estimated, not built).
  - Nothing else found cuts the K test. A 32-bit-lane carry form of the low columns costs 1 more per part, not less.
  - It does not beat the scalar form; it removes study 29's loss (46 against 24).
- **x86 still has scalar ahead in cache:** 2.08 against 3.06 ns (1.47 times). From memory they tie (3.46 against 3.46). The stand-in spends 55.75 x86 instructions per element on 22.25 NEON ones (odd-lane `pmuldq` shifts, 64-bit `srai`). The two measures disagree, so the ranking is uncertain.
- **The test is what moved,** not the layout. In the int64 layout the same test takes the vector check from 46 to 25.75 AArch64 instructions, and on x86 from 4.17 to 3.54 ns. Most of that is the edge corrections, replaced by the `smin` detector and the fallback.
- **The pieces layout is a small win wherever the code stays in vector lanes:**
  - by count: multiply −2.25, add −1.75, promotion −0.25, shrink test −0.75 per element;
  - on x86: the add is 6% faster, the shrink test level, promotion 27% slower.
  - It costs scalar code 14 instructions per element on the checked multiply (38 against 24): every value has to be assembled and split again.
  - The bare product in pieces is the cheapest form of all: 9.25 against 13 scalar.
- **Storing h' instead of hi** would save about 2 per element (no `usra`, no `smin`). It is counted, not built, because it changes the size's range to [−2^63 − 2^31, 2^63 − 2^31).

### Part B: division with remainder

**Options surveyed:**
- **Estimate from the Gaussian reciprocal in 32-bit fixed point (built).** q ≈ x·conj(y)/N with conj(y)/N held as two 32-bit words, the estimate by `sqrdmulh`, and one exact test.
- **Remainder-based test (built, s1).** The remainder is taken in 32-bit lanes, and only the test widens to 64 bits.
- **Per-lane reciprocal by `sqdmulh` Newton in Q30 (built, l1).** It replaces m5's packed-33-bit Newton and its two 64-bit fix-ups (about 22 NEON instructions per element).
- **`urecpe` as the seed.** An integer instruction (9 bits), it would save one Newton step. Not used: x86 has no stand-in.
- **`sqrdmlah` (ARMv8.1).** It fuses the estimate's add (6 instead of 8 per 4 elements). Not used: it is not in the portable list.
- **Keeping the quotient in 32-bit lanes.** The quotient has at most 33 bits: it fits as the estimate's fixed point for every divisor with N ≥ 4. It does not fit for units and (1 + i) times units.

**The reshape:**
- **Reciprocal words.** W1 = round(c·2^(31+T)/N) and W2 = round(d·2^(31+T)/N), with T the largest value such that 2^(2T+2) ≤ N. Then |W| ≤ 2^30, and T ≥ 3 for N ≥ 256.
- **Estimate.** q̂1 = floor((`sqrdmulh`(a, W1) + `sqrdmulh`(b, W2)) / 2^T), and q̂2 likewise with b·W1 − a·W2. Two roundings of 1/2 and W's rounding of at most 1 in total give |q̂ − X/N| ≤ 2/2^T ≤ 1/4 before the floor. So q̂ is q − 1 or q for the nearest q, and at a tie it is the toward-zero candidate or one below it.
- **One exact test per part.** Round up iff ρ + s > H, where ρ = X − q̂N, H = N >> 1, and s = [q̂ < 0] when N is even (0 when N is odd, where there are no ties). At ρ = N/2 the part goes up exactly when q̂ < 0, which is ties toward zero. The result is the same function as decision 28's rule (study 29's B2, mask form), flags included.
  - The mask form's purpose (no sign work in the threshold) is met here by the one bit s in the low bit of ρ (`uaddw`).
- **s1 (shared divisor, r̂ form).**
  - r̂ = x − q̂y in 32-bit lanes (`mla`, `mls`), exact while |r̂| < 2^31. With q̂ in {q − 1, q}, |r̂| ≤ 1.77|y|, so s1 is used for N ≤ 2^59.
  - ρ1 = r̂1·c + r̂2·d and ρ2 = r̂2·c − r̂1·d by `smull`/`smlal`.
- **s2 (shared divisor, X form, any N ≥ 256).**
  - ρ1 = X − q̂1·N from `smull`(a, c) + `smlal`(b, d) − `smull`(q̂1, Nl) − (q̂1·Nh mod 2^32)·2^32, with N = Nh·2^32 + Nl in signed-low pieces (`smlsl`, `mul`, `shll`, sub).
  - ρ lies in [−εN, (1 + ε)N] with ε ≤ 2^−29, which is exact mod 2^64 unless N(1 + ε) ≥ 2^63. That needs |c| and |d| within 2 of 2^31 and an estimate error beyond 1 + (2^63 − N)·2^T/N. For every such divisor, W's rounding errors computed exactly keep the error above that bound, so no input reaches it.
- **Shared fallback.** N < 256 goes to study 29's m4, and so does N > 2^59 for s1. This is once per array and costs nothing per element.
- **l1 (per-lane divisor):**
  - N = c² + d² per lane, and L = floor(log2 N) from the two 32-bit `clz` (study 29's 64-bit clz).
  - dp = the top 31 bits of N.
  - v ≈ 2^61/dp by a linear seed (48/17 − 32/17·D) and three Newton steps v ← 2·`sqrdmulh`(v, 2^31 − `sqdmulh`(dp, v)), with `sqshl` for the doubling.
  - Then W = `sqrdmulh`(c << sc, v) with sc = 31 + T − L. The exponents cancel exactly, so no shift follows. When L ≥ 61, sc is a right shift by 1 or 2, harmless there because the quotient is at most 2.
  - Stage 1 is the shared estimate, with T per lane.
  - Lanes with N < 2^14 also run stage 2. There the stage-1 error can reach about 17, but r0 = x − q0·y stays below 2^13, so q0 += floor(estimate(r0 << 18) / 2^(T+18)). The stage-2 result is added only where an and-mask is set.
  - The test is s2's, on per-lane pieces of N, in the form (ρ − H) + s > 0 (one more subtract per pair). Per-lane W errors are a few units, so the bound used for s2 was not established here.
  - A unit, a (1 + i) times unit, or a zero divisor (N ≤ 2 after the zero fix) sets a flag that sends the array to the scalar divide, once per array.
  - l2 interleaves two registers.

**The per-lane reciprocal,** run over all 2^30 values of dp: v − round(2^61/dp) is −1 for 61,552,851 values, 0 for 395,212,353, 1 for 472,672,256, 2 for 141,659,099 and 3 for 2,645,265. The worst relative error is 2^−29.0.

**Correctness** (an __int128 reference from exact rationals, study 25's; quotient and remainder mod 2^32, both flags, and `resolve`):
- **Small divisors:** every divisor with |c|, |d| ≤ 32, against a box of x values and every pair of the 20 edges. 129 million results, the m4 fallback included.
- **Ties:** batches aimed at ties (c ≡ d mod 2, x = qy + half of y, iy or (1 + i)y), 42 million results. There were 22.0 million elements at a tie in all.
- **Edges:** every quadruple of the 20 edges, each element alone in a register of 4 (flags per element). Divisors near powers of 2 from 2^8 to 2^63, with edge and random x.
- **Random:** shared divisors of every size, 26 million results.
- **s1's limit:** N from 2^58 to 2^62, with x placed so that x/y − q̂ lies at the corners of [−1/4, 5/4)², where r̂ is largest. 610,326 results.
- **Near 2^63:** N within 2^35 of 2^63 (|c|, |d| within 5 of 2^31, every sign), |a| and |b| near 2^31, and X just above a multiple of N. 2.75 million results.
- **Per lane:** divisors whose sizes mix inside each register, from 1 bit to 32 bits, and x random, from the edges, near multiples of y, or at ties. Every 8th batch also has units, (1 + i) times units and zeros. 42 million results.
- **Totals:** 1,092,726,750 element results, failures none. Each shared kernel had 163 to 172 million results on the new path. The lane kernels had 474 million on the vector path; 7,414 arrays went to the scalar divide (units or zeros).
- **Injected faults, caught:**
  - T one too large;
  - no tie bit;
  - a tie bit for odd N too;
  - round in place of floor in the estimate;
  - s1's limit at 2^62 (only by the limit test, 3,665 failures in the quick run);
  - ρ without the Nh term;
  - l1 without stage 2;
  - l1 with two Newton steps.
- **Not caught:**
  - stage 2 below N = 2^8 only, so the 2^14 threshold has margin;
  - l1 without the safe form of the test (neither proved nor shown needed; the safe form is kept).

**Counts and timing,** shared divisor, per element:

| | m1s (scalar, floor core only) | m4_mb2 (study 29) | s1 | s2 |
|---|---|---|---|---|
| AArch64 loop | 29 | 33.75 | 13.75 | 16.5 |
| NEON counter + loads/stores | | 25.5 | 12.5 | 15.0 |
| x86 loop | 58 | 54.5 | 50.5 | 55.25 |
| x86 L1 ns, random 32-bit divisor (N ≈ 2^62.8; s1 falls back to m4) | 5.18–5.19 | 4.17–4.18 | (4.17) | 2.72–2.73 |
| x86 L1 ns, small divisor (parts ≤ 256, N ≈ 2^14.6) | 5.18–5.21 | 4.16–4.21 | 2.93–2.94 | 2.73 |
| x86 L1 ns, medium divisor (parts ≤ 2^20, N ≈ 2^38.8) | 5.21–5.22 | 4.17–4.21 | 2.94–2.95 | 2.73–2.75 |
| x86 memory ns (1 run, every divisor size) | 5.33 | 4.28–4.31 | 3.22–3.25 | 3.07 |

The third L1 run of the medium group was disturbed (quartiles ±8%) and is left out.

**Counts and timing,** per-lane divisor, per element:
| | m1 (scalar, floor core only) | m1_mb2 (scalar, toward zero) | m5_mb2 / m5x_mb2 (study 29) | l1 | l2 |
|---|---|---|---|---|---|
| AArch64 loop | 37 (study 29) | 57 | 55.75 / 59.6 | 38.25 | 43.5 |
| AArch64 spills, register copies per 4 elements | 0 | 0 | 0 / 13.5, 6 | 0, 4 | 9.5, 14 |
| NEON counter + loads/stores | | | 50.25 (m5x) | 34.5 | 34.5 |
| x86 loop | 58 | 94 | 84.1 (m5x) | 206.5 | 149.6 |
| x86 L1 ns, random 32-bit divisors | 5.18–5.21 | 6.84–6.85 | 9.10–9.11 (m5x) | 12.62 | 9.21–9.24 |
| x86 L1 ns, small divisors (parts ≤ 256, N ≥ 4) | 5.19–5.21 | 6.84–6.89 | 9.09–9.13 | 12.63–12.65 | 9.23–9.25 |
| x86 L1 ns, medium divisors (parts ≤ 2^20) | 5.19–5.20 | 6.83–6.87 | 9.07–9.10 | 12.62–12.64 | 9.12–9.22 |
| x86 memory ns (1 run, the three kinds) | 5.36–5.40 | 6.96–7.01 | 9.25–9.30 | 12.80–12.85 | 9.36–9.39 |

An array that holds a unit, a (1 + i) times unit or a zero divisor costs l1's sweep plus the scalar divide.

**What this shows:**
- **Shared divisor: the reshape wins on both measures.**
  - s1 and s2 take 13.75 and 16.5 AArch64 instructions per element, against 33.75 for m4 with the same rule and 29 for the scalar divide's floor core alone (2.5 and 2.0 times fewer than m4).
  - On x86, s2 runs 2.72 ns against m4's 4.17 (1.53 times faster) and against 5.18 for the divide's floor core (1.9 times). From memory it is 3.07 against 4.29 and 5.33.
  - Study 29's shared ranking of the reciprocal against the divide (1.38 times) widens to about 2 times.
- **s1 against s2: the measures disagree.**
  - By count s1 is cheaper: 13.75 against 16.5.
  - On x86 s2 is 7% faster (2.73 against 2.94). s1 has 4 more 32-bit `mla`/`mls` per 4 elements, which are `pmulld` here (2 µops, 10 cycles); on NEON they are one instruction each.
  - The ranking is uncertain. By the ARM measure, s1 is the form for N ≤ 2^59, with s2 above that. s2 alone is the simpler choice and gives up 2.75 instructions per element.
- **Per-lane divisor: the count and x86 disagree.**
  - By count, l1 takes 38.25 AArch64 instructions per element against 57 for the scalar divide with the same rule. That is a third fewer, and level with the scalar floor core alone (37). It also takes 31 to 36% fewer than m5 (55.75) and m5x (59.6), and it does not spill.
  - On x86, l1 is the slowest form at 12.6 ns, against 6.85 for the scalar divide with the rule. l2 (9.2) only ties m5x (9.1).
  - l1 is latency-bound, as m5 was at one register (study 29: 11.24 ns). Each `sqdmulh`/`sqrdmulh` and the signed lane shift costs about 6 x86 instructions here, so l1's loop is 206 x86 instructions per element against 34.5 NEON ones.
  - The ranking is uncertain. On ARM it rests on `sqrdmulh` throughput and the length of the dependency chain (normalise, 3 Newton steps, W, two estimates, test), so it joins the reopen list.
- **Interleaving per lane:** two registers spill on AArch64 (9.5 per 4 elements, plus 14 copies), so l2 is 43.5 against l1's 38.25. On x86, l2 is 27% faster than l1. One register is the AArch64 form; two the x86 form.
- **What the reshape removed:** in the shared kernels, all 64-bit work in the estimate (in study 29's m4 the estimate and its 64-bit correction were about 15 of 24 NEON instructions per element) and all sign work. In the lane kernels, m5's packed reciprocal with its two 64-bit fix-ups (about 22 NEON instructions per element) became a Q30 Newton of 14 per 4 elements. The one 64-bit step left is the exact test.
- **The tie rule** stays decision 28's: ties toward zero, the same results as study 29's mask form. On this path it costs one bit, s, folded into the test by `uaddw`, 2 instructions per 4 elements per part.
- **Compiled-code deviations:**
  - In l1, clang negates the lane shift counts (`neg`, 10 per 4 elements) where the kernel could produce −T directly at the same cost (portable-isa.md note 1); about 2.5 per element could be saved.
  - In s1, clang adds 2 register copies per 4 elements.
  - No spills in s1, s2 or l1.

### Summary
- **The checked int64-half multiply:**
  - Reshaped, the vector form ties plain registers by AArch64 count: 23.5 against 24 per element in the pieces layout, and 25.75 in the int64 layout. Study 29 had 46 against 24.
  - The gain is a new exact test, not the layout. The overflow test K = HH + (M1 >> 32) + (M2 >> 32) + round(Z0/2^32) is built from `ssra` and `srsra`, the output pieces come straight from `uzp1`, and an `smin` detector with a once-per-array scalar fallback replaces the edge corrections.
  - On x86, scalar stays 1.47 times faster in cache and ties from memory, so that ranking is uncertain.
  - The multiply floor (8 per element on both sides) leaves no room for a clear vector win with 32-bit multipliers. Smaller limbs need 18 or more multiplies per element.
  - The pieces layout is slightly cheaper for every vector operation measured (multiply, add, shrink test, promotion by count; on x86 promotion is slower) and costs scalar code 14 instructions per element on the multiply.
  - SVE2's 64-bit lane `SMULH` and `ADCLB`/`ADCLT` would change this; not testable here.
- **Division with remainder, shared divisor:**
  - The 32-bit fixed-point Gaussian reciprocal (`sqrdmulh` estimate, one exact test with the tie bit) beats m4 and the divide on both measures.
  - s1 takes 13.75 and s2 16.5 AArch64 instructions per element, against 33.75 for m4 and 29 for the divide's floor core alone.
  - On x86, s2 takes 2.72 ns against 4.17 for m4 and 5.18 for the divide.
  - s1 against s2 is uncertain (count favours s1, x86 s2).
  - N < 256 stays with m4, once per array.
- **Division with remainder, per-lane divisor:**
  - The Q30 `sqdmulh` Newton reciprocal, per-lane W by `sqrdmulh`, and a second estimate stage for N < 2^14 take 38.25 AArch64 instructions per element. That is against 57 for the scalar divide with the rule (37 for its floor core) and 59.6 for m5x.
  - On x86 it is the slowest form (12.6 ns, latency-bound and dear in the stand-in; two registers 9.2, against the divide's 6.85). Uncertain until timed on ARM.
  - Arrays with unit, (1 + i) times unit or zero divisors go to the scalar divide, once per array.
- **For the decisions** (not edited here):
  - Decision 30's shared reciprocal could become s2 (or s1 for N ≤ 2^59).
  - Decision 32 and the reopen item on per-lane division gain a candidate that leads by ARM count.
  - The reopen item on the grown-size multiply now reads "ties by count, loses on x86". The pieces layout is a candidate to weigh against decision 15's int64 arrays at the grown size.
- **ARM reopen list, additions:**
  - k_pc against `mul` + `smulh` (tie by count).
  - s1 against s2.
  - l1 against the scalar divide, and l1 against l2.
  - Whether l1 needs its safe test form.

## Study 31: the (1+i)-adic form x = (1+i)^k u and cheaper addition (adic31.py)

**What was done.** The representation: every nonzero x is (1+i)^k u with u odd (a + b odd), and zero has a reserved code. u is stored either in unit digits (words A, B; bit pair 00, 01, 10, 11 picks 1, i, -1, -i; u = sum u_j 2^j over n positions) or as an ordinary pair. Python only, no timing. Costs are counted in a simple model: alu (add, sub, logic, mask), shift, ctz (tzcnt, ctz(0) = 64), branch (a jump table counts 1). `python3 adic31.py all` runs everything in about 2 minutes.

### Converters
- **Unit digits.** With M = 2^n - 1 and X = A ^ B:
  - Decode, 4 ops: re = M - X - A, im = X - A.
  - Encode, 8 ops: R = a - b, S = a + b, R' = (R + M) >> 1, S' = (S + M) >> 1, A = ~S', B = R' ^ S'.
  - So unit digits are the offset binary of (1+i)u, with the bits mixed by one XOR.
- **Valuation.** k = 2 ctz(a|b) + bit m of (a & b), with m = ctz(a|b).
- **Digit-wise unit operations.** Negation: A = ~A. Conjugation: A ^= B. Times i: A ^= B, B = ~B. Times i^r with masks: 6 ops.
- **Checks.** n = 1..8 exhaustive: 4^n codes, all distinct, exactly the odd diamond |a| + |b| <= M. A box of side 81 and random 41-bit values: exact.

### Patterns (sums from the box |a|, |b| <= 31; digit rules exhaustive at n = 6)
- **Result k.** If k1 != k2, the result has k = min(k1, k2), and its odd part u1 + (1+i)^d u2 needs no stripping. Only k1 = k2 strips. For random values that is 1/3 of sums (input k is geometric, P(k) = 2^-(k+1)).
- **Stripped count t (k1 = k2).** Geometric:

| t | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| share | 0.499 | 0.252 | 0.125 | 0.062 | 0.031 |

  Subtraction and adding a unit give the same distribution.
- **t from the digits, exact.** Let j0 be the first position where u2_j != -u1_j, i.e. j0 = ctz(~((A1^A2) & ~(B1^B2))).
  - t = 2 j0 + 1 if u2_j0 = +-i u1_j0.
  - t = 2 j0 + 2 if u2_j0 = u1_j0.
  - From digit 0 alone: t = 1 iff B1_0 != B2_0 (half the cases), t = 2 iff the codes are equal, t >= 3 iff the digits are opposite.
  - t >= m depends only on the lowest ceil(m/2) digits.
- **Word-level sum of two unit-digit numbers.** u1 + u2 = -i(1+i)(P + iQ), with P = M - (X1 + X2) and Q = M - (A1 + A2). It takes two ordinary adds, and t = 1 + v(P + iQ).
  - When t = 2 the result digits come almost directly: S' = (X1 + X2) >> 1, R' = M - ((A1 + A2) >> 1).
- **Large gaps.** For d = 2m, the result's digits below m - 1 equal u1's, and digit m - 1 is u1's negated (one carry of u1_{m-1} into position m). For d = 2m + 1, the digits below m equal u1's.
- **Growth.** With k1 = k2, the stripped result needs from 5 digits fewer to 1 digit more than the larger input (+0: 35%, -1: 35%, +1: 4%).
- **Carries.** Digit-serial unit-digit addition with one position of parity lookahead is exact (n = 4, d = 1..5, 327,680 cases).
  - Even gap: the carries stay units.
  - Odd gap: there are 9 carries, with (1+i)c in 2{-1, 0, 1}^2.
  - Carry maps: equal digits generate, opposite digits propagate, and adjacent digits (u2_j = +-i u1_j) generate one coordinate and propagate the other. The semigroup has 9 elements, {generate 0, generate 1, propagate}^2.
  - So the carries are two independent ordinary binary carries, on the real and imaginary sign bits of (1+i)·carry. Propagation is exactly as in binary: unbounded in the worst case, short on average, and lookahead works. Nothing beats doing it as the two word adds P, Q.

### A rotation-free variant
**Split form.** x = 2^(k>>1) (1+i)^(k&1) w, with w = i^(k>>1) u. It has the same k, and w is still unique, so the form is still canonical.
- Aligning is then a plain shift, plus at most one multiply by (1+i) or (1-i) when d is odd: (1-i) if the smaller k is odd.
- Stripping needs a times-i only when both the input k and the strip are odd ((1+i)^2 = 2i).
- The canonical form needs a data-dependent rotation by i^h on alignment and by (-i)^m on stripping.

### Addition algorithms (verified exhaustively at n = 3 with k1, k2 in 0..8 and zero; n = 4 with k1 in {0, 1}; sampled at n = 5, 6 with cancelling pairs; random at n = 32)
Zero is coded k = INF with u = 0. INF exceeds every k, so zero always lands on the aligned side and needs no special case except a zero sum. Ops per add at n = 32 with geometric k:

| algorithm | avg | d = 0 | d odd | d even | worst |
|---|---|---|---|---|---|
| ordinary pair | 2 | | | | 2 |
| canonical pair, branchy | 16.4 | 22.2 | 13.8 | 12.7 | 25 |
| split pair, branchy | 16.1 | 21.8 | 14.7 | 10.5 | 24 |
| canonical pair, branch-free | 81 | | | | 81 |
| split pair, branch-free | 67 | | | | 67 |
| unit digits: decode, split branchy, encode | 32.1 | 37.8 | 30.7 | 26.5 | 40 |
| unit digits, direct d = 0 (P, Q) | 29.7 | 30.6 | 30.7 | 26.5 | 33 |
| unit digits, branch-free | 83 | | | | 83 |
| lazy split (w any), branchy | 10.6 | | | | |
| lazy 2^m w, branchy | 6.2 | | | | |
| lazy split, branch-free | 35 | | | | |
| lazy 2^m w, branch-free | 19 | | | | |
| normalise a lazy value | 15.3 | | | | |

- **Branch-free costs.** The branch-free forms pay for a conditional swap of the operands (8), the odd-gap multiply (about 13), the strip select (9) and, in canonical form, two masked rotations (15 each). In the branch-free form the strip runs unconditionally: when d != 0 the sum is odd and the strip finds m = e = 0.
- **Branchy costs.** The branchy forms branch on d = 0 (taken 1/3 of the time) and on e (50/50), so on real hardware both mispredict often.
- **Unit digits** add their conversions (8 + 8) to every add. They cannot hold an even value, so they force a full normalisation each time. The direct P, Q formula saves about 7 ops on d = 0 only.
- **Lazy normalisation is the real saving.** Keep (k, w) with w allowed to be even, and multiply, add and align without stripping. Normalise (15 ops) only for equality, zero tests, valuation or gcd steps.
  - In the 2^m w form, adding with equal m is just the 2 pair adds, and with unequal m it is a shift and 2 adds. It is a binary floating-point Gaussian integer.

### Odd representatives modulo an odd modulus nu (coordinator's addition)
Checked exhaustively on 42 moduli, 1,142,378 (x, y) pairs: rational 3..19 including 9 and 15, Gaussian primes of norm <= 300, and products such as (2+i)(3+2i) and 3+6i. Also random nu with parts below 2^30 (3000 cases).
- **(1) Multiplication.** Odd times odd is odd, and reducing the product mod (1+i)nu keeps it odd, so no parity fix is needed. The only cost is a modulus of norm 2N, one bit wider.
- **(2) Range.** Two odd representatives of one class differ by a multiple of (1+i)nu, never by nu itself, since odd minus odd is even.
  - The right range is a half-open cell of (1+i)nu. It has 2N points, exactly N of them odd, one per class.
  - The plain cell of nu holds an odd representative for only 31% to 80% of classes (about half for rational p). So "odd and inside the plain range" fails. The odd range is √2 larger in magnitude: 1 bit more per coordinate in pair form (for rational p, the diamond |a| + |b| <= p against the box |a|, |b| <= p/2).
  - Canonical form: if x is even add nu (a masked add), then reduce mod (1+i)nu.
- **Addition and subtraction.** The sum or difference of two odd representatives is even, and exactly one correction from {+-nu, +-i nu} always applies. It is chosen by the signs of the two cell coordinates (verified).
  - Plain residues instead need 0 or 1 of 8 corrections, chosen by 4 threshold compares.
  - For a Gaussian nu both need the two linear forms s·conj(g) (4 multiplies) unless the residues are kept in lattice coordinates.
- **(3)/(4) Equality, negation, canonical form.** The representative is canonical in the half-open cell, so equality is a word compare.
  - The (1+i)nu cell has boundary ties (its norm is even), while the plain cell of an odd nu has none.
  - So negation needs a fix on the tied classes: 2p - 1 of p^2 for rational p, 1 for the Gaussian primes tried. Plain symmetric residues negate for free.
- **Rational p, add kernels** (verified exhaustively for p = 3..23, and randomly for p < 2^61):

| residues | ops per add |
|---|---|
| plain [0, p) | 10 |
| plain symmetric | 18 |
| odd, stored as (R, S) = (1+i)x | 11 |
| odd, stored as the pair x | 17 |

- **Why odd representatives give nothing for rational p.** With (R, S) = (1+i)x, where R is odd in (-p, p] and S is odd in [-p, p), the values ((R+p)/2, (S+p)/2) are exactly the plain residues of x(1+i)/2. This is verified for p = 3, 7, 11, 13 and 101.
  - So odd representatives mod p are plain residues twisted by the unit (1+i)/2, with range [1, p] x [0, p-1]. Products pick up one factor (1-i) (2 adds).
  - This is the same offset map as unit digits. n unit digits are the odd representatives for the modulus 2^n - 1, with the 2M + 1 boundary ties of the closed diamond counted twice ((M+1)^2 = M^2 + 2M + 1).

### Summary
- **Patterns.**
  - The result's k is min(k1, k2) unless k1 = k2.
  - With k1 = k2 the stripped count t is geometric (mean 2) and exactly predictable from the digits: t = 2 j0 + 1 + [u2_j0 = u1_j0]. Half the time t = 1, decided by one bit pair.
  - Unit-digit carries are two independent binary carry chains in rotated coordinates.
- **Best strict algorithm.** The split form x = 2^(k>>1) (1+i)^(k&1) w, branchy: about 16 ops per add against 2 for a pair add (22 when k1 = k2, 10.5 to 15 otherwise). Branch-free it takes 67. Unit digits roughly double the cost (30 to 32 branchy, 83 branch-free).
- **Best overall.** Do not normalise on add. The lazy 2^m w form costs 2 ops when the exponents match, 6.2 on average, and 19 branch-free. Normalise (15) only where the canonical form is needed.
- **Where addition is cheap.**
  - Equal exponents in the lazy form.
  - Any k1 != k2 (no strip).
  - t = 1 (half of the equal-k cases), recognised from one bit pair.
  - Large gaps, where the low digits are u1's unchanged.
- **Costs and gains overall.**
  - The form costs an exponent per value and an alignment on every add. Strict normal form costs 8 times a pair add (branchy) to 30+ times (branch-free).
  - It gains free (1+i)-power multiplication and division, a free valuation (gcd and the (1+i)-ary gcd keep values stripped anyway), and multiplication at the cost of the odd parts plus one add of exponents.
  - Unit digits gain cheap unit multiplications (1 to 2 ops) and a reserved-free code space (every code is a value). They lose on every add, because they cannot hold even intermediates.
  - Modulo an odd nu, keeping odd representatives is canonical and closed under the ring operations. It costs one bit of range in pair form and a forced correction on each add, and gains nothing over plain residues for rational p (it is a twist of them).
