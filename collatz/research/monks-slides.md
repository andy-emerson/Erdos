# K. G. Monks's talk slides and the quotient viewpoint

Working notes, 6 November 2026. Nothing here is in the paper drafts or the catalog. Context: `quotient.md` (A1 to A14) and `fundamental-domain.md`. Sources, none copied into the repo: the survey deck "The 3x+1 Problem (or how to assign intractable open questions to undergraduates)" (`Slides.pdf`, 18 pages, the copy at https://monks.scranton.edu/files/talks/CollatzSurvey/Slides.pdf, supplied by the author), which contains the Parity Neutral Collatz section; and three other decks, `GermanySlides.pdf` (26 pages), `SlidesV4.pdf` (141 pages) and `TalkV4.pdf` (6 pages), which do not. All quotations from the survey deck below were transcribed from the page images (PDF pages 7 to 13 and 1 to 6, 14 to 18 read as images). The decks have no slide numbers; "page" means PDF page, and a page of the survey deck holds several slide boxes. A check script, `omega.py`, is in the session scratchpad, not in the repo.

## 1. Summary

1. The section "Parity Neutral Collatz" is on pages 10 and 11 of the survey deck, between "The Nontrivial Autoconjugacy of T" (pages 8 to 10) and "Application to Divergent Orbits" (page 12).
2. The relation is $x \sim y \Leftrightarrow (x = y \text{ or } x = \Omega(y))$, where $\Omega = \Phi \circ V \circ \Phi^{-1}$ is the nontrivial autoconjugacy of $T$ from Monks and Yazinski. It is not $x \sim -x$. The map on classes is $\Psi([x]) = [T(x)]$, $T$ itself pushed to the quotient. The auxiliary map is $\xi(x) = x/2$ on even $x$, $\Omega(x)$ on odd $x$. The theorem states that the Collatz conjecture, "the $\xi$-orbit of any positive integer contains 1" and "the $\Psi$-orbit of the class of any positive integer contains $[1]$" are equivalent. No proof is on the slides. The section names no one; the autoconjugacy section it builds on begins "Results from: Monks, K. and Yazinski, J.; The Autoconjugacy of the 3x + 1 Function, to appear in Discrete Math" (page 9).
3. The reconstruction in the first version of this note, made from text extraction before the PDF was available, is confirmed in every point; only the names differ (the slide writes $\xi$ where I wrote $\tau$, and $\Psi$ for the induced map). The earlier entry in `fundamental-domain.md` saying the section uses $x \sim -x$ is wrong (Section 6).
4. The deck has nothing on $x \sim -x$, odd maps, units or associates. The only "negation" in it is Hedlund's bit complement $V(x) = -1 - x$ (page 9), the shift's automorphism, which $\Phi$ transports to $\Omega$. The only ring extension is Joseph's map on $\mathbb{Z}_2[i]$, which does not respect associates.
5. Monks and Yazinski restate the conjecture on the quotient of $\mathbb{Z}_2$ by a symmetry commuting with $T$, the involution $\Omega$. That is the same kind of restatement as ours, and it predates ours; ours uses the unit group as the symmetry, which is larger in general, simpler (multiplication rather than a transported shift automorphism), and present in every ring. Section 5 gives the proposed credit.

## 2. The Parity Neutral Collatz section (survey deck)

### 2.1 The preceding section, "The Nontrivial Autoconjugacy of T"

Page 7 defines the shift and the parity vector map: "Definition (Lagarias) Define the parity vector map, $\Phi^{-1} : \mathbb{Z}_2 \to \mathbb{Z}_2$ by $\Phi^{-1}(x) = v_0v_1v_2\ldots_{(2)}$ where $v_i \in \{0,1\}$ and $v_i \equiv_2 T^i(x)$ for all $i \in \mathbb{N}$", "$\Phi^{-1}$ is a topological conjugacy between $T$ and $\sigma$ ! (Lagarias)", and Bernstein's formula for $\Phi$. Page 8 ends with the heading. Page 9, verbatim:

> Results from: Monks, K. and Yazinski, J.; The Autoconjugacy of the $3x + 1$ Function, to appear in Discrete Math
>
> Hedlund (1969): $\mathrm{Aut}(\sigma) = \{id, V\}$ where $V(x) = -1 - x$ and $id$ is the identity map
>
> $V(x)$ is the 2-adic whose digits are the bit-complement of the digits of $x$
>
> Example $V(\overline{11010}_{(2)}) = \overline{00101}_{(2)}$
>
> Q: What is $\mathrm{Aut}(T)$?
>
> Notice [a commuting diagram: $T$ on top, $\Phi^{-1}$ down to $\sigma$, $V$ down to $\sigma$, $\Phi$ down to $T$] commutes.
>
> Definition Define $\Omega := \Phi \circ V \circ \Phi^{-1}$. We call $\Omega$ the nontrivial autoconjugacy of $T$.
>
> Answer: $\mathrm{Aut}(T) = \{id, \Omega\}$
>
> Facts about $\Omega$: $\Omega^2 = id$ and $\Omega \circ T = T \circ \Omega$. $\Omega$ maps a 2-adic integer $x$ to the unique 2-adic integer $\Omega(x)$ whose parity vector is the one's complement of the parity vector of $x$, i.e. all corresponding terms in the $T$-orbits of $x$ and $\Omega(x)$ have opposite parity.

Page 10, top: "The $T$-orbit of $-11/3$ is $-\frac{11}{3}, \overline{-5, -7, -10}$ and the $T$-orbit of $8/5$ is $\frac{8}{5}, \overline{\frac{4}{5}, \frac{2}{5}, \frac{1}{5}}$. By uniqueness we conclude that $\Omega(-11/3) = 8/5$." Then: "Example Suppose we wish to compute $\Omega(3)$. The $T$-orbit of $3$ is $3, 5, 8, 4, \overline{2, 1}$ so that $\Phi^{-1}(3) = 1100\overline{01}$ and its one's complement is $V \circ \Phi^{-1}(3) = 0011\overline{10}$. By Bernstein's formula for $\Phi$ we obtain $\Omega(3) = \Phi \circ V \circ \Phi^{-1}(3) = \Phi(0011\overline{10}) = -\frac{4}{9}$ whose $T$-orbit is $-\frac{4}{9}, -\frac{2}{9}, -\frac{1}{9}, \frac{1}{3}, \overline{1, 2}$."

(Here $T(x) = x/2$ for even $x$ and $(3x+1)/2$ for odd $x$, page 1.)

### 2.2 The section, verbatim (pages 10 and 11)

Page 10, under the heading "Parity Neutral Collatz":

> Definition Let $\xi : \mathbb{Z}_2 \to \mathbb{Z}_2$ by $\xi(x) = \frac{x}{2}$ if $x$ is even, $\Omega(x)$ if $x$ is odd, for all $x \in \mathbb{Z}_2$.
>
> Example

Page 11 continues the example with a ladder diagram: $3 \overset{\Omega}{\leftrightarrow} -4/9$, $5 \overset{\Omega}{\leftrightarrow} -2/9$, $8 \overset{\Omega}{\leftrightarrow} -1/9$, $4 \overset{\Omega}{\leftrightarrow} 1/3$, $2 \overset{\Omega}{\leftrightarrow} 1$, with vertical arrows $T\downarrow$ on both sides between consecutive rows. Then:

> Definition Define $\sim$ on $\mathbb{Z}_2$ by $x \sim y \Leftrightarrow (x = y \text{ or } x = \Omega(y))$ for all $x, y \in \mathbb{Z}_2$
>
> $\sim$ is an equivalence relation on $\mathbb{Z}_2$
>
> $\mathbb{Z}_2/\!\sim\; = \{\{x, \Omega(x)\} : x \in \mathbb{Z}_2 \text{ and } x \text{ is odd}\}$
>
> Example $[3] = [-4/9] = \{3, -4/9\}$
>
> Definition $\Psi : \mathbb{Z}_2/\!\sim\; \to \mathbb{Z}_2/\!\sim$ by $\Psi([x]) = [T(x)]$ for all $x \in \mathbb{Z}_2$.
>
> Theorem The following are equivalent.
> (a) The Collatz Conjecture.
> (b) The $\xi$-orbit of any positive integer contains $1$.
> (c) The $\Psi$-orbit of the class of any positive integer contains $[1]$.
>
> Example The $T$-orbit of $3$ is $3, 5, 8, 4, \overline{2, 1}$ while the $\xi$-orbit of $3$ is $3, -4/9, -2/9, -1/9, 8, 4, \overline{2, 1}$ and the $\Psi$-orbit of $[3]$ is $\{3, -\frac{4}{9}\}, \{-\frac{2}{9}, 5\}, \{-\frac{1}{9}, 8\}, \{4, \frac{1}{3}\}, \overline{\{2, 1\}}$

The next heading, "Application to Divergent Orbits" (page 12), states "Conjecture Autoconjugacy Conjecture: $\Omega(\mathbb{Q}_{odd}) \subseteq \mathbb{Q}_{odd}$" and "Theorem The following are equivalent. (a) The Periodicity Conjecture. (b) The Autoconjugacy Conjecture. (c) No oddrat has a divergent $T$-orbit. Furthermore, the statement $\Omega(\mathbb{Z}^+) \subseteq \mathbb{Q}_{odd}$ is equivalent to the Divergent Orbits Conjecture." "Application to Cycles" (pages 12 and 13): "Definition $T$-cycle $C$ is self conjugate if $\Omega(C) = C$." "Example $\{1, 2\}$ is a self-conjugate $T$-cycle." A characterization of self-conjugate cycles as orbits of $\Phi(\overline{v_0 \cdots v_k v_0^* \cdots v_k^*})$, a table of self-conjugate cycles with at most ten elements, "Theorem If $C$ is a self conjugate $T$-cycle then $C \subseteq \mathbb{Q}^+_{odd}$", and "Theorem For any self conjugate $T$-cycle $C$, $0 < \min(C) \le 1 < \max(C)$. Hence, the only self conjugate $T$-cycle of integers is $\{1, 2\}$."

### 2.3 Check against the reconstruction

1. Relation: as reconstructed, $x \sim y$ iff $x = y$ or $x = \Omega(y)$.
2. $\Omega$: as reconstructed, $\Phi \circ V \circ \Phi^{-1}$, an involution commuting with $T$. The values computed earlier by `omega.py` ($\Omega(3) = -4/9$, $\Omega(5) = -2/9$, $\Omega(8) = -1/9$, $\Omega(1/3) = 4$, $\Omega(1) = 2$, $\Omega(8/5) = -11/3$) all agree with the slide.
3. Map on classes: as reconstructed, $[x] \mapsto [T(x)]$; the slide calls it $\Psi$. This $\Psi$ is unrelated to the 3-adic map $\Psi$ of SlidesV4 page 70.
4. Auxiliary map: as reconstructed, with the name $\xi$ instead of $\tau$. The $\xi$-orbit of $3$ on the slide is exactly the one `omega.py` produced.
5. Equivalence: as reconstructed, items (a), (b), (c).
6. Proof and attribution: none on the section's slides; attribution only through the page 9 header and the paper. The Monks and Yazinski abstract (https://monks.scranton.edu/files/pubs/autoconjabs.txt, read through WebFetch) says they use $\Omega$ to restate the conjecture "in a parity neutral form", so the theorem is from that paper, Discrete Math. 275 (2004), 219 to 236 (volume and pages from TalkV4 page 6, which spells the name "Yasinski").

### 2.4 Why the equivalence holds (my sketch; the slides give none)

$\Omega$ commutes with $T$, so $T$ maps classes to classes and $\Psi$ is well defined. $\Omega$ exchanges parity ("all corresponding terms ... have opposite parity", so in particular $x$ and $\Omega(x)$), hence each class has one odd element, which is the slide's description of $\mathbb{Z}_2/\!\sim$. On even $x$, $\xi = T$; on odd $x$, $\xi$ stays in the class. So the $\xi$-orbit of $x$ meets, in order, one element of each class $[T^k(x)]$, and $[\xi$-orbit$]$ is the $\Psi$-orbit with repetitions. The class $[1]$ is $\{1, 2\}$, and $\xi(2) = 1$. For a positive integer $n$ the $T$-orbit reaches $1$ iff it meets $\{1, 2\}$ iff the $\Psi$-orbit of $[n]$ reaches $[1]$ iff the $\xi$-orbit meets $\{1,2\}$ iff it contains $1$. This is Theorem 3.2 of `quotient.md` with $\langle\Omega\rangle$ in place of $U$. The name "parity neutral" fits $\xi$: neither branch involves $3x+1$, which is hidden in $\Omega$ through $T(x) = \Omega(\Omega(x)/2)$ for odd $x$.

## 3. Other material bearing on our work

### 3.1 The survey deck, other sections

1. Page 2, "Finite Cycles Conjecture: The only $T$-cycles of integers are: $\{1,2\}$, $\{0\}$, $\{-1\}$, $\{-5,-7,-10\}$, $\{-17, -25, -37, -55, -82, -41, -61, -91, -136, -68, -34\}$". The negative cycles are listed, and no relation between positive and negative orbits is drawn.
2. Page 3, student publications: Farruggia, Lawrence and Waterhouse; Fusaro; "Joseph, J.; A Chaotic Extension of the $3x+1$ Function to $\mathbb{Z}_2[i]$, Fibonacci Quarterly, 36.4 (Aug 1998), 309-316"; "Fraboni, M.; Conjugacy and the $3x+1$ Conjecture (1998 MAA EPADEL section student paper competition winner)"; "Kucinski, G.; Cycles for the $3x+1$ Map on the Gaussian Integers, to appear, Pi Mu Epsilon Journal"; "Yazinski, J.; Elimination of $\Phi$-fixed point candidates (in preparation)"; and among publications "Monks, K. and Yazinski, J.; The Autoconjugacy of the $3x+1$ Function, to appear in Discrete Math".
3. Page 8, "Fraboni - Classification of Linear Conjugacies": modular functions $f_{a,b,c,d}$; "Theorem (Fraboni) (1) A modular function $f$ is conjugate to $T$ if and only if $f \in \mathcal{F}$. (2) Every element of $\mathcal{F}$ is topologically conjugate to $T$. (3) Every function that is conjugate to $T$ by a linear map is in $\mathcal{F}$." "Example $T = f_{1,0,3,1}$ and $\sigma = f_{1,0,1,-1}$ are both in $\mathcal{F}$".
4. Pages 14 and 15, "Distant Cousins - Changing Categories": the definition of semi-conjugacy, the induced topology $\tau_f$, "Semiconjugacies are continuous with respect to the induced topologies", "The Collatz conjecture is true if and only if the topological space $(\mathbb{Z}^+, \tau_T)$ is connected."
5. Pages 15 to 17: Joseph's $\widetilde T$ on $\mathbb{Z}_2[i]$ ($\frac{x}{2}$, $\frac{3x+1}{2}$, $\frac{3x+i}{2}$, $\frac{3x+1+i}{2}$ on $[0], [1], [i], [1+i]$ in $\mathbb{Z}_2[i]/2\mathbb{Z}_2[i]$); "Theorem (Kucinski) $\widetilde T|\mathbb{Z}[i]$ has exactly 77 distinct cycles of period less than or equal to 400" with a table (periods 1, 2, 3, 5, 8, 11, 19, 46, 84, 103; $\widetilde T|\mathbb{Z}[i]$ counts 4, 3, 9, 2, 10, 5, 30, 2, 10, 2), which differs from the GermanySlides table (period 19: 24 there, marked empirical; period 84 absent there); and "Theorem (Joseph, Monks)", the entire extension.

### 3.2 GermanySlides ("A Category of Topological Spaces Encoding Acyclic Set-Theoretic Dynamics (and other Collatz fun)")

1. Pages 5 to 7: Joseph's $\widetilde T$ as above, and "Theorem (J. Joseph) (a) $\widetilde T|\mathbb{Z}_2 = T$. (i.e. it is an extension) (b) $\widetilde T$ is not conjugate to $T \times T$ via a $\mathbb{Z}_2$-module isomorphism. (i.e. it is nontrivial) (c) $\widetilde T$ is topologically conjugate to $T \times T$. (d) $\widetilde Q$ is a homeomorphism. (e) $\widetilde T : \mathbb{Z}_2[i] \to \mathbb{Z}_2[i]$ is chaotic."; the cycle table "(An Extended Finite Cycles Conjecture?)".
2. Pages 11 to 13, Fraboni, in a different form: "(c) $s$ is conjugate to $T$ via a linear conjugacy $h(x) = px + q$ if and only if $s = f_{1,q,3,p-q}$, with $p$ odd and $q$ even, or $s = f_{3,p-q,1,q}$ with $p$ and $q$ both odd." With $p = -1$, $q = 0$ this is the conjugacy of $T$ by $x \mapsto -x$ to the $3x - 1$ map: $-x$ appears as a conjugacy, not a symmetry.
3. Pages 15 and 18: semiconjugacy, and "(b) Semiconjugacies map cyclic points of order $k$ to cyclic points of order $d$ for some $d$ dividing $k$." Our quotient map $z \mapsto [z]$ is a semiconjugacy, and this is the divisibility in Theorem 4.1(2) of `quotient.md`.

### 3.3 SlidesV4 and TalkV4 ("On the distribution of arithmetic sequences in the Collatz graph", Keenan, Ken G., Ken M. and Maria Monks)

1. SlidesV4 page 115: "In 2004, K. G. Monks and J. Yasinski used $V$ to construct the unique nontrivial autoconjugacy of $T$: $\Omega := \Phi \circ V \circ \Phi^{-1}$." Pages 117 to 120: "$\Omega$ is solenoidal, that is, it induces a permutation on $\mathbb{Z}/2^n\mathbb{Z}$ for all $n$." "$\Omega$ is also an involution that pairs evens with odds." Page 124: $\Gamma_{2^n}$ is self-color-dual via $a \mapsto \Omega(a) \bmod 2^n$.
2. Pages 126 to 130: "$R := \Phi \circ D \circ \Phi^{-1}$ is an endomorphism of $T$", "(M., 2009.) $R$ is a two-to-one map, and $R(\Omega(x)) = R(x)$ for all $x$.", "Can use $R$ to "fold" $\Gamma_{2^{n+1}}$ onto $\Gamma_{2^n}$ by identifying $\Omega$-pairs." Pages 132 to 134 fold $\Gamma_8$ onto $\Gamma_4$ with classes $\{0,7\}, \{1,2\}, \{3,4\}, \{5,6\}$. Since the discrete derivative $D$ is onto, two-to-one and $D \circ V = D$, the fibres of $R$ are exactly the $\Omega$-pairs and $R$ commutes with $T$; so $R$ induces a bijection $\mathbb{Z}_2/\!\sim\; \to \mathbb{Z}_2$ conjugating $\Psi$ to $T$. The quotient of $T$ by its autoconjugacy is again $T$ (my inference; the slides do not say it in these words). It does not preserve the positive integers, so it does not make (c) trivially (a).
3. TalkV4 pages 5 and 6 are references: Monks and Yasinski 2004, Kraft and Monks 2010, M. Monks 2009 (Discrete Math. 309, 5196 to 5205), and others. Nothing else in these two decks on $-x$, units or rings.

## 4. Comparison with the quotient viewpoint

### 4.1 What Monks and Yazinski have

1. A symmetry of a Collatz map ($\Omega$, with $\Omega^2 = id$ and $\Omega T = T\Omega$), the quotient by the group it generates, the induced map $\Psi$, and the restatement of the conjecture as "the $\Psi$-orbit of the class of any positive integer contains $[1]$". That is our descent for a commuting map (Proposition 2.2(1),(2) of `quotient.md`) and the transfer theorem (Theorem 3.2) for one group of order $2$.
2. A fundamental domain for the group, the odd 2-adic integers (the slide's description of $\mathbb{Z}_2/\!\sim$), and a lift $\xi$ of the quotient dynamics to $\mathbb{Z}_2$.
3. The lifted cycles: $\{1,2\}$ lies over the fixed class $[1]$ with $T(1) = \Omega(1)$, and self-conjugate cycles ($\Omega(C) = C$) are those stabilized by the group. These are the $\langle\Omega\rangle$ cases of Theorem 4.2 and Section 6.3 of `quotient.md` (a cycle over $[1]$ with $T^p(1) = \varepsilon \cdot 1$; $vC = C$ iff $v \in \langle\varepsilon\rangle$). Their results that self-conjugate cycles are positive and that $\{1,2\}$ is the only self-conjugate integer cycle have no counterpart in our work.
4. With M. Monks 2009 and SlidesV4, finite-level quotients by $\Omega$ and the fact that the quotient map is itself an endomorphism of $T$.

### 4.2 What they do not have

1. The symmetry $x \mapsto -x$. $T$ does not commute with $-1$ ($T(5) = 8$, $T(-5) = -7$), and in all four decks $-x$ appears only as a conjugacy (Fraboni, $p = -1$) or in the affine form $V(x) = -1 - x$ on the shift side. Nothing says $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, that Collatz's map on $\mathbb{N}$ is the quotient of $3z + \mathrm{sgn}(z)$, or that $7x \pm 1$ commutes with $-1$.
2. A ring symmetry. $\Omega$ is a homeomorphism of $\mathbb{Z}_2$, not multiplication by a unit: it is not additive, exchanges parity where units preserve it, and does not preserve $\mathbb{Z}$ ($\Omega(3) = -4/9$), so the positive integers are not a union of classes and the restatement lives in $\mathbb{Z}_2$.
3. Rings and units beyond $\pm 1$. The Gaussian material is Joseph's $\widetilde T$ and Kucinski's cycles. $\widetilde T$ reads digits modulo $2 = -i(1+i)^2$; $\widetilde T(iz) = i\widetilde T(z)$ for $z \in [1]$, but $\widetilde T(-z) \ne -\widetilde T(z)$, so it does not respect associates (Corollary 5.2(1) of `quotient.md`), and no $\mathbb{Z}[i]/U$ appears.
4. Everything specific to associates: the Collatz-type versus Barina-type distinction, respects-iff-commutes for residue-class maps (Theorem 5.1), the twisted maps (Proposition 5.3), Corollary 4.3 of `fundamental-domain.md`, the cycle count $|U|/\mathrm{ord}(\varepsilon)$ for larger groups, the rotation and mirror corollaries, and the results on $\mathbb{Z}[i]/U$.

## 5. How the paper should credit Monks and Yazinski

The credit should be full and plainly stated, not a footnote that minimizes it. Monks and Yazinski are the earlier instance of the move our quotient section makes: they take a symmetry commuting with a Collatz map, pass to the quotient, and restate the conjecture there, with an equivalence theorem. Our contribution is the same kind of restatement for a different symmetry, the unit group, which is larger in general, simpler (multiplication by units rather than a symmetry transported from the shift), and exists in every ring; and the results that depend on that choice (descent of 3x+1 versus $7x \pm 1$, respects-iff-commutes, $R/U$ for $\mathbb{Z}[i]$).

1. Citation: K. G. Monks and J. Yazinski, The autoconjugacy of the $3x+1$ function, Discrete Math. 275 (2004), 219 to 236. It is in `papers/collatz-rings.md` with the tag [unverified: known through Lagarias's second bibliography]; the bibliographic data are now confirmed by TalkV4 page 6 and the survey deck, and the parity neutral statement by the survey deck pages 10 and 11. The paper text itself is still unread, so the citation should point to the paper for the theorem, and may add the slides for the formulation.
2. Proposed wording, at the start of the quotient section: "Restating the conjecture on a quotient by a symmetry is not new. Monks and Yazinski [MY04] showed that $T$ has exactly one nontrivial autoconjugacy, the involution $\Omega$ of $\mathbb{Z}_2$ that complements parity vectors, and restated the $3x+1$ conjecture on $\mathbb{Z}_2/\langle\Omega\rangle$ in a parity neutral form: the conjecture holds if and only if the orbit of the class of every positive integer under the induced map $[x] \mapsto [T(x)]$ contains the class of $1$. We make the same kind of restatement for a different and simpler symmetry, multiplication by the units, which every ring has. For $\mathbb{Z}$ it gives $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, and in general $R/U$."
3. Where it helps, also mention their self-conjugate cycles as the analogue of our cycles stabilized by units (Theorem 4.2), and, if the paper cites M. Monks 2009, that the quotient of $T$ by $\Omega$ is again conjugate to $T$.
4. The sentence in `papers/collatz-rings.md` line 440 ("Monks and Yazinski find an autoconjugacy of $3x+1$ ... Neither changes the multiplier") is correct but understates the connection; the quotient section should carry the credit above. The "not found" claims in `quotient.md` Section 10 (the phrase $\mathbb{N} = \mathbb{Z}/\{\pm 1\}$, the descent failure of $3x+1$, Theorem 5.1) are unaffected.

## 6. Corrections needed elsewhere (not made here)

1. `fundamental-domain.md` Section 1 item 8: "Monks's survey slides have a section on $\mathbb{Z}_2/\!\sim$ with $x \sim -x$" should read "with $x \sim \Omega(x)$, $\Omega$ the Monks and Yazinski autoconjugacy".
2. `fundamental-domain.md` Section 10 item 3: the relation is $x = y$ or $x = \Omega(y)$, the induced map is $\Psi([x]) = [T(x)]$ with auxiliary $\xi$, the source is Monks and Yazinski 2004, and it is prior art for the restatement on a quotient by a symmetry, not for $\mathbb{Z}/\{\pm 1\}$. The "Not reached" paragraph can drop the parity neutral source.
3. `literature.md` line 56 and `rotation-prior-art.md` 3.4 can cite the survey deck by its pages (Kucinski table page 16, Fraboni page 8).
