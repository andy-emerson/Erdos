# The (1+i)-adic view of the Gaussian gcd and the quartic residue symbol (Nov 4, 2026)

Question: does working (1+i)-adically, with Hensel quotients instead of single subtractions, give a cleaner or faster treatment of the gcd and the quartic (biquadratic) residue symbol in $\mathbb{Z}[i]$ than Weilert's algorithms? This note covers the mathematics only, independent of the machine representation. The work was reading plus Python checks. The scripts are in `research/weilert-adic/` (`g.py` and `alg.py` hold the algorithms, `t1.py` to `t8.py` the checks). Nothing was timed. Notation: $\pi = 1+i$, $v$ is the $\pi$-adic valuation, and $\ell(n)$ is the bit length of $n$. An odd $z$ (one prime to $\pi$) is primary when $z \equiv 1 \pmod{\pi^3}$, which for $z = c+di$ means $d$ even and $c+d \equiv 1 \pmod 4$.

## 1. Sources and what could be read

1.1 Weilert's three papers are still unreadable here. ScienceDirect, doi.org, Crossref, Unpaywall, CORE and Semantic Scholar were all refused by the proxy (403), and Bohrium serves only a firewall page. What is known about them comes from secondary sources that were read in this pass:

- Weilert, "(1+i)-ary GCD computation in Z[i] as an analogue to the binary GCD algorithm", J. Symbolic Comput. 30(5) (2000) 605–617. Its abstract (study 10) gives the binary-style extraction of powers of $\pi$, $O(n^2)$ bit operations, and a constant factor better than a Lehmer-type Gaussian gcd. Wikström's KTH report ([Wik04TR](https://www.csc.kth.se/~dog/research/papers/Wik04TR.pdf), §1.1) says this paper generalises both the binary gcd and the binary Jacobi-symbol algorithm to $\mathbb{Z}[i]$. So the JSC paper also contains an $O(n^2)$ quartic symbol of the subtract-and-strip kind.
- Weilert, "Asymptotically fast GCD computation in Z[i]", ANTS-IV, LNCS 1838 (2000) 595–613. This is a Schönhage-type half-gcd. Wikström also credits Weilert with a Gaussian form of Sorenson's $k$-ary reduction, citing this paper.
- Weilert, "Fast computation of the biquadratic residue symbol", J. Number Theory 96 (2002) 133–151. It gives an $n^{1+o(1)}$ quartic symbol built on the fast Euclidean (top-down) gcd ([Bach and Sandlund](https://arxiv.org/abs/1807.07719); Belabas on [pari-users](https://pari.math.u-bordeaux.fr/archives/pari-users-0612/msg00002.html) calls it "a rather painful exercise"). Brent and Zimmermann ([arXiv:1004.2091](https://arxiv.org/abs/1004.2091), §5) quote it: "We are not able to use a GCD calculation in Z[i] similar to the binary GCD algorithm … because we do not get a corresponding quotient sequence in an obvious manner." They say they "filled that gap" for the Jacobi symbol in $\mathbb{Z}$, and they pose no problem about $\mathbb{Z}[i]$.

1.2 Other sources:

- Damgård and Frandsen, [BRICS RS-03-8](https://brics.dk/RS/03/8/BRICS-RS-03-8.pdf), read. Their cubic algorithm keeps both values primary and replaces the approximately larger by the primary part of $\alpha-\beta$, folding the unit and $(1-\zeta)$ exponents into a running exponent $t$. Their §5 transfers this to $\mathbb{Z}[i]$: the primary form $1+(2+2i)\beta$, $N(\pi^3) = 8$, reciprocity with sign $(-1)^{\frac{N\alpha-1}{4}\frac{N\beta-1}{4}}$, and supplements for $1+i$ and $i$. They do not cite Weilert. Their step bound is "the product of the norms at least halves".
- Stehlé and Zimmermann, [recursive binary gcd](https://perso.ens-lyon.fr/damien.stehle/downloads/recbinary.pdf), read. In their binary division, for $v_2(a) < v_2(b)$, $r = a + q\,b/2^{j}$ with $j = v_2(b) - v_2(a)$, $q$ odd and $|q| < 2^{j}$, so that $v_2(r) > v_2(b)$. The sequence length is at most about $n/\log_2\frac{\sqrt{17}-1}{2} \approx 1.555\,n$, proved by an entrywise order on the quotient matrices. The paper says nothing about other rings.
- Bach and Sandlund, read. They state the quartic laws used below and show that a Gaussian Euclid with exact rounded quotients costs $\Omega(n M(n))$ in the worst case, while approximate quotients bring it to $O(n^2)$.
- FLINT's `fmpzi_gcd_binary` is documented as "Weilert's (1+i)-ary algorithm". Per a summary of its source (not read verbatim), each step tries $x \pm y$ and $x \pm iy$, keeps the smallest norm, and falls back to Euclid above 500 bits.
- Agarwal and Frandsen (ANTS 2004, LATIN 2006): only the metadata was reachable, so their text is still unread.
- Brent and Zimmermann, MCA §1.4.8: not reread; the arXiv paper above covers the relevant material.

1.3 What Weilert's algorithms do, as far as can be established. His $O(n^2)$ algorithm is the subtract-and-strip loop: both values odd and normalised, the larger replaced by the stripped and normalised difference, with symbol bookkeeping by the supplements and reciprocity. This is the algorithm of studies 9 and 11 and of W below. Its proven bound is the Damgård–Frandsen one, at most about $\log_2 N(ab)$ steps. Two points remain unconfirmed: whether he subtracts with a unit chosen mod $\pi^3$ (as in study 9) or picks the best of four (as FLINT does), and his exact step constant. Did he use Hensel quotients? Every source points to no. The 2002 remark above says he had no low-bit quotient sequence, and his fast algorithm is top-down. His $k$-ary variant (Sorenson's two-sided $u a + v b \equiv 0 \bmod k$) is the nearest relative, but it can add spurious factors that need a cleanup pass, and the one-sided step below cannot.

## 2. The Hensel-quotient step for the gcd (study 33), stated and bounded

2.1 The step. Let $x, y$ be odd with $N(x) \ge N(y)$, let $L = \ell(N(x)) - \ell(N(y))$, and let $j = \lfloor (L+c)/2 \rfloor$ for a fixed even offset $c \ge 6$. Let $q \equiv x y^{-1} \pmod{2^j}$, with both coordinates centred in $[-2^{j-1}, 2^{j-1})$. Write
$$x - qy = i^{s}\,\pi^{t}\,x', \qquad x' \text{ primary},$$
and replace $x$ by $x'$. In practice $q = (x\bar y)\cdot N(y)^{-1} \bmod 2^j$, since $N(y)$ is odd.

2.2 Correctness. $\gcd(x', y) = \gcd(x - qy, y) = \gcd(x, y)$ up to units, because $\gcd(\pi, y) = 1$. No spurious factor appears, unlike Sorenson's two-sided reduction. Also $2^j \mid x - qy$, so $t \ge 2j$.

2.3 Size. $|q| \le 2^{j}/\sqrt2$, so
$$|x'| \le |x|\,2^{-j} + |y|/\sqrt2 .$$
From $N(x)/N(y) < 2^{L+1}$ and $2^j \ge 2^{(L+c-1)/2}$ we get $|x|2^{-j} < 2^{1-c/2}|y|$. For $c = 6$ this gives $|x'| < 0.957\,|y|$, so $x'$ is always the new smaller element. Let $x_k$ be the size of the smaller element after step $k$ (a strictly decreasing sequence), and write $j_{\min} = c/2$. Then
$$x_{k+1} \le \min\!\left(2^{-j_{\min}} x_{k-1},\; 2^{1-c/2} x_k\right) + x_k/\sqrt2 .$$

2.4 Bound. The right-hand side is monotone in both arguments, so iterating it from $x_0 = x_1 = 2^{31.5}$ until it falls below $\sqrt5$ bounds the number of steps for 31-bit coordinates. (A primary element of norm below 5 is 1.) The results are in the table below. Study 33's argument used the one-term factor 0.916 on the product of the norms, which gave about 960. Two changes give the improvement: use the smaller element as the potential, and keep the two-term recurrence.

The asymptotic rate is the root of $\rho^2 = \rho/\sqrt2 + 2^{-c/2}$, giving 4.4, 2.9 and 2.4 steps per bit of coordinate at $c = 6, 8, 10$. As $c \to \infty$ it tends to 2, which is the same 1 bit of norm per step as the naive Euclid bound. Pushing the worst case below this would need an argument that consecutive centred quotients cannot all sit at the corner of their box. That was not attempted.

| offset $c$ | proven steps, 31-bit | observed mean / max (3,000 random 31-bit pairs) |
|---|---|---|
| 6 (study 33) | 133 | 15.17 / 23 |
| 8 | 88 | 14.90 / 22 |
| 10 | 73 | 14.88 / 22 |
| 12 | 67 | 14.80 / 22 |
| for comparison: the $\varepsilon$ step (W) | about 126 ($\log_2 N(xy)$) | 29.43 / 39 |
| for comparison: Euclid, nearest rounding (E) | 63 (naive) | 14.71 / 22 |

The worst cases seen stay 3 to 6 times below the proven bounds, as with every bound in studies 9 to 14.

## 3. A valuation-driven step (a Gaussian Stehlé–Zimmermann division)

3.1 Why this step. Study 33's step reads $L$ from the top bits, so its quotient sequence is not a function of low bits alone, and it cannot drive a binary recursive (half-gcd) algorithm. The Gaussian form of Stehlé–Zimmermann's division uses valuations only. State: odd $A, B$ and a gap $g \ge 1$, standing for the pair $(A, \pi^g B)$; start with $g = 1$. For a fixed $\delta \ge 1$, take $q \equiv A B^{-1} \pmod{\pi^{g+\delta}}$, choosing the minimal representative (nearest rounding in the square lattice $\pi^{g+\delta}\mathbb{Z}[i]$). Write $A - qB = i^s \pi^t C$ with $C$ primary. Then $t \ge g+\delta$, and the new state is $(A, B, g) \leftarrow (B, C, t-g)$. Correctness is as in 2.2. $\delta = 1$ is the exact analogue of Stehlé–Zimmermann.

3.2 Size. In unnormalised terms $r_{k+1} = r_{k-1} - q_k r_k/\pi^{g_k}$ and $v(r_{k+1}) \ge v(r_k) + \delta$. Let $\rho(m)$ be the largest minimal representative of an odd class mod $\pi^m$ (computed exactly for $m \le 11$, and bounded by the covering radius $2^{(m-1)/2}$ beyond). The normalised sizes $x_k = |r_k|/2^{v(r_k)/2}$ then satisfy
$$x_{k+1} \le 2^{-g_{k+1}/2}\left(2^{-g_k/2}x_{k-1} + \frac{\rho(g_k+\delta)}{2^{g_k/2}}\,x_k\right), \qquad g_{k+1} \ge \delta .$$
A componentwise-maximum dynamic programme over the gaps (a valid upper bound, since the maps are monotone) gives the following per-step growth bounds:

| $\delta$ | growth bound per step | proven steps, 31-bit | observed mean / max |
|---|---|---|---|
| 1 | 1.000 (no proof) | none | 27.52 / 41 |
| 2 | 0.852 | 131 | 22.54 / 34 |
| 3 | 0.784 | 87 | 18.16 / 26 |
| 4 | 0.748 | 73 | 16.17 / 23 |

3.3 Termination at $\delta = 1$. The bound is exactly 1, reached on runs of gap 1: there $q$ is a unit and the bound reads $x_{k+1} \le (x_{k-1}+x_k)/2$. This is the Gaussian form of the weak triangle inequality that Wikström says blocks a naive transfer of binary methods. Termination is therefore unproven for $\delta = 1$. It held on every pair with $|\text{coordinates}| \le 10$ (96,800 pairs, at most 6 steps) and on every random case. For $\delta \ge 2$ it is proven by the table above. In $\mathbb{Z}$ the same recurrence at gap 1 already contracts: $x_{k+1} \le (x_{k-1} + x_k)/4$, with rate $\frac{1+\sqrt{17}}{8} = \left(\frac{\sqrt{17}-1}{2}\right)^{-1}$, which is Stehlé–Zimmermann's constant. In $\mathbb{Z}[i]$ a gap of one $\pi$ gains only $\sqrt2$ in size, and one extra digit of quotient precision buys the missing margin.

3.4 Low bits only. With $\delta = 4$ the valuation-driven step comes within 10% of the size-guided Hensel step's mean (16.2 against 14.9) and has the same proven bound (73). Its quotients depend only on low digits. A perturbation test (`t8.py`, 1,500 trials per $\delta$) changed the inputs by $\pi^M X$. The first quotient that differed always came at a step where $2v(B) \ge M - 15$. So the quotients up to valuation $v$ depend only on the inputs mod $\pi^{2v + O(1)}$, as in Stehlé–Zimmermann's Lemma. This was tested, not proven.

## 4. The quartic residue symbol

4.1 Laws used. For an odd prime $P$, $\left[\frac{\alpha}{P}\right] \equiv \alpha^{(NP-1)/4} \pmod P$, a power of $i$. For odd $\beta$ the symbol is extended multiplicatively over the prime factors. It depends only on the ideal $(\beta)$, so $\beta$ may be made primary at no cost. For primary $\beta = c+di$ and primary $\alpha = a+bi$:
$$\left[\tfrac{i}{\beta}\right] = i^{-(c-1)/2},\qquad \left[\tfrac{1+i}{\beta}\right] = i^{(c-d-d^2-1)/4},\qquad \left[\tfrac{\alpha}{\beta}\right] = (-1)^{\frac{a-1}{2}\cdot\frac{c-1}{2}}\left[\tfrac{\beta}{\alpha}\right].$$
These were checked against Euler's criterion for composite primary moduli: 3,000 moduli for the supplements and 2,780 coprime pairs for reciprocity, with no failures (`t1.py`). Every exponent depends only on $c, d \bmod 16$ and $a \bmod 4$.

4.2 One lemma covers every step. For any $q \in \mathbb{Z}[i]$, primary $y$ and $x - qy = i^s\pi^t x' \ne 0$ with $x'$ primary,
$$\left[\tfrac{x}{y}\right] = i^{\,t\,k_1(y) + s\,k_i(y)}\left[\tfrac{x'}{y}\right],$$
where $i^{k_1(y)}$ and $i^{k_i(y)}$ are the two supplements. This holds because $x \equiv x - qy \pmod y$. The quotient never enters, so the size rule and the choice of $q$ are free. The symbol follows any of the gcd loops, with one reciprocity sign each time the roles of the two values swap. It ends at $y = 1$ with $i^e$, or at $x - qy = 0$ with $y \ne 1$, which gives 0. The extra work per step is $t \bmod 4$ and $s$ (both already found by strip and primary), a 256-entry table or a little arithmetic on $(c \bmod 16, d \bmod 16)$, and one AND of bit 1 of the two real parts for the reciprocity sign. That is about 10 operations, an estimate that was not counted in the op model.

4.3 Verification. Four symbol algorithms were written: W ($\varepsilon$ step, Weilert and Damgård–Frandsen), H (section 2), E (Euclid with nearest rounding, Eisenstein's scheme) and S (section 3). Each was checked against Euler's criterion via the factorisation:

- exhaustively over all 234 primary moduli of norm at most 600, each in all 4 associates, with every $a$ in $|a|, |b| \le 8$: 269,568 cases;
- on 7,000 random cases with composite moduli of up to four prime factors (up to 30 bits each, split and inert) and 40-bit numerators, including ones divisible by $1+i$ and ones sharing a factor;
- for mutual agreement, on 3,000 random 31-bit pairs (2,639 nonzero symbols).

There were no mismatches in any algorithm. Step counts are the gcd counts in sections 2 and 3: the symbol halves the steps exactly as the gcd does (W 29.4, H 14.8 to 15.2, S at $\delta = 4$ 16.2, E 14.7).

4.4 Why $\mathbb{Z}[i]$ is easier than $\mathbb{Z}$ here. Brent and Zimmermann's difficulty with the binary Jacobi symbol in $\mathbb{Z}$ is archimedean. Reciprocity needs positive arguments, so they need positive quotients, and they remark that a centred binary quotient "does not appear to give a subquadratic algorithm". In $\mathbb{Z}[i]$ the role of "positive" is played by "primary", which is a congruence mod $\pi^3$, and every correction in 4.1 depends only on residues mod 16. So the symbol's running exponent is a function of the low digits of the quotient sequence alone. The valuation-driven sequence of section 3 therefore carries the symbol with no sign problem. That is exactly the quotient sequence Weilert's 2002 remark says was missing.

## 5. Assessment

5.1 Already known (or very likely known):

- the subtract-and-strip gcd and quartic symbol in $O(n^2)$ (Weilert 2000, Damgård–Frandsen 2003, FLINT);
- the quartic laws;
- the Euclidean $n^{1+o(1)}$ symbol (Weilert 2002);
- the integer prototypes of every step here: Stehlé–Zimmermann binary division, the one-sided Hensel remainder, and Sorenson's $k$-ary reduction (with a Gaussian $k$-ary credited to Weilert).

That the symbol bookkeeping is indifferent to the quotient (4.2) is elementary, and was already implicit in Eisenstein's 1844 scheme as Bach and Sandlund present it.

5.2 New here, as far as the readable literature shows:

- a clean statement of the one-sided Hensel-quotient step for $\mathbb{Z}[i]$ with a size-guided precision, and a proven bound of 133 steps for 31-bit inputs at study 33's offset (instead of about 960), falling to 73 at $c = 10$;
- the valuation-driven Gaussian Stehlé–Zimmermann step, with the observation that $\delta = 1$ sits exactly at the weak-triangle threshold and that $\delta \ge 2$ gives a proven linear bound;
- the quartic symbol carried by both steps, verified against the definition;
- the observation that in $\mathbb{Z}[i]$, unlike $\mathbb{Z}$, all symbol corrections are 2-adic, which removes the obstacle that made the binary Jacobi symbol delicate.

For one word, H and S halve W's step count (about 15 against 29) at the cost of a mod-$2^j$ inverse per step. Study 33 found that trade worth about 36% in its op model.

5.3 Is it a publishable note? Possibly, but not in its present form. The content that would justify a note is a binary recursive quartic symbol: the section 3 sequence with $\delta \ge 2$, a proven truncation lemma (3.4), the half-gcd recursion and an $O(M(n)\log n)$ bound. That would answer Weilert's remark directly, much as Brent and Zimmermann did for $\mathbb{Z}$, and it should be simpler than his top-down $n^{1+o(1)}$ algorithm. None of the recursion has been written or proven here; it is only plausible from the tests. An exposition note (the Hensel step, its bound, and the symbol) on its own is probably too slight for a journal, though it would suit a section of the book. Before any novelty claim, four caveats apply:

- Weilert's three papers and Agarwal–Frandsen remain unread, and Weilert's $k$-ary variant may already contain the one-sided step;
- the $\delta = 1$ termination question is open;
- the bounds are linear but loose (3 to 6 times the observed worst cases);
- nothing has been timed, and the per-step inverse is a dependent multiply chain.

The first action should be to get the JSC 2000 and JNT 2002 papers through a library.
