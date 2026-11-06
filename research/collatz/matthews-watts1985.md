# Matthews and Watts 1985: what it contains and what it changes in `invariants-prior-art.md`

Read 5 November 2026 from a scan supplied by the user (8 PDF pages in landscape, two journal pages to a sheet, no text layer). Every page was read as an image, and the pages quoted below were reread at 220 dpi, one journal page at a time. Quotations are my transcriptions of the scan. Page numbers are journal page numbers: PDF page 1 holds p. 29 (its left half is blank), PDF page $s$ for $2 \le s \le 7$ holds pp. $2s + 26$ and $2s + 27$, and PDF page 8 holds p. 42 and, on its right half, the first page of an unrelated paper (C. W. Lloyd-Smith, Algebraic numbers near the unit circle). The PDF was not copied into the repository. The computations of Section 3 were run from scratchpad scripts, not kept in the repository.

## 1. Citation

K. R. Matthews and A. M. Watts (Brisbane, Australia), A Markov approach to the generalized Syracuse algorithm, *Acta Arithmetica* XLV (1985), no. 1, pp. 29 to 42. Title, authors and volume are on p. 29; the issue is in the footer of p. 33 ("3 - Acta Arithmetica XLV.1"); the paper ends on p. 42 with "Received on 6. 4. 1983 and in revised form on 17. 10. 1983 (1351)". This is the paper expected, and the citation in Leigh's bibliography ([3] in `leigh1986.md` Section 5) is correct. The acknowledgement thanks "R. N. Buttsworth and G. Leigh for some useful comments" (p. 42).

## 2. Setting, conjectures and theorems

### 2.1 The maps

> "Let $d, m_0, \ldots, m_{d-1}$ be positive integers; $R$ is a complete set of residues mod $d$; $r_i \in R$ is defined for $i = 0, \ldots, d-1$ by $r_i \equiv i m_i \pmod d$. Then $T\colon Z \to Z$ is defined by (1.1) $T(x) = \frac{m_i x - r_i}{d}$ if $x \equiv i \pmod d$." (p. 29)

So the setting is $\mathbb{Z}$, any modulus $d$, and positive multipliers. Leigh's later paper drops positivity ("non-zero", Leigh p. 125). Taken literally, (1.1) also asks that all $r_i$ come from one complete residue system $R$, so two branches whose $i m_i$ are congruent must use the same $r_i$. The paper's own examples do not keep to this (Section 5.4).

The relation to the 1984 paper:

> "In this paper we extend some of the conjectures and results of our recent paper [4], which dealt with the special case where $\gcd(m_i, d) = 1$ for $i = 0, \ldots, d-1$." (p. 29)

> "We are interested in the distribution among the congruence classes mod $m$ of the sequence of iterates $T^K(n)$, $K \ge 0$, where the sequence is not eventually periodic. We call such sequences 'divergent trajectories', this being the nomenclature of a forth-coming survey [3] on the $3x+1$ problem by J. Lagarias. Only the case $m = d^\alpha$ was discussed in [4]." (p. 29)

### 2.2 The Markov matrix and condition C

$B(j, m) = \{x \in Z \mid x \equiv j \pmod m\}$, and

> "If $p_{jk}(m)$ is the number of congruence classes mod $md$ contained in $T^{-1}(B(j, m)) \cap B(k, m)$ and $q_{jk}(m) = p_{jk}(m)/d$, then $Q(m) = [q_{jk}(m)]$ is a Markov matrix whose ergodic sets are in 1-1 correspondence with the minimal $T$-invariant sets mod $m$, the correspondence being $S' = \{B(i_1, m), \ldots, B(i_t, m)\} \leftrightarrow S = B(i_1, m) \cup \ldots \cup B(i_t, m)$." (p. 29)

$Q(m)$ is column-stochastic (Lemma 2.9, p. 34: "a non-negative matrix, each of whose column sums equals 1"): column $k$ is the class one comes from, row $j$ the class one goes to. The condition that the paper adds to the 1984 setting:

> "If $\gcd(m_i, d) = 1$ for $i = 0, \ldots, d-1$, or if $\gcd(m_i, d^\alpha) = \gcd(m_i, d)$ for $\alpha \ge 1$, $i = 0, \ldots, d-1$ and $d \mid m$ (we call this condition C), then we can show that if $p_{Kjk}(m)$ is the number of congruence classes mod $md^K$ contained in $T^{-K}(B(j, m)) \cap B(k, m)$, then (1.2) $[p_{Kjk}(m)] = [p_{jk}(m)]^K$." (pp. 29 to 30)

In the notation of Lemma 2.5 (p. 33), condition C is "$\beta = 0$, or $\beta = 1$ and $d \mid m$", where $\beta$ is the least exponent with $\gcd(m_i, d^\alpha) = \gcd(m_i, d^\beta)$ for all $\alpha \ge \beta$. The case $\beta \le 1$ is Leigh's condition $(m_i, d^2) = (m_i, d)$ of his Theorem 7, which confirms what `leigh1986.md` inferred from Leigh's account.

### 2.3 The conjectures

> "Corresponding to conjecture (iv) of our paper [4], we have CONJECTURE 4. If condition C holds, a divergent trajectory eventually enters a minimal $T$-invariant set $S$ mod $m$. If $B(j, m) \subseteq S$ the limiting frequency (1.3) $\lim_{N\to\infty} \frac1N \operatorname{card}\{K \le N \mid T^K(n) \in B(j, m)\}$ exists and equals $\mu_{S'}(B(j, m))$, the $B(j, m)$-component of the stationary vector of $Q(m)$ which corresponds to the ergodic set $S'$." (p. 30)

> "If condition C does not hold, the limiting frequencies still appear to exist, but the simple interpretation in terms of stationary vectors may not hold." (p. 30)

> "Corresponding to conjectures (i) and (ii) of our paper [4], we have the following more general conjectures. CONJECTURE 1. Let condition C hold and let $S$ be a minimal $T$-invariant set mod $d$. Then if (1.5) $\prod_{B(i,d) \in S'} \left(\frac{m_i}{d}\right)^{\mu_{S'}(B(i,d))} < 1$, all trajectories starting in $S$ will eventually be periodic. CONJECTURE 2. If (1.6) $\prod_{B(i,d) \in S'} \left(\frac{m_i}{d}\right)^{\mu_{S'}(B(i,d))} > 1$, almost all trajectories starting in $S$ will be divergent." (p. 30)

There is no Conjecture 3; the numbers follow those of the 1984 paper. The product condition is two-sided: below $1$, every trajectory in $S$ is eventually periodic; above $1$, almost all diverge. This is the divergence half that Leigh's Corollary (iii) lacks (`leigh1986.md` 2.4). "Almost all" is not defined in the paper. The product is a weighted geometric mean of $m_i/d$, with weights from the stationary vector of $Q(d)$ restricted to one ergodic set, not the unweighted product $m_0 \cdots m_{d-1} < d^d$ of the relatively prime case.

### 2.4 Theorems

> "Lemma 2 of [4] remains valid and corresponding to Theorem 1 of [4] we have THEOREM 1.1. Suppose that the sequence $(T^K(n))_{K \ge 0}$ lies in $S$ (a minimal $T$-invariant set mod $d$) and is unbounded. Also suppose that for $B(i, d) \in S'$, $\lim_{N\to\infty} \frac1N \operatorname{card}\{K \le N \mid T^K(n) \in B(i, d)\} = \mu_{S'}(B(i, d))$. Then (1.7) (a) $\prod_{B(i,d) \in S'} (m_i/d)^{\mu_{S'}(B(i,d))} \ge 1$, (1.8) (b) $\lim_{K\to\infty} |T^K(n)|^{1/K} = \prod_{B(i,d) \in S'} (m_i/d)^{\mu_{S'}(B(i,d))}$." (pp. 30 to 31)

No proof is given in this paper; it is referred to [4]. The rest is proved:

- Lemma 2.1 (p. 31): $T^{-1}(B(j, m))$ is a disjoint union of $\sum \gcd(m_i, m)$ classes modulo $md$, the sum over those $i$ with $\gcd(m_i, m) \mid j - M_i$, where $M_i = (m_i i - r_i)/d$. The proof rests on (2.2), $T(B(x, md)) = B(\frac{m_i x - r_i}{d}, m_i m)$.
- Corollary 2.2 (p. 31): if $\gcd(m_i, m) = 1$ for all $i$, $T^{-1}(B(j, m))$ is a union of $d$ classes modulo $md$.
- Lemmas 2.3 to 2.8 (pp. 32 to 34): the Chinese remainder theorem, the count $p_{jk}(n, m)$, the stabilization of $\gcd(m_i, d^\alpha)$, and (2.9) $[p_{Kjk}(m)] = [p_{jk}(m)]^K$ when $\beta = 0$, or $\beta = 1$ and $d \mid m$.
- Lemma 2.9 (p. 34): column sums of $Q(m)$ equal $1$.
- Lemma 3.1 (p. 35): $S \mapsto S'$ is a bijection between $T$-invariant sets modulo $m$ and closed sets of $Q(m)$, minimal ones going to ergodic sets.
- Lemma 3.2 (p. 35): "If $\gcd(m_i, m) = 1$ for $i = 0, \ldots, d-1$, then $Q(m)$ is doubly stochastic", with the remark that then "the stationary vector of $Q(m)$ which corresponds to an ergodic set $S'$ of $Q(m)$ has its non-zero components equal to $1/n$, where $n = \operatorname{card} S'$."
- Lemma 3.3 and Corollary 3.4 (p. 36): if $m \mid m_i$ for some $i$ with $\gcd(m_i, d) = 1$, $Q(m)$ (and $Q(m^k)$) has one ergodic set, primitive when all $\gcd(m_i, d) = 1$.
- Lemma 3.5 and Corollary 3.6 (pp. 37 to 39): for $\gcd(m_i, m) = 1$, the structure of a periodic ergodic block and the existence of the limit (3.15), $\lim_K \operatorname{card}_{md^K}(T^{-K}(B(j,m)) \cap S)/\operatorname{card}_{md^K}(S) = \mu_{S'}(B(j, m))$. The text on p. 30 says that this limit, not just the Cesàro mean (1.4), "appears" to exist in general, "but we are unable to prove this, except in the case where $\gcd(m_i, m) = 1$ for $i = 0, 1, \ldots, d-1$."

### 2.5 Examples (Section 4, pp. 39 to 42)

1. $x/2$, $(5x+1)/2$: "There appears to be one ergodic set for each $m \ge 2$, though we cannot prove this in general. $n = 7$ appears to give the first divergent trajectory." (p. 40)
2. $x/2$, $(5x-3)/2$: "There appears to be one ergodic set if $3 \nmid m$, two ergodic sets if $3 \mid m$." (p. 40)
3. $x/3$, $(2x-2)/3$, $(13x-2)/3$: "As expected, all trajectories appear to end up in cycles; there appear to be six cycles only. As a curiosity, the trajectory starting with $n = 338$ takes 7161 iterations to reach the cycle beginning with 2; also the maximum value of $T^K(338)$ is $T^{2726}(338)$, a number with 73 digits." (p. 40)
4. $x/3 - 1$, $(x+5)/3$, $10x - 5$ modulo $3$: $Q(3)$ primitive, stationary vector $[\frac12, \frac14, \frac14]$, product $(\frac13)^{1/2}(\frac13)^{1/4}(\frac{30}{3})^{1/4} < 1$, "so we expect all trajectories to eventually cycle. There appear to be five cycles." (p. 40)
5. $3x/2$, $(x+1)/2$, $x/2 + 1$, $(7x+1)/2$ on the classes $0, 1, 2, 3$ modulo $4$: two ergodic sets $S'_1 = \{B(0,4), B(2,4)\}$ and $S'_2 = \{B(1,4), B(3,4)\}$, both with stationary vector $[\frac12, \frac12]$, products $(\frac32)^{1/2}(\frac12)^{1/2} < 1$ and $(\frac12)^{1/2}(\frac72)^{1/2} > 1$. "Hence we expect all trajectories starting with an even integer to eventually cycle, while most trajectories starting with an odd integer should be divergent." (pp. 40 to 41)
6. $3x/2 + 1$, $(x-1)/2$, $x/2$, $(7x-1)/2$ modulo $4$: "$Q(4)$ is periodic with period 2 and has only one ergodic set, $Z_4$. The stationary vector is $[\frac14, \frac14, \frac14, \frac14]'$ and we expect most trajectories to be divergent as" $(\frac32)^{1/4}(\frac12)^{1/4}(\frac12)^{1/4}(\frac72)^{1/4} > 1$ (p. 41).
7. "(Conway [1], p. 121)" $3x/2$, $(3x+1)/4$, $3x/2$, $(3x-1)/4$ modulo $4$, "(This transformation is 1-1.)", $Q(4)$ primitive, stationary vector uniform, product $(\frac32)^{1/4}(\frac34)^{1/4}(\frac32)^{1/4}(\frac34)^{1/4} > 1$, "Hence we expect most trajectories to be unbounded." (p. 42)

## 3. What the paper gives for the draft's maps on $\mathbb{Z}$

### 3.1 Encoding

The draft's accelerated map $S$ (multiply by $m$, add the digit $d(x)$ with $2^k \mid mx + d(x)$, divide by $2$ while possible) is not of the form (1.1) at any modulus, because the number of divisions is unbounded. The orbit of $S$ is a subsequence of the orbit of a map that is of the form (1.1) at $d = 2^k$, in two ways:

- the encoding of `invariants-prior-art.md` 3.1: $x/2 = 2^{k-1}x/2^k$ on the even classes and $(mx + d(x))/2^k$ on the odd ones, so $m_i = 2^{k-1}$, $r_i = 0$ for even $i$ and $m_i = m$, $r_i = -d(i)$ for odd $i$;
- the draft's unaccelerated $T$: $x/2$ on the even classes and $mx + d(x) = (2^k m x + 2^k d(x))/2^k$ on the odd ones, so $m_i = 2^{k-1}$ and $m_i = 2^k m$.

In both, $r_i \equiv i m_i \pmod{2^k}$ holds by the choice of digit. The single-$R$ requirement of (1.1) holds too: the $r_i$ of odd classes lie in distinct odd classes by Lemma 2.2 of the draft, and the even classes all use $r_i = 0$. Condition C holds with $\beta = 1$ (for $k \ge 2$): $\gcd(2^{k-1}, 2^{k\alpha}) = 2^{k-1}$, $\gcd(m, 2^{k\alpha}) = 1$, and $\gcd(2^k m, 2^{k\alpha}) = 2^k$ for every $\alpha \ge 1$, and Conjectures 1 and 2 take $m = d$. For $k = 1$ the first encoding ($x/2$ and $(mx + d)/2$) is of relatively prime type and was already in the 1984 paper; for $k \ge 2$ the even branch has $\gcd(2^{k-1}, 2^k) \ne 1$ and only the 1985 paper covers it. This corrects `invariants-prior-art.md` 3.1, which said that the encoded map is covered by no conjecture of Matthews and Watts. Negative $m$ (for example $-7x \pm 1$) is outside the 1985 setting, which takes $m_i > 0$, and inside Leigh's.

### 3.2 The stationary vector and the product

For the first encoding, $Q(2^k)$ is doubly stochastic with one ergodic set $Z_{2^k}$, so the weights are $2^{-k}$ on every class. Lemma 3.2 does not give this, since $\gcd(m_i, 2^k) \ne 1$ on the even classes. My argument: $T^{-1}(B(j, 2^k))$ meets the even classes in $B(2j, 2^{k+1})$, of Haar measure $2^{-k-1}$, and each of the $2^{k-1}$ odd classes in one class modulo $2^{2k}$, of measure $2^{-2k}$, so its measure is $2^{-k}$; and every class reaches an odd class by halving, while every odd class reaches every class. Then

$$\prod_{i} \left(\frac{m_i}{2^k}\right)^{2^{-k}} = \left(\frac12\right)^{1/2}\left(\frac{m}{2^k}\right)^{1/2} = \left(\frac{m}{2^{k+1}}\right)^{1/2},$$

which is below $1$ exactly when $m < 2^{k+1}$. Here the weighted product agrees with the unweighted one, $m_0 \cdots m_{d-1} = 2^{(k-1)2^{k-1}} m^{2^{k-1}} < 2^{k 2^k}$, because the weights are uniform.

For the second encoding the weights are not uniform: the odd classes carry total weight $1/(k+2)$ and class $0$ carries $1/3$ at $k = 4$, for instance. The product is $(2^{-(k+1)} m)^{1/(k+2)}$, with the same threshold $m < 2^{k+1}$. The unweighted product gives the wrong answer here (for $3x + 1$ it is $(1 \cdot 6)^{1/2}/2 = 1.22 > 1$), so it is the weighting of the 1985 paper that makes the conjecture independent of how the map is written.

Computed exactly, by counting the classes modulo $md$ in $T^{-1}(B(j, m)) \cap B(k, m)$ with $m = d = 2^k$ and least absolute residue digits (in every case the eigenvalue $1$ of $Q(d)$ is simple, so there is one ergodic set):

| map | $k$ | weights (first encoding) | product, first encoding | product, second encoding |
|---|---|---|---|---|
| $3x + 1$ | 1 | $\frac12, \frac12$ | $(3/4)^{1/2} = 0.866$ | $(3/4)^{1/3} = 0.909$ |
| $5x + 1$ | 1 | $\frac12, \frac12$ | $(5/4)^{1/2} = 1.118$ | $(5/4)^{1/3} = 1.077$ |
| $7x \pm 1$ | 2 | $\frac14$ each | $(7/8)^{1/2} = 0.935$ | $(7/8)^{1/4} = 0.967$ |
| $9x \pm 1$ | 2 | $\frac14$ each | $(9/8)^{1/2} = 1.061$ | $(9/8)^{1/4} = 1.030$ |
| $21x + d$ | 4 | $\frac1{16}$ each | $(21/32)^{1/2} = 0.810$ | $(21/32)^{1/6} = 0.932$ |
| $29x + d$ | 4 | $\frac1{16}$ each | $(29/32)^{1/2} = 0.952$ | $(29/32)^{1/6} = 0.984$ |
| $31x + d$ | 4 | $\frac1{16}$ each | $(31/32)^{1/2} = 0.984$ | $(31/32)^{1/6} = 0.995$ |
| $33x + d$ | 4 | $\frac1{16}$ each | $(33/32)^{1/2} = 1.016$ | $(33/32)^{1/6} = 1.005$ |

For $7x \pm 1$ the first encoding is $m_i = (2, 7, 2, 7)$, $r_i = (0, -1, 0, 1)$ on the classes $0, 1, 2, 3$ modulo $4$, with $R = \{-1, 0, 1, 2\}$. For $21x + d$ at $k = 4$ the digits are $-5, 1, 7, -3, 3, -7, -1, 5$ on the classes $1, 3, \ldots, 15$.

So for these maps Conjecture 1 predicts that every trajectory is eventually periodic when $m < 2^{k+1}$, and Conjecture 2 predicts that almost all trajectories diverge when $m > 2^{k+1}$. Example 1 of the paper, $5x + 1$ at $k = 1$, is the first case above the line, with "$n = 7$" as the first apparent divergent trajectory (p. 40).

### 3.3 The two ends of the window

The paper reproduces the upper end of the window on $\mathbb{Z}$, for every $k$, and in both directions. It does not reproduce the lower end. Nothing in the product condition changes at $m = 2^k$: the product is $(m/2^{k+1})^{1/2}$, continuous in $m$, and Conjecture 1 makes the same prediction on both sides of $2^k$. Below $2^k$, every $m_i/d$ is below $1$ and the map contracts in the worst case, which is the draft's Proposition 2.4. The paper states neither this nor its consequence for Theorem 1.1, whose hypothesis (1.7) then fails for every set of weights. The lower end has to be credited elsewhere: it is elementary, and Leigh's Lemma 3, (61), gives it on $\mathbb{Z}$ unconditionally (`leigh1986.md` 3.1).

Theorem 1.1 (b) also gives the draft's drift. With the first encoding, half the steps are odd in the stationary measure, so $2\log_2$ of the product, $\log_2(m/2^k) - 1$, is the drift per odd step of Definition 2.7 with $q = 2$.

### 3.4 The norm form

The norm form is a direct rewriting, though not one the paper makes. Replace $\mathbb{Z}$ by $R$, $d$ by $\pi^k$, and $m_i/d$ by $N(m_i)/N(\pi^k)$. The even classes are a fraction $1/q$ of all classes, with ratio $N(\pi^{k-1})/N(\pi^k) = 1/q$, and the odd ones a fraction $(q-1)/q$, with ratio $N(\alpha)/q^k$. The same Haar measure argument makes the weights uniform, and the product is
$$\left(\frac1q\right)^{1/q}\left(\frac{N(\alpha)}{q^k}\right)^{(q-1)/q} < 1 \iff N(\alpha) < q^{k + 1/(q-1)}.$$
I checked this on $\mathbb{Z}[i]$ at modulus $(1+i)^4 = -4$, building $Q$ by counting the $256$ classes modulo $16$ for $\alpha = 5 + 2i, 3, 1 + 4i, 5 + 3i$. In each case $Q$ was doubly stochastic with one ergodic set, and the product of norms equalled $(N(\alpha)/32)^{1/2}$. Two points need saying when the conjecture is carried over. First, (1.8) is stated for $|T^K(n)|$, and for imaginary quadratic rings $N(z) = |z|^2$, so the rewriting is exact. Second, "divergent" means "not eventually periodic", and that is the same as unbounded only when there are finitely many elements of bounded size; that holds for $\mathbb{Z}$, $\mathbb{Z}[i]$ and $\mathbb{Z}[\omega]$, not for $\mathbb{Z}[\sqrt 2]$ with the absolute value. The exponent $1/(q-1)$ for $q > 2$ has no counterpart in the paper, whose divisor $d$ plays both parts. At $q = d$, $k = 1$, it is Heppner's form.

## 4. The eleven rows

### 4.1 Row 1: the window

Anticipated on $\mathbb{Z}$ for the upper end, at every $k$ and in both directions. The paper does not contain the lower end, the norm or the exponent $1/(q-1)$.

Change to `invariants-prior-art.md` 3.1. Replace the sentences "The encoded map is not of relatively prime type ..., so Conjecture 3.1 does not cover it, and the criterion that applies is Leigh's Markov-chain Conjecture 7.1 ... [my reading, not checked against Leigh's paper]". They should say that the encoded map satisfies condition C (Matthews and Watts 1985, pp. 29 to 30) with $\beta = 1$ and $m = d$, so that Conjectures 1 and 2 (p. 30) apply. Its Markov matrix $Q(2^k)$ is doubly stochastic with one ergodic set (my computation; the paper's Lemma 3.2 does not cover it), and the weighted product is $(m/2^{k+1})^{1/2}$. Leigh's Corollary (iii) is the one-sided form of Conjecture 1 for a wider class. The verdict becomes "known on $\mathbb{Z}$ (Matthews and Watts 1985, Conjectures 1 and 2, every $k$; Heppner at $k = 1$); the lower end elementary (Leigh's Lemma 3 on $\mathbb{Z}$); the norm form and the exponent $1/(q-1)$ a direct rewriting, not found". Recommendation: cite Matthews and Watts 1985 as the source of the upper end, and keep the norm form as the draft's rewriting.

### 4.2 Row 2: which moduli admit multipliers in the window

Not anticipated. On $\mathbb{Z}$ every modulus admits multipliers; the question needs norms. Verdict unchanged.

### 4.3 Row 3: at most $|U|$ trivial cycles

Not anticipated. Cycles appear only as counts in Examples 3 and 4 ("six cycles only", "five cycles", p. 40), with no theory and no units. Verdict unchanged.

### 4.4 Row 4: maps commuting with the units

Not anticipated. Example 7 (p. 42) is Conway's permutation from Guy's book, $3x/2$ on even $x$ and $(3x \pm 1)/4$ on the odd classes, which is an odd function at modulus $4$, but the paper says only that it is one-to-one and predicts that most trajectories are unbounded. It fits the draft's remark that the known odd maps with a power of $2$ have modulus $4$ or more, and can be added as a further source for that map (it is OEIS A006368). Positive $m_i$ are required, so the odd maps with negative multiplier are outside the paper. Verdict unchanged.

### 4.5 Row 5: forced fixed points

Not anticipated, but Example 2 is an instance. For $x/2$, $(5x-3)/2$ the paper finds "two ergodic sets if $3 \mid m$" (p. 40): the multiples of $3$ are invariant. This is the integer form of Lemma 3.3 of the draft: $2^2 - 5 = -1$ is a unit, and the map $5x - 3$ at modulus $2$ fixes $3$ ($5 \cdot 3 - 3 = 12 = 4 \cdot 3$), with the multiples of the digit as the invariant set, the mechanism of Crandall's $qx + r$ (`invariants-prior-art.md` 3.5). The paper does not draw either conclusion. Change: optionally add Example 2 to the remark in 3.5 as an early instance. Verdict unchanged.

### 4.6 Row 6: smallest digits

Not anticipated. $R$ is any complete residue system, and the examples use various $r_i$ (Section 5.4). Lemma 2.1 shows where the digits enter: through $M_i = (m_i i - r_i)/d$ modulo $\gcd(m_i, m)$ only, so they drop out of $Q(m)$ when $\gcd(m_i, m) = 1$. For the draft's maps the odd branches have $\gcd(m, 2^k) = 1$ and the even ones have $r_i = 0$, so the weights do not depend on the digit table. This is the integer case of the draft's remark that digits do not affect the density heuristic. Verdict unchanged.

### 4.7 Row 7: unit cycles

Not anticipated. Verdict unchanged.

### 4.8 Row 8: rotated and mirrored maps

Not anticipated. The paper never compares a map with its conjugate by $x \mapsto -x$, and its positive $m_i$ exclude the change of sign of the multiplier. Verdict unchanged.

### 4.9 Row 9: cycles at best approximations

Not anticipated. Verdict unchanged.

### 4.10 Row 10: divergence on invariant lines

Closely related precedent, earlier than Leigh's. Example 5 (pp. 40 to 41) has two minimal invariant sets modulo $4$, the even and the odd integers, with products $(3/4)^{1/2} < 1$ and $(7/4)^{1/2} > 1$, and the prediction that even starts cycle while most odd starts diverge. This is "different invariant pieces, different drift" on $\mathbb{Z}$, with unions of residue classes in place of lines. Change: in 3.10 add Matthews and Watts 1985, Example 5, beside Leigh's Example 2. Verdict unchanged.

### 4.11 Row 11: fixed points of $mx \pm 1$ at modulus $2^k$

Not anticipated. Example 6 has the branches $(x-1)/2$ and $(7x-1)/2$ on the classes $1$ and $3$ modulo $4$ but is not Barina's map, and no fixed points are computed. Verdict unchanged.

## 5. Negative multipliers, symmetry, cycles and digits

### 5.1 Negative multipliers

Excluded: "$d, m_0, \ldots, m_{d-1}$ be positive integers" (p. 29). The products (1.5) to (1.8) are written without absolute values. Leigh's extension to non-zero $m_i$ uses $|m_j/d|$, which gives $-m$ the same product as $m$.

### 5.2 Symmetry under $x \mapsto -x$

Nothing. Example 7 is odd, and the paper does not say so.

### 5.3 Cycles

Only as counts in Examples 3 and 4, and the long transient of $338$ in Example 3. No statement about trivial cycles, scaled cycles or the cycle equation.

### 5.4 Digit choice

$R$ is "a complete set of residues mod $d$" (p. 29), so the $r_i$ are fixed by $R$, but the examples do not keep to one $R$. In Example 4, $x/3 - 1 = (x - 3)/3$ and $10x - 5 = (30x - 15)/3$ give $r_0 = 3$ and $r_2 = 15$, both $\equiv 0 \pmod 3$; in Example 5, $3x/2 = 6x/4$ and $x/2 + 1 = (2x + 4)/4$ give $r_0 = 0$ and $r_2 = -4$; in Example 6, $r_0 = -4$ and $r_2 = 0$. In practice the paper uses Leigh's condition (any integers $r_i \equiv i m_i$). No least absolute residues, and no discussion of how the choice affects cycles.

## 6. References

The bibliography (p. 42) has four entries:

- [1] R. K. Guy, *Unsolved Problems in Number Theory*, Springer-Verlag, New York 1981. Cited for Example 7 at p. 121. Not seen here; it is the source for Conway's permutation as a $3x+1$-type map, and Guy's problem E16 may bear on the history in Section 1.1 of the draft [unverified: problem number from memory].
- [2] D. L. Isaacson and R. W. Madsen, *Markov Chains: Theory and Applications*, Wiley, New York 1976. A textbook, cited at pp. 43, 49 and 78 for closed sets, transient classes and the Cesàro interpretation (1.4). It bears on no row.
- [3] J. Lagarias, The '3x+1' problem and its generalizations, to appear in *Amer. Math. Monthly*. Already cited by the draft as [Lagarias 1985].
- [4] K. R. Matthews and A. M. Watts, A generalization of Hasse's generalization of the Syracuse algorithm, *Acta Arith.* 43 (1984), pp. 167 to 175. Already cited by the draft; not read. Its conjectures (i), (ii) and (iv) and its Theorem 1 and Lemma 2 are the ones the 1985 paper generalizes.

The paper cites neither Hasse, Möller nor Heppner, and nothing on rings other than $\mathbb{Z}$.

## 7. Summary of changes

1. `invariants-prior-art.md` Section 1: Matthews and Watts 1985 moves from "not reached" to read (scan supplied by the user), *Acta Arith.* 45 (1985), no. 1, 29 to 42, received 6 April 1983, revised 17 October 1983.
2. Row 1: the upper end of the window on $\mathbb{Z}$, for every $k$, is the dividing line of Conjectures 1 and 2 of Matthews and Watts 1985 (p. 30), which predict eventual periodicity below it and divergence of almost all trajectories above it. The draft's maps satisfy their condition C, and the weights are uniform (my computation). The earlier statement that their conjectures do not cover the draft's maps was wrong, as Leigh's paper already suggested. The lower end is not in the paper. The norm form is a direct rewriting (checked on $\mathbb{Z}[i]$ at $(1+i)^4$), not found in print.
3. Row 5: Example 2 ($5x - 3$, invariant multiples of $3$) is an instance of the forced fixed point, not remarked by the authors.
4. Row 4: Example 7 is Conway's odd permutation at modulus $4$, with no remark on symmetry.
5. Row 10: Example 5 (even and odd integers as separate invariant sets, products below and above $1$) is the earliest integer precedent found for drift computed piece by piece.
6. Rows 2, 3, 6, 7, 8, 9 and 11: nothing. Verdicts unchanged.
7. For the draft, Section 1.1: the sentence attributing to Matthews and Watts 1984 "a conjecture that almost all trajectories cycle if $|m_0 \cdots m_{d-1}| < d^d$" misstates the convergent half. In the 1985 form (and, by its own account, in the 1984 one) it says that all trajectories are eventually periodic. Add [Matthews and Watts 1985] to the references and cite it for maps not of relatively prime type.
8. For the draft, Section 2.2: cite Matthews and Watts 1985, Conjectures 1 and 2, for the upper end on $\mathbb{Z}$ at every $k$, in place of "the growth-rate heuristic of Matthews and Watts". Keep Heppner for $k = 1$ and claim only the rewriting to the norm and to $q > 2$.
