# Literature survey: exact signed, rational and complex arithmetic at the machine level

Status: Milestone 1 (books) and Milestone 2 (libraries and languages) complete.
Scope (updated during Milestone 2): exact integers in the complex plane, i.e. Gaussian
integers ℤ[i] and Gaussian integers mod n, ℤ[i]/n. Rational-specific items are background.
Method: web research only (searches and fetched publisher pages, tables of contents, and
author pages), November 2026. Every claim carries a link. Claims marked **[unverified]**
come from memory or from an indirect source and were not confirmed on a fetched page.
Where a fetched page gave only chapter titles, coverage below is inferred from those titles
and is said to be.

The book this survey checks for: one that builds exact arithmetic up from machine words
(two's complement and modular word arithmetic, bit-level tricks) through rationals and
complex numbers to computational number theory, especially over the Gaussian integers or
number fields, ideally with formal proofs and timings.

---

## Milestone 1: books

### 1. Short answer

**No such book was found.** The literature splits into four groups, and no single book
covers more than two of them:

| Layer | Best existing book(s) | What stops them being "the" book |
|---|---|---|
| Machine words and bit tricks | Warren, *Hacker's Delight*; Arndt, *Matters Computational* | Stop at word-level integers; no rationals, no number fields. |
| Multiprecision integers, modular arithmetic | Knuth TAOCP vol. 2; Brent & Zimmermann; *Handbook of Applied Cryptography* ch. 14; St Denis, *BigNum Math* | Integers (and floats); rationals only as a short section; no Gaussian integers or number fields. |
| Finite-precision and exact non-integer number systems | Kornerup & Matula; Gregory & Krishnamurthy; RNS books | Number systems and hardware, not number theory. |
| Computational number theory, number fields | Cohen (GTM 138, 193); Crandall & Pomerance; Shoup; Bach & Shallit; Stein; Belabas & Cohen | Assume a bignum layer exists; describe algorithms, not machine representation, word sizes or array layout. |
| Formal proofs of computer arithmetic | Boldo & Melquiond; Russinoff | Floating point only. |

Gaussian integers specifically get at most one chapter in a computational book (Bressoud &
Wagon, chapter 9), or appear as a number-representation curiosity (complex bases in Knuth
and Warren), or as a formalisation exercise (Mathematics in Lean, Isabelle AFP, Mathlib).
No book was found that treats ℤ[i] or ℚ(i) as a *machine data type* with a word-level
representation, overflow analysis and timings.

### 2. Nearest neighbours, one by one

#### 2.1 Henri Cohen, *A Course in Computational Algebraic Number Theory* (GTM 138, Springer 1993)

- Chapters: fundamental number-theoretic algorithms; linear algebra and lattices;
  polynomials; algebraic number theory I; quadratic fields; algebraic number theory II;
  elliptic curves; factoring; primality; modern factoring
  ([Springer](https://link.springer.com/978-3-540-55640-4)).
- Chapter 1 opens with short subsections "1.1.2 Multi-precision" and "1.1.3 Base Fields and
  Rings", then powering, Euclid/Lehmer, CRT, Legendre symbol, Tonelli–Shanks and
  **Cornacchia's algorithm** (1.5.2) ([table of contents, Lehmanns](https://www.lehmanns.ch/media/970604/1);
  the page does not name the book but its structure matches).
- Chapter 5 is entirely about quadratic fields (discriminant, integral basis, ideals and
  quadratic forms, class groups of imaginary quadratic fields, units)
  ([same TOC](https://www.lehmanns.ch/media/970604/1)). ℚ(i) is an imaginary quadratic
  field, but it is not named in the TOC.
- **Covers:** the mathematics of number fields at algorithmic level; it is the reference
  that PARI/GP follows.
- **Omits:** machine representation. The multiprecision subsection is short and treats
  the bignum layer as given **[unverified: length and content of 1.1.2 not read]**. No
  word-level arithmetic, no data layout, no vectorisation, no proofs of implementations.

#### 2.2 Henri Cohen, *Advanced Topics in Computational Number Theory* (GTM 193, Springer 2000)

- Dedekind domains, relative extensions, class field theory (theory and computation),
  Kummer theory, analytic construction of defining polynomials, class and unit group
  variants, cubic fields, number-field tables
  ([Springer](https://link.springer.com/978-0-387-98727-9)).
- **Omits:** everything below the algorithmic level, even more so than GTM 138.

#### 2.3 Crandall & Pomerance, *Prime Numbers: A Computational Perspective* (2nd ed., Springer 2005)

- Nine chapters, ending with "Fast Algorithms for Large-Integer Arithmetic"; more than 100
  algorithms in pseudocode ([Springer](https://link.springer.com/book/9780387252827)).
- Chapter 9 explicitly concerns integers "split into machine-sized pieces" and
  reassembled, assumes multiprecision add/subtract, and starts at multiplication
  ([chapter abstract](https://www.springerprofessional.de/fast-algorithms-for-large-integer-arithmetic/13867310)).
- **Covers:** the bridge from number theory down to fast multiplication (FFT, Karatsuba,
  Montgomery, etc.) **[unverified: section list of ch. 9 not seen]**.
- **Omits:** rationals as a data type, number fields (beyond what factoring and primality
  need), machine-word tricks, formal proofs.

#### 2.4 Victor Shoup, *A Computational Introduction to Number Theory and Algebra* (2nd ed., CUP 2008; free PDF)

- 21 chapters; chapter 3 is "Computing with large integers" with sections "Machine models
  and complexity theory" and "Basic integer arithmetic"; later chapters cover Euclid,
  primes, groups, rings, probabilistic algorithms, primality, discrete logarithms,
  modules, matrices, polynomials, finite fields, deterministic primality
  ([PDF v2](https://shoup.net/ntb/ntb-v2.pdf); [book page](https://mail.shoup.net/ntb)).
- Electronic version under CC BY-NC-ND 3.0 ([PDF v2](https://shoup.net/ntb/ntb-v2.pdf)).
- **Omits:** Gaussian integers and algebraic number fields are not in the TOC
  ([PDF v2](https://shoup.net/ntb/ntb-v2.pdf)); arithmetic is analysed at the level of
  bit complexity, not machine words. Shoup is also the author of NTL (Milestone 2).

#### 2.5 Donald Knuth, *The Art of Computer Programming*, vol. 2, *Seminumerical Algorithms* (3rd ed.)

- Chapter 4 "Arithmetic": 4.1 Positional number systems, 4.2 Floating point, 4.3
  Multiple-precision, 4.4 Radix conversion, 4.5 Rational arithmetic, 4.6 Polynomial
  arithmetic, 4.7 Power series ([O'Reilly TOC](https://www.oreilly.com/library/view/art-of-computer/9780321635778/ch04a.xhtml)).
- 4.1 includes complex number bases: Knuth's quater-imaginary base 2i (Knuth 1960) and
  Penney's base i−1 ([Herd, arXiv:1701.04506](https://arxiv.org/pdf/1701.04506);
  an exercise of 4.1 asks for multiplication in base 2i "using the method illustrated in
  the text", [solution page](https://brain-taocp.pages.dev/vol2/4.1/14/)).
- **Covers:** the closest thing to a "machine words up to rationals" path in one volume:
  positional systems, multiprecision on a word radix, rational arithmetic, gcd and its
  analysis (4.5.2), factoring and primality (4.5.4) **[unverified: subsection numbers from
  memory]**.
- **Omits:** Gaussian integers as a ring with Euclidean division, number fields,
  vectorised or array computation, formal proofs. Written for MIX/MMIX, not modern
  SIMD hardware.

#### 2.6 Richard Brent & Paul Zimmermann, *Modern Computer Arithmetic* (CUP 2010; free PDF)

- Five chapters: integer arithmetic; modular arithmetic and the FFT; floating-point
  arithmetic; elementary and special functions; implementations and pointers
  ([Cambridge](https://www.cambridge.org/core/books/modern-computer-arithmetic/B345DF9E08B232BC5BAFE05C2F3A36D8);
  [free PDF](https://maths-people.anu.edu.au/~brent/pd/mca-cup-0.5.9.pdf)).
- The publisher's description says algorithms are given "at a high level ... avoiding
  machine-dependent detail" ([Cambridge](https://www.cambridge.org/core/books/modern-computer-arithmetic/B345DF9E08B232BC5BAFE05C2F3A36D8)).
- **Covers:** state-of-the-art bignum and modular algorithms (the GMP/MPFR world).
- **Omits:** rationals, complex/Gaussian integers, number fields, word-level tricks (by
  design), proofs.

#### 2.7 von zur Gathen & Gerhard, *Modern Computer Algebra* (3rd ed., CUP 2013)

- Parts named Euclid, Newton, Gauß, Fermat, Hilbert: Euclidean algorithm and modular
  methods, fast multiplication, Newton iteration, fast evaluation/interpolation, fast
  Euclid, fast linear algebra, factoring polynomials, Hensel lifting, lattice reduction,
  primality, factoring integers, cryptography, Gröbner bases, symbolic integration and
  summation ([front matter PDF](https://assets.cambridge.org/97811070/39032/frontmatter/9781107039032_frontmatter.pdf)).
- **Covers:** the algorithmic core of computer algebra, with "Implementations" sections.
- **Omits:** no mention of Gaussian integers, number fields, machine-word arithmetic or
  SIMD in the TOC ([front matter](https://assets.cambridge.org/97811070/39032/frontmatter/9781107039032_frontmatter.pdf)).
  Euclidean domains are treated abstractly, so ℤ[i] may appear as an example
  **[unverified]**.

#### 2.8 Henry S. Warren Jr., *Hacker's Delight* (2nd ed., Addison-Wesley 2012)

- Bit-level identities, overflow detection, shifts, rounding to powers of two, bounds
  checking, counting bits, searching and permuting bits, multiplication, integer division,
  division by known constants, integer roots, unusual number systems, float/int
  conversion, CRC and codes, Gray codes, Hilbert curves
  ([Wikipedia](https://en.wikipedia.org/wiki/Hacker%27s_Delight)).
- Chapter 12 "Unusual Bases for Number Systems" includes section 12-2 "Base −1 + i",
  in which every Gaussian integer has a unique signless binary representation
  ([O'Reilly, 12-2](https://www.oreilly.com/library/view/hackers-delight/0201914654/0201914654_ch12lev1sec2.html)).
- **Covers:** exactly the bottom layer of the target book (two's complement, overflow,
  magic-number division), and it touches complex integers.
- **Omits:** multiword arithmetic, modular inverses, rationals, any number theory beyond a
  curiosity ([Wikipedia summary](https://en.wikipedia.org/wiki/Hacker%27s_Delight)); no
  proofs. The author says he does not prove the base −1+i properties
  ([O'Reilly, 12-2](https://www.oreilly.com/library/view/hackers-delight/0201914654/0201914654_ch12lev1sec2.html)).

#### 2.9 Jörg Arndt, *Matters Computational: Ideas, Algorithms, Source Code* (Springer 2010; free PDF)

- Part I low-level algorithms (ch. 1 "Bit wizardry", about 100 pages); Part III fast
  transforms (FFT, NTT); Part IV fast arithmetic; Part V finite fields, starting with
  ch. 39 "Modular arithmetic and some number theory"
  ([TOC](https://jjj.de/fxt/fxtbook-toc-short.txt); [FXT page](https://jjj.de/fxt/)).
- **Covers:** the only single book found that spans bit tricks, transforms, fast
  arithmetic and modular number theory, with C++ source (FXT library, GPL).
- **Omits:** rationals as a type, Gaussian integers and number fields (not in the TOC),
  proofs. Its number theory is aimed at finite fields and transforms.

#### 2.10 Bach & Shallit, *Algorithmic Number Theory*, vol. 1 *Efficient Algorithms* (MIT Press 1996)

- Complexity theory survey, gcd (Euclid, extended, least-remainder, binary), computing in
  ℤ/(n), finite fields, solving equations over finite fields, primes (facts, heuristics,
  basic algorithms); "Extensions to algebraic numbers" is one section (8.7); bibliography
  of over 1800 items ([TOC](https://cs.uwaterloo.ca/~shallit/antdesc.html)).
- Volume 2 was projected but, as far as could be found, not published
  ([author page](https://cs.uwaterloo.ca/~shallit/antdesc.html)) **[unverified that it never appeared]**.
- **Omits:** Gaussian integers are not in the TOC; analysis is in a formal machine model,
  not real words.

#### 2.11 Chee Yap, *Fundamental Problems of Algorithmic Algebra* (OUP 2000; draft free online)

- Arithmetic (DFT, integer and polynomial multiplication), gcd, subresultants, modular
  techniques, fundamental theorem of algebra, roots, Sturm theory, **Gaussian lattice
  reduction** (ch. 8, planar lattices), lattice reduction, linear systems, elimination,
  Gröbner bases, continued fractions ([TOC](https://cs.nyu.edu/cs/faculty/yap/book/berlin/toc.html)).
- "Gaussian lattice reduction" is Gauss's 2-D reduction, not Gaussian-integer arithmetic.
- **Omits:** machine-level representation, number fields.

#### 2.12 Muller et al., *Handbook of Floating-Point Arithmetic* (2nd ed., Birkhäuser 2018) — for contrast

- Four parts: basics and history; analysing and optimising FP algorithms; IEEE 754
  implementations in hardware and software; extensions including interval arithmetic,
  double/triple-word arithmetic, **complex-number operations**, and **formal
  verification** ([author page](https://perso.ens-lyon.fr/jean-michel.muller/Handbook.html)).
- This is the model of what the target book would be for exact arithmetic: a single
  reference running from bit formats to algorithms to proofs. Nothing comparable exists
  on the exact side.

#### 2.13 Kornerup & Matula, *Finite Precision Number Systems and Arithmetic* (CUP 2010)

- Radix polynomial representations, base/digit-set conversion, addition, multiplication,
  division, square root, floating point, modular arithmetic and RNS, **rational
  arithmetic** ([TOC](https://biblioteca.izt.uam.mx/vufind/Record/EBSCO_ocn853359788/TOC)).
- The rational chapter (pp. 633–690) presents finite-precision rationals as pairs of
  bounded integers, with a bound on each or on their product, and rounding into that set
  ([Cambridge chapter page](https://www.cambridge.org/core/books/abs/finite-precision-number-systems-and-arithmetic/rational-arithmetic/EB6EC690FBF6997528CBAE23F9897D57)).
- **Relevance:** the nearest book to "rationals at machine width". It is about
  representation and rounding (approximate), not exact number theory; no complex numbers
  in the TOC.

#### 2.14 Gregory & Krishnamurthy, *Methods and Applications of Error-Free Computation* (Springer 1984)

- Ch. I: single- and multiple-modulus residue systems for integer or **rational**
  operands; ch. II: finite-segment p-adic systems and **Hensel codes**, giving exact
  arithmetic on a finite set of rationals; then applications
  ([MPDL record](https://ebooks.mpdl.mpg.de/ebooks/Record/EB000620389/Description)).
- **Relevance:** the classic book on exact rational arithmetic in fixed-size machine
  representations. Old, out of the mainstream, no complex numbers or number fields.

#### 2.15 Residue number system books

- Omondi & Premkumar, *Residue Number Systems: Theory and Implementation* (Imperial
  College Press 2007): conversion, addition, multiplication, comparison/overflow/sign/
  division, reverse conversion, applications
  ([TOC](https://libcat.mcphs.edu/Record/in00000209045/TOC)). No complex RNS in the TOC.
- The **quadratic residue number system** (QRNS) does complex integer multiplication
  componentwise modulo primes p ≡ 1 (mod 4), i.e. it is Gaussian-integer arithmetic mod
  n done as machine-word lanes: Krishnan, Jullien & Miller, "Complex digital signal
  processing using quadratic residue number systems"
  ([record](https://www.sciencegate.app/document/10.1109/icassp.1985.1168328); ICASSP 1985
  per DOI 10.1109/icassp.1985.1168328). This literature is hardware/DSP, in papers, not
  books.

#### 2.16 Cryptographic-implementation books (the "machine word up" tradition)

- Menezes, van Oorschot & Vanstone, *Handbook of Applied Cryptography*, ch. 14 "Efficient
  Implementation": multiple-precision integer arithmetic, multiple-precision modular
  arithmetic (Montgomery, Barrett), gcd, CRT, exponentiation, recoding
  ([TOC](https://cacr.uwaterloo.ca/hac/about/toc/toc14.html)).
- Tom St Denis, *BigNum Math* (Syngress 2006): builds a bignum library (LibTomMath) from
  scratch: add/sub/shift, Karatsuba, Toom-3, Barrett, Montgomery, exponentiation,
  division, gcd, Jacobi, inverses, primality, with C source
  ([O'Reilly](https://www.oreilly.com/library/view/bignum-math/9781597491129/)).
- **Relevance:** the best "bottom-up from words" treatments of integer and modular
  arithmetic. They stop at ℤ and ℤ/n.

#### 2.17 Number-theory books with a computational slant that reach Gaussian integers or number fields

- Bressoud & Wagon, *A Course in Computational Number Theory* (Key College / Wiley,
  Mathematica): ch. 9 "Prime Imaginaries and Imaginary Primes", with 9.2 "The Gaussian
  Integers" (complex number theory, Gaussian primes, the Gaussian moat problem, "the
  Gaussian zoo") ([Wiley TOC](https://gov.wiley.com/WileyCDA/WileyTitle/productCd-0470412151,descCd-tableOfContents.html)).
  The only computational textbook found with a Gaussian-integer chapter; built on
  Mathematica, nothing below the CAS.
- William Stein, *Algebraic Number Theory, a Computational Approach* (free, CC BY-SA 3.0)
  ([book page](https://williamstein.org/books/ant)); uses Sage **[unverified: not stated
  on the fetched page]**.
- Abhijit Das, *Computational Number Theory* (CRC 2013): arithmetic of integers, finite
  fields, polynomials, elliptic curves, primality, factoring, discrete logs, sparse linear
  systems, cryptography; examples in GP/PARI
  ([Lavoisier](https://www.lavoisier.fr/livre/autre/computational-number-theory/das/descriptif_2622982)).
  No number fields or Gaussian integers in the TOC.
- Belabas & Cohen, *Numerical Algorithms for Number Theory: Using Pari/GP* (AMS
  Mathematical Surveys and Monographs 254, 2021)
  ([TAMU catalogue](https://libcat.library.tamu.edu/Record/in00004414728/TOC)). Numerical
  (multiprecision floating) methods for number theory **[unverified: contents not
  fetched; AMS page failed]**.
- Belabas, "Topics in computational algebraic number theory", *J. Théor. Nombres
  Bordeaux* 16 (2004) — a survey paper, not a book, on basic arithmetic in number fields
  as implemented in PARI ([JTNB](https://jtnb.centre-mersenne.org/articles/10.5802/jtnb.433)).
  The closest text found on *how* number-field elements are represented in a real system.
- Stepanov & Rose, *From Mathematics to Generic Programming* (Addison-Wesley 2015):
  Egyptian multiplication to power algorithms, Euclid, Euclidean domains (8.7), Stein's
  binary gcd, extended gcd, RSA, in C++ ([fm2gp.com](https://fm2gp.com/)). Same spirit
  (mathematics → efficient generic code); does not go to number fields; Gaussian integers
  not confirmed on the TOC **[unverified]**.

#### 2.18 Books pairing formal proofs with computer arithmetic

- Boldo & Melquiond, *Computer Arithmetic and Formal Proofs: Verifying Floating-point
  Algorithms with the Coq System* (ISTE/Elsevier 2017) — floating point only
  ([ISTE](https://www.iste.co.uk/book.php?id=1238)).
- Russinoff, *Formal Verification of Floating-Point Hardware Design: A Mathematical
  Approach* (Springer 2019; ACL2) ([Lehmanns](https://lehmanns.de/shop/technik/43325078-9783319955124-formal-verification-of-floating-point-hardware-design)) — floating point only.
- *Mathematics in Lean* builds the Gaussian integers as a structure, proves the
  commutative-ring laws and constructs a Euclidean-domain instance via rounded division
  ([MIL §6.3 docs](https://florisvandoorn.com/LeanCourse24/docs/LeanCourse/MIL/C06_Structures/S03_Building_the_Gaussian_Integers.html);
  [MIL](https://leanprover-community.github.io/mathematics_in_lean)). This is a
  formalisation exercise on unbounded `Int`, not machine arithmetic.
- Not books, but the formal-proof state of the art below the book level:
  - Mathlib's `GaussianInt` (= `ℤ√(-1)`), with Euclidean domain instance and the
    "p not irreducible ⇒ sum of two squares" result
    ([Mathlib docs](https://leanprover-community.github.io/mathlib4_docs/Mathlib/NumberTheory/Zsqrtd/GaussianInt.html)).
  - Isabelle AFP *Gaussian Integers* (Eberl, 2020): Euclidean ring, classification of
    primes, sums of two squares, "an executable (albeit not very efficient)
    factorisation algorithm" ([AFP](https://isa-afp.org/entries/Gaussian_Integers.html)).
  - Rieu-Helft, "A Why3 proof of GMP algorithms" (J. Formalized Reasoning 12, 2019):
    verified 64-bit-limb mpn add/sub/mul (incl. Toom-2, 2.5)/div/sqrt, extracted to C,
    GMP-compatible and nearly as fast below about 1000 bits
    ([JFR PDF](https://jfr.unibo.it/article/download/9730/10489/35186)).
  - *hex*: Lean 4 libraries for verified computational algebra (polynomials, finite
    fields, LLL, Berlekamp–Zassenhaus) with `Array`-backed `UInt64` coefficients and
    Barrett/Montgomery reduction, no Mathlib dependency in the computational part
    ([SPEC](https://raw.githubusercontent.com/kim-em/hex-dev/main/SPEC/SPEC.md)).
    Repository owner is GitHub user `kim-em` **[unverified: identity of authors]**. No
    Gaussian integers or number fields mentioned. **This is the closest active project to
    the Lean side of the erdos work and should be read in Milestone 2.**
  - Lean 4 port of AWS s2n-bignum's verification framework for aarch64 machine code
    (goal stated; no completed results listed)
    ([GitHub](https://github.com/arademaker/bignum)).
  - Ji, "Machine-Checked Arithmetic Bit Complexity of the Kannan-Bachem Smith Normal Form
    in Lean 4" (arXiv 2607.22524, July 2026): proves correctness and polynomial
    arithmetic-cost bounds, but excludes compiled runtime behaviour
    ([arXiv](https://arxiv.org/abs/2607.22524)).

  No book was found that pairs formal proofs with *exact* (integer, rational, complex or
  algebraic-number) machine arithmetic. The formal-proof books are floating-point books;
  the exact-arithmetic proofs are papers and libraries.

### 3. Conclusion (books)

1. **The described book does not appear to exist.** No book found goes from machine words
   (two's complement, overflow, bit tricks) through rationals and complex numbers to
   computational number theory over ℤ[i] or number fields, and none does it with machine
   proofs and timings.
2. **Closest competitors, by the half they cover:**
   - Bottom half (words → multiprecision → rationals): **Knuth TAOCP vol. 2, ch. 4** (the
     only one that reaches rationals and touches complex bases), then **Hacker's Delight**
     (word level, includes base −1+i) and **Arndt, Matters Computational** (bit wizardry
     through modular number theory, free).
   - Top half (number fields): **Cohen GTM 138** (with Cornacchia and quadratic fields),
     and for the textbook level **Bressoud & Wagon ch. 9** (the only computational text
     with a Gaussian-integer chapter).
   - Method (proofs about arithmetic): **Muller et al.** and **Boldo & Melquiond**, which
     are the floating-point analogues of what the target book would be on the exact side.
   - Exact non-integer types at fixed width: **Kornerup & Matula** (finite-precision
     rationals) and **Gregory & Krishnamurthy** (residue and Hensel-code rationals).
3. **Gap statement a book could use:** "Floating point has its handbook (Muller et al.) and
   its formal-proof book (Boldo & Melquiond); multiprecision integers have Knuth and
   Brent–Zimmermann; number fields have Cohen. Nothing joins the machine-word layer to
   ℤ[i], ℚ(i) and number fields, and nothing proves the exact layer correct the way the
   floating-point books do."

**Scope note (integer-only: ℤ[i] and ℤ[i]/n, no rationals).** The conclusion still
holds, and the gap is cleaner. Still directly relevant: Hacker's Delight (word-level
two's complement, overflow, division by constants, base −1+i for Gaussian integers);
Knuth vol. 2 §4.1 and §4.3 (complex bases, multiprecision); Arndt (bit wizardry,
modular arithmetic, NTTs); the cryptographic-implementation books (Montgomery and Barrett
reduction, the core of ℤ[i]/n arithmetic); Crandall & Pomerance ch. 9; Brent &
Zimmermann ch. 1–2; Cohen GTM 138 ch. 1 (Cornacchia) and ch. 5 (imaginary quadratic
fields); Bressoud & Wagon ch. 9 (Gaussian primes, moat); the residue-number-system
literature, especially the quadratic RNS (§2.15), which is ℤ[i]/n arithmetic in machine
lanes; and the formal Gaussian-integer developments (Mathlib, Isabelle AFP, Mathematics in
Lean). Now background only: Knuth §4.5, Kornerup & Matula ch. 9 and Gregory &
Krishnamurthy ch. II (finite-precision and Hensel-code rationals), and the parts of Cohen
GTM 193 about general relative extensions. Under the integer-only scope the target book is
"Hacker's Delight and Cohen ch. 1 and 5 for ℤ[i], with proofs and timings", and no book
of that kind was found.

---

## Milestone 2: libraries and languages

Weighting follows the updated scope: arithmetic in ℤ[i] and ℤ[i]/n first, including SIMD
and GPU; number fields second; rationals as background.

### 4. Short answer

**Nothing found combines (1) an exact Gaussian-integer or ℤ[i]/n element type,
(2) whole-array (columnar, SIMD or GPU) execution, and (3) a terse array-language
surface, let alone (4) machine-checked proofs.** Each pair exists separately:

| Have | Example | Missing |
|---|---|---|
| Exact ℤ[i] + fast C | FLINT `fmpzi` | Per-element, bignum-backed; no vector or batch functions; not a language |
| Exact ℤ[i] + array language | April APL (complex rationals from Common Lisp), J essay with integer pairs | Boxed per-element numbers; no SIMD; not fast |
| Gaussian GCD/floor in an array language | Dyalog APL, J, NARS2000 (McDonnell complex floor) | Built on complex **doubles**, so exact only while parts fit in 53 bits |
| Modular arithmetic at SIMD/GPU speed | Mathemagix SIMD, FLINT `nmod_vec` (AVX2), NTL, CGBN, cuPQC, JAX on TPU, HE NTT libraries | Only ℤ/p, no ℤ[i]/n type; libraries, not languages |
| Complex integer instructions in hardware | Arm SVE2 `CMLA` (8/16/32/64-bit complex integer lanes), AMD AI Engine `cint32`, Intel patent for packed 16-bit complex words | No library or language exposing them as Gaussian integers mod n |
| ℤ[i]/p as transform ring | Fast Galois transform over GF(q²), q = 2⁶¹−1; QRNS | Used inside convolution and DSP, never exposed as a data type |
| Verified exact algebra with machine words | Lean 4 *hex* (`UInt64`, Barrett/Montgomery, number fields) | No Gaussian-integer library listed; not array-style |
| Array languages with good performance | q/kdb+, k9, BQN, Uiua, Futhark, JAX, Halide | No complex integer types at all (complex is float, or missing) |

### 5. Libraries

#### 5.1 FLINT: `fmpzi` (Gaussian integers)

- `fmpzi_t` is a pair of `fmpz` (FLINT multiprecision integers, small values inline).
  Provides Euclidean division with nearest rounding (N(r) ≤ N(y)/2), an approximate
  floating-point-quotient divrem, exact division, removal of factors of 1+i, canonical
  associates, proven and probable primality. **Factorisation is not implemented**
  ([FLINT docs](https://flintlib.org/doc/fmpzi/index.html)).
- Four GCD algorithms: Euclidean, improved Euclidean (approximate quotients finished in
  double precision), Weilert's (1+i)-ary binary GCD, and a lattice/shortest-vector
  method; `fmpzi_gcd` chooses one, and the binary GCD hands over to Euclid above 500 bits
  ([FLINT docs](https://flintlib.org/doc/fmpzi/index.html)).
- Polynomial multiplication over ℤ[i] (FFT-small, Kronecker substitution,
  Schönhage–Strassen); matrices and polynomials through the generic-ring context
  `gr_ctx_init_fmpzi` ([FLINT docs](https://flintlib.org/doc/fmpzi/index.html)).
- **No vector/batch functions for `fmpzi`, and no ℤ[i]/n type**
  ([FLINT docs](https://flintlib.org/doc/fmpzi/index.html)). The ℤ[i]/n type is not
  listed; ℤ[i]/p for p ≡ 3 mod 4 can be had as GF(p²) via FLINT's finite-field modules
  **[unverified: not checked whether FLINT's `fq` modules special-case this]**.
- Julia's Nemo exposes it: "The Gaussian integer type in Nemo is provided by a pair of
  FLINT integers", via `GaussianIntegers()` and `GaussianRationals()`
  ([Nemo docs](https://nemocas.github.io/Nemo.jl/stable/integer)).
- **Relevance:** the strongest existing exact ℤ[i] implementation, and the natural
  per-element baseline for timings. Its design (pointer-or-small-value `fmpz`) is the
  opposite of a fixed-width packed column.

#### 5.2 FLINT: `nf_elem` (number fields, formerly Antic)

- Antic ("Algebraic Number Theory In C", Hart) is archived; "Antic has been merged into
  FLINT" ([GitHub](https://github.com/flintlib/antic)).
- `nf_elem` is a union with special layouts: linear fields (num, den) and **quadratic
  fields as `fmpz num[3]` plus `den`**, the third slot being an "extra coeff for delayed
  reduction"; general fields are `fmpq_poly_t` ([nf_elem.h](https://docs.rs/crate/flint3-sys/3.5.1/source/flint/src/nf_elem.h)).
  So ℚ(i) is a quadratic special case, but always with a denominator and multiprecision
  coefficients.
- No vector operations; conversion to and from matrix rows and the multiplication matrix
  in the power basis ([FLINT docs](https://flintlib.org/doc/nf_elem.html)).

#### 5.3 FLINT: word-size modular vectors

- `nmod_vec` dot products are vectorised for AVX2 only (4×64-bit lanes via a `vec4n`
  abstraction, delayed reduction); no NEON path in that file
  ([source](https://docs.rs/crate/flint-sys/latest/source/flint-3.5.0/src/nmod_vec/dot.c)).
- **Relevance:** the existing SIMD modular kernels are for ℤ/p; a ℤ[i]/p kernel would
  be two of these plus a cross term, and nobody ships it as such.

#### 5.4 Calcium (now part of FLINT)

- Exact real and complex numbers as elements of fields ℚ(a₁,…,aₙ), with `qqbar_t` for
  canonical algebraic numbers; built on Arb, Antic and FLINT. Per-element `ca_t` with
  `ca_poly_t` and `ca_mat_t` for polynomials and matrices
  ([Johansson, arXiv:2011.01728](https://arxiv.org/pdf/2011.01728)).
- **Relevance:** exact complex arithmetic at the opposite end from machine words
  (symbolic field towers). Shows "exact complex" in practice means heavy objects.

#### 5.5 PARI/GP

- Number-field elements may be `t_INT`, `t_FRAC`, `t_POL`, `t_POLMOD`, a `t_COL` on the
  integral basis, or a factored form (famat) "to avoid coefficient explosion"
  ([PARI docs](https://pari.math.u-bordeaux.fr/dochtml/html-stable/General_number_fields.html)).
- Gaussian integers are ordinary `t_COMPLEX` with `t_INT` parts **[unverified: not
  checked on a fetched page]**. Vectors are arrays of pointers to generic objects
  **[unverified]**, so there is no packed columnar arithmetic.

#### 5.6 NTL (Shoup)

- Multimodular `ZZ_p` matrix multiplication (10×–40× faster than naive), small-prime FFTs
  for `ZZ_pX`, AVX in the `zz_p` matrix kernels, thread boosting
  ([Shoup 2016, PDF](https://www.libntl.org/boost.pdf)).
- No Gaussian-integer type was found **[unverified: absence not checked against the
  full NTL documentation]**.

#### 5.7 Mathemagix modular SIMD (van der Hoeven, Lecerf, Quintin)

- Branch-free Barrett, Montgomery and floating-point/FMA modular arithmetic for 8- to
  128-bit integer types, on SSE 4.x, AVX, AVX2 and FMA; speedups close to the theoretical
  lane counts, sometimes an order of magnitude over existing software
  ([paper, HTML](https://www.texmacs.org/joris/simd/simd.html);
  [HAL](https://hal.archives-ouvertes.fr/hal-01022383)). Published in ACM TOMS
  ([DOI 10.1145/2876503](https://unpaywall.org/10.1145%2F2876503)).
- Complex or Gaussian integers are not discussed
  ([paper](https://www.texmacs.org/joris/simd/simd.html)).
- **Relevance:** the best reference for the word-level modular kernels a ℤ[i]/n column
  type would need; ℤ[i]/n is two such lanes per element.

#### 5.8 Gaussian integers mod p as a transform ring (GF(q²), QRNS)

These are the closest prior art to "Gaussian integers mod n at machine speed", but they
use ℤ[i]/p internally and never expose it as a data type.

- **Complex integer transforms over GF(q²), q a Mersenne prime:** Liu, Reed & Truong,
  "Fast algorithms for complex integer transforms", IEEE TASSP 1977
  ([record](https://www.bohrium.com/paper-details/fast-algorithms-for-complex-integer-transforms/812354561061683202-13724)).
  Crandall's "fast Galois transform" over GF(q²) with q = 2⁶¹−1: for q ≡ 3 mod 4,
  Gaussian integers mod q form GF(q²), q²−1 is divisible by 2⁶², so large power-of-two
  transforms exist ([Mersenne list, Daly 1998](https://mail-archive.com/mersenne@base.com/msg01012.html)).
- **Quadratic residue number system (QRNS):** for p ≡ 1 (mod 4), ℤ[i]/p ≅ 𝔽_p × 𝔽_p via
  a + bi ↦ (a + jb, a − jb), where j² ≡ −1, so a complex product costs two independent
  modular products with no cross term **[standard CRT fact; not quoted from a fetched
  source]**. QRNS was developed for complex DSP hardware: Jenkins & Lee 1980/81 (complex
  residue arithmetic without complex NTTs,
  [PSU](https://pure.psu.edu/en/publications/complex-residue-number-arithmetic-for-digital-signal-processing/));
  Krishnan, Jullien & Miller, ICASSP 1985
  ([record](https://www.sciencegate.app/document/10.1109/icassp.1985.1168328)); Jenkins &
  Krogmeier, ISCAS 1986, complex recursive filters, noting QRNS "limit[s] interaction
  between the real and imaginary channels"
  ([PSU](https://pure.psu.edu/en/publications/complex-digital-filtering-in-quadratic-modular-number-systems/)).
- Homomorphic-encryption work uses ℤ_t[x]/(1+x²) ≅ ℤ_{t²} for t ≡ 3 mod 4 to pack
  complex values ([Pedrouzo-Ulloa et al., arXiv:1607.05229](https://arxiv.org/pdf/1607.05229)).
- Gaussian-integer public-key variants (e.g. Elkamchouchi, Elshenawy & Shaban, NRSC 2003,
  [record](https://bvbvf1.bib-bvb.de/vufind/PrimoRecord/cdi_ieee_primary_1217350)) use
  ℤ[i]/n arithmetic, but no fast implementation library was found.
- **Relevance:** QRNS gives the erdos project a known, citable split: for p ≡ 1 mod 4 a
  ℤ[i]/p column is two independent ℤ/p columns (array-friendly, no cross terms); for
  p ≡ 3 mod 4 it is GF(p²) with the 3- or 4-multiplication complex product. No software
  library implementing either as a vector type was found.

#### 5.9 SIMD hardware with complex integer lanes

- **Arm SVE2 `CMLA`:** complex integer multiply-add with rotation on 8-, 16-, 32- or
  64-bit elements, real part in the even lane and imaginary in the odd lane; two `CMLA`s
  with rotations 90° apart give a full complex multiply-add; requires FEAT_SVE2 or
  FEAT_SME ([instruction reference](https://www.scs.stanford.edu/~zyedidia/arm64/cmla_z_zzz.html);
  [.NET intrinsic](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.intrinsics.arm.sve2.multiplyaddrotatecomplex?view=net-11.0)).
  With 32-bit elements this is exactly "a Gaussian integer as two int32 lanes in one
  64-bit slot", in hardware.
- **AMD AI Engine:** vector types such as `v4cint32` with complex multiply,
  multiply-accumulate and multiply-subtract into 80-bit complex accumulators
  (`v4cacc80`) ([AMD intrinsics guide](https://download.amd.com/docnav/aiengine/xilinx2024_2/aiengine_intrinsics/intrinsics/group__vect__mult__cmp__32x32c__nn.html)).
- **x86:** AVX512-FP16 has complex multiply for FP16 pairs only
  ([VFCMADDCPH](https://www.felixcloutier.com/x86/vfcmaddcph:vfmaddcph)). An Intel
  patent application describes dual complex multiply-add on packed signed 16-bit complex
  words (VPCDPWQRE/VPCDPWQIMM) ([USPTO](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/11163563));
  shipping status not verified **[unverified]**. No x86 complex-integer instruction was
  confirmed, so ℤ[i] on x86 is done with ordinary integer lanes (e.g. `vpmuldq`/
  `vpmullq` and shuffles) **[unverified: no source found describing a ℤ[i] kernel]**.
- No software library was found that uses these instructions for exact Gaussian-integer
  or ℤ[i]/n arithmetic; their documented use is DSP/ML.

#### 5.10 GPU number-theory and modular arithmetic

- **CGBN** (NVlabs): fixed-size unsigned multiprecision (32 bits to 32K bits) with
  cooperative thread groups: add, sub, mul, Montgomery, powm, division, sqrt, gcd,
  modular inverse; 4×–77× over 20-core GMP on V100; last release noted 2018
  ([GitHub](https://github.com/NVlabs/CGBN)).
- **cuPQC-BigInt** (NVIDIA): device-side fixed-width unsigned big integers (32-bit
  limbs, up to 4096 bits), `mul_mod`, Montgomery and Barrett; **unsigned only**; batching
  is by instance index inside user kernels
  ([NVIDIA docs](https://docs.nvidia.com/cuda/cupqc/libraries/cupqc_bigint/cupqc_bigint_feature.html)).
- **Midsize integers on GPU, CUDA and Futhark:** Oancea & Watt (arXiv 2405.14642) beat
  CGBN for 2¹⁴–2¹⁸ bits; FFT multiplication over word-size prime fields; their Futhark
  version is competitive with CGBN on most tests but scales poorly, which the authors
  put down to a missing sequentialisation pass
  ([ar5iv](https://ar5iv.labs.arxiv.org/html/2405.14642)). **This is the closest data
  point for "array language vs hand-written GPU code" on exact integer arithmetic.**
- **HE NTT kernels:** GPU NTT over ℤ_p with Barrett variants and fused kernels
  (Kim et al., [arXiv:2012.01968](https://arxiv.org/pdf/2012.01968.pdf); Shivdikar et
  al., [arXiv:2209.01290](https://ar5iv.arxiv.org/html/2209.01290)). Both are ℤ_p only;
  neither uses Gaussian integers.
- **TPU via JAX:** CROSS (Tong et al., [arXiv:2501.07047](https://arxiv.org/html/2501.07047v1))
  splits 28-bit residues into 8-bit chunks so modular products and NTTs run on matrix
  units; Barrett reduction built from mul/shift/sub. Jaxite (JAX FHE backend for TPU/GPU,
  [PyPI](https://pypi.org/project/jaxite)) is boolean-gate based.
- No GPU library with a Gaussian-integer or ℤ[i]/n type was found. Gaussian-prime
  computations such as the Gaussian moat (width 6 separates the origin from infinity,
  Tsuchimura 2005) are cited without hardware details
  ([Wikipedia](https://en.wikipedia.org/wiki/Gaussian_moat)) **[unverified: whether any
  moat computation used SIMD/GPU]**.

#### 5.11 Lean 4: *hex*

- *hex* is published as `leanprover/hex`, "Verified computational algebra in Lean 4",
  Apache 2.0, about 35 component libraries, each computational library Mathlib-free with
  an optional `*Theory` library holding the Mathlib-facing proofs
  ([Reservoir](https://reservoir.lean-lang.org/@leanprover/hex);
  [GitHub](https://github.com/leanprover/hex)). Development is in `kim-em/hex-dev`, and
  the manual and blog are on Kim Morrison's site
  ([Reservoir payload](https://reservoir.lean-lang.org/@leanprover/hex/_payload.json)).
- Components relevant here: **HexArith "Exact word arithmetic"** (no theory library),
  HexModArith (modular arithmetic, with theory), HexModular (CRT and rational
  reconstruction), HexPrimality and HexECPP (certified primality), **HexNumberField
  "Algebraic numbers"** and HexNumberFieldTower, HexRoots (complex root isolation),
  HexGFq/HexGF2 (finite fields), HexLLL, HexBerlekampZassenhaus
  ([Reservoir payload](https://reservoir.lean-lang.org/@leanprover/hex/_payload.json)).
- The spec: dense `Array`-backed polynomials with `UInt64` coefficients, Barrett/
  Montgomery reduction, GMP FFI for big integers, correspondences such as
  `ZMod64 p ≃+* ZMod p` and `GFq p n ≃+* GaloisField p n`
  ([SPEC.md](https://raw.githubusercontent.com/kim-em/hex-dev/main/SPEC/SPEC.md)).
  The development rules forbid axioms, `native_decide` and `sorry`, and benchmarks are
  compared against recorded external-system observations
  ([DEV.md](https://raw.githubusercontent.com/kim-em/hex-dev/main/DEV.md)).
- A HexNumberField announcement post exists ("Exact algebraic numbers and number fields
  in Lean", Sept 2026), but the blog and manual were not reachable from this session
  **[unverified: contents of HexArith and HexNumberField; whether ℤ[i] or ℤ[i]/n is
  specialised]**.
- No Gaussian-integer library appears in the component list
  ([Reservoir payload](https://reservoir.lean-lang.org/@leanprover/hex/_payload.json)).
- **Relevance: the closest active project to the erdos Lean work.** Same idea (verified
  exact arithmetic on machine words in Lean 4, timed), different target (polynomials,
  finite fields, factorisation) and different style (library, scalar loops, not array
  language). erdos should cite it, compare HexArith/HexModArith against its own word
  layer, and position ℤ[i] and ℤ[i]/n columns as what hex does not cover.
- Other formal neighbours (from Milestone 1): Mathlib `GaussianInt`; Isabelle AFP
  Gaussian Integers with a slow executable factoriser; Why3-verified GMP `mpn`; Lean
  port of s2n-bignum's framework.

#### 5.12 Smaller items

- GaussianIntegers.jl (Ulrich Thiel): teaching package on AbstractAlgebra.jl, with HNF
  and SNF over ℤ[i]; last updated about 2022 ([JuliaPackages](https://juliapackages.com/p/gaussianintegers)).
- Python `galois`: NumPy `ndarray` subclass for GF(pᵐ) with numba-JIT ufuncs, lookup
  tables up to order 2²⁰ ([PyPI](https://pypi.org/project/galois/0.0.16)). It is the
  closest "array of finite-field elements" library; GF(p²) for p ≡ 3 mod 4 is ℤ[i]/p, but
  as a polynomial-basis field, not a Gaussian-integer type.
- Mathematica: `GaussianIntegers -> True` option for `FactorInteger`, `PrimeQ`,
  `Factor` ([Wolfram](https://reference.wolfram.com/language/ref/GaussianIntegers.html)).
  Mathematics without array speed, as in the original gap statement.

### 6. Array languages and array compilers

| Language | Integer types | Complex type | Exact ℤ[i]? | Source |
|---|---|---|---|---|
| q/kdb+ | short, int, long (16/32/64-bit) | none | no | [KX](https://code.kx.com/q/learn/tour/datatypes) |
| k9 (Shakti) | 1/2/4-byte unsigned, 8-byte signed | not mentioned | no | [k9 manual](https://estradajke.github.io/k9-simples/k9/Noun.html) |
| J | machine int, int2, int4, extended, rational | IEEE doubles only | GCD via complex floor on doubles; exact only as user-built integer pairs | [J wiki](https://code.jsoftware.com/wiki/Vocabulary/NumericPrecisions); [Hui essay](https://code.jsoftware.com/wiki/Essays/Gaussian_Integers) |
| Dyalog APL | machine ints | binary-float components | GCD/floor on complex floats | [APL Wiki: Number](https://aplwiki.com/wiki/Number); [GCD](https://aplwiki.com/wiki/GCD) |
| NARS2000 | ints, rationals, multiprecision | complex, quaternion, octonion | component types not confirmed | [APL Wiki](https://aplwiki.com/wiki/Number); [sudleyplace](https://www.sudleyplace.com/APL/projects.html) |
| April (APL → Common Lisp) | Lisp integers, rationals | complex with rational parts, e.g. `3r4J9r5` | **yes, boxed** | [GitHub](https://github.com/phantomics/april) |
| BQN | doubles (internal ints hidden) | not mentioned | no | [BQN docs](https://mlochbaum.github.io/BQN/doc/types.html) |
| Uiua | — | `Complex { re: f64, im: f64 }` | no | [docs.rs](https://docs.rs/uiua/0.18.0-dev.4/uiua/struct.Complex.html) |
| Julia | all widths, BigInt, Rational | `Complex{T}` incl. integer `T` | `Complex{Int}` exists, but `/` returns floats; ℤ[i] proper via Nemo | [Julia manual](https://docs.julialang.org/en/v1.0/manual/complex-and-rational-numbers/); [Nemo](https://nemocas.github.io/Nemo.jl/stable/integer) |
| Futhark | i8–i64, u8–u64 | none built in | no | [reference](https://hackage-content.haskell.org/package/futhark-0.25.32/src/docs/language-reference.rst) |
| JAX | int8–int64, uint8–uint64 | complex64/128 only | no | [type promotion](https://kolonist26-jax-kr.readthedocs.io/en/latest/_sources/type_promotion.rst) |
| Halide | int, uint (any width), float, bfloat | none | no | [Halide](https://halide-lang.org/docs/api/generated/halide_type_code_t.html) |

Notes:

- **McDonnell's complex floor (APL75)** is the array-language tradition's Gaussian-integer
  arithmetic: it makes residue, GCD and LCM well defined on complex arguments, so
  `2J2∨3J1` is `1J1`; implemented in Dyalog, J and NARS2000
  ([Complex floor](https://aplwiki.com/wiki/Complex_Floor); [GCD](https://aplwiki.com/wiki/GCD)).
  Since Dyalog and J store complex parts as binary doubles
  ([APL Wiki: Number](https://aplwiki.com/wiki/Number);
  [J wiki](https://code.jsoftware.com/wiki/Vocabulary/NumericPrecisions)), this is exact
  only while the parts are integers below 2⁵³ (an inference, not a documented
  guarantee **[unverified]**). **This is the nearest thing to "q-style Gaussian
  integers" that exists**, and it has the semantics but not exactness guarantees or
  integer-lane speed.
- Roger Hui's J essay on Gaussian integers avoids the complex type entirely and uses
  `real imaginary` integer pairs (extended precision), with plus, times, rounded divide,
  residue and Euclid's GCD as array verbs
  ([J wiki](https://code.jsoftware.com/wiki/Essays/Gaussian_Integers)). This is the
  closest existing *design sketch* of the target: Gaussian integers as a trailing axis of
  length 2.
- **April** inherits Common Lisp's exact complex rationals
  ([GitHub](https://github.com/phantomics/april); CL complex rationals:
  [X3J13 issue](https://www.lispworks.com/documentation/HyperSpec/Issues/iss071_w.htm)),
  so it is the only array language found with an exact Gaussian-integer number type,
  but its elements are boxed Lisp numbers and it makes no performance claims.
- q/kdb+ and k have no complex type at all; BQN, Uiua, JAX and Futhark have only
  floating complex or none; Halide and Futhark have exactly the fixed-width integer lanes
  a packed ℤ[i] column would be built from, but no complex integer type.

### 7. Projects combining array style with exact complex or number-field arithmetic

Searched for: array language + Gaussian integers; q/k + exact complex; APL + Gaussian
integers; SIMD/GPU Gaussian-integer libraries. Found only:

1. Dyalog/J/NARS2000 complex floor and GCD (floats; see §6).
2. Hui's J essay (integer pairs, not a type; see §6).
3. April's exact complex rationals (boxed; see §6).
4. QRNS and GF(q²) transforms (hardware/DSP, used internally; see §5.8).
5. Python `galois` (NumPy arrays over finite fields, not ℤ[i]; see §5.12).

None is a fixed-width packed ℤ[i] or ℤ[i]/n column type with whole-array kernels.

### 8. Conclusion (libraries and languages)

1. **The described library/language does not appear to exist.** No library or language
   was found that offers Gaussian integers or ℤ[i]/n as a fixed-width, packed, columnar
   type with SIMD or GPU kernels, and none pairs that with a q-style surface or with
   proofs.
2. **Closest, by axis:**
   - *Exact ℤ[i] mathematics, fast per element:* FLINT `fmpzi` (with Nemo). The baseline
     to time against.
   - *Array-language semantics for ℤ[i]:* McDonnell's complex floor and GCD in Dyalog/J
     (floating), Hui's J integer-pair essay, April's exact complex rationals (boxed).
   - *Machine-word modular kernels:* Mathemagix modular SIMD; FLINT `nmod_vec`; NTL;
     on GPU, CGBN, cuPQC-BigInt, HE NTT kernels, CROSS on TPU. All ℤ/p only.
   - *ℤ[i]/p in machine lanes:* QRNS (p ≡ 1 mod 4, two independent residue channels)
     and Crandall's GF(q²) transform (q = 2⁶¹−1). Prior art for the representation, but
     never exposed as a data type.
   - *Hardware:* Arm SVE2 `CMLA` (complex integer lanes up to 64 bits) and AMD AI Engine
     `cint32` are native "Gaussian integer as two int32 lanes" instructions with no
     exact-arithmetic software on top.
   - *Verified, machine words, Lean 4:* *hex* (HexArith, HexModArith, HexNumberField). The
     nearest project in method; it does not cover ℤ[i] or arrays.
3. **Gap statement a library or language could use:** "CAS and FLINT give exact ℤ[i] one
   boxed element at a time; SIMD/GPU libraries give ℤ/p at full lane width; APL and J
   give Gaussian GCD in array style but on doubles; q gives the style with no complex type
   at all. A packed ℤ[i] or ℤ[i]/n column, with two machine-integer lanes per element,
   overflow and reduction proved in Lean, and q-style whole-array verbs, sits in the
   middle of these and was not found."
4. **Leads not followed up** (worth a later pass): the hex blog post on HexNumberField and
   the HexArith source (blocked here); NARS2000's hypercomplex component types; whether
   FLINT's `fq_nmod`/`fq_zech` special-case GF(p²) for p ≡ 3 mod 4; any Gaussian-moat or
   Gaussian-prime sieve on GPU; Kap's numeric tower.
