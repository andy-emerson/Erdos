# Literature search: symmetric Collatz-type rules on Z, 5x ± 1 and 7x ± 1

Search done 5 November 2026 for Section 3.6 of `papers/gaussian-collatz-2.md`. It follows the Gaussian search in `literature.md`. The questions were:

1. Are 7x ± 1, 5x ± 1, or odd-symmetric generalized Collatz maps (with T(−x) = −T(x)) studied anywhere? Is the 5x ± 1 cycle 7, 9, 11 recorded? Is it recorded that 3x+1 on Z is not odd-symmetric, and that 3x−1 on the positive integers is its mirror image?
2. Has anyone proposed symmetry under the units as a natural requirement, or classified such rules by level?
3. What does the OEIS have?

The terms are those of the paper. A rule at level k with odd multiplier m sends an even x to x/2. It sends an odd x to (mx + d(x))/2^v, where the odd digit d(x) depends only on x mod 2^k and makes 2^k divide mx + d. The rule is symmetric if d(−x) = −d(x), and Collatz-like if 2^k < m < 2^(k+1). At level 2 the symmetric rules with digits ±1 are 5x ± 1 (digit −1 for x ≡ 1 mod 4) and 7x ± 1 (digit +1 for x ≡ 1 mod 4).

## 1. Method and access

All fetching went through WebSearch and WebFetch. The shell proxy refused oeis.org and arxiv.org (HTTP 403 on CONNECT) and could not resolve numbertheory.org, so nothing was downloaded. WebFetch only takes URLs that a search has returned. The consequences:

- OEIS A317640, the 7x ± 1 map itself, could not be fetched. Its content is known from the OEIS wiki page and from the cross-references in A317753, A318489 and A318972, which were read.
- OEIS's own search (oeis.org/search) is blocked by robots.txt for the fetch tool. So the term searches in Section 4 went through a general search engine, which is much weaker than OEIS's own search. A negative result there is weak evidence.
- Keith Matthews's pages on numbertheory.org redirect to themselves, as in the earlier search. These include "David Barina's 7x+1 conjecture" (linked from A317753, A318489 and A318972) and "A generalization of the 3x−1 mapping due to Lu Pei, locating a cycle" (php/Lu_Pei0.html). Neither was read. His chapter in *The Ultimate Challenge* (numbertheory.org/PDFS/matthews-final-revised.pdf) was reachable, and it was searched for the terms below.
- Lagarias's 1985 *Monthly* survey was read only in its HTML sections on cycles (node8) and generalizations (node11) at cecm.sfu.ca.
- Lagarias's overview (arXiv:2111.02635) was searched for keywords through WebFetch. The extraction may have been truncated.
- The other chapters of *The Ultimate Challenge* (Chamberland, Wirsching and the rest) were not seen. Nor were Wirsching's book, Hasse's 1975 lecture notes, the papers of Möller and Heppner, and Leigh's 1983 work.
- Barina's paper was read in full: the arXiv PDF, arXiv HTML v7 and the published PDF from fit.vut.cz.

## 2. The 7x ± 1 map is known

### 2.1 Barina (2018, published 2022)

D. Barina, 7x ± 1: Close Relative of the Collatz Problem. arXiv:1807.00908 (v1 2 July 2018, v7 1 October 2024). Published in *Computational Methods in Science and Technology* 28(4) (2022), 143-147, doi:10.12921/cmst.2022.0000025. URLs: https://arxiv.org/abs/1807.00908, https://www.fit.vut.cz/research/publication-file/12909/final.pdf. Read in full.

Barina defines f(n) = n/2 for even n, f(n) = 7n + 1 for n ≡ 1 (mod 4) and f(n) = 7n − 1 for n ≡ 3 (mod 4). He notes that 7n ± 1 is then divisible by 4. This is exactly the paper's 7x ± 1 rule at level 2. His results:

- A conjecture that every positive orbit reaches 1.
- The heuristic (1/2) log(7/4) + (1/2) log(1/2) < 0. This is the paper's window criterion at level 2 for m = 7.
- A computational verification of the conjecture for all n up to 10^15.
- Tables of record stopping times and peaks compared with 3x+1. Below 10^10 the longest orbit has 5717 steps, from 6,346,893,259.
- On the negative integers: "every progression in 7x ± 1 on negative numbers corresponds to negated progression on positive numbers, and vice versa". The only cycles are those through 1 and −1, and 0. He contrasts this with the three negative cycles of 3x+1 (through −1, −5 and −17).

He writes that, to his knowledge, the map "has never appeared before". He cites Crandall only for a different "7x+1" problem, and Matthews only as one of several generalizations.

How close. This is the paper's 7x ± 1 rule, its single cycle up to sign, its negation symmetry stated in words, and its average-contraction heuristic. His bound of 10^15 is eight orders of magnitude beyond the paper's 10^7. What Barina does not do:

- He does not derive the rule from a symmetry requirement, nor treat symmetry as a selection principle. It appears as a remark after the definition.
- He does not frame the rule by level, or relate it to the window 2^k < m < 2^(k+1).
- He does not consider 5x ± 1, or any other multiplier.
- He does not say that 3x+1 cannot be symmetric.

### 2.2 OEIS entries and the OEIS wiki

- A317640, "the 7x+-1 function", by D. Barina, 2018. Not fetched (Section 1); its definition is quoted on the wiki page and in A317753.
- A317753, number of steps to reach 1 in the 7x+-1 problem (total stopping time). D. Barina, 6 August 2018. https://oeis.org/A317753. Read.
- A318489, dropping time (glide) in the 7x+-1 problem. D. Barina, 27 August 2018. https://oeis.org/A318489. Read.
- A318972, the 7x+-1 function in shortcut form, (7n ± 1)/4 on odd n and n/2 on even n. D. Barina, 6 September 2018. https://oeis.org/A318972. Read. It has a formula by Chai Wah Wu: the shortcut sequence satisfies a linear recurrence.
- OEIS wiki, "7x±1 problem". https://oeis.org/wiki/7x%C2%B11_problem. Read. It states both conjectures, for the positive integers and for the negative integers (which reach the cycle of −1). It repeats the negation correspondence, and cites Barina and Matthews's page.

All three of A317753, A318489 and A318972 link to K. Matthews, "David Barina's 7x+1 conjecture", on numbertheory.org. That page was not reached [unverified: Matthews's discussion of the map not seen].

Unrelated 7x+1 entries, for completeness: A133421 and A133422 are the "7x+1" map that divides by 2, 3 and 5 (Oliveira e Silva's px+1 problem). A267703 is the 7x+1 map with no sign change. A174796 counts admissible sequences for 7x+1. None of these is symmetric.

### 2.3 Secondary mentions

F. R. Villatoro, "Los primos de la conjetura de Collatz", La Ciencia de la Mula Francis, 3 February 2020. https://francis.naukas.com/2020/02/03/los-primos-de-la-conjetura-de-collatz/. Read through WebFetch. It is a popular survey that lists Barina's 7x ± 1 conjecture next to 5x+1 (Roosendaal), 7x+1 and px+1, without discussing symmetry. I found no research paper that cites Barina and studies the map further [unverified: no citation index was reachable; copies on academia.edu were not opened].

## 3. Other symmetric maps on Z

### 3.1 Collatz's original permutation

The map g(n) = 2n/3 for n ≡ 0, (4n − 1)/3 for n ≡ 1 and (4n + 1)/3 for n ≡ 2 (mod 3) is Collatz's own problem from the 1930s, as usually told. It is odd. R. Tremblay, Generalized 3x + 1 Mappings: searching for cycles, arXiv:1907.13302, Section 2, equation (1), says so explicitly: "The cycles are the same with the negative integers because the function is odd, g(−n) = −g(n)." URL: https://arxiv.org/html/1907.13302. Read through WebFetch.

How close. This is an explicit statement that a classical Collatz-type map on Z is odd, and that its cycles on the negative integers are the mirror images of those on the positive ones. But g is a permutation with modulus 3 and no division by a power of 2, so it is not of the paper's form.

### 3.2 Conway's amusical permutation

OEIS A006368: a(2n) = 3n, a(4n+1) = 3n+1, a(4n−1) = 3n−1. https://oeis.org/A006368. Read. Its formula section records a(n) = −a(−n). The references are J. H. Conway, "On unsettleable arithmetical problems", *Amer. Math. Monthly* 120 (2013), 192-198, and "Unpredictable iterations" (1972).

How close. The odd branch is (3x ± 1)/4, with the sign chosen so that 4 divides the numerator. That is the paper's construction at level 2 with m = 3. This m lies below the window (3 < 4), and the even branch multiplies by 3/2 instead of halving. So the map is a symmetric relative, but not one of the paper's rules.

### 3.3 Lu Pei's map and Matthews's generalization by least absolute residues

According to Carnielli and Tremblay, Matthews generalized a map due to Lu Pei to

L_d(x) = x/d if d divides x, and L_d(x) = ((d+1)x − i)/d if x ≡ i (mod d), with −d/2 < i ≤ d/2 and i ≠ 0.

The case d = 3 is Lu Pei's map: x/3, or (4x ∓ 1)/3 for x ≡ ±1 (mod 3). Carnielli attributes the "balancing parity" idea to Matthews.

- W. Carnielli, Some natural generalizations of the Collatz problem, *Applied Mathematics E-Notes* 15 (2015), 207-215. URLs: https://www.math.nthu.edu.tw/~amen/2015/AMEN(150711).pdf and https://emis.de/ft/8276. Read through WebFetch, with the formulas partly garbled. Carnielli reports that trajectories "appear to meet 1 or −1, according as x is positive or negative". The extraction does not make clear for which d this is claimed. He gives a table of further cycles of L_d for 2 ≤ d ≤ 150 and x ≤ 50,000, which I did not see [unverified].
- Tremblay, arXiv:1907.13302, equation (43), states the same definition and attributes it to Matthews.
- K. R. Matthews, "A generalization of the 3x−1 mapping due to Lu Pei, locating a cycle (conjectured)", http://www.numbertheory.org/php/Lu_Pei0.html. Not reached. Lu Pei's own publication was not identified [unverified].

How close. Whenever the residue system is symmetric (d odd), L_d is odd, because the residue of −x is −i. Choosing the digit as the least absolute residue is exactly what produces symmetry, and it is the same mechanism as the paper's ±1 digits at level 2. So the idea of a balanced digit that makes a generalized Collatz map commute with x ↦ −x was in use, through Matthews, before 2015. None of the sources seen states symmetry as the reason for the choice, and the modulus is d rather than a power of 2.

My own check, not from any source (scratchpad `lupei.py`): every x with 1 ≤ x < 10^5 reaches 1 under L_3. Under L_5 and L_7 there are further cycles. In the paper's language, then, Lu Pei's L_3 is a symmetric, Collatz-like map (product of multipliers 16 < 3^3) that appears to have only the cycle of ±1. It is the same phenomenon as 7x ± 1, at modulus 3.

### 3.4 3x+1 on the negative integers and 3x−1

These facts are standard:

- Lagarias, The 3x+1 problem and its generalizations, *Amer. Math. Monthly* 92 (1985), 3-23, section "Are there non-trivial cycles?" (https://www.cecm.sfu.ca/organics/papers/lagarias/paper/html/node8.html, read). It says there are other cycles "if negative integers are allowed", starting from −1, −5 and −17, and reports the Böhm and Sontacchi conjecture that these, with 0 and 1, are all the cycles on Z.
- OEIS A037084 (positive integers not reaching 1 under 3n−1). https://oeis.org/A037084. Read. Comment by D. Kamenetsky, 12 January 2017: "Equivalent to the Collatz ('3n+1') problem for negative integers." Also Cloitre's conjecture of three cycles, with minima 1, 5 and 17.
- Encyclopaedia of Mathematics, "Syracuse problem". https://encyclopediaofmath.org/wiki/Syracuse_problem. Read. It notes the cycle −5 → −14 → −7 → −20 → −10 and the 3a−1 variant.
- Barina, Section 2.1, contrasts the cycles of 3x+1 on the negative integers with the mirror symmetry of 7x ± 1.

What I did not find: an explicit statement that no rule at level 1, and so no rule with multiplier 3 and division by 2, can commute with x ↦ −x because x ≡ −x (mod 2). This is a one-line observation. Its absence from the sources I could read says little.

### 3.5 Works checked that have no symmetric map

All of these were read through WebFetch, or seen in summary where noted:

- K. R. Matthews, Generalized 3x+1 mappings: Markov chains and ergodic theory, in *The Ultimate Challenge* (2010), 79-103. A keyword search found no "Lu Pei", "symmetric", "odd function", "negative" or "least absolute". Its modulus-4 examples (5.3, 5.6, 8.4) are not symmetric.
- MathWorld, "Matthews Map". https://mathworld.wolfram.com/MatthewsMap.html. Gives the general (m_i x − r_i)/d with r_i ≡ i m_i (mod d), and says nothing on symmetry.
- A. Edgington, The autoconjugacy of a generalized Collatz map, arXiv:1206.0553. Maps (mx + r)/2 with r fixed. Its autoconjugacy is a 2-adic involution, not x ↦ −x.
- R. Tremblay, arXiv:1910.11798 and 1909.00213. Maps with fixed or mixed constants. He cites Matthews for five cycles of 5x+1 (through 0, 1, 13, 17 and −1). No symmetric 5x ± 1.
- F. Briscese and F. Calogero, Conjectures analogous to the Collatz conjecture, *OCNMP* Special Issue 1 (2024), 1-5. https://ocnmp.episciences.org/12933/pdf. Maps of the form kn+1 with extra division rules; no symmetry.
- G. Goyal, preprints.org 202408.2050, and arXiv:2409.07929: mZ+1 with +1 only.
- Shrimali, Antu and Jones, arXiv:2305.01017: 5n+1 and 3n+5 on the positive integers.
- Random environment on coloured trees, arXiv:0707.1746: qx+1 with +1 only.
- Barina and Maat, 3n + 3^k problem, *CMST* 31 (2025), 67-70. https://cmst.eu/articles/3n-3k-problem/. Seen through the abstract page only.
- A. Bouhamidi, arXiv:2601.17573: admissible triplets, on N only.
- X. Ye, arXiv:2610.10660: quadratic maps on N₀.
- M. C. Siegel, arXiv:1909.09733: Hydra maps (pn+1)/2.
- Behajaina and Paran, arXiv:2312.00390. A polynomial-ring map f(x+1) − f₀. It commutes with multiplication by every constant, because the digit is the whole constant term, but the paper does not discuss this.

## 4. 5x ± 1 and the cycle 7, 9, 11

I found no source that defines the symmetric 5x ± 1 rule (5x − 1 for x ≡ 1 mod 4, 5x + 1 for x ≡ 3 mod 4), and none that records its cycle 7 → 36 → 18 → 9 → 44 → 22 → 11 → 56 → 28 → 14 → 7. The following searches gave nothing relevant:

- Searches for "5x±1", "5x+-1" and "5x+1 if" with "5x−1 if" and mod 4.
- A search for the literal orbit "7, 36, 18, 9, 44, 22, 11, 56, 28, 14".
- Searches for the map's value sequences in plain form (4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56) and in shortcut form (1, 1, 3, 9, 11, 7, 1, 19, 21).

Barina, the OEIS wiki page and the 5x+1 literature (Kontorovich and Lagarias, arXiv:0910.1944; Tremblay; Matthews) discuss 5x+1 with the fixed digit +1 only. Matthews's L_4 has multiplier 5 at modulus 4, but it divides multiples of 4 by 4 and sends x ≡ 2 (mod 4) to (5x − 2)/4, so it is a different map. Carnielli's table of L_d cycles, which I did not see, could not contain 7, 9, 11 for that reason.

Because the OEIS's own search could not be used (Section 1), this is the weakest negative in the report. A direct OEIS search for 1, 1, 3, 9, 11, 7, 1, 19, 21 or for 4, 1, 16, 2, 24, 3, 36, 4, 44, 5, 56 should be run before the paper says the rule is unrecorded.

## 5. Symmetry as a requirement, and classification by level

I found no source that proposes symmetry under the units (x ↦ −x in Z, or the unit group in a ring of integers) as a natural requirement for a Collatz-type conjecture. I also found no source that classifies symmetric rules by level, or by the power of 2 in the modulus. The sources that come closest:

- Barina (2.1) states the mirror property of 7x ± 1 as a consequence, and uses it to contrast with 3x+1. He does not present it as a criterion.
- Tremblay (3.1) states that Collatz's original permutation is odd, as a remark explaining its negative cycles.
- Matthews's least-absolute-residue maps L_d (3.3) are symmetric by construction. As far as the secondary sources show, the motivation is "balancing", not symmetry [unverified: Matthews's own page not read].
- The sources from the Gaussian search (`literature.md`, Section 2) do not raise it either. The Alvarado REU slides show units permuting the level-1 rules, not fixing them. Kucinski's symmetry results for Joseph's map are about the conjugacies of a fixed non-symmetric map.
- The Ultimate Challenge chapters other than Lagarias's overview and Matthews's chapter were not seen. Neither were Wirsching's book, Hasse's notes, Möller, Heppner or Leigh. This gap matters most for this question [unverified].

## 6. Verdict

What is known:

1. The rule 7x ± 1 is known. Barina introduced it in 2018 and published it in 2022, with the same definition, the conjecture that every positive orbit reaches 1, the remark that negative orbits are the negated positive ones, the cycles ±1 only, and the same heuristic. He verified it to 10^15, which supersedes the paper's 10^7. It has four OEIS entries (A317640, A317753, A318489, A318972), an OEIS wiki page, and a page by Matthews.
2. Odd-symmetric Collatz-type maps on Z are known. They include Collatz's original permutation (stated to be odd by Tremblay), Conway's amusical permutation (a(n) = −a(−n) in A006368), and Matthews's generalized Lu Pei maps with least absolute residues. The last are symmetric by the same balanced-digit mechanism as the paper's rules, and are reported (Carnielli) to send orbits to ±1 by sign.
3. That 3x+1 has extra cycles on the negative integers, and that 3x−1 on the positive integers is the same problem as 3x+1 on the negative integers, is standard (Lagarias 1985; OEIS A037084).

What appears new, with the caveats of Sections 1 and 4:

4. The 5x ± 1 rule and its second cycle 7, 9, 11. Not found, but the OEIS could not be searched directly.
5. The framing: symmetry under the units as a selection criterion, the window 2^k < m < 2^(k+1) as the definition of Collatz-like, and the observation that level 1 admits no symmetric rule. Under that framing, 5x ± 1 and 7x ± 1 are the two symmetric rules at level 2, and 7x ± 1 is the first with a single cycle up to sign. Each ingredient is elementary, and two of them are already in Barina in other words: the symmetry of 7x ± 1 and its heuristic. What I did not find is the criterion and the ordering.

Changes Section 3.6 of the paper needs:

- Cite Barina (arXiv:1807.00908; *CMST* 28(4) (2022), 143-147) as the source of 7x ± 1. Cite his verification to 10^15, and either drop the paper's own 10^7 claim or present it only as a check of the paper's code.
- Remove "Integer rules of this kind may already appear in the literature ... which we have not searched for them." Replace it with the citations above: Barina, Matthews's L_d via Carnielli and Tremblay, Collatz's permutation via Tremblay, and Conway's permutation.
- State that Barina already noted the mirror symmetry of 7x ± 1, and its contrast with the negative cycles of 3x+1.
- Present 5x ± 1 with its cycle 7, 9, 11 as not found in the literature, not as new, unless an OEIS search confirms it.
- Optionally mention Lu Pei's L_3 as a symmetric, Collatz-like map at modulus 3 that also appears to have only the cycle of ±1. This is my computation up to 10^5, not a cited result.
