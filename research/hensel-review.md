# Bottom-up methods across the library: a review (Nov 4, 2026)

Scope: every operation in SPEC.md 1.4 and decisions 28 to 53, asked one question: is there a bottom-up method (Hensel, 2-adic, (1+i)-adic, working from the low end), is it in use, how does it compare with the textbook choice, and could it improve it. Storage is the ordinary pair (decision 15), not the paired-bit form of studies 31 to 33. Reading, web and small Python checks only; nothing was timed. Decision numbers refer to decisions.md, study numbers to studies.md. Sources marked "verified" were checked online in this pass; "from the surveys" means cited in an earlier research file and not rechecked; "unverified" means from memory.

## 0. Summary

| Operation | Bottom-up method | Status here | Verdict |
|---|---|---|---|
| Add, subtract, negate, conjugate | Wrapping add is 2-adic arithmetic mod 2^32 | In use (it is the machine) | Nothing to change |
| Multiply | Low-half products are 2-adic | In use inside the widening multiply | Nothing to change |
| Norm | v_{1+i}(x) = v_2(N(x)) | Not used | A fact for the book, not a method |
| Exact division | Hensel division (Jebelean), Newton inverse mod 2^32 | Textbook (29) | Already right; grown sizes need one Newton step per doubling |
| Division with remainder | Hensel division with remainder (x = qy + 2^k r) | Not applicable | A different operation; nearest rounding stays top-down |
| Mod n reduction | Montgomery | Trick (38) | Stays a trick as an operation; becomes the engine inside powers mod n |
| Powers mod n | Montgomery form internally; even n by a 2^e split | Missing | Candidate textbook answer |
| Inverses mod n | Newton mod 2^k, Bernstein–Yang divsteps, Kaliski | Research (53) | Fermat with Montgomery for prime n; batched divsteps otherwise |
| Gcd | (1+i)-ary binary gcd; Hensel quotient (study 33) | Textbook (31); candidate | Hensel quotient is the strongest candidate, needs a NEON-shaped study |
| Extended gcd | (1+i)-ary with coefficient fix; bulk Montgomery fix | Study 14 | Candidate: Hensel quotient plus a bulk fix (checked) |
| CRT | Hensel lifting is its p-adic dual; 2^e component by a mask | Missing | No change; the 2^e case is nearly free |
| Primality | Montgomery powers; 2-adic trial division test; binary Jacobi | Missing | Use all three |
| Square roots mod p | Tonelli–Shanks (2-Sylow, bit by bit); Hensel lifting to p^k and 2^k | Missing | Lifting is the textbook way to prime powers |
| Standard associate | Primary associate (u ≡ 1 mod 2 + 2i), read from low bits | Quadrant (41) | Quadrant stays; primary is an internal tool |
| Divisibility | Hensel quotient plus the multiply's fit test | Left out (48) | Improvement, checked: no back-multiply compare needed |
| Overflow, shrink | None: overflow is an archimedean notion | Mask tests (13, 20) | Lesson only: skip the test where a bound proves the fit |

## 1. Add, subtract, negate, conjugate

A wrapping int32 lane computes in Z/2^32, the 2-adic integers truncated to 32 digits, and its carries run from the low end up. The pair with wrap is the ring Z[i]/2^32 = Z[i]/(1+i)^64 (decision 54's remark on base −1+i). So the machine default is already bottom-up, and the overflow test (13, 20) is what makes it archimedean: it asks whether the 2-adic answer is also the integer answer. There is no bottom-up alternative to compare. One small consequence: after an overflowing add the low piece is still correct, so a redo at the next size could reuse it; at one rare event per array this is not worth the complication.

## 2. Multiply

The low 32 bits of each part of a product are 2-adic and come from plain `mul`, `mla`, `mls` in 32-bit lanes (four per register). The textbook widening multiply (smull, smlsl) exists because the fit test needs the high half. Computing the low half by wrapping and the high half separately (sqdmulh on each product, plus the borrow from the low halves) costs more than the widening form, so nothing changes. Where a result is known to fit in advance, the wrapping multiply alone is exact; sections 4 and 15 use this.

## 3. Norm

The norm is archimedean by nature (decision 52 keeps it as N + 0i). The bottom-up fact worth stating: v_{1+i}(x) = v_2(N(x)), so the (1+i)-adic valuation is one trailing-zero count of the norm, equal to the strip formula 2·min(ctz a, ctz b) + [ctz a = ctz b] of the adic survey. It is cheaper only where the norm is computed anyway (division, the gcd's size test). No method change.

## 4. Exact division

In use (29): strip 2^k and at most one 1+i, then multiply x·conj(y) by N(y)^−1 mod 2^32 from a Newton iteration x ← x(2 − Nx). This is Hensel's exact division (Jebelean 1993; Granlund and Montgomery 1994, section 9; GMP's `mpn_divexact` and `binvert`), and Newton mod 2^k is Hensel lifting (Brent and Zimmermann, MCA 1.4.8: "Newton's iteration is called Hensel lifting in the LSB case", verified). Inverting the integer norm is cheaper than Newton on the Gaussian inverse directly (one integer step is two multiplies, one Gaussian step two Gaussian products), and the result is the same, since y is a unit mod 2 exactly when N(y) is odd. Comparison with the top-down route (two divides by N, or a reciprocal): no correction step, constant time in lanes, fixed width, everything in 32-bit lanes because only the low halves of x·conj(y) are needed. Its input condition, that y divides x, is met by definition, and the strip handles even divisors. Gaps: at grown sizes the inverse mod 2^(32k) costs one more Newton step per doubling of width, and for 1,024 bits and up Krandick and Jebelean's bidirectional division (high half from the top, low half from the bottom) is the known saving (from the surveys, unverified). Verdict: already the textbook answer; it also gives divisibility almost for free (section 15).

## 5. Division with remainder

Nearest rounding (28) is defined by size, so it is top-down by nature. The bottom-up counterpart is Hensel division with remainder: for odd y, q = x/y mod 2^k and r = (x − qy)/2^k, so x = qy + 2^k r (GMP's `mpn_bdiv_qr`: "Q = N / D mod B^n, R = N − QD", verified in the source). It cancels the low part instead of the high part. It does not produce our quotient, and no hybrid found does: the low bits of the nearest quotient need the remainder first. It is, however, the step inside Montgomery reduction (section 7) and study 33's gcd (section 9), and it deserves a page in the book as the mirror image of the box: the 2-adic remainder lies in a 2-adic ball instead of a box around zero. MCA 1.4.8 explains a cost difference we measured without naming: "LSB-division does not need any correction step, since the carries go in the direction opposite to the cancelled bits" (verified). That is why exact division (4) has no fix-ups while 44 and 45 need an exact test and fallbacks for N < 256 and unit divisors. Verdict: keep 28, 44, 45.

## 6. Mod n reduction (and add, subtract, negate mod n)

Montgomery reduction is Hensel division keeping only the remainder (MCA 1.4.8: "When only the remainder is wanted, Hensel's division is usually known as Montgomery reduction", verified; Montgomery 1985, Math. Comp. 44(170) 519–521, unverified page numbers). It is the bottom-up twin of Barrett. Study 28's NEON counts: array by array, widening Montgomery 22 against 44 for Barrett per 4 Gaussian products; with a shared multiplier, Barrett multiplication by `sqrdmulh` 5 against 6 or 9, and Montgomery's conversions repay only after about 8 chained shared operations. Becker, Hwang, Kannwischer, Yang and Yang call their shared-multiplier method a "combination of Montgomery multiplication and Barrett reduction" (TCHES 2022(1), ePrint 2021/986, verified), which is the cleanest statement that the two meet when one factor is precomputed. Both are constant time and fixed width. Montgomery needs odd n and returns xR^−1, a twisted form. The odd-n limit has a standard repair: write n = 2^e·m, reduce mod m by Montgomery and mod 2^e by a mask, recombine with m^−1 mod 2^e (Koç, "Montgomery reduction with even modulus", IEE Proc. Comput. Digit. Tech. 141(5) 314–316, 1994, verified; GMP's `mpz_powm` does exactly this, with `mpn_powlo` for the 2^e part and `mpn_binvert` for the recombination, verified in the source). Add, subtract and negate mod n have no bottom-up content; Montgomery form is additive, so they are unchanged in it. Verdict: Barrett stays the textbook answer for mod n as a user-visible operation (any n, any call shape, results in normal form). Montgomery's objections (conversions, twisted form, odd n) disappear inside an operation that runs a chain of products and converts once at each end; see sections 7, 8 and 12.

For a Gaussian modulus: Safieh and Freudenberger reduce mod a Gaussian prime π with R = 2^l (Cryptography 5(1):6, 2021, verified), the Gaussian form of the same idea. Mod (1+i)^(2k) = mod 2^k·(unit) is a mask on both parts, the one modulus where the 2-adic view makes reduction free.

## 7. Powers mod n

Missing in the spec. The standard fast answer is Montgomery form held across the whole exponentiation: one conversion in, about 1.5·log2(e) products, one conversion out, all hidden inside the call, so the user sees normal residues. Study 28's count (22 against 44 per 4 Gaussian products array by array, repaid after about 8 chained operations) says this wins for any exponent longer than about 8 bits. Even n by the 2^e split of section 6, as GMP does. Constant time needs a ladder or fixed window over a public exponent length (classification, section 4); right-to-left scanning also gives two independent chains, which suits latency, but that is bottom-up in the exponent, not 2-adic arithmetic. Verdict: candidate textbook answer, Montgomery inside, Barrett for the outside interface; this is how the book can answer the decisions' note that Montgomery is crypto's standard while Barrett is ours.

## 8. Inverses mod n

Bottom-up methods everywhere: the Newton inverse mod 2^k (Hensel lifting, in use in exact division; Dumas 2014, from the surveys), Bernstein–Yang divsteps (2-adic: each step halves g, TCHES 2019(3), from the surveys; the iteration bound used in study 13 is consistent with the b = log2(f² + 4g²) form ⌊49b/34⌋ quoted by the safegcd-bounds repository, verified there, and that repository also gives the roughly 18% shorter hddivsteps bound), Pornin's binary gcd on top and bottom 31 bits (ePrint 2020/972, from the surveys), and Kaliski's Montgomery inverse b^−1·2^n mod a by the right-shifting binary algorithm (IEEE Trans. Computers 44(8) 1064–1065, 1995, verified; not constant time). Decision 53's plan (divsteps for the odd part, Newton for 2^e, CRT) is the same 2^e split as Koç and GMP. Study 13's costs: Euclid about 136 instructions mean, unbatched divsteps 3,517 fixed. Two bottom-up savings were not tried there: batching about 30 divsteps into a 2×2 matrix built from the low 32 bits (what Pornin and libsecp256k1 do at 62), and delaying the halving of the coefficient into one multiply by 2^−K mod n at the end. For prime n, Fermat x^(n−2) by section 7's Montgomery power is constant time and vectorised: about 46 integer Montgomery products at 31 bits, plausibly 100 to 250 NEON instructions per element against 3,517 (estimate, not counted). Verdict: for prime n, Fermat inside the power routine; for other odd n, batched divsteps; even n by the 2^e split. Euclid (22) stays for the variable-time default.

## 9. Gcd

In use (31): the (1+i)-ary binary gcd is bottom-up (Weilert 2000, from the surveys). Study 33's Hensel quotient q = a/b mod 2^j with a − qb shedding at least 2j factors of 1+i is the Gaussian form of Stehlé and Zimmermann's binary division (ANTS 2004, LNCS 3076, verified; MCA 1.6.3) and a relative of the k-ary reduction of Sorenson (J. Algorithms 16, 1994, 110–144) and Jebelean and Weber, which finds small a, b with au + bv ≡ 0 mod k (verified from the citations in arXiv 1402.2266). One difference matters: the k-ary step multiplies u and can add spurious factors that need a cleanup pass, while a − qb keeps gcd(a − qb, b) = gcd(a, b), so study 33 needs none. Study 33 picks j from the size difference L (two leading-zero counts): a top guide for a bottom step, the same hybrid shape as Pornin's top and bottom bits. In the model: 15.5 steps against 30.2, 1,148 ops against 1,777 (−36%) on pairs, no divides, 18 multiplies per step. Caveat for decision 34: the study used 64-bit wrapping arithmetic and intermediates of about 36 bits. NEON has no 64-bit lane multiply, so a − qb must be formed by widening multiply-subtract into 64-bit lanes and narrowed after the exact shift by j (the result is again below |a|), and the 3-round Newton inverse is a dependent multiply chain. Not constant time (a Gaussian divstep remains a research project). Verdict: the strongest candidate in this review for changing a decided textbook answer; it needs a NEON-shaped count before it can.

## 10. Extended gcd

Study 14's extended (1+i)-ary gcd already contains a one-digit Montgomery step: before dividing a by 1+i it adds m·(B′, −A′) with m ∈ {0, 1} so both coefficients become divisible. The bulk form strips t factors at once with m = −u_a·B′^−1 mod 2^(⌈t/2⌉ + 1) (B′ odd), after which u_a + mB′ and v_a − mA′ are both divisible by (1+i)^t; checked in 20,000 random cases at 31-bit inputs, no failures (section 18). Combined with the Hensel quotient (coefficients updated by u_a − q·u_b), it would replace study 14's 91 single divisions by 1+i (about 2,370 of its 4,470 instructions) with about 15 bulk fixes. For inverses mod n the delayed correction (one multiply by (1+i)^−K mod n at the end, as Bernstein–Yang do with 2^−K) removes the fixes entirely, but section 8's norm route stays cheaper for rational n. Verdict: candidate for the missing extended gcd, together with section 9's study.

## 11. Chinese remainder theorem

Garner's method is not bottom-up. Hensel lifting is the p-adic dual of CRT: for a modulus p^k one lifts a solution mod p instead of combining coprime moduli (MCA 2.7 and 1.4, verified by contents only). The bottom-up special case worth stating: when one modulus is 2^e, the combination needs m^−1 mod 2^e by Newton and a mask, nothing else (GMP powm, section 6). Verdict: Garner stays; the 2^e case is the cheap one used in sections 6 to 8.

## 12. Primality and prime generation

A Gaussian prime test reduces to an integer test on N(x) or on |a| (inert primes, a ≡ 3 mod 4). Three bottom-up tools: Miller–Rabin and BPSW start from n − 1 = 2^s d (one trailing-zero count) and run powers, so section 7's Montgomery power is their engine (GMP's tests call `mpz_powm`); trial division by small primes d uses the 2-adic divisibility test x·d^−1 mod 2^32 ≤ ⌊(2^32 − 1)/d⌋ for odd d (Granlund and Montgomery 1994 section 9, from the surveys; Warren, Hacker's Delight 10-17, unverified), branch-free and division-free in lanes; and the Jacobi symbol for BPSW's Lucas parameters has a binary, low-end-first algorithm (Brent and Zimmermann, ANTS IX 2010, arXiv 1004.2091, from the surveys). Verdict: all three belong in the textbook answer; the 64-bit norm needs the trial-division test in 32-bit pieces, not yet designed.

## 13. Square roots mod a prime

Tonelli–Shanks writes p − 1 = 2^s q and then fixes the 2-part of the discrete log one bit at a time, a bottom-up walk through the 2-Sylow subgroup; its loop length depends on the values, so it is not constant time (for p ≡ 3 mod 4 one power suffices). Square roots mod p^k come by Hensel lifting from mod p (odd p, a a unit), and mod 2^k by the division-free inverse-square-root Newton y ← y(3 − ay²)/2, which needs a ≡ 1 mod 8 and gains k → 2k − 2 bits per step (checked; the 2k − 1 one might expect fails). The Gaussian use: √−1 mod p splits p = ππ̄ (gcd(p, t + i)), and lifting t to mod p^k gives the map Z[i]/(π^k) → Z/p^k that SPEC's "mod a Gaussian modulus through the mapping" needs for prime-power moduli. A side result for the library-level exact absolute value (decisions, library-level ideas): a perfect-square norm has its root computed exactly by the 2-adic Newton mod 2^64 (5 rounds from 3 bits), then the smallest of the four roots ±r, ±r + 2^62 and a range test, with no divide and no correction loop; checked exhaustively below 2^16 and on 2·10^5 random cases. Verdict: Hensel lifting is the textbook way to prime powers; Tonelli–Shanks or Cipolla for the prime itself.

## 14. Standard associate

The quadrant convention (re > 0, im ≥ 0) is archimedean and agrees with |n| on integers (41). The 2-adic canonical choice is the primary associate, u ≡ 1 mod (1+i)^3, read from low bits only (study 33: 11 ops in rotated coordinates). It exists only for odd elements, needs a separate convention for the power of 1+i, and sends the odd integer 3 to −3, breaking 41's |n| rule. Verdict: quadrant stays the textbook answer; primary stays an internal tool (the gcd loop, quartic residue symbols).

## 15. Divisibility

Left out of the first version (48), and classification section 5 suggested exact division plus a back-multiply and compare. A cheaper bottom-up test: strip from y the 2^k and 1+i (failing at once if x's low bits lack them), compute the Hensel quotient q as in section 4, form q·y with the multiply's widening products. The low 32 bits of each part of q·y equal x's by construction, so y divides x exactly when q·y passes the multiply's existing fit test; no compare against x is needed. Checked exhaustively at 5-bit halves (1,048,576 pairs) and on 200,000 random cases at 32 bits (5,154 divisible): the only disagreements were the 129 cases with divisor norm 1 or 2 whose true quotient has a part of 2^(w−1), the edge classification section 3 already names. For an integer divisor the Granlund–Montgomery range test (section 12) does it with one multiply and one compare. Constant time and fixed width. Verdict: an improvement worth adopting when divisibility returns; it costs exact division plus one checked multiply.

## 16. Overflow and shrink tests

No bottom-up counterpart: 2-adically nothing overflows, and the test is exactly the question whether the 2-adic answer is the integer one. Hensel-code systems (Krishnamurthy, Rao and Subramanian; from the adic survey) meet the same problem as "overflow detection". Two uses of the 2-adic view: where a bound proves the result fits (exact division's quotient outside the norm-1 and norm-2 edge, the gcd's new values, the strip), the wrapping computation needs no test; and divisibility (15) turns the fit test itself into an answer. The shrink test reads top bits; nothing to add.

## 17. Hensel as a theme

17.1 What unifies these methods. Every integer has two sizes: the archimedean size |x|, and the 2-adic one, measured by divisibility by powers of 2 (for Z[i], by powers of 1+i, with v_{1+i}(x) = v_2(N(x)); 1+i is the ramified prime above 2, 2 = −i(1+i)², and its residue field is F_2, so the parity tricks of Z carry over). A machine word is a 2-adic integer truncated at 32 digits, so the machine is natively 2-adic and becomes archimedean only through the overflow test. Each top-down method then has a bottom-up mirror that cancels the low digits instead of the high ones (MCA 1.4.8: "Most MSB-division variants ... have their LSB-counterpart", verified):

| Top-down (cancel high digits) | Bottom-up (cancel low digits) |
|---|---|
| Division by reciprocal, with corrections | Exact (Hensel) division, no corrections |
| Nearest remainder in a box | 2-adic remainder, x = qy + 2^k r |
| Barrett | Montgomery |
| Newton reciprocal | Newton inverse mod 2^k (Hensel lifting) |
| Euclid; Knuth–Schönhage half-gcd | Binary gcd; Stehlé–Zimmermann binary recursive gcd |
| Rounded Gaussian quotient step | Hensel quotient step (study 33) |
| Integer square root | 2-adic square root; Hensel lifting of roots to p^k |
| Quadrant associate | Primary associate |
| Garner's CRT | Hensel lifting across powers of one prime |

The bottom-up side pays in three currencies: it needs a unit at the bottom (odd divisor, odd modulus, 1+i stripped first); its answers come twisted (Montgomery's R^−1, a remainder times 2^k, a result valid only mod 2^w); and it needs to know the true answer fits, or a separate test. In return it has no correction steps, carries run away from the cancelled digits, and it is naturally constant time. The best recent methods are hybrids that take sizes from the top and steps from the bottom: study 33's L-guided Hensel quotient, Pornin's top and bottom 31 bits, Bernstein–Yang's δ counter standing in for a size comparison.

17.2 Where it fits in the three levels. At the machine default the 2-adic view is already there (wrapping arithmetic), and for the crypto audience Montgomery is the default. At the textbook level a bottom-up method belongs where a cheap normalisation lets it cover every input and the answer comes back in normal form: exact division, divisibility, the gcd (binary now, Hensel quotient if section 9's study confirms), inverses mod 2^k, Montgomery inside powers and Fermat inverses, Hensel lifting to prime powers. Top-down stays where size is the definition: division with remainder, canonical residues mod n, the standard associate, overflow. As tricks: bottom-up forms held across operations (Montgomery form exposed to the user, the valuation-unit form of study 31), and odd-only methods without their 2^e split. A suggestion for Part 1: introduce the int32 as Z/2^32 when the overflow test is first explained, then at each later operation show the mirror image from the table above. The theme then explains several measured facts (exact division 3 times cheaper than division with remainder, Montgomery's 2× in study 28, the binary gcd's win in study 26) instead of listing them.

17.3 Studies to run next, ranked by expected effect on a textbook answer.

1. Study 33's Hensel-quotient gcd in NEON shape on pairs: j capped so q fits 16 bits per part, a − qb by widening multiply-subtract into 64-bit lanes, exact narrowing shift, ARM instruction count against the 453 per gcd of decisions 31 and 39. Could replace 31.
2. Powers mod n with Montgomery inside and Barrett outside, even n by the 2^e split; then the Fermat inverse for prime n against study 13's divsteps (3,517). Fills two missing entries and the constant-time inverse.
3. Divisibility by Hensel quotient plus fit test (section 15), counted in NEON shape; small, already checked for correctness.
4. Batched divsteps for odd n in 32-bit lanes (about 30 steps per matrix) with delayed coefficient halving, and decision 53's CRT with 2^e.
5. Extended gcd: Hensel quotient plus bulk Montgomery coefficient fix against study 14's 4,470 instructions; record coefficient sizes.
6. Exact division and the Newton inverse at grown sizes (one step per doubling), and when Krandick–Jebelean bidirectional division pays.
7. Hensel lifting for square roots mod p^k and the map to Z/p^k for prime-power Gaussian moduli; the 2-adic exact square root for the library-level absolute value.
8. A Gaussian divstep with a proven step count (already on the research list; a project, not a study).

## 18. Checks run

Script `hensel_checks.py` in this session's scratchpad (not in the repo; about 2 s):
- Divisibility (section 15): exhaustive at 5-bit halves, 200,000 random at 32 bits. Only the 129 norm-1 and norm-2 edge cases disagree, all with a quotient part of 2^(w−1).
- 2-adic exact square root (section 13): 100,000 random squares of 31-bit numbers, 100,000 random N < 2^62, and every N < 2^16, against `math.isqrt`. No mismatches after correcting the precision step to k → 2k − 2.
- Bulk coefficient fix (section 10): 20,000 random cases with 31-bit A, odd B′ and 33-bit coefficients, t up to 40. No failures; the fixed coefficients satisfy the identity after division by (1+i)^t.

## 19. Sources

Verified in this pass:
- Brent, R. P. and Zimmermann, P., Modern Computer Arithmetic, Cambridge University Press, 2010, sections 1.4.5, 1.4.8, 1.6.3, 2.1.2, 2.1.4, 2.4.2 ([contents](https://prometeus.nsc.ru/acquisitions/13-09-10/cont06f.ssi); [draft 0.5.9](https://maths-people.anu.edu.au/~brent/pd/mca-cup-0.5.9.pdf)).
- Stehlé, D. and Zimmermann, P., "A Binary Recursive Gcd Algorithm", ANTS 2004, LNCS 3076 ([doi:10.1007/978-3-540-24847-7_31](https://link.springer.com/doi/10.1007/978-3-540-24847-7_31); [page](https://perso.ens-lyon.fr/damien.stehle/BINARY.html)).
- Koç, Ç. K., "Montgomery reduction with even modulus", IEE Proc. Comput. Digit. Tech. 141(5) 314–316, 1994 ([record](https://search.isc.ac/inventory/49/1184791.htm)).
- Kaliski, B. S., "The Montgomery inverse and its applications", IEEE Trans. Computers 44(8) 1064–1065, 1995 ([record](https://manuscript.isc.ac/Inventory/49/873803.htm)).
- Safieh, M. and Freudenberger, J., "Montgomery Reduction for Gaussian Integers", Cryptography 5(1):6, 2021 ([MDPI](https://www.mdpi.com/2410-387X/5/1/6)).
- Becker, H., Hwang, V., Kannwischer, M. J., Yang, B.-Y. and Yang, S.-Y., "Neon NTT: Faster Dilithium, Kyber, and Saber on Cortex-A72 and Apple M1", TCHES 2022(1) ([ePrint 2021/986](https://eprint.iacr.org/2021/986)).
- Seiler, G., "Faster AVX2 optimized NTT multiplication for Ring-LWE lattice cryptography", [ePrint 2018/039](https://eprint.iacr.org/2018/039) (a modified Montgomery reduction for integer instructions; study 28's mon_h is of this kind).
- GMP source: `mpz/powm.c` (even modulus by `mpn_powm` on the odd part, `mpn_powlo` on 2^k, `mpn_binvert`; [mirror](https://ftp.riken.jp/NetBSD/NetBSD-current/src/external/lgpl3/gmp/dist/mpz/powm.c)) and `mpn/generic/bdiv_qr.c` ([mirror](https://src.rcs.uwaterloo.ca/xref/netbsd-src/external/lgpl3/gmp/dist/mpn/generic/bdiv_qr.c)).
- sipa, safegcd-bounds ([GitHub](https://github.com/sipa/safegcd-bounds)): Bernstein–Yang's bound in the b form, and the hddivsteps bound.
- Sorenson, J., "Two fast GCD algorithms", J. Algorithms 16 (1994) 110–144; Jebelean, T., "A generalization of the binary GCD algorithm", ISSAC 1993, 111–116; Weber, K., "The accelerated integer GCD algorithm" (1995), all as cited in [arXiv 1402.2266](https://arxiv.org/pdf/1402.2266); the journal details of Weber's paper (ACM TOMS 21(1), 1995) are unverified.

From the earlier surveys, not rechecked: Jebelean 1993 (exact division, JSC 15(2)); Granlund and Montgomery 1994 (PLDI, section 9); Krandick and Jebelean 1996; Dumas 2014; Bernstein and Yang 2019; Pornin 2020; Weilert 2000; Brent and Zimmermann 2010 (Jacobi symbol); Krishnamurthy, Rao and Subramanian (Hensel codes).

Unverified: Montgomery 1985 page numbers; Warren, Hacker's Delight 10-17.
