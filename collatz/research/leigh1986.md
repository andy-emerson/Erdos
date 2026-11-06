# Leigh 1986: what it contains and what it changes in `invariants-prior-art.md`

Read 5 November 2026 from a scan supplied by the user (10 PDF pages, two journal pages to a sheet, no text layer). Every page was read as an image, and the pages quoted below were reread at 200 dpi. Quotations are transcriptions of the scan; formulas are rendered in the notation of `invariants-prior-art.md`. Page numbers are journal page numbers. PDF page 1 holds p. 125, and PDF page $s \ge 2$ holds pp. $2s + 122$ and $2s + 123$. The PDF was not copied into the repository.

## 1. Citation

G. M. Leigh (Brisbane, Australia), A Markov process underlying the generalized Syracuse algorithm, *Acta Arithmetica* XLVI (1986), no. 2, pp. 125 to 143. The title, author and volume are on p. 125; the issue number is in the footer of p. 139 ("4 - Acta Arithmetica XLVI.2"); the paper ends on p. 143 with "Received on 7. 6. 1984 (1431)". The citation in `invariants-prior-art.md` Section 2.2 (46 (1986), 125 to 143) is correct. The date 1985 used by Lagarias and Matthews is not supported by the paper itself, which carries 1986 and a 1984 receipt date.

## 2. What Leigh proves and conjectures

### 2.1 Setting

The setting is $\mathbb{Z}$ only, with an arbitrary modulus $d \ge 2$, prime or composite. Nothing in the paper concerns rings of algebraic integers, $\mathbb{Z}[\sqrt 2]$, norms, or units. The map is defined on p. 125:

> "Let $d \ge 2$ and $m_0, \ldots, m_{d-1}$ be non-zero integers. Let $r_0, \ldots, r_{d-1}$ be integers with $r_i \equiv i m_i \pmod d$. $T\colon Z \to Z$ is defined by (1) $T(x) = \frac{m_i x - r_i}{d}$ if $x \equiv i \pmod d$ $(0 \le i < d)$." (p. 125)

Leigh places it against Matthews and Watts:

> "Only the case $(m_i, d) = 1$ for all $i$ was considered in [2]. In [3] this was extended to $(m_i, d^2) = (m_i, d)$. Also in [3], they used results on Markov matrices, without observing that there was an actual Markov chain involved." (p. 125)

> "In this paper no restriction will be placed on the $m_i$, except that they be non-zero." (p. 125)

So the $m_i$ may be negative (Example 1 has a branch $(-4x - 7)/3$, p. 139) and may share factors with $d$ (the example (11) on p. 128 is $x/2$ on even $x$ and $5x + 1$ on odd $x$, that is $d = 2$, $m_1 = 10$).

The object of study is the residue-class frequency of divergent trajectories, where divergent means not eventually periodic:

> "Given a modulus $m \ge 2$, the aim is to predict the limiting frequencies $\lim_{N \to \infty} f_{Nj}$, where (2) $f_{Nj} = \frac1N \operatorname{card}\{n;\ 0 \le n < N,\ T^n(x) \equiv j \pmod m\}$, for trajectories $\{T^n(x)\}_{n \ge 0}$ which are not eventually periodic; these are called divergent trajectories." (p. 125)

### 2.2 The Markov chains (proved)

Leigh puts $l = [m, d]$, extends $T$ to the $l$-adic integers, and works with Haar measure:

> "To permit the application of the theory of probability, we put $l = [m, d]$ and extend $T$ to a mapping from the set of $l$-adic integers, $Z_l$, into itself." (p. 126)

> "Markov chains $\{X_n\}$ and $\{Y_n\}$ are defined over the probability space $(Z_l, P)$ where $P$ is the Haar measure on $Z_l$." (p. 126)

The states are residue classes $B(j, M) = \{x \in Z_l : x \equiv j \pmod M\}$ (p. 126). $Y_n$ is the class of $T^n(x)$ determined by $X_n$ and $X_{n+1} = H(Y_n)$ is its image under $T$ (pp. 127 to 128, (5) to (9)). The proved results are:

- Theorem 1: "$\{X_n\}$ is an infinite Markov chain with stationary transition probabilities" (p. 128), with the quantities $p_{nij}$ of (3) (the proportion of classes modulo $md^n$ in class $i$ modulo $m$ whose $n$th iterate lies in class $j$) given by the $n$-step transition probabilities, (13).
- Theorem 2: $\{Y_n\}$ is a Markov chain (p. 128).
- Theorem 4: if the chains enter positive classes with probability one, the Cesàro limits of $p_{nij}$ are given by the limiting probabilities, (22) to (27) (p. 129), which "corresponds to equation (1.4) of [3]".
- Theorem 5: for Haar-almost every $l$-adic $x$ whose chain enters a positive class, $\lim_N f_{Nj} = p_j$ with $p_j$ given by (29) (p. 130).
- Theorem 6: "If $\{X_n\}$ and $\{Y_n\}$ are finite Markov chains, each enters a positive class with probability one." (p. 130)
- Lemma 3, unconditional and about integers: "If $\{T^n(x)\}$ is a divergent trajectory with $T^n(x) \equiv i_n \pmod d$ $(0 \le i_n < d)$ then (60) $|T^n(x)|^{1/n} \sim \prod_{k=0}^{n-1} |m_{i_k}/d|^{1/n}$ as $n \to \infty$ and (61) $\liminf_{n\to\infty} \prod_{k=0}^{n-1} |m_{i_k}/d|^{1/n} \ge 1$." (p. 136). The proof uses $T^n(x) = \frac{m_{i_0}\cdots m_{i_{n-1}}}{d^n}\, x \prod_{k=0}^{n-1}\{1 - \frac{r_{i_k}}{m_{i_k}T^k(x)}\}$, (62), and "$|T^k(x)| \to \infty$ as $k \to \infty$" (pp. 136 to 137).
- Theorem 7: "If $(m_i, d^2) = (m_i, d)$ for all $i$ (so that $d_i \mid d$), the Markov chains are finite and $Y_n$ is a residue class mod $l$ for all $n$." (p. 137). "Most of the results of [3] now follow."
- Theorem 8: "If $(m_i, d) = 1$ for all $i$ (so $d_i = 1$), (63) $P(\{T^n(x)\}$ is uniformly distributed mod $d^\alpha$ for each $\alpha \ge 1) = 1$." "This was proved in [2] (Theorem 3) using ergodic theory." (p. 137)

The remark after Theorem 6 says where the theory is complete:

> "The case where $\{X_n\}$ and $\{Y_n\}$ are finite is very important. Indeed, it is unknown if there is any non-finite example in which $\{X_n\}$ and $\{Y_n\}$ enter positive classes with probability 1. All cases considered by Matthews and Watts correspond to finite Markov chains (see Section 4)." (p. 130)

### 2.3 The conjectures

The passage from $l$-adic to rational integers is conjectural:

> "Results on $l$-adic integers are transferred to rational integers by means of CONJECTURE 1. Any condition on the distribution of iterates which has probability zero, does not occur in divergent trajectories." (p. 126)

> "Corollaries of this conjecture include the following: (i) If a divergent trajectory enters a positive class then $\lim_{N\to\infty} f_{Nj} = p_j$ where $p_j$ are given by (29) (see Theorem 5). (ii) If the Markov chains are finite, every divergent trajectory enters a positive class. (See Theorem 6.) (iii) Following on from (i), take $m = d$. Suppose a trajectory enters a positive class and (4) $\prod_{j=0}^{d-1} |m_j/d|^{p_j} < 1$. Then the trajectory is eventually periodic. This follows from (61) of Lemma 3." (p. 126)

Section 5 gives a heuristic for infinite chains: replace $\mathcal B$ by a subset $\mathcal D$ and treat the resulting process $\tilde X_n$ as a Markov chain, which "is not in general a Markov chain; the essence of this method is that good approximations are obtained by treating it as a Markov chain" (p. 138). Conjecture 2 (p. 138) says that as $\mathcal D \to \mathcal B$ the classes of $\tilde X_n$ correspond to those of $X_n$ and $\lim \tilde p_j = p_j$.

### 2.4 Form of the results

The criterion is a Markov chain criterion: the residue-class frequencies of a divergent trajectory are the limiting probabilities of a positive class of an explicit chain on residue classes (Theorem 5 proved for $l$-adic integers, Conjecture 1 for integers). The divergence criterion is a necessary condition only, in the form (4): a trajectory in a positive class with weighted geometric mean $\prod |m_j/d|^{p_j} < 1$ is eventually periodic. Leigh states no converse. Nowhere does he conjecture that a product above $1$ forces divergent trajectories, or that almost all trajectories diverge, and he states no density result for the set of starting points with a given behaviour beyond the Haar-almost-sure statements of Theorems 5 and 8.

### 2.5 Examples

Example 1 (p. 139): $d = 3$, branches $3x - 1$, $(x - 16)/3$, $(-4x - 7)/3$, $m = 2$, a finite chain with five states, predicted frequencies $1/5$ and $4/5$ for even and odd iterates. Example 2 (pp. 140 to 141): $d = 8$, eight branches, two positive classes $\mathcal C_1$ and $\mathcal C_2$; for $\mathcal C_1$, $p_1 = p_5 = 1/2$ and "$(\frac12)^{1/2}(\frac32)^{1/2} = \sqrt3/2 < 1$, so by Conjecture 1 (iii), every trajectory which enters $\mathcal C_1$ should be eventually periodic" (p. 140), and for $\mathcal C_2$ the predicted frequencies of iterates $\equiv 0, 2, 4, 6 \pmod 8$ are $4/7, 1/7, 1/7, 1/7$ (p. 141). Example 3 (pp. 141 to 142): $d = 6$, an infinite chain approximated with $\mathcal D = \{B(x, 2^a 3^b) : a \le 4\}$, compared with 1000 iterates from $x = 53$. Example 3 was suggested by Matthews (acknowledgement, p. 125).

## 3. The eleven rows

Throughout, "on $\mathbb{Z}$" means the draft's maps over $\mathbb{Z}$ with $\pi = 2$, $q = 2$. Leigh's class (1) contains them. The draft's unaccelerated $T$ (Definition 2.1: $z/\pi$ on even $z$, $\alpha z + d(z)$ on odd $z$, digit depending on the class modulo $2^k$) is (1) with Leigh's modulus $2^k$, $m_i = 2^{k-1}$ on even classes and $m_i = 2^k\alpha$ on odd classes. The encoding of `invariants-prior-art.md` 3.1 ($z/2$ on even classes, $(\alpha z + d)/2^k$ on odd classes) is (1) with $m_i = 2^{k-1}$ and $m_i = \alpha$. In both, $(m_i, d^2) = (m_i, d)$, so Leigh's Theorem 7 applies: the chains are finite, and these maps are in the class of Matthews and Watts [3]. Leigh's own example (11), $x/2$ and $5x + 1$, is the draft's $T$ for $\alpha = 5$, $k = 1$.

### 3.1 Row 1: the window

Partly anticipated, on $\mathbb{Z}$ only.

The upper end. Leigh's (4) with $m = d$ is the weighted form of the growth-rate criterion, and it covers the encoded map at modulus $2^k$, which Matthews's Conjecture 3.1 (relatively prime type) does not. My computation, not in Leigh: for the encoding at modulus $2^k$ with least absolute residue digits, the proportions $p_{nij}$ of (3) with $m = d = 2^k$ are exactly $2^{-k}$ for every $i, j$ (checked by exhaustive count for $(\alpha, k, n) = (7, 2, 5), (5, 2, 5), (13, 3, 3)$; it also follows from the fact that each odd class modulo $2^k$ is mapped onto all of $\mathbb{Z}_2$ and each even class onto a class modulo $2^{k-1}$, so Haar measure is preserved). Then (4) reads $(1/2)^{1/2}(\alpha/2^k)^{1/2} < 1$, that is $\alpha < 2^{k+1} = q^{k + 1/(q-1)}$. So on $\mathbb{Z}$ the upper end of the window is the boundary of Leigh's Corollary (iii) for these maps, with uniform weights. Leigh states this for no specific family, and his criterion runs in one direction only: below the boundary, divergent trajectories are conjectured not to exist. That nothing diverges above the window is not in Leigh. The q-dependence (the exponent $1/(q-1)$ from residue fields of size $q > 2$) and the norm have no counterpart in the paper.

The lower end. Proposition 2.4 on $\mathbb{Z}$ follows from Leigh's Lemma 3, unconditionally: if $\alpha < 2^k$ then every $|m_i/d|$ of the encoding is below $1$ ($1/2$ and $\alpha/2^k$), so (61) fails for every trajectory and there is no divergent trajectory. Leigh does not draw this consequence, but (61) is stated and proved in exactly the form needed (p. 136). The proof of Lemma 3 uses $|T^k(x)| \to \infty$ for a divergent trajectory, which holds in $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$ (finitely many elements of bounded absolute value) but not for absolute value in $\mathbb{Z}[\sqrt 2]$, where the draft's own proof also relies on the norm.

Change to `invariants-prior-art.md`: in 3.1, replace "the criterion that applies is Leigh's Markov-chain Conjecture 7.1 ... [my reading, not checked against Leigh's paper]" by a statement that the encoded map satisfies $(m_i, d^2) = (m_i, d)$, so it is in Matthews and Watts's 1985 class and has finite chains by Leigh's Theorem 7 (p. 137); that Leigh's criterion is Conjecture 1, Corollary (iii), (4) (p. 126), which Matthews numbers 7.1 [unverified: Matthews's wording not compared]; and that the weights are uniform for these maps (my computation), which is what reduces (4) to the window. Add that Leigh's Lemma 3 (61) gives Proposition 2.4 on $\mathbb{Z}$. The verdict stays "known in a special case, and equivalent to a known heuristic in general", with Leigh added to Heppner as the source of the special case on $\mathbb{Z}$ for every $k$ (as a one-directional conjecture). The recommendation "credit ... Matthews and Watts (with Leigh) for the growth-rate heuristic" becomes specific: cite Leigh, Corollary (iii) of Conjecture 1, and Lemma 3.

### 3.2 Row 2: which moduli admit multipliers in the window

Not anticipated. Leigh's $d$ is arbitrary and his $m_i$ are unrestricted; there is no question of which norms occur. Verdict unchanged.

### 3.3 Row 3: at most $|U|$ trivial cycles

Not anticipated. Leigh does not discuss cycles at all; eventually periodic trajectories appear only as the complement of divergent ones. Verdict unchanged.

### 3.4 Row 4: maps commuting with the units

Not anticipated. Leigh's representatives are $0 \le i < d$ (p. 125), the $r_i$ are any integers with $r_i \equiv i m_i \pmod d$, and nothing is said about odd maps or about $x \mapsto -x$. Verdict unchanged.

### 3.5 Row 5: forced fixed points

Not anticipated. No fixed points are computed in the paper. Verdict unchanged.

### 3.6 Row 6: smallest digits

Not anticipated. The congruence $r_i \equiv i m_i \pmod d$ leaves the $r_i$ free and the paper never chooses smallest ones; its examples use various $r_i$ (for instance $(x - 16)/3$ on $x \equiv 1 \pmod 3$, p. 139, and $20x - 40$, $20x + 48$ on $x \equiv 2, 4 \pmod 8$, p. 140). The freedom in $r_i$ is visible in Leigh's results: the chain and the frequencies depend only on the $m_i$ and $d$, which is the integer case of the draft's remark that the digits do not affect the density heuristic. Verdict unchanged.

### 3.7 Row 7: unit cycles

Not anticipated. Verdict unchanged.

### 3.8 Row 8: rotated and mirrored maps

Not anticipated. Leigh allows negative $m_i$ but never compares a map with its conjugate by $x \mapsto -x$ or with a map whose multipliers differ in sign. Verdict unchanged.

### 3.9 Row 9: cycles at best approximations

Not anticipated. Verdict unchanged.

### 3.10 Row 10: divergence on invariant lines

Partly anticipated as a method, not in substance. Leigh's positive classes are invariant unions of residue classes, and a map can have several, with different frequencies and different values of (4): in Example 2, $\mathcal C_1$ has product $\sqrt 3/2 < 1$ and is predicted to contain no divergent trajectory, while $\mathcal C_2$ carries the predicted divergent behaviour (pp. 140 to 141). The draft's two-state drift along an invariant line is the same computation, a finite Markov chain on states with a weighted product of multipliers, applied to a one-dimensional invariant subset of $\mathcal O_K$ rather than to a union of residue classes of $\mathbb{Z}$. Leigh has no invariant lines, no higher-rank lattice and no ring other than $\mathbb{Z}$. Change: add Leigh's Example 2 and the positive-class formalism to the remark in 3.10 as the integer form of "different invariant regions, different drift". Verdict stays "closely related precedents".

### 3.11 Row 11: fixed points of $mx \pm 1$ at modulus $2^k$

Not anticipated. Verdict unchanged.

## 4. Units, symmetry, conjugation, residues, norm form

Units and symmetry under units. Nothing. The only appearance of sign is the decomposition $x = C(x)D(x)$ on p. 127: "For non-zero integers $x$, the functions $C(x)$ and $D(x)$ are defined by: $C(x) \in Z$, $D(x) \in N$, $C(x)D(x) = x$, with $(C(x), d) = 1$ and $D(x)$ a divisor of some power of $d$. For example, if $d = 10$ then $C(-360) = -9$ and $D(-360) = 40$." The sign is carried by the part prime to $d$, which is the integer analogue of putting the unit into the odd part, but it is used only for bookkeeping in Lemma 1 (pp. 130 to 132).

Symmetry under $x \mapsto -x$. Nothing. Negative multipliers are allowed (p. 125, Example 1) and absolute values $|m_j/d|$ enter (4) and Lemma 3, so the criterion is blind to signs of the $m_j$, but this is not remarked.

Conjugation. Nothing on Galois conjugation (the paper is on $\mathbb{Z}$) or on conjugating maps by affine changes of variable. The only relation between chains is Theorem 3, linking $\{X_n\}$ and $\{Y_n\}$.

Least absolute residues. Nothing. Class representatives are $0 \le i < d$ throughout.

The window in norm form. Nothing. The criterion is in absolute values on $\mathbb{Z}$, (4) and (60) to (61); there is no norm, no residue field size $q$ and no exponent $1/(q-1)$.

Leigh's 1983 map on $\mathbb{Z}[\sqrt 2]$ (`invariants-prior-art.md` 2.2, Matthews's Example 10.1) is not mentioned. The paper gives no support for the statement that Leigh was the first to consider rings of algebraic integers; that attribution still rests on Siegel and Matthews.

## 5. References

The bibliography (p. 143) has three entries:

- [1] K. L. Chung, *Markov Chains with Stationary Transition Probabilities*, Springer-Verlag, Berlin 1960.
- [2] K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arith.* 43 (1984), pp. 167 to 175.
- [3] K. R. Matthews and A. M. Watts, A Markov approach to the generalized Syracuse algorithm, ibid. 45 (1985), pp. 29 to 42.

Chung is a textbook on Markov chains, used for Theorems 4 to 6 (pp. 135 to 136), and bears on no row. Matthews and Watts 1984 and 1985 were already known and not reached (`invariants-prior-art.md` Section 1). Leigh's account makes them more relevant to row 1 than that file assumed: [3] treats exactly the condition $(m_i, d^2) = (m_i, d)$ satisfied by the draft's maps on $\mathbb{Z}$, and equation (1.4) of [3] is the formula for the limits of $p_{nij}$ (p. 126, p. 129). [2], Theorem 3, is the uniform distribution result for $(m_i, d) = 1$ (p. 137). Reading [3] remains the most useful next step for row 1. Leigh cites neither Hasse, Möller nor Heppner, and nothing on rings other than $\mathbb{Z}$.

## 6. Summary of changes to `invariants-prior-art.md`

1. Section 1: Leigh 1986 moves from "not reached" to read (scan supplied by the user). The citation is confirmed as *Acta Arith.* 46 (1986), no. 2, 125 to 143, received 7 June 1984.
2. Section 2.2: confirmed that the Acta paper is on $\mathbb{Z}$ only, with arbitrary modulus $d$, and does not mention $\mathbb{Z}[\sqrt 2]$ or rings of integers.
3. Row 1: Leigh's Conjecture 1, Corollary (iii), (4), with uniform weights (my computation) gives the upper end on $\mathbb{Z}$ for every $k$, as a one-directional conjecture (below it, no divergence). Leigh's Lemma 3, (61), gives Proposition 2.4 on $\mathbb{Z}$ unconditionally. The draft's maps satisfy $(m_i, d^2) = (m_i, d)$, so they are in Matthews and Watts's 1985 class and Leigh's Theorem 7 makes their chains finite; the sentence that Matthews and Watts's conjecture does not cover them should say instead that their 1985 paper does, and that the relevant criterion is Leigh's (4). Verdict unchanged in kind; credit Leigh alongside Heppner.
4. Row 10: add Leigh's Example 2 (two positive classes, one with weighted product $\sqrt3/2 < 1$) as the integer precedent for computing drift separately on invariant pieces. Verdict unchanged.
5. Rows 2 to 9 and 11: Leigh has nothing on them. Verdicts unchanged.
6. For the draft (`collatz/papers/collatz-rings.md`, Section 1.1 and Section 2.2): Leigh 1986 can be cited directly for the Markov chain criterion and Lemma 3, and the sentence "Leigh is reported to have been the first to consider such maps on rings of algebraic integers" should keep its attribution to Siegel and Matthews, since the Acta paper does not bear it out.
