# Cryptography and number theory over Z[i]: who would use an exact Gaussian-integer library

Survey date: 2026-11-03. Light, web-only survey. Each claim cites a source we read. Items marked "(not verified)" are cited from memory or from a secondary mention, and should be checked before anyone relies on them.

## 1. The research community and the schemes

### 1.1 Three tiers

The literature splits into three groups that differ sharply in quality.

Tier A is serious work: refereed journals in cryptography, coding or computational number theory, by groups that publish on the topic regularly.

- Hardware and coding over Gaussian integers, HTWG Konstanz (Jürgen Freudenberger, Malek Safieh, Johann-Philipp Thiers). This is the most active applied group. M. Safieh and J. Freudenberger, "Montgomery Reduction for Gaussian Integers", Cryptography 5(1):6, 2021, https://doi.org/10.3390/cryptography5010006 ([MDPI](https://www.mdpi.com/2410-387X/5/1/6)). It adapts Montgomery reduction to Z[i]/(π) for elliptic-curve point multiplication over GF(p) with p = a² + b², using the randomized initial point countermeasure, with FPGA results. The authors say the method is faster than comparable prime-field designs. The same group's repository lists "Side Channel Attack Resistance of the Elliptic Curve Point Multiplication using Gaussian Integers" (2020) and an Eisenstein counterpart ([HTWG OPUS list](https://opus.htwg-konstanz.de/solrsearch/index/search/searchtype/authorsearch/author/Malek+Safieh/rows/50)). On codes for McEliece: J. Freudenberger and J.-P. Thiers, "A New Class of Q-Ary Codes for the McEliece Cryptosystem", Cryptography 5(1):11, 2021 ([MDPI](https://mdpi.com/2410-387X/5/1/11/xml)), and Thiers and Freudenberger, "Generalized concatenated codes over Gaussian and Eisenstein integers for code-based cryptography", Cryptography 5(4):33, 2021 ([HTWG PDF](https://opus.htwg-konstanz.de/files/2975/cryptography-05-00033-v2.pdf)). Bibliographic details for these two codes papers come from search listings, because the pages were behind bot checks (not verified in full). The codes lineage begins with K. Huber, "Codes over Gaussian integers", IEEE Trans. Inf. Theory 40(1):207–216, 1994 ([PDF via ResearchGate](https://www.researchgate.net/profile/Klaus_Huber/publication/220036065_Codes_over_Gaussian_Integers/links/0d1c84f564dae5d496000000/Codes-over-Gaussian-Integers.pdf)).
- Cryptanalysis of RSA analogues (Nitaj, Susilo, Teşeleanu and coauthors; details in 1.3).
- Rabin over number fields: A. Cobbe, A. Nickel, A. Schuster, "The Rabin cryptosystem over number fields", arXiv:2506.09569, 2025 ([arXiv](https://arxiv.org/html/2506.09569v1)). Universität der Bundeswehr München. Z[i] is the main worked case and the benchmark. The implementation is in PARI/GP and published on GitHub.
- Protocols: D. Megías, "Complex domain approach for reversible data hiding and homomorphic encryption", J. Inf. Security and Applications 102:104596, 2026 ([arXiv:2510.03770](https://arxiv.org/pdf/2510.03770)). It defines ElGamal over Z[i] modulo a Gaussian prime ("H[i]dden-EG") with 2048-bit default moduli, 3072 and 4096 for longer-term use. Timings are in Python on an AMD EPYC server, and there is an ESP32 emulation.
- Lattices, lightly: NTRU over the Eisenstein integers (K. Jarvis, M. Nevins, "ETRU: NTRU over the Eisenstein integers", Des. Codes Cryptogr., 2015, doi:10.1007/s10623-013-9850-3, [Unpaywall](https://unpaywall.org/10.1007%2FS10623-013-9850-3); thesis at [uOttawa](https://ruor.uottawa.ca/handle/10393/19862)). We found no comparably serious NTRU or LWE construction over Z[i] itself. Z[i] is just the n = 2 case of the power-of-two cyclotomic rings that lattice crypto already uses.

Tier B is academic but low-traffic: theses and smaller proceedings, mostly sound but not novel.

- S. Fazekas, "Public Key Cryptography over the Gaussian and Eisenstein Integers", MSc thesis, Maynooth, 2023 ([MURAL](https://mural.maynoothuniversity.ie/id/eprint/18135/)). It covers RSA, Diffie–Hellman and ElGamal analogues.
- X. Dong, "Digital Signature Schemes over the Ring of Gaussian Integers", ICEMC 2016, Atlantis Press, pp. 828–832, doi:10.2991/icemc-16.2016.167 ([Atlantis](https://atlantis-press.com/proceedings/icemc-16/25856745)). ElGamal signatures with appendix and with message recovery.
- H. Elkamchouchi, K. Elshenawy, H. Shaban, "Two new public key techniques in the domain of Gaussian integers", NRSC 2003, doi:10.1109/NRSC.2003.157337 ([record](https://bvbvf1.bib-bvb.de/vufind/PrimoRecord/cdi_ieee_primary_1217350)). Together with their 2002 APCC paper, this is the origin of "Gaussian RSA", the scheme most of the later work attacks or varies.

Tier C is marginal. These are claims of "more security" with no reduction, toy parameters, and mostly low-tier or regional venues. They are numerous, and their authors are plausible users of a convenient tool, but they are not a research frontier.

- Y. Awad, D. Jomaa, Y. Alkhezi, R. Hindi, "A new approach combining RSA and ElGamal ... using Gaussian integers", Jordanian J. Computers and IT 11(1):54–72, 2025, doi:10.5455/jjcit.71-1720618955 ([DOAJ](https://doaj.org/article/726d5dc9557a42eeab514f9a4332dd1e)).
- Y. Awad, H. Chehade, R. Hindi, "Enhancing the efficiency and security of the Rabin public-key cryptosystem using Gaussian integers", EPSTEM 34:107–119, 2025 ([DergiPark](https://dergipark.org.tr/en/pub/epstem/article/1753690)). The Awad and El-Kassar group (Lebanon) has published Gaussian-integer variants for about two decades.
- H. A. Khalaf, R. Z. Khalaf, "Hybrid cryptosystem using Polybius and ElGamal algorithm over the Gaussian integers", Academic Science Journal 2(1):135–147, 2024 ([UoDiyala](https://acadscij.uodiyala.edu.iq/index.php/Home/article/view/344)).
- S. Pradhan, B. K. Sharma, "A modified variant of RSA algorithm for Gaussian integers", IJINS 2(4), 2013 ([IAES](https://ijins.iaescore.com/index.php/IJINS/article/view/20264)).
- "SPN based RGB image encryption over Gaussian integers", Heliyon 10(9):e30353, 2024 ([PubMed](https://www.pubmed.gov/38737253/)), and a 2025 sequel on S-boxes over residue classes of Z[i] in Cluster Computing (doi:10.1007/s10586-025-05867-z, [Crossref](https://api.crossref.org/works/10.1007%2FS10586-025-05867-Z)). These need only tiny moduli.
- S. Kavitha et al., "Gaussian graceful labeling and NTRU", EAI Endorsed Trans. IoT 10(1), 2024 ([EUDL](https://eudl.eu/doi/10.4108/eetiot.4992)). The Gaussian content is nominal.

### 1.2 Schemes studied

| Scheme family | Where | Underlying hard problem |
|---|---|---|
| RSA analogue, N = pq, Z[i]/(N) with group order (p²−1)(q²−1) | Elkamchouchi 2002/2003; Awad 2025; Pradhan 2013; Fazekas 2023 | factoring N, same as RSA |
| Rabin analogue | Cobbe–Nickel–Schuster 2025; Awad 2025 | factoring N |
| Diffie–Hellman and ElGamal in (Z[i]/(π))* | Dong 2016; Fazekas 2023; Megías 2026; Khalaf 2024 | DLP in GF(p) or GF(p²) |
| ECC with Z[i]/(π) as the field representation | Safieh–Freudenberger 2020/2021 | ECDLP over GF(p); Z[i] is only an arithmetic trick |
| McEliece with codes over Gaussian integers | Freudenberger–Thiers 2021 | decoding, code-based |
| NTRU-like | Jarvis–Nevins (Eisenstein); marginal Gaussian variants | lattice |

### 1.3 Cryptanalysis: no stronger than the integer versions

The algebra explains why. Z[i]/(π) is GF(p) for a split prime p = ππ̄, or GF(p²) for an inert p ≡ 3 mod 4. So Gaussian ElGamal and Diffie–Hellman are ordinary finite-field DLP. Gaussian RSA is factoring N, and its larger group order adds attack surface rather than removing it. The published attacks confirm this:

- M. Bunder, A. Nitaj, W. Susilo, J. Tonien, "Cryptanalysis of RSA-type cryptosystems based on Lucas sequences, Gaussian integers and elliptic curves", J. Inf. Security and Applications 40:193–198, 2018 ([UOW](https://ro.uow.edu.au/eispapers1/1449), [HAL](https://hal.archives-ouvertes.fr/hal-02320970)). Continued fractions break the Elkamchouchi Gaussian RSA when d is small. In experiments with 513-bit p and 511-bit q, d up to 520 bits was recovered.
- P. Cotan, G. Teşeleanu, "A security analysis of two classes of RSA-like cryptosystems", J. Math. Cryptology, 2024, doi:10.1515/jmc-2024-0013 ([De Gruyter](https://www.degruyterbrill.com/document/doi/10.1515/jmc-2024-0013/html)). Covers the (pⁿ−1)(qⁿ−1) family; n = 2 is the Gaussian case.
- G. Teşeleanu, "Partial exposure attacks against a family of RSA-like cryptosystems", Cryptography 9(1):2, 2025 ([MDPI](https://mdpi.com/2410-387X/9/1/2)). Lattice (Coppersmith/Kunihiro) attacks with partial key exposure. For Gaussian RSA with e ≈ N² it gives d < N^0.585, matching earlier lattice bounds by Feng, Nitaj and Pan (not verified separately).
- M. Seck, O. Niang, D. Sow, A. Nitaj, M. Zheng, M. Boudabra, "A new generalized attack on RSA-like cryptosystems", IACR ePrint 2025/380 ([ePrint](https://eprint.iacr.org/2025/380)). Ordinary-integer RSA-like family; same line of work.
- Cobbe et al. 2025 present their Z[i] Rabin as a plaintext-capacity gain (twice the bits per ciphertext), not a security gain. Decryption is about 23 times slower than classical Rabin in their PARI/GP implementation.

The Tier C security claims ("more secure", "higher brute-force complexity") are not supported by any reduction, and the Tier A cryptanalysis contradicts them.

## 2. Operations and sizes the papers need

| Operation | Who needs it | Sizes |
|---|---|---|
| Multiplication and squaring mod a Gaussian modulus (π, or N = pq viewed in Z[i]) | all RSA, Rabin, ElGamal, ECC papers | 1024 to 4096-bit N (Megías: 2048 default, 3072, 4096; Bunder et al.: 1024-bit N); ECC 160 to 521-bit p |
| Modular exponentiation in Z[i]/(n) | RSA, ElGamal, DH, Rabin square tests | as above; exponents up to about 2× the modulus bits, since group orders like (p²−1)(q²−1) are about N² |
| Inverses mod n (via norm and conjugate, or a Gaussian xgcd) | RSA keygen, ElGamal decryption, ECC | as above |
| gcd and extended gcd in Z[i] | CRT recombination (Awad and El-Kassar use the Z[i] Euclidean xgcd, per Cobbe et al.), sums of two squares | up to millions of digits in the number-theory use |
| CRT over Z[i] | Rabin and RSA decryption | 512 to 2048-bit factors |
| Square roots mod π (Tonelli–Shanks in GF(p) or GF(p²)) | Rabin; sums of two squares (sqrt(−1) mod p) | crypto sizes; up to 10⁷ digits in records |
| Montgomery or Barrett reduction mod π | ECC hardware (Safieh–Freudenberger) | 160 to 521 bits |
| Gaussian primality and prime generation (split p ≡ 1 mod 4 with p = a² + b², or inert p ≡ 3 mod 4) | all keygen; moat and distribution studies | crypto: 512 to 2048-bit norms; number theory: many small primes (norm up to about 10¹⁶) |
| Norms and canonical associates | everywhere | everywhere |
| Factorisation in Z[i] | cryptanalysis (via N), teaching, Tier C | reduces to factoring the norm in Z |
| Polynomial arithmetic over Z[i] or Z[i]/(q) | NTRU and lattice variants; FLINT's internal use for exact complex polynomials | degree 256 to 1024, small q |
| Quartic (biquadratic) residue symbol, Gaussian Jacobi symbol | residuosity-based protocols, primality | crypto sizes |

Two size regimes stand out. Crypto uses fixed moduli of about 1024 to 4096 bits, with heavy exponentiation, which favours batched Montgomery or Barrett arithmetic. Number theory uses either very many small values (moat searches, prime counting, with norms below 2⁶⁴ to 2¹²⁸) or a few huge values with millions of digits (sum-of-two-squares records).

## 3. Software in use, and the gaps

| Tool | Z[i] support | Gaps |
|---|---|---|
| FLINT `fmpzi` ([docs](https://flintlib.org/doc/fmpzi/index.html)) | add, sub, mul, sqr, pow_ui, divexact, divrem (round to nearest, ties up, N(r) ≤ N(y)/2), divrem_approx, remove_one_plus_i, gcd (Euclidean, improved, binary, shortest), is_prime and is_probabprime (via the norm), polynomial mulmid (KS, Schönhage–Strassen, small-prime FFT) | no xgcd, no arithmetic mod a Gaussian modulus, no powmod, no factorisation (the docs say so), no vector or batched API |
| PARI/GP | `t_COMPLEX` with integer parts; gcd and factor work on Gaussian integers; `nfeltdivrem` and `nfeltdiveuc` through `nfinit(y^2+1)` | No built-in Gaussian mod. In a Nov 2025 pari-users thread H. Stamm-Wilbrandt asked for a `gimod()` ([request](https://pari.math.u-bordeaux.fr/archives/pari-users-2511/msg00050.html), [gist](https://gist.github.com/Hermann-SW/9fd371d850ac1822e0fc3482ed3e7a2f)). B. Allombert replied that a division convention would have to be fixed and that `t_COMPLEX` is not Gaussian-only ([reply](https://pari.math.u-bordeaux.fr/archives/pari-users-2511/msg00051.html)). Also: "`\` fails on t_COMPLEX by t_INT". |
| SageMath | `ZZ[I]`, `GaussianIntegers()`, ideals, residue fields, generic quotients | Generic quotients `ZZ[I].quotient(J)` have "barebones functionality" and do not know they are finite ([ask.sagemath 2020](https://ask.sagemath.org/question/49838/find-order-of-an-elliptic-curve-over-gaussian-integer/), [2020](https://ask.sagemath.org/question/49612/how-to-define-ring-of-gaussian-integers-modulo-n/)); reduction of Z[i] polynomials mod p needs workarounds ([sage-nt 2021](https://groups.google.com/g/sage-nt/c/q51ruMkCjLU)). Python-level speed. |
| Mathematica | `GaussianIntegers -> True` for FactorInteger, PrimeQ, Factor ([docs](https://reference.wolfram.com/language/ref/GaussianIntegers.html)) | The docs do not mention Gaussian moduli for PowerMod or ModularInverse. Closed source. |
| Magma | general number-field and order machinery | nothing specific found in this survey |
| Malachite (Rust) ([mapping](https://www.malachite.rs/mapping/flint-gaussian-integers/)) | mirrors fmpzi: arithmetic, divrem, gcd | no primality yet ("planned"), no modular layer |
| `gauss_int` (Rust crate) ([docs.rs](https://docs.rs/crate/gauss_int/0.2.0)) | Gaussian arithmetic, gcd, Pollard rho, BPSW, CRT through a BigInt wrapper | hobby scale, no vectorisation |
| GauInt (Python, 2023) ([PyPI](https://pypi.org/project/gauint)) | arithmetic, xgcd, factorisation, random Gaussian primes | pure Python |

What the papers report:

- Most Tier B and C papers report no timings, or report Python timings (Megías: about 0.18 s per 2048-bit complex product with four multiplications versus 0.22 s with Gauss's three-multiplication trick).
- Cobbe et al. report only ratios from PARI/GP.
- Safieh and Freudenberger report FPGA area and latency.
- Gaussian-moat computations used custom Java on 38 Pentium III CPUs, about 26 hours to rule out step √36 (N. Tsuchimura, "Computational results for Gaussian moat problem", METR 2004-13, [PDF](https://www.keisu.t.u-tokyo.ac.jp/data/2004/METR04-13.pdf); IEICE Trans. Fundamentals 88(5):1267–1273, 2005).
- L. Devin's Gaussian prime discrepancy data to 5·10⁹ used SageMath 9.1 ([arXiv:2105.02492](https://arxiv.org/html/2105.02492v3)).

The clearest speed complaint is from the sum-of-two-squares record work. A Gaussian gcd was "orders of magnitude slower" than PARI's integer `halfgcdii` for a 9,383,761-digit prime ([gmp-discuss, Aug 2023](https://gmplib.org/list-archives/gmp-discuss/2023-August/006908.html)). For an 11,981,518-digit prime, `gcd(s+I, p)` took 1,247 ms versus 190 ms for `halfgcd` ([pari-users, Feb 2024](https://pari.math.u-bordeaux.fr/archives/pari-users-2402/msg00086.html)). So this community routes around Gaussian gcd entirely, using a half-gcd in Z (Cornacchia, Hermite–Serret).

We found no Gaussian-integer library aimed at cryptographers, and nothing vectorised or on GPU. The closest work is hardware: the HTWG Konstanz FPGA designs.

## 4. Related number-theory research that would use the same tools

- Gaussian moat (Erdős problem 952, [forum](https://www.erdosproblems.com/forum/thread/952)). E. Gethner, S. Wagon, B. Wick, "A stroll through the Gaussian primes", Amer. Math. Monthly 105(4):327–337, 1998; Tsuchimura 2004/2005 above. Recent arXiv claims (Das 2019, [arXiv:1901.04549](https://www.arxiv.org/abs/1901.04549); Stumpenhusen 2024, [arXiv:2401.08441](https://arxiv.org/abs/2401.08441v2)) are both withdrawn. Analogues: B. Li, S. J. Miller et al., moats in Z[√2], J. Integer Seq. 25, 2022 ([JIS](https://cs.uwaterloo.ca/journals/JIS/VOL25/Miller/miller11.html)). Workload: enumerating and testing very many Gaussian integers with norm below about 10¹⁶, which is exactly a vector, small-word workload.
- Distribution of Gaussian primes in sectors and angles (Devin 2021, above; "Angles of Gaussian primes", [arXiv:1705.07498](https://arxiv.org/pdf/1705.07498); "Gaussian primes in almost all narrow sectors", [arXiv:1903.04005](https://arxiv.org/pdf/1903.04005)). The computation is sieving and primality of norms to 10⁹ to 10¹².
- Sums of two squares and Cornacchia for record primes (Stamm-Wilbrandt, above). Needs a huge-operand gcd or half-gcd and sqrt(−1) mod p.
- Fast gcd and residue symbols in Z[i]: A. Weilert, "(1+i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Comput. 30(5):605–617, 2000; Weilert, "Asymptotically fast GCD computation in Z[i]", ANTS-IV, LNCS 1838, 2000; Weilert, "Fast computation of the biquadratic residue symbol", J. Number Theory 96, 2002 (all three not verified). I. Damgård, G. S. Frandsen, "Efficient algorithms for gcd and cubic residuosity in the ring of Eisenstein integers", BRICS RS-03-8, 2003 ([BRICS](https://brics.dk/RS/03/8/index.html)), which notes the same technique gives fast gcd and quartic residuosity in Z[i]. Our vector binary gcd belongs to this (1+i)-ary family.
- Pollard rho in Z[i] (the Stamm-Wilbrandt and Chapman port above), Gaussian factorisation as a teaching topic, and binary quadratic forms of discriminant −4, which are reduction problems in Z[i] in another form.

## 5. Direction for our library

### 5.1 Needs ranked by how often they appear

Status key: "have" means already in our design; "listed" means on our current list; "missing" means not planned.

| Rank | Need | Status |
|---|---|---|
| 1 | Arithmetic modulo a Gaussian modulus (reduce, add, mul, sqr in Z[i]/(n), for n a Gaussian prime π, a rational N, or a general Gaussian n) | partial: we have mod n by Barrett. Need a canonical residue convention and fused mulmod/sqrmod, with Montgomery as an option |
| 2 | Modular exponentiation in Z[i]/(n) with exponents of 1 to 2× the modulus bits | missing |
| 3 | Inverse mod n | have (on list) |
| 4 | Sizes of 1024 to 4096 bits for the modulus, fixed per batch | partly: our limbs grow by 32 bits, so large sizes are possible, but they must be fast at 1024 to 4096 rather than merely correct |
| 5 | gcd and extended gcd | gcd: have (vector binary gcd). xgcd: missing, and FLINT lacks it too |
| 6 | Gaussian primality (via the norm, plus the inert case p ≡ 3 mod 4) and prime generation (split π = a + bi from p ≡ 1 mod 4 via sqrt(−1) and half-gcd; inert p) | missing |
| 7 | CRT over Z[i] | missing (cheap once xgcd exists) |
| 8 | Square roots mod π (GF(p) or GF(p²) Tonelli–Shanks; sqrt(−1) mod p) | missing |
| 9 | Division with remainder, exact division, norms, canonical associates | have. Our ties-toward-zero rounding differs from FLINT's ties-up, so document it and offer a FLINT-compatible mode for cross-checking |
| 10 | Batch small-norm primality and enumeration (moats, sector counts, norm up to 2⁶⁴ to 2¹²⁸) | missing, and a natural fit for NEON and GPU |
| 11 | Polynomial arithmetic over Z[i]/(q) (NTRU-like, degree 256 to 1024) | missing; lower priority |
| 12 | Quartic residue symbol | missing; niche |
| 13 | Factorisation | missing; low priority, since it reduces to factoring the norm in Z, which PARI and FLINT already do |

### 5.2 What a first useful release would contain

Make it a batched modular layer over Z[i], aimed at 1024 to 4096-bit moduli and at large batches of small values:

1. A modulus context for n in Z[i] (precomputed Barrett constants, optionally Montgomery), with canonical residues and with mulmod, sqrmod, powmod and invmod, each vectorised across many operands sharing one modulus.
2. Extended gcd, with CRT built on top.
3. Gaussian prime testing and generation: Miller–Rabin or BPSW on the norm, plus a sqrt(−1) mod p and half-gcd path to split p = a² + b².
4. A batched small-norm primality and enumeration kernel for moat and distribution work. This is our clearest performance niche, since the existing record used Java on 38 CPUs.
5. Bit-exact cross-checks against FLINT `fmpzi` and PARI on every operation, and a short benchmark table at 1024, 2048 and 4096 bits against FLINT and PARI. No current paper has such numbers.

Positioning, honestly: the cryptographic schemes over Z[i] are no stronger than their integer counterparts, and the serious users are a few groups (HTWG Konstanz on hardware and codes, the Nitaj, Teşeleanu and Susilo cryptanalysis line, the Bundeswehr Rabin group) plus many marginal authors. The strongest case for the library is as a fast, exact, correct workbench for (a) researchers prototyping and attacking these schemes at realistic sizes, which FLINT and PARI do not support with a modular layer, and (b) computational number theory on Gaussian primes. For (b), batch throughput on NEON and GPU is a real advantage and no existing tool offers it.
