# Gaussian integer gcd and extended gcd: literature survey

Scope: gcd and extended gcd (Bezout coefficients, modular inverses) in Z[i], with an eye to a one-word representation (two int32 halves in a 64-bit lane), integer-only, branch-light, vectorisable on x86-64, ARM64 and RISC-V. Web survey only, November 2026. Items marked "not verified online" are cited from standard bibliographies but their publisher pages were blocked by our proxy.

## 1. Algorithms for gcd in Z[i]

### 1.1 Euclidean variants

Z[i] is norm-Euclidean: rounding the exact quotient a/b to the nearest Gaussian integer gives a remainder r with N(r) <= N(b)/2, so the norm at least halves per step and the step count is at most about log2 N(b). Ties need a fixed rule; GAP rounds to the nearest lattice point and breaks ties towards the origin, and notes that always rounding towards the origin (truncation) breaks the algorithm ([GAP manual, Gaussian integers](https://www.math.rwth-aachen.de/~GAP/WWW2/Gap3/Manual3/C014S007.htm)).

- Caviness, B. F. and Collins, G. E., "Algorithms for Gaussian integer arithmetic", Proc. SYMSAC 1976, pp. 36-45. The first systematic treatment; nearest-integer Euclid. Not verified online.
- Rolletschek, H., "On the number of divisions of the Euclidean algorithm applied to Gaussian integers", J. Symbolic Computation 2 (1986) 261-291. Worst-case step counts, least-remainder choice. Not verified online.
- Knopfmacher, A. and Knopfmacher, J., "The number of steps in the Euclidean algorithm over complex quadratic fields", BIT 31 (1991), [doi:10.1007/BF01931288](https://doi.org/10.1007/BF01931288). Average step counts.
- Collins, G. E., "A fast Euclidean algorithm for Gaussian integers", J. Symbolic Computation 33(4) (2002) 385-392 (volume and pages not verified online). Computes quotients from the leading bits only (a Lehmer-like approximate quotient), giving O(n^2) bit complexity instead of the O(n M(n)) of exact norm division. FLINT's `fmpzi_gcd_euclidean_improved` follows this idea.
- Bach, E. and Sandlund, B., "On Euclidean methods for cubic and quartic Jacobi symbols", ANTS-XIII poster, 2018, [arXiv:1807.07719](https://arxiv.org/abs/1807.07719). Shows exact-quotient Gaussian Euclid costs Omega(n M(n)) in the worst case, and approximate quotients (Collins) give O(n^2), which is optimal for these methods. Also shows the "even quotient" variants (Eisenstein, Smith) are exponential in the worst case, so they are not worth studying.
- Graves, H., "A division algorithm for the Gaussian integers' minimal Euclidean function", Canadian Math. Bulletin 2025, [arXiv:2502.21136](https://arxiv.org/abs/2502.21136). Division relative to the minimal Euclidean function rather than the norm; theoretical, no gcd speed claims.

Relevance to us: our rounded-division Euclid (14 steps mean) is the classic algorithm. The only known practical improvement at small sizes is Collins's: never compute an exact quotient, derive it from a few leading bits (or from double-precision floats, which is what FLINT does below 2^50, see section 5).

### 1.2 Binary, (1+i)-ary and l-ary algorithms

The prime 1+i has norm 2 and plays the role of 2. z = p + qi is divisible by (1+i) iff p + q is even, and z/(1+i) = ((p+q) + (q-p)i)/2. Since (1+i)^2 = 2i, removing two factors is a shift plus a multiplication by -i (swap and negate). Every z coprime to 1+i has exactly one associate (unit multiple) that is congruent to 1 mod (1+i)^3 = 2+2i ("primary"); the units {1, i, -1, -i} represent (Z[i]/(1+i)^3)^*. So if a and b are both made primary, a - b is divisible by (1+i)^3 and each subtraction removes at least three factors of 1+i (norm drops by 8).

- Weilert, A., "(1+i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Computation 30(5) (2000) 605-617. The algorithm we modelled. Quadratic bit complexity. Weilert also uses it for the quartic residue symbol. Not reachable through our proxy.
- Damgard, I. and Frandsen, G. S., "Efficient algorithms for gcd and cubic residuosity in the ring of Eisenstein integers", FCT 2003, LNCS 2751, pp. 109-117; revised in J. Symbolic Computation 39(6) (2005) 643-652; tech report [BRICS RS-03-8](https://www.brics.dk/RS/03/8/). Binary-style gcd with 1 - zeta (norm 3), primary normalisation 1 + 3 alpha, "approximate norm" from the six leading bits to decide which operand to replace. Section 5 states the same technique gives an O(n^2) gcd in Z[i] using N((1+i)^3) = 8. No extended gcd.
- Agarwal, S. and Frandsen, G. S., "Binary GCD like algorithms for some complex quadratic rings", ANTS-VI 2004, LNCS 3076, pp. 57-71, [doi:10.1007/978-3-540-24847-7_4](https://doi.org/10.1007/978-3-540-24847-7_4) ([dblp](https://dblp.org/rec/conf/ants/AgarwalF04)). Extends the binary approach to the other imaginary quadratic Euclidean rings Z[sqrt(-2)], Z[(1+sqrt(-7))/2] and so on.
- Agarwal, S. and Frandsen, G. S., "A new GCD algorithm for quadratic number rings with unique factorization", LATIN 2006, LNCS 3887, pp. 30-42, [doi:10.1007/11682462_8](https://doi.org/10.1007/11682462_8) ([bib](https://www.latintcs.org/bib/l06-p30-42.txt)). Quadratic-time gcd also for non-Euclidean UFD quadratic rings.
- Wikstrom, D., "On the l-ary GCD-algorithm in rings of integers", ICALP 2005, LNCS 3580, pp. 1189-1201 ([PDF](https://www.csc.kth.se/~dog/research/papers/Wik05aConf.pdf), [tech report](https://www.csc.kth.se/~dog/research/papers/Wik04TR.pdf)). Generalises Sorenson's l-ary gcd (J. Algorithms 16(1) (1994) 110-144) to any class-number-one ring of integers. Theoretical, brute-force subroutines, gcd only.
- Kaltofen, E. and Rolletschek, H., "Computing greatest common divisors and factorizations in quadratic number fields", Math. Comp. 53(188) (1989) 697-720 ([AMS](https://www.ams.org/journals/mcom/1989-53-188/S0025-5718-1989-0982367-2/home.html), [PDF](https://kaltofen.math.ncsu.edu/bibliography/89/KaRo89.pdf)). Lattice (short vector) gcd for imaginary quadratic class-number-one rings. FLINT's `fmpzi_gcd_shortest` is in this spirit.

None of these papers treats the extended gcd explicitly, as far as the abstracts and the BRICS and KTH texts show.

### 1.3 Lehmer, half-gcd and asymptotically fast

- Weilert, A., "Asymptotically fast GCD computation in Z[i]", ANTS-IV 2000, LNCS 1838, pp. 595-613. A Schonhage-style half-gcd for Z[i], O(M(n) log n). Irrelevant at one word but it is the reference for the multi-word tier.
- Weilert, A., "Fast computation of the biquadratic residue symbol", J. Number Theory 96 (2002) 133-151. Applies the fast gcd to quartic residuosity (cited by PARI's maintainers, [pari-users 2006](https://pari.math.u-bordeaux.fr/archives/pari-users-0612/msg00002.html)).
- For integers, the references are Lehmer (Amer. Math. Monthly 45 (1938) 227-233), Moller, "On Schonhage's algorithm and subquadratic integer gcd computation", Math. Comp. 77(261) (2008) 589-607 ([AMS](https://www.ams.org/mcom/2008-77-261/S0025-5718-07-02017-0/)), Stehle and Zimmermann, "A binary recursive gcd algorithm", ANTS-VI 2004, LNCS 3076, pp. 411-425 ([page](https://perso.ens-lyon.fr/damien.stehle/BINARY.html)), and Lichtblau's half-gcd and planar lattice paper ([PDF](https://library.wolfram.com/infocenter/Conferences/7534/HGCD_and_planar_lattices.pdf)). Brent and Zimmermann's Jacobi symbol paper ([arXiv:1004.2091](https://arxiv.org/abs/1004.2091)) cites Weilert for the Z[i] analogue.

Relevance to us: a Lehmer layer only pays when operands span several words. It becomes relevant to our promoted (two int64 halves and larger) tiers, where the natural design is: run the one-word kernel on the leading bits, collect a 2x2 transition matrix with Gaussian entries, apply it to the full operands.

### 1.4 Stein and Brent-Kung style

Stein's binary gcd (J. Comput. Phys. 1(3) (1967) 397-405) and Brent and Kung's systolic variant (ARITH-7, 1985, pp. 118-125) replace magnitude comparison by a small counter that tracks the difference in bit lengths, so each step looks only at low bits. For Z[i] the published (1+i)-ary algorithms still compare norms (exactly or approximately); we found no Brent-Kung-style Gaussian variant. The Bernstein-Yang divstep (section 3) is the modern form of the Brent-Kung idea and is the obvious thing to port.

## 2. Extended (1+i)-ary gcd without division

The integer binary extended gcd keeps invariants a = u A + v B and, when it halves a, either halves (u, v) or first adds (B, -A) to make them even (Knuth, TAOCP vol. 2, 4.5.2). For a modular inverse mod odd n only one coefficient is needed: keep a = u x (mod n), and halve u as "if u odd then u + n; shift right".

The Z[i] analogue is exact, because Z[i]/(1+i) is the field with two elements, just as Z/2 is.

Inverse of x modulo nu, with (1+i) not dividing nu:

1. State (a, u) and (b, v) with a = u x and b = v x (mod nu). Start (x, 1) and (nu, 0).
2. While (1+i) divides a: a <- a/(1+i); u <- (u + m nu)/(1+i), where m = 1 if p + q is odd for u = p + qi, else 0. Since nu is not divisible by 1+i, u + nu is divisible whenever u is not. Branch-free: m is the low bit of p + q; the add is masked.
3. Multiply a by the unit epsilon that makes it primary; multiply u by the same epsilon (a swap and negate, done with selects).
4. If N(a) < N(b), swap (a, u) with (b, v). Set a <- a - b, u <- u - v. Now (1+i)^3 divides a.
5. Stop when a = 0. Then b is a unit (if x is invertible) and x^{-1} = v b^{-1} = v conj(b) mod nu.

Cost per removed factor of 1+i, on the coefficient alone, is about eight lane operations: parity (add, and), masked add of nu (two ands, two adds), rotate (add, sub), two arithmetic shifts. Removing (1+i)^2 at once is cheaper since dividing by 2i is a shift and a swap-negate, and the conditional add is then of 0, nu, 2 nu or 3 nu (by residue mod 2, precomputable). Total cost: the step count of the plain (1+i)-ary gcd, plus roughly the same work again for u, plus the coefficient reductions mod nu after u - v (a masked add of nu if the result leaves the range). Coefficients stay bounded by nu in each half (with the usual sign-range trick) so they fit the same int32 halves when nu does.

For the full Bezout pair (no modulus), keep both (u, v) and add (B, -A) when they are not divisible; this needs A or B coprime to 1+i, so first strip the common (1+i)^k from both inputs. Coefficients are then bounded by the inputs' sizes.

Two refinements from the integer literature apply directly.

- Delayed correction (Bernstein-Yang, section 3). If each iteration divides by exactly one 1+i, never adjust u: track the unreduced transition and multiply by the precomputed constant (1+i)^{-K} mod nu at the end, with K the fixed iteration count. One constant per modulus.
- Batched transitions (Pornin, Bernstein-Yang). Run k iterations on the low bits only, accumulating a 2x2 matrix of Gaussian integers whose entries grow by at most a factor of about sqrt(2) per step, then apply it to (a, b) and to (u, v) in one go. At one word this saves little on (a, b) but removes per-step work on (u, v): with int64 accumulators, around 25-30 steps can be batched before overflow.

## 3. Constant-time and branch-free gcd for integers

- Bernstein, D. J. and Yang, B.-Y., "Fast constant-time gcd computation and modular inversion", TCHES 2019(3) 340-398, [doi:10.13154/tches.v2019.i3.340-398](https://doi.org/10.13154/tches.v2019.i3.340-398), [ePrint 2019/266](https://eprint.iacr.org/2019/266), [site](https://gcd.cr.yp.to/papers.html). The divstep: with f odd, if delta > 0 and g odd then (delta, f, g) <- (1 - delta, g, (g - f)/2), else (1 + delta, f, (g + (g mod 2) f)/2). There are no comparisons, only one parity bit and the sign of delta. A fixed iteration count guarantees g = 0. Batches of 62 (or 30) divsteps are done on low bits, giving a 2x2 matrix that is then applied to the full numbers; the inverse comes out times 2^{-K} and is fixed by one multiplication by a constant.
- Bernstein, Chen, Harrison, Maxwell, Wang, Wuille and Yang, "More on fast constant-time gcd computation and modular inversion", slides, Academia Sinica, December 2024 ([PDF](https://troll.iis.sinica.edu.tw/ws2024/safegcd2.pdf)). Starting delta at 1/2 ("hddivsteps") cuts the bound to about 2.3 n iterations for n-bit inputs (588 for 255 bits, against 744 originally). Curve25519 inversion in 3,880 Skylake cycles. Bounds tooling: [sipa/safegcd-bounds](https://github.com/sipa/safegcd-bounds). Gaussian integers and number fields are not mentioned.
- libsecp256k1's write-up, "The safegcd implementation in libsecp256k1 explained" ([copy](https://sources.debian.org/src/libsecp256k1/0.7.0-1/doc/safegcd_implementation.md)), is the clearest engineering description, including the variable-time variant. O'Connor and Poelstra formally verified it in Coq ([arXiv:2507.17956](https://arxiv.org/abs/2507.17956), 2025).
- Pornin, T., "Optimized binary GCD for modular inversion", [ePrint 2020/972](https://eprint.iacr.org/2020/972). Classic binary extended gcd made constant-time. It works on approximations of a and b built from their top and bottom 31 bits, packed in one 64-bit word, does 31 iterations there accumulating update factors, then applies them. 6,253 Coffee Lake cycles for inversion mod 2^255 - 19. Used in RustCrypto crypto-bigint (`bingcd`).
- Sreedhar, K., Horowitz, M. and Torng, C., "A fast large-integer extended GCD algorithm and hardware design for verifiable delay functions and modular inversion", TCHES 2022(4), [ePrint 2021/1292](https://eprint.iacr.org/2021/1292). Finds that Stein-style subtraction algorithms beat division-based ones in hardware. This matches our observation that removing the divides is what counts once the substrate is wide and simple.

Has anyone adapted these to Z[i]? We found nothing: no divstep, safegcd or Pornin-style algorithm for Gaussian or other quadratic integers. The port looks natural because the residue field at 1+i is F_2, exactly as at 2 for Z, so "g odd" and "(g - f)/(1+i)" make sense unchanged, and the unit choice (section 1.2) offers extra freedom. The open part is the termination bound. Bernstein-Yang's proof rests on a 2-adic analysis and a large computation; the ramified (1+i)-adic case needs its own proof or at least an empirical worst-case bound, which at 32-bit halves is cheap to measure exhaustively over structured inputs and by random sampling.

## 4. Alternatives to the gcd for inverses mod n

The key fact is that Gaussian modular inversion reduces to integer modular inversion in the common cases.

| Modulus | Reduction | Integer work |
|---|---|---|
| rational integer n | x^{-1} = conj(x) * N(x)^{-1} mod n (x is a unit mod n iff N(x) is) | one integer inverse mod n, then 2 mulmods |
| primitive nu = c + di (gcd(c, d) = 1) | Z[i]/(nu) is isomorphic to Z/N(nu), with i mapped to t = -c d^{-1} mod N(nu); x = p + qi maps to p + q t | one integer inverse mod N(nu); t costs one inverse per modulus |
| Gaussian prime pi, p = pi conj(pi) | special case of the line above, field F_p | as above, or Fermat |
| inert prime p = 3 mod 4 | Z[i]/(p) = F_{p^2}; use the first line | one inverse mod p |
| general nu = g nu', g rational | split by CRT when gcd(g, N(nu')) = 1, else fall back to the Gaussian xgcd | mixed |

Caution: the norm trick is only valid for a rational modulus. For nu = 2 + i and x = 2 - i, N(x) = 5 is not invertible mod N(nu) = 5, yet x = -1 mod nu. Use the isomorphism for Gaussian moduli.

With int32 halves, N(nu) is below 2^63, so the integer inverse is a 64-bit problem and the map needs 64x64 to 128-bit products. Vector support for the high half differs: RVV has vmulh/vmulhu, SVE has SMULH/UMULH, while NEON and AVX2 have only 32x32 to 64 (umull, vpmuludq) and AVX-512 has 52-bit IFMA. So 64-bit mulmod is the expensive primitive on two of the three families.

Fermat: for prime modulus p, x^{-1} = x^{p-2}, about 1.2 log2 p mulmods with a window (about 75 for a 63-bit p). It is constant-time and trivially vectorised, but at 10 to 30 instructions per 64-bit mulmod it is in the same 1,000 to 2,000 instruction range as our current gcds. It wins only when a fast vector mulmod already exists, primality is known, and lockstep matters more than speed.

Batch inversion (Montgomery, "Speeding the Pollard and elliptic curve methods of factorization", Math. Comp. 48(177) (1987) 243-264): k inverses for one inversion plus 3(k - 1) multiplications, all mod the same modulus. In Z[i] mod nu each multiplication is about three or four integer mulmods (or one in the Z/N(nu) image). It wins whenever an array shares one modulus and k is more than a handful. It is useless across lanes with different moduli, and one non-invertible element poisons the whole batch (detect with a gcd on the product, then bisect). Visegrady, "Low latency data-flow graphs for simultaneous modular inversion of many inputs" ([arXiv:2607.11337](https://arxiv.org/abs/2607.11337), July 2026) reorganises the trick into a log-depth tree: 16 inverses cost 1.34 times the latency of one in libsecp256k1.

Rule of thumb: same modulus and many elements, use batch inversion; rational or primitive modulus, map to Z/N and use an integer inverse; Gaussian xgcd only for general nu or when the actual Bezout pair is wanted (the second coefficient (1 - x y)/nu can be recovered from y by one exact division if needed).

## 5. Vector and batched implementations, and what libraries do

Vector gcd. We found no published SIMD Gaussian gcd, and few SIMD integer gcds; SIMD modular arithmetic papers (for example Fortin, Fleury, Lemaire and Monagan, 2020, [PDF](https://cecm-www.dcr.sfu.ca/personal/monaganm/papers/PierreCCPE20.pdf); Didier et al., IACR CiC 1(3) 2024, [PDF](https://cic.iacr.org/p/1/3/11/pdf)) vectorise multiplication and exponentiation and leave inversion scalar or to precomputation. The constant-time literature is the de facto vector literature, since a fixed iteration count with masked selects is exactly what SIMD lanes need: in a vector the cost is the worst case over the lanes, not the mean. Bernstein and Yang's own software interleaves scalar and vector work. Variable-length loops with count-trailing-zeros are awkward in vectors: AVX-512 has lzcnt (CD) but no tzcnt, NEON has CLZ only up to 32-bit lanes, SVE has CLZ and RBIT on all sizes, and RVV has vctz in Zvbb. Batch gcd in the Bernstein and Heninger sense (product and remainder trees over many moduli) solves a different problem.

Libraries.

| Library | Gaussian gcd | Gaussian xgcd | Notes |
|---|---|---|---|
| FLINT `fmpzi` ([docs](https://flintlib.org/doc/fmpzi/index.html), [source](https://docs.rs/crate/flint-sys/latest/source/flint-3.5.0/src/fmpzi/gcd.c)) | yes | no | Dispatcher: components below 2^50 go to a double-precision kernel `_fmpzi_gcd_dddd`; single-word or under 30,000 bits use `euclidean_improved` (approximate quotients, double-precision finish); larger use the lattice `shortest` method. `fmpzi_gcd_binary` (Weilert) exists but falls back to Euclid above 500 bits and is not the default. |
| PARI/GP | `gcd` accepts Gaussian integers | no dedicated one found | nf-level tools (`nfdiveuc`, ideal functions) cover Bezout-type needs; Gaussian Euclidean division convention discussed on [pari-users, Nov 2025](https://pari.math.u-bordeaux.fr/archives/pari-users-2511/msg00051.html). |
| SageMath | via number field order or PARI | not built in as of the [2019 Q&A](https://ask.sagemath.org/question/45441/compute-xgcd-over-gaussian-integers/) | Users hand-roll Euclid. |
| GAP | generic Euclidean ring functions | generic Gcdex | Nearest rounding, ties towards the origin. |
| Magma | not checked | not checked | Documentation not reached. |
| GMP `mpz_gcdext` ([manual](https://gmplib.org/manual/Extended-GCD)) | integer reference | yes | Lehmer-style with one- and two-limb hgcd2 below a threshold, a loop around subquadratic HGCD above, extra bookkeeping for cofactors. `mpz_invert` builds on it. |

The practical lesson from FLINT is that, for one-word Gaussian gcd, its authors chose Euclid with floating-point quotient estimates over the binary algorithm. That fits our numbers for a scalar machine with fast divide and fast double. It does not settle the question for integer-only vector lanes, where divides are slow or missing (NEON, SVE and AVX2 have no integer vector divide; RVV has vdiv but it is slow) and branches cost lockstep.

## 6. Finding pi with p = a^2 + b^2

For a prime p = 1 mod 4:

1. Find t with t^2 = -1 mod p, for example t = c^{(p-1)/4} mod p for a quadratic non-residue c (try small c; by quadratic reciprocity c = 2 works when p = 5 mod 8).
2. Run integer Euclid on (p, t) and stop at the first remainder r_k < sqrt(p); then p = r_k^2 + r_{k+1}^2 (with r_{k+1} the next remainder, or p - r_k^2 checked as a square).

This is Brillhart's shortening of Hermite's 1848 continued-fraction method: Brillhart, J., "Note on representing a prime as a sum of two squares", Math. Comp. 26(120) (1972) 1011-1013 ([AMS](https://www.ams.org/journals/mcom/1972-26-120/S0025-5718-1972-0314745-6/)); Hermite, C., "Note au sujet de l'article precedent", J. Math. Pures Appl. 13 (1848) 15; Serret, J.-A., "Sur un theoreme relatif aux nombres entiers", ibid. 12-14. Williams, K. S., "Some refinements of an algorithm of Brillhart", CMS Conf. Proc. 15 (1995) ([PDF](https://people.math.carleton.ca/~williams/papers/pdf/202.pdf)) gives refinements. Cornacchia's algorithm (Giornale di Matematiche di Battaglini 46 (1908) 33-90; Cohen, A Course in Computational Algebraic Number Theory, GTM 138, Algorithm 1.5.2) is the same truncated Euclid for x^2 + d y^2 = m.

Equivalently pi = gcd_Z[i](p, t + i), and the half-way stop is just the integer Euclid run on the real projection, so the Gaussian gcd is not needed: Brillhart's version is integer-only and takes about half the steps of a full Euclid on (p, t). With 32-bit halves, p < 2^62 is in range and the integer Euclid is on 63-bit numbers. The cost is dominated by step 1 (one modular exponentiation), as for the Fermat inverse. The same t is the i-image needed by the isomorphism in section 4, so finding pi and setting up inversion mod pi share work.

## 7. Candidates for our next study

1. Map to Z/N and invert there with integer hddivsteps. For a rational or primitive modulus (the cases a mod-pi or mod-n tool actually meets), x^{-1} mod nu becomes one 64-bit integer inverse plus two or three mulmods. Use the Bernstein-Yang delta = 1/2 divstep with a fixed iteration count of about 2.3 times 64 (about 150), batched 30 or 62 at a time into a 2x2 matrix. Reasons: proven bounds, perfectly lockstep across lanes, no divides, no norm comparisons, and the integer code is reusable elsewhere. Risk: the 64x64 to 128 mulmod on NEON and AVX2, so measure the map and correction cost, not just the loop.

2. Extended (1+i)-ary gcd with modular coefficient tracking, branch-free. The section 2 algorithm with primary-unit normalisation (three factors of 1+i per subtraction), the masked "add nu then divide by 1+i" coefficient step, an approximate norm comparison from leading bits (Damgard-Frandsen), and a fixed iteration count equal to the measured worst case at 32-bit halves. Reasons: stays in Z[i] and int32 halves, handles any odd nu and the full Bezout pair, and builds directly on our 1,270 to 1,370 instruction toy. The coefficient work should be cut by delaying it into batched 2x2 Gaussian matrices.

3. A Gaussian divstep (new, unpublished as far as we can find). Replace the norm comparison by a delta counter: with f coprime to 1+i, if delta > 0 and (1+i) does not divide g, then (delta, f, g) <- (1 - delta, g, (g - eps f)/(1+i)), else (1 + delta, f, (g + [g odd] f)/(1+i)), with eps a unit chosen from the residue mod (1+i)^3. One fixed shift and rotate per step, one constant (1+i)^{-K} mod nu at the end. Reasons: removes the most expensive part of candidate 2 (norms) and gives Bernstein-Yang's SIMD shape natively in Z[i]. Risk: no termination proof, so the first study must measure the worst-case step count empirically before any cost claims.
