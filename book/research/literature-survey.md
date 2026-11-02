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

---

## Milestone 2: libraries and languages

(pending)
