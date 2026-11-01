# Expressions at the machine level: prior art for Design A

Survey written 1 November 2026. Scope: whether bounded sums of differently-directed numbers (Design A's "expressions"), and the magnitude-plus-direction representation they are built from, have been represented or evaluated at the machine level, and what existing systems teach about layout, tagging, normal forms and operations. Method: web search and direct reading of papers, manuals, header files and design notes, preferring primary sources. Where I could reach only a secondary page or only metadata, I say so. Where I could not find something, I say that too. Absence of a finding is not evidence that nothing exists.

A short restatement of what is being compared. In Design A a number is an unsigned magnitude plus a 2-bit direction d (0, 1, 2, 3 meaning 1, i, -1, -i), held in a separate header byte with a size class. Multiplying directions adds d mod 4. Numbers on different axes combine into an expression with at most one term per axis, flagged by a header bit. Design B stores an integer as a pair (P, N) with value P - N.

## 1. Expressions, symbolic data and complex values at the machine level

### Lisp machines and tagged architectures

The Symbolics 3600 technical summary (October 1985) describes 36-bit internal data paths split into 32 data bits and 4 tag bits, with run-time type checking done "concurrently with instruction fetch, decode, and execution." The later Symbolics Ivory paper (1987) says tag checking "is done in parallel with the ALU operation, so that in the common cases no time penalty is paid for type checking," and lists a "parallel tag processor, late-branch ROM and comprehensive trap logic" to support generic arithmetic. The I-Machine Architecture Specification (revision 2, dated December 1986) is the most directly relevant: its number types are dtp-fixnum, dtp-small-ratio, dtp-single-float, dtp-double-float, dtp-bignum, dtp-big-ratio, dtp-complex and dtp-spare-number. A dtp-complex word is a tagged reference whose address points to a two-word cons: the car holds the real part and the cdr the imaginary part. The specification notes that the hardware recognises dtp-complex as a number but may not perform arithmetic on it; non-fixnum cases trap out to software handlers.

That is structurally the same move as Design A's expression flag: a tag in the word says "this is a sum of a real and an imaginary part" and the payload points to the parts. The difference is that the Ivory complex holds signed parts, not magnitudes with directions, and the hardware only accelerated the fixnum and single-float fast path.

SPUR (Berkeley, mid to late 1980s) carried 6-bit tags with 32-bit data; its integer add wrote the result only if both operands were tagged integers and otherwise trapped. SOAR (Smalltalk on a RISC) had 8-bit tags on 40-bit registers. SPARC's TADDcc and TSUBcc tagged add and subtract instructions check the low tag bits of 30-bit fixnums, with trapping variants (TADDccTV). A 1994 comp.arch summary by Bryan O'Sullivan reports that CMU CL used them mostly for type checks, and that in a SPARC-versus-MIPS comparison the tagged-arithmetic advantage was "more or less negated" by MIPS's branch-on-equal. Steenkiste and Hennessy's ASPLOS 1987 paper "Tags and Type Checking in Lisp: Hardware and Software Approaches" is the standard reference for this tradeoff; I could reach only its bibliographic record, not its figures.

The newest data point is Stephen M. Watt's "Look Before You Leap: Checking In on Type Tag Checking" (arXiv 2606.05466, 2026). On current AArch64 and x86-64 machines it finds that a tag test in the value word is roughly 10 to 30 times faster than reading a type from a heap header in tag-only loops, that immediate values are 2 to 5 times faster than stored payloads, and that "the first discriminator should preferably be in the value word itself." It does not measure separate tag arrays.

What Design A can learn: every tagged machine that succeeded did a cheap common case inline and pushed everything else (bignums, ratios, complex numbers) to a trap or a software routine. Design A's "both operands plain, same axis" case is that common case. Complex values never got arithmetic hardware on Lisp machines; they were a tag plus a pointer to a pair, which is what Design A proposes.

### Reduction and rewriting machines

The Reduceron (Naylor and Runciman, University of York, first paper around 2007, ICFP 2010 and JFP follow-ups) is an FPGA graph-reduction machine. Its 2007 design widened memory so a whole template could be read or written in one cycle and used five parallel memories. The 2010 "Reduceron Reconfigured" paper adds primitive-redex speculation: primitive arithmetic such as `(-) 5 1` is evaluated during function-body instantiation instead of being built as a heap node, because "primitive redexes need not be constructed in memory, nor fetched again when needed." It reports that 85% of primitive-redex candidates turned out to be primitive redexes, and that with update avoidance and PRS the normalised run time fell to 0.40 of baseline.

SRI's Rewrite Rule Machine (Lincoln, Meseguer, Ricciulli, around 1994) stored one term vertex per cell, with a label register and two or three child-address registers, and matched and replaced rule instances in SIMD across cells.

What Design A can learn: the Reduceron's measured win came from not materialising arithmetic nodes that collapse immediately. The analogue is to compute the result of a same-axis operation straight into a plain number and to build an expression only when two axes genuinely survive.

### Hardware for computer algebra

I found systolic and VLSI proposals for polynomial operations, not for expression representation: Brent and Kung, "Systolic VLSI arrays for polynomial GCD computation" (IEEE Transactions on Computers, 1984), and a SYMSAC 1981 paper "Use of VLSI in algebraic computation: some suggestions" (DOI 10.1145/800206.806399), proposing chips for polynomial multiplication and division; the records I reached did not show its authors. Later work (for example Emeliyanenko's 2012 thesis) uses GPUs for exact polynomial arithmetic. I found no processor that represents computer-algebra expressions (sums, products, symbolic terms) as a native data type beyond the Lisp-machine tag-and-pointer approach.

### Complex-number instructions

ARM's FCMLA (Armv8.3 Advanced SIMD and SVE) has a 2-bit rot field: 00, 01, 10, 11 select rotations of 0, 90, 180 and 270 degrees. Rotations 0 and 180 multiply by the real part of the first source, 90 and 270 by the imaginary part, and the pair FCMLA #0 then FCMLA #90 accumulates (a+bi)(c+di). FCADD, and SVE2's integer CADD, rotate the second operand by 90 or 270 degrees before adding, which the manual states is "equivalent to multiplying the complex numbers in the second source vector by ±j." SVE2 also has integer CMLA, with complex numbers held as even/odd element pairs (real in the even lane), and results truncated to element width in the pseudocode. On x86, AVX512-FP16 provides VFMULCSH/VFCMULCSH and packed forms for FP16 complex multiply and conjugate multiply. TI's C64x DSP family has a CMPY packed complex multiply (I saw only forum and datasheet references, not the instruction manual text). For RISC-V I did not find a ratified complex-arithmetic extension; a 2023 GCC discussion on vectorising complex arithmetic mentions AArch64 and Kalray KVX instructions but not RISC-V.

What Design A can learn: ARM's rot encoding is the same 2-bit code as Design A's direction, with the same meaning (multiply by i to the power rot), but it lives in the instruction as an immediate, not in each value. All the hardware stores complex data as a dense interleaved (real, imaginary) pair of signed values. None of it keeps a per-value quarter-turn tag.

### Clifford-algebra hardware: the closest structural match

Perwass, Gebken and Sommer, "Implementation of a Clifford Algebra Co-processor Design on a Field Programmable Gate Array" (submitted August 2002), store a multivector as a list of basis blades, each with a scalar factor (a 23-bit integer plus a separate sign bit) and a bitmask naming the basis vectors in the blade. The blade product is a bitwise XOR of the masks; the sign is combined from the operands' sign bits, a signature register (which basis vectors square to -1) and a count of reordering swaps. "Each blade has a unique blade index, so that the latter can be used as a memory offset address." They found the 20 MHz FPGA only about 1.5 times faster than Gaigen-generated software at equal clock, mainly for lack of native arithmetic. Fontijne's reference implementation (2005 to 2007, accompanying his 2007 Amsterdam thesis "Efficient Implementation of Geometric Algebra") does the same in software: `bitmap = a.bitmap ^ b.bitmap` and a sign from `canonicalReorderingSign`.

The complex numbers are the Clifford algebra with one generator squaring to -1. In that view Design A's direction d is exactly (sign bit, blade bit): write d = 2s + a, with a the axis bit and s the sign bit. Then multiplying directions is a = a1 XOR a2 and s = s1 XOR s2 XOR (a1 AND a2), which is the coprocessor's XOR rule with the signature term i·i = -1, and is the same as adding d mod 4. A Design A expression with at most one term per axis is a multivector with at most one coefficient per blade. So the representation (unsigned magnitude, separate sign, basis label, bounded sum with one slot per basis element, product by XOR of labels) was built in hardware for geometric algebra in 2002.

### Phases mod 4 elsewhere

Aaronson and Gottesman's stabilizer-circuit simulator ("Improved Simulation of Stabilizer Circuits," Physical Review A, 2004) tracks the phase of Pauli products by summing exponents of i mod 4 (its function g "returns the exponent to which i is raised"), then stores the result as a single sign bit because the sum is always 0 or 2 for valid stabilizers. This is the direction-add-mod-4 rule used as a low-level bit trick in a performance-critical simulator.

## 2. Multi-term machine-level number representations

### Floating-point expansions

Shewchuk ("Robust Adaptive Floating-Point Geometric Predicates," SoCG 1996; full version in Discrete & Computational Geometry 18:305-363, 1997) represents a value as an expansion: an unevaluated sum of ordinary floating-point numbers that are nonoverlapping and sorted by magnitude. The sign of the whole is the sign of the largest component. Expansions are not unique. His sum and scale algorithms produce nonoverlapping output without a separate renormalisation pass, but "there may be spurious zero components scattered throughout the output expansion," so zero elimination is required for speed; the 2D orientation test's exact value can need up to sixteen components but typically two to six after elimination.

Hida, Li and Bailey ("Quad-Double Arithmetic: Algorithms, Implementation, and Application," 30 October 2000) define a quad-double as "an unevaluated sum of four IEEE double precision numbers" with |a(i+1)| at most half an ulp of a(i). Under that condition the representation is unique. Operations produce five terms and then renormalise back to four; the paper notes the conditions under which renormalisation is guaranteed are not fully known. Multiplication accumulates 13 partial terms. They use FMA for exact products where available.

What Design A can learn: expansions are bounded sums with a fixed component budget and a normalisation rule, which is what a Design A expression is. Two lessons transfer directly. First, zero components must be removed eagerly or every later operation pays for them. Second, uniqueness only holds if a normalisation invariant is stated and enforced after every operation. Design A has an advantage expansions lack: its components live in distinct "slots" (axes), so there is no overlap problem and no growth, as long as the set of directions stays closed under multiplication.

### Redundant and signed-digit systems (relevant to Design B)

Avizienis ("Signed-digit number representations for fast parallel arithmetic," IRE Transactions on Electronic Computers, vol. 10, 1961) introduced digit sets including negative digits so that addition is carry-free. Borrow-save, attributed in Bajard's 2007 slides to Duprat and Muller (1989), is the radix-2 case. In redundant binary, each digit is a two-bit pair and several encodings mean zero, so most integers have several representations; addition is constant-time but conversion to ordinary binary needs a prefix adder. Jaberipur, Parhami and Ghodsi (2001) note for redundant systems generally that the reverse conversion "involves word-width carry propagation."

Design B's (P, N) with value P - N is a word-level version of the same idea. It shares the same costs: values are not unique, sign and comparison need a full subtraction, and a canonical form requires reducing to (P - N, 0) or (0, N - P), which is sign-magnitude again.

### Residue and quadratic residue number systems

Krishnan, Jullien and Miller published on complex digital signal processing with quadratic residue number systems (ICASSP 1985, and "Computation of complex number theoretic transforms using quadratic residue number systems," ICASSP 1986). I could not reach the full texts in this session. From general knowledge, not verified here: for primes p with p ≡ 1 mod 4 there is j with j squared ≡ -1, and a + bi maps to the pair (a + jb, a - jb), turning a complex product into two independent modular products. This is a different answer to the same problem Design A attacks (making the two axes multiply cheaply), and relevant only if Design A ever adds modular arithmetic.

### Sign-magnitude integers

Sign-magnitude with the sign outside the magnitude is standard in big-integer libraries. GMP's `_mp_size` is "the number of limbs, or the negative of that when representing a negative integer"; zero is size 0; the top limb is always nonzero. CPython 3.12's `lv_tag` uses its low two bits as a three-valued sign, "negative=2, positive=0, zero=1," with the digit count above them, and keeps the magnitude unsigned. Maple's 1980s design used a separate type tag for negative integers (Char, Geddes, Gentleman and Gonnet, "The Design of Maple," about 1983). Design A generalises exactly this: the sign field becomes a 2-bit direction, and the magnitude stays unsigned.

### Gaussian integers and complex bases

FLINT's `fmpzi` stores a Gaussian integer as a pair of `fmpz` (real and imaginary), and `fmpzi_canonical_unit_i_pow` "returns k in {0,1,2,3} such that i^k x is the canonical associate of x" (positive real part, imaginary part in (-a, a]). That is the number-theoretic decomposition into unit times normalised associate, with the unit stored as a quarter-turn exponent, but FLINT uses it as a function result, not as the stored form. Knuth's quater-imaginary base (1960) writes complex numbers in base 2i with digits 0 to 3 and no sign. Giac stores complex values as a pointer to a reference-counted pair of `gen` (real and imaginary), per third-party documentation generated from its `gen.h`; I could not open the Giac source itself.

I found no hardware or low-level library that stores a Gaussian integer as unsigned magnitude plus a 2-bit unit exponent.

## 3. How compiled computer-algebra kernels represent expressions

Maple. The early design paper (about 1983) says every structure has a header encoding its length, type, one bit of simplification status and two bits of GC status, and that Maple "maintains a unique copy of every expression and subexpression computed" through a simplification table keyed by an order-independent signature, so a+b+c and c+a+b collide. Monagan and Pearce (PeerJ preprint, 17 September 2014) describe the modern form: a header word with type and length; SUM as a sequence of (term, coefficient) pairs; PROD as (base, exponent) pairs; small integers as immediate 2x+1 in a tagged word; and the global simpl table so equality is "a pointer comparison which costs one clock cycle." Maple 17 added POLY, which packs a whole monomial's exponents and total degree into one machine word so that monomial comparison is a word compare and monomial product is a word add, used only when the degrees fit; otherwise Maple falls back to sum-of-products, converting automatically.

GiNaC (Bauer, Frink and Kreckel, arXiv cs/0004015, 2000). Sums and products share a class `expairseq`, an "unexpanded distributive" representation with numeric coefficients kept apart from the rest of each term. Built-in rules bring sums and products to a canonical form on construction; expansion is left to the user. Objects are reference counted and copy-on-write, subexpressions are shared, and hash values speed up comparison. The current `status_flags` include `evaluated`, `expanded`, `hash_calculated`, `not_shareable`, and sign hints `is_positive` and `is_negative`.

Mathematica (Wolfram documentation, "Some Notes on Internal Implementation," undated). Expressions are contiguous arrays of pointers, the first to the head. Each expression carries a hash code used in pattern matching and evaluation. Memory is reference counted, unique copies are kept of small integers and some approximate numbers, and numeric lists are stored as packed arrays. Complex-number layout is not described.

FLINT and Calcium. FLINT's `fmpz` is one word: values up to COEFF_MAX (2^62 - 1 on 64-bit) are stored directly; otherwise the word holds a shifted pointer to a GMP `mpz`. Values that fit "must always be stored inline," and all functions expect and produce that canonical form. Calcium (Johansson, arXiv 2011.01728, 2020; now in FLINT 3) represents a number as `{ ulong field; elem }`, where the element is a rational, a number-field element or a multivariate rational function over a field Q(a1, ..., an) whose generators carry an ideal of algebraic relations. The low two bits of the field pointer encode special values (unknown, undefined, infinity). Calcium's signed infinities are e^(iθ)·∞, a direction times infinity, and "there are no rectangular signed infinities": +∞ + i∞ is undefined (Johansson's blog, 25 January 2021).

SymPy, for contrast. Expressions are trees of immutable `Basic` objects; construction calls `eval()` to return a canonical form, and the old "How It Works" wiki page says sum terms are ordered by hash. That page is undated and old, so treat details as historical.

Do any of these keep small expressions inline in a word? Maple keeps integers and (in POLY) monomials in a word; FLINT keeps integers; Calcium keeps special-value tags in pointer low bits; Giac keeps machine integers and doubles in its union. I found no kernel that keeps a two-term sum such as 3 + 4i inline in a single word. Complex numbers are consistently a pointer to a pair.

## 4. Small-value and tagging techniques

Gudeman's 1993 report "Representing Type Information in Dynamically Typed Languages" (Arizona TR 93-27; summary posted to comp.compilers, 30 April 1993) gives the taxonomy: high tags, low tags, tags at both ends, NaN-encoding, table lookup by memory segment (BiBOP), two-word descriptors with a separate type word, and "segregated type codes" stored apart from the value.

Low-bit tagging: OCaml stores ints shifted left with the low bit set; boxed blocks have a one-word header with size, 2 GC colour bits and a tag byte. V8 tags Smis with a 0 low bit; with pointer compression (blog post, 30 March 2020) Smis are 31-bit and heap size drops by up to 43%. NaN-boxing: SpiderMonkey's `JS::Value` is 64 bits on every platform, storing doubles directly and other types in NaN payloads, with "only 47 bits" needed for pointers. Sign in a separate field: GMP and CPython as above. Small-integer inline with mandatory demotion: FLINT `fmpz`. I could not retrieve Julia's manual in this session, so I make no claims about Julia.

Separate tag arrays: Zig's standard `MultiArrayList` "stores separate lists for each field of the struct or lists of tags and bare unions," citing padding savings and better cache use "if only some fields or just tags are needed for a computation." That is the structure-of-arrays form of Design A's header byte. Watt (2026), on the other hand, measures that a tag in the value word is the fastest first discriminator. The two are not in conflict: side arrays win when loops scan many tags without touching payloads; in-word tags win when each value is inspected individually.

What Design A can learn: a 1-byte header outside the word is a segregated tag in Gudeman's terms and a MultiArrayList in Zig's. It buys a full 64-bit magnitude (no bits stolen), which none of the in-word schemes allow. It costs a second memory access per value, unless headers and words for the same values are fetched together.

## 5. What looks new about Design A

Each ingredient has precedent. Sign kept outside an unsigned magnitude: GMP, CPython, early Maple. A 2-bit quarter-turn code multiplied by adding mod 4: ARM's FCMLA rot field, FLINT's canonical unit exponent, the Aaronson-Gottesman phase sum. Unsigned magnitude plus sign plus basis label, with products by XOR and a bounded one-slot-per-basis sum: the Perwass-Gebken-Sommer Clifford coprocessor and Fontijne's blade representation. A tag bit meaning "this word points to a real/imaginary pair": Symbolics dtp-complex. Tags in a separate array: Gudeman's segregated codes, Zig's MultiArrayList. Bounded unevaluated sums with a normal form: double-double and quad-double, Shewchuk expansions.

What I did not find is a general-purpose exact number representation that stores a per-value 2-bit direction in a header separate from an unsigned magnitude, makes the magnitude the only thing arithmetic routines touch, and marks a bounded one-term-per-axis expression with a bit in the same header, implemented as portable instruction-level routines. The closest system is the Clifford coprocessor, which is a geometric-algebra engine rather than a number representation, uses a list rather than fixed slots, and was not portable code. My searches were limited to what an English-language web search and the pages I could open would show, so this is a statement about what I found, not a claim of novelty.

## Lessons for Design A

1. Make the canonical form mandatory, as FLINT does for `fmpz`: a value that fits the plain form must never be stored as an expression. An expression whose real or imaginary term becomes zero collapses immediately to a plain number, and functions may assume their inputs are canonical. (FLINT `fmpz` documentation; Shewchuk on zero elimination.)

2. Fix the representation of zero. GMP uses size 0; CPython gives zero its own sign value. Design A has four ways to write magnitude 0 (one per direction); pick one (d = 0) and enforce it, or equality tests and hashing break. (GMP manual; CPython `pycore_long.h`.)

3. Store an expression's terms in fixed slots by axis, not as a list. With at most one term per axis, slot 0 is the real axis and slot 1 the imaginary axis, as Perwass et al. use the blade index as a memory offset and SVE2 uses even/odd lanes. Each term then needs only a sign bit relative to its axis, and the expression is a signed (real, imaginary) pair of magnitudes. (Perwass, Gebken, Sommer 2002; ARM SVE2 CMLA.)

4. Implement direction products as bit operations. With d = 2s + a, the product has a = a1 XOR a2 and s = s1 XOR s2 XOR (a1 AND a2). This is the Clifford coprocessor's rule and is equivalent to adding mod 4; either form is a few instructions on all three ISAs. Note that ARM uses the same 2-bit code in FCMLA's rot field, which can serve as a cross-check in tests. (Perwass et al.; Fontijne `BasisBlade`; ARM FCMLA.)

5. Keep the common case inline and route everything else to a slower path, as Lisp machines and SPUR did with traps. The hot path is "both plain, same axis." Check the expression bit once, before touching magnitudes. Be wary of investing in hardware-style tricks: on SPARC the tagged-add advantage was largely negated by an ordinary compare-and-branch. (Symbolics Ivory 1987; I-Machine specification 1986; O'Sullivan 1994.)

6. Do not build an expression only to collapse it. Compute same-axis results straight into a plain number, and build the two-term form only when both axes survive. This is the Reduceron's primitive-redex speculation applied to arithmetic. (Naylor and Runciman 2010.)

7. Count multiplications honestly. A general expression product is four magnitude products plus collection, the same four products ARM's FCMLA #0 / #90 pair performs. FLINT's notes mention three-multiplication tricks for Gaussian squaring; whether a Karatsuba-style product helps depends on magnitude size and on handling signs in unsigned arithmetic, and should be measured rather than assumed. (ARM FCMLA; FLINT `fmpzi`.)

8. Decide tag placement by measurement. The separate header byte keeps all 64 magnitude bits, which in-word schemes cannot. Watt's 2026 measurements favour in-word discriminators for single-value dispatch; Zig's MultiArrayList argues for separate tag arrays when loops scan tags. Benchmark Design A's actual access pattern (scanning headers versus touching both). (Watt 2026; Zig `MultiArrayList`; Gudeman 1993.)

9. Use the header for cached facts, as Maple and GiNaC do. Maple reserved a simplification-status bit; GiNaC caches `evaluated`, `expanded` and `hash_calculated`. If Design A's header has spare bits beyond the expression flag, "known canonical" or "hash cached" are proven uses. (Char et al. about 1983; GiNaC `flags.h`.)

10. Consider sharing for pooled magnitudes. Maple's unique-copy table makes equality a pointer compare; Mathematica shares small integers; GiNaC shares subexpressions with copy-on-write. If Design A's pool of large numbers is immutable, hash-consing it gives cheap equality, at the cost of a global table. (Monagan and Pearce 2014; Wolfram internal-implementation notes; Bauer et al. 2000.)

11. Do not assume a carry flag. RISC-V has no condition codes; multiword magnitude addition detects carry with `sltu` after the add. Routines meant for x86-64, ARM64 and RISC-V should be written around explicit carry computation. (Project F, "RISC-V Branch and Set," 2024.)

12. Know where the bound breaks. Design A's two-term bound holds because {1, i, -1, -i} is closed under multiplication. Adding any direction outside that group (for example an eighth root of unity) breaks the bound, and the honest model for the general case is Calcium's field towers with relations. Calcium also refuses to give meaning to rectangular combinations of infinities, a useful precedent if Design A ever adds infinities. (Johansson 2020; Johansson blog 2021.)

13. For Design B, expect borrow-save costs. A (P, N) pair has many representations, needs a full subtraction to find its sign or compare it, and reaches a canonical form only by reducing to sign-magnitude. Its gain, borrow-free addition, matters most in hardware adders, less in software where a 64-bit add is a single instruction. (Avizienis 1961; Jaberipur, Parhami, Ghodsi 2001; redundant binary representation overview.)

## Sources

Lisp machines and tagged architectures
- [Symbolics Technical Summary](https://smbx.org/symbolics-technical-summary/), Symbolics Inc., October 1985 (transcription).
- [The Symbolics Ivory: A 40 Bit Tagged Architecture Microprocessor](https://www.Gwern.net/doc/cs/hardware/1987-baker.pdf), Baker et al., IEEE, 1987 (OCR scan).
- [I-Machine Architecture Specification, Revision 2](https://ftp.mirrorservice.org/sites/www.bitsavers.org/pdf/symbolics/I_Machine/I-Machine_Architecture_Specification.pdf), Symbolics, December 1986.
- [Tags and Type Checking in Lisp: Hardware and Software Approaches (bibliographic record)](https://bibtex.github.io/ASPLOS-1987-SteenkisteH.html), Steenkiste and Hennessy, ASPLOS II, 1987. Full text not read.
- [Summary of responses: Lisp Machine architecture and other h/w support](https://cdimage.debian.org/mirror/archive/ftp.sunet.se/pub/lang/lisp/doc/notes/lispm.txt), Bryan O'Sullivan, comp.arch, 3 February 1994.
- [SPUR, RISC IV: The LISP Multiprocessor Workstation](https://thechipletter.substack.com/p/spur-risc-iv-the-lisp-multiprocessor), The Chip Letter (secondary), covering 1985 to 1988 work.
- [Look Before You Leap: Checking In on Type Tag Checking](https://arxiv.org/abs/2606.05466), Stephen M. Watt, arXiv, 2026.

Reduction, rewriting and algebra hardware
- [The Reduceron: Widening the von Neumann Bottleneck for Graph Reduction using an FPGA](https://mn416.github.io/reduceron-project/reduceron.pdf), Naylor and Runciman, about 2007.
- [The Reduceron Reconfigured](https://mn416.github.io/reduceron-project/report.pdf), Naylor and Runciman, ICFP 2010.
- [The Reduceron project page](https://www.cs.york.ac.uk/fp/reduceron), University of York, project 2008 to 2010.
- [The Rewrite Rule Machine Node Architecture and its Performance](https://www.csl.sri.com/papers/conpar94/conpar94.pdf), Lincoln, Meseguer, Ricciulli, SRI, about 1994.
- [Systolic VLSI arrays for polynomial GCD computation](https://maths-people.anu.edu.au/~brent/pub/pub073.html), Brent and Kung, IEEE Transactions on Computers, 1984.
- [Use of VLSI in algebraic computation: some suggestions](https://www.doi.org/10.1145/800206.806399), SYMSAC 1981 (authors not shown in the record I reached).
- [Harnessing the power of GPUs for problems in real algebraic geometry](https://publikationen.sulb.uni-saarland.de/handle/20.500.11880/26460?locale=en), Emeliyanenko, Saarland University thesis, 2012.

Complex-number instructions
- [FCMLA (vectors)](https://www.scs.stanford.edu/~zyedidia/arm64/fcmla_z_p_zzz.html), Arm A64 ISA reference (mirror).
- [CADD: Complex integer add with rotate](https://df.lth.se/~getz/ARM/A64/cadd_z_zz.html), Arm A64 ISA reference (mirror).
- [CMLA (vectors)](https://www.scs.stanford.edu/~zyedidia/arm64/cmla_z_zzz.html), Arm A64 ISA reference (mirror).
- [VFCMULCSH/VFMULCSH: Complex Multiply Scalar FP16 Values](https://www.felixcloutier.com/x86/vfcmulcsh:vfmulcsh), x86 reference derived from the Intel SDM.
- [Complex numbers support: discussions summary](https://gcc.gnu.org/pipermail/gcc/2023-October/242718.html), Sylvain Noiry, GCC mailing list, 16 October 2023.

Clifford algebra and phases
- [Implementation of a Clifford Algebra Co-processor Design on a Field Programmable Gate Array](https://www.informatik.uni-kiel.de/inf/Sommer/doc/Publications/chp/perwass_gebken_iccaa02.pdf), Perwass, Gebken, Sommer, submitted August 2002.
- [BasisBlade.java, geometric algebra reference implementation](https://geometricalgebra.org/ga_ref_impl/subspace/basis/BasisBlade.java.html), Daniel Fontijne, 2005 to 2007.
- [Efficient Implementation of Geometric Algebra (thesis record)](https://dare.uva.nl/id/627c5dcb-1b7d-4501-ba81-8c5a97db2749), Fontijne, University of Amsterdam, 2007.
- [Improved Simulation of Stabilizer Circuits](https://www.scottaaronson.com/papers/chp5.pdf), Aaronson and Gottesman, 2004.

Multi-term and redundant representations
- [Robust Adaptive Floating-Point Geometric Predicates](https://people.eecs.berkeley.edu/~jrs/papers/robust-predicates.abstract), Shewchuk, SoCG, May 1996; [paper PDF](https://www.wias-berlin.de/people/si/course/files/Shewchuk96-predicates.pdf).
- [Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates](https://www.doi.org/10.1007/PL00009321), Shewchuk, Discrete & Computational Geometry 18, 1997.
- [Quad-Double Arithmetic: Algorithms, Implementation, and Application](https://webarchive.library.unt.edu/eot2008/20080920093431/http://www.nersc.gov/~dhbailey/dhbpapers/quad-double.pdf), Hida, Li, Bailey, 30 October 2000.
- [Signed-digit number representations for fast parallel arithmetic (citation)](https://cr.yp.to/bib/1961/avizienis.bibtex), Avizienis, IRE Transactions on Electronic Computers, 1961.
- [Number systems slides (borrow-save attribution)](https://www.math.tugraz.at/JdN07/Slides/Bajard.pdf), J.-C. Bajard, April 2007.
- [A Class of Stored-Transfer Representations for Redundant Number Systems](https://web.ece.ucsb.edu/~parhami/pubs_folder/parh01-asilo-stored-xfer.pdf), Jaberipur, Parhami, Ghodsi, 2001.
- [Redundant binary representation](https://en.wikipedia.org/wiki/Redundant_binary_representation), Wikipedia (secondary), accessed November 2026.
- [Computation of complex number theoretic transforms using quadratic residue number systems (citation)](https://researchr.org/publication/KrishnanJM86-1/bibtex), Krishnan, Jullien, Miller, ICASSP 1986. Full text not read.
- [Quater-imaginary base](https://en.wikipedia.org/wiki/Quater-imaginary_base), Wikipedia (secondary) on Knuth 1960.

Sign-magnitude, Gaussian integers, CAS kernels
- [GMP Integer Internals](https://gmplib.org/manual/Integer-Internals), GMP manual, current.
- [CPython 3.12 pycore_long.h](https://datascience.imtech.res.in/anshu/RAPID-engine/ls/include/python3.12/internal/pycore_long.h), CPython 3.12 header (installed copy).
- [fmpz: integers](https://flintlib.org/doc/fmpz/index.html), FLINT documentation, current.
- [fmpzi: Gaussian integers](https://flintlib.org/doc/fmpzi/index.html), FLINT documentation, current.
- [ca_types.h](https://docs.rs/crate/flint-sys/latest/source/flint-3.5.0/src/ca_types.h) and [ca.h](https://docs.rs/crate/flint-sys/latest/source/flint-3.5.0/src/ca.h), FLINT 3.5.0 source (mirror).
- [Calcium: computing in exact real and complex fields](https://arxiv.org/abs/2011.01728), Johansson, arXiv, 3 November 2020.
- [Infinity in Calcium and Fungrim](https://fredrikj.net/blog/2021/01/infinity-in-calcium-and-fungrim/), Johansson, 25 January 2021.
- [The Design of Maple: A Compact, Portable, and Powerful Computer Algebra System](https://www.12000.org/my_notes/faq/maple_history/pdf_files/design_of_maple_paper.pdf), Char, Geddes, Gentleman, Gonnet, about 1983.
- [The design of Maple's sum-of-products and POLY data structures](https://peerj.com/preprints/504.pdf), Monagan and Pearce, PeerJ PrePrints, 17 September 2014.
- [Introduction to the GiNaC framework for symbolic computation within the C++ programming language](https://arxiv.org/abs/cs/0004015), Bauer, Frink, Kreckel, 2000 (PDF dated 2012).
- [GiNaC flags.h](https://ginac.de/reference/flags_8h_source.html), GiNaC 1.8.10 reference.
- [Some Notes on Internal Implementation](https://reference.wolfram.com/language/tutorial/SomeNotesOnInternalImplementation.html), Wolfram Language documentation, undated.
- [Giac data types](https://www.mintlify.com/geogebra/giac/concepts/data-types), third-party documentation of the Giac `gen.h`, undated.
- [How It Works](https://www.github.com/sympy/sympy/wiki/How-It-Works), SymPy wiki, undated and old.

Tagging
- [Summary: Representations of Dynamic Type Information](https://compilers.iecc.com/comparch/article/93-04-116), David Gudeman, comp.compilers, 30 April 1993; [TR 93-27](https://discmaster.textfiles.com/file/19864/ftp.cs.arizona.edu.tar/ftp.cs.arizona.edu/reports/1993/TR93-27.ps/TR93-27.pdf).
- [Pointer Compression in V8](https://v8.dev/blog/pointer-compression), Sheludko and Aboy Solanes, 30 March 2020.
- [JS::Value (Value.h)](https://searchfox.org/firefox-main/source/js/public/Value.h), SpiderMonkey source, current.
- [Memory Representation of Values](https://dev.realworldocaml.org/runtime-memory-layout.html), Real World OCaml, current edition.
- [multi_array_list.zig](https://zstd-live.pages.dev/0.12.0/multi_array_list.zig.html), Zig 0.12.0 standard library.
- [RISC-V Branch and Set](https://projectf.io/posts/riscv-branch-set/), Will Green, Project F, 19 March 2024 (updated 8 August 2025).
