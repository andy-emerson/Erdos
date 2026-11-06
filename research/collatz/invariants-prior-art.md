# Prior art for the general principles of `papers/collatz-rings.md`

Search done 5 November 2026. It follows `literature.md` (Gaussian search, 4 November) and `symmetric-literature.md` (symmetric integer maps, 5 November), and does not repeat what they established unless a row needs it. The rows are the eleven principles listed in the request, numbered as there. Paper references (Proposition 2.4, Lemma 3.3 and so on) are to the draft `papers/collatz-rings.md` of 5 November.

## 1. Method and access

All fetching went through WebSearch and WebFetch. The shell proxy refused every outside host tried (numbertheory.org, arxiv.org, matwbn.icm.edu.pl, oeis.org: HTTP 403 on CONNECT), so nothing was downloaded. WebFetch refused several URLs as not on its allowlist, and it cuts PDF text at about 86,000 characters. A WebFetch result is a summary by a small model, with quotations limited to short fragments, so formulas below are quoted as that summary rendered them. Each was checked for plausibility against the definitions, but not against a scan of the page.

Read in this search:

- Matthews's chapter in *The Ultimate Challenge* (http://www.numbertheory.org/PDFS/matthews-final-revised.pdf), searched twice: Sections 3, 7, 9, 10, 11 and the bibliography. Page numbers come from the running heads of the extracted text and may be off by one.
- Lagarias, annotated bibliography I, version 12 (https://arxiv.org/pdf/math/0309224v12), entries 17 to 83. The current version (https://arxiv.org/pdf/math/0309224) stops at entry 76 in extraction, as before.
- Lagarias, annotated bibliography II (https://arxiv.org/pdf/math/0608208), searched by keyword.
- Lagarias, 1985 *Monthly* survey, HTML node 13 and its annotation page (https://www.cecm.sfu.ca/organics/papers/lagarias/paper/html/node13.html, https://www.cecm.sfu.ca/organics/papers/lagarias/paper/html/anhtml/node13-an1.shtml). The formulas there are images, which the tool could not read.
- Siegel, arXiv:2111.07882 (withdrawn), arXiv:2412.02902 (dissertation), arXiv:2601.17030v2 (HTML).
- Siegel and O'Dwyer, arXiv:2609.33018v1, in HTML (https://arxiv.org/html/2609.33018v1). The abstract page and the unversioned HTML were refused. The extraction again lacked the text of hypothesis MH1.
- Kohl's RCWA: manual chapter 1 and the index (https://docs.gap-system.org/pkg/rcwa/doc/chap1.html, https://docs.gap-system.org/pkg/rcwa/doc/chapInd.html), and the source file `rcwamap.gi` of RCWA 4.6.4 (https://grok.dragonflybsd.org/raw/dports/math/gap/gap-4.11.0/pkg/rcwa-4.6.4/lib/rcwamap.gi). Manual chapter 2, where the definitions are, was refused.
- Chamberland, An update on the 3x+1 problem (https://chamberland.math.grinnell.edu/papers/3x_survey_eng.pdf).
- Nichols, arXiv:1610.02545, through ar5iv (https://ar5iv.arxiv.org/html/1610.02545).
- B. Brent, arXiv:math/0204170; F. Wegner, arXiv:2101.08060; Barina and Maat, arXiv:2602.05732 (abstract); Lesieutre, MIT RSI 2004 report (https://web.mit.edu/rsi/www/pdfs/papers/2004/2004-lesjohn.pdf); an undated blog post "the balanced collatz problem" (https://frank-9976.github.io/first_post.html); the Bell Labs abstract page of Lagarias 1990 (https://www.nokia.com/bell-labs/publications-and-media/publications/the-set-of-cycles-for-the-3x-1-and-3x-k-problems/).
- Metadata pages only: Leigh 1986 (https://geodesic.mathdoc.fr/articles/10.4064/aa-46-2-125-143), Matthews and Watts 1984 (https://eudml.org/doc/205897), Lagarias 1990 (https://eudml.org/doc/206298), Heppner 1978 (https://portal.mardi4nfdi.de/wiki/Publication:1246565), Belaga 2003 (impan.pl).

Not reached, with the reason:

- The Acta Arithmetica PDFs of Leigh 1986, Matthews and Watts 1984 and 1985, Möller 1978 and Lagarias 1990. Both the impan.pl download links and matwbn.icm.edu.pl were refused (allowlist or proxy). So Leigh's paper, Möller's paper and Lagarias 1990 are known here only through annotations and secondary accounts.
- Heppner 1978 itself (Springer, DOI 10.1007/BF01226454). Known through Lagarias's annotation.
- Hasse's 1975 notes. Known through Lagarias's annotation.
- Belaga and Mignotte, Exp. Math. 7 (1998) (Project Euclid, bot protection) and their HAL reports (hal-00129656, hal-00129727, bot protection). Known through Lagarias's annotations.
- Matthews's slides on Leigh's 1983 map, and his web pages (Lu Pei, Barina), as before.
- Wirsching's book, Venturini 1992, Simons and de Weger 2005, Steiner 1977, Eliahou 1993. None was fetched in this search. What is said of them below comes from the annotations and surveys listed above or from memory, and is marked.
- zbMATH reviews (refused).

A negative verdict below means only that the sources above, read as described, do not contain the statement.

## 2. Findings that bear on several rows

### 2.1 Hasse and Heppner

Lagarias's bibliography I, v12, entry 76, gives Hasse's class from the 1975 notes (pp. 23 to 33): for $m > d \ge 2$, $T(x) = (mx + f(r))/d$ for $x \equiv r \pmod d$, with $f(r) \equiv -mr \pmod d$. Hasse gives a probabilistic argument that all orbits are eventually periodic when $m = d + 1$. Entry 78 gives Heppner's class, $T(n) = (mn + r_j)/d$ for $n \equiv j$, $1 \le j \le d-1$, and $n/d$ for $n \equiv 0$, with $\gcd(m, d) = 1$ and $r_j \equiv -mj \pmod d$. It says that the behaviour depends on whether $m$ is below or above $d^{d/(d-1)}$: below, almost all iterates decrease, and above, almost all increase. Heppner's paper is E. Heppner, Eine Bemerkung zum Hasse-Syracuse-Algorithmus, *Archiv der Mathematik* 31 (1978), 317 to 320 (pages from Lagarias 1985, bibliography entry 41, https://www.cecm.sfu.ca/organics/papers/lagarias/paper/html/node16.html; DOI 10.1007/BF01226454).

Hasse's map is the draft's map at $k = 1$ on $\mathbb{Z}$, with $d$ in place of $\pi$ and the digit fixed by the same congruence. Heppner's threshold $d^{d/(d-1)} = d^{1 + 1/(d-1)}$ is the upper end of the draft's window at $k = 1$ with $q = d$. For prime $d$ the residue field has size $d$, so this is exactly the draft's Definition 2.6 at $k = 1$ on $\mathbb{Z}$ with $\pi = d$. Möller characterized the maps of this class that have finite stopping time for almost all integers (Lagarias 1985, Section 3.2, node 13). The characterizing condition is an image that was not read [unverified: presumably the same threshold].

### 2.2 Leigh's 1983 map

Siegel (arXiv:2111.07882, Section 1) writes that "the first such generalization was given by Leigh in 1983", and in Section 3.0.1 gives the map: $z/\sqrt 2$ on the class $0$ and $(3z+1)/\sqrt 2$ on the class $1$ modulo $\sqrt 2$ in $\mathbb{Z}[\sqrt 2]$. This is Matthews's Example 10.1 (Matthews 2010, p. 23). Siegel's dissertation (arXiv:2412.02902, Section 1.1.2) says instead that such maps "were first considered by Leigh (1981)". So the ring-of-integers work attributed to Leigh is this example, as far as the sources show. Leigh's Acta Arithmetica paper (46 (1986), 125 to 143; Lagarias and Matthews date it 1985) is the Markov-chain paper, on $\mathbb{Z}$, and was not read.

### 2.3 Matthews, Section 10

Matthews 2010, Section 10 (pp. 22 to 24), opens with "Let d be a non-unit in the ring O_K of integers of an algebraic number field K" and "Again, the conjectural picture for trajectories is not entirely clear." It gives no general definition, no norm condition and no conjecture for $\mathcal{O}_K$, only Examples 10.1 to 10.3. It does not mention units (except "non-unit"), symmetry under units, Galois conjugates or least absolute residues. Heppner is not cited anywhere in the chapter. Section 11 says that "The conjectural situation for rings other than Z also invites systematic investigation."

### 2.4 Siegel's Hydra maps, Siegel and O'Dwyer, Kohl

None of Siegel's three papers states a norm window, a condition on units, or anything about counting cycles. arXiv:2601.17030v2, Sections 5 and 6, defines $\Lambda$-Hydra maps on $\mathcal{O}_K$ of a global field, with branches $z \mapsto r_j z + c_j$ on the cosets of $\Lambda$ and $r_j$ invertible $\mathbb{Q}$-linear maps. Its only number-field example is a remark on $\mathbb{Q}(\sqrt 2)$ (Remark 5.1). Lemma 6.1 uses $\rho_{H,\ell} = \prod_j \|r_j\|_\ell$ at a place $\ell$.

Siegel and O'Dwyer (arXiv:2609.33018v1) define $\rho_H = \prod_{i=0}^{p-1} \|r_i\|_{Mink}$ (their (1.8)), the product of Minkowski operator norms. Their Section 3 gives the heuristic that $\rho_H < 1$ gives $\|H^{\circ n}(\mathcal N)\|_{Mink} \ll \rho_H^{n/p}\|\mathcal N\|_{Mink}$ (their (3.6)). Their theorem needs the stronger $(p-1)^2\rho_H^d < 1$ (MH3), with $d = [K:\mathbb{Q}]$. There is no example on $\mathbb{Z}[i]$ or $\mathbb{Z}[\omega]$, and nothing on units or cycles in the text extracted.

Kohl's RCWA supports exactly $\mathbb{Z}$, $\mathbb{Z}^2$, the semilocalizations $\mathbb{Z}_{(\pi)}$ and $\mathrm{GF}(q)[x]$. Any other ring raises the error "rcwa mappings over ... are not yet implemented" (`rcwamap.gi`, file header). The index lists `IsBalanced`, `IsSignPreserving`, `Multiplier`, `Divisor`, `GuessedDivergence` (2.8-12) and `Mirrored` (3.1-9). `Mirrored` conjugates by $n \mapsto -n-1$, "the automorphism switching action on negative and nonnegative integers". The index has no entry for units. The definition behind `GuessedDivergence` is in manual chapter 2, which was refused.

## 3. The rows

### 3.1 Row 1: the window $q^k < N(\alpha) < q^{k + 1/(q-1)}$

Verdict: known in a special case, and equivalent to a known heuristic in general.

- At $k = 1$ on $\mathbb{Z}$ the upper end is Heppner's threshold $m < d^{d/(d-1)}$ for Hasse's maps, with $q = d$ (Section 2.1; Lagarias bibliography I v12, entry 78; Heppner, *Arch. Math.* 31 (1978), 317 to 320). Hasse's case $m = d + 1$ lies inside it. Crandall's conjecture that $qx + 1$ has divergent orbits for $q \ge 5$ (bibliography I v12, entry 46; Crandall, *Math. Comp.* 32 (1978), 1281 to 1292) is the same window at $q = 2$, $k = 1$: $2 < 3 < 4 < 5$.
- For any $q$ and $k$ the upper end is the Matthews and Watts growth-rate heuristic (Matthews 2010, Section 3.2, pp. 5 to 6), applied after writing the accelerated map at modulus $\pi^k$. My computation, not in any source: put $z \mapsto z/\pi = \pi^{k-1}z/\pi^k$ on the $q^{k-1}$ even classes and $z \mapsto (\alpha z + d)/\pi^k$ on the $q^{k-1}(q-1)$ odd classes. Then the product of the norms of the multipliers is below $N(\pi^k)^{q^k}$ exactly when $N(\alpha)^{q-1}q^{k-1} < q^{kq}$, that is, $N(\alpha) < q^{k+1/(q-1)}$. On $\mathbb{Z}$ with $q = 2$ this is Conjecture 3.1's product $|m_0 \cdots m_{d-1}| < d^d$ with $d = 2^k$, and Barina states the case $m = 7$, $k = 2$ in this form. The encoded map is not of relatively prime type (the even classes have $m_i = 2^{k-1}$), so Conjecture 3.1 does not cover it, and the criterion that applies is Leigh's Markov-chain Conjecture 7.1 (Matthews 2010, Section 7, pp. 14 to 17). That conjecture weights $|m_i/d|$ by the chain's limiting probabilities, for which Matthews gives no closed form. For these maps the weights reduce to Proposition 2.5 of the draft [my reading, not checked against Leigh's paper].
- Siegel and O'Dwyer's heuristic $\rho_H < 1$ (arXiv:2609.33018v1, Section 3, (3.6)) gives the same upper end. My computation, assuming the Minkowski operator norm of multiplication by $\alpha$ in an imaginary quadratic field is $|\alpha|$: at $k = 1$, $\rho_H = |\pi|^{-1}(|\alpha|/|\pi|)^{q-1}$, so $\rho_H < 1$ exactly when $N(\alpha) < q^{q/(q-1)}$; at general $k$ the same computation with $p = q^k$ branches gives $q^{k+1/(q-1)}$.
- Nichols (arXiv:1610.02545, Theorems 2.2 and 3.1) proves a Terras-type theorem for $mx+1$ maps on $\mathbb{F}_2[t]$ and on a hyperelliptic ring $R_r$, with thresholds $\deg m \le 2$ and $\deg m \le 4$. These are the function-field form of the same averaging. I did not check whether they coincide with the window under the right normalization of the degree.
- The lower end (Proposition 2.4: below the window every orbit is eventually periodic) is routine and is the setting of the $x+1$ problem; I found no statement for $\mathcal{O}_K$ but did not look for one beyond the sources above.

Not found: the statement for $\mathcal{O}_K$ with the residue-field size $q$ and the exponent $k + 1/(q-1)$, or the remark that the naive window $(4^k, 4^{k+1})$ is wrong for $\mathbb{Z}[\omega]$. Matthews's Section 10 explicitly has no conjecture there (Section 2.3).

Recommendation: cite as a remark. Credit Heppner for $k = 1$ and Matthews and Watts (with Leigh) for the growth-rate heuristic, and say that the general form is that heuristic carried over to the norm. The draft's Section 2.2 already does most of this, but cites Heppner as "[unverified: not seen]". It can now give the page range and Lagarias's annotation, and keep "not seen" for the paper itself.

### 3.2 Row 2: which moduli admit multipliers in the window

Verdict: not found as a principle. Closely related observations exist.

- The Alvarado REU slides (https://fefoaranda.com/wp-content/uploads/2025/04/colima-alvarado-2025.pdf, see `literature.md` 2.3) study $2 + i$ at modulus $1 + i$ and remark that Crandall's heuristic predicts growth. They do not say that no multiplier fits that modulus.
- The blog post "the balanced collatz problem" (https://frank-9976.github.io/first_post.html, undated, by "keith") asks for the expected valuation change $v(\alpha) - 2v(\beta)$ to be $0$ with $R/(\beta) \cong \mathbb{Z}/2$, and notes that this fails over $\mathbb{Z}$, over $\mathbb{Z}[x]/(x^2+1)$ with $\beta = x + 1$ and over $\mathbb{Z}[x]/(x^2 - 2)$ with $\beta = x$. In the draft's terms, the blog notes that no odd $\alpha$ sits at the upper end $N(\alpha) = 4$ of the window at modulus $1 + i$. That is the boundary, not the open window $(2, 4)$, but it is the same parity-of-norms argument. It is not a citable source.
- The draft's Lemma 5.3 table in `theorems.md` (odd sums of two squares in each window) is elementary, and no source lists the windows by norm.

Recommendation: prove (it is a one-line argument, which the draft already gives). Mention the REU's divergence as the observation it explains, as the draft does. Do not cite the blog.

### 3.3 Row 3: at most $|U|$ trivial cycles, and their number divides $|U|$ for commuting maps

Verdict: not found.

Nothing in the sources counts trivial cycles. On $\mathbb{Z}$ the statement is implicit in every list of the cycles of $3x+1$ on $\mathbb{Z}$, which has two trivial cycles (through $1$ and $-1$) (Lagarias 1985, section "Are there non-trivial cycles?", HTML node 8, as recorded in `symmetric-literature.md` 3.4). Barina notes that $7x \pm 1$ has exactly the cycles through $1$ and $-1$ (arXiv:1807.00908, Section 2). Kucinski's symmetry theorem for Joseph's map (Theorem 3, see `literature.md` 2.2) is about conjugacies of one fixed map, not about orbits of cycles under $U$. The count $|U|/|U \cap C_1|$ is the orbit-stabilizer theorem for the action of $U$ on cycles.

Recommendation: prove, and present it as elementary. The draft's Proposition 3.1 and Theorem 6.2(1) need no credit, but should not claim more than that no source was found.

### 3.4 Row 4: maps commuting with the units exist at $\pi^k$ iff $k > \kappa$; none for infinite unit groups

Verdict: not found in this generality. On $\mathbb{Z}$, odd maps are known, and their known moduli are consistent with the theorem. No classification of odd maps by modulus was found.

- Odd maps on $\mathbb{Z}$ of Collatz type, from `symmetric-literature.md` Section 3: Barina's $7x \pm 1$ at modulus $4$, stated to be mirror-symmetric (arXiv:1807.00908, Section 2); Collatz's original permutation at modulus $3$, stated to be odd by Tremblay (arXiv:1907.13302, Section 2, equation (1)); Conway's amusical permutation at modulus $4$, with $a(n) = -a(-n)$ in OEIS A006368; and Matthews's generalization $L_d$ of Lu Pei's map, which uses least absolute residues $-d/2 < i \le d/2$ (Carnielli, *Applied Mathematics E-Notes* 15 (2015), 207 to 215; Tremblay, arXiv:1907.13302, equation (43)).
- Theorem 6.1 at the prime $p$ of $\mathbb{Z}$ gives $\kappa = v_p(-2)$, which is $1$ for $p = 2$ and $0$ for odd $p$. So odd maps exist at modulus $2^k$ exactly for $k \ge 2$, and at modulus $p^k$ for odd $p$ already at $k = 1$. All the examples fit: the modulus-$2$ maps ($3x \pm 1$) are never odd, the odd maps with a power of $2$ have modulus $4$ (Barina, Conway), and those with odd modulus have modulus $3$ or $d$ (Collatz, Lu Pei, $L_d$ for odd $d$). This consistency is my observation; no source states it.
- No source discusses symmetry under a unit group larger than $\{\pm 1\}$, or rings with infinite unit groups in this connection. Matthews's Example 10.3 on $\mathbb{Z}[\sqrt 2]$ uses the unit factor $1 - \sqrt 2$ in its even branch (Matthews 2010, p. 24), but says nothing about commuting with units. The REU slides show units permuting the digit-$u$ maps at modulus $1+i$ rather than fixing one (`literature.md` 2.3), which is the case $k = 1 \le \kappa = 2$ of the theorem.
- Kohl's `IsSignPreserving` is a different property (preserving the sign of $n$). Kohl's index has no notion of a map commuting with $n \mapsto -n$ [unverified: manual chapter 2 not read].

Recommendation: prove (new as far as found), with a remark that the known odd maps on $\mathbb{Z}$ (Barina, Conway, Collatz's permutation, Matthews's $L_d$ for odd $d$) sit at the moduli the theorem allows. The draft says of Proposition 3.2 that "We did not find this one-line observation stated in any source we read", which remains accurate.

### 3.5 Row 5: forced fixed points (Lemma 3.3, Corollary 3.4)

Verdict: known in a special case on $\mathbb{Z}$; the lemma itself was not found.

- On $\mathbb{Z}$ with $m = 3$ at modulus $2$, $4 - 3 = 1$, and Lemma 3.3 says that $3x + d$ fixes $d$. That is the scaled trivial cycle: the orbit of $d$ under $3x + d$ is $d$ times the orbit of $1$ under $3x + 1$. Lagarias 1990 separates such cycles from the primitive ones, whose members are prime to $k$ (Lagarias, The set of rational cycles for the $3x+1$ problem, *Acta Arith.* 56 (1990), 33 to 53, as summarized at https://www.nokia.com/bell-labs/publications-and-media/publications/the-set-of-cycles-for-the-3x-1-and-3x-k-problems/ and by Chamberland, Section 6.2). Belaga and Mignotte study primitive cycles of $3x + d$ for $1 < d < 20000$ (Lagarias bibliography II, entry 13). Barina and Maat prove that the iterates of $3^k n$ under $3n + 3^k$ are $3^k$ times the Collatz iterates of $n$ (arXiv:2602.05732, abstract; *CMST* 31 (2025), 67 to 70).
- Crandall proves that for $qx + r$ with $r \ge 3$ some orbit never reaches $1$ (bibliography I v12, entry 46). This is the integer content of Corollary 3.4 for $q = 3$: a Collatz map with multiplier $3$ has digit $\pm 1$. Crandall's mechanism (the multiples of $r$ form an invariant set) differs from the draft's (a fixed point).
- No source states the lemma for a general multiplier, modulus and digit table, with the unit $\varepsilon$ and the condition that $\varepsilon\pi^v - \alpha$ be a unit.

Recommendation: prove, with credit to the special case. Say that for $3x + d$ the fixed point $d$ is the scaled trivial cycle of Lagarias's non-primitive cycles, and that Crandall had already shown that $qx + r$ with $r \ge 3$ is not a Collatz map on $\mathbb{N}$.

### 3.6 Row 6: smallest digits are unique and commute with the units except on the Voronoi boundary

Verdict: known on $\mathbb{Z}$ (least absolute residues) and standard as lattice geometry; the Collatz statement on $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$ was not found.

- On $\mathbb{Z}$, Matthews's $L_d$ (via Carnielli 2015 and Tremblay, arXiv:1907.13302, equation (43)) uses least absolute residues $-d/2 < i \le d/2$. For odd $d$ the residue system is symmetric and $L_d$ is odd. For even $d$ the tie at $\pm d/2$ is broken toward $+d/2$, and $L_d$ is not odd. That is the one-dimensional case of the draft's remark that ties on the Voronoi boundary break the symmetry. No source draws this conclusion; Carnielli calls the idea "balancing" (`symmetric-literature.md` 3.3).
- Least-norm remainders modulo a Gaussian or Eisenstein integer, with ties on the boundary of the Voronoi cell, are standard in the Euclidean algorithm for these rings and in lattice geometry. I did not look for a citation, since none is needed for so elementary a fact.
- The counts of tied classes in $\mathbb{Z}[i]$ (the draft's $2^j$ tied classes at $k = 2j$, and $2^{2^{j-2}}$ commuting tables) were not found anywhere.

Recommendation: cite as a remark, with Matthews's $L_d$ as the integer precedent (the only modulus-$2^k$ precedent is Barina's $7x \pm 1$ at $k = 2$). Prove the tie counts as the draft's own.

### 3.7 Row 7: unit cycles of commuting maps (units permuted by multiplication by $S(1)$)

Verdict: not found.

No source discusses unit cycles. On $\mathbb{Z}$ the content is that an odd map has either both fixed points $\pm 1$ or the loop $1 \leftrightarrow -1$, and the draft's examples ($7x \pm 1$ and $-7x \pm 1$) are the only places where this appears.

Recommendation: prove (elementary, Theorem 6.2(2)). The counterexample $2 - i$ at $(1+i)^2$ is the draft's own.

### 3.8 Row 8: rotated and mirrored maps, and the factor $(\bar\pi/\pi)^v$

Verdict: closely related special cases. The theorems (Proposition 6.3, Theorem 6.4) were not found.

- The REU slides prove that $z$ reaches a unit under the map with digit $u_1$ exactly when $u_2 z$ reaches $u_1 u_2$ under the map with digit $u_1 u_2$, in the same number of steps (`literature.md` 2.3). This is conjugation by $z \mapsto uz$ at modulus $1+i$, which changes the digit and keeps the multiplier, the relation the draft uses in Section 4.3 to reduce tables. It is not the rotation $\alpha \mapsto u\alpha$.
- Kucinski's Theorem 3 (`literature.md` 2.2) finds conjugate subsystems of Joseph's map via $h(x + yi) = y + xi$. Since $y + xi = i\bar z$, this is a mirror followed by a rotation. Joseph's map has the real multiplier $3$ and divides by $2$, which commutes with conjugation, so no factor $(\bar\pi/\pi)^v$ arises there. The factor arises only when the division is by a non-real prime such as $1+i$, as in the draft.
- On $\mathbb{Z}$, Kohl's `Mirrored` conjugates an rcwa map by $n \mapsto -n-1$ (RCWA 4.6.4, `rcwamap.gi`), and Monks and Yazinski's autoconjugacy uses $V(x) = -1 - x$ (Lagarias bibliography II, entry 77). Both are conjugacies of a single map, not rotations of the multiplier. The pair $3x + 1$, $3x - 1$ is the standard example of a map and its conjugate by $-1$ (OEIS A037084; Lagarias 1985).
- Galois conjugation of the multiplier was not found in any source, including Matthews's real quadratic examples.

Recommendation: prove (new as far as found), citing Kucinski's Theorem 3 and the REU proposition as the special cases closest to it.

### 3.9 Row 9: cycles at best approximations, and counts set by $|\alpha^n - \varepsilon\pi^V|$

Verdict: known on $\mathbb{Z}$ for $3x+1$ and $3x+k$. The Gaussian form (closeness in angle as well as modulus) and the heuristic count $E(n, V)$ were not found.

- Cycle equation. For $3x+1$ the rational point of a cycle with given parity vector is $x = \sum 3^{m-k-1}2^{v_k}/(2^n - 3^m)$ (Böhm and Sontacchi 1978, in bibliography I v12, entry 26). Lagarias 1990 restates this as the "BSL theorem": every parity vector of length $n$ gives a unique rational periodic point of period $n$ (as recalled by B. Brent, arXiv:math/0204170, Section 2, which also gives Lagarias's count of irreducible cycles, $I(n) = \frac1n\sum_{d \mid n}\mu(d)2^{n/d}$, Lagarias 1990, equation (2.5), p. 38). The draft's "each residue class holds one solution of $S^n(x) = \varepsilon x$" is this theorem carried over.
- Best approximations of $\log_2 3$. These appear in Steiner's theorem on one-circuit cycles (Proc. 7th Manitoba Conf. (1978), 553 to 559, bibliography I v12, entry 64), in Eliahou's lower bound, where the cycle length is $301994a + 17087915b + 85137581c$ (Chamberland, Section 5; Eliahou, *Discrete Math.* 118 (1993)), and in Crandall 1978 (bibliography I v12, entry 46). Simons and de Weger extend Steiner's method to $m$-cycles (*Acta Arith.* 117 (2005), 51 to 70 [unverified: not reached; from memory and from bibliography II entry 74, which cites them]).
- Counts. Lagarias 1990 proves integer cycles of $3x + k$ for infinitely many $k$, with estimates on their number and length, and conjectures that integer cycles exist for every $k$ (Chamberland, Section 6.2). The Bell Labs abstract adds that the number of primitive cycles of a given period is zero infinitely often for periods near $k^{1/3}$. Belaga and Mignotte bound the number of periodic orbits with at most $k$ odd elements by $d^{k^c}$ (bibliography I v12, entry 18; Baker and Wüstholz), and Belaga gives a polynomial bound in $d$ for a fixed odd length (*Acta Arith.* 106 (2003), 197 to 206; bibliography II, entry 11). Belaga and Mignotte tabulate factorizations of $2^l - 3^k$ for $l \le 114$ for this purpose (bibliography II, entry 14). Wegner (arXiv:2101.08060) writes each cycle of $3x + k$ through $D = 2^a - 3^b$, which is the draft's mechanism: when $d = 2^V - 3^n$, every rational cycle with $n$ odd steps and $V$ divisions becomes an integer cycle of $3x + d$.
- Gaussian integers. Kucinski gives a Steiner-type bound for Joseph's map in terms of the class counts of the parity vector, with bounds on real parts (`literature.md` 2.2). I found no condition on the angle of $\alpha^n$.
- The heuristic count $E(n, V)$, with its factor $1/|2^V - m^n|$, was not found. Lagarias's cycle counts are exact counts of rational cycles, and Belaga and Mignotte's are upper bounds; neither gives the expected number of integer cycles as a function of $|2^V - m^n|$ [unverified: Lagarias 1990 itself not read; it may contain such a heuristic].

Recommendation: cite on $\mathbb{Z}$ (Böhm and Sontacchi, Steiner, Eliahou, Simons and de Weger, Lagarias 1990, Belaga and Mignotte, Wegner). Present the Gaussian angle condition and $E(n, V)$ as the draft's own, the count clearly as a heuristic, with the caveat that Lagarias 1990 was not read.

### 3.10 Row 10: divergence along invariant lines for multipliers that act like rationals

Verdict: closely related precedents. The draft's lines and drift computation were not found.

- Leigh's map (Matthews 2010, Example 10.1, p. 23) on $\mathbb{Z}[\sqrt 2]$ with the rational multiplier $3$ has "at least three one-dimensional T-invariant sets", among them $S_1$ with $T^2(x, 0) = (C(x), 0)$, and divergent trajectories "evenly distributed mod $(\sqrt 2)^a$". Example 10.3 has divergent trajectories that need not tend to infinity in absolute value.
- Kucinski's Theorem 3 finds invariant subsystems of Joseph's map, which has the rational multiplier $3$, on the real axis, the imaginary axis and the line $y = x$ (`literature.md` 2.2).
- Kionke's tame cones give divergent points for lattice maps with scalar (rational) multipliers (arXiv:1511.05893; `literature.md` 2.5). This is the mechanism by which a multiplier that acts as a rational number on a sublattice can diverge.
- The lines $x = \pm 1$, $y = \pm 1$, $x \pm y = \pm 1$ in $\mathbb{Z}[i]$ (and $b = 1$, $a - b = 1$ in $\mathbb{Z}[\omega]$), and the two-state drift along them, were not found.

Recommendation: cite as a remark (Matthews Example 10.1, Kucinski, Kionke), as the draft's Section 8 already does in part. The specific lines and drift values are the draft's own.

### 3.11 Row 11: fixed points of the minimal odd map $mx \pm 1$ at modulus $2^k$

Verdict: not found.

Barina's $7x \pm 1$ ($\delta = 1$, so $1$ is the only positive fixed point) is consistent with Proposition 3.6 but states nothing general (arXiv:1807.00908). No source treats the family of odd maps $mx + d$ with $d$ the least absolute residue at modulus $2^k$ for $k \ge 3$ (`symmetric-literature.md` Sections 4 and 5, and the searches for "9x±1", "13x±1", "17x±1" in this search, which found only Barina). The nearest family results are Lesieutre's for a different generalization (primorial modulus; MIT RSI 2004 report, Theorem 9, finitely many cycles for each multiplier) and Crandall's for $qx + r$ (entry 46).

Recommendation: prove (new as far as found). Mention that it explains why Barina's $7x \pm 1$ and the draft's $5x \pm 1$ have no second fixed point.

## 4. Other items checked

- Venturini (*Stud. Appl. Math.* 86 (1992), 185 to 218), Wirsching's book and Hasse's notes were not seen; Lagarias's annotations and Matthews's chapter give no sign that they treat rings other than $\mathbb{Z}$, units or symmetry.
- Bernstein and Lagarias's conjugacy map (bibliography I v12, entries 22 and 23) concerns the 2-adic shift and the $3x+1$ function, and bears on none of the rows.
- Lagarias bibliography II has no entry on Joseph, Kucinski, Siegel or Tremblay, and none on maps over $\mathbb{Z}[i]$ or on odd maps (keyword search). Its only number-field entry is Matthews's (entry 70), with the map on $\mathbb{Z}[\sqrt 2]$.
- Searches for Collatz maps on rings of integers, imaginary quadratic fields and Eisenstein integers in 2025 and 2026 turned up only Siegel and O'Dwyer and the REU slides, already covered. arXiv:2610.34556 (Shiu) is on admissible sets in imaginary quadratic fields, not Collatz maps.
- Hamada (SSRN 5378103, 2025) and Lesieutre (2004) study other generalizations on the positive integers and touch none of the rows.

## 5. Corrections and additions for the draft

1. Section 2.2: Heppner can be cited as *Archiv der Mathematik* 31 (1978), 317 to 320, with the threshold as annotated in Lagarias's bibliography I (entry 78 in v12). Hasse's map and his case $m = d+1$ are in entry 76. The general upper end can be stated as the Matthews and Watts growth-rate heuristic for the map written at modulus $\pi^k$, and Siegel and O'Dwyer's heuristic $\rho_H < 1$ gives the same bound.
2. Section 1.1: "Leigh is reported to have been the first to consider such maps on rings of algebraic integers [unverified]" can say that the map in question is Leigh's $(3z+1)/\sqrt 2$ on $\mathbb{Z}[\sqrt 2]$, Matthews's Example 10.1, dated 1983 by Siegel (arXiv:2111.07882) and 1981 in Siegel's dissertation (arXiv:2412.02902, Section 1.1.2).
3. Section 3.3, Lemma 3.3: add that for $3x + d$ the fixed point $d$ is the scaled trivial cycle (Lagarias 1990, non-primitive cycles; Barina and Maat 2025), and that Crandall proved that $qx + r$ with $r \ge 3$ has an orbit that does not reach $1$.
4. Section 3.1: cite Böhm and Sontacchi, Lagarias 1990 and Wegner for the cycle equation and the role of $2^V - 3^n$; present $E(n, V)$ as a heuristic, with no prior form found.
5. Section 6.1: add the remark that the known odd maps on $\mathbb{Z}$ sit at the moduli Theorem 6.1 allows, and that Matthews's $L_d$ for even $d$ shows the tie-breaking failure in one dimension.
6. Section 6.2: cite Kucinski's Theorem 3 ($h(z) = i\bar z$ for Joseph's map) as the nearest precedent for mirrored maps, noting that division by $2$ produces no factor $(-i)^v$.

## 6. Summary of verdicts

1. Window $q^k < N(\alpha) < q^{k+1/(q-1)}$: known in a special case (Heppner, $k = 1$ on $\mathbb{Z}$) and equivalent to the Matthews and Watts growth heuristic (and Siegel and O'Dwyer's $\rho_H < 1$) for the map written at modulus $\pi^k$; the norm form for $\mathcal{O}_K$ not found. Cite as a remark.
2. Moduli admitting multipliers depend on represented norms: not found (REU observation and an uncitable blog are related). Prove.
3. At most $|U|$ trivial cycles, a divisor of $|U|$ for commuting maps: not found. Prove (elementary).
4. Commuting maps exist iff $k > \kappa$; none for infinite unit groups: not found; known odd maps on $\mathbb{Z}$ (Barina, Conway, Collatz, Matthews's $L_d$) consistent with it. Prove, with a remark on the integer examples.
5. Forced fixed points: known in a special case ($3x + d$ fixes $d$, Lagarias 1990, Barina and Maat; Crandall's $qx + r$, $r \ge 3$). Prove with credit to the special case.
6. Smallest digits unique and commuting off the Voronoi boundary: known on $\mathbb{Z}$ (Matthews's least-absolute-residue $L_d$), standard as geometry. Cite as a remark; the tie counts are the draft's own.
7. Unit cycles permuted by $S(1)$: not found. Prove (elementary).
8. Rotated and mirrored maps: closely related (REU digit conjugacy, Kucinski's Theorem 3, Kohl's `Mirrored`); not found as stated. Prove, citing those.
9. Cycles at best approximations, counts by $|\alpha^n - \varepsilon\pi^V|$: known on $\mathbb{Z}$ (Böhm and Sontacchi, Steiner, Eliahou, Simons and de Weger, Lagarias 1990, Belaga and Mignotte, Wegner); Gaussian angle condition and $E(n, V)$ not found. Cite for $\mathbb{Z}$; the rest the draft's own, $E$ as heuristic.
10. Divergence on invariant lines: closely related (Matthews Example 10.1, Kucinski, Kionke). Cite as a remark.
11. Integer fixed-point theorem for $mx \pm 1$ at $2^k$: not found. Prove.
