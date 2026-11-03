# Spec 14: the operations at three levels (classification, Nov 3)

A review of decisions.md against the book's three levels: the machine default (what a plain machine or an ordinary-integer library does), the textbook answer (the general solution for the Gaussian world, which the first spec and build contain), and tricks (special cases, kept for later). Decision numbers refer to decisions.md. Nothing here changes a decision.

Two columns need a convention. Fixed width means decision 49: the user declares a size, promotion and shrinking are off, and a set overflow flag is an error. Constant time means that no branch, memory address or loop count depends on the values; the shape of a call, the sizes, and a modulus or divisor named public by the caller may steer the code. Every textbook method below ends its sweep with one branch on the sticky flag. At a fixed width that branch only chooses between a result and an error, and with values reduced mod n it never fires (49), so the tables count it as constant time and list only the value-dependent steps beyond it.

## 1. Making and reading numbers

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| From one or two integers | A struct or complex type of two ints, interleaved | Separate real and imaginary arrays (15), values in vector lanes even one at a time (7, 42); an integer n becomes (n, 0); a value too large enters at the size it needs (25, conversion note) | None | Yes; a value too large is an error | Yes at a fixed width; under growth the entry size depends on the value |
| Read the real or imaginary part | Field access | Read one array; at grown sizes the 32-bit pieces are joined (43) | None | Yes | Yes |
| Leave the exact type | Silent conversion to float | Refused (40) | None (the suppressible refusal is library level) | Yes | Yes |

Creation and reading are listed in 41 but have no written design. They are simple, but the choice of entry size for a large integer (smallest size that passes rule B, as study 12 did for fresh arrays) is not decided anywhere.

## 2. Ring operations

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Add, subtract | Two scalar adds; wraps silently (C signed overflow is undefined); a bignum library grows each number by itself | One lane add per array, XOR test into the sticky flag (13, 15, 20); grown sizes in 32-bit pieces with the carry by compare (43, study 30) | NEON's saturating add with the QC flag (37); lazy reduction belongs to mod n | Yes | Yes |
| Negate, conjugate | 0 − x on each part, or on the imaginary part | 0 − x on the lane, check x AND r, since −2^31 overflows (12, study 15); conjugate negates only the imaginary array | None | Yes; −2^31 is an error | Yes |
| Multiply | Four products, ac − bd and ad + bc in 64 bits, then a fit test or none | Widening lane multiply (smull, smlsl), add-form fit test into the sticky flag (15, 28); grown sizes: 32-bit pieces with the K test (43, study 30) | Three products instead of four; the packed multiply; multiply by i as a swap and negate (41); the QC flag (37) | Yes | The 32-bit form is; the grown form is not (see below) |
| Powers | Square and multiply, a branch per exponent bit | None yet | None listed | Open | Open |
| Norm a² + b² | Two widening multiplies and an add | None written; 41 notes it may need twice the width | None | Not as stated: the output is a size up (see below) | Yes if written like the multiply |

The grown-size multiply's K test keeps its speed by an smin detector: if any operand piece comes within 2^31 of ±2^63, the whole array is redone by the exact scalar kernel, once per array. That is a choice made once per array that serves only some inputs, and its outcome depends on the values. For the book it reads as a trick on top of a textbook form that always applies study 23's edge corrections (about 20 instructions per register more). It also puts the rare path in plain registers, which decision 42 asks to justify. For constant time the edge-correction form, or storing h' and accepting the shifted range study 30 counted, would replace it.

The QC flag (37) is filed as a candidate for the checked add and multiply. It is chip specific: RISC-V's vector unit has a comparable sticky saturation bit (vxsat), x86 has none, so it fails decision 4's test for portable pseudocode. It belongs on the tricks list, with the XOR test as the textbook form.

The norm leaves the 32-bit-halves size by construction (2^31² · 2 = 2^63 does not fit int64 signed), and it is a rational integer, not a Gaussian one. Whether it returns (N, 0) one size up, or a plain integer (which decision 6 and 25's conversion rules must then allow), is not decided. Division and the per-lane reciprocal already form N per lane (study 30's l1), so RULES 3 suggests one shared norm kernel.

## 3. Division

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Exact division | x · conj(y), then two 64-bit divides by N | Strip 2^k and one 1 + i, Newton inverse of N mod 2^32, multiply (29, study 25 m2); a shared divisor precomputes once (shape, not a trick) | None listed | Yes at 32-bit halves; grown sizes need the inverse mod 2^(32k), not studied | Yes in lanes (variable shifts and multiplies only); no divisibility check |
| Division with remainder, one shared divisor | Floor or truncating divide per part, two divides each; libraries round ties up | s2: conj(y)/N as two 32-bit fixed-point words, one exact test per part, ties toward zero (28, 44); N < 256 goes to study 29's m4, chosen once per array | s1, remainder in 32-bit lanes, N ≤ 2^59 (44) | Yes, but a quotient can need one more bit (see below) | Per element yes; the N < 256 choice and the resolve pass depend on values |
| Division with remainder, divisors varying per element | The same, per element | l1: per-lane Q30 Newton reciprocal, second stage for N < 2^14 under a mask (45); arrays holding a unit, (1 + i) times a unit or zero go to the plain divide | l2 (two registers) for x86 only | Same as above | No: the unit fallback depends on secret divisors |
| Tie rule | Ties up (floor((2X + N)/2N)), the fixed-box convention | Nearest, ties toward zero, on the magnitude with one sign mask (28); folded into the exact test as one bit (44) | Direct against mask form (reopen list) | Yes | Yes |

The split by call shape (array by one number, array by array, 32) is textbook: they are two operations with different signatures, chosen from the call, never from the values. The other choices inside 44 and 45 are different in kind. N < 256 (s2 needs T ≥ 3) and unit divisors (the quotient needs 33 bits) are holes in the general method's domain, filled by a slower path. They are not tricks, since tricks are faster paths, but the book should present each method with its domain stated and the fallback as part of the textbook answer. For constant time the per-lane fallback would be replaced by computing the unit case in every lane under a mask (the quotient of a unit divisor is ±x or ±ix, of (1 + i) times a unit a halving), and the resolve pass by holding the quotient one size up, or one bit wider, at a fixed width.

A quotient part reaches 2^31 only for unit or (1 + i)-unit divisors and a −2^31 half in x (study 25). Under growth that promotes; at a fixed width it is an overflow error from an exact, in-range division, which a user of a fixed size may not expect. A remainder part can reach 2^31 when |y| is near 2^31.5. Both should be stated in the fixed-width spec.

## 4. Mod n

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Reduction after a product | Hardware remainder (%); bignum crypto libraries use Montgomery for odd n | Barrett for any n (38): array by array by study 28's widening Barrett; array by a shared number by Barrett multiplication with k' = round(k · 2^31/n) and the high-half multiply | Montgomery (odd n, array by array, data kept in form); bar_h30 for n ≤ 2^30; the split form for p ≡ 1 mod 4; lazy reduction | Yes, never overflows (49) | Yes: corrections are unsigned min, no branch |
| Add, subtract, negate mod n | Add, compare, subtract (a branch) | Not written. 41 says Barrett, but no product is involved: one add and one unsigned min (the "reduce after every add" form, open note on mod n) | Lazy reduction (n < 2^29) | Yes | Yes with the min form |
| Multiply mod n | Four products, then % | Barrett as above, one reduction per product (study 28: 44 NEON instructions per 4 Gaussian products array by array, 26 against a shared Gaussian) | Montgomery (22 per 4); summing two products before one reduction at n ≤ 2^30 | Yes | Yes |
| Powers mod n | Square and multiply with a branch per bit | None yet | Montgomery form (repays after a few products) | Yes | Needs a ladder or fixed window over a public exponent length |
| Inverse mod n | Extended Euclid on integers; Fermat's power for prime n | Norm route: conj(x) · N(x)^−1 mod n, the integer inverse by extended Euclid (22) | Batch inversion | Yes | No: Euclid's step count and divides depend on the values |

For Part 2 the inverse is the main gap. Replacements: Bernstein–Yang divsteps for odd n (study 13: about 25 times Euclid's mean, branch-free, fixed count), or a power x^(p − 2) for prime p (needs the power operation). Euclid in 22 is also scalar with hardware divides, against decision 42's vector default, and the hardware divide is not data-independent in time on many cores.

Two places look misfiled. Montgomery is the machine default of the field Part 2 belongs to (most crypto bignum code reduces that way), so for that audience the book's level 1 for mod n is Montgomery, and Barrett is the textbook answer because it accepts any n and every call shape. Batch inversion is the standard textbook method for many inverses mod one n, but it fails the whole batch on one non-unit, which a general per-element operation cannot, so it stays a trick.

Mod n works on residues in [0, n) per part, a canonical representative, while decision 40 keeps a canonical mod out of the textbook version and division with remainder returns symmetric remainders. Reducing x mod n by division with remainder by (n, 0) and by Barrett give different representatives of the same class. The spec should say which one each operation returns and how a user moves between them.

## 5. Predicates

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Equality (23), is zero | Compare words | Not written: XOR (or OR) of both arrays, one lane mask per element | None | Yes | Yes |
| Is a unit | N(x) = 1 | Left out of the first version (48) | None | Yes | Can be |
| Divisibility | Remainder is zero | Left out (48); exact division would add a back-multiply (study 25) | None | Yes | Can be |

Equality across sizes needs a rule: rule B's margin means one value can be stored at two widths, so equality compares values, sign-extending the narrower array, not storage.

## 6. The gcd family

| Operation | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Gcd | Euclid by rounded division, scalar | (1+i)-ary binary gcd in lanes, 3 registers stepped together, may branch (9, 31); pairs with a −2^31 half set aside and redone in scalar 64-bit once per array | None listed (the branched lookup is under bit counting) | Partly: the gcd can be 2^31 (study 26), an error at the base size | No: loop count, and the set-aside pass, depend on the values |
| Bit counting in the gcd | Scalar ctz | Lane clz of x AND −x, no branch (39) | Branch to a table lookup when every lane has k < 4 (33, reopen list) | Yes | Yes |
| Extended gcd | Extended Euclid | To study (41); candidates in study 14 | None | No as studied: (1+i)-ary coefficients reach about 2 bits past the inputs | No |
| Lcm, standard associate, coprimality | xy/gcd; abs; gcd = 1 | Left out (48) | None | Lcm needs the product's width | The associate can be (masks); the others inherit the gcd |

The constant-time replacement for the gcd is the fixed step count, T = 276 at 32-bit halves (study 24, about 11 times slower), but its proof covers only the symmetric range, so −2^31 inputs would still need a branch-free pre-step. Gcds of secret values are rare in practice; the inverse matters more (section 4). The gcd already ends by normalising to re > 0, im ≥ 0, which is the standard associate, so RULES 3 points to one shared kernel when 48's operations return. The −2^31 set-aside in 31 is, like 44's N < 256 path, a hole in the method's domain rather than a trick. Three registers is tuning for one chip family and belongs in the build, not in either level.

## 7. Cross-cutting mechanisms

| Mechanism | Machine default | Textbook (decision) | Tricks | Fixed width | Constant time |
|---|---|---|---|---|---|
| Overflow test and sticky flag | None (wrap), a branch per operation (checked add builtins), or a trap | Mask test in the sweep, ORed into one register, checked once per array; redo at the next size from the untouched inputs (13, 17, 20) | QC flag (37); GPU compaction of failed elements (research list) | Yes, the flag is an error (49) | Yes at a fixed width |
| Promotion and growth | Each number grows by itself, a branch per operation | Whole array redone at the next size, sign in the upper pieces, 32-bit pieces as separate arrays (11, 43) | None | Off (49) | No by nature: the size follows the values |
| Shrink test | Strip leading zero words after each operation | Fused into the sweep of add, subtract, divide and mod, only above the base size, rule B's one-bit margin (18, 26) | None listed | Off (49) | No by nature |
| Layout | Interleaved pairs | Separate real and imaginary arrays (15); pairs for single values and I/O; grown sizes as separate arrays of pieces (43) | None | Yes | Neutral |
| Mod n reduction | % | Barrett (38) | Montgomery, lazy reduction, split form | Yes | Yes |
| Division methods and tie rule | Divide instruction, ties up | Section 3 (28, 29, 44, 45) | s1 | Yes, with the quotient bit noted | Shared yes, per lane no |
| Inverses | Extended Euclid, Fermat | Norm route with Euclid (22) | Batch inversion | Yes | No |
| Gcd and bit counting | Euclid, scalar ctz | Section 6 (31, 39) | Branched lookup (33) | Partly | No, the counting yes |

Shrinking is an optimization: no result depends on it, and it pays only for workloads whose values fall. The operation itself has a case for textbook status (without it a width can only grow, and decision 49 already makes it a separate, blockable operation). Fusing its test into the sweep (18) is the special-case speed-up, and its cost in cache (11 to 31% of a grown add, reopen list) is paid by every caller. A clean split for the book: a shrink pass after the operation at level 2, the fused test at level 3. Shrinking after mod n is decided by n, not by the values, so there it can be computed once per modulus.

## Operations on 41's list with no textbook answer yet

Creation and reading (simple, unwritten); powers, plain and mod n; the norm; add, subtract and negate mod n (41 names Barrett, which does not apply); equality and is zero (simple, unwritten); is a unit, divisibility, lcm, the standard associate and coprimality (left out by 48); the extended gcd; exact division and the gcd at grown sizes; mod n for n of 32 bits or more.

## Open questions this raises

1. Growth step: decision 11 doubles the word (64 to 128), the Defaults line and 43 say larger sizes add 32-bit pieces. Does an array grow by one piece per half or double its pieces?
2. The norm's output: (N, 0) a size up, or a plain integer, and how that fits decisions 6 and 25.
3. Which representative each mod operation returns ([0, n) per part, or 28's symmetric remainder), and the bridge between them, given decision 40.
4. Whether the textbook grown multiply is the edge-correction form, with study 30's smin fallback as a trick.
5. Whether the QC flag (37) leaves the candidate decisions for the tricks list, given decision 4.
6. For constant-time forms: which inputs Part 2 treats as public (the modulus surely; a shared divisor; the exponent's length), since that decides whether the once-per-modulus and once-per-divisor choices (N < 256, s1 against s2) are acceptable.
7. The constant-time inverse: divsteps for odd n, a power for prime n, and what serves even n.
8. Whether every instruction in the textbook kernels is on ARM's data-independent-timing list (FEAT_DIT), and the plain divide fallbacks in 45 and the gcd's set-aside pass, which are not.
9. Fixed-width overflow from in-range exact work (a unit divisor's quotient, gcd = 2^31, the norm): an error, or a documented one-size-up result.
10. Powers without mod n: the output width is known from the exponent, so a width chosen in advance may be the general answer (bound tracking, dropped in 21, was a stored structure; this would not be).
11. Equality across widths, and the entry size for a large integer.
12. Mod n at grown sizes (n ≥ 2^31): Barrett's constants and corrections in 32-bit pieces are unstudied.
