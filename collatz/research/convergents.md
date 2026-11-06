# Cycle lengths and best approximations (5 Nov)

A toy test and a short literature survey, about an hour of work. Notation follows families.md and collatz/papers/gaussian-collatz-2.md, Sections 3 and 5.6. A cycle up to units has n odd steps, total valuation V, and unit eps with S^n(z) = eps z. Its relative denominator is rel(n, V, eps) = |eps (1+i)^V - alpha^n| / 2^(V/2), and best(n) is the minimum of rel over V and the four units. Call n a record if best(n) < best(m) for every m < n. For real alpha only even V can be close, and the records are the denominators of the convergents of log2|alpha|, apart from an occasional extra record at very small n, where rel = |1 - 2^x| is not symmetric in x (n = 2 for alpha = 5, n = 3 for m = 19). For complex alpha they are the best simultaneous approximations to the modulus condition V/n close to log2 N(alpha) and the angle condition n arg(alpha)/(pi/4) - V close to an even integer. Nothing here is committed, and no paper was edited.

## 1. Question and answer

The hypothesis was that the dominant cycle lengths of a rule are the n at which alpha^n is unusually close to a unit times (1+i)^V, and that these n are the records above.

Answer: it holds partly. Three statements are supported.

1. In every rule tested, the largest class sits at a record. So does every class with at least 3 cycles, except the multiples and sums of the main record. Those are classes like 74, 111 and 148 for 11 at level 6 (main record 37), 66 and 132 for 11 + 10i (main record 33), and 75 = 43 + 32 for 15. They have rel close to m times the record's rel, so they are good approximations, but they are not records. For the real rules the dominant lengths are convergent denominators: 10 for 13, 32, 43, 247 and 537 for 15, 37 (and 2, 11, 13, 24) for 11 at level 6, 6 for 9, 3 and 28 for 5 at level 4, 5 and 26 for 7 at level 5.
2. The converse fails. Many records carry no cycles, for example 13 at n = 7 and 227, and 9 at n = 5, 47 and 53. Many cycles sit at non-record lengths, mostly single cycles with heuristic count E far below 1. Being a record is close to necessary for a large class. It is not sufficient, because the branch count in E decays exponentially in n. The heuristic E of families.md, Section 3, is the better predictor, and the record property follows from it: if the prefactor E rel^2 decreases in n, then the n maximising E is a record.
3. For complex alpha the simultaneous condition is the right one. Records of the modulus condition alone, which are the convergents of log2 N(alpha), predict cycles that do not exist when the angle is wrong (Section 4.3).

Is it known? For 3x+1 and its relatives, the link between cycle lengths and convergents of log2 3 is classical, but as a necessary condition used to bound cycles. That is the work of Crandall, Steiner, Eliahou, Halbeisen and Hungerbuehler, and Simons and de Weger. The observation that the many-cycle rules have their cycles concentrated at convergent lengths, with class sizes set by rel, was not found stated anywhere. It is a short step from Simons and de Weger and from Belaga and Mignotte, and the integer examples (5x+1 at n = 3, 181x+1 at n = 2) are folklore. families.md, Section 3, already says that the main classes of 13 sit at convergents of log2 13. This note tests that remark across the census and the integer maps.

## 2. Prior art

Searches used WebSearch and WebFetch. Several sources could not be read, as listed at the end.

1. Crandall, On the "3x+1" problem, Math. Comp. 32 (1978). This was seen only as quoted by Halbeisen and Hungerbuehler: a nontrivial cycle with n odd terms and minimum m has n > min(q_k, 2m/(q_k + q_{k+1})), where p_k/q_k are the convergents of log2 3. This is the first appearance of convergents as constraints on cycle lengths. Crandall also introduced the qx+1 maps. The 181x+1 cycles (27 and 35, two odd steps, 181^2 = 2^15 - 7) are attributed to him in arXiv:2005.00346, which lists them in its Table 2 together with the 5x+1 cycles through 13 and 17 (three odd steps, 5^3 = 125 close to 2^7). That paper does not mention continued fractions.
2. Steiner (1977), and Simons and de Weger, Mersenne en het Syracuseprobleem, NAW 5/5 (2004) 218-220, and Acta Arith. 117 (2005). For a 1-cycle with K odd and L even terms, 2^(K+L) is close to 3^K. Simons and de Weger state that for K >= 4, (K+L)/K must be a convergent of log 3/log 2. Baker's bound then leaves finitely many convergents, and these are checked. For m-cycles the same method, with a reduction step, rules out m <= 68. The NAW note also treats 3x+q, where q = 2^r - 3^s forces an extra 1-cycle, and mentions a 34-step cycle of 7x+11.
3. Eliahou, The 3x+1 problem: new lower bounds on nontrivial cycle lengths, Discrete Math. 118 (1993). The minimal period has the form 301994a + 17087915b + 85137581c. The numbers come from the convergents of log2 3. Seen only through citations.
4. Halbeisen and Hungerbuehler, Optimal bounds for the length of rational Collatz cycles, Acta Arith. 78 (1997). Read through WebFetch. It gives bounds for the length of rational 3x+1 cycles, refines Eliahou's criterion, and uses that q_16 log_3 2 is close to an integer. It does not claim that rational cycle lengths are convergent denominators.
5. Belaga and Mignotte, and Belaga (Acta Arith. 106, 2003), on 3x+d. Seen only through the annotations in Lagarias's bibliography II (arXiv:math/0608208, read). They cover the distribution of the number and lengths of cycles of 3x+d, give tables of primitive cycles for d < 20000, and bound length/oddlength between log2 3 and log2(d+3). One entry says that numbers D = 2^l - 3^k "give 3x+D problems having many primitive cycles". That is the small-denominator mechanism, with d chosen to fit (n, l) rather than a fixed multiplier.
6. Cox, Ghosh and Sultanow, arXiv:2101.04067, and Gupta, arXiv:2008.11103. Both treat 3n+c cycles through c = 2^l - 3^n and parity vectors, without convergents.
7. Matthews and Watts, and Matthews's survey chapter (see literature.md, 2.6). They give the growth heuristic for generalized maps but say nothing about cycle lengths and convergents, as far as was read.

No source treats the Gaussian setting, the symmetric mx +- 1 maps, or the statement that the size of a class is governed by rel at record lengths. Not read: Crandall 1978 and Lagarias, The set of rational cycles for the 3x+1 problem, Acta Arith. 56 (1990), both off the fetch allowlist; the Belaga and Mignotte full texts (HAL refused the fetch); Matthews's qx+1 tables (his site is unreachable, see literature.md); Lagarias's 1985 survey and bibliography I beyond what literature.md records. Lagarias 1990 is the most likely place for a remark on which lengths carry rational 3x+k cycles [unverified].

## 3. Method and data

convergents.py computes best(n) for n up to 3000, with V within 3 of n log2 N(alpha) and all four units, and lists the records. For each observed class (n, V, eps) it gives the count, rel, E, whether n is a record, and whether (V, eps) is the best pair for that n. The heuristic E is the formula of families.md, Section 3, evaluated in floating point. It agrees with famtheory.py on the main classes: 117, 29.3, 764, 20.5, 613 and 18.4.

Rules tested:
- The families rules at their final bounds: 13, 15, 11 + 10i and 15 + 2i at level 7; 9 and 11 at level 6 (sets A and B).
- Every census rule at levels 4 to 7 with at least 3 cycle classes in logs/k345_1e6.txt, logs/k6_1e6.txt and logs/k7_1e6.txt, keeping one rule of each mirror pair: 54 rules. convrun.py reran families.py on each at bound 10^6 (2 minutes), so that every cycle is listed with (n, V, eps). The census lines themselves show only the top five cycles.
- The integer maps on odd x with |x| <= 2 * 10^5. The symmetric map is x -> (m x + d)/2^v with d = +-1 chosen so that 4 divides m x + d, the level-2 analogue of the Gaussian rules. The other is x -> (m x + 1)/2^v. Both were run for odd m from 3 to 31, and the second also for m = 181. For mx + 1 the cycle equation x (2^V - m^n) = (positive sum) fixes the side: x > 0 needs 2^V > m^n. So the one-sided records, which include the intermediate fractions, are reported as well.

## 4. Results for the Gaussian rules

### 4.1 The many-cycle rules

Records up to n = 3000, each with best(n), the heuristic E at the best (V, eps), and the observed number of cycles. Observed lengths that are not records come after the semicolon.

| rule | records n (best, E, cycles) | other observed n (cycles) |
|---|---|---|
| 13, k = 7 | 1 (0.19, 2.6, 1); 3 (0.073, 3.2, 2); 7 (0.065, 0.76, 0); 10 (0.0031, 117, 160); 227 (1.3e-4, 2e-10, 0) | 2 (1), 20 (2) |
| 15, k = 7 | 1 (0.062, 17, 16); 10 (0.049, 1.0, 1); 11 (0.017, 7.3, 2); 32 (0.014, 1.6, 7); 43 (0.0026, 29, 32); 247 (0.0014, 1.1, 4); 290 (0.0012, 0.72, 0); 537 (1.7e-4, 1.3, 2) | 8 (2), 75 (6), 107 (2) |
| 11 + 10i, k = 7 | 1 (0.32, 2.0, 1); 2 (0.16, 1.2, 1); 14 (0.12, 0.12, 0); 19 (0.12, 0.07, 0); 33 (6.2e-4, 764, 728) | 66 (29), 99 (2), 132 (3), 198 (1), 264 (1) |
| 15 + 2i, k = 7 | 1 (0.14, 3.9, 3); 6 (0.016, 21, 28); 1505 (0.0012, 2e-5, 0) | 5 (1), 12 (1) |
| 9, k = 6 (set A) | 1 (0.13, 8.8, 8); 5 (0.099, 0.72, 0); 6 (0.014, 18, 15); 47, 53, 153, 665 (E < 0.003, 0) | none |
| 11, k = 6 (set A, 10^6) | 1 (0.31, 1.7, 1); 2 (0.055, 8.2, 7); 11 (0.038, 1.4, 1); 13 (0.019, 4.4, 3); 24 (0.018, 1.8, 1); 37 (7.1e-4, 613, 498); 949 (4.2e-4, 2.9, 0); 986 (2.9e-4, 5.2, 0); 1935 (1.3e-4, 2.0, 0) | 74 (46), 111 (5), 148 (3), 185 (1), 222 (2) |
| 5, k = 4 | 1 (0.25, 2.8, 2); 2 (0.22, 0.9, 0); 3 (0.023, 29, 28); 28 (0.0097, 2.3, 3); 59 (0.0043, 1.2, 1); 146 (0.001, 0.17, 0) | 25 (1) |
| 7, k = 5 | 1 (0.12, 4.7, 4); 5 (0.026, 10, 4); 26 (0.0061, 6.1, 5); 109 (0.0012, 0.41, 0) | 4 (2), 6 (1), 10 (2) |
| 10 + 7i, k = 7 | 1 (0.20, 4.0, 3); 4 (0.095, 1.0, 2); 9 (0.0096, 6.7, 6); 616 (E 5e-73, 0) | none |

For real alpha these records are the convergent denominators. log2 13 = [3; 1, 2, 2, 1, 22, 23, ...] has q = 1, 3, 7, 10, 227. log2 15 = [3; 1, 9, 1, 2, 1, 5, 1, ...] has q = 1, 10, 11, 32, 43, 247, 290, 537. log2 11 = [3; 2, 5, 1, 1, 1, 25, ...] has q = 1, 2, 11, 13, 24, 37, 949, 986. log2 9 has q = 1, 5, 6, 47, 53, 153. log2 5 = [2; 3, 9, 2, 2, ...] has q = 1, 3, 28, 59, 146 (the extra record n = 2 is the intermediate fraction 5/2, a record only because rel is not symmetric). log2 7 has q = 1, 5, 26, 109. The ratios named in the hypothesis are convergents: 37/10 for log2 13, 128/37 for log2 11 and 168/43 for log2 15.

So every convergent denominator of log2 15 up to 537 except 290 carries cycles, and so does every one of log2 11 up to 37. The two 537-step cycles of 15, found last in families.md, sit exactly at the next convergent, 2098/537. For 11 at level 6 the heuristic predicts a few cycles at the convergents 949 and 986 (E = 2.9 and 5.2). That is a concrete prediction. It is out of reach of the present searches: Lemma 1 of families.md puts such cycles at norms near 10^13.

### 4.2 The census

Over the 54 census rules, 116 classes have n >= 2. 67 of them are at records, and they hold 1909 of the 2104 cycles. Records are sparse: a typical rule has 4 to 8 records below n = 300. Every class with at least 3 cycles is listed in logs/convergents_census.txt. All of them are at records, except the multiples and sums 74, 111 and 148 (11, level 6), 66 and 132 (11 + 10i) and 75 (15). In every class with at least 3 cycles, (V, eps) is also the best pair for that n. The dominant class of each rule is at a record, and so is the n maximising E, apart from a few small rules where E peaks at n = 2 and that length is not a record (n = 2 loses to n = 1 in rel but wins in branch count). Further examples beyond 4.1 are n = 2 for 9 + 4i (4 or 5 cycles per digit set) and 12 + 5i (7), n = 3 for 7 + 2i and 11 + 6i, n = 5 for 13 + 2i, n = 11 for 4 + 3i at level 4, and n = 16 for 9 + 6i at level 6.

### 4.3 Simultaneous versus modulus-only approximation

logs/convergents_modonly.txt lists, for eight complex rules, the records of the modulus error |n log2 N(alpha) - V| alone (the convergents of log2 N(alpha)) beside the records of the full condition. The angle error is given in units of pi/4, modulo 2.

- 9 + 4i, level 6: log2 97 = [6; 1, 1, 2, 458, ...], so n = 5 has modulus error 0.0004, the best of any small case. Its angle error is 0.34, so best(5) = 0.26, and there are no cycles of 5 odd steps. The cycles are at n = 2, a full record with angle error 0.065.
- 12 + 5i: the modulus record n = 5 has angle error 0.49, and there are no cycles. Its 7 cycles are at n = 2, with angle error 0.005.
- 13 + 2i: modulus records 7 and 23 have angle errors 0.64 and 0.53, and carry no cycles. The full record 5, which is not a modulus record, has 3.
- 11 + 10i: n = 33 is both a modulus record and an angle coincidence (0.0008 and 0.0007). The modulus records 4 and 5 have bad angles and no cycles.

So for complex alpha the relevant notion is the simultaneous one, as the hypothesis says. A modulus-only version would fail.

## 5. Integer analogues

From logs/convergents_int_sym.txt and logs/convergents_int_plus.txt, cycles with |min| <= 2 * 10^5.

Symmetric mx +- 1. The only cycles besides +-1 are:
- m = 5: n = 3 (+-7), at 7/3;
- m = 9: n = 6 (+-13), at 19/6;
- m = 11: n = 2 (+-1, V = 7), at 7/2;
- m = 13: n = 3 (+-1, V = 11) and n = 10 (+-25, +-49), at 11/3 and 37/10.

All are convergents of log2 m, the same near-coincidences as in the Gaussian rules: 9^6 close to 2^19, 13^10 close to 2^37. m = 7 and 15 have only +-1, although 26/73 and 43/168 are convergents. For m >= 19 the map is far outside the level-2 window, and no cycles were found.

mx + 1.
- m = 3: the cycles at -1, 1 and -5 sit at records. The cycle through -17 has n = 7 and V = 11. 11/7 is not a convergent of log2 3. It is an intermediate fraction, and a record of the one-sided problem 2^V < 3^n, which is the side forced for x < 0. So for maps with a sign constraint, the hypothesis needs one-sided best approximations, and those include intermediate fractions.
- m = 5: 13 and 17 at n = 3 (7/3, on the side 2^V > 5^n, as x > 0 requires).
- m = 181: 27 and 35 at n = 2 (15/2, the first partial quotient of log2 181 after 7 is 2, followed by 1621).
- Other m up to 31: nothing beyond the trivial cycles.

## 6. Attempts to break it

1. Dominant class not at a record: none found, in 54 census rules plus the families rules. The large secondary classes at multiples m q of the main record q are the closest thing. They break the literal statement that dominant lengths are best approximations, but rel(m q) is about m rel(q), so they come from the same coincidence.
2. Cycles at non-record lengths: common for single cycles. The extreme cases are 8 + 7i at level 6 (one cycle of n = 82 with rel 0.10 and E = 0.005, holding 79% of the basin; others at 84, 129 and 25), 5 + 2i at level 4 (n = 70, rel 0.07, E = 0.018) and 10 + 3i at level 6 (n = 70, E = 0.009). These are sporadic cycles. The record notion says nothing about them, and the heuristic says they are rare but expected somewhere among about 50 rules.
3. Records without cycles: common, and expected whenever E is small. That covers large n (13 at 227, 9 at 47, 53 and 153), the small-n records 13 at 7 (E = 0.76) and 9 at 5 (E = 0.72), and 15 at 290 (E = 0.72). Only two records with E >= 1 have no cycle: n = 2 for 6 + 5i at level 5 (E = 1.0) and n = 16 for one digit set of 9 + 6i (E = 1.3, while the other three digit sets have 2 to 3 cycles there). Both are consistent with Poisson fluctuation.
4. A modulus coincidence without the angle (Section 4.3): no cycles, as the simultaneous version predicts.
5. A sign constraint (Section 5, -17 for 3x + 1): the two-sided record notion misses this cycle, and the one-sided notion catches it.

## 7. Assessment

The idea holds partly, and in its correct form it is a corollary of the heuristic count rather than an independent principle. Large classes need a small rel at moderate n. A small rel at n that is also smaller than at every shorter length is what being a record means. So the main classes land at convergent denominators for real alpha and at best simultaneous approximations for complex alpha, with no exception in the data. The hypothesis is wrong as a two-way statement. Records with exponentially small branch count carry nothing, and sporadic single cycles occur at non-record lengths. The secondary classes at multiples of a record are not records. For integer maps with a sign constraint the right notion is one-sided best approximation.

As for novelty, for 3x+1 the use of convergents of log2 3 to constrain cycle lengths goes back to Crandall and Steiner and runs through Eliahou to Simons and de Weger. The empirical version tested here, that cycles concentrate at convergent lengths in rules with many cycles, with counts set by rel, does not appear in what could be read. It is an easy remark, and it may be stated in Lagarias 1990 or in Belaga and Mignotte, which were not read. It is worth a sentence in the paper's Section 5.6, attached to the existing remark on near-coincidences. It could also carry one prediction: cycles of 11 at level 6 at the convergent lengths 949 and 986.

## 8. Files and commands

From collatz/research:
- python3 convrun.py OUTDIR 1000000 runs families.py on the 54 census rules (2 minutes). The outputs went to the scratchpad and are not kept in the repository. The rule list is reproducible from the census logs.
- python3 convergents.py gauss 3000 FILES > logs/convergents_families.txt (families files at their final bounds) and > logs/convergents_census.txt (the 54 census files). Each takes about 1 second.
- python3 convergents.py modonly k ax,ay 600 > logs/convergents_modonly.txt.
- python3 convergents.py int sym|plus m 200000 3000 > logs/convergents_int_{sym,plus}.txt (seconds per m).
- python3 convergents.py cf 'log2(13)' prints continued fractions.
