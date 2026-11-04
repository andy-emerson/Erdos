# Literature search: Collatz maps on the Gaussian integers

Search done 4 November 2026 for the draft `papers/gaussian-collatz.md`. It covers what was looked at, what each relevant work does, how it bears on the draft's claims 1 to 5, and what could not be reached. Claims are numbered as in the request:

1. The family T (divide by 1+i on evens, αz + d(z) on odds, digit set D of 2^(k-1) odd residues modulo (1+i)^k) and the accelerated map S.
2. The window 2^k < N(α) < 2^(k+1); no rule at level 1 (3 is not a norm); no rule at levels 1 or 2 commutes with units.
3. The rule H (α = 5+2i, k = 4, digits ±1, ±i, ±(2+i), ±(1-2i)) and the conjecture that it has a single cycle up to units.
4. The classification of unit-symmetric rules at levels 3 to 6, G_3, and Proposition 5 (no α = 3 rule both agrees with 3x+1 on the positive integers and contracts).
5. Joseph's map and anything like it.

## 1. Method and access

All fetching went through WebFetch and WebSearch. The shell proxy refused every outside host (arXiv, Semantic Scholar, umich, numbertheory.org, OEIS, MathOverflow, zenodo, Google Scholar: HTTP 403 on CONNECT). So nothing was downloaded, and nothing could be grepped locally. WebFetch turns a PDF into text and then cuts it off after roughly 80,000 to 86,000 characters. The consequences for the main sources:

- Lagarias, bibliography I (arXiv:math/0309224). The current version was read up to entry 76 (Guy), and version 7 up to entry 100 (Margenstern and Matiyasevich). Entries from about Ma to Z were not read [unverified: entries M to Z of bibliography I not seen].
- Lagarias, bibliography II (arXiv:math/0608208). Version 2 (72 entries, Akin to Zarnowski) was read in full. The current version was read up to entry 89 (Rouet and Feix). Entries added after version 2 that come alphabetically after Rouet were not seen.
- Matthews's chapter in *The Ultimate Challenge*. Read through WebFetch from his own PDF (sections 3, 9, 10, 11 and the bibliography). Some quotations are short fragments, because of the tool's quotation limit.
- Matthews's 3x+1 index page (numbertheory.org/3x+1/). Not reachable: it redirects to itself.
- Garg's Zenodo record. Not reachable (blocked), so still seen only through a summary.
- A Google Docs PDF that came up for "Minnesota Journal of Undergraduate Mathematics Collatz Gaussian". Not reachable (its redirect target is blocked). It may be the Alvarado group's paper, or may be unrelated.
- *The Ultimate Challenge* as a book. Not seen beyond Lagarias's overview (arXiv:2111.02635) and Matthews's chapter. Chamberland's chapter was seen only through his earlier survey, listed below.
- Wirsching's book. Not seen (see 2.12).

Searches were run on arXiv, general web, MathOverflow, Math StackExchange, the OEIS wiki, easychair, preprints.org, Zenodo and university pages. They used the terms Gaussian integers, Z[i], 1+i, Eisenstein integers, number fields, Z^2, lattices, matrix multipliers, (1+i)-adic, Z_2[i], polynomial rings and RCWA, combined with Collatz and 3x+1. MathOverflow and Math StackExchange produced no relevant question in any search result. That only means the search engine did not surface one, not that none exists.

## 2. Relevant works

### 2.1 Joseph (1998)

J. A. Joseph, A chaotic extension of the 3x+1 function to Z_2[i], *Fibonacci Quarterly* 36(4) (1998), 309-316. URL: https://www.mathstat.dal.ca/FQ/Scanned/36-4/joseph.pdf. Read through WebFetch (OCR text, some formulas garbled).

The map is defined on the 2-adic Gaussian integers. It sends z to z/2 on the class 0 modulo 2, and to (3z+1)/2, (3z+i)/2 or (3z+1+i)/2 on the classes 1, i and 1+i. Joseph proves four things:

- T restricted to Z_2 is the 3x+1 map.
- T is not conjugate to T × T by a Z_2-module isomorphism, but it is topologically conjugate to T × T.
- The parity vector map is periodic, and its limit is an isometric homeomorphism.
- T is chaotic in the sense of Devaney.

There is no division by 1+i, the only multiplier is 3, and no orbits in Z[i] are computed.

Relation to the claims. This work is different for claims 1 to 4: it divides by 2 and has a single fixed multiplier. For claim 5, the draft's description is accurate. The draft gives the page range as 309-317. The paper's own headers, Kionke and Chamberland all give 309-316.

### 2.2 Kucinski (2000)

Gina I. Kucinski, Cycles for the 3x+1 map on the Gaussian integers, preprint, University of Scranton, May 2000, 12 pp. The work was done under K. G. Monks. URL: https://monks.scranton.edu/files/pubs/kucinski.pdf. Read through WebFetch.

Monks's survey slides list it as forthcoming in the *Pi Mu Epsilon Journal*. I did not confirm that it appeared there [unverified]. Kionke [2.5] and Chamberland [2.11] cite it as an unpublished preprint.

This is the preprint the draft says it could not find. It studies Joseph's map restricted to Z[i]. Its results:

- A Steiner-type bound (Theorem 1 and Corollary 2) on the period of a cycle, in terms of the counts of the classes 1, i and 1+i in its parity vector, given bounds on the real parts.
- Symmetry results (Theorem 3): conjugate subsystems such as the real axis, the imaginary axis and the line y = x, via h(x+yi) = y+xi.
- Theorem 4: no cyclic points in the second and fourth quadrants, or on two families of lines.
- With a Maple computation, the determination of all cycles of period at most 400: exactly 77 cycles, with periods 1, 2, 3, 5, 8, 11, 19, 46, 84 and 103.

Monks's slides (https://monks.scranton.edu/files/talks/CollatzSurvey/Slides.pdf) quote "T restricted to Z[i] has exactly 77 distinct cycles of period less than or equal to 400". They also mention an entire function, due to Joseph and Monks, that extends T restricted to Z[i] to C.

Relation to the claims. This anticipates part of claim 5: Kucinski already studied orbits of Joseph's map on Z[i], at least as far as cycles go. The draft's Section 10 says Joseph "computed no orbits in Z[i]", which is true of Joseph, but the next step, Joseph's map on Z[i], was taken by Kucinski. The draft's own finding is that 98% of starts with N(z) ≤ 10^5 fail to reach a cycle within 20,000 steps, and that the rest fall into 67 cycles. Kucinski does not report that, as far as the summary shows. It is consistent with her count of 77 cycles, since hers is an exhaustive count by period and the draft's is a count of the cycles reached from small starts. The draft should cite Kucinski, compare the 67 cycles it found with her table, and drop the sentence saying the preprint could not be found. Kucinski does not consider other multipliers (1 ± 2i, 2 ± i) or division by 1+i, so the draft's Joseph-type variants with α = 1 ± 2i are not in her paper [unverified: read through WebFetch, tables not checked entry by entry].

### 2.3 Alvarado and the CSU Channel Islands REU (2017 onward)

Alejandra Alvarado, The Collatz Conjecture over the Gaussian Integers, three talks, none with a paper behind it that I could find:

- Colloquium, University of Colima, 2 April 2025. Slides: https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf.
- Colloquium, Eastern Illinois University, 1 December 2017. Announcement only: https://booth.eiu.edu/math/pdf/Alejandra_Alvarado_Dec_1_2017.pdf.
- Lathisms lecture, posted 26 May 2023: https://www.youtube.com/watch?v=bgRoCCOHmzY.

The work is from a summer 2017 REU at CSU Channel Islands (NSF 1359165). The mentors named are Martinez-Avendano, Martinez and Sittinger, and the students Knutsen, Ceja Ayala and Torchinsky. The 2025 slides say the paper was submitted to the *Minnesota Journal of Undergraduate Mathematics*. I did not find it published [unverified: only the slides and the video description seen; the video itself was not watched].

The map in the slides is C_{2+i,u}. A Gaussian integer is even if 1+i divides it; C sends an even z to z/(1+i) and an odd z to (2+i)z + u, with u a fixed unit. Reported results:

- Computations with u = 1. Of 23,730 tested starts in the box |a|, |b| ≤ 100, about 5.6% reach a unit within 10,000 steps. None takes between 252 and 10,000 steps. The talk remarks that Crandall's heuristic predicts unbounded growth.
- A proposition: z reaches a unit under the rule with digit u_1 exactly when u_2 z reaches u_1 u_2 under the rule with digit u_1 u_2, in the same number of steps.
- Closed forms for the odd P_n with C(P_n) = (1+i)^(2n).
- A congruence description of coalescing points modulo 5.
- An analogous map on the Eisenstein integers that divides by 1-ω, without computational results.

Relation to the claims. This partly overlaps claims 1 and 2. In the draft's terms, C_{2+i,u} is a rule at level k = 1 with α = 2+i and the one-element digit set {u}. So the family of claim 1 has been considered at level 1 with this multiplier, though not as a family indexed by level and digit set, and not with the accelerated map S. The draft's Proposition 3 explains the REU's observation: the level-1 window (2, 4) contains no norm, and N(2+i) = 5 > 4 gives positive drift ln 5 - 2 ln 2 per odd step. The REU's proposition on associates is the level-1 form of the fact behind the draft's Proposition 1. Multiplying by a unit does not fix a rule at level 1; it carries the rule with digit u_1 to the rule with digit u_1 u_2. The draft's statement that no rule at levels 1 or 2 commutes with units does not appear in the slides, nor does anything at level k ≥ 2, the window, H, or the classification. The draft should cite this work in Section 3 or Section 11. It should also say the level-1 Gaussian analogue with α = 2+i was studied computationally by this group, and not present the level-1 map as new.

### 2.4 Garg (2026)

A. Garg, Collatz-Type Dynamics on Gaussian Integers: Fractal Stability Islands, Long Cycles, and Dyadic Rational Structure, Zenodo, doi:10.5281/zenodo.19078383, 18 March 2026. Seen only through the summary at https://synapsesocial.com/papers/W7139080527 [unverified: only a summary seen; the Zenodo record and PDF could not be reached].

According to the summary, Z[i] is split into "four residue classes modulo (1+i)" (sic), with an affine rule on each. In "Variant E" one class is divided by 4. The paper reports:

- A period-40 orbit in Z[1/2][i].
- An argument that the natural space is Z[1/2][i] rather than Z[i].
- A box dimension of about 1.70 for the boundaries of "stability islands".
- Negative Lyapunov exponents for "Variant C".

Relation to the claims. Probably different from claims 1 to 4, since the maps are not integral on Z[i]. Without the formulas I cannot rule out that one of its variants is a rule of the draft's form, so the draft's "[unverified]" should stay.

### 2.5 Kionke (2015, 2017)

S. Kionke, A geometric approach to divergent points of higher dimensional Collatz mappings, arXiv:1511.05893. URL: https://ar5iv.arxiv.org/html/1511.05893. Alvarado's slides give the published version as *Monatshefte für Mathematik* 182 (2017) [unverified: journal details taken from Alvarado's slides, not checked].

Kionke studies maps on a free abelian group Λ of finite rank, of the form T(x) = (m_ω x + r_ω)/d on each coset ω of dΛ, with positive integer scalars m_ω. The paper does three things relevant here:

- It proves that tame cones give divergent orbits.
- It treats the extension of 3x+1 to Z[√2], with no cycles in two quadrants.
- In its outlook, it suggests integer matrices M_ω in place of the scalars m_ω.

On Z[i] it says only that "a similar observation" was made by Kucinski for Joseph's extension. That matches Kucinski's Theorem 4, no cyclic points in the second and fourth quadrants.

Relation to the claims. Different, as the draft says: the multipliers are scalars, and Gaussian multipliers are not. But the outlook explicitly raises matrix multipliers of the form m·g with g in GL(Λ). Multiplication by α composed with division by (1+i)^v is of that kind, so the draft's rules sit inside the extension Kionke proposed and left open. The draft could say so. The divergence mechanism in the draft's Section 8, where λ = α/(1+i)^k is real up to a unit, is exactly the case in which the draft's map with exactly k divisions is a scalar map in Kionke's sense. That fits the draft's own reading.

### 2.6 Matthews, the chapter in *The Ultimate Challenge*

K. R. Matthews, Generalized 3x+1 mappings: Markov chains and ergodic theory, in J. C. Lagarias (ed.), *The Ultimate Challenge: The 3x+1 Problem*, AMS, 2010, pp. 79-103. URL: http://www.numbertheory.org/PDFS/matthews-final-revised.pdf.

Section 9 covers GF(2)[x]: an example of Matthews and Leigh with a divergent orbit, and the Hicks-Mullen-Yucas-Zavislak map, all of whose orbits reach 0 or 1. Section 10, "Other rings: algebraic number fields", begins "Let d be a non-unit in the ring O_K of integers of an algebraic number field K" but gives no general theory. It works three examples instead:

- Example 10.1, on Z[√2]: α/√2 or (3α+1)/√2. Finitely many cycles found, three one-dimensional invariant sets, and divergent orbits evenly distributed modulo powers of √2.
- Example 10.2, on Z[√3]: 103 cycles and one apparently divergent orbit.
- Example 10.3, on Z[√2] with a unit factor: divergent orbits that need not tend to infinity in absolute value.

Section 10 does not mention Z[i]. The concluding remarks say that "the conjectural situation for rings other than Z also invites systematic investigation". Conjecture 3.1, of Matthews and Watts, generalizing Möller, is for maps (m_i x - r_i)/d of relatively prime type. It says all trajectories eventually cycle if |m_0 ⋯ m_{d-1}| < d^d, and almost all diverge if the product exceeds d^d. Section 3.2 derives this from a growth-rate heuristic.

Relation to the claims.

- Claim 1: overlaps partly in framework. The draft's T is a generalized map on O_K = Z[i] with modulus (1+i)^k in the sense of Section 10. On the class of z modulo (1+i)^k it is affine with Gaussian integer coefficients over the denominator (1+i)^k, of non-relatively-prime type because of the even classes. The specific family, with digit sets of minimal norm, unit symmetry and level as parameter, is not in the chapter.
- Claim 2: the average-contraction half of the window is the norm analogue of the Matthews-Watts product heuristic and of Section 3.2. The draft should present it as such, not as a new criterion.
- Section 8 of the draft: Example 10.1 is a precedent. It has a real multiplier, invariant one-dimensional sets and apparent divergence, which matches the draft's λ-real mechanism and its nearly invariant lines.
- Claims 3 and 4: not anticipated.

### 2.7 Other works of Matthews, Leigh and Watts; Hasse, Möller, Heppner

These were seen only through annotations in Lagarias's bibliographies and in Matthews's chapter [unverified: annotations only]. All are on Z, or on F_q[x] for Matthews and Leigh.

- K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arith.* 43 (1984), 167-175.
- K. R. Matthews and A. M. Watts, A Markov approach to the generalized Syracuse algorithm, *Acta Arith.* 45 (1985), 29-42.
- G. M. Leigh, A Markov process underlying the generalized Syracuse algorithm, *Acta Arith.* 46 (1985), 125-143. Bibliography I lists Leigh (1986).
- K. R. Matthews and G. M. Leigh, A generalization of the Syracuse algorithm in F_q[x], *J. Number Theory* 25 (1987), 274-278.
- H. Möller, Über Hasses Verallgemeinerung des Syracuse-Algorithmus (Kakutanis Problem), *Acta Arith.* 34 (1978), 219-226.
- E. Heppner, Eine Bemerkung zum Hasse-Syracuse-Algorithmus, *Arch. Math.* 31 (1978). Bibliography I annotates it as proving that "almost all iterates get smaller" for Hasse's maps when m < d^(d/(d-1)) [unverified: volume and pages from memory].
- H. Hasse, *Unsolved Problems in Elementary Number Theory*, lectures, 1975. Bibliography I says Hasse "proposed a class of generalized 3x+1 maps", studied by Möller and Heppner.

Hasse's class is the closest one-dimensional relative of the draft's construction: multiply by m and subtract the residue that makes the result divisible by d. That is the draft's digit construction, with d in place of (1+i)^k. Heppner's threshold is the one-dimensional analogue of the draft's average-contraction condition.

Siegel's withdrawn arXiv:2111.07882 (2.9) says the first generalization to rings of algebraic integers "was given by Leigh in 1983", and points to Matthews's slides for details. I did not find Leigh's 1983 work or the slides [unverified]. Matthews's chapter credits Leigh with the Markov-chain theory, not with a ring-of-integers example.

Relation to the claims. These works overlap claim 2 in method only: the window is the standard heuristic carried over to the norm. They are different for claims 1, 3, 4 and 5, since all of them are on Z or F_q[x].

### 2.8 Kohl, residue-class-wise affine maps

- S. Kohl, *Restklassenweise affine Gruppen*, Dissertation, Universität Stuttgart, 2005. URL: https://d-nb.info/977164071/34.
- S. Kohl, RCWA: Residue-Class-Wise Affine Groups, GAP package. Manual at https://docs.gap-system.org/pkg/rcwa/doc/chap2.html; source at https://grok.dragonflybsd.org/raw/dports/math/gap/gap-4.11.0/pkg/rcwa-4.6.4/lib/rcwamap.gi.

The thesis works over Z, semilocalizations and F_q[x]. It mentions Z[i] only in passing, as a ring with a weak residue-class divisibility property. It extends the Collatz map to a permutation of Z × Z with scalar coefficients (Example 3.13). The GAP package supports rcwa mappings of Z^2. In the source code each affine piece has a coefficient triple: a 2x2 integer matrix, a vector and an integer denominator. The modulus is a 2x2 integer matrix in Hermite normal form, that is, a sublattice. Neither the manual nor the source shows a worked Z^2 example.

Relation to the claims. This overlaps claim 1 in framework only. Each step of the draft's T is an rcwa mapping of Z^2 in Kohl's sense: the modulus is the lattice (1+i)^k Z[i]; on even classes the multiplier is the matrix of 1-i with denominator 2; on odd classes it is the matrix of α with denominator 1. The software could have been used to study these maps, but I found no sign that anyone did. The rules themselves are different.

### 2.9 Siegel; Siegel and O'Dwyer

- M. C. Siegel, Functional equations associated to Collatz-type maps on integer rings of algebraic number fields, arXiv:2111.07882 (withdrawn). URL: https://arxiv.org/pdf/2111.07882. Examples on Z[√2] (Leigh's map) and Z[√3]; nothing on Z[i]. It says that apart from the work chronicled by Matthews there are "no other noteworthy examples of Hydra maps on algebraic number fields".
- M. C. Siegel, (p,q)-adic analysis and the Collatz conjecture, arXiv:2412.02902. Already cited in the draft; not re-read here.
- M. C. Siegel, The Hydra map and numen formalisms for Collatz-type problems, arXiv:2601.17030, 17 January 2026. URL: https://arxiv.org/pdf/2601.17030. A formalism for Hydra maps on O_K of a global field. Examples on Z only; no Z[i] in the text seen.
- M. Siegel and R. O'Dwyer, Almost-boundedness of Collatz-type maps on number rings via Archimedean methods, arXiv:2609.33018, 26 September 2026. URL: https://arxiv.org/abs/2609.33018.

Siegel and O'Dwyer extend Tao's almost-bounded-orbits theorem to p-Hydra maps on O_K. These are affine maps r_j z + c_j indexed by the residues modulo an ideal of index p, where the r_j are Q-linear maps of K and so may be Gaussian multiplications. The theorem holds under conditions MH1 to MH4 and a constraint (p-1)^2 ρ^d < 1, with ρ the product of the Minkowski operator norms of the r_j. The text seen has no Z[i] example [unverified: the text was cut off before the references, and MH1 was missing from the extraction].

Relation to the claims. Different for claims 1 to 5. The Siegel and O'Dwyer theorem is relevant to the draft's open problem 3, a density theorem in (1+i)-adic form. If H, or an iterate of it, can be written as a p-Hydra map satisfying their hypotheses, it would give almost-bounded orbits for H on a set of logarithmic density one. I have not checked this, and it is not obvious. H's digits depend on the class modulo (1+i)^4, so H is not a 2-Hydra over (1+i). A naive encoding modulo (1+i)^4 (p = 16) appears to fail the constraint (p-1)^2 ρ^2 < 1 by a wide margin [unverified: my rough estimate, assuming ρ is the product of |1/(1+i)| over the 8 even classes and |α/(1+i)^4| over the 8 odd ones; the exact definitions were not read]. The draft's open problem 3 should cite this paper and say whether it applies.

### 2.10 Polynomial-ring analogues

- K. Hicks, G. L. Mullen, J. L. Yucas and R. Zavislak, A polynomial analogue of the 3N+1 problem?, *Amer. Math. Monthly* 115 (2008). Seen through the bibliography II annotation and Matthews's Example 9.2.
- Matthews and Leigh (1987), in 2.7.
- D. Nichols, Analogues of the 3x+1 problem in polynomial rings of characteristic 2, *Experimental Mathematics* (2016), doi:10.1080/10586458.2016.1227734, arXiv:1610.02545. A Terras-type stopping-time theorem for function rings of curves over F_2. Abstract only [unverified: only abstract seen].
- L. H. Gallardo and O. Rahavandrainy, A variant of Collatz's conjecture over binary polynomials, arXiv:2510.07530, 10 October 2025. It uses x(x+1) in place of 2 and x^2+x+1 in place of 3, and proves every sequence finite.
- K. G. Monks, On q-analogs of the 3x+1 dynamical system, arXiv:2508.10153 (v5, 18 November 2025). Maps on F_2[[q]] conjugate to 3x+1 on Z_2.
- Behajaina and Paran, arXiv:2312.00390. Listed in the working notes; not re-read.

Relation to the claims. Different. Nichols's Terras-type theorem in function rings is the closest precedent for the draft's open problem 3, in a non-archimedean setting.

### 2.11 Surveys and complex-plane interpolations

- J. C. Lagarias, The 3x+1 problem: an overview, in *The Ultimate Challenge* (2010), pp. 3-29, arXiv:2111.02635. It mentions rings of integers of number fields and function fields only in general terms. No Gaussian integers, Joseph, Kucinski or Leigh.
- M. Chamberland, An update on the 3x+1 problem. URL: https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf. Section 6.4 covers Joseph and Kucinski on Z[i], and Section 6.6 the extensions to C. The draft could cite it as the survey that records Kucinski.
- Complex-analytic interpolations: Letherman, Schleicher and Wood (*Exp. Math.* 8 (1999)), Dumont and Reiter (2001), Pe (2004), Chamberland (1996), Berg and Meinardus (1994, 1995). These are analytic, not integral maps on Z[i], so they are different for all claims.

### 2.12 Wirsching

G. J. Wirsching, *The Dynamical System Generated by the 3n+1 Function*, Lecture Notes in Mathematics 1681, Springer, 1998. Not seen [unverified]. From what is known about it, the book is about the predecessor sets of 3n+1 on Z and the 3-adic integers. I found no indication that it treats other rings. Bibliography II v2 annotates two later Wirsching papers (2001, 2003), both on Z.

### 2.13 Works checked and found to be on Z only

All of these were checked and none has a Gaussian, lattice or number-field map:

- R. Tremblay, arXiv:1907.13302, 1909.00213, 1910.11798.
- W. Carnielli, Some natural generalizations of the Collatz problem, *Applied Mathematics E-Notes* 15 (2015), 207-215.
- J. R. Goodwin, arXiv:1504.03040.
- Hohertz and Kalantari, arXiv:2001.00482.
- B. Khanzadeh Holasou, easychair preprint 7083.
- Wiltrout and Landquist, *EJUM* 17 (2013).
- O. Rozier, arXiv:1805.00133.
- J. S. Gilbert, preprints.org 202607.0575.
- M. Carelli, arXiv:2605.15094.
- Zhu, Chen and Sun, arXiv:1903.10096 (base-k digits, despite "k-adic field" in the title).
- The OEIS wiki page on the 3x+1 problem.
- Clark and Lewis, and Clark: planar difference equations, not lattice maps.

Bruschi, arXiv:0810.5169, came up in searches. The working notes list it as Z only; I did not re-read it.

A correction for the draft: its reference 5 says "See also arXiv:1907.13302 and arXiv:1910.11798" after Matthews's chapter. Both papers are by Robert Tremblay, not Matthews. Both concern maps on Z.

## 3. Claim by claim

### 3.1 Claim 1: the family T, digit sets and the accelerated map S

The general setting is known in three frameworks, none of which works out this family or gives Z[i] examples:

- Matthews's maps on O_K modulo a non-unit d (2.6).
- Kohl's rcwa mappings of Z^2 with integer matrix coefficients (2.8).
- Siegel's Hydra maps on O_K (2.9).

Kionke proposed matrix multipliers as an extension and left it open (2.5). The digit construction, multiplying and then adding the residue that makes the result divisible, is Hasse's construction carried over from d to (1+i)^k (2.7).

The specific family with z/(1+i) on evens and αz + d(z) on odds has been studied before, at level 1 with α = 2+i, by Alvarado and the 2017 CSU Channel Islands REU (2.3), and there it diverges on average. I found no prior treatment of levels k ≥ 2, of digit sets modulo (1+i)^k as a parameter, or of the accelerated map S on Z[i].

So the family is new as a systematic object, as far as this search reaches. The draft should cite the REU work for the level-1 case and the general frameworks for context.

### 3.2 Claim 2: the window, level 1, and unit symmetry at levels 1 and 2

The averaging argument behind the upper end of the window is the standard growth-rate heuristic of Matthews and Watts (2.6), with antecedents in Möller and Heppner (2.7), carried over to the norm. The worst-case contraction below 2^k (Proposition 2) is routine.

I found no statement of the window 2^k < N(α) < 2^(k+1) for Z[i], of Proposition 3 (the level-1 window contains no norm because 3 is not a sum of two squares), or of Proposition 1 (no rule at levels 1 or 2 commutes with units).

These are elementary. The REU slides contain observations that a reader will connect with both. Their level-1 rule with α = 2+i diverges, which Proposition 3 explains. Their proposition on associates shows units permuting the level-1 rules rather than fixing one, which is the k = 1 content of Proposition 1.

So the statements appear to be new as stated, but they are easy, and the draft should not lean on their novelty. It should give the Matthews-Watts heuristic as the source of the averaging criterion.

### 3.3 Claim 3: the rule H and its single-cycle conjecture

Nothing found. No source considers α = 5+2i, level 4, or the digit set {±1, ±i, ±(2+i), ±(1-2i)}, and no source conjectures a single cycle for a Gaussian rule.

This appears to be new. Possible exceptions are the unread parts of the sources listed in Section 1 (bibliography I from M on, Garg's formulas, the Alvarado group's paper, Matthews's web pages). None of them is likely to contain H, but I cannot rule that out.

### 3.4 Claim 4: classification at levels 3 to 6, G_3, and Proposition 5

Nothing found on any of the following:

- The classification of unit-symmetric rules with minimal digits at levels 3 to 6.
- G_3, the rule 3z + unit at level 3, or its five cycles.
- The restriction of G_3 to the axes.
- Proposition 5, that no α = 3 rule both agrees with 3x+1 on the positive integers and contracts.

The nearest relatives restrict to 3x+1 in some sense: Joseph's map restricts to 3x+1 on Z_2 (2.1), and Kohl's Z × Z permutation restricts to the Collatz map on the first coordinate (2.8). Both divide by 2 or act with scalars, so neither bears on Proposition 5. The divergence observations in the draft's Section 8 have precedents in Matthews's Example 10.1 on Z[√2] (invariant one-dimensional sets with a real multiplier) and in Kionke's cones.

All of claim 4 appears to be new, subject to the same caveat as claim 3. Section 8 should cite Matthews's Example 10.1 alongside Kionke.

### 3.5 Claim 5: Joseph's map and similar maps

Joseph's own results are correctly described in the draft. Joseph's map on Z[i] was studied by Kucinski (2000), who determined all 77 cycles of period at most 400 and proved that the second and fourth quadrants contain no cyclic points (2.2). The draft's sentence saying the Kucinski preprint could not be found is now wrong, and Section 10 should cite her and compare cycles.

Two parts of Section 10 appear to be new:

- The observation that about 98% of starts with N(z) ≤ 10^5 do not reach a cycle within 20,000 steps, with the drift estimate +0.26.
- The divide-by-2 variants with α = 1 ± 2i and 2 ± i.

Some caution is needed on the first: Kucinski's proof of Corollary 5 refers to orbits that are "divergent", and the summary I saw does not show what she claims about them.

## 4. Changes the draft needs

1. Cite Kucinski (2.2) in Sections 10 and 11, and delete the statement that the preprint could not be found.
2. Cite Alvarado and the 2017 REU (2.3) in Section 3 or Section 11 as the prior study of the level-1 rule with α = 2+i.
3. Correct reference 5: arXiv:1907.13302 and 1910.11798 are by Tremblay.
4. Correct Joseph's page range to 309-316.
5. Give Kionke's journal version, after checking it.
6. Attribute the averaging half of the window to the Matthews-Watts heuristic (and Möller, Heppner).
7. Mention that the family fits Matthews's O_K setting, Kohl's RCWA(Z^2) and Kionke's proposed matrix extension.
8. Cite Matthews's Example 10.1 in Section 8.
9. Cite Siegel and O'Dwyer (2.9) in open problem 3, and Nichols (2.10) as a Terras-type precedent outside Z.
10. Keep the "[unverified]" on Garg, and soften "We found no published study ..." in Section 11 to take account of Kucinski (Joseph's map on Z[i]) and the REU (division by 1+i at level 1, unpublished as far as found).
