# Prior art for rotated and mirrored maps

Search done 5 November 2026, for Proposition 6.3 and Theorem 6.4 of `papers/collatz-rings.md` (paper 1), which are Proposition 4.2, Theorem 4.4, Proposition 4.6 and Theorem 4.7 of `theorems.md`. It extends row 8 of `invariants-prior-art.md` and does not repeat the access notes of `literature.md` except where they changed.

## 1. The result, and how it splits

Let $T$ have multiplier $\alpha$ and digit table $d$ at modulus $\pi^k$, with accelerated map $S_\alpha$.

(a) Rotation. The rotated map has multiplier $u\alpha$ and table $z \mapsto d(uz)$, and $S_{u\alpha}(z) = S_\alpha(uz)$. If $d$ commutes with the units, then $S_{u\alpha} = uS_\alpha$, $S^n_{u\alpha} = u^nS^n_\alpha$, and one map is a Collatz map exactly when the other is.

(b) Reflection. For an automorphism $\sigma$ with $\sigma(\pi) = \eta\pi$, $\eta$ a unit (complex conjugation on $\mathbb{Z}[i]$, $\eta = -i$), the mirrored map has multiplier $\sigma(\alpha)$ and table $\sigma \circ d \circ \sigma$, and $S_{\sigma\alpha}(z) = \eta^{v}\sigma(S_\alpha(\sigma z))$ for one step. With a commuting table the same holds for iterates, with the total exponent in place of $v$, so the norm sequences of $z$ and $\sigma z$ agree and the two maps are Collatz maps together.

For judging prior art it helps to write both parts as a classical operation followed by a Collatz-specific one. Put $C_u(z) = u^{-1}S_\alpha(uz)$, the conjugate of $S_\alpha$ by multiplication by $u$; it is the accelerated map with multiplier $\alpha$ and table $z \mapsto u^{-1}d(uz)$. Then $S_{u\alpha} = u \cdot C_u$: rotation is conjugation by $u$ followed by multiplying the output by $u$. When the table commutes with the units, $C_u = S_\alpha$. Likewise $\sigma \circ S_\alpha \circ \sigma^{-1}$ is the accelerated map with multiplier $\sigma(\alpha)$ and table $\sigma d \sigma$ that divides by $\sigma(\pi)$; the mirrored map divides by $\pi$ instead, and the factor $\eta^v$ is the price of changing the generator of the prime. So (a) is "conjugate by a unit, then twist by a unit", and (b) is "conjugate by an automorphism, then change the generator of $\pi$". The conjugations are standard; the twist and the change of generator are where the Collatz setting enters. In both cases the new map and the conjugated map differ at each point by a unit, so they induce the same map on the orbit space $R/U$ when the table commutes with the units, and the rest of Theorem 6.4 (Lemma 4.3 of `theorems.md`) is the fact that a relative periodic point of a map commuting with a finite group is periodic.

## 2. Method and access

WebSearch (standard and extended) and WebFetch only, as in the earlier searches. WebFetch returns a small-model summary, so quotations below are short fragments as that summary rendered them, and theorem numbers come from those summaries.

Refused or not reached in this search: the RCWA manual chapter 3, where `Mirrored` is documented (https://docs.gap-system.org/pkg/rcwa/doc/chap3.html, not on the allowlist), so `Mirrored` is still known only from the index and the source file read on 5 November (`invariants-prior-art.md` 2.4); Wikipedia's Collatz page (not on the allowlist); Snapp and Tracy's JIS source file (returned binary data); the Lamb and Roberts survey abstract page (binary data; the reference was taken from the bibliography of arXiv:1812.08727); Fraboni's paper itself (no copy found; known only from Monks's slides); the full text of Kohl's dissertation beyond the front matter and Chapters 1 and 2; Golubitsky and Stewart's book and Field's book beyond the excerpt in item 3.12. Kucinski's preprint, refused on a direct fetch, was read after it appeared as a search result.

## 3. Works

### 3.1 The sign conjugacy on $\mathbb{Z}$: $3x+1$ on the negatives is $3x-1$

That $3x+1$ on the negative integers is $3x-1$ on the positive integers, by $x \mapsto -x$, is folklore. It is stated in OEIS A037084 (comment by D. Kamenetsky, 2017, "Equivalent to the Collatz ('3n+1') problem for negative integers", https://oeis.org/A037084) and in F. Gonçalves, R. Greenfeld and J. Madrid, Generalized Collatz maps with almost bounded orbits, arXiv:2111.06170v2 (2022), Remark 1.7 ("equivalent to the original Collatz map, but over the negative integers", https://arxiv.org/abs/2111.06170). Lagarias's 1985 survey, section on cycles (https://www.cecm.sfu.ca/organics/papers/lagarias/paper/html/node8.html), lists the negative cycles but in the extracted text does not name the $3x-1$ map. Paper 2 already uses the identity $-(3x+1) = 3(-x) - 1$ in its introduction.

Closeness. This is conjugation by the unit $-1$ of $\mathbb{Z}$: it keeps the multiplier and changes the digit, so it is the map $C_u$ of Section 1 with $u = -1$, not the rotation $\alpha \mapsto -\alpha$.

### 3.2 Scaling conjugacies: Bernstein and Lagarias, Lagarias 1990, Matthews

D. J. Bernstein and J. C. Lagarias, The $3x+1$ conjugacy map, *Canad. J. Math.* 48 (1996), 1154 to 1169 (retypeset copy https://cr.yp.to/papers/3x1conjmap-19960215-retypeset20220326.pdf). The introduction notes that for odd $k$, $Q(x) = kx$ conjugates the $3x+1$ map to the $3x+k$ map on $\mathbb{Z}_2$, and that the cycle structures modulo $2^n$ of the conjugacy maps $\Phi_{3,k}$ are identical. Section 4 and Appendix B treat the $ax+b$ maps and a wider class $U_{V_0,V_1}$, all solenoidally conjugate to the shift. The paper also recalls (Hedlund) that the automorphism group of the shift is $\{1, V\}$ with $V(x) = -1-x$.

Matthews's chapter (http://www.numbertheory.org/PDFS/matthews-final-revised.pdf, Example 5.1) records Lagarias's observation that the cycles of $T_{3t}$ are $3t$ times those of $T_1$, and the correspondence between rational cycles of $3x+1$ with denominator $k$ and integer cycles of $3x+k$. Gonçalves, Greenfeld and Madrid (Section 2.2) use the same scaling, "$C(hN) = hC^*(N)$", calling $C$ and $C^*$ "topologically conjugated in $h\mathbb{N}$". Barina and Maat (arXiv:2602.05732) prove the case $3n + 3^k$ (`invariants-prior-art.md` 3.5).

Closeness. Conjugation by multiplication by a $2$-adic unit $k$ (with $k = -1$ the case of 3.1). It is the integer analogue of $C_u$ and of the REU proposition (3.8): the multiplier is fixed and the digit is scaled. None of these rotate the multiplier. Matthews's chapter has no passage on $x \mapsto -T(-x)$ or on equivalent maps beyond these (fetched again for this search).

### 3.3 Autoconjugacy: Monks and Yazinski, and the generalization to $T_{m,r}$

K. G. Monks and J. Yazinski, The autoconjugacy of the $3x+1$ function, *Discrete Math.* 275 (2004), 219 to 236; and the generalization in arXiv:1206.0553 (The autoconjugacy of a generalized Collatz map, https://arxiv.org/pdf/1206.0553). For $T_{m,r}$ on $\mathbb{Z}_2$ ($m, r$ odd), $\Omega_{m,r} = \Phi_{m,r} V Q_{m,r}$ commutes with $T_{m,r}$ and is an involution, and $\Phi_{m,r} = r\Phi_{m,1}$. The fetched summary says the paper does not discuss conjugation by $x \mapsto -x$ or affine maps beyond this, nor families of equivalent maps.

Closeness. A conjugacy of one map with itself, transported from the shift; it is not a relation between maps with different multipliers. The scaling $\Phi_{m,r} = r\Phi_{m,1}$ is again the digit-scaling of 3.2.

### 3.4 Fraboni's classification of linear conjugacies

M. Fraboni, Conjugacy and the $3x+1$ conjecture, 1998 (MAA EPADEL student paper prize), known only from K. G. Monks's slides (https://monks.scranton.edu/files/talks/CollatzSurvey/Slides.pdf and https://monks.scranton.edu/files/talks/GermanySlides.pdf). For $f_{a,b,c,d}(x) = (ax+b)/2$ on evens and $(cx+d)/2$ on odds over $\mathbb{Z}_2$, a map is topologically conjugate to $T$ exactly when $a, c, d$ are odd and $b$ is even, and the slides list which members are conjugate to $T$ by a linear map $h(x) = px + q$ (for instance $f_{1,q,3,p-q}$ with $p$ odd and $q$ even). No published version was found [unverified: only the slides seen].

Closeness. This is the nearest thing found to a classification of Collatz-type maps up to affine conjugacy, on $\mathbb{Z}_2$ for $2$-branch maps. It concerns conjugacy, not the twist by a unit, and has no unit group beyond $\pm 1$.

### 3.5 Kohl: RCWA `Mirrored`, conjugates of Collatz-type maps, the involution $n \mapsto -n$

- RCWA (https://docs.gap-system.org/pkg/rcwa/doc/chapInd.html; source `rcwamap.gi` of RCWA 4.6.4): `Mirrored` conjugates an rcwa map of $\mathbb{Z}$ by $n \mapsto -n-1$ (`invariants-prior-art.md` 2.4). Manual chapter 2 (https://docs.gap-system.org/pkg/rcwa/doc/chap2.html) mentions conjugation only through the operator `^` and defines `IsSignPreserving` and `IsClassWiseOrderPreserving`; it has nothing on maps commuting with $n \mapsto -n$.
- S. Kohl, dissertation (https://d-nb.info/977164071/34), Examples 1.1.3, item 2: a Collatz-type permutation $\alpha$ "zentralisiert die Involution $\varsigma: n \mapsto -n$", that is, $(-n)^\alpha = -(n^\alpha)$; Lemma 1.3.1 bounds the modulus of a conjugate $f^\sigma$; the English summary restates the $3n+1$ conjecture as the existence of a conjugating permutation, and Theorem 3.11 shows it cannot be rcwa.
- S. Kohl, On conjugates of Collatz-type mappings (https://stefan-kohl.github.io/articles/monoconj.pdf; received 2006, WSPC template, presumably *Int. J. Number Theory* [unverified: venue not on the copy read]): Theorem 2.1, a necessary condition for an rcwa map to be conjugate to a monotonic map by an rcwa permutation; Corollary 2.5, the Collatz map is not.

Closeness. Conjugation of a single map, and one example of a map commuting with $-1$. No rotation of the multiplier and no ring other than $\mathbb{Z}$ (RCWA is not implemented over $\mathbb{Z}[i]$, `invariants-prior-art.md` 2.4).

### 3.6 Barina, $7x \pm 1$

D. Barina, $7x \pm 1$: close relative of Collatz problem, arXiv:1807.00908, and the OEIS wiki page (https://www.oeis.org/wiki/7x%C2%B11_problem): "every progression in $7x\pm 1$ on negative numbers corresponds to negated progression on positive numbers". Neither mentions $-7x \pm 1$.

Closeness. The orbit statement $S(-z) = -S(z)$ for a map commuting with $-1$ (Lemma 1.4(c) of `theorems.md` on $\mathbb{Z}$). The rotation $\alpha \mapsto -\alpha$ on $\mathbb{Z}$, which for an odd map gives $S_{-m} = -S_m$ and the loop $1 \leftrightarrow -1$ noted in paper 1, Section 6, was not found anywhere.

### 3.7 Joseph, $\mathbb{Z}_2[i]$

J. A. Joseph, A chaotic extension of the $3x+1$ function to $\mathbb{Z}_2[i]$, *Fibonacci Quart.* 36 (1998), 309 to 316 (https://www.mathstat.dal.ca/FQ/Scanned/36-4/joseph.pdf). The map is topologically conjugate to $T \times T$ but not by a $\mathbb{Z}_2$-module isomorphism (also in Monks's Germany slides).

Closeness. Conjugacy of one map to a product; division by $2$, a single multiplier $3$, and a digit set $\{1, i, 1+i\}$ that is not closed under units or conjugation. Nothing on rotations or mirrors.

### 3.8 Kucinski, Theorem 3

G. I. Kucinski, Cycles for the $3x+1$ map on the Gaussian integers, preprint, University of Scranton, 2000 (https://monks.scranton.edu/files/pubs/kucinski.pdf), reread for this search. Theorem 3: the real axis, the imaginary axis and the line $y = x$ are conjugate subdynamical systems of Joseph's map on $\mathbb{Z}[i]$, and so are the second and fourth quadrants and two further pairs of sets. The conjugacies are $h_1(z) = zi$ (real axis to imaginary axis), $h_2(z) = z + zi$ (real axis to the diagonal) and $h(x+yi) = y + xi$ restricted to the sets; the verification is "an elementary, but tedious, exercise" and is omitted. The use made of it: "we need only examine one set from each of (a)-(d) above in order to determine all cycles", and if $a + bi$ is cyclic so is $b + ai$. Complex conjugation itself and rotated maps do not appear.

Closeness. The closest precedent for (b): $h(z) = i\bar z$ is a reflection followed by a rotation, and it is used exactly as paper 1 uses the mirror, to transport cycles. But it is a conjugacy of one fixed map with itself on invariant subsets, the multiplier $3$ is real, the division is by $2 = -i(1+i)^2$ with $\bar 2 = 2$, so no factor $\eta^v$ arises, and nothing is said about the map with conjugated parameters.

### 3.9 Alvarado and the 2017 CSU Channel Islands REU

A. Alvarado, The Collatz conjecture over the Gaussian integers, slides, University of Colima, April 2025 (https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf), reread for this search. The proposition: "A Gaussian integer $z$ lands using the unit $u_1$ if and only if its associate $u_2z$ lands using the unit $u_1u_2$", in the same number of steps, landing on $u_2u$. The only multiplier is $2+i$; $2 - i$, $i(2+i)$ and complex conjugation do not appear. The Eisenstein map has no digit parameter and no analogous proposition.

Closeness. On $\mathbb{Z}[i]$ at modulus $1+i$ this is exactly the conjugation $C_u$ of Section 1: multiplier kept, digit multiplied by $u_2$. It is the Gaussian form of 3.1 and 3.2. Rotation of the multiplier is not in it.

### 3.10 Siegel; Siegel and O'Dwyer; Matthews Section 10

Not refetched. The earlier searches (`invariants-prior-art.md` 2.3 and 2.4) found no discussion of units, Galois conjugates or equivalences of maps in Siegel's papers, in Siegel and O'Dwyer, or in Matthews's Section 10. A search for an affine normalization of Hydra maps ("without loss of generality" conjugations) turned up nothing beyond those papers.

### 3.11 Recent Gaussian and Eisenstein material

Extended searches for 2024 to 2026 work on Collatz maps on $\mathbb{Z}[i]$ or $\mathbb{Z}[\omega]$ with units, conjugation or rotated multipliers found only items already listed and unrelated pieces: Diarte-Carot, *J. Appl. Math. Phys.* 13 (2025), 1960 to 1968 ("Gaussian" means Gauss's congruences; positive integers only); Bouhamidi, arXiv:2601.17573 (integer maps, no symmetry); Gilbert, preprints.org 202607.0575 (a conjugacy of the Collatz map to a map on the nonzero integers built from classes modulo $3$); Karami, arXiv:2003.07708; a 2021 blog post at matrixdreams.com (no symmetry remarks).

### 3.12 Equivariant dynamics: the standard facts

- M. Field, *Dynamics and Symmetry*, Imperial College Press, 2007 (excerpt of Chapters 1 and 2 read at https://chaosbook.org/library/FieldEquiv.pdf): Definition 2.2.1 of a $G$-equivariant map, $f(gx) = gf(x)$; the orbit space and the induced map of a map constant on orbits (Exercise 2.3.7); isotropy groups and orbit-stabilizer (Definition 2.3.8, Exercise 2.3.10). Skew products and principal $G$-extensions are in Section 9.1, not read.
- P. Cvitanović et al., *Chaos: Classical and Quantum*, ChaosBook.org, Chapter 11, "World in a mirror" (edition 16.2, 2019; https://www.chaosbook.org/chapters/discrete.pdf): the symmetry image of a cycle is a cycle; the multiplicity of a cycle is $|G|/|G_p|$; relative periodic orbits, $x(t) = g_px(t+T_p)$; and the reduction of the dynamics to the fundamental domain $M/G$, where each family of symmetry-related cycles is one cycle.
- J. S. W. Lamb and J. A. G. Roberts, Time-reversal symmetry in dynamical systems: a survey, *Physica D* 112 (1998), 1 to 39 (reference from arXiv:1812.08727). The standard reference for the symmetry group and reversing symmetry group of a map. arXiv:1812.08727 itself (Recognition of symmetries in reversible maps) states the definitions and that conjugate maps "generate equivalent dynamics".
- M. Golubitsky and I. Stewart, *The Symmetry Perspective*, Birkhäuser, 2002, was not read; it is the usual citation for the fact that a symmetry maps solutions to solutions [unverified: not seen].

Closeness. These give, in general form: a conjugacy carries orbits, cycles and their stabilizers to those of the conjugate map; a map commuting with a finite group $G$ has its cycles in $G$-families of size $|G|/|G_p|$ (Theorem 2.1 of `theorems.md`); two equivariant maps that agree up to an element of $G$ at every point induce the same map on $X/G$; and a relative periodic point ($f^p(x) = gx$, $g \in G$ finite) is periodic. With these, (a) for commuting tables is the statement that $u \cdot S_\alpha$ and $S_\alpha$ induce the same map on $R/U$ (twisting by an element of the centre of the symmetry group), and (b) is that the mirror and $\sigma S_\alpha \sigma^{-1}$ induce the same map on $R/U$, which is conjugate to the map induced by $S_\alpha$ through the map induced by $\sigma$. I did not find these four facts stated together in one textbook with theorem numbers; each is elementary and stated or used in the sources above.

## 4. Verdict

Partly known, and largely a standard fact of equivariant dynamics; the Collatz-specific parts were not found.

1. Conjugating a Collatz-type map by a unit, which keeps the multiplier and moves the digit, is known: on $\mathbb{Z}$ as $3x+1$ versus $3x-1$ (folklore; OEIS A037084; Gonçalves, Greenfeld and Madrid, Remark 1.7) and as the scaling $Q(x) = kx$ from $3x+1$ to $3x+k$ (Bernstein and Lagarias 1996; Lagarias 1990 via Matthews, Example 5.1); on $\mathbb{Z}[i]$ at modulus $1+i$ as the REU proposition. Affine conjugacy classes of $2$-branch maps on $\mathbb{Z}_2$ were classified by Fraboni [unverified: slides only].
2. Conjugating by an automorphism or a reflection is standard topological conjugacy; Kucinski's Theorem 3 is the Gaussian special case for a single map with a real multiplier and division by $2$.
3. The rotation $\alpha \mapsto u\alpha$ with the table $d(uz)$, and the identity $S_{u\alpha}(z) = S_\alpha(uz)$, were not found, not even on $\mathbb{Z}$ as $m \mapsto -m$. For commuting tables, $S_{u\alpha} = uS_\alpha$ and the equivalence of the two maps is a standard equivariant fact once stated (same map on $R/U$; relative periodic implies periodic for finite $U$).
4. The mirrored map with the unit factor $\eta^{v}$, coming from $\sigma(\pi) = \eta\pi$ with $\eta \ne 1$, was not found. Neither was the remark that without a commuting table the factor breaks the relation after one step, nor the observation that a non-commuting map depends on the choice of generator of $\pi$.
5. The family of up to $2|U|$ equivalent maps, the orbit of $(\alpha, d)$ under the group generated by $U$ and $\sigma$, was not found for Collatz maps. In general terms it is the action of the normalizer of the symmetry group on equivariant maps, which is standard.

## 5. Recommendation for paper 1

Prove with credit, presenting the result as a standard fact plus its Collatz-specific consequence. Concretely:

1. Keep Proposition 6.3 and Theorem 6.4, since the proofs are a few lines and are in Lean, but do not present the equivariance argument as new. Add a sentence such as: "The argument is the standard one for maps commuting with a finite group: two maps that commute with $U$ and agree up to a unit at each point induce the same map on $R/U$, and a point with $g^p(x) = \varepsilon x$ is periodic [Field, Ch. 2; ChaosBook, Ch. 11]."
2. Say what is classical and what is not by splitting rotation into conjugation and twist: "Conjugating by a unit keeps $\alpha$ and replaces $d$ by $u^{-1}d(u\,\cdot)$; on $\mathbb{Z}$ this is the passage from $3x+1$ on the negatives to $3x-1$, and from $3x+1$ to $3x+k$ by $x \mapsto kx$ [Bernstein and Lagarias 1996], and on $\mathbb{Z}[i]$ it is the REU proposition [Alvarado 2025]. Rotation is this conjugation followed by multiplication by $u$, which changes the multiplier to $u\alpha$." The integer case $m \mapsto -m$ (odd maps $mx \pm 1$ and $-mx \mp 1$, with fixed points $\pm 1$ against the loop $1 \leftrightarrow -1$) can be mentioned as the simplest instance.
3. For the mirror, cite Kucinski's Theorem 3 as the nearest precedent (the conjugacy $h(z) = i\bar z$ of Joseph's map on invariant subsets) and say that division by $2$, which conjugation fixes, produces no unit factor, while division by $1+i$, with $\overline{1+i} = -i(1+i)$, produces $(-i)^v$. That factor, and the computations showing that non-commuting tables lose the relation (the pairs $2 \pm i$ at $(1+i)^2$, $3$ and $3i$ at $(1+i)^3$), are the paper's own and should be stated as such, with "not found in the sources we read" rather than "new".
4. Mention Kohl's `Mirrored` (conjugation by $n \mapsto -n-1$) and Monks and Yazinski's autoconjugacy only in passing, as integer conjugacies of a single map, so that a referee who knows them sees they were considered.
5. The phrase "each map comes in a family of eight rotated and mirrored maps" in paper 2's abstract can stay; paper 1 might add that the family is the orbit of the parameters under the group generated by the units and conjugation, so that it is clearly a symmetry of the parameter space and not a further theorem.
