# Survey: (1+i)-adic, complex-base and valuation forms for Gaussian integers, with a focus on addition

Scope: light web survey, November 2026. Setting: exact Gaussian integer x = (1+i)^k · u, u odd (not divisible by 1+i), u stored either as two binary words whose bit pairs pick a unit digit u_j in {1, i, -1, -i} with u = Σ u_j 2^j, or as an ordinary pair (a, b) of binary integers.

## 0. A fact about our own unit-digit code (checked numerically)

Before the literature, one observation that changes how addition looks. Each unit can be written as u_j = ((1+i)/2)(s_j + i t_j) with s_j, t_j in {+1, -1}:

| (s_j, t_j) | (+1,+1) | (+1,-1) | (-1,+1) | (-1,-1) |
|---|---|---|---|---|
| u_j | i | 1 | -1 | -i |

Put s_j = 2a_j - 1, t_j = 2b_j - 1 with bits a_j, b_j, and let A, B be the n-bit words and M = 2^n - 1. Then

  u = (1+i)(A + iB) - M·i,  that is  Re u = A - B,  Im u = A + B - M.

So the unit-digit code is just the plain pair (A, B) of unsigned binary words seen through a fixed 45 degree rotation and scaling by (1+i), plus a constant. This is the Gaussian analogue of the classical fact that an odd integer with all digits in {-1, +1} is 2A - M for an ordinary binary word A. I checked for n = 4 that the map is a bijection from the 256 codes onto the 256 odd points of the diamond |a| + |b| ≤ 15 (script output: 256 256 True). If the project's bit-pair-to-unit assignment differs from the table above, it agrees with it up to one of the 8 symmetries of the square (a per-digit swap or complement of the two bit planes) or else is not affine; it is worth choosing an assignment of this affine kind.

Consequences for addition:

- Conversion is O(1) per word, no carries: (a, b) = (A - B, A + B - M); back: A = (a + b + M)/2, B = (b - a + M)/2 (exact, since a + b and M are both odd).
- Sum of two odd values in this code: u1 + u2 = (1+i)·[(A1 + A2 - M) + i(B1 + B2 - M)]. The first factor of (1+i) comes out for free and the cofactor is an ordinary pair of integers. So the code costs nothing extra compared to the rectangular pair; the two storage options are the same representation up to a linear change of coordinates, and addition is best done in rectangular (or in the (A, B) coordinates, which are rectangular coordinates of (u + Mi)/(1+i)).

Everything below should be read with this in mind: the "digit" view does not need digit-level carry logic at all.

## 1. Positional number systems for Z[i]

Base -1+i, digits {0,1}. Every Gaussian integer has a unique finite expansion (Penney 1965, [A "Binary" System for Complex Numbers, JACM 12](https://projects.csail.mit.edu/jacm/References/penney1965:247.html); NSA declassified version [PDF](https://www.nsa.gov/news-features/declassified-documents/tech-journals/assets/files/a-binary-system.pdf); Knuth, TAOCP vol. 2, 4.1; overview in [Complex-base system](https://en.wikipedia.org/wiki/Complex-base_system)). Base -1-i is the conjugate system. Kátai and Szabó (1975) showed that -n ± i with digits {0, ..., n²} are exactly the canonical number systems for Z[i] ([Acta Sci. Math.; discussed with Gilbert's work here](https://www.math.uwaterloo.ca/~wgilbert/Research/GilbertDivAlg.pdf)). Gilbert studied arithmetic, division and multiple expansions in complex bases ([Arithmetic in Complex Bases, Math. Mag. 1984](https://katalog.lib.cas.cz/KNAV/EdsRecord/edsjsr,edsjsr.10.2307.2689587); [The division algorithm in complex bases](https://www.cambridge.org/core/journals/canadian-mathematical-bulletin/article/division-algorithm-in-complex-bases/ACC1178476AB5587B648D2E0AC8F5AC6); [radix representations of quadratic fields](https://www.math.uwaterloo.ca/~wgilbert/Research/GilbertRadixQuad.pdf)).

Addition: since 2 = (-1+i)^3 + (-1+i)^2, the digit rule is 1 + 1 = 1100, so a carry from position j lands at positions j+2 and j+3. There is also a "zero rule" 11 + 111 = 0 (i + (-i)). Carries can chain and spawn further carries, and the expansion of a sum can be noticeably longer than either input; negation is not a bit flip. A ripple adder therefore needs a two-position carry state and is slower and less regular than a binary adder ([Jamil et al., WCECS 2010](https://www.iaeng.org/publication/WCECS2010/WCECS2010_pp832-837.pdf)).

Base 2i (quater-imaginary, Knuth 1960), digits {0,1,2,3}. Even positions form a base -4 number for the real part, odd positions a base -4 number times 2i for the imaginary part ([Quater-imaginary base](https://en.wikipedia.org/wiki/Quater-imaginary_base); [Imaginary Number Bases, arXiv:1701.04506](https://arxiv.org/pdf/1701.04506.pdf)). Addition splits into two independent negabase-4 additions on interleaved digits; carry of 4 is 10300 (16 - 12). Practically this is just rectangular arithmetic with an odd radix sign.

Base 1+i or 2 with unit digits. The (1+i)-adic view underlies the (1+i)-ary GCD (Weilert, [ANTS 2000 / JSC](https://www.bohrium.com/paper-details/1-i-ary-gcd-computation-inz-i-as-an-analogue-to-the-binary-gcd-algorithm/812456148325105666-885)), and FLINT exposes it directly as `fmpzi_remove_one_plus_i` (strip the largest power of 1+i and return k) on a rectangular pair representation ([FLINT fmpzi docs](https://arblib.org/fmpzi.html)). I found no published system that uses exactly base 2 with digit set {±1, ±i} as a storage format; Section 0 shows why: it is an affine relabeling of binary pairs, so it has no independent arithmetic.

Redundant and NAF forms. With base 1+i and digits {0, ±1, ±i}, an odd x has a unique unit d with x ≡ d mod (1+i)^3, which yields a width-3 non-adjacent form (each nonzero digit followed by at least two zeros). Width-w NAFs for imaginary quadratic bases, including optimality of weight, are treated by Heuberger and Krenn ([JTNB 2013, arXiv:1110.0966](https://arxiv.org/pdf/1110.0966); [analysis paper](https://graz.elsevierpure.com/en/publications/analysis-of-width-w-non-adjacent-forms-to-imaginary-quadratic-bas)). Heuberger also showed that minimal redundant expansions in Gaussian CNS are not determined locally by low digits, unlike in Z ([JTNB 2002](https://jtnb.centre-mersenne.org/item/JTNB_2002__14_2_517_0)). The tau-adic NAF of Solinas for Koblitz curves (base τ with τ² - μτ + 2 = 0, digits {0, ±1}) is the closest cryptographic analogue ([Solinas, DCC 2000](https://portal.mardi4nfdi.de/wiki/Item:Q1976040); [Avanzi, Heuberger, Prodinger](https://www.math.tugraz.at/~prodinger/Avanzi-Heuberger-Prodinger-Alternative-Tau.pdf)). These forms optimize weight for scalar multiplication; they do not make addition of two numbers easier.

Parallel (carry-free) addition. Frougny, Pelantová and Svobodová show that base -1+i and base 2i both admit parallel, constant-depth addition on any 5-digit alphabet containing 0 (5 is minimal), for example {-2, ..., 2}; Herreros showed base -1+i works on {-1, 0, 1} with a block-based local function ([arXiv:1610.08299](https://arxiv.org/pdf/1610.08299); [earlier parallel addition paper](https://ar5iv.arxiv.org/html/1102.5683)).

| System | Digits | Carry pattern | Carry-free possible |
|---|---|---|---|
| base -1+i | {0,1} | 2 = 1100, to j+2, j+3 | with 5 digits, e.g. {-2..2} |
| base 2i | {0..3} | 4 = 10300, to j+2, j+4 | with 5 digits |
| base 1+i | {0,±1,±i} | redundant, NAF exists | yes (redundant) |
| base 2 | {±1,±i} | affine image of binary pair | as binary pair |

## 2. Valuation-and-unit ("exact floating point") forms

p-adic floating point. Caruso's survey describes floating p-adic numbers as p^v · (unit, mod p^N) and analyses precision under addition, including the ultrametric fact that v(x+y) = min(v(x), v(y)) whenever v(x) ≠ v(y), with cancellation (and loss of relative precision) only when valuations are equal ([Caruso, Computations with p-adic numbers, arXiv:1701.06794](https://arxiv.org/pdf/1701.06794); [JNCF lecture notes](https://jncf2017.lip6.fr/files/lecture-notes/jncf2017-caruso.pdf); Roe on precision tracking in Sage, [notes](https://math.mit.edu/~roed/writings/research/padicprec.pdf)). Sage's capped-relative and floating-point p-adic types are this exact layout.

Hensel codes. Krishnamurthy, Rao and Subramanian, and Gregory, represent rationals by truncated p-adic expansions (a valuation plus N digits), doing exact arithmetic mod p^N and recovering rationals by Farey reconstruction ([Finite segment p-adic number systems](https://repository.ias.ac.in/28162); [Hensel code to Farey rationals](https://repository.ias.ac.in/28201); [overflow detection](https://www.academia.edu/20587888/A_method_for_Hensel_code_overflow_detection)). Addition there is just modular addition after aligning valuations; the hard part is overflow detection, not carries.

Logarithmic number systems store sign and log magnitude; multiplication is addition of logs, addition needs the nonlinear function log(1 + 2^d), which is tabulated and inexact. Our valuation form is the exact discrete cousin: only the "2-part" is logarithmic and the unit part stays exact, so addition is exact but needs alignment.

Gaussian (1+i)-adic normalization. For a + bi, v_{1+i} = 2·min(ctz a, ctz b) + [ctz a = ctz b]. Division by (1+i)^2 = 2i is a shift and a rotation; an odd remaining power costs one (a + bi)/(1+i) = ((a+b) + i(b-a))/2. This is what Weilert's GCD and FLINT's `fmpzi_remove_one_plus_i` do.

## 3. Hardware and software computing in such forms

Jamil and coworkers built ripple adders, subtractors, nibble-size multipliers and dividers for base -1+j on FPGAs and argued for a single-unit complex ALU ([WCECS 2010](https://www.iaeng.org/publication/WCECS2010/WCECS2010_pp832-837.pdf); [nibble multiplier FPGA](https://squ.elsevierpure.com/en/publications/nibble-size-multiplier-circuit-designs-and-their-fpga-implementat); [design and implementation](https://squ.elsevierpure.com/en/publications/design-and-implementation-of-a-nibble-size-multiplier-for-1j-base)). The reported comparisons are mostly at nibble size and do not show an advantage for addition over a pair of binary adders; the carry rule 1 + 1 = 1100 makes the adder wider and slower per digit.

Redundant complex number systems. Aoki, Ohki and Higuchi's RCNS uses radix 2j with digits {-3..3} or {-2..2}, giving carry-free complex addition and regular multiplier arrays ([VLSISP 1995](https://manuscript.isc.ac/Inventory/49/1575567.htm)); a redundant base -1+j variant with digits {-3..3} is in [this paper](https://www.iiisci.org/Journal/PDV/sci/pdfs/P859267.pdf). Duprat, Herreros and Kla proposed the "polygonal" redundant representation of complex numbers and vectors with carry-free addition suited to multiplication ([IEEE TC 42(7), 1993](https://search.isc.ac/inventory/49/954724.htm)). Note: Arno and Wheeler's 1993 IEEE TC paper is on minimal-weight signed-digit representations of ordinary integers, not complex numbers; the redundant complex multiplier work is Aoki et al. and Duprat, Herreros, Kla.

Reported costs in all of these are VLSI area and delay; the carry-free versions win on latency by removing carry chains, which on a CPU or SIMD lane is irrelevant because a 32-bit binary add is already one instruction.

## 4. When addition in a valuation form is cheap

- Unequal valuations. With k < l, x + y = (1+i)^k (u + (1+i)^(l-k) w), and u + (even) is odd, so the result valuation is exactly k and no renormalization is needed. Only alignment (shift, rotate, maybe one (1+i) multiply) and growth of the cofactor (by about (l-k)/2 bits) cost anything.
- Equal valuations. The sum of two odds is (1+i) times a rectangular pair (Section 0), and further cancellation is found with two ctz and a compare; the shift is variable but predictable from the low bits. In random data the extra valuation beyond 1 is geometric with mean about 1, so the renormalization shift is usually small.
- Lazy normalization. p-adic libraries (capped-absolute, fixed-modulus types) and Hensel-code systems simply do not normalize after each operation; they keep a common valuation and an unnormalized unit and normalize at output or before division ([Caruso](https://arxiv.org/pdf/1701.06794)). For sums of many terms, align everything to the minimum valuation once, accumulate in a wide rectangular register, normalize once.
- Redundant forms make every addition carry-free (Section 1, 3), but only matter when carry propagation is the bottleneck, which it is not with native word adds.

## Ideas for our addition

1. Treat the unit-digit code as rectangular coordinates. Use the affine map of Section 0 (pick the bit-pair assignment that makes it affine) so that adding or converting is a few integer adds per lane, never digit logic. Equivalently, store the rectangular pair and keep the unit-digit code only as an interchange or packing format.
2. Add by valuation case. If k ≠ l, align the higher-valuation operand down (shift by floor(d/2), rotate by i^floor(d/2) via lane swap and negate, one (a-b, a+b) step if d is odd) and add; result valuation is min(k, l) with no normalization. If k = l, add, then renormalize with v = 2·min(ctz a, ctz b) + [ctz a = ctz b]. Measure how often each branch occurs in our workloads; with SIMD, do both and blend.
3. Lazy normalization. Allow u to be non-odd between operations, with k a lower bound on the valuation, and normalize only before multiplication chains, comparisons, output, or when a headroom check fails. This turns most adds into plain lane adds plus an aligned shift.
4. Batch accumulation. For sums of many terms (dot products, reductions), align all terms to the minimum k, accumulate in 64-bit rectangular lanes, normalize once at the end.
5. Overflow guard, not carry logic, is the real cost. Exactness means alignment grows the cofactor upward (unlike floating point, nothing can be dropped). Use a cheap bound such as max(|a|, |b|) plus shift amount against 31 bits, falling back to a wide path; this mirrors the overflow-detection problem of Hensel codes.
6. Lane primitives (32-bit units): ctz per lane via popcnt((x & -x) - 1) or lzcnt on AVX-512 (VPOPCNTD / VPLZCNTD), variable per-lane shifts (VPSLLVD / VPSRAVD on AVX2), rotation by powers of i as swap plus sign mask. All are single-cycle-class operations, so a branch-free add with renormalization should be on the order of 10 to 15 vector instructions.
7. Skip complex-base digit systems (-1+i, 2i) and redundant complex digit sets as storage. They solve hardware carry latency, which native adds already eliminate, and they complicate valuation extraction and overflow checks. The width-3 (1+i)-NAF is worth keeping in mind only if we ever need low-weight multipliers (for example, scalar multiplication by fixed Gaussian constants).
