# Prior art: second, thorough pass

Oct 27, 2026 · second pass for [idea.md](idea.md), following [prior-art-light.md](prior-art-light.md)

## How to read this

The three properties are the same as in the first pass:
- **(S) shared factorization:** pairwise-coprime denominator nodes, shared across values;
- **(L) lazy:** staged decomposition that refines nodes later and never changes a value;
- **(G) grouping:** values with identical denominators stored as dense numerator arrays.

Every claim is tagged by its source:
- **[read]:** primary source or source code read for this pass;
- **[tested]:** run on this machine;
- **[docs]:** official documentation page;
- **[abstract]:** abstract or metadata only;
- **[memory]:** from prior knowledge, not re-checked here.

Only one paper was read in full: Brown 1974, from the Bell Labs report scan, through an extracted-text fetch whose OCR is partly garbled. Source code was read for:
- FriCAS `Factored`;
- Q-exact;
- np-extras `FactoredRational`;
- arithmoi `Prefactored`;
- relp / relp-num;
- Sage `Factorization`.

## Corrections to the first pass

1. **ALTRAN DOIs and authors.**
   - Brown, "On computing with factored rational expressions", is *SIGSAM Bull.* 8(3):26–34, DOI **10.1145/1086837.1086842**.
   - Hall, "Factored rational expressions in ALTRAN", is 8(3):35–45, DOI **10.1145/1086837.1086843**.

   The first pass gave …843 as Brown's. The **benchmarks are in Hall's paper**: Brown's has none and defers to Hall.
2. **ALTRAN rejects the pairwise-coprime invariant.** Brown considers keeping all factors pairwise relatively prime and rejects it as too costly. Only the numerator and denominator are kept coprime.
3. **Q-exact *does* add in factored form.** It takes the per-prime minimum exponents, scales the odd residues and adds them. It then trial-factors only the sum. It is not "naive", but it is limited to 53-bit safe integers and throws on overflow.
4. **np-extras `FactoredRational` uses a global base.** It is `FQ Bool [Integer]`: a dense exponent list indexed by the global prime sequence 2, 3, 5, …. Its nodes are therefore implicitly shared by all values. Factoring is plain trial division by every prime.
5. **The Dahan et al. citation is wrong.** "Dahan, Moreno Maza, Schost, Wu, Xie 2005" is *Lifting techniques for triangular decompositions* (ISSAC 2005), about polynomial triangular sets. It is not an integer coprime-base paper.
6. **Three new close items:**
   - **FriCAS `Factored`**, used as `Fraction Factored Integer`;
   - **Rich and Stoutemyer's quasi-primes**;
   - **relp-num's staged factorization.**

## Summary table

| Item | Domain | S shared | L lazy | G grouped | Benchmarkable here |
|---|---|---|---|---|---|
| ALTRAN (Brown; Hall 1974) | polynomials | partly: factor objects shared by pointer | partly: effort levels, on demand | no | no |
| FriCAS `Fraction Factored Integer` | integers, rationals | no: per value | partly: "nil" flags; integers fully factored on entry | no | yes (apt, installed) |
| Rich and Stoutemyer quasi-primes | radicands | coprime, sharing not stated | staged, not deferred | no | no (proposal) |
| relp-num `factorize` | integers, rationals | no | staged, not deferred | no | yes (cargo) |
| arithmoi `Prefactored` | integers | no: per value, coprime | no | no | yes (cabal, not tried) |
| np-extras `FactoredRational` | rationals | implicitly: global prime index | no: eager | no | yes (cabal, not tried) |
| Q-exact | rationals < 2^53 | no | no: eager | no | yes (npm) |
| BDS / Bernstein coprime bases | integers | it *is* the shared coprime base | algorithm | no | yes (FLINT, PARI, dcba) |
| Bernstein smooth parts (2004) | integers, batch | batch over many values | algorithm | no | yes (source, not tried) |
| PARI famat, `Z_cba` | integers, ideals | no | no | no | yes (gp installed) |
| Hecke `FacElem` | number fields, Q | no | no | no | Julia, not here |
| FLINT `fmpq_mat`, LinBox `VectorFraction` | Q | no (opaque denominator) | no | yes, per structure | yes (apt) |
| Magma, Sage, Maple, Mathematica, Singular, REDUCE | — | no | no | no | partly |

## Part 1: the near misses, rechecked

### 1. ALTRAN factored rational expressions (Brown 1974; Hall 1974)

- **What [read, Brown]:** rational expressions are "formal quotients of relatively prime, possibly factored polynomials".
  - **Factors:** carry exponents and need not be irreducible. Brown states "we cannot afford to maintain all numerators and denominators in fully factored form". He rejects refining factors until they are pairwise relatively prime, as too costly.
  - **Canonical form:** the numerator and denominator are coprime.
  - **Addition:** (AD ± BC)/BD. A common divisor of the numerators is pulled out, H = gcd(B, D) is formed from the explicit shared factors, and gcd(H, N) is taken on the new numerator. Brown notes that A/Bᵐ + C/Bⁿ needs no gcd at all, because the shared factor is visible.
  - **Multiplication:** explicit common factors are cancelled first. It then suffices to take gcd(A, D) and gcd(B, C), which is Henrici's rule.
  - **Effort levels:** three common-divisor levels, icd, idcd and idgcd. Roughly: identical-factor matching; plus divisibility tests between factors; plus Euclid on factor pairs. The user may change the strategy.
  - **Sharing:** the system "need not maintain multiple copies of a shared factor, but instead can access a single copy through multiple pointers". Brown argues sharing arises naturally from addition, differentiation, substitution and determinants.
- **What [abstract, Hall]:** detailed system-independent algorithms for equality, exponentiation, multiplication and addition, with options for factoring effort, and benchmarks against the earlier algorithms. Hall's authorship is inferred from Brown's reference to "a companion paper" by A. D. Hall; scilit lists no authors.
- **Closeness:**
  - **S partly.** Factor objects are shared by pointer, and the cheapest test is identity, which is close to "shared nodes". There is no global index, and factors within one value need not be coprime.
  - **L partly.** Effort is chosen per operation and gcds are computed "only to the extent necessary"; nothing is deferred to the background.
  - **G no.**

  It is still the closest conceptual precedent, especially the reduction rule that matches known factors before any general gcd. It lives in the polynomial domain.
- **Links:** Brown full text (Bell Labs CSTR 22 scan): <https://mirror.tipsy.coffee/packages/cios-dhitechnical/TheTelecomArchive_Mirror/CSTR%2022.pdf>; Brown abstract: <https://scilit.com/publications/3fa72d0e13105d9436131dd90a74ca09>; Hall abstract: <https://scilit.com/publications/02a599058af86c24f6a3570608869631>
- **Benchmarkable:** no.

### 2. arithmoi `Math.NumberTheory.Prefactored` (Haskell)

- **What [read, source]:**
  - **Type:** `Prefactored a = Prefactored { prefValue :: a, prefFactors :: Coprimes a Word }`, documented as "pairwise coprime (but not necessarily prime) factors".
  - **Operations:** `*` gives `Prefactored (v1*v2) (f1 <> f2)`, where `<>` merges and refines to coprime form. `+` and `-` give `fromValue (v1 ± v2)`, which drops the factors. `signum` drops them too.
  - **Scope:** integers only, MIT licence, Andrew Lelechenko 2017.
- **Closeness:**
  - **S per value only:** the coprime invariant matches the idea exactly, but nothing is shared between values.
  - **L only in Haskell's lazy-evaluation sense.**
  - **G no.** There is no rational type.
- **Link:** <https://hackage.haskell.org/package/arithmoi-0.13.2.0/src/Math/NumberTheory/Prefactored.hs>
- **Benchmarkable:** cabal and ghc are in apt; not tried. Use it as a reference for coprime merging (`splitIntoCoprimes`), not as a rational contender.

### 3. np-extras `MathObj.FactoredRational` (Haskell)

- **What [read, source 0.3.1.2]:**
  - **Type:** `data T = FQZero | FQ Bool [Integer]`, a sign plus "prime exponents", positional over the infinite prime list.
  - **Factoring:** "a simple factoring method" that divides by each prime in turn, with no bound.
  - **Addition:** converts both values to ordinary rationals, adds and re-factors. The docs suggest factoring only the relatively prime parts, but that is not implemented.
- **Closeness:**
  - **S:** the base is global (the primes), so every value implicitly shares nodes. This is the static, fully prime special case of the idea's graph, stored as dense per-value exponent vectors.
  - **L no:** factoring is eager and full, so large primes are impractical.
  - **G no.**
- **Link:** <https://hackage.haskell.org/package/np-extras-0.2/docs/MathObj-FactoredRational.html>; source: <https://hackage-content-origin.haskell.org/package/np-extras-0.3.1.2/candidate/src/MathObj/FactoredRational.hs>
- **Benchmarkable:** cabal; not tried. It is a weak contender.

### 4. Q-exact (JavaScript)

- **What [read, source `lib/q.js`, v0.1.2, last commit 2018-12-29, ISC, F. Trotta]:**
  - **Representation:** a sign plus an object mapping prime to signed exponent.
  - **Addition (`addFactors` / `decompose`):** takes the minimum exponent per prime as the common factor, multiplies the leftover odd prime powers into each term, adds, and factors only the sum, by least-prime search. It throws on overflow.
  - **Range:** everything is IEEE doubles, so integers are limited to 2^53.
- **Closeness:**
  - **S per value only.** Its addition is the idea's "scale by missing node powers" step, in miniature.
  - **L no.**
  - **G no.**
- **Link:** <https://github.com/fasttime/q-exact>
- **Benchmarkable:** npm and node are present. It is a curiosity only, because of the 53-bit limit.

### 5. Factor refinement and coprime bases

- **Bach, Driscoll and Shallit:** "Factor refinement", *J. Algorithms* 15(2):199–222, 1993, DOI 10.1006/jagm.1993.1038; also Univ. Wisconsin TR 883, 1989 [abstract / metadata]. The quadratic bound is [memory]. A 2020s analysis by Ravi and Volkovich relates its output to square-free decomposition and multiplicative functions [abstract].
- **Bernstein, "Factoring into coprimes in essentially linear time":** *J. Algorithms* 54 (2005) 1–30 [abstract + intro read]. It computes the natural coprime base of a set "as a substitute for prime factorization in many applications". It has no data-structure use; a proof-of-concept implementation is linked (licence not stated).
- **Bernstein, "Fast ideal arithmetic via lazy localization":** ANTS-II, LNCS 1122 (1996) 27–34 [abstract]. It "proposes a new representation for ideals", with arithmetic "generally much faster" than the Z-basis or two-element forms. This is the only literature found where coprime splitting done lazily *is the representation*, but for ideals of orders in number fields. **Worth reading in full.**
- **Bernstein, "How to find smooth parts of integers":** 2004 draft [abstract]. Given primes P and integers S, it finds the P-smooth part of every element of S at once in b(lg b)^(2+o(1)) time. That is stage 2 batched over a whole column of denominators.
- **Closeness:**
  - **S:** a coprime base *is* the idea's denominator side;
  - **L:** algorithms only, not lazy representations (except lazy localization, for ideals);
  - **G no.**
- **Links:** <https://cr.yp.to/coprimes.html>, <https://bibbase.org/network/publication/bach-driscoll-shallit-factorrefinement-1993>, <https://doi.org/10.1007/3-540-61581-4_38>, <https://nymity.ch/anomalous-tor-keys/bibliography/pdf/Bernstein04.pdf>
- **Benchmarkable:**
  - FLINT `fmpz_factor_refine` (C);
  - PARI `Z_cba` and `ZV_cba` (C). [tested] The symbols are present in gp 2.15.4. Per Belabas (pari-users, 9 Dec 2020), `Z_cba` "is linear time" for two inputs, but `ZV_cba` "still uses the naive quadratic algorithm".
  - Bernstein's dcba (C++, source build).

### 6. FLINT `fmpq_mat` and factoring primitives

- **What:**
  - **[docs]** Conversion helpers: `fmpq_mat_get_fmpz_mat_entrywise`, `_matwise` (a single common denominator), `_rowwise`, `_rowwise_2` and `_colwise`. `fmpq_mat_mul` uses `mul_cleared`, and the solvers "clear denominators to obtain a rescaled system over the integers".
  - **[memory]** Storage: the matrix itself is an array of `fmpq`, each entry with its own denominator, so the shared denominator exists only transiently.
  - **[docs]** Factoring: `fmpz_factor_trial` (up to 3512 primes); `fmpz_factor_trial_range` (reports completeness); `fmpz_factor_smooth` (trial division to a bit bound, then primality and perfect-power tests, leaving a cofactor); `fmpz_factor_refine` (pairwise-coprime bases); plus p+1 and ECM.
  - **[tested]** python-flint 0.9.0 `fmpz.factor_smooth(16)` on 2^10·3^5·1000003·(2^61−1) returns `[(2,10),(3,5),(2305849926742721592081853,1)]`. That last entry is exactly an "unsplit leftover node".
- **Closeness:**
  - **S no:** the denominator is opaque.
  - **L no**, though the primitives match stages 1–4.
  - **G partly:** per matrix, row or column, and transient.
- **Links:** <https://flintlib.org/doc/_sources/fmpq_mat.rst.txt>, <https://flintlib.org/doc/_sources/fmpz_factor.rst.txt>
- **Benchmarkable:** yes. libflint-dev 3.0.1 is in apt, and python-flint 0.9.0 is installed (it exposes `fmpq_mat`, `fmpz_mat` and `factor_smooth`, but not `refine`).

### 7. LinBox `VectorFraction`

- **What [docs]:**
  - "a vector of rational elements with common reduced denominator";
  - `combineSolution` replaces the vector by a combination with the gcd of the two denominators;
  - `axpyin` computes this += a·x;
  - `simplify` reduces to lowest terms.

  That the denominator is a single ring element is inferred, not stated.
- **Closeness:**
  - **S no.**
  - **L no.**
  - **G yes, for one vector.**
- **Link:** <https://linalg.org/linbox-html/class_lin_box_1_1_vector_fraction.html>
- **Benchmarkable:** liblinbox-dev 1.7.0 is in apt (C++). It is the same shape as the vector form; the FLINT variants are simpler to drive.

### 8. Calcium `ca_factor`

Not rechecked: the FLINT `ca` docs URL was blocked this session. The first-pass description (effort flags `ZZ_NONE`/`ZZ_SMOOTH`/`ZZ_FULL`, used inside simplification) stands as [docs, first pass].

## Part 2: the wider search

### 9. FriCAS / Axiom `Factored`, and `Fraction Factored Integer`

- **What [read, `src/algebra/fr.spad`; tested, FriCAS 1.3.10]:**
  - **Values:** a unit plus a list of `(flag, base, exponent)`. The flag is one of "nil", "sqfr", "irred" or "prime", recording **what is known about the base**: "nil" means nothing is known, which is the idea's "leftover treated as if prime".
  - **Multiplication:** merges lists and adds exponents on identical bases.
  - **gcd:** matches identical primes. When any factor is not prime or irreducible, it computes a **coprime-base decomposition** (`gcdDecomposition$GcdBasis`).
  - **Addition:** `u + v == ((expand u exquo g) + (expand v exquo g)) * g` with `g := gcd(u, v)`, i.e. the factored common part is kept and the cofactors are added.
  - **Coercion from integers:** coercing an `Integer` into `Factored Integer` calls the full `factor()`, so each new cofactor sum is factored **eagerly**.
  - **No refinement:** a source comment says the code "assumes that factors either are equal or coprime" and that gcd refinement "is more expensive than current implementation and not done".
  - **The rational type:** `Fraction Factored Integer` is the obvious composition.
- **What it did when run [tested]:**
  - `1/12 + 5/18` gave `13/(2²·3²)`, with prime-flagged factors.
  - `gcd(nil 77, nil 91)` gave 7, flagged nil.
  - `nil 77 * nil 91` kept **two non-coprime factors** (77 and 91).
  - `nil 77 + nil 91` gave `2³·3·7`: the shared 7 was kept nil and the cofactor sum 24 was fully factored.
  - A rational whose denominator is the product of two Mersenne primes (2^61−1 and 2^89−1, about 150 bits) did not finish within 300 s, in a batch with other small lines. This is consistent with eager factoring.
- **Closeness:** the most complete *per-value* analogue in a released system.
  - **Knowledge flags:** knowledge levels per factor resemble the idea's unsplit leftover nodes.
  - **Reduction:** coprime-base gcds are used for reduction.
  - **Addition:** done in factored form.

  Against the idea:
  - **S no:** no sharing across values and no coprime invariant;
  - **L partly:** nil factors persist, but integers are fully factored on entry;
  - **G no.**
- **Links:** <https://fricas.github.io/api/Factored.html>, <https://github.com/fricas/fricas/blob/master/src/algebra/fr.spad>
- **Benchmarkable:** yes. It is installed here (`apt install fricas`, on GCL Lisp). It will be slow; it is useful as a correctness oracle and as a "per-value factored rational" data point.

### 10. Rich and Stoutemyer, fractional powers of rationals (arXiv 1302.2169)

- **What [read, partly, via ar5iv; the venue is not stated, and the text is dated 2018]:** about representing "absurd numbers", rationals times fractional powers of rationals. Section 2.5 proposes a hybrid:
  - factor radicands only up to a chosen prime bound;
  - perfect-power-factor larger composites and treat them as **"quasi-primes"**, the same as primes;
  - "compute gcds between each new quasi prime and any other quasi primes" to split them;
  - keep radicands pairwise coprime.

  The authors note the form "is not necessarily canonical" with quasi-prime radicands. No implementation is named (two authors wrote Derive).
- **Closeness:** this is stages 2, 3 and 5 plus the coprime invariant, almost verbatim, for radicands rather than denominators.
  - **S:** coprime across the radicands of one expression; a global table is not stated.
  - **L:** staged by bound, not deferred.
  - **G no.**

  It is the strongest evidence that the decomposition pipeline is known.
- **Link:** <https://arxiv.org/abs/1302.2169>
- **Benchmarkable:** no.

### 11. relp / relp-num (Rust exact LP solver)

- **What [read, source relp-num 0.2.1, relp 0.2.6]:**
  - **Trait:** `NonZeroFactorizable` factors integers and rationals "into numbers that are often primes", into `{sign, factors: Vec<(Factor, Power)>, residual}` with `value == sign·residual·∏factorᵖ`. `is_complete()` is `residual == 1`.
  - **u64 pipeline:** a small odd-prime table, then trial division up to a limit, then Pollard rho (off under the crate's default constants, per a test comment), then the residual.
  - **Use in relp:** prescaling only. All matrix entries are factorized at once, an optimization is solved per prime, and the rows and columns are scaled.
- **Closeness:**
  - **S no.**
  - **L staged, not deferred.**
  - **G no.**

  It is a near-identical implementation of stages 1–4 for u8–u64, with a residual that plays the role of the leftover node. Doing "work per prime across a whole matrix" is a related idea.
- **Link:** <https://docs.rs/relp-num/latest/src/relp_num/traits/factorization.rs.html>
- **Benchmarkable:** yes (cargo is present). It is a direct stage-pipeline baseline.

### 12. PARI/GP famat, `Z_factor_limit`, `Z_smoothen`

- **What:**
  - **[first pass, docs]** famat is (object, exponent) matrices for the multiplicative group, with no addition and no coprime requirement.
  - **[tested]** gp 2.15.4 links `famat_reduce`, `Z_factor_limit`, `Z_smoothen`, `Z_cba` and `ZV_cba`.
  - **[memory]** `Z_smoothen` divides out a given prime vector and returns the cofactor.
- **Closeness:** low for the representation. It supplies primitives for stages 2 and 5.
- **Link:** <https://pari.math.u-bordeaux.fr/dochtml/html-stable/usersch6.html>
- **Benchmarkable:** yes. gp is installed, and libpari-dev 2.15.4 is in apt.

### 13. Hecke / Oscar `FacElem` (Julia)

- **What [docs]:**
  - factored elements ∏aᵢ^eᵢ, which "can easily be multiplied, raised to large powers, but in general not compared and not added";
  - `simplify` makes the base coprime for integer and rational `FacElem`;
  - `factor_coprime` gives a partial factorization into coprime pieces.

  Whether bases are shared is not stated.
- **Closeness:**
  - **S per value**, coprime after `simplify`.
  - **L no.**
  - **G no.**
  - There is no addition.
- **Link:** <https://docs.oscar-system.org/v1/Hecke/manual/misc/FacElem/>
- **Benchmarkable:** Julia is not installed here; it is pip-free but downloadable. Not worth it.

### 14. Magma `RngIntEltFact`, `PartialFactorization`, `CoprimeBasis`

- **What [docs]:**
  - factorization sequences of increasing distinct primes;
  - `+` exists as `RngIntEltFact, RngIntEltFact -> RngIntEltFact`, but its implementation is undocumented (closed source);
  - `-` is allowed only when the result is positive;
  - `/` errors on a non-integer quotient;
  - `PartialFactorization` (Cremona's algorithm) uses "only gcd and exact integer division", and its outputs are equal or pairwise coprime across inputs.
- **Closeness:** low. These are integer factorization objects with no rational type and no laziness.
- **Links:** <https://magma.maths.usyd.edu.au/magma/handbook/text/185>, <https://magma.maths.usyd.edu.au/magma/handbook/text/186>
- **Benchmarkable:** no (commercial).

### 15. Sage `Factorization`

- **What [read, source]:** `__add__` returns `self.value() + other`, documented as "the (unfactored) sum". Negative exponents are allowed, for example `factor(-10)/factor(16)`.
- **Closeness:** low. It is a container.
- **Link:** <https://doc.sagemath.org/html/en/reference/structure/sage/structure/factorization.html>
- **Benchmarkable:** no (no apt package here).

### 16. Rational internals in other systems [memory unless noted]

- **Maple:** a `RATIONAL` DAG holds an integer numerator and a positive integer denominator. All DAGs are hash-consed through the simplification table, so **equal denominators are physically shared**, but they are not factored. The Maple guide PDFs were not retrievable this session.
- **Mathematica:** `Rational[n, d]` is an atomic pair of bignums.
- **Singular:** `longrat` pairs of GMP integers, with an immediate small-integer fast path.
- **REDUCE:** rationals are `(:rn: n . d)`. Its `facform` module (Hearn, modified by Wright, RAND 1990) [read, header] is a factored form for **polynomials**: "a product of a monomial quotient and powers of non-monomial factors".
- **Maxima:** CRE rational functions over integer coefficients.
- **SymPy:** `Rational` is a pair of integers; `factorrat` is a utility.

None stores rational *numbers* with factored denominators.

### 17. Other things checked, all low closeness

- **King, "Numbers as data structures: the prime successor function as primitive"** (arXiv 1104.3056) [abstract and summary]: "prime bags", multisets of primes with negative multiplicities for rationals. Addition "appears to be computationally hard". A curiosity.
- **Hehner and Horspool, "A new representation of the rational numbers for fast easy arithmetic"** (*SIAM J. Comput.* 8(2), 1979) [abstract]: p-adic "quote notation". It is a different alternative to num/den and is not factored.
- **D5 dynamic evaluation** (Della Dora, Dicrescenzo, Duval 1985) [memory]: splits lazily when a zero-test forces it, without changing results. It is a philosophical precedent for lazy refinement, for algebraic numbers. Not examined.
- **Exact LP and SMT solvers** (SoPlex exact, QSopt_ex, Z3, cvc5): one search found no factor-aware or shared-denominator storage. **Not examined in depth.**
- **Patents:** one search found nothing relevant.
- **"Rationals as numerators linked to shared prime or coprime denominator nodes":** several phrasings, no hits.

## Is the combination new?

**Verdict: probably yes as a combination, and no in any single part.** Confidence is moderate, about 70% that no published or released system does all three of S, L and G for rational numbers.

What is known, and where:
- **Per-value factored rationals with factored addition:**
  - ALTRAN, for polynomials, with pointer-shared factor objects and effort levels;
  - FriCAS `Fraction Factored Integer`, with knowledge flags per factor and coprime-base gcds;
  - Q-exact and np-extras, which are toys.
- **The decomposition pipeline:**
  - staged trial division, primality test, rho and a leftover: FLINT `fmpz_factor_smooth`, relp-num;
  - "quasi-primes split by gcd and kept coprime": Rich and Stoutemyer;
  - coprime-base refinement: BDS 1993, Bernstein 2005, with implementations in FLINT, PARI and Magma.
- **Lazy coprime splitting as a representation:** Bernstein's "lazy localization", for ideals (abstract only).
- **Shared-denominator arrays:** FLINT `fmpq_mat` helpers, LinBox `VectorFraction`, decimal columns. The denominator is always opaque.

What was not found:
- **A shared base across values:** a global, growing, pairwise-coprime denominator base shared by *many independent values*. np-extras' fixed global prime index is the static, eager special case; Maple's hash-consing shares whole denominators, not their factors.
- **Grouping by identical edge set**, with the graph as the index and dense numerator vectors as the storage.
- **Deferred or background refinement** that moves values between groups and never changes them.

The novelty, then, is the combination plus the engineering: the base is shared and refined over time, and the grouping falls out of that. A referee would likely call it a composition of known techniques (ALTRAN or FriCAS factored arithmetic, plus coprime bases, plus a shared-denominator vector) and judge it on the measurements.

Sources of uncertainty:
- **Closed systems:** the internals of Maple, Mathematica and Magma could not be inspected.
- **Unread papers:**
  - Hall 1974 (only Brown was read in full);
  - Bernstein 1996, whose "new representation for ideals" might generalize;
  - the citing literature of BDS 1993 and Bernstein 2005, not traced.
- **Exact LP and SMT solvers** were not inspected.
- **Search coverage:** a forward-citation search was not possible with these tools, so absence remains weak evidence.

**Read next:** Bernstein 1996 in full, and Hall 1974 for the measured speedups.

## Candidate benchmarks, ranked

1. **FLINT vector form:** `fmpz_mat` / `_fmpz_vec` over a shared `fmpz`, plus the `fmpq_mat` row, column and matrix variants. C, libflint-dev 3.0.1 (apt); python-flint 0.9.0 is installed for quick checks. *Covers G*, the case the idea must not lose.
2. **GMP `mpq_t` and FLINT `fmpq`.** C, apt. *The per-value baseline* for mixed, unrelated and single-value workloads (already planned).
3. **FLINT `fmpz_factor_trial_range`, `fmpz_factor_smooth` and `fmpz_factor_refine`.** C, apt. *Covers the decomposition stages and coprime refinement.* Compare these against the idea's ctz, table, Miller–Rabin and rho pipeline per denominator.
4. **relp-num `factorize`.** Rust, cargo (present). *Covers stages 1–4 for u8–u64*, the closest existing pipeline, with residuals.
5. **PARI `Z_cba`, `ZV_cba`, `Z_factor_limit` and `Z_smoothen`.** C, libpari-dev 2.15.4 (apt; gp installed). *Covers coprime-base refinement (stage 5)*; note that `ZV_cba` is quadratic.
6. **Bernstein's batch smooth parts and dcba.** C or C++, source build (licence unstated, not tried). *Covers batched stage 2 and refinement over a whole column.* It is the right baseline if decomposition is done per group rather than per value.
7. **FriCAS `Fraction Factored Integer`.** SPAD on GCL, apt (installed). *Covers the whole per-value factored-rational representation, without S or G.* It is slow, but it is the only released "factored rational with factored addition" for arbitrary-size integers, and a good correctness oracle.
8. **Q-exact.** JS, npm. *Covers per-value factored addition*, below 2^53 only. A curiosity.
9. **arithmoi `Prefactored` / `splitIntoCoprimes`.** Haskell, cabal (apt; not tried). A design reference for coprime merging; not a speed contender.

Not benchmarkable: ALTRAN, Magma, Maple, Mathematica (unavailable or closed), and Hecke `FacElem` (no addition).
