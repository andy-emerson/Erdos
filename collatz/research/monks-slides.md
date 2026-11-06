# K. G. Monks's talk slides and the quotient viewpoint

Working notes, 6 November 2026. Nothing here is in the paper drafts or the catalog. Context: `quotient.md` (A1 to A14) and `fundamental-domain.md`. Sources read: the three attached PDFs, `GermanySlides.pdf` (26 pages), `SlidesV4.pdf` (141 pages) and `TalkV4.pdf` (6 pages), not copied into the repo; and, because none of them contains the section asked about, the survey deck at https://monks.scranton.edu/files/talks/CollatzSurvey/Slides.pdf, read only through WebFetch (direct download was refused by the proxy, 403), so through a small model's summaries of an extracted text layer that drops glyphs and minus signs. Page numbers below are PDF page numbers; the slides themselves carry no slide numbers. The check script is `omega.py` in the session scratchpad, not in the repo.

## 1. Summary

1. None of the three attached PDFs has a "Parity Neutral Collatz" section, nor anything on $x \sim -x$ or $\mathbb{Z}_2/\!\sim$. I checked the text layer of every page of all three and read as images GermanySlides pages 1 to 26, TalkV4 pages 1 to 6 and SlidesV4 pages 112 to 141 (the 2-adic part; pages 1 to 111 are incremental beamer builds of the arithmetic-progression material, checked by text only).
2. The section is in the survey deck "The 3x+1 Problem (or how to assign intractable open questions to undergraduates)", where it is the 19th main heading, after "The Nontrivial Autoconjugacy of T" and before "Application to Divergent Orbits".
3. The relation there is not $x \sim -x$. It is $x \sim y$ iff $x = y$ or $x = \Omega(y)$, where $\Omega$ is the Monks and Yazinski autoconjugacy of $T$. The extracted text drops the glyph, but the worked example pairs $3$ with $4/9$ (sign dropped), and I computed $\Omega(3) = -4/9$; under $x \sim -x$ no fractions could appear in the orbit of the class of $3$. The note in `fundamental-domain.md` (Section 1 item 8, Section 10 item 3, and the "Not reached" paragraph) that the section uses $x \sim -x$ is therefore wrong and should be corrected.
4. The source is K. G. Monks and J. Yazinski, The autoconjugacy of the $3x+1$ function, Discrete Math. 275 (2004), 219 to 236, whose abstract says that $\Omega$ is used to restate the conjecture "in a parity neutral form". The slide cites that paper ("to appear") in the preceding section; the parity neutral slide itself names no one.
5. For our paper: Monks and Yazinski have the quotient of a Collatz map by an involution commuting with it, the induced map on the quotient, and the equivalence of the conjecture with its quotient form, for the group $\langle \Omega \rangle$ of order $2$ acting on $\mathbb{Z}_2$. They do not have the quotient by $-1$, the identification $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, units, rings $R/U$, or any of our results specific to associates. They should be credited as the earliest instance we know of the quotient idea for Collatz maps, with the difference of symmetry stated plainly (Section 4).

## 2. The Parity Neutral Collatz section

### 2.1 What the extraction shows

The map throughout is $T(x) = x/2$ for even $x$, $(3x+1)/2$ for odd $x$, on $\mathbb{Z}_2$. Raw fragments of the section (a glyph lost in extraction is marked here by $[\,?\,]$; fragments are those returned by WebFetch and are short by necessity):

1. "Define $[\,?\,]$ on Z2 by", then "x / x / 2 if x is even" and "x if x is odd". So a map, call it $\tau$, with $\tau(x) = x/2$ on even $x$ and $\tau(x) = [\,?\,](x)$ on odd $x$.
2. "Define $[\,?\,]$ on Z2 by / x $[\,?\,]$ y $[\,?\,]$ x $[\,?\,]$ y or x $[\,?\,]$ y / for all x, y $[\,?\,]$ Z2 / is an equivalence relation on Z2".
3. "Z2/ $[\,?\,]$ x,x : x $[\,?\,]$ Z2 and x is odd", that is, $\mathbb{Z}_2/\!\sim\; = \{\{x, [\,?\,]x\} : x \in \mathbb{Z}_2 \text{ and } x \text{ is odd}\}$.
4. An induced map on $\mathbb{Z}_2/\!\sim$ sending the class of $x$ to the class of $T(x)$ (from the summary; the symbol is lost).
5. A theorem that the following are equivalent: "(a) The Collatz Conjecture.", "(b) The $[\,?\,]$-orbit of any positive integer contains 1.", "(c) The $[\,?\,]$-orbit of the class of any positive integer contains 1."
6. Examples: "The T-orbit of 3 is / 3, 5, 8, 4,2, 1"; "while the $[\,?\,]$-orbit of 3 is / 3,4/9,2/9,1/9, 8, 4,2, 1"; "and the $[\,?\,]$-orbit of 3 is / 3, 4 / 9 , 29 , 5 , 19 , 8 , 4, 13 ,2, 1" (stacked fractions in the source).
7. No proof, no citation, no student or coauthor on these slides.

### 2.2 Reconstruction

The preceding section defines $\Omega = \Phi \circ V \circ \Phi^{-1}$, with $\Phi^{-1}$ the parity vector map, $V$ the bit complement and $\Phi$ its inverse (Bernstein, Lagarias), so that $\Omega(x)$ is the 2-adic integer whose $T$-parity vector is the complement of that of $x$. Computing $\Omega$ to $2^{54}$ (`omega.py`) gives $\Omega(1) = 2$, $\Omega(3) = -4/9$, $\Omega(5) = -2/9$, $\Omega(8) = -1/9$, $\Omega(1/3) = 4$, $\Omega(8/5) = -11/3$. With these values every fragment above is consistent with the following, once minus signs and the symbol for $\Omega$ are restored, and with nothing else I could find:

1. $\tau(x) = x/2$ if $x$ is even and $\tau(x) = \Omega(x)$ if $x$ is odd.
2. $x \sim y$ iff $x = y$ or $x = \Omega(y)$.
3. $\mathbb{Z}_2/\!\sim\; = \{\{x, \Omega(x)\} : x \in \mathbb{Z}_2,\ x \text{ odd}\}$; this is a correct description because $\Omega$ is an involution that "pairs evens with odds" (SlidesV4 page 119), so each class has exactly one odd element.
4. The induced map $\bar T[x] = [T(x)]$.
5. The $\tau$-orbit of $3$ is $3, -4/9, -2/9, -1/9, 8, 4, 2, 1$ (the script reproduces exactly this), and the orbit of the class of $3$ is $\{3, -4/9\}, \{5, -2/9\}, \{8, -1/9\}, \{4, 1/3\}, \{2, 1\}$, which is the stacked line "3, 4/9, 2/9, 5, 1/9, 8, 4, 1/3, 2, 1" read in pairs.

So the map on $\mathbb{Z}_2/\!\sim$ is $T$ itself pushed to the quotient, not a sign-respecting version of $3x+1$; there is no $3|x|+1$ and no $3x + \mathrm{sgn}(x)$ anywhere. The word "parity neutral" refers to $\tau$: both of its branches are free of the arithmetic of $3x+1$ (halving, and the symmetry $\Omega$), and the identity $T(x) = \Omega(\Omega(x)/2)$ for odd $x$ (from $T\Omega = \Omega T$ and $\Omega(x)$ even) hides the $3x+1$ inside $\Omega$.

This reconstruction rests on the extracted text and on my computation of $\Omega$, not on seeing the slide. It should be checked against the slide image or against Monks and Yazinski 2004 before it is quoted in a paper.

### 2.3 Why the equivalence holds (my sketch; the slides give none)

$\Omega$ commutes with $T$, so $T$ sends $\sim$-classes to $\sim$-classes and $\bar T$ is well defined. The $\tau$-orbit of $x$ visits, in order, either $T^k(x)$ or $\Omega(T^k(x))$ for each $k$, since $\tau$ on even $x$ is $T$ and on odd $x$ stays in the class. The class of $1$ is $\{1, 2\}$, and both $\tau$ and $T$ send $2$ to $1$. So for a positive integer $n$: the $T$-orbit reaches $1$ iff it meets $\{1, 2\}$ iff the class orbit reaches $[1]$ (c) iff the $\tau$-orbit meets $\{1, 2\}$, hence contains $1$ (b). This is our transfer theorem (Theorem 3.2 of `quotient.md`) for the group $\langle\Omega\rangle$ in place of $U$.

## 3. Other material in the slides that bears on our work

### 3.1 GermanySlides ("A Category of Topological Spaces Encoding Acyclic Set-Theoretic Dynamics (and other Collatz fun)", Ken Monks, University of Scranton)

1. Page 3, "2 Undergraduate Papers": "Joseph, J.; A Chaotic Extension of the 3x + 1 Function to $\mathbb{Z}_2[i]$, Fibonacci Quarterly, 36.4 (Aug 1998), 309-316" and "Fraboni, M.; Conjugacy and the 3x+1 Conjecture, submitted".
2. Page 5, "4 J. Joseph": "Even and odd correspond to equivalence classes in $\mathbb{Z}/2\mathbb{Z}$." "$\mathbb{Z}_2[i]/2\mathbb{Z}_2[i] = \{[0],[1],[i],[1+i]\}$". "Definition: Let $\widetilde T : \mathbb{Z}_2[i] \to \mathbb{Z}_2[i]$ by $\widetilde T(x) = \frac{x}{2}$ if $x \in [0]$, $\frac{3x+1}{2}$ if $x \in [1]$, $\frac{3x+i}{2}$ if $x \in [i]$, $\frac{3x+1+i}{2}$ if $x \in [1+i]$."
3. Page 6, "4.1 A Nontrivial Matter?", "Theorem (J. Joseph) (a) $\widetilde T|\mathbb{Z}_2 = T$. (i.e. it is an extension) (b) $\widetilde T$ is not conjugate to $T \times T$ via a $\mathbb{Z}_2$-module isomorphism. (i.e. it is nontrivial) (c) $\widetilde T$ is topologically conjugate to $T \times T$. (d) $\widetilde Q$ is a homeomorphism. (e) $\widetilde T : \mathbb{Z}_2[i] \to \mathbb{Z}_2[i]$ is chaotic."
4. Page 7, "4.2 Some Empirical Results on $\widetilde T|\mathbb{Z}[i]$ (An Extended Finite Cycles Conjecture?)": a table of cycle counts by period for $T|\mathbb{Z}$ and $\widetilde T|\mathbb{Z}[i]$ (period 1: 2 and 4; 2: 1 and 3; 3: 1 and 9; 5: 0 and 2; 8: 0 and 10; 11: 1 and 5*; 19: 0 and 24*; 46: 0 and 2*; 103: 0 and 2*; "*Empirical search only.").
5. Pages 9 and 10: "Theorem (Joseph, Monks)": an explicit entire $F : \mathbb{C} \to \mathbb{C}$, built by interpolation along a spiral enumeration of $\mathbb{Z}[i]$, with "$F$ is an entire function which extends $\widetilde T|\mathbb{Z}[i]$." Page 8 quotes Chamberland's entire extension of $T|\mathbb{Z}$.
6. Pages 11 to 13, "6 M. Fraboni", "6.1 Approach: attack via conjugacies" and "6.2 Nice Conjugates and Linear Conjugacies": $f_{a,b,c,d}(x) = \frac{ax+b}{2}$ if $x$ is even, $\frac{cx+d}{2}$ if $x$ is odd; "$\mathcal{F} = \{f_{a,b,c,d} : a, c, d$ are odd and $b$ is even$\}$"; "Theorem (Fraboni) (a) $f_{a,b,c,d}$ is conjugate to $T$ if and only if $f \in \mathcal{F}$. (b) Every $f \in \mathcal{F}$, is topologically conjugate to $T$. (c) $s$ is conjugate to $T$ via a linear conjugacy $h(x) = px + q$ if and only if $s = f_{1,q,3,p-q}$, with $p$ odd and $q$ even, or $s = f_{3,p-q,1,q}$ with $p$ and $q$ both odd."
7. Page 15, "7.1 More members of our cast": "The dynamical systems Dyn$(X,f)$, Dyn$(Y,g)$ are said to be semi-conjugate if there exists a map $h : X \to Y$ such that [$h \circ f = g \circ h$] commutes. In this situation $h$ is called a semiconjugacy."
8. Page 18, "7.2.1 Properties of SetDyn and ADyn": "(b) Semiconjugacies map cyclic points of order $k$ to cyclic points of order $d$ for some $d$ dividing $k$." "(c) Semiconjugacies map orbits to orbits".
9. Page 25: "Corollary: If $h$ is a semiconjugacy from Dyn$(X,f)$ onto Dyn$(\mathbb{Z}^+, T|\mathbb{Z}^+)$ and Top$(X,\tau_f)$ is connected, then the Collatz conjecture is true." and the converse-direction corollary for a semiconjugacy from Dyn$(\mathbb{Z}^+, T|\mathbb{Z}^+)$.

Nothing in GermanySlides on $x \mapsto -x$, $\mathbb{Z}_2/\!\sim$, odd maps, units or associates.

### 3.2 SlidesV4 and TalkV4 ("On the distribution of arithmetic sequences in the Collatz graph", Keenan Monks, Ken G. Monks, Ken M. Monks, Maria Monks)

TalkV4 is the first four slides and the two reference slides of the same talk; its pages 5 and 6 list, among others, "Monks, K. G., Yasinski, J., The Autoconjugacy of the 3x + 1 function, Discrete Math. 275 (2004), 219-236" (spelled Yasinski there), "Kraft, B., Monks, K., On Conjugacies of the 3x + 1 Map Induced by Continuous Endomorphisms of the Shift Dynamical System, Discrete Math. 310 (2010), 1875-1883" and "Monks, M., Endomorphisms of the shift dynamical system, discrete derivatives, and applications, Discrete Math. 309 (2009), 5196-5205". SlidesV4:

1. Page 112: "Theorem. (Bernstein, Lagarias.) Inverse parity vector function $\Phi : \mathbb{Z}_2 \to \mathbb{Z}_2$ is well defined, and $T = \Phi \circ \sigma \circ \Phi^{-1}$."
2. Page 115: "In 2004, K. G. Monks and J. Yasinski used $V$ to construct the unique nontrivial autoconjugacy of $T$: $\Omega := \Phi \circ V \circ \Phi^{-1}$." (Page 114: Hedlund's "only two automorphisms ... are the identity map and the bit complement map $V$".)
3. Pages 117 to 120: "$\Omega$ is solenoidal, that is, it induces a permutation on $\mathbb{Z}/2^n\mathbb{Z}$ for all $n$." "$\Omega$ is also an involution that pairs evens with odds." "We say that, mod 8, $\Omega(3) = 4$."
4. Page 124: "Theorem For any $n \ge 1$, the graph $\Gamma_{2^n}$ is self-color-dual. Idea of proof: if we replace each label $a$ with $\Omega(a) \bmod 2^n$, we get the color dual of $\Gamma_{2^n}$."
5. Pages 126 to 130: "Discrete derivative map: $D : \mathbb{Z}_2 \to \mathbb{Z}_2$ by $D(a_0a_1a_2\ldots) = d_0d_1d_2\ldots$ where $d_i = |a_i - a_{i+1}|$", "$R := \Phi \circ D \circ \Phi^{-1}$ is an endomorphism of $T$." "(M., 2009.) $R$ is a two-to-one map, and $R(\Omega(x)) = R(x)$ for all $x$." "Can use $R$ to "fold" $\Gamma_{2^{n+1}}$ onto $\Gamma_{2^n}$ by identifying $\Omega$-pairs."
6. Pages 132 to 134: "Folding $\Gamma_8$ onto $\Gamma_4$", with the $\Omega$-pairs $\{0,7\}, \{1,2\}, \{3,4\}, \{5,6\}$ as the vertices of the folded graph, relabelled $0, 1, 2, 3$.

Item 5 says more than the survey slide. $D$ is onto and two-to-one with $D \circ V = D$, so $R$ is onto with fibres exactly the pairs $\{x, \Omega(x)\}$, and $R \circ T = T \circ R$. Hence $R$ induces a bijection $\mathbb{Z}_2/\!\sim\; \to \mathbb{Z}_2$ that conjugates the parity neutral map $\bar T$ to $T$ itself. The quotient of $T$ by its autoconjugacy is again $T$ (my inference from pages 128 to 130; the slides do not say it in these words). It does not carry positive integers to positive integers, so it does not make the parity neutral conjecture trivially the original one, but it explains why the quotient gives no simplification. Item 6 is the same quotient at the level of residues modulo $2^n$.

Nothing in SlidesV4 or TalkV4 on $x \mapsto -x$, odd maps, units, Gaussian or ring versions.

### 3.3 The survey deck (through WebFetch only)

Besides the parity neutral section: "The Nontrivial Autoconjugacy of T" (with "Results from: Monks, K. and Yazinski, J.; The Autoconjugacy of the 3x 1 Function, to appear in Discrete Math." and the worked examples $\Omega(-11/3) = 8/5$ and $\Omega(3) = -4/9$, signs restored, which `omega.py` confirms), "Application to Divergent Orbits" (an "Autoconjugacy Conjecture" that $\Omega$ preserves the odd-denominator rationals, related to the absence of divergent orbits), "Application to Cycles", "Yazinski - Work on the fixed point conjecture", "Kucinski - Cycles in T|Z[i]" ("T|Z[i] has exactly 77 distinct cycles of period" at most 400, as `literature.md` records), and Joseph's extension and the analytic extension as in GermanySlides. The Monks and Yazinski abstract also mentions self-conjugate cycles ($\Omega C = C$), that they contain only positive elements, and that $\{1, 2\}$ is the only self-conjugate cycle of integers.

## 4. Comparison with the quotient viewpoint

### 4.1 What Monks and Yazinski have

1. A symmetry of a Collatz map (an involution $\Omega$ of $\mathbb{Z}_2$ with $\Omega T = T\Omega$), the quotient of the space by the group it generates, the induced map $[x] \mapsto [Tx]$, and the restatement of the Collatz conjecture as "the orbit of the class of every positive integer contains the class of 1". This is the shape of our Proposition 2.2(1),(2) (descent for a commuting map) and Theorem 3.2 (transfer), for one group of order $2$.
2. A fundamental domain for that group: the odd 2-adic integers, one per class, which is how the slide describes $\mathbb{Z}_2/\!\sim$. The map $\tau$ is a map on $\mathbb{Z}_2$ whose orbits move through the classes of the $T$-orbit, a different device from our $\mathrm{rep}_F$ but in the same spirit.
3. The cycle $\{1,2\}$ of $T$ lies over the fixed class $[1] = \{1,2\}$ with $T(1) = \Omega(1)$, and self-conjugate cycles ($\Omega C = C$) are the cycles stabilized by the group. These are the $\langle\Omega\rangle$ cases of our Theorem 4.2 and Section 6.3: the cycle over $[1]$ with $T^p(1) = \varepsilon \cdot 1$, and $vC = C$ iff $v \in \langle\varepsilon\rangle$. Their result that $\{1,2\}$ is the only self-conjugate integer cycle has no counterpart in our notes.
4. Finite-level quotients ($\Gamma_{2^{n+1}}$ folded onto $\Gamma_{2^n}$ by $\Omega$-pairs, via M. Monks 2009) and the semiconjugacy language of GermanySlides page 15, in which our quotient map $z \mapsto [z]$ is a semiconjugacy and Theorem 4.1(2) of `quotient.md` is GermanySlides page 18 item (b) for this map.

### 4.2 What they do not have

1. The symmetry $x \mapsto -x$. $T$ does not commute with $-1$ ($T(5) = 8$, $T(-5) = -7$), and in the slides $-x$ appears only through conjugacy: Fraboni's theorem (c) with $p = -1$, $q = 0$ gives $f_{1,0,3,-1}$, the map $3x - 1$ (GermanySlides page 13), which is the statement that $x \mapsto -x$ conjugates $3x+1$ to $3x-1$, not a symmetry of either. So nothing in these slides says $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, or that Collatz's map on $\mathbb{N}$ is the quotient of $3z + \mathrm{sgn}(z)$, or that $7x \pm 1$ commutes with $-1$.
2. Linear or ring symmetries. $\Omega$ is a homeomorphism of $\mathbb{Z}_2$, not multiplication by a unit; it is not additive, does not preserve $\mathbb{Z}$ ($\Omega(3) = -4/9$), and it swaps parity, whereas units preserve it. The positive integers are not a union of $\sim$-classes ($\{3, -4/9\}$), which is why the slide states (b) and (c) for positive integers inside $\mathbb{Z}_2$ rather than as a map on a quotient of $\mathbb{N}$.
3. Rings and units beyond $\pm 1$. The Gaussian material is Joseph's $\widetilde T$ on $\mathbb{Z}_2[i]$ and Kucinski's cycles. $\widetilde T$ has digits read modulo $2$ (that is, $(1+i)^2$), it satisfies $\widetilde T(iz) = i\widetilde T(z)$ on the class $[1]$ but $\widetilde T(-z) \ne -\widetilde T(z)$, so it does not respect associates (Corollary 5.2(1) of `quotient.md`: nothing at level $k \le 2$ does), and no quotient $\mathbb{Z}[i]/U$ is formed. The cycle counts on GermanySlides page 7 are for $\mathbb{Z}[i]$, not for classes.
4. The distinction between Collatz-type maps (defined on a set of representatives, digit read from the representative) and Barina-type maps (residue-class maps commuting with the units), Theorem 5.1 (for residue-class maps, respecting associates is commuting), Proposition 5.3 and the twisted maps, Corollary 4.3 of `fundamental-domain.md` (no residue-class map induces 3x+1 on $\mathbb{N}$), the count $|U|/\mathrm{ord}(\varepsilon)$ of lifted cycles for a group larger than $\mathbb{Z}/2$, the rotation and mirror corollaries, and all results on $\mathbb{Z}[i]/U$ (the 5+2i map as the first quotient of a map of our form, the cone and quadrant maps at $(1+i)^2$).
5. The respects-versus-commutes distinction itself. With a single involution that commutes with $T$ by construction the question does not arise.

### 4.3 How our paper should credit them

1. Cite the paper, not the slides: K. G. Monks and J. Yazinski, The autoconjugacy of the $3x+1$ function, Discrete Math. 275 (2004), 219 to 236. It is already in `papers/collatz-rings.md` as [unverified: known through Lagarias's second bibliography]; TalkV4 page 6 confirms the volume, year and pages, so the tag can become "verified from Monks's reference list" (the paper text is still unread).
2. Suggested wording for the quotient section: "Passing to the quotient by a symmetry that commutes with a Collatz map is not new: Monks and Yazinski [MY04] identify each 2-adic integer with its image under the nontrivial autoconjugacy $\Omega$ of $T$ and restate the $3x+1$ conjecture on the quotient, in what they call a parity neutral form. Their symmetry is a nonlinear involution of $\mathbb{Z}_2$ that exchanges parities. Ours is multiplication by the units of the ring; for $\mathbb{Z}$ it gives $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, and in general $R/U$." If the paper cites M. Monks 2009, add that the quotient of $T$ by $\Omega$ is conjugate to $T$ through the discrete-derivative endomorphism.
3. Do not attribute $x \sim -x$ to Monks, and do not cite the survey slides for it. The existing sentence in `papers/collatz-rings.md` line 440 ("Monks and Yazinski find an autoconjugacy of $3x+1$ ... neither changes the multiplier") stays correct; the new citation belongs in the quotient section, and the claims "not found" in `quotient.md` Section 10 (the phrase $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, the descent failure of $3x+1$, Theorem 5.1) still stand after reading these slides.
4. Before submission, read the Monks and Yazinski paper itself for the exact parity neutral statement and for any remark on $x \mapsto -x$, and look at the survey slide as an image to confirm the reconstruction of Section 2.2.

## 5. Corrections needed elsewhere (not made here)

1. `fundamental-domain.md` Section 1 item 8: "Monks's survey slides have a section on $\mathbb{Z}_2/\!\sim$ with $x \sim -x$" should read "with $x \sim \Omega(x)$, $\Omega$ the Monks and Yazinski autoconjugacy".
2. `fundamental-domain.md` Section 10 item 3: the relation is $x = y$ or $x = \Omega(y)$, the map is $T$ itself on classes (with the auxiliary $\tau$), the source is Monks and Yazinski 2004, and it is not prior art for $\mathbb{Z}/\{\pm 1\}$ but for the general quotient idea. The "Not reached" line can drop the parity neutral source.
