# Prior art: full review

Oct 28, 2026 · third and final prior-art pass for [paper/v0.1.md](../../paper/v0.1.md), "Adding Rationals over a Shared Coprime Base". It builds on [prior-art-light.md](prior-art-light.md), [prior-art-2.md](prior-art-2.md) and [prior-art-3-sharing.md](prior-art-3-sharing.md). Items those passes settled are only summarised here and point back to them.

## How to read this

**The idea under review (paper §§2–6).** Denominators are stored as exponent vectors over a coprime base that every value shares. The base is pairwise coprime but not necessarily prime, and it is refined incrementally (factor refinement) as denominators turn out to share factors. Addition reads the common denominator as a componentwise maximum, multiplies by the unshared cofactors, and reduces only by tests against base elements held at equal exponent. Values with identical vectors are stored as a common-denominator block, with lazy reduction.

**Source tags.** Every claim carries one:
- **[read]:** the primary text was fetched and read for this pass, sometimes through a summarising fetcher or OCR;
- **[src]:** source code was read for this pass (the repository was cloned or the package downloaded);
- **[docs]:** official documentation;
- **[abstract]:** only the abstract or metadata was seen;
- **[secondary]:** described by another work that was read;
- **[memory]:** prior knowledge, not re-checked.

**Closeness.** Rated against five pieces of the idea:
- **B:** a coprime base, primes optional;
- **E:** exponent-vector arithmetic in place of gcds;
- **S:** the base is shared by all values;
- **R:** the base is refined incrementally;
- **G:** common-denominator blocks with lazy reduction.

**Access limits this session.** Several hosts were blocked or failed:
- curl to cr.yp.to and minds.wisc.edu (proxy policy);
- WebFetch to Springer, ScienceDirect, doc.cgal.org and arrow.apache.org (allowlist).

As a result, BDS 1993, Hall 1974 and Bernstein 1996 could not be read in full. Where a source was blocked, its source code or a mirror was used where one existed.

## Corrections to earlier passes

1. **The "BDS abstract" on sciweavers belongs to another paper.** The abstract that sciweavers lists under "Factor Refinement, Bach–Driscoll–Shallit, SODA 1990" (orders, overorders, Ge's algorithm) is the abstract of **Buchmann and Eisenbrand 1999**, confirmed on the AMS page. Do not quote it as BDS.
2. **Hall 1974 authorship is confirmed.** Crossref lists Andrew D. Hall, Jr., Bell Laboratories, as the sole author of DOI 10.1145/1086837.1086843. prior-art-2 had inferred it.
3. **Henrici is not cited by Brown.** Brown 1974 never mentions Henrici, although his multiplication rule (gcd(A, D), gcd(B, C)) is Henrici's.
4. **"Lazy localization" is still unread.** prior-art-2 recommended reading it. No copy could be reached, and coprimes.html lists it under applications with no PDF.

---

## Part A. Key primary sources

### A1. Henrici 1956, rational arithmetic with gcd-first reduction
- **Citation:** P. Henrici, "A subroutine for computations with rational numbers", *J. ACM* 3(1):6–9, January 1956 [docs: JACM 1956 index].
- **Access:** the paper itself was not reachable. Its content is **[secondary]**, from Knuth, TAOCP vol. 2, §4.5.1, whose TeX source was read on SAIL.
- **What it does, as Knuth reports it:**
  - Exact fractions on computers were "first discussed in the literature by P. Henrici".
  - **Addition:** compute d₁ = gcd(u′, v′) first. If d₁ = 1, the plain cross-multiplied result is already reduced; Knuth says to "single out this case separately". Otherwise set t = u(v′/d₁) ± v(u′/d₁), d₂ = gcd(t, d₁), and return t/d₂ over (u′/d₁)(v′/d₂). Exercise 6 proves the result is reduced.
  - **Multiplication:** d₁ = gcd(u, v′) and d₂ = gcd(u′, v), cancelled before multiplying.
  - Knuth notes that d₁ = 1 "about 61 percent of the time" (Theorem 4.5.2D, 6/π²).
- **Who uses it today:**
  - **CPython `fractions`** [src, Python 3.13]: `_add` is exactly this. The module comment reads "Rational arithmetic algorithms: Knuth, TAOCP, Volume 2, 4.5.1."
  - **Z3** [src]: `mpq_manager::lin_arith_op` uses gcd(den_a, den_b) with a g = 1 fast path, then gcd(n, g).
  - **GMP `mpq_add`/`mpq_mul` and FLINT `fmpq`** [memory]: the same scheme.
- **Closeness:** this is the paper's §2 refinement exactly. It is the baseline the idea replaces, not a competitor. B, E, S, R and G are absent.

### A2. Brown 1974, "On computing with factored rational expressions" (ALTRAN)
- **Citation:** W. S. Brown, *ACM SIGSAM Bulletin* 8(3):26–34, 1974, DOI 10.1145/1086837.1086842.
- **Access: [read]**, the Bell Labs CSTR 22 scan through OCR (re-read for this pass, confirming prior-art-2).
- **What it does:**
  - **Representation:** rationals are "formal quotients of relatively prime, possibly factored polynomials". Factors carry exponents and need not be irreducible. Only the numerator and denominator are kept mutually coprime.
  - **No pairwise-coprime invariant:** Brown explicitly rejects factoring until "all its factors are pairwise relatively prime" ("Clearly we cannot afford this strategy either"), because even A·C + B·C would then need gcd(A + B, C). He also rejects full factorization.
  - **Addition:**
    - The key remark is that A/Bᵐ + C/Bⁿ with m > n needs **no gcd**: N = A + B^(m−n)·C, and N/Bᵐ is already canonical, because any irreducible dividing B and N would divide A.
    - In general: G = cd(A, C); H = gcd(B, D) is formed from the *explicit* shared factors; N = AD ± BC; and the final gcd is gcd(H, N) only.
  - **Multiplication:** explicit common factors are cancelled first, then gcd(A, D) and gcd(B, C) (Henrici's rule, uncredited).
  - **Effort levels:** three common-divisor routines:
    - icd: identity of factors;
    - idcd: plus divisibility tests;
    - idgcd: plus Euclid on pairs of factors.
  - **Sharing:** "the system need not maintain multiple copies of a shared factor, but instead can access a single copy through multiple pointers". Brown argues that sharing arises naturally from addition, substitution, determinants and series reciprocation.
- **Closeness:** the **highest conceptual match** of any single work.
  - **E partly:** shared factors are visible, so gcd(B, D) is "known without computing it". Unequal powers of a shared factor need no test, which is exactly the paper's §4.2 reduction rule.
  - **S partly:** factors are shared by pointer, but there is no global base or index.
  - **B no:** pairwise coprimality is explicitly rejected.
  - **R no:** effort levels replace refinement.
  - **G no.**
  - The domain is multivariate polynomials, not integers.

### A3. Hall 1974, "Factored rational expressions in ALTRAN"
- **Citation:** A. D. Hall Jr., *ACM SIGSAM Bulletin* 8(3):35–45, 1974, DOI 10.1145/1086837.1086843.
- **Access: [abstract]**, through Crossref and scilit. No full text was reachable (ACM DL blocked, no CSTR scan found).
- **What it does:** "detailed algorithms for the basic arithmetic operations on symbolic rational expressions" represented as formal quotients of factored polynomials. It covers:
  - the representation;
  - options to control effort in finding factors and canonicalising;
  - equality, exponentiation, multiplication and addition, with notes on division and subtraction;
  - **benchmarks against earlier methods.**
- **Closeness:** the same as Brown, which describes it as the companion paper. It is the only source of measured speedups for factored rational arithmetic. **This is still the main unread risk:** it could contain a global factor table or a coprime-merge step. Brown's text suggests neither.

### A4. Bach, Driscoll and Shallit, "Factor refinement"
- **Citations:**
  - SODA 1990, pp. 201–211 [memory for the pages];
  - *J. Algorithms* 15(2):199–222, 1993, DOI 10.1006/jagm.1993.1038 [docs: bibbase];
  - Univ. Wisconsin CS TR 883, 1989 [docs: MINDS metadata].
- **Access:** the TR PDF exists at MINDS but returned binary or was blocked. The content is **[secondary]** (Bernstein 2005; Balaji et al. 2022, arXiv 2202.07961; Ali's 2011 UWO slides) plus [memory].
- **What it does:**
  - **Input:** integers a₁, …, a_k.
  - **Output:** pairwise coprime m₁, …, m_ℓ, "not necessarily prime", with each aᵢ a product of powers of the m_j [secondary: Balaji et al.].
  - **Cost:** O(log² a) bit operations, a = lcm(aᵢ), i.e. quadratic; output size ℓ ≤ Σ log aᵢ [secondary].
  - **Method:** repeatedly replace a non-coprime pair (x, y) by x/g, g, y/g with g = gcd(x, y), with an "augmented" variant that keeps exponent bookkeeping [secondary: Ali slides; memory].
  - **Naming:** they coined the term **"factor refinement"** and credit earlier gcd-free-basis uses (Collins' CAD and others) [memory].
  - Bernstein lists them as the previous best and notes their element-by-element approach "inherently takes quadratic time" [read: dcba].
- **Closeness:** **B and R as an algorithm.** It is the source of the paper's term "factor refinement" and the right citation for §5.1. It is not a representation of values: there is no S (one list, refined once), no E for arithmetic, and no G.

### A5. Bernstein 2005, "Factoring into coprimes in essentially linear time"
- **Citation:** D. J. Bernstein, *J. Algorithms* 54(1):1–30, 2005; draft 2004-04-04.
- **Access: [read]** (the draft PDF, through WebFetch).
- **What it does:**
  - **Definitions:**
    - "P is a base for S" if S lies in the multiplicative closure of P;
    - "P is coprime" if its elements are pairwise coprime;
    - the **natural coprime base** cb(S) is the set of minimal elements of the closure of S under products, exact quotients and gcds.
  - **Setting:** "free coids", which include positive integers, monic polynomials over a field and ideals of a Dedekind domain.
  - **Algorithms**, all essentially linear time, using only multiplication, exact division, gcd and equality:
    - two-element base (13.2);
    - `split` (15.3);
    - **extend a coprime base by one element, cb(P ∪ {b}) (16.2)**;
    - merge two bases (17.3);
    - cb(S) (18.1);
    - factor over a coprime base, giving (p, n) pairs (20.1, 21.2).
  - **Motivation:** "Factoring into coprimes is often an adequate substitute for factoring into primes."
  - **Exponents:** "natural coprime bases act on exponents as greatest common divisors".
  - **Applications:** a list of about 30 citations, from Stieltjes 1890 onwards. None is a rational-number representation. The nearest are von zur Gathen 1986 (rational *functions*, Remark 6.8; not read) and Bernstein's own lazy localization (ideals).
- **Closeness:**
  - **B yes.**
  - **R yes**, as an algorithm: 16.2 is the paper's §5.3 step, done with a product tree.
  - **E partly:** it factors elements into (p, n) pairs over P, but no arithmetic on values is defined.
  - **S no:** P is a set of numbers, not a store shared by live values.
  - **G no.**

  It is the right citation for the paper's coprime base and its incremental extension, and the standard reference for "primes are not needed".

### A6. Bernstein 1996, "Fast ideal arithmetic via lazy localization"
- **Citation:** D. J. Bernstein, in H. Cohen (ed.), *Algorithmic Number Theory (ANTS-II)*, LNCS 1122, Springer, 1996, pp. 27–34, DOI 10.1007/3-540-61581-4_38.
- **Access: [abstract]**, from prior-art-2 (Springer page) and the cr.yp.to bibtex. No full text was reachable this pass (Springer and cr.yp.to papers page blocked; unpaywall shows no OA copy).
- **What it does:**
  - **[abstract]:** it proposes a new representation for ideals in orders of number fields, with arithmetic "generally much faster" than the Z-basis or two-element forms.
  - **[memory, low confidence]:** the ideal is handled through its localisations at a coprime set of moduli built from norms and discriminants, and moduli are split lazily when a computation reveals a factor.
- **Closeness:** possibly the only work where **a coprime base refined lazily *is* the storage**. It is for ideals, not rationals, and nothing indicates denominators, sharing across many values, or blocks.
  - **B likely.**
  - **R likely.**
  - **E, S, G unknown or no.**

  **Second main unread risk**, after Hall.

### A7. Rich and Stoutemyer, quasi-primes
- **Citation:** A. D. Rich and D. R. Stoutemyer, "Representation, simplification and display of fractional powers of rational numbers in computer algebra", arXiv:1302.2169, 2013.
- **Access: [read]**, §2.5, via ar5iv.
- **What it does:**
  - **The hybrid of §2.5:** radicands are factored into primes only up to a bound p̂. Larger composite leftovers are perfect-power-factored, and any radicand above p̂² becomes a **"quasi-prime"**, treated like a prime.
  - **Splitting:** gcds are taken between each new quasi-prime and the other quasi-primes, and against large rational numerators and denominators. A nontrivial gcd "splits the radicand or radicands".
  - **Invariant:** radicands are kept pairwise coprime, and coprime to the rational coefficient.
  - **Caveat:** the authors concede the form is "not necessarily canonical" with quasi-primes.
  - **No named techniques:** BDS, Bernstein, "factor refinement" and "coprime base" are not named.
  - **No implementation is reported.**
- **Closeness:**
  - **B yes**, within one expression.
  - **R yes:** staged factoring followed by gcd splitting.
  - **S no:** no global table.
  - **E only for radicand exponents.**
  - **G no.**

  The domain is radicands, not denominators. It is the closest precedent for the paper's §5.3 entry step: trial division, then leftovers as provisional base elements split by gcd.

---

## Part B. New areas

### B1. SMT solvers
| System | Rational type | Addition | Denominator or factor sharing | Tag |
|---|---|---|---|---|
| **Z3** | `mpq`: two `mpz`s, each a small `int` or a GMP pointer | Henrici (`lin_arith_op`): an int64 fast path, then gcd(den_a, den_b) with a g = 1 shortcut | none | [src] |
| **cvc5** | `Rational` wraps `mpq_class` (GMP) or CLN | GMP's | per constraint only: `Polynomial::denominatorLCM()` clears denominators so atoms have integer coefficients | [src] |
| **Yices 2** | `rational_t`: a union of a 32-bit num/den pair and a GMP `mpq` pointer | `q_add` → `mpq_add` on the slow path | per polynomial: `poly_buffer_make_integral`; lcm of a column's coefficients for integer periods | [src] |

The simplex papers themselves say nothing about representation. Dutertre and de Moura (CAV 2006) [read] treat the tableau as a fixed rational matrix over pairs of rationals (c + kδ), and are silent on gcds and denominators.

- **Closeness:** none. Each value is a canonical per-value rational with a small-integer fast path. Lcm clearing is a one-off normalisation of a constraint, not a storage format.

### B2. Exact LP solvers
- **SoPlex (exact mode)** [src, current master]:
  - **Type:** `Rational = boost::multiprecision::number<gmp_rational, et_off>`, one canonical mpq per entry. The rational LU (`clufactor_rational`) stores mpq entries.
  - **Transient common denominator:** `reconstructVector` takes the lcm of all entry denominators, rescales to an integer numerator vector, and reconstructs entries while accumulating a running gcd bound (an output-sensitive vector reconstruction).
  - **Statistic:** `dlcmSizeRational` reports the bit size of the lcm of a vector's denominators.
  - **Method:** exactness comes from iterative refinement and rational reconstruction (Gleixner, Steffy and Wolter) and, since v7, precision boosting (Eifler, Nicolas-Thouvenin and Gleixner 2023) [abstract / read].
- **QSopt_ex** (Applegate, Cook, Dash, Espinoza) [docs + memory]:
  - **Type:** GMP rationals, with the solver templated over number types (EGlib `EGlpNum`).
  - **Method:** exactness through precision boosting: double, then 128 bits, then ×1.5 up to a 3164-bit limit (per Eifler et al. 2023). Exact mpq is used for the final verification and solve.
  - **Sharing:** none known.
- **Integer-preserving (fraction-free) simplex:** Azulay and Pique.
  - **Citations:**
    - "Optimized Q-pivot for exact linear solvers", CP'98, LNCS 1520, pp. 55ff;
    - "A revised simplex method with integer Q-matrices", *ACM TOMS* 27(3), 2001, DOI 10.1145/502800.502804.
  - **What it does [abstract]:** the simplex formulas are rewritten with Edmonds' Q-matrices, so every tableau entry is an integer over **one shared denominator**, the determinant of the basis. It is computed with an integer multiprecision library, motivated by incremental solvers in constraint (Prolog) languages, with integer and rational versions compared.
  - **Closeness:** **G yes**, at the scale of the whole tableau, and **the strongest precedent for "a block of numerators over one denominator, with no gcds".** The denominator is opaque (a determinant), and there is no partial sharing.
- **relp** (Rust) was covered in prior-art-2: staged factoring for prescaling only.

### B3. Exact linear algebra
- **Integer-preserving Gaussian elimination (IPGE).**
  - **Theory:** Edmonds 1967 showed that every intermediate entry is a minor. Bareiss 1968 showed that the divisions are exact.
  - **Common denominator:** the solution is x = b^(m)/ρ^(m), with the **determinant as the common denominator** of an integer numerator vector.
  - **Implementations:** modern sparse versions are REF-LU and SPEX (Escobedo, Moreno-Centeno, Lourenco and others).
  - [read: Escobedo dissertation, through CORE]
- **IML** (Chen and Storjohann) [src: `iml.h` from libiml-dev 1.0.5]:
  - **API:** `nonsingSolvMM(…, mpz_t *mp_N, mpz_t mp_D)` and `certSolveMP(…, mp_N, mp_D, …)` return **an integer numerator vector and one denominator**.
  - **RNS:** `nonsingSolvRNSMM` takes its matrix in a residue number system.
  - **Certified solving:** `certSolveMP` finds a *minimal-denominator* solution.
- **Bright and Storjohann**, "Vector rational number reconstruction", ISSAC 2011 [read]:
  - **Problem:** reconstruct a whole solution vector with **one common denominator** d | det A.
  - **Algorithm 4 (LinSolRecon):** keeps a **running denominator**, d := d·RatRecon(d·aᵢ mod M), entry by entry. The surrounding output-sensitive approach is credited to Steffy; the bounds to Cabay 1971; Monagan 2004.
  - This is the incremental version of the paper's "bring a set into common-denominator form".
- **Saunders, Wood and Youse**, "Numeric-symbolic exact rational linear system solver", ISSAC 2011 [read]: keeps **dyadic numerators N over a common power-of-two denominator D**, with D ← D·2^s per iteration. Vector rational reconstruction comes at the end.
- **LinBox `VectorFraction`** and **FLINT `fmpq_mat` matwise/rowwise/colwise:** see prior-art-2 §§6–7.
- **Closeness:** **G yes, everywhere.** The common denominator is a scalar integer, sometimes a power of 2, never factored or partly shared. None of these systems has B, E, S or R.

### B4. CGAL, LEDA and other per-value lazy reduction
- **CGAL** [src: `Number_types/include/CGAL`, current master]:
  - **`Gmpq`, `mpq_class`, `Exact_rational`:** canonical GMP rationals.
  - **`Quotient<NT>`:** `operator+=` computes `num = num*r.den + r.num*den; den *= r.den;` and then calls `simplify_quotient`, which **does nothing** by default ("Currently the default template doesn't do anything"; only the `cpp_int` overload takes a gcd). Reduction happens only in an explicit `normalize()`. This is **lazy reduction per value**, with the cost growth the paper's §1 describes.
  - **`Mpzf` and `Gmpzf`:** dyadic numbers, `data_ * 2^(64*exp)`: a one-element base {2} with an exponent.
  - **`Lazy_exact_nt`:** lazy *evaluation* (an interval filter plus a DAG), a different sense of "lazy".
- **LEDA `rational`** [docs]: "Rational Numbers are not necessarily normalized … Call `normalize()` once in a while." This is the earliest library statement of lazy reduction found.
- **XRationals.jl** (J. Sarnoff, March 2026) [docs: announcement]: fixed-width rationals with "Lazy GCD normalization: deferred until display, hashing, or conversion".
- **hyperreal** (Rust) [docs: README 0.11.1]: "shared-denominator and signed-product-sum hooks used by matrix/vector callers to delay rational canonicalization", and dyadic fast paths.
- **Closeness:** precedent for **lazy reduction (G's second half)** and for dyadic single-prime bases. These are per value, apart from hyperreal's transient shared-denominator dot products. B, E, S and R are absent.

### B5. Polyhedral libraries: common-denominator vectors as the storage
- **Parma Polyhedra Library** [src: `Generator_defs.hh`, PPL 1.2]: a point is stored as **integer coefficients aᵢ and one divisor d > 0**, with coordinates aᵢ/d.
- **isl** [src: `isl_aff.c`]: an affine expression is an `isl_vec` whose **`el[0]` is the common denominator**. `isl_aff_add` scales by d₂/gcd(d₁, d₂) and d₁/gcd, sets the denominator to lcm(d₁, d₂), and **does not reduce afterwards**. Normalisation is a separate step.
- **Closeness:** **G yes, as the permanent storage of a vector**, with lcm-on-add and deferred normalisation. These are the closest match to the paper's §3 "common-denominator representation", in a released library. The denominator is a scalar, so B, E, S and R are absent.

### B6. Decimal columns and shared scale
- **SQL `DECIMAL(p, s)`, ClickHouse, Yellowbrick, Arrow/Gandiva** [docs; src for Gandiva `decimal_type_util.cc`]: a column stores integers with **one scale s for the whole column**, i.e. a common denominator 10^s.
- **Result-scale rules:**
  - add or subtract: **s = max(s₁, s₂)**;
  - multiply: **s = s₁ + s₂**.

  These rules are **exactly the paper's a ∨ b and a + b**, for a one-element base {10}. Rescaling the operand with the smaller scale by 10^(s−sᵢ) is the paper's "multiply by the unshared cofactor".
- **No reduction:** trailing zeros are not stripped (no reduction), so the scale is a type property.
- **Closeness:**
  - **G yes**, per column.
  - **E yes**, for a single fixed base element.
  - **B, S, R no:** the base is fixed at {10} and is never refined. 10 is not split into 2 and 5, so 1/2 and 1/5 cost scale 1 each, and 1/3 is not representable.

  Worth citing in §3 and §4 as the everyday instance of the max/sum rule.

### B7. GPUs and residue number systems
- **XMP 2.0 and CGBN** (NVIDIA; Emmart and Weems) [docs: CGBN README]: fixed-size *unsigned integer* multiprecision (32 bits to 32K bits) for CUDA: add, multiply, Montgomery, gcd, modular inverse. **There is no rational type.** No GPU exact-rational library was found. Research uses CRT and rational reconstruction on GPUs (IEEE Xplore 7550900, metadata only).
- **Residue number systems** [memory]:
  - **History:** Garner 1959 and Szabo and Tanaka 1967.
  - **Representation:** an integer is stored by its residues modulo **pairwise coprime moduli**.
  - **Use for rationals:** multimodular exact solvers recover rationals by reconstruction (Wang 1981; Wang, Guy and Davenport 1982). IML's `nonsingSolvRNSMM` is an instance [src].
  - **Hensel codes:** truncated p-adic encodings of rationals (Krishnamurthy, Rao and Subramanian 1975; Gregory and Krishnamurthy 1984) are a related exact representation.
- **Closeness:** RNS shares the word "pairwise coprime" but is dual to the idea. RNS uses *fixed*, chosen moduli, and a value is a vector of **residues**. The idea uses a *data-driven* base, and a denominator is a vector of **exponents**. None of B (in the paper's sense), E, S, R or G applies. This answers the question "does GPU or RNS work share denominators?": no.

### B8. Factored integers as objects
These were settled in the earlier passes (see prior-art-light §§7–8 and prior-art-2 §§2, 12–15). In summary:

| Object | Coprime? | Addition | Shared across values? |
|---|---|---|---|
| **Magma `RngIntEltFact`** | primes | `+` exists, undocumented | no |
| **Magma `CoprimeBasis`, `PartialFactorization`** (Cremona) | yes | n/a | n/a |
| **PARI `famat`** | not required | none | no |
| **PARI `Z_cba`, `ZV_cba`** | yes | n/a | n/a |
| **Sage `Factorization`** | not required | to the unfactored value | no |
| **arithmoi `Prefactored`** | yes, kept under `*` | drops factors | no |
| **Hecke/Oscar `FacElem`** | after `simplify` | none ("not added") | no |
| **FriCAS `Factored`** | assumed equal-or-coprime | factored (gcd kept, cofactors added, sum fully factored) | no |
| **np-extras `FactoredRational`** | primes, global index | converts and re-factors | implicitly (a static prime index) |

**Closeness:** these supply B and E per value. Only np-extras has a global base, and it is static and fully prime.

### B9. Other analogues worth a sentence in the paper
- **Factor bases in factoring and discrete logs** [memory]:
  - **Precedent:** Morrison and Brillhart 1975, then Dixon, the quadratic sieve and index calculus.
  - **The analogy:** many values are each stored as an **exponent vector over a shared base**, in a **sparse matrix with one row per value and one column per base prime**. In the large-prime variation, base elements are added as data reveal them.
  - **The difference:** the base is prime and fixed in advance, and the vectors serve linear algebra mod 2, not addition of the values.
  - **Why it matters:** this is the precedent for the paper's §5.2 matrix view.
- **Valuations** [memory]: the rule "pᵢ can divide the sum only if aᵢ = bᵢ" is the strict ultrametric property v(x + y) = min(v(x), v(y)) when v(x) ≠ v(y), which is classical (Kürschák 1913; Ostrowski). Brown's A/Bᵐ + C/Bⁿ remark is the same fact for polynomials. The paper extends it to composite coprime base elements, which is valid because each element is coprime to the other elements and to the numerator.
- **Maple** [memory]: rationals are hash-consed DAGs, so *equal* denominators are physically shared, but not their factors.
- **D5 / dynamic evaluation** (Della Dora, Dicrescenzo and Duval 1985) [memory]: a splitting is refined lazily when a computation forces it, and earlier results are never changed. It is a philosophical precedent for R, and Bernstein lists it as a coprime-base application.
- **Bernstein, batch smooth parts and batch gcd:** covered in prior-art-3 §§1d and 3.

---

## Attribution table

| Paper section and idea | Credit | Notes |
|---|---|---|
| §1 the identity a/b + c/d = (ad + cb)/bd and integer growth | classical | none needed |
| §2 lowest terms as canonical form on a computer | **Henrici 1956**, via Knuth TAOCP 2 §4.5.1 | Knuth: "first discussed in the literature by P. Henrici" |
| §2 gcd cost G(n): O(n²) (Euclid) and O(M(n) log n) | Knuth 1970 and Schönhage 1971 (half-gcd); Möller 2008 for the practical version [memory] | cite one survey, e.g. Möller 2008 |
| §2 gcd-first refinement (g₁ = gcd(b, d), then g₂ = gcd(t, g₁)) | **Henrici 1956; Knuth §4.5.1** (algorithm and exercise 6) | used verbatim in CPython, Z3, GMP and FLINT; d₁ = 1 with probability 6/π² (Dirichlet; Knuth Thm 4.5.2D) |
| §3 common-denominator representation of a collection | **Edmonds 1967 / Bareiss 1968** (determinant as common denominator); **Azulay and Pique 1998/2001** (integer simplex); **PPL** generator divisor; **isl_aff**; **IML** (mp_N, mp_D); **LinBox `VectorFraction`**; **FLINT `fmpq_mat`** matwise; SQL/Arrow `DECIMAL` | none of these factors the denominator |
| §3 bringing a set to D = lcm(bᵢ) | folklore; incremental form in **Bright and Storjohann 2011** (running denominator) and SoPlex `reconstructVector` | |
| §3 lcm(1..N) ≈ e^N | Chebyshev; the prime number theorem (ψ(N) ~ N) | |
| §3 lazy reduction | **LEDA `rational`** ("normalize once in a while"); **CGAL `Quotient`**; isl (lcm without reduction); XRationals.jl; Brown 1974 ("gcds only to the extent necessary") | collection-level reduction by gcd(D, m₁, …, m_N) matches PPL and isl normalisation |
| §4.1 coprime base; exponent vectors; lcm = max, gcd = min | **Bach, Driscoll and Shallit 1993** ("gcd-free"/coprime refinement); **Bernstein 2005** (coprime base, natural coprime base, exponents acting through gcd); max/min over primes is classical | for prime bases the identities are textbook; for coprime bases, cite Bernstein |
| §4.2 common denominator read from shared factors; no full gcd | **Brown 1974** (H = gcd(B, D) from explicit factors; A/Bᵐ + C/Bⁿ needs no gcd); FriCAS `Factored` addition; Q-exact (min-exponent scaling) | Brown is the direct precedent |
| §4.2 reduction only where exponents are equal | ultrametric property of valuations (classical); Brown 1974 for polynomials | the extension to composite coprime elements is straightforward and should be stated with its proof (the paper already does) |
| §4.4 multiplication with cross-cancellation | **Henrici 1956 / Knuth** (gcd(u, v′), gcd(u′, v)); Brown 1974; arithmoi `Prefactored` (coprime merge under ×) | |
| §5.1 base elements need not be prime | **Bernstein 2005** ("adequate substitute for factoring into primes"); BDS 1993; arithmoi; **Rich and Stoutemyer 2013** (quasi-primes); FriCAS "nil" flags | |
| §5.1 the term "factor refinement" | **Bach, Driscoll and Shallit 1990/1993** | earlier gcd-free bases: Stieltjes 1890, Collins 1974, Lüneburg 1986 (as listed by Bernstein) |
| §5.2 the bipartite graph / sparse exponent matrix | factor-base relation matrices (Morrison and Brillhart 1975 and successors); PARI `famat` (two-column factor matrix) | applied to denominators of live rationals: not found |
| §5.3 incremental entry: trial division, new elements, refinement on conflict | **Bernstein 2005 Alg. 16.2** (extend a coprime base by one element); arithmoi `Coprimes.insert`; **Rich and Stoutemyer** (staged factoring, then gcd splitting); FLINT `fmpz_factor_smooth`; relp-num | |
| §5.3 propagating a split to every value that uses the element | exponent bookkeeping in BDS refinement [secondary]; **Ge 1993** and **Buchmann and Eisenbrand 1999** (refining a factorization) | across a store of independent values: not found |
| §5 sharing one base among all values | partial: ALTRAN pointer sharing (Brown 1974); np-extras (static global prime index); Bernstein 1996 lazy localization (ideals; unread); Maple hash-consing (whole denominators) | the shared, refined base for rationals: not found |
| §6 common-denominator blocks inside a coprime-base store | **not found** | |

## Novelty verdict

**Verdict: the combination is not known, and no single ingredient is new.** No work found stores rational numbers with denominators as exponent vectors over a coprime base shared by all live values, refines that base incrementally, does addition and multiplication by max/sum on vectors with reduction by tests on equal-exponent elements, and groups identical vectors into common-denominator blocks.

**Confidence: about 75% that no published or released system does this** (prior-art-2 said about 70%). The increase is because the four new areas (SMT, exact LP, exact linear algebra, CGAL, decimals, GPU and RNS) were checked at source level and none shares denominator factors. Every system examined either:
- keeps a canonical per-value mpq (Z3, cvc5, Yices, SoPlex, QSopt_ex, CGAL `Gmpq`); or
- keeps one opaque common denominator per structure (IML, LinBox, FLINT, PPL, isl, Q-matrix simplex, decimal columns).

**Remaining risks:**
1. **Hall 1974** (abstract only), which could hold a global factor table.
2. **Bernstein 1996, lazy localization** (abstract only), a lazily refined coprime representation, but of ideals.
3. **Closed systems:** Maple, Mathematica and Magma internals.
4. **No forward-citation search** of BDS and Bernstein 2005 was possible with these tools.

**Closest single work: Brown 1974** (ALTRAN; with Hall 1974 as its unread companion). It has:
- rational expressions as quotients of factored, pointer-shared factors;
- the common divisor of denominators taken from explicit shared factors rather than computed;
- the observation that A/Bᵐ + C/Bⁿ needs no gcd;
- Henrici's cancellation for products.

**How it differs:**
1. **Domain:** polynomials, not integers.
2. **No coprime invariant:** it explicitly *rejects* keeping factors pairwise coprime, as too costly; the paper makes coprimality the invariant and pays for it with refinement.
3. **No global base:** factors are shared opportunistically by pointer, with no base index and no exponent *vectors* over it, so there are no max/min rules.
4. **Effort levels instead of refinement:** it chooses among identity, divisibility and gcd tests per operation instead of refining a base.
5. **No common-denominator blocks** and no lazy reduction.

**Closest by ingredient, for the related-work section:**
- **Integers, per value:** FriCAS `Fraction Factored Integer` (factored addition; eager factoring; no coprime invariant).
- **The base and its refinement:** BDS 1993 and Bernstein 2005 (algorithms, not a store of values).
- **Entry pipeline:** Rich and Stoutemyer 2013 (quasi-primes).
- **Blocks:** Azulay and Pique's integer Q-matrix simplex, PPL, isl and IML (one opaque denominator).
- **max/sum on a single base element:** SQL `DECIMAL` scale rules.

**What the paper can claim:** the unification. A coprime base shared across values makes Henrici's gcds, Brown's visible shared factors and the common-denominator block into one representation, with the cost proportional to what the denominators do not share. The claim should be made as a composition, credited as in the table above, and judged on measurements.

**Recommended actions before submission:**
1. Obtain Hall 1974 and Bernstein 1996, through a library or ACM/Springer access, and check them for a global factor table.
2. Ask Bernstein, or check the later version of the coprimes page, whether any application stores numbers over a maintained coprime base.

---

## Bibliography

Format: Authors. *Title*. Venue volume(issue):pages, year. DOI or URL. [access tag]

1. **[Azulay1998]** D.-O. Azulay, J.-F. Pique. *Optimized Q-pivot for exact linear solvers*. Principles and Practice of Constraint Programming (CP'98), LNCS 1520, pp. 55ff, 1998. <https://ftp.math.utah.edu/pub/tex/bib/idx/lncs1998b/1520/0/55-z.html> [abstract]
2. **[Azulay2001]** D.-O. Azulay, J.-F. Pique. *A revised simplex method with integer Q-matrices*. ACM Trans. Math. Softw. 27(3), 2001. DOI 10.1145/502800.502804 [abstract]
3. **[BDS1993]** E. Bach, J. Driscoll, J. Shallit. *Factor refinement*. J. Algorithms 15(2):199–222, 1993. DOI 10.1006/jagm.1993.1038. Preliminary versions: Univ. Wisconsin CS TR 883, 1989 (<https://minds.wisconsin.edu/handle/1793/59196>); SODA 1990. [secondary]
4. **[Balaji2022]** N. Balaji, K. Nosan, M. Shirmohammadi, J. Worrell. *Identity testing for radical expressions*. LICS 2022; arXiv:2202.07961. [read, for its BDS statement; authors and venue from memory]
5. **[Bareiss1968]** E. H. Bareiss. *Sylvester's identity and multistep integer-preserving Gaussian elimination*. Math. Comp. 22(103):565–578, 1968. DOI 10.1090/S0025-5718-1968-0226829-0 [memory]
6. **[Bernstein1996]** D. J. Bernstein. *Fast ideal arithmetic via lazy localization*. Algorithmic Number Theory (ANTS-II), LNCS 1122, pp. 27–34, 1996. DOI 10.1007/3-540-61581-4_38 [abstract]
7. **[Bernstein2005]** D. J. Bernstein. *Factoring into coprimes in essentially linear time*. J. Algorithms 54(1):1–30, 2005. <https://cr.yp.to/lineartime/dcba-20040404.pdf> [read]
8. **[Bernstein2004sp]** D. J. Bernstein. *How to find smooth parts of integers*. Draft, 2004. <https://cr.yp.to/papers.html#smoothparts> [read in prior-art-3]
9. **[BrightStorjohann2011]** C. Bright, A. Storjohann. *Vector rational number reconstruction*. ISSAC 2011, pp. 51–58. DOI 10.1145/1993886.1993899. <https://cs.uwaterloo.ca/~cbright/reports/issac47c-bright.pdf> [read; pages and DOI from memory]
10. **[Brown1974]** W. S. Brown. *On computing with factored rational expressions*. ACM SIGSAM Bull. 8(3):26–34, 1974. DOI 10.1145/1086837.1086842. Scan: Bell Labs CSTR 22. [read]
11. **[BuchmannEisenbrand1999]** J. Buchmann, F. Eisenbrand. *On factor refinement in number fields*. Math. Comp. 68(225):345–350, 1999. DOI 10.1090/S0025-5718-99-01023-6 [abstract]
12. **[CGAL]** The CGAL Project. *CGAL User and Reference Manual: Number Types* (`Quotient`, `Gmpq`, `Mpzf`, `Lazy_exact_nt`). <https://github.com/CGAL/cgal/tree/master/Number_types/include/CGAL> [src]
13. **[ChenStorjohann2005]** Z. Chen, A. Storjohann. *A BLAS based C library for exact linear algebra on integer matrices* (IML). ISSAC 2005, pp. 92–99. <https://cs.uwaterloo.ca/~astorjoh/iml.html> [docs + src header]
14. **[D5-1985]** J. Della Dora, C. Dicrescenzo, D. Duval. *About a new method for computing in algebraic number fields*. EUROCAL '85, LNCS 204, pp. 289–290, 1985. [memory]
15. **[DutertreDeMoura2006]** B. Dutertre, L. de Moura. *A fast linear-arithmetic solver for DPLL(T)*. CAV 2006, LNCS 4144, pp. 81–94. DOI 10.1007/11817963_11 [read]
16. **[Edmonds1967]** J. Edmonds. *Systems of distinct representatives and linear algebra*. J. Res. Nat. Bur. Standards 71B(4):241–245, 1967. [memory]
17. **[Eifler2023]** L. Eifler, J. Nicolas-Thouvenin, A. Gleixner. *Combining precision boosting with LP iterative refinement for exact linear optimization*. arXiv:2311.08037, 2023. [read]
18. **[Escobedo]** A. R. Escobedo. *Roundoff-error-free algorithms for exact linear programming and linear systems* (PhD dissertation), Texas A&M University. <https://core.ac.uk/download/79653584.pdf> [read, in part; exact title from memory]
19. **[FriCAS]** FriCAS developers. `Factored` domain, `src/algebra/fr.spad`. <https://github.com/fricas/fricas/blob/master/src/algebra/fr.spad> [src in prior-art-2]
20. **[Gleixner2016]** A. Gleixner, D. E. Steffy, K. Wolter. *Iterative refinement for linear programming*. INFORMS J. Comput. 28(3):449–464, 2016. ZIB-Report 15-15. <https://opus4.kobv.de/opus4-zib/frontdoor/index/index/docId/5511> [abstract]
21. **[Hall1974]** A. D. Hall Jr. *Factored rational expressions in ALTRAN*. ACM SIGSAM Bull. 8(3):35–45, 1974. DOI 10.1145/1086837.1086843 [abstract]
22. **[Henrici1956]** P. Henrici. *A subroutine for computations with rational numbers*. J. ACM 3(1):6–9, 1956. DOI 10.1145/320815.320818 [secondary, via Knuth; DOI from memory]
23. **[isl]** S. Verdoolaege. *isl: an integer set library for the polyhedral model*. ICMS 2010, LNCS 6327, pp. 299–302. Source `isl_aff.c`. [src]
24. **[Knuth-TAOCP2]** D. E. Knuth. *The Art of Computer Programming, Vol. 2: Seminumerical Algorithms*, 3rd ed., §4.5.1 "Fractions". Addison-Wesley, 1997. [read: §4.5.1 source]
25. **[LEDA]** K. Mehlhorn, S. Näher. *LEDA: A Platform for Combinatorial and Geometric Computing*. Cambridge University Press, 1999. Rational numbers guide: <https://leda.uni-trier.de/leda/guide/number_types/rational.html> [docs]
26. **[Moller2008]** N. Möller. *On Schönhage's algorithm and subquadratic integer gcd computation*. Math. Comp. 77(261):589–607, 2008. [memory]
27. **[MorrisonBrillhart1975]** M. A. Morrison, J. Brillhart. *A method of factoring and the factorization of F₇*. Math. Comp. 29(129):183–205, 1975. [memory]
28. **[PPL]** R. Bagnara, P. M. Hill, E. Zaffanella. *The Parma Polyhedra Library: toward a complete set of numerical abstractions for the analysis and verification of hardware and software systems*. Sci. Comput. Program. 72(1–2):3–21, 2008. Source `Generator_defs.hh`. [src]
29. **[RichStoutemyer2013]** A. D. Rich, D. R. Stoutemyer. *Representation, simplification and display of fractional powers of rational numbers in computer algebra*. arXiv:1302.2169, 2013. [read, §2.5]
30. **[SaundersWoodYouse2011]** B. D. Saunders, D. H. Wood, B. S. Youse. *Numeric-symbolic exact rational linear system solver*. ISSAC 2011, pp. 305–312. <https://www.eecis.udel.edu/~youse/itersolve.pdf> [read; pages from memory]
31. **[SoPlex]** Zuse Institute Berlin. *SoPlex: the Sequential object-oriented simPlex*, exact mode; files `rational.h`, `ratrecon.hpp`. <https://github.com/scipopt/soplex> [src]
32. **[SQLDecimal]** Decimal result-scale rules: Apache Arrow Gandiva `decimal_type_util.cc` (<https://apache.googlesource.com/arrow/+/refs/tags/apache-arrow-0.14.1/cpp/src/gandiva/decimal_type_util.cc>); ClickHouse Decimal docs (<https://clickhouse.com/docs/reference/data-types/decimal>). [src / docs]
33. **[SzaboTanaka1967]** N. S. Szabo, R. I. Tanaka. *Residue Arithmetic and Its Applications to Computer Technology*. McGraw-Hill, 1967. [memory]
34. **[vzGathen1986]** J. von zur Gathen. *Representations and parallel computations for rational functions*. SIAM J. Comput. 15(2):432–452, 1986. DOI 10.1137/0215030 [metadata only]
35. **[Wang1981]** P. S. Wang. *A p-adic algorithm for univariate partial fractions*. SYMSAC 1981, pp. 212–217; with M. J. T. Guy and J. H. Davenport, *p-adic reconstruction of rational numbers*, ACM SIGSAM Bull. 16(2):2–3, 1982. [memory]
36. **[XMP/CGBN]** N. Emmart, C. Weems et al. (NVIDIA Research). *CGBN: CUDA accelerated multiple precision arithmetic using cooperative groups*, and XMP 2.0. <https://github.com/NVlabs/CGBN> [docs]
37. **[Yices]** B. Dutertre. *Yices 2.2*. CAV 2014, LNCS 8559, pp. 737–744. Source `src/terms/rationals.h`. <https://github.com/SRI-CSL/yices2> [src]
38. **[Z3]** L. de Moura, N. Bjørner. *Z3: an efficient SMT solver*. TACAS 2008, LNCS 4963, pp. 337–340. Source `src/util/mpq.cpp`. <https://github.com/Z3Prover/z3> [src]
39. **[cvc5]** H. Barbosa et al. *cvc5: a versatile and industrial-strength SMT solver*. TACAS 2022, LNCS 13243, pp. 415–442. Source `src/util/rational_gmp_imp.h`, `src/theory/arith/linear/normal_form.h`. <https://github.com/cvc5/cvc5> [src]
40. **[QSoptEx]** D. L. Applegate, W. Cook, S. Dash, D. G. Espinoza. *Exact solutions to linear programming problems*. Oper. Res. Lett. 35(6):693–699, 2007. <https://www.math.uwaterloo.ca/~bico/qsopt/ex/> [docs + memory]
41. **[CPython-fractions]** Python Software Foundation. `fractions` module, `Fraction._add` (cites Knuth §4.5.1). CPython 3.13. [src]

Items carried over from earlier passes, with full entries in prior-art-2: arithmoi `Prefactored` (Lelechenko 2017), np-extras `FactoredRational`, Q-exact (Trotta 2018), PARI `famat`/`Z_cba`, Magma `CoprimeBasis`/`PartialFactorization`, Sage `Factorization`, Hecke `FacElem`, FLINT `fmpz_factor_refine`/`fmpq_mat`, LinBox `VectorFraction`, relp-num.
