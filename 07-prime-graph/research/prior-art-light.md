# Prior art: light first pass

Oct 27, 2026 · light scan (about 30 minutes of web searching) for [idea.md](idea.md)

**This is a first pass, not a review.** Each item comes from documentation pages, source files and abstracts found by search. No papers were read in full, nothing was benchmarked, and absence of evidence here is weak evidence. Closeness is judged against the idea's three distinguishing pieces:
- (a) denominators kept as products of pairwise-coprime nodes, with sharing across values;
- (b) lazy, staged decomposition that refines the nodes;
- (c) values with identical denominators grouped into dense numerator arrays.

## Summary

Nothing found combines all three. Each piece has solid prior art on its own:
- **(a) + (b):** factor refinement and coprime bases are well studied and implemented in FLINT, PARI, Magma, CoCoA and arithmoi. They are used as **number-theory utilities**, not as the storage format of a rational type.
- **Factored rationals as a value type:** these exist (Haskell `FactoredRational`, JS Q-exact, Calcium `ca_factor`). They factor **eagerly and per value**, with no sharing between values, and none of them adds in factored form.
- **(c):** "integer vector over one shared denominator" is standard (FLINT `fmpq_mat` matwise/rowwise/colwise, LinBox `VectorFraction`, decimal columns). The denominator there is an opaque integer, never factored or partly shared.

**Closest single item:** the factored rational expressions in ALTRAN (Brown; Hall, SIGSAM Bulletin 1974). ALTRAN represented rational *functions* as quotients of products of factor powers. It offered user control over how hard to look for factors, and it had algorithms for addition and multiplication in factored form, with benchmarks. That is the idea's (a) + (b) + factored addition, in the polynomial domain. It has no grouping (c) and no integer-specific staging. The closest **integer** analogue is arithmoi's `Prefactored`: a value plus a pairwise-coprime, not necessarily prime, factorization that is preserved under multiplication and dropped under addition.

## Items

### 1. ALTRAN factored rational expressions (1974)
- **What:** W. S. Brown, "On computing with factored rational expressions", and A. D. Hall Jr., "Factored rational expressions in ALTRAN". Both appeared in *ACM SIGSAM Bulletin* 8(3), 1974; DOI 10.1145/1086837.1086843 is the first. A rational expression is a quotient of factored polynomials. Options control how much effort goes into finding factors or canonicalizing. Algorithms are given for equality, exponentiation, multiplication and addition, with benchmarks against earlier approaches. Attribution of authors is from memory plus abstract pages; confirm in the second pass.
- **Closeness:** high in spirit, for (a), (b) and factored addition and reduction. The domain is different: polynomials, not integers. It has no pairwise-coprime invariant as stated (to confirm), no background refinement, and no grouping into vectors.
- **Link:** <https://scilit.com/publications/02a599058af86c24f6a3570608869631>, <https://scilit.com/publications/3fa72d0e13105d9436131dd90a74ca09>
- **Benchmarkable implementation:** no. ALTRAN is historical. Read the papers for the addition algorithm and how effort is controlled.

### 2. arithmoi `Math.NumberTheory.Prefactored` (Haskell)
- **What:** `Prefactored a` holds a value together with "its pairwise coprime (but not necessarily prime) factorisation", "designed to preserve information about factors under multiplication". Multiplication merges factor lists and refines them to coprime form via `Math.NumberTheory.Euclidean.Coprimes` (`splitIntoCoprimes`). `+` and `-` drop the factors. It is for integers only; there is no rational or division instance.
- **Closeness:** the closest match to the idea's **invariant**: pairwise-coprime nodes, primality optional, refinement never changes the value. It is per value, has no denominators, no sharing between values, no staged prime search and no grouping.
- **Link:** <https://hackage.haskell.org/package/arithmoi-0.13.2.0/src/Math/NumberTheory/Prefactored.hs>, <https://hackage.haskell.org/package/arithmoi-0.13.2.0/src/Math/NumberTheory/Euclidean/Coprimes.hs>
- **Benchmarkable implementation:** it exists, but it is the wrong shape for a rational-arithmetic benchmark. Use it as a design reference for coprime-merge code.

### 3. Factor refinement and coprime bases (Bach–Driscoll–Shallit, Bernstein, and others)
- **What:**
  - Bach, Driscoll and Shallit, "Factor refinement" (SODA 1990; *J. Algorithms* 15, 1993): given a list of integers, refine it to a pairwise-coprime list with the same product. This is quadratic.
  - Bernstein, "Factoring into coprimes in essentially linear time" (*J. Algorithms* 54, 2005), and "Detecting perfect powers by factoring into coprimes" (*Math. Comp.* 2007).
  - Buchmann and Eisenbrand, "On factor refinement in number fields" (*Math. Comp.* 1999).
  - Dahan, Moreno Maza, Schost, Wu and Xie (2005) on gcd-free bases via subproduct trees, with parallel Cilk++ work at Western Ontario (2011).
  - Bernstein's page lists many applications, including "rational function representations", but none is "store every rational over a coprime base".
- **Closeness:** this is exactly the idea's stage 5 (gcd refinement of large leftovers) and its invariant. It is used as an algorithm, not as a representation of rationals.
- **Links:** <https://minds.wisconsin.edu/handle/1793/59196> (BDS tech report), <https://cr.yp.to/coprimes.html>, <https://www.ams.org/journals/mcom/1999-68-225/S0025-5718-99-01023-6>, <https://www.sambuz.com/doc/on-the-factor-refinement-principle-and-its-implementation-presentation-1089885>
- **Benchmarkable implementation:** yes, for the refinement step on its own:
  - FLINT `fmpz_factor_refine`: "bases are pairwise relatively prime", and the input need not be canonical;
  - PARI `Z_cba` (linear time, two inputs) and `ZV_cba` (still the naive quadratic algorithm, per Belabas, pari-users Dec 2020);
  - Magma `CoprimeBasis` and `PartialFactorization`;
  - CoCoA `GCDFreeBasis`;
  - Bernstein's proof-of-concept `dcba` C++.

### 4. FLINT partial and staged factoring (`fmpz_factor_smooth`, `fmpz_factor_trial_range`)
- **What:** factoring with an effort budget. `fmpz_factor_smooth` extracts primes up to a bit bound and leaves a possibly composite cofactor. `fmpz_factor_trial_range` reports whether the factorization is complete. `fmpz_factor_t` is a pair of base and exponent vectors plus a sign.
- **Closeness:** this matches stages 1–4 (staged trial division, then a leftover) as a primitive. It has no lazy or background scheduling and no rational type built on it.
- **Link:** <https://flintlib.org/doc/_sources/fmpz_factor.rst.txt>
- **Benchmarkable implementation:** yes. It is a natural baseline for the decomposition stages, compared against the idea's ctz, table, Miller–Rabin and rho pipeline.

### 5. Calcium / FLINT `ca_factor`
- **What:** factors a `ca` value, rationals included, into base^exponent pairs. The integer effort is set by a flag: `CA_FACTOR_ZZ_NONE`, `ZZ_SMOOTH` (with a configurable smooth limit and no primality test) or `ZZ_FULL`. Denominator primes get negative exponents. It is used inside simplification, not as the storage format.
- **Closeness:** the effort-level flags are the nearest found to "decompose only as far as it pays". It is on demand, not lazy, keeps no persistent shared nodes, and does not store values in factored form.
- **Link:** <https://fossies.org/linux/flint/src/ca/factor.c>, <https://fredrikj.net/calcium/index.html>
- **Benchmarkable implementation:** it exists, but it is not a rational-arithmetic contender.

### 6. Factored-rational value types: `FactoredRational` (Haskell np-extras) and Q-exact (JS)
- **What:**
  - **np-extras `MathObj.FactoredRational`:** a rational stored as a list of prime powers. Multiplication and division are cheap; addition converts to an ordinary rational, adds, and re-factors. Its docs suggest exactly the improvement the idea makes: find a common denominator, pull out common factors, and add only the coprime parts. It is aimed at combinatorics, such as factorials.
  - **Q-exact:** "Rational numbers in Q-exact are stored as products of prime factors", with signed exponents (−75/28 = −2⁻²·3·5²·7⁻¹). It is a tiny ISC-licensed JS project with 0 stars.
- **Closeness:** they match "rational = product of prime powers with signed exponents". Both factor eagerly and fully, so huge numbers are impractical. Each value is separate, nothing is shared, addition is naive, and there is no grouping. They are evidence that the representation alone is not new.
- **Links:** <https://hackage.haskell.org/package/np-extras-0.2/docs/MathObj-FactoredRational.html>, <https://github.com/fasttime/q-exact>
- **Benchmarkable implementation:** technically yes, but a weak contender (slow languages, eager factoring). Not worth benchmarking except as a curiosity.

### 7. PARI/GP factorization matrices ("famat")
- **What:** a two-column matrix of (object, exponent) standing for ∏gᵢ^eᵢ. It is used to avoid expanding huge powers in class-group, unit and ideal computations. It supports multiplication (concatenation), powers, inversion and `famat_reduce`. It has **no addition**, and the bases are not required to be coprime or even comparable.
- **Closeness:** low to moderate. It is a "keep it factored" representation for the multiplicative group only, not a general rational type. It is mainly a precedent for lazy expansion.
- **Link:** <https://pari.math.u-bordeaux.fr/dochtml/html-stable/usersch6.html>
- **Benchmarkable implementation:** not for this workload.

### 8. Magma `RngIntEltFact`, Sage `Factorization`
- **What:** factored-integer objects: sorted (prime, exponent) sequences in Magma, and generic `Factorization` and `IntegerFactorization` in Sage. They support multiplication and similar operations; addition was not checked. Magma also has `CoprimeBasis` and `PartialFactorization`, the latter using only gcds and exact division.
- **Closeness:** low. These are factorization containers, not a rational number type.
- **Links:** <https://magma.maths.usyd.edu.au/magma/handbook/text/185>, <https://doc.sagemath.org/html/en/reference/structure/sage/structure/factorization.html>
- **Benchmarkable implementation:** no, not for rational arithmetic.

### 9. Shared-denominator arrays: FLINT `fmpq_mat`, LinBox `VectorFraction`, exact linear algebra
- **What:**
  - **FLINT:** `fmpq_mat_get_fmpz_mat_matwise`, `_rowwise` and `_colwise` clear denominators to one per matrix, row or column. `fmpq_mat_mul` calls `fmpq_mat_mul_cleared`, and all solvers "clear denominators to obtain a rescaled system over the integers".
  - **LinBox `VectorFraction`:** a vector of numerators over a "common reduced denominator", with `combineSolution` (gcd of denominators), `axpyin` and `simplify`.
  - **IML** and Dixon or Hensel lifting solvers return integer vectors over one denominator.
- **Closeness:** this is exactly the idea's **vector form** (c) and the prior spec's design. The denominator is a single opaque integer. Nothing is factored or partly shared, and grouping is per structure (matrix, row, column), not by identical denominator across arbitrary values.
- **Links:** <https://flintlib.org/doc/_sources/fmpq_mat.rst.txt>, <https://linalg.org/linbox-html/class_lin_box_1_1_vector_fraction.html>
- **Benchmarkable implementation:** yes. FLINT `fmpz_mat` / `_fmpz_vec` over a shared `fmpz` denominator is already a listed contender, and the FLINT row and column variants are worth adding for the "mixed denominators" case.

### 10. Fixed-scale decimals and block floating point
- **What:**
  - Arrow `Decimal128` and `Decimal256`, and SQL `DECIMAL(p,s)`, store a column of integers with one scale per column, i.e. a shared denominator 10^s.
  - `BigDecimal` and Python `decimal` store one power-of-10 scale per value.
  - Block floating point (DSP, ML quantization) shares one power-of-2 exponent per block. It is inexact.
  - Dyadic types, such as CGAL `Gmpzf` and the hyperreal crate's dyadic fast path, fix the denominator to 2^k and reduce by shifts.
- **Closeness:** these are precedent for (c) with the denominator restricted to a single known prime power, 2^k or 10^s = 2^s·5^s. The idea generalizes this to any coprime base. None groups by arbitrary denominator.
- **Links:** <https://arrow.apache.org/docs/python/generated/pyarrow.decimal128.html>, <https://www.emergentmind.com/topics/block-floating-point-bfp-format>, <https://docs.rs/crate/hyperreal/latest/source/README.md>
- **Benchmarkable implementation:** Arrow compute kernels on `Decimal128` are a fair baseline for the decimal-column case.

### 11. Henrici's rational arithmetic (1956) and its descendants
- **What:** cancel by gcd before multiplying or adding, to keep operands small (Knuth, TAOCP vol. 2, §4.5.1). GMP and FLINT `fmpq` implement this per value, with general gcds.
- **Closeness:** the idea explicitly builds on it, replacing general gcds with divisibility tests against known nodes. This is the baseline, not a competitor in representation.
- **Benchmarkable implementation:** GMP `mpq_t` and FLINT `fmpq`, already planned.

### Searched, nothing found
- **"Rationals as a bipartite graph, numerators linked to prime denominator nodes":** no hits beyond unrelated material (Calkin–Wilf tree, factorization trees). The graph or index framing appears to be new as a description, though it is equivalent to "per-value sparse exponent vector over a shared, growing coprime base".
- **Lazy, background or incremental factorization inside an arithmetic library's number type:** nothing found for integers or rationals. Closest are the ALTRAN effort controls, the Calcium flags, and the D5 "dynamic evaluation" splitting in polynomial system solving, which was not examined.

## For the thorough second pass

1. **Read the ALTRAN papers in full** (Brown 1974; Hall 1974). Look at their addition algorithm in factored form, how they keep factors coprime or detect sharing, the effort controls, and their reported speedups. This is the most likely source of "already done, in polynomials".
2. **Check Maple, Mathematica, Reduce and Macsyma** for factored-normal-form rational *function* representations (Maple `factor` / `normal` options, Reduce `FACTOR` and `ON FACTOR`). The polynomial CAS world is where (a) and (b) most likely live; the question is whether any system applied it to integers.
3. **Look for an exact or symbolic system that stores rationals over a global coprime base**: SymPy, Giac, Axiom/FriCAS, Singular, Pari internals, `Nemo` / `Hecke.jl` (`FacElem` in Hecke is famat-like; check whether it handles addition or coprime bases, e.g. `coprime_base`), and Oscar.
4. **Track forward citations** of Bach–Driscoll–Shallit and Bernstein's dcba for any "representation" or "data structure" use. Bernstein's application list mentions "rational function representations"; find that paper.
5. **Look at exact LP and SMT solvers** (SoPlex exact mode, QSopt_ex, Z3 and cvc5 rationals, Yices) for per-row or per-column denominator sharing or factor-aware fast paths. They are heavy mpq users with real workloads.
6. **Check the D5 / dynamic evaluation** literature (Della Dora, Dicrescenzo, Duval) as a precedent for "refine the splitting lazily, never change the value".
7. **Benchmark baselines to add**, beyond those already listed:
   - FLINT `fmpz_factor_smooth` and `fmpz_factor_refine` for the decomposition stages;
   - PARI `Z_cba` and `ZV_cba` for refinement;
   - FLINT `fmpq_mat` rowwise and colwise denominators;
   - Arrow `Decimal128` kernels for decimal columns.
8. **Search patents** for "rational number", "shared denominator" and "prime factor" in hardware or database contexts (the fixed-scale decimal engines in databases).
