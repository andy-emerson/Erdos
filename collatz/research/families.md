# Cycle families of the many-cycle rules (4 Nov)

Working notes on the minimal unit-equivariant rules with many cycles: at level 7, alpha = 13, 15, 11 +- 10i and 15 +- 2i; at level 6, alpha = 9 and 11 with all four minimal digit sets. Notation follows the draft (collatz/papers/gaussian-collatz.md, Section 2). S is the accelerated map on odd z, and a cycle is counted up to units. A cycle has n odd steps with valuations v_0, ..., v_{n-1}, total V. Its unit eps is defined by S^n(z_0) = eps z_0 (S commutes with units, so after n odd steps the orbit returns to an associate of its start). The data of a cycle is (n, V, eps). Nothing here is committed, and the paper is unchanged.

## 1. Summary

1. No infinite family was found. Within each rule, the cycles fall into a few classes (n, V, eps). These are exactly the triples for which eps (1+i)^V is unusually close to alpha^n. No affine or multiplicative map carries one cycle to another, apart from the mirror symmetry. The cycles do not lie near a line, and they share no common parity vector beyond the trivial fact that most valuations equal k.

2. The large counts come from near-coincidences of powers. The main ones are 13^10 = 1.00305 * 2^37, (11+10i)^33 within a relative 6.2 * 10^-4 of a unit times (1+i)^257, in modulus and angle together, 15^43 close to 2^168, (15+2i)^6 close to i (1+i)^47, 11^37 = 0.99929 * 2^128 and 9^6 = 1.0136 * 2^19. Each class (n, V, eps) contains only finitely many cycles. A heuristic count per class, explained in Section 3, matches the data to within a factor of about 1.4. Summed over all n, the heuristic converges, which suggests that each rule has finitely many cycles.

3. Proven (computer-assisted). Every cycle with data (n, V, eps) has an element whose norm obeys an explicit bound (Section 5). Combined with gcz and gstop runs, this gives the complete list of cycles up to the following lengths:
   - alpha = 13: 166 cycles with at most 216 odd steps;
   - alpha = 15: 74 cycles with at most 246 odd steps;
   - 11 +- 10i: 766 cycles with at most 2000 odd steps;
   - 15 +- 2i: 33 cycles with at most 1498 odd steps.

   In each case these are also all the cycles whose least norm is at most 10^9. In particular the counts are exact for the main classes: alpha = 13 has exactly 160 cycles of 10 odd steps, 11 + 10i has exactly 728 of 33, alpha = 15 has exactly 32 of 43, and 15 + 2i has exactly 28 of 6. Whether any of these rules has infinitely many cycles is not decided.

4. Even k. The many cycles of alpha = 11 and 9 at k = 6 have the same arithmetic origin, 11^37 close to 2^128 and 9^6 close to 2^19. They are not caused by the invariant lines: no cycle of 37 odd steps has an element on them. The escaping orbits do run along the lines. Section 6 adds a finding that bears on Section 8 of the draft. For every real alpha tested, at levels 3 to 7, the union L of the eight lines x = +-1, y = +-1, x - y = +-1, x + y = +-1 is mapped into itself by S, odd levels included. Whether orbits on L diverge is decided by a drift along L, computed from a two-state chain. That drift is positive exactly for the rules observed to diverge (alpha = 5 at k = 4, alpha = 9 and 11 at k = 6). It is negative for alpha = 3 (k = 3), 7 (k = 5), 13 and 15 (k = 7).

## 2. Data

Tools, all in collatz/research:
- families.py runs gcz non-quiet on one rule, groups the raw cycles up to units by exact iteration (exact.make_S), and writes one line per cycle. The fields are: the canonical element (the least-norm element of the cycle, first-quadrant associate x > 0, y >= 0), n, V, eps, the least and largest norm on the cycle, the basin size within the table, and the valuation vector read from the canonical element (an entry (10) means v = 10). The .json file also holds the S-orbit of the canonical element.
- gcz copy. For more than 4096 raw cycles, gcz needs a larger MAXC. A copy was built as `sed 's/#define MAXC 4096/#define MAXC 65536/' gcz.c > gczbig.c; gcc -O2 -o gczbig gczbig.c -lm` in the scratchpad. gcz.c itself is unchanged.

Level 7. The digit set is the unique minimal one, with generators G7 = 1, 3, 5, 7, 1 +- 2i, 1 +- 4i, 1 +- 6i, 2 +- 3i, 2 +- 5i, 3 +- 4i. The rules are given by the associates alpha = -13, -15, -11-10i (an associate of 11 + 10i), -11+10i and -15-2i, and the units eps below refer to these associates. Bounds were 10^6 and 10^7 for all, 1.3 * 10^7 for 13, 1.65 * 10^7 for 15 and 2.85 * 10^7 for 15 + 2i. These are the bounds that Section 5 requires. Count of cycles up to units:

| alpha | 10^5 (census) | 10^6 | 10^7 | final bound | least norm <= 10^9 (Section 5) |
|---|---|---|---|---|---|
| 13 | 141 | 166 | 166 | 166 at 1.3e7 | 166 |
| 15 | 64 | 72 | 72 | 74 at 1.65e7 | 74 |
| 11 +- 10i | 149 | 545 | 766 | | 766 |
| 15 +- 2i | | 33 | 33 | 33 at 2.85e7 | 33 |

Distribution of (n, V, eps), at the final bound:
- alpha = 13: (10, 74, -i) 160; (3, 22, -i) 2; (20, 148, -1) 2; the fixed class of 1, (1, 8); and (2, 14, i), through 1 + 2i.
- 11 + 10i: (33, 257, i) 728; (66, 514, -1) 29; (132, 1028, 1) 3; (99, 771, -i) 2; (198, 1542, -1) 1; (264, 2056, 1) 1; (1, 7) and (2, 16) one each. The mirror rule 11 - 10i has the mirror images, with the conjugate units.
- alpha = 15: (43, 336, -1) 32; (1, 8, -1) 16; (32, 250, -i) 7; (75, 586, i) 6; (247, 1930, i) 4; (107, 836, 1) 2; (11, 86, -i) 2; (8, 62, i) 2; (537, 4196, 1) 2; (10, 78, i) 1. The 16 fixed points come from the 16 digit generators: with alpha = -15 and z = -d, alpha z + d = 16 d = (1+i)^8 d, and d is the digit of its own class, so S(-d) = d = -(-d). So every digit gives a fixed point. The lengths 75 = 43 + 32, 107 = 75 + 32, 247 = 5 * 43 + 32 and 537 = 12 * 43 + 21 are sums of near-resonant lengths, as Section 3 explains.
- 15 + 2i: (6, 47, i) 28; (1, 8, -1) 3; (5, 39, -i) 1; (12, 94, -1) 1.

How the least norm grows. Number of cycles with least norm at most B:

| alpha | 10^2 | 10^3 | 10^4 | 10^5 | 10^6 | 10^7 | 10^8 |
|---|---|---|---|---|---|---|---|
| 13 | 2 | 6 | 30 | 127 | 166 | 166 | 166 |
| 15 | 20 | 29 | 40 | 62 | 72 | 72 | 74 |
| 15 + 2i | 5 | 21 | 33 | 33 | 33 | 33 | 33 |
| 11 + 10i | 2 | 3 | 6 | 66 | 395 | 765 | 766 |
| 9 (k = 6, set A) | 9 | 13 | 23 | 23 | 23 | | |
| 11 (k = 6, set A) | 8 | 11 | 26 | 98 | 445 | 568 | |

The counts saturate. For 13 the largest least norm is 2.7 * 10^5. For 11 + 10i it is 1.2 * 10^7, against an a priori bound of 6.3 * 10^8 (Section 5). For alpha = 15 the last two cycles found form the mirror pair of 537 odd steps through 4081 + 4664i and 4664 + 4081i, with least norm 3.84 * 10^7.

Angles. In every rule the canonical elements and the cycle elements are spread over all angles. In 10-degree bins mod 90, the elements of the 33-cycles of 11 + 10i have counts between 2599 and 2715, and those of the 10-cycles of 13 between 156 and 188. The share of elements within relative distance 0.02 of an axis or diagonal is 0.049 (13), 0.054 (15), 0.065 (15 + 2i) and 0.051 (11 + 10i), against 0.051 for uniformly distributed angles. alpha = 15 is the one exception to a uniform picture: its relative distances bunch near 0.066 (about 4 degrees), because the long cycles have many elements in a few directions. Even there, no cycle lies near a fixed line in absolute terms (median absolute distance 352).

Level 6. Digit sets: A = 1, 3, 1 +- 2i, 2 +- 3i, 1 - 4i, 3 - 4i; B = A with 3 + 4i in place of 3 - 4i; C = A with 1 + 4i in place of 1 - 4i; D = A with both 1 + 4i and 3 + 4i. These are the four sets of the census, and A and D, B and C are mirror pairs.
- alpha = 9: 23 (A, D) or 25 (B, C) cycles at 10^6. The classes are (6, 38, i) with 15 or 17 cycles and (1, 6, -i) with 8 fixed points: S(d) = (-9 d + d)/(1+i)^6 = -8d/(-8i) = -i d for every digit d.
- alpha = 11: 568 (A, D) or 560 (B, C) cycles at 10^6. The classes are (37, 256, -1) with 498 or 481; (74, 512, 1) with 46 or 51; (2, 14, i) with 7; then a few each of (13, 90), (111, 768), (148, 1024), (11, 76), (185, 1280), (222, 1536) and (209, 1446).
- A gstop pass over least norms in (10^6, 3 * 10^6] for alpha = 11 with set A found 172 cycles. 101 of them were already in the 10^6 list, being reached from smaller starts. So set A has at least 639 cycles. A complete pass to the bound of Section 5 (5.6 * 10^8 for n = 37) is out of reach with gstop. Many starts fall into cycles that lie entirely above their own norm, and each of those runs to the step cap, so the rate drops to about 13000 starts per second.

All cycles in all these files pass the exact check of Section 5 (famverify.py).

## 3. The cycle equation and a heuristic count

Write z_{j+1} = (alpha z_j + d_j)/(1+i)^{v_j}, and s_j = v_j + ... + v_{n-1}. Then n odd steps give

  S^n(z_0) = alpha^n z_0 / (1+i)^V + sum_j d_j alpha^{n-1-j} / (1+i)^{s_j},

so a cycle with data (n, V, eps) satisfies

  z_0 (eps (1+i)^V - alpha^n) = sum_j d_j alpha^{n-1-j} (1+i)^{V - s_j}.     (1)

The denominator den = eps (1+i)^V - alpha^n is odd. Call r = |alpha|^n / 2^{V/2} the modulus ratio and rel = |1 - alpha^n / (eps (1+i)^V)| the relative denominator, so that N(den) = 2^V rel^2. The right-hand side of (1) is a sum of n terms of size about |alpha|^{n-1} 2^{-s_j/2} 2^{V/2}. Its size relative to 2^{V/2} is bounded unless the walk of valuations runs far behind. So |z_0| is of order 1/rel: large cycles need a small relative denominator.

Branch count. The odd (1+i)-adic integers z for which the first n steps have valuations v_0, ..., v_{n-1} form 2^{(k-1)n} residue classes, one for each choice of odd class mod (1+i)^k at each step. On such a class, S^n is affine and onto the odd (1+i)-adics. So each class contains exactly one point with S^n(z) = eps z, namely the rational point z_0 = c/den of (1). That point is a Gaussian integer exactly when den divides c. Treat this divisibility as an event of probability 1/N(den). Each cycle up to units accounts for 4n such points (n starting points, 4 associates). The number of valuation vectors with n entries at least k and sum V is C(V - kn + n - 1, n - 1). The expected number of cycles up to units with data (n, V, eps) is then

  E(n, V, eps) = 2^{(k-1)n} C(V - kn + n - 1, n - 1) / (4 n N(den)).

Heuristic against exact counts, with the exact counts from Section 5 (famtheory.py, logs/families_theory_classes.txt):

| rule | class | rel | E | exact |
|---|---|---|---|---|
| 13 | (10, 74, -i) | 3.05e-3 | 117 | 160 |
| 13 | (20, 148, -1) | 6.11e-3 | 2.8 | 2 |
| 11 + 10i | (33, 257, i) | 6.17e-4 | 764 | 728 |
| 11 + 10i | (66, 514, -1) | 1.23e-3 | 44.6 | 29 |
| 11 + 10i | (99, 771, -i) | 1.85e-3 | 7.1 | 2 |
| 11 + 10i | (132, 1028, 1) | 2.47e-3 | 1.7 | 3 |
| 15 | (43, 336, -1) | 2.56e-3 | 29.3 | 32 |
| 15 | (32, 250, -i) | 1.43e-2 | 1.6 | 7 |
| 15 + 2i | (6, 47, i) | 1.58e-2 | 20.5 | 28 |
| 11, k = 6 | (37, 256, -1) | 7.1e-4 | 613 | 498 (A), 481 (B) at 10^6 |
| 9, k = 6 | (6, 38, i) | 1.36e-2 | 18.4 | 15 (A), 17 (B) |

The fixed points of 15 at k = 7 and of 9 at k = 6 are structural (every digit is one). The heuristic gives 16 and 8 for them, which here is exact.

Why the classes are where they are. rel is small only when two things hold at once: the modulus ratio r is close to 1, so V/n is close to log2 N(alpha), and the angle n arg(alpha) - V pi/4 - arg(eps) is close to 0. For real alpha the angle condition is automatic for V even with the right eps. Then only one number, log2 alpha, has to be approximated by V/(2n). The relevant n are denominators of convergents and intermediate fractions of log2 13 = [3; 1, 2, 2, 1, 22, ...] (n = 10, then 227) and of log2 15. For non-real alpha both conditions must hold. For 11 + 10i, n = 33 satisfies both to about 6 * 10^-4, which is unusually good (the generic two-dimensional Dirichlet bound is of order n^{-1/2}). That is why this rule has the most cycles. The classes at multiples m n_0 have rel about m rel_0 while the branch count falls, so E decreases along them (764, 44.6, 7.1, 1.7, 0.5, ...).

Totals (famheur.py, logs/families_heur.txt). Summing E over all n up to 20000, all V within 3 of n log2 N(alpha), and all eps gives 129 (13), 70 (15), 30.5 (15 + 2i), 825 (11 + 10i), 31 (9 at k = 6) and 721 (11 at k = 6). The partial sums are constant from n = 1000 on (from n = 3000 on for 11 at k = 6). The convergence comes from large deviations. V/n must stay near log2 N(alpha), which lies below the mean valuation k + 1, and the probability of such valuation vectors decays exponentially in n. This decay beats the polynomial gain from Dirichlet approximation. So the heuristic predicts finitely many cycles for every one of these rules, with totals close to the counts found. It is a heuristic, not a proof.

## 4. Structure tests

famstruct.py tests each rule's cycle file (output in logs/families_k7_*_struct.txt).

1. Affine maps. For every pair of cycles in the same class, it tests whether w_j = a z_{j+r} + b holds exactly for all j, for some a, b in Q(i) and some rotation r. The test is run on the raw S-orbits and on their first-quadrant associates, each with and without complex conjugation of the second cycle. The associate version is needed because conjugation changes the unit factor at each division: conj(w/(1+i)) = i conj(w)/(1+i). The only maps found are conjugation composed with a unit, on the associate-normalised orbits. These are the mirror pairs that exist for real alpha because D is closed under conjugation. For 13: 79 mirror pairs among the 160 cycles of 10 steps, and 2 self-mirror cycles. For 15: 16 pairs in the 43-class, and the other classes consist of mirror pairs and a few self-mirror cycles. 11 + 10i and 15 + 2i have no symmetry of their own (their mirrors belong to the conjugate rules) and no affine relations. No map with |a| != 1 or b != 0 occurs.

2. Constant differences. Sort the cycles of a class by least norm. No two consecutive differences of canonical elements are equal. The most common pairwise difference among the 728 canonical elements of 11 + 10i occurs 6 times, and for 13 it occurs 8 times. That is consistent with chance together with the mirror and sign symmetries of the list.

3. Valuation vectors. The valuations are mostly k, with an excess V - kn of 4 in 10 entries for 13 and 26 in 33 entries for 11 + 10i. For 13 the 160 cycles of 10 steps use 35 distinct necklaces (vectors up to rotation), with multiplicities 2 to 14. Several cycles share a valuation necklace and differ in their digits. Two necklaces differ in 2 to 5 positions, after the best rotation. For 11 + 10i all 728 necklaces are distinct. A family with "a fixed parity vector except for a few entries" would therefore not be a family in any useful sense: changing one valuation changes the branch, and the new branch has an integral fixed point with probability about 1/N(den), as in Section 3.

4. Composite classes. For 11 + 10i, 18 of the 29 cycles of 66 steps have a rotation that splits their valuations into two blocks of 33 with sum 257 each. These cycles pass twice near the 33-step resonance. The other 11, and the cycles of 99, 132, 198 and 264 steps, have block sums that wander (for example 251, 257, 263, 257). For 13, the two 20-cycles split as 73 + 75, not 74 + 74. For 15, the 75-cycles contain one block of 43 with sum 336. This is the same composition of resonances that the lengths 75 = 43 + 32 and so on show. It is not a family.

5. Lines. Section 2 gives the angle data. Section 6 treats the eight lines L. At level 7, L is invariant only for real alpha. For real alpha, a cycle meets L either in all its elements or in none; for 11 + 10i, 37 of the 728 cycles of 33 steps have some elements on L, by chance. For 13, 6 of the 160 cycles of 10 steps lie entirely on L; for 15, 2 of the 8-cycles and 2 of the 32-cycles; for 11 + 10i, only the 2-cycle through 1 + 2i lies on L (logs/families_lines.txt).

Conclusion of the tests: no pattern of the kinds asked for (affine map, fixed difference, line, shared parity vector) holds beyond mirror symmetry. The organising principle is arithmetic, the small denominators of (1).

## 5. What is proven: complete lists up to a length

Lemma 1 (bound on one element). Let a cycle have data (n, V, eps). Put g_i = ln|alpha| - (v_i/2) ln 2, ln r = sum_i g_i, c = max(0, ln r). Then some element z of the cycle satisfies

  |z| <= dmax * |alpha|^{-1} * sum_{m=1}^{n} exp(G_m) / rel,   G_m = min(m (ln|alpha| - (k/2) ln 2), c) for m < n,  G_n = ln r,

where dmax = max |d| over the digit set: 7 at level 7, 5 at level 6.

Proof. By (1), for the element at any rotation,

  |z_0| rel <= sum_j |d_j| |alpha|^{n-1-j} 2^{-s_j/2} = |alpha|^{-1} sum_{m=1}^{n} |d_{n-m}| exp(G'_m),

where G'_m is the sum of the last m of the g_i. Choose the rotation that starts at an index p where the prefix sum g_0 + ... + g_{p-1} is least (p = 0 included). In that rotation every suffix sum G'_m is at most max(ln r, 0) = c. This is the cycle lemma: a suffix sum is either the total minus a non-negative amount, or a wrapped sum that is at most 0. Also G'_m <= m (ln|alpha| - (k/2) ln 2), because every v_i >= k, and G'_n = ln r. []

Lemma 2 (all V at once). If |V - n log2 N(alpha)| >= 3, then r >= 2^{1.5} or r <= 2^{-1.5}. Since rel >= |1 - r| and the sum in Lemma 1 is at most n max(1, r), the bound is then at most 1.55 dmax n / |alpha|. So for each n only six values of V and four units need to be evaluated. famtheory.maxbound_one does this, and famcover.py tabulates it (logs/families_cover.txt).

Coverage. The bound of Lemma 1 on N(z), maximised over V and eps for each n, stays at most T for every n <= N:

| rule | T | N | first n beyond |
|---|---|---|---|
| 13 | 1.3e7 | 156 | 157 (1.6e7) |
| 13 | 1e9 | 216 | 217 (1.4e9) |
| 15 | 1e9 | 246 | 247 (7.1e9) |
| 15 + 2i | 2.85e7 | 982 | 983 (3.2e7) |
| 15 + 2i | 1e9 | 1498 | 1499 (1.9e9) |
| 11 + 10i | 1e9 | 2000 (end of the table) | none up to 2000; largest 6.4e8 |
| 11, k = 6 | 1e9 | 578 | 579 (1.2e9) |

For 11 + 10i, the bound at n = 33 m is nearly constant in m, about 6.3 * 10^8, because rel grows like m while the sum grows like n. Every cycle in the data satisfies Lemma 1. The ratio of least norm to bound is at most 0.29 (13), 0.27 (11 + 10i) and 0.51 (15 + 2i). For 15 it reaches 1.0, at the fixed point 7, where the bound is attained.

Searches. gcz classifies every start with N(z) <= B, so it finds every cycle that has an element of norm at most B. gstop, run over (B, 10^9], records each cycle whose least element has norm in that range: the orbit of the least element never drops below its own norm and returns to an associate of it. Results (logs/families_gstop_k7_*_1e9.txt, summary in logs/families_gstop_k7_summary_1e9.txt):
- 13 over (1.3e7, 1e9]: 387593990 starts, no failure, no cycle; 128 s.
- 11 + 10i over (1e7, 1e9]: 388772062 starts, no failure, one cycle with least element 2318 + 2599i (norm 1.21e7). It is already in the gcz list, reached from a smaller start; 329 s.
- 15 over (1.65e7, 1e9]: 386219564 starts, 8 starts at the step cap, one cycle-min pair 4081 + 4664i and 4664 + 4081i (the 537-step mirror pair, already in the 1.65e7 list); 370 s. The 8 capped starts (311 + 4664i, 326 + 4891i, 359 + 5382i, 2679 + 3062i and their mirrors) were iterated in exact arithmetic. Each reaches the 537-step pair after 1 to 21 odd steps without first dropping below its own norm.
- 15 + 2i over (2.85e7, 1e9]: 381507118 starts, no failure, no cycle; 419 s.

Theorem (computer-assisted). For the level-7 rules with the minimal digit set:
1. alpha = 13 has exactly 166 cycles up to units with least norm at most 10^9, and these include every cycle with at most 216 odd steps. Exactly 160 cycles have 10 odd steps.
2. alpha = 15 has exactly 74 such cycles, including every cycle with at most 246 odd steps; exactly 32 have 43 odd steps.
3. alpha = 11 + 10i (and, by conjugation, 11 - 10i) has exactly 766, including every cycle with at most 2000 odd steps. Exactly 728 have 33 odd steps, 29 have 66, 2 have 99 and 3 have 132.
4. alpha = 15 + 2i (and 15 - 2i) has exactly 33, including every cycle with at most 1498 odd steps; exactly 28 have 6 odd steps.

The proof consists of Lemmas 1 and 2, the gcz runs at the bounds of Section 2 and the gstop runs above. It relies on the correctness of gcz and gstop (128-bit arithmetic, escape at 2^100 and 2^110 coordinates, never reached on these rules except where stated) and on double-precision evaluation of the bound. Two facts guard the latter. The bound is used with a factor of at least 1.4 to spare in every case where it decides coverage (at 1e9 the deciding bounds are at most 7e8). And rel is confirmed in exact integer arithmetic for the main classes (famverify.py prints N(den)/2^V = rel^2: 9.318e-6 for (10, 74) of 13, 3.808e-7 for (33, 257) of 11 + 10i, 6.576e-6 for (43, 336) of 15 and 2.503e-4 for (6, 47) of 15 + 2i).

Exact verification of the cycles (famverify.py). For every cycle listed, with the digit recovered as d_j = (1+i)^{v_j} z_{j+1} - alpha z_j, exact Gaussian-integer arithmetic confirms four things. First, d_j lies in D and (1+i)^k divides alpha z_j + d_j. D has one element in each odd class mod (1+i)^k, so this is the digit the rule picks, justified by the residue. Second, z_{j+1} is odd. Third, (1) holds. Fourth, the n points are pairwise non-associate. Result: 166, 74, 33, 766, 4 * 23-25 and 4 * 560-568 cycles verified, none failed.

What is not proven. That any of these rules has finitely many cycles, or infinitely many. A cycle missing from the lists would need more odd steps than the lengths above and a least norm above 10^9. The heuristic of Section 3 gives such cycles a small total expectation (for alpha = 15 the heuristic expects about 1.4 cycles with n > 300 in total, and the 537-step pair shows that such cycles do occur; for the other rules the expectation beyond n = 300 is below 0.1, except 11 at k = 6 with about 13). Proving finiteness would need an effective lower bound on |eps (1+i)^V - alpha^n| / 2^{V/2} for all large n. That is a linear form in two logarithms, or in a logarithm and an angle for non-real alpha. A bound of Baker type, together with Lemma 1, would turn every check here into a finite one, as Simons and de Weger do for 3x+1. That has not been attempted.

No explicit infinite family exists to verify symbolically, because none was found. Section 3 explains why a parametrised family with a fixed number of odd steps cannot exist: each class (n, V, eps) is finite by Lemma 1. A family with growing n would need rel_n of order n * |alpha|^{-1} / |z|, for |z| growing along the family. The classes at multiples of a resonant length have rel growing linearly, which is consistent with bounded |z|, but their counts fall off exactly as the heuristic predicts (764, 29, 2, 3, 0, 1, 1 at m = 1 to 8 for 11 + 10i).

## 6. Even levels, the invariant lines, and a correction to the divergence criterion

Escaping orbits at k = 6. Random starts with |x|, |y| <= 1000 were iterated until N(z) > 2^120 (famescape.py, logs/families_k6_escape.txt). For alpha = 9, 13 to 29 of about 150 starts per digit set escaped. For alpha = 11, 55 to 69 of about 150 escaped. Every escaped orbit ended on one of the lines x - y = +-1, x + y = +-1, x = +-1 or y = +-1, all but 13 of them on the diagonal ones, with valuations 6, 7 and 8 only.

The cycles at k = 6 are not on these lines. For alpha = 11, none of the 498 (set A) or 481 (set B) cycles of 37 steps has an element on L. In each rule only 6 or 19 of the about 23500 cycle elements lie on L. They belong to the three 2-cycles that lie on L and, for sets B and C, one 13-cycle. For alpha = 9, 4 (A, D) or 6 (B, C) of the 15 or 17 cycles of 6 steps lie entirely on L, and the rest have no element there. So the many cycles of alpha = 11 come from the resonance 11^37 close to 2^128 of Section 3, and the lines carry the divergence, not the cycles.

Proposition 3 (the eight lines). Let alpha be real, and take the rules alpha = 3 (k = 3), 5 (k = 4, both minimal digit sets), 7 (k = 5), 9 and 11 (k = 6, all four sets), 13 and 15 (k = 7). For each of them, S maps L = {x = +-1} u {y = +-1} u {x - y = +-1} u {x + y = +-1} into itself, and every odd step from L has valuation k, k + 1 or k + 2.

Proof. A point of L is u + t w with u a unit, w in {1, 1+i} up to units, and t an integer. Then q = alpha z + d = alpha t w + (alpha u + d). Since alpha is real, the first term lies on the line R w, so the transverse coordinate of q with respect to w equals that of delta = alpha u + d. The digit d depends only on t modulo a power of 2 (it is fixed by z mod (1+i)^k), so delta is constant on residue classes of t. If (1+i)^v divides q, then 2^{floor(v/2)} divides the transverse coordinate of delta. On a class where that coordinate is nonzero, v is therefore bounded, and v itself is fixed by t modulo a further power of 2. Then S(z) = t alpha w/(1+i)^v + delta/(1+i)^v lies on a line through 0 in an axis or diagonal direction, shifted by an offset that depends only on (delta, v). So whether S(z) lies in L depends only on t modulo a fixed power of 2. famline1d.py checks every t in a window of 2^16 (and 2^18) consecutive integers starting at 10^12, for the lines y = +-1 and x - y = -+1. The other six lines are their images under units, and S commutes with units. In the window every point maps into L, and the valuations are k, k + 1 or k + 2. These valuations are bounded on every residue class, so no class has transverse coordinate 0 (on such a class v would grow without bound along t), and the window covers a full period. []

So the line mechanism of Section 8 of the draft is not special to even k. At odd k, (1+i)^k has a 45-degree phase, and an odd step with odd valuation turns an axis-type line (x or y = +-1) into a diagonal one, and back. The union L is still invariant. The draft reports that 876 of 1000 sample points on the diagonal lines map back onto them for alpha = 5; with the four axis-type lines included, every point maps into L.

Drift along L. On L, S acts as a one-dimensional map, and the log of |z| changes by (1/2)(log2 N(alpha) - v) bits per odd step. Two state types, A (axis) and Dg (diagonal), alternate according to the parity of v. famline1d.py measures, over a full period of t, the mean valuation on each type and the transition probabilities between types. It then takes the stationary distribution of this two-state chain (assuming the residues of t are equidistributed along an orbit, which is the usual heuristic):

| rule | mean v on A | mean v on Dg | stationary share of A | mean v | log2 N(alpha) | drift (bits per odd step) | observed |
|---|---|---|---|---|---|---|---|
| 3, k = 3 | 3.750 | 3.500 | 0.400 | 3.600 | 3.170 | -0.215 | converges |
| 5, k = 4 | 4.500 | 4.375 | 0.200 | 4.400 | 4.644 | +0.122 | diverges |
| 7, k = 5 | 6.125 | 5.500 | 0.444 | 5.778 | 5.615 | -0.082 | converges |
| 9, k = 6 | 6.500 | 6.188 | 0.111 | 6.222 | 6.340 | +0.059 | diverges |
| 11, k = 6 | 6.500 | 6.563 | 0.273 | 6.545 | 6.919 | +0.187 | diverges |
| 13, k = 7 | 7.938 | 7.500 | 0.421 | 7.684 | 7.401 | -0.142 | converges |
| 15, k = 7 | 8.313 | 7.500 | 0.471 | 7.882 | 7.814 | -0.034 | converges |

The values are the same for all digit sets of a rule. The sign of the drift agrees with the observed behaviour in every case. alpha = 15 at k = 7 is the instructive one. Averaged uniformly over L its drift would be +0.021, but the chain spends only 0.47 of its time on axis-type lines, where valuations are larger, and its drift is -0.034. Exact iteration agrees: 300 starts on L with m up to 2 * 10^9 all returned to cycles, in most cases the cycle whose first-quadrant point is 7 + 8i. 200 starts on L for alpha = 9 at k = 6 all escaped (famlinerun.py). In the draft's terms, "lambda real up to a unit" is the right condition only at even k. At odd k the invariant set L still exists for real alpha, and what decides divergence is the sign of the drift along L. For the level-7 rules this sign happens to be negative, which explains why level 7 has no divergence although alpha = 13 and 15 are real. This is tested, not proven: the drift relies on equidistribution of residues along orbits on L, as for 3x+1 itself.

## 7. What this adds to the census

1. The level-7 counts in the draft (166, 72, 545 at 10^6) are not just lower bounds that grow without end. With the bound of Lemma 1 they become exact statements: 166, 74, 766 and 33 cycles with least norm at most 10^9, complete for all cycles up to 216, 246, 2000 and 1498 odd steps. The count of 72 for alpha = 15 should become 74, and 545 for 11 +- 10i should become 766.
2. The repeated lengths (33 for 11 + 10i, 10 for 13, 43 for 15, 6 for 15 + 2i, 37 for 11 at k = 6) are explained by near-coincidences alpha^n close to eps (1+i)^V. The counts per length match the heuristic E of Section 3.
3. The rules with many cycles are not candidates for infinitely many cycles in any explicit way. The heuristic predicts finite totals close to the counts found.
4. For Section 8: the eight lines L are invariant for real alpha at odd levels too. Divergence is predicted by the sign of a two-state drift along L, which separates 5, 9, 11 from 3, 7, 13, 15. The level-6 counts for alpha = 11 (560, 568 at 10^6) are lower bounds that keep growing (at least 639 for set A below least norm 3 * 10^6). The heuristic total is about 720.

## 8. Commands and files

From collatz/research. G7 is "0,1;0,3;0,5;0,7;1,2;1,4;1,6;1,-6;1,-4;1,-2;2,3;2,5;2,-5;2,-3;3,4;3,-4" and GB is the scratchpad gczbig of Section 2. Times are on this machine, one job at a time.

- python3 families.py 7 -13,0 "$G7" B logs/families_k7_a13_X --gcz $GB for B = 10^6, 10^7, 1.3 * 10^7 (X = 1e6, 1e7, 1.3e7; 18 and 22 s at the larger bounds), and the same for -15,0 (a15; 1.65e7 in 62 s), -11,-10 (a11p10i), -11,10 (a11m10i, 10^6 only) and -15,-2 (a15p2i; 2.85e7 in 122 s).
- python3 families.py 6 -9,0 "<set>" 1000000 logs/families_k6_a9_<S>_1e6 --gcz $GB for the four sets S = A, B, C, D, and the same for -11,0 (all eight in 69 s).
- ./gstop <rule> B1 1000000000 > logs/families_gstop_k7_<rule>_1e9.txt with rule files written by rules.write for alpha = -13, -11-10i, -15, -15-2i (in the scratchpad) and B1 = 1.3e7, 1e7, 1.65e7, 2.85e7 (128, 329, 370, 419 s).
- ./gstop k6_a11_A.rule 1000000 3000000 > logs/families_gstop_k6_a11_A_1e6_3e6.txt (62 s).
- python3 famtheory.py k alpha dmax^2 nmax Emin > logs/families_theory_classes.txt (classes, E, rel and the worst-case bound on all elements; the functions bound_one and maxbound_one hold Lemmas 1 and 2).
- python3 famcover.py > logs/families_cover.txt (20 s); python3 famheur.py > logs/families_heur.txt (about 2 min).
- python3 famstruct.py logs/families_k7_<rule>.json > logs/families_k7_<rule>_struct.txt (seconds, about 8 min for 11 + 10i).
- python3 famverify.py logs/families_*.json (exact check of every cycle).
- python3 famlines.py ... > logs/families_lines.txt; python3 famline1d.py k alpha set 10^12 65536 > logs/families_line_drift.txt; python3 famescape.py > logs/families_k6_escape.txt; python3 famlinerun.py 7 -15,0 "$G7" m0 trials (m0 = 10^3 and 10^9) and 6 -9,0 (set A).

Data files in logs/: families_k7_{a13,a15,a11p10i,a11m10i,a15p2i}_{1e6,1e7}.{txt,json}, families_k7_a13_1.3e7, families_k7_a15_1.65e7, families_k7_a15p2i_2.85e7, families_k6_{a9,a11}_{A,B,C,D}_1e6, the *_struct.txt files, families_gstop_*, families_theory_classes.txt, families_cover.txt, families_heur.txt, families_lines.txt, families_line_drift.txt and families_k6_escape.txt.
